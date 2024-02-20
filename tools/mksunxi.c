#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>

#define __ALIGN_MASK(x, mask) (((x) + (mask)) & ~(mask))
#define ALIGN(x, a)			  __ALIGN_MASK((x), (typeof(x))(a)-1)

#if 0
static inline uint32_t __swab32(uint32_t x)
{
	return ( (x<<24) | (x>>24) | \
		((x & (uint32_t)0x0000ff00UL)<<8) | \
		((x & (uint32_t)0x00ff0000UL)>>8) );
}
#define cpu_to_le32(x) (__swab32((uint32_t)(x)))
#define le32_to_cpu(x) (__swab32((uint32_t)(x)))
#else
#define cpu_to_le32(x) (x)
#define le32_to_cpu(x) (x)
#endif

#define BROM_PAGE_SIZE 2048

struct boot_head_t {
	uint32_t instruction;
	uint8_t	 magic[8];
	uint32_t checksum;
	uint32_t length;
	uint8_t	 spl_signature[4];
	uint32_t fel_script_address;
	uint32_t fel_uenv_length;
	uint32_t dt_name_offset;
	uint32_t reserved1;
	uint32_t boot_media;
	uint32_t string_pool[13];
};

/*
 * BUG: the bootrom assumes all SPI flashes are 2KB page size.
 * If we want to boot from a device with larger page size, we need to adjust
 * the image in flash so that only the 1st 2KB of each page is used.
 */
static char *expand_pagesize(char *buffer, int *buflen, int pagesize)
{
	char *buffer2;
	int	  offset, multiple;

	multiple = pagesize / BROM_PAGE_SIZE;

	if (multiple == 1)
		return buffer;

	buffer2 = malloc(*buflen * multiple);
	memset(buffer2, 0, *buflen * multiple);

	for (offset = 0; offset < *buflen; offset += BROM_PAGE_SIZE) {
		memcpy(buffer2 + (offset * multiple), buffer + offset, BROM_PAGE_SIZE);
	}

	free(buffer);
	*buflen *= multiple;
	return buffer2;
}

int main(int argc, char *argv[])
{
	struct boot_head_t *h;
	FILE				 *fp;
	char				 *buffer;
	int					buflen, filelen;
	uint32_t			 *p;
	uint32_t			sum;
	int					i, l, loop, padding, pagesize;

	if (argc < 3 || argc > 4) {
		printf("Usage: mksunxi <bootloader> <padding> [pagesize]\n");
		return -1;
	}

	padding = atoi(argv[2]);
	printf("padding: %d\n", padding);

	if (argc >= 4) {
		pagesize = atoi(argv[3]);
		if ((pagesize < 1) || (pagesize & (BROM_PAGE_SIZE - 1))) {
			printf("pagesize must be multiple of 2048\n");
			exit(1);
		}
		printf("pagesize: %d\n", pagesize);
	} else {
		pagesize = BROM_PAGE_SIZE;
	}

	if (padding != 512 && padding != 8192) {
		printf("padding must be 512 (block devices) or 8192 (flash)\n");
		exit(1);
	}

	fp = fopen(argv[1], "r+b");
	if (fp == NULL) {
		printf("Open bootloader error\n");
		return -1;
	}
	fseek(fp, 0L, SEEK_END);
	filelen = ftell(fp);
	fseek(fp, 0L, SEEK_SET);

	if (filelen <= sizeof(struct boot_head_t)) {
		fclose(fp);
		printf("The size of bootloader too small\n");
		return -1;
	}

	buflen = ALIGN(filelen, padding);
	buffer = malloc(buflen);
	memset(buffer, 0, buflen);
	if (fread(buffer, 1, filelen, fp) != filelen) {
		printf("Can't read bootloader\n");
		free(buffer);
		fclose(fp);
		return -1;
	}

	h = (struct boot_head_t *)buffer;
	p = (uint32_t *)h;
	l = le32_to_cpu(h->length);
	printf("len: %u\n", l);
	l			= ALIGN(l, padding);
	h->length	= cpu_to_le32(l);
	h->checksum = cpu_to_le32(0x5F0A6C39);
	loop		= l >> 2;
	for (i = 0, sum = 0; i < loop; i++)
		sum += le32_to_cpu(p[i]);
	h->checksum = cpu_to_le32(sum);

	buffer = expand_pagesize(buffer, &buflen, pagesize);

	fseek(fp, 0L, SEEK_SET);
	if (fwrite(buffer, 1, buflen, fp) != buflen) {
		printf("Write bootloader error\n");
		free(buffer);
		fclose(fp);
		return -1;
	}

	free(buffer);
	fclose(fp);
	printf("The bootloader head has been fixed, spl size is %d bytes, flash size %d bytes.\r\n", l, buflen);
	return 0;
}
