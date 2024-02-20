LIB := lib
FS_FAT := $(LIB)/fatfs
EXT2_FS := $(LIB)/ext2fs

INCLUDE_DIRS += -I $(FS_FAT) -I $(LIB)  -I $(EXT2_FS)

SRCS    +=  $(FS_FAT)/ff.c
SRCS    +=  $(FS_FAT)/diskio.c
SRCS    +=  $(FS_FAT)/ffsystem.c
SRCS    +=  $(FS_FAT)/ffunicode.c

SRCS	+=  $(LIB)/debug.c
SRCS	+=  $(LIB)/nutheap.c
SRCS	+=  $(LIB)/console.c
SRCS	+=  $(LIB)/fdt.c
SRCS	+=  $(LIB)/ringbuffer.c
SRCS	+=  $(LIB)/string.c
SRCS	+=  $(LIB)/strtok.c
SRCS	+=  $(LIB)/xformat.c

SRCS    +=  $(EXT2_FS)/ext2.c
SRCS    +=  $(EXT2_FS)/file.c
SRCS    +=  $(EXT2_FS)/bcache.c
SRCS    +=  $(EXT2_FS)/bio.c
SRCS    +=  $(EXT2_FS)/io.c
#SRCS    +=  $(EXT2_FS)/fs.c
SRCS    +=  $(EXT2_FS)/dir.c
SRCS    +=  $(EXT2_FS)/partition.c
