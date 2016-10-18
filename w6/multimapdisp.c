/*  File: sprdmap.c
 *  Author: Jean Thierry-Mieg (mieg@mrc-lmb.cam.ac.uk)
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
 * Description:
 **  The aim of this tool is to display on a unique graph as many
 **  system of maps as control by sprdctrl.c
 * Exported functions:
 * HISTORY:
 * Last edited: May  6 10:22 2003 (edgrif)
 * * Feb  8 16:45 1999 (edgrif): Remove use of graph internals in myMapPrint()
 * * Oct 15 16:44 1998 (edgrif): Change names of MAP2GRAPH & GRAPH2MAP
 *              as they clash with the graph headers versions.
 * Created: Wed May 27 11:40:21 1992 (mieg)
 * CVS info:   $Id: multimapdisp.c,v 1.11 2003-05-06 13:13:12 edgrif Exp $
 *-------------------------------------------------------------------
 */

#include <wh/acedb.h>
#include <wh/aceio.h>
#include <wh/bs.h>
#include <wh/bump.h>
#include <wh/table.h>
#include <wh/lex.h>
#include <wh/pick.h>
#include <wh/query.h>
#include <wh/spread.h>
#include <whooks/sysclass.h>
#include <whooks/classes.h>
#include <whooks/tags.h>
#include <whooks/systags.h>
#include <wh/display.h>
#include <wh/tabledisp.h>

BITSET_MAKE_BITFIELD	  /* define bitField for bitset.h ops */

/************************************************************/

typedef struct MapColumnStruct MapColumn;
typedef struct SegStruct SEG;

struct MapColumnStruct {
  char type;
  BOOL isMappable;
  BOOL isFlipMap;
  float mag, centre, min, max;

  Array segs , segps ;  /* segps are used to reorder the segs without loosing the friend info */
  int subTitleBox;
  int mapColBox;
  BUMP bump ; 
} ;

struct SegStruct {
  KEY key ;
  int line ;
  MapColumn *mcol;
  int x, xs ; float y, ys ;    
  int flag ;
  SEG *friends ;  /* Loop of related segs */
  int ibox ;
  KEY parent, grandParent ;
} ;

typedef struct {
  SEG* seg ;		/* pointer OK: segs fixed when drawing */
  int  color ;
  float xs, ys ;
} BOX ;



static magic_t MultiMap_MAGIC = "MultiMap";

typedef struct MultiMapStruct {
  magic_t *magic;

  Graph graph, parentGraph;

  TABLE *table;
  char *title;
  BOOL zoomAll ;
  Array mapColumns;		/* of type MapColumn */
  MapColumn *activeMapColumn ;
  int activeColBox ;
  Array mapBoxes ;
  int  cursorBox , chromoBox , allButton ;
} *MultiMap;


static int	nx, ny ;	/* window dimensions */
static float	yCentre ;	/* ny/2 */
static float	yLength ;	/* length of picture */
static float	topMargin = 6 ;	/* space at top for buttons etc */
static float	bottomMargin = 1 ; /* space at bottom */
static float	xCursor = 5 ;	/* midline of mini-chromosome */
static float	xScale = 10 ;	/* scale bar text LHS */

static float	symbolSize_L = 3.0 ; /* size of little squares */

static BOOL isPapDisp = FALSE ;

#define SPRDMAP2GRAPH(mcol, x) \
  (yCentre  +  mcol->mag * ((float)(x) - mcol->centre))
#define SPRDGRAPH2MAP(c,x) \
  (((x) - yCentre) / mcol->mag + c->centre)

#define MAP2CHROM(x) \
  (topMargin + yLength * (x - mcol->min) / (mcol->max != mcol->min ? mcol->max - mcol->min : 1 ))

#define FLAG_HIGHLIGHT 1	/* not implemented yet */
#define FLAG_INSITU 2
#define FLAG_NAME 4

/************************************************************/

/* p1, p2 are used for contigs: multiMap <-> pmap
                   for in_situ data for in_situ clones
*/
 

static void multiMapDisplayDestroy (void);
static void multiMapDraw (MultiMap mmdisp);
static MultiMap currentMultiMap (char *caller);

static void multiMapPick (int box, double x , double y, int modifier_unused) ;
static void multiMapDragCursor (float *x, float *y, BOOL isDone) ;
static void multiMapMiddleDown (double x, double y) ;
static void multiMapMiddleDrag (double x, double y) ;
static void multiMapMiddleUp (double x, double y) ;
static void multiMapConvert (MultiMap mmdisp) ;
static void multiMapDrawKey (MultiMap mmdisp, KEY key) ;
static void multiMapResize (void) ;
static void multiMapSelect (MultiMap mmdisp, int box) ;
static void multiMapFollow (MultiMap mmdisp, double x, double y) ;
static void multiMapChangeSymbolSize (void) ;
static void drawChromosomeBox (MultiMap mmdisp) ;

static void multiMapWhole (void) ;
static void multiMapZoomIn (void);
static void multiMapZoomOut (void);
static void multiMapZoomAll (void);
static void multiMapFlip (void);
static void multiMapClear (void);

static void myMapPrint(void) ;

static Array multiMapFindLimits(TABLE *table, Array isFlipMapArray);

/************************************************************/

static MENUOPT multiMapMenu[] = { 
  {graphDestroy,              "Quit"},
  {help,                      "Help"},
  {graphPrint,                "Print Screen"},
  {myMapPrint,                "Print Whole Map"},
  {menuSpacer,                ""},
  {displayPreserve,           "Preserve"},
  {multiMapChangeSymbolSize, "Symbol size"},
  {0, 0}
} ;

static MENUOPT buttonOpts[] = {
  {multiMapWhole,   "Whole"}, 
  {multiMapZoomIn,  "Zoom In"},
  {multiMapZoomOut, "Zoom Out"},
  {multiMapClear,   "Clear"},
  {multiMapFlip,    "Flip"},
  {0, 0}
} ;

/************************************************************/

/*
 * DisplayFunc for _VMultiMap objects
 */
BOOL multiMapDisplay (KEY key, KEY from, BOOL isOldGraph, void *unused) 
{ OBJ obj ;
  KEY map, anchor = 0 , anchor_group;
  int n = 0 , cl, minMap = 2 ;
  KEYSET ks ;
  char *cp ;
  BOOL isSuccess;

  if (key == KEY_UNDEFINED || class(key) != _VMultiMap || !(obj = bsCreate(key)))
    {
      return FALSE ;
    }

  ks = keySetCreate();

  bsGetData(obj, _Min, _Int, &minMap) ;
  if (minMap <1) minMap = 1 ;
  if (bsGetKey(obj, _Map, &map))
    do { 	  
	 keySet(ks, n++) = map ;
       } while (bsGetKey(obj, _bsDown, &map)) ;
  if (minMap >n)
    minMap = n ;
  if (bsGetData (obj, _Anchor, _Text, &cp) &&
      (cl = pickWord2Class (cp)) &&
      bsGetData (obj, _bsRight, _Text, &cp) &&
      lexword2key (cp, &anchor_group, _VSystem)  &&
      bsGetData (obj, _bsRight, _Text, &cp) &&
      lexword2key (cp, &anchor, _VSystem)) ;
  else
    { cl = 0 ; anchor = 0 ; anchor_group = 0 ; }
  bsDestroy(obj) ;

  isSuccess = multiMapDisplayKeySet (name(key), ks, cl, anchor_group, anchor, minMap);

  keySetDestroy (ks);		/* is this correct here ? */

 return isSuccess;
} /* multiMapDisplay */

