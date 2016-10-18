/*  File: messubs.c
 *  Author: Richard Durbin (rd@mrc-lmb.cam.ac.uk)
 *  Copyright (C) J Thierry-Mieg and R Durbin, 1992
 * -------------------------------------------------------------------
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * -------------------------------------------------------------------
 * This file is part of the ACEDB genome database package, written by
 * 	Richard Durbin (MRC LMB, UK) rd@mrc-lmb.cam.ac.uk, and
 *	Jean Thierry-Mieg (CRBM du CNRS, France) mieg@kaa.cnrs-mop.fr
 *
 * Description: low level messaging/exit routines, encapsulate vararg messages,
 *              *printf, crash handler, calling of application registered exit
 *              routines for cleaning up etc.
 *
 * Exported functions: see regular.h
 *
 * HISTORY:
 * Last edited: Feb 18 13:35 2010 (edgrif)
 * * Aug 12       2004 (mieg) : changed messout etc to infinite elastic buffers
 * * May 23 10:18 2000 (edgrif): Fix SANgc07771
 * * Apr 13 16:10 2000 (edgrif): Attempt to improve messages output when
 *              messcrash recurses, totally rubbish before.
 * * Mar 22 14:41 1999 (edgrif): Replaced messErrorInit with messSetMsgInfo.
 * * Jan 27 09:19 1999 (edgrif): Put small orphan functions in utils.c
 * * Jan 13 11:20 1999 (edgrif): Small correction to Jeans changes to messGetErrorProgram
 *              and messGetErrorFile.
 * * Nov 19 13:26 1998 (edgrif): Removed the test for errorCount and messQuery
 *              in messerror, really the wrong place.
 * * Oct 22 15:26 1998 (edgrif): Replaced strdup's with strnew.
 * * Oct 21 15:07 1998 (edgrif): Removed messErrorCount stuff from graphcon.c
 *              and added to messerror (still not perfect), this was a new.
 *              bug in the message system.
 * * Sep 24 16:47 1998 (edgrif): Remove references to ACEDB in messages,
 *              change messExit prefix to "EXIT: "
 * * Sep 22 14:35 1998 (edgrif): Correct errors in buffer usage by message
 *              outputting routines and message formatting routines.
 * * Sep 11 09:22 1998 (edgrif): Add messExit routine.
 * * Sep  9 16:52 1998 (edgrif): Add a messErrorInit function to allow an
 *              application to register its name for use in crash messages.
 * * Sep  3 11:32 1998 (edgrif): Rationalise strings used as prefixes for
 *              messages. Add support for new messcrash macro to replace
 *              messcrash routine, this includes file/line info. for
 *              debugging (see regular.h for macro def.) and a new
 *              uMessCrash routine.
 * * Aug 25 14:51 1998 (edgrif): Made BUFSIZE enum (shows up in debugger).
 *              Rationalise the use of va_xx calls into a single macro/
 *              function and improve error checking on vsprintf.
 *              messdump was writing into messbuf half way up, I've stopped
 *              this and made two buffers of half the original size, one for
 *              messages and one for messdump.
 * * Aug 21 13:43 1998 (rd): major changes to make clean from NON_GRAPHICS
 *              and ACEDB.  Callbacks can be registered for essentially
 *              all functions.  mess*() versions continue to centralise
 *              handling of ... via stdarg.
 * * Aug 20 17:10 1998 (rd): moved memory handling to memsubs.c
 * * Jul  9 11:54 1998 (edgrif): 
 *              Fixed problem with SunOS not having strerror function, system
 *              is too old to have standard C libraries, have reverted to
 *              referencing sys_errlist for SunOS only.
 *              Also fixed problem with getpwuid in getLogin function, code
 *              did not check return value from getpwuid function.
 * * Jul  7 10:36 1998 (edgrif):
 *      -       Replaced reference to sys_errlist with strerror function.
 * * DON'T KNOW WHO MADE THESE CHANGES...NO RECORD IN HEADER....(edgrif)
 *      -       newformat added for the log file on mess dump.
 *      -       Time, host and pid are now always the first things written.
 *      -       This is for easier checking og the log.wrm with scripts etc.
 *      -       Messquery added for > 50 minor errors to ask if user wants to crash.
 *      -       Made user,pid and host static in messdump.
 * * Dec  3 15:52 1997 (rd)
 * 	-	messout(): defined(_WINDOW) =>!defined(NON_GRAPHIC)
 * * Dec 16 17:26 1996 (srk)
 * * Aug 15 13:29 1996 (srk)
 *	-	WIN32 and MACINTOSH: seteuid() etc. are stub functions
 * * Jun 6 10:50 1996 (rbrusk): compile error fixes
 * * Jun  4 23:31 1996 (rd)
 * Created: Mon Jun 29 14:15:56 1992 (rd)
 * CVS info:   $Id: messubs.c,v 1.127 2010-02-18 16:48:15 edgrif Exp $
 *-------------------------------------------------------------------
 */



