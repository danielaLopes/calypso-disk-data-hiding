/* 
 * Calypso block device driver
 * Block driver implementation responsible for reading 
 * and writing data to the shadow partition
 */
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/version.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/string.h>
#include <linux/types.h>
#include <linux/genhd.h> /* allow partitions and gendisk */
#include <linux/blkdev.h>
#include <linux/errno.h>
#include <linux/blk-mq.h>
/* ioctl */
#include <linux/hdreg.h>
#include <linux/blkpg.h>
/* remap io requests */
#include <trace/events/block.h>
/* tracing */
//#include <linux/blktrace_api.h>
#include <linux/device-mapper.h>

#include <linux/delay.h>

#include "../../lib/mtwister.h"
 
#include "../../lib/global.h"
#include "../../lib/debug.h"
#include "../../lib/disk_io.h"
#include "../../lib/file_io.h"
#include "../../lib/virtual_device.h"
#include "../../lib/physical_device.h"
#include "../../lib/fs_utils.h"
#include "../../lib/persistence.h"
#include "../../lib/data_hiding.h"
#include "../../lib/requests.h"
#include "../../lib/hkdf.h"
#include "../../lib/disk_entropy.h"
#include "../../lib/block_encryption.h"

/* Error codes: https://kdave.github.io/errno.h/ */


static blk_qc_t calypso_make_request(struct request_queue *q, struct bio *bio);
 
static int calypso_dev_open(struct block_device *bdev, fmode_t mode);

static void calypso_dev_close(struct gendisk *disk, fmode_t mode);

int calypso_dev_ioctl(struct block_device *bdev, fmode_t mode, unsigned cmd, unsigned long arg);

static void calypso_update_mappings(unsigned long virtual_block_nr, unsigned long next_free_physical_block_nr);

static void calypso_reset_mapping(unsigned long previous_physical_block_nr);

//static void calypso_print_bio_data(struct bio *bio);
static void calypso_print_bio_data(void *data);

static void calypso_bio_end_io(struct bio *bio);


static long blocks = CALYPSO_VIRTUAL_NR_BLOCKS_DEFAULT;
module_param(blocks, long, 0);
/* Decides whether Calypso retrieves stored metadata or if it is a fresh install */
static bool is_clean_start = false;
/* true for fresh install, false to retrieve metadata */
module_param(is_clean_start, bool, 0);
MODULE_PARM_DESC(is_clean_start, "Set to 1 for fresh install, 0 to retrieve metadata");

static blk_qc_t (*orig_request_fn)(struct request_queue *q, struct bio *bio) = NULL;


static struct calypso_blk_device *calypso_dev = NULL;

/*
 *
 */
static struct block_device_operations calypso_fops =
{
    .owner = THIS_MODULE,
    /*.open = calypso_dev_open,
    .release = calypso_dev_close,*/
    .ioctl = calypso_dev_ioctl,
};


struct trace_data {
	atomic64_t request_counter_read;
    atomic64_t request_counter_write;
    atomic64_t request_counter_write_calypso;
    atomic64_t request_counter_read_calypso;
    atomic64_t request_counter_other;
};

static struct trace_data trace_data = {
	.request_counter_read = ATOMIC64_INIT(0),
    .request_counter_write = ATOMIC64_INIT(0),
    .request_counter_write_calypso = ATOMIC64_INIT(0),
    .request_counter_read_calypso = ATOMIC64_INIT(0),
    .request_counter_other = ATOMIC64_INIT(0)
};


static void calypso_bio_end_io(struct bio *bio)
{
    debug(KERN_DEBUG, __func__, "Inside bio_endio of request to be copied\n");

    //calypso_print_bio_data(page_address(bio_page(bio)) + bio_offset(bio));
    //calypso_print_bio_data(page_address(calypso_dev->pending_page));

    /* Issue write request to copy data to next free block */
    //calypso_print_bio_data(page_address(calypso_dev->pending_page));
    new_bio_write_page(page_address(calypso_dev->pending_page), calypso_dev->physical_dev, calypso_get_sector_nr_from_block(calypso_dev->to_physical_block_pending_copy, 0));

    //calypso_print_bio_data(bio_data(calypso_dev->pending_bio));
    /* Finish processing the original write request that was going to override Calypso data */
    struct request_queue *q = bio->bi_disk->queue;
    //memcpy(bio_data(calypso_dev->pending_bio), calypso_dev->pending_data, 4095);
    // TODO check if this is doing anything, maybe submit_bio with a different flag?
    orig_request_fn(q, calypso_dev->pending_bio);
    
    //calypso_print_bio_data(bio);

    /* Once we receive the response of our read request, we issue a write request to the next free block with the contents read from the previous block */
    //new_bio_write_page(bio_data(bio), calypso_get_sector_nr_from_block(calypso_dev->to_physical_block_pending_copy, 0));

    // TODO: this is not uncommented! the mappings need to be returned to normal
    calypso_update_mappings(calypso_dev->physical_to_virtual_block_mapping[calypso_dev->from_physical_block_pending_copy], calypso_dev->to_physical_block_pending_copy);
    calypso_reset_mapping(calypso_dev->from_physical_block_pending_copy);
    calypso_dev->from_physical_block_pending_copy = -1;
    calypso_dev->to_physical_block_pending_copy = -1;

    bio_free_pages(bio);
	bio_put(bio);
}

