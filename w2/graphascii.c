/*  File: graphascii.c
 *  Author: Richard Durbin (rd@mrc-lmb.cam.ac.uk)
 *  Copyright (C) J Thierry-Mieg and R Durbin, 1992
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
 * Description: to provide postscript output for the graph package
 * Exported functions: graphASCII(), graphPrint()
 * HISTORY:
 * Last edited: Jun 19 15:31 2002 (edgrif)
 * * Aug 14 18:05 1992 (rd): COLOR_SQUARES
 * * Jul 25 12:28 1992 (mieg): TEXT_PTR_PTR
 * Created: Wed May 20 08:30:44 1992 (rd)
 *-------------------------------------------------------------------
 */

/*  $Id: graphascii.c,v 1.22 2002-06-20 10:39:22 edgrif Exp $ */

#include "regular.h"

#include "bump.h"
#include "bump_.h"	/* intrude into private BUMP-package - YUCK */

#include <w2/graph_.h>

static FILE *fil ;

typedef struct { int x; float y ; char *cp; } ASCII_CELL ;
static int ncell ;
static Array cells = 0 ;
static int nbLines ;
/***********************************************/
static int asciiCellOrder(void *a, void *b)
{ int x1 = ((ASCII_CELL*)a)->x , x2 = ((ASCII_CELL*)b)->x ;
  float y1 = ((ASCII_CELL*)a)->y , y2 = ((ASCII_CELL*)b)->y ;
  
  if (y1 < y2)
    return -1 ;
  else if (y1 > y2)
    return 1 ;
  else
    return (x1 - x2) ;
}
/***********************/

static void asciiBump(void)
{
  int lenOld, xOld = -1 ;
  float yOld = -1 ;
  BUMP  bump = bumpCreate(80, 0) ; 
  int n = arrayMax(cells) , gap = 0 ;
  float old ;
  ASCII_CELL *c = arrp(cells,0, ASCII_CELL) - 1 ;

  bump->yAscii = 0 ;
  bump->xGapAscii = 0 ;
  while(c++, n--)
    if (c->cp && *c->cp)
      { if (c->y != yOld)
	  { bump->xGapAscii = 0 ;
	    bump->xAscii = 0 ;
	  }
      else
	{ bump->xGapAscii = c->x - lenOld - xOld ; 
	}
	yOld = c->y ;
	xOld = c->x ;
	lenOld = strlen (c->cp) ;
	old = c->y += gap ;

/* mhmp 05/05/97
   bumpItem(bump, strlen(c->cp) + 1 , 1, &(c->x), &(c->y)) ; */

	asciiBumpItem(bump, strlen(c->cp) , 1, &(c->x), &(c->y)) ;
	gap += c->y - old ;
      }
  bumpDestroy(bump) ;
  nbLines = yOld + gap ;
}

/***********************/

static void asciiDump(void)
{ char space[301] ;
  int i,  n = arrayMax(cells) , oldx = 0, oldy = 0 , x, y ;
  char *cp ;
  ASCII_CELL* c = arrp(cells, 0, ASCII_CELL) - 1 ;

  memset(space, ' ', (mysize_t)300) ;
  space[300] = 0 ;
  while (c++, n--)
    if ((cp = c->cp) && *cp)
      { x = c->x ; y = c->y ;
	
	i = ( y == oldy) ? x - oldx : x ;
	if (i>299)
	  i = 299 ;
	if (i<0)
	  i = 0 ;
	
	for (;oldy<y; oldy++)
	  fputc('\n', fil) ;
	fprintf(fil, space + 300 - i) ;
	fprintf(fil, "%s",cp) ; /* avoid bug if cp contains a % , mhmp 02.04.98 */
	oldx = x + strlen(cp) ;
      }      
}

/***********************************************/

