#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <sys/mman.h>
#include "ext2.h"
#include "helper.h"
#include <errno.h>
#include <time.h>


unsigned char *disk;

//#define index(disk, i) (disk+ (i*1024))

/*void printBit(unsigned char *byte){
    printf(" ");
    for(int i = 0; i < 8; i++){
        int bit = *byte & 1 << i; //check bit 1 or 0
        if(bit) printf("1"); 
        else printf("0");
    }
}
*/
int main(int argc, char **argv) {
    char *source_file;
    char *link_file;
    //Make sure there's 3 arguments
    if(argc != 4 && argc != 5 ) {
        fprintf(stderr, "Usage: %s <image file name> -s <Source file> <link file name>\n", argv[0]);
        exit(1);
    }
    if(argc == 5){
        printf("second arg is %s\n", argv[2]);
        if(strcmp(argv[2], "-s") != 0){
            fprintf(stderr, "Usage: %s <image file name> -s <Source file> <link file name>\n", argv[0]);
            exit(1);
        }else{
            source_file = argv[3];
            link_file = argv[4];
        }
    }else{
        source_file = argv[2];
        link_file = argv[3];
    }

    if(strcmp(argv[2], argv[3]) == 0){
        printf("Cannot create link using same file name in same directory\n");
        return -1;
    }



    //Get the disk.
    int fd = open(argv[1], O_RDWR);

    disk = mmap(NULL, 128 * 1024, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if(disk == MAP_FAILED) {
        perror("mmap");
        exit(1);
    }
    struct ext2_super_block *sb = (struct ext2_super_block *)(disk + 1024);
    struct ext2_group_desc *group = (struct ext2_group_desc *)(disk + 2048);
    unsigned char *inode_table =  index(disk, group->bg_inode_table);
      /*  //File to copy.
    FILE * pf;
    
    //Open file to read
    pf = fopen (argv[2], "rb");
    
    //Error if file not found.
    if (pf == NULL) {
   
      errnum = errno;
      return ENOENT;
    }
*/
    //Find inode number of the directory path to copy into.

    printf("Checking if this dir exist already....\n");
    if(findDirectoryInodeNum(disk, ShortenPath(source_file)) == -1){
        fprintf(stderr, "Wrong source file path\n");
        exit(1);
    }
    if(findDirectoryInodeNum(disk, ShortenPath(link_file)) == -1){
        fprintf(stderr, "Wrong source file path\n");
        exit(1);
    }
    int source_file_inode = findFileInodeNum(disk, source_file);
    printf("Source file inode is %d\n", source_file_inode);
    if(source_file_inode == -1){
      //printf("no\n");

        printf("This source file does not exist. Source file is: %s\n", source_file);
      return EEXIST;
    }

    struct ext2_inode *source_inode = (struct ext2_inode *) inode(inode_table, source_file_inode);

    //Not a file, dir or symlink
    if(source_inode->i_mode & EXT2_S_IFDIR){
        printf("This is a diretory..\n");
        return EISDIR;
    }
    //we can do hard link to symlink or hard link file...
    /*
    if(source_inode->i_mode & EXT2_S_IFLNK){
        printf("This is a soft link file..\n");
        return -1;
    }*/

    int link_file_inode = findFileInodeNum(disk, link_file);
    if(link_file_inode != -1){
        struct ext2_inode *link_inode = (struct ext2_inode *) inode(inode_table,link_file_inode);

        if(link_inode->i_mode & EXT2_S_IFDIR){
            printf("This link file is not a file (..\n");
            return EISDIR;
        }else{
            printf("This link file exist...\n");
            return EEXIST;
        }
    }

     /**************************Error checking done..**************************/
        
    char *link_file_name = getNewDirName(link_file);
    //Create Hardlink to normal file...
    if(argc == 4){
        printf("Creating a hard link...\n");
        source_inode -> i_links_count++;
        
        //int link_inode_num = find_FreeInode(disk);
        //toggleInodeBitmap(disk, link_inode_num, 1);
        if(source_inode -> i_mode == EXT2_S_IFREG){
            put_inode_in_DirInode(disk, ShortenPath(link_file), source_file_inode, link_file_name,EXT2_FT_REG_FILE);
        }else{
            put_inode_in_DirInode(disk, ShortenPath(link_file), source_file_inode, link_file_name,EXT2_FT_SYMLINK);
        }
        
    

    //Create a file and store the address of source file there. 
    //means I need to create and inode and set inode attribute and put it to dir..
    }else{
        printf("Creating a soft link...\n");
        int link_inode_num = find_FreeInode(disk);
        toggleInodeBitmap(disk, link_inode_num, 1);
        unsigned char  *inode_table = index(disk,group->bg_inode_table);
        struct ext2_inode *inode = (struct ext2_inode *) inode(inode_table, link_inode_num );
        inode->i_mode = EXT2_S_IFLNK;        /* File mode */
        inode->i_dtime = 0;
        /* Use 0 as the user id for the assignment. */
        inode->i_uid = 0;         /* Low 16 bits of Owner Uid */
        inode->i_size = strlen(source_file);        /* Size in bytes */
       
        inode->i_ctime = (unsigned int)time(NULL);       /* Creation time */
        int blocks = 0;
        if (inode->i_size % 1024 == 0){
            blocks = inode->i_size / 1024;
        }else{
            blocks = (inode->i_size / 1024) + 1;
        }
        //Account for indirect block that used to store IDs.
        if(blocks > 12){
            blocks++;
        }

        if(blocks > sb -> s_free_blocks_count -1 ){ //the file is too large to be copied
            printf("There is insufficient space for the file \n");
            return ENOENT;
        }
       
        inode->i_gid = 0;         /* Low 16 bits of Group Id */
        inode->i_links_count = 0; /* Links count */
        inode-> i_blocks = 2 * blocks;      /* Blocks count IN DISK SECTORS*/
      
        inode->osd1 = 0;          /* OS dependent 1 */
        //inode->i_block[15];   /* Pointers to blocks */
        inode->i_generation = 0;  /* File version (for NFS) */
        inode->i_file_acl = 0;    /* File ACL */
        inode->i_dir_acl = 0;     /* Directory ACL */
        inode->i_faddr = 0;       /* Fragment address */
        //unsigned int   extra[3];

        int byte_written = 0;
        int byte_left = inode->i_size;
        //int indirect_block = 0;
        int i = 0;
        while(i < blocks){
            if(i < 12){
                inode->i_block[i] = find_FreeBlock(disk);
                printf("\nNew free block for this dir... is: %d\n", inode->i_block[i]);
                toggleBlockBitmap(disk, inode->i_block[i], 1);
                if(byte_left >=EXT2_BLOCK_SIZE){

                    //Need to know the size
                    memcpy (index(disk, inode->i_block[i]), source_file + byte_written, EXT2_BLOCK_SIZE);
                    byte_written += EXT2_BLOCK_SIZE;
                    byte_left -= EXT2_BLOCK_SIZE;

                }else{
                    memset(index(disk,inode->i_block[i]), 0, EXT2_BLOCK_SIZE);
                    memcpy (index(disk, inode->i_block[i]), source_file + byte_written, byte_left);
                    byte_written += byte_left;
                    byte_left = 0;

                }
                i++;
            }else{
                if(i == 12){
                    inode -> i_block[i] = find_FreeBlock(disk);
                    toggleBlockBitmap(disk, inode -> i_block[i], 1);
                    i++;
                    //indirect_block = 1;
                // int id_left_for_indirect_block = num_of_id_per_block;
                // int id_written_size = 0;
                }else{
                    unsigned int block_id_for_data = find_FreeBlock(disk);
                    toggleBlockBitmap(disk, block_id_for_data, 1);

                    if(byte_left  < EXT2_BLOCK_SIZE){
                        memset(index(disk,block_id_for_data), 0, EXT2_BLOCK_SIZE);
                        memcpy(index(disk,block_id_for_data), source_file + byte_written, byte_left);
                        byte_written += byte_left;
                        byte_left = 0;
                        //printf("sizeWritten should be the same  as file_size ?  %d\n", sizeWritten == file_size);
                    }else{
                        memcpy(index(disk,block_id_for_data), source_file + byte_written, EXT2_BLOCK_SIZE);
                        byte_written += EXT2_BLOCK_SIZE;
                        byte_left -= EXT2_BLOCK_SIZE;
                    }
                    //getting the id of the block that stores the id of blocks that store data 
                    unsigned int block_id_for_blocks = inode->i_block[12];
                    //copy the block id into the data block that stores the id of data block 
                    memcpy(index(disk,block_id_for_blocks) + sizeof(unsigned int) * (i - 13) , &block_id_for_data, sizeof(unsigned int));
                    i++;
                    //inode -> i_size += 1024;
                    //inode -> i_blocks += 2;
                }
            }
        }

        put_inode_in_DirInode(disk, ShortenPath(link_file), link_inode_num, link_file_name,EXT2_FT_SYMLINK);
    }
    //here//
    //Done..
    
    return 0;
}
