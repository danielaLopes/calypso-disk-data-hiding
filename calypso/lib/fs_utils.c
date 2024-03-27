#include <linux/bitmap.h>
#include <linux/statfs.h>
#include <linux/user_namespace.h>

#include "debug.h"
#include "global.h"
#include "fs_utils.h"

// TODO: remove these
static LIST_HEAD(super_blocks);
static DEFINE_SPINLOCK(sb_lock);

void fscrypt_sb_free(struct super_block *sb)
{
	key_put(sb->s_master_keys);
	sb->s_master_keys = NULL;
}

/*void __put_user_ns(struct user_namespace *ns)
{
	schedule_work(&ns->work);
}

static inline void put_user_ns(struct user_namespace *ns)
{
	if (ns && atomic_dec_and_test(&ns->count))
		__put_user_ns(ns);
}*/

static inline void security_sb_free(struct super_block *sb)
{ }

static void destroy_super_work(struct work_struct *work)
{
	struct super_block *s = container_of(work, struct super_block,
							destroy_work);
	int i;

	for (i = 0; i < SB_FREEZE_LEVELS; i++)
		percpu_free_rwsem(&s->s_writers.rw_sem[i]);
	kfree(s);
}

static void destroy_super_rcu(struct rcu_head *head)
{
	struct super_block *s = container_of(head, struct super_block, rcu);
	INIT_WORK(&s->destroy_work, destroy_super_work);
	schedule_work(&s->destroy_work);
}


unsigned long calypso_get_byte_bitmap(unsigned long *bitmap, int start)
{
	const size_t index = BIT_WORD(start);
	const unsigned long offset = start % BITS_PER_LONG;

	return (bitmap[index] >> offset) & 0xFF;
}

void calypso_set_byte_bitmap(unsigned long *bitmap, unsigned long value, int start)
{
	const size_t index = BIT_WORD(start);
	const unsigned long offset = start % BITS_PER_LONG;

	bitmap[index] &= ~(0xFFUL << offset);
	bitmap[index] |= value << offset;
}

struct super_block *calypso_get_blkdev_sb(struct block_device *bdev) 
{   
    /* find the superblock of the filesystem mounted on bdev */
    return get_super(bdev);
}

struct file_system_type *calypso_get_fs_type(struct super_block *sb)
{
    return sb->s_type;
}

static void __put_super(struct super_block *s)
{
	if (!--s->s_count) {
		list_del_init(&s->s_list);
		WARN_ON(s->s_dentry_lru.node);
		WARN_ON(s->s_inode_lru.node);
		WARN_ON(!list_empty(&s->s_mounts));
		security_sb_free(s);
		fscrypt_sb_free(s);
		put_user_ns(s->s_user_ns);
		kfree(s->s_subtype);
		call_rcu(&s->rcu, destroy_super_rcu);
	}
}

// TODO: USE THE KERNEL ONE
struct super_block *calypso_get_super(struct block_device *bdev)
{
	struct super_block *sb;
	bool excl = false;

	if (!bdev)
		return NULL;
	debug(KERN_DEBUG, __func__, "before spinlock()");
	spin_lock(&sb_lock);
	debug(KERN_DEBUG, __func__, "after spinlock()");
rescan:
	list_for_each_entry(sb, &super_blocks, s_list) {
		debug(KERN_DEBUG, __func__, "before if (hlist_unhashed(&sb->s_instances))\n");
		if (hlist_unhashed(&sb->s_instances))
			continue;
		debug(KERN_DEBUG, __func__, "before if (sb->s_bdev == bdev)\n");
		if (sb->s_bdev == bdev) {
			sb->s_count++;
			debug(KERN_DEBUG, __func__, "before spin_unlock()\n");
			spin_unlock(&sb_lock);
			debug(KERN_DEBUG, __func__, "before down_read()\n");
			if (!excl)
				down_read(&sb->s_umount);
			else
				down_write(&sb->s_umount);
			/* still alive? */
			debug(KERN_DEBUG, __func__, "before if (sb->s_root && (sb->s_flags & SB_BORN))\n");
			if (sb->s_root && (sb->s_flags & SB_BORN))
				return sb;
			debug(KERN_DEBUG, __func__, "before upread()\n");
			if (!excl)
				up_read(&sb->s_umount);
			else
				up_write(&sb->s_umount);
			/* nope, got unmounted */
			debug(KERN_DEBUG, __func__, "before2 spinlock()\n");
			spin_lock(&sb_lock);
			debug(KERN_DEBUG, __func__, "after __put_super()\n");
			__put_super(sb);
			debug(KERN_DEBUG, __func__, "after __put_super()\n");
			goto rescan;
		}
	}
	spin_unlock(&sb_lock);
	debug(KERN_DEBUG, __func__, "after last spin_unlock\n");
	return NULL;
}


