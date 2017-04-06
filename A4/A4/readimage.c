#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <sys/mman.h>
#include "ext2.h"

unsigned char *disk;

#define index(disk, i) (disk+ (i*1024))

void printBit(unsigned char *byte){
    printf(" ");
    //printf("byte is %d\n", (int)byte);
    for(int i = 0; i < 8; i++){
        int bit = *byte & (1 << i); //check bit 1 or 0
        if(bit) printf("1"); 
        else printf("0");
    }
}

int main(int argc, char **argv) {

    if(argc != 2) {
        fprintf(stderr, "Usage: %s <image file name>\n", argv[0]);
        exit(1);
    }
    int fd = open(argv[1], O_RDWR);

    disk = mmap(NULL, 128 * 1024, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if(disk == MAP_FAILED) {
        perror("mmap");
        exit(1);
    }






    struct ext2_super_block *sb = (struct ext2_super_block *)(disk + 1024);
    printf("Inodes: %d\n", sb->s_inodes_count);
    printf("Blocks: %d\n", sb->s_blocks_count);

    struct ext2_group_desc *group = (struct ext2_group_desc *)(disk + 2048);
    printf("Block group:\n");
    printf("    block bitmap: %d\n", group->bg_block_bitmap);
    printf("    inode bitmap: %d\n", group->bg_inode_bitmap);  
    printf("    inode table: %d\n", group->bg_inode_table);  
    printf("    free blocks: %d\n", group->bg_free_blocks_count);  
    printf("    free inodes: %d\n", group->bg_free_inodes_count);  
    printf("    used_dirs: %d\n", group->bg_used_dirs_count);  
    //keep printing (inode_table,...,used_dir) (EX 7 done)


    //Check the value of bitmap..
    
    
    unsigned char *block_bitmap = index(disk, group->bg_block_bitmap);
    unsigned char *inode_bitmap = index(disk, group->bg_inode_bitmap);

     
    
    //EX8
    //how do we figure out how many of these bytes to print out ...??
    //Function of number of blocks..
    //divide # of blocks/8, and we have info on the # of blocks.
    
    printf("Block bitmap:");
    for (int i =0; i < sb->s_blocks_count / 8; i++){
        printBit(block_bitmap + i);
    }
    printf("\nInode bitmap:");
    for (int i =0; i < sb->s_inodes_count / 8; i++){
        printBit(inode_bitmap + i);
    }
    
    
    //struct ext2_inode *inode = (struct ext2_inode *)index(disk, group->bg_inode_table);
    //struct ext2_inode *root = (struct ext2_inode *) &inode[1]; // something keeps the first inode.. if i understand it right..
     
    printf("\n\nInodes:\n"); 

    unsigned char  *inode_table = index(disk,group->bg_inode_table);

    char type = '0';
    int i, j;
    for (i = EXT2_ROOT_INO - 1; i < sb->s_inodes_count; i++){

      //Skip those reserved inodes except root.
      if (i < EXT2_GOOD_OLD_FIRST_INO && i != EXT2_ROOT_INO - 1) {
        continue;
      }

      //locate the inode
      struct ext2_inode *inode = (struct ext2_inode *) (inode_table + sizeof(struct ext2_inode) * i);


      //Skip the empty inode.
      if (inode->i_size == 0) {
        continue;
      }

      //Check the inode type. Either Directory or File.
      if (inode->i_mode == EXT2_S_IFREG) {
        type = 'f';
      } else if (inode->i_mode == EXT2_S_IFDIR) {
        type = 'd';
      }else{
        type = 's';
      }

      printf("[%d] type: %c size: %d links: %d blocks: %d\n", (int) i + 1, type, inode->i_size, inode->i_links_count, inode->i_blocks);
      printf("[%d] Blocks:", i + 1);

      //Get Number of blocks (2 sectors in 1 block)
      //Data block(0 - 11)
      int block_count = (int) inode->i_blocks / (EXT2_BLOCK_SIZE / 512);
      //int blocks;
      for (j = 0; j < block_count; j++) {
        //if (j > 11) {
          //break;
       // }
        printf(" %d", inode->i_block[j]);
      }
      printf("\n");
      //Handle Single Indirect Block (index > 11[12 , 13, 14...], so 12 data blocks.. )
      //http://www.nongnu.org/ext2-doc/ext2.html <- useful to understand indirect block

      //Indirect block contains a block(1024bytes) of blockIDs
      //Block 13 - block 268 is inside this indirect block (256 blocks/128) cuz 1024b can take 256 int if 4bytes a int or 126if
      //8 byte an int 
      /*if (block_count > 12) {
        block_count -= 12; // 12 data block, 
        //1st indirect block
        int indirect_block = inode->i_block[12];
        int* data = (void*) index(disk, indirect_block);
        //int* data = (void*)disk + EXT2_BLOCK_SIZE * indirect_block;
        printf(" %d", indirect_block);
        //printf(" [%d] (", indirect_block);
        printf(" %d", *data);
        while (block_count-- > 0) {
          printf(" %d", *data);
          data++;
        }
        printf(" )");
      }*/
     


     //This inode is a directory....
     
  }
  printf("\nDirectory Blocks:");
  for (i = EXT2_ROOT_INO - 1; i < sb->s_inodes_count; i++){

      //Skip those reserved inodes except root.
      if (i < EXT2_GOOD_OLD_FIRST_INO && i != EXT2_ROOT_INO - 1) {
        continue;
      }



      //locate the inode
      struct ext2_inode *inode = (struct ext2_inode *) (inode_table + sizeof(struct ext2_inode) * i);


      //Skip the empty inode.
      if (inode->i_size == 0) {
        continue;
      }

      //Check the inode type. Either Directory or File.
  if (inode->i_mode & EXT2_S_IFDIR) {
      
      int block_count = (int) inode->i_blocks / (EXT2_BLOCK_SIZE / 512);
      printf("\nThis inode has size: %d\n", inode->i_size);
      //int blocks;
      for (j = 0; j < block_count; j++) {
        
        
        
        struct ext2_dir_entry  *dir = (struct ext2_dir_entry *)index(disk, inode->i_block[j]);
        int sum =0;
        //struct ext2_dir_entry  *dirr;
        printf("\n    DIR BLOCK NUM: %d (for inode %d)",inode->i_block[j], i+1);
        //int count = 0;
          while(sum < EXT2_BLOCK_SIZE){
          		//dirr = dir + sum;
					sum +=  dir->rec_len;
            
            //struct ext2_dir_entry 
            
                //printf("hiiii\n");
          if ((unsigned int) dir->file_type == EXT2_FT_REG_FILE) {
            type = 'f';
          } 
          else if((unsigned int) dir->file_type == EXT2_FT_DIR) {
            type = 'd';
        }
                  
              

              
				  printf("\nInode: %d rec_len: %d name_len: %d type= %c name=%.*s", dir->inode, dir->rec_len, dir->name_len, type, dir->name_len, dir->name);
	
            
             dir =  (struct ext2_dir_entry *)((char *)dir + dir->rec_len); //<- linux


          }
      }
  }else{
    continue;
  }
}

    printf("\n");
    return 0;
}
