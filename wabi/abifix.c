/*  File: abifix.c
 *  Author: Jean Thierry-Mieg (mieg@crick.wustl.edu)
 *  Copyright (C) J Thierry-Mieg and R Durbin, 1994
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
 * Exported functions:
 * HISTORY:
 * Last edited: Nov 15 14:41 1999 (fw)
 * Created: Mon Dec  5 10:25:50 1994 (mieg)
 *-------------------------------------------------------------------
 */

/* @(#)abifix.c	1.11 11/12/96 */

#include "acedb.h"
#include "call.h"
#include "bs.h"
#include "dna.h"
#include "whooks/systags.h"
#include "whooks/sysclass.h"
#include "whooks/classes.h"
#include "whooks/tags.h"
#include "dnaalign.h" 
#include "query.h"
#include "pick.h"
#include "dbpath.h"

#include "display.h"

#include "acembly.h"
#include "basecall.h"

/********************************************************************/

static int abiFixDoubleReads (KEY contig, int *ip)
{ OBJ Contig, obj ;
  Array dna = 0, consensus, consensusReverse, consensusDirect = 0, aa = 0, errArray = 0 ;
  int amax, i, j, nn, x, x1, x11, x2, x3, a1, a11, a2, a3 , oldx2 ;
  int vClipTop, vClipEnd, nmv = 0 ;
  KEY key, dnaKey, consensusKey ;
  A_ERR *ep ;
  BOOL isUp ;

  Contig = bsUpdate (contig) ;
  if (!Contig) 
    return 0 ;
  if (!bsFindTag (Contig, _Assembled_from) ||
      !bsGetKey (Contig, _DNA, &consensusKey) ||
      !(consensus = dnaGet(consensusKey)))
    { bsDestroy (Contig) ;
      return 0 ;
    }
  consensusDirect = consensus ;
  consensusReverse = arrayCopy (consensus) ;
  reverseComplement (consensusReverse) ;
  amax = arrayMax (consensus) ;

  aa = arrayCreate(3000, BSunit) ;
  bsFindTag (Contig, _Assembled_from) ;
  bsFlatten (Contig, 5, aa) ;

  for (i = 0 ; i < arrayMax(aa) ; i += 5)
    { key = arr (aa, i, BSunit).k ;
      a1 = arr (aa, i + 1, BSunit).i ;
      a2 = arr (aa, i + 2, BSunit).i ;
      x1 = arr (aa, i + 3, BSunit).i ;
      x2 = arr (aa, i + 4, BSunit).i ;

      isUp = a1 < a2 ? FALSE : TRUE ;
      if (isUp)
	{ a1 = amax - a1 - 1 ;
	  a2 = amax - a2 - 1 ;
	  consensus = consensusReverse ;
	}
      else
	consensus = consensusDirect ;
      obj = bsUpdate(key) ;
      if (!obj) continue ;
      
      if (!bsGetKey (obj, _DNA, &dnaKey) ||
	  bsFindTag (obj, _Assembled_from) ||
	  !(dna = dnaGet(dnaKey)))
	goto abort ;


      if (!bsFindTag (obj, _Old_Clipping))
	{ bsAddData (obj, _Old_Clipping, _Int, &x1) ;
	  bsAddData (obj, _bsRight, _Int, &x2) ;
	}
	
      oldx2 = x2 ;
      vClipTop = -1 ;
      if (bsGetData (obj, _Vector_Clipping, _Int, &vClipTop))
	{ vClipTop-- ;
	  bsGetData (obj, _bsRight, _Int, &vClipEnd) ;
	}

      a1-- ; x1-- ; /* no ZERO */
      errArray = 0 ;
/***************** adjust clipTop ********************/
      if (x1 < 35)
	{ x11 = x1 + 50 ;
	  a11 = a1 + 50 ;
	  if (x11 > x2) x11 = x2 ;
	  if (a11 > a2) a11 = a2 ;
	  errArray = dnaAlignCptErreur (consensus, dna, &a1, &a11, &x1, &x11) ;

	  j = arrayMax(errArray) ; 
	  if (j)
	    ep = arrp(errArray, 0, A_ERR) ;

	  if (!j || ep->iShort > x1 + 3)
	    goto topOk ;
	  if (j > 1) /* if j == 1 ep -> newClipEnd */
	    { nn = 0 ;
	      x = x1 ; j-- ;
	      while (nn++, j-- &&
		     (ep->iShort - x1 < 3 * nn) &&
		     (ep + 1)->iShort < ep->iShort + 4) ep++ ;
	    }
	  
	  switch (ep->type)
	    {
	    case TROU: 
	      x1 = ep->iShort ;
	      a1 = ep->iLong + 1 ;
	      break ;
	    case TROU_DOUBLE: 
	      x1 = ep->iShort ;
	      a1 = ep->iLong + 2 ;
	      break ;
	    case INSERTION : /*_AVANT: */
	      x1 = ep->iShort + 1 ;
	      a1 = ep->iLong ;
	      break ;
	    case INSERTION_DOUBLE: /* _AVANT: */
	      x1 = ep->iShort + 2 ;
	      a1 = ep->iLong ;
	      break ;
	    default:
	      x1 = ep->iShort + 1 ;
	      a1 = ep->iLong + 1 ;
	      break ;
	    }
	}
    topOk:
      arrayDestroy (errArray) ;
      /***************** adjust clipEnd ********************/

      x3 = arrayMax(dna) ;
      if (vClipTop != -1 &&
	  x3 > vClipEnd)
	x3 = vClipEnd ;

      a3 = a2 + x3 - x2 ;
      if (a3 > amax) ;
      { x3 = x3 - (a3 - amax) ;
	a3 = amax ;
      }
      a3-- ; x3-- ;
      errArray = dnaAlignCptErreur (consensus, dna, &a1, &a3, &x1, &x3) ;
      a3++ ; x3++ ;
      
      ep = arrp(errArray, 0, A_ERR)  - 1 ;
      j = arrayMax(errArray) ; nn = 0 ;
      
      while (ep++, j--)
	{ x = ep->iShort ;
	  
	  if (x < x2 - 50) continue ;
	  nn++ ;
	  if (x > x2 && 
	      nn * 20 > x - x2 - 50)   /* % d'erreur = 1/20 */
	    { x3 = x ; a3 = ep->iLong ; break ; }
	}
      /* now remove all errors in last 10 bases */
      ep = arrp(errArray, 0, A_ERR)  - 1 ;
      j = arrayMax(errArray) ;
      while (ep++, j--)
	{ x = ep->iShort ;
	  if (x > x3 - 10) 
	    { x3 = x ; 
	      a3 = ep->iLong ;
	      break ; 
	    }
	}
      if (vClipTop != -1 &&
	  x3 > vClipEnd)
	{ a3 -= x3 - vClipEnd ;
	  x3 = vClipEnd ;
	}

	  /* because of the insertions I may be out of the consensus */
      if (a3 > amax)
	{ x3 = x3 - (a3 - amax) ;
	  a3 = amax ;
	}

      if (isUp)
	{ a1 = amax - a1 - 1 ;
	  a3 = amax - a3 - 1 ;
	}

      if (TRUE)
	{ x1++ ; 
	  if (x3 > 1000)
	    invokeDebugger() ;
	  bsAddData (obj, _Clipping, _Int, &x1) ;
	  bsAddData (obj, _bsRight, _Int, &x3) ;
	  nmv++ ; *ip += x3 - oldx2 ;
	  bsFindKey (Contig, _Assembled_from, key) ;
	  a1++ ;
	  bsAddData (Contig, _bsRight, _Int, &a1) ;
	  bsAddData (Contig, _bsRight, _Int, &a3) ;
	  bsAddData (Contig, _bsRight, _Int, &x1) ;
	  bsAddData (Contig, _bsRight, _Int, &x3) ;
	  defCptForget (0, key) ;
	}
      bsSave (obj) ;

    abort:
      arrayDestroy (dna) ;
      arrayDestroy (errArray) ;
      bsDestroy (obj) ;
    }

  bsSave (Contig) ;
  arrayDestroy (aa) ;
  arrayDestroy (consensusDirect) ;
  arrayDestroy (consensusReverse) ;
  return nmv ;
}

/********************************************************************/

typedef struct { int x1, x2, x3, oldX2, a1, a2, a3, nbErr ; 
		 KEY key ; Array dna, err ; } FIX ;

