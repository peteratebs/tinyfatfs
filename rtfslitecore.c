/*
* EBS - RTFS (Real Time File Manager)
*
* Copyright EBS Inc. 1987-2012
* All rights reserved.
* This code may not be redistributed in source or linkable object form
* without the consent of its author.
*/


#include "rtfslite.h"

#define BUFHMASK(H) (1<<H)
#define BUFHMASKNOT(H) (~(1<<H))

static const unsigned char dos_partition_types[]= {
                    0x01,0x04,0x06,
                    0x0E,   /* Windows FAT16 Partition */
                    0x0B,   /* FAT32 Partition */
                    0x0C,   /* FAT32 Partition */
                    0x55,   /* FAT32 Partition */
                    0x07    /* exFat Partition */
                    };
#if (RTFSL_CFG_FILE_FRAG_BUFFER_SIZE>1)
int rtfsl_load_cluster_chain(unsigned long start_cluster, unsigned long *segment_length_clusters, unsigned long *start_next_segment, unsigned long cluster_segment_array[][2], int cluster_segment_array_size);
#endif
#if (RTFSL_INCLUDE_WRITE_SUPPORT)


void rtfsl_mark_sector_buffer(int bufferhandle)
{
    rtfsl.buffer_pool_dirty|=BUFHMASK(bufferhandle);
}

int rtfsl_flush_sector_buffer(int bufferhandle,int force)
{
	int rval=0;
    if (force||(rtfsl.buffer_pool_dirty&BUFHMASK(bufferhandle)))
    {
        unsigned char *b = rtfsl.pcurrent_buffer_pool+(bufferhandle*rtfsl.current_dr.bytespsector);
	    rval = rtfsl_write_sectors(rtfsl.current_buffered_sectors[bufferhandle],1,b);
        if (rval==0&&rtfsl.current_dr.numfats==2)
        {
        unsigned long l;
            l=rtfsl.current_buffered_sectors[bufferhandle]-rtfsl.current_dr.partition_base;
            if (l>=rtfsl.current_dr.secreserved && l<rtfsl.current_dr.secreserved+(unsigned long)rtfsl.current_dr.secpfat)
		        rval = rtfsl_write_sectors(rtfsl.current_buffered_sectors[bufferhandle]+(unsigned long)rtfsl.current_dr.secpfat,1,b);
        }
	    rtfsl.buffer_pool_dirty&=BUFHMASKNOT(bufferhandle);

    }
    return rval;
}
int rtfsl_flush_all_buffers(void)     /*__apifn__*/
{
int bufferhandle,rval;
    rval=0;
    for(bufferhandle=0;rval==0&&bufferhandle<RTFSL_CFG_NUMBUFFERS;bufferhandle++)
        rval=rtfsl_flush_sector_buffer(bufferhandle,0);
#if (RTFSL_INCLUDE_FAT32)
	rval=rtfsl_flush_info_sec();
#endif
    return rval;
}
#endif


int rtfsl_read_sector_buffer(unsigned long sector)
{
int found,rval;
unsigned char *b,i,bufferhandle;
	found=0;
	bufferhandle=rtfsl.buffer_pool_agingstack[RTFSL_CFG_NUMBUFFERS-1];
	rval=0;
#if(RTFSL_CFG_NUMBUFFERS>1)
	b=rtfsl.buffer_pool_agingstack;
	for(i=0;i<RTFSL_CFG_NUMBUFFERS;i++)
	{
		if (sector==rtfsl.current_buffered_sectors[i])
		{
			bufferhandle=i;
			found=1;
			break;
		}
	}
#endif

#if (RTFSL_INCLUDE_WRITE_SUPPORT)
	if (!found)
		rval = rtfsl_flush_sector_buffer(bufferhandle,0);
#endif
    if (rval>=0)
	{
		if (!found)
			rval=rtfsl_read_sectors(sector,1, rtfsl.pcurrent_buffer_pool+(bufferhandle*rtfsl.current_dr.bytespsector));
		if (rval>=0)
		{
			int ntocopy=0;
			rtfsl.current_buffered_sectors[bufferhandle]=sector;
			rval=(int)bufferhandle;
#if (RTFSL_CFG_NUMBUFFERS>1)
            if (b[0]!=bufferhandle)
            {   /* Move the buffer to the top of the aging stack if it isn;t already there */
			    for (i=1;i<RTFSL_CFG_NUMBUFFERS-1;i++)
			    {
				    if(b[i]==bufferhandle)
				    {
					    ntocopy=i;
					    break;
				    }
			    }
			    if (!ntocopy)
				    ntocopy=RTFSL_CFG_NUMBUFFERS-1;
			    ANSImemcpy(b+1,b,ntocopy);
            }
#endif
			b[0]=bufferhandle;
		}
	}
	return rval;
}

