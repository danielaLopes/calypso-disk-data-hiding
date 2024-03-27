#ifndef FILE_IO_H
#define FILE_IO_H


struct file *calypso_open_file(const char *path, int flags, int rights);

void calypso_close_file(struct file *file);

void calypso_set_file_pos_to_end(struct file *calypso_file);

void calypso_set_file_pos(struct file *calypso_file, loff_t offset_bytes);

loff_t calypso_get_file_size(struct file *calypso_file);

int calypso_check_metadata_file(struct file* file);

ssize_t calypso_read_segment_from_file_old(struct file *calypso_file, 
                                loff_t offset_bytes, 
                                size_t len_bytes,
                                unsigned char *req_buf);

ssize_t calypso_read_segment_from_file(struct file *calypso_file, 
                            loff_t offset_bytes, 
                            size_t len_bytes,
                            unsigned char *req_buf);

// ssize_t calypso_write_segment_to_file_old(struct file *calypso_file, 
//                                 loff_t offset_bytes, 
//                                 size_t len_bytes,
//                                 unsigned char *req_buf);

ssize_t calypso_write_segment_to_file(struct file *calypso_file, 
                            loff_t offset_bytes, 
                            size_t len_bytes,
                            unsigned char *req_buf);

size_t calypso_read_file_with_offset(struct file *file, loff_t offset, unsigned char *data, size_t size);
size_t calypso_write_file_with_offset(struct file *file, loff_t offset, unsigned char *data, size_t count);

size_t calypso_read_file(struct file *file, unsigned char *data, size_t count);

size_t calypso_write_file(struct file *file, unsigned char *data, size_t count);

//int calypso_file_size(struct file *file);

int calypso_transfer_to_file(struct request *req);

void calypso_use_file_example(void);


#endif