/*
 * ------------------------------------------------------------
 * Helper functions during initialization of Calypso
 * ------------------------------------------------------------
 */

void calypso_get_super_block_physical_dev(struct calypso_blk_device *calypso_dev) {
	debug(KERN_INFO, __func__, "begin\n");
	calypso_dev->physical_super_block = get_super(calypso_dev->physical_dev);
	debug(KERN_INFO, __func__, "after get_super\n");
	calypso_dev->physical_ext4_sb_info = EXT4_SB(calypso_dev->physical_super_block);
	debug(KERN_INFO, __func__, "after getting ext4 sb info\n");
	calypso_dev->physical_ext4_sb = calypso_dev->physical_ext4_sb_info->s_es;
	debug(KERN_INFO, __func__, "after getting ext4_sb_info->s_es\n");
	calypso_dev->physical_nr_groups = ext4_get_groups_count(calypso_dev->physical_super_block);
	debug(KERN_INFO, __func__, "after ext4_get_groups_count\n");
}

unsigned long calypso_get_physical_block_count(struct calypso_blk_device *calypso_dev) 
{
	debug(KERN_DEBUG, __func__, "before blocks_count()\n");
	// TODO: yes it is blocking on get_super()!!!! checkout why! see if it is because of opening and closing the device
	// the thing is get_super causes a dev open and close, and that might be what's causing the block or something that happens after
	//struct super_block *sb = calypso_get_super(physical_dev);
	unsigned long nblocks = ext4_blocks_count(calypso_dev->physical_ext4_sb);
	debug_args(KERN_DEBUG, __func__, "There are %lu physical blocks\n", nblocks);

	return nblocks;
}

/*
 * Each group has a block bitmap
 * This function iterates through each of those
 * block bitmaps
 */
void calypso_iterate_set_regions_bitmap(const unsigned long *orig_bitmap, unsigned long *res_bitmap, unsigned int nbits, ext4_fsblk_t block_offset)
{
	unsigned int rs, re, start = 0; /* region start, region end */
	unsigned int set_nbits;
	bitmap_for_each_set_region(orig_bitmap, rs, re, start, nbits)
	{
		set_nbits = re - rs;
		//debug_args(KERN_DEBUG, __func__, "rs : %lu; re: %lu; start: %lu; nbits: %lu; set_nbits: %lu\n", block_offset + rs, block_offset + re, start, nbits, set_nbits);
		bitmap_set(res_bitmap, block_offset + rs, set_nbits);
	}
}

void calypso_bitmap_helper(const char *ptr, unsigned long *res_bitmap, unsigned int numchars, ext4_fsblk_t block_offset)
{
	size_t longs;
	const unsigned char *orig_bitmap = ptr;

	longs = numchars / sizeof(long);
	if (longs) {
		BUG_ON(longs >= INT_MAX / BITS_PER_LONG);
		calypso_iterate_set_regions_bitmap((unsigned long *)orig_bitmap, 
				res_bitmap,
				longs * BITS_PER_LONG,
				block_offset);
		numchars -= longs * sizeof(long);
		orig_bitmap += longs * sizeof(long);
	}
}

void calypso_get_fs_blocks_bitmap(struct calypso_blk_device *calypso_dev)
{
	ext4_group_t i;
	struct buffer_head *bitmap_bh = NULL;
	long bits_per_group = EXT4_CLUSTERS_PER_GROUP(calypso_dev->physical_super_block) / 8;
	ext4_fsblk_t block_offset;
	unsigned long res_bitmap_offset = 0;
	struct super_block *sb = get_super(calypso_dev->physical_dev);

	debug_args(KERN_DEBUG, __func__, "Iterating through %u groups\n", calypso_dev->physical_nr_blocks);

	for (i = 0; i < calypso_dev->physical_nr_groups; i++) 
	{
		//debug_args(KERN_DEBUG, __func__, "------ START GROUP # %u ------\n", i);

		brelse(bitmap_bh);
		bitmap_bh = ext4_read_block_bitmap(calypso_dev->physical_super_block, i);
		if (IS_ERR(bitmap_bh)) {
			bitmap_bh = NULL;
			continue;
		}

		block_offset = ext4_group_first_block_no(calypso_dev->physical_super_block, i);
		calypso_bitmap_helper(bitmap_bh->b_data,
					calypso_dev->physical_blocks_bitmap,
				    bits_per_group ,
					block_offset);
		//calypso_iterate_each_bitmap((unsigned long *)bitmap_bh, bitmap, block_offset);
		// debug_args(KERN_DEBUG, __func__, "orig bitmap %lu", (unsigned long *)bitmap_bh);
		// debug_args(KERN_DEBUG, __func__, "copied bitmap %lu", (unsigned long *)bitmap);
	}
	brelse(bitmap_bh);
}

