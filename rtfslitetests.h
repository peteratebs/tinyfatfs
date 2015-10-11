/*
* EBS - RTFS (Real Time File Manager)
*
* Copyright EBS Inc. 1987-2012
* All rights reserved.
* This code may not be redistributed in source or linkable object form
* without the consent of its author.
*/


extern unsigned char test_buffer[RTFSL_CFG_MAXBLOCKSIZE];
int ut_fill_directory(int leave_n_free);
void ut_unfill_directory(unsigned char *name);
int ut_release_fragments(int file_fraglen, int gap_fraglen, int numfrags, unsigned long *fragrecord/*[numfrags]*/);
int ut_create_fragments(int file_fraglen, int gap_fraglen, int numfrags, unsigned long *fragrecord/*[numfrags]*/);

int rtfsltest_file_sequential_write(unsigned char*filename, int file_fraglen, int gap_fraglen, int numlongs, int test_append);
int rtfsltest_file_fill_drive(unsigned char*filename);
int rtfsltest_file_sequential_read(unsigned char*filename);
int rtfsltest_file_random_read(unsigned char*filename);
int rtfsltest_file_random_write(unsigned char*filename);

#include <stdio.h>
#define PRINTF printf
