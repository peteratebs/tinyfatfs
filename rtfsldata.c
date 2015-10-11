/*
* EBS - RTFS (Real Time File Manager)
*
* Copyright EBS Inc. 1987-2012
* All rights reserved.
* This code may not be redistributed in source or linkable object form
* without the consent of its author.
*/

#include "rtfslite.h"


struct rtfsl_context rtfsl;
unsigned char rtfsl_sector_buffer[RTFSL_CFG_MAXBLOCKSIZE*RTFSL_CFG_NUMBUFFERS];


