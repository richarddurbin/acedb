 /*  File: fmapblast.c
 *  Author: Richard Durbin (rd@mrc-lmb.cam.ac.uk)
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
 *	Jean Thierry-Mieg (CRBM du CNRS, France) mieg@kaa.cnrs-mop.fr
 *
 * Description: Interface to BLAST, Oligo selection program
 * HISTORY:
 * Last edited: Jan 26 16:02 2003 (edgrif)
 * * Jul 16 10:09 1998 (edgrif): Introduce private header fmap_.h
 * Created: Tue Oct 7 1996 (mieg)
 *-------------------------------------------------------------------
 */

/* $Id: fmapblast.c,v 1.23 2003-02-06 14:30:06 edgrif Exp $ */

#define ARRAY_CHECK
#define MALLOC_CHECK

#include "acedb.h"
#include "aceio.h"
#include "call.h"
#include "dna.h"
#include "query.h"
#include "session.h"
#include "dbpath.h"
#include "parse.h"
#include "lex.h"
#include "whooks/systags.h"
#include "whooks/classes.h"

#include "display.h"
#include <w7/fmap_.h>

static Graph blastControlGraph = 0 ;
static void fMapBlastControl (void) ;
static void fMapBlastControlDraw (void) ;
static FeatureMap BLASTGET (char *title) ;
static void popBlastControl(void) ;
static void fMapBlastX_pir (void) ;
static void fMapBlastX_nr (void) ;
static void fMapBlastN (void) ;

static BOOL running = FALSE ;
static int MSPupper = 50 ; /* limit for conserved hits, see /usr/local/bin/MSPcrunch -C help */

/***************************************************************************************/

void fMapBlastInit (void)
{
  fMapBlastControl () ;
}

/***********************************************************************/
/***************** actual calls to blast *******************************/
/***********************************************************************/

/* i just need to parse the returned file 
   and the look buffer which contains the alignment info 
*/
static void fMapBlastEnd (ACEIN fi, void *lk)
     /* fucntion of type ExternalFuncType (wh/call.h) */
{
  KEY key = 0 ;
  Stack s = (Stack) lk ;
  char *cp ;

  sessionGainWriteAccess () ;
  if (fi && stackExists (s))
    { 
      stackCursor (s, 0) ;
      cp = stackNextText (s) ;
      lexaddkey (cp, &key, _VSequence) ;
      cp = stackNextText (s) ;
      parseBuffer (cp, 0, FALSE) ;
      
      parseAceIn (fi, 0, TRUE) ; /* keepGoing = TRUE */
#ifdef CANNOT_DO_TWICE_BECAUSE_FI_IS_DESTROYED_IN_PARSING
      parseAceIn (fi, 0, TRUE) ; /* do it twice because alias system is bugged */
#endif

    }
  stackDestroy (s) ;
  if ( fMapActivateGraph())
    { graphUnMessage() ;
      displayPreserve () ;
    }
  if (key) display (key, 0, 0) ;
  running = FALSE ;  
  popBlastControl() ;
  fMapBlastControlDraw () ;

  return;
} /* fMapBlastEnd */

static void fMapBlastAny (char *pgm, char *db)
{
  Array dna, colors ; 
  Stack lk = 0 ;
  KEY seqKey ;
  int from, to, seqFrom, seqTo, i ;
  ACETMP tmpFile ;
  FeatureMap look = BLASTGET("fMapBlastAny") ;
  char *seqBit ;
   
  if (!look) return ;
  if (!fMapActive (&dna, &colors, 0, 0))
    {
      messout ("First select a Sequence window") ;
      return ;
    }

  if (!graphCheckEditors(blastControlGraph, TRUE))
    return ;
  fMapFindZone (look, &from, &to, 0) ;
  
  tmpFile = aceTmpCreate ("w", 0);
  if (!tmpFile)
    return;

  seqFrom = from ; seqTo = to-1 ;
  if (!fMapFindSpanSequence (look, &seqKey, &seqFrom, &seqTo))
    { messout ("Can't find parent sequence") ;
      return ;
    }
  seqBit = messprintf ("%s_%d-%d", name(seqKey), seqFrom, seqTo) ;
  dnaDumpFastA (dna, from, to-1, seqBit, aceTmpGetOutput (tmpFile));

  aceTmpClose (tmpFile);

  lk = stackCreate (50) ;
  pushText (lk,	seqBit) ;
  pushText(lk,  messprintf(
   "Sequence %s\nSource %s\n\nSequence %s\nSubsequence %s %d %d \n\n",
   seqBit, name(seqKey), name(seqKey), seqBit, seqFrom, seqTo)) ;

  i = stackMark (lk) ;
  pushText (lk, messprintf("%s %s %s %d", aceTmpGetFileName(tmpFile), pgm, db, MSPupper)) ;
  if ( externalAsynchroneCommand 
       ("blast_search", stackText (lk, i), lk, fMapBlastEnd))
    { displayPreserve() ;
      running = TRUE ;
      popBlastControl() ;
      fMapBlastControlDraw () ;
    }

  aceTmpDestroy (tmpFile);

  return;
} /* fMapBlastAny */

