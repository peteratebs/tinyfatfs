/*
* EBS - RTFS (Real Time File Manager)
*
* Copyright EBS Inc. 1987-2012
* All rights reserved.
* This code may not be redistributed in source or linkable object form
* without the consent of its author.
*/

#include "rtfslite.h"

static unsigned long file_seek_callback (struct rtfsl_file const *pfile, unsigned long start_sector, unsigned long nbytes, void *puser_data)
{
	unsigned long target = (unsigned long) puser_data;
	if (pfile->file_pointer+nbytes > target)
		return target-pfile->file_pointer;
	else
		return nbytes;
}

long rtfsl_lseek(int fd, long offset, int origin)       /*__apifn__*/
{
unsigned long target_fp;
int rval;
    if (origin == PSEEK_SET)  /* offset from beginning of file */
    {
        target_fp = (unsigned long)offset;
    }
    else if (origin == PSEEK_CUR)   /* offset from current file pointer */
    {
       target_fp=rtfsl.rtfsl_files[fd].file_pointer+offset;
    }
    else if (origin == PSEEK_END)   /*  offset from end of file */
    {
		if (offset>0)
			return RTFSL_ERROR_ARGS;
        target_fp=rtfsl.rtfsl_files[fd].dos_inode.fsize+offset;
    }
	else
		return RTFSL_ERROR_ARGS;
    rval=rtfsl_enumerate_file(&rtfsl.rtfsl_files[fd],file_seek_callback, (void *) target_fp);
    if (rval<0)
        return (long)rval;
	if (rtfsl.rtfsl_files[fd].file_pointer!=target_fp)
		return RTFSL_ERROR_CONSISTENCY;
	else
		return (long) rtfsl.rtfsl_files[fd].file_pointer;
}
