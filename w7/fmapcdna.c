/*  File: fmapcdna.c
 *  Author: Danielle et Jean Thierry-Mieg (mieg@mrc-lmba.cam.ac.uk)
 *  Copyright (C) J Thierry-Mieg and R Durbin, 1993
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
 * 	Richard Durbin (MRC LMB, UK) rd@mrc-lmba.cam.ac.uk, and
 *	Jean Thierry-Mieg (CRBM du CNRS, France) mieg@kaa.cnrs-mop.fr
 *
 * Description:
   Display cDNA
 * Exported functions:
 * HISTORY:
 * Last edited: Apr 19 18:42 2002 (edgrif)
 * * Apr 20 11:53 2000 (edgrif): Had to change MapColDrawFunc prototypes.
 * * Jan 28 14:00 1999 (edgrif): Repair accidental damage to this
 *              module as a result of the development and patch copies
 *              becoming swapped.
 * Created: Thu Dec  9 00:01:50 1993 (mieg)
 * CVS info:   $Id: fmapcdna.c,v 1.23 2002-04-19 17:52:26 edgrif Exp $
 *-------------------------------------------------------------------
 */

/*
#define ARRAY_CHECK
#define MALLOC_CHECK
*/

#include <wh/acedb.h>
#include <wh/aceio.h>
#include <wh/bump.h>
#include <whooks/tags.h>
#include <whooks/systags.h>
#include <whooks/classes.h>
#include <wh/bs.h>
#include <wh/lex.h>
#include <wh/a.h>
#include <wh/bindex.h>
#include <wh/dna.h>
#include <wh/dnaalign.h>
#include <wh/query.h>
#include <wh/session.h>
#include <wh/cdna.h>
#include <wh/acembly.h>
#include <wh/graph.h>
#include <wh/gex.h>
#include <wh/dotter.h>
#include <wh/parse.h>
#include <wh/annot.h>
#include <wh/method.h>

#include <wh/display.h>
#include <w7/fmap_.h>

#ifndef ACEMBLY
#include <wh/cdnainit.h>
#endif /* !ACEMBLY */

#ifdef ACEMBLY
static ANNOTATION  notes[] = 
{
  /*   { 1, "Ready for submission",  "Ready_for_submission",  0, 'k', 0, {0}}, */
  { 1, "Fully edited",  "Fully_edited",  0, 'k', 0, {0}},
  { 1, "Remark", "Remark", 0, _Text, 1000, {0}},
  { 1, "Sequencing error in genomic DNA",  "Sequencing_error_in_genomic_dna",  0, _Text, 1000, {0}},
  /*
  { 4,    "No Gap", "No_Gap", 0, 'k', 0, {0}},
  { 4,    "Gap in single open exon", "Single_gap", 0, 'k', 0, {0}},
  { 4,    "Gap", "Gaps", 0, 'k', 0, {0}},
  { 4,    "Single clone", "Single_clone", 0, 'k', 0, {0}},
  */
  { 4,    "5-prime", 0, 0, 'k', 0, {0}},
  /*
  { 8,          "SL1",  "SL1", 0, 'k', 0, {0}},
  { 8,          "SL2",  "SL2", 0, 'k', 0, {0}},
  { 8,          "gccgtgctc",  "gccgtgctc", 0, 'k', 0, {0}},
  */
  { 8,          "Non standard 5p motif",  "5p_motif", 0, _Text, 1000, {0}},
  { 8,          "Clone with 5p inversion",  "5p_inversion", 0, 'k', 0, {0}},
  /*
  { 8,          "Many clones start here",  "Many_clones_start_here", 0, 'i', 0, {0}},
  { 8,          "5p alternatif",  "Alternative_5prime", 0, 'k', 0, {0}},
  */
  { 4,          "Alternative splicing",  "Alternative_splicing", 0, 'k', 0, {0}},
  /*
  { 12,             "Coupled to alt 3p",  "coupled_to_alt_3p", 0, 'i', 0, {0}},
  { 12,             "Alternative exon",  "Alternative_exon", 0, 'k', 0, {0}},
  { 12,             "Overlapping alternative exons",  "Overlapping_alternative_exons", 0, 'k', 0, {0}},
  { 12,             "3_6_9 splice variations",  "3_6_9_splice_variations", 0, 'k', 0, {0}},
  */
{ 8,          "Confirmed non gt_ag",  "Confirmed_non_gt_ag", 0, 'k', 0, {0}},
  { 4,    "3-prime", 0, 0, 'k', 0, {0}},
  /*   { 8,        "Poly A seen in 5p read",  "Poly_A_seen_in_5p_read", 0, 'k', 0, {0}}, */
  { 8,        "Alternative poly A",  "Alternative_poly_A", 0, 'k', 0, {0}},
  { 8,        "No Alternative poly A",  "No_Alternative_poly_A", 0, 'k', 0, {0}},
  { 8,        "Fake_internal poly A",  "Fake_internal_poly_A", 0, 'k', 0, {0}},
  /*
 { 1, "Translation", 0, 0, 'k', 0, {0}},
  { 4,    "Open from first to last exon",  "Open_from_first_to_last_exon", 0, 'k', 0, {0}},
  { 4,    "Nb introns after stop", "Nb_introns_after_stop", 0, _Int, 0, {0}},
  */
  { 1, "Problem",  0, 0, 'k', 0, {0}},
  { 4,    "Jack Pot Bug",  "Jack_Pot_Bug", 0, 'k', 0, {0}},
  { 4,    "Fuse Bug",  "Fuse_Bug", 0, 'k', 0, {0}},
  { 4,    "Other Program Bug",  "Other_Bug", 0, _Text, 1000, {0}},
  { 1, "Genefinder", 0, 0, 'k', 0, {0}},
  { 4,    "Compatible", "Compatible_prediction", 0, 'k', 0, {0}},
  { 4,    "Different", "Different", 0, 'k', 0, {0}},
  /*   { 4,    "Corresponds to one predicted gene", "Matching_genefinder_gene", 0, 'k', 0, {0}}, */
  { 4,    "No predicted gene", "No_prediction", 0, 'k', 0, {0}},
  /*
  { 4,    "cDNA covers several predicted genes", "cDNA_covers_several_predicted_genes", 0, 'k', 0, {0}},
  { 4,    "Predicted covers several genes", "Predicted_covers_several_genes", 0, 'k', 0, {0}},
*/
  { 0, 0, 0, 0, 0, 0, {0}}
} ;
#endif

static void fMapSplicedcDNADecorate (FeatureMap look, float x, SEG *seg, BOOL isUp, int x1, int x2) ;
static Array findSpliceErrors (KEY link, KEY key, Array dnaD, Array dnaR, int a1, int a2, 
				 int x1, int x2, BOOL upSequence) ;
static Array spliceCptErreur (Array dnaD, Array dnaR, Array dnaCourt, BOOL isUp,
			 int x1, int x2, int x3, 
			 int *clipTopp, int *cEndp, int *cExp) ;


static void fMapcDNAAction (KEY key, int box) ;
static KEY myTranslatedGene = 0 ;
static FREEOPT fMapcDNAMenu[] = {
#ifdef ACEMBLY
  {  6, "cDNA menu"},
  {'c', "Show cDNA clone"},
  {'t', "Show EST"},
  {'d', "Show EST Sequence"},
  {'s', "Show EST Trace"},
  {'w', "Change Strand"},
  {'e', "Discard from here"},
  /*   {'p', "Dot plot"} useless */
#else
  {  3, "cDNA menu"},
  {'c', "Show cDNA clone"},
  {'t', "Show EST"},
  {'d', "Show EST Sequence"}
#endif
} ;

static void fMapTranscriptAction (KEY key, int box) ;
static FREEOPT fMapTranscriptMenu[] = {
  {11, "T_Gene menu"},
  {'w', "Who am I"},
  {'t', "Translate"},
  {'s', "Recompute Splicing"},
  {'S', "Limit this gene to the active zone"},
  {'F', "Fuse genes present in the active zone to this one"},
  {'R', "Revert to original base call"},
  {'d', "Show cDNA is new window"},
  {'r', "Rename this gene"},
  {'k', "Eliminate this gene"},
  {'a', "Annotate this gene"},
  {'b', "New annotator"}
} ;

/***********************************************************************/

void fMapcDNAInit (FeatureMap look)
{
  static BOOL firstPass = TRUE ;
  
  if (!firstPass)   
    cDNAAlignInit () ;

  firstPass = FALSE ;
}

