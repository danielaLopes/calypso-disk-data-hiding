#include <linux/string.h>
#include <linux/crypto.h>
#include <crypto/internal/rng.h>
#include <crypto/internal/skcipher.h>
#include <crypto/drbg.h>
#include <linux/bitmap.h>

#include "debug.h"
#include "requests.h"
#include "file_io.h"
#include "hkdf.h"
#include "block_hashing.h"
#include "persistence.h"
#include "fs_utils.h"

#include "data_hiding.h"


// -------------------------------------------------------
// ------------------- Helper functions ------------------ 
// -------------------------------------------------------

// --- Code for Key derivation function and encryption ---
/* PBKDF2HMAC applies a HMAC to an input password and a salt and
repeats the process many times to produce a derived key */
// TODO: initialize key outside to maintain memory location
unsigned char *calypso_generate_key_from_password(char *password, unsigned char *key)
{
    unsigned char salted_password[SALT_BYTES_LEN + MAX_PASSWORD_LEN + 1];
    unsigned char kdf[HMAC_BYTES_LEN + 1];
    
    snprintf(salted_password, SALT_BYTES_LEN + MAX_PASSWORD_LEN, "%s%s", SALT, password);
    debug_args(KERN_INFO, __func__, "SALTED PASSWORD %s: ", salted_password);
    return key;
}

/* Perform cipher operation */
unsigned int test_skcipher_encdec(struct skcipher_def *sk,
                     int enc)
{
    int rc;

    if (enc)
    {
        rc = crypto_wait_req(crypto_skcipher_encrypt(sk->req), &sk->wait);
    } else
        rc = crypto_wait_req(crypto_skcipher_decrypt(sk->req), &sk->wait);

    if (rc)
    {
        pr_info("skcipher encrypt returned with result %d\n", rc);
    }
    return rc;
}

/* Initialize and trigger cipher operation */
int test_skcipher(void)
{
    struct skcipher_def sk;
    struct crypto_skcipher *skcipher = NULL;
    struct skcipher_request *req = NULL;
    char *scratchpad = NULL;
    char *ivdata = NULL;
    unsigned char key[32];
    int ret = -EFAULT;

    skcipher = crypto_alloc_skcipher("cbc-aes-aesni", 0, 0);
    if (IS_ERR(skcipher)) {
        debug(KERN_INFO, __func__, "could not allocate skcipher handle\n");
        return PTR_ERR(skcipher);
    }

    req = skcipher_request_alloc(skcipher, GFP_KERNEL);
    if (!req) {
        debug(KERN_ERR, __func__, "could not allocate skcipher request\n");
        ret = -ENOMEM;
        goto out;
    }

    skcipher_request_set_callback(req, CRYPTO_TFM_REQ_MAY_BACKLOG,
                      crypto_req_done,
                      &sk.wait);

    /* AES 256 with random key */
    get_random_bytes(&key, 32);
    if (crypto_skcipher_setkey(skcipher, key, 32)) {
        debug(KERN_ERR, __func__, "key could not be set\n");
        ret = -EAGAIN;
        goto out;
    }

    /* IV will be random */
    ivdata = kmalloc(16, GFP_KERNEL);
    if (!ivdata) {
        debug(KERN_ERR, __func__, "could not allocate ivdata\n");
        goto out;
    }
    snprintf(ivdata, SALT_BYTES_LEN, "%s", SALT);
    debug_args(KERN_INFO, __func__, "CHECKPOINT salt %s\n", ivdata);

    /* Input data will be random */
    scratchpad = kmalloc(16, GFP_KERNEL);
    if (!scratchpad) {
        pr_info("could not allocate scratchpad\n");
        goto out;
    }
    get_random_bytes(scratchpad, 16);

    sk.tfm = skcipher;
    sk.req = req;

    /* We encrypt one block */
    sg_init_one(&sk.sg, scratchpad, 16);
    skcipher_request_set_crypt(req, &sk.sg, &sk.sg, 16, ivdata);
    crypto_init_wait(&sk.wait);

    /* encrypt data */
    ret = test_skcipher_encdec(&sk, 1);
    if (ret)
        goto out;

    debug(KERN_INFO, __func__, "Encryption triggered successfully\n");

out:
    if (skcipher)
        crypto_free_skcipher(skcipher);
    if (req)
        skcipher_request_free(req);
    if (ivdata)
        kfree(ivdata);
    if (scratchpad)
        kfree(scratchpad);
    return ret;
}


// --------- Code for other cryptography utils ---------

void calypso_pad_last_block_metadata_string(char *string, unsigned int padding_len)
{
    size_t i;
    for (i = 0; i < padding_len; i++) 
    {
        strcat(string, " ");
    }
}

/**
 * By keeping these two functions separated, we avoid unpadding the next block string
 */
void calypso_pad_next_block_string(char *string, unsigned int padding_len)
{
    size_t i;
    for (i = 0; i < padding_len; i++) 
    {
        strcat(string, "\0");
    }
}

/**
 * Unpads the ' ' characters that were added to fill the last block's metadata
 * section
 * Does not work for other blocks
 */
void calypso_unpad_last_block_metadata_string(char *string, unsigned int str_len)
{
    size_t i, j;
    size_t k = 0;
    for (i = j = 0; i < str_len; i++) 
    {
        if (string[i] != ' ')
        {
            string[j++] = string[i];
        }
    }
    string[j] = '\0';
}

