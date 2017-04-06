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
#include <time.h>

unsigned char *disk;

//loop checked by R

void printBit(unsigned char *byte){
    printf(" ");
    for(int i = 0; i < 8; i++){
        int bit = *byte & 1 << i; //check bit 1 or 0
        if(bit) printf("1"); 
        else printf("0");
    }     
}


//returns 1 while the inode_num's correponding bitmap is 1 and 0 otherwise
int checkInodeBit(unsigned char *disk, int inode_id){
    //be aware that inode_num and inode_index are different.
    inode_id = inode_id - 1;
    struct ext2_group_desc *group = (struct ext2_group_desc *)(disk + EXT2_BLOCK_SIZE * 2 );
    //struct ext2_super_block *sb = (struct ext2_super_block *)(disk + EXT2_BLOCK_SIZE);

    unsigned char *inode_bitmap = index(disk, group->bg_inode_bitmap);

    unsigned int count_of_eight = inode_id / 8;
    unsigned int left = inode_id % 8 ;

    unsigned char *byte = inode_bitmap + count_of_eight;
    //e.g. 1 << 5 == 100000
    int bit = *byte & 1 << left; //check bit 1 or 0
    if(bit){
        return 1;
    }else{
        return 0;
    }
}

//returns 1 while the block_num's corresponding bitmap is 1 and 0 otherwise
int checkBlockBit(unsigned char *disk, int block_id){
    //be aware that block_id and block_index are different
    block_id = block_id - 1;
    struct ext2_group_desc *group = (struct ext2_group_desc *)(disk + EXT2_BLOCK_SIZE * 2 );
    //struct ext2_super_block *sb = (struct ext2_super_block *)(disk + EXT2_BLOCK_SIZE);

    unsigned char *block_bitmap = index(disk, group ->bg_block_bitmap);

    unsigned int count_of_eight = block_id / 8;
    unsigned int left = block_id % 8 ;

    unsigned char *byte = block_bitmap + count_of_eight;

    int bit = *byte & 1 << left;
    if(bit){
        return 1;
    }else{
        return 0;
    }
}

char * substring(char *string, int length){
    //char substring[length + 1];
    //printf("length is %d\n", length);
    char* substring = 0;
    substring = (char *) malloc(sizeof(char) * (length + 1));

    /*int c = 0;
     while (c < length) {
      substring[c] = string[c];
      c++;
    }
    */



    memcpy(substring,string,length);
    //otherString[length] = 0;
    substring[length] = '\0';
    return substring;
}

int find_FreeInode(unsigned char *disk){
    struct ext2_group_desc *group = (struct ext2_group_desc *)(disk + 2048);
    struct ext2_super_block *sb = (struct ext2_super_block *)(disk + 1024);

    int counter = 0;
    unsigned char *inode_bitmap = index(disk, group->bg_inode_bitmap);
    //printf("\nInode bitmap:");
    for (int i =0; i < sb->s_inodes_count / 8; i++){

        unsigned char *byte = inode_bitmap + i;
        for(int j = 0; j < 8; j++){
            counter++;
            int bit = *byte & 1 << j; //check bit 1 or 0
            if(bit){
                continue;
            }else{
                return counter;
            }   
        }
    }
    //No Free Inode found.
    return -1;

}


