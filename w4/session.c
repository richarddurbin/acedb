/*  File: session.c
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
 *	Jean Thierry-Mieg (CRBM du CNR, France) mieg@kaa.cnrs-mop.fr
 *
 * Description:
 *   Session control, passwords, write access, locks etc.
 * Exported functions:
 * HISTORY:
 * Last edited: Mar 30 14:39 2007 (edgrif)
 * * Jun 20 10:54 2001 (edgrif): Add extra check of blockN.wrm file read/write
 *              access to check for write access permission.
 * * Jul 20 14:46 2000 (edgrif): Change logfile format for intial record for
 *              new user, add standard prefix of time, network_id and PID.
 * * Dec 16 11:23 1999 (edgrif): Added callbacks for server, made models.wrm
 *              test fatal if models.wrm not found.
 * * Sep 29 11:30 1999 (edgrif): Move sync write & nolock options from passwd.wrm
 *              to database.wrm, required moving getOtherDatabaseConfigs() call.
 * * Sep 21 11:54 1999 (edgrif): Added code to read/set sync write option
 *              from passwd.wrm.
 * * Apr 19 12:06 1999 (fw): added // for messages written to stdout
 * * Apr 19 12:06 1999 (fw): set the readlock for (thisSession_num - 1)
 * * Jan 25 16:01 1999 (edgrif): Remove getLogin, now in w1/utils.c
 * * Dec  3 14:43 1998 (edgrif): Change calls to new interface to aceversion.
 * * Nov 26 11:21 1998 (fw): removed waction stuff
 * * Oct 22 11:54 1998 (edgrif): Add action.h for init/close_action()
 *              THERE IS A PROBLEM WITH CLASHES WITH disk.h for BP..sigh.
 * * Oct 21 14:01 1998 (edgrif): Remove use of isGraphics to make graphOut
 *              do a printf, not needed now.
 * * Sep 11 09:56 1998 (rbsk) need strnew for filGetFullPath assigned to ACEDIR
 * * Sep 11 09:56 1998 (edgrif): Replace calls to messcrash with messExit
 *              where the problem is a user error.
 * * Sep 11 09:43 1998 (edgrif): Replace messcrash calls with messExit
 *              where this is appropriate. Remove from messages any mention
 *              of "setenv ACEDB" as this is now deprecated.
 * * Sep  3 11:14 1998 (edgrif): Remove "ACEDB" from fprintf string in
 *              acedbCrash, this is provided by uMessCrash now.
 * * Aug 21 14:49 1998 (rd): moved ruid/euid back here, along with getLogin
 *      and dump/crash handlers
 * * missing date/name
 *      - Moved the write access messages to the locking subroutines to cover
 *      - all uses. And removed time etc which is now automatically done.
 * * Sep  5 10:44 1997 (rd)
 *	-	WIN32 runs sessionFilName() like UNIX when defined(NON_GRAPHIC)
 *		WinTace user should specify ACEDB directory on the commandline?
 * * Jun  9 14:50 1996 (rd)
 *	-	extern uid_t euid, ruid moved to session.h
 * * Jun  6 17:41 1996 (rd)
 * * Jun  3 19:00 1996 (rd)
 * * Feb  2 11:46 1992 (mieg): interactive session control
 * * Jan 24 11:20 1992 (mieg): session fusion.
 * Created: Fri Jan 24 11:19:27 1992 (mieg)
 * CVS info:   $Id: session.c,v 1.270 2007-04-02 09:43:27 edgrif Exp $
 *-------------------------------------------------------------------
 */

/* $Id: session.c,v 1.270 2007-04-02 09:43:27 edgrif Exp $ */
/* #define CHRONO  define it to get global chrono on whole session */

#define DEF_BP /* avoid typedefing it as void* in disk.h */
typedef struct block *BLOCKP ;
typedef BLOCKP BP ;

#include <wh/acedb.h>

#if defined(__CYGWIN__)
#include <windows.h> 
#endif

#include <wh/mytime.h>
#include <signal.h>           /* for autosave */

#include <wh/aceio.h>
#include <wh/bindex.h>
#include <wh/lex.h>
#include <wh/freeout.h>		/* for freeOutInit */
#include <whooks/systags.h>
#include <whooks/sysclass.h>
#include <wh/byteswap.h>		/* for calls in swapSuperBlock() */
#include <wh/block.h>
#include <wh/session.h>
#include <wh/a.h>
#include <wh/bs.h>
#include <wh/disk.h>
#include <wh/disk_.h>   /* defines BLOCKHEADER */
#include <wh/session_.h>		/* uses BLOCKHEADER */
#include <wh/lex_sess_.h>		/* lexSessionStart etc.. */
#include <wh/pick.h>
#include <wh/pref.h>		/* prefReadDBPrefs() */
#include <wh/model.h>		/* for readModels() */
#include <wh/status.h>		/* for tStatus */
#include <wh/help.h>		/* for helpSetDir during initilisation */
#include <wh/dbpath.h>
#include <wh/log.h>
#include <wh/chrono.h>


/* Stub functions for user ID's in non-multiuser systems
   Note: A WIN32 implementation for Windows NT may be possible */
#if defined(MACINTOSH)

void	seteuid(uid_t uid) { return ; }
uid_t	getuid() { return 1 ; }
uid_t	geteuid() { return 1 ; }
int	gethostname(char *b, int l) { *b = 0; }
typedef int pid_t;
pid_t	getpid (void) { return 0; }

#endif /* not MAC */


#if defined(FreeBSD)
#define O_SYNC O_FSYNC  /* O_SYNC may be XOPEN thing +gg */
#endif /* FreeBSD */


static void closeSession(BOOL doSave) ;


/* One day this should hold all of the below globals....and should be passed into
 * session calls as context. */
static SessionContext session_context_G = NULL ;


/************************************************************/
/* globals declared in session.h */


SESSION thisSession ;
BOOL	swapData = FALSE ;


VoidRoutine messcrashroutine = simpleCrashRoutine;
/* The meaning of this function has changed now.
   it is now only used for acedb code, where acedbCrash()
   is messCrashRegister'd with the message package.
   keep using this name for now because used frequently in code */

/* By default (simpleCrashRoutine) will be close down in style,
   but not save the database.
   A user defined routine may check write access and save
   if required.. (Should always release locks). */

/************************************************************/
/* local globals                                                             */


/* A section of database configuration variables that can be overridden from */
/* wspec/database.wrm                                                        */

/* number of sessions to keep living before a session expires
    (if non-permanent and not readlocked) Default is 2, must be > 1 */
enum {SESSKEEPALIVE_MIN = 2, SESSKEEPALIVE_DEF = 2} ;
static int sessionKeepAlive = SESSKEEPALIVE_DEF ;	    /* init'd in sessionInit */


enum {RDLCKTIMEOUT_MIN = 1, RDLCKTIMEOUT_DEF = 8} ;
static int readlockTimeout = RDLCKTIMEOUT_DEF ;		    /* readlock files older than that
							     * are removed and ignored */

/* suppressLock - indicates whether the file-locking system lockf(3)
 *                should be avoided or not.
 *                The default depends on OS type, but it can be
 *                set per-database by inserting the word NOLOCK
 *                in wspec/database.wrm */
#if defined(MACINTOSH) || defined(__CYGWIN__)
 
#define F_ULOCK 0
#define F_TLOCK 0

static BOOL suppressLock = TRUE ;
/* don't use the lockf system, This fine on win & mac even without
   NOLOCK for this database, because it is unlikely for a Unix program
   (which uses lockf) to access a database on a PC or Mac */

/* stub.. */
BOOL lockf ( int lockFD, int mode1, int mode2 ) { return 0 ; }

#else  /* UNIX */

static BOOL suppressLock = FALSE ; /* do use lockf */

#endif /* not MAC */

/* If TRUE then all acedb writes to the database will be with O_SYNC which   */
/* guarantees the data has been written to disk before returning, BUT is     */
/* VERY slow. (n.b. this need not be a global static but is placed here so   */
/* all the database configuration variables are together.)                   */
static BOOL syncWrite = FALSE ;

/* If TRUE then we stop old, non-object-timestamp-aware code from accessing 
   the database. This ensures that an object cannot be updated without its 
   timestamp being updated also. This is a hack, we should really increment
   the major release number, but it is only required for a small number of
   databases, so we don't want to obsolete all of them.
*/
static BOOL enforceTimestamps = FALSE;


/* The string to use as the "name" of this session user. */
static char *userSessionUser = NULL ;


/* This is a title that can be given to the database and will override that  */
/* given in wspec/displays.wrm for the main window. Really the main window   */
/* title should be disabled.                                                 */
static char *db_title_G = NULL ;

/* If this is TRUE then any users who are in the unix group that owns the    */
/* wspec/models.wrm will be allowed to initialise the database. Uncomment    */
/* the GROUP_ADMIN option in wspec/database.wrm to activate this.            */
static BOOL group_admin_G = FALSE ;


/* end of database configuration variables.                                  */


/* some write access and save behaviour controlling variables, they may
 * need rationalising..... */
static BOOL dropWriteAccess = FALSE ;	/* once TRUE, write access
				   is forbidden and can't be regained
				   this is an odd behaviour */

static int saveMagic = 0 ;				    /* read/write verification seems
							       always to be zero currently ??? */

static BOOL writeAccess = FALSE ;			    /* Does process have write access
							       currently ? */

static BOOL saveDB_G = TRUE ;				    /* Even if the process can get write
							       access this flag if FALSE prevents
							       any saving. */


static BLOCK theSuperBlock ;

static BOOL debug = FALSE;	/* more log-file output */

static char *messcrashbuf;	/* a memory buffer that is
				   allocated at initialization time
				   and free'd when acedb crashes, to
				   gain some extra memory for the crash
				   recovery procedures */


/* Logfile state, sometimes we run without being able to log, this is        */
/* allowed for databases where we don't have write permission in the         */
/* database directory.                                                       */
static ACEOUT logfile_out = NULL;
static BOOL   logfile_possible = FALSE;
static BOOL   logfile_request_new_G = FALSE ;


/* graphical versions of acedb may need to have special routines called at   */
/* various points during session handling.                                   */
static GraphSessionCB graphCBs_G = NULL ;

/* Boolean forceInit_G indicates, if TRUE, that the database 
** is to be initialised without further prompting
*/
static BOOL forceInit_G = FALSE;

/************************************************************/
/* The server needs to perform some special actions on initialisation/       */
/* termination and does so through callbacks.                                */
static ServerSessionCB serverCBs_G = NULL ;


/************************************************************/


/* if set, externalServer will be called once on every new object, and
   always in query(0,..) and in grep 
   Registration of this pointer done via xCientInit
*/

KEYSET (*externalServer) (KEY key, char *query, char *grep, BOOL getNeighbours) ;
void   (*externalSaver) (KEY key, int action) ;
void   (*externalAlias) (KEY key, char *oldname, BOOL keepOldName) ;

void   (*externalCommit) (void) ;
BOOL   (*externalServerState) (int nn) ;
char*  (*externalServerName) (void) ;

/************************************************************/


static char *getConfiguredDatabaseName (void) ;
static void getOtherDatabaseConfigs(int *keepAlive, int *timeOut,
				    BOOL *suppressLock, BOOL *syncWrite,
				    BOOL *enforceTimestamps, char **db_title,
				    BOOL *group_admin) ;
static BOOL blocksWriteable(BOOL report_errors) ;

static void sessionDoInit (void) ; /* session.c's own initialisation */
static int  sessionFindSon (Array sessionTree, KEY father, KEY *sonp) ;
static BOOL sessionDatabaseWritable (BOOL doTalk);
static BOOL sessionCheckUserName (BOOL doTalk);

static BOOL readlockCreateDir (void);
static BOOL readlockCreateFile (void);
static BOOL readlockUpdateFile (void); /* increase thisSession.session before calling */
static Array readlockGetSessionNumbers (BOOL isReport);

static BOOL acedbStartNewLogFile(BOOL just_reopen, char *old_log_name, char *log_msg,
				 BOOL new_session) ;
static void acedbLog (char *text, void *user_pointer);
static void acedbCloseLog(aceLogExitType exit_type) ;

static void acedbExit (char *mesg_buf, void *user_pointer);
static void acedbCrash (char *mesg_buf, void *user_pointer);

static BOOL checkFileGroupPerm(struct stat *passwd_stat) ;

/************************************************************/

static VoidRoutine writeAccessChangeRoutine = 0;

/************************************************************/

/* this mechanism is used to register a function that redraws the 
   window layout or changes menus etc. when the write access changes */


VoidRoutine writeAccessChangeRegister (VoidRoutine func)
     /* call with NULL to enquire function */
{ 
  VoidRoutine old = writeAccessChangeRoutine ; 
  if (func)
    writeAccessChangeRoutine = func ; 
  return old ;
} /* writeAccessChangeRegister */

void writeAccessChangeUnRegister (void)
{ 
  writeAccessChangeRoutine = NULL ; 
  return;
} /* writeAccessChangeUnRegister */

/*************************************************************/

/* this mechanism is used to reflect changes in the session structure
   in a graphical representation of session objects */

static VoidRoutine sessionChangeRoutine = 0;

VoidRoutine sessionChangeRegister (VoidRoutine func)
{ 
  VoidRoutine old = sessionChangeRoutine ; 
  sessionChangeRoutine = func ; 
  return old ;
} /* sessionChangeRegister */

/*************************************************************/

#ifndef ACEDB5
#ifdef ACEDB4
void swapSuperBlock(BLOCKP bp)	/* used also by disknew.c */
{ bp->h.disk = swapDISK(bp->h.disk);
  bp->h.nextdisk = swapDISK(bp->h.nextdisk);
  bp->h.session = swapInt(bp->h.session);
  bp->h.key = swapKEY(bp->h.key);
  bp->gAddress = swapDISK(bp->gAddress);
  bp->mainRelease = swapInt(bp->mainRelease);
  bp->subDataRelease = swapInt(bp->subDataRelease);
  bp->subCodeRelease = swapInt(bp->subCodeRelease);
  bp->alternateMainRelease = swapInt(bp->alternateMainRelease);
} /* swapSuperBlock */
#endif
#endif


/*************************************************************/

/* user-defined routine for messExit -
 * perform acedb-specific tasks when we have to exit
 * because of some error.
 * NOTE, that this exit is not as severe as acedbCrash
 * it merely means that acedb is unable to continue. */
static void acedbExit (char *mesg_buf, void *user_pointer)
{
  /* make copy of message to prevent mesg_buf tangle */
  char *text_copy = strnew(mesg_buf, 0);

  /* output the exit-message to the user
   * uuugggghhh, unfortunately this call to messout subverts the principle
   * that exit messages should _not_ go to the scrolled message window.
   * So before doing them I make sure we switch back to popup windows. In
   * the giface version this call does nothing. */
  if (graphCBs_G)
    graphCBs_G->exit_msg_cb(FALSE) ;
  messout(text_copy);					    /* See server code below. */

  messdump (text_copy);

  /* the messcrashroutine should call aceQuit, to clean up,
     it may give the user a chance to save their work so far */
  if (messcrashroutine)
    {
      VoidRoutine the_routine = messcrashroutine ;
      messcrashroutine = 0;	/* avoids blind recursion */
      (*the_routine)();		/* call user cleanup routine */
    }

  acedbCloseLog(ACELOG_ABNORMAL_EXIT) ;

  /* Give the server a chance to clean up/set lock files before we exit.    
   * Note that we do not pass the exit message as the second parameter because
   * the messout() above has already dumped the message, if you remove the messout() 
   * above you should pass the message in here instead. */
  if (serverCBs_G)
    serverCBs_G->exit.funcs.mesgfunc(serverCBs_G->exit.data, NULL) ;

  messfree(text_copy) ;

  return;			/* give control back to messExit to
				 * terminate the program */
} /* acedbExit */

