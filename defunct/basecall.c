/*  File: basecall.c
 *  Author: Jean Thierry-Mieg (mieg@kaa.crbm.cnrs-mop.fr)
 *  Copyright (C) J Thierry-Mieg and R Durbin, 1994
 *-------------------------------------------------------------------
 * This file is part of the ACEDB genome database package, written by
 * 	Richard Durbin (MRC LMB, UK) rd@mrc-lmba.cam.ac.uk, and
 *	Jean Thierry-Mieg (CRBM du CNRS, France) mieg@kaa.cnrs-mop.fr
 *
 * Description:
 * Exported functions:
 * HISTORY:
 * Last edited: Mar 15 21:13 1995 (mieg)
 * Created: Fri Dec 23 16:43:01 1994 (mieg)
 *-------------------------------------------------------------------
 */

/*  $Id: basecall.c,v 1.2 1995-03-24 02:25:49 mieg Exp $ */

#define ARRAY_CHECK
#define MALLOC_CHECK

#include "acedb.h"
#include "basecall.h"
#include "a.h"
#include "lex.h"
#include "tags.h"
#include "classes.h"

#define seqMax(seq)  getNPoints(seq) 
#define seqMaxBase(seq)  (seq->NorigBases)

static void seq2ace(LANE *lane) ;


/***************************************************************************/


void baseCallGet (LANE *lane)
{ KEY  baseCallKey = lane->baseCallKey ;
  Array base = 0, basePos = 0, compressedBasecall = 0 ;
  int n ; short x , *bp , *bp1 ;
  char *b, *b1, *bc, c ;

  if (baseCallKey)
    compressedBasecall = arrayGet (baseCallKey, char, "c") ;

  if (compressedBasecall)
    { n = arrayMax (compressedBasecall) ;
      n /= 2 ;
      base = arrayCreate (n, char) ;
      array (base, n - 1, char) = 0 ;
      basePos = arrayCreate (n, short) ;
      array (basePos, n - 1, short) = 0 ;
      x = 0 ;
      b = arrp (base, 0, char) ;
      bp = arrp (basePos, 0, short) ;
      bc = arrp (compressedBasecall, 0, char) ;
      while (n--)
	{ x += *bc++ ;
	  *bp++ = x ;
	  *b++ = *bc++ ;
	}
    }
  else
    { n = seqMaxBase(lane->seq) ;
      base = arrayCreate (n, char) ;
      array (base, n - 1, char) = 0 ;
      basePos = arrayCreate (n, short) ;
      array (basePos, n - 1, short) = 0 ;
      b = arrp (base, 0, char) ;
      bp = arrp (basePos, 0, short) ;
      b1 = lane->seq->base ;
      bp1 = lane->seq->basePos ;
      x = - 1000 ;
      while (n--)
	{ if (x < *bp1)
	    x = *bp1++ ;
	  else
	    { x++ ; bp1++ ; }
	  *bp++ = x ;
	    
	  c = *b1++ ;
	  switch (c)
	    {
	    case 'A': c = A_ ; break ;
	    case 'T': c = T_ ; break ;
	    case 'G': c = G_ ; break ;
	    case 'C': c = C_ ; break ;
	    case 'a': c = BC_LOW | A_ ; break ;
	    case 't': c = BC_LOW | T_ ; break ;
	    case 'g': c = BC_LOW | G_ ; break ;
	    case 'c': c = BC_LOW | C_ ; break ;
	    case 'n': case '-': c = BC_LOW | N_ ;
	    }
	  *b++ = c ;
	}
      x = seqMax(lane->seq) ;
    }      
      
  lane->basePos = basePos ;
  lane->base = base ;
  lane->maxPos = x ;
  if (x > seqMax(lane->seq))
    x = seqMax(lane->seq) ;
  arrayDestroy (compressedBasecall) ;
}

/***************************************************/

void baseCallStore (LANE *lane)
{ KEY key = lane->key , baseCallKey = lane->baseCallKey ;
  OBJ obj ;
  Array base = lane->base, basePos = lane->basePos, baseCall = 0 ;
  int n ; short x ,dx, *bp , xmax = seqMax(lane->seq) ;
  char *b, *bc ;

  if (!arrayExists(base) || 
      !arrayExists(basePos) ||
      arrayMax(base) != arrayMax(basePos))
    messcrash("Problem in basecallStore") ;

  if (!baseCallKey)
    { obj = bsUpdate(key) ;
      if (!obj)
	return ; 
      lexaddkey (name(lane->key), &baseCallKey, _VBaseCall) ;
      bsAddKey (obj, _BaseCall, baseCallKey) ;
      bsSave (obj) ;
      lane->baseCallKey = baseCallKey ;
    }

  n = arrayMax (base) ;
  baseCall = arrayCreate (2*n, char) ;
  array (baseCall, 2*n - 1, char) = 0 ;
  x = 0 ;
  b = arrp (base, 0, char) ;
  bp = arrp (basePos, 0, short) ;
  bc = arrp (baseCall, 0, char) ;
  while (n-- && x < xmax)
    { dx = *bp - x ;
    /* these will smoothe away after a few steps */
      if (dx <= 0) dx = 1 ;
      if (dx > 254) dx = 254 ;
      *bc++ = dx ;
      x = *bp++ ;
      *bc++ = *b++ ;
    }

  arrayStore (baseCallKey, baseCall,"c") ;
  arrayDestroy (baseCall) ;
  messalloccheck() ;
}

