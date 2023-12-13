# Target
TARGET := awboot
CROSS_COMPILE ?= arm-none-eabi

# Log level defaults to info
LOG_LEVEL ?= 30

SRCS := main.c board.c lib/debug.c lib/xformat.c lib/fdt.c lib/string.c

INCLUDE_DIRS :=-I . -I include -I lib
LIBS := -lgcc -nostdlib
DEFINES := -DLOG_LEVEL=$(LOG_LEVEL) -DBUILD_REVISION=$(shell cat .build_revision)

include	arch/arch.mk
include	lib/fatfs/fatfs.mk

CFLAGS += -mcpu=cortex-a7 -mthumb-interwork -mthumb -mno-unaligned-access -mfpu=neon-vfpv4 -mfloat-abi=hard
CFLAGS += -ffast-math -ffunction-sections -fdata-sections -Os -std=gnu99 -Wall -Werror -Wno-unused-function -g -MMD $(INCLUDES) $(DEFINES)

ASFLAGS += $(CFLAGS)

LDFLAGS += $(CFLAGS) $(LIBS) -Wl,--gc-sections

STRIP=$(CROSS_COMPILE)-strip
CC=$(CROSS_COMPILE)-gcc
SIZE=$(CROSS_COMPILE)-size
OBJCOPY=$(CROSS_COMPILE)-objcopy

HOSTCC=gcc
HOSTSTRIP=strip

MAKE=make

DTB ?= sun8i-t113-mangopi-dual.dtb
KERNEL ?= zImage

all: git begin build mkboot

begin:
	@echo "---------------------------------------------------------------"
	@echo -n "Compiler version: "
	@$(CC) -v 2>&1 | tail -1

build_revision:
	@expr `cat .build_revision` + 1 > .build_revision

.PHONY: tools git begin build mkboot clean format
.SILENT:

