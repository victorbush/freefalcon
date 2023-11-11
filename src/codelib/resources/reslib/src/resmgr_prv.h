/* ----------------------------------------------------------------------
   Resource Manager

   Version 2.00                    Released 03/09/97

   Written by Kevin Ray (x301)    (c) 1996 Spectrum Holobyte
   ----------------------------------------------------------------------

   ResInit              - start up the resource manager
   ResExit              - shut down the resource manager

   ResMountCD           - mount the CD
   ResDismountCD        - unmount a previously mounted CD
   ResAttach            - opens an archive file (zip file) and attaches it to a directory point
   ResDetach            - closes an archive file

   ResCheckMedia        - check to see if media in device has been changed

   ResOpenFile          - open a file for read/write
   ResReadFile          - read a file into a buffer
   ResLoadFile          - load an entire file into memory
   ResCloseFile         - closes a previously opened file
   ResWriteFile         - write to an opened file
   ResDeleteFile        - delete a file (only from HD obviously)
   ResModifyFile        - modify the attributes of a file
   ResSeekFile          - seek within an opened file
   ResTellFile          - returns the offset into the file of the current read position
   ResFlushFile         - write any buffer to the opened file
   ResSizeFile          - get the uncompressed size of a file

   ResExistFile         - does a file exist
   ResExistDirectory    - does a directory exist

   ResMakeDirectory     - create a directory
   ResDeleteDirectory   - remove a directory and any held files
   ResOpenDirectory     - open a directory for read (similiar to _findfirst() - same as opendir())
   ResReadDirectory     - read the contents of a directory (similiar to _findnext())
   ResCloseDirectory    - close a directory (similiar to _findclose() - same as closedir())
   ResSetDirectory      - set the current working directory
   ResGetDirectory      - return the current working directory
   ResCountDirectory    - return the number of files found in directory

   ResBuildPathname     - convenience function for concatenating strings to system paths
   ResAssignPath        - name the system paths (optional)
   ResOverride          - override any previously found files with those found in this directory

   ResSetCD             - set the current CD drive volume
   ResGetCD             - return the current CD drive volume

   ResCreatePath        - creates a search path (releases any previous)
   ResAddPath           - adds a directory to the search path
   ResGetPath           - returns the nth entry in the search path

   ResRemovePath        - removes a directory from the search path
   ResRemoveTree        - removes a directory (recursively) from the search path

   ResWhereIs           - returns the location of a file
   ResWhichCD           - returns the current cd number
   ResWriteTOC          - writes the unified table-of-contents file

   ResStatusFile        - returns the status of a file

   ResAsynchRead        - asynchronous read  (context of read thread)
   ResAsynchWrite       - asynchronous write

   ResFOpen             - stdio style streams (fopen)
   ResFClose            - stdio style streams (fclose)
   ResSetBuf            - stdio style streams (setvbuf)

   ResOpenStream        - YET TO BE IMPLEMENTED
   ResDefineStream      -
   ResPlayStream        -  CDI / 3DO style streams
   ResWriteStream       -

   ResDbg               - turn debugging on and off
   ResDbgLogOpen        - open a log file
   ResDbgLogClose       - close a log file
   ResDbgLogPause       - toggle the paused state of event logging
   ResDbgDump           - dump debug statistic

   ---------------------------------------------------------------------- */

#ifndef RESOURCE_MANAGER_PRV
#  define RESOURCE_MANAGER_PRV    1

#include <stdlib.h>       /* _MAX_FNAME, _MAX_PATH                                        */
#include <stdio.h>        /* FILE*                                                        */

#include "omni.h"  /* The configuration options */

#if !defined(__LISTS_H_INCLUDED)
#   define LIST void
#endif /* __LISTS_H_INCLUDED */

   // Must comment this out if you're not on windows, but
   // the following header stuff really requires it.
#include <windows.h>      /* all this for MessageBox (may move to debug.cpp)              */
#include <io.h>

/* ---------------------------------------------------------------------



/* ----------------------------------------------------------------------

        G E N E R A L   C O M P I L E R   D E F I N I T I O N S

   ---------------------------------------------------------------------- */

#define MAX_DIR_DEPTH                   25      /* maximum depth of directory nesting           */
#define MAX_DIRECTORY_DEPTH             25      /* chop one of em                               */
#define MAX_FILENAME                    60      /* maximum length of a filename                 */
   //#define MAX_DIRECTORIES                 500     /* maximum number of directories in search path */
   /* moved to omni.h GFG 3.11.98 */
