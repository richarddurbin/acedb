 /*  File: fmapmenes.c
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
 * Description: Interface to MENES, Oligo selection program
 * HISTORY:
 * Last edited: Sep 28 16:57 2001 (edgrif)
 * * Jul 16 10:09 1998 (edgrif): Introduce private header fmap_.h
 * Created: Tue Oct 7 1996 (mieg)
 *-------------------------------------------------------------------
 */

/* $Id: fmapmenes.c,v 1.30 2001-10-03 12:31:11 edgrif Exp $ */

#define ARRAY_CHECK
#define MALLOC_CHECK
#define MENES_S_BIT 0x8000  /* used as a number in fmapselectbox */
#define MENES_O_BIT 0x4000  /* used as a number in fmapselectbox */

#include "acedb.h"
#include "aceio.h"
#include "bump.h"
#include "call.h"
#include "dna.h"
#include "lex.h"
#include "query.h"
#include "dbpath.h"
#include "whooks/systags.h"
#include "whooks/tags.h"
#include "whooks/classes.h"

#include "display.h"
#include "keysetdisp.h"

#include <w7/fmap_.h>

/* #define DEBUG */
#define MYMOTIF  _VMotif

static Graph menesControlGraph = 0 ;
static void fMapMenesControl (void) ;
static void fMapMenesControlDraw (void) ;
static void menesRedraw (void) ;
static FeatureMap MENESGET (char *title) ;

extern void dnaRepaint(Array colors) ;

static BOOL amino = FALSE ;
static int target = 0 ;

static KEY matchSeqTag = 0 , _Restriction = 0 ;
static BOOL running = FALSE ;
static char question [1024] ;
static char *myFasta = 0 ;
static int nActiveTarget = 0, nTargetKs = 0, nWholeKs = 0 , line = 0 ;

/***************************************************************************************/

void fMapMenesInit (void)
{
  if (!matchSeqTag)
    lexaddkey ("Match_sequence", &matchSeqTag, 0) ;
  lexword2key("Restriction",&_Restriction,0) ;
  fMapMenesControl() ;
}

/***************************************************************************************/
/****************** actual calls to menes **********************************************/
/***************************************************************************************/

