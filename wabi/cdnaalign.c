/*  File: cdnaalign.c
 *  Author: Danielle et Jean Thierry-Mieg (mieg@mrc-lmba.cam.ac.uk)
 *  Copyright (C) J Thierry-Mieg. 1997
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
 * This file is part of the ACEMBLY extension to 
        the ACEDB genome database package, written by
 * 	Richard Durbin (MRC LMB, UK) rd@mrc-lmba.cam.ac.uk, and
 *	Jean Thierry-Mieg (CRBM du CNRS, France) mieg@kaa.cnrs-mop.fr
 *
 * Description:
   Align cDNA
 * Exported functions:
 * HISTORY:
 * Last edited: Jul  4 11:39 2003 (edgrif)
 * Created: Thu Dec  9 00:01:50 1997 (mieg)
 *-------------------------------------------------------------------
 */

/* %W% %G% */
/*
#define ARRAY_CHECK
#define MALLOC_CHECK
*/

#include "acedb.h"
#include "aceio.h"
#include "lex.h"
#include "bs.h"
#include "a.h"
#include "dna.h"
#include "whooks/systags.h"
#include "whooks/tags.h"
#include "whooks/classes.h"
#include "dna.h"
#include "dnaalign.h"
#include "bump.h"
#include "query.h"
#include "session.h"
#include "bindex.h"
#include "cdna.h"
#include "acembly.h"
#include "cdnainit.h"
#include "parse.h"
#include "bitset.h"

BITSET_MAKE_BITFIELD		/* define bitField for bitset.h ops */

static int OLIGO_LENGTH = 15 ;
int acemblyMode = SHOTGUN ;

static void showHits(Array hits) ;
static void showSet (KEYSET set) ;

/********************************************************************/
/********************************************************************/
/* this code is duplicated in wabi/fmapcDNA.c */
typedef struct  { int dcp, dcq, lng, ok ; } JUMP ;


static JUMP jumper [] = {
 {1, 0, 5, 1},    /* insert in 1 */
 {0, 1, 5, 1},    /* trou in 1 */
 {1, 1, 3, 0},    /* ponctuel */
 {1, 0, 7, 2},    /* insert in 1 */
 {0, 1, 7, 2},    /* trou in 1 */
 {2, 0, 9, 2}, 
 {0, 2, 9, 2}, 

 {3, 0, 13, 2},
 {0, 3, 13, 2},
 {4, 0, 15, 3},
 {0, 4, 15, 3},
 {5, 0, 15, 3},
 {0, 5, 15, 3},
 {6, 0, 15, 3},
 {0, 6, 15, 3},
 {7, 0, 15, 3},
 {0, 7, 15, 3},
 {8, 0, 15, 3},
 {0, 8, 15, 3},
 {9, 0, 15, 3},
 {0, 9, 15, 3},
 {10, 0, 15, 3},
 {0, 10, 15, 3},
 {1, 1, 0, 0}    /* default is punctual */
} ;

static JUMP jumpN [] = {{1000,0,0,0}} ;  /* a kludge, to jump ambiguities */
  /* dna1 = short, dna2 = long */

Array spliceTrackErrors (Array  dna1, int pos1, int *pp1, 
		   Array dna2, int pos2, int *pp2, int *NNp, Array err, int maxJump)
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
    }
  *pp1 = cp - cp0 ;
  *pp2 = cq - cq0 ;
  if (NNp) *NNp = nAmbigue ;
  return errArray ;
}

/*********************************************************************/



/*********************************************************************/
/*********************************************************************/

/* this func most often gets wrong polyA, 
   disabled
static int locatePolyA (Array dna)
{ 
  char *cp ; int i, j = 0 ;

  return 0 ;
  i = dna ? arrayMax(dna)  : 0 ;
  if (i < 20) return 0 ;
  cp = arrp(dna, i-1, char) ;
  while (i-- && (*cp-- & A_)) j++ ;
  return j > 6 ? i + 1 : 0 ;
}
*/
/*********************************************************************/

/* allSpl, allDna :: one per poossible product
   spl:  KEY int int int int 
     tag (intron/gap/exon..)
     offset of this exon in genemic dna
     begin+end of this exon in (product) dna
     number of this exon in original
     */

typedef struct { KEY tag ; int branch, subBranch, offset, 
                a1, a2, x1, x2, exonNb, iGap ; 
                BOOL isExon, isLastExon ; } SPL ;
/* a1 a2 are in genomic, x1 x2 in mrna in construction */

static int cDNAdna (KEY gene, Array all, Array allSpl, STORE_HANDLE handle)
{
  Array dna = 0, dnaParent = 0, aa = 0,  spl = 0 ;
  KEY tt, parent ;
  OBJ obj = 0 ;
  int a1, a2, x1, x2, i, iaa, j, k, jAll = 0, jspl, igap, nnew, exonNb ;
  BSunit *u ;
  SPL *ss, *ss1 ;
  int nBranch = 0, subBranch = 0, newBranch ;
  KEYSET oks = 0, fils = 0 ;
  BOOL ok, diff, isExon ;

  if (!bIndexGetKey(gene, _Genomic_sequence, &parent) || !(dnaParent = dnaGet (parent)))
    return 0 ;

  obj = bsCreate (parent) ;
  if (!obj || !bsFindKey (obj, _Transcribed_gene, gene) ||
      !bsGetData (obj,_bsRight, _Int, &a1) ||
      !bsGetData (obj,_bsRight, _Int, &a2))
    goto abort ;
  bsDestroy (obj) ;

  if (a1 > a2) 
    {
      reverseComplement (dnaParent) ;
      a1 = arrayMax(dnaParent) - a1 + 1 ;
      a2 = arrayMax(dnaParent) - a2 + 1 ;
    }
  a1-- ; /* plato */
  if (a1 < 0 || a2 < 10 || a2 > arrayMax(dnaParent))
    goto abort ;
  
  obj = bsCreate (gene) ; 
  aa = arrayCreate (64, BSunit) ;
  if (!obj || !bsGetArray (obj, _Splicing, aa, 4))
    goto abort ;
  bsDestroy (obj) ;


  oks = keySetCreate () ;

  jAll = 0 ;

  for (iaa = 0 ; iaa < arrayMax(aa) && jAll < 24 ; iaa += 4)
    {
      u = arrp (aa, iaa, BSunit) ;
      x1 = u[0].i ;
      x2 = u[1].i ;
      tt = u[2].k ;
      exonNb = u[3].k ;
      

      /* switch does not work here because _Exon etc are not constants */
      if (tt == _Exon || tt == _First_exon || tt == _Last_exon ||
	  tt == _Partial_exon || tt == _Alternative_first_exon ||
	  tt ==  _Alternative_exon || tt == _Alternative_partial_exon)
	isExon = TRUE ;
      else
	isExon = FALSE ;

      if (tt == _Exon || tt == _First_exon || tt == _Last_exon ||
	  tt == _Partial_exon || tt == _Alternative_first_exon ||
	  tt == _Alternative_intron || tt == _Intron ||
	  tt ==  _Alternative_exon || tt == _Alternative_partial_exon)
	{ 
	  oks = keySetReCreate(oks) ;
	  nnew = 0 ;  ok = FALSE ; fils = keySetReCreate (fils) ;
	  for (jspl = 0 ; jspl < jAll ; jspl++)
	    {
	      spl = array (allSpl, jspl, Array) ;
	      diff = TRUE ;

	      if (tt != _Alternative_first_exon &&
		  tt != _First_exon)
		{
		  for (i = 1 ; i < arrayMax(spl) ; i++)
		    {
		      ss = arrayp (spl, i, SPL) ;
		      if (ss->a2 == x1 - 1 && ss->isExon != isExon && !ss->isLastExon)
			{ keySet (oks, jspl) = i + 1 ; ok = TRUE ; }  /* hooking at i+1 in jspl is good */
		    }
		}
	      if (!keySet (oks, jspl) ||
		  keySet (oks, jspl) == arrayMax(spl))  /* only in case of conflict, do create a new gene up to the conflict */
		continue ;
		
	      /* verify we are not  creating a double  */
	      diff = TRUE ;
	      for (j = jAll ; j < jAll + nnew ; j++)
		{
		  Array spl1 = array (allSpl, j, Array) ;
		  diff = FALSE ;  /* all new have for sure the current new exon as last one */
		  for (i = 0 ; i < keySet (oks, jspl) && i < arrayMax(spl1) ; i++)
		    {
		      ss = arrayp (spl, i, SPL) ;
		      ss1 = arrayp (spl1, i, SPL) ;
		      if (ss->exonNb != ss1->exonNb || ss->iGap != ss1->iGap)
			{ diff = TRUE ; break ; }
		    }
		  if (!diff) break ;
		}
	      if (!diff)  /* already treated */
		{ keySet (oks, jspl) = 0 ; continue ; }
	      
	      dna = array (all, jspl, Array) ;
	      array (all, jAll + nnew, Array) = arrayCopy (dna) ;
	      dna =  array (all, jAll + nnew, Array) ;
	      
	      spl = array (allSpl, jspl, Array) ;
	      array (allSpl, jAll + nnew, Array) = arrayCopy (spl) ;
	      spl =  array (allSpl, jAll + nnew, Array) ;
	      arrayMax(spl) =  keySet (oks, jspl) ;
	      ss = arrayp (spl, 0, SPL) ;
	      newBranch = keySet(fils, ss->branch) ;
	      if (!newBranch)
		keySet(fils, ss->branch) = newBranch = nBranch++ ;
	      for (i = 0, ss = 0 ; i < arrayMax(spl) ; i++)
		{
		  ss = arrayp (spl, i, SPL) ;
		  ss->branch = newBranch ;
		}
	      arrayMax(dna) = ss ? ss->x2 : 0 ;
	      keySet (oks, jspl) = 0 ;
	      keySet (oks, jAll + nnew) = arrayMax(spl) ;
	      /*	      printf("x1 = %d x2 = %d isExon = %d , creating new branch %d ",   x1, x2, isExon, ss->branch) ;*/
	     nnew++ ;
	    }

	  if (!ok) /* matches nowhere, create a new gene starting here */
	    { 
	      dna = arrayHandleCreate (a2 - a1 , char, handle) ; j = 0 ;
	      array (all, jAll + nnew, Array) = dna ;
	      
	      spl = arrayHandleCreate (64, SPL,handle) ;
	      ss = arrayp(spl, 0, SPL) ;   /* make room */
	      ss->branch = nBranch++ ;
	      array (allSpl, jAll + nnew , Array) = spl ;
	      keySet (oks, jAll + nnew) = 1 ;
	      if (tt == _First_exon)
		ss->tag  = 1 ; /* begin found */ 
	      nnew++ ;
	      /* printf("x1 = %d x2 = %d isExon = %d , creating new branch %d ",   x1, x2, isExon, ss->branch) ; */
	    }

	  for (jspl = 0 ; jspl < nnew + jAll ; jspl++)
	    {
	      if (!keySet(oks, jspl))
		continue ;
	      spl = array (allSpl, jspl, Array) ;
	      dna = array (all, jspl, Array) ;
	      j = arrayMax (dna) ;

	      ss = arrayp (spl, keySet(oks, jspl), SPL) ;
	      ss->tag = tt ;
	      ss->isExon = isExon ;
	      ss->isLastExon = (tt == _Last_exon) ;
	      ss->exonNb = exonNb ;
	      ss->offset = x1 - 1 - j ; /* plato */
	      ss->branch = (ss-1)->branch ;
	      ss->a1 = x1 ;
	      ss->a2 = x2 ;
	      ss->x1 = j ;
	      if (jspl > 0 && (ss-1)->iGap)  
		ss->x1 -= (ss-1)->iGap ; /* eat up a partial triplet to stay in frame */
	      
	      dna = array (all, jspl, Array) ;
	      if (isExon)
		{   /* not the introns */
		  j = arrayMax (dna) ;
		  for (k = a1 + x1 - 1; k < a1 + x2 && k < a2 ; k++)
		    array (dna, j++, char) = array (dnaParent, k, char) ; 
		}
	      ss->x2 = arrayMax (dna) ;
	    }
	  jAll += nnew ;
	}   
      else if (tt == _Gap)
	{
	  nnew = 0 ;
	  for (jspl = 0 ; jspl < jAll ; jspl++)
	    {
	      spl = array (allSpl, jspl, Array) ;
	      ss = arrp(spl, arrayMax(spl) - 1, SPL) ;
	      if (ss->a2 != x1 - 1 || ss->isLastExon)
		continue ;
	      array (allSpl, jAll + nnew, Array) = arrayCopy (spl) ;
	      array (allSpl, jAll + nnew + 1, Array) = arrayCopy (spl) ;
	      array (allSpl, jAll + nnew + 2, Array) = arrayCopy (spl) ;
	      
	      dna = array (all, jspl, Array) ;
	      array (all, jAll + nnew, Array) = dnaCopy (dna) ;
	      array (all, jAll + nnew + 1, Array) = dnaCopy (dna) ;
	      array (all, jAll + nnew + 2, Array) = dnaCopy (dna) ;
	    
	      for (igap = 0 ; igap < 3 ; igap++)
		{
		  spl = array (allSpl, jAll + nnew + igap, Array) ;
		  dna = array (all, jAll + nnew + igap, Array) ;
		  j = arrayMax (dna) ;
		  ss = arrayp (spl, arrayMax(spl), SPL) ;
		  ss->tag = tt ;
		  ss->exonNb = iaa/3 ;
		  ss->offset = x1 - 1 - j ; /* plato */
		  ss->branch = (ss-1)->branch ;
		  ss->a1 = x1 ;
		  ss->a2 = x2 ;
		  ss->x1 = j ;
		  j = arrayMax (dna) ;
		  k = (x2 - x1 + 1)/2 + igap ; /* guess half exon half intron */
		  while (k--) 
		    array (dna, j++, char) = N_ ; 
		  ss->x2 = arrayMax (dna) ; 
		  ss->iGap = 3 + ((ss->x2 - ss->x1) % 3) ;
		  for (j = 0 ; j < arrayMax(spl) ; j++)
		    {
		      ss = arrayp (spl, j, SPL) ;
		      ss->subBranch = subBranch + 1 + igap ;
		    }
		}
	      
	      /* in original jspl  use genomic dna */
	      spl = array (allSpl, jspl, Array) ;
	      dna = array (all, jspl, Array) ;
	      j = arrayMax (dna) ;
	      ss = arrayp (spl, arrayMax(spl), SPL) ;
	      ss->tag = tt ;
	      ss->exonNb = iaa/3 ;
	      ss->offset = x1 - 1 - j ; /* plato */
	      ss->branch = (ss-1)->branch ;
	      ss->a1 = x1 ;
	      ss->a2 = x2 ;
	      ss->x1 = j ;

	      for (k = a1 + x1 - 1; k < a1 + x2 && k < a2 ; k++)
		array (dna, j++, char) = array (dnaParent, k, char) ; 
	      ss->x2 = arrayMax (dna) ;	  
	      ss->iGap = 3 + (ss->x2 - ss->x1) % 3 ; 
	      for (j = 0 ; j < arrayMax(spl) ; j++)
		 {
		   ss = arrayp (spl, j, SPL) ;
		   ss->subBranch = subBranch + 4 ;
		 }
	      nnew += 3 ;
	      subBranch += 5 ;
	    }
	  jAll += nnew ;
	}

      if (0)    /* debug */
	{
	  printf("\n") ;
	  for (jspl = 0 ; jspl < jAll ; jspl++)
	    {
	      spl = array (allSpl, jspl, Array) ;
	      for (i = 0 ; i < arrayMax(spl) ; i++)
		{
		  ss = arrayp (spl, i, SPL) ;
		  if (i) printf(" %d", ss->exonNb) ;
		  else printf("#%d : exons = ", ss->branch) ;
		} 
	      printf("\n") ;
	    }
	}
		  
     }
  /* insure a terminal zero */
  for (i = 0 ; i < jAll ; i++)
    {
      dna = array (all,  i, Array) ;
      j = arrayMax (dna) ;
      array(dna, j, char) = 0 ; arrayMax(dna) = j ; 
      dna = 0 ;  /* do not destroy ! */
    }

abort:
  keySetDestroy (fils) ;
  bsDestroy (obj) ;
  arrayDestroy (dnaParent) ;
  arrayDestroy (aa) ;
  keySetDestroy (oks) ;
  return nBranch ;
}

/*********************************************************************/

static void cdnaExportCloneLength (KEY product, KEY gene, Array spl, ACEOUT dump_out)
{
  KEYSET clones = queryKey (product, ">Assembled_from_cDNA_clone"), reads = 0 ;
  Array aa = arrayCreate (500, BSunit) ;
  OBJ Clone = 0, Gene = bsCreate (gene) ;
  KEY clone, est ;
  BSunit *up ;
  int iaa, iclone, iread, isp, a1, a2, x1, x2, b0, b1, b2 ;
  float xo = 0 ;
  KEY psize = str2tag("PCR_product_size") ;
  SPL* ss ;

  if (Gene && bsGetArray (Gene, _Assembled_from, aa, 3))
    for (iclone = 0 ; iclone < keySetMax(clones) ; iclone++)
      {
	/* locate the extremities of the cdna_clone using gene->Assembled_from info */
	clone = keySet (clones, iclone) ;
	reads = queryKey (clone, ">Read") ;
	a1 = a2 = -1 ;
	for (iread = 0 ; iread < keySetMax (reads) ; iread++)
	  for (est = keySet (reads, iread), iaa = 0 ; iaa < arrayMax(aa) ; iaa += 3)
	    {
	      up = arrayp(aa, iaa, BSunit) ;
	      if (est == up[2].k)
		{
		  if (a1 == -1 || a1 >  up[0].i - 1) 
		    a1 = up[0].i - 1 ;
		  if (a2 == -1 || a2 <  up[1].i) 
		    a2 = up[1].i ;
		}
	    }
	keySetDestroy (reads) ;
	if (a1 == -1)
	  continue ;
	/* now releate a1 a2 to x1 x2 counted inside the product dna using spl */
	x1 = a1 ; x2 = -1 ;
	for (isp = 1 ; isp < arrayMax(spl) ; isp++)
	  {
	    ss = arrp (spl, isp, SPL) ;
	    if (ss->tag == _Alternative_intron || ss->tag == _Intron)
	      continue ;
	    b0 = ss->offset ; /* cumulated intron length so far */
	    b1 = ss->x1 ;
	    b2 = ss->x2 ;
	    if (a1 >= b0 + b1) x1 = a1 - b0 ;
	    if (a2 >= b0 + b1) x2 = a2 - b0 ;  /* use nearest exon start */	    
	  }
	if (x2 == -1) 
	  continue ;
	/* find calculated and observed lengths */
	xo = -2 ;  /* not -1, mind float roundings ! */
	if ((Clone = bsCreate (clone)))
	  {
	    bsGetData (Clone, psize, _Float, &xo) ;
	    bsDestroy (Clone) ;
	  }
	/* ace out the result */
	aceOutPrint (dump_out, "clone_lengths %s calculated %d bp", name (clone), x2 - x1) ;
	if (xo > -1)
	  aceOutPrint (dump_out, " observed  %g kb", xo) ;
	aceOutPrint (dump_out, "\n") ;
      }
  arrayDestroy (aa) ;
  keySetDestroy (clones) ;
  bsDestroy (Gene) ;

  return ;
} /* cdnaExportCloneLength */


/*********************************************************************/

KEY cDNAMakeOneDna (KEY gene, int branch, int maxBranch, Array all, Array allSpl)
{
  Array dna = 0, spl = 0 ;
  int ii, k, besti1, besti2, i1, i2, jaa, bestaa = 0, bestX = 0, nX, subBranch, maxSubBranch ;
  Stack s = 0 ;
  ACEOUT dump_out;
  KEY key = 0 ;
  char *cp ;
  SPL *ss ;
  int coding1, coding2 ;
  BOOL isTranspliced = FALSE ;
  Array tt = NULL ;

  if (!(tt = pepGetTranslationTable(gene, NULL)))
    return KEY_UNDEFINED ;


  for (maxSubBranch = 0, jaa = 0 ; jaa < arrayMax(all) ; jaa++)
    {
      spl = array (allSpl, jaa, Array) ;
      ss = arrayp (spl, 0, SPL) ;
      if (ss->branch == branch && ss->subBranch > maxSubBranch)
	maxSubBranch = ss->subBranch ;
    }
      
  /* find the limits of the longest ORF in any subBranch */
  bestaa = besti1 = besti2 = nX = 0 ; 
  for (subBranch = 0 ; subBranch <= maxSubBranch ; subBranch++)
    for (jaa = 0 ; jaa < arrayMax(all) ; jaa++)
      {
	dna = array (all, jaa, Array) ;
	spl = array (allSpl, jaa, Array) ;

	ss = arrayp (spl, 0, SPL) ;
	if (ss->branch != branch)
	  continue ;
	if (ss->subBranch != subBranch)
	  continue ;

	/* find the limits of the longest ORF in any frame */
	for (k = 0, i2 = 0, nX = 0 ; k < 3 ; k++, i2 = k)
	  for (ii = k,  cp = arrp (dna, ii, char); ii <= arrayMax(dna) - 3 ; cp += 3 , ii += 3)
	    {	
	      if (ii >= arrayMax(dna) - 5 || e_codon(cp, tt) == '*')
		{ 
		  if (e_codon(cp, tt) == 'X')
		    nX++ ;
		  i1 = i2 ; i2 = ii ;
		  if (e_codon(cp, tt) == '*')
		    i2 += 3 ;  /* include the stop */
		  if (i2 - i1 > besti2 - besti1)
		    {
		      besti1 = i1 ;
		      besti2 = i2 ;
		      bestaa = jaa ;
		      bestX = nX ;
		    }
		  else if (i2 - i1 == besti2 - besti1 && bestX > nX)
		    {
		      besti1 = i1 ;
		      besti2 = i2 ;
		      bestaa = jaa ;
		      bestX = nX ;
		    }
		}
	    }
      }
  
  dna = array (all, bestaa, Array) ;
  spl = array (allSpl, bestaa, Array) ;
  
  /* now find the first atg */
  if (array(spl, 0, SPL).tag == 1)
     isTranspliced = TRUE ;
  if (besti1 > 2 || array(spl, 0, SPL).tag == 1)
    for (ii = besti1,  cp = arrp (dna, ii, char); ii <= besti2 - 2 ; cp += 3 , ii += 3)
      if (*cp == A_ && *(cp + 1) == T_ && *(cp+2) == G_)
	{ besti1 = ii ; break ; }

  dnaDecodeArray (dna) ;

  s = stackCreate (500) ;
  dump_out = aceOutCreateToStack (s, 0) ;

  if (maxBranch)
    lexaddkey(messprintf("%s.%d",name(gene),branch+1),  &key, _VSequence) ;
  else
    lexaddkey(messprintf("%s",name(gene)),  &key, _VSequence) ;

  aceOutPrint (dump_out, "Sequence %s\n-D CDS\nMessenger_RNA\nDerived_from_gene %s using_exons ",  /* -D to be backward compatible */
	       name(key), name(gene)) ;
  for (k = 1 ; k < arrayMax (spl) ; k++)
    {  
      ss = arrp(spl, k, SPL) ;
      if (ss->branch != branch) continue ;
      if (!ss->isExon)
	continue ;
      i1 = ss->x1 ;
      i2 = ss->x2 ;
      ii = ss->exonNb ;
      if (i1 < besti1) i1 = besti1 ;
      if (i2 > besti2) i2 = besti2 ;
      if (i1 < i2)
	aceOutPrint (dump_out, "%d ",  ii) ;
    } 
  aceOutPrint (dump_out, "\n") ;
  aceOutPrint (dump_out, "Subsequence G%s %d %d\n-D Clone_lengths\n",  
	       name(key), besti1 + 1, besti2) ;
  cdnaExportCloneLength (key, gene, spl, dump_out) ;

  aceOutPrint (dump_out, "\n\n") ;

  aceOutPrint (dump_out, "Sequence G%s \nCDS\nMethod Coding_region_constructed_from_cDNAs\n-D Source_Exons\n",  name(key)) ;
  for (k = 1 ; k < arrayMax (spl) ; k ++)
    {  
      ss = arrp(spl, k, SPL) ;
      if (ss->branch != branch) continue ;
      if (!ss->isExon)
	continue ;
      i1 = ss->x1 ;
      i2 = ss->x2 ;
      if (i1 < besti1) i1 = besti1 ;
      if (i2 > besti2) i2 = besti2 ;
      if (i1 < i2)
	aceOutPrint (dump_out, "Source_Exons %d %d\n",  i1 - besti1 + 1, i2 - besti1) ;
    }
  aceOutPrint (dump_out, "\n\n") ;

  /* report coding sequence extremities inside the gene */
  coding1 = coding2 = 0 ;

  aceOutPrint (dump_out, "Transcribed_gene %s\nCoding_sequence ", name(gene)) ;
  for (k = 1 ; k < arrayMax (spl) ; k ++)
    {  
      ss = arrp(spl, k, SPL) ;
      if (ss->branch != branch) continue ;
      if (!ss->isExon)
	continue ;
      i1 = ss->x1 ;
      i2 = ss->x2 ;
      ii = ss->offset ;
      if (besti1 >= i1 && besti1 < i2)
	{ aceOutPrint (dump_out, "%d ", coding1 = besti1 + ii + 1) ; break ; }
    }
  for (k = 1 ; k < arrayMax (spl) ; k ++)
    {  
      ss = arrp(spl, k, SPL) ;
      if (ss->branch != branch) continue ;
      if (!ss->isExon)
	continue ;
      i1 = ss->x1 ;
      i2 = ss->x2 ;
      ii = ss->offset ;
      if (besti2 > i1 && besti2 <= i2)
	{ aceOutPrint (dump_out, "%d \n", coding2 = besti2 + ii ) ; break ; }
    }
  aceOutPrint (dump_out, "\n\n") ;

  aceOutPrint (dump_out, "Transcribed_gene %s\n", name(gene)) ;
  for (k = 1 ; k < arrayMax (spl) ; k++)
    {  
      ss = arrp(spl, k, SPL) ;
      if (ss->branch != branch) continue ;
      if (ss->isExon)
	{
	  i1 = ss->a1 + 1 ; i2 = ss->a2 ;
	  if (i2 < coding1)
	    {
	      aceOutPrint (dump_out, "Derived_sequence %s %d %d 5prime_UTR\n",
			   name(key), i1 - 1 , i2) ;
	      continue ;
	    }
	  
	  if (i1 > coding2)
	    {
	      aceOutPrint (dump_out, "Derived_sequence %s %d %d 3prime_UTR\n",
			   name(key), i1 - 1 , i2) ;
	      continue ;
	    }
	  
	  if (i1 < coding1) i1 = coding1 + 1;
	  if (coding2 && i2 > coding2) i2 = coding2 ;
	  if (ss->a1 < i1 - 1)
	    {
	      /* if (isTranspliced || i1 >= 5) */
	      aceOutPrint (dump_out, "Derived_sequence %s %d %d 5prime_UTR\n",
			   name(key), ss->a1, i1 - 2) ;
	    }
	  aceOutPrint (dump_out, "Derived_sequence %s %d %d Exon %d\n",
		       name(key), i1 - 1, i2, ss->exonNb) ;
	  if (ss->a2 > i2 + 1)
	    aceOutPrint (dump_out, "Derived_sequence %s %d %d 3prime_UTR\n",
			 name(key), i2 + 1, ss->a2) ;
	  continue ;
	}
      else if (ss->iGap)
	aceOutPrint (dump_out, "Derived_sequence %s %d %d Gap\n",
		     name(key), ss->a1, ss->a2) ;
      else 
	aceOutPrint (dump_out, "Derived_sequence %s %d %d Intron\n",
		     name(key), ss->a1, ss->a2) ;
    } 
  aceOutPrint (dump_out, "\n\n") ;
      
  aceOutPrint (dump_out, "DNA %s\n", name(key)) ;
  aceOutPrint (dump_out, "%s", arrp(dna,0,char)) ;  /* export whole cdna */
  aceOutPrint (dump_out, "\n\n") ;
  aceOutDestroy (dump_out);

  parseBuffer(stackText(s,0), 0, FALSE) ;
  stackDestroy (s) ;

  return key ;
} /* cDNAMakeOneDna */

/*********************************************************************/

static KEYSET cDNAMakeDnaSet (KEY gene)
{
  Array all , allSpl ;
  KEYSET ks = 0 ;
  STORE_HANDLE handle = handleCreate () ;
  int i, ii, maxBranch ;

  all = arrayHandleCreate (20, Array, handle) ;
  allSpl = arrayHandleCreate (20, Array, handle) ;
  ks = keySetCreate() ;

  maxBranch = cDNAdna (gene, all, allSpl, handle) ;
  
  for (i = 0, ii = 0 ; i < maxBranch ; i++)
    keySet(ks, ii++) = cDNAMakeOneDna (gene, i, maxBranch, all, allSpl) ;
 
  for (i = 0 ; i < arrayMax(all) ; i++)
    arrayDestroy (arr(all, i, Array)) ;
  for (i = 0 ; i < arrayMax(allSpl) ; i++)
    arrayDestroy (arr(allSpl, i, Array)) ;
  handleDestroy (handle) ;
  return ks ;
}

KEY cDNAMakeDna (KEY gene)
{
  OBJ obj = bsUpdate(gene) ;
  KEYSET ks = 0 ;
  KEY key ;

  if (obj)
    {
      if (bsFindTag (obj, str2tag("Derived_sequence")))
	bsRemove (obj) ;
      bsSave (obj) ;
    }
  ks = queryKey (gene, "{>Derived_sequence} $| {>Derived_sequence ; >Subsequence} $| {>Derived_sequence ; >DNA}") ;
  keySetKill (ks) ;
  keySetDestroy (ks) ;
  ks = cDNAMakeDnaSet (gene) ;
  key = ks ? keySet (ks,0) : 0 ;
  keySetDestroy (ks) ;

  return key ;
}

/*********************************************************************/
/*********************************************************************/
/*   realignement des cDNA sur le genomique */


/*********************************************************************/

static Array getSeqDna(KEY key)
{
  static Associator ass = 0 ;
  void *vp, *vq ;
  Array a = 0 ;
  
  if (key == KEYMAKE (_VCalcul, 12345))
    {
      if (!ass) return 0 ;
      vp = vq = 0 ;
      while (assNext(ass, &vp, &vq))
	{
	  a = (Array) vq ;
	  if (a == assVoid(1))
	    continue ;
	  if (arrayExists(a))
	    arrayDestroy (a) ;
	  else 
	    messcrash ("getSeqDna(%s) misused", name(assInt(vp))) ;
	}
      assDestroy (ass) ;
      return 0 ;
    }
  if (!ass) ass = assCreate() ;
  if (assFind (ass, assVoid(key),&vq))
    {
      if (vq == assVoid(1))
	return 0 ;
      a = (Array) vq ;
      if (!arrayExists (a))
	messcrash ("getSeqDna(%s) misused", name(key)) ;
      return a ;
    }
  a = dnaGet (key) ;
  vq = a ? a : assVoid(1) ;
  assInsert (ass, assVoid(key), vq) ;
  return a;
}

/**********************************************************************/

