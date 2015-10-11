/*
* EBS - RTFS (Real Time File Manager)
*
* Copyright EBS Inc. 1987-2012
* All rights reserved.
* This code may not be redistributed in source or linkable object form
* without the consent of its author.
*/

//HEREHERE
//#define RTFSLFS_JTEST	1
//#define RTFSLFS_JCLEAR	2
//#define RTFSLFS_JREAD	3
//#define RTFSLFS_JWRITE	4

#include "rtfslite.h"
#if (RTFSL_INCLUDE_FAILSAFE_SUPPORT)
int rtfslfs_access_journal(int command)
{
	struct rtfsl_failsafe_context *pfs=rtfsl.rtfsl_current_failsafe_context;
	int i,rval;
	unsigned char *b=pfs->journal_buffer;
    unsigned long journal_file_start_sector;
	rval=0;
	/* Clear zeor fills the buffer and writes it */
	if (command==RTFSLFS_JCLEAR)
		ANSImemset(b,0,pfs->journal_buffer_size);
	if (RTFSL_CFG_FSBUFFERSIZESECTORS+1 > rtfsl.current_dr.partition_base)
		return RTFSL_ERROR_JOURNAL_NONE;
	/* JTEST Will fall through without doing any I/O */
	journal_file_start_sector=rtfsl.current_dr.partition_base-RTFSL_CFG_FSBUFFERSIZESECTORS;
	for (i=0;rval==0&&i<pfs->journal_buffer_size/rtfsl.current_dr.bytespsector; i++)
    {
        if (command==RTFSLFS_JREAD)
            rval=rtfsl_read_sectors(journal_file_start_sector+i,1,b);
        else if (command==RTFSLFS_JWRITE||command==RTFSLFS_JCLEAR)
            rval=rtfsl_write_sectors(journal_file_start_sector+i,1,b);
        b+=rtfsl.current_dr.bytespsector;
    }
	return rval;
}
#endif