/************************************************************/

static void acedbCrash (char *mesg_buf, void *user_pointer)
     /* user-defined routine for messcrash -
      * perform acedb-specific tasks when we have to crash out
      * in a ball of flames because of a FATAL ERROR.
      * This is traditionally used to crash out for failed
      * assertions, as the crash location in the code will be reported.
      */
{
  char *text_copy;

  if (messcrashbuf)
    free (messcrashbuf) ;	/* frees up space that might be useful */

  /* copy message-text for messout to avoid mesg_buf tangle */
  /* NOTE, don't use messalloc & Co, to avoid using more code 
   * that could potentially fail */
  text_copy = malloc(strlen(mesg_buf) + 1);
  if (!text_copy)
    {
      /* we're in big trouble */
      fprintf (stderr, "fatal memory allocation error in acedbCrash() termination routine, abort");

      /* Hopefully will write a core file.                                   */
      abort() ;
    }
  strcpy (text_copy, mesg_buf);	


  /***** output crash-message to user and to log-file ****/

  /* uuugggghhh, unfortunately these calls to messout subvert the principle  */
  /* that crash messages should _not_ go to the scrolled message window.     */
  /* So before doing them I make sure we switch back to popup windows. In    */
  /* the giface version this call does nothing.                              */
  if (graphCBs_G)
    graphCBs_G->exit_msg_cb(FALSE) ;
  messout (text_copy);
  messout ("Sorry for the crash, please report it, "
	   "if it is reproducible\n") ;

  messdump (text_copy);


  /********* if memory failure report usage info **************/

  if (strstr (text_copy,  HANDLE_ALLOC_FAILED_STR) != NULL)
    { 
      if (logfile_out)
	{
	  tStatus (logfile_out);
	  aceOutFlush (logfile_out);
	}
      else
	/* e.g. if can't write database/ directory */
	{
	  ACEOUT status_out = aceOutCreateToStderr (0);
	  tStatus (status_out) ;
	  aceOutDestroy (status_out);
	}
    }


  /* the messcrashroutine should call aceQuit, to clean up,
     it may give the user a chance to save their work so far */
  if (messcrashroutine)
    {
      VoidRoutine the_routine = messcrashroutine ;
      messcrashroutine = 0;	/* avoids blind recursion */
      (*the_routine)();		/* call user cleanup routine */
    }

  acedbCloseLog(ACELOG_CRASH_EXIT) ;

  /* Give the server a chance to clean up/set lock files before we exit.     */
  if (serverCBs_G)
    serverCBs_G->crash.funcs.mesgfunc(serverCBs_G->crash.data, text_copy) ;

  free(text_copy) ;

  return;
} /* acedbCrash */


/************************************************************/
/*                                                                           */
/* acedb log file routines, used for the general acedb log file (log.wrm).   */
/*                                                                           */


/* Start a new log file at request of user, probably because old one is      */
/* getting too big.                                                          */
/* if log_file_name is NULL then old log is called "log.<ace-time>.wrm"      */
/*                                                                           */
BOOL sessionNewLogFile(char *old_log_name)
{
  BOOL result = FALSE ;

  result = acedbStartNewLogFile(FALSE, old_log_name, NULL, FALSE) ;

  return result ;
}

/* Request that new log file be started next time a logging entry is made,   */
/* this routine can be called from a signal handler so that an ace server    */
/* for instance can be sent a signal to start a new log.                     */
/*                                                                           */
void sessionRequestNewLogFile(void)
{
  logfile_request_new_G = TRUE ;

  return ;
}

/* Do a straight close/reopen of the logfile, can be used in conjunction     */
/* with the "rotatelogs" script found on some unix versions (e.g. linux).    */
/*                                                                           */
BOOL sessionReopenLogFile(void)
{
  BOOL result = FALSE ;

  result = acedbStartNewLogFile(TRUE, NULL, NULL, FALSE) ;

  return result ;
}



/* Rename the existing log file and start a new one. Gets called either when */
/* requested by user or when database is bootstrapped (start new log because */
/* old one is really only applicable to previous incarnation of database.    */
/*                                                                           */
  /*************************************************************
   * The messages in logfiles only really apply to the database
   * in its current state, if the database is being rebuilt the
   * previous log becomes inappropriate.
   * The previous log is therefore moved to a backup location,
   * and a new logfile is started at this point (SANgc02773) */
/* if log_file_name is NULL then old log is called "log.<ace-time>.wrm"      */
/* if log_msg is non-NULL then its written to end of old log.                */
/*                                                                           */
static BOOL acedbStartNewLogFile(BOOL just_reopen, char *old_log_name, char *log_msg, BOOL new_session)
{
  BOOL result = FALSE ;

  if (logfile_possible)
    {
      if (just_reopen)
	{
	  result = reopenAceLogFile(&logfile_out) ;
	}
      else
	{
	  BOOL timestamp = FALSE ;
	  char *log_name = "log" ;
	  ACEOUT new_logfile = NULL ;

	  if (old_log_name)
	    {
	      log_name = old_log_name ;
	    }
	  else
	    timestamp = TRUE ;

	  logfile_possible = FALSE;			    /* Prevent logging by below calls. */
	  if (logfile_out)
	    {
	      if ((new_logfile = startNewAceLogFile(logfile_out, log_name, timestamp,
						    log_msg, new_session)))
		result = TRUE ;
	    }
	  else
	    {
	      result = openAceLogFile("log", &new_logfile, new_session) ;
	    }

	  if (result)
	    logfile_out = new_logfile ;
	}

      if (result)
	{
	  logfile_possible = TRUE ;
	}
    }

  return result ;
}


/* registered to be the messDumpRoutine for messdump
 * PARAMETERS:
 *   text : the message to be logged
 *      may be NULL, if we just want to init the logfile
 *      but not actually log a message
 *   user_pointer : unused */
static void acedbLog (char *text, void *user_pointer_unused)
{

  if (!logfile_possible)
    return ;

  /* open the file - establish status */
  if (!logfile_out)
    {
      if (!(openAceLogFile("log", &logfile_out, TRUE)))
	logfile_possible = FALSE;
      else
	logfile_possible = TRUE;
    }

  /* A signal handler may have asked for a new log to be started...          */
  if (logfile_request_new_G && logfile_out)
    {
      logfile_request_new_G = FALSE ;			    /* Must come first to prevent possible
							       recursion between any messXXX and
							       here. */
      if (!acedbStartNewLogFile(FALSE, NULL, NULL, FALSE))
	{
	  messdump("Could not honour requested switch to new log file.") ;
	}
    }


  /* Log the message if ...                                                  */
  if (text != NULL &&		/* if we're not initialising */
      logfile_out != NULL)	/* if we have a log-file */
    {
      appendAceLogFile(logfile_out, text) ;
    }

  return;
} /* acedbLog */


/* Close the log file and put in a closing message for the session.          */
/*                                                                           */
static void acedbCloseLog(aceLogExitType exit_type)
{
  if (logfile_out)
    {
      closeAceLogFile(logfile_out, exit_type) ;

      logfile_out = NULL ;
      logfile_possible = FALSE;
    }

  return ;
}


/************************************************************/
/*                                                                           */
/* session callback routine registration, needed for allowing other bits of  */
/* acedb to take special actions on initialisation etc., e.g. both graph and */
/* the server code need to do special things.                                */
/*                                                                           */


/* Register the graph callbacks, this routine is expected to be called       */
/* before sessionInit(). This is _not_ some generalised interface, it is     */
/* simply a way to allow the server to take some graph-specific actions      */
/* at various points. Note that all functions must be provided.              */
/* NOTE that we expect the caller to keep their struct around and allocated, */
/* we do not take a copy so that the caller can change dynamically the       */
/* routines/data that we point at.                                           */
/*                                                                           */
/* This routine is not thread safe, but we shouldn't be threading here       */
/* anyway.                                                                   */
void sessionRegisterGraphCB(GraphSessionCB callbacks)
{
  if (callbacks == NULL
      || callbacks->exit_msg_cb == NULL
      || callbacks->crash_msg_cb == NULL)
    messcrash("Registering of graph callbacks failed, "
	      "a NULL callback struct or NULL callback routine was passed.") ;
  else
    graphCBs_G = callbacks ;

  return ;
}


/************************************************************/

/* Register the server callbacks, this routine is expected to be called      */
/* before sessionInit(). This is _not_ some generalised interface, it is     */
/* simply a way to allow the server to take some server-specific actions     */
/* on intialisation and termination of acedb. Note that all functions must   */
/* be provided.                                                              */
/* NOTE that we expect the caller to keep their struct around and allocated, */
/* we do not take a copy so that the caller can change dynamically the       */
/* routines/data that we point at.                                           */
/*                                                                           */
/* This routine is not thread safe, but we shouldn't be threading here       */
/* anyway.                                                                   */
void sessionRegisterServerCB(ServerSessionCB callbacks)
{
  if (callbacks == NULL
      || callbacks->init.funcs.func == NULL
      || callbacks->checkFiles.funcs.func == NULL
      || callbacks->mess_out.user_func == NULL
      || callbacks->mess_err.user_func == NULL
      || callbacks->exit.funcs.mesgfunc == NULL
      || callbacks->crash.funcs.mesgfunc == NULL
      || callbacks->sighup.funcs.mesgfunc == NULL
      || callbacks->sigterm.funcs.mesgfunc == NULL)
    messcrash("Registering of server callbacks failed, "
	      "a NULL callback struct or NULL callback routine was passed.") ;
  else
      serverCBs_G = callbacks ;

  return ;
}

/* Get a pointer to the serverCBs struct, needed by some routines outside    */
/* of session.c (e.g. sigsubs.c) to find out whether to call server          */
/* routines.                                                                 */
/*                                                                           */
ServerSessionCB sessionGetServerCB(void)
{
  return serverCBs_G ;
}


/************************************************************/

/* Allocate and get hold of a global session control block, intent is to move
 * all session stuff over to a context one day. */
static SessionContext createContext(void)
{
  SessionContext context ;

  context = messalloc(sizeof(SessionContextStruct)) ;

  context->unused = NULL ;

  session_context_G = context ;

  return context ;
}

SessionContext getContext(void)
{
  return session_context_G ;
}



/************************************************************/

void sessionInit (char* ace_path)
     /* general public function */
{
  /* NOTE: only ever called from aceInit() */
  KEY lastSession = 0;
  SessionContext context = NULL ;
  
#if !(defined(MACINTOSH))
  euid = geteuid();
  ruid = getuid();
  seteuid(ruid);

  if (euid != ruid && !getenv("ACEDB_NO_BANNER"))
    { printf("// Acedb is running for user %s.\n", getLogin (TRUE));
      printf("// Acedb is inheriting database write access from user %s.\n", 
	     getLogin (FALSE));
    }

  if (getgid() != getegid())
     messerror("WARNING. Your acedb executable has a permission "
	       "flag that inherits the group-id on execution (setgid). "
	       "This is pointless, and will stop acedb from spawning "
	       "external programs on systems with dynamic linking. "
               "Use executables with a set-user-id flag instead.");

#endif

  /* Macro KEYKEY relies on KEY beeing 4 bytes long  */
  /* Furthermore, it may be assumed implicitly elsewhere */

  if (sizeof(KEY) != 4)
    messcrash ("KEY size is %d - it must be 4 - rethink and recompile!",
	        sizeof(KEY)) ;

  if (__Global_Bat != KEYMAKE(1,1)) /* something is really wrong */
    messcrash 
      ("Hand edit systags.h because KEYMAKE(1,1) = %d != __Global_Bat",
       KEYMAKE(1,1)) ; 


  /* init these freeXXX first, they belong to the utilities-library,
     do all the acedb specific inits after that */
  freeinit () ;
  freeOutInit () ; 
  utilsInit () ;  /* makes later calls to getLogin effectivelly threadSafe */

  if (!dbPathInit (ace_path))
    messExit ("Home database directory not found - you must give a "
	      "parent directory with subdirectories database/ and wspec/ "
	      "as a command line argument.") ;
 
  /* Check that the database directory has /database and /wspec 
     sub directories and that the models.wrm file is in /spec.
  */
  {
    char *tmp;
    if ((tmp = dbPathStrictFilName("wspec", 0, 0, "rd", 0)) != NULL)
      messfree(tmp);
    else
      messExit ("wspec directory not found - you must give a "
		"parent directory with subdirectories database/ and wspec/ "
		"as a command line argument.") ; 
    
    if ((tmp = dbPathStrictFilName("database", 0, 0, "rd", 0)) != NULL)
      messfree(tmp);
    else
      messExit ("database directory not found - you must give a "
		"parent directory with subdirectories database/ and wspec/ "
		"as a command line argument.") ;

    if ((tmp = dbPathStrictFilName("wspec", "models", "wrm", "r", 0)) != NULL)
      messfree(tmp);
    else
      messExit("The file \"wspec/models.wrm\" could not be found "
	       "in the parent database directory, the database must have "
	       "this file to be usable.") ;

    if ((tmp = dbPathStrictFilName("wspec", "database", "wrm", "r", 0)) != NULL)
      messfree(tmp);
    else
      messExit("The file \"wspec/database.wrm\" could not be found "
	       "in the parent database directory, the database must have "
	       "this file to be usable.") ;
  }

  /* Locate a whelp directory if we can. Note that we use the contents of
     ${ACEDB}/whelp if we can, otherwise ${ACEDB_COMMON}/whelp, but we never 
     mix the contents of the two. */
  { 
    char *helpDir = dbPathFilName("whelp", "", "", "rd", 0);
    if (helpDir)
      {
	helpSetDir(helpDir);
	messfree(helpDir);
      }
  }


  /* Set up the session context, in the end this should be returned by sessionInit()
   * to the application. */
  context = createContext() ;


  /* Initialise the server call backs, must come after sessionSetPath but    */
  /* before crash routine registration.                                      */
  if (serverCBs_G)
    serverCBs_G->init.funcs.func(serverCBs_G->init.data) ;


  /* this is panic-memory, we claim some memory here which 
     we'll never use. In case the program messcrash'es we free this
     chunk of storage, so hopefully we can exit gracefully and
     save our work before we have to exit */
  messcrashbuf = malloc (12*1024) ;

  /* now register all the acedb specific crash/recovery routines */
  /* NOTE, if you add an outContext or ErrorContext you should revue the     */
  /* code for these functions which is registered by the server below.       */
  {
    static struct messContextStruct exitContext = {acedbExit, NULL};
    static struct messContextStruct crashContext = {acedbCrash, NULL};
    static struct messContextStruct dumpContext = {acedbLog, NULL};

    messExitRegister (exitContext);
    messCrashRegister (crashContext);

    logfile_possible = TRUE;	/* make sure we try to open log */
    acedbLog (NULL, NULL);	/* open logfile - establish status 
				 * before we register it as the 
				 * dumpRoutine to use for messubs */
    messDumpRegister (dumpContext);

    /* Server needs normal or error messages to go to special places.        */
    if (serverCBs_G)
      {
	messOutRegister(serverCBs_G->mess_out) ;
	messErrorRegister(serverCBs_G->mess_err) ; 
      }
  }


  /* Call server routine so server can check for config. or lock files.      */
  /* NOTE, this call must come before the database is truly opened and       */
  /* intialised because it may exit if a database lock file is found. It will*/
  /* exit directly, not via messcrash, because of interactions with inetd.   */
  /* Needs to be after crash routine registration so we can log errors.      */
  if (serverCBs_G)
    serverCBs_G->checkFiles.funcs.func(serverCBs_G->checkFiles.data) ;


#ifdef CHRONO
  chronoStart () ;
#endif  
  /* The order of the initialisations is crucial */ 
  chrono ("preInit") ;
  blockInit() ;			/* Initialisation du cache */  
  lexInit() ;                   /* Initialisation du lexique */
  pickPreInit() ;		/* Enough SysClass definitions 
				   to read the session voc */

  getOtherDatabaseConfigs(&sessionKeepAlive, &readlockTimeout,
			  &suppressLock, &syncWrite,
			  &enforceTimestamps, &db_title_G, &group_admin_G) ;

  if (syncWrite)	        /* Synchronous writing to disk, slow */
    diskSetSyncWrite() ;        /* but reliable. */
  sessionDoInit() ;             /* Checks ACEDB.wrm 
				   (and boostraps if neces.)
				   Sets the number thisSession.session
				   sets readlock for thisSession 
				   Reads in the lexiques etc, */

  lastSession = lexLastKey(_VSession);
  if (lastSession == 0)
    messcrash("The session class is empty, Reconstruct, sorry") ;
  sessionStart(lastSession) ;


  chronoReturn () ;
  chrono ("realInit") ;

  pickInit() ;                  /* Class definitions */

  prefInit () ;
  prefReadDBPrefs() ;					    /* Preferences */

  pickCheckConsistency() ;     /* Check tag values and class hierarchy */
  lexAlphaClear () ;            /* Prevents pointless savings */
  sessionInitialize () ;	/* must come after pickInit */
  parseArrayInit() ;			/* in quovadis.c */
  bIndexInit(BINDEX_AUTO) ;           /* will do only from disk */

  {
    /* purge the readlocks directory of timed-out readlocks
       and report the readlocks still active
       (unless ACEDB_NO_BANNER is set) */
    Array tmp = readlockGetSessionNumbers
      (getenv("ACEDB_NO_BANNER") ? FALSE : TRUE);
    arrayDestroy (tmp);
  }

  chronoReturn () ;
  return;
} /* sessionInit */

