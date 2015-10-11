/*
* EBS - RTFS (Real Time File Manager)
*
* Copyright EBS Inc. 1987-2012
* All rights reserved.
* This code may not be redistributed in source or linkable object form
* without the consent of its author.
*/

#include "rtfslite.h"
#include <stdio.h>
#if (RTFSL_INCLUDE_FAILSAFE_SUPPORT)

/* Frame structure
1xxx  :28:32  = 64   CLUSTER  - OPCODE START LENGTH
031   :32     = 64   DOSINODE - OPCODE SECTOR INDEX|TABLEINDEX|DELETED
*/

#define DIRENT_RECORD           0x80000000
#define DIRENT_SECTOR_MASK      0x7fffffff
#define CLUSTER_DELETE_RECORD   0x30000000
#define CLUSTER_CHAIN_RECORD    0x20000000
#define CLUSTER_TCHAIN_RECORD   0x10000000
#define CLUSTER_INSTANCE_RECORD 0x00000000
#define CLUSTER_RECORD_MASK     0x30000000
#define CLUSTER_VALUE_MASK      0x0fffffff

#define REPLACEMENTRECORDSTART 1

#define DELETEEDINODEMARKER 0xfffe

#define RELATIONSHIP_NONE               0
#define RELATIONSHIP_OVERLAP            1
#define RELATIONSHIP_ADJACENT_LEFT      2
#define RELATIONSHIP_ADJACENT_RIGHT     3
int rtfslfs_cluster_map(unsigned long cluster_number,unsigned long value)
{
int i;
int get_next_record;
unsigned long cluster_type;
char emit_new_record;

    struct rtfsl_failsafe_context *pfs=rtfsl.rtfsl_current_failsafe_context;
    if (!pfs)
        return 1;
    /* We may need up to 3 replacement records, fail if we don't have them */
    if (pfs->journal_buffer_free < (3*REPLACEMENT_RECORD_SIZE_BYTES))
        return RTFSL_JOURNALFULL_CLUSTER;

    /* set up default instructions in case we don't overlap a region */
    emit_new_record=1;
    if (value==0)
    {
       cluster_type = CLUSTER_DELETE_RECORD;
       value = cluster_number;
    }
    else if (value == (rtfsl.current_dr.end_cluster_marker|0xf))
    {
        cluster_type = CLUSTER_TCHAIN_RECORD;
        value = cluster_number;
    }
    else if (cluster_number+1 == value)
    {
        cluster_type = CLUSTER_CHAIN_RECORD;
        value = cluster_number;
    }
    else
    {
        cluster_type = CLUSTER_INSTANCE_RECORD;
    }
	//
	// 
	//REHERE - Add a new state to discard a cluster from all records if it is used.
    get_next_record=1;
    for (i = 0; get_next_record && i < (int)*pfs->preplacement_record_count;i++)
    {
        unsigned long end_record_cluster,start_record_cluster,record_type;
        int relationship;
        char overwrite_current_record,split_current_record,change_start_value,change_end_value,change_record_type;
        if ((pfs->preplacement_records[i][0]&DIRENT_RECORD)!=0)     /* Skip non cluster remap records */
            continue;

        start_record_cluster=pfs->preplacement_records[i][0]&CLUSTER_VALUE_MASK;
        record_type=pfs->preplacement_records[i][0]&CLUSTER_RECORD_MASK;
		if (record_type==CLUSTER_INSTANCE_RECORD)
			end_record_cluster=start_record_cluster;
		else
			end_record_cluster=pfs->preplacement_records[i][1];

		overwrite_current_record=split_current_record=change_start_value=change_end_value=change_record_type=0;
		if (cluster_number+1 == start_record_cluster)
           relationship=RELATIONSHIP_ADJACENT_LEFT;
		else if (cluster_number-1 == end_record_cluster)
	       relationship=RELATIONSHIP_ADJACENT_RIGHT;
		else if (start_record_cluster<=cluster_number&&end_record_cluster>=cluster_number)
	        relationship=RELATIONSHIP_OVERLAP;
		else
	        relationship=RELATIONSHIP_NONE;
		switch (relationship)
		{
		    case RELATIONSHIP_NONE:
		    default:
		    break;
		    case RELATIONSHIP_ADJACENT_LEFT:	/* Cluster is 1 to the left of the region */
		        switch (record_type)
		        {
                case CLUSTER_TCHAIN_RECORD:		/* Cluster is 1 to the left of a terminated chain record */
                    switch (cluster_type)
                    {
                        case CLUSTER_TCHAIN_RECORD:
                        case CLUSTER_INSTANCE_RECORD:
                        case CLUSTER_DELETE_RECORD:
                        default:
 						    break;
                        case CLUSTER_CHAIN_RECORD:
                            change_start_value=1;   /* Chain cluster immed to left of a terminated chain, set new start of terminated chain to cluster number. */;
                            break;
                    }
                    break;
                case CLUSTER_INSTANCE_RECORD:		/* Instance record is 1 to the left of a link record, no connection */
 				    break;
                case CLUSTER_DELETE_RECORD:			/* Cluster is 1 to the left of an erase record */
                {
                    switch (cluster_type)
                    {
                        case CLUSTER_TCHAIN_RECORD:
                        case CLUSTER_INSTANCE_RECORD:
                        case CLUSTER_CHAIN_RECORD:
                        default:
  						    break;
                        case CLUSTER_DELETE_RECORD:
                            change_start_value=1; /* delete cluster immed to left of a deleted chain, set new start of deleted chain to cluster number. */;
                            break;
                    }
                }
                break;
                case CLUSTER_CHAIN_RECORD:			/* Cluster is 1 to the left of a chain record */
                {
                    switch (cluster_type)
                    {
                        case CLUSTER_TCHAIN_RECORD:
                        case CLUSTER_INSTANCE_RECORD:
                        case CLUSTER_DELETE_RECORD:
                        default:
  						    break;
                        case CLUSTER_CHAIN_RECORD:
                            change_start_value=1;  /* Chain cluster immed to left of a chain, set new start of chain to cluster number. */;
                            break;
                    }
                }
                break;
                default:
                break;
		    }
		    break;
        case RELATIONSHIP_ADJACENT_RIGHT:		/* Cluster is 1 to the right of the region */
            switch (record_type)
            {
                case CLUSTER_TCHAIN_RECORD:			        /* Cluster is 1 to the right of a terminator record, no connection */
                case CLUSTER_INSTANCE_RECORD:			    /* Cluster is 1 to the right of a link record, no connection */
                default:
               	    break;
                case CLUSTER_DELETE_RECORD:					/* Cluster is 1 to the right of an erase record */
                {
                    switch (cluster_type)
                    {
                        case CLUSTER_TCHAIN_RECORD:
                        case CLUSTER_INSTANCE_RECORD:
                        case CLUSTER_CHAIN_RECORD:
  						    break;
                        case CLUSTER_DELETE_RECORD:
						    change_end_value=1;             /* delete cluster immed to right of a deleted region, set new end of deleted chain to cluster number. */;
						    break;
                    }
                }
                break;
                case CLUSTER_CHAIN_RECORD:			    /* Cluster is 1 to the right of the chain region */
                {
                    switch (cluster_type)
                    {
                        case CLUSTER_INSTANCE_RECORD:
                        case CLUSTER_DELETE_RECORD:
                        default:
                        break;
                        case CLUSTER_TCHAIN_RECORD:
						    change_record_type=1;              /* Append a tchain to a chain so change type, fall through to change end  */
                        case CLUSTER_CHAIN_RECORD:
						    change_end_value=1;         /* Chain cluster immed to right of a chain, set new end of chain to cluster number. */;
                        break;
                    }
                }
                break;
            }
                break;
        case RELATIONSHIP_OVERLAP:			                    /* Cluster overlaps the region */
             switch (record_type)
             {
                 case CLUSTER_TCHAIN_RECORD:
                 {
                     switch (cluster_type)
                     {
                         case CLUSTER_TCHAIN_RECORD:            /* terminate cluster overlaps a terminated chain, split the record into 2 terminated chains and don't emit a new record */
                         {
     						if (cluster_number==end_record_cluster)
                            {
     						    /* Writing a terminator at the current end of a terminated chain is a no-op */
     							get_next_record=0;
     							emit_new_record=0;
                            }
                            else
                            {
     							split_current_record=1;         /* terminate cluster overlaps a terminated chain, split into two terminated chains no need to emit a new record */
     							emit_new_record=0;
                            }
                         }
                         break;
                        case CLUSTER_INSTANCE_RECORD:		    /* link cluster overlaps a terminated chain */
     					case CLUSTER_DELETE_RECORD:		        /* erase cluster overlaps a terminated chain */
     					{
     						split_current_record=1;             /* Split current record in two and emit a new record where the hole is */
     					}
     					    break;
     					case CLUSTER_CHAIN_RECORD:		        /* chain cluster overlaps a terminated chain */
                        {
     						if (cluster_number==end_record_cluster)
     						{
     							change_record_type=1;           /* End of a terminated chain record is now a chain, change to a chain record and return */;
     						}
                            else
                            {
     						    /* A chain cluster overlapping a tchain in the middle is a no-op */
     							get_next_record=0;
     							emit_new_record=0;
                            }
                            break;
                        default:
     					    break;
                     }
                     break;
                 }
                 case CLUSTER_INSTANCE_RECORD:	                /* The record is a one cluster instance record and they overlap, force an overwrite of the record with the current cluster */
                 {
        			 overwrite_current_record=1;
                     break;
                 }
                 case CLUSTER_DELETE_RECORD:	                /* The record is an erase and they overlap */
                 {
                     switch (cluster_type)
                     {
                        case CLUSTER_TCHAIN_RECORD:
                        case CLUSTER_CHAIN_RECORD:
                        case CLUSTER_INSTANCE_RECORD:
     					{
     						split_current_record=1;
     					}
     					break;
                        case CLUSTER_DELETE_RECORD:
   							get_next_record=0;
                            emit_new_record=0;
                            get_next_record=0;
     						/* Already erased.. is a no-op */
                        break;
                        default:
                        break;
                     }
                 }
                     break;
                 case CLUSTER_CHAIN_RECORD:                     /* The record i a chain and they overlap */
                 {
                     switch (cluster_type)
                     {
                         case CLUSTER_TCHAIN_RECORD:
                         {
     						if (cluster_number==end_record_cluster)
     						{
     							change_record_type=1;
     						}
     						else
     						{
     							split_current_record=1;
     						}
                         }
                         break;
                         case CLUSTER_DELETE_RECORD:
                         case CLUSTER_INSTANCE_RECORD:
                         {
     						split_current_record=1;
                         }
                         break;
                         case CLUSTER_CHAIN_RECORD:
   							emit_new_record=0;
     						/* A chain overlapping a chain is a no-op */
     						get_next_record=0;
                         break;
                         default:
                         break;
                     }
                     break;
                 }
                 default:
                    break;
               }
                    break;
			}
		}
     	if (change_record_type)
        {
            pfs->preplacement_records[i][0]=cluster_type|(pfs->preplacement_records[i][0]&CLUSTER_VALUE_MASK);
			emit_new_record=0;
            get_next_record=0;
        }
        if (change_start_value)
        {
            pfs->preplacement_records[i][0]=cluster_number|(pfs->preplacement_records[i][0]&CLUSTER_RECORD_MASK);
			emit_new_record=0;
            get_next_record=0;
        }
        if (change_end_value)
        {
		    pfs->preplacement_records[i][1]=cluster_number;
			emit_new_record=0;
            get_next_record=0;
        }
        /* Overwrite the current record if instructed or if we are inserting a new record between
           the start and end of a record that is only one cluster long */
        if (overwrite_current_record||(split_current_record&&(end_record_cluster==start_record_cluster)))
        {
            pfs->preplacement_records[i][0]=start_record_cluster|cluster_type;
            pfs->preplacement_records[i][1]=value;
            split_current_record=0;
            emit_new_record=0;
            get_next_record=0;
        }
        /* We are splitting a record in two. If emit_new_record is non zero we need to remove one cluster from the range that
           we are splitting. If emit_new_record is non zero we know that the start and end are not the same. */
        if (split_current_record)
        {
            get_next_record=0;
            /* Copy the record we are going to split into the next free record slot, don't reserve it yet */
            *pfs->preplacement_records[*pfs->preplacement_record_count]=*pfs->preplacement_records[i];

            if (emit_new_record)
            {
                if (cluster_number==start_record_cluster)
                { /* We're overwriting the first cluster in a range, don't split it just move the start, let emit create a new record */
                    pfs->preplacement_records[i][0]=(start_record_cluster+1)|(pfs->preplacement_records[i][0]&CLUSTER_RECORD_MASK);
                }
                else
                {  /* We're overwriting a cluster in the range, it isn't the first cluster and the range is > 1, split the range */
                    /* Change the end of the original record */
                    pfs->preplacement_records[i][1]=cluster_number-1;
                    /* If the original record was a tchain make it a chain */
                    if (record_type==CLUSTER_TCHAIN_RECORD)
					{
                        pfs->preplacement_records[i][0]=CLUSTER_CHAIN_RECORD|(pfs->preplacement_records[i][0]&CLUSTER_VALUE_MASK);
					}
                    /* If we are at the end, don't split, the emit further down will creat the new record */
                    if (cluster_number==end_record_cluster)
                    {
                        ;
                    }
                    else
                    {   /* Consume the cloned record, for the fragment of the region after the cluster we carved out. Just change the start point */
                        pfs->preplacement_records[*pfs->preplacement_record_count][0]=(start_record_cluster+1)|(pfs->preplacement_records[*pfs->preplacement_record_count][0]&CLUSTER_RECORD_MASK);
						*pfs->preplacement_record_count= *pfs->preplacement_record_count + 1;
                        pfs->journal_buffer_free -= REPLACEMENT_RECORD_SIZE_BYTES;
                    }
                }
            }
            else /* We splitting a record but not taking a cluster away */
            {
                if (cluster_number==start_record_cluster)
                {   /* We're splitting at the start of a range, terminate the old record at the start */
                    pfs->preplacement_records[i][1]=cluster_number;
                    /* Consume the cloned record, for the fragment of the region after the cluster we carved out. Just change the start point */
                    pfs->preplacement_records[*pfs->preplacement_record_count][0]=(cluster_number+1)|(pfs->preplacement_records[*pfs->preplacement_record_count][0]&CLUSTER_RECORD_MASK);
                }
                else
                {  /* We're splitting but not at the start of a range, terminate the old record at the cluster - 1 */
                    pfs->preplacement_records[i][1]=cluster_number-1;
                    pfs->preplacement_records[*pfs->preplacement_record_count][0]=(cluster_number)|(pfs->preplacement_records[*pfs->preplacement_record_count][0]&CLUSTER_RECORD_MASK);
                }
                /* If the original record was a tchain make it a chain */
                if (record_type==CLUSTER_TCHAIN_RECORD)
				{
                    pfs->preplacement_records[i][0]=CLUSTER_CHAIN_RECORD|(pfs->preplacement_records[i][0]&CLUSTER_VALUE_MASK);
				}
                *pfs->preplacement_record_count= *pfs->preplacement_record_count + 1;
                pfs->journal_buffer_free -= REPLACEMENT_RECORD_SIZE_BYTES;
            }
        }
     }  /* End for loop */
     if (emit_new_record)
     { /* or one past if we are making a hole */
        pfs->preplacement_records[*pfs->preplacement_record_count][0]= cluster_number|cluster_type;
        pfs->preplacement_records[*pfs->preplacement_record_count][1]= value;
        *pfs->preplacement_record_count= *pfs->preplacement_record_count + 1;
        pfs->journal_buffer_free -= REPLACEMENT_RECORD_SIZE_BYTES;
     }
     return 0;
}

