/*  File: drawdisp.c
 *  Author: Richard Durbin (rd@sanger.ac.uk)
 *  Copyright (C) J Thierry-Mieg and R Durbin, 1995
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
 * 	Richard Durbin (MRC LMB, UK) rd@sanger.ac.uk, and
 *	Jean Thierry-Mieg (CRBM du CNRS, France) mieg@kaa.cnrs-mop.fr
 *
 * Description: to provide postscript output for the graph package
 * Exported functions: graphPS(), graphPrint()
 * HISTORY:
 * Last edited: May  6 10:43 2003 (edgrif)
 * Created: Sat May 22 08:30:44 1995 (rd)
 * $Id: drawdisp.c,v 1.16 2003-05-06 13:13:41 edgrif Exp $
 *-------------------------------------------------------------------
 */

#include <wh/acedb.h>
#include <wh/display.h>
#include <wh/lex.h>
#include <wh/bs.h>
#include <whooks/systags.h>
#include <whooks/sysclass.h>

static int MAGIC = 28426847 ;

static BOOL isInitialised = FALSE ;

static KEY _Point, _Rectangle, _Circle, _Polygon, _Contains, _Draw ;
static KEY _Scale, _Bounds, _Shape, _Foreground, _Background ;

static void drawInitialise (void)
{
  lexaddkey ("Point", &_Point, 0) ;
  lexaddkey ("Draw", &_Draw, 0) ;
  lexaddkey ("Rectangle", &_Rectangle, 0) ;
  lexaddkey ("Polygon", &_Polygon, 0) ;
  lexaddkey ("Circle", &_Circle, 0) ;
  lexaddkey ("Contains", &_Contains, 0) ;
  lexaddkey ("Scale", &_Scale, 0) ;
  lexaddkey ("Bounds", &_Bounds, 0) ;
  lexaddkey ("Shape", &_Shape, 0) ;
  lexaddkey ("Foreground", &_Foreground, 0) ;
  lexaddkey ("Background", &_Background, 0) ;
}

/*************************/

typedef struct LOOKSTUFF
  { int   magic;		/* == MAGIC */
    KEY   key ;			/* for the Drawing object */
    Array segs ;		/* array of SEG's indexed by box */
    Array boxIndex ;		/* array of indices in segs */
    float scale ;		/* coord->pixel factor */
    float x0, y0 ;		/* in pixels */
    STORE_HANDLE handle ;
    int   activeBox ;
    char  imageFileName[256] ;
  } *LOOK ;

typedef struct
  { KEY key ;
    KEY tag ;			/* tag2 tag in Drawing obj */
    KEY fg, bg ;
  } SEG ;

LOOK lookGet (char *name)
{ 
  LOOK look ;
  if (!graphAssFind (&MAGIC, &look))
    messcrash ("graph not found in drawing%s()", name) ;
  if (look->magic != MAGIC)
    messcrash ("%s received a wrong pointer",name) ;
  return look ;
}

#define X2P(z) (((z)-look->x0)*look->scale)
#define Y2P(z) (((z)-look->y0)*look->scale)

/*************************/

static void drawDestroy (void) 
{ handleDestroy (lookGet("Destroy")->handle) ; }
static void drawPick (int box_unused, double x_unused, double y_unused, int modifier_unused) ;
static void drawDraw (void) ;
static BOOL drawConvert (LOOK look) ;

static MENUOPT drawMenu[] = {
  { graphDestroy, "Quit" }, 
  { graphPrint, "Print" }, 
  { help, "Help" },
  { graphBoxInfoWrite, "Box info" },
  { 0, 0 }
} ;