/*************************************************************/
/* definitelly release all memories before exit */

void sessionShutDown (void)
{

#ifdef CHRONO
  chronoStop () ;
  chronoReport () ;
#endif  

#if 0
  /* called funct not yet folded in from ncbi (mieg, june 03 */
  int n0, n1 ;
  BOOL debug = FALSE ;

  messAllocStatus (&n1) ;
  
  if (debug) freeOutf ("%18s%8s%8s\n","// Layer","Freed","Used") ;
  if (debug) freeOutf ("%18s%8d%8d kb\n","// Shutdown start", 0, n1 >> 10) ; n0 = n1 ;

  lexAlphaShutDown () ; 
  messAllocStatus (&n1) ; 
  if (debug) freeOutf ("%18s%8d%8d kb\n","// LexAlpha", (n0 - n1) >> 10, n1 >> 10) ;
  n0 = n1 ;

  lexShutDown () ; 
  messAllocStatus (&n1) ; 
  if (debug) freeOutf ("%18s%8d%8d kb\n","// Lex", (n0 - n1) >> 10, n1 >> 10) ;
  n0 = n1 ;

  bIndexShutDown () ;
  messAllocStatus (&n1) ; 
  if (debug) freeOutf ("%18s%8d%8d kb\n","// Bindex", (n0 - n1) >> 10, n1 >> 10) ;
  n0 = n1 ;

  bsMakePathShutDown () ; 
  messAllocStatus (&n1) ; 
  if (debug) freeOutf ("%18s%8d%8d kb\n","// MakePaths", (n0 - n1) >> 10, n1 >> 10) ;
  n0 = n1 ;

  cacheShutDown () ;
  messAllocStatus (&n1) ; 
  if (debug) freeOutf ("%18s%8d%8d kb\n","// Cache2", (n0 - n1) >> 10, n1 >> 10) ;
  n0 = n1 ;

  blockShutDown  () ;
  messAllocStatus (&n1) ; 
  if (debug) freeOutf ("%18s%8d%8d kb\n","// Blocks", (n0 - n1) >> 10, n1 >> 10) ;
  n0 = n1 ;

  BSshutdown () ;
  messAllocStatus (&n1) ; 
  if (debug) freeOutf ("%18s%8d%8d kb\n","// BS", (n0 - n1) >> 10, n1 >> 10) ;
  n0 = n1 ;

  BTshutdown () ;
  messAllocStatus (&n1) ; 
  if (debug) freeOutf ("%18s%8d%8d kb\n","// BT", (n0 - n1) >> 10, n1 >> 10) ;
  n0 = n1 ;

  OBJShutDown () ;
  messAllocStatus (&n1) ; 
  if (debug) freeOutf ("%18s%8d%8d kb\n","// OBJ", (n0 - n1) >> 10, n1 >> 10) ;
  n0 = n1 ;
  
  diskShutDown () ;
  messAllocStatus (&n1) ; 
  if (debug) freeOutf ("%18s%8d%8d kb\n","// OBJ", (n0 - n1) >> 10, n1 >> 10) ;
  n0 = n1 ;
  
  /*
    prefShutDown () ;
  */ 

  messfree (messcrashbuf) ;
  if (debug)  freeOutf ("\n") ;
  freeOutShutDown () ; /* cannot use freeout after freeshutdown, which anyway uses very little */  
  freeshutdown () ;
  messAllocShutdown () ;
  n0 = n1 ;
#endif
}

/*************************************************************/

/* External interface to I_own_the_passwd() function. */
BOOL sessionAdminUser(char *err_msg)
{
  BOOL result = FALSE ;

  result = I_own_the_passwd(err_msg) ;

  return result ;
}

/* Check ownership of wspec/passwd.wrm file which is used to decide which    */
/* users are allowed to reinitialise the database.                           */
/*                                                                           */
/* for_what is a string like                                                 */
/*  "initialize the database" or "change something important"                */
/*   or "enable write access"                                                */
/*                                                                           */
/* 1) following must be true: ruid == euid  and  rgid == egid                */
/* 2) wspec/passwd.wrm must be readable                                      */
/* 3) by default we check that ruid == uid of wspec/passwd.wrm               */
/* 4) but if GROUP_ADMIN is specified in wspec/database.wrm then in addition */
/*    we check whether rgid or users supplementary groups match the gid of   */
/*    wspec/passwd.wrm.                                                      */
/*                                                                           */
/*                                                                           */
BOOL I_own_the_passwd(char *for_what)
{
#if defined(MACINTOSH)

  return TRUE;

#elif defined (__CYGWIN__)

  /* For windows 95, 98, always return TRUE, since it has no security */
  if ( GetVersion() & 0x80000000)

    return TRUE ;

#else /* normal Unixes */

  struct stat status ;
  char *passwd_filename = 0;
  STORE_HANDLE handle;
  BOOL result = FALSE ;

  handle = handleCreate();
  passwd_filename = dbPathStrictFilName("wspec", "passwd", "wrm", "r", handle);

  if (passwd_filename && stat(passwd_filename, &status) == 0)
    {
      if (ruid == status.st_uid)
	{
	  result = TRUE;
	}
      else if (group_admin_G)
	{
	  /* Group permissions check allowed.                                */
	  if (checkFileGroupPerm(&status))
	    result = TRUE ;
	  else
	    {
	      messout ("Sorry, to %s, you must be in the unix group for the "
		       "password file. Check %s !",
		       for_what, passwd_filename);
	      result = FALSE;
	    }
	}
      else
	{
	  messout ("Sorry, to %s, you must be the owner of the "
		   "password file. Check %s !",
		   for_what, passwd_filename);
	  result = FALSE;
	}
    }
  else
    {
      messout ("Sorry, the database has no password file. "
	       "To %s, the usernames of trusted administrators "
	       "have to be listed in the file : %s/passwd.wrm",
	       for_what, dbPathStrictFilName("wspec", "", "", "rd", handle));
      result = FALSE;
    }

  handleDestroy(handle) ;

  return result ;
#endif
}

/* This could be made into a general utility with a little work.             */
/*                                                                           */
/* Strict check of whether a users rgid or supplementary gids match a files  */
/* gid.                                                                      */
/*                                                                           */
/* Note that some Unixes do not support supplementary groups, we test        */
/* NGROUPS_MAX to look for this.                                             */
/*                                                                           */
static BOOL checkFileGroupPerm(struct stat *file_stat)
{
  BOOL result = FALSE ;
  gid_t rgroup, egroup ;

  rgroup = getgid() ;
  egroup = getegid() ;

  if (rgroup == egroup)
    {
      if (file_stat->st_gid == rgroup)
	result = TRUE ;
#ifdef NGROUPS_MAX 
      else
	{
	  int ngroups ;

	  /* This looks a little arcane, basically you have to find out how  */
	  /* many supplementary groups there are before you can find what    */
	  /* they are...sigh...                                              */
	  ngroups = getgroups(0, NULL) ;
	  if (ngroups == -1)
	    messcrash("Internal coding error in checkFileGroupPerm() "
		      "the getgroups() call to find the number of supplementary "
		      "groups failed.") ;
	  else if (ngroups == 0)
	    result = FALSE ;
	  else
	    {
	      gid_t *groups ;

	      groups = messalloc(sizeof(gid_t) * ngroups) ;

	      if (getgroups(ngroups, groups) <= 0)
		messcrash("Internal coding error in checkFileGroupPerm() "
			  "the getgroups() call to retrieve the supplementary "
			  "groups failed.") ;
	      else
		{
		  int i ;
		  gid_t gid ;
		  gid_t *grpptr = groups ;

		  for (i = 0 ; i < ngroups ; i++)
		    {
		      gid = *grpptr++ ;

		      if (gid == file_stat->st_gid)
			{
			  result = TRUE ;
			  break ;
			}
		    }
		}

	      messfree(groups) ;
	    }
	}
#endif  /* NGROUPS_MAX */
    }

  return result ;
}


/*************************************************************/
/************** Disk Lock System *****************************/
/*************************************************************/

static BOOL showWriteLock (void)
{
  static char* logName = "Don't-know-who" ;
  int n = 0;
  ACEIN lock_io = 0 ;
  char *filename;

  filename = dbPathMakeFilName("database", "lock", "wrm", 0);
  lock_io = aceInCreateFromFile (filename, "r", "", 0);
  messfree(filename);

  if (lock_io)
    {
      if (aceInCard(lock_io))
	{ 
	  aceInNext(lock_io) ;
	  if (aceInInt(lock_io,&n))
	    { 
	      char *name ;

	      aceInNext(lock_io) ;
	      name = aceInWord(lock_io) ;
	      if (name != NULL)
		logName = name ;
	    }
	}
      aceInDestroy (lock_io) ;
      messout ("%s (session %d) already has write access", logName, n) ;
      return TRUE ;
    }

  return FALSE;
} /* showWriteLock */

/**************************/

static int lockFd = -1 ;	/* lock file descriptor */

static void releaseWriteLock (void)
{
  char *filename = NULL;

  seteuid(euid);
  if (lockFd == -1)
    messcrash ("Unbalanced call to releaseWriteLock") ;

  if (!suppressLock)
    if (lockf (lockFd, F_ULOCK, 0)) /* non-blocking */
      messcrash ("releaseWriteLock cannot unlock the lock file");

  close (lockFd) ;

  filename = dbPathStrictFilName("database", "lock", "wrm", "w", 0);
  if ((!filename) || (!filremove(filename, "")))
    messerror ("Cannot unlock database "
	       "by removing database/lock.wrm (%s)", 
	       messSysErrorText()) ;
  
  if (filename)
    messfree(filename);

  lockFd = -1 ;
  seteuid(ruid);

  messdump ("Write access dropped") ; 
} /* releaseWriteLock */

/**************************/

static BOOL setWriteLock (void)
{  
  unsigned int size ;
  char *cp ;
  /* should never fail */
  char *filename = dbPathMakeFilName("database", "lock", "wrm", 0);

  if (lockFd != -1)
    messcrash ("Unbalanced call to setWriteLock") ;

  if (suppressLock)
    {
      /* check for existence of unlocked lock.wrm file
	 if it exists, then the database is considered locked
      */       
      lockFd = open(filename, O_RDONLY);
      if(lockFd != -1)		/* open was successful 
				 * the lock-file exists */
	{
	  showWriteLock();	/* show the owner of the lock */
	  close(lockFd);
	  lockFd = -1;
	  messfree(filename);
	  return FALSE;		/* can't get lock */
	}
      /* OK so we don't have a lock-file .... */
    }

  /* open the file regardless whether the file is there,
   * create it if it's not there and open in readwrite mode */

  seteuid(euid);
  lockFd = open (filename, O_RDWR | O_CREAT | O_SYNC, 0644) ;

  if (lockFd == -1)
    { 
      /* is there a permissions problem ? */
      /* shouldn't have happened, if 
       *   sessionDatabaseWritable() succeeded... */
      messerror ("I can't open or create the lock file %s. (%s)", 
		 filename, messSysErrorText()) ;
      seteuid(ruid);
      messfree(filename);
      return FALSE ;
    }

  if (!suppressLock)
    {
      /* on systems that permit file-locking we make use of lockf(3)
       * on the actual lockfile. Its advantage is that the kernel-lock 
       * will be cleaned up even when the program dies and
       * leaves the file hanging around.
       */
      /* THERE IS A BIG PROBLEM HERE WITH NFS, IF THE FILE EXISTS ON A       */
      /* REMOTE FILE SYSTEM AND THE LOCK DAEMON IS NOT WORKING PROPERLY,     */
      /* THE CODE WILL HANG ON THIS TEST AND NOT RETURN. There is nothing we */
      /* can do about this because our NFS mounts at sanger are "hard" mounts*/
      if (lockf (lockFd, F_TLOCK, 0)) /* non-blocking test for lock */
	{
	  if (errno == EACCES)
	    {
	      /* "Permission denied" - standard case
	       *  can't lock file - show who has locked locked it */
	      showWriteLock();
	    }
	  else
	    {
	      /* other error */
	      /* could be the errno ENOLCK - "No locks available"
		 hints to a possible misconfiguration 
		 of the lockdaemon, see man-page lockf(3) */
	      messout ("Sorry, I am unable to establish a lock "
		       "on file database/lock.wrm\n"
		       "(%s)\n"
		       "There may be a problem with the lock daemon "
		       "on this machine, esp. if running NFS.\n"
		       "To avoid the use of the lock daemon, "
		       "insert the word NOLOCK\n"
		       "in the file wspec/database.wrm",
		       messSysErrorText());
	    }
	  
	  close(lockFd);
	  lockFd = -1 ;
	  
	  seteuid(ruid);
	  messfree(filename);
	  return FALSE ;
	}
    }

  /* write lock-owner info into the lock-file */
  cp = messprintf ("%d %s\n\n", thisSession.session, getLogin(TRUE)) ;
  size = strlen(cp) + 1 ;
  write (lockFd, cp, size) ;

  seteuid(ruid);

  messdump ("Write access");
  messfree(filename);
  return TRUE;
} /* setWriteLock */

/****************************************************************/
/****************************************************************/

/* If thisSessionStartTime == 0 then no session has currently been started
   we set the start time here but only create the userSession key in 
   sessionUserKey in case it is never needed. */

static mytime_t  thisSessionStartTime = 0 ;


/* set start time of session and init userKey */
void sessionInitialize (void)
{
  thisSessionStartTime = timeNow() ;
  thisSession.userKey = 0 ;
  lexSessionStart () ;

  return ;
} /* sessionInitialize */