int get_reclen(unsigned char name_len){

    int name_len_int = (int) name_len ; 
    int new_rest = 0;
    if(name_len_int % 4 == 0){
        new_rest = name_len_int;
    }else{
        new_rest = ((name_len_int / 4) * 4) + 4;
    }
    //int new_rest = name_len_int - (name_len_int % 4) + 4;
    //printf("size of: %d, name: %d\n", sizeof(struct ext2_dir_entry) ,name_len_int);
    return (int)(sizeof(struct ext2_dir_entry) + new_rest);

}
int find_FreeBlock(unsigned char *disk){
    struct ext2_group_desc *group = (struct ext2_group_desc *)(disk + 2048);
        struct ext2_super_block *sb = (struct ext2_super_block *)(disk + 1024);

        unsigned char *block_bitmap = index(disk, group->bg_block_bitmap);
        int counter = 0;
    for (int i =0; i < sb->s_blocks_count / 8; i++){
        unsigned char *byte = block_bitmap + i;
        for(int j = 0; j < 8; j++){
            counter++;
            int bit = *byte & 1 << j; //check bit 1 or 0
            if(bit){
                continue;
            }else{
                return counter;
            }   
        }
    }
    //No Free Inode found.
    return -1;

}
int fixInode(unsigned char *disk, int inode_id){
    //Toggle to 1
    inode_id = inode_id - 1;
    struct ext2_group_desc *group = (struct ext2_group_desc *)(disk + EXT2_BLOCK_SIZE * 2 );
    struct ext2_super_block *sb = (struct ext2_super_block *)(disk + EXT2_BLOCK_SIZE);

    unsigned char *inode_bitmap = index(disk, group->bg_inode_bitmap);

    unsigned int count_of_eight = inode_id / 8;
    unsigned int left = inode_id % 8 ;

    unsigned char *byte = inode_bitmap + count_of_eight;
    //e.g. 1 << 5 == 100000
    int bit = *byte & 1 << left; //check bit 1 or 0
    if(bit){
        return -1;
    }else{
        //Set it to one if it's not 1.
        *byte |= (1 << left);
        group->bg_free_inodes_count--;
        sb->s_free_inodes_count--;
        return 0;
    }
      
    return -1;

}
int fixBlock(unsigned char *disk, int block_id){
    //Toggle to 1

    block_id = block_id - 1;
    struct ext2_group_desc *group = (struct ext2_group_desc *)(disk + EXT2_BLOCK_SIZE * 2 );
    struct ext2_super_block *sb = (struct ext2_super_block *)(disk + EXT2_BLOCK_SIZE);

    unsigned char *block_bitmap = index(disk, group ->bg_block_bitmap);

    unsigned int count_of_eight = block_id / 8;
    unsigned int left = block_id % 8 ;

    unsigned char *byte = block_bitmap + count_of_eight;

    int bit = *byte & 1 << left;
    if(bit){
        return -1;
    }else{
        *byte |= (1 << left);
        group->bg_free_blocks_count--;
        sb->s_free_blocks_count--;
        return 0;
    }
    
    return -1;

}
int toggleInodeBitmap(unsigned char *disk, int id, int value){
    //Toggle to 1

        struct ext2_group_desc *group = (struct ext2_group_desc *)(disk + 2048);
        struct ext2_super_block *sb = (struct ext2_super_block *)(disk + 1024);

        int counter = 0;
        unsigned char *inode_bitmap = index(disk, group->bg_inode_bitmap);
        //printf("\nInode bitmap:");
        for (int i =0; i < sb->s_inodes_count / 8; i++){
            unsigned char *byte = inode_bitmap + i;
            for(int j = 0; j < 8; j++){
                counter++;
                //set bit
                if(counter == id){
                    if(value == 1){//Set high
                        group->bg_free_inodes_count--;
                        sb->s_free_inodes_count--;
                        *byte |= (1 << j);
                        return 0;
                    }else{
                        group->bg_free_inodes_count++;
                        sb->s_free_inodes_count++;
                        *byte &= ~(1 << j);
                        return 0;
                    }
                }
               
            }
        }
        return -1;

}
int toggleBlockBitmap(unsigned char *disk, int id, int value){
    //Toggle to 1

        struct ext2_group_desc *group = (struct ext2_group_desc *)(disk + 2048);
        struct ext2_super_block *sb = (struct ext2_super_block *)(disk + 1024);

        int counter = 0;
        unsigned char *block_bitmap = index(disk, group->bg_block_bitmap);
        //printf("\nInode bitmap:");
        for (int i =0; i < sb->s_blocks_count / 8; i++){
            unsigned char *byte = block_bitmap + i;
            for(int j = 0; j < 8; j++){
                counter++;
                //set bit
                if(counter == id){
                    if(value == 1){
                        //printf("counter arrive at %d\n", counter);
                        group->bg_free_blocks_count--;
                        sb->s_free_blocks_count--;
                        *byte |= (1 << j);
                        return 0;
                        //printf("byte is now %d\n", (int)byte);
                    }else{
                        group->bg_free_blocks_count++;
                        sb->s_free_blocks_count++;
                        *byte &= ~(1 << j);
                        return 0;
                    }
                }
               
            }
        }
        return -1;

}

