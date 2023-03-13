#ifndef __T113_REG_CCU_H__
#define __T113_REG_CCU_H__

#include "types.h"

#define T113_CCU_BASE (0x02001000)

#define CCU_PLL_CPU_CTRL_REG		 (0x000)
#define CCU_PLL_DDR_CTRL_REG		 (0x010)
#define CCU_PLL_PERI0_CTRL_REG		 (0x020)
#define CCU_PLL_PERI1_CTRL_REG		 (0x028)
#define CCU_PLL_GPU_CTRL_REG		 (0x030)
#define CCU_PLL_VIDEO0_CTRL_REG		 (0x040)
#define CCU_PLL_VIDEO1_CTRL_REG		 (0x048)
#define CCU_PLL_VE_CTRL				 (0x058)
#define CCU_PLL_DE_CTRL				 (0x060)
#define CCU_PLL_HSIC_CTRL			 (0x070)
#define CCU_PLL_AUDIO0_CTRL_REG		 (0x078)
#define CCU_PLL_AUDIO1_CTRL_REG		 (0x080)
#define CCU_PLL_DDR_PAT0_CTRL_REG	 (0x110)
#define CCU_PLL_DDR_PAT1_CTRL_REG	 (0x114)
#define CCU_PLL_PERI0_PAT0_CTRL_REG	 (0x120)
#define CCU_PLL_PERI0_PAT1_CTRL_REG	 (0x124)
#define CCU_PLL_PERI1_PAT0_CTRL_REG	 (0x128)
#define CCU_PLL_PERI1_PAT1_CTRL_REG	 (0x12c)
#define CCU_PLL_GPU_PAT0_CTRL_REG	 (0x130)
#define CCU_PLL_GPU_PAT1_CTRL_REG	 (0x134)
#define CCU_PLL_VIDEO0_PAT0_CTRL_REG (0x140)
#define CCU_PLL_VIDEO0_PAT1_CTRL_REG (0x144)
#define CCU_PLL_VIDEO1_PAT0_CTRL_REG (0x148)
#define CCU_PLL_VIDEO1_PAT1_CTRL_REG (0x14c)
#define CCU_PLL_VE_PAT0_CTRL_REG	 (0x158)
#define CCU_PLL_VE_PAT1_CTRL_REG	 (0x15c)
#define CCU_PLL_DE_PAT0_CTRL_REG	 (0x160)
#define CCU_PLL_DE_PAT1_CTRL_REG	 (0x164)
#define CCU_PLL_HSIC_PAT0_CTRL_REG	 (0x170)
#define CCU_PLL_HSIC_PAT1_CTRL_REG	 (0x174)
#define CCU_PLL_AUDIO0_PAT0_CTRL_REG (0x178)
#define CCU_PLL_AUDIO0_PAT1_CTRL_REG (0x17c)
#define CCU_PLL_AUDIO1_PAT0_CTRL_REG (0x180)
#define CCU_PLL_AUDIO1_PAT1_CTRL_REG (0x184)
#define CCU_PLL_CPU_BIAS_REG		 (0x300)
#define CCU_PLL_DDR_BIAS_REG		 (0x310)
#define CCU_PLL_PERI0_BIAS_REG		 (0x320)
#define CCU_PLL_PERI1_BIAS_REG		 (0x328)
#define CCU_PLL_GPU_BIAS_REG		 (0x330)
#define CCU_PLL_VIDEO0_BIAS_REG		 (0x340)
#define CCU_PLL_VIDEO1_BIAS_REG		 (0x348)
#define CCU_PLL_VE_BIAS_REG			 (0x358)
#define CCU_PLL_DE_BIAS_REG			 (0x360)
#define CCU_PLL_HSIC_BIAS_REG		 (0x370)
#define CCU_PLL_AUDIO0_BIAS_REG		 (0x378)
#define CCU_PLL_AUDIO1_BIAS_REG		 (0x380)
#define CCU_PLL_CPU_TUN_REG			 (0x400)
#define CCU_CPU_AXI_CFG_REG			 (0x500)
#define CCU_CPU_GATING_REG			 (0x504)
#define CCU_PSI_CLK_REG				 (0x510)
#define CCU_AHB3_CLK_REG			 (0x51c)
#define CCU_APB0_CLK_REG			 (0x520)
#define CCU_APB1_CLK_REG			 (0x524)
#define CCU_MBUS_CLK_REG			 (0x540)
#define CCU_DMA_BGR_REG				 (0x70c)
#define CCU_DRAM_CLK_REG			 (0x800)
#define CCU_MBUS_MAT_CLK_GATING_REG	 (0x804)
#define CCU_DRAM_BGR_REG			 (0x80c)
#define CCU_SMHC0_CLK_REG			 (0x830)
#define CCU_SMHC_BGR_REG			 (0x84c)
#define CCU_USART_BGR_REG			 (0x90c)
#define CCU_SPI0_CLK_REG			 (0x940)
#define CCU_SPI_BGR_REG				 (0x96c)
#define CCU_RISCV_CLK_REG			 (0xd00)
#define CCU_RISCV_GATING_REG		 (0xd04)
#define CCU_RISCV_CFG_BGR_REG		 (0xd0c)

