unsigned int parse_bytes_to_decimal_u(unsigned char* entry_info, int start, int len);
int parse_bytes_to_decimal_s(unsigned char* entry_info, int start, int len);
void read_block(__u32 offset, __u32 block_size, void *into);
void write_block(__u32 block_offset, __u32 block_size, char* from);
void print_block(char* contents);
__u32 get_block_size();
__u32 pad_to_4_bytes(__u32 len);
void write_number_into_block(char* dir_block, __u32 offset_in_block, __u32 num, __u32 byte_num);
