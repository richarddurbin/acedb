/*  File: plot.c
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
 * Description: to plot various sorts of data
 * Exported functions: plotHisto()
 * HISTORY:
 * Last edited: May  6 10:23 2003 (edgrif)
 * * Apr  4 13:51 1995 (rd)
 * Created: Tue May 19 22:57:41 1992 (rd)
 * CVS info:   $Id: plot.c,v 1.30 2003-05-06 13:13:13 edgrif Exp $
 *-------------------------------------------------------------------
 */

#include <wh/acedb.h>
#include <wh/aceio.h>
#include <wh/display.h> 
#include <wh/plot.h>
#include <wh/chrono.h>

static int PLOT_MAGIC = 237165;	/* also use address as graphAss handle */

typedef struct LOOKstruct
  { int   magic ;
    Array a, originalArray ;
    char title[48], subtitle[24] ;
    char fileName[FIL_BUFFER_SIZE] ;
    char dirName[DIR_BUFFER_SIZE] ;
    int  xyBox, retBox, sBox, zoneBox,stepBox, titleBox, subtitleBox ;
    int s , minX, maxX, startStep, step ; 
  /* mhmp 10.03.99 float shift + xMax */
    float x, y, minY, maxY, shift, xMin, xEnd, xMax, scale, startScale ;
    int xMul, xDiv,facY ;
    char sBuffer [8], zoneBuffer [24], xBuffer [16], yBuffer [16], stepBuffer [10] ;
    BOOL reticule, isStepping, isSmoothing, plus1, isPrinting ;
    float oldx, oldy ;
    float Xmag, Ymag,axisShift ;
    float firstX, firstY ;
    int leftMargin, topMargin ;
    Graph graph ;
  } *LOOK ;

#define LOOKGET(name)     LOOK look ; \
       			  if (!graphAssFind (&PLOT_MAGIC, &look)) \
		            messcrash ("graph not found in %s", name) ; \
                          if (look->magic != PLOT_MAGIC) \
                            messcrash ("%s received a wrong pointer",name)

static Graph plotDisplay (void) ;
static void logxTransform (void) ;
static void logyTransform (void) ;
static void plotCorrel (void) ;
static Array convert (Array a) ;

#define XX(_x)  (look->leftMargin + (_x - look->firstX) * look->Xmag)
#define YY(_y)  (look->topMargin + (look->maxY - _y) * look->Ymag)

static Array plotCurrHis = 0 ; /* to know the created Histo */
static int plotCurrNb ;
static void plotCopy (void) ;
static void plotSave (void) ;
static void plotLoad (void) ;
static void plotInit (void) ;
static void plotLoadAscii (void) ;
static void plotRefresh (void);
static void smoothingBoxEntry (char *cp) ;
static void zoneBoxEntry (char *cp) ;
static void histoStatistics(void);
static void histoExport (void);
static void histoImport (void);
static void plotPrint (void) ;
/******************************************************************/

static MENUOPT histoMenu[]={
        {graphDestroy, "Quit"},
        {help, "Help"},
        {plotPrint, "Print"},
	{plotRefresh, "Refresh"},
        {histoStatistics, "Statistics"},
	{histoExport, "Export"},
	{histoImport, "Import"},
	{logyTransform, "log(y) :: x"},
	{logxTransform, "y :: log(x)"},
	{plotCorrel, "z(t)= y(x) y(x+t)"},
	{plotCopy, "Duplicate"},
	{plotSave, "Save"},
	{plotLoad, "Load"},
	{plotLoadAscii, "Load Ascii"},
	{plotHistoRemove, "Kill all histograms"},
        {0,0} };

/*************************************************************************/
/*************************************************************************/

static void plotDestroy(void)
{
  LOOKGET("plotDestroy") ;

  look->magic = 0 ;
  
  arrayDestroy (look->a) ;
  arrayDestroy (look->originalArray) ;
  messfree (look) ;

  graphAssRemove (&PLOT_MAGIC) ;
  plotCurrNb-- ;
  if (!plotCurrNb)
    arrayDestroy(plotCurrHis) ;
}

/******************************************************************/

static void makeReticule(LOOK look)
{ 
/* mhmp 08.03.99 */  
  float x ;
  x =  look->shift + look->x/look->scale ;
  sprintf (look->xBuffer, "%g", x/look->xDiv) ; 
  sprintf (look->yBuffer, "%g", look->y) ;
  look->reticule = TRUE ;
  plotDisplay () ;

}

/******************************************************************/

static void moveReticule (int box, double x, double y, int modifier_unused)
{
  LOOKGET("moveReticule") ;

  if (box == look->sBox)
    { graphTextEntry (look->sBuffer, 0, 0, 0, 0) ;
      return ;
    }
  if (box == look->stepBox)
    { graphTextEntry (look->stepBuffer, 0, 0, 0, 0) ;
    return ;
    }
  if (box == look->zoneBox)
    { graphTextEntry (look->zoneBuffer, 0, 0, 0, 0) ;
      return ;
    }  
  if (box == look->titleBox)
    { graphTextEntry (look->title, 0, 0, 0, 0) ;
      return ;
    }
  if (box == look->subtitleBox)
    { graphTextEntry (look->subtitle, 0, 0, 0, 0) ;
      return ;
    }
  if(box != look->retBox )
    return ;

  y -= 3 ;   /* axis overhangs by 3 units */
  y /= look->Ymag ;
  /* mhmp 28.04.99 x-0.5*/
  x -= look->axisShift ;
  x /= look->Xmag ;
  x += look->firstX ;
  y = -y + look->maxY*look->facY ; /* coord is relative to the axis box */
  if(x == look->x && y == look->y)
    return ;
  look->x = x ; look->y = y ;
  makeReticule(look) ;
}
/***************************************/

