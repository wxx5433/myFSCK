#include "common.h"
#include "util.h"

//#define DEBUG

#define bits_per_byte 8

extern struct ext2_group_desc read_group_desc(__u32 id);
int read_group_inode_table(__u32 block_offset, char* group_inode_table);
__u32 get_inode_bitmap_block_num();
__u32 get_group_inode_block_num();
__u32 get_group_inode_bitmap_block_num();
inline __u32 get_inode_group_num();
inline __u32 get_inode_group_offset(__u32);
inline __u32 get_inode_offset_in_group(__u32);
inline __u32 get_file_type_from_inode(struct ext2_inode*);

/*
 * note that inode id start from 1, not 0
 */
struct ext2_inode read_inode(__u32 inode_num) {
    // read corresponding group descriptor
    __u32 group_id = get_inode_group_offset(inode_num);
    struct ext2_group_desc group_desc = read_group_desc(group_id);
    // inode table size in one group, round to multiple of block size
    __u32 inode_table_size = get_group_inode_block_num() * block_size;
    char inode_table[inode_table_size];
    // read the inode table
    int64_t start_sector = partition_entry.start;
    int block_num = read_group_inode_table(group_desc.bg_inode_table, inode_table);

    struct ext2_inode inode;
    __u32 offset = get_inode_offset_in_group(inode_num) * INODE_SIZE;

    inode.i_mode = parse_bytes_to_decimal_u(inode_table, offset, 2);
    inode.i_uid = parse_bytes_to_decimal_u(inode_table, offset + 2, 2);
    inode.i_size = parse_bytes_to_decimal_u(inode_table, offset + 4, 4);
    inode.i_atime = parse_bytes_to_decimal_u(inode_table, offset + 8, 4);
    inode.i_ctime = parse_bytes_to_decimal_u(inode_table, offset + 12, 4);
    inode.i_mtime = parse_bytes_to_decimal_u(inode_table, offset + 16, 4);
    inode.i_dtime = parse_bytes_to_decimal_u(inode_table, offset + 20, 4);
    inode.i_gid = parse_bytes_to_decimal_u(inode_table, offset + 24, 2);
    inode.i_links_count = parse_bytes_to_decimal_u(inode_table, offset + 26, 2);
    inode.i_blocks = parse_bytes_to_decimal_u(inode_table, offset + 28, 4);
    inode.i_flags = parse_bytes_to_decimal_u(inode_table, offset + 32, 4);
    // ignore operting system info here, which is offset+36
    int i;
    for (i = 0; i < EXT2_N_BLOCKS; ++i) {
        inode.i_block[i] = parse_bytes_to_decimal_u(
                inode_table, offset + 40 + i * 4, 4);
    }

    return inode;
}

void write_inode(__u32 inode_num, __u32 links_count) {
    // read corresponding group descriptor
    __u32 group_id = get_inode_group_offset(inode_num);
    struct ext2_group_desc group_desc = read_group_desc(group_id);
    // inode table size in one group, round to multiple of block size
    __u32 inode_table_size = get_group_inode_block_num() * block_size;
    char inode_table[inode_table_size];
    // read the inode table
    int64_t start_sector = partition_entry.start;
    int block_num = read_group_inode_table(
            group_desc.bg_inode_table, inode_table);

    struct ext2_inode inode;
    __u32 offset = get_inode_offset_in_group(inode_num) * INODE_SIZE;
    __u32 index = offset / block_size;
    __u32 offset_in_block = offset % block_size + 26;

    // write it into inode table
    write_number_into_block(inode_table + index * block_size, 
            offset_in_block, links_count, 2);
    inode.i_links_count = parse_bytes_to_decimal_u(
            inode_table, offset + 26, 2);
    write_block(group_desc.bg_inode_table + index, 
            block_size, inode_table + index * block_size);

}

/*
 * Read the inode table blocks in the given group
 * Return how many inodes block in a group
 */
