#include <linux/types.h>
#include <linux/limits.h>
#include <linux/string.h>
#include <linux/bits.h>

#include <linux/fs.h>
#include <linux/seq_file.h>
#include <linux/slab.h>
#include <linux/proc_fs.h>
//#include <linux/part_stat.h>
#include <linux/backing-dev.h>

//#include <trace/events/ext4.h>

#include "ext4/ext4.h"
#include "ext4_fs_utils.h"


static int ext4_init_block_bitmap(struct super_block *sb,
				   struct buffer_head *bh,
				   ext4_group_t block_group,
				   struct ext4_group_desc *gdp);

static ext4_fsblk_t ext4_valid_block_bitmap(struct super_block *sb,
					    struct ext4_group_desc *desc,
					    ext4_group_t block_group,
					    struct buffer_head *bh);

static int ext4_validate_block_bitmap(struct super_block *sb,
				      struct ext4_group_desc *desc,
				      ext4_group_t block_group,
				      struct buffer_head *bh);

static unsigned int num_clusters_in_group(struct super_block *sb,
					  ext4_group_t block_group);

static unsigned ext4_num_base_meta_clusters(struct super_block *sb,
				     ext4_group_t block_group);

static inline int ext4_block_in_group(struct super_block *sb,
				      ext4_fsblk_t block,
				      ext4_group_t block_group);

static inline int test_root(ext4_group_t a, int b);

void ext4_read_bh_nowait(struct buffer_head *bh, int op_flags,
			 bh_end_io_t *end_io);

static inline void __ext4_read_bh(struct buffer_head *bh, int op_flags,
				  bh_end_io_t *end_io);

unsigned long ext4_bg_num_gdb(struct super_block *sb, ext4_group_t group);

static void save_error_info(struct super_block *sb, int error,
			    __u32 ino, __u64 block,
			    const char *func, unsigned int line);

static int ext4_commit_super(struct super_block *sb, int sync);

static int block_device_ejected(struct super_block *sb);

static __le32 ext4_superblock_csum(struct super_block *sb,
				   struct ext4_super_block *es);

static void __ext4_update_tstamp(__le32 *lo, __u8 *hi)
{
	time64_t now = ktime_get_real_seconds();

	now = clamp_val(now, 0, (1ull << 40) - 1);

	*lo = cpu_to_le32(lower_32_bits(now));
	*hi = upper_32_bits(now);
}

