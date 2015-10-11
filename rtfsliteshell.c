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
#include "stdio.h"
#include <ctype.h>

#define rtp_term_gets gets

// File load
void BootProgressReport(int percent)
{

}

char lite_term_buffer[40];
int lite_shell_running=0;
int lite_shell_help=0;
struct _command_list {
    char *cmd;
    int  (*proc)( int argc, char **argv);
    char *helpstr;
};
static int dortfslite_help(int argc,char **argv);
static int dortfslite_exit(int argc,char **argv);
static int dortfslite_mount(int argc,char **argv);
static int dortfslite_lflush(int argc,char **argv);
static int dortfslite_dir(int argc,char **argv);
static int dortfslite_delete(int argc,char **argv);
static int dortfslite_rename(int argc,char **argv);
static int dortfslite_chdir(int argc,char **argv);
static int dortfslite_mkdir(int argc,char **argv);
static int dortfslite_rmdir(int argc,char **argv);
static int dortfslite_fill(int argc,char **argv);
static int dortfslite_append(int argc,char **argv);
static int dortfslite_check(int argc,char **argv);
static int dortfslite_randread(int argc,char **argv);
static int dortfslite_randwrite(int argc,char **argv);
static int dortfslite_filldisk(int argc,char **argv);
static int dortfslite_fsstart(int argc,char **argv);
static int dortfslite_fsstop(int argc,char **argv);
static int dortfslite_fssync(int argc,char **argv);
static int dortfslite_fsflush(int argc,char **argv);
static int dortfslite_fsrestore(int argc,char **argv);
static int dortfslite_load(int argc,char **argv);

static const struct _command_list command_list[] = {
    {  "HELP",         dortfslite_help,     "HELP"},
    {  "LOAD",         dortfslite_load,    "LOAD FILENAME"},
    {  "LMOUNT",       dortfslite_mount,    "LMOUNT   - re-mount rtfs lite drive"},
    {  "LEXIT",        dortfslite_exit,     "LEXIT    - Exit Lite mode refreshes device mount for rtfs"},
    {  "LFLUSH",        dortfslite_lflush,   "LFLUSH   - Flush rtfs lite buffers"},
    {  "DIR",          dortfslite_dir,      "DIR"},
    {  "RENAME",       dortfslite_rename,   "RENAME oldname newname"},
    {  "DELETE",       dortfslite_delete,   "DELETE filename"},
    {  "CHDIR",        dortfslite_chdir,    "CHDIR path"},
    {  "MKDIR",        dortfslite_mkdir,    "MKDIR dirname 0|1 (1 = fragment)"},
    {  "RMDIR",        dortfslite_rmdir,    "RMDIR dirname"},
    {  "FILLPAT",      dortfslite_fill,     "FILLPAT filename nlongs 0|1 (1=fragment)"},
    {  "APPENDPAT",    dortfslite_append,   "APPENDPAT filename nlongs 0|1 (1=fragment)"},
    {  "READPAT",     dortfslite_check,     "READPAT filename"},
    {  "RANDREAD",     dortfslite_randread, "RANDREAD  filename"},
    {  "RANDWRITE",    dortfslite_randwrite,"RANDWRITE filename"},
    {  "FILLDISK",     dortfslite_filldisk,"FILLDISK filenamebase"},

    {  "FSSTART",      dortfslite_fsstart,  "FSSTART Start Journaling"},
    {  "FSSTOP",       dortfslite_fsstop,   "FSSTOP  Stop  Journaling"},
    {  "FSSYNC",       dortfslite_fssync,   "FSSYNC  Sync  volume, keep journaling"},
    {  "FSFLUSH",      dortfslite_fsflush,  "FSFLUSH Flush journal keep journaling"},
    {  "FSRESTORE",    dortfslite_fsrestore,"FSRESTORE Retore the volume from the journal"},
	{0,0,0}
};

static void GetUserInput(void);
static char *gnext(char *p);
static void prompt(void);
static void prompt(void);
static unsigned char * Convert83tortfslite83(char *to, char *from);

