/*  File: graphsub_srk.c
 *  Author: Richard Durbin (rd@mrc-lmb.cam.ac.uk)
 *          Simon Kelley (srk@sanger.ac.uk)
 *  Copyright (C) J Thierry-Mieg and R Durbin, 1991
 * -------------------------------------------------------------------
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * -------------------------------------------------------------------
 * This file is part of the ACEDB genome database package, written by
 * 	Richard Durbin (MRC LMB, UK) rd@mrc-lmb.cam.ac.uk, and
 *	Jean Thierry-Mieg (CRBM du CNRS, France) mieg@kaa.cnrs-mop.fr
 *
 * Description: graph package - device independent routines acting on
 *	the current active graph.
 * Exported functions: (many)
 * HISTORY:
 * Last edited: Jan  6 15:00 2011 (edgrif)
 * * Nov 10 14:49 1999 (fw): converted graphBoxInfoFile to ACEOUT
 *              so gifaceserver will work
 * * Oct  8 12:40 1999 (fw): applied ed's fixes to entryBoxEntry SANgc03596
 *              see graphsub.c,  now pray it works !
 * * May  6 10:23 1999 (edgrif): Fix stupid bug in busy cursor call code (sigh).
 * * Apr 29 16:06 1999 (edgrif): Big rearrangement to support graphdev
 *              layer, see Graph_Iternals.html for details. A lot of
 *              routines have been moved here from graphxt.c.
 * * Feb  8 16:37 1999 (edgrif): Add calls to support sprdmap.c
 * * Feb  1 17:39 1999 (edgrif): Added null graphTextAlign call for Unix.
 * * Jan 21 14:36 1999 (edgrif): Moved gifLeftDown to graphgd.c
 * * Dec 15 23:30 1998 (edgrif): Added busy cursor to gLeftDown, gMiddleDown.
 * * Nov 12 13:36 1998 (edgrif): Added code to trap NULL return from
 *              graphPasteBuffer(), otherwise we bomb out.
 * * Sep 30 09:59 1998 (edgrif): Replace #ifdef ACEDB with a call to
 *              the acedb/graph interface. Correct invalid use of
 *              '? :' construct for output to freeOutf or fprintf.
 * * Apr 30 16:21 1997 (rd)
 *		-   In graphBoxStart(), removed newGraphBox()
 *		-	graphDeleteContents() extracted from graphClear() and invoked by
 *			both graphClear(), and in graphDestroy() in lieu of graphClear()
 *
 * * Jun 6 16:34 1996 (rbrusk):
 *	- graphTextEntry2() etc. now all "void (*fn)(char*)" callbacks
 * * Jun  5 01:37 1996 (rd)
 * * Jan  5 13:50 1994 (mieg): restore gActive after button action
 * * Aug 19 12:02 1992 (rd): polygon squares.
 * * Aug 19 12:01 1992 (mieg): textPtrPtr
 * * Feb 11 00:11 1992 (mieg): graphClassEntry -> graphCompletionEntry
 * * Jan 28 1992 (neil): save existing drag routines over a drag operation, 
                         restore them on button up;
                         (changed graphBoxDrag, upBox)
 * * Dec  1 19:21 1991 (mieg): textEntry recognition of arrow keys
 * * Dec  1 19:20 1991 (mieg): graphClassEntry: autocompletes on tab
 * * Oct 10 14:51 1991 (rd): cleaned graphBoxClear - now a toggle
 * * Oct  8 23:00 1991 (mieg): added graphBoxClear
 * * Oct  7 23:00 1991 (rd): added graphBoxShift
 * CVS info:   $Id: graphsub.c,v 1.80 2011-01-06 17:51:07 edgrif Exp $
 *-------------------------------------------------------------------
 */

#include <wh/regular.h>
#include <wh/aceio.h>
#include <wh/menu_.h>
#include <w2/graphdev.h>
#include <w2/graph_.h>	  /* defines externals within graph package */
#include <wh/key.h>


static float  gPixAspect = 1.0 ;
static int  psize, psize2 ;

/* Box clipping statics and macros.                                          */
static int clipx1, clipx2, clipy1, clipy2 ;

#define xclip(z)  if (z < clipx1) z = clipx1 ; \
		  else if (z > clipx2) z = clipx2
#define yclip(z)  if (z < clipy1) z = clipy1 ; \
		  else if (z > clipy2) z = clipy2
#define xsafe(z)  if (z < -30000) z = -30000 ; \
		  else if (z > 30000) z = 30000
#define ysafe(z)  if (z < -30000) z = -30000 ; \
		  else if (z > 30000) z = 30000

static void drawBox (Box box) ;


/*******************************/


BOOL gFontInfo (Graph_ graph, int height, int* w, int* h)
	/* note that this depends on an integer height, and so
	   is only reliable in the long term for height 0 */
{
  BOOL result = TRUE; 

  if (graph->dev && gDevFunc->graphDevFontInfo)
    result = gDevFunc->graphDevFontInfo(graph->dev, height, w, h) ;
  else
    {
      if (w) *w = 8;
      if (h) *h = 13;
    }
  return result ;
}


/*******************************/


void graphPasteBuffer (void (*callback)(char *))
{
  if (gDev && gDevFunc->graphDevPasteBuffer)
    {
      gActive->pasteCallBack = callback;
      gDevFunc->graphDevPasteBuffer(gDev) ;
    }
}

void graphCBPaste(GraphPtr graph, char *text)
{
  Graph gsave = gActive->id;
  graphActivate(graph->id);
 
  if (gActive->pasteCallBack)
    (*gActive->pasteCallBack)(text);
  
  graphActivate(gsave);
}
  
void graphPostBuffer (char *text)
{
  
  if (gDev && gDevFunc->graphDevPostBuffer)
    gDevFunc->graphDevPostBuffer(gDev, text) ;

  return ;
}

/*******************************/

/* the current mapping, so we can return the pixel intensity */
static unsigned char map[256] ;

void graphSetGreyMap(unsigned char *newMap, BOOL isDrag)
{
  memcpy(map, newMap, 256);

  if (gDev && gDevFunc->graphDevSetGreyRamp)
    gDevFunc->graphDevSetGreyRamp(gDev, map, isDrag);
  
}

int graphGreyRampPixelIntensity(int weight)
{ /* Given a pixel whose weight in a pixelsRaw image is weight,
     How gray is it displayed, currently.
     Weight and grayness are 0-255 */

  return (weight<0 || weight>255) ? 0 : map[weight];
}

unsigned char *graphGreyRampGetMap(void)
{
  return map;
}

/* Set a global variable which control if widgets and windows being created
   have the greyRamp colormap installed or not. */
BOOL graphSetInstallMap(BOOL arg) 
{
  BOOL ret = FALSE;
  
  if (gDevFunc && gDevFunc->graphDevSetGreyMap)
    ret = gDevFunc->graphDevSetGreyMap(arg);

  return ret;
}
  
 
void graphRedraw (void)	/* now do this by creating expose event */
{
  
  if (gDev && gDevFunc->graphDevRedraw)
    {
    gActive->isClear = FALSE ;
    if (gActive->stack)
      gDevFunc->graphDevRedraw(gDev, gActive->w, gActive->h) ;
    }

  return ;
}



/******* now draw boxes **********/



void graphBoxDraw (int k, int fcol, int bcol)
{
  Box box = gBoxGet (k) ;

  if (fcol >= 0)
    box->fcol = fcol ;
  if (bcol >= 0)
    box->bcol = bcol ;

  if (gDev && gDevFunc->graphDevSetBoxDefaults && !gActive->isClear)
    {
      clipx1 = uToXabs(box->x1) ; if (clipx1 < 0) clipx1 = 0 ;
      clipx2 = uToXabs(box->x2) ; if (clipx2 > 30000) clipx2 = 30000 ;
      clipy1 = uToYabs(box->y1) ; if (clipy1 < 0) clipy1 = 0 ;
      clipy2 = uToYabs(box->y2) ; if (clipy2 > 30000) clipy2 = 0 ;
      if (clipx1 > clipx2 || clipy1 > clipy2)
	return ;
      
      gDevFunc->graphDevSetBoxDefaults(gDev, box->line_style,
				       uToXrel(box->linewidth),
				       uToYrel(box->textheight)) ;
      psize = uToXrel(box->pointsize);
      psize2 = psize/2 ;
      if (!psize2)
	psize2 = 1 ;
      
      drawBox (box) ;
      
    }
}

/* This routine is the main routine for filling windows with their contents. */
/* It can be called because a window is new, because its contents have       */
/* changed, because the window has been resized or exposed.                  */
/*                                                                           */
void graphClipDraw (int x1, int y1, int x2, int y2) /* expose action */
{
 Box box = gBoxGet(0) ;

 /* We only do any drawing if there is an active window and subwindow, if   */
 /* there is anything on the stack to draw and the window has not been      */
 /* cleared of everything.                                                  */
 /* Note, as drawing can take a while, we turn on a busy cursor and then     */
 /* turn it off once finished.                                               */
 if (gDev && gActive->stack && !gActive->isClear
     && gDevFunc->graphDevSetBoxClip  && gDevFunc->graphDevUnsetBoxClip)
   {
     clipx1 = (x1 > 0) ? x1 : 0 ; 
     clipx2 = (x2 < 30000) ? x2 : 30000 ; 
     clipy1 = (y1 > 0) ? y1 : 0 ; 
     clipy2 = (y2 < 30000) ? y2 : 30000 ;

     if (clipx1 <= clipx2 && clipy1 <= clipy2)
       {
	 graphBusyCursorAll ();
	 gDevFunc->graphDevSetBoxDefaults(gDev, box->line_style,
					  uToXrel(box->linewidth),
					  uToYrel(box->textheight)); 
	 psize = uToXrel(box->pointsize);
	 psize2 = psize/2 ;
	 if (!psize2)
	   psize2 = 1 ;
	 
	 gDevFunc->graphDevSetBoxClip(gDev, 
				      clipx1, clipy1, clipx2, clipy2) ;
	 drawBox (box) ;
	 gDevFunc->graphDevUnsetBoxClip(gDev);
	 
       }
   }

 return ;
}



