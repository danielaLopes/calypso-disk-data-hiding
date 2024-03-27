#ifndef HKDF_H
#define HKDF_H


#define HKDF_HMAC_ALG		"hmac(sha512)"
#define HKDF_HASHLEN		SHA512_DIGEST_SIZE


// int calypso_hkdf(struct crypto_shash *hmac_tfm, const u8 *master_key,
		    //   unsigned int master_key_size, struct calypso_skcipher_def *cipher);

int calypso_hkdf(struct crypto_shash *hmac_tfm, const u8 *master_key,
            unsigned int master_key_size, struct calypso_skcipher_def *cipher,
            unsigned char *key);


#endif