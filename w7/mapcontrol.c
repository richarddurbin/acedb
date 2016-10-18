/*  File: mapcontrol.c
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
 * Description: package for generic map drawing containing:
 *		columns control
 *		zoom, middle button scroll, locator
 *		findScaleUnits
 * Exported functions:
 * HISTORY:
 * Last edited: Oct 15 09:05 2007 (edgrif)
 * * Apr 20 12:07 2000 (edgrif): Add column configuration pulldown menus.
 * * Mar 30 15:45 2000 (edgrif): Added code for "no redraw" option for
 *              column toggling.
 * * Jun 25 10:03 1999 (edgrif): mapColCopySettings - copies col settings
 *              cumulatively from one map to another.
 * * Apr 27 13:05 1999 (fw): mapColGetList to get at valid column names
 * * Apr 27 13:05 1999 (fw): mapColSetByName now case-insensitive
 * * (mieg) zoom_set_g at least should be part of the map structure
 *          anyway i disable 
 * * Jul 23 14:51 1998 (edgrif): Add fmap.h public header for function defs
 *      removed from map.h
 * * Dec 24 1997 (rd): made MapColRec local.  Must use mapInsertCol().
 * * Mar 12 18:29 1995 (rd): added MapColRec2 for dymanic specs
 *	based on priorities, and changed to copy cols, not use 
 *	colSwitch.
 * Created: Wed Nov 18 00:40:35 1992 (rd)
 * CVS info:   $Id: mapcontrol.c,v 1.75 2012-04-12 15:21:38 edgrif Exp $
 *-------------------------------------------------------------------
 */
#include "acedb.h"
#include "aceio.h"
#include <wh/display.h>
#include "map.h"					    /* declare opaque LookStruct */
							    /* to be completed by users 
							       of the mapPackage */
#include "fmap.h"					    /* public fmap function headers */



#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
extern float col_Priority_G ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */



/* Use this prefix in all mapcontrol error messages.                         */
#define MAP_PREFIX "MapControl -  "


#define MAP_NAME_BUFLEN 100				    /* buffer size for column title */

/* Main structure for holding state about each column.                       */
/* N.B. by having an array for the name we can use straight forward array    */
/* copies and avoid a lot of string pointer messing about.                   */
typedef struct _MapColRec
{
  BOOL   isOn ;						    /* Is the column displayed ? */
  char   name[MAP_NAME_BUFLEN] ;			    /* String title of column */
  MapColDrawFunc func;					    /* Function for drawing column */
  float  priority ;					    /* Position of column on screen */
  int box ;						    /* box number of button for this column. */
} MapColRec ;


/************************************************************/
/* public mapPackage globals   LOOK OUT....                             */

int mapGraphWidth, mapGraphHeight; /* globals, only valid while drawing */
float topMargin, halfGraphHeight ;

magic_t MAP2LOOK_ASSOC = "MAP2LOOK" ;	/* find LOOK pointer for the map */


/************************************************************/

static MENU mapCreateColumnDropdown(float top, float left_end_offset, float right_end_offset) ;
static void mapThumbCalc (MAP map) ;

/************************************************************/

static magic_t GRAPH2MAP_ASSOC = "GRAPH2MAP"; /* find the MAP on gActive */
static magic_t MAP_MAGIC = "MAP_MAGIC";       /* verify MAP-pointer */

static BOOL isMagPossible = TRUE ;
static MAP selectedMap = 0 ;

#define NAME_SPACE 20


static MENUOPT simpleMenu[] = {
  { graphDestroy,	"Quit" },
  { help,		"Help" },
  { 0, 0 }
} ;


/* If method Right_priority values are less than this apart then the features
 * with those methods are drawn in the same column. Note that this priority
 * can be changed in displays.wrm using the -overlap flag in the _DFMAP stanza. */
static float mapcol_priority_g = 0.0 ;



/************************************************************/


MAP mapCreate (VoidRoutine draw)
{
  MAP map ;

  /* Set any global properties, e.g. priority may have been set in displays.c */
  mapcol_priority_g = displayGetOverlapThreshold() ;


  /* Create the map. */
  map = (MAP) messalloc (sizeof (struct MapStruct)) ;

  map->handle = handleCreate ();
  map->magic = &MAP_MAGIC ;
  map->draw = draw ;
  map->drag = NULL;

  map->cols = arrayHandleCreate(32, MapColRec, map->handle) ;

  map->preservedCols = NULL ;

  /* Important that this array is empty otherwise mapDrawColumns will fail.  */
  map->col_menus = arrayHandleCreate(32, MENU, map->handle) ;

  map->butPressCol = NULL ;

  selectedMap = map ;

  return map ;
} /* mapCreate */



MAP currentMap(char *caller)
{
  MAP map;

  if (!(graphAssFind(&GRAPH2MAP_ASSOC, &map)))
    messcrash(MAP_PREFIX "%s() could not find MAP on graph.", caller);
  if (!map)
    messcrash(MAP_PREFIX "%s() received NULL MAP pointer.", caller);
  if (map->magic != &MAP_MAGIC)
    messcrash(MAP_PREFIX "%s() received non-magic MAP pointer.", caller);

  return map;
} /* currentMap */


void mapAttachToGraph (MAP map)
{
  graphAssociate (&GRAPH2MAP_ASSOC, map) ;
  graphRegister (MIDDLE_DOWN, mapMiddleDown) ;

  return;
} /* mapAttachToGraph */


void uMapDestroy (MAP map)
{
  if (map->magic != &MAP_MAGIC)
    messcrash(MAP_PREFIX "uMapDestroy received mon-magic MAP pointer");
  map->magic = 0 ;
  graphAssRemove (&GRAPH2MAP_ASSOC) ;
  handleDestroy (map->handle) ;
  if (map->preservedCols != NULL)
    arrayDestroy(map->preservedCols) ;
  messfree (map);

  return ;
} /* uMapDestroy */