/***************************************************/
/***************************************************/

BOOL baseCorrel(Seq seq1, int x1, BOOL direct,
	       Seq seq2, int x2, int ll, int nstep, int step, int *bestdxp)
{ 
  int
    x11 = x1 - ll/2, x21 = x2 - ll/2 ,
    x12 = x1 + ll/2, x22 = x2 + ll/2 ,
    dx, dxmax = nstep * step,
    i,
    max1 = seqMax(seq1) ,
    max2 = seqMax(seq2) , n, di = 1 ;
  double  s11, s22, s12 ;
  float z, bestz ;
  TRACE *bp1[4], *bp2[4], *ip1[4] , *ip2[4] ;
  int t1, t2 ;
  
  if (x11 < dxmax || x21 < dxmax || 
      x12 + dxmax >= max1 || x22 + dxmax >= max2)
    return FALSE ;

  bp1[0] = seq1->traceA ;
  bp1[1] = seq1->traceT ;
  bp1[2] = seq1->traceG ;
  bp1[3] = seq1->traceC ;
  
  if (direct)
    { bp2[0] = seq2->traceA ;
      bp2[1] = seq2->traceT ;
      bp2[2] = seq2->traceG ;
      bp2[3] = seq2->traceC ;
    }
  else
    { bp2[1] = seq2->traceA ;
      bp2[0] = seq2->traceT ;
      bp2[3] = seq2->traceG ;
      bp2[2] = seq2->traceC ;
      x21 = x22 ; di = -1 ;
    }

      
  bestz = - 1 ;
  for (dx = -dxmax ; dx < dxmax ; dx += step)
    {
      i = 4 ; while (i--) ip1[i] = bp1[i] + x11 ;
      i = 4 ; while (i--) ip2[i] = bp2[i] + x21 + dx ;
  
      n = ll ;
      s12 = s11 = s22 = 0 ;
      while (n--)
	{ i = 4 ; 
	  while(i--) 
	    { t1 = *ip1[i]++ ;
	      t2 = *ip2[i] ; ip2[i] += di ;
              s11 += t1 * t1 ; s22 += t2 * t2 ; s12 += t1 * t2 ;
	    }
	}
      if (s12 > 0)
	{ z = s12*s12 ; /* /(s11*s22) ; */
	  if (z > bestz)
	    { *bestdxp = dx ; bestz = z ;
	    }
	}
    }
  return bestz > 0 ? TRUE : FALSE ;
}

/***************** energy ***********************************/
/*
static Array seqFindEnergy(Seq seq, int min, int max)
{
  TRACE *a, *t, *g, *c ; int i ; int e ; 
  Array energy ;

  if (min < 0 || max > seqMax(seq))
       return 0 ;
 
  energy = arrayCreate(seqMax(seq), int) ;
  
  a = seq->traceA + min ; 
  t = seq->traceT + min ;  
  g = seq->traceG + min ;  
  c = seq->traceC + min ;  
  
  array(energy,max-min, int) = 0 ; // make room 
  for (i= min; i< max ; i++, a++, t++, g++, c++)
    {
      e = (int)*a * *a  + (int)*t * *t + (int)*g * *g +(int)*c * *c ;
      arr(energy, i - min, int) = e ;

    }
  return energy ;
}
*/
/*************/
 /* carre de la derivee de l'enveloppe */
int seqEnergyOfDerivee (Seq seq, int min, int max)
{
  TRACE *a, *t, *g, *c, a0, t0, g0, c0, y ; 
  int x, i, e = 0 ;

  if (min < 0 || max > seqMax(seq))
       return 0 ;
 
  a = seq->traceA + min ; 
  t = seq->traceT + min ;  
  g = seq->traceG + min ;  
  c = seq->traceC + min ;  
  
  a0 = *a ; t0 = *t ; g0 = *g ; c0 = *c ;

  for (i= min; i< max ; i++, a++, t++, g++, c++)
    { y = 0 ;
      if (*a > y) x = *a - a0 ; 
      if (*t > y) x = *t - t0 ; 
      if (*g > y) x = *g - g0 ; 
      if (*c > y) x = *c - c0 ; 
      e+= x*x ;
      a0 = *a ; t0 = *t ; g0 = *g ; c0 = *c ;
    }
  return e ;
}

/***************************************************/
/***************************************************/
/*********** extrema ************************************/

