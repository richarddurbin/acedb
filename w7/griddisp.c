/*  File: griddisp.c
 *  Author: Richard Durbin (rd@mrc-lmb.cam.ac.uk)
 *  Copyright (C) J Thierry-Mieg and R Durbin, 1991
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
 * Description:  to display clone grids, such as the Yac grid
 * Exported functions: gridDisplay(), gridCluster(), gridClusterKey()
 * HISTORY:
 * Last edited: May 13 08:38 2008 (edgrif)
 * * Jul  7 13:36 1999 (edgrif): Added code to support exclusive tag for
 *              grid. Added code to make sure middle button updates
 *              clone label.
 * * Apr 16 13:52 1999 (edgrif): Proper fix for SANgc03641 
 * * Mar 24 15:54 1999 (edgrif): Removed below fix...incorrect.
 * * Mar 16 13:06 1999 (edgrif): Fix grid box edit menus (SANgc03641)
 * * Jan  1 11:05 1995 (rd): change to use View to set colours
 * * Jan  1 11:05 1995 (rd):   and to work tag2 on Positive, Negative
 * * Jun 23 23:20 1992 (rd): change GRIDMAP, gridCluster() etc to
 *	        work with intervals
 * * Mar  2 13:45 1992 (rd): Pool handling, surrounds for 
 *	        comparison, getting probe and clone names by clicking
 * * Feb 21 20:02 1992 (rd): CloneDisp looks for matches in any 
 *	        grid FLAG_CHANGED
 * * Jan  9 19:01 1992 (rd): gridKeyFind()
 * * Jan  9 15:06 1992 (rd): FLAG_BLANK
 * * Dec 31 13:22 1991 (rd): added support for _No_stagger
 * Created: Thu Nov  7 15:30:07 1991 (rd)
 * CVS info:   $Id: griddisp.c,v 1.52 2008-05-20 15:43:36 edgrif Exp $
 *-------------------------------------------------------------------
 */
#include <wh/acedb.h>
#include <wh/aceio.h>
#include <wh/lex.h>
#include <wh/bs.h>
#include <wh/query.h>
#include <wh/session.h>
#include <wh/grid.h>
#include <wh/pmap.h>
#include <wh/pick.h>
#include <wh/aql.h>
#include <whooks/systags.h>
#include <whooks/classes.h>
#include <whooks/sysclass.h>
#include <whooks/tags.h>
#include <wh/menu.h>
#include <wh/display.h>
#include <wh/main.h>
#include <wh/keysetdisp.h>
#include <w7/grid_.h>
#include <wh/utils.h>             /* for lexstrcmp */


/************************************************************/
enum {GRID_REPORT_BOX_LEN = 1024} ;
#define GRID_REPORT_BOX_OVERFLOW " *"


/* Default grid sizes.                                                       */
#define GRID_NORMAL_SPACEWIDTH   2.0 
#define GRID_NORMAL_SPACEHEIGHT  GRID_NORMAL_SPACEWIDTH * 0.5
#define GRID_NORMAL_BOXWIDTH     0.9
#define GRID_NORMAL_BOXHEIGHT    GRID_NORMAL_BOXWIDTH * 0.8
#define GRID_SMALL_WIDTH         GRID_NORMAL_SPACEWIDTH * 0.66
#define GRID_SMALL_HEIGHT        GRID_NORMAL_SPACEHEIGHT * 0.66

/* By default "small" grid is this much smaller than "normal"                */
#define GRID_REDUCE              0.5





/* Grid display control block.                                               */
/*                                                                           */
typedef struct GridDispStruct {
  magic_t *magic ;					    /* == &GridState_MAGIC */
  KEY   key ;						    /* for the grid object */
  Array segs ;						    /* array of SEG's indexed by box */
  KEY   editTag ;
  MENU  editMenu ;					    /* popup edit mode menu over grids,
							       can be NULL. */
  MENU  gridMenu ;					    /* Main grid menu. */
  BOOL  exclusiveTags ;					    /* Are tags mutually exclusive. */
  MENUITEM exclusiveItem ;				    /* Menu item for toggling exclusive tag. */
  Array tagValues ;    					    /* All possible tag values for a grid. */
  Associator tagMap ;					    /* Associates a tag value with its
							       priority, colour & surround colour. */
  KEY   probe, probeStore ;
  Array tab, tabStore ;					    /* relational table of TAB */
  int   dxLine, dyLine, dxSpace, dySpace ;
  char  genXLabel[32], genYLabel[32] ;			    /* Martin Ferguson, for generic grid 
							       labels */
  char  *title ;
  int   flag ;
  char  selectName[64] ;
  char  probeName[64] ;
  char  probeClassName[64] ;
  Array map ;
  int   activeBox ;
  int   crossBox ;					    /* Moved around over the grid boxes to 
							       show which box has been selected. */
  char  reportBuffer[GRID_REPORT_BOX_LEN] ;

  char  *prefix;					    /* prefix for lazy grid */
  KEY   template;					    /* i think this will be a clone or a
							       pac key */   

  /* Grid spacing/sizing for "normal" and "small" views.                     */
  /*                                                                         */
  GridDimensionStruct small ;
  GridDimensionStruct normal ;

} *GridDisp ;


/************************************************************/

/* We use the address of the these strings as unique ids for the grid        */
/* associator from which we retrieve the control block and the control block */
/* contains the magic tag which tries to show the control block is a) the    */
/* right one and b) not corrupted.                                           */
static magic_t GRAPH2GridState_ASSOC = "GRAPH2GridStateAssoc" ;
static magic_t GridState_MAGIC = "GridState" ;


/************************************************************/

typedef int GKEY;

typedef struct
{ GKEY gkey ;
  int x, y ;
  char *name;
  unsigned short flag ;
  unsigned short flagStore ;
  int mapIndex ;
} SEG ;

typedef struct
{ KEY  probe, tag;
  GKEY target ;
  int targetbox ; /* used for lazy grid, since it does not have a key yet */
  unsigned int flag ;
} TAB ;

/************************************************************/


enum BoxNames { BACKGROUND=0, 
		SELECT_BUTTON_BOX, 
		SELECT_NAME_BOX, SELECT_NAME_BOX2,
		CLEAR_BOX, 
		PROBE_BUTTON_BOX,
		PROBE_CLASS_NAME_BOX, PROBE_CLASS_NAME_BOX2,
		PROBE_NAME_BOX, PROBE_NAME_BOX2,
		EDIT_MODE_BOX, MAP_MODE_BOX, REPORT_BOX, 
		GRID_BOX,
		MIN_LIVE_BOX } ;  /* MIN_LIVE_BOX must be last */


/************************************************************/


	/* per GridDisp flags */
#define FLAG_NAMES		0x0001
#define FLAG_EDIT_MODE		0x0002
#define FLAG_EDITED		0x0004
#define FLAG_STAGGER		0x0008
#define FLAG_A1			0x0010
#define FLAG_SMALL		0x0020
#define FLAG_EDIT_NEG		0x0040
#define FLAG_VIRTUAL		0x0080
#define FLAG_LABELS             0x0100			    /* Martin Ferguson, for generic grid labels */
#define FLAG_XY			0x0200			    /* Martin Ferguson, for generic grid labels */
#define FLAG_SQUASHED           0x0400			    /* Simon Kelley, squashed grid display */
#define FLAG_EXCLUSIVETAGS      0x0800			    /* For mutually exclusive flags. */
#define FLAG_LABEL_PER_BOX      0x1000			    /* For showing a label per box, rather */
							    /* than default label per block. */

/* per SEG flags */
#define FLAG_POS		0x0001
#define FLAG_NEG		0x0002
#define FLAG_BLANK		0x0004
#define FLAG_SELECTED		0x0008
#define FLAG_PER_KEY		0x0004 /* info about clone, not hyb */
#define FLAG_TEMP		0x0010 /* used for default_negative */

/* per TAB flags */
/* #define FLAG_POS		0x0001 */
/* #define FLAG_NEG		0x0002 */
#define FLAG_DELETED		0x0004
#define FLAG_CHANGED		0x0008

#define MAX_NEDIT 20


/************************************************************/

static BOOL isInitialised = FALSE ;

static int _VGrid ;
static KEY _Positive, _Surround_colour, _Tag ;
static KEY _Grid_exclusive_tags, _Grid_edit_default, _Grid_edit_menu ;
static KEY _Negative, _Grid_map, _View, _Mixed ;
static KEY _Grid_data, _Grid_data_item, _Conflict, _Default_negative ;
static KEY _Export_order ;


/************************************************************/
static GridDisp getGridState(char *caller) ;
static void gridDestroy (void) ;
static void gridConvert (GridDisp look) ;     
static void gridDraw (GridDisp look) ;
static void gridPick (int box, double x_unused, double y_unused, int modifier_unused) ;
static void gridKeyboard (int k, int modifier_unused) ;
static void gridSelect (GridDisp look, int box) ;
static void gridSelectEntry (char *string) ;
static void gridToggleSymbolic (void) ;
static void gridToggleSmall (void) ;
static void gridToggleExclusive (void) ;
static void gridProbeDisplay (GridDisp look, KEY probe) ;
static void gridDump (void) ;
static void gridTreeProbe (void) ;
static void gridSave (void) ;
static void gridSetRange (void) ;
static BOOL gridgKeyFind (GridDisp look, GKEY clone, int *pbox) ;
static void gridKeySetStats (void) ;
static void gridKeySetApply (void) ;
static void gridComparisonStore (void) ;
static void gridSelectButton (void) ;
static void gridProbeButton (void) ;
static void gridChangeClone (void) ;
static void gridMapMode (void) ;
static void gridSpaceSet (void) ;
static void gridSizeSet (void) ;
static void gridHybBuild (GridDisp look, KEY probe) ;
static int  gridBoxColour (GridDisp look, int box, BOOL isStore) ;
static void gridReport (GridDisp look, int box) ;
static void gridKeySetExportData (void) ;
static void printhyb(void);
static void clampBoxSize(GridDimension grid_box) ;



#define GRID_EXCLUSIVE_OFF "Set exclusive tags Off"
#define GRID_EXCLUSIVE_ON  "Set exclusive tags On"

static MENUSPEC gridMenuSpec[] = {
  { (MENUFUNCTION)graphDestroy, "Quit"},
  { (MENUFUNCTION)help, "Help"},
  { (MENUFUNCTION)graphPrint, "Print"},
  { (MENUFUNCTION)displayPreserve, "Preserve"},
  { (MENUFUNCTION)gridComparisonStore, "Centre - Surround"},	/* mac can't handle <,> */
  { (MENUFUNCTION)gridToggleExclusive, GRID_EXCLUSIVE_ON},
  { (MENUFUNCTION)gridToggleSymbolic, "Toggle name display"},
  { (MENUFUNCTION)gridToggleSmall, "Toggle small display"},
  { (MENUFUNCTION)gridSpaceSet, "Set grid box spacing"},
  { (MENUFUNCTION)gridSizeSet, "Set grid box size"},
  { (MENUFUNCTION)gridTreeProbe, "Display probe as tree"},
  { (MENUFUNCTION)gridSave, "Save data with probe"},
  { (MENUFUNCTION)gridSetRange, "Set cluster range"},
  { (MENUFUNCTION)gridKeySetStats, "Stats on keyset"},
  { (MENUFUNCTION)gridKeySetApply, "Apply keyset"},
  { (MENUFUNCTION)gridKeySetExportData, "Export data on keyset"},
  { (MENUFUNCTION)printhyb,"print hyb data"},
  { 0, 0 }, 
  { (MENUFUNCTION)gridChangeClone, "Change Gridded Clone"},
  { (MENUFUNCTION)gridDump, "Dump"},
  /*{ printhyb,"print hyb data"}, debug check */
  { 0, 0 }
} ;


/************************************************************/


/* Retrieves pointer to our global grid state control block from the global  */
/* graph associator.                                                         */
/*                                                                           */
static GridDisp getGridState(char *caller)
  {
  GridDisp result = NULL ;

  if (!graphAssFind (&GRAPH2GridState_ASSOC, &result))
    messcrash(GRID_PREFIX "%() could not retrieve GridDisp pointer from graph.", caller) ;
  if (result == NULL)
    messcrash(GRID_PREFIX "%s() retrieved the GridDisp pointer from graph but it was NULL.", caller) ;
  if (result->magic != &GridState_MAGIC)
    messcrash(GRID_PREFIX "%s() retrieved the GridDisp pointer from graph but the GridDisp is corrupted", caller) ;

  return result ;
  }


static void checkGridKeys()
{
  KEY newKey;
  OBJ Temp, New;
  SEG *seg;
  TAB *tab;
  register int i;
  GridDisp look ;

  look = getGridState("createGridKey");

  tab = arrp(look->tab, 0, TAB) ;
  for (i = 0 ; i < arrayMax(look->tab) ; ++i, ++tab)
    {
      if(tab->target == 0)
	{
	  seg = arrp(look->segs, tab->targetbox , SEG);
      
	  lexaddkey(seg->name, &newKey, class(look->template));
	  Temp = bsCreate(look->template);      
	  New = bsClone(newKey, Temp);
	  
	  bsDestroy(Temp);
	  bsSave(New);
	  tab->target = newKey;
	  seg->gkey = newKey;
	}
    }

  return ;
}

