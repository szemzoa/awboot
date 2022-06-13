FS_FAT:=$(TOPDIR)/lib/fatfs

INCLUDE_DIRS += -I $(FS_FAT)/

SRCS	+=  $(FS_FAT)/ff.o
SRCS	+=  $(FS_FAT)/diskio.o
SRCS	+=  $(FS_FAT)/option/ccsbcs.o