/************************************************************/

void sessionStart (KEY fromSession)
     /* private function in session module */
{
  OBJ Session ;
  int  a3 = 0, b3 = 0, c3 = 0, h3 = 0,
    parent_index_version = 0 ;
  
  Session = bsCreate(fromSession) ;
  if(!Session )
    { messout("Sorry, I cannot find %s", name(fromSession)) ;
      return ;
    }
  if(!bsGetData(Session,_VocLex,_Int,&a3) ||
     !bsGetData(Session,_bsRight,_Int,&b3)  ||
     !bsGetData(Session,_bsRight,_Int,&c3)  ||
         /* Hasher only introduced in release 1.4  !bsGetData(Session,_bsRight,_Int,&h0)  || */
     !bsGetData(Session,_CodeRelease,
	    _Int,&thisSession.mainCodeRelease) ||
     !bsGetData(Session,_bsRight,
		_Int,&thisSession.subCodeRelease) ||
     !bsGetData(Session,_DataRelease,
	    _Int,&thisSession.mainDataRelease) ||
     !bsGetData(Session,_bsRight,
	    _Int,&thisSession.subDataRelease) ) 
    { messout("No address in this session") ;
      bsDestroy(Session) ;
      return ;
    }
  bsGetData(Session,_IndexVersion, 
	    _Int,&parent_index_version) ;
  
  bsDestroy(Session) ;

  thisSession.from = fromSession ;
  thisSession.upLink = fromSession ;
  thisSession.index_version = bIndexVersion(parent_index_version) ;

  lexOverLoad(__lexi3,(DISK)a3) ;
  lexOverLoad(__voc3,(DISK)c3) ;
  lexOverLoad(__lexh3,(DISK)h3) ;

  lexRead() ;

  return;
} /* sessionStart */

/*************************************************************/

static BOOL sessionOedipe (void) /* Kill the father of the present session */
{ KEY father = thisSession.upLink, grandFather, fPlus, fMinus ;
  OBJ Father ; 
  Array fPlusArray, fMinusArray ;

   if(!father || !(Father = bsUpdate(father)))
    return FALSE ;
  if(!bsGetKey(Father,_BatPlus,&fPlus) ||
     !bsGetKey(Father,_BatMinus,&fMinus) ) 
    { messout("No address in this father") ;
      bsDestroy(Father) ;
      return FALSE ;
    }

  if(!bsFindTag(Father,_Permanent_session))
    { messout("The previous session being delared permanent, I keep it") ;
      bsDestroy(Father) ;
      return FALSE ;
    }

  if (bsGetKey(Father, _Up_linked_to, &grandFather))
    { thisSession.upLink = grandFather ;
      bsAddData(Father, _Destroyed_by_session, _Int, &thisSession.session) ;
      bsSave(Father) ;
    }
  else
   { thisSession.upLink = 0 ;
     bsDestroy(Father) ;
   }  
  
  fPlusArray = arrayGet(fPlus,unsigned char,"c") ;
  fMinusArray = arrayGet(fMinus,unsigned char,"c") ;

  diskFuseOedipeBats(fPlusArray, fMinusArray) ; 

  arrayDestroy(fPlusArray) ;
  arrayDestroy(fMinusArray) ;
              /* Kill the father bats */
  arrayKill(fPlus) ;
  arrayKill(fMinus) ;

  saveAll() ;
  return TRUE ;
} /* sessionOedipe */

/******************/

static BOOL sessionFuse (KEY father, KEY son) 
{ 
  KEY fPlus, fMinus, sPlus, sMinus, grandFather ;
  OBJ Father = 0 , Son = 0 ; 
  int nPlus = 0, nMinus = 0 ; 

  Father = bsUpdate(father) ; 
  Son = bsUpdate(son) ;
  if(!Father || !Son || 
     !bsGetKey(Father,_BatPlus,&fPlus) ||
     !bsGetKey(Father,_BatMinus,&fMinus) ||
     bsFindTag(Father, _Destroyed_by_session) ||
     !bsGetKey(Son,_BatPlus,&sPlus) ||
     !bsGetKey(Son,_BatMinus,&sMinus) 
     )
    goto abort ;

  if (!diskFuseBats(fPlus, fMinus, sPlus, sMinus, &nPlus, &nMinus)) 
    goto abort ;

  /* Store the sons bat sizes */
  bsAddKey(Son, _BatPlus, sPlus) ;   /* to reposition */
  bsAddData(Son, _bsRight, _Int, &nPlus) ;
  bsAddKey(Son, _BatMinus, sMinus) ;
  bsAddData(Son, _bsRight, _Int, &nMinus) ;
  
  bsAddData(Father, _Destroyed_by_session, _Int, &thisSession.session) ;
  if (bsGetKey(Father, _Up_linked_to,&grandFather))
    bsAddKey(Son , _Up_linked_to, grandFather) ;

  bsSave(Father) ;
  bsSave(Son) ;
  saveAll() ;  
  return TRUE ;

 abort:
  bsSave(Son) ;
  bsSave(Father) ;
  return FALSE ;
} /* sessionFuse */

/************************************************************/

Array sessionTreeCreate(BOOL showDeads)
     /* private function in session module */
{ 
  KEY key = 0 , kk ; 
  int treeArrayMax ; 
  char *cp ;
  OBJ obj ;
  ST *st , *st1 ;
  int gen, i, j;
  BOOL found ;
  Array sessionTree;

  messStatus ("Reading sessions");

  sessionTree = arrayCreate(50, ST) ;
  
  /* Add all Session objects to the session tree */
  treeArrayMax = 0;
  while (lexNext(_VSession, &key))
    if ((obj = bsCreate(key)))
      {
	st = arrayp(sessionTree, treeArrayMax++, ST) ;
	st->key = key ;
	bsGetData(obj,_Session, _Int, &st->number);

	if (bsGetKey(obj,_Created_from, &kk))
	  st->father = st->ancester = kk ;
	if (bsGetKey(obj,_Up_linked_to, &kk))
	  st->ancester = kk ;
	else if (st->key != KEYMAKE(_VSession, 2))
	  st->isDestroyed = TRUE ; /* i.e. to old a release */
	if (bsFindTag(obj,_Destroyed_by_session))
	  st->isDestroyed = TRUE ;
	if (bsFindTag(obj,_Permanent_session))
	  st->isPermanent = TRUE ;

	if (bsGetData(obj,_User, _Text, &cp))
	  st->user = strnew(cp, 0);

	if (bsGetData(obj,_Date, _Text, &cp))
	  st->date = strnew(cp, 0);

	if (bsGetData(obj,_Session_Title, _Text, &cp) && strlen(cp) > 0)
	  st->title = strnew(cp, 0);
	else
	  st->title = strnew(name(st->key), 0);

	st->len = strlen(st->title);
	
	bsDestroy(obj) ;
      }

  /* Add thisSession to the session tree 
     (the object for the current session is not a finalized object
     yet and therfore won't show up in the lexer up) */
  st = arrayp(sessionTree, treeArrayMax++, ST) ;
  st->key = _This_session ;
  st->father = st->ancester = thisSession.from ;
  st->title = strnew(name(st->key), 0);
  st->len = strlen(st->title);
	
  /* links the st */
  for (i = 0, st = arrp(sessionTree,0,ST) ; i < arrayMax(sessionTree) ; st++, i++)
    for (j = 0, st1 = arrp(sessionTree,0,ST) ; j < arrayMax(sessionTree) ; st1++, j++)
      if(st->key == st1->ancester)
	st1->sta = st ;

  /* Count generations */
  if (showDeads)
    { 
      for (i = 0, st = arrp(sessionTree,0,ST); i < arrayMax(sessionTree) ; st++, i++)
	if (!st->ancester)
	  st->generation = 1 ;
    }
  else
    {
      for (i = 0, st = arrp(sessionTree,0,ST) ; i < arrayMax(sessionTree) ; st++, i++)
	if ((!st->isDestroyed || st->isReadlocked) && ( !st->ancester || (st->sta && st->sta->isDestroyed)) )
	  st->generation = 1 ;
    }
  gen = 1 ;
  found = TRUE ;
  while (found)
    { found = FALSE ;
      if (showDeads)
	{
	  for (i = 0, st = arrp(sessionTree,0,ST) ; i < arrayMax(sessionTree) ; st++, i++)
	    if (st->sta && st->sta->generation == gen)
	      { st->generation = gen + 1 ;
		found = TRUE ;
	      }
	}
      else
	{
	  for (i = 0, st = arrp(sessionTree,0,ST) ; i < arrayMax(sessionTree) ; st++, i++)
	    if ((!st->isDestroyed || st->isReadlocked) && st->sta && st->sta->generation == gen)
	      { st->generation = gen + 1 ;
		found = TRUE ;
	      }
	}
      gen++ ;
    }

  /* mark readlocked sessions, they won't get destroyed, because
     other processes still have that data cached */
  {
    Array readlockedSessions;
    int n, locked_num;

    if ((readlockedSessions = readlockGetSessionNumbers(FALSE)))
      {
	for (n = 0; n < arrayMax(readlockedSessions); n++)
	  {
	    locked_num = arr (readlockedSessions, n, int);

	    for (i = 0, st = arrp(sessionTree,0,ST) ;
		 i < arrayMax(sessionTree) ; st++, i++)
	      /* locked_num - 1 is the session that the process
		 (that keeps the readlock) was initialised from - 
		 that is the number that needs to be protected. */
	      if (st->number == locked_num - 1)
		st->isReadlocked = TRUE;
	  }
	  arrayDestroy (readlockedSessions);
      }
  }

  return sessionTree;
} /* sessionTreeConstruct */
    
/************************************************************/  

void sessionTreeDestroy(Array sessionTree)
     /* private function in session module */
{
  ST *st ;
  int i;
 
  if (!arrayExists(sessionTree))
    messcrash("sessionTreeDestroy() - called with invalid sessionTree");

  for (i = 0, st = arrp(sessionTree, 0, ST);
       i < arrayMax(sessionTree); st++, i++)
    {
      if (st->user) messfree (st->user);
      if (st->date) messfree (st->date);
      if (st->title) messfree (st->title);
    }
  
  arrayDestroy (sessionTree);

  return;
} /* sessionTreeDestroy */

/************************************************************/  

static int sessionFindSon(Array sessionTree, KEY father, KEY *sonp)
{ 
  int i, n = 0;
  ST *st, *sta ;

  *sonp = 0 ;             /* Suicide */
  if (father == _This_session)
    return -2 ;
  if (!father)           /* Trivial */
    return -3 ;
  if (father <= KEYMAKE(_VSession, 2))
    return -6 ;

  for (i = 0, sta = arrp(sessionTree,0,ST) ; i < arrayMax(sessionTree) ; sta++, i++)
    if (sta->key == father)
      break ;

  if (sta->key != father)       /* not found error */
    return -5 ;
  
  if (sta->isDestroyed)          /* dead */
      return -3 ;
 
  for (i = 0, st = arrp(sessionTree,0,ST) ; i < arrayMax(sessionTree) ; st++, i++)
    if(!st->isDestroyed && st->sta == sta)
      { *sonp = st->key ;
	n++ ;
      }
  
  if (n == 1)            /* is st1 in my direct ancestors */
    { for (i = 0, st = arrp(sessionTree,0,ST) ; i < arrayMax(sessionTree) ; st++, i++)
	if (st->key == thisSession.from)
	  {
	    while (st)
	      { if (st == sta)
		  return 1 ;
		st = st->sta ;  
	      }
	    return -4 ;
	  }
      return -4 ;        /* not ancestor */
    }

  return n ;             /* number of sons */
} /* sessionFindSon */

/************************************************************/

static BOOL sessionKillAncestors (Array sessionTree, int n)
/* Kill all father over level n */
{
  KEY father = thisSession.upLink, son ;
  ST *st ;
  int i ;

  for (i = 0, st = arrp(sessionTree,0,ST);
       i < arrayMax(sessionTree); st++, i++)
    if (st->key == father)
      break ;

  if (father && st->key != father) 
    { 
      messerror ("sessionKillAncestors() - "
		 "could not find the father he wanted to murder.") ;
      return FALSE ;
    }
 
  while (st && --n)            /* Move up the ladder n times */
    st = st->sta ;
  if (!st || n)                /* Not many ancesters alive */
    return FALSE ;
  
  while (st && (st->isPermanent || 
		st->isReadlocked || 
		sessionFindSon(sessionTree, st->key, &son) != 1))
    st = st->sta ;            /* Try higher up */
    
  if (st && st->key && son)
    return sessionFuse(st->key, son) ;

  return FALSE ;
} /* sessionKillAncestors */

/*************************************************************/

static char* getConfiguredDatabaseName (void)
{ 
  static char dbname[32] = "";
  char *word ;
  ACEIN db_io = 0 ;
  char *filename;

  filename = dbPathMakeFilName("wspec", "database", "wrm", 0);
  db_io = aceInCreateFromFile (filename, "r", "", 0);
  messfree(filename);
  
  if (db_io);
    {
      while (aceInCard(db_io))
	if ((word = aceInWord(db_io))
	    && strcmp (word, "NAME") == 0
	    && (word = aceInWord(db_io)))
	  {
	    if (strlen (word) > 31)
	      messExit ("Error in wspec/database.wrm : "
			"the NAME is too long (31 characters max).") ;
	    else
	      strcpy (dbname, word) ;
	  }
      aceInDestroy (db_io) ;
    }

  return &dbname[0];
} /* getConfiguredDatabaseName */

/*************************************************************/

/* Set any default value overrides from wspec/database.wrm */
static void getOtherDatabaseConfigs(int *keepAlive, int *timeOut,
				    BOOL *suppressLock, BOOL *syncWrite,
				    BOOL *enforceTs, char **db_title,
				    BOOL *group_admin)
{ 
  char *word ;
  ACEIN db_io = 0 ;
  char *filename;

  filename = dbPathMakeFilName("wspec", "database", "wrm", 0);
  db_io = aceInCreateFromFile (filename, "r", "", 0);
  messfree(filename);
  
  if (db_io)
    {
      while (aceInCard(db_io))
	{
	  if ((word = aceInWord(db_io)))
	    {
	      if (strcmp (word, "SESSION_KEEP_ALIVE") == 0)
		{
		  BOOL rc ;
		  rc = aceInInt(db_io,keepAlive) ;
		  if (rc == FALSE || *keepAlive < SESSKEEPALIVE_MIN)
		    messExit ("Error in wspec/database.wrm : "
			      "smallest legal value for SESSION_KEEP_ALIVE is %d",
			      SESSKEEPALIVE_MIN) ;
		}
	      else if (strcmp (word, "READLOCK_TIMEOUT") == 0)
		{
		  BOOL rc ;
		  rc = aceInInt(db_io,timeOut) ;
		  if (rc == FALSE || *timeOut < RDLCKTIMEOUT_MIN)
		    messExit ("Error in wspec/database.wrm : "
			      "smallest legal value for READLOCK_TIMEOUT is %d hours",
			      RDLCKTIMEOUT_MIN) ;
		}
	      else if (strcmp(word, "NOLOCK") == 0)
		{
		  *suppressLock = TRUE ;
		}
	      else if (strcmp(word, "SYNC_WRITE") == 0)
		{
		  *syncWrite = TRUE ;
		}
	      else if (strcmp(word, "ENFORCE_TIMESTAMPS") == 0)
		{
		  *enforceTs = TRUE;
 		}
	      else if (strcmp(word, "TITLE") == 0)
		{
		  word = aceInPos(db_io) ;
		  if (word && *word)
		    *db_title = strnew(word, 0) ;
 		}
	      else if (strcmp(word, "GROUP_ADMIN") == 0)
		{
		  *group_admin = TRUE ;
 		}
	    }
	}

      aceInDestroy (db_io) ;
    }

  return;
} /* getOtherDatabaseConfigs */