/**********************************************************/
/************* code to manipulate columns.    *************/

static int mapColOrder (void *aa, void *bb)
{
  MapColRec *a = (MapColRec*)aa, *b = (MapColRec*)bb ;
  if (a->priority > b->priority) return 1 ;
  else if (a->priority < b->priority) return -1 ;
  else return strcmp (a->name, b->name) ;
} /* mapColOrder */


BOOL mapInsertCol (MAP map, float priority, BOOL isOn, char *name, 
		   MapColDrawFunc func)
{
  int i ;
  MapColRec r ;

  if (!map)
    return FALSE ;

  for (i = 0 ; i < arrayMax(map->cols) ; ++i)
    if (strcmp (name, arrp(map->cols, i, MapColRec)->name) == 0)
      /* there already */
      return FALSE ;

  r.priority = priority ;
  r.isOn = isOn ;
  r.name[0] = '\0' ;
  if (strlen(name) >= MAP_NAME_BUFLEN)
    messcrash(MAP_PREFIX "internal buffer is too small (%d) to hold the column name \"%s\".",
	      MAP_NAME_BUFLEN, name) ;
  if (strcpy(r.name, name) == NULL)
    messcrash(MAP_PREFIX "simple copy of column name \"%s\" failed.", name) ;
  r.func = func ;
  arrayInsert (map->cols, &r, mapColOrder) ;

  return TRUE ;
} /* mapInsertCol */

BOOL mapColSetByName (MAP map, char *text, int isOn)
{
  int i ;
  MapColRec *col ;

  col = arrp(map->cols,0,MapColRec) ;
  for (i = 0 ; i < arrayMax(map->cols) ; ++i, ++col)
    if (strcasecmp (col->name, text) == 0)
      { if (isOn == 2)   /* toggle */
	  col->isOn ^= TRUE ;
        else if (isOn == -1)   /* query */
	  return col->isOn ;
        else if (isOn)
	  col->isOn = TRUE ;
        else
	  col->isOn = FALSE ;
        return TRUE ;
      }

  return FALSE ;		/* not found */
} /* mapColSetByName */


/* Called from column menu which is displayed when user right clicks over    */
/* "Columns" button, toggles on/off the display of individual columns.       */
/*                                                                           */
void mapColToggle (KEY key, int box)
{
  MAP map = currentMap("mapColToggle") ;

  arr(map->cols,key,MapColRec).isOn ^= TRUE ;

  /* Record which column was toggled, potentially needed by the draw routine */
  /* called after this.                                                      */
  map->butPressCol = &(arr(map->cols,key,MapColRec)) ;

  (map->draw)() ;

  map->butPressCol = NULL ;				    /* make sure we reset. */

  return;
} /* mapColToggle */


Array mapColGetList (MAP map, STORE_HANDLE handle)
     /* return an array of strings containing the names of
      * columns in the given map, the array and the strings in it
      * are allocated upon the given handle */
{
  int i ;
  MapColRec *col ;
  Array colList;

  if (!handle)
    /* warn so the array and the strings can be cleaned up
     * on the handle, if people just call arrayDestroy on the
     * array the strings are left behind, that's why we need the handle */
    messcrash(MAP_PREFIX "mapColGetList() received NULL handle, this practise "
	      "may leave memory leaks. Pass in a valid handle...");

  colList = arrayHandleCreate(arrayMax(map->cols), char*, handle);

  col = arrp(map->cols,0,MapColRec) ;
  for (i = 0 ; i < arrayMax(map->cols) ; ++i, ++col)
    {
      array(colList, arrayMax(colList), char*) = 
	strnew(col->name, handle);
      }

  return colList;
} /* mapColGetList */


/* Some displays (e.g. fmap) allow the user to keep their selected column    */
/* settings while reusing the display, this function copies across the       */
/* settings of one map to another. NOTE that this is done cumulatively, so   */
/* we incorporate any columns that were recorded as 'preserved' in the map   */
/* we are copying settings from. This is important because otherwise the     */
/* settings are only as good as the last map displayed, which by chance may  */
/* have had very few columns.                                                */
void mapColCopySettings(MAP curr_map, MAP new_map)
{
  int i ;

  /* Destroy existing settings so they will be completely replaced.          */
  /* (will only happen if this function is called multiple times for the     */
  /*  same new_map)                                                          */
  if (new_map->preservedCols != NULL)
    arrayDestroy(new_map->preservedCols) ;

  /* If columns have not yet been preserved, copy the current map cols,      */
  /* otherwise copy the current preserved columns and overlay them with the  */
  /* current map cols.                                                       */
  if (curr_map->preservedCols == NULL)
    {
      new_map->preservedCols = arrayCopy(curr_map->cols) ;
    }
  else
    {
      new_map->preservedCols = arrayCopy(curr_map->preservedCols) ;

      for(i = 0 ; i < arrayMax(curr_map->cols) ; i++)
	{
	  MapColRec *col = arrp(curr_map->cols, i, MapColRec) ;
	  BOOL found = FALSE ;
	  int j ;

	  for (j = 0 ; j < arrayMax(new_map->preservedCols) && found == FALSE ; ++j)
	    {
	      if (strcmp(col->name, arrp(new_map->preservedCols, j, MapColRec)->name) == 0)
		{
		  found = TRUE ;
		  arrp(new_map->preservedCols, j, MapColRec)->isOn = col->isOn ;
		}
	    }
	}
    }

  /* Store the preserved settings in the new map columns.                */
  for(i = 0 ; i < arrayMax(new_map->preservedCols) ; i++)
    {
      MapColRec *col = arrp(new_map->preservedCols, i, MapColRec) ;

      mapColSetByName(new_map, col->name, col->isOn) ;
    }

  /* Now add any new columns from the new map to the new preserved settings. */
  for (i = 0 ; i < arrayMax(new_map->cols) ; i++)
    {
      MapColRec *col = arrp(new_map->cols, i, MapColRec) ;
      BOOL found = FALSE ;
      int j ;
  
      for (j = 0 ; j < arrayMax(new_map->preservedCols) && found == FALSE ; ++j)
	{
	  if (strcmp(col->name, arrp(new_map->preservedCols, j, MapColRec)->name) == 0)
	    found = TRUE ;
	}

      if (found == FALSE)
	arrayInsert(new_map->preservedCols, col, mapColOrder) ;
    }

  return ;
}


