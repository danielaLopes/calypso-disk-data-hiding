/*
 * Checks out primary information on a partition
 * 
 * To be compared with the output of $ sudo fdisk -l
 * -> same values
 * Usage:
 *      $ gcc partition_info.c -o partition_info
 *      $ sudo ./partition_info /dev/sda 
 * 
 */

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#define MBR_DISK_SIGNATURE_OFFSET 440
#define PARTITION_ENTRY_SIZE 16 // sizeof(PartEntry)
#define PARTITION_TABLE_SIZE (4 * PARTITION_ENTRY_SIZE)

typedef struct
{
    unsigned char boot_type; // 0x00 - Inactive; 0x80 - Active (Bootable)
    unsigned char start_head;
    unsigned char start_sec:6;
    unsigned char start_cyl_hi:2;
    unsigned char start_cyl;
    unsigned char part_type;
    unsigned char end_head;
    unsigned char end_sec:6;
    unsigned char end_cyl_hi:2;
    unsigned char end_cyl;
    unsigned int abs_start_sec;
    unsigned int sec_in_part;
} PartEntry;

typedef struct
{
    unsigned char boot_code[MBR_DISK_SIGNATURE_OFFSET];
    unsigned int disk_signature;
    unsigned short pad;
    unsigned char pt[PARTITION_TABLE_SIZE];
    unsigned short signature;
} MBR;

void print_computed(unsigned int sector)
{
    unsigned int heads, cyls, tracks, sectors;

    sectors = sector % 63 + 1 /* As indexed from 1 */;
    tracks = sector / 63;
    cyls = tracks / 255 + 1 /* As indexed from 1 */;
    heads = tracks % 255;
    printf("(%3d/%5d/%1d)", heads, cyls, sectors);
}

int main(int argc, char *argv[])
{
    char *dev_file = "/dev/sda";
    int fd, i, rd_val;
    MBR m;
    PartEntry *p = (PartEntry *)(m.pt);

    if (argc == 2)
    {
        dev_file = argv[1];
    }
    if ((fd = open(dev_file, O_RDONLY)) == -1)
    {
        fprintf(stderr, "Failed opening %s: ", dev_file);
        perror("");
        return 1;
    }
    if ((rd_val = read(fd, &m, sizeof(m))) != sizeof(m))
    {
        fprintf(stderr, "Failed reading %s: ", dev_file);
        perror("");
        close(fd);
        return 2;
    }
    close(fd);
    printf("\nDOS type Partition Table of %s:\n", dev_file);
    printf("  B Start (H/C/S)   End (H/C/S) Type  StartSec    TotSec\n");
    for (i = 0; i < 4; i++)
    {
        printf("%d:%d (%3d/%4d/%2d) (%3d/%4d/%2d)  %02X %10d %9d\n",
            i + 1, !!(p[i].boot_type & 0x80),
            p[i].start_head,
            1 + ((p[i].start_cyl_hi << 8) | p[i].start_cyl),
            p[i].start_sec,
            p[i].end_head,
            1 + ((p[i].end_cyl_hi << 8) | p[i].end_cyl),
            p[i].end_sec,
            p[i].part_type,
            p[i].abs_start_sec, p[i].sec_in_part);
    }
    printf("\nRe-computed Partition Table of %s:\n", dev_file);
    printf("  B Start (H/C/S)   End (H/C/S) Type  StartSec    TotSec\n");
    for (i = 0; i < 4; i++)
    {
        printf("%d:%d ", i + 1, !!(p[i].boot_type & 0x80));
        print_computed(p[i].abs_start_sec);
        printf(" ");
        print_computed(p[i].abs_start_sec + p[i].sec_in_part - 1);
        printf(" %02X %10d %9d\n", p[i].part_type,
            p[i].abs_start_sec, p[i].sec_in_part);
    }
    printf("\n");
    return 0;
}