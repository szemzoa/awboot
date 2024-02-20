/*
 * Copyright (c) 2007-2015 Travis Geiselbrecht
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files
 * (the "Software"), to deal in the Software without restriction,
 * including without limitation the rights to use, copy, modify, merge,
 * publish, distribute, sublicense, and/or sell copies of the Software,
 * and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include "debug.h"
#include "fs.h"
#include "ext2_priv.h"

static void endian_swap_superblock(struct ext2_super_block *sb)
{
	LE32SWAP(sb->s_inodes_count);
	LE32SWAP(sb->s_blocks_count);
	LE32SWAP(sb->s_r_blocks_count);
	LE32SWAP(sb->s_free_blocks_count);
	LE32SWAP(sb->s_free_inodes_count);
	LE32SWAP(sb->s_first_data_block);
	LE32SWAP(sb->s_log_block_size);
	LE32SWAP(sb->s_log_frag_size);
	LE32SWAP(sb->s_blocks_per_group);
	LE32SWAP(sb->s_frags_per_group);
	LE32SWAP(sb->s_inodes_per_group);
	LE32SWAP(sb->s_mtime);
	LE32SWAP(sb->s_wtime);
	LE16SWAP(sb->s_mnt_count);
	LE16SWAP(sb->s_max_mnt_count);
	LE16SWAP(sb->s_magic);
	LE16SWAP(sb->s_state);
	LE16SWAP(sb->s_errors);
	LE16SWAP(sb->s_minor_rev_level);
	LE32SWAP(sb->s_lastcheck);
	LE32SWAP(sb->s_checkinterval);
	LE32SWAP(sb->s_creator_os);
	LE32SWAP(sb->s_rev_level);
	LE16SWAP(sb->s_def_resuid);
	LE16SWAP(sb->s_def_resgid);
	LE32SWAP(sb->s_first_ino);
	LE16SWAP(sb->s_inode_size);
	LE16SWAP(sb->s_block_group_nr);
	LE32SWAP(sb->s_feature_compat);
	LE32SWAP(sb->s_feature_incompat);
	LE32SWAP(sb->s_feature_ro_compat);
	LE32SWAP(sb->s_algorithm_usage_bitmap);

	/* ext3 journal stuff */
	LE32SWAP(sb->s_journal_inum);
	LE32SWAP(sb->s_journal_dev);
	LE32SWAP(sb->s_last_orphan);
	LE32SWAP(sb->s_default_mount_opts);
	LE32SWAP(sb->s_first_meta_bg);
}

static void endian_swap_inode(struct ext2_inode *inode)
{
	LE16SWAP(inode->i_mode);
	LE16SWAP(inode->i_uid_low);
	LE32SWAP(inode->i_size);
	LE32SWAP(inode->i_atime);
	LE32SWAP(inode->i_ctime);
	LE32SWAP(inode->i_mtime);
	LE32SWAP(inode->i_dtime);
	LE16SWAP(inode->i_gid_low);
	LE16SWAP(inode->i_links_count);
	LE32SWAP(inode->i_blocks);
	LE32SWAP(inode->i_flags);

	// leave block pointers/symlink data alone

	LE32SWAP(inode->i_generation);
	LE32SWAP(inode->i_file_acl);
	LE32SWAP(inode->i_dir_acl);
	LE32SWAP(inode->i_faddr);

	LE16SWAP(inode->i_uid_high);
	LE16SWAP(inode->i_gid_high);
}

static void endian_swap_group_desc(struct ext2_group_desc *gd)
{
	LE32SWAP(gd->bg_block_bitmap);
	LE32SWAP(gd->bg_inode_bitmap);
	LE32SWAP(gd->bg_inode_table);
	LE16SWAP(gd->bg_free_blocks_count);
	LE16SWAP(gd->bg_free_inodes_count);
	LE16SWAP(gd->bg_used_dirs_count);
}