static void drawBox (Box box)
{
  float  t ;
  int    x1, x2, y1, y2, i, r, s, old, fcol ;
  char   *text ;
  unsigned char *utext;
  int    action ;
#ifdef DEBUG_RECURSION
  static int recursionLevel = 0 ;
#endif

  if (!stackExists (gStk)) /* does happen when playing fast with many windows
			    * during expose events on uncomplete windows
			    * this is probably due to fancy X11 event queing 
			    */
    return ;

#ifdef DEBUG_RECURSION
  ++recursionLevel ;
#endif

  if (box->x1 > box->x2 || box->y1 > box->y2 ||
      (x1 = uToXabs(box->x1)) > clipx2 ||
      (y1 = uToYabs(box->y1)) > clipy2 ||
      (x2 = uToXabs(box->x2)) < clipx1 ||
      (y2 = uToYabs(box->y2)) < clipy1)
    {
      int nDeep = 1 ;
      stackCursor (gStk,box->mark) ; /* sets position to mark */
      while (!stackAtEnd (gStk))
	switch (action = stackNext (gStk,int))
	  {
	  case BOX_END:
	    if (!--nDeep)
	      {
#ifdef DEBUG_RECURSION
		fprintf (stderr,"  Level %d return non-draw box-end\n",
			 recursionLevel--) ;
#endif
		return ;                        /* exit point */
	      }
	    break ;
	  case BOX_START:
	    r = stackNext (gStk, int) ;
	    ++nDeep ;
	    break ;
	  case COLOR: case TEXT_FORMAT:
	    r = stackNext (gStk,int) ; 
	    break ;
	  case LINE_WIDTH: case TEXT_HEIGHT: case POINT_SIZE:
	    t = stackNext (gStk,float) ;
	    break ;
	  case LINE_STYLE:
	    r = stackNext (gStk, int) ;
	    break ;
	  case LINE: case RECTANGLE: case FILL_RECTANGLE:
	    t = stackNext (gStk,float) ;
	    t = stackNext (gStk,float) ;
	    t = stackNext (gStk,float) ;
	    t = stackNext (gStk,float) ;
	    break ;
	  case PIXELS: case PIXELS_RAW:
	    t = stackNext (gStk,float) ;
	    t = stackNext (gStk,float) ;
	    utext = stackNext (gStk, unsigned char*) ;
            r = stackNext (gStk, int) ;
            r = stackNext (gStk, int) ;
            r = stackNext (gStk, int) ;
	    if (action == PIXELS)
	      {
		unsigned int *uintp = stackNext (gStk, unsigned int*) ;
		r = stackNext (gStk,int) ;
	      }
	    break ;
          case POLYGON : case LINE_SEGS:
            r = stackNext (gStk, int) ;
            while (r--)
	      { t = stackNext (gStk,float) ;
                t = stackNext (gStk,float) ;
	      }
            break ;
	  case CIRCLE: case POINT: 
	  case TEXT: case TEXT_UP: case TEXT_PTR: case TEXT_PTR_PTR: 
	  case COLOR_SQUARES: case FILL_ARC: case ARC:
	    t = stackNext (gStk,float) ;
	    t = stackNext (gStk,float) ;
	    switch (action)
	      {
	      case CIRCLE:
		t = stackNext (gStk,float) ;
		break ;
	      case FILL_ARC: case ARC:
		t = stackNext (gStk,float) ;
		t = stackNext (gStk,float) ;
		t = stackNext (gStk,float) ;
		break ;
	      case POINT:
		break ;
	      case TEXT: case TEXT_UP:
		text = stackNextText (gStk) ;
		break ;
	      case TEXT_PTR:
		text = stackNext (gStk,char*) ;
		break ;
	      case TEXT_PTR_PTR:
		text = *stackNext (gStk,char**) ;
		break ;
	      case COLOR_SQUARES:
		text = stackNext (gStk,char*) ;
		r = stackNext (gStk, int) ;
		r = stackNext (gStk, int) ;
		text = (char*) stackNext (gStk,int*) ;
		break ;
	      }
	    break ;
	  case IMAGE:
	    (void)stackNext (gStk, void*) ; 
	    break ;
	  default:
	    messout ("Invalid draw action %d received",action) ;
	  }
#ifdef DEBUG_RECURSION
      fprintf (stderr, "  Level %d return bottom of stack nondraw\n", 
	       recursionLevel--) ;
#endif
      return ;
    }

  if (box->bcol != TRANSPARENT)
    {
      xclip(x1) ; xclip(x2) ; yclip(y1) ; yclip(y2) ;

      gDevFunc->graphDevSetColours(gDev, box->bcol, box->bcol) ;

      gDevFunc->graphDevDrawRectangle(gDev, GRAPHDEV_SHAPE_FILL, x1, y1, (x2-x1)+1, (y2-y1)+1) ;
    }

  fcol = box->fcol;
  gDevFunc->graphDevSetColours(gDev, fcol, box->bcol) ;
  
  stackCursor (gStk,box->mark) ; /* sets position to mark */

  while (!stackAtEnd (gStk))
    switch (action = stackNext (gStk,int))
      {
      case BOX_END:
#ifdef DEBUG_RECURSION
	fprintf (stderr,"  Level %d return\n", recursionLevel--) ;
#endif
        return ;                        /* exit point */
      case BOX_START:
        r = stackNext (gStk, int) ;
#ifdef DEBUG_RECURSION
	fprintf (stderr, "Level %d mark %d calls %d\n", 
		 recursionLevel, box->mark, r) ;
#endif
        drawBox (gBoxGet (r)) ;             /* recursion */
	gDevFunc->graphDevSetColours(gDev, fcol, box->bcol);
        break ;
      case COLOR:
        x1 = stackNext (gStk,int) ; 
	if (x1 == FORECOLOR) 
	  fcol = box->fcol; 
	else if (x1 == BACKCOLOR)
	  fcol = box->bcol;
	else fcol = x1;
	gDevFunc->graphDevSetColours(gDev, fcol, box->bcol);
        break ;
      case LINE_WIDTH:
        t = stackNext (gStk,float) ;
	gDevFunc->graphDevSetLineWidth(gDev, uToXrel(t)) ;
        break ;
      case LINE_STYLE:
        r = stackNext (gStk,int) ;
	gDevFunc->graphDevSetLineStyle(gDev, gGetGraphDevLineStyle(r)) ;
        break ;
      case TEXT_HEIGHT:
        t = stackNext (gStk,float) ; x1 = uToYrel(t) ;
        gDevFunc->graphDevSetFont (gDev, x1) ;
        break ;
      case TEXT_FORMAT:
        x1 = stackNext (gStk,int) ; 
        gDevFunc->graphDevSetFontFormat(gDev, gGetGraphDevFontFormat(x1)) ;
        break ;
      case POINT_SIZE:
        t = stackNext (gStk,float) ;
	psize = uToXrel(t) ; psize2 = psize/2 ;
        break ;
      case LINE: case RECTANGLE: case FILL_RECTANGLE:
        t = stackNext (gStk,float) ; x1 = uToXabs(t) ; xsafe(x1) ;
        t = stackNext (gStk,float) ; y1 = uToYabs(t) ; ysafe(y1) ;
        t = stackNext (gStk,float) ; x2 = uToXabs(t) ; xsafe(x2) ;
        t = stackNext (gStk,float) ; y2 = uToYabs(t) ; ysafe(y2) ;
   	switch (action)
          {
          case LINE:
            if (!((x1 < clipx1 && x2 < clipx1) ||
		  (x1 > clipx2 && x2 > clipx2) ||
		  (y1 < clipy1 && y2 < clipy1) ||	/* not perfect */
		  (y1 > clipy2 && y2 > clipy2) )) /* excludes both ends left,right,above or below */
	      gDevFunc->graphDevDrawLine(gDev, x1, y1, x2, y2) ;
            break ;
          case RECTANGLE:
	    if (x2 == x1) x2 = x1+1 ;
	    if (y2 == y1) y2 = y1+1 ;
	    if (x2 < x1) { r = x1 ; x1 = x2 ; x2 = r ; }
	    if (y2 < y1) { r = y1 ; y1 = y2 ; y2 = r ; }
            if (x1 <= clipx2 && y1 <= clipy2 && x2 >= clipx1 && y2 >= clipy1)
	      gDevFunc->graphDevDrawRectangle(gDev, GRAPHDEV_SHAPE_OUTLINE, x1, y1, (x2-x1), (y2-y1));
            break ;
          case FILL_RECTANGLE:
      	    if (x2 == x1) x2 = x1+1 ;
	    if (x2 < x1) { r = x1 ; x1 = x2 ; x2 = r ; }
	    if (y2 == y1) y2 = y1+1 ;
	    if (y2 < y1) { r = y1 ; y1 = y2 ; y2 = r ; }
            if (x1 <= clipx2 && y1 <= clipy2 && x2 >= clipx1 && y2 >= clipy1)
	      gDevFunc->graphDevDrawRectangle(gDev, GRAPHDEV_SHAPE_FILL, x1, y1, (x2-x1), (y2-y1));
            break ;
	  }
	break ;
      case PIXELS: case PIXELS_RAW:
	{ 
	  int xbase, ybase, w, h, len, ncols ;
	  unsigned char *pixels ;
	  unsigned int *colors;
	  

	  t = stackNext (gStk,float) ; x1 = uToXabs(t) ; xbase = x1 ;
	  t = stackNext (gStk,float) ; y1 = uToYabs(t) ; ybase = y1 ;
	  pixels = (unsigned char *)stackNext (gStk, unsigned char*) ;
	  w = stackNext (gStk, int) ;
	  h = stackNext (gStk, int) ;
	  len = stackNext (gStk, int) ;
	  if (action == PIXELS)
	    { colors = (unsigned int *)stackNext (gStk, unsigned int*) ;
	      ncols = stackNext (gStk,int) ;
	    }
	  else
	    { x2 = x1 + w ;
	      y2 = y1 + h ;
	      xclip(x1) ; xclip(x2) ; yclip(y1) ; yclip(y2) ; 
	    }
	  if (x1 < x2 && y1 < y2)
	    {
	      if (action == PIXELS)
		gDevFunc->graphDevDrawImage(gDev, pixels, w, h,
					    len,xbase, ybase,
					    colors, ncols);
	      else 
		gDevFunc->graphDevDrawRampImage(gDev, pixels, w, h, 
						len, x1, y1, x2, y2, 
						xbase, ybase) ;
	    }
	}
	break ;
      case POLYGON : case LINE_SEGS :
	{ int numVertices = stackNext (gStk, int) ;
	  int *x_vertices = (int *) messalloc (numVertices * sizeof (int)) ;
	  int *y_vertices = (int *) messalloc (numVertices * sizeof (int)) ;
	  GraphDevShapeStyle poly_type = (action == POLYGON ? GRAPHDEV_SHAPE_FILL :
					  GRAPHDEV_SHAPE_OUTLINE) ;

	  for (i = 0 ; i < numVertices ; i++)
	    { t = stackNext(gStk,float) ; 
	      x1 = uToXabs(t) ; xsafe(x1) ; x_vertices[i] = x1 ;
	      t = stackNext(gStk,float) ; 
	      y1 = uToYabs(t) ; ysafe(y1) ; y_vertices[i] = y1 ;
	    }

        /* NOTE this assumes the polygons are of the simplest
         * possible type, with no intersecting lines */
	  gDevFunc->graphDevDrawPolygon(gDev, poly_type, x_vertices, y_vertices, numVertices) ;

	  messfree (x_vertices) ;
	  messfree (y_vertices) ;
	}
        break ;
      case CIRCLE: case POINT: 
      case TEXT: case TEXT_UP: case TEXT_PTR: case TEXT_PTR_PTR: 
      case COLOR_SQUARES: case FILL_ARC: case ARC:
        t = stackNext (gStk,float) ; x1 = uToXabs(t) ;
	t = stackNext (gStk,float) ; y1 = uToYabs(t) ;
        switch (action)
          {
          case CIRCLE:      /* given center x1, y1, radius r*/
            t = stackNext (gStk,float) ; r = uToXrel(t) ;
	    if (!r) r = 1 ;
	    if (x1 - r < clipx2 && x1 + r > clipx1 &&
		y1 - r < clipy2 && y1 + r > clipy1)
	      gDevFunc->graphDevDrawArc(gDev, GRAPHDEV_SHAPE_OUTLINE,
			      x1-r, y1-r, 2*r, 2*r, 0, 64*360) ;
		    /* NB upper left corner, major/minor axes and 64ths of a degree */
            break ;
	  case ARC:
            t = stackNext (gStk,float) ; r = uToXrel(t) ;
	    { float ang = stackNext (gStk,float) ;
	      float angDiff = stackNext (gStk,float) ;
	      if (!r) r = 1 ;
	    if (x1 - r < clipx2 && x1 + r > clipx1 &&
		y1 - r < clipy2 && y1 + r > clipy1)
	      gDevFunc->graphDevDrawArc(gDev, GRAPHDEV_SHAPE_OUTLINE,
			      x1-r, y1-r, 2*r, 2*r, (int)(ang*64), (int)(angDiff*64)) ;
	      /* NB upper left corner, major/minor axes and 64ths of a degree */
	    }
	    break ;
	  case FILL_ARC:
            t = stackNext (gStk,float) ; r = uToXrel(t) ;
	    { float ang = stackNext (gStk,float) ;
	      float angDiff = stackNext (gStk,float) ;
	      if (!r) r = 1 ;
	    if (x1 - r < clipx2 && x1 + r > clipx1 &&
		y1 - r < clipy2 && y1 + r > clipy1)
	      gDevFunc->graphDevDrawArc(gDev, GRAPHDEV_SHAPE_FILL,
			      x1-r, y1-r, 2*r, 2*r, (int)(ang*64), (int)(angDiff*64)) ;
	      /* NB upper left corner, major/minor axes and 64ths of a degree */
	    }
	    break ;
          case POINT:
	    {
	    if (x1 - psize2 < clipx2 && x1 + psize2 > clipx1 &&
		y1 - psize2 < clipy2 && y1 + psize2 > clipy1)
	      gDevFunc->graphDevDrawRectangle(gDev, GRAPHDEV_SHAPE_FILL, 
				    x1-psize2, y1-psize2, psize, psize) ;
            break ;
	    }
          case TEXT:
	    {
	    int fontWidth, fontHeight, fontdy ;

	    gDevFunc->graphDevGetFontSize(gDev, 
					  &fontWidth, &fontHeight, &fontdy) ;
            text = stackNextText (gStk) ;
	    s = strlen(text) ;
	    if (x1 < clipx2 && x1 + s*fontWidth > clipx1 &&
		y1 < clipy2 && y1 + fontHeight > clipy1)
	      gDevFunc->graphDevDrawString(gDev, GRAPHDEV_STRING_LTOR, 
					   x1, y1+fontdy, text, s) ; 
            break ;
	    }
	  case TEXT_UP:
	    {
	    int fontWidth, fontHeight, fontdy ;

	    gDevFunc->graphDevGetFontSize(gDev, &fontWidth, &fontHeight, &fontdy) ;
            text = stackNextText (gStk) ;
	    s = strlen(text) ;
	    if (x1 < clipx2 && x1 + fontWidth > clipx1 &&
		y1 - s*fontHeight < clipy2 && y1 > clipy1)
	      gDevFunc->graphDevDrawString(gDev, GRAPHDEV_STRING_TTOB, x1, y1, text, s) ;
            break ;
	    }
	  case TEXT_PTR:
	    {
	    int fontWidth, fontHeight, fontdy ;

	    gDevFunc->graphDevGetFontSize(gDev, &fontWidth, &fontHeight, &fontdy) ;
	    text = stackNext (gStk,char*) ;
	    s = strlen(text) ;
	    if (x1 < clipx2 && x1 + s*fontWidth > clipx1 &&
		y1 < clipy2 && y1 + fontHeight > clipy1)
	      gDevFunc->graphDevDrawString(gDev, GRAPHDEV_STRING_LTOR, x1, y1+fontdy, text, s) ;
	    break ;
	    }
	  case TEXT_PTR_PTR:
	    {
	    int fontWidth, fontHeight, fontdy ;

	    gDevFunc->graphDevGetFontSize(gDev, &fontWidth, &fontHeight, &fontdy) ;
	    if (!(text = *stackNext (gStk,char**)))
	      break ;
	    s = strlen(text) ;
	    if (x1 < clipx2 && x1 + s*fontWidth > clipx1 &&
		y1 < clipy2 && y1 + fontHeight > clipy1)
	      gDevFunc->graphDevDrawString(gDev, GRAPHDEV_STRING_LTOR, x1, y1+fontdy, text, s) ; 
	    break ;
	    }
	  case COLOR_SQUARES:
	    { int *tints ;
	      text = stackNext (gStk,char*) ;
	      r = stackNext (gStk, int) ;
	      s = stackNext (gStk, int) ;
	      tints = stackNext (gStk, int*) ;
	      x2 = 0;
	      y2 = uToYrel (1.0) ;
	      if (x1 >= clipx2 || x1 + r*uToXrel(1.0) <= clipx1 ||
		  y1 >= clipy2 || y1 + y2 <= clipy1)
		break ;
	      old = fcol ;
	      while (r--)
		{ int newColor;
		  switch (*text^((*text)-1)) /* trick to find first on bit */
		    { 
		    case -1:   newColor = WHITE; break;
		    case 0x01: newColor = tints[0]; break;
		    case 0x03: newColor = tints[1]; break;
		    case 0x07: newColor = tints[2]; break;
		    case 0x0f: newColor = tints[3]; break;
		    case 0x1f: newColor = tints[4]; break;
		    case 0x3f: newColor = tints[5]; break;
		    case 0x7f: newColor = tints[6]; break;
		    case 0xff: newColor = tints[7]; break;
		    }
		  
		  if (newColor != old)
		    { if (x2 > 0)
			{ gDevFunc->graphDevSetColours(gDev, old, box->bcol) ;
			  gDevFunc->graphDevDrawRectangle(gDev, GRAPHDEV_SHAPE_FILL, x1, y1, x2, y2) ; 
			  x1 += x2; x2 = 0; 
			}
		      old = newColor ;
		    }
		  x2 += uToXrel(1.0) ;
		  text += s ;
		}
	      if (x2 > 0)
		{ gDevFunc->graphDevSetColours(gDev, old, box->bcol) ;
		gDevFunc->graphDevDrawRectangle(gDev, GRAPHDEV_SHAPE_FILL, x1, y1, x2, y2) ;
		} 
	      gDevFunc->graphDevSetColours(gDev, fcol, box->bcol);
	      break ;
	    }
          }
	break ;
      case IMAGE:
	(void)stackNext (gStk, void*) ;
	break ;
      default:
	messout ("Invalid action %d received in drawBox()",action) ;
      }
#ifdef DEBUG_RECURSION
  fprintf (stderr, "  Level %d return at botom of func\n", 
	   recursionLevel--) ;
#endif
}



