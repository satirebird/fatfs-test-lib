/*
 * ff_linux.c
 *
 *  Created on: 07.04.2020
 *      Author: sven
 */

#define DIR FATFS_DIR
#include "ff.h"
#undef DIR

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <time.h>
#include <sys/time.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/statvfs.h>

#define DIR SYS_DIR
#include <dirent.h>
#undef DIR

#include <unistd.h>
#include <errno.h>

#include <libgen.h>

static FATFS *FatFs[_VOLUMES];
#if _FS_RPATH != 0 && _VOLUMES >= 2
static int CurrVol = 0;
#endif

#define IsUpper(c)	(((c)>='A')&&((c)<='Z'))
#define IsLower(c)	(((c)>='a')&&((c)<='z'))
#define IsDigit(c)	(((c)>='0')&&((c)<='9'))

#define _MAX_PATH_LENGTH 1024

/**
 * Returns logical drive number (-1:invalid drive)
 *
 * @param path Pointer to pointer to the path name
 * @return The logical drive number (-1:invalid drive)
 */
static int get_ldnumber(const TCHAR** path)
{
	const TCHAR *tp, *tt;
	UINT i;
	int vol = -1;
#if _STR_VOLUME_ID		/* Find string drive id */
	static const char* const str[] = {_VOLUME_STRS};
	const char *sp;
	char c;
	TCHAR tc;
#endif


	if (*path) {	/* If the pointer is not a null */
		for (tt = *path; (UINT)*tt >= (_USE_LFN ? ' ' : '!') && *tt != ':'; tt++) ;	/* Find ':' in the path */
		if (*tt == ':') {	/* If a ':' is exist in the path name */
			tp = *path;
			i = *tp++ - '0';
			if (i < 10 && tp == tt) {	/* Is there a numeric drive id? */
				if (i < _VOLUMES) {	/* If a drive id is found, get the value and strip it */
					vol = (int)i;
					*path = ++tt;
				}
			}
#if _STR_VOLUME_ID
			 else {	/* No numeric drive number, find string drive id */
				i = 0; tt++;
				do {
					sp = str[i]; tp = *path;
					do {	/* Compare a string drive id with path name */
						c = *sp++; tc = *tp++;
						if (IsLower(tc)) tc -= 0x20;
					} while (c && (TCHAR)c == tc);
				} while ((c || tp != tt) && ++i < _VOLUMES);	/* Repeat for each id until pattern match */
				if (i < _VOLUMES) {	/* If a drive id is found, get the value and strip it */
					vol = (int)i;
					*path = tt;
				}
			}
#endif
			return vol;
		}
#if _FS_RPATH != 0 && _VOLUMES >= 2
		vol = CurrVol;	/* Current drive */
#else
		vol = 0;		/* Drive 0 */
#endif
	}
	return vol;
}

static FRESULT get_fspath(char *dest, const TCHAR *path){

	const TCHAR *rp = path;

	int vol = get_ldnumber(&rp);
	if (vol < 0)
		return FR_INVALID_DRIVE;

	FATFS *fs = FatFs[vol];

  if (fs == NULL)
    return FR_NO_FILESYSTEM;

	if (fs->mount_point == NULL)
	  return FR_NO_FILESYSTEM;

	strcpy(dest, fs->mount_point);
	if (rp[0] != '/')
		strcat(dest, "/");
	strcat(dest, path);

	return FR_OK;
}

static WORD get_fatfsdate(struct timespec *tv)
{
  WORD result = 0;
  int year;
  struct tm tm;

  (void)localtime_r(&tv->tv_sec, &tm);

  year = tm.tm_year;
  if (year < 80)
    year = 80;

  result |= ((unsigned int)((year - 80) & 0x7F) << 25);
  result |= ((unsigned int)((tm.tm_mon + 1) & 0x0F) << 21) ;
  result |= ((unsigned int)(tm.tm_mday & 0x1F) << 16);

  return result;
}

static WORD get_fatfstime(struct timespec *tv)
{
  WORD result = 0;
  struct tm tm;

  (void)localtime_r(&tv->tv_sec, &tm);

  result |= ((unsigned int)(tm.tm_hour & 0x1F) << 11);
  result |= ((unsigned int)(tm.tm_min & 0x3F) << 5);
  result |= ((unsigned int)((tm.tm_sec / 2) & 0x1F));

  return result;
}


static void get_fataltname(TCHAR *dest, const TCHAR *basename){
  strncpy(dest, basename, 13); //dirty hack?
}

static FRESULT errno_to_fresult(void){
	switch(errno){
	case EBADF:
	case EROFS:
	case EINVAL:
		return FR_INVALID_OBJECT;
	case EIO:
	case ENOSPC:
		return FR_DISK_ERR;
	case ENOENT:
		return FR_NO_FILE;
	case EACCES:
		return FR_DENIED;
	default:
		return FR_INT_ERR;
	}
}

