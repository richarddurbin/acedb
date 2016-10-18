/*  File: fmaposp.c
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
 * Description: Interface to OSP, Oligo selection program
 * HISTORY:
 * Last edited: Oct 20 15:41 2005 (edgrif)
 * Created: Tue Oct 7 1996 (mieg)
 * CVS info:   $Id: fmaposp.c,v 1.60 2012-10-30 09:56:38 edgrif Exp $
 *-------------------------------------------------------------------
 */

/*
#define ARRAY_CHECK
#define MALLOC_CHECK
*/

#include "acedb.h"
#include "aceio.h"
#include "call.h"
#include "bump.h"
#include "whooks/systags.h"
#include "whooks/tags.h"
#include "lex.h"
#include "dna.h"
#include "query.h"
#include "session.h"
#include "parse.h"
#include "pick.h" 
#include "dbpath.h"

#include "display.h"
#include "keysetdisp.h" 
#include "main.h" 

#include <w7/fmap_.h>

/************************************************************/

#define OSP_S_BIT 0x8000  /* used as a number in fmapselectbox */
#define OSP_O_BIT 0x4000  /* used as a number in fmapselectbox */

static void ospSubmitPairs (void) ;
static void fMapOspControl (void) ;
static void fMapOspControlDraw (void) ;
static void ospRedraw (void) ;

static Graph ospControlGraph = 0 ;
static Array selectedOligos = 0 ;

static int 
  oligoMaxScore = 16,
  oligoMinLength = 18, 
  oligoMaxLength = 20, 
  oligoTmMin = 49, 
  oligoTmMax = 52, 
  productMinLength = 1000, 
  productMaxLength = 2000,
  productMaxScore = 80,
  scoreLimit = 10,
  fillLimit = 75 ;
static int oligoNameBox = 0 ;

/******************************/

static Array mySegs = 0 ;	/* Array of SEG - global because used
				   by arraySort function for Array aa
				   of segment scores.
				   This will become look->segs */

static int ospScoreOrder (void *a, void *b)
{ 
  int x = *(int*) a, y = *(int*) b ;
  SEG *seg1 =  arrp(mySegs,x,SEG), *seg2 =  arrp(mySegs,y,SEG) ;

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
  int f1 = seg1->data.i & 0xfff, f2 = seg2->data.i  & 0xfff ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */
  int f1 = seg1->feature_data.oligo_type & 0xfff, f2 = seg2->feature_data.oligo_type  & 0xfff ;

  if (x <= 0 || x > arrayMax(mySegs) ||
      y <= 0 || y > arrayMax(mySegs))
    messcrash("ospScoreOrder") ;
 
  if (f1 * f2 > 0)
    return f1 > f2 ? 1 : -1 ;

  return 1 ;
} /* ospScoreOrder */

static int intOrder(void *a, void *b)
{
  return
    (int)  ( (*(int *) a) - (*(int *) b) ) ;
} /* intOrder */

/* return active oligos exactly in correct order to draw them */
static KEYSET getActiveOligos (FeatureMap look, int from, int to)
{
  int try, i, j, jj, j1, dummy,  x1, x2,  ln;
  int iSeg, nf, nks = 0, maxMask, beginMask, showUp ;
  SEG *seg ; 
  KEYSET ks = 0 ;
  BOOL up , selected, found ;
  int ss, tm ;
  Array aa = arrayCreate(200, int) ;
  unsigned char *mask = 0 , *maskup = 0 , *mymask ;
  unsigned char j2 ;
  int ddx = to - from ;
  
  if (ddx < 3*fillLimit)
    ddx = 3*fillLimit ;

  if (ddx <10)
    return aa ;

  beginMask = from - ddx/2 ; maxMask = 2 * ddx ;
  maxMask /= 8 ;

  if (maxMask <= 32)
    maxMask = 32 ;

  mask = (unsigned char*) messalloc(maxMask  + 8) ;
  maskup = (unsigned char*) messalloc(maxMask  + 8) ;

  found = FALSE ;
  for (iSeg = 1; iSeg <  arrayMax(look->segs) ; iSeg++)
    {
      seg = arrp(look->segs,iSeg,SEG) ;

      if  ((seg->type | 0x1) != OLIGO_UP)
	continue ;
      if (!seg->x1 || !seg->x2)
	continue ;

      x1 = seg->x1 ; x2 = seg->x2 ;
      ln = x2 - x1 + 1 ;

      if ( ln <   oligoMinLength ||
	   ln >   oligoMaxLength)
	continue ;


#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
      tm = seg->data.i >> 16 & 0x3ff ; 
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */
      tm = seg->feature_data.oligo_type >> 16 & 0x3ff ; 

      if (tm && (tm < 10 * oligoTmMin || tm > 10 * oligoTmMax))
	continue ;

      found = TRUE ;

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
      ss =  seg->data.i ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */
      ss = seg->feature_data.oligo_type ;

      selected = ss &  OSP_S_BIT ? TRUE : FALSE ;
      up = seg->type & 0x1 ? TRUE : FALSE ;
      if (x1 > to + ddx/2 || x2 < from - ddx/2)
	continue ;
      ss &= 0xfff ;
      if (ss > scoreLimit - .1) 
	{
	  array(aa, arrayMax(aa), int) = iSeg ;
	  continue ;
	} 

       /* now mask */
      mymask = up ? maskup : mask ;
      for (i = seg->x1 - fillLimit ; i < seg->x2 + fillLimit ; i++)
	{
	  j = i - beginMask ; j1 = j/8 ; j2 = 1 << (j%8) ;
	  if (j > 0 && j1 >= 0 && j1 < maxMask)
	    mymask[j1] |= j2 ;
	} 
    }

  if (!found)
    goto abort ;

  /* now sort by score and re add them or destroy them */
  mySegs = look->segs ;
  arraySort (aa, ospScoreOrder) ;


  for (jj = 0 ; jj < arrayMax(aa) ; jj++)
    {
      iSeg = arr(aa, jj, int) ;
      if (iSeg <= 0 || iSeg >= arrayMax(look->segs))
	messcrash("osp 3") ;
      seg = arrp(look->segs,arr(aa, jj, int),SEG) ;
      up = seg->type & 0x1 ? TRUE : FALSE ;
      mymask = up ? maskup : mask ;
      nf = fillLimit * .8 ;
      found = FALSE ;
      for (try = 0 ; try < 2 ; try++)
	{ 
	  
	  for (i = seg->x1 - fillLimit ; i < seg->x2 + fillLimit ; i++)
	    { j = i - beginMask ; j1 = j/8 ;  j2 = 1 << (j%8) ;
	      if (j > 0 && j1 >= 0 && j1 < maxMask)
		{
		  if (try)
		    mymask[j1] |= j2 ;
		  else
		    if (! (mymask[j1] & j2))
		      { if(!nf--)  { found = TRUE  ; break ;}}
		}
	    }
	  if (!found)
	    { arr(aa, jj, int) = 0 ;
	      break ;
	    }
	}
    } 
  arraySort (aa, intOrder) ;
  arrayCompress (aa) ;

  nks = 0 ; ks = keySetCreate () ;
  for (showUp = 0 ; showUp < 2 ; showUp++) 
    {
      for (iSeg = 1; iSeg <  arrayMax(look->segs) ; iSeg++)
	{
	  seg = arrp(look->segs,iSeg,SEG) ;

	  if  ((seg->type | 0x1) != OLIGO_UP)
	    continue ;


#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
	  ss =  seg->data.i ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */
	  ss = seg->feature_data.oligo_type ;

	  selected = ss &  OSP_S_BIT ? TRUE : FALSE ;
	  ss &= 0xfff ;

	  if ((ss > scoreLimit - .1) && 
	      !arrayFind (aa, &iSeg, &dummy, intOrder))
	    continue ;
	  up = (seg->type & 0x01) ? TRUE : FALSE ;
	  if ((up && !showUp) || (!up && showUp)) continue ;
	  x1 = seg->x1 ; x2 = seg->x2 ;
	  if (x2 < from || x1 > to) continue ;
	  keySet(ks, nks++) = iSeg ;
	}
    }

abort:
  arrayDestroy (aa) ;
  messfree (mask) ;
  messfree (maskup) ;
  return ks ;
} /* getActiveOligos */