/*******************************/
void graphXorLine (float x1, float y1, float x2, float y2)
{
  int ix1 = uToXabs(x1) ;
  int iy1 = uToYabs(y1) ;
  int ix2 = uToXabs(x2) ;
  int iy2 = uToYabs(y2) ;
  
  if (gDev && gDevFunc->graphDevXorLine)
    gDevFunc->graphDevXorLine(gDev, ix1, iy1, ix2, iy2);
}

void graphXorBox(int k, float x, float y)
{
  Box box = gBoxGet (k) ;
  int x1 = uToXabs(x) ;
  int y1 = uToYabs(y) ;
  int x2 = x1 + uToXrel(box->x2 - box->x1) ;
  int y2 = y1 + uToYrel(box->y2 - box->y1) ;

  if (gDev)
    {
    if ((x2 == x1 || y2 == y1) && gDevFunc->graphDevXorLine)
      gDevFunc->graphDevXorLine(gDev, x, y, x + (x2-x1), y + (y2-y1)) ;
    else if (gDevFunc->graphDevXorBox)
      gDevFunc->graphDevXorBox(gDev, x1, y1, (x2-x1), (y2-y1));
    }
}



void graphBusyCursorAll ()
{
  if (gDevFunc && gDevFunc->graphDevBusyCursorAll)
    gDevFunc->graphDevBusyCursorAll() ;

}




/*******************************/

void graphFacMake (void)	/* for PLAIN and MAP graphs */
{
  float xfac,yfac ;
  float asp ;

  xfac = gActive->w / gActive->uw ;
  yfac = gActive->h / gActive->uh ;

  if (gActive->aspect)
    { asp = gPixAspect * gActive->aspect ;
      if (xfac*asp > yfac)
	xfac = yfac/asp ;
      yfac = xfac * asp ;
    }
  else
    gActive->aspect = yfac/xfac ;

  gActive->xFac = xfac ;
  gActive->yFac = yfac ;
  gActive->xWin = uToXrel (gActive->ux) ;
  gActive->yWin = uToYrel (gActive->uy) ;
}

void gUpdateBox0 (void)
{
  Box box ;
  
  if (!gActive->nbox)
    return ;

  box = gBoxGet(0) ;
  box->x1 = gActive->ux ; box->x2 = gActive->ux + gActive->uw ;
  box->y1 = gActive->uy ; box->y2 = gActive->uy + gActive->uh ;
}


void graphGetPlainBounds(float *ux, float *uy, float *uw, float *uh, float *aspect)
{
  if (ux)
    *ux = gActive->ux ;

  if (uy)
    *uy = gActive->uy ;

  if (uw)
    *uw = gActive->uw ;

  if (uh)
    *uh = gActive->uh ;

  if (aspect)
    *aspect = gActive->aspect ;

  return ;
}

void graphPlainBounds (float ux, float uy, float uw, float uh, float aspect)
{                                 /* makes ->fac, ->xWin, ->yWin */
  gActive->ux = ux ;
  gActive->uy = uy ;
  gActive->uw = uw ;
  gActive->uh = uh ;
  gActive->aspect = aspect ;
  graphFacMake() ;
  gUpdateBox0 () ;
}

void graphTextInfo (int *dx, int *dy, float *cw, float *ch)
{
  if (!gFontInfo (gActive, uToYrel(gActive->textheight), dx, dy)) 
    /* number of pixels of a character */
    messcrash ("Can't get font info for current font") ;
     /* default font dimensions are device dependent */
  if (cw && dx)
    *cw = XtoUrel(*dx) ; /* char width in user's coord */
  if (ch && dy)
    *ch = YtoUrel(*dy) ; /* char height in user's coord */
}

void graphFitBounds (int *nx, int *ny)
{

  /* For Scrolling graphs, this returns the size of the 
     physical window (as opposed to the virtual, scrollable area */
 
  if ((gActive->type == TEXT_SCROLL || gActive->type == PIXEL_SCROLL ||
      gActive->type == TEXT_FULL_SCROLL || gActive->type == PIXEL_VSCROLL ||
      gActive->type == PIXEL_HSCROLL || gActive->type == TEXT_HSCROLL)
      && gDev && gDevFunc->graphDevGetWindowSize)
    {
      float x, y, width, height ;
      
      gDevFunc->graphDevGetWindowSize(gDev, &x, &y, &width, &height) ;
      
      if (nx)
	*nx = (int) (width / gActive->xFac);
      if (ny)
	*ny = (int) (height / gActive->yFac);
    }
  else
    {
      /* rbsk: better behaviour in graph displays like blixem,
	 if I round the calculation upwards by the addition of 0.5 */
      if (nx)
	*nx = (int)((float)gActive->w / gActive->xFac + 0.5) ; 
      if (ny)
	*ny = (int)((float)gActive->h / gActive->yFac + 0.5) ;
    }
}
/* other graphXxxBounds() are in graphdev.c since they actively
   change the size of the scrolled window, which is a device
   dependent operation.
*/

