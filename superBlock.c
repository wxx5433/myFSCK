/*
 * Fuctions used to parse superblock.
 *
 * Author: Xiaoxiang Wu
 * Anderw ID: xiaoxiaw
 */

#include "common.h"
#include "util.h"


/*
 * Superblock is located at offset 1024 bytes into the partition and its size 
 * of 1024 bytes.
 * This function read the contents of superblock and parse its attributes.
 */
int read_super_block() {
    char contents[SUPER_BLOCK_SIZE];

    read_block(1, SUPER_BLOCK_SIZE, contents);  // size of super block is 1024 bytes
#ifdef DEBUG
    printf("********** start printing super block ************\n");
    print_block(contents);
    printf("********** stop printing super block ************\n");
#endif

    super_block.s_inodes_count = parse_bytes_to_decimal_u(contents, 0, 4);
    super_block.s_blocks_count = parse_bytes_to_decimal_u(contents, 4, 4);
    super_block.s_r_blocks_count = parse_bytes_to_decimal_u(contents, 8, 4);
    super_block.s_free_blocks_count = parse_bytes_to_decimal_u(contents, 12, 4);
    super_block.s_free_inodes_count = parse_bytes_to_decimal_u(contents, 16, 4);
    super_block.s_first_data_block = parse_bytes_to_decimal_u(contents, 20, 4);
    super_block.s_log_block_size = parse_bytes_to_decimal_u(contents, 24, 4);
    super_block.s_log_frag_size = parse_bytes_to_decimal_u(contents, 28, 4);
    super_block.s_blocks_per_group = parse_bytes_to_decimal_u(contents, 32, 4);
    super_block.s_frags_per_group = parse_bytes_to_decimal_u(contents, 36, 4);
    super_block.s_inodes_per_group = parse_bytes_to_decimal_u(contents, 40, 4);
    super_block.s_mtime = parse_bytes_to_decimal_u(contents, 44, 4);
    super_block.s_wtime = parse_bytes_to_decimal_u(contents, 48, 4);
    super_block.s_mnt_count = parse_bytes_to_decimal_u(contents, 52, 2);
    super_block.s_max_mnt_count = parse_bytes_to_decimal_s(contents, 54, 2);
    super_block.s_magic = parse_bytes_to_decimal_u(contents, 56, 2);
    super_block.s_state = parse_bytes_to_decimal_u(contents, 58, 2);
    super_block.s_errors = parse_bytes_to_decimal_u(contents, 60, 2);
    super_block.s_minor_rev_level = parse_bytes_to_decimal_u(contents, 62, 2);
    super_block.s_lastcheck = parse_bytes_to_decimal_u(contents, 64, 4);
    super_block.s_checkinterval = parse_bytes_to_decimal_u(contents, 68, 4);
    super_block.s_creator_os = parse_bytes_to_decimal_u(contents, 72, 4);
    super_block.s_rev_level = parse_bytes_to_decimal_u(contents, 76, 4);
    super_block.s_def_resuid = parse_bytes_to_decimal_u(contents, 80, 2);
    super_block.s_def_resgid = parse_bytes_to_decimal_u(contents, 82, 2);
    super_block.s_first_ino = parse_bytes_to_decimal_u(contents, 84, 4);
    super_block.s_inode_size = parse_bytes_to_decimal_u(contents, 88, 2);
    super_block.s_block_group_nr = parse_bytes_to_decimal_u(contents, 90, 2);
    super_block.s_feature_compat = parse_bytes_to_decimal_u(contents, 92, 4);
    super_block.s_feature_incompat = parse_bytes_to_decimal_u(contents, 96, 4);
    super_block.s_feature_ro_compat = parse_bytes_to_decimal_u(contents, 100, 4);

    // initialize global block size
    block_size = get_block_size();

    return 0;
}
