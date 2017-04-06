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

    //Make sure there's 3 arguments
    if(argc != 3) {
        fprintf(stderr, "Usage: %s <image file name> <native file name> <disk directory name>\n", argv[0]);
        exit(1);
    }

    //Get the disk.
    int fd = open(argv[1], O_RDWR);
    if(fd == -1){
        printf("No img file found!\n");
        exit(1);
    }

    disk = mmap(NULL, 128 * 1024, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if(disk == MAP_FAILED) {
        perror("mmap");
        exit(1);
    }
    //struct ext2_super_block *sb = (struct ext2_super_block *)(disk + 1024);
    struct ext2_group_desc *group = (struct ext2_group_desc *)(disk + 2048);
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
    char *path = argv[2];
    printf("Checking if this dir exist already....\n");

    int directory_inode = findDirectoryInodeNum(disk, path);
    if(directory_inode != -1){
      //printf("no\n");

        printf("This directory alreadt exist, inode num is: %d\n",directory_inode);
      return EEXIST;
    }
    //printf("checking path to the dir....\n");
    char *no_dir_path = (char *)ShortenPath(path);
    printf("Shorten dir path is %s. Checking if path non exisit\n", no_dir_path);
   
    int dir_path_exist = findDirectoryInodeNum(disk, no_dir_path);
    int dir_path_isfile = findFileInodeNum(disk, no_dir_path);
    if(dir_path_isfile != -1){
        printf("Path to this directory is a file...\n");
        return ENOTDIR;
    }
    if(dir_path_exist == -1){
        printf("Path to this directory doesn't exist...\n");
        return ENOENT;
    }
    printf("ready to make the dir....\n");

    //Free inode to store this idr entry.
    //Modify the info of this entry...
    int inode_num = (int)find_FreeInode(disk);
    toggleInodeBitmap(disk, inode_num, 1);
    unsigned char  *inode_table = index(disk,group->bg_inode_table);
    struct ext2_inode *inode = (struct ext2_inode *) (inode_table + sizeof(struct ext2_inode) * (inode_num -1));
    inode->i_mode = EXT2_S_IFDIR;        /* File mode */
    /* Use 0 as the user id for the assignment. */
    inode->i_uid = 0;         /* Low 16 bits of Owner Uid */
    inode->i_size = 1024;        /* Size in bytes */
   
    inode->i_ctime = (unsigned int)time(NULL);       /* Creation time */
   
    inode->i_dtime = 0;
    inode->i_gid = 0;         /* Low 16 bits of Group Id */
    inode->i_links_count = 0; /* Links count */
    inode-> i_blocks = 2;      /* Blocks count IN DISK SECTORS*/
  
       inode->osd1 = 0;          /* OS dependent 1 */
    //inode->i_block[15];   /* Pointers to blocks */
    inode->i_generation = 0;  /* File version (for NFS) */
    inode->i_file_acl = 0;    /* File ACL */
    inode->i_dir_acl = 0;     /* Directory ACL */
    inode->i_faddr = 0;       /* Fragment address */
    //unsigned int   extra[3];

    inode->i_block[0] = find_FreeBlock(disk);
    printf("\nNew free block for this dir... is: %d\n", inode->i_block[0]);
    toggleBlockBitmap(disk, inode->i_block[0], 1);
    
    char *dir_name = getNewDirName(path);
    struct ext2_dir_entry  *Inode_first_dir = (struct ext2_dir_entry *)index(disk, inode->i_block[0]);
    Inode_first_dir->rec_len = 12;
    //Refresh new dir..
    Inode_first_dir->inode = inode_num;
    Inode_first_dir->name_len = 1;
    Inode_first_dir->file_type = EXT2_FT_DIR;
    Inode_first_dir->name[0] = '.';

    //PArent entry
    Inode_first_dir =  (struct ext2_dir_entry *)((char *)Inode_first_dir + Inode_first_dir->rec_len);
    Inode_first_dir->rec_len = EXT2_BLOCK_SIZE - 12;
    //Refresh new dir..
    Inode_first_dir->inode = dir_path_exist;
   // char *parent = getNewDirName(no_dir_path);
    Inode_first_dir->name_len = 2;
    Inode_first_dir->file_type = EXT2_FT_DIR;
    Inode_first_dir->name[0] = '.';
     Inode_first_dir->name[1] = '.';



    
    
    //Put the inode to the entry...
     //Make a helper.. 
     //disk, path to put that inode..

    
     put_inode_in_DirInode(disk, no_dir_path , inode_num, dir_name, EXT2_FT_DIR);


    //One more dir created..
    group->bg_used_dirs_count++;


    






    //Put this entry to its parent entry..




    
    return 0;
}