status_t ext2_mount(bdev_t *dev, fscookie **cookie)
{
	int err;

	ext2_t *ext2 = NutHeapAlloc(sizeof(ext2_t));
	ext2->dev	 = dev;

	err = bio_read(dev, &ext2->sb, (uint64_t)1024, sizeof(struct ext2_super_block));

	if (err < 0)
		goto err;

	endian_swap_superblock(&ext2->sb);

	/* see if the superblock is good */
	if (ext2->sb.s_magic != EXT2_SUPER_MAGIC) {
		error("%s ext2 magic error 0x%x\r\n", __FUNCTION__, ext2->sb.s_magic);
		return -1;
	}

#if 0
    /* print some info */
    message("rev level %ld\r\n", ext2->sb.s_rev_level);
    message("compat features 0x%lx\r\n", ext2->sb.s_feature_compat);
    message("incompat features 0x%lx\r\n", ext2->sb.s_feature_incompat);
    message("ro compat features 0x%lx\r\n", ext2->sb.s_feature_ro_compat);
    message("block size %ld\r\n", EXT2_BLOCK_SIZE(ext2->sb));
    message("inode size %d\r\n", EXT2_INODE_SIZE(ext2->sb));
    message("block count %ld\r\n", ext2->sb.s_blocks_count);
    message("blocks per group %ld\r\n", ext2->sb.s_blocks_per_group);
    message("group count %d\r\n", ext2->s_group_count);
    message("inodes per group %ld\r\n", ext2->sb.s_inodes_per_group);
#endif

	/* calculate group count, rounded up */
	ext2->s_group_count = (ext2->sb.s_blocks_count + ext2->sb.s_blocks_per_group - 1) / ext2->sb.s_blocks_per_group;

	/* we only support dynamic revs */
	if (ext2->sb.s_rev_level > EXT2_DYNAMIC_REV) {
		err = -2;
		return err;
	}

	/* make sure it doesn't have any ro features we don't support */
	if (ext2->sb.s_feature_ro_compat & ~(EXT2_FEATURE_RO_COMPAT_SPARSE_SUPER | EXT2_FEATURE_RO_COMPAT_LARGE_FILE)) {
		err = -3;
		return err;
	}

	/* read in all the group descriptors */
	ext2->gd = NutHeapAlloc(sizeof(struct ext2_group_desc) * ext2->s_group_count);
	err		 = bio_read(ext2->dev, (void *)ext2->gd, (EXT2_BLOCK_SIZE(ext2->sb) == 4096) ? 4096 : 2048,
						sizeof(struct ext2_group_desc) * ext2->s_group_count);
	if (err < 0) {
		err = -4;
		return err;
	}

#if 0
    int i;
    for (i=0; i < ext2->s_group_count; i++) {
        endian_swap_group_desc(&ext2->gd[i]);

        message("group %d:\n", i);
        message("\tblock bitmap %d\r\n", ext2->gd[i].bg_block_bitmap);
        message("\tinode bitmap %d\r\n", ext2->gd[i].bg_inode_bitmap);
        message("\tinode table %d\r\n", ext2->gd[i].bg_inode_table);
        message("\tfree blocks %d\r\n", ext2->gd[i].bg_free_blocks_count);
        message("\tfree inodes %d\r\n", ext2->gd[i].bg_free_inodes_count);
        message("\tused dirs %d\r\n", ext2->gd[i].bg_used_dirs_count);
    }
#endif

	/* initialize the block cache */
	ext2->cache = bcache_create(ext2->dev, EXT2_BLOCK_SIZE(ext2->sb), 4);

	/* load the first inode */
	err = ext2_load_inode(ext2, EXT2_ROOT_INO, &ext2->root_inode);
	if (err < 0)
		goto err;

	debug("EXT2FS mount success\r\n");

	*cookie = (fscookie *)ext2;

	return 0;

err:
	debug("EXT2FS mount failed\r\n");

	NutHeapFree(ext2);
	return err;
}

status_t ext2_unmount(fscookie *cookie)
{
	// free it up
	ext2_t *ext2 = (ext2_t *)cookie;

	bcache_destroy(ext2->cache);
	NutHeapFree(ext2->gd);
	NutHeapFree(ext2);

	return 0;
}

static void get_inode_addr(ext2_t *ext2, inodenum_t num, blocknum_t *block, size_t *block_offset)
{
	num--;

	uint32_t group = num / ext2->sb.s_inodes_per_group;

	// calculate the start of the inode table for the group it's in
	*block = ext2->gd[group].bg_inode_table;

	// add the offset of the inode within the group
	size_t offset = (num % EXT2_INODES_PER_GROUP(ext2->sb)) * EXT2_INODE_SIZE(ext2->sb);
	*block_offset = offset % EXT2_BLOCK_SIZE(ext2->sb);
	*block += offset / EXT2_BLOCK_SIZE(ext2->sb);
}

int ext2_load_inode(ext2_t *ext2, inodenum_t num, struct ext2_inode *inode)
{
	int err;

	blocknum_t bnum;
	size_t	   block_offset;
	get_inode_addr(ext2, num, &bnum, &block_offset);

	/* get a pointer to the cache block */
	void *cache_ptr;
	err = bcache_get_block(ext2->cache, &cache_ptr, bnum);
	if (err < 0)
		return err;

	/* copy the inode out */
	memcpy(inode, (uint8_t *)cache_ptr + block_offset, sizeof(struct ext2_inode));

	/* put the cache block */
	bcache_put_block(ext2->cache, bnum);

	/* endian swap it */
	endian_swap_inode(inode);

	return 0;
}
