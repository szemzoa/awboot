# awboot

Usage:
    - compile mksunxi tool in the tools directory.
    - rename the CONFIG_DTB_FILENAME to your board's dts.
    - compile the bootloader: make. This will generate the bootloader with valid EGON header, usable with the xfel tool.
    - make a FAT16/32 partition on an SDCARD.
    - copy zImage and the devicetree to the FAT partition
    - xfel write 0x30000 awboot.bin
    - xfel exec 0x30000