/***********************************************************************/
/***********************************************************************/
#ifndef ACEMBLY
/* this code is duplicated in wabi/cDNAalign.c */

typedef struct  { int dcp, dcq, lng, ok ; } JUMP ;

static JUMP jumper [] = {
 {1, 0, 5, 1},    /* insert in 1 */
 {0, 1, 5, 1},    /* trou in 1 */
 {1, 1, 3, 0},    /* ponctuel */
 {1, 0, 7, 2},    /* insert in 1 */
 {0, 1, 7, 2},    /* trou in 1 */
 {2, 0, 9, 2}, 
 {0, 2, 9, 2}, 
/*
 3, 0, 13, 2,
 0, 3, 13, 2,
 4, 0, 15, 3,
 0, 4, 15, 3,
 5, 0, 15, 3,
 0, 5, 15, 3,
 6, 0, 15, 3,
 0, 6, 15, 3,
 7, 0, 15, 3,
 0, 7, 15, 3,
 8, 0, 15, 3,
 0, 8, 15, 3,
 9, 0, 15, 3,
 0, 9, 15, 3,
 10, 0, 15, 3,
 0, 10, 15, 3,
*/
 {1, 1, 0, 0}    /* default is punctual */
} ;

/***********************************************************************/

#ifdef JUNK

/* in a pdded system, there should be no insert delete ? */
static JUMP paddedJumper [] = {
{ 1, 1, 0, 0 }   /* default is punctual */
} ;


static JUMP reAligner [] = {
 0, 0, 5, 1,    /* accept current location */
 1, 0, 5, 1,    /* insert in 1 */
 0, 1, 5, 1,    /* trou in 1 */
 1, 1, 5, 0,    /* ponctuel */
 1, 0, 7, 2,    /* insert in 1 */
 0, 1, 7, 2,    /* trou in 1 */
 2, 0, 9, 2, 
 0, 2, 9, 2, 
 3, 0, 13, 2,
 0, 3, 13, 2,
 4, 0, 15, 3,
 0, 4, 15, 3,
 5, 0, 15, 3,
 0, 5, 15, 3,
 6, 0, 15, 3,
 0, 6, 15, 3,
 7, 0, 15, 3,
 0, 7, 15, 3,
 8, 0, 15, 3,
 0, 8, 15, 3,
 9, 0, 15, 3,
 0, 9, 15, 3,
 10, 0, 15, 3,
 0, 10, 15, 3,
 0, 0, 0, 0    /* default is accept current location */
} ;
#endif /* JUNK */

/***********************************************************************/
static JUMP jumpN [] = {{1000,0,0,0}} ;  /* a kludge, to jump ambiguities */
  /* dna1 = short, dna2 = long */

Array spliceTrackErrors (Array  dna1, int pos1, int *pp1, 
			 Array dna2, int pos2, int *pp2, 
			 int *NNp, Array err, int maxJump, int maxError)
{ char *cp, *cq, *cp1, *cq1, *cp0, *cq0, *cpmax, *cqmax ;
  int i, n, sens = 1, nerr = 0, nAmbigue = 0 ;
  JUMP *jp ;
  A_ERR *ep ;
  Array errArray = arrayReCreate (err, 50, A_ERR) ;


  cp0 = arrp (dna1, 0, char) ;
  cp = cp0 + pos1 - 1 ; cpmax = cp0 + 
    ( *pp1 < arrayMax(dna1)  ? *pp1 : arrayMax(dna1)) ;
  cq0 = arrp (dna2, 0, char) ;
  cq = cq0 + pos2 - 1 ; cqmax =  cq0 + arrayMax(dna2) ;

  while (++cq, ++cp < cpmax && cq < cqmax) /* always increment both */
    {
      if (*cp == *cq)
	continue ;
      if  (!(*cp & 0x0F) || (*cp & 0x0F) == N_) 
	{ jp = jumpN ; goto foundjp ; }
      jp = jumper ; jp-- ;
      while (jp++) /* 1, 1, 0, 0 = last always accepted */
	{ 
	  
	  if (jp->dcp > maxJump ||
	      jp->dcq > maxJump)
	    continue ;
	  n = 0 ; i = jp->ok ;
	  cp1 = cp + jp->dcp ; cq1 = cq + jp->dcq ;
	  while (i--)
	    if ((*cp1++ & 0x0F) != (*cq1++ & 0x0F))
	      goto nextjp ;
	  n = 0 ; i = jp->lng ;
	  cp1 = cp + jp->dcp ; cq1 = cq + jp->dcq ;
	  while (i--)
	    if ((*cp1++ & 0x0F) != (*cq1++ & 0x0F))
	      if (++n > jp->ok)
		goto nextjp ;
	  goto foundjp ;
	nextjp:
	  continue ;	  
	}
    foundjp:
      ep = arrayp(errArray, nerr++, A_ERR) ;
      
      switch (jp->dcp - jp->dcq)
	{ 
	case 1000:
	  ep->type = AMBIGUE ;
	  ep->iShort = cp - cp0 ;
	  ep->iLong = cq - cq0 ;
	  ep->sens = sens ;
	  ep->baseShort = *cp ;
	  nAmbigue++ ;
	  break ;
	case 0:
	  ep->type = ERREUR ;
	  ep->iShort = cp - cp0 ;
	  ep->iLong = cq - cq0 ;
	  ep->sens = sens ;
	  ep->baseShort = *cp ;
	  break ;
	case 1:
	  ep->type = INSERTION ;
	  ep->iShort = cp - cp0 ;
	  ep->iLong = cq - cq0 ;
	  ep->sens = sens ;
	  ep->baseShort = *cp ;
	  cp += sens ;
	  break ;
	case -1:
	  ep->type = TROU ;
	  ep->iShort = cp - cp0 ;
	  ep->iLong = cq - cq0 ;
	  ep->sens = sens ;
	  ep->baseShort = '*' ;
	  cq++ ;
	  break ;
	default:
	  if ( jp->dcp > jp->dcq)
	    {
	      ep->type = INSERTION_DOUBLE ;
	      ep->iShort = cp - cp0 ;
	      ep->iLong = cq - cq0 ;
	      ep->sens = sens ;
	      ep->baseShort = *cp ;
	      cp += 2*sens ;
	    }
	  else
	    {
	      ep->type = TROU_DOUBLE ;
	      ep->iShort = cp - cp0 ;
	      ep->iLong = cq - cq0 ;
	      ep->sens = sens ;
	      ep->baseShort = '*' ;
	      cq += 2 ;
	    }
	}
      if (/*doStop && */
	  arrayMax(errArray) > 5 &&
	  ep->iShort < (ep - 5)->iShort + 10)
	break ;   
      if (maxError &&  arrayMax(errArray) >= maxError)
	break ;
    }
  *pp1 = cp - cp0 ;
  *pp2 = cq - cq0 ;
  if (NNp) *NNp = nAmbigue ;
  return errArray ;
}
#endif /* !ACEMBLY */

/***********************************************************************/
/***********************************************************************/

static BOOL isTooLong (SEG *seg)
{
  int a1, a2, b1, b2 ;

  a1 = ((seg - 1)->data.i >> 14) & 0x3FFF ;
  a2 = (seg - 1)->data.i & 0x3FFF ;
  b1 = (seg->data.i >> 14) & 0x3FFF ;
  b2 = seg->data.i & 0x3FFF ;
  
  return b1 - a2 > 1000 ? TRUE : FALSE ;
}

static BOOL isAmbiguous (SEG *seg)
{
  return seg->data.i &  0x20000000 ?  TRUE : FALSE ;
}

/***********************************************************************/
/***********************************************************************/

BOOL fMapcDNADoFillData(SEG *seg)
{
  KEY dummy, cDNA_clone = 0 ;
  int xx ; 
  OBJ obj2 = 0;

  cDNAAlignInit () ;
  if (seg->data.i & 0x80000000) /* bit 31 set */
    return TRUE ;

  seg->data.i |=  0x80000000 ; /* no recursion */

  if (bIndexTag(seg->key, _PolyA_after_base) ||
      bIndexTag(seg->key, _cDNA_clone))
    {
      obj2 = bsCreate (seg->key) ;
      if (obj2)  
	{ 
	  if (bsGetData (obj2, _PolyA_after_base, _Int, &xx) &&
	      xx == (seg->data.i & 0x3FFF))
	    seg->data.i |=  0x40000000 ;
	  if (bsGetKey (obj2, _cDNA_clone, &cDNA_clone))
	    seg->parent = cDNA_clone ; /* needed for bumping */
	  if (bsGetKey (obj2, _From_gene, &dummy) && 
	      bsGetKey (obj2, _bsDown, &dummy))  /* at lesast 2 genes */
	    seg->data.i |=  0x20000000 ;   
	  bsDestroy (obj2) ;
	}
    }
  return TRUE ;
}