static int assSequence (KEY key, BOOL isUp, int nn /* OLIGO_LEMGTH */,
			Associator ass, int step, int minPos,
			KEYSET cDNA_clones, KEYSET clipTops, KEYSET clipEnds)
{
  int ii, pos, pos1, max, maxpos, j, n1 = 0, eshift = 0, vtop, vend ;
  unsigned int oligo, anchor, smask, mask = (1<<(2 * nn)) -1 , keyMask = (1<<22) -1 ;
  Array aa = 0, dna =  dnaGet(key) ; /* NOT to be stored in local dna cache ! */
  KEY cDNA_clone = 0 ;
  char *cp ;
  OBJ obj = 0 ;
  KEY topFlag = 0, endFlag = 0 ;
  if (nn > 15) messcrash ("Oligo_length > 15 in assSequence") ;
  
  if (!dna)
    return 0 ;

  vtop = 1 ;
  vend = max = arrayMax(dna) ;
  
  ii = 0 ;
  if ((obj = bsCreate(key)) &&  !isUp &&
      bsGetData (obj, _PolyA_after_base, _Int, &ii)) ;
  /* else   ii = locatePolyA (dna) ; */
  bsDestroy (obj) ;

  if (ii && (obj = bsUpdate (key)))
    {
      bsAddData (obj, _PolyA_after_base, _Int, &ii) ;
      bsSave (obj) ;
      if (vend > ii) { vend = ii ; endFlag = KEYMAKE (1,0) ; }
    }
  obj = bsCreate (key) ;
  if (obj &&
      bsGetData (obj, str2tag("Vector_clipping"), _Int, &vtop) &&
      bsGetData (obj, _bsRight, _Int, &vend))
    {
      if (ii && vtop > ii) vtop = ii ;
      if (vtop > max) vtop = max ;
      if (vtop < 1) vtop = 1 ;
      keySet (clipTops, KEYKEY(key)) = vtop ;
      if (ii && vend > ii) vend = ii ;
      if (vend < 20 || vend > max) vend = max ;
      keySet (clipTops, KEYKEY(key)) = vend ;
    }

  aa = arrayCreate (6, BSunit) ;
  if (obj && bsGetArray (obj, _Transpliced_to, aa, 2))
    {
      for (ii = 0, j = 0 ; j < arrayMax(aa) ; j += 2)
	if (arrp(aa, j + 1, BSunit)->i > ii)
	  ii = arrp(aa, j + 1, BSunit)->i ;
      if (ii)
	{
	  if (!isUp && vtop < ii + 1) 
	    { vtop = ii + 1 ; topFlag = 2 ; }
	  if (isUp && vend > ii)
	    { vend = ii - 1 ; endFlag = 2 ; }
	}
    }
  arrayDestroy (aa) ;
	
  bsDestroy (obj) ;
  
  if (minPos < vtop) minPos = vtop ;
  if (minPos + nn + 10 > vend)
    goto abort ;
  
  topFlag = endFlag = 0 ; /* don t activate this yet, 
			   *  if yes, check all usage of clipTops/Ends 
			   */
  if (clipTops) keySet (clipTops, KEYKEY(key)) = KEYMAKE (topFlag, vtop) ;
  if (clipEnds) keySet (clipEnds, KEYKEY(key)) = KEYMAKE (endFlag, vend) ;

  if (cDNA_clones) 
    {
      OBJ Seq = bsCreate (key) ;
      bsGetKey (Seq, _cDNA_clone, &cDNA_clone) ;
      bsDestroy (Seq) ;
      keySet (cDNA_clones, KEYKEY(key)) = cDNA_clone ;
    }


  /* eshift, smask is used to anchor seqs longer than 1024
   * only 10 bits can be spared for the anchoring
   * for longer seqs, i position modulo smask
   */ 
  smask = vend ;
  smask >>= 10 ; eshift = 0 ;
  while (smask) { smask >>= 1 ; eshift++ ; }
  smask = (1<<eshift) - 1 ;
  if (step < (1<<eshift)) step = 1 << eshift ;
  
  maxpos = vend - nn ;
  if (isUp) 
    { 
      reverseComplement (dna) ;
      pos = minPos ;
      minPos = max - maxpos ;
      maxpos = max - pos ;
    }

  if (step < (maxpos - minPos) / 20)
    step = (maxpos - minPos) / 20 ;
  pos = minPos ;
  ii = -10000000 ;
  while (pos < maxpos)
    {
      j = nn ; oligo = 0 ;
      pos1 = isUp ? max - pos : pos  ;
      pos1 = ((pos1 + smask) >> eshift) << eshift ; /* +smask prevents forever */
      if (pos1 == ii) { pos++ ; continue ; }
      ii = pos1 ;
      pos = isUp ? max - pos1 : pos1  ;
      cp = arrp(dna, pos, char) ;
      while (cp++, j--)
	{ 
	  if (!*cp || *cp == N_)
	    goto suite ;	    
	  oligo <<= 2 ; oligo |= B2[(int)(*cp)] ; oligo &= mask ; 
	}
      cp -= nn ; /* reset to pos */

      anchor = (((pos1 >> eshift) & 1023) << 22) | (key & keyMask) ;
      if (oligo  /* not the poly A */
	  /* && !assFind (ass, assVoid(oligo), 0)   */ )
	{ 
	  assMultipleInsert (ass, assVoid(oligo), assVoid(anchor)) ; n1++ ; 
	  /* printf("%s %d \n", name(key), pos1) ;  */
	  pos += step ;  continue ;
	}
      /* else
	  printf("NO: %s %d \n", name(key), pos) ; 
	  */
    suite:
      pos++ ;
    }
  arrayDestroy (dna) ;
  /* printf("%s %d\n", name(key), n1) ; */
  return n1 ;
  
abort:
  arrayDestroy (dna) ;
  return 0 ;
}


/*********************************************************************/
/*********************************************************************/
/* we know there is an exact hit, try to extend it
   a1, a2 are cosmid coord, x1, x2, cDNA coord 
   */

static BOOL extendHits2 (KEY cosmid, Array dna, Array dnaR,
                        KEY est, Array dnaEst,
                        int *a1p, int *a2p, int *x1p, int *x2p,
			 int v1, int v2)
{
  BOOL isUp = FALSE ;
  int a1, a2, x1, x2, u1, u2, nn ;
  static Array dnaLong, aa = 0 ;

  x1 = *x1p ; x2 = *x2p ; a1 = *a1p ; a2 = *a2p ;
  if (a1 > a2) isUp = TRUE ;

  nn = arrayMax (dna) ;
  if (isUp)
    { dnaLong = dnaR ; u1 = nn - a1 + 1 ; u2 = nn - a2 + 1; }
  else
    { dnaLong = dna ; u1 = a1 ; u2 = a2 ; }
  x2 = v2 ;
  if (x1 < v1) x1 = v1 ;
  x1-- ; u1-- ; /* plato */
  aa = spliceTrackErrors (dnaEst, x1, &x2,
		    dnaLong, u1, &u2, 0, aa, 2) ;
  x1++ ; u1++ ; /* plato */
  if (isUp) a2 = nn - u2 + 1 ; 
  else  a2 = u2 ;
  
  *x1p = x1 ; *x2p = x2 ; *a1p = a1 ; *a2p = a2 ;
  return TRUE ;
}

/*********************************************************************/

static BOOL extendHits (KEY cosmid, Array dna, Array dnaR, HIT *vp)
{
  BOOL estUp = FALSE, ok, debug = FALSE ;
  int a1, a2, a3, x1, x2, x3, v1, v2, v3, nn, b2, y2 ;
  Array dnaEst = 0 ;
  
  /* placer les coords dans l'ordre naturel */
  a1 = vp->a1 ; a2 = vp->a2 ; x1 = vp->x1 ; x2 = vp->x2 ;
  if (x1 > x2) 
    { 
      estUp = TRUE ;
      x3 = x1 ; x1 = x2 ; x2 = x3 ; 
      a3 = a1 ; a1 = a2 ; a2 = a3 ;
    }
  else
    estUp = FALSE ;

  dnaEst = getSeqDna (vp->est) ;
  if (!dnaEst) return FALSE ;
  nn = arrayMax(dnaEst) ;
  if (debug) printf("extend 1:: a1 = %d a2 = %d x1 = %d x2 = %d\n",
		    a1, a2, x1, x2) ;

  v1 = vp->clipTop ; v2 = vp->clipEnd ;
  extendHits2(cosmid, dna,dnaR,vp->est,dnaEst,&a1,&a2,&x1,&x2,v1,v2) ;
  if (debug) printf("extend 2:: a1 = %d a2 = %d x1 = %d x2 = %d\n",
		    a1, a2, x1, x2) ;

  ok = x1 < 2 ? TRUE : FALSE ;
  /* retablir */
  if (estUp) 
    {  x3 = x1 ; x1 = x2 ; x2 = x3 ; 
       a3 = a1 ; a1 = a2 ; a2 = a3 ; 
    }
  b2 = a2 ; y2 = x2 ;
  if (ok)
    goto done ;

  /* extend backwards, starting from the exact oligo  */
  a1 = vp->a1 ; a2 = vp->a2 ; x1 = vp->x1 ; x2 = vp->x2 ;

    /* inverser */
  reverseComplement (dnaEst) ;
  x3 = x1 ; x1 = nn - x2 + 1 ; x2 = nn - x3 + 1 ;
  a3 = a1 ; a1 = a2 ; a2 = a3 ;
  v3 = v1 ; v1 = nn - v2 + 1 ; v2 = nn - v3 + 1 ;

  /* placer les coords dans l'ordre naturel */
  if (x1 > x2) 
    { estUp = TRUE ; x3 = x1 ; x1 = x2 ; x2 = x3 ; 
    a3 = a1 ; a1 = a2 ; a2 = a3 ; }
  else
    estUp = FALSE ;

  if (debug) printf("extend 3:: a1 = %d a2 = %d x1 = %d x2 = %d\n",
		    a1, a2, x1, x2) ;
  extendHits2(cosmid, dna,dnaR,vp->est,dnaEst,&a1,&a2,&x1,&x2,v1, v2) ;
  if (debug) printf("extend 4:: a1 = %d a2 = %d x1 = %d x2 = %d\n",
		    a1, a2, x1, x2) ;
  
  /* retablir */
  if (estUp) 
    {  x3 = x1 ; x1 = x2 ; x2 = x3 ; 
       a3 = a1 ; a1 = a2 ; a2 = a3 ;  
    }
  reverseComplement (dnaEst) ;
  x3 = x1 ; x1 = nn - x2 + 1 ; x2 = nn - x3 + 1 ;
  a3 = a1 ; a1 = a2 ; a2 = a3 ;
 done:
  if (debug) printf("extend 5:: a1 = %d a2 = %d x1 = %d x2 = %d\n",
		    a1, a2, x1, x2) ;
  vp->a1 = a1 ; vp->a2 = b2 ; vp->x1 = x1 ; vp->x2 = y2 ;

  return TRUE ;
}

/*********************************************************************/
/*********************************************************************/

static int cDNAOrderEstByA1 (void *va, void *vb)
{
  HIT *up = (HIT *)va, *vp = (HIT *)vb ;

  if (up->gene != vp->gene)
    return up->gene - vp->gene ;
  if (up->a1 != vp->a1)
    return up->a1 - vp->a1 ;
  else if (up->a2 != vp->a2)
    return up->a2 - vp->a2 ;      
  else  
    return up->x1 - vp->x1 ;
}

/*********************************************************************/

static int cDNAOrderGloballyByA1 (void *va, void *vb)
{
  HIT *up = (HIT *)va, *vp = (HIT *)vb ;

  if (up->a1 != vp->a1)
    return up->a1 - vp->a1 ;
  else if (up->a2 != vp->a2)
    return up->a2 - vp->a2 ;      
  else if (up->reverse != vp->reverse)  /* order by strand */
    return up->reverse ? 1 : -1 ;
  else 
    return up->est - vp->est ;
}

/*********************************************************************/

static int cDNAOrderGenesByA1 (void *va, void *vb)
{
  HIT *up = (HIT *)va, *vp = (HIT *)vb ;

  if (up->reverse != vp->reverse)  /* order by strand */
    return up->reverse ? 1 : -1 ;

  if (!up->reverse)
    {
      if (up->a1 != vp->a1)
	return up->a1 - vp->a1 ;
      else if (up->a2 != vp->a2)
	return up->a2 - vp->a2 ;      
      else  
	return up->type - vp->type ;    
    }
  else
    {
      if (up->a2 != vp->a2)
	return up->a2 - vp->a2 ;      
      else if (up->a1 != vp->a1)
	return up->a1 - vp->a1 ;
      else  
	return up->type - vp->type ;    
    }
}

/*********************************************************************/

static int cDNAOrderByA1 (void *va, void *vb)
{
  HIT *up = (HIT *)va, *vp = (HIT *)vb ;

  if (up->cDNA_clone != vp->cDNA_clone)
    return up->cDNA_clone - vp->cDNA_clone ;  /* the cDNA_clone */
  else if (up->est != vp->est)
    return up->est - vp->est ;  /* the cDNA_clone */
  else if (up->a1 != vp->a1)
    return up->a1 - vp->a1 ;
  else if (up->a2 != vp->a2)
    return up->a2 - vp->a2 ;      
  else  
    return up->x1 - vp->x1 ;    
}

/*********************************************************************/

static int cDNAOrderByX1 (void *va, void *vb)
{
  HIT *up = (HIT *)va, *vp = (HIT *)vb ;

  if (up->cDNA_clone != vp->cDNA_clone)
    return up->cDNA_clone - vp->cDNA_clone ;  /* the cDNA_clone */
  else if (up->est != vp->est)
    return up->est - vp->est ;  /* the cDNA_clone */
  else if (up->x1 != vp->x1)
    return up->x1 - vp->x1 ;
  else
    return up->x2 - vp->x2 ;
}

/*********************************************************************/

/* i need to sort by cDNA_clone */
static void cDNASwapX (Array hits)
{
  int tmp, i ;
  HIT *vp ;

  for (i = 0 ; i < arrayMax(hits) ; i++)
    { 
      vp = arrp (hits, i, HIT) ; 
      if (vp->x1 > vp->x2)
	{
	  tmp = vp->x1 ; vp->x1 = vp->x2 ; vp->x2 = tmp ;
	  tmp = vp->a1 ; vp->a1 = vp->a2 ; vp->a2 = tmp ;
	}
    }
}

/*********************************************************************/

static void cDNASwapA (Array hits)
{
  int tmp, i ;
  HIT *vp ;

  for (i = 0 ; i < arrayMax(hits) ; i++)
    { 
      vp = arrp (hits, i, HIT) ; 
      if (vp->a1 > vp->a2)
	{
	  tmp = vp->x1 ; vp->x1 = vp->x2 ; vp->x2 = tmp ;
	  tmp = vp->a1 ; vp->a1 = vp->a2 ; vp->a2 = tmp ;
	}
    }
}

/*********************************************************************/

static void  getcDNA_clonesAndClips (Array hits, KEYSET cDNA_clones, KEYSET clipTops, KEYSET clipEnds)
{
  int  i ;
  HIT *vp ;

  for (i = 0 ; i < arrayMax(hits) ; i++)
    { 
      vp = arrp (hits, i, HIT) ; 
      vp->cDNA_clone = keySet (cDNA_clones, KEYKEY(vp->est)) ;
      vp->clipTop = keySet (clipTops, KEYKEY(vp->est)) ;
      vp->clipEnd = keySet (clipEnds, KEYKEY(vp->est)) ;
    }
}

/*********************************************************************/

static void  saveHits (KEY cosmid, Array hits) 
{
  int j ;
  OBJ Cosmid = 0, Est = 0 ;
  KEY old ; 
  HIT *up ;

  cDNASwapA (hits) ;
  arraySort (hits, cDNAOrderByA1) ;

  Cosmid = bsUpdate(cosmid) ;
  if (bsFindTag (Cosmid, _Hit))
    bsRemove (Cosmid) ;

  for (j = 0 ; j < arrayMax (hits); j ++)
    {
      up = arrp(hits, j, HIT) ;
      if (up->est < 2) /* ghost */ 
	continue ;
      bsAddKey (Cosmid, _Hit, up->est) ;
      bsAddData (Cosmid, _bsRight, _Int, &(up->a1));
      bsAddData (Cosmid, _bsRight, _Int, &(up->a2)) ;
      bsAddData (Cosmid, _bsRight, _Int, &(up->x1)) ;
      bsAddData (Cosmid, _bsRight, _Int, &(up->x2)) ;
    }
  bsSave(Cosmid) ;
  
  cDNASwapX (hits) ;
  arraySort (hits, cDNAOrderByX1) ;
  for (j = 0, old = 0; j < arrayMax (hits); j ++)
    {
      up = arrp(hits, j, HIT) ;

      if (up->est != old)
	{
	  bsSave (Est) ;
	  old = up->est ;
	  if (up->est > 1)
	    Est = bsUpdate (old) ;
	}
      if (!Est) continue ;	
      bsAddKey (Est, _Hit, cosmid) ;
      bsAddData (Est, _bsRight, _Int, &(up->x1)) ;
      bsAddData (Est, _bsRight, _Int, &(up->x2)) ;
      bsAddData (Est, _bsRight, _Int, &(up->a1));
      bsAddData (Est, _bsRight, _Int, &(up->a2)) ;
    }
  bsSave (Est) ;
}

/*********************************************************************/

static void  saveAlignment (KEY cosmid, Array hits, Array dna, char *nom) 
{
  int i, j ;
  OBJ obj = 0 ;
  KEY alignment = 0, subAlignment = 0 ;
  HIT *up ;

  cDNASwapA (hits) ;
  arraySort (hits, cDNAOrderByA1) ;
  cDNASwapX (hits) ;

  i = 0 ;
  if (!nom || !*nom) /* let the system decide, no reuse */
    {
      nom = "alignment" ;
      if (lexword2key (messprintf("%s_%s",name(cosmid), nom), &alignment, _VSequence))
	{
	  while (i++, lexword2key (messprintf("%s_%s.%d",name(cosmid), nom, i), &alignment, _VSequence)) ;
	  lexaddkey (messprintf("%s_%s.%d",name(cosmid), nom,i), &alignment, _VSequence) ;
	}
      else
	lexaddkey (messprintf("%s_%s",name(cosmid), nom), &alignment, _VSequence) ;
      
      if (lexword2key ("z_1", &subAlignment, _VSequence))
	{
	  i = 0 ;
	  while (i++, lexword2key (messprintf("z_.%d",i), &subAlignment, _VSequence)) ;
	  lexaddkey (messprintf("z_.%d",i), &subAlignment, _VSequence) ;
	}
      else
	lexaddkey ("z_1", &subAlignment, _VSequence) ;
    }
  else  /* reuse */
    {
      lexaddkey (messprintf("%s_%s",name(cosmid), nom), &alignment, _VSequence) ;
      lexaddkey (messprintf("z_%s_%s",name(cosmid), nom), &subAlignment, _VSequence) ;
    }
  
  /*
  lexaddkey(name(subAlignment), &key, _VDNA) ;
  dnaStoreDestroy (key, arrayCopy (dna)) ;  // sets length in subAlignment 
  */
  obj = bsUpdate(subAlignment) ;
  if (bsFindTag (obj, _Assembled_from))
    bsRemove (obj) ;
  bsAddTag (obj, str2tag("Is_Reference")) ;

  for (j = 0 ; j < arrayMax (hits); j ++)
    {
      up = arrp(hits, j, HIT) ;

      bsAddKey (obj, _Assembled_from, up->est) ;
      bsAddData (obj, _bsRight, _Int, &(up->a1));
      bsAddData (obj, _bsRight, _Int, &(up->a2)) ;
      bsAddData (obj, _bsRight, _Int, &(up->x1)) ;
      bsAddData (obj, _bsRight, _Int, &(up->x2)) ;
    }
  bsSave(obj) ;

  obj = bsUpdate(alignment) ;
  if (obj)
    {
      bsAddKey (obj, _Subsequence, cosmid) ;
      i = 1 ;
      bsAddData (obj, _bsRight, _Int, &i) ;
      i = arrayMax(dna) ;
      bsAddData (obj, _bsRight, _Int, &i) ;
      bsAddKey (obj, _Subsequence, subAlignment) ;
      i = 1 ;
      bsAddData (obj, _bsRight, _Int, &i) ;
      i = arrayMax(dna) ;
      bsAddData (obj, _bsRight, _Int, &i) ;
      bsSave (obj) ;
    }
}

/*********************************************************************/
/*********************************************************************/

void cDNARemoveCloneFromGene (KEY clone, KEY gene) 
{
  KEY seq = 0 ;
  OBJ obj = 0 ;
  int x1, x2 ;
  char *cp ;

  if ((obj = bsUpdate(gene)))
    {
      KEY tmp ;

      if (bsFindKey (obj, _cDNA_clone, clone))
	bsRemove (obj) ;
      bsGetKey (obj, _Genomic_sequence, &seq) ;
      x1 = x2 = 0 ;
      if (bsGetData (obj, _Covers, _Int, &x1) &&
	  bsGetData (obj, _bsRight, _Text, &cp) &&
	  bsGetData (obj, _bsRight, _Int, &x1) &&
	  bsGetData (obj, _bsRight, _Int, &x2)) ;
      bsAddKey (obj, str2tag("Homolog_cDNA"), clone) ;
      bsSave (obj) ;
      if (bIndexGetKey(gene, _cDNA_clone, &tmp))
	cDNARealignGene (gene, 0, 0) ;
      else
	{	 
	  obj = bsUpdate (gene) ; 
	  if (obj) bsKill(obj) ;
	}
      if (seq && (obj = bsUpdate (seq)))
	{
	  bsAddKey (obj, _Discarded_cDNA, clone) ;
	  if (x2)
	    { 
	      if (x1 > x2) { int tmp = x1 ; x1 = x2 ; x2 = tmp ; }
	      bsAddData (obj, _bsRight, _Int, &x1) ;
	      bsAddData (obj, _bsRight, _Int, &x2) ;
	    }
	  bsSave (obj) ;
	}
   }
}

/*********************************************************************/
/*********************************************************************/

static void fixEstIntrons (KEY cosmid, KEY est, 
			  Array dna, Array dnaR, 
			  Array oneHits, Array newHits)
{
  A_ERR *ep1, *ep2 ;
  static Array err1 = 0, err2 = 0 ;
  Array cDNA = 0 ;
  int a1, a2, b1, b2, x1, x2, y1, y2, dx ;
  int j0, j1, i1, i2, besti1, besti1_0, nbest ;
  char *cp, *cq ;
  BOOL debug = FALSE, isNewe2 ;
  HIT *up, *vp ;
  
  if (!arrayMax(oneHits))
    return ;
  up = arrp(oneHits, 0, HIT) ;
  
  j1 = arrayMax (newHits) ;
  cDNA = getSeqDna (est) ;

  for (j0 = 0 ; j0 < arrayMax(oneHits) - 1 ; j0++)
    {
      up = arrp(oneHits, j0, HIT) ; vp = up+1 ;
      if (vp->est != est) break ;
      if (up->est != est) break ;
      arrayDestroy (err1) ;
      arrayDestroy (err2) ;

      if (up->x2 < vp->x1)
	goto intronDone ;
      /* recompute err, because a1 has moved after adjustment of first intron */

      a1 = up->a1 - 1 ; a2 = up->a2 ; x1 = up->x1 - 1 ; x2 = up->x2 ;
    lao:
      if (a1 > a2)
	{ 
	  showHits(oneHits) ;
	  messerror ("isUp = true in fixEstIntrons cosmid = %s, est = %s a1=%d  a2=%d", 
		     name(cosmid), name(est), a1, a2) ;  
	  goto intronDone ; /* failure */
	}
      err1 = spliceTrackErrors (cDNA, x1, &x2, dna, a1, &a2, 0, err1,2) ;
      
      b1 = vp->a1 - 1 ; b2 = vp->a2 ; y1 = vp->x1 - 1 ; y2 = vp->x2 ;
  
      if (x2 < y1 && arrayMax(err1))
	{ 
	  i1 = arrayMax(err1) - 2 ; if (i1 < 0) i1 = 0 ;
	  ep1 = arrp(err1, i1, A_ERR) ;
	  a1 = ep1->iLong ; x1 = ep1->iShort ; a2 = up->a2 ;  x2 = up->x2 ;
	  if (a1 < a2 && x1 < x2)
	    goto lao ;
	  else
	    goto intronDone ; /* failure */
	}

      if (x2 <= y1)
	goto intronDone ;

      for (ep1 = 0, i1 = 0 ; i1 < arrayMax(err1) ; i1++)
	{ 
	  ep1 = arrp(err1, i1, A_ERR) ;
	  if (ep1->iShort >=  y1 && 
	      (i1 == arrayMax(err1) - 1 || ep1->type != AMBIGUE))
	    break ;
	}
    newe1:
      if (ep1)
	{  /* error is one base downstream, fitting Plato */
	  x2 = up->x2 = ep1->iShort ; up->a2 = ep1->iLong ;
	}
      dx = x2 + 1 - vp->x1 ; vp->x1 += dx ; vp->a1 += dx ;
      isNewe2 = FALSE ;
    newe2:
      b1 = vp->a1 - 1 ; b2 = vp->a2 ; y1 = vp->x1 - 1 ; y2 = vp->x2 ;
      err2 = spliceTrackErrors (cDNA, y1, &y2, dna, b1, &b2, 0, err2, 10) ; 
      
      /* adjust dx */
     if (arrayMax(err2) && !isNewe2)
       {
	 for (dx = 0, i2 = 0 ; i2 < arrayMax(err2) ; i2++)
	   {  
	     ep2 = arrp(err2, i2, A_ERR) ;
	     if (ep2->iShort > x2 + 10) break ;
	     switch (ep2->type)
	       {
	       case TROU: dx++ ; break ;
	       case TROU_DOUBLE: dx += 2; break ;
	       case INSERTION: dx-- ; break ; 
	       case INSERTION_DOUBLE: dx -= 2; break ;
	       default: break ;
	       }
	   }
	 isNewe2 = TRUE ;
	 if (dx * dx > 1) { vp->a1 += dx ; goto newe2 ; }
       }
      if (arrayMax(err2))
	{
	  besti1 = besti1_0 = i1 ; nbest = i1 ;  /* might be an over estimate */
	  for (i1 = besti1 , i2 = 0 ; i1 < arrayMax(err1) ; i1++)
	    {
	      ep1 = arrp(err1, i1, A_ERR) ;
	      for (i2 = 0; i2 < arrayMax (err2) ; i2++)
		{ 
		  ep2 = arrp(err2, i2, A_ERR) ;
		  if (ep2->iShort >= ep1->iShort &&
		      i1 - i2  < nbest)
		    { besti1 = i1 ; nbest = i1 - i2 ; }
		}
	      if (b2 >  ep1->iShort  &&  i1 - i2   < nbest)
		{ besti1 = i1 ; nbest = i1 - i2  ; }
	    }
	  if (besti1 != besti1_0)
	    { ep1 = arrp(err1, besti1, A_ERR) ; goto newe1 ; }
	  for (i2 = 0 ; i2 < arrayMax(err2) ; i2++)
	    {  
	      ep2 = arrp(err2, i2, A_ERR) ;
	      if (ep2->iShort >= x2)
		{ 
		  switch (ep2->type)
		    {
		    case AMBIGUE:
		    case ERREUR:
		      break ;
		    case TROU: /* ok1 */ 
		      cp = arrp (cDNA, ep2->iShort, char) ; y2 = ep2->iShort ;
		      cq = arrp (dna, ep2->iLong + 1, char) ;
		      while (y2 > 0 && *cp == *cq) { cp-- ; cq-- ; y2-- ; }
		      if (y2 <= y1) ep2->iLong += 1 ; /* tranfert trou to top */
		      if (debug) printf("Cosmid %s  %s err2, trou at vp->x1 = %d\n",
					name(cosmid), name(vp->est), vp->x1) ;
		      break ;
		    case TROU_DOUBLE:
		      cp = arrp (cDNA, ep2->iShort, char) ; y2 = ep2->iShort ;
		      cq = arrp (dna, ep2->iLong + 2, char) ;
		      while (y2 > 0 && *cp == *cq) { cp-- ; cq-- ; y2-- ; }
		      if (y2 <= y1) ep2->iLong += 2 ; /* tranfert trou to top */
		      if (debug) printf("Cosmid %s  %s err2, trou double at vp->x1 = %d\n",
					name(cosmid), name(vp->est), vp->x1) ;
		      break ;
		    case INSERTION: 
		      cp = arrp (cDNA, ep2->iShort + 1, char) ; y2 = ep2->iShort + 1 ;
		      cq = arrp (dna, ep2->iLong, char) ;
		      while (y2 > 0 && *cp == *cq) { cp-- ; cq-- ; y2-- ; }
		      if (y2 <= y1) ep2->iLong -= 1 ; /* tranfert insert to top */
		      /* else rien zk637, cm08g12, mais err2 calcule shifte */
		      if (debug) printf("Cosmid %s  %s err2, insert at vp->x1 = %d\n",
					name(cosmid), name(vp->est), vp->x1) ;
		      break ;
		    case INSERTION_DOUBLE:
		      cp = arrp (cDNA, ep2->iShort + 2, char) ; y2 = ep2->iShort + 2 ;
		      cq = arrp (dna, ep2->iLong, char) ;
		      while (y2 > 0 && *cp == *cq) { cp-- ; cq-- ; y2-- ; }
		      if (y2 <= y1) ep2->iLong -= 2 ; /* tranfert insert to top */
		      if (debug) printf("Cosmid %s  %s err2, insert double at vp->x1 = %d\n",
					name(cosmid), name(vp->est), vp->x1) ;
		      break ;
		    };  
		  vp->a1 = ep2->iLong - ep2->iShort + vp->x1 ;
		  goto intronDone ;
		}
	    }
	  vp->a1 = b2 + vp->x1 - y2 ; /* no error till y2 */
	}
      else
	vp->a1 = b1 + vp->x1 - y1 ;
      /* find du fix de cet intron */		      

    intronDone:
      array (newHits, j1++, HIT) = *up++ ;
    }
  if (up->est == est)
    array (newHits, j1++, HIT) = *up ; /* register last */
  
  arrayDestroy (err1) ;
  arrayDestroy (err2) ;
}

/*********************************************************************/
 
static void complementHits (Array hits, int max, int i1, int i2)
{
  int i ;
  HIT *vp ;
  
  for (i = i1; i < i2 ; i++)
    { 
      vp = arrp(hits, i, HIT) ;
      vp->a1 = max - vp->a1 + 1 ; vp->a2 = max - vp->a2 + 1 ;
    }
}

/*********************************************************************/
 
static void fixOneIntrons (KEY cosmid, KEY est, Array oneHits, Array hits, Array dna, Array dnaR)
{
  BOOL isUp ;
  int i1, max = arrayMax(dna) ;
  HIT *vp ; 
  Array dnaLong ;

  if (arrayMax(oneHits))
    {
      vp = arrp(oneHits, 0, HIT) ;
      if (vp->a1 > vp->a2)
	{
	  isUp = TRUE ;  dnaLong = dnaR ;
	  complementHits (oneHits, max, 0, arrayMax(oneHits)) ; 
	}
      else
	{
	  isUp = FALSE ; dnaLong = dna ;
	}
      i1 = arrayMax(hits) ;
      fixEstIntrons (cosmid, est, dnaLong, 0, oneHits, hits) ;
      if (isUp)
	complementHits (hits, max, i1, arrayMax(hits)) ; 
    }
}

/*********************************************************************/

static void fixIntrons (KEY cosmid, Array hits, Array dna, Array dnaR)
{
  Array oldHits = arrayCopy (hits), oneHits = 0 ;
  KEY est, old ;
  int j, j1 ;
  HIT *up ;


  cDNASwapX (oldHits) ;
  arraySort (oldHits, cDNAOrderByX1) ;
  arrayMax (hits) = 0 ;

  oneHits = arrayCreate (12, HIT) ; j1 = 0 ; old = 0 ;
  for (old = 0, j = 0 ; j < arrayMax(oldHits) ; j ++)
    {
      up = arrp(oldHits, j, HIT) ;
      est = up->est ;
      if (est != old)
	{
	  if (old) fixOneIntrons (cosmid, old, oneHits, hits, dna, dnaR) ;
	  oneHits = arrayReCreate (oneHits, 12, HIT) ;  j1 = 0 ; old = est ;
	}
      array (oneHits, j1++, HIT) = *up ;
    }
  if (old) fixOneIntrons (cosmid, old, oneHits, hits, dna, dnaR) ;

  arrayDestroy (oldHits) ;
  arrayDestroy (oneHits) ;
}

