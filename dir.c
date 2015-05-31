#include "common.h"
#include "util.h"

extern struct ext2_inode read_inode(__u32);

inline int extract_entry_name(char*);
inline void parse_name(struct ext2_inode* , char*);

/*
 * read a inode entry's directory entry start from a offset
 * return the length of the directory entry.
 */
int read_dir_entry_in_block(char* dir_block, int offset, 
        struct ext2_dir_entry_2* dir_entry) {
    dir_entry->inode = parse_bytes_to_decimal_u(dir_block, offset, 4);    
    if (dir_entry->inode == 0) {
        return -1;
    }
    dir_entry->rec_len = parse_bytes_to_decimal_u(dir_block, offset + 4, 2);
    dir_entry->name_len = parse_bytes_to_decimal_u(dir_block, offset + 6, 1);
    dir_entry->file_type = parse_bytes_to_decimal_u(dir_block, offset + 7, 1);
    strncpy(dir_entry->name, dir_block + offset + 8, dir_entry->name_len);
    dir_entry->name[dir_entry->name_len] = '\0';
    return dir_entry->rec_len;
}

/*
 * Print all entries' name in the directory
 */
void print_entry_name_in_dir(struct ext2_inode* inode) {
    char dir_block[block_size];
    int i;
    for (i = 0; i < EXT2_N_BLOCKS; ++i) {
        __u32 block_id = inode->i_block[i];
        if (block_id == 0) {
            break;
        }
        read_block(block_id, block_size, dir_block);

        struct ext2_dir_entry_2 entry;
        int offset = 0;
        int len;
        while (offset < block_size) {
            len = read_dir_entry_in_block(dir_block, offset, &entry);
            if (len < 0) {
                break;
            }
            offset += len;
            printf("inode number: %d, %s\n", entry.inode, entry.name);
        }
    }
}

/*
 * Find the dir entry in the given directory pointed by inode.
 * It can be a file or a subdirectory in this directory. 
 * This function just does not recursively search for result.
 * Return 1 if found, -1 if not found.
 * Paremeters:
 *   inode -- inode pointing to the current dir entry, which 
 *            we will search the name in
 *   name  -- name of dir entry to search
 *   dir_entry -- search result
 */
int search_dir_entry(struct ext2_inode* inode, char* name, 
        struct ext2_dir_entry_2* dir_entry) {
    char dir_block[block_size];

    int i;
    for (i = 0; i < EXT2_N_BLOCKS; ++i) {
        __u32 block_id = inode->i_block[i];
        if (block_id == 0) {
            break;
        }
        read_block(block_id, block_size, dir_block);

        struct ext2_dir_entry_2 entry;
        int offset = 0;
        int len;
        int found = -1;
        while (offset < block_size) {
            len = read_dir_entry_in_block(dir_block, offset, &entry);
            if (len < 0) {
                break;
            }
            if (strcmp(entry.name, name) == 0) {
                found = 1;
                memcpy(dir_entry, &entry, sizeof(struct ext2_inode));
                break;
            }
            offset += len;
        }
        return found;
    }
}

/*
 * Find the dir entry in the given directory pointed by inode 
 * RECURSIVELY.
 * Return 1 if found, -1 if not found.
 * Paremeters:
 *   inode -- inode pointing to the current dir entry, which 
 *            we will search the name in
 *   name  -- complete path of dir entry to search
 *   dir_entry -- search result
 */
int search_dir_entry_r(struct ext2_inode* cur_inode, char* path, 
        struct ext2_dir_entry_2* dir_entry) {
    char name[EXT2_NAME_LEN];
    int start, end;
    struct ext2_dir_entry_2 entry;
    struct ext2_inode inode;

    start = (path[0] == '/')? 1: 0;
    // start searching from the root directory
    inode = *cur_inode;
    while (path[end] != '\0') {
        end = start + extract_entry_name(path + start);
        // invalid path input
        if (!INODE_IS_DIR(&inode) && path[end] != '\0') {
            return -1;
        }
        strncpy(name, path + start, end - start);
        name[end - start] = '\0';
        // cannot find the entry
        if (search_dir_entry(&inode, name, &entry) < 0) {
            return -1;
        }
        // new directory level's inode
        inode = read_inode(entry.inode);
        start = end + 1; // skip '/'
    }
    memcpy(dir_entry, &entry, sizeof(struct ext2_dir_entry_2));
    return 1;
}

/*
 * Given an inode number, return the entry's name.
 * In normal case, the first entry should be '.'. 
 * In this case, we can get the correct result.
 */
int get_dir_name(__u32 inode_num, char* name) {
    struct ext2_inode inode = read_inode(inode_num);
    struct ext2_dir_entry_2 entry;
    char dir_block[block_size];
    if (INODE_IS_LNK(&inode)) {
        parse_name(&inode, name);
    } else {
        // read the first direct datablock pointed by inode
        __u32 block_id = inode.i_block[0];
        read_block(block_id, block_size, dir_block);
        read_dir_entry_in_block(dir_block, 0, &entry); 
        strncpy(name, entry.name, entry.name_len);
        name[entry.name_len] = '\0';
    }
    return 0;
}

/*
 * return the length of the dir/file name
 */
inline int extract_entry_name(char* path) {
    int i = 0;
    while (path[i] != '/' && path[i] != '\0') {
        ++i;
    }
    return i;
}

/*
 * If the inode's type is a symbolic link, then the
 * data store in inode.i_block is the name of the file 
 * which this link references, if the file name can fit
 * into the 15*4 = 60 bytes length.
 */
inline void parse_name(struct ext2_inode* inode, char* name) {
    int i, j;
    int offset;
    char c;
    for (i = 0; i < EXT2_N_BLOCKS; ++i) {
        for (j = 0; j < 4; ++j) {
            offset = BITS_PER_BYTE * j; 
            c = (inode->i_block[i] >> offset) & 0xFF;
            name[i * 4 + j] = c;
            if (c == '\0') {
                return;
            }
        }
    }
}