unsigned long rtfslfs_cluster_remap(unsigned long cluster,unsigned long value)
{
    int i;
    unsigned long new_instance_cluster,new_delete_start,new_chain_start,new_instance_value,new_chain_end,new_delete_end;
    struct rtfsl_failsafe_context *pfs=rtfsl.rtfsl_current_failsafe_context;
    if (!pfs)
        return value;
	new_delete_start=new_chain_start=new_instance_cluster=new_delete_end=new_chain_end=new_instance_value=0;


    for (i = 0; i < (int)*pfs->preplacement_record_count;i++)
    {
    unsigned long cluster_record_type;
    unsigned long end_record_cluster,start_record_cluster;

        if ((pfs->preplacement_records[i][0]&DIRENT_RECORD)!=0)     /* Skip non cluster remap records */
            continue;
        start_record_cluster=pfs->preplacement_records[i][0]&CLUSTER_VALUE_MASK;
        cluster_record_type=pfs->preplacement_records[i][0]&CLUSTER_RECORD_MASK;
		if (cluster_record_type==CLUSTER_INSTANCE_RECORD)
			end_record_cluster=start_record_cluster;
		else
			end_record_cluster=pfs->preplacement_records[i][1];
        if (cluster<start_record_cluster)   /* No relationship */
            continue;
        if (cluster_record_type==CLUSTER_INSTANCE_RECORD)
        {
            if (start_record_cluster==cluster)
                return pfs->preplacement_records[i][1];
        }
        else
        {   /* It a delete, chain or tchain */
            if (start_record_cluster<=cluster&&end_record_cluster>=cluster)
            {
                if (cluster_record_type==CLUSTER_DELETE_RECORD)
                    return RTFSL_JOURNALDELETED_CLUSTER;
                /* It's either a tchain or a chain */
                /* last cluster of a tchain is chain terminator */
                if (cluster_record_type==CLUSTER_TCHAIN_RECORD && end_record_cluster==cluster)
                    return rtfsl.current_dr.end_cluster_marker|0xf;
				else
					return cluster+1;
            }
        }
    }
    /* Fall through and return the unmapped value */
    return value;
}


