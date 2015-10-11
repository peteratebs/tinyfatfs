/*
* EBS - RTFS (Real Time File Manager)
*
* Copyright EBS Inc. 1987-2012
* All rights reserved.
* This code may not be redistributed in source or linkable object form
* without the consent of its author.
*/

#include "rtfslite.h"
#include "rtfslitetests.h"
#include "stdlib.h"

#define MAXTESTFRAGS 128
#if (RTFSL_INCLUDE_WRITE_SUPPORT)
int rtfsltest_file_fill_drive(unsigned char*filename)
{
int rval;
unsigned char filename_indexed[12];
char index = 'A';
int pos=8;
	strcpy((char*)filename_indexed,(char*)filename);
	for(;;)
	{
		filename_indexed[pos] = index;
		if (index == 'Z')
		{
			pos += 1;
			index = 'A';
		}
		index++;

		rval=rtfsl_create(filename_indexed,0);
		if (rval==0)
		{
		int fd;
		unsigned long i;
			fd = rtfsl_open(filename_indexed);
			if(fd<0)
			{
				rval=fd;
				break;
			}
			else
			{
				for(i=0;i<1024*1024*1024;i+=1024*1024)
				{
					if ((i % 1024*1024*4)==0)
                    {
						PRINTF("Fill : %8.8ld\r", i);
                    }
					rval=rtfsl_write(fd,0,1024*1024);
					if (rval!=1024*1024)
					{
						for(;i<1024*1024*1024;i+=512)
						{
							PRINTF("Fill : %8.8ld\r", i);
							rval=rtfsl_write(fd,0,512);
							if (rval!=512)
							{
								break;
							}
						}
						break;
					}
				}
				rtfsl_flush(fd);
				rtfsl_close(fd);
			}
		}
		if (rval!=512)
			break;
	}
	rtfsl_flush_all_buffers();

    return rval;
}
int rtfsltest_file_sequential_write(unsigned char*filename, int file_fraglen, int gap_fraglen, int numlongs, int test_append)
{
int rval,numfrags;
struct rtfsl_statstructure statstruct;
unsigned long fragrecord[MAXTESTFRAGS];

	numfrags=((numlongs*4)+rtfsl.current_dr.bytespcluster-1)/rtfsl.current_dr.bytespcluster;
	rval=rtfsl_create(filename,0);
    if (rval==0)
	{
	int fd;
	unsigned long i;
		if (gap_fraglen)
		{
			rval=ut_create_fragments(file_fraglen, gap_fraglen, numfrags, fragrecord);
			if (rval<0)
				return rval;
		}
		fd = rtfsl_open(filename);
		if(fd<0)
		    rval=fd;
        else
		    for(i=0;i<(unsigned long)numlongs;i++)
		    {
			    rval=rtfsl_write(fd, (unsigned char*)&i,4);
			    if (rval!= 4)
			    {
				    break;
			    }
			    if (test_append && (i&0x3ff)==0)
                {
					rtfsl_flush(fd);
                    rtfsl_close(fd);
                    fd = rtfsl_open(filename);
                    if (fd<0)
                        rval=fd;
                    else
                        rval=(int)rtfsl_lseek(fd,0,PSEEK_END);
				    if (rval< 0)
				    {
				        break;
				    }
                }
		    }
        if(rval>=0)
        {
		    rtfsl_fstat(fd,&statstruct);
		    if (statstruct.st_size!=(unsigned long)numlongs*4)
		        rval=RTFSL_ERROR_TEST;
            else
                rval=0;
        }
        if(rval>=0)
		    rval=rtfsl_flush(fd);
		rtfsl_close(fd);
		if (gap_fraglen)
		{
			rval=ut_release_fragments(file_fraglen, gap_fraglen, numfrags, fragrecord);
			if (rval<0)
				return rval;
		}
	}
    return rval;
}
#endif
int rtfsltest_file_sequential_read(unsigned char*filename)
{
int fd,rval;
struct rtfsl_statstructure statstruct;


    fd = rtfsl_open((unsigned char*)filename);
	if (fd>=0)
	{
		unsigned long i,j;
		rtfsl_fstat(fd,&statstruct);
		for(i=0;i<(unsigned long)statstruct.st_size/4;i++)
		{
            rval=rtfsl_read(fd, (unsigned char*)&j,4);
			if (rval!= 4)
			{
				PRINTF("Seq: Read Failed at offset == %lu \n",i*4);\
				return rval;
			}
			if (j != i)
            {
				PRINTF("Seq: Compare falied at offset == %lu \n",i);
				return RTFSL_ERROR_TEST;
            }
		}
    }
    return 0;
}
int rtfsltest_file_random_read(unsigned char*filename)
{
struct rtfsl_statstructure statstruct;
int fd;
unsigned long seekoffset;
unsigned long r;

    fd = rtfsl_open(filename);
	if(fd<0)
	    return fd;
	rtfsl_fstat(fd,&statstruct);
	PRINTF("Seek read test. Takes a long time to complete \n");
	for (seekoffset=statstruct.st_size-1;seekoffset>0;seekoffset--)
	{
		rtfsl_lseek(fd,0,PSEEK_SET);
		rtfsl_read(fd,test_buffer,4);
		rtfsl_lseek(fd,(seekoffset/4)*4,PSEEK_SET);
		rtfsl_read(fd,(unsigned char *)&r,4);
        if (r!= seekoffset/4)
        {
			PRINTF("\nreade seek test compare error at unsigned long offset %lu\n",seekoffset/4);
            return RTFSL_ERROR_TEST;
        }
	}
	rtfsl_close(fd);
    return 0;
}
#if (RTFSL_INCLUDE_WRITE_SUPPORT)
int rtfsltest_file_random_write(unsigned char*filename)
{
struct rtfsl_statstructure statstruct;
int fd;
unsigned long seekoffset;
unsigned long l,r;
unsigned char *pl = (unsigned char *)&l;

    fd = rtfsl_open(filename);
	if(fd<0)
	    return fd;
	rtfsl_fstat(fd,&statstruct);
	PRINTF("Seek write test. Takes a long time to complete \n");
	for (seekoffset=statstruct.st_size-1;seekoffset>0;seekoffset--)
	{
		if ((seekoffset & 0xf) == 0)
        {
			PRINTF("seek write test pass %8.8lu\r", seekoffset);
        }
        /* Read from the beginning of file to be sure we are seeking */
		rtfsl_lseek(fd,0,PSEEK_SET);
		rtfsl_read(fd,test_buffer,4);
        /* Write one byte of the current unsigned long flipped */
		rtfsl_lseek(fd,seekoffset,PSEEK_SET);
		l=(seekoffset/4);
		pl = (unsigned char *)&l;
		{
            *(pl+(seekoffset&0x3)) = ~*(pl+(seekoffset&0x3));
			rtfsl_write(fd,pl+(seekoffset&0x3),1);
		}
        /* Read from the beginning of file to be sure we are seeking */
		rtfsl_lseek(fd,0,PSEEK_SET);
		rtfsl_read(fd,test_buffer,4);
        /* Read the  byte and make sure it's what we wrote */
		rtfsl_lseek(fd,seekoffset,PSEEK_SET);
        rtfsl_read(fd,test_buffer,1);
        if (test_buffer[0]!=*(pl+(seekoffset&0x3)))
        {
			PRINTF("\nWrite seek test compare error at byte offset %lu\n",seekoffset);
            return RTFSL_ERROR_TEST;
        }
        /* toggle it again and write it back */
		rtfsl_lseek(fd,seekoffset,PSEEK_SET);
        *(pl+(seekoffset&0x3)) = ~*(pl+(seekoffset&0x3));
		rtfsl_write(fd,pl+(seekoffset&0x3),1);

        /* Read from the beginning of file to be sure we are seeking */
		rtfsl_lseek(fd,0,PSEEK_SET);
		rtfsl_read(fd,test_buffer,4);
		rtfsl_lseek(fd,(seekoffset/4)*4,PSEEK_SET);
		rtfsl_read(fd,(unsigned char *)&r,4);
        if (r!= seekoffset/4)
        {
			PRINTF("\nWrite seek test compare error at unsigned long offset %lu\n",seekoffset/4);
            return RTFSL_ERROR_TEST;
        }
	}
	rtfsl_flush(fd);
	rtfsl_close(fd);
    return 0;
}
#endif