unsigned long rtfsl_sec2cluster(unsigned long sector)
{
	if (sector < rtfsl.current_dr.firstclblock)
		return 0;
	return 2+(sector-rtfsl.current_dr.firstclblock)/rtfsl.current_dr.secpalloc;
}


unsigned long rtfsl_cl2sector(unsigned long cluster)
{
unsigned long cl;
    if (cluster > 1)
        cl = rtfsl.current_dr.firstclblock + (cluster - 2)*rtfsl.current_dr.secpalloc;
    else
        cl=0;
    return cl;
}



int rtfsl_diskopen(void)     /*__apifn__*/
{
unsigned char *sector_buffer;
unsigned char i,*b;
int rval;


	ANSImemset(&rtfsl, 0, sizeof(rtfsl));
    rtfsl.pcurrent_buffer_pool=rtfsl_sector_buffer;
//    ANSImemset(&rtfsl.current_dr, 0, sizeof(rtfsl.current_dr));
	/* Initialize a stack we use to age buffer usage. The bottom of the stack is the least recently used handle */
	for(i=0;i<RTFSL_CFG_NUMBUFFERS;i++)
		rtfsl.buffer_pool_agingstack[i]=i;
    /* Check for MBR */
    rval=rtfsl_read_sectors(0,1,rtfsl.pcurrent_buffer_pool);
    if (rval<0)
	    return rval;
    sector_buffer=rtfsl.pcurrent_buffer_pool;
#if (RTFSL_INCLUDE_MBR_SUPPORT)
    if (sector_buffer[511]==0xaa)
    {
        if (sector_buffer[510]==0x55
#if (INCLUDE_WINDEV)
        || sector_buffer[510]==0x66
#endif
        )
        {
        int i;
        unsigned char *p=&sector_buffer[446];
            for(i=0; rtfsl.current_dr.partition_base==0 && i < 4; i++,p+=16)
            {
                int j;
                unsigned char p_type = *(p+4);
                for (j = 0; j < sizeof(dos_partition_types);j++)
                {
                    if(p_type==dos_partition_types[j])
                    {
                        rtfsl.current_dr.partition_base = to_ULONG  (p+8);
                        rtfsl.current_dr.partition_size =  to_ULONG (p+12);
                        rval=rtfsl_read_sectors(rtfsl.current_dr.partition_base,1,sector_buffer);
                        if (rval<0)
                            return rval;
                        break;
                    }
				}
			}
		}
    }
#endif
    /* Check for BPB signature */
    b=&sector_buffer[0];
    /* The valid bpb test passes for NTFS. So identify NTFS and force jump to zero so the bpb test fails. */
    /* Check for EXFAT and NTFS */
    if (ANSImemcmp(b+3, (void *)"EXFAT",4)==0||ANSImemcmp(b+3, (void *)"NTFS",4)==0)
	{
#if (INCLUDE_EXFAT)
		!!!!!!;
#else
	    return RTFSL_ERROR_FORMAT;
#endif
	}
    if ( (*b!=0xE9) && (*b!=0xEB)
#if (INCLUDE_WINDEV)
        && (*b!=0xE9)    /* E8 BPB signature so volume it is writeable*/
#endif
    )
    {
	    return RTFSL_ERROR_FORMAT;
    }

    rtfsl.current_dr.secpalloc      = b[0xd];
    rtfsl.current_dr.numfats        = b[0x10];
    rtfsl.current_dr.bytespsector   = to_USHORT(b+0xb) ;
    rtfsl.current_dr.secreserved    = to_USHORT(b+0xe) ;
    rtfsl.current_dr.numroot        = to_USHORT(b+0x11);
    rtfsl.current_dr.numsecs        = to_USHORT(b+0x13);
    rtfsl.current_dr.secpfat        = (unsigned long) to_USHORT(b+0x16);
    rtfsl.current_dr.bytespcluster  = (unsigned long) rtfsl.current_dr.bytespsector*rtfsl.current_dr.secpalloc;
	rtfsl.current_dr.rootbegin		= rtfsl.current_dr.secreserved + rtfsl.current_dr.secpfat*rtfsl.current_dr.numfats;
    if (rtfsl.current_dr.numsecs==0)
        rtfsl.current_dr.numsecs        = to_ULONG(b+0x20);/*X*/ /* # secs if > 32M (4.0) */
    if (rtfsl.current_dr.numroot == 0)
        rtfsl.current_dr.fasize = 8;
	else
	{ /* Check the bpb file sys type field. If it was initialized by format use that to determine FAT type */
        if (ANSImemcmp(b+0x36, (void *)"FAT1",4)==0)
		{
			if (*(b + 0x3A) == (unsigned char)'2')
#if (RTFSL_INCLUDE_FAT12)
				rtfsl.current_dr.fasize = 3;
#else
			    return RTFSL_ERROR_FORMAT;
#endif
			else
			if (*(b + 0x3A) == (unsigned char)'6')
				rtfsl.current_dr.fasize = 4;
		}
		else
			return RTFSL_ERROR_FORMAT;
	}
    if (rtfsl.current_dr.fasize == 8)
    {
#if (RTFSL_INCLUDE_FAT32)
        rtfsl.current_dr.secpfat     = to_ULONG(b+0x24);
        rtfsl.current_dr.flags       = to_USHORT(b+0x28);
        rtfsl.current_dr.rootbegin   = to_ULONG(b+0x2c);
        rtfsl.current_dr.infosec     = to_USHORT(b+0x30);
        rtfsl.current_dr.backup      = to_USHORT(b+0x32);
        /* Read one block */
        rval=rtfsl_read_sectors(rtfsl.current_dr.partition_base+rtfsl.current_dr.infosec,1,sector_buffer);
        if (rval<0)
            return rval;
		b=sector_buffer;
        b += 484;
#define FSINFOSIG 0x61417272ul
		if (FSINFOSIG == to_ULONG(b))
		{
        	rtfsl.current_dr.free_alloc = to_ULONG(b+4);
        	rtfsl.current_dr.next_alloc = to_ULONG(b+8);
		}
		else
		{  /* If the signature is not found default to the beginning of the FAT */
            rtfsl.current_dr.infosec     = 0;
        	rtfsl.current_dr.free_alloc = 0xffffffff;
        	rtfsl.current_dr.next_alloc = 2;

		}
#else
        return RTFSL_ERROR_FORMAT;
#endif
    }
	else
	{  /* If the signature is not found default to the beginning of the FAT */
       	rtfsl.current_dr.free_alloc = 0xffffffff;
       	rtfsl.current_dr.next_alloc = 2;
	}

    rtfsl.current_dr.firstclblock = rtfsl.current_dr.secreserved + (unsigned long)rtfsl.current_dr.secpfat*rtfsl.current_dr.numfats;
    if (rtfsl.current_dr.fasize != 8)
        rtfsl.current_dr.firstclblock += ((unsigned long)rtfsl.current_dr.numroot*32+rtfsl.current_dr.bytespsector-1)/rtfsl.current_dr.bytespsector;
#if (RTFSL_INCLUDE_FAT12)
	if (rtfsl.current_dr.fasize == 3)
	{
		rtfsl.current_dr.end_cluster_marker = 0xff7;
	}
	else
#endif
	if (rtfsl.current_dr.fasize == 4)
	{
		 rtfsl.current_dr.end_cluster_marker = 0xfff7;
	}
#if (RTFSL_INCLUDE_FAT32)
	else /*  if (rtfsl.current_dr.fasize == 8) */
	{
		 rtfsl.current_dr.end_cluster_marker = 0x0ffffff7;
	}
#endif
	rtfsl.current_dr.maxfindex = (unsigned long)(1 + ((rtfsl.current_dr.numsecs-rtfsl.current_dr.firstclblock)/rtfsl.current_dr.secpalloc));
    {
        /* Make sure the calculated index doesn't overflow the fat sectors */
        unsigned long max_index;
        /* For FAT32 Each block of the fat holds 128 entries so the maximum index is
           (pdr->secpfat * pdr->drive_info.clpfblock32 (128 for block size 512) )-1; */
		max_index = (unsigned long) rtfsl.current_dr.secpfat;
		max_index *= (rtfsl.current_dr.bytespsector*2)/rtfsl.current_dr.fasize;
        max_index -= 1;
        if (rtfsl.current_dr.maxfindex > max_index)
            rtfsl.current_dr.maxfindex = max_index;
    }
    return(0);
}

