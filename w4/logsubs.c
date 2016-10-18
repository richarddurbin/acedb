/*  File: logsubs.c
 *  Author: Ed Griffiths (edgrif@sanger.ac.uk)
 *  Copyright (c) J Thierry-Mieg and R Durbin, 2001
 *-------------------------------------------------------------------
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
 *-------------------------------------------------------------------
 * This file is part of the ACEDB genome database package, written by
 * 	Richard Durbin (Sanger Centre, UK) rd@sanger.ac.uk, and
 *	Jean Thierry-Mieg (CRBM du CNRS, France) mieg@kaa.crbm.cnrs-mop.fr
 *
 * Description: A set of routines to log messages in a standard format
 *              for acedb programs, all logs are kept in database/
 *              subdirectory of the database.
 *              All messages have a standard prefix which includes
 *              process id, user id, timestamp.
 *              Used by session.c to write messages in database/log.wrm,
 *              i.e. this happens for _all_ acedb programs.
 *              Used by serverace.c to write messages in database/serverlog.wrm,
 *              this log contains server specific information.
 *              
 * Exported functions: See wh/log.h
 * HISTORY:
 * Last edited: Oct  7 15:42 2002 (edgrif)
 * Created: Mon Feb 26 13:15:08 2001 (edgrif)
 * CVS info:   $Id: logsubs.c,v 1.8 2002-10-18 12:43:59 edgrif Exp $
 *-------------------------------------------------------------------
 */

#include <wh/regular.h>
#include <wh/aceio.h>
#include <wh/dbpath.h>
#include <wh/aceversion.h>
#include <wh/mytime.h>
#include <wh/log.h>


/* This code shows the less good side of error handling in our code, really  */
/* every single aceOut call should be tested for its return code but in      */
/* fact this needlessly complicates the code here. It would be better to     */
/* have macro versions of the aceOut calls that could be used instead, these */
/* would messcrash if the aceOut call failed giving the Line/file info. so   */
/* no result testing would be required at this level.                        */

static void logPrintPrefix(ACEOUT logfile_out, char *logfile_host, int logfile_pid, BOOL timestamp) ;


/* Keep in step with aceLogExitType in log.h                                 */
char *aceLogExitStrings[] = {"NORMAL EXIT", "ABNORMAL EXIT", "CRASH", "CLOSED"} ;


/* These are going to be the same for all logging within a process, really   */
/* they should be in a context that is supplied to the logging calls.        */
/*                                                                           */
static char   logfile_host[MAXHOSTNAMELEN + 1] = "" ;
static int    logfile_pid = 0 ;
static BOOL   add_timestamps_G = FALSE ;		    /* Add timestamps to all messages, not
							       just the initial session one. */


/* Open an existing logfile (or if logfile doesn't exist, create a new one), */
/* and write an initial "session start" message.                             */
/* The file will be opened in the ..../database subdirectory of the database.*/
/*                                                                           */
BOOL openAceLogFile(char *logfile_name, ACEOUT *logfile_out, BOOL new_session)
{
  BOOL result = FALSE ;
  char *logfile_path = NULL ;

  if (!logfile_name || logfile_out == NULL)
    messcrash("Internal coding error, "
	      "logfile cannot be opened because: %s%s.",
	      (logfile_name ? "" : "logfile name is NULL "),
	      (logfile_out ? "" : "pointer to logfile ACEOUT is NULL ")) ;

  /* Need permissions to write into database/ directory.                     */
  if (euid != ruid)
    seteuid(euid);

  /* Should get these from some sort of program wide repository for this     */
  /* information.                                                            */
  logfile_pid = getpid() ; 
  gethostname(logfile_host, MAXHOSTNAMELEN) ;

  /* test whether we can append to avoid messerror from aceOutCreateToFile() */
  logfile_path = dbPathStrictFilName ("database", logfile_name, "wrm", "a", 0);

  /* ok, open the logfile.                                                   */
  if (logfile_path)
    {
      ACEOUT new_logfile = NULL ;

      if ((new_logfile = aceOutCreateToFile (logfile_path, "a", 0)))
	{
	  *logfile_out = new_logfile ;
	  result = TRUE ;
	}
      messfree(logfile_path);
    }

  /* write an intial message that can be used to identify the start of a     */
  /* programs session with the database.                                     */
  if (result && new_session)
    {
      aceOutPrint(*logfile_out, "\n") ;
      logPrintPrefix(*logfile_out, logfile_host, logfile_pid, TRUE) ;
      aceOutPrint(*logfile_out, "SESSION_START -  User:%s, Program:%s, Level:%s, %s\n",
		  getLogin(TRUE), messGetErrorProgram(),
		  aceGetVersionString(), aceGetLinkDateString()) ;
      aceOutFlush(*logfile_out) ;
    }


  if (euid != ruid)
    seteuid(ruid);

  return result ;
}