static char* gName(SEG *gi)
{
  if(gi->name)
    return gi->name;
  else
    return name(gi->gkey);
}

static void printhyb()
{
  TAB *tab;
  register int i;
  GridDisp look ;

  look = getGridState("printhyb");

  tab = arrp(look->tab, 0, TAB) ;
  for (i = 0 ; i < arrayMax(look->tab) ; ++i, ++tab)
    printf("%d, probe=%d, tag=%d,target=%d,targetbox=%d,flag=%d\n",i,tab->probe,tab->tag,tab->target,tab->targetbox,tab->flag);
}
/*
static OBJ gUpdate(SEG *gi)
{
  KEY key;
  OBJ obj;
  if(gi->name){
    lexaddkey(gi->name,&key,class(gi->gkey));
    obj = bsUpdate(key);
    COMMENT copy gi->key into obj COMMENT 
    gi->gkey = key;
    messfree(gi->name);
    gi->name = 0;
    return obj;
  }
  else
    return bsUpdate(gi->gkey);
}
*/
static void lazySegBuild (GridDisp look, int *n, char *prefix, BOOL blank)
{
  static int xblock,yblock;
  register int k,i;
  char row[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
  char temp[15],colchar[2],rowchar[2];
  int classe,nn;
  BOOL XAlpha = FALSE, YAlpha = FALSE;
  int xstart,ystart,temp1,xend,yend;
  SEG *seg;
  KEY key;

/*  classe = pickWord2Class (name(look->key));*/

  if(*n == MIN_LIVE_BOX){
    xblock = 0;
    yblock = 0;
  }
  else{
    xblock++;
    if(xblock >= look->dxLine){
      xblock = 0;
      yblock++;
    }
  }
  if(blank)
    return;
  look->flag |= FLAG_LABELS ;
  look->flag |= FLAG_XY ; 
/*  look->dxLine = look->dyLine = 1;*/

  xstart = atoi(&look->genXLabel[0]) ;
  xend = atoi(&look->genXLabel[3]) ;
  
  ystart = atoi(&look->genYLabel[0]) ;
  yend = atoi(&look->genYLabel[3]) ;
  
  if (look->genXLabel[5] == 'A') XAlpha = TRUE ;
  if (look->genYLabel[5] == 'A') YAlpha = TRUE ;

  if (ystart > yend) 
    { 
      temp1 = ystart ;
      ystart = yend ;
      yend = temp1 ;
    }
  if (xstart > xend) 
    {
      temp1 = xstart ;
      xstart = xend ;
      xend = temp1 ;
    }
  
  classe = class(look->template);
/*  n = MIN_LIVE_BOX;*/
  nn = *n;
  for(k=ystart-1;k<=yend-1;k++){ /* for each row */
    if(YAlpha)
      sprintf(colchar,"%c",row[k]);
    else
      sprintf(colchar,"%d",k+1);
    for(i=xstart-1;i<=xend-1;i++){ /* for each colomn */
      if(XAlpha)
	sprintf(rowchar,"%c",row[i]);
      else
	sprintf(rowchar,"%d",i+1);
      seg = arrayp(look->segs, nn, SEG);
      seg->flag = seg->flagStore = 0;
      sprintf(temp,"%s%s%s",prefix,colchar,rowchar);
      if (lexword2key (temp, &key, classe)){
	seg->gkey = key;
	seg->name = 0;
      }
      else{
	seg->name = messalloc(strlen(temp) +1);
	strcpy(seg->name,temp);
	seg->gkey = 0;         /* not possible, used as a check */
      }
      seg->x = xblock + i*look->dxLine;
      seg->y = yblock + k*look->dyLine;
      ++nn;
    }
   }
  *n=nn;
}

void gridInitialise (void)
{
  KEY key ;

  lexaddkey ("Grid", &key, _VMainClasses) ; _VGrid = KEYKEY(key) ;

  lexaddkey ("Positive", &_Positive, 0) ;
  lexaddkey ("Negative", &_Negative, 0) ;
  lexaddkey ("Surround_colour", &_Surround_colour, 0) ;
  lexaddkey ("Tag", &_Tag, 0) ;
  lexaddkey ("Grid_exclusive_tags", &_Grid_exclusive_tags, 0) ;
  lexaddkey ("Grid_edit_default", &_Grid_edit_default, 0) ;
  lexaddkey ("Grid_edit_menu", &_Grid_edit_menu, 0) ;
  lexaddkey ("Grid_map", &_Grid_map, 0) ;
  lexaddkey ("View", &_View, 0) ;
  lexaddkey ("Mixed", &_Mixed, 0) ;
  lexaddkey ("Grid_data", &_Grid_data, 0) ;
  lexaddkey ("Grid_data_item", &_Grid_data_item, 0) ;
  lexaddkey ("Conflict", &_Conflict, 0) ;
  lexaddkey ("Default_negative", &_Default_negative, 0) ;
  lexaddkey ("Export_order", &_Export_order, 0) ;

  isInitialised = TRUE ;
}

/*********************************/

/* this is a DisplayFunc for the GRID type */
BOOL gridDisplay (KEY key, KEY from, BOOL isOldGraph, void *unused)
{
  int box ;
  GridDisp look = NULL ;

  if (!isInitialised)
    gridInitialise () ;

  if (key && class(key) != _VGrid)
    {
      from = key ;
      key = 0 ;
    }

  if (isOldGraph)
    {
      if ((look = getGridState("gridDisplay"))
	  && (!key || look->key == key))
	{
	  if (gridgKeyFind (look, from, &box))
	    {
	      gridSelect (look, box) ;
	      return TRUE ;
	    }
	  else if (from)
	    {
	      gridProbeDisplay (look, from) ;
	      return TRUE ;
	    }
	}
      if (!key)
	goto abort ;
      gridDestroy () ;
      graphAssRemove (&GRAPH2GridState_ASSOC) ;
    }
  else if (!key)
    goto abort ;
  else
    { 
      displayCreate ("GRID");

      graphRegister (DESTROY, gridDestroy) ;
      graphRegister (KEYBOARD,(KeyBoardFunc) gridKeyboard) ;
      graphRegister (PICK,(GraphFunc) gridPick) ;
      graphRegister (MESSAGE_DESTROY, displayUnBlock) ;
    }

  /* Set up/initialise our main control block.                               */
  look = (GridDisp) messalloc (sizeof (struct GridDispStruct)) ;      
  look->magic = &GridState_MAGIC ;
  look->key = key ;
  look->map = arrayCreate (16, GRIDMAP) ;
  look->tab = arrayCreate (16, TAB) ;
  look->tabStore = arrayCreate (16, TAB) ;
  look->tagMap = assCreate () ;
  look->gridMenu = menuInitialise("Grid Main Menu", gridMenuSpec) ;
  graphNewMenu(look->gridMenu);
  look->exclusiveItem = menuItem(look->gridMenu, GRID_EXCLUSIVE_ON) ;

  /* default sizings                                                         */
  look->small.horiz_space = GRID_SMALL_WIDTH ;
  look->small.vert_space = GRID_SMALL_HEIGHT ;
  look->small.box_width = GRID_SMALL_WIDTH ;
  look->small.box_height = GRID_SMALL_HEIGHT ;
  look->normal.horiz_space = GRID_NORMAL_SPACEWIDTH ;
  look->normal.vert_space = GRID_NORMAL_SPACEHEIGHT ;
  look->normal.box_width = GRID_NORMAL_BOXWIDTH ;
  look->normal.box_height = GRID_NORMAL_BOXHEIGHT ;


  gridConvert (look) ;		/* makes ->segs, dx values etc. */

  graphAssociate (&GRAPH2GridState_ASSOC, look) ;

  if (from)
    gridProbeDisplay (look, from) ;
  else
    gridDraw (look) ;

  return TRUE ;

 abort:
  return FALSE ;
}

/************************************************************/
/***************** Registered routines *********************/

static void gridDestroy (void)
{
  SEG *seg;
  register int i;
  GridDisp look ;

  look = getGridState("gridDestroy") ;

  if (look->editMenu)
    menuDestroy(look->editMenu) ;
  menuDestroy(look->gridMenu) ;


  for(i = MIN_LIVE_BOX; i < arrayMax(look->segs); ++i)
    {
      seg = arrp(look->segs, i, SEG);
      if(seg->name)
	messfree(seg->name);
    }
  arrayDestroy (look->segs) ;
  arrayDestroy (look->tab) ;
  arrayDestroy (look->tabStore) ;
  arrayDestroy (look->map) ;
  arrayDestroy (look->tagValues) ;
  assDestroy (look->tagMap) ;
  graphAssRemove (&GRAPH2GridState_ASSOC) ;
  look->magic = 0 ;
  messfree (look) ;
}

/***********************************/

static void gridEdit (GridDisp look, int box, int tag, int isPos)
{
  SEG *seg = arrp(look->segs, box, SEG), *seg2 ;
  GKEY key = seg->gkey ;
  TAB *tab ;
  int i, newFlag, box2 ;

  if (seg->flag & FLAG_BLANK)
    return ;

  /* Loop through existing edits until we get to the end, only stopping if.. */
  /*  - for exclusiveTags we find the same clone has been editted already,   */
  /*  - for non-exclusiveTags we find the same clone AND it has the same tag */
  /*    value (then we delete the existing tag).                             */
  tab = arrp(look->tab, 0, TAB) ;
  for (i = 0 ; i < arrayMax(look->tab) ; ++i, ++tab)
    {
    if (((tab->target == key && key !=0 ) || tab->targetbox == box)
	&& (!tab->probe || tab->probe == look->probe))
      {
	if (look->exclusiveTags)
	  break ;
	else if (tab->tag == tag)
	  break ;
      }
    }


  /* If we found an existing edit of the clone then change the edit,         */
  /* otherwise add a new one.                                                */
  if (i < arrayMax(look->tab))
    {
      if (look->exclusiveTags && tag != tab->tag)	    /* Same clone, new tag, overwrite old one. */
	tab->tag = tag ;
      else
	tab->flag ^= FLAG_DELETED ;			    /* Otherwise delete existing one. */
    }
  else
    {
      tab = arrayp(look->tab,arrayMax(look->tab),TAB) ;
      tab->probe = 0 ;	/* keep 0 until saved - look->probe may change */
      tab->tag = tag ;
      if(key != 0)
	tab->target = key ;
      else
	{
	tab->target = 0;
	tab->targetbox = box;
	}
      tab->flag = isPos ? FLAG_POS : FLAG_NEG ;
    }
  tab->flag |= FLAG_CHANGED ;


  /* reset FLAG_POS/NEG */
  newFlag = 0 ;
  for (i = 0 ; i < arrayMax(look->tab) ; ++i)
    {
      if ( (((tab = arrp(look->tab,i,TAB))->target == key) || arrp(look->tab,i,TAB)->targetbox == box)
	   && !(tab->flag & FLAG_DELETED))
	{
	  if (tab->flag & FLAG_POS)
	    newFlag |= FLAG_POS ;
	  if (tab->flag & FLAG_NEG)
	    newFlag |=  FLAG_NEG ;
	}
    }

  /* set flags and colours on segs */
  while (gridgKeyFind (look, seg->gkey, &box2))
    { 
      seg2 = arrp(look->segs, box2, SEG) ;
      seg2->flag &= ~(FLAG_POS | FLAG_NEG) ;
      seg2->flag |= newFlag ;
      graphBoxDraw (box2, -1, gridBoxColour (look, box2, FALSE)) ;
    }

  if(seg->gkey==0)
    {
      seg->flag &= ~(FLAG_POS | FLAG_NEG) ;
      seg->flag |= newFlag ;
      graphBoxDraw (box, -1, gridBoxColour (look, box, FALSE)) ;
    }

  look->flag |= FLAG_EDITED ;

  return ;
}


/* n.b. menuBox is an extern from graphcon.c                                 */
static void editFromMenu (MENUITEM item)
{
#if !defined(MACINTOSH)
  int tag = menuGetValue (item) ;
  GridDisp look ;

  look = getGridState("editFromMenu") ;
  
  if (tag > 0)
    gridEdit (look, menuBox, tag, TRUE) ;
  else
    gridEdit (look, menuBox, -tag, FALSE) ;

  gridReport (look, menuBox) ;

  gridSelect (look, menuBox) ;				    /* Make sure title box updated */
#endif
}

/**********************************/

static void colourNeighbours (GridDisp look, int colour)
{
  int i, index ;

  if (look->flag & FLAG_EDIT_MODE)
    return ;

  index = arrp(look->segs, look->activeBox, SEG)->mapIndex ;
  if (!index)
    return ;

  for (i = MIN_LIVE_BOX ; i < arrayMax(look->segs) ; ++i)
    if (arrp(look->segs, i, SEG)->mapIndex == index)
      {
	if (colour)
	  graphBoxDraw (i, -1, colour) ;
	else
	  graphBoxDraw (i, -1, gridBoxColour (look, i, FALSE)) ;
      }
}

static void gridSelect (GridDisp look, int box)
{
  float x1,x2,y1,y2 ;
  SEG *seg ;
  int box2;

  if (look->activeBox == box)
    return ;

  if (look->activeBox)
    { seg = arrp(look->segs, look->activeBox, SEG) ;
      while (gridgKeyFind (look, seg->gkey, &box2))
	graphBoxDraw (box2, BLACK, -1) ;
      colourNeighbours (look, 0) ;
    }

  /* presume that if key not found then lazy But test for gkey = 0*/
  /* also if lazy then only one key one box */

  if(look->activeBox){
    if(seg->gkey == 0){
      graphBoxDraw (look->activeBox, BLACK, -1);
      colourNeighbours (look, 0);
    }
  }
  
  look->activeBox = box ;
  
  seg = arrp(look->segs, look->activeBox, SEG) ;

  while (gridgKeyFind (look, seg->gkey, &box2))
    {
    graphBoxDraw (box2, RED, -1) ;
    }

  /* presume that if key not found then lazy */
  /* also if lazy then only one key one box */

  if(seg->gkey == 0)
    {
    graphBoxDraw (box, RED, -1);
    }

  colourNeighbours (look, LIGHTRED) ;
  graphBoxDim (box, &x1, &y1, &x2, &y2) ;


  graphBoxShift (look->crossBox, x1, y1) ;

  strncpy (look->selectName, gName(seg), 64) ;
  graphCompScrollEntry (0, look->selectName, 0, 0, 0, 0, 0) ; 
  graphEntryDisable () ;
}

/***********************************/

static void gridFollow (GridDisp look)
{
  int index = arrp(look->segs, look->activeBox, SEG)->mapIndex ;
  GRIDMAP *map ;
  static KEYSET kset = 0 ;
  int i, j ;
  int x ;

  if (index)
    { map = arrp(look->map, index-1, GRIDMAP) ;
      kset = keySetReCreate (kset) ;
      j = 0 ;
      for (i = MIN_LIVE_BOX ; i < arrayMax(look->segs) ; ++i)
	if (arrp(look->segs, i, SEG)->mapIndex == index)
	  keySet(kset, j++) = arrp(look->segs, i, SEG)->gkey ;
      keySetSort (kset) ; keySetCompress (kset) ;
      pMapSetHighlightKeySet (kset) ;
      x = 0.5 * (map->x1 + map->x2) ;
      display (map->ctg, KEYMAKE(_VCalcul, (0x800000 + x)), "PMAP") ;
      pMapSetHighlightKeySet (0) ;
    }
  else{
    if(arrp(look->segs,look->activeBox,SEG)->gkey != 0)
      display (arrp(look->segs,look->activeBox,SEG)->gkey, 
	       look->key, 0) ;
    else
      messout("Object does not exist at present");
  }
}

/***********************************/

static int entryCompletion (char *cp, int len)
{
  static KEYSET ks = 0 ;
  int i, k, cplen ;
  SEG *seg ;
  GridDisp look ;

  look = getGridState("entryCompletion") ;
  
  ks = keySetReCreate (ks) ; k = 0 ;
  cplen = strlen (cp) ;
  for (i = MIN_LIVE_BOX ; i < arrayMax(look->segs) ; ++i)
    { seg = arrp(look->segs, i, SEG) ;
      if (!strncmp (gName(seg), cp, cplen))
        keySet(ks, k++) = seg->gkey ;
    }

  mainKeySetComplete(ks, cp, len, FALSE) ;

  return keySetMax (ks) ;
}


static int probeClassCompletion (char *cp, int len)
{
  return ksetClassComplete (cp, len, _VClass, FALSE) ;
}

static int probeCompletion (char *cp, int len)
{
  int result = 0 ;
  int classe ;
  GridDisp look ;

  look = getGridState("probeCompletion") ;

  if ((classe = pickWord2Class (look->probeClassName)))
    result = ksetClassComplete (cp, len, classe, FALSE) ;
  else
    messout(GRID_PREFIX "%s is not a valid class name", look->probeClassName) ;

  return result ;
}

/***********************************/

static void gridPick (int box, double x_unused, double y_unused, int modifier_unused) 
{
  GridDisp look ;

  look = getGridState("gridPick") ;

  switch (box)
    {
    case PROBE_CLASS_NAME_BOX:
      graphCompScrollEntry (0, look->probeClassName, 0, 0, 0, 0, 0) ; 
      break ;
    case PROBE_NAME_BOX:
      graphCompScrollEntry (0, look->probeName, 0, 0, 0, 0, 0) ; 
      break ;
    case SELECT_NAME_BOX:
      graphCompScrollEntry (0, look->selectName, 0, 0, 0, 0, 0) ; 
      break ;
    default:
      if (box >= MIN_LIVE_BOX &&
	  box < arrayMax (look->segs)) /* a seg */
	{
	  if (look->flag & FLAG_EDIT_MODE)
	    {
	      gridEdit (look, box, look->editTag, 
			(look->flag & FLAG_EDIT_NEG) ? FALSE : TRUE) ;
	      gridReport (look, box) ;
	    }
	  else
	    {
	      if (box == look->activeBox)
		gridFollow (look) ;
	      else
		gridReport (look, box) ;
	    }

	  gridSelect (look, box) ;

	  /*     What is this comment by Ian ??, is it a test or not ??      */
	  graphRegister (KEYBOARD,(KeyBoardFunc) gridKeyboard) ;  /* just a test il */
	}
    }
}

/************************************/

static void gridToggleSymbolic (void)
{
  GridDisp look ;

  look = getGridState("gridToggleSymbolic") ;

  look->flag ^= FLAG_NAMES ;
  gridDraw (look) ;

  return ;
}


/************************************/

/* User only allowed to toggle if they can get write access.                 */
/*                                                                           */
static void gridToggleExclusive(void)
{
  GridDisp look ;
  look = getGridState("gridToggleSymbolic") ;

  if (checkWriteAccess())
    {
      OBJ gridObj, viewObj ;
      KEY viewKey ;

      if (!(gridObj = bsCreate (look->key)))
	messcrash(GRID_PREFIX "Can't create Grid object from which to get the"
		  "View object for setting \"exclusiveTags\"") ;

      if (!bsGetKey (gridObj, _View, &viewKey))
	messcrash(GRID_PREFIX "Can't find View key from Grid object for setting \"exclusiveTags\"") ;

      viewObj = bsUpdate(viewKey) ;
      if (viewObj == NULL)
	messcrash(GRID_PREFIX "Can't create View object in which to set \"exclusiveTags\"") ;

      if (look->exclusiveTags)
	{
	  bsGoto(viewObj, 0) ;
	  if (bsFindTag (viewObj, _Grid_exclusive_tags))
	    {
	      if (!bsRemove(viewObj))
		messerror(GRID_PREFIX "Unable to remove \"exclusiveTags\" tag for this grid") ;
	      else
		{
		  look->exclusiveTags = FALSE ;
		  menuSetLabel(look->exclusiveItem, GRID_EXCLUSIVE_ON) ;
		  graphNewMenu(look->gridMenu) ;
		}
	    }
	}
      else
	{
	  if (!bsAddTag (viewObj, _Grid_exclusive_tags))
	      messerror(GRID_PREFIX "Unable to add \"exclusiveTags\" tag for this grid") ;
	  else
	    {
	      look->exclusiveTags = TRUE ;
	      menuSetLabel(look->exclusiveItem, GRID_EXCLUSIVE_OFF) ;
	      graphNewMenu(look->gridMenu) ;
	    }
	}

      bsSave(viewObj) ;
      bsDestroy(viewObj) ;
      bsDestroy(gridObj) ;

    }

  return ;
}

/***********************/

static void gridToggleSmall (void)
{
  GridDisp look ;

  look = getGridState("gridToggleSmall") ;

  look->flag ^= FLAG_SMALL ;
  gridDraw (look) ;
}

/***********************/

static void gridEditMode (void)
{
  int i ;
  GridDisp look ;

  look = getGridState("gridEditMode") ;

  if (look->flag & FLAG_EDIT_MODE)
    return ;

  graphBoxDraw (EDIT_MODE_BOX, WHITE, DARKRED) ;
  graphBoxDraw (MAP_MODE_BOX, BLACK, WHITE) ;

  if (!look->editTag)
    { messout(GRID_PREFIX "There is no tag type set for editing "
	       "- set one in the view object and rebuild") ;
      return ;
    }
  colourNeighbours (look, 0) ;

  look->flag |= FLAG_EDIT_MODE ;

  /* Set edit menus for all grids including the selected box.                */
  if (look->editMenu)
    {
    for (i = MIN_LIVE_BOX ; i < arrayMax(look->segs) ; ++i)
      { graphNewBoxMenu (i, look->editMenu) ;
      }

    }

  /* Once the user starts to edit they cannot flip the exclusive mode.       */
  /* I've done this because I think the user will get in a muddle if they    */
  /* keep toggling the flag between edits.                                   */
  menuSetFlags(look->exclusiveItem, MENUFLAG_DISABLED) ;
  graphNewMenu(look->gridMenu) ;

}

/*************************************************/

static int localRange = 200 ;
static Array posStructArray = 0 ;

static int mapOrder (void *a, void *b)
{
  GRIDMAP *map1 = arrp(posStructArray, *(int*)a, GRIDMAP) ;
  GRIDMAP *map2 = arrp(posStructArray, *(int*)b, GRIDMAP) ;

  if (map1->ctg < map2->ctg)
    return -1 ;
  if (map1->ctg > map2->ctg)
    return 1 ;

  if (map1->x1 < map2->x1)
    return -1 ;
  if (map1->x1 > map2->x1)
    return 1 ;

  return 0 ;
}

void gridCluster (Array posArray, Array mapArray, float range)
{				/* also called from pmapdisp.c */
  GRIDMAP *map, *pos ;
  int i, nmap = -1 ;
  float tmp ;
  static Array ptr ;

  if (!isInitialised) gridInitialise() ;

  if (!arrayExists (mapArray))
    messcrash(GRID_PREFIX "mapArray does not exist in gridCluster") ;
  arrayMax(mapArray) = 0 ;

  if (!arrayMax(posArray))
    return ;
  posStructArray = posArray ;

  ptr = arrayReCreate (ptr, arrayMax(posArray), int) ;
  arrayMax(ptr) = arrayMax(posArray) ;
  for (i = arrayMax(posArray) ; i-- ;)
    { arr(ptr,i,int) = i ;
      if (arrp(posArray, i, GRIDMAP)->x1 > arrp(posArray, i, GRIDMAP)->x2)
	{ tmp = arrp(posArray, i, GRIDMAP)->x1 ;
	  arrp(posArray, i, GRIDMAP)->x1 = arrp(posArray, i, GRIDMAP)->x2 ;
	  arrp(posArray, i, GRIDMAP)->x2 = tmp ;
	}
    }

  arraySort (ptr, mapOrder) ;	/* preserve posArray order */

  for (i = 0 ; i < arrayMax(posArray) ; i++)
    { pos = arrp(posArray, arr(ptr, i, int), GRIDMAP) ;
      if (nmap >= 0 &&
	  pos->ctg == map->ctg && 
	  pos->x1 < map->x2 + range)
	{ map->x1 = pos->x1 ;	/* must be >= after sort */
	  if (pos->x2 < map->x2)
	    map->x2 = pos->x2 ;
	}
      else
	{ map = arrayp(mapArray, ++nmap, GRIDMAP) ;
	  *map = *pos ;
	}
      pos->clump = nmap ;
    }
}

static int nYacHits ;		/* for gridKeySetStats */

BOOL gridClusterKey (KEY key, Array map, float range)
{
  int j, x1, x2 ;
  static Array positives, units ;
  KEY ctg ;
  OBJ obj, Clone ;
  GRIDMAP *pos ;

  if (!isInitialised) gridInitialise() ;

  if (!arrayExists (map))
    return FALSE ;
  arrayMax(map) = 0 ;
  
  if ((obj = bsCreate (key)))
    { units = arrayReCreate (units, 16, BSunit) ;
      if (bsFindTag (obj, _Positive) && 
	  bsFlatten (obj, 2, units))
	{ positives = arrayReCreate (positives, 16, GRIDMAP) ; 
	  for (j = 1 ; j < arrayMax(units) ; j += 2)
	    if ((Clone = bsCreate(arr(units,j,BSunit).k)))
	      { if (bsGetKey (Clone, _pMap, &ctg) &&
		    bsGetData (Clone, _bsRight, _Int, &x1) &&
		    bsGetData (Clone, _bsRight, _Int, &x2))
		  { pos = arrayp (positives, 
				  arrayMax(positives), GRIDMAP) ;
		    pos->ctg = ctg ;
		    pos->x1 = x1 ;
		    pos->x2 = x2 ;
		    ++nYacHits ;
		  }
		bsDestroy (Clone) ;
	      }
	  gridCluster (positives, map, range) ;
	}
      bsDestroy (obj) ;
    }

  return (arrayMax (map) > 0) ;
}

static void gridSetRange (void)
{
  GridDisp look ;
  ACEIN range_in;

  look = getGridState("gridSetRange") ;

  if ((range_in = messPrompt ("Give range in pmap bands for clustering into single loci",
			      messprintf ("%d", localRange), "iz", 0)))
    { 
      aceInInt (range_in, &localRange) ;
      aceInDestroy (range_in);

      if (!(look->flag & FLAG_EDIT_MODE))
	gridMapMode () ;
    }
}

/*******************************************************************/

static void gridKeySetStats (void)
{
  int i ;
  static Array map ;
  KEYSET kset = 0 ;
  int nClones = 0, nSingle = 0, nLoci = 0 ;

  map = arrayReCreate (map, 16, GRIDMAP) ;
 
  if (!keySetActive(&kset, 0))
    { messout("First select a keySet window, thank you.") ;
      return  ;
    }
  nYacHits = 0 ;

  for (i = 0 ; i < keySetMax(kset) ; ++i)
    if (gridClusterKey (keySet(kset,i), map, localRange))
      { ++nClones ;
	nLoci += arrayMax(map) ;
	if (arrayMax(map) == 1)
	  ++nSingle ;
      }
  
  messout(GRID_PREFIX "%d clones, %d Yac hits, %d loci, %d singles\n",
	   nClones, nYacHits, nLoci, nSingle) ;
}

/**************************************************/

static void gridKeySetApply (void)
{
  int i, j ;
  KEYSET kset = 0 ;
  GridDisp look ;

  look = getGridState("gridKeySetApply") ;
 
  if (!keySetActive(&kset, 0))
    { messout("First select a keySet window, thank you.") ;
      return  ;
    }

  for (i = 0 ; i < keySetMax(kset) ; ++i)
    { for (j = MIN_LIVE_BOX ; j < arrayMax(look->segs) ; ++j)
	if (arrp(look->segs, j, SEG)->gkey == keySet(kset,i))
	  arrp(look->segs, j, SEG)->flag |= FLAG_SELECTED ;
      gridHybBuild (look, keySet(kset,i)) ;
    }

  gridDraw (look) ;
}

/*******************************/

static void gridMapMode (void)
{
  int i, x1, x2, npositives = 0 ;
  static Array positives, reverse ;
  SEG *seg ;
  GRIDMAP *pos ;
  OBJ obj ;
  GridDisp look ;

  look = getGridState("gridMap") ;

  graphBoxDraw (MAP_MODE_BOX, WHITE, DARKRED) ;
  graphBoxDraw (EDIT_MODE_BOX, BLACK, WHITE) ;

  if (!(look->flag & FLAG_EDIT_MODE))
    return ;

  look->flag &= ~FLAG_EDIT_MODE ;

  positives = arrayReCreate (positives, 16, GRIDMAP) ; 
  reverse = arrayReCreate (reverse, 16, int*) ;

  for (i = MIN_LIVE_BOX ; i < arrayMax(look->segs) ; ++i)
    { seg = arrp(look->segs, i, SEG) ;
      seg->mapIndex = 0 ;
      if ((seg->flag & FLAG_POS) && (obj = bsCreate(seg->gkey)))
	{ pos = arrayp (positives, npositives, GRIDMAP) ;
	  array (reverse, npositives, int*) = &(seg->mapIndex) ;
	  if (bsGetKey (obj, _pMap, &pos->ctg) &&
	      bsGetData (obj, _bsRight, _Int, &x1) &&
	      bsGetData (obj, _bsRight, _Int, &x2))
	    { pos->x1 = x1 ;
	      pos->x2 = x2 ;
	      ++npositives ;
	    }
	  else
	    --arrayMax(positives) ;
	  bsDestroy (obj) ;
	}
      graphNewBoxMenu (i, 0) ;
    }

  gridCluster (positives, look->map, localRange) ;
  for (i = 0 ; i < arrayMax(positives) ; ++i)
    *(arr(reverse, i, int*)) = 
      arrp(positives, i, GRIDMAP)->clump + 1 ;
}

/*********************************/

static void gridHybBuild (GridDisp look, KEY probe)
{
  int i, box ;
  TAB *tab ;
  OBJ obj ;
  BOOL isDefNeg = FALSE ;
  Array column = arrayCreate (24, BSunit) ;

  if ((obj = bsCreate (probe)))
    { if (bsFindTag (obj, _Contains) && bsFlatten (obj, 2, column))
	for (i = 0 ; i < arrayMax(column) ; i += 2)
	  gridHybBuild (look, arr(column, i+1, BSunit).k) ;
      if (bsFindTag (obj, _Grid_data) && bsFlatten (obj, 2, column))
	for (i = 0 ; i < arrayMax(column) ; i += 2)
	  gridHybBuild (look, arr(column, i+1, BSunit).k) ;
      if (bsFindTag (obj, _Default_negative))
	{ for (i = 0 ; i < arrayMax(look->segs) ; ++i)
	    arrp(look->segs, i, SEG)->flag &= ~FLAG_TEMP ;
	  isDefNeg = TRUE ;
	}
      if (bsFindTag (obj, _Positive) && bsFlatten (obj, 2, column))
	for (i = 0 ; i < arrayMax(column) ; i += 2)
	  if (gridgKeyFind(look,arr(column, i+1, BSunit).k, &box))
	    { tab = arrayp(look->tab, arrayMax(look->tab), TAB) ;
	      tab->probe = probe ;
	      tab->tag = arr(column, i, BSunit).k ;
	      tab->target = arr(column, i+1, BSunit).k ;
	      tab->flag = FLAG_POS ;
	      arrp(look->segs, box, SEG)->flag |= FLAG_POS | FLAG_TEMP ;
	      while (gridgKeyFind(look,arr(column, i+1, BSunit).k, &box))
		arrp(look->segs, box, SEG)->flag |= FLAG_POS | FLAG_TEMP ;
	    }
      if (bsFindTag (obj, _Negative) && bsFlatten (obj, 2, column))
	for (i = 0 ; i < arrayMax(column) ; i += 2)
	  if (gridgKeyFind(look,arr(column, i+1, BSunit).k, &box))
	    { tab = arrayp(look->tab, arrayMax(look->tab), TAB) ;
	      tab->probe = probe ;
	      tab->tag = arr(column, i, BSunit).k ;
	      tab->target = arr(column, i+1, BSunit).k ;
	      tab->flag = FLAG_NEG ;
	      arrp(look->segs, box, SEG)->flag |= FLAG_NEG | FLAG_TEMP ;
	      while (gridgKeyFind(look, arr(column, i+1, BSunit).k, &box))
		arrp(look->segs, box, SEG)->flag |= FLAG_NEG | FLAG_TEMP ;
	    }
      if (bsFindTag (obj, _Conflict) && bsFlatten (obj, 2, column))
	for (i = 0 ; i < arrayMax(column) ; i += 2)
	  if (gridgKeyFind(look,arr(column, i+1, BSunit).k, &box))
	    { tab = arrayp(look->tab, arrayMax(look->tab), TAB) ;
	      tab->probe = probe ;
	      tab->tag = arr(column, i, BSunit).k ;
	      tab->target = arr(column, i+1, BSunit).k ;
	      tab->flag = FLAG_POS | FLAG_NEG ;
	      arrp(look->segs, box, SEG)->flag |= 
		FLAG_POS | FLAG_NEG | FLAG_TEMP ;
	      while (gridgKeyFind(look, arr(column, i+1, BSunit).k, &box))
		arrp(look->segs, box, SEG)->flag |= 
		  FLAG_POS | FLAG_NEG | FLAG_TEMP ;
	    }
      if (isDefNeg)
	for (i = 0 ; i < arrayMax(look->segs) ; ++i)
	  if (!(arrp(look->segs, i, SEG)->flag & FLAG_TEMP))
	    arrp(look->segs, i, SEG)->flag |= FLAG_NEG ;
      bsDestroy (obj) ;
    }

  arrayDestroy (column) ;
}

static void gridProbeDisplay (GridDisp look, KEY probe)
{
  int i ;
  BOOL empty = TRUE ;

  look->probe = probe ;
  strncpy (look->probeClassName, className(probe), 63) ;
  strncpy (look->probeName, name(probe), 63) ;

				/* clear hybridisation flags */
  for (i = MIN_LIVE_BOX ; i < arrayMax(look->segs) ; ++i)
    arrp(look->segs, i, SEG)->flag &= ~(FLAG_POS | FLAG_NEG) ;

  arrayMax(look->tab) = 0 ;	/* clear hit table */

  gridHybBuild (look, probe) ; 

  for (i = MIN_LIVE_BOX ; i < arrayMax(look->segs) ; ++i)
    {
      if (arrp(look->segs, i, SEG)->flag & (FLAG_POS | FLAG_NEG))
	empty = FALSE ;
  /*  if (empty)
    messout(GRID_PREFIX "Sorry - no positives or negatives of %s on this grid", 
	     name (probe)) ;
	     */
    }

  gridDraw (look) ;

  return ;
}

static void gridProbeEntry (char* string)
{
  int classe ;
  KEY probe = 0 ;
  GridDisp look ;

  look = getGridState("gridProbeEntry") ;

  gridMapMode () ;

  if (!(classe = pickWord2Class (look->probeClassName)))
    messout(GRID_PREFIX "Sorry, %s is not a valid class name", look->probeClassName) ;
  else if (!lexword2key (string, &probe, classe))
    messout(GRID_PREFIX "Sorry, %s is not in class %s", string, look->probeClassName) ;
  else 
    gridProbeDisplay (look, probe) ;
}

static void probeClassEntry (char* string)
{
  GridDisp look ;

  look = getGridState("probeClassEntry") ;

  if (!pickWord2Class (string))
    messout(GRID_PREFIX "Sorry, %s is not a valid class name", string) ;
  else
    graphCompScrollEntry (0, look->probeName, 0, 0, 0, 0, 0) ;
}

static void gridProbeAction (KEY key)
{
  GridDisp look ;

  look = getGridState("gridProbeAction") ;

  gridMapMode () ;
  gridProbeDisplay (look, key) ;
  displayRepeatBlock () ;
}

static void gridProbeButton (void)
{
  displayBlock (gridProbeAction, 
		"The hybridisation pattern for that object will be shown.\n"
		"Remove this message to cancel.") ;
}

/*************************************************/

static void gridClear (void)
{
  int i ;
  GridDisp look ;

  look = getGridState("gridClear") ;

  if ((look->flag & FLAG_EDITED) && isWriteAccess() && look->probe && 
      messQuery ("Do you want to save the current pattern?"))
    gridSave () ;

  for (i = MIN_LIVE_BOX ; i < arrayMax(look->segs) ; ++i)
    arrp(look->segs, i, SEG)->flag &= 
      ~(FLAG_POS | FLAG_NEG | FLAG_SELECTED) ;

  arrayMax(look->tab) = 0 ;
  look->probe = 0 ;
  *look->probeName = 0 ;
  look->flag &= ~FLAG_EDITED ;

  colourNeighbours (look, 0) ;
  gridDraw (look) ;
}

/************************************/

static void gridKeyboard (int k, int modifier_unused)
{
  int i, ibest ;
  float x, y, best ;
  SEG *seg ;
  GridDisp look ;

  look = getGridState("gridKeyboard") ;

  if (!look->activeBox)
    return ;

  if (k == RETURN_KEY)
    { if (look->flag & FLAG_EDIT_MODE)
	gridEdit (look, look->activeBox, look->editTag,
		  (look->flag & FLAG_EDIT_NEG) ? FALSE : TRUE) ;
      else
	gridFollow (look) ;
      return ;
    }

  seg = arrp(look->segs, look->activeBox, SEG) ;
  x = seg->x ;  y = seg->y ;
  ibest = 0 ;
  if (k == LEFT_KEY || k == UP_KEY)
    best = -1000.0 ;
  else
    best = 1000.0 ;

  for (i = MIN_LIVE_BOX ; i < arrayMax(look->segs) ; ++i)
    { seg = arrp(look->segs, i, SEG) ;
      switch (k)
      {
      case LEFT_KEY:
	if (seg->y < y+0.5 && seg->y > y-0.5 && seg->x < x && seg->x > best)
	  { best = seg->x ; ibest = i ; }
	break ;
      case RIGHT_KEY:
	if (seg->y < y+0.5 && seg->y > y-0.5 && seg->x > x && seg->x < best)
	  { best = seg->x ; ibest = i ; }
	break ;
      case UP_KEY:
	if (seg->x < x+1 && seg->x > x-1 && seg->y < y && seg->y > best)
	  { best = seg->y ; ibest = i ; }
	break ;
      case DOWN_KEY:
	if (seg->x < x+1 && seg->x > x-1 && seg->y > y && seg->y < best)
	  { best = seg->y ; ibest = i ; }
	break ;
      }
    }
  if (ibest)
    {
      gridReport (look, ibest) ;
      gridSelect (look, ibest) ;
    }
  graphRegister (KEYBOARD,(KeyBoardFunc) gridKeyboard) ;  /* just a test il */

  return ;
}


static void gridTreeProbe (void)
{
  GridDisp look ;

  look = getGridState("gridTreeProbe") ;

  if (look->probe)
    display (look->probe, look->key, "TREE") ;
}


/*************************************************/
/************* conversion and drawing ************/

static void setView (GridDisp look, KEY view)
{
  OBJ View ;
  int priority ;
  unsigned int pack ;
  char *text ;
  KEY tag, col, scol ;
  MENUITEM item ;
  static BSMARK mark = 0 ;


  View = bsCreate(view) ;
  if (View)
    {
      look->tagValues = arrayCreate (16, KEY) ;

      /* If the model contains the exclusive tag then check to see if this   */
      /* particular grid has it.                                             */
      /* If the tag is not in the model then disable the toggle menu item.   */
      /* N.B. default is FALSE so old databases continue as before.          */
      if (bsIsTagInObj(0, view, _Grid_exclusive_tags))
	{
	  look->flag |= FLAG_EXCLUSIVETAGS ;
	  if (bsFindTag (View, _Grid_exclusive_tags))
	    {
	      look->exclusiveTags = TRUE ;
	      menuSetLabel(look->exclusiveItem, GRID_EXCLUSIVE_OFF) ;
	    }
	  else
	    look->exclusiveTags = FALSE ;
	}
      else
	{
	  look->flag &= ~FLAG_EXCLUSIVETAGS ;
	  menuSetFlags(look->exclusiveItem, MENUFLAG_DISABLED) ;
	  graphNewMenu(look->gridMenu) ;
	  look->exclusiveTags = FALSE ;
	}

      if (bsGetData (View, _Grid_edit_default, _Text, &text))
	{
	  if (lexaddkey (text, &look->editTag, 0))
	    messout(GRID_PREFIX "Warning: edit tag %s in View %s not known previously", 
		    text, name(view)) ;
	  if (bsGetData (View, _bsRight, _Text, &text) &&
	      !lexstrcmp (text, "negative")) /* case insensitive */
	    look->flag |= FLAG_EDIT_NEG ;
	}


      if (bsGetData (View, _Grid_edit_menu, _Text, &text)) 
	{
	  MENUITEM tmp_item = NULL ;

	  if (look->editMenu)
	    menuDestroy (look->editMenu) ;
	  look->editMenu = menuCreate ("Tags") ;

	  do
	    { 
	      mark = bsMark (View, mark) ;
	      if (lexaddkey (text, &tag, 0))
		messout(GRID_PREFIX "Warning: edit tag %s in View %s not known previously", 
			text, name(view)) ;
	      item = menuCreateItem (name(tag), editFromMenu) ;
	      if (bsGetData (View, _bsRight, _Text, &text) &&
		  !lexstrcmp (text, "negative")) /* case insensitive */
		tag = -tag ;
	      menuSetValue (item, tag) ;

	      if (look->exclusiveTags == TRUE)
		{
		  if (tmp_item == NULL)
		    menuSetFlags (item, MENUFLAG_START_RADIO) ;
		}
	      else
		menuSetFlags (item, MENUFLAG_TOGGLE) ;
	      tmp_item = item ;

	      menuAddItem (look->editMenu, item, 0) ;
	      bsGoto (View, mark) ;
	    }
	  while (bsGetData (View, _bsDown, _Text, &text)) ;

	  if (look->exclusiveTags == TRUE)
	    menuSetFlags(tmp_item, MENUFLAG_END_RADIO) ;
	}

      if (bsGetData (View, _Grid_map, _Int, &priority))
	{
	  int i = 0 ;
	  do
	    {
	      mark = bsMark (View, mark) ;
	      bsPushObj (View) ;
	      
	      if (priority > 127)
		{ priority = 127 ; messout(GRID_PREFIX "Grid_map value must be < 128") ; }
	      if (priority < -128)
		{ priority = -128 ; messout(GRID_PREFIX "Grid_map value must be >= -128") ; }
	      
	      if (!bsGetKeyTags (View, _Colour, &col))
		messout(GRID_PREFIX "Warning: view %s Grid_map %d should specify colour",
			name(view), priority) ;
	      col -= _WHITE ;
	      if (bsGetKeyTags (View, _Surround_colour, &scol))
		scol -= _WHITE ;
	      else
		scol = col ;
	      
	      pack = GRID_PACK(priority, col, scol) ;
	      
	      if (bsGetData (View, _Tag, _Text, &text))
		do
		  { if (lexaddkey (text, &tag, 0))
		    messout(GRID_PREFIX "Warning: tag %s in View %s not known previously", 
			    text, name(view)) ;
		  if (!assInsert (look->tagMap, assVoid(tag), assVoid(pack)))
		    messout(GRID_PREFIX "Warning: tag %s assigned twice in View %s",
			    text, name(view)) ;

		  array(look->tagValues, i, KEY) = tag ;
		  i++ ;
		  }
		while (bsGetData (View, _bsDown, _Text, &text)) ;

	      bsGoto (View, mark) ;
	    }
	  while (bsGetData (View, _bsDown, _Int, &priority)) ;
	}
      else
	messout(GRID_PREFIX "Warning: no tag colour/priority information in View %s",
		name(view)) ;

      bsDestroy (View) ;
    }

  return ;
}


static void gridConvert (GridDisp look)
{ 
  int i, row, n ;
  OBJ obj, subgrid ;
  KEY key ;
  SEG *seg ;
  BOOL isMixed ;
  char *text,*prefix ;  /* Martin Ferguson - for generic axis labelling */
  static BSMARK mark = 0, submark = 0 ;
  KEY _XY_labelling, _Label_per_box, _Squashed, _Square_size, _Box_size ; 
  char *textptr;

  if (!(obj = bsCreate (look->key)))
    messcrash(GRID_PREFIX "Can't make object in gridConvert") ;

  if (bsGetKey (obj, _Title, &key))
    look->title = name (key) ;
  else
    look->title = name (look->key) ;


  look->flag = 0 ;

  if (!bsFindTag (obj, _No_stagger))
    look->flag |= FLAG_STAGGER ;

  if (bsFindTag (obj, _A1_labelling))
    { look->flag |= FLAG_LABELS ;
      look->flag |= FLAG_A1 ;
    }

  lexaddkey ("high_density", &_Squashed, 0) ;
  if (bsFindTag(obj, _Squashed))
    look->flag |= FLAG_SQUASHED;

  /* Check for setting of grid square/box sizes. Size applies to both small  */
  /* and normal views of grid, small is just 0.5 times normal size.          */
  /* Dimensions must be positive, no checking otherwise.                     */
  /* If height is omitted it defaults to width.                              */
  lexaddkey ("Square_size", &_Square_size, 0) ;
  if (bsFindTag (obj, _Square_size))
    {
      float dimension = 0 ;

      if (bsGetData(obj, _bsRight, _Float, &dimension) && dimension > 0)
	{
	  look->normal.horiz_space = dimension ;
	  look->normal.vert_space = look->normal.horiz_space ;

	  dimension = 0 ;
	  if (bsGetData(obj, _bsRight, _Float, &dimension)&& dimension > 0)
	    look->normal.vert_space = dimension ;

	  look->small.horiz_space = (look->normal.horiz_space * GRID_REDUCE) ;
	  look->small.vert_space = (look->normal.horiz_space * GRID_REDUCE) ;
	}
    }

  lexaddkey ("Box_size", &_Box_size, 0) ;
  if (bsFindTag (obj, _Box_size))
    {
      float dimension = 0 ;

      if (bsGetData(obj, _bsRight, _Float, &dimension) && dimension > 0)
	{
	  look->normal.box_width = dimension ;
	  look->normal.box_height = look->normal.box_width ;

	  dimension = 0 ;
	  if (bsGetData(obj, _bsRight, _Float, &dimension)&& dimension > 0)
	    look->normal.box_height = dimension ;

	  look->small.box_width = (look->normal.box_width * GRID_REDUCE) ;
	  look->small.box_height = (look->normal.box_height * GRID_REDUCE) ;
	}
    }

  clampBoxSize(&(look->small)) ;
  clampBoxSize(&(look->normal)) ;

  lexaddkey ("Label_per_box", &_Label_per_box, 0) ;
  if (bsFindTag(obj, _Label_per_box))
    look->flag |= FLAG_LABEL_PER_BOX ;

  lexaddkey ("XY_labelling", &_XY_labelling, 0) ;
  if (bsFindTag (obj, _XY_labelling))    /* Martin Ferguson - labelling */
    {
      BOOL isGood = TRUE ;

      if (bsGetData (obj, _bsRight, _Text, &text) &&  /* X axis */
	  text[0] >= '0' && text[0] <= '9' &&
	  text[1] >= '0' && text[1] <= '9' &&
	  text[2] == '-' && 
	  text[3] >= '0' && text[3] <= '9' &&
	  text[4] >= '0' && text[4] <= '9')
	strcpy(look->genXLabel, text) ;
      else
	{ messout(GRID_PREFIX "The XY_labelling format must be nn-nn (numeric) or "
		   "nn-nnA (alphabetic), e.g. for A1_labelling use "
		   "XY_labelling 01-12 01-08A") ;
	  isGood = FALSE ;
	}
      if (bsGetData (obj, _bsRight, _Text, &text) &&  /* X axis */
	  text[0] >= '0' && text[0] <= '9' &&
	  text[1] >= '0' && text[1] <= '9' &&
	  text[2] == '-' && 
	  text[3] >= '0' && text[3] <= '9' &&
	  text[4] >= '0' && text[4] <= '9')
	strcpy(look->genYLabel, text) ;
      else
	{ messout(GRID_PREFIX "The XY_labelling format must be nn-nn (numeric) or "
		   "nn-nnA (alphabetic), e.g. for A1_labelling use "
		   "XY_labelling 01-12 01-08A") ;
	  isGood = FALSE ;
	}
      if (isGood)
	{ look->flag |= FLAG_LABELS ;
	  look->flag |= FLAG_XY ;
	}
    }

  look->dxLine = look->dyLine = look->dxSpace = look->dySpace = 0 ;
  bsGetData (obj, _Lines_at, _Int, &look->dxLine) ;
  bsGetData (obj, _bsRight,  _Int, &look->dyLine) ;
  bsGetData (obj, _Space_at, _Int, &look->dxSpace) ;
  bsGetData (obj, _bsRight,  _Int, &look->dySpace) ;
  if (look->dxLine == 1 && look->dyLine == 1)
    look->flag |= FLAG_NAMES ;

  look->segs = arrayReCreate (look->segs, 512, SEG) ;

  n = MIN_LIVE_BOX ;
  if (bsGetData (obj, _Row, _Int, &row))
    {
      do
	{
	  mark = bsMark (obj, mark) ;
	  row-- ;
	  i = 0 ; 
	  if (!bsGetKeyTags (obj, _bsRight, &key)) /* class or Mixed */
	    continue ;
	  isMixed = (key == _Mixed) ;
	  while (bsGetKeyTags (obj, _bsRight, &key)) /* class if Mixed */
	    {
	      if (isMixed && !bsGetKey (obj, _bsRight, &key)) /* key itself */
		break ;
	      seg = arrayp (look->segs, n, SEG) ;
	      seg->flag = seg->flagStore = 0 ;
	      if (!strcmp (name(key),"-"))
		seg->flag |= FLAG_BLANK ;
	      seg->gkey = key ;
	      seg->x = i ;
	      seg->y = row ;
	      seg->name =0;
	      ++n ; ++i ;
	    }
	  bsGoto (obj, mark) ;
	} while (bsGetData (obj, _bsDown, _Int, &row)) ;
    }
  else if (bsGetData (obj, _Virtual_row, _Int, &row))
    {
    do
      {
	int xblock = 0, yblock = row-1 ;

	mark = bsMark (obj, mark) ;
	while (bsGetKey (obj, _bsRight, &key)) /* key is a clone_grid */
	  {
	    if ((subgrid = bsCreate (key)))
	      {
		if(!strcmp(name(key),"-"))
		  lazySegBuild(look,&n,prefix,TRUE);
		if(bsFindTag(subgrid,str2tag("Lazy_grid")))
		  {
		    if (bsFindTag(subgrid, str2tag("Lazy_template")))
		      {
			KEY template;
			if (bsGetKeyTags(subgrid, _bsRight, &template) &&
			    bsGetKeyTags(subgrid, _bsRight, &template))
			  look->template = template;
		      }
		    if(bsGetData(subgrid,str2tag("Prefix"),_Text, &textptr))
		      {
			prefix = strnew(textptr,0);
		      }
		    else
		      {
			prefix = NULL;
		      }
		    lazySegBuild(look,&n,prefix,FALSE);
		  }	    
		else if (bsGetData (subgrid, _Row, _Int, &row)) do
		  {
		    submark = bsMark (subgrid, submark) ;
	      row-- ;
	      i = 0 ;
	      if (!bsGetKeyTags (subgrid, _bsRight, &key))
		continue ;
	      isMixed = (key == _Mixed) ;
	      while (bsGetKeyTags (subgrid, _bsRight, &key))
		{ if (isMixed && !bsGetKey (subgrid, _bsRight, &key))
		  break ;
		seg = arrayp (look->segs, n, SEG) ;
		seg->flag = seg->flagStore = 0 ; 
		if (!strcmp (name(key),"-"))
		  seg->flag |= FLAG_BLANK ;
		seg->gkey = key ;
		seg->x = xblock + i*look->dxLine ;
		seg->y = yblock + row*look->dyLine ;
		seg->name =0;
		++n ; ++i ;
		}
	      bsGoto (subgrid, submark) ;
	      } while (bsGetData (subgrid, _bsDown, _Int, &row)) ;
	    bsDestroy (subgrid) ;
	    look->flag |= FLAG_VIRTUAL ;
	  }
	  else
	    messout(GRID_PREFIX "Can't open subgrid %s of Grid %s", 
		    name(key), name(look->key)) ;
	  ++xblock ;
	  }
	bsGoto (obj, mark) ;
      } while (bsGetData (obj, _bsDown, _Int, &row)) ;
    }
  else if(bsFindTag(obj,str2tag("Lazy_grid")))
    {
      if(bsGetData(obj,str2tag("Prefix"),_Text, &textptr))
	{
	  look->prefix = strnew(textptr,0);
	}
      else
	{
	  look->prefix = NULL;
	}
      if (bsFindTag(obj, str2tag("Lazy_template")))
	{
	  KEY template;
	  if (bsGetKeyTags(obj, _bsRight, &template) &&
	      bsGetKeyTags(obj, _bsRight, &template))
	    look->template = template;
	}
      look->segs = arrayReCreate (look->segs, 512, SEG) ;
      look->dxLine = look->dyLine = 1;
      lazySegBuild(look,&n,look->prefix,FALSE);
    }
  else
    {
      messout(GRID_PREFIX "No clones in Grid %s", name(look->key)) ;
    }
  
  if (bsGetKey (obj, _View, &key))
    setView (look, key) ;

  bsDestroy (obj) ;

  return ;
}

/***************************************/

static void gridSpaceSet(void)
{
  GridDisp look ;
  ACEIN size_in;
  char *size_str = NULL ;
  char *dialog_title = NULL ;
  float *width = NULL, *height = NULL ;
  
  look = getGridState("gridSpaceSet") ;

  if (look->flag & FLAG_SMALL)
    {
      width = &(look->small.horiz_space) ;
      height = &(look->small.vert_space) ;
      size_str = "Small" ;
    }
  else
    {
      width = &(look->normal.horiz_space) ;
      height = &(look->normal.vert_space) ;
      size_str = "Normal" ;
    }

  dialog_title = hprintf(0, "%s box spacing - width, height:", size_str) ;

  if ((size_in = messPrompt(dialog_title,
			    messprintf("%.3f %.3f", *width, *height),
			    "ffz", 0)))
    { 
      aceInFloat (size_in, width) ;
      aceInFloat (size_in, height) ;
      gridDraw(look) ;
    }

  messfree(dialog_title) ;

  return ;
}


/***************************************/

static void gridSizeSet(void)
{
  GridDisp look ;
  ACEIN size_in;
  char *size_str = NULL ;
  char *dialog_title = NULL ;
  float *width = NULL, *height = NULL ;
  
  look = getGridState("gridSizeSet") ;

  if (look->flag & FLAG_SMALL)
    {
      width = &(look->small.box_width) ;
      height = &(look->small.box_height) ;
      size_str = "Small" ;
    }
  else
    {
      width = &(look->normal.box_width) ;
      height = &(look->normal.box_height) ;
      size_str = "Normal" ;
    }

  dialog_title = hprintf(0, "%s box size - width, height:", size_str) ;

  if ((size_in = messPrompt(dialog_title,
			    messprintf("%.3f %.3f", *width, *height),
			    "ffz", 0)))
    { 
      aceInFloat (size_in, width) ;
      aceInFloat (size_in, height) ;
      gridDraw(look) ;
    }

  messfree(dialog_title) ;

  return ;
}


/***************************************/

/* Ensures that box size is consistent with square size.                     */
static void clampBoxSize(GridDimension grid_box)
{
  if (grid_box->box_width > grid_box->horiz_space)
    grid_box->box_width = grid_box->horiz_space ;

  if (grid_box->box_height > grid_box->vert_space)
    grid_box->box_height = grid_box->vert_space ;

  return ;
}


/***************************************/

static void gridDraw (GridDisp look)
{
  float   xmax = 0, ymax = 0 ;
  int   i ;
  float x, y ;
  SEG   *seg ;
  float yOff = 0.5 ;
  float xOff = 2 ;
  BOOL  isSymbolic = !(look->flag & FLAG_NAMES) ;
  BOOL  isSquashed = look->flag & FLAG_SQUASHED; 
  float itemWidth, itemHeight ;
  float box_width, box_height ;
  float dxL = 0.5 ;
  float dyL = 0.5 ;

  /* NOTICE that (int) castings are vital because these macros may be used   */
  /* with a float.                                                           */
#define i2x(z)	((z)*itemWidth + \
		 (look->dxLine ? dxL*(int)((z)/look->dxLine) : 0) + \
		 (look->dxSpace ? 2*(int)((z)/look->dxSpace) : 0))

#define j2y(z)	((z)*itemHeight + \
		 (look->dyLine ? dyL*(int)((z)/look->dyLine) : 0) + \
		 (look->dySpace ? (int)((z)/look->dySpace) : 0))
  

  if (look->flag & FLAG_NAMES)
    {
      itemWidth = 1;
      for ( i = MIN_LIVE_BOX; i < arrayMax(look->segs); i++)
	{
	  seg = arrp(look->segs, i, SEG);
	  if (!(seg->flag & FLAG_BLANK))
	    {
	      int n = strlen(gName(seg));
	      if (n > itemWidth)
		itemWidth = n;
	      if (seg->x > xmax)
		xmax = seg->x;
	      if (seg->y > ymax)
		ymax = seg->y;
	    }
	}
      if (itemWidth > 25)
	itemWidth = 25; /* sanity check */
      itemWidth++; /* gap */
      itemHeight = 1 ; 
      box_width = box_height = 0.0 ;
    }
  else if (look->flag & FLAG_SMALL)
    {
      itemWidth = look->small.horiz_space ;
      itemHeight = look->small.vert_space ; 
      box_width = box_height = 0.0 ;
      dxL = dyL = 0.0 ; 
    }
  else if (isSquashed)
    {
      itemWidth = 1.2 ;
      itemHeight = 1.0 ;
      box_width = itemWidth * 0.9 ;
      box_height = itemHeight * 0.9 ;
    }
  else
    {
      clampBoxSize(&(look->normal)) ;

      itemWidth = look->normal.horiz_space ;
      itemHeight = look->normal.vert_space ;
      box_width = look->normal.box_width ;
      box_height = look->normal.box_height ;
    }

  graphClear () ;

  graphRetitle (messprintf ("%s:%s", className(look->key), name (look->key))) ;

  graphText (look->title, 1, yOff) ; /* title */

  x = 3 + strlen(look->title) ;
  if (graphButton ("Gridded item:", gridSelectButton, x, yOff) 
      != SELECT_BUTTON_BOX)
    messcrash(GRID_PREFIX "box screwup at %d in gridDraw", SELECT_BUTTON_BOX) ;
  *look->selectName = 0 ;
  if (graphCompScrollEntry (entryCompletion, look->selectName, 
			    64, 10, x+15, yOff+0.2, gridSelectEntry)
      != SELECT_NAME_BOX)
    messcrash(GRID_PREFIX "box screwup at %d in gridDraw", SELECT_NAME_BOX) ;

  if (graphButton ("Clear", gridClear, x+27, yOff) != CLEAR_BOX)
    messcrash(GRID_PREFIX "box screwup at %d in gridDraw", CLEAR_BOX) ;


  yOff += 1.6 ;

  x = 1 ;
  if (graphButton ("Probe:", gridProbeButton, x, yOff) 
      != PROBE_BUTTON_BOX)
    messcrash(GRID_PREFIX "box screwup at %d in gridDraw", PROBE_BUTTON_BOX) ;
  if (graphCompScrollEntry (probeClassCompletion, look->probeClassName,
			    64, 10, x+8, yOff+0.2, probeClassEntry)
      != PROBE_CLASS_NAME_BOX)
    messcrash(GRID_PREFIX "box screwup at %d in gridDraw", PROBE_BUTTON_BOX) ;
  graphText (":", x+19, yOff+0.2) ;
  if (graphCompScrollEntry(probeCompletion, look->probeName, 
			   64, 18, x+20, yOff+0.2, gridProbeEntry) != PROBE_NAME_BOX)
    messcrash(GRID_PREFIX "box screwup at %d in gridDraw", PROBE_NAME_BOX) ;

  x += 34 ;
  if (graphButton ("Edit Mode", gridEditMode, x+6, yOff) != EDIT_MODE_BOX)
    messcrash(GRID_PREFIX "box screwup at %d in gridDraw", EDIT_MODE_BOX) ;
  if (graphButton ("Map Mode", gridMapMode, x+17, yOff) != MAP_MODE_BOX)
    messcrash(GRID_PREFIX "box screwup at %d in gridDraw", MAP_MODE_BOX) ;
  if (look->flag & FLAG_EDIT_MODE)
    gridEditMode () ;
  else
    gridMapMode () ;

  yOff += 1.6 ;
  if (graphBoxStart() != REPORT_BOX)
    messcrash(GRID_PREFIX "box screwup at %d in gridDraw", REPORT_BOX) ;
  graphTextPtr (look->reportBuffer, 0, yOff, 1000) ;
  graphBoxEnd() ;
  graphBoxDraw (REPORT_BOX, BLACK, PALECYAN) ;

  yOff += 1.4 ;						    /* Finish off grid header area. */
  graphLine (0, yOff, 300, yOff) ;


  /* Draw the actual grid (maybe with labels).                               */
  yOff += 0.5 ; 

  /* This section of code now handles both the A1_labelling and XY_labelling.
     I've written it to label in any arbitrary fashion.  A1_labelling is just a
     specific case.  */
  if (look->flag & FLAG_LABELS)              /* Yes, there are grid axis labels. */
    {
      int xstart, xend, ystart, yend, temp ; /* Start and end values for x-axis and y-axis labels. */
      BOOL XAlpha = FALSE, YAlpha = FALSE ;  /* Whether axis labels are alphabetic or numeric. */
      int xincr = 0, yincr = 0 ;             /* Subtraction value if labeling is descending. */
      static char la[] = 
	"*ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz"; /* for labels */

      if (look->flag & FLAG_A1)
	{
	  /* A1_labelling - as for Bentley/Dunham grids */
	  xstart = 1 ; 
	  xend = 12;

	  ystart = 1 ;
	  yend = 8;
	  YAlpha = TRUE ;
	}
      else if (look->flag & FLAG_XY)
	{
	  /* For XY_labelling, values specified in data. */
	  xstart = atoi(&look->genXLabel[0]) ;
	  xend = atoi(&look->genXLabel[3]) ;
	  
	  ystart = atoi(&look->genYLabel[0]) ;
	  yend = atoi(&look->genYLabel[3]) ;
	
	  if (look->genXLabel[5] == 'A') XAlpha = TRUE ;
	  if (look->genYLabel[5] == 'A') YAlpha = TRUE ;
	}

      if (ystart > yend) 
	{
	  yincr = ystart + 1 ;
	  temp = ystart ;
	  ystart = yend ;
	  yend = temp ;
	}
      if (xstart > xend) 
	{ 
	  xincr = xstart + 1 ;
	  temp = xstart ;
	  xstart = xend ;
	  xend = temp ;
	}
	
      /* Move all drawing over by a fudge factor which should be linked to   */
      /* the label character size but isn't.                                 */
      xOff += 3 ;
      yOff += 2 ;

      graphTextFormat(BOLD);

      /* top labels */
      for (i = xstart; i <= xend; i++)
	{
	  float x_offset, position ;

	  /* Either we are drawing a label for every box or a label per      */
	  /* group of boxes.                                                 */
	  if (look->flag & FLAG_LABEL_PER_BOX)
	    position = i - 1 ;
	  else
	    position = ((i - 1) * look->dxLine) + ((look->dxLine - 1) * 0.5) ;
	  x_offset = xOff + i2x(position) ;

	  if (XAlpha)
	    graphText (messprintf ("%c", la[abs(xincr-i)]),
		       x_offset, yOff - 1.5) ;
	  else
	    graphText (messprintf ("%d", abs(xincr-i)),
		       x_offset, yOff - 1.5) ;
	}

      /* LH side labels */
      for (i = ystart ; i <= yend ; i++)
	{
	  float y_offset, position ;

	  /* Either we are drawing a label for every box or a label per      */
	  /* group of boxes.                                                 */
	  if (look->flag & FLAG_LABEL_PER_BOX)
	    position = i - 1 ;
	  else
	    position = ((i - 1) * look->dyLine) + ((look->dyLine - 1) * 0.5) ;
	  y_offset = yOff + j2y(position) ;

	  if (YAlpha) 
	    { 
	      graphText(messprintf ("%c", la[abs(yincr-i)]), 2, y_offset) ;
	      if (look->flag & FLAG_A1)
		graphText (messprintf ("%c",la[abs(yincr-i)]),
			   9 + ((xend+1)*itemWidth), y_offset) ;
	    }
	  else 
	    {
	      char *format_str ;

	      /* I can't see how to get printf to pad with blanks, seems a   */
	      /* shame as this is the obvious thing to do with ints.         */
	      if (i > 9)
		format_str = "%d" ;
	      else
		format_str = " %d" ;
	      graphText (messprintf (format_str, abs(yincr-i)), 1, y_offset) ;
	      if (look->flag & FLAG_A1)
		graphText (messprintf ("%d",abs(yincr-i)),
			   9 + ((xend+1)*itemWidth), y_offset) ;
	    }
	}

      graphTextFormat(PLAIN);
    }
  else
    {
      /* default Cambridge labeling */
      xOff += 4 ;
      graphRectangle (1, 4+yOff, 4, 20+yOff) ;
      graphText ("L", 2, 8+yOff) ;
      graphText ("A", 2, 10+yOff) ;
      graphText ("B", 2, 12+yOff) ;
      graphText ("E", 2, 14+yOff) ;
      graphText ("L", 2, 16+yOff) ;
    }

  if (graphBoxStart() != GRID_BOX)
    messcrash(GRID_PREFIX "box screwup at %d in gridDraw", GRID_BOX) ;

  /* draw comparison boxes */
  if (isSymbolic && !(look->flag & FLAG_SMALL))
    {
      for (i = MIN_LIVE_BOX ; i < arrayMax(look->segs) ; ++i)
	{
	  seg = arrp(look->segs, i, SEG) ;

	  if (seg->flag & FLAG_BLANK || 
	      !(seg->flagStore & (FLAG_SELECTED | FLAG_POS | FLAG_NEG)))
	    continue ;

	  x = i2x(seg->x) ;
	  if ((look->flag & FLAG_STAGGER) && isSymbolic && (seg->y%2))
	    x += 0.5 ;

	  y = j2y(seg->y) ;
	
	  if (!isSymbolic)
	    x += strlen(gName(seg)) ;
	
	  graphColor (gridBoxColour (look, i, TRUE)) ;
	  graphFillRectangle (xOff+x-0.3, yOff+y-0.1, 
			      xOff+x+1.3, yOff+y+1.0) ;
	  graphColor (DARKGREEN) ;
	  graphRectangle (xOff+x-0.3, yOff+y-0.1, 
			  xOff+x+1.3, yOff+y+1.0) ;
	}
    }


  /* draw main hybridisation boxes */
  graphColor (BLACK) ;

  xmax = 0; ymax = 0;
  for (i = MIN_LIVE_BOX ; i < arrayMax(look->segs) ; ++i)
    {
      seg = arrp(look->segs, i, SEG) ;

      x = i2x(seg->x) ;
      if ((look->flag & FLAG_STAGGER) && isSymbolic && (seg->y%2))
	x += 0.5 ;

      y = j2y(seg->y) ;

      if (i != graphBoxStart())
	messcrash(GRID_PREFIX "minLiveBox wrong in gridDraw()") ;
      if (seg->flag & FLAG_BLANK)
	{
	  graphText ("-", xOff+x, yOff+y) ;
	}
      else if (!isSymbolic)
	{
	  graphText (gName(seg), xOff+x, yOff + y) ;
	}
      else if (look->flag & FLAG_SMALL)
	{
	  graphLine (xOff+x, yOff+y, xOff+x, yOff+y) ;
	  graphLine (xOff+x+itemWidth, yOff+y+itemHeight,
		     xOff+x+itemWidth, yOff+y+itemHeight) ;
	}
      else
	{
	  if (isSquashed)
	    graphRectangle (xOff+x, yOff+y+0.15, 
			    xOff+x+0.8, yOff+y+0.75) ;
	  else
	    graphRectangle (xOff+x, yOff+y, 
			    xOff+x+box_width, yOff+y+box_height) ;
	}
      graphBoxEnd () ;

      graphBoxDraw (i, BLACK, gridBoxColour (look, i, FALSE)) ;

      if (look->editMenu && (look->flag & FLAG_EDIT_MODE))
	graphNewBoxMenu (i, look->editMenu) ;

      if (x > xmax)
	xmax = x ;
      if (y > ymax)
	ymax = y ;
    }

  if (look->flag & FLAG_SMALL)
    {
      graphRectangle (xOff, yOff, 
		      xOff+xmax+itemWidth, yOff+ymax+itemWidth) ;
    }
  else if (!isSymbolic)		/* draw grids */
    {
				/* the grid of lines */
      if (look->dxLine)
	for (i = look->dxLine ; ; i += look->dxLine)
	  { x = i2x(i) - 1 ;
	    if (x > xmax)
	      break ;
	    graphLine (xOff+x, yOff, xOff+x, yOff+ymax+itemHeight) ;
	  }
      if (look->dyLine)
	for (i = look->dyLine ; ; i += look->dyLine)
	  { y = j2y(i) - 0.25 ;
	    if (y > ymax)
	      break ;
	    graphLine (xOff, y+yOff, xOff+xmax+itemWidth, y+yOff) ;
	  }
      
      graphColor (WHITE) ;	/* the grid of spaces */
      if (look->dxSpace)
	for (i = look->dxSpace ; i < xmax ; i += look->dxSpace)
	  { x = i2x(i) - 2.5 ;
	    graphFillRectangle (xOff+x, yOff, xOff+x+2, ymax+yOff+1) ;
	  }
      if (look->dySpace)
	for (i = look->dySpace ; i < ymax ; i += look->dySpace)
	  { y = j2y(i) - 1.5 ;
	    graphFillRectangle (xOff, y+yOff, 
				xOff+xmax+itemWidth, y+yOff+1.5) ;
	  }
    }
  graphBoxEnd() ;		/* GRID_BOX */
  graphBoxDraw (GRID_BOX, -1, TRANSPARENT) ;

  look->crossBox = graphBoxStart() ;
  if (isSymbolic)
    {
      if (isSquashed)
	{
	  graphLine (-0.9, -0.85, -0.2, -0.25) ;
	  graphLine (-0.9, -0.25, -0.2, -0.85) ;
	}
    else
      {
	graphLine (-1.0, -0.9, -0.1, -0.2) ;
	graphLine (-1.0, -0.2, -0.1, -0.9) ;
      }
    }
  graphBoxEnd () ;
  graphBoxDraw (look->crossBox, BLACK, TRANSPARENT) ;

  /* Disable all mouse clicks on the crossbox, we want these clicks to go to */
  /* the box below.                                                          */
  graphBoxSetPick (look->crossBox, FALSE) ;
  graphBoxSetMenu (look->crossBox, FALSE) ;

  look->activeBox = 0 ;

  graphTextBounds((int)xOff + (int)xmax + (int)itemWidth + 4, 
		  (int)yOff + (int)ymax + (int)itemHeight + 4) ;

  graphRedraw() ;

  return ;
}