/******************************/

static void showOligos (FeatureMap look, float *offset)
{ 
  int from, to, box, x1, x2, x, xmax = 0,  x2max = 0, iSeg ;
  int ii, showUp ;
  SEG *seg ;
  BUMP bump = 0 ;
  float xx, y1, y2, yb ;
  BOOL up , selected, old ;
  int ss ;
  Array aa = arrayCreate(200, int) ;
  KEYSET ks = 0 ;

  from = look->zoneMin ;
  to =  look->zoneMax ;
  ks = getActiveOligos (look, from, to) ;
  if (!ks) return ;
  bump = bumpCreate (20, 0) ; showUp = 0 ;
  for (ii = 0 ; ii < keySetMax (ks) ; ii++)
    {
      iSeg = keySet(ks, ii) ;
      seg = arrp(look->segs,iSeg,SEG) ;

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
      ss =  seg->data.i ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */
      ss = seg->feature_data.oligo_type ;

      selected = ss &  OSP_S_BIT ? TRUE : FALSE ;
      old = ss &  OSP_O_BIT ? TRUE : FALSE ;
      ss &= 0xFFF ;
      up = (seg->type & 0x01) ? TRUE : FALSE ;
      if (up && !showUp)
	{ showUp = TRUE ;
	  *offset += xmax*1.2 ; x2max += xmax*1.2 ; xmax = 0 ;   
	  bumpDestroy(bump) ;  
	  bump = bumpCreate (20, 0) ; 
	}
      x1 = seg->x1 ;
      x2 = seg->x2 ;
      y1 = MAP2GRAPH(look->map,x1) ;
      y2 = MAP2GRAPH(look->map,x2+1) ;
      if (y2 < topMargin  || y1 > mapGraphHeight)
	continue ;
      
      x = 0 ;
      yb = seg->x1 ; /* bump in seg coords, since increasing */
      x = ss/4 ;

      if (x2 <= x1 || yb < 0)
	messcrash("%s has bad coords: %d %d", nameWithClass(seg->key), seg->x1, seg->x2) ;
      else if (x < 0)
	messcrash("%s has bad OSP data: %d", nameWithClass(seg->key), x) ;

      bumpItem(bump, 1, x2 - x1 + 2, &x, &yb) ; 
      xx = *offset + 1.2*x ;
      box = graphBoxStart () ;
      array(look->boxIndex,box,int) = iSeg ;
      if (y1 < topMargin ) y1 =  topMargin  ;
      graphLine (xx, y1, xx, y2) ;
      if (up)
	{
	  graphLine (xx + .4, y1 +.7, xx, y1) ;
	  graphLine (xx - .4, y1 +.7, xx, y1) ;
	}
      else
	{
	  graphLine (xx + .4, y2 - .7, xx, y2) ;
	  graphLine (xx - .4, y2 - .7, xx, y2) ;
	}
      graphBoxEnd() ;
      graphBoxDraw  (box, BLACK, selected ? GREEN : old ? ORANGE : WHITE) ;
      if (xmax < x) xmax = x ;
    }
  *offset += xmax*1.2 ; x2max += xmax*1.2 ;
  if (x2max < 12)  *offset += 12 - x2max ;
  *offset +=  2 ;

  arrayDestroy (aa) ;
  bumpDestroy (bump) ;
} /* showOligos */

/**********************************************************************/
/**********************************************************************/

BOOL fMapOspPositionOligo (FeatureMap look, SEG *seg, KEY oligo, int *a1p, int *a2p)

{
  char *cp, *buf1 = 0, *cq, *buf = 0 ;
  OBJ Oligo = 0, Parent = 0 ;
  int x1, x2, pos, oligoLength ;
  int i, maxError = 0 , maxN = 0, from, len ;
  BOOL pUp = seg->type & 0x1 ? TRUE : FALSE ;
  char *motif ;
  BOOL up ;
  int y1, y2 ;
  
  Oligo = bsCreate (oligo) ;
  if (!Oligo || !look->dna) return FALSE ;
  
  if (!bsGetData(Oligo, _Sequence, _Text, &motif))
    goto abort ;
  
  oligoLength = strlen(motif) ;
  buf = strnew(motif, 0) ;
  bsDestroy (Oligo) ;
  
  if ((Parent = bsCreate (seg->key)))
      { 
	if (bsFindKey (Parent, _Oligo, oligo) &&
	    bsGetData (Parent, _bsRight, _Int, &y1) &&
	    bsGetData (Parent, _bsRight, _Int, &y2))
	  {
	    bsDestroy (Parent) ;
	    goto done ;
	  }
      }
 
  bsDestroy (Parent) ;
  
  cp = buf - 1 ;
  while(*++cp)
    *cp = freelower(*cp) ;
  dnaEncodeString(buf) ;
  
  /* try downwards */
  up = FALSE ;
  from = look->zoneMin ; /* seg->x1 - look->start ; */
  if (from < 0) from = 0 ;
  len = look->zoneMax - from ; /* seg->x2 - look->start - from ; */
  if (from + len >= arrayMax(look->dna))
    len =  arrayMax(look->dna) - 2 - from ;
  if (from < 0 || len < 0 || from + len >= arrayMax(look->dna))
    goto abort ;
 
  cp = arrp(look->dna,from,char) ;
  pos = dnacptPickMatch (cp, len, buf, maxError, maxN) ;
  if (pos && pos >= len) invokeDebugger() ;
  if (pos && pos < len)
    goto ok ;
  /* try upwards */
  up = TRUE ;
  cp = buf + oligoLength - 1 ; buf1 = buf ;
  buf = strnew(buf, 0) ;
  cq = buf ; 
  i = oligoLength ;
  while (i--)
    *cq++ = complementBase[(int)(*cp--)] ;
  messfree (buf1) ;
  
  cp = arrp(look->dna,from,char) ;
  pos = dnacptPickMatch (cp, len, buf, maxError, maxN) ;
  if (!pos || pos >= len)
    goto abort ;
  
ok:
  /* coordinates in look coord */
  x1 = pos - 1 + from ; 
  x2 = pos + oligoLength - 1 + from ; 
  /* coordinates relative to seg */
  if (a1p) /* not in case of new oligo */
    { x1 -= seg->x1 ; x2 -= seg->x1 ; }

  if (!up) { y1 = x1 + 1 ; y2 = x2 ; }
  else     { y1 = x2 ; y2 = x1 + 1 ; }
  if (pUp) 
    { y1 = len - y1 + 1 ; y2 = len - y2 + 1 ; }
  
  /* now store */

  if ((Parent = bsUpdate (seg->key)))
      { 
	bsAddKey(Parent, _Oligo, oligo) ;
	if (bsFindKey(Parent, _Oligo, oligo))
	  { 
	    bsAddData (Parent, _bsRight, _Int, &y1) ;
	    bsAddData (Parent, _bsRight, _Int, &y2) ;
	  }
	bsSave(Parent) ;
      }
done: 
  messfree (buf) ;
  if (a1p) *a1p = y1 ; if (a2p) *a2p = y2 ;
  return TRUE ;

abort: 
  messfree (buf) ;
  bsDestroy (Oligo) ;
  return FALSE ;
} /* fMapOspPositionOligo */