int calypso_get_first_block_num_random_to_read(MTRand r, unsigned char *metadata, 
        unsigned long *metadata_to_physical_block_mapping, struct file *metadata_file, 
        unsigned char *read_metadata_block, unsigned int bitmap_data_len, 
        unsigned long total_physical_blocks, unsigned long seed, 
        unsigned long *first_block_num, unsigned long *first_random_num, 
        unsigned long *cur_block_num, struct calypso_skcipher_def *cipher, 
        unsigned char *key)
{
    int ret;
    unsigned int iter = 0;
    unsigned long random_num, random_physical_block;

    /* Generate first random number and calculate respective block number */
    random_num = genRandLong(&r);
    // debug_args(KERN_INFO, __func__, "---> random_num BEFORE: %lu\n", random_num);

    random_physical_block = random_num % total_physical_blocks;
    debug_args(KERN_INFO, __func__, "---> random_physical_block BEFORE %lu\n", random_physical_block);
    (*first_random_num) = random_physical_block;
    (*cur_block_num) = random_physical_block;

    ret = calypso_retrieve_data_block(metadata, metadata_to_physical_block_mapping, metadata_file, read_metadata_block, bitmap_data_len, 0UL, total_physical_blocks, cur_block_num, cipher, key);
    debug_args(KERN_INFO, __func__, "ret %d\n", ret);
    
    while (ret != 0 && iter < MAX_METADATA_ITERS)
    {   
        random_num = genRandLong(&r);
        // debug_args(KERN_INFO, __func__, "---> random_num: %lu\n", random_num);

        random_physical_block = random_num % total_physical_blocks;
        (*cur_block_num) = random_physical_block;
        debug_args(KERN_INFO, __func__, "---> random_physical_block %lu\n", random_physical_block);

        /* Check if the pseudo-random-generator has gone around and is repeating numbers */
        // if (random_physical_block == (*first_random_num))
        // {
        //     // TODO: see better what to do in this case, right now we increment the seed by one and reseed
        //     debug(KERN_INFO, __func__, "---> RESEED! seed needed to be incremented!\n");
        //     seed++;
        //     // TODO: POTENTIALLY PROBLEMATIC BECAUSE THIS INTRODUCES A SEG FSULT FOR SOME REASON
        //     r = seedRand(seed);

        //     random_num = genRandLong(&r);
        //     debug_args(KERN_INFO, __func__, "---> random_num: %lu\n", random_num);

        //     random_physical_block = random_num % total_physical_blocks;
        //     debug_args(KERN_INFO, __func__, "---> random_physical_block %lu\n", random_physical_block);
        // }

        ret = calypso_retrieve_data_block(metadata, metadata_to_physical_block_mapping, metadata_file, read_metadata_block, bitmap_data_len, 0UL, total_physical_blocks, cur_block_num, cipher, key);

        iter++;
    }

    // This means there is no metadata to retrieve
    if (ret != 0)
    {
        return ret;
    }

    // TODO: careful with this!
    metadata_to_physical_block_mapping[0] = random_physical_block;
    // TODO: two ways to do this
    // calypso_set_bit(bitmap, random_physical_block);

    // here we don't set the bit because it was already previously set and we are going to retrieve the updated bitmap now
    (*first_block_num) = random_physical_block;
    debug_args(KERN_INFO, __func__, "FIRST BLOCK!!!!! %lu; %lu; %lu\n", random_physical_block, random_num, total_physical_blocks);

    return 0;
}


int calypso_get_first_block_num_random_to_write(unsigned long *metadata_to_physical_block_mapping, unsigned long *physical_blocks_bitmap, unsigned long *high_entropy_blocks_bitmap, unsigned long total_physical_blocks, unsigned char *seed_str, unsigned long *first_block_num, unsigned long *first_random_num)
{
    unsigned long random_num, random_physical_block;
    unsigned long seed;
    MTRand r;

    /* First block number is already assigned */
    random_physical_block = metadata_to_physical_block_mapping[0];
    if (random_physical_block < MAX_UNSIGNED_LONG)
    {
        // do nothing!
        debug_args(KERN_INFO, __func__, "FIRST BLOCK WAS ALREADY ASSIGNED! %lu, %lu, %lu\n", random_physical_block, MAX_UNSIGNED_LONG, (unsigned long)(-1));
    }
    /* Still hasn't assigned a first block */
    else
    {
        /* Get seed value and seed the pseudo-random number generator */
        kstrtol(seed_str, 10, &seed);
        // debug_args(KERN_INFO, __func__, "---> AFTER SEED %lu\n", seed);
        r = seedRand(seed);
        // debug(KERN_INFO, __func__, "---> AFTER seedRand\n");

        /* Generate first random number and calculate respective block number */
        random_num = genRandLong(&r);
        // debug_args(KERN_INFO, __func__, "---> random_num: %lu\n", random_num);

        random_physical_block = random_num % total_physical_blocks;
        (*first_random_num) = random_physical_block;
        debug_args(KERN_INFO, __func__, "---> random_physical_block %lu\n", random_physical_block);
        debug_args(KERN_INFO, __func__, "---> (*first_random_num) %lu\n", (*first_random_num));

        // while (test_bit(random_physical_block, bitmap))
        while (!test_bit(random_physical_block, high_entropy_blocks_bitmap))
        {
            random_num = genRandLong(&r);
            debug_args(KERN_INFO, __func__, "--while--> random_num: %lu\n", random_num);

            random_physical_block = random_num % total_physical_blocks;
            debug_args(KERN_INFO, __func__, "---while----> random_physical_block %lu\n", random_physical_block);

            /* Check if the pseudo-random-generator has gone around and is repeating numbers */
            if (random_physical_block == (*first_random_num))
            {
                // TODO: see better what to do in this case, right now we increment the seed by one and reseed
                debug(KERN_INFO, __func__, "---> RESEED! seed needed to be incremented!\n");
                seed++;
                r = seedRand(seed);

                random_num = genRandLong(&r);
                // debug_args(KERN_INFO, __func__, "---> random_num: %lu\n", random_num);

                random_physical_block = random_num % total_physical_blocks;
                debug_args(KERN_INFO, __func__, "---> random_physical_block %lu\n", random_physical_block);
            }
        }

        metadata_to_physical_block_mapping[0] = random_physical_block;
        calypso_update_bitmaps(physical_blocks_bitmap, high_entropy_blocks_bitmap, random_physical_block);
        // calypso_set_bit(bitmap, random_physical_block);
    }
  
    (*first_block_num) = random_physical_block;
    debug_args(KERN_INFO, __func__, "FIRST BLOCK!!!!! %lu; %lu; %lu\n", random_physical_block, random_num, total_physical_blocks);

    return 0;
}


