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
#include "debug.h"
#include <string.h>
#include <stdlib.h>
#include "bio.h"
#include "partition.h"

struct chs {
	uint8_t c;
	uint8_t h;
	uint8_t s;
} __attribute__((packed));

struct mbr_part {
	uint8_t	   status;
	struct chs start;
	uint8_t	   type;
	struct chs end;
	uint32_t   lba_start;
	uint32_t   lba_length;
} __attribute__((packed));

static status_t validate_mbr_partition(bdev_t *dev, const struct mbr_part *part)
{
	/* check for invalid types */
	if (part->type == 0)
		return -1;
	/* check for invalid status */
	if (part->status != 0x80 && part->status != 0x00)
		return -1;

	/* make sure the range fits within the device */
	if (part->lba_start >= dev->block_count)
		return -1;
	if ((part->lba_start + part->lba_length) > dev->block_count)
		return -1;

	/* that's about all we can do, MBR has no other good way to see if it's valid */
	return 0;
}

int partition_publish(bdev_t *dev, off_t offset)
{
	struct mbr_part part[4];
	unsigned int	i;

	// get a dma aligned and padded block to read info
	STACKBUF_DMA_ALIGN(buf, dev->block_size);

	/* sniff for MBR partition types */
	if (bio_read(dev, buf, offset, 512) < 0)
		return -1;

	/* look for the aa55 tag */
	if (buf[510] != 0x55 || buf[511] != 0xaa)
		return -1;

	/* see if a partition table makes sense here */
	memcpy(part, buf + 446, sizeof(part));
	/*
		message("mbr partition table dump:\r\n");
		for (i=0; i < 4; i++)
			message("\t%i: status 0x%hhx, type 0x%hhx, start 0x%lx, len 0x%lx\r\n", i, part[i].status, part[i].type,
	   part[i].lba_start, part[i].lba_length);
	*/
	/* validate each of the partition entries */
	for (i = 0; i < 4; i++) {
		if (validate_mbr_partition(dev, &part[i]) >= 0 && part[i].type == 0x83) {
			debug("EXT2FS: found on partition %u\r\n", i);
			dev->lba_start = part[i].lba_start;
			return 0;
		}
	}

	return -1;
}
