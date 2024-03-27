#include <linux/fs.h> /* For system calls, structures, ... */
#include <linux/kernel.h> /* For printk, ... */
#include <linux/module.h> /* For module related macros, ... */
#include <linux/slab.h>
#include <linux/blk_types.h>
#include <linux/buffer_head.h>


//#define PHYSICAL_DEV_NAME "/dev/loop14"
#define PHYSICAL_DEV_NAME "/dev/loop15"
//#define PHYSICAL_DEV_NAME "/dev/sda5"

#define FS_BLOCK_SIZE 4096

//static struct block_device *physical_dev;


static struct dentry *sfs_mount(struct file_system_type *fs_type, int flags, const char *devname, void *data);

static void sfs_kill_sb(struct super_block *sb);


/*
 * The internal representation of our device.
 */
static struct native_blk_dev {
    sector_t capacity; /* Sectors */
    struct gendisk *gd;
    spinlock_t lock;
    //struct bio_list bio_list;    
    //struct task_struct *thread;
    //int is_active;
    struct block_device *physical_dev;
    /* Our request queue */
    struct request_queue *queue;
} native_blk_dev;

typedef struct sfs_super_block
{
	unsigned int type; /* Magic number to identify the file system */
	unsigned int block_size; /* Unit of allocation */
	unsigned int partition_size; /* in blocks */
	unsigned int entry_size; /* in bytes */
	unsigned int entry_table_size; /* in blocks */
	unsigned int entry_table_block_start; /* in blocks */
	unsigned int entry_count; /* Total entries in the file system */
	unsigned int data_block_start; /* in blocks */
	unsigned int reserved[FS_BLOCK_SIZE / 4 - 8];
} sfs_super_block_t; /* Making it of SIMULA_FS_BLOCK_SIZE */

typedef struct sfs_info
{
	struct super_block *vfs_sb; /* Super block structure from VFS for this fs */
	sfs_super_block_t sb; /* Our fs super block */
	//byte1_t *used_blocks; /* Used blocks tracker */
	//spinlock_t lock; /* Used for protecting used_blocks access */
} sfs_info_t;

// static int read_block_fs_fill_super(struct super_block *sb, void *data, int silent)
// {
//     printk(KERN_INFO "read_block_fs_fill_super\n");

//     sb->s_blocksize = SIMULA_FS_BLOCK_SIZE;
//     sb->s_blocksize_bits = SIMULA_FS_BLOCK_SIZE_BITS;
//     sb->s_magic = SIMULA_FS_TYPE;
//     sb->s_type = &sfs; // file_system_type
//     sb->s_op = &sfs_sops; // super block operations

//     sfs_root_inode = iget_locked(sb, 1); // obtain an inode from VFS
//     if (!sfs_root_inode)
//     {
//         return -EACCES;
//     }
//     if (sfs_root_inode->i_state & I_NEW) // allocated fresh now
//     {
//         printk(KERN_INFO "sfs: Got new root inode, let's fill in\n");
//         sfs_root_inode->i_op = &sfs_iops; // inode operations
//         sfs_root_inode->i_mode = S_IFDIR | S_IRWXU |
//             S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH;
//         sfs_root_inode->i_fop = &sfs_fops; // file operations
//         sfs_root_inode->i_mapping->a_ops = &sfs_aops; // address operations
//         unlock_new_inode(sfs_root_inode);
//     }
//     else
//     {
//         printk(KERN_INFO "sfs: Got root inode from inode cache\n");
//     }

//     sb->s_root = d_make_root(sfs_root_inode);
//     if (!sb->s_root)
//     {
//         iget_failed(sfs_root_inode);
//         return -ENOMEM;
//     }

//     return 0;
// }

static struct file_system_type ext4_fs_type = {
	.owner		= THIS_MODULE,
	.name		= "ext4_2",
	//.mount		= ext4_mount,
    .mount		= sfs_mount,
	.kill_sb	= sfs_kill_sb,
	.fs_flags	= FS_REQUIRES_DEV,
};

static int get_bit_pos(unsigned int val)
{
	int i;

	for (i = 0; val; i++)
	{
		val >>= 1;
	}
	return (i - 1);
}

/*
 * Reads superblock, which is equivalent to reading block 0 of that device
 */
static int read_sb_from_real_sfs(sfs_info_t *info, sfs_super_block_t *sb, block_t block);
{
    struct buffer_head *bh;

    printk(KERN_INFO "sfs: read_sb_from_real_sfs, reading block %d\n", block);

    //block_t block = 64721600;
    if (!(bh = sb_bread(info->vfs_sb, block /* Super block is the 0th block */)))
    //if (!(bh = sb_bread(info->vfs_sb, block /* Super block is the 0th block */)))
    {
        return -EIO;
    }
    memcpy(sb, bh->b_data, FS_BLOCK_SIZE);
    if (sb) {
        if (sb->block_size)
            printk(KERN_INFO "READ SUPERBLOCK, sb->block_size: %d\n", sb->block_size);
        if (sb->type) {
            printk(KERN_INFO "READ SUPERBLOCK, sb->type: %x\n", sb->type);
            //printk(KERN_INFO "READ SUPERBLOCK, sb->type: %d\n", sb->type);
        }
        if (bh->b_data) {
            printk(KERN_INFO "READ SUPERBLOCK, bh->b_data exists: %s\n", bh->b_data);
            //printk(KERN_INFO "READ SUPERBLOCK, bh->b_data[0]: %02x\n", bh->b_data[0]);
        }      
    }
    brelse(bh);
    printk(KERN_INFO "READ SUPERBLOCK, after brelse\n");
    return 0;
}

