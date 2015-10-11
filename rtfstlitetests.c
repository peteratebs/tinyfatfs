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


/* Callback instructions */
#define ROOT_SCAN_TEST_PRINT_FILE_NAMES 1
#define ROOT_SCAN_TEST_BOOTFILETEST 2

#define BOOTFILENAME    "BOOTFILEBIN"
#define RENAMEDFILENAME "RENAMEDFILE"
extern int writing;
static unsigned char *path[4] = {(unsigned char*)"RTFSL      ",0,0,0};
/*
Failsafe test scenarios
// Create a directory in the journal but not on the medium rtfs reports no directory exists
#define DOFAILSAFEOPENTEST  1
#define DOFAILSAFERESTORETEST 0
#define DOMKDIRTEST 1
#define DORMDIRTEST 0
#define DOFAILSAFEFLUSHTEST 0
// Restore from the journal. Rtfs reports the directory exists
#define DOFAILSAFEOPENTEST  1
#define DOFAILSAFERESTORETEST 1
#define DOMKDIRTEST 0
#define DORMDIRTEST 0
#define DOFAILSAFEFLUSHTEST 0
// Remove the director and flush the journal. Rtfs still reports the directory exists
#define DOFAILSAFEOPENTEST  1
#define DOFAILSAFERESTORETEST 1
#define DOMKDIRTEST 0
#define DORMDIRTEST 0
#define DOFAILSAFEFLUSHTEST 1
// Restore from the journal. Rtfs reports the directory does not
#define DOFAILSAFEOPENTEST  1
#define DOFAILSAFERESTORETEST 1
#define DOMKDIRTEST 0
#define DORMDIRTEST 0
#define DOFAILSAFEFLUSHTEST 0

// Write to the journal file and perform the file read and write tests. There is no record of the fiole in RTFS
#define DOFAILSAFEOPENTEST  1
#define DOFAILSAFERESTORETEST 0
#define DOMKDIRTEST 0
#define DORMDIRTEST     0
#define DOFILEWRITETEST 1
#define DOBOOTFILEEST   1
#define DOFILIOREADAPITEST 1
#define DOFAILSAFEFLUSHTEST 1

// Restore from the journal file RTFS sees the file
#define DOFAILSAFEOPENTEST  1
#define DOFAILSAFERESTORETEST 1

// Delete the to the journal file. The file is still seen in Rtfs
#define DOFAILSAFEOPENTEST  1
#define DOFAILSAFERESTORETEST 0
#define DOMKDIRTEST 0
#define DORMDIRTEST 0
#define DOFILEWRITETEST 0
#define DOFILEDELETETEST 1
#define DOBOOTFILEEST   0
#define DOFILIOREADAPITEST 0
#define DOFAILSAFEFLUSHTEST 1


// Restore from the journal file RTFS no longer sees the file
#define DOFAILSAFEOPENTEST  1
#define DOFAILSAFERESTORETEST 1

*/

/* Compile tiome options for test selection */
#define DOFATSCANTEST 0
#define DOROOTSCANTEST 1
#define DOROOTGFIRSTSCANTEST 1

#define DOFAILSAFEFLUSHTEST 0
#define DOFAILSAFEOPENTEST  1
#define DOFAILSAFESYNCTEST	1
#define DOFAILSAFERESTORETEST 1

#define DOMKDIRTEST 1
#define DORMDIRTEST 0

#define DIRECTORY_FRAGMENT_LENGTH 1     /* One for every other cluster fragmentation but may be larger */
#define DIRECTORY_GAP_FRAGMENT_LENGTH 0 /* Zero for no fragments, or the gap between file fragments */


