/*
* EBS - RTFS (Real Time File Manager)
*
* Copyright EBS Inc. 1987-2012
* All rights reserved.
* This code may not be redistributed in source or linkable object form
* without the consent of its author.
*/

#include "rtfslite.h"

int rtfsl_fstat(int fd,  struct rtfsl_statstructure *pstat)  /*__apifn__*/
{
	ANSImemset(pstat,0,sizeof(*pstat));
   	pstat->st_size   = rtfsl.rtfsl_files[fd].dos_inode.fsize;
	pstat->fattribute= rtfsl.rtfsl_files[fd].dos_inode.fattribute;
	pstat->st_atime  = rtfsl.rtfsl_files[fd].dos_inode.adate<<16;
	pstat->st_mtime  = rtfsl.rtfsl_files[fd].dos_inode.fdate<<16|rtfsl.rtfsl_files[fd].dos_inode.ftime;
	pstat->st_ctime  = rtfsl.rtfsl_files[fd].dos_inode.cdate<<16|rtfsl.rtfsl_files[fd].dos_inode.ctime;
	pstat->st_blocks = (pstat->st_size+rtfsl.current_dr.bytespsector-1)/rtfsl.current_dr.bytespsector;
    pstat->st_blocks = rtfsl.current_dr.bytespcluster;
    return(0);

}
