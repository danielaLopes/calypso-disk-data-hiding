#include <crypto/internal/skcipher.h>

#include "debug.h"
#include "data_hiding.h"

#include "block_encryption.h"


// int calypso_init_block_encryption_key(unsigned char *key, char *salt, struct calypso_skcipher_def *cipher)
int calypso_init_block_encryption_key(unsigned char *key, char *salt, struct calypso_skcipher_def *cipher)
{
    struct crypto_skcipher *skcipher = NULL;
    struct skcipher_request *req = NULL;
    int ret = -EFAULT;
    cipher = kmalloc(sizeof(struct calypso_skcipher_def), GFP_KERNEL);

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
                      cipher->wait);

    if (crypto_skcipher_setkey(skcipher, key, ENCRYPTION_KEY_LEN)) {
        debug(KERN_ERR, __func__, "key could not be set\n");
        ret = -EAGAIN;
        goto out;
    }

    cipher->tfm = skcipher;
    cipher->req = req;

    return ret;

out:
    if (skcipher)
        crypto_free_skcipher(skcipher);
    if (req)
        skcipher_request_free(req);

    return ret;
}

/**
 * crypto_cipher_decrypt_one() - decrypt one block of ciphertext
 * @tfm: cipher handle
 * @dst: points to the buffer that will be filled with the plaintext
 * @src: buffer holding the ciphertext to be decrypted
 *
 * Invoke the decryption operation of one block. The caller must ensure that
 * the plaintext and ciphertext buffers are at least one block in size.
 */
int calypso_decrypt_block(struct calypso_skcipher_def *cipher,
					     u8 *ciphertext, const u8 *plaintext, unsigned char *key)
{
    int ret;
    int rc;

    char *src_buf;
    char *dst_buf;

    // TODO: BEGIN HERE
    struct skcipher_def sk;
    struct crypto_skcipher *skcipher = NULL;
    struct skcipher_request *req = NULL;
    // TODO: END HERE

    char *ivdata = kmalloc(16, GFP_KERNEL);
    if (!ivdata) {
        debug(KERN_ERR, __func__, "could not allocate ivdata\n");
        goto out;
    }
    snprintf(ivdata, SALT_BYTES_LEN, "%s", SALT);
    // debug(KERN_INFO, __func__, "after SALT\n");
    
    // TODO: BEGIN HERE
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

    if (crypto_skcipher_setkey(skcipher, key, ENCRYPTION_KEY_LEN)) {  
        debug(KERN_ERR, __func__, "key could not be set\n");
        ret = -EAGAIN;
        goto out;
    }

    // debug_args(KERN_INFO, __func__, "key: %s\n", key);

    sk.tfm = skcipher;
    sk.req = req;
    // TODO: END HERE

    /* We encrypt one block of 4096 */
    /* Places the contents of the plaintext into a scatterlist in sk.sg */
    // sg_init_one(cipher->sg, plaintext, BLOCK_BYTES);
    sg_init_one(&sk.sg, ciphertext, BLOCK_BYTES);
    // debug(KERN_INFO, __func__, "after sg_init_one\n");
    // skcipher_request_set_crypt(cipher->req, cipher->sg, cipher->sg, BLOCK_BYTES, ivdata);
    skcipher_request_set_crypt(req, &sk.sg, &sk.sg, BLOCK_BYTES, ivdata);
    // debug(KERN_INFO, __func__, "after skcipher_request_set_crypt\n");
    // crypto_init_wait(cipher->wait);
    crypto_init_wait(&sk.wait);
    // debug(KERN_INFO, __func__, "after crypto_init_wait\n");
    // src_buf = sg_virt(cipher->req->src);
    src_buf = sg_virt(req->src);
    // debug_args(KERN_INFO, __func__, "CIPHERTEXT before: %s \n", src_buf);

    /* encrypt data */
    // rc = crypto_wait_req(crypto_skcipher_encrypt(cipher->req), cipher->wait);
    rc = crypto_wait_req(crypto_skcipher_decrypt(sk.req), &sk.wait);
    // debug(KERN_INFO, __func__, "after crypto_wait_req\n");
    if (rc)
    {
        debug_args(KERN_INFO, __func__, "skcipher encrypt returned with result %d\n", rc);
    }
    // debug(KERN_INFO, __func__, "END BLOCK ENCRYPTION\n");
    
    /* Obtain data from scatterlist through its virtual address */
    src_buf = sg_virt(req->src);
    dst_buf = sg_virt(req->dst);
    // src_buf = sg_virt(cipher->req->src);
    // dst_buf = sg_virt(cipher->req->dst);

    /* Plaintext changes when it gets encrypted, so no point in printing this */
    // debug_args(KERN_INFO, __func__, "TEXT TO ENCRYPT: %s \n", plaintext);
    // debug_args(KERN_INFO, __func__, "PLAINTEXT after: %s \n", src_buf);

    /* Print resulting ciphertext */
    // debug_args(KERN_INFO, __func__, "PLAINTEXT: %s \n", dst_buf);

    /* Copy ciphered text result to buffer */
    memcpy(plaintext, dst_buf, BLOCK_BYTES);

out:
    if (ivdata)
        kfree(ivdata);

    return rc;
    // int rc;
    // char *scratchpad = kzalloc(16, GFP_KERNEL);

    // /* We encrypt one block */
    // sg_init_one(cipher->sg, scratchpad, 16);
    // skcipher_request_set_crypt(cipher->req, cipher->sg, cipher->sg, 16, SALT);
    // crypto_init_wait(cipher->wait);

    // /* decrypt data */
    // rc = crypto_wait_req(crypto_skcipher_decrypt(cipher->req), cipher->wait);

    // if (rc)
    // {
    //     debug_args(KERN_INFO, __func__, "skcipher encrypt returned with result %d\n", rc);
    // }

    // kfree(scratchpad);
    // return rc;

	// crypto_cipher_crt(tfm)->cit_decrypt_one(crypto_cipher_tfm(tfm),
	// 					dst, src);
}

