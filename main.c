#include "common.h"
#include "dram.h"
#include "fdt.h"
#include "sunxi_clk.h"
#include "sunxi_wdg.h"
#include "sdmmc.h"
#include "arm32.h"
#include "psci.h"
#include "debug.h"
#include "board.h"
#include "barrier.h"
#include "loaders.h"
#include "sunxi_dma.h"
#include <asm/armv7.h>
#include <psci.h>
#include <asm/secure.h>

static bool range_in_sdram(uint32_t start, uint32_t size)
{
	const uint64_t base	 = (uint64_t)SDRAM_BASE;
	const uint64_t top	 = dram_get_top();
	const uint64_t start64 = (uint64_t)start;
	const uint64_t end	 = start64 + (uint64_t)size;

	return (start64 >= base) && (end <= top) && (end >= start64);
}

#if !CONFIG_BOOT_SDCARD && !CONFIG_BOOT_MMC && !CONFIG_BOOT_SPINAND
static inline uint32_t fel_mailbox_read(uint32_t addr)
{
	return *((volatile uint32_t *)addr);
}

static bool addr_in_sdram(uint32_t addr)
{
	const uint64_t base = (uint64_t)SDRAM_BASE;
	const uint64_t top	= dram_get_top();
	const uint64_t pos	= (uint64_t)addr;

	return (pos >= base) && (pos < top);
}

static void apply_fel_mailboxes(image_info_t *img)
{
	const uint32_t kernel_addr	 = fel_mailbox_read(CONFIG_MAIL_KERNEL_ADDR_ADDR);
	const uint32_t dtb_addr	  = fel_mailbox_read(CONFIG_MAIL_DTB_ADDR_ADDR);
	const uint32_t initrd_size	 = fel_mailbox_read(CONFIG_MAIL_INITRD_SIZE_ADDR);
	const uint32_t initrd_start   = fel_mailbox_read(CONFIG_MAIL_INITRD_START_ADDR);
	uint64_t	initrd_end_64 = (uint64_t)initrd_start + (uint64_t)initrd_size;

	if (kernel_addr != 0U) {
		if (!addr_in_sdram(kernel_addr)) {
			fatal("FEL: kernel addr 0x%08" PRIx32 " outside SDRAM\r\n", kernel_addr);
		}
		img->kernel_dest = (u8 *)kernel_addr;
	}

	if (dtb_addr != 0U) {
		if (!addr_in_sdram(dtb_addr)) {
			fatal("FEL: dtb addr 0x%08" PRIx32 " outside SDRAM\r\n", dtb_addr);
		}
		img->dtb_dest = (u8 *)dtb_addr;
	}

	if (initrd_size != 0U && initrd_start != 0U) {
		if (initrd_size > CONFIG_INITRAMFS_MAX_SIZE) {
			fatal("FEL: initrd size %" PRIu32 " exceeds max %" PRIu32 "\r\n",
						initrd_size, (uint32_t)CONFIG_INITRAMFS_MAX_SIZE);
		}

		if ((CONFIG_INITRD_ALIGNMENT != 0U) &&
			((initrd_start & (CONFIG_INITRD_ALIGNMENT - 1U)) != 0U)) {
			warning("FEL: initrd start 0x%08" PRIx32 " not %u-byte aligned\r\n",
				initrd_start, CONFIG_INITRD_ALIGNMENT);
		}
		if (!range_in_sdram(initrd_start, initrd_size)) {
			fatal("FEL: initrd 0x%08" PRIx32 "-0x%08" PRIx32 " outside SDRAM\r\n",
						initrd_start, (uint32_t)initrd_end_64);
		}
		if ((dtb_addr != 0U) && (initrd_end_64 > (uint64_t)dtb_addr)) {
			fatal("FEL: initrd overlaps DTB (initrd end 0x%08" PRIx32 ", dtb @ 0x%08" PRIx32 ")\r\n",
						(uint32_t)initrd_end_64, dtb_addr);
		}
		img->initrd_dest = (u8 *)initrd_start;
		img->initrd_size = initrd_size;
	} else if (initrd_size != 0U) {
		warning("FEL: initrd size provided but address missing, ignoring\r\n");
		img->initrd_dest = NULL;
		img->initrd_size = 0U;
	}

	info("FEL: kernel@0x%08" PRIx32 " dtb@0x%08" PRIx32 " initrd@0x%08" PRIx32 " (%" PRIu32 " bytes)\r\n",
			 (uint32_t)(uintptr_t)img->kernel_dest,
			 (uint32_t)(uintptr_t)img->dtb_dest,
			 (uint32_t)(uintptr_t)img->initrd_dest,
			 (uint32_t)(uintptr_t)img->initrd_size);
}
#endif