/****************************************************************
 * Function to build a menu of map-columns of the
 * map associated with the active graph. This menu
 * will be attached to the box with the given number
 * (Taken that we've just drawn a button with that box-number)
 ****************************************************************/
void mapColMenu (int box)
{
  static Associator cols2options = 0 ;
  int n;
  FREEOPT *options ;
  MAP map = currentMap("mapColMenu") ;

  if (!cols2options)
    cols2options = assCreate () ;
  if (assFind (cols2options, map->cols, &options) &&
      arrayMax(map->cols) == options->key)
    { graphBoxFreeMenu (box, mapColToggle, options) ;
      return ;
    }

  options = (FREEOPT*) messalloc ((arrayMax(map->cols)+1) * sizeof(FREEOPT)) ; 
  options->key = arrayMax(map->cols) ;
  options->text = "Columns" ;

  for (n = 0 ; n < arrayMax(map->cols) ; ++n)
    { options[n+1].text = arrp(map->cols,n,MapColRec)->name ;
      options[n+1].key = n ;
    }

  graphBoxFreeMenu (box, mapColToggle, options) ;
  assInsert (cols2options, map->cols, options) ;

  return;
} /* mapColMenu */

/************************************************************/

static void colPick (int i, double x_unused, double y_unused, int modifier_unused)
{
  int old ; 
  MAP map = currentMap("colPick") ;

  selectedMap = map ;
  if (i < 2)
    return ;

  old = arrp(map->cols,i-2,MapColRec)->isOn ; /* background and button */

  if (old)
    graphBoxDraw (i, BLACK, WHITE) ;
  else
    graphBoxDraw (i, WHITE, BLACK) ;
  arrp(map->cols,i-2,MapColRec)->isOn = old ? FALSE : TRUE;

  return;
} /* colPick */

/***********************************/

void mapColControl (void)	/* menu function */
{
  int i, c, nx, ny, x, y, box ;
  MapColRec *col ;
  char *cp, *cq, old ;
  MAP map = currentMap("mapColControl") ;

  col = arrp(map->cols,0,MapColRec) ;

  graphClear () ;
  graphButton ("Return to display", map->draw, 5, 1) ;
  graphText ("Display options:", 1, 3) ;

  graphFitBounds (&nx, &ny) ;
  nx /= NAME_SPACE ; if (!nx) nx = 1 ;
  nx = 1 ; /* mieg hard coded, to get the form of the display fixed */
  x = 2 ; y = 4 ;
  for (c = 0, i = 0 ; c < arrayMax(map->cols) ; ++c, ++col)
    { box = graphBoxStart() ;
      cp = cq = col->name ;
      while (*cq && *cq != ':') cq++ ;
      old = *cq ; if (old) *cq = 0 ;/* mieg, segmentation fault on solaris */
      graphText (cp, x, y) ;
      graphBoxEnd() ;
      if (old) *cq = old ; /* mieg, segmentation fault on solaris */
      if (old)
	graphText (cq, x+ strlen(cp) + .4 , y) ;
      
      if (col->isOn)
	graphBoxDraw (box, BLACK, LIGHTBLUE) ;
      else
	graphBoxDraw (box, BLACK, WHITE) ;
      if ((++i) % nx)		/* increment i here */
	x += NAME_SPACE ;
      else
	{ x = 2 ;
	  ++y ;
	}
    }
  graphRegister (PICK, colPick) ;
  graphMenu (simpleMenu) ;
  graphRedraw () ;
  
  return;
} /* mapColControl */


/************************************/

/* Is called by fmapDraw to arrange the drawing of the fmap columns. Also
 * draws the pulldown column configuration menus for columns that can be
 * configured.
 * Returns the offset of the _end_ of the columns.
 */
