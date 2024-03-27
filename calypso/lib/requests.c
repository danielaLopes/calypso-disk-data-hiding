#include "debug.h"

#include "requests.h"


void new_bio_read_page(sector_t sector, struct block_device *physical_dev, struct page *pending_page, void (*bio_end_io_func)(struct bio *))
{
    struct bio *bio = bio_alloc(GFP_NOIO, 1);
    struct page *page = alloc_page(GFP_KERNEL);
    pending_page = page;
    debug(KERN_DEBUG, __func__, "------ calypso is issuing a read_page\n");
    bio_set_dev(bio, physical_dev);
    bio->bi_iter.bi_sector = sector;
    bio->bi_opf = REQ_OP_READ;
    bio->bi_opf |= REQ_CALYPSO;
    bio_add_page(bio, page, 4096, 0);

    /* so that we can copied read data into next free block */
    bio->bi_end_io = bio_end_io_func;

    submit_bio(bio);
}

void new_bio_write_page(void *data, struct block_device *physical_dev, sector_t sector)
{
    struct bio *bio = bio_alloc(GFP_NOIO, 1);
    struct page *page = alloc_page(GFP_KERNEL);
    // debug(KERN_DEBUG, __func__, "------ calypso is issuing a write_page\n");
    bio_set_dev(bio, physical_dev);
    bio->bi_iter.bi_sector = sector;
    bio->bi_opf = REQ_OP_WRITE;
    bio->bi_opf |= REQ_CALYPSO;
    bio_add_page(bio, page, 4096, 0);

    void *buffer = bio_data(bio);
    memcpy(buffer, data, 4096);

    submit_bio(bio);
}