#define MAX_CD                          2       /* maximum number of cd's allowed               */
#define MAX_READ_SIZE                   32767   /* maximum buffer to read in a single ResRead   */
#define MAX_FILE_HANDLES                256      /* maximum number of open files                 */
#define MAX_DEVICES                     26      /* number of devices to keep track of (A-Z)     */

#define FILENAME_LENGTH                 14      /* for 8.3 naming convention use 14             */

#define STRING_SAFETY_SIZE              0.85    /* safety buffer ratio for string pools         */

/* ----------------------------------------------------------------------

        H A S H   T A B L E    O P T I O N S

   ---------------------------------------------------------------------- */

   //       for all these values, the 'primer' the better      //
//
//#if( RES_USE_FLAT_MODEL )
//#    define HASH_TABLE_SIZE       211      /* needs to hold all the files           */
//
//#    define ARCHIVE_TABLE_SIZE    53       /* size of hash table to start with when 
//opening an archive.  since there is no
//simple entry in a zip file containing
//the number of entries within a zip file,
//    and since counting the entries is a
//    rather laborious process - this value
//    is used as the starting point.  If the
//    number of entries exceed this value,
//    the hash table size is dynamically
//    resized. */
//#else /* otherwise, heirarchical */
//#    define HASH_TABLE_SIZE       29       /* only needs to hold directory names    */
//
//#    define ARCHIVE_TABLE_SIZE    29       /* if you are using hierarchies, and
//you have recursed directory pathnames
//into the zip, you can probably use the
//same size hash table for archive table
//as what you defined as HASH_TABLE_SIZE */
//#endif /* RES_USE_FLAT_MODEL */
//
///* ----------------------------------------------------------------------
//
//    HASH_TABLE_SIZE is the default size for the main hash table.  The hash
//    table will grow if you try to put too many entries into it, but this
//    is the starting point.  I try to maintain about 125% hash size vs.
//    number of entries, but I only trigger a resize of the hash table when
//    you get to 80% hash size vs. number of entries (20% more entries than
//    slots).
//
//   ---------------------------------------------------------------------- */
//
//#define HASH_OPTIMAL_RATIO        1.15    /* 1.15 == 115 percent                    */
//#define HASH_MINIMAL_RATIO        0.80    /* 0.80 == 80 percent                     */
//
//#define HASH_CONST                26      /* kind of like a seed value              */
//
//#define USE_AFU                   YES     /* hash function used in STTNG:AFU        */
//#define USE_SEDGEWICK             NO     /* hash function from Alogrithms in C     */
//




   /* ----------------------------------------------------------------------

           I N T E R F A C E   F U N C T I O N S

      ---------------------------------------------------------------------- */

      //#define RES_DEBUG_STDIO(msg,len)        OutputDebugString(msg)


      /* ------------------------------------------------------------
          RES_DEBUG_STDIO

          Either define with your own message handler, for example:

          #define RES_DEBUG_STDIO(msg,len)    printf(msg)
          #define RES_DEBUG_STDIO(msg,len)    DebugStdio(msg,len)

          or, if you don't want any logging, or only want events
          logged to a file, then don't define RES_DEBUG_STDIO.

          Also, don't define it as ResDbgPrintf or you will
          get infinite recursion.
         ------------------------------------------------------------ */






         /* -------------------------------------------------------------------------------

                 E N U M E R A T I O N   T A B L E S

            ------------------------------------------------------------------------------- */



/* ------------------------------------------------------------------------

        D A T A   S T R U C T U R E S

   ------------------------------------------------------------------------ */

typedef unsigned char   UCH;
typedef unsigned long   ULG;



typedef struct HASH_ENTRY
{
    char* name;                       /* filename    or pathname                                      */
    size_t  offset;                     /* offset within an archive                                     */
    size_t  size;                       /* size of file                                                 */
    size_t  csize;                      /* compressed size of file                                      */
    short   method;                     /* compress method (deflate or store)                           */
    long    file_position;              /* = -1 for loose file                                          */
    struct ARCHIVE*   archive;          /* if from an archive, or NULL  if not                          */
    int     attrib;                     /* file attributes                                              */

    int    directory;                   /* GFG changed from char to int 3/10/98 */
    char    volume;

    void* dir;                        /* if this entry is a directory                                 */

    struct HASH_ENTRY* next;           /* for imperfect hashes    -or- a subdirectory                  */

} HASH_ENTRY;