unsigned int calypso_iterate_partition_blocks_groups(struct block_device *physical_dev)
{
    struct super_block *sb = get_super(physical_dev);
    ext4_fsblk_t desc_count;
	struct ext4_group_desc *gdp;
	ext4_group_t i;
	ext4_group_t ngroups = ext4_get_groups_count(sb);
	struct ext4_group_info *grp;
	struct ext4_super_block *es;
	ext4_fsblk_t bitmap_count;
	unsigned int x;
	struct buffer_head *bitmap_bh = NULL;

	es = EXT4_SB(sb)->s_es;
	desc_count = 0;
	bitmap_count = 0;
	gdp = NULL;

	//debug_args(KERN_DEBUG, __func__, "Iterating through %u groups\n", ngroups);

	for (i = 0; i < ngroups; i++) 
	{
		//debug_args(KERN_DEBUG, __func__, "------ START GROUP # %u ------\n", i);

  		gdp = ext4_get_group_desc(sb, i, NULL);
		if (!gdp)
			continue;
		grp = NULL;
		if (EXT4_SB(sb)->s_group_info)
			grp = ext4_get_group_info(sb, i);
		if (!grp || !EXT4_MB_GRP_BBITMAP_CORRUPT(grp))
			desc_count += ext4_free_group_clusters(sb, gdp);
		brelse(bitmap_bh);
		bitmap_bh = ext4_read_block_bitmap(sb, i);
		if (IS_ERR(bitmap_bh)) {
			bitmap_bh = NULL;
			continue;
		}

		x = ext4_count_free(bitmap_bh->b_data,
				    EXT4_CLUSTERS_PER_GROUP(sb) / 8);

		/*
		 * Gather block offset of the group so that we can determine
		 * actual block numbers when iterating through bitmap
		 */
		ext4_fsblk_t block_offset = ext4_group_first_block_no(sb, i);
		calypso_ext4_print_free_blocks(bitmap_bh->b_data,
				    EXT4_CLUSTERS_PER_GROUP(sb) / 8,
					block_offset);

		//debug_args(KERN_DEBUG, __func__, "GROUP # %u: stored = %d, counted = %u\n",
		//	        i, ext4_free_group_clusters(sb, gdp), x);
		bitmap_count += x;

		//debug_args(KERN_DEBUG, __func__, "------ END GROUP # %u ------\n", i);
	}
	brelse(bitmap_bh);
	// debug_args(KERN_DEBUG, __func__, "ext4_count_free_clusters: stored = %llu"
    //             ", computed = %llu, %llu\n",
    //             EXT4_NUM_B2C(EXT4_SB(sb), ext4_free_blocks_count(es)),
    //             desc_count, bitmap_count);

	//debug_args(KERN_DEBUG, __func__, "last block is in group %u\n", ext4_get_group_number(sb, EXT4_SB(sb)->s_es->s_blocks_count_lo));

	return bitmap_count;
}

void calypso_ext4_print_free_blocks(const char *ptr, unsigned int numchars, ext4_fsblk_t block_offset)
{
	size_t ret = 0;
	size_t longs;
	const unsigned char *bitmap = ptr;
	size_t bitmap_val;

	//printk(KERN_DEBUG "numchars: %u\n", numchars);

	longs = numchars / sizeof(long);
	if (longs) {
		BUG_ON(longs >= INT_MAX / BITS_PER_LONG);
		bitmap_val = calypso_iterate_bitmap((unsigned long *)bitmap,
				longs * BITS_PER_LONG,
				block_offset);
		ret += bitmap_val;
		numchars -= longs * sizeof(long);
		bitmap += longs * sizeof(long);

		//printk(KERN_DEBUG "*** BITMAP ***: byte %x ; bitmap_val %lu ; numchars %d\n", *bitmap, bitmap_val, numchars);
	}
}

int calypso_iterate_bitmap(const unsigned long *bitmap, unsigned int nbits, ext4_fsblk_t block_offset)
{
	unsigned int rs, re, start = 0; /* region start, region end */
	int free_count = 0;

	//printk(KERN_DEBUG "bitmap_for_each_clear_region\n");
	//printk(KERN_DEBUG "block_offset %u\n", block_offset);
	bitmap_for_each_clear_region(bitmap, rs, re, start, nbits)
	{
		free_count += (re - rs);
		//printk(KERN_DEBUG "free_count %u ; rs %u ; re %u\n", free_count, rs, re);
		//printk(KERN_DEBUG "Empty blocks from block #%llu to block #%llu\n", calypso_get_block_no(block_offset, rs), calypso_get_block_no(block_offset, re) - 1);
	}

	return free_count; /* this is supposed to return the same as bitmap_val, which sums the amount of 0's */
}


