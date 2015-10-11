/*
* EBS - RTFS (Real Time File Manager)
*
* Copyright EBS Inc. 1987-2012
* All rights reserved.
* This code may not be redistributed in source or linkable object form
* without the consent of its author.
*/

#include "rtfslite.h"
extern long _rtfsl_bfilio_io(int fd, unsigned char *pdata, long n_bytes, unsigned char opflags);

struct rtfsl_create_structure
{
	unsigned char *name;
	int eof;
	unsigned long free_file_pointer;
	unsigned long file_pointer;
	unsigned char attribute;
};

int rtfsl_clzero(unsigned long cluster)
{
int i, rval;
unsigned long sector;
    sector=rtfsl_cl2sector(cluster);
	rval=0;
	for (i =0; rval==0 && i < rtfsl.current_dr.secpalloc; i++,sector++)
    {
	unsigned char *b;
		rval=rtfsl_read_sector_buffer(sector);
		if (rval<0)
			return rval;
		b = rtfsl_buffer_address(rval);
		ANSImemset(b,0,rtfsl.current_dr.bytespsector);
        rval = rtfsl_write_sectors(rtfsl.current_dr.partition_base+sector,1, b);
    }
	return rval;
}
#define NOFREEFILESFOUND 0xffffffff

static int create_callback(struct rtfsl_file const *pcurrent_entry_file, void *puser_data)
{
struct rtfsl_create_structure *pcreate_structure=(struct rtfsl_create_structure *) puser_data;

	{
		if ((pcurrent_entry_file->rtfsl_direntry_type&RTFSL_ENTRY_AVAILABLE)!=0)
		{
			if (pcreate_structure->free_file_pointer==NOFREEFILESFOUND)
				pcreate_structure->free_file_pointer=pcreate_structure->file_pointer;
			if ((pcurrent_entry_file->rtfsl_direntry_type&RTFSL_ENTRY_TYPE_EOF)==0)
				pcreate_structure->eof=1;
		}
 		else if (!pcreate_structure->eof && ANSImemcmp(pcreate_structure->name,pcurrent_entry_file->dos_inode.fname,11)==0)
		{ /* Don't allow any duplicate names even if different attributes */
			return RTFSL_ERROR_EXIST;
		}
	}
	pcreate_structure->file_pointer+=32;
    return 0; /* Continue */
}