#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
/* To get this compile on Solaris 5.8 will require a hack to get round glibs */
/* stupid  #define G_VA_COPY va_copy  this is not ANSI-C...                  */
/* Hopefully systems support will fix glib for us.                           */

#include </usr/include/varargs.h>

#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

#ifdef SGI
#include </usr/include/varargs.h>
#endif

#include <glib.h>
#include <wh/regular.h>
#include <wh/aceio.h>
#include <wh/sigsubs.h>


/***************************************************************/

/* This is horrible...a hack for sunos which is not standard C compliant.    */
/* to allow accessing system library error messages, will disappear....      */
#ifdef SUN
extern const char *sys_errlist[] ;
#endif


/* The messOutCond system */
typedef struct _MessErrorCondContextStruct
  {
    MessErrorCondMsg which_msg ;
    BOOL stop_on_first_failure ;
    void *msg_func_data ;
  } MessErrorCondContextStruct ;




/* This buffer is used only by the routines that OUTPUT a message. Routines  */
/* that format messages into buffers (e.g. messprintf, messSysErrorText)     */
/* have their own buffers. Note that there is a problem here in that this    */
/* buffer can be overflowed, unfortunately because we use vsprintf to do     */
/* our formatting, this can only be detected after the event.                */
/*                                                                           */
/* Constraints on message buffer size - applicable to ALL routines that      */
/* format externally supplied strings.                                       */
/*                                                                           */
/* BUFSIZE:  size of message buffers (messbuf, a global buffer for general   */
/*           message stuff and a private ones in messdump & messprintf).     */
/* PREFIX:   length of message prefix (used to report details such as the    */
/*           file/line info. for where the error occurred.                   */
/* MAINTEXT: space left in buffer is the rest after the prefix and string    */
/*           terminator (NULL) are subtracted.                               */
/* Is there an argument for putting this buffer size in regular.h ??         */
/*                                                                           */
enum {BUFSIZE = 32768, PREFIXSIZE = 1024, MAINTEXTSIZE = BUFSIZE - PREFIXSIZE - 1} ;

static char messbuf[BUFSIZE] ;



/* Some standard defines for titles/text for messages:                       */
/*                                                                           */
#define       ERROR_PREFIX             "ERROR -  "
#define        EXIT_PREFIX              "EXIT -  "
#define       CRASH_PREFIX       "FATAL ERROR (%s, file: %s, line: %d) -  "

#define CRASH_INFO_FORMAT "ruid: %s, euid: %s, nodeid: %s, program: %s, version: %s"
#define CRASH_INFO_DEFAULT "ruid: null, euid: null, nodeid: null, program: null, version: null"

#if defined(MACINTOSH)
#define      SYSERR_FORMAT             "system error %d"
#else
#define      SYSERR_FORMAT             "system error %d - %s"
#endif


/* Macro to format strings using va_xx calls 
 *  since it contains declarations, you must enclose it 
 * at the begining of a {} block and use the resulting
 * char *message
 * in the same block 
 * each of the function using this macro define their own
 * static elastic buffer
 *   - there is no hard limit to the message size
 *   - BUT the buffer is static so the function is non reentrant
 *     and NOT thread safe
 */

#define ACFORMAT(_prefix)  static Stack s = 0 ; \
  int len, len2, prefixLength ; \
  char *message ; \
  va_list ap; \
  s = stackReCreate (s, 500) ; \
\
  va_start(ap, format);\
  len = g_printf_string_upper_bound (format, ap) ;\
  va_end (ap) ;\