static int sfs_fill_super(struct super_block *sb, void *data, int silent)
{
	sfs_info_t *info;

	if (!(info = (sfs_info_t *)(kzalloc(sizeof(sfs_info_t), GFP_KERNEL))))
		return -ENOMEM;
	info->vfs_sb = sb;
	
    if (sb) {
        if (sb->s_magic)
            printk(KERN_INFO "sfs_fill_super: sb->s_magic %ld", sb->s_magic);
        if (sb->s_blocksize)
            printk(KERN_INFO "sfs_fill_super: sb->s_blocksize %ld", sb->s_blocksize);
        //if (sb->s_type)
        //    printk(KERN_INFO "sb->s_type %s", sb->s_type);
    }

    read_sb_from_real_sfs(info, &info->sb, 0);
    read_sb_from_real_sfs(info, &info->sb, 1);
    read_sb_from_real_sfs(info, &info->sb, 2);
    
    // sb->s_magic = info->sb.type;
	// sb->s_blocksize = info->sb.block_size;
	// sb->s_blocksize_bits = get_bit_pos(info->sb.block_size);
	// sb->s_type = &ext4_fs_type; // file_system_type
	//sb->s_op = &sfs_sops; // super block operations

    printk(KERN_INFO "sfs_fill_super: after read_sb_from_real_sfs");


    return 0;
}

static void sfs_kill_sb(struct super_block *sb)
{
	sfs_info_t *info = (sfs_info_t *)(sb->s_fs_info);

	kill_block_super(sb);
	if (info)
	{
		kfree(info);
	}
}

void open_blk_dev(void) 
{
    struct block_device *physical_dev = lookup_bdev(PHYSICAL_DEV_NAME);
    printk("Opened %s\n", PHYSICAL_DEV_NAME);
    if (IS_ERR(physical_dev))
    {
        printk("stackbd: error opening raw device <%lu>\n", PTR_ERR(physical_dev));
    }
    if (!bdget(physical_dev->bd_dev))
    {
        printk("stackbd: error bdget()\n");
    }
    if (blkdev_get(physical_dev, FMODE_READ | FMODE_WRITE | FMODE_EXCL, &native_blk_dev))
    {
        printk("stackbd: error blkdev_get()\n");
        bdput(physical_dev);
    }
}

/*
 * File-System Operations
 */
static struct dentry *sfs_mount(struct file_system_type *fs_type, int flags, const char *devname, void *data)
{
	printk(KERN_INFO "sfs: sfs_mount: devname = %s\n", devname);

	 /* sfs_fill_super this will be called to fill the super block */
	return mount_bdev(fs_type, flags, devname, data, &sfs_fill_super);
}

static int __init read_block_fs_init(void)
{
	int err;

	printk(KERN_INFO "read_block_fs_init\n");

    err = register_filesystem(&ext4_fs_type);

    //char *buf = kzalloc(4096 * sizeof(char), GFP_KERNEL);

	// physical_dev = blkdev_get_by_dev(PHYSICAL_DEV_NAME, FMODE_READ|FMODE_WRITE|FMODE_EXCL,
	// 			 shared ? (struct md_rdev *)lock_rdev : rdev);
    //physical_dev = blkdev_get_by_path(PHYSICAL_DEV_NAME, FMODE_READ | FMODE_WRITE | FMODE_EXCL, THIS_MODULE);

    //open_blk_dev();

    //mount_bdev(&ext4_fs_type, flags, PHYSICAL_DEV_NAME, data, &sfs_fill_super);

    //printk(KERN_INFO "before __bread");

    //struct buffer_head *bh = __bread(physical_dev, 26974752, 8);
    //struct buffer_head *bh = __bread(physical_dev, 25922080, 8);
    //struct buffer_head *bh = __bread(native_blk_dev.physical_dev, 288, 8);

    //printk(KERN_INFO "after __bread");

    // memcpy(buf, bh->b_data, 8);
    // brelse(bh);

    // printk(KERN_INFO "READ: \"%s\"", buf);

    // kfree(buf);

    
	
	//return err;
    return err;
}

static void __exit read_block_fs_exit(void)
{
	printk(KERN_INFO "read_block_fs_exit\n");

    unregister_filesystem(&ext4_fs_type);

    //blkdev_put(physical_dev, FMODE_READ|FMODE_WRITE|FMODE_EXCL);
	
}

module_init(read_block_fs_init);
module_exit(read_block_fs_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Daniela Lopes");
MODULE_DESCRIPTION("Read block from file through the file system layer");