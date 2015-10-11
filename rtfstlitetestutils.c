/*
* EBS - RTFS (Real Time File Manager)
*
* Copyright EBS Inc. 1987-2012
* All rights reserved.
* This code may not be redistributed in source or linkable object form
* without the consent of its author.
*/

#include "rtfslite.h"
#include "stdlib.h"


struct ut_fill_structure  {
	int last_index_pointer;
	int first_free_pointer;
	int eof_pointer;
};

static int ut_fill_callback(struct rtfsl_file const *pcurrent_entry_file, void *puser_data)
{
struct ut_fill_structure *fill_structure= (struct ut_fill_structure *) puser_data;
	{
		if ((pcurrent_entry_file->rtfsl_direntry_type&RTFSL_ENTRY_AVAILABLE)!=0)
		{
			if (fill_structure->first_free_pointer==-1)
				fill_structure->first_free_pointer=fill_structure->last_index_pointer;
			if (fill_structure->eof_pointer==-1&&(pcurrent_entry_file->rtfsl_direntry_type&RTFSL_ENTRY_TYPE_EOF))
				fill_structure->eof_pointer=fill_structure->last_index_pointer;
		}
	}
	fill_structure->last_index_pointer++;
    return 0; /* Continue */
}

int ut_fill_directory(int leave_n_free)
{
    char name[12];
	int n_left;
    ANSImemcpy(name,"TESTNAMEEXT",11);
    char c='A';
    int index = 0;
    int fcount;


	fcount = (rtfsl.current_dr.bytespcluster/32);
	{
	int fd;
	int rval;
	struct ut_fill_structure fill_structure;
	struct rtfsl_file new_directory_file;

		fill_structure.first_free_pointer =-1;
		fill_structure.eof_pointer=-1;
		fill_structure.last_index_pointer=0;
		fd = rtfsl_alloc_fd();
		if (fd<0)
			return fd;
		rval = rtfsl_open_path(rtfsl.current_dr.pathnamearray,0, &new_directory_file, &rtfsl.rtfsl_files[fd]);
		if (rval==0)
		{
			rval = rtfsl_enumerate_directory(&rtfsl.rtfsl_files[fd],&new_directory_file, ut_fill_callback,(void *) &fill_structure);
			if(rval<0)
				return rval;
			fcount = fill_structure.eof_pointer;
			if (fill_structure.eof_pointer!=fill_structure.first_free_pointer)
				return RTFSL_ERROR_TEST; //  Warning fill directory will be inaccurate
		}
		rtfsl.rtfsl_files[fd].rtfsl_file_flags=0;
	}
	n_left = (rtfsl.current_dr.bytespcluster/32)-fcount;
    while (n_left > leave_n_free)
    {
	int rval;
        name[index]=c;
        c+=1;
        if (c >'Z')
        {
            c='A';
            index+=1;
        }
        rval=rtfsl_create((unsigned char*)name,0);
        if (rval<0)
            return rval;
		n_left--;
    }
    return 0;
}

/* Unconditional delete all files and subdirectories in the folder.
   Does not descend. If subdirectories contain information lost clusters will result */
void ut_unfill_directory(unsigned char *name)
{
	struct rtfsl_dstat dstat;
	while (rtfsl_gfirst(&dstat, name)==1)
	{
		if ( (rtfsl_gnext(&dstat)==1) && /* ".." */
			 (rtfsl_gnext(&dstat)==1)
		   )
				_rtfsl_delete(dstat.fnameandext, dstat.fattribute&ADIRENT);
		else
			break;
	}
}

int ut_release_fragments(int file_fraglen, int gap_fraglen, int numfrags, unsigned long *fragrecord/*[numfrags]*/)
{
int fragnum,rval;
unsigned long cluster,value;
    value=0;
    for(fragnum=0; fragnum < numfrags; fragnum++)
    {
        for (cluster=*(fragrecord+fragnum); cluster < *(fragrecord+fragnum)+gap_fraglen;cluster++)
        {
            /* Use write call to free up gap_fraglen clusters */
            rval=fatop_buff_get_frag(cluster|RTFSL_WRITE_CLUSTER, &value, 1);
            if (rval < 0)
                return rval;
        }
    }
    return 0;
}

int ut_create_fragments(int file_fraglen, int gap_fraglen, int numfrags, unsigned long *fragrecord/*[numfrags]*/)
{
unsigned long new_start_hint,start_hint,next_cluster,current_cluster,cluster,value;
int rval,fragnum,len;
    if (gap_fraglen==0)
        return 0;
    /* */
    start_hint=2;
	new_start_hint=0;
    for(fragnum=0; fragnum < numfrags; fragnum++)
    {
        /* Use allocate call to get file_fraglen contiguous clusters, but do not write to the clusters */
        len=0;
        current_cluster=0;
        while (len<file_fraglen)
        {
           rval=fatop_buff_get_frag(start_hint|RTFSL_ALLOC_CLUSTER, &next_cluster, 1);
           if (rval<=0)
            return rval;
           start_hint=next_cluster+1;
           if (current_cluster==0||((current_cluster+1)!=next_cluster))
           {
                current_cluster=next_cluster;
                len=1;
           }
           else
                len += 1;
        }
		if (new_start_hint==0)
			new_start_hint=current_cluster;
        len=0;
        current_cluster=0;
        while (len<gap_fraglen)
        {
           rval=fatop_buff_get_frag(start_hint|RTFSL_ALLOC_CLUSTER, &next_cluster, file_fraglen);
           if (rval<=0)
            return rval;
           if (current_cluster==0||((current_cluster+1)!=next_cluster))
           {
                current_cluster=next_cluster;
                len=1;
           }
           else
                len += 1;
           start_hint=next_cluster+1;
        }
        *(fragrecord+fragnum)=current_cluster;
        /* Use write call to allocate gap_fraglen clusters */
		value = rtfsl.current_dr.end_cluster_marker|0xf;
        for (cluster=*(fragrecord+fragnum); cluster < *(fragrecord+fragnum)+gap_fraglen;cluster++)
        {
            rval=fatop_buff_get_frag(cluster|RTFSL_WRITE_CLUSTER, &value, 1);
            if (rval < 0)
                return rval;
        }
    }
	/* Set next_alloc to the start of our fragment chain so when we allocate we'll start where we wanted to */
	rtfsl.current_dr.next_alloc=new_start_hint;
    return 0;
}
