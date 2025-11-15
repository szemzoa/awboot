# Target
TARGET := awboot
CROSS_COMPILE ?= arm-none-eabi

# Log level defaults to info
LOG_LEVEL ?= 30

SRCS := main.c board.c

INCLUDE_DIRS :=-I . -I include -I lib
LIB_DIR := -L ./
LIBS := -lgcc -nostdlib
DEFINES := -DLOG_LEVEL=$(LOG_LEVEL) -DBUILD_REVISION=$(shell cat .build_revision)

include arch/arch.mk
include lib/lib.mk

CFLAGS += -mcpu=cortex-a7 -mthumb-interwork -mthumb -mno-unaligned-access -mfpu=neon-vfpv4 -mfloat-abi=hard
CFLAGS += -fno-tree-vectorize -ffreestanding -fno-builtin
CFLAGS += -ffunction-sections -fdata-sections -Os -std=c2x -Wall -Werror -Wno-unused-function $(INCLUDES) $(DEFINES)

ASFLAGS += $(CFLAGS)

LDFLAGS += $(CFLAGS) $(LIBS) -Wl,--gc-sections

STRIP=$(CROSS_COMPILE)-strip
CC=$(CROSS_COMPILE)-gcc
SIZE=$(CROSS_COMPILE)-size
OBJCOPY=$(CROSS_COMPILE)-objcopy

HOSTCC=gcc
HOSTSTRIP=strip

MAKE=make

SUPPORTED_VARIANTS := fel spi sdmmc emmc all
VARIANT ?= emmc
comma := ,
VARIANT_LIST := $(strip $(subst $(comma), ,$(VARIANT)))

ifneq ($(filter all,$(VARIANT_LIST)),)
BUILD_VARIANTS := $(SUPPORTED_VARIANTS)
else
BUILD_VARIANTS := $(VARIANT_LIST)
endif

INVALID_VARIANTS := $(filter-out $(SUPPORTED_VARIANTS),$(BUILD_VARIANTS))
ifneq ($(INVALID_VARIANTS),)
$(error Unknown VARIANT(s): $(INVALID_VARIANTS). Supported: $(SUPPORTED_VARIANTS))
endif

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

# $(1): variant name
# $(2): key=value overrides applied to board.h
define REGISTER_VARIANT =

# Objects
$(1)_OBJ_DIR = build-$(1)
$(1)_BUILD_OBJS = $$(SRCS:%.c=$$($(1)_OBJ_DIR)/%.o)
$(1)_BUILD_OBJSA = $$(ASRCS:%.S=$$($(1)_OBJ_DIR)/%.o)
$(1)_OBJS = $$($(1)_BUILD_OBJSA) $$($(1)_BUILD_OBJS)

build:: $$($(1)_OBJ_DIR)/$$(TARGET)-boot.elf $$($(1)_OBJ_DIR)/$$(TARGET)-boot.bin $$($(1)_OBJ_DIR)/$$(TARGET)-fel.elf $$($(1)_OBJ_DIR)/$$(TARGET)-fel.bin

.PRECIOUS : $$($(1)_OBJS)
$$($(1)_OBJ_DIR)/$$(TARGET)-fel.elf: $$($(1)_OBJS)
	echo "  LD    $$@"
	$$(CC) -E -P -x c -D__RAM_BASE=0x00028000 ./arch/arm32/mach-t113s3/link.ld > $$($(1)_OBJ_DIR)/link-fel.ld
	$$(CC) $$^ -o $$@ $(LIB_DIR) -T $$($(1)_OBJ_DIR)/link-fel.ld $$(LDFLAGS) -Wl,-Map,$$($(1)_OBJ_DIR)/$$(TARGET)-fel.map

$$($(1)_OBJ_DIR)/$$(TARGET)-boot.elf: $$($(1)_OBJS)
	echo "  LD    $$@"
	$$(CC) -E -P -x c -D__RAM_BASE=0x00020000 ./arch/arm32/mach-t113s3/link.ld > $$($(1)_OBJ_DIR)/link-boot.ld
	$$(CC) $$^ -o $$@ $(LIB_DIR) -T $$($(1)_OBJ_DIR)/link-boot.ld $$(LDFLAGS) -Wl,-Map,$$($(1)_OBJ_DIR)/$$(TARGET)-boot.map

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
	cp $$< $$@
	$(foreach opt,$(2),sed -i "s/^#define $(word 1,$(subst =, ,$(opt))).*/#define $(word 1,$(subst =, ,$(opt))) $(word 2,$(subst =, ,$(opt)))/" $$@;)