/********************* box control ********************************/

Box gBoxGet (int k)
{ 
  if (k < 0 || k >= gActive->nbox)
    messcrash ("Cannot get box %d - index out of range",k) ;
  return arrp (gActive->boxes, k, BoxStruct) ;
}

int graphLastBox(void)
{
  return gActive->nbox - 1;
}

int graphBoxStart (void)
{
  Box box ;
  int k = gActive->nbox++ ;
  int col = k ? gBox->bcol : BACK_COLOR ;
#ifdef BETTER_BOX_SHIFT
  int parentid = k ? gBox->id : 0 ;
#endif

  box = arrayp (gActive->boxes, k, BoxStruct) ;	/* invalidates gBox */

  box->linewidth = gActive->linewidth ;
  box->line_style = gActive->line_style ;
  box->pointsize = gActive->pointsize ;
  box->textheight = gActive->textheight ;
  box->fcol = gActive->color ;
  box->format = gActive->textFormat ;
  box->bcol = col;
  box->x1 = 1000000.0 ; box->x2 = -1000000.0 ;
  box->y1 = 1000000.0 ; box->y2 = -1000000.0 ;
  box->flag = 0 ;
#ifdef BETTER_BOX_SHIFT
  box->id = k ;
  box->parentid = parentid ;
#endif

  push (gStk, BOX_START, int) ; push (gStk, k, int) ;
  box->mark = stackMark (gStk) ;

  gBox = box ;
  if (k)
    push (gActive->boxstack, gActive->currbox, int) ;
  gActive->currbox = k ;

  return k ;
}

void graphBoxEnd (void)
{ Box box = gBox ;

  if (gActive->currbox == 0)
    messcrash ("Tried to end box 0") ;

  push (gStk, BOX_END, int) ;
  gActive->currbox = pop (gActive->boxstack, int) ;
  gBox = gBoxGet (gActive->currbox) ;

  if (box->x1 < gBox->x1) gBox->x1 = box->x1 ;
  if (box->x2 > gBox->x2) gBox->x2 = box->x2 ;
  if (box->y1 < gBox->y1) gBox->y1 = box->y1 ;
  if (box->y2 > gBox->y2) gBox->y2 = box->y2 ;
}

BOOL graphBoxClear (int k)
/* toggles appearance status of box and 
   redraws/clears box area as necessary */
{
  Box box = gBoxGet(k) ;

  if (!k)  /* do not clear background box */
    return TRUE ;  

  box->x2 = -box->x2 ;		/* will not redraw if x2 < x1 */
  box->x1 = -box->x1 ;

  if (box->x1 > box->x2)	/* Clear the screen */
   {
     graphClipDraw (uToXabs(-box->x1), uToYabs(box->y1),
		    uToXabs(-box->x2), uToYabs(box->y2)) ;
     return FALSE;
   }
  else
    {
      graphBoxDraw (k, -1, -2) ;	/* expose box */
      return TRUE;
    }
}

BOOL graphBoxMarkAsClear (int k)
{
  Box box = gBoxGet(k) ;

  if (!k)  /* do not clear background box */
    return TRUE ;  

  box->x2 = -box->x2 ;		/* will not redraw if x2 < x1 */
  box->x1 = -box->x1 ;

  return (box->x1 > box->x2) ? FALSE : TRUE ;
}

/* Get some/all of the dimensions of a graph box. */
void graphBoxDim (int k, float *x1, float *y1, float *x2, float *y2)
{
  Box box = gBoxGet (k) ;

  if (x1)
    *x1 = box->x1 ; 
  if (x2)
    *x2 = box->x2 ;
  if (y1)
    *y1 = box->y1 ; 
  if (y2)
    *y2 = box->y2 ;

  return ;
}

/* Set some/all of the dimensions of a graph box, to avoid losing box contents
 * you cannot set the dimensions so the box is smaller than the current box dimensions.
 * This MAY NOT BE the desired way for it work but it does play safe.... */
void graphBoxSetDim(int k, float *x1, float *y1, float *x2, float *y2)
{
  Box box = gBoxGet (k) ;

  if (x1 && *x1 < box->x1)
    box->x1 = *x1 ; 
  if (x2 && *x2 > box->x2)
    box->x2 = *x2 ;
  if (y1 && *y1 < box->y1)
    box->y1 = *y1 ; 
  if (y2 && *y2 > box->y2)
    box->y2 = *y2 ;

  return ;
}


int graphBoxAt(float x, float y, float *rx, float *ry)
{ int i;
  Box box;

  for(i = gActive->nbox; i--; )
    { box = gBoxGet(i);
      if (x >= box->x1 && x <= box->x2 &&
	  y >= box->y1 && y <= box->y2)
	break;
    }

  if (i != -1)
    {
      if (rx) *rx = x - box->x1;
      if (ry) *ry = y - box->y1;
    }

  return i;
}

void graphBoxSetPick (int k, BOOL pick)
{
  Box box = gBoxGet (k) ;

  if (pick)
    box->flag &= ~GRAPH_BOX_NOPICK_FLAG ;
  else
    box->flag |= GRAPH_BOX_NOPICK_FLAG ;
}


/* A new and maybe temp. function to set a flag to exclude a box from menu   */
/* click logic...if this flag is on, then the box is excluded altogether     */
/* from being selected by a right (menu) click.                              */
void graphBoxSetMenu (int k, BOOL menu)
{
  Box box = gBoxGet (k) ;

  if (menu)
    box->flag &= ~GRAPH_BOX_NOMENU_FLAG ;
  else
    box->flag |= GRAPH_BOX_NOMENU_FLAG ;
}


/*********** box info stuff ************/

void graphBoxInfo (int box, KEY key, char *text)
{ 
  BOXINFO *inf ;

  if (!gActive)
    return ;
  if (!gActive->boxInfo)
    gActive->boxInfo = arrayHandleCreate (32, BOXINFO, gActive->clearHandle) ;

  inf = arrayp(gActive->boxInfo, box, BOXINFO) ;
  inf->key = key ;
  inf->text = text ;
}


void graphBoxInfoFile (ACEOUT info_out)
{ 
  int i ;
  BOXINFO *inf ;
  Box box ;


  if (!gActive || !gActive->boxInfo) return ;

  for (i = 0 ; i < arrayMax(gActive->boxes) ; ++i)
    {
      inf = arrayp(gActive->boxInfo, i, BOXINFO) ;
      
      if (!(inf->key || inf->text)) continue ;
      
      
      box = gBoxGet(i) ;
      if (box->x2 < box->x1 || box->y2 < box->y1) /* ignore empty or hidden boxes */
	continue ;
      
      
      aceOutPrint (info_out, "%d   %d %d %d %d",
		   i, uToXabs(box->x1), uToYabs(box->y1), 
		   uToXabs(box->x2), uToYabs(box->y2)) ;
      
      if (inf->key)
	{ 	  
	  /* ACEDB-GRAPH INTERFACE: if acedb has registered a function to print  */
	  /* the acedb class then call it.                                       */
	  if (getGraphAcedbClassPrint() != NULL) (getGraphAcedbClassPrint())(info_out, inf->key) ;
	  else
	    {
	      /* I have no idea why this is done again, it has already been done     */
	      /* above and box is not used below...remove ?                          */
	      box = gBoxGet(i) ;
	      
	      aceOutPrint (info_out, "  %d", inf->key) ;
	    }
	}

      if (inf->text)
	aceOutPrint (info_out, "  %s", inf->text) ;
	  
      aceOutPrint (info_out, "\n");
    }
      
      
  return ;
} /* graphBoxInfoFile */



void graphBoxInfoWrite (void)
{ 
  FILE *tmp_fil ;
  ACEOUT info_out;
  static char directory[DIR_BUFFER_SIZE] = "";
  static char filename[FIL_BUFFER_SIZE] = "";

  if (!gActive || !gActive->boxInfo)
    return;

  tmp_fil = filqueryopen (directory, filename, "box", "w",
			  "File for box information");
  if (!tmp_fil)
    return ;
  filclose (tmp_fil);		/* just interested in filename */

  info_out = aceOutCreateToFile
    (messprintf("%s/%s.box", directory, filename), "w", 0);

  if (info_out)
    {
      graphBoxInfoFile (info_out) ;
      aceOutDestroy (info_out) ;
    }

  return;
} /* graphBoxInfoWrite */

/********************* text entry box *******************************/
/** structure definition must be here because used in gLeftDown() ***/

typedef struct EntryBoxStruct
  { int		magic ;
    char*	text ;
    char*	cp ;
    int		len ;
    int		box ;
    int		cursorBox ;
    int         insertMode ;
    void	(*action)(char*) ;
    GraphCompletionFunc  completionFunction ;
    struct EntryBoxStruct *nxt ;
    char*	winText ;
    int		winPos ;
    int		winLen ;
  } *EntryBox ;

static int EBOX_MAGIC = 187648 ;
static int TBOX_HANDLE ;

static int graphTextEntry2 (GraphCompletionFunc f, char* text, 
			    int len, int winlen,
			    float x, float y, void (*fn)(char*)) ;
static void graphPositionCursorFromEvent (EntryBox ebox) ;
static void entryBoxEntry (int key, int modifier_unused) ;

/********* box picking and dragging **********/

static void (*dragRoutine)(float*,float*,BOOL) ;
static GraphFunc leftDragR, leftUpR, middleDragR, middleUpR;
  /*saved over dragging operations, by graphBoxDrag, restored by upBox*/
static float oldx,oldy,xbase,ybase,xuser,yuser ;
static int draggedBox ;

void gLeftDown(float x, float y, int modifier)
{
  int i = -1 ;
  Box box ;
  VoidRoutine buttonFunc ;
  typedef void (*MouseFunc)(double, double) ;
  typedef void (*PickFunc)(int, double, double, int) ;
  MouseFunc mfn ;
  PickFunc pfn ;
  Graph old ;
  void *arg;
  int bg, fg;

  for (i = gActive->nbox ; i-- ;)
    { box = gBoxGet (i) ;
      if (box->flag & GRAPH_BOX_NOPICK_FLAG)
	continue ;
      if (x >= box->x1 && x <= box->x2 && 
	  y >= box->y1 && y <= box->y2)
	break ;
    }

  if (i == -1) 	/* outside even the whole drawing area! */
    return ;

  if (box->flag & GRAPH_BOX_ENTRY_FLAG)	/* make this the active entry */
    { EntryBox ebox ;
      if (graphAssFind (&TBOX_HANDLE, &ebox))
	for ( ; ebox ; ebox = ebox->nxt)
	  if (i == ebox->box || i == ebox->cursorBox)
	    { graphTextEntry2 (0, ebox->text, 0, 0, 0, 0, 0) ;
	      graphPositionCursorFromEvent (ebox) ;
	    }
		/* NB - continue because user may need to do something */
    }
  else if (box->flag & GRAPH_BOX_TOGGLE_FLAG)
    editorToggleChange(i);
  else if (i && gActive->buttonAss && 
	   assFind (gActive->buttonAss, assVoid(i*4), &buttonFunc))
    { graphBoxDraw (i, WHITE, BLACK) ;
      old = graphActive() ;
				/* do the action */
      if (assFind(gActive->buttonAss, assVoid(i*4 + 1), &arg))
	{ ColouredButtonFunc cbf = (ColouredButtonFunc) buttonFunc ;
	  cbf (arg) ; /* mieg, to please the IBM compiler */
	}
      else
	(*buttonFunc)() ;
				/* redraw button */
      if (graphActivate(old) &&
	  i < gActive->nbox && (box = gBoxGet(i)) &&
	  box->fcol == WHITE && box->bcol == BLACK &&
	  gActive->buttonAss &&
	  assFind (gActive->buttonAss, assVoid(i*4), &buttonFunc))
	{ fg = (assFind(gActive->buttonAss, assVoid(i*4 + 2), &arg)) 
	    ? assInt(arg) : BLACK ;
	  bg = (assFind(gActive->buttonAss, assVoid(i*4 + 3), &arg))
	    ? assInt(arg) : WHITE ;
	  graphBoxDraw (i, fg, bg) ;
	}
      return ;
    }

  /* If the user clicked the left mouse button on something, this is where   */
  /* the applications callback function gets called, we use busy cursors to  */
  /* warn the user we are busy.                                              */
  /* This applies to PICK & LEFT_DOWN.                                        */
  if (gActive->func[PICK])
    {
    pfn = (PickFunc)(gActive->func[PICK]) ;

    graphBusyCursorAll ();

    (*pfn)(i,(double)(x - box->x1),(double)(y - box->y1), modifier) ;

    return ;
  }
  
  if (gActive->func[LEFT_DOWN])
    {
    mfn = (MouseFunc)(gActive->func[LEFT_DOWN]);

    graphBusyCursorAll ();
    
    (*mfn)((double)x,(double)y) ;
    }

  return ;
}