\
  prefixLength = _prefix ? strlen(_prefix) : 0 ; \
  stackExtend (s, len + prefixLength + 1) ;\
  if (*_prefix) catText (s, _prefix) ;\
  message = stackText (s, 0) ;\
\
  va_start(ap, format);\
  len2 = g_vsnprintf(message+prefixLength, len, format, ap) ;\
  va_end (ap) ; \
\
  if (len2>len) \
    messcrash("Error in ACFORMAT(), expected to write no more than %d chars, but wrote %d.", len - 1, len2) ;



/* messcrash now reports the file/line no. where the messcrash was issued    */
/* as an aid to debugging. We do this using a static structure which holds   */
/* the information which is filled in by a macro version of messcrash        */
/* (see regular.h), the structure elements are retrieved using access        */
/* functions.                                                                */
/*                                                                           */
/* Originally messGetErrorProgram was an internal function, now it has       */
/* become external and I have made it return an 'unknown' string, this is    */
/* because the result may be used in a printf which will crash if passed a   */
/* null string on some systems.                                              */
/*                                                                           */
typedef struct _MessErrorInfo
  {
  char *progname ;					    /* Name of executable reporting
							       error. */
  char *progversion ;					    /* version of executable. */
  char *real_userid ;					    /* real userid */
  char *effective_userid ;				    /* effective userid */
  char *machineid ;					    /* machineid */
  const char *filename ;				    /* Filename where error reported */
  int line_num ;					    /* Line number of file where error
							       reported. */
  } MessErrorInfo ;

static MessErrorInfo message_L = 
{
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  0
} ;

/* This is where we keep the prefix for exit/crash errors.                   */
static char *errorPrefix_L = NULL ;

/* Keeps a running total of errors so far (incremented whenever messerror is */
/* called).                                                                  */
static int errorCount_L = 0 ;



/***************************************************************/

/* Exception handling routines:                                              */
/*                                                                           */
/* Use these when you want to subvert the action of either messerror or      */
/* messcrash completely (see also "callback" routines below). You are        */
/* completely responsible for any recursion that may occur.                  */
/*                                                                           */
/*                          These allow an application to recover from an    */
/* error or a crash by jumping back to the routine that originally started   */
/* actions that led to the error or crash. A good example of when you would  */
/* want to do this is for dumping a corrupted database, you might be able to */
/* dump individual classes but not the whole database, so you could keep     */
/* jumping back to the dump routine from the messcrash routine and carry on  */
/* dumping until you crash again, then you can jump back and try to carry on */
/* dumping.                                                                  */
/*                                                                           */
/* These are a very poor approximation to C++ exception handling like        */
/* behaviour.                                                                */
/*                                                                           */
static jmp_buf *errorJmpBuf = 0 ;
static jmp_buf *crashJmpBuf = 0 ;


/*****************************************************************************/
/* Message callback functions:                                               */
/*                                                                           */
/* Use these when you want to augment the action of any of the messaging     */
/* routines (messerror etc.) with actions of your own (e.g. graph uses this  */
/* as a mechanism to output messages in windows). This mechanism will trap   */
/* recursion so that your routine and the message routine do not call each   */
/* other endlessly.                                                          */
/*                                                                           */
/* These statics define a call-back for each different type of message 
 * interaction. The user can register its own functions to deal with every 
 * type of message. The messContextStruct defines a function and a generic 
 * pointer. This means that messout can be called context-less from anywahere 
 * in the code, but when the user-defined routine is dispatched it is called
 * with a parameter to tell the user-defined handler function about the
 * context of the message.
 *
 * It would be better if these statics were kept bundled up in a stack
 * so one could push and pop these handler stack with a complete set of
 * handler functions at a time. pooping the stack would save the need
 * to remember previous values to restore old behaviour.
 */
static struct messContextStruct outContext = { NULL, NULL };
static struct messContextStruct errorContext = { NULL, NULL };
static struct messContextStruct exitContext = { NULL, NULL };
static struct messContextStruct crashContext = { NULL, NULL };
static struct messContextStruct dumpContext = { NULL, NULL };
static struct messContextStruct statusContext = { NULL, NULL };
static QueryRoutine	  queryRoutine = 0 ;
static PromptRoutine	  promptRoutine = 0 ;
static VoidRoutine	  beepRoutine = 0 ;
static IsInterruptRoutine isInterruptRoutine = 0 ;
static ResetInterruptRoutine resetInterruptRoutine = 0 ;