/**********************************/

static void gridDump (void)	/* for debugging */
{
  int i ;
  SEG *seg ;
  GRIDMAP *map ;
  GridDisp look ;

  look = getGridState("gridDump") ;

  for (i = MIN_LIVE_BOX ; i < arrayMax(look->segs) ; ++i)
    { seg = arrp(look->segs, i, SEG) ;
      if (seg->flag)
	printf ("    %8s  %3d %3d  %4x  %2d\n",	gName(seg),
		seg->x, seg->y, seg->flag, seg->mapIndex) ;
    }

  for (i = 0 ; i < arrayMax(look->map) ; ++i)
    { map = arrp(look->map, i, GRIDMAP) ;
      printf ("    %6s  %f   %f\n", 
	      name(map->ctg), map->x1, map->x2) ;
    }
}

/**********************************/

static void gridSave (void)
{
  OBJ obj ;
  int i, classe ;
  KEY key ;
  TAB *tab ;
  GridDisp look ;

  look = getGridState("gridSave") ;


  if (!checkWriteAccess())
    return ;
  
  if (!*look->probeName || !*look->probeClassName)
    { messout(GRID_PREFIX
	       "Please type a valid class name and object name into "
	       "the probe boxes and reselect this option."
	       "Do NOT hit 'Return' in the probe name box if you want "
	       "to save the current pattern.") ;
      return ;
    }

  if (!(classe = pickWord2Class (look->probeClassName)))
    { messout(GRID_PREFIX
	       "Sorry, %s is not a valid class name - edit it then retry",
	       look->probeClassName) ;
      return ;
    }

  if (!lexword2key (look->probeName, &key, classe))
    { if (!messQuery (messprintf (GRID_PREFIX
				  "%s is a new object in class %s - "
				  "should I create it?", look->probeName,
				  pickClass2Word (classe))))
      return ;
      lexaddkey (look->probeName, &key, classe) ;
    }
  else if (look->probe && key != look->probe && 
	   !messQuery (messprintf (GRID_PREFIX
				   "Probe name or class has been edited - "
				   "save changes in %s:%s?", 
				   className(key), name(key))))
    return ;

  look->probe = key ;
  if (!(obj = bsUpdate (key)))
    { messout(GRID_PREFIX "Sorry, object %s is locked", name(key)) ;
      return ;
    }

  /* Create the keys NOW if they do not exist */
  checkGridKeys();


  /* If the grid has the exclusive tags attribute then we must check to see  */
  /* if this database has clones that have multiple tags through a previous  */
  /* edit before the exclusive tag was added. If so we give the user the     */
  /* option to clean up the tags.                                            */
  if (look->exclusiveTags)
    {
      for (i = 0 ; i < arrayMax(look->tab) ; ++i)
	{
	  int  j, count ;
	  Array tagsFound ;

	  tab = arrp(look->tab, i, TAB) ;
	  tagsFound = arrayCreate (16, KEY) ;

	  for (j = 0, count = 0 ; j < arrayMax(look->tagValues) ; j++)
	    {
	      STORE_HANDLE scope_handle = handleCreate();
	      TABLE *result;
	      char *query ;
	      KEY tag ;
	      
	      tag = array(look->tagValues, j, KEY) ;

	      /* Check to see if the clone already has a tag entry for this  */
	      /* probe object.                                               */
	      query = strnew(messprintf("select c "
					"from c in object(\"Clone\", \"%s\"), "
					"to in c->%s "
					"where to.name = \"%s\" ",
					look->probeName,  name(tag), name(tab->target)), scope_handle);
	      result = aqlTable(query, (ACEOUT)NULL, scope_handle);
	      if (result == NULL)
		/* this crash may be too harsh, aqlExecute may return NULL
		 * for all sorts of reasons */
		messcrash(GRID_PREFIX "embedded AQL query %s failed", query);

	      if (tableMax(result) > 0)
		{
		  array(tagsFound, count, KEY) = tag ;
		  count++ ;
		}
	      /* else we may want to report the error in stackText (s, 0) */

	      handleDestroy(scope_handle);
	    }

	  /* If we found more than one tag for a clone, give the user the    */
	  /* chance to clean it up. (if they say "no" then the tag just gets */
	  /* added to the existing tags, i.e. non-exclusive)                 */
	  if (arrayMax(tagsFound) > 1)
	    {
	      if (messQuery (messprintf (GRID_PREFIX
					 "The probe object  \"%s\"  has multiple tags "
					 "for the clone  \"%s\", "
					 "including  \"%s, %s\".... "
					 "Do you want to replace these existing tags with the "
					 "single new  \"%s\"  tag ?  "
					 "(If you say no then this tag will be added to the "
					 "existing tags.)",
					 look->probeName, name(tab->target),
					 name(array(tagsFound, 0, KEY)),
					 name(array(tagsFound, 1, KEY)),
					 name(tab->tag))))
		{
		  int k ;
		  for (k = 0 ; k < arrayMax(tagsFound) ; k++)
		    {
		      bsGoto(obj,0) ;
		      if (bsFindKey(obj, array(tagsFound, k, KEY), tab->target))
			bsRemove (obj) ;
		    }
		}
	    }

	  arrayDestroy(tagsFound) ;	  
	}

    }


  /* OK, add the tags and save the object.                                   */
  for (i = 0 ; i < arrayMax(look->tab) ; ++i)
    { tab = arrp(look->tab, i, TAB) ;
      if (!(tab->flag & FLAG_CHANGED))
	continue ;
      if (!tab->probe) 
	tab->probe = look->probe ;
      if (tab->probe != look->probe &&
	  !messQuery (messprintf (GRID_PREFIX
				  "%s %s comes from probe %s -- "
				  "should it also apply "
				  "to this probe, %s?",
				  name(tab->tag), name(tab->target),
				  name(tab->probe), name(look->probe))))
	continue ;


      if (tab->flag & FLAG_DELETED)
	{
	  if (bsFindKey (obj, tab->tag, tab->target))
	    bsRemove (obj) ;
	}
      else
	{
	  if(!bsAddKey(obj, tab->tag, tab->target)) 
	    {
	      messerror(GRID_PREFIX
			"attempt to save tag \"%s\" for clone \"%s\" in probe object \"%s\" failed "
			"probably because that tag for that clone is already saved "
			"in the probe.",
			name(tab->tag), name(tab->target), look->probeName);
	    }
	  else if (strcmp(name(bsType(obj, _bsRight)), "?Grid") == 0)
	    {
	      /* SRK - support for probes on multiple grids for glm */
	      if(!bsAddKey (obj, _bsRight, look->key))
		messerror(GRID_PREFIX
			  "attempt to save tag \"%s\" for clone \"%s\" in probe object \"%s\" "
			  "on an associated grid failed "
			  "probably because that tag for that clone is already saved "
			  "in the probe for that grid.",
			  name(tab->tag), name(tab->target), look->probeName);
	    }
	}
      tab->flag &= ~FLAG_CHANGED ;
    }
  

  bsSave(obj) ;

  look->flag &= ~FLAG_EDITED ;

  return ;
}


