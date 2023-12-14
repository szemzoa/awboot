FS_FAT := lib/fatfs

INCLUDE_DIRS += -I $(FS_FAT)

SRCS	+=  $(FS_FAT)/ff.c
SRCS	+=  $(FS_FAT)/diskio.c
SRCS	+=  $(FS_FAT)/ffsystem.c
SRCS	+=  $(FS_FAT)/ffunicode.c