/* MMC clock bit field */
#define CCU_MMC_CTRL_M(x)		  ((x)-1)
#define CCU_MMC_CTRL_N(x)		  ((x) << 8)
#define CCU_MMC_CTRL_OSCM24		  (0x0 << 24)
#define CCU_MMC_CTRL_PLL6X1		  (0x1 << 24)
#define CCU_MMC_CTRL_PLL6X2		  (0x2 << 24)
#define CCU_MMC_CTRL_PLL_PERIPH1X CCU_MMC_CTRL_PLL6X1
#define CCU_MMC_CTRL_PLL_PERIPH2X CCU_MMC_CTRL_PLL6X2
#define CCU_MMC_CTRL_ENABLE		  (0x1 << 31)
/* if doesn't have these delays */
#define CCU_MMC_CTRL_OCLK_DLY(a) ((void)(a), 0)
#define CCU_MMC_CTRL_SCLK_DLY(a) ((void)(a), 0)

#define CCU_MMC_BGR_SMHC0_GATE (1 << 0)
#define CCU_MMC_BGR_SMHC1_GATE (1 << 1)
#define CCU_MMC_BGR_SMHC2_GATE (1 << 2)

#define CCU_MMC_BGR_SMHC0_RST (1 << 16)
#define CCU_MMC_BGR_SMHC1_RST (1 << 17)
#define CCU_MMC_BGR_SMHC2_RST (1 << 18)