int rtfsl_create(unsigned char *name,unsigned char attribute)   /*__apifn__*/
{
int rval;
struct rtfsl_create_structure create_structure;
struct rtfsl_file new_directory_file;
unsigned short date, time;
int fd;

    fd = rtfsl_alloc_fd();
    if (fd<0)
        return fd;
    rtfsl_get_system_date(&time, &date);
	rval = rtfsl_open_path(rtfsl.current_dr.pathnamearray,0, &new_directory_file, &rtfsl.rtfsl_files[fd]);
	if (rval==0)
	{
		create_structure.free_file_pointer=NOFREEFILESFOUND;
		create_structure.file_pointer=0;
		create_structure.name = name;
		create_structure.attribute=attribute;
		create_structure.eof=0;
		/* Find a slot */
		rval = rtfsl_enumerate_directory(&rtfsl.rtfsl_files[fd],&new_directory_file, create_callback,(void *) &create_structure);
	    if(rval==0)
	    {
			unsigned long seek_to;
			rval = rtfsl_finode_open(&rtfsl.rtfsl_files[fd]);

			if (create_structure.free_file_pointer==NOFREEFILESFOUND)
				seek_to=create_structure.file_pointer;
			else
				seek_to=create_structure.free_file_pointer;
			/* Will fail on 16 bit systems if the direcory extents > 32 K*/
			if ((unsigned long)rtfsl_write(fd,0,seek_to)!=seek_to)
			{
				rval=RTFSL_ERROR_CONSISTENCY;
			}
			else
			{
				if (create_structure.free_file_pointer==NOFREEFILESFOUND && rtfsl.current_dr.fasize<8 && (rtfsl.rtfsl_files[fd].rtfsl_direntry_type&TRTFSFILE_ISROOT_DIR))
					rval=RTFSL_ERROR_DIR_FULL;
			}
			if (rval==0)
			{
#if (RTFSL_INCLUDE_FAILSAFE_SUPPORT)
				if (rtfsl.rtfsl_current_failsafe_context)
					rtfsl.rtfsl_current_failsafe_context->flags |= RTFSLFS_WRITE_DIRENTRY;
#endif
				// Format the dos inode and write it
				ANSImemcpy(&new_directory_file.dos_inode.fname,name,11);
				new_directory_file.dos_inode.fattribute=attribute;      /* File attributes */
				new_directory_file.dos_inode.reservednt=0;
				new_directory_file.dos_inode.create10msincrement=0;
				new_directory_file.dos_inode.ctime=time;            /* time & date create */
				new_directory_file.dos_inode.cdate=date;
				new_directory_file.dos_inode.adate=date;			  /* Date last accessed */
				new_directory_file.dos_inode.ftime=time;            /* time & date lastmodified */
				new_directory_file.dos_inode.fdate=date;
				new_directory_file.dos_inode.fsize =0;
				new_directory_file.dos_inode.fcluster=0;
				new_directory_file.dos_inode.fclusterhi=0;
				// call rtfsl_write on new_directory_file to extend it
                rval=rtfsl_write(fd,(unsigned char *)&new_directory_file.dos_inode,32);
                if (rval>=0)
					rval=rtfsl_flush(fd);
#if (RTFSL_INCLUDE_FAILSAFE_SUPPORT)
				if (rtfsl.rtfsl_current_failsafe_context)
					rtfsl.rtfsl_current_failsafe_context->flags &= ~RTFSLFS_WRITE_DIRENTRY;
#endif
			}
		}
	}
    rtfsl.rtfsl_files[fd].rtfsl_file_flags=0;
    return rval;
}
#include <stdio.h>
int rtfsl_write(int fd, unsigned char *in_buff, int count)    /*__apifn__*/
{
unsigned long needed,bytespcluster,prev_cluster;
int rval,i,updatesize=0;

	bytespcluster=rtfsl.current_dr.bytespcluster;
	needed = rtfsl.rtfsl_files[fd].file_pointer+count;
	if (needed > rtfsl.rtfsl_files[fd].dos_inode.fsize)
	{
	unsigned long allocated =
		((rtfsl.rtfsl_files[fd].dos_inode.fsize+rtfsl.current_dr.bytespcluster-1)/rtfsl.current_dr.bytespcluster)
		*bytespcluster;
		unsigned long next_segment_base=0;
		prev_cluster=0;
		for (i =0;i < RTFSL_CFG_FILE_FRAG_BUFFER_SIZE;i++)
		{
			if(rtfsl.rtfsl_files[fd].cluster_segment_array[i][0])
				prev_cluster=rtfsl.rtfsl_files[fd].cluster_segment_array[i][1];
			else
				break;
		}
		updatesize=1;
		while (allocated < needed)
		{
		int n;
		unsigned long new_cluster,value;
			n=fatop_buff_get_frag(rtfsl.current_dr.next_alloc|RTFSL_ALLOC_CLUSTER, &new_cluster, 1);
			if (n==0)
			{
				rtfsl.current_dr.next_alloc=2;
				n=fatop_buff_get_frag(rtfsl.current_dr.next_alloc|RTFSL_ALLOC_CLUSTER, &new_cluster, 1);

			}
			if (n==1)
            {
				rtfsl.current_dr.next_alloc=new_cluster;
				if (rtfsl.current_dr.free_alloc>=1)
					rtfsl.current_dr.free_alloc-=1;
            }
			else if (n<1)
            {
                if (n==0)
				    return RTFSL_ERROR_DISK_FULL;
                else
                    return n;
            }
#if (RTFSL_INCLUDE_FAILSAFE_SUPPORT)
			if (!rtfsl.rtfsl_current_failsafe_context)
#endif
			{
				/* Write a terminate chain value before linking to it.
				   This is safer if not using Failsafe, but it reduces the journal file efficiency when using it */
				value=rtfsl.current_dr.end_cluster_marker|0xf;
				rval=fatop_buff_get_frag(new_cluster|RTFSL_WRITE_CLUSTER, &value, 1);
				if (rval<0)
				    return rval;
			}

			if (prev_cluster)
			{ /* Link the previous cluster to the new one */
				value=new_cluster;
				rval=fatop_buff_get_frag(prev_cluster|RTFSL_WRITE_CLUSTER, &value, 1);
				if (rval<0)
					return rval;
			}
			else
			{
				fr_USHORT((unsigned char *) &rtfsl.rtfsl_files[fd].dos_inode.fcluster,(unsigned short)(new_cluster&0xffff));
				fr_USHORT((unsigned char *) &rtfsl.rtfsl_files[fd].dos_inode.fclusterhi,(unsigned short)((new_cluster>>16)&0xffff));
			}
#if (RTFSL_INCLUDE_FAILSAFE_SUPPORT)
			if (rtfsl.rtfsl_current_failsafe_context)
			{
				/* Write failsafe terminating the chain after extending it makes it easier to coalesce chains in the journal */
				value=rtfsl.current_dr.end_cluster_marker|0xf;
				rval=fatop_buff_get_frag(new_cluster|RTFSL_WRITE_CLUSTER, &value, 1);
				if (rval<0)
					return rval;
			}
#endif
			if (!next_segment_base) /* We are allocating, but not loading the chain, this allows load next segment to follw the chain */
				rtfsl.rtfsl_files[fd].next_segment_base=next_segment_base=new_cluster;
			prev_cluster=new_cluster;
			allocated+=bytespcluster;
		}

    }

	rval=_rtfsl_bfilio_io(fd, in_buff, (long)count, RTFSL_FIO_OP_WRITE);
	if (rval<0)
		return rval;
	if (updatesize)
    {
        rtfsl.rtfsl_files[fd].rtfsl_file_flags|=TRTFSFILE_DIRTY;
		rtfsl.rtfsl_files[fd].dos_inode.fsize=rtfsl.rtfsl_files[fd].file_pointer;
    }
	return count;
}

