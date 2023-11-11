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

#ifndef RESOURCE_MANAGER
#  define RESOURCE_MANAGER    1

#include <stdlib.h>       /* _MAX_FNAME, _MAX_PATH                                        */
#include <stdio.h>        /* FILE*                                                        */

#include "omni.h"  /* The configuration options */

//#if !defined(__LISTS_H_INCLUDED)
//#   define LIST void
//#endif /* __LISTS_H_INCLUDED */

// Must comment this out if you're not on windows, but
// the following header stuff really requires it.
#include <windows.h>      /* all this for MessageBox (may move to debug.cpp)              */
#include <io.h>

/* ---------------------------------------------------------------------



/* ----------------------------------------------------------------------

        G E N E R A L   C O M P I L E R   D E F I N I T I O N S

   ---------------------------------------------------------------------- */

#ifdef _DLL_VERSION
#   define RES_EXPORT  CFUNC __declspec (dllexport)
#else
#   define RES_EXPORT  CFUNC
#endif


/* ----------------------------------------------------------------------

        H A S H   T A B L E    O P T I O N S

   ---------------------------------------------------------------------- */

//       for all these values, the 'primer' the better      //

#if( RES_USE_FLAT_MODEL )
#    define HASH_TABLE_SIZE       211      /* needs to hold all the files           */

#    define ARCHIVE_TABLE_SIZE    53       /* size of hash table to start with when 
opening an archive.  since there is no
simple entry in a zip file containing
the number of entries within a zip file,
    and since counting the entries is a
    rather laborious process - this value
    is used as the starting point.  If the
    number of entries exceed this value,
    the hash table size is dynamically
    resized. */
#else /* otherwise, heirarchical */
#    define HASH_TABLE_SIZE       29       /* only needs to hold directory names    */

#    define ARCHIVE_TABLE_SIZE    29       /* if you are using hierarchies, and
you have recursed directory pathnames
into the zip, you can probably use the
same size hash table for archive table
as what you defined as HASH_TABLE_SIZE */
#endif /* RES_USE_FLAT_MODEL */

    /* ----------------------------------------------------------------------

        HASH_TABLE_SIZE is the default size for the main hash table.  The hash
        table will grow if you try to put too many entries into it, but this
        is the starting point.  I try to maintain about 125% hash size vs.
        number of entries, but I only trigger a resize of the hash table when
        you get to 80% hash size vs. number of entries (20% more entries than
        slots).

       ---------------------------------------------------------------------- */

#define HASH_OPTIMAL_RATIO        1.15    /* 1.15 == 115 percent                    */
#define HASH_MINIMAL_RATIO        0.80    /* 0.80 == 80 percent                     */

#define HASH_CONST                26      /* kind of like a seed value              */

#define USE_AFU                   YES     /* hash function used in STTNG:AFU        */
#define USE_SEDGEWICK             NO     /* hash function from Alogrithms in C     */





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


    /* ----- ERROR CODES ----- */

    enum RES_ERRORS
    {
        RES_ERR_FIRST_ERROR = -5000,
        RES_ERR_NO_MEMORY,
        RES_ERR_INCORRECT_PARAMETER,
        RES_ERR_PATH_NOT_FOUND,
        RES_ERR_FILE_SHARING,
        RES_ERR_ILLEGAL_CD,
        RES_ERR_FILE_ALREADY_CLOSED,
        RES_ERR_FILE_NOT_FOUND,
        RES_ERR_CANT_ATTRIB_DIRECTORY,
        RES_ERR_CANT_ATTRIB_FILE,
        RES_ERR_DIRECTORY_EXISTS,
        RES_ERR_IS_NOT_DIRECTORY,
        RES_ERR_COULD_NOT_DELETE,
        RES_ERR_COULD_NOT_CHANGE_DIR,
        RES_ERR_MUST_CREATE_PATH,
        RES_ERR_COULDNT_SPAWN_THREAD,
        RES_ERR_COULDNT_OPEN_FILE,
        RES_ERR_PROBLEM_READING,
        RES_ERR_PROBLEM_WRITING,
        RES_ERR_NO_SYSTEM_PATH,
        RES_ERR_CANT_INTERPRET,
        RES_ERR_UNKNOWN_ARCHIVE,
        RES_ERR_TOO_MANY_FILES,
        RES_ERR_ILLEGAL_FILE_HANDLE,
        RES_ERR_CANT_DELETE_FILE,
        RES_ERR_CANT_OPEN_ARCHIVE,
        RES_ERR_BAD_ARCHIVE,
        RES_ERR_UNKNOWN,
        RES_ERR_ALREADY_IN_PATH,
        RES_ERR_UNKNOWN_WRITE_TO,
        RES_ERR_CANT_WRITE_ARCHIVE,
        RES_ERR_UNSUPPORTED_COMPRESSION,
        RES_ERR_TOO_MANY_DIRECTORIES,
        RES_ERR_LAST_ERROR
    };