/**************************/

static BOOL gridgKeyFind (GridDisp look, GKEY gkey, int *pbox)
{
  static GKEY oldKey ;
  static int ibase, oldBox ;
  int i ;

  if (gkey != oldKey || *pbox != oldBox)
    ibase = MIN_LIVE_BOX ;

  for (i = ibase ; i < arrayMax(look->segs) ; ++i)
    if(arrp(look->segs, i, SEG)->gkey  != 0){
      if (arrp(look->segs, i, SEG)->gkey == gkey)
	{ oldKey = gkey ;
	  oldBox = *pbox = i ;
	  ibase = i+1 ;
	  return TRUE ;
	}
    }
  
  ibase = MIN_LIVE_BOX ;
  return FALSE ;
}

/************************/

static void gridComparisonStore (void)
{
  int i ;
  unsigned short tmp ;
  KEY keyTmp ;
  Array aTmp ;
  SEG *seg ;
  GridDisp look ;

  look = getGridState("gridComparisonStore") ;

  for (i = MIN_LIVE_BOX ; i < arrayMax(look->segs) ; ++i)
    { seg = arrp(look->segs, i, SEG) ;
      tmp = seg->flagStore ;
      seg->flagStore = seg->flag ;
      seg->flag = tmp ;
      seg->flag |= seg->flagStore & FLAG_PER_KEY ; /* clone info */
    }

  keyTmp = look->probeStore ;
  look->probeStore = look->probe ;
  look->probe = keyTmp ;
  if (keyTmp)
    strncpy (look->probeName, name(keyTmp), 63) ;
  else
    *look->probeName = 0 ;

  aTmp = look->tabStore ;
  look->tabStore = look->tab ;
  look->tab = aTmp ;

  graphCompScrollEntry (0, look->probeName, 0, 0, 0, 0, 0) ; 
  graphEntryDisable () ;

  gridDraw (look) ;
}

