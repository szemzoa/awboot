/*
 * Copyright (c) 2009 Travis Geiselbrecht
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
#include <stdlib.h>
#include <debug.h>
#include "bio.h"
#include "list.h"

/* default implementation is to use the read_block hook to 'deblock' the device */
static ssize_t bio_default_read(struct bdev *dev, void *_buf, off_t offset, size_t len)
{
	uint8_t *buf		= (uint8_t *)_buf;
	ssize_t	 bytes_read = 0;
	bnum_t	 block;
	int		 err = 0;
	STACKBUF_DMA_ALIGN(temp, dev->block_size); // temporary buffer for partial block transfers

	/* find the starting block */
	block = offset / dev->block_size;

	/* handle partial first block */
	if ((offset % dev->block_size) != 0) {
		/* read in the block */
		err = bio_read_block(dev, temp, block, 1);
		if (err < 0) {
			goto err;
		}

		/* copy what we need */
		size_t block_offset = offset % dev->block_size;
		size_t tocopy		= MIN(dev->block_size - block_offset, len);
		memcpy(buf, temp + block_offset, tocopy);

		/* increment our buffers */
		buf += tocopy;
		len -= tocopy;
		bytes_read += tocopy;
		block++;
	}

	/* handle middle blocks */
	if (len >= dev->block_size) {
		/* do the middle reads */
		size_t block_count = len / dev->block_size;
		err				   = bio_read_block(dev, buf, block, block_count);
		if (err < 0) {
			goto err;
		}
		/* increment our buffers */
		size_t bytes = block_count * dev->block_size;

		buf += bytes;
		len -= bytes;
		bytes_read += bytes;
		block += block_count;
	}

	/* handle partial last block */
	if (len > 0) {
		/* read the block */
		err = bio_read_block(dev, temp, block, 1);
		if (err < 0) {
			goto err;
		}

		/* copy the partial block from our temp buffer */
		memcpy(buf, temp, len);

		bytes_read += len;
	}

err:
	/* return error or bytes read */
	return (err >= 0) ? bytes_read : err;
}

static ssize_t bio_default_write(struct bdev *dev, const void *_buf, off_t offset, size_t len)
{
	return -1;
}

static ssize_t bio_default_read_block(struct bdev *dev, void *buf, bnum_t block, uint count)
{
	message("%s no reasonable default operation\n", __PRETTY_FUNCTION__);
	return 0;
}

ssize_t bio_read(bdev_t *dev, void *buf, off_t offset, size_t len)
{
	/* range check */
	if (offset < 0)
		return -1;
	if (offset >= dev->size)
		return 0;
	if (len == 0)
		return 0;
	if (offset + len > dev->size)
		len = dev->size - offset;

	return dev->read(dev, buf, offset, len);
}

ssize_t bio_read_block(bdev_t *dev, void *buf, bnum_t block, uint count)
{
	/* range check */
	if (block > dev->block_count)
		return 0;
	if (count == 0)
		return 0;
	if (block + count > dev->block_count)
		count = dev->block_count - block;

	return dev->read_block(dev, buf, block, count);
}

ssize_t bio_write(bdev_t *dev, const void *buf, off_t offset, size_t len)
{
	return -1;
}

void bio_initialize_bdev(bdev_t *dev, size_t block_size, bnum_t block_count)
{
	dev->block_size	 = block_size;
	dev->block_count = block_count;
	dev->size		 = (off_t)block_count * block_size;
	dev->label		 = NULL;
	dev->is_gpt		 = false;
	dev->is_subdev	 = false;

	/* set up the default hooks, the sub driver should override the block operations at least */
	dev->read		= bio_default_read;
	dev->read_block = bio_default_read_block;
	dev->close		= NULL;
}
