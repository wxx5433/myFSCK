#include "common.h"
#include "util.h"


PartitionEntry read_ebr(PartitionEntry* base_partition_entry, int partition_num);
int read_partition_info(int partition_num);
void read_partition_entry(unsigned char* section, int start, int offset, PartitionEntry* partition_entry);
