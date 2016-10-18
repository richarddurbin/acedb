/*  File: statusdisp.c
 *  Author: Fred Wobus (fw@sanger.ac.uk)
 *  Copyright (C) J Thierry-Mieg and R Durbin, 1999
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
 *	Jean Thierry-Mieg (CRBM du CNRS, France) mieg@crbm.cnrs-mop.fr
 *
 * Description: graphical display of status information
 * Exported functions:
 *              statusDisplayCreate
 * HISTORY:
 * Last edited: May  4 16:40 2001 (edgrif)
 * * May 12 13:28 1999 (fw): moved all code from graphical status.c here
 * Created: Wed May 12 13:14:57 1999 (fw)
 *-------------------------------------------------------------------
 */

#include <wh/aceio.h>
#include <wh/acedb.h>
#include <wh/status.h>
#include <wh/display.h>
#include <wh/statusdisp.h>
#include <wh/cachedisp.h>				    /* for cacheShow */
#include <wh/pick.h>					    /* for pickList stuff */

/* should probably use the tSelectStatus call instead...                     */
#include "lex.h"					    /* for lexavail */



#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
#include "version.h"		/* for get compile date */
#include "disk.h"		/* for diskavail */
#include "block.h"		/* for blockavail() */
#include "bs.h"			/* for BSstatus BTstatus */
#include "session.h"		/* for isWriteAccess */
#include "bindex.h"		/* for bIndexVersion */
#include "a.h"			/* for aStatus */
#include "dbpath.h"
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */


/************************************************************/
static void statusDestroy(void);
static void lexDetails(void);
static void arrayDoShow (void);
static void arrayDoMark(void);


static int NbMaxArray = 0 ;
static Graph statusGraph = 0;

static MENUOPT statusMenu[]={
        { graphDestroy,        "Quit" },
        { help,                "Help" },
	{ graphPrint,          "Print"},
	{ menuSpacer,          ""},
	{ statusDisplayCreate, "Refresh"},
	{ lexDetails,          "Number of Objects per class"},
#if !defined(MACINTOSH)
	{ arrayDoShow,         "Array report on stderr"},
	{ arrayDoMark,         "Array Mark"},
#endif
#ifdef JUNK /* need to recompile blocksubs with define TESTBLOCK */
        { blockshow,           "About Cache 1"},
#endif
	{ cacheShow,           "About Cache 2"},
        {0,0} };

/************************************************************/

/* This is just the same as the status command from tace, only its output to */
/* a window. Its a display of general acedb stats.                           */
/*                                                                           */
void statusDisplayCreate (void)
{
  Stack s = NULL ;
  ACEOUT fo = NULL ;
  ACEIN fi = NULL ;
  char *text = NULL ;
  char *cp = NULL ;
  int line ;

  if(graphActivate(statusGraph))
    {
      graphPop() ;
      graphClear();
      graphTextBounds (100,15) ;
    }
  else
    { 
      statusGraph =  displayCreate("DtStatus");
      
      graphMenu (statusMenu);
      graphRegister (DESTROY, statusDestroy) ;
      graphTextBounds (100,15) ;
      graphColor (BLACK) ;
    }

  /* Seems incredibly cack-handed: we make a stack and then an ACEOUT to     */
  /* receive the output from tStatus(), but then because graph can't cope    */
  /* with newlines properly, we have to get the text from the stack and put  */
  /* it into an ACEIN so we can read it a line at a time...sigh...and set    */
  /* up the graph correctly.                                                 */
  /*                                                                         */
  s = stackCreate(1000) ;
  stackTextOnly(s) ;
  fo = aceOutCreateToStack(s, 0) ;

  tStatus(fo) ;

  text = popText(s) ;
  fi = aceInCreateFromText(text, "", 0) ;
  aceInSpecial(fi, "\n\t") ;				    /* Must reset so as not to ignore
							       lines starting with "//" */
  line = 1 ;
  while ((cp = aceInCard(fi)) != NULL)
    {
      graphText(cp, 2, line) ;
      line++ ;
    }


  graphTextBounds(40, line) ;
  graphRedraw() ;


  /* Clean up all the junk...                                                */
  aceOutDestroy(fo) ;
  aceInDestroy(fi) ;
  messfree(s) ;
  
  return;
}


/********************************************/

static void statusDestroy(void)
{ statusGraph = 0 ; }

static void lexDetails(void)
{
  int i , line = 3 ;
  
  graphClear() ;
  graphPop() ;
  graphTextBounds (100,10) ;
  for(i=0; i<256;i++)
    if(pickType(KEYMAKE(i,0)))
      { line++ ;
	graphText("Class :",4, line) ;
	graphText(pickClass2Word(i),12, line) ;
	graphText(messprintf("%6d entries    %8d char  %8d hashed keys",
			     lexMax(i), vocMax(i), lexHashMax (i)), 25, line) ;
      }
  graphTextBounds (100,(line+5)) ;
  graphRedraw() ;
  
  return;
} /* lexDetails */

static void arrayDoShow (void)
{ arrayReport(NbMaxArray) ; }

static void arrayDoMark(void)
{ NbMaxArray = arrayReportMark() ; }


/********************** eof *********************************/