static void fMapBlastX_pir (void)
{ fMapBlastAny ("blastx", "pir") ;
}

static void fMapBlastX_nr (void)
{ fMapBlastAny ("blastx", "nr") ;
}

static void fMapBlastN (void)
{ fMapBlastAny ("blastn", "nr") ;
}

/**************************************************************************/
#ifdef JUNK
static void fMapBlastMail (void)
{
  Array dna, colors ; 
  void  *look ;
  KEY	seqKey ;
  Stack s = stackCreate(50) ;
  OBJ obj ;
  KEY titleKey = 0 ;
  int from, to, origin, i ;
  char buf[2] ;
  DNACPTGET("fMapBlastMail") ;
  char *script;
  
  if(!fMapActive(&dna,&colors,&seqKey, &look))
    { messout("First select a dna window or the Search Active KeySet button") ;
      return ;
    }
  fMapFindZone (look, &from, &to, &origin) ;
  
  if ((obj = bsCreate(seqKey)))
    { 
      bsGetKey (obj, _Title, &titleKey) ;
      bsDestroy (obj) ;
    }
 
  pushText(s, titleKey ? name(titleKey) : "acedb_submission") ;
  catText(s,"  ") ;
  
  buf[1] = 0 ;
  for (i = from ; i < to ;)
    { buf[0] = dnaDecodeChar[((int)arr(dna, i++, char)) & 0xff] ;
      catText(s,buf) ;
    }
  catText(s," &\n") ;
 
  script = dbPathFilName(WSCRIPTS, "blast_mailer", "", "x", 0);
  if (script && (callScript (script, stackText(s,0))== 0))
    messout ("I sent your request to the blast mailer") ;
  else
    messout ("Failed to find script to mail to blast") ;
  if (script)
    messfree(script);

  stackDestroy(s) ;
}

/**************************************************************************/

static void dnacptFastamailMail (void)
{
  Array dna, colors ; 
  void  *look ;
  KEY	seqKey ;
  Stack s = stackCreate(50) ;
  OBJ obj ;
  KEY titleKey = 0 ;
  int from, to, origin, i ;
  char buf[2] ;
  DNACPTGET("dnacptFastaMail") ;
  cahr *script;

  if(!fMapActive(&dna,&colors,&seqKey, &look))
    { messout("First select a dna window or the Search Active KeySet button") ;
      return ;
    }
  fMapFindZone (look, &from, &to, &origin) ;
  
  if ((obj = bsCreate(seqKey)))
    { 
      bsGetKey (obj, _Title, &titleKey) ;
      bsDestroy (obj) ;
    }
 
  pushText(s, titleKey ? name(titleKey) : "acedb_submission") ;
  catText(s,"  ") ;
  buf[1] = 0 ;
  for (i = from ; i < to ;)
    { buf[0] = dnaDecodeChar[((int)arr(dna, i++, char)) & 0xff] ;
      catText(s,buf) ;
    }
  catText(s,"  &\n") ;
 
  script = dbPathFilName(WSCRIPTS, "fastamail_mailer", "", "x", 0);
  if (script && (callScript (script, stackText(s,0)) == 0))
    messout ("I sent your request to the fasta mailer") ;
  else
    messout ("failed to find script to mail to fasta") ;
  
  if (script)
    messfree(script);
  
  stackDestroy(s) ;
}

#endif /* JUNK */

/***********************************************************************/
/****************** Blast control graph ********************************/
/***********************************************************************/

static int selecting = 0 ;

static void popBlastControl(void)
{
  if (graphActivate (blastControlGraph))
    graphPop() ;
}