FRESULT f_mount (FATFS* fs, const TCHAR* path, BYTE opt){

	int vol;
	const TCHAR *rp = path;

	if (path == NULL)
		path = ".";
	if (strlen(path) == 0)
		path = ".";

	if (fs == NULL)
		return FR_NOT_ENABLED;

	vol = get_ldnumber(&rp);
	if (vol < 0)
		return FR_INVALID_DRIVE;
	// The volume is stripped from rp
	// Check if we must create the directory
	if (mkdir(rp, 0777) < 0){
	  if (errno != EEXIST)
	    return errno_to_fresult();
	}

	// Create the mount point and ensure the trailing path separator is cutted
	char *pp = strdup(rp);
	if (pp == NULL)
		return FR_INT_ERR;
	size_t pp_last = strlen(pp) - 1;
	if (pp[pp_last] == '/')
		pp[pp_last] = 0;

	fs->mount_point = pp;

	FatFs[vol] = fs;

	return FR_OK;
}

FRESULT f_open (FIL* fp, const TCHAR* path, BYTE mode){

	char pp[_MAX_PATH_LENGTH];
	FRESULT res = get_fspath(pp, path);
	if (res != FR_OK)
		return res;

	char px_mode[16] = "";
	if (mode & FA_CREATE_ALWAYS){
		strcat(px_mode, "w");
		if (mode & FA_READ)
			strcat(px_mode, "+");
	}else if (mode & FA_OPEN_ALWAYS){
		strcat(px_mode, "a");
		if (mode & FA_READ)
			strcat(px_mode, "+");
  }else if ((mode & (FA_CREATE_NEW | FA_READ)) == (FA_CREATE_NEW | FA_READ)){
    strcat(px_mode, "w+x");
	}else if (mode & FA_CREATE_NEW){
		strcat(px_mode, "wx");
	}else if (mode & FA_READ){
		strcat(px_mode, "r");
		if (mode & FA_WRITE)
			strcat(px_mode, "+");
	}else if (mode & FA_WRITE){
    strcat(px_mode, "w");
  }

	fp->fp = fopen(pp, px_mode);
	if (fp->fp == NULL)
		errno_to_fresult();

	if (mode & FA_OPEN_ALWAYS)
		if (fseek(fp->fp, 0, SEEK_SET) < 0)
			return errno_to_fresult();

	return FR_OK;
}

FRESULT f_close (FIL* fp){

	int res = 0;

	if (fp->fp){
		res = fclose(fp->fp);
		fp->fp = NULL;
	}

	if (res < 0)
		return errno_to_fresult();

	return FR_OK;
}


FRESULT f_read (FIL* fp, void* buff, UINT btr, UINT* br){

	if (fp->fp == NULL)
		return FR_INVALID_OBJECT;

	size_t res = fread(buff, 1, btr, fp->fp);

	if (br)
		*br = res;

	if (ferror(fp->fp) != 0)
		return errno_to_fresult();

	return FR_OK;
}

FRESULT f_write (FIL* fp, const void* buff, UINT btw, UINT* bw){

	if (fp->fp == NULL)
		return FR_INVALID_OBJECT;

	size_t res = fwrite(buff, 1, btw, fp->fp);

	if (bw)
		*bw = res;

	if (ferror(fp->fp) != 0)
		return errno_to_fresult();

	return FR_OK;

}

FRESULT f_lseek (FIL* fp, FSIZE_t ofs){

	if (fp->fp == NULL)
		return FR_INVALID_OBJECT;

	if (fseek(fp->fp, ofs, SEEK_SET) < 0)
		return errno_to_fresult();

	return FR_OK;
}

FRESULT f_truncate (FIL* fp){

	if (fp->fp == NULL)
		return FR_INVALID_OBJECT;

	if (ftruncate(fileno(fp->fp), ftello(fp->fp)) < 0)
		return errno_to_fresult();

	return FR_OK;
}

FRESULT f_sync (FIL* fp){

	if (fp->fp == NULL)
		return FR_INVALID_OBJECT;

	if (fsync(fileno(fp->fp)) < 0)
		return errno_to_fresult();

	return FR_OK;

}

FRESULT f_opendir (FATFS_DIR* dp, const TCHAR* path){

  char pp[_MAX_PATH_LENGTH];
  FRESULT res = get_fspath(pp, path);
  if (res != FR_OK)
    return res;

  dp->dir = opendir(pp);
  if (dp->dir == NULL)
    return errno_to_fresult();

  return FR_OK;
}

FRESULT f_closedir (FATFS_DIR* dp){

  if (dp->dir == NULL)
    return FR_INVALID_OBJECT;

  if (closedir(dp->dir) < 0)
    return errno_to_fresult();
  return FR_OK;
}

