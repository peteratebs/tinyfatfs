/*
* EBS - RTFS (Real Time File Manager)
*
* Copyright EBS Inc. 1987-2012
* All rights reserved.
* This code may not be redistributed in source or linkable object form
* without the consent of its author.
*/

#include "rtfslite.h"



int rtfsl_rename(unsigned char *name, unsigned char *newname)
{
struct rtfsl_file scratch_dir_file;
int fd,rval;
	fd = rtfsl_alloc_fd();
	if (fd>=0)
	{
        rval=rtfsl_open_path(rtfsl.current_dr.pathnamearray,newname,&scratch_dir_file, &rtfsl.rtfsl_files[fd]);
        if (rval==0)
			rval=RTFSL_ERROR_EXIST;
		else
	    {
			rval=rtfsl_open_path(rtfsl.current_dr.pathnamearray,name,&scratch_dir_file, &rtfsl.rtfsl_files[fd]);
			if (rval==0)
			{
				ANSImemcpy(rtfsl.rtfsl_files[fd].dos_inode.fname,newname,11);
				rval=rtfsl_flush(fd);
			}
		}
		rtfsl.rtfsl_files[fd].rtfsl_file_flags=0;	/* Deallocates the file */
	}
	else
		rval=fd;
	return rval;
}