static BOOL purgeDirectory (char *directoryPath)
{
  BOOL isError = FALSE;
  int n;
  char fullFileName[MAXPATHLEN];
  FilDir dirList;

  if (!directoryPath)
    return FALSE;

  
  dirList = filDirCreate (directoryPath, 0, "w");
  /* list all files that we can potentially delete */
  
  for (n = 0; n < filDirMax(dirList); n++)
    {
      strcpy (fullFileName, directoryPath);
      strcat (fullFileName, SUBDIR_DELIMITER_STR);
      strcat (fullFileName, filDirEntry(dirList, n));
      
      if (unlink(fullFileName) == -1)
	isError = TRUE;
    }
  
  messfree (dirList);
  
  return isError;
} /* purgeDirectory */

/*************************************************************/

static void sessionDoInit(void)
{
  FILE *f ;
  int mainRelease;
  char *filename;
  char *log_msg = NULL ;

  if ((filename = dbPathStrictFilName ("database", "ACEDB","wrm","r", 0)) 
      != NULL) 
    { 
      messfree(filename);
      /* this doesn't swap the superblock (write does) */
#ifndef ACEDB5
      dataBaseAccessInit();
      diskblockread (&theSuperBlock,1) ; 
      if (theSuperBlock.byteSexIndicator != 1)
	{ swapData = TRUE;
	  swapSuperBlock(&theSuperBlock);
	  if (!getenv("ACEDB_NO_BANNER"))
	    printf("// Database has non-native byte-order: swapping\n");
	}
      else
	{ 
	  swapData = FALSE;
	}


      /* In order to stop old database code running on databases which
	 have timestamps, we zero the mainrelease value in the superblock.
	 Hence old code will fail the test below. 
	 For code in the know, the real mainrelease value is held elsewhere
	 in the superblock. Note that we only implement this if 
	 ENFORCE_TIMETSTAMPS is set in database.wrm */
      if (theSuperBlock.mainRelease)
	{
	  mainRelease = theSuperBlock.mainRelease;
	  if (enforceTimestamps)
	    { 
	      theSuperBlock.alternateMainRelease = mainRelease;
	      theSuperBlock.mainRelease = 0;
	    }
	}
      else
	mainRelease = theSuperBlock.alternateMainRelease;

#else /* ACEDB5 */
      aDiskAccessInit(&theSuperBlock);
      mainRelease = theSuperBlock.mainRelease;
#endif /* ACEDB5 */

      if (mainRelease != aceGetVersion())
	{
	  if (getenv ("ACEDB_CHANGE_MAIN_RELEASE")) /* override */
	    {
#ifdef ACEDB4
	      if (theSuperBlock.mainRelease)
		theSuperBlock.mainRelease = aceGetVersion() ;
	      else
		theSuperBlock.alternateMainRelease = aceGetVersion();
#else
	      theSuperBlock.mainRelease = aceGetVersion() ;
#endif  
	    }
	  else
	    messExit
	      ("You are running %s of the code.\n"
	       "The database was created under release %d.\n"
	       "Either use the correct binary or upgrade the database.\n",
	       aceGetVersionString(), theSuperBlock.mainRelease) ;
	}
#ifdef ACEDB5
      thisSession.session = ++theSuperBlock.session ;
#else
      thisSession.session = ++theSuperBlock.h.session ;
#endif
      thisSession.gAddress = theSuperBlock.gAddress ;
      lexOverLoad (__lexi1,theSuperBlock.gAddress) ;
      lexReadGlobalTables () ;

      if (!readlockCreateFile())
	fprintf (stderr,
		 "Unable to set readlock for session %d.\n"
		 "The database owner should run an acedb program\n"
		 "with this new version (%s) to prepare the\n"
		 "database for readlocks.\n",
		 thisSession.session, aceGetVersionString());

      return ;
    }
  /* else reinitialise the system */
  
  /* Boolean forceInit_G indicates that if the main program was called with -init
  ** then the database is to be initialised without further prompting.
  */
  if (forceInit_G == FALSE)
  {
    if (!messQuery ("The file database/ACEDB.wrm does not exist, \n"
		  "indicating that the database is empty.\n"
		  "  Should I re-initialise the system?"))
    messExit ("Database will remain uninitialized. Bye!") ;
  }

  /* make sure we start the new database with a fresh log-file, do that      */
  /* before anything else writes to the current log-file.                    */
  log_msg = hprintf(0, "Database is being rebuilt by %s - End of this logfile",
		    getLogin(TRUE)) ;
  if (!acedbStartNewLogFile(FALSE, NULL, log_msg, TRUE))
    messExit ("Cannot start new log file for new session !  Initialization failed !") ;
  messfree(log_msg) ;

  /* Only the acedb administrator is entitled to proceed
   * I check this by (a) getuid == geteuid
   *                 (b) checking for a password file
   *                 (c) testing that the user owns the file or
   *                     if GROUP_ADMIN = TRUE that the user is
   *                     is in the group for the password file.
   *                 (d) testing that the user is listed in that file
   * In the install documentation, it is stated that
   * passwd.wrm should be set chmod 644, but 444 is OK, 
   * once the file is set-up
   */
  if (!I_own_the_passwd ("initialise the database") || 
      !sessionCheckUserName (TRUE))
    messExit ("Not authorized to initialize the database.\n"
	      "Check wspec/passwd.wrm");
  

  if (!setWriteLock())
    /* RARE occurence, but this can happen if another process is also 
       in the middle of the initialization process at the same time */
    messExit ("Cannot set write lock! Initialization failed!") ;

  /****************************************************************/
  /****** all permissions clear, boostrap an empty database *******/
  /****************************************************************/

  { 
    char *tmp;
    if ((tmp = dbPathStrictFilName("database", "new", 0, "wd", 0)) != NULL)
      {
	purgeDirectory(tmp);
	messfree(tmp);
      }
    if ((tmp = dbPathStrictFilName("database", "touched", 0, "wd", 0)) != NULL)
       {
	purgeDirectory(tmp);
	messfree(tmp);
      }
    if ((tmp = dbPathStrictFilName("database", "readlocks", 0, "wd", 0)) != NULL)
      {
	purgeDirectory(tmp);
	messfree(tmp);
      }
  }
  
  thisSession.session = 0;	/* special session number to be used
				   for this first readlock 
				   will be updated by sessionDoClose()
				   called at the end of this function */
  if (!readlockCreateFile())
    messExit ("Unable to set readlock in database/readlocks/");
  
  /****************************************************************/

  writeAccess = TRUE  ; /* implied */
  swapData = FALSE ; /* The database file must be in our order */
  thisSession.session = 1 ;
  strcpy (thisSession.name, "Empty_db") ;
  thisSession.gAddress = 0 ; /* so far */
  thisSession.mainCodeRelease = aceGetVersion() ; 
  thisSession.subCodeRelease = aceGetRelease() ; 
  thisSession.mainDataRelease = aceGetVersion() ; 
  thisSession.subDataRelease = 0 ; 
  thisSession.from = 0 ; 
  thisSession.upLink = 0 ; 

#ifndef ACEDB5
  theSuperBlock.h.disk = 1 ;
  theSuperBlock.h.nextdisk = 0 ;
  theSuperBlock.h.type = 'S' ;
  theSuperBlock.h.key  = 0 ;
  theSuperBlock.byteSexIndicator = 1;

  if (enforceTimestamps)
    { 
      theSuperBlock.mainRelease = 0;
      theSuperBlock.alternateMainRelease = aceGetVersion();
    }
  else
    theSuperBlock.mainRelease = aceGetVersion() ;
#endif

  strcpy (theSuperBlock.dbName, getConfiguredDatabaseName());

  /* DataBase creation */
  if (!dataBaseCreate())
    messExit ("Database Creation failed, check write permissions etc.") ;


  messdump("**********************************************************");
  messdump("Making a new database file and starting at session 1 again");
  
  readModels ();

  /* application specific initialisations, in quovadis.wrm */
  diskPrepare ();

  sessionInitialize ();		/* do this last before saving */

  filename = dbPathMakeFilName("database", "ACEDB", "wrm", 0);
  if (!(f = filopen (filename, 0 , "w")))
    messExit ("Sorry, the operating system won't let me create %s", filename) ;
  fprintf (f, "Destroy this file to reinitialize the database\n") ;
  filclose (f) ;
  messfree(filename);
  
  filename = dbPathStrictFilName("database", "lock", "wrm", "w", 0);
  f = filopen (filename, 0, "w") ;
  fprintf (f, "The lock file\n") ;
  filclose (f) ;
  messfree(filename);
  
  readModels () ; /* second read needed to register the _VModel obj */

  bIndexInit(BINDEX_AUTO) ;
  sessionDoClose () ;	/* real save without sessionInitialize
			   will also update readlock-file */
} /* sessionDoInit */

/************************************************************/


/* Used by status.c and display.c to get users title for the database.       */
/*                                                                           */
char *sessionDBTitle(void)
{
  return db_title_G ;
}


char *sessionDbName (void)
     /* general public function */
{
#ifdef ACEDB4
  int mainRelease = theSuperBlock.mainRelease ? 
    theSuperBlock.mainRelease : theSuperBlock.alternateMainRelease;
#else
   int mainRelease = theSuperBlock.mainRelease;
#endif
/* returns the name of the database as gathered from the superblock,
     used for the window title and messages in data updates */
   if (mainRelease < 4 || theSuperBlock.subCodeRelease < 3)
     return 0 ;			/* not implemented before 4.3 */
   else if (*theSuperBlock.dbName)
    return theSuperBlock.dbName ;
   else
     return 0 ;
} /* sessionDbName */



/* Return last time/date the database was saved. */
char *sessionLastSaveTime(void)
{
  char *save_time = NULL, *cp ;
  KEY last_session ;
  OBJ obj ;

  last_session = thisSession.upLink ;

  if (!(obj = bsCreate(last_session)))
    messcrash("Unable to open last session object from its key: \"%s\"", name(last_session)) ;

  if (!(bsGetData(obj,_Date, _Text, &cp)))
    messcrash("Unable to get \"Date\" from last session object: \"%s\"", name(last_session)) ;
  save_time = strnew(cp, 0);

  bsDestroy(obj) ;

  return save_time ;
}



/************************************************************/

static void sessionSaveSessionObject(void)
{
  /* store the Session Object */
  /* update the BAT block access table for thisSession */
  DISK d ;
  OBJ Session ;
  KEY kPlus, kMinus ;
  int free2, plus, minus, used, ndread,ndwrite ;


  Session = bsUpdate (thisSession.key) ;
  if(!Session)
    messcrash("Session register fails to create the session object") ;

  if (thisSession.session == 1)
    bsAddTag(Session,_Permanent_session) ;

  bsAddData(Session,_User,_Text,&thisSession.name) ;

  if (!bsFindTag (Session, _Date)) /* don't add date twice - it changes => save loop */
    {
      bsAddTag (Session, _Date) ;

      if (bsType (Session, _bsRight) == _Text)
	/* dates used to be _Text, we have to account for that */
	bsAddData (Session, _Date, _Text, timeShow(timeNow())) ;
      else
	/* with recent versions, the date is _DateType */
	/* ERRRRRMMMMM, this seems not to be true...see definition in whooks/sysclass.c */
	{ 
	  mytime_t session_date = timeNow() ;
	  bsAddData (Session, _Date, _DateType, &session_date) ;
	}
    }

  if (*thisSession.title)
    bsAddData(Session,_Session_Title,_Text,thisSession.title) ;
  bsAddData(Session,_Session,_Int,&thisSession.session) ;
  if(thisSession.from)
    bsAddKey(Session,_Created_from,thisSession.from) ;
  if(thisSession.upLink)
      bsAddKey(Session,_Up_linked_to,thisSession.upLink) ;

  thisSession.mainCodeRelease = aceGetVersion() ; 
  thisSession.subCodeRelease = aceGetRelease() ; 
  bsAddData(Session,_CodeRelease,
	    _Int,&thisSession.mainCodeRelease) ;
  bsAddData(Session,_bsRight,
	    _Int,&thisSession.subCodeRelease) ;
  bsAddData(Session,_DataRelease,
	    _Int,&thisSession.mainDataRelease) ;
  bsAddData(Session,_bsRight,
	    _Int,&thisSession.subDataRelease) ;
  { int v = bIndexVersion(-1) ;
    if (v)
      bsAddData(Session,_IndexVersion,
		_Int,&v) ;
  }
 
  diskavail(&free2,&used,&plus,&minus,&ndread,&ndwrite) ;

  d = lexDisk(__Global_Bat) ;
  bsAddData(Session,_GlobalBat,_Int,&d) ; 
  bsAddData(Session,_bsRight,_Int,&used) ;

  d = lexDisk(__lexi1) ;
  bsAddData(Session,_GlobalLex,_Int,&d) ;

  d = lexDisk(__lexi2) ;
  bsAddData(Session,_SessionLex,_Int,&d) ;
  d = lexDisk(__lexa2) ;
  bsAddData(Session,_bsRight,_Int,&d) ;
  d = lexDisk(__voc2) ;
  bsAddData(Session,_bsRight,_Int,&d) ;
  d = lexDisk(__lexh2) ;
  bsAddData(Session,_bsRight,_Int,&d) ;

  d = lexDisk(__lexi3) ;
   bsAddData(Session,_VocLex,_Int,&d) ;
  d = lexDisk(__lexa3) ;
  bsAddData(Session,_bsRight,_Int,&d) ;
  d = lexDisk(__voc3) ;
  bsAddData(Session,_bsRight,_Int,&d) ;
  d = lexDisk(__lexh3) ;
  bsAddData(Session,_bsRight,_Int,&d) ;

  lexaddkey(messprintf("p-%d",thisSession.session),&kPlus,_VBat) ;
  lexaddkey(messprintf("m-%d",thisSession.session),&kMinus,_VBat) ;
  bsAddKey(Session,_BatPlus,kPlus) ;
  bsAddData(Session,_bsRight,_Int,&plus) ;

  bsAddKey(Session,_BatMinus,kMinus) ;
  bsAddData(Session,_bsRight,_Int,&minus) ;
 
  bsSave(Session) ;
} /* sessionSaveSessionObject */

/************************************************************/

static void sessionSaveUserSessionObject (void)
{ 
  /* complete the UserSession object */
  /* this might be called several times over 
     in the process of saveAll() */
  OBJ obj ; 
  KEY key, tag ;
  mytime_t timEnd = timeNow() ;
  
  lexSessionEnd () ;	/* writes new and touched keysets */
  
  sessionUserKey () ;
  obj = bsUpdate (thisSession.userKey) ;
  
  bsAddKey (obj, _Session, thisSession.key) ;
  bsAddData (obj, _User, _Text, thisSession.name) ;
  
  /* don't change dates again, once set, to avoid causing
     saveAll() to think that modifications have happened
     in the multiple saving loop */
  if (!bsFindTag (obj, _Start))
    {
      if (!thisSessionStartTime)
	messcrash ("Can't save UserSession object "
		   "with unititialized StartTime");
      bsAddData (obj, _Start, _DateType, &thisSessionStartTime) ;
    }
  if (!bsFindTag (obj, _Finish))
    bsAddData (obj, _Finish, _DateType, &timEnd) ;
  
  if (lexword2key (messprintf ("new-%s", name(thisSession.userKey)),
		   &key, _VKeySet))
    { 
      lexaddkey ("New", &tag, 0) ;
      bsAddKey (obj, tag, key) ;
    }

  if (lexword2key (messprintf ("touched-%s", name(thisSession.userKey)),
		   &key, _VKeySet))
    { 
      lexaddkey ("Touched", &tag, 0) ;
      bsAddKey (obj, tag, key) ;
    }
  
  bsSave (obj) ;
} /* sessionSaveUserSessionObject */