/*********************************************************************/

static void dropNonColinearHits (Array hits, int *jp, int j1, int j2)
{
  int i, j , k, dx, da, da1, da2, best, nn, ng, bestnn ;
  BOOL isUp ;
  HIT *up, *vp, *wp ;
  static KEYSET group = 0 ;

  if (j1 >= j2) return ;
  up = arrp(hits, j1, HIT) ;
  isUp =  up->x2 < up->x1 ? TRUE : FALSE ;

  group = keySetReCreate (group) ; best = bestnn = 0 ;
  for (k = 0 , i = j1 ; i < j2 ; i++)
    {
      /* for each hit, create a group of compatible hits and sum them */
      if (keySet(group, i)) continue ;
      up = arrp(hits, i, HIT) ;
      keySet(group, i) = ++k ;
      vp = up ; nn = up->a2 - up->a1 ; ng = 1 ;
      da1 = vp->a2 - vp->a1 ;
      for (j = i+1 ; j < j2 ; j++)
	{
	  wp = arrp(hits, j, HIT) ;
	  da = wp->a1 - vp->a2 ;
	  da2 = wp->a2 - wp->a1 ;
	  dx = wp->x1 - vp->x2 ; /* negatif intron overshooting */
	  if (!isUp) dx = - dx ;  /* now an x overlap is seen as a positive number */
	  if (da - dx + 30 < 0  ||  /* x overlap larger than a overlap */
	      da + dx < 30)    /* x gap (-dx) larger than a gap */
	    continue ;  /* start accumulating in new group */
	  if ((!isUp && (2*dx > vp->x2 - vp->x1 || 
			2*dx > wp->x2 - wp->x1)) ||
	      ( isUp && (2*dx > vp->x1 - vp->x2 || 
			2*dx > wp->x1 - wp->x2)))
	    continue ; /* inersect >= half the length */
	  if (da > 1000 && 
	      (
	       (ng == 1 &&  da1 < 50 && 10 * da1 < da) ||
	       (j == j2 - 1 && da2 < 50 && 10 * da2 < da )	      
	       )
	      )
	    continue ; /* do not trust very long terminal introns */
	  nn += wp->a2 - wp->a1 ; ng++ ;
	  keySet(group, j) = k ; 
	  vp = wp ; 
	}
      if (nn > bestnn) { bestnn = nn ; best = k ; }
    }

  /* only register member of best group */
  for (i = j1; i < j2 ; i++)
    { 
      if (keySet(group, i) == best)
	{
	  up = arrp(hits, i, HIT) ;
	  arr (hits, (*jp)++, HIT) = arr(hits, i, HIT) ;    
	}
      /*
	up = arrp(hits, i, HIT) ;
	printf("%s %d %d %d %d group %d\n",
	name(up->est), up->a1, up->a2, up->x1, up->x2,keySet(group, i)) ; 
	*/
    }
}

/*********************************************************************/

static void bumpHits (Array hits, int *jp, int j1, int j2)
{
  int i, n, nn,  oldx2, oldx1 = 0 ;
  HIT *up = 0 ;
  KEY est = 0 ;
  int v1 = 0 , v2 = 0 ;

  nn = 0 ;
  for (i = j1; i < j2 ; i++)
    {
      up = arrp(hits, i, HIT) ;
      if (!est) { oldx1 = up->x1 ; oldx2 = up->x2 ; }
      est = up->est ; 
      v1 = up->clipTop ; v2 = up->clipEnd ;
      n = up->reverse ? up->x2 - up->x1 : up->x1 - up->x2 ;
      nn += n ;
    }
  for (i = j1; i < j2 ; i++)  /* accept all */
    { 
      up = arrp(hits, i, HIT) ;
      arr (hits, (*jp)++, HIT) = arr(hits, i, HIT) ;    
    }
}

/*********************************************************************/

static void dropSmallHits (Array hits, int *jp, int j1, int j2, int mini)
{
  int i, n, nn, nt, gap = 0, oldx2, oldx1 = 0 ;
  HIT *up = 0 ;
  KEY est = 0 ;
  int v1 = 0 , v2 = 0 ;

  nn = 0 ;
  for (i = j1; i < j2 ; i++)
    {
      up = arrp(hits, i, HIT) ;
      if (!est) { oldx1 = up->x1 ; oldx2 = up->x2 ; }
      est = up->est ; 
      v1 = up->clipTop ; v2 = up->clipEnd ;
      n = up->reverse ? up->x2 - up->x1 : up->x1 - up->x2 ;
      nn += n ;
    }
  /* note that we do an algebraic sum so zitterbewegung cancels out */
  n = nn < 0 ? -nn : nn ;
  /* counting negative gap penalty should eliminate random
     long full genes that match on repeats 
  */
  if (up->x2 > oldx1) gap = up->x2 - oldx1 - n ;
  else gap = oldx1 - up->x2 - n ;
  n -= gap/2 ;
  nt = v2 - v1 + 1 ;
  if ((n < 150 && 10 * n < 5 * nt) || /* 50% coverage at least */
      (n >= 150 && 10 * n < nt))
    return ;
  nn = nn > 0 ? 1 : -1 ;
  for (i = j1; i < j2 ; i++)
    { 
      up = arrp(hits, i, HIT) ;
      n = up->reverse ? up->x2 - up->x1 : up->x1 - up->x2 ;
      if (n * nn < mini) /* drop backwards or mini hits */
	continue ;
      arr (hits, (*jp)++, HIT) = arr(hits, i, HIT) ;    
    }
}

/*********************************************************************/

static void dropBackwardHits (Array hits, int *jp, int j1, int j2)
{
  int i, n, n1, n2;
  HIT *up ;
  KEY est ;
  int sens ;

  n = n1 = n2 = 0 ;
  for (i = j1; i < j2 ; i++)
    {
      up = arrp(hits, i, HIT) ;
      est = up->est ;
      n = up->x2 - up->x1 ;
      if (n > 0) n1 += n ; 
      else n2 -= n ;
    }
  sens = n1 > n2 ? 1 : -1 ;
  
  for (i = j1; i < j2 ; i++)
    {
      up = arrp(hits, i, HIT) ;
      est = up->est ;
      n = up->x2 - up->x1 ;
      if (sens * n > 0) 
	arr (hits, (*jp)++, HIT) = arr(hits, i, HIT) ;   
    } 
}

/*********************************************************************/

static void dropBadHits (Array hits, int type)
{
  KEY est, old ;
  int j, j0, j1;
  HIT *up ;

  cDNASwapA (hits) ;
  arraySort (hits, cDNAOrderByA1) ;

  j0 = j1 = 0 ;
  for (old = 0, j = 0 ; j < arrayMax(hits) ; j ++)
    {
      up = arrp(hits, j, HIT) ;
      est = up->est ;
      if (est != old)
	{
	  if (j > j1)
	    {
	      if (type == 1)  dropBackwardHits (hits, &j0, j1, j) ;
	      else if (type == 2) dropSmallHits (hits, &j0, j1, j, 12) ;
	      else if (type == 3) dropNonColinearHits (hits, &j0, j1, j) ;
	      else if (type == 4) bumpHits (hits, &j0, j1, j) ;
	      else if (type == 5) dropSmallHits (hits, &j0, j1, j, 5) ;
	    }
	  j1 = j ;
	}
      old = up->est ;
    }
  if (j > j1)
    {
      if (type == 1)  dropBackwardHits (hits, &j0, j1, j) ;
      else if (type == 2) dropSmallHits (hits, &j0, j1, j, 12) ;
      else if (type == 3) dropNonColinearHits (hits, &j0, j1, j) ;
      else if (type == 4) bumpHits (hits, &j0, j1, j) ;
      else if (type == 5) dropSmallHits (hits, &j0, j1, j, 8) ;
    }
  arrayMax (hits) = j0 ;
}

/*********************************************************************/

static void dropOutOfZoneHits (Array hits, int z1, int z2)
{
  int j, j1 ;
  HIT *up ;

  if (z1 >= z2) return ;

  cDNASwapA (hits) ;
  arraySort (hits, cDNAOrderByA1) ;

  for (j = 0, j1 = 0 ; j < arrayMax(hits) ; j++)
    {
      up = arrp(hits, j, HIT) ;
      if (up->a2 < z1 || up->a1 > z2)
	continue ;
      if (j != j1)
	arr (hits, j1, HIT) = arr(hits, j, HIT) ;   
      j1++ ;
    } 
  arrayMax (hits) = j1 ;
}

/*********************************************************************/

static void dropOneBackToBackPair (Array hits, int *jp, int j1, int j2)
{
  int i, x1, x2, y1, y2, a1, a2, b1, b2 ;
  HIT *up ;
  KEY est = 0 ;
  int  s1, s2 ;
  BOOL u1, u2 ;

  x1 = x2 = y1 = y2 = 0 ;
  a1 = a2 = b1 = b2 = 0 ;
  for (i = j1; i < j2 ; i++)
    {
      up = arrp(hits, i, HIT) ;
      if (est != up->est) /* all exons allready colinear, checkone */
	{
	  if (up->reverse)
	    { b1 = up->a1 ; b2 = up->a2 ; y1 = up->x1 ; y2 = up->x2 ; }
	  else
	    { a1 = up->a1 ; a2 = up->a2 ; x1 = up->x1 ; x2 = up->x2 ; }
	}
      else
	{
	  if (up->reverse)
	    { if (b2 < up->a2) b2 = up->a2 ; }
	  else
	    { if (a2 < up->a2) a2 = up->a2 ; }
	  if (up->reverse)
	    {
	      if (y1 < y2)
		{
		  if (y1 > up->x1) y1 = up->x1 ;
		  if (y2 < up->x2) y2 = up->x2 ;
		}
	      else
		{
		  if (y1 < up->x1) y1 = up->x1 ;
		  if (y2 > up->x2) y2 = up->x2 ;
		}
	    }
	  else
	    {
	      if (x1 < x2)
		{
		  if (x1 > up->x1) x1 = up->x1 ;
		  if (x2 < up->x2) x2 = up->x2 ;
		}
	      else
		{
		  if (x1 < up->x1) x1 = up->x1 ;
		  if (x2 > up->x2) x2 = up->x2 ;
		}
	    }
	}
      est = up->est ;
    }
  
  u1 = u2 = TRUE ;
  s1 = x2 - x1 ;
  s2 = y2 - y1 ;
  if (s1 * s2 > 0 || /* 3p 5p MUST be antiparallel */
      (s1 * s2 < 0 &&    /* antiparallel */
       ((s1 > 0 && a1 > b2) ||
	  (s1 < 0 && a2 < b1)))
      )
    if (s1 * s1 > s2 * s2) 
      u2 = FALSE ;
    else
      u1 = FALSE ;

  /* register happy few  */
  for (i = j1; i < j2 ; i++)
    {
      up = arrp(hits, i, HIT) ;
      if (up->reverse && !u2)
	continue ;
      if (!up->reverse && !u1)
	continue ;
      if (u1 * u2 * s1 * s2 &&  /* drop overshooting */
	  (  /* but do not drop pieces longer than the opposite strand */
	   (!up->reverse && s1 > 0 && up->a1 > b2 && x2 - up->x1 < - 1.7 * s2) ||
	   ( up->reverse && s2 < 0 && up->a2 < a1 && y1 - up->x2 < 1.7 * s1) ||
	   (!up->reverse && s1 < 0 && up->a2 < b1 && x1 - up->x2 < 1.7 * s2) ||
	   ( up->reverse && s2 > 0 && up->a1 > a2 && y2 - up->x1 < - 1.7 * s1))
	  )
	continue ;
      if (i != *jp)
	arr (hits, *jp, HIT) = arr(hits, i, HIT) ; 
      (*jp)++ ;
    } 
}

/*********************************************************************/

static void dropBackToBackPairs (Array hits)
{
  KEY old, clone ;
  int j, j0, j1;
  HIT *up ;

  cDNASwapA (hits) ;
  arraySort (hits, cDNAOrderByA1) ;

  j0 = j1 = 0 ;
  for (old = 0, j = 0 ; j < arrayMax(hits) ; j ++)
    {
      up = arrp(hits, j, HIT) ;
      clone = up->cDNA_clone ;
      if (clone != old)
	{
	  if (j > j1)
	    dropOneBackToBackPair (hits, &j0, j1, j) ;
	  j1 = j ;
	}
      old = up->cDNA_clone ;
    }
  if (j > j1)
    dropOneBackToBackPair (hits, &j0, j1, j) ;

  arrayMax (hits) = j0 ;
}

/*********************************************************************/

static void dropOneDoubleRead (Array hits, KEYSET clipEnds, int *jp, int j1, int j2)
{
  int i,a1, a2, b1, b2 , milieu, amax, bmin, drop ;
  HIT *up ;
  KEY est = 0 ;

  a1 = a2 = b1 = b2 = 0 ;

  for (i = j1; i < j2 ; i++)
    {
      up = arrp(hits, i, HIT) ;
      if (est != up->est)
	{
	  if (up->x1 > up->x2)
	    { b1 = up->a1 ; b2 = up->a2 ;}
	  else
	    { a1 = up->a1 ; a2 = up->a2 ; }
	}
      else
	{
	  if (up->x1 > up->x2)
	    {
	      if (b1 > up->a1) b1 = up->a1 ;
	      if (b2 < up->a2) b2 = up->a2 ;
	    }
	  else
	    {
	      if (a1 > up->a1) a1 = up->a1 ;
	      if (a2 < up->a2) a2 = up->a2 ;
	    }
	}
      est = up->est ;
    }
  
  /* now i cover my on genomic from a1 a2 downwards, then from b1 to b2 upwards */
  if (b1 > 0 && a2 > b1 + 30)
    { milieu = (a2 + b1)/2 ; 
      amax = milieu + 15 ; bmin = milieu - 15 ; 
    }
  else
    { amax = a2 ; bmin = b1 ; }
  /* do not stop inside an intron */
  for (i = j1; i < j2 ; i++)
    {
      up = arrp(hits, i, HIT) ;
      if (up->x1 > up->x2)
	continue ;
      else
	{
	  if (amax < up->a2)
	    {
	      if (amax < up->a1 + 30)
		{
		  amax = up->a1 + 30 ;
		  if (amax >= up->a2)
		    amax = up->a2 - 1 ;
		}
	      break ;
	    }
	}
    }
  for (i = j2 - 1; i >= j1 ; i--)
    {
      up = arrp(hits, i, HIT) ;
      if (up->x1 < up->x2)
	continue ;
      else
	{
	  if (bmin > up->a1)
	    {
	      if (bmin > up->a2 - 30)
		{
		  bmin = up->a2 - 30 ;
		  if (bmin <= up->a1)
		    bmin = up->a1 + 1 ;
		}
	      break ;
	    }
	}
    }

  /* register happy few  */
  for (i = j1; i < j2 ; i++)
    { /* drop overshooting */
      up = arrp(hits, i, HIT) ;
      /* do not trim reads that have a polyA 
      if (clipEnds && KEYKEY(up->est) < keySetMax(clipEnds) &&
	  (class(keySet(clipEnds, KEYKEY(up->est))) & 1)) ;
	  */
      if (TRUE)
	{	  
	  if (up->x1 > up->x2)
	    {
	      if (up->reverse ||  
		  (!up->reverse && (b1 > a1 + 30)))
		{
		  if (up->a2 < bmin)
		    continue ;
		  if (up->a1 < bmin)
		    { 
		      drop = bmin - up->a1 ;
		      up->a1 += drop ;
		      up->x1 -= drop ;
		    }
		}
	    }
	  else
	    {
	      if (up->reverse || 
		  (!up->reverse && (a2 < b2 - 30))) /* do not cut  reads hitting all the way to the end */
		{
		  
		  if (up->a1 > amax)
		    continue ;
		  if (up->a2 > amax)
		    { 
		      drop = up->a2 - amax ;
		      up->a2 -= drop ;
		      up->x2 -= drop ;
		    }
		}
	    }
	}
      if (i != *jp)
	arr (hits, *jp, HIT) = arr(hits, i, HIT) ; 
      (*jp)++ ;
    } 
}

/*********************************************************************/

static void dropDoubleReads (Array hits, KEYSET clipEnds)
{
  KEY old, clone ;
  int j, j0, j1;
  HIT *up ;

  cDNASwapA (hits) ;
  arraySort (hits, cDNAOrderByA1) ;

  j0 = j1 = 0 ;
  for (old = 0, j = 0 ; j < arrayMax(hits) ; j ++)
    {
      up = arrp(hits, j, HIT) ;
      clone = up->cDNA_clone ;
      if (clone != old)
	{
	  if (j > j1)
	    dropOneDoubleRead (hits, clipEnds, &j0, j1, j) ;
	  j1 = j ;
	}
      old = up->cDNA_clone ;
    }
  if (j > j1)
    dropOneDoubleRead (hits, clipEnds, &j0, j1, j) ;

  arrayMax (hits) = j0 ;
}

/*********************************************************************/

static void mergeOverlappingHits (Array hits)
{
  int j, j1;
  HIT *up, *oldup ;
  BOOL isUp ;

  cDNASwapX (hits) ;
  arraySort (hits, cDNAOrderByX1) ;

  for (oldup = 0, j1 = 0, j = 0 ; j < arrayMax(hits) ; j ++)
    {
      up = arrp(hits, j, HIT) ;
      isUp = up->a1 <= up->a2 ? FALSE : TRUE ;

      if (oldup && oldup->est == up->est &&
	  oldup->x1 <= up->x1 &&
	  oldup->x2 >= up->x2) /* absorb new in old */
	continue ;
      if (oldup && oldup->est == up->est &&
	  oldup->x1 >= up->x1 &&
	  oldup->x2 <= up->x2) /* absorb old in new */
	{ *oldup = *up ; continue ; }


      if (oldup && oldup->est == up->est &&
	  oldup->x2 >= up->x1 - 1 &&
	  (
	   ( !isUp && 
	     oldup->a2 + 20 >= up->a1 - 1 + oldup->x2 - up->x1 &&
	     oldup->a1 < up->a1 
	     ) ||
	   ( isUp && 
	     oldup->a2 - 20 <= up->a1 + 1 - oldup->x2 + up->x1 &&
	     oldup->a1 > up->a1
	     )
	   )
	  )
	{ /* this will actually drop absurd hits when second  exon is before first */
	  oldup->x2 = up->x2 ;
	  if (!isUp && oldup->a2 < up->a2)
	    oldup->a2 = up->a2 ;
	  else if (isUp && oldup->a2 > up->a2)
	    oldup->a2 = up->a2 ;
	}
      else if (up->x2 != up->x1 && up->a2 != up->a1) /* drop nils */
	{
	  oldup = arrp (hits, j1++, HIT) ;
          *oldup = *up ;
	}
    }
  arrayMax (hits) = j1 ;
}

/*********************************************************************/
 
static void slideIntron (KEY cosmid, KEY est, Array dna, Array newHits, Array hits)
{
  int a1, a2, b1, b2, x1, x2, y1, y2, z1, z2, z3, z4, dz ;
  int i, j1, maxcDNA, maxdna = 0 ;
  HIT *up, *vp ;
  char *cp, *cq ;
  Array cDNA = 0 ;
  BOOL debug = FALSE ;

  cDNA = getSeqDna (est) ;
  maxcDNA = arrayMax(cDNA) ; maxdna = arrayMax(dna) ;

  
  for (j1 = 0 ; j1 < arrayMax(hits) - 1 ; j1++)
    {
      up = arrp (hits, j1, HIT) ;
      vp = up+1 ;
      a1 = up->a1 ; a2 = up->a2 ; x1 = up->x1 ; x2 = up->x2 ;
      b1 = vp->a1 ; b2 = vp->a2 ; y1 = vp->x1 ; y2 = vp->x2 ;

      /* slide back, slide forth, if slideIsPossible, 
	 stop on GT/AG, then AT/AC then GC/AG */
      z1 = a2 ; z2 = b1 ;
      cp = arrp(dna, a2 - 1, char) ; /* last base of top exon */
      cq = arrp(dna, b1 - 2, char) ; /* first base of top exon */
      while (z1 > a1 && (*cp & *cq)) { cp--; cq--; z1-- ; z2-- ;}
      z3 = z1 ; z4 = z2 ; cp++ ; cq++ ;
      while (z4 < b2 && (*cp & *cq)) { cp++; cq++; z3++ ; z4++ ;}
      if (z1 == z3) continue ; /* no sliding possible */
      /* recherche GT en aval de z1 */

      if (!up->reverse)
	{
	  cp = arrp(dna,z1, char) ; cq = arrp(dna,z2 - 3, char) ;
	  for (i=0 ; i <= z3 - z1 ; i++, cp++, cq++) /* majeur */
	    if (*cp == G_ && *(cp+1) == T_ && *cq == A_ && *(cq+1) == G_)
	      goto ok ;
	  cp = arrp(dna,z1, char) ; cq = arrp(dna,z2 - 3, char) ;
	  for (i=0 ; i <= z3 - z1 ; i++, cp++, cq++)  /* U12 */
	    if (*cp == G_ && *(cp+1) == A_ && *cq == A_ && *(cq+1) == G_)
	      goto ok ;
	  cp = arrp(dna,z1, char) ; cq = arrp(dna,z2 - 3, char) ;
	  for (i=0 ; i <= z3 - z1 ; i++, cp++, cq++)  /* U12 */
	    if (*cp == A_ && *(cp+1) == T_ && *cq == A_ && *(cq+1) == C_)
	      goto ok ;
	  cp = arrp(dna,z1, char) ; cq = arrp(dna,z2 - 3, char);
	  for (i=0 ; i <= z3 - z1 ; i++, cp++, cq++) /* nemat 2 ? */
	    if (*cp == G_ && *(cp+1) == T_ && *cq == A_ && *(cq+1) == C_)
	      goto ok ;
	  cp = arrp(dna,z1, char) ; cq = arrp(dna,z2 - 3, char);
	  for (i=0 ; i <= z3 - z1 ; i++, cp++, cq++) /* nemat 2 ? */
	    if (*cp == G_ && *(cp+1) == T_ && *cq == G_ && *(cq+1) == G_)
	      goto ok ;
	  cp = arrp(dna,z1, char) ; cq = arrp(dna,z2 - 3, char);
	  for (i=0 ; i <= z3 - z1 ; i++, cp++, cq++) /* nemat 3 ? */
	    if (*cp == G_ && *(cp+1) == C_ && *cq == A_ && *(cq+1) == T_)
	      goto ok ;
	  cp = arrp(dna,z1, char) ; cq = arrp(dna,z2 - 3, char);
	  for (i=0 ; i <= z3 - z1 ; i++, cp++, cq++) /* human 3 ? */
	    if (*cp == G_ && *(cp+1) == C_ && *cq == A_ && *(cq+1) == G_)
	      goto ok ;
	  cp = arrp(dna,z1, char) ; cq = arrp(dna,z2 - 3, char);
	  for (i=0 ; i <= z3 - z1 ; i++, cp++, cq++) /* worm new */
	    if (*cp == C_ && *(cp+1) == T_ && *cq == A_ && *(cq+1) == C_)
	      goto ok ;
	  /* test gt/ag with N include */
	  cp = arrp(dna,z1, char) ; cq = arrp(dna,z2 - 3, char) ;
	  for (i=0 ; i <= z3 - z1 ; i++, cp++, cq++) /* majeur */
	    if ((*cp & G_) && (*(cp+1) & T_) && (*cq & A_) && (*(cq+1) &  G_))
	      goto ok ;

	  i = 0 ;  /* failed to slide, pack left */
	}
      else  /* ZK637//yk264a4.3,le sliding doit complementer le motif cherche */
	{
	  cp = arrp(dna,z1, char) ; cq = arrp(dna,z2 - 3, char) ;
	  for (i=0 ; i <= z3 - z1 ; i++, cp++, cq++) /* majeur */
	    if (*cp == C_ && *(cp+1) == T_ && *cq == A_ && *(cq+1) == C_)
	      goto ok ;
	  cp = arrp(dna,z1, char) ; cq = arrp(dna,z2 - 3, char) ;
	  for (i=0 ; i <= z3 - z1 ; i++, cp++, cq++)  /* U12 */
	    if (*cp == C_ && *(cp+1) == T_ && *cq == T_ && *(cq+1) == C_)
	      goto ok ;
	  cp = arrp(dna,z1, char) ; cq = arrp(dna,z2 - 3, char) ;
	  for (i=0 ; i <= z3 - z1 ; i++, cp++, cq++)  /* U12 */
	    if (*cp == G_ && *(cp+1) == T_ && *cq == A_ && *(cq+1) == T_)
	      goto ok ;
	  cp = arrp(dna,z1, char) ; cq = arrp(dna,z2 - 3, char);
	  for (i=0 ; i <= z3 - z1 ; i++, cp++, cq++) /* nemat 2 ? */
	    if (*cp == G_ && *(cp+1) == T_ && *cq == A_ && *(cq+1) == C_)
	      goto ok ;
	  cp = arrp(dna,z1, char) ; cq = arrp(dna,z2 - 3, char);
	  for (i=0 ; i <= z3 - z1 ; i++, cp++, cq++) /* nemat 2 ? */
	    if (*cp == C_ && *(cp+1) == C_ && *cq == A_ && *(cq+1) == C_)
	      goto ok ;
	  cp = arrp(dna,z1, char) ; cq = arrp(dna,z2 - 3, char);
	  for (i=0 ; i <= z3 - z1 ; i++, cp++, cq++) /* nemat 3 ? */
	    if (*cp == A_ && *(cp+1) == T_ && *cq == G_ && *(cq+1) == C_)
	      goto ok ;
	  cp = arrp(dna,z1, char) ; cq = arrp(dna,z2 - 3, char);
	  for (i=0 ; i <= z3 - z1 ; i++, cp++, cq++) /* human 3 ? */
	    if (*cp == C_ && *(cp+1) == T_ && *cq == G_ && *(cq+1) == C_)
	      goto ok ;
	  cp = arrp(dna,z1, char) ; cq = arrp(dna,z2 - 3, char);
	  for (i=0 ; i <= z3 - z1 ; i++, cp++, cq++) /* worm new */
	    if (*cp == G_ && *(cp+1) == T_ && *cq == A_ && *(cq+1) == G_)
	      goto ok ;
	  /* test gt/ag with N include */
	  cp = arrp(dna,z1, char) ; cq = arrp(dna,z2 - 3, char) ;
	  for (i=0 ; i <= z3 - z1 ; i++, cp++, cq++) /* majeur */
	    if ((*cp & C_) && (*(cp+1) & T_) && (*cq & A_) && (*(cq+1) &  C_))
	      goto ok ;

	  i-- ; /* failed to slide, pack left */ 
	}
  
    ok:
      dz = z1 + i - up->a2 ;
      if (dz)
	if (debug) printf("Cosmid %s Moved %s %d %d %d %d by %d bases\n",
	       name(cosmid), name(up->est), up->a2, vp->a1, z1 + i, z2 + i, dz) ;
      up->a2 = z1 + i ; vp->a1 = z2 + i ; up->x2 += dz ;vp->x1 += dz ; 
      up++ ; 
    }

  for (j1 = 0 ; j1 < arrayMax(hits) ; j1++)
    array(newHits, arrayMax(newHits), HIT) = arr (hits, j1, HIT) ;
}

/*********************************************************************/
 
static void slideOneIntrons (KEY cosmid, KEY est, Array oneHits, Array hits, Array dna, Array dnaR)
{
  int i1, max = arrayMax(dna) ;
  HIT *vp ; 
  Array dnaLong ;
  BOOL isUp ;

  if (arrayMax(oneHits))
    {
      vp = arrp(oneHits, 0, HIT) ;
      if (vp->a1 > vp->a2)
	{
	  dnaLong = dnaR ; isUp = TRUE ;
	  complementHits (oneHits, max, 0, arrayMax(oneHits)) ; 
	}
      else
	{
	  dnaLong = dna ; isUp = FALSE ;
	}
      i1 = arrayMax(hits) ;
      slideIntron (cosmid, est, dnaLong, hits, oneHits) ;
      if (isUp)
	complementHits (hits, max, i1, arrayMax(hits)) ; 
    }
}

/*********************************************************************/

static void slideIntrons (KEY cosmid, Array hits, Array dna, Array dnaR)
{
  Array oldHits = arrayCopy (hits), oneHits = 0 ;
  KEY est, old ;
  int j, j1 ;
  HIT *up ;


  cDNASwapX (oldHits) ;
  arraySort (oldHits, cDNAOrderByX1) ;
  arrayMax (hits) = 0 ;

  oneHits = arrayReCreate (oneHits, 12, HIT) ; j1 = 0 ; old = 0 ;
  for (old = 0, j = 0 ; j < arrayMax(oldHits) ; j ++)
    {
      up = arrp(oldHits, j, HIT) ;
      est = up->est ;
      if (est != old)
	{
	  if (old) slideOneIntrons (cosmid, old, oneHits, hits, dna, dnaR) ;
	  oneHits = arrayReCreate (oneHits, 12, HIT) ;  j1 = 0 ; old = est ;
	}
      array (oneHits, j1++, HIT) = *up ;
    }
  if (old) slideOneIntrons (cosmid, old, oneHits, hits, dna, dnaR) ;

  arrayDestroy (oldHits) ;
  arrayDestroy (oneHits) ;
}

/*********************************************************************/
/*********************************************************************/

static void cDNAReverseOneClone (KEY cosmid, KEY est, Array hits, Array introns)
{
  int j1 ;
  HIT *vp ;
  OBJ obj = 0, obj1;
  KEY clone, key ;
  
  for (j1 = 0 ; j1 < arrayMax(hits) ; j1 ++)
    {
      vp = arrp(hits, j1, HIT) ;
      if (vp->est == est) vp->reverse = !vp->reverse ;
    }
  for (j1 = 0 ; j1 < arrayMax(introns) ; j1 ++)
    {
      vp = arrp(introns, j1, HIT) ;
      if (vp->est == est) vp->reverse = !vp->reverse ;
    }
  
  obj = bsUpdate (est) ;
  if (!obj) return ;
  if (bsFindTag (obj, _Reverse))
    bsAddTag (obj, _Direct) ;
  else
    bsAddTag (obj, _Reverse) ;
  bsGetKey (obj, _cDNA_clone, &clone) ;
  bsSave (obj) ;  
  
  if (clone)
    obj = bsUpdate (clone) ;
  if (!clone) return ;
  bsAddKey (obj, str2tag ("Reversed_by"), cosmid) ;
  if (bsGetKey (obj, _Read, &key))
    do
      {
	if (key != est) /* already done */
	  {
	    obj1 = bsUpdate (est) ;
	    if (!obj1) break ;
	    if (bsFindTag (obj1, _Reverse))
	      bsAddTag (obj1, _Direct) ;
	    else
	      bsAddTag (obj1, _Reverse) ;
	    bsSave (obj1) ;  
	  }
      } while (bsGetKey (obj, _bsDown, &key)) ;
  bsSave (obj) ;
}

/*********************************************************************/

static void dubious (KEY cosmid, KEY key)
{ 
  OBJ obj = 0 ;
  KEY clone =  ;

  if (bIndexGetKey(key, _cDNA_clone ,&clone) && (obj = bsUpdate (clone)))
    {
      bsAddKey (obj, str2tag ("Dubious_orientation_see"), cosmid) ;
      bsSave (obj) ;  
    }
}

/*********************************************************************/

