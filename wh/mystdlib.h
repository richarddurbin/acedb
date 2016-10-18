/*  File: mystdlib.h
 *  Author: Jean Thierry-Mieg (mieg@mrc-lmb.cam.ac.uk)
 *  Copyright (C) J Thierry-Mieg and R Durbin, 1992
 * -------------------------------------------------------------------
 * Acedb is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 * or see the on-line version at http://www.gnu.org/copyleft/gpl.txt
 * -------------------------------------------------------------------
 * This file is part of the ACEDB genome database package, written by
 * 	Richard Durbin (MRC LMB, UK) rd@mrc-lmb.cam.ac.uk, and
 *	Jean Thierry-Mieg (CRBM du CNRS, France) mieg@kaa.cnrs-mop.fr
 *
 * Description:
 *  Prototypes of system calls
 *  One should in principle use stdlib, however on the various machines,
 *  stdlibs do not always agree, I found easier to look by hand 
 *  and copy here my human interpretation of what I need.
 *  Examples of problems are: reservations for multi processor
 *  architectures on some Silicon machines, necessity to define myFile_t on
 *  some machines and not on others etc.
 *              
 * Exported functions:
 *              
 * HISTORY:
 * Last edited: Oct  7 10:35 2010 (edgrif)
 * * Apr  7 14:16 1999 (edgrif): Added MAC def for isascii (see ctype.h).
 * * Mar 22 14:46 1999 (edgrif): Added sys/utsname.h for uname call.
 * * Mar 18 12:00 1999 (edgrif): Added some system headers for POSIX type sizes,
 *              removed incorrectly defined ACEDB_MAX...int, float etc.
 * * Feb  6 14:04 1997 (srk)
 * * Jun 11 16:46 1996 (rbrusk): WIN32 tace fixes
 * * Jun 10 17:46 1996 (rbrusk): strcasecmp etc. back to simple defines...
 * * Jun  9 19:29 1996 (rd)
 * * Jun 5 15:36 1996 (rbrusk): WIN32 port details
 *	-	Added O/S specific pathname syntax token conventions as #defined symbols
 * * Jun 5 10:06 1996 (rbrusk): moved X_OK etc. from filsubs.c for IBM
 * Jun  4 23:33 1996 (rd)
 * * Jun  4 21:19 1996 (rd): WIN32 changes
 *              
 * Created: Fri Jun  5 18:29:09 1992 (mieg)
 * CVS info:   $Id: mystdlib.h,v 1.74 2010-10-07 10:48:32 edgrif Exp $
 *-------------------------------------------------------------------
 */
#ifndef DEF_MYSTDLIB_H
#define DEF_MYSTDLIB_H


/* WARNING: this file has over time become a horrible mess. It was very necessary
 * a long time ago but now needs tidying up. The first move will be to separate
 * whole file into two halves: POSIX compliant compilation and non-POSIX.
 * That way the more modern systems will not have periodic compilation errors. */


/* needed for MAXPATHLEN, MAXHOSTNAMELEN */
#if !defined(WIN32)

#include <sys/param.h>

#ifdef SOLARIS						    /* Solaris 5.5 keeps MAXHOSTNAMELEN here. */
#include <netdb.h>
#endif /* SOLARIS */

#ifndef MAXHOSTNAMELEN
#define MAXHOSTNAMELEN 64	/* value on True64-UNIX */
#endif /* !MAXHOSTNAMELEN */

#endif


#if defined(MSDOS)
  #define O_RDONLY 1
  #define O_WRONLY 2
  #define O_RDWR   4
  #define O_BINARY 0x8000

#else

#if !(defined(MACINTOSH) || defined(WIN32))
#define O_BINARY 0
#endif

#endif


#if defined(ALLIANT) || defined(CONVEX) 
#define O_RDONLY 0
#endif



#if defined(MFC)
/************************ WIN32 stuff ************************/

#include <sys/stat.h> /* for S_IREAD | S_IWRITE */

#define X_OK	0
#define W_OK	2
#define R_OK	4
#define F_OK	X_OK /* if i exist in WIN32, then i might be executable? */

#include <io.h>		/* for access() in dotter.c */
typedef int uid_t ;	/* UNIX/RPC types not currently used in WIN32 */

/*  O/S specific file system pathname conventions:
	general syntax conventions symbolically defined */