static void menesExe (void)
{
  Array dna ;  Array colors ; KEY  seqKey ;
  int i, i1, i2, j, j1, j2, n, level, from = 0, to, origin, nbSol, topLine ; 
  
  char c, color, mycolor[5] , *cp = mycolor, 
    *tmpMotif = 0 ;
  FILE *fil, *filFasta, *filMotif ;
  Stack s = 0, s1 = 0, sparm = 0 ;
  KEYSET keySetAlpha = 0, sol = 0 ;
  KEY key ;
  char *cq ;
  BOOL found ;
  OBJ obj ;
  char buf[2] ;
  Graph old = graphActive () ;
  FeatureMap look = MENESGET("dnacptMenes") ;
  char *scriptpath = 0;

  buf[1] = 0 ;

  cp = mycolor ;
  *cp++ = TINT_RED ;
  *cp++ =  TINT_LIGHTGRAY ;
  *cp++ =  TINT_MAGENTA ;
  *cp++ =  TINT_CYAN ;
  *cp++ =  TINT_LIGHTGREEN ;


  s = stackCreate (50) ,
  s1 = stackCreate (50) ;
  sparm = stackCreate (50) ;
 
  if (!(filMotif = filtmpopen(&tmpMotif,"w")))
    goto abort ;

  switch (target)
    {    case 1: /* active sequence */
      if(!look || !fMapActive(&dna,&colors,&seqKey, 0))
	{ messout ("First select a DNA window or the "
		   "'KeySet' button") ;
	  goto abort ;
	}
      fMapFindZone (look, &from, &to, &origin) ;
      /* { Graph old = graphActive () ;
	if (fMapActivateGraph())
	  { mapColSetByName (look->map, "Locator", TRUE) ;
	    mapColSetByName (look->map, "Summary bar", TRUE) ;
	    graphActivate (old) ;
	  }
      }
      */
      fMapReDrawDNA (look) ;
  
      cp = arrp(dna, from, char) ;
      dnaRepaint (colors) ;

      i = to - from ; 
      /* reexporte pour etre sur du dessin */
      if (!(filFasta = filtmpopen(&myFasta,"w")))
	goto abort ;
      fprintf (filFasta, ">query\n") ;
      while (i--)
	fputc(dnaDecodeChar[(int)(*cp++)], filFasta) ;
      break ;
    case 2: /* active keyset */  
    case 3:  /* whole database */
      if (!(filFasta = filopen(myFasta,"","r")))
	goto abort ; 
      break ;
    }


  filclose (filFasta) ;
  graphActivate (old) ;

  if (*question)
    { 
      s1 = stackCreate (50) ;
      pushText (s1, question) ;
      j = 0 ;
      graphText (messprintf ("Question: %s", question),
		 4,line++) ;
      
      
    lao:                /* recursivelly */
      found = FALSE ;
      freeforcecard(stackText (s1, 0)) ;
      freespecial ("") ;  /* % must be passed lower down to menes, unspecial them */
      while (TRUE)
	{ 
	  freenext() ;
	  /* we substitute $motif by the site content of the motif 
	   * and pass to the lower strata those motif we don't know
	   * some of them will be interpreted inside the
	   * findmotif code, some pass outside again
	   */
	  j = freestep ('$') ? 1 : 0 ;
	  c = 0 ;
	  cp = freewordcut(" ()&|!$,[]", &c) ;
	  if (j)
	    {
	      if (lexword2key(cp, &key, MYMOTIF) &&
		  (obj = bsCreate(key)))
		{ BOOL goLow = bsFindTag (obj, _Restriction_Enzyme) || 
		    (_Restriction && bsFindTag (obj, _Restriction)) ;
		if (bsGetData (obj, matchSeqTag, _Text, &cp))
		  {
		    found = TRUE ;
		    buf[0] = '(' ; /* always parenthesize macro substitutions */
		    catText(s, buf) ;
		    if (goLow)
		      {
			cq = messprintf(cp) ; cp = cq ;
			while (*cq) { *cq = freelower(*cq) ; cq++ ;}
		      }
		    catText(s, cp) ;
		    buf[0] = ')' ;
		    catText(s, buf) ;
		  }
		bsDestroy(obj) ;
		continue ;
		}
	      else  /* reestablish the $ sign */
		{ 
		  buf[0] = '$' ;
		  catText(s, buf) ;
		}
	    }

	  if (!c && !cp)
	    break ;
	  if (cp)
	    catText(s, cp) ;
	  buf[0] = c ;
	  catText(s, buf) ;
	}
      
      if (found)
	{ stackClear (s1) ;
	  pushText (s1, stackText (s, 0)) ;
	  stackClear (s) ;
	  goto lao ;
	}
      
      fprintf(filMotif,">motif\n%s\n",stackText (s, 0)) ;
      graphText (messprintf ("Rescanned as: %s", stackText(s,0)),
		 4,line++) ;
      filclose(filMotif) ;
      stackClear (s) ;
      messStatus ("Looking for Motifs") ;
      if (amino)
	pushText (sparm, " PRO ACE ") ;
      if (target == 1)
	pushText (sparm, " DNA SS ACE ") ;
      else
	pushText (sparm, " DNA ACE ") ;
      catText (sparm, " -m ") ;
      catText (sparm, tmpMotif) ;
      catText (sparm, " -s ") ;
      catText (sparm, myFasta) ;
      pushText (sparm, "toto") ; /* to ensure a zero termianted string ? */
    }
  else
    goto abort ;

  scriptpath = dbPathFilName(WSCRIPTS, "bioMotif", "", "x", 0);

  if (!scriptpath || 
      !(fil = callScriptPipe (scriptpath, stackText (sparm, 0))))
    goto abort ;

  graphActivate (menesControlGraph) ;
  level = freesetpipe (fil, "") ;
  freespecial ("\t\n") ;
  n = 0 ;
  topLine = line += 2 ;
  line += 2 ;
  nbSol = 0 ;
  s1 = stackReCreate (s1, 300) ;
  stackClear (s) ;
  if (target > 1) sol = keySetCreate() ;
  while (freecard (level)) /* will close fil */
    { 
      pushText (s1, freepos()) ; 

      j1 = 0 ;
      cp = freeword() ;
      if (!cp)
	continue ;
      switch (*cp)
	{
	case '>':
	  stackClear (s) ;
	  if (*++cp)
	    pushText(s, cp) ;
	  else
	    { freenext() ;
	    if ((cp = freeword()))
	      pushText(s, cp) ;
	    }
	  catText(s, ": ") ;
	  if (sol && lexword2key(cp, &key, _VSequence))
	    keySetInsert (sol, key) ;
	  continue ;
	case 'n':
	  freenext() ;
	  freeint(&i) ;
	  nbSol += i ;
	  continue ;
	case 'm':
	  color = TINT_MAGENTA ;
	  catText (s, cp) ;
	  break ;
	case 'f':
	  color = TINT_LIGHTGREEN ;
	  catText (s, cp) ;
	  break ;
	default:
	  continue ;
	}
      j2 = 0 ;
      while (freeint(&i) && freeint(&j))
	{     /* -1, Plato dixit */
	  if (target == 1)
	    for (i1 = i, i2 = from + i1 - 1 ; i1 <= j ; i1++, i2++)
	      array (colors, i2, char) |= color ;
	  catText (s, messprintf (" %d-%d", i, j)) ;
	  j1++ ; n++ ;
	}
      if (j1 && n < 300) 
	graphText (stackText (s, 0), 3, line++) ;  
      stackClear (s) ;
    }
    
  freeclose (level) ;

  
  graphPop() ;
  graphText (messprintf 
	     ("%d sequences contain %d solutions", sol ? keySetMax(sol) : 1 , nbSol),
	     4,topLine) ;
  if (n > 300)graphText ("Sorry, but only the first 300 are shown", 4,topLine + 1) ;
  graphTextBounds (80,line += n + 2) ;
  graphRedraw () ;

  
  if (target == 1 && fMapActivateGraph() && look)
    {
      if (mapColSetByName (look->map, "Summary bar", -1))
	fMapReDrawDNA (look) ;
      else
	{
	  mapColSetByName (look->map, "Locator", TRUE) ;
	  mapColSetByName (look->map, "Summary bar", TRUE) ; 
	  look->pleaseRecompute = TRUE ;
	  fMapDraw (look, 0) ;
	}
 
    }
  if (sol)
    {
      if (keySetMax(sol))
	keySetNewDisplay(sol, messprintf("Motif %s", question ? question : ""));
      else
	keySetDestroy (sol) ;
    }
  externalFileDisplay ("Motifs", 0, s1) ;
 abort:
  stackDestroy (s) ;
  stackDestroy (s1) ;
  stackDestroy (sparm);
  keySetDestroy (keySetAlpha) ;
  if (scriptpath)
    messfree(scriptpath);
  
  if (tmpMotif) filtmpremove (tmpMotif) ;
}

