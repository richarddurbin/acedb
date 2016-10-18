/*  File: tacemain.c
 *  Author: Jean Thierry-Mieg (mieg@crbm.cnrs-mop.fr)
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
 *	Jean Thierry-Mieg (CRBM du CNRS, France) mieg@crbm.cnrs-mop.fr
 *
 * Description: main function for a text-only acedb interface
 *              
 * Exported functions:
 * HISTORY:
 * Last edited: Sep 24 11:14 2008 (edgrif)
 * * Mar 22 14:44 1999 (edgrif): Replaced messErrorInit with messSetMsgInfo
 * * Dec  2 16:22 1998 (edgrif): Corrected decl. of main, added code
 *              to record build time of this module.
 * * Jun  7 16:49 1996 (srk)
 * * May 15 12:37 1992 (mieg): This file used to be called querymain
 * *                  I import an adapt parseInput from DKFZ
 * Created: Fri May 15 12:27:28 1992 (mieg)
 *-------------------------------------------------------------------
 */

/* $Id: tacemain.c,v 1.32 2008-09-24 10:28:33 edgrif Exp $ */

#include <locale.h>
#include <wh/acedb.h>
#include <wh/aceio.h>
#include <wh/apputils.h>
#include <wh/session.h>
#include <wh/command.h>
#include <wh/banner.h>
#include <wh/acelib.h>
#include <wh/sigsubs.h>
#include <wh/version.h>
#include <wh/aceversion.h>
#include <wh/help.h>
#include <wh/pref.h>

/**************************************************/
/******* ACEDB non graphic user interface  ********/
/**************************************************/

/* Defines a routine to return the compile date of this file.                */
UT_MAKE_GETCOMPILEDATEROUTINE()

int main (int argc, char **argv)
     /* main function used for tace and tacembly */
{
  BOOL interactiveInput;
  BOOL save_db ;
  BOOL get_write_access ;
  ACEIN fi = 0 ;
  ACEOUT fo = 0 ;
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

#ifdef ACEMBLY
  bannerWrite (bannerMainStrings ("tacembly", FALSE, TRUE)) ;
#else
  bannerWrite (bannerMainStrings ("tace", FALSE, FALSE)) ;
#endif

  /* Check "-tsuser" option to allow a different user to be specified as the database 
   * user to be recorded in the timestamps. */
  appTSUser(&argc, argv) ;

  
  /* Note, sessionInit will now make sure acedb-specific cleanup tasks
   * are performed upon messExit/Crash */
  sessionInit(argc > 1 ? argv[argc - 1] : 0) ;

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
		  (getCmdLineOption(&argc, argv, NOSIGCATCH_OPT, NULL) ? TRUE : FALSE)) ;


  bannerWrite (bannerDataInfoStrings ()); /* must come after aceInit() */
    
  /*****************************/

#ifdef ACEMBLY
  acemblyInit () ;
#endif

  /*****************************/

  /* If a user has write access they get to see more commands than if they   */
  /* can only read.                                                          */
  command_set = (CHOIX_UNIVERSAL | CHOIX_NONSERVER) ;
  if (writeAccessPossible())
    perm_set = (PERM_READ | PERM_WRITE | PERM_ADMIN) ;
  else
    perm_set = PERM_READ ;
  commandExecute(fi, fo, command_set, perm_set, 0) ;


  /*****************************/

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
  aceOutDestroy (fo) ;

  printf ("\n// A bientot \n\n") ;

  return (EXIT_SUCCESS) ;
} /* main */

/***************************** eof **************************/
