/*
 * This program is to detect four types of errors in disk image
 * and correct them. 
 *    1. Directory Pointers
 *    2. Unreferenced inodes
 *    3. Inode link count
 *    4. Block allocation bitmap
 *
 *  Author: Xiaoxiang Wu
 *  AndrewID: xiaoxiaw
 */
#include "common.h"
#include "util.h"
#include "correct.h"


/*
 * Verify for each directory - that the first directory entry is '.' 
 * which self-references, and that the second directory entry is '..' 
 * which references its parent inode. 
 * If finds an error, print a short description of the error, 
 * and correct the entry.
 *
 * Recursively search the whole directory tree.
 * If it is a directory, continue searching its subdirectory.
 * If there is an error, print out an error message and correct
 * the entry.
 */
void pass1(__u32 parent_inode_num, __u32 inode_num) {
    struct ext2_inode inode = read_inode(inode_num);
    if (!INODE_IS_DIR(&inode)) {  // only checking directory
        return;
    }

    char dir_block[block_size];
    struct ext2_dir_entry_2 entry;

    int i;
    for (i = 0; i < EXT2_N_BLOCKS; ++i) {
        __u32 block_id = inode.i_block[i];
        if (block_id == 0) {
            break;
        }

        read_block(block_id, block_size, dir_block);

        int offset = 0;
        int len;
        int count = 0;
        while (offset < block_size) {
            len = read_dir_entry_in_block(dir_block, offset, &entry);
            if (len < 0) {
                break;
            }
            if (DIR_IS_DIR(&entry)) {  // continue searching if is directory
                if (count == 0) {
                    if (entry.inode != inode_num) {
                        print_dir_entry_error(inode_num, &entry);
                        pass1_corrector(dir_block, block_id, offset, inode_num);
                    } 
                } else if (count == 1) {
                    if (entry.inode != parent_inode_num) {
                        print_dir_entry_error(parent_inode_num, &entry);
                        pass1_corrector(dir_block, block_id, offset, 
                                parent_inode_num);
                    }
                } else {
                    pass1(inode_num, entry.inode);
                }
            }
            offset += len;
            ++count;
        }
    }
}

/*
 * Correct the dir entry in pass 1
 */
void pass1_corrector(char* dir_block, __u32 block_id, 
        __u32 offset_in_block, __u32 inode_num) {
    int i;
    int offset;
    char c;

    write_number_into_block(dir_block, offset_in_block, inode_num, 4);
    write_block(block_id, block_size, dir_block);
}

/*
 * Check that all allocated inodes are referenced in a directory 
 * entry somewhere. If you find an unreferenced inode, place it 
 * in the /lost+found directory - the name of this new entry in 
 * the directory should be the inode number.
 *  (i.e., if inode number 1074 is an allocated but unreferenced inode, 
 *  create a file or directory entry - /lost+found/1074.)
 */
void pass2() {
    /* create our own inode bitmap by scanning all entries and
     * compare it with the alloc inode bitmap.
     */

    char* inode_links_count = 
        (char*)calloc((super_block.s_inodes_count + 1), sizeof(char));
    int i;
    directory_traversor(inode_links_count, ROOT_INODE_NUM);
    // get the partition's inode 
    char* bitmap = get_inode_bitmap_in_partition();

    struct ext2_inode inode;
    char alloc_bit;
    for (i = 1; i <= super_block.s_inodes_count; ++i) {
        inode = read_inode(i);
        if (inode.i_links_count == 0) {  
            continue;
        }
        alloc_bit = get_inode_alloc_bit(bitmap, i);
        if (alloc_bit == 1 && inode_links_count[i] == 0) {
            printf("Unreferenced inodes: %u\n", i);
            if (INODE_IS_DIR(&inode)) {
                /* All contents in an unreferenced directory are 
                 * unreference as well. I only put the topmost 
                 * unreferenced directory into lost+found.*/
                directory_traversor(inode_links_count, i);
            }
            add_to_lost_found(inode_links_count, i);
        }
    }

    free(inode_links_count);
    free(bitmap);
}

void directory_traversor(char* inode_links_count, __u32 inode_num) {
    struct ext2_inode inode = read_inode(inode_num);
    char dir_block[block_size];

    int i;
    for (i = 0; i < EXT2_N_BLOCKS; ++i) {
        __u32 block_id = inode.i_block[i];
        if (block_id == 0) {
            break;
        }
        read_block(block_id, block_size, dir_block);

        int offset = 0;
        int len;
        int count = 0;
        struct ext2_dir_entry_2 entry;
        while (offset < block_size) {
            len = read_dir_entry_in_block(dir_block, offset, &entry);
            if (len < 0) {
                break;
            } 
            inode_links_count[entry.inode]++;
            if (i != 0 || count >= 2) {
                if (DIR_IS_DIR(&entry)) {
                    directory_traversor(inode_links_count, entry.inode);
                }
            }
            offset += len;
            ++count;
        }
    }
}

