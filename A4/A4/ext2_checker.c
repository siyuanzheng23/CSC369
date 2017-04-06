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
    if(argc != 2) {
        fprintf(stderr, "Usage: %s <image file name>\n", argv[0]);
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
    struct ext2_super_block *sb = (struct ext2_super_block *)(disk + 1024);
    struct ext2_group_desc *group = (struct ext2_group_desc *)(disk + 2048);
    unsigned char *inode_table =  index(disk, group->bg_inode_table);
  

    //Super blocks/ Group block's free inode/blocks number should match bitmap.
    //Trust bitmap and update acordingly.

    int total = 0;


    int free_inode_count = 0;
    unsigned char *inode_bitmap = index(disk, group->bg_inode_bitmap);
    //printf("\nInode bitmap:");
    for (int i =0; i < sb->s_inodes_count / 8; i++){

        unsigned char *byte = inode_bitmap + i;
        for(int j = 0; j < 8; j++){
            int bit = *byte & 1 << j; //check bit 1 or 0
            if(bit){
                continue;
            }else{
                free_inode_count++;
            }   
        }
    }

    unsigned char *block_bitmap = index(disk, group->bg_block_bitmap);
    int free_block_count = 0;
    for (int i =0; i < sb->s_blocks_count / 8; i++){
        unsigned char *byte = block_bitmap + i;
        for(int j = 0; j < 8; j++){
            int bit = *byte & 1 << j; //check bit 1 or 0
            if(bit){
                continue;
            }else{
                free_block_count++;
            }   
        }
    }

    int group_block_diff = group->bg_free_blocks_count - free_block_count;
    int sb_block_diff = sb->s_free_blocks_count - free_block_count;

    int group_inode_diff = group->bg_free_inodes_count - free_inode_count;
    int sb_inode_diff = sb->s_free_inodes_count - free_inode_count;

    if(group_block_diff != 0){
        group->bg_free_blocks_count = free_block_count;
        total += abs(group_block_diff);

        printf("block group 's free blocks counter was off by %d compared to the bitmap\n", abs(group_block_diff));
    }

    if(sb_block_diff != 0){
        sb->s_free_blocks_count = free_block_count;
        total += abs(group_block_diff);
        printf("super block 's free blocks counter was off by %d compared to the bitmap\n", abs(sb_block_diff));
    }

    if(group_inode_diff != 0){
        group->bg_free_inodes_count = free_inode_count;
        total += abs(group_inode_diff);
        printf("block group 's free inodes counter was off by %d compared to the bitmap\n", abs(group_inode_diff));
    }

    if(sb_inode_diff != 0){
        sb->s_free_inodes_count = free_inode_count;
        total += abs(group_inode_diff);
        printf("super block 's free inodes counter was off by %d compared to the bitmap\n", abs(sb_inode_diff));
    }


    //b
    //Check if inode imode match directory file_type..
    //Trust i_mode and correct file_type..

    //Going through each dir entry and comparing locating the inode..
    //First, go through each blocks of dir inode

    for (int i = EXT2_ROOT_INO - 1; i < sb->s_inodes_count; i++){

      //Skip those reserved inodes except root.
        if (i < EXT2_GOOD_OLD_FIRST_INO && i != EXT2_ROOT_INO - 1) {
            continue;
        }



        //locate the inode
        struct ext2_inode *inode = (struct ext2_inode *) inode(inode_table, i + 1);


        //Skip the empty inode.
        if (inode->i_size == 0) {
            continue;
        }

        //Check the inode type. Either Directory or File.
        if (inode->i_mode & EXT2_S_IFDIR) {
            int fixed_block = 0;
            int block_count = (int) inode->i_blocks / (EXT2_BLOCK_SIZE / 512);
            //printf("\nThis inode has size: %d\n", inode->i_size);
            //int blocks;
            //loop over each block of this inode..
            for (int j = 0; j < block_count; j++) {

                struct ext2_dir_entry  *dir = (struct ext2_dir_entry *)index(disk, inode->i_block[j]);
                struct ext2_inode *current_inode;
                int sum =0;

                //loop over each entry
                while(sum < EXT2_BLOCK_SIZE){
                    ;
                    sum +=  dir->rec_len;

                            //printf("hiiii\n");
                    current_inode = (struct ext2_inode *) inode(inode_table, dir->inode);

                    if(current_inode->i_mode == EXT2_S_IFLNK){
                        if(dir->file_type != EXT2_FT_SYMLINK){
                            printf("Fixed: Entry type[symlink] vs inode mismatch: inode[%d]\n", dir->inode);
                            total += 1;
                            dir->file_type = EXT2_FT_SYMLINK;
                        }

                    }else if(current_inode->i_mode == EXT2_S_IFREG){
                        if(dir->file_type != EXT2_FT_REG_FILE){
                            printf("Fixed: Entry type[file] vs inode mismatch: inode[%d]\n", dir->inode);
                            total += 1;
                            dir->file_type = EXT2_FT_REG_FILE;
                        }

                    }else if(current_inode->i_mode == EXT2_S_IFDIR){
                        if(dir->file_type != EXT2_FT_DIR ){
                            printf("Fixed: Entry type[dir] vs inode mismatch: inode[%d]\n", dir->inode);
                            total += 1;
                            dir->file_type = EXT2_FT_DIR ;
                        }      

                    }
                    //c
                    int fix_error = fixInode(disk, dir->inode);
                    /*if (fix_error == -1){
                        printf("Can't fix the bit map according to inode number because it's correct..\n");
                    }else*/
                    if (fix_error != -1){
                        printf("Fixed: inode[%d] not marked as in-use.\n", dir->inode);
                        total++;
                    }

                    if(current_inode->i_dtime != 0){
                        printf("Fixed: valid inode marked for deletion: [%d]\n", dir->inode);
                        current_inode->i_dtime = 0;
                        total++;
                    }

                    //Correct block bitmap for file type...
                    if(dir->file_type == EXT2_FT_SYMLINK || dir->file_type == EXT2_FT_REG_FILE){
                        int current_block_count = (int) current_inode->i_blocks / (EXT2_BLOCK_SIZE / 512);
                    
                        //int fix_indirect = 0;
                        int fix_file_blocks = 0;
                        for (int z = 0; z < current_block_count; z++) {
                            if(z < 12 ){
                                int block_error = fixBlock(disk, current_inode->i_block[z]);
                                /*if (block_error == -1){
                                    printf("Can't fix the bit map according to block number cuz it's correct\n");
                                }else*/
                                if (block_error != -1){
                                    printf("Fixed: block[%d] not marked as in-use.\n", inode->i_block[z]);
                                    total++;
                                    fix_file_blocks++;
                                }
                            }else{
                                int indirect_block = current_inode->i_block[12];
                                int block_error;
                                if(z == 12){
                                    //fix_indirect = 1;
                                    block_error = fixBlock(disk, indirect_block);
                                    /*if (block_error == -1){
                                        printf("Can't fix the bit map according to block number cuz it's correct\n");
                                    }else*/
                                    if (block_error != -1){
                                        printf("Fixed: block[%d] not marked as in-use.\n", indirect_block);
                                        total++;
                                        fix_file_blocks++;
                                    }
                                }else{  
                                    int block_id = 0;
                                    memcpy(&block_id,(index(disk,indirect_block) + sizeof(unsigned int) * (z -13)), sizeof(unsigned int));
                                    block_error = fixBlock(disk, block_id);
                                    /*if (block_error == -1){
                                        printf("Can't fix the bit map according to block number cuz it's correct\n");
                                    }else*/
                                    if (block_error != -1){
                                        printf("Fixed: block[%d] not marked as in-use.\n", block_id);
                                        total++;
                                    }
                                }                        
                            }
                        
                        }
                        if(fix_file_blocks != 0){
                            printf("%d in-use data blocks not marker in data bitmap fpr inode: [%d}\n", fix_file_blocks, dir->inode);
                        }
                    }

                     dir =  (struct ext2_dir_entry *)((char *)dir + dir->rec_len); //<- linux
                }


                //Correct block bit map for dir type: Check this block in data block or not
                int block_error = fixBlock(disk, inode->i_block[j]);
                /*if (block_error == -1){
                    printf("Can't fix the bit map according to block number cuz it's correct\n");
                }else*/
                if (block_error != -1){
                    printf("Fixed: inode[%d] not marked as in-use.\n", inode->i_block[j]);
                    total++;
                    fixed_block++;
                }       
            }
            
            if(fixed_block != 0){
                printf("%d in-use data blocks not marker in data bitmap fpr inode: [%d}\n", fixed_block, i + 1);
            }
        }
    
    }
    printf("%d file system inconsistencies repaired!\n", total);
            
    return 0;
}