void fMapOspDestroy (FeatureMap look)
     /* called by fMapDestroy() */
{
  return ;
} /* fMapOspDestroy */

/******************************/


void fMapOspShowOligo (LOOK genericLook, float *offset, MENU *unused)
     /* This is of type MapColDrawFunc */
{ 
  FeatureMap look = (FeatureMap)genericLook;

  showOligos (look, offset) ;
} /* fMapOspShowOligo */

/***********************************************************************/

typedef struct keyseg {
  KEY o1, o2 ;
  int i1, i2, a1, a2, b1, b2 , f1, f2, score ; 
  float Tm1, Tm2, Tm ;
} OLPR ;

/***********************************************************************/

void fMapOspShowOligoPairs (LOOK genericLook, float *offset, MENU *unused)
     /* This is of type MapColDrawFunc */
{
  FeatureMap look = (FeatureMap)genericLook;
  int iSeg, i, box, x1, x2, x0 , xmax = 0, ss ;
  float xx, y1, y2,yb ;
  SEG *seg ;
  BUMP  bump = 0 ;
  Array mm = look->oligopairs ;
  OLPR* mmp ;
  BOOL found ;

  if (!mm)
    return ;
  bump = bumpCreate (mapGraphWidth,0) ;

  found = FALSE ;
  for (iSeg = 1; iSeg <  arrayMax(look->segs) ; iSeg++)
    {
      seg = arrp(look->segs,iSeg,SEG) ;
      /*
	does not work, because pairs coord do not flip
	if  ((seg->type | 0x1) != OLIGO_PAIR_UP) continue ;
	*/
      if  ((seg->type) != OLIGO_PAIR) continue ;
      if (!seg->x1 || !seg->x2) continue ;
      found = TRUE ;
  
      x1 = seg->x1 ; x2 = seg->x2 ;
      if (x2 - x1 <   productMinLength ||
	  x2 - x1 >   productMaxLength)
	continue ;

      y1 = MAP2GRAPH(look->map,x1) ;
      y2 = MAP2GRAPH(look->map,x2) ;
      if (y2 < topMargin || y1 > mapGraphHeight)
	continue ;
      i = seg->data.i & 0xffff ;

      if (i < 0 || i >= arrayMax(look->oligopairs))
	  continue ;
      mmp = arrp(look->oligopairs, i, OLPR) ;
      ss = mmp->score ;  
      if (ss > productMaxScore)
	continue ;

      x0 = 0 ; yb = seg->x1 ; /* bump in seg coords, since increasing */
     
      if (x2 <= x1 || yb < 0) messcrash("") ;
      bumpItem (bump, 1, x2 - x1 + 2.0, &x0, &yb) ; 
      xx = *offset + 1.2*x0 ;
      box = graphBoxStart () ;
      array(look->boxIndex,box,int) = iSeg ;
      if (y1 < topMargin ) y1 =  topMargin  ;
      if (y2 > mapGraphHeight - .6) y2 = mapGraphHeight - .6 ;
      graphLine (xx, y1, xx, y2) ;

      if (seg->type & 0x1)  /* up */
	{
	  y1 = MAP2GRAPH(look->map,mmp->b1) ;
	  y2 = MAP2GRAPH(look->map,mmp->b2) ;
	  if (y1 > topMargin && y1 < mapGraphHeight)
	    { 
	      box = graphBoxStart () ;
	      array(look->boxIndex,box,int) = mmp->i2 ;
	      graphLine (xx, y1, xx, y2) ;
	      graphLine (xx - .4, y1, xx + .4, y1) ;
	      graphLine (xx + .4, y2 -.7, xx, y2) ;
	      graphLine (xx - .4, y2 -.7, xx, y2) ;
	      graphBoxEnd() ;
	      if (keySetFind (selectedOligos, mmp->o2, 0))
		graphBoxDraw (box, BLACK, GREEN) ;
	    }

	  y1 = MAP2GRAPH(look->map,mmp->a1) ;
	  y2 = MAP2GRAPH(look->map,mmp->a2) ;
	  if (y1 > topMargin && y1 < mapGraphHeight)
	    {
	      box = graphBoxStart () ;
	      array(look->boxIndex,box,int) = mmp->i1 ;
	      graphLine (xx, y1, xx, y2) ;
	      graphLine (xx - .4, y2, xx + .4, y2) ;
	      graphLine (xx + .4, y1 +.7, xx, y1) ;
	      graphLine (xx - .4, y1 +.7, xx, y1) ;
	      graphBoxEnd() ;
	      if (keySetFind (selectedOligos, mmp->o1, 0))
		graphBoxDraw (box, BLACK, GREEN) ;
	    }
	}
      else /* down */
	{
	  y1 = MAP2GRAPH(look->map,mmp->a1) ;
	  y2 = MAP2GRAPH(look->map,mmp->a2) ;
	  if (y1 > topMargin && y1 < mapGraphHeight)
	    { 
	      box = graphBoxStart () ;
	      array(look->boxIndex,box,int) = mmp->i1 ;
	      graphLine (xx, y1, xx, y2) ;
	      graphLine (xx - .4, y1, xx + .4, y1) ;
	      graphLine (xx + .4, y2 -.7, xx, y2) ;
	      graphLine (xx - .4, y2 -.7, xx, y2) ;
	      graphBoxEnd() ;
	      if (keySetFind (selectedOligos, mmp->o1, 0))
		graphBoxDraw (box, BLACK, GREEN) ;
	      else if (mmp->f1 &  OSP_O_BIT)
		graphBoxDraw (box, BLACK, ORANGE) ;
	    }
	  else if (y1 <= topMargin)
	    graphCircle(xx,topMargin, .4) ;

	  y1 = MAP2GRAPH(look->map,mmp->b1) ;
	  y2 = MAP2GRAPH(look->map,mmp->b2) ;
	  if (y1 > topMargin && y1 < mapGraphHeight)
	    {
	      box = graphBoxStart () ;
	      array(look->boxIndex,box,int) = mmp->i2 ;
	      graphLine (xx, y1, xx, y2) ;
	      graphLine (xx - .4, y2, xx + .4, y2) ;
	      graphLine (xx + .4, y1 +.7, xx, y1) ;
	      graphLine (xx - .4, y1 +.7, xx, y1) ;
	      graphBoxEnd() ;

	      if (keySetFind (selectedOligos, mmp->o2, 0))
		graphBoxDraw (box, BLACK, GREEN) ;  
	      else if (mmp->f2 &  OSP_O_BIT)
		graphBoxDraw (box, BLACK, ORANGE) ;
	    }
	  else if (y1 > mapGraphHeight)
	    graphCircle(xx,mapGraphHeight - .6, .4) ;
	}

      graphBoxEnd() ;
      if (xmax < x0) xmax = x0 ;
    }
  *offset += xmax*1.2 ; 
  *offset +=  2 ;

  bumpDestroy (bump) ;
} /* fMapOspShowOligoPairs */

