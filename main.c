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
#ifdef __linux__
int rtfsl_open_disk(char *raw_dev_name);
#endif

void main(int argc, char **argv)
{
  if (argc != 2)
  {
    printf("Usage: %s block_dev_name\n", argv[0]);
    printf("Usage: You must provide the device name of fat formatted device\n");
    return;
  }
  if (rtfsl_open_disk(argv[1]) < 0)
  {
    printf("Could not open device: %s\n", argv[1]);
    return;
  }
  rtfslite_shell();
}
