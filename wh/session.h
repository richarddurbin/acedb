/*  File: session.h
 *  Author: Ed Griffiths (edgrif@sanger.ac.uk)
 *  Copyright (C) J Thierry-Mieg and R Durbin, 1998
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
 * 	Richard Durbin (Sanger Centre, UK) rd@sanger.ac.uk, and
 *	Jean Thierry-Mieg (CRBM du CNRS, France) mieg@kaa.crbm.cnrs-mop.fr
 *
 * Description: public functions of sessionsubs.c handling the session.
 * Exported functions:
 * HISTORY:
 * Last edited: Mar 30 14:04 2007 (edgrif)
 * * Dec 23 17:37 1999 (edgrif): Added typedefs for server callbacks.
 * * Jan 25 14:47 1999 (edgrif): Moved getLogin to regular.h
 * * Sep 25 12:24 1998 (edgrif): Added prototype for getLogin()
 * * Richard Bruskiewich
 *		-	declare function prototypes for setuid() et al. for WIN32 & MAC's
 *		-	also can put extern declarations for ruid and euid here
 * * Jun  3 19:08 1996 (rd)
 * Created: Fri Sep 25 12:24:38 1998 (edgrif)
 * CVS info:   $Id: session.h,v 1.79 2007-04-02 09:43:27 edgrif Exp $
 *-------------------------------------------------------------------
 */
#ifndef ACEDB_SESSION_H
#define ACEDB_SESSION_H


#include <wh/log.h>

/* On all but the macintosh the new acedb_version code is used to get the release numbers. */
/* When I've found a macintosh to test on I will replace the code on that to.              */
#if defined(MACINTOSH) 

#if defined(ACEDB5)
#define MAINCODERELEASE 5
#define SUBCODERELEASE 0
#define DATE_OF_CODE_RELEASE "Version 5 is UNRELIABLE,  DO NOT USE IT"
#else
#define MAINCODERELEASE 4
#define SUBCODERELEASE 6
#define DATE_OF_CODE_RELEASE "July 4, 1997"
#endif

#endif /* MACINTOSH */



#ifndef DEF_DISK
#include "disk.h"
#endif

    /* sizeof(SESSION) must remain smaller than BLOCKSIZE */
typedef struct {
                 int session ; /* unique identifier */
                 DISK gAddress ;
		 int mainCodeRelease, subCodeRelease,
		     mainDataRelease, subDataRelease ;
                 char name[80], title[255] ;
		 KEY key, from, upLink, userKey ;
                 int index_version ;
	       } SESSION ;

/************************************************************/

extern SESSION thisSession ;

extern BOOL swapData;

/************************************************************/

void  sessionInit (char *ace_path) ;
void  sessionRegister (void) ;
void  sessionMark (BOOL doSave, int newTimeOut);
void  sessionClose (BOOL doSave) ;
void sessionCloseExit(BOOL doSave, aceLogExitType exit_type) ;
void  sessionShutDown (void) ; /* release all memories */
unsigned int  sessionAutoSave (int s, int tInactivity) ; 
              /* Sets autosaving every s seconds
	       * or tInactivity  seconds after last call to sessionAutoSave
	       * if s == 0, disables autosaving 
	       * iff s < 0, just returns registered interval
	       * ATTENTION: grabs alarm() */
unsigned int sessionTimeSave (unsigned int s, int mode) ;
                        /* mode == 0 :saves and regains write access 
			 * if s seconds have  elapsed since last save 
			 * returns time since last save 
			 * mode == 1,2 private
			 */


void  sessionDoSave (BOOL reGainWriteAccess);   /* tries to save if 
						   write access
						   is allowed, will 
						   re-gain write-access
						   if required. */

BOOL readlockDeleteFile (void);

void sessionReadlockAppendFileWinID(unsigned long id);
void sessionCheckReadlock (void); /* touch readlock or 
				     re-create if lost */

BOOL sessionCheckSuperBlock (void); /* FALSE if the superblock on disk
				       is inconsistent with ours */

/****************** write access functions *************************/

BOOL  sessionGainWriteAccess (void); /* Get if possible */

void  sessionReleaseWriteAccess(void); /* drop write lock and access */

void  sessionForbidWriteAccess (void); /* can never re-gain
					 write access again */

BOOL  sessionAdminUser(char *err_msg) ;			    /* Does current user have admin access */