static void drawBox (Box box)
{
  int    n ;
  float  x, y ;
  int    action ;
  char	 *cp ;
  Box	 nextbox ;
  ASCII_CELL* cellp ;

  stackCursor (gStk,box->mark) ;

  while (!stackAtEnd (gStk))
    switch (action = stackNext (gStk, int))
      {
      case BOX_END:
        return ;                        /* exit point */
      case BOX_START:
        n = stackNext (gStk, int) ;
	nextbox = gBoxGet (n) ;
	drawBox (nextbox) ;                  /* recursion */
	break ;
      case COLOR:
      case TEXT_FORMAT:
	n = stackNext (gStk,int) ;
        break ;
      case TEXT_HEIGHT:
      case LINE_WIDTH:
      case POINT_SIZE:
        x = stackNext (gStk,float) ;
        break ;
      case LINE_STYLE:
	n = stackNext (gStk,int) ;
        break ;
      case LINE: case RECTANGLE: case FILL_RECTANGLE:
        x = stackNext (gStk,float) ;
        x = stackNext (gStk,float) ;
        x = stackNext (gStk,float) ;
        x = stackNext (gStk,float) ;
	break ;
      case PIXELS: case PIXELS_RAW:
	x = stackNext (gStk,float) ;
	x = stackNext (gStk,float) ;
	cp = stackNext (gStk,char*) ;
	n = stackNext (gStk, int) ;
	n = stackNext (gStk, int) ;
	n = stackNext (gStk, int) ;
	if (action == PIXELS)
	  { x = stackNext (gStk,float) ;
	    x = stackNext (gStk,float) ;
	  }
	break ;
      case POLYGON: case LINE_SEGS:
	n = stackNext (gStk, int) ;
	if (n > 2)
	  { while (n--)
	      { x = stackNext (gStk, float) ;
		x = stackNext (gStk, float) ;
	      } 
	  }
	break ;
      case CIRCLE: case POINT: case TEXT: case TEXT_UP: case FILL_ARC:
      case TEXT_PTR: case TEXT_PTR_PTR: case COLOR_SQUARES:
        x = stackNext (gStk,float) ;
        y = stackNext (gStk,float) ;
        switch (action)

          {
          case CIRCLE:
	    x = stackNext (gStk,float) ;
            break ;
          case FILL_ARC:
	    x = stackNext (gStk,float) ;
	    x = stackNext (gStk,float) ;
	    x = stackNext (gStk,float) ;
            break ;
          case POINT:
	    break ;
          case TEXT: case TEXT_UP:
	    cp = stackNextText(gStk) ;
	    cellp = arrayp( cells, ncell++, ASCII_CELL) ;
	    cellp->x = x / UtextX ; /* mhmp 03.10.97 */
	    cellp->y = y / UtextY ;
	    cellp->cp = cp ;
	    break ;
	  case TEXT_PTR:
	    cp = stackNext(gStk,char*) ;
	    cellp = arrayp( cells, ncell++, ASCII_CELL) ;
	    cellp->x = x / UtextX ;  /* mhmp 03.10.97 */
	    cellp->y = y / UtextY ;
	    cellp->cp = cp ;
	    break ;
	  case TEXT_PTR_PTR:
            cp = *stackNext(gStk,char**) ;
	    if (cp && *cp)
	      { cellp = arrayp( cells, ncell++, ASCII_CELL) ;
		cellp->x = x / UtextX ; /* mhmp 03.10.97 */
		cellp->y = y / UtextY ;
		cellp->cp = cp ;
	      }
	    break ;
	  case COLOR_SQUARES:
	    cp = stackNext (gStk, char*) ; /* colors */
	    n = stackNext (gStk, int) ;	/* len */
	    n = stackNext (gStk, int) ;	/* skip */
	    cp = (char*) stackNext (gStk,int*) ; /* tints */
	    break ;
          }
	break ;
      default:
	messout ("Invalid action %d received in drawASCIIBox",action) ;
	break ;
      }
}

static void graphASCIIBox (int k, int fcol, int bcol)
{
  Box box = gBoxGet (k) ;

  drawBox (box) ;

  return ;
}

void graphASCII (char *myfilname, char *mail, char *print, char *title)
{
  Stack pc = 0 ;
  char *localfilname ;
  int pages ;
  int limit = 1000 ;
  ncell = 0 ;
  cells = arrayReCreate (cells, 100, ASCII_CELL) ;

  if (myfilname && *myfilname)
    { localfilname = myfilname ;
    }
  else
    { if (!(localfilname = tempnam ("/var/tmp", "AcePr")))
	{ messout ("failed to create temporary file name") ;
	  return ;
	}
    }
  if (!(fil = fopen (localfilname,"w")))
    { messout ("failed to open ascii file %s", localfilname) ;
      return ;
    }


  switch (gActive->type)
    { 
    case TEXT_SCROLL: 
    case TEXT_FULL_SCROLL:
    case TEXT_HSCROLL:
    case MAP_SCROLL:
    case PLAIN: case TEXT_FIT:
    case PIXEL_SCROLL: case PIXEL_FIT: 
    case PIXEL_VSCROLL: case PIXEL_HSCROLL:
      pages = 1 ;
      break ;
    default:
      messout ("Unknown graph type in graphASCII") ;
      filclose (fil) ;
      return ;
    }

  graphASCIIBox (0,-1,-1) ;
  arraySort(cells, asciiCellOrder) ;
  asciiBump() ;
  asciiDump() ;

  filclose (fil) ;
  arrayDestroy(cells) ;
  if (nbLines > limit)
    if (!messQuery
	(messprintf("You are going to write %d lines, do you want to continue ?", 
		    nbLines)))
      goto fin ;
  if (print && *print)
    { pc = stackCreate(100) ;
      pushText (pc, print) ;
      catText (pc, " ") ;
      catText (pc, localfilname) ;
      if (system (stackText(pc, 0)))
	messout ("Sorry, for some reason print command %s failed", 
		 stackText(pc, 0)) ;
      stackDestroy (pc) ;
    }
  if (!*title) title = "acedb_mail" ;

  if (mail && *mail)
    if (system (messprintf ("Mail -s \"%s\" %s < %s", 
			    title, mail, localfilname)))
      messout (
   "Sorry, for some reason mail command failed \n Mail -s \"%s\" %s < %s", 
			   title, mail, localfilname) ;
 fin:
  if (!myfilname || !*myfilname) /* no Copy reqired */
    filremove (localfilname,"") ;
}
/***** end of file *****/