static void abiFixExtendTerminalReads (KEY contig, BOOL force)
{ OBJ Contig, obj ;
  Array dna = 0, consensus = 0, aa = 0, errArray = 0, fixes ;
  int 
    i, ii, i1, i2, j, nn, x, x1, x2, oldX2, 
    x3, a1 = 0, a2 = 0, a3, amax, endOther = 0 ;
  int vClipTop, vClipEnd ;
  KEY key, dnaKey, consensusKey ;
  A_ERR *ep ;
  FIX *fp, *bestfp ;
  char *cp, *cq ;

  Contig = bsUpdate (contig) ;
  if (!Contig) 
    return ;
  if (!bsFindTag (Contig, _Assembled_from) ||
      !bsGetKey (Contig, _DNA, &consensusKey) ||
      !(consensus = dnaGet(consensusKey)))
    { bsDestroy (Contig) ;
      return ;
    }

  aa = arrayCreate(3000, BSunit) ;
  bsFindTag (Contig, _Assembled_from) ;
  bsFlatten (Contig, 5, aa) ;

       /* look for terminal reads */
  fixes = arrayCreate (12, FIX) ;
  amax = arrayMax (consensus) ;
  for (ii = 0 , j = 0 ; ii < arrayMax(aa) ; ii += 5)
    { key = arr (aa, ii, BSunit).k ;
      a1 = arr (aa, ii + 1, BSunit).i ;
      a2 = arr (aa, ii + 2, BSunit).i ;
      x1 = arr (aa, ii + 3, BSunit).i ;
      x2 = arr (aa, ii + 4, BSunit).i ;
      
      obj = bsCreate(key) ;
      if (!obj || bsFindTag(obj, _Assembled_from))
	{ bsDestroy (obj) ;
	  continue ;
	}
      bsDestroy (obj) ;

      if (a1 > a2 &&
	  a1 >= endOther)
	endOther = a1 + 1 ;
	
      if (a1 > a2 || a2 < amax - 200)
	{ if (a2 > endOther)
	    endOther = a2 ;
	  continue ;
	}

      obj = bsUpdate(key) ;
      if (!obj) continue ;
      
      if (!bsGetKey (obj, _DNA, &dnaKey) ||
	  !(dna = dnaGet(dnaKey)))
	{ bsDestroy(obj) ;
	  continue ;
	}

      if (!bsFindTag (obj, _Old_Clipping))
	{ bsAddData (obj, _Old_Clipping, _Int, &x1) ;
	  bsAddData (obj, _bsRight, _Int, &x2) ;
	}
	
      vClipTop = -1 ;
      if (bsGetData (obj, _Vector_Clipping, _Int, &vClipTop))
	bsGetData (obj, _bsRight, _Int, &vClipEnd) ;

      oldX2 = 0 ;
      if (bsGetData (obj, _Old_Clipping, _Int, &x3))
	bsGetData (obj, _bsRight, _Int, &oldX2) ;

      a1-- ; x1-- ; /* no ZERO */


      if (x2 > 3000)
	invokeDebugger() ;
      bsDestroy (obj) ;
      fp = arrayp (fixes, j++, FIX) ;
      fp->key = key ;
      fp -> a1 = a1 ; fp->a2 = a2 ; fp->a3 = a2 ; /* at least */
      if (x2 > arrayMax(dna))
	x2 = arrayMax(dna) ;
      fp->x1 = x1 ; fp->x2 = x2 ; fp->oldX2 = oldX2 ;
      x3 = x2 + 100 ;
      if (x3 > arrayMax(dna))
	x3 = arrayMax(dna) ;
      if (vClipTop != -1 && x3 > vClipEnd)
	x3 = vClipEnd ;
      fp->x3 = x3 ;
      i = fp->a2 + fp->x3 - fp->x2 ;
      if (i > amax + 100)  /* do not over extend into rubbish */
	fp->x3 -= i - amax - 100 ;
      fp->dna = dna ;
    }

     /* count the errors in last 100 bases */

  arrayDestroy (aa) ;
  i = arrayMax(fixes) ;
  if (i < 1) goto abort ;

  fp = arrp (fixes, 0, FIX) - 1 ;
  while (fp++, i--)
    { key = fp->key ;
      a1 = fp->a1 ; a2 = fp->a2 - 1;
      dna = fp->dna ;
      x1 = fp->x1 ; x2 = fp->x2 - 1 ;
      x = amax - a2 ;
      x2 += x ; a2 += x ;
      if (x2 <= arrayMax(fp->dna))
	{ 
	  errArray = dnaAlignCptErreur (consensus, dna, &a1, &a2, &x1, &x2) ;
	  
	  if (errArray)
	    {
	      ep = arrp(errArray, 0, A_ERR)  - 1 ;
	      j = arrayMax(errArray) ; nn = 0 ;
	      
	      while (ep++, j--)
		{ x = ep->iLong ;
		  
		  if (x > amax - 100) 
		    nn ++ ;
		}
	      fp->nbErr = nn ;
	      arrayDestroy (errArray) ;
	    }
	}
      else
	fp->nbErr = 2 * amax  + 1000 ;
    }
  
      /* look for best sequence */
  i = arrayMax(fixes) ;
  fp = arrp (fixes, 0, FIX) - 1 ;
  bestfp = 0 ;
  j = 2 * amax ; /* plus infini */
  while (fp++, i--)
    { nn = fp->nbErr ;
      if (fp->a2 > amax - 50 &&
	  nn < j) 
	{ j = nn ; bestfp = fp ; }
    }
  
  /* extend consensus */
  if (bestfp)
    { if (bestfp->x3 > arrayMax (bestfp->dna))
	bestfp->x3 = arrayMax(bestfp->dna) ;
      i = bestfp->a2 + bestfp->x3 - bestfp->x2 ;
      if (i > arrayMax(consensus))
	array(consensus, i - 1, char) = 0 ;
      else
	arrayMax(consensus) = i ; /* This may shorten the consensus */
      bestfp->a3 = i ;
      cp = arrayp(consensus, bestfp->a2, char) ;
      cq = arrayp(bestfp->dna, bestfp->x2, char) ;
      i = bestfp->x3 - bestfp->x2 ;
      if (i>0)
	while (i--) *cp++ = *cq++ ;
      cp = arrayp(consensus, bestfp->a2, char) ;
      cq = arrayp(bestfp->dna, bestfp->x2, char) ;
      i = bestfp->x2  < bestfp->a2 ? bestfp->x2 : bestfp->a2 ;
      while (cp--, cq--, --i)
	if (*cp == 0) *cp = *cq ;
    }
  else 
    goto abort ;

  /* restudy the errors */
  amax = arrayMax (consensus) ;
  i = arrayMax(fixes) ;
  fp = arrp (fixes, 0, FIX) - 1 ;
  while (fp++, i--)
    { a1 = fp->a1 ; a2 = fp->a2 - 1 ;
      x1 = fp->x1 ; x2 = fp->x2 - 1 ;
      x3 = fp->x3 - 1 ;
      a3 = a2 + x3 - x2 ;
      if (a3 >= amax)
	{ x3 = x3 - (a3 - amax + 1) ;
	  a3 = amax - 1 ;
	}
      errArray = dnaAlignCptErreur (consensus, dna, &a1, &a3, &x1, &x3) ;      
      ep = arrp(errArray, 0, A_ERR)  - 1 ;
      j = arrayMax(errArray) ; nn = 0 ;
      
      while (ep++, j--)
	{ x = ep->iShort ;
	  
	  if (x < x2 - 100) continue ;
	  
	  if (x > x2 - 100) nn ++ ;
	  if (x > x2 &&  (nn * 20 > (x - x2 + 100)))  /*  4% d'erreur = 1/25 */
	    { x3 = x - 1 ; a3 = ep->iLong - 1 ; break ; }
	}
       /* now remove all errors in last 10 bases */
      ep = arrp(errArray, 0, A_ERR)  - 1 ;
      j = arrayMax(errArray) ; nn = 0 ;
       while (ep++, j--)
	{ x = ep->iShort ;
	  if (x < x3 - 10) continue ;
	  x3 = x - 1 ; a3 = ep->iLong - 1 ; break ; 
	}
      x3++ ; a3++ ; /* coordonne bio */
      if (a3 > amax)
	{ x3 = x3 - (a3 - amax) ;
	  a3 = amax ;
	}
     if (!force || fp != bestfp)
	{ fp->x3 = x3 ; fp->a3 = a3 ; fp->a1 = a1 ; fp->x1 = x1 ; }
      arrayDestroy (errArray) ;
    }
  
  /* check for a3 present twice */
  i = arrayMax(fixes) ;
  fp = arrp (fixes, 0, FIX) - 1 ;
  if (!force)
    {
      i1 = i2 = -1 ;      
      while (fp++, i--)
	{ if (fp->a3 >= i1)
	    { i2 = i1 ; i1 = fp->a3 ; }
	  else if (fp->a3 >= i2)
	    i2 = fp->a3 ;
	}
      if (i2 < endOther)
	i2 = endOther ;	
      if (arrayMax(fixes) == 1)
	{ fp = arrp (fixes, 0, FIX) ;
	  i2 = i1 ;
	  if (i2 > endOther)
	    { fp->x3 -= (i2 - endOther) ;
	      fp->a3 -= (i2 - endOther) ;
	      i2 = endOther ;
	    }
	  if (fp->x3 < fp->oldX2)
	    { i2 += fp->oldX2 - fp->x3 ;
	      fp->a3 += fp->oldX2 - fp->x3 ;
	      fp->x3 = fp->oldX2 ;
	    }
	}
    
      arrayMax (consensus) = i2 ;  /* second largest a3 */
      i = arrayMax(fixes) ;
      fp = arrp (fixes, 0, FIX) - 1 ;
      while (fp++, i--)
	{ if (fp->a3 > i2)
	    { fp->x3 = fp->x3 - (fp->a3 - i2) ;
	      fp->a3 = i2 ;
	    }
	}

      i = arrayMax(fixes) ;
      fp = arrp (fixes, 0, FIX) - 1 ;
      while (fp++, i--)
	{ if (fp->a3 == i2)
	    goto ok ;
	  if (fp->a1 <0)
	    messcrash("negative fp->a1") ;
	}
      if (i2 != endOther)
	messcrash ("no seg reaches a3") ;
    }
 ok:
  

  /* register */

  amax = arrayMax(consensus) ;
  dnaStoreDestroy(consensusKey, consensus) ;
  defCptForget (0, consensusKey) ;

  i = arrayMax(fixes) ;
  fp = arrp (fixes, 0, FIX) - 1 ;
  while (fp++, i--)
    { 
      if (fp->nbErr > amax)
	continue ;
      obj = bsUpdate(fp->key) ;
      x1 = fp->x1 ; x2 = fp->x2 ; x3 = fp->x3 ; 
      a1 = fp->a1 ; a2 = fp->a2 ; a3 = fp->a3 ;
/*
      printf("Terminals %s x1 = %d x2 = %d x3 = %d amax = %d a1 = %d a2 = %d a3 = %d \n",
	     name(fp->key), x1, x2, x3, amax, a1, a2, a3) ;
*/      
      if (a3 > amax)
	{ x3 = x3 - (a3 - amax) ;
	  a3 = amax ;
	}
      if (x3 > arrayMax(fp->dna))
	{ a3 -= x3 - arrayMax(fp->dna) ; /* on devrait restudy les erreurs */
	  x3 = arrayMax(fp->dna) ;
	}
      x1++ ; /* x2 ; x3 ; ulrich */
      bsAddData (obj, _Clipping, _Int, &x1) ;
      bsAddData (obj, _bsRight, _Int, &x3) ;
      bsFindKey (Contig, _Assembled_from, fp->key) ;
      a1++ ; /* a3 ; ulrich */
      bsAddData (Contig, _bsRight, _Int, &a1) ;
      bsAddData (Contig, _bsRight, _Int, &a3) ;
      bsAddData (Contig, _bsRight, _Int, &x1) ;
      bsAddData (Contig, _bsRight, _Int, &x3) ;
      defCptForget (0, fp->key) ;

      bsSave (obj) ;
    }
   /* clean up */
  if (bsFindKey(Contig, _DNA, consensusKey))
    bsAddData (Contig, _bsRight, _Int, &amax) ; 

 abort:
  bsSave (Contig) ;
  arrayDestroy (consensus) ;
  i = arrayMax(fixes) ;
  fp = arrp (fixes, 0, FIX) - 1 ;
  while (fp++, i--)
    { arrayDestroy (fp->dna) ;
      arrayDestroy (fp->err) ;
    }
  arrayDestroy (fixes) ;
}

