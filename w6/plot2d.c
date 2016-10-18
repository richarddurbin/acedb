/*  File: plot2d.c
 *  Author: Danielle et Jean Thierry-Mieg (mieg@mrc-lmb.cam.ac.uk)
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
 * 	Richard Durbin (MRC LMB, UK) rd@mrc-lmb.cam.ac.uk, and
 *	Jean Thierry-Mieg (CRBM du CNRS, France) mieg@kaa.cnrs-mop.fr
 *
 * Description:
 * Exported functions:
 *     plot2d(char *title, Array a)
 *     where a is an Array of pairs of float.
 * HISTORY:
 * Last edited: May  6 10:23 2003 (edgrif)
 * Created: Mon Apr  5 16:00:10 1993 (mieg)
 * CVS info:   $Id: plot2d.c,v 1.7 2003-05-06 13:13:14 edgrif Exp $
 *-------------------------------------------------------------------
 */

#include <wh/regular.h>
#include <wh/graph.h>
#include <wh/plot.h>

static magic_t Plot2d_MAGIC = "Plot2d";

/*************************************************************************/

typedef struct p2dstuff { 
  magic_t *magic ;
  STORE_HANDLE handle ;
  char title[48], subtitleX[24], subtitleY[24] ;
  int titleBox, subtitleXbox, subtitleYbox ;
  Graph graph ;
  Array xy ;
  float xMin, xMax, yMin, yMax ;
  float x, y ;
  float xCentre, yCentre, xMag, yMag, oldXmag, oldYmag, startXmag ;
  int nx, ny ;
  int leftMargin, topMargin, bottomMargin, rightMargin ;
  int retBox, xyBox ;
  float xStart, yStart ;
  BOOL reticule, regress, isPrinting ;
  char xBuffer [16], yBuffer [16] ;
  double a, b, r, w ;
  float axisShift ;
} *P2D ;

static void plot2dZoomIn (void) ;
static void plot2dZoomOut (void) ;
static void plot2dWhole (void) ;
static void plot2dDraw (P2D p2d) ;
static void plot2dInit (P2D p2d) ;
static void plot2dReticule (int box, double x, double y, int modifier_unused) ;
static void plot2dCentre (double x, double y) ;
static void linearRegression(Array xy, double *ap, double *bp, double *rp, double *wp);
static void plot2dRegression (void) ;
static void plot2dPrint (void) ;
/*************************************************************************/

#define XGRAPH2MAP(x)   (p2d->xCentre  +  p2d->xMag * ((float)(x) - p2d->leftMargin - p2d->nx/2.))
#define XMAP2GRAPH(x)  (((x) - p2d->xCentre) / p2d->xMag + p2d->leftMargin + p2d->nx/2.)

#define YGRAPH2MAP(y) (p2d->yCentre  -  p2d->yMag * ((float)(y) - p2d->topMargin - p2d->ny/2.))
#define YMAP2GRAPH(y)  ((-(y) + p2d->yCentre) / p2d->yMag + p2d->topMargin + p2d->ny/2.)

/*************************************************************************/
/*************************************************************************/

static MENUOPT plot2dMenu[] =
  { 
    {graphDestroy, "Quit"},
    {help,"Help"},
    {plot2dPrint,"Print"},
    {plot2dZoomIn, "ZoomIn"},
    {plot2dZoomOut, "ZoomOut"},
    {plot2dWhole, "Whole"},
    {plot2dRegression, "Lin.Reg."},
    {0, 0}
  } ;

/*************************************************************************/

static P2D currentP2d (char *caller)
{
  P2D p2d = 0 ;

  if (!graphAssFind (&Plot2d_MAGIC,&p2d))
    messcrash("%s() could not find Plot2d on graph", caller);
  if (!p2d)
    messcrash("%s() received NULL  Plot2d pointer", caller);
  if (p2d->magic != &Plot2d_MAGIC)
    messcrash("%s() received non-magic pointer", caller);
  
  return p2d ;
} /* currentVerticalMap */

/*************************************************************************/
/*********** action routines *************/
/*************************************************************************/

static void plot2dResize (void)
{
  P2D p2d = currentP2d ("plot2dResize") ;

  plot2dDraw (p2d) ;
}

/***************************************/

