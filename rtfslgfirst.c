/*
* EBS - RTFS (Real Time File Manager)
*
* Copyright EBS Inc. 1987-2012
* All rights reserved.
* This code may not be redistributed in source or linkable object form
* without the consent of its author.
*/

#include "rtfslite.h"

/* Structure for use by rtfsl_gfirst, rtfsl_gnext */

#define GFIRST_EOF 2
static int rtfsl_gfirst_callback(struct rtfsl_file const *pcurrent_entry_file, void *puser_data)
{
	struct rtfsl_dstat *pdstat = (struct rtfsl_dstat *) puser_data;

	if (pcurrent_entry_file->rtfsl_direntry_type==RTFSL_ENTRY_TYPE_EOF)
		return GFIRST_EOF;
	if ((pcurrent_entry_file->rtfsl_direntry_type&(RTFSL_ENTRY_TYPE_DIRECTORY|RTFSL_ENTRY_TYPE_VOLUME|RTFSL_ENTRY_TYPE_FILE))!=0)
	{
		if (!pdstat->nametomatch||ANSImemcmp(pcurrent_entry_file->dos_inode.fname,pdstat->nametomatch,11)==0)
		{
			ANSImemcpy(&pdstat->fnameandext,pcurrent_entry_file->dos_inode.fname,11);
			pdstat->fnameandext[11]=0;   /* Null terminate file and extension */
			pdstat->fattribute=pcurrent_entry_file->dos_inode.fattribute;;
			pdstat->ftime=pcurrent_entry_file->dos_inode.ftime;
			pdstat->fdate=pcurrent_entry_file->dos_inode.fdate;
			pdstat->ctime=pcurrent_entry_file->dos_inode.ctime;
			pdstat->cdate=pcurrent_entry_file->dos_inode.cdate;
			pdstat->atime=0;
			pdstat->adate=pcurrent_entry_file->dos_inode.adate;
			pdstat->fsize=pcurrent_entry_file->dos_inode.fsize;
			return 1;
		}
	}
    return 0; /* Continue */
}

/* Return <0 on error, 1 if contents are valid and scan may continue, 0, if end of directory was reached. */
int rtfsl_gfirst(struct rtfsl_dstat *statobj, unsigned char *name)       /*__apifn__*/
{
int rval;
	ANSImemset(statobj,0,sizeof(*statobj));
	statobj->nametomatch=name;
	/* Open the current directory or root use current_matched_file as a scratch directory entry*/
	rval=rtfsl_open_path(rtfsl.current_dr.pathnamearray,0, &statobj->current_matched_file, &statobj->directory_file);
	if (rval >= 0)
	{
		/* Enumerate statobj->directory_file in search of name */
		rval = rtfsl_finode_open(&statobj->directory_file);
		if (rval==0)
		{
			rval=rtfsl_enumerate_directory(&statobj->directory_file,&statobj->current_matched_file,rtfsl_gfirst_callback,(void *) statobj);
			if (rval==GFIRST_EOF)
				rval=0;
		}
	}
	return rval;
}

/* Return <0 on error, 1 if contents are valid and scan may continue, 0, if end of directory was reached. */
int rtfsl_gnext(struct rtfsl_dstat *statobj)                        /*__apifn__*/
{
	statobj->nametomatch=0;
    return rtfsl_enumerate_directory(&statobj->directory_file,&statobj->current_matched_file,rtfsl_gfirst_callback,(void *) statobj);
}

void rtfsl_done(struct rtfsl_dstat *statobj)                        /*__apifn__*/
{
}