//static void calypso_print_bio_data(struct bio *bio)
static void calypso_print_bio_data(void *data)
{
    size_t i;
    // char *ptr = (char *) bio_data(bio);
    char *ptr = (char *) data;
    debug(KERN_INFO, __func__, "Printing bio_data\n");

    // TODO change this!
    //for(i = 0; i < 4096; i++)
    for(i = 0; i < 20; i++)
    {
        debug_args(KERN_INFO, __func__, "%c", *ptr);
        ptr++;
    }
    debug(KERN_INFO, __func__, "\n");
}

static void calypso_update_bio_sector(struct bvec_iter *iter, sector_t new_sector) 
{
    iter->bi_sector = new_sector;
}

struct bio *calypso_split_bio(struct bio *bio, sector_t nsectors)
{
    struct request_queue *q = bio->bi_disk->queue;
    struct bio_set *bs = &(q->bio_split);

    return bio_split(bio, nsectors, GFP_NOIO, bs);
}

struct bio *calypso_clone_bio(struct bio *bio)
{
    struct request_queue *q = bio->bi_disk->queue;
    struct bio_set *bs = &(q->bio_split);

    return bio_clone_fast(bio, GFP_NOIO, bs);
}

static void calypso_update_mappings(unsigned long virtual_block_nr, unsigned long next_free_physical_block_nr)
{
    calypso_dev->virtual_to_physical_block_mapping[virtual_block_nr] = next_free_physical_block_nr;
    calypso_dev->physical_to_virtual_block_mapping[next_free_physical_block_nr] = virtual_block_nr;
}

static void calypso_reset_mapping(unsigned long previous_physical_block_nr)
{
    calypso_dev->physical_to_virtual_block_mapping[previous_physical_block_nr] = -1;
}

// TO TEST: 
// $ sudo insmod calypso_driver.ko is_clean_start=1
// $ sudo dd if=hello.txt count=1 seek=90 bs=4096 of=/dev/calypso0
// $ sudo dd if=/dev/calypso0 count=1 skip=90 bs=4096
// $ dmesg
static void calypso_make_encrypted_request(struct bio *bio)
{
    unsigned char data[4096 + 1];
    unsigned char result[4096 + 1];
    unsigned char hello_str[4096 + 1] = "HELOOOOOOOOOOOUUUUU XXXXXXX";

    unsigned char *key = kzalloc(ENCRYPTION_KEY_LEN + 1, GFP_KERNEL);
    struct calypso_skcipher_def *cipher;

    if (!key)
    {
        debug(KERN_ERR, __func__, "Could not allocate key!\n");
        return -ENOMEM;
    }

    debug_args(KERN_DEBUG, __func__, "~~~~~~ REQUEST SIZE: %lu\n", bio->bi_iter.bi_size);

    if (bio_data_dir(bio) == WRITE)
    {
        /* Reads request data, encrypts it, and replaces the request data with the encrypted data */
        memcpy(data, bio_data(bio), 4096);
        debug_args(KERN_DEBUG, __func__, "~~~~~~ WRITE REQUEST BEFORE: %s\n", data);
        // calypso_encrypt_block(cipher, data, result, key);
        calypso_encrypt_block(cipher, result, data, key);
        memcpy(bio_data(bio), result, 4096);
        debug_args(KERN_DEBUG, __func__, "~~~~~~ WRITE REQUEST AFTER: %s\n", result);

        // TODO: DEBUG
        // This does not match the unencrypted block, so it is not ok!
        calypso_decrypt_block(cipher, result, data, key);
        debug_args(KERN_DEBUG, __func__, "~~~~~~ DEBUG DECRYPT ENCRYPTED DATA: %s\n", data);

        // This matches the encrypted block above, so it's ok
        // calypso_encrypt_block(cipher, hello_str, result, key);
        calypso_encrypt_block(cipher, result, hello_str, key);
        debug_args(KERN_DEBUG, __func__, "~~~~~~ DEBUG hello_str ENCRYPTED: %s\n", result);
        // Is also wrong, but matches the decrypted data above
        calypso_decrypt_block(cipher, result, data, key);
        debug_args(KERN_DEBUG, __func__, "~~~~~~ DEBUG hello_str DECRYPTED: %s\n", data);
    }
    else
    {
        /* Reads request data, decrypts it, and replaces the request data with the decrypted data */
        memcpy(data, bio_data(bio), 4096);
        debug_args(KERN_DEBUG, __func__, "~~~~~~ READ REQUEST BEFORE: %s\n", data);
        calypso_decrypt_block(cipher, data, result, key);
        memcpy(bio_data(bio), result, 4096);
        debug_args(KERN_DEBUG, __func__, "~~~~~~ READ REQUEST AFTER: %s\n", result);


    }
    generic_make_request(bio);
}