image_info_t image;
static char   kernel_filename[MAX_FILENAME_SIZE]  = CONFIG_KERNEL_FILENAME;
static char   dtb_filename[MAX_FILENAME_SIZE]	   = CONFIG_DTB_FILENAME;
static char   initrd_filename[MAX_FILENAME_SIZE] = CONFIG_INITRD_FILENAME;

static char cmd_line[128] = {0};

#if CONFIG_SPINAND_TEST_ONLY
static uint32_t load_le32(const uint8_t *buf)
{
	return (uint32_t)buf[0] |
		   ((uint32_t)buf[1] << 8) |
		   ((uint32_t)buf[2] << 16) |
		   ((uint32_t)buf[3] << 24);
}

static void dump_spinand_test_data(const uint8_t *buf, uint32_t len, uint32_t fail_offset)
{
	uint32_t start = fail_offset & ~0xFU;
	uint32_t end;
	uint32_t i;

	if (start >= 0x40U)
		start -= 0x40U;
	else
		start = 0U;

	end = start + 0x80U;
	if (end > len)
		end = len;

	info("SPI-NAND TEST: dump 0x%08" PRIx32 "..0x%08" PRIx32 "\r\n",
		 (uint32_t)(CONFIG_SPINAND_TEST_ADDR + start), (uint32_t)(CONFIG_SPINAND_TEST_ADDR + end));

	for (i = start; i < end; i++) {
		if ((i & 0xFU) == 0U)
			message("%08" PRIx32 ":", (uint32_t)(CONFIG_SPINAND_TEST_ADDR + i));
		message(" %02x", buf[i]);
		if ((i & 0xFU) == 0xFU)
			message("\r\n");
	}
	if ((end & 0xFU) != 0U)
		message("\r\n");
}

static void verify_spinand_testdata(sunxi_spi_t *spi)
{
	uint8_t *buf = (uint8_t *)(uintptr_t)SDRAM_BASE;
	uint32_t expected = CONFIG_SPINAND_TEST_SEED;
	uint32_t fail_count = 0U;
	uint32_t first_fail_word = 0U;
	uint32_t first_expected = 0U;
	uint32_t first_got = 0U;
	uint64_t start;
	uint64_t elapsed;
	uint32_t i;
	uint32_t ret;
	uint32_t mb_per_second;

	info("SPI-NAND TEST: read addr=0x%08" PRIx32 " len=%" PRIu32 "\r\n",
		 (uint32_t)CONFIG_SPINAND_TEST_ADDR, (uint32_t)CONFIG_SPINAND_TEST_LEN);

	start = time_us();
	ret = spi_nand_read(spi, buf, CONFIG_SPINAND_TEST_ADDR, CONFIG_SPINAND_TEST_LEN);
	elapsed = time_us() - start;
	mb_per_second = (elapsed == 0U) ? 0U : (uint32_t)(((uint64_t)CONFIG_SPINAND_TEST_LEN * 1000000ULL) /
													  ((uint64_t)elapsed * 1024ULL * 1024ULL));

	info("SPI-NAND TEST: read returned %" PRIu32 ", time=%" PRIu32 "us speed=%" PRIu32 "MB/S\r\n",
		 (uint32_t)ret, (uint32_t)elapsed, mb_per_second);

	for (i = 0U; i < (CONFIG_SPINAND_TEST_LEN / 4U); i++) {
		uint32_t got = load_le32(&buf[i * 4U]);

		if (got != expected) {
			if (fail_count == 0U) {
				first_fail_word = i;
				first_expected = expected;
				first_got = got;
			}
			fail_count++;
		}

		expected = expected * 1664525U + 1013904223U;
	}

	if (fail_count == 0U) {
		info("SPI-NAND TEST: PASS, %" PRIu32 " words\r\n", (uint32_t)(CONFIG_SPINAND_TEST_LEN / 4U));
	} else {
		uint32_t first_fail_offset = first_fail_word * 4U;

		error("SPI-NAND TEST: FAIL, %" PRIu32 " mismatches\r\n", fail_count);
		error("SPI-NAND TEST: first mismatch word=%" PRIu32 " offset=0x%08" PRIx32
			  " expected=0x%08" PRIx32 " got=0x%08" PRIx32 "\r\n",
			  (uint32_t)first_fail_word, (uint32_t)(CONFIG_SPINAND_TEST_ADDR + first_fail_offset),
			  (uint32_t)first_expected, (uint32_t)first_got);
		dump_spinand_test_data(buf, CONFIG_SPINAND_TEST_LEN, first_fail_offset);
	}
}
#endif

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