float mapDrawColumns (MAP map)	/* topMargin must be set */
{
  float offset = 0 ;
  float oldOffset = 0 ;
  float maxOffset = 0 ;
  float oldPriority = -1000000 ;
  float overlap_threshold ;
  MapColRec *col;
  int i, frameCol, frame = 0 ;
  LOOK look ;
  BOOL dropdown_drawn ;


  /* Destroy the previous column configuration menu, user may have switched  */
  /* columns off, so we need to rebuild. Note, its only the top level menus  */
  /* that get destroyed, not the columns individual menus.                   */
  for (i = 0 ; i < arrayMax(map->col_menus) ; i++)
    {
      menuDestroy((MENU)arrp(map->col_menus, i, MENU)) ;
    }
  arrayReCreate(map->col_menus, 32, MENU) ;		    /* Empty the array. */


  /* What size is the fmap ?                                                 */
  graphFitBounds (&mapGraphWidth, &mapGraphHeight) ;
  halfGraphHeight = 0.5 * (mapGraphHeight - topMargin) ;

  if (/*!(look = fMapGifLook) &&*/
      !graphAssFind (&MAP2LOOK_ASSOC, &look))
    messcrash(MAP_PREFIX "mapDrawColumns() - Can't find look for this map.\n"
	      "The programmer may have forgotten to associate "
	      "MAP2LOOK_ASSOC with the look on this graph.") ;

  selectedMap = map ;
  mapThumbCalc (map) ;
  map->cursor.box = 0 ;
  map->thumb.box = 0 ;
  map->thumb.x = 0 ;

  arraySort (map->cols, mapColOrder) ;

  col = arrp (map->cols, 0, MapColRec) ;

  frameCol = 0 ;
  map->isFrame = FALSE ;
  dropdown_drawn = FALSE ;
  for (i = 0 ; i < arrayMax(map->cols) ; ++i) 
    {
      /* check start of frame-sensitive region */
      if (col[i].isOn && map->fixFrame && !map->isFrame && fmapIsFrameFunc(col[i].func))
	{
	  frameCol = i ;
	  map->isFrame = TRUE ;
	  oldPriority = -1000000 ; /* so no overlap */
	}

      /* check end of frame-sensitive region */
      if (map->isFrame && fmapIsGeneTranslationFunc(col[i].func))
	{
	  if (frameCol && frame < 2)
	    {
	      ++(*map->fixFrame) ;
	      ++frame ;
	      i = frameCol ;	/* loop */
	    }
	  else
	    {
	      *map->fixFrame -= 2 ; /* restore original value */
	      map->isFrame = FALSE ;
	    }
	  oldPriority = -1000000 ; /* so no overlap */
	}

      /* decide whether or not to overlap */
      overlap_threshold = mapcol_priority_g ;
      if (col[i].priority < oldPriority + overlap_threshold)
	{
	  offset = oldOffset ;
	}
      else
	{
	  oldOffset = offset ;
	  dropdown_drawn = FALSE ;			    /* New column position. */
	}

      /* draw the column if its currently turned on. */
      if (col[i].isOn)
	{ 
	  float drop_begin, drop_end ;
	  MENU col_dropdown ;
	  MENU col_menu ;
	  MENUITEM menu_item;
	  BOOL col_has_menu = FALSE ;

	  map->activeColName = col[i].name ;


	  /* Call the function to do the column drawing.                     */
	  /* Note that "offset" before the call is where the column starts,  */
	  /* "offset" after the call has been set to where it ends.          */
	  /* The function will also return a column configuration menu if    */
	  /* it supports configuration.                                      */
	  col_menu = NULL ;
	  drop_begin = offset ;
	  if (col[i].func)
	    (*col[i].func)(look, &offset, &col_menu) ;
	  drop_end = offset ;


	  /* I think this function should go, or col_menu should be put in   */
	  /* to the mapColRec, as it is we have two ways of checking for menu*/
	  /* either if col_menu != NULL or by the result of this call...not  */
	  /* good...                                                         */
	  /* I guess we need to ask "fmap" if we are a column that can have  */
	  /* a menu, this is partly because "methods" do not have some kind  */
	  /* of flag to say what type they are.                              */
	  col_has_menu = fmapColHasMenu(col[i].func) ;


	  /* We at a new column position, so if we haven't yet drawn a pull- */
	  /* down and this column has a menu, then create the pulldown.      */
	  /* Note that we must do this AFTER the first column has been       */
	  /* added at this position otherwise we don't know what width to    */
	  /* make the pulldown.                                              */
	  if (col_has_menu && dropdown_drawn == FALSE)
	    {
	      col_dropdown
		= array(map->col_menus, arrayMax(map->col_menus), MENU)
		= mapCreateColumnDropdown(topMargin, drop_begin, drop_end) ;
	      dropdown_drawn = TRUE ;
	    }

	  /* Add the menu returned by the column drawing routine to the pull-*/
	  /* down menu.                                                      */
	  if (col_has_menu)
	    {
	      menu_item = menuCreateItem(col[i].name, NULL) ;

	      if (!menuSetMenu(menu_item, col_menu)
		  || !menuAddItem(col_dropdown, menu_item, NULL))
		messcrash("could not create menu item for column pull down menu, "
			  "column was: %s", col[i].name) ;
	    }

	  if (offset < maxOffset)
	    offset = maxOffset ;
	  else
	    maxOffset = offset ;
	}

      oldPriority = col[i].priority ;
    }

  return maxOffset;

} /* mapDrawColumns */


/********************/
/* Draw the drop down menu at the top of configurable columns.               */
/*                                                                           */
static MENU mapCreateColumnDropdown(float top, float left_end_offset, float right_end_offset)
{
  int menu_box ;
  float x, y ;
  int old_colour ;
  MENU menu;
  MENUITEM menu_item ;

  /* Create the menu */
  menu = menuCreate("Column configuration");

  menu_item = menuCreateItem("", NULL) ;		    /* Add a spacer. */
  menuSetFlags(menu_item, MENUFLAG_SPACER) ;
  menuAddItem(menu, menu_item, NULL) ;

  
  x = left_end_offset ;
  y = top + 0.2 ;

  /* draw a shaded area across the width of the column */
  old_colour = graphColor(GRAY);
  graphFillRectangle(x + 0.05, y, right_end_offset - 0.05, y + 1);
  graphColor(old_colour);

  /* menu box */
  menu_box = graphMenuTriangle (FALSE, x+0.5, y);
  graphNewBoxMenu (menu_box, menu);
  graphBoxSetPick (menu_box, FALSE); /* shouldn't be selectable by fMapPick */

  return menu ;
}


/******************************************************/
/************ zoom and display stuff ******************/ 

void mapWhole (void)		/* menu function */
{ 
  BOOL isNeg ;
  MAP map = currentMap("mapWhole") ; 

  selectedMap = map ;
  graphFitBounds (&mapGraphWidth, &mapGraphHeight) ;

  halfGraphHeight = 0.5 * (mapGraphHeight - topMargin) ;

  map->centre = (map->max + map->min) / 2 ;  

  isNeg = (map->mag < 0) ;

  /* Impenetrable fudge factors....(EG), the "- 5" and the "1.05" just seem  */
  /* to be hacks to make things come out right....                           */
  map->mag = (mapGraphHeight - topMargin - 5) /  (1.05 * (map->max - map->min)) ;

  if (isNeg) map->mag = -map->mag ;

  (map->draw) () ;

  return;
} /* mapWhole */


void mapZoomIn (void)
{ 
  MAP map = currentMap("mapZoomIn") ;

  selectedMap = map ;
  map->mag *= 2 ;
  (map->draw) () ;

  return;
} /* mapZoomIn */


