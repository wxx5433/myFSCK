#include "common.h"
#include "util.h"

#define bits_per_byte 8
/*#define DEBUG*/

extern struct ext2_group_desc read_group_desc(__u32 id);

inline __u32 get_block_group_num();
__u32 get_group_block_bitmap_block_num();

/*
 * Read the group's block bitmap
 */
void get_block_bitmap_in_group(char* block_bitmap, 
        __u32 group_id, __u32 block_num) {
    // get group descriptor
    struct ext2_group_desc group_desc = read_group_desc(group_id);
    __u32 start_block = group_desc.bg_block_bitmap;
    __u32 i;
    for (i = 0; i < block_num; ++i) {
        read_block(start_block + i, block_size, block_bitmap + i * block_size);
    }
}

/*
 * Load all block bitmap in the partition
 */
char* get_block_bitmap_in_partition() {
    __u32 group_num = get_block_group_num();
    __u32 block_num = get_group_block_bitmap_block_num();
    __u32 size = group_num * block_num * block_size;
    char* bitmap = (char*)malloc(size * sizeof(char));
    __u32 i;
    for (i = 0; i < group_num; ++i) {
        __u32 offset = i * block_num * block_size;
        get_block_bitmap_in_group(bitmap + offset, i, block_num);
    }
    return bitmap;
}

/*
 * Write the group's block bitmap into disk image
 */
void write_block_bitmap_in_group(char* block_bitmap, 
        __u32 group_id, __u32 block_num) {
    // get group descriptor
    struct ext2_group_desc group_desc = read_group_desc(group_id);
    __u32 start_block = group_desc.bg_block_bitmap;
    __u32 i;
    for (i = 0; i < block_num; ++i) {
        write_block(start_block + i, block_size, block_bitmap + i * block_size);
    }
}
/*
 * Write partition's blcok bitmap into disk image 
 */
void write_block_bitmap_in_partition(char* bitmap) {
    __u32 group_num = get_block_group_num();
    __u32 block_num = get_group_block_bitmap_block_num();

    int i;
    for (i = 0; i < group_num; ++i) {
        __u32 offset = i * block_num * block_size;
        write_block_bitmap_in_group(bitmap + offset, i, block_num);
    }
}

/*
 * Fix the block bitmap difference with the real block
 * allocation conditions obtained by traversing directory tree.
 *
 * Parameter:
 *     block_id: the block id that is not correct in bitmap
 */
void fix_block_bitmap_in_partition(char* bitmap, __u32 block_id) {
    __u32 index = (block_id - 1) / bits_per_byte;
    __u32 offset = (block_id - 1) % bits_per_byte;

    // flip the bit
    bitmap[index] = bitmap[index] ^ (1 << offset);
}

/*
 * Return the block's allocate bit in the block bitmap
 * NOTE: 
 * block 0 is NOT represented in the block bitmap
 * block 1 is store in the LEAST significant bit of byte 0
 * block 8 is store in the MOST significant bit of byte 0
 */
char get_block_alloc_bit(char* block_bitmap, __u32 block_id) {
    /*block_id = block_id % super_block.s_blocks_per_group;*/
    __u32 index = (block_id - 1) / bits_per_byte;
    __u32 offset = (block_id - 1) % bits_per_byte;
#ifdef DEBUG
    printf("block index: %d, offset: %u\n", index, offset);
#endif
    /*return (block_bitmap[index] >> (7 - offset)) & 1;*/
    return (block_bitmap[index] >> offset) & 1;
}

__u32 get_block_group_id(__u32 block_id) {
    /*return block_id  / super_block.s_blocks_per_group;*/
    return (block_id - 1) / super_block.s_blocks_per_group;
}

__u32 get_block_bitmap_block_num() {
    return get_group_block_bitmap_block_num() * get_block_group_num();
}

inline __u32 get_block_group_num() {
    __u32 blocks_per_group = super_block.s_blocks_per_group;
    return (super_block.s_blocks_count + 
            blocks_per_group - 1) / blocks_per_group;
}

__u32 get_group_block_bitmap_block_num() {
    __u32 bits_num = (super_block.s_blocks_per_group + 
            bits_per_byte - 1) / bits_per_byte;
    return (bits_num + block_size - 1) / block_size;
}
