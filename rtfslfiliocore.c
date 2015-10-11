/*
* EBS - RTFS (Real Time File Manager)
*
* Copyright EBS Inc. 1987-2012
* All rights reserved.
* This code may not be redistributed in source or linkable object form
* without the consent of its author.
*/

#include "rtfslite.h"

static long rtfsl_map_region_clusterwindow(int fd,unsigned long file_pointer, unsigned long *sector, unsigned long *offset)
{
    int region;
    unsigned long startcluster,file_pointer_past_segment_end,file_pointer_at_region_base,file_pointer_past_region_end;
    /* */
    *offset = file_pointer%rtfsl.current_dr.bytespsector;
	if (rtfsl.rtfsl_files[fd].rtfsl_file_flags & TRTFSFILE_SECTOR_REGION )
	{
        *sector = rtfsl.rtfsl_files[fd].cluster_segment_array[0][0]+file_pointer/rtfsl.current_dr.bytespsector;
		return (rtfsl.rtfsl_files[fd].cluster_segment_array[0][1]-rtfsl.rtfsl_files[fd].cluster_segment_array[0][0])*rtfsl.current_dr.bytespsector-file_pointer;
	}
	startcluster=0;
	file_pointer_past_segment_end=rtfsl.rtfsl_files[fd].file_pointer_at_segment_base+rtfsl.rtfsl_files[fd].segment_size_bytes;
	if (file_pointer >= file_pointer_past_segment_end)
	{
		startcluster = rtfsl.rtfsl_files[fd].next_segment_base;
	}
    else if (file_pointer >= rtfsl.rtfsl_files[fd].file_pointer_at_segment_base) /* Scan up from current */
		startcluster = rtfsl.rtfsl_files[fd].cluster_segment_array[0][0];

	if (!startcluster)
	{
		startcluster = to_USHORT((unsigned char *)& rtfsl.rtfsl_files[fd].dos_inode.fclusterhi);
		startcluster <<= 16;
		startcluster |= to_USHORT((unsigned char *)& rtfsl.rtfsl_files[fd].dos_inode.fcluster);
		rtfsl.rtfsl_files[fd].segment_size_bytes=0;
		rtfsl.rtfsl_files[fd].file_pointer_at_segment_base=0;
	}
	while (file_pointer >= file_pointer_past_segment_end)
    {
    long rval;
		rval=rtfsl_load_next_segment(&rtfsl.rtfsl_files[fd],startcluster);
		if (rval<=0)
			return rval;
		file_pointer_past_segment_end=rtfsl.rtfsl_files[fd].file_pointer_at_segment_base+rtfsl.rtfsl_files[fd].segment_size_bytes;
        startcluster=0;
    }
    file_pointer_at_region_base=file_pointer_past_region_end=rtfsl.rtfsl_files[fd].file_pointer_at_segment_base;
    for(region=0; region < RTFSL_CFG_FILE_FRAG_BUFFER_SIZE && rtfsl.rtfsl_files[fd].cluster_segment_array[region][0];region++)
    {
        unsigned long region_length;
		region_length = (rtfsl.rtfsl_files[fd].cluster_segment_array[region][1]-rtfsl.rtfsl_files[fd].cluster_segment_array[region][0]+1)*rtfsl.current_dr.bytespcluster;

		file_pointer_past_region_end += region_length;
		if (file_pointer_past_region_end>file_pointer)
        {
            *sector = rtfsl_cl2sector(rtfsl.rtfsl_files[fd].cluster_segment_array[region][0])+(file_pointer-file_pointer_at_region_base)/rtfsl.current_dr.bytespsector;
            return file_pointer_past_region_end-file_pointer;
        }
		file_pointer_at_region_base += region_length;

    }
    return RTFSL_ERROR_CONSISTENCY;
 }

#if (RTFSL_INCLUDE_FAILSAFE_SUPPORT)
static int _rtfsl_journal_io(unsigned long sector, int index, unsigned char *pdata, long n_bytes)
{
    int rval;
	long copied;
    for (copied=0; copied<n_bytes;copied+=32,index++,pdata+=32)
    {
        rval=rtfslfs_dirent_remap(sector,index, (struct rtfsl_dosinode *) pdata,0);
        if (rval<0)
            return rval;
    }
    return 0;
}
#endif

