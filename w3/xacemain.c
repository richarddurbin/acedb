/*  File: xacemain.c
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
 * Last edited: Sep 24 11:12 2008 (edgrif)
 * * Mar 22 14:45 1999 (edgrif): Replaced messErrorInit with messSetMsgInfo
 * * Dec  3 12:16 1998 (edgrif): Remove externs from within function.
 * * Dec  2 15:52 1998 (edgrif): Add code to record compile time of this module.
 * * Oct 15 10:49 1998 (edgrif): Add new call for graphics initialisation.
 * * Sep  9 16:51 1998 (edgrif): Add call to messErrorInit to record
 *              program name (xace probably) for use in messcrash calls.
 * Created: Tue Oct 22 12:51:04 1991 (mieg)
 * CVS info:   $Id: xacemain.c,v 1.52 2008-09-24 10:28:33 edgrif Exp $
 *-------------------------------------------------------------------
 */

#include <ctype.h>
#include <locale.h>
#include <wh/acedb.h>
#include <wh/aceio.h>
#include <wh/session.h>
#include <wh/banner.h>
#include <wh/model.h>
#include <wh/sigsubs.h>
#include <wh/apputils.h>
#include <wh/version.h>
#include <wh/aceversion.h>
#include <wh/command.h>		/* for gifcommand stuff */
#include <wh/regular.h>
#include <wh/display.h>
#include <wh/acedbgraph.h>
#include <wh/main.h>
#include <wh/dbpath.h>
#include <wh/pref.h>
#include <wh/gex.h>
#include <w3/splash1.xpm>
#include <w3/mainicon.xpm>
#include <wh/fmap.h>
#include <wh/tree.h>



/************************************************************/
static void xaceExitRoutine (void);
static char *xaceRemoteHandler (char *command);

/* Defines a routine to return the compile date of this file. */
UT_MAKE_GETCOMPILEDATEROUTINE()

/********************************************************/
/**********************  ACEDB  *************************/
/********************************************************/

int main (int argc, char **argv)
     /* main function used for xace and xacembly */
{
  char *value;
  char *db;
  char dirName[DIR_BUFFER_SIZE];
  GraphSessionCBstruct graph_cbs = {acedbSwitchMessaging, acedbSwitchMessaging} ;
  char *db_name ;
  BOOL get_write_access ;
  char *locale ;


  /* We cannot cope with anything other than the standard locale. */
  locale = setlocale(LC_ALL, "C") ;


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


  /* Start with write access. */
  get_write_access = getCmdLineOption(&argc, argv, "-writeaccess", NULL) ;


  /* extract arguments for xace */

  if (!getCmdLineOption(&argc, argv, "-nosplash", NULL) && prefValue(SPLASH_SCREEN))
    gexSplash((char **)splash1_xpm);
  
  gexSetIconFromXPM((char *)mainicon);

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

  if (getCmdLineOption (&argc, argv, "-no_tree_edit", NULL))
    {
      treeSetEdit(FALSE) ;
    }


  /* Check "-tsuser" option to allow a different user to be specified as the database 
   * user to be recorded in the timestamps. */
  appTSUser(&argc, argv) ;

  if (getCmdLineOption (&argc, argv, "-fmapcutcoords", NULL))
    {
      fMapSetDefaultCutDataType(FMAP_CUT_OBJDATA) ;
    }


#ifdef ACEMBLY
  bannerWrite (bannerMainStrings ("xacembly", TRUE, TRUE)) ;
#else
  bannerWrite (bannerMainStrings ("xace", TRUE, FALSE)) ;
#endif

  *dirName = 0;
  if (getCmdLineOption(&argc, argv, "-promptdb", NULL) && 
      filqueryopen(dirName, 0, 0, "rd" ,
		   "Acedb: Please select the database you wish to open."))
    db = dirName;
  else 
    db = argc > 1 ? argv[argc-1] : 0;
  
  /* Callback registration                                                   */
  /* Add graph specific callbacks for message output etc.                    */
  sessionRegisterGraphCB(&graph_cbs) ;

  /* Note, sessionInit will now make sure acedb-specific cleanup tasks
   * are performed upon messExit/Crash */
  sessionInit (db) ;
#ifdef ACEMBLY
  acemblyInit () ;
#endif

  if (get_write_access && !sessionGainWriteAccess())
    messcrash ("Cannot get write access.") ;


  /* Optionally set the database name as the window title prefix. */
  if (prefValue(WINDOW_TITLE_PREFIX) && (db_name = sessionDbName()))
    {
      graphSetTitlePrefix(db_name) ;
    }


  /* Set some process resources to unlimited, user is given chance to exit   */
  /* if some resources are too limited.                                      */
  utUnlimitResources(TRUE) ;


  /* once the database is open it is important to deal with signals */
  signalCatchInit(FALSE,					    /* don't init Ctrl-C */
		  (getCmdLineOption(&argc, argv, NOSIGCATCH_OPT, NULL) ? TRUE : FALSE)) ;

  messcrashroutine = xaceExitRoutine; /* override default set 
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

  displayInit();	/* init acedb displays,
			 * parse wspec/displays.wrm etc,
			 * comes after sessionInit (it uses the db) */


  pickCreate();		/* open main window */

#if defined(ACEDB_GTK) && !defined(__CYGWIN__)
  /* Write the X window id of the main window to readlock file 
     for James's scripts. */

  sessionReadlockAppendFileWinID(gexGetWindowId(graphActive()));

#endif

  mainWindowQuitRegister (xaceExitRoutine); /* same close actions
					     * for quit/exit button
					     * as for crash */

  graphRemoteRegister(xaceRemoteHandler); /* register command handler
					   * to receive remote commands*/

  /*****************************/

  /**** MAIN LOOP:  does all the work ****/
  /* the loop returns when the last window (mainWindow) quits */
  graphLoop (FALSE) ;

  /* finish the session and clean up */
  xaceExitRoutine();

  return (EXIT_SUCCESS) ;
} /* main */