typedef struct HASH_TABLE
{
    int     table_size;                 /* number of slots in hash table                                */
    int     num_entries;                /* number of entries entered into hash table                    */
    char* name;                       /* name the table (usually the directory name)                  */
    char* ptr_in;                     /* ptr to insert strings                                        */
    char* ptr_end;                    /* end of string space for this allocation                      */
    LIST* str_list;                   /* list of string space allocations                             */


    struct HASH_ENTRY* table;          /* array -table- of hash entries                                */

} HASH_TABLE;


typedef struct COMPRESSED_FILE
{
    UCH* slide;                      /* sliding window                               was: slide      */
    UCH* in_buffer;                  /* input buffer                                 was: inbuf      */
    UCH* in_ptr;                     /* current write-to ptr within in_buffer        was: inptr      */
    int     in_count;                   /*                                              was: incnt      */
    int     in_size;                    /* size of input buffer                         was: inbufsiz   */
    char* out_buffer;                 /* output buffer                                was: outbuf     */
    ULG     out_count;                  /* number of bytes written to output buffer     was: outcnt     */

    unsigned        wp;                 /* Inflate state variable: current position in slide            */
    unsigned long   bb;                 /* Inflate state variable: bit buffer                           */
    unsigned        bk;                 /* Inflate state variable: bits in bit buffer                   */
    long            csize;              /* Inflate state variable:                                      */

    struct ARCHIVE* archive;           /* archive from which this file is extracted                    */

} COMPRESSED_FILE;


typedef struct ARCHIVE
{
    char    name[_MAX_FNAME];         /* filename of the compressed archive                           */
    int     num_entries;                /* number of files within archive                               */
    //int     os_handle;                  /* handle returned by open()    was: zipfd                      */
    FILE*   file_ptr;                   /* FILE pointer if a standard file                              */
    int     length;                     /* was: ziplen                                                  */
    int     start_buffer;               /* was: cur_zipfile_bufstart                                    */
    char    volume;                     /* used to purge during ResDismount()                           */
    char    directory;                  /* used to purge during ResDismount()                           */

    HANDLE  lock;                       /* mutex lock                                                   */

    /* ------------ used temporarily to parse zip hdr ------------- */
    /* renamed to avoid accidentally misusing COMPRESSED_FILE mmbrs */
    UCH* tmp_hold;                   /* was: fold?                                                   */
    UCH* tmp_slide;                  /* sliding window                               was: slide      */
    UCH* tmp_in_buffer;              /* input buffer                                 was: inbuf      */
    UCH* tmp_in_ptr;                 /* current write-to ptr within in_buffer        was: inptr      */
    int     tmp_in_count;               /*                                              was: incnt      */
    int     tmp_in_size;                /* size of input buffer                         was: inbufsiz   */
    UCH* tmp_out_buffer;             /* output buffer                                was: outbuf     */
    int     tmp_out_count;              /* number of bytes written to output buffer     was: outcnt     */
    int     tmp_len;                    /* zip file length                              was: ziplen     */
    int     tmp_bytes_to_read;          /* bytes to read...?                            was: csize      */
    /* ------------ used temporarily to parse zip hdr ------------- */

    LIST* open_list;                  /* list of COMPRESSED_FILE structs pointing back here           */

    struct HASH_TABLE* table;          /* hash table for this archive                                  */

} ARCHIVE;


typedef struct FILE_ENTRY               /* called "filedes" within AFU                                  */
{
    FILE*   file_ptr;                   /* FILE pointer if a standard file                              */
    int     seek_start;
    size_t  current_pos;
    size_t  size;                       /* uncompressed file size                                       */
    size_t  csize;                      /* compressed file size                                         */
    int     attrib;                     /* file attributes                                              */
    int     mode;                       /* mode file was opened in                                      */
    int     location;
    char    device;
    char* data;
    char* filename;
    size_t current_filbuf_pos;
    struct COMPRESSED_FILE* zip;       /* struct for maintaining unzip state data                      */

} FILE_ENTRY;


typedef struct DEVICE_ENTRY
{
    char    letter;                     /* drive letter                                                 */
    char    type;                       /* type of device                                               */
    char    name[32];                   /* volume name of device                                        */
    char    id;                         /* which cd number                                              */

    unsigned long serial;               /* serial number of volume                                      */

} DEVICE_ENTRY;

typedef struct PATH_ENTRY
{
    char* name;                       /* complete path name                                           */
    char    device;                     /* index pointing to specific device                            */
    char    state;                      /* open, mounted, error, etc.                                   */
    char    open_count;                 /* number of open files within directory                        */

} PATH_ENTRY;