/***********************************************************************/

void fMapOspSelectOligoPair (FeatureMap look, SEG *seg) 
{
  OLPR* mpp ;
  int i ;

  if (seg->type != OLIGO_PAIR && seg->type != OLIGO_PAIR_UP)      
    return ;

  i = seg->data.i & 0xffff ;

  if (!arrayExists(look->oligopairs) || i < 0 || i >= arrayMax(look->oligopairs)) 
    return ;

  mpp = arrp(look->oligopairs, i, OLPR) ;
	  
  strncpy (look->segTextBuf, 
	   messprintf(
  "%d %d (l= %d), %s %s, score %d , Tm_o1 %3.1f, Tm_o2 %3.1f, Tm_p %3.1f",
	     seg->x1 + 1, seg->x2, seg->x2 - seg->x1, 
	     name(mpp->o1), name(mpp->o2), mpp->score, 
	     mpp->Tm1, mpp->Tm2, mpp->Tm),
	   125) ;

  return;
} /* fMapOspSelectOligoPair */

/***************************************************************************************/

static int OLPROrder (void *a, void *b)
{ 
  int aa = ((OLPR*)a)->a1, ab = ((OLPR*)b)->a1 ;
  int sa = ((OLPR*)a)->score, sb = ((OLPR*)b)->score ;
  
  if (sa < sb) return -1 ; if (sa > sb) return 1 ;
  return aa - ab ; /* equal score, sort on position */
} /* OLPROrder */

void fMapOspFindOligoPairs (FeatureMap look) 
{
  static KEY  _Pairwise_scores = 0 ;
  OBJ obj ;
  KEY o2 ;
  float score = 0, Tm ;
  int i, j , i2 ;
  Array mm = 0 ;
  OLPR *mmp ;
  SEG *seg , *seg2 ;
  void *v ;
  BSMARK mark = 0 ;
  Associator aa = assCreate() ;

  if (!keySetExists(selectedOligos))
    selectedOligos = keySetCreate () ;
  /* accumulate all the oligos */
  for (j = 0, i = 1 ; i < arrayMax(look->segs) ; ++i)
    { seg = arrp(look->segs,i,SEG) ;
      
      switch (seg->type)
	{
	case OLIGO:
	case OLIGO_UP:
	  j++ ;
	  assInsert (aa, assVoid(seg->key), assVoid(i)) ;


#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
	  if (keySetFind (selectedOligos, seg->key, 0))
	    seg->data.i |=  OSP_S_BIT ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */
	  if (keySetFind (selectedOligos, seg->key, 0))
	    seg->feature_data.oligo_type |=  OSP_S_BIT ;

	  break ;
	default:
	  break ;
	}
    }
  if (!j) { assDestroy (aa) ; return ; }


  if (!_Pairwise_scores)
    lexaddkey("Pairwise_scores",&_Pairwise_scores,0) ;

  mm = arrayCreate (32, OLPR) ;
  for (j = 0, i = 1 ; i < arrayMax(look->segs) ; ++i)
    {
      seg = arrp(look->segs,i,SEG) ;
      
      switch (seg->type)
	{
	case OLIGO:
	case OLIGO_UP:
	  if ((obj = bsCreate (seg->key)))
	    {
	      if (bsGetKey (obj, _Pairwise_scores, &o2))
		do
		  { 
		    mark = bsMark (obj, mark) ;
		    if (bsGetData (obj, _bsRight, _Float, &score) &&
			assFind (aa, assVoid(o2), &v))
		      { 
			i2 = assInt (v) ;
			seg2 = arrp(look->segs,i2,SEG) ;
			mmp = arrayp (mm, j++, OLPR) ;
			mmp->i1 = i ;
			mmp->i2 = i2 ;
			mmp->o1 = seg->key ;
			mmp->o2 = o2 ;
			mmp->score = score ;


#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
			mmp->Tm1 = ((seg->data.i >> 16) & 0x3ff)/10.0 ;
			mmp->Tm2 = ((seg2->data.i >> 16) & 0x3ff)/10.0 ;
			mmp->f1 = seg->data.i ; mmp->f2 = seg2->data.i ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */
			mmp->Tm1 = ((seg->feature_data.oligo_type >> 16) & 0x3ff)/10.0 ;
			mmp->Tm2 = ((seg2->feature_data.oligo_type >> 16) & 0x3ff)/10.0 ;
			mmp->f1 = seg->feature_data.oligo_type ;
			mmp->f2 = seg2->feature_data.oligo_type ;

			Tm = 0 ;
			bsGetData (obj, _bsRight, _Float, &Tm) ;
			mmp->Tm = Tm ;
			switch (seg->type)
			  {
			  case OLIGO:
			    mmp->a1 = seg->x1 ; mmp->a2 = seg->x2 ;
			    mmp->b1 = seg2->x1 ; mmp->b2 = seg2->x2 ;
			    break ;
			  case OLIGO_UP:
			    mmp->a1 = seg2->x1 ; mmp->a2 = seg2->x2 ;
			    mmp->b1 = seg->x1 ; mmp->b2 = seg->x2 ;
			    break ;
			  default:
			    break ;
			  }
		      }
		    bsGoto (obj, mark) ;
		  } while (bsGetKey (obj, _bsDown, &o2)) ;

	      messfree (mark) ;
	      bsDestroy (obj) ;
	    }
	  break ;
	default:
	  break ;
	}
    }
  assDestroy (aa) ;

  if (arrayMax(mm))
    {
      arraySort (mm, OLPROrder) ;
      arrayCompress (mm) ;

      for (j = arrayMax(look->segs), i = 0 ; i < arrayMax(mm) ; i++)
	{
	  mmp = arrayp (mm, i, OLPR) ;
	  seg = arrayp(look->segs, j++, SEG) ;
	  seg->type = OLIGO_PAIR ;
	  seg->parent = 0 ;
	  seg->key = mmp->o1 ;
	  seg->x1 = mmp->a1 ;
	  seg->x2 = mmp->b2 ;
	  seg->data.i = i ;
	}
    }
  else
    {
      arrayDestroy (mm) ;
    }

  look->oligopairs = mm ;

  return ;
} /* fMapOspFindOligoPairs */

/***********************************************************************/

void fMapOspInit (void)
{
  FeatureMap look = 0 ;

  if (graphAssFind (&GRAPH2FeatureMap_ASSOC, &look))
    {
      /* re-use exisitng fmaplook */
      if (look->magic != &FeatureMap_MAGIC)
	messcrash ("fMapOspInit found non-magic FeatureMap on graph");

      mapColSetByName (look->map, "Oligos", TRUE) ;
      mapColSetByName (look->map, "Oligo_pairs", TRUE) ;  
      look->pleaseRecompute = TRUE ;
      fMapDraw(look,0) ;
    }

  fMapOspControl() ;

  return;
} /* fMapOspInit */

/***********************************************************************/
/************ Osp control graph, actual calls to osp *******************/
/***********************************************************************/

static PickFunc oldPick = 0 ;
static int selecting = 0 ;
static int selectButtonBox = 0 ;
static int renaming = 0 ;
static int renameButtonBox = 0 ;