/*********************************************************************/
/* economize function calls in the seg loop */

#define cDNAFillData(__seg) (((__seg)->data.i & 0x80000000) ? TRUE : fMapcDNADoFillData(__seg))

/*********************************************************************/

void fMapcDNAReportLine (char *buffer, SEG *seg1, int maxBuf)
{
  int x1, x2, y1, y2, ln = 0, polyA = 0 ;
  SEG *seg ;
  OBJ obj = 0 ;
  KEY dnaKey ;
 
  x1 = x2 = seg1->data.i >> 14 & 0x3FFF ; /* start equal, since order unknown */
  seg = seg1 ;
  while (seg1->type == seg->type && seg1->key == seg->key)
    {
      y1 = seg->data.i >> 14 & 0x3FFF ;
      y2 = seg->data.i & 0x3FFF ;
      if (x1 > y1) x1 = y1 ;
      if (x2 < y2) x2 = y2 ;
      if (x1 > y2) x1 = y2 ;
      if (x2 < y1) x2 = y1 ;
      seg++ ;
    }
  seg = seg1 - 1 ;
  while (seg1->type == seg->type && seg1->key == seg->key)
    {
      y1 = seg->data.i >> 14 & 0x3FFF ;
      y2 = seg->data.i & 0x3FFF ;
      if (x1 > y1) x1 = y1 ;
      if (x2 < y2) x2 = y2 ;
      if (x1 > y2) x1 = y2 ;
      if (x2 < y1) x2 = y1 ;
      seg-- ;
    }
  if ((obj = bsCreate(seg1->key)))
    {
      if (bsGetKey(obj,_DNA,&dnaKey))
	bsGetData (obj, _bsRight, _Int, &ln) ;
      bsGetData (obj, _PolyA_after_base, _Int, &polyA) ; 
      bsDestroy(obj) ;
    }

  strncat (buffer, messprintf ("# Length %d, Match from %d to %d", ln, x1, x2), maxBuf) ;
  if (polyA)
    strncat (buffer, messprintf ("# PolyA at %d", polyA, x2), maxBuf) ;
}

/*********************************************************************/

void fMapcDNAShowSplicedcDNA (LOOK genericLook, float *offset, MENU *unused)
     /* This is of type MapColDrawFunc */
{
  FeatureMap look = (FeatureMap)genericLook;
  int ii, box = 0, x1, x2 ;
  SEG *seg ;
  float x = 0, y1, y2, oldy2 = 0, oldWidth, xMax ;
  BOOL oldIsReadUp , isReadUp ;
  BOOL isDown ;
  KEY parent = 0, oldkey = 0 ;
  BUMP bump = 0 ; 
  float ytop, ybottom, yz ;
  SEG *seg1 ;
  int ix = 0 ;

  if (look->map->activeColName[0] == '-')
    /* on reverse strand, if first character of column name is '-'*/
    { 
      isDown = FALSE ;
      xMax = *offset + 1.7 * 6 ;   /* limit to 6 columns */

      methodCacheGetByName(look->mcache, 
			   look->map->activeColName+1,
			   /* no defaults */
			   "",
			   /* no aceDiff */
			   "");
    }
  else
    { 
      isDown = TRUE ;
      xMax = *offset + 1.7 * 60 ;

      methodCacheGetByName(look->mcache, 
			   look->map->activeColName,
			   /* no defaults */
			   "",
			   /* no aceDiff */
			   "");
    }

  for (ii = 1 ; ii < arrayMax(look->segs) ; ++ii)
    { seg = arrp(look->segs,ii,SEG) ;
      switch (seg->type)
	{
	case SPLICED_cDNA: if (isDown) break ; else continue ;
	case SPLICED_cDNA_UP: if (!isDown) break ; else continue ;
	default: continue ;
	}
      if (seg->x2 < look->min)
	{ if (box > 0) box = -1 ;
	  continue ;
	}
      if (seg->x1 > look->max)
	{
	  if (box > 0 && seg->type == (seg - 1)->type &&
	      oldy2 < mapGraphHeight - 1 &&
	      cDNAFillData(seg) && cDNAFillData(seg - 1) &&
	      seg->parent == (seg - 1)->parent) /* link to previous */
	    { 
	      if (seg->key == (seg - 1)->key)
		graphColor (MAGENTA) ;  /* YELLOW */
	      else
		graphColor (isTooLong(seg) ? RED : GREEN) ;  /* YELLOW */
	      graphLine (x +.8, oldy2, x + .8, mapGraphHeight - 1) ;
	      graphColor (BLACK) ;
	    }
	  if (box > 0)
	    box = -1 ;
	  continue ;
	}
      if (class(seg->key) == _VMethod)
	continue ;

      if (!bump)
	bump = bumpCreate (mapGraphWidth, 0) ;
      y1 = MAP2GRAPH(look->map,seg->x1) ;
      y2 = MAP2GRAPH(look->map,seg->x2+1) ;	/* to cover full base */
      if (y1 < 2 + topMargin) y1 = 2 + topMargin ;
      if (y2 > mapGraphHeight) y2 = mapGraphHeight ;
      x1 = seg->data.i >> 14 & 0x3FFF ;
      x2 = seg->data.i & 0x3FFF ;
      isReadUp = x1 > x2 ; if (!isDown) isReadUp = !isReadUp ;
      cDNAFillData(seg) ;
      if (seg->parent == parent)
	{
	  if (seg->key == oldkey)
	    {
	      float mid = (y1 + oldy2) / 2 ;
	      graphColor (MAGENTA) ; /* BLACK MAGENTA  ORANGE) ; */
	      graphLine (x +.8, oldy2, x + 1.5 , mid) ;
	      graphLine (x +1.5, mid, x + .8, y1) ;
	      graphColor (BLACK); 
	      graphLine (x + .1, y1, x + 1.5, y1) ;
	    }
	  else /* different component */
	   {
	     if (oldkey && oldy2 < y1)
	       {
		 graphColor (isTooLong(seg) ? RED : GREEN) ;  /* YELLOW */	
		 graphLine (x +.8, oldy2, x + .8, y1) ;
		 graphColor (BLACK); 
	       }  
	   }	  
	}
      else
	{

	  if ((seg->type | 0x1) == ((seg - 1)->type | 0x1) && 
	      cDNAFillData(seg - 1) &&
	      seg->parent == (seg - 1)->parent) /* was clipped by window top */
	    {
	      ytop = 2 + topMargin ;
	    }
          else
	    ytop = y1 ;

	  seg1 = seg ;
	  while ((seg1 + 1)->parent == seg->parent) seg1++ ;
	  ybottom = MAP2GRAPH(look->map,seg1->x2+1) ;
	  if (ybottom > mapGraphHeight) ybottom = mapGraphHeight ;  
	  yz = y1 ; ix = 0 ;
	  bumpItem (bump,1,(ybottom - ytop + 1)+0.2,&ix,&yz) ;
	  x = *offset + 1.7*ix ;

	  if (x < xMax &&
	      seg->type == (seg - 1)->type && 
	      cDNAFillData(seg) && cDNAFillData(seg - 1) &&
	      seg->parent == (seg - 1)->parent) /* was clipped by window top */
	    {
	      if (seg->key == (seg - 1)->key)
		graphColor (MAGENTA) ;  /* YELLOW */
	      else
		graphColor (isTooLong(seg) ? RED : GREEN) ;  /* YELLOW */
	      graphLine (x +.8, 2 + topMargin, x + .8, y1) ;
	      graphColor (BLACK) ;
	    }
	}

      oldy2 = y2 ;
      parent = seg->parent ;
      oldIsReadUp = isReadUp ; 
      if (x > xMax) continue ;
      box = graphBoxStart() ;
      if (oldkey != seg->key &&
	  isReadUp)
	{
	  if (seg->data.i & 0x40000000)
	    graphCircle (x + .9, y1, .6) ;
	  else
	    { 
	      graphLine (x + .1 , y1 + .6, x + .8, y1) ;
	      graphLine (x + 1.5, y1 + .6, x + .8, y1) ;
	    }
	}
      else
	graphLine (x + .1, y1, x + 1.5, y1) ;
      oldkey = seg->key ; 
      graphColor (BLACK) ;

      oldWidth = graphLinewidth (isAmbiguous(seg) ? .5 : .3) ;
      graphLine (x + .9, y1, x + .9, y2) ;
      graphLinewidth (oldWidth) ;
      if (seg->key != (seg+1)->key &&
	  !isReadUp) 
	{
	  if (seg->data.i & 0x40000000)
	    graphCircle (x + .9, y2, .6) ;
	  else
	    { 
	     graphLine (x + .1 , y2 - .6, x + .8, y2) ;
	     graphLine (x + 1.5, y2 - .6, x + .8, y2) ;
	    }
	}
      else
	graphLine (x + .1, y2, x + 1.5, y2) ;
      if (look->dna)
	fMapSplicedcDNADecorate (look, x + .8, seg, isReadUp, x1, x2) ;
      graphBoxEnd () ;
      graphBoxDraw (box, BLACK, TRANSPARENT) ;
      array(look->boxIndex, box, int) = ii ;
      graphBoxFreeMenu(box, fMapcDNAAction, fMapcDNAMenu) ;
    }
  
  *offset += 1.7*bumpMax (bump) ;
  if (*offset > xMax) *offset = xMax ;
  bumpDestroy (bump) ;

  return;
} /* fMapcDNAShowSplicedcDNA */