int rtfsl_finode_get(struct rtfsl_file *pfile, unsigned long sector, unsigned short index)
{
unsigned char *sector_buffer;
struct rtfsl_dosinode *pdos_inode;
int rval;
    ANSImemset(pfile, 0, sizeof(*pfile));
    rval=rtfsl_read_sector_buffer(rtfsl.current_dr.partition_base+sector);
	if (rval<0)
	    return rval;
    sector_buffer = rtfsl_buffer_address(rval);
    pfile->sector=sector;
    pfile->index=index;
    pdos_inode = (struct rtfsl_dosinode *) sector_buffer;
	pdos_inode += index;
#if (RTFSL_INCLUDE_FAILSAFE_SUPPORT)
	if (rtfsl.rtfsl_current_failsafe_context)
    {
        /* Check the journal file for an override directory entry and copy it into the buffer if one is found. */
        rval=rtfslfs_dirent_remap(rtfsl.current_dr.partition_base+sector,index, pdos_inode,1);
        if (rval<0)
            return rval;
    }
#endif
    pfile->dos_inode = *pdos_inode;
	if (pfile->dos_inode.fname[0]==0 || (ANSImemcmp(pfile->dos_inode.fname,end_name,8)==0) )
		pfile->rtfsl_direntry_type =RTFSL_ENTRY_TYPE_EOF;
	else if (pfile->dos_inode.fname[0]==PCDELETE)
		pfile->rtfsl_direntry_type =RTFSL_ENTRY_TYPE_ERASED;
	else
	{
	    /* Convert Kanji esc character 0x05 to 0xE5 */
        if (pfile->dos_inode.fname[0]==0x05)
            pfile->dos_inode.fname[0]=PCDELETE;
		if (pfile->dos_inode.fattribute== CHICAGO_EXT)
			 pfile->rtfsl_direntry_type =RTFSL_ENTRY_TYPE_LFN;
		else if (pfile->dos_inode.fattribute&AVOLUME)
			pfile->rtfsl_direntry_type =RTFSL_ENTRY_TYPE_VOLUME;
		else if (pfile->dos_inode.fattribute&ADIRENT)
			pfile->rtfsl_direntry_type =RTFSL_ENTRY_TYPE_DIRECTORY;
		else
        {
            /* Dos file size is set during get, direectory size during open */
            /* Swap pfile->dos_inode.fsize since we use it a lot */
            unsigned long l = to_ULONG((unsigned char *)&pfile->dos_inode.fsize);
			pfile->rtfsl_direntry_type =RTFSL_ENTRY_TYPE_FILE;
			pfile->dos_inode.fsize=l;
        }
	}
    return 0;
}
/* Load cluster chains set file size if it's a directory */
int rtfsl_finode_open(struct rtfsl_file *pfile)
{
unsigned long current_cluster,chain_length_bytes;
long segments;
	pfile->file_pointer=0;
	pfile->file_pointer_at_segment_base=0;
	pfile->segment_size_bytes=0;
	if (pfile->rtfsl_file_flags & TRTFSFILE_SECTOR_REGION)
		return 0;
	else if (pfile->rtfsl_file_flags & TRTFSFILE_ISROOT_DIR)
    {
#if (RTFSL_INCLUDE_FAT32)
        if (rtfsl.current_dr.fasize==8)
            current_cluster = rtfsl.current_dr.rootbegin;
        else
#endif
        {
	        pfile->dos_inode.fsize = rtfsl.current_dr.numroot*32;
	        pfile->cluster_segment_array[0][0]=rtfsl.current_dr.rootbegin;
			pfile->cluster_segment_array[0][1]=rtfsl.current_dr.rootbegin+pfile->dos_inode.fsize/rtfsl.current_dr.bytespsector;
			pfile->segment_size_bytes=pfile->dos_inode.fsize;
			pfile->next_segment_base=0;
			pfile->rtfsl_file_flags|=TRTFSFILE_SECTOR_REGION;
            return 0;
        }
    }
    else
	{
		current_cluster = to_USHORT((unsigned char *)&pfile->dos_inode.fclusterhi);
		current_cluster <<= 16;
		current_cluster |= to_USHORT((unsigned char *)&pfile->dos_inode.fcluster);
	}
	if (!current_cluster)
		return 0;
	/* Get the first segment */
	segments = rtfsl_load_next_segment(pfile,current_cluster);
	if (segments < 0)
		return (int)segments;
	/* Traverse the cluster chain, return the total number of segments, &chain_length_clusters and populate pfile->cluster_segment_array with up to RTFSL_CFG_FILE_FRAG_BUFFER_SIZE segments */
	chain_length_bytes=pfile->segment_size_bytes;
	if (pfile->cluster_segment_array[RTFSL_CFG_FILE_FRAG_BUFFER_SIZE-1][0])
	{
		while (pfile->cluster_segment_array[RTFSL_CFG_FILE_FRAG_BUFFER_SIZE-1][0])
		{
			segments = rtfsl_load_next_segment(pfile,0);
			if (segments < 0)
				return (int)segments;
			chain_length_bytes+=pfile->segment_size_bytes;
		}
		/* Get the first segment again */
		pfile->segment_size_bytes=0;
		pfile->file_pointer_at_segment_base=0;
		segments=rtfsl_load_next_segment(pfile,current_cluster);
		if (segments < 0)
			return (int)segments;
	}
    /* Dos file size is set during get, direectory size during open */
    if (pfile->dos_inode.fattribute & ADIRENT)
    {
        pfile->dos_inode.fsize = chain_length_bytes;
    }
    return 0;
}

