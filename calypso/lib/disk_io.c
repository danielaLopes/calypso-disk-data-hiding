/*
 *
 */

#include <linux/kernel.h>
#include <linux/string.h>
#include <linux/blkdev.h>
#include <linux/genhd.h>
#include <linux/vmalloc.h>

#include "debug.h"
#include "disk_io.h"
#include "physical_device.h"


void calypso_read_segment_from_disk(sector_t sector_offset, unsigned int sectors,
                            u8 *req_buf)
{
	
}

void calypso_write_segment_to_disk(sector_t *next_sector_to_write, unsigned int sectors, 
                            u8 *req_buf)
{

}

void write_to_output_buffer(sector_t sector_off, unsigned int sectors, 
							char *read_data_buf, u8 *data_to_output)
{
	memcpy(data_to_output + sector_off * DISK_SECTOR_SIZE, read_data_buf,
		sectors * DISK_SECTOR_SIZE);
	debug(KERN_INFO, __func__, "Finished writing read data to output buffer\n")
}

// TODO: should we just open and close the disk in read or write or keep it opened
//void read_from_disk(struct block_device *bdev)
void calypso_read_from_disk(sector_t sector_offset, unsigned int sectors,
					u8 *req_buf)
{
	struct block_device * bdev;
    struct bio *bio = bio_alloc(GFP_NOIO, 1);
	struct page *page;
	char *disk_buf;
	
	unsigned long bytes_offset = sectors * KERNEL_SECTOR_SIZE;
	//unsigned long n_bytes = sectors * KERNEL_SECTOR_SIZE;
	// TODO: change this
	//char chars_to_output[8];

	debug(KERN_INFO, __func__, "Init\n");

	bdev = calypso_open_disk_simple(PHYSICAL_DISK_NAME);

	/* fill bio (bdev, sector, direction) */
	bio->bi_disk = bdev->bd_disk;
    /* the first sector of the disk is the sector with index 0 */
	//bio->bi_iter.bi_sector = 0;
	//bio->bi_iter.bi_sector = 900;
	bio->bi_iter.bi_sector = sector_offset;
	/* is going to be updated with either REQ_OP_READ or REQ_OP_WRITE */
	bio->bi_opf = REQ_OP_READ;

    /* allocate a page to add to the bio */
	page = alloc_page(GFP_NOIO); 
	bio_add_page(bio, page, KERNEL_SECTOR_SIZE, 0);

	/* submit bio and wait for completion */
	submit_bio_wait(bio);

	/* read data (first 3 bytes) from bio buffer and print it */
	disk_buf = kmap_atomic(page);
	//debug_args(KERN_INFO, "[read_from_disk] read %02x %02x %02x\n", read_data_buf[0], read_data_buf[1], read_data_buf[2]);
	debug_args(KERN_INFO, __func__, "Needs to read %ld chars\n", sizeof(disk_buf));
	//write_to_output_buffer(sector_off, sectors, read_data_buf, buffer_to_output);
	debug_args(KERN_DEBUG, __func__, "Reading \"%s\" to %s", disk_buf + bytes_offset, PHYSICAL_DISK_NAME);
	memcpy(req_buf, disk_buf + bytes_offset/*, n_bytes*/, 8);
	debug_args(KERN_DEBUG, __func__, "Read \"%s\" to %s", req_buf, PHYSICAL_DISK_NAME);
	//memcpy(chars_to_output, read_data_buf, sizeof(chars_to_output));
	kunmap_atomic(disk_buf); 

	bio_put(bio);
	__free_page(page);

	debug(KERN_INFO, __func__, "Finished reading segment from disk\n");

	calypso_close_disk_simple(bdev);
}

//void write_to_disk(struct block_device *bdev, sector_t *next_sector_to_write, unsigned int sectors, u8 *data_to_write)
void calypso_write_to_disk(sector_t *next_sector_to_write, unsigned int sectors, u8 *req_buf)
{
	struct block_device * bdev;

	struct bio *bio = bio_alloc(GFP_NOIO, 1);
	struct page *page;
	char *disk_buf;

	unsigned long bytes_offset = sectors * KERNEL_SECTOR_SIZE;
	//unsigned long n_bytes = next_sector_to_write * KERNEL_SECTOR_SIZE;

	debug(KERN_INFO, __func__, "Init\n");
	debug_args(KERN_INFO, __func__, "Writing: %s\n", req_buf);

	bdev = calypso_open_disk_simple(PHYSICAL_DISK_NAME);

	/* fill bio (bdev, sector, direction) */
	bio->bi_disk = bdev->bd_disk;
    /* the first sector of the disk is the sector with index 0 */
	//bio->bi_iter.bi_sector = START_SECTOR; 
	//bio->bi_iter.bi_sector = *next_sector_to_write; 
	bio->bi_iter.bi_sector = 900; 
	//(*next_sector_to_write) += sectors;
    /* is going to be updated with either REQ_OP_READ or REQ_OP_WRITE */
	bio->bi_opf = REQ_OP_WRITE;

	debug(KERN_INFO, __func__, "Before page_alloc\n");
    /* allocate a page to add to the bio */
	page = alloc_page(GFP_NOIO); 
	bio_add_page(bio, page, KERNEL_SECTOR_SIZE, 0);
	debug(KERN_INFO, __func__, "After bio_add_page\n");

	/* write message to bio buffer */
	/* short duration mapping of single page with global 
	synchronization */
	disk_buf = kmap_atomic(page);
	/* add contents of data_to_write in the request to the page */
	// TODO change size in memcpy
	debug_args(KERN_DEBUG, __func__, "Writing \"%s\" to %s", disk_buf, PHYSICAL_DISK_NAME);
	memcpy(disk_buf + bytes_offset, req_buf/*, n_bytes*/, 40);
	debug_args(KERN_DEBUG, __func__, "Wrote \"%s\" to %s", disk_buf + bytes_offset, PHYSICAL_DISK_NAME);
	/* unmap the page */
	kunmap_atomic(disk_buf);

	/* submit bio and wait for completion */
	submit_bio_wait(bio);
	debug(KERN_INFO, __func__, "Done bio\n");

	bio_put(bio);
	__free_page(page);
	debug(KERN_INFO, __func__, "After __free_page\n");

	calypso_close_disk_simple(bdev);
}