void gMiddleDown (float x, float y)
{
  typedef void (*MouseFunc)(double, double) ;
  MouseFunc mfn ;
  EntryBox ebox ;
  int i ;
  Box box = 0 ;

  for (i = gActive->nbox ; i-- ;)
    { box = gBoxGet (i) ;
      if (box->flag & GRAPH_BOX_NOPICK_FLAG)
	continue ;
      if (x >= box->x1 && x <= box->x2 && 
	  y >= box->y1 && y <= box->y2)
	break ;
    }
  if (box->flag & GRAPH_BOX_ENTRY_FLAG &&
      graphAssFind (&TBOX_HANDLE, &ebox))	/* make active and paste */
    { for ( ; ebox ; ebox = ebox->nxt)
	if (i == ebox->box || i == ebox->cursorBox)
	  { graphTextEntry2 (0, ebox->text, 0, 0, 0, 0, 0) ;
	    graphPositionCursorFromEvent (ebox) ;
	    entryBoxEntry(25, 0) ; /* CTRL-Y is paste action */
	  }
      /* NB - continue because user may need to do something */
    }

  /* If the user clicked the middle mouse button on something, this is where */
  /* the applications callback function gets called, we use busy cursors to  */
  /* warn the user we are busy.                                              */
  if (gActive->func[MIDDLE_DOWN])
    {
    mfn = (MouseFunc)(gActive->func[MIDDLE_DOWN]);

    graphBusyCursorAll ();
    (*mfn)((double)x,(double)y) ;
    }


  return ;
}


static void dragBox (double x, double y)
{
  float oldxuser = xuser, oldyuser = yuser ;
 
  xbase += x-oldx ;
  ybase += y-oldy ;
  xuser = xbase;
  yuser = ybase;
  (*dragRoutine)(&xuser,&yuser,FALSE) ;
  if (xuser != oldxuser || yuser != oldyuser)
    { graphXorBox (draggedBox,oldxuser,oldyuser) ;
      graphXorBox (draggedBox,xuser,yuser) ;
    }
  oldx = x ; oldy = y ;
}

static void upBox (double x, double y)
{ 
  graphXorBox (draggedBox,xuser,yuser) ;
  xbase += x-oldx ;
  ybase += y-oldy ;
  (*dragRoutine)(&xbase,&ybase, TRUE) ;

  /*.....restore the saved methods*/
  graphRegister(LEFT_DRAG, leftDragR);
  graphRegister(LEFT_UP, leftUpR);
  graphRegister(MIDDLE_DRAG, middleDragR);
  graphRegister(MIDDLE_UP, middleUpR);

  return;
}

void graphBoxDrag (int k, void (*clientRoutine)(float*,float*,BOOL))
{
  Box box = gBoxGet(k) ;

  dragRoutine = clientRoutine ;
  draggedBox = k ;

  leftDragR=graphRegister(LEFT_DRAG, dragBox);
  leftUpR=graphRegister(LEFT_UP, upBox);
  middleDragR=graphRegister(MIDDLE_DRAG, dragBox);
  middleUpR=graphRegister(MIDDLE_UP, upBox);
    /*saving the existing methods for restoration by upBox*/

  xbase = xuser = box->x1 ; ybase = yuser = box->y1 ;
  oldx = graphEventX ; oldy = graphEventY ; 
  (*dragRoutine) (&xuser,&yuser,FALSE) ;

  graphXorBox (draggedBox,xuser,yuser) ;

  return;
}

/*****************************************************************/
/******* text entry box code - structure before gLeftDown() ******/

static void graphUnSetCursor(EntryBox ebox)
{ 
  graphBoxDraw (ebox->cursorBox, LIGHTGREEN, TRANSPARENT) ;
}

/******************************/

static void graphSetCursor(EntryBox ebox)
{ 
  float x1,x2,y1,y2 ;
  int offset = ebox->cp - ebox->text ;

  if (ebox->winText != ebox->text)
    { if (offset < 0)
	offset = 0 ;
      if (offset < ebox->winPos)
	ebox->winPos = offset ;
      else if (offset >= ebox->winPos + ebox->winLen)
	ebox->winPos = offset - ebox->winLen + 1 ;
      strncpy (ebox->winText, ebox->text + ebox->winPos, ebox->winLen) ;
    }
      
  graphBoxDim (ebox->box, &x1, &y1, &x2, &y2) ;
  graphBoxShift (ebox->cursorBox, 
		 x1 + UtextX*(ebox->cp - ebox->text - ebox->winPos),
		 y1) ;
  graphBoxDraw (ebox->cursorBox, RED, TRANSPARENT) ;
}

static void graphPositionCursorFromEvent (EntryBox ebox)
{ 
  float x1,x2,y1,y2 ;

  graphBoxDim (ebox->box, &x1, &y1, &x2, &y2) ;
  if (graphEventY >= y1 && graphEventY <= y2)
    { int n = (graphEventX - x1) + ebox->winPos ;
      if (n <= 0 || n > strlen(ebox->text))
	n = strlen(ebox->text) ;
      ebox->cp = ebox->text + n ;
    }

  graphSetCursor (ebox) ;
}

/******************************/
/* Copy the pasted text into the ebox text buffer.                           */
/*                                                                           */
static void entryBoxPasteCallBack(char *cr)
{
  EntryBox ebox ; 
  char *cp, *cq;
  int maxtextlen ;
  
  if (!graphAssFind (&TBOX_HANDLE, &ebox) ||
      ebox->magic != EBOX_MAGIC )
    messcrash ("entryBoxPasteCallBack() can't find entryBox") ;

  /* Max. length of string is the buffer length - 1 !                        */
  maxtextlen = ebox->len - 1 ;
  
  cp = ebox->cp;
  
  if (ebox->insertMode)
    while (*cr && strlen(ebox->text) < maxtextlen)
      { 
	cq = ebox->text + strlen(ebox->text) ;
	while (--cq >= cp) 
	  *(cq + 1) = *cq ;
	*cp++ = *cr++ ;
      }
  else
    while (*cr && cp <= ebox->text + maxtextlen)
      *cp++ = *cr++ ;

  ebox->cp = cp ;
  graphSetCursor (ebox) ;
  graphBoxDraw (ebox->box,-1,-1) ;

  return;
} /* entryBoxPasteCallBack */

/******************************/


/* Handle keyboard input from user for text entry box.
 *
 * You need to be careful calculating lengths here, the string can be up to
 * (ebox->len - 1) chars long which allows for the terminating null byte.
 */
static void entryBoxEntry (int key, int modifier_unused)
{
  EntryBox ebox ;
  int n, oldbox ;
  char *cp, *cq, *cr ;
  Graph currGraph = graphActive () ;
  int maxtextlen ;

  if (!graphAssFind(&TBOX_HANDLE, &ebox) || ebox->magic != EBOX_MAGIC )
    messcrash(GRAPH_INTERNAL_LOGIC_ERROR_MSG "entryBoxEntry() can't find entryBox") ;


  /* Max. length of string is the buffer length - 1 !                        */
  maxtextlen = ebox->len - 1 ;

  if (strlen(ebox->text) > maxtextlen)
    messcrash (GRAPH_INTERNAL_LOGIC_ERROR_MSG
	       "Over flow in entryBoxEntry, buffer length: %d,  text length: %d",
	       ebox->len, strlen(ebox->text)) ;


  /* I'm not sure how this can happen, the test was in the old code so I've  */
  /* left it in but put in an error message.                                 */
  cp = ebox->cp ;
  if (cp < ebox->text || cp > ebox->text + maxtextlen)
    {
      messerror(GRAPH_INTERNAL_LOGIC_ERROR_MSG
		"Text cursor is outside of text box and has been reset to end of current text.") ;
      cp = ebox->text + strlen(ebox->text) ;
    }


  switch (key)
    {
    case '\t': /* auto complete */
      if (ebox->completionFunction)
	n = (*ebox->completionFunction)(ebox->text, ebox->len) ;
      else
	n = 0 ;
      cp = ebox->text + strlen(ebox->text) ;
      if (n != 1)
	break ;			/* note fall-through if one match only */

    case RETURN_KEY:
      ebox->cp = cp ;
      graphSetCursor(ebox) ;
      graphBoxDraw (oldbox = ebox->box, BLACK, RED) ;

      if (ebox->action)
	(*(ebox->action))(ebox->text) ;

      graphActivate (currGraph) ;  /* May work or not */

      if (graphAssFind (&TBOX_HANDLE, &ebox) &&  /* reget after action */
	  ebox->magic == EBOX_MAGIC &&
	  ebox->box == oldbox &&
	  ebox->box < gActive->nbox)
	{
	  Box testBox = gBoxGet (ebox->box) ;

	  /* test if ebox->action hasn't changed the entry boxes colour to make it look inactive */
	  if (testBox->fcol == BLACK && testBox->bcol == RED)
	    /* action hasn't change it, so redraw it YELLOW */
	    graphBoxDraw (ebox->box, BLACK, YELLOW) ;
	}

      /* Reactivate any keyboard handling at the device level.               */
      if (gActive->dev && gDevFunc->graphDevEnableKeyboard)
	gDevFunc->graphDevEnableKeyboard(gActive->dev) ;

      return ;   /* because action may have destroyed or graphRedrawn */

    case L_KEY:			/* C-l center box on cursor */  
      if (cp > ebox->text + ebox->winLen/2)
	ebox->winPos = cp - ebox->text - ebox->winLen/2 ;
      else
	ebox->winPos = 0 ;
      break;

    case INSERT_KEY:
      ebox->insertMode = 1 - ebox->insertMode ;
      break ;

    case DELETE_KEY:
    case BACKSPACE_KEY:		/* delete one char to my left */
      if (cp == ebox->text)
	break ;
      cq = --cp ;
      while (*cq)
	{ *cq = *(cq + 1) ;
	  cq++ ;
	}
      break ;

    case HOME_KEY:
    case A_KEY:			/* C-a */
      cp = ebox->text ; 
      break ;

    case LEFT_KEY:
    case B_KEY:			/* C-b */
      if (cp > ebox->text)
	cp-- ;
      break ;

    case C_KEY:			/* C-c copy whole entry */
      graphPostBuffer(ebox->text) ;
      break ;

    case X_KEY:			/* C-x kill whole entry */
      cq = ebox->text + maxtextlen ;
      cp = ebox->text ;
      while (cq >= cp) 
	*cq-- = 0 ;
      break ;

    case D_KEY:			/* C-d delete at cp */
      cq = cp ;
      while(*cq)
	{ *cq = *(cq + 1) ;
	  cq ++ ;
	}
      break ;

    case END_KEY:
    case E_KEY:			/* C-e go to last char */
      cp = ebox->text + strlen(ebox->text) ;
      break ;

    case RIGHT_KEY:
    case F_KEY:			/* C-f */
      if (*cp)
	cp++ ;
      if ((cp > (ebox->text + maxtextlen)) && ebox->len > 0 ) /* wierd...when is len <= 0 ?? */
	cp-- ;
      break ;

    case K_KEY:			/* C-k kill from cp to the right */
      cq = ebox->text + maxtextlen ;
      graphPostBuffer (cp) ;
      while (cq >= cp) 
	*cq-- = 0 ;
      break ;

    case U_KEY:			/* C-u kill _and_ post whole entry */
      cq = ebox->text + maxtextlen ;
      cp = ebox->text ;
      graphPostBuffer (cp) ;
      while (cq >= cp) 
	*cq-- = 0 ;
      break ;

    case V_KEY:                    /* C-v is windows "paste" */
    case Y_KEY:			/* C-y yank buffer */
      graphPasteBuffer(entryBoxPasteCallBack) ;
      break ;

    case W_KEY:			/* C-w delete a word leftwards at cp */
      if (cp == ebox->text)
	break ;
      cr = cq = cp ;
      while (*cq == ' ' && cq > ebox->text) cq-- ;
      while (*cq != ' ' && cq > ebox->text) cq-- ;
      cp = cq ;
      while ((*cq++ = *cr++)) ;	/* copy */
      while (*cq) *cq++ = 0 ;	/* clean up */
      break ;

    default:
      if (!isascii(key) || !isprint(key))
	{
	  ebox->cp = cp ; 
	  return ; 
	}
      
      if (ebox->insertMode)
	{
	  if (strlen(ebox->text) < maxtextlen)
	    {
	      cq = ebox->text + strlen(ebox->text) ;
	      while (--cq >= cp) 
		*(cq + 1) = *cq ;
	      *cp++ = key ;
	    }
	}
      else
	{
	  if (cp < ebox->text + maxtextlen)
	    *cp++ = key ;
	}

      break ;
    }

  ebox->cp = cp ;
  graphSetCursor (ebox) ;
  graphBoxDraw (ebox->box,-1,-1) ;

  return;
} /* entryBoxEntry */

