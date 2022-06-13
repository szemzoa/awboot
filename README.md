# awboot

Usage: <br/>
    - compile mksunxi tool in the tools directory. <br/>
    - rename the CONFIG_DTB_FILENAME to your board's dts. <br/>
    - compile the bootloader: make. This will generate the bootloader with valid EGON header, usable with the xfel tool. <br/>
    - make a FAT16/32 partition on an SDCARD. <br/>
    - copy zImage and the devicetree to the FAT partition. <br/>
    1. <br/>
    - xfel write 0x30000 awboot.bin <br/>
    - xfel exec 0x30000 <br/>
    2. or boot without xfel: modify the link address in arch/arm32/mach-t113s3/link.ld, ORIGIN = 0x00030000 to ORIGIN = 0x00020000 <br/>
    - and write the awboot to sdcard, example: sudo dd if=awboot.bin of=/dev/(your sd device) bs=1024 seek=8
