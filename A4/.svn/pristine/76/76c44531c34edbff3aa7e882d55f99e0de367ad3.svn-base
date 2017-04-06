#include "ext2.h"

#define index(disk, i) (disk+ (i*1024))
#define inode(inodeTable, id) (inodeTable + (id -1) * sizeof(struct ext2_inode))

int fixInode(unsigned char *disk, int inode_id);
int fixBlock(unsigned char *disk, int block_id);
void printBit(unsigned char *byte);
char* concat(char* string1, char* string2);
int findFileInodeNum(unsigned char *disk, char * path);
char * substring(char *string, int length);
int equal_string(char *string1, char *string2, int length);
int findDirectoryInodeNum(unsigned char *disk, char * path);
int find_FreeInode(unsigned char *disk);
int get_reclen(unsigned char name_len);
int find_FreeBlock(unsigned char *disk);
int toggleInodeBitmap(unsigned char *disk, int id, int value);
int toggleBlockBitmap(unsigned char *disk, int id, int value);
char* getParent(char * path);
char* ShortenPath(char * path);
int put_inode_in_DirInode(unsigned char *disk, char *path_to_save, int inode_num, char* dir_name, unsigned char mode);
char* getNewDirName(char * path);
int checkBlockBit(unsigned char *disk, int block_id);
int checkInodeBit(unsigned char *disk, int inode_id);