FRESULT f_readdir (FATFS_DIR* dp, FILINFO* fno){

  if (dp->dir == NULL)
    return FR_INVALID_OBJECT;

  struct dirent *dirent;

  errno = 0;
  dirent = readdir(dp->dir);
  if (dirent == NULL){
    if (errno != 0){
      return errno_to_fresult();
    }else{
      // end of directory
      fno->fname[0] = 0;
      return FR_OK;
    }
  }

#if _USE_LFN != 0
  strncpy(fno->fname, dirent->d_name, _MAX_LFN);
  get_fataltname(fno->altname, dirent->d_name);
#else
  get_fataltname(fno->altname, dirent->d_name);
#endif

  fno->fsize = 0; //Some sizes needed?
  fno->fattrib = 0;

  switch (dirent->d_type){
  case DT_DIR:
    fno->fattrib |= AM_DIR;
    break;
  case DT_BLK:
  case DT_CHR:
  case DT_SOCK:
    fno->fattrib |= AM_SYS;
    break;
  }

  return FR_OK;
}

#if _USE_FIND
FRESULT f_findfirst (FATFS_DIR* dp, FILINFO* fno, const TCHAR* path, const TCHAR* pattern){

  if (dp->dir == NULL)
    return FR_INVALID_OBJECT;
}

FRESULT f_findnext (FATFS_DIR* dp, FILINFO* fno){

  if (dp->dir == NULL)
    return FR_INVALID_OBJECT;

}
#endif

FRESULT f_mkdir (const TCHAR* path){

	char pp[_MAX_PATH_LENGTH];
	FRESULT res = get_fspath(pp, path);
	if (res != FR_OK)
		return res;

	if (mkdir(pp, 0777) < 0)
		return errno_to_fresult();

	return FR_OK;
}

FRESULT f_unlink (const TCHAR* path){

	char pp[_MAX_PATH_LENGTH];
	FRESULT res = get_fspath(pp, path);
	if (res != FR_OK)
		return res;

	if (unlink(pp) < 0)
		return errno_to_fresult();

	return FR_OK;
}

FRESULT f_rename (const TCHAR* path_old, const TCHAR* path_new){

	char old_pp[_MAX_PATH_LENGTH];
	char new_pp[_MAX_PATH_LENGTH];

	FRESULT res = get_fspath(old_pp, path_old);
	if (res != FR_OK)
		return res;

	res = get_fspath(new_pp, path_new);
	if (res != FR_OK)
		return res;

	if (rename(old_pp, new_pp) < 0)
		return errno_to_fresult();

	return FR_OK;
}


FRESULT f_stat (const TCHAR* path, FILINFO* fno){

	char pp[_MAX_PATH_LENGTH];
	FRESULT res = get_fspath(pp, path);
	if (res != FR_OK)
		return res;

	struct stat st;
	if (stat(pp, &st) < 0)
		return errno_to_fresult();

	fno->fsize = st.st_size;
	fno->fdate = get_fatfsdate(&st.st_mtim);
	fno->ftime = get_fatfstime(&st.st_mtim);
	fno->fattrib = 0;
	if (st.st_mode & S_IFDIR)
		fno->fattrib |= AM_DIR;
	//TODO: Add support for other attributes
#if _USE_LFN != 0
	strncpy(fno->fname, basename(pp), _MAX_LFN);
  get_fataltname(fno->altname, basename(pp));
#else
  get_fataltname(fno->altname, basename(pp));
#endif

	return FR_OK;
}

FRESULT f_chmod (const TCHAR* path, BYTE attr, BYTE mask){

	// TODO: convert attributes possible?
	return FR_OK;
}

FRESULT f_utime (const TCHAR* path, const FILINFO* fno){

	// TODO: change time support
	return FR_OK;
}

#if _FS_RPATH >= 1
FRESULT f_chdir (const TCHAR* path){

	char pp[_MAX_PATH_LENGTH];
	FRESULT res = get_fspath(pp, path);
	if (res != FR_OK)
		return res;

	if (chdir(pp) < 0)
		return errno_to_fresult();

	return FR_OK;
}

#if _VOLUMES >= 2
FRESULT f_chdrive (const TCHAR* path){

	const TCHAR *rp = path;

	int vol = get_ldnumber(&rp);
	if (vol < 0)
		return FR_INVALID_DRIVE;

	CurrVol = vol;

	return FR_OK;
}
#endif

#if _FS_RPATH >= 2
FRESULT f_getcwd (TCHAR* buff, UINT len){

	if (getcwd(buff, len) < 0)
		errno_to_fresult();

	return FR_OK;
}
#endif /* _FS_RPATH >= 2 */
#endif /* _FS_RPATH >= 1 */