/************************************************************/

BOOL multiMapDisplayKeySet (char *title, KEYSET ks, int cl, 
			    KEY anchor_group, KEY anchor, int minMap)
     /* also called from keySetDisplay */
{ OBJ obj ;
  Stack s = 0 ;
  KEYSET loci = 0 , loci1 = 0 ; 
  Array aa = 0;
  Array flipped ;
  int i, j, n = 0 ;
  SPREAD spread;
  TABLE *table;
  char *tableTitle;

  aa = arrayCreate(100, BSunit) ;
  loci = keySetCreate() ; n = 0 ;
  flipped = arrayCreate(12, BOOL) ;

  /* this may take very long, 
   * but when it does the user has probably chosen the wrong keyset */
  messStatus ("Multimap, F4 to interrupt...");

  for (i = 0; i < keySetMax(ks); i++)
    {
      if (messIsInterruptCalled())
	{
	  messout("Multimap interrupted...\n"
		  "Processed %d out of %d possible objects",
		  i, keySetMax(ks));
	  break;
	}
      if ((obj = bsCreate(keySet(ks, i))))
	{
	  arrayMax(aa) = 0 ;
	  if (bsFindTag(obj, _Contains) &&
	      bsFlatten(obj, 2, aa))
	    for (j = 1 ; j < arrayMax(aa); j += 2)
	      keySet(loci, n++) = arr(aa, j, BSunit).k ;
	  if (bsFindTag(obj, _Flipped))
	    array(flipped,i,BOOL) = TRUE ;
	  bsDestroy(obj) ;
	}
    }

  if (keySetMax(loci) == 0)
    {
      messout("Objects do not contain loci\n"
	      "Unable to make a multi-map !");
      return FALSE;
    }

  keySetSort(loci) ;
  keySetCompress(loci) ;

  s = stackCreate(100) ;
  if (!anchor)
    { 
      /*************** Multi-map ***************/
      loci1 = loci ;
      loci = query(loci1, "COUNT Map >= 1") ;  
      
      if (ks)
	{
	  pushText(s,"Title \"") ;
	  catText(s, "Multi-map") ;
	  if (title)
	    {
	      catText (s, " ") ;
	      catText (s, title) ;
	    }
	  catText (s, "\"\n\n") ;
	  catText(s,"Colonne 1\n"
		  "Subtitle Locus\n"
		  "Width 12\nVisible\nClass Locus\n");
	  catText(s,messprintf("Condition COUNT Map >= %d\n\n", minMap)) ;
	  
	  for (i = 0; i <  keySetMax(ks) ; i++)
	    { catText(s, "Colonne ") ;
	      catText(s, messprintf("%d\n", i + 2)) ;
	      catText(s, messprintf("Subtitle %s\n", name(keySet(ks,i)))) ;
	      catText(s, "Float\nFrom 1\nTag Map = \"") ;
	      catText(s, name(keySet(ks,i))) ;
	      catText(s, "\"") ;
	      catText(s," # Position") ;
	      catText(s," \nVisible \nOptional") ;
	      if (array(flipped,i,BOOL))
		catText(s,"\nFlipMap") ;
	      catText(s, "\n\n") ;
	    }
	}
    }
  else
    {
      /***************** Homology-map ****************/
      loci1 = loci ;
      loci = query(loci1, messprintf(">%s", name(anchor_group))) ;
      
      if (ks)
	{
	  pushText(s,"Title \"") ;
	  catText(s, "Homology-map") ;
	  if (title)
	    {
	      catText (s, " ") ;
	      catText (s, title) ;
	    }
	  catText (s, "\"\n\n") ;
	  catText(s,"Colonne 1\n"
		  "Subtitle ");
	  catText (s, pickClass2Word (cl)) ;
	  catText(s, "\n"
		  "Width 12 \nVisible \nClass ") ;
	  catText (s, pickClass2Word (cl)) ;
	  catText (s, "\n") ;
	  
	  for (i = 0; i <  keySetMax(ks) ; i++)
	    { catText(s, "Colonne ") ;
	      catText(s, messprintf("%d\n", 2*i + 2)) ;
	      catText(s, messprintf("Subtitle Locus on %s\n", name(keySet(ks,i)))) ;
	      catText(s,"Class ") ;
	      catText (s, pickClass2Word (cl)) ;
	      catText (s, " \nFrom 1 \nTag ") ;
	      catText (s, freeprotect(name (anchor))) ;
	      catText (s, "\n ") ;
	      catText(s,"Condition Map = ") ;
	      catText(s, freeprotect(name(keySet(ks,i)))) ;
	      catText(s,"\n") ;
	      catText(s," \nVisible \nOptional") ;
	      catText(s, "\n\n") ;

	      catText(s, "Colonne ") ;
	      catText(s, messprintf("%d\n", 2*i + 3)) ;
	      catText(s, messprintf("Subtitle %s\n", name(keySet(ks,i)))) ;
	      catText(s,messprintf("Float\n From %d \n Tag Map = ", 2*i + 2)) ;
	      catText(s,freeprotect(name(keySet(ks,i)))) ;
	      catText(s," # Position \n") ;
	      catText(s,"Visible \nOptional") ;
	      if (array(flipped,i,BOOL))
		catText(s,"\nFlipMap") ;
	      catText(s, "\n\n") ;
	    }
	}
    }

  /********** display data and multi-map for these definitions ********/

  /* parse definition Stack */
  spread = spreadCreateFromStack (s, NULL);
	    
  table = spreadCalculateOverKeySet (spread, loci, NULL);
  tableTitle = spreadGetTitle (spread);

  /* display results */
  tableDisplayCreate (table, tableTitle, flipped, 0) ;
  multiMapDisplayCreate (table, tableTitle, flipped) ;

  /******** clean up ********/

  spreadDestroy (spread);


  keySetDestroy(loci) ;
  if (keySetExists(loci1))
    keySetDestroy(loci1) ;

  arrayDestroy(aa) ;
  arrayDestroy (flipped);
  stackDestroy(s) ;

  /* NOTE, do -NOT- destroy ks, it belongs to caller 
   * (e.g. keysetdisplay) */

  return TRUE ;
} /* multiMapDisplayKeySet */