#if (0)

#if (DOBOOTFILEEST)
	{
	 int fd;
     void * load_address = malloc(1000000);
	    rtfsl_load_file(&tdrive,(unsigned char *)BOOTFILENAME, (unsigned long) load_address);
 #if (DOFILIOREADAPITEST)
		fd=rtfsl_open((byte*)BOOTFILENAME);
		if (fd>=0)
		{
			int nread;
			unsigned long total=0;
			unsigned char *loadimage=(unsigned char *)load_address;
			rtfsl_fstat(fd,&statstruct);
			do
			{
				nread=rtfsl_read(fd,test_buffer,tdrive.bytespsector);
				if (nread>0)
				{
					total+=nread;
					if (ANSImemcmp(test_buffer, loadimage, nread)!=0)
                    {
						PRINTF("Compare failed\n");
                    }
					loadimage += nread;
				}
			} while (nread > 0);
			if (statstruct.st_size!=total)
			{
				PRINTF("filesize == %lu total read == %lu\n",statstruct.st_size,total);
			}
			for (seekoffset=1;seekoffset<7;seekoffset++)
			{
				for (seekpointer=0; seekpointer<statstruct.st_size;seekpointer += seekoffset)
				{
					loadimage=(unsigned char *)load_address;
					if ((seekpointer & 0x3ff) == 0)
                    {
						PRINTF("seek test pass %lu pointer: %8.8lu\r", seekoffset, seekpointer);
                    }
					rtfsl_lseek(fd,0,PSEEK_SET);
					rtfsl_lseek(fd,seekpointer,PSEEK_SET);
					rtfsl_read(fd,test_buffer,1);
					if (test_buffer[0] != *(loadimage+seekpointer))
                    {
						PRINTF("\nSeek test compare error at %lu\n",seekpointer);
                    }
				}
			}
			PRINTF("load-sequential read-seek-read test completed %lu bytes read and verified\n",statstruct.st_size);
		}
#endif // #if (DOFILIOREADAPITEST)
	    free(load_address);
    }
#endif
#if (DOFAILSAFEFLUSHTEST&&RTFSL_INCLUDE_FAILSAFE_SUPPORT)
	rtfslfs_flush();
#endif
}
#endif
