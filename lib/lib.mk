LIB := lib
FS_FAT := $(LIB)/fatfs

INCLUDE_DIRS += -I $(FS_FAT) -I $(LIB)

#USE_FAT = $(shell grep -E "^\#define CONFIG_BOOT_(SDCARD|MMC)" boards/board.h)

#ifneq ($(USE_FAT),)
SRCS    +=  $(FS_FAT)/ff.c
SRCS    +=  $(FS_FAT)/diskio.c
SRCS    +=  $(FS_FAT)/ffsystem.c
SRCS    +=  $(FS_FAT)/ffunicode.c
#endif

SRCS	+=  $(LIB)/debug.c
SRCS	+=  $(LIB)/fdt.c
SRCS	+=  $(LIB)/string.c
SRCS	+=  $(LIB)/xformat.c