/************************************************************/

Graph multiMapDisplayCreate (TABLE *table, char *title, Array isFlipMapArray)
{
  MultiMap mmdisp;
  Array mapColumns;
  Graph oldGraph = graphActive();

  mapColumns = multiMapFindLimits(table, isFlipMapArray);

  if (!mapColumns)
    { 
      messout("No visible numerical column in this spread sheet") ;
      return -1 ;
    }
  
  mmdisp = (MultiMap)messalloc(sizeof(struct MultiMapStruct));
  mmdisp->magic = &MultiMap_MAGIC;
  mmdisp->graph = displayCreate("DtMULTIMAP");
  mmdisp->table = tableCopy(table, 0);
  mmdisp->title = strnew(title, 0);

  mmdisp->mapColumns = mapColumns;
  mmdisp->parentGraph = oldGraph;
  mmdisp->zoomAll =  TRUE ;

  /* default for help, if undef in displays.wrm */
  if (graphHelp(0) == NULL)
    graphHelp("Multi_Map");


  graphMenu (multiMapMenu) ;

  graphAssociate (&MultiMap_MAGIC, mmdisp) ;

  graphRegister (DESTROY,     multiMapDisplayDestroy);
  graphRegister (RESIZE,      multiMapResize) ;
  graphRegister (PICK,        multiMapPick) ;
  graphRegister (MIDDLE_DOWN, multiMapMiddleDown) ;
  graphRegister (MIDDLE_DRAG, multiMapMiddleDrag) ;
  graphRegister (MIDDLE_UP,   multiMapMiddleUp) ;


  multiMapDraw (mmdisp);

  return mmdisp->graph;
} /* multiMapDisplayCreate */



void multiMapDisplayReCreate (TABLE *table, char *title, Array isFlipMapArray)
{
  MultiMap mmdisp = currentMultiMap("multiMapDisplayReCreate");
  Array mapColumns;

  mapColumns = multiMapFindLimits(table, isFlipMapArray);

  if (!mapColumns)
    { 
      messout("No visible numerical column in this spread sheet") ;
      return ;
    }
 
  messfree(mmdisp->title);
  mmdisp->title = strnew(title, 0);

  tableDestroy(mmdisp->table);
  mmdisp->table = tableCopy(table, 0);

  mmdisp->mapColumns = mapColumns;
 
  
  multiMapDraw (mmdisp);

  return;
} /* multiMapDisplayReCreate */

/*****************************************/

static MultiMap currentMultiMap (char *caller)
{
  /* find and verify MultiMap struct on active graph */
  MultiMap mmdisp = 0 ;

  if (!(graphAssFind(&MultiMap_MAGIC, &mmdisp)))
    messcrash("%s() could not find MultiMapStruct on graph", caller);
  if (!mmdisp)
    messcrash("%s() received NULL MultiMapStruct pointer", caller);
  if (mmdisp->magic != &MultiMap_MAGIC)
    messcrash("%s() received non-magic MultiMapStruct pointer", caller);

  return mmdisp; 
} /* currentMultiMap */

/************************************************************/

static void multiMapDisplayDestroy (void)
{
  MultiMap mmdisp = currentMultiMap("multiMapDisplayDestroy");
  int colNum;
  MapColumn *mcol;

  tableDestroy(mmdisp->table);
  messfree(mmdisp->title);

  for (colNum = 0; colNum < arrayMax(mmdisp->mapColumns); colNum++)
    { 
      mcol = arrp(mmdisp->mapColumns, colNum, MapColumn) ;
      bumpDestroy(mcol->bump);
      arrayDestroy(mcol->segs);
      arrayDestroy(mcol->segps);
    }
  arrayDestroy(mmdisp->mapColumns);

  mmdisp->magic = 0;
  messfree(mmdisp);

  return;
} /* multiMapDisplayDestroy */

static void multiMapDraw (MultiMap mmdisp)
{
  char *windowTitle;

  if (!graphActivate(mmdisp->graph))
    return;

  windowTitle = displayGetTitle("DtMULTIMAP");
  if (strcmp(windowTitle, "DtMULTIMAP") == 0)
    windowTitle = "Multi Map";	/* if unchanged in displays.wrm */

  if (strlen(mmdisp->title) > 0)
    graphRetitle(messprintf("%s : %s",
			    windowTitle, mmdisp->title));
  else
    graphRetitle(messprintf("%s : untitled", windowTitle));

  /*****************/

  graphClear();

  multiMapConvert(mmdisp) ;
  multiMapWhole () ;	/* sets ->centre, ->mag and calls Draw() */

  return;
} /* multiMapDraw */


static void multiMapResize (void)
{
  MultiMap mmdisp = currentMultiMap("multiMapResize") ;

  multiMapDrawKey (mmdisp, 0) ;
}

/***********************************/

static void multiMapPick (int box, double x, double y, int modifier_unused) 
{
  int i ;
  MapColumn *mcol ;
  MultiMap mmdisp = currentMultiMap("multiMapPick") ;

  if (!box)
    return ;

  if (box == mmdisp->cursorBox)
    graphBoxDrag (mmdisp->cursorBox, multiMapDragCursor) ;

  
  for (i = 0; i < arrayMax(mmdisp->mapColumns); i++)
    { 
      mcol = arrp(mmdisp->mapColumns, i, MapColumn) ;

      if (box == mcol->subTitleBox ||
	  box == mcol->mapColBox)
	{
	  if (mcol != mmdisp->activeMapColumn)
	    {
	      if (mmdisp->activeMapColumn &&
		  mmdisp->activeMapColumn->subTitleBox)
		graphBoxDraw(mmdisp->activeMapColumn->subTitleBox, BLACK, WHITE) ;
	      mmdisp->activeMapColumn = mcol ;
	      if (mmdisp->activeMapColumn->subTitleBox)
		graphBoxDraw(mmdisp->activeMapColumn->subTitleBox, BLACK, LIGHTGREEN) ;
	      drawChromosomeBox(mmdisp) ;
	    }
	  return ;
	}
    }
				
  
  if (box == mmdisp->activeColBox)
    multiMapFollow (mmdisp, x, y) ;
  else
    multiMapSelect (mmdisp, box) ;

  return;
} /* multiMapPick */

/*********************/

static void multiMapClear (void)
{
  KEY curr ;
  MultiMap mmdisp = currentMultiMap("multiMapClear") ;
  
  if (mmdisp->activeColBox && arrp(mmdisp->mapBoxes, mmdisp->activeColBox,BOX)->seg)
    curr = arrp(mmdisp->mapBoxes,mmdisp->activeColBox,BOX)->seg->key ;
  else 
    curr = 0 ;
  
  multiMapDrawKey (mmdisp, curr) ;
  
  return;
} /* multiMapClear */


