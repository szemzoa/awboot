# AWBoot

Small linux bootloader for Allwinner T113-S3

## Building

Run `make`.  
This will generate the bootloader with a valid EGON header, usable with the xfel tool or BOOTROM  

## Using

You will need [xfel](https://github.com/xboot/xfel) for uploading the file to memory or SPI flash.  
This it not needed for writing to an SD card.  

### FEL memory boot:
```
xfel write 0x30000 awboot-fel.bin
xfel exec 0x30000
```

### FEL SPI NOR boot:
```
xfel spinor
xfel spinor write 0 boot.img
xfel reset
```

### FEL SPI NAND boot:
```
xfel spinand
xfel spinand write 0 boot.img
xfel reset
```

### SD Card boot:
- create an MBR partition table and a FAT32 partition with an offset of 40KB using fdisk.  
```
sudo fdisk /dev/(your sd device)
```
- write awboot-boot.bin to sdcard with an offset of 8KB (16 sectors)  
```
sudo dd if=awboot-boot.bin of=/dev/(your sd device) bs=1024 seek=8
```
- compile (if needed) and copy your `.dtb` file to the FAT partition.
- copy zImage to the FAT partition.

### Linux kernel:
WIP kernel from here: https://github.com/smaeul/linux/tree/d1/all
