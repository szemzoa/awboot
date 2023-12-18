ARCH := arch/arm32
SUNXI:=$(ARCH)/sunxi

CPU:=$(SUNXI)/$(SOC)

INCLUDE_DIRS += -I $(ARCH)/include -I $(CPU) -I $(SUNXI)

ASRCS	+=  $(SUNXI)/start.S

SRCS	+=  $(ARCH)/arch_timer.c
SRCS	+=  $(ARCH)/exception.c
ASRCS	+=  $(ARCH)/memcpy.S

SRCS	+=  $(SUNXI)/sunxi_usart.c
SRCS	+=  $(SUNXI)/sunxi_dma.c
SRCS	+=  $(SUNXI)/sunxi_gpio.c
SRCS	+=  $(SUNXI)/sunxi_spi.c
SRCS	+=  $(SUNXI)/sunxi_sdhci.c
SRCS	+=  $(SUNXI)/sdmmc.c

SRCS	+=  $(CPU)/dram.c
SRCS	+=  $(CPU)/sunxi_clk.c