static void multiMapSelect (MultiMap mmdisp, int box) 
{
  MapColumn *mcol ;
  SEG *seg, *friend ;
  
  if (mmdisp->activeColBox)
    { 
      graphBoxDraw (mmdisp->activeColBox, BLACK, LIGHTBLUE) ;
      seg = friend = arrp(mmdisp->mapBoxes, mmdisp->activeColBox, BOX)->seg ;
      while (friend = friend->friends, friend != seg)
	if (friend->ibox)
	  graphBoxDraw (friend->ibox, BLACK, LIGHTBLUE) ;
    }

  if ((seg = arrp(mmdisp->mapBoxes, box, BOX)->seg))
    {
      mmdisp->activeColBox = box ;
      graphBoxDraw (mmdisp->activeColBox, BLACK, RED) ;
      seg = friend = arrp(mmdisp->mapBoxes, mmdisp->activeColBox, BOX)->seg ;
      while (friend = friend->friends, friend != seg)
	if (friend->ibox)
	  graphBoxDraw (friend->ibox, BLACK, LIGHTRED) ;

      mcol = seg->mcol ;
      if (mcol != mmdisp->activeMapColumn)
	{ 
	  if (mmdisp->activeMapColumn && /* check if we had no 
					  * previously selected column */
	      mmdisp->activeMapColumn->subTitleBox)
	    graphBoxDraw(mmdisp->activeMapColumn->subTitleBox, BLACK, WHITE) ;
	  mmdisp->activeMapColumn = mcol ;
	  if (mmdisp->activeMapColumn->subTitleBox)
	    graphBoxDraw(mmdisp->activeMapColumn->subTitleBox, BLACK, LIGHTGREEN) ;
	  drawChromosomeBox(mmdisp) ;
	}
      /*    
      graphActivate(spread->dataGraph) ;
      spreadSelectFromMap(spread, seg->line, c->colonne) ;
      graphActivate(spread->mapGraph) ;
      */
    }
  else
    mmdisp->activeColBox = 0 ;

  return;
} /* multiMapSelect */


static void multiMapFollow (MultiMap mmdisp, double x, double y)
{
  SEG *seg ;

  seg = arrp(mmdisp->mapBoxes, mmdisp->activeColBox, BOX)->seg ;
  if (seg && seg->parent)
    {
      if (seg->grandParent)
	{
	  if (display (seg->parent, seg->grandParent, 0))
	    displayPreserve() ;   /* automatically preserve the called map */
	}
      else
	display (seg->parent, 0, "TREE") ;
    }
  return;
} /* multiMapFollow */

/*****************************************/

/**************************************************/
/**************** drawing info ********************/

static int segpOrder(void *a, void *b)
{ 
  SEG *seg1 = *(SEG**)a;
  SEG *seg2 = *(SEG**)b;
  float ya = seg1->ys;
  float yb = seg2->ys;
  
  if (seg1->mcol && seg2->mcol)
    return ya > yb ? 1 : ( ya == yb ? 0 : -1 ) ;
  if (seg1->mcol) /* !seg2-> implied */
    return -1 ; 
  if (seg2->mcol)
    return 1 ;
  return seg1 < seg2 ? -1 : 1 ;
} /* segpOrder */

/***********************************************/

/********* start off with some utility routines ********/

static void getNxNy(void)
{
  graphFitBounds (&nx, &ny) ;
  yLength = (ny - topMargin - bottomMargin) ;
  yCentre = topMargin + 0.5*yLength ;
}

/***************************************/

static void multiMapWhole (void)
{ 
  float  l ;
  MapColumn *mcol ;
  int j, maxCol ;
  MultiMap mmdisp = currentMultiMap("multiMapWhole") ; 

  getNxNy() ;
  if (!mmdisp->activeMapColumn || mmdisp->zoomAll)
    { 
      maxCol = arrayMax(mmdisp->mapColumns) ;
      for(j = 0 ; j < maxCol ; j++)
	{ 
	  mcol = arrp(mmdisp->mapColumns, j, MapColumn) ;
	  if (!j || mcol->isMappable)
	    { 
	      l = mcol->max - mcol->min ;
	      mcol->centre = mcol->min + 0.5 * l ;
	      if (l == 0)
		l = 1 ;
	      mcol->mag = .9 * yLength / l ;
	      mmdisp->activeMapColumn = mcol ;
	    }
	}
    }
  else
    {
      mcol = mmdisp->activeMapColumn;
      if (mcol && mcol->mag)  /* will work on name collone */
	{ 
	  l = mcol->max - mcol->min ;
	  if (l == 0) 
	    l = 1 ;
	  mcol->centre = mcol->min + 0.5 * l ;
	  mcol->mag = .9 * yLength / l ;
	}
    }

  multiMapDrawKey (mmdisp, 0) ;

  return;
} /* multiMapWhole */

static void multiMapZoomIn (void)
{
  MapColumn *mcol;
  int j, maxCol;
  MultiMap mmdisp = currentMultiMap("multiMapZoomIn") ; 

  if (mmdisp->zoomAll)
    {
      maxCol = arrayMax(mmdisp->mapColumns) ;
      for(j = 0 ; j < maxCol ; j++)
	{
	  mcol = arrp(mmdisp->mapColumns, j, MapColumn) ;
	  if (!j || mcol->isMappable)
	    mcol->mag *= 2 ; 
	}
    }
  else 
    if (mmdisp->activeMapColumn)
      {
	mcol = mmdisp->activeMapColumn;
	if (mcol->mag)
	  mcol->mag *= 2 ; 
      }

  multiMapDrawKey (mmdisp, 0) ;

  return;
} /* multiMapZoomIn */


static void multiMapZoomOut (void)
{
  MapColumn *mcol ;
  int j , maxCol ;
  MultiMap mmdisp = currentMultiMap("multiMapZoomIn") ; 

  if (mmdisp->zoomAll)
    { 
      maxCol = arrayMax(mmdisp->mapColumns) ;
      for(j = 0 ; j < maxCol ; j++)
	{
	  mcol = arrp(mmdisp->mapColumns, j, MapColumn) ;
	  if (!j || mcol->isMappable)
	    mcol->mag /= 2 ; 
	}
    }
  else
    if (mmdisp->activeMapColumn)
      { 
	mcol = mmdisp->activeMapColumn;
	if (mcol->mag)
	  mcol->mag /= 2 ; 
      }

  multiMapDrawKey (mmdisp, 0) ;

  return;
} /* multiMapZoomOut */


static void multiMapZoomAll (void)
{ 
  MultiMap mmdisp = currentMultiMap("multiMapZoomAll") ; 

  mmdisp->zoomAll =  TRUE ;
  graphBoxDraw(mmdisp->allButton, BLACK, LIGHTBLUE) ;
  graphBoxDraw(mmdisp->allButton + 1, BLACK, WHITE) ;

  return;
} /* multiMapZoomAll */