static void plotPrint (void)
{ 
  Graph old = graphActive();

  LOOKGET("plotPrint") ;

  look->isPrinting = TRUE ;
  plotDisplay () ;
  graphPrint () ;
  graphActivate (old);  
  look->isPrinting = FALSE ;
  plotDisplay () ;
}

/*************************************************************************/

static void histoStatistics(void)
{ 
  double s = 0, s1, y, yt = 0, ym, yt2 = 0, xt = 0, xt2 = 0, xm = 0 ;
  int x, x1 ;
  Array a ;
  LOOKGET("histoStatistics") ;

  a = look->a ;
  if (!a || !arrayMax(a))
    return ;

  for (x = 0 ; x < arrayMax(a) ; ++x)
    if ((y = arr(a,x,float)))
      { yt += y ;
	yt2 += y*y ;
	x1 = x + look->shift ;
	xt += y*x1 ;
	xt2 += y*x1*x1 ;
      }

  for (x = 0 ; x < arrayMax(a) ; ++x)
    if ((y = arr(a,x,float) > 0))
      { s1 = y/yt ;
	s -= s1 * log (s1) ;
      }
	
  s1 = 2.0 ; s /= log (s1) ; /* go to log base 2 */

  ym = yt / (float)arrayMax(a) ;
  if (arrayMax(a) > 1)
    yt2 = sqrt ((yt2 - ym*yt) / (arrayMax(a)-1)) ;
  else
    yt2 = 0 ;

  xm = xt/yt ;
  if (yt > 1)
    xt2 = sqrt ((xt2 - xm*xm*yt) / (yt-1)) ;
  else
    xt2 = 0 ;

  messout (messprintf ("Cumulated x = %f Average x = %f, sigma = %f\n"
		       "Cumulated y = %f Average y = %f, sigma = %f\n Entropy %g bit/pixel",
		       xt, xm, xt2, yt, ym, yt2, s)) ;
}

/*************************************************************************/

static void histoExport (void)
{ 
  int *fp ;
  int i ;
  ACEOUT histo_out;
  LOOKGET("histoExport") ;

  if (look->originalArray && arrayMax(look->originalArray))
    {
      histo_out = aceOutCreateToChooser ("Histo export file",
					 look->dirName, look->fileName, 
					 "", "w", 0);
      if (!histo_out)
	return;

      aceOutPrint (histo_out, "%g\n", look->shift) ;
      aceOutPrint (histo_out, "%g\n", look->xEnd) ;
      aceOutPrint (histo_out, "%g\n", look->startScale) ;
      aceOutPrint (histo_out, "%d\n", look->xMul) ;
      
      i = arrayMax (look->originalArray) ;
      fp = arrp(look->originalArray, 0, int) ;
      while (i--)
	aceOutPrint (histo_out, "%d\n", *fp++) ;

      aceOutDestroy (histo_out);
    }
  else
    messout ("Sorry, no histogram");

  return;
} /* histoExport */
/*************************************************************************/

static void histoImport (void)
{
  int max=0, i ; 
  Array a ;
  float ff ;
  int nn;
  ACEIN histo_in;
  LOOKGET("histoImport") ;
  
  look->title[0] = '\0' ;
  look->subtitle[0] = '\0' ;

  histo_in = aceInCreateFromChooser("Text File of Integers",
				    look->dirName,look->fileName,
				    "histo", "r", 0);
  if (!histo_in)
    return;
  
  a = arrayCreate (1000, int) ;

  if (aceInCard(histo_in) && aceInFloat(histo_in, &ff))
    look->shift = ff ;
  if (aceInCard(histo_in) && aceInFloat(histo_in, &ff))
    look->xEnd = ff ;
  if (aceInCard(histo_in) && aceInFloat(histo_in, &ff))
    look->startScale = ff ;
  if (aceInCard(histo_in) && aceInInt(histo_in, &nn))
    look->xMul = nn ;
  i = 0 ;
  while (aceInCard(histo_in))
    {
      if (aceInInt(histo_in, &nn))
	array (a, i++, int) = nn ;
    }
  max = arrayMax(a) ;

  aceInDestroy (histo_in);

  plotInit() ;
  if(!max)
    {
      messout("I read an empty file") ;
      arrayDestroy(a) ;
      return;
    }
  if (!look->title || !*look->title)
    sprintf(look->title, "%s", look->fileName) ;
  graphRetitle (look->title) ;
  arrayDestroy (look->originalArray) ;
  arrayDestroy (look->a) ;    
  look->originalArray = a ;
  look->a = convert (a) ;
  look->minX = 0 ; /* included */
  look->maxX = max ;  /* excluded as usual in Arrays */
  
  plotDisplay() ;

  return;
} /* histoImport */
/******************************************************************/

static void logyTransform (void)
{ 
  double x, log10 ;
  int k ;
  Array a ;
  LOOKGET("logTransform") ;

  a = look->a ;

  if (!a || arrayMax(a) < 10) 
    return ;
  log10 = log((double)10.0) ;
  for (k = 0 ; k < arrayMax(a) ; k++ )
    { x = arr (a, k, float) ;
      if (x < 1.0/1000) x = 1.0/1000 ;
      array (a, k, float) = log(x)/log10 ;
    }

  plotDisplay () ;
}

