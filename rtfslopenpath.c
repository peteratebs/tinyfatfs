/*
* EBS - RTFS (Real Time File Manager)
*
* Copyright EBS Inc. 1987-2012
* All rights reserved.
* This code may not be redistributed in source or linkable object form
* without the consent of its author.
*/

#include "rtfslite.h"
struct rtfsl_open_path_structure
{
	int current_index;
	unsigned char **pathlist;
	unsigned char *pathentry;
};

static int open_path_callback(struct rtfsl_file const *pcurrent_entry_file, void *puser_data)
{
	if (pcurrent_entry_file->rtfsl_direntry_type==RTFSL_ENTRY_TYPE_DIRECTORY || pcurrent_entry_file->rtfsl_direntry_type==RTFSL_ENTRY_TYPE_FILE)
    {
	    struct rtfsl_open_path_structure *open_path_structure = (struct rtfsl_open_path_structure *) puser_data;
        unsigned char *p=open_path_structure->pathentry;
        if (!p)
            p=open_path_structure->pathlist[open_path_structure->current_index];
		if (p&&ANSImemcmp(pcurrent_entry_file->dos_inode.fname,p,11)==0)
        {
            open_path_structure->current_index+=1;
            return 1;
        }
    }
    return 0; /* Continue */
}

int rtfsl_open_path(unsigned char **pathlist,unsigned char *pentryname, struct rtfsl_file *directory_file, struct rtfsl_file *ptarget_file)
{
struct rtfsl_open_path_structure open_path_structure;
int rval;
	open_path_structure.current_index=0;
	open_path_structure.pathlist=pathlist;
	open_path_structure.pathentry=0;
    ANSImemset(directory_file,0,sizeof(*directory_file));
    rval=rtfsl_root_finode_open(directory_file);
    if (rval<0)
        return rval;
#if(RTFSL_INCLUDE_SUBDIRECTORIES)
    while (pathlist && pathlist[open_path_structure.current_index])
	{
        ANSImemset(ptarget_file,0,sizeof(*ptarget_file));
		if (directory_file->rtfsl_direntry_type!=RTFSL_ENTRY_TYPE_DIRECTORY)
            return RTFSL_ERROR_PATH;
		rval = rtfsl_enumerate_directory(directory_file,ptarget_file, open_path_callback,(void *) &open_path_structure);
        if (rval < 0)
            return rval;
		if (rval==1)
		{
            ANSImemcpy(directory_file,ptarget_file,sizeof(*ptarget_file));
			rval=rtfsl_finode_open(directory_file);
			if (rval<0)
				return rval;
            if (!pathlist[open_path_structure.current_index])
                break;
		}
        else
            return RTFSL_ERROR_PATH;
	}
#endif
    if (!pentryname)
    {
        ANSImemcpy(ptarget_file,directory_file,sizeof(*ptarget_file));
        rval=0;
    }
    else if (directory_file->rtfsl_direntry_type==RTFSL_ENTRY_TYPE_DIRECTORY)
    {
	    open_path_structure.pathentry=pentryname;
	    rval=RTFSL_ERROR_NOTFOUND;
	    if (rtfsl_enumerate_directory(directory_file,ptarget_file, open_path_callback,(void *) &open_path_structure)==1)
		{
            rval=rtfsl_finode_open(ptarget_file);
		}
    }
    else
	    rval=RTFSL_ERROR_PATH;
    return rval;
}