static int graphTextEntry2 (GraphCompletionFunc f, char* text, 
			    int len, /* buffer-length, i.e. max-num of chars+1 */
			    int winlen,
			    float x, float y, void (*fn)(char*))
{
  EntryBox ebox = 0, previous = 0, old = 0 ;
  int n = (int)x ;
  char *cp ;

  if (!gActive)
    return 0 ;

  if (len < 0 || len < winlen)
    messcrash ("Negative length in graphTextEntry") ;
  if (!text)
    messcrash ("Null text in graphTextEntry") ;

  cp = text - 1 ;  /* Clean the buffer */
  while (*++cp) ;
  while (cp < text + len)
    *cp++ = 0 ;

  graphRegister (KEYBOARD, entryBoxEntry) ;

  if (graphAssFind (&TBOX_HANDLE, &previous))
    {
      if (previous->text == text)
	{
	  n = strlen(text) ;
	  if (n > previous->len)
	    messcrash ("graphTextEntry2 text > limit %d :\n %s",
		       previous->len, text) ;

	  for ( ; n < previous->len ; n++)
	    text[n] = 0 ;  /* clean up text */

	  if (previous->cp > text + strlen(text))
	    previous->cp = text + strlen(text) ;

	  graphSetCursor (previous) ;
	  graphBoxDraw (previous->box, BLACK, YELLOW) ;

	  /* Disable any keyboard handling at device level because it will interfer with
	   * text entry box keyboard handling (e.g. arrow keys). */
	  if (gActive->dev && gDevFunc->graphDevDisableKeyboard)
	    gDevFunc->graphDevDisableKeyboard(gActive->dev) ;
	  
	  return previous->box ;
	}
      else
	{
	  graphUnSetCursor (previous) ;
	  graphBoxDraw (previous->box, BLACK, LIGHTGREEN) ;
	  graphAssRemove (&TBOX_HANDLE) ;
	  old = previous ;

	  for (ebox = old->nxt ; ebox && ebox->text != text ; ebox = ebox->nxt)
	    old = ebox ; 
	}
    }

  if (old && ebox)			/* merely unlink */
    old->nxt = ebox->nxt ;
  else				/* new box */
    {
      ebox = (EntryBox) handleAlloc (0, gActive->clearHandle, sizeof (struct EntryBoxStruct)) ;
      ebox->magic = EBOX_MAGIC ;
      ebox->text = text ;
      ebox->cp = text + strlen(text) ;
      ebox->len = len ;
      ebox->winLen = winlen ;
      if (winlen < len)
	ebox->winText = (char*) handleAlloc (0, gActive->clearHandle, winlen+1) ;
      else
	ebox->winText = text ;
      ebox->winPos = 0 ;
      ebox->action = fn ;
      ebox->completionFunction = f ;
      ebox->insertMode = 1 ;
      
      ebox->box = graphBoxStart () ;
      graphTextPtr (ebox->winText,x,y,winlen) ;
      ebox->cursorBox = graphBoxStart () ;
      graphLine (x, y, x, y+UtextY) ;
      graphBoxEnd () ;		/* cursor */
      graphBoxEnd () ;		/* ebox */

      {
	Box box = gBoxGet(ebox->box) ;	/* set flag for gLeftDown */

	box->flag |= GRAPH_BOX_ENTRY_FLAG ;
	box = gBoxGet(ebox->cursorBox) ;
	box->flag |= GRAPH_BOX_ENTRY_FLAG ;
      }

      text[len-1] = 0 ;		/* for subsequent safety !!!!!!!!!!!!!*/
    }

  ebox->nxt = previous ;	/* 0 if previous not found */
  graphSetCursor (ebox) ;
  graphBoxDraw (ebox->box, BLACK, YELLOW) ;
  graphAssociate (&TBOX_HANDLE, ebox) ;
 
  return ebox->box ;
}

int graphTextEntry (char* text, int len, float x, float y, void (*fn)(char*))
{
  return graphTextEntry2 (0, text, len, len, x, y, fn) ;
}

int graphCompletionEntry (GraphCompletionFunc f, char* text, int len, float x, float y, void (*fn)(char*))
{
  return graphTextEntry2 (f, text, len, len, x, y, fn) ;
}

/* gha added next two procedures
   Richard rewwrote this because of cursor problems
 */
int graphTextScrollEntry (char* text, int len, int wlen, float x, float y, void (*fn)(char*))
{ 
  return graphTextEntry2 (0, text, len, wlen, x, y, fn) ;
}

int graphCompScrollEntry (GraphCompletionFunc f, char* text, int len, int wlen, float x, float y, void (*fn)(char*))
{ 
  return graphTextEntry2 (f, text, len, wlen, x, y, fn) ;
}

void graphEntryDisable (void)
{
  EntryBox ebox ;

  if (graphAssFind (&TBOX_HANDLE, &ebox))
    {
      graphUnSetCursor(ebox) ;
      graphBoxDraw (ebox->box, BLACK, LIGHTGREEN) ;
      graphRegister (KEYBOARD, 0) ;
    }

  return ;
}

/********* clear box list, stacks etc - also used during initialisation ***/

void graphDeleteContents (Graph_ theGraph)  /* Used in graphDestroy() instead of graphClear() */
{
  EntryBox ebox ;

  if (!theGraph)
    return ;
  
  if (theGraph->dev && gDevFunc->graphDevClear)
    gDevFunc->graphDevClear(theGraph->dev);
  
  stackClear (theGraph->boxstack) ;
  stackClear (theGraph->stack) ;
  assClear (theGraph->buttonAss) ;
  assClear (gActive->boxMenus) ;

  /* clean up things hanging off clearHandle */
  handleDestroy (theGraph->clearHandle) ;
  theGraph->clearHandle = handleHandleCreate (theGraph->handle) ;
  theGraph->editors = 0 ;	/* zero dangling pointers */
  theGraph->boxInfo = 0 ;

  if (graphAssFind (&TBOX_HANDLE, &ebox))
    { graphAssRemove (&TBOX_HANDLE) ;
      graphRegister (KEYBOARD, 0) ;
    }
}

void graphClear ()
{
  if (!gActive)
    return ;

  graphDeleteContents (gActive) ;
  graphWhiteOut () ;

  gActive->nbox = 0 ;
  graphBoxStart () ;
  graphTextHeight (0.0) ;
  graphColor (BLACK) ;
  graphTextFormat (PLAIN_FORMAT) ;
  gActive->isClear = TRUE ;

  graphGoto (0.0,0.0) ;		/* RMD 28/5/92 */
}

/********************************* Low Level Graphics ************/

                        /* checks to increase box size */
#define CHECK4\
  if (x0 < x1) { if (x0 < gBox->x1) gBox->x1 = x0 ;\
		   if (x1 > gBox->x2) gBox->x2 = x1 ; }\
  else         { if (x1 < gBox->x1) gBox->x1 = x1 ;\
		   if (x0 > gBox->x2) gBox->x2 = x0 ; }\
  if (y0 < y1) { if (y0 < gBox->y1) gBox->y1 = y0 ;\
		   if (y1 > gBox->y2) gBox->y2 = y1 ; }\
  else         { if (y1 < gBox->y1) gBox->y1 = y1 ;\
		   if (y0 > gBox->y2) gBox->y2 = y0 ; }
#define CHECK3\
  if (x-r < gBox->x1) gBox->x1 = x-r ;\
  if (x+r > gBox->x2) gBox->x2 = x+r ;\
  if (y-r < gBox->y1) gBox->y1 = y-r ;\
  if (y+r > gBox->y2) gBox->y2 = y+r ;

#define CHECK_TEXT\
  if (x < gBox->x1)         gBox->x1 = x ;\
  if (x + len > gBox->x2)   gBox->x2 = x + len ;\
  if (y < gBox->y1)         gBox->y1 = y ;\
  if (y + UtextY > gBox->y2)     gBox->y2 = y + UtextY ;

void graphLine (float x0, float y0, float x1, float y1)

{ push (gStk, LINE, int) ;
  push (gStk, x0, float) ; push (gStk, y0, float) ;
  push (gStk, x1, float) ; push (gStk, y1, float) ;
  CHECK4
}

void graphRectangle (float x0, float y0, float x1, float y1)
{ push (gStk, RECTANGLE, int) ;
  push (gStk, x0, float) ; push (gStk, y0, float) ;
  push (gStk, x1, float) ; push (gStk, y1, float) ;
  CHECK4
}

