#include <linux/fs.h>
#include <linux/blk-mq.h>
#include <linux/blkdev.h>

#include "debug.h"
#include "file_io.h"

struct file *calypso_open_file(const char *path, int flags, int rights) 
{
    struct file *filp = NULL;
    int err = 0;

    filp = filp_open(path, flags, rights);

    if (IS_ERR(filp)) {
        err = PTR_ERR(filp);
        return NULL;
    }
    return filp;
}

void calypso_close_file(struct file *file) 
{
    filp_close(file, NULL);
}

void calypso_set_file_pos_to_end(struct file *calypso_file) 
{
	vfs_llseek(calypso_file, 0L, SEEK_END);
}

loff_t calypso_get_file_size(struct file *calypso_file) 
{
	loff_t size = vfs_llseek(calypso_file, 0L, SEEK_END);
	loff_t pos = calypso_file->f_pos;
	// place pos back to the current
	vfs_llseek(calypso_file, pos, SEEK_SET);
	return size;
}

int calypso_check_metadata_file(struct file* file)
{
    /* go to the end of the file */
    loff_t size = vfs_llseek(file, 0L, SEEK_END);
    /* go back to the beginning of the file */
    vfs_llseek(file, 0L, SEEK_SET);

    if (size == 0) {
        debug(KERN_DEBUG, __func__, "Calypso metadata file is empty\n");
        return -1;
    }
    return 0;
}

void calypso_set_file_pos(struct file *calypso_file, loff_t offset_bytes) 
{
	vfs_llseek(calypso_file, offset_bytes, SEEK_SET);
}

/*
 * Random read from file, using older kernel file read
 * methods
 */
// size_t calypso_read_segment_from_file_old(struct file *calypso_file, 
// 								loff_t offset_bytes, 
// 								size_t len_bytes,
// 								unsigned char *req_buf)
// {
// 	size_t res;

// 	loff_t pos;

// 	char __user *data;
// 	mm_segment_t oldfs;

// 	debug(KERN_DEBUG, __func__, "Reading from file...");

// 	debug_args(KERN_DEBUG, __func__, "len_bytes: %ld", len_bytes);
// 	debug_args(KERN_DEBUG, __func__, "File position before reading: %lld", calypso_file->f_pos);
// 	debug_args(KERN_DEBUG, __func__, "offset_bytes before read: %lld", offset_bytes);

// 	data = (__force char __user *)req_buf;
// 	oldfs = get_fs(); /* store current user-space memory segment */
// 	set_fs(KERNEL_DS); /* set user-space memory segment equal to kernel's one */

// 	set_file_pos(calypso_file, offset_bytes);
// 	pos = calypso_file->f_pos;
// 	debug_args(KERN_DEBUG, __func__, "File position after seek: %lld", calypso_file->f_pos);

//     res = vfs_read(calypso_file, req_buf, len_bytes, &pos);
	
// 	set_fs(oldfs); /* restore user-space memory segment after reading */

// 	debug_args(KERN_DEBUG, __func__, "res: %ld", res);
// 	if (res > 0)
// 		calypso_file->f_pos = pos;

// 	debug_args(KERN_DEBUG, __func__, "File position after reading: %lld", calypso_file->f_pos);
// 	debug_args(KERN_DEBUG, __func__, "offset_bytes after read: %lld", offset_bytes);

// 	return res;
// }

/*
 * Random read from file
 */
ssize_t calypso_read_segment_from_file(struct file *calypso_file, 
							loff_t offset_bytes, 
							size_t len_bytes,
							unsigned char *req_buf)
{
	ssize_t res;

	loff_t pos;
	
	//char *data = kzalloc(4096 * sizeof(unsigned char), GFP_KERNEL);
	//copy_from_user(data, req_buf, 4096);
	//char *data = kzalloc(4096 * sizeof(unsigned char), GFP_KERNEL);

	debug(KERN_DEBUG, __func__, "Reading from file...");

	debug_args(KERN_DEBUG, __func__, "len_bytes: %ld", len_bytes);
	debug_args(KERN_DEBUG, __func__, "File position before reading: %lld", calypso_file->f_pos);
	debug_args(KERN_DEBUG, __func__, "offset_bytes before read: %lld", offset_bytes);

	calypso_set_file_pos(calypso_file, offset_bytes);
	pos = calypso_file->f_pos;
	debug_args(KERN_DEBUG, __func__, "File position after seek: %lld", calypso_file->f_pos);

	if (req_buf) {
		debug_args(KERN_DEBUG, __func__, "req_buf %p", req_buf);
	}
	else debug(KERN_DEBUG, __func__, "req_buf does not exist");
	
	debug_args(KERN_DEBUG, __func__, "calypso_file %p", calypso_file);

	debug_args(KERN_DEBUG, __func__, "file->f_op->read: %p", calypso_file->f_op->read);
	debug_args(KERN_DEBUG, __func__, "file->f_op->read_iter: %p", calypso_file->f_op->read_iter);

	//res = calypso_read(calypso_file, req_buf, len_bytes, &pos);
	//res = calypso_kernel_read(calypso_file, req_buf, len_bytes, &pos);
	res = kernel_read(calypso_file, req_buf, len_bytes, &pos);
	debug_args(KERN_DEBUG, __func__, "res: %ld", res);
	if (res > 0)
	 	calypso_file->f_pos = pos;

	debug_args(KERN_DEBUG, __func__, "File position after reading: %lld", calypso_file->f_pos);
	debug_args(KERN_DEBUG, __func__, "offset_bytes after read: %lld", offset_bytes);
	// if (res == len_bytes) 
	// {
    //	debug_args(KERN_DEBUG, __func__, "Read \"%s\" from %s", req_buf, CALYPSO_FILE_PATH);
	// }
	// else 
	// {
	// 	debug_args(KERN_ERR, __func__, "Could not read from file %s", CALYPSO_FILE_PATH);
	// }
	//kfree(data);

    return res;
}

