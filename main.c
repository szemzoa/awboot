#include "main.h"
#include "fdt.h"
#include "ff.h"
#include "div.h"
#include "sunxi_gpio.h"
#include "sunxi_sdhci.h"
#include "sunxi_spi.h"
#include "sunxi_clk.h"
#include "sdcard.h"
#include "arm32.h"
#include "reg-ccu.h"
#include "debug.h"
#include "board.h"

image_info_t image;

#if 0
void neon_enable(void)
{
        /* set NSACR, both Secure and Non-secure access are allowed to NEON */
        asm volatile("MRC p15, 0, r0, c1, c1, 2");
        asm volatile("ORR r0, r0, #(0x3<<10) @ enable fpu/neon");
        asm volatile("MCR p15, 0, r0, c1, c1, 2");
        /* Set the CPACR for access to CP10 and CP11*/
        asm volatile("LDR r0, =0xF00000");
        asm volatile("MCR p15, 0, r0, c1, c0, 2");
        /* Set the FPEXC EN bit to enable the FPU */
        asm volatile("MOV r3, #0x40000000");
        asm volatile("MCR p10, 7, r3, c8, c0, 0");
}

void enable_smp(void)
{
        u32 val;

        /* SMP status is controlled by bit 6 of the CP15 Aux Ctrl Reg:ACTLR */
        asm volatile("MRC     p15, 0, r0, c1, c0, 1");
        asm volatile("ORR     r0, r0, #0x040");
        asm volatile("MCR     p15, 0, r0, c1, c0, 1");
#ifdef DEBUG_MMU
                asm volatile("MRC         p15, 0, r0, c1, c0, 1");
                asm volatile("mov %0, r0" : "=r"(val));
                debug("val:%x\n", val);
#endif
}
#endif

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

#define CHUNK_SIZE 0x20000
static int sdcard_loadimage(char *filename, BYTE *dest)
{
	FIL		file;
	UINT	byte_to_read = CHUNK_SIZE;
	UINT	byte_read;
	FRESULT fret;
	int		ret;

	fret = f_open(&file, filename, FA_OPEN_EXISTING | FA_READ);
	if (fret != FR_OK) {
		error("FATFS: f_open, filename: [%s]: error %d\r\n", filename, fret);
		ret = -1;
		goto open_fail;
	}

	do {
		byte_read = 0;
		fret	  = f_read(&file, (void *)(dest), byte_to_read, &byte_read);
		dest += byte_to_read;
	} while (byte_read >= byte_to_read);

	if (fret != FR_OK) {
		error("FATFS: f_read: error %d\r\n", fret);
		ret = -1;
		goto read_fail;
	}
	ret = 0;

read_fail:
	fret = f_close(&file);

open_fail:
	return ret;
}

int load_sdcard(image_info_t *image)
{
	FATFS	fs;
	FRESULT fret;
	int		ret;

	/* mount fs */
	fret = f_mount(&fs, "", 1);
	if (fret != FR_OK) {
		error("FATFS: f_mount mount error %d\r\n", fret);
		return -1;
	} else {
		debug("FATFS: f_mount OK\r\n");
	}

	info("SMHC: Read %s addr=%x\r\n", image->of_filename, (unsigned int)image->of_dest);
	ret = sdcard_loadimage(image->of_filename, image->of_dest);
	if (ret)
		return ret;

	info("SMHC: Read %s addr=%x\r\n", image->filename, (unsigned int)image->dest);
	ret = sdcard_loadimage(image->filename, image->dest);
	if (ret)
		return ret;

	/* umount fs */
	fret = f_mount(0, "", 0);
	if (fret != FR_OK) {
		error("FATFS: f_mount unmount error %d\r\n", fret);
		return -1;
	} else {
		debug("FATFS: f_mount unmount OK\r\n");
	}

	return 0;
}