void graphFillRectangle (float  x0, float y0, float x1, float y1)
{ push (gStk, FILL_RECTANGLE, int) ;
  push (gStk, x0, float) ; push (gStk, y0, float) ;
  push (gStk, x1, float) ; push (gStk, y1, float) ;
  CHECK4
}

void graphCircle (float x, float y, float r)
{ push (gStk, CIRCLE, int) ;
  push (gStk, x, float) ; push (gStk, y, float) ; push (gStk, r, float) ;
  CHECK3
}

void graphPoint (float x, float y)
{ float r = 0.5 * gActive->pointsize ;  /* needed for check3 */
  push (gStk, POINT, int) ;
  push (gStk, x, float) ; push (gStk, y, float) ;
  CHECK3
}

void graphText (char *text, float x, float y)
{
  float len ;

  if (!gActive || !text) 
    return ;

  push (gStk, TEXT, int) ;
  push (gStk, x, float) ;
  push (gStk, y, float) ;
  pushText (gStk, text) ;

  len = strlen(text) * UtextX ;
  CHECK_TEXT
}

void graphTextUp (char *text, float x, float y)
{
  float len ;

  if (!gActive || !text) 
    return ;

  push (gStk, TEXT_UP, int) ;
  push (gStk, x, float) ;
  push (gStk, y, float) ;
  pushText (gStk, text) ;

  len = strlen(text) ;
  if (x < gBox->x1) gBox->x1 = x ;
  if (x + 1 > gBox->x2) gBox->x2 = x + 1 ;
  if (y - len*0.6 < gBox->y1) gBox->y1 = y - len*0.6 ;
  if (y > gBox->y2) gBox->y2 = y ;
}

void graphTextPtr (char *text, float x, float y, int st_len)
{
  float len ;

  if (!gActive || !text)
    return ;

  push (gStk, TEXT_PTR, int) ;
  push (gStk, x, float) ;
  push (gStk, y, float) ;
  push (gStk, text, char*) ;

  len = st_len * UtextX ;
  CHECK_TEXT ;
} 

void graphTextPtrPtr (char **text, float x, float y, int st_len)
{
  float len ;

  if (!text)
    return ;
  push (gStk, TEXT_PTR_PTR, int) ;
  push (gStk, x, float) ;
  push (gStk, y, float) ;
  push (gStk, text, char**) ;

  len = st_len * UtextX ;
  CHECK_TEXT
} 

void graphPixels (unsigned char *pixels, int w, int h, int len, 
		  float x0, float y0, 
		  unsigned int *colors, int ncols)
{
  float x1, y1 ;
  
  push (gStk, PIXELS, int) ;
  push (gStk, x0, float) ; push (gStk, y0, float) ;
  push (gStk, pixels, unsigned char*) ;
  push (gStk, w, int) ; 
  push (gStk, h, int) ; 
  push (gStk, len, int) ;
  push (gStk, (unsigned int *)colors, unsigned int *) ; push (gStk, ncols, int) ;

  x1 = x0 + XtoUrel(w) ;
  y1 = y0 + YtoUrel(h) ;
  CHECK4
}

void graphPixelsRaw (unsigned char *pixels, int w, int h, int len,
		     float x0, float y0)
{
  float x1, y1 ;

  push (gStk, PIXELS_RAW, int) ;
  push (gStk, x0, float) ; push (gStk, y0, float) ;
  push (gStk, pixels, unsigned char*) ;
  push (gStk, w, int) ; 
  push (gStk, h, int) ; 
  push (gStk, len, int) ;

  x1 = x0 + XtoUrel(w) ;
  y1 = y0 + YtoUrel(h) ;
  CHECK4
}

void graphColorSquares (char *colors, float x, float y, int len, int skip, int *tints)
{
  if (len < 0)
    messcrash ("len < 0 in graphColorSquares") ;
  if (!len)
    return ;

  push (gStk, COLOR_SQUARES, int) ;
  push (gStk, x, float) ;
  push (gStk, y, float) ;
  push (gStk, colors, char*) ;
  push (gStk, len, int) ;
  push (gStk, skip, int) ;
  push (gStk, tints, int*) ;

  CHECK_TEXT
}

void graphPolygon (Array pts)
{
  float x, y ;
  int i ;

  push (gStk, POLYGON, int) ;
  push (gStk, arrayMax(pts)/2, int);

  for (i = 0 ; i < arrayMax(pts)-1 ; i += 2)
    {
      x = arr (pts, i, float) ;
      y = arr (pts, i+1, float) ;
      push (gStk, x, float) ;
      push (gStk, y, float) ;
      if (x < gBox->x1) gBox->x1 = x ;
      if (y < gBox->y1) gBox->y1 = y ;
      if (x > gBox->x2) gBox->x2 = x ;
      if (y > gBox->y2) gBox->y2 = y ;
    }
}

void graphLineSegs (Array pts)
{
  float x, y ;
  int i ;

  push (gStk, LINE_SEGS, int) ;
  push (gStk, arrayMax(pts)/2, int);

  for (i = 0 ; i < arrayMax(pts)-1 ; i += 2)
    { x = arr (pts, i, float) ;
      y = arr (pts, i+1, float) ;
      push (gStk, x, float) ;
      push (gStk, y, float) ;
      if (x < gBox->x1) gBox->x1 = x ;
      if (y < gBox->y1) gBox->y1 = y ;
      if (x > gBox->x2) gBox->x2 = x ;
      if (y > gBox->y2) gBox->y2 = y ;
    }
}

void graphArc (float x, float y, float r, float ang, float angDiff)
{ push (gStk, ARC, int) ;
  push (gStk, x, float) ; push (gStk, y, float) ; 
  push (gStk, r, float) ; 
  CHECK3
  push (gStk, ang, float) ; 
  push (gStk, angDiff, float) ; 
}

void graphFillArc (float x, float y, float r, float ang, float angDiff)
{ push (gStk, FILL_ARC, int) ;
  push (gStk, x, float) ; push (gStk, y, float) ; 
  push (gStk, r, float) ; 
  CHECK3
  push (gStk, ang, float) ; 
  push (gStk, angDiff, float) ; 
}

/********************** routines to set static properties ************/

float graphLinewidth (float x)
{
  float old = gActive->linewidth ;

  if (x >= 0)
    {
      push (gStk, LINE_WIDTH, int) ;
      push (gStk, x, float) ;
      gActive->linewidth = x ;
    }

  return old ;
}

GraphLineStyle graphLineStyle(GraphLineStyle style)
{
  GraphLineStyle old = gActive->line_style ;

  messAssert(style == GRAPH_LINE_SOLID || style == GRAPH_LINE_DASHED
	     || style < 0) ;

  if (style >= 0)
    {
      push(gStk, LINE_STYLE, int) ;
      push(gStk, style, int) ;
      gActive->line_style = style ;
    }

  return old ;
}

float graphPointsize (float x)
{float old = gActive->pointsize ;
  if (x >= 0)
    { push (gStk, POINT_SIZE, int) ;
      push (gStk, x, float) ;
      gActive->pointsize = x ;
    }
  return old ;
}

float graphTextHeight (float x)
{ float old = gActive->textheight ;
  float dx, dy ;
  if (x >= 0)
    { push (gStk, TEXT_HEIGHT, int) ;
      push (gStk, x, float) ;
      gActive->textheight = x ;
      graphTextInfo (&gActive->textX, &gActive->textY, &dx, &dy) ;
    }
  return old ;
}

int graphColor (int color)
{ int old = gActive->color ;

  if (color >= 0)
    { push (gStk, COLOR, int) ;
      push (gStk, color, int) ;
      gActive->color = color ;
    }
  return old ;
}

int graphTextFormat (int textFormat)
{ int old = gActive->textFormat ;

  if (textFormat >= 0)
    { push (gStk, TEXT_FORMAT, int) ;
      push (gStk, textFormat, int) ;
      gActive->textFormat = textFormat ;
    }
  return old ;
}

int graphTextAlign (int textAlignment)
{ int old = GRAPH_TEXT_TOP | GRAPH_TEXT_LEFT ;

/* Currently this function only does something in the Windows implementation.*/ 

  return old ;
}

/******** routine to shift a box to a new origin ********/

#ifdef BETTER_BOX_SHIFT
static void adjustParentBounds(Box box)
{
  if (box->id) /* for all boxes other than box0 */
    { Box pBox = gBoxGet(box->parentid) ;

      if (box->x1 < pBox->x1) pBox->x1 = box->x1 ;
      if (box->x2 > pBox->x2) pBox->x2 = box->x2 ;
      if (box->y1 < pBox->y1) pBox->y1 = box->y1 ;
      if (box->y2 > pBox->y2) pBox->y2 = box->y2 ;
      adjustParentBounds (pBox) ;
    }
}
#endif

void graphBoxShift (int kbox, float xbase, float ybase)
{
  Box box = gBoxGet (kbox), subBox, box0 ;
  float dx = xbase - box->x1 ;
  float dy = ybase - box->y1 ;
  int nDeep = 1 ;
  int action, r ;
  float t ;
  char* text ;
  unsigned char *utext;

  stackCursor (gStk,box->mark) ; /* sets position to mark */
  while (!stackAtEnd (gStk))
    switch (action = stackNext (gStk,int))
      {
      case BOX_END:
	if (!--nDeep)
	  goto redraw ;                        /* exit point */
	break ;
      case BOX_START:
	r = stackNext (gStk, int) ; /* shift its coords */
	subBox = gBoxGet (r) ;
	subBox->x1 += dx ; subBox->x2 += dx ;
	subBox->y1 += dy ; subBox->y2 += dy ;
	++nDeep ;
	break ;
      case COLOR: case TEXT_FORMAT:
	r = stackNext (gStk,int) ; 
	break ;
      case LINE_WIDTH: case TEXT_HEIGHT: case POINT_SIZE:
	t = stackNext (gStk,float) ;
	break ;
      case LINE_STYLE:
	r = stackNext (gStk,int) ;
	break ;
      case LINE: case RECTANGLE: case FILL_RECTANGLE:
	stackNext (gStk,float) += dx ;
	stackNext (gStk,float) += dy ;
	stackNext (gStk,float) += dx ;
	stackNext (gStk,float) += dy ;
	break ;
      case PIXELS: case PIXELS_RAW:
	stackNext (gStk,float) += dx ;
	stackNext (gStk,float) += dy ;
	utext = stackNext (gStk, unsigned char*) ;
	r = stackNext (gStk, int) ;
	r = stackNext (gStk, int) ;
	r = stackNext (gStk, int) ;
	if (action == PIXELS)
	  { unsigned int *uintp = stackNext (gStk, unsigned int*) ;
	    r = stackNext (gStk,int) ;
	  }
	break ;
      case POLYGON : case LINE_SEGS:
	r = stackNext (gStk, int) ;
	if (r > 2)
	  while (r--)
	    { stackNext (gStk,float) += dx ;
	      stackNext (gStk,float) += dy ;
	    }
	break ;
      case CIRCLE: case POINT: 
      case TEXT: case TEXT_UP: case TEXT_PTR: case TEXT_PTR_PTR:
      case COLOR_SQUARES: case FILL_ARC: case ARC:
	stackNext (gStk,float) += dx ;
	stackNext (gStk,float) += dy ;
	switch (action)
	  {
	  case CIRCLE:
	    t = stackNext (gStk,float) ; 
	    break ;
	  case FILL_ARC: case ARC:
	    t = stackNext (gStk,float) ; 
	    t = stackNext (gStk,float) ; 
	    t = stackNext (gStk,float) ; 
	    break ;
	  case POINT: 
	    break ;
	  case TEXT: case TEXT_UP:
	    text = stackNextText (gStk) ;
	    break ;
	  case TEXT_PTR:
	    text = stackNext (gStk,char*) ;
	    break ;
	  case TEXT_PTR_PTR:
	    text = *stackNext (gStk,char**) ;
	    break ;
	  case COLOR_SQUARES:
	    text = stackNext (gStk,char*) ; /* colors */
	    r = stackNext (gStk, int) ;	/* len */
	    r = stackNext (gStk, int) ;	/* skip */
	    text = (char*) stackNext (gStk,int*) ; /* tints */
	    break ;
	  }
	break ;
      case IMAGE: 
	{ void *gim = stackNext (gStk,void*) ; 
	  gim = 0 ;		/* to suppress compiler warning */
	}
	break ;
     default:
	messout ("Invalid draw action %d when shifting",action) ;
      }

redraw:
  box->x1 = xbase ; box->x2 += dx ; box->y1 = ybase ; box->y2 += dy ;

#ifdef BETTER_BOX_SHIFT  /* Change parent box bounds, recursively */
  adjustParentBounds (box) ;
#else			 /* incorrect hack */
  box0 = gBoxGet(0) ;
  if (box->x1 < box0->x1) box0->x1 = box->x1 ;
  if (box->x2 > box0->x2) box0->x2 = box->x2 ;
  if (box->y1 < box0->y1) box0->y1 = box->y1 ;
  if (box->y2 > box0->y2) box0->y2 = box->y2 ;
#endif

  graphClipDraw (uToXabs(box->x1-dx)-1, uToYabs(box->y1-dy)-1, 
		 uToXabs(box->x2-dx)+1, uToYabs(box->y2-dy)+1) ;
  graphBoxDraw (kbox, -1, -1) ;
}