static void plot2dDestroy(void)
{ 
  P2D p2d = currentP2d ("plot2dDestroy") ;
  
  if (p2d) 
    { 
      p2d->magic = 0 ; 
      arrayDestroy (p2d->xy) ;
      messfree (p2d->handle) ; 
    }
}

/********* start off with some utility routines ********/

static BOOL getNxNy(P2D p2d)
{
  int nx, ny ;
  graphFitBounds (&nx, &ny) ;
  nx = nx - p2d->leftMargin - p2d->rightMargin ;
  ny = ny - p2d->topMargin - p2d->bottomMargin ;
  if (nx > 0 && ny > 0)
    {
      p2d->nx = nx ; p2d->ny = ny ;
      return TRUE ;
    }
  p2d->nx = p2d->ny = 3 ;  /* arbitrary non stupid values */
  return FALSE ;
}

/***************************************/

static void plot2dWhole (void)
{ 
  BOOL regress ;

  P2D p2d = currentP2d ("plot2dWhole") ; 

  regress = p2d->regress ;
  plot2dInit (p2d) ;
  p2d->regress = regress ;
  plot2dDraw (p2d) ;
}

/***************************************/

static void plot2dZoomOut (void)
{ 
  P2D p2d = currentP2d ("plot2dZoomIn") ; 

  p2d->xMag = p2d->oldXmag ;
  p2d->yMag = p2d->oldYmag ;

  if (p2d->xMag < 4.* p2d->startXmag) {
    p2d->xMag *= 2 ;
    p2d->yMag *= 2 ;
  }
  
  plot2dDraw (p2d) ;
}

/***************************************/

static void plot2dZoomIn (void)
{ 
  P2D p2d = currentP2d ("plot2dZoomIn") ; 

  p2d->xMag = p2d->oldXmag ;
  p2d->yMag = p2d->oldYmag ;
  if (p2d->xMag > 1.E-5) {
    p2d->xMag /= 2 ;
    p2d->yMag /= 2 ;
  }
  plot2dDraw (p2d) ;
}
/***************************************/

static void plot2dPrint (void)
{ 
  P2D p2d = currentP2d ("plot2dPrint") ; 

  Graph old = graphActive();
  p2d->isPrinting = TRUE ;

  plot2dDraw (p2d) ;
  graphPrint () ;
  graphActivate (old);  
  p2d->isPrinting = FALSE ;
  plot2dDraw (p2d) ;
}

/***************************************/
static void plot2dReticule (int box, double x, double y, int modifier_unused)
{
  float x1, y1, x2, y2 ;

  P2D p2d = currentP2d ("plot2dReticule") ;

  if (box == p2d->titleBox)
    { graphTextEntry (p2d->title, 0, 0, 0, 0) ;
      return ;
    }
  if (box == p2d->subtitleXbox)
    { graphTextEntry (p2d->subtitleX, 0, 0, 0, 0) ;
      return ;
    }
  if (box == p2d->subtitleYbox)
    { graphTextEntry (p2d->subtitleY, 0, 0, 0, 0) ;
      return ;
    }
  if (box != p2d->retBox){
    if (p2d->reticule) {
      p2d->xMag = p2d->oldXmag ;
      p2d->yMag = p2d->oldYmag ;
      p2d->reticule = FALSE ;
      plot2dDraw (p2d) ;
    }
    return ;
  }
  graphBoxDim (p2d->retBox, &x1, &y1, &x2, &y2) ;
  p2d->x = XGRAPH2MAP(x + x1) ;
  p2d->y = YGRAPH2MAP(y + y1) ;
  sprintf (p2d->xBuffer, "%g", p2d->x) ;
  sprintf (p2d->yBuffer, "%g", p2d->y) ;
  p2d->reticule = TRUE ;
  p2d->xMag = p2d->oldXmag ;
  p2d->yMag = p2d->oldYmag ;
  plot2dDraw (p2d) ;
}  
/***************************************/
static void plot2dCentre (double x, double y)
{
  float x1, y1, x2, y2 ;
  P2D p2d = currentP2d ("plot2dCentre") ;

  graphBoxDim (p2d->retBox, &x1, &y1, &x2, &y2) ;
  if ( x >= x1 && x <= x2 && y >= y1 && y <= y2) {
    p2d->xCentre = XGRAPH2MAP(x) ;
    p2d->yCentre = YGRAPH2MAP(y) ;
    p2d->xMag = p2d->oldXmag ;
    p2d->yMag = p2d->oldYmag ;
    plot2dDraw (p2d) ;
  }
  else return ;
} 
/**************************************************/
/**************************************************/