/* internal utility routines.                                                */
/*                                                                           */
static int getErrorLine() ;
static const char *getErrorFile() ;
static char* getErrorPrefix(void) ;
static void makeErrorPrefix(MessErrorInfo *messinfo) ;




/* Used by messcrash to decide whether to abort() or exit() on termination.  */
static BOOL mess_abort_G = FALSE ;


/* NOTE that messexit() and messcrash() messages are always shown and logged. */

/* If FALSE then messout() and messerror() messages are not shown, otherwise all messages are shown. */
static BOOL show_all_messages_G = TRUE ;

/* If FALSE then messerror() messages are not logged, otherwise they are. */
static BOOL log_all_messages_G = TRUE ;



/***************************************************************/


/* Callback registering functions.                                           */

struct messContextStruct messOutRegister (struct messContextStruct context)
{
  struct messContextStruct old = outContext;
  outContext = context;

  return old;
} /* messOutRegister */

struct messContextStruct messErrorRegister (struct messContextStruct context)
{  
  struct messContextStruct old = errorContext;
  errorContext = context;

  return old;
} /* messErrorRegister */

struct messContextStruct messExitRegister (struct messContextStruct context)
{  
  struct messContextStruct old = exitContext;
  exitContext = context;

  return old;
} /* messExitRegister */

struct messContextStruct messCrashRegister (struct messContextStruct context)
{  
  struct messContextStruct old = crashContext;
  crashContext = context;

  return old;
} /* messCrashRegister */

struct messContextStruct messDumpRegister (struct messContextStruct context)
{  
  struct messContextStruct old = dumpContext;
  dumpContext = context;

  return old;
} /* messDumpRegister */

struct messContextStruct messStatusRegister (struct messContextStruct context)
{  
  struct messContextStruct old = statusContext;
  statusContext = context;

  return old;
} /* messStatusRegister */

QueryRoutine messQueryRegister (QueryRoutine func)
{ QueryRoutine old = queryRoutine ; queryRoutine = func ; return old ; }

PromptRoutine messPromptRegister (PromptRoutine func)
{ PromptRoutine old = promptRoutine ; promptRoutine = func ; return old ; }

VoidRoutine messBeepRegister (VoidRoutine func)
{ VoidRoutine old = beepRoutine ; beepRoutine = func ; return old ; }

IsInterruptRoutine messIsInterruptRegister (IsInterruptRoutine func)
{ IsInterruptRoutine old = isInterruptRoutine ; isInterruptRoutine = func ; return old ; }

ResetInterruptRoutine messResetInterruptRegister (ResetInterruptRoutine func)
{ ResetInterruptRoutine old = resetInterruptRoutine ; resetInterruptRoutine = func ; return old ; }


/************************************************************/



/*
 * The message output routines.
 */


/* Access function for returning running error total.                        */
int messErrorCount (void)
{
  return errorCount_L ;
}



/* These two routines control display/logging of informational and error messages (see
 * declaration of globals for more information). */
void messSetShowMsgs(BOOL show_all_messages)
{
  show_all_messages_G = show_all_messages ;

  return ;
}

void messSetLogMsgs(BOOL log_all_messages)
{
  log_all_messages_G = log_all_messages ;

  return ;
}




/* For displaying standard informational messages. */
void messout (char *format,...)
{
  if (show_all_messages_G)
    {
      ACFORMAT ("") ;
      if (outContext.user_func)
	(*outContext.user_func)(message, outContext.user_pointer);
      else
	fprintf (stdout, "// %s\n", message) ;
      strncpy (messbuf, message, BUFSIZE) ;
    }

  return ;
}

/************************************************************/