/* ----- USER CALLBACKS ----- */

enum RES_CALLBACKS
{
    CALLBACK_UNKNOWN_CD,
    CALLBACK_SWAP_CD,
    CALLBACK_SYNCH_CD,
    CALLBACK_RESYNCH_CD,
    CALLBACK_OPEN_FILE,                  /* for your own debugging                                       */
    CALLBACK_CLOSE_FILE,                 /*   ditto                                                      */
    CALLBACK_READ_FILE,                  /*   ditto                                                      */
    CALLBACK_WRITE_FILE,                 /*   ditto                                                      */
    CALLBACK_ERROR,                      /*   ditto                                                      */
    CALLBACK_FILL_STREAM,                /* to replace my version of _filbuf()                           */

    NUMBER_OF_CALLBACKS
};


/* ----- SYSTEM PATHS ----- */

enum RES_PATHS                          /* enumerated slots for system paths                            */
{
    RES_DIR_NONE,
    RES_DIR_INSTALL,
    RES_DIR_WINDOWS,
    RES_DIR_CURR,
    RES_DIR_CD,
    RES_DIR_HD,
    RES_DIR_TEMP,
    RES_DIR_SAVE,
    RES_DIR_USER1,
    RES_DIR_USER2,
    RES_DIR_USER3,
    RES_DIR_USER4,
    RES_DIR_USER5,
    RES_DIR_USER6,
    RES_DIR_USER7,
    RES_DIR_USER8,
    RES_DIR_USER9,
    RES_DIR_LAST
};

#define FORCE_BIT                (1<<31)


/* ------------------------------------------------------------------------

        D A T A   S T R U C T U R E S

   ------------------------------------------------------------------------ */

typedef unsigned char   UCH;
typedef unsigned long   ULG;

typedef void* RES_archive_hndl;
typedef void* RES_file_hndl;

typedef struct RES_dir
{
    int     current;                    /* used to to index into name array                             */
    int     num_entries;                /* number of filenames found in directory                       */
    char** filenames;                  /* dynamically allocated array of filenames                     */
    char* string_pool;                /* to glob allocations                                          */
    char* string_ptr;                 /* current write ptr into string_pool                           */
    char* name;                       /* directory name                                               */

} RES_dir;

typedef struct RES_stat
{
    int      size;                       /* size of file (bytes)                                         */
    int      csize;                      /* compressed size of file (if in archive)                      */
    int      directory;                  /* location (directory)                                         */
    int      volume;                     /* location (volume)                                            */
    RES_archive_hndl      archive;                    /* archive id                                                   */
    int      attributes;                 /* file attributes                                              */
    int      mode;                       /* open mode _O_RDWR, etc...                                    */

} RES_stat;

/* ------------------------------------------------------------------------

        P R O T O T Y P E S

   ------------------------------------------------------------------------ */

RES_EXPORT int    ResInit(HWND hwnd);
RES_EXPORT void   ResExit(void);

RES_EXPORT void   ResPurge(RES_archive_hndl archive, const char * volume, const int * directory, const char * filename);

RES_EXPORT RES_archive_hndl  ResAttach(const char * attach_point, const char * filename, int replace_flag);
RES_EXPORT void   ResDetach(RES_archive_hndl handle);

