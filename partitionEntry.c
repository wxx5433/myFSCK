/*
 * Read information about partition entry from MBR and EBR.
 *
 * Author: Xiaoxiang Wu
 * Andrew ID: xiaoxiaw
 */
#include "partitionEntry.h"

/*
 * read the MBR from section 0 in the disk image
 */
int read_mbr(char* MBR, PartitionEntry* parition_entry, 
        int partition_num) {
    int offset = FIRST_PARTITION_OFFSET + 
        PARTITION_ENTRY_SIZE * (partition_num - 1);
    int i;
    read_partition_entry(MBR, 0, offset, parition_entry);
    return 0;
}

/*
 * Read a partition from EBR
 * If the partition does not exist, set the return partition
 * entry's type to INVALID_TYPE
 */
PartitionEntry read_ebr(PartitionEntry* base_partition_entry, 
        int partition_num) {
    unsigned int start = base_partition_entry->start;;
    unsigned int end = start + base_partition_entry->length;
    unsigned int offset = FIRST_PARTITION_OFFSET;
    unsigned char new_sector[sector_size_bytes];
    PartitionEntry partition_entry;
    while (partition_num != 4) {
        if (start >= end) {
            partition_entry.type = INVALID_TYPE;
            return partition_entry;
        }
        read_sectors(start, 1, new_sector);
#ifdef PARTITION_ENTRY_DEBUG
        print_sector(new_sector);
#endif
        read_partition_entry(new_sector, start, 
                offset, &partition_entry);
        if (partition_entry.start + partition_entry.length > end) {
            partition_entry.type = INVALID_TYPE;
            return partition_entry;
        }
        if (partition_entry.type == DOS_EXTENDED_PARTITION) {
            return read_ebr(&partition_entry, partition_num);
        } else {
            --partition_num;
            // if not EBR, next partition is following current partition
            start = partition_entry.start + partition_entry.length;
        }
    }
    return partition_entry;
}

/*
 * read the specific partition's information
 *  1) partition type
 *  2) start sector
 *  3) parition size
 *  If the partition does not exist, set the return partition
 *  entry's type to 0
 */
int read_partition_info(int partition_num) {
    unsigned char MBR[sector_size_bytes];
    read_sectors(0, 1, MBR);  // read the sector 0
#ifdef PARTITION_ENTRY_DEBUG
    print_sector(MBR);
#endif
    if (partition_num <= 4) {  // request the first four partition entries
        read_mbr(MBR, &partition_entry, partition_num);
    } else {  
        int i;
        PartitionEntry base_partition_entry;
        // find which primary partition is extended.
        // NOTICE: at most one partition can be extended
        for (i = 1; i <= 4; ++i) {  
            read_mbr(MBR, &base_partition_entry, i);
            if (base_partition_entry.type == DOS_EXTENDED_PARTITION) {
                break;
            }
        }
        if (i > 4) {  // no extended partition in the first four entries
            partition_entry.type = INVALID_TYPE;
            return -1;
        }
        PartitionEntry entry = read_ebr(&base_partition_entry, partition_num);
        partition_entry.type = entry.type;
        partition_entry.start = entry.start;
        partition_entry.length = entry.length;
    }
    return 0;
}

/*
 * Read a partition entry from MBR or EBR, which starts at a given offset
 *  If the partition does not exist, set the return partition
 *  entry's type to 0
 */
void read_partition_entry(unsigned char* section, int start, 
        int offset, PartitionEntry* partition_entry) {
    partition_entry->type = section[offset + 4];
    partition_entry->start = start + 
        parse_bytes_to_decimal_u(section, offset + 8, offset + 12);
    partition_entry->length = 
        parse_bytes_to_decimal_u(section, offset + 12, offset + 16);
}

/*
 * Print out the partition entry's type, start sector and
 * number of sectors
 */
int print_partition_info(int partition_num) {
    read_partition_info(partition_num);
    if (IS_NULL_ENTRY(&partition_entry)) {
        printf("-1\n");
        return -1;
    } 
    printf("0x%02X %d %d\n", partition_entry.type, 
            partition_entry.start, partition_entry.length);
    return 0;
}