static void multiMapZoomActive (void)
{ 
  MultiMap mmdisp = currentMultiMap("multiMapZoomActive") ; 

  mmdisp->zoomAll = FALSE ;
  graphBoxDraw(mmdisp->allButton + 1, BLACK, LIGHTBLUE) ;
  graphBoxDraw(mmdisp->allButton, BLACK, WHITE) ;

  return;
} /* multiMapZoomActive */


static void multiMapChangeSymbolSize (void)
{
  ACEIN size_in;

  size_in = messPrompt ("Change size of little square symbols to",
			messprintf ("%f", symbolSize_L), "fz", 0);
  if (size_in)
    {
      aceInFloat (size_in, &symbolSize_L) ;
      aceInDestroy (size_in);
    }

  return;
} /* multiMapChangeSymbolSize */


static void multiMapFlip (void)
{ 
  MapColumn *mcol ;
  SEG *seg ;
  int i ;
  float tmp ;
  MultiMap mmdisp = currentMultiMap("multiMapFlip") ; 

  mcol = mmdisp->activeMapColumn ;
  if (!mcol)
    return ;
  
  mcol->isFlipMap = ! mcol->isFlipMap ;

  i = arrayMax(mcol->segs) ;
  seg = arrp(mcol->segs, 0, SEG) - 1 ;
  while(seg++, i--)
    if (seg->mcol == mcol)
      seg->y = - seg->y ;
  tmp = mcol->min ;
  mcol->min = - mcol->max ; mcol->max = - tmp ;
  mcol->centre = - mcol->centre ;

  multiMapDrawKey (mmdisp, 0) ;

  return;
} /* multiMapFlip */

/**************************************************************/
/***************** dragging code - middle button **************/

static double	oldy, oldDy, oldx;
static BOOL	dragFast ;
#define DRAGFASTLIMIT xScale

static void multiMapMiddleDrag (double x, double y) 
{
  if (dragFast)
    {
      graphXorLine (0, oldy - oldDy, DRAGFASTLIMIT, oldy - oldDy) ;
      graphXorLine (0, oldy + oldDy, DRAGFASTLIMIT, oldy + oldDy) ;
    }
  else
    graphXorLine (DRAGFASTLIMIT, oldy, nx, oldy) ;

  oldy = y ;

  if (dragFast)
    {
      oldDy *= exp ((x - oldx) / 25.) ;
      oldx = x ;
      graphXorLine (0, y - oldDy, DRAGFASTLIMIT, y - oldDy) ;
      graphXorLine (0, y + oldDy, DRAGFASTLIMIT, y + oldDy) ;
    }
  else
    graphXorLine (DRAGFASTLIMIT, y, nx, y) ;

  return;
} /* multiMapMiddleDrag */


static void multiMapMiddleUp (double x, double y) 
{
  MapColumn *mcol ;
  float x1,x2,y1,y2 ;
  int j, maxCol ;
  MultiMap mmdisp = currentMultiMap("multiMapMiddleUp") ;

  if (mmdisp->zoomAll)
    { 
      maxCol = arrayMax(mmdisp->mapColumns) ;
      for(j = 0 ; j < maxCol ; j++)
	{
	  mcol = arrp(mmdisp->mapColumns,j, MapColumn) ;
	  if (!j || mcol->isMappable)
	    {
	      if (dragFast)
		{
		  graphBoxDim (mmdisp->cursorBox, &x1, &y1, &x2, &y2) ;
		  mcol->mag *= (y2 - y1) / (2. * oldDy) ;
		  mcol->centre = mcol->min + 
		    (mcol->max - mcol->min) * (y - topMargin) / yLength ;
		}
	      else
		mcol->centre +=  (y - 0.5 - yCentre) / mcol->mag ;
	    }
	}
    }
  else
    if (mmdisp->activeMapColumn)
      { 
	mcol = mmdisp->activeMapColumn ;
	if (dragFast)
	  { 
	    graphBoxDim (mmdisp->cursorBox, &x1, &y1, &x2, &y2) ;
	    mcol->mag *= (y2 - y1) / (2. * oldDy) ;
	    mcol->centre = mcol->min + 
	      (mcol->max - mcol->min) * (y - topMargin) / yLength ;
	  }
	else
	  mcol->centre +=  (y - 0.5 - yCentre) / mcol->mag ;
      }
  
  multiMapDrawKey (mmdisp, 0) ;

  return;
} /* multiMapMiddleUp */

static void multiMapMiddleDown (double x, double y) 
{  
  float x1,x2,y1,y2 ;
  MultiMap mmdisp = currentMultiMap("multiMapMiddleDown") ;

  getNxNy () ; 

  graphBoxDim (mmdisp->cursorBox, &x1, &y1, &x2, &y2) ;
  oldDy = (y2 - y1) / 2. ;

  dragFast = (x < DRAGFASTLIMIT) ? TRUE : FALSE ;

  if(dragFast)
    {
      graphXorLine (0, y - oldDy, DRAGFASTLIMIT, y - oldDy) ;
      graphXorLine (0, y + oldDy, DRAGFASTLIMIT, y + oldDy) ;
    }
  else
    graphXorLine (DRAGFASTLIMIT, y, nx, y) ;
   
  oldx = x ;
  oldy = y ;
  graphRegister (MIDDLE_DRAG,  multiMapMiddleDrag) ;
  graphRegister (MIDDLE_UP,  multiMapMiddleUp) ;

  return;
} /* multiMapMiddleDown */

static void multiMapDragCursor (float *x, float *y, BOOL isDone)
{
  if (isDone)
    {
      float x1,y1,x2,y2 ;
      MapColumn *mcol ;
      int j, maxCol ;
      MultiMap mmdisp = currentMultiMap("multiMapDragCursor") ;

      getNxNy() ;
      graphBoxDim (mmdisp->cursorBox, &x1, &y1, &x2, &y2) ;
      if (mmdisp->zoomAll)
	{ 
	  maxCol = arrayMax(mmdisp->mapColumns) ;
	  for(j = 0 ; j < maxCol ; j++)
	    {
	      mcol = arrp(mmdisp->mapColumns,j, MapColumn) ;
	      if (!j || mcol->isMappable)
		mcol->centre = mcol->min + (mcol->max - mcol->min) * 
		  ((*y + 0.5*(y2-y1)) - topMargin) / yLength ;
	    }
	}
      else
	if (mmdisp->activeMapColumn)
	  {
	    mcol = mmdisp->activeMapColumn ;
	    mcol->centre = mcol->min + (mcol->max - mcol->min) * 
	      ((*y + 0.5*(y2-y1)) - topMargin) / yLength ;
	  }

      multiMapDrawKey (mmdisp, 0) ;
    }
  else
    *x = xCursor - 0.5 ;

  return;
} /* multiMapDragCursor */

/**************************************************/
/**************************************************/