long _rtfsl_bfilio_io(int fd, unsigned char *pdata, long n_bytes, unsigned char opflags)
{
long n_left,copied,count_inregion;
unsigned long file_pointer; // ,region_byte_next,file_region_byte_base, file_region_block_base;
unsigned long sector,residual,offset;
int rval=0;
    /* use local copies of file info so if we fail the pointers will not advance */
	file_pointer  = rtfsl.rtfsl_files[fd].file_pointer;
	n_left = n_bytes;
	sector=count_inregion=residual=offset=0;


	while (n_left)
 	{
		if (!residual)
		{
			count_inregion = rtfsl_map_region_clusterwindow(fd, file_pointer, &sector, &offset);
			if (count_inregion<0)
				return count_inregion;
			if (count_inregion==0)
				break;
			if (count_inregion > n_left)
				count_inregion = n_left;
		}
		if (residual||offset)
		{
		unsigned char *p;
        int startcopyfr;
            if (offset)
            {
			    /* If reading and writing are enabled use opflags to ccheck for write */
			    copied=rtfsl.current_dr.bytespsector-offset;
                startcopyfr=(int)offset;
                offset=0;
            }
            else
            {
			    copied=residual;
                startcopyfr=0;
            }
			if (copied > n_left)
				copied = n_left;
			if (pdata)
			{
				int buffernumber=rtfsl_read_sector_buffer(rtfsl.current_dr.partition_base+sector);
				if (buffernumber<0)
					return buffernumber;
                p = rtfsl_buffer_address(buffernumber);
#if (RTFSL_INCLUDE_WRITE_SUPPORT)
				if (opflags & RTFSL_FIO_OP_WRITE)
				{
					if (rtfsl.rtfsl_files[fd].rtfsl_file_flags&TRTFSFILE_ISMKDIR)
					{
					struct rtfsl_dosinode *pdos_inode=(struct rtfsl_dosinode *)pdata;
					unsigned short w;
					unsigned long  cl=rtfsl_sec2cluster(rtfsl.rtfsl_files[fd].sector);
						/* "." points to self */
						pdos_inode->fclusterhi=rtfsl.rtfsl_files[fd].dos_inode.fclusterhi;
						pdos_inode->fcluster=rtfsl.rtfsl_files[fd].dos_inode.fcluster;
						pdos_inode++;
						/* ".." points to parent */
						w=(unsigned short) (cl>>16);
						fr_USHORT((unsigned char *)&pdos_inode->fclusterhi, w);
						w=(unsigned short) cl&0xffff;
						fr_USHORT((unsigned char*)&pdos_inode->fcluster, w);
					}
#if (RTFSL_INCLUDE_FAILSAFE_SUPPORT)
                    /* A small write (32 or 64 bytes) needs to be journaled will always be offset or residual so all cases are covered */
                    if (rtfsl.rtfsl_current_failsafe_context && (rtfsl.rtfsl_current_failsafe_context->flags&RTFSLFS_WRITE_DIRENTRY))
                        rval=_rtfsl_journal_io(rtfsl.current_dr.partition_base+sector, startcopyfr/32, pdata, copied);
                    else
#endif
                    {
					    ANSImemcpy(p+startcopyfr, pdata,copied);
					    rval=rtfsl_flush_sector_buffer(buffernumber,1);
                    }
					if (rval<0)
						return rval;
				}
				else
#endif
                {
				    ANSImemcpy(pdata,p+startcopyfr,copied);
                }
				pdata+=copied;
			}
			n_left-=copied;

			file_pointer+=copied;
			count_inregion-=copied;
			sector+=1;
		}
		while (count_inregion >= rtfsl.current_dr.bytespsector)
		{
			copied=rtfsl.current_dr.bytespsector;
			if(pdata)
			{
#if (RTFSL_INCLUDE_WRITE_SUPPORT)
				if (opflags & RTFSL_FIO_OP_WRITE)
				{
					rval = rtfsl_write_sectors(rtfsl.current_dr.partition_base+sector,1, pdata);
				}
				else
#endif
				{
					rval=rtfsl_read_sectors(rtfsl.current_dr.partition_base+sector,1, pdata);
				}
				if (rval < 0)
					return rval;
				pdata+=copied;
			}
			n_left-=copied;
			file_pointer+=copied;
			sector+=1;
			count_inregion-=copied;
		}
		residual=(long)count_inregion;
	}
	rtfsl.rtfsl_files[fd].file_pointer=file_pointer;
	return(n_bytes-n_left);
}