RES_EXPORT RES_file_hndl  ResOpenFile(const char * name, int mode);
RES_EXPORT int    ResSizeFile(RES_file_hndl handle);
RES_EXPORT int    ResReadFile(RES_file_hndl handle, void * buffer, size_t count);
RES_EXPORT char * ResLoadFile(const char * filename,  char * buffer, size_t * size);
RES_EXPORT void   ResUnloadFile(char * buffer);
RES_EXPORT int    ResCloseFile(RES_file_hndl handle);
RES_EXPORT size_t ResWriteFile(RES_file_hndl handle, const void * buffer, size_t count);
RES_EXPORT int    ResDeleteFile(const char * name);
RES_EXPORT int    ResModifyFile(const char * name, int flags);
RES_EXPORT long   ResTellFile(RES_file_hndl handle);
RES_EXPORT int    ResSeekFile(RES_file_hndl handle, size_t offset, int origin);
RES_EXPORT int    ResStatusFile(const char * filename, RES_stat * stat_buffer);
RES_EXPORT int    ResExtractFile(const char * dst, const char * src);

RES_EXPORT RES_file_hndl   RES_FOPEN(const char* name, const char* mode);
RES_EXPORT int             RES_FCLOSE(RES_file_hndl file);
RES_EXPORT long            RES_FTELL(RES_file_hndl file);
RES_EXPORT size_t          RES_FREAD(void* buffer, size_t size, size_t num, RES_file_hndl file);
RES_EXPORT size_t          RES_FWRITE(void* buffer, size_t size, size_t num, RES_file_hndl file);
RES_EXPORT int             RES_FSEEK(RES_file_hndl file, long offset, int whence);
RES_EXPORT int             RES_FPRINTF(RES_file_hndl file, const char* fmt, ...);
RES_EXPORT int             RES_FSCANF(RES_file_hndl file, const char* fmt, ...);
RES_EXPORT char*           RES_FGETS(char* buffer, int max_count, RES_file_hndl file);
RES_EXPORT int             RES_FEOF(RES_file_hndl file);

//#  define fsetpos(f,o) fseek(f,o,SEEK_SET)
//#  define rewind(f)    fseek(f,0L,SEEK_SET)

//RES_EXPORT int    ResMakeDirectory(char * pathname);
//RES_EXPORT int    ResDeleteDirectory(char * pathname, int forced);
//RES_EXPORT RES_dir* ResOpenDirectory(char * pathname);
//RES_EXPORT char * ResReadDirectory(RES_dir* dir);
//RES_EXPORT void   ResCloseDirectory(RES_dir* dir);

RES_EXPORT int    ResExistFile(char * name);
RES_EXPORT int    ResExistDirectory(char * pathname);

RES_EXPORT int    ResSetDirectory(const char * pathname);
RES_EXPORT int    ResGetDirectory(char * buffer);
RES_EXPORT int    ResCountDirectory(char * path, struct _finddata_t **file_data);
RES_EXPORT int    ResWhereIs(char * filename, char * path);
RES_EXPORT int    ResWhichCD(void);
RES_EXPORT int    ResWriteTOC(char * filename);

RES_EXPORT PFI    ResSetCallback(int which, PFI func);
RES_EXPORT PFI    ResGetCallback(int which);

RES_EXPORT void   ResAssignPath(int index, char * path);
RES_EXPORT int    ResBuildPathname(int index, char * path_in, char * path_out);

RES_EXPORT int    ResGetPath(int idx, char * buffer);
RES_EXPORT int    ResCreatePath(char * path, int recurse);
RES_EXPORT int    ResAddPath(char * path, int recurse);
RES_EXPORT int    ResGetArchive(RES_archive_hndl handle, char * buffer);

RES_EXPORT int    ResAsynchRead(RES_file_hndl handle, void * buffer, PFV callback);
RES_EXPORT int    ResAsynchWrite(RES_file_hndl handle, void * buffer, PFV callback);

#if( RES_DEBUG_VERSION )
RES_EXPORT void   ResDbg(int on);
RES_EXPORT int    ResDbgLogOpen(char * filename);
RES_EXPORT void   ResDbgLogClose(void);
RES_EXPORT void   ResDbgPrintf(char * msg, ...);
RES_EXPORT void   ResDbgLogPause(int on);
RES_EXPORT void   ResDbgDump(void);
#endif /* RES_DEBUG_VERSION */

#if 0
int ResOpenStream(char *, int);
int ResDefineStream(int, int, PFV);
size_t ResPlayStream(int, int);
size_t ResWriteStream(int, int, size_t);
#endif