static void popOspControl(void)
{
  if (graphActivate (ospControlGraph))
    graphPop() ;
} /* popOspControl */

static FeatureMap OSPGET (char *title)
{
  FeatureMap look ; 
  
  if (!graphActivate (ospControlGraph))
    { ospControlGraph = 0 ; return 0 ; }
  graphCheckEditors(ospControlGraph, TRUE) ;

  /*  if (selecting == 1) selecting = 0 ; */
  fMapOspControlDraw () ;
  if (!fMapActive (0,0,0,&look)) 
    { messout("Sorry, no active sequence, I cannot proceed") ;
    return 0 ;
    }
  graphUnMessage () ;

  if (graphActivate (look->graph))
    {
      if (oldPick) graphRegister (PICK, oldPick) ;
      if (!mapColSetByName(look->map, "Oligos", -1) ||
	  !mapColSetByName(look->map, "Oligo_pairs", -1))
	{ graphPop() ;
	  mapColSetByName (look->map, "Oligos", TRUE) ;
	  mapColSetByName (look->map, "Oligo_pairs", TRUE) ;  
	  look->pleaseRecompute = TRUE ;
	  fMapDraw(look,0) ;
	  messout("Sorry, the active sequence has changed,  try again") ;
	  return 0 ;
	}
    }
  return look ;
} /* OSPGET */

/******************************/

static void ospRedraw (void)
{
  FeatureMap look = OSPGET ("ospRedraw") ;
  if (!look) return ;

  fMapDraw(look,0) ;   
  popOspControl() ;
  fMapOspControlDraw () ;

  return;
} /* ospRedraw */


static void ospLimitUp (void)
{
  scoreLimit += 2 ;
  ospRedraw () ;

  return;
} /* ospLimitUp */

static void ospLimitDown (void)
{
  if (scoreLimit > 1) scoreLimit -= 2 ;
  ospRedraw () ;

  return;
} /* ospLimitDown */

/******************************/

static void ospFillUp (void)
{
  int n = fillLimit/2 ;

  if (n < 10) n = 10 ;
  fillLimit += n ;
  fillLimit -= fillLimit % 20 ;
  ospRedraw () ;

  return;
} /* ospFillUp */

static void ospFillDown (void)
{
  int n = fillLimit/3 ;

  if (n < 1) n = 1 ;
  fillLimit -= n ;
  fillLimit -= fillLimit % n ;
  if (fillLimit < n) fillLimit = n ;
  ospRedraw () ;

  return;
} /* ospFillDown */

/******************************/

static void ospSubmitPairs (void)
{ 
  int ii, iSeg, from = 0, to ;
  static BOOL showUp = FALSE, up ;
  char *tmpFasta = 0, *tmpOligos= 0;
  char *scriptName = 0;
  ACEOUT fasta_out, oligo_out;
  Stack sparm = 0 ;
  KEYSET  ks = 0 ;
  SEG *seg ;
  FeatureMap look = OSPGET ("ospSubmitPairs") ;
  ACEIN fi;
  STORE_HANDLE handle = handleCreate();

  if (!look) return ;

  from = look->zoneMin ;
  to = look->zoneMax ; 
  if (look->flag & FLAG_COMPLEMENT)
    { 
      messout 
	("Sorry, please un-reverse the dna, i can only work on the down strand") ;
      return ;
    }
  ks = getActiveOligos (look, from, to) ;
  if (!ks || keySetMax(ks) < 2)
    { messout ("First get oligos") ; keySetDestroy (ks) ; return ; }
  if (keySetMax(ks) > 100 && 
      !messQuery("This may take several minutes, do you want to proceed"))
    goto abort ;

  /********************/

  {
    FILE *tmp_fil = filtmpopen (&tmpFasta,"w");
    if (!tmp_fil)
      goto abort ;
    filclose (tmp_fil);		/* just interested in name */
  }
  fasta_out = aceOutCreateToFile (tmpFasta, "w", handle);
  if (!fasta_out)
    goto abort;


  /* export correctly ? */
  dnaDumpFastA (look->dna, from, to, name(look->seqKey), fasta_out) ;

  aceOutDestroy (fasta_out) ;

  /******************/

  {
    FILE *tmp_fil = filtmpopen (&tmpOligos,"w");
    if (!tmp_fil)
      goto abort ;
    filclose (tmp_fil);		/* just interested in name */
  }
  oligo_out = aceOutCreateToFile (tmpOligos, "w", handle);
  if (!oligo_out)
    goto abort;

  aceOutPrint (oligo_out, "// top strand\n") ;
  showUp = FALSE ;
  for (ii = 0 ; ii < keySetMax(ks) ; ii++)
    { 
      iSeg = keySet(ks, ii) ;
      seg = arrp(look->segs,iSeg,SEG) ;
      up = (seg->type & 0x01) ? TRUE : FALSE ;  
      if (up && !showUp)
	{
	  showUp = TRUE ;
	  aceOutPrint (oligo_out, "// bottom strand\n") ;
	}
      aceOutPrint (oligo_out,  
		   "%s %d %d\n", name(seg->key), seg->x1 - from, seg->x2 - from) ;
    }
       
  aceOutDestroy (oligo_out);

  /********************/

  /*   messout ("Exported in %s and %s", tmpFasta, tmpOligos ) ;  */
  messStatus ("OSP: searching pairs") ;
 
  sparm = stackHandleCreate (50, handle) ;
  pushText(sparm," -s  ") ;  
  catText(sparm,tmpFasta) ; 
  catText(sparm," -g ") ;  
  catText(sparm,tmpOligos) ; 
  catText (sparm, messprintf(" -l %d -L %d ", productMinLength, productMaxLength)) ;

  scriptName = dbPathFilName(WSCRIPTS, "osp2", "pl", "x", handle);
   
  if (!scriptName)
    { messout ("Sorry, cannot find wscripts/osp2.csh") ; goto abort ; } 
  
  messStatus("Oligo  Pairs Scoring") ;

  if (!sessionGainWriteAccess())
    goto abort;

  fi = aceInCreateFromScriptPipe (scriptName, "", 
				  stackText (sparm, 0), 0);
  if (!fi)
    goto abort ;

  keySetDestroy (ks) ;
  ks = keySetCreate () ;

  parseAceIn (fi, ks, TRUE) ;	/* will destroy fi */

    
 /* report in fmap window */
  if (graphActivate (look->graph))
    { mapColSetByName (look->map, "Oligo_pairs", TRUE) ;
      look->pleaseRecompute = TRUE ;
      fMapDraw(look,0) ; 
    }

 abort:
  graphUnMessage() ;
  handleDestroy (handle);
  keySetDestroy (ks) ;

  if (tmpFasta) filtmpremove (tmpFasta) ;
  if (tmpOligos) filtmpremove (tmpOligos);

  return;
} /* ospSubmitPairs */