void rtfslite_shell(void)
{
    lite_shell_running=1;
	lite_shell_help = 1;
    if (rtfsl_diskopen()!=0)
	{
		PRINTF("Lite disk mount failed can not continue\n");
		return;
	}
    while (lite_shell_running)
        prompt();
}


static int check_args(int argc,int shouldbe);
static void check_return(int rval,int shouldbe);

static int dortfslite_help(int argc,char **argv)
{
    lite_shell_help=1;
    return 0;
}

#define LOAD_ADDRESS  0xa0100000
#define START_ADDRESS 0xa014d8bc  /* iar_start_address */
#define STACK_ADDRESS 0xa01937d0  /* cstack limit */
typedef void (* JUMPTO)(void);
unsigned long rtp_strtoul (
  const char * str,                     /** String to be converted. */
  char ** delimiter,                    /** Character that stops the convertion. */
  int base                              /** Base of return value. */
);

static int get_load_address(unsigned char *filename, unsigned long *loadaddr,  unsigned long *stackaddr, unsigned long *startaddr)
{
int fd,rval;
struct rtfsl_statstructure statstruct;
char readbuffer [36];
    *loadaddr= 0xa0100000;
    *stackaddr=0xa01937d0;
    *startaddr=0xa014d8bc;
    fd = rtfsl_open((unsigned char*)filename);
	if (fd>=0)
	{
        rval=rtfsl_read(fd, readbuffer, 36);
        if (rval < 36)
        {
          PRINTF("Read Metadata failed\n");
          return -1;
        }
        /* [0123456789AB][0123456789AB][0123456789AB] 10 digits plus CR LF */
        readbuffer[10]=0;
        readbuffer[22]=0;
        readbuffer[34]=0;

//        *loadaddr=rtp_strtoul(&readbuffer[0], 0, 16);
//        *stackaddr=rtp_strtoul(&readbuffer[12], 0, 16);
//        *startaddr=rtp_strtoul(&readbuffer[24], 0, 16);
        rtfsl_close(fd);
        return 0;
    }
    else
    {
      PRINTF("Meta Data Open failed\n");
    }


    return -1;
}

static JUMPTO go_main;
static unsigned long loadaddr, stackaddr, startaddr;
int rtfslite_loadfilefromdisk(char *filenambase);
static int dortfslite_load(int argc,char **argv)
{
  return rtfslite_loadfilefromdisk(*argv);
}
extern void TouchScrClose(void);
int rtfslite_loadfilefromdisk(char *filenambase)
{
int rval;
char buffer[32];
char filename[32];
char binfilename[32];
char addrfilename[32];
loadaddr = LOAD_ADDRESS;

    strcpy(binfilename,filenambase);
    strcat(binfilename,".BIN");
    strcpy(addrfilename,filenambase);
    strcat(addrfilename,".TXT");

    if (get_load_address(Convert83tortfslite83(filename, addrfilename), &loadaddr,  &stackaddr, &startaddr)==0)
    {
       // PRINTF("Got load addr %s\n", rtp_ultoa (loadaddr,  buffer,16));
       // PRINTF("Got stack addr %s\n", rtp_ultoa (stackaddr,  buffer,16));
       // PRINTF("Got run addr %s\n", rtp_ultoa (startaddr,  buffer,16));

       go_main = (JUMPTO) startaddr;
       Convert83tortfslite83(filename,binfilename);
        PRINTF("loading file: :%s: \n", filename);
        rval = rtfsl_load_file(filename, (unsigned long) loadaddr);
        if (rval==0)
        {
//            PRINTF("load file returned %d \n", rval);
//           TouchScrClose(); /* Disable interrupts before jumping off */
//            __set_SP(stackaddr);
            go_main();
        }
        return 0;
    }
}

static int dortfslite_mount(int argc,char **argv)
{
int rval;
    rval=rtfsl_diskopen();
    check_return(rval,0);
    return 0;
}