/**************************************************/

mytime_t sessionUserSession2Time(KEY userSession)
{
  OBJ obj;
  static Associator key2time = NULL;
  void *v;
  mytime_t time;

  if (!key2time)
    key2time = assCreate();

  if (!userSession)
    return 0;

  /* If the timestamp is for th current session, the data will not
     be in the object yet */
  if (userSession == sessionUserKey())
    return sessionStartTime();

  if (assFind(key2time, assVoid(userSession), &v))
    return (mytime_t) assInt(v);
  
  time = 0;
  if ((obj = bsCreate(userSession)))
    {
      bsGetData(obj, str2tag("Start"), _DateType, &time);
      bsDestroy(obj);
    }
  
  assInsert(key2time, assVoid(userSession), assVoid(time));

  return time;
}

/************************************************************/

void sessionRegister(void)
     /* general public function */
{
  /* this is called several times over during saveAll() */
  OBJ ModelObj ;

  if (!isWriteAccess())
    return ;

#ifdef ACEDB4
  {
    OBJ obj1 = bsCreate (KEYMAKE (_VUserSession,0)) ;
    if (!obj1)
      {
	messout ("You are missing the model for UserSession.  "
	       "To bring your database up to date please Read Models.  "
	       "You will not be able to save until you do this.") ;
        return ;
      }
    bsDestroy (obj1) ;
  }
#endif


  strncpy (thisSession.name, getLogin(TRUE), 78) ;
  lexaddkey (messprintf ("s-%d-%s", 
			 thisSession.session, thisSession.name),
	     &thisSession.key, _VSession) ;

#ifdef ACEDB4
  ModelObj = bsCreate(KEYMAKE(_VSession,0)) ;
#else
  { KEY ModelKey ;
    lexaddkey ("#Session", &ModelKey, _VModel) ;
    ModelObj = bsCreate(ModelKey) ;
  }
#endif

  if(!ModelObj || !bsFindTag(ModelObj,_Up_linked_to))
    /* Always check on latest tag added to ?Session */
    { messerror("%s\n%s\n%s\n%s\n%s\n",
		"FATAL, an eror occured in sessionRegister",
		"Probably, your model for the session class is not correct",
		"and corresponds to an earlier release.",
		"Look for the latest version of the wspec/models.wrm file,",
		"read the model file and try again.") ;
      exit(1) ;
    }
  bsDestroy(ModelObj) ;


  sessionSaveSessionObject() ;

  sessionSaveUserSessionObject() ;

  /* will be reset for next new session (in sessionInitialize) */
  thisSessionStartTime = 0 ;
} /* sessionRegister */


/*************************************************************/

/* returns TRUE if modifications had been saved */
BOOL sessionDoClose (void)
{
  BOOL savedSomething = FALSE ;
  KEY son ;

  if (!isWriteAccess())
    return FALSE ;

  /* Try to save all caches - return TRUE, if there
     had been modifications that were saved to disk*/
  if (saveAll())		
    {
      /* Yes we saved modifications - adjust the session structure*/

      /* I keep only a certain number of session alive 
	 (the sessionKeepAlive static int) */
      if (thisSession.session != 1)
	{
	  Array sessionTree;

	  sessionTree = sessionTreeCreate (FALSE); /* exclude dead sessions */

	  if (sessionKeepAlive != 1)
	    while (sessionKillAncestors(sessionTree, sessionKeepAlive))
	      {
		sessionTreeDestroy(sessionTree);
		sessionTree = sessionTreeCreate (FALSE);
	      }
	  else if (sessionFindSon(sessionTree, thisSession.upLink, &son) == 1)
	    sessionOedipe () ;

	  sessionTreeDestroy (sessionTree);
	}

                  /****************************/
                  /* increment session number */
                  /****************************/

      /* if you edit this code, edit also readsuperblock 
	 and sessionDestroy */
      theSuperBlock.gAddress = thisSession.gAddress = lexDisk(__lexi1) ;
#ifdef ACEDB4
      if (!theSuperBlock.mainRelease)
	theSuperBlock.alternateMainRelease = aceGetVersion() ;
      else
#endif
      theSuperBlock.mainRelease = aceGetVersion() ;
      theSuperBlock.subCodeRelease = aceGetRelease() ;
      theSuperBlock.magic = saveMagic ;
      thisSession.from =  thisSession.key ;
      thisSession.upLink =  thisSession.key ;
#ifdef ACEDB5   
      theSuperBlock.session =  thisSession.session ;
#else
      theSuperBlock.h.session =  thisSession.session ;
#endif
      diskWriteSuperBlock(&theSuperBlock); /* writes and zeros the BATS*/

      messdump ("Save session %d", thisSession.session) ;

      thisSession.session++;	/* increase current session number */

      readlockUpdateFile (); /* update readlock 
				for next session */

      savedSomething = TRUE ;

      if (sessionChangeRoutine)
	(*sessionChangeRoutine)();
    }
  /* else savedSomething is still FALSE */

  sessionReleaseWriteAccess()  ;

  if (externalCommit) externalCommit() ;

  return savedSomething ;
} /* sessionDoClose */

/*************************************************************/

BOOL isWriteAccess(void)
     /* general public function */
{ return writeAccess ? TRUE : FALSE ;
} /* isWriteAccess */

/*************************************************************/

/* gains write access for the user, if possible */
BOOL sessionGainWriteAccess(void)
{
  BOOL write_lock = FALSE ;

  if (writeAccess)		/* already has write access */
    return TRUE;

  /* We don't have write access, but can we grab it ??                       */
  /* The tests made are:                                                     */
  /* dropWriteAccess => cannot re-gain write access once permanently dropped.*/
  /* sessionCheckUserName() => checks wspec/passwd.wrm for user.             */
  /* setWriteLock() => fails if we can't set a write lock, MUST do this      */
  /*                   before doing sessionDatabaseWritable() otherwise      */
  /*                   there is a window where another user can screw up     */
  /*                   file permissions and we don't see this.               */
  /* sessionDatabaseWritable() => check we can write files into database     */
  /*                              subdir and that blockN.wrm files are R/W   */
  if (dropWriteAccess        
      || !sessionCheckUserName(TRUE)
      || !(write_lock = setWriteLock())			    /* Remember if we got a lock. */
      || !sessionDatabaseWritable(TRUE))
    {
      if (debug)
	{
	  messdump ("gainWriteAccess Failed") ;
	  if (dropWriteAccess) messdump ("dropWriteAcces=TRUE") ;
	  if (!sessionDatabaseWritable(TRUE)) messdump ("sessionDatabaseWritable=FALSE") ; 
	  if (!sessionCheckUserName(TRUE)) messdump ("sessionCheckUserName=FALSE") ; 
	}

      if (write_lock)
	releaseWriteLock() ;

      return FALSE;
    }


  /*   Cannot gain write access in   an older session */
  if (!sessionCheckSuperBlock())
    {
      messout ("Sorry, while you where reading the database, "
	       "another process updated it.\n"
	       "// You can go on reading, but to write, you must "
	       "quit and rerun.") ;

      if (write_lock)
	releaseWriteLock();
      
      return FALSE;
    }

  /* all checks passed */
  writeAccess = TRUE ;		/* was false before */

  /* therefore calls the routine that gets notified of the change,
     this is usually to re-draw buttons or change the menus, etc */
  if (writeAccessChangeRoutine)
    (*writeAccessChangeRoutine)();

  return TRUE;
} /* sessionGainWriteAccess */

/*************************************************************/

void sessionReleaseWriteAccess (void)
     /* general public function */     
{
  if (isWriteAccess())
    {
      releaseWriteLock () ;
      writeAccess = FALSE ;

      if (writeAccessChangeRoutine)
	(*writeAccessChangeRoutine)();
    }

  return;
} /* sessionReleaseWriteAccess */

/*************************************************************/

static BOOL sessionCheckUserName (BOOL doTalk)
{
#if defined(MACINTOSH)

    extern BOOL passPrompt ( void ) ;

    if(!passPrompt()) {
    	messout ("Sorry, your name was not found in the password file") ;
    	return FALSE ;
    }
    else
      return TRUE ;

#else  /* !MACINTOSH */
  static int result = 0 ;
  register char *cp, *name ;
  Stack uStack ;
  ACEIN passwd_io = 0 ;
  char *passwd_filename;

  /* check only once, since the userid of the process cannot change */
  switch (result)
    {
    case 0: result = 1 ; break ;
    case 1: return FALSE ;
    case 2: return TRUE ;
    }

#if defined (__CYGWIN__)
  if ( GetVersion() & 0x80000000)
    /* For windows 95, 98, always return TRUE, since it has no security */
    { 
      result = 2;
      return TRUE;
    }
#endif

/* Only users listed in wspec/passwd.wrm can get write access */

/* I cannot use cuserid, because I want the real userid and, 
   depending on systems, cuserid may return either the real or 
   the effective userid which is probably reset by setuid().
   
   On the other hand, Unix getlogin() is replaced by local 
   getLogin() because getlogin() returns NULL if the process 
   is not attached to a terminal */

  name = getLogin(TRUE) ;
  if (debug)
    {
      char *fname = dbPathMakeFilName("wspec","passwd","wrm",0);
      messdump ("getLogin=%s",name?name:"name=0") ;
      messdump ("wspec=%s",fname) ;
      messfree(fname);
    }

  passwd_filename = dbPathMakeFilName("wspec", "passwd", "wrm", 0);
  passwd_io = aceInCreateFromFile(passwd_filename, "r", "", 0);

  if (!passwd_io)
    {				
      messout ("The database has no file wspec/passwd.wrm. To enable "
	       "write access to the database, the usernames "
	       "of trusted administrators have to be listed in the "
	       "file : %s",
	       passwd_filename);
      messfree(passwd_filename);
      return FALSE ;
    }

  messfree(passwd_filename);
  uStack = stackCreate(32);

  while (aceInCard (passwd_io))
    { 
      aceInNext (passwd_io) ;
      if (!aceInStep(passwd_io,'#'))     /* skip # lines */
	if ((cp = aceInWord(passwd_io))) /* read user name */
	  pushText(uStack,cp) ;
    }


  /* Warn user that NOLOCK is no longer valid in wspec/passwd.wrm            */
#ifdef ACEDB5_forget_taht_mess
   /* within acedb4, this creates incompatibility with acedb.4_7 code */
  stackCursor(uStack, 0) ;
  while (!stackAtEnd (uStack))
    if (strcmp ("NOLOCK", stackNextText(uStack)) == 0)
      {
	messExit("The \"NOLOCK\" option has been found in wspec/passwd.wrm, "
		 "this option must now be set in wspec/database.wrm. "
		 "You should ask your database administrator to move this option "
		 "from wspec/passwd.wrm to wspec/database.wrm and then you will "
		 "be able to use the database. (This session will now terminate "
		 "because running with \"NOLOCK\" set incorrectly may cause "
		 "database integrity problems.)") ;
      }
#endif

  /* find our user-name amongst names in passwd.wrm */
  stackCursor(uStack, 0) ;
  while (!stackAtEnd (uStack))
    if (strcmp (name, stackNextText(uStack)) == 0)
      { 
	stackDestroy (uStack) ;
	aceInDestroy (passwd_io) ;
        result = 2 ;

	return TRUE ;		/* found it */
      }

  /* failed to find the name */
  if (doTalk)
    {
      if (strcmp (name, "getLoginFailed") == 0)
	messout ("On your particular machine, the operating system call "
		 "\"getlogin\" failed. To circumvent this problem, you "
		 "may uncomment the user name \"getLoginFailed\" in "
		 "the file wspec/passwd.wrm.\n"
		 "But from then on, everybody will have write access "
		 "unless you set up other security measures using "
		 "write permissions.") ;
      else
	messout ("%s is not a registered user. "
		 "To register a user for write access add their "
		 "login name to the file wspec/passwd.wrm", name) ;
    }

  stackDestroy(uStack) ;
  aceInDestroy (passwd_io) ;

  return FALSE ;
#endif /* !MACINTOSH */
}

/*************************************************************/

static BOOL sessionDatabaseWritable(BOOL doTalk)
{
  BOOL result = FALSE ;
  FILE *f;
  char *name = NULL ;

  /* first see if we can create a file in the database directory.            */
  name = dbPathMakeFilName("database", "test", 0, 0);
  seteuid(euid);
  unlink(name);
  f = fopen (name, "a");
  if (f)
    fclose(f);
  unlink(name);
  seteuid(ruid);
  messfree(name);

  if (!f)
    { 
      if (doTalk)
	{
	  name = dbPathMakeFilName("database", 0, 0, 0);
	  messout ("Sorry, the program does not have permission to write "
		   "files in %s", name) ;
	  messfree(name);
	}
      result = FALSE ;
    }
  else
    {
      /* OK, now check the blockN.wrm files.                                 */
      result = blocksWriteable(doTalk) ;
    }

  return result ;
} /* sessionDatabaseWritable */


/* This code checks that all the blockN.wrm files specified in               */
/* wspec/database.wrm that actually exist in database/ are readable and      */
/* writeable. This check was introduced because previously users could get   */
/* write access without actually having write access to some block files.    */
/* Then when they tried to save, the database would get corrupted if they    */
/* needed to write to those files.                                           */
/* If report_errors is TRUE, errors are logged and an error message output,  */
/* otherwise the errors are just logged.                                     */
/*                                                                           */
static BOOL blocksWriteable(BOOL report_errors)
{
  BOOL result ;
  char *config_file ;
  ACEIN config_in;
  char *cp ;

  seteuid(euid);

  /* If we can't find wspec/database.wrm then something is very wrong.       */
  if (!(config_file = dbPathStrictFilName("wspec", "database", "wrm", "r", 0)))
    messcrash("Cannot find database configuration file wspec/database.wrm") ;

  config_in = aceInCreateFromFile (config_file, "r", "", 0);
  messfree(config_file);

  /* Look for all the block records which have the format:                   */
  /*     PART :       1   local    ACEDB          block1.wrm  1000     0     */
  result = TRUE ;
  while (aceInCard (config_in))
    {
      char *block_name = NULL ;

      if (!(cp = aceInWord(config_in)))
	continue ;					    /* skip empty lines and comments */

      if (strcmp(cp, "PART") != 0)			    /* only do lines with PART keyword */
	continue ;

      /* Fourth word is blocks file name.                                */
      aceInStep (config_in, ':') ;
      if (!(cp = aceInWord (config_in))
	  || !(cp = aceInWord (config_in))
	  || !(cp = aceInWord (config_in))
	  || !(cp = aceInWord (config_in)))
	messExit("Parse error in database definition file %s at line %d",
		 dbPathMakeFilName("wspec", "database", "wrm", 0),
		 (aceInStreamLine(config_in) - 1)) ;

      /* If the file exists, check to see if it is readable/writeable.       */
      block_name = dbPathMakeFilName("database", cp, NULL, 0) ;
      if (access(block_name, F_OK) == 0)		    /* No equivalent in filsubs.c ? */
	{
	  BOOL error = FALSE ;
	  char *access = NULL ;

	  if (!filCheckName(block_name, NULL, "r"))
	    {
	      result = FALSE ;
	      error = TRUE ;
	      access = "read" ;
	    }
	  else if (!filCheckName(block_name, NULL, "w"))
	    {
	      result = FALSE ;
	      error = TRUE ;
	      access = "write" ;
	    }

	  if (error)
	    {
	      if (report_errors)
		messerror("Cannot get %s access to database block file %s",
			  access, block_name) ;
	      else
		messdump("Cannot get %s access to database block file %s",
			 access, block_name) ;
	    }

	}
      messfree(block_name) ;
    }

  aceInDestroy (config_in);

  seteuid(ruid);

  return result ;
}