int rtfsl_flush(int fd)           /*__apifn__*/
{
	unsigned long l;
	unsigned char *p;
    int rval;

	if (rtfsl.rtfsl_files[fd].rtfsl_file_flags&(TRTFSFILE_ISROOT_DIR|TRTFSFILE_SECTOR_REGION))
		return 0;
	l=rtfsl.rtfsl_files[fd].dos_inode.fsize;
	if (rtfsl.rtfsl_files[fd].dos_inode.fattribute&(ADIRENT|AVOLUME))
		fr_ULONG((unsigned char *) &rtfsl.rtfsl_files[fd].dos_inode.fsize,0);
	else
		fr_ULONG((unsigned char *) &rtfsl.rtfsl_files[fd].dos_inode.fsize,l);
#if (RTFSL_INCLUDE_FAILSAFE_SUPPORT)
    /* Submit the entry to the journal failsafe is on */
	if (rtfsl.rtfsl_current_failsafe_context)
	{
		rval=rtfslfs_dirent_remap(rtfsl.current_dr.partition_base+rtfsl.rtfsl_files[fd].sector,rtfsl.rtfsl_files[fd].index, &rtfsl.rtfsl_files[fd].dos_inode,0);
		rtfsl.rtfsl_files[fd].dos_inode.fsize=l;
		if (rval<0)
			return rval;
		else if (rval !=1)
			return RTFSL_ERROR_CONSISTENCY;
		else
			return 0;
	}
#endif
	rval=rtfsl_read_sector_buffer(rtfsl.current_dr.partition_base+rtfsl.rtfsl_files[fd].sector);
    if (rval<0)
	    return (int) rval;
	p = rtfsl_buffer_address(rval)+rtfsl.rtfsl_files[fd].index*32;
	ANSImemcpy(p,&rtfsl.rtfsl_files[fd].dos_inode,32);
	rtfsl.rtfsl_files[fd].dos_inode.fsize=l;
    rval=rtfsl_flush_sector_buffer(rval,1);
    return rval;
}
int rtfsl_flush_info_sec(void)      /*__apifn__*/
{
#if (RTFSL_INCLUDE_FAT32)
    int rval=0;
	if ((rtfsl.current_dr.flags&RTFSL_FAT_CHANGED_FLAG) && rtfsl.current_dr.infosec)
	{
		rtfsl.current_dr.flags&=~RTFSL_FAT_CHANGED_FLAG;
#if (RTFSL_INCLUDE_FAILSAFE_SUPPORT)
		if (rtfsl.rtfsl_current_failsafe_context)
		{
			struct rtfsl_dosinode info_inode;
			unsigned long *pl;
            info_inode.fname[0]='A'; /* As long as it is not the delete character */
            pl=(unsigned long *)&info_inode;
			pl++;
			*pl++=rtfsl.current_dr.free_alloc;
			*pl=rtfsl.current_dr.next_alloc;
			/* Put the information in a fake dos_inode record in the journal. restore will recognize the sector as the info sector and call rtfsl_flush_info_sec() to update */
			if (rtfsl.rtfsl_current_failsafe_context)
				return rtfslfs_dirent_remap(rtfsl.current_dr.partition_base+rtfsl.current_dr.infosec,0, &info_inode,0);
		}
#endif
	    rval=rtfsl_read_sector_buffer(rtfsl.current_dr.partition_base+rtfsl.current_dr.infosec);
	    if (rval>=0)
	    {
	        unsigned char *p = rtfsl_buffer_address(rval);
	        fr_ULONG((p+488),rtfsl.current_dr.free_alloc);
	        fr_ULONG((p+492),rtfsl.current_dr.next_alloc);
	        rval=rtfsl_flush_sector_buffer(rval,1);
	    }
	}
    return rval;
#else
    return 0;
#endif
}
