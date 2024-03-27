#ifndef BLOCK_ENCRYPTION_H
#define BLOCK_ENCRYPTION_H

#include <linux/crypto.h>


#define ENCRYPTION_KEY_LEN 32 // 32 bytes, 256 bits

/* tie all data structures together */
struct calypso_skcipher_def {
    struct scatterlist *sg;
    struct crypto_skcipher *tfm;
    struct skcipher_request *req;
    struct crypto_wait *wait;
};

// int calypso_init_block_encryption_key(unsigned char *key, char *salt, 
//                         struct calypso_skcipher_def *cipher);
int calypso_init_block_encryption_key(unsigned char *key, char *salt, struct calypso_skcipher_def *cipher);

int calypso_decrypt_block(struct calypso_skcipher_def *cipher,
					     u8 *ciphertext, const u8 *plaintext, unsigned char *key);

int calypso_encrypt_block(struct calypso_skcipher_def *cipher,
					     u8 *ciphertext, const u8 *plaintext, unsigned char *key);    

void calypso_cleanup_block_encryption_key(struct calypso_skcipher_def *cipher);                     

#endif