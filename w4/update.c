/*  File: update.c
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
 * Description: Parses the official acedb updates.
 * Exported functions:
 *              updateData
 *              updateDoAction
 * HISTORY:
 * Last edited: Nov  4 11:59 1999 (fw)
 * * Jan 20 10:16 1992 (mieg): interactive access to alias, 
   should be done differently really.
 * Created: Fri Oct 25 13:35:14 1991 (mieg)
 *-------------------------------------------------------------------
 */

/*$Id: update.c,v 1.38 2000-02-07 11:07:52 srk Exp $ */

#include "acedb.h"
#include "aceio.h"
#include "session.h"
#include "parse.h"
#include "model.h"
#include "lex.h"		/* for lexAlphaMakeAll */
#include "bs.h"			/* for global BOOL flags */

#include "display.h"
#include "main.h"		/* pickDraw */
#include "vmap.h"		/* for vMapMakeAll() */
#include "dbpath.h"

/*************************************************************/

static int lineBox ;

/*************************************************************/
/*************************************************************/

static Graph updateGraph = 0;
static int line = 3 ;
static void upMessage(char *cp)
{
  Graph old = graphActive() ;
  if(graphActivate(updateGraph))
    { graphText (cp, 1, line++) ;
      graphTextBounds (80,line+2) ;
      graphBoxDraw (0,-1,-1) ;
    }
  graphActivate(old) ;
}
    
/******************************************/

static void updateAlign (void)
{
  extern void gMapMakeAll(void) ;
  extern void pMapMakeAll(void) ;
  extern void cMapMakeAll(void) ;

  upMessage ("Making physical maps - a few minutes") ;
  pMapMakeAll () ;

  upMessage ("Making genetic maps - a minute or two") ;
  vMapMakeAll () ;
  gMapMakeAll () ;

  upMessage ("Making physical chromo maps - a minute or two") ;
  cMapMakeAll () ;

  upMessage ("Sorting the lexiques - a minute or two") ;
  lexAlphaMakeAll () ;

  upMessage ("I save to disk") ;
  sessionClose (TRUE);

  pickDraw () ;			/* updates title bar */

  upMessage ("Update complete") ;
  if (sessionDbName())
    messout ("We are now using data release %s %d-%d",
	     sessionDbName(),
	     thisSession.mainDataRelease, thisSession.subDataRelease) ;
  else
    messout ("We are now using data release %d-%d",
	     thisSession.mainDataRelease, thisSession.subDataRelease) ;
}

/******************************************/

static BOOL updateCheckFile (ACEIN update_io)
{ 
  int mainRelNum = -1, subRelNum = -1 ;
  char *first_line;

 /* When reading the first line we need to get the comment line.
  * Although it may seem dangerous to tinker with the speciaql here,
  * the update_io will be passed to the parsing module which resets
  * the special anyway. */
  aceInSpecial (update_io, "\n");

  first_line = aceInCard (update_io);

  if (sscanf (first_line, "// acedb update %d %d\n",
	      &mainRelNum, &subRelNum) != 2)
    { 
      messout ("The first line of the update does not have the "
	       "correct format.  Abandoning update.") ;
      return FALSE ;
    }

  if (mainRelNum != thisSession.mainDataRelease ||
      subRelNum != thisSession.subDataRelease+1)
    { 
      messout  ("Update %d.%d does not match the current state",
		mainRelNum, subRelNum) ;
      return FALSE ;
    }
  return TRUE ;
} /* updateCheckFile */

/******************************************/

static BOOL updateParseAceIn (ACEIN update_io)
     /* destroys update_io */
{
  BOOL ok ;

  messStatus (messprintf ("Adding update %d", 
			    thisSession.subDataRelease + 1)) ;
  upMessage ("Update started. This may take a few minutes please wait.") ;

  XREF_DISABLED_FOR_UPDATE = TRUE ;
  ok = parseAceIn (update_io, 0, FALSE) ;  /* destroys update_io */
  XREF_DISABLED_FOR_UPDATE = FALSE ;

  return ok ;
} /* updateParseAceIn */

/******************************************/

