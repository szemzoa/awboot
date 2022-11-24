ARCH := arch
SOC:=$(ARCH)/arm32/mach-t113s3

INCLUDE_DIRS += -I $(ARCH)/arm32/include -I $(SOC)/include -I $(SOC) -I $(SOC)/mmc

CFLAGS += -DCOUNTER_FREQUENCY=24000000

SRCS	+=  $(SOC)/mctl_hal.c
SRCS	+=  $(SOC)/sunxi_usart.c
SRCS	+=  $(SOC)/arch_timer.c
SRCS	+=  $(SOC)/sunxi_spi.c
SRCS	+=  $(SOC)/sunxi_gpio.c
SRCS	+=  $(SOC)/sdcard.c
SRCS	+=  $(SOC)/sunxi_sdhci.c
SRCS	+=  $(SOC)/sunxi_clk.c
SRCS	+=  $(SOC)/exception.c
ASRCS	+=  $(SOC)/start.S