static int dortfslite_exit(int argc,char **argv)
{
    lite_shell_running=0;
    return 0;
}
static int dortfslite_lflush(int argc,char **argv)
{
    rtfsl_flush_all_buffers();
    return 0;
}
static int dortfslite_dir(int argc,char **argv)
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
    return 0;
}
static int dortfslite_delete(int argc,char **argv)
{
char filename[12];
int rval;
    if (check_args(argc,1)==0)
	{
		rval=rtfsl_delete(Convert83tortfslite83(filename, *argv));
		check_return(rval,0);
	}
    return 0;
}
static int dortfslite_rename(int argc,char **argv)
{
unsigned char filename[12],tofilename[12];
int rval;
    if (check_args(argc,2)==0)
	{
		Convert83tortfslite83((char *)filename, *argv++);
		Convert83tortfslite83((char *)tofilename, *argv);
		rval=rtfsl_rename(filename,tofilename);
		check_return(rval,0);
	}
    return 0;
}

unsigned char *shellpathnamearray[8];
char shellpathnamestore[8][12];

static int dortfslite_chdir(int argc,char **argv)
{
char *p,*pnext;
int depth = 0;
    if (argc==0)
	{
        rtfsl_setpath(0);
		return 0;
	}
    p= *argv;
    while (p)
    {
        pnext = strstr(p, "\\");
        if (pnext)
            *pnext =0;
        Convert83tortfslite83(shellpathnamestore[depth],p);
        shellpathnamearray[depth]=(unsigned char *)shellpathnamestore[depth];
        shellpathnamearray[depth+1]=0;
        depth+=1;
        if (pnext)
            p=pnext+1;
        else
            p=0;
    }
    rtfsl_setpath(shellpathnamearray);
    return 0;
}
static int dortfslite_mkdir(int argc,char **argv)
{
char filename[12];
int rval;
    rval=rtfsl_mkdir(Convert83tortfslite83(filename, *argv));
    check_return(rval,0);
    return 0;
}
static int dortfslite_rmdir(int argc,char **argv)
{
char filename[12];
int rval;
    rval=rtfsl_rmdir(Convert83tortfslite83(filename, *argv));
    check_return(rval,0);
    return 0;
}
static int dortfslite_fill(int argc,char **argv)
{
char filename[12];
int nlongs;
int gap;
int rval;

    if (check_args(argc,3)==0)
    {
        Convert83tortfslite83(filename, *argv++);
        nlongs = atol(*argv++);
        gap = atol(*argv++);
        rval=rtfsltest_file_sequential_write((unsigned char *)filename, 1, gap, nlongs, 0);
        check_return(rval,0);
    }
    return 0;
}

static int dortfslite_filldisk(int argc,char **argv)
{
char filename[12];
int rval;

    if (check_args(argc,1)==0)
    {
        Convert83tortfslite83(filename, *argv++);
        rval=rtfsltest_file_fill_drive((unsigned char *)filename);
        check_return(rval,RTFSL_ERROR_DISK_FULL);
    }
    return 0;
}

static int dortfslite_append(int argc,char **argv)
{
char filename[12];
int nlongs;
int gap;
int rval;

    if (check_args(argc,3)==0)
    {
        Convert83tortfslite83(filename, *argv++);
        nlongs = atol(*argv++);
        gap = atol(*argv++);
        rval=rtfsltest_file_sequential_write((unsigned char *)filename, 1, gap, nlongs, 1);
        check_return(rval,0);
    }
    return 0;
}


static int dortfslite_check(int argc,char **argv)
{
char filename[12];
int rval;
    if (check_args(argc,1)==0)
    {
        Convert83tortfslite83(filename, *argv++);
        rval=rtfsltest_file_sequential_read((unsigned char *)filename);
        check_return(rval,0);
    }
    return 0;
}
static int dortfslite_randread(int argc,char **argv)
{
char filename[12];
int rval;
    if (check_args(argc,1)==0)
    {
        Convert83tortfslite83(filename, *argv++);
        rval=rtfsltest_file_random_read((unsigned char *)filename);
        check_return(rval,0);
    }
    return 0;
}
static int dortfslite_randwrite(int argc,char **argv)
{
char filename[12];
int rval;
    if (check_args(argc,1)==0)
    {
        Convert83tortfslite83(filename, *argv++);
        rval=rtfsltest_file_random_write((unsigned char *)filename);
        check_return(rval,0);
    }
    return 0;
}