/***********************************/

static void gridSelectEntry (char* string)
{
  int i ;
  GridDisp look ;

  look = getGridState("gridSelectEntry") ;

  for (i = MIN_LIVE_BOX ; i < arrayMax(look->segs) ; ++i)
    if (!lexstrcmp (string, gName(arrp(look->segs,i,SEG))))
      break ;
  if (i < arrayMax(look->segs))
    gridSelect (look, i) ;
  else
    messout(GRID_PREFIX "Sorry, I can not find %s on this grid", string) ;
}

static void gridSelectAction (KEY key)
{
  int box ;
  GridDisp look ;

  look = getGridState("gridSelectAction") ;

  if (gridgKeyFind(look, key, &box))
    gridSelect (look, box) ;
  else
    messout(GRID_PREFIX "Sorry, %s is not on this grid", name(key)) ;
  displayRepeatBlock () ;
}

static void gridSelectButton (void)
{
  displayBlock (gridSelectAction, 
		"When you pick an object that exists on this grid\n"
		"its location will be indicated by a cross.\n"
		"Remove this message to cancel.") ;
}

/************************************/

static void gridChangeAction (KEY key)
{
  GridDisp look ;

  look = getGridState("gridChangeAction") ;

  if (!isWriteAccess())
    { messout(GRID_PREFIX "Sorry, you lost write access.  Regain it and try again.") ;
      return ;
    }
  if (!look->activeBox)
    { messout(GRID_PREFIX "Sorry, there is no current gridded clone (indicated by "
	       "name on the top line and a cross on the diagram) to be "
	       "replaced.") ;
      return ;
    }
  if (class (key) != _VClone)
    { messout(GRID_PREFIX "Sorry, only clones can be placed on a Grid, and "
	       "%s is not a clone.", name (key)) ;
      return ;
    }

  gridClear () ;		/* clear everything */
  gridComparisonStore () ;
  gridClear () ;

  messout(GRID_PREFIX "This doesn't do anything yet.  It is more complicated than "
	   "I thought.  Can I have a special kernel function to help?") ;
}