BOOL  isWriteAccess (void) ;

BOOL  writeAccessPossible(void);

VoidRoutine writeAccessChangeRegister (VoidRoutine func); /* register
							     a function
							     to be 
							     called,
							     if write
							     access
							     changes */
void writeAccessChangeUnRegister (void);


void sessionSetSaveStatus(BOOL save) ;			    /* Turn saving on/off independently of
							       write access. */

void sessionSetforceInit(BOOL value);			    /* Reinitialise the database when
							       database/ACEDB.wrm is missing
							       without asking the user. */


/*******************************************************************/

/* Various functions to get/set session information. */
char *sessionDbName(void) ;				    /* name of database,
							       used to title main-window etc. */
char *sessionDBTitle(void) ;				    /* Title of database, used in status
							       and main-window title. */
char *sessionLastSaveTime(void) ;			    /* Last time database was saved. */

KEY   sessionUserKey (void) ;
BOOL sessionSetUserSessionUser(char *user);		    /* Username must be alphanumerics. */
mytime_t sessionUserSession2Time(KEY userSession);
mytime_t sessionStartTime(void);
void swapSuperBlock();

#if defined (MACINTOSH)
void seteuid (uid_t uid) ;
uid_t getuid () ;
uid_t geteuid () ;
#endif /* Mac */



/* logging routines, most logging is done by making a messdump call from     */
/* the code, but these calls are needed to control the actuall usage of the  */
/* acedb logfiles.                                                           */
/* sessionNewLogFile() starts a new log immediately, if old_log_name is      */
/*   NULL then old log is called "log.<ace-time>.wrm".                       */
/* sessionRequestNewLogFile() requests that a new log file be started the    */
/*   next time a log record is made, needed for use from signal handler.     */
/* sessionReopenLogFile() requests that the existing log be closed and then  */
/*   reopened, can be used in conjunction with unix system "rotatelogs" cmd. */
/*                                                                           */
BOOL sessionNewLogFile(char *old_log_name) ;
void sessionRequestNewLogFile(void) ;
BOOL sessionReopenLogFile(void) ;


/********************** ace graphic callbacks   ******************************/
/*                                                                           */
/* The graphical version of acedb sometimes needs to do some different       */
/* processing for message output etc.                                        */
/*                                                                           */
/* Add any future graph specific stuff to this struct.                       */
/*                                                                           */
typedef void (*GraphTerminateMesgCB)(BOOL msg_list) ;

typedef struct _GraphSessionCBstruct
{
  GraphTerminateMesgCB exit_msg_cb ;
  GraphTerminateMesgCB crash_msg_cb ;
} GraphSessionCBstruct, *GraphSessionCB ;

void sessionRegisterGraphCB(GraphSessionCB callbacks) ;



/********************** ace server callbacks   *******************************/
/*                                                                           */
/* The server needs to do additional processing at various stages of session */
/* processing (e.g. init/termination) it does this via callbacks that it     */
/* registers at server start up, prior to making the call to sessionInit().  */
/*                                                                           */
/* NOTE the philosophy is that the server deserves to be a special case of   */
/* initialising acedb and so it is acceptable that it has its own callback   */
/* interface.                                                                */
/*                                                                           */
/* sessionRegisterServerCB() is called before sessionInit().                 */
/*                                                                           */
typedef void (*ServerCB)(void *server_pointer) ;
typedef void (*ServerMesgCB)(void *server_pointer, char *mesg) ;

typedef struct _ServerContextStruct
{
  union u_funcs
  {
    ServerCB func ;
    ServerMesgCB mesgfunc ;
  } funcs ;

  void *data ;
} ServerContextStruct ;


typedef struct _ServerSessionCBstruct
{
  ServerContextStruct init ;
  ServerContextStruct checkFiles ;
  struct messContextStruct mess_out ;			    /* Directly register with messout */
  struct messContextStruct mess_err ;			    /* Directly register with messerror */
  ServerContextStruct exit ;
  ServerContextStruct crash ;
  ServerContextStruct sighup ;
  ServerContextStruct sigterm ;
} ServerSessionCBstruct, *ServerSessionCB ;

void sessionRegisterServerCB(ServerSessionCB callbacks) ;
ServerSessionCB sessionGetServerCB(void) ;



#endif	/* !ACEDB_SESSION_H */
 
