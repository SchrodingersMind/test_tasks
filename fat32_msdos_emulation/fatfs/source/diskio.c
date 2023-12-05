/*-----------------------------------------------------------------------*/
/* Low level disk I/O module SKELETON for FatFs     (C)ChaN, 2019        */
/*-----------------------------------------------------------------------*/
/* If a working storage control module is available, it should be        */
/* attached to the FatFs via a glue function rather than modifying it.   */
/* This is an example of glue functions to attach various exsisting      */
/* storage control modules to the FatFs module with a defined API.       */
/*-----------------------------------------------------------------------*/

#include "ff.h"			/* Obtains integer types */
#include "diskio.h"		/* Declarations of disk functions */

/* Definitions of physical drive number for each drive */
#define DEV_RAM		0	/* Example: Map Ramdisk to physical drive 0 */
#define DEV_MMC		1	/* Example: Map MMC/SD card to physical drive 1 */
#define DEV_USB		2	/* Example: Map USB MSD to physical drive 2 */


#define MAX_DISKS 256
#define	BUFSIZE 262144UL	/* Size of data transfer buffer */
#define DEFAULT_SZ_SECTOR 512
#define DEFAULT_SECTORS 2e+7/DEFAULT_SZ_SECTOR

typedef struct {
	DSTATUS	status;
	WORD sz_sector;
	DWORD n_sectors;
	FILE* h_drive;
} STAT;

static volatile STAT Stat[MAX_DISKS];

BYTE *vmem = (void*)0;

/*-----------------------------------------------------------------------*/
/* Get Drive Status                                                      */
/*-----------------------------------------------------------------------*/

DSTATUS disk_status (
	BYTE pdrv		/* Physical drive nmuber to identify the drive */
)
{
	DSTATUS stat;
	int result;
	// printf("[+] Checking disk status\n");

	// There is only one disk, so don't check pdrv value
	// switch (pdrv) {
	// case DEV_RAM :
	// 	result = RAM_disk_status();

	// 	// translate the reslut code here

	// 	return stat;

	// case DEV_MMC :
	// 	result = MMC_disk_status();

	// 	// translate the reslut code here

	// 	return stat;

	// case DEV_USB :
	// 	result = USB_disk_status();

	// 	// translate the reslut code here

	// 	return stat;
	// }
	if (vmem == (void*)0) {
		return STA_NOINIT;
	}
	return RES_OK;
}

DRESULT disk_reformat ()
{
	char tmp_vmem[FF_MAX_SS*10] = {'\0'};
	bzero(vmem, DEFAULT_SECTORS*DEFAULT_SZ_SECTOR);
	// tmp_vmem = mmap(0, DEFAULT_SECTORS*DEFAULT_SZ_SECTOR, PROT_READ|PROT_WRITE, MAP_SHARED|MAP_ANONYMOUS, -1, 0); //malloc(DEFAULT_SECTORS*DEFAULT_SZ_SECTOR);
	return f_mkfs("", FM_SFD|FM_FAT, 0, tmp_vmem, sizeof(tmp_vmem));
	// munmap(tmp_vmem, )
	
}



/*-----------------------------------------------------------------------*/
/* Inidialize a Drive                                                    */
/*-----------------------------------------------------------------------*/