/**
 * crypto_cipher_encrypt_one() - encrypt one block of plaintext
 * @tfm: cipher handle
 * @dst: points to the buffer that will be filled with the ciphertext
 * @src: buffer holding the plaintext to be encrypted
 *
 * Invoke the encryption operation of one block. The caller must ensure that
 * the plaintext and ciphertext buffers are at least one block in size.
 */
// unsigned int calypso_encrypt_block(struct calypso_skcipher_def *cipher,
// 					     u8 *dst, const u8 *src)
// {
//     int rc;

//     debug(KERN_INFO, __func__, "BEING BLOCK ENCRYPTION\n");

//     if (!cipher->sg)
//     {
//         debug(KERN_ERR, __func__, "No cipher->sg!\n");
//     }
//     if (!cipher->tfm)
//     {
//         debug(KERN_ERR, __func__, "No cipher->tfm!\n");
//     }
//     if (!cipher->req)
//     {
//         debug(KERN_ERR, __func__, "No cipher->req!\n");
//     }
//     if (!cipher->wait)
//     {
//         debug(KERN_ERR, __func__, "No cipher->wait!\n");
//     }

//     /* We encrypt one block */
//     // sg_init_one(cipher->sg, scratchpad, 16);
//     sg_init_one(cipher->sg, scratchpad, 16);
//     debug(KERN_INFO, __func__, "after sg_init_one\n");
//     skcipher_request_set_crypt(cipher->req, cipher->sg, cipher->sg, 16, SALT);
//     debug(KERN_INFO, __func__, "after skcipher_request_set_crypt\n");
//     crypto_init_wait(cipher->wait);
//     debug(KERN_INFO, __func__, "after crypto_init_wait\n");

//     /* encrypt data */
//     rc = crypto_wait_req(crypto_skcipher_encrypt(cipher->req), cipher->wait);
//     debug(KERN_INFO, __func__, "after crypto_wait_req\n");
//     if (rc)
//     {
//         debug_args(KERN_INFO, __func__, "skcipher encrypt returned with result %d\n", rc);
//     }
//     debug(KERN_INFO, __func__, "END BLOCK ENCRYPTION\n");

//     return rc;

// 	// crypto_cipher_crt(tfm)->cit_encrypt_one(crypto_cipher_tfm(tfm),
// 	// 					dst, src);
// }

// static unsigned int test_skcipher_encdec(struct skcipher_def *sk,
//                      int enc)
// {
//     int rc;

