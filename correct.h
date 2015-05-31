// ext2_dir_entry_2's entry length excluding name length
#define DIR_ENTRY_PREFIX_LEN 8

// the first entry is '.', its length is 12
#define FIRST_ENTRY_LEN 12

#define INDIRECT_MAX_BLOCK 268
#define DOUBLE_INDIRECT_MAX_BLOCK 65549

//#define CORRECT_DEBUG

char* lost_found_dir_name = "lost+found";
//int first_block_id;

extern int read_partition_info(int);
extern int read_super_block();
extern void read_group_desc_block(char*);
extern struct ext2_group_desc read_group_desc(__u32);
extern struct ext2_inode read_inode(__u32);
extern int read_dir_entry_in_block(char*, int, struct ext2_dir_entry_2*);
extern char* get_inode_bitmap_in_partition();
extern char get_inode_alloc_bit(char*, __u32);
extern int search_dir_entry(struct ext2_inode*, char*, struct ext2_dir_entry_2*);
extern void write_inode(__u32, __u32);
extern char* get_block_bitmap_in_partition();
extern char get_block_alloc_bit(char*, __u32);
extern __u32 get_inode_bitmap_block_num();
extern __u32 get_block_bitmap_block_num();
extern __u32 get_inode_table_block_num();
extern __u32 get_group_block_bitmap_block_num();
extern __u32 get_group_inode_block_num();
extern __u32 get_group_inode_bitmap_block_num();
extern void fix_block_bitmap_in_partition(char*, __u32);
extern void write_block_bitmap_in_partition(char*);

inline __u32 get_skip_block_num_in_group();
inline void print_dir_entry_error(__u32, struct ext2_dir_entry_2*);
void pass1_corrector(char*, __u32, __u32, __u32);
int add_to_lost_found(char*, __u32);
void directory_traversor(char*, __u32);
struct ext2_dir_entry_2 create_new_entry(__u32);
void write_new_entry(struct ext2_dir_entry_2*, char*, __u32, __u32);
void get_true_block_bitmap(char*, __u32);
void get_file_block_bitmap(char*, struct ext2_inode*);
void get_file_indirect_block_bitmap(char*, struct ext2_inode*, __u32);
void get_file_double_indirect_block_bitmap(char*, struct ext2_inode*, __u32);