static void fMapOspSubmit (void)
{
  int i, from = 0, to ;
  OBJ obj = 0 ;
  char *tmpFasta = 0, *scriptName = 0 ;
  ACEOUT fasta_out;
  Stack sparm = 0 ;
  KEYSET  kSet = 0, ksold, ksnew ;
  KEY _Temporary ;
  FeatureMap look = OSPGET ("fMapOspSubmit") ;
  ACEIN fi = 0 ;

  if (!look) return ;
  sparm = stackCreate (50) ;
 
  from = look->zoneMin ;
  to = look->zoneMax ; 
 
  if (to > from + 5000 && 
      !messQuery(
      messprintf("%s%d%s\n%s\n%s",
		 "Searching oligos on ", to - from,
		 " bases may take several minutes",
		 " You may consider reducing the active zone",
		 "Do you wish to continue ?")))
    goto abort ;


  {
    FILE *tmp_fil= filtmpopen (&tmpFasta,"w");
    if (!tmp_fil)
      goto abort ;
    filclose (tmp_fil);		/* just interested in the name */
  }
  fasta_out = aceOutCreateToFile (tmpFasta, "w", 0);
  if (!fasta_out)
    goto abort;

  dnaDumpFastA (look->dna, from, to, name(look->seqKey), fasta_out);

  aceOutDestroy (fasta_out);

  messStatus ("OSP: searching Oligos") ;
  pushText(sparm,messprintf("%s %d %d %d %d %d %s",
			    name(look->seqKey), /* to get the oligos back in place */
			    oligoMaxScore, oligoMinLength,  oligoMaxLength ,  
			    oligoTmMin,oligoTmMax,
			    tmpFasta )) ; /* the sequence fatsa file */

  scriptName = dbPathFilName(WSCRIPTS, "osp1", "csh","x", 0);
  if (!scriptName)
    { messout ("Sorry, cannot find wscripts/osp1.csh") ; goto abort ; } 
 
 
  messStatus("Oligo Searching") ;

  if (!sessionGainWriteAccess())
    goto abort;

  fi = aceInCreateFromScriptPipe (scriptName, "", 
				  stackText (sparm, 0), 0);
  if (!fi)
    goto abort ;

  ksold = query (0, "FIND Oligo") ;

  aceInSpecial (fi,"\n/\\\t@") ;
  kSet = keySetCreate () ;
  parseAceIn (fi, kSet, FALSE) ; /* destroys fi */

  ksnew = keySetMINUS (kSet, ksold) ;

  /* flag just the new oligos as temporary */
  lexaddkey ("Temporary", &_Temporary, 0) ;
  i = keySetMax (ksnew) ;
  while (i--)
    {
      if ((obj = bsUpdate (keySet (ksnew, i))))
	{
	  bsAddTag (obj, _Temporary) ;
	  bsSave (obj) ;
	}
    }
  keySetDestroy (ksold) ;
  keySetDestroy (ksnew) ;
  /* report in dnacpt window */

   look->pleaseRecompute = TRUE ;
  fMapDraw(look,0) ;

 abort:
  messfree (scriptName);
  graphUnMessage () ;
  stackDestroy (sparm);
  keySetDestroy (kSet) ;
  
  if (tmpFasta) filtmpremove (tmpFasta) ;

  return;
} /* fMapOspSubmit */

/********************************************/

static void fMapOspSelectBox (int box, double x_unused, double y_unused, int modifier_unused)
{
  int index ;
  SEG *seg ;
  int ss ;
  BOOL selected = FALSE ;
  Graph old ;
  FeatureMap look = currentFeatureMap("fMapOspSelectBox") ;

  if (!selecting) 
    { 
      if (oldPick) graphRegister (PICK, oldPick) ;
      return ;
    }

  if (!(index = arr(look->boxIndex, box, int)))
    return ;

  seg = arrp(look->segs, index, SEG) ;

  if ((seg->type | 0x1) != OLIGO_UP)
    return ;


#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
  if (!(seg->data.i & OSP_O_BIT))
    {
      if (box == look->activeBox)
	seg->data.i ^= OSP_S_BIT ; /* toggle */ 
      ss =  seg->data.i ;
      selected = ss  &  OSP_S_BIT ? TRUE : FALSE ;
      if (selected)
	keySetInsert (selectedOligos, seg->key) ;
      else
	keySetRemove (selectedOligos, seg->key) ;
    }
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */
  if (!(seg->feature_data.oligo_type & OSP_O_BIT))
    {
      if (box == look->activeBox)
	seg->feature_data.oligo_type ^= OSP_S_BIT ; /* toggle */ 

      ss =  seg->feature_data.oligo_type ;

      selected = ss  &  OSP_S_BIT ? TRUE : FALSE ;
      if (selected)
	keySetInsert (selectedOligos, seg->key) ;
      else
	keySetRemove (selectedOligos, seg->key) ;
    }




  old = graphActive () ;
  if (oldPick)
    oldPick (box, 0, 0, 0) ;
  graphActivate (old) ;
  graphBoxDraw  (box, BLACK, selected ? GREEN : WHITE) ; 
  
  return;
} /* fMapOspSelectBox */

static void fMapOspSelectEnds (void)
{ 
  FeatureMap look = 0 ; 
  
  selecting = 0 ;
  if (graphActivate (ospControlGraph))
    look = OSPGET ("selectEnds") ;
  if (look)
    look->isOspSelecting = FALSE;

  return ;
} /* fMapOspSelectEnds */


static void fMapOspSelectOligos (void)
{ 
  FeatureMap look = 0 ;
  if (isDisplayBlocked())
    {
      messout ("%s", "Finish what you are doing first") ;
      return ;
    }
  selecting = selecting ? 1 : 2 ;
  if (!selecting || !OSPGET ("fMapOspSelectOligos")) /* if (selecting == 1) resets it  to 0 */
    return ;
  if (isDisplayBlocked())
    {
      messout ("%s", "Finish what you are doing first") ;
      return ;
    }
  if(!selecting)
    { graphUnMessage(); 
      return ;
    }
  look = OSPGET ("fMapOspSelectOligos") ;
  selecting = 1 ;
  if (look) 
  {
    PickFunc tmp  = (PickFunc) graphRegister (PICK,  (GraphFunc)fMapOspSelectBox) ;
    if (tmp != (PickFunc) fMapOspSelectBox)
      oldPick = tmp ; 
    look->isOspSelecting = TRUE;
  }
  graphActivate (ospControlGraph) ;
  graphMessage  ("Click on the oligos you want to select,\n they will turn green\n\n"
		"Reclick on the select button or remove this box when you are done") ;
  graphRegister (MESSAGE_DESTROY, fMapOspSelectEnds) ; 

  return;
} /* fMapOspSelectOligos */

static void fMapOspDiscardOligos (void)
{ 
  KEYSET ks = query (0,"FIND Oligo Temporary") ;
  int i = keySetMax (ks) ;
  OBJ obj = 0 ;
  FeatureMap look ;

  if (selecting == 1)
  {
    messout ("%s", "Finish what you are doing first") ;
    return ;
  }
  look = OSPGET ("fMapOspDiscardOligos") ;
  if (!look) return ;

  sessionGainWriteAccess() ;  

  while (i--)
    if ((obj = bsUpdate (keySet (ks, i))))
      bsKill (obj) ;	      
  look->pleaseRecompute = TRUE ;
  ospRedraw () ;

  return;
} /* fMapOspDiscardOligos */

static void *keySetOrderedLook = 0 ; /*mhmp 16.10.98 */