/*********************************************************************/

static void fMapSplicedcDNADecorate (FeatureMap look, float x, SEG *seg, BOOL isUp, int x1, int x2)
{
  A_ERR *ep ;
  Array a ;
  Array dna = look->dna, dnaR ;
  int oldColor, color = 0, i, a1 = seg->x1, a2 = seg->x2 ;
  float y, yy, y1, y2 ;

  if (isUp && !look->dnaR)
    { look->dnaR = arrayCopy (look->dna) ;
      reverseComplement (look->dnaR) ;
    } 
  dnaR = look->dnaR ;

  a = findSpliceErrors (look->seqKey, seg->key, dna, dnaR, a1, a2, x1, x2, isUp) ;
  if (!arrayExists(a) || !arrayMax(a) || a->size != sizeof(A_ERR))
    return ;

  i = arrayMax(a) ;
  ep = arrp(a, 0, A_ERR)  - 1 ;
  y1 = y2 = 0 ; oldColor = -1 ;
  while (ep++, i--)
    {
      if (ep->iLong > a2)
	break ;
      if (ep->iLong < a1)
	continue ;
      switch(ep->type)
	{
	case ERREUR: 
	  color = RED ; 
	  break ;
	case INSERTION: /* _AVANT: */
	case INSERTION_DOUBLE:  /*_AVANT:  */
	case TROU:
	case TROU_DOUBLE:
	  color = BLUE ; 
	  break ;
	case AMBIGUE: 
	  color = -1 ; /* DARKGREEN ; don t show the N */
	  break ;
	}
      y = ep->iLong - .5 ;
      yy = ep->iLong + .5 ;
	
      y = MAP2GRAPH(look->map, y) ;
      yy = MAP2GRAPH(look->map, yy) ;
      if (yy - y < .20) yy = y + .20 ;
      if (y > topMargin + 2 && y < mapGraphHeight )
	{ 
	  if (y < y2 - .1)
	    { y2 = yy ; /* coalesce the 2 boxes */
	      if (color != oldColor)
		oldColor = DARKRED ;
	    }
	  else
	    { if (oldColor != -1)
		{ graphColor(oldColor) ;
		  graphFillRectangle(x - .35, y1, x + .35, y2) ;
		  graphColor (BLACK) ;
		}
	      /* register */
	      y1 = y ; y2 = yy ; oldColor = color ;
	    }
	}
    }
  
  if (oldColor != -1)
    { graphColor(oldColor) ;
      graphFillRectangle(x - .35, y1, x + .35, y2) ;
      graphColor (BLACK) ;
    }
}

/***************************************************************************************/

static Array findSpliceErrors (KEY link, KEY key, Array dnaD, Array dnaR, int a1, int a2, 
				 int x1, int x2, BOOL upSequence)
{ Array a = 0, mydna = 0 ;
  OBJ obj ;
  int x3, b1, b2, b3, u ;
  KEY mydnakey = 0 ;

  if ((obj = bsCreate(key)))
    { if (bsGetKey (obj, _DNA, &mydnakey))
	{ 
	  if ((mydna = dnaGet(mydnakey)))
	    {
	      /* the window may have only part of the dna */
	      if (a1 < 0) 
		{
		  if (upSequence) 
		    { x2 -= a1 ; a1 -= a1 ; }
		  else
		    { x1 -= a1 ; a1 -= a1 ; }
		}
	      if ((u = a2 - arrayMax(dnaD)) > 0)
		{
		  if (upSequence) 
		    { x1 -= u ; a2 -= u ; }
		  else
		    { x2 -= u ; a2 -= u ; }
		}
	      if (upSequence) 
		{  
		  b1 = a2 ;
		  b2 = a1 ;
		}
	      else
		{ 
		  b1 = a1 ; b2 = a2 ;
		}
	      if (x1 > x2) 
		{ x3 = x1 ; x1 = x2 ; x2 = x3 ; } 
	      b3 = b2 ; x3 = x2 ; x1-- ;
	      if (a1 < a2 && x1 < x2)
		a = spliceCptErreur (dnaD, dnaR, mydna, upSequence, 
				     b1, b2, b3,
				     &x1, &x2, &x3) ;
	    }
	  arrayDestroy (mydna) ;
	}
      bsDestroy (obj) ;
    }
  return a ;
}