void calypso_make_single_block_request(struct bio *bio, size_t i, unsigned int req_blocks_count_complete, unsigned int req_blocks_count_incomplete, sector_t physical_sector_nr)
{
    struct bio *new_bio;
    /* 
     * If we have a request to a single block left, we don't need
     * to split the bio further
     */
    if (req_blocks_count_complete == 0)
    {
        // debug(KERN_INFO, __func__, "*** INCOMPLETE BLOCK\n");
        // debug_args(KERN_INFO, __func__, "***** before bio_offset %lu\n", bio_offset(bio));
        calypso_update_bio_sector(&(bio->bi_iter), physical_sector_nr);
        // debug_args(KERN_INFO, __func__, "***** AFTER bio_offset %lu\n", bio_offset(bio));
        calypso_make_encrypted_request(bio);
    }
    else if (i < req_blocks_count_complete - 1)
    {
        // debug(KERN_INFO, __func__, "*** we need to split the bio\n");
        //calypso_create_bio_read_req2(calypso_dev->physical_dev, physical_sector_nr, 4096, bio);
        // debug_args(KERN_INFO, __func__, "***** bio bio_offset %lu\n", bio_offset(bio));
        new_bio = calypso_split_bio(bio, 8);
        // debug_args(KERN_INFO, __func__, "***** new_bio bio_offset %lu\n", bio_offset(new_bio));
        calypso_update_bio_sector(&(new_bio->bi_iter), physical_sector_nr);
        calypso_make_encrypted_request(new_bio);
    }
    else {
        // debug(KERN_INFO, __func__, "*** no need to split bio further\n");
        /* if there is an incomplete block, we still need to split once more */
        if (req_blocks_count_incomplete > 0) {
            // debug(KERN_INFO, __func__, "*** INCOMPLETE BLOCK after some complete blocks\n");
            // debug_args(KERN_INFO, __func__, "***** bio bio_offset %lu\n", bio_offset(bio));
            new_bio = calypso_split_bio(bio, 8);
            // debug_args(KERN_INFO, __func__, "***** new_bio bio_offset %lu\n", bio_offset(new_bio));
            calypso_update_bio_sector(&(new_bio->bi_iter), physical_sector_nr);
            calypso_make_encrypted_request(new_bio);
        }
        // debug_args(KERN_INFO, __func__, "***** BEFORE bio_offset %lu\n", bio_offset(bio));
        calypso_update_bio_sector(&(bio->bi_iter), physical_sector_nr);
        // debug_args(KERN_INFO, __func__, "***** AFTER bio_offset %lu\n", bio_offset(bio));
        calypso_make_encrypted_request(bio);
    }
    // debug(KERN_INFO, __func__, "*** after generic_make_request\n");
}

