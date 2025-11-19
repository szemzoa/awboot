# AWBoot

Small linux bootloader for Allwinner T113-s3, T113-s4, V851s

## Building

Run `make VARIANT=all LOG_LEVEL=40`.  
This builds every supported output (fel/spi/sdmmc/emmc) and produces awboot binaries with valid EGON headers.  
`VARIANT` accepts a comma‑separated list (for example `VARIANT=fel,emmc`) if you only need a subset, and `LOG_LEVEL`
continues to control verbosity (default 30 = info).  

## Using

You will need [xfel](https://github.com/xboot/xfel) for uploading the file to memory or SPI flash.  
This it not needed for writing to an SD card.  

### FEL memory boot:
1. Build the variant that includes FEL (`make VARIANT=fel …`).  
2. Provide your kernel/dtb (and optional initrd).  
3. Run the helper script:
```
./tools/fel.sh path/to/zImage path/to/board.dtb [path/to/initrd]
```
The script uploads the freshly built `awboot-fel.bin`, kernel, DTB and optional initrd, updates the FEL mailboxes and
boots the SoC automatically.  

### FEL SPI NOR boot:
```
make VARIANT=spi spi-boot.img
xfel spinor
xfel spinor write 0 spi-boot.img
xfel reset
```

### FEL SPI NAND boot:
```
make VARIANT=spi spi-boot.img
xfel spi_nand
xfel spi_nand write 0 spi-boot.img
xfel reset
```

### SD Card boot:
- create an MBR or GPT partition table and a FAT32 partition with an offset of 4MB or more using fdisk.  
```
sudo fdisk /dev/(your sd device)
```
write awboot-boot.bin to sdcard with an offset of:  
- MBR: 8KB (sector 16)
- GPT: 128KB (sector 256)
```
sudo dd if=awboot-boot-sd.bin of=/dev/(your sd device) bs=1024 seek=8
```
- compile (if needed) and copy your `.dtb` file to the FAT partition.
- copy zImage to the FAT partition.

### Linux kernel:
WIP kernel from here: https://github.com/smaeul/linux/tree/d1/all