int read_group_inode_table(__u32 block_offset, char* group_inode_table) {
    int64_t partition_start = partition_entry.start;
    __u32 inode_block_per_group = get_group_inode_block_num();
    __u32 i;
    for (i = 0; i < inode_block_per_group; ++i) {
        read_block(block_offset + i, 
                block_size, group_inode_table + block_size * i);
    }
    return inode_block_per_group;
}

/*
 * Read the group's inode bitmap
 */
void get_inode_bitmap_in_group(char* inode_bitmap, 
        __u32 group_id, __u32 block_num) {
    // get group descriptor
    struct ext2_group_desc group_desc = read_group_desc(group_id);
    // the block number of the first inode bitmap block
    __u32 start_block = group_desc.bg_inode_bitmap;
    __u32 i;
    for (i = 0; i < block_num; ++i) {
        read_block(start_block + i, 
                block_size, inode_bitmap + i * block_size);
    }
}

/*
 * Load all inode_bitmap in the partition
 */
char* get_inode_bitmap_in_partition() {
    __u32 group_num = get_inode_group_num();
    __u32 block_num = get_group_inode_bitmap_block_num();
    __u32 size = group_num * block_num * block_size;
    char* bitmap = (char*)malloc(size * sizeof(char));
    __u32 i;
    for (i = 0; i < group_num; ++i) {
        get_inode_bitmap_in_group(bitmap + 
                i * block_num * block_size, i, block_num);
    }
    return bitmap;
}

/*
 * Return the inode's allocate bit in the inode bitmap
 * NOTE: 
 * inode 1 is store in the LEAST significant bit of byte 0. 
 * inode 8 is sotre in the MOST significant bit of byte 0. 
 */
char get_inode_alloc_bit(char* inode_bitmap, __u32 inode_num) {
    /*inode_num = (inode_num - 1) % super_block.s_inodes_per_group;*/
    // one byte can represent 8 inodes
    __u32 index = (inode_num - 1) / bits_per_byte;
    // e.g. inode 1 is store in the first bit of a byte
    __u32 offset = (inode_num - 1) % bits_per_byte;
#ifdef DEBUG
    printf("inode index: %u, offset: %u\n", index, offset);
#endif
    /*return (inode_bitmap[index] >> (7 - offset)) & 1;*/
    return (inode_bitmap[index] >> offset) & 1;
}

/*
 * Return how many blocks the inode bitmap occupies 
 * in the partition
 */
__u32 get_inode_bitmap_block_num() {
    __u32 group_num = get_inode_group_num();
    __u32 block_num = get_group_inode_bitmap_block_num();
    return group_num * block_num;
}

__u32 get_inode_table_block_num() {
    __u32 group_num = get_inode_group_num();
    __u32 block_num = get_group_inode_block_num();
    return group_num * block_num;
}

/*
 * Return how many blocks a bitmap in a group occupies
 * each inode only need one bit to represent, so it is #inode/8
 * The number should be the same for all block group 
 */
__u32 get_group_inode_bitmap_block_num() {
    __u32 bits_num = (super_block.s_inodes_per_group + 
            bits_per_byte - 1) / bits_per_byte;
    return (bits_num + block_size - 1) / block_size;
}


inline __u32 get_inode_group_num() {
    __u32 inodes_per_group = super_block.s_inodes_per_group;
    return (super_block.s_inodes_count + 
            inodes_per_group - 1) / inodes_per_group;
}

/*
 * Get how many blocks are used to cantain inodes per group
 * This should be the same for all groups in the same partition.
 */
__u32 get_group_inode_block_num() {
    return (super_block.s_inodes_per_group * INODE_SIZE + 
            block_size - 1) / block_size;
}

/*
 * Given an inode number, return its group number
 * inode id start from 1, not 0
 */
inline __u32 get_inode_group_offset(__u32 inode_num) {
    return (inode_num - 1) / super_block.s_inodes_per_group;
}

inline __u32 get_inode_offset_in_group(__u32 inode_num) {
    return (inode_num - 1) % super_block.s_inodes_per_group;
}

/*
 * The leftmost four bits of inode.i_mode are file types. 
 * The remaining are permissinos.
 */
inline __u32 get_file_type_from_inode(struct ext2_inode* inode) { 
    return inode->i_mode & 0xF000;
}