/***************************************************************************************/
/***************************************************************************************/
static void cleanErr (Array err, Array dnaLong, Array dnaShort, BOOL isUp)
{ int i, j, n = arrayMax(err), nn, start, stop ;
  A_ERR *eq, *ep, *epMax ;
  char *cl, *cs, cc ;
  int sens = isUp ? -1 : 1 ;
  ALIGN_ERROR_TYPE type ;

  if (n < 2) return ;
  if (isUp)
    { ep = arrp (err, 0, A_ERR) - 1 ;
      n-- ;
      while (ep++, n--)
	if ((ep->type == INSERTION) && ((ep+1)->type == ERREUR) &&
	    (ep->iShort == (ep+1)->iShort + 1))
	  { type = ep->type ; ep->type = (ep+1)->type ; (ep+1)->type = type ;
	    i = ep->iLong ; ep->iLong = (ep+1)->iLong ; (ep+1)->iLong = i + 1;
	  }
    }
  n = arrayMax (err) ;
  ep = arrp(err, 0, A_ERR) - 1 ;
  while (ep++, n--)
    { 
      switch (ep->type)
	{
	case AMBIGUE:
	  nn = arrayMax (err) - 1 ;
	  if (isUp)
	    { 
	      if (n < nn &&
		  (ep - 1)->type == INSERTION &&
		  (ep - 1)->iShort < arrayMax(dnaShort) &&
		  (ep - 1)->iLong >= 0 &&
		  (ep - 1)->iLong < arrayMax(dnaLong))
		{ cl = arrp(dnaLong, (ep - 1)->iLong, char) ; 
		  cs = arrp(dnaShort, (ep - 1)->iShort, char) ;
		  i = j = 0 ;
		  cc = *cl ; while (*cl++ == cc) i++ ;
		  if (sens == -1)
		    { cc = complementBase[(int)cc] ;
		      while (*cs-- == cc) j++ ;
		    }
		  else
		    while (*cs++ == cc) j++ ; 
		  if (i >= j && (ep - 1)->iShort == ep->iShort + j)
		    { 
		      eq = ep - 2 ;
		      i = n + 1 ;
		      while (eq++, i--) *eq = *(eq + 1) ;
		      arrayMax(err) -= 1 ;
		      ep-- ;
		      ep->type = INSERTION ;
		      ep->iLong++ ;
		    }
		}
	      if (n < nn &&
		  (ep + 1)->type == INSERTION &&
		  (ep + 1)->iShort < arrayMax(dnaShort) &&
		  (ep + 1)->iLong >= 0 &&
		  (ep + 1)->iLong < arrayMax(dnaLong))
		{ cl = arrp(dnaLong, (ep + 1)->iLong, char) ; 
		  cs = arrp(dnaShort, (ep + 1)->iShort, char) ;
		  i  = j = 0 ;
		  cc = *cl ; while (*cl++ == cc) i++ ;
		  if (sens == -1)
		    { cc = complementBase[(int)cc] ;
		      while (*cs-- == cc) j++ ;
		    }
		  else
		    while (*cs++ == cc) j++ ; 
		  if (i >= j && (ep + 1)->iShort == ep->iShort + j)
		    { 
		      eq = ep - 1 ;
		      i = n  ;
		      while (eq++, i--) *eq = *(eq + 1) ;
		      arrayMax(err) -= 1 ;
		      ep-- ;
		      ep->type = INSERTION ;
		      ep->iLong++ ;
		    }
		}
	    }
	  else
	    { if (n > 0 &&
		  (ep + 1)->type == INSERTION &&
		  ep->iShort < arrayMax(dnaShort) &&
		  ep->iLong >= 0 &&
		  ep->iLong < arrayMax(dnaLong))
/*		  (ep + 1)->type == INSERTION_DOUBLE) plus difficile */
		{ cl = arrp(dnaLong, ep->iLong, char) ; 
		  cs = arrp(dnaShort, ep->iShort, char) + sens ;
		  i = j = 0 ;
		  cc = *cl ; while (*cl++ == cc) i++ ;
		  if (sens == -1)
		    { cc = complementBase[(int)cc] ;
		      while (*cs-- == cc) j++ ;
		    }
		  else
		    while (*cs++ == cc) j++ ; 
		  if (i == j && (ep + 1)->iShort == ep->iShort + i * sens)
		    { ep->type = INSERTION ;
		      eq = ep ;
		      i = n - 1 ;
		      while (eq++, i--) *eq = *(eq + 1) ;
		      arrayMax(err) -= 1 ;
		      n-- ;
		    }
		}
	    }
	  break ;
	case INSERTION: case INSERTION_DOUBLE:
	  start = ep->iShort ;
	  epMax = arrp (err, arrayMax (err) - 1, A_ERR) + 1 ;
	  if (n >= 0 && !isUp && ep->iShort >= 0 &&
	      ep->iShort < arrayMax(dnaShort))
	    { cs = arrp(dnaShort, ep->iShort, char) ;
	      cc = *cs ; i = arrayMax(dnaShort) - ep->iShort ;
	      while (i-- && *(++cs) == cc)
		{ ep->iShort++ ;
		  ep->iLong++ ;
		}
	      if (ep->type == INSERTION_DOUBLE)
		ep->iLong-- ;
	    }
	  else if (n >= 0 && isUp && ep->iShort >= 0 &&
		   ep->iShort < arrayMax(dnaShort))
	    { cs = arrp(dnaShort, ep->iShort, char) ;
	      cc = *cs ; i = ep->iShort ;
	      while (i-- && *(--cs) == cc && ep->iShort > 0)
		{ ep->iShort-- ;
		  ep->iLong++ ;
		}
	    }
	  stop = ep->iShort ;
	  eq = ep ;
	  if (isUp)
	    while (++eq < epMax && eq->iShort < start && 
		   eq->iShort >= stop)
	      eq->iShort++ ;
	  else
	    while (++eq < epMax && eq->iShort > start && 
		   eq->iShort <= stop)
	      eq->iShort-- ;
	  if (ep->type == INSERTION_DOUBLE)
	    { nn = n + 1 ;
	      eq = arrayp (err, arrayMax (err), A_ERR) + 1 ;
	      while (eq--, nn--)
		*eq = *(eq - 1) ;
	      ep->type = INSERTION ;
	      ep->baseShort = W_ ;
	      if (!isUp)
		ep->iShort-- ;
	      ep++ ;
	      ep->type = INSERTION ;
	      if (isUp)
		ep->iShort++ ;
	      ep->baseShort = W_ ;
	      ep++ ; if (n) n-- ;
	    }
	  break ;
	case TROU: case TROU_DOUBLE:
	  if (n >= 0 && ep->iLong >= 0 && ep->iLong < arrayMax(dnaLong))
	    { cs = arrp(dnaLong, ep->iLong, char) ;
	      cc = *cs ; i = arrayMax(dnaLong) - ep->iLong ;
	      while (i-- && *(++cs) == cc)
		{ ep->iShort += sens ;
		  ep->iLong++ ;
		}
	      if (ep->type == TROU_DOUBLE && !isUp)
		ep->iShort -- ;
	    }
	  break ;
	case ERREUR:
	  break ;
	}
    }

}


/*********************************************************/

static void fuseErr (Array err1, Array err2)
{ int i, n1 = arrayMax(err1), n2 = arrayMax(err2) ;
  for (i = 0 ; i < n2 ; i++)
    array(err1, n1 + i, A_ERR) = arr(err2, i, A_ERR) ;
}

/*********************************************************/

static Array spliceCptErreur (Array dnaD, Array dnaR, Array dnaCourt, BOOL isUp,
			 int x1, int x2, int x3, 
			 int *clipTopp, int *cEndp, int *cExp)
{ int amax, ctop, c2, c3, i, nn ;
  Array err1 = 0, err2 = 0, dnaLong ;
  A_ERR *ep, *eq, ee ;

  amax = arrayMax (dnaD) ;

  if (isUp)
    { x1 = amax - x1 - 1 ;
      x2 = amax - x2 - 1 ;
      x3 = amax - x3 - 1 ;
      dnaLong = dnaR ;
    }
  else
    dnaLong = dnaD ;

  ctop = *clipTopp ;
  x2-- ;  c2 = *cEndp -1 ;
  err1 = /* makeErr2 (dnaLong, dnaCourt, &x1, &x2, &ctop, &c2) ;*/
         spliceTrackErrors (dnaCourt, ctop, &c2, dnaLong, x1, &x2, &nn, err1, 6, 0) ;
  *cEndp = c2 = c2 + 1 ; x2++ ;  /* may have moved a bit */      
  *clipTopp = ctop ;
  c3 = *cExp - 1 ;  
  if (x2 < x3 && c2 < c3)
    { err2 =  /* makeErr2 (dnaLong, dnaCourt, &x2, &x3, &c2, &c3) ; */
             spliceTrackErrors (dnaCourt, c2, &c3, dnaLong, x2, &x3, &nn, err2, 6, 0) ;
      x3++ ;
      *cExp = c3 + 1 ;
      fuseErr (err1, err2) ;  /* accumulates in err1 */
      arrayDestroy (err2) ;
    }
  if (isUp)
    { x1 = amax - x1 - 1 ;
      x2 = amax - x2 - 1 ;
      x3 = amax - x3 - 1 ;
      i = arrayMax(err1) ;
      if (i)
	{ ep = arrp (err1, 0, A_ERR) - 1 ;
	  while (ep++, i--)
	    { 
	      switch (ep->type)
		{
		case INSERTION:
		case INSERTION_DOUBLE:
		  ep->iLong-- ;
		  break ;
		case TROU:
		case TROU_DOUBLE:
		  ep->iShort-- ;
		  break ;
		case AMBIGUE:
		case ERREUR:
		  break ;
		}
	      ep->iLong = amax - ep->iLong - 1 ;
	    }
	  i = arrayMax(err1) ;
	  ep = arrp (err1, 0, A_ERR) ;
	  eq = arrp (err1, i - 1, A_ERR) ;
	  while (ep < eq)
	    { ee = *ep ; *ep++ = *eq ; *eq-- = ee ; }
	}
    }
  cleanErr(err1, dnaD, dnaCourt, isUp) ;
  return err1 ;
}

/*********************************************************************/
/*********************************************************************/

#ifdef ACEMBLY
static FREEOPT cDNAFreeMenu[] = 
{
  {4,"cDNA menu"},
  {1, "align est on all cosmids"},
  {2, "align est on active set"},
  {3, "align est on one cosmid"},
  {5, "export confirmed introns"},
} ;
#endif

/************************************************************/

void fMapcDNASelector(ACEIN fi, ACEOUT fo)
{
#ifdef ACEMBLY
  KEY k ; 

  if (!graphSelect (&k,cDNAFreeMenu))
    return ;

  fMapcDNADoSelect (fi, fo, k, 0, 0) ;
#endif

  return;
} /* fMapcDNASelector */

/************************************************************/

/************************************************************/

void fMapcDNAReportTranscript (char *buffer, SEG *seg1, int maxBuf)
{
  char *cp = strnew(buffer,0) ;
  strncpy (buffer, messprintf ("Gene %s, %s  %s", name(seg1->parent), 
			       buffer, seg1->data.k ? name(seg1->data.k) : ""), maxBuf) ;
  messfree (cp) ;

  return;
} /* fMapcDNAReportTranscript */

/************************************************************/