/*************************************************************************/

static void logxTransform (void)
{ 
  float *fp ; double x ;
  int i, k, max ;
  Array a, b ;
  LOOKGET("logTransform") ;

  a = look->a ;

  if (!a || arrayMax(a) < 10) 
    return ;
  
  if (!a || arrayMax(a) < 10) 
    return ;
  
  max = arrayMax (a) ;
  b = arrayCreate (max, float) ;

  array (b, max - 1 , float) = 0 ; /* make room */
  fp = arrp (b, 0, float) ;
  for (k = 100 ; k < arrayMax(a) ; k++ )
    { x = k/100.0 ;
      i = 5000 * log10 (x) ;
      array (b, i, float) = arr (a, k, float) ;
    }
  arrayDestroy (look->a) ;
  look->a = b ;

  plotDisplay () ;
}

/*************************************************************************/

static void plotCorrel (void)
{ 
  double x = 0 ; float *fp = 0, *fip, *fjp, fmax=0, fmin = 0 ; 
  int i,j, t,max, *ip ;
  Array a, b ;
  LOOKGET("plotCorrel") ;

  a = look->a ;

  if (!a || arrayMax(a) < 10) 
    return ;
  
  max = arrayMax (a) ;
  b = arrayCreate (max, float) ;

  array (b, max - 1 , float) = 0 ; /* make room */
  fp = arrp (b, 0, float) ; fmax = 0 ;
  for (t = 0 ; t < max/2 ; t++)
    {
      x = 0 ;
      for (i = 0, j = t,
	     fip = arrp (a, i, float), 
	     fjp = arrp (a, j, float); i < max ;
	   i++, j++, fip++, fjp++)
	{ 
	  if (j == max) fjp = arrp (a, 0, float) ;  /* circularize to ensure normalisation */
	  x += (*fip) * (*fjp) ;
	}
      *fp++ = x ;
      if (!t) fmin = fmax = x ;
      if (x > fmax) fmax = x ;
      if (x < fmin) fmin = x ;
    }
  for (t = max/2; t < max ; t++) *fp++ = x ;
  for (t = 0, fp = arrp (b, 0, float);
       t < max ; fp++, ip++, t++) 
    *fp *= 100.0/fmax ; 
    
  arrayDestroy (look->a) ;
  look->a = b ;

  plotDisplay () ;
}
/*************************************************************************/
static void steppingBoxEntry (char *cp)
{ 
  int s, step, level, i, j, maxa, maxb ;
  int *fa = 0 ;
  float *fb = 0, x = 0 ;
  Array a, b ;

  LOOKGET("steppingBoxEntry") ;
 
  a = look->originalArray ;
  if (!a || arrayMax(a) < 10) 
    return ;
  level = freesettext (cp, "") ;
  freecard(level) ;
  s = -1 ;
  freeint (&s) ;
  freeclose (level) ;
  maxa = arrayMax (a) ;
  step = s/look->startStep ;
  s = step * look->startStep ;
  if ( step < 1 || step > maxa /2) {
    step = 1  ;
    s = look->startStep ;
  }
  look->plus1 = FALSE ;
  maxb = maxa/step ;
  if (maxa%step) {
    maxb++ ;
    look->plus1 = TRUE ;
  }
  b = arrayCreate (maxb, float) ;

  array (b, maxb - 1 , float) = 0 ;
  fb = arrp (b, 0, float) ;
  for (i=0 ; i < maxb ; i++) {
    x = 0 ;
    for (j = 0, fa = arrp (a, step*i + j, int) ; j < step && j + step*i < maxa; j ++, fa++)
      x += *fa ;
    *fb++ = x ;
  }
  arrayDestroy (look->a) ;
  look->a = b ; 
  look->step = s ;
  look->scale = 1./(float)look->step ;
  look->isStepping = TRUE ;
  if(!(look->isSmoothing))
    smoothingBoxEntry(look->sBuffer) ;
  look->isSmoothing = FALSE ;

}
/*************************************************************************/
static void smoothingBoxEntry (char *cp)
{ 
  float *fp, *fjp, *ip ;
  int i, j, k , k1, k2, s, t, level, di, ifac ;
  Array a, b ;
  LOOKGET("smoothingBoxEntry") ;

  look->isSmoothing = TRUE ;

  if (!(look->isStepping))
    steppingBoxEntry (look->stepBuffer) ;
  look->isStepping = FALSE ;
  a = look->a ;

  level = freesettext (cp, "") ;
  freecard(level) ;
  s = -1 ;
  freeint (&s) ;
  freeclose (level) ;

  s = s/2 + 1 ;
  if (s < 1) {
    plotDisplay() ;
    return ;
  }
  t = s * s ; /* sum of the triangle */

  if (!a || arrayMax(a) < 2*s + 3)
    return ;
  if (!arrayMax (a))
    return ;

  
  i = arrayMax (a) ;
  look->a = b = arrayCreate (i, float) ;

  array (b, i - 1 , float) = 0 ; /* make room */
  for (k = - s + 1 ; k < s ; k++)
    { ip = arrp(a, k > 0 ? k : 0 , float) ;
      fp = arrp(b, k < 0 ? -k : 0 , float) ;
      k1 = k >= 0 ? k : - k ;
      i = arrayMax (a) - k1 ;
      k2 = s - k1 ;
      while (i--)
	*fp++ += k2 * (*ip++) ;
    }
  i = s -1 ;
  j = arrayMax(a) - s + 1 ;
  fjp = arrp(b, j, float) ; 
  di = 2 ;
  ifac = 1 ;
  fp = arrp(b, i-1, float) ;  
  while (i--){
    *fp-- *=  t/(float)(t-ifac) ;
    *fjp++ *=  t/(float)(t-ifac) ;
    ifac += di ;
    di++ ;
  }

  i = arrayMax (b) ;
  fp = arrp(b, 0 , float) - 1 ;
  while (fp++, i--)
    *fp /= t ;
  if (s > 1)
    look->s = (s -1) *2 ;
  else
    look->s = 1 ;

  zoneBoxEntry(look->zoneBuffer) ;

}