static void gridChangeClone (void)
{
  GridDisp look ;

  look = getGridState("gridChangeClone") ;

  if (!isWriteAccess())
    messout(GRID_PREFIX "You must have write access to change a clone in the grid") ;

  else if (look->flag & FLAG_VIRTUAL)
    messout(GRID_PREFIX "You can not edit a virtual grid directly - "
	     "edit the real subgrid") ;

  else displayBlock (gridChangeAction,
	     "The clone that you pick will replace the current active\n"
	     "clone (indicated by name and a cross on the diagram).\n"
	     "Before doing this the current hybridisation pattern(s) will\n"
	     "be cleared.\n\n"
	     "Remove this message to cancel.") ;
}

static int gridBoxColour (GridDisp look, int box, BOOL isStore)
{
  int i, bestPrior = -129, col, segFlag ;
  void *pack;
  BOOL isPos, okay ;
  SEG *seg ;
  Array tabArray ;



  if (box < MIN_LIVE_BOX || box >= arrayMax(look->segs))
    messcrash(GRID_PREFIX "box out of bounds in grid:gridBoxColour()") ;
  seg = arrp(look->segs, box, SEG) ;

  segFlag = isStore ? seg->flagStore : seg->flag ;
  if (segFlag & FLAG_SELECTED)
    return MAGENTA ;
  if ((segFlag & FLAG_POS) && (segFlag & FLAG_NEG))
    return RED ;
  if (!(segFlag & (FLAG_POS | FLAG_NEG)))
    return WHITE ;

  okay = FALSE ;
  col = WHITE ;
  tabArray = isStore ? look->tabStore : look->tab ;
  for (i = 0 ; i < arrayMax(tabArray) ; ++i)
    {
      if(seg->gkey != 0)
	{
	  if (arrp(tabArray,i,TAB)->target == seg->gkey &&
	      !(arrp(tabArray,i,TAB)->flag & FLAG_DELETED))
	    okay = TRUE;
	}
      else
	{
	  if (arrp(tabArray,i,TAB)->targetbox == box &&
	      !(arrp(tabArray,i,TAB)->flag & FLAG_DELETED))
	    okay = TRUE;
	}

      if(okay)
	{ 
	  okay = FALSE; /* il presumably only go through this if above test is true */
	  isPos = arrp(tabArray,i,TAB)->flag & FLAG_POS ;

	  if ((assFind (look->tagMap, assVoid(arrp(tabArray,i,TAB)->tag), &pack)
	       || assFind (look->tagMap, assVoid(isPos ? _Positive : _Negative), &pack))
	      && (GRID_PRIORITY(assInt(pack)) > bestPrior))
	    {
	      bestPrior = GRID_PRIORITY(assInt(pack)) ;
	      col = isStore ? GRID_SCOLOUR(assInt(pack)) : GRID_COLOUR(assInt(pack)) ;
	    }
	  else if (bestPrior < -128)
	    {
	      col = isPos ? GREEN : LIGHTBLUE ;
	    }
	}  
    }

  return col ;
}	  

