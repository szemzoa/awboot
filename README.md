# awboot

Usage: <br/>
    - compile mksunxi tool in the tools directory. <br/>
    - rename the CONFIG_DTB_FILENAME to your board's dts. <br/>
    - compile the bootloader: make. This will generate the bootloader with valid EGON header, usable with the xfel tool. <br/>
    - make a FAT16/32 partition on an SDCARD. <br/>
    - copy zImage and the devicetree to the FAT partition. <br/>
    - xfel write 0x30000 awboot.bin <br/>
    - xfel exec 0x30000 <br/>