/*************************************************************************/

static void zoneBoxEntry (char *cp)
{ int i, j, level, oldMin, oldMax, oldXmin, oldXmax ;
  int minX2, maxX2, k ;

 LOOKGET("zoneBoxEntry") ;

  look->reticule = FALSE ;
  oldMin = look->minX ;
  oldMax = look->maxX ;
  oldXmin = look->xMin ;
  oldXmax = look->xMax ;
  level = freesettext (cp, "") ;
  freecard(level) ;
  if (!freeint (&i) || !freeint (&j))
    goto abort ;
  if (i> j) goto abort ;
  i *= look->xDiv ;
  j *= look->xDiv ;

  look->minX = (int)(look->scale * (i  - look->shift)) ;
  look->maxX = (int)(look->scale * (j  - look->shift)) ;

  if (look->minX >= look->maxX)
    goto abort ;
  if (look->minX < 0)
    look->minX = 0 ;
   if (look->maxX > arrayMax (look->a))
     look->maxX = arrayMax (look->a) ; 

  k = 0 ;
  minX2 = utArrondi(look->shift + look->minX/look->scale) ;
  while ((i != minX2) && (k < 50)) {
    if (i  > minX2)
      (look->minX)++ ;
    else
      (look->minX)-- ;
    k++ ;
    minX2 = utArrondi(look->shift + look->minX/look->scale) ;
  }
  if (look->minX < 0)
    {
      look->minX = 0 ;
      look->xMin = utArrondi(look->shift) ;
    }
  else look->xMin = i ;

  k = 0 ;
  maxX2 = utArrondi(look->shift + look->maxX/look->scale) ;
  while ((j != maxX2 - 1) && k < 50){
    if (j > maxX2 - 1)
      (look->maxX)++ ;
    else
      (look->maxX)-- ;
    k++ ;
    maxX2 = utArrondi(look->shift + look->maxX/look->scale) ;
  }
  if (look->maxX > arrayMax (look->a))
    {
      look->maxX = arrayMax (look->a) ; 
      look->xMax = utArrondi(look->xEnd) ;
    }
  else 
    look->xMax = j ;
  if (look->minX >= look->maxX)
    goto abort ;
  if (look->xMin < look->shift) look->xMin = utArrondi(look->shift) ;
  if (look->xMax > look->xEnd) look->xMax = utArrondi(look->xEnd) ;
  if (look->xMax < look->xMin) look->xMax = look->xMin ;
  if ((look->plus1) && (look->maxX < arrayMax(look->a)))
      (look->maxX)++ ;
  plotDisplay () ;
  return ;
abort:
  look->minX = oldMin ;
  look->maxX = oldMax ;
  look->xMin = oldXmin ;
  look->xMax = oldXmax ;
  plotDisplay () ;
  return ;
}
/*************************************************************************/

static void wholeButton (void)
{ 
  LOOKGET("wholeButton") ;
  /* mhmp 10.03.99 */
  look->minX = 0 ;
  look->xMin = utArrondi(look->shift) ;
  look->maxX = arrayMax (look->a) ;
  look->xMax = utArrondi(look->xEnd) ;
  look->reticule = FALSE ;

  plotDisplay () ;
}

/*************************************************************************/