/*
 * Add the unreferenced inodes into lost+found directory
 * Return -1 if cannot find the directory
 */
int add_to_lost_found(char* inode_links_count, __u32 inode_num) {
    struct ext2_dir_entry_2 lost_found_dir;
    struct ext2_inode root_inode = read_inode(ROOT_INODE_NUM);
    if (search_dir_entry(&root_inode, 
                lost_found_dir_name, &lost_found_dir) < 0) {
        printf("cannot find lost+found dir\n");
        return -1;
    }
    struct ext2_inode lost_found_inode = read_inode(lost_found_dir.inode);

    // create a dir entry to put into lost+found
    struct ext2_dir_entry_2 new_entry = create_new_entry(inode_num);

    __u32 block_id;
    char dir_block[block_size];
    struct ext2_dir_entry_2 entry;
    int offset, len;
    int i;
    for (i = 0; i < EXT2_N_BLOCKS; ++i) {
        block_id = lost_found_inode.i_block[i];
        if (block_id == 0) {
            break;
        }
        read_block(block_id, block_size, dir_block);

        offset = 0;
        // find the last entry
        while (offset < block_size) {
            len = read_dir_entry_in_block(dir_block, offset, &entry);
            if (len < 0) {
                break;
            } 
            offset += len;
        }
        // get the last entry's real length
        offset -= len;
        __u32 real_len = pad_to_4_bytes(DIR_ENTRY_PREFIX_LEN + entry.name_len);
        // enough space to put the new entry
        if (offset + real_len + new_entry.rec_len < block_size) {
            // rewrite the last entry's rec_len
            write_number_into_block(dir_block, offset + 4, real_len, 2);
            write_new_entry(&new_entry, dir_block, block_id, offset + real_len);
            break;
        }
    }
    // set inode's parent dir to lost+found
    struct ext2_inode inode = read_inode(inode_num);
    read_block(inode.i_block[0], block_size, dir_block);
    write_number_into_block(dir_block, FIRST_ENTRY_LEN, 
            lost_found_dir.inode, 4);
    write_block(inode.i_block[0], block_size, dir_block);
}

/*
 * Create a new entry to put into lost+found directory
 */
struct ext2_dir_entry_2 create_new_entry(__u32 inode_num) {
    struct ext2_dir_entry_2 new_entry;
    new_entry.inode = inode_num;
    
    struct ext2_inode inode = read_inode(inode_num);
    if (INODE_IS_DIR(&inode)) {
        new_entry.file_type = DIR_TYPE;
    } else {
        new_entry.file_type = REG_TYPE;
    }
    int len = 0;
    char name[EXT2_NAME_LEN];
    // set emtry name
    while (inode_num != 0) {
        name[len++] = inode_num % 10 + '0';
        inode_num /= 10;
    }
    int i;
    for (i = 0; i < len; ++i) {
        new_entry.name[i] = name[len - 1 - i];
    }
    new_entry.name[len] = '\0';
    new_entry.name_len = len;
    // real dir etnry length, but need to change later
    new_entry.rec_len = pad_to_4_bytes(DIR_ENTRY_PREFIX_LEN + len);
    return new_entry;
}

/*
 * Write the new entry into disk image
 */
void write_new_entry(struct ext2_dir_entry_2* entry, char* dir_block, 
        __u32 block_id, __u32 offset_in_block) {
    // write inode number
    write_number_into_block(dir_block, offset_in_block, entry->inode, 4);
    // write rec_len, notice that it should be changed so that
    entry->rec_len = block_size - offset_in_block;
    write_number_into_block(dir_block, offset_in_block + 4, entry->rec_len, 2);
    dir_block[offset_in_block + 6] = entry->name_len;
    dir_block[offset_in_block + 7] = entry->file_type;
    strncpy(dir_block + offset_in_block + 8, entry->name, entry->name_len + 1);
    // write it into disk
    write_block(block_id, block_size, dir_block);
}

/*
 * Count the number of directory entries that point to each inode
 * (e.g., the number of hard links) and compare that to the inode link counter. 
 * If your tool finds a discrepancy, it should print a short description of the error, 
 * and update the inode link counter.
 */
void pass3() {
    char* inode_links_count 
        = (char*)calloc((super_block.s_inodes_count + 1), sizeof(char));
    directory_traversor(inode_links_count, ROOT_INODE_NUM);
    __u32 i;
    for (i = 1; i <= super_block.s_inodes_count; ++i) {
        struct ext2_inode inode = read_inode(i);
        if (inode.i_links_count != inode_links_count[i]) {
            printf("Inode %u ref count is %u, should be %u\n", 
                    i, inode.i_links_count, inode_links_count[i]);
            write_inode(i, inode_links_count[i]);
        }
    }
    free(inode_links_count);
}