static void fMapOspOrderOligos (void)
{
  KEYSET oldSet = 0, nKs = 0 ;
  OBJ Oligo ;
  KEY oligo, _Temporary ;
  int i ;
  KeySetDisp keySetLook ;

  if (selecting == 1)
    {
      messout ("%s", "Finish what you are doing first") ;
      return ;
    }
  if (!OSPGET ("Order"))
    return ;

  sessionGainWriteAccess() ;

  lexaddkey ("Temporary", &_Temporary, 0) ;
  for (i = 0 ; i < keySetMax (selectedOligos) ; i++)
    {
      oligo = keySet (selectedOligos, i) ;
      Oligo = bsUpdate (oligo) ;
      if (Oligo)
	{ 
	  if (bsFindTag (Oligo, _Temporary))
	    bsRemove(Oligo) ;
	  bsSave (Oligo) ;
	}
    }
  /* On cherchait visiblement a garder la meme fenetre. En vain ... */  
  nKs = keySetCopy (selectedOligos) ;
  if (keySetActive(&oldSet, &keySetLook) &&
      (keySetLook ==  keySetOrderedLook)) 
    {
      keySetDestroy (oldSet) ;
      keySetShow (nKs, keySetLook) ;
    }
  else
    {
      keySetLook = keySetNewDisplay (nKs, "Selected Oligos To be Ordered") ;
      keySetOrderedLook = keySetLook ;
    }
} /* fMapOspOrderOligos */


static void fMapOspImportOligos (void)
{
  KEYSET ks = 0 , ks0 = 0, ks1 = 0 ;  
  SEG Seg ;
  int i, nfit = 0, nnofit = 0, nbadl = 0 ;
  FeatureMap look = OSPGET ("fMapOspImportOligos") ;

  if (!look) return ;

  if (!keySetActive(&ks, 0)) 
    { messout("First select a KeySet containing oligos") ;
    return ;
    }
  /* never destroy ks, it belongs to keySetActive */
  ks0 = query (ks, "(CLASS Oligo) AND  (Sequence = *) AND (NOT Temporary)") ; 
  ks1 = keySetAlphaHeap(ks0, keySetMax(ks0)) ; keySetDestroy (ks0) ;
  i = keySetMax (ks1) ;
  if (!i)
    { messout("First select a KeySet containing oligos") ;
    keySetDestroy (ks1) ;
    return ;
    }

  Seg.x1 =  look->zoneMin ;
  Seg.x2 =  look->zoneMax ;
  Seg.key = look->seqKey ;
  
  i = keySetMax (ks1) ;
  while (i--)
    { 
      OBJ obj = bsCreate (keySet(ks1, i)) ;
      char *cp ;

      if (!obj) goto jump ;
      if (!bsGetData (obj, _Sequence, _Text, &cp))
	goto jump ;
      bsDestroy (obj) ;
      continue ;
    jump:
      nbadl++ ;
      bsDestroy (obj) ;
      keySet(ks1, i) = 0 ;
    }
  i = keySetMax (ks1) ;
  while (i--)
    if ( keySet(ks1,i))
      {
	if (fMapOspPositionOligo (look, &Seg, keySet(ks1, i), 0, 0)) 
	  nfit++ ; 
	else
	  nnofit++ ;
      }
  i = keySetMax (ks1) ;
  if (nfit)
    { look->pleaseRecompute = TRUE ;
    fMapDraw(look,0) ;
    popOspControl() ;
    }
  messout ("%d Oligos accepted, %d don't fit, %d do", 
	   keySetMax (ks1) - nbadl, nnofit, nfit) ;
  keySetDestroy (ks1) ;

  return;
} /* fMapOspImportOligos */

/*****************************************************************/

static MENUOPT ospControlMenu[] =
{ { graphDestroy, "Quit" },
  { help,         "Help" },
  { ospRedraw,    "Redraw" },
  { 0, 0 }
} ;


/*****************************************************************/
static BOOL checkTC (int tc)
{
  if (tc < 20 || tc > 99)
    return FALSE ;
  return TRUE ;
} /* checkTC */

static void fMapOspShowAllOldOligos (void)
{ 
  KEYSET ks = query (0,"FIND OLigo") ;

  keySetNewDisplay (ks, "All oligos") ;

  return;
} /* fMapOspShowAllOldOligos */

static void fMapOspShowFilteredOldOligos (void)
{
  KEYSET ks = 0 ;

  ks = query (0, messprintf (
  "FIND Oligo ; Tm <= %d AND Tm >= %d AND Length <= %d AND Length >= %d",
  oligoTmMax, oligoTmMin,  oligoMaxLength,  oligoMinLength)) ;
  keySetNewDisplay (ks, "Filtered oligos") ;

  return;
} /* fMapOspShowFilteredOldOligos */

static int fMapOspAliasOligo(KEY key, char* newName)
{
  char *oldName ;
  int  classe = class(key) ;
  KEY  newKey ;
  BOOL isCaseSensitive = pickList[classe & 255 ].isCaseSensitive ;
  int (*lexstrIsCasecmp)() = isCaseSensitive ?
    strcmp : strcasecmp ;
  int ret;

     /********** Protection *************/ 

  if (lexiskeylocked(key))
    { 
       messout("Sorry, alias fails because %s is locked elsewhere",
	      name(key)) ;
      return 2 ;
    }

      /************* Identity *************/

  newName = lexcleanup(newName, 0) ;
  oldName = name(key) ;
  if (!strcmp(oldName, newName))
    ret = 1 ;
  else if (!lexstrIsCasecmp(oldName, newName))
    ret = 1 ;

     /************* Unknown name case *************/

  else ret = !lexword2key (newName, &newKey, classe) ;

  messfree(newName);
  
  return ret;
} /* fMapOspAliasOligo */


static void fMapOspPickOligo (KEY key)
{
  int i, pos, suffix, iNew ;
  char oligoName [35], oligoNameBegin [25] ;
  SEG *seg ;
  FeatureMap look = OSPGET("fMapOspPickOligo") ;

  displayRepeatBlock () ;  
  if (!keySetFind (selectedOligos, key, 0))
    return ;
  for (i = 1 ; i < arrayMax(look->segs) ; ++i)
    if (arrp(look->segs,i,SEG)->key == key)
      { 
	seg = arrp(look->segs,i,SEG) ;
	if (seg->type != OLIGO && seg->type != OLIGO_UP)
	  return ;
	strcpy(oligoName, look->oligoNameBuffer) ;
	if (seg->type & 0x01)
	  strcat (oligoName, "-") ;
	else
	  strcat (oligoName, "+") ;
	pos = COORD (look, seg->x1) ;
	if (pos < 0)
	  strcat (oligoName, "x") ;
	pos = abs(pos)/100 ;
	strcat (oligoName, messprintf("%d",pos)) ;

	strcpy (oligoNameBegin, oligoName) ;
	suffix = 1 ;
	while (!(iNew =fMapOspAliasOligo (key, oligoName))) 
	  {
	    strcpy (oligoName, oligoNameBegin) ;
	    strcat (oligoName, ".") ;
	    strcat (oligoName, messprintf("%d", suffix)) ;
	    suffix++ ;
	  }
	if (iNew == 1)
	  {
	    ACEIN name_in;

	    /* pour permettre de choisir "son" suffixe ex: z9+x12bam */
	    if ((name_in = messPrompt
		 ("This is the new name.\n"
		  "You can modify this name. Then type OK",
		  oligoName, "w", 0)))
	      {
		aceInNext (name_in) ;
		strcpy (oligoName, aceInPos(name_in)) ;
		aceInDestroy (name_in);

		if (!fMapOspAliasOligo (key, oligoName))
		  messout ("Sorry! This name already exists.") ;
		else
		  {
		    lexAlias (key, oligoName, TRUE, FALSE) ;
		    if (!(look->flag & FLAG_HIDE_HEADER)) 
		      {
			fMapReportLine (look, seg, FALSE, 0) ;
		        graphBoxDraw (look->segBox, BLACK, LIGHTBLUE) ;

			/* default is to put dna in cut buffer, alternative is coord data.         */
			if (look->cut_data == FMAP_CUT_OBJDATA)
			  graphPostBuffer(look->segTextBuf) ;
		      }
		  }
	      }
	  } 
	fMapOspOrderOligos () ;
	return ; 
      }

  return;
} /* fMapOspPickOligo */


