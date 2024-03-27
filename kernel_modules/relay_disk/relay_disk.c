/*
 * SO2 Lab - Block device drivers (#7)
 * Linux - Exercise #4, #5 (Relay disk - bio)
 * 
 * read data from the PHYSICAL_DISK_NAME disk 
 * (/dev/vdb) directly from the kernel
 * 
 * https://linux-kernel-labs.github.io/refs/heads/master/labs/block_device_drivers.html#block-device
 * https://github.com/linux-kernel-labs/linux/tree/master/tools/labs/templates/block_device_drivers/4-5-relay-disk
 * 
 * Usage:
 *      ---------------- SETUP DISK IMAGE ----------------
 *      $ sudo dd if=/dev/zero of=relay_disk.img bs=512 count=1000 // creates disk with 5kb (512 x 1000)
 *      $ sudo fdisk relay_disk.img // create partition that occupies all free sectors
 *      $ n
 *      $ p // primary partition
 *      $ (enter)
 *      $ (enter)
 *      $ w
 *      $ sudo mkfs.ext4 relay_disk.img // install ext4 filesystem
 *      > Filesystem too small for a journal
 *      > Creating filesystem with 125 4k blocks and 64 inodes
 *      
 *      ---------------- CREATE LOOP DEVICE ----------------
 *      $ sudo losetup -fP relay_disk.img // create loop device with disk file
 *      $ losetup -a // list loop devices and associated files
 *      > /dev/loop14: []: (/home/daniela/vboxshare/thesis/kernel_modules/relay_disk/relay_disk.img)
 *      
 *      ---------------- MOUNT LOOPBACK FILESYSTEM ----------------
 *      $ mkdir ~/relay_disk // directory where the disk will be mount
 *      $ sudo mount -o loop /dev/loop14 /home/daniela/relay_disk
 *      $ df -hP /home/daniela/relay_disk/
 *      > Filesystem      Size  Used Avail Use% Mounted on
 *      > /dev/loop15     476K   24K  420K   6% /home/daniela/relay_disk
 *      $ mount | grep relay_disk
 *      > /dev/loop14 on /home/daniela/relay_disk type ext4 (rw,relatime)
 *      
 *      ---------------- USAGE ----------------
 *      $ make
 *      $ sudo insmod relay_disk.ko
 * 
 * $ sudo xxd /home/daniela/relay_disk/hello.txt 
00000000: 6f6c 6161 6161 2061 6465 7573 7364 6466  olaaaa adeussddf
00000010: 730a

[send_test_bio] Done bio
[37534.260697] read 00 00 00
[37751.271638] [send_test_bio] Submiting bio
[37751.271773] [send_test_bio] Done bio
[37751.271774] read 64 65 66
[37758.885935] [send_test_bio] Submiting bio
[37758.886069] [send_test_bio] Done bio
[37758.886071] read 64 65 66

After erasing hello.txt
37534.260695] [send_test_bio] Done bio
[37534.260697] read 00 00 00
[37751.271638] [send_test_bio] Submiting bio
[37751.271773] [send_test_bio] Done bio
[37751.271774] read 64 65 66
[37758.885935] [send_test_bio] Submiting bio
[37758.886069] [send_test_bio] Done bio
[37758.886071] read 64 65 66
[37888.940443] [send_test_bio] Submiting bio
[37888.940629] [send_test_bio] Done bio
[37888.940631] read 64 65 66
[37904.334666] [send_test_bio] Submiting bio
[37904.334797] [send_test_bio] Done bio
[37904.334799] read 64 65 66
[37912.602997] [send_test_bio] Submiting bio
[37912.603176] [send_test_bio] Done bio
[37912.603179] read 64 65 66
[37933.116533] [send_test_bio] Submiting bio
[37933.116854] [send_test_bio] Done bio
[37933.116856] read 64 65 66

00000000: 7765 2061 7265 2072 6561 6469 6e67 2066  we are reading f
00000010: 726f 6d20 2f64 6576 2f6c 6f6f 7031 340a  rom /dev/loop14.
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/wait.h>
#include <linux/sched.h>
#include <linux/genhd.h>
#include <linux/blkdev.h>

MODULE_AUTHOR("SO2");
MODULE_DESCRIPTION("Relay disk");
MODULE_LICENSE("GPL");

#define KERN_LOG_LEVEL		KERN_ALERT

//#define PHYSICAL_DISK_NAME	"/dev/vdb"
#define PHYSICAL_DISK_NAME "/dev/loop14"
#define KERNEL_SECTOR_SIZE	512

#define BIO_WRITE_MESSAGE	"def"


/* pointer to physical device structure */
static struct block_device *phys_bdev;