/*
 * Walk the directory tree and verify that the block bitmap is correct. 
 * If your tool finds a block that should (or should not) be marked in the bitmap, 
 * it should print a short description of the error, and correct the bitmap.
 */
void pass4() {
    // encoding from the first data block

    char* bitmap = get_block_bitmap_in_partition();
    // bitmap obtained by walking through eht directory tree
    char* true_bitmap = 
        (char*)calloc((super_block.s_blocks_count + 1), sizeof(char));
    get_true_block_bitmap(true_bitmap, ROOT_INODE_NUM);

    __u32 i, j;
    __u32 blocks_per_group = super_block.s_blocks_per_group;
    __u32 group_num 
        = (super_block.s_blocks_count + blocks_per_group - 1) / blocks_per_group;
    __u32 skip_block_num = get_skip_block_num_in_group();

    __u32 block_id = 1;
    char alloc_bit;
    char found_error = 0;
    for (i = 0; i < group_num; ++i) {
        block_id += skip_block_num;
        for (j = 0; j + skip_block_num < blocks_per_group; ++j) {
            alloc_bit = get_block_alloc_bit(bitmap, block_id);
            if (true_bitmap[block_id] != alloc_bit) {
                found_error = 1;
                printf("Block bitmap difference: %u\n", block_id);
                fix_block_bitmap_in_partition(bitmap, block_id);
            }
            ++block_id;
            if (block_id >= super_block.s_blocks_count) {
                break;
            }
        }
    }

    // check inode table block allocation
    for (i = 0; i < group_num; ++i) {
        struct ext2_group_desc group_desc = read_group_desc(i);
        __u32 start_block_id = group_desc.bg_inode_table;
        __u32 inode_block_per_group = get_group_inode_block_num();
        for (j = 0; j < inode_block_per_group; ++j) {
            block_id = start_block_id + j;
            alloc_bit = get_block_alloc_bit(bitmap, block_id);
            if (alloc_bit != 1) {
                found_error = 1;
                printf("Block bitmap difference: %u\n", block_id);
                fix_block_bitmap_in_partition(bitmap, block_id);
            }
        }
    }

    if (found_error == 1) {
        write_block_bitmap_in_partition(bitmap);
    }
    
    free(bitmap);
    free(true_bitmap);
}

/*
 * bitmap obtained by walking through eht directory tree
 */
void get_true_block_bitmap(char* block_bitmap, __u32 inode_num) {
    char dir_block[block_size];
    struct ext2_dir_entry_2 entry;
    
    struct ext2_inode inode = read_inode(inode_num);

    // TODO assume symbolic name fits into 60 bytes
    if (INODE_IS_LNK(&inode)) {  
        return;
    } else if (INODE_IS_REG(&inode)) {
        get_file_block_bitmap(block_bitmap, &inode);
        return;
    }
    
    int i;
    for (i = 0; i < EXT2_N_BLOCKS; ++i) {
        __u32 block_id = inode.i_block[i];
        if (block_id == 0) {
            break;
        }

        block_bitmap[block_id] = 1;
        read_block(block_id, block_size, dir_block);

        int offset = 0;
        int len;
        int count = 0;
        while (offset < block_size) {
            len = read_dir_entry_in_block(dir_block, offset, &entry);
            if (len < 0) {
                break;
            } 
            if (i != 0 || count >= 2) {
                get_true_block_bitmap(block_bitmap, entry.inode);
            }
            offset += len;
            ++count;
        }
    }
}

/*
 * Get the given file's block allocation condition.
 * I assume that there is only indirect and double-indirect block
 */
void get_file_block_bitmap(char* block_bitmap, struct ext2_inode* inode) {
    int file_block_num = (inode->i_size + block_size - 1) / block_size;
    int i;
    if (file_block_num <= EXT2_NDIR_BLOCKS) {
        for (i = 0; i < file_block_num; ++i) {
            block_bitmap[inode->i_block[i]] = 1;
        }
    } else if (file_block_num <= INDIRECT_MAX_BLOCK) {
        get_file_indirect_block_bitmap(block_bitmap, inode, file_block_num);
    } else if (file_block_num <= DOUBLE_INDIRECT_MAX_BLOCK){ 
        get_file_double_indirect_block_bitmap(
                block_bitmap, inode, file_block_num);
    } else { // TODO assume only doubly linked
        printf("ooooooooops!!\n");
    }
}

/*
 * If the file's size is more than DIRECT_MAX_BLOCK and not 
 * more than INDIRECT_MAX_BLOCK, we can store it using the 
 * 13rd i_block pointer, which is indirect block pointer
 */