// -------------------------------------------------------
// -------------------- Main functions ------------------- 
// -------------------------------------------------------

/**
 * Right when Calypso is initialized, it needs to assign the Calypso blocks 
 * mappings that are going to be dedicated to persisting metadata. 
 * This method should only be called after an unsuccessful retrieval of 
 * metadata, in which case we can assume Calypso has not been installed yet.
 * This method determines the amount of blocks that are going to be necessary
 * based on the size of the underlying native partition to store the
 * bitmap and the size of Calypso block device for the mappings
 */
unsigned long calypso_calc_metadata_size_in_blocks(loff_t bitmap_data_len, unsigned long mappings_data_len)
{
    unsigned int metadata_bytes_len = (unsigned int)METADATA_BYTES_LEN;

    /* 8 is to account for initial number of Calypso blocks */
    unsigned long total_size = 8 + bitmap_data_len + mappings_data_len;

    if (total_size % metadata_bytes_len == 0)
    {
        return total_size / metadata_bytes_len;
    }
    else {
        return total_size / metadata_bytes_len + 1;
    }
}

unsigned long calypso_calc_mappings_metadata_size(unsigned long total_virtual_blocks)
{
    /* Each mapping takes a block number which is an unsigned int,
    so the total size occupied by the mappings is 
    8 bytes * total_virtual_blocks */   
    debug_args(KERN_INFO, __func__, "total_virtual_blocks: %lu; sizeof(unsigned long): %d\n", total_virtual_blocks, sizeof(unsigned long));

    return sizeof(unsigned long) * total_virtual_blocks;
}

loff_t calypso_calc_bitmap_metadata_size(unsigned long total_physical_blocks)
{
    /* bits to bytes */
    int remainder = total_physical_blocks % 8;
    /* The bitmap encodes 8 blocks in each byte, so the space occupied 
    is the following */ 
    loff_t bitmap_data_len;
    if (remainder > 0)
    {
        bitmap_data_len = total_physical_blocks / 8 + 1;
    }
    else
    {
        bitmap_data_len = total_physical_blocks / 8 ;
    }

    return bitmap_data_len;
}