static void drawScale (MapColumn *mcol, int xScale1)
{
  float cutoff = 5 / mcol->mag ;
  float unit = 0.01 ;
  float subunit = 0.001 ;
  float x, xx, y, start, end ;

  while (unit < cutoff)
    {
      unit *= 2 ;
      subunit *= 5 ;
      if (unit >= cutoff)
	break ;
      unit *= 2.5 ;
      if (unit >= cutoff)
	break ;
      unit *= 2 ;
      subunit *= 2 ;
    }

  start = SPRDGRAPH2MAP(mcol, topMargin) ;
  if (start < mcol->min)
    start = mcol->min ;
  end = SPRDGRAPH2MAP(mcol, ny-bottomMargin) ;
  if (end > mcol->max)
    end = mcol->max ;
      
  x = unit * (((start > mcol->min) ? 1 : 0) + (int)(start/unit)) ;
  while (x <= end)
    {
      y = SPRDMAP2GRAPH(mcol, x) ;
	
      xx = mcol->isFlipMap ?  -x :  x ;
      graphLine (xScale1-1.5,y,xScale1-0.5,y) ;
      if(unit >= 1)
	graphText (messprintf ("%-4.0f",xx),xScale1,y-.5) ;
      else if(unit >= .1)
	graphText (messprintf ("%-4.1f",xx),xScale1,y-.5) ;
      else if(unit >= .01)
	graphText (messprintf ("%-4.2f",xx),xScale1,y-.5) ;
      else if(unit >= .001)
	graphText (messprintf ("%-4.3f",xx),xScale1,y-.5) ;
    
      x += unit ;
    }
  
  x = subunit * (((start >= mcol->min)? 1 : 0) + (int)(start/subunit)) ;
  while (x <= end)
    {
      y = SPRDMAP2GRAPH(mcol, x) ;
      graphLine (xScale1-1.0,y,xScale1-0.5,y) ;
      x += subunit ;
    }
 
  graphLine (xScale1-0.5, SPRDMAP2GRAPH(mcol,start), 
	     xScale1-0.5, SPRDMAP2GRAPH(mcol,end)) ;

  return;
} /* drawScale */



static void drawChromosomeBox (MultiMap mmdisp)
{ 
  MapColumn *mcol = mmdisp->activeMapColumn ;

  if (!mcol)
    return ;

  if (mmdisp->chromoBox)
    graphBoxClear(mmdisp->chromoBox) ;

  mmdisp->chromoBox = graphBoxStart() ;
  graphFillRectangle (xCursor - 0.25, topMargin, 
		      xCursor + 0.25, ny - bottomMargin) ;
  mmdisp->cursorBox = graphBoxStart() ;

  arrayp(mmdisp->mapBoxes, mmdisp->cursorBox, BOX)->seg = 0 ;

  if (mcol->max != mcol->min)
    graphRectangle (xCursor - 0.5, 
		    MAP2CHROM(SPRDGRAPH2MAP(mcol, topMargin)), 
		    xCursor + 0.5, 
		    MAP2CHROM(SPRDGRAPH2MAP(mcol, ny - bottomMargin))) ;
  graphBoxEnd () ;
  graphBoxEnd () ;
  graphBoxDraw (mmdisp->chromoBox,BLACK, WHITE) ;
  graphBoxDraw (mmdisp->cursorBox,DARKGREEN,GREEN) ;

  return;
} /* drawChromosomeBox */



static void drawChromosomeLine (MultiMap mmdisp)
{
  int i ;

  mmdisp->chromoBox = 0 ;

/*
  graphColor (DARKGRAY) ;
  graphLine (xCursor, MAP2CHROM(SPRDGRAPH2MAP(c,topMargin)), 
	     xScale-0.5, topMargin ) ;
  graphLine (xCursor, MAP2CHROM(SPRDGRAPH2MAP(c,ny - bottomMargin)),
	     xScale-0.5, ny - bottomMargin) ;
  graphColor (BLACK) ;
*/
  graphBoxStart() ;
  graphTextHeight (0.75) ;
  for (i = 0 ; i <= 10 ; ++i)
    graphText (messprintf ("%3d%%",10*i),
	       1, topMargin + yLength * i / 10.0 - 0.25) ;
  graphTextHeight (0) ;
  graphBoxEnd() ;

  return;
} /* drawChromosomeLine */

/***********************************************/

static void multiMapBoxEnd (int ibox, BOX *box)
{
  graphBoxEnd () ;
  if (box->seg && box->seg->flag & FLAG_HIGHLIGHT)
    graphBoxDraw (ibox, BLACK, MAGENTA) ;
  else
    graphBoxDraw (ibox, BLACK, box->color) ;

  return;
} /* multiMapBoxEnd */

/***********************************************/

static Array multiMapFindLimits(TABLE *table, Array isFlipMapArray)
{
  int rowNum, colNum, colNum1 ;
  BOOL isSuccess = FALSE ;
  float y ;
  BOOL first, found ;
  MapColumn *mcol;
  Array mapColumns = arrayCreate(table->ncol, MapColumn);
  
  if (table->ncol == 0)
    messcrash("multiMapFindLimits() - received table without columns");
  
  for(colNum = 0 ; colNum < table->ncol ; colNum++)
    {
      mcol = arrayp(mapColumns, colNum, MapColumn);
      
      mcol->type = table->type[colNum];

      if (isFlipMapArray && array(isFlipMapArray, colNum, BOOL))
	mcol->isFlipMap = TRUE;
      else
	mcol->isFlipMap = FALSE;

      if ((mcol->type == 'i' || mcol->type == 'f') /*&& !mcol->isHidden*/)
	mcol->isMappable = TRUE;
      else
	{
	  mcol->isMappable = FALSE;
	  continue;
	}
      
      first = TRUE;
      for (rowNum = 0 ; rowNum < tabMax(table, colNum) ; rowNum++)
	{
	  y = 0.0 ;
	  found = FALSE ;
	  if (!tabEmpty(table, rowNum, colNum))
	    switch (table->type[colNum])
	      {
	      case 'i':
		found = TRUE ;
		y = (float)tabInt(table, rowNum, colNum) ;
		break ;
	      case 'f':
		found = TRUE ;
		y = tabFloat(table, rowNum, colNum) ;
		break ;
	      }

          if (found)
	    {
	      y = mcol->isFlipMap ? -y : y ;
	      if (first)
		{ 
		  isSuccess = TRUE ;
		  first = FALSE ;
		  mcol->min = y ;
		  mcol->max = y ;
		}
	      if (y < mcol->min)
		mcol->min = y ;
	      if (y > mcol->max)
		mcol->max = y ;
	    }
	}
    }

  for(colNum = 0 ; colNum < table->ncol ; colNum++)
    {
      mcol = arrp(mapColumns, colNum, MapColumn);
      if (!mcol->isMappable /*&& !mcol->isHidden*/)
	{
	  for(colNum1 = table->ncol-1 ; colNum1 >= 0 ; colNum1--)
	    {
	      MapColumn *mcol1;
	      mcol1 = arrp(mapColumns,colNum1, MapColumn) ;

	      if(mcol1->isMappable)
		{
		  mcol->min = mcol1->min ;
		  mcol->max = mcol1->max ;
		  break ;
		}
	    }
	}
    }
  if (!isSuccess)
    {
      arrayDestroy(mapColumns);
      mapColumns = NULL;
    }

  return mapColumns ;
} /* multiMapFindLimits */

  /************************************************************/