/*
 * Random write to file, using older kernel file write
 * methods
 */
// ssize_t calypso_write_segment_to_file_old(struct file *calypso_file, 
// 								loff_t offset_bytes, 
// 								size_t len_bytes,
// 								unsigned char *req_buf) 
// {
// 	size_t res;

// 	loff_t pos;

// 	char __user *data;
// 	mm_segment_t oldfs;

// 	debug(KERN_DEBUG, __func__, "Writing to file...");

// 	debug_args(KERN_DEBUG, __func__, "len_bytes: %ld", len_bytes);
// 	debug_args(KERN_DEBUG, __func__, "File position before writing: %lld", calypso_file->f_pos);
// 	debug_args(KERN_DEBUG, __func__, "offset_bytes before write: %lld", offset_bytes);

// 	/*
// 	 * ---------------------------------------------------------
// 	 * vfs_write expects the second argument to be a buffer 
// 	 * pointed to user-space memory (__user attribute), so
// 	 * passing in-kernel buffer will not work and it will
// 	 * return -EFAULT
// 	 * We can overcome this by changing user-space memory segment
// 	 * ---------------------------------------------------------
// 	 */
// 	data = (__force char __user *)req_buf;
// 	oldfs = get_fs(); /* store current user-space memory segment */
// 	set_fs(KERNEL_DS); /* set user-space memory segment equal to kernel's one */
	
// 	calypso_set_file_pos(calypso_file, offset_bytes);
// 	pos = calypso_file->f_pos;
// 	debug_args(KERN_DEBUG, __func__, "File position after seek: %lld", calypso_file->f_pos);
// 	res = vfs_write(calypso_file, data, len_bytes, &pos);
// 	debug_args(KERN_DEBUG, __func__, "res: %ld", res);
	
// 	set_fs(oldfs); /* restore user-space memory segment after reading */
	
// 	if (res > 0)
// 		calypso_file->f_pos = pos;
// 	if (res == -EBADF) 
// 	{
// 		debug_args(KERN_DEBUG, __func__, "returned -EBADF");
// 	}
// 	else if (res == -EINVAL)
// 	{
// 		debug_args(KERN_DEBUG, __func__, "returned -EINVAL");
// 	}
// 	else if (res == -EFAULT)
// 	{
// 		debug_args(KERN_DEBUG, __func__, "returned -EFAULT");
// 	}

// 	debug_args(KERN_DEBUG, __func__, "File position after writing: %lld", calypso_file->f_pos);
// 	debug_args(KERN_DEBUG, __func__, "offset_bytes after write: %lld", offset_bytes);

//     return res;
// }

/*
 * Random write to file
 */
ssize_t calypso_write_segment_to_file(struct file *calypso_file, 
							loff_t offset_bytes, 
							size_t len_bytes,
							unsigned char *req_buf) 
{	
	size_t res;

	loff_t pos;

	debug(KERN_DEBUG, __func__, "Writing to file...");

	debug_args(KERN_DEBUG, __func__, "len_bytes: %ld", len_bytes);
	debug_args(KERN_DEBUG, __func__, "File position before writing: %lld", calypso_file->f_pos);
	debug_args(KERN_DEBUG, __func__, "offset_bytes before write: %lld", offset_bytes);

	calypso_set_file_pos(calypso_file, offset_bytes);
	pos = calypso_file->f_pos;
	debug_args(KERN_DEBUG, __func__, "File position after seek: %lld", calypso_file->f_pos);
	res = kernel_write(calypso_file, req_buf, len_bytes, &pos);
	debug_args(KERN_DEBUG, __func__, "res: %ld", res);
	if (res > 0)
		calypso_file->f_pos = pos;

	debug_args(KERN_DEBUG, __func__, "File position after writing: %lld", calypso_file->f_pos);
	debug_args(KERN_DEBUG, __func__, "offset_bytes after write: %lld", offset_bytes);

	// if (len == len_bytes) 
	// {
    	//debug_args(KERN_DEBUG, __func__, "Wrote (after write) \"%s\" to %s", req_buf, CALYPSO_FILE_PATH);
	// }
	// else 
	// {
	// 	debug_args(KERN_ERR, __func__, "Could not write to file %s", CALYPSO_FILE_PATH);
	// }

    return res;
}

size_t calypso_read_file_with_offset(struct file *file, loff_t offset, unsigned char *data, size_t count)
{
    size_t len;
    len = kernel_read(file, data, count, &offset);

    return len;
}

size_t calypso_write_file_with_offset(struct file *file, loff_t offset, unsigned char *data, size_t count)
{
    size_t len;
    len = kernel_write(file, data, count, &offset);

    return len;
}

size_t calypso_read_file(struct file *file, unsigned char *data, size_t count)
{
    loff_t pos = file->f_pos;

    size_t res= kernel_read(file, data, count, &pos);
	if (res > 0)
		file->f_pos = pos;

    //debug_args(KERN_DEBUG, __func__, "Read \"%s\" from file", data);

    return res;
}

size_t calypso_write_file(struct file *file, unsigned char *data, size_t count) 
{
	loff_t pos = file->f_pos;

    size_t res = kernel_write(file, data, count, &pos);
	if (res > 0)
		file->f_pos = pos;

    //debug_args(KERN_DEBUG, __func__, "Wrote \"%s\" to file", data);

    return res;
}