int rtfsl_root_finode_open(struct rtfsl_file *pfile)
{
    ANSImemset(pfile, 0, sizeof(*pfile));
	pfile->rtfsl_file_flags = TRTFSFILE_ISROOT_DIR|TRTFSFILE_ALLOCATED;
	pfile->rtfsl_direntry_type=RTFSL_ENTRY_TYPE_DIRECTORY;
    pfile->dos_inode.fattribute = ADIRENT|TRTFSFILE_ISROOT_DIR;
    return rtfsl_finode_open(pfile);
}
/* Returns bytes */
long rtfsl_load_next_segment(struct rtfsl_file *pfile,unsigned long current_cluster)
{
#if (RTFSL_CFG_FILE_FRAG_BUFFER_SIZE>1)
long cluster_segments;
unsigned long cluster_segment_length;
#endif
	ANSImemset(pfile->cluster_segment_array,0,sizeof(pfile->cluster_segment_array));
	if (!current_cluster)
	    current_cluster=pfile->next_segment_base;
	if (!current_cluster)
	{
		return 0;
	}
#if (RTFSL_CFG_FILE_FRAG_BUFFER_SIZE==1)
	{
	int length;
 		length = fatop_buff_get_frag(current_cluster, &pfile->next_segment_base,pfile->rtfsl.current_dr.end_cluster_marker);
		if (length > 0)
		{
			pfile->segment_size_bytes = (unsigned long) length*pfile->rtfsl.current_dr.bytespcluster;
			return 1;
		}
		else
			return length;
	}
#else
     /* Traverse the cluster chain, return the total number of segments, &chain_length_clusters and populate pfile->cluster_segment_array with up to RTFSL_CFG_FILE_FRAG_BUFFER_SIZE segments */
    cluster_segments = (long)rtfsl_load_cluster_chain(current_cluster, &cluster_segment_length, &pfile->next_segment_base, pfile->cluster_segment_array, RTFSL_CFG_FILE_FRAG_BUFFER_SIZE);
    if (cluster_segments>=0)
	{
		pfile->file_pointer_at_segment_base += pfile->segment_size_bytes;
        cluster_segment_length = cluster_segment_length*rtfsl.current_dr.bytespcluster;
		pfile->segment_size_bytes=cluster_segment_length;
	}
    return cluster_segments;
#endif
}


