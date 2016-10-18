/*  File: xclientmain.c
 *  Author: Jean Thierry-Mieg (mieg@mrc-lmb.cam.ac.uk)
 *  Copyright (C) J Thierry-Mieg and R Durbin, 1991
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
 * Description: Main() of the complete program
        Provides Initialisation and Termination routines.
 * Exported functions:
 * HISTORY:
 * Last edited: Jan 14 13:54 2003 (edgrif)
 * * Mar 22 14:45 1999 (edgrif): Replaced messErrorInit with messSetMsgInfo
 * * Dec  2 15:54 1998 (edgrif): Add code to return build time of this module.
 * * Oct 15 10:50 1998 (edgrif): Add new call for graphics initialisation.
 * Created: Tue Oct 22 12:51:04 1991 (mieg)
 *-------------------------------------------------------------------
 */

/* $Id: xclientmain.c,v 1.27 2003-01-14 14:09:01 edgrif Exp $ */

#include <wh/acedb.h>
#include <wh/aceio.h>
#include <wh/session.h>
#include <wh/apputils.h>
#include <wh/banner.h>
#include <wh/model.h>
#include <wh/sigsubs.h>
#include <wh/version.h>
#include <wh/aceversion.h>
#include <wh/dbpath.h>
#include <wh/display.h>
#include <wh/acedbgraph.h>
#include <wh/pref.h>
#include <wh/main.h>
#include <wh/xclient.h>


/************************************************************/
static void xaceclientExitRoutine (void);

/* Defines a routine to return the compile date of this file. */
UT_MAKE_GETCOMPILEDATEROUTINE()

/********************************************************/
/**********************  ACEDB  *************************/
/********************************************************/


int main (int argc, char **argv)
     /* main function used for xaceclient */
{
  char *value;
  GraphSessionCBstruct graph_cbs = {acedbSwitchMessaging, acedbSwitchMessaging} ;

  /* Does not return if user typed "-version" option.                        */
  appCmdlineVersion(&argc, argv) ;


  /*****************************/

  prefInit() ;						    /* Set up App preferences. */

  /*****************************/

  /* Initialise the graphics package and its interface for acedb */
  /* this will also register messout to pop up a model dialog box */
#ifdef ACEDB_GTK
  graphInit (&argc, argv);
  gexInit(&argc, argv);
  acedbGraphInit() ; 
#else
  acedbAppGraphInit(&argc, argv) ;
#endif

  /* init message system details. */
  messSetMsgInfo(argv[0], aceGetVersionString()) ;

  /*****************************/

  /* extract arguments for xace */
  if (getCmdLineOption (&argc, argv, "-initclass", &value))
    {
      mainSetInitClass (value);
      messfree (value);
    }

  if (getCmdLineOption (&argc, argv, "-inittemplate", &value))
    {
      mainSetInitTemplate (value);
      messfree (value);
    }

  /*****************************/

  bannerWrite (bannerMainStrings ("xaceclient", TRUE, FALSE)) ;

  /* Callback registration                                                   */
  /* Add graph specific callbacks for message output etc.                    */

  sessionRegisterGraphCB(&graph_cbs) ;

  /* Check "-tsuser" option to allow a different user to be specified as the database 
   * user to be recorded in the timestamps. */
  appTSUser(&argc, argv) ;

  /* Note, sessionInit will now make sure acedb-specific cleanup tasks
   * are performed upon messExit/Crash */

  sessionInit (argc > 1 ? argv[argc-1]: 0) ;

  /* Set some process resources to unlimited, user is given chance to exit   */
  /* if some resources are too limited.                                      */

  utUnlimitResources(TRUE) ;

  /* once the database is open it is important to deal with signals */

  signalCatchInit(FALSE,			    /* don't init Ctrl-C */
	 (getCmdLineOption(&argc, argv, NOSIGCATCH_OPT, NULL) ? TRUE : FALSE)) ;

  xClientInit () ;

  messcrashroutine = xaceclientExitRoutine; /* override default set 
					     * in sessionInit */

  bannerWrite (bannerDataInfoStrings ()) ; /* NOTE: after sessionInit,
					    * (data info is read 
					    *  from superblock) */

  /*****************************/

  /* Some window specific initialisation which must be done after the        */
  /* database has been opened, e.g. the graph/gex messlist/messpopup stuff   */
  /* can't be done until the pref system is init'd, which can't be done      */
  /* until the database is opened.                                           */
#ifdef ACEDB_GTK
  {
    char *fonts, *rc;

    /* Set messages to appear either in popups or a scrolling list.          */
    acedbInitMessaging() ;

#ifdef __CYGWIN__
    rc = dbPathFilName("wspec", "wingtkrc", NULL, "r", 0);
    if (!rc) 
      rc = dbPathFilName("wspec", "gtkrc", NULL, "r", 0);
    fonts = dbPathFilName("wspec", "winfonts", "wrm", "r", 0);
#else
    rc = dbPathFilName("wspec", "gtkrc", NULL, "r", 0);
    fonts = dbPathFilName("wspec", "xfonts", "wrm", "r", 0);
#endif
    gexSetConfigFiles(rc, fonts);
    if (rc) messfree(rc);
    if (fonts) messfree(fonts);
  }
#endif /* ACEDB_GTK */

  /*****************************/

  displayInit();	     /* init acedb displays,
				parse wspec/displays.wrm etc,
				comes after aceInit (it uses the db) */


  pickCreate();		/* open main window */

  mainWindowQuitRegister (xaceclientExitRoutine); /* same close actions
						     for quit/exit button
						     as for crash */

  /*****************************/

  /**** MAIN LOOP:  does all the work ****/
  /* the loop returns when the last window (mainWindow) quits */
  graphLoop (FALSE) ;

  /* finish the session and clean up */
  xaceclientExitRoutine();

  return (EXIT_SUCCESS) ;
} /* main */

/*************************************************/

static void xaceclientExitRoutine (void)
     /* (1) called by acedbCrash(),  which is called by messcrash()
      *     we use special version for xace instead
      *     of defaultAceCrashRoutine()
      * (2) also called by the Quit and Exit buttons/menus in the
      *     main window.
      */
{
  graphCleanUp () ;  /* kills all open windows,
			forces displayed objects to cache */

  writeAccessChangeUnRegister(); /* avoid unnecessary re-draws
				  * just before exiting */

  if (isWriteAccess() && 
      messQuery ("You did not save your work, should I ?"))
    sessionClose (TRUE);
  else
    sessionClose (FALSE);
   
  filtmpcleanup () ;		/* deletes any temporary files made */
  readlockDeleteFile ();	/* release readlock */
  xClientClose () ;

  /* graphFinish () ;
     probably not needed anyway, it causes problems for now */

  if (!getenv("ACEDB_NO_BANNER"))
    printf ("\n\n A bientot\n") ; /* use printf just as bannerWrite does */

  return;
} /* xaceclientExitRoutine */

/**************************************************/
/**************************************************/
 
 
