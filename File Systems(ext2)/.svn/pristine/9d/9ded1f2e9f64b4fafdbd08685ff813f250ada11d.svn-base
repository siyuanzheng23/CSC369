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



int main(int argc, char **argv) {
	if(argc != 3) {
        fprintf(stderr, "Usage: %s <image file name>  <absolute path to the file or link>\n", argv[0]);
        exit(1);
    }
    int disk_fd = open(argv[1], O_RDWR);
    if(disk_fd == -1){
        printf("%s is an invalid disk image path\n", argv[1]);
        return ENOENT;
    }
    disk = mmap(NULL, 128 * EXT2_BLOCK_SIZE , PROT_READ | PROT_WRITE, MAP_SHARED, disk_fd, 0);
    if(disk == MAP_FAILED) {
        perror("mmap for disk image filed\n");
        exit(1);
    }

    char * path = argv[2];
    int directory_inode = findDirectoryInodeNum(disk, path); 
    if(directory_inode != -1){
      printf("This path is a directory\n");
      return ENOENT;
    }

    if(findFileInodeNum(disk, path) != -1){
     printf("File to restore exist!\n");
      return EEXIST;
    }

    //struct ext2_super_block *sb = (struct ext2_super_block *)(disk + EXT2_BLOCK_SIZE);
    struct ext2_group_desc *bg = (struct ext2_group_desc *)(disk + EXT2_BLOCK_SIZE * 2);
    unsigned char  *inode_table = index(disk,bg->bg_inode_table);

    //The absolute path to the file that we want to recover ..(e.g. /level1/file.txt)
    char *full_path_to_file = argv[2];
    //Directory to the file (e.g. /level1)
    char *directory_to_file = ShortenPath(full_path_to_file);
    //e.g. file.txt
    char *file_name = getNewDirName(full_path_to_file);

    //Find if the directory containing the file exist
    int directory_inode_num = findDirectoryInodeNum(disk, directory_to_file);
    if(directory_inode_num == -1){
        printf("Path to file does not exist..\n");
        return ENOENT;
    }

    //Inode of the directory that has file we want to recover..
    struct ext2_inode *d_inode = (struct ext2_inode *) inode(inode_table, directory_inode_num);

    //note: recall that rec_len += dir_entry->rec_len until rec_len excesses the block size
    //there are 12 direct blocks for the directory inode and there is no indirect block

 
 
    for(int i = 0;  i < (d_inode->i_blocks / (EXT2_BLOCK_SIZE / 512)); i ++){
        //only consider the first 12 blocks as direct blocks,  there is no indirect block
        //current block of the directory inode
        int cur_block_id = d_inode -> i_block [i];
        printf("Current at data block %d and block id is %d\n", i+1, cur_block_id);
        //cast the address of the block to directory entry
        //struct ext2_dir_entry *previous_entry = (struct ext2_dir_entry *) index(disk, cur_block_id);
        //same, cast the address of the block to directory entry
        struct ext2_dir_entry *d_entry = (struct ext2_dir_entry *) index(disk, cur_block_id);
        //size is used to detech whether reach the end of the data block, i.e., EXT2_BLOCK_SIZE
        unsigned int size = 0;
        //if deleted_entry = 1, file found, 0 otherwise
        //int deleted_entry = 0;

        while ( size < EXT2_BLOCK_SIZE){


            unsigned short rec_length = d_entry -> rec_len; 
            int true_length = get_reclen(d_entry -> name_len);
            int gap = rec_length - true_length;
            int gap_left = gap;

            if(gap == 0){ //no gap
                d_entry = (void *) d_entry + (unsigned short)true_length ; //here true_length equal rec_length
                //dir =  (struct ext2_dir_entry *)((char *)dir + dir->rec_len);
                size += true_length;
                printf("There is no gap and thus moving to the next 'non-hodded' dir_entry...\n");
            
            }else{ //gap
                //previous_entry =  d_entry;

                //d_entry is the entry we need to fix rec_len later...
                struct ext2_dir_entry *potential_entry = (void * ) d_entry + (unsigned short)true_length;
                size += true_length;

                


                while(gap_left > 0){
                    
                    //d_entry = (void * ) d_entry + (unsigned short)true_length;
                    if(potential_entry -> file_type == EXT2_FT_REG_FILE || potential_entry -> file_type == EXT2_FT_SYMLINK){
                        //char buffer to copy the file name
                        // char f_name[EXT2_NAME_LEN] ;
                        if(strlen(file_name) != potential_entry->name_len){
                            printf("The file name has different length from the dicrectory entry length\n");
                            int potential_true_len = get_reclen(potential_entry -> name_len);
                            potential_entry = (void *) potential_entry + potential_true_len;
                            gap_left -= potential_true_len;
                            //size +=  potential_true_len;
                            continue;
                        }


                        // memcpy(f_name, d_entry -> name, d_entry -> name_len);
                        //NOT SURE whether sizeof(file_name)/sizeof(char) works...
                        int the_same =equal_string(potential_entry->name, file_name, strlen(file_name));
                        if(the_same == 0){ //the file name is not the same
                            printf("The file has same length with the directory entry -> name, but different name\n");
                            int potential_true_len = get_reclen(potential_entry -> name_len);
                            potential_entry = (void *) potential_entry + potential_true_len;
                            gap_left -= potential_true_len;
                            //size +=  potential_true_len;
                            continue;
                        
                        //found the inode
                        }else{
                            printf("found the correct file name in this dir_entry \n");

                            unsigned int inode_num = potential_entry -> inode;
                            //checking whether inode is still avaliable
                            if (checkInodeBit(disk,inode_num) == 1){ 
                                printf("Inode of the file used\n");
                                return ENOENT;
                            } 
                            //the inode has not been used by other file yet..
                            //start checking data block
                            
                            struct ext2_inode *inode = (struct ext2_inode *) inode(inode_table, inode_num);
                            //int data_block_flag = 0;
                            int blocks_total = inode -> i_blocks/2;
                            //check indirect block 
                            if(blocks_total > 12){
                                if(checkBlockBit(disk,inode->i_block[12]) == 1){ //this indirect data block has been used
                                    return ENOENT;
                                }
                            }

                            for(int j = 0 ; j < blocks_total ; j++){ //checking all data blocks 
                                if(j <= 11){ //checking direct block
                                    unsigned int cur_data_block_id = inode->i_block[j];
                                    if(checkBlockBit(disk,cur_data_block_id) == 1){ //data block has been used by another file 
                                        printf("Direct case: the %dth direct data block with block id %d of the file has been used\n",j,cur_data_block_id);
                                        return ENOENT;
                                    }
                                }else if(j > 12){ //checking indirect block..
                                    unsigned int block_id_for_blocks = inode->i_block[12];

                                    //for every id in the data block..
                                    int id;
                                    //copy the block id number into id..
                                    memcpy(&id, index(disk,block_id_for_blocks) + sizeof(unsigned int) * (j -13), sizeof(unsigned int));
                                    if(checkBlockBit(disk,id) == 1){ //this indirect data block has been used
                                        printf("Indirect case: the %dth indirect block with id %d of file has been used\n", j, id);
                                        return ENOENT;
                                    }
                                }
                            }


                            //Restore
                            //Step 1, change the rec_len of the d_entry back to its true length

                            d_entry -> rec_len = rec_length - gap_left;
                            potential_entry->rec_len = gap_left;
                            inode -> i_dtime = 0;


                            //put_inode_in_DirInode(disk, directory,freeInodeId, file_name, EXT2_FT_REG_FILE);
                            //STEP 2, toggle all relevant bitmap , inode + data block's bits 
                            //togging relevant bitmap
                            toggleInodeBitmap(disk, potential_entry->inode, 1 );
                            //struct ext2_inode *inode = (struct ext2_inode *) inode(inode_table, inode_num);
                            //int blocks_total = (inode -> i_blocks)/2;
                            if(blocks_total > 12){
                                toggleBlockBitmap(disk, inode->i_block[12], 1);
                            }
                            printf("Now toggling bitmap of all data blocks to 1\n");
                            for(int j = 0 ; j < blocks_total ; j++){ //checking all data blocks 
                                if(j <= 11){
                                    //direct block 
                                    //unsigned int cur_direct_block_id = ;
                                    //toggling those bitmap for direct blocks to 1 ..
                                    toggleBlockBitmap(disk, inode -> i_block[j], 1 );

                                }else if(j>12){
                                    unsigned int block_id_for_indirect_block = inode ->i_block[12];
                                    unsigned char *start_point = index(disk,block_id_for_indirect_block);
                                    //toggling those bitmap for indirect blocks to 1 
                                    int id;
                                    memcpy(&id, start_point + (j - 13), sizeof(unsigned int ));
                                    toggleBlockBitmap(disk, id , 1);
                                 }
                            }


                            //STEP 3 
                            //adjust the rec_len back to its true lec 
                            

                            //setting dtime to 0;
                            
                            return 0;
                        }
                        
                        potential_entry = (void * ) potential_entry + get_reclen(potential_entry->name_len);
                    
                    //This inode is not file type.
                    }else{
                        int potential_true_len = get_reclen(potential_entry -> name_len);
                        potential_entry = (void *) potential_entry + potential_true_len;
                        gap_left -= potential_true_len;
                            //size +=  potential_true_len;
                        continue;
                    }
                
                }//end while gap                
                //Cannot find the entry in this gap..
                size += gap;
            }
        
          
        }

    }
    printf("Failed, already search all the blocks of the directory inode\n");
    return -1;
}





    



    