static void findMaxima(TRACE *bp, int max, int delta, Array xx)
{ short d = delta ; TRACE lasty = *bp, *bp1 ;
  int i, i1, j = 0, lastx = 0 ;
  BOOL up ;
  
  for (i = 0 ; i < max ; i++, bp++) 
    {
      if (up)
	{ if (*bp < lasty - d)
	    { up = FALSE ;
	      bp1 = bp - i + lastx ; i1 = 0 ;
	      while (*++bp1 == lasty) i1++ ;
	      lastx += i1/2 ;  /* middle of a saturation */
	      array(xx, j, Extremum).x = lastx ;
	      arr(xx, j, Extremum).y = lasty ;
	      j++ ;
	      lastx = i ; lasty = *bp ;
	    }
	  if (*bp > lasty)
	    { 
	      lastx = i ; lasty = *bp ;
	    }
	}
      else
	{ if (*bp > lasty + d)
	    { up = TRUE ;
/*	don t store the minima
              array(xx, j, Extremum).x = lastx ;
	      arr(xx, j, Extremum).y = lasty ;
	      j++ ;
*/
	      lastx = i ; lasty = *bp ;
           }
	  if (*bp < lasty)
	    { 
	      lastx = i ; lasty = *bp ;
	    }
	}
    }
}

/************/

static BOOL find4Extrema(LANE *lane, Array aExtrema[])
{ TRACE *bp[4], yy, y[4] ;
  int x, i, i1, j, k, max, delta ;  
  Extremum *e1, *e2 ;

  delta = 5 ;
  if(!lane->seq)
    return FALSE ;

  
  max = seqMax(lane->seq) ;

  bp[0] = lane->seq->traceA ; 
  bp[1] = lane->seq->traceG ; 
  bp[2] = lane->seq->traceC ; 
  bp[3] = lane->seq->traceT ; 

  for (i=0 ; i<4 ; i++)
    { aExtrema[i] = arrayReCreate(aExtrema[i], 
				  100, Extremum) ;
      findMaxima(bp[i], max, delta, aExtrema[i]) ;
    }
  for (i=0 ; i<4 ; i++)
    { 
      for (j = 0 , k = 0, e1 = e2 = arrp(aExtrema[i], 0, Extremum) ;
	   j < arrayMax(aExtrema[i]) ; j++, e1++)
	{ x = e1->x ;
	  for (i1 = 0 ; i1 < 4 ; i1++)
	    y[i1] = *(bp[i1] + x)  ;
	  yy = y[i] ;
	  for (i1 = 0 ; i1 < 4 ; i1++)
	    if (yy < y[i1])
	      goto forget ;
	  if (e1 != e2)
	    *e2 = *e1 ; 
	  e2++ ; k++ ;
	forget:
	  continue ;
	}
      arrayMax(aExtrema[i]) = k ;
    }  
  return TRUE ;
}

/*
static int gg[] = 
{ 2, 6, 13, 27, 48, 72, 92, 100, 92, 72, 48, 27, 13, 6, 2 } ;
*/

#define contrib(_hh, _z) \
  { int icc = 15 ; \
    int *_ggp = *gg[0], *_ih = arrayp(_hh, _z + 7, int) ; \
    while (_icc--) *(_ih--) += *_ggp++ ; \
  }