int calypso_retrieve_data_block(unsigned char *metadata, unsigned long *metadata_to_physical_block_mapping, struct file *metadata_file, unsigned char *read_metadata_block, unsigned long bitmap_data_len, unsigned long block_index, unsigned long total_physical_blocks, unsigned long *cur_block_num, struct calypso_skcipher_def *cipher, unsigned char *key)
{   
    int ret; 

    unsigned long next_block;
    unsigned char next_block_str[NEXT_BLOCK_BYTES_LEN + 1];
    unsigned char hashed_block_contents[HASHED_CONTENTS_BYTES_LEN + 1];
    unsigned char check_hashed_block_contents[HASHED_CONTENTS_BYTES_LEN + 1];
    int next_block_padding_len;
    unsigned char *plaintext_block_contents = kzalloc(BLOCK_BYTES + 1, GFP_KERNEL);
    unsigned int metadata_padding_len;
    /* For some reason METADATA_BYTES_LEN could not be casted to unsigned and start_byte was overflowing */
    unsigned int metadata_bytes_len = METADATA_BYTES_LEN;
    unsigned long start_byte = (unsigned long)block_index * metadata_bytes_len;
    unsigned int num_chars_copied;
    unsigned long offset_within_file = (*cur_block_num) * 4096;

    calypso_read_file_with_offset(metadata_file, offset_within_file, read_metadata_block, 4096);
    // calypso_read_file_with_offset(metadata_file, offset_within_file, ciphered_block_contents, 4096);
    // if ((block_index == 0 && (*cur_block_num) == 606403) || (block_index == 0 && (*cur_block_num) == 63387) || block_index == 1)
    // debug_args(KERN_INFO, __func__, "read_metadata_block: %s\n", read_metadata_block);
    calypso_decrypt_block(cipher, read_metadata_block, plaintext_block_contents, key);
    // if ((block_index == 0 && (*cur_block_num) == 606403) || (block_index == 0 && (*cur_block_num) == 63387) || block_index == 1)
    // debug_args(KERN_INFO, __func__, "plaintext_block_contents: %s\n", plaintext_block_contents);
    // memcpy(plaintext_block_contents, read_metadata_block, 4096);

    /* Obtain the required fields from the encoded block */
    // memcpy(metadata, read_metadata_block + METADATA_START, METADATA_BYTES_LEN);

    // memcpy(metadata + start_byte, read_metadata_block + METADATA_START, METADATA_BYTES_LEN);
    // memcpy(next_block_str, read_metadata_block + NEXT_BLOCK_START, NEXT_BLOCK_BYTES_LEN);
    // memcpy(hashed_block_contents, read_metadata_block + HASHED_CONTENTS_START, HASHED_CONTENTS_BYTES_LEN);
    memcpy(metadata + start_byte, plaintext_block_contents + METADATA_START, METADATA_BYTES_LEN);
    memcpy(next_block_str, plaintext_block_contents + NEXT_BLOCK_START, NEXT_BLOCK_BYTES_LEN);
    debug_args(KERN_INFO, __func__, "----> NEXT BLOCK BEFORE KSTRTOUL! next_block_str: %s\n", next_block_str);
    next_block_str[NEXT_BLOCK_BYTES_LEN] = '\0';
    // strcpy(next_block_str, plaintext_block_contents + NEXT_BLOCK_START);
    memcpy(hashed_block_contents, plaintext_block_contents + HASHED_CONTENTS_START, HASHED_CONTENTS_BYTES_LEN);

    debug_args(KERN_INFO, __func__, ">>>>> block_index: %lu; *cur_block_num: %lu\n", block_index, *cur_block_num);

    /* Check hash */
    ret = calypso_hash_block(read_metadata_block, metadata_bytes_len + NEXT_BLOCK_BYTES_LEN, check_hashed_block_contents);
    // if ((block_index == 0 && (*cur_block_num) == 606403) || (block_index == 0 && (*cur_block_num) == 63387) || block_index == 1)
    // {

    //     ret = kstrtoul("-1", 10, &next_block);
    //     debug_args(KERN_INFO, __func__, "XXXXX next_block -1: %lu\n", next_block);

    //     ret = kstrtoul("12157\03", 10, &next_block);
    //     debug_args(KERN_INFO, __func__, "XXXXX next_block -1 A): %lu; ret: %d\n", next_block, ret);

    //     ret = kstrtoul("12157\0\03", 10, &next_block);
    //     debug_args(KERN_INFO, __func__, "XXXXX next_block -1 B): %lu; ret: %d\n", next_block, ret);
    // }
    
    if (memcmp(hashed_block_contents, check_hashed_block_contents, HASHED_CONTENTS_BYTES_LEN) == 0)
    {
        debug(KERN_INFO, __func__, "----> HASHES MATCH! IT IS THE BLOCK WE WANT!\n");
    }
    else {
        debug(KERN_INFO, __func__, "----> HASHES DO NOT MATCH! NOT THE BLOCK WE WANT!\n");
        ret = -1;
        goto error_incoherent_block;
    }

    ret = kstrtoul(next_block_str, 10, &next_block);
    debug_args(KERN_INFO, __func__, "----> NEXT BLOCK AFTER KSTRTOUL! next_block_str: %s; next_block: %lu; (long) next_block: %ld\n", next_block_str, next_block, (long)next_block);

    if (ret != 0)
    {
        debug_args(KERN_ERR, __func__, "Error passing next_block_str into an unsigned long with code %d\n", ret);
        goto error_incoherent_block;
    }
    /* Take care of last block case */
    if (next_block == -1) {
        calypso_unpad_last_block_metadata_string(metadata, METADATA_BYTES_LEN);
        debug(KERN_INFO, __func__, "LAST BLOCK TO RETRIEVE!!!!\n");
    }
    /* Store metadata mappings in memory */
    else
    {
        metadata_to_physical_block_mapping[block_index + 1] = next_block; 
        // debug_args(KERN_INFO, __func__, "metadata_to_physical_block_mapping[block_index + 1]: %d; %lu\n", block_index + 1, metadata_to_physical_block_mapping[block_index + 1]);
    }

    (*cur_block_num) = next_block;

error_incoherent_block:
    kfree(plaintext_block_contents);

    return ret;
}