static void cdnaReverseDubiousClone (KEY cosmid, Array hits, Array dna, Array dnaR)
{
  Array introns = 0 ;
  int j, j1, n1, n2, n3 ;
  KEY est, old = 0 ;
  HIT *up, *vp ;


  cDNASwapA (hits) ;
  arraySort (hits, cDNAOrderByA1) ;

  introns = arrayCreate (40, HIT) ;
  for (old = 0, j = 0, j1 = 0 ; j < arrayMax(hits) ; j ++)
    {
      up = arrp(hits, j, HIT) ;
      est = up->est ;
      if (est == old)
	{ 
	  vp = arrayp (introns, j1++, HIT) ;
	  vp->reverse = up->reverse ;
	  vp->a1 = (up -1)-> a2 ; vp->a2 = up->a1 ; 
	  
	}
      old = up->est ;
    }
  cDNASwapA (introns) ;
  arraySort (introns, cDNAOrderGloballyByA1) ;

  /* search backwards clones */ 
  for (n3 = 0, j = 0 ; j < arrayMax(introns) ; j ++)
    {
      up = arrp(introns, j, HIT) ;
      n1 = 0 ; n2 = 0 ;
      if (up->reverse) n2++ ; else n1++ ;
      for (j1 = j + 1 ; j1 < arrayMax(introns) ; j1 ++)
	{
	  vp = arrp(introns, j1, HIT) ;
	  if (vp->a1 > up->a1) break ;
	  if (vp->reverse) n2++ ; else n1++ ;
	}
      if (n1 * n2)  /* problem */
	{
	  if (n2 == 1 && n1 > 1) /* then n2 is dubious */
	    for (j1 = j + 1 ; j1 < arrayMax(introns) ; j1 ++)
	      {
		vp = arrp(introns, j1, HIT) ;
		if (vp->a1 > up->a1) break ;
		if (vp->reverse)
		  { n3++ ; cDNAReverseOneClone (cosmid, vp->est, hits, introns) ; }
	      }
	  else if (n1 == 1 && n2 > 1) /* then n1 is dubious */
	    { n3++ ;  cDNAReverseOneClone (cosmid, up->est, hits, introns) ; }
	  else
	    { 
	      dubious (cosmid, up->est) ; 
	      for (j1 = j + 1 ; j1 < arrayMax(introns) ; j1 ++)
		{
		  vp = arrp(introns, j1, HIT) ;
		  if (vp->a1 > up->a1) break ;
		  dubious (cosmid, vp->est) ;
		}
	    }
	}
    }
  arrayDestroy (introns) ;
}

/*********************************************************************/
/*********************************************************************/

static void removeIntronWobbling (KEY cosmid, Array hits, Array dna, Array dnaR)
{
  Array introns = 0 ;
  KEYSET plus = 0, minus = 0 ;
  int j, j1, a1, a2, np, nm ;
  HIT *up, *vp ;
  KEY old = 0 ;
  char *cp ;


  cDNASwapA (hits) ;
  arraySort (hits, cDNAOrderByA1) ;

  introns = arrayCreate (40, HIT) ;
  for (old = 0, j = 0, j1 = 0 ; j < arrayMax(hits) ; j ++)
    {
      up = arrp(hits, j, HIT) ;
      if (up->est == old)
	{ 
	  vp = arrayp (introns, j1++, HIT) ;
	  vp->reverse = up->reverse ^ (up->x1 > up->x2) ;
	  vp->a1 = (up -1)-> a2 ; vp->a2 = up->a1 ; 
	}
      old = up->est ;
    }
  cDNASwapA (introns) ;
  arraySort (introns,  cDNAOrderGloballyByA1 ) ;

  /* search backwards clones */ 
  nm = np = 0 ;
  plus = keySetCreate () ; minus = keySetCreate () ;
  for (j = 0 ; j < arrayMax(introns) ; j ++)
    {
      up = arrp(introns, j, HIT) ;
      a1 = up->a1 ; a2 = up->a2 ;
      for (j1 = j + 1 ; j1 < arrayMax(introns) ; j1++)
	{
	  vp = arrp(introns, j1, HIT) ;
	  if (vp->a1 > up->a2) break ;
	  if (up->reverse != vp->reverse) continue ;
	  if (vp->a1 == a1 + 1)
	    if (!up->reverse)
	      {
		cp = arrayp (dna, a1, char) ;
		if (*cp == G_ && *(cp+1) == T_)
		  keySet (minus, nm++) = a1 + 1 ;
		if (*(cp + 1) == G_ && *(cp+2) == T_)
		  keySet (plus, np++) = a1 ;
	      }
	  else
	      {
		cp = arrayp (dna, a1, char) ;
		if (*cp == C_ && *(cp+1) == T_)
		  keySet (minus, nm++) = a1 + 1 ;
		if (*(cp + 1) == C_ && *(cp+2) == T_)
		  keySet (plus, np++) = a1 ;
	      }
	  if (vp->a2 == a2 + 1)
	    if (!up->reverse)
	      {
		cp = arrayp (dna, a2 - 3, char) ;
		if (*cp == A_ && *(cp+1) == G_)
		  keySet (minus, nm++) = a2 + 1 ;
		if (*(cp + 1) == A_ && *(cp+2) == G_)
		  keySet (plus, np++) = a2 ;
	      }
	  else
	      {
		cp = arrayp (dna, a2 - 3, char) ;
		if (*cp == A_ && *(cp+1) == C_)
		  keySet (minus, nm++) = a2 + 1 ;
		if (*(cp + 1) == A_ && *(cp+2) == C_)
		  keySet (plus, np++) = a2 ;
	      }
	}
    }
  if (keySetMax(plus))
    {
      keySetSort (plus) ; keySetCompress (plus) ;
      for (j = 0 ; j < arrayMax(hits) ; j ++)
	{
	  up = arrp(hits, j, HIT) ;
	  if (keySetFind (plus, up->a1, 0))
	    up->a1++ ; /* creates as desided an error in the est */
	  if (keySetFind (plus, up->a2, 0))
	    up->a2++ ; /* creates as desided an error in the est */
	}
    }
  if (keySetMax(minus))
    {
      keySetSort (minus) ; keySetCompress (minus) ;
      for (j = 0 ; j < arrayMax(hits) ; j ++)
	{
	  up = arrp(hits, j, HIT) ;
	  if (keySetFind (minus, up->a1, 0))
	    up->a1-- ; /* creates as desided an error in the est */
	  if (keySetFind (minus, up->a2, 0))
	    up->a2-- ; /* creates as desided an error in the est */
	}
    }
  keySetDestroy (plus) ;
  keySetDestroy (minus) ;
  
  arrayDestroy (introns) ;
}

/*********************************************************************/
/*********************************************************************/

static void cDNAFilterDiscardedHits (KEY cosmid, Array hits)
{
  KEY cDNA_clone ;
  int j, j1, a1, a2 ;
  HIT *up ;
  OBJ Cosmid = bsCreate (cosmid) ;

  cDNASwapA (hits) ;
  arraySort (hits, cDNAOrderByA1) ;

  for (j = 0, j1 = 0 ; j < arrayMax(hits) ; j ++)
    {
      up = arrp(hits, j, HIT) ;
      cDNA_clone = up->cDNA_clone ;
      if (bsFindKey(Cosmid, _Discarded_cDNA, cDNA_clone))
	if (bsGetData (Cosmid, _bsRight, _Int, &a1))
	  {
	    if (bsGetData (Cosmid, _bsRight, _Int, &a2) &&
		a1 < up->a1 &&
		a2 > up->a2)
	      continue ; /* eliminate that hit */
	  }
	else
	  continue ; /* if coordinates of discard are unkown, assume discard */
      arr (hits,j1++,HIT) = *up ;  /* EN PLACE */
    }
  arrayMax(hits) = j1 ;
  bsDestroy (Cosmid) ;
}

/*********************************************************************/

static int cDNAFilterMultipleHits (KEY cosmid, Array hits)
{
  KEY cDNA_clone, old ;
  int j, j1,  nn2 = 0 ;
  HIT *up ;
  int min = 3 ; /* at least min hits */
 
  cDNASwapX (hits) ;
  arraySort (hits, cDNAOrderByX1) ;

  for (old = 0, j = 0, j1 = 0 ; j < arrayMax(hits) ; j ++)
    {
      up = arrp(hits, j, HIT) ;
      cDNA_clone = up->cDNA_clone ;
      if (cDNA_clone != old &&
	  (j + min > arrayMax(hits)  || (up+min-1)->cDNA_clone != cDNA_clone))
	continue ; /* not at least 3 hits to the same est */
      if (cDNA_clone!= old) nn2++ ;
      old = cDNA_clone ;
      arr (hits,j1++,HIT) = *up ;  /* EN PLACE */
    }
  arrayMax(hits) = j1 ;
  return nn2 ;
}

/*********************************************************************/

static void extendMultipleHits (KEY cosmid, Array hits, Array dna, Array dnaR)
{
  int j, j1, oldx1 = 0, oldx2 = 0, olda1, olda2 ;
  KEY oldEst = 0 ;
  HIT *vp ;

  cDNASwapX (hits) ;
  arraySort (hits, cDNAOrderByX1) ;

  for (j = 0, oldEst = 0, j1 = 0 ; j < arrayMax(hits) ; j++)
    {
      vp = arrp(hits, j, HIT) ;
      if (vp->est == oldEst &&
	  vp->x1 >= oldx1 && vp->x2 <= oldx2)
	continue ;  /* drop overlapping oligos */
      extendHits (cosmid, dna, dnaR, vp) ;
      oldEst = vp->est ; 
      oldx1 = vp->x1 ; oldx2 = vp->x2 ;
      olda1 = vp->a1 ; olda2 = vp->a2 ;
      array(hits, j1++, HIT) = *vp ;
    }
  arrayMax (hits) = j1 ; /* drop overlapping oligos */
}

/*********************************************************************/

static void dropEst (Array hits, int ii) 
{
  int i ;
  
  for (i= ii ; i < arrayMax(hits) - 1 ; i++)
    array (hits, i, HIT) = array(hits, i+1, HIT) ;
  arrayMax (hits) -= 1 ;
}

static BOOL getNewExon (KEY cosmid, KEY est, Array hits, int upIndex, int type, 
			Array dna, Array dnaR)
{
  int 
    v1, v2,
    a1, a2, x1, x2, /* in up */
    b1, b2, y1, y2, /* in vp */
    c1, c2, z1, z2, /* new */
    errmin, cbest, c2best, cmin, cmax, zmin, zmax,
    nz, nz1, maxDna = 0 ;
  static Array err = 0 ;
  Array cDNA = 0, longDna ;
  BOOL reverse = TRUE ;
  HIT *up = 0, *vp = 0 ;

  cmin = cmax = zmin = zmax = 0 ;
  a1 = a2 = b1 = b2 = v1 = v2 = y1 = y2 = x1 = x2 = 0 ;
  switch (type)
    {
    case 1:
      up = 0 ;
      vp = arrp(hits, upIndex, HIT) ;
      v1 = vp->clipTop ; v2 = vp->clipEnd ;
      break ;
    case 2:
      up = arrp(hits, upIndex, HIT) ;
      vp = 0 ;
      v1 = up->clipTop ; v2 = up->clipEnd ;
      break ;
    case 3:
      up = arrp(hits, upIndex, HIT) ;
      vp = arrp(hits, upIndex + 1, HIT) ;
      v1 = vp->clipTop ; v2 = vp->clipEnd ;
      break ;
    }
  if (up)
    { a1 = up->a1 ; a2 = up->a2 ; x1 = up->x1 ; x2 = up->x2 ;  }
  if (vp)
    { b1 = vp->a1 ; b2 = vp->a2 ; y1 = vp->x1 ; y2 = vp->x2 ;  }

  cDNA = getSeqDna (est) ;
  if (!cDNA) return FALSE ;

  reverse = up ? up->a1 > up->a2 : vp->a1 > vp->a2 ;
  if (reverse)
    { 
      longDna = dnaR ; 
      maxDna = arrayMax(dna) ;
      a1 = maxDna - a1 + 1 ;  a2 = maxDna - a2 + 1 ; b1 = maxDna - b1 + 1 ; b2 = maxDna - b2 + 1 ; 
    }
  else
    longDna = dna ;

  if (up && !vp)
    { 
      zmin = x2 ; zmax = v2 ;
      nz = zmax - zmin > 20 ? 6000 : 3000 ; 
      cmin = a2 ; cmax = arrayMax (dna) > a2 + nz ? a2 + nz : arrayMax (dna) ;
    }
  else if (!up && vp)
    { 
      zmin = y1 - 20 ; zmax = y1 ;
      if (zmin < v1) zmin = v1 ; 
      nz = zmax - zmin > 20 ? 6000 : 3000 ; 
      cmin = b1 > nz ? b1 - nz : 1 ; cmax = b1 ;
    }
  else if (up && vp)
    { 
      zmin = x2 - 8 ; zmax = y1 + 8 ; 
      if (zmin < v1) zmin = v1 ; 
      if (zmax > y2) zmax = y2 ;
      cmin = a2 - 8 ; cmax = b1 + 8 ;
      if (cmin < 0) cmin = 0 ; if (cmax > b2) cmax = b2 ;
    }
  else if (!up && !vp)
    return FALSE ; 
  
  nz = zmax - zmin ;

  if (nz < 8)
    {
      if (up)	  /* allonge a droite */
	{
	  up->x2 = zmax ; a2 = a2 + zmax - x2 ;
	  if (vp && y1 > 10 && b1 > 10) { vp->x1 -= 10 ; b1 -= 10 ; }
	  if (reverse)
	    { a2 = maxDna - a2 + 1 ; b1 = maxDna - b1 + 1 ; }
	  up->a2 = a2 ; if (vp) vp->a1 = b1 ;
	  return zmax > x2 ? TRUE : FALSE ;
	}
      else if (vp && b1 > y1) /* allonge a gauche */
	{
	  vp->x1 = zmin ; b1 += zmin - y1 ;  
	  if (reverse)  { b1 = maxDna - b1 + 1 ; }
	  vp->a1 = b1 ;
	  return zmin < y1 ? TRUE : FALSE ;
	}
      return FALSE ;
    }

  /* cherche un 20 mer */
  nz1 = nz > 20 ? 20 : nz ;
  z1 = zmin ; z2 = zmin + nz1 ; errmin = nz1/4 ;
  cbest = 1 ; c2best = 0 ;
  if (up)
    for (c1 = cmin ; c1 < cmax - nz1 ; c1++)
      {
	c2 = c1 + nz1 + 10 ; z2 = zmin + nz1 ;
	err = spliceTrackErrors (cDNA, z1, &z2, longDna, c1, &c2, 0, err, 2) ;
	if (arrayMax(err) < errmin)
	  { errmin = arrayMax(err) ; cbest = c1 ; c2best = c2 ; }
	else if (errmin < nz1/4 && cbest > a2 + (cmax - cmin)/3)
	  break ;
      }
  else /* search backwards */
    for (c1 = cmax - nz1 - 1 ; c1 >= cmin ; c1--)
       {
	c2 = c1 + nz1 + 10 ; z2 = zmin + nz1 ;
	err = spliceTrackErrors (cDNA, z1, &z2, longDna, c1, &c2, 0, err, 2) ;
	if (arrayMax(err) < errmin)
	  { errmin = arrayMax(err) ; cbest = c1 ; c2best = c2 ; }
	else if (errmin < nz1/4)
	  break ;
      }
  
  if (c2best < cbest + nz1 - 2)
    return FALSE ; 
  c1 = cbest ; c2 = c1 + nz1 ;  z1 = zmin ; z2 = zmin + nz1 ;

  nz = arrayMax(hits) ;
  vp = arrayp(hits, nz, HIT) ; /* make room */
  for (; nz > upIndex ; nz--, vp--) 
    *vp = *(vp - 1) ;
  /* thus, entry upIndex is duplicated */
  /* remember that hits may be relocated */
  if (type != 1) vp = arrayp (hits, upIndex + 1, HIT) ; /* insert after */
  else  vp = arrayp (hits, upIndex , HIT) ; /* before */
  /* vp->est, cDNA_clone, reverse were duplicated */
  if (reverse) { c1 = maxDna - c1 + 1 ; c2 = maxDna - c2 + 1 ; }
  vp->a1 = c1 ; vp->a2 = c2 ; vp->x1 = z1 ; vp->x2 = z2 ;
  /* printf("%s %s: new exon[%d] avant extend  %s %d %d  %d %d\n", 
	 name(cosmid), name(est), type, reverse ? "reverse" : "direct",
	 vp->a1, vp->a2, vp->x1, vp->x2) ;
  */
  extendHits( cosmid, dna, dnaR, vp) ;
  if (up && vp && vp->x2 <= up->x2) /* may happen after extend */
     {
       nz = upIndex ;  /* deallocate */ 
       for (; nz < arrayMax(hits) - 1 ; nz++, vp++)
	 *vp = *(vp + 1) ;
       arrayMax(hits) -- ;
       return FALSE ;
     }
  /*
    printf("%s %s: new exon[%d]  %s %d %d  %d %d\n", 
	 name(cosmid), name(est), type, reverse ? "reverse" : "direct",
	 vp->a1, vp->a2, vp->x1, vp->x2) ;
  */
  return TRUE ;
}

static void getOtherExons (KEY cosmid, Array hits, Array dna, Array dnaR)
{
  int j, j1, oldx1 = 0, oldx2 = 0, dx ;
  KEY oldEst ;
  HIT *up, *vp ;
  BOOL debug = FALSE ;

  cDNASwapX (hits) ;
  arraySort (hits, cDNAOrderByX1) ;

  for (j = 0, oldEst = 0, j1 = 0 ; j < arrayMax(hits) ; j++)
    {
      up = arrp(hits, j, HIT) ;
      vp = j < arrayMax(hits) - 1 ? arrp(hits, j + 1, HIT) : 0 ;
      if (up->est != oldEst && up->x1 > 6)
	{
	  if (debug) printf("Call new exon[1]: j=%d %s %s x1=%d x2 = %d ll=%d\n",
		 j, name(cosmid), name(up->est), up->x1,up->x2, up->clipEnd) ;
	  if (getNewExon (cosmid, up->est, hits, j, 1, dna, dnaR))
	    { j-- ; continue ; }
	}
      if (vp && oldEst && vp->est == oldEst &&
	  vp->x1 >= oldx1 && vp->x2 <= oldx2)
	{ dropEst (hits, j) ; j-- ; continue ; }  /* drop overlapping oligos */
      if (vp && up->est == vp->est && up->x2 < vp->x1 - 9)
	{
	  if (debug) printf("Call new exon[3]: j=%d %s %s x1=%d x2=%d ll=%d\n",
		 j, name(cosmid), name(up->est), up->x1,up->x2, up->clipEnd) ;
	  if (getNewExon (cosmid, up->est, hits, j, 3, dna, dnaR))
	    { j-- ; continue ; };
	}
      else if (vp && up->est == vp->est && up->x2 >= vp->x1 - 9)
	{
	  dx = (vp->x1 + 6 - up->x2)/2 ; 
	  if (dx > 0)
	    { up->x2 += dx ; vp->x1 -= dx ;
	    if (up->a1 < up->a2) { up->a2 += dx ; vp->a1 -= dx ;}
	    else { up->a2 -= dx ; vp->a1 += dx ; }
	    }
	}
      else if ((!vp || up->est != vp->est) && 
	  up->x2 < up->clipEnd - 10)
	{ 
	  if (debug) printf("Call new exon[2]: j=%d %s %s x1=%d x2=%d ll=%d\n",
		 j, name(cosmid), name(up->est), up->x1,up->x2, up->clipEnd) ;
	  getNewExon (cosmid, up->est, hits, j, 2, dna, dnaR) ;
	}
      if (j>=0)
	{
	  up = arrp(hits, j, HIT) ;
	  oldx1 = up->x1 ; oldx2 = up->x2 ;
	  oldEst = up->est ;
	}
      else
	oldEst = 0 ;
    }
}

/*********************************************************************/

static HIT* cDNA_clone2gene(KEY cDNA_clone, Array genes, Array sets)
{
  int j1 = arrayMax(genes) ;
  HIT *gh ;
  
  while (j1--)
    {
      gh = arrp(genes,j1,HIT) ;
      if (gh->type != 1 && gh->type != 5) continue ;
      if (keySetFind (arr(sets, gh->zone,KEYSET), cDNA_clone, 0))
	return gh ;
    }
  return 0 ;
}

/*********************************************************************/

static Array prepareGeneAssembly (KEY cosmid, Array genes, Array phits, Array hits, Array sets)
{
  int j1, jaf; 
  HIT *hh, *gh, *gh0=0, *ah ;
  Array   assembledFrom = 0 ;

  /* code duplicated below */

  if (!genes || !arrayMax(genes))
    return 0 ;
  assembledFrom = arrayCreate (300, HIT) ; jaf = 0 ;
  cDNASwapA (hits) ;
  arraySort (hits, cDNAOrderByA1) ;

  /* register genes in plaingenehits */
  for (j1 = 0 ; j1 < arrayMax(phits) ; j1++)
    {
      hh = arrp(phits, j1, HIT) ;
      if (hh->est ==1)
	continue ;
      gh = cDNA_clone2gene (hh->cDNA_clone, genes, sets) ;
      if (gh && gh->type != 5)
	hh->gene = gh->gene ;
    }

   /* register genes in hits and construct the af table */
  for (j1 = 0 ; j1 < arrayMax(hits) ; j1++)
    {
      hh = arrp(hits, j1, HIT) ;
      if (hh->est ==1)
	continue ;
      if (!hh->cDNA_clone) 
	{ 
	  showHits(hits) ;
	  messcrash("no cDNA_clone for %s", name(hh->est)) ;
	}
      gh = cDNA_clone2gene (hh->cDNA_clone, genes, sets) ;
      if (!gh)
	{  
	  /* happens when a clone is gobbled by its neighbours
	     if (gh0)
	     gh = gh0 ;
	     else
	     */
	  {
	    /* showHits(hits) ; */
	    messerror ("No gene associated to cosmid %s est %s ",
		       name(cosmid), name(hh->cDNA_clone)) ; 
	    continue ; 
	  }
	}
      if (gh && gh->type == 5) /* dropped gene */
	continue ;
      gh0 = gh ;
      hh->gene = gh->gene ;
      ah = arrayp (assembledFrom, jaf++, HIT) ;
      ah->gene = hh->gene ; ah->est = hh->est ;
      ah->x1 = hh->x1 ; ah->x2 = hh->x2 ;
      ah->clipTop = hh->clipTop ;
      ah->clipEnd = hh->clipEnd ;
      if (gh->a1 > gh->a2)
	{ ah->a2 = - hh->a2 + gh->a1 + 1 ; ah->a1 = - hh->a1 +  gh->a1 + 1 ; }
      else
	{ ah->a1 = hh->a1 - gh->a1 + 1 ; ah->a2 = hh->a2 - gh->a1 + 1 ; }
    }

  cDNASwapA (assembledFrom) ;
  arraySort (assembledFrom, cDNAOrderEstByA1) ;

  return assembledFrom ;
}

/*********************************************************************/

static void saveGeneAssembly (KEY cosmid, Array assembledFrom)
{
  int pp, j1, xts, xx1, xx2 ; KEY ts ;
  OBJ Est = 0, Gene = 0 ;
  KEY gene ;
  HIT *ah ;
  BSMARK mark = 0 ;

  if (assembledFrom)
    for (j1 = 0, gene = 0, Gene = 0 ; j1 < arrayMax(assembledFrom); j1++)
    {
      ah = arrp(assembledFrom, j1, HIT) ;
      if (gene != ah->gene)
	bsSave (Gene) ; /* resets Gene to zero */
      gene = ah->gene ;
      if (!Gene && !(Gene = bsUpdate(gene)))
	continue ;
      bsAddData (Gene, _Assembled_from, _Int, &(ah->a1));
      bsAddData (Gene, _bsRight, _Int, &(ah->a2)) ;
      bsAddKey (Gene, _bsRight, ah->est) ;
      bsAddData (Gene, _bsRight, _Int, &(ah->x1)) ;
      bsAddData (Gene, _bsRight, _Int, &(ah->x2)) ;
      pp = 0 ; ts = 0 ; xts = 0 ;
      if ((Est = bsCreate (ah->est)))
	{
	  bsGetData (Est, _PolyA_after_base,_Int, &pp) ; 
	  if (bsGetKey (Est, _Transpliced_to, &ts)) 
	    bsGetData (Est, _bsRight,_Int, &xts) ; 
	  bsDestroy (Est) ;
	}
      if (pp && pp == ah->x2)
	{
	  bsAddData (Gene, _PolyA_after_base,_Int, &(ah->a2)) ; 
	  if (bsGetData (Gene, _Splicing, _Int, &xx1))
	    do
	      {
		mark = bsMark (Gene, mark) ;
		if (bsGetData (Gene, _bsRight, _Int, &xx2) &&
		    xx2 == ah->a2 &&
		    bsPushObj (Gene))
		  {
		    int exonNb = 0 ;
		    
		    if (bsGetKeyTags (Gene, _bsRight, 0))
		      bsGetData (Gene, _bsRight, _Int, &exonNb) ;
		    
		    bsGoto (Gene, mark) ;
		    if (bsGetData (Gene, _bsRight, _Int, &xx2) &&
			xx2 == ah->a2 &&
			bsPushObj (Gene) &&
			bsGetKeyTags (Gene, _bsRight, 0))
		      bsRemove (Gene) ; 

		    bsGoto (Gene, mark) ;
		     if (bsGetData (Gene, _bsRight, _Int, &xx2) &&
			 xx2 == ah->a2 &&
			 bsPushObj (Gene) &&
			 bsAddKey (Gene, _bsRight, _Last_exon) &&
			 exonNb)
		       bsAddData (Gene, _bsRight, _Int, &exonNb) ;
		  }
		bsGoto (Gene, mark) ;
	      } while (bsGetData (Gene, _bsDown, _Int, &xx1)) ;
	}
		   
      
      if (ts && strcmp(name(ts),"gccgtgctc") &&
	  (
	   ((ah->x1 < ah->x2) && xts + 1 == ah->x1) || 
	   ((ah->x1 > ah->x2) && xts == (ah->x1 + 1))   
	   ))
	{ bsAddKey (Gene, _Transpliced_to, ts) ; 
	bsAddData (Gene, _bsRight, _Int, &(ah->a1)) ; 
	}
    }
  messfree (mark) ;
		
  bsSave(Gene) ;
}

/*********************************************************************/

static void saveMatchQuality (KEY cosmid, Array dna, Array dnaR,
			      Array genes, Array hits, Array sets)
{
  static KEY _Read_matchingLength_nerrors = 0 ;
  KEY cDNA_clone, est = 0, gene = 0 ;
  int a1, a2, x1, x2, j1, j2, j3, nmatch, nerr, ngene = 0 ;
  OBJ Gene = 0,Est = 0 ; 
  HIT *gh, *ah ;
  Array err1 = 0, dnaLong = 0, dnaEst = 0 ;
  KEYSET ks = 0 ;

  if (!_Read_matchingLength_nerrors) 
    _Read_matchingLength_nerrors = str2tag("Read_matchingLength_nerrors") ;
  
  if (!_Read_matchingLength_nerrors)
    return ;

  /* code duplicated from above */
  if (!genes || !arrayMax(genes))
    return ;

  cDNASwapA (hits) ;
  arraySort (hits, cDNAOrderByA1) ;
  ks = keySetCreate () ;

  for (ngene = 0 ; ngene < keySetMax(genes) ; ngene++)
    {
      gene = keySet(genes, ngene) ;
      if (!(Gene = bsUpdate(gene)))
	continue ;
      for (j1 = 0 ; j1 < arrayMax(hits) ; j1++)
	{
	  ah = arrp(hits, j1, HIT) ;
	  if (!ah->cDNA_clone || ah->est == 1) 
	    continue ;	
	  gh = cDNA_clone2gene (ah->cDNA_clone, genes, sets) ;
	  if (!gh || gh->gene != gene) 
	    continue ;
	  cDNA_clone = ah->cDNA_clone ;
	  for (j2 = j1 ; j2 < arrayMax(hits); j2++)
	    {
	      ah = arrp(hits, j2, HIT) ;
	      if (ah->cDNA_clone != cDNA_clone)
		continue ;
	      est = ah->est ;
	      if (keySetFind (ks, est, 0))
		continue ;
	      keySetInsert (ks, est) ;
	      nmatch = 0 ;
	      nerr = 0 ;
	      for (j3 = j2 ; j3 < arrayMax(hits); j3++)
		{
		  ah = arrp(hits, j3, HIT) ;
		  if (est != ah->est)
		    continue ;
		  if (ah->x1 <= ah->x2) 
		    { 
		      a1 = ah->a1 - 1 ; a2 = ah->a2 ; 
		      x1 = ah->x1 ; x2 = ah->x2 ; 
		      x1 = ah->x1 - 1 ; x2 = ah->x2 ;
		      dnaLong = dna ; 
		    } 
		  else if (ah->x1 > ah->x2) /* reject length zero */
		    { 
		      a1 = arrayMax(dna) - ah->a2 ; 
		      a2 = arrayMax(dna) - ah->a1 + 1 ; 
		      x1 = ah->x2 - 1 ; x2 = ah->x1 ;
		      dnaLong = dnaR ; 
		    }
		  if ((dnaEst = getSeqDna (est)))
		    {
		      err1 = spliceTrackErrors (dnaEst, x1, &x2, 
						dnaLong, a1, &a2, 0, err1, 2) ;
		      
		      nmatch += a2 - a1 ;
		      nerr += arrayMax(err1) ;
		    }
		  dnaEst = 0 ;  /* do not destroy, got by getSeqDna */
		}
	      
	      bsAddKey (Gene, _Read_matchingLength_nerrors, est) ;
	      bsAddData (Gene, _bsRight, _Int, &nmatch) ;
	      bsAddData (Gene, _bsRight, _Int, &nerr) ;
	      if ((Est = bsUpdate (est)))
		{
		  bsAddKey (Est, _From_gene,gene) ;
		  bsAddData (Est, _bsRight, _Int, &nmatch) ;
		  bsAddData (Est, _bsRight, _Int, &nerr) ;
		  
		  bsSave (Est) ;
		}
	    }
	}
      bsSave(Gene) ;
    }
  keySetDestroy (ks) ;
  arrayDestroy (err1) ;
}

/*********************************************************************/

static void saveTranscripts (KEYSET genes)
{
  int i = arrayExists (genes) ? arrayMax(genes) : 0 ;
  
  while (i--)
    cDNAMakeDna (keySet (genes, i)) ;
}

/*********************************************************************/
/*********************************************************************/

static Associator estAssCreate (KEY gene, KEYSET estSet, BOOL reverse, 
				KEYSET cDNA_clones, KEYSET clipTops, KEYSET clipEnds, 
				int *n1p, int *n2p, int type)
{
  KEYSET ks = 0 ;
  int ii, n1 = 0, n2 = 0 ;
  KEY *kp ;
  Associator ass = 0 ;
  int step = 30 ;
  /* create associator and fill it with the est */
  
  switch (type)
    {
    case 0: case 1: case 2:
      if (reverse)
	ks = query (0, "FIND Sequence cDNA_clone AND NOT Discarded AND (Reverse || 3_prime)") ;
      else
	ks = query (0, "FIND Sequence cDNA_clone  AND NOT Discarded AND NOT (Reverse || 3_prime)") ;
      break ;
    case 3:
      if (estSet)
	{
	  if (reverse)
	    ks = query (estSet, "(Reverse || 3_prime) AND NOT Discarded ") ;
	  else
	    ks = query (estSet, "NOT (Reverse || 3_prime) AND NOT Discarded ") ;
	}
      else
	{
	  if (reverse)
	    ks = queryKey (gene, ">cDNA_clone ; >Read   (Reverse || 3_prime)  AND NOT Discarded ") ;
	  else
	    ks = queryKey (gene, ">cDNA_clone ; >Read   NOT (Reverse || 3_prime) AND NOT Discarded ") ;
	  }
      step = 10 ;
      break ;
    case 1002: case 1001: case 1003:
      if (estSet)
	{
	  if (reverse)
	    ks = query (estSet, "(Reverse || 3_prime )AND NOT Discarded  ") ;
	  else
	    ks = query (estSet, "NOT (Reverse || 3_prime) AND NOT Discarded ") ;
	}
      step = 10 ;
      break ;
    }
  n1 = ii = ks ? keySetMax(ks) : 0 ;
  if (!ii)
    return 0 ;
  ass = assBigCreate (15 * keySetMax(ks)) ; /* room for 10 oligos per est */
  kp = arrp(ks, 0, KEY) -1  ;
  while (kp++, ii--) 
    {
      if (KEYKEY(*kp) >= (1<<22))
	messcrash ("sorry the alignment subroutine is limited to 4M sequences in the database") ;
      n2 += assSequence (*kp, reverse, OLIGO_LENGTH, ass, step, 0, cDNA_clones, clipTops, clipEnds) ;
    }
  *n1p += n1 ; *n2p += n2 ;

  keySetDestroy (ks) ;
  return ass ;
}

