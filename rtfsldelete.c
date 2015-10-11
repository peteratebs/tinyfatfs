/*
* EBS - RTFS (Real Time File Manager)
*
* Copyright EBS Inc. 1987-2012
* All rights reserved.
* This code may not be redistributed in source or linkable object form
* without the consent of its author.
*/

#include "rtfslite.h"

int rtfsl_delete(unsigned char *name) /*__apifn__*/
{
	return _rtfsl_delete(name,0);
}

int _rtfsl_delete(unsigned char *name,unsigned char attribute)
{
int fd,rval,offset;
    rval= _rtfsl_open(name,attribute);
	if (rval >= 0)
	{
		fd=rval;
		rval=rtfsl_read(fd, 0, rtfsl.rtfsl_files[fd].segment_size_bytes);
 		while (rval>0)
		{
		unsigned long cluster,value;
			value=0;
			for (offset=0;rtfsl.rtfsl_files[fd].cluster_segment_array[offset][0]&&offset<RTFSL_CFG_FILE_FRAG_BUFFER_SIZE;offset++)
			{
				for (cluster = rtfsl.rtfsl_files[fd].cluster_segment_array[offset][0]; cluster <= rtfsl.rtfsl_files[fd].cluster_segment_array[offset][1];cluster++)
				{
                    rval= fatop_buff_get_frag(cluster|RTFSL_WRITE_CLUSTER, &value, 1);
					if (rval<0)
                    {
					    break;
					}
				}
			}
			if (rval>=0)
			    rval=rtfsl_read(fd, 0, rtfsl.rtfsl_files[fd].segment_size_bytes);
		}
		if (rval==0)
		{
			rtfsl.rtfsl_files[fd].dos_inode.fname[0]=PCDELETE;
			rval=rtfsl_flush(fd);
		}
		rtfsl.rtfsl_files[fd].rtfsl_file_flags=0;	/* Deallocates the file */
	}
	return rval;
}