/* 
 * In case of write: fill in the buffer associated to the strict bio
 * with message BIO_WRITE_MESSAGE
 */
static void send_test_bio(struct block_device *bdev, int dir)
{
	struct bio *bio = bio_alloc(GFP_NOIO, 1);
	struct page *page;
	char *buf;

	/* TODO 4/3: fill bio (bdev, sector, direction) */
	bio->bi_disk = bdev->bd_disk;
    /* the first sector of the disk is the sector with index 0 */
	bio->bi_iter.bi_sector = 0; 
    /* is going to be updated with either REQ_OP_READ or REQ_OP_WRITE */
	bio->bi_opf = dir;

    /* allocate a page to add to the bio */
	page = alloc_page(GFP_NOIO); 
	bio_add_page(bio, page, KERNEL_SECTOR_SIZE, 0);

	/* TODO 5/5: write message to bio buffer if direction is write */
	if (dir == REQ_OP_WRITE) {
        /* short duration mapping of single page with global 
        synchronization */
		buf = kmap_atomic(page);
        /* add contents of BIO_WRITE_MESSAGE to the page */
		memcpy(buf, BIO_WRITE_MESSAGE, strlen(BIO_WRITE_MESSAGE));
		/* unmap the page */
        kunmap_atomic(buf);
	}

	/* TODO 4/3: submit bio and wait for completion */
	printk(KERN_LOG_LEVEL "[send_test_bio] Submiting bio\n");
	submit_bio_wait(bio);
	printk(KERN_LOG_LEVEL "[send_test_bio] Done bio\n");

	/* TODO 4/3: read data (first 3 bytes) from bio buffer and print it */
	buf = kmap_atomic(page); // make read of 3 bytes atomic -start
	printk(KERN_LOG_LEVEL "read %02x %02x %02x\n", buf[0], buf[1], buf[2]);
	kunmap_atomic(buf); // make read of 3 bytes atomic -end

	bio_put(bio);
	__free_page(page);
}

static struct block_device *open_disk(char *name)
{
	struct block_device *bdev;

	/* TODO 4/5: get block device in exclusive mode */
	bdev = blkdev_get_by_path(name, FMODE_READ | FMODE_WRITE | FMODE_EXCL, THIS_MODULE);
	if (IS_ERR(bdev)) {
		printk(KERN_ERR "blkdev_get_by_path\n");
		return NULL;
	}

	return bdev;
}

/*
 * Function for reading
 */
static int __init relay_init(void)
{
	phys_bdev = open_disk(PHYSICAL_DISK_NAME);
	if (phys_bdev == NULL) {
		printk(KERN_ERR "[relay_init] No such device\n");
		return -EINVAL;
	}

	send_test_bio(phys_bdev, REQ_OP_READ);

	return 0;
}

static void close_disk(struct block_device *bdev)
{
	/* TODO 4/1: put block device */
	blkdev_put(bdev, FMODE_READ | FMODE_WRITE | FMODE_EXCL);
}

/*
 * Function for writing
 */
static void __exit relay_exit(void)
{
	/* TODO 5/1: send test write bio */
	send_test_bio(phys_bdev, REQ_OP_WRITE);

	close_disk(phys_bdev);
}

module_init(relay_init);
module_exit(relay_exit);