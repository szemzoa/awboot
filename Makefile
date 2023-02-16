# Target
TARGET := awboot
CROSS_COMPILE ?= arm-none-eabi

# Log level defaults to info
LOG_LEVEL ?= 30

SRCS := main.c board.c lib/debug.c lib/xformat.c lib/fdt.c lib/string.c

INCLUDE_DIRS :=-I . -I include -I lib
LIB_DIR := -L ./
LIBS := -lgcc -nostdlib
DEFINES := -DLOG_LEVEL=$(LOG_LEVEL) -DBUILD_REVISION=$(shell cat .build_revision)

include	arch/arch.mk
include	lib/fatfs/fatfs.mk

CFLAGS += -mcpu=cortex-a7 -mthumb-interwork -mthumb -mno-unaligned-access -mfpu=neon-vfpv4 -mfloat-abi=hard
CFLAGS += -ffast-math -Os -std=gnu99 -Wall -g $(INCLUDES) $(DEFINES)

ASFLAGS += $(CFLAGS)

LDFLAGS += $(CFLAGS) $(LIBS)

STRIP=$(CROSS_COMPILE)-strip
CC=$(CROSS_COMPILE)-gcc
SIZE=$(CROSS_COMPILE)-size
OBJCOPY=$(CROSS_COMPILE)-objcopy

HOSTCC=gcc
HOSTSTRIP=strip

DATE=/bin/date
CAT=/bin/cat
ECHO=/bin/echo
WORKDIR=$(/bin/pwd)
MAKE=make

# Objects
EXT_OBJS =
OBJ_DIR = build
BUILD_OBJS = $(SRCS:%.c=$(OBJ_DIR)/%.o)
BUILD_OBJSA = $(ASRCS:%.S=$(OBJ_DIR)/%.o)
OBJS = $(BUILD_OBJSA) $(BUILD_OBJS) $(EXT_OBJS)

DTB ?= sun8i-t113-mangopi-dual.dtb
KERNEL ?= zImage

all: git begin build mkboot

begin:
	@echo "---------------------------------------------------------------"
	@echo -n "Compiler version: "
	@$(CC) -v 2>&1 | tail -1

build_revision:
	@/bin/expr `cat .build_revision` + 1 > .build_revision

.PHONY: tools boot.img
.SILENT:

git:
	cp -f tools/hooks/* .git/hooks/

build: build_revision $(TARGET)-boot.elf $(TARGET)-boot.bin $(TARGET)-fel.elf $(TARGET)-fel.bin

.SECONDARY : $(TARGET)
.PRECIOUS : $(OBJS)
$(TARGET)-fel.elf: $(OBJS)
	echo "  LD    $@"
	$(CC) -E -P -x c -D__RAM_BASE=0x00030000 ./arch/arm32/mach-t113s3/link.ld > build/link-fel.ld
	$(CC) $^ -o $@ $(LIB_DIR) -T build/link-fel.ld $(LDFLAGS) -Wl,-Map,$(TARGET)-fel.map

$(TARGET)-boot.elf: $(OBJS)
	echo "  LD    $@"
	$(CC) -E -P -x c -D__RAM_BASE=0x00020000 ./arch/arm32/mach-t113s3/link.ld > build/link-boot.ld
	$(CC) $^ -o $@ $(LIB_DIR) -T build/link-boot.ld $(LDFLAGS) -Wl,-Map,$(TARGET)-boot.map

$(TARGET)-fel.bin: $(TARGET)-fel.elf
	@echo OBJCOPY $@
	$(OBJCOPY) -O binary $< $@
	$(SIZE) $(TARGET)-fel.elf

$(TARGET)-boot.bin: $(TARGET)-boot.elf
	@echo OBJCOPY $@
	$(OBJCOPY) -O binary $< $@
	$(SIZE) $(TARGET)-boot.elf

$(OBJ_DIR)/%.o : %.c
	echo "  CC    $<"
	mkdir -p $(@D)
	$(CC) $(CFLAGS) $(INCLUDE_DIRS) -c $< -o $@

$(OBJ_DIR)/%.o : %.S
	echo "  CC    $<"
	mkdir -p $(@D)
	$(CC) $(ASFLAGS) $(INCLUDE_DIRS) -c $< -o $@

clean:
	rm -rf $(OBJ_DIR)
	rm -f $(TARGET)
	rm -f $(TARGET)-*.bin
	rm -f $(TARGET)-*.map
	rm -f $(TARGET)-*.elf
	rm -f *.img
	$(MAKE) -C tools clean

format:
	find . -iname "*.h" -o -iname "*.c" | xargs clang-format --verbose -i

tools:
	$(MAKE) -C tools all

mkboot: build tools
	cp $(TARGET)-boot.bin $(TARGET)-boot-spi.bin
	cp $(TARGET)-boot.bin $(TARGET)-boot-sd.bin
	tools/mksunxi $(TARGET)-fel.bin 8192
	tools/mksunxi $(TARGET)-boot-spi.bin 8192
	tools/mksunxi $(TARGET)-boot-sd.bin 512

spi-boot.img: mkboot
	rm -f spi-boot.img
	dd if=$(TARGET)-boot-spi.bin of=spi-boot.img bs=2k
	dd if=$(TARGET)-boot-spi.bin of=spi-boot.img bs=2k seek=32 # Second copy on page 32
	dd if=$(TARGET)-boot-spi.bin of=spi-boot.img bs=2k seek=64 # Third copy on page 64
	# dd if=linux/boot/$(DTB) of=spi-boot.img bs=2k seek=128 # DTB on page 128
	# dd if=linux/boot/$(KERNEL) of=spi-boot.img bs=2k seek=256 # Kernel on page 256