static void baseCallAdd (LANE *lane, Array bc, int dx)
{ int reject, 
    n, ni, i, ibc, i1, j1, x, x0, x1, x2, y, yy, dx1, dx2, ddx ;
  BASECALL *bb, *bb1 ;
  TRACE *bp[4], *tp ;

  bp[0] = lane->seq->traceA ; 
  bp[1] = lane->seq->traceG ; 
  bp[2] = lane->seq->traceC ; 
  bp[3] = lane->seq->traceT ; 

  for (ibc = 0 ; ibc < arrayMax(bc) - 1 ; ibc++)  /* will stop sur avant dernier */
    { bb = arrp (bc, ibc, BASECALL) ;
      x1 = bb->x ; x2 = (bb + 1)->x ; 
      ni = (x2 - x1 + .3  * dx) / dx ; /* nb of steps in interval */
      if (ni < 2)
	continue ;

      yy = 0 ; j1 = 0 ;

      reject = 7 ;
    tryotherpeak:
      x = x1 + (x2 - x1) / ni ;
      for (i = x - .35 * dx ; i <= x + .35 * dx ; i++)
	{ i1 = 4 ;
	  while (i1--) /* find highest trace */
	    { if (i1 == reject) continue ;
	      y = *(bp[i1] + i) ;
	      if (yy < y) { yy = y ; j1 = i1 ; x0 = i ; }
	    }
	}
      i1 = ibc - 16 ; if (i1 < 0) i1 = 0 ;
      for (y = yy, n = 1, i = i1 ; i < arrayMax(bc) && n < 5 ; i++)
	{ if (arrp(bc, i, BASECALL)->t == j1)
	    { y += arrp(bc, i, BASECALL)->y ; n++ ; }
	}
      y /= n ; /* hauteur moyenne des pics  */
      if (yy * 4 <  y) /* not high enough to keep it */
	{ if (reject == 7)
	    { reject = j1 ;
	      goto tryotherpeak ; /* may be second peak is good */
	    }
	  if (x2 - x1 < 2.11 * dx)
	    continue ; /* no real peak here */
	  j1 = reject ; /* well, there is nothing better and i want one */
	}
      if (!yy || yy * 8 <  y) /* really not high enough to keep it */
	continue ;
      /* check that curvature is negative */
      
      i = x - .33 * dx ; /* centre on middle not on highest point */
      tp = bp[j1] + i ; x = i - 1 ;
      dx1 = *tp - *(tp - 1) ;
	  i = .66  * dx ;
      ddx = 0 ;
      while (tp++, x++, i--)
	{ dx2 = dx1 ;
	  dx1 = *tp - *(tp - 1) ;
	  if (dx1 -  dx2 < ddx)
		{ ddx = dx1 - dx2 ; x0 = x ; }
	}
      
      if (ddx < 0 || ni > 2  || (x2 -  x1 > 2.1 * dx))
	{
	  /* insertion */
	  i = arrayMax(bc) ;
	  bb1 = arrayp (bc, i, BASECALL) ; /* increases arrayMax */
	      while (--i > ibc) { *bb1 = *(bb1 - 1) ; bb1-- ;}
	  bb = arrp (bc, ibc, BASECALL) ;
	  bb1 = bb + 1 ;
	  bb1->x = x0 ; bb1->y = yy ; 
	  bb1->t = j1 ; bb1->flag = BC_LOW ;
	  if (bb->x >= bb1->x) invokeDebugger() ;
	}
    }
}

static void baseCallSuppress (Array bc, int dx)
{ int n, i, ibc, i1, j1, x1, x2, y, yy ;
  BASECALL *bb, *bb1 ;

  for (ibc = 1 ; ibc < arrayMax(bc) - 1 ; ibc++)  /* will stop sur avant dernier */
    { bb = arrp (bc, ibc, BASECALL) ;
      x1 = (bb - 1)->x ; x2 = (bb + 1)->x ;
      if (x2 - x1 < 1.6 * dx)
	{ yy = bb->y ; j1 = bb->t ;
	  i1 = ibc - 16 ; if (i1 < 0) i1 = 0 ;
	  for (y = yy, n = 1, i = i1 ; i < arrayMax(bc) && n < 5 ; i++)
	    { if (arrp(bc, i, BASECALL)->t == j1)
		{ y += arrp(bc, i, BASECALL)->y ; n++ ; }
	    }
	  y /= n ; /* hauteur moyenne des pics  */
	  if (yy * 3 < 0) /* low enough to remove */
	    { /* deletion */
	      i = arrayMax(bc) - ibc - 1 ;
	      bb1 = bb ;
	      while (i--) { *bb1 = *(bb1 + 1) ; bb1++ ; }
	      arrayMax(bc)-- ;
	    }
	  else
	    bb->flag |= BC_LOW ;
	}
    }
}

BOOL findBaseCall (LANE *lane)
{ int x, mx[4], i, jbc, i1, dx ;
  Extremum *ep[4] ;
  Array bc, aExtrema[4] ; 
  BASECALL *bb ;

  if (arrayExists(lane->baseCall))
    return FALSE ;
  bc = lane->baseCall = arrayCreate (1000, BASECALL) ;
  jbc = 0 ;

  i = 4 ;
  while (i--)
    aExtrema[i] = arrayCreate (1000, Extremum) ;

  if (!find4Extrema(lane, aExtrema))
    return FALSE ;
  
  i = 4 ;
  while (i--)
    { mx[i] = arrayMax (aExtrema[i]) ;
      ep[i] = arrp (aExtrema[i], 0, Extremum) ;
    }
 encore:
  x = 1000000 ; i1 = - 1 ;
  i = 4 ;
  while (i--)
    { if (x > ep[i]->x && mx[i] > 0)
	{ x = ep[i]->x ; i1 = i ; }
    }
  if (i1 >= 0)
    { bb = arrayp(bc, jbc++, BASECALL) ;
      bb->t = i1 ; bb->x = ep[i1]->x ; bb->y = ep[i1]->y ; 
      ep[i1]++ ; mx[i1]-- ;
      if (jbc > 1 && bb->x < (bb - 1)->x)
      invokeDebugger() ;
      goto encore ;
    }
  i = 4 ;
  while (i--)
    arrayDestroy (aExtrema[i]) ;
  
  i = arrayMax(bc) > 300 ? 300 : arrayMax(bc) - 1 ;
  if (!i)
    return FALSE ;
  dx = arr(bc, i, BASECALL).x / i ;

  i1 = seqMax(lane->seq) ;
  i = arrayMax(bc) - 1 ;
  bb = arrp(bc, 0, BASECALL) - 1 ;
  while (bb++, i--)
    if (bb->x > i1 || bb->x > (bb + 1)->x)
      invokeDebugger() ;
  baseCallAdd (lane, bc, dx) ;
  i = arrayMax(bc) - 1 ;
  bb = arrp(bc, 0, BASECALL) - 1 ;
  while (bb++, i--)
    if (bb->x > i1 || bb->x > (bb + 1)->x)
      invokeDebugger() ;

  baseCallSuppress (bc, dx) ;
  i = arrayMax(bc) - 1 ;
  bb = arrp(bc, 0, BASECALL) - 1 ;
  while (bb++, i--)
    if (bb->x > i1 || bb->x > (bb + 1)->x)
      invokeDebugger() ;

  return TRUE ;
}

