#ifndef RAM_IO_H
#define RAM_IO_H

#include <linux/types.h>


struct ram_device {
    char *data;
};


int calypso_init_ram(struct ram_device **ram_dev, sector_t n_sectors);

void calypso_free_ram(struct ram_device *ram_dev);

void calypso_write_segment_to_ram(char *ram_buf, 
							unsigned long offset_bytes,
							size_t len_bytes, 
                    		u8 *req_buf);

void calypso_read_segment_from_ram(char *ram_buf,
							unsigned long offset_bytes, 
							size_t len_bytes,
                    		u8 *req_buf);


#endif