/*
 * ------------------------------------------------------------
 * Helper functions to expose information about physical 
 * device's fs
 * ------------------------------------------------------------
 */

void calypso_partition_fs_info(struct block_device *physical_dev)
{
	struct super_block *sb = get_super(physical_dev);
	struct file_system_type* fs_type = sb->s_type;

	struct ext4_sb_info *ext4_sb_info = EXT4_SB(sb);
	struct ext4_super_block *ext4_sb = ext4_sb_info->s_es;

    __le32 ext4_total_blocks_count = ext4_sb->s_blocks_count_lo;
    __le32 ext4_reserved_blocks_count = ext4_sb->s_r_blocks_count_lo;
    __le32 ext4_free_blocks_count = ext4_sb->s_free_blocks_count_lo;
    __le32 ext4_cluster_size = ext4_sb->s_log_cluster_size;
    __le32 ext4_blocks_per_group = ext4_sb->s_blocks_per_group;

    debug_args(KERN_INFO, __func__, "The file system installed on %s is %s\n", PHYSICAL_DISK_NAME, fs_type->name);
    
    debug_args(KERN_INFO, __func__, "-------- INFO ON ext4_super_block: --------\n \
                                    Partition %s has %d blocks\n \
                                    %d reserved blocks\n \
                                    %d free blocks\n \
                                    cluster size is %d\n \
                                    %d blocks per group\n", 
                                    PHYSICAL_DISK_NAME, ext4_total_blocks_count, 
                                    ext4_reserved_blocks_count,
                                    ext4_free_blocks_count,
                                    ext4_cluster_size,
                                    ext4_blocks_per_group
                                    );
}

void calypso_print_free_blocks_bitmap(struct block_device *physical_dev, unsigned int nblocks, unsigned long *bitmap)
{
    struct super_block *sb = get_super(physical_dev);
	ext4_fsblk_t block_offset = ext4_group_first_block_no(sb, 0);
	
	calypso_iterate_bitmap(bitmap, nblocks, block_offset);
}


/*
 * ------------------------------------------------------------
 * Helper functions during execution of Calypso
 * ------------------------------------------------------------
 */

void calypso_copy_block(struct block_device *physical_dev, unsigned long from_block, unsigned long to_block) 
{
	// TODO: check if we need this first part
	struct super_block *sb = get_super(physical_dev);
	struct file_system_type* fs_type = sb->s_type;

	struct ext4_sb_info *ext4_sb_info = EXT4_SB(sb);
	struct ext4_super_block *ext4_sb = ext4_sb_info->s_es;

	ext4_group_t ngroups = ext4_get_groups_count(sb);
	unsigned long block_offset, i;

	debug(KERN_INFO, __func__, "reading third block\n");
	struct buffer_head *third_block = sb_bread(sb, 3);
	debug(KERN_INFO, __func__, "after read third block\n");
	brelse(third_block);
	debug(KERN_INFO, __func__, "after brelse\n");

	debug(KERN_INFO, __func__, "before from_bh\n");
	//struct buffer_head *from_bh = sb_bread(sb, from_block);
	// struct buffer_head *from_bh = ext4_sb_bread(sb, from_block, 0);
	debug(KERN_INFO, __func__, "before to_bh");
	//struct buffer_head *to_bh = sb_bread(sb, to_block);
	// struct buffer_head *to_bh = ext4_sb_bread(sb, to_block, 0);
	debug_args(KERN_DEBUG, __func__, "from_block %lu, to_block %lu", from_block, to_block);
	//debug_args(KERN_DEBUG, __func__, "EXT4 BLOCK SIZE: %d", ext4_sb->s_log_block_size);
	//debug_args(KERN_DEBUG, __func__, "EXT4 BLOCK SIZE: %d", EXT4_BLOCK_SIZE(ext4_sb));
	// memcpy(to_bh->b_data, from_bh->b_data, ext4_sb->s_log_block_size);
	//mark_buffer_dirty(bh);
	// brelse(from_bh);
	// brelse(to_bh);

	// for (i = 0; i < ngroups; i++) 
	// {
	// 	block_offset = ext4_group_first_block_no(sb, i);
	// 	struct buffer_head *bh = sb_bread(sb, block_offset);
	// 	debug_args(KERN_DEBUG, __func__, "i: %lu; block_offset %lu", i, block_offset);
	// 	brelse(bh);
	// }
}