/***************************************************/
/***************************************************/

/***************************************************/
/***************************************************/

/*********************************************************/

void  laneMakeErrArray(LOOK look, LANE *lane)
{ int x1, x2, x3, ctop, cend, c3, n1, n2, i ;
  Array err1, err2 ;

  arrayDestroy (lane->errArray) ;

  if (!lane->dna)
    { lane->errArray = arrayCreate (12, A_ERR) ;
      return ;
    }
  if (lane->x1 < lane->x2)
    { x1 = lane->x1 ; ctop = lane->clipTop ;
      x3 = lane->x3 - 1 ; c3 = lane->clipExtend - 1 ;  
      lane->errArray = 
	dnaAlignCptErreur (look->dna, lane->dna, 
			   &x1, &x3,
			   &ctop, &c3) ;
      lane->x3 = x3 + 1 ;
      lane->clipExtend = c3 + 1 ;
    }
  else
    { x3 = lane->x3 + 1 ; c3 = lane->clipExtend - 1 ;  
      x2 = lane->x2 ; cend = lane->clipEnd ;
      if (x3 < 0)
	{ c3 += x3 ; x3 = 0 ; } 
        /* i compute in 2 parts becasause the clipped errors
	   may screw up the error tracking in the good region */
      if (x3 < x2)
	err1 = 
	  dnaAlignCptErreur (look->dna, lane->dna, 
			     &x3, &x2,
			     &c3, &cend) ;
      else
	err1 = arrayCreate (12, A_ERR) ;	
      x2 = lane->x2 + 1 ; cend = lane->clipEnd - 1 ; /* may have moved, don t trust it */
      x1 = lane->x1 ; ctop = lane->clipTop ;
      if (x2 < 0)
	{ cend += x2 ; x2 = 0 ; } 
      
      err2 = 
	dnaAlignCptErreur (look->dna, lane->dna, 
			   &x2, &x1,
			   &cend, &ctop) ;
      lane->clipTop = ctop ; lane->x1 = x1 ;
      lane->errArray = err1 ;
      n1 = arrayMax(err1) ; n2 = arrayMax(err2) ;
      for (i = 0 ; i < n2 ; i++)
	array(err1, n1 + i, A_ERR) = arr(err2, i, A_ERR) ;
      arrayDestroy (err2) ;
    }
  if (!lane->errArray)
    messcrash("") ;
}

/*******************************************************************************/