git:
	cp -f tools/hooks/* .git/hooks/

build:: build_revision

# $(1): varient name
# $(2): values to remove from board.h
define VARIENT =

# Objects
$(1)_OBJ_DIR = build-$(1)
$(1)_BUILD_OBJS = $$(SRCS:%.c=$$($(1)_OBJ_DIR)/%.o)
$(1)_BUILD_OBJSA = $$(ASRCS:%.S=$$($(1)_OBJ_DIR)/%.o)
$(1)_OBJS = $$($(1)_BUILD_OBJSA) $$($(1)_BUILD_OBJS)

build:: $$($(1)_OBJ_DIR)/$$(TARGET)-boot.elf $$($(1)_OBJ_DIR)/$$(TARGET)-boot.bin $$($(1)_OBJ_DIR)/$$(TARGET)-fel.elf $$($(1)_OBJ_DIR)/$$(TARGET)-fel.bin

.PRECIOUS : $$($(1)_OBJS)
$$($(1)_OBJ_DIR)/$$(TARGET)-fel.elf: $$($(1)_OBJS)
	echo "  LD    $$@"
	$$(CC) -E -P -x c -D__RAM_BASE=0x00030000 ./arch/arm32/mach-t113s3/link.ld > $$($(1)_OBJ_DIR)/link-fel.ld
	$$(CC) $$^ -o $$@ -T $$($(1)_OBJ_DIR)/link-fel.ld $$(LDFLAGS) -Wl,-Map,$$($(1)_OBJ_DIR)/$$(TARGET)-fel.map

$$($(1)_OBJ_DIR)/$$(TARGET)-boot.elf: $$($(1)_OBJS)
	echo "  LD    $$@"
	$$(CC) -E -P -x c -D__RAM_BASE=0x00020000 ./arch/arm32/mach-t113s3/link.ld > $$($(1)_OBJ_DIR)/link-boot.ld
	$$(CC) $$^ -o $$@ -T $$($(1)_OBJ_DIR)/link-boot.ld $$(LDFLAGS) -Wl,-Map,$$($(1)_OBJ_DIR)/$$(TARGET)-boot.map

$$($(1)_OBJ_DIR)/$$(TARGET)-fel.bin: $$($(1)_OBJ_DIR)/$$(TARGET)-fel.elf
	@echo OBJCOPY $$@
	$$(OBJCOPY) -O binary $$< $$@

$$($(1)_OBJ_DIR)/$$(TARGET)-boot.bin: $$($(1)_OBJ_DIR)/$$(TARGET)-boot.elf
	@echo OBJCOPY $$@
	$$(OBJCOPY) -O binary $$< $$@

$$($(1)_OBJ_DIR)/%.o : %.c
	echo "  CC    $$@"
	mkdir -p $$(@D)
	$$(CC) $$(CFLAGS) -include $$($(1)_OBJ_DIR)/board.h $$(INCLUDE_DIRS) -c $$< -o $$@

$$($(1)_OBJ_DIR)/%.o : %.S
	echo "  CC    $$@"
	mkdir -p $$(@D)
	$$(CC) $$(ASFLAGS) $$(INCLUDE_DIRS) -c $$< -o $$@

$$($(1)_OBJS): $$($(1)_OBJ_DIR)/board.h

$$($(1)_OBJ_DIR)/board.h: board.h
	echo "  GEN   $$@"
	mkdir -p $$(@D)
	grep -v "$(2)" >$$@ <$$<

clean::
	rm -rf $$($(1)_OBJ_DIR)

-include $$(patsubst %.o,%.d,$$($(1)_OBJS))

endef

# build spi-only image without sd/mmc
$(eval $(call VARIENT,spi,CONFIG_BOOT_SDCARD\|CONFIG_BOOT_MMC))

# build sd/mmc only image without spi
$(eval $(call VARIENT,sdmmc,CONFIG_BOOT_SPINAND))

# build image with everything
$(eval $(call VARIENT,all,XXXXXXXXX))

clean::
	rm -f $(TARGET)-*.bin
	rm -f $(TARGET)-*.map
	rm -f *.img
	rm -f *.d
	$(MAKE) -C tools clean

format:
	find . -iname "*.h" -o -iname "*.c" | xargs clang-format --verbose -i

tools:
	$(MAKE) -C tools all

mkboot: build tools
	echo "SPI:"
	$(SIZE) build-spi/$(TARGET)-boot.elf
	cp -f build-spi/$(TARGET)-boot.bin $(TARGET)-boot-spi.bin
	cp -f build-spi/$(TARGET)-boot.bin $(TARGET)-boot-spi-4k.bin
	tools/mksunxi $(TARGET)-boot-spi.bin 8192
	tools/mksunxi $(TARGET)-boot-spi-4k.bin 8192 4096

	echo "SDMMC:"
	$(SIZE) build-sdmmc/$(TARGET)-boot.elf
	cp -f build-sdmmc/$(TARGET)-boot.bin $(TARGET)-boot-sd.bin
	tools/mksunxi $(TARGET)-boot-sd.bin 512

	echo "ALL:"
	$(SIZE) build-all/$(TARGET)-boot.elf
	cp -f build-all/$(TARGET)-boot.bin $(TARGET)-boot-all.bin
	cp -f build-all/$(TARGET)-boot.bin $(TARGET)-fel.bin
	tools/mksunxi $(TARGET)-fel.bin 8192
	tools/mksunxi $(TARGET)-boot-all.bin 8192

spi-boot.img: mkboot
	rm -f spi-boot.img
	dd if=$(TARGET)-boot-spi.bin of=spi-boot.img bs=2k
	dd if=$(TARGET)-boot-spi.bin of=spi-boot.img bs=2k seek=32 # Second copy on page 32
	dd if=$(TARGET)-boot-spi.bin of=spi-boot.img bs=2k seek=64 # Third copy on page 64
	# dd if=linux/boot/$(DTB) of=spi-boot.img bs=2k seek=128 # DTB on page 128
	# dd if=linux/boot/$(KERNEL) of=spi-boot.img bs=2k seek=256 # Kernel on page 256