int load_spi_nand(sunxi_spi_t *spi, image_info_t *image)
{
	linux_zimage_header_t *hdr;
	unsigned int		   size;

	if (spi_nand_detect(spi) != 0)
		return -1;

	/* get dtb size and read */
	spi_nand_read(spi, image->of_dest, CONFIG_SPINAND_DTB_ADDR, (uint32_t)sizeof(boot_param_header_t));
	if (of_get_magic_number(image->of_dest) != OF_DT_MAGIC) {
		error("SPI-NAND: DTB verification failed\r\n");
		return -1;
	}
	size = of_get_dt_total_size(image->of_dest);
	info("SPI-NAND: dt blob: Copy from 0x%08x to 0x%08lx size:0x%08x\r\n", CONFIG_SPINAND_DTB_ADDR,
		 (uint32_t)image->of_dest, size);
	spi_nand_read(spi, image->of_dest, CONFIG_SPINAND_DTB_ADDR, (uint32_t)size);

	/* get kernel size and read */
	spi_nand_read(spi, image->dest, CONFIG_SPINAND_KERNEL_ADDR, (uint32_t)sizeof(linux_zimage_header_t));
	hdr = (linux_zimage_header_t *)image->dest;
	if (hdr->magic != LINUX_ZIMAGE_MAGIC) {
		debug("SPI-NAND: zImage verification failed\r\n");
		return -1;
	}
	size = hdr->end - hdr->start;
	info("SPI-NAND: Image: Copy from 0x%08x to 0x%08lx size:0x%08x\r\n", CONFIG_SPINAND_KERNEL_ADDR,
		 (uint32_t)image->dest, size);
	spi_nand_read(spi, image->dest, CONFIG_SPINAND_KERNEL_ADDR, (uint32_t)size);

	return 0;
}

int main(void)
{
	unsigned int entry_point;
	uint32_t	 reg32;
	void (*kernel_entry)(int zero, int arch, unsigned int params);

	board_init();

	info("AWBoot starting\r\n");
	sunxi_clk_init();

	/* ?? */
	reg32 = read32(0x070901f4);
	if ((reg32 & 0x1f) != 0) {
		reg32 = (reg32 & 0xffffff8f) | 0x40;
		reg32 = (reg32 & 0xffffff8f) | 0xc0;
		reg32 = (reg32 & 0xffffff8e) | 0xc0;
		write32(0x070901f4, reg32);
		/* debug("fix vccio detect value:0x%x\r\n", reg32); */
	}

	uint32_t addr = 0x0200180C;
	uint32_t val  = (1 << 16) | (1 << 0);
	write32(addr, val);
	udelay(200);

	init_DRAM(0, &ddr_param);

#ifdef CONFIG_ENABLE_CPU_FREQ_DUMP
	sunxi_clk_dump();
#endif

	memset(&image, 0, sizeof(image_info_t));

	image.of_dest = (unsigned char *)CONFIG_DTB_LOAD_ADDR;
	image.dest	  = (unsigned char *)CONFIG_KERNEL_LOAD_ADDR;

#ifdef CONFIG_BOOT_SDCARD

	strcpy(image.filename, CONFIG_KERNEL_FILENAME);
	strcpy(image.of_filename, CONFIG_DTB_FILENAME);

	if (sunxi_sdhci_init(&sdhci0) != 0) {
		fatal("SMHC: %s controller init failed\r\n", sdhci0.name);
	} else {
		info("SMHC: %s controller initialized\r\n", sdhci0.name);
	}
	if (sdcard_init(&sdcard0, &sdhci0) != 0) {
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
_spi:
	debug("SPI: init...\r\n");
	if (sunxi_spi_init(&sunxi_spi0) != 0) {
		fatal("SPI: init failed\r\n");
	}

	if (load_spi_nand(&sunxi_spi0, &image) != 0) {
		fatal("SPI-NAND: loading failed\r\n");
	}

	sunxi_spi_disable(&sunxi_spi0);

#endif // CONFIG_SPI_NAND

_boot:
	if (boot_image_setup((unsigned char *)image.dest, &entry_point)) {
		fatal("boot setup failed\r\n");
	}

	info("booting linux...\r\n");

	//	neon_enable();
	//	enable_smp();

	arm32_mmu_disable();
	arm32_dcache_disable();
	arm32_icache_disable();

	kernel_entry = (void (*)(int, int, unsigned int))entry_point;
	kernel_entry(0, ~0, (unsigned int)image.of_dest);
}