/*********************************************************************/

static void getCosmidHits (KEY cosmid, Array hits, 
			   Associator ass, Associator assR, 
			   Array dna, BOOL isUp, KEYSET clipEnds, int nn)
{
  KEY est ;
  void *vp = 0 ; HIT *up ;
  int i, pos, epos, max, a1, a2, x1, x2, eshift ;
  unsigned int oligo, anchor, smask, mask = (1<<(2 * nn)) -1 , keyMask = (1<<22) -1 ;
  char *cp ;

  i = nn - 1 ;
  oligo = 0 ;
  max = arrayMax (dna) ;
  if (max < 4*nn) return ;
  cp = arrp(dna, 0, char) - 1  ;
  while (cp++, i--)
    { oligo <<= 2 ; oligo |= B2[(int)(*cp)] ; } 
  oligo &= mask ;
  pos = -1 ; i = arrayMax(dna) - nn + 1 ;
  if (i > 20) while (pos++, cp++, i--)
    { 
      oligo <<= 2 ; oligo |= B2[(int)(*cp)] ; oligo &= mask ;
      if (oligo && ass && assFind(ass, assVoid(oligo), &vp))
	while (assFindNext(ass, assVoid(oligo), &vp))
	{
	  anchor = assInt(vp) ;
	  est = KEYMAKE (_VSequence, (anchor & keyMask)) ;
	  smask = keySet (clipEnds, KEYKEY(est)) ;
	  smask >>= 10 ; eshift = 0 ;
	  while (smask) { smask >>= 1 ; eshift++ ; }
	  epos = ((anchor >> 22) & 1023) << eshift  ;

	  if (isUp)
	    { a1 = max - pos ; a2 = max - pos - nn + 1 ;}
	  else
	    { a1 = pos + 1 ; a2 = pos + nn ; }
	  x1 = epos + 1 ; x2 = epos + nn ;  
	  up = arrayp (hits, arrayMax(hits), HIT) ;
	  up->est = est ; up->reverse = FALSE ;
	  up->a1 = a1 ; up->a2 = a2 ; up->x1 = x1 ; up->x2 = x2 ;
	}
      if (oligo && assR && assFind(assR, assVoid(oligo), &vp))
	while (assFindNext(assR, assVoid(oligo), &vp))
	{
	  anchor = assInt(vp) ;
	  est = KEYMAKE (_VSequence, (anchor & keyMask)) ;
	  smask = keySet (clipEnds, KEYKEY(est)) ;
	  smask >>= 10 ; eshift = 0 ;
	  while (smask) { smask >>= 1 ; eshift++ ; }
	  epos = ((anchor >> 22) & 1023) << eshift ;

	  if (isUp)
	    { a1 = max - pos ; a2 = max - pos - nn + 1 ;}
	  else
	    { a1 = pos + 1 ; a2 = pos + nn ; }
	  /*
	  maxEst = keySet(clipEnds, KEYKEY(est)) ;
	  x1 = maxEst - epos ; x2 = maxEst - epos - nn + 1 ;  
	  */
	  x1 = epos ; x2 = epos -nn + 1 ;
	  up = arrayp (hits, arrayMax(hits), HIT) ;
	  up->est = est ; up->reverse = TRUE ;
	  up->a1 = a1 ; up->a2 = a2 ; up->x1 = x1 ; up->x2 = x2 ;
	}
     }
}

/*********************************************************************/

static BOOL checkIntron (KEY cosmid, Array dna, Array dnaR, Array dnaEst, HIT *up, HIT *vp)
{
  Array dnaLong = 0 ;
  char *cp, *cq ;
  int i, a1, a2, x1, x2 ;

  a1 = a2 = x1 = x2 = 0 ;
  if (up->x2 + 1 != vp->x1)
    return FALSE ;
  /* check up  exon */
  if (up->a1 < up->a2) 
    { a1 = up->a1 ; a2 = up->a2 ; dnaLong = dna ; } 
  else if (up->a1 > up->a2) /* reject length zero */
    { a1 = arrayMax(dna) - up->a1 + 1 ; a2 = arrayMax(dna) - up->a2 + 1 ; 
      dnaLong = dnaR ; 
    }
  if (!dnaLong)
    return FALSE ;
  cp = arrp (dnaLong, a2 - 1, char) ;
  x1 = up->x1 ; x2 = up->x2 ;
  cq = arrp (dnaEst, x2 - 1, char) ;
  if (a2 < a1 + 8 || x2 < x1 + 8) return FALSE ;
  for (i = 0 ; i < 8 && a1 <= a2 - i && x1 <= x2 - i ; i++, cp--, cq--)
    if (*cp != *cq) /* n prevent sliding so do not say : !(*cp & *cq)) */
      return FALSE ;
  /* check down  exon */
  if (vp->a1 < vp->a2) 
    { a1 = vp->a1 ; a2 = vp->a2 ; dnaLong = dna ; }
  else if (vp->a1 > vp->a2) /* reject length zero */
    { a1 = arrayMax(dna) - vp->a1 + 1 ; a2 = arrayMax(dna) - vp->a2 + 1 ; 
      dnaLong = dnaR ; 
    } 
  if (!dnaLong)
    return FALSE ;
  cp = arrp (dnaLong, a1 - 1, char) ;
  x1 = vp->x1 ; x2 = vp->x2 ;
  cq = arrp (dnaEst, x1 - 1, char) ;
  if (a2 < a1 + 8 || x2 < x1 + 8) return FALSE ;
  for (i = 0 ; i < 8 && a1 + i < a2 && x1 + i < x2 ; i++, cp++, cq++)
    if (*cp != *cq) /* n prevent sliding so do not say : !(*cp & *cq)) */
      return FALSE ;

  return TRUE ;
}


/*********************************************************************/

static BOOL checkExonEnd (KEY cosmid, Array dna, Array dnaR, Array cDNA, HIT *hh)
{
  Array dnaLong = 0;
  Array err1 = 0 ;
  A_ERR *ep1 ;
  int i1, a1 = 0 , a2, x1, x2, dx ;

  /* check hh  exon */
  if (hh->a1 < hh->a2) 
    { a1 = hh->a1 ; a2 = hh->a2 ; dnaLong = dna ; } 
  else if (hh->a1 > hh->a2) /* reject length zero */
    { a1 = arrayMax(dna) - hh->a1 + 1 ; a2 = arrayMax(dna) - hh->a2 + 1 ; 
      dnaLong = dnaR ; 
    }
  if (!dnaLong)
    return FALSE ;

  x1 = hh->x1 ; x2 = hh->x2 ; 
  dx = (x2 - x1)/5 ; x1 += dx ; a1 += dx ; /* jump the frequent init errors */
  err1 = spliceTrackErrors (cDNA, x1, &x2, dnaLong, a1, &a2, 0, err1, 2) ;
  if (arrayMax(err1))
    for (i1 = 0 ; i1 < arrayMax(err1) ; i1++)
      {  
	ep1 = arrp(err1, i1, A_ERR) ;
	if (ep1->iShort >= hh->x2 - 5 ||
	    (i1 < arrayMax(err1) - 2 && ep1->iShort > (ep1+2)->iShort - 6))
	  { a2 = ep1->iLong - 1 ; x2 = ep1->iShort - 1 ;break ; }
      }
  if (dnaR == dnaLong)
    { a1 = arrayMax(dna) - a1 + 1 ; a2 = arrayMax(dna) - a2 + 1 ; }
  hh->a2 = a2 ; hh->x2 = x2 ;
  arrayDestroy (err1) ;
  if (hh->x2 > hh->x1 + 8)
    return TRUE ;
  return FALSE ;
}

/*********************************************************************/
#ifdef JUNK
static BOOL isEstReverse (KEY est)
{
  BOOL r = FALSE ;
  OBJ Est = bsCreate (est) ;
  
  if (Est && bsFindTag (Est, _Reverse))
    r = TRUE ;
  bsDestroy (Est) ;
  return r ;
}
#endif
/*********************************************************************/
#define REV(_g,_t) (((_g)->reverse && (_t) > 20 && (_t) < 23) ? 43 - (_t) : (_t))

static void cleanGeneHits (KEY cosmid, Array geneHits)
{
  int  j0, j1, j2, newj = 0, t1, t2 ;
  HIT *gh1, *gh2 ;
  Array oldHits = 0 ;

  if (!arrayMax(geneHits))
    return ;
  for (j0 = 0 ; j0 < 2 ; j0++)   /* first one round to match 3p and 5p of one clone */
    {
      oldHits = arrayCopy (geneHits) ; newj = 0 ;
      cDNASwapA (oldHits) ;
      arraySort (oldHits, cDNAOrderGenesByA1) ;

      for (j2 = 0 ; j2 < arrayMax (oldHits) ; j2++)
    { 
      gh2 = arrayp (oldHits, j2, HIT) ;
      if (gh2->type == 6)
	goto nextj2 ;
      /* compare and upgrade previous */
      for (j1 = 0 ; j1 < newj ; j1++)
	{ 
	  gh1 = arrayp (geneHits, j1, HIT) ;
	  if (gh1->type == 6)
	    continue ;

	  if (!j0 && gh1->cDNA_clone != gh2->cDNA_clone)
	    continue ;
	  if (gh2->reverse != gh1->reverse) 
	    continue ; /* different strands */
	  if (gh2->a1 > gh1->a2 + 5 || gh2->a2 + 5 < gh1->a1)
	    continue ; /* no quasi intersect */

	  if (gh1->type == 100 || gh2->type == 100) /* ghosts */
	    continue ;
	  if (gh2->type == gh1->type &&
	      gh1->a1 == gh2->a1 && gh1->a2 == gh2->a2)
	    goto nextj2 ;    /* do not double */
	  
	  t1 = REV(gh1, gh1->type) ;
	  t2 = REV(gh2, gh2->type) ;

	  switch (t1)
	    {
	    case 20: /* nothing found */
	      switch (t2)
		{
		case 20: /* nothing found */
		  /* quasi intersect, so keep union */
		  if (gh2->a1 < gh1->a1) gh1->a1 = gh2->a1 ;
		  if (gh2->a2 > gh1->a2) gh1->a2 = gh2->a2 ;
		  goto nextj2 ; 
		case 21: /* end found */
		  if (gh2->a2 + 10 > gh1->a2)
		    { /* promote */
		      gh1->a2 = gh2->a2 ; gh1->type = REV(gh1, 21) ;
		      if (gh2->a1 < gh1->a1) gh1->a1 = gh2->a1 ;
		      goto nextj2 ; 
		    }
		  else
		    { /* extend beginning */ 
		      if (gh2->a1 < gh1->a1) gh1->a1 = gh2->a1 ;  
		      else gh2->a1 = gh1->a1 ;
		      /* but do not goto nextj2 ; */
		    }
		  break ;
		case 22: /* begin found */
		  if (gh2->a1 < gh1->a1 + 10)
		    { /* promote */
		      gh1->a1 = gh2->a1 ; gh1->type = REV(gh1, 22) ;
		      if (gh2->a2 > gh1->a2) gh1->a2 = gh2->a2 ;
		      goto nextj2 ;
		    }
		  else
		    { /* extend extremity */ 
		      if (gh2->a2 > gh1->a2) gh1->a2 = gh2->a2 ;  
		      else gh2->a2 = gh1->a2 ;  
		      /* but do not goto nextj2 ; */
		    }
		  break ;
		case 23: case 24: case 25: case 26:
		  if (gh1->a1 > gh2->a1 - 10 && gh1->a2 < gh2->a2 + 10)
		    {
		      *gh1 = *gh2 ;
		      /* gh1->a1 = gh2->a1 ; gh1->a2 = gh2->a2 ; gh1->type = t2 ;*/
		      goto nextj2 ;
		    }
		  else  if (gh1->a1 <= gh2->a1 - 10 && gh1->a2 < gh2->a2 + 10)
		    { /* extend extremity */
		      gh1->a2 = gh2->a2 ; gh1->type = REV(gh1, 21) ;
		      /* but do not goto nextj2 ; */
		    }
		  else  if (gh1->a1 >  gh2->a1 - 10 && gh1->a2 >= gh2->a2 + 10)
		    { /* extend beginning */
		      gh1->a1 = gh2->a1 ; gh1->type = REV(gh1, 22) ;
		      /* but do not goto nextj2 ; */
		    }
		}
	      break ;
	    case 21: /* end found */
	      switch (t2)
		{
		case 20: /* nothing found */
		  if (gh2->a2 < gh1->a2 + 10)
		    { /* extend begin */
		      if (gh2->a1 < gh1->a1) gh1->a1 = gh2->a1 ;
		      goto nextj2 ; 
		    }
		  else  if (gh1->a1 < gh2->a1)
		    { /* extend beginning */ 
		      gh2->a1 = gh1->a1 ;  
		      /* but do not goto nextj2 ; */
		    }
		  break ;
		case 21: /* end found */
		  if (gh2->a2 == gh1->a2)
		    { /* extend begin */
		      if (gh2->a1 < gh1->a1) gh1->a1 = gh2->a1 ;
		      goto nextj2 ; 
		    }
		  else  if (gh1->a1 < gh2->a1)
		    { /* extend beginning */ 
		      if (gh2->a1 > gh1->a1) gh2->a1 = gh1->a1 ;  
		      else gh1->a1 = gh2->a1 ;  
		      /* but do not goto nextj2 ; */
		    }
		  break ;
		case 22: /* begin found */
		  if (gh2->a1 < gh1->a1 + 10 && gh2->a2 < gh1->a2 + 10)
		    { /* win */
		      gh1->a1 = gh2->a1 ; gh1->type = 23 ; 
		      goto nextj2 ;
		    }
		  else if (gh2->a1 >= gh1->a1 + 10 && gh2->a2 < gh1->a2 + 10)
		    { /* extend extremity */ 
		      gh2->a2 = gh1->a2 ; gh2->type = 23 ;
		      /* but do not goto nextj2 ; */
		    }
		  else if (gh2->a1 < gh1->a1 + 10 && gh2->a2 >= gh1->a2 + 10)
		    { /* extend beginning */ 
		      gh1->a1 = gh2->a1 ;   gh1->type = 23 ;
		      /* but do not goto nextj2 ; */
		    }
		  break ;
		case 23: case 24: case 25: case 26:
		  if (gh1->a1 > gh2->a1 - 10 && gh1->a2 == gh2->a2)
 		    {
		      /* gh1->a1 = gh2->a1 ; gh1->type = t2 ; */
		      *gh1 = *gh2 ;
		      goto nextj2 ;
		    }
		  else  if (gh1->a1 > gh2->a1 - 10 && gh1->a2 != gh2->a2)
		     { /* extend beginning */ 
		      gh1->a1 = gh2->a1 ;   gh1->type = t2 ;
		      /* but do not goto nextj2 ; */
		     }
		}
	      break ;
	    case 22: /* begin found */
	      switch (t2)
		{
		case 20: /* nothing found */
		  if (gh1->a1 < gh2->a1 + 10)
		    { /* extend end */
		      if (gh1->a2 < gh2->a2) gh1->a2 = gh2->a2 ;
		      goto nextj2 ;
		    }
		  else
		    { /* extend extremity */ 
		      if (gh2->a2 < gh1->a2) gh2->a2 = gh1->a2 ;
		      else gh1->a2 = gh2->a2 ;
		      /* but do not goto nextj2 ; */
		    }
		  break ;
		case 21: /* end found */
		  if (gh1->a1 < gh2->a1 + 10 && gh1->a2 < gh2->a2 + 10)
		    { /* win */
		      gh1->a2 = gh2->a2 ; gh1->type = 23 ; 
		      goto nextj2 ;
		    }
		  else if (gh1->a1 >= gh2->a1 + 10 && gh1->a2 < gh2->a2 + 10)
		    {
		      /* extend end */
		      gh1->a2 = gh2->a2 ; gh1->type = 23 ;
		      /* but do not goto nextj2 ; */
		    }
		  else if (gh1->a1 < gh2->a1 + 10 && gh1->a2 >= gh2->a2 + 10)
		    { /* extend beginning */ 
		      gh2->a1 = gh1->a1 ;   gh2->type = 23 ;
		      /* but do not goto nextj2 ; */
		    }
		  break ;
		case 22:/* begin found */
		  if (gh2->a1 == gh1->a1)
		    { /* extend end */
		      if (gh2->a2 > gh1->a2) gh1->a2 = gh2->a2 ;
		      goto nextj2 ;  
		    }
		  else
		    { /* extend end */
		      if (gh2->a2 < gh1->a2) gh2->a2 = gh1->a2 ;
		      else gh1->a2 = gh2->a2 ;
		      /* but do not goto nextj2 ; */
		    }
		  break ;
		case 23: case 24: case 25: case 26:
		  if (gh1->a1 == gh2->a1 && gh1->a2 < gh2->a2 + 10)
		    {
		      *gh1 = *gh2 ;
		      /* gh1->a2 = gh2->a2 ; gh1->type = t2 ;  */
		      goto nextj2 ;
		    }
		  else if (gh1->a1 != gh2->a1 && gh1->a2 < gh2->a2 + 10)
		    {/* extend end */
		      gh1->a2 = gh2->a2 ; gh1->type = 23 ;
		      /* but do not goto nextj2 ; */
		    }
		}
	      break ;
	    case 23: case 24: case 25: case 26: /* gh1 wins */ 
	      switch (t2)
		{
		case 20:  /* nothing found */
		  if (gh1->a1 < gh2->a1 + 10 && gh1->a2 > gh2->a2 - 10)
		    goto nextj2 ;
		  else if (gh1->a1 < gh2->a1 + 10 && gh1->a2 <= gh2->a2 - 10)
		    { /* extend beginning */ 
		      gh2->a1 = gh1->a1 ; gh2->type = REV(gh1, 22) ;
		      /* but do not goto nextj2 ; */
		    }
		  else if (gh1->a1 >= gh2->a1 + 10 && gh1->a2 > gh2->a2 - 10)
		    { /* extend extremity */ 
		      gh2->a2 = gh1->a2 ; gh2->type = REV(gh1, 21) ;
		      /* but do not goto nextj2 ; */
		    }
		  break ;
		case 21: /* end found */
		  if (gh1->a1 < gh2->a1 + 10 && gh1->a2 == gh2->a2)
		    goto nextj2 ; 
		  else if (gh1->a1 < gh2->a1 + 10 && gh1->a2 != gh2->a2)
		    { /* extend beginning */ 
		      gh2->a1 = gh1->a1 ;   gh2->type = 23 ;
		      /* but do not goto nextj2 ; */
		    }
		  break ;
		case 22: /* begin found */
		  if (gh1->a1 == gh2->a1 && gh1->a2 > gh2->a2 - 10)
		    goto nextj2 ; 
		  else if (gh1->a1 != gh2->a1 && gh1->a2 > gh2->a2 - 10)
		    { /* extend extremity */ 
		      gh2->a2 = gh1->a2 ; gh2->type = 23 ;
		      /* but do not goto nextj2 ; */
		    }
		  break ;
		case 23: /* begin and end found */
		case 24: /* begin and end found + TSL */
		case 25: /* begin and end found + PolyA */
		case 26: /* begin and end found + TSL + PolyA */
		  if (t1 != t2) break ; /* do not mix */
		  /* polyA wobble */
		  if ((!gh1->reverse && gh1->a1 != gh2->a1) ||
		      ( gh1->reverse && gh1->a2 != gh2->a2))
		    break ;
		  if ((t1 + t2) & 0x1)
		    break ; /* do not mix a 23-25 and a 24-26 == sl1 found */
		  if (t1 < t2) t1 = t2 ; /* gain polyA */
		  
		  if (gh2->clipEnd && gh1->clipEnd)
		    {
		      if (!gh1->reverse)
			{
			  if (gh1->x2 > gh1->clipEnd - 5 &&
			      gh2->x2 > gh2->clipEnd - 5)
			    { 
			      if (gh1->a2 < gh2->a2 &&
				  gh1->a2 > gh2->a2 - 12)
				{ gh1->a2 = gh2->a2 ; gh1->type = t1 ; goto nextj2 ;}
			      if (gh1->a2 > gh2->a2 &&
				  gh1->a2 < gh2->a2 + 12)
				{ gh1->type = t1 ; goto nextj2 ;}
			    }
			}
		      else
			{
			  if (gh1->x1 > gh1->clipEnd - 5 &&
			      gh2->x1 > gh2->clipEnd - 5)
			    {
			      if (gh1->a2 == gh2->a2 && gh1->a1 > gh2->a1 &&
				  gh1->a1 < gh2->a1 + 12)
				{ gh1->a1 = gh2->a1 ; gh1->type = t1 ; goto nextj2 ;}
			      if (gh1->a2 == gh2->a2 && gh1->a1 < gh2->a1 &&
				  gh1->a1 > gh2->a1 - 12)
				{ gh1->type = t1 ; goto nextj2 ;}
			    }
			}
		    }
		}
	      break ;
	    }
	}
      /* register */
      gh1 = arrayp (geneHits, newj++, HIT) ;
      *gh1 = *gh2  ;
      continue ;
    nextj2:
      gh1 = arrayp (geneHits, newj++, HIT) ;
      *gh1 = *gh2  ;
      gh1->type = 6 ; /* kill gh2 */
      continue ;
    }  /* end j1 loop */
  arrayMax(geneHits) = newj ;
  arrayDestroy (oldHits) ;
    }  /* end j0 loop */
}

/*********************************************************************/
/*********************************************************************/

static void addOneGhostHits (int dnaMax, Array hits, int j1, int j2)
{
  int i ;
  float xf = 0 ; 
  int cloneLength = 0 ;
  HIT *up ;
  KEY upEst = 0, downEst = 0, est = 0, clone = 0 ;
  int a1, a2, b1, b2 ;
  OBJ Clone = 0 ;
  BOOL reverse = FALSE ;

  a1 = a2 = b1 = b2 = 0 ;

  for (i = j1; i < j2 ; i++)
    {
      up = arrp(hits, i, HIT) ;
      
      if (up->x1 < up->x2)
	{ 
	  if (!downEst)
	    { a1 = up->a1 ; a2 = up->a2 ; }
	  downEst = up->est ;
	  if (a1 > up->a1) a1 = up->a1 ;
	  if (a2 < up->a2) a2 = up->a2 ;
	  cloneLength -= up->a2 - up->a1 ;
	  reverse = up->reverse ; 
	}
      else
	{
	  if (!upEst)
	    { b1 = up->a1 ; b2 = up->a2 ; }
	  upEst = up->est ;
	  if (b1 > up->a1) b1 = up->a1 ;
	  if (b2 < up->a2) b2 = up->a2 ;
	  upEst = up->est ;
	  cloneLength -= up->a2 - up->a1 ; /* cumul both in same clone ! */
	  reverse = up->reverse ; /* multiply defined but already rationalised */
	}
    }
  if (upEst && downEst)
    return ;
  if (!upEst && !downEst)
    return ;
  est = upEst ? upEst : downEst ;
  if (!bIndexGetKey (est, _cDNA_clone, &clone) || !(Clone = bsCreate(clone)))
    return ;
  if (bsGetData (Clone, str2tag("PCR_product_size"), _Float, &xf))
    cloneLength += 1000 * xf ;
  bsDestroy (Clone) ;
  cloneLength  -= 450 ; /* resolution limit */
  if (cloneLength < 0) 
    return ;
  
  up = arrayp (hits, arrayMax(hits), HIT) ;
  up->reverse = !reverse ;
  up->est = 1 ; /* ghost */
  up->cDNA_clone = clone ;

  if (downEst) /* create a ghost upEst */
    { up->a1 = a2 + cloneLength ; up->a2 = up->a1 + 50 ;
      up->x1 = 50 ; up->x2 = 1 ;
    }
  else
    { up->a2 = b1 - cloneLength ; up->a1 = up->a2 - 50 ;
      up->x1 = 1 ; up->x2 = 50 ;
    }  
}

/*********************************************************************/

static void addGhostHits (int dnaMax, Array hits)
{
  KEY old, clone ;
  int j, j1, jmax;
  HIT *up ;

  cDNASwapA (hits) ; 
  arraySort (hits, cDNAOrderByA1) ;

  jmax = arrayMax(hits)  ;
  for (old = 0, j = 0, j1 = 0 ; j < jmax ; j++)
    {
      up = arrp(hits, j, HIT) ;
      clone = up->cDNA_clone ;
      if (clone != old)
	{
	  if (j > j1)
	    addOneGhostHits (dnaMax, hits, j1, j) ;
	  j1 = j ;
	}
      old = up->cDNA_clone ;
    }
  if (j > j1)
    addOneGhostHits (dnaMax, hits, j1, j) ; 
  cDNASwapA (hits) ;
  arraySort (hits, cDNAOrderByA1) ;
}

/*********************************************************************/

static Array  getGeneHits (KEY cosmid, Array hits, Array dna, Array dnaR)
{
  Array geneHits = 0 ;
  KEY dummy, est = 0;
  int ii, jj, z1, z2, g1 ;
  HIT *hh, *hh1, *gh, *lastConfirmedhh = 0 ;
  Array dnaEst = 0 ;
  OBJ obj = 0 ;
  BOOL intronFound ;

  cDNASwapX (hits) ;
  arraySort (hits, cDNAOrderByX1) ;
  
  geneHits = arrayCreate (10, HIT) ; g1 = 0 ;
  for (jj = 0 ; jj < arrayMax (hits) ; jj++)
    { 
      hh = arrayp (hits, jj, HIT) ;
      hh1 = jj < arrayMax (hits) - 1 ? hh + 1 : 0 ;
      if (hh->est != est) dnaEst = 0 ;
      est = hh->est ;
      if (!dna || 
	  (!dnaR && (hh->a1 > hh->a2 || (hh+1)->a1 > (hh+1)->a2)))
	messcrash ("no dna in getGeneHits") ;
      /*dna = getSeqDna (cosmid) ;
      if (!dna) continue ;
      if (!dnaR && (hh->a1 > hh->a2 || (hh+1)->a1 > (hh+1)->a2))
	{ dnaR = arrayCopy (dna) ; reverseComplement (dnaR) ; }
	*/
      if (hh->est == 1) /* ghost */
	{
	  gh = arrayp (geneHits, g1++, HIT) ;
	  *gh = *hh ; /* a1 a2 x1 x2 est, reverse etc */
	  gh->reverse = hh->a1 < hh->a2  ? hh->reverse : ! hh->reverse ;
	  gh->type = 100 ;
	  continue ;
	}
      if (!dnaEst) dnaEst = getSeqDna (est) ;
      if (!dnaEst)
	continue ;
      if (!hh1 || hh->est != hh1->est) 
	goto lastExon ;
      intronFound = checkIntron (cosmid, dna,dnaR,dnaEst, hh,hh+1);

      if (intronFound)
	{  /* register intron */
	  if (hh->a2 > hh->a1 )
	    { z1 = hh->a2 + 1 ; z2 =  (hh+1)->a1 - 1 ;}
	  else
	    { z1 = hh->a2 - 1 ; z2 = (hh+1)->a1 + 1 ; }
	  
	  gh = arrayp (geneHits, g1++, HIT) ;
	  
	  *gh = *hh ; /* est, reverse etc */
	  gh->a1 = z1 ; gh->a2 = z2 ;
	  gh->type = 10 ; /* introns */
	  gh->x1 = hh->x2 ;  gh->x2 = hh->x2 + 1 ;
	  gh->reverse = z1 < z2  ? hh->reverse : ! hh->reverse ;
	}
      /* register previous exon */
      gh = arrayp (geneHits, g1++, HIT) ;
      *gh = *hh ; /* a1 a2 x1 x2 est, reverse etc */
      gh->reverse = hh->a1 < hh->a2  ? hh->reverse : ! hh->reverse ;
      if (hh == lastConfirmedhh)  /* Begin_Found */ 
	gh->type = intronFound ? 23 : (hh->reverse ? 21 : 22) ;
      else
	gh->type = intronFound ? (hh->reverse ? 22 : 21) : 20 ;

      obj = bsCreate (hh->est) ;
      if (obj && !hh->reverse &&
	  bsGetKey (obj, _Transpliced_to, &dummy) &&
	  /* strcmp(name(dummy),"gccgtgctc") && */
	  (
	   !strcmp(name(dummy),"SL1") ||
	   !strcmp(name(dummy),"SL2") 
	   ) &&
	  bsGetData (obj, _bsRight, _Int, &ii) &&
	  ii + 1 == hh->x1)
	gh->type =  intronFound ? 24 : 21 ;

      if (obj && hh->reverse &&
	  bsGetData (obj, _PolyA_after_base, _Int, &ii) &&
	  ii == hh->x1)
	{ gh->type =  intronFound ? 25 : 22 ; gh->clipEnd = 1 ; } 
     bsDestroy (obj) ;

      if (intronFound)
	lastConfirmedhh = hh1 ;
      continue ;
      
    lastExon:   /* special case */
      if (hh == lastConfirmedhh || hh->x1 == 1 ||
	  hh->x2 > arrayMax(dnaEst) - 10 || hh->x2 > hh->x1 + 50
	  )
	{
	  gh = arrayp (geneHits, g1++, HIT) ; 
	  *gh = *hh ; /* a1 a2 x1 x2 est, reverse etc */
	  gh->reverse = hh->a1 < hh->a2  ? hh->reverse : ! hh->reverse ;

	  gh->type = hh == lastConfirmedhh ?  /* begin found */
	    (hh->reverse ? 21 : 22) : 20 ;
	  obj = bsCreate (hh->est) ;
	  
	  if (!hh->reverse && obj &&
	       bsGetData (obj, _PolyA_after_base, _Int, &ii) &&
	       ii == hh->x2)
	    { gh->type =  hh == lastConfirmedhh ?  /* begin found */
	      25 : 21 ; gh->clipEnd = 1 ; }

	  if (hh->reverse &&  obj &&
	      bsGetKey (obj, _Transpliced_to, &dummy) &&
	      strcmp(name(dummy),"gccgtgctc") &&
	      bsGetData (obj, _bsRight, _Int, &ii) &&
	       ii == hh->x2 + 1)
	    gh->type =  hh == lastConfirmedhh ?  /* end found */
	      24 : 22 ;

	  bsDestroy (obj) ;
	       
	  if (!checkExonEnd(cosmid, dna, dnaR, dnaEst, gh))
	    arrayMax(geneHits) = --g1 ;
	}
      lastConfirmedhh = 0 ;
    }

  return geneHits ;
}

/*********************************************************************/

static int intersectLength (HIT *gh1, HIT *gh2) 
{
  int x1, x2, y1, y2 ;

  x1 = gh1->a1 ; x2 = gh1->a2 ;
  y1 = gh2->a1 ; y2 = gh2->a2 ;

  if (x1 < y1) x1 = y1 ;
  if (x2 > y2) x2 = y2 ;

  return x2 > x1 ? x2 - x1 : 0 ;
}