/*************************************************/

static void xaceExitRoutine (void)
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

  /* graphFinish () ;
     probably not needed anyway, it causes problems for now */

  if (!getenv("ACEDB_NO_BANNER"))
    printf ("\n\n A bientot\n") ; /* use printf just as bannerWrite does */

  return;
} /* xaceExitRoutine */

/*************************************************/

static char *xaceRemoteHandler (char *command)
     /* returns the text output produced by the command */
     /* command is the data received from the XA_XREMOTE_COMMAND
      * atom and will be freed in the Xt-code */
{
  Stack resultStack;
  char *resultText;
  ACEIN fi;
  ACEOUT fo;
  struct messContextStruct oldOutContext;
  struct messContextStruct newOutContext;
  unsigned int command_set = CHOIX_UNDEFINED ;		    /* MUST be unsigned ints because later */
  unsigned int perm_set = PERM_UNDEFINED ;		    /* we do logic on these variable. */


  gifEntry = gifControl;	/* extend command language to deal
				   with giface commands */


  fi = aceInCreateFromText (command, "", 0);
  /* no subshells ($), no commands (@)
   * recognise ';' as card separator - WARNING: this will disable some 
   *   tace-commands where the ; is needed to submit multiple entries
   *   on one command line using the '=' assignment. */
  aceInSpecial (fi, "\n\t\\/%;");

  resultStack = stackCreate(100);
  fo = aceOutCreateToStack (resultStack, 0);

  /* make sure we also redirect messOut/Error to the same output stack
   * while the remote-command is being executed */
  newOutContext.user_func = acedbPrintMessage;
  newOutContext.user_pointer = fo;

  /* redirect messout/messerror to the same resultStack
   * as the output for the command-execution */
  oldOutContext = messOutRegister (newOutContext);


  /* If a user has write access they get to see more commands than if they   */
  /* can only read.                                                          */
  command_set = (CHOIX_UNIVERSAL | CHOIX_NONSERVER | CHOIX_GIF) ;
  if (writeAccessPossible())
    perm_set = (PERM_READ | PERM_WRITE | PERM_ADMIN) ;
  else
    perm_set = PERM_READ ;
  commandExecute(fi, fo, command_set, perm_set, 0) ;


  gifEntry = 0;

  messOutRegister (oldOutContext);

  /* this buffer will be free'd after data has been sent to the
   * XA_XREMOTE_RESPONSE atom */
  resultText = strnew(popText(resultStack), 0);

  aceInDestroy (fi);
  aceOutDestroy (fo);
  stackDestroy (resultStack);

  messStatus (0);

  return resultText;		/* NOTE: must never be NULL */
} /* xaceRemoteHandler */



/**************************************************/
/**************************************************/