//     if (enc)
//     {
//         rc = crypto_wait_req(crypto_skcipher_encrypt(sk->req), &sk->wait);
//     } else
//         rc = crypto_wait_req(crypto_skcipher_decrypt(sk->req), &sk->wait);

//     if (rc)
//     {
//         pr_info("skcipher encrypt returned with result %d\n", rc);
//     }
//     return rc;
// }

int calypso_encrypt_block(struct calypso_skcipher_def *cipher,
					     u8 *ciphertext, const u8 *plaintext, unsigned char *key)
{
    int ret;
    int rc;

    char *src_buf;
    char *dst_buf;

    // TODO: BEGIN HERE
    struct skcipher_def sk;
    struct crypto_skcipher *skcipher = NULL;
    struct skcipher_request *req = NULL;
    // TODO: END HERE

    char *ivdata = kmalloc(16, GFP_KERNEL);
    if (!ivdata) {
        debug(KERN_ERR, __func__, "could not allocate ivdata\n");
        goto out;
    }
    snprintf(ivdata, SALT_BYTES_LEN, "%s", SALT);
    // debug(KERN_INFO, __func__, "after SALT\n");
    
    // TODO: BEGIN HERE
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

    if (crypto_skcipher_setkey(skcipher, key, ENCRYPTION_KEY_LEN)) {  
        debug(KERN_ERR, __func__, "key could not be set\n");
        ret = -EAGAIN;
        goto out;
    }

    // debug_args(KERN_INFO, __func__, "key: %s\n", key);

    sk.tfm = skcipher;
    sk.req = req;
    // TODO: END HERE

    /* We encrypt one block of 4096 */
    /* Places the contents of the plaintext into a scatterlist in sk.sg */
    // sg_init_one(cipher->sg, plaintext, BLOCK_BYTES);
    sg_init_one(&sk.sg, plaintext, BLOCK_BYTES);
    // debug(KERN_INFO, __func__, "after sg_init_one\n");
    // skcipher_request_set_crypt(cipher->req, cipher->sg, cipher->sg, BLOCK_BYTES, ivdata);
    skcipher_request_set_crypt(req, &sk.sg, &sk.sg, BLOCK_BYTES, ivdata);
    // debug(KERN_INFO, __func__, "after skcipher_request_set_crypt\n");
    // crypto_init_wait(cipher->wait);
    crypto_init_wait(&sk.wait);
    // debug(KERN_INFO, __func__, "after crypto_init_wait\n");
    // src_buf = sg_virt(cipher->req->src);
    src_buf = sg_virt(req->src);
    // debug_args(KERN_INFO, __func__, "PLAINTEXT before: %s \n", src_buf);

    /* encrypt data */
    // rc = crypto_wait_req(crypto_skcipher_encrypt(cipher->req), cipher->wait);
    rc = crypto_wait_req(crypto_skcipher_encrypt(sk.req), &sk.wait);
    // debug(KERN_INFO, __func__, "after crypto_wait_req\n");
    if (rc)
    {
        debug_args(KERN_INFO, __func__, "skcipher encrypt returned with result %d\n", rc);
    }
    // debug(KERN_INFO, __func__, "END BLOCK ENCRYPTION\n");
    
    /* Obtain data from scatterlist through its virtual address */
    src_buf = sg_virt(req->src);
    dst_buf = sg_virt(req->dst);
    // src_buf = sg_virt(cipher->req->src);
    // dst_buf = sg_virt(cipher->req->dst);

    /* Plaintext changes when it gets encrypted, so no point in printing this */
    // debug_args(KERN_INFO, __func__, "TEXT TO ENCRYPT: %s \n", plaintext);
    // debug_args(KERN_INFO, __func__, "PLAINTEXT after: %s \n", src_buf);

    /* Print resulting ciphertext */
    // debug_args(KERN_INFO, __func__, "CIPHERTEXT: %s \n", dst_buf);

    /* Copy ciphered text result to buffer */
    memcpy(ciphertext, dst_buf, BLOCK_BYTES);

out:
    if (ivdata)
        kfree(ivdata);

    return rc;
}

void calypso_cleanup_block_encryption_key(struct calypso_skcipher_def *cipher)
{
    if (cipher)
    {
        if (cipher->tfm)
            crypto_free_skcipher(cipher->tfm);
        if (cipher->req)
            skcipher_request_free(cipher->req);
        kfree(cipher);
    }
}