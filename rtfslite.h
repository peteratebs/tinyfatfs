/*
* EBS - RTFS (Real Time File Manager)
*
* Copyright EBS Inc. 1987-2012
* All rights reserved.
* This code may not be redistributed in source or linkable object form
* without the consent of its author.
*/

#ifndef __RTFSLITE__
#include <string.h>

#define  ANSImemset memset
#define  ANSImemcmp memcmp
#define  ANSImemcpy memcpy


#define RTFSL_INCLUDE_SUBDIRECTORIES 1
#define RTFSL_INCLUDE_FAT32 1
#define RTFSL_INCLUDE_FAT12 1
#define RTFSL_INCLUDE_MBR_SUPPORT 1  /* 180 bytes, need 3rd option as well */
#define RTFSL_INCLUDE_WRITE_SUPPORT 1  /* XXX bytes */
#define RTFSL_INCLUDE_FAILSAFE_SUPPORT 1  /* XXX bytes */
#define RTFSL_INCLUDE_SECURE_DIGITAL 1
#define RTFSL_CFG_FILE_FRAG_BUFFER_SIZE 4
#define RTFSL_CFG_FSBUFFERSIZESECTORS 1
#define RTFSL_CFG_FSBUFFERSIZEBYTES (RTFSL_CFG_FSBUFFERSIZESECTORS*512)
#define RTFSL_CFG_NFILES 2
#define RTFSL_CFG_MAXBLOCKSIZE 512
#define RTFSL_CFG_NUMBUFFERS   2

/* Set this to 1 build a minimum system supporting only the API functions
rtfsl_load_file and rtfsl_setpath. If being able to load only from the root is
acceptable you can reduce footprint further by setting RTFSL_INCLUDE_SUBDIRECTORIES to 0 */

#define RTFSL_INCLUDE_LOAD_ONLY 0
#if (RTFSL_INCLUDE_LOAD_ONLY)
#undef RTFSL_INCLUDE_WRITE_SUPPORT
#undef RTFSL_INCLUDE_FAILSAFE_SUPPORT
#undef RTFSL_INCLUDE_SECURE_DIGITAL
#undef RTFSL_CFG_NFILES
#undef RTFSL_CFG_NUMBUFFERS
#define RTFSL_INCLUDE_WRITE_SUPPORT     0
#define RTFSL_INCLUDE_FAILSAFE_SUPPORT  0
#define RTFSL_INCLUDE_SECURE_DIGITAL    0
#define RTFSL_CFG_NFILES                1
#define RTFSL_CFG_NUMBUFFERS            1
#endif

#define RTFSL_ERROR_NONE              0
#define RTFSL_ERROR_ARGS             -1
#define RTFSL_ERROR_CONSISTENCY      -2
#define RTFSL_ERROR_DIR_FULL         -3
#define RTFSL_ERROR_DISK_FULL        -4
#define RTFSL_ERROR_FDALLOC          -5
#define RTFSL_ERROR_FORMAT           -6
#define RTFSL_ERROR_JOURNAL_FULL     -7
#define RTFSL_ERROR_JOURNAL_NONE     -8
#define RTFSL_ERROR_NOTFOUND         -9
#define RTFSL_ERROR_PATH             -10
#define RTFSL_ERROR_EXIST            -11
#define RTFSL_ERROR_ENOTEMPTY        -12
#define RTFSL_ERROR_TEST             -13

#define RTFSL_ERROR_IO_WRITE_PROTECT -14
#define RTFSL_ERROR_IO_NO_MEDIA      -15
#define RTFSL_ERROR_IO_ERROR         -16

#define RTFSL_WRITE_CLUSTER 0x8000000
#define RTFSL_ALLOC_CLUSTER 0x4000000
#define RTFSL_MASK_COMMAND  0xC000000
#define RTFSL_JOURNALFULL_CLUSTER 0xffffffff
#define RTFSL_JOURNALDELETED_CLUSTER 0xfffffffe

