/*
 * Main function of my fsck program
 * It accepts the following arguments:
 *  1) i: disk image
 *  2) p: partition number to print information
 *
 * Author: Xiaoxiang Wu
 * Andrew ID: xiaoxiaw
 */

#include "common.h"
#include "util.h"

extern void correct_partition(int partition_num);
extern int print_partition_info(int partition_num);

//#define DEBUG

void usage(const char* progname) {
    printf("Usage: %s [options]\n", progname);
    printf("Program Options:\n");
    printf("  -p <partition number>    partition to read\n");
    printf("  -i <disk image>          path to disk image\n");
    printf("  -h                       help information");
}

int main(int argc, char** argv) {
    int opt;
    int print_partition_num = -1;
    int correct_partition_num = -1;
    char* image_path;
    char help = 0;

    while ((opt = getopt(argc, argv, "p:f:i:?h")) != EOF) {
        switch (opt) {
            case 'p':
                print_partition_num = atoi(optarg);
                break;
            case 'f':
                correct_partition_num = atoi(optarg);
                break;
            case 'i':
                image_path = optarg;
                break;
            case 'h':
                help = 1;
                break;
        }
    }

    if (help == 1 || optind != 5) {
        usage(argv[0]);
    }

#ifdef DEBUG
    printf("print partion number: %d\n", print_partition_num);
    printf("correct partion number: %d\n", correct_partition_num);
    printf("image path: %s\n", image_path);
#endif

    if ((device = open(image_path, O_RDWR)) == -1) {
        perror("Fail to open disk image\n");
        exit(-1);
    }

    if (print_partition_num != -1) {
        return print_partition_info(print_partition_num);
    } else if (correct_partition_num != -1) {
        correct_partition(correct_partition_num);
    }

    return 0;
}