/* Doing the time stamping is expensive computationally so this will allow this features to be
 * turned on/off for all messages except the session start/end messages. */
void setTimeStampsAceLogFile(BOOL timestamps_on)
{
  add_timestamps_G = timestamps_on ;
}


/* Add a message to the log.                                                 */
/*                                                                           */
BOOL appendAceLogFile(ACEOUT logfile_out, char *text)
{
  BOOL result = FALSE ;

  if (logfile_out == NULL || text == NULL)
    messcrash("Internal coding error, "
	      "logfile cannot be appended to because: %s%s.",
	      (logfile_out ? "" : "logfile ACEOUT is NULL "),
	      (text ? "" : "message text is NULL")) ;

  logPrintPrefix(logfile_out, logfile_host, logfile_pid, add_timestamps_G) ;
  aceOutPrint(logfile_out, "%s\n", text) ;
  aceOutFlush(logfile_out) ;

  result = TRUE ;

  return result ;
}


/* Close the log file and put in a closing message for the session.          */
/*                                                                           */
BOOL closeAceLogFile(ACEOUT logfile_out, aceLogExitType exit_type)
{
  BOOL result = FALSE ;

  if (logfile_out == NULL)
    messcrash("Internal coding error, "
	      "logfile cannot be closed because logfile ACEOUT is NULL.") ;


  if (exit_type != ACELOG_NORMAL_EXIT && exit_type != ACELOG_ABNORMAL_EXIT
      && exit_type != ACELOG_CRASH_EXIT
      && exit_type != ACELOG_CLOSE_LOG)
    messcrash("Internal coding error, "
	      "logfile cannot be closed because aceLogExitType is invalid: %d",
	      exit_type) ;

  if (exit_type != ACELOG_CLOSE_LOG)
    {
      char *text ;

      text = aceLogExitStrings[exit_type] ;

      logPrintPrefix(logfile_out, logfile_host, logfile_pid, TRUE) ;
      aceOutPrint(logfile_out, "SESSION_END -  %s\n", text) ;
      aceOutPrint(logfile_out, "\n") ;
    }

  aceOutFlush(logfile_out) ;
  aceOutDestroy (logfile_out);

  result = TRUE ;

  return result ;
}

/* Rename the existing log file and start a new one. Gets called either when */
/* requested by user or when database is bootstrapped (start new log because */
/* old one is really only applicable to previous incarnation of database.    */
/*                                                                           */
/* The messages in logfiles only really apply to the database
 * in its current state, if the database is being rebuilt the
 * previous log becomes inappropriate.
 * The previous log is therefore moved to a backup location,
 * and a new logfile is started at this point (SANgc02773) */