/********************************************************************/

static void abiFixFlipAssembly (KEY contig)
{ OBJ Contig ;
  Array aa, consensus = 0 ;
  int 
    nn, i, a1, a2, clipt, clipe ;
  KEY key,  consensusKey ;

  Contig = bsUpdate (contig) ;
  if (!Contig) 
    return ;
  if (!bsFindTag (Contig, _Assembled_from) ||
      !bsGetKey (Contig, _DNA, &consensusKey) ||
      !(consensus = dnaGet(consensusKey)))
    { bsDestroy (Contig) ;
      return ;
    }

  aa = arrayCreate(3000, BSunit) ;
  bsFindTag (Contig, _Assembled_from) ;
  bsFlatten (Contig, 5, aa) ;
  bsFindTag (Contig, _Assembled_from) ;
  bsRemove (Contig) ;
  nn = arrayMax(consensus) + 1 ;
  for (i = 0 ; i < arrayMax(aa) ; i += 5)
    { key = arr (aa, i, BSunit).k ;
      a1 = arr (aa, i + 1, BSunit).i ;
      a2 = arr (aa, i + 2, BSunit).i ;
      clipt = arr (aa, i + 3, BSunit).i ;
      clipe = arr (aa, i + 4, BSunit).i ;
      a1 = nn - a1 ; a2 = nn - a2 ;
      bsAddKey (Contig, _Assembled_from, key) ;
      bsAddData (Contig, _bsRight, _Int, &a1) ;
      bsAddData (Contig, _bsRight, _Int, &a2) ;
      if (clipt)
	{ bsAddData (Contig, _bsRight, _Int, &clipt) ;
	  bsAddData (Contig, _bsRight, _Int, &clipe) ;
	}
    }
  arrayDestroy (aa) ;
  reverseComplement (consensus) ;
  dnaStoreDestroy (consensusKey, consensus) ;
  defCptForget (0, consensusKey) ;
  bsSave (Contig) ;
}

/********************************************************************/
/******************* public routines  *******************************/
/********************************************************************/

int abiFixDoubleContig (KEY link, KEY contig, int *ip)
{
  messStatus ("Double Stranding") ;
  return abiFixDoubleReads (contig, ip) ;
  /*
  abiFixExtendTerminalReads (contig, FALSE) ;

  abiFixFlipAssembly (contig) ;
  abiFixExtendTerminalReads (contig, FALSE) ;
  abiFixFlipAssembly (contig) ;
  if (link) 
    alignToolsAdjustContigSize (link, contig) ;
    */
}

/*************/

int abiFixDouble (KEY link, KEYSET ks, int *ip)
{ int n = 0, i = ks ? keySetMax(ks) : 0 ;
  KEY key, seq ;
  
  while (i--)
    { key = keySet(ks, i) ;
      if (class (key) == _VDNA)
	lexReClass (key, &seq, _VSequence) ;
      else
	seq = key ;
      if (seq)
	n += abiFixDoubleContig (link, seq, ip) ;
    }
  return n ;
}

/********************************************************************/

void abiFixExtendContig (KEY link, KEY contig)
{ OBJ Contig, obj ;
  Array aa ;
  int 
    i, a1, a2, dx, amax, x1, x2, end, fair2, v1, v2, c1, c2 ;
  KEY key, dnaKey ;

  messStatus ("Extend") ;
  Contig = bsUpdate (contig) ;
  if (!Contig) 
    return ;
  if (!bsGetKey (Contig, _DNA, &dnaKey) ||
      !bsGetData (Contig, _bsRight, _Int, &amax) ||
      !bsFindTag (Contig, _Assembled_from))
    { bsDestroy (Contig) ;
      return ;
    }

  aa = arrayCreate(3000, BSunit) ;
  bsFindTag (Contig, _Assembled_from) ;
  bsFlatten (Contig, 5, aa) ;
  bsFindTag (Contig, _Assembled_from) ;
  bsRemove (Contig) ;

  for (i = 0 ; i < arrayMax(aa) ; i += 5)
    { key = arr (aa, i, BSunit).k ;
      a1 = arr (aa, i + 1, BSunit).i ;
      a2 = arr (aa, i + 2, BSunit).i ;
      c1 = arr (aa, i + 3, BSunit).i ;
      c2 = arr (aa, i + 4, BSunit).i ;

       if ((obj = bsUpdate (key)))
	 { if (bsGetKey (obj, _DNA, &dnaKey) &&
	       bsGetData (obj, _bsRight, _Int, &end) &&
	       bsGetData (obj, _Clipping, _Int, &x1) &&
	       bsGetData (obj, _bsRight, _Int, &x2))	       
	     { if (!bsFindTag (obj, _Old_Clipping))
		 { if (bsAddData (obj, _Old_Clipping, _Int, &x1))
		     bsAddData (obj, _bsRight, _Int, &x2) ;
		 }
	       if (c1 && c2) /* so dna was clipped in contig */
		 { x1 = c1 ;
		   x2 = c2 ;
		   if (bsGetData (obj, _Fair_upto, _Int, &fair2))
		     { if (fair2 < end)
			 end = fair2 ;
		     }
		   else
		     { if (end > x2 + 50)
			 end = x2 + 50 ;
		     }
		   if (bsGetData (obj, _Vector_Clipping, _Int, &v1) &&
		       bsGetData (obj, _bsRight, _Int, &v2) &&
		       v2 < end)
		     end = v2 ;
		   
		   dx = end - x2 ;
		   
		   if (a1 < a2)
		     { if (a2 + dx > amax)
			 { a2 += dx ; 
			   x2 += dx ;
			 }
		     }
		   else
		     { if (a2 - dx < 0)
			 { a2 -= dx ; 
			   x2 += dx ;
			 }
		     }
		 }
	     }
	   bsSave (obj) ;
	   defCptForget (0, key) ;
	 }
      
      bsAddKey (Contig, _Assembled_from, key) ;
      bsAddData (Contig, _bsRight, _Int, &a1) ;
      bsAddData (Contig, _bsRight, _Int, &a2) ;
      if (c1 && c2)
	{ bsAddData (Contig, _bsRight, _Int, &x1) ;
	  bsAddData (Contig, _bsRight, _Int, &x2) ;
	}
    }
  arrayDestroy (aa) ;
  bsSave (Contig) ;
  alignToolsAdjustContigSize (link, contig) ;
  dnaAlignFixContig (0, contig) ;
}

/********************************************************************/

void abiFixExtend (KEY link, KEYSET ks)
{ int i = ks ? keySetMax(ks) : 0 ;
  KEY key, seq ;

  while (i--)
    { key = keySet(ks, i) ;
      if (class(key) == _VDNA)
	lexReClass (key, &seq, _VSequence) ;
      else if (class(key) == _VSequence)
	seq = key ;
      else
	continue ;
      
      abiFixExtendContig (link, seq) ;
    }
}

/********************************************************************/
 
#ifndef NON_GRAPHIC

static void dumpFinish (FILE *f, KEY contig)
{ OBJ obj = bsCreate (contig) ;
  Array aa ;
  static int nn = 1 ; /* unique seq id for finish */
  int i, a1, a2, pp, ll ;
  KEY key ;
  char buf[256], *cp, *cq ;

  if (!obj)
    return ;

  aa = arrayCreate (200, BSunit) ;
  if (bsFindTag (obj, _Assembled_from))
    { bsFlatten (obj, 3, aa) ;
      if (arrayMax (aa))
	  fprintf (f, "\nLEFT\nLEFT\n") ;	  
      for (i = 0 ; i < arrayMax(aa) ; i += 3)
	{ key = arr(aa, i, BSunit).k ;
	  a1 = arr(aa, i + 1, BSunit).i ;
	  a2 = arr(aa, i + 2, BSunit).i ;
	  if (a1 < a2)
	    { pp = a1 ; ll = a2 - a1 + 1 ;}
	  else
	    { pp = a2 ; ll = a2 - a1 - 1 ;}	

	    /* name is CX12.bp84a09.s1abi */
	  strncpy (buf, name(key), 254) ;
            /* clean up the cosmid name */
	  cp = buf ;
	  while (*cp && *cp != '.') cp++ ;
	  if (*cp == '.') *cp++ = 0 ;
	   /* clean up the ending */
	  cq = cp + strlen(cp) ;
	  while (cq > cp && !isdigit (*cq)) cq-- ;
	  if (cq > cp) *(cq + 1) = 0 ;
	  if (pp > 0)
	    fprintf (f, "%s %d %d %d 0 0 \n",
		     cp, nn++ , pp, ll) ;
	}
    }
  bsDestroy (obj) ;
  fprintf (f, "\n\n") ;
}

