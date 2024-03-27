#ifndef PERSISTENCE_H
#define PERSISTENCE_H

#include "virtual_device.h"

/* Files in the native file system to access to retrieve metadata */
#define CALYPSO_MAPPINGS_FILE_PATH "/home/daniela/vboxshare/thesis/calypso/drivers/calypso_driver/calypso_mappings.dat"
#define CALYPSO_BITMAP_FILE_PATH "/home/daniela/vboxshare/thesis/calypso/drivers/calypso_driver/calypso_bitmap.dat"


void calypso_write_mappings_to_file(struct file *metadata_file, struct calypso_blk_device *calypso_dev);

void calypso_read_mappings(struct file *metadata_file, struct calypso_blk_device *calypso_dev);

void calypso_write_bitmap_to_file(struct file *metadata_file, struct calypso_blk_device *calypso_dev);

int calypso_read_bitmap(char *bitmap_data, loff_t bitmap_len, struct calypso_blk_device *calypso_dev);

void calypso_persist_bitmap(struct calypso_blk_device *calypso_dev);

void calypso_persist_mappings(struct calypso_blk_device *calypso_dev);

void calypso_retrieve_mappings(struct calypso_blk_device *calypso_dev);

void calypso_retrieve_bitmap(struct calypso_blk_device *calypso_dev);

void calypso_persist_metadata(struct calypso_blk_device *calypso_dev);

void calypso_retrieve_metadata(struct calypso_blk_device *calypso_dev);


#endif