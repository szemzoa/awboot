/*
 * Copyright (c) 2007 Travis Geiselbrecht
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
#include "ext2_priv.h"
#include "debug.h"

size_t strlcpy(char *dst, char const *src, size_t s)
{
	size_t i = 0;

	if (!s) {
		return strlen(src);
	}

	for (i = 0; ((i < s - 1) && src[i]); i++) {
		dst[i] = src[i];
	}

	dst[i] = 0;

	return i + strlen(src + i);
}

/* read in the dir, look for the entry */
static int ext2_dir_lookup(ext2_t *ext2, struct ext2_inode *dir_inode, const char *name, inodenum_t *inum)
{
	uint	 file_blocknum;
	int		 err;
	uint8_t *buf;
	size_t	 namelen = strlen(name);

	if (!S_ISDIR(dir_inode->i_mode))
		return -1;

	buf = NutHeapAlloc(EXT2_BLOCK_SIZE(ext2->sb));

	file_blocknum = 0;
	for (;;) {
		/* read in the offset */
		err =
			ext2_read_inode(ext2, dir_inode, buf, file_blocknum * EXT2_BLOCK_SIZE(ext2->sb), EXT2_BLOCK_SIZE(ext2->sb));
		if (err <= 0) {
			NutHeapFree(buf);
			return -1;
		}

		/* walk through the directory entries, looking for the one that matches */
		struct ext2_dir_entry_2 *ent;
		uint					 pos = 0;
		while (pos < EXT2_BLOCK_SIZE(ext2->sb)) {
			ent = (struct ext2_dir_entry_2 *)&buf[pos];

			/* sanity check the record length */
			if (LE16(ent->rec_len) == 0)
				break;

			if (ent->name_len == namelen && memcmp(name, ent->name, ent->name_len) == 0) {
				// match
				*inum = LE32(ent->inode);
				NutHeapFree(buf);
				return 1;
			}

			pos += ROUNDUP(LE16(ent->rec_len), 4);
		}

		file_blocknum++;

		/* sanity check the directory. 4MB should be enough */
		if (file_blocknum > 1024) {
			NutHeapFree(buf);
			return -1;
		}
	}
}

/* note, trashes path */
static int ext2_walk(ext2_t *ext2, char *path, struct ext2_inode *start_inode, inodenum_t *inum, int recurse)
{
	char			 *ptr;
	struct ext2_inode inode;
	struct ext2_inode dir_inode;
	int				  err;
	bool			  done;

	if (recurse > 4)
		return -1;

	/* chew up leading slashes */
	ptr = &path[0];
	while (*ptr == '/')
		ptr++;

	done = false;
	memcpy(&dir_inode, start_inode, sizeof(struct ext2_inode));
	while (!done) {
		/* process the first component */
		char *next_sep = strchr(ptr, '/');
		if (next_sep) {
			/* terminate the next component, giving us a substring */
			*next_sep = 0;
		} else {
			/* this is the last component */
			done = true;
		}

		/* do the lookup on this component */
		err = ext2_dir_lookup(ext2, &dir_inode, ptr, inum);
		if (err < 0) {
			return err;
		}

	nextcomponent:

		/* load the next inode */
		err = ext2_load_inode(ext2, *inum, &inode);
		if (err < 0) {
			return err;
		}

		/* is it a symlink? */
		if (S_ISLNK(inode.i_mode)) {
			char link[512];

			err = ext2_read_link(ext2, &inode, link, sizeof(link));
			if (err < 0)
				return err;

			/* recurse, parsing the link */
			if (link[0] == '/') {
				/* link starts with '/', so start over again at the rootfs */
				err = ext2_walk(ext2, link, &ext2->root_inode, inum, recurse + 1);
			} else {
				err = ext2_walk(ext2, link, &dir_inode, inum, recurse + 1);
			}

			if (err < 0)
				return err;

			/* if we weren't done with our path parsing, start again with the result of this recurse */
			if (!done) {
				goto nextcomponent;
			}
		} else if (S_ISDIR(inode.i_mode)) {
			/* for the next cycle, point the dir inode at our new directory */
			memcpy(&dir_inode, &inode, sizeof(struct ext2_inode));
		} else {
			if (!done) {
				/* we aren't done and this walked over a nondir, abort */
				return -1;
			}
		}

		if (!done) {
			/* move to the next seperator */
			ptr = next_sep + 1;

			/* consume multiple seperators */
			while (*ptr == '/')
				ptr++;
		}
	}

	return 0;
}

/* do a path parse, looking up each component */
int ext2_lookup(ext2_t *ext2, const char *_path, inodenum_t *inum)
{
	char path[512];

	strlcpy(path, _path, sizeof(path));

	return ext2_walk(ext2, path, &ext2->root_inode, inum, 1);
}

// © 2019 Mis012 - SPDX-License-Identifier: GPL-3.0
// from here till the end of file

status_t ext2_open_directory(fscookie *cookie, const char *path, dircookie **dircookie)
{
	off_t		entry_len = 0;
	ext2_dir_t *dir		  = NutHeapAlloc(sizeof(ext2_dir_t));
	memset(dir, 0, sizeof(ext2_dir_t));
	if (!dir) {
		return -1;
	}
	int ret;

	ret = ext2_open_file(cookie, (path[0] == 0 ? "/." : path), (filecookie **)&dir->file); // fails if path=""
	if (ret < 0) {
		NutHeapFree(dir);
		return ret;
	}

	if (!S_ISDIR(dir->file->inode.i_mode)) {
		NutHeapFree(dir);
		return -1;
	}

	entry_len = ext2_file_len(dir->file->ext2, &dir->file->inode);

	dir->offset = 0;
	dir->length = entry_len;

	*dircookie = dir;

	return 0;
}

status_t ext2_read_directory(dircookie *dircookie, struct dirent *ent)
{
	struct ext2_dir_entry_2 direntry;
	int						ret;

	ext2_dir_t *dir = (ext2_dir_t *)dircookie;

	if (dir->offset >= dir->length)
		return -1;

	ret = ext2_read_inode(dir->file->ext2, &dir->file->inode, &direntry, dir->offset, sizeof(struct ext2_dir_entry_2));
	if (ret < 0)
		return ret;

	memcpy(ent->name, direntry.name, direntry.name_len);
	ent->name[direntry.name_len] = '\0';

	dir->offset += direntry.rec_len;

	return 0;
}

status_t ext2_close_directory(dircookie *dircookie)
{
	ext2_dir_t *dir = (ext2_dir_t *)dircookie;

	ext2_close_file(dir->file);
	NutHeapFree(dir);
	return 0;
}