BOOL imageDisplay (KEY key, KEY from, BOOL isOldGraph, void *unused)
{ 
  LOOK look = 0 ;

  if (!isInitialised)
    drawInitialise () ;

  /* from here assume key support #Image model */

  /* ignore isOldGraph */

  {
    STORE_HANDLE handle = handleCreate() ;
    look = (LOOK) handleAlloc (0, handle, sizeof (struct LOOKSTUFF)) ;
    look->handle = handle ;
  }

  look->magic = MAGIC;
  look->key = key ;
  look->segs = arrayHandleCreate (32, SEG, look->handle) ;
  look->boxIndex = arrayHandleCreate (32, int, look->handle) ;

  displayCreate ("DtImage");

  graphRegister (DESTROY, drawDestroy) ;
  graphRegister (PICK, drawPick) ;
  graphMenu (drawMenu) ;
  graphAssociate (&MAGIC, look) ;

  if (!drawConvert (look))
    goto abort ;

  drawDraw () ;

  return TRUE ;

 abort:
  if (look)
    handleDestroy (look->handle) ;

  return FALSE ;
}

/**************************************/

static BOOL drawConvert (LOOK look)
{ 
  OBJ obj = bsCreate (look->key) ;
  float bx0, by0, bx1, by1 ;
  Array flat ;
  int i ;
  SEG *seg ;
  char *fileName = 0 ;

#define ABORT(z) { messout ("Can't draw %s: %s", name(look->key), z) ; \
		     return FALSE ; }

  if (!obj)
    ABORT("can't open object") ;
    
  if (!bsGetData (obj, _Scale, _Float, &look->scale))
    ABORT("no Scale") ;

  if (!(bsGetData (obj, _Bounds, _Float, &bx0) &&
	bsGetData (obj, _bsRight, _Float, &by0) &&
	bsGetData (obj, _bsRight, _Float, &bx1) &&
	bsGetData (obj, _bsRight, _Float, &by1)))
    ABORT("no Bounds") ;

  look->x0 = bx0 * look->scale ;
  look->y0 = by0 * look->scale ;
  graphPixelBounds (look->scale*(bx1-bx0), look->scale*(by1-by0)) ;

  arrayMax(look->segs) = 1 ;	/* don't use seg 0 */

  if (bsFindTag (obj, _File) && bsPushObj (obj) && 
      bsFindTag (obj, str2tag("GIF")) && 
      bsGetData (obj, str2tag("File_name"), _Text, &fileName))
    {	   
      strncpy (look->imageFileName, fileName, 255) ;
    }
  bsGoto (obj,0) ;
  flat = arrayCreate (128, BSunit) ;
  if (bsFindTag (obj, _Contains) && bsFlatten (obj, 2, flat))
    for (i = 0 ; i < arrayMax(flat) ; i += 2)
      { 
	if (class(arr(flat, i+1, BSunit).k)) 
	  {
	    seg = arrayp(look->segs, arrayMax(look->segs), SEG) ;
	    seg->tag = arr(flat, i, BSunit).k ;
	    seg->key = arr(flat, i+1, BSunit).k ;
	    bsGoto (obj, 0) ;
	    if (bsFindKey (obj, seg->tag, seg->key) && 
		bsPushObj (obj) &&
		bsFindTag (obj, _Foreground) &&
		bsGetKeyTags (obj, _bsRight, &seg->fg)) ;
	    else
	      seg->fg = BLACK ;
	    bsGoto (obj, 0) ;
	    
	    if (bsFindKey (obj, seg->tag, seg->key) && 
		bsPushObj (obj) &&
		bsFindTag (obj, _Background) &&
		bsGetKeyTags (obj, _bsRight, &seg->bg)) ;
	    else
	      seg->bg = TRANSPARENT ;
	    bsGoto (obj, 0) ;
	  }
      }

  return TRUE ;
}

/*************************************/

static void drawSelect (LOOK look, int box, BOOL on)
{ 
  int i = arr(look->boxIndex, box, int) ;
  SEG *seg = arrp (look->segs, i, SEG) ;

  if (!i)
    return ;
  if (on)
    graphBoxDraw (box, CYAN, seg->bg) ;
  else
    graphBoxDraw (box, seg->fg, seg->bg) ;
}

/*************************************/