void get_file_indirect_block_bitmap(char* block_bitmap, 
        struct ext2_inode* inode, __u32 file_block_num) {
    int i;
    for (i = 0; i < EXT2_NDIR_BLOCKS; ++i) {
        block_bitmap[inode->i_block[i]] = 1;
    }
    char pointer_block[block_size];
    int k = EXT2_IND_BLOCK;
    block_bitmap[inode->i_block[k]] = 1;
    read_block(inode->i_block[k], block_size, pointer_block);
    for (i = 0; i < block_size; i += 4) {
        block_bitmap[parse_bytes_to_decimal_u(pointer_block, i, 4)] = 1;
        ++k;
        if (k == file_block_num) {
            return;
        }
    }
}

/*
 * If the file's size is more than INDIRECT_MAX_BLOCK, 
 * It is stored using the 14th i_block pointer, 
 * which is double indirect block pointer
 */
void get_file_double_indirect_block_bitmap(char* block_bitmap, 
        struct ext2_inode* inode, __u32 file_block_num) {
    // read first 12 direct blocks
    int i, j;
    for (i = 0; i < EXT2_NDIR_BLOCKS; ++i) {
        block_bitmap[inode->i_block[i]] = 1;
    }
    char pointer_block_lev1[block_size];
    char pointer_block_lev2[block_size];
    // read the 13rd indirect block
    block_bitmap[inode->i_block[EXT2_IND_BLOCK]] = 1;
    read_block(inode->i_block[EXT2_IND_BLOCK], block_size, pointer_block_lev1);
    int k = EXT2_NDIR_BLOCKS;
    for (i = 0; i < block_size; i += 4) {
        block_bitmap[parse_bytes_to_decimal_u(pointer_block_lev1, i, 4)] = 1;
        ++k;
    }

    // read the 14th double indirect block
    block_bitmap[inode->i_block[EXT2_DIND_BLOCK]] = 1;
    read_block(inode->i_block[EXT2_DIND_BLOCK], block_size, pointer_block_lev1);
    for (i = 0; i < block_size; i += 4) {
        __u32 indirect_block_id = parse_bytes_to_decimal_u(pointer_block_lev1, i, 4);
        block_bitmap[indirect_block_id] = 1;
        read_block(indirect_block_id, block_size, pointer_block_lev2);
        for (j = 0; j < block_size; j += 4) {
            block_bitmap[parse_bytes_to_decimal_u(pointer_block_lev2, j, 4)] = 1;
            ++k;
            if (k == file_block_num) {
                return;
            }
        }
    }
}

/*
 * Check the disk image and correct errors on the given partition
 */
int correct_one_partition(int partition_num) {
    read_partition_info(partition_num);
    if (IS_NULL_ENTRY(&partition_entry)) {
        return -1;
    } else if(!IS_EXT2_FS(&partition_entry)) {
        return 0;
    }

    // read super block
    read_super_block();

    // read the group descriptor block
    group_desc_block = (char*)malloc(block_size * sizeof(char));
    read_group_desc_block(group_desc_block);

    // read the root directory's inode
    /*struct ext2_inode root_inode = read_inode(ROOT_INODE_NUM); */

    char* inode_links_count = 
        (char*)calloc((super_block.s_inodes_count + 1), sizeof(char));
    pass1(ROOT_INODE_NUM, ROOT_INODE_NUM);
    pass2();
    pass3();
    pass4();
    /*struct ext2_inode inode = read_inode(2010);*/
    /*printf("size: %d\n", inode.i_size);*/

    free(inode_links_count);
    free(group_desc_block);
}

/*
 * Check the disk image and correct errors.
 * If the partition_num is 0, then check all ext2 partitions
 */
int correct_partition(int partition_num) {
    if (partition_num == 0) {
        int i = 1;
        while (1) {
            if (correct_one_partition(i) == -1) {
                break;
            }
            ++i;
        }
    } else {
        correct_one_partition(partition_num);
    }
}

inline void print_dir_entry_error(__u32 inode_num, 
        struct ext2_dir_entry_2* entry) {
    printf("dir entry incorrect, parent inode_num: %u, subdir_name: %s,\
            inode: %u\n", inode_num, entry->name, entry->inode);
}

/*
 * When checking the block bitmap, skip the first few blocks
 * of each group that contains superblock, group descriptor,
 * block bitmap, inode bitmap and inode table, they should 
 * always be 1.
 */
inline __u32 get_skip_block_num_in_group() {
    // superblock and group descriptor are both 1 block
    __u32 skip_block_num = 2;
    // group block bitmap
    skip_block_num += get_group_block_bitmap_block_num();
    // group inode bitmap
    skip_block_num += get_group_inode_bitmap_block_num();
    // group inode table
    skip_block_num += get_group_inode_block_num();
    return skip_block_num;
}