FRESULT f_getfree (const TCHAR* path, DWORD* nclst, FATFS** fatfs){

	const TCHAR *rp = path;

	int vol = get_ldnumber(&rp);
	if (vol < 0)
		return FR_INVALID_DRIVE;

	if (fatfs)
		*fatfs = FatFs[vol];

	if (nclst){
		struct statvfs svfs;

		if (statvfs(FatFs[vol]->mount_point, &svfs) < 0)
			return errno_to_fresult();

		*nclst = svfs.f_bavail * (svfs.f_bsize / 512);
	}

	return FR_OK;
}

static FRESULT get_line(const TCHAR* vol_path, const char *file, char *dest, size_t len){

	const TCHAR *rp = vol_path;

	int vol = get_ldnumber(&rp);
	if (vol < 0)
		return FR_INVALID_DRIVE;

	FATFS *fs = FatFs[vol];

	char pp[_MAX_PATH_LENGTH];
	strcpy(pp, fs->mount_point);
	strcat(pp, "/label.txt");

	strcpy(dest, "");
	FILE *lf = fopen(pp, "r");
	if (lf != NULL){
		size_t len = 0;
		char *line = NULL;
		ssize_t nr = getline(&line, &len, lf);
		if (nr > 0){
			if (line[nr - 1] == '\n')
				line[nr - 1] = 0;
			strncpy(dest, line, len);
		}
		if (line)
			free(line);
		fclose(lf);
	}

	return FR_OK;

}

FRESULT f_getlabel (const TCHAR* path, TCHAR* label, DWORD* vsn){

	FRESULT result = FR_OK;

	if (label){
		result = get_line(path, "label.txt", label, 12);
		if (result != FR_OK)
			return result;
	}
	if (vsn){
		char sn[32];
		result = get_line(path, "vsn.txt", sn, sizeof(sn));
		if (result != FR_OK)
			return result;
		*vsn = atol(sn);
	}
	return FR_OK;
}

FRESULT f_setlabel (const TCHAR* label){

	int vol = get_ldnumber(&label);
	if (vol < 0)
		return FR_INVALID_DRIVE;

	FATFS *fs = FatFs[vol];

	char pp[_MAX_PATH_LENGTH];
	strcpy(pp, fs->mount_point);
	strcat(pp, "/label.txt");

	FILE *lf = fopen(pp, "w+");
	if (lf != NULL){
		fwrite(label, strlen(label), 1, lf);
		fclose(lf);
	}
	return FR_OK;
}

#if _USE_FORWARD
FRESULT f_forward (FIL* fp, UINT(*func)(const BYTE*,UINT), UINT btf, UINT* bf){
	//TODO: forward implementation
}
#endif

#if _USE_EXPAND
FRESULT f_expand (FIL* fp, FSIZE_t szf, BYTE opt){
	//TODO: expand implementation needed?
}
#endif

#if _USE_MKFS
FRESULT f_mkfs (const TCHAR* path, BYTE sfd, UINT au){
	return FR_OK; //Nothing to du here!
}
#endif

#if _MULTI_PARTITION
FRESULT f_fdisk (BYTE pdrv, const DWORD szt[], void* work){
	return FR_OK; //Nothing to du here!
}
#endif

int f_putc (TCHAR c, FIL* fp){

	if (fp->fp == NULL)
		return FR_INVALID_OBJECT;

	return fputc(c, fp->fp);
}

int f_puts (const TCHAR* str, FIL* fp){

	if (fp->fp == NULL)
		return FR_INVALID_OBJECT;

	return fputs(str, fp->fp);
}

int f_printf (FIL* fp, const TCHAR* str, ...){

	if (fp->fp == NULL)
		return FR_INVALID_OBJECT;

	va_list ap;

	va_start(ap, str);
	int result = vfprintf(fp->fp, str, ap);
	va_end(ap);

	return result;
}

TCHAR* f_gets (TCHAR* buff, int len, FIL* fp){

	if (fp->fp == NULL)
		return NULL;

	return fgets(buff, len, fp->fp);
}

int f_eof(const FIL* fp){
	return feof(fp->fp);
}

int f_error(const FIL* fp){
	return ferror(fp->fp);
}

FSIZE_t f_tell(const FIL* fp){
	return (FSIZE_t)ftell(fp->fp);
}

FSIZE_t f_size(const FIL* fp){
	return (FSIZE_t)ftell(fp->fp);
}

DWORD get_fattime(void)
{
  DWORD result = 0;
  struct timeval tv;
  struct timespec ts;

  if (gettimeofday(&tv, NULL) == 0){
    ts.tv_nsec = tv.tv_usec * 1000;
    ts.tv_sec = tv.tv_sec;
    result = get_fatfsdate(&ts);
    result <<= 16;
    result |= get_fatfstime(&ts);
  }

  return result;
}