/*                                                                           */
/* if log_file_name is NULL then old log is called "log.<ace-time>.wrm"      */
/* if log_msg is non-NULL then its written to end of old log.                */
/*                                                                           */
/* The old log always gets closed.                                           */
/* If we fail to open the new log we return NULL, otherwise we return an     */
/* ACEOUT to the opened log.                                                 */
/*                                                                           */
ACEOUT startNewAceLogFile(ACEOUT current_log, char *old_logname, BOOL timestamp,
			  char *log_msg, BOOL new_session)
{
  ACEOUT result = NULL ;
  STORE_HANDLE handle = NULL ;
  BOOL status = TRUE ;
  char *current_logname = NULL ;


  handle = handleCreate() ;

  if (current_log == NULL)
    messcrash("Internal coding error: start of new log file failed because startNewAceLogFile() "
	      "was passed a NULL log ptr.") ;

  /* Print final message to log ?                                            */
  if (log_msg)
    messdump(log_msg) ;

  /* Make sure existing log is flushed but _NOT_ closed, otherwise the below */
  /* messerrors will cause a crash because the log has disappeared from      */
  /* underneath their feet !                                                 */
  aceOutFlush(current_log) ;


  current_logname = strnew(filGetFilenameBase(aceOutGetFilename(current_log)), 0) ;

  /* Check we can write to the "oldlogs" directory (may need to create it).  */
  if (status)
    {
      if (dbPathStrictFilName("database", current_logname, "wrm", "r", handle))
	{
	  if (!dbPathStrictFilName("database", "oldlogs", 0, "wd", handle))
	    {
	      /* There is no "oldlogs" directory so create it.               */
	      if (!dbPathStrictFilName("database", "oldlogs", 0, "rd", handle))
		{
		  if (mkdir(dbPathMakeFilName("database", "oldlogs", 0, handle), 0755) == -1)
		    {
		      /* during init we shouldn't have problems with this, that's
			 why we output an error (could be NFS timeout, because
			 permissions should be ok) */
		      messerror("failed to create directory database/oldlogs/ (%s)",
				messSysErrorText());
		      status = FALSE ;
		    }
		}
	      else if (chmod (dbPathMakeFilName("database", "oldlogs", 0, handle), 0755) == -1)
		{
		  /* There is an "oldlogs" directory but perms are wrong.    */
		  messerror("failed to chmod 755 directory database/oldlogs/ (%s)",
			    messSysErrorText());
		  status = FALSE ;
		}
	    }
	}
    }

  /* Rename/move existing log into "oldlogs" directory.                      */
  if (status)
    {
      char logFileBackupName[MAXPATHLEN];		    /* This is not actually going to do
							       it, sadly MAXPATHLEN does not do
							       what you think ! */
      char *logFileBackupPath;

      /* Make sure existing log is flushed and closed. */   
      /* Note that we have to do this _before_ renaming, 
	 at least on cygwin */

      aceOutFlush(current_log) ;
      aceOutDestroy(current_log);
      
      if (timestamp)
	sprintf(logFileBackupName, "%s-%s", old_logname, timeShow(timeNow()));
      else
	sprintf(logFileBackupName, "%s", old_logname) ;
      
      logFileBackupPath = dbPathStrictFilName("database/oldlogs", logFileBackupName, "wrm",
					      "w", handle);

      if ((!logFileBackupPath)
	  || (rename(dbPathStrictFilName("database", current_logname, "wrm", "r", handle),
		     logFileBackupPath) == -1))
	{
	  messerror ("failed to rename previous log-file "
		     "%s to %s (%s)",
		     dbPathStrictFilName("database", current_logname, "wrm", "r", handle),
		     logFileBackupPath, 
		     messSysErrorText()); 
	  status = FALSE ;
	}
    }

  /* If all is still OK then open the new log file.                          */
  if (status)
    {
      if (!openAceLogFile(current_logname, &result, new_session))
	{
	  result = NULL ;
	}
    }

  if (handle)
    handleDestroy(handle);

  return result ;
}

/* Just close and reopen the existing logfile. This routine can be used in   */
/* conjunction with utilities such as the Linux "rotatelogs" program to      */
/* allow acedb logs to be manipulated by system utilities while acedb        */
/* programs continue to use the log files uninterrupted.                     */
/*                                                                           */
BOOL reopenAceLogFile(ACEOUT *current_log_inout)
{
  BOOL result = FALSE ;
  char *current_logname = NULL ;

  if (current_log_inout == NULL || *current_log_inout == NULL)
    messcrash("Internal coding error: start of new log file failed because reopenAceLogFile() "
	      "was passed a NULL logfile ptr.") ;

  current_logname = strnew(filGetFilenameBase(aceOutGetFilename(*current_log_inout)), 0) ;

  /* Do the close and reopen.                                                */
   if ((result = closeAceLogFile(*current_log_inout, ACELOG_CLOSE_LOG)))
    {
      result = openAceLogFile(current_logname, current_log_inout, FALSE) ;
    }

  return result ;
}



/* Common prefix for all log records:                                        */
/*                 - prepend time, host, pid                                 */
static void logPrintPrefix(ACEOUT logfile_out, char *logfile_host, int logfile_pid, BOOL timestamp)
{
  aceOutPrint(logfile_out, "%s %s %d\t",
	      (timestamp ? timeShow(timeNow()) : ""),
	      logfile_host, logfile_pid) ;

  return ;
}

