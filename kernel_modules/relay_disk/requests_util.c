/*
 * Processing requests from the request queue at 
 * struct bio level
 * 
 * Processes all struct bio_vec (segments) from all 
 * struct bio structures
 */

#define KERNEL_SECTOR_SIZE	512

static void my_xfer_request(struct my_block_dev *dev, struct request *req)
{
	/* TODO 6/10: iterate segments */
	struct bio_vec bvec;
	struct req_iterator iter;

    /* 
     * Iterate through the bio_vec structures of 
     * each struct bio from the request
     */
	rq_for_each_segment(bvec, req, iter) {
        /* get current sector */
		sector_t sector = iter.iter.bi_sector;
		unsigned long offset = bvec.bv_offset;
		size_t len = bvec.bv_len;
        /* find if it's a read or a write */
		int dir = bio_data_dir(iter.bio);
		char *buffer = kmap_atomic(bvec.bv_page);
		printk(KERN_LOG_LEVEL "%s: buf %8p offset %lu len %u dir %d\n", __func__, buffer, offset, len, dir);

		/* TODO 6/3: copy bio data to device buffer */
		my_block_transfer(dev, sector, len, buffer + offset, dir);
		kunmap_atomic(buffer);
	}
}

static void my_block_transfer(struct my_block_dev *dev, sector_t sector,
		unsigned long len, char *buffer, int dir)
{
	unsigned long offset = sector * KERNEL_SECTOR_SIZE;

	/* check for read/write beyond end of block device */
	if ((offset + len) > dev->size)
		return;

	/* TODO 3/4: read/write to dev buffer depending on dir */
	if (dir == 1)		/* write */
		// copy from buffer to dev->data
		memcpy(dev->data + offset, buffer, len);
	else /* read */
		// copy from dev->data to buffer
		memcpy(buffer, dev->data + offset, len);
}