static blk_qc_t hooked_physical_make_request_fn(struct request_queue *q, struct bio *bio)
{
    // TODO: remember that these requests include the ones sent by calypso
    int is_set;
    unsigned long physical_block_nr;
    unsigned long virtual_block_nr;

    // if (bio_data_dir(bio) == WRITE) 
    // {
        /* Check if the request is within the sectors Calypso might be using from the underlying devices */
        if (bio->bi_iter.bi_sector > calypso_dev->first_physical_sector && bio->bi_iter.bi_sector < calypso_dev->last_physical_sector)
        {
            // debug(KERN_DEBUG, __func__, "####### REQUEST TO SDA8 #######\n");
            /* requests remapped by /dev/calypso0 */
            if (bio->bi_opf & (1 << __REQ_CALYPSO))
            {
                // debug(KERN_DEBUG, __func__, "REQUEST WAS ISSUED BY CALYPSO!!!!!!!!!!!\n");
                if (bio_data_dir(bio) == WRITE)
                { 
                    atomic64_inc(&trace_data.request_counter_write_calypso);
                    debug_args(KERN_INFO , __func__, "Received %lld WRITE requests FROM /dev/calypso0 to /dev/sda8 to sector %lld\n", atomic64_read(&trace_data.request_counter_write_calypso), bio->bi_iter.bi_sector);
                }
                else {
                    atomic64_inc(&trace_data.request_counter_read_calypso);
                    debug_args(KERN_INFO , __func__, "Received %lld READ requests FROM /dev/calypso0 to /dev/sda8 to sector %lld\n", atomic64_read(&trace_data.request_counter_read_calypso), bio->bi_iter.bi_sector);
                }    
            }
            /* regular requests that were not remapped by /dev/calypso0 */
            else 
            {
                debug(KERN_DEBUG, __func__, "REQUEST WAS not ISSUED BY CALYPSO!!!!!!!!!!!\n");
                debug_args(KERN_INFO, __func__, "***** HOOKED bio_offset %lu\n", bio_offset(bio));
                debug_args(KERN_INFO, __func__, "***** request size %u\n", bio->bi_iter.bi_size);
                physical_block_nr = calypso_get_block_nr_from_sector(bio->bi_iter.bi_sector, calypso_dev->first_physical_sector);
                if (physical_block_nr >= calypso_dev->physical_nr_blocks)
                {
                    // TODO take advantage of these blocks
                    /* Because there are sectors that aren't mapped to file system blocks at the end */
                    debug_args(KERN_ERR , __func__, "Block %lld is out of range! Native disk only has %lld!\n", physical_block_nr, calypso_dev->physical_nr_blocks);
                    return 0;
                }
                /* we only need to care about writes because they change the physical blocks state */
                if (bio_data_dir(bio) == WRITE)
                { 
                    atomic64_inc(&trace_data.request_counter_write);
                    debug_args(KERN_INFO , __func__, "Received %lld WRITE requests to /dev/sda8\n", atomic64_read(&trace_data.request_counter_write));
                    debug_args(KERN_INFO , __func__, "Request to sector %lld, block %lld\n", bio->bi_iter.bi_sector, bio->bi_iter.bi_sector / 8);
                    debug_args(KERN_INFO , __func__, "Only inside partition /dev/sda8 the block offset is  %lld\n", (bio->bi_iter.bi_sector / 8) - (calypso_dev->first_physical_sector / 8));
                    // debug_args(KERN_INFO , __func__, "bio->bi_iter.bi_size %d", bio->bi_iter.bi_size);

                    debug_args(KERN_INFO , __func__, "physical_block_nr: %lu\n", physical_block_nr);

                    virtual_block_nr = calypso_dev->physical_to_virtual_block_mapping[physical_block_nr];
                    debug(KERN_INFO , __func__, "WRITING TO AN UNALLOCATED BLOCK\n");
                    debug_args(KERN_INFO , __func__, "virtual_block_nr: %lu\n", virtual_block_nr);
                    
                    /* 
                    * Update bitmap since we had a write 
                    * This should be done before getting next free block,
                    * otherwise it will just pick the same block
                    */
                    // is_set = test_bit(physical_block_nr, calypso_dev->physical_blocks_bitmap);
                    // if (!is_set)
                    // {
                    //     calypso_set_bit(calypso_dev->physical_blocks_bitmap, physical_block_nr);
                    //     debug(KERN_INFO , __func__, "SET BIT AS ALLOCATED\n");
                    // }
                    is_set = test_bit(physical_block_nr, calypso_dev->high_entropy_blocks_bitmap);
                    if (is_set)
                    {
                        calypso_update_bitmaps(calypso_dev->physical_blocks_bitmap, calypso_dev->high_entropy_blocks_bitmap, physical_block_nr);
                        // calypso_clear_bit(calypso_dev->high_entropy_blocks_bitmap, physical_block_nr);
                        debug(KERN_INFO , __func__, "SET BIT AS ALLOCATED\n");
                    }

                    /* 
                     * physical block has corresponding virtual block, so it is in use by Calypso,
                     * thus calypso's data in that block needs to be copied to another block and 
                     * the mappings updated 
                     */
                    if (virtual_block_nr >= 0 && virtual_block_nr < calypso_dev->virtual_nr_blocks)
                    {
                        /* 
                        * To test: 
                        * Cleanup: 
                        *   $ sudo dd if=text3.txt count=1 bs=4096 seek=12157 of=/dev/sda8
                        *   $ sudo dd if=text3.txt count=1 bs=4096 seek=12158 of=/dev/sda8
                        *
                        *   $ sudo dd if=text.txt count=1 bs=4096 seek=100 of=/dev/calypso0
                        * dmesg: block = x 12157 937500
                        *   $ sudo dd if=testing/first_block.txt count=1 bs=4096 seek=12157 of=/dev/sda8
                        * restart VM
                        *   $ sudo dd if=/dev/sda8 count=1 bs=4096 skip=12157
                        *   $ sudo dd if=/dev/sda8 count=1 bs=4096 skip=12158
                        */

                        /* Find next free block to replace this one */
                        unsigned long physical_block_nr_to_move = 0;
                        if (calypso_get_next_free_block(calypso_dev->high_entropy_blocks_bitmap, calypso_dev->physical_nr_blocks, &physical_block_nr_to_move) == -1) {
                        // if (calypso_get_next_free_block(calypso_dev->physical_blocks_bitmap, calypso_dev->physical_nr_blocks, &physical_block_nr_to_move) == -1) {
                            debug(KERN_ERR, __func__, "No more blocks to allocate in physical partition\n");
                            bio_endio(bio);
                            return;
                        }
                        // TODO: see if this makes sense, because it wasn't here before
                        // calypso_clear_bit(calypso_dev->high_entropy_blocks_bitmap, physical_block_nr_to_move);
                        calypso_update_bitmaps(calypso_dev->physical_blocks_bitmap, calypso_dev->high_entropy_blocks_bitmap, physical_block_nr_to_move);
                        debug_args(KERN_INFO, __func__, "MOVING DATA to block %lu\n", physical_block_nr_to_move);
                        debug_args(KERN_INFO, __func__, "COPYING FROM block %lu to block %lu\n", physical_block_nr, physical_block_nr_to_move);

                        /* Move Calypso block to another physical block */
                        // TODO change mappings
                        //calypso_copy_block(calypso_dev->physical_dev, physical_block_nr, physical_block_nr_to_move);   
                        calypso_dev->from_physical_block_pending_copy = physical_block_nr;
                        calypso_dev->to_physical_block_pending_copy = physical_block_nr_to_move;
                        debug(KERN_INFO, __func__, "COPIED DATA\n");

                        calypso_dev->pending_bio = bio;
                        // TODO: WHY 4095 INSTEAD OF 4096???
                        memcpy(calypso_dev->pending_data, bio_data(bio), 4095);
                        //calypso_print_bio_data(bio_data(calypso_dev->pending_bio));

                        new_bio_read_page(calypso_get_sector_nr_from_block(calypso_dev->from_physical_block_pending_copy, 0), calypso_dev->physical_dev, calypso_dev->pending_page, calypso_bio_end_io);
                        //struct bio *read_bio = calypso_clone_bio(bio);
                        //read_bio->bi_opf = REQ_OP_READ;
                        //submit_bio(read_bio);

                        return 0;
                    }        
                    /* Write to this block is not mapped by Calypso, se we don't care about it because it is not going to overwrite Calypso's data */   
                    else {
                        debug(KERN_INFO, __func__, "WE DON'T CARE ABOUT THIS WRITE BECAUSE IT IS NOT MAPPED BY CALYPSO\n");
                    }         
                }
                else
                {
                    atomic64_inc(&trace_data.request_counter_read);
                    debug_args(KERN_INFO , __func__, "Received %lld READ requests to /dev/sda8\n", atomic64_read(&trace_data.request_counter_read));
                    debug_args(KERN_INFO , __func__, "Request to sector %lld, block %lld\n", bio->bi_iter.bi_sector, bio->bi_iter.bi_sector / 8);
                    debug_args(KERN_INFO , __func__, "Only inside partition /dev/sda8 the block offset is  %lld\n", (bio->bi_iter.bi_sector / 8) - (calypso_dev->first_physical_sector / 8));

                    /*if ((bio->bi_iter.bi_sector / 8 - 6250496) == calypso_dev->from_physical_block_pending_copy)
                    {
                        debug(KERN_DEBUG, __func__, "----- **** READING BLOCK THAT NEEDS TO BE COPIED!!!!!\n");
                        calypso_print_bio_data(bio);
                        orig_request_fn(q, calypso_dev->pending_bio);
                    }*/
                }
            }
        }
        // else
        // {
            // if (bio_data_dir(bio) == WRITE)
            // {
            //     debug_args(KERN_INFO , __func__, "REQUEST TO sector %lld, block %lld\n", bio->bi_iter.bi_sector, bio->bi_iter.bi_sector / 8); 
            //     atomic64_inc(&trace_data.request_counter_other);
            //     debug_args(KERN_INFO , __func__, "Received %lld requests to /dev/sda\n", atomic64_read(&trace_data.request_counter_other));
            // }
        // }
    // }
    
    return orig_request_fn(q, bio);
}