void mapZoomOut (void)
{
  MAP map = currentMap("mapZoomOut") ; 

  selectedMap = map ;
  map->mag /= 2 ; 
  (map->draw)() ;

  return;
} /* mapZoomOut */

/*************************************************/
/************* middle button for thumb **********/

static double oldx, oldy, oldDy ;
static BOOL dragFast ;

static void mapMiddleDrag (double x, double y) 
{
  MAP map = currentMap("mapMiddleDrag") ;

  if(dragFast)
    { graphXorLine (0, oldy - oldDy, map->thumb.x, oldy - oldDy) ;
      graphXorLine (0, oldy + oldDy, map->thumb.x, oldy + oldDy) ;
    }
  else
    graphXorLine (map->thumb.x, oldy, 1000, oldy) ;

  oldy = y ;

  if(dragFast)
    { oldDy *= exp ((x - oldx) / 25.) ;
      oldx = x ;
      graphXorLine (0, y - oldDy, map->thumb.x, y - oldDy) ;
      graphXorLine (0, y + oldDy, map->thumb.x, y + oldDy) ;
    }
  else
    {
      graphXorLine (map->thumb.x, y, 1000, y) ;
      if (map->drag)
	(*(map->drag))(y);
    }

  return;
} /* mapMiddleDrag */

/**************/

void mapNoMag (void)
{ isMagPossible = FALSE ; }

/**************/
static void mapMiddleUp (double x, double y) 
{
  float x1,x2,y1,y2 ;
  MAP map = currentMap("mapMiddleUp") ;

  if (dragFast)
    {
      graphBoxDim (map->thumb.box, &x1, &y1, &x2, &y2) ;
      if (isMagPossible)
        map->mag *= (y2 - y1) / (2. * oldDy) ;
      map->centre = WHOLE2MAP(map, y) ;
    }
  else
    map->centre = GRAPH2MAP(map,y) ;

  (map->draw)() ;
  
  return;
} /* mapMiddleUp */

void mapMiddleDown (double x, double y)
{
  float x1,x2,y1,y2 ;
  MAP map = currentMap("mapMiddleDown") ;

  selectedMap = map ;
  if (map->thumb.box)
    { graphBoxDim (map->thumb.box, &x1, &y1, &x2, &y2) ;
      oldDy = (y2 - y1) / 2. ;
    }
  else
    oldDy = 0;
  
  dragFast = (oldDy && x < map->thumb.x) ; /* thumb->box == 0 if locator not shown */

  if (dragFast)
    { graphXorLine (0, y - oldDy, map->thumb.x, y - oldDy) ;
      graphXorLine (0, y + oldDy, map->thumb.x, y + oldDy) ;
    }
  else
    graphXorLine (map->thumb.x, y, 1000, y) ;
   
  oldx = x ;
  oldy = y ;
  graphRegister (MIDDLE_DRAG, mapMiddleDrag) ;	/* must redo */
  graphRegister (MIDDLE_UP, mapMiddleUp) ;

  return;
} /* mapMiddleDown */

/****************************************************/
/********************** scale ***********************/

void mapFindScaleUnit (MAP map, float *u, float *sub)
{
  float cutoff = 5 / map->mag ;
  float unit = *u ;
  float subunit = *u ;

  if (cutoff < 0)
    cutoff = -cutoff ;

  while (unit < cutoff)
    { unit *= 2 ;
      subunit *= 5 ;
      if (unit >= cutoff)
	break ;
      unit *= 2.5000001 ;	/* safe rounding */
      if (unit >= cutoff)
	break ;
      unit *= 2 ;
      subunit *= 2 ;
    }
  subunit /= 10 ;
  if (subunit > *sub)
    *sub = subunit ;
  *u = unit ;

  return;
} /* mapFindScaleUnit */

/***********************************************/
/************** whole map locator **************/

static void mapThumbCalc (MAP map) /* sets up whole transformation */
{
  float y0, yn;
  float  delta = map->max - map->min ;

  y0 = 3 + topMargin ;
  yn = mapGraphHeight - 3 ;

  if (delta == 0)
    delta = 1 ;

  if (map->mag < 0)
    { map->thumb.offset = map->max ;
      map->thumb.fac = (y0 - yn) / delta ;
    }
  else
    { map->thumb.offset = map->min ;
      map->thumb.fac = (yn - y0) / delta ;
    }


#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
  /* I've put this back....ugh... */
  map->thumb.halfwidth = 0.5 * map->thumb.fac *
				(mapGraphHeight-topMargin) / map->mag ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */


  /* This is still not right, the half width ends up being too small by a    */
  /* fraction, I think some of the above can be rationalised.                */
  map->thumb.halfwidth = 0.5 * map->thumb.fac *
				(yn - y0) / map->mag ;



  map->thumb.x = 0 ;					    /* ?????? */

  return;
} /* mapThumbCalc */

/*******************************/

void mapThumbDrag (float *x, float *y, BOOL isDone)
{
  static float oldX, oldy ;
  static BOOL isDragging = FALSE ; 
  static BOOL isDragged ;
  float top, bottom ;
  MAP map = currentMap("scaleBoxDrag") ;

  if (isDragging)
    {
      *x = oldX ;   /* fix x */  
      if (oldy + 1 < *y || oldy - 1 > *y) /* dragging occured */
	isDragged = TRUE ;
    }
  else
    { oldX = *x ; oldy = *y ;
      isDragging = TRUE ; isDragged = FALSE ;
    }

  /* stop the user pulling the thumb bar out of the extent */
  top = MAP2WHOLE(map, map->min);
  bottom = MAP2WHOLE(map, map->max);
  if (map->thumb.fac < 0 )
    { float tmp = bottom; 
      bottom = top;
      top = tmp;
    }
  if (*y < top)
    *y = top;
  if ((*y + 2 * map->thumb.halfwidth) > bottom)
    *y = bottom - 2 * map->thumb.halfwidth;


  if (isDone)
    { 
      isDragging = FALSE ;
      if (isDragged)
	map->centre = WHOLE2MAP(map, *y + map->thumb.halfwidth);
      /* else used centre reregistered in mapLocatorPick */
      (*map->draw)() ;
    }

  return;
} /* mapThumbDrag */


