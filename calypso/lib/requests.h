#ifndef REQUESTS_H
#define REQUESTS_H

#include "virtual_device.h"


void new_bio_read_page(sector_t sector, struct block_device *physical_dev, struct page *pending_page, void (*bio_end_io_func)(struct bio *));

void new_bio_write_page(void *data, struct block_device *physical_dev, sector_t sector);


#endif