#if (RTFSL_CFG_FILE_FRAG_BUFFER_SIZE>1)
/*

    Scan from start_cluster, if chain_length_clusters is zero break out when is hit
*/
int rtfsl_load_cluster_chain(unsigned long start_cluster, unsigned long *segment_length_clusters, unsigned long *start_next_segment, unsigned long cluster_segment_array[][2], int cluster_segment_array_size)
{
unsigned long next_cluster;
int segment_count;
	segment_count=0;
    *segment_length_clusters=0;
	*start_next_segment=0;
	ANSImemset(cluster_segment_array,0,sizeof(cluster_segment_array[0])*cluster_segment_array_size);
	while (start_cluster != 0)
	{
	int length;
 		length = fatop_buff_get_frag(start_cluster, &next_cluster,rtfsl.current_dr.end_cluster_marker);
		if (length < 0)
			return length;

		if (segment_count<cluster_segment_array_size)
		{
			cluster_segment_array[segment_count][0]=start_cluster;
			cluster_segment_array[segment_count][1]=start_cluster+length-1;
			if (segment_length_clusters)
				*segment_length_clusters += length;
			*start_next_segment=next_cluster;
		}
        else
            break;
		segment_count += 1;
		start_cluster=next_cluster;
	}
	return segment_count;
}
#endif
int rtfsl_enumerate_file(struct rtfsl_file *pfile,FileScanCallback pCallback, void *puser_data)   /*__apifn__*/
{
unsigned long start_sector,end_sector,nbytes,callback_value;
int current_cluster_segment = RTFSL_CFG_FILE_FRAG_BUFFER_SIZE;

    pfile->file_pointer=0;
	pfile->file_pointer_at_segment_base=0;
	pfile->segment_size_bytes=0;
	pfile->next_segment_base=0;
	pfile->cluster_segment_array[0][0]=0;
	pfile->next_segment_base = to_USHORT((unsigned char *)&pfile->dos_inode.fclusterhi);
	pfile->next_segment_base <<= 16;
	pfile->next_segment_base |= to_USHORT((unsigned char *)&pfile->dos_inode.fcluster);
	for(;;)
	{
		if (current_cluster_segment >= RTFSL_CFG_FILE_FRAG_BUFFER_SIZE)
		{
        int rval;
			rval = rtfsl_load_next_segment(pfile,0);
			if (rval<= 0)
				return rval;
			current_cluster_segment = 0;
		}
		if (!pfile->cluster_segment_array[current_cluster_segment][0])
			break;
		start_sector=rtfsl_cl2sector(pfile->cluster_segment_array[current_cluster_segment][0]);
		end_sector=rtfsl_cl2sector(pfile->cluster_segment_array[current_cluster_segment][1])+rtfsl.current_dr.secpalloc-1;
		nbytes = (end_sector-start_sector+1)*rtfsl.current_dr.bytespsector;
		if (pfile->file_pointer+nbytes > pfile->dos_inode.fsize)
			nbytes = pfile->dos_inode.fsize-pfile->file_pointer;
		if (nbytes==0)
            return 0;
		callback_value=pCallback(pfile,start_sector, nbytes, puser_data);
        if (callback_value==0)
            return 0;
		pfile->file_pointer += callback_value;
        if (pfile->file_pointer >= pfile->dos_inode.fsize)
			pfile->file_pointer=pfile->dos_inode.fsize;
        if (callback_value != nbytes ||pfile->file_pointer >= pfile->dos_inode.fsize)
		{
			return 0;
		}
		current_cluster_segment += 1;

	}
	return 0;
}


