/*
* EBS - RTFS (Real Time File Manager)
*
* Copyright EBS Inc. 1987-2012
* All rights reserved.
* This code may not be redistributed in source or linkable object form
* without the consent of its author.
*/

#include "rtfslite.h"


extern long _rtfsl_bfilio_io(int fd, unsigned char *pdata, long n_bytes, unsigned char opflags);

void rtfsl_setpath(unsigned char **pathnamearray)      /*__apifn__*/
{
    rtfsl.current_dr.pathnamearray=pathnamearray;
}

int rtfsl_alloc_fd(void)
{
int fd;
	for (fd=1;fd<RTFSL_CFG_NFILES;fd++)
	{
		if (rtfsl.rtfsl_files[fd].rtfsl_file_flags==0)
		{
			rtfsl.rtfsl_files[fd].rtfsl_file_flags=TRTFSFILE_ALLOCATED;
			return fd;
		}
	}
	return RTFSL_ERROR_FDALLOC;
}
int rtfsl_open(unsigned char *name) /*__apifn__*/
{
	return _rtfsl_open(name, 0);
}
int _rtfsl_open(unsigned char *name, unsigned char attributes)
{
struct rtfsl_file scratch_dir_file;
int fd,rval;
	fd = rtfsl_alloc_fd();
	if (fd>=0)
	{
	unsigned char want_type=RTFSL_ENTRY_TYPE_FILE;
		if (attributes&ADIRENT)
			want_type=RTFSL_ENTRY_TYPE_DIRECTORY;
        rval=rtfsl_open_path(rtfsl.current_dr.pathnamearray,name,&scratch_dir_file, &rtfsl.rtfsl_files[fd]);
        if (rval==0 && rtfsl.rtfsl_files[fd].rtfsl_direntry_type!=want_type)
			rval=RTFSL_ERROR_PATH;
	    if (rval <0)
		{
			rtfsl.rtfsl_files[fd].rtfsl_file_flags=0;	/* Deallocates the file */
			fd=rval;
		}
	}
	return fd;
}

int rtfsl_read(int fd, unsigned char *in_buff, int count)    /*__apifn__*/
{
long n_bytes;
	if (rtfsl.rtfsl_files[fd].file_pointer+count > rtfsl.rtfsl_files[fd].dos_inode.fsize) /* fsize is stores in native byte order */
		n_bytes=rtfsl.rtfsl_files[fd].dos_inode.fsize-rtfsl.rtfsl_files[fd].file_pointer;
	else
		n_bytes=(long)count;
	return _rtfsl_bfilio_io(fd, in_buff, n_bytes, 0);
}


int rtfsl_close(int fd)  /*__apifn__*/
{
#if (RTFSL_INCLUDE_WRITE_SUPPORT)
int rval=0;
    if (rtfsl.rtfsl_files[fd].rtfsl_file_flags&TRTFSFILE_DIRTY)
        rval=rtfsl_flush(fd);
    rtfsl.rtfsl_files[fd].rtfsl_file_flags=0;	/* Deallocates the file */
    return(rval);
#else
    rtfsl.rtfsl_files[fd].rtfsl_file_flags=0;	/* Deallocates the file */
    return(0);
#endif
}
