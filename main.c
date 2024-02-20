#include "main.h"
#include "fdt.h"
#include "ff.h"
#include "sunxi_gpio.h"
#include "sunxi_sdhci.h"
#include "sunxi_spi.h"
#include "sunxi_clk.h"
#include "sunxi_dma.h"
#include "sdmmc.h"
#include "arm32.h"
#include "debug.h"
#include "board.h"
#include "console.h"
#include "barrier.h"
#include "nutheap.h"
#include "bio.h"
#include "ext2_fs.h"
#include "ext2_priv.h"
#include "partition.h"

extern void			 board_init(void);
extern unsigned long sunxi_dram_init(void);

image_info_t image;

/* Linux zImage Header */
#define LINUX_ZIMAGE_MAGIC 0x016f2818
typedef struct {
	unsigned int code[9];
	unsigned int magic;
	unsigned int start;
	unsigned int end;
} linux_zimage_header_t;

static int boot_image_setup(unsigned char *addr, unsigned int *entry)
{
	linux_zimage_header_t *zimage_header = (linux_zimage_header_t *)addr;

	if (zimage_header->magic == LINUX_ZIMAGE_MAGIC) {
		*entry = ((unsigned int)addr + zimage_header->start);
		return 0;
	}

	error("unsupported kernel image\r\n");

	return -1;
}

#if defined(CONFIG_BOOT_SDCARD) || defined(CONFIG_BOOT_MMC)
#define CHUNK_SIZE 0x20000
static int fatfs_loadimage(char *filename, BYTE *dest)
{
	FIL					  file;
	UINT				  byte_to_read = CHUNK_SIZE;
	UINT				  byte_read;
	UINT				  total_read = 0;
	FRESULT				  fret;
	int					  ret;
	uint32_t UNUSED_DEBUG start, time;

	fret = f_open(&file, filename, FA_OPEN_EXISTING | FA_READ);
	if (fret != FR_OK) {
		error("FATFS: open, filename: [%s]: error %d\r\n", filename, fret);
		ret = -1;
		goto open_fail;
	}

	start = time_ms();

	do {
		byte_read = 0;
		fret	  = f_read(&file, (void *)(dest), byte_to_read, &byte_read);
		dest += byte_to_read;
		total_read += byte_read;
	} while (byte_read >= byte_to_read && fret == FR_OK);

	time = time_ms() - start + 1;

	if (fret != FR_OK) {
		error("FATFS: read: error %d\r\n", fret);
		ret = -1;
		goto read_fail;
	}
	ret = 0;

read_fail:
	fret = f_close(&file);

	debug("FATFS: read in %" PRIu32 "ms at %.2fMB/S\r\n", time, (f32)(total_read / time) / 1024.0f);

open_fail:
	return ret;
}

#ifdef CONFIG_ENABLE_EXT2FS
struct bdev block_device;

ssize_t sunxi_read_block(struct bdev *bdev, void *buf, bnum_t block, uint count)
{
	if (sdmmc_blk_read(&card0, buf, (uint64_t)(block + bdev->lba_start), (uint64_t)count) != count) {
		message("%s read error\r\n", __FUNCTION__);
		return -1;
	}

	return 0;
}

static int load_sdcard(image_info_t *image)
{
	int				 ret;
	u32 UNUSED_DEBUG start;
	fscookie		 *fsc;
	filecookie	   *fic;
	struct file_stat fstat;

#if defined(CONFIG_SDMMC_SPEED_TEST_SIZE) && LOG_LEVEL >= LOG_DEBUG
	u32 test_time;
	start = time_ms();
	sdmmc_blk_read(&card0, (u8 *)(SDRAM_BASE), 0, CONFIG_SDMMC_SPEED_TEST_SIZE);
	test_time = time_ms() - start;
	debug("SDMMC: speedtest %uKB in %" PRIu32 "ms at %" PRIu32 "KB/S\r\n", (CONFIG_SDMMC_SPEED_TEST_SIZE * 512) / 1024,
		  test_time, (CONFIG_SDMMC_SPEED_TEST_SIZE * 512) / test_time);
#endif // SDMMC_SPEED_TEST

	bio_initialize_bdev(&block_device, 512, card0.card.capacity);
	block_device.read_block = sunxi_read_block;

	start = time_ms();
	/* mount fs */
	if (partition_publish(&block_device, (uint64_t)0) != 0) {
		error("EXT2FS: no partition found\r\n");
		return 1;
	}

	if (ext2_mount(&block_device, &fsc) != 0) {
		error("EXT2FS: mount error\r\n");
		return -1;
	} else {
		debug("EXT2FS: mount OK\r\n");
	}

	info("EXT2FS: read %s addr=%x\r\n", image->of_filename, (unsigned int)image->of_dest);
	ext2_open_file(fsc, image->of_filename, &fic);
	ext2_stat_file(fic, &fstat);
	ext2_read_file(fic, (void *)image->of_dest, 0, fstat.size);
	ext2_close_file(fic);

	info("EXT2FS: read %s addr=%x\r\n", image->filename, (unsigned int)image->dest);
	ext2_open_file(fsc, image->filename, &fic);
	ext2_stat_file(fic, &fstat);
	ext2_read_file(fic, (void *)image->dest, 0, fstat.size);
	ext2_close_file(fic);

	/* umount fs */
	ext2_unmount(fsc);
	debug("EXT2FS: unmount OK\r\n");
	debug("EXT2FS: done in %" PRIu32 "ms\r\n", time_ms() - start);

	return 0;
}

