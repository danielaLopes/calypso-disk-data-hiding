#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "disk_entropy.h"

// $ gcc -Wall disk_entropy.c -o disk_entropy -lm

int count_chars_in_block(unsigned char *block, int *counters)
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
double shannon_entropy(unsigned char *block)
{
    unsigned int i;
    double entropy = 0;
    double px;
    int counters[256] = {0};

    count_chars_in_block(block, counters);
    // for (i = 0; i < 8; i++)
    for (i = 0; i < 256; i++)
    {
        /* count number of bytes with code i in a block
        Then divide by the size of the block, thus resulting
        in the probabillity of a certain byte being of code i */

        px = counters[i] / (double)4096;
        if (px > 0)
        {
            entropy += -px * log2(px);
        }
    }

    return entropy;
}

// int main()
// {
//     long int i;
//     unsigned char block[4096 + 1];
//     double entropy;
//     long int size, total_blocks;

//     /* We need to read the file /dev/sda8 block by block, 
//     because it may not fit all in memory */
//     // FILE *fptr = fopen("/dev/sda8", "rb");
//     FILE *fptr = fopen("/dev/sdb", "rb");
//     if (!fptr)
//     {
//         printf("Error opening /dev/sda8");
//         return -1;
//     }

//     /* Obtain size of the disk */
//     fseek(fptr, 0L, SEEK_END);
//     size = ftell(fptr);
//     total_blocks = size / 4096;

//     /* Place cursor back at the beginning */
//     fseek(fptr, 0L, SEEK_SET);

//     /* just to advance cursor */
//     // for (i = 0; i < 12157; i++)
//     // {
//     //     fread(block, 1, 4096, fptr);
//     // }
//     /* For each block, we apply the entropy function */ 
//     // for (i = 0; i < 10; i++)
//     // for (i = 12157; i < 12180; i++)
//     // for (i = 0; i < 86; i++)
//     // for (i = 0; i < 122; i++)
//     // for (i = 0; i < 526; i++)
//     // for (i = 0; i < 950; i++)

//     for (i = 0; i < total_blocks; i++)
//     {
//         fread(block, 1, 4096, fptr);
//         entropy = shannon_entropy(block);

//         if (entropy < PLAINTEXT_THRESHOLD)
//         {
//             printf("Entropy for block %d is: %f; LOW ENTROPY, generally for semi empty blocks\n", i, entropy);
//         }
//         else if (entropy < ENCRYPTED_THRESHOLD && entropy >= PLAINTEXT_THRESHOLD)
//         {
//             printf("Entropy for block %d is: %f; PLAINTEXT\n", i, entropy);
//         }
//         else if (entropy >= ENCRYPTED_THRESHOLD)
//         {
//             printf("Entropy for block %d is: %f; ENCRYPTED\n", i, entropy);
//         }
//     }

//     fclose(fptr);

//     return 0;
// }