#define WRITETESTNLONGS 10240
#define DOFILEWRITETEST 1
#define DOSEQUENTIALREADTEST 1
#define DOFILERANDOMWRITETEST 0
#define FILE_FRAGMENT_LENGTH 1     /* One for every other cluster fragmentation but may be larger */
#define FILE_GAP_FRAGMENT_LENGTH 0 /* Zero for no fragments, or the gap between file fragments */
#define DOAPPENDTEST 1     /* If 1 the sequential write test closes and seeks to the end every 1000th write */
#define DORENAMETEST 1
#define DOFILEDELETETEST 0

#if (DORMDIRTEST&&DOFILEDELETETEST)
#error Tests are mutually exclusive
#endif
#define DOBOOTFILEEST   0
#define DOFILIOREADAPITEST 0
#define DOFILERANDOMREADTEST 0


#define DOSETPATHTEST 1
#define DOMULTICHAINSUBDIRECTORYTEST 0 /* Requires DOSETPATH */
#if (RTFSL_INCLUDE_LOAD_ONLY)

#undef DOFATSCANTEST
#undef DOROOTSCANTEST
#undef DOROOTGFIRSTSCANTEST
#undef DOFAILSAFEFLUSHTEST
#undef DOFAILSAFEOPENTEST
#undef DOFAILSAFESYNCTEST
#undef DOFAILSAFERESTORETEST

#define DOFATSCANTEST 0
#define DOROOTSCANTEST 0
#define DOROOTGFIRSTSCANTEST 0
#define DOFAILSAFEFLUSHTEST 0
#define DOFAILSAFEOPENTEST  0
#define DOFAILSAFESYNCTEST	0
#define DOFAILSAFERESTORETEST 0

#undef DOMKDIRTEST
#undef DORMDIRTEST
#define DOMKDIRTEST 0
#define DORMDIRTEST 0


#undef DOFILEWRITETEST
#undef DOFILERANDOMWRITETEST
#undef DOAPPENDTEST
#undef DORENAMETEST
#undef DOFILEDELETETEST

#undef DOSEQUENTIALREADTEST
#define DOSEQUENTIALREADTEST 0
#define DOFILEWRITETEST 0
#define DOFILERANDOMWRITETEST 0
#define DOAPPENDTEST 0
#define DORENAMETEST 0
#define DOFILEDELETETEST 0


#undef DOBOOTFILEEST
#undef DOFILIOREADAPITEST
#undef DOFILERANDOMREADTEST

#define DOBOOTFILEEST   1
#define DOFILIOREADAPITEST 0
#define DOFILERANDOMREADTEST 0

#undef DOSETPATHTEST
#define DOSETPATHTEST 0
#undef DOMULTICHAINSUBDIRECTORYTEST
#define DOMULTICHAINSUBDIRECTORYTEST 0
#endif

//HEREHERE
//.. Test putting next index and bytes free into failsafe buffer so it re-loads
//.. Create failsafe load and failsafe sync functions
//.. Failsafe representation of info sector update
//.. Put checksum of all journalled entities into failsafe file for catching changes outside.
//.. Update of multiple fat copies
//.. Update of FAT32 info sector fields.
//.. done test directory extend
//.. test directory extend with fragments
//.. done test file append (seek to end and write)
//..

unsigned char test_buffer[RTFSL_CFG_MAXBLOCKSIZE];

