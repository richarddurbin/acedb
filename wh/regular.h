/*
 *  File regular.h  : header file for libfree.a utility functions
 *  Author: Richard Durbin (rd@sanger.ac.uk)
 *  Copyright (C) J Thierry-Mieg and R Durbin, 1994
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
 * HISTORY:
 * Last edited: Mar 27 17:43 2009 (edgrif)
 * * May 23 12:06 2000 (edgrif): Fix SANgc07771
 * * Apr 26 10:07 1999 (edgrif): Added UTIL_NON_DOUBLE for freedouble check.
 * * Mar 22 14:47 1999 (edgrif): Added messSetMsgInfo, getSystemName calls,
 *              removed messErrorInit.
 * * Mar 18 10:53 1999 (edgrif): Moved some includes to mystdlib.h & put
 *              POSIX constants for invalid ints/floats.
 * * Jan 25 15:56 1999 (edgrif): Added getLogin from session.h
 * * Jan 21 15:27 1999 (edgrif): Added UtArraySize macro to determine array
 *              size at compile time.
 * * Jan 13 11:50 1999 (edgrif): Interface changed for messGetErrorProgram().
 * * Sep  9 16:54 1998 (edgrif): Add messErrorInit decl.
 * * Sep  9 14:31 1998 (edgrif): Add filGetFilename decl.
 * * Aug 20 11:50 1998 (rbrusk): AUL_FUNC_DCL
 * * Sep  3 11:50 1998 (edgrif): Add macro version of messcrash to give
 *              file/line info for debugging.
 * Created: 1991 (rd)
 * CVS info:   $Id: regular.h,v 1.124 2009-03-30 09:09:04 edgrif Exp $
 *-------------------------------------------------------------------
 */

#ifndef DEF_REGULAR_H
#define DEF_REGULAR_H

#include <setjmp.h>
#include <wh/mystdlib.h>				    /* full prototypes of system calls */
#include <wh/aceiotypes.h>				    /* public ACEIN/ACEOUT types */
#include <wh/strsubs.h>					    /* String utilities. */


/**********************************************************************/
/******* be nice to C++ - patch from Steven Ness **********************/
/**********************************************************************/

