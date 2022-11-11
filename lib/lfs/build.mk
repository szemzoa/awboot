FS_LITTLE := lib/lfs

INCLUDE_DIRS += -I $(FS_LITTLE)/

SRCS	+=  $(FS_LITTLE)/lfs.c
SRCS	+=  $(FS_LITTLE)/lfs_util.c