#if (DOFAILSAFETEST&&RTFSL_INCLUDE_FAILSAFE_SUPPORT)
int rtfslfs_test(void);
#endif
int rootscan_test_callback(struct rtfsl_file const *pcurrent_entry_file, void *puser_data)
{
	if (puser_data==(void *)ROOT_SCAN_TEST_PRINT_FILE_NAMES)
    {
		if (pcurrent_entry_file->rtfsl_direntry_type==RTFSL_ENTRY_TYPE_FILE)
		{
		    PRINTF("Found file %8.8s.%3.3s\n",pcurrent_entry_file->dos_inode.fname,pcurrent_entry_file->dos_inode.fext);
			return 2;
		}
		else if (pcurrent_entry_file->rtfsl_direntry_type==RTFSL_ENTRY_TYPE_EOF)
			return 1;
		else if (pcurrent_entry_file->rtfsl_direntry_type==RTFSL_ENTRY_TYPE_LFN)
				;
		else if (pcurrent_entry_file->rtfsl_direntry_type==RTFSL_ENTRY_TYPE_VOLUME)
			    ;
		else if (pcurrent_entry_file->rtfsl_direntry_type==RTFSL_ENTRY_TYPE_DIRECTORY)
		{
		    PRINTF("Found dir  %8.8s.%3.3s\n",pcurrent_entry_file->dos_inode.fname,pcurrent_entry_file->dos_inode.fext);
			return 2;
		}
   }
    return 0; /* Continue */
}
void test_rtfsl_lite(void)
{
	unsigned long fragrecord[32];
	struct rtfsl_statstructure statstruct;
	int rval;
	struct rtfsl_file root_file;
    rtfsl_diskopen();

#if (DOFAILSAFEOPENTEST&&RTFSL_INCLUDE_FAILSAFE_SUPPORT)
	rtfsl.rtfsl_current_failsafe_context=&rtfsl.rtfsl_failsafe_context;
	ANSImemset(rtfsl.rtfsl_current_failsafe_context,0,sizeof(*rtfsl.rtfsl_current_failsafe_context));
	rtfsl.rtfsl_current_failsafe_context->journal_buffer=rtfsl.rtfslfs_sector_buffer;
	rtfsl.rtfsl_current_failsafe_context->journal_buffer_size=RTFSL_CFG_FSBUFFERSIZEBYTES;
	rtfslfs_start();
#endif

#if (DOFAILSAFERESTORETEST&&RTFSL_INCLUDE_FAILSAFE_SUPPORT)
	rtfslfs_restore();
	rtfsl_setpath(0);
	rtfsl_mkdir((unsigned char*)"AFTERRESTORE");
	return;
#endif
#if (DOFATSCANTEST)
	{
	unsigned long maxfindex;
	unsigned long cluster, next_cluster;
	long rval;
#ifdef RTFSL_MAJOR_VERSION
	maxfindex=pdr->drive_info.maxfindex;
#else
	maxfindex=pdr->maxfindex;
#endif

		for (cluster=2;cluster< maxfindex;cluster++)
		{
			next_cluster=cluster+1;
			rval=fatop_buff_get_frag(&tdrive, cluster|RTFSL_WRITE_CLUSTER, &next_cluster, 1);
			next_cluster=0;
			rval=fatop_buff_get_frag(&tdrive, cluster, &next_cluster, 1);
			if (next_cluster != cluster+1)
            {
				PRINTF("error: Cluster %ld: should be %X but returned %X\n", cluster, cluster+1,next_cluster);
            }
		}
		{
			unsigned long cluster_segment_array[32][2];
			unsigned long chain_length_clusters,start_next_segment;
			int i,num_segments;

			num_segments = rtfsl_load_cluster_chain(&tdrive, 2, &chain_length_clusters, &start_next_segment, cluster_segment_array, 32);

			PRINTF(" = %d\n", num_segments);
			for (i = 0; i < num_segments; i++)
			{
				PRINTF("%d: (%ld) - (%ld) \n", i, cluster_segment_array[i][0],cluster_segment_array[i][1]);
			}

		}
	}
#endif
#if (RTFSL_INCLUDE_WRITE_SUPPORT&&RTFSL_INCLUDE_SUBDIRECTORIES&&DOSETPATHTEST)
	{
		if (rtfsl_mkdir(path[0])<0)
        {
			PRINTF("First mkdir failed-assuming path exists\n");
        }
		path[1]=0; /* Set default path one layer deep */
		rtfsl_setpath(path);
		/* Set up a scenario so the directory entries we create are fragmented - make 4 fragments but test only uses 2 */
#if (DIRECTORY_GAP_FRAGMENT_LENGTH&&DOMKDIRTEST)
			ut_create_fragments(DIRECTORY_FRAGMENT_LENGTH, DIRECTORY_GAP_FRAGMENT_LENGTH, 4, fragrecord/*[numfrags]*/);
#endif
		if (rtfsl_mkdir(path[0])<0)
        {
			PRINTF("Second mkdir failed-assuming path exists\n");
        }
		path[1]=path[0];      /* Path should be two deep now */
	}
#endif

#if (RTFSL_INCLUDE_WRITE_SUPPORT&&DOMKDIRTEST&&RTFSL_INCLUDE_SUBDIRECTORIES&&DOSETPATHTEST&&DOMULTICHAINSUBDIRECTORYTEST)
	ut_fill_directory(0);
#endif

#if (RTFSL_INCLUDE_WRITE_SUPPORT&&RTFSL_INCLUDE_SUBDIRECTORIES&&DOMKDIRTEST)
	if (rtfsl_mkdir((unsigned char*)"MKDIR   MDX")<0)
	{
			PRINTF("mkdir test failed\n");
			return;
	}

#endif
#if (RTFSL_INCLUDE_WRITE_SUPPORT&&DOMKDIRTEST&&DIRECTORY_GAP_FRAGMENT_LENGTH&&RTFSL_INCLUDE_SUBDIRECTORIES&&DOSETPATHTEST&&DOMULTICHAINSUBDIRECTORYTEST)
	/* Release the clusters we allocate so the directory entries we create were fragmented */
		ut_release_fragments(DIRECTORY_FRAGMENT_LENGTH, DIRECTORY_GAP_FRAGMENT_LENGTH, 4, fragrecord/*[numfrags]*/);
#endif

#if (DOROOTSCANTEST)
	{
	struct rtfsl_file current_entry_file;
		if (rtfsl_root_finode_open(&root_file) == 0)
        {
            int r;
            do {
                PRINTF("Calling enum dir \n");
		        r=rtfsl_enumerate_directory(&root_file,&current_entry_file,rootscan_test_callback,(void *) ROOT_SCAN_TEST_PRINT_FILE_NAMES);
             } while (r==2);
		}
	}
#endif
#if (DOROOTGFIRSTSCANTEST)
	if (rtfsl_root_finode_open(&root_file) == 0)
    {
		struct rtfsl_dstat dstat;
		if (rtfsl_gfirst(&dstat, 0)==1)
		{
			do {
				if ((dstat.fattribute&(AVOLUME|ADIRENT))==0)
                {
					PRINTF("Got file : %s size: %ld \n",dstat.fnameandext,dstat.fsize);
                }
				else
                {
					PRINTF("Got entry: %s \n",dstat.fnameandext);
                }
			} while (rtfsl_gnext(&dstat)==1);
		}
	}
#endif

#if (RTFSL_INCLUDE_WRITE_SUPPORT&&DOFILEWRITETEST)
		rval=rtfsltest_file_sequential_write((unsigned char *)BOOTFILENAME, FILE_FRAGMENT_LENGTH, FILE_GAP_FRAGMENT_LENGTH, WRITETESTNLONGS, DOAPPENDTEST);
		if (rval<0)
			return;
#endif
#if (DOSEQUENTIALREADTEST)
		rval=rtfsltest_file_sequential_read((unsigned char *)BOOTFILENAME);
		if (rval<0)
			return;
#endif
#if (RTFSL_INCLUDE_WRITE_SUPPORT&&DOFILEWRITETEST&&DOFILERANDOMWRITETEST)
		rval=rtfsltest_file_random_write((unsigned char *)BOOTFILENAME);
		if (rval<0)
			return;
#endif
	PRINTF("File sequential write-seek-write-seek-read test completed\n");
#if (RTFSL_INCLUDE_WRITE_SUPPORT&&DOFILEWRITETEST&&DORENAMETEST)
	if (rtfsl_rename((unsigned char *)BOOTFILENAME, (unsigned char *)BOOTFILENAME)!=RTFSL_ERROR_EXIST)
    {
		PRINTF("Rename should have failed but did not\n");
    }
	rtfsl_rename((unsigned char *)BOOTFILENAME, (unsigned char *)RENAMEDFILENAME);
#endif
#if (DOBOOTFILEEST)
	{
	 int fd;
     void * load_address = malloc(1000000);
	    rtfsl_load_file((unsigned char *)BOOTFILENAME, (unsigned long) load_address);
 #if (DOFILIOREADAPITEST)
		fd=rtfsl_open((unsigned char*)BOOTFILENAME);
		if (fd>=0)
		{
			int nread;
			unsigned long total=0;
			unsigned char *loadimage=(unsigned char *)load_address;
			rtfsl_fstat(fd,&statstruct);
			do
			{
				nread=rtfsl_read(fd,test_buffer,rtfsl.current_dr.bytespsector);
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
				PRINTF("filesize == %d total read == %ld\n",statstruct.st_size,total);
			}
#if (DOFILERANDOMREADTEST)
			{
			unsigned long seekoffset,seekpointer;
			for (seekoffset=1;seekoffset<7;seekoffset++)
			{
				for (seekpointer=0; seekpointer<statstruct.st_size;seekpointer += seekoffset)
				{
					loadimage=(unsigned char *)load_address;
					if ((seekpointer & 0x3ff) == 0)
                    {
						PRINTF("seek test pass %d pointer: %8.8ld\r", seekoffset, seekpointer);
                    }
					rtfsl_lseek(fd,0,PSEEK_SET);
					rtfsl_lseek(fd,seekpointer,PSEEK_SET);
					rtfsl_read(fd,test_buffer,1);
					if (test_buffer[0] != *(loadimage+seekpointer))
                    {
						PRINTF("\nSeek test compare error at %ld\n",seekpointer);
                    }
				}
			}
			}
#endif
			PRINTF("load-sequential read-seek-read test completed %ld bytes read and verified\n",statstruct.st_size);
		}
#endif // #if (DOFILIOREADAPITEST)
	    free(load_address);
    }
#endif
#if (RTFSL_INCLUDE_WRITE_SUPPORT&&DORMDIRTEST&&RTFSL_INCLUDE_SUBDIRECTORIES&&DOSETPATHTEST)
	/* Remove the current working directory */
	path[1]=0; /* Set default path one layer deep */
	rtfsl_setpath(path);
	if (rtfsl_rmdir((unsigned char*)path[0])==0)
    {
		PRINTF("rmdir succeeded but it should have failed\n");
    }
	path[1]=path[0]; /* Set default path 2 deep */
	rtfsl_setpath(path);
	ut_unfill_directory(0);
	path[1]=0; /* Set default path one layer deep */
	rtfsl_setpath(path);
	if (rtfsl_rmdir((unsigned char*)path[0])!=0)
	{
		PRINTF("rmdir failed even after cleaning directory\n");
		return;
	}
#endif
#if (DOFAILSAFESYNCTEST&&DOFAILSAFEFLUSHTEST)
#error Mutually exclusive
#endif
#if (RTFSL_INCLUDE_WRITE_SUPPORT&&RTFSL_INCLUDE_SUBDIRECTORIES&&DOFILEDELETETEST)
	rtfsl_delete((unsigned char *)BOOTFILENAME);
#endif
#if (DOFAILSAFEFLUSHTEST&&RTFSL_INCLUDE_FAILSAFE_SUPPORT)
	rtfslfs_flush();
#endif
#if (DOFAILSAFESYNCTEST&&RTFSL_INCLUDE_FAILSAFE_SUPPORT)
	rtfslfs_sync();
	rtfsl_setpath(0);
	rtfsl_mkdir((unsigned char*)"AFTERSYNC  ");
	rtfslfs_flush();
	PRINTF("Restore and you will see the \\AFTERSYNC directorty \n");
#endif

}