static void plot2dXscale (P2D p2d)
{

  float x, xp1, start, end, xMag, sdx ;
  float xPos, yPos, fin ;
  double xx, iDeb, iFin, xxp1 ;
  int xDiv, dx, ddx, i, j, oldi, nx, ny ;
  int xxx ;
  int xMul, isScale, scale= 1 ;
  yPos = p2d->topMargin + p2d->ny ;
  xPos = (p2d->leftMargin + p2d->nx) * 0.75 ;

  start = XGRAPH2MAP(p2d->leftMargin) ;
  end = XGRAPH2MAP(p2d->leftMargin + p2d->nx) ;
  p2d->xStart = start ;
  
  graphFitBounds (&nx, &ny) ;

  xMul = 1. ;
  if (start == end) end = start + 1 ;
  while ((end - start) * xMul < 1.)
    xMul *= 10 ;
  start *= xMul ;
  end *= xMul ;
  if (abs(end) > abs(start))
    i = abs(end) ;
  else 
    i = abs(start) ;
  oldi = i ;
  j = 1 ;
  while (i /= 10) j++ ; /* count chars */
  xMag = (nx - p2d->leftMargin) /  (end - start);
  dx = 5  * j / xMag ;
  dx = freeMainRoundPart(dx) ;
  if(dx <= 0) dx = 1;

  xDiv = 1 ;
  ddx = dx ;
   while (ddx && !(ddx % 10))
    {
      xDiv *= 10 ;
      ddx /= 10 ;
    }
  if (xDiv > 1)
    xDiv /= 10 ;

  if (xDiv > 1 )
    {
      i = oldi/ xDiv ;
      j = 1 ;
      while (i /= 10) j++ ; 
      dx = 5  * j / xMag ;
      dx = freeMainRoundPart(dx) ;
      if(dx <= 0) dx = 1;
    }
  /* mhmp 04.05.98 */
  if (dx * xMag < 2.5)
    dx *= 2 ;
  isScale = 0 ;
  if (xDiv > 1 || xMul > 1)
    {
      if (xDiv > xMul)
	{
	  scale = xDiv / xMul ;
	  isScale = 1 ;
	}
      else
	{
	  scale = xMul / xDiv ;
	  isScale = -1 ;
	}
    }
  if (isScale >= 0) /* mhmp 06.07.99 > ---> >= */
    graphText (messprintf ("scale x: 1/%d", scale), xPos, yPos + 3.0) ;
  else
    graphText (messprintf ("scale x: 1*%d", scale), xPos, yPos + 3.0) ;

  iDeb = utMainPart(utArrondi(start)) ;
  iFin = end ;
/*recalage a gauche */
  if (end - iDeb > dx * 100000) 
    iDeb = start ;
    else
      for (xx = iDeb ; xx < end ; xx += dx)
	if (xx > start) {
	  iDeb =  xx - dx ;
	  iFin = 2*p2d->xCentre * xMul - iDeb ;
	  break ;
	}

  fin = XMAP2GRAPH((float)iFin/xMul) ;
  p2d->oldXmag = p2d->xMag ;
  p2d->xMag = (p2d->xCentre - (float)iDeb/xMul) / (p2d->nx/2.) ;
  if (p2d->xMag < 1.E-5)
    return ;
  for (xx = iDeb ; xx <= iFin ; xx += dx)
    { 
      x = XMAP2GRAPH((float)xx /xMul) ;
      if (x >= p2d->leftMargin - 0.0001) {
	xxx = utArrondi((float)xx / xDiv + 0.0001);
	graphLine (x, yPos + 1.5, x, yPos + 0.5) ;
	graphText (messprintf ("%d",xxx), x, yPos + 2.0) ;
	/*tirets*/
	xxp1 = xx + dx ;
	xp1 = XMAP2GRAPH((float)xxp1 /xMul ) ;
	sdx = (xp1 - x) / 5. ;
	for (i = 0 ; i < 4 ; i++) {
	  x += sdx ;
	  if (x <= fin + 0.0001)
	    graphLine (x, yPos + 1., x, yPos + 0.5) ;
	}
      }
    }

  graphLine (XMAP2GRAPH((float)iDeb/xMul), yPos + p2d->axisShift,
	     fin, yPos + p2d->axisShift) ;
}