#ifdef  __cplusplus
extern "C" {
#endif

/**********************************************************************/
/******************** utilities for general use.   ********************/
/**********************************************************************/

/* debugger friendly boolean datatype */
#ifdef FALSE
#undef FALSE
#endif

#ifdef TRUE
#undef TRUE
#endif


#if defined(SGI) || defined (__CYGWIN__)
/* kludge for SGI cc-compiler which complains about mixing enum-types
 * with integer contexts, which is basically what we're doing for BOOL
 */
#define TRUE 1
#define FALSE 0
typedef int BOOL;
#else  /* !SGI */
/* other compilers don't complain, and we we'll be able to see 
 * TRUE/FALSE labels in the debugger, rather than 1 or 0 numbers */
typedef enum {FALSE=0,TRUE=1} BOOL ;
#endif /* !SGI */


/****************************/

#ifndef MAX
#define MAX(a,b) (((a) >= (b)) ? (a) : (b))
#endif
#ifndef MIN
#define MIN(a,b) (((a) <= (b)) ? (a) : (b))
#endif

/**********************************************************************/

typedef unsigned char UCHAR ; /* for convenience */

/* One of the most important typedefs in acedb, really it shouldn't be in    */
/* regular.h which is not at the acedb "level". However, lets leave that,    */
/* and make the best of it. We should have an "undefined" value for KEY for  */
/* applications to use, I'll try zero.                                       */
typedef unsigned int KEY ;
#define KEY_UNDEFINED 0
#define KEY_NULL 0


typedef void (*VoidRoutine)(void) ;
typedef void (*Arg1Routine)(void *arg1) ;

/* magic_t : the type that all magic symbols are declared of.
   They become magic (i.e. unique) by using the pointer
   to that unique symbol, which has been placed somewhere
   in the address space by the compiler */
/* type-magics and associator codes are defined at
   magic_t MYTYPE_MAGIC = "MYTYPE";
   The address of the string is then used as the unique 
   identifier (as type->magic or graphAssXxx-code), and the
   string can be used during debugging */
typedef char* magic_t;


/* Use this macro to determine the size of a declared array.                 */
#define UtArraySize(ARRAY) ((unsigned int) (sizeof(ARRAY) / sizeof(ARRAY[0])))




/********************************************************************/
/************** memory management - memsubs.c ***********************/
/********************************************************************/

#if defined(WIN32) && defined(_DEBUG)
#include <crtdbg.h>
#endif

/* This string is used in acedbCrash() in session.c to check for an 'out of  */
/* memory' failure, we should consider keeping state for this sort of thing. */
/* Not pretty but better than just having the strings embedded willy nilly   */
/* in the code...                                                            */
#define HANDLE_ALLOC_FAILED_STR "Memory allocation failure"

typedef struct _STORE_HANDLE_STRUCT *STORE_HANDLE ; /* opaque outside memsubs.c */
#define NULL_HANDLE ((struct _STORE_HANDLE_STRUCT *)0)


/* internal functions - do not call directly */
STORE_HANDLE handleHandleCreate_dbg (STORE_HANDLE handle,
				     const char *hfname, int hlineno) ;
void *halloc_dbg(int size, STORE_HANDLE handle,
		 const char *hfname, int hlineno) ;
void *handleAlloc_dbg(void (*final)(void *), STORE_HANDLE handle, int size,
		      const char *hfname, int hlineno) ;
char *strnew_dbg(char *old, STORE_HANDLE handle,
		 const char *hfname, int hlineno) ;
void uMessFree (void *cp) ;

void messMemStats(char *title, BOOL newline) ;

/* macros to be called in user-code */

/********* create destroy handles *************/
#define handleCreate() handleHandleCreate_dbg(0, __FILE__, __LINE__)
#define handleHandleCreate(h) handleHandleCreate_dbg(h, __FILE__, __LINE__)

#define handleDestroy(handle) messfree(handle)

/************************************************************/
/*            allocate/free memory chunks                   */
/*                                                          */
/* these called are implemented as macros, so the           */
/* pre-compiler can embed the original positions of the     */
/* memory request. These are then use to report the source  */
/* of the statement in the user-code in case the allocation */
/* failed and the program needs to exit.                    */
/************************************************************/

/* messalloc - memory needs to be free'd by messfree */
#define messalloc(size) halloc_dbg(size, 0, __FILE__, __LINE__)

/* halloc - memory allocated upon given handle,
 * which is freed when handle is killed
 * NOTE, a NULL handle mean that the programmer has to 
 * explicitly messfree the memory. */
#define halloc(size, handle) halloc_dbg(size, handle, __FILE__, __LINE__)

/* handleAlloc - memory allocated upon given handle,
 * which is freed when handle is killed 
 * finalisation function will be called when that happens
 * NOTE, a NULL handle mean that the programmer has to 
 * explicitly messfree the memory.
 * a NULL finalfunc just means that nothing special will happen
 * on destruction of the memory chunk. */
#define handleAlloc(finalfunc, handle, size) handleAlloc_dbg(finalfunc, handle, size,__FILE__,__LINE__)

/* strnew - useful utility to duplicate a NULL-terminated
 * string of characters. The memory which is required
 * will be allocated upon given handle,
 * which is freed when handle is killed.
 * NOTE, a NULL handle mean that the programmer has to 
 * explicitly messfree the string. */
#define strnew(oldstring, handle) strnew_dbg(oldstring, handle,__FILE__,__LINE__)

/* messfree - complements calls to messalloc and
 * in case of a NULL handle also the calls to halloc
 * handleAlloc and strnew */
#define messfree(cp)  ((cp) ? uMessFree((void*)(cp)),(cp)=0,TRUE : FALSE)

/* blockSetFinalise - sets finalisation function for a memory chunk
 * that was allocated by messalloc, halloc, handleAlloc or strnew.
 * the function which receives a generic pointer may take specific
 * actions to be performed when the given memory chunk is destroyed
 * (either explicitly by the programmer or implicitly, when the
 *  parent handle is killed) */
void blockSetFinalise(void *block, void (*final)(void *)) ;

/* handleSetFinalise - sets finalisation functions for the given
 * handle. This function gets called when the given handle
 * is killed. */
void handleSetFinalise(STORE_HANDLE handle, void (*final)(void *), void *arg);

/********* functions to get status and debug info ********/
void handleInfo (STORE_HANDLE handle, int *number, int *size) ;
void messalloccheck (void) ;	/* can be used anywhere - does nothing
				   unless MALLOC_CHECK set in messubs.c */
int messAllocStatus (int *np) ; /* returns number of outstanding allocs
				   *np is total mem if MALLOC_CHECK */


/*****************************************************************************/
/*****************************************************************************/


/* Some routines in this huge mishmash file need to be hived off in a        */
/* separate file, they need to be initialised before use using the below   */
/* call.                                                                   */
void utilsInit(void) ;


/* getLogin returns the real or effective user names but also manipulates    */
/* these globals to record the corresponding uids. This seems not too good,  */
/* especially as these are manipulated in quite a few places in the code.    */
/* Perhaps we should have access routines that guarantee that these uids     */
/* have actually been set to something....                                   */
extern uid_t ruid, euid ;

char *getLogin (BOOL isReal) ;

/* Get the system name that the program is running on.                       */
char *getSystemName(void) ;

/* Get the users home directory. 
 * user_name - e.g. returned by getLogin()
 * result string allocated upon given handle */
char *getUserHomeDir (char *user_name, STORE_HANDLE h);

/* get command line option (and its value if *arg_val is non-NULL)
 * removes option from argument list */
BOOL getCmdLineOption (int *argcp, char **argv,
		       char *arg_name, char **arg_val);

/* For debugging, checks to see if the "-sleep secs" cmd line option was     */
/* specified and then sleeps for the requested number of secs, allows a      */
/* debugger to be attached to the process.                                   */
void checkCmdLineForSleep(int *argcp, char **argv) ;


/* Routine to unlimit resources, currently sets stack and datasize to      */
/* highest possible values. If running non-interactive you may want to     */
/* disable the user abort prompt raised if hard limits are very low.       */
void utUnlimitResources(BOOL allow_user_abort) ;



/*****************************************************************************/
/*****************************************************************************/

/* Some maximum sizes for ints and floats based on POSIX, these are the      */
/* absolute limits of the int/float range, you CAN'T have numbers bigger     */
/* than these for these types.                                               */
/* Note that freeint/float will return values just smaller than these        */
/* values to allow for these limits to be used as special 'too big/little'   */
/* values.                                                                   */
/*                                                                           */
#define UT_INT_MAX             INT_MAX
#define UT_INT_MIN             INT_MIN
#define UT_FLOAT_MINUS_TINY    (-FLT_MIN)
#define UT_FLOAT_MINUS_INF     (-FLT_MAX)
#define UT_FLOAT_PLUS_TINY     FLT_MIN
#define UT_FLOAT_PLUS_INF      FLT_MAX



/**********************************************************************/
/******************** Free package - freesubs.c    ********************/
/**********************************************************************/
/*                                                                    */
/*  for reading/writing to/from files/stdout.                         */
/*                                                                    */
/**********************************************************************/

/* Values that mean "not a valid number", returned by freeint/float/double.  */
/* We use POSIX values to set these, meaning that for floats and ints we get */
/* a slightly reduced range of valid ints/floats.                            */
#define UT_NON_INT      INT_MIN
#define UT_NON_FLOAT   -FLT_MAX
#define UT_NON_DOUBLE  -DBL_MAX
#define UT_NON_BOOL (2)		/* always test b & TRUE , b & NON_BOOL */
#define UT_NON_FLAG (~0)		/* all bits - same as in flag.h */


/* Values used for rounding in a machine independant way the acedb floats */
/* the rounding is maintained using  sprintf (buffer, "%1.7g", obj->curr->n.f) */
#define ACE_FLT_MIN  1e-40 
#define ACE_FLT_RESOLUTION .25e-12

typedef struct freestruct
  { KEY  key ;
    char *text ;
  } FREEOPT ;

void freeinit (void) ;
int freeCurrLevel(void) ;			    /* Returns current level. */
char* freecard (int level) ;	/* 0 if below level (returned by freeset*) */
void freecardback (void) ;  /* goes back one card */
void freeforcecard (char *string);
int  freesettext (char *string, char *parms) ; /* returns level to be used in freecard () */
int  freesetfile (FILE *fil, char *parms) ;
int  freesetpipe (FILE *fil, char *parms) ;  /* will call pclose */
void freeclose(int level) ; /* closes the above */
void freespecial (char *set) ;	/* set of chars to be recognized from "\n;/%\\@$" */
BOOL freeread (FILE *fil) ;	/* returns FALSE if EOF */
int  freeline (FILE *fil) ;	/* line number in file */
int  freestreamline (int level) ;/* line number in stream(level)*/
char *freeword (void) ;

#if defined(WIN32)  /* A variation to correctly parse MS DOS/Windows pathnames */
  char *freepath (void) ;
#else	/* NOT defined(WIN32) */
#define freepath freeword  /* freeword() works fine if not in WIN32 */
#endif	/* defined(WIN32) */

char *freewordcut (char *cutset, char *cutter) ;
void freeback (void) ;		/* goes back one word */
BOOL freeint (int *p) ;
BOOL freefloat (float *p) ;
BOOL freedouble (double *p) ;
BOOL freekey (KEY *kpt, FREEOPT *options) ;
BOOL freekeymatch (char *text, KEY *kpt, FREEOPT *options) ;
void freemenu (void (*proc)(KEY), FREEOPT *options) ;
char *freekey2text (KEY k, FREEOPT *o)  ;  /* Return text corresponding to key */
BOOL freeselect (KEY *kpt, FREEOPT *options) ;
BOOL freelevelselect (int level,
				    KEY *kpt, FREEOPT *options);
void freedump (FREEOPT *options) ;
BOOL freestep (char x) ;
void freenext (void) ;
BOOL freeprompt (char *prompt, char *dfault, char *fmt) ;/* gets a card */
BOOL freecheck (char *fmt) ;	/* checks remaining card fits fmt */
int  freefmtlength (char *fmt) ;
BOOL freequery (char *query) ;
char *freepos (void) ;		/* pointer to present position in card */
char *freeprotect (char* text) ; /* protect so freeword() reads correctly */
char* freeunprotect (char *text) ; /* reverse of protect, removes \ etc */

extern char FREE_UPPER[] ;
#define freeupper(x)	(FREE_UPPER[(x) & 0xff])  /* table is only 128 long */

extern char FREE_LOWER[] ;
#define freelower(x)	(FREE_LOWER[(x) & 0xff])




/**********************************************************************/
/******************** message routines - messubs.c ********************/
/**********************************************************************/


/* 'Internal'/hidden functions, do not call directly. */
void uMessSetErrorOrigin(const char *filename, int line_num) ;
void uMessCrash(char *format, ...) ;

/* External Interface.                                                       */
/* Note that messcrash is a macro and that it makes use of the ',' operator  */
/* in C. This means that the messcrash macro will only produce a single C    */
/* statement and hence does not have to be enclosed in { } brackets, e.g.    */
/*              if (!(this_must_be_true_or_i_have_to_die))                   */
/*                messcrash("I have to die");                                */
/* will become:                                                              */
/* uMessSetErrorOrigin(__FILE__, __LINE__), uMessCrash("I have to die");     */
/*                                                                           */
/* The preprocessor will then substitute the current C-file and line-number  */
/* into the call to uMessSetErrorOrigin, so uMessCrash can give a complete   */
/* report about the code-location that caused the crash.                     */

void messSetMsgInfo(char *progname, char *progversion) ;    /* Record details which will be 
							       output in exit/crash messages. */

char *messGetErrorProgram (void) ; /* Returns the application name or
				      "program_name_not_set" if not set. */

char *messprintf (char *format, ...) ;			    /* sprintf into static string, BEWARE:
							       1) you can overflow the buffer
                                                               2) subsequent functions may overwrite
							          your string !!!! */


/* Better to use hprintf, which allocates store */
char *hprintf(STORE_HANDLE handle, char *format, ...);


void messout (char *format, ...) ;			    /* simple message */
void messerror (char *format, ...) ;			    /* error message and write to log file */
void messdump (char *format, ...) ;			    /* write to log file */
void messExit(char *format, ...) ;			    /* error message, write to log file &
							       exit */
#define messcrash   uMessSetErrorOrigin(__FILE__, __LINE__),uMessCrash
							    /* Exit program on bad error. */

BOOL messQuery (char *text,...) ;			    /* ask yes/no question */
ACEIN messPrompt (char *prompt, char *dfault, char *fmt, STORE_HANDLE handle) ;
	/* ask for data satisfying format
	 * will create new ACEIN upon given handle
	 * get results via aceInCard() and destroy ACEIN when done */

void messbeep (void) ;					    /* make a beep */
BOOL messIsInterruptCalled (void);			    /* return TRUE if an interrupt key has */
							    /* been pressed */
void messResetInterrupt(void) ;			            /* Reset interrupt, e.g. if user has
							       changed their mind.  */
void messStatus (char* text);				    /* called at the beginning of any
							       long-running loop to warn the user
							       that the program will be busy for a while */


/* The messOutCond system, provides a routine that monitors whether none, just the first or
 * all messages are output and whether failure should be on first failure or never. */
typedef enum {MESSERRORCONDMSGS_NONE, MESSERRORCONDMSGS_FIRST, MESSERRORCONDMSGS_ALL} MessErrorCondMsg ;
typedef struct _MessErrorCondContextStruct *MessErrorCondContext ;
MessErrorCondContext messErrorCondCreate(MessErrorCondMsg message_policy,
					 BOOL signal_fail_on_first_error,
					 void *msg_func_data) ;	/* Make context for context, just
								   messfree it when finished. */
BOOL messErrorCond(MessErrorCondContext mess_context, char *format, ...) ;
							    /* Conditional error output. */




/* Checking/asserting code,
 *       use messCheck() for pieces of code that must _always_ be executed
 *       use messAssert() for debug type assertions, n.b. remember that if
 *           ACEDB_NDEBUG is defined the test will _not_ be executed.
 *       use messAbortOnCrash() to force messcrash() to call abort() to get a core dump. */

/* Use to check calls in either of these formats:
 *     messCheck( var = func(arg1, arg2), cond, "rubbish coding") ;
 * or  messCheck( func(arg1, arg2), cond, "just bad luck") ;
 *   - you don't have to assign the function result to a variable if you don't want to.
 *   - "cond" should be a simple equality test like  != 0  or  == NULL  */
#define messCheck(CALL, FAIL_CONDITION, ERR_TEXT)                          \
{                                                                          \
  if ((CALL) FAIL_CONDITION)                                               \
    messcrash("Function failed, result \"%s\" for \"%s ;\", reason: %s",   \
              #FAIL_CONDITION, #CALL, ERR_TEXT) ;                          \
}




/* Use like traditional assert, remember that the test will not be executed if ACEDB_NDEBUG
 * is defined for compilation of the code. */
#ifdef  ACEDB_NDEBUG

# define messAssert(EXPR)  ((void) 0)

#else

#define messAssert(EXPR)                                                                \
((void) ((EXPR) ? 0                                                                    \
          : (messAbortOnCrash(TRUE), messcrash("Failed assertion:  (%s)", #EXPR), 0)))

#endif

#ifdef  ACEDB_NDEBUG

# define messAssertNotReached(ERR_MSG)  ((void) 0)

#else

#define messAssertNotReached(ERR_MSG)                                                                \
  ((void) (messAbortOnCrash(TRUE), messcrash("Unreachable code executed:  %s", ERR_MSG), 0))

#endif

void messAbortOnCrash(BOOL call_abort) ;





/**** registration of callbacks for message routines ****/
/* Use these if you want to augment the behaviour of the message routines,   */
/* you will be passed the formatted message that the original message        */
/* routine received. You should return to the message routine, which will    */
/* then either exit or return to its calling routine.                        */
/* NOTE recursion _will_ be trapped so that messcrash can only be called     */
/* once (if the application callback calls messcrash, then we exit with an   */
/* error message).                                                           */
/*                                                                           */
typedef void (*OutRoutine)(char *message, void *user_pointer);
typedef BOOL (*QueryRoutine)(char *question) ;
typedef ACEIN (*PromptRoutine)(char*, char*, char*, STORE_HANDLE) ;
typedef BOOL (*IsInterruptRoutine)(void) ;
typedef void (*ResetInterruptRoutine)(void) ;

struct messContextStruct {
  OutRoutine user_func;
  void *user_pointer;
};

struct messContextStruct  messOutRegister (struct messContextStruct outContext);
struct messContextStruct  messErrorRegister (struct messContextStruct errorContext);
struct messContextStruct  messExitRegister (struct messContextStruct exitContext);
struct messContextStruct  messCrashRegister (struct messContextStruct crashContext);
struct messContextStruct  messDumpRegister (struct messContextStruct  dumpContext) ;
struct messContextStruct  messStatusRegister (struct messContextStruct  statusContext);
QueryRoutine	   messQueryRegister (QueryRoutine func) ;
PromptRoutine	   messPromptRegister (PromptRoutine func) ;
VoidRoutine	   messBeepRegister (VoidRoutine func) ;
IsInterruptRoutine messIsInterruptRegister (IsInterruptRoutine func) ;
ResetInterruptRoutine messResetInterruptRegister (ResetInterruptRoutine func) ;


/***************************************/

  char* messSysErrorText (void) ; 
	/* wrapped system error message for use in messerror/crash() */

char* messErrnoText (int error) ;
/* as above but take err as arg, instaed of using errno */

int messErrorCount (void);
	/* return numbers of error so far */


/* Turn information/error messsages and logging on/off. */
void messSetShowMsgs(BOOL show_all_messages) ;
void messSetLogMsgs(BOOL log_all_messages) ;



/**** routines to catch exceptions.                                   ****/
/* Use these routines if you want to replace the behaviour of the message    */
/* routines (currently only messerror and messcrash). A typical example is   */
/* dumping of a corrupted database, dumping code may crash for one class,    */
/* but then can be branched back into, to try and dump the next class via    */
/* mechanism.                                                                */
/* NOTE that recursion is not trapped, the application must catch run away   */
/* recursion.                                                                */
  /* if a setjmp() stack context is set using messCatch*() then rather than
     exiting or giving an error message, messCrash() and messError() will
     longjmp() back to the context.
     messCatch*() return the previous value. Use argument = 0 to reset.
     messCaughtMessage() can be called from the jumped-to routine to get
     the error message that would have been printed.
  */
jmp_buf* messCatchError (jmp_buf* ) ;			    /* register replacement for messerror */
jmp_buf* messCatchCrash (jmp_buf* ) ;			    /* register replacement for messcrash */
char*	 messCaughtMessage (void) ;			    /* retrieve original error message
							       formatted by messerror or messcrash. */



/********************************************************************/
/******** growable arrays and flexible stacks - arraysub.c **********/
/********************************************************************/

/* to be included after the declarations of STORE_HANDLE etc. */
#include "array.h"

/********************************************************************/
/************** file opening/closing from filsubs.c *****************/
/********************************************************************/

/* Functions to get various parts of a filename, all return a pointer to   */
/* an internal static, you need to copy the result if you want to reuse    */
/* safely. You should _not_ write into the static buffer.                  */
char *filGetDirname(char *path) ;			    /* returns directory part from a path. */
char *filGetFilename(char *path);			    /* returns filename part of a pathname. */
char *filGetFilenameBase(char *path) ;			    /* returns basename part of a filename */
							    /* within in a pathname.*/
char *filGetExtension(char *path);			    /* returns the file-extension part of
							       a path or file-name */

/* returns an absolute path string for dir in relation to user's CWD */
char *filGetFullPath (char *dir, STORE_HANDLE handle);

char *filName (char *name, char *ending, char *spec) ;
BOOL filCheckName(char *name, char *ending, char *spec);
char *filGetName(char *name, char *ending, 
		 char *spec, STORE_HANDLE handle);

/* determines time since last modification, FALSE if no file */
BOOL  filAge (char *name, char *ending,
	      int *diffYears, int *diffMonths, int *diffDays,
	      int *diffHours, int *diffMins, int *diffSecs);

FILE *filopen (char *name, char *ending, char *spec) ;
FILE *filmail (char *address) ;
void filclose (FILE* fil) ;

BOOL filremove (char *name, char *ending) ;
BOOL filCopyFile (char *existingFile, char *newFile) ;

FILE *filtmpopen (char **nameptr, char *spec) ;
FILE *filTmpOpenWithSuffix (char **nameptr, char *suffix, char *spec);
BOOL filtmpremove (char *name) ;
void filtmpcleanup (void) ;

/* file chooser */
typedef FILE* (*QueryOpenRoutine)(char*, char*, char*, char*, char*) ;
QueryOpenRoutine filQueryOpenRegister (QueryOpenRoutine newRoutine);
		/* allow graphic file choosers to be registered */

FILE *filqueryopen (char *dirname, char *filname,
		    char *ending, char *spec, char *title);

	/* if dirname is given it should be DIR_BUFFER_SIZE long
	     and filname FILE_BUFFER_SIZE long
	   if not given, then default (static) buffers will be used */

/**************************************************/
/*               directory access                 */
/**************************************************/
typedef struct FilDirStruct {
  Array entries;
  STORE_HANDLE handle;
} *FilDir;

/* create a directory listing, 
   the new object can be allocated upon a given handle, such that the
   storage taken by the object is freed upon destruction of that handle.
   the handle-less versions allocate on handle0, their storage has to be
   reclaimed using messfree, which does the appropriate finalisation. */
FilDir filDirHandleCreate (char *dirName,
			   char *ending, 
			   char *spec,
			   STORE_HANDLE handle);
#define filDirCreate(_d, _e, _s) filDirHandleCreate (_d, _e, _s, 0)

FilDir filDirHandleCopy (FilDir fd, STORE_HANDLE handle);
#define filDirCopy(_fd) filDirHandleCopy (_fd, 0)

/* readonly access of the elements of the directory */
#if (defined(ARRAY_CHECK) && !defined(ARRAY_NO_CHECK))
#define filDirEntry(fd,i)	(*(char**)uArrCheck(fd->entries,i))
#else
#define filDirEntry(fd,i)	((*(char**)((fd->entries)->base + (i)*(fd->entries)->size)))
#endif /* ARRAY_CHECK */

#define filDirMax(fd)            ((fd->entries)->max)



/****************** little string routines ************************/

/* Translates newlines into escaped newlines, i.e. '\n' becomes '\''n'     */
char *utCleanNewlines(char *s, STORE_HANDLE handle) ;
/* returns the number of bytes to allocate to safely use the vararg list */
int utPrintfSizeOfArgList (char * formatDescription, va_list marker) ;

/****************** little arithmetic routines ************************/

int utArrondi (float x) ;
int utMainPart (float p) ;
int utMainRoundPart (float p) ;
double utDoubleMainPart (double p) ;


/****************** little conversion routines ************************/

char utInt2Char(int num) ;
BOOL utStr2Int(char *num_str, int *num_out) ;
BOOL utStr2LongInt(char *num_str, long int *num_out) ;


/************* randsubs.c random number generator ******************/

double randfloat (void) ;
double randgauss (void) ;
int randint (void) ;
void randsave (int *num_arr) ;
void randrestore (int *num_arr) ;


/****************** alternative qsort() ************************/
void mSort (void *b, int n, int s, int (*cmp)()) ;


/******************* Unix debugging.  ****************************/
/* put "break invokeDebugger" in your favourite debugger init file */
/* this function is empty, it is defined in messubs.c used in
   messerror, messcrash and when ever you need it. */
void invokeDebugger(void) ;


/* wildcrad matching of strings vs. template */
int regExpMatch (char *text,char *REtemplate) ;


/*******************************************************************/
/************* some WIN32 debugging utilities **********************/

#if defined (WIN32)
#if defined(_DEBUG)
void WinTrace(char *prompt, unsigned long code) ;
void AceASSERT(int condition) ;
void NoMemoryTracking() ;
#endif	/* defined(_DEBUG) */
#endif	/* defined(WIN32) */


#ifdef  __cplusplus
}
#endif


#endif /* defined(DEF_REGULAR_H) */

/************************ End of File ******************************/
