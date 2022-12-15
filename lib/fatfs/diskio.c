/*-----------------------------------------------------------------------*/
/* Low level disk I/O module skeleton for FatFs     (C)ChaN, 2007        */
/*-----------------------------------------------------------------------*/
/* This is a stub disk I/O module that acts as front end of the existing */
/* disk I/O modules and attach it to FatFs module with common interface. */
/*-----------------------------------------------------------------------*/
#include "diskio.h"
#include "ffconf.h"
#include "integer.h"
#include "media.h"

#include "main.h"
#include "sdmmc.h"
#include "debug.h"
#include "dram.h"
#include "sunxi_dma.h"
#include "board.h"

//------------------------------------------------------------------------------
//         Internal variables

static volatile DSTATUS Stat = STA_NOINIT; /* Disk status */
#ifdef CONFIG_FATFS_CACHE_SIZE
static u8 *const cache		= (u8 *)SDRAM_BASE;
static const u32 cache_size = (CONFIG_FATFS_CACHE_SIZE);
static u32		 cache_first, cache_last;
#endif

//------------------------------------------------------------------------------
/* Initialize a Drive                                                    */
/*-----------------------------------------------------------------------*/

DSTATUS disk_initialize(BYTE drv /* Physical drive number (0..) */
)
{
	if (drv)
		return STA_NOINIT;

	Stat &= ~STA_NOINIT;

#ifdef CONFIG_FATFS_CACHE_SIZE
	cache_first = 0xFFFFFFFF - cache_size; // Set to a big sector for a proper init
	cache_last	= 0xFFFFFFFF;
#endif

	return Stat;
}

/*-----------------------------------------------------------------------*/
/* Return Disk Status                                                    */
/*-----------------------------------------------------------------------*/

DSTATUS disk_status(BYTE drv /* Physical drive number (0..) */
)
{
	if (drv)
		return STA_NOINIT;

	return Stat;
}

/*-----------------------------------------------------------------------*/
/* Read Sector(s)                                                        */
/*-----------------------------------------------------------------------*/

DRESULT disk_read(BYTE	drv, /* Physical drive nmuber to identify the drive */
				  BYTE *buff, /* Data buffer to store read data */
				  LBA_t sector, /* Start sector in LBA */
				  UINT	count /* Number of sectors to read */
)
{
	u32 blkread, read_pos, first, last, chunk, bytes;

	if (drv || !count)
		return RES_PARERR;
	if (Stat & STA_NOINIT)
		return RES_NOTRDY;

	first = sector;
	last  = sector + count;
	bytes = count * FF_MIN_SS;

	trace("FATFS: read %u sectors at %u\r\n", count, first);

#ifdef CONFIG_FATFS_CACHE_SIZE
	// Read starts in cache but overflows
	if (first >= cache_first && first < cache_last && last > cache_last) {
		chunk = (cache_last - first) * FF_MIN_SS;
		memcpy(buff, cache + (first - cache_first) * FF_MIN_SS, chunk);
		buff += chunk;
		first += (cache_last - first);
		trace("FATFS: chunk %u first %u\r\n", chunk, first);
	}

	// Read is NOT in the cache
	if (last > cache_last || first < cache_first) {
		trace("FATFS: if %u > %u || %u < %u\r\n", last, cache_last, first, cache_first);

		read_pos	= (first / cache_size) * cache_size; // TODO: check with card max capacity
		cache_first = read_pos;
		cache_last	= read_pos + cache_size;
		blkread		= sdmmc_blk_read(&card0, cache, read_pos, cache_size);

		if (blkread != cache_size) {
			warning("FATFS: MMC read %u/%u blocks\r\n", blkread, cache_size);
			return RES_ERROR;
		}
		trace("FATFS: cached %u sectors (%uKB) at %u/[%u-%u]\r\n", blkread, (blkread * FF_MIN_SS) / 1024, first,
			  read_pos, read_pos + cache_size);
	}

	// Copy from read cache to output buffer
	trace("FATFS: copy %u from 0x%x to 0x%x\r\n", bytes, (cache + ((first - cache_first) * FF_MIN_SS)), buff);
	memcpy(buff, (cache + ((first - cache_first) * FF_MIN_SS)), bytes);

	return RES_OK;
#else
	return (sdmmc_blk_read(&card0, buff, sector, count) == count ? RES_OK : RES_ERROR);
#endif
}

/*-----------------------------------------------------------------------*/
/* Write Sector(s)                                                       */
/*-----------------------------------------------------------------------*/

#if _READONLY == 0
DRESULT disk_write(BYTE		   drv, /* Physical drive nmuber to identify the drive */
				   const BYTE *buff, /* Data to be written */
				   LBA_t	   sector, /* Start sector in LBA */
				   UINT		   count /* Number of sectors to write */
)
{
	return RES_ERROR;
}
#endif /* _READONLY */

/*-----------------------------------------------------------------------*/
/* Miscellaneous drive controls other than data read/write               */
/*-----------------------------------------------------------------------*/
#if _USE_IOCTL == 1
DRESULT disk_ioctl(BYTE	 drv, /* Physical drive number (0..) */
				   BYTE	 ctrl, /* Control code */
				   void *buff /* Buffer to send/receive control data */
)
{
}
#endif

/*----------------------------------------------------------------------*/
/* User Provided RTC Function for Fats module				*/
/*----------------------------------------------------------------------*/
/* Currnet time is returned with packed into a DWORD value.		*/
/* The bit field is as follows:						*/
/*   bit31:25  Year from 1980 (0..127)					*/
/*   bit24:21  Month (1..12)						*/
/*   bit20:16  Day in month(1..31)					*/
/*   bit15:11  Hour (0..23)						*/
/*   bit10:5   Minute (0..59)						*/
/*   bit4:0    Second / 2 (0..29)					*/

DWORD get_fattime(void)
{
	DWORD time;

	time = ((2009 - 1980) << 25) | (9 << 21) | (15 << 16) | (17 << 11) | (45 << 5) | ((59 / 2) << 0);

	return time;
}