/**************************************************/

static void plot2dYscale (P2D p2d)
{

  float y, yp1, start, end, yMag, sdy ;
  float xPos, yPos, fin ;
  double yy, iDeb, iFin, yyp1 ;
  int yDiv, dy, ddy, nx, ny, i ;
  int yyy ;
  int yMul, isScale, scale = 1 ;
  char *cp ;
  xPos = p2d->leftMargin ;
  yPos = p2d->topMargin + p2d->ny ;

  start = YGRAPH2MAP(p2d->topMargin + p2d->ny) ;
  p2d->yStart = start ;
  end = YGRAPH2MAP(p2d->topMargin) ;

  graphFitBounds (&nx, &ny) ;

  yMul = 1. ;
  if (start == end) end = start + 1 ;
  while ((end - start) * yMul < 1.)
    yMul *= 10 ;
  start *= yMul ;
  end *= yMul ;

  yMag = (ny - p2d->topMargin - p2d->bottomMargin) / (end - start) ;
  dy = 5. / yMag ;
  dy = freeMainRoundPart (dy) ;
  if(dy <= 0) dy = 1;
  if (dy * yMag < 2.5)
    dy *= 2 ;
  yDiv = 1 ;
  ddy = dy ;
   while (ddy && !(ddy % 10))
    {
      yDiv *= 10 ;
      ddy /= 10 ;
    }
   if (yDiv > 1)
     yDiv /= 10 ;

  isScale = 0 ;
  if (yDiv > 1 || yMul > 1)
    {
      if (yDiv > yMul)
	{
	  scale = yDiv / yMul ;
	  isScale = 1 ;
	}
      else
	{
	  scale = yMul / yDiv ;
	  isScale = -1 ;
	}
    }
  if (isScale >= 0) /* mhmp 06.07.99 > ---> >= */
    graphText (messprintf ("scale y: 1/%d", scale), 2,yPos + 3) ;
  else 
    graphText (messprintf ("scale y: 1*%d", scale), 2,yPos + 3) ;    

  iDeb = utMainPart(utArrondi(start)) ;
  iFin = end ;
  /* recalage en bas */
  if (end - iDeb > dy * 100000)
    iDeb = start ;
    else
      for (yy = iDeb ; yy < end ; yy += dy)
	if (yy > start) {
	  iDeb = yy - dy ;
	  iFin = 2*p2d->yCentre * yMul - iDeb ;
	  break ;
	}

  fin = YMAP2GRAPH((float)iFin/yMul) ;
  if (fin < p2d->topMargin)
    fin = p2d->topMargin ;
  p2d->oldYmag = p2d->yMag ;
  p2d->yMag = (p2d->yCentre - (float)iDeb/yMul) / (p2d->ny/2.) ;
  if (p2d->yMag < 1.E-5)
    return ;
  for (yy = iDeb ; yy <= iFin ; yy += dy)
    { 
      y = YMAP2GRAPH((float)yy / yMul) ;
      if (y <= p2d->topMargin + p2d->ny + 0.0001) {
	yyy = utArrondi((float)yy / yDiv + 0.0001);
	graphLine (xPos - 1.5, y, xPos - 0.5, y) ;
	cp = messprintf ("%d", yyy) ;
	graphText (cp, p2d->leftMargin - strlen(cp)-1.5,y-.5) ;
	/*tirets*/
	yyp1 = yy + dy ;
	yp1 = YMAP2GRAPH((float)yyp1 /yMul ) ;
	sdy = (yp1 - y) / 5. ;
	for (i = 0 ; i < 4 ; i++) {
	  y += sdy ;
	  if (y >= fin - 0.0001)
	    graphLine (xPos - 1., y, xPos - 0.5, y) ;
	}  
      }
    }

  graphLine (xPos - p2d->axisShift, YMAP2GRAPH((float)iDeb/yMul), 
	     xPos - p2d->axisShift, fin) ;
}

/***************************************/
/* intersection de la droite de regression avec le cadre */
/* y = ax + b  topMargin (+ ny) leftMargin (+ nx) */