char* concat(char* string1, char* string2){
    //const char* name = "hello";
//const char* extension = ".txt";
    char* name_with_extension;
    //printf("last is %c\n", string1[strlen(string1) - 1]);
    if(string1[strlen(string1) - 1] != '/'){
        //printf("(%s) (%s)\n", string1, string2);
        
        name_with_extension = malloc(strlen(string1)+2+strlen(string2)); /* make space for the new string (should check the return value ...) */
        strcpy(name_with_extension, string1);
        name_with_extension[strlen(string1)] = '/'; /* copy name into the new var */
        strcpy(name_with_extension + strlen(string1) + 1, string2); 
        name_with_extension[strlen(string1)+strlen(string2)+1] = '\0';
    }else{
       // printf("(%s) (%s)\n", string1, string2);
        //char* name_with_extension;
        name_with_extension = malloc(strlen(string1)+1+strlen(string2)); /* make space for the new string (should check the return value ...) */
        strcpy(name_with_extension, string1); /* copy name into the new var */
        strcat(name_with_extension, string2); 
        name_with_extension[strlen(string1)+strlen(string2)] = '\0';
    }
    
    return name_with_extension;
}


int equal_string(char *string1, char *string2, int length){
    //char substring[length + 1];
    int i = 0;
    //printf("checking %s %s\n", string1, string2);
    while(i<length){
        //printf("1: %c, 2: %c\n", string1[i], string2[i]);
        if(string1[i] == string2[i]){
            i++;
            continue;
        }else{
            return 0;
        }

        
    }
    return 1;
}

char* getParent(char * path){
    int path_len = (int)strlen(path);


    //While loop to loop through the path 
    //static var(need refresh in each loop): inode number, directory name, counter for name
    int end = 0;
    for(int i = path_len; i >= 0; i--){
        if(path[i] == '/'){
            end = i;
            break;
        }
    }
    char * new_path = malloc(sizeof(char) * (path_len- end + 1));

    int z = 0;
    for(int j = end + 1; j < path_len; j ++){

        new_path[z] = path[j];
        z++;
    }
    new_path[path_len- end] = '\0';

    return new_path;
}


//Get path to a dir or file.
char* ShortenPath(char * path){
    int path_len = (int)strlen(path);


    //While loop to loop through the path 
    //static var(need refresh in each loop): inode number, directory name, counter for name
    if(strlen(path) == 1 && path[0] == '/'){
        return path;
    }
    int end = 0;
    for(int i = path_len; i >= 0; i--){
        if(path[i] == '/'){
            end = i;
            break;
        }
    }
    if(end == 0){
        char *result = malloc(sizeof(char) * 2);
        result[0] = path[0];
        result[1] = '\0';
        return result;
    }
    char * new_path = malloc(sizeof(char) * (end + 1));
    for(int j = 0; j <= end; j ++){
        new_path[j] = path[j];
    }
    new_path[end] = '\0';

    return new_path;
}

//Get dir or file name..
char* getNewDirName(char * path){

    int path_len = (int)strlen(path);
    //printf("path len is %d\n", path_len);

    if(path_len != 1 && path[path_len - 1] == '/'){
        path[path_len - 1   ] ='\0';
    }

    //While loop to loop through the path 
    //static var(need refresh in each loop): inode number, directory name, counter for name
    int end = -1;
    for(int i = path_len; i >= 0; i--){
        if(path[i] == '/'){
            end = i;
            break;
        }
    }
    char * new_dir = malloc(sizeof(char) * (path_len - end));
    //*new_dir = path_len * '\0';
    int z = 0;
    for(int j = end + 1; j <path_len; j++){
        new_dir[z] = path[j];
        z++;
    }
    new_dir[path_len - end - 1] = '\0';

    return new_dir;
}


