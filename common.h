#include <stdlib.h>
#include <stdio.h>
#include <getopt.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>

#include "genhd.h"
#include "ext2_fs.h"

//#define DEBUG

#define INVALID_TYPE 0xFF
#define IS_NULL_ENTRY(entry) (entry)->type==INVALID_TYPE
#define FIRST_PARTITION_OFFSET  446
#define PARTITION_ENTRY_SIZE  16
#define sector_size_bytes 512
#define SUPER_BLOCK_SIZE 1024
#define INODE_SIZE 128
#define GROUP_DESC_SIZE 32
#define ROOT_INODE_NUM 2
#define BITS_PER_BYTE 8

#define EXT2_FS 0x83
#define IS_EXT2_FS(entry) ((entry)->type == EXT2_FS)

/* 
 * Judging file type from inode
 * The leftmost 4 bits of inode.i_mode is file type values
 */
#define INODE_IS_DIR(inode) ((inode)->i_mode & 0xF000)==EXT2_S_IFDIR
#define INODE_IS_REG(inode) ((inode)->i_mode & 0xF000)==EXT2_S_IFREG
#define INODE_IS_LNK(inode) ((inode)->i_mode & 0xF000)==EXT2_S_IFLNK

/*
 * Juding file type from directory entry, only can judge 
 * directory and regular file.
 */
#define REG_TYPE 1
#define DIR_TYPE 2
#define DIR_IS_REG(dir_entry) (dir_entry)->file_type==REG_TYPE
#define DIR_IS_DIR(dir_entry) (dir_entry)->file_type==DIR_TYPE

typedef struct PartitionEntry {
    unsigned char type;
    unsigned int start;
    unsigned int length;
} PartitionEntry;

int device;  /* disk image file descriptor */
struct ext2_super_block super_block;
__u32 block_size;
PartitionEntry partition_entry;
char* group_desc_block;


extern void print_sector (unsigned char *buf);
extern void read_sectors (int64_t start_sector, unsigned int num_sectors, void *into);
extern void write_sectors (int64_t start_sector, unsigned int num_sectors, void *from);