static void regressInit (P2D p2d, float *x1, float *x2, float *y1, float *y2, 
			 int *nbInterp)
{
  float x, y ;
  int nbInter ;

  nbInter = 0 ;
  x = XMAP2GRAPH((YGRAPH2MAP(p2d->topMargin) - p2d->b) / p2d->a );
  if (x >= p2d->leftMargin && x <= p2d->leftMargin + p2d->nx) {
    nbInter++ ;
    *x1 = x ;
    *y1 = p2d->topMargin ;
  }
  x = XMAP2GRAPH((YGRAPH2MAP(p2d->topMargin + p2d->ny) - p2d->b) / p2d->a) ;
  if (x >= p2d->leftMargin && x <= p2d->leftMargin + p2d->nx) {
    nbInter++ ;
    if (nbInter < 2) {
      *x1 = x ;
      *y1 = p2d->topMargin + p2d->ny ;
    }	
    else {
      *x2 = x ;
      *y2 = p2d->topMargin + p2d->ny;
      *nbInterp = nbInter ;
      return ;
    }
  }
  y = YMAP2GRAPH(p2d->a * XGRAPH2MAP(p2d->leftMargin) + p2d->b) ;
  if ( y >= p2d->topMargin && y <= p2d->topMargin + p2d->ny) {
    nbInter++ ;
    if (nbInter < 2) {
      *x1 = p2d->leftMargin ;
      *y1 = y ;
    }	
    else {
      *x2 = p2d->leftMargin ;
      *y2 = y ;
      *nbInterp = nbInter ;
      return ;
    }
  }   
  y = YMAP2GRAPH(p2d->a * XGRAPH2MAP(p2d->leftMargin + p2d->nx) + p2d->b) ;
  if ( y >= p2d->topMargin && y <= p2d->topMargin + p2d->ny) {
    nbInter++ ;
    *x2 = p2d->leftMargin + p2d->nx ;
    *y2 = y ;
    *nbInterp = nbInter ;
  }
}
/***************************************/

static void plot2dDraw (P2D p2d)
{
  int box ;
  int i ;
  POINT2D *pp ;
  int nbInter ;
  float x = 0, y = 0, eps = 0.1, shift;
  float xPos = 0, yPos = 0, x1, x2, y1, y2 ;
  graphClear () ;
  if (!getNxNy (p2d))
    { graphRedraw() ; return ; }
  xPos = (p2d->leftMargin + p2d->nx) * 0.75 ;
  yPos = p2d->topMargin + p2d->ny ;
  p2d->titleBox = graphTextEntry(p2d->title, 47, 2,  0.1, 0) ;
  /* mhmp 06.07.99 xPos <--> 2 Xbox <-> Ybox */
  p2d->subtitleXbox = graphTextEntry(p2d->subtitleX, 23, xPos,  yPos + 4., 0) ;
  p2d->subtitleYbox = graphTextEntry(p2d->subtitleY, 23, 2,  yPos + 4., 0) ;
  if (!p2d->isPrinting)
    graphButtons (plot2dMenu, 2, 1.8, p2d->nx) ;
  box = graphBoxStart () ;
  graphBoxEnd() ;
 
 /* les axes */
  plot2dXscale (p2d) ;
  if (p2d->xMag < 1.E-5)
    goto fin ;

  plot2dYscale (p2d) ;
  if (p2d->yMag < 1.E-5)
      goto fin ;

  /* le cadre */
  x1 = p2d->leftMargin ;
  x2 = p2d->leftMargin + p2d->nx ;
  p2d->retBox = graphBoxStart() ;
  shift = p2d->axisShift ;
  graphLine (x1 - shift/2, p2d->topMargin - shift, 
	     x2 + shift, p2d->topMargin - shift) ;
  graphLine (x2 + shift, p2d->topMargin - shift, 
	     x2 + shift, p2d->topMargin + p2d->ny + shift/2) ;

  /* les points */
  for (i = 0 , pp = arrp (p2d->xy, 0, POINT2D) ; 
       i < arrayMax (p2d->xy) ; i++, pp++)
    {
      x = XMAP2GRAPH(pp->x) ;
      y = YMAP2GRAPH(pp->y) ;
      if (x >= p2d->leftMargin - eps && x <= p2d->leftMargin + p2d->nx + eps &&
	  y >= p2d->topMargin - eps && y <= p2d->topMargin + p2d->ny + eps)
	graphCircle (x, y, 0.02) ;
    }

  /* droite de regression mhmp 25.05.99 */
  if (p2d->regress) {
    regressInit (p2d, &x1, &x2, &y1, &y2, &nbInter) ;
    if (nbInter == 2) {
      graphColor (RED) ;
      graphLine (x1, y1, x2, y2) ;
      graphColor (BLACK) ;
    }
    graphBoxStart() ;
    graphColor (RED) ;
    graphText (messprintf("Regression y = %g * x + %g", p2d->a, p2d->b),
	       20, 3) ;
    graphText (messprintf ("r = %g w = %g", p2d->r, p2d->w), 31, 4) ;
    graphColor (BLACK) ;
    graphBoxEnd() ;
  }
  graphBoxEnd() ;

  /* le reticule */
  if (p2d->reticule) {
    graphText("x = ", 2, 3) ;
    graphText("y = ", 2, 4) ;
    p2d->xyBox = graphBoxStart() ;
    graphTextPtr (p2d->xBuffer, 6,  3 , 8) ;
    graphTextPtr (p2d->yBuffer, 6,  4 , 8) ;
    graphBoxEnd () ;
    graphBoxMenu(p2d->xyBox, plot2dMenu);
    x = XMAP2GRAPH(p2d->x);
    y = YMAP2GRAPH(p2d->y) ;
    graphLine (x - 1, y, x + 1, y) ;
    graphLine (x, y - 1, x, y + 1) ;
  }

  graphRedraw() ;
  return ;
fin:
    { graphText ("You are a little zoom-zoom!", 2, 5) ;
      graphRedraw () ;
    }

}