static void renameOligoDestroy (void)
{
  renaming = 0 ; 
  OSPGET("renameOligoDestroy") ;
  displayUnBlock() ;

  return;
} /* renameOligoDestroy */

static char oligoName [16] ;

static void fMapOspRenameOligo (void) 
{ 
  FeatureMap look ;

  if (renaming)
    { renameOligoDestroy () ;
      return ;
    }
  if (selecting == 1)
    {
      messout ("%s", "Finish what you are doing first") ;
      return ;
    }
  look = OSPGET ("fMapOspRenameOligo") ;
  if (!look) return  ;
  if (selecting == 1)
    {
      messout ("%s", "Finish what you are doing first") ;
      return ;
    }
  if (!keySetMax(selectedOligos))
    {
      messout ("%s%s", "Select first the oligos\n",
	       "you want to rename");
      return ;
    }
  if(!checkWriteAccess())
    { messout("Sorry, you do not have Write Access");
    return ;
    }
  renaming = 1 ;
  strcpy (look->oligoNameBuffer, oligoName) ;
  graphRegister (MESSAGE_DESTROY, renameOligoDestroy) ;
  if (!strlen(look->oligoNameBuffer))
     messout("%s%s",
		  "Type first the begin of the new name. ",
		  "You have also to fix the Origin ") ;
  else
    { displayBlock (fMapOspPickOligo,
		  "Pick an oligo to rename it.\n "
		  "Remove this message to cancel.\n\n"
		  "If you haven't fixed the Origin:\n"
		  "   Remove\n"
		  "   Fix the Origin.\n"
		  "   Then come back to Rename Oligo.") ;
      fMapOspOrderOligos () ; 
    }

  return;
} /* fMapOspRenameOligo */

/***********************************/

static void fMapOspControlDraw (void)
{ 
  int line = 3;

  graphFitBounds (&mapGraphWidth, &mapGraphHeight) ;

  graphClear () ;
  graphText ("An interface by D&J T-M to OSP",
	     2, line++) ;
  graphText (" the Oligo Selection Program",
	     2, line++) ;
  graphText (" from Ladeana Hillier",
	     2, line++) ;
  line += 1.2 ;
  graphLine(0,line, mapGraphWidth, line) ;
  line += 1.2 ;

  graphText ("Filters: ", 1, line++) ;
  graphText ("Oligo length", 3, line) ;
  graphIntEditor ("Min", &oligoMinLength, 18, line++, 0) ; 
  graphIntEditor ("Max", &oligoMaxLength, 18, line++, 0) ;line += .3 ;
  graphIntEditor ("Tm min", &oligoTmMin, 3, line, checkTC) ;
  graphIntEditor ("Tm max", &oligoTmMax, 21, line, checkTC) ;

  line += 2.2 ;
  graphLine(0, line, mapGraphWidth, line) ;
  line += 1.2 ;

  graphButton("List all old oligos", fMapOspShowAllOldOligos, 2,line) ; line += 2 ; 
  graphButton("List old oligos satisfying the filters", fMapOspShowFilteredOldOligos, 2,line) ;  line += 2 ;
  graphButton("Get Selected list of Oligos", fMapOspImportOligos, 2,line) ;  line += 2 ;

  line += 1 ;

  graphButton("Find New Oligos satisfying the filters",fMapOspSubmit, 2,line) ; 

  line += 2.2 ;
  graphLine(0, line, mapGraphWidth, line) ;
  line += 1.2 ;

  graphText ("Display Oligos", 2,line) ; line += 1.2 ;
  graphText(messprintf("with score <= %3d", scoreLimit),5 + 3.27, line + .7) ;
  graphButton("<", ospLimitDown,5 , line + .5) ;
  graphButton(">", ospLimitUp , 5 + 22.20, line + .5) ;
  graphText(messprintf("or every %5d bp", fillLimit * 2),5 + 3.27, line + 2.2) ;
  graphButton("<", ospFillDown, 5, line + 2.0) ;
  graphButton(">", ospFillUp , 5 + 22.20, line + 2.0) ;

  line += 4.2 ;
  graphLine(0, line, mapGraphWidth, line) ;
  line += 1.2 ;

  graphText ("Product length", 3, line) ;
  graphIntEditor ("Min", &productMinLength, 18, line++, 0) ;
  graphIntEditor ("Max", &productMaxLength, 18, line++, 0) ;
  graphIntEditor ("Max Score", &productMaxScore, 12, line, 0) ;
  line += 2.2 ;
  graphButton("Score new pairs of displayed Oligos", ospSubmitPairs, 2,line) ; 

  line += 2.2 ;
  graphLine(0,line, mapGraphWidth, line) ;
  line += 1.2 ;

  selectButtonBox = graphButton("Select/Unselect Oligos", fMapOspSelectOligos, 2,line) ; line += 2.1 ;
  if (selecting) graphBoxDraw (selectButtonBox,BLACK, YELLOW) ;
  graphButton("Order Selected Oligos", fMapOspOrderOligos, 2,line) ; line += 2.1 ;
  graphButton("Discard New OLigos, except if ordered", fMapOspDiscardOligos, 2,line) ;

  line += 2.1 ;
  graphLine(0,line, mapGraphWidth, line) ;
  line += 1.2 ;

  renameButtonBox = graphButton ("Rename Oligo:",  fMapOspRenameOligo, 2, line) ;
   if (renaming) graphBoxDraw (renameButtonBox,BLACK, YELLOW) ;
  /* mhmp 15.09.98  je dois mettre 16 / 10 !!! */
 oligoNameBox = graphTextEditor (" ",oligoName, 16, 17, line,0); 

  line += 2.1 ;
  graphButtons (ospControlMenu, 1, 1, 80) ;
  graphRedraw () ;

  return;
} /* fMapOspControlDraw */

static void fMapOspControlDestroy (void)
{
  ospControlGraph = 0 ;
  selecting = 0 ;
  oldPick = 0 ;
  selecting = 0 ;
  selectButtonBox = 0 ;
  renaming = 0 ;
  renameButtonBox = 0 ;

  return ;
} /* fMapOspControlDestroy */

static void fMapOspControl (void)
{
  Graph old = graphActive () ;
  
  if (graphActivate (ospControlGraph))
    { graphPop () ;
      return ;
    }
  
  ospControlGraph = graphCreate (TEXT_SCROLL, "OSP parameters", 
				 0, 0, 0.4, 0.7) ;
  graphHelp("OSP") ;
  graphRegister (DESTROY, fMapOspControlDestroy) ;
  graphMenu (ospControlMenu) ;
  
  fMapOspControlDraw () ;
  graphActivate (old) ;

  return;
} /* fMapOspControl */

/***************************** eof ************************************/
/**********************************************************************/