static void calypso_hook_physical_make_request_fn(void)
{
    orig_request_fn = bdev_get_queue(calypso_dev->physical_dev)->make_request_fn;
    bdev_get_queue(calypso_dev->physical_dev)->make_request_fn = hooked_physical_make_request_fn;
}	

static void calypso_restore_physical_make_request_fn(void)
{
    bdev_get_queue(calypso_dev->physical_dev)->make_request_fn = orig_request_fn;
}

static void printBioFlagsValues(struct bio *bio)
{
    // bio->bi_opf
    // bio->bi_flags
    // bio->bi_ioprio
    // bio->bi_write_hint
    debug(KERN_DEBUG, __func__, "---------> FLAGS FOR BIO\n");

    debug_args(KERN_DEBUG, __func__, "bio->bi_opf: %u\n", bio->bi_opf);
    debug_args(KERN_DEBUG, __func__, "bio->bi_flags: %u\n", bio->bi_flags);
    debug_args(KERN_DEBUG, __func__, "bio->bi_ioprio: %u\n", bio->bi_ioprio);
    debug_args(KERN_DEBUG, __func__, "bio->bi_write_hint: %u\n", bio->bi_write_hint);

    debug_args(KERN_DEBUG, __func__, "IS BIO REQ_OP_READ: %u\n", bio_op(bio) == REQ_OP_READ);
    debug_args(KERN_DEBUG, __func__, "IS BIO BIO_USER_MAPPED: %u\n", bio_flagged(bio, BIO_USER_MAPPED));
}

/* 
 * Actual remapping of I/O requests
 */