/***************************************************************************************/
/****************** Menes control graph ************************************************/
/***************************************************************************************/

static void popMenesControl(void)
{
  if (graphActivate (menesControlGraph))
    graphPop() ;
}

static FeatureMap MENESGET (char *title)
{
  FeatureMap look ; 
  
  if (!graphActivate (menesControlGraph))
    { menesControlGraph = 0 ; return 0 ; }
  graphCheckEditors(menesControlGraph, TRUE) ;

  fMapMenesControlDraw () ;
 
  fMapActive(0, 0, 0, &look) ;
  if (look) graphActivate (look->graph) ;

  return look ;
}

/******************************/

static void menesRedraw (void)
{
  FeatureMap look = MENESGET ("menesRedraw") ;
  if (!look) return ;

  fMapDraw(look,0) ;   
  popMenesControl() ;
  fMapMenesControlDraw () ;
}


/*****************************************************************/

static MENUOPT menesControlMenu[] =
{ { graphDestroy, "Quit" },
  { help, "Help" },
  { 0, 0 }
} ;


/*****************************************************************/

static void targetActive (void)
{ 
  FILE *filFasta = 0 ;
  FeatureMap look = MENESGET("dnacptMenes") ;
  Array dna = 0 ;
  char *cp ;
  int i, from,  to, origin ;
  KEY seqKey ;

  target = 0 ;
     
  if(!fMapActive(&dna,0,&seqKey, 0))
    { messout ("First select a DNA window or the "
	       "'KeySet' button") ;
    goto abort ;
    }
  fMapFindZone (look, &from, &to, &origin) ;
  
  cp = arrp(dna, from, char) ;

  i = to - from ;
  nActiveTarget = i ;
  if (!(filFasta = filtmpopen(&myFasta,"w")))
    goto abort ;
  fprintf (filFasta, ">query\n") ;
  while (i--)
    fputc(dnaDecodeChar[(int)(*cp++)], filFasta) ;
  filclose (filFasta) ;

  target = 1 ; 
abort:
  fMapMenesControlDraw () ;
}

static void targetKeyset (void)
{  
  ACEOUT fasta_out = 0;
  KEYSET kSet = 0, keySetAlpha = 0 ;

  target = 0 ;
  if (!keySetActive(&kSet, 0)) 
    { 
      messout ("First select a DNA window") ;
      goto abort ;
    }

  {
    FILE *tmp_fil = filtmpopen (&myFasta,"w");
    if (!tmp_fil)
      goto abort;
    filclose (tmp_fil);		/* just interested in name */
  }
  fasta_out = aceOutCreateToFile (myFasta, "w", 0);
  if (!fasta_out)
    goto abort ;

  keySetAlpha = 
    keySetAlphaHeap(kSet, keySetMax(kSet)) ; 
  
  nTargetKs = dnaDumpFastAKeySet (fasta_out, keySetAlpha);
  
  aceOutDestroy (fasta_out);
  
  if (nTargetKs)
    target = 2 ;
    
abort:
  keySetDestroy (keySetAlpha) ;
  fMapMenesControlDraw () ;

  return;
} /* targetKeyset */