int findDirectoryInodeNum(unsigned char *disk, char * path){

    int path_len = (int)strlen(path);

    if(path_len == 1 && path[0] == '/'){
        return 2;  
    }

    //struct ext2_super_block *sb = (struct ext2_super_block *)(disk + 1024);
    struct ext2_group_desc *group = (struct ext2_group_desc *)(disk + 2048);
    unsigned char  *inode_table = index(disk,group->bg_inode_table);

    //While loop to loop through the path 
    //static var(need refresh in each loop): inode number, directory name, counter for name

    int path_counter = 1;
    int path_counter_cp = 1;
    int inode_num = 2;

    //printf("path len is :%d\n", path_len);
    //Root is inode 2.
    

    while(path_counter < path_len){

       // printf("I am at %d\n", path_counter);

        //char[]
        if(inode_num == -1){
            return -1;
        }
        int dir_name_len = 0;
        //Ignore slash..

        //Use to skip a slash, go direct to point to dir name, 
        //Useless for first loop
        if(path[path_counter] == '/'){
            path_counter++;
            path_counter_cp++;
            //printf("next round: %d %d\n", path_counter, path_len);
            continue;
        }
        //Get length of dir
        while(path[path_counter] != '/' && path[path_counter] != '\0'){
          dir_name_len++;
          path_counter++;
        }
        //Skip the next '/'
        /*if((path + path_counter) == '\0'){
          break;
        }*/
        //path_counter = path_counter + 1;
        //get the directort name to compare..
        char * dir_name = substring(path + path_counter_cp, dir_name_len);
        //printf("teminatorrrr: %c\n",dir_name[dir_name_len]);

        path_counter_cp = path_counter;
        //printf("This is our dir name %s\n", dir_name);
        //If previous dir can't find, return -1 directly.

        struct ext2_inode *inode = (struct ext2_inode *) (inode_table + sizeof(struct ext2_inode) * (inode_num - 1));

        
        //Loop through directory entry of this node. And compare the name of dir.
        int block_count = (int) inode->i_blocks / (EXT2_BLOCK_SIZE / 512);
            //int blocks;
        int found_dir = 0;
        for (int j = 0; j < block_count; j++) {
            if(found_dir == 1){
                break;
            }    
            struct ext2_dir_entry  *dir = (struct ext2_dir_entry *)index(disk, inode->i_block[j]);
            int sum =0;
            //printf("64\n");
            
            while(sum < EXT2_BLOCK_SIZE){
              //printf("80\n");
                sum +=  dir->rec_len;   
                if ((unsigned int) dir->file_type == EXT2_FT_DIR) {//This is a dir entry, check if name matching
                    //printf("\nInode: %d rec_len: %d name_len: %d name=%.*s\n", dir->inode, dir->rec_len, dir->name_len, dir->name_len, dir->name);

                    //char * abpath_dir_name = substring((char *)dir->name, (int)dir->name_len);
                   //char * abpath_dir_name = (char *) malloc(sizeof(char) * dir->name_len + 1);
                   //sprintf(abpath_dir_name, "%.*s", dir->name);
                    //printf("This is inode dir name%s\n", abpath_dir_name);
                    
                    if(equal_string(dir_name, dir->name, dir->name_len) == 1 && (strlen(dir_name) == dir->name_len)){
                      inode_num = dir->inode;
                      found_dir = 1;
                      //printf("find dir\n");
                      break;
                    }else{
                      //printf("cant find dir\n");
                     // printf("teminator: %c\n", abpath_dir_name[dir->name_len]);
                      //printf("size: %d, %d", (int)strlen(dir_name), (int)dir->name_len);
                      inode_num = -1;
                      dir =  (struct ext2_dir_entry *)((char *)dir + dir->rec_len); //<- linux
                    }
                }else{//Not a dir
                    inode_num = -1;
                    dir =  (struct ext2_dir_entry *)((char *)dir + dir->rec_len);
                }              
            }
        }

    }

    return inode_num;
    

}
int len(char * word){
    int i =0;
    while(word[i] != '\0' && word[i] != ' '){
        //printf("word %d: %c\n",i, word[i] );
          for (int j = 0; j < 8; j++) {
             //printf("%d", !!((word[i] << j) & 0x80));
            }
  //printf("\n");
        i++;
    }

    return i;
}
int findFileInodeNum(unsigned char *disk, char * path){


    int dir_inode = findDirectoryInodeNum(disk, ShortenPath(path));
    //printf("DIR inode is %d\n", dir_inode);
    if(dir_inode == -1){
        return -1;
    }



    //struct ext2_super_block *sb = (struct ext2_super_block *)(disk + 1024);
    struct ext2_group_desc *group = (struct ext2_group_desc *)(disk + 2048);
    unsigned char  *inode_table = index(disk,group->bg_inode_table);
    char *file_name = getNewDirName(path);

 

    struct ext2_inode *inode = (struct ext2_inode *) inode(inode_table,dir_inode);

    
    //Loop through directory entry of this node. And compare the name of dir.
    int block_count = (int) inode->i_blocks / (EXT2_BLOCK_SIZE / 512);
        //int blocks;
    int found_dir = 0;
    int inode_num;
    //printf("Block count is %d\n", block_count);
    for (int j = 0; j < block_count; j++) {
        if(found_dir == 1){
            break;
        }    
        struct ext2_dir_entry  *dir = (struct ext2_dir_entry *)index(disk, inode->i_block[j]);
        int sum =0;
        //printf("64\n");
        
        while(sum < EXT2_BLOCK_SIZE){
          //printf("80\n");
            sum +=  dir->rec_len;   
            if ((unsigned int) dir->file_type == EXT2_FT_REG_FILE) {
                //printf("I am a file\n");
            //This is a file entry, check if name matching
                //printf("\nInode: %d rec_len: %d name_len: %d name=%.*s\n", dir->inode, dir->rec_len, dir->name_len, dir->name_len, dir->name);

                //char * abpath_dir_name = substring((char *)dir->name, (int)dir->name_len);
               //char * abpath_dir_name = (char *) malloc(sizeof(char) * dir->name_len + 1);
               //sprintf(abpath_dir_name, "%.*s", dir->name);
                //printf("This is inode dir name%s\n", abpath_dir_name);
               // printf("filename length is %d, dir->len is %d\n", len(file_name),dir->name_len);
               // printf("filename is (%s)\n", file_name);
                if(equal_string(file_name, dir->name, dir->name_len) == 1 && (strlen(file_name) == dir->name_len)){
                    //printf("We are equal!\n");
                  inode_num = dir->inode;
                  found_dir = 1;
                  //printf("find dir\n");
                  break;
                }else{
                  //printf("cant find dir\n");
                 // printf("teminator: %c\n", abpath_dir_name[dir->name_len]);
                  //printf("size: %d, %d", (int)strlen(dir_name), (int)dir->name_len);
                  inode_num = -1;
                  dir =  (struct ext2_dir_entry *)((char *)dir + dir->rec_len); //<- linux
                }
            }else{
                inode_num = -1;
                dir =  (struct ext2_dir_entry *)((char *)dir + dir->rec_len);
                //printf("I am not a file\n");
            }              
        }
    }



    return inode_num;
    

}