static BOOL patchError (Array dna, LANE *lane, A_ERR *ep, Array err)
{ int 
    t, i, nb, ns, jmax,
    c0,  /* in dna */
    s0, s1, s2, /* in lane base */
    j0, /* in basecall */
    x0, x1, x2 ;
  Array 
    bc = lane->baseCall, 
    base = lane->base, 
    basePos = lane->basePos ;
  char cc , *cp ;
  short *posp ;
  BASECALL *bb, *bb1, *bb2 ;
  
  c0 = ep->iLong ; 
  s0 = s1 = s2 = ep->iShort ; s1-- ; s2++ ;
  if (s1 < 5 || s2 > arrayMax(basePos) - 10 ||
      c0 < 0 || c0 >= arrayMax (dna))
    return FALSE ;
  x1 = arr(basePos, s1, short) ;
  x0 = arr(basePos, s0, short) ;
  x2 = arr(basePos, s2, short) ;
  
    /* search for the last base left of x0 */
  j0 = 0 ; jmax = arrayMax (bc) ;
  bb = arrp (bc, j0, BASECALL) ;
  while (j0 < jmax && bb->x < x0)
    { j0 += 20 ; bb += 20 ; }
  if (j0 > 0)
    { bb -= 20 ; j0 -= 20 ; }
  while (j0 < jmax && bb->x < x0)
    { j0 ++ ; bb++ ;}
  if (j0 > 0)
    { bb -- ; j0-- ;}


  switch (ep->type)
    {
    case AMBIGUE: 
    case ERREUR: 
      cc =  arr(dna, c0, char) ;
      switch (cc)
	{ 
	case A_: t = 0 ; break ;
	case G_: t = 1 ; break ;
	case C_: t = 2 ; break ;
	case T_: t = 3 ; break ;
	default: return FALSE ;
	}
      
      x1 = x1 + (x0 - x1)/2 ;
      x2 = x0 + (x2 - x0)/2 ;
      bb-- ;
      while (bb++, bb->x < x2)
	if ( bb->x > x1 &&
	    bb->x < x2 &&
	    (bb - 1)->x < x1 &&
	    (bb + 1)->x > x2 &&
	    bb->t == t)
	  { arr (base, s0, char) = BC_ACE | cc ;
	    arr (basePos, s0, short) = bb->x ;
	    return TRUE ;
	  }
      break ;
    case INSERTION_DOUBLE: /*_AVANT: */
      break ;
    case INSERTION: /*_AVANT: */
      cc =  arr(base, s0, char) & 0x0f ;
      switch (cc)
	{ 
	case A_: t = 0 ; break ;
	case G_: t = 1 ; break ;
	case C_: t = 2 ; break ;
	case T_: t = 3 ; break ;
	default: return FALSE ;
	}
      while (cc ==  (arr(base, s1, char) & 0x0f) ||
	     N_ ==  (arr(base, s1, char) & 0x0f) ) s1-- ;
      while (cc ==  (arr(base, s2, char) & 0x0f) ||
	     N_ ==  (arr(base, s2, char) & 0x0f) ) s2++ ;
      ns = s2 - s1 - 1 ; /* nb de bases identiques dans short */
      bb1 = bb ; bb2 = bb + 1 ;
      while (bb1->t == t) bb1-- ;
      while (bb2->t == t) bb2++ ;
      nb = bb2 - bb1 - 1 ;
      if (nb != ns - 1) /* i have one less so i will delete base s2 - 1 */
	return FALSE ;
            /* reposition */
      if (arr (basePos, s1 - 1, short) < bb1->x &&
	  (bb2+1)->x < arr (basePos, s2 + 1 , short))
	{ while (s1 < s2) 
	    arr (basePos, s1++, short) = (bb1++)->x ;
	  arr (basePos, s2, short) = bb2->x ;
	}
            /* delete */
      cp = arrp (base, s2 - 1, char) ;
      posp = arrp (basePos, s2 - 1, short) ;
      i = arrayMax(base) - s2 ;
      while (i--)
	{ *cp = *(cp + 1) ; *posp = *(posp + 1) ; posp++, cp++ ; }
      arrayMax(base)-- ;
      arrayMax(basePos)-- ;
      if (s2 - 1 < lane->clipTop)
	lane->clipTop-- ;
      if (s2 < lane->clipEnd)
	lane->clipEnd-- ;
      if (s2 < lane->clipExtend)
	lane->clipExtend-- ;
            /* update the error table */
      ep = arrp(err, 0, A_ERR) - 1 ;
      i = arrayMax(err) ;
      while (ep++, i--)
	if (ep->iShort > s0)
	  ep->iShort-- ;	
      return TRUE ;

    case TROU_DOUBLE:
      break ;
    case TROU:
      cc =  arr(dna, c0, char) ;
      switch (cc)
	{ 
	case A_: t = 0 ; break ;
	case G_: t = 1 ; break ;
	case C_: t = 2 ; break ;
	case T_: t = 3 ; break ;
	default: return FALSE ;
	}
      s2 = s0 ; /* since s0 is probably != cc already */
      while (cc ==  (arr(base, s1, char) & 0x0f) ||
	     N_ ==  (arr(base, s1, char) & 0x0f) ) s1-- ;
      while (cc ==  (arr(base, s2, char) & 0x0f) ||
	     N_ ==  (arr(base, s2, char) & 0x0f) ) s2++ ;
      ns = s2 - s1 - 1 ; /* nb de bases identiques dans short */
      bb1 = bb - 1 ; bb2 = bb ;
      while (bb1->t == t) bb1-- ;
      while (bb2->t == t) bb2++ ;
      nb = bb2 - bb1 - 1 ;
      if (nb != ns + 1) /* i have one more so i will insert left of s2 */
	return FALSE ;
                /* insert */
      cp = arrayp (base, arrayMax(base), char) ;
      posp = arrayp (basePos, arrayMax(basePos), short) ;
      i = arrayMax(base) - s2 - 1 ;
      while (i--)
	{ *cp = *(cp - 1) ; *posp = *(posp - 1) ; posp--, cp-- ; }
      array (base, s2, char) = cc ;
      array (basePos, s2, short) = 
	(arr(basePos, s2 - 1, short) + arr(basePos, s2 + 1, short))/2  ;
              /* reposition  after inserting */
      if (arr (basePos, s1 - 1, short) < bb1->x &&
	  (bb2 + 1)->x < arr (basePos, s2 + 2 , short))
	{ while (s1 <= s2 + 1) 
	    arr (basePos, s1++, short) = (bb1++)->x ;
	}
               /* update the error table */
      ep = arrp(err, 0, A_ERR) - 1 ;
      i = arrayMax(err) ;
      while (ep++, i--)
	if (ep->iShort > s0)
	  ep->iShort++ ;	
      if (s2 < lane->clipTop)
	lane->clipTop++ ;
      if (s2 <= lane->clipEnd)
	lane->clipEnd++ ;
      if (s2 <= lane->clipExtend)
	lane->clipExtend++ ;

      return TRUE ;
    }
  return FALSE ;
}

