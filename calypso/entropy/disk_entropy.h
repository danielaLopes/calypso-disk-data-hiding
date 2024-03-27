#ifndef DISK_ENTROPY_H
#define DISK_ENTROPY_H


#define ENCRYPTED_THRESHOLD 7.174
#define PLAINTEXT_THRESHOLD 4.347


double shannon_entropy(unsigned char *block);

int count_chars_in_block(unsigned char *block, int *counters);


#endif