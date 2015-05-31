CC_FILES=readwrite.c myfsck.c partitionEntry.c util.c superBlock.c groupDescriptor.c inode.c dir.c dataBlock.c correct.c

CC=gcc

CCFLAGS=-g

all: myfsck

myfsck: clean
	$(CC) $(CC_FILES) $(CCFLAGS) -o myfsck

clean:
	rm -rf myfsck

tar: 
	tar cvf myfsck.tar *.c *.h Makefile
