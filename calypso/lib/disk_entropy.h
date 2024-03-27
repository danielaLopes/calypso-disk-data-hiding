#ifndef DISK_ENTROPY_H
#define DISK_ENTROPY_H


#define ENCRYPTED_THRESHOLD 7
// #define ENCRYPTED_THRESHOLD 7.174
#define PLAINTEXT_THRESHOLD 4.347

/* Value that is going to be used to decide which blocks will encode encrypted Calypso data */
#define ENTROPY_THRESHOLD 0

/* 
 * To help the fact that we cannot use floating point inside the kernel 
 * This way we can alliviate the rounding imprecisions
 * The higher this value, the less imprecise it is going to be
 */
#define FIXED_POINT_FACTOR 100000


int shannon_entropy(unsigned char *block);

/**
 * @returns the number of free blocks with high entropy
 */
unsigned long classify_free_blocks_entropy(unsigned long *physical_blocks_bitmap, unsigned long *high_entropy_blocks_bitmap, unsigned long nbits);


#endif