int rtfsl_enumerate_directory(struct rtfsl_file *pdirectory_file,struct rtfsl_file *pcurrent_entry_file,DirScanCallback pCallback, void *puser_data)      /*__apifn__*/
{
unsigned long sector,start_sector,end_sector,sector_offset;
int current_cluster_segment = 0;
unsigned short start_index;
   sector_offset = pdirectory_file->file_pointer/rtfsl.current_dr.bytespsector;
   start_index =  (unsigned short) (pdirectory_file->file_pointer%rtfsl.current_dr.bytespsector)/32;
	pcurrent_entry_file->rtfsl_direntry_type =RTFSL_ENTRY_TYPE_EOF;
	for(;;)
	{
		if (pdirectory_file->rtfsl_file_flags & TRTFSFILE_SECTOR_REGION)
		{
			start_sector=pdirectory_file->cluster_segment_array[current_cluster_segment][0];
			end_sector = pdirectory_file->cluster_segment_array[current_cluster_segment][1];
		}
#if (RTFSL_INCLUDE_SUBDIRECTORIES)
		else
		{
			start_sector=rtfsl_cl2sector(pdirectory_file->cluster_segment_array[current_cluster_segment][0]);
			end_sector=rtfsl_cl2sector(pdirectory_file->cluster_segment_array[current_cluster_segment][1])+rtfsl.current_dr.secpalloc-1;

		}
#endif
        if (sector_offset)
        {
            unsigned long l = end_sector-start_sector+1;
            if (sector_offset>l)
            {
                sector_offset-=l;
                end_sector=start_sector-1;   /* Dont scan any sectors just advance */

            }
            else
            {
                start_sector+=sector_offset;
            }
        }
		for (sector=start_sector;sector<=end_sector;sector++)
		{
			unsigned short  index;
			for (index=start_index; index < rtfsl.current_dr.bytespsector/32; index++)
			{
            int callback_value;
				rtfsl_finode_get(pcurrent_entry_file, sector, index);
				pdirectory_file->file_pointer += 32;
				callback_value=pCallback(pcurrent_entry_file, puser_data);
				if (callback_value != 0)
                    return callback_value;
			}
            start_index=0;
		}
#if (!RTFSL_INCLUDE_SUBDIRECTORIES)
          break;
#else
		if (pdirectory_file->rtfsl_file_flags & TRTFSFILE_SECTOR_REGION)
			break;

		else
		{
			if (pcurrent_entry_file->rtfsl_direntry_type==RTFSL_ENTRY_TYPE_EOF)
				break;
			current_cluster_segment += 1;
			if (current_cluster_segment >= RTFSL_CFG_FILE_FRAG_BUFFER_SIZE)
			{
			    int rval;
				rval = rtfsl_load_next_segment(pdirectory_file,0);
				if (rval<= 0)
					break;
				current_cluster_segment = 0;
			}
			if (!pdirectory_file->cluster_segment_array[current_cluster_segment][0])
				break;
		}
#endif
	}
	return 0;
}