#else

static int load_sdcard(image_info_t *image)
{
	FATFS			 fs;
	FRESULT			 fret;
	int				 ret;
	u32 UNUSED_DEBUG start;

#if defined(CONFIG_SDMMC_SPEED_TEST_SIZE) && LOG_LEVEL >= LOG_DEBUG
	u32				 test_time;
	start = time_ms();
	sdmmc_blk_read(&card0, (u8 *)(SDRAM_BASE), 0, CONFIG_SDMMC_SPEED_TEST_SIZE);
	test_time = time_ms() - start;
	debug("SDMMC: speedtest %uKB in %" PRIu32 "ms at %" PRIu32 "KB/S\r\n", (CONFIG_SDMMC_SPEED_TEST_SIZE * 512) / 1024,
		  test_time, (CONFIG_SDMMC_SPEED_TEST_SIZE * 512) / test_time);
#endif // SDMMC_SPEED_TEST

	start = time_ms();
	/* mount fs */
	fret = f_mount(&fs, "", 1);
	if (fret != FR_OK) {
		error("FATFS: mount error: %d\r\n", fret);
		return -1;
	} else {
		debug("FATFS: mount OK\r\n");
	}

	info("FATFS: read %s addr=%x\r\n", image->of_filename, (unsigned int)image->of_dest);
	ret = fatfs_loadimage(image->of_filename, image->of_dest);
	if (ret)
		return ret;

	info("FATFS: read %s addr=%x\r\n", image->filename, (unsigned int)image->dest);
	ret = fatfs_loadimage(image->filename, image->dest);
	if (ret)
		return ret;

	/* umount fs */
	fret = f_mount(0, "", 0);
	if (fret != FR_OK) {
		error("FATFS: unmount error %d\r\n", fret);
		return -1;
	} else {
		debug("FATFS: unmount OK\r\n");
	}
	debug("FATFS: done in %" PRIu32 "ms\r\n", time_ms() - start);

	return 0;
}
#endif
#endif

#ifdef CONFIG_BOOT_SPINAND
int load_spi_nand(sunxi_spi_t *spi, image_info_t *image)
{
	linux_zimage_header_t *hdr;
	unsigned int		   size;
	uint64_t UNUSED_DEBUG  start, time;

	if (spi_nand_detect(spi) != 0)
		return -1;

	/* get dtb size and read */
	spi_nand_read(spi, image->of_dest, CONFIG_SPINAND_DTB_ADDR, (uint32_t)sizeof(boot_param_header_t));

	if (of_get_magic_number(image->of_dest) != OF_DT_MAGIC) {
		error("SPI-NAND: DTB verification failed\r\n");
		return -1;
	}

	size = of_get_dt_total_size(image->of_dest);
	debug("SPI-NAND: dt blob: Copy from 0x%08x to 0x%08lx size:0x%08x\r\n", CONFIG_SPINAND_DTB_ADDR,
		  (uint32_t)image->of_dest, size);
	start = time_us();
	spi_nand_read(spi, image->of_dest, CONFIG_SPINAND_DTB_ADDR, (uint32_t)size);

	time = time_us() - start;
	info("SPI-NAND: read dt blob of size %u at %.2fMB/S\r\n", size, (f32)(size / time));

	/* get kernel size and read */
	spi_nand_read(spi, image->dest, CONFIG_SPINAND_KERNEL_ADDR, (uint32_t)sizeof(linux_zimage_header_t));

	hdr = (linux_zimage_header_t *)image->dest;
	if (hdr->magic != LINUX_ZIMAGE_MAGIC) {
		debug("SPI-NAND: zImage verification failed\r\n");
		return -1;
	}
	size = hdr->end - hdr->start;
	debug("SPI-NAND: Image: Copy from 0x%08x to 0x%08lx size:0x%08x\r\n", CONFIG_SPINAND_KERNEL_ADDR,
		  (uint32_t)image->dest, size);
	start = time_us();
	spi_nand_read(spi, image->dest, CONFIG_SPINAND_KERNEL_ADDR, (uint32_t)size);

	time = time_us() - start;
	info("SPI-NAND: read Image of size %u at %.2fMB/S\r\n", size, (f32)(size / time));

	return 0;
}
#endif