void fMapcDNAShowTranscript (LOOK genericLook, float *offset, MENU *unused)
     /* This is of type MapColDrawFunc */
{
  FeatureMap look = (FeatureMap)genericLook;
  int ii, box = 0 ;
  SEG *seg ;
  float x = 0, y1, y2 ;
  BOOL isDown ;
  BUMP bump = 0 ; 
  float yz ;
  int ix = 0, color = 0 ;

  cDNAAlignInit () ;
  if (look->map->activeColName[0] == '-')
    { 
      isDown = FALSE ;

      methodCacheGetByName(look->mcache, 
			   look->map->activeColName+1,
			   /* no defaults */
			   "",
			   /* no aceDiff */
			   "");
    }
  else
    {
      isDown = TRUE ;

      methodCacheGetByName(look->mcache, 
			   look->map->activeColName,
			   /* no defaults */
			   "",
			   /* no aceDiff */
			   "");
    }

  for (ii = 1 ; ii < arrayMax(look->segs) ; ++ii)
    { seg = arrp(look->segs,ii,SEG) ;
      switch (seg->type)
	{
	case TRANSCRIPT: if (isDown) break ; else continue ;
	case TRANSCRIPT_UP: if (!isDown) break ; else continue ;
	default: continue ;
	}

      if (class(seg->key) == _VMethod)
	continue ;

      if (seg->key == seg->parent) /* the overall transcript, do not draw */
	continue ;

      if (!bump)
	bump = bumpCreate (mapGraphWidth, 0) ;
      y1 = MAP2GRAPH(look->map,seg->x1) ;
      y2 = MAP2GRAPH(look->map,seg->x2+1) ;	/* to cover full base */
      if (y1 < 2 + topMargin) y1 = 2 + topMargin ;
      if (y2 < 2 + topMargin) y2 = 2 + topMargin ;
   

      if (y2 > mapGraphHeight)
	y2 = mapGraphHeight ;  

      yz = y1 ; ix = 0 ;
      bumpItem (bump,1,(y2 - y1) , &ix, &yz) ;
      x = *offset + 1.7*ix ;
      array(look->boxIndex, box = graphBoxStart (), int) = ii ;
      graphColor (MAGENTA) ; /* RED  MAGENTA) ; */
      if (seg->key == _Intron || seg->key == _Alternative_intron)
	{
	  float mid = (y1 + y2) / 2 ;
	  if (seg->data.k && seg->data.k != str2tag("gt_ag"))
	    graphColor (BLUE) ;
	  graphLine (x +.2, y2, x + 1.6 , mid) ;
	  graphLine (x +1.6, mid, x + .2, y1 +.1) ;
	  color = TRANSPARENT ;
	}
      else if (seg->key == _Exon || seg->key == _First_exon || seg->key == _Last_exon ||
	       seg->key == _Alternative_exon)
	{ 
	  graphRectangle (x +.2, y1, x + 1.4 , y2) ;
	  color = PALEMAGENTA ;
	}
      else if (seg->key == _Partial_exon || seg->key == _Alternative_partial_exon)
	{ 
	  graphRectangle (x +.2, y1, x + 1.4 , y2) ;
	  color = WHITE ;
	}
      else if (seg->key == _Gap)
	 { 
	   graphColor (BLACK) ;
	   graphLine (x +.8, y1, x + .8 , y2) ; 
	   color = WHITE ;
	 }
      else
	{
	  graphColor (RED) ;
	  graphRectangle (x +.1, y1, x + .4 , y2) ;
	}
      graphColor (BLACK); 

      graphBoxEnd () ;
      array(look->boxIndex, box, int) = ii ;
      graphBoxDraw (box, BLACK, color) ;
      graphBoxFreeMenu(box, fMapTranscriptAction, fMapTranscriptMenu) ;
    }
  
  *offset += 1.7*bumpMax (bump) ;
  bumpDestroy (bump) ;
}

/*********************************************************************/
/*********************************************************************/


Array fMapcDNAReferenceDna = 0,  fMapcDNAReferenceHits = 0 ;

#ifdef ACEMBLY

KEY  fMapcDNAReferenceGene = 0 ;
KEY  fMapcDNAReferenceEst = 0 ;
int fMapcDNAReferenceOrigin = 0 ;

static void annotStart (KEY gene)
{ 
  KEY key ;
  OBJ Gene = bsUpdate(gene) ;
  lexaddkey(name(gene), &key, _VAnnotation) ;
  
  bsAddKey (Gene, str2tag("Annotations"), key) ;
  bsSave (Gene) ;
  autoAnnotateGene (gene, key) ;
  annotate (key, notes) ;
}

/*********************************************************************/

BOOL fMapcDNAPickTrace (KEY gene, KEY est, int from)
{
  KEY key = 0 ; 
  SEG *seg2  ;
  Array aa = 0 ;
  int i, j, origin=0 ;
  char *cp, *cq ;
  Array hits = 0 ;
  FeatureMap look = currentFeatureMap("fMapcDNAPickTrace") ;

  
  for (i = 0 ; i < arrayMax(look->segs) ; ++i)
      {	
	seg2 = arrp(look->segs,i, SEG) ;  
	if (seg2->key != gene || seg2->key != seg2->parent)  /* I want the overall transcript */
	  continue ;
	if (seg2->type & 0x1) /* wrong strand */
	  continue ;  
	origin = seg2->x1 > 5000 ? 5000 : seg2->x1 ;
	j = seg2->x2 + origin + 5000 ; /* take gene plus 5 kb downstream */
	if (j > arrayMax(look->dna))
	  j = arrayMax(look->dna) ;
	j = j -  seg2->x1 + origin ;
	aa = arrayCreate (j, char) ;
	array(aa,j - 1,char) = 0 ; 
	arrayMax(aa)-- ; /* make room, add terminal 0 */
	cp = arrp(aa,0,char) ; cq = arrp(look->dna, seg2->x1 - origin,char) ;
	while(j--) *cp++ = *cq++ ;
	hits = cDNAGetReferenceHits (gene, origin) ;
	if (!hits)
	  { arrayDestroy(hits) ; arrayDestroy(aa) ;
	  continue ;
	  }
	fMapcDNAReferenceOrigin = origin ;
	fMapcDNAReferenceDna = aa ;
	fMapcDNAReferenceHits = hits ;
	fMapcDNAReferenceGene = gene ;
	fMapcDNAReferenceEst = est ;
	key = gene ;
	origin = seg2->x1 ;
      }
  
  if (key)
    {
      if (look->origin != origin)
	{ look->origin = origin ;
	fMapDraw (look, 0) ;
	}	    
      display (key, from, "DtMultiTrace") ; 
      /*      annotStart (gene) ; */
      return TRUE ;
    } 
  return FALSE ;
}
#endif

BOOL fMapcDNAFollow (int box)
{
  KEY key, gene, clone, est ;
  SEG *seg ;  
#ifdef ACEMBLY
  SEG *seg2 ;  
  int from, i, origin ;
  KEYSET geneSet = 0 ;
#endif  
  FeatureMap look = currentFeatureMap("fMapcDNAFollow") ;
  
  seg = BOXSEG(box) ;
  if (!seg)
    return FALSE ; 
  
  if (isDisplayBlocked())
    { display (seg->key, 0, 0) ;
      return TRUE ;
    }

  est = seg->key ;
  key = gene = 0 ; fMapcDNAReferenceDna = 0 ;

#ifdef ACEMBLY
  origin = look->origin  ;
  from = GRAPH2MAP (look->map, graphEventY + .2) ; /* + look->map->mag * .5) ; */
    
  geneSet = queryKey (est, "FOLLOW From_gene") ;
  if (keySetMax(geneSet))
    for (i = 0 ; !key && i < arrayMax(look->segs) ; ++i)
      {
	seg2 = arrp(look->segs,i, SEG) ;
	if (seg2->type == TRANSCRIPT && keySetFind (geneSet, seg2->key, 0) && 
	    seg2->x2 > seg2->x1 + 10 &&
	    from >= seg2->x1 && from < seg2->x2 &&
	    seg2->x1 >=0 && seg2->x2 <= arrayMax(look->dna) )
	  {
	    if (seg2->type & 0x1) /* wrong strand */
	      continue ; 
	    if (seg2->key != seg2->parent)  /* I want the overall transcript */
	      continue ; 
	    if (fMapcDNAPickTrace (seg2->key, est, from - seg2->x1))
	      { keySetDestroy (geneSet) ; return TRUE ; }
#ifdef JUNK
	    gene = seg2->key ;
	    origin = seg->x1 > 5000 ? 5000 : seg->x1 ;
	    j = seg2->x2 + origin + 5000 ; /* take gene plus 5 kb downstream */
	    if (j > arrayMax(look->dna))
	      j = arrayMax(look->dna) ;
	    j = j -  seg2->x1 + origin ;
	    aa = arrayCreate (j, char) ;
	    array(aa,j - 1,char) = 0 ; 
	    arrayMax(aa)-- ; /* make room, add terminal 0 */
	    cp = arrp(aa,0,char) ; cq = arrp(look->dna, seg2->x1 - origin,char) ;
	    while(j--) *cp++ = *cq++ ;
	    hits = cDNAGetReferenceHits (gene, origin) ;
	    if (!hits)
	      { arrayDestroy(hits) ; arrayDestroy(aa) ;
	      continue ;
	      }
	    fMapcDNAReferenceOrigin = origin ;
	    fMapcDNAReferenceDna = aa ;
	    fMapcDNAReferenceHits = hits ;
	    fMapcDNAReferenceGene = gene ;
	    fMapcDNAReferenceEst = est ;
	    key = gene ;
	    from = from - seg2->x1 ;
	    origin = seg2->x1 ;
	    break ;
#endif
	  }
      }
  keySetDestroy (geneSet) ;


#endif

  clone = KEY_UNDEFINED ;
  bIndexGetKey(est, _cDNA_clone, &clone) ;
  display (clone ? clone : est, 0, "TREE") ;
  return TRUE ;
}