int calypso_retrieve_hidden_metadata(unsigned long bitmap_data_len, 
        unsigned long mappings_data_len, unsigned long *metadata_to_physical_block_mapping, 
        unsigned long n_metadata_blocks, struct block_device *physical_dev, 
        unsigned long *physical_blocks_bitmap, unsigned long *high_entropy_blocks_bitmap,
        unsigned long total_physical_blocks, 
        struct crypto_shash *sym_enc_tfm, struct calypso_skcipher_def *cipher, 
        unsigned long virtual_nr_blocks, unsigned long *virtual_nr_blocks_ptr,
        unsigned long *virtual_to_physical_block_mapping, 
        unsigned long *physical_to_virtual_block_mapping)
{
    struct file *metadata_file = calypso_open_file(PHYSICAL_DISK_NAME, O_CREAT|O_RDWR, 0755);

    unsigned int iter = 1;
    int ret = 0;

    unsigned long first_block_num, first_random_num;
    unsigned long cur_block_num;

    unsigned long metadata_bytes_len = (unsigned long)METADATA_BYTES_LEN;

    unsigned long *prev_bitmap, *res_bitmap;
    unsigned int res_rs, res_re;

    unsigned char *key = kzalloc(ENCRYPTION_KEY_LEN + 1, GFP_KERNEL);
    if (!key)
    {
        debug(KERN_ERR, __func__, "Could not allocate key!\n");
        return -ENOMEM;
    }

    unsigned long seed;
    MTRand r;

    /* Get seed value and seed the pseudo-random number generator */
    kstrtol(SEED, 10, &seed);

    r = seedRand(seed);

    /* The native disk is too big to read all at once, so we are going to read each block individually.
    By doing this outside, we only allocate this memory once and use this every time */
    unsigned char *read_metadata_block = kzalloc(4096, GFP_KERNEL);
    if (!read_metadata_block)
    {
        debug(KERN_ERR, __func__, "Could not allocate memory for read_metadata_block\n");
        ret = -ENOMEM;
        goto error_after_metadata_bitmap;
    }

    unsigned char *metadata = kzalloc(metadata_bytes_len * n_metadata_blocks + 1, GFP_KERNEL);
    if (!metadata)
    {
        debug(KERN_ERR, __func__, "Could not allocate memory for metadata\n");
        ret = -ENOMEM;
        goto cleanup_metadata_block;
    }

    /* Generates symmetric encryption key from given password */
    // TODO: input actual password
    calypso_hkdf(sym_enc_tfm, PASSWORD, sizeof(PASSWORD), cipher, key);

    // TODO: PROCESS PASSWORD USER PASSES AS INPUT
    ret = calypso_get_first_block_num_random_to_read(r, metadata, metadata_to_physical_block_mapping, metadata_file, read_metadata_block, bitmap_data_len, total_physical_blocks, SEED, &first_block_num, &first_random_num, &cur_block_num, cipher, key);
    if (ret != 0)
    {
        debug(KERN_INFO, __func__, "Calypso did not find a coherent first metadata block, so we assume there's no metadata to retrieve since it is the first execution of Calypso!\n");
        goto full_cleanup;
    }
    
    /* Retrieve number of Calypso blocks. This can be here since we have already read the first block. */
    char virtual_blocks_str[8+1];
    unsigned long virtual_blocks;
    memcpy(virtual_blocks_str, metadata, 8);
    virtual_blocks_str[8] = '\0';
    // debug_args(KERN_DEBUG, __func__, "$$$$$ virtual_blocks_str before %s; chars: %c; %c; %c; %c; %c; %c; %c; %c; %c; %c; \n", virtual_blocks_str, virtual_blocks_str[0], virtual_blocks_str[1], virtual_blocks_str[2], virtual_blocks_str[3], virtual_blocks_str[4], virtual_blocks_str[5], virtual_blocks_str[6], virtual_blocks_str[7], virtual_blocks_str[8], virtual_blocks_str[9]);
    ret = kstrtoul(virtual_blocks_str, 16, virtual_nr_blocks_ptr);
    if (ret != 0)
    {
        debug_args(KERN_ERR, __func__, "Error passing virtual_blocks_str into an unsigned long with code %d\n", ret);
        goto full_cleanup;
    }

    /* This needs to go on until the last block which will return cur_block_num == -1 */
    // IMPORTANT!! FOR SOME REASON, PRINTS HERE BLOCK THE SYSTEM
    while (cur_block_num != -1 && ret == 0)
    {
        ret = calypso_retrieve_data_block(metadata, metadata_to_physical_block_mapping, metadata_file, read_metadata_block, bitmap_data_len, iter, total_physical_blocks, &cur_block_num, cipher, key);
        iter++;
    }
    if (iter != n_metadata_blocks)
    {
        debug_args(KERN_ERR, __func__, "Did not successfully retrieve %lu metadata blocks\n", n_metadata_blocks);
        goto full_cleanup;
    }

    /* We've read the whole file, so we can close it */
    calypso_close_file(metadata_file);

    /* Retrieve data from persisted bitmap */
    res_bitmap = bitmap_alloc(total_physical_blocks, GFP_KERNEL);
    if (!res_bitmap)
    {
        debug(KERN_ERR, __func__, "Could not allocate memory for resulting bitmap\n");
        ret = -ENOMEM;
        goto full_cleanup;
    }

    prev_bitmap = bitmap_alloc(total_physical_blocks, GFP_KERNEL);
    if (!prev_bitmap)
    {
        debug(KERN_ERR, __func__, "Could not allocate memory for previous bitmap\n"); 
        bitmap_free(res_bitmap);
        ret = -ENOMEM;
        goto full_cleanup;
    }

    // debug(KERN_DEBUG, __func__, "BEFORE RETRIEVING BITMAPS!!\n");
    bitmap_from_arr32(prev_bitmap, (u32 *)(metadata + 8), total_physical_blocks);
    /* Check differences between previous and updated bitmap */
    bitmap_xor(res_bitmap, prev_bitmap, physical_blocks_bitmap, total_physical_blocks);

    bitmap_for_each_set_region(res_bitmap, res_rs, res_re, 0, total_physical_blocks)
	{
        // TODO check if these blocks were mapped by Calypso, only those ones matter
        // if ()
		debug_args(KERN_DEBUG, __func__, "***** THE FOLLOWING REGION MIGHT BE CORRUPTED 2 ***** rs : %u; re: %u;\n", res_rs, res_re);
	}

    bitmap_free(res_bitmap);
    bitmap_free(prev_bitmap);

    /* Retrieve data from persisted mappings */
    unsigned long virtual;
    char physical_str[8+1];
    unsigned long physical;
    loff_t offset = 8 + bitmap_data_len; /* initial position to read mappings */
    debug_args(KERN_DEBUG, __func__, "XXXX before physical_str: %s; offset :%lu\n", physical_str, offset);
    // debug(KERN_DEBUG, __func__, "BEFORE RETRIEVING VIRTUAL MAPPINGS!!\n");
    // TODO: change from virtual_nr_blocks to this virtual_nr_blocks_ptr ??????
    for (virtual = 0; virtual < virtual_nr_blocks; virtual++)
    {
        memcpy(physical_str, metadata + offset, 8);
        physical_str[8] = '\0';
        offset += 8;
        ret = kstrtoul(physical_str, 16, &physical);
        debug_args(KERN_DEBUG, __func__, "XXXX virtual: %lu; physical: %lu; physical_str: %s\n", virtual, physical, physical_str);
        if (ret != 0)
        {
            debug_args(KERN_ERR, __func__, "Error passing physical_str into an unsigned long with code %d\n", ret);
            goto full_cleanup;
        }
        
        /* We only set the mappings if they are not the default value */
        // TODO: CHANGE BACK
        // if (physical < MAX_UNSIGNED_LONG && physical != 0)
        if (physical < MAX_UNSIGNED_LONG)
        {
            debug_args(KERN_DEBUG, __func__, "MAPPED virtual: %lu; physical: %lu\n", virtual, physical);
            virtual_to_physical_block_mapping[virtual] = physical;
            physical_to_virtual_block_mapping[physical] = virtual;
            // debug_args(KERN_DEBUG, __func__, "SET BIT IN READ physical: %lu\n", physical);
            calypso_update_bitmaps(physical_blocks_bitmap, high_entropy_blocks_bitmap, physical);
        }
    }

    unsigned long i;
    for (i = 0; i < n_metadata_blocks; i++)
    {
        if (metadata_to_physical_block_mapping[i] < MAX_UNSIGNED_LONG)
        {
            // debug_args(KERN_DEBUG, __func__, "SET BIT IN READ physical 2: %lu\n", metadata_to_physical_block_mapping[i]);
            calypso_update_bitmaps(physical_blocks_bitmap, high_entropy_blocks_bitmap, metadata_to_physical_block_mapping[i]);
        }
    }

    // debug(KERN_DEBUG, __func__, "AFTER RETRIEVING MAPPINGS\n");

full_cleanup:
    kfree(metadata);
cleanup_metadata_block:
    kfree(read_metadata_block);
error_after_metadata_bitmap:
    kfree(key);

    return ret;
}