int put_inode_in_DirInode(unsigned char *disk, char *path_to_save, int inode_num, char* dir_name, unsigned char mode){
    struct ext2_group_desc *group = (struct ext2_group_desc *)(disk + 2048);
    unsigned char  *inode_table = index(disk,group->bg_inode_table);
    int to_modify_dir_inode = findDirectoryInodeNum(disk, path_to_save);
    struct ext2_inode *dir_inode = (struct ext2_inode *) (inode_table + sizeof(struct ext2_inode) * (to_modify_dir_inode - 1));
    
     int block_count = (int) dir_inode->i_blocks / (EXT2_BLOCK_SIZE / 512);
      //printf("This inode is #: %d", i);
      //int blocks;
     int found_flag = 0;

     //Loop over the directory inode's blocks to put the new inode dir entry.....
    for (int j = 0; j < block_count; j++) {
        
        
        
        struct ext2_dir_entry  *dir = (struct ext2_dir_entry *)index(disk, dir_inode->i_block[j]);
        int sum =0;
        //struct ext2_dir_entry  *dirr;
        //printf("\n    DIR BLOCK NUM: %d (for inode %d)",inode->i_block[j], to_modify_dir_inode);
        //int count = 0;
        while(sum < EXT2_BLOCK_SIZE){
                //dirr = dir + sum;
            sum +=  dir->rec_len;
            
            //struct ext2_dir_entry 
            
            //printf("hiiii\n");
            int true_len = get_reclen(dir->name_len);
            int gap_len = dir->rec_len;
            int gap = (int) (gap_len - true_len);
            //Found a gap to place the inode in...
            if(gap > (int)get_reclen(strlen(dir_name))){
                found_flag = 1;

                dir->rec_len = true_len;
                //Refresh new dir..
                dir =  (struct ext2_dir_entry *)((char *)dir + dir->rec_len);
                dir->rec_len = gap;
                dir->inode = inode_num;
                dir->name_len = strlen(dir_name);
                dir->file_type = mode;
                for(int z = 0; z < dir->name_len; z++){
                    if(dir_name[z] != '\0'){//Don't add null terminator
                        dir->name[z] = dir_name[z];
                    }
                }
                //dir->name = dir_name;
                break;
            }else{
                dir =  (struct ext2_dir_entry *)((char *)dir + dir->rec_len);
                continue;
            }   
        }
    }
    //Can't find gap with current blocks..
    //Create a new block... and initialize the dir entry...
    if(found_flag == 0){
        dir_inode->i_blocks += 2;//Add a new block..
        dir_inode->i_size += EXT2_BLOCK_SIZE;
        block_count = (int) dir_inode->i_blocks / (EXT2_BLOCK_SIZE / 512);
        //New block 
        int new_block_id = find_FreeBlock(disk);
        toggleBlockBitmap(disk, new_block_id, 1);
        dir_inode->i_block[block_count - 1] = new_block_id;
        //id no need -1 ..
        struct ext2_dir_entry *dir = (struct ext2_dir_entry *)index(disk, new_block_id);
        //dir->rec_len = get_reclen(strlen(dir_name));
        dir->rec_len = EXT2_BLOCK_SIZE;
        dir->inode = inode_num;
        dir->name_len = strlen(dir_name);
        dir->file_type = mode;
        for(int z = 0; z < dir->name_len; z++){
            if(dir_name[z] != '\0'){//Don't add null terminator
                dir->name[z] = dir_name[z];
            }
        }
    }

    return 0;
}