/***************************************************/

BOOL baseCallPatchLane (Array dnaDirect, LANE *lane)
{ BOOL f1, f2 = FALSE ;
  Array dna, myReverse = 0 , err = 0 ;
  A_ERR *ep ;
  int i, x1, x3, ctop, c3, amax = arrayMax(dnaDirect) ;

  if (!lane->scf) traceGetLane (0, lane) ;
  if (lane->scf != 2)
    return FALSE ;
  if (! arrayExists(lane->baseCall) &&
      ! findBaseCall (lane))
    return FALSE ;
 
  if (! arrayMax(lane->baseCall))
    return FALSE ;

  if (lane->x1 < lane->x2)
    dna = dnaDirect ;
  else
    { myReverse = arrayCopy (dnaDirect) ;
      reverseComplement (myReverse) ;
      dna = myReverse ;
      if (lane->x1 > amax)
	return FALSE ;
      lane->x1 = amax - lane->x1 - 1 ; 
      lane->x2 = amax - lane->x2 - 1 ; 
      lane->x3 = amax - lane->x3 - 1 ; 
    }

  f1 = TRUE ;
  while (f1)
    { f1 = FALSE ;
      x1 = lane->x1 ; ctop = lane->clipTop ;
      x3 = lane->x3 - 1 ; c3 = lane->clipExtend - 1 ;  
      if (x3 >= arrayMax(dnaDirect))
	x3 = arrayMax(dnaDirect) - 1;
	err = dnaAlignCptErreur (dna, lane->dna, 
				 &x1, &x3,
				 &ctop, &c3) ;
      if (!err || ! arrayMax(err))
	break ;
  
      lane->x1 = x1 ;
      lane->x3 = x3 + 1 ;
      lane->clipTop = ctop ;
      lane->clipExtend = c3 + 1 ;
  
      for (i = 0, ep = arrp(err, 0, A_ERR) ; i < arrayMax(err) ; ep++, i++)
	{
	  if (ep->iLong >= arrayMax(dna))
	    break ;
	  f1 |= patchError (dna, lane, ep, err) ;
	}
      seq2ace (lane) ;
      arrayDestroy (err) ;
      f2 |= f1 ;
    }

  if (myReverse)
    { arrayDestroy (myReverse) ;
      lane->x1 = amax - lane->x1 - 1 ; 
      lane->x2 = amax - lane->x2 - 1 ; 
      lane->x3 = amax - lane->x3 - 1 ; 
    }
  arrayDestroy (err) ;
  return f2 ;
}

/***************************************************/ 

static void seq2ace(LANE *lane) 
{ char *cp ;
  int n = arrayMax (lane->base) ;
  if (n <= 0)
    return ;
  arrayDestroy (lane->dna) ;
  lane->dna = arrayCopy (lane->base) ;
  cp = arrp(lane->dna, 0, char) - 1 ;

  while (cp++, n--)
    *cp &= 0x0F ;
}

/***************************************************/ 

void findXclipping (LANE *lane)
{ int n, nBase, top, end, extend ;
  short *basePos ;

  if (lane->clipEnd > arrayMax (lane->dna))
    lane->clipEnd = arrayMax (lane->dna) ;

  top = lane->clipTop ;
  end = lane->clipEnd ;
  extend =  end + 20 ;
  if (extend >= lane->maxPos)
    extend = lane->maxPos ;
  lane->clipExtend = extend ;
  lane->x3 = lane->x2 + (lane->x1 < lane->x2 ? extend - end : end - extend) ;

  n = 1 ; nBase = arrayMax (lane->base) - 2 ;
  basePos = arrp (lane->basePos, 2, short) ;

  if (top)
    { while(n < top && nBase)
	{ basePos++  ; nBase-- ; n++ ;
	}
      lane->xClipTop = (*(basePos - 1) + *(basePos - 2))/ 2 ;
    }
  else
    lane->xClipTop = 0 ;
  while(n < end && nBase)
    { basePos++  ; nBase-- ; n++ ;
    }
  lane->xClipEnd = (*(basePos - 2) + *(basePos - 1))/ 2 ;
  while(n < extend)
    { basePos++  ; nBase-- ; n++ ;
    }
  lane->xClipExtend = (*(basePos - 2) + *(basePos - 1))/ 2 ;
}

/***************************************************/ 