// We are going to begin by encoding and deconding a single metadata block
int calypso_encode_data_block(unsigned long metadata_blocks_num, unsigned long *metadata_to_physical_block_mapping, unsigned char *metadata_to_write, unsigned long bitmap_data_len, unsigned long mappings_data_len, unsigned long block_index, struct block_device *physical_dev, unsigned long *physical_blocks_bitmap, unsigned long *high_entropy_blocks_bitmap, unsigned long total_physical_blocks, unsigned long *cur_block_num, bool is_last_block, struct calypso_skcipher_def *cipher, unsigned char *key)
{   
    int ret;
    unsigned char metadata[METADATA_BYTES_LEN + 1];
    unsigned long next_block = -1UL;
    unsigned char next_block_str[NEXT_BLOCK_BYTES_LEN + 1]; // TODO: +1? unsigned char?
    int next_block_padding_len;
    unsigned char *encoded_block_contents = kzalloc(BLOCK_BYTES + 1, GFP_KERNEL);
    unsigned char hashed_block_contents[HASHED_CONTENTS_BYTES_LEN + 1];
    // size is the same because the fs block size is multiple of the block cipher block size
    unsigned char *ciphered_block_contents = kzalloc(BLOCK_BYTES + 1, GFP_KERNEL);
    unsigned int metadata_padding_len;
    /* For some reason METADATA_BYTES_LEN could not be casted to unsigned and start_byte was overflowing */
    unsigned int metadata_bytes_len = METADATA_BYTES_LEN;
    unsigned long start_byte = (unsigned long)block_index * metadata_bytes_len;
    unsigned int num_chars_copied;

    if (!encoded_block_contents)
    {
        debug(KERN_ERR, __func__, "Could not allocate memory for encoded_block_contents\n");
    }
    if (!ciphered_block_contents)
    {
        debug(KERN_ERR, __func__, "Could not allocate memory for ciphered_block_contents\n");
    }

    // debug_args(KERN_INFO, __func__, "^^^^^^^^^^BLOCK with index %lu!\n", block_index);
    
    if (!is_last_block)
    {
        next_block = metadata_to_physical_block_mapping[block_index + 1];
    }

    /* if it is the last block, then the value is -1 but it can't be accessed */  
    // debug_args(KERN_INFO, __func__, "??? STORED NEXT BLOCK MAPPING %lu\n", next_block);
    /* We only want to set a new free block if we haven't assigned one already in previous executions
    If we have previously assigned, then next block will be different than -1 */
    if (next_block == -1)
    {
        debug(KERN_INFO, __func__, "NEXT BLOCK WAS NOT PREVIOUSLY ASSIGNED\n");
        /* We are only going to assign a next block if there is a next block */
        if (!is_last_block) 
        {
            next_block = 0;
            calypso_get_next_free_block(high_entropy_blocks_bitmap, total_physical_blocks, &next_block);
            // calypso_set_bit(bitmap, next_block);
            // calypso_clear_bit(bitmap, next_block);
            calypso_update_bitmaps(physical_blocks_bitmap, high_entropy_blocks_bitmap, next_block);
            metadata_to_physical_block_mapping[block_index + 1] = next_block;
            debug_args(KERN_INFO, __func__, "NEXT BLOCK IS %lu BECAUSE IT WAS NOT PREVIOUSLY ASSIGNED!\n", next_block);
        }
        /* There is no next block, so we just use -1 */
        else 
        {
            /* We don't even need to do anything here, just to make it more clear */
            debug(KERN_INFO, __func__, "NEXT BLOCK IS -1 DUE TO CURRENT BEING LAST BLOCK!\n");
            // next_block = -1;
        }
    }
    else 
    {
        debug_args(KERN_INFO, __func__, "NEXT BLOCK WAS PREVIOUSLY ASSIGNED! %d; %lu\n", block_index + 1, metadata_to_physical_block_mapping[block_index + 1]);
    }
    
    /* if the next block value was already assigned, we already retrieve it from metadata_to_physical_block_mapping */
    
    /* For these strings with strange characters we need to use memcpy because snprintf()
     stops at some character present in the strings */
    memcpy(metadata, metadata_to_write + start_byte, metadata_bytes_len);

    /* does metadata need padding */
    /* metadata allocated space must be atleast METADATA_BYTES_LEN + 1 */
    // TODO: this needs to change to accomodate mappings also, and it should only be done on the last block by verifying is_last_block condition
    // debug_args(KERN_INFO, __func__, "XXX -> CHECKPOINT bitmap_data_len: %lu; start_byte: %lu\n", bitmap_data_len, start_byte);
    if (is_last_block)
    {
        metadata_padding_len = metadata_blocks_num * metadata_bytes_len - (8 + bitmap_data_len + mappings_data_len);
        debug_args(KERN_INFO, __func__, "CHECKPOINT metadata_padding_len %d; %lu\n", metadata_padding_len, metadata_padding_len);
        if (metadata_padding_len < metadata_bytes_len) 
        {
            debug(KERN_INFO, __func__, "METADATA NEEDS PADDING!\n");
            calypso_pad_last_block_metadata_string(metadata, metadata_padding_len);
        }
        else
        {
            debug(KERN_INFO, __func__, "METADATA DOES NOT NEED PADDING!\n");
        }
    }

    /* We convert to from unsigned long to long so that -1 occupies 8 bytes instead of 21 bytes */
    snprintf(next_block_str, NEXT_BLOCK_BYTES_LEN, "%ld", (long)next_block);
    // debug_args(KERN_INFO, __func__, "----> BEFORE next_block_str: %s\n", next_block_str, next_block);

    /* Here there is no problem to use strlen because numbers are not 
    strange characters with nulls in the middle. That is only a problem for metadata */
    next_block_padding_len = NEXT_BLOCK_BYTES_LEN - strlen(next_block_str);
    if (next_block_padding_len > 0)
    {
        /* Pad the next block */
        calypso_pad_next_block_string(next_block_str, next_block_padding_len);
    }

    // debug_args(KERN_INFO, __func__, "----> NEXT BLOCK next_block_str: %s, next_block: %lu; (long) next_block: %ld\n", next_block_str, next_block, (long)next_block);

    memcpy(encoded_block_contents + METADATA_START, metadata, metadata_bytes_len);
    memcpy(encoded_block_contents + NEXT_BLOCK_START, next_block_str, NEXT_BLOCK_BYTES_LEN);
    /* Hash contents and include them in the block contents */
    ret = calypso_hash_block(encoded_block_contents, metadata_bytes_len + NEXT_BLOCK_BYTES_LEN, hashed_block_contents);
    memcpy(encoded_block_contents + HASHED_CONTENTS_START, hashed_block_contents, HASHED_CONTENTS_BYTES_LEN);

    /* 
    * Encrypt entire block with hash 
    * Both ciphered_block_contents and encoded_block_contents get encrypted content 
    */
    calypso_encrypt_block(cipher, ciphered_block_contents, encoded_block_contents, key);

    /* for the cast we want to get the address to the first position of the metadata array */
    new_bio_write_page((void *) encoded_block_contents, physical_dev, calypso_get_sector_nr_from_block((*cur_block_num), 0));  

    (*cur_block_num) = next_block;

    kfree(encoded_block_contents);
    kfree(ciphered_block_contents);

    return ret;
}

