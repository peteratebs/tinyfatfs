/*
* EBS - RTFS (Real Time File Manager)
*
* Copyright EBS Inc. 1987-2012
* All rights reserved.
* This code may not be redistributed in source or linkable object form
* without the consent of its author.
*/

#include "rtfslite.h"


struct rtfst_load_structure {
	unsigned char *bootfile_name;
	unsigned long load_count;
	unsigned char *load_address;
};

void BootProgressReport(int percent);
static unsigned long file_load_callback (struct rtfsl_file const *pfile, unsigned long start_sector, unsigned long nbytes, void *puser_data)
{
	struct rtfst_load_structure *pload_structure = (struct rtfst_load_structure *) puser_data;
	unsigned long sector,nbytes_left,bytes_read;
    unsigned char *load_address;
	int rval,bytes_to_skip,percent_reported;
    int n_sectors;
    int n_bytes;
    static int done_so_far=0;;

    load_address = pload_structure->load_address + pfile->file_pointer;
	pload_structure->load_count += nbytes;
    bytes_read=0;
    percent_reported = 0;
#define BYTES_TO_SKIP (320*240*8) /* 2 frame buffers worth */
//#define BYTES_TO_SKIP 0
    if (done_so_far==0)
        bytes_to_skip=BYTES_TO_SKIP;
    else
        bytes_to_skip=0;
    n_sectors=1;
    n_bytes = rtfsl.current_dr.bytespsector;
	for(sector=start_sector,nbytes_left=nbytes;nbytes_left;sector+=n_sectors,load_address+=n_bytes)
	{
    int per_cent = ((nbytes-nbytes_left)*100)/nbytes;
        done_so_far += bytes_read;
        if (per_cent >= (percent_reported+10))
        {
            BootProgressReport(per_cent);
            percent_reported = per_cent;
        }
        if (bytes_read <= bytes_to_skip)
        {
            n_sectors=1;
            n_bytes = rtfsl.current_dr.bytespsector;
       		rval=0;
        }
        else
        {
#define DEFAULT_SIZE_SECTORS 32
            n_sectors = DEFAULT_SIZE_SECTORS;
            n_bytes = n_sectors*rtfsl.current_dr.bytespsector;
            if (n_bytes > nbytes_left)
            {
                n_bytes=nbytes_left;
                n_sectors=(n_bytes+rtfsl.current_dr.bytespsector-1)/rtfsl.current_dr.bytespsector;
                if (!n_sectors)
                  break;
            }
            rval=rtfsl_read_sectors(rtfsl.current_dr.partition_base+sector, n_sectors, load_address);
        }
        bytes_read += n_bytes;

		if (rval<0)
			return rval;
		if (nbytes_left < rtfsl.current_dr.bytespsector)
			nbytes_left = 0;
		else
		    nbytes_left-=n_bytes;
	}
	return nbytes;
}

int rtfsl_load_file(unsigned char *filename, unsigned long load_address)      /*__apifn__*/
{
struct rtfsl_file root_file,current_entry_file;
struct rtfst_load_structure loadstruct;
int rval;
    ANSImemset(&loadstruct,0,sizeof(loadstruct));
    loadstruct.bootfile_name = filename;
    loadstruct.load_address=(unsigned char *)load_address;

	rval=rtfsl_open_path(rtfsl.current_dr.pathnamearray,filename,&root_file, &current_entry_file);
	if (rval== 0)
		rval=rtfsl_enumerate_file(&current_entry_file,file_load_callback, (void *) &loadstruct);
    return rval;
}