/*************************************************************/

  /* If another process has written since I started
     either he has grabed the lock, and I can't write,
     or he may have released it but then he may have updated
     the disk.
     In this case, the session number on block 1 will be
     modified and I will know that my BAT is out of date
     */

BOOL sessionCheckSuperBlock(void)
{
  int nn = 0 ;
#ifndef ACEDB5
  BLOCK newSuperBlock ;
#endif

/* The call to diskFlushLocalCache forces the latest version of the
   superblock to be read from the NFS server (for remote disks). 
   This closes a concurrency  hole which can see this function 
   succeed because it is checking a stale, client side cached 
   copy of the superblock. */

  diskFlushLocalCache();

#ifdef ACEDB5
  nn = aDiskGetGlobalAdress() ; 
#else  /* !ACEDB5 */
  diskblockread (&newSuperBlock,1) ; 

  if (swapData)
    swapSuperBlock(&newSuperBlock);
  nn = newSuperBlock.gAddress ; 
  if (thisSession.gAddress != nn)
    {
      int ourSession = theSuperBlock.h.session, diskSession = newSuperBlock.h.session;
      
      /* keep a record of this occurence in the logfile */
      messdump("FATAL ERROR cached superblock inconstistent "
	       "with disk : ourSession=%d  diskSession=%d",
	       ourSession, diskSession);
      return FALSE ;
    }
#endif
  return TRUE ;
} /* sessionCheckSuperBlock */

/*************************************************************/

/* general public function */
void sessionForbidWriteAccess (void)
{
  /* definitive, can't re-gain write access ever again,
     Should we have a AllowWriteAccess function to match it ?? */

  readlockDeleteFile();		/* read locks no longer needed */
  dropWriteAccess = TRUE ;	/* make it permanent */

  /* the permanent loss of write access will change the
     behaviour of writeAccessPosibble(), and therefor may 
     change buttons or menus */
  if (writeAccessChangeRoutine)
    (*writeAccessChangeRoutine)();
 
  return;
} /* sessionForbidWriteAccess */

/*************************************************************/

BOOL writeAccessPossible(void)
     /* general public function */
{ 
  /* return TRUE if it would be possible to gain write access */
  static int status = 0;
  
  if (dropWriteAccess)
    return FALSE ;
  if (status)
    return status == 1 ? TRUE : FALSE ;
  
  if (sessionDatabaseWritable(FALSE) && sessionCheckUserName(FALSE))
    { status = 1;
      return TRUE;
    }
  else
    { status = 2;
      return FALSE;
    }
} /* writeAccessPossible */


/**************************************************************/

/* Used to turn on/off saving globally, i.e. if you want to completely disable
 * saving, perhaps for testing purposes, you can use this call to prevent any
 * database saving. */
void sessionSetSaveStatus(BOOL save)
{
  saveDB_G = save ;

  return ;
}


/**************************************************************/

/* Used largely in command.c by commands that need to save a session 
 * after some operation that updates the database. */
/* used also in aceserver.c */
void sessionMark (BOOL doIt, int newTimeOut)
{
  static mytime_t t = 0 , t0 = 0 ;
  static int dt, timeOut = -1 ;

  t = timeNow () ;
  if (!t0)
    t0 = t ;
  if (newTimeOut > 0)
    timeOut = newTimeOut ;
 
  if (isWriteAccess())
    {
      if (doIt || (timeDiffSecs (t, t0, &dt) && dt > timeOut))
	{
	  messdump("Saving Session %d at %s",
		   thisSession.session, timeShow(timeParse("now"))) ;

	  closeSession(TRUE) ;

	  messdump("Saving done %s", timeShow(timeParse("now"))) ;
	}
    }
  t0 = timeNow () ;

  return;
} /* sessionMark */


/**************************************************************/

/* Close a database session, with or without saving current changes.
 *
 * This routine has come to be used just prior to calling exit() by acedb
 * applications so it closes the log. It should only be used
 * for "normal" exits, not for crashes.
 * 
 * This is only here becaue sessionClose() calls are in a lot of places.
 * If they were all replaced with sessionCloseExit() it could be
 * removed.
 */
void sessionClose (BOOL doSave)
{ 
  sessionCloseExit(doSave, ACELOG_NORMAL_EXIT) ;

  return ;
}


/* Close a database session, with or without saving current changes and
 * closes log with the given exit_type.
 */
void sessionCloseExit(BOOL doSave, aceLogExitType exit_type)
{ 
  closeSession(doSave) ;

  acedbCloseLog(exit_type) ;

  return ;
}


/* close and reinitialise a session, used for instance to save the database
 * periodically while an application is making updates.
 * Note that saving can be completely turned off, good for testing but
 * not for the faint hearted. */
static void closeSession(BOOL doSave)
{

  /* Only save if the user has not turned off saving altogether. */
  if (saveDB_G)
    {
      sessionAutoSave(-3, 0) ;				    /* synchronizes autosave, without
							       activating it */
      if (doSave == FALSE)
	{ 
	  sessionReleaseWriteAccess() ;
	}
      else
	{
	  if (sessionDoClose ())			    /* returns TRUE if something was saved */
	    sessionInitialize () ;			    /* only init if we had saved */
	}

      sessionAutoSave (-4, 0) ;     /* synchronizes autosave, without activating it */
    }
  else
    {
      /* Some tidying up to do even we are not saving. */
      sessionReleaseWriteAccess() ;
    }

  return ;
}



/*****************************************************************/
/*****************************************************************/
/******* AutoSaving package, mieg: march 2000 ********************/
/*****************************************************************/
/* Saves if more than timeLapse seconds elapsed since last save 
 * or since timeZero if set
 * returns time since last save
 */

unsigned int sessionTimeSave (unsigned int timeLapse, int mode)
{ 
  int dt = 0 ;
  mytime_t now = timeNow () ;
  BOOL debug = FALSE ;
  static mytime_t timeZero = 0 ;

  switch (mode)
    {
    case 2:
      timeZero = now ; 
      break ;
    
    case 1:
      timeZero = 0 ;
      break ;
    
    case 0:
      if (timeZero)
	timeDiffSecs (timeZero, now, &dt) ;
      else
	timeDiffSecs (thisSessionStartTime, now, &dt) ;
      if (isWriteAccess() &&
	  (dt + 10 > timeLapse)) /* avoid rounding problems, and never substract on unsigned int  */
	{
	  messStatus ("Saving");
	  messdump("Autosaving if %d s elapsed done, last save was %d s ago", timeLapse, dt) ;
	  timeZero = 0 ;
	  sessionClose (TRUE) ;
	  sessionGainWriteAccess () ; /* grab again */
	  
	  now = timeNow () ;
	  messdump("Autosaving done") ;
	  timeDiffSecs (thisSessionStartTime, now, &dt) ;
	}
      else if (debug)
	messdump("Autosaving  if %d s elapsed not needed, last %d s ago  ", 
		 timeLapse,  dt) ;
      break ;

    default:
      messcrash ("Wrong call to sessionTimeSave, mode = %d", mode) ;
      break ;
    }
  return dt ;
}

/*************************************************************/
/* alarm routine */
static void  sessionAutoSaveAlarm (int dummy)
{
  unsigned int 
    dt = 0,                                /* time since last save */ 
    dtAlarm = 0,                          /* delay till next alarm */
    s1 = sessionAutoSave (-1, 0) ,                /* in seconds */
    s2 = 0 ;                          /* ask value after saving */
  BOOL debug = FALSE ; ;

  alarm ((unsigned int) 0) ;             /* disables autosaving */
  if (s1 > 0)
    {
      if (debug) messdump("AutoSaveAlarm calling sessionTimeSave (%d,0)", s1, 0) ;
      dt = sessionTimeSave (s1, 0) ;          /* save if necessary */
      s2 = sessionAutoSave (-2, 0) ;
      if (s2 > 0)
	{
	  dtAlarm = s2 > dt ? s2 - dt : s2 ;
	  if (dtAlarm < 10) dtAlarm = 10 ;
	  signal (SIGALRM, sessionAutoSaveAlarm) ; /* self register */
	  alarm (dtAlarm) ;                          /* raise alarm */
	  if (debug)
	    messdump ("Autosaving alarm expected in %d s", dtAlarm) ;
	}
      else if (debug)
	messdump ("Autosaving alarm cancelled", dtAlarm) ;
    }
  else if (debug)
    messdump ("Autosaving alarm cancelled", dtAlarm) ;
  return ;
}

/*************************************************************/
/*
 * sessionAutoSave (int saveInterval, int inactivityInterval)
 *
 * Example
 *  From anywhere in the code call
 *    sessionAutoSave (1800, 300)
 * As a result the system will autosave every half hour
 * or at the end of the first 5 minutes of innactivity
 * or of course on explicit user request
 *
 ************** Public calls 
 * saveInterval >  0 : The maximal delay before a save
 * saveInterval =  0 : cancels
 * inactivityInterval : The maximal inactivity delay before a save
 * Activity is raised by the acedb kernel calls to bsCreate
 ************** Kernel calls 
 * negative values are private to sessionAutoSave/acedb-kernel
 *** saveInterval = -1 : returns maximal saveTime interval
 *** saveInterval = -2 : returns delay till next alarm
 *** saveInterval = -3 : ace kernel starts saving
 *** saveInterval = -4 : ace kernel finished saving
 *** saveInterval = -8 : signals modif, called by bsUpdate
 *** saveInterval = -9 : signals activity, called by bsCreate
 ************** State
 *** 0: unset
 *** 1: saving while unset
 *** 2: saving while set
 *** 3: dead
 *** 4: inactif
 *** 5: actif
 ************* Algorithm
 * This routine is just a finite state machine, actual job done by 
 * sessionAutoSaveAlarm  which self registers in alarm() and calls
 * sessionTimeSave which saves if last save too old
 */

unsigned int sessionAutoSave (int saveInterval, int inactivityInterval)
{
  static int 
    state = 0,  
    longDelay = 0 ,
    shortDelay = 0 ;
  int noDelay = 1 ;
  BOOL debug = FALSE ;

  if (getenv ("ACEDB_NO_AUTOSAVE")) return 0 ;
  if (saveInterval == -9)        /* record activity, called by all bsCreate */
    switch (state)
      {
      case 0:                  /* autoSave not set, ignore */
      case 2:                  /* saving while set , ignore */
      case 1:                  /* saving while unset, ignore */
	return 0 ;
      case 3:                  /* dead, ignore */
	return 3 ; 
      case 4:                  /* inactif */
	state = 5 ;            /* new activity occured */
      case 5:                  /* actif */
	return 5 ;             /* return actif */
      default:
	messcrash ("sessionAutoSave in wrong state %d", state) ;
	return state ;
      }
  
  if (saveInterval == -8)        /* record modif, called by all bsUpdate */
    switch (state)
      {
      case 0:                  /* autoSave not set, ignore */
      case 2:                  /* saving while set , ignore */
      case 1:                  /* saving while unset, ignore */
	return 0 ;
      case 3:                  /* raise from dead */
      if (debug) messdump("sessionAutoSave(-8) calling sessionTimeSave (0,2)") ;
	sessionTimeSave (0, 2) ;    /* synchronize */
	sessionAutoSaveAlarm (0) ;
      case 4:                  /* inactif */
	state = 5 ;            /* new activity occured */
      case 5:                  /* actif */
	return 5 ;             /* return actif */
      default:
	messcrash ("sessionAutoSave in wrong state %d", state) ;
	return state ;
      }
  
  if (saveInterval == -1)      /* gives maximal delay since previous save */
    switch (state)
      {
      case 0:                  /* autoSave not set */
      case 1:                  /* saving while unset, ignore */
      case 2:                  /* saving while set, ignore */
      case 3:                  /* dead, goto sleep */
	return 0 ;
      case 4:                  /* inactif, save immediately */
	return noDelay ;
      case 5:                  /* actif */
	state = 4 ;            /* sets to inactif, crucial switch of whole system */	
	return longDelay ;
      default:
	messcrash ("sessionAutoSave in wrong state %d", state) ;
	return state ;
      }
  
  if (saveInterval == -2)      /* gives delay till next alarm */
    switch (state)
      {
      case 0:                  /* autoSave not set */
      case 1:                  /* saving while unset, ignore */
      case 2:                  /* saving while set, ignore */
      case 3:                  /* dead, go to sleep */
	return 0 ;        
      case 4:                  /* inactif */
      case 5:                  /* actif */
	return shortDelay ;
      default:
	messcrash ("sessionAutoSave in wrong state %d", state) ;
	return state ;
      }
  
  if (saveInterval == -3)        /* saving starts */
    switch (state)
      {
      case 0:                  /* autoSave not set */
	state = 1 ;            /* set saving while unset */
      case 1:                  /* saving while unset, ignore */
	return 1 ;
      case 2:                  /* saving while set, ignore */
      case 3:                  /* dead */
      case 4:                  /* inactif */
      case 5:                  /* actif */
	state = 2 ;
	return  2 ;
      default:
	messcrash ("sessionAutoSave in wrong state %d", state) ;
	return state ;
      }
  
  if (saveInterval == -4)        /* saving ends */
    switch (state)
      {
      case 0:                  /* autoSave not set */
	return 0 ;
      case 1:                  /* saving while unset, revert to unset */
	state = 0 ;
      if (debug) messdump("sessionAutoSave(-4) calling sessionTimeSave (0,2)") ;
	sessionTimeSave (0, 2) ;   /* synchronize */
	return 0 ;
      case 2:                  /* saving while set, revert to dead */
      case 3:                  /* dead */
      case 4:                  /* inactif */	
	state = 3 ; 
	return  3 ;
      case 5:                  /* actif */
	state = 3 ; 
	if (debug) messdump("sessionAutoSave(-5) calling sessionTimeSave (0,2)") ;
	sessionTimeSave (0, 2) ;   /* synchronize */
	sessionAutoSaveAlarm (0) ;  
	return  3 ;
      default:
	messcrash ("sessionAutoSave in wrong state %d", state) ;
	return state ;
      }

  if (saveInterval <= 0)         /*  zero or silly values, disables autosave */
    switch (state)
      {
      case 0:                  /* autoSave not set, ignore */
      case 1:                  /* saving while unset ignore */
	return 0 ;
      case 2:                  /* saving while set, unset with delay */
	state = 1 ;
	longDelay = shortDelay = 0 ;
	return 1 ;
      case 3:                  /* autosave set, unset */
      case 4:                 
      case 5:
	state = 0 ;	
	longDelay = shortDelay = 0 ;
	sessionAutoSaveAlarm (0) ; 
	return 1 ;
      default:
	messcrash ("sessionAutoSave in wrong state %d", state) ;
	return state ;
      }
  
  /* verify parameters are reasonable */
  if (saveInterval <= 3 * 60) 
    saveInterval = 180 ; /* 300 ; */
  if (inactivityInterval < 60)
    inactivityInterval = 60 ;
  
  if (shortDelay != inactivityInterval ||
      longDelay != saveInterval)
    state = 0 ;  /* reregistering needed */
  
  if (state != 5)
    {
      state = 5 ;        /* short wait needed */
      shortDelay = inactivityInterval ; 
      longDelay = saveInterval ;
      sessionAutoSaveAlarm (0) ;   /* register */
    }

  return state ;
}