/* Output a non-fatal error message, for all messages a call to messdump is  */
/* made which may result in the message being logged. The single error count */
/* is also incremented so that functions can use this to check how many      */
/* errors have been recorded so far.                                         */
void messerror (char *format, ...)
{
  if (show_all_messages_G || log_all_messages_G)
    {
      /* Contruct the error message, may be needed either by the exception       */
      /* handlers (jmpbuf stuff) or by the application callback routines.        */
      ACFORMAT(ERROR_PREFIX) ;
      /* always increment the error count.                                       */
      ++errorCount_L ;

      invokeDebugger () ;

      /* If application registered an error exception handler routine, jump off  */
      /* to it.                                                                  */
      if (errorJmpBuf)
	longjmp (*errorJmpBuf, 1) ;

      /* Log the message. use %s because the message is not a format ! */
      if (log_all_messages_G)
	messdump("%s", message) ;

      /* Output the message or call the application registered error routine.    */
      if (show_all_messages_G)
	{
	  if (errorContext.user_func)
	    (*errorContext.user_func)(message, errorContext.user_pointer) ;
	  else
	    fprintf (stderr, "// %s\n", message) ;
	}

      strncpy (messbuf, message, BUFSIZE) ;
    }

  return ;
}

/************************************************************/

/* Use this function for errors that while being unrecoverable are not a     */
/* problem with the acedb code, e.g. if the user starts xace without         */
/* specifying a database.                                                    */
/* Note that there errors are logged but that this routine will exit without */
/* any chance to interrupt it (e.g. the crash routine in uMessCrash), this   */
/* could be changed to allow the application to register an exit handler.    */
/*                                                                           */
void messExit (char *format, ...)
{
  /* Format the message string.  */
  ACFORMAT(ERROR_PREFIX) ;
  
  strncpy (messbuf, message, BUFSIZE) ;

  /* call user-defined exit-routine */
  if (exitContext.user_func)
    (*exitContext.user_func)(message, exitContext.user_pointer) ;
  else
    fprintf (stderr, "// %s\n", message);

  exit(EXIT_FAILURE) ;

  return ;			/* Should never get here. */
} /* messExit */


/************************************************************/

/* This is the routine called by the messcrash macro (see regular.h) which   */
/* actually does the message/handling and exit.                              */
/* This routine may encounter errors itself, in which case it will attempt   */
/* to call itself to report the error. To avoid infinite recursion we limit  */
/* this to just one reporting of an internal error and then we abort.        */
/*                                                                           */
/* NOTE that the order in which things happen is very important, don't change*/
/* without understanding what you are doing, you may screw up dumping of     */
/* corrupted databases or something else vital.                              */
/*                                                                           */
void uMessCrash (char *format, ...)
{
  static BOOL called_already = FALSE ;
  static char *prev_called_message = NULL ;
  char prefix[2056] ;
  int rc ;

  /* Contruct the error message, may be needed either by the exception       */
  /* handlers (jmpbuf stuff) or by the application callback routines.        */
  rc = sprintf(prefix, CRASH_PREFIX, getErrorPrefix(), getErrorFile(), getErrorLine()) ;
  if (rc < 0)
    fprintf(stderr, "sprintf failed in messcrash routine, cannot continue.") ;

  invokeDebugger() ;
    
  { /* start a block here to please the declarations in ACFORMAT macro */
    ACFORMAT (prefix) ;

    strncpy (messbuf, message, BUFSIZE) ;

    /* Application may want to keep trying to operate, e.g. to try to dump as  */
    /* much as possible of a corrupted database, so we jump directly to the    */
    /* application registered routine.                                         */
    /* Note, here we are at the mercy of the application, we may recurse for   */
    /* ever, its really up to the application to control this.                 */
    if (crashJmpBuf)
      longjmp(*crashJmpBuf, 1) ;
    
    
    /* From now on, if we recurse it is because the applications crash
     * routine has failed and we really have to exit at this point because
     * the whole application is probably out of control. */
    
    
    /* Check for recursive calls and abort if necessary, we cannot cope with
     * recursion here, we just abort to hopefully produce a core dump. */
    if (called_already) 
      {
	fprintf(stderr, "\n// Fatal internal error:\n"
		"// %s has called messCrash and when messCrash called the %s\n"
		"// clean up routine, this routine called messCrash again.\n"
		"// messCrash will now abort the program as it cannot be terminated cleanly\n",
		message_L.progname, message_L.progname) ;
	
	if (prev_called_message != NULL)
	  fprintf(stderr, "// The original error was:\n"
		  "// %s\n",
		  prev_called_message) ;
	
	/* Hopefully will write core file.                                     */
	abort() ;
      }
    else
      called_already = TRUE ;
    
    
    /* Remember this message in case the user-defined routine calls messcrash  */
    /* again, in this case we have to abort, but we will be able to print out  */
    /* this previously remembered message.                                     */
    prev_called_message = strnew (message, 0) ;
    
    /* call user-defined crash-routine or just output the message. */
    if (crashContext.user_func)
      (*crashContext.user_func)(message, crashContext.user_pointer) ;
    else
      fprintf(stderr, "%s\n", message) ;
    
    if (mess_abort_G)
      abort() ;
    else
      exit(EXIT_FAILURE) ;
  }
    /* Should never get here. */
  return ;
}


