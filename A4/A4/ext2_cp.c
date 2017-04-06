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

//For this assignment, for the inode that represents a file , we assume that there is only one single indirect block and no other indirect blocks
//for the inode that represents a directory, we assume that there is no indirect block

unsigned char *disk;
#define index(disk, i) (disk+ (i*1024))


int main(int argc, char **argv) {

    if(argc != 4) {
        fprintf(stderr, "Usage: %s <image file name> <native file name> <disk directory name>\n", argv[0]);
        exit(1);
    }

    int disk_fd = open(argv[1], O_RDWR);
    if(disk_fd == -1){
        printf("No disk image found named: %s\n", argv[1]);
        return ENOENT;
    }
    disk = mmap(NULL, 128 * EXT2_BLOCK_SIZE , PROT_READ | PROT_WRITE, MAP_SHARED, disk_fd, 0);
    if(disk == MAP_FAILED) {
        perror("mmap for disk image filed\n");
        exit(1);
    }

    //open file and check if it exist..
    int file_fd = open(argv[2],O_RDWR);
    if(file_fd == -1){
        printf("No file found in system called: %s\n", argv[2]);
        exit(ENOENT);
    }



    char * file_name;
    file_name = getNewDirName(argv[2]);
    char * directory; 
    directory = argv[3];
    
    char *path = ShortenPath(directory);
    if(findDirectoryInodeNum(disk, path) == -1){
      printf("Path to this directory or file does not exist..\n");
      exit(ENOENT);
    }
    

    int directory_inode = findDirectoryInodeNum(disk, directory); 
    //No such directory..check if this is a file...
    if(directory_inode == -1){
        int dir_path_isfile = findFileInodeNum(disk, directory);
        //Found this path as file..
        if(dir_path_isfile != -1){
            printf("This file exist already, can't make a copy of file using this name.\n");
             exit(EEXIST);
        }else{
            //Given absolute path has a file name..
            file_name = getNewDirName(directory);
            //printf("filename is (%s)\n", file_name);
            //printf("Abs path is to a file...\n");
            directory = path;
            //printf("directory is (%s)\n", directory);

        }
    }

    //Copy this file to the directory.
    

    struct ext2_super_block *sb = (struct ext2_super_block *)(disk + EXT2_BLOCK_SIZE);
    //printf("There are total %d of blocks that can be used by the data \n", sb->s_free_blocks_count - 1);
    struct ext2_group_desc *bg = (struct ext2_group_desc *)(disk + EXT2_BLOCK_SIZE * 2);
    
    int blockRequired = -1;

    int file_size = lseek(file_fd, 0, SEEK_END);

    //printf("The size of the file is %d\n", file_size);

    //Calculate how many data blocks are required for the file, EXCLUDING the indirect block
    if(file_size % EXT2_BLOCK_SIZE == 0){
        blockRequired = file_size/EXT2_BLOCK_SIZE;
    }else{
        blockRequired = file_size/EXT2_BLOCK_SIZE  + 1;
    }

    //printf("The # of data blocks required for the file is %d\n", blockRequired);


    //max_block_id_per_block indicates how many data block number can be stored in one data block
    //int num_of_id_per_block = EXT2_BLOCK_SIZE/sizeof(unsigned int);
    //printf("Maximum of IDs can be stored in one block is %d\n" , num_of_id_per_block);
    //BE AWARE THAT We assumes that for the disk image that has 128 blocks, we can never use up the first indirect pointer block 
    if(blockRequired > sb -> s_free_blocks_count -1 ){ //the file is too large to be copied
        printf("There is insufficient space for the file \n");
        exit(ENOSPC);
    }

    unsigned char* file = mmap(NULL, file_size, PROT_READ | PROT_WRITE, MAP_PRIVATE, file_fd, 0);
    if(file == MAP_FAILED){
        perror("mmap for source file failed\n");
        exit(1);
    }

    //locating the directory inode ...

    //printf("Found Dir Inode#: %d\n", directory_inode);

    unsigned char *inodeTable =  index(disk, bg->bg_inode_table);

    //The first step is to find an unused inode for the file and then copy the file content into data block
    int freeInodeId = find_FreeInode(disk);

    struct ext2_inode *inode = (struct ext2_inode *) inode(inodeTable, freeInodeId);
    //initialize the attributes of inode 
    inode -> i_mode = (unsigned short) EXT2_S_IFREG;
    inode -> i_uid = (unsigned short ) 0;
    inode -> i_ctime = (unsigned int)time(NULL); 
    inode -> i_gid = (unsigned short ) 0;
    inode-> i_dtime = 0;
    inode ->  osd1 = (unsigned int) 0 ;
    inode -> i_generation = (unsigned int) 0;
    inode -> i_file_acl = (unsigned int) 0 ;
    inode -> i_dir_acl = (unsigned int) 0 ;
    inode -> i_faddr = (unsigned int) 0 ;
    inode -> i_size = file_size;
    /*for(int i = 0 ; i < 3; i++){
        inode -> extra[i] = (unsigned int) 0 ;
    }*/

    //indicates that this inode is in use by setting the corresponding inode bitmap high
    toggleInodeBitmap(disk,freeInodeId,1);
    //decreases the inodes_count of super block 
   //sb -> s_inodes_count -= 1;

    int sizeWritten = 0;
    int sizeLeft = file_size;

    int indirect_block = 0;
    
    if(blockRequired > 12){ //There will be usage of blocks for indirection
        //1 indicates that currently it is finding a data block to store the pointers of data block
        //0 indicates that currently it is filling the data block with the pointers of the data block 
        indirect_block = 1;
    }

    int i = 0;

    while(i < blockRequired){
        if(i <= 11){ //the first 12 direct block 
            int curDataBlock = find_FreeBlock(disk);
            inode ->i_block[i] = curDataBlock;
            if(sizeLeft < EXT2_BLOCK_SIZE){
                memset(index(disk,curDataBlock), 0, EXT2_BLOCK_SIZE);
                memcpy(index(disk,curDataBlock), file + sizeWritten , sizeLeft);
                sizeWritten += sizeLeft;
                sizeLeft = 0;
                //printf("sizeWritten should be the same  as file_size ?  %d\n", sizeWritten == file_size);
            }else{
                memcpy(index(disk,curDataBlock), file + sizeWritten , EXT2_BLOCK_SIZE);
                sizeLeft -= EXT2_BLOCK_SIZE;
                sizeWritten += EXT2_BLOCK_SIZE;
            }
            
            toggleBlockBitmap(disk, curDataBlock, 1);
            //sb -> s_blocks_count -= 1; , has been implemented in the function toggleBlockBitmap
            i ++;
            //inode -> i_size += 1024;
            inode -> i_blocks += 2;
        }else{  //start using indirect block
            if(indirect_block == 1){
                inode -> i_block[i] = find_FreeBlock(disk);
                toggleBlockBitmap(disk, inode -> i_block[i], 1);
                indirect_block = 0;
                inode -> i_blocks += 2;
                //DO NOT i++ here, since blockRequired DOES NOT include indirect block
                // int id_left_for_indirect_block = num_of_id_per_block;
                // int id_written_size = 0;
            }else{
                unsigned int block_id_for_data = find_FreeBlock(disk);
                toggleBlockBitmap(disk, block_id_for_data, 1);

                if(sizeLeft < EXT2_BLOCK_SIZE){
                    memset(index(disk,block_id_for_data), 0, EXT2_BLOCK_SIZE);
                    memcpy(index(disk,block_id_for_data), file + sizeWritten, sizeLeft);
                    sizeWritten += sizeLeft;
                    sizeLeft = 0;
                    //printf("sizeWritten should be the same  as file_size ?  %d\n", sizeWritten == file_size);
                }else{
                    memcpy(index(disk,block_id_for_data), file + sizeWritten, EXT2_BLOCK_SIZE);
                    sizeLeft -= EXT2_BLOCK_SIZE;
                    sizeWritten += EXT2_BLOCK_SIZE;
                }
                //getting the id of the block that stores the id of blocks that store data 
                unsigned int block_id_for_blocks = inode->i_block[12];
                //copy the block id into the data block that stores the id of data block 
                memcpy(index(disk,block_id_for_blocks) + sizeof(unsigned int) * (i -12) , &block_id_for_data, sizeof(unsigned int));
                i++;
                //inode -> i_size += 1024;
                inode -> i_blocks += 2;
            }
        }
    }

    //The second step is to make the inode 'belongs' to the directory
    //printf("sizeLeft should equal to 0 ???  %d\n", sizeLeft == 0);

    // 'append' that file inode to the directory inode 
    put_inode_in_DirInode(disk, directory,freeInodeId, file_name, EXT2_FT_REG_FILE);
    close(file_fd);
    
    return 0;
}