/********************************************/

static void gridReport (GridDisp look, int box)
{
  int i ;
  SEG *seg ;
  TAB *tab ;
  BOOL okay ;

  if (box < MIN_LIVE_BOX || box >= arrayMax(look->segs))
    messcrash(GRID_PREFIX "box out of bounds in gridReport()") ;

  seg = arrp(look->segs, box, SEG) ;
  *look->reportBuffer = 0 ;
  okay = FALSE;
  for (i = 0 ; i < arrayMax(look->tab) ; ++i)
    {
    tab = arrp(look->tab,i,TAB);
    okay = FALSE;
    if(seg->gkey != 0)
      {
	if(tab->target == seg->gkey)
	  okay = TRUE;
      }
    else
      {
	if(tab->targetbox == box)
	  okay = TRUE;
      }
    if (okay && !(tab->flag & FLAG_DELETED))
      {
	if ((strlen(look->reportBuffer) + strlen(name(tab->tag)) + 4) >= GRID_REPORT_BOX_LEN)
	  {
	    /* this code is bugged...doesn't check that overflow chars are   */
	    /* too  long...                                                  */
	    strcat (look->reportBuffer, GRID_REPORT_BOX_OVERFLOW) ;
	    break ;
	  }
	if (*look->reportBuffer)
	  strcat (look->reportBuffer, " ") ;
	strcat (look->reportBuffer, name(tab->tag)) ;
      }
    }
  graphBoxDraw (REPORT_BOX, -1, -1) ;

  return ;
}	  