static FREEOPT finishTypeMenu[] =
{ {2, "Types"},
  {'x', "XL"},
   {'r', "RV"}
} ;

void abiParseFinish (FILE *f, Array aa, Array bb)
{ Array cc1, cc2, cc3 ;
  KEY kk,  type ;
  KEY new, contig ;
  int i, level = freesetfile(f,0) ;
  char *cp , buf[300] ;
  int a1, a2 ;
  OBJ obj ;

  while (freecard(level))
    {
      if (!freekey (&type, finishTypeMenu))
	continue ;
  /* get the new read */
      cp = freeword () ;
      lexaddkey (cp, &new, _VSequence) ;
  /* find the read we started from */
      if ((cp = freeword ()))
	strncpy (buf, cp,298) ;
      if (!freeint (&a1) || !freeint(&a2))
	continue ;
      cc1 = query (aa, messprintf("IS *%s*", cp)) ;
      cc2 = cc3 = 0 ;
      contig = 0 ;
  /* find the correct contig */
      for (i= 0 ; i < keySetMax(cc1) ; i++)
	{ kk = keySet(cc1, i) ;
	  keySetDestroy(cc2) ;
	  keySetDestroy(cc3) ;
	  cc2 = queryKey (kk, "> Assembled_into") ;
	  cc3 = keySetAND (cc2, bb) ;
	  if (keySetMax(cc3))
	    { contig = keySet(cc3, 0) ;
	      break ;
	    }
	}
      keySetDestroy(cc1) ;
      keySetDestroy(cc2) ;
      keySetDestroy(cc3) ;
      if (!contig)
	continue ;

      if ((obj = bsUpdate(contig)))
	{ bsAddKey (obj, _Assembled_from, new) ;
	  bsAddData (obj, _bsRight, _Int, &a1) ;
	  bsAddData (obj, _bsRight, _Int, &a2) ;
	  bsSave (obj) ;
	}
      if ((obj = bsUpdate(new)))
	{ bsAddTag (obj, _Proposed) ;
	  bsSave (obj) ;
	}
    }
}

void abiFixDoFinish (KEY link)
{
  KEYSET aa = 0, bb = 0, old ;
  int i, j ;
  FILE *f ;
  char *tmpName  = 0 , buf[100] , *cp ;
  static BOOL firstPass = TRUE ;
  char *script;

    /* get whole link and its hierarchy */
  i = 0 ;
  aa = bb = keySetCreate () ;
  aa = keySetCreate () ;
  keySet(aa, 0) = keySet (bb, i++) = link ;
  while (TRUE)
    { old = aa ;
      aa = query (old, "> Subsequence") ;
      keySetDestroy (old) ;
      if (!keySetMax(aa))
	break ;
      j = keySetMax (aa) ;
      while (j--)
	keySet(bb, i++) = keySet (aa, j) ;
    }
  
  keySetDestroy (aa) ;
  
  keySetSort (bb) ;
  keySetCompress (bb) ;
  

  if (!keySetMax(bb))
    { messout("No reads to finish") ;
      goto abort ;
    }

  aa = query (bb ,"> Assembled_from") ;
  strncpy (buf, name(keySet(aa,0)), 99) ;

  cp = buf ;
  while (*cp && *cp != '.') cp++ ;
  *cp = 0 ;

  f = filtmpopen(&tmpName,"w") ;
  if (!f)
    goto abort ;
  i = keySetMax(bb) ;
  while (i--)
    dumpFinish (f, keySet(bb, i)) ;
  filclose (f) ;

  if (firstPass)
    { firstPass = FALSE ;
      messout ("wscripts/finish \nContributed by Gabor Marth (wash U)") ;
    }

  script = dbPathFilName(WSCRIPTS, "ace2finish", "", "x", 0);
  messStatus ("Finishing") ;
  if (!callScript(script, messprintf ( "%s %s", buf, tmpName)))
    { 
      if ((f = filopen(tmpName,"a","r")))
      { 
	externalFileDisplay ("Finish", f, 0) ;
	filremove(tmpName,"a") ;
      }
    if ((f = filopen(tmpName,"c","r")))
	{ 
	  abiParseFinish (f, aa, bb) ;
	  /*      filremove(tmpName,"c") ;
	   */
	  callScript ("emacs", messprintf("%s.c &", tmpName)) ;
	}	
    }
  
  messfree(script);
  filtmpremove(tmpName) ;
  display (link, 0, 0) ;
 abort:
  keySetDestroy(aa) ;
  keySetDestroy(bb) ;
}

#endif /* !NON_GRAPHIC */

/********************************************************************/
/********************************************************************/

typedef struct  { int dcp, dcq, lng, ok ; } JUMP ;

/* in a pdded system, there should be no insert delete ? */
#ifdef JUNK
static JUMP paddedJumper [] = {
{ 1, 1, 0, 0 }   /* default is punctual */
} ;
#endif