//int main(int argc, char **argv) {
    //int bb = (int) time(NULL);
  // printf("file/dir:(%s)\n", getNewDirName(argv[1]));
   //printf("path is (%s) \n", ShortenPath(argv[1]));
  //printf("%sxxx\n", getParent("/level1/level2"));
   //printf("(%s)\n", getNewDirName(argv[1]));
    //Make sure there's 3 arguments
    /*if(argc != 2) {
        fprintf(stderr, "Usage: %s <image file name> <native file name> <disk directory name>\n", argv[0]);
        exit(1);
    }

    //Get the disk.
    int fd = open(argv[1], O_RDWR);

    disk = mmap(NULL, 128 * 1024, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if(disk == MAP_FAILED) {
        perror("mmap");
        exit(1);
    }
    printf("Blocks:\n");
    for(int i=0; i<100; i++){
        printf("%d", checkBlockBit(disk, i));
    }
    printf("Inodes:\n");
    for(int i=0; i<20; i++){
        printf("%d", checkInodeBit(disk, i));
    }*/

    /*int free_inode = find_FreeInode(disk);
    printf("Free inode is: %d\n", free_inode);
        free_inode = find_FreeInode(disk);
    printf("Free inode is: %d\n", free_inode);
    toggleInodeBitmap(disk, free_inode, 1);
    toggleInodeBitmap(disk, free_inode, 0);
         free_inode = find_FreeInode(disk);
    printf("Free inode is: %d\n", free_inode);*/

    /*    int free_inode = find_FreeBlock(disk);
    printf("Free block is: %d\n", free_inode);
       
        free_inode = find_FreeBlock(disk);
    printf("Free block is: %d\n", free_inode);
    
    toggleBlockBitmap(disk, free_inode, 1);
    free_inode = find_FreeBlock(disk);
    printf("Free block is: %d\n", free_inode);
    
    toggleBlockBitmap(disk, free_inode - 1, 0);
         free_inode = find_FreeBlock(disk);
    printf("Free block is: %d\n", free_inode);

*/


   /* int directory_inode = findFileInodeNum(disk, argv[2]);
    if(directory_inode == -1){
       printf("Can't find file..: %d\n", directory_inode);
    }else{
      printf("FOUND! Inode Num is %d\n", directory_inode);
    }*/

    //printf("test: %d\n", get_reclen(2));

//}