int rtfslfs_dirent_remap(unsigned long sector,unsigned long index, struct rtfsl_dosinode *p_dos_inode,int reading)
{
	struct rtfsl_failsafe_context *pfs=rtfsl.rtfsl_current_failsafe_context;
    int replacement_record_index=-1;
	int remap_index=0;
    if (pfs)
    {
    int i;
        for (i=0; i < (int)*pfs->preplacement_record_count;i++)
        {
            if ((pfs->preplacement_records[i][0]&DIRENT_RECORD)==0)     /* Skip non dirent remap records */
                continue;
            if ((pfs->preplacement_records[i][0]&DIRENT_SECTOR_MASK)==sector)
            {
                if ((pfs->preplacement_records[i][1]&0x0000ffff)==index)
                {
                unsigned long remap_index =  pfs->preplacement_records[i][1]>>16;
                    if (reading)
                    {
                        if (remap_index == DELETEEDINODEMARKER)
                            p_dos_inode->fname[0]=PCDELETE;
                        else
                            ANSImemcpy(p_dos_inode, pfs->journal_buffer+remap_index,REPLACEMENT_DOSINODESIZE_SIZE_BYTES);
                        return 1;
                    }
                    else
                    {
                        if (p_dos_inode->fname[0]==PCDELETE)
                        {   /* Mark the dosinode deleted. we can't reclaim the dossinode space in the buffer but it will be ignored */
                            pfs->preplacement_records[i][1]= (unsigned long)(DELETEEDINODEMARKER)<<16|(unsigned long)index;
                            return 1;
                        }
                        else
                        {
                            if (remap_index != DELETEEDINODEMARKER)
                            {
                                ANSImemcpy(pfs->journal_buffer+remap_index, p_dos_inode,REPLACEMENT_DOSINODESIZE_SIZE_BYTES);
                                return 1;
                            }
                            /* We found a record for the dossinode but its flagged deleted and we need to copy record in place */
                            replacement_record_index=i;
                            break;
                        }
                    }
                }
            }
        }
		if (reading)
			return 0;
        /* If we get here we have to allocate a records */
		{
        int bytes_needed=0;
        if (p_dos_inode->fname[0]!=PCDELETE)
            bytes_needed+=REPLACEMENT_DOSINODESIZE_SIZE_BYTES;
        if (replacement_record_index<0)
            bytes_needed+=REPLACEMENT_RECORD_SIZE_BYTES;
        if (pfs->journal_buffer_free < bytes_needed)
            return RTFSL_ERROR_JOURNAL_FULL;
		}
        if (p_dos_inode->fname[0]==PCDELETE)
            remap_index=DELETEEDINODEMARKER;
        else
        {
            *pfs->pcurrent_dosinode_offset-=REPLACEMENT_DOSINODESIZE_SIZE_BYTES;
            pfs->journal_buffer_free-=REPLACEMENT_DOSINODESIZE_SIZE_BYTES;
            remap_index=(int)*pfs->pcurrent_dosinode_offset;
            ANSImemcpy(pfs->journal_buffer+remap_index,p_dos_inode,REPLACEMENT_DOSINODESIZE_SIZE_BYTES);
        }
        if (replacement_record_index<0)
        {
            replacement_record_index=*pfs->preplacement_record_count;
			*pfs->preplacement_record_count= *pfs->preplacement_record_count + 1;
        }
        pfs->preplacement_records[replacement_record_index][0]=sector|DIRENT_RECORD;
        pfs->preplacement_records[replacement_record_index][1]=remap_index<<16|index;
		return 1;
    }
    return 0;
}
/* Format of the buffer: replacement_record_count|replacement_records|->growsup              Grows down<-dosinode1|dosinode0|]*/
int rtfslfs_start(void)  /*__apifn__*/
{
int rval;
struct rtfsl_failsafe_context *pfs;

	pfs=rtfsl.rtfsl_current_failsafe_context=&rtfsl.rtfsl_failsafe_context;
	ANSImemset(rtfsl.rtfsl_current_failsafe_context,0,sizeof(*rtfsl.rtfsl_current_failsafe_context));
	rtfsl.rtfsl_current_failsafe_context->journal_buffer=rtfsl.rtfslfs_sector_buffer;
	rtfsl.rtfsl_current_failsafe_context->journal_buffer_size=RTFSL_CFG_FSBUFFERSIZEBYTES;
	rval=rtfslfs_access_journal(RTFSLFS_JTEST);
    if (rval==0)
    {
        pfs->preplacement_record_count=(unsigned long *) pfs->journal_buffer;
        pfs->pcurrent_dosinode_offset=(unsigned long *) (pfs->journal_buffer+4);
        pfs->preplacement_records = (treplacement_record *) (pfs->journal_buffer+8);
        *pfs->pcurrent_dosinode_offset=pfs->journal_buffer_size;
        *pfs->preplacement_record_count=0;
        pfs->journal_buffer_free = pfs->journal_buffer_size-8;
/*
        For reload..
            pfs->journal_buffer_free = (pfs->journal_buffer_size-8-( (*pfs->preplacement_record_count*8)+ pfs->journal_buffer_size-*pfs->pcurrent_dosinode_offset) );
*/
    }
	return rval;
}