clean::
	rm -rf $$($(1)_OBJ_DIR)

-include $$(patsubst %.o,%.d,$$($(1)_OBJS))

endef

# build image with no storage support
ifneq ($(filter fel,$(BUILD_VARIANTS)),)
$(eval $(call REGISTER_VARIANT,fel,CONFIG_BOOT_SPINAND=0 CONFIG_BOOT_SDCARD=0 CONFIG_BOOT_MMC=0))
endif

ifneq ($(filter spi,$(BUILD_VARIANTS)),)
$(eval $(call REGISTER_VARIANT,spi,CONFIG_BOOT_SPINAND=1 CONFIG_BOOT_SDCARD=0 CONFIG_BOOT_MMC=0))
endif

# build sd/mmc only image without spi
ifneq ($(filter sdmmc,$(BUILD_VARIANTS)),)
$(eval $(call REGISTER_VARIANT,sdmmc,CONFIG_BOOT_SPINAND=0 CONFIG_BOOT_SDCARD=1 CONFIG_BOOT_MMC=1))
endif

# build emmc only image without spi
ifneq ($(filter emmc,$(BUILD_VARIANTS)),)
$(eval $(call REGISTER_VARIANT,emmc,CONFIG_BOOT_SPINAND=0 CONFIG_BOOT_SDCARD=0 CONFIG_BOOT_MMC=1))
endif

# build image with everything
ifneq ($(filter all,$(BUILD_VARIANTS)),)
$(eval $(call REGISTER_VARIANT,all,CONFIG_BOOT_SPINAND=1 CONFIG_BOOT_SDCARD=1 CONFIG_BOOT_MMC=1))
endif

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
ifneq ($(filter fel,$(BUILD_VARIANTS)),)
	echo "FEL:"
	$(SIZE) build-fel/$(TARGET)-boot.elf
	cp -f build-fel/$(TARGET)-boot.bin $(TARGET)-boot-fel.bin
	tools/mksunxi $(TARGET)-boot-fel.bin 512
endif

ifneq ($(filter spi,$(BUILD_VARIANTS)),)
	echo "SPI:"
	$(SIZE) build-spi/$(TARGET)-boot.elf
	cp -f build-spi/$(TARGET)-boot.bin $(TARGET)-boot-spi.bin
	cp -f build-spi/$(TARGET)-boot.bin $(TARGET)-boot-spi-4k.bin
	tools/mksunxi $(TARGET)-boot-spi.bin 8192
	tools/mksunxi $(TARGET)-boot-spi-4k.bin 8192 4096
endif

ifneq ($(filter sdmmc,$(BUILD_VARIANTS)),)
	echo "SDMMC:"
	$(SIZE) build-sdmmc/$(TARGET)-boot.elf
	cp -f build-sdmmc/$(TARGET)-boot.bin $(TARGET)-boot-sd.bin
	tools/mksunxi $(TARGET)-boot-sd.bin 512
endif

ifneq ($(filter emmc,$(BUILD_VARIANTS)),)
	echo "eMMC:"
	$(SIZE) build-emmc/$(TARGET)-boot.elf
	cp -f build-emmc/$(TARGET)-boot.bin $(TARGET)-boot-emmc.bin
	tools/mksunxi $(TARGET)-boot-emmc.bin 512
endif

ifneq ($(filter all,$(BUILD_VARIANTS)),)
	echo "ALL:"
	$(SIZE) build-all/$(TARGET)-boot.elf
	cp -f build-all/$(TARGET)-boot.bin $(TARGET)-boot-all.bin
	cp -f build-all/$(TARGET)-boot.bin $(TARGET)-fel.bin
	tools/mksunxi $(TARGET)-fel.bin 512
	tools/mksunxi $(TARGET)-boot-all.bin 8192
endif

spi-boot.img: mkboot
	rm -f spi-boot.img
	dd if=$(TARGET)-boot-spi.bin of=spi-boot.img bs=2k
	dd if=$(TARGET)-boot-spi.bin of=spi-boot.img bs=2k seek=32 # Second copy on page 32
	dd if=$(TARGET)-boot-spi.bin of=spi-boot.img bs=2k seek=64 # Third copy on page 64
	# dd if=linux/boot/$(DTB) of=spi-boot.img bs=2k seek=128 # DTB on page 128
	# dd if=linux/boot/$(KERNEL) of=spi-boot.img bs=2k seek=256 # Kernel on page 256