/*****************************************************************/

/* If call_abort is TRUE then if messcrash is called it will terminate by
 * calling the system abort() function to take a core dump, otherwise
 * messcrash simply calls exit(). */
void messAbortOnCrash(BOOL call_abort)
{
  mess_abort_G = call_abort ;
  return ;
}


/*****************************************************************/

void messdump (char *format,...)
{
  /* Format the message string.                                              */
  ACFORMAT ("") ;

  /* call user-defined dump-routine */
  if (dumpContext.user_func)
    (*dumpContext.user_func)(message, dumpContext.user_pointer) ;

  return;
} /* messdump */


/************************************************************/

/* Creates a message context (this is opaque to the caller) which must be passed
 * to subsequent calls to messErrorCond(). */
MessErrorCondContext messErrorCondCreate(MessErrorCondMsg message_policy,
					 BOOL signal_fail_on_first_error,
					 void *msg_func_data)
{
  MessErrorCondContext context ;

  context = messalloc(sizeof(MessErrorCondContextStruct)) ;

  context->which_msg = message_policy ;
  context->stop_on_first_failure = signal_fail_on_first_error ;
  context->msg_func_data = msg_func_data ;

  return context ;
}

/* Outputs error messages in the same way as messerror() but according to state
 * data in msg_context. This allows the application to select whether none, only
 * the first or all messages should be shown.
 * The function always returns TRUE unless signal_fail_on_first_error was set to TRUE
 * when the context was created, in which case it returns TRUE the first time its
 * called and FALSE thereafter. This enables an application to stop processing on the
 * first error without having to implmenent the state/logic required all over the place. */
BOOL messErrorCond(MessErrorCondContext msg_context, char *format, ...)
{
  BOOL status ;

  if (msg_context->stop_on_first_failure)
    status = FALSE ;
  else
    status = TRUE ;

  if (msg_context->which_msg == MESSERRORCONDMSGS_FIRST
      || msg_context->which_msg == MESSERRORCONDMSGS_ALL)
    {
      /* Contruct the error message, may be needed either by the exception       */
      /* handlers (jmpbuf stuff) or by the application callback routines.        */
      {
	ACFORMAT (ERROR_PREFIX) ;

	strncpy (messbuf, message, BUFSIZE) ;

	/* Log the message.                                                        */
	messdump("%s", message) ;
	
	/* Output the message or call the application registered error routine.    */
	if (errorContext.user_func)
	  (*errorContext.user_func)(message, errorContext.user_pointer) ;
	else
	  fprintf (stderr, "// %s\n", message) ;
	
	if (msg_context->which_msg == MESSERRORCONDMSGS_FIRST)
	  msg_context->which_msg = MESSERRORCONDMSGS_NONE ;
      }
    }
  return status ;
}



/************************************************************/

void messStatus (char* text)
{
  /* call user-defined status-routine */
  if (statusContext.user_func)
    (*statusContext.user_func)(text, statusContext.user_pointer);
  
  return ;
} /* messStatus */

/************************************************************/

ACEIN messPrompt (char *prompt, char *dfault, char *fmt, 
		  STORE_HANDLE handle)
{ 
  ACEIN fi ;
  
  if (promptRoutine)
    fi = (*promptRoutine)(prompt, dfault, fmt, handle) ;
  else
    /* by default get result from stdin */
    {
      fi = aceInCreateFromStdin(TRUE, "", handle);
      aceInPrompt (fi, messprintf("%s > ", prompt));
      if (!aceInCheck (fi, fmt))
	aceInDestroy (fi);	/* will set fi NULL */
    }

  return fi ;
} /* messPrompt */

/*****************************************************************/