static int dortfslite_fsstart(int argc,char **argv)
{
int rval;
    rval=rtfslfs_start();
    check_return(rval,0);
	return 0;
}
static int dortfslite_fsstop(int argc,char **argv)
{
int rval;
    rval=rtfsl_diskopen();
    check_return(rval,0);
    return 0;
}
static int dortfslite_fssync(int argc,char **argv)
{
int rval;
	rval=rtfslfs_sync();
    check_return(rval,0);
    return 0;
}
static int dortfslite_fsflush(int argc,char **argv)
{
int rval;
	rval=rtfslfs_flush();
    check_return(rval,0);
    return 0;
}
static int dortfslite_fsrestore(int argc,char **argv)
{
int rval;
	rval=rtfslfs_restore();
    check_return(rval,0);
    return 0;
}

const char *err_strings[] = {
"RTFSL_ERROR_NONE",
"RTFSL_ERROR_ARGS",
"RTFSL_ERROR_CONSISTENCY",
"RTFSL_ERROR_DIR_FULL",
"RTFSL_ERROR_DISK_FULL",
"RTFSL_ERROR_FDALLOC",
"RTFSL_ERROR_FORMAT",
"RTFSL_ERROR_JOURNAL_FULL",
"RTFSL_ERROR_JOURNAL_NONE",
"RTFSL_ERROR_NOTFOUND",
"RTFSL_ERROR_PATH",
"RTFSL_ERROR_EXIST",
"RTFSL_ERROR_ENOTEMPTY",
"RTFSL_ERROR_TEST"};
static void check_return(int rval,int shouldbe)
{
    if (rval!=shouldbe)
    {
		if (rval < 0)
        {
		    PRINTF("Returned: %s ", err_strings[-rval]);
        }
        else
        {
		    PRINTF("Returned: %d ", rval);
        }
        PRINTF(", should be %d\n", shouldbe);
    }
}

static int check_args(int argc,int shouldbe)
{
    if (argc!=shouldbe)
    {
        PRINTF("Please pass %d arguments\n", shouldbe);
        return -1;
    }
    return 0;
}

/* ================================ */
static void GetUserInput()
{
   rtp_term_gets(lite_term_buffer);
}

static char *gnext(char *p)                                           /*__fn__*/
{
    while (*p==' ') p++;   /* GET RID OF LEADING SPACES */
    while (*p)
    {
        if (*p==' ')
        {
            *p=0;
            p++;
            break;
        }
        p++;
    }
    while (*p==' ') p++; /* GET RID OF Trailing SPACES */
    if (*p==0)
        return(0);
    return (p);
}
/* ******************************************************************** */
/* get next command; process history log */
static void prompt(void)
{
    int i;
    char *cmd,*p;
    char *args[4];
    int  argc=0;

	if (lite_shell_help)
	{
		for (i=0; command_list[i].cmd;i++)
		{
			PRINTF("%s\n",command_list[i].helpstr);
		}
		lite_shell_help=0;
	}
    argc = 0;
    /* "CMD>" */
    PRINTF("CMD>");
    GetUserInput();

    p = cmd = &lite_term_buffer[0];
    p = gnext(p);
    /* Keep grabbing tokens until there are none left */
    while (p)
    {
        args[argc++] = p;
        p = gnext(p);
    }

    for (i=0; command_list[i].cmd;i++)
    {
        if (strcmp(command_list[i].cmd, cmd)==0)
        {
           if (command_list[i].proc)
            command_list[i].proc(argc,args);
           break;
        }
    }
 }


static unsigned char * Convert83tortfslite83(char *to, char *from)
{
int i;
char *r=to;
    for (i =0; i < 8; i++)
    {
        if (*from==0||*from=='.')
            *to++=' ';
        else
            *to++=(char)toupper(*from++);
    }
    if (*from=='.')
        from++;
    for (i =0; i < 3; i++)
    {
        if (*from==0)
            *to++=' ';
        else
            *to++=(char)toupper(*from++);
    }
	*to=0;
	return (unsigned char *)r;
}