int rtfslfs_flush(void)    /*__apifn__*/
{
int rval;
	rval=rtfsl_flush_info_sec();
	if (rval==0)
		rval=rtfslfs_access_journal(RTFSLFS_JWRITE);
	return rval;
}

static int _rtfslfs_sync(void);
int rtfslfs_sync(void)              /*__apifn__*/
{
int rval;
	rval=_rtfslfs_sync();
	if (rval==0)
		rval=rtfslfs_start();
	return rval;
}

int rtfslfs_restore(void)         /*__apifn__*/
{
int rval=0;
    rval=rtfslfs_start();		/* Initialize offset, pointers etc. */
    if (rval==0)
    {
        rval=rtfslfs_access_journal(RTFSLFS_JREAD); /* read the journal */
        if (rval==0)
			rval=_rtfslfs_sync();
	}
	if (rval==0)
		rval=rtfsl_diskopen();
	return rval;
}

static int _rtfslfs_sync(void)
{
int rval=0;
struct rtfsl_failsafe_context *pfs=rtfsl.rtfsl_current_failsafe_context;
    if (pfs)
    {
        unsigned char *b=0;
        struct rtfsl_dosinode *p_dos_inode;
		unsigned long sector,offset,index;
        int replacement_record_index;
        int buffernumber;
        /* null out the failsafe context pointer so the cluster routines go to the volume, not the journal */
        rtfsl.rtfsl_current_failsafe_context=0;

        for (replacement_record_index=0;replacement_record_index<(int)*pfs->preplacement_record_count;replacement_record_index++)
        {
            if (pfs->preplacement_records[replacement_record_index][0]&DIRENT_RECORD)
                sector=pfs->preplacement_records[replacement_record_index][0]&DIRENT_SECTOR_MASK;
            else
                sector=0;
            if (pfs->preplacement_records[replacement_record_index][0]&DIRENT_RECORD)
            {
                index = pfs->preplacement_records[replacement_record_index][1]&0x0000ffff;
                offset = pfs->preplacement_records[replacement_record_index][1]>>16;
                buffernumber=rtfsl_read_sector_buffer(sector);
                if (buffernumber<0)
					return buffernumber;
                b = rtfsl_buffer_address(buffernumber);
                p_dos_inode = (struct rtfsl_dosinode *) b;
                p_dos_inode += index;
                if (offset == DELETEEDINODEMARKER && p_dos_inode->fname[0]!=PCDELETE)
                {
                    p_dos_inode->fname[0]=PCDELETE;
                    rtfsl_mark_sector_buffer(buffernumber); /* Mark the buffer dirty */
                }
#if (RTFSL_INCLUDE_FAT32)
	            else if (sector==rtfsl.current_dr.infosec)
                {
                    unsigned long *pl= (unsigned long *)p_dos_inode;
					pl++;
					rtfsl.current_dr.free_alloc=*pl++;
                    rtfsl.current_dr.next_alloc=*pl;
					rtfsl.current_dr.flags|=RTFSL_FAT_CHANGED_FLAG;
                    rval=rtfsl_flush_info_sec();
                    if (rval<0)
                        return rval;
                }
#endif
                else
                {
                    if (ANSImemcmp(p_dos_inode,pfs->journal_buffer+offset,REPLACEMENT_DOSINODESIZE_SIZE_BYTES)!=0)
                    {
                        ANSImemcpy(p_dos_inode,pfs->journal_buffer+offset,REPLACEMENT_DOSINODESIZE_SIZE_BYTES);
                        rtfsl_mark_sector_buffer(buffernumber); /* Mark the buffer dirty */
                    }
                }
            }
            else
            {
                unsigned long cluster_record_type,end_record_cluster,current_cluster,value;
                current_cluster=pfs->preplacement_records[replacement_record_index][0]&CLUSTER_VALUE_MASK;
                cluster_record_type=pfs->preplacement_records[replacement_record_index][0]&CLUSTER_RECORD_MASK;
                end_record_cluster=pfs->preplacement_records[replacement_record_index][1];
                if (cluster_record_type==CLUSTER_DELETE_RECORD)
                {
                    value=0;
                    do {
                        rval=fatop_buff_get_frag(current_cluster|RTFSL_WRITE_CLUSTER, &value, 1);
                        } while(rval==0 && current_cluster++<end_record_cluster);
                }
                else if (cluster_record_type==CLUSTER_INSTANCE_RECORD)
                {
                    value=end_record_cluster;
                    rval=fatop_buff_get_frag(current_cluster|RTFSL_WRITE_CLUSTER, &value, 1);
                }
                else /* if (cluster_record_type==CLUSTER_CHAIN_RECORD)||cluster_record_type==CLUSTER_TCHAIN_RECORD */
                {
                    value=current_cluster+1;
                    while (rval==0 && current_cluster!=end_record_cluster)
                    {
                      rval=fatop_buff_get_frag(current_cluster|RTFSL_WRITE_CLUSTER, &value, 1);
                      value+=1;
					  current_cluster+=1;
                    };
                    if (cluster_record_type==CLUSTER_TCHAIN_RECORD)
                        value = rtfsl.current_dr.end_cluster_marker|0xf;
                    if (rval==0)
                        rval=fatop_buff_get_frag(current_cluster|RTFSL_WRITE_CLUSTER, &value, 1);
                 }
            }
        }
        if (rval==0)
            rval=rtfsl_flush_all_buffers();
		if (rval==0 && *pfs->preplacement_record_count)
		{
			rtfsl.rtfsl_current_failsafe_context=pfs;
			rtfslfs_access_journal(RTFSLFS_JCLEAR);
			rtfsl.rtfsl_current_failsafe_context=0;
		}
    }
    return rval;
}
#endif