/* look not used but kept because this is a column display */
/* This is of type MapColDrawFunc                                            */
/*                                                                           */
void mapShowLocator(LOOK look_unused, float *offset,  MENU *menu_unused)
{
  MAP map = currentMap("mapShowLocator") ;
  float t, b, y, y1, y2 ;
  Array aa = 0 ;
  int box, i ;

  *offset += 1.5 ;

  y = MAP2WHOLE(map, map->centre) ;

 /* now the full black rectangle  also in the background */
  t = GRAPH2MAP(map, topMargin + 1) ;
  t = MAP2WHOLE (map, t) ;
   if (t < topMargin + .2)
     t =  topMargin + .2 ;
  b = GRAPH2MAP(map, mapGraphHeight-1) ;
  b = MAP2WHOLE (map, b) ;
  if (b > mapGraphHeight)
    b = mapGraphHeight ;

  
   /* having a box around the fillrect is needed to see it
     on the web display, at least for a sequence display
     not sure why
     */

  box = graphBoxStart() ;
  graphFillRectangle (*offset - 0.15, t, *offset + 0.15, b) ;
  /*
  graphFillRectangle (*offset - 0.25, MAP2WHOLE(map, map->min),
		      *offset + 0.25, MAP2WHOLE(map, map->max)) ;
		      */
  graphBoxEnd () ;
  graphBoxInfo(box, 0, "Scroll Bar") ;


 
  /* now a left mouse pickable box, for direct action, usable under netscape */
  box = graphBoxStart () ;
  graphLine (*offset, t, *offset, b) ;  /* insures correct size of pickable box */

  aa = arrayCreate (8, float) ; i = 0 ;
  y1 = topMargin + 1. ;
  y2 = topMargin + 2. ;

  if (y1 < t )
    {
      graphColor (GREEN) ;
      array (aa, i++, float) = *offset ;
      array (aa, i++, float) = y1 ;
      array (aa, i++, float) = *offset + .5 ;
      array (aa, i++, float) = y2 ;
      array (aa, i++, float) = *offset - .5 ;
      array (aa, i++, float) = y2 ;
      array (aa, i++, float) = *offset ;
      array (aa, i++, float) = y1 ;
      graphPolygon (aa) ;
      graphColor (BLACK) ;
      graphLine (*offset, t, *offset, y2) ;
    }

  arrayDestroy (aa) ;
  aa = arrayCreate (8, float) ;
  y1 = mapGraphHeight - 1. ;
  y2 = mapGraphHeight - 2. ;
  
  i = 0 ;
  if (y1 > b )
    {
      graphColor (GREEN) ;
      array (aa, i++, float) = *offset ;
      array (aa, i++, float) = y1 ;
      array (aa, i++, float) = *offset + .5 ;
      array (aa, i++, float) = y2 ;
      array (aa, i++, float) = *offset - .5 ;
      array (aa, i++, float) = y2 ;
      array (aa, i++, float) = *offset ;
      array (aa, i++, float) = y1 ;
      graphPolygon (aa) ;
      graphColor (BLACK) ;
      graphLine (*offset, b, *offset, y2) ;
    }
  arrayDestroy (aa) ;
  graphBoxEnd();

  graphBoxDraw (box, BLACK, WHITE) ;
  graphBoxSetPick (box, FALSE);

  /* now the box-dragable thumb box */
  map->thumb.x = *offset;

  map->thumb.box = graphBoxStart();  
  t = y - map->thumb.halfwidth;
  b = y + map->thumb.halfwidth;
  if (t < topMargin)
    t = topMargin;
  if (b > mapGraphHeight)
    b = mapGraphHeight;

  if (b > topMargin && 
      t < mapGraphHeight)
    graphRectangle (*offset - 0.5, t, *offset + 0.5, b) ;

  graphBoxEnd();
  graphBoxDraw (map->thumb.box, DARKGREEN, GREEN);

  *offset += 1 ;

  return;
} /* mapShowLocator */



/*********************************************/
/***************** cursor ********************/

void mapCursorCreate (MAP map, float unit, float x0)
{
  map->cursor.unit = unit ;
  map->cursor.val = x0 / unit ;

  return;
} /* mapCursorCreate */


void mapCursorSet (MAP map, float x)
{
  if (x > 0)
    map->cursor.val = 0.5 + x / map->cursor.unit ;
  else
    map->cursor.val = -0.5 + x / map->cursor.unit ;
  mapCursorShift (map) ;

  return;
} /* mapCursorSet */

static float yCursor ;

void mapCursorDraw (MAP map, float x)
{
  float z = mapCursorPos(map) ;
  float y = MAP2GRAPH(map, z) ;	/* Jean - your x here was a bug */

  if (map->flip)
    z = - z ; 
  if (map->cursor.unit < .011)
    strcpy (map->cursor.text, messprintf ("%.2f",z)) ;
  else
    strcpy (map->cursor.text, messprintf ("%.0f",z)) ;
  map->cursor.box = graphBoxStart() ;
  graphLine (map->thumb.x, y, 1000, y) ;
  map->cursor.pickBox = graphBoxStart() ;
  graphColor (LIGHTGREEN) ;
  graphFillRectangle (x+0.5, y-0.5, x+2.5+strlen(map->cursor.text), y+0.5) ;
  graphColor (BLACK) ;
  graphTextPtr (map->cursor.text, x+0.5, y-0.5, strlen(map->cursor.text)) ;
  graphBoxEnd () ;
  graphBoxEnd () ;
  graphBoxSetPick (map->cursor.box, FALSE) ; /* only pick on .pickBox */
  graphBoxDraw (map->cursor.box, BLACK, TRANSPARENT) ;
  yCursor = y ;

  return;
} /* mapCursorDraw */