/**************************************************************/

static void plot2dInit (P2D p2d)
{
  float l, xMin, xMax , yMin, yMax ;
  int i ; 
  POINT2D *pp ; 
  if (0) { /*test petites valeurs */
    pp = arrp(p2d->xy, 0, POINT2D) ;
    for (i=0; i< arrayMax(p2d->xy) ; i++, pp++)
      { 
	pp->x = 35 + pp->x / 10000  ;
	pp->y *= 100000 ;
      }
  }
pp  = arrp(p2d->xy, 0, POINT2D) ;
  xMin = xMax = pp->x ;
  yMin = yMax = pp->y ;
  
  for (i=0; i< arrayMax(p2d->xy) ; i++, pp++)
    { 
      if (pp->x > xMax)
	xMax = pp->x ;
      if (pp->x < xMin)
	xMin = pp->x ;
      if (pp->y > yMax)
	yMax = pp->y ;
      if (pp->y < yMin)
	yMin = pp->y ;
    }
  p2d->xMin = xMin ;
  p2d->xMax = xMax ;
  p2d->yMin = yMin ;
  p2d->yMax = yMax ;

  getNxNy(p2d) ;
    
  l = p2d->xMax - p2d->xMin ;
  p2d->xCentre = p2d->xMin + l/2.0 ;
  /*  p2d->xMag = l/ (.9 * p2d->nx) ;mhmp 29.04*/
  p2d->xMag = l/ (1.0 * p2d->nx) ;
  p2d->oldXmag = p2d->xMag ;
  p2d->startXmag = p2d->xMag ;
  l = p2d->yMax - p2d->yMin ;
  p2d->yCentre = p2d->yMin + l/2.0 ;
  /*  p2d->yMag = l/ (.9 * p2d->ny) ;mhmp 29.04*/
  p2d->yMag = l/ (1.0 * p2d->ny) ;
  p2d->oldYmag = p2d->yMag ;
  p2d->reticule = FALSE ;
  p2d->regress = FALSE ;
  p2d->isPrinting = FALSE ;
  p2d->axisShift = 0.5 ;
}

/*****************************************/