static void countAlternatives (Array genes, int jmin, int jmax, int *aep, int *aip)
{
  int dx, j1, j2 ;
  HIT *gh1 , *gh2;
  BOOL found ;


  *aip = *aep = 0 ;
  for (j1 = jmin ; j1 < jmax ; j1++)
    {  
      gh1 = arrayp (genes, j1, HIT) ;
      switch (gh1->type)
	{
	case 1: /* new gene */
	case 2: /* Gap */ 
	  continue ;

	case 10: /* intron */
	case 110: /* alternatif if covered by other intron or long exon */
	  found = FALSE ;
	  for (j2 = jmin ; !found && j2 < jmax ; j2++)
	    {  
	      if (j1 == j2) continue ; 

	      gh2 = arrayp (genes, j2, HIT) ;
	      dx = intersectLength (gh1, gh2) ;

	      switch (gh2->type)
		{
		case 1: /* new gene */
		case 2: /* Gap */ 
		  continue ;
		case 10: /* intron */
		case 110: /* alternatif if covered by other intron or long exon */
		  if (j2 > j1 && dx > 0)  found = TRUE ; 
		  break ;
		case 20: case 21: case 22: case 23: /* exon */
		case 120: case 121: case 122: case 123: /*  alternative exon */  
		  if (dx > 20) found = TRUE ;
		  break ;
		}
	    }
	  if (found) (*aip)++ ;
	  break ;
	case 20: case 21: case 22: /* unconfirmed exon */
	case 120: case 121: case 122: 
	  found = FALSE ;
	  for (j2 = jmin ; !found && j2 < jmax ; j2++)
	    {  
	      if (j1 == j2) continue ; 

	      gh2 = arrayp (genes, j2, HIT) ;
	      dx = intersectLength (gh1, gh2) ;

	      switch (gh2->type)
		{
		case 1: /* new gene */
		case 2: /* Gap */ 
		  continue ;
		case 10: /* intron */ case 23:case 123:
		case 110: /* alternatif if covered by other intron or long exon */ 
		  if (dx > 20) found = TRUE ;
		  break ;
		case 20: case 21: case 22: /* exon */
		case 120: case 121: case 122:  /*  alternative exon */  
		  if (j1 < j2 && dx > 20) found = TRUE ;
		}
	    }
	  if (found) (*aep)++ ;
	  break ;
	case 23: /* exon */ 	case 123: /*  alternative exon */
	  found = FALSE ;
	  for (j2 = jmin ; !found && j2 < jmax ; j2++)
	    {  
	      if (j1 == j2) continue ; 

	      gh2 = arrayp (genes, j2, HIT) ;
	      dx = intersectLength (gh1, gh2) ;

	      switch (gh2->type)
		{
		case 1: /* new gene */
		case 2: /* Gap */ 
		  continue ;
		case 10: /* intron */
		case 110: /* alternatif if covered by other intron or long exon */
		  if (dx > 0)  found = TRUE ;
		  break ;
		case 23: case 123:
		  if (j1 < j2 && dx > 0)  found = TRUE ;
		  break ;
		case 20: case 21: case 22: /* weak exon */
		case 120: case 121: case 122: 
		  if (dx > 20) found = TRUE ;
		}
	    }
	  if (found) (*aep)++ ;
	  break ;
	}
    }     
}

/*********************************************************************/
/* seve confirmed introns boundaries as gt_ag */
static void saveIntronBoundaries (KEY cosmid, Array genes, Array dnaD, Array dnaR)
{
  KEY fKey, gene = 0, _other = 0 , dummy ;
  OBJ Gene = 0 ;
  int n, j1, a1 = 0, a2 = 0 ;
  HIT *gh ;
  char *cp, feet[6] ;
  Array dna = 0 ;
  KEYSET ks = 0 ;

  if (!genes || !arrayMax(genes))
      return ;
  ks = keySetCreate () ;
  _other = str2tag ("Other") ;
  feet[2] = '_' ; feet[5] = 0 ;
  for (j1 = 0 ; j1 < arrayMax(genes) ; j1++)
    {  
      gh = arrayp (genes, j1, HIT) ;
      switch (gh->type)
	{
	case 1:  /* new gene, global coordinates */
	  bsSave (Gene) ;
	  gene = gh->gene ;
	  Gene = bsUpdate (gene) ;
	  if (bsFindTag (Gene, _Intron_boundaries))
	    bsRemove (Gene) ;
	  ks = keySetReCreate (ks) ;
	  a1 = gh->a1 ; a2 = gh->a2 ;
	  if (a1 < a2) dna = dnaD ;
	  else { dna = dnaR ; a1 = arrayMax(dna) - a1 + 1 ; }
	  break ;
	case 5:  /* drop this gene */
	  bsSave (Gene) ;
	  gene = 0 ;
	  break ;
	case 6:
	  break ;
	case 10: /* intron */
	case 110: /* alternative intron */
	  if (!Gene) break ;
	  if (a1 + gh->a1 - 2 < 0 || a1 + gh->a1 + 5 > arrayMax(dna))
	    break ;
	  cp = arrp (dna, a1 + gh->a1 - 2, char) ;
	  feet[0] = dnaDecodeChar [(int)(*cp++)] ;
	  feet[1] = dnaDecodeChar [(int)(*cp)] ;
	  cp = arrp (dna, a1 + gh->a2 - 3, char) ;
	  feet[3] = dnaDecodeChar [(int)(*cp++)] ;
	  feet[4] = dnaDecodeChar [(int)(*cp)] ;
	  fKey = 0 ;
	  if (lexword2key (feet, &fKey, 0))
	    { 
	      n = ++(keySet (ks, fKey)) ;
	      bsAddTag (Gene, fKey) ; 
	      bsAddData (Gene, _bsRight, _Int, &n) ;
	    }
	  else
	    { 
	      n = ++(keySet (ks, 0)) ;
	      bsAddData (Gene, _other, _Text, feet) ;
	      bsAddData (Gene, _bsRight, _Int, &n) ;
	    }
	  bsAddData (Gene, _Splicing, _Int, &(gh->a1)) ;
	  bsAddData (Gene, _bsRight, _Int, &(gh->a2)) ;
	  if (bsPushObj(Gene) && bsGetKeyTags (Gene, _bsRight, &dummy))
	    bsAddData (Gene, _bsRight, _Text, feet) ;
	  bsGoto (Gene, 0) ;
	  break ;
	default:
	  break ;
	}
    }
  bsSave (Gene) ;
  keySetDestroy (ks) ;
}

/*********************************************************************/
/*********************************************************************/
/******* Attribute clones to transcripts *****************************/

static Array cdnaPatternMakePartition (KEY gene, STORE_HANDLE handle)
{
  int n, ii, jj, a1, a2 ;
  HIT *gh=0 ;
  Array segs = arrayCreate (64, HIT) ;
  KEYSET ss = arrayCreate (32000, KEY) ;
  OBJ Gene = 0 ;
  Array aa = arrayHandleCreate (256, BSunit, handle) ;
  KEY tt ;
  BSunit *u ;

  if ((Gene = bsCreate (gene)) &&
       bsGetArray (Gene, str2tag("Derived_sequence"), aa, 5))
    for (ii = 0 ; ii < arrayMax(aa) ; ii += 5)
      {
	u = arrp (aa, ii, BSunit) ;
	tt = u[3].k ;
	if (tt == _Exon || tt == _First_exon || tt == _Last_exon ||
	    tt == _Partial_exon || tt == _Alternative_first_exon ||
	    tt ==  _Alternative_exon || tt == _Alternative_partial_exon ||
	    tt == _3prime_UTR || tt == _5prime_UTR
	    )
	  {
	    a1 = u[1].i ; a2 = u[2].i ;
	    if (tt == _3prime_UTR || tt == _5prime_UTR)
	      n = 2 ; /* because they are connex to exon */
	    else
	      n = 1 ;
	  /* compte les exons  a chaque position */
	    for (jj = a1 ; jj <= a2 ; jj++)
	      keySet (ss, jj) += n ;
	  }
      }

  arrayDestroy (aa) ;
  bsDestroy (Gene) ;

  for (ii = 0, jj= 0, n = 0 ; jj < keySetMax(ss) ; jj++)
    if (keySet(ss,jj) != n)
      {
	if (n)
	  gh->a2 = jj - 1 ;
	n = keySet(ss,jj) ;
	if (n)
	  {
	    gh = arrayp (segs, ii++, HIT) ;
	    gh->gene = gene ;
	    gh->a1 = jj ;
	  }
      }
  if (n)
    gh->a2 = jj - 1 ;  
  keySetDestroy (ss) ;
  return segs ;
}

/*****************************************/

static void cdnaPatternPartitionGenes (KEY gene, Array allBb, KEYSET products, Array segs, 
			    STORE_HANDLE handle, BOOL debug)
{
  OBJ Gene = 0 ;
  BSunit *u ;
  Array aa = arrayHandleCreate (256, BSunit, handle) ;
  int ii, a1, a2, np, ns ;
  KEY old, tt ;
  HIT *gh ;
  BitSet bb = 0 ;

  if ((Gene = bsCreate (gene)) &&
       bsGetArray (Gene, str2tag("Derived_sequence"), aa, 5))
    for (ii = 0, np = 0, ns = 0, old = 0 ; ii < arrayMax(aa) ; ii += 5)
      {
	u = arrp (aa, ii, BSunit) ;
	if (u[0].k != old)
	  {
	    array (allBb, np, BitSet) = bb = bitSetCreate (64,handle) ;
	    bitUnSet (bb, arrayMax(segs)) ; /* make room */
	    keySet (products, np) = u[0].k ;
	    np++ ; /* product number */
	    ns = 0 ;
	    old = u[0].k ;
	    if (debug) 
	      printf("\n%s uses segs: ", name(old)) ;
	  }
	tt = u[3].k ;
	if (tt == _Exon || tt == _First_exon || tt == _Last_exon ||
	    tt == _Partial_exon || tt == _Alternative_first_exon ||
	    tt ==  _Alternative_exon || tt == _Alternative_partial_exon ||
	    tt == _3prime_UTR || tt == _5prime_UTR
	    )
	  {
	    a1 = u[1].i ; a2 = u[2].i ;
	    for (ns = 0 ; ns < arrayMax(segs) ; ns++)
	      {
		gh = arrp (segs, ns, HIT) ;
		if (gh->a1 > a2)
		  break ;
		if (gh->a1 >= a1 && gh->a2 <= a2)
		  {
		    bitSet (bb, ns) ;
		    if (debug) printf (" %d",ns) ;
		  }
	      }
	  }
      } 
  if (debug) printf ("\n") ;
  arrayDestroy (aa) ;
  bsDestroy (Gene) ;
}

/*****************************************/
/* compute hte expected length of the cdna */
static int cdnaPatternLength (int iProduct, HIT *ch, Array allBb , Array segs,
			      int *z1p, int *z2p, int *z3p, int *z4p)
{
  int ii, s1, s2, a1, a2, u1, u2 ; 
  HIT *sh ;
  BitSet bb = array (allBb, iProduct, BitSet) ;
  int nn = 0, n0 = 0 ;

  a1 = ch->a1 < ch->x1 ? ch->a1 : ch->x1 ;   /* begin of clone */
  a2 = ch->a2 < ch->x2 ? ch->x2 : ch->a2 ;   /* end of clone */
  *z1p = *z2p = *z3p = *z4p = -1000000 ;
  for (ii = 0 ; ii < arrayMax(segs) ; ii++)
    {
      if (!bit (bb, ii))      /* segment absent form product */
	continue ;
      sh = arrayp (segs, ii, HIT) ;
      s1 = sh->a1 ; s2 = sh->a2 ;
      
      u1 = a1 > s1 ? a1 : s1 ;
      u2 = a2 < s2 ? a2 : s2 ;  /* u1 u2 is the intersect */
      if (u1 <= u2)
	nn += u2 - u1 + 1 ;

      if (*z1p == -1000000 && ch->a1 <= s2)
	*z1p = n0 + ch->a1 - s1 + 1 ;
      if (ch->a2 >= s1)/* gets the lat segment */
	*z2p = n0 + ch->a2 - s1 + 1 ;   
      if (*z3p == -1000000 && ch->x1 <= s2)
	*z3p = n0 + ch->x1 - s1 + 1 ;
      if (ch->x2 >= s1)
	*z4p = n0 + ch->x2 - s1 + 1 ;
      n0 += s2 - s1 + 1 ;
    }
  if (*z1p == -1000000)
    *z1p = ch->a1 ; /*  a better value ! */
  if (*z2p == -1000000)
    *z2p = ch->a2 ; /*  a better value ! */
  if (*z3p == -1000000)
    *z3p = ch->x1 ; /*  a better value ! */
  if (*z4p == -1000000)
    *z4p = ch->x2 ; /*  a better value ! */
  return nn ;
}

/*****************************************/

static void cdnaPatternExport (int iProduct, KEY product, Array allBb, KEYSET clones, Array ends, Array allNo, Array segs)
{
  int ic, xx ;
  float xo ;
  OBJ Clone = 0, Product = bsUpdate (product) ;
  BitSet no ;
  static KEY _afcc = 0 ;
  KEY clone ;
  int z1, z2, z3, z4 ;
  HIT *ch ;
  
  if (!_afcc)
    _afcc = str2tag("Assembled_from_cDNA_clone") ;
  if (Product)
    {
      if (bsFindTag (Product, _afcc))
	bsRemove (Product) ;
      for (ic = 0 ; ic < keySetMax(clones); ic++)
	{
	  clone = keySet(clones, ic) ;
	  no = array (allNo, ic, BitSet) ;
	  if (!bit(no, iProduct))
	    {
	      ch = arrayp (ends, ic, HIT) ;

		      /* find observed lengths */
	      xo = -2 ;  /* not -1, mind float roundings ! */
	      if ((ch->type & 0x3) == 0x3 && 
		  (Clone = bsCreate (clone)))
		{
		  bsGetData (Clone, str2tag("PCR_product_size"), _Float, &xo) ;
		  bsDestroy (Clone) ;
		}

	      bsAddKey (Product, _afcc, clone) ;

	      if (ch->cDNA_clone == clone && ch->type)
		{
		  xx = cdnaPatternLength (iProduct, ch, allBb, segs, &z1, &z2, &z3, &z4) ;
		  if (ch->type & 0x1)
		    {
		      bsAddData (Product, _bsRight, _Text, "5\' from") ;
		      bsAddData (Product, _bsRight, _Int, &z1) ;
		      bsAddData (Product, _bsRight, _Text, "to") ;
		      bsAddData (Product, _bsRight, _Int, &z2) ;
		    }
		  if (ch->type & 0x2)
		    {
		      bsAddData (Product, _bsRight, _Text, "3\' from") ;
		      bsAddData (Product, _bsRight, _Int, &z3) ;
		      bsAddData (Product, _bsRight, _Text, "to") ;
		      bsAddData (Product, _bsRight, _Int, &z4) ;
		    }
		  if ((ch->type & 0x3) == 0x3)
		    {
		      bsAddData (Product, _bsRight, _Text, "Calculated length") ;
		      bsAddData (Product, _bsRight, _Int, &xx) ;
		      if (xo > -1)
			{
			  bsAddData (Product, _bsRight, _Text, "Observed") ;
			  bsAddData (Product, _bsRight, _Float, &xo) ; 
			  bsAddData (Product, _bsRight, _Text, "kb") ;
			}
		    }		  
		}
	    }
	}
    }
  bsSave (Product) ;
}

/*****************************************/
/** find which cdna goes on which exons **/
/*****************************************/

static void cdnaPatternPartitionClones (KEY gene, int g1, int g2, Array hits, Array allBb, Array allNo,
					KEYSET products, KEYSET clones, Array ends,
					Array  segs, STORE_HANDLE handle, BOOL debug)
{
  KEY old ;
  int i, ii, ic, a1, a2, ns ;
  HIT *gh, *hh, *ch=0 ;
  BitSet  no = 0 ;
  BOOL isExon ;

  for (ii = 0, old = 0 , ic = 0 ; ii < arrayMax(hits) ; ii++)
    {
      hh = arrp (hits, ii, HIT) ;
       if (hh->gene != gene)
	continue ;
      if (hh->cDNA_clone != old)
	{
	  no = array (allNo, ic, BitSet) = bitSetCreate (64, handle) ;
	  keySet (clones, ic) = hh->cDNA_clone ;
	  ch = arrayp (ends, ic, HIT) ;
	  ch->cDNA_clone = hh->cDNA_clone ;
	  ic++ ;
	}
      old = hh->cDNA_clone ;
      isExon = FALSE ;
      switch (hh->type)
	{
	case 20: case 21: case 22: case 23:   /* exons */
	case 120: case 121: case 122: case 123:
	  isExon = TRUE ;
	  /* fall thru */
	case 10: /* intron */
	case 110: /* alternatif if covered by other intron or long exon */
	  if (g1 < g2)
	    { a1 = hh->a1 - g1 + 1 ; a2 = hh->a2 - g1 + 1 ; }
	  else
	    { a1 = g1 - hh->a2 + 1 ; a2 = g1 - hh->a1 + 1 ; }
	  if (isExon)
	    {
	      if ((g2 - g1) * (hh->x2 - hh->x1) > 0)   /* 5prime */
		{
		  if (!(ch->type & 0x1) || a1 < ch->a1) ch->a1 = a1 ;
		  if (!(ch->type & 0x1) || a2 > ch->a2) ch->a2 = a2 ;
		  ch->type |= 0x1 ;
		}
	      else
		{
		  if (!(ch->type & 0x2) || a1 < ch->x1) ch->x1 = a1 ;
		  if (!(ch->type & 0x2) || a2 > ch->x2) ch->x2 = a2 ;
		  ch->type |= 0x2 ;
		}
	    }
	  for (ns = 0 ; ns < arrayMax(segs) ; ns++)
	    {
	      gh = arrp (segs, ns, HIT) ;
	      if (gh->a1 > a2)
		continue ;
	      if (gh->a2 > a1 && gh->a1 < a2)  /* intersect */
		{
		  
		  for (i = 0 ; i < keySetMax(products) ; i++)
		    if (!isExon)
		      { /* i do jump segment ns, so i am not in any product using it */
			if (bit (array(allBb, i, BitSet), ns))
			  {
			    bitSet (no, i) ;
   if (debug) printf ("clone %s has intron %d %d in product %d seg %d (%d %d)\n",
					       name(old), a1, a2, i, ns, gh->a1, gh->a2) ;
			  }
		      }
		    else
		      { /* i do use segment ns, so i am not in any product noy using it */
			if (!bit (array(allBb, i, BitSet), ns))
			  {
			    bitSet (no, i) ;
   if (debug) printf ("clone %s has exon %d %d not  in product %d seg %d (%d %d)\n",
					       name(old), a1, a2, i, ns, gh->a1, gh->a2) ;
			  }
		    }
		}
	    }
	  break ;
	default:
	  continue ;
	  break ;
	}
    }
}

/********************************************************************************/

static void cdnaPatternDo (KEY gene, int g1, int g2, int j1, int j2,
			    Array genes, Array hits) 
{
  STORE_HANDLE handle = handleCreate () ;
  Array  segs ;
  Array allBb = arrayHandleCreate (32, BitSet, handle) ;
  Array allNo = arrayHandleCreate (32, BitSet, handle) ;
  KEYSET 
    ks, 
    products = arrayHandleCreate (32, KEY, handle) ,
    clones = arrayHandleCreate (32, KEY, handle),
    ends = arrayHandleCreate (32, HIT, handle) ;
  int ii ;
  BOOL debug = FALSE ;

  /* create an array of segments forming a partition of all exons */
  segs = cdnaPatternMakePartition (gene, handle) ;
  
  /* create for each product a bitset of segment beeing used */
  cdnaPatternPartitionGenes (gene, allBb, products, segs, handle, debug) ;
  if (debug)
    { showHits (segs) ; showSet (products) ; }
  
  /* for each clone find segments used or jumped by introns
   *  this will tell us which product the cdna does not belong to
   */
  cdnaPatternPartitionClones (gene, g1, g2, hits, allBb, allNo, products, 
			      clones, ends, segs, handle, debug) ;
   
  for (ii = 0; ii < keySetMax (products) ; ii++)
    cdnaPatternExport (ii, keySet (products, ii), allBb, clones, ends, allNo, segs) ;

  ks = query (products, "NOT Assembled_from_cDNA_clone") ;
  keySetKill (ks) ;
  keySetDestroy (ks) ;
  messfree (handle) ;
}

/*****************************************/

static KEY cdnaPatternSaveOne (int *ip, KEY cosmid, Array genes, Array hits) 
{
  int ii, j1 = 0, a1 = 0, a2 = 0 ;
  HIT *gh ;
  KEY gene = 0 ;

  for (ii = *ip ; ii < arrayMax(genes) ; ii++)
    {
      gh = arrayp (genes, ii, HIT) ;
      switch (gh->type)
	{
	case 1:  /* new gene, global coordinates */ 
	  if (gene)
	    {
	      cdnaPatternDo (gene, a1, a2, j1, ii, genes, hits) ;
	      goto done ;
	    }
	  gene = gh->gene ;
	  a1 = gh->a1 ; a2 = gh->a2 ;
	  j1 = ii ;
	  break ;
	default:
	  break ;
	}
    }
  if (gene)
    cdnaPatternDo (gene, a1, a2, j1, ii, genes, hits) ;

done:
  *ip = ii ;
  return ii < arrayMax(genes) ;
}

/*****************************************/

static void savePattern (KEY cosmid, Array genes, Array hits) 
{
  int ii = 0 ;

  if (!genes || !arrayMax(genes))
      return ;

  cDNASwapA (hits) ;
  arraySort (hits, cDNAOrderByA1) ;

  while (cdnaPatternSaveOne (&ii, cosmid, genes, hits)) ;
}

/*********************************************************************/

static void filterGenes (KEY cosmid, Array genes, Array sets, KEYSET parts, KEYSET junctions)
{
  KEYSET ks = 0 ;
  OBJ obj ;
  int x, y, i, j1, n, ga1, ga2 ;
  HIT *gh ;
  KEYSET ks1, ks2, ks3 ;
  BOOL ok ;

  if (!genes || !arrayMax(genes) || !junctions)
      return ;
  
  for (j1 = 0 ; j1 < arrayMax(genes) ; j1++)
    {  
      gh = arrayp (genes, j1, HIT) ;
      switch (gh->type)
	{
	case 1: /* new gene */
	  n = 0 ;
	  ga1 = gh->a1 ; ga2 = gh->a2 ; 
	  ok = FALSE ;
	  for (i=0 ; !ok && i < keySetMax(junctions) ; i++)
	    {
	      /* x,y so that genes completely in overlap are accepted as ok */
	      x = keySet(junctions, i) ;
	      if (i < keySetMax(junctions)-1)
		y = keySet(junctions, i+1) ;
	      else
		y = x ;
	      if ((ga1 < y && ga2 > x) ||
		  (ga1 > x && ga2 < y))
		ok = TRUE ;
	    }
	  if (!ok)
	    { gh->type = 5 ; continue ; } /* drop this gene */

	  /* eliminate matching genes from all parts */

	  ks = array (sets, gh->zone, KEYSET) ;
	  ks1 = query (ks, "Follow From_gene") ;
	  ks2 = query (parts,"Follow Transcribed_gene") ;
	  ks3 = keySetAND (ks1, ks2) ;
	  for (i=0 ; i < keySetMax(ks3) ; i++)
	    if ((obj = bsUpdate(keySet(ks3,i))))
	      bsKill (obj) ; 
	  keySetDestroy (ks1) ;
	  keySetDestroy (ks2) ;
	  keySetDestroy (ks3) ;
	  break ;
	default: 
	  break ;
	}
    }
}

/*********************************************************************/
/*********************************************************************/

static BOOL reviseInitialIntronJump (int jaf,  Array dna, Array dnaR, 
				     Array genes, Array  hits, Array assembledFrom)
{
  HIT *up, *vp, *wp, *zp ;
  int j, j1 ;
  int n, i, a1, a2, x1, x2, a1min ;
  Array dnaLong, dnaEst  ;
  char *cp, *cq ;
  BOOL foundCandidate = FALSE ;

  if (!assembledFrom)
    return FALSE ;
  up = arrp(assembledFrom, jaf, HIT) ;
  for (j = jaf + 1 ; j < arrayMax(assembledFrom) ; j++)
    {
      vp = arrp(assembledFrom, j, HIT) ;
      if (vp->a1 > up->a2 || vp->gene != up->gene)
	break ;
      if (vp->a2 != up->a2)
	continue ;
      for (j1 = j - 1 ; j1 >= 0 ; j1--)
	{ 
	  wp = arrp(assembledFrom, j1, HIT) ;
	  if (wp->est != vp->est) 
	    continue ;
	  if (wp->gene != up->gene)
	    break ;
	  /* now we have found the upstream exon and will break for that est */
	  foundCandidate = TRUE ;
	  a1 = a2 = 0 ;
	  for (i = 0 ; i < arrayMax(hits) ; i++)
	    { 
	      zp = arrp(hits, i, HIT) ;
	      if (zp->est == wp->est && zp->x1 == wp->x1 && zp->x2 == wp->x2)
		{ a1 = zp->a1 ; a2 = zp->a2 ; }
	    }
	  if (a1 == a2)
	    break ;
	  if (a1 < a2) 
	    dnaLong = dna ;
	  else
	    {
	      dnaLong = dnaR ; 
	      a1 = arrayMax(dnaR) - a1 + 1 ;
	      a2 = arrayMax(dnaR) - a2 + 1 ;
	    }
	  x2 = up->x2 - 1 - (vp->a2 - vp->a1) ;
	  dnaEst = getSeqDna (up->est) ;
	  if (dna && dnaEst &&
	      a2 - 5 >= 0 && a2 < arrayMax(dna) &&
	      x2 - 5 >= 0 && x2 < arrayMax(dnaEst))
	    {
	      int n = 0 ;

	      i = 5 ; 
	      i = (x2 < a2) ? x2 : a2 ;
	      cp = arrp (dnaLong, a2, char) ;
	      cq = arrp (dnaEst, x2, char) ;
	      while (cp--, cq--, n++, i--)
		if (!(*cp & *cq & 0xff))
		  break ;
	      if (n > 5)
		{
		  HIT *newp ;
		  /* reviseGenes (up, genes, assembledFrom) ; */
		  for (i = 0 ; i < arrayMax(hits) ; i++) 
		    {
		      wp = arrp (hits, i, HIT) ;
		      if (wp->est == up->est && wp->x2 == up->x2) 
			break ;
		    }
		  /* a2++ ; x2++ ;  */
		  if (x2 - n + 1 < wp->clipTop)
		    n = x2 + 1 - wp->clipTop ;
		  if (n < 3)
		    continue ;
		  newp = arrayp(hits, arrayMax(hits), HIT) ;
		  *newp = *wp ;	
		  if (dnaLong == dnaR)
		    {
		      a2 = arrayMax(dnaR) - a2  + 1 ;
		    }


		  newp->a2 = a2 ; 
		  newp->x2 = x2 ; newp->x1 = newp->x2 - n + 1 ;
		  if (wp->a1 < wp->a2)
		    {
		      newp->a1 = a2 - n + 1 ;
		      wp->a1 += x2  + 1 - wp->x1 ;
		    }
		  else
		    {
		      newp->a1 = a2 + n - 1 ;
		      wp->a1 -= x2 + 1 - wp->x1 ;
		    }
		  wp->x1 = x2 + 1 ;
		  /* now readjust wp->a1, it may be off because of previous errors */
		  if (dnaLong == dnaR)
		    a1 = arrayMax(dnaR) - wp->a1  + 1 ;
		  else
		    a1 = wp->a1 ;		  
		  for (i = -2 ; i < 3 ; i++)
		    {
		      n = 5 ;
		      cp = arrp (dnaLong, a1 + i, char) - 1 ;
		      cq = arrp (dnaEst, wp->x1, char) - 1 ;
		      while (cp++, cq++, n--)
			if (!(*cp & *cq & 0xff))
			  break ;
		      if (n == -1)
			{
			  if (dnaLong == dnaR)
			    wp->a1 -= i ;
			  else
			    wp->a1 += i ;
			  break ;
			}
		    }
		  
		  return TRUE ;
		} 
	    }
	  break ;
	}
    }
  if (0 && foundCandidate)  /* not top exon */
    return FALSE ;
 	
  a1 = a2 = 0 ;
  for (i = 0 ; i < arrayMax(hits) ; i++)
    { 
      zp = arrp(hits, i, HIT) ;
      if (zp->est == up->est && zp->x1 == up->x1 && zp->x2 == up->x2)
	{ a1 = zp->a1 ; a2 = zp->a2 ; }
    }
  if (a1 == a2)
    return FALSE ;
  if (a1 < a2) 
    dnaLong = dna ;
  else
    {
      dnaLong = dnaR ; 
      a1 = arrayMax(dnaR) - a1 + 1 ;
      a2 = arrayMax(dnaR) - a2 + 1 ;
    }
  dnaEst = getSeqDna (up->est) ;
  /* position on last base of this exon */
  cp = arrp (dnaLong, a2, char) ;
  cq = arrp (dnaEst, up->x2, char) ;
  n = 0 ; a1 = a2 + 1 ; i = up->x2 - up->clipTop ; x1 = up->x2 ;
  while (cp--, cq--, n++, x1--, a1-- && (i-- > 4))
    if (!(*cp & *cq & 0xff))
      break ;
  if (n < up->x2 - up->x1 - 12 || n < 8 || i < 5 || a1 == -1)
    return  FALSE ;
  /* now we are on the first error going upward,
   * we now search for a hit higher up 
   */
  a1min = x1 - up->clipTop < 12 ? a1 - 2000 : a1 - 5000 ;
  for (j = a1 - 20 ; j > 10 && j > a1min ; j--)
    {
      int n = 0, b1, b2, y1  ;
      cp = arrp (dnaLong, j, char) + 1 ;
      cq = arrp (dnaEst, x1, char) + 1 ;
      i = x1 - up->clipTop - 1 ;
      if (i > j) i = j ;
      b1 = j + 1 ; b2 = j ; y1 = x1 + 1 ;
      while (cp--, cq--, b1--, y1--, n++, i > 0)
	if (!(*cp & *cq & 0xff))
	  break ;
      if (n > 4 && (n > 8 || ((1 << (2*n)) > 1000* (a1 - j))))
	{
	  HIT *newp ;
	  
	  /* printf ("Success %s\n", name(up->est)) ; */
	  /* b1 <-->  y1   a1 <--> (x1+1), coord including zero */
	  x1++ ; /* to be ok */
	  for (vp = 0, i = 0 ; i < arrayMax(hits) ; i++) 
	    {
	      vp = arrp (hits, i, HIT) ;
	      if (vp->est == up->est && vp->x2 == up->x2)
		break ;
	    }
	  if (!vp) return FALSE ;
	  i = vp->clipTop - y1 - 1 ;
	  if (i > 0)
	    { y1 += i ; b1 += i ; }
	  else if (i < -2 && n < 20) /* short new exon must be fully edited */
	    continue ;
	  if (b2 - b1 < 3)
	    continue ;
	  newp = arrayp(hits, arrayMax(hits), HIT) ;
	  *newp = *vp ;
	  if (dnaLong == dnaR)
	    {
	      a1 = arrayMax(dnaR) - a1  + 0 ;
	      b1 = arrayMax(dnaR) - b1  + 0 ;
	      b2 = arrayMax(dnaR) - b2  + 0 ;
	    }
	  else
	    { b1 ++ ; b2 ++ ; a1 ++ ; }
	  newp->a2 = b2 ; newp->a1 = b1 ;
	  newp->x2 = x1 ; newp->x1 = y1 + 1 ;
	  vp->x1 = x1 + 1 ; 
	  vp->a1 = a1 ;
	  return TRUE ;
	}
    } 
  return  FALSE ;
}

/*********************************************************************/