static JUMP jumper [] = {
 {1, 1, 7, 0},    /* ponctuel */
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


static JUMP insertJumper [] = {
 {1, 1, 7, 0},    /* ponctuel */
 {1, 0, 5, 1},    /* insert in 1 */
 {0, 1, 2, 0},    /* trou in 1 */
 {1, 1, 3, 0},    /* ponctuel */
 {1, 0, 7, 2},    /* insert in 1 */
 {0, 1, 4, 2},    /* trou in 1 */
 {2, 0, 9, 2}, 
 {0, 2, 5, 2}, 
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


static JUMP deleteJumper [] = {
 {1, 1, 7, 0},    /* ponctuel */
 {1, 0, 2, 0},    /* insert in 1 */
 {0, 1, 5, 1},    /* trou in 1 */
 {1, 1, 3, 0},    /* ponctuel */
 {1, 0, 4, 2},    /* insert in 1 */
 {0, 1, 7, 2},    /* trou in 1 */
 {2, 0, 5, 2}, 
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


static JUMP reAligner [] = {
 {0, 0, 5, 1},    /* accept current location */
 {1, 0, 5, 1},    /* insert in 1 */
 {0, 1, 5, 1},    /* trou in 1 */
 {1, 1, 5, 0},    /* ponctuel */
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
 {0, 0, 0, 0}    /* default is accept current location */
} ;

static JUMP jumpN [] = {{1000,0,0,0}} ;  /* a kludge, to jump ambiguities */
  /* dna1 = short, dna2 = long */

Array trackErrors (Array  dna1, int pos1, int *pp1, 
		   Array dna2, int pos2, int *pp2, int *NNp, Array err, int mode)
{ char *cp, *cq, *cp1, *cq1, *cp0, *cq0, *cpmax, *cqmax ;
  int i, n, sens = 1, nerr = 0, nAmbigue = 0 ;
  JUMP *jp ;
  A_ERR *ep ;
  Array errArray = arrayReCreate (err, 50, A_ERR) ;
  JUMP *myJumper ;

  switch (mode)
    {
    case 1: /* favor delete */
      myJumper = deleteJumper ;
      break ;
    case 2: /* favor insert */
      myJumper = insertJumper ;
      break ;
    default:
      myJumper = jumper ;
      break ;
    }

  cp0 = arrp (dna1, 0, char) ;
  cp = cp0 + pos1 ; cpmax = cp0 + 
    ( *pp1 < arrayMax(dna1)  ? *pp1 : arrayMax(dna1)) ;
  cq0 = arrp (dna2, 0, char) ;
  cq = cq0 + pos2 ; cqmax = cq0 + arrayMax(dna2) ;
  cp-- ; cq-- ;
  while (++cp < cpmax && ++cq < cqmax)
    {
      if (*cp == *cq)
	continue ;

#ifdef JUNK
      /* old system, never allow N_ in jumps */
      if  (!(*cp & 0x0F) || (*cp & 0x0F) == N_) 
	{ jp = jumpN ; goto foundjp ; }

      jp = jumper ; jp-- ;
      while (jp++) /* 1, 1, 0, 0 = last always accepted */
	{ n = 0 ; i = jp->ok ;
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
#endif /* JUNK */
      /* allow N_ */
      if  (!(*cp & 0x0F))
	{ jp = jumpN ; goto foundjp ; }

      jp = myJumper ; jp-- ;
      while (jp++) /* 1, 1, 0, 0 = last always accepted */
	{ n = 0 ; i = jp->ok ;
	  cp1 = cp + jp->dcp ; cq1 = cq + jp->dcq ;
	  while (i--)
	    {
	      if (cp1 == cpmax || cq1 == cqmax)
		goto nextjp ;
	      if (! ((*cp1 & 0x0F) & (*cq1++ & 0x0F)))
		goto nextjp ;
	      if ((*cp1++ & 0x0F) == N_)
		{
		  i++ ; 
		  if (++n > jp->ok)
		    goto nextjp ;
		}
	    }
	  n = 0 ; i = jp->lng ;
	  cp1 = cp + jp->dcp ; cq1 = cq + jp->dcq ;
	  while (i--)
	    {
	      if (cp1 == cpmax || cq1 == cqmax)
		goto nextjp ;
	      if (! ((*cp1 & 0x0F) & (*cq1++ & 0x0F)))
		if (++n > jp->ok)
		  goto nextjp ;
	      if ((*cp1++ & 0x0F) == N_)
		{
		  i++ ; 
		  if (++n > jp->ok)
		    goto nextjp ;
		}
	    }
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
	  ep->type = *cp == N_ ? AMBIGUE : ERREUR ;
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
	  if ((*cp & 0x0f) == N_) { cp -= sens ; cq-- ; }
	  break ;
	case -1:
	  ep->type = TROU ;
	  ep->iShort = cp - cp0 ;
	  ep->iLong = cq - cq0 ;
	  ep->sens = sens ;
	  ep->baseShort = '*' ;
	  cq++ ;
	  if ((*cp & 0x0f) == N_) { cp -= sens ; cq-- ; }
	  break ;
	default:
	  if ( jp->dcp > jp->dcq)
	    {
	      ep->iShort = cp - cp0 ;
	      ep->iLong = cq - cq0 ;
	      ep->sens = sens ;
	      ep->baseShort = *cp ;
	      /* insertion accepted, try single insertion */
	      cp1 = cp + sens ; cq1 = cq ;
	      if ((*cp1 & 0x0F) & (*cq1 & 0x0F))
		{ ep->type = INSERTION ; cp += sens ; }
	      else
		{ ep->type = INSERTION_DOUBLE ; cp += 2*sens ; }
	    }
	  else
	    {
	      ep->iShort = cp - cp0 ;
	      ep->iLong = cq - cq0 ;
	      ep->sens = sens ;
	      ep->baseShort = '*' ;
	      /* trou accepted, try single trou */
	      cp1 = cp + 1 ; cq1 = cq ;
	      if ((*cp1 & 0x0F) & (*cq1 & 0x0F))
		{ ep->type = TROU ; cq++ ; }
	      else
		{ ep->type = TROU_DOUBLE ; cq += 2 ; }
	    }
	  if ((*cp & 0x0f) == N_) { cp -= sens ; cq-- ; }
	}
    }
  *pp1 = cp - cp0 ;
  *pp2 = cq - cq0 ;
  *NNp = nAmbigue ;
  return errArray ;
}


static int trackFirstJump (Array  dna1, int pos1, Array dna2, int pos2)
{ char *cp, *cq, *cp1, *cq1 ;
  int i, n ;
  JUMP *jp ;

  if (pos1 < 0 || pos1 > arrayMax(dna1))
    return 0 ;
  cp = arrp (dna1, pos1, char) ;
  if (pos2 < 0 || pos2 > arrayMax(dna2))
    return 0 ;
  cq = arrp (dna2, pos2, char) ;

  if (*cp == *cq)
    return 0 ;

  jp = reAligner ; jp-- ;
  while (jp++) /* 1, 1, 0, 0 = last always accepted */
    { n = 0 ; i = jp->ok ;
      cp1 = cp + jp->dcp ; cq1 = cq + jp->dcq ;
      while (i--)
	if ((*cp1++ & 0x0F) != (*cq1++ & 0x0F))
	  goto nextjp ;
      n = 0 ; i = jp->lng ;
      cp1 = cp + jp->dcp ; cq1 = cq + jp->dcq ;
      while (i--)
	if ((*cp1++ & 0x0F) != (*cq1++ & 0x0F))
	  if (++n >= jp->ok)
	    goto nextjp ;
      goto foundjp ;
    nextjp:
      continue ;	  
    }
 foundjp:

  return jp->dcq - jp->dcp ;
}

/********************************************************************/

void newLocalCptErreur(Array longDna, int xl1, int xl2, int pol,
		    Array shortDna, int xs1, int xs2, int pos, int sens,
		    int *NNp, int *startp, int *stopp, int *recouvp, Array errArray)
{ Array err ;

  if (sens == 1)
    { xl1 += trackFirstJump (shortDna, xs1, longDna, xl1) ;
      err = trackErrors (shortDna, xs1, &xs2, 
			 longDna, xl1, &xl2, NNp, errArray, 0) ;
      *stopp = xl2 ; *startp = xl1 ;
      *recouvp = xl2 - xl1 ;
    }
  else
/* 
    oldLocalCptErreur (longDna, xl1, xl2, pol, shortDna, xs1, xs2,
		       pos, sens, NNp, startp, stopp, recouvp, errArray) ; 
*/ messcrash ("newLocalCptErreur pas pgm") ;
  return ;
}

/*********************************************************/

int laneGlobalOrder  (void *a, void *b)
{ LANE* la = (LANE*)a,  *lb = (LANE*) b ;

  BOOL
    ra = la->upSequence ,
    rb = lb->upSequence ;
  int 
    xa = ra ? la->x2 : la->x1 ,
    xb = rb ? lb->x2 : lb->x1 ;
/*
  if (ra && !rb)
    return -1 ;
  if (rb && !ra)
    return 1 ;
*/
  return
    xa < xb ? -1 : (xa == xb ? 0 : 1) ;
}

/********************************************************************/

static void traceSort (LOOK look)
{ arraySort (look->lanes, laneGlobalOrder) ;
}

static void reRegister (Array lanes, OBJ obj, int sens)
{ int ss ;
  int x1, x2, c1, c2,  iLane ;
  LANE *lane ;

  if (!obj) return ;
 
  for (iLane = 0 ; iLane < arrayMax(lanes) ; iLane++)
    { 
      lane = arrp (lanes, iLane, LANE) ;
      ss = lane->upSequence ? -sens : sens ;
      if (ss > 0)
	{
	  x1 = lane->x1 + 1 ;
	  x2 = lane->x2 + 2 ;
	}
      else
	{
	  x1 = lane->x1 + 1 ;
	  x2 = lane->x2 ;
	}
      c1 = lane->clipTop+ 1 ;
      c2 = lane->clipEnd ;
      
      if (lane->isAligned)
	{
	  if (!bsFindKey (obj, _Aligned, lane->key))
	    bsAddKey (obj, _Aligned, lane->key) ;
	}
      else
	{ 
	  if (!bsFindKey (obj, _Assembled_from, lane->key))
	    bsAddKey (obj, _Assembled_from, lane->key) ;
	}

      bsAddData (obj, _bsRight, _Int, &x1) ;
      bsAddData (obj, _bsRight, _Int, &x2) ;
      bsAddData (obj, _bsRight, _Int, &c1) ;
      bsAddData (obj, _bsRight, _Int, &c2) ;
    }
}


typedef struct {LANE* lane ; BOOL up, in ; int pos ;
		char *cp ; JUMP* jp ; } READ ;

void trackContig (LOOK look, int z1, int z2, BOOL whole)
{ int 
    i, dx0, dq, dx, dxw, pos, dxModif, maxModif, nrtot,
    n, nr, nrr, nRes, iLane, iLanesMax, iRead, nRead ;
  char *cp, *cq, *cq0, *cp1, *cq1, *cqMax, cc1, cc2, ccq ;
  Array 
    reads, result,
    consensus = look->dna , 
    lanes = look->lanes ; 
  int done[16], nq, sens ;
  float nn[16] ;
  JUMP *jp ;
  LANE *lane ; READ *rr ;
  OBJ obj ;


  if (whole || z2 >= arrayMax (consensus))
    z2 = arrayMax (consensus) ;

  traceSort (look) ;
  i = 16 ; while (i--) done[i] = 0 ;
  i = 16 ; while (i--) nn[i] = 0 ;

  iLane = 0 ; iLanesMax = lanes ? arrayMax (lanes) : 0 ;
  if (whole && iLanesMax)
    { 
      lane = arrp (lanes, 0, LANE) ;
      z1 = lane->upSequence ? lane->x2 : lane->x1 ;
    }

  if (z2 < z1)
    return ;
 
  obj = bsUpdate (look->key) ;
  result = arrayCreate (2*arrayMax(look->dna) + 2000, char) ;
  reads = arrayCreate (50, READ) ; nRead = 0 ;
  pos = z1 ;
  if (z1 > 0 && !whole) /* copier le debut, mais pas si whole */
    for (nRes = 0 ; nRes < z1 ; nRes++)
      array (result, nRes, char) = array (consensus, nRes, char) ;
  if (z1 < 0)
    { /* shift inside obj */
      float dq = -z1 , zero = 0;
      if (obj)
	bsCoordShift (obj, _Structure, zero, dq, TRUE) ;
      /*  shilft lanes */
      iLane = iLanesMax ;
      while (iLane--)
	{ lane = arrp (lanes, iLane, LANE) ;
	  lane->x1 += dq ; lane->x2 += dq ;
	}
      /* shift the consensus */
      arrayDestroy (look->dnaR) ;
      i = arrayMax(consensus) ;
      array(consensus, i - 1 - z1, char) = N_ ; /* make room */
      while (i-- > 0)
	arr (consensus, i- z1, char) = arr (consensus, i, char) ;  
      i = -z1 ;
      while (i-- > 0)
	arr (consensus, i- z1, char) = N_ ;
      look->dnaR = arrayCopy (look->dna) ;
      reverseComplement (look->dnaR) ;
      z2 += -z1 ; z1 = 0 ;
    }
  
  iLane = 0 ;
  cq0 = arrp (consensus, 0, char) ;
  cqMax = cq0 + z2 ;
  nRes = z1 > 0 ? z1 : 0 ; 
  arrayMax (result) = 0 ; array (result, nRes, char) = 0 ; 
  pos = z1 ; /* may be negative */
  cq = cq0 + pos ;

  dq = 0 ;
  while (cq < cqMax || (nRead && whole)) 
    { 
      dx0 = 0 ; pos = cq - cq0 ;
      while (iLane < iLanesMax &&
	     (lane = arrp (lanes, iLane, LANE)))
	{ 
	  if (
	      (!lane->scf && !traceGetLane (look, lane)) || /* non usable */
	      lane->isAligned ||  /* uninteresting */
	      ((lane->upSequence && lane->x1 < pos) ||
	       (!lane->upSequence && lane->x2 < pos)) ||   /* too far down in consensus */
	      (!lane->upSequence &&  /* too far in the read */
	       pos - lane->x1  >= lane->clipEnd - lane->clipTop - 1) ||
	      (lane->upSequence &&    /* too far in the read */
	       pos - lane->x2  >= lane->clipEnd - lane->clipTop)
	      )
	   { iLane++ ; continue ; }  /* forget that one */

	   if ((lane->upSequence && lane->x2 > pos) ||
	       (!lane->upSequence && lane->x1 > pos)  )
	   break ; /* too early */
	   iLane++ ; /* avoid recursion */
	  /* insert */
	   nRead++ ;
	  for (i = 0 ; i < arrayMax(reads) ; i++)
	    { rr = arrp (reads, i, READ) ;
	      if (!rr->in) /* search empty spot */
		break ;
	    }

	  rr = arrayp (reads, i, READ) ;
	  rr->in = TRUE ;
	  rr->lane = lane ;
	  if (lane->upSequence)
	    { rr->up = TRUE ;
	      rr->pos = lane->clipEnd - pos + lane->x2 ;
	      rr->cp = arrp (lane->dna, rr->pos, char) ;
	    }
	  else
	    { rr->up = FALSE ;
	      rr->pos = lane->clipTop + pos - lane->x1 ;
	      rr->cp = arrp (lane->dna, rr->pos, char) ;
	    }
	  /* realign */
	  jp = reAligner ; jp-- ;
	  sens = rr->up ? -1 : 1 ;
          cp = rr->cp ;
	  while (jp++) /* 0, 0, 0, 0 = last always accepted */
	    { if (cq < cq0 || cq >= cqMax)
	       { 
		 if (jp->ok)
		   continue ;
		 else
		   break ;
	       }
	      n = 0 ; i = jp->ok ;
	      cp1 = cp + sens * ( jp->dcp - 1) ; cq1 = cq + jp->dcq ;
	      while (cp1 += sens, i--)
		{ cc1 = (*cp1 & 0x0F) ;
		  if (sens == -1) cc1 = complementBase[(int)(cc1)] ;
		  if (cc1 != (*cq1++ & 0x0F))
		    goto nextjp1 ;
		}
	      n = 0 ; i = jp->lng ;
	      cp1 = cp + sens * (jp->dcp - 1) ; cq1 = cq + jp->dcq ;
	      while (cp1 += sens, i--)
		{ cc1 = (*cp1 & 0x0F) ;
		  if (sens == -1) cc1 = complementBase[(int)(cc1)] ;
		  if (cc1 != (*cq1++ & 0x0F))
		    if (++n >= jp->ok)
		      goto nextjp1 ;
		}
	      goto foundjp1 ;
	    nextjp1:
	      continue ;	  
	    }
	foundjp1:
	  if (jp)
	    { dx = jp->dcp - jp->dcq ;
	      if (rr->up) dx = - dx ; 
	      rr->pos += dx ;
	      rr->cp += dx ;
	    }
	  if (!rr->up)
	    rr->lane->x1 = nRes - (rr->pos - lane->clipTop) ;
	  else
	    rr->lane->x2 = nRes + (rr->pos - lane->clipEnd) ;   
	}
      

      maxModif = dx0 =  0 ;
      i = 16 ; while (i--) done[i] = 0 ; cc1 = 0 ;
    lao:
      dx = 0 ; nr = nrr = nq = nrtot = 0 ; dxModif = 0 ;
      i = 16 ; while (i--) nn[i] = 0 ;
      iRead = arrayMax(reads) ; ccq = cq >= cq0  && cq < cqMax ? (*cq  & 0x0f) : cc1 ;
      if (ccq == N_) ccq = 0 ;
      while (iRead--)
	{ rr = arrp (reads, iRead, READ) ;
	  if (!rr->in)
	    continue ;
	  dxw = 10000 ; /* poids de cette base en nombre de base */
	  /* dxw := in the future: 55 70 90 100 110 130 145 */
          dxModif += dxw - 10000 ; nrr += 10000 ;
	  /* if (rr->excellent) */
	    nr = 10000  + (/* rr->excellent */  - rr->pos) ;  /* N_ counts like 1/3 */
	  rr->jp = 0 ;
	  cp = rr->cp ;
	  cc1 = *cp & 0x0f ; if (cc1 == N_) { cc1 = 0 ; nr -= 7000 ; }  /* N_ counts like 1/3 */
	  if (rr->up) cc1 = complementBase[(int)cc1] ;
	  nn[(int)cc1] += nr ; /* this favors good quality i hope */
	  nrtot += nr ;
	  if (cc1 & ccq)  /* see the N_ */
	    { nq++ ; continue ; }
	  jp = jumper ; jp-- ;
	  sens = rr->up ? -1 : 1 ;
	  while (jp++) /* 1, 1, 0, 0 = last always accepted */
	    { 
	      if (cq < cq0 || cq >= cqMax)
	       { 
		 if (jp->ok)
		   continue ;
		 else
		   break ;
	       }
	      n = 0 ; i = jp->ok ;
	      cp1 = cp + sens * ( jp->dcp - 1) ; cq1 = cq + jp->dcq ;
	      while (cp1 += sens, i--)
		{ cc2 = (*cp1 & 0x0F) ;
		  if (sens == -1) cc2 = complementBase[(int)(cc2)] ;
		    if (cc2 != (*cq1++ & 0x0F))
		      goto nextjp ;
		}
	      n = 0 ; i = jp->lng ;
	      cp1 = cp + sens * (jp->dcp - 1) ; cq1 = cq + jp->dcq ;
	      while (cp1 += sens, i--)
		{ cc2 = (*cp1 & 0x0F) ;
		  if (sens == -1) cc2 = complementBase[(int)(cc2)] ;
		  if (cc2 != (*cq1++ & 0x0F))
		    if (++n > jp->ok)
		      goto nextjp ;
		}
	      goto foundjp ;
	    nextjp:
	      continue ;	  
	    }
	foundjp:
	  rr->jp = jp ;
	  if (rr->jp)
	    { 
	      if (jp->dcp > jp->dcq) dx += 10000 ; 
	      else if (jp->dcp < jp->dcq) dx -= 10000 ;
	      dxModif += 10000 * (jp->dcp - jp->dcq) ; 
	    }
	}
      if (!dx0) maxModif = dxModif ;
      if (2*dx > nrr && dx0 >= 0 && nrr * dx0 < maxModif 
	  && cq > cq0 && cq < cqMax)
	{ dx0++ ; dq += 1 ; *--cq  = cc1 ; goto lao ; }
      if (2*dx < - nrr && dx0 <= 0 && nrr * dx0 > maxModif && cq > cq0)
	{ dx0-- ; dq -= 1 ; cq++ ; goto lao ; }

      /* en moyenne on a maintenant le bon nombre de bases */
      if (2 * nn[(int)(ccq)] < nrtot)
	{ int f = 0 ;
	  cc1 = N_ ;
	  if (nn[A_] > f) { f = nn[A_] ; cc1 = A_ ; }
	  if (nn[T_] > f) { f = nn[T_] ; cc1 = T_ ; }
	  if (nn[G_] > f) { f = nn[G_] ; cc1 = G_ ; }
	  if (nn[C_] > f) { f = nn[C_] ; cc1 = C_ ; }
	  
	  if (ccq != cc1 && !done[(int)(cc1)])
	    { done[(int)(cc1)] = TRUE ; if (cq >= cq0 && cq < cqMax) *cq = cc1 ; goto lao ; }
	}
      /* eliminate finished reads */
      i = arrayMax (reads) ;
      while (i--)
	{ rr = arrp (reads, i, READ) ;
	  if (!rr->in) continue ;
	  if (!rr->up && rr->pos >= rr->lane->clipEnd - 1)
	    { nRead-- ; 
	      rr->in = FALSE ;
	      rr->lane->x2 = nRes + 1 ;
	      arrayDestroy (rr->lane->errArray) ; /* now obsolete */
	    }
	  else if (rr->up && rr->pos <= rr->lane->clipTop)
	    { nRead-- ;
	      rr->in = FALSE ;
	      rr->lane->x1 = nRes ;
	      arrayDestroy (rr->lane->errArray) ; /* now obsolete */ 
	    }
	}
      /* register new solution */
      array(result, nRes++, char) = ccq ;
      arrayMax (result) = nRes ;

      /* move forward in active reads */
      i = arrayMax(reads) ; /* cc = ccq ; */
      while (i--)
	{ rr = arrp (reads, i, READ) ;
	  if (!rr->in)
	    continue ;
	  if (rr->jp && rr->jp->dcp > rr->jp->dcq)
	    dx = 1 + rr->jp->dcp ; /* jump */
	  else if (rr->jp && rr->jp->dcp < rr->jp->dcq && nRead > 1)
	    dx = 0 ; /* wait */
	  else
	    dx = 1 ;
	  rr->cp += rr->up ? -dx : dx ;
	  rr->pos += rr->up ? -dx : dx ;
	}
      cq++ ;   pos = cq - cq0 ;
    }

  iLane = 0 ; z2 = pos ;
  if (obj)
    bsCoordShift (obj, _Structure, (float) pos, (float) dq, TRUE) ;
      /* eliminate finished reads */
  i = arrayMax (reads) ;
  while (i--)
    { rr = arrp (reads, i, READ) ;
      if (!rr->in) continue ;
      if (!rr->up)
      { nRead-- ; 
        rr->in = FALSE ;
        rr->lane->x2 += dq ;
        arrayDestroy (rr->lane->errArray) ; /* now obsolete */
      }
    else if (rr->up && rr->pos <= rr->lane->clipTop)
      { nRead-- ;
        rr->in = FALSE ;
        rr->lane->x1 += dq ;
        arrayDestroy (rr->lane->errArray) ; /* now obsolete */ 
      }
    }
  /* shift all other reads */
  while (iLane < iLanesMax &&
	 (lane = arrp (lanes, iLane++, LANE)))
    { if (lane->x1 > pos) lane->x1 +=  dq ;
      if (lane->x2 > pos) lane->x2 +=  dq ;
      arrayDestroy (lane->errArray) ; /* now obsolete, could be shifted en pos, dq */	    
      if (lane->x1 > z2) z2 = lane->x1 ;
      if (lane->x2 > z2) z2 = lane->x2 ;
    }
      
  if (obj)
    { reRegister (lanes, obj, look->sens) ;
      bsSave (obj) ;
    }
  if (z2 < 0) z2 = 0 ; /* possibly trim off the end */
  cq-- ; cqMax = cq0 + arrayMax (consensus) ;
  while (++cq < cq0 + z2) /* but copy over the explored region to the end of last read */
    array (result, nRes++, char) = cq >= cq0 && cq < cqMax ? *cq : N_ ;
  arrayDestroy (reads) ;
  arrayDestroy (look->dna) ;
  arrayDestroy (look->dnaR) ;
  look->dna = arrayCopy(result) ;
  look->dnaR = arrayCopy (look->dna) ;
  reverseComplement (look->dnaR) ;
  if (look->sens < 0)
    reverseComplement (result) ;
  if (look->dnaKey)
    dnaStoreDestroy (look->dnaKey, result) ;
  else 
    arrayDestroy (result) ;
}

void trackFixContig (KEY link, KEY contig, KEY contigDnaKey, Array af, Array dna)
{ int i, j ;
  LOOK look =(LOOK)  messalloc (sizeof (struct LookStruct)) ;
  BSunit *uu ;
  LANE *lane = 0 ;
  char *cp, *cq ;
       
  look->magic = (void*)1 ;
			
  look->key = contig ;
  look->dnaKey = contigDnaKey ;
  look->dna = arrayCopy (dna) ;
  look->dnaR = arrayCopy (dna) ;
  reverseComplement (look->dnaR) ;
  look->lanes = arrayCreate (arrayMax(af)/4, LANE) ;

  for (i = 0, j = 0 ; i < arrayMax(af) ; i += 4)
    { lane = arrayp (look->lanes, j++, LANE) ;
      uu = arrp (af, i, BSunit) ;
      lane->key = uu[0].k ; 
      if (uu[1].i < uu[2].i)
	{ lane->upSequence = FALSE ; 
	  lane->x1 = uu[1].i - 1 ;
	  lane->x2 = uu[2].i ;
	  lane->x3 = uu[2].i ;
	}
      else
	{ lane->upSequence = TRUE ; 
	  lane->x1 = uu[1].i - 1 ;
	  lane->x2 = uu[2].i - 2 ;
	  lane->x3 = uu[2].i - 2 ;
	}
      lane->dna = blyDnaGet (link, contig, lane->key) ; /* clipped dna */
      lane->clipTop = 0 ;
      lane->minBase = uu[3].i ; /* surcharge */
      lane->clipEnd = arrayMax (lane->dna) ;
      lane->scf = 1 ;
    }

  trackContig (look, 0, arrayMax(look->dna), TRUE) ;

  /* reregister for ulrich */

  cp = arrp (look->dna, 0, char) ;
  i = arrayMax(look->dna) ;
  array (dna, i,char) = 0 ;
  arrayMax(dna) = i ;
  cq = arrp (dna, 0, char) ;
  while (i--) *cq++ = *cp++ ;

  for (i = 0, j = 0 ; i < arrayMax(af) ; i += 4)
    { lane = arrayp (look->lanes, j++, LANE) ;
      uu = arrayp (af, i, BSunit) ;
      uu[0].k = lane->key ;
      if (lane->upSequence)
	{ uu[1].i = lane->x1 + 1 ;
	  uu[2].i = lane->x2 + 2;
	}
      else
	{ uu[1].i = lane->x1 + 1 ;
	  uu[2].i = lane->x2 ;
	}
      uu[3].i = lane->minBase ;   /* overloading */
    }

  arrayDestroy (look->dna) ;
  arrayDestroy (look->dnaR) ;
  arrayDestroy (look->lanes) ;
  look->magic = 0 ;
  messfree (look) ;
}

/********************* end of file **********************************/
/********************************************************************/
 
static BOOL doGetVector (KEY *vecp, KEY seq,  Array v1, Array v2, int *posp, int *posMaxp)
{ KEYSET ks = 0 ;  KEY vec ;
  static KEY  _sv = 0, _lm, _lr, _Maximal_position ;
  int i ;
  OBJ obj = 0 , obj1 = 0 ;
  char *cp ;
  static KEY mainClone = 0 ;
  char *codage = 0 ;

  if (!_sv)
    {
      lexaddkey ("Sequencing_Vector", &_sv, _VSystem) ;
      lexaddkey ("Left_Motif", &_lm, _VSystem) ;
      lexaddkey ("Right_Motif", &_lr, _VSystem) ;
      lexaddkey ("Maximal_position", &_Maximal_position, _VSystem) ;
    }
  if (!mainClone)
    { if ((ks = query (0, "Find Clone Main_Clone")) &&
	  keySetMax (ks) > 0)
	mainClone = keySet (ks, 0) ;
      else
	mainClone = 1 ;
      keySetDestroy (ks) ;
    }
      
  if (!(obj = bsCreate (seq)))
    return FALSE ;
  /* search the vector in the read then in main clone */
  vec = *vecp ;
lao:
  codage = 0 ;
  if (!vec)
    { bsGetKey (obj, _sv, &vec) ;
      if (!vec && mainClone && mainClone > 1 &&
	  (obj1 = bsCreate (mainClone) ))
	if (bsGetKey (obj1, _sv, &vec))
	  bsGetData (obj1, _bsRight, _Text, &codage) ;
    }
  else
    { if (bsFindKey (obj, _sv, vec))
	 { vec = 0 ;
	  bsGetKey (obj, _bsDown, &vec) ;
	}
      else if (mainClone && mainClone > 1 &&
	       (obj1 = bsCreate (mainClone) ) &&
	       bsFindKey (obj1, _sv, vec))
	{ vec = 0 ;
	  if (bsGetKey (obj1, _bsDown, &vec))
	    bsGetData (obj1, _bsRight, _Text, &codage) ;
	}
      else
	vec = 0 ;
    }
  bsDestroy (obj1) ;
  if (vec && !codage && (obj1 = bsCreate(vec)))
    {
      bsGetData (obj1, str2tag("Codage"), _Text, &codage) ;
      bsDestroy(obj1) ;
    }
  if (vec && codage && !pickMatch(name(seq), codage))
    goto lao ;
  bsDestroy (obj) ;

  if (!vec || 
      !(obj = bsCreate (vec)))
    return FALSE ;

  *posp = *posMaxp = 0 ;
  bsGetData (obj, _Position, _Int, posp) ;
  bsGetData (obj, _Maximal_position, _Int, posMaxp) ;
  v1 = arrayReCreate (v1, 20, char) ;
  v2 = arrayReCreate (v2, 20, char) ;
  if (bsGetData (obj, _lm, _Text, &cp))
    { i = 0 ;
      while ((array (v1, i++, char) = dnaEncodeChar[(int)(*cp++)])) ;
      arrayMax(v1) -- ;
    }
  if (bsGetData (obj, _lr, _Text, &cp))
    { i = 0 ;
      while ((array (v2, i++, char) = dnaEncodeChar[(int)(*cp++)])) ;
      arrayMax(v2) -- ;
    }
  bsDestroy (obj) ;
  *vecp = vec ;
  return TRUE ;
}

/***********************/

int doTrackVector (KEY seq, Array v1, Array v2, BOOL force) 
{ 
  Array dna = 0, vector ;
  OBJ obj ;
  KEY dnaKey, vec ;
  int 
    old, c1, c2, ok = 0, n, pos, dx, ddx,
    pp = 0 , pp1, pp2, min, lg, lg1, nn, 
    pos1, pos2, /* begin end of insert */ 
    posMax, posMax2,  /* never pi > posMax, careful after posMax2 = (pp1+posMax) / 2 */
    lg2 ;
  Array err = 0 ;
  int seuil = 3 ;
  
  if (class(seq) == _VDNA && !lexReClass(seq, &seq, _VSequence))
    return 0 ;
  
  obj = bsUpdate (seq) ;
  if (!obj)
    return 0 ;
  
  if (!bsGetKey (obj, _DNA, &dnaKey) ||
      (bsFindTag (obj, _Vector_Clipping) && !force))
    goto abort ;
  
  dna = dnaGet (dnaKey) ;
  if (!dna)
    goto abort ;
  
  pp1 = 1 ; pp2 = arrayMax (dna) ;
  vec = 0 ;
  while (!ok && doGetVector (&vec, seq, v1, v2, &pos1, &posMax))
    { 
      /* search pre_insert of vector among 50 bp at beginning of sequence */
      min = 100 ;
      vector = v1 ;
      lg = old = arrayMax (vector) ;
      lg1 = arrayMax (vector) - 8 ; 
      if (pos1 < 0) pos1 = 0 ; 
      if (posMax < 0) posMax = 0 ; 
      if (!posMax) { posMax = pos1 + 35 ; }
      posMax2 = (pos1 + posMax) / 2 ;
      if (posMax2 < 25) posMax2 = posMax < 25 ? posMax : 25 ;
      if (lg1 >= 0)
	for (dx = 0 ; min > 0 && dx < 35 ; dx++)
	  for (ddx = -1 ; min > 0 && ddx < 2 ; ddx += 2)
	    { 
	      pos = pos1 + ddx*dx ;
	      if (ddx == 1 && dx == 0) continue ;
	      if (pos <=0 ||  pos > posMax || pos >= arrayMax(dna) - lg)
		continue ;
	      lg2 = lg ; pos2 = pos + lg ; lg1 = 0 ;
	      if (pos < 20 && lg1 > lg - 8) lg1 = lg - 8 ;
	      err = trackErrors (vector, lg1, &lg2, dna, pos, &pos2, &nn, err, 0) ;
	      n = arrayMax (err) ;
	      if (lg2 == lg && min > n) 
		{ min = n; pp = pos2 + 2 ; }
	      if (n > 2*seuil && min < 2)
		break ;
	    }
      n = min ;
      if ((pp < posMax2 && n < 3) || n < 2)
	{ ok++ ;
	pp1 = pp ;
	}
      
      /* search end of post_insert_region of vector at end of sequence */
      min = 100 ; nn = 0 ;
      vector = v2 ;
      lg = old = arrayMax (vector) ; /* if (lg > 15) lg = arrayMax (vector) = 15 ; */
      if (lg > 8)
	for (pos = pp1 > 1 ? pp1 : pos1 ; pos < arrayMax(dna) - lg && min > 0 && nn < 6 ; pos++)
	  { lg2 = lg ; pos2 = pos + lg ;
	  err = trackErrors (vector, 0, &lg2, dna, pos, &pos2, &nn, err, 0) ;
	  n = arrayMax (err) ;
	  if (lg2 == lg && min > n) 
	    { min = n; pp = pos ; }
	  if (n > 2*seuil && min < seuil)
	    break ;
	  }
      n = min ;
      if (n < seuil) /*  && arrp (err,0, A_ERR)->iShort != 0) */
	{ 
	  if (pp2 > pp) pp2 = pp ;
	  ok++ ;
	}
      arrayMax (vector) = old ;
    }
  arrayDestroy (dna) ;
  
  if (ok)
    { 
      bsAddData (obj, _Vector_Clipping, _Int, &pp1) ;
      bsAddData (obj, _bsRight, _Int, &pp2) ;
      bsAddKey (obj, _bsRight, vec) ;
      
      c1 = -1 ; c2 = 100000 ;
      bsGetData (obj, _Clipping, _Int, &c1) ;
      bsGetData (obj, _bsRight, _Int, &c2) ;
      if (c2 > pp2) c2 = pp2 ; 
      if (c1 < 0 || pp1 > c1 ) c1 = pp1 ;
      bsAddData (obj, _Clipping, _Int, &c1) ;
      bsAddData (obj, _bsRight, _Int, &c2) ;
      defCptForget (0, dnaKey) ;
    }
  else
    bsAddTag (obj, _Vector_Clipping) ; /* Prevent recursive search */
  
 abort:
  arrayDestroy (err) ;
  bsSave (obj) ;
  return ok ;
}
 
/***********************/

int trackVector (KEYSET ks, KEY key, BOOL force)
{ int n = 0, i = ks ? keySetMax (ks) : 0 ; 
  KEY *kp = i ? arrp(ks, 0, KEY) - 1 : 0 ;
  Array 
    v1 = arrayCreate (20, char),
    v2 = arrayCreate (20, char) ;

  if (!ks && key) { i =  1; kp = &key ; kp-- ; }
  while (kp++, i--)
    n += doTrackVector (*kp, v1, v2, force) ;
  arrayDestroy (v1) ;
  arrayDestroy (v2) ;
  return n ;
}
 
/***********************/

static int doTrackBadQuality (KEY seq)
{
  BOOL isBad = FALSE ;
  Array dna = dnaGet (seq) ;
  int i, j, max, NMAX=6 ;
  char *cp ;
  int nn[NMAX + 1] ; /* rolling position of last NMAX=6 + 1 n */
  /* with cp on 7th n, there is only 6 n up tocp ! */
  j = NMAX+1 ; while (j--) nn[j] = 0 ; 
  if (dna && (max = arrayMax(dna)))
    {
      isBad = TRUE ;
      for (i = 0, cp = arrp (dna, 0, char) ; 
	   isBad && i <= max ; cp++, i++)
	{
	  if (i < max)  /* we enter the loop once after the last letter */
	    switch (*cp)
	    {
	    case A_:
	    case T_:
	    case G_:
	    case C_:
	      continue ;
	    }
	  if (i - nn[NMAX] > 100)  /* rock  */
	    isBad = FALSE ;
	  else                         /* roll */
	    { j =  NMAX ;  while (j--) nn[j + 1] = nn[j] ;  nn[0] = i ; }
	}
    }

  arrayDestroy (dna) ;
  if (isBad)
    {
      OBJ Seq = bsUpdate (seq) ;
      KEY tag = str2tag ("Bad_quality") ;
      
      cp = "At least 6n inbest 100 bp" ;
      if (Seq)
	{
	  bsAddTag (Seq, tag) ;
	  if (bsFindTag (Seq, tag))
	    bsAddData (Seq, _bsRight, _Text, cp) ;
	  bsSave (Seq) ;
	}
    }

  return isBad ;
}

/***********************/

int trackBadQuality (KEYSET ks)
{ 
  int n = 0, i ;

  if (ks) 
    for (i = 0 ; i < keySetMax(ks) ; i++)
      if (doTrackBadQuality (keySet (ks, i)))
	n++ ;
  return n ;
}
 
/***********************/
  /* search for at least 8 contiguous matches */
BOOL baseCallTrackBout (Array dna1, Array dna2, int *x1p, int *x2p, int *sensp)
{ int i, j, x, p, mn, y1, y2 ;
  int max1 = arrayMax (dna1), max2 = arrayMax (dna2) ;
  char *cp, *cq, c ;
  
  if (max1 < 152 || max2 < 152) return FALSE ;
/* esaie 1 2 dans l'ordre */
  mn = y1 = y2 = 0 ; 
  for (x = max1 - 150 ; x < max1 - 8 ; x++)
    { p = 0 ; i = x ; j = 0 ;
      cp = arrp (dna1, i, char) ;
      cq = arrp (dna2, j, char) ;
      
      while (i++ < max1 && j++ < max2)
	{ c = *cp++ & *cq++ & 0x0f ;
	  if (c == N_) continue ;
	  else if (c) p++ ;
	  else 
	    { if (p > mn) 
		{ mn = p ; y1 = x ; y2 = 0 ; *sensp = 1 ; }
	      p = 0 ;
	    }
	}
      if (p > mn) 
	{ mn = p ; y1 = x ; y2 = 0 ; *sensp = 1 ; }
    }
	      
/* esaie 2 1 dans l'ordre */

  for (x = max2 - 150 ; x < max2 - 8 ; x++)
    { p = 0 ; i = 0 ; j = x ;
      cp = arrp (dna1, i, char) ;
      cq = arrp (dna2, j, char) ;
      
      while (i++ < max1 && j++ < max2)
	{ c = *cp++ & *cq++ & 0x0f ;
	  if (c == N_) continue ;
	  else if (c) p++ ;
	  else 
	    { if (p > mn) 
		{ mn = p ; y1 = 0 ; y2 = x ; *sensp = 1 ; }
	      p = 0 ;
	    }
	}
      if (p > mn) 
	{ mn = p ; y1 = 0 ; y2 = x ; *sensp = 1 ; }
    }
	      
/* esaie 1 anti-2 dans l'ordre */

  for (x = max1 - 150 ; x < max1 - 8 ; x++)
    { p = 0 ; i = x ; j = max2 - 1 ;
      cp = arrp (dna1, i, char) ;
      cq = arrp (dna2, j, char) ;
      
      while (i++ < max1 && j--)
	{ c = *cp++ & complementBase [*cq-- & 0x0f] ;
	  if (c == N_) continue ;
	  else if (c) p++ ;
	  else 
	    { if (p > mn) 
		{ mn = p ; y1 = x ; y2 = max2 - 1 ; *sensp = -1 ; }
	      p = 0 ;
	    }
	}
      if (p > mn) 
	{ mn = p ; y1 = x ; y2 = max2 - 1 ; *sensp = -1 ; }
    }
	      
/* esaie anti-2 1 dans l'ordre */

  for (x = 150 ; x > 8 ; x--)
    { p = 0 ; i = 0 ; j = x ;
      cp = arrp (dna1, i, char) ;
      cq = arrp (dna2, j, char) ;
      
      while (i++ < max1 && j--)
	{ c = *cp++ & complementBase [*cq-- & 0x0f] ;
	  if (c == N_) continue ;
	  else if (c) p++ ;
	  else 
	    { if (p > mn) 
		{ mn = p ; y1 = 0 ; y2 = x ; *sensp = -1 ; }
	      p = 0 ;
	    }
	}
      if (p > mn) 
	{ mn = p ; y1 = 0 ; y2 = x ; *sensp = -1 ; }
    }

  if (mn >= 12)
    { *x1p = y1 ; *x2p = y2 ; return TRUE ; }
  return FALSE ;
}

/***********************/
/***********************/
