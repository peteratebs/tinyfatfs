/*
* EBS - RTFS (Real Time File Manager)
*
* Copyright EBS Inc. 1987-2012
* All rights reserved.
* This code may not be redistributed in source or linkable object form
* without the consent of its author.
*/
#ifdef _MSC_VER
#include <windows.h>
#endif
#ifdef __linux__
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
int  raw_dev_fd;

int rtfsl_open_disk(char *raw_dev_name)
{
  raw_dev_fd = open(raw_dev_name,O_RDWR);
  return raw_dev_fd;
}
static int rtfsl_dev_seek(unsigned long sector)
{
unsigned long hi, lo;
unsigned long long lhi, llo, result,llbytes;

   llbytes = (unsigned long long) sector;
   llbytes *= 512;

   lhi = llbytes >> 32;
   llo = llbytes & 0xffffffff;
   lo = (unsigned long) llo;
   hi = (unsigned long) lhi;

   if (lseek64(raw_dev_fd, llbytes, SEEK_SET)!= llbytes)
    return(-1);

   return(0);
}

#endif
//#include "rtfs.h"
#ifdef RTFS_MAJOR_VERSION
DDRIVE *rtfsl_p_drive;
#endif

#if (INCLUDE_SDCARD)
int SDCARD_blkmedia_io(void  *devhandle, void *pdrive, unsigned long sector, void  *buffer, unsigned long count, int reading);
#endif

int rtfsl_read_sectors(unsigned long sector, int count, unsigned char *buffer)   /*__apifn__*/
{
#ifdef __linux__
unsigned long nbytes, nread;
   if (rtfsl_dev_seek(sector)==0)
   {
        nbytes = (unsigned long)count;
        nbytes *= 512;

      if ((nread = read(raw_dev_fd,buffer,nbytes)) == nbytes)
	   return 0;
   }
   return -1;
#endif
#ifdef RTFS_MAJOR_VERSION
	return raw_devio_xfer(rtfsl_p_drive, (sector), buffer,1, TRUE, TRUE)?0:-1000;
#endif
#if (INCLUDE_SDCARD)
    return SDCARD_blkmedia_io( (void  *) 0, (void *) 0, sector, buffer, count, 1);
#endif
    return  -1; // RTFSL_ERROR_IO_ERROR;
}
int rtfsl_write_sectors(unsigned long sector, int count, unsigned char *buffer)    /*__apifn__*/
{
#ifdef __linux__
unsigned long nbytes, nwr;
   if (rtfsl_dev_seek(sector)==0)
   {
        nbytes = (unsigned long)count;
        nbytes *= 512;

      if ((nwr = write(raw_dev_fd,buffer,nbytes)) == nbytes)
	   return 0;
   }
   return -1;
#endif
#ifdef RTFS_MAJOR_VERSION
	return raw_devio_xfer(rtfsl_p_drive, (sector), buffer,1, TRUE, FALSE)?0:-1000;
#endif
#if (INCLUDE_SDCARD)
    return SDCARD_blkmedia_io( (void  *) 0, (void *) 0, sector, buffer, count, 0);
#endif
    return  -1; // RTFSL_ERROR_IO_ERROR;
}

/*
    When the system needs to date stamp a file it will call this routine
    to get the current time and date. YOU must modify the shipped routine
    to support your hardware's time and date routines.
*/

void rtfsl_get_system_date(unsigned short *time, unsigned short *date)                    /*__apifn__*/
{
#ifdef _MSC_VER
	/* Windows runtime provides rotuines specifically for this purpose */
    SYSTEMTIME systemtime;
    FILETIME filetime;

    GetLocalTime(&systemtime);
    SystemTimeToFileTime(&systemtime, &filetime);
    FileTimeToDosDateTime(&filetime, date, time);
#else

#define USE_ANSI_TIME 0   /* Enable if your runtime environment supports ansi time functions */
#if (USE_ANSI_TIME)
	{ /* Use ansi time functions. */
    struct tm *timeptr;
    time_t timer;
   unsigned short  year;     /* relative to 1980 */
    unsigned short  month;    /* 1 - 12 */
    unsigned short  day;      /* 1 - 31 */
    unsigned short  hour;
    unsigned short  minute;
    unsigned short  sec;      /* Note: seconds are 2 second/per. ie 3 == 6 seconds */

    time(&timer);
    timeptr = localtime(&timer);

    hour    =   (unsigned short) timeptr->tm_hour;
    minute  =   (unsigned short) timeptr->tm_min;
    sec =   (unsigned short) (timeptr->tm_sec/2);
    /* Date comes back relative to 1900 (eg 93). The pc wants it relative to
        1980. so subtract 80 */
    year  = (unsigned short) (timeptr->tm_year-80);
    month = (unsigned short) (timeptr->tm_mon+1);
    day   = (unsigned short) timeptr->tm_mday;
    *time = (unsigned short) ( (hour << 11) | (minute << 5) | sec);
    *date = (unsigned short) ( (year << 9) | (month << 5) | day);
	}
#else /* In not windows and not using ansi time functions use hardwired values. */
    /* Modify this code if you have a clock calendar chip and can retrieve the values from that device instead */
#define hour 19    /* 7:37:28 PM */
#define minute 37
#define sec 14
    /* 3-28-2008 */
#define year 18       /* relative to 1980 */
#define month 3      /* 1 - 12 */
#define day 28       /* 1 - 31 */
    *time = (unsigned short) ( (hour << 11) | (minute << 5) | sec);
    *date = (unsigned short) ( (year << 9) | (month << 5) | day);
#endif

#endif /* #ifdef WINDOWS #else */
}


/* Convert a 32 bit intel item to a portable 32 bit */
unsigned long to_ULONG (unsigned char *from)                                                                        /*__fn__*/
{
    unsigned long res;
    unsigned long t;
    t = ((unsigned long) *(from + 3)) & 0xff;
    res = (t << 24);
    t = ((unsigned long) *(from + 2)) & 0xff;
    res |= (t << 16);
    t = ((unsigned long) *(from + 1)) & 0xff;
    res |= (t << 8);
    t = ((unsigned long) *from) & 0xff;
    res |= t;
    return(res);
}

/* Convert a 16 bit intel item to a portable 16 bit */
unsigned short to_USHORT (unsigned char *from)                                                                      /*__fn__*/
{
    unsigned short nres;
    unsigned short t;
    t = (unsigned short) (((unsigned short) *(from + 1)) & 0xff);
    nres = (unsigned short) (t << 8);
    t = (unsigned short) (((unsigned short) *from) & 0xff);
    nres |= t;
    return(nres);
}

/* Convert a portable 16 bit to a  16 bit intel item */
void fr_USHORT (unsigned char *to,  unsigned short from)                                            /*__fn__*/
{
    *to             =       (unsigned char) (from & 0x00ff);
    *(to + 1)   =   (unsigned char) ((from >> 8) & 0x00ff);
}

/* Convert a portable 32 bit to a  32 bit intel item */
void fr_ULONG (unsigned char *to,  unsigned long from)                                          /*__fn__*/
{
    *to = (unsigned char) (from & 0xff);
    *(to + 1)   =  (unsigned char) ((from >> 8) & 0xff);
    *(to + 2)   =  (unsigned char) ((from >> 16) & 0xff);
    *(to + 3)   =  (unsigned char) ((from >> 24) & 0xff);
}