static BOOL reviseIntronJumps (KEY cosmid, Array dna, Array dnaR, Array genes, 
				Array hits, Array assembledFrom)
{
  HIT *up ;
  int jaf, afMax ; 
  BOOL ok = FALSE ;

  if (!genes || !arrayMax(genes) ||
      !assembledFrom || !arrayMax(assembledFrom))
      return FALSE ;
  
  afMax = arrayMax(assembledFrom) ;
  cDNASwapX (hits) ;
  arraySort (hits, cDNAOrderByX1) ;

  for (jaf = 0 ; jaf < afMax ; jaf ++)
    {
      up = arrp(assembledFrom, jaf, HIT) ;
      if (up->est)
	lexUnsetStatus(up->est, CALCULSTATUS) ;
    }

  for (jaf = 0 ; jaf < afMax ; jaf ++)
    {
      up = arrp(assembledFrom, jaf, HIT) ;
      if (!up->est ||
	  (lexGetStatus (up->est) & CALCULSTATUS))
	continue ;
      lexSetStatus(up->est, CALCULSTATUS) ;
      if (up->x1 < up->x2)
	ok |= reviseInitialIntronJump (jaf, dna, dnaR, genes, hits, assembledFrom) ;
    }

  if (ok)
    {
      cDNASwapA (assembledFrom) ;
      arraySort (assembledFrom, cDNAOrderEstByA1) ;
    }
  return ok ;
}

/*********************************************************************/
/*********************************************************************/

static KEYSET saveGenes (KEY cosmid, Array genes, Array sets)
{
  KEY gene, cgroup, cosmid2 ;
  KEYSET ks = 0, newGenes = 0 ; 
  OBJ Cosmid = 0, Gene = 0, Cgroup = 0 ;
  int j0 = 0, j1, n, a1, a2, ga1, ga2, ll, exl, intl, gapl ;
  int nExons, nIntrons, nAexons, nAintrons ;
  HIT *gh, *gh1 ;
  mytime_t tt ;

  if (!genes || !arrayMax(genes))
      return 0 ;
  
  /* get keyset of cDNA_clones , then smallest alpha one */
  
  Cosmid = bsUpdate(cosmid) ;
  if (!Cosmid)
    return 0 ;
  newGenes = keySetCreate () ;
  for (j1 = 0 ; j1 < arrayMax(genes) ; j1++)
    {  
      gh = arrayp (genes, j1, HIT) ;
      switch (gh->type)
	{
	case 1: /* new gene */
	case 5: /* dropped gene */
	  if (Gene)
	    {
	      bsAddData (Gene, _Covers, _Int, &ll) ;
	      bsAddData (Gene, _bsRight, _Text, "bp from") ;
	      bsAddData (Gene, _bsRight, _Int, &ga1) ;
	      bsAddData (Gene, _bsRight, _Int, &ga2) ;
	      bsAddData (Gene, _Nb_possible_exons, _Int, &nExons) ;
	      countAlternatives (genes, j0, j1, &nAexons, &nAintrons) ;
	      if (nAexons) bsAddData (Gene, _Nb_alternative_exons, _Int, &nAexons) ;
	      bsAddData (Gene, _Nb_confirmed_introns, _Int, &nIntrons) ;
	      if (nAintrons) bsAddData (Gene, _Nb_confirmed_alternative_introns, _Int, &nAintrons) ;
	      if (gapl) bsAddData (Gene, _Total_gap_length, _Int, &gapl) ;
	      /* transpliced_to ? */
	      bsSave(Gene) ; /* save previous */
	    }
	  if (gh->type == 5)
	    break ;
	  n = 0 ;
	  lexaddkey (messprintf("G_%s", name(gh->est)), &gene, _VTranscribed_gene) ;
	  if (class(gh->est) == _VClone_Group)
	    cgroup = gh->est ;
	  else /* gh->est is in fact a clone, so we create a new clone_group */ 
	    lexaddkey (messprintf("G_%s", name(gh->est)), &cgroup, _VClone_Group) ;

	  Cgroup = bsUpdate (cgroup) ;
	  while(TRUE)
	    {
	      cosmid2 = 0 ;
	      if (bsFindKey(Cgroup, _Gene, gene))
		bsGetKey (Cgroup, _bsRight, &cosmid2) ;
	      if (cosmid2)
		{
		  if (cosmid2 != cosmid)
		    lexaddkey (messprintf("G_%s_%d", name(gh->est), ++n), &gene, _VTranscribed_gene) ;
		  else
		    {  /* needed if the gene is divided into 2 new genes */
		      gh1 = arrayp (genes, 0, HIT) - 1 ;
		      while (++gh1 < gh)
			if (gh1->type == 1 && gh1->gene == gene)
			  break ;
		      if (gh == gh1)
			break ;
		      else
			lexaddkey (messprintf("G_%s_%d", name(gh->est), ++n), &gene, _VTranscribed_gene) ;
		    }
		}
	      else
		break ;
	    }
	  bsAddKey (Cgroup, str2tag("Gene"), gene) ;
	  bsAddKey (Cgroup, _bsRight, cosmid) ;
	  bsSave (Cgroup) ;
	  gh->gene = gene ;
	  keySet (newGenes, keySetMax(newGenes)) = gene ;

	  bsAddKey (Cosmid, _Transcribed_gene, gene) ;
	  ga1 = gh->a1 ; ga2 = gh->a2 ; j0 = j1 ;
	  ll = ga2 - ga1 + 1 ; if (ll < 0) ll = - ll ;
	  nExons = nAexons = nIntrons = nAintrons = 0 ;
	  intl = exl = gapl = 0 ;
	  bsAddData (Cosmid, _bsRight, _Int, &ga1) ;
	  bsAddData (Cosmid, _bsRight, _Int, &ga2) ;
	  
	  Gene = bsUpdate (gene) ;
	  if (!Gene)
	    break ;

	  if (bsFindTag (Gene, _Splicing))
	      bsRemove(Gene) ;
	  if (bsFindTag (Gene, _Assembled_from))
	      bsRemove(Gene) ;
	  if (bsFindTag (Gene, _cDNA_clone))
	      bsRemove(Gene) ;
	  if (bsFindTag (Gene, _Structure))
	      bsRemove(Gene) ;
	  tt = timeNow() ;
	  bsAddData (Gene, _Date, _DateType, &tt) ;
	  ks = array (sets, gh->zone, KEYSET) ;
	  if (ks)
	    {
	      int i ;
	      arraySort (ks, keySetAlphaOrder) ;
	      for (i = 0 ; i < keySetMax(ks) ; i++)
		bsAddKey (Gene, _cDNA_clone, keySet (ks, i)) ;
	      keySetSort (ks) ; /* reestablish */
	    }

	  break ;
	case 6:
	  break ;
	default: 
	  if (!Gene || !gh->type || gh->type == 100)
	    break ;
	  a1 = gh->a1 ; a2 = gh->a2 ;
	  bsAddData (Gene, _Splicing, _Int,&a1) ;
	  bsAddData (Gene, _bsRight, _Int, &a2) ;
	  bsPushObj (Gene) ;
	  switch (gh->type)
	    {
	    case 2: /* Gap */
	      gapl += a2 - a1 + 1 ;
	      bsAddKey (Gene, _bsRight, _Gap) ;
	      break ;
	    case 10: /* intron */
	      nIntrons++ ;
	      intl += a2 - a1 + 1 ;
	      bsAddKey (Gene, _bsRight, _Intron) ;
	      break ;
	    case 110: /* alternative intron */
	      nIntrons++ ;
	      bsAddKey (Gene, _bsRight, _Alternative_intron) ;
	      break ;
	    case 20: case 21: case 22: /* exon */
	      exl += a2 - a1 + 1 ;
	      bsAddKey (Gene, _bsRight, _Partial_exon) ;
	      nExons++ ;
	      bsAddData (Gene,_bsRight, _Int, &nExons) ;
	      break ;
	    case 23: /* exon */
	      exl += a2 - a1 + 1 ;
	      if (nExons++)
		{
		  if (j1 + 1 == arrayMax(genes) ||
		      (gh+1)->type == 1 || (gh+1)->type == 5)  
		    bsAddKey (Gene, _bsRight, _Last_exon) ;
		  else
		    bsAddKey (Gene, _bsRight, _Exon) ;
		}
	      else
		bsAddKey (Gene, _bsRight, _First_exon) ;  
	      bsAddData (Gene,_bsRight, _Int, &nExons) ;
	      break ;
	    case 24: case 26:  /* first exon */
	      exl += a2 - a1 + 1 ;
	      bsAddKey (Gene, _bsRight, _First_exon) ;
	      nExons++ ;
	      bsAddData (Gene,_bsRight, _Int, &nExons) ;
	      break ;
	    case 124: case 126: /* first exon */
	      exl += a2 - a1 + 1 ;
	      bsAddKey (Gene, _bsRight, _Alternative_first_exon) ;
	      nExons++ ;
	      bsAddData (Gene,_bsRight, _Int, &nExons) ;
	      break ;
	    case 25:  /* last exon */
	      exl += a2 - a1 + 1 ;
	      bsAddKey (Gene, _bsRight, _Last_exon) ;
	      nExons++ ;
	      bsAddData (Gene,_bsRight, _Int, &nExons) ;
	      break ;
	    case 125: /* last exon */
	      exl += a2 - a1 + 1 ;
	      bsAddKey (Gene, _bsRight, _Last_exon) ;
	      nExons++ ;
	      bsAddData (Gene,_bsRight, _Int, &nExons) ;
	      break ;
	    case 120: case 121: case 122:  /*  alternative exon */
	      nExons++ ;
	      bsAddKey (Gene, _bsRight, _Alternative_partial_exon) ;
	      bsAddData (Gene,_bsRight, _Int, &nExons) ;
	      break ;
	    case 123: /*  alternative exon */
	      nAexons++ ; nExons++ ;
	      bsAddKey (Gene, _bsRight, _Alternative_exon) ;
	      bsAddData (Gene,_bsRight, _Int, &nExons) ;
	      break ;
	    default:
	      break ;
	    }
	  bsGoto (Gene, 0) ;
	}
    }
  if (Gene)
    {
      bsAddData (Gene, _Covers, _Int, &ll) ;
      bsAddData (Gene, _bsRight, _Text, "bp from") ;
      bsAddData (Gene, _bsRight, _Int, &ga1) ;
      bsAddData (Gene, _bsRight, _Int, &ga2) ;
      bsAddData (Gene, _Nb_possible_exons, _Int, &nExons) ;
      countAlternatives (genes, j0, j1, &nAexons, &nAintrons) ;
      bsAddData (Gene, _Nb_alternative_exons, _Int, &nAexons) ;
      bsAddData (Gene, _Nb_confirmed_introns, _Int, &nIntrons) ;
      bsAddData (Gene, _Nb_confirmed_alternative_introns, _Int, &nAintrons) ;
      if (gapl) bsAddData (Gene, _Total_gap_length, _Int, &gapl) ;
      /* transpliced_to ? */
      bsSave(Gene) ; /* save previous */
    }
  
  bsSave (Cosmid) ; 
  keySetSort (newGenes) ;
  keySetCompress (newGenes) ;
  return newGenes ;
}

/*********************************************************************/

static void saveGene2Locus (Array genes)
{
  int i = genes ? arrayMax(genes) : 0 , j ;
  KEY gene, locus ;
  OBJ Gene = 0 ;
  KEYSET loci = 0 ;

  while (i--)
    {
      gene = keySet (genes, i) ;
      loci = queryKey (gene, ">Read_matchingLength_nerrors ; >Locus") ;
      j = keySetMax(loci) ;
      if (j && (Gene = bsUpdate (gene)))
	{
	  while (j--)
	    {
	      locus = keySet (loci, j) ;
	      bsAddKey (Gene, _Gene, locus) ;
	    }
	  bsSave (Gene) ;
	}
      keySetDestroy (loci) ;
    }
}

/*********************************************************************/

static void predictGenes (KEY cosmid, Array hits, Array geneHits, 
			  Array *genesp, Array *setsp)
{
  Array zone = 0, genes = 0, sets = 0  ;
  KEYSET ks2 = 0, ks3 = 0, ks4 = 0, ks5 = 0, bs, bs2 ;
  int i, j, j1, z1, z2, a1, a2, b1, b2, c1, c2, d2 = 0 ;
  int geneBegins, geneEnds, maxHit = arrayMax(hits), newj ;
  HIT *gh, *gh0, *hh, *zh = 0, *zh2, *newg ;
  BOOL reverse = FALSE, estReverse ;

  genes = arrayCreate (10, HIT) ; newj = 0 ;

  if (!arrayMax(geneHits))
      goto abort ;

  cDNASwapA (geneHits) ;
  arraySort (geneHits, cDNAOrderGenesByA1) ;
  
  cDNASwapA (hits) ;
  arraySort (hits, cDNAOrderByA1) ;
  
  /* a zone is a quasi contiguous set of introns/exons
     on a single strand 
  */

  zone = arrayCreate (10, HIT) ; z1 = 0 ;
  for (j1 = 0, zh = 0 ; j1 < arrayMax (geneHits) ; j1++)
    { 
      gh = arrayp (geneHits, j1, HIT) ;
      if (gh->type < 20 || gh->a1 == gh->a2) /* || gh->type == 100)   */
	continue ;

      if (gh->type == 100 && !reverse)
	{     /* if the current zone allready contains a 3prime, forget the ghost */
	  BOOL discard = FALSE ;
	  int jj1 = j1 ;
	  HIT *xh = gh ;

	  while (xh--, jj1--)
	    {
	      if (zh->a1 < xh->a1 && 
		  xh->reverse == reverse &&
		  xh->x1 > xh->x2)
		{ discard = TRUE ; break ; }
	      if (zh->a1 > xh->a1)
		break ;
	    }
	  if (discard)
	    continue ;
	}

      if (zh && 
	  reverse == gh->reverse  && /* same strand */
	  gh->a1 < zh->a2 + 10)
	{ 
	  if (zh->a2 < gh->a2) zh->a2 = gh->a2 ; 
	  if (zh->a1 > gh->a1) zh->a1 = gh->a1 ; 
	  continue ; 
	}
      zh = arrayp (zone, z1++, HIT) ;
      zh->a1 = gh->a1 ; zh->a2 = gh->a2 ;
      reverse = zh->reverse = gh->reverse ;
    }

  sets = arrayCreate (z1, KEYSET) ;

  /* for each zone, find the corresponding cDNA_clones */
  for (z1 = 0 ; z1 < arrayMax (zone) ; z1++)
    {  
      zh = arrayp (zone, z1, HIT) ;
      bs = array (sets, z1, KEYSET) = keySetCreate () ;
      for (j1 = 0 ; j1 < maxHit ; j1++)
	{ 
	  hh = arrayp (hits, j1, HIT) ;
	  estReverse = hh->x1 < hh->x2 ? hh->reverse : !hh->reverse ;
	  if (estReverse != zh->reverse)
	    continue ;
	  if (hh->a1 >= zh->a2 || hh->a2 <= zh->a1)
	    continue ; /* no intersect */
	  keySetInsert (bs, hh->cDNA_clone) ;
	}
      if (!keySetMax(bs))
	{
	  messout("\n\nEmpty zone[%d]:: %s a1=%d a2=%d \n", z1, name(cosmid), zh->a1, zh->a2) ;
	  showHits(hits) ;
	  messout("\n\nEmpty zone[%d]:: geneHits") ;
	  showHits(geneHits) ;
	  sessionClose(1) ;
	  messcrash("Empty zone") ;
	}
    }
  /* for each zone, regroup with similar zones */

  lao: /* recursion is needed in some cases */
  for (z1 = 0 ; z1 < arrayMax (zone) - 1 ; z1++)
    {  
      zh = arrayp (zone, z1, HIT) ;
      bs = array (sets, z1, KEYSET) ;
      if (!bs) continue ;
      for (z2 = z1 + 1 ; z2 < arrayMax (zone) ; z2++)
	{  
	  zh2 = arrayp (zone, z2, HIT) ;
	  bs2 = array (sets, z2, KEYSET) ;
	  if (!bs2) continue ;
	  i = keySetMax (bs2) ;
	  while (i--)
	    if (keySetFind (bs, keySet(bs2,i), 0) ||
		( /*intersect */
		 (zh->reverse == zh2->reverse && 
		  zh->a1 < zh2->a2 && zh->a2 > zh2->a1) 
		 /* &&
		 !( avoid to merge neighboring successive genes 
                    see the good example g_yk2362
		   zh->a1 > zh2->a1 && zh->a2 > zh2->a2 &&
		   
		 )
		 */
		 )) 
	      { /* merge */
		if (zh2->a1 < zh->a1) zh->a1 = zh2->a1 ;
		if (zh2->a2 > zh->a2) zh->a2 = zh2->a2 ;
		j = keySetMax (bs2) ;
		while (j--)
		  keySetInsert (bs, keySet(bs2, j)) ;
		keySetDestroy (bs2) ; 
		array (sets, z2, KEYSET) = 0 ;
		goto lao ;
	      }
	}
    }
	 
  /* compress the zones */

  for (z1 = 0, z2 = 0 ; z1 < arrayMax (zone) ; z1++)
    {  
      zh = arrayp (zone, z1, HIT) ;
      bs = array (sets, z1, KEYSET) ;
      if (!bs)
	continue ;
      if (z1 != z2)
	{
	  zh2 = arrayp (zone, z2, HIT) ;
	  *zh2 = *zh ;
	  array (sets, z2, KEYSET) = bs ;
	}
      z2++ ;
    }
  arrayMax(zone) = arrayMax(sets) = z2 ;

  /* for each zone, create a gene */
  
  /* get keyset of cDNA_clones , then smallest alpha one */
  
  for (z1 = 0 ; z1 < arrayMax (zone) ; z1++)
    {  
      zh = arrayp (zone, z1, HIT) ;
      bs = array (sets, z1, KEYSET) ;
      if (!bs) continue ;
      
      /* get name of smallest Clone Group or smallest clone */
      ks2 = query (bs, "FOLLOW Clone_group") ;
      if (!keySetMax(ks2)) 
	{ keySetDestroy (ks2) ; ks2 = keySetCopy (bs) ; }
      ks3 = query(ks2,"IS yk*") ;
      ks4 = keySetMax(ks3) ? ks3 : ks2 ;
      ks5 = keySetAlphaHeap (ks4, 1) ;
      if (!keySetMax(ks5))
	messcrash ("bad zone") ;
      newg = arrayp (genes, newj++, HIT) ;
      newg->est = keySet(ks5,0) ;
      newg->type = 1 ; newg->zone = z1 ;

      keySetDestroy (ks2) ;  keySetDestroy (ks3) ;  keySetDestroy (ks5) ; 

      if (zh->reverse) { a1 = zh->a2 ; a2 = zh->a1 ; }
      else { a1 = zh->a1 ; a2 = zh->a2 ; }

      newg->a1 = a1 ; newg->a2 = a2 ; newg->zone = z1 ;
      c1 = 1 ; c2 = 0 ;

      j1 = arrayMax (geneHits) ; gh0 = 0 ; geneBegins = geneEnds = 0 ;
      while (j1--)
	{ BOOL alternative  ;

	  gh = zh->reverse ?
	    arrp (geneHits, j1, HIT) :  
	    arrp (geneHits, arrayMax(geneHits) - j1 - 1, HIT);
	  
	  if (gh->est == 1 ||
	      gh->type == 6 ||
	      zh->reverse != gh->reverse ||
	      zh->a1 > gh->a2 || zh->a2 < gh->a1)
	    continue ;
	  
	  if (gh0 && 
	      gh0->a1 == gh->a1 && gh0->a2 == gh->a2 &&
	      gh->type  + gh0->type == 47 &&
	      (
	       (!zh->reverse && gh->a1 == geneBegins) ||
	       (zh->reverse && gh->a2 == geneBegins)
	       )
	      )
	    { gh0->type = 24 ; continue ;} /*  merge */
	       
	  if (gh0 && 
	      gh0->a1 == gh->a1 && gh0->a2 == gh->a2 &&
	      gh->type  + gh0->type == 48 &&
	      (
	       (!zh->reverse && gh->a2 == geneEnds) ||
	       (zh->reverse && gh->a1 == geneEnds)
	       )
	      )
	    { gh0->type = 25 ; continue ;} /*  merge */
	       
	      
	  if (c1 < c2 && c1 < gh->a2 && c2 > gh->a1)
	    alternative = TRUE ;
	  else
	    {
	      alternative = FALSE ;
	      if (c1 > c2) /* first exon */
		{ c1 = gh->a1 ; c2 = gh->a2 ;
		geneBegins = zh->reverse ? c2 : c1 ; 
		geneEnds = zh->reverse ? c1 : c2 ; 
		}
	      if (c1 > gh->a1) c1 = gh->a1 ;
	      if (c2 < gh->a2) c2 = gh->a2 ;
	    }
	  
	  if (zh->reverse) 
	    { b1 = a1 - gh->a2 + 1 ; b2 = a1 - gh->a1 + 1 ; } 
	  else
	    { b1 = gh->a1 - a1 + 1 ; b2 = gh->a2 - a1 + 1 ; }

	  if (!alternative)
	    { 
	      if (newg->type != 1 && d2 + 1 < b1) /* gap */
		{
		    newg = arrayp (genes, newj++, HIT) ;
		    newg->a1 = d2 + 1 ; newg->a2 = b1 - 1 ;
		    newg->type = 2 ; newg->zone = z1 ;
		}
	      d2 = b2 ;
	    }
	  newg = arrayp (genes, newj++, HIT) ;
	  newg->a1 = b1 ; newg->a2 = b2 ;  
	  newg->type = gh->type ; newg->zone = z1 ;
	  if (alternative)
	    newg->type += 100 ;
	  gh0 = gh ;
	}
    }
  *genesp = genes ; *setsp = sets ;
abort: 
  arrayDestroy (zone) ;
}

/*********************************************************************/
/*********************************************************************/

static void exportConfirmedGeneIntrons (KEY gene, 
					BOOL only_alter, 
					BOOL nonSliding, 
					ACEOUT dump_out)
{
  Array units = 0 ;
  int i, ii, jj, x1, x2, z1, z2, a1, a2 ;
  OBJ Cosmid = 0, Gene = 0 ;
  BSunit *up, *vp ;
  Array dna = 0, dnaR = 0 ;
  char *cp, *cq, buffer[60001] ;
  KEY cosmid ;
  int inContext = nonSliding ? 1 : 0 ;

  if (!bIndexGetKey (gene,str2tag("Genomic_sequence"), &cosmid)
      || !(Cosmid = bsCreate(cosmid)) ||
      !bsFindKey (Cosmid, _Transcribed_gene, gene) ||
      !bsGetData (Cosmid, _bsRight, _Int, &a1) ||
      !bsGetData (Cosmid, _bsRight, _Int, &a2) ||
      !(Gene = bsUpdate (gene)))
    goto abort ;

  units = arrayCreate (20, BSunit) ;
  dna = dnaGet (cosmid) ;
  if (!dna) goto abort ;
  bsGetArray (Gene, _Splicing, units, 4) ;
  for (ii = 0 ; ii < arrayMax(units) ; ii+= 4)
    {
      up = arrp(units, ii, BSunit) ;
      if (up[2].k != _Intron &&
	  up[2].k != _Alternative_intron)
	continue ;
      x1 = up[0].i ; x2 = up[1].i ;
      if (only_alter)
	{
	  BOOL isAlter = FALSE ;
	  for (jj = 0 ; jj < arrayMax(units) ; jj+= 3)
	    { 
	      vp = arrp(units, jj, BSunit) ;
	      if (ii != jj && 
		  vp[0].i <= x2 &&  vp[1].i >= x1)
		isAlter = TRUE ;	      
	    }
	  if (!isAlter)
	    continue ;
	}
      if (a1 < a2) { x1 += a1 - 1 ; x2 += a1 - 1 ; }
      else { x1 = a1 - x1 + 1 ; x2 = a1 - x2 + 1 ; }
      if (x1 < 1 || x1 >= arrayMax(dna) ||
	  x2 < 1 || x2 >= arrayMax(dna))
	continue ;
      z1 = x1 ; z2 = x2 ;
      if (!dnaR && z1 > z2)
	{ dnaR = arrayCopy (dna) ; reverseComplement (dnaR) ; }
      
      if (z1 == z2 ||
	  z1 < inContext + 1 ||
	  z2 + inContext >= arrayMax(dna))
	continue ;
      cq = buffer ; i = 0 ;
      if (z1 <= z2)
	{ 
	  if (nonSliding &&
	      ( arr (dna, z1 - 1, char) == arr (dna, z2, char) ||
		arr (dna, z1 - 2, char) == arr (dna, z2 - 1, char))
	      )
	    continue ;
	  
	  cp = arrp(dna, z1 - 1, char) ;
	  while (z1 <= z2 && i < 60000 && z1 < arrayMax(dna) && z1 >= 0)
	    { *cq++ = dnaDecodeChar[(int)(*cp++)] ; z1++ ; i++ ; }
	  if (z1 != z2 + 1 || !i)
	    continue ;
	}
      else
	{ 
	  z1 =  arrayMax(dna) - z1 ; z2 = arrayMax(dna) - z2 ; 
	  if (nonSliding &&
	      ( arr (dna, z1 - 1, char) == arr (dna, z2, char) ||
		arr (dna, z1 - 2, char) == arr (dna, z2 - 1, char))
	      )
	    continue ;
	  
	  cp = arrp(dnaR, z1, char) ;
	  cq = buffer ; i = 0 ;
	  while (z1 <= z2 && i < 60000 && z1 < arrayMax(dna) && z1 >= 0)
	    { *cq++ = dnaDecodeChar[(int)(*cp++)] ; z1++ ; i++ ; }
	  if (z1 != z2 + 1 || !i)
	    continue ;
	}
      *cq = 0 ;
      aceOutPrint (dump_out, "%s", buffer); /* too big for messprintf! */
      aceOutPrint (dump_out, " %s %d %d\n", name(cosmid), x1, x2) ;
    }
  bsSave (Gene) ;
abort:
  bsDestroy (Gene) ;
  bsDestroy (Cosmid) ;
  arrayDestroy (dna) ; arrayDestroy (dnaR) ; 
  arrayDestroy (units) ; 

  return;
} /* exportConfirmedGeneIntrons */

/*********************************************************************/

static void exportConfirmedIntrons (KEYSET ks,
				    BOOL only_alter,
				    BOOL nonSliding,
				    ACEOUT dump_out)
{
  KEY *kp ;
  int ii = 0 ;

  if (!keySetMax(ks))
    messout("No Transcribed_gene in active keyset, sorry") ;
  else
    {
      ii = keySetMax(ks) ;
      messout ("Exporting the%s introns of %d genes",
	       nonSliding ? " nonSliding" : "", keySetMax(ks)) ;
      kp = arrp(ks, 0, KEY) -1  ;
      while (kp++, ii--) 
	exportConfirmedGeneIntrons (*kp, only_alter, nonSliding, dump_out) ;
    }

  return;
} /* exportConfirmedIntrons */

/*********************************************************************/
/*********************************************************************/

static void exportOneOligoExons (KEY gene, ACEOUT dump_out)
{
  KEY clone ;

  if (bIndexGetKey (gene, str2tag("Longest_cDNA_clone"), &clone))
    aceOutPrint (dump_out, "cDNA_clone %s\n",name(clone)) ;

  return;
} /* exportOneOligoExons */

/*********************************************************************/

static void exportOligoExons (ACEOUT dump_out)
{
  KEY *kp ;
  int ii ;
  KEYSET ks = 0 ;
  
  ks = query (0, "Find Transcribed_gene") ;

  if (!keySetMax(ks))
    messout("No Transcribed_gene in active keyset, sorry") ;
  else 
    for (ii = 0, kp = arrp(ks, 0, KEY) ; 
	 ii <  keySetMax(ks) ; kp++, ii++)
      exportOneOligoExons (*kp, dump_out) ;

  keySetDestroy (ks) ;

  return;
} /* exportOligoExons */

/*********************************************************************/
/*********************************************************************/
/*********************************************************************/

static void showHits(Array hits)
{
  int j ;
  HIT *vp ;
  
  if (arrayExists(hits)) 
    for (j = 0  ; j < arrayMax(hits) ; j++)
      {
	vp = arrp(hits, j, HIT) ;
	printf("gene: %s est:%s clone:%s  a1=%d a2=%d   x1=%d x2=%d  rv=%d type=%d\n", 
	       name(vp->gene), name(vp->est), name(vp->cDNA_clone), 
	       vp->a1, vp->a2, vp->x1, vp->x2, vp->reverse, vp->type) ;
      }
}

/*********************************************************************/

static void showSet (KEYSET set)
{
  int j ;
  KEY key ;
  
  if (keySetExists(set)) 
    for (j = 0  ; j < keySetMax(set) ; j++)
      {
	key = arr(set, j, KEY) ;
	printf("%s   ", name(key)) ;
	if (j % 5 == 0 || j == keySetMax(set))
	  printf("\n") ;
      }
}

/*********************************************************************/

static BOOL setPairJunction (KEY junction, KEYSET *partsp, KEYSET *junctionsp)
{
  KEY part, chromo = 0 ;
  int iJunc, ii, x1, x2, y1, y2 ;
  OBJ Part = 0, Junction = 0 ;
  BOOL ok = FALSE, isUp = FALSE;
  KEYSET parts =0, junctions = 0 ;

  /* get the absolute coordinates of the junction */
  Junction = bsCreate(junction) ;
  if (Junction && bsGetKey (Junction, str2tag("IntMap"), &chromo) &&
      bsGetData (Junction, _bsRight, _Int, &x1) &&
      bsGetData (Junction, _bsRight, _Int, &x2))
    ok = TRUE ;
  bsDestroy (Junction) ;
  if (!ok)
    return FALSE ;

  /* need to be in the direction of the pair ! */
  if (x1 <= x2) { isUp = FALSE ; x2 = x2 - x1 + 1 ; }
  else { isUp = TRUE ; x2 = x1 - x2 + 1 ; }

  /* get the absolute coordinates of the members */
  parts = queryKey (junction, "> Parts") ;

  parts = queryKey (junction, "FOLLOW parts ; NOT DNA") ;
  if (keySetMax(parts))
    {
      printf("Error: junction %s contains part %s with no dna", 
	     name(junction), name(keySet(parts,0))) ;
      goto abort ;
    }
  keySetDestroy (parts) ;

  parts = queryKey (junction, "FOLLOW parts ; DNA") ;
  junctions = keySetCreate() ;

  for (iJunc = 0, ii = 0 ; ii < arrayMax(parts) ; ii++)
    {
      part = keySet (parts, ii) ;
      Part = bsCreate(part) ;
      if (Part && bsFindKey (Part, str2tag("IntMap"), chromo) &&
	  bsGetData (Part, _bsRight, _Int, &y1) &&
	  bsGetData (Part, _bsRight, _Int, &y2))
	ok = TRUE ;
      bsDestroy (Part) ;
      if (!ok)
	goto abort ;

      if (!isUp) { y1 = y1 - x1 + 1 ; y2 = y2 - x1 + 1 ; }
      else { y1 = x1 - y1 + 1 ; y2 = x1 - y2 + 1 ; } 

      if (y1 != 1 && y1 != x2)
	keySet(junctions, iJunc++) = y1 ;
      if (y2 != 1 && y2 != x2)
	keySet(junctions, iJunc++) = y2 ;
    }

  keySetSort(junctions) ; keySetCompress(junctions) ;
  if (!keySetMax(junctions))
    goto abort ;
  *partsp = parts ; *junctionsp = junctions ; 

  return TRUE ;

 abort:

  keySetDestroy (parts) ;      
  keySetDestroy (junctions) ;
  return FALSE ;
}

/*********************************************************************/

