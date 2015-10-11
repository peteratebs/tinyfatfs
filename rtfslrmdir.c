/*
* EBS - RTFS (Real Time File Manager)
*
* Copyright EBS Inc. 1987-2012
* All rights reserved.
* This code may not be redistributed in source or linkable object form
* without the consent of its author.
*/

#include "rtfslite.h"



struct rtfsl_rmdir_structure
{
	unsigned long file_pointer;
};
static int rmdir_callback(struct rtfsl_file const *pcurrent_entry_file, void *puser_data)
{
struct rtfsl_rmdir_structure *prmdir_structure=(struct rtfsl_rmdir_structure *) puser_data;

	if (prmdir_structure->file_pointer>=64 && (pcurrent_entry_file->rtfsl_direntry_type&RTFSL_ENTRY_AVAILABLE)==0)
    {
        return RTFSL_ERROR_ENOTEMPTY;
    }
	prmdir_structure->file_pointer+=32;
    return 0; /* Continue */
}

int rtfsl_rmdir(unsigned char *name)    /*__apifn__*/
{
int fd,rval;
struct rtfsl_rmdir_structure rmdir_structure;
struct rtfsl_file new_directory_file;

	fd= rtfsl_dir_open(name);
	if (fd>=0)
	{
		rmdir_structure.file_pointer=0;
		/* returns RTFSL_ERROR_ENOTEMPTY if the directory contains any files or subdirectories */
		rval = rtfsl_enumerate_directory(&rtfsl.rtfsl_files[fd],&new_directory_file, rmdir_callback,(void *) &rmdir_structure);
	    if(rval>=0)
	    {
		    rtfsl.rtfsl_files[fd].rtfsl_file_flags=0;	/* Deallocates the file */
	        rval=_rtfsl_delete(name,ADIRENT);
		}
	}
	else
		rval=fd;
    return rval;
}