void mapCursorShift (MAP map)
{
  float x1, x2, y1, y2, z = mapCursorPos(map) ;

  if (!map->cursor.box)
    return ;
  graphBoxDim (map->cursor.box, &x1, &y1, &x2, &y2) ;
  if (map->flip)
    z = - z ; 
  if (map->cursor.unit < .011)
    strcpy (map->cursor.text, messprintf ("%.2f",z)) ;
  else
    strcpy (map->cursor.text, messprintf ("%.0f",z)) ;
  y1 = MAP2GRAPH(map, mapCursorPos(map)) ;
  graphBoxShift (map->cursor.box, x1, y1-0.5) ;
  yCursor = y1 ;

  return;
} /* mapCursorShift */


void mapCursorDrag (float *x, float *y, BOOL isDone)
{
  static float oldX ;
  static BOOL isDragging = FALSE ;

  if (isDragging)
    *x = oldX ;
  else
    { oldX = *x ;
      isDragging = TRUE ;
    }

  if (isDone)
    { float newpos ;
      MAP map = currentMap("mapCursorDrag") ;

      newpos = GRAPH2MAP(map, *y+0.5) ;
      mapCursorSet (map, newpos) ;
      isDragging = FALSE ;
    }

  return;
} /* mapCursorDrag */

/***************************************************/

/* look not used but kept because this is a column display */
/* This is of type MapColDrawFunc                                            */
/*                                                                           */
void mapShowScale(LOOK generic_look, float *offset,  MENU *menu_unused)
{
  float unit = 0.01 ;
  float subunit = 0.001 ;
  float x, xx, y ;
  int resolution = 0, max = 0 ;
  char *cp = 0 ;
  MAP map = currentMap("mapShowScale") ;

  mapFindScaleUnit (map, &unit, &subunit) ;
  if (unit >= 1)
    resolution = 0 ;
  else if (unit >= .1)
    resolution = 1 ;
  else if (unit >= .01)
    resolution = 2 ;
  else if (unit >= .001)
    resolution = 3 ;

  x = GRAPH2MAP(map, topMargin+1) ;
  x = unit * (((x>=0)?1:0) + (int)(x/unit)) ;
  while ((y = MAP2GRAPH(map, x)) < mapGraphHeight - 1)
    { graphLine (*offset+0.5,y,*offset+1.5,y) ;
      xx = map->flip ? -x : x ;
      cp = messprintf ("%-4.*f", resolution, xx) ;
      graphText (cp, *offset+2, y-0.5) ;
      if (strlen(cp) > max)
	max = strlen(cp) ;
      x += unit ;
    }

  x = GRAPH2MAP(map, topMargin+1) ;
  x = subunit * (((x>=0)?1:0) + (int)(x/subunit)) ;
  while ((y = MAP2GRAPH(map, x)) < mapGraphHeight - 1)
    {
      graphLine(*offset+1.0, y, *offset+1.5, y) ;
      x += subunit ;
    }
  
  graphLine(*offset+1.5, topMargin+1, 
	    *offset+1.5, mapGraphHeight - 1.5 ) ;
#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
  /* I'd like to use look->max but its private to fmap, there would need to  */
  /* be some sort of interface to get top/bot information set....it's a      */
  /* bit of a mess.                                                          */
	    *offset+1.5, MAP2GRAPH(look->map, look->max)) ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */


  if (map->thumb.x)
    {
      graphLine (map->thumb.x, 
		 MAP2WHOLE(map, map->centre) - map->thumb.halfwidth,
		 *offset+1.5, topMargin+1) ;
      graphLine (map->thumb.x, 
		 MAP2WHOLE(map, map->centre) + map->thumb.halfwidth,
		 *offset+1.5, mapGraphHeight-0.5) ;
    }

  mapCursorDraw (map, *offset) ;

  *offset += 2 + max ;

  return;
} /* mapShowScale */


/*************** mapPrint stuff back from the graphPackage **************/

/* Called as a menu function from at least fmap and probably other displays. */
void mapPrint (void)
{ 
  float oldCentre, oldMag ;
  float min, max, tmp ;
  int   origin = 0, nx = 1, ny, imin, imax ;
  void  *fMapLook = 0 ;
  MAP map = currentMap("mapPrint") ; 
  ACEIN zone_in;
 
  selectedMap = map ;

  /* save our state. */
  oldCentre = map->centre ;
  oldMag = map->mag ;

  graphFitBounds(&nx, &ny) ;

  if (/*!(fMapLook = fMapGifLook) &&*/
      !graphAssFind (&MAP2LOOK_ASSOC, &fMapLook)
      && fMapFindZone(fMapLook, &imin, &imax, &origin))
    {
      min = imin ;
      max = imax ;
    }
  else
    {
      min = map->min ; 
      max = map->max ; 
      origin = 0 ; 
    }

  /* convert to user visible coords */
  if (map->mag < 0)
    {
      tmp = min ;
      min = map->max - max + 1 - origin ; 
      max = map->max - min - origin ;
    }
  else
    {
      min -= origin - 1 ;
      max -= origin ;
    }
				/* ask for bounds */
#ifdef WORM
  if (!(zone_in = messPrompt("Give nx, mag, min, max",
			     messprintf("%d %.2f %.2f %.2f", 
					nx, mag, min, max), 
			     "ifffz", 0)))
    return ;
  aceInInt (zone_in, &nx) ;
  aceInFloat (zone_in, &map->mag) ;
#else
  if (!(zone_in = messPrompt("Please state the zone you wish to print",
			     messprintf("%g   %g", min, max),
			     "ffz", 0)))
    return ;
#endif

  aceInFloat (zone_in, &min) ;
  aceInFloat (zone_in, &max) ;
  aceInDestroy (zone_in);

  if (min >= max)
    {
#ifdef WORM
      map->mag = oldMag ;
#endif

      return ;
    }

  /* convert back */
  if (map->mag < 0)
    {
      tmp = min ;
      min = map->max - origin - max ;
      max = map->max - origin - tmp + 1 ;
    }
  else
    {
      min += origin - 1 ;
      max += origin ;
    }

  if (min < map->min)
    min = map->min ;
  if (max > map->max)
    max = map->max ;

  map->centre = (max + min) / 2 ;
 




  /* THIS GETS THE RIGHT BIT OF GRAPH DRAWN BUT AT A STUPID SCALE.... */

  /* We need to set the new mag factor here otherwise when graphBoundsPrint calls our
   * draw function it will all be wrong. */
  map->mag = (ny - topMargin - 5) /  (1.05 * (max - min)) ;

  {
    int tmp_x, tmp_y ;

    /* TRIAL CODE TO TRY AND DO MULTI PAGE PRINTS.... */

    graphGetBounds(&tmp_x, &tmp_y) ;


    graphRawBounds(tmp_x, tmp_y * 3) ;

    map->mag = (tmp_y - topMargin - 5) /  (1.05 * (max - min)) ;


    tmp_x = tmp_y ;					    /* for debugger stop. */
  }

  /* This function uses map->draw to redraw the map at the new scale before printing it. */
  graphBoundsPrint(nx + 0.2, 
		   ((1.05 * (max - min)) * (map->mag > 0 ? map->mag : - map->mag)) + topMargin + 5,
		   map->draw) ;


  
  /* Now restore the old position and redraw the map as it was originally. */
  map->centre = oldCentre ;
  map->mag = oldMag ;
  (map->draw)() ;

  return ;
} /* mapPrint */


