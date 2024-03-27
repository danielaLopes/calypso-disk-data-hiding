// #include <stdio.h>
// #include <stdlib.h>
// #include <math.h>
#include <linux/log2.h>
#include <linux/bitmap.h>

#include "debug.h"
#include "file_io.h"
#include "fs_utils.h"

#include "disk_entropy.h"

static int count_chars_in_block(unsigned char *block, int *counters)
{
    unsigned int i;
    int count = 0;

    for (i = 0; i < 4096; i++)
    {
        counters[block[i]]++;
    }

    return count;
}

/*
 * n symbols within a block -> 8
 * p(xi) is the probability of the ith bit 
 *      in event x series of n symbols
 *      when there are 256 possibilities, the 
 *      entropy value is bounded within the 
 *      range of 0 to 8
 * -(summation[i=1;n](p(xi)log2(p(xi))))
 *
 * Each parameter block is a disk block and it has 4096 bytes
 * Each byte has 8 bits, thus each byte has 2^8 = 256 different possibilities
 */
int shannon_entropy(unsigned char *block)
{
    unsigned int i;
    int entropy = 0;
    int px;
    int counters[256] = {0};
    int log_factor = ilog2(FIXED_POINT_FACTOR);

    count_chars_in_block(block, counters);
    for (i = 0; i < 256; i++)
    {
        /* count number of bytes with code i in a block
        Then divide by the size of the block, thus resulting
        in the probabillity of a certain byte being of code i */

        px = counters[i] * FIXED_POINT_FACTOR / 4096;
        // debug_args(KERN_INFO, __func__, "%d; char: %c; counters[i]: %d; px: %d\n", i, i, counters[i], px);

        // ilog2(2);
        if (px > 0)
        {
            // entropy += -px * ilog2(px);
            entropy += -px * (ilog2(px) - log_factor);
            // debug_args(KERN_INFO, __func__, "ilog2(px): %d; entropy: %d\n", ilog2(px), entropy);
        }
    }

    return entropy;
}

unsigned long classify_free_blocks_entropy(unsigned long *physical_blocks_bitmap, unsigned long *high_entropy_blocks_bitmap, unsigned long nbits)
{
    unsigned int i;
    unsigned char block[4096 + 1];
    int entropy;
    unsigned long offset_within_file;
    unsigned long cur_block;
    unsigned int rs, re, start = 0;
    unsigned int bytes_in_region;
    unsigned long free_high_entropy_blocks = 0;

    struct file *metadata_file = calypso_open_file(PHYSICAL_DISK_NAME, O_CREAT|O_RDWR, 0755);

    // bitmap_for_each_clear_region(physical_blocks_bitmap, rs, re, start, 12200)
    bitmap_for_each_clear_region(physical_blocks_bitmap, rs, re, start, nbits)
	{
        bytes_in_region = re - rs;
        for (i = 0; i < bytes_in_region; i++)
        {
            cur_block = rs + i;
            offset_within_file = cur_block * 4096;
            calypso_read_file_with_offset(metadata_file, offset_within_file, block, 4096);
            entropy = shannon_entropy(block) / FIXED_POINT_FACTOR;
            if (entropy >= ENTROPY_THRESHOLD)
            {
                free_high_entropy_blocks++;
                bitmap_set(high_entropy_blocks_bitmap, cur_block, 1);
            }
        } 
    }

    return free_high_entropy_blocks;
}