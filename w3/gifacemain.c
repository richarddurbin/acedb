/*  File: gifacemain.c
 *  Author: Richard Durbin (rd@sanger.ac.uk)
 *  Copyright (C) J Thierry-Mieg and R Durbin, 1996
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
 * 	Richard Durbin (MRC LMB, UK) rd@sanger.ac.uk, and
 *	Jean Thierry-Mieg (CRBM du CNRS, France) mieg@crbm.cnrs-mop.fr
 *
 * SCCS: $Id: gifacemain.c,v 1.36 2008-09-24 10:28:33 edgrif Exp $ 
 * Description:
 * Exported functions:
 * HISTORY:
 * Last edited: Sep 24 11:13 2008 (edgrif)
 * * Mar 22 14:44 1999 (edgrif): Replaced messErrorInit with messSetMsgInfo
 * * Jan 27 13:44 1999 (fw): moved gifControl() function to 
 *              a separate module in gifcommand.c
 * * Jan 21 14:42 1999 (edgrif): Added EPSF parameter to graphPS call.
 * * Jan  8 10:34 1999 (edgrif): Fixed two bugs: 1) call to graphPS was
 *              completely incorrect, wrong params !!
 *              2) display option incorrectly freed keyset supplied to it.
 * * Dec  2 16:20 1998 (edgrif): Corrected decl. of main, added code
 *              to record build time of this module.
 * * Oct 15 11:30 1998 (edgrif): Add new graph/acedb initialisation call.
 * * Sep 15 10:29 1998 (edgrif): Correct faulty main declaration, add
 *              call to allow inserting program name into crash messages.
 * * Jul 23 13:54 1998 (edgrif): Remove naughty redeclarations of
 *      fmap functions and instead include the fmap.h public header.
 * * Mar  4 22:33 1996 (rd): built from tacemain.c
 * Created: Mon Mar  4 22:32:03 1996 (rd)
 *-------------------------------------------------------------------
 */

/******************************************************/
/********* ACEDB gif generator for WWW system  ********/
/******************************************************/


#include <locale.h>
#include <wh/acelib.h>
#include <wh/acedb.h>
#include <wh/aceio.h>
#include <wh/apputils.h>
#include <wh/command.h>
#include <wh/session.h>
#include <wh/banner.h>
#include <wh/version.h>
#include <wh/help.h>
#include <wh/display.h>
#include <wh/acedbgraph.h>		/* for acedbAppGraphInit */
#include <wh/sigsubs.h>
#include <wh/pref.h>

/* Defines a routine to return the compile date of this file.                */
UT_MAKE_GETCOMPILEDATEROUTINE()

/************************************************************/

