#CMU 18-746/15-746 Storage Systems
## Overview
EXT2 file system checking tools.

## Print Partition Information
### Usage
./myfsck -p \<partition number\> -i \<disk image file\>

### Example
./myfsck -p 1 -i diskimage

Return: 0x83 63 48132

Running myfsck on a partition that does not exist should return -1. 

./myfsck -p 10 -i diskimage

Return: -1

## Correct Partition Error
#### Usage
./myfsck -f \<partition number\> -i \<disk image file\>

If the user specifies -f 0, the tool will correct disk errors on every ext2 partition contained in the disk image.

### Errors to check
* Pass 1: Directory pointers (see McKusick & Kowalski, section 3.7). Verify for each directory - that the first directory entry is “.” which self-references, and that the second directory entry is “..” which references its parent inode. If your tool finds an error, it should print a short description of the error, and correct the entry.

* Pass 2: Unreferenced inodes (section 3.5). Check that all allocated inodes are referenced in a directory entry somewhere. If you find an unreferenced inode, place it in the /lost+found directory - the name of this new entry in the directory should be the inode number. (i.e., if inode number 1074 is an allocated but unreferenced inode, create a file or directory entry - /lost+found/1074.)

* Pass 3: Inode link count (section 3.5). Count the number of directory entries that point to each inode (e.g., the number of hard links) and compare that to the inode link counter. If your tool finds a discrepancy, it should print a short description of the error, and update the inode link counter.

* Pass 4: Block allocation bitmap (section 3.3). Walk the directory tree and verify that the block bitmap is correct. If your tool finds a block that should (or should not) be marked in the bitmap, it should print a short description of the error, and correct the bitmap.
For this part, running the following command should fix disk errors on the specified partition.

## Resources
* Information on partition tables: 

		http://www.tldp.org/HOWTO/Large-Disk-HOWTO-6.html 
		
		http://www.tldp.org/HOWTO/Large-Disk-HOWTO-13.html

* Information on ext2 file system internals: 

		http://www.tldp.org/LDP/tlk/fs/filesystem.html 

		http://homepage.smc.edu/morgan_david/cs40/analyze-ext2.htm 

		http://www.nongnu.org/ext2-doc/ext2.html 

		http://wiki.osdev.org/Ext2

* Information about MBR, EBR, EXT2 : 

		http://en.wikipedia.org/wiki/Master_boot_record 

		http://en.wikipedia.org/wiki/Extended_boot_record