/* Use of the following routines is severely deprecated, they are just for   */
/* an incursion by sprdmap.c into graph internals. These routines will be    */
/* withdrawn when possible.                                                  */
void graphGetSprdmapData(int *h, float *uh, int *type, float *yfac)
  {
  *h = gActive->h ;
  *uh = gActive->uh ;
  *type = gActive->type ;
  *yfac = gActive->yFac ;

  return ;
  }

void graphSetSprdmapCoords(int h, float uh)
  {
  gActive->h = h ;
  gActive->uh = uh ;

  return ;
  }

void graphSetSprdmapType(int type)
  {
  gActive->type = type ;

  return ;
  }


/*                                                                           */
/*                                                                           */
/* Graph web browser calls.                                                  */
/*                                                                           */
/*                                                                           */

/* Display a link in a web browser and maybe control its display remotely    */
/* (possible for netscape, but for other browsers ?)                         */
BOOL graphWebBrowser(char *link)
{
  BOOL result = FALSE ;

  if (link && (strlen(link) > 0))
    {
      if (gDevFunc && gDevFunc->graphDevWebBrowser != NULL)
	result = gDevFunc->graphDevWebBrowser(link) ;
    }

  return result ;
} /* graphWebBrowser */


/***************************************************************************/

/*                                                                           */
/* The graph menu package routines.                                          */
/*                                                                           */
/*                                                                           */

MENU graphCBMenuPopup(GraphPtr graph, int x, int y)
/* Find the menu that is at this position on this graph and return
   it to the dev dependent code to display */
{
  Box box ;
  MENU menu = 0 ;

  graphActivate(graph->id) ;

  graphEventX = XtoUabs(x) ;
  graphEventY = YtoUabs(y) ; 
  
  for (menuBox = gActive->nbox ; --menuBox ;)
    { box = gBoxGet (menuBox) ;
      
      if (box->flag & GRAPH_BOX_NOMENU_FLAG)		    /* Exclude these boxes ala nopick. */
	continue ;
      
      if (graphEventX >= box->x1 && graphEventX <= box->x2 && 
	  graphEventY >= box->y1 && graphEventY <= box->y2)
	break ;
    }
  
  if (menuBox == 0)
    menu = gActive->menu0 ;	/* background menu */
  else 
    {
      if (box->flag & GRAPH_BOX_MENU_FLAG)
	assFind (gActive->boxMenus,
		 assVoid(4*menuBox), &menu) ; /* use box menu */
      else
	menu = gActive->menu0 ; /* no menu specified,
				   use the background menu */
    }
  return(menu) ;
}

void graphCBMenuSelect(MENUITEM item)
/* called when a menu item is selected */
{
  graphBusyCursorAll ();

  menuSelectItem (item);

  return ;
}

void graphNewBoxMenu (int k, MENU menu)
{
  Box box;
  union 
  { int i;
    void *v;
  } u;
  
  if (!gDev) return;

  box = gBoxGet (k) ;
  u.v = 0;
  u.i = 4*k;
  
  if (menu)
    { 
      if (k)
	{ if (!gActive->boxMenus)
	  gActive->boxMenus = assCreate() ;
	assRemove (gActive->boxMenus, u.v) ;
	assInsert (gActive->boxMenus, u.v, menu) ;
	}
      else
	gActive->menu0 = menu ;
    
      box->flag |= GRAPH_BOX_MENU_FLAG ;
    }
  else
    {
      if (!k)
	gActive->menu0 = 0 ;
      else if (gActive->boxMenus)
	{
	  BOOL isPreviousMenu = assRemove (gActive->boxMenus, u.v) ;
	  
	  if (isPreviousMenu)
	/* There was a menu attached to this box before, so we
	   have removed it and re-instate the original state
	   of the box, as if no menu was ever attached to it.
	   The box will return to the default menu behaviour.
	   The background menu will be used again for this box */
	    box->flag &= ~GRAPH_BOX_MENU_FLAG ;
	  else
	    /* There was NO menu attached to this box before.
	       This was probably caused by calling graphBoxMenu(box, 0)
	       on the box just after creating it. This box is therefor
	       marked to have no menu at all and will not behave
	       according to default menu behaviour (which displays the
	       background menu by default */
	    box->flag |= GRAPH_BOX_MENU_FLAG ;
	}
    }
}



void graphMenuBar(MENU menuBar)
{ 
  if (gDev && gDevFunc->graphDevMenuBar)
    gDevFunc->graphDevMenuBar(gDev, menuBar);
}

/*****************************************************************************/

/* Implementation of Freemenus in terms of new menus, Freemenus get converted
   into newmenus before they leave the device-independent layer, so we don't
   have to worry about them there */

static Associator freeMenus;
  
struct freemenubits 
{ 
  MENU newMenu;
  FREEOPT *opts;
  FreeMenuFunction proc;
};

static void freeMenuSelect (MENUITEM item)
{
  struct freemenubits *menu = menuGetPtr(item);
  (menu->proc)((KEY) menuGetValue(item), menuBox) ;
}

void graphBoxFreeMenu (int k, FreeMenuFunction proc, FREEOPT *options)
{
  struct freemenubits *menu;
  FREEOPT *o;
  int i;
  MENUITEM item ;

  if (!gDev)
    return ;
  if (!proc || !options)
    { graphNewBoxMenu(k, 0) ;
      return ;
    }

  if (!freeMenus)
    freeMenus = assCreate() ;

  if (assFind (freeMenus, options, &menu))
    { while (assFindNext(freeMenus, options, &menu))
	{ o = menu->opts;
	  if (menu->proc != proc) goto notfound;
	  if (o->key != options->key ) goto notfound;
	  for (i=1; i <= o->key; i++)
	    { if (o[i].key != options[i].key) goto notfound;
	      if ( 0 != strcmp(o[i].text, options[i].text)) goto notfound;
	    }
	  /* Found an existing menu here, resuse it */
	  graphNewBoxMenu (k, menu->newMenu);
	  return;
	notfound: 
	  continue;
	}
    }
  /*  make a new one here. */
  o = (FREEOPT *) messalloc(((options->key)+1)*sizeof(FREEOPT));

  for (i=0; i<= options->key; i++)
    { o[i].key = options[i].key;
      o[i].text = (char *) messalloc(1+strlen(options[i].text));
      strcpy(o[i].text, options[i].text);
    }
  menu = (struct freemenubits *) messalloc(sizeof(struct freemenubits));
  menu->opts = o;
  menu->proc = proc;
  menu->newMenu = menuCreate("");

  assMultipleInsert (freeMenus, options, menu) ;

  messalloccheck();
   
  for (i=1; i <= o->key; i++)
    { 
      item = menuCreateItem(o[i].text, freeMenuSelect);
      menuSetValue(item, o[i].key);
      menuSetPtr(item, menu); 
      if (o[i].key == 0 && (strcmp(o[i].text, "") == 0))
	menuSetFlags(item, MENUFLAG_SPACER); /* menu seperator */
      menuAddItem(menu->newMenu, item, 0);
    }
  graphNewBoxMenu (k, menu->newMenu) ;
}

/***************************************************************************/

/* Implementation of old (MENUOPT) menus in terms of new menus - this 
   means that the device-dependent layer doesn't have to worry about these */

static Associator oldMenus;

static void oldMenuSelect (MENUITEM item)
{
  MENUOPT *o = (MENUOPT*)menuGetPtr(item);
  ((MenuFunction)o->f)(menuBox) ;
}


void graphBoxMenu (int k, MENUOPT *options)
{
  struct menubits 
    { MENU newMenu;
      MENUOPT *opts;
    } *menu;
  MENUOPT *o;
  int i;
  MENUITEM item;

  if (!gDev)
    return ;

  if (!options)
    { graphNewBoxMenu(k, 0) ;
      return ;
    }

  if (!oldMenus)
    oldMenus = assCreate() ;

  if (assFind (oldMenus, options, &menu))
    { while (assFindNext(oldMenus, options, &menu))
	{ o = menu->opts;
	  for (i=0; o[i].f && options[i].f; i++)
	    if (o[i].f != options[i].f ||
		strcmp(o[i].text, options[i].text)) 
	      break ;
	  if (!o[i].f && !options[i].f)
	    {	  /* Found an existing menu here, resuse it */
	      graphNewBoxMenu (k, menu->newMenu);
	      return;
	    }
	  continue;
	}
    }

  /*  make a new one here. */

  for (i=0; options[i].f; i++);
  o = (MENUOPT *) messalloc((i+1)*sizeof(MENUOPT));

  for (i=0; options[i].f; i++)
    { o[i].f = options[i].f;
      o[i].text = (char *) messalloc(1+strlen(options[i].text));
      strcpy(o[i].text, options[i].text);
    }
  o[i].f =  0;
  o[i].text = 0;
  menu = (struct menubits *) messalloc(sizeof(struct menubits));
  menu->opts = o;

  menu->newMenu = menuCreate("");

  assMultipleInsert (oldMenus, options, menu) ;
  for (i=0; o[i].f; i++)
    { 
      item = menuCreateItem(o[i].text, oldMenuSelect);
      menuSetPtr(item, &o[i]);
      if (o[i].f == menuSpacer)
	menuSetFlags(item, MENUFLAG_SPACER); /* menu seperator */
      menuAddItem(menu->newMenu, item, 0);
    }
  
  graphNewBoxMenu (k, menu->newMenu) ;
}

/******** end of file ********/
 
 
 
 
 
