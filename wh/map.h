/*  File: map.h
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
 *	Jean Thierry-Mieg (CRBM du CNRS, France) mieg@crbm.cnrs-mop.fr
 *
 * Description: map.h - public interface for the mapcontrol.c package
 *              This is not to be confused with Simon Kelleys
 *              ColControl, which is seperate and mutually exlcusive
 *              with this one.
 *              This mapPackage is used for the fmap display
 * Exported functions:
 * HISTORY:
 * Last edited: Dec  3 16:17 2001 (edgrif)
 * * Jun 25 16:23 1999 (edgrif): Add state for 'preserve cols' option.
 * * Jul 23 14:53 1998 (edgrif): Remove redeclarations of fmap
 *     - modules must include fmap.h public header instead.
 * Created: Sat Nov 21 17:13:13 1992 (rd)
 * CVS info:   $Id: map.h,v 1.41 2001-12-03 19:17:20 edgrif Exp $
 *-------------------------------------------------------------------
 */
#ifndef _MAP_H
#define _MAP_H

#include <wh/acedb.h>
#include <wh/menu.h>

/************************************************************/

typedef struct LookStruct *LOOK ;			    /* opaque, cannot be accessed by map. */


/* All functions that are called by mapcontrol to redraw a column must have  */
/* this function type, they take a position which says where they must draw  */
/* the column, they return the end position of the column they have drawn.   */
/* They may also return a menu which can be included in the columns pull     */
/* down cascading menu for interactive column configuration.                 */
typedef void (*MapColDrawFunc)(LOOK look, float *pos_inout, MENU *menu_out) ;

/* Use these values for mapColSetByName() in preference to the raw ints, or  */
/* worse still, BOOL as are used in a number of places...                    */
/* Pedantically assign values because currently these are the values that    */
/* are scattered across the code.                                            */
typedef enum _MapColSet {MAPCOL_QUERY = -1,
			 MAPCOL_OFF = 0, MAPCOL_ON = 1,
			 MAPCOL_TOGGLE = 2} MapColSet ;

typedef struct _MapColRec *MapCol ;			    /* Private to map. */


typedef struct MapStruct {
  magic_t *magic ;					    /* == &MAP_MAGIC */
  Array cols ;						    /* Currently displayed cols. */
  Array preservedCols ;					    /* Cumulative list of all cols displayed
							       while reusing/preserving a display. */
  Array col_menus ;					    /* Array of column configuration menus. */

  MapCol butPressCol ;					    /* Which column was toggled ? */

  float min, max ;
  float mag, centre ;
  float fastScroll ;					    /* x coord of locator */
  int   *fixFrame ;
  char  *activeColName ;
  BOOL  flip ;						    /* To get the coordinates going upwards */
  struct 
    { int box ;
      float fac, offset, halfwidth, x ;
    } thumb ;
  struct
    { int box, pickBox ;
      int val ;
      float unit ;
      char text[32] ;
    } cursor ;
  VoidRoutine draw ;
  void (*drag)(float y);
  LOOK look ;						    /* for call back */
  KEYSET (*activeSet)(LOOK look) ;			    /* should return keySet(look->seg->key) */
  STORE_HANDLE handle ;
  BOOL isFrame ;					    /* for when showing 3 frames */ 
} *MAP ;


enum {INVALID_COLBUT = -1} ;

/************************************************************/
                     /* public mapPackage globals */

extern int mapGraphWidth, mapGraphHeight ;
extern float halfGraphHeight, topMargin ;

extern magic_t MAP2LOOK_ASSOC ;	/* find LOOK pointer for the map */

/************************************************************/

#define MAP2GRAPH(map,x) \
  (map->mag * ((x) - map->centre) + halfGraphHeight + topMargin)
#define GRAPH2MAP(map,x) \
  (((x) - halfGraphHeight - topMargin) / map->mag + map->centre)
#define MAP2WHOLE(map,x) \
  (3 + topMargin + ((x) - map->thumb.offset)*map->thumb.fac)
#define WHOLE2MAP(map,x) \
  (map->thumb.offset + ((x) - 3 - topMargin)/map->thumb.fac)

/************************************************************/

MAP currentMap(char *caller);	/* find MAP on graph */
MAP mapCreate (VoidRoutine draw) ; /* use mapInsertCol() to add cols now */
void uMapDestroy (MAP z) ;
#define mapDestroy(z) ((z) ? uMapDestroy(z), (z)=0, TRUE : FALSE)
void mapAttachToGraph (MAP map) ; /* needed normally after mapCreate() */

/* if name present already, return FALSE - don't change isOn
   else insert at priority level */
BOOL mapInsertCol (MAP map, float priority, BOOL init, char *name, 
		   MapColDrawFunc func) ;

BOOL mapColSetByName (MAP map, char *text, int isOn) ;
	/* BUT SEE the MapColSet typedef above for more readable alternatives...
	   isOn = -1: return present state, 
           	   0: set off, 
		   1: set on,
		   2: toggle 
	*/

Array mapColGetList (MAP map, STORE_HANDLE handle);
/* return an array of strings containing the names of
 * columns in the given map, the array and the strings in it
 * are allocated upon the given handle (NULL handle not allowed) */

void mapColCopySettings(MAP curr_map, MAP new_map) ;	    /* Copy across the cumulative column 
							       settings from one map to another. */
void mapColControl (void) ;
float mapDrawColumns (MAP map) ;
int mapDrawColumnButtons (MAP map);			    /* returns boxNum of first button */

BOOL mapColButPressed (MAP map) ;			    /* Was a col. button toggled ? */
void mapRedrawColBut (MAP map) ;			    /* Redraw the button toggled. */

void mapWhole (void) ;
void mapZoomIn (void) ;
void mapZoomOut (void) ;

void mapColMenu (int box) ;
void mapPrint (void) ;

void mapFindScaleUnit (MAP map, float *u, float *sub) ;
void mapShowLocator (LOOK look, float *offset, MENU *menu_out) ;
int  mapGetLocatorBox(MAP map) ;
void mapShowScale (LOOK look, float *offset, MENU *menu_out) ;
void mapThumbDrag (float *x, float *y, BOOL isDone) ;

void mapCursorCreate (MAP map, float unit, float x0) ;
void mapCursorSet (MAP map, float x) ;
void mapCursorDraw (MAP map, float x) ;
void mapCursorShift (MAP map) ;
void mapCursorDrag (float *x, float *y, BOOL isDone) ;
void mapCursorDrawLine (MAP map) ;

void mapNoMag (void) ;          /* prevents rescaling by horizontal mouse motion */
void mapMiddleDown (double x, double y) ;  /* callback for mouse middle down */
#define mapCursorPos(map) ((map)->cursor.val * (map)->cursor.unit)

/*****************************/

#ifdef CODE_NEVER_USED
 /* returns the objects in the active map */
BOOL mapActive(KEYSET *setp, void** mapp)  ;
#endif /* CODE_NEVER_USED */

#endif  /* _MAP_H */

/*************************** eof ****************************/
