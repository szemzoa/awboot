ARCH := arch
SOC:=$(ARCH)/arm32/mach-t113s3

INCLUDE_DIRS += -I $(ARCH)/arm32/include -I $(SOC)/include -I $(SOC) -I $(SOC)/mmc

CFLAGS += -DCOUNTER_FREQUENCY=24000000

ASRCS	+=  $(SOC)/start.S
ASRCS	+=  $(SOC)/memcpy.S

ifneq ($(wildcard $(SOC)/dram_sun20i_d1.c),)
SRCS	+=  $(SOC)/dram_sun20i_d1.c
else
SRCS	+=  $(SOC)/dram.c
endif
SRCS	+=  $(SOC)/sunxi_usart.c
SRCS	+=  $(SOC)/arch_timer.c
SRCS	+=  $(SOC)/sunxi_gpio.c
SRCS	+=  $(SOC)/sunxi_clk.c
SRCS	+=  $(SOC)/exception.c

USE_SPI = $(shell grep -E "^\#define CONFIG_BOOT_SPI" board.h)
ifneq ($(USE_SPI),)
SRCS	+=  $(SOC)/sunxi_spi.c
SRCS	+=  $(SOC)/sunxi_dma.c
endif

USE_SDMMC = $(shell grep -E "^\#define CONFIG_BOOT_(SDCARD|MMC)" board.h)
ifneq ($(USE_SDMMC),)
SRCS	+=  $(SOC)/sdmmc.c
SRCS	+=  $(SOC)/sunxi_sdhci.c
endif