/***************************************************************/
/************ code to draw buttons at bottom of page ***********/

/* Called when a column button is clicked at the bottom of the map to turn   */
/* on/off a column.                                                          */
/*                                                                           */
static void mapFlipButton (void *arg)
{
  MapColRec *col = (MapColRec*)arg ;
  MAP map = currentMap("mapFlipButton") ;

  col->isOn = !col->isOn ;

  /* Record which column was toggled, potentially needed by the draw routine */
  /* called after this.                                                      */
  map->butPressCol = col ;

  (map->draw)() ;

  map->butPressCol = NULL ;				    /* make sure we reset. */

  return;
} /* mapFlipButton */



/* Return whether last user action was toggling a column on/off, whether     */
/* from the columns menu, or from the columns buttons at the bottom of the   */
/* fmap.                                                                     */
/* Should only be called from fmaps redraw function, not a great interface,  */
/* but this function does not justify a more generalised interface.          */
/*                                                                           */
/* mapFlipButton() & mapColToggle() set map->butPressCol then call fmap draw,*/
/* then reset  map->butPressCol                                              */
BOOL mapColButPressed(MAP map)
{
  BOOL result ;

  if (map->butPressCol == NULL)
    result = FALSE ;
  else
    result = TRUE ;

  return(result) ;
}


/* Can be called by fmap to get the column button that was last clicked to   */
/* be redrawn. This is done when we don't want to redraw the whole of fmap   */
/* for each button click to turn a column on/off.                            */
void mapRedrawColBut(MAP map)
{
  int bg ;

  if (map->butPressCol != NULL)
    {
      bg = map->butPressCol->isOn ? LIGHTGRAY : WHITE ;

      graphBoxDraw(map->butPressCol->box, BLACK, bg) ;
    }

  return ;
}


int mapDrawColumnButtons (MAP map)
{
  int i ;
  int box ;
  float h ;
  MapColRec *col ;
  COLOUROPT *buttons ;
  int first_box_num;

  /* use ColouredButtons to get background and to pass arg = MapColRec pointer */
  buttons = (COLOUROPT*) messalloc (sizeof(COLOUROPT)*(arrayMax(map->cols)+1)) ;
  for (i = 0 ; i < arrayMax(map->cols) ; ++i)
    { 
      col = arrp(map->cols, i, MapColRec) ;
      buttons[i].text = col->name ;
      buttons[i].arg = col ;
      buttons[i].f = mapFlipButton ;
      buttons[i].fg = BLACK ;
      buttons[i].bg = col->isOn ? LIGHTGRAY : WHITE ;
    }

  /* draw dummy version off screen to find height */
  box = graphBoxStart () ;
  graphColouredButtons(buttons, -1000, 0, -1000 + mapGraphWidth) ;
  graphBoxEnd () ;
  graphBoxDim (box,0,0,0,&h) ;

  if (h >= mapGraphHeight)
    {
      messout ("Sorry, window not tall enough to show column buttons") ;
      return 0;			/* box 0 is impossible BoxNo for button */
    }

  /* white out background */
  graphColor (WHITE) ;
  graphFillRectangle (0, mapGraphHeight-h, mapGraphWidth+1, mapGraphHeight+1) ;

  graphColor (BLACK) ;
  first_box_num = graphColouredButtons(buttons, 0, mapGraphHeight-h, mapGraphWidth) ;

  /* Record the box number of each column button.                            */
  for (i = 0 ; i < arrayMax(map->cols) ; i++)
    {
      arrp(map->cols, i, MapColRec)->box = i + first_box_num ;
    }


  return first_box_num;
} /* mapDrawColumnButtons */

/************** end of file ***********/
 

#ifdef CODE_NEVER_USED
BOOL mapActive(KEYSET *setp, void** mapp) 
{ 
  if (selectedMap && selectedMap->magic == &MAP_MAGIC
      && selectedMap->activeSet)
    { if (setp) *setp = selectedMap->activeSet (selectedMap->look) ;
      if (mapp) *mapp = selectedMap ;
      return TRUE ;
    }
  selectedMap = 0 ;
  if (setp) *setp = 0 ; 
  if (mapp) *mapp = 0 ;
  return FALSE ;
}
#endif /* CODE_NEVER_USED */

/***************************************/