BOOL messQuery (char *format,...)
{ 
  BOOL answer ;

  /* Format the message string.                                              */
  {
    ACFORMAT ("") ;

    strncpy (messbuf, message, BUFSIZE) ;

    if (queryRoutine)
      answer = (*queryRoutine)(message) ;
    else
      answer = freequery (message) ;
  }
  return answer ;
} /* messQuery */

/************************************************************/

void messbeep (void)
{
  if (beepRoutine)
    (*beepRoutine)() ;
  else
    { 
      /* write bell-character to terminal */
      if (isatty ( fileno(stdout) ) !=  0)
	{
	  fprintf (stdout, "%c",0x07);
	  fflush (stdout);
	}
    }

  return;
} /* messbeep */


/************************************************************/

/* Routines to call registered interrupt handlers, used to implement stuff   */
/* like user pressing F4 in xace.                                            */
/*                                                                           */
BOOL messIsInterruptCalled (void)
{
  if (isInterruptRoutine)
    return (*isInterruptRoutine)() ;

  /* unless a routine is registered, we assume no interrupt
     (e.g. F4 keypress in graph-window) has been called */
  return FALSE;
} /* messIsInterruptCalled */

void messResetInterrupt(void)
{
  if (resetInterruptRoutine)
    (*resetInterruptRoutine)() ;

  return ;
}

/************************************************************/



/******* interface to crash/error exception trapping *******/

jmp_buf* messCatchError (jmp_buf* new)
{
  jmp_buf* old = errorJmpBuf ;
  errorJmpBuf = new ;
  return old ;
}

jmp_buf* messCatchCrash (jmp_buf* new)
{
  jmp_buf* old = crashJmpBuf ;
  crashJmpBuf = new ;
  return old ;
}

char* messCaughtMessage (void)
{
  return messbuf ;
}


/* Message formatting routines.                                              */
/*                                                                           */
/*                                                                           */

/* This function writes into its own buffer,  note that subsequent calls     */
/* will overwrite this buffer.                                               */

char *messprintf (char *format, ...)
{
  /* Format the message string.                                              */
  ACFORMAT ("") ;

  return message ;
}


/* Used internally for formatting into a specified buffer.                   */
/* (currently only used as a cover function to enable us to use ACEFORMAT-   */
/* STRING from messSysErrorText)                                             */
static char *printToBuf(char *buffer, unsigned int buflen, char *format, ...)
{
  ACFORMAT ("") ;
  
  if (message && strlen (message) > buflen)
    {
      fprintf (stderr, "printToBuf() : "
	       "the message is longer than the provided buffer length (%d) : %s"
	       , buflen, message) ;
      
      invokeDebugger();
      exit (EXIT_FAILURE);
    }
  strncpy (buffer, message, buflen) ;
  
  return message ;
}



/* Return the string for a given errno from the standard C library.          */
/*                                                                           */
char* messSysErrorText (void)
{
  return messErrnoText(errno);
}
 
char* messErrnoText (int error)
  {
  enum {ERRBUFSIZE = 2000} ;				    /* Should be enough. */
  static char errmess[ERRBUFSIZE] ;
  char *mess ;

#ifdef SUN
  /* horrible hack for Sunos/Macs(?) which are not standard C compliant */
  mess = printToBuf(&errmess[0], ERRBUFSIZE, SYSERR_FORMAT, error, sys_errlist[errno]) ;
#elif defined(MACINTOSH)
  mess = printToBuf(&errmess[0], ERRBUFSIZE, SYSERR_FORMAT, error) ;
#else
  mess = printToBuf(&errmess[0], ERRBUFSIZE, SYSERR_FORMAT, error, strerror(errno)) ;
#endif

  return mess ;
  }



/********************** serious error info. routines. ************************/
/* When the acedb needs to exit because there has been an unrecoverable      */
/* error we want to output the file and line number of the code that         */
/* detected the error and various other bits of information, here are the    */
/* routines that provide the interface for that.                             */
/*                                                                           */

