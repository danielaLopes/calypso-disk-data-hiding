#ifndef RAMDEVICE_H
#define RAMDEVICE_H

#define HW_SECTOR_SIZE 512 /* hardware sector size */
#define RB_SECTOR_SIZE 512
 
extern int ramdevice_init(void);
extern void ramdevice_cleanup(void);
// extern void ramdevice_write(unsigned long offset_bytes, u8 *buffer, unsigned long n_bytes);
// extern void ramdevice_read(unsigned long offset_bytes, u8 *buffer, unsigned long n_bytes);
extern void ramdevice_write(sector_t sector_off, u8 *buffer, unsigned int sectors);
extern void ramdevice_read(sector_t sector_off, u8 *buffer, unsigned int sectors);
#endif