#include <crypto/hash.h>
#include <crypto/sha.h>

#include "debug.h"
#include "data_hiding.h"
#include "block_encryption.h"

#include "hkdf.h"


/**
 * Taken from https://elixir.bootlin.com/linux/v5.4/source/fs/crypto/hkdf.c
 * 
 * This allows us to obtain a key of length HKDF_HASHLEN bytes
 * from a smaller string such as a password
 */
static int hkdf_extract(struct crypto_shash *hmac_tfm, const u8 *ikm,
			unsigned int ikmlen, u8 prk[HKDF_HASHLEN])
{
    // TODO don't know if the salt is actually being applied
    // TODO: change this to strcpy
	static const u8 salt[HKDF_HASHLEN] = SALT;
	SHASH_DESC_ON_STACK(desc, hmac_tfm);
	int err;

    /* Sets initial key for the message digest */
	err = crypto_shash_setkey(hmac_tfm, salt, HKDF_HASHLEN);
	if (err)
		return err;

	desc->tfm = hmac_tfm;
    /* Calculate message digest for buffer
     * Combination of crypto_shash_init,
     * crypto_shash_update and crypto_shash_final
     * 
     * @desc: operational state handle already initialized
     * @ikm: input data to be added to the message digest
     * @ikmlen: length of the input data
     * @prk: output buffer filled with the message digest
     */
	err = crypto_shash_digest(desc, ikm, ikmlen, prk);

    // debug_args(KERN_INFO, __func__, "salt after: %s\n", salt);
    // debug_args(KERN_INFO, __func__, "ikm: %s\n", ikm);
    // debug_args(KERN_INFO, __func__, "prk in hkdf_extract: %s\n", prk);

	shash_desc_zero(desc);
    // debug(KERN_INFO, __func__, "AFTER shash_desc_zero\n");
	return err;
}

int calypso_hkdf(struct crypto_shash *hmac_tfm, const u8 *master_key,
		    //   unsigned int master_key_size, struct calypso_skcipher_def *cipher)
            unsigned int master_key_size, struct calypso_skcipher_def *cipher,
            unsigned char *key)
{
	u8 encryption_key[HKDF_HASHLEN];
	int err;
    // TODO don't know if the salt is actually being applied
    // TODO: change this to strcpy
    // TODO: see if salt should be used for generating key before cipher or after
	static const u8 salt[HKDF_HASHLEN] = SALT;

    /* Allocates a cipher handle for a message digest
    Returns the cipher handle required for any subsequent 
    invocation for that message digest */
	hmac_tfm = crypto_alloc_shash(HKDF_HMAC_ALG, 0, 0);
	if (IS_ERR(hmac_tfm)) {
		debug_args(KERN_ERR, __func__, "Error allocating " HKDF_HMAC_ALG ": %ld\n",
			    PTR_ERR(hmac_tfm));
		return PTR_ERR(hmac_tfm);
	}

    /* Returns the size for the message digest created */
	if (WARN_ON(crypto_shash_digestsize(hmac_tfm) != sizeof(encryption_key))) {
		err = -EINVAL;
		goto err_free_tfm;
	}

    /* Increases the size of the password given to be able to be a key */
	err = hkdf_extract(hmac_tfm, master_key, master_key_size, encryption_key);
	if (err)
		goto err_free_tfm;

    /* Sets a key for a message digest
    @prk: buffer holding the key */
	err = crypto_shash_setkey(hmac_tfm, encryption_key, sizeof(encryption_key));
	if (err)
		goto err_free_tfm;

    calypso_init_block_encryption_key(encryption_key, salt, cipher);

    // TODO: see if this is correct!!!!
    // err = crypto_cipher_setkey(hmac_tfm->base.crt_u.cipher, encryption_key, sizeof(encryption_key));
	// if (err)
	// 	goto err_free_tfm;

    // debug_args(KERN_INFO, __func__, "prk: %s\n", encryption_key);

	goto out;

err_free_tfm:
	crypto_free_shash(hmac_tfm);
out:
	memzero_explicit(encryption_key, sizeof(encryption_key));
	return err;
}