void laneEditSave (LOOK look, LANE *lane)
{ OBJ obj ;
  int x, n ;
  
  Array a ;
  KEY dnaKey ;
  
  seq2ace (lane) ;
#ifndef NON_GRAPHIC
  if (look && graphExists(look->fMapGraph))
    { Graph old = graphActive() ;
      graphActivate (look->fMapGraph) ;
      fMapTraceForget(lane->key) ;
      graphActivate (old) ;
    }
#endif

  dnaAlignForget (lane->key) ;

  if (obj = bsUpdate(lane->key))
    { if (bsGetKey (obj, _DNA, &dnaKey))
	{ a = arrayCopy (lane->dna) ;
	  n = arrayMax (a) ;
	  dnaStoreDestroy (dnaKey, a) ;
	  bsAddData (obj, _bsRight, _Int, &n) ;
	  x = lane->clipTop + 1 ;
	  bsAddData (obj, _Clipping, _Int, &x) ;
	  x = lane->clipEnd ;
	  bsAddData (obj, _bsRight, _Int, &x) ;
	}
      bsSave (obj) ;
    }
  findXclipping (lane) ; /* to readjust the clipping */

  if (look)
    { if (obj = bsUpdate(look->key))
	{ if (bsFindKey (obj, _Assembled_from, lane->key))
	    { x = lane->x1 + 1 ;
	      bsAddData (obj, _bsRight, _Int, &x) ;
	      x = lane->x2 + 2 ;
	      bsAddData (obj, _bsRight, _Int, &x) ;
	    }
	  bsSave (obj) ;
	}    
      laneMakeErrArray(look, lane) ;
    }
  baseCallStore (lane) ;
}

/***************************************************/ 

BOOL baseCallPatchContig (KEY contig)
{ OBJ obj = bsUpdate(contig) ;
  Array aa, dna = 0 , dna2 = 0 ;
  int i, amax ;
  KEY dnaKey ;
  LANE *lane ;

  if (!obj ||
      !bsFindTag (obj, _Assembled_from) || 
      !bsGetKey (obj, _DNA, &dnaKey) ||
      !(dna = dnaGet (dnaKey)))
    { bsDestroy (obj) ;
      return FALSE ;
    }
  
  mainActivity ("Autoedit") ;
  aa = arrayCreate (100, BSunit) ;
  bsFindTag (obj, _Assembled_from) ;
  bsFlatten (obj, 3, aa) ;
  bsSave (obj) ;

  lane = (LANE*) messalloc (sizeof (struct LaneStruct)) ;

  for (i = 0 ; i < arrayMax(aa) ; i+= 3)
    { 
      lane->key = arr(aa,i, BSunit).k ;
      lane->x1 = arr(aa,i + 1, BSunit).i - 1;
      lane->x2 = arr(aa,i + 2, BSunit).i ;
      lane->x3 = lane->x2 ;
      if (lane->x1 >= lane->x2)
	continue ;

      if (traceGetLane (0, lane))
	{ lane->x2-- ;
	  dnaAlignRecale(dna, &(lane->x1), &(lane->x2),
		     lane->dna, lane->clipTop, lane->clipEnd - 1 ) ;
	  lane->x2++ ;
	  findXclipping (lane) ; /* to adjust the clipping */
	  if (baseCallPatchLane (dna, lane))
	    laneEditSave (0, lane) ;
	}
      laneDestroy (lane) ;
    }
  
  reverseComplement (dna) ;
  dna2 = arrayCopy (dna) ;
  amax = arrayMax (dna) ;
  for (i = 0 ; i < arrayMax(aa) ; i+= 3)
    { amax = arrayMax (dna) ;
      lane->key = arr(aa,i, BSunit).k ;
      lane->x1 = arr(aa,i + 1, BSunit).i - 1 ;
      lane->x2 = arr(aa,i + 2, BSunit).i - 2 ;
      lane->x3 = lane->x2 ;
      if (lane->x1 <= lane->x2)
	continue ;

      lane->x1 = amax - lane->x1 - 1 ; 
      lane->x2 = amax - lane->x2 - 1 ; 
      lane->x3 = amax - lane->x3 - 1 ; 
      if (traceGetLane (0, lane))
	{ lane->x2-- ;
	  dnaAlignRecale(dna, &(lane->x1), &(lane->x2),
		     lane->dna, lane->clipTop, lane->clipEnd - 1) ;
	  lane->x2++ ;
	  findXclipping (lane) ; /* to adjust the clipping */
	  if (baseCallPatchLane (dna, lane))
	    { lane->x1 = amax - lane->x1 - 1 ; 
	      lane->x2 = amax - lane->x2 - 1 ; 
	      lane->x3 = amax - lane->x3 - 1 ; 
	      laneEditSave (0, lane) ;
	    }
	}
      laneDestroy (lane) ;
    }
  
  arrayDestroy (aa) ;
  arrayDestroy (dna) ;
  arrayDestroy (dna2) ;

  messfree (lane) ;
/*  dnaAlignFixContig (contig) ; */
  return TRUE ; /* ulrich */
}

/*****************************************************/
/*****************************************************/