/* In WIN32/DOS... */ 

#define PATH_DELIMITER ';'
#define DRIVE_DELIMITER ':'			/* Not used in UNIX */
#define DRIVE_DELIMITER_STR ":"		/* Not used in UNIX */
#define SUBDIR_DELIMITER     '\\'
#define SUBDIR_DELIMITER_STR "\\"
#define EXT_DELIMITER     '.'
#define EXT_DELIMITER_STR "."

char *DosToPosix(char *path) ; /* defined in filsubs.c */
#define UNIX_PATHNAME(z) DosToPosix(z)


/************************ END OF WIN32 stuff ************************/
#else  
/**************** UNIX-like, NOT WIN32 ******************/

#define PATH_DELIMITER ':'
#define SUBDIR_DELIMITER     '/'
#define SUBDIR_DELIMITER_STR "/"
#define EXT_DELIMITER     '.'
#define EXT_DELIMITER_STR "."

#define UNIX_PATHNAME(z) z   /* Already a UNIX filename? */

/************** #endif !defined(WIN32) *************************/
#endif



/* System error headers/assertion testing.                                   */
#include <errno.h>

#ifndef ESUCCESS
#define ESUCCESS 0
#endif /* !ESUCCESS */

#include <assert.h>


/* math stuff */
#include <math.h>
#include <limits.h>
#include <float.h>



/* The next few are designed to determine how the compiler aligns structures,
   not what we can get away with; change only if extreme circumstances */

#define INT_ALIGNMENT (sizeof(struct{char c; int i; }) - sizeof(int))
#define DOUBLE_ALIGNMENT (sizeof(struct {char c; double d; }) - sizeof(double))
#define SHORT_ALIGNMENT (sizeof(struct {char c; short s; }) - sizeof(short))
#define FLOAT_ALIGNMENT (sizeof(struct {char c; float f; }) - sizeof(float))
#define PTR_ALIGNMENT (sizeof(struct {char c; void *p; }) - sizeof(void *))

/* Constants for store alignment */

/* These are defined as follows:
   MALLOC_ALIGNMENT
   Alignment of most restrictive data type, the system malloc will 
   return addresses aligned to this, and we do the same in messalloc.

   STACK_ALIGNMENT 
   Alignment of data objects on a Stack; this should really be
   the same as MALLOC_ALIGNMENT, but for most 32 bit pointer machines
   we align stacks to 4 bytes to save memory.

   STACK_DOUBLE_ALIGNMENT
   Alignment of doubles required on stack, if this is greater than
   STACK_ALIGNMENT, we read and write doubles on a stack by steam.


   Put specific exceptions first, the defaults below should cope 
   with most cases. Oh, one more thing, STACK_ALIGNMENT and
   STACK_DOUBLE ALIGNMENT are used on pre-processor constant 
   expressions so no sizeofs, sorry.
*/


/* 680x0 processors can fix up unaligned accesses so we trade off speed
   against memory usage on a Mac. I have no idea if this is a good
   trade-off, I only program real computers - srk */

#if defined(NEXT) || defined(MACINTOSH) 
#  define STACK_ALIGNMENT 2
#  define STACK_DOUBLE_ALIGNMENT 2
#  define MALLOC_ALIGNMENT 4
#endif

/* Alpha pointers are 8 bytes, so align the stack to that */
/* Duncan, aug 2004 64-bit code for AMD64/Opteron */
#if defined(ALPHA) || defined(__x86_64__) || defined(OPTERON) || defined(ALIGNMENT_64_BIT)
#  define STACK_ALIGNMENT 8
#endif

#if !defined(STACK_ALIGNMENT)
#  define STACK_ALIGNMENT 4
#endif

#if !defined(STACK_DOUBLE_ALIGNMENT)
#  define STACK_DOUBLE_ALIGNMENT 8
#endif

#if !defined(MALLOC_ALIGNMENT) 
#  define MALLOC_ALIGNMENT DOUBLE_ALIGNMENT
#endif




#include <sys/utsname.h>

/* In posix case, just include standard defs
 * on these other machines, one must go around various problems
 * the idea on any new platform we work with is first to
 * assume POSIX
 *  then realize that Pere Noel does not exist
 *  and start coding the unfortunate exceptions
 */ 