/* Allows the application to set information to be formatted into a serious  */
/* error message.                                                            */
/*                                                                           */
/* The application can set these at any time, this would allow the server    */
/* for example to set the user/machine id for the request it is currently    */
/* processing.                                                               */
/* Note that some things have to be set by the application, the program      */
/* name (given by argv[0]) cannot easily be obtained any other way (you      */
/* can't get it in a portable way from the process information).             */
/*                                                                           */
void messSetMsgInfo(char *progname, char *progversion)
{
  if (progname != NULL)
    message_L.progname = strnew(filGetFilename(progname), 0) ;
  if (progversion != NULL)
    message_L.progversion = strnew(progversion, 0) ;
  message_L.real_userid = strnew(getLogin(TRUE), 0) ;
  message_L.effective_userid = strnew(getLogin(FALSE), 0) ;
  message_L.machineid = strnew(getSystemName(), 0) ;
  
  makeErrorPrefix(&message_L) ;

  return ;
}


/* I don't want to expand this interface, we don't want this message utility */
/* to become the repository of correct userids/machineids etc without        */
/* some careful thought....                                                  */
/*                                                                           */
/* This function always returns a string, saves caller checking for NULL     */
/* string if they want to use the result in a printf.                        */
char *messGetErrorProgram (void)
{
  char *cp = message_L.progname ;

  if (!cp)
    cp = g_get_prgname() ; /* mieg, apr 2004: this call may return NULL */
  if (!cp)
    cp = "unkwon program" ;
  return cp ;
}  


/* This function is called by the messcrash macro which inserts the file and */
/* line information using the __FILE__ & __LINE__ macros.                    */
/*                                                                           */
/* Like all uNNNN routines it SHOULD NOT be called directly.                 */
/*                                                                           */
void uMessSetErrorOrigin(const char *filename, int line_num)
{
  assert(filename != NULL && line_num != 0) ;
  
  /* We take the filename pointer as it is, it came from a 
   * static const char* embedded in the code, which is safe,
   * any strnew etc. would be unsafe, 
   * as we may messcrash for memory trouble */
  message_L.filename = filename;
  
  message_L.line_num = line_num ;

  return ;
}



static const char *getErrorFile (void)
{
  return message_L.filename ;
}  

static int getErrorLine()
{
  return message_L.line_num ;
}  

/* I'm thinking that we should not be calling messCrash anywhere in these    */
/* routines, its tricky because the application could call this routine      */
/* at a time when it needs this message in a window....                      */
/*                                                                           */
/* Constructs the error message prefix, note that this is redone each time   */
/* messSetMsgInfo() is called so that we don't try to do this when an error  */
/* comes along.                                                              */
/*                                                                           */
/* Here is what we try to construct....                                      */
/*
    "FATAL ERROR (ruid: <user>, euid: <user>, nodeid: <machine>, program: <name>, version: <x_xn>, file <name>, line: <num>)
                 ===========================================================================================================
*/
static void makeErrorPrefix(MessErrorInfo *messinfo)
{
  static char *prefix = NULL ;

  if (prefix != NULL)
    {
      messfree(prefix) ;
      prefix = errorPrefix_L = NULL ;
    }

  prefix = hprintf(0, CRASH_INFO_FORMAT,
		   (messinfo->real_userid ? messinfo->real_userid : "null"),
		   (messinfo->effective_userid ? messinfo->effective_userid : "null"),
		   (messinfo->machineid ? messinfo->machineid : "null"),
		   (messinfo->progname ? messinfo->progname : "null"),
		   (messinfo->progversion ? messinfo->progversion : "null")) ;

  errorPrefix_L = prefix ;

  return ;
}


/* Return the error prefix for crashes/exits, if this is still NULL we DO
 * NOT try call makeErrorPrefix() to fill in the prefix because this can
 * cause recursion in messcrash, its up to the application to make sure that
 * it calls messSetMsgInfo() to fill in the information at the right time.
 * We use a predefined string to use minimum memory in a crash situation.    */
static char* getErrorPrefix(void)
{
  if (errorPrefix_L == NULL)
    errorPrefix_L = CRASH_INFO_DEFAULT ;
  
  return errorPrefix_L ;
}


/*****************************************************************************/

/* Utility function, do printf into dynamically allocated memory,
 * allocate the memory on a handle, if provided.
 * Consider this to replace messprintf everywhere....
 */
char *hprintf(STORE_HANDLE handle, char *format, ...)
{
  ACFORMAT ("") ;

  return strnew (message, handle) ;
}