/*****************************************************************/
/********** AutoSaving package end *******************************/
/*****************************************************************/

void sessionDoSave (BOOL reGainWriteAccess)  
     /* general public function */
{ 
  /* saves even if i had forgotten to take write access before,
     there can be no conflict, because if another user has written
     i won't be able to grab the write access */

  messStatus ("Saving") ;

  if (!isWriteAccess())  
    sessionGainWriteAccess () ; /* try to grab it */
  if (isWriteAccess())
    {
      sessionClose (TRUE) ;
      if (reGainWriteAccess) 
	sessionGainWriteAccess () ; /* grab again */
    }
}

/*************************************************************/


BOOL sessionSetUserSessionUser(char *user)
{
  BOOL result  ;
  int i ;

  if (userSessionUser)
    messcrash("User session user set twice");

  for (i = 0, result = TRUE ; (user[i]) ; i++)
    {
      if (!isalnum((int)(user[i])) &&  user[i] != '_')
	{
	  result = FALSE ;
	  break ;
	}
    }

  if (result)
    userSessionUser = strnew(user, 0);

  return result ;
}

KEY sessionUserKey (void)
{ 
  mytime_t savedTime;
  
  /* Return zero when thisSessionStartTime is zero. This is important in two 
     cases. a) Before a userSession has started, when eg timestamps get put 
     on objects in some system classes. b) To avoid infinite recursion when
     trying to timestamp timestamps. These get zero timestamps because
     thisSessionStartTime is temporarily set to zero before calling lexaddkey
     which recursively calls sessionUserKey */

  if (!thisSessionStartTime)
    return 0;

  if (!thisSession.userKey)
   { 	
     /* This avoids inifinite recursion. The timestamps for the system objects
	created in recusive calls to sessionUserKey during the lexaddkey below
        will be zero. (because sessionUserKey always returns zero when 
	thisSessionSTartTime is zero. This is OK. */
     savedTime = thisSessionStartTime;
     thisSessionStartTime = 0;
     
     if (!userSessionUser)
       sessionSetUserSessionUser(getLogin(TRUE));
     
     if (!lexaddkey (messprintf ("%s_%s",
				 timeShow(savedTime), 
				 userSessionUser), 
		     &thisSession.userKey, _VUserSession))
       { /* we've had that key already, now make a unique one */
	 int i = 0 ;
	 while (!lexaddkey (messprintf ("%s.%d_%s", 
					timeShow (savedTime),
					++i,
					userSessionUser), 
			 &thisSession.userKey, _VUserSession)) ;
       }
     thisSessionStartTime = savedTime;
   }

  return thisSession.userKey ;	
}

mytime_t sessionStartTime(void)
{
   return thisSessionStartTime;
}
/*************************************************************/
/********************* Readlock manager **********************/
/*************************************************************/

static char readlock_filename[MAXPATHLEN] = "";


void sessionCheckReadlock (void)
     /* public function called periodically to make sure that
	nobody else has destroyed our readlock. This
	may happen if a process is legitimately running for a long
	time (longer than readlockTimeout) and the readlocks
	was killed due to timeout by another process */
{
  struct stat status ;

  if (strlen(readlock_filename) == 0) /* don't use readlocks */
    return;

  if (stat (readlock_filename, &status) != 0)
    /* failed to probe the file */
    readlockCreateFile();

  return;
} /* sessionCheckReadlock */


/*************************************************************/
/* creates the directory database/readlocks/ and gives
   everybody write access to it. This directory will contain
   lock files, that specify which sessions are being looked at,
   so they aren't destroyed under an old process' feet */
/*************************************************************/

static BOOL readlockCreateDir (void)
{
  static int readlock_status = 0; /* 0 : not tried before
				     1 : everything OK
				     2 : had problems, don't try again */
  char *readlock_dir;
  STORE_HANDLE handle = handleCreate();;

  if (readlock_status != 0)
    return (readlock_status == 1 ? TRUE : FALSE);

  if (dbPathStrictFilName("database", "readlocks", 0, "wd", handle))
    {
      /* a writable directory already exists */
      readlock_status = 1;	/* no need to check again next time */
      handleDestroy(handle);
      return TRUE;			
    }

  readlock_dir = dbPathMakeFilName("database", "readlocks", 0, handle);
  
  /* make the readlocks directory with rwxrwxrwx permissions,
     so that any other user can create and remove files from there */
  if (mkdir (readlock_dir, 0000777) == -1)
    {
      /* unsuccessful completion of command */
      /* This will happen, if we us this code on a database, which
	 the administrator (the one with all the permissions) hasn't
	 initialised yet - that would have created the directory
	 correctly, even for users to use, that normally only have
	 read-only permissions. */
      
      /* no big error-message, we'll have to silently accept it */
      readlock_status = 2;	/* don't try (and fail) again */

      handleDestroy (handle);
      return FALSE;
    }

  /* I noticed that the mkdir(2) won't assign the right permissions
     at all times. That may have to do with different umask(2)
     settings. So we explicitly change the permissions again. 
     The chmod fails for very mysterious reasons under windows, so
     we skip this on that platfrom.
  */    
#ifndef __CYGWIN__
  if (chmod (readlock_dir, 0777) == -1)
    {
      readlock_status = 2;	/* don't try (and fail) again */

      handleDestroy (handle);
      return FALSE;
    }
#endif

  readlock_status = 1;		/* everything went OK */

  messdump ("Readlock manager : created readlocks directory %s",
	    readlock_dir);
  

  handleDestroy (handle);
  return TRUE;			/* managed to create directory alright */
} /* readLockCreateDir */



/*************************************************************/
/* for the given session, create the readlock file
   the file-name has the format <session>.<host>.<pid>
   and the file is in the directory database/readlocks/... */
/*************************************************************/
static BOOL readlockCreateFile (void)
{
  char host_name[100];
  int pid;
  int fd;
  char *buffer;
  STORE_HANDLE handle = handleCreate();

  if (dropWriteAccess) return TRUE;

  if (!(readlockCreateDir()))
    return FALSE;

  gethostname (host_name, 100) ;
  pid = getpid () ; 
  	   
  sprintf(readlock_filename, "%s/%d.%s.%d", 
	  dbPathStrictFilName("database", "readlocks", 0, "wd", handle),
	  thisSession.session, host_name, pid);
  /* create the readlockfile with rw-rw-rw- permission. */
  /* That makes it possible for other processes to
     remove expired readlock files that this proces may leave behind */

  fd = open(readlock_filename, O_RDWR | O_CREAT | O_SYNC, 0666);

  if (fd == -1)
    {
      fprintf (stderr, "Cannot create readlock file %s (%s)",
	       readlock_filename, messSysErrorText());

      strcpy (readlock_filename, "");
      handleDestroy(handle);
      return FALSE;
    }

  buffer = messprintf
    ("Readlock file to prevent destruction of "
     "sessions still in use by other processes\n"
     "Created: %s\n"
     "User: %s\n"
     "Program: %s\n"
     "Version: %s\n"
     "Linkdate: %s\n",
     timeShow(timeNow()), 
     getLogin(TRUE), 
     messGetErrorProgram(), 
     aceGetVersionString(), 
     aceGetLinkDateString());
	   
  write (fd, buffer, strlen(buffer));
  close (fd);

  /* I noticed that the open(2) won't assign the right permissions
     at all times. That may have to do with different umask(2)
     settings. So we explicitly change the permissions again. */    
  if (chmod (readlock_filename, 0666) == -1)
    {
      fprintf (stderr, "Cannot chmod readlock file %s (%s)",
	       readlock_filename, messSysErrorText());

      strcpy (readlock_filename, "");      
      return FALSE;
    }

  if (debug)
    messdump ("Readlock manager : start readlock for session %d by %s",
	      thisSession.session, getLogin(TRUE));

  handleDestroy(handle);
  return TRUE;
} /* readLockCreateFile */

/* The window is is not known when the readlock file is created, so we 
   append it here */
void sessionReadlockAppendFileWinID(unsigned long id)
{

  FILE *f = fopen(readlock_filename, "a");
  if (f)
    {
      fprintf(f, "WindowID: 0x%lx\n", id);
      fclose(f);
    }
}
/*************************************************************/
/* when you save a session, we get a new session number
   (in thisSession.session)
   so we rename the current readlock-file to contain
   the new session-number in the file name */
/*************************************************************/
static BOOL readlockUpdateFile (void)
{
  char host_name[100];
  int pid;
  char new_filename[MAXPATHLEN] = "";
  STORE_HANDLE handle = handleCreate();

  if (dropWriteAccess) return TRUE;

  if (strlen(readlock_filename) == 0)
    return FALSE;

  gethostname (host_name, 100) ;
  pid = getpid () ; 

  sprintf(new_filename, "%s/%d.%s.%d", 
	  dbPathStrictFilName("database", "readlocks", 0, "rd", handle),
	  thisSession.session, host_name, pid);   
  
  if (rename (readlock_filename, new_filename) == -1)
    {
      if (debug)
	messerror ("failed to rename readlock file %s to %s (%s)",
		   readlock_filename,
		   new_filename,
		   messSysErrorText());

      /* don't know how we lost it, but we must have a new readlock */
      handleDestroy(handle);
      return readlockCreateFile();
    }

  strcpy (readlock_filename, new_filename);

  if (debug)
    messdump ("Readlock manager : update readlock from session %d to %d",
	      thisSession.session-1, thisSession.session);

  handleDestroy(handle);
  return TRUE;
} /* readlockUpdateFile */

/*************************************************************/
/* called when the process exists */
/*************************************************************/
BOOL readlockDeleteFile (void)
{
  if (dropWriteAccess) return TRUE;

  if (strlen(readlock_filename) == 0)
    return TRUE;		/* OK if we haven't got a file */

  if (unlink(readlock_filename) == -1)
    {
      if (debug)
	messerror ("failed to remove readlock file %s (%s)",
		   readlock_filename,
		   messSysErrorText());
      strcpy (readlock_filename, "");
      return FALSE;
    }
  strcpy (readlock_filename, "");

  if (debug)
    messdump ("Readlock manager : end readlock for session %d",
	      thisSession.session);

  return TRUE;
} /* readlockDeleteFile */


/*************************************************************/
/* We will create an array of session numbers that still have
   a valid readlock. Sessions with those numbers will then be
   treated with caution, because other processes are still
   looking at that data of those older sessions. */
/*************************************************************/
static Array readlockGetSessionNumbers (BOOL isReport)
/* isReport == TRUE,  output report about readlocked sessions
 *                     on parent window 
 * isReport == FALSE, do output
 */
{
  Array readlockedSessions;
  FilDir dirList = 0;
  char lockfile_name[MAXPATHLEN];
  char *cp1, *cp2, *cp3;
  char *dirEntry;
  int  file_session_num;
  char file_host_name[100];
  int  file_pid;
  int n, hourAge;
  char *dir_name;
  STORE_HANDLE handle = handleCreate();

  if (!(readlockCreateDir()))
    {
      handleDestroy(handle);
      return 0;			/* no readlocks being used */
    }

  dir_name = dbPathStrictFilName("database", "readlocks", 0, "rd", handle);
  
  if (dir_name)
    dirList = filDirCreate (dir_name, 0, "r");
  
  if (!dirList)
    {
      /* this shouldn't really go wrong, because we've managed to
	 open or create the directory already */
      messerror ("failed to open directory database/readlocks/ (%s)",
		 messSysErrorText());
      
      return 0;
    }

  if (filDirMax(dirList) == 0)	/* nothing is readlocked */
    return 0;

  readlockedSessions = arrayCreate (filDirMax(dirList), int);

  for (n = 0; n < filDirMax(dirList); n++)
    /* start at 1 to skip . entry */
    {
      dirEntry = filDirEntry(dirList, n);

      sprintf(lockfile_name, "%s/%s", 
	      dbPathStrictFilName("database", "readlocks", 0, "rd", handle),
	      dirEntry);

      /* don't consider the readlock file of this process */
      if (strcmp (lockfile_name, readlock_filename) == 0)
	continue;
	
      /* extract the <sessionnum>.<hostname>.<pid> parts from filename */

      cp1 = dirEntry;
      /* find the first dot in the name */
      if (!(cp2 = strstr(cp1, "."))) continue;

      *cp2 = 0; 		/* cp1 now ends at the dot */
      ++cp2;			/* cp2 starts just after it */

      /* find the first dot from the end */
      cp3 = cp2 + strlen(cp2);
      while (cp3 > cp2 && *cp3 != '.') --cp3;

      if (cp3 == cp2) continue;

      *cp3 = 0;			/* cp2 now ends at the 2nd dot */
      ++cp3;			/* cp3 starts just after */

      if (!*cp1 || !*cp2 || !*cp3) continue;

      if (sscanf (cp1, "%d", &file_session_num) != 1) continue;
      if (sscanf (cp2, "%s", file_host_name) != 1) continue;
      if (sscanf (cp3, "%d", &file_pid) != 1) continue;

      hourAge = 0;

      if (!(filAge (lockfile_name, "", 0, 0, 0, &hourAge, 0, 0)))
	continue;		/* couldn't get age, don't consider
				   it a valid readlock then */

      if (hourAge < readlockTimeout) /* still locked then */
	{
	  int max = arrayMax(readlockedSessions);
	  /* get session-number from file-name */
	  /* terminate the filename at the position of the dot
	     then read a number from that first part of the string */
	  array(readlockedSessions, max, int) = file_session_num;

	  if (isReport)
	    {
	      ACEIN readlock_io;
	      char *readlockCreateTime = "(unknown time)";
	      char *readlockOwner = "(unknown user)";
	      char *readlockProgram = "(unknown program)";
	      char *word;
	
	      readlock_io = aceInCreateFromFile (lockfile_name, "r", "", handle);
	      if (readlock_io)
		{ 
		  while (aceInCard (readlock_io))
		    {
		      if ((word = aceInWord(readlock_io)))
			{
			  if (strcmp (word, "Created:") == 0 
			      && (word = aceInWord(readlock_io)))
			    readlockCreateTime = strnew (word, handle);
			  else if (strcmp (word, "User:") == 0
				   && (word = aceInWord(readlock_io)))
			    readlockOwner = strnew (word, handle);
			  else if (strcmp (word, "Program:") == 0 
				   && (word = aceInWord(readlock_io)))
			    readlockProgram = strnew (word, handle);
			}
		    }
		}

	      /* NOTE, the session number that is actually protected
	       * is file_session_num-1, because that's the session
	       * the process was initialised from. */
	      printf ("// Session %d is "
		      "readlocked by %s since %s using %s "
		      "(process %d on machine %s)\n",
		      file_session_num-1,
		      readlockOwner, readlockCreateTime,
		      readlockProgram, file_pid, file_host_name);

	    }
	}
      else			/* lock has expired */
	{
	  if (unlink (lockfile_name) == 0)
	    {
	      /* successfully removed */
	      if (debug)
		messdump
		  ("Readlock manager : kill readlock for session %d, "
		   " process %d on %s is older than %d hours",
		   file_session_num, file_pid, file_host_name,
		   readlockTimeout);
	    }
	}
    }

  messfree(dirList);
  handleDestroy(handle);
  
  return readlockedSessions;
} /* readlockGetSessionNumbers */


/******************************************************************/
void sessionSetforceInit(BOOL value)
{
    forceInit_G = value;
    return;
}
/******************************************************************/
/*********************** eof **************************************/