static Graph plotDisplay(void)
{ 
  char *cp ;
  register int i, j ;
  float x2=0, firstX=0, firstY=0 ; 
  /* mhmp 05.03.99 */
  int minX2, maxX2, i1, xDiv, i3, ddx, fac=1, isFac ;
  int zoneMin, zoneMax, oldi, facY ;
  float Xmag2=1 ; 
  int dx, dy,  nx, ny, 
  maxX, minX, xofmax ;
  float  minY=0 , maxY=0, tot = 0, fmax = 0 ; 
  Array a ;
  float h, x0, lastX0=0, y0, Xmag, Ymag, *yp ;
  int bottomMargin ;
  LOOKGET("plotDisplay") ;

  graphClear() ;
  a = look->a ; 

  maxX = look->maxX ;
  xofmax = minX = look->minX ;
  minY = maxY = arr(a, minX,float) ;
  for (i = minX, yp = arrp(a,i,float) ; i < maxX ; i++, yp++)
    { if (*yp > maxY)
	{ maxY = *yp ; xofmax = i ;}
      if (*yp < minY) minY = *yp ;
      tot += *yp ;
    }
  look->maxY = maxY ;
  look->minY = minY;
  facY = 1;
  /* mhmp 19.04.99 maxY > 0  26.04 maxY < 2 /1  + maxY - minY*/
  if ((maxY - minY> 0) && (maxY < 2)) {
    while ((maxY - minY)*facY < 1) 
      facY *= 10 ;
  }
  /* mhmp 19.04.99  sorti du while */
  look->facY = facY ;
  minY *= facY ;
  maxY *= facY ;
  look->minY *= facY ;
  look->maxY *= facY ;
  h = maxY - minY ;
  if (!h)
    h = 1;

  look->topMargin = 9 ;
  look->leftMargin = 7 ;
  bottomMargin = 5  ;
 
  graphFitBounds (&nx, &ny) ;
  if (ny < look->topMargin + bottomMargin + 6 || nx < look->leftMargin + 8)
    { graphText ("This graph is too small", 2, 2) ;
      graphRedraw () ;
      return graphActive () ;
    }
  if (maxX < minX) maxX = minX  ;
  look->Xmag = Xmag = (nx - look->leftMargin - 3) / ((float)(maxX - minX + 1)) ;
  look->Ymag = Ymag = (ny - look->topMargin - bottomMargin) / ((float)(h)) ;

  minX2 = utArrondi(look->shift + minX/look->scale) ;
  maxX2 = utArrondi(look->shift + maxX/look->scale) ;
  if (maxX2 < minX2) maxX2 = minX2 ;
  fmax = look->shift + xofmax/look->scale ;
  Xmag2 = (nx - look->leftMargin - 3) / ((float)(maxX2 - minX2 + 1)) ;

  i = abs(maxX2) ;
  if (abs(minX2) > i) i = abs(minX2) ;
  oldi = i ;
  j = 1 ;
  while (i /= 10) j++ ; /* count chars */
  dx = 5  * j / Xmag2 ;
  dx = utMainRoundPart(dx) ;
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
  look->xDiv = xDiv ;
  if (xDiv > 1 )
    {
      i = oldi/ xDiv ;
      j = 1 ;
      while (i /= 10) j++ ; 
      dx = 5  * j / Xmag2 ;
      dx = utMainRoundPart(dx) ;
      if(dx <= 0) dx = 1;
    }
  for(i = i1 = utMainPart(minX2); i <= maxX2; i += dx)
    {
      i3 = utArrondi(i / (float)xDiv) ;
      x2 = (look->scale * ( i3 * xDiv - look->shift)) ;
      if (x2 >= minX)
	{
	  i1 = i - dx ;
	  break ;
	}
    }

  dy = utMainRoundPart((int)(5. / Ymag)) ;
  if(dy <= 0) dy = 1; 
  for(i = utMainPart(minY) ; i <= maxY ; i += dy)
    if (i > minY) {
      firstY = i - dy ;
      look->firstY = firstY ; /* mhmp verifier le + 1 26.04.99*/
      look->Ymag = Ymag = (ny - look->topMargin - bottomMargin) / ((float)(maxY- firstY)) ;   
      break ;
    }
  firstX = -1000000 ;
  for(i = i1; i <= maxX2; i += dx) {
    /* 26.04.99 mhmp pb de -0.5 1.5 ---> -1 et 2*/
    i3 = utArrondi(i / (float)xDiv + 0.0001) ;
    x2 = (look->scale * ( i3*xDiv - look->shift)) ;
    if (firstX < -100000){
      firstX = x2 ;
      look->firstX = firstX ;
      look->Xmag = Xmag = (nx - look->leftMargin - 3) / ((float)(maxX - firstX + 1)) ;
    }
    graphText(messprintf("%d",i3), XX(x2) - .5, YY (firstY) + 1.1);
    /* mhmp 22.03.99 les tirets */
    graphLine(XX(x2), YY (firstY)+ look->axisShift,XX(x2), YY (firstY)+look->axisShift + 0.5) ;
  }

  i3 = utArrondi(i / (float)xDiv) ;
  x2 = (look->scale * ( i3*xDiv - look->shift)) ;
  if (XX(x2) < XX(maxX) + 3) {
    graphText(messprintf("%d",i3), XX(x2) - .5, YY (firstY) + 1.1);
    graphLine(XX(x2), YY (firstY)+ look->axisShift, XX(x2), YY (firstY)+ look->axisShift + 0.5) ;
  }
  /* mhmp 19.04.99 -1 +1*/
  /*  totMin = utArrondi(look->shift/xDiv) ;
  if (totMin > look->shift/xDiv)
    totMin-- ;
  totMax = utArrondi(look->xEnd/xDiv) ;
  if (totMax < look->xEnd/xDiv)
    totMax++ ;
  graphText(messprintf("Total interval  Min: %d  Max: %d", totMin, totMax), 2, 4.7 ) ;*/
  if (facY > 1)
    graphText (messprintf("scale y 1*%d", facY), 2, YY (firstY) + 2.2) ;

  for(i = firstY ; i <= maxY ; i += dy)
    { cp = messprintf("%d",i) ;
    /* mhmp 22.03.99 les tirets */
    graphText(cp, look->leftMargin -1  - look->axisShift - strlen(cp) , YY(i) - .5) ;
    graphLine(XX(firstX)-look->axisShift,  YY(i), XX(firstX)- look->axisShift - 0.8, YY(i));
    }
  if (YY(i) > YY (maxY) - 3) {
    cp = messprintf("%d",i) ;
    graphText(cp, look->leftMargin - 1 - look->axisShift - strlen(cp) , YY(i) - .5) ;
    graphLine(XX(firstX)-look->axisShift,  YY(i), XX(firstX)- look->axisShift - 0.8, YY(i));
  }
  look->retBox = graphBoxStart () ;
  /*23.04.99 minX -> firstX */
  graphLine(XX(firstX), YY(firstY)+look->axisShift, XX(maxX) + 3 , YY (firstY)+look->axisShift) ;  /* x axis */
  graphLine(XX(firstX)-look->axisShift, YY(firstY), XX(firstX)-look->axisShift , YY (maxY) - 3) ;  /* y axis, the 3 is used in 
                                                                moveReticule()  */

  graphColor(BLUE) ;

  for(i = minX, x0 = XX(i), y0 = YY(0), yp = arrp (look->a, i, float) ; 
      i < maxX - 1 ;
      x0 += Xmag, i++, yp++){
    if (i == minX)
      graphLine(x0,YY(look->firstY)+ look->axisShift, 
		x0, y0 - (*yp) * Ymag*facY) ;
    graphLine(x0, y0 - (*yp) * Ymag*facY, 
	      x0 + Xmag, y0 - (*yp) * Ymag*facY) ;
    graphLine(x0 + Xmag, y0 - (*yp) * Ymag*facY, 
	      x0 + Xmag, y0 - (*(yp + 1) * Ymag*facY)) ;

    lastX0 = x0 + Xmag ;
  }
  graphLine(lastX0, y0 - (*yp) * Ymag*facY, 
		lastX0 + Xmag, y0 - (*yp) * Ymag*facY) ;
  graphLine(x0 + Xmag, y0 - (*yp) * Ymag*facY, 
	      x0 + Xmag, YY(look->firstY)+ look->axisShift) ;
  graphBoxEnd () ;

  graphBoxMenu (look->retBox, histoMenu);

  look->xyBox = graphBoxStart() ;
  if (look->reticule) 
    { 
      graphTextPtr (look->xBuffer, 6,  2.5 , 8) ;
      graphTextPtr (look->yBuffer, 6,  3.7 , 8) ;
    }
  graphBoxEnd () ;
  graphBoxMenu(look->xyBox, histoMenu);
  isFac = 0 ;
  if (look->xDiv > 1 || look->xMul > 1)
    {
      if (look->xDiv > look->xMul)
	{
	  fac = look->xDiv / look->xMul ;
	  isFac = 1 ;
	}
      else
	{
	  fac = look->xMul / look->xDiv ;
	  isFac = -1 ;
	}
    }

  /*mhmp 19.04.99 -1/+1*/
  zoneMin = utArrondi(look->xMin/xDiv) ;
  if (zoneMin > look->xMin/xDiv)
    zoneMin-- ;
  zoneMax = utArrondi(look->xMax/xDiv) ;
  if (zoneMax < look->xMax/xDiv)
    zoneMax++ ;
 
  xofmax = fmax ;
  if (xofmax < zoneMin*xDiv)
    xofmax = zoneMin*xDiv ;
  if (xofmax > zoneMax*xDiv)
    xofmax = zoneMax*xDiv ;

  if (isFac < 0)
    graphText (messprintf ("scale x 1*%d", fac), XX(maxX) - 20, YY (firstY) + 2.2 ) ; 
  else if (isFac > 0)
    graphText (messprintf ("scale x 1/%d", fac), XX(maxX) - 20, YY (firstY) + 2.2 ) ; 

  i = graphBoxStart () ;

  graphText (messprintf ("In zone: total %g  max y(%d) = %g", 
			   tot, xofmax, maxY/facY), 2, 1.3) ;  

  graphBoxEnd () ;
  graphBoxDraw(i,RED,TRANSPARENT) ;
  graphBoxMenu (i, histoMenu);

  look->subtitleBox =graphTextEntry(look->subtitle, 23, XX(maxX) -20, YY(firstY) + 3.3, 0) ;

  if (look->reticule) 
    { 
      graphText("x = ", 2, 2.5) ;
      graphText("y = ", 2, 3.7) ;
      graphLine (XX(look->firstX)-look->axisShift,YY(look->y), XX(look->maxX), YY(look->y)) ;
      graphLine (XX(look->x), YY(look->firstY)+look->axisShift, XX(look->x), YY(look->maxY)) ;
    }
  minY /= facY ;
  maxY /= facY ;
  look->minY /= facY ;
  look->maxY /= facY ;
  sprintf(look->sBuffer, "%d", look->s) ;
  graphText ("Smoothing:",  21,  3.7) ;
  sprintf(look->stepBuffer, "%d", look->step) ;
  graphText ("Step:",  40,  3.7) ;
  look->sBox = graphTextEntry(look->sBuffer, 7,  32,  3.7, smoothingBoxEntry) ;
  look->stepBox = graphTextEntry(look->stepBuffer, 9,  46,  3.7, steppingBoxEntry) ;
  sprintf(look->zoneBuffer, "%d %d", zoneMin, zoneMax) ;
  graphText ("Active Zone:",  19,  2.5) ;
  look->zoneBox= graphTextEntry(look->zoneBuffer, 23, 32,  2.5, zoneBoxEntry) ;

  look->titleBox = graphTextEntry(look->title, 47, 2,  0.1, 0) ;
  if (!look->isPrinting)
    graphButton ("Whole", wholeButton, 12., 2.5) ;

  graphRedraw () ;
  return graphActive () ;
}


