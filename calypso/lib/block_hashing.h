#ifndef BLOCK_HASHING_H
#define BLOCK_HASHING_H


struct sdesc {
    struct shash_desc shash;
    char ctx[];
};


int calypso_hash_block(const unsigned char *data, unsigned int datalen,
             unsigned char *digest);


#endif