typedef struct {
	u32 pll1_cfg; /* 0x000 pll1 (cpux) control */
	u8	reserved_0x004[12];
	u32 pll5_cfg; /* 0x010 pll5 (ddr) control */
	u8	reserved_0x014[12];
	u32 pll6_cfg; /* 0x020 pll6 (periph0) control */
	u8	reserved_0x020[4];
	u32 pll_periph1_cfg; /* 0x028 pll periph1 control */
	u8	reserved_0x028[4];
	u32 pll7_cfg; /* 0x030 pll7 (gpu) control */
	u8	reserved_0x034[12];
	u32 pll3_cfg; /* 0x040 pll3 (video0) control */
	u8	reserved_0x044[4];
	u32 pll_video1_cfg; /* 0x048 pll video1 control */
	u8	reserved_0x04c[12];
	u32 pll4_cfg; /* 0x058 pll4 (ve) control */
	u8	reserved_0x05c[4];
	u32 pll10_cfg; /* 0x060 pll10 (de) control */
	u8	reserved_0x064[12];
	u32 pll9_cfg; /* 0x070 pll9 (hsic) control */
	u8	reserved_0x074[4];
	u32 pll2_cfg; /* 0x078 pll2 (audio) control */
	u8	reserved_0x07c[148];
	u32 pll5_pat; /* 0x110 pll5 (ddr) pattern */
	u8	reserved_0x114[20];
	u32 pll_periph1_pat0; /* 0x128 pll periph1 pattern0 */
	u32 pll_periph1_pat1; /* 0x12c pll periph1 pattern1 */
	u32 pll7_pat0; /* 0x130 pll7 (gpu) pattern0 */
	u32 pll7_pat1; /* 0x134 pll7 (gpu) pattern1 */
	u8	reserved_0x138[8];
	u32 pll3_pat0; /* 0x140 pll3 (video0) pattern0 */
	u32 pll3_pat1; /* 0x144 pll3 (video0) pattern1 */
	u32 pll_video1_pat0; /* 0x148 pll video1 pattern0 */
	u32 pll_video1_pat1; /* 0x14c pll video1 pattern1 */
	u8	reserved_0x150[8];
	u32 pll4_pat0; /* 0x158 pll4 (ve) pattern0 */
	u32 pll4_pat1; /* 0x15c pll4 (ve) pattern1 */
	u32 pll10_pat0; /* 0x160 pll10 (de) pattern0 */
	u32 pll10_pat1; /* 0x164 pll10 (de) pattern1 */
	u8	reserved_0x168[8];
	u32 pll9_pat0; /* 0x170 pll9 (hsic) pattern0 */
	u32 pll9_pat1; /* 0x174 pll9 (hsic) pattern1 */
	u32 pll2_pat0; /* 0x178 pll2 (audio) pattern0 */
	u32 pll2_pat1; /* 0x17c pll2 (audio) pattern1 */
	u8	reserved_0x180[384];
	u32 pll1_bias; /* 0x300 pll1 (cpux) bias */
	u8	reserved_0x304[12];
	u32 pll5_bias; /* 0x310 pll5 (ddr) bias */
	u8	reserved_0x314[12];
	u32 pll6_bias; /* 0x320 pll6 (periph0) bias */
	u8	reserved_0x324[4];
	u32 pll_periph1_bias; /* 0x328 pll periph1 bias */
	u8	reserved_0x32c[4];
	u32 pll7_bias; /* 0x330 pll7 (gpu) bias */
	u8	reserved_0x334[12];
	u32 pll3_bias; /* 0x340 pll3 (video0) bias */
	u8	reserved_0x344[4];
	u32 pll_video1_bias; /* 0x348 pll video1 bias */
	u8	reserved_0x34c[12];
	u32 pll4_bias; /* 0x358 pll4 (ve) bias */
	u8	reserved_0x35c[4];
	u32 pll10_bias; /* 0x360 pll10 (de) bias */
	u8	reserved_0x364[12];
	u32 pll9_bias; /* 0x370 pll9 (hsic) bias */
	u8	reserved_0x374[4];
	u32 pll2_bias; /* 0x378 pll2 (audio) bias */
	u8	reserved_0x37c[132];
	u32 pll1_tun; /* 0x400 pll1 (cpux) tunning */
	u8	reserved_0x404[252];
	u32 cpu_axi_cfg; /* 0x500 CPUX/AXI clock control*/
	u8	reserved_0x504[12];
	u32 psi_ahb1_ahb2_cfg; /* 0x510 PSI/AHB1/AHB2 clock control */
	u8	reserved_0x514[8];
	u32 ahb3_cfg; /* 0x51c AHB3 clock control */
	u32 apb1_cfg; /* 0x520 APB1 clock control */
	u32 apb2_cfg; /* 0x524 APB2 clock control */
	u8	reserved_0x528[24];
	u32 mbus_cfg; /* 0x540 MBUS clock control */
	u8	reserved_0x544[188];
	u32 de_clk_cfg; /* 0x600 DE clock control */
	u8	reserved_0x604[8];
	u32 de_gate_reset; /* 0x60c DE gate/reset control */
	u8	reserved_0x610[16];
	u32 di_clk_cfg; /* 0x620 DI clock control */
	u8	reserved_0x024[8];
	u32 di_gate_reset; /* 0x62c DI gate/reset control */
	u8	reserved_0x630[64];
	u32 gpu_clk_cfg; /* 0x670 GPU clock control */
	u8	reserved_0x674[8];
	u32 gpu_gate_reset; /* 0x67c GPU gate/reset control */
	u32 ce_clk_cfg; /* 0x680 CE clock control */
	u8	reserved_0x684[8];
	u32 ce_gate_reset; /* 0x68c CE gate/reset control */
	u32 ve_clk_cfg; /* 0x690 VE clock control */
	u8	reserved_0x694[8];
	u32 ve_gate_reset; /* 0x69c VE gate/reset control */
	u8	reserved_0x6a0[16];
	u32 emce_clk_cfg; /* 0x6b0 EMCE clock control */
	u8	reserved_0x6b4[8];
	u32 emce_gate_reset; /* 0x6bc EMCE gate/reset control */
	u32 vp9_clk_cfg; /* 0x6c0 VP9 clock control */
	u8	reserved_0x6c4[8];
	u32 vp9_gate_reset; /* 0x6cc VP9 gate/reset control */
	u8	reserved_0x6d0[60];
	u32 dma_gate_reset; /* 0x70c DMA gate/reset control */
	u8	reserved_0x710[12];
	u32 msgbox_gate_reset; /* 0x71c Message Box gate/reset control */
	u8	reserved_0x720[12];
	u32 spinlock_gate_reset; /* 0x72c Spinlock gate/reset control */
	u8	reserved_0x730[12];
	u32 hstimer_gate_reset; /* 0x73c HS Timer gate/reset control */
	u32 avs_gate_reset; /* 0x740 AVS gate/reset control */
	u8	reserved_0x744[72];
	u32 dbgsys_gate_reset; /* 0x78c Debugging system gate/reset control */
	u8	reserved_0x790[12];
	u32 psi_gate_reset; /* 0x79c PSI gate/reset control */
	u8	reserved_0x7a0[12];
	u32 pwm_gate_reset; /* 0x7ac PWM gate/reset control */
	u8	reserved_0x7b0[12];
	u32 iommu_gate_reset; /* 0x7bc IOMMU gate/reset control */
	u8	reserved_0x7c0[64];
	u32 dram_clk_cfg; /* 0x800 DRAM clock control */
	u32 mbus_gate; /* 0x804 MBUS gate control */
	u8	reserved_0x808[4];
	u32 dram_gate_reset; /* 0x80c DRAM gate/reset control */
	u32 nand0_clk_cfg; /* 0x810 NAND0 clock control */
	u32 nand1_clk_cfg; /* 0x814 NAND1 clock control */
	u8	reserved_0x818[20];
	u32 nand_gate_reset; /* 0x82c NAND gate/reset control */
	u32 smhc0_clk_cfg; /* 0x830 MMC0 clock control */
	u32 smhc1_clk_cfg; /* 0x834 MMC1 clock control */
	u32 smhc2_clk_cfg; /* 0x838 MMC2 clock control */
	u8	reserved_0x83c[16];
	u32 smhc_gate_reset; /* 0x84c MMC gate/reset control */
	u8	reserved_0x850[188];
	u32 uart_gate_reset; /* 0x90c UART gate/reset control */
	u8	reserved_0x910[12];
	u32 twi_gate_reset; /* 0x91c I2C gate/reset control */
	u8	reserved_0x920[28];
	u32 scr_gate_reset; /* 0x93c SCR gate/reset control */
	u32 spi0_clk_cfg; /* 0x940 SPI0 clock control */
	u32 spi1_clk_cfg; /* 0x944 SPI1 clock control */
	u8	reserved_0x948[36];
	u32 spi_gate_reset; /* 0x96c SPI gate/reset control */
	u8	reserved_0x970[12];
	u32 emac_gate_reset; /* 0x97c EMAC gate/reset control */
	u8	reserved_0x980[48];
	u32 ts_clk_cfg; /* 0x9b0 TS clock control */
	u8	reserved_0x9b4[8];
	u32 ts_gate_reset; /* 0x9bc TS gate/reset control */
	u32 irtx_clk_cfg; /* 0x9c0 IR TX clock control */
	u8	reserved_0x9c4[8];
	u32 irtx_gate_reset; /* 0x9cc IR TX gate/reset control */
	u8	reserved_0x9d0[44];
	u32 ths_gate_reset; /* 0x9fc THS gate/reset control */
	u8	reserved_0xa00[12];
	u32 i2s3_clk_cfg; /* 0xa0c I2S3 clock control */
	u32 i2s0_clk_cfg; /* 0xa10 I2S0 clock control */
	u32 i2s1_clk_cfg; /* 0xa14 I2S1 clock control */
	u32 i2s2_clk_cfg; /* 0xa18 I2S2 clock control */
	u32 i2s_gate_reset; /* 0xa1c I2S gate/reset control */
	u32 spdif_clk_cfg; /* 0xa20 SPDIF clock control */
	u8	reserved_0xa24[8];
	u32 spdif_gate_reset; /* 0xa2c SPDIF gate/reset control */
	u8	reserved_0xa30[16];
	u32 dmic_clk_cfg; /* 0xa40 DMIC clock control */
	u8	reserved_0xa44[8];
	u32 dmic_gate_reset; /* 0xa4c DMIC gate/reset control */
	u8	reserved_0xa50[16];
	u32 ahub_clk_cfg; /* 0xa60 Audio HUB clock control */
	u8	reserved_0xa64[8];
	u32 ahub_gate_reset; /* 0xa6c Audio HUB gate/reset control */
	u32 usb0_clk_cfg; /* 0xa70 USB0(OTG) clock control */
	u32 usb1_clk_cfg; /* 0xa74 USB1(XHCI) clock control */
	u8	reserved_0xa78[4];
	u32 usb3_clk_cfg; /* 0xa78 USB3 clock control */
	u8	reserved_0xa80[12];
	u32 usb_gate_reset; /* 0xa8c USB gate/reset control */
	u8	reserved_0xa90[32];
	u32 pcie_ref_clk_cfg; /* 0xab0 PCIE REF clock control */
	u32 pcie_axi_clk_cfg; /* 0xab4 PCIE AXI clock control */
	u32 pcie_aux_clk_cfg; /* 0xab8 PCIE AUX clock control */
	u32 pcie_gate_reset; /* 0xabc PCIE gate/reset control */
	u8	reserved_0xac0[64];
	u32 hdmi_clk_cfg; /* 0xb00 HDMI clock control */
	u32 hdmi_slow_clk_cfg; /* 0xb04 HDMI slow clock control */
	u8	reserved_0xb08[8];
	u32 hdmi_cec_clk_cfg; /* 0xb10 HDMI CEC clock control */
	u8	reserved_0xb14[8];
	u32 hdmi_gate_reset; /* 0xb1c HDMI gate/reset control */
	u8	reserved_0xb20[60];
	u32 tcon_top_gate_reset; /* 0xb5c TCON TOP gate/reset control */
	u32 tcon_lcd0_clk_cfg; /* 0xb60 TCON LCD0 clock control */
	u8	reserved_0xb64[24];
	u32 tcon_lcd_gate_reset; /* 0xb7c TCON LCD gate/reset control */
	u32 tcon_tv0_clk_cfg; /* 0xb80 TCON TV0 clock control */
	u8	reserved_0xb84[24];
	u32 tcon_tv_gate_reset; /* 0xb9c TCON TV gate/reset control */
	u8	reserved_0xba0[96];
	u32 csi_misc_clk_cfg; /* 0xc00 CSI MISC clock control */
	u32 csi_top_clk_cfg; /* 0xc04 CSI TOP clock control */
	u32 csi_mclk_cfg; /* 0xc08 CSI Master clock control */
	u8	reserved_0xc0c[32];
	u32 csi_gate_reset; /* 0xc2c CSI gate/reset control */
	u8	reserved_0xc30[16];
	u32 hdcp_clk_cfg; /* 0xc40 HDCP clock control */
	u8	reserved_0xc44[8];
	u32 hdcp_gate_reset; /* 0xc4c HDCP gate/reset control */
	u8	reserved_0xc50[688];
	u32 ccu_sec_switch; /* 0xf00 CCU security switch */
	u32 pll_lock_dbg_ctrl; /* 0xf04 PLL lock debugging control */
} ccu_reg_t;

extern volatile ccu_reg_t *const ccu;

#endif /* __T113_REG_CCU_H__ */