/* ------------------------------------------------------------------------

        P R O T O T Y P E S

   ------------------------------------------------------------------------ */



/* -------------------------------------------------------------------------------

        C O N V E N I E N C E    D E F I N I T I O N S

   ------------------------------------------------------------------------------- */

#ifndef SEEK_CUR
#  define SEEK_SET 0
#  define SEEK_CUR 1
#  define SEEK_END 2
#endif /* SEEK_CUR */

enum
{
    RES_NOT_FORCED = 0,
    RES_FORCED
};

#define RES_HD                  0x00010000
#define RES_CD                  0x00020000
#define RES_NET                 0x00040000
#define RES_ARCHIVE             0x00080000
#define RES_FLOPPY              0x00100000

//#ifndef _O_APPEND                           /* so you don't need <fcntl.h>                                  */
//#  define _O_RDONLY             0x0000      /* open for reading only                                        */
//#  define _O_WRONLY             0x0001      /* open for writing only                                        */
//#  define _O_RDWR               0x0002      /* open for reading and writing                                 */
//#  define _O_APPEND             0x0008      /* writes done at eof                                           */
//#  define _O_CREAT              0x0100      /* create and open file                                         */
//#  define _O_TRUNC              0x0200      /* open and truncate                                            */
//#  define _O_EXCL               0x0400      /* open only if file doesn't already exist                      */
//#  define _O_TEXT               0x4000      /* file mode is text (translated)                               */
//#  define _O_BINARY             0x8000      /* file mode is binary (untranslated)                           */
//#  define _O_RAW                _O_BINARY   /* msvc 2.0 compatability                                       */
//#endif   /* _O_APPEND */
//
//#ifndef O_RDONLY /* Yes, there was fcntl.h before Microsoft (even if not ansi) */
//#  define O_RDONLY _O_RDONLY   /* and... #define O_RDONLY 0x0000 will complain because MS.SUX */
//#  define O_WRONLY _O_WRONLY
//#  define O_RDWR _O_RDWR
//#  define O_APPEND _O_APPEND
//#  define O_CREAT _O_CREAT
//#  define O_TRUNC _O_TRUNC
//#  define O_EXCL _O_EXCL
//#  define O_TEXT _O_TEXT
//#  define O_BINARY _O_BINARY
//#  define O_RAW _O_BINARY
//#endif

//#define READ_ONLY_MODE          _O_RDONLY
//#define WRITE_ONLY_MODE         _O_WRONLY
//#define READ_WRITE_MODE         _O_RDWR
//#define APPEND_MODE             _O_APPEND
//#define CREATE_MODE             _O_CREAT
//#define TRUNCATE_MODE           _O_TRUNC
//#define TEXT_MODE               _O_TEXT
//#define BINARY_MODE             _O_BINARY

#define ASCII_BACKSLASH         0x5c
#define ASCII_FORESLASH         0x2f
#define ASCII_SPACE             0x20
#define ASCII_DOT               0x2e
#define ASCII_STAR              0x2a
#define ASCII_OPEN_BRACKET      0x5b
#define ASCII_CLOSE_BRACKET     0x5d
#define ASCII_COLON             0x3a
#define ASCII_QUOTE             0x22
#define ASCII_ASTERISK          ASCII_STAR
#define ASCII_PERIOD            ASCII_DOT


/* VC++ mutex code is NOT very lightweight.  These macros are for our
   simple semaphores for blocking hash table resizing while ptrs are
   exposed */
   /* #if( RES_USE_MULTITHREAD ) */

#if( RES_MULTITHREAD )
#   define  RES_LOCK(a)         {(a)->lock = TRUE;}
#   define  RES_UNLOCK(a)       {(a)->lock = FALSE;}
#   define  RES_IS_LOCKED(a)    ((a)->lock == TRUE)
#   define  RES_WHILE_LOCKED(a) {while((a)->lock);}
#else
#   define  RES_LOCK(a)
#   define  RES_UNLOCK(a)
#   define  RES_IS_LOCKED(a)    (TRUE == TRUE)
#   define  RES_WHILE_LOCKED(a)
#endif /* RES_USE_MULTITHREAD */

#define UNZIP_SLIDE_SIZE        32768
#define UNZIP_BUFFER_SIZE       2048
#define INPUTBUFSIZE            20480

#endif /* RESOURCE_MANAGER_PRV */