static void drawPick (int box_unused, double x_unused, double y_unused, int modifier_unused)
{ 
  LOOK look = lookGet ("Pick") ;
  int i ;
  int box = graphBoxAt (graphEventX, graphEventY, 0, 0) ;

  if (box && box == look->activeBox && (i = arr(look->boxIndex, box, int)))
    display (arrp(look->segs,i,SEG)->key, look->key, 0) ;
  else
    { if (look->activeBox)
	drawSelect (look, look->activeBox, FALSE) ;
      look->activeBox = box ;
      if (look->activeBox)
	drawSelect (look, look->activeBox, TRUE) ;
    }
}

/*************************************/

static void drawDraw (void)
{ 
  LOOK look = lookGet ("Draw") ;
  int box, i ;
  SEG *seg ;
  OBJ obj ;
  KEY type ;
  static Array units = 0, coords = 0 ;
  float x0, x1, y0, y1 ;
  FILE *fil = 0 ;

  arrayMax(look->boxIndex) = 0 ;

  graphClear () ;
  graphPointsize (10.0) ;


  if (*look->imageFileName && (fil = filopen (look->imageFileName,"","r")))
    graphGIFread (fil, 0, 0) ;
  if (fil) filclose (fil) ;

  obj = bsCreate (look->key) ;
  if (obj)
    for (i = 1 ; i < arrayMax (look->segs) ; ++i)
      {
	seg = arrp(look->segs, i, SEG) ;
	bsGoto (obj, 0) ;
	if (!bsFindKey (obj, seg->tag, seg->key) || 
	    !bsPushObj (obj) ||
	    !bsFindTag (obj, _Shape) ||
	    !bsGetKeyTags (obj, _bsRight, &type))
	  continue ;
	
	box = 0 ;
	if (type == _Point)
	  { if (bsGetData (obj, _bsRight, _Float, &x0) &&
		bsGetData (obj, _bsRight, _Float, &y0))
	    { box = graphBoxStart() ;
	    graphPoint (X2P(x0), Y2P(y0)) ;
	    graphBoxEnd() ;
	    }
	  }
	else if (type == _Rectangle)
	  { if (bsGetData (obj, _bsRight, _Float, &x0) &&
		bsGetData (obj, _bsRight, _Float, &y0) &&
		bsGetData (obj, _bsRight, _Float, &x1) &&
		bsGetData (obj, _bsRight, _Float, &y1))
	    { box = graphBoxStart() ;
	    graphRectangle (X2P(x0), Y2P(y0), X2P(x1), Y2P(y1)) ;
	    graphBoxEnd() ;
	    }
	  }
	else if (type == _Circle)
	  { if (bsGetData (obj, _bsRight, _Float, &x0) &&
		bsGetData (obj, _bsRight, _Float, &y0) &&
		bsGetData (obj, _bsRight, _Float, &y1))
	    { box = graphBoxStart() ;
	    graphCircle (X2P(x0), Y2P(y0), Y2P(y1)) ;
	    graphBoxEnd() ;
	    graphBoxDraw(box, BLACK, TRANSPARENT) ;
	    }
	  }
	else if (type == _Polygon)
	  { units = arrayReCreate (units, 32, BSunit) ;
	  coords = arrayReCreate (coords, 32, float) ;
	  if (bsFlatten (obj, 2, units))
	    { int j ;
	    for (j = 0 ; j < arrayMax (units) ; j += 2)
	      { array(coords, j, float) = X2P(arr(units, j, BSunit).f) ;
	      array(coords, j+1, float) = Y2P(arr(units, j+1, BSunit).f) ;
	      }
	    box = graphBoxStart() ;
	    graphPolygon (coords) ;
	    graphBoxEnd() ;
	    graphBoxDraw(box, BLACK, TRANSPARENT) ;
	    }
	  }
	if (box)
	  { array(look->boxIndex, box, int) = i ;
	  graphBoxInfo (box, seg->key, 0) ;
	  graphBoxDraw (box, seg->fg, seg->bg) ;
	  }
	
      }
  
  bsDestroy (obj) ;
  look->activeBox = 0 ;
  graphRedraw () ;
}

/********************* end of file ******************/