void plot2d (char *title, char *subtitleX, char *subtitleY, Array xy)
{
  P2D p2d = 0 ;
  STORE_HANDLE hh = 0 ;

  if (!arrayExists (xy) || !arrayMax(xy) ||
      xy->size != sizeof (POINT2D))
    return ;
  hh = handleCreate () ;

  p2d = (P2D) halloc (sizeof (struct p2dstuff), hh) ;
  p2d->handle = hh ;
	     
  if (!title || !*title)
    title = "Plot" ;
  sprintf(p2d->title, "%s", title) ;
  sprintf(p2d->subtitleX, "%s", subtitleX) ;
  sprintf(p2d->subtitleY, "%s", subtitleY) ;

  p2d->xy = xy ;
  p2d->topMargin = 6 ;/* mhmp 17.03.99 was 5*/
  p2d->bottomMargin = 5 ;/* mhmp 21.05.99 was 4*/
  p2d->leftMargin = 10 ; /* mhmp 25.05.99 was 4 */
  p2d->rightMargin = 2 ;

  if (graphExists(p2d->graph))
    { graphActivate(p2d->graph) ;
      graphPop() ;
    }
  else 
    { 
      if (!graphCreate(TEXT_FIT,title,.2,.2,.6,.5))
	return ;
      graphRetitle (title) ;
      p2d->graph = graphActive() ;
      p2d->magic =  &Plot2d_MAGIC ;
      graphAssociate (&Plot2d_MAGIC,p2d) ;
      graphRegister (DESTROY,plot2dDestroy) ;
      graphRegister (RESIZE,plot2dResize) ;
      
      graphRegister (PICK, plot2dReticule) ;
      graphRegister (MIDDLE_DOWN, plot2dCentre) ;
      /*     graphRegister (MIDDLE_DRAG,  plot2dDrag) ;
      graphRegister (MIDDLE_UP,  plot2dUp) ;
      */
      graphMenu (plot2dMenu) ;
    }
  plot2dInit (p2d) ;  /* sets ->centre, ->mag */
  linearRegression (p2d->xy, &p2d->a, &p2d->b, &p2d->r, &p2d->w) ;
  plot2dDraw (p2d) ;	
}

/*************************************************************************/
/*************************************************************************/
/* Solves, y = a x + b , in a and b   a defaults to *ap  if too few data given */
static void linearRegression(Array xy, double *ap, double *bp, double *rp, double *wp)
{
  int i, max ;
  double r, w, xm,ym,xym , x2m, y2m ,b, a;

  if(!xy || !(max = arrayMax(xy)))
    {
      messout("linearRegression received a null array") ;
      *ap = * bp = *rp = *wp  = 0 ;
      return;
    }
  if(xy->size != sizeof(POINT2D))
    messcrash(
  "linearRegression received a wrong type of Array, should be  a pair of doubles ") ;

  i = max ;
  xm = ym = xym = 0 ;
  while (i--)
    {
      xm += arr(xy,i,POINT2D).x ; ym += arr(xy,i,POINT2D).y;
    }
  xm /= max ; ym /= max;
  x2m = y2m =xym =0;
  i = max ;
  while (i--)
    {
      xym += (arr(xy,i,POINT2D).x-xm) *( arr(xy,i,POINT2D).y-ym) ;
      x2m +=  (arr(xy,i,POINT2D).x-xm) *( arr(xy,i,POINT2D).x-xm) ;
      y2m +=  (arr(xy,i,POINT2D).y-ym) *( arr(xy,i,POINT2D).y-ym) ;
    }
  if(max >3 && x2m>0 && y2m >0)
    { r = xym / sqrt((double)x2m * y2m) ;
      if (r >= 1)    /* mhmp 28.05.99 */
	w = 0.999 ;
      else
   	w = r;
	i = max - 3 ;
	w = .5 * log((1+w) / (1-w)) * sqrt((double) i) ;
     }
  else
    r = w = 0 ;

  i = max ;
  a = 0 ;
  while (i--)
      a += (arr(xy,i,POINT2D).x - xm) * arr(xy,i,POINT2D).y ;

  if (x2m)
    a /= x2m ;
  else
    a = *ap ;  /* default */

  b = ym - a*xm ;
  
  *ap = a ;
  *bp = b ;
  *rp = r ;
  *wp = w ;
}

static void plot2dRegression (void)
{		 
  P2D p2d = currentP2d ("plot2dRegression") ; 

  p2d->regress = !p2d->regress ;
  p2d->xMag = p2d->oldXmag ;
  p2d->yMag = p2d->oldYmag ;
  plot2dDraw (p2d) ;
}
/*************************************************************************/
/*************************************************************************/