int main (int argc, char **argv)
{
  BOOL signal_catch ;
  BOOL interactiveInput;
  BOOL save_db ;
  BOOL get_write_access ;
  ACEIN fi;
  ACEOUT fo;
  unsigned int command_set = CHOIX_UNDEFINED ;		    /* MUST be unsigned ints because later */
  unsigned int perm_set = PERM_UNDEFINED ;		    /* we do logic on these variable. */
  char *locale ;


  /* We cannot cope with anything other than the standard locale. */
  locale = setlocale(LC_ALL, "C") ;


  /* Set up debugging stuff.                                                 */
  checkCmdLineForSleep(&argc, argv) ;			    /* "-sleep secs" option. */



  /* init message system details. */
  messSetMsgInfo(argv[0], aceGetVersionString()) ;


  /*****************************/

  prefInit() ;						    /* Set up App preferences. */

  /*****************************/

  /* set input/output unbuffered */
  setbuf (stdout, NULL) ;
  setbuf (stderr, NULL) ;

  /**** parse command-line arguments ******/

  /* Does not return if user typed "-version" option.                        */
  appCmdlineVersion(&argc, argv) ;

  /* Is signal handling to be turned off ?                                   */
  signal_catch = getCmdLineOption(&argc, argv, NOSIGCATCH_OPT, NULL) ;
  
  graphInit (&argc, argv);
  acedbGraphInit() ; 
  
  if (getCmdLineOption (&argc, argv, "-noprompt", NULL))
    interactiveInput = FALSE ;
  else
    interactiveInput = TRUE ;
  /* do NOT test stdinIsInteractive(), 
   * this would  changes the behaviour of the
   * code in pipe calls
   */


  /* if called with -init flag, sessionInit() will not prompt before
  ** initialising the database.  
  */
  if (getCmdLineOption (&argc, argv, "-init", NULL))
    sessionSetforceInit(TRUE);

  /* Start with write access. */
  get_write_access = getCmdLineOption(&argc, argv, "-writeaccess", NULL) ;


  /*****************************/

  /* initialize the input/output context */
  fi = aceInCreateFromStdin (interactiveInput, "", 0);
  aceInSpecial (fi,"\n\t\\/@%") ;  /* forbid sub shells */
  fo = aceOutCreateToStdout (0);

  /* register output context for messout, messerror and helpOn */
  {
    struct messContextStruct messContext;
    struct helpContextStruct helpContext;

    messContext.user_func = acedbPrintMessage;
    messContext.user_pointer = (void*)fo;

    helpContext.user_func = helpPrint;
    helpContext.user_pointer = (void*)fo;

    messOutRegister (messContext);
    messErrorRegister (messContext);

    helpOnRegister (helpContext);
  }
  
  /*****************************/

  bannerWrite (bannerMainStrings ("giface", FALSE, FALSE)) ;

  /* Check "-tsuser" option to allow a different user to be specified as the database 
   * user to be recorded in the timestamps. */
  appTSUser(&argc, argv) ;

  /* Note, sessionInit will now make sure acedb-specific cleanup tasks
   * are performed upon messExit/Crash */
  aceInit (argc>1 ? argv[1] : 0) ;

  if (get_write_access && !sessionGainWriteAccess())
    messcrash ("Cannot get write access.") ;

  /* Set some process resources to unlimited, if we are running interactive  */
  /* then user is given chance to exit if some resources are too limited.    */
  if (interactiveInput)
    utUnlimitResources(TRUE) ;
  else
    utUnlimitResources(FALSE) ;

  /* once the database is open it is important to deal with signals */
  signalCatchInit(TRUE,					    /* init Ctrl-C as well */
		  signal_catch) ;

  bannerWrite (bannerDataInfoStrings ()) ; /* must come after aceInit,
					      (data info is read 
					      from superblock) */

  /*****************************/

  displayInit();	     /* init acedb displays,
				parse wspec/displays.wrm etc,
				comes after aceInit (it uses the db) */

  /* Crucial, must set up gif sub menu routine, but also must set global     */
  /* UUUGGGHHH to make sure that gif displays get cleared up properly.       */
  gifEntry = gifControl ;
  isGifDisplay = TRUE ;

#ifdef ACEMBLY
  acemblyInit () ;
#endif


  /* If a user has write access they get to see more commands than if they   */
  /* can only read.                                                          */
  command_set = (CHOIX_UNIVERSAL | CHOIX_NONSERVER | CHOIX_GIF) ;
  if (writeAccessPossible())
    perm_set = (PERM_READ | PERM_WRITE | PERM_ADMIN) ;
  else
    perm_set = PERM_READ ;
  commandExecute(fi, fo, command_set, perm_set, 0) ;


  /* finish the session and clean up */
  /* n.b. if tace is run from a pipe then we should implicitly save all data,*/
  /* this behaviour is expected by many, many scripts.                       */
  if (isWriteAccess() && aceInIsInteractive(fi)
      && messQuery ("You did not save your work, should I ?"))
    save_db = TRUE ;
  else if (isWriteAccess() && !aceInIsInteractive(fi))
    save_db = TRUE ;
  else
    save_db = FALSE ;

  aceQuit(save_db);

  aceInDestroy (fi) ;
  aceOutDestroy (fo);

  printf ("\n// A bientot \n\n") ;

  return (EXIT_SUCCESS) ;
} /* main */


/***************************************************************************/
/* Stubs to make giface link. Once the display code has been enhanced to
   allow provide interactive and display-only sections of data displays
   this should go. */


Graph 
dotter ()
{
  return 0;
}

Graph blxview()
{ 
  return 0;
}

char  *gexTextEditor()
{
  return NULL;
}

void *gexTextEditorNew()
{
  return NULL;
}

char *gexEditorGetText()
{
  return NULL;
}

void gexSetModalDialog(BOOL modal)
{
  return ;
}

int gexGetDefListSize(int curr_value)
{
  return 0 ;
}

BOOL gexCheckListSize(int curr_value)
{
  return FALSE ;
}

void gexSetMessListSize(int list_length)
{
  return ;
}

void gexSetMessPrefs(BOOL use_msg_list, int msg_list_length)
{
  return ;
}

void gexSetMessPopUps(BOOL msg_list, int list_length)
{
  return ;
}


/************************ eof *******************************/
