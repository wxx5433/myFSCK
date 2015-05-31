/*
 * Some util functions called by other functions
 *
 * Author: Xiaoxiang Wu
 * Andrew ID: xiaoxiaw
 */

/*
 * convert at most 4 bytes to unsigned decimal. (big endian)
 */
#include "common.h"

unsigned int parse_bytes_to_decimal_u(unsigned char* entry_info, 
        int start, int len) {
    unsigned int result = 0;
    int i;
    for (i = start + len - 1; i >= start; --i) {
        result = (result << 8) + entry_info[i];
    }
    return result;
}

/*
 * convert at most 4 bytes to signed decimal. (big endian)
 */
int parse_bytes_to_decimal_s(unsigned char* entry_info, 
        int start, int len) {
    int result = 0;
    int i;
    for (i = start + len - 1; i >= start; --i) {
        result = (result << 8) + entry_info[i];
    }
    return result;
}

/*
 * Read the nth block from the partition
 */
void read_block(__u32 block_offset, __u32 block_size, void *into) {
    int64_t start_sector = partition_entry.start;
    __u32 sector_per_block = block_size / sector_size_bytes;
    __u32 sector_offset = block_offset * sector_per_block;
    read_sectors(start_sector + sector_offset, sector_per_block, into);
}

void write_block(__u32 block_offset, __u32 block_size, char* from) {
    int64_t start_sector = partition_entry.start;
    __u32 sector_per_block = block_size / sector_size_bytes;
    __u32 sector_offset = block_offset * sector_per_block;
    write_sectors(start_sector + sector_offset, sector_per_block, from);
}

void print_block(char* contents) {
    __u32 sector_per_block = block_size / sector_size_bytes;

    __u32 i;
    for (i = 0; i < sector_per_block; ++i) {
        print_sector(contents + sector_size_bytes * i);
    }
}

__u32 get_block_size() {
    __u32 exp = super_block.s_log_block_size;
    return 1024 * (exp == 0? 1: 2 << (exp - 1));
}

__u32 pad_to_4_bytes(__u32 len) {
    return (len % 4 == 0)? len: 4 * (len / 4 + 1);
}

/*
 * Write a number into block, the least significant byte should
 * appear first
 */
void write_number_into_block(char* dir_block, __u32 offset_in_block, 
        __u32 num, __u32 byte_num) {
    int offset;
    int i;
    for (i = 0; i < byte_num; ++i) {
        offset = BITS_PER_BYTE * i;
        dir_block[offset_in_block + i] = (num >> offset) & 0xFF;
    }
}