static void boot_linux_psci(void (*kernel_entry)(int zero, int arch,
							unsigned int params),
					unsigned long machid, unsigned long fdt_addr)
{
	void (*nonsec_entry)(void *target_pc, unsigned long r0,
					 unsigned long r1, unsigned long r2);

	if (armv7_init_nonsec()) {
		fatal("PSCI: armv7_init_nonsec() failed\r\n");
	}

	nonsec_entry = secure_ram_addr(_do_nonsec_entry);
	nonsec_entry((void *)kernel_entry, 0, machid, fdt_addr);
}

int main(void)
{
	unsigned int entry_point = 0;
	void (*kernel_entry)(int zero, int arch, unsigned int params);
	uint32_t	 memory_size;

#if CONFIG_BOOT_SDCARD || CONFIG_BOOT_MMC
	bool	 sd_boot_ready = false;
#endif

#if CONFIG_BOOT_SPINAND
	bool image_loaded = false;
#endif
	sunxi_clk_init();
	board_init();

	message("\r\n");
	info("AWBoot r%" PRIu32 " starting...\r\n", (u32)BUILD_REVISION);

	uint32_t clk_fail = sunxi_clk_get_fail_addr();
	if (clk_fail != 0U) {
		warning("CLK: init timeout at 0x%08" PRIx32 "\r\n", clk_fail);
	}

	memory_size = sunxi_dram_init();
	info("DRAM init done: %" PRIu32 " MiB\r\n", memory_size >> 20);

#if CONFIG_SPINAND_TEST_ONLY
	dma_init();
	debug("SPI: init\r\n");
	if (sunxi_spi_init(&sunxi_spi0) != 0) {
		fatal("SPI: init failed\r\n");
	}

	if (spi_nand_detect(&sunxi_spi0) != 0) {
		fatal("SPI-NAND: detect failed\r\n");
	}
	info("SPI-NAND TEST: detect OK\r\n");

	verify_spinand_testdata(&sunxi_spi0);
	sunxi_spi_disable(&sunxi_spi0);
	sunxi_wdg_set(0);
	info("SPI-NAND TEST: done\r\n");
	while (1) {
	}
#endif

#ifdef CONFIG_ENABLE_CPU_FREQ_DUMP
	sunxi_clk_dump();
#endif

	sunxi_wdg_set(10);
	memset(&image, 0, sizeof(image_info_t));
	image.filename		   = kernel_filename;
	image.of_filename	   = dtb_filename;
	image.initrd_filename = initrd_filename;

	image.dtb_dest	  = (u8 *)(uintptr_t)(dram_get_top() - CONFIG_DTB_GUARD_SIZE);
	image.kernel_dest = (u8 *)(uintptr_t)CONFIG_KERNEL_LOAD_ADDR;

// Normal media boot
#if CONFIG_BOOT_SDCARD || CONFIG_BOOT_MMC

	debug("SMHC: init start\r\n");
	if (sunxi_sdhci_init(&SDHCI) != 0) {
		fatal("SMHC: %s controller init failed\r\n", SDHCI.name);
	}
	info("SMHC: detect start\r\n");
	if (sdmmc_init(&card0, &SDHCI) != 0) {
#if CONFIG_BOOT_SPINAND
		warning("SMHC: init failed, trying SPI\r\n");
		strcpy(cmd_line, CONFIG_DEFAULT_BOOT_CMD);
#else
		fatal("SMHC: init failed\r\n");
#endif
	} else {
		sdmmc_speed_test();
		info("SMHC: mount start\r\n");
		if (mount_sdmmc() != 0) {
			fatal("SMHC: card mount failed\r\n");
		}

		image.initrd_size = 0; // Set by load_sdmmc()
		sd_boot_ready		  = true;
	}

#elif CONFIG_BOOT_SPINAND
	// Static slot configs for SPI
	image.initrd_size = 0; // disabled
	strcpy(cmd_line, CONFIG_DEFAULT_BOOT_CMD);
	strcpy(image.filename, CONFIG_KERNEL_FILENAME);
	strcpy(image.of_filename, CONFIG_DTB_FILENAME);

#else // 100% Fel boot
	info("BOOT: FEL mode\r\n");

	apply_fel_mailboxes(&image);

	strcpy(cmd_line, CONFIG_DEFAULT_BOOT_CMD);
#endif

#if CONFIG_BOOT_SDCARD || CONFIG_BOOT_MMC
	if (sd_boot_ready) {
#if CONFIG_BOOT_SPINAND
		if (load_sdmmc(&image) != 0) {
			unmount_sdmmc();
			warning("SMHC: loading failed, trying SPI\r\n");
			sd_boot_ready = false;
			strcpy(cmd_line, CONFIG_DEFAULT_BOOT_CMD);
		} else {
			unmount_sdmmc();
			image_loaded = true;
		}
#else
		if (load_sdmmc(&image) != 0) {
			fatal("SMHC: card load failed\r\n");
		}
		unmount_sdmmc();
#endif
	}
#endif

#if CONFIG_BOOT_SPINAND
	if (!image_loaded) {
		if (cmd_line[0] == '\0') {
			strcpy(cmd_line, CONFIG_DEFAULT_BOOT_CMD);
		}
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
		image_loaded = true;
	}
#endif // CONFIG_SPI_NAND

	// The kernel will reset WDG
	sunxi_wdg_set(3);

	if (boot_image_setup((unsigned char *)image.kernel_dest, &entry_point) != 0) {
		fatal("boot setup failed\r\n");
	}

#if !CONFIG_BOOT_SPINAND && !CONFIG_BOOT_SDCARD && !CONFIG_BOOT_MMC
	cmd_line[0] = '\0'; 
#endif

	if (strlen(cmd_line) > 0) {
		debug("BOOT: args %s\r\n", cmd_line);
		if (fdt_update_bootargs(image.dtb_dest, cmd_line)) {
			fatal("BOOT: Failed to set boot args\r\n");
		}
	}

	const uint32_t usable_memory_size = memory_size - CONFIG_PSCI_DRAM_RESERVE;
	if (memory_size <= CONFIG_PSCI_DRAM_RESERVE) {
		fatal("BOOT: invalid memory size %" PRIu32 "\r\n", memory_size);
	}
	if (fdt_update_memory(image.dtb_dest, SDRAM_BASE, usable_memory_size)) {
		fatal("BOOT: Failed to set memory size\r\n");
	} else {
		debug("BOOT: Set memory size to 0x%" PRIx32 " (reserve 0x%" PRIx32 ")\r\n",
					usable_memory_size, (uint32_t)CONFIG_PSCI_DRAM_RESERVE);
	}

	if ((image.initrd_size > 0U) && (image.initrd_dest == NULL)) {
		uint64_t initrd_pos = dram_get_top() - (uint64_t)image.initrd_size;
		image.initrd_dest   = (u8 *)(uintptr_t)initrd_pos;
	}

	if (image.initrd_size > 0U) {
		if (image.initrd_size > CONFIG_INITRAMFS_MAX_SIZE) {
			fatal("BOOT: bad initrd size (%u)\r\n", image.initrd_size);
		}

		const uintptr_t initrd_start = (uintptr_t)image.initrd_dest;
		const uintptr_t initrd_end   = initrd_start + (uintptr_t)image.initrd_size;

		if (!range_in_sdram((uint32_t)initrd_start, image.initrd_size)) {
			fatal("BOOT: initrd range 0x%08" PRIx32 "-0x%08" PRIx32 " invalid\r\n",
						(uint32_t)initrd_start, (uint32_t)initrd_end);
		}

		if ((CONFIG_INITRD_ALIGNMENT != 0U) &&
			((initrd_start & (CONFIG_INITRD_ALIGNMENT - 1U)) != 0U)) {
			warning("BOOT: initrd start 0x%08" PRIx32 " not %u-byte aligned\r\n",
				(uint32_t)initrd_start, CONFIG_INITRD_ALIGNMENT);
		}

		if (fdt_update_initrd(image.dtb_dest, (uint32_t)image.initrd_dest,
					(uint32_t)(image.initrd_dest + image.initrd_size))) {
			fatal("BOOT: Failed to set initrd address\r\n");
		} else {
			debug("BOOT: Set initrd to 0x%08" PRIx32 "->0x%08" PRIx32 "\r\n",
						(uint32_t)image.initrd_dest,
						(uint32_t)(image.initrd_dest + image.initrd_size));
		}
	} else {
		image.initrd_dest = NULL;
	}

	info("booting linux...\r\n");
	board_set_led(LED_BOARD, 0);

	arm32_mmu_disable();
	arm32_dcache_disable();
	arm32_icache_disable();
	arm32_interrupt_disable();

	kernel_entry = (void (*)(int, int, unsigned int))entry_point;
	boot_linux_psci(kernel_entry, ~0UL, (unsigned int)image.dtb_dest);
	fatal("PSCI: boot_linux_psci returned unexpectedly\r\n");

	return 0;
}