int main(void)
{
	uint32_t msize;

	board_init();
	sunxi_clk_init();

	message("\r\n");
	info("AWBoot r%" PRIu32 " starting...\r\n", (u32)BUILD_REVISION);

	msize = sunxi_dram_init() >> 20;
	info("DRAM size: %luMB\r\n", msize);

	//	arm32_mmu_enable(SDRAM_BASE, msize);

	/* Initialize our heap memory. */
	NutHeapAdd((void *)CONFIG_NUTHEAP_BASE, CONFIG_NUTHEAP_SIZE & ~3);

#ifdef CONFIG_ENABLE_CONSOLE
	console_init(&USART_DBG);
	console_handler(CONSOLE_NO_TIMEOUT);
#endif

	unsigned int entry_point = 0;
	void (*kernel_entry)(int zero, int arch, unsigned int params);

#ifdef CONFIG_ENABLE_CPU_FREQ_DUMP
	sunxi_clk_dump();
#endif

	memset(&image, 0, sizeof(image_info_t));

	image.of_dest = (u8 *)CONFIG_DTB_LOAD_ADDR;
	image.dest	  = (u8 *)CONFIG_KERNEL_LOAD_ADDR;

#if defined(CONFIG_BOOT_SDCARD) || defined(CONFIG_BOOT_MMC)

	strcpy(image.filename, CONFIG_KERNEL_FILENAME);
	strcpy(image.of_filename, CONFIG_DTB_FILENAME);

	if (sunxi_sdhci_init(&sdhci0) != 0) {
		fatal("SMHC: %s controller init failed\r\n", sdhci0.name);
	} else {
		info("SMHC: %s controller v%" PRIx32 " initialized\r\n", sdhci0.name, sdhci0.reg->vers);
	}
	if (sdmmc_init(&card0, &sdhci0) != 0) {
#ifdef CONFIG_BOOT_SPINAND
		warning("SMHC: init failed, trying SPI\r\n");
		goto _spi;
#else
		fatal("SMHC: init failed\r\n");
#endif
	}

#ifdef CONFIG_BOOT_SPINAND
	if (load_sdcard(&image) != 0) {
		warning("SMHC: loading failed, trying SPI\r\n");
	} else {
		goto _boot;
	}
#else
	if (load_sdcard(&image) != 0) {
		fatal("SMHC: card load failed\r\n");
	} else {
		goto _boot;
	}
#endif // CONFIG_SPI_NAND
#endif

#ifdef CONFIG_BOOT_SPINAND
#if defined(CONFIG_BOOT_SDCARD) || defined(CONFIG_BOOT_MMC)
_spi:
#endif
	dma_init();
	dma_test();

	debug("SPI: init\r\n");
	if (sunxi_spi_init(&sunxi_spi0) != 0) {
		fatal("SPI: init failed\r\n");
	}

	if (load_spi_nand(&sunxi_spi0, &image) != 0) {
		fatal("SPI-NAND: loading failed\r\n");
	}

	sunxi_spi_disable(&sunxi_spi0);
	dma_exit();

#endif // CONFIG_SPI_NAND

#if defined(CONFIG_BOOT_SDCARD) || defined(CONFIG_BOOT_MMC)
_boot:
#endif
	if (boot_image_setup((unsigned char *)image.dest, &entry_point)) {
		fatal("boot setup failed\r\n");
	}

#ifdef CONFIG_CMD_LINE_ARGS
	info("setup bootargs: %s\r\n", CONFIG_CMD_LINE_ARGS);
	fixup_chosen_node(image.of_dest, CONFIG_CMD_LINE_ARGS);
#endif

	info("booting linux...\r\n");

	arm32_mmu_disable();
	arm32_dcache_disable();
	arm32_icache_disable();
	arm32_interrupt_disable();

	kernel_entry = (void (*)(int, int, unsigned int))entry_point;
	kernel_entry(0, ~0, (unsigned int)image.of_dest);

	return 0;
}
