/*---------------------------------------------------------------------------/
/  FatFs - FAT file system module configuration file  R0.12  (C)ChaN, 2016
/---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------/
/ Function Configurations
/---------------------------------------------------------------------------*/


/*---------------------------------------------------------------------------/
/ Locale and Namespace Configurations
/---------------------------------------------------------------------------*/

#define	_USE_LFN	2
#define	_MAX_LFN	255
/* The _USE_LFN switches the support of long file name (LFN).
/
/   0: Disable support of LFN. _MAX_LFN has no effect.
/   1: Enable LFN with static working buffer on the BSS. Always NOT thread-safe.
/   2: Enable LFN with dynamic working buffer on the STACK.
/   3: Enable LFN with dynamic working buffer on the HEAP.
/
/  To enable the LFN, Unicode handling functions (option/unicode.c) must be added
/  to the project. The working buffer occupies (_MAX_LFN + 1) * 2 bytes and
/  additional 608 bytes at exFAT enabled. _MAX_LFN can be in range from 12 to 255.
/  It should be set 255 to support full featured LFN operations.
/  When use stack for the working buffer, take care on stack overflow. When use heap
/  memory for the working buffer, memory management functions, ff_memalloc() and
/  ff_memfree(), must be added to the project. */


#define	_LFN_UNICODE	0
/* This option switches character encoding on the API. (0:ANSI/OEM or 1:Unicode)
/  To use Unicode string for the path name, enable LFN and set _LFN_UNICODE = 1.
/  This option also affects behavior of string I/O functions. */


#define _STRF_ENCODE	3
/* When _LFN_UNICODE == 1, this option selects the character encoding on the file to
/  be read/written via string I/O functions, f_gets(), f_putc(), f_puts and f_printf().
/
/  0: ANSI/OEM
/  1: UTF-16LE
/  2: UTF-16BE
/  3: UTF-8
/
/  This option has no effect when _LFN_UNICODE == 0. */


#define _FS_RPATH	2
/* This option configures support of relative path.
/
/   0: Disable relative path and remove related functions.
/   1: Enable relative path. f_chdir() and f_chdrive() are available.
/   2: f_getcwd() function is available in addition to 1.
*/

#define _DIR_SUPPORT 0

#define _FS_EXFAT 0

#define _FS_READONLY 0

#define _FS_NORTC 0

#define _FS_REENTRANT 0

#define _USE_FIND 0

#define _USE_FORWARD 0

#define _USE_EXPAND 0

#define _USE_MKFS 0

/*---------------------------------------------------------------------------/
/ Drive/Volume Configurations
/---------------------------------------------------------------------------*/

#define _VOLUMES	2
/* Number of volumes (logical drives) to be used. */


#define _STR_VOLUME_ID	0
#define _VOLUME_STRS	"RAM","NAND","CF","SD1","SD2","USB1","USB2","USB3"
/* _STR_VOLUME_ID switches string support of volume ID.
/  When _STR_VOLUME_ID is set to 1, also pre-defined strings can be used as drive
/  number in the path name. _VOLUME_STRS defines the drive ID strings for each
/  logical drives. Number of items must be equal to _VOLUMES. Valid characters for
/  the drive ID strings are: A-Z and 0-9. */


#define	_MULTI_PARTITION	0
/* This option switches support of multi-partition on a physical drive.
/  By default (0), each logical drive number is bound to the same physical drive
/  number and only an FAT volume found on the physical drive will be mounted.
/  When multi-partition is enabled (1), each logical drive number can be bound to
/  arbitrary physical drive and partition listed in the VolToPart[]. Also f_fdisk()
/  funciton will be available. */


#define _FS_NOFSINFO	0
/* If you need to know correct free space on the FAT32 volume, set bit 0 of this
/  option, and f_getfree() function at first time after volume mount will force
/  a full FAT scan. Bit 1 controls the use of last allocated cluster number.
/
/  bit0=0: Use free cluster count in the FSINFO if available.
/  bit0=1: Do not trust free cluster count in the FSINFO.
/  bit1=0: Use last allocated cluster number in the FSINFO if available.
/  bit1=1: Do not trust last allocated cluster number in the FSINFO.
*/

