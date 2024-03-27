#ifndef GLOBAL_H
#define GLOBAL_H


#define CALYPSO_DEV_NAME "calypso"

// #define CALYPSO_VIRTUAL_NR_BLOCKS_DEFAULT 1000
#define CALYPSO_VIRTUAL_NR_BLOCKS_DEFAULT 100

//#define PHYSICAL_DISK_NAME "/dev/sda"
#define PHYSICAL_DISK_NAME "/dev/sda8"
//#define PHYSICAL_DISK_NAME "/dev/sda5"

#define DISK_SECTOR_SIZE 512
#define KERNEL_SECTOR_SIZE 512

// TODO: check if max unsigned long value is 4294967295 or 18446744073709551615
// -1 in unsigned long is 18446744073709551615, but I am afraid some things stop working, chane after tests
#define MAX_UNSIGNED_LONG 4294967295

#define BLOCKS_IN_A_GB 262144


#endif