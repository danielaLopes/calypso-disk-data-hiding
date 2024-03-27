#ifndef DATA_HIDING_H
#define DATA_HIDING_H

#include "virtual_device.h"
#include "block_encryption.h"
#include "mtwister.h"


// #define SEPARATOR "\x25\x26" // %&
// #define SEPARATOR_SIZE 2


#define MAX_PASSWORD_LEN 40

#define SALT "F\x18\xad$\xbd\xc2\xa5\xd9d.\xd1\xc0\x92\x1b)\xb4"
#define RANDOM_NUM_UPPER_LIMIT 100000000

/* Establishes a maximum number of blocks to look for when searching
for the initial Calypso block
If no consistent block is found (hashes match), Calypso assumes there
is no metadata to recover and assigns new blocks to hide the metadata */
#define MAX_METADATA_ITERS 40

/* TODO: understand with it needs to be 8, with less the cipher_text length is 4108, with 8 or more it is 4088 < 4096 */
// #define BLOCK_BYTES  4096U * 3U / 4U - 8U /* due to base64 performed by Fernet increasing */
#define BLOCK_BYTES  4096

#define UNSIGNED_LONG_LEN 8

#define SALT_BYTES_LEN 16
#define HMAC_BYTES_LEN 32

// Data structure
#define HASHED_CONTENTS_BYTES_LEN 32
/*
 * If we write -1 instead of the equivalent 18446744073709551615 to the metadata 
 * it will convert fine and only require 8 bytes instead of 21 bytes 
 */
#define NEXT_BLOCK_BYTES_LEN sizeof(unsigned long) 
// #define NEXT_BLOCK_BYTES_LEN 21 // -1 or 18446744073709551615 has 20 bytes
#define METADATA_BYTES_LEN BLOCK_BYTES - (NEXT_BLOCK_BYTES_LEN + HASHED_CONTENTS_BYTES_LEN)

#define METADATA_START 0
#define NEXT_BLOCK_START METADATA_START + METADATA_BYTES_LEN
#define HASHED_CONTENTS_START NEXT_BLOCK_START + NEXT_BLOCK_BYTES_LEN

// TODO: temporary
#define SEED "123456789"
#define PASSWORD "123456789-daniela"


/* tie all data structures together */
struct skcipher_def {
    struct scatterlist sg;
    struct crypto_skcipher *tfm;
    struct skcipher_request *req;
    struct crypto_wait wait;
};

int test_skcipher(void);

void calypso_pad_last_block_metadata_string(char *string, unsigned int padding_len);
void calypso_pad_next_block_string(char *string, unsigned int padding_len);

void calypso_unpad_last_block_metadata_string(char *string, unsigned int str_len);

void calypso_generate_random_number(u8 *random_num_buf, unsigned long len);

int calypso_get_first_block_num_random_to_read(MTRand r, unsigned char *metadata, 
        unsigned long *metadata_to_physical_block_mapping, struct file *metadata_file, 
        unsigned char *read_metadata_block, unsigned int bitmap_data_len, 
        unsigned long total_physical_blocks, unsigned long seed, 
        unsigned long *first_block_num, unsigned long *first_random_num, 
        unsigned long *cur_block_num, struct calypso_skcipher_def *cipher, 
        unsigned char *key);
int calypso_get_first_block_num_random_to_write(unsigned long *metadata_to_physical_block_mapping, unsigned long *physical_blocks_bitmap, unsigned long *high_entropy_blocks_bitmap, unsigned long total_physical_blocks, unsigned char *seed_str, unsigned long *first_block_num, unsigned long *first_random_num);

unsigned long calypso_calc_metadata_size_in_blocks(loff_t bitmap_data_len, unsigned long mappings_data_len);
unsigned long calypso_calc_mappings_metadata_size(unsigned long total_virtual_blocks);
loff_t calypso_calc_bitmap_metadata_size(unsigned long total_physical_blocks);

int calypso_encode_data_block(unsigned long metadata_blocks_num, unsigned long *metadata_to_physical_block_mapping, unsigned char *metadata_to_write, unsigned long bitmap_data_len, unsigned long mappings_data_len, unsigned long block_index, struct block_device *physical_dev, unsigned long *physical_blocks_bitmap, unsigned long *high_entropy_blocks_bitmap, unsigned long total_physical_blocks, unsigned long *cur_block_num, bool is_last_block, struct calypso_skcipher_def *cipher, unsigned char *key);
int calypso_retrieve_data_block(unsigned char *metadata, unsigned long *metadata_to_physical_block_mapping, struct file *metadata_file, unsigned char *read_metadata_block, unsigned long bitmap_data_len, unsigned long block_index, unsigned long total_physical_blocks, unsigned long *cur_block_num, struct calypso_skcipher_def *cipher, unsigned char *key);

int calypso_encode_hidden_metadata(unsigned long bitmap_data_len, 
        unsigned long mappings_data_len, unsigned long *metadata_to_physical_block_mapping, 
        unsigned long n_metadata_blocks, struct block_device *physical_dev, 
        unsigned long *physical_blocks_bitmap, unsigned long *high_entropy_blocks_bitmap,
        unsigned long total_physical_blocks, 
        struct crypto_shash *sym_enc_tfm, struct calypso_skcipher_def *cipher, 
        unsigned long virtual_nr_blocks,
        unsigned long *virtual_to_physical_block_mapping);

int calypso_retrieve_hidden_metadata(unsigned long bitmap_data_len, 
        unsigned long mappings_data_len, unsigned long *metadata_to_physical_block_mapping, 
        unsigned long n_metadata_blocks, struct block_device *physical_dev, 
        unsigned long *physical_blocks_bitmap, unsigned long *high_entropy_blocks_bitmap,
        unsigned long total_physical_blocks, 
        struct crypto_shash *sym_enc_tfm, struct calypso_skcipher_def *cipher, 
        unsigned long virtual_nr_blocks, unsigned long *virtual_nr_blocks_ptr,
        unsigned long *virtual_to_physical_block_mapping, 
        unsigned long *physical_to_virtual_block_mapping);


#endif