#if defined(POSIX) || defined(LINUX) || defined(OPTERON) || \
	defined(SOLARIS) || defined(SGI) || \
	defined(HP) || defined(WIN32) || defined(DARWIN)
/* ********************* POSIX LIKE MACHINES ********************* */

#ifdef MFC
#include <mbctype.h>
#endif /* MFC */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>					    /* this is for va_start/end. */
#include <fcntl.h>
#include <ctype.h>

#include <sys/types.h>
#include <sys/stat.h>


#if !defined(WIN32)
#include <signal.h>
#include <unistd.h>
#endif /* !WIN32 */

#if defined(HP)
#include <sys/unistd.h>
#define seteuid setuid     /* bizare that this is missing on the HP ?? */
#endif /* HP */

typedef size_t mysize_t;
/* typedef fpos_t myoff_t;  why? i remove this on jan 98 to compile on fujitsu */
typedef off_t myoff_t;
typedef mysize_t myFile_t;

#define FIL_BUFFER_SIZE 256
#define DIR_BUFFER_SIZE MAXPATHLEN


/* Specials for windows....not supported any more in fact.... */
#if defined(WIN32)

  /* _MAX_PATH is 260 in WIN32 but each path component can be max. 256 in size */
#undef DIR_BUFFER_SIZE
#define DIR_BUFFER_SIZE   FIL_BUFFER_SIZE
#define MAXPATHLEN        _MAX_PATH

#define popen _popen
#define pclose _pclose

/* rename to actual WIN32 built-in functions
* (rbrusk): this little code generated a "trigraph" error message
* when built in unix with the gcc compiler; however, I don't understand
* why gcc even sees this code, which is #if defined(WIN32)..#endif protected.
* Changing these to macros is problematic in lex4subs.c et al, which expects
* the names as function names (without parentheses.  So, I change them back..
* If the trigraph error message returns, look for another explanation,
* like MSDOS carriage returns, or something? */
#define strcasecmp  _stricmp 
#define strncasecmp  _strnicmp 

#endif /* WIN32 */


/* ********************* END OF POSIX LIKE MACHINES ********************* */

#else

/* ********************* NON POSIX LIKE MACHINES ********************* */



/* local versions of general types */

#if defined(ALLIANT) || defined (DEC) || defined(MAC_AUX) || defined(MACINTOSH)
  typedef unsigned int mysize_t ;
#elif defined(SGI)
  typedef unsigned mysize_t ;
#elif defined(NEXT) || defined(IBM) || defined(MACINTOSH)
  typedef unsigned long mysize_t ;
#else
  typedef int mysize_t ;
#endif


/* stdio */
#include <stdio.h>


/* Type testing (isalpha etc).                                               */
#include <ctype.h>
#if defined(MACINTOSH)
  #define isascii(x) ((x) < 128)
#endif



/* Definition of the file position type */
#if defined(SUN) 
  typedef long    fpos_t;
#endif /* SUN */

#if defined(ALLIANT) || defined(CONVEX) || defined(MAC_AUX) || defined(METROWERKS)
  typedef long myoff_t ;
#else
  typedef fpos_t myoff_t ;
#endif

/* Constants to be used as 3rd argument for "fseek" function */
#if !defined(SGI) && !defined(ALLIANT)
#define SEEK_CUR        1
#define SEEK_END        2
#define SEEK_SET        0
#endif


/* io.h definitions and prototypes */
#ifndef METROWERKS
#include <fcntl.h>
#endif /* !METROWERKS */

#ifdef IBM
#include <sys/param.h>
#include <strings.h>
#endif /* IBM */


#if !defined(MACINTOSH)
#include <sys/types.h>
#include <sys/stat.h>

#include <unistd.h>
#include <time.h>
#endif /* !MACINTOSH */


/* string and memory stuff */

#include <memory.h>
#include <string.h>


/* missing */
#if defined(DEC) || defined(MACINTOSH) || defined (SUN) || defined (NEC)|| defined(HP) || defined(IBM)
/* case-insensitive string comparison */
int     strcasecmp (const char *a, const char *b) ;
int     strncasecmp(const char *s1, const char *s2, mysize_t n);
#endif

#ifndef	__malloc_h
void free (void *block) ;  /* int on SUN, void on SGI etc */
#endif