int fatop_buff_get_frag(unsigned long current_cluster, unsigned long *pnext_cluster, unsigned long max_length)
{
    unsigned long next_cluster,sector,fat12accumulator,last_sector;
    unsigned long n_contig;
	unsigned char *sector_buffer;
	int Fat12ReadState,buffernumber;
	int byte_offset_in_buffer,bytes_remaining,bytes_per;
#if (RTFSL_INCLUDE_WRITE_SUPPORT)
	int towrite=3;
	unsigned long writing;
	unsigned long value;
#endif

    n_contig = 1;
#if (RTFSL_INCLUDE_WRITE_SUPPORT)
	writing=current_cluster&RTFSL_MASK_COMMAND;
	current_cluster=current_cluster&~RTFSL_MASK_COMMAND;
	value = *pnext_cluster;
	if (writing==RTFSL_ALLOC_CLUSTER)
		n_contig=0;
#if (RTFSL_INCLUDE_FAT32)
	rtfsl.current_dr.flags|=RTFSL_FAT_CHANGED_FLAG; /* We have to flush the infosec */
#endif
#if (RTFSL_INCLUDE_FAILSAFE_SUPPORT)
    if (writing==RTFSL_WRITE_CLUSTER && rtfsl.rtfsl_current_failsafe_context)
    {
        return rtfslfs_cluster_map(current_cluster, value);
    }
#endif
	if (writing!=RTFSL_WRITE_CLUSTER)
#endif
		*pnext_cluster = 0x0;

	Fat12ReadState = 0;
	fat12accumulator=0;
	bytes_per=rtfsl.current_dr.fasize/2;
#if (RTFSL_INCLUDE_FAT12)
	if (rtfsl.current_dr.fasize == 3)
	{
		unsigned long bytenumber;
		bytenumber = (current_cluster * 3)/2;
		sector = bytenumber/rtfsl.current_dr.bytespsector;
		byte_offset_in_buffer = bytenumber & (rtfsl.current_dr.bytespsector-1);
		Fat12ReadState=(current_cluster % 2);
		/*
			Fat12ReadState==0 - Good to go
			FatReadstate==1   - Goto FatReadstate3
			FatReadstate==3   - Start first pass ignoring accumulator and goto state 2
			FatReadstate==2   - Decrement byte offset and go to state 3 on read state 2 on write
		*/
		if (Fat12ReadState==2)
		{
			if (bytenumber)
				bytenumber-=1;
			else
			{
				sector-=1;
				bytenumber=rtfsl.current_dr.bytespsector-1;
			}
#if (RTFSL_INCLUDE_WRITE_SUPPORT)
			if (writing==RTFSL_WRITE_CLUSTER)
				Fat12ReadState=2;
			else
#endif
				Fat12ReadState=3;
		}
		else if (Fat12ReadState==1)
#if (0 && RTFSL_INCLUDE_WRITE_SUPPORT)
			if (writing==RTFSL_WRITE_CLUSTER)
				; // Fat12ReadState=1;
			else
#endif
				Fat12ReadState=3;
	}
    else
#endif
    {
         sector = current_cluster / (rtfsl.current_dr.bytespsector/bytes_per);
		 byte_offset_in_buffer = (current_cluster & ((rtfsl.current_dr.bytespsector/bytes_per)-1)) * bytes_per;
    }
	sector = sector + rtfsl.current_dr.secreserved;
	last_sector = sector+rtfsl.current_dr.secpfat-1;
	bytes_remaining = rtfsl.current_dr.bytespsector-byte_offset_in_buffer;
    buffernumber=rtfsl_read_sector_buffer(rtfsl.current_dr.partition_base+sector);
    if (buffernumber<0)
       return buffernumber;
    sector_buffer = rtfsl_buffer_address(buffernumber);
	while(n_contig <= max_length)
	{
		while (bytes_remaining)
		{
			next_cluster=0;
			if (rtfsl.current_dr.fasize>3)
            {
			    if (bytes_per==2)
				{
#if (RTFSL_INCLUDE_WRITE_SUPPORT)
					if (writing==RTFSL_WRITE_CLUSTER)
					{
					    fr_USHORT(sector_buffer+byte_offset_in_buffer,(unsigned short)value);
						break;
					}
					else
#endif
						next_cluster = (unsigned long)to_USHORT(sector_buffer+byte_offset_in_buffer);
				}
                else
				{
#if (RTFSL_INCLUDE_WRITE_SUPPORT)
					if (writing==RTFSL_WRITE_CLUSTER)
					{
				        fr_ULONG(sector_buffer+byte_offset_in_buffer,value);
                        rtfsl_mark_sector_buffer(buffernumber);
						break;
					}
					else
#endif
					    next_cluster = (unsigned long)to_ULONG(sector_buffer+byte_offset_in_buffer);
				}
       			bytes_remaining-=bytes_per;
            	byte_offset_in_buffer+=bytes_per;
            }
#if (RTFSL_INCLUDE_FAT12)
			else // if (rtfsl.current_dr.fasize == 3)
			{
			unsigned char *p,b;
				bytes_remaining-=1;
				p=(sector_buffer+byte_offset_in_buffer);
				b = *p;
				byte_offset_in_buffer+=1;

				if (Fat12ReadState==0)
				{
					Fat12ReadState=1;
#if (RTFSL_INCLUDE_WRITE_SUPPORT)
					if (writing==RTFSL_WRITE_CLUSTER)
					{
						*p = (unsigned char )value&0xff;
						towrite-=2;
						if (!towrite)
							break;
					}
					else
#endif
						fat12accumulator = b;
					continue;
				}
				else if (Fat12ReadState==1)
				{
					Fat12ReadState=2;
#if (RTFSL_INCLUDE_WRITE_SUPPORT)
					if (writing==RTFSL_WRITE_CLUSTER)
					{
						*p &= 0xf0;
						*p |= (unsigned char)(value>>8)&0xf; // next_clusterv[hinibble];
						if (!--towrite)
							break;
						continue;
					}
#endif
					next_cluster = b&0x0f;
					next_cluster <<= 8;
					next_cluster |= fat12accumulator;
					fat12accumulator = (b&0xf0)>>4;
				}
				else if (Fat12ReadState==2)
				{
					Fat12ReadState=0;
#if (RTFSL_INCLUDE_WRITE_SUPPORT)
					if (writing==RTFSL_WRITE_CLUSTER)
					{
						*p = (unsigned char)(value>>4)&0xff; // b = v>>4; next_clusterv[hinibble];
						towrite-=2;
						if (!towrite)
							break;
						continue;
					}
#endif
					next_cluster = b<<4;
					next_cluster |= fat12accumulator;
				}
				else if (Fat12ReadState==3)
				{
					Fat12ReadState=2;
#if (RTFSL_INCLUDE_WRITE_SUPPORT)
					if (writing==RTFSL_WRITE_CLUSTER)
					{
						*p &= 0x0f;
						*p |= (unsigned char)(value&0x0f)<<4;
						if (!--towrite)
							break;
						continue;
					}
#endif
					fat12accumulator = (b&0xf0)>>4; /* hi byte of b to accumulator lo byte of value*/

					continue;
				}
			}
#endif
#if (RTFSL_INCLUDE_FAILSAFE_SUPPORT)
			/* If failsafe is enabled give it a chance to override the cluster number or return the same value, there is no failure condition */
			if (rtfsl.rtfsl_current_failsafe_context)
			{
				unsigned long mapped_cluster;
				mapped_cluster = rtfslfs_cluster_remap(current_cluster, next_cluster);
				/* If we are allocating do not report zero valued clusters in the journal file as free or we could overwrite blocks in an uncommitted delete */
				if (mapped_cluster != next_cluster)
				{
					if (mapped_cluster==RTFSL_JOURNALDELETED_CLUSTER)
					{
						if (writing==RTFSL_ALLOC_CLUSTER) /* If we are allocating force a nonmatch on next_cluster so we skip the cluster */
							next_cluster=rtfsl.current_dr.end_cluster_marker;
						else
							next_cluster=0;
					}
					else
						next_cluster=mapped_cluster;
				}
			}
#endif
#if (RTFSL_INCLUDE_WRITE_SUPPORT)
			if (writing==RTFSL_ALLOC_CLUSTER)
			{
				if (next_cluster==0)
				{
					*pnext_cluster = current_cluster;
					return 1;
				}
				else
				{
					if (current_cluster==rtfsl.current_dr.maxfindex)
					{
						*pnext_cluster = 0;
						return 0;
					}
					current_cluster += 1;
				}
			}
			else
#endif
            if (n_contig < max_length && next_cluster==(current_cluster+1))
			{
				n_contig += 1;
				current_cluster=next_cluster;
			}
			else
			{
				if (next_cluster < 2 || next_cluster >= rtfsl.current_dr.end_cluster_marker)
				{
					*pnext_cluster = 0;

				}
				else
					*pnext_cluster = next_cluster;
				return (n_contig);
		}
		}
#if (RTFSL_INCLUDE_WRITE_SUPPORT)
		if (writing==RTFSL_WRITE_CLUSTER)
		{
			int rval;
			rval=rtfsl_flush_sector_buffer(buffernumber,1);
			if (rval<0)
				return rval;
			if (rtfsl.current_dr.fasize !=3||towrite==0)
				return 0;
		}
#endif
		if (sector >= last_sector)
		{
			*pnext_cluster = 0;
			break;
		}
		else
		{
			sector += 1;
			{
			int rval=rtfsl_read_sector_buffer(rtfsl.current_dr.partition_base+sector);
			    if (rval<0)
			        return rval;
			    sector_buffer = rtfsl_buffer_address(rval);
			}
			byte_offset_in_buffer = 0;
			bytes_remaining = rtfsl.current_dr.bytespsector;
		}
	}
    return n_contig;
}