/********************************************/

static void gridExportAdd (OBJ obj, KEY tag, int flag, 
			   KEYSET export, KEYSET result)
{ 
  static Array column = 0 ;
  register int i, j ;

  flag |= FLAG_TEMP ;
  column = arrayReCreate (column, 24, BSunit) ;
  if (bsFindTag (obj, tag) && bsFlatten (obj, 2, column))
    for (i = 0 ; i < arrayMax(column) ; i += 2)
      for (j = 0 ; j < arrayMax(export) ; ++j)
	if (arr(export, j, KEY) == arr(column, i+1, BSunit).k)
	  { arr(result, j, int) |= flag ;
	    break ;
	  }
}

static void gridExportBuild (KEY key, KEYSET export, Array result)
{
  int i ;
  OBJ obj ;
  BOOL isDefNeg = FALSE ;
  Array column = arrayCreate (24, BSunit) ;

  if ((obj = bsCreate (key)))
    { if (bsFindTag (obj, _Contains) && bsFlatten (obj, 2, column))
	for (i = 0 ; i < arrayMax(column) ; i += 2)
	  gridExportBuild (arr(column, i+1, BSunit).k, export, result) ;
      if (bsFindTag (obj, _Grid_data) && bsFlatten (obj, 2, column))
	for (i = 0 ; i < arrayMax(column) ; i += 2)
	  gridExportBuild (arr(column, i+1, BSunit).k, export, result) ;

      if (bsFindTag (obj, _Default_negative))
	{ for (i = 0 ; i < arrayMax(result) ; ++i)
	    arr(result, i, int) &= ~FLAG_TEMP ;
	  isDefNeg = TRUE ;
	}

      gridExportAdd (obj, _Positive, FLAG_POS, export, result) ;
      gridExportAdd (obj, _Negative, FLAG_NEG, export, result) ;
      gridExportAdd (obj, _Conflict, FLAG_POS | FLAG_NEG, export, result) ;

      if (isDefNeg)
	for (i = 0 ; i < arrayMax(result) ; ++i)
	  if (!(arr(result, i, int) & FLAG_TEMP))
	    arr(result, i, int) |= FLAG_NEG ;

      bsDestroy (obj) ;
    }

  arrayDestroy (column) ;
}

static void gridKeySetExportData (void)
{ 
  static char directory[DIR_BUFFER_SIZE] = "";
  static char filename[FIL_BUFFER_SIZE] = "";
  int i, j, n ;
  KEY key ;
  OBJ obj ;
  ACEOUT data_out = NULL ;
  KEYSET kSet = NULL, exportSet = NULL ;
  Array result = NULL ;
  GridDisp look ;

  look = getGridState("gridExportKeyset") ;

  if (!keySetActive (&kSet, 0))
    { messout(GRID_PREFIX "To export data you must make an active keyset of "
	       "items to export") ;
      goto end ;
    }
  if (!(obj = bsCreate (look->key)))
    { messout(GRID_PREFIX "Can't find object for grid!") ;
      goto end ;
    }
  if (!bsGetKeyTags (obj, _Export_order, 0))
    { messout(GRID_PREFIX "No export order specified in Grid object %s", 
	       name(look->key)) ;
      goto end ;
    }

  data_out = aceOutCreateToChooser ("File to write data into",
				    directory, filename,
				    "dat", "w", 0);
  if (!data_out)
    goto end ;
  exportSet = keySetCreate() ;
  while (bsGetKey (obj, _bsRight, &key))
    keySet (exportSet, keySetMax(exportSet)) = key ;
  n = keySetMax (exportSet) ;
  result = arrayCreate (n, int) ;
  array (result, n-1, int) = 0 ; /* set arrayMax() */

  for (i = 0 ; i < keySetMax(kSet) ; ++i)
    { for (j = 0 ; j < n ; ++j)
	arr(result, j, int) = 0 ;
      key = keySet(kSet, i) ;
      gridExportBuild (key, exportSet, result) ;
      aceOutPrint (data_out, "%-16s ", name(key)) ;
      for (j = 0 ; j < n ; ++j)
	arr(result, j, int) &= FLAG_POS | FLAG_NEG ;
      for (j = 0 ; j < n ; ++j)
	switch (arr(result, j, int))
	  { 
	  case 0:
	    aceOutPrint (data_out, ".") ; break ;
	  case FLAG_POS:
	    aceOutPrint (data_out, "1") ; break ;
	  case FLAG_NEG:
	    aceOutPrint (data_out, "0") ; break ;
	  case (FLAG_POS|FLAG_NEG):
	    aceOutPrint (data_out, "R") ; break ;
	  }
      aceOutPrint (data_out, "\n") ;
    }

 end:
  if (exportSet)
    keySetDestroy (exportSet) ;
  if (obj)
    bsDestroy (obj) ;
  if (data_out)
    aceOutDestroy (data_out) ;

  return ;
}

/************** end of file ****************/
 
 
 
 