/******************************************************************/


static void plotRefresh (void)
{ 
  LOOKGET ("plotRefresh") ;
  look->reticule = FALSE; 
  plotDisplay() ;
}

/******************************************************************/

static Array convert (Array a) 
{ int max = arrayMax (a) ;
  int *ip = max ? arrp (a, 0, int) : 0 ;
  Array b = arrayCreate(max, float) ;
  float *fp ;

  array (b, max - 1, float) = 0 ;
  fp = arrp (b, 0, float) ;
  while (max--)
    *fp++ = *ip++ ;
  return b ;
}

/******************************************************************/
/****************** public routine ********************************/

void plotHisto(char *title, Array histArray)
{
  float  max = (float)(arrayMax(histArray)-1) ;
  char *subtitle ;
  subtitle = "" ;
  plotShiftedHisto (title,subtitle,histArray, 0, max,  (float) 1.0, 1) ; 
}

/* mhmp 10.03.99 float shift */
void plotShiftedHisto(char *title, char *subtitle, Array histArray, float shift, float xMax, float scale, int xMul)
{
  int  i, max ;
  LOOK look ; 

  if (!histArray)
    messcrash ("plotHisto(%s, array) received NULL array", title);

  if(histArray->size != sizeof(int))
    messcrash("plotHisto received a wrong type of Array, should be int");

  if(arrayMax(histArray) == 0)
    {
      arrayDestroy (histArray) ;
      messerror("plotHisto(%s,array) received a empty array", title);
      return ;
    }

  max = arrayMax(histArray);

  i = max - 1;
  while( !arr(histArray,i,int) && i-- );  
  max = i + 1 ;
  
  if(!max)
    {
      arrayDestroy (histArray) ;
      messerror("plotHisto(%s) received a null histogram", title);
      return ;
    }
  
  if(!displayCreate("DtHistogram")) 
    {
      arrayDestroy (histArray) ;
      return ;
    }

  if (!plotCurrHis)
    { plotCurrHis = arrayCreate(32, Graph) ;
      plotCurrNb = 0 ;
    }
  array(plotCurrHis, arrayMax(plotCurrHis), Graph) = graphActive() ;
  plotCurrNb++ ;
  look = (LOOK)messalloc(sizeof(struct LOOKstruct)) ;
  graphAssociate (&PLOT_MAGIC, look) ;
  graphRegister (DESTROY, plotDestroy) ;
  /* mhmp 20.04.99 plotDisplay -> plotRefresh */
  graphRegister (RESIZE, plotRefresh) ;
  graphRegister (PICK,  moveReticule) ;
  if (title && *title)
    graphRetitle (title) ;
  graphMenu(histoMenu);
  look->magic = PLOT_MAGIC ;
  look->originalArray = histArray ;
  look->a = convert (histArray) ;
  if (title && *title)
    sprintf(look->title, "%s", title) ;
  else
    sprintf (look->title , "%s", "Histogram");
  sprintf(look->subtitle, "%s", subtitle) ;
  look->reticule = FALSE ;
  look->isStepping = FALSE ;
  look->isSmoothing = FALSE ;
  look->isPrinting = FALSE ;
  look->plus1 = FALSE ;
  look->s = 1 ;

  look->scale = scale ;
  if (scale < 1)
    look->startStep = utArrondi(1./scale) ;
  else
    look->startStep = 1 ;
  look->step = look->startStep ;
  look->shift = shift ;
  look->xMin = utArrondi(shift) ;
  look->xEnd = xMax ;
  look->xMax = utArrondi(xMax) ;
  look->startScale = scale ;
  look->axisShift = 0.5 ;
  look->xMul = xMul ;
  look->minX = 0 ; /* included */
  look->maxX = max ;  /* excluded as usual in Arrays */
  
  plotDisplay() ;
  return ;
}