#define ext4_update_tstamp(es, tstamp) \
	__ext4_update_tstamp(&(es)->tstamp, &(es)->tstamp ## _hi)

unsigned int ext4_count_free(char *bitmap, unsigned int numchars)
{
	return numchars * BITS_PER_BYTE - memweight(bitmap, numchars);
}

/**
 * ext4_get_group_desc() -- load group descriptor from disk
 * @sb:			super block
 * @block_group:	given block group
 * @bh:			pointer to the buffer head to store the block
 *			group descriptor
 */
struct ext4_group_desc * ext4_get_group_desc(struct super_block *sb,
					     ext4_group_t block_group,
					     struct buffer_head **bh)
{
	unsigned int group_desc;
	unsigned int offset;
	ext4_group_t ngroups = ext4_get_groups_count(sb);
	struct ext4_group_desc *desc;
	struct ext4_sb_info *sbi = EXT4_SB(sb);
	struct buffer_head *bh_p;

	if (block_group >= ngroups) {
		ext4_error(sb, "block_group >= groups_count - block_group = %u,"
			   " groups_count = %u", block_group, ngroups);

		return NULL;
	}

	group_desc = block_group >> EXT4_DESC_PER_BLOCK_BITS(sb);
	offset = block_group & (EXT4_DESC_PER_BLOCK(sb) - 1);
	bh_p = sbi_array_rcu_deref(sbi, s_group_desc, group_desc);
	/*
	 * sbi_array_rcu_deref returns with rcu unlocked, this is ok since
	 * the pointer being dereferenced won't be dereferenced again. By
	 * looking at the usage in add_new_gdb() the value isn't modified,
	 * just the pointer, and so it remains valid.
	 */
	if (!bh_p) {
		ext4_error(sb, "Group descriptor not loaded - "
			   "block_group = %u, group_desc = %u, desc = %u",
			   block_group, group_desc, offset);
		return NULL;
	}

	desc = (struct ext4_group_desc *)(
		(__u8 *)bh_p->b_data +
		offset * EXT4_DESC_SIZE(sb));
	if (bh)
		*bh = bh_p;
	return desc;
}

/**
 * ext4_read_block_bitmap_nowait()
 * @sb:			super block
 * @block_group:	given block group
 *
 * Read the bitmap for a given block_group,and validate the
 * bits for block/inode/inode tables are set in the bitmaps
 *
 * Return buffer_head on success or an ERR_PTR in case of failure.
 */
struct buffer_head *
ext4_read_block_bitmap_nowait(struct super_block *sb, ext4_group_t block_group,
			      bool ignore_locked)
{
	struct ext4_group_desc *desc;
	struct ext4_sb_info *sbi = EXT4_SB(sb);
	struct buffer_head *bh;
	ext4_fsblk_t bitmap_blk;
	int err;

	desc = ext4_get_group_desc(sb, block_group, NULL);
	if (!desc)
		return ERR_PTR(-EFSCORRUPTED);
	bitmap_blk = ext4_block_bitmap(sb, desc);
	// if ((bitmap_blk <= le32_to_cpu(sbi->s_es->s_first_data_block)) ||
	//     (bitmap_blk >= ext4_blocks_count(sbi->s_es))) {
	// 	ext4_error(sb, "Invalid block bitmap block %llu in "
	// 		   "block_group %u", bitmap_blk, block_group);
	// 	ext4_mark_group_bitmap_corrupted(sb, block_group,
	// 				EXT4_GROUP_INFO_BBITMAP_CORRUPT);
	// 	return ERR_PTR(-EFSCORRUPTED);
	// }
	bh = sb_getblk(sb, bitmap_blk);
	//printk(KERN_INFO "Got the following bitmap block %s", bh->b_data);
	if (unlikely(!bh)) {
		ext4_warning(sb, "Cannot get buffer for block bitmap - "
			     "block_group = %u, block_bitmap = %llu",
			     block_group, bitmap_blk);
		return ERR_PTR(-ENOMEM);
	}

	if (ignore_locked && buffer_locked(bh)) {
		/* buffer under IO already, return if called for prefetching */
		put_bh(bh);
		return NULL;
	}

	if (bitmap_uptodate(bh))
		goto verify;

	lock_buffer(bh);
	if (bitmap_uptodate(bh)) {
		unlock_buffer(bh);
		goto verify;
	}
	ext4_lock_group(sb, block_group);
	if (ext4_has_group_desc_csum(sb) &&
	    (desc->bg_flags & cpu_to_le16(EXT4_BG_BLOCK_UNINIT))) {
		if (block_group == 0) {
			ext4_unlock_group(sb, block_group);
			unlock_buffer(bh);
			ext4_error(sb, "Block bitmap for bg 0 marked "
				   "uninitialized");
			err = -EFSCORRUPTED;
			goto out;
		}
		err = ext4_init_block_bitmap(sb, bh, block_group, desc);
		set_bitmap_uptodate(bh);
		set_buffer_uptodate(bh);
		set_buffer_verified(bh);
		ext4_unlock_group(sb, block_group);
		unlock_buffer(bh);
		if (err) {
			ext4_error(sb, "Failed to init block bitmap for group "
				   "%u: %d", block_group, err);
			goto out;
		}
		goto verify;
	}
	ext4_unlock_group(sb, block_group);
	if (buffer_uptodate(bh)) {
		/*
		 * if not uninit if bh is uptodate,
		 * bitmap is also uptodate
		 */
		set_bitmap_uptodate(bh);
		unlock_buffer(bh);
		goto verify;
	}
	/*
	 * submit the buffer_head for reading
	 */
	set_buffer_new(bh);
	//trace_ext4_read_block_bitmap_load(sb, block_group, ignore_locked);
	ext4_read_bh_nowait(bh, REQ_META | REQ_PRIO |
			    (ignore_locked ? REQ_RAHEAD : 0),
			    ext4_end_bitmap_read);
	return bh;
verify:
	err = ext4_validate_block_bitmap(sb, desc, block_group, bh);
	if (err)
		goto out;
	return bh;
out:
	put_bh(bh);
	return ERR_PTR(err);
}

struct buffer_head *
ext4_read_block_bitmap(struct super_block *sb, ext4_group_t block_group)
{
	struct buffer_head *bh;
	int err;

	bh = ext4_read_block_bitmap_nowait(sb, block_group, false);
	if (IS_ERR(bh))
		return bh;
	err = ext4_wait_block_bitmap(sb, block_group, bh);
	if (err) {
		put_bh(bh);
		return ERR_PTR(err);
	}
	return bh;
}

/* Initializes an uninitialized block bitmap */
static int ext4_init_block_bitmap(struct super_block *sb,
				   struct buffer_head *bh,
				   ext4_group_t block_group,
				   struct ext4_group_desc *gdp)
{
	unsigned int bit, bit_max;
	struct ext4_sb_info *sbi = EXT4_SB(sb);
	ext4_fsblk_t start, tmp;

	J_ASSERT_BH(bh, buffer_locked(bh));

	/* If checksum is bad mark all blocks used to prevent allocation
	 * essentially implementing a per-group read-only flag. */
	// if (!ext4_group_desc_csum_verify(sb, block_group, gdp)) {
	// 	ext4_mark_group_bitmap_corrupted(sb, block_group,
	// 				EXT4_GROUP_INFO_BBITMAP_CORRUPT |
	// 				EXT4_GROUP_INFO_IBITMAP_CORRUPT);
	// 	return -EFSBADCRC;
	// }
	memset(bh->b_data, 0, sb->s_blocksize);

	bit_max = ext4_num_base_meta_clusters(sb, block_group);
	if ((bit_max >> 3) >= bh->b_size)
		return -EFSCORRUPTED;

	for (bit = 0; bit < bit_max; bit++)
		ext4_set_bit(bit, bh->b_data);

	start = ext4_group_first_block_no(sb, block_group);

	/* Set bits for block and inode bitmaps, and inode table */
	tmp = ext4_block_bitmap(sb, gdp);
	if (ext4_block_in_group(sb, tmp, block_group))
		ext4_set_bit(EXT4_B2C(sbi, tmp - start), bh->b_data);

	tmp = ext4_inode_bitmap(sb, gdp);
	if (ext4_block_in_group(sb, tmp, block_group))
		ext4_set_bit(EXT4_B2C(sbi, tmp - start), bh->b_data);

	tmp = ext4_inode_table(sb, gdp);
	for (; tmp < ext4_inode_table(sb, gdp) +
		     sbi->s_itb_per_group; tmp++) {
		if (ext4_block_in_group(sb, tmp, block_group))
			ext4_set_bit(EXT4_B2C(sbi, tmp - start), bh->b_data);
	}

	/*
	 * Also if the number of blocks within the group is less than
	 * the blocksize * 8 ( which is the size of bitmap ), set rest
	 * of the block bitmap to 1
	 */
	ext4_mark_bitmap_end(num_clusters_in_group(sb, block_group),
			     sb->s_blocksize * 8, bh->b_data);
	return 0;
}

/*
 * Return the block number which was discovered to be invalid, or 0 if
 * the block bitmap is valid.
 */
static ext4_fsblk_t ext4_valid_block_bitmap(struct super_block *sb,
					    struct ext4_group_desc *desc,
					    ext4_group_t block_group,
					    struct buffer_head *bh)
{
	struct ext4_sb_info *sbi = EXT4_SB(sb);
	ext4_grpblk_t offset;
	ext4_grpblk_t next_zero_bit;
	ext4_grpblk_t max_bit = EXT4_CLUSTERS_PER_GROUP(sb);
	ext4_fsblk_t blk;
	ext4_fsblk_t group_first_block;

	if (ext4_has_feature_flex_bg(sb)) {
		/* with FLEX_BG, the inode/block bitmaps and itable
		 * blocks may not be in the group at all
		 * so the bitmap validation will be skipped for those groups
		 * or it has to also read the block group where the bitmaps
		 * are located to verify they are set.
		 */
		return 0;
	}
	group_first_block = ext4_group_first_block_no(sb, block_group);

	/* check whether block bitmap block number is set */
	blk = ext4_block_bitmap(sb, desc);
	offset = blk - group_first_block;
	if (offset < 0 || EXT4_B2C(sbi, offset) >= max_bit ||
	    !ext4_test_bit(EXT4_B2C(sbi, offset), bh->b_data))
		/* bad block bitmap */
		return blk;

	/* check whether the inode bitmap block number is set */
	blk = ext4_inode_bitmap(sb, desc);
	offset = blk - group_first_block;
	if (offset < 0 || EXT4_B2C(sbi, offset) >= max_bit ||
	    !ext4_test_bit(EXT4_B2C(sbi, offset), bh->b_data))
		/* bad block bitmap */
		return blk;

	/* check whether the inode table block number is set */
	blk = ext4_inode_table(sb, desc);
	offset = blk - group_first_block;
	if (offset < 0 || EXT4_B2C(sbi, offset) >= max_bit ||
	    EXT4_B2C(sbi, offset + sbi->s_itb_per_group) >= max_bit)
		return blk;
	next_zero_bit = ext4_find_next_zero_bit(bh->b_data,
			EXT4_B2C(sbi, offset + sbi->s_itb_per_group),
			EXT4_B2C(sbi, offset));
	if (next_zero_bit <
	    EXT4_B2C(sbi, offset + sbi->s_itb_per_group))
		/* bad bitmap for inode tables */
		return blk;
	return 0;
}

static int ext4_validate_block_bitmap(struct super_block *sb,
				      struct ext4_group_desc *desc,
				      ext4_group_t block_group,
				      struct buffer_head *bh)
{
	ext4_fsblk_t	blk;
	struct ext4_group_info *grp = ext4_get_group_info(sb, block_group);

	if (buffer_verified(bh))
		return 0;
	if (EXT4_MB_GRP_BBITMAP_CORRUPT(grp))
		return -EFSCORRUPTED;

	ext4_lock_group(sb, block_group);
	if (buffer_verified(bh))
		goto verified;
	// if (unlikely(!ext4_block_bitmap_csum_verify(sb, block_group,
	// 					    desc, bh) ||
	// 	     ext4_simulate_fail(sb, EXT4_SIM_BBITMAP_CRC))) {
	// 	ext4_unlock_group(sb, block_group);
	// 	ext4_error(sb, "bg %u: bad block bitmap checksum", block_group);
	// 	ext4_mark_group_bitmap_corrupted(sb, block_group,
	// 				EXT4_GROUP_INFO_BBITMAP_CORRUPT);
	// 	return -EFSBADCRC;
	// }
	blk = ext4_valid_block_bitmap(sb, desc, block_group, bh);
	// if (unlikely(blk != 0)) {
	// 	ext4_unlock_group(sb, block_group);
	// 	ext4_error(sb, "bg %u: block %llu: invalid block bitmap",
	// 		   block_group, blk);
	// 	ext4_mark_group_bitmap_corrupted(sb, block_group,
	// 				EXT4_GROUP_INFO_BBITMAP_CORRUPT);
	// 	return -EFSCORRUPTED;
	// }
	set_buffer_verified(bh);
verified:
	ext4_unlock_group(sb, block_group);
	return 0;
}

static unsigned int num_clusters_in_group(struct super_block *sb,
					  ext4_group_t block_group)
{
	unsigned int blocks;

	if (block_group == ext4_get_groups_count(sb) - 1) {
		/*
		 * Even though mke2fs always initializes the first and
		 * last group, just in case some other tool was used,
		 * we need to make sure we calculate the right free
		 * blocks.
		 */
		blocks = ext4_blocks_count(EXT4_SB(sb)->s_es) -
			ext4_group_first_block_no(sb, block_group);
	} else
		blocks = EXT4_BLOCKS_PER_GROUP(sb);
	return EXT4_NUM_B2C(EXT4_SB(sb), blocks);
}

/*
 * This function returns the number of file system metadata clusters at
 * the beginning of a block group, including the reserved gdt blocks.
 */
static unsigned ext4_num_base_meta_clusters(struct super_block *sb,
				     ext4_group_t block_group)
{
	struct ext4_sb_info *sbi = EXT4_SB(sb);
	unsigned num;

	/* Check for superblock and gdt backups in this group */
	num = ext4_bg_has_super(sb, block_group);

	if (!ext4_has_feature_meta_bg(sb) ||
	    block_group < le32_to_cpu(sbi->s_es->s_first_meta_bg) *
			  sbi->s_desc_per_block) {
		if (num) {
			num += ext4_bg_num_gdb(sb, block_group);
			num += le16_to_cpu(sbi->s_es->s_reserved_gdt_blocks);
		}
	} else { /* For META_BG_BLOCK_GROUPS */
		num += ext4_bg_num_gdb(sb, block_group);
	}
	return EXT4_NUM_B2C(sbi, num);
}

static inline int ext4_block_in_group(struct super_block *sb,
				      ext4_fsblk_t block,
				      ext4_group_t block_group)
{
	ext4_group_t actual_group;

	actual_group = ext4_get_group_number(sb, block);
	return (actual_group == block_group) ? 1 : 0;
}

/*
 * Calculate block group number for a given block number
 */
ext4_group_t ext4_get_group_number(struct super_block *sb,
				   ext4_fsblk_t block)
{
	ext4_group_t group;

	if (test_opt2(sb, STD_GROUP_SIZE))
		group = (block -
			 le32_to_cpu(EXT4_SB(sb)->s_es->s_first_data_block)) >>
			(EXT4_BLOCK_SIZE_BITS(sb) + EXT4_CLUSTER_BITS(sb) + 3);
	else
		ext4_get_group_no_and_offset(sb, block, &group, NULL);
	return group;
}

void ext4_end_bitmap_read(struct buffer_head *bh, int uptodate)
{
	if (uptodate) {
		set_buffer_uptodate(bh);
		set_bitmap_uptodate(bh);
	}
	unlock_buffer(bh);
	put_bh(bh);
}

ext4_fsblk_t ext4_block_bitmap(struct super_block *sb,
			       struct ext4_group_desc *bg)
{
	return le32_to_cpu(bg->bg_block_bitmap_lo) |
		(EXT4_DESC_SIZE(sb) >= EXT4_MIN_DESC_SIZE_64BIT ?
		 (ext4_fsblk_t)le32_to_cpu(bg->bg_block_bitmap_hi) << 32 : 0);
}

ext4_fsblk_t ext4_inode_bitmap(struct super_block *sb,
			       struct ext4_group_desc *bg)
{
	return le32_to_cpu(bg->bg_inode_bitmap_lo) |
		(EXT4_DESC_SIZE(sb) >= EXT4_MIN_DESC_SIZE_64BIT ?
		 (ext4_fsblk_t)le32_to_cpu(bg->bg_inode_bitmap_hi) << 32 : 0);
}

ext4_fsblk_t ext4_inode_table(struct super_block *sb,
			      struct ext4_group_desc *bg)
{
	return le32_to_cpu(bg->bg_inode_table_lo) |
		(EXT4_DESC_SIZE(sb) >= EXT4_MIN_DESC_SIZE_64BIT ?
		 (ext4_fsblk_t)le32_to_cpu(bg->bg_inode_table_hi) << 32 : 0);
}

/*
 * Calculate the block group number and offset into the block/cluster
 * allocation bitmap, given a block number
 */
void ext4_get_group_no_and_offset(struct super_block *sb, ext4_fsblk_t blocknr,
		ext4_group_t *blockgrpp, ext4_grpblk_t *offsetp)
{
	struct ext4_super_block *es = EXT4_SB(sb)->s_es;
	ext4_grpblk_t offset;

	blocknr = blocknr - le32_to_cpu(es->s_first_data_block);
	offset = do_div(blocknr, EXT4_BLOCKS_PER_GROUP(sb)) >>
		EXT4_SB(sb)->s_cluster_bits;
	if (offsetp)
		*offsetp = offset;
	if (blockgrpp)
		*blockgrpp = blocknr;

}

/**
 *	ext4_bg_has_super - number of blocks used by the superblock in group
 *	@sb: superblock for filesystem
 *	@group: group number to check
 *
 *	Return the number of blocks used by the superblock (primary or backup)
 *	in this group.  Currently this will be only 0 or 1.
 */
int ext4_bg_has_super(struct super_block *sb, ext4_group_t group)
{
	struct ext4_super_block *es = EXT4_SB(sb)->s_es;

	if (group == 0)
		return 1;
	if (ext4_has_feature_sparse_super2(sb)) {
		if (group == le32_to_cpu(es->s_backup_bgs[0]) ||
		    group == le32_to_cpu(es->s_backup_bgs[1]))
			return 1;
		return 0;
	}
	if ((group <= 1) || !ext4_has_feature_sparse_super(sb))
		return 1;
	if (!(group & 1))
		return 0;
	if (test_root(group, 3) || (test_root(group, 5)) ||
	    test_root(group, 7))
		return 1;

	return 0;
}

static inline int test_root(ext4_group_t a, int b)
{
	while (1) {
		if (a < b)
			return 0;
		if (a == b)
			return 1;
		if ((a % b) != 0)
			return 0;
		a = a / b;
	}
}

/*
 * To avoid calling the atomic setbit hundreds or thousands of times, we only
 * need to use it within a single byte (to ensure we get endianness right).
 * We can use memset for the rest of the bitmap as there are no other users.
 */
void ext4_mark_bitmap_end(int start_bit, int end_bit, char *bitmap)
{
	int i;

	if (start_bit >= end_bit)
		return;

	ext4_debug("mark end bits +%d through +%d used\n", start_bit, end_bit);
	for (i = start_bit; i < ((start_bit + 7) & ~7UL); i++)
		ext4_set_bit(i, bitmap);
	if (i < end_bit)
		memset(bitmap + (i >> 3), 0xff, (end_bit - i) >> 3);
}

void ext4_read_bh_nowait(struct buffer_head *bh, int op_flags,
			 bh_end_io_t *end_io)
{
	BUG_ON(!buffer_locked(bh));

	if (ext4_buffer_uptodate(bh)) {
		unlock_buffer(bh);
		return;
	}
	__ext4_read_bh(bh, op_flags, end_io);
}

static inline void __ext4_read_bh(struct buffer_head *bh, int op_flags,
				  bh_end_io_t *end_io)
{
	/*
	 * buffer's verified bit is no longer valid after reading from
	 * disk again due to write out error, clear it to make sure we
	 * recheck the buffer contents.
	 */
	clear_buffer_verified(bh);

	bh->b_end_io = end_io ? end_io : end_buffer_read_sync;
	get_bh(bh);
	submit_bh(REQ_OP_READ, op_flags, bh);
}

/* Returns 0 on success, -errno on error */
int ext4_wait_block_bitmap(struct super_block *sb, ext4_group_t block_group,
			   struct buffer_head *bh)
{
	struct ext4_group_desc *desc;

	if (!buffer_new(bh))
		return 0;
	desc = ext4_get_group_desc(sb, block_group, NULL);
	if (!desc)
		return -EFSCORRUPTED;
	wait_on_buffer(bh);
	ext4_simulate_fail_bh(sb, bh, EXT4_SIM_BBITMAP_EIO);
	// if (!buffer_uptodate(bh)) {
	// 	ext4_error_err(sb, EIO, "Cannot read block bitmap - "
	// 		       "block_group = %u, block_bitmap = %llu",
	// 		       block_group, (unsigned long long) bh->b_blocknr);
	// 	ext4_mark_group_bitmap_corrupted(sb, block_group,
	// 				EXT4_GROUP_INFO_BBITMAP_CORRUPT);
	// 	return -EIO;
	// }
	clear_buffer_new(bh);
	/* Panic or remount fs read-only if block bitmap is invalid */
	return ext4_validate_block_bitmap(sb, desc, block_group, bh);
}

static unsigned long ext4_bg_num_gdb_meta(struct super_block *sb,
					ext4_group_t group)
{
	unsigned long metagroup = group / EXT4_DESC_PER_BLOCK(sb);
	ext4_group_t first = metagroup * EXT4_DESC_PER_BLOCK(sb);
	ext4_group_t last = first + EXT4_DESC_PER_BLOCK(sb) - 1;

	if (group == first || group == first + 1 || group == last)
		return 1;
	return 0;
}

static unsigned long ext4_bg_num_gdb_nometa(struct super_block *sb,
					ext4_group_t group)
{
	if (!ext4_bg_has_super(sb, group))
		return 0;

	if (ext4_has_feature_meta_bg(sb))
		return le32_to_cpu(EXT4_SB(sb)->s_es->s_first_meta_bg);
	else
		return EXT4_SB(sb)->s_gdb_count;
}

/**
 *	ext4_bg_num_gdb - number of blocks used by the group table in group
 *	@sb: superblock for filesystem
 *	@group: group number to check
 *
 *	Return the number of blocks used by the group descriptor table
 *	(primary or backup) in this group.  In the future there may be a
 *	different number of descriptor blocks in each group.
 */
unsigned long ext4_bg_num_gdb(struct super_block *sb, ext4_group_t group)
{
	unsigned long first_meta_bg =
			le32_to_cpu(EXT4_SB(sb)->s_es->s_first_meta_bg);
	unsigned long metagroup = group / EXT4_DESC_PER_BLOCK(sb);

	if (!ext4_has_feature_meta_bg(sb) || metagroup < first_meta_bg)
		return ext4_bg_num_gdb_nometa(sb, group);

	return ext4_bg_num_gdb_meta(sb,group);

}

__u32 ext4_free_group_clusters(struct super_block *sb,
			       struct ext4_group_desc *bg)
{
	return le16_to_cpu(bg->bg_free_blocks_count_lo) |
		(EXT4_DESC_SIZE(sb) >= EXT4_MIN_DESC_SIZE_64BIT ?
		 (__u32)le16_to_cpu(bg->bg_free_blocks_count_hi) << 16 : 0);
}

void __ext4_warning(struct super_block *sb, const char *function,
		    unsigned int line, const char *fmt, ...)
{
	struct va_format vaf;
	va_list args;

	// if (!ext4_warning_ratelimit(sb))
	// 	return;

	va_start(args, fmt);
	vaf.fmt = fmt;
	vaf.va = &args;
	printk(KERN_WARNING "EXT4-fs warning (device %s): %s:%d: %pV\n",
	       sb->s_id, function, line, &vaf);
	va_end(args);
}

void __ext4_error(struct super_block *sb, const char *function,
		  unsigned int line, int error, __u64 block,
		  const char *fmt, ...)
{
	struct va_format vaf;
	va_list args;

	if (unlikely(ext4_forced_shutdown(EXT4_SB(sb))))
		return;

	//trace_ext4_error(sb, function, line);
	// if (ext4_error_ratelimit(sb)) {
	// 	va_start(args, fmt);
	// 	vaf.fmt = fmt;
	// 	vaf.va = &args;
	// 	printk(KERN_CRIT
	// 	       "EXT4-fs error (device %s): %s:%d: comm %s: %pV\n",
	// 	       sb->s_id, function, line, current->comm, &vaf);
	// 	va_end(args);
	// }
	save_error_info(sb, error, 0, block, function, line);
	//ext4_handle_error(sb);
}

static void __save_error_info(struct super_block *sb, int error,
			      __u32 ino, __u64 block,
			      const char *func, unsigned int line)
{
	struct ext4_super_block *es = EXT4_SB(sb)->s_es;
	int err;

	EXT4_SB(sb)->s_mount_state |= EXT4_ERROR_FS;
	if (bdev_read_only(sb->s_bdev))
		return;
	es->s_state |= cpu_to_le16(EXT4_ERROR_FS);
	ext4_update_tstamp(es, s_last_error_time);
	strncpy(es->s_last_error_func, func, sizeof(es->s_last_error_func));
	es->s_last_error_line = cpu_to_le32(line);
	es->s_last_error_ino = cpu_to_le32(ino);
	es->s_last_error_block = cpu_to_le64(block);
	switch (error) {
	case EIO:
		err = EXT4_ERR_EIO;
		break;
	case ENOMEM:
		err = EXT4_ERR_ENOMEM;
		break;
	case EFSBADCRC:
		err = EXT4_ERR_EFSBADCRC;
		break;
	case 0:
	case EFSCORRUPTED:
		err = EXT4_ERR_EFSCORRUPTED;
		break;
	case ENOSPC:
		err = EXT4_ERR_ENOSPC;
		break;
	case ENOKEY:
		err = EXT4_ERR_ENOKEY;
		break;
	case EROFS:
		err = EXT4_ERR_EROFS;
		break;
	case EFBIG:
		err = EXT4_ERR_EFBIG;
		break;
	case EEXIST:
		err = EXT4_ERR_EEXIST;
		break;
	case ERANGE:
		err = EXT4_ERR_ERANGE;
		break;
	case EOVERFLOW:
		err = EXT4_ERR_EOVERFLOW;
		break;
	case EBUSY:
		err = EXT4_ERR_EBUSY;
		break;
	case ENOTDIR:
		err = EXT4_ERR_ENOTDIR;
		break;
	case ENOTEMPTY:
		err = EXT4_ERR_ENOTEMPTY;
		break;
	case ESHUTDOWN:
		err = EXT4_ERR_ESHUTDOWN;
		break;
	case EFAULT:
		err = EXT4_ERR_EFAULT;
		break;
	default:
		err = EXT4_ERR_UNKNOWN;
	}
	es->s_last_error_errcode = err;
	if (!es->s_first_error_time) {
		es->s_first_error_time = es->s_last_error_time;
		es->s_first_error_time_hi = es->s_last_error_time_hi;
		strncpy(es->s_first_error_func, func,
			sizeof(es->s_first_error_func));
		es->s_first_error_line = cpu_to_le32(line);
		es->s_first_error_ino = es->s_last_error_ino;
		es->s_first_error_block = es->s_last_error_block;
		es->s_first_error_errcode = es->s_last_error_errcode;
	}
	/*
	 * Start the daily error reporting function if it hasn't been
	 * started already
	 */
	if (!es->s_error_count)
		mod_timer(&EXT4_SB(sb)->s_err_report, jiffies + 24*60*60*HZ);
	le32_add_cpu(&es->s_error_count, 1);
}

static void save_error_info(struct super_block *sb, int error,
			    __u32 ino, __u64 block,
			    const char *func, unsigned int line)
{
	__save_error_info(sb, error, ino, block, func, line);
	if (!bdev_read_only(sb->s_bdev))
		ext4_commit_super(sb, 1);
}

static int ext4_commit_super(struct super_block *sb, int sync)
{
	struct ext4_super_block *es = EXT4_SB(sb)->s_es;
	struct buffer_head *sbh = EXT4_SB(sb)->s_sbh;
	int error = 0;

	if (!sbh || block_device_ejected(sb))
		return error;

	/*
	 * The superblock bh should be mapped, but it might not be if the
	 * device was hot-removed. Not much we can do but fail the I/O.
	 */
	if (!buffer_mapped(sbh))
		return error;

	/*
	 * If the file system is mounted read-only, don't update the
	 * superblock write time.  This avoids updating the superblock
	 * write time when we are mounting the root file system
	 * read/only but we need to replay the journal; at that point,
	 * for people who are east of GMT and who make their clock
	 * tick in localtime for Windows bug-for-bug compatibility,
	 * the clock is set in the future, and this will cause e2fsck
	 * to complain and force a full file system check.
	 */
	if (!(sb->s_flags & SB_RDONLY))
		ext4_update_tstamp(es, s_wtime);
	// TODO
	if (sb->s_bdev->bd_part)
		es->s_kbytes_written =
			cpu_to_le64(EXT4_SB(sb)->s_kbytes_written +
			    ((part_stat_read(sb->s_bdev->bd_part,
					     sectors[STAT_WRITE]) -
			      EXT4_SB(sb)->s_sectors_written_start) >> 1));
	else
		es->s_kbytes_written =
			cpu_to_le64(EXT4_SB(sb)->s_kbytes_written);
	if (percpu_counter_initialized(&EXT4_SB(sb)->s_freeclusters_counter))
		ext4_free_blocks_count_set(es,
			EXT4_C2B(EXT4_SB(sb), percpu_counter_sum_positive(
				&EXT4_SB(sb)->s_freeclusters_counter)));
	if (percpu_counter_initialized(&EXT4_SB(sb)->s_freeinodes_counter))
		es->s_free_inodes_count =
			cpu_to_le32(percpu_counter_sum_positive(
				&EXT4_SB(sb)->s_freeinodes_counter));
	BUFFER_TRACE(sbh, "marking dirty");
	ext4_superblock_csum_set(sb);
	if (sync)
		lock_buffer(sbh);
	if (buffer_write_io_error(sbh) || !buffer_uptodate(sbh)) {
		/*
		 * Oh, dear.  A previous attempt to write the
		 * superblock failed.  This could happen because the
		 * USB device was yanked out.  Or it could happen to
		 * be a transient write error and maybe the block will
		 * be remapped.  Nothing we can do but to retry the
		 * write and hope for the best.
		 */
		ext4_msg(sb, KERN_ERR, "previous I/O error to "
		       "superblock detected");
		clear_buffer_write_io_error(sbh);
		set_buffer_uptodate(sbh);
	}
	mark_buffer_dirty(sbh);
	if (sync) {
		unlock_buffer(sbh);
		error = __sync_dirty_buffer(sbh,
			REQ_SYNC | (test_opt(sb, BARRIER) ? REQ_FUA : 0));
		if (buffer_write_io_error(sbh)) {
			ext4_msg(sb, KERN_ERR, "I/O error while writing "
			       "superblock");
			clear_buffer_write_io_error(sbh);
			set_buffer_uptodate(sbh);
		}
	}
	return error;
}

/*
 * The del_gendisk() function uninitializes the disk-specific data
 * structures, including the bdi structure, without telling anyone
 * else.  Once this happens, any attempt to call mark_buffer_dirty()
 * (for example, by ext4_commit_super), will cause a kernel OOPS.
 * This is a kludge to prevent these oops until we can put in a proper
 * hook in del_gendisk() to inform the VFS and file system layers.
 */
static int block_device_ejected(struct super_block *sb)
{
	struct inode *bd_inode = sb->s_bdev->bd_inode;
	struct backing_dev_info *bdi = inode_to_bdi(bd_inode);

	return bdi->dev == NULL;
}

static __le32 ext4_superblock_csum(struct super_block *sb,
				   struct ext4_super_block *es)
{
	struct ext4_sb_info *sbi = EXT4_SB(sb);
	int offset = offsetof(struct ext4_super_block, s_checksum);
	__u32 csum;

	csum = ext4_chksum(sbi, ~0, (char *)es, offset);

	return cpu_to_le32(csum);
}

void ext4_superblock_csum_set(struct super_block *sb)
{
	struct ext4_super_block *es = EXT4_SB(sb)->s_es;

	if (!ext4_has_metadata_csum(sb))
		return;

	es->s_checksum = ext4_superblock_csum(sb, es);
}

void __ext4_msg(struct super_block *sb,
		const char *prefix, const char *fmt, ...)
{
	struct va_format vaf;
	va_list args;

	if (!___ratelimit(&(EXT4_SB(sb)->s_msg_ratelimit_state), "EXT4-fs"))
		return;

	va_start(args, fmt);
	vaf.fmt = fmt;
	vaf.va = &args;
	printk("%sEXT4-fs (%s): %pV\n", prefix, sb->s_id, &vaf);
	va_end(args);
}

/*
 * This works like sb_bread() except it uses ERR_PTR for error
 * returns.  Currently with sb_bread it's impossible to distinguish
 * between ENOMEM and EIO situations (since both result in a NULL
 * return.
 */
struct buffer_head *
ext4_sb_bread(struct super_block *sb, sector_t block, int op_flags)
{
	struct buffer_head *bh = sb_getblk(sb, block);

	if (bh == NULL)
		return ERR_PTR(-ENOMEM);
	if (buffer_uptodate(bh))
		return bh;
	ll_rw_block(REQ_OP_READ, REQ_META | op_flags, 1, &bh);
	wait_on_buffer(bh);
	if (buffer_uptodate(bh))
		return bh;
	put_bh(bh);
	return ERR_PTR(-EIO);
}