#define RTFSL_FIO_OP_WRITE  0x01
#define RTFSL_FIO_OP_APPEND 0x02

void rtfsl_get_system_date(unsigned short *time, unsigned short *date);
int rtfsl_read_sectors(unsigned long sector,int count, unsigned char *buffer);
int rtfsl_write_sectors(unsigned long sector, int count, unsigned char *buffer);

struct rtfsl_dosinode {
        unsigned char   fname[8];
        unsigned char   fext[3];
        unsigned char   fattribute;      /* File attributes */
        unsigned char   reservednt;
        unsigned char   create10msincrement;
        unsigned short  ctime;            /* time & date create */
        unsigned short  cdate;
        unsigned short  adate;			  /* Date last accessed */
        unsigned short  fclusterhi;     /* This is where fat32 stores file location */
        unsigned short  ftime;            /* time & date lastmodified */
        unsigned short  fdate;
        unsigned short  fcluster;         /* Cluster for data file */
        unsigned long   fsize;            /* File size */
        };

struct rtfsl_drive
{
	unsigned long  partition_base;
	unsigned long  partition_size;
	unsigned long  maxfindex;
    unsigned long  numsecs;
    unsigned long  secpfat;
    unsigned long  rootbegin;
    unsigned long  firstclblock;
    unsigned long  free_alloc;
    unsigned long  next_alloc;
	unsigned long  end_cluster_marker;
    unsigned short bytespsector;
    unsigned long  bytespcluster;
    unsigned short secreserved;
    unsigned short numroot;
#define RTFSL_IOTYPE_FILE_FLAG 0x01
#define RTFSL_FAT_CHANGED_FLAG 0x02	/* For FAT32 systems this is set when freespece has changed and the infosec needs a flush */
    unsigned short flags;
    unsigned short infosec;
    unsigned short backup;
    unsigned char  secpalloc;
    unsigned char  numfats;
    unsigned char  fasize;
	unsigned char  **pathnamearray;
};
struct rtfsl_file {
    unsigned long sector;
    struct rtfsl_dosinode dos_inode;
    unsigned long  file_pointer;
    unsigned long  file_pointer_at_segment_base;
    unsigned long  segment_size_bytes;
	unsigned long  next_segment_base;
    unsigned long  cluster_segment_array[RTFSL_CFG_FILE_FRAG_BUFFER_SIZE][2];
    unsigned short index;
#define TRTFSFILE_ISROOT_DIR		0x01
#define TRTFSFILE_DIRTY				0x02
#define TRTFSFILE_SECTOR_REGION		0x04
#define TRTFSFILE_ALLOCATED			0x08
#define TRTFSFILE_ISMKDIR			0x10
    unsigned char rtfsl_file_flags;
#define RTFSL_ENTRY_TYPE_ERASED     1 /* Must be power of 2. */
#define RTFSL_ENTRY_TYPE_DIRECTORY  2
#define RTFSL_ENTRY_TYPE_VOLUME	    4
#define RTFSL_ENTRY_TYPE_LFN		    8
#define RTFSL_ENTRY_TYPE_FILE	   16
#define RTFSL_ENTRY_TYPE_EOF	    32
#define RTFSL_ENTRY_AVAILABLE 		(RTFSL_ENTRY_TYPE_ERASED|RTFSL_ENTRY_TYPE_EOF)
    unsigned char rtfsl_direntry_type;
};

/* Structure for use by rtfsl_gfirst, rtfsl_gnext */
struct rtfsl_dstat {
        unsigned char     fnameandext[12];   /* Null terminated file and extension */
        unsigned char     fattribute;         /* File attributes */
        unsigned short    ftime;              /* time & date lastmodified. See date */
        unsigned short    fdate;              /* and time handlers for getting info */
        unsigned short    ctime;              /* time & date created */
        unsigned short    cdate;
        unsigned short    atime;              /* time & date accessed */
        unsigned short    adate;			  /* Date last accessed */
        unsigned long   fsize;              /* File size */
        /* INTERNAL */
		struct rtfsl_file directory_file;
		struct rtfsl_file current_matched_file;
		unsigned char *nametomatch;
        };