/*************************************************************************/

void plotHistoRemove(void)
{
  Graph *gp ;
  int i ;

  if (!plotCurrHis)
    return ;
  i = arrayMax(plotCurrHis) ;
  gp = arrp(plotCurrHis, 0, Graph) - 1 ;
  while(gp++, i--)
    if (graphActivate(*gp))
      graphDestroy() ;
  arrayDestroy(plotCurrHis) ;
  plotCurrNb = 0 ;
}

/*************************************************************************/
/*************************************************************************/

static void plotCopy (void) 
{ 
  LOOKGET("plotCopy") ;

  /* we have to pass a copy of the original array, 
     because it get's destroyed in plotDestroy,
     and if two graphs use the same array it'd get dest. twice */
  /* mhmp 05.03.99*/
  plotShiftedHisto (look->title, look->subtitle, arrayCopy(look->originalArray), look->shift, look->xEnd, look->startScale, look->xMul) ;
}
/*****************/
static float cleanScale(float scale)
{
  int inter;
  inter = 1 ;
  if (scale < 1) {
    inter = 1./scale ;
    inter = utMainRoundPart (inter) ;
    return 1./inter ;
  }
  else scale = 1;
  return scale ;
}

/*****************/

static void plotDoLoad (BOOL isAscii) 
{
  void *vp ;
  int max=0, i ; 
  unsigned int 
    nb = 2000, nr,
    size = sizeof (int) ,
    size2 = sizeof (float) ;
  Array a0, a ;
  int n, n1 ;
  float x = 0, scale = 0, xmin = 0, xmax = 0 ;
  ACEIN histo_in;
  LOOKGET("plotDoLoad") ;
  
  look->title[0] = '\0' ;
  look->subtitle[0] = '\0' ;

  histo_in = aceInCreateFromChooser
    (messprintf ("%s File of Integers", isAscii ? "Text" : "Binary"),
     look->dirName,look->fileName, "histo", "r", 0);

  if (!histo_in)
    return;
 
  a0 = arrayCreate (1000, int) ;
  a = arrayCreate (1000, int) ;

  if (isAscii)
    { 
      int nn;
      i = 0 ;
      while (aceInCard(histo_in))
	{
	  if (aceInInt(histo_in, &nn))
	    array (a0, i++, int) = nn ;
	}
      aceInDestroy (histo_in);

      n = arrayMax(a0) ;
      scale = 0 ; xmin = 1; xmax = 0 ;
      while (n--) 
	{ 
	  x = array(a0, n, int) ;
	  if (xmin > xmax) xmin = xmax = x ;
	  else { if (xmin > x) xmin = x ; if(xmax < x) xmax = x ; } 
	}
      if (xmax - xmin < 10000) scale = 1.0 ;
      else scale = 10000.0 / (utArrondi(xmax) - utArrondi(xmin)) ;
      scale = cleanScale (scale) ;/* mhmp 01.04.99 pour smooth */
      n = arrayMax(a0) ;
      while (n--) 
	{
	  x = array(a0, n, int) ;
          n1 = (x - xmin ) * scale ;
	  array(a, n1, int)++ ;
	}
      look->shift = xmin ;
      look->xEnd = xmax ;
      look->startScale = scale ;
      look->xMul = 1 ;
      max = arrayMax(a) ;
      arrayDestroy (a0) ;
      plotInit() ;
    } 
  else				/* Binary */
    {
      FILE *fil = NULL;
      char *filename;
      
      filename = strnew (aceInGetURL(histo_in), 0);
      aceInDestroy (histo_in);

      if (filename)
	fil = filopen (filename, "", "r");
      if (fil)
	{
	  fread (look->title, sizeof (char), 47, fil) ;
	  fread (look->subtitle, sizeof (char), 23, fil) ;
	  fread (&look->shift, size2, 1, fil) ;
	  fread (&look->xEnd, size2, 1, fil) ;
	  fread (&look->startScale, size2, 1, fil) ;
	  fread (&look->xMul, size, 1, fil) ;
	  i = 0 ;
	  do
	    {
	      array(a,(++i)*nb,int) = 0; /* to create space */
	      vp = arrp(a,nb*(i-1),int); /* possible relocation */
	    }  while ((nr=fread(vp,size,nb,fil))==nb);
	  arrayMax(a) -= nb - nr; /* artificial space removed */
	  i = arrayMax (a) - 1;
	  if (i>=0)
	    while( !arr(a,i,int) && i-- );  
	  max = i + 1 ;
	  arrayMax (a) = max ;
	  filclose(fil) ;
	  plotInit() ;
	}
      messfree (filename);
    }

  if(!max)
    {
      messout("I read an empty file") ;
      arrayDestroy(a) ;
      return;
    }
  if (!look->title || !*look->title)
    sprintf(look->title, "%s", look->fileName) ;
  graphRetitle (look->title) ;
  arrayDestroy (look->originalArray) ;
  arrayDestroy (look->a) ;    
  look->originalArray = a ;
  look->a = convert (a) ;
  look->minX = 0 ; /* included */
  look->maxX = max ;  /* excluded as usual in Arrays */
  
  plotDisplay() ;
  
  return;
} /* plotDoLoad */