static void multiMapConvert (MultiMap mmdisp)
{ 
  SEG *seg, *oldSeg ;
  int rowNum, rowMax, colNum, colMax ;
  float y ;
  MapColumn *mcol;

  colMax = mmdisp->table->ncol ;
  rowMax = tableMax(mmdisp->table);

  for (colNum = 0 ; colNum < colMax ; colNum++)
    { 
      mcol = arrp(mmdisp->mapColumns, colNum, MapColumn);

      if (colNum == 0 || mcol->isMappable)
	mcol->segs = arrayReCreate (mcol->segs, rowMax, SEG) ;
      else
	arrayDestroy(mcol->segs) ;
    }
   
  for(rowNum = 0 ; rowNum < rowMax ; rowNum++)
    {
      oldSeg = 0 ;

      for(colNum = 0 ; colNum < colMax ; colNum++)
	{
	  mcol = arrp(mmdisp->mapColumns, colNum, MapColumn);
	      
	  if (!mcol->isMappable)
	    continue ;

	  if (tabEmpty(mmdisp->table, rowNum, colNum))
	    continue;

	  seg = arrayp(mcol->segs, rowNum, SEG) ;
	  seg->line = rowNum ;
	  seg->mcol = mcol ;
	  if (oldSeg)
	    {
	      seg->friends = oldSeg->friends ;
	      oldSeg->friends = seg ;
	    }
	  else
	    {
	      seg->friends = seg ;    /* loop ! */
	      oldSeg = seg ;
	    }

	  switch (mcol->type)
	    {
	    case 'i':
	      y = (float)tabInt(mmdisp->table, rowNum, colNum) ;
	      break ;
	    case 'f':
	      y = tabFloat(mmdisp->table, rowNum, colNum) ;
	      break ;
	    }
	  seg->y = mcol->isFlipMap ? -y : y ;
	  seg->parent = tabParent(mmdisp->table, rowNum, colNum) ;
	  seg->grandParent = tabGrandParent(mmdisp->table, rowNum, colNum) ;
	}

      /* Now the names - they are the keys in the master-column No 0 */
      mcol = arrp(mmdisp->mapColumns, 0, MapColumn);
      seg = arrayp(mcol->segs, rowNum, SEG) ;
	  
      seg->key = tabKey(mmdisp->table, rowNum, 0);
      seg->parent = tabKey(mmdisp->table, rowNum, 0);
      seg->grandParent = 0 ;

      seg->line = rowNum ;
      seg->flag = FLAG_NAME ;
      seg->mcol = mcol ;
      if (oldSeg)
	{ 
	  seg->friends = oldSeg->friends ;
	  oldSeg->friends = seg ;
	  seg->y = seg->friends->y ;
	}
      else
	seg->friends = seg ;    /* loop ! */
    }


  for(colNum = 0 ; colNum < colMax ; colNum++)
    {
      mcol = arrp(mmdisp->mapColumns, colNum, MapColumn);
      if (mcol->segs)
	{ 
	  SEG *zs, **zsp ;
	  mcol->segps = arrayReCreate (mcol->segps, rowMax, SEG*) ;
	  rowNum = rowMax ;
	  zsp = arrayp(mcol->segps, rowNum - 1, SEG*) ;
	  zs = arrp(mcol->segs, rowNum - 1, SEG) ;
	  while (rowNum--)
	    *zsp-- = zs-- ;
	}
    }

  return;
} /* multiMapConvert */

/**********/

static void multiMapDrawKeyC(MapColumn *mcol, Array mapBoxes, float x)
{
  int i = arrayMax(mcol->segs), ibox, oldFormat ;
  SEG *seg = arrp(mcol->segs, 0, SEG) - 1 ;
  BOX *box ;
  int screenMin = topMargin, screenMax = ny - bottomMargin ;
	
  while(seg++, i--)
    { 
      if (!seg->mcol) 
	continue ;
      
      if (seg->ys > screenMin && seg->ys < screenMax + 30)
	{
	  ibox = graphBoxStart() ;
	  box = arrayp(mapBoxes, ibox, BOX) ;
	  box->seg = seg ;
	  seg->ibox = ibox ;
	  box->color = YELLOW ;
	  
	  if (seg->flag & FLAG_NAME)
	    {
	      box->color = WHITE ;
	      if (iskey(seg->key) == 2)
		oldFormat = graphTextFormat(BOLD) ;
	      graphText (name(seg->key), x + seg->xs, seg->ys) ;
	      if (iskey(seg->key) == 2)
		graphTextFormat(oldFormat) ;
	      seg->x = x + seg->xs ;
	    }
	  else
	    {
	      graphLine(x , seg->ys, x + symbolSize_L, seg->ys ) ;
	      graphLine(x , seg->ys , x , seg->ys + .5) ;
	      
	      seg->x = x + seg->xs ;
	    }
	  multiMapBoxEnd(ibox, box) ;
	}
      else
	{ seg->x = x ;  /* needed to draw lines */
	  seg->ibox = 0 ;
	}
    }
} /* multiMapDrawKeyC */

/**********/

static void multiMapDrawKeyLines(Array segs)
{
  float x, y ;
  int i = arrayMax(segs) ;
  int screenMin = topMargin, screenMax = ny - bottomMargin ;
  SEG *friend, *seg = arrp(segs, 0, SEG) - 1 ;
  
  while (seg++, i--)
    { 
      friend = seg ;
      x = seg->x ; y = seg->ys ;
      if (seg->mcol) /* may be zero if line is hidden */
	while (friend = friend->friends, friend != seg)
	  {
	    if ((y > screenMin && y < screenMax) ||
		(friend->ys > screenMin && friend->ys < screenMax))
	      if (x - friend->x < 26)  /* I hope nearest neighbours */
		graphLine(x, y, friend->x + symbolSize_L, friend->ys) ;
	    x = friend->x ; y = friend->ys ;
	  }
    }

  return;
} /* multiMapDrawKeyLines */

/**********/