static void targetWhole (void)
{
  ACEOUT fasta_out;

  if (!filCheckName("fulldb","dna","r") ||
      messQuery("You already have a fasta file of the whole database, "
		"Should I reexport it"))
    { 
      KEYSET ks = query (0,"FIND DNA");

      fasta_out = aceOutCreateToFile ("fulldb.dna", "w", 0);
      if (!fasta_out) 
	goto abort ;
      nWholeKs = dnaDumpFastAKeySet (fasta_out, ks) ;
      aceOutDestroy (fasta_out);

      keySetDestroy (ks) ;
    }

  /* XXXXX This is dodgy - myFasta is a char* which comes
   * from filtmpopen in all other cases, where it is allocated by
   * malloc using tempnam(). and eventually free'd using free()
   * in filtmpcleanup(), which may crash .. arrrgh !!! XXXXX */
  myFasta = filGetName("fulldb","dna","r",graphHandle()) ;
  
  target = 3 ;
abort:
  fMapMenesControlDraw () ;

  return;
} /* targetWhole */

static void fMapMenesControlDraw (void)
{ 
  int box ;
  int wlen ;

  if (!graphActivate (menesControlGraph))
    return ;

  graphFitBounds (&mapGraphWidth, &mapGraphHeight) ;
  wlen = mapGraphWidth - 6 ;
  if (wlen <3) return ;
  line = 3 ;
  graphClear () ;
  graphText ("A direct interface to BioMotif",
	     2, line++) ;
  graphText (" A program to search complex dna and proteic motifs ",
	     2, line++) ;
  graphText (" from Gerard Mennessier",
	     2, line++) ;
  line++ ;

  graphText ("Query a la bioMotif", 2, line++) ;
  graphText ("Example: [0,](n) $HindIII  [1,15](nnn) (A OR C)C", 2, line++) ;
  if (!*question) strcpy (question, "[0,]n ") ;
  graphTextScrollEntry (question, 1023, wlen, 3, line, 0) ;
  line += 2 ;
  graphText ("Target", 2, line) ;
  box = graphButton ("Active Zone", targetActive, 12, line) ;
  if (target == 1) 
    { 
      graphBoxDraw (box, BLACK, LIGHTBLUE) ; 
      graphText(messprintf("%d bases",nActiveTarget), 30, line) ; 
    }
  line += 2.2 ;
  box = graphButton ("Active Keyset", targetKeyset, 12, line) ;
  if (target == 2) 
    { 
      graphBoxDraw (box, BLACK, LIGHTBLUE) ; 
      graphText(messprintf("%d sequences",nTargetKs), 30, line) ; 
    }
  line += 2.2 ;
  box = graphButton ("Whole database", targetWhole, 12, line) ;
  if (target == 3) 
    { 
      graphBoxDraw (box, BLACK, LIGHTBLUE) ; 
      graphText(messprintf("%d sequences",nWholeKs), 30, line) ; 
    }
  line += 2.2 ;

  graphButton ("Help", help, 3,line) ; 
  graphButton ("Comments", acedbMailComments, 13,line) ; line += 4 ;
  if (target) graphButton("Search", menesExe, 2, line) ; 
  graphButton ("Clear", menesRedraw, 15,line) ; 
  if (!running) graphButton ("Quit", graphDestroy, 26,line) ;

   line += 3 ;
  graphRedraw () ;
}

static void fMapMenesControlDestroy (void)
{
  menesControlGraph = 0 ;
}

static void fMapMenesControl (void)
{
  Graph old = graphActive () ;
  
  if (graphActivate (menesControlGraph))
    { graphPop () ;
    return ;
    }
  
  menesControlGraph = graphCreate (TEXT_SCROLL, "BioMotif parameters", 
				 0, 0, 0.4, 0.5) ;
  graphHelp("BioMotif") ;
  graphRegister (DESTROY, fMapMenesControlDestroy) ;
  graphMenu (menesControlMenu) ;
  
  fMapMenesControlDraw () ;
  graphActivate (old) ;
}

/***************************************************************************************/
/***************************************************************************************/