/* Structure for use by rtfsl_stat and rtfsl_fstat */
struct rtfsl_statstructure
{
    int st_dev;			/* (0) */
    int st_ino;			/* (0) */
    unsigned long   st_mode;	/* (0) */
    int st_nlink;		/* (0) */
    int st_rdev;        /* (0) */
    unsigned long   st_size;    /* file size, in bytes */
    unsigned long   st_atime;   /* last access date in high 16 bits */
    unsigned long   st_mtime;   /* last modification date|time */
    unsigned long   st_ctime;   /* file create date|time  */
    unsigned long   st_blksize; /* optimal blocksize for I/O (cluster size) */
    unsigned long   st_blocks;  /* blocks allocated for file */
    unsigned char   fattribute;    /* File attributes - DOS attributes
									(non standard but useful) */
};

#define PCDELETE (unsigned char) 0xE5
#define ARDONLY 0x1  /* MS-DOS File attributes */
#define AHIDDEN 0x2
#define ASYSTEM 0x4
#define AVOLUME 0x8
#define ADIRENT 0x10
#define ARCHIVE 0x20
#define ANORMAL 0x00
#define CHICAGO_EXT 0x0f    /* Chicago extended filename attribute */


#define PSEEK_SET   0   /* offset from begining of file*/
#define PSEEK_CUR   1   /* offset from current file pointer*/
#define PSEEK_END   2   /* offset from end of file*/

unsigned long to_ULONG( unsigned char *from);
unsigned short to_USHORT(  unsigned char *from);
void fr_USHORT(unsigned char *to,  unsigned short from);
void fr_ULONG(unsigned char *to, unsigned long from);



int rtfsl_diskopen(void);
unsigned long rtfsl_cl2sector(unsigned long cluster);
int rtfsl_finode_get( struct rtfsl_file *pfile, unsigned long sector, unsigned short index);
unsigned long rtfsl_sec2cluster( unsigned long sector);
int rtfsl_finode_open(struct rtfsl_file *pfile);
int rtfsl_root_finode_open(struct rtfsl_file *pfile);
long rtfsl_load_next_segment(struct rtfsl_file *pfile,unsigned long current_cluster);

typedef int (*DirScanCallback) (struct rtfsl_file const *pcurrent_entry_file, void *puser_data);
typedef unsigned long (*FileScanCallback) (struct rtfsl_file const *pfile, unsigned long start_sector, unsigned long nbytes, void *puser_data);

int rtfsl_enumerate_file(struct rtfsl_file *pfile,FileScanCallback pCallback, void *puser_data);
int rtfsl_enumerate_directory(struct rtfsl_file *pdirectory_file,struct rtfsl_file *pcurrent_entry_file,DirScanCallback pCallback, void *puser_data);

int rtfsl_read_sector_buffer(unsigned long sector);
int rtfsl_flush_sector_buffer(int bufferhandle,int force);
void rtfsl_mark_sector_buffer(int bufferhandle);
int rtfsl_flush_all_buffers(void);
int fatop_buff_get_frag(unsigned long current_cluster, unsigned long *pnext_cluster, unsigned long max_length);
int rtfsl_open_path(unsigned char **pathlist,unsigned char *pentryname, struct rtfsl_file *directory_file, struct rtfsl_file *ptarget_file);
int rtfsl_clzero(unsigned long cluster);

int rtfsl_load_file(unsigned char *filename, unsigned long load_address);
void rtfsl_setpath(unsigned char **pathnamearray);
int rtfsl_gfirst(struct rtfsl_dstat *statobj, unsigned char *name);
int rtfsl_gnext(struct rtfsl_dstat *statobj);
void rtfsl_done(struct rtfsl_dstat *statobj);

