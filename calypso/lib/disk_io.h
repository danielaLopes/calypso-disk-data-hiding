#ifndef DISK_IO_H
#define DISK_IO_H

#include <linux/types.h>


void calypso_read_segment_from_disk(sector_t sector_offset, unsigned int sectors,
                            u8 *req_buf);

void calypso_write_segment_to_disk(sector_t *next_sector_to_write, unsigned int sectors, 
                            u8 *req_buf);


// void block_transfer(struct block_device *dev, char *req_data, 
//                             char *disk_data, sector_t sector,
// 		                    unsigned long len, int dir);
// void my_xfer_request(struct request *req);


void calypso_write_to_output_buffer(sector_t sector_off, unsigned int sectors, 
                            char *read_data_buf, u8 *data_to_output);

//void read_from_disk(struct block_device *bdev);
void calypso_read_from_disk(sector_t sector_offset, unsigned int sectors,
                    u8 *req_buf);

//void write_to_disk(struct block_device *bdev, sector_t *next_sector_to_write, unsigned int sectors, u8 *data_to_write);
void calypso_write_to_disk(sector_t *next_sector_to_write, unsigned int sectors, 
                    u8 *req_buf);

#endif