static void calypso_remap_io_request_to_physical_dev(struct bio *bio)
{
    /* This is where we know to which Calypso virtual block
    the request was done */
    sector_t req_start_sector = bio->bi_iter.bi_sector;
    unsigned long virtual_block_nr = req_start_sector / 8;
    sector_t physical_sector_nr;

    /* 
     * We need to find if this virtual block is already mapped to a 
     * physical block
     */
    unsigned long physical_block_nr = calypso_dev->virtual_to_physical_block_mapping[virtual_block_nr];
    unsigned long next_free_physical_block_nr = 0;

    /* There may be requests that fulfill an entire block and requests for only a part of the block */
    unsigned int req_blocks_count_complete = bio->bi_iter.bi_size / 4096;
    unsigned int req_blocks_count_incomplete = bio->bi_iter.bi_size % 4096;
    size_t i = 0;

    // debug(KERN_INFO, __func__, "----------- BEGINNING OF REQUEST TO CALYPSO -----------\n");
    // printBioFlagsValues(bio);
    // debug_args(KERN_INFO, __func__, "Remapping requests from calypso device's block %lu to %s\n", virtual_block_nr, PHYSICAL_DISK_NAME);

    // debug_args(KERN_INFO , __func__, "bio->bi_iter.bi_size %d, size in blocks %d \n", bio->bi_iter.bi_size, req_blocks_count_complete);
    // debug_args(KERN_INFO , __func__, "req_start_sector %llu\n", req_start_sector);

    /* This needs to happen in every case */
    bio_set_dev(bio, calypso_dev->physical_dev);

    /* Set REQ_CALYPSO flag so that sda knows this request came from Calypso */
    bio->bi_opf |= REQ_CALYPSO;

    /* 
     * Since requests might refer to more than one block, we need a loop 
     * to make these actions for each block in the request 
     */
    do {
    //for (i = 0; i < req_blocks_count_complete; i++) {
        // debug_args(KERN_INFO , __func__, "*** Block %ld of the request, >>> BLOCK nr %ld\n", i);
        /* virtual block is mapped to physical block */
        if (physical_block_nr >= 0 && physical_block_nr < calypso_dev->physical_nr_blocks) 
        {
            debug_args(KERN_INFO, __func__, "BLOCK IS MAPPED to %lu\n", physical_block_nr);

            /* First, we change the sector associated with the bio */
            physical_sector_nr = calypso_get_sector_nr_from_block(physical_block_nr,  bio_offset(bio));
            calypso_make_single_block_request(bio, i, req_blocks_count_complete, req_blocks_count_incomplete, physical_sector_nr);
        }
        /* Reads are treated the same way as writes */
        else {
            /* 
            * If the block is mapped, we already issued the operation to that 
            * physical block, if no, we need to find the next free 
            * physical block and update the mapping to this virtual block
            */
            debug(KERN_INFO, __func__, "BLOCK IS NOT MAPPED\n");

            /* 
             * Find next free block
             * next_free_physical_block_nr can start with the previous value so that 
             * we don't need to start from the beginning of the bitmap all over again 
             */
            if (calypso_get_next_free_block(calypso_dev->high_entropy_blocks_bitmap, calypso_dev->physical_nr_blocks, &next_free_physical_block_nr) == -1) {
            // if (calypso_get_next_free_block(calypso_dev->physical_blocks_bitmap, calypso_dev->physical_nr_blocks, &next_free_physical_block_nr) == -1) {
                debug(KERN_ERR, __func__, "No more blocks to allocate in physical partition\n");
                bio_endio(bio);
                return;
            }  
            debug_args(KERN_INFO, __func__, "remapping to NEXT FREE BLOCK %lu\n", next_free_physical_block_nr);

            /* We need to set that block as allocated so that we don't assign same free block more than once */
            calypso_update_bitmaps(calypso_dev->physical_blocks_bitmap, calypso_dev->high_entropy_blocks_bitmap, next_free_physical_block_nr);
            // calypso_clear_bit(calypso_dev->high_entropy_blocks_bitmap, next_free_physical_block_nr);
            // calypso_set_bit(calypso_dev->physical_blocks_bitmap, next_free_physical_block_nr);
            
            /* Update mapping */
            // ***** TODO which one is right?????
            calypso_update_mappings(virtual_block_nr, next_free_physical_block_nr);
            //calypso_update_mappings(virtual_block_nr+i, next_free_physical_block_nr);

            physical_sector_nr = calypso_get_sector_nr_from_block(next_free_physical_block_nr, bio_offset(bio));
            //calypso_update_bio_sector(&(bio->bi_iter), physical_sector_nr); 

            if (bio_data_dir(bio) == WRITE)
            {
                /* To test: $ sudo dd if=text.txt count=1 bs=4096 seek=100 of=/dev/calypso0 */
                debug(KERN_INFO, __func__, "REMAPPING WRITE\n");
                //calypso_create_bio_write_req(calypso_dev->physical_dev, physical_sector_nr, 4096, bio);
            }
            else
            {
                /* To test: $ sudo dd if=/dev/calypso0 count=1 bs=4096 skip=200 */
                debug(KERN_INFO, __func__, "REMAPPING READ\n");
                // calypso_create_bio_read_req2(calypso_dev->physical_dev, physical_sector_nr, 4096, bio);
            }
            calypso_make_single_block_request(bio, i, req_blocks_count_complete, req_blocks_count_incomplete, physical_sector_nr);
        }
        /* Advance to the next block of the request */
        virtual_block_nr++;
        physical_block_nr = calypso_dev->virtual_to_physical_block_mapping[virtual_block_nr];
        i++;
    } while (i < req_blocks_count_complete);
    //bio_set_dev(bio, calypso_dev->physical_dev);
    //direct_make_request(bio);
    //orig_request_fn(bdev_get_queue(calypso_dev->physical_dev), bio);
    //return orig_request_fn(bdev_get_queue(calypso_dev->physical_dev), bio);

    /* Then, we send the bio's request to the underlying native device */
    /*bio_set_dev(bio, calypso_dev->physical_dev);*/
    //bio->bi_private = &(calypso_dev->major);

    /*trace_block_bio_remap(bdev_get_queue(calypso_dev->physical_dev), 
                        bio,
                        bio_dev(bio), 
                        req_start_sector);*/

    /*return generic_make_request(bio);*/
}

/*
 * Handle an I/O request
 */
static blk_qc_t calypso_make_request(struct request_queue *q, struct bio *bio)
{
    // struct request *req = q->last_merge;

	// debug(KERN_INFO, __func__, "Processing requests...");
    
	// return calypso_remap_io_request_to_physical_dev(req, bio);
    calypso_remap_io_request_to_physical_dev(bio);
    return 0;
}

// TODO: make these call the original functions
static int calypso_dev_open(struct block_device *bdev, fmode_t mode)
{
    debug(KERN_INFO, __func__, "Device is opened\n");

    // struct rb_device *dev = bdev->bd_disk->private_data;
    
    // spin_lock(&dev->lock);
    // dev->users++;
    // spin_unlock(&dev->lock);

    return 0;
}

static void calypso_dev_close(struct gendisk *disk, fmode_t mode)
{
    // struct sbull_dev *dev = disk->private_data;

    // spin_lock(&dev->lock);
	// dev->users--;
    // spin_unlock(&dev->lock);

    debug(KERN_INFO, __func__, "Device is closed\n");
}

int calypso_dev_ioctl(struct block_device *bdev, fmode_t mode, 
						unsigned cmd, unsigned long arg)
{
    debug_args(KERN_INFO, __func__, "ioctl call, cmd: %d\n", cmd);

	switch(cmd) {
		/* retrieve disk geometry */
	    case HDIO_GETGEO:
        /*
		 * Get geometry: since we are a virtual device, we have to make
		 * up something plausible. So we claim 16 sectors, four heads,
		 * and calculate the corresponding number of cylinders.  We set the
		 * start of data at sector four.
		 */
            debug(KERN_INFO, __func__, "HDIO_GETGEO\n");
            break;

		default:
			debug(KERN_INFO, __func__, "default\n");

	}
    return -ENOTTY;
}
 
/* 
 * This is the registration and initialization section of the block device
 * driver
 */
 static int __init calypso_init(void)
{
    int ret = 0;
	
	debug_args(KERN_INFO, __func__, "Initializing Calypso...\n");

	ret = calypso_dev_init(&calypso_dev);
	if (ret != 0)
    {
        debug(KERN_ERR, __func__, "Unable to initialize Calypso\n");
		kfree(calypso_dev);
        return ret;
    } else {
		debug(KERN_INFO, __func__, "Calypso driver is initialized\n");
	}

    /* Get the capacity given as input to our Calypso device */
    if (blocks % BLOCKS_IN_A_GB == 0) {
    /** 
     * For some reason, formatting with a file system when the blocks are multiples of a Gb
     * blocks the system, I suspect due to not having space left besides the exact blocks or 
     * some count that was not done correctly when defining the capacity of the block device
     * This is a temporary fix
     */
        blocks++;
    }
    calypso_dev->virtual_nr_blocks = blocks; // capacity in blocks
    calypso_dev->capacity = blocks * 8; // capacity in sectors

    //ret = calypso_dev_setup(calypso_dev, &calypso_fops, &calypso_fops_req);
	ret = calypso_dev_no_queue_setup(calypso_dev, &calypso_fops, CALYPSO_DEV_NAME, &(calypso_dev->major), calypso_make_request);
    if (ret != 0)
    {
        debug(KERN_ERR, __func__, "Unable to set up Calypso\n");
		goto error_after_bdev;
    } else {
		debug(KERN_INFO, __func__, "Calypso driver is set up\n");
	}

	//physical_dev = calypso_open_physical_disk(PHYSICAL_DISK_NAME, calypso_dev);
	calypso_dev->physical_dev = calypso_open_physical_disk(PHYSICAL_DISK_NAME, calypso_dev);
	if (!calypso_dev->physical_dev) {
		debug_args(KERN_ERR, __func__, "Unable to open physical disk %s\n", PHYSICAL_DISK_NAME);
		return -ENOENT;
	}
	else
		debug_args(KERN_INFO, __func__, "Opened physical disk %s\n", PHYSICAL_DISK_NAME);

    // TODO I think the blocking here is solved 
    calypso_set_physical_partition_sector_ranges(calypso_dev);
    debug(KERN_INFO, __func__, "calypso_set_physical_partition_sector_ranges\n");
    // TODO sometimes it blocks here, before finishing this function, right after begin, during get_super()!!!!!!!
    calypso_get_super_block_physical_dev(calypso_dev);
    calypso_dev->physical_nr_blocks = calypso_get_physical_block_count(calypso_dev);
    debug(KERN_INFO, __func__, "After get physical block count\n");

    /* Determine metadata sizes */
    calypso_dev->bitmap_data_len = calypso_calc_bitmap_metadata_size(calypso_dev->physical_nr_blocks);
    calypso_dev->mappings_data_len = calypso_calc_mappings_metadata_size(calypso_dev->virtual_nr_blocks);
    calypso_dev->metadata_nr_blocks = calypso_calc_metadata_size_in_blocks(calypso_dev->bitmap_data_len, calypso_dev->mappings_data_len);
    debug_args(KERN_INFO, __func__, "calypso_dev->metadata_nr_blocks: %lu\n", calypso_dev->metadata_nr_blocks);

    ret = calypso_dev_init_bitmaps(calypso_dev);
    if (ret != 0)
		goto error_after_bdev;
    debug(KERN_INFO, __func__, "After bitmaps\n");

    ret = calypso_dev_init_mappings(calypso_dev);
    if (ret != 0)
		goto error_after_bitmaps;
    debug(KERN_INFO, __func__, "After mappings\n");

    /* 
     * We should always retrieve the bitmap to check the changes that may have happened during the time
     * Calypso was not active. Then, if a block was being used by calypso and was free, if it is occupied 
     * when Calypso is realoaded, then maybe it needs to be recovered with error correcting codes and 
     * copied into another free block
     */
    calypso_get_fs_blocks_bitmap(calypso_dev);
    debug(KERN_INFO, __func__, "After get_fs_blocks_bitmap\n");

    // /*
    //  * After bitmap and mapping data structures are initialized, we can check
    //  * if we have metadata to retrieve
    //  */
    // calypso_retrieve_metadata(calypso_dev);

    // TODO this needs to be here for me to know the sectors, or determine the sectors as fast as possible
    // but the teacher said for us to start intercepting requests before getting the free blocks
    // calypso_hook_physical_make_request_fn();
    // debug(KERN_INFO, __func__, "After make_request_fn\n");

    calypso_dev->from_physical_block_pending_copy = -1;
    calypso_dev->to_physical_block_pending_copy = -1;
    // TODO check if successful
    calypso_dev->pending_data = kmalloc(4096, GFP_KERNEL);

    debug(KERN_INFO, __func__, "------------ CRYPTO_PART ------------\n");
    debug_args(KERN_INFO, __func__, "***** bitmap_data_len: %lu, mappings_data_len: %lu, metadata_nr_blocks: %lu\n", calypso_dev->bitmap_data_len, calypso_dev->mappings_data_len, calypso_dev->metadata_nr_blocks);
    // sudo dd if=hello.txt bs=4096 seek=36 count=1 of=/dev/calypso0
    // sudo dd if=/dev/calypso0 bs=4096 skip=36 count=1

    // sudo dd if=/dev/sda8 count=1 bs=4096 skip=606403

    if (!is_clean_start)
    {
        // debug_args(KERN_INFO, __func__, "virtual blocks before: %lu\n", calypso_dev->virtual_nr_blocks);
        // calypso_retrieve_hidden_metadata(calypso_dev->bitmap_data_len, calypso_dev->mappings_data_len, calypso_dev->metadata_to_physical_block_mapping, calypso_dev->metadata_nr_blocks, calypso_dev->physical_dev, calypso_dev->physical_blocks_bitmap, calypso_dev->high_entropy_blocks_bitmap, calypso_dev->physical_nr_blocks, calypso_dev->sym_enc_tfm, calypso_dev->cipher, calypso_dev->virtual_nr_blocks, calypso_dev->virtual_to_physical_block_mapping, calypso_dev->physical_to_virtual_block_mapping);
        calypso_retrieve_hidden_metadata(calypso_dev->bitmap_data_len, calypso_dev->mappings_data_len, calypso_dev->metadata_to_physical_block_mapping, calypso_dev->metadata_nr_blocks, calypso_dev->physical_dev, calypso_dev->physical_blocks_bitmap, calypso_dev->high_entropy_blocks_bitmap, calypso_dev->physical_nr_blocks, calypso_dev->sym_enc_tfm, calypso_dev->cipher, calypso_dev->virtual_nr_blocks, &(calypso_dev->virtual_nr_blocks), calypso_dev->virtual_to_physical_block_mapping, calypso_dev->physical_to_virtual_block_mapping);
        debug_args(KERN_INFO, __func__, "virtual blocks after: %lu\n", calypso_dev->virtual_nr_blocks);
    }
    // else {
    // TODO: see if it should be done every time and if it should be done after calypso_retrieve_hidden_metadata
    calypso_dev->free_high_entropy_blocks = classify_free_blocks_entropy(calypso_dev->physical_blocks_bitmap, calypso_dev->high_entropy_blocks_bitmap, calypso_dev->physical_nr_blocks);
    debug_args(KERN_INFO, __func__, "free_high_entropy_blocks: %lu\n", calypso_dev->free_high_entropy_blocks);
    // }
    // TODO: CHANGE THIS TO BEFORE??
    calypso_hook_physical_make_request_fn();

    return ret;

error_after_bitmaps:
    calypso_dev_cleanup_bitmaps(calypso_dev);
error_after_bdev:
    calypso_dev_physical_cleanup(calypso_dev, CALYPSO_DEV_NAME, &(calypso_dev->major));
    return ret;
}


