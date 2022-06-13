#include "main.h"
#include "fdt.h"
#include "dram_v2.h"
#include "ff.h"
#include "sunxi_gpio.h"
#include "sunxi_sdhci.h"
#include "sdcard.h"
#include "arm32.h"
#include "debug.h"
#include "board.h"

struct image_info image;

extern void		  sys_uart_init(void);
extern void		  sys_clock_init(void);


dram_para_t ddr_param = {
	.dram_clk	 = 792,
	.dram_type	 = 3,
	.dram_zq	 = 0x7b7bfb,
	.dram_odt_en = 0x00,
	.dram_para1	 = 0x000010d2,
	.dram_para2	 = 0x0000,
	.dram_mr0	 = 0x1c70,
	.dram_mr1	 = 0x042,
	.dram_mr2	 = 0x18,
	.dram_mr3	 = 0x0,
	.dram_tpr0	 = 0x004A2195,
	.dram_tpr1	 = 0x02423190,
	.dram_tpr2	 = 0x0008B061,
	.dram_tpr3	 = 0xB4787896,
	.dram_tpr4	 = 0x0,
	.dram_tpr5	 = 0x48484848,
	.dram_tpr6	 = 0x00000048,
	.dram_tpr7	 = 0x1620121e,
	.dram_tpr8	 = 0x0,
	.dram_tpr9	 = 0x0,
	.dram_tpr10	 = 0x0,
	.dram_tpr11	 = 0x00340000,
	.dram_tpr12	 = 0x00000046,
	.dram_tpr13	 = 0x34000100,
};

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
struct linux_zimage_header {
	unsigned int code[9];
	unsigned int magic;
	unsigned int start;
	unsigned int end;
};

static int boot_image_setup(unsigned char *addr, unsigned int *entry)
{
	struct linux_zimage_header *zimage_header = (struct linux_zimage_header *)addr;

	if (zimage_header->magic == LINUX_ZIMAGE_MAGIC) {
		*entry = ((unsigned int)addr + zimage_header->start);
		return 0;
	}

	debug("unsupported kernel image\r\n");

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
		debug("*** FATFS: f_open, filename: [%s]: error\r\n", filename);
		ret = -1;
		goto open_fail;
	}

	do {
		byte_read = 0;
		fret	  = f_read(&file, (void *)(dest), byte_to_read, &byte_read);
		dest += byte_to_read;
	} while (byte_read >= byte_to_read);

	if (fret != FR_OK) {
		debug("*** FATFS: f_read: error\r\n");
		ret = -1;
		goto read_fail;
	}
	ret = 0;

read_fail:
	fret = f_close(&file);

open_fail:
	return ret;
}

int load_sdcard(struct image_info *image)
{
	FATFS	fs;
	FRESULT fret;
	int		ret;

	/* mount fs */
	fret = f_mount(0, &fs);
	if (fret != FR_OK) {
		debug("*** FATFS: f_mount mount error **\r\n");
		return -1;
	}

	debug("SD/MMC: Image: Read file %s addr=%x\r\n", image->filename, image->dest);
	ret = sdcard_loadimage(image->filename, image->dest);
	if (ret)
		return ret;

	debug("SD/MMC: dt blob: Read file %s addr=%x\r\n", image->of_filename, image->of_dest);
	ret = sdcard_loadimage(image->of_filename, image->of_dest);
	if (ret)
		return ret;

	/* umount fs */
	fret = f_mount(0, NULL);
	if (fret != FR_OK) {
		debug("*** FATFS: f_mount umount error **\r\n");
		return -1;
	}

	return 0;
}

int main(void)
{
//	unsigned int dtb_size;
	unsigned int entry_point;
	int ret;
	uint32_t reg32;
	void (*kernel_entry)(int zero, int arch, unsigned int params);


//	sys_clock_init();
	board_init();

	debug("Allwinner T113-loader\r\n");
	udelay(10000);

	/* ?? */
	reg32 = read32(0x070901f4);
	if ((reg32 & 0x1f) != 0) {
		reg32 = (reg32 & 0xffffff8f) | 0x40;
		reg32 = (reg32 & 0xffffff8f) | 0xc0;
		reg32 = (reg32 & 0xffffff8e) | 0xc0;
		write32(0x070901f4, reg32);
		debug("fix vccio detect value:0x%x\r\n", reg32);
	}


	uint32_t addr = 0x0200180C;
	uint32_t val  = (1 << 16) | (1 << 0);
	write32(addr, val);
	udelay(200);

	init_DRAM(0, &ddr_param);

	memset(&image, 0, sizeof(struct image_info));
	image.dest = (unsigned char *)CONFIG_KERNEL_LOAD_ADDR;
	strcpy(image.filename, CONFIG_KERNEL_FILENAME);

	image.of_dest = (unsigned char *)CONFIG_DTB_LOAD_ADDR;
	strcpy(image.of_filename, CONFIG_DTB_FILENAME);

	if (load_sdcard(&image) != 0)
		goto _error;

	// dtb_size = of_get_dt_total_size((void *)CONFIG_DTB_LOAD_ADDR);
	// debug("dtb size=%u bytes\r\n", dtb_size);

	ret = boot_image_setup((unsigned char *)image.dest, &entry_point);
	if (ret)
		goto _error;

	debug("booting linux...\r\n");

	//	neon_enable();
	//	enable_smp();

	arm32_mmu_disable();
	arm32_dcache_disable();
	arm32_icache_disable();

	kernel_entry = (void (*)(int, int, unsigned int))entry_point;
	kernel_entry(0, ~0, CONFIG_DTB_LOAD_ADDR);

_error:
	debug("oops, something went wrong...\r\n");
	for (;;)
		;
}