// void calypso_encode_hidden_metadata(unsigned long *metadata_to_physical_block_mapping, unsigned long n_metadata_blocks, struct block_device *physical_dev, unsigned long *bitmap, unsigned long total_physical_blocks, struct crypto_shash *sym_enc_tfm, struct calypso_skcipher_def *cipher)
int calypso_encode_hidden_metadata(unsigned long bitmap_data_len, 
        unsigned long mappings_data_len, unsigned long *metadata_to_physical_block_mapping, 
        unsigned long n_metadata_blocks, struct block_device *physical_dev, 
        unsigned long *physical_blocks_bitmap, unsigned long *high_entropy_blocks_bitmap, 
        unsigned long total_physical_blocks, 
        struct crypto_shash *sym_enc_tfm, struct calypso_skcipher_def *cipher, 
        unsigned long virtual_nr_blocks,
        unsigned long *virtual_to_physical_block_mapping)
{
    unsigned long i;
    int ret;

    unsigned long first_block_num, first_random_num;
    unsigned long cur_block_num;

    unsigned long metadata_bytes_len = (unsigned long)METADATA_BYTES_LEN;

    unsigned int res_rs, res_re;

    unsigned char *key = kzalloc(ENCRYPTION_KEY_LEN + 1, GFP_KERNEL);
    if (!key)
    {
        debug(KERN_ERR, __func__, "Could not allocate key!\n");
        return -ENOMEM;
    }

    unsigned char *metadata = kmalloc(metadata_bytes_len * n_metadata_blocks + 1, GFP_KERNEL);
    if (!metadata)
    {
        debug(KERN_ERR, __func__, "Could not allocate memory for metadata\n");
        ret = -ENOMEM;
        goto key_cleanup_encode_metadata;
    }

    u32 *bitmap_data = kmalloc(bitmap_data_len, GFP_KERNEL);
    if (!bitmap_data)
    {
        debug(KERN_ERR, __func__, "Could not allocate memory for bitmap metadata\n");
        ret = -ENOMEM;
        goto full_cleanup_encode_metadata;
    }
    bitmap_to_arr32(bitmap_data, physical_blocks_bitmap, total_physical_blocks);

    /* Store number of Calypso blocks in first block of metadata */
    char virtual_nr_blocks_str[8+1];
    snprintf(virtual_nr_blocks_str, 8+1, "%.8lx", virtual_nr_blocks);
    // debug_args(KERN_DEBUG, __func__, "# virtual_nr_blocks_str: %s\n", virtual_nr_blocks_str);
    memcpy(metadata, virtual_nr_blocks_str, 8);
    // debug_args(KERN_DEBUG, __func__, "# metadata: %s\n", metadata);

    /* 8 bytes to account for number of Calypso blocks */
    memcpy(metadata + 8, (unsigned char*)bitmap_data, bitmap_data_len);

    unsigned long virtual;
    char physical_str[8+1];
    for (virtual = 0; virtual < virtual_nr_blocks; virtual++)
    {
        snprintf(physical_str, 8+1, "%.8lx", virtual_to_physical_block_mapping[virtual]);
        memcpy(metadata + 8 + bitmap_data_len + 8 * virtual, physical_str, 8);
        debug_args(KERN_DEBUG, __func__, "ENCODING virtual: %lu; physical: %lu\n", virtual, virtual_to_physical_block_mapping[virtual]);
        debug_args(KERN_DEBUG, __func__, "ENCODING2 physical in hex: %.8s\n", metadata + bitmap_data_len + 8 * virtual);
    }
    

    /* Generates symmetric encryption key from given password */
    // TODO: input actual password
    // calypso_hkdf(sym_enc_tfm, PASSWORD, sizeof(PASSWORD), cipher);
    calypso_hkdf(sym_enc_tfm, PASSWORD, sizeof(PASSWORD), cipher, key);

    ret = calypso_get_first_block_num_random_to_write(metadata_to_physical_block_mapping, physical_blocks_bitmap, high_entropy_blocks_bitmap, total_physical_blocks, SEED, &first_block_num, &first_random_num);
    cur_block_num = first_block_num;
    /* First iteration is done outside */
    calypso_encode_data_block(n_metadata_blocks, metadata_to_physical_block_mapping, metadata, bitmap_data_len, mappings_data_len, 0UL, physical_dev, physical_blocks_bitmap, high_entropy_blocks_bitmap, total_physical_blocks, &cur_block_num, n_metadata_blocks == 1, cipher, key);

    // TODO this needs to go until there is no more metadata to be saved, and next_block needs to be set to -1
    for (i = 1; i < n_metadata_blocks; i++)
    {
        calypso_encode_data_block(n_metadata_blocks, metadata_to_physical_block_mapping, metadata, bitmap_data_len, mappings_data_len, i, physical_dev, physical_blocks_bitmap, high_entropy_blocks_bitmap, total_physical_blocks, &cur_block_num, (n_metadata_blocks - 1) == i, cipher, key);
    }

    kfree(bitmap_data);

full_cleanup_encode_metadata:
    kfree(metadata);

key_cleanup_encode_metadata:   
    kfree(key);

    return ret;
}