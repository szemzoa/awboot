LIB := lib

INCLUDE_DIRS += -I $(LIB)

USE_SDMMC = $(shell grep -E "^\#define CONFIG_BOOT_(SDCARD|MMC)" board.h)

ifneq ($(USE_SDMMC),)
SRCS	+=  $(LIB)/loaders.c
endif

SRCS	+=  $(LIB)/fdt.c
SRCS	+=  $(LIB)/debug.c
SRCS	+=  $(LIB)/string.c
SRCS	+=  $(LIB)/xformat.c

include lib/fatfs/fatfs.mk