static KEYSET alignEst2Cosmid (KEY cosmid, KEY gene, KEYSET estSet, int type, int z1, int z2, char *nom)
{
  int n1 = 0, n2 = 0 ;
  Array dna = 0, dnaR = 0, hits = 0, geneHits = 0, plainGeneHits = 0, genes = 0, sets = 0 ;
  KEYSET   newgenes = 0, parts = 0, junctions = 0 ;
  static Associator ass = 0 , assR = 0 ;
  static KEYSET cDNA_clones = 0, clipTops = 0, clipEnds = 0 ;
  BOOL isAlign = type/1000 ;

  if (type == 9999) goto abort ; /* cleanup */
  dna = getSeqDna(cosmid) ;
  if (!dna || arrayMax(dna) < 50) goto abort ;
  dnaR = arrayCopy (dna) ;
  reverseComplement (dnaR) ;

  if (type == 1 && !setPairJunction (cosmid, &parts, &junctions))
    goto abort ;

  hits  = arrayCreate (4096,HIT) ;
  
  if (type == 3 || !clipEnds) 
    {
      n1 = n2 = 0 ;
      cDNA_clones = keySetReCreate (cDNA_clones) ;
      clipTops = keySetReCreate(clipTops) ;
      clipEnds = keySetReCreate (clipEnds) ; 
      assDestroy (ass) ; assDestroy (assR) ;
      ass = estAssCreate (gene,estSet,FALSE, cDNA_clones, clipTops, clipEnds, &n1, &n2, type) ;
      assR = estAssCreate (gene,estSet, TRUE, cDNA_clones, clipTops, clipEnds, &n1, &n2, type) ;
      if (type != 3)
	messout ("Created an associator for %d cDNA, %d oligos\n", n1, n2) ;
    }
  if (!ass && !assR) goto abort ;
  
  n2 = 0 ;
  getCosmidHits (cosmid, hits, ass, assR, dna, FALSE, clipEnds, OLIGO_LENGTH) ;
  getCosmidHits (cosmid, hits, ass, assR, dnaR, TRUE, clipEnds, OLIGO_LENGTH) ;
  getcDNA_clonesAndClips (hits, cDNA_clones, clipTops, clipEnds) ;
  cDNAFilterDiscardedHits (cosmid, hits) ;
  cDNAFilterMultipleHits (cosmid, hits) ;
  if (z1 || z2) dropOutOfZoneHits (hits, z1, z2) ;
  extendMultipleHits (cosmid, hits, dna, dnaR) ;
  dropBadHits (hits, 1) ; /* dropBackwardHits */
  mergeOverlappingHits  (hits) ;
  dropBadHits (hits, 3) ; /* drop non colinear hits */
  getOtherExons  (cosmid, hits, dna, dnaR) ;
  if (z1 || z2) dropOutOfZoneHits (hits, z1, z2) ;
  mergeOverlappingHits  (hits) ; /* the new may overlap the old in rare cases */
  dropBackToBackPairs (hits) ;  /* 3' and 5' turning away from each other */
  fixIntrons (cosmid, hits, dna, dnaR) ;
  dropBadHits (hits, 2) ; /* drop small */
  slideIntrons (cosmid, hits, dna, dnaR) ;
  if (z1 || z2) dropOutOfZoneHits (hits, z1, z2) ;
  dropBadHits (hits, 3) ; /* drop non colinear and isolated distant hits */
  dropBadHits (hits, 4) ; /* bumpHits, no point reading both starnds in error zone */
  dropBadHits (hits, 5) ; /* drop very small, redo after sliding */
  dropDoubleReads  (hits, clipEnds) ; /* drop double reading of 3p 5p */
  cdnaReverseDubiousClone (cosmid, hits,dna,dnaR) ; 
  removeIntronWobbling (cosmid, hits,dna,dnaR) ;
  if (!isAlign) /* cdna case */
    {
      Array af = 0 ;

      addGhostHits (arrayMax(dna), hits) ;
      geneHits = getGeneHits (cosmid, hits,dna,dnaR) ;
      plainGeneHits = arrayCopy (geneHits) ;
      cleanGeneHits (cosmid, geneHits) ;
      predictGenes (cosmid, hits, geneHits, &genes, &sets) ;
      filterGenes (cosmid, genes, sets, parts, junctions) ;
      newgenes = saveGenes (cosmid, genes, sets) ; /* do it here just to prepare gh->gene */
      af = prepareGeneAssembly (cosmid, genes, plainGeneHits, hits, sets) ;
      if (reviseIntronJumps (cosmid, dna, dnaR, genes, hits, af))
	{
	  slideIntrons(cosmid,hits,dna,dnaR) ;
	  arrayDestroy (geneHits) ;
	  geneHits = getGeneHits (cosmid, hits,dna,dnaR) ;
          arrayDestroy (plainGeneHits) ;
	  plainGeneHits = arrayCopy (geneHits) ;
	  cleanGeneHits (cosmid, geneHits) ;
	  arrayDestroy(genes) ; arrayDestroy(sets) ;
	  predictGenes (cosmid, hits, geneHits, &genes, &sets) ;
	  filterGenes (cosmid, genes, sets, parts, junctions) ;
	  keySetDestroy(newgenes) ;
	  newgenes = saveGenes (cosmid, genes, sets) ; /* do it for real genes may have changed */
	  arrayDestroy (af) ;
	  af = prepareGeneAssembly (cosmid, genes, plainGeneHits, hits, sets) ;
	}
      saveGeneAssembly (cosmid, af) ;
      arrayDestroy (af) ;
      saveGene2Locus (newgenes) ;
      saveMatchQuality (cosmid, dna, dnaR, genes, hits, sets) ;
      savePattern (cosmid, genes, plainGeneHits) ; 
      saveTranscripts (newgenes) ;
      saveIntronBoundaries (cosmid, genes, dna,dnaR) ;
      saveHits (cosmid, hits) ;
    }
  else     /* align to reference case  g_yk5503_53 T10B10_H03A11 */
    {
      saveAlignment (cosmid, hits, dna, nom) ;
    }
abort:
  arrayDestroy (dnaR) ; /* dna created by getSeqDna */
  arrayDestroy (genes) ;
  arrayDestroy (geneHits) ;
  arrayDestroy (plainGeneHits) ;
  arrayDestroy (hits) ;
  keySetDestroy (parts) ;      
  keySetDestroy (junctions) ;
  /* keep ass, assR, clipTops, estlenghts around as static */
  if (sets)
    {
      int j1 ;
      for (j1 = 0 ; j1 < arrayMax (sets) ; j1++)
	{
	  KEYSET ks = array (sets, j1, KEYSET) ;
	  if (ks) keySetDestroy (ks) ;
	}
      arrayDestroy (sets) ;
    }
  if (type == 3 || type == 9999)
    {
      keySetDestroy(cDNA_clones) ;
      keySetDestroy(clipTops) ;
      keySetDestroy(clipEnds) ;
      assDestroy (ass) ; assDestroy (assR) ;
    }
  getSeqDna (KEYMAKE (_VCalcul, 12345)) ; /* dna clean Up */

  return newgenes ;
}

/*********************************************************************/

static KEYSET getReads (KEY gene)
{
  KEYSET 
    ks1 = 0, ks2 = 0 ;

  ks1 = queryKey (gene,"Follow cDNA_clone") ;
  ks2 = query (ks1," FOLLOW Read") ;
  
  keySetDestroy (ks1) ;
  return ks2 ;
}

/*********************************************************************/

void autoAnnotateGene (KEY gene, KEY annot)
{
  KEY key ;
  OBJ Gene, Annot ;
  int n ;

  Gene = bsUpdate(gene) ;
  Annot = bsUpdate (annot) ;
  if (!Gene || !Annot)
    goto done ;
  
  n = 0 ;
  if (bsGetKey (Gene, _cDNA_clone, &key)) 
    do { n++; 
    } while (bsGetKey (Gene, _bsDown, &key)) ;
  if (n == 1)
    bsAddTag (Annot,str2tag("Single_clone")) ;
  else if (bsFindTag (Annot,str2tag("Single_clone")))
    bsRemove (Annot) ;		      
	
  /* 5 prime end */
  if (bsGetKey (Gene, _Transpliced_to, &key)) 
    {
      if (!strcmp(name(key), "SL1"))
	bsAddTag (Annot, str2tag("SL1")) ;
      else if (!strcmp(name(key), "SL2"))
	bsAddTag (Annot, str2tag("SL2")) ;
      else if (!strcmp(name(key), "gccgtgctc"))
	/* bsAddTag (Annot, str2tag("gccgtgctc")) */ ;
      else 
	bsAddData (Annot, str2tag("5p_motif"), _Text, name(key)) ;
    }
  /* many clones */
  if (!bsFindTag (Gene, _Total_gap_length))
    {
      if (bsFindTag (Annot, str2tag("Gaps")))
	bsRemove (Annot) ;
      if (bsFindTag (Annot, str2tag("Single_gap")))
	bsRemove (Annot) ;
      bsAddTag (Annot,str2tag("No_Gap")) ;
    }
  else
    {
      if (bsFindTag (Annot, str2tag("No_Gap")))
	bsRemove (Annot) ;
      /* do not add gap, dan does not like it 
	 if (!bsFindTag (Annot, str2tag("Single_gap")))
	 bsAddTag (Annot,str2tag("Gaps")) ;
	 */
    }
  /* 3 prime end */ 
  if (bsFindTag (Gene, str2tag("PolyA_after_base")))
    bsAddTag (Annot,str2tag("Poly_A_seen_in_5p_read")) ;
  /* translation */
  /* genefinder */
done:
  bsSave(Gene) ;
  bsSave (Annot) ;
}

/*********************************************************************/

void cDNARealignGene (KEY gene, int z1, int z2)
{
  KEY cosmid = 0 ;
  int i, old = OLIGO_LENGTH ;
  KEYSET genes = 0, genes2 = 0, genes3 = 0, oldr = 0, newr = 0, error = 0 ;
  OBJ Gene = 0 ;

  if (!sessionGainWriteAccess())
    return ;
  cDNAAlignInit() ;
  OLIGO_LENGTH = 12 ;
  if (bIndexGetKey (gene, _Genomic_sequence , &cosmid))
    {
      oldr = getReads (gene) ;
      genes = alignEst2Cosmid (cosmid, gene, oldr, 3, z1, z2, 0) ;
      newr = query (genes,"Follow cDNA_clone; FOLLOW Read") ;
      error = keySetMINUS (oldr, newr) ;
      if (z1 > z2)
	{
	  if ((Gene = bsUpdate (gene)) && keySetMax (error))
	    {
	      for (i = 0 ; i < keySetMax (error) ; i++)
		bsAddKey (Gene, str2tag ("LostRead"), keySet (error, i)) ;
	      /* messout ("Sorry, I lost %d reads, please club jean", keySetMax (error)) ; */
	    }
	  bsSave (Gene) ;
	}
      else
	if (arrayMax(error))
	  genes2 = alignEst2Cosmid (cosmid, gene, error, 3, 0, 0, 0) ;
      genes3 = keySetOR (genes, genes2) ;
      for (i = 0 ; i < keySetMax(genes3) ; i++)
	if (gene == keySet(genes3, i)) { gene = 0 ; break ; }
      if (gene && (Gene = bsUpdate(gene))) bsKill (Gene) ;
      keySetDestroy (genes) ;
      keySetDestroy (genes2) ;
      keySetDestroy (genes3) ;
    }
  OLIGO_LENGTH = old ;
  keySetDestroy (oldr) ;
  keySetDestroy (error) ;
  keySetDestroy (newr) ;
}

/*********************************************************************/

void cDNARealignGeneKeySet (KEYSET ks)
{
  KEYSET ks1 = ks ? query (ks, "CLASS Transcribed_gene") :
    query (0, "FIND Transcribed_gene") ;
  int i = keySetMax (ks1) ;

  if (!sessionGainWriteAccess())
    return ;
  while (i--)
    cDNARealignGene (keySet(ks1, i), 0, 0) ;
  keySetDestroy (ks1) ;
}

/*********************************************************************/

static void alignKeysetEst2Cosmids (KEYSET ks, int type)
{
  KEY *kp ;
  int ii,  ng, nng ;
  KEYSET genes = 0 ;

  if (!ks || !keySetMax(ks))
    return ;

  if (!sessionGainWriteAccess())
    return ;

  ii = keySetMax(ks) ;
  kp = arrp(ks, 0, KEY) -1  ;
  ng = nng = 0 ;
  printf ("\n//") ;
  while (kp++, ii--) 
    {
      printf (" %s  ",name(*kp)) ;
      if (!ii%5) printf ("\n//") ;
      if ((genes = alignEst2Cosmid (*kp, 0, 0, type, 0, 0, 0)))
	{ ng += keySetMax(genes) ;   nng += keySetMax(genes) ; }
      keySetDestroy (genes) ;
      if (ng > 500)
	{
	  printf ("\nSaving, %d genes %d sequences togo ; %s\n", ng, ii, timeShow(timeNow())) ;
	  fprintf (stderr,"Saving, %d sequences togo ; %s\n", ii, timeShow(timeNow())) ;
	  sessionDoSave (TRUE) ;
	  ng = 0 ;
	}
    } 
  printf ("\n// Constructed a total of %d genes \n", nng) ;
  sessionDoSave (TRUE) ;
}

/*********************************************************************/

static void alignAllEst2Cosmids (int type)
{
  KEYSET ks = 0 ;
  
  switch (type)
    {
    case 0:
      ks = query (0, "Find sequence Genomic") ; 
      break ;
    case 1:
      ks = query (0, "Find sequence Junction") ;
      break ;
    }
  alignKeysetEst2Cosmids (ks, type) ;
  keySetDestroy (ks) ;
}

/*********************************************************************/
/*********************************************************************/

static void alignKeysetRead2KeysetRef (KEYSET ks1, KEYSET ks2, int type, char *nom)
{
  KEY *kp ;
  int ii, ng = 0 ;
  KEYSET genes = 0 ;

  if (!ks1 || !keySetMax(ks1) || !ks2 || !keySetMax(ks2))
    return ;

  if (!sessionGainWriteAccess())
    return ;

  genes = alignEst2Cosmid (0, 0, 0, 9999, 0, 0, 0) ; /* clean up */
  keySetDestroy (genes) ;
  ii = keySetMax(ks2) ;
  kp = arrp(ks2, 0, KEY) -1  ; 
  printf ("\n//") ;
  while (kp++, ii--) 
    {
      printf (" %s  ",name(*kp)) ;
      if (!ii%5) printf ("\n// ") ;
      genes = alignEst2Cosmid (*kp, 0, ks1, type, 0, 0, nom) ;
      if (genes) 
	ng += keySetMax(genes) ;
      keySetDestroy (genes) ;
      if (ng > 500)
	{
	  printf ("\nSaving, %d sequences togo ; %s\n", ii, timeShow(timeNow())) ;
	  fprintf (stderr,"Saving, %d sequences togo ; %s\n", ii, timeShow(timeNow())) ;
	  sessionDoSave (TRUE) ;
	  ng = 0 ;
	}
    } 
  printf ("\n// ") ;    
  genes = alignEst2Cosmid (0, 0, 0, 9999, 0, 0, 0) ; /* clean up */ 
  keySetDestroy (genes) ;
  sessionDoSave (TRUE) ;
}

/*********************************************************************/

static void alignReads2Reference (int type, KEYSET ks01, KEYSET ks02, char *nom)
{
  KEYSET ks1 = 0, ks2 = 0 ;

  ks1 = ks01 ? ks01 : query (0, "Find Sequence Is_read") ; 
  ks2 = ks02 ? ks02 : query (0, "Find sequence Is_Reference && !Assembled_from") ; 

  alignKeysetRead2KeysetRef (ks1, ks2, type, nom) ;
  if (!ks01) keySetDestroy (ks1) ;
  if (!ks02) keySetDestroy (ks2) ;
}

/*********************************************************************/
/*********************************************************************/

static int cDNAOrderToAssigncDNA (void *va, void *vb)
{
  HIT *up = (HIT *)va, *vp = (HIT *)vb ;

  if ((up->gene || vp->gene) && ! (up->gene && vp->gene))  /* better to have a gene */
    return up->gene ? -1 : 1 ;
  if (up->x1 != vp->x1)  /* better to have 2 reads */
     return up->x1 > vp->x1 ? -1 : 1 ;
  if (up->a1 != vp->a1)  /* better to have long match */
    return up->a1 > vp->a1 ? -1 : 1 ;
  if (up->a2 != vp->a2)  /* better to have few errors */
    return up->a1 < vp->a1 ? -1 : 1 ;
  return 
    up->gene - vp->gene ; /* not to be ambiguous */
}

/*********************************************************************/

static void  assigncDNA (KEY key) 
{
  int i, j, icdna, iread, ne , ng, neg ;
  KEY cDNA, gene, est ;
  OBJ Est = 0 , Gene = 0 ;
  Array aa = 0, hits = 0 ;
  KEYSET cdnaKs = 0, reads = 0, genes = 0 ;
  HIT *hh, *h2 ;
  BSunit *u ;

  cdnaKs = key ? queryKey (key, "CLASS cDNA_Clone") : query (0, "FIND  cDNA_clone") ;
  if (!keySetMax(cdnaKs))
    { messout ("No cDNA clone in list, sorry") ; goto abort ; }

  genes = keySetCreate () ;
  ne = neg = ng = 0 ;

  for (icdna = 0 ; icdna < keySetMax(cdnaKs) ; icdna++)
    {
      cDNA = keySet (cdnaKs, icdna) ;
      reads = queryKey (cDNA, "FOLLOW Read") ;
      hits = arrayReCreate (hits, 64, HIT) ;
      /* find all genes hit by all reads of this clone */
      for (iread = 0 ; iread < keySetMax(reads) ; iread++)
	{
	  est = keySet(reads, iread) ;
	  aa = arrayReCreate (aa, 12, BSunit) ;
	  if ((Est = bsCreate (est)))
	    {
	      if (bsGetArray(Est, _From_gene, aa, 3))  /* gene, nmatch, nerr */
		for (i = 0 ; i < arrayMax(aa) ; i += 3)
		  {
		    hh = arrayp(hits, arrayMax(hits), HIT) ;
		    u = arrp (aa, i, BSunit) ;
		    hh->gene = u[0].k ;  hh->a1 = u[1].i ;  hh->a2 = u[2].i ;
		    /* hh->cdnaClone = 0 needed in orderbyA1 further down */
		  }
	      bsDestroy (Est) ;
	    }
	}
      keySetDestroy (reads) ;

      /* accumulate per gene the number of hitting reads and total match */
      for (i = 0 ; i < arrayMax(hits) ; i++)
	{
	  hh = arrp (hits, i, HIT) ;
	  hh->x1 = 1 ;
	  for (j = i + 1; j < arrayMax(hits) ; j++)
	    {
	      h2 = arrp (hits, j, HIT) ;
	      if (hh->gene == h2->gene)
		{ 
		  hh->x1++ ; hh->a1 += h2->a1 ; hh->a2 += h2->a2 ;
		  h2->a1 = h2->a2 = h2->x1 = h2->gene = 0 ;
		}
	    }
	}
      /* do NOT   cDNASwapA (hits) ; a1 and a2 mean hitlength, nerr !! */
      arraySort (hits,cDNAOrderToAssigncDNA) ;  /* relies on clone = 0 */

      for (i = 0 ; i < arrayMax(hits) ; i++)
	{
	  hh = arrp (hits, i, HIT) ;
	  j = 0 ;
	  if (hh->gene)
	    {
	      KEYSET ks = 0 ;
	      /* verify that the values have all been evaluated */
	      ks = queryKey (cDNA, 
			     messprintf(">read ; From_gene = %s AND NEXT AND NEXT",
					name(hh->gene))) ;
	      j = keySetMax(ks) ;
	      keySetDestroy (ks) ;
	    }
		
	  if (!j)
	    continue ;

	  for (j = i + 1 ; j < arrayMax(hits) ; j++)
	    {
	      h2 = arrp (hits, j, HIT) ;
	      if (!h2->gene)
		continue ;
	      if (hh->x1 > h2->x1 ||       /* not both reads match gene 2 */
		  hh->a1 > h2->a1 + 100 || /* the match is much shorter */
		  (    /* many more errors */
		   (hh->a2 < 20 && hh->a2 + 10 < h2->a2)  ||
		   (hh->a2 > 20 && hh->a2 * 3 < h2->a2 * 2)
		   )
		  )
		{
		  KEYSET ks = 0 ;
		  
		  
		  gene = h2->gene ;
		  /* verify that the values have all been evaluated */
		  ks = queryKey (cDNA, 
				 messprintf(">read ; From_gene = %s AND NEXT AND NEXT",
					    name(gene))) ;
		  if (keySetMax(ks) &&
		      (Gene = bsUpdate(gene)))
		    {
		      if (bsFindKey (Gene, _cDNA_clone, cDNA))
			{
			  ne++ ;
			  keySet (genes, ng++) = gene ;
			  bsRemove (Gene) ;
			}
		      bsSave (Gene) ;
		    }
		  keySetDestroy (ks) ;
		}
	    }
	}
    }
  
  keySetSort (genes) ;
  keySetCompress (genes) ;

  messout ("Analysed %d cDNA, eliminated %d clones from %d genes",
	   keySetMax(cdnaKs), ne, keySetMax(genes)) ;
  /* to avoid naming problems, first kill */
  i = keySetMax (genes) ; neg = 0 ;
  while (i--)
    {
      gene = keySet (genes, i) ;
      reads = getReads (gene) ;
      if (!keySetMax(reads))
	{
	  if ((Gene = bsUpdate (gene)))
	    { bsKill (Gene) ; neg++ ; }
	  keySet (genes, i) = 0 ;
	}
      keySetDestroy (reads) ;      
    }
  /* then recalculate, this will reuse small numbers */
  i = keySetMax (genes) ; neg = 0 ;
  while (i--)
    {
      gene = keySet (genes, i) ;
      if (gene)
	{
	  reads = getReads (gene) ;
	  if (keySetMax(reads))
	    cDNARealignGene (gene, 0, 0) ;
	  keySetDestroy (reads) ;
	}
    }

  messout ("Analysed %d cDNA, eliminated %d clones from %d genes, eliminated %d genes",
	   keySetMax(cdnaKs), ne, keySetMax(genes), neg) ;
abort:
  keySetDestroy (genes) ;
  keySetDestroy (cdnaKs) ;
  arrayDestroy (aa) ;
  arrayDestroy (hits) ;
}

static void  assigncDNAKeySet (KEYSET ks)
{
  KEYSET ks1 = query (ks, "CLASS Transcribed_gene") ;
  int i = keySetMax (ks1) ;

  if (!sessionGainWriteAccess())
    return ;
  while (i--)
    assigncDNA (keySet(ks1, i)) ;
  keySetDestroy (ks1) ;
}

/*********************************************************************/
/*********************************************************************/

void fMapcDNADoSelect (ACEIN command_in, ACEOUT result_out,
		       KEY k, KEYSET ks, KEYSET taceKs)
{
  KEY key ;
  char *cp = 0, *cq = 0 ;
  int type = k/10 ;
  KEYSET genes = 0 ;

  cDNAAlignInit () ;

  switch (k)
    {
    case 1: case 11:		/* "align est on all cosmids" */
      alignAllEst2Cosmids(type) ;
      break ;
    case 2: case 12:		/* "align est on active set" */
      if (!keySetExists(ks))
	{ messout ("no active sequence set, sorry") ; break ; }
      alignKeysetEst2Cosmids(ks, type) ;
      break ;
    case 3: case 13:		/* "align est on one cosmid" */
      {
	ACEIN name_in;
	
	if ((type == 0 && !(name_in = messPrompt ("Please enter a cosmid name:","ZK637","w", 0))) ||
	    (type == 1 && !(name_in = messPrompt ("Please enter a link name:","K06H7_C14B9","w", 0))))
	  break ;
	cp = aceInWord (name_in) ;
	if (!lexword2key(cp, &key, _VSequence))
	  { 
	    messout ("Unknown Sequence, sorry") ;
	    aceInDestroy (name_in);
	    break ;
	  }
	genes = alignEst2Cosmid(key, 0, 0, type, 0, 0, 0) ;
	keySetDestroy (genes) ;
	aceInDestroy (name_in);
      }
      break ;
    case 31: 
      assigncDNA (0) ;
      break ;      
    case 32:  
      if (!keySetExists(taceKs))
	{ messout ("no active cDNA set, sorry") ; break ; }
      assigncDNAKeySet (taceKs) ;
      break ;
    case 33:
      cp = aceInWord(command_in) ;
      if (!lexword2key(cp,&key,_VcDNA_clone))
	{ messout ("Unknown cDNA_Clone, sorry") ; break ; }
      assigncDNA (key) ;
      break ;      
    case 5:			/* "export confirmed introns" */
      {
	BOOL only_alter = FALSE ;
	BOOL nonSliding = FALSE ;
	BOOL isError = FALSE ;
	ACEOUT dump_out;

	dump_out = aceOutCopy (result_out, 0);

	while ((cp = aceInWord(command_in)) && *cp == '-')
	  {
	    if (strcmp("-nonSliding",cp) == 0)
	      nonSliding = TRUE ;	     
	    else if (strcmp("-f",cp) == 0
		     && (cp = aceInPath(command_in)))
	      {
		ACEOUT fo;
		if ((fo = aceOutCreateToFile (cp, "w", 0)))
		  {
		    aceOutDestroy (dump_out);
		    dump_out = fo;
		  }
		else
		  {
		    isError = TRUE ;
		    messout ("Sorry, i cannot open %s", cp) ;
		  }
	      }
	    else if (strcmp("-alter",cp) == 0)
	      only_alter = TRUE ;
	    else
	      {
		isError = TRUE ;
		messout ("Usage [-f file] [-nonSliding] [-alter]") ;
	      }
	      
	  }

	if (!isError)
	  exportConfirmedIntrons (taceKs, only_alter, nonSliding, dump_out);

	aceOutDestroy (dump_out);
      }
      break ;
    case 6:
      {
	ACEOUT dump_out;

	dump_out = aceOutCopy (result_out, 0);

	while ((cp = aceInWord(command_in)) && *cp == '-')
	  {
	    if (strcmp("-f",cp) == 0
		&& (cp = aceInPath(command_in)))
	      { 
		ACEOUT fo;
		if ((fo = aceOutCreateToFile (cp, "w", 0)))
		  {
		    aceOutDestroy (dump_out);
		    dump_out = fo;
		  }
	      }
	  }
	exportOligoExons (dump_out) ;

	aceOutDestroy (dump_out);
      }
      break ;
    case 71: 
      cDNARealignGeneKeySet (0) ;
      break ;      
    case 73: 
       if (!keySetExists(taceKs))
	 { messout ("no active transcribed_gene set, sorry") ; break ; }
      cDNARealignGeneKeySet (taceKs) ;
      break ;      
    case 1002:  
      if (!keySetExists(ks))
	{ messout ("no active sequence set, sorry") ; break ; }
      /* fall through */
    case 1001:  
      cp = aceInWord(command_in); 
      cq = cp && *cp ? strnew(cp, 0) : 0 ;
      alignReads2Reference(k, ks, 0, cq) ;
      messfree (cq) ;
      break ;
    case 1003:  break ;
      {
	ACEIN name_in;
	
	if ((type == 0 && !(name_in = messPrompt ("Please enter a cosmid name:","ZK637","w", 0))) ||
	    (type == 1 && !(name_in = messPrompt ("Please enter a link name:","K06H7_C14B9","w", 0))))
	  break ;
	cp = aceInWord(name_in);
	if (!lexword2key(cp, &key, _VSequence))
	  { 
	    messout ("Unknown Sequence, sorry") ;
	    aceInDestroy (name_in);
	    break ;
	  }
	alignReads2Reference(k, ks, 0, cp) ;
	aceInDestroy (name_in);
      }
      break ;
    }

  return;
} /* fMapcDNADoSelect */

/*********************************************************************/
/*********************************************************************/


/* cosmides danielle:  zk637, r05d3 c41g7 c05d11 f22b5  */

/*********************************************************************/
/* this code worked well to give a single trace */
#ifdef JUNK
static Array cDNAGetHitDna (KEY est, Array dna, Array dnaR, Array hits, int *fromp)
{
  int i, j1, j2 = 0, sens ;
  char *cp, *cq ;
  HIT *hh ;
  Array new = arrayCreate (1024, char) ;

  cDNASwapX (hits) ;
  arraySort (hits, cDNAOrderByX1) ;
  for (j1 = 0; j1 < arrayMax (hits); j1 ++)
    {
      hh = arrp(hits, j1, HIT) ;

      if (hh->est != est)
	continue ;
      sens = hh->a1 < hh->a2 ? 1 : -1 ;
      if (!j2)
	{  /* complete the dna with wahtever is there */
	  if (sens == 1) hh->a1 -= hh->x1 - 1 ;
	  else hh->a1 += hh->x1 - 1 ;
	  if (hh->a1 < 1)
	    {
	      i = 1 - hh->a1 ;
	      hh->a1 = 1 ;
	      array (new, j2 + i, char) = 0 ; /* open hh */
	      cq = arrp(new, j2, char) ; j2 += i ;
	      while (i--) *cq++ = N_ ;
	    }
	  else if (hh->a1  > arrayMax(dna))
	    {
	      i = hh->a1 - arrayMax(dna) ;
	      hh->a1 = arrayMax(dna) ;
	      array (new, j2 + i, char) = 0 ; /* open hh */
	      cq = arrp(new, j2, char) ; j2 += i ;
	      while (i--) *cq++ = N_ ;
	    }
	}
      if (fromp && sens == 1 && *fromp >= hh->a1 - 1 && *fromp <= hh->a2)
	*fromp = j2 + *fromp - hh->a1 + 1 ;
      if (fromp && sens == 11 && *fromp >= hh->a2 - 1 && *fromp <= hh->a1)
	*fromp = j2 - *fromp + hh->a1 - 1 ;
      if (sens == 1)
	{ 
	  i = hh->a2 - hh->a1 + 1 ;
	  cp = arrp (dna, hh->a1 -1, char) ;
	  array (new, j2 + i, char) = 0 ; /* open hh */
	  cq = arrp(new, j2, char) ; j2 += i ;
	  while (i--) *cq++ = *cp++ ;
	}
      else if (sens == -1)
	{ 
	  i = hh->a1 - hh->a2 + 1 ;
	  cp = arrp (dna, hh->a1 - 1, char) ;
	  array (new, j2 + i, char) = 0 ; /* open hh */
	  cq = arrp(new, j2, char) ; j2 += i ;
	  while (i--) *cq++ = complementBase [(int)(*cp--)] ;
	}
    }
  if (!j2) arrayDestroy (new) ;
  return new ;
}
#endif
/*********************************************************************/
/*********************************************************************/
/* now for multitraces */

/*********************************************************************/

Array cDNAGetReferenceHits (KEY gene, int origin)
{
  Array units = arrayCreate (60, BSunit) ;
  Array hits = arrayCreate (12, HIT) ;
  OBJ Gene = 0 ;
  int j1, j2 ;
  BSunit *up ; HIT *hh ;

  cDNAAlignInit () ;
  if (gene &&
      (Gene = bsCreate (gene)) &&
      bsGetArray (Gene, _Assembled_from, units, 5))
    for (j1 = j2 = 0 ; j1 < arrayMax(units) - 4 ; j1 += 5)
      {
	up = arrp(units, j1, BSunit) ;
	hh = arrayp (hits, j2++, HIT) ;
	hh->est = up[2].k ;
	hh->a1 = up[0].i + origin ; hh->a2 = up[1].i + origin ; 
	hh->x1 = up[3].i ; hh->x2 = up[4].i ;
      }
  bsDestroy (Gene) ;
  arrayDestroy (units) ;
  if (!arrayMax(hits)) 
    arrayDestroy (hits) ;
  else
    {
      cDNASwapA (hits) ;
      arraySort (hits, cDNAOrderByA1) ;
    }
  return hits ;
}

/*********************************************************************/
/*********************************************************************/