static void multiMapBump (MapColumn *mcol)
{
  int i = arrayMax(mcol->segs) ;
  SEG **segp, *seg ;
  int screenMin = topMargin ; 
  float h = freelower(mcol->type) == 'k' ? 1 : .01 ;
  segp = arrp(mcol->segps, 0, SEG*) ;


  while (i--)
    { 
      seg = *segp++ ;
      if (seg->mcol)
	{ 
	  seg->xs = 0 ;
	  if ((seg->flag & FLAG_NAME)
	      && seg->friends)
	    seg->ys = SPRDMAP2GRAPH (seg->friends->mcol, seg->y) ;
	  else
	    seg->ys = SPRDMAP2GRAPH (seg->mcol, seg->y) ;
	}
    }
  i = arrayMax(mcol->segs) ;
  arraySort(mcol->segps, segpOrder) ;
  
  mcol->bump = bumpReCreate(mcol->bump, 1, ny) ; 

  segp = arrp(mcol->segps, 0, SEG*) ;
  while(i--)
    {
      seg = *segp++ ;
      if (!(seg->flag & FLAG_NAME) || seg->ys >= screenMin)
	bumpItem(mcol->bump, 1, h, &seg->xs, &seg->ys) ;
    }
}

/**********/

static void multiMapDrawKey (MultiMap mmdisp, KEY curr)
{
  int x, j, maxCol;
  MapColumn *mcol, *lastmcol ;

  mmdisp->mapBoxes = arrayReCreate (mmdisp->mapBoxes, 64, BOX) ;
  mmdisp->activeColBox = 0 ;
  if (mmdisp->activeMapColumn && !mmdisp->activeMapColumn->isMappable)
     mmdisp->activeMapColumn = 0 ;

  getNxNy () ;
  if (!isPapDisp)
    { 
      graphClear () ;
      graphColor (BLACK) ;
      if (yLength < 6)
	{ messout ("Sorry, this window is too small for a multi map") ;
	  return ;
	}
       drawChromosomeLine (mmdisp) ;
    }
  
  maxCol = arrayMax(mmdisp->mapColumns) ;
  
  if (strlen(mmdisp->title) > 0)
    graphText(mmdisp->title, 32, 1.5) ;

  x = xScale;
  for(j = 0 ; j < maxCol ; j++)
    {
      mcol = arrp(mmdisp->mapColumns, j, MapColumn) ;

      if (mcol->isMappable)
	{ 
	  drawScale(mcol, x) ;
	  lastmcol = mcol ;
	  x += 8 ;
	  multiMapBump(mcol) ;

	  mcol->mapColBox = graphBoxStart() ;

	  multiMapDrawKeyC(mcol, mmdisp->mapBoxes, x) ;
	  if (strlen(tableGetColumnName(mmdisp->table, j)) > 0)
	    {
	      mcol->subTitleBox = graphBoxStart() ;
	      graphText(tableGetColumnName(mmdisp->table, j), x, 5) ;
	      graphBoxEnd() ;
	    }
	  else
	    mcol->subTitleBox = 0;

	  graphBoxEnd() ;	/* mapColBox */

	  x += 3 * symbolSize_L + 2 ;
	}
      if (!mmdisp->activeMapColumn)
	mmdisp->activeMapColumn = mcol ;
    }

  drawChromosomeBox (mmdisp) ;
  mcol = lastmcol ;
  arrp(mmdisp->mapColumns, 0, MapColumn)->mag = mcol->mag ;
  arrp(mmdisp->mapColumns, 0, MapColumn)->centre = mcol->centre ;
  arrp(mmdisp->mapColumns, 0, MapColumn)->min = mcol->min ;
  arrp(mmdisp->mapColumns, 0, MapColumn)->max = (mcol->max > mcol->min ? mcol->max : mcol->max + 1) ;
  mcol = arrp(mmdisp->mapColumns, 0, MapColumn) ;
  x += 4 ;
  multiMapBump(mcol) ; 
  multiMapDrawKeyC(mcol, mmdisp->mapBoxes, x) ;
  multiMapDrawKeyLines(mcol->segs) ;

  mmdisp->allButton = graphButton ("Zoom All", multiMapZoomAll, 5, 0.5) ; 
  graphButton ("Zoom Active", multiMapZoomActive, 15, 0.5) ; /* has to be the box right after the "Zoom All" button */

  if (mmdisp->zoomAll)
    {
      graphBoxDraw(mmdisp->allButton, BLACK, LIGHTBLUE) ;
      graphBoxDraw(mmdisp->allButton + 1, BLACK, WHITE) ;
    }
  else
    { 
      graphBoxDraw(mmdisp->allButton + 1, BLACK, LIGHTBLUE) ;
      graphBoxDraw(mmdisp->allButton, BLACK, WHITE) ;
    }
  graphButtons (buttonOpts, 5, 2.5, nx) ; 

  mmdisp->activeMapColumn = 0 ;	/* ? ? ? */
  if (mmdisp->activeMapColumn && mmdisp->activeMapColumn->subTitleBox)
    graphBoxDraw(mmdisp->activeMapColumn->subTitleBox, BLACK, LIGHTGREEN) ;

  graphRedraw () ;

  return;
} /* multiMapDrawKey */

/*********************************************************/
/* This routine is a bit artificial in that it used to dig in 
 * the internals of graph but this has been changed so that it uses 
 * some calls provided by graph just to support it.
 */
static void myMapPrint(void)
{
  float min, max ;
  float oldcentre ;
  int   oldh, newh ;
  int   oldtype ;
  float olduh, newuh ;
  float yfac ;
  MapColumn *mcol ;
  MultiMap mmdisp = currentMultiMap("myMapPrintWhole") ; 
  ACEIN zone_in;

  mcol = mmdisp->activeMapColumn ;
  if (!mcol)
    {
      messerror ("No active column");
      return;
    }
  
  min = mcol->min ; max = mcol->max ;
  zone_in = messPrompt("Please state the zone you wish to print",
		       messprintf("%g   %g", min, max), "ff", 0);
  if (!zone_in)
    return ;

  aceInFloat(zone_in, &min) ; aceInFloat(zone_in, &max) ;
  aceInDestroy (zone_in);

  if (min >= max)
    return ;
  if (min < mcol->min)
    min = mcol->min ;
  if (max > mcol->max)
    max = mcol->max ;

  oldcentre = mcol->centre ;
  mcol->centre = (max + min) / 2 ;  

  /* Record current settings of active graph height etc.   */
  graphGetSprdmapData(&oldh, &olduh, &oldtype, &yfac) ;

  /* Set a new height and draw the map.                    */
  newuh = 1.05 *  (max - min) * mcol->mag + topMargin + 5 ;
  newh = newuh * yfac ;
  graphSetSprdmapCoords(newh, newuh) ;
  multiMapDrawKey (mmdisp, 0) ;

  /* OK, now it's drawn we can print it.                   */
  graphSetSprdmapType(TEXT_SCROLL) ;
  graphPrint() ;
  graphSetSprdmapType(oldtype) ;

  /* Restore the old graph height settings and redraw the map. */
  graphSetSprdmapCoords(oldh, olduh) ;
  mcol->centre = oldcentre ;
  multiMapDrawKey (mmdisp, 0) ;

  return;
} /* myMapPrint */

/*************************** eof ****************************/