/* system functions and sorts - simplest to give full prototypes for all */
int      system    (const char *command);
#ifndef IBM
void     exit      (int status); 
#endif
char   * getenv    (const char *name);

#if !defined(NEXT) && !defined(ALPHA) && !defined(FreeBSD) && !defined(DARWIN)
void     qsort     (void *base, mysize_t nelem, mysize_t width,
                    int  (*fcmp)(const void *, const void *)) ;
#endif /* !NEXT or !ALPHA or !FreeBSD or !DARWIN */


#ifndef THINK_C
extern double atof (const char *cp) ; /* I hope ! */
#endif

#ifndef MAXPATHLEN
#define MAXPATHLEN 1024
#endif
#define FIL_BUFFER_SIZE 256
#define DIR_BUFFER_SIZE MAXPATHLEN 


/***************** missing in some stdio.h ****************/
#ifdef SUN
int rename (const char *from, const char *to);
#endif /* SUN */


/************** missing in some unistd.h *******************/
#if defined SUN || defined SOLARIS 
int lockf(int filedes, int request, off_t size );
int gethostname(char *name, int namelen);
#endif /* SOLARIS */


/************* handling of variable-length parameter lists *********/
#include <stdio.h>					    /* should have vsprintf etc. */
#include <stdarg.h>					    /* this is for va_start/end. */

#if !(defined(MACINTOSH)  || defined(SOLARIS) || defined(POSIX) || defined(WIN32))
 int vfprintf (FILE *stream, const char *format, va_list arglist);
 int vprintf  (const char *format, va_list arglist);
#endif	/* !( defined(MACINTOSH)  etc. ) */

#ifdef SUN
 char *vsprintf (char *buffer, const char *format, va_list arglist);
#else
 /* !SUN */
 int vsprintf (char *buffer, const char *format, va_list arglist);
#endif /* !SUN */




#ifdef SUN			/* missing prototypes on SUN */

/*******************************************************************/

int       fclose   (FILE *stream);
int       fflush   (FILE *stream);
int       fgetc    (FILE *stream);
int       ungetc   (int c, FILE *stream) ;
int       _filbuf  (FILE *stream) ;
int       _flsbuf  (unsigned char x, FILE *stream) ;
int       fprintf  (FILE *stream, const char *format, ...);
int       fscanf   (FILE *stream, const char *format, ...);
int       scanf    (const char *format, ...);
int       printf   (const char *format, ...);
int       sscanf   (const char *buffer, const char *format, ...);
int       fgetpos  (FILE *stream, fpos_t *pos);
char    * fgets    (char *s, int n, FILE *stream);
FILE    * fopen    (const char *path, const char *mode);
int       fputc    (int c, FILE *stream);
int       fputs    (const char *s, FILE *stream);
int       fseek    (FILE *stream, long offset, int whence);
int       fsetpos  (FILE *stream, const fpos_t *pos);
long      ftell    (FILE *stream);
mysize_t  fread    (void *ptr, mysize_t size, mysize_t n, FILE *stream);
mysize_t  fwrite   (const void *ptr, mysize_t size, mysize_t n,
                          FILE *stream);
void      perror   (const char *s);
FILE      *popen   (const char *command, const char *type);
int       pclose   (FILE *stream);
void      rewind   (FILE *stream);
void      setbuf   (FILE *stream, char *buf);

/*int       isalpha  (int c); - fails for some reason with "parse error before `+'" */
char      getopt   (int c, char **s1, char *s2);

/* memmove is not included in SunOS libc, bcopy is */
#define memmove(d,s,l) bcopy(s,d,l)
void  bcopy(char *b1, char *b2, int length);

/* for 'bare' calls, in case the storage is destroyed by 
   lower-level libraries not using messalloc */
#include "malloc.h"

/* not defined in SUN's unistd.h */
int setruid(uid_t ruid);
int seteuid(uid_t euid);

/*******************************************************************/

#endif /* SUN */



/* some stdlib.h don't define these exit codes */
#ifndef EXIT_FAILURE
#define EXIT_FAILURE (1)				    /* program exit code for failure. */
#endif

#ifndef EXIT_SUCCESS
#define EXIT_SUCCESS (0)				    /* program exit code for success. */
#endif


/* ********************* END OF NON POSIX LIKE MACHINES ********************* */

#endif



#endif  /* DEF_MYSTDLIB_H */

/***********************************************************/