/*
 * This is the unregistration and uninitialization section of the ram block
 * device driver
 */
static void __exit calypso_cleanup(void)
{
    calypso_encode_hidden_metadata(calypso_dev->bitmap_data_len, calypso_dev->mappings_data_len, calypso_dev->metadata_to_physical_block_mapping, calypso_dev->metadata_nr_blocks, calypso_dev->physical_dev, calypso_dev->physical_blocks_bitmap, calypso_dev->high_entropy_blocks_bitmap, calypso_dev->physical_nr_blocks, calypso_dev->sym_enc_tfm, calypso_dev->cipher, calypso_dev->virtual_nr_blocks, calypso_dev->virtual_to_physical_block_mapping);

	calypso_restore_physical_make_request_fn();

    /* Free cryptograpic info */
    crypto_free_shash(calypso_dev->sym_enc_tfm);
    calypso_cleanup_block_encryption_key(calypso_dev->cipher);

    // TODO put this in cleanup function
    kfree(calypso_dev->pending_data);

    // calypso_persist_metadata(calypso_dev);

    calypso_dev_cleanup_bitmaps(calypso_dev);
    calypso_dev_cleanup_mappings(calypso_dev);

	calypso_dev_physical_cleanup(calypso_dev, CALYPSO_DEV_NAME, &(calypso_dev->major));

    debug(KERN_DEBUG, __func__, "Calypso driver cleanup");
}
 
module_init(calypso_init);
module_exit(calypso_cleanup);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Daniela Lopes");
MODULE_DESCRIPTION("Calypso Driver");