void updateDoAction (BOOL doAll)
{
  VoidRoutine previousCrashRoutine ;
  char fileName[256] , * fn;
  BOOL doAlign = FALSE ;
  ACEIN update_io;
  char *rawdir = dbPathStrictFilName("rawdata", "", "", "rd", 0);

#if !defined(MACINTOSH)
  if (getenv ("ACEDB_DATA"))
    strcpy (fileName, getenv("ACEDB_DATA")) ;
  else
#endif
  if (!rawdir)
    { messout ("Sorry, can't find rawdata directory") ;
      return ;
    }
  else
    strcpy (fileName, rawdir) ;

  if (rawdir)
    messfree(rawdir);

  if (isWriteAccess())
    { messout("You still have write access\n"
	      "First, save your earlier work") ;
      return ;
    }

  if (!sessionGainWriteAccess())
    { 
      messout ("Sorry, you cannot gain write access\n"
	       "Update impossible.") ;
      return ;
    }

  fn = fileName + strlen(fileName) ;

  previousCrashRoutine = messcrashroutine ;
  messcrashroutine = simpleCrashRoutine ; /* don't allow crash recovery */

  upMessage ("Reading in model file") ;
  if (!getNewModels ())		/* needs write access */
    messExit ("There were errors while reading the models during the update") ;

  while(TRUE)
    { 
      if (!isWriteAccess())
	sessionGainWriteAccess() ;
      if (sessionDbName())
	strcpy (fn, messprintf ("/update.%s.%d-%d",
				sessionDbName(),
				thisSession.mainDataRelease,
				thisSession.subDataRelease + 1)) ;
      else
	strcpy (fn, messprintf ("/update.%d-%d",
			      thisSession.mainDataRelease,
			      thisSession.subDataRelease + 1)) ;
      upMessage (messprintf("Looking for file %s",fileName)) ;
      upMessage (timeShow(timeNow())) ;
      if (!filCheckName (fileName, "", "r"))
	{ 
	  if (!doAlign)
	    messout ("Could not find file %s",fileName) ;
	  else
	    upMessage (messprintf ("Could not find file %s",
				   fileName)) ;
	  break ;
	}

      /* note: these files are server side (in rawdata directory) */
      update_io = aceInCreateFromFile (fileName, "r", "", 0);

      if (!update_io) /* should not fail */
	{ 
	  messcrash ("updateDoAction() - inconsistency "
		     "filName(%s) succeeded, "
		     "but now we can't make an ACEIN from that file.", 
		     fileName) ;
	  break ;
	}
      
      messStatus ("Updating, please wait.") ;
      if (!updateCheckFile (update_io))
	{
	  aceInDestroy (update_io);
	  break ;
	}

      if (!updateParseAceIn (update_io))  /* will destroy update_io */
	messExit ("There were errors while parsing the "
		   "official data: I exit.") ;

      thisSession.subDataRelease++ ;
      doAlign = TRUE ;
      sessionClose(TRUE) ;
      if (!doAll && !messQuery
	  (messprintf ("We are now using data release %d-%d\n%s",
		       thisSession.mainDataRelease,
		       thisSession.subDataRelease,
		       "Do you want to read the next update file ? ")))
	break ;
    }
  upMessage (timeShow(timeNow())) ;
  if (doAlign)
    { if (!isWriteAccess())
	sessionGainWriteAccess() ;
      updateAlign () ;
    }
  sessionClose(TRUE) ;  
  upMessage (timeShow(timeNow())) ;
  messcrashroutine = previousCrashRoutine; /* reestablish crash recovery */

  return;
} /* updateDoAction */

/******************************************/

static void updateAction(void)
{ updateDoAction(FALSE) ;
}

static void allUpdateAction(void)
{ updateDoAction(TRUE) ;
}

/******************************************/

static void updateDestroy (void)
{
  updateGraph = 0 ;
}

/*********************************************/
/************* public routine ****************/

static MENUOPT updateMenu[]=
{
  { graphDestroy,	"Quit" },
  { help,		"Help" },
  { updateAction,	"Next Update" },
  { allUpdateAction,	"All Updates" },
  { 0,0 }
};

/********************************************/

void updateData(void)
{
  if (!graphActivate(updateGraph))
    {
      updateGraph = displayCreate("DtUpdate") ;

      graphMenu (updateMenu);
      graphTextBounds (80,100) ;
      graphRegister(DESTROY,updateDestroy) ;
      graphColor (BLACK) ;
      graphButtons (updateMenu, 1, 1, 50) ;
      graphText ("Line:", 2, 2.5) ;
      lineBox = graphBoxStart () ;
      /*
       *parseLineText = 0 ;
       graphTextPtr (parseLineText, 8, 2.5, 60) ;
       */
      graphBoxEnd () ;
      line = 4 ;
      if (sessionDbName())
	upMessage (messprintf ("We are now using data release %s %d-%d",
			     sessionDbName(),
			     thisSession.mainDataRelease,
			     thisSession.subDataRelease)) ;
      else
	upMessage (messprintf ("We are now using data release %d-%d", /*  */
			     thisSession.mainDataRelease,
			     thisSession.subDataRelease)) ;
      graphRedraw () ;
    }
  else
    graphPop() ;
}
 
/*********************************************/
/*********************************************/

 