static void plotLoad (void) 
{ plotDoLoad(FALSE) ; }
static void plotLoadAscii (void) 
{ plotDoLoad(TRUE) ; }




static void plotSave (void) 
{ 
  FILE *fil;
  unsigned int size = sizeof (int) ;
  /* mhmp 18.05.99 */
  unsigned int size2 = sizeof (float) ;
  LOOKGET("plotSave") ;
  
  if (!look->originalArray || 
      ! arrayMax(look->originalArray))
    return ;
  	
  if (look->title != look->fileName)
    strcpy(look->fileName, look->title) ;

  fil = filqueryopen (look->dirName,look->fileName,"histo","wb",
		      "Histogram Output File") ;

  if (!fil)
    return ;
  fwrite (look->title, sizeof (char), 47, fil) ;
  fwrite (look->subtitle, sizeof (char), 23, fil) ;
  fwrite (&look->shift, size2, 1, fil) ;
  fwrite (&look->xEnd, size2, 1, fil) ;
  fwrite (&look->startScale, size2, 1, fil) ;
  fwrite (&look->xMul, size, 1, fil) ;

  fwrite(arrp(look->originalArray,0,int),size,
	 arrayMax(look->originalArray),fil);
  filclose(fil) ;

  return;
} /* plotSave */

static void plotInit (void) 
{
  float scale = 1, shift = 0, xMax = 0 ;
  LOOKGET("plotInit") ;
  look->reticule = FALSE ;
  look->isStepping = FALSE ;
  look->isSmoothing = FALSE ;
  look->plus1 = FALSE ;
  look->s = 1 ;
  scale = look->startScale ;
  look->scale = scale ;
  if (scale < 1)
    look->startStep = utArrondi(1./scale) ;
  else
    look->startStep = 1 ;
  look->step = look->startStep ;
  shift = look->shift ;
  look->xMin = utArrondi(shift) ;
  xMax = look->xEnd ;
  look->xMax = utArrondi(xMax) ;
  look->startScale = scale ;
  look->axisShift = 0.5 ;
}
/*************************************************************************/
/*************************************************************************/