int rtfsl_alloc_fd(void);
int _rtfsl_open(unsigned char *name,unsigned char attributes);
int rtfsl_open(unsigned char *name);
#define rtfsl_dir_open(N) _rtfsl_open(N,ADIRENT)
int rtfsl_read(int fd, unsigned char *in_buff, int count);
int rtfsl_close(int fd);
long rtfsl_lseek(int fd, long offset, int origin);
int rtfsl_fstat(int fd,  struct rtfsl_statstructure *pstat);
int rtfsl_write(int fd, unsigned char *in_buff, int count);
int rtfsl_flush(int fd);
int rtfsl_flush_info_sec(void);
int rtfsl_create(unsigned char *name, unsigned char attribute);
int rtfsl_mkdir (unsigned char *name);
int rtfsl_rmdir(unsigned char *name);
int _rtfsl_delete(unsigned char *name, unsigned char attribute);
int rtfsl_delete(unsigned char *name);
int rtfsl_rename(unsigned char *name, unsigned char *newname);


unsigned long rtfslfs_cluster_remap(unsigned long cluster,unsigned long value);
int rtfslfs_cluster_map(unsigned long cluster,unsigned long value);
int rtfslfs_dirent_remap(unsigned long sector,unsigned long index, struct rtfsl_dosinode *p_dos_inode,int reading);

#define RTFSLFS_JTEST		1
#define RTFSLFS_JCLEAR		2
#define RTFSLFS_JREAD		3
#define RTFSLFS_JWRITE		4

int rtfslfs_start(void);
int rtfslfs_flush(void);
int rtfslfs_restore(void);
int rtfslfs_sync(void);
int rtfslfs_access_journal(int command);



#if (RTFSL_INCLUDE_FAILSAFE_SUPPORT)
typedef unsigned long treplacement_record[2];
struct rtfsl_failsafe_context {
    /* Current cache */
    int journal_buffer_size;
    int journal_buffer_free;
    unsigned char *journal_buffer;
#define REPLACEMENT_RECORD_SIZE_BYTES 8
#define REPLACEMENT_DOSINODESIZE_SIZE_BYTES 32
    unsigned long *preplacement_record_count; /* The first entry in the  journal is reserved for replacement_record_count:replacement_dosinode_count */
    unsigned long *pcurrent_dosinode_offset; /*  The second entry in the journal is reserved for replacement_record_count:replacement_dosinode_count */
    treplacement_record *preplacement_records;
#define RTFSLFS_WRITE_DIRENTRY  0x1
#define RTFSLFS_MAPPED_CL_FOUND 0x2
    unsigned char flags;
};
#endif

extern const char *dotname;
extern const char *dotdotname;
extern const unsigned char end_name[11];

struct rtfsl_context {
	struct rtfsl_drive current_dr;
	struct rtfsl_file rtfsl_files[RTFSL_CFG_NFILES];
	unsigned long current_buffered_sectors[RTFSL_CFG_NUMBUFFERS];
	unsigned char buffer_pool_agingstack[RTFSL_CFG_NUMBUFFERS];
	unsigned char *pcurrent_buffer_pool;
	unsigned long buffer_pool_dirty;
#if (RTFSL_INCLUDE_SECURE_DIGITAL)
	unsigned long buffer_isfiledata;
#endif
#if (RTFSL_INCLUDE_FAILSAFE_SUPPORT)
	struct rtfsl_failsafe_context rtfsl_failsafe_context;
	unsigned char rtfslfs_sector_buffer[RTFSL_CFG_FSBUFFERSIZEBYTES];
	struct rtfsl_failsafe_context *rtfsl_current_failsafe_context;
#endif
};
extern struct rtfsl_context rtfsl;

extern unsigned char rtfsl_sector_buffer[];

#define rtfsl_buffer_address(H) rtfsl.pcurrent_buffer_pool+((H)*rtfsl.current_dr.bytespsector)


#endif  /* __RTFSLITE__ */