DSTATUS disk_initialize (
	char* path,				/* Physical drive to identify the drive */
	FILE** out
)
{
	DSTATUS stat;
	int result;
	char tmp_buf[DEFAULT_SZ_SECTOR];
	char *tmp_vmem;
	int is_file_exist;

	if (vmem != (BYTE*)0) {
		// printf("[+] Returning initialized vmem\n");
		return RES_OK;
	}

	// Just try to "re-open"
	if (path == (void*)0) {
		if (vmem == (void*)0) {
			return STA_NOINIT;
		}
		return RES_OK;
	}

	is_file_exist = access(path, F_OK) == 0;

	if (is_file_exist) {
		// File exists, just open it
		printf("[+] Disk exists, just open it\n");
		*out = fopen(path, "r+");
	} else {
		printf("[+] Create new disk\n");
		*out = fopen(path, "w+");
	}

	if (*out == 0) {
		printf("[-] Error opening disk file!!!\n");
		perror("fopen");
		return RES_ERROR;
	}
	if (!is_file_exist) {
		ftruncate(fileno(*out), DEFAULT_SECTORS*DEFAULT_SZ_SECTOR);
	}

	vmem = mmap(0, DEFAULT_SECTORS*DEFAULT_SZ_SECTOR, PROT_READ|PROT_WRITE, MAP_SHARED, fileno(*out), 0);
	if (vmem == MAP_FAILED) {
		vmem = (void*)0;
		printf("[-] Unable to mmap file to memory!!");
		perror("mmap");
		return RES_ERROR;
	}

	// printf("[+] New vmem could be found at %p\n", vmem);

	if (!is_file_exist) {
		// printf("[+] Formating existing file\n");
		disk_reformat();
	}

	
	if (vmem == (BYTE*)0)
		return STA_NOINIT;
	return RES_OK;
}



/*-----------------------------------------------------------------------*/
/* Read Sector(s)                                                        */
/*-----------------------------------------------------------------------*/

DRESULT disk_read (
	BYTE pdrv,		/* Physical drive nmuber to identify the drive */
	BYTE *buff,		/* Data buffer to store read data */
	DWORD sector,	/* Start sector in LBA */
	UINT count		/* Number of sectors to read */
)
{
	DRESULT res;
	int result;
	// printf("[+] Reading disk\n");

	// if (fseek(fp, sector*DEFAULT_SZ_SECTOR, SEEK_SET) < 0) {
	// 	return RES_PARERR;
	// }
	// if (fread(buff, count, DEFAULT_SZ_SECTOR, fp) != count*DEFAULT_SZ_SECTOR) {
	// 	return RES_PARERR;
	// }
	if (vmem == (void*)0) {
		return STA_NOINIT;
	}
	memcpy(buff, &vmem[sector*DEFAULT_SZ_SECTOR], count*DEFAULT_SZ_SECTOR);
	return RES_OK;
}



/*-----------------------------------------------------------------------*/
/* Write Sector(s)                                                       */
/*-----------------------------------------------------------------------*/

#if FF_FS_READONLY == 0

DRESULT disk_write (
	BYTE pdrv,			/* Physical drive nmuber to identify the drive */
	const BYTE *buff,	/* Data to be written */
	DWORD sector,		/* Start sector in LBA */
	UINT count			/* Number of sectors to write */
)
{
	DRESULT res;
	int result;
	// printf("[+] Writing disk\n");
	// if (fseek(fp, sector*DEFAULT_SZ_SECTOR, SEEK_SET) < 0) {
	// 	return RES_PARERR;
	// }
	// if (fwrite(buff, count, DEFAULT_SZ_SECTOR, fp) != count*DEFAULT_SZ_SECTOR) {
	// 	return RES_PARERR;
	// }

	if (vmem == (void*)0) {
		return STA_NOINIT;
	}
	memcpy(&vmem[sector*DEFAULT_SZ_SECTOR], buff, count*DEFAULT_SZ_SECTOR);

	return RES_OK;
}

#endif


/*-----------------------------------------------------------------------*/
/* Miscellaneous Functions                                               */
/*-----------------------------------------------------------------------*/

DRESULT disk_ioctl (
	BYTE pdrv,		/* Physical drive nmuber (0..) */
	BYTE cmd,		/* Control code */
	void *buff		/* Buffer to send/receive control data */
)
{
	DRESULT res;
	int result;

	// printf("[+] IOCTL disk\n");

	if (cmd == GET_SECTOR_SIZE || cmd == GET_BLOCK_SIZE) {
	  *((WORD*)buff) = DEFAULT_SZ_SECTOR;
	  return RES_OK;
	}
	if (cmd == GET_SECTOR_COUNT) {
	  *((DWORD*)buff) = DEFAULT_SECTORS;
	  return RES_OK;	  
	}
	if (cmd == CTRL_SYNC) {
	  return RES_OK;	  
	}
	return RES_PARERR;
}


DWORD get_fattime() { return 0; };

void disk_flush() {
}


