/*
* EBS - RTFS (Real Time File Manager)
*
* Copyright EBS Inc. 1987-2012
* All rights reserved.
* This code may not be redistributed in source or linkable object form
* without the consent of its author.
*/

#include "rtfslite.h"


int rtfsl_mkdir(unsigned char *name)  /*__apifn__*/
{
struct rtfsl_dosinode dotentries[3];
int fd,rval;

	rval=rtfsl_create(name,0);
    if (rval==0)
	{
		rval= rtfsl_open(name);
		if (rval >= 0)
		{
			fd=rval;
			rval = rtfsl_write(fd,0,rtfsl.current_dr.bytespcluster);
			if (rval<0)
				return rval;
			rtfsl_lseek(fd, 0, PSEEK_SET);
			rval=rtfsl_clzero(rtfsl.rtfsl_files[fd].cluster_segment_array[0][0]);
			if (rval<0)
				return rval;
		   ANSImemcpy(&dotentries[0],&rtfsl.rtfsl_files[fd].dos_inode,sizeof(rtfsl.rtfsl_files[fd].dos_inode));
		   ANSImemcpy(&dotentries[1],&rtfsl.rtfsl_files[fd].dos_inode,sizeof(rtfsl.rtfsl_files[fd].dos_inode));
		   dotentries[0].fattribute=ADIRENT;
		   dotentries[1].fattribute=ADIRENT;
		   ANSImemcpy(&dotentries[0].fname,dotname,11);
		   ANSImemcpy(&dotentries[1].fname,dotdotname,11);
		   memset(&dotentries[2].fname,0,11);
		   dotentries[1].fsize  = 0;
		   dotentries[0].fsize  = 0;
#if (RTFSL_INCLUDE_FAILSAFE_SUPPORT)
            if (rtfsl.rtfsl_current_failsafe_context)
                rtfsl.rtfsl_current_failsafe_context->flags |= RTFSLFS_WRITE_DIRENTRY;
#endif


			/* Instructs the write operation to poulate "." with "self" and ".." with parent */
			rtfsl.rtfsl_files[fd].rtfsl_file_flags|=TRTFSFILE_ISMKDIR;
		    rval = rtfsl_write(fd, (unsigned char*)dotentries,96);
			rtfsl.rtfsl_files[fd].rtfsl_file_flags&=~TRTFSFILE_ISMKDIR;
#if (RTFSL_INCLUDE_FAILSAFE_SUPPORT)
            if (rtfsl.rtfsl_current_failsafe_context)
	            rtfsl.rtfsl_current_failsafe_context->flags &= ~RTFSLFS_WRITE_DIRENTRY;
#endif
		    if (rval > 0)
            {
               rtfsl.rtfsl_files[fd].dos_inode.fattribute=ADIRENT;
               rtfsl.rtfsl_files[fd].dos_inode.fsize=0;
               rval=rtfsl_flush(fd);
            }
		    rtfsl.rtfsl_files[fd].rtfsl_file_flags=0;	/* Deallocates the file */
		}
    }
	return rval;
}