/***************************************************************************************/
/***************************************************************************************/

static void fMapcDNAAction (KEY key, int box)
{ 
  SEG *seg ;
  KEY clone, dna ;
#ifdef ACEMBLY
  OBJ obj = 0 ;
  KEY gene=0;
  KEYSET genes ;
  int i ;
#endif /* ACEMBLY */
  FeatureMap look = currentFeatureMap("fMapcDNAAction") ;

  seg = BOXSEG(box) ;
  if (!seg)
    return ;
  displayPreserve() ;
  switch (key)
    {
    case 'c':
      clone = KEY_UNDEFINED ;
      bIndexGetKey (seg->key, _cDNA_clone, &clone) ;
      display (clone ? clone : seg->key, 0, "TREE") ;
      break ;
    case 'd':
      if (bIndexGetKey (seg->key, _DNA, &dna))
	display (dna, 0, 0) ;
      else
	display (seg->key, 0, "TREE") ;
      break ;
    case 't':
      display (seg->key, 0, "TREE") ;
      break ;
#ifdef ACEMBLY
    case 's':
      if (bIndexTag (seg->key, _SCF_File))
	display (seg->key, 50, "DtMultiTrace") ;
      else display (seg->key, 0, "TREE") ;
      break ;
    case 'w':
      if (!sessionGainWriteAccess())
	return ;
      if (!messQuery("Do you really believe that the 3' 5' labelling of this cDNA is wrong"))
	return ;
      if ((obj = bsUpdate(seg->key)))
	{
	  BOOL isF = FALSE ;


	  if (bsFindTag (obj, _Forward))
	    bsAddTag (obj, _Reverse) ;
	  else if (bsFindTag (obj, _Reverse))
	    { isF = TRUE ; bsAddTag (obj, _Forward) ; }
	  bsAddTag (obj, str2tag("Relabelled")) ;
	  bsSave (obj) ;
#ifdef JUNK
	  KEY otherRead = 0 ; 
	  /* this would automatically inverse the other read */

	  clone = KEY_UNDEFINED ;
	  bIndexGetKey(seg->key, _cDNA_clone, &clone) ;
	  if ((obj = bsCreate(clone)))
	    {
	      if (bsGetKey (obj, _Read, &otherRead) &&
		  seg->key == otherRead)
		bsGetKey (obj, _bsDown, &otherRead) ;
	      bsDestroy (obj) ;
	    }
	  if (seg->key != otherRead && otherRead &&
	      (obj = bsUpdate (otherRead)))
	    {
	      if (isF)
		bsAddTag (obj, _Reverse) ; 
	      else
		bsAddTag (obj, _Forward) ;
	      bsAddTag (obj, str2tag("Relabelled")) ;
	      bsSave (obj) ;
	    }
#endif /* JUNK */
	  if (bIndexGetKey (seg->key, _From_gene, &gene))
	    cDNARealignGene (gene, 0, 0) ;
	  look->pleaseRecompute = TRUE ;
	  fMapDraw(look,0) ;
	}
      break ;
      /*#ifdef ACEMBLY*//* already within the ACEMBLY-only block */
    case 'e':
      clone = seg->parent ;
      if (!clone || !sessionGainWriteAccess())
	return ;
      genes = queryKey (seg->key, "> From_gene") ;
      for (i = 0 ; i < keySetMax(genes) ; i++)
	{
	  SEG *s = arrp (look->segs, 0, SEG) - 1 ;
	  int j = arrayMax(look->segs) ;
	  gene = keySet (genes, i) ;
	  while (s++, j--)
	    if (s->key == gene) break ;
	  if (j >= 0)
	    break ; /* relevant gene was not found */
	  gene = 0 ;
	}
      if (gene)
	{
	  if (!messQuery("Do you really want to remove this cDNA clone from this gene"))
	    return ;
	  cDNARemoveCloneFromGene (clone,gene) ;
	  look->pleaseRecompute = TRUE ;
	  fMapDraw(look,0) ;
	}
      else
	messout ("removal failed, please club jean") ;
      break ;
      /*#endif*/
    case 'p':   /* dotter */
      {
	Array dnaGene, dnaEst = dnaGet (seg->key) ;
	static char *ss1, *ss2 ;
	KEY est = seg->key ;
	KEYSET geneSet = 0 ;
	int i, origin, j, from ;
	char *cp, *cq ;
	SEG *seg2 ;

	est = seg->key ;
	key = gene = 0 ; fMapcDNAReferenceDna = 0 ;
	
	origin = look->origin  ;
	from = (seg->x1 + seg->x2) / 2 ;
	
	geneSet = queryKey (est, "FOLLOW From_gene") ;
	if (keySetMax(geneSet))
	  for (i = 0 ; !key && i < arrayMax(look->segs) ; ++i)
	    {
	      seg2 = arrp(look->segs,i, SEG) ;
	      if (seg2->type == TRANSCRIPT && keySetFind (geneSet, seg2->key, 0) && 
		  seg2->x2 > seg2->x1 + 10 &&
		  from >= seg2->x1 && from < seg2->x2 &&
		  seg2->x1 >=0 && seg2->x2 <= arrayMax(look->dna) )
		{
		  if (seg2->type & 0x1) /* wrong strand */
		    continue ; 
		  gene = seg2->key ;
		  origin = seg->x1 > 5000 ? 5000 : seg->x1 ;
		  j = seg2->x2 + origin + 5000 ; /* take gene plus 5 kb downstream */
		  if (j > arrayMax(look->dna))
		    j = arrayMax(look->dna) ;
		  j = j -  seg2->x1 + origin ;
		  dnaGene = arrayCreate (j, char) ;
		  array(dnaGene,j - 1,char) = 0 ; 
		  arrayMax(dnaGene)-- ; /* make room, add terminal 0 */
		  cp = arrp(dnaGene,0,char) ; cq = arrp(look->dna, seg2->x1 - origin,char) ;
		  while(j--) *cp++ = *cq++ ;

		  from = from - seg2->x1 ;
		  origin = seg2->x1 ;

		  dnaEst = dnaGet (est) ;
		  if (!dnaEst || ! arrayMax(dnaEst))
		    goto abort ;
		  dnaDecodeArray (dnaEst) ;
		  dnaDecodeArray (dnaGene) ;
		  ss1 = messalloc (arrayMax(dnaEst) + 1) ;
		  memcpy (ss1, arrp(dnaEst, 0, char), arrayMax(dnaEst)) ;
		  ss2 = messalloc (arrayMax(dnaGene) + 1) ;
		  memcpy (ss2, arrp(dnaGene, 0, char), arrayMax(dnaGene)) ;
		  ss1[arrayMax(dnaEst)] = 0 ;
		  ss2[arrayMax(dnaGene)] = 0 ;
		  
		  dotter ('N', "     ", 
			  
			  name(gene), ss2, origin,  /* name, dna and offset */
			  name(seg->key), ss1, 0,
			  
			  
			  from, 0, /* central coord */
			  NULL, NULL, NULL, 0.0, /* irrelevant */
			  0, /* zoomfactor */
			  NULL, 0, /* MSP */
			  NULL, 0) ;
		  
		abort:
		  arrayDestroy (dnaEst) ;
		  arrayDestroy (dnaGene) ;
		  
		  break ;
		}
	    }
	keySetDestroy (geneSet) ;


      }
      break ;
#endif /* ACEMBLY */
    }
}

