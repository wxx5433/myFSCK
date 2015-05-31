/*
 * Fuctions to read group descriptors
 *
 * Author: Xiaoxiang Wu
 * AndrewID: xiaoxiaw
 */

#include "common.h"
#include "util.h"

/*
 * second block of the partiion is group descriptors block
 */
void read_group_desc_block(char* contents) {
    read_block(2, block_size, contents);
#ifdef DEBUG
    printf("************ start printing group desc block ***************\n");
    print_block(contents);
    printf("************ stop printing group desc block ***************\n");
#endif
}

struct ext2_group_desc read_group_desc(__u32 id) {
    struct ext2_group_desc group_desc;
    __u32 offset = id * GROUP_DESC_SIZE;
    group_desc.bg_block_bitmap = 
        parse_bytes_to_decimal_u(group_desc_block, offset, 4);
    group_desc.bg_inode_bitmap = 
        parse_bytes_to_decimal_u(group_desc_block, offset + 4, 4);
    group_desc.bg_inode_table = 
        parse_bytes_to_decimal_u(group_desc_block, offset + 8, 4);
    group_desc.bg_free_blocks_count = 
        parse_bytes_to_decimal_u(group_desc_block, offset + 12, 2);
    group_desc.bg_free_inodes_count = 
        parse_bytes_to_decimal_u(group_desc_block, offset + 14, 2);
    group_desc.bg_used_dirs_count = 
        parse_bytes_to_decimal_u(group_desc_block, offset + 16, 2);
    return group_desc;
}