static FeatureMap BLASTGET (char *title)
{
  FeatureMap look ; 
  
  if (!graphActivate (blastControlGraph))
    { blastControlGraph = 0 ; return 0 ; }
  graphCheckEditors(blastControlGraph, TRUE) ;

  if (selecting == 1) selecting = 0 ;
  fMapBlastControlDraw () ;
  if (!fMapActive (0,0,0,&look)) 
    { messout("Sorry, no active sequence, I cannot proceed") ;
      return 0 ;
    }
  graphUnMessage () ;
  graphActivate (look->graph) ;

  return look ;
}

/********************
commented out, seems useless
static void blastRedraw (void)
{
  FeatureMap look = BLASTGET ("blastRedraw") ;
  if (!look) return ;

  fMapDraw(look,0) ;   
  popBlastControl() ;
  fMapBlastControlDraw () ;
}


******************************************************/

static MENUOPT blastControlMenu[] =
{ { graphDestroy, "Quit" },
  { help, "Help" },
  { 0, 0 }
} ;

/*****************************************************************/

static BOOL MSPlimitcheck (int n)
{ return (n <= 80 && n >= 30) ;
}

static void fMapBlastControlDraw (void)
{ 
  int line = 3;
  Graph old = graphActive () ;


  if (!graphActivate (blastControlGraph))
    return ;
  graphClear () ;
  graphText ("A direct interface to BLAST",
	     2, line++) ;
  graphText (" the homology search program",
	     2, line++) ;
  graphText (" from the NCBI",
	     2, line++) ;
  line++ ;

  if (running)
    {
      graphText ("I sent your request to blast", 3, line++) ; 
      graphText ("on the server defined in the file $ACEDB/wscripts/blast-search", 3, line++) ; 
      line += 2 ;
      graphText ("It takes around 5 minutes to search a 5 kb", 3, line++) ;
      graphText ("fragment against the nr database", 3, line++) ;
      graphText ("During the search you can keep working", 3, line++) ;
      graphText ("but you should not edit the searched sequence", 3, line++) ;
      line += 2 ;
    }
  else
    {
      graphText ("It takes around 5 minutes to search a 5 kb", 3, line++) ;
      graphText ("fragment against the nr database", 3, line++) ;
      graphText ("During the search you can keep working", 3, line++) ;
      graphText ("but you should not edit the searched sequence", 3, line++) ;

      line++ ;
      graphText ("If all works", 3, line++) ;
      graphText ("   a new window will pop out with the results", 3, line++) ;
      graphText ("If not", 3, line++) ;
      graphText ("   check the script wscripts/blast_search", 3, line++) ;
      graphText ("This is a new tool, please send comments", 3, line++) ;
      line += 3 ;
      graphButton ("BlastX/PIR:", fMapBlastX_pir, 2,line) ;  /* line += 2.2 ; */
      graphButton ("BlastX/nr:", fMapBlastX_nr, 15,line) ;  /* line += 2.2 ; */
      graphButton ("BlastN/nr:", fMapBlastN, 30,line) ;  line += 2.2 ;
      graphIntEditor ("Minimal score [30-80]", &MSPupper, 18, line++, MSPlimitcheck) ; 
      graphText ("Compare the 6-frame translations against", 4,line++) ; 
      graphText ("the protein database PIR", 4,line) ; 
      line += 2.2 ;
      /*
      graphButton("tBlastX: translate 6 frames and blast against the 6 translations of the dna database nr", fMapBlastN, 2,line) ; line += 2.2 ;  
      */
    }

  graphButton ("Help", help, 3,line) ; 
  graphButton ("Comments", acedbMailComments, 13,line) ; line += 2 ;
  if (!running) graphButton ("Cancel", graphDestroy, 3,line) ;
  graphRedraw () ;
  graphActivate (old) ;

}

static void fMapBlastControlDestroy (void)
{
  blastControlGraph = 0 ;
}

static void fMapBlastControl (void)
{
  Graph old = graphActive () ;
  
  if (graphActivate (blastControlGraph))
    { graphPop () ;
    return ;
    }
  
  blastControlGraph = graphCreate (TEXT_SCROLL, "BLAST parameters", 
				 0, 0, 0.6, 0.5) ;
  graphHelp("BLAST") ;
  graphRegister (DESTROY, fMapBlastControlDestroy) ;
  graphMenu (blastControlMenu) ;
  
  fMapBlastControlDraw () ;
  graphActivate (old) ;
}

/***************************************************************************************/
/***************************************************************************************/