/* -------------------------------------------------------------------------------

        C O N V E N I E N C E    D E F I N I T I O N S

   ------------------------------------------------------------------------------- */
//
//#ifndef SEEK_CUR
//#  define SEEK_SET 0
//#  define SEEK_CUR 1
//#  define SEEK_END 2
//#endif /* SEEK_CUR */
//
//enum
//{
//    RES_NOT_FORCED  = 0,
//    RES_FORCED
//};
//
//#define RES_HD                  0x00010000
//#define RES_CD                  0x00020000
//#define RES_NET                 0x00040000
//#define RES_ARCHIVE             0x00080000
//#define RES_FLOPPY              0x00100000

#ifndef _O_APPEND                           /* so you don't need <fcntl.h>                                  */
#  define _O_RDONLY             0x0000      /* open for reading only                                        */
#  define _O_WRONLY             0x0001      /* open for writing only                                        */
#  define _O_RDWR               0x0002      /* open for reading and writing                                 */
#  define _O_APPEND             0x0008      /* writes done at eof                                           */
#  define _O_CREAT              0x0100      /* create and open file                                         */
#  define _O_TRUNC              0x0200      /* open and truncate                                            */
#  define _O_EXCL               0x0400      /* open only if file doesn't already exist                      */
#  define _O_TEXT               0x4000      /* file mode is text (translated)                               */
#  define _O_BINARY             0x8000      /* file mode is binary (untranslated)                           */
#  define _O_RAW                _O_BINARY   /* msvc 2.0 compatability                                       */
#endif   /* _O_APPEND */

#ifndef O_RDONLY /* Yes, there was fcntl.h before Microsoft (even if not ansi) */
#  define O_RDONLY _O_RDONLY   /* and... #define O_RDONLY 0x0000 will complain because MS.SUX */
#  define O_WRONLY _O_WRONLY
#  define O_RDWR _O_RDWR
#  define O_APPEND _O_APPEND
#  define O_CREAT _O_CREAT
#  define O_TRUNC _O_TRUNC
#  define O_EXCL _O_EXCL
#  define O_TEXT _O_TEXT
#  define O_BINARY _O_BINARY
#  define O_RAW _O_BINARY
#endif

#define READ_ONLY_MODE          _O_RDONLY
#define WRITE_ONLY_MODE         _O_WRONLY
#define READ_WRITE_MODE         _O_RDWR
#define APPEND_MODE             _O_APPEND
#define CREATE_MODE             _O_CREAT
#define TRUNCATE_MODE           _O_TRUNC
#define TEXT_MODE               _O_TEXT
#define BINARY_MODE             _O_BINARY

//#define ASCII_BACKSLASH         0x5c
//#define ASCII_FORESLASH         0x2f
//#define ASCII_SPACE             0x20
//#define ASCII_DOT               0x2e
//#define ASCII_STAR              0x2a
//#define ASCII_OPEN_BRACKET      0x5b
//#define ASCII_CLOSE_BRACKET     0x5d
//#define ASCII_COLON             0x3a
//#define ASCII_QUOTE             0x22
//#define ASCII_ASTERISK          ASCII_STAR
//#define ASCII_PERIOD            ASCII_DOT


///* VC++ mutex code is NOT very lightweight.  These macros are for our
//   simple semaphores for blocking hash table resizing while ptrs are
//   exposed */
///* #if( RES_USE_MULTITHREAD ) */
//
//#if( RES_MULTITHREAD )
//#   define  RES_LOCK(a)         {(a)->lock = TRUE;}
//#   define  RES_UNLOCK(a)       {(a)->lock = FALSE;}
//#   define  RES_IS_LOCKED(a)    ((a)->lock == TRUE)
//#   define  RES_WHILE_LOCKED(a) {while((a)->lock);}
//#else
//#   define  RES_LOCK(a)
//#   define  RES_UNLOCK(a)
//#   define  RES_IS_LOCKED(a)    (TRUE == TRUE)
//#   define  RES_WHILE_LOCKED(a)
//#endif /* RES_USE_MULTITHREAD */
//
//#define UNZIP_SLIDE_SIZE        32768
//#define UNZIP_BUFFER_SIZE       2048
//#define INPUTBUFSIZE            20480

#endif /* RESOURCE_MANAGER */