/***********************************************************************/

static void fMapTranscriptAction (KEY key, int box)
{ 
  KEY gene = 0 ;
  SEG *seg ;
#ifdef ACEMBLY
  OBJ Gene = 0 ;
#endif /* ACEMBLY */
  Graph old = graphActive () ;
  FeatureMap look = currentFeatureMap("fMapTranscriptAction") ;

  seg = BOXSEG(box) ;
  if (!seg)
    return ;
  gene = seg->parent ;
#ifdef ACEMBLY
lao:
#endif /* ACEMBLY */
  switch (key)
    {
    case 'w':
      display (gene, 0, "TREE") ;
      graphActivate (old) ;
      return ;
      break ;
    case 't':
      if (gene == myTranslatedGene)
	myTranslatedGene = 0 ;
      else
	myTranslatedGene = gene ;
      mapColSetByName (look->map, "3 Frame Translation", TRUE) ;
      break ;
    case 's':
#ifdef ACEMBLY
      if (gene)
	cDNARealignGene (gene, 0, 0) ;
#endif /* ACEMBLY */
      break ;
    case 'S':
#ifdef ACEMBLY  
      if (gene)
	{
	  KEY cosmid = KEY_UNDEFINED ;
	  int z1= 0, z2 = 0, i = arrayMax (look->segs) ;
	  SEG *seg1=0 ;

	  bIndexGetKey (gene, _Genomic_sequence, &cosmid) ;
	  while (i--)
	    {
	      seg1 = arrp (look->segs, i, SEG) ;
	      if (seg1->key == cosmid && (seg1->type | 0x1) == SEQUENCE_UP)
		{ z1 = seg1->x1 ; z2 = seg1->x2 ; break ; }
	    }
	  if (z1 >= z2) break ;
	  if (seg1->type & 0x1) /* up cosmid */
	    cDNARealignGene (gene, z2 - look->zoneMax, z2 - look->zoneMin) ;
	  else
	    cDNARealignGene (gene, look->zoneMin - z1, look->zoneMax - z1) ;
	}
#endif /* ACEMBLY */
      break ;
    case 'F':  /* fuse */
#ifdef ACEMBLY  
      if (!gene)
	break ;
      {
	KEYSET clones, genes, ks1, cosmids ; 
	KEY cosmid ;
	int z1= 0, z2 = 0, i = arrayMax (look->segs), j, jcl = 0, jg = 0, jcs = 0 ;
	SEG *seg1=0 ;
	OBJ Gene ;

	/* accumulate clones of other local genes */
	clones = keySetCreate () ; 
	genes = keySetCreate () ; 
	cosmids = keySetCreate () ; 
	while (i--)
	  {
	    seg1 = arrp (look->segs, i, SEG) ;
	    if (seg1->key == seg1->parent && seg1->type == TRANSCRIPT)
	      {
		z1 = seg1->x1 ; z2 = seg1->x2 ;  
		cosmid = KEY_UNDEFINED ;
	        bIndexGetKey (seg1->key, _Genomic_sequence, &cosmid) ;
		keySet (cosmids, jcs++) = cosmid ; 
		keySet (genes, jg++) = seg1->key  ;
	        if (z2 > look->zoneMin && z1 < look->zoneMax)
		  {
		    ks1 = queryKey (seg1->key, "FOLLOW cDNA_clone") ;
		    for (j = 0 ; j < keySetMax(ks1) ; j++)
		      keySet(clones, jcl++) = keySet (ks1, j) ;
		    if (keySetMax(ks1))
		      keySet (genes, jg++) = seg1->key ;
		    keySetDestroy (ks1) ;
		  }
	      }
	  }
	keySetSort (genes) ;
	keySetCompress (genes) ;
	if (keySetMax(genes))
	  { KEYSET aa = keySetAlphaHeap (genes, 1) ;
	    OBJ obj = 0 ;

	    gene = keySet(aa, 0) ;

	    aa = query (genes, "Follow annotations") ;
	    for (i = 0 ; i < keySetMax(aa) ; i++)
	      if ((obj = bsUpdate(keySet(aa, i))))
		bsKill (obj) ;
	  }
	for (j = 0 ; j < keySetMax(genes) ; j++)
	  if (gene != keySet(genes, j) &&  
	      (Gene = bsUpdate (keySet(genes, j))))
	    bsKill (Gene) ;
	keySetSort (clones) ;
	keySetCompress (clones) ;
	/* destroy other genes */
	Gene = bsUpdate (gene) ;
	if (Gene) for (j = 0 ; j < keySetMax(clones) ; j++)
	  bsAddKey (Gene, _cDNA_clone, keySet(clones, j)) ;
	bsSave (Gene) ;
	keySetDestroy (clones) ;
	keySetDestroy (genes) ; 


	keySetSort (cosmids) ;
	keySetCompress (cosmids) ;
	cosmid = keySet(cosmids, 0) ; 
	keySetDestroy (cosmids) ;

	/* get coord of cosmid */

	i = arrayMax (look->segs) ;
	while (i--)
	  {
	    seg1 = arrp (look->segs, i, SEG) ;
	    if (seg1->key == cosmid && (seg1->type | 0x1) == SEQUENCE_UP)
	      { z1 = seg1->x1 ; z2 = seg1->x2 ; break ; }
	  }
	if (z1 >= z2) break ;
	if (seg1->type & 0x1) /* up cosmid */
	  cDNARealignGene (gene, z2 - look->zoneMax, z2 - look->zoneMin) ;
	else
	  cDNARealignGene (gene, look->zoneMin - z1, look->zoneMax - z1) ;
      }

#endif /* ACEMBLY */
      break ;
    case 'r': /* rename */
      {
	ACEIN name_in;

	if ((name_in = messPrompt("New name for this gene", name(gene),"w", 0)))
	  {
	    char *cp;
	    if ((cp = aceInWord(name_in)))
	      lexAlias (gene, cp, TRUE, FALSE) ;
	    
	    aceInDestroy (name_in);
	  }
      }
      break ;

    case 'd':  /* show sequence in new window */
      {
	KEY dnaGene = KEY_UNDEFINED ;

	gene = seg->parent ;
	bIndexGetKey (gene, str2tag("From_gene"), &dnaGene) ;
#ifdef ACEMBLY
	if (!dnaGene)
	  dnaGene = cDNAMakeDna (gene) ;
#endif /* ACEMBLY */
	if (!dnaGene)
	  {
	    messout ("Sorry, i cannot yet deal with alternative splicing") ;
	    return ;
	  }

	displayPreserve () ;
	display(dnaGene, 0, 0)  ;
	mapColSetByName (look->map, "Gene Translation", TRUE) ;
	mapColSetByName (look->map, "DNA", TRUE) ;
	return ;
      }
      break ;
#ifdef ACEMBLY
    case 'k': /* kill */

      if (messQuery("Do you really want to eliminate this gene and all reads inside it ?"))
	{
	  KEYSET gks = queryKey (gene, "> cDNA_clone") ;
	  int i ;
	  
	  for (i = 0 ; i < keySetMax(gks) ; i++)
	    cDNARemoveCloneFromGene (keySet(gks,i),gene) ;

	  Gene = bsUpdate (gene) ; 
	  if (Gene) bsKill(Gene) ;
	}
      break ;

    case 'R': /* revert to abi base call */
      if (messQuery 
   ("You will lose all the editions of this gene\nDo you wish to proceed ?"))
      {
	KEYSET ks = queryKey (gene, "{FOLLOW BaseCall} $| {FOLLOW SCF_Position}") ;
	int i = keySetMax(ks) ;
	while (i--)
	  arrayKill (keySet(ks,i)) ;
	key = 's' ;
	goto lao ;
      }
      else
	return ;
      break ;
    case 'a': /* annotate */
      annotStart (gene) ;
      return ; /* no fmapredraw needed */
    case 'b': /* new annotator */
      geneAnnotDisplay (0, gene, 0) ;
      return ; /* no fmapredraw needed */
#endif /* ACEMBLY */
    } 
  look->pleaseRecompute = TRUE ;
  graphActivate (old) ;
  fMapDraw(look,0) ;
}

/*******************************************************************/
/************************* eof *************************************/
