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
        fprintf(stderr, "Usage: %s <image file name> <absolute path to file>\n", argv[0]);
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
    unsigned char *inode_table =  index(disk, group->bg_inode_table);
  
    //Find inode number of the directory path to copy into.

    char *file = argv[2];
    int file_inode_num = findFileInodeNum(disk, file);
    //printf("Source file inode is %d\n", source_file_inode);
    if(file_inode_num == -1){
      //printf("no\n");
        printf("This file does not exist. \n");
      return EEXIST;
    }

    struct ext2_inode *file_inode = (struct ext2_inode *) inode(inode_table, file_inode_num);


        file_inode->i_size = 0;
        //file_inode->i_gid = 0;         /* Low 16 bits of Group Id */
        //file_inode->i_links_count = 0; /* Links count */
        //file_inode-> i_blocks = 2 * blocks;      /* Blocks count IN DISK SECTORS*/
        file_inode->i_dtime = 1; //1 indicate it's deleted.
        int blocks = file_inode->i_blocks / 2;
        int i = 0;
        int cleared_indirect = 0;
        //Free inode bitmap
        printf("Removing inode %d\n", file_inode_num);
        toggleInodeBitmap(disk, file_inode_num, 0);
        //Free blocks on bitmap
        while(i < blocks){
            if(i < 12){
                printf("Removing block %d\n", i);
                toggleBlockBitmap(disk, file_inode->i_block[i], 0);
                i++;
            }else{
                //Clear indirect blocl
                if(i == 12){
                    cleared_indirect = file_inode->i_block[i];
                    toggleBlockBitmap(disk, file_inode->i_block[i], 0);
                    i++;
                }else{
                    int id_to_rm = 0;
                    memcpy(&id_to_rm,(index(disk,cleared_indirect) + sizeof(unsigned int) * (i -13)), sizeof(unsigned int));
                    toggleBlockBitmap(disk, id_to_rm , 0);
                    i++;
                }


            }
        }
//int put_inode_in_DirInode(unsigned char *disk, char *path_to_save, int inode_num, char* dir_name, unsigned char mode){
    //char *path = ShortenPath(file);
    //int inode_num = file_inode_num;
    //struct ext2_group_desc *group = (struct ext2_group_desc *)(disk + 2048);
    //unsigned char  *inode_table = index(disk,group->bg_inode_table);
    int to_modify_dir_inode = findDirectoryInodeNum(disk, ShortenPath(file));
    struct ext2_inode *dir_inode = (struct ext2_inode *) inode(inode_table, to_modify_dir_inode);
    
     int block_count = (int) dir_inode->i_blocks / (EXT2_BLOCK_SIZE / 512);
      //printf("This inode is #: %d", i);
      //int blocks;
     //int found_flag = 0;

     //Loop over the directory inode's blocks to put the new inode dir entry.....
    for (int j = 0; j < block_count; j++) {
        printf("At block %d of directory\n", j);
        
        
        
        struct ext2_dir_entry  *dir = (struct ext2_dir_entry *)index(disk, dir_inode->i_block[j]);
        //struct ext2_dir_entry  *last_dir;
        struct ext2_dir_entry *second_dir;
        struct ext2_dir_entry  *next_dir;
        int sum =0;
        while(sum < EXT2_BLOCK_SIZE){
            //check if the file inode is the first dir entry and it's not the first block
            printf("At dir entry: %d\n", sum);
            if(sum == 0 && dir->inode == file_inode_num){
                //This block only have one entry...
                printf("File is at the first entrt of block %d\n", j);
                if(dir->rec_len == 1024){
                    printf("This block only have one entry...\n");
                    dir->inode = 0;
                    //Free this block.
                    toggleBlockBitmap(disk, dir_inode->i_block[j], 0);
                    dir_inode->i_size -= 1024;
                    dir_inode->i_blocks -= 2;
                    break;


                }else{
                    printf("This block has more than one entry...\n");
                    //int current_rec = dir->rec_len;
                    second_dir =  (struct ext2_dir_entry *)((char *)dir + dir->rec_len);
                    dir->inode = second_dir->inode;

                    dir->rec_len += second_dir->rec_len;   /* Directory entry length */
                    dir->name_len = second_dir->rec_len;  /* Name length */
                    dir->file_type = second_dir->file_type;
                    for(int i =0; i < second_dir->name_len; i++){
                        dir->name[i] = second_dir->name[i];
                    }
                    break; 

                }
            
            //This entry has a entry above it..
            }else{
                printf("This entry not at top\n");
                //Not this dir entry, skip to next one.
                if(dir->inode != file_inode_num){
                    /*printf("This block has inode number %d, and file jas %d\n",dir->inode , file_inode_num);
                    
                    last_dir = (struct ext2_dir_entry *)index(disk, dir_inode->i_block[j] + sum);

                    dir =  (struct ext2_dir_entry *)((char *)dir + dir->rec_len);
                    printf("last dir inode: %d, now dir inode: %d\n", last_dir->inode, dir->inode);
                    sum+=dir->rec_len;
                    continue;*/
                    sum+=dir->rec_len;
                    next_dir =  (struct ext2_dir_entry *)((char *)dir + dir->rec_len);
                    printf("This dir inode: %d, next dir inode: %d\n", dir->inode, next_dir->inode);
                    if(next_dir->inode == file_inode_num){
                        printf("We found our dir entry!!\n");
                        dir->rec_len += next_dir->rec_len;
                        break;
                    }else{
                        //sum+=next_dir->rec_len;

                        dir = next_dir;
                    }


                //Found the dir entry..
                }else{
                    /*printf("We found our dir entry!!\n");
                    //Extend the rec_len of last_dir..
                    printf("last dir inode: %d, now dir inode: %d\n", last_dir->inode, dir->inode);
                    last_dir->rec_len += dir->rec_len;
                    break;
*/              printf("We should never reach here...\n");

                }

            }
        }

    }

        //If not..
       
            

}
