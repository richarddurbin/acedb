/*  file: pmapdisp.c
 *  Author: Richard Durbin (rd@mrc-lmb.cam.ac.uk)
 *  Copyright (C) J Thierry-Mieg and R Durbin, 1991
 *-------------------------------------------------------------------
 * This file is part of the ACEDB genome database package, written by
 * 	Richard Durbin (MRC LMB, UK) rd@mrc-lmb.cam.ac.uk, and
 *      Jean Thierry-Mieg (CRBM du CNRS, France) mieg@kaa.cnrs-mop.fr
 *
 * Description: physical map display
 * Exported functions: pMapDisplay()
 * HISTORY:
 * Last edited: Sep  1 11:11 1994 (srk)
 * * Jan 12 14:50 1994 (mieg): boxMneuCall to fingerPrint display
 * * Aug 13 1992 (neil): implement a sort of radio-buttons effect on graphButton rather than graphToggle, because
                         radio-buttons don't toggle; tweak rescaling; more work on MoveSisters;
 * * Aug 12 1992 (neil): use richard's graphBoxMenu to give a sort of pulldown menus; initialise
                         cursor position text; lightweight optimisation of RecalculateContig macro
                         to avoid extensive recomputation during editing; relaxation of rules of associativity
                         so that associativity is preserved between a buried clone and its parent after
                         Autopositioning;
 * * Jun92       (neil): port and integrate (first stage) JES's contig9 functionality: much reorganisation
                         and reconstruction, changes to display; write manipulation primitives;
 * * Jun 23 23:19 1992 (rd): change GRIDMAP to handle intervals
 * * Apr 23 1992 (neil): make pick on background deselect everything;
 * * Mar 17 1992 (neil): some contig-editing functionality: scale, draw edit panel;
 * * Apr 16 1992 (neil): fuse parameterised pmap layout with Jean's and Richard's changes;
 * * Mar 10 18:22 1992 (mieg): added an extra probe line - changed SHOWY, YMIN
 * * Feb 13 1992 (neil): un-#ifdef-out buried clones code;
 * * Feb  3 17:00 1992 (mieg): added segFormat, needed now by arrayGet/Store
 * * Jan 31 1992 (neil): add proforma for exported routine pMapGetCMapKeySet();
                         fix bug I introduced to drawing minicontig;
 * * Jan 16 1992 (neil): use showFacXY not separate scale factor for each object;
                         reintroduce menu entry for scaling to allow explicit scale factor;
                         make minicontig X-scale a constant (X0.5);
                         bug in graphical zoom - always increased scale, so:
                         make midbutton Y+ -> increase scale, midbutton Y- -> decrease scale
                         (gmap has X+ -> increase, X- -> decrease);
                         register MIDDLE_DRAG and MIDDLE_UP actions only once, on creation;
                         add routine pMapSetHighlightKeySet() to allow a caller via display 
                         to send a KEYSET to be highlit in pMapDisplay;
 * * Jan 13 01:30 1992 (mieg): Introduced look->mag, suppressed globals showFacXY
                               Horizontal midButton zooming copied from gMap
 * * Jan  6 18:48 1992 (rd): change probe localization to use gridDisp calls
 * * Jan  6 14:50 1992 (rd): add horizontal scale change to menu
 * * Dec 19 1991 (neil): add RESIZE, write pMapResize(); add centre marker;
                         correct virtually all the calls to LOOKGET;
                         add pMapHighlight(), analogous to gMapHighlight(); use keySetFind;
                         make highlight on selection RED, not BLUE, so it means the same everywhere;
                         change "Show All Objects" to "Unhighlight, Revert" (menu);
                         change free() to messfree();
                         add attachBuriedClones(), though it isn't called yet;
                         change compareSeg in line with ANSI qsort and modify for buried clones;
                         remove showWorkRemarks int to flag on LOOK flag reg;
                         add names for menu entry selections;
 * * Dec 16 14:32 1991 (rd): changed hybridisation -> map rules
 * * Dec  1 18:10 1991 (mieg): added intersection with keyset and FLAG_HIDE
 * * Oct 22 18:54 1991 (mieg): adjusted size of gMapBox 
 * * Oct 21 13:46 1991 (mieg): factor 20 in follow sequence
 * * Oct 18 20:38 1991 (mieg): made seqbox >=2,
 added a sequence box if the exists seq(gene(clone))
 * * Oct 14 22:02 1991 (rd): draw mini contig with scroll box
 * * Oct 14 18:45 1991 (rd): use TextFit window instead of MapScroll
 * * Oct  7 22:20 1991 (rd): changed cursor to redraw properly
 *-------------------------------------------------------------------
 */

/* $Id: pmapdisp+fp.c,v 1.2 1994-09-01 11:11:51 srk Exp $ */

#include <stdarg.h>
#include "acedb.h"
#include "array.h"
#include "graph.h"
#include "key.h"
#include "lex.h"
#include "bs.h"
#include "plot.h"
#include "a.h"
#include "sysclass.h"
#include "systags.h"
#include "classes.h"
#include "tags.wrm"
#include "disptype.h"
#include "display.h"
#include "session.h"
#include "keyset.h"
#include "flag.h"
#include "pmap.h"
#include "pmap_.h"
#include "grid.h"

#ifdef ACEDB1
#define _VMyGene _VGene
#define _VMyLocus _VLocus
#else
#define _VMyGene _VLocus
#define _VMyLocus 256      /* imposible value I hope */
#define _gMap _Map
#endif

#ifndef NEIL
#define DrawCursorLocText(__look)
#define DrawCursor(x,y)
mark_t *toCurrentMark(LOOK look) { return 0 ; }
void pMapCancelDisplayBlock (void) { ; }
extern void pMapFreeEditBuf(edits_t *editBufp) { ; }
extern void pMapPadKeyPress(int keyPress) { ; }
extern BOOL pMapCloneIsPositioned(KEY key, OBJ *Objpp, KEY *contigp, int *ctgLp, int *ctgRp, KEY *canonp) { return FALSE ; }
#endif

/*..... ..... ..... .....cue bar parameters
*/
static float pMapCursorLocX=0.0, pMapCueX=0.0, pMapCueY=0.0;

/*..... ..... ..... .....static routines
*/

static void pMapBoxDraw(int box, DrawType drawType, SEG *seg);

static char *pMapName(KEY key, char *buf);
static void mapDestroy(void);
static void pMapPick(int box, double x, double y);
static void pMapMiddleDown (double x , double y) ;
static void pMapMiddleDrag (double x, double y); 
static void pMapMiddleUp (double x, double y);

static void findLimits(LOOK look);

static void mapKbd (int k) ;
static void pMapShowAll(void) ;
static void pMapHide(void) ;

static void setDisplayDepths (void) ;
static void pMapShowSegs (LOOK look, KEY key) ;
static void pMapSelectBox (LOOK look, int box) ;
extern void pMapFollow(LOOK look, double x, double y);
static void pMapScroll (float *px, float *py, BOOL isDone) ;
static void pMapHighlight(KEYSET k, KEY key);
static void pMapResize (void);
static void drawMiniContig(LOOK look, float fac);
static void drawBandScale(LOOK look);
static void drawMarks(LOOK look);

static void utilMenuT(void);
static void autoMenuT(void);
static void markMenuT(void);
static void clonesMenuT(void);
static void contigMenuT(void);
static void createButtonMenu(LOOK look, void (*actionR) (void), FREEOPT *menu);

/*..... ..... ..... .....stray extern
*/
extern void pMapToGMap(KEY contig ,KEY key, int position); /*<<--code for this is in gmapdisp.c*/
/*extern Array pMapFingerPrint(KEY clone) ; */

/*..... ..... ..... .....
*/
static char pMapNameBuffer[128], pMapEdNameBuffer[65];

/*..... ..... ..... .....
*/
static int gMapBox=0;

/*..... ..... ..... .....anchor points used by pMapShowSegs - these may be modified in other places now
*/
static int
  anchorRule,
  anchorCueBar,
  anchorProbes,
  anchorYACs,
  anchorClones,
  anchorSequence,
  anchorNavigBar,
  anchorGenes,
  anchorRemarks,
  anchorMiniContig,
  anchorScrollHelp;

int remarksDisplay=1; /*0->no remarks, 1->show remarks, 2->show all remarks*/
static int nReg = 10 ; /*depth of display for cosmids (enlarged to show buried clones)*/

/*..... ..... ..... .....
*/
LOOK pMapEditPanelRoot=NULL;

/*..... ..... ..... .....
*/

float Xmagn=0.4, oviewXmagn=0.05, Ymagn=1.3, Xoffset=0.0, oviewXoffset=0.0, Yoffset=(-1.0);
  /*
  I gave Xoffset and oviewXoffset arbitrary initial values to keep the link editor happy -
  they have no meaning in the application
  */

static double oldy , oldDx, oldx;
static BOOL dragFast;
static LOOK lookDrag=0;

/*..... ..... ..... .....
*/
static int centreDisplayCursor=0; /*actually calculated during pMapEdName call*/

/*..... ..... ..... .....
*/
static KEYSET pMapPendingKeySet=0;

/*..... ..... ..... .....
*/
BOOL interruptFlagged=FALSE;

/*..... ..... ..... .....default physical map menu
*/
FREEOPT pmap_menu[]=
{
  13 /*no of entries - **NB update this when change menu*/, "Physical Map",

  MENU_QUIT, "Quit",
  MENU_HELP, "Help",
  MENU_PRINT, "Print",
  MENU_PRESERVE, "Preserve",
  MENU_REDRAW, "Redraw",
    
  MENU_RECALCULATE, "Recalculate",
  MENU_SHOW_REMARKS, "Show All Remarks",
  MENU_SHOW_SELECTED, "Show Selected Objects",
  MENU_HIGHLIGHT_SELECTED, "Highlight Selected Objects",
  MENU_REVERT_DISPLAY, "Unhighlight, Revert",
    
  MENU_SHOW_ALL_BURIED_CLONES, "Show All Buried Clones",
  MENU_SET_ZOOM, "Set Zoom",
  MENU_SET_DISPLAY_DEPTHS, "Set display depths",

  MENU_TO_EDIT_CONTIG, "Edit Contig" /* must be last - add if ACEDB_CONTIG9 */
};

/*..... ..... ..... .....edit panel menus
*/
static FREEOPT utilMenu[]= /*.....utilities*/
{
  10, "Utilities...",
  MENU_QUIT_PMAPED, "Quit",
  MENU_HELP_PMAPED, "Help",
  MENU_PRINT, "Print",
  MENU_PRESERVE, "Preserve",
  MENU_REDRAW, "Redraw",

  MENU_RECALCULATE, "Recalculate",
  MENU_PEEK_PMAP, "Overview Physical Map",
  MENU_FROM_EDIT_CONTIG, "Finished Edit",
  MENU_SHOW_EDIT_REMARKS, "Remarks",
  MENU_REVERT_DISPLAY, "Unhighlight, Revert",
};

static FREEOPT autoMenu[]= /*.....utilities*/
{
  8, "Auto...",

  MENU_AUTOPOSITION_KEYSET, "Autoposition Clones from Keyset",
  MENU_SET_AUTOPOSITION_DOMAIN, "Set Autoposition Domain",
  MENU_LOAD_CLONECACHE, "Load Domain",
  MENU_RESET_AUTOPOSITION_DOMAIN, "Use Whole Database",
  MENU_SAVE_CLONECACHE, "Save Domain",

  MENU_LOAD_AUTOPOS_STATE, "Load Results",
  MENU_SAVE_AUTOPOS_STATE, "Save Results",
  MENU_DRAW_FINGERPRINT, "Fingerprint",
};

static FREEOPT markMenu[]= /*.....scale, mark operations*/
{
  12, "Marks...",
  MENU_PUT_MARK, "Set Mark At Cursor Position",
  MENU_MARK_CENTRE, "Mark Centre",
  MENU_MARK_CENTRE_OF_COSMID, "Mark Centre Of Cosmid",
  MENU_SET_MARK, "Set Mark",
  MENU_ZERO_SCALE_TO_MARK, "Zero Scale To Mark",

  MENU_SET_SCALE, "Set Scale At Mark",
  MENU_RESET_SCALE, "Reset Scale",
  MENU_SNAP_TO_MARK, "Snap To Mark",
  MENU_SNAP_TO_PREVIOUS_MARK, "Snap To Previous Mark",
  MENU_VISIT_MARK, "Visit Mark",

  MENU_DELETE_MARK, "Delete Mark",
  MENU_DELETE_ALL_MARKS, "Delete All Marks",
};

static FREEOPT clonesMenu[]= /*.....position clones*/
{
  10, "Clones...",
  MENU_ABUT_MARK_LEFT, "Left End Of Clone To Mark",
  MENU_ABUT_MARK_RIGHT, "Right End Of Clone To Mark",
  MENU_EXTEND_TO_MARK_LEFT, "Extend Left End Of Clone To Mark",
  MENU_EXTEND_TO_MARK_RIGHT, "Extend Right End Of Clone To Mark",
  MENU_ATTACH_CLONE, "Attach Clone",

  MENU_POSITION_CLONE, "Autoposition Clone", /*posclone*/
  MENU_POSITION_PARAMETERS, "Set Autoposition Parameters",
  MENU_DETACH_CLONE, "Detach Clone",
  MENU_SHOW_BURIED_CLONES, "Show Buried Clones Of Clone",
  MENU_SHOW_ALL_BURIED_CLONES, "Show All Buried Clones",
};

static FREEOPT contigMenu[]= /*.....edit contig*/
{
  1, "Contig...",
  MENU_OPEN_CONTIG, "Open Contig",
};

/*..... ..... ..... .....version to reuse edit buffer/29Jun92
*/
extern BOOL pMapDisplay (KEY key, KEY from, BOOL isOldGraph) /*called by display()/display.c as a functional*/
{
  KEY contig=0, clone=0, pmap;
  int pos;
  OBJ  Clone;
  BOOL isProbe=FALSE;
  Array segs;
  LOOK look=NULL;
  static int isFirst = TRUE ;	/* RMD change to add Edit menu if ACEDB_CONTIG9 */

  if (isFirst)
    { isFirst = FALSE ;
      if (getenv ("ACEDB_CONTIG9"))
	++pmap_menu->key ;
    }				/* end of RMD change */

  if (class(key) == _VClone)	/* use the contig */
    { clone = key ;

      if (!(Clone = bsCreate(clone)))
	goto abort ;

      if (!bsGetKey(Clone, _pMap, &contig) && !CanonicalFromBuriedClone(Clone, clone))
	if (CloneIsGridded(Clone, key))
	  { bsDestroy (Clone) ;
	    display (key, clone, 0) ; /* key is a Grid_Clone */
	    goto abort ;
	  }
	else
	  goto abortNoMap ;
      if (clone == from)
	goto abortClone ;
      if (!contig)
	{
	  bsDestroy (Clone) ;
	  if (!(Clone = bsCreate(clone))) /*value of clone may have changed*/
	    goto abort ;
	  if(!bsGetKey (Clone,_pMap,&contig))
	    goto abortNoMap ; /*even parent not located on a contig*/
	}
      if (!bsGetData(Clone,_bsRight,_Int,&pos))
	goto abortNoMap ;

      bsDestroy (Clone) ;
      if (isProbe)
	clone = key ;
      key = contig ;
    }

  if (class(key) == _VpMap)
    pmap = key ;
  else if (class(key) == _VContig)
    lexaddkey (name(key), &pmap, _VpMap) ;
  else
    goto abort ;

  if (!(segs=arrayGet(pmap, SEG, segFormat)) && (class(key)!=_VContig || !(segs=pMapConvert(NULL, key, pmap))))
    goto abort ;

  if (IsntSet(arrayp(segs, 0, SEG), FLAG_POST17JUN92_REPR)) /*recalculate whole array into new format*/
  {
    arrayDestroy(segs);
    if (class(key)==_VContig)
    {
      segs=pMapConvert(NULL, key, pmap);
    }
    else
    {
      messout("pMapDisplay couldn't reformat physical map.");
      goto abort;
    }
  }

  if (isOldGraph) /*check whether I'm looking up a clone on the current edit panel - special case*/
  {
    KEY lookContig;

    if (contig && pMapEditPanelRoot) /*wherever the pick is, I want to be able to view a displayed clone on the edit panel window*/
    {
      look=pMapEditPanelRoot;
      messfree(look->segs);
      look->segs=segs;
      graphRetitle(pMapEdName(look, clone)); /*may be a different contig*/
      look->edit->cursorBandPos=centreDisplayCursor; /*update the cursorBandPos to a sensible value*/
    }
    else if (contig /*given a clone and it had a contig attribute*/ &&
        graphAssFind(pMapDisplay, &look) /*this window*/ && look->magic==MAGIC /*I can retrieve the old look*/ &&
        (lexaddkey(name(look->key), &lookContig, _VContig) /*the look key is in pMap class - don't test result*/, (0<iskey(lookContig))) &&
        EditingContig(look) && IsSet(look, FLAG_EDIT_PANEL) && IsSet(look, FLAG_PRESERVE_EDIT_SESSION) &&
          (lookContig==contig /*clone's contig same as pmap's!*/ ||
           IsSet(look, FLAG_AUTOPOSITIONING_CLONES) /*want to be able to view any contig from within edit panel*/)
      )
        /*..then reuse - I want the existing edit panel..*/
    {
      messfree(look->segs);
      look->segs=segs;
      if (IsSet(look, FLAG_AUTOPOSITIONING_CLONES)) graphRetitle(pMapEdName(look, clone)); /*may be a different contig*/
      look->edit->cursorBandPos=centreDisplayCursor; /*update the cursorBandPos to a sensible value*/
    }
    else
    {
      mapDestroy();
      graphAssRemove(pMapDisplay);
      graphRetitle(pMapName(key, pMapNameBuffer));
      look=(LOOK) messalloc(sizeof(struct LOOKSTUFF));
      look->magic=MAGIC;
      look->key=pmap;
      look->segs=segs;
      look->edit=NULL;
      look->menu=pmap_menu;
    }
  }
  else 
  {
    Graph g;

    if ((g=displayCreate(PMAP))==0)
      goto abort;

    graphRetitle(pMapName(key, pMapNameBuffer));
    graphRegister(DESTROY, mapDestroy) ;
    graphRegister(MESSAGE_DESTROY, pMapCancelDisplayBlock);
    graphRegister(RESIZE, pMapResize);
    graphRegister(PICK, pMapPick) ;
    graphRegister(MIDDLE_DOWN, pMapMiddleDown) ;
    graphRegister(MIDDLE_DRAG, pMapMiddleDrag) ;
    graphRegister(MIDDLE_UP, pMapMiddleUp) ;
    graphRegister(KEYBOARD, mapKbd) ;
    look=(LOOK) messalloc(sizeof(struct LOOKSTUFF));
    look->magic=MAGIC;
    look->mainGraph=g;
    look->key=pmap;
    look->segs=segs;
    look->edit=NULL;
    look->menu=pmap_menu;
      /*always use the default menu when showing a contig for the first time*/
  }
  look->activebox=0;
  look->box2seg=arrayCreate(32,SEG*);
  look->centre=0;

  graphFreeMenu(pMapMenu, look->menu);

  graphAssociate(pMapDisplay, look); /*doesn't matter if already been associated*/

  if (!clone && class(from)==_VClone || class(from)==_VCalcul)  /* Geometrical positioning */
    clone=from;
  findLimits(look);
  if (pMapPendingKeySet!=NULL)
  {
    pMapHighlight(pMapPendingKeySet, clone); /*<<--calls pMapDraw, so pass clone down to it*/
    pMapPendingKeySet=NULL;
  }
  else
  {
    pMapDraw(look, clone);
  }
  return TRUE;

abortNoMap:
abortClone:
  bsDestroy(Clone);
  display(key, from, TREE);	/* just show as a TREE */

abort:
/*I don't create the look until later and this messfree shd be redundant now:*/
  messfree (look);
  return FALSE;
}

/************************************************************/

static void mapDestroy(void)
{
  LOOKGET("mapDestroy");

  if (IsntSet(look, FLAG_PRESERVE_EDIT_SESSION))
    /*(neil)
    **NB I'm making a special use of this routine:
    mapDestroy may be called from pMapDisplay, eg when I want to go to a named or picked clone on 
    the current contig, on which some edits have already been done, which I don't want to lose:
    the call to pMapDisplay, which, when "reusing" the current graph, typically makes a new LOOK struct 
    before deallocating the old one (even if the old one is still valid - the same contig is going to 
    be displayed), in this case must save the edit control buffer: 
    */
    FreeEditBuf(look->edit);

  arrayDestroy(look->segs);
  arrayDestroy(look->box2seg);
  look->magic=0;
  messfree(look);
}

/*************************************************************/

static char *pMapName(KEY key, char *buf) 
{ KEY contig , chromo ;
  float start , stop ;
  OBJ Contig ;

  strcpy(buf, name(key)) ;
  if (class(key) == _VContig)
    contig = key ;
  else if (class(key) == _VpMap)
    lexaddkey (name(key), &contig, _VContig) ;
  else
    return buf;

  if (Contig = bsCreate(contig))
    { if(bsGetKey(Contig,_gMap,&chromo))
	{ strcpy(buf,name(chromo)) ;
	  if(bsGetData(Contig,_bsRight,_Float,&start) &&
	     bsGetData(Contig,_bsRight,_Float,&stop) )
	    strcat(buf, messprintf(": [%4.2f, %4.2f]", start, stop)) ;
	  else
	    strcat(buf, " position unknown") ;
	}
      else
	strcpy(buf, "This contig is not localised") ;
      bsDestroy(Contig) ;
    }
  
  return buf;
}

/*..... ..... ..... .....
*/
extern char *pMapEdName(LOOK look, KEY clone) /*if given a clone, derive context from it, else from the look*/
{
  KEY contig=0;
  OBJ Ctg=NULL;
  int lext=0, rext=0; /*left and right extremes of contig*/

  if (clone) /*from pick on sensitive KEY of a report pad, eg*/
  {
    OBJ Clone=NULL; /*not used here*/ KEY canon; /*not used here*/
    int cloneL, cloneR;

    if (pMapCloneIsPositioned(clone, &Clone, &contig, &cloneL, &cloneR, &canon))
    {
      bsDestroy(Clone);
      pMapName(contig, pMapEdNameBuffer);
      centreDisplayCursor=(int) (0.5*(cloneL+cloneR));
    }
  }
  if (!contig) /*"Edit Contig" menu pick, or couldn't get contig from clone - use the look*/
  {
    lexaddkey(name(look->key), &contig, _VContig);
    strcpy(look->edit->chromo, pMapNameBuffer);
      /*save the chromosome hdr info so I can restore it*/
    strcpy(pMapEdNameBuffer, pMapNameBuffer);
  }
  if (0<iskey(contig) && (Ctg=bsCreate(contig))!=NULL)
  {
    if (bsGetData(Ctg, _pMap, _Int, (void *) &lext) && bsGetData(Ctg, _bsRight, _Int, (void *) &rext))
    {
      sprintf(&pMapEdNameBuffer[strlen(pMapEdNameBuffer)], "; %s: [%d, %d]\0", name(contig), lext, rext);
    }
    else
    {
      messout("pMapEdName couldn't get data for %s.", name(contig));
    }
    bsDestroy(Ctg);
  }
  else
  {
    messout("pMapEdName given invalid contig (key %d).", contig);
  }


  return pMapEdNameBuffer;
}

/*..... ..... ..... ..... ..... ..... ..... ..... ..... ..... ..... .....menu
*/
extern void pMapMenu(KEY k)

#define RefuseIfNoWriteAccess \
  if (!isWriteAccess()) \
  { \
    messout("Sorry, you do not have Write Access."); break; \
  }

#define RefuseIfNoCurrentCloneOrContig \
  if (!look->activebox) \
  { \
    messout("First select a clone."); break; \
  } \
  if (!lexword2key(name(look->key), &contig, _VContig)) \
  { \
    messout("Sorry, can't find contig object."); break; \
  } \
  seg=arr(look->box2seg, look->activebox, SEG*); \
  if (class(seg->key)!=_VClone) \
  { \
    messout("First select a clone."); break; \
  }

#define RefuseIfNoCurrentCloneContigGene \
  if (!look->activebox) \
  { \
    messout("First select a clone or gene."); break; \
  } \
  if (!lexword2key(name(look->key), &contig, _VContig)) \
  { \
    messout("Sorry, can't find contig object."); break; \
  } \
  seg=arr(look->box2seg, look->activebox, SEG*); \
  if (!(class(seg->key)==_VClone || class(seg->key)==_VMyGene)) \
  { \
    messout("First select a clone or gene."); break; \
  }

#define RefuseIfNoCurrentMark \
  if (look->edit && look->edit->mark.hd==NULL) \
  { \
    messout("No marks have been set."); break; \
  }

#define RevertToDefaultPMAP(__finished) \
  if (__finished) \
    Clear(look, FLAG_PRESERVE_EDIT_SESSION); \
  Clear(look, FLAG_EDIT_PANEL); \
  graphTextHeight(0 /*back to default*/); \
  graphFreeMenu(pMapMenu, (look->menu=pmap_menu)); \
  graphRetitle(look->edit->chromo); \
  graphRegister(KEYBOARD, mapKbd); \
  if (__finished) \
    FreeEditBuf(look->edit); \
  pMapDraw(look, 0)

{
  LOOKGET("pMapMenu");

  switch ((int) k)
  {
  /*.....display*/
  case MENU_QUIT:
    graphDestroy () ;
    break ;
  case MENU_HELP: 
    helpOn("Physical-map"); 
    break;
  case MENU_PRINT: 
    graphPrint(); 
    break;
  case MENU_PRESERVE: 
    displayPreserve(); 
    break;
  case MENU_REDRAW: 
    findLimits(look); 
    pMapDraw(look,0); 
    break;
  case MENU_RECALCULATE:
    pMapRecalculateContig(look, 0, TRUE, FALSE, TRUE);
    break ;
  case MENU_SHOW_REMARKS:
    Toggle(look, FLAG_SHOW_WORK_REMARKS);
    pMapDraw(look, 0);
    break ;
  case MENU_SHOW_EDIT_REMARKS:
    /*I treat this a bit differently in the edit panel - only to be used if FLAG_EDIT_PANEL set*/
    switch(remarksDisplay=(remarksDisplay+1)%3)
      {
      case 0:
        anchorRemarks=5000; /*take remarks off-screen*/
        break;
      case 1:
        Clear(look, FLAG_SHOW_WORK_REMARKS);
        anchorRemarks=anchorProbes+nReg+15;
        break;
      case 2:
        Set(look, FLAG_SHOW_WORK_REMARKS);
        anchorRemarks=anchorProbes+nReg+15;
        break;
      }
    pMapDraw(look, 0);
    break ;    
  case MENU_SHOW_SELECTED: 
    pMapHide(); 
    break;
  case MENU_HIGHLIGHT_SELECTED: 
    pMapHighlight(NULL, 0); 
    break;
  case MENU_REVERT_DISPLAY: 
    pMapShowAll(); 
    break;
  case MENU_SHOW_ALL_BURIED_CLONES: /*switch*/
    Toggle(look, FLAG_SHOW_BURIED_CLONES);
    if (IsSet(look, FLAG_SHOW_BURIED_CLONES))
      displayAllBuriedClones(look);
    pMapDraw(look, 0);
    break;
  case MENU_SET_ZOOM:
    if (messPrompt("Increase to zoom in, decrease to zoom out:", messprintf("%.2f",Xmagn), "fz"))
    {
      freefloat(&Xmagn);
      pMapDraw(look, 0);
    }
    break;
  case MENU_SET_DISPLAY_DEPTHS:
    setDisplayDepths () ;
    break ;

#ifdef NEIL

  /*.....change context*/
  case MENU_TO_EDIT_CONTIG: RefuseIfNoWriteAccess;
    Set(look, FLAG_EDIT_PANEL);
    Set(look, FLAG_PRESERVE_EDIT_SESSION);
    if (IsntSet(arrayp(look->segs, 0, SEG), FLAG_POST17JUN92_REPR))
      /*recalculate whole array into new format - it shd have been done by now, but just in case..*/
      pMapRecalculateContig(look, 0, TRUE, FALSE, FALSE);
    newEditBuf(look);
    graphTextHeight(0.8); /*squeeze a bit more onto edit window*/
    graphFreeMenu(pMapMenu, (look->menu=markMenu));
    graphRetitle(pMapEdName(look, 0 /*get KEY from look*/));
    graphRegister(KEYBOARD, pMapEdMapKbd);
    pMapDraw(look, 0);
    break;
  case MENU_FROM_EDIT_CONTIG:
    RevertToDefaultPMAP(TRUE);
    break;
  case MENU_PEEK_PMAP: /*revert to pmap default display for scrolling - contig editing not finished*/
    RevertToDefaultPMAP(FALSE);
    break;
  case MENU_GENETIC_MAP: RefuseIfNoCurrentCloneContigGene;
    pMapToGMap(contig, seg->key, SegMidPt(seg));
    break;

  /*.....pmaped: scale, mark operations*/
  case MENU_PUT_MARK:
    if (look->edit->cursorBandPos==MINUSINF)
      messout("You have not used the cursor yet and its position is undefined.");
    else
    {
      addMark(look, look->edit->cursorBandPos);
      pMapDraw(look, 0);
    }
    break;
  case MENU_MARK_CENTRE: /*set a mark at current graph centre*/
    addMark(look, (int) look->centre);
    pMapDraw(look, 0);
    break;
  case MENU_MARK_CENTRE_OF_COSMID: /*set a mark at current cosmid centre*/
    {
      SEG *seg;
      KEY key;
      if (look->activebox!=0 && (seg=arr(look->box2seg, look->activebox, SEG*))!=NULL && 
        0<iskey(key=seg->key) && class(key)==_VClone && CloneIsCosmid(seg))
      {
        addMark(look, (int) SegMidPt(seg));
        pMapDraw(look, 0);
      }
      else
        messout("No, or inappropriate, current selection: couldn't set a mark.");
    }
    break;
  case MENU_SET_MARK:
    if (messPrompt("set mark at band position:", messprintf("%d", (int) look->centre), "iz"))
    {
      int band;
      freeint(&band);
      addMark(look, band);
    }
    pMapDraw(look, 0);
    break;
  case MENU_MOVE_MARK_TO_LEFT_OF_CLONE:
    break;
  case MENU_MOVE_MARK_TO_RIGHT_OF_CLONE:
    break;
  case MENU_ZERO_SCALE_TO_MARK: RefuseIfNoCurrentMark;
    rescaleToCurrentMark(look);
    pMapDraw(look, 0);
    break;
  case MENU_SET_SCALE: RefuseIfNoCurrentMark;
    if (messPrompt("set current mark position:", messprintf("%d", (int) look->centre), "iz"))
    {
      int band;
      freeint(&band);
      setScale(look, band);
    }
    pMapDraw(look, 0);
    break;
  case MENU_RESET_SCALE: RefuseIfNoCurrentMark;
    resetScale(look);
    pMapDraw(look, 0);
    break;
  case MENU_SNAP_TO_MARK: RefuseIfNoCurrentMark;
    SaveCurrentSelected(look);
    snapToCurrentMark(look);
    pMapDraw(look, 0);
    break;
  case MENU_SNAP_TO_PREVIOUS_MARK: RefuseIfNoCurrentMark;
    SaveCurrentSelected(look);
    snapToMark(look, TRUE);
    pMapDraw(look, 0);
    break;
  case MENU_VISIT_MARK: RefuseIfNoCurrentMark;
    SaveCurrentSelected(look);
    visitMark(look);
    pMapDraw(look, 0);
    break;
  case MENU_DELETE_MARK: RefuseIfNoCurrentMark;
    SaveCurrentSelected(look);
    deleteCurrentMark(look);
    pMapDraw(look, 0);
    break;
  case MENU_DELETE_ALL_MARKS: RefuseIfNoCurrentMark;
    SaveCurrentSelected(look);
    deleteMarks(look);
    pMapDraw(look, 0);
    break;
  
  /*.....pmaped: position clones*/
  case MENU_ABUT_MARK_LEFT:
    if (abutCurrentMark(look, LEFT)) pMapDraw(look, 0);
    break;
  case MENU_ABUT_MARK_RIGHT:
    if (abutCurrentMark(look, RIGHT)) pMapDraw(look, 0);
    break;
  case MENU_EXTEND_TO_MARK_LEFT:
    if (extendToCurrentMark(look, LEFT)) pMapDraw(look, 0);
    break;
  case MENU_EXTEND_TO_MARK_RIGHT:
    if (extendToCurrentMark(look, RIGHT)) pMapDraw(look, 0);
    break;
  case MENU_SHOW_BURIED_CLONES: /*of current clone*/ /*switch*/
    switchDisplayBuriedClones(look);
    pMapDraw(look, 0);
    break;
  case MENU_ATTACH_CLONE:
    if (InPriorModality(look))
      messout("Please complete %s operation first.", CurrentModalOperation(look));
    else if (!displayBlock(pMapAttachClone, NULL))
      messout("Please complete pick operation already started before trying this.");
    else
      pMapDraw(look, 0); /*does this get executed in right order?*/
    break;
  case MENU_DETACH_CLONE: RefuseIfNoWriteAccess; RefuseIfNoCurrentCloneOrContig;
    pMapDetachClone(look);
    pMapDraw(look, 0);
    break;
  case MENU_POSITION_CLONE: RefuseIfNoWriteAccess; RefuseIfNoCurrentCloneOrContig;
    pMapPositionClone(look, 0, NULL /*use current selection*/);
    pMapDraw(look, 0);
    break;
  case MENU_POSITION_PARAMETERS:
    if (messPrompt("Tolerance for band comparisons (1/10 mm):", messprintf("%d", pMapTolerance), "iz"))
    {
      freeint(&pMapTolerance);
    }
    if (messPrompt("Match against clones in region of current nominal position +/- (band units):", messprintf("%d", pMapExtn), "iz"))
    {
      freeint(&pMapExtn);
    }
    break;
  case MENU_BURY_UNBURY: break;
  case MENU_FIND_CLONE: break;

  /*.....pmaped: auto*/
  case MENU_SET_AUTOPOSITION_DOMAIN:
    {
      void *dummy;
      KEYSET kSet = 0 ;
      if (!keySetActive(&kSet, &dummy))
        messout("First select a keyset of clones to define the domain.");
      else
        if (kSet && 0<keySetMax(kSet) && (class(arr(kSet, 0, KEY))==_VClone))
        {
          pMapAutoposDomain=arrayCopy(kSet);
            /*
            1/ seems to be a problem with changing Selected Keyset
            2/ autopositionFromKeyset/pmapcons.c arrayDestroys the previous copied KEYSET
            */
          pMapReadClones=FALSE; /*force the code to make new domain cloneCache*/
          messout("OK.");
        }
        else
          messout("Domain must be specified as a keyset of clones.");
    }
    break;
  case MENU_RESET_AUTOPOSITION_DOMAIN:
    pMapAutoposDomain=NULL;
    pMapReadClones=FALSE; /*force the code to make new domain cloneCache*/
    messout("OK.");
    break;
  case MENU_AUTOPOSITION_KEYSET:
    pMapAutopositionClones(look);
    break;
  case MENU_DRAW_FINGERPRINT:
    if (look->activebox)
      pMapDrawFingerprint(look, array(look->box2seg, look->activebox, SEG *)->key);
    else
      messout("First pick a clone.");
    break;
  case MENU_SET_SHOW_FINGERPRINT:
    Toggle(look, FLAG_SET_SHOW_FINGERPRINT);
    break;
  case MENU_LOAD_AUTOPOS_STATE:
    pMapLoadScoreCards(look);
    CreateScoreCardPad(look, "Autoposition From KeySet");
    pMapPrintScoreCards(look);
    pMapDrawPad();
    break;
  case MENU_SAVE_AUTOPOS_STATE:
    pMapSaveScoreCards(look);
    break;
  case MENU_LOAD_CLONECACHE:
    pMapLoadCloneCache(look);
    break;
  case MENU_SAVE_CLONECACHE:
    pMapSaveCloneCache(look);
    break;

  /*.....global ops over contigs*/  

  /*.....pmaped: edit contigs*/  
  case MENU_JOIN_CONTIGS: break;
  case MENU_OPEN_CONTIG:
    if (InPriorModality(look))
      messout("Please complete %s operation first.", CurrentModalOperation(look));
    else if (CurrMark(look)==NULL)
      messout("First set a mark.  You can only open or close up the contig at the current mark.");
    else
    {
      SetModalOp(look, FLAG_STRETCHING_CONTIG);
        /*this preempts all other picking on the pmap, and enforces stretch contig IF (and only if) the left button is pressed down*/
      interruptFlagged=FALSE; /*interrupts pending on the X event queue are not cleared however*/
      pMapCue(look, PMAP_CUE, "Use left mouse button to drag contig from current mark.");
    }
    break;
  case MENU_NEW_CONTIG: break;
  case MENU_INVERT_CONTIG_REGION: break;
  case MENU_ALIGN_CONTIGS: break;

  /*.....utilities*/
  case MENU_NULL_OP: break;
  case MENU_QUIT:
  case MENU_QUIT_PMAPED:
    Clear(look, FLAG_PRESERVE_EDIT_SESSION); /*"Quit" from default physical map display OR edit panel*/
    if (look->edit!=NULL && look->edit->pad.pad)
    {
      graphActivate(look->edit->pad.pad);
      graphDestroy();
      graphActivate(look->mainGraph);
    }
    FreeEditBuf(look->edit); /*users can do "Overview Physical Map", then "Quit"*/
    graphDestroy();
    break;
  case MENU_HELP_PMAPED: helpOn("Contig-editor"); break;

  /*.....consistency check*/
  case MENU_VERIFY_COR_DATA: checkCORfile(look); break;
#endif /* NEIL */
  }
  return;
}

#undef RefuseIfNoWriteAccess
#undef RefuseIfNoCurrentCloneOrContig
#undef RefuseIfNoCurrentCloneContigGene
#undef RefuseIfNoCurrentMark
#undef RevertToDefaultPMAP

/*..... ..... ..... ..... ..... ..... ..... ..... ..... ..... ..... .....
*/

#ifdef NEIL

static GraphFunc leftDragR=NULL, leftUpR=NULL;
static float
  currModalOpX, currModalOpY, currModalOpDispX,
  barX, barY,
  slideContigRegionFromX, slideContigRegionToX;
static int ctgLExtreme, ctgRExtreme; /*untransformed left and right extremes of contig in model coords*/
static LOOK currModalOpLook=0;
static mark_t *currMark=NULL;
static int currModalOpTextBox=0;
static KEY currModalOpSelection=0;
int currModalOp_nSelected=0;

#define X_ARM 5.0
#define Y_ARM 5.0

#define ShiftLeftSegment (slideContigRegionFromX<=barX)

#define DrawCursorLocText(__look) \
  (__look)->edit->cue.cursorTextBox=graphBoxStart(); \
  if ((__look)->edit->cursorBandPos==MINUSINF) (__look)->edit->cursorBandPos=(int) (__look)->centre; \
  if ((__look)->edit->cursorBandPos==MINUSINF) \
    graphText(messprintf("     "), pMapCursorLocX+LeftXSpace, pMapCueY+LeftYSpace); \
  else \
    graphText(messprintf(COORDFORMAT, (__look)->edit->cursorBandPos-(__look)->edit->virtualXorig), \
      pMapCursorLocX+LeftXSpace, pMapCueY+LeftYSpace); \
  graphRectangle(pMapCursorLocX, pMapCueY, pMapCursorLocX+RightXSpace, pMapCueY+RightYSpace); \
  graphBoxEnd(); \
  graphBoxDraw((__look)->edit->cue.cursorTextBox, BLACK, YELLOW)

#define DrawCursor(__look, __x) \
  (__look)->edit->cursorBandPos=(int) GraphToModelX(__x); \
  DrawXorEditCursor(__x); \
  DrawCursorLocText(__look)

#define DrawXorCross(__look) \
  graphXorLine(currModalOpX-X_ARM, currModalOpY, currModalOpX+X_ARM, currModalOpY); \
  graphXorLine(currModalOpX, currModalOpY-Y_ARM, currModalOpX, currModalOpY+Y_ARM); \
  (__look)->edit->cursorBandPos=(int) GraphToModelX(currModalOpX); \
  DrawCursorLocText(__look)

#define DrawDragText drawDragText(currModalOpLook)

#define DrawXorMark \
  graphXorLine(barX, ModelToGraphY(anchorCueBar), barX, barY); \
  DrawDragText

#define DragXorCrossTo(__look, __x, __y) \
  graphXorLine(currModalOpX-X_ARM, currModalOpY, currModalOpX+X_ARM, currModalOpY); \
  graphXorLine(currModalOpX, currModalOpY-Y_ARM, currModalOpX, currModalOpY+Y_ARM); \
  currModalOpX=(__x); currModalOpY=(__y); \
  graphXorLine(currModalOpX-X_ARM, currModalOpY, currModalOpX+X_ARM, currModalOpY); \
  graphXorLine(currModalOpX, currModalOpY-Y_ARM, currModalOpX, currModalOpY+Y_ARM); \
  (__look)->edit->cursorBandPos=(int) GraphToModelX(currModalOpX); \
  DrawCursorLocText(__look)

#define DragXorMark(__disp) \
  DrawXorEditCursor(barX+currModalOpDispX); \
  currModalOpDispX=(__disp); \
  DrawXorEditCursor(barX+currModalOpDispX); \
  DrawDragText

#define ClearModalDragOp \
  graphRegister(LEFT_DRAG, leftDragR); \
  graphRegister(LEFT_UP, leftUpR); \
  ClearModalOp(currModalOpLook, FLAG_STRETCHING_CONTIG)

/*..... ..... ..... .....drag mark
*/
static void leftDrag(double x, double y) 
{
  DrawXorEditMark(currModalOpX);
  currModalOpX=x;
  DrawXorEditMark(currModalOpX);

  currMark->text=graphBoxStart();
  graphText(messprintf(COORDFORMAT, (int) (GraphToModelX(currModalOpX)-currModalOpLook->edit->virtualXorig)), 
    barX+LeftXSpace, barY+LeftYSpace);
  graphRectangle(barX, barY, barX+RightXSpace, barY+RightYSpace);
  graphBoxEnd();
  graphBoxDraw(currMark->text, BLACK, YELLOW /*by defn, the current mark*/);

  return;
}

/*..... ..... ..... .....reposition mark
*/
static void leftUp(double x, double y) 
{
  moveCurrentMark(currModalOpLook, GraphToModelX(x));
  /*restore ambient left button routines*/
  graphRegister(LEFT_DRAG, leftDragR);
  graphRegister(LEFT_UP, leftUpR);
  Recentre(currModalOpLook, currMark->band);
  pMapDraw(currModalOpLook, 0);
  return;
}

/*..... ..... ..... .....drag contig region
*/
static void leftShiftContigRegion(double x, double y) 
{
  DragXorCrossTo(currModalOpLook, x, Trunc(ModelToGraphY(anchorCueBar)+Y_ARM, y, barY-Y_ARM));
  slideContigRegionToX=x; /*needed for diagnostics*/
  DragXorMark(x-slideContigRegionFromX);
  return;
}

/*..... ..... ..... .....reposition contig region
*/
static void leftDropContigRegion(double x, double y) 
{
  if (!INTERRUPT_REQUESTED)
    if (shiftContigRegion(currModalOpLook, barX, slideContigRegionFromX, (slideContigRegionToX=x)))
      /*shiftContigRegion fails if it processes an interrupt*/
    {
      addMark(currModalOpLook, currMark->band+pMapShift);
      snapToMark(currModalOpLook, FALSE); /*preserve the original one's status as current mark*/
    }
  ClearModalDragOp;
  pMapDraw(currModalOpLook, 0);
  return;
}

/*..... ..... ..... .....
*/
static void drawDragText(LOOK look)
     
#define NShift (int) GraphToModelX(slideContigRegionToX)-(int) GraphToModelX(slideContigRegionFromX)
  /*see pMapShift/pmaped.c: no of bands shifted*/

{
  int N=NShift, markC=CurrentMarkPos;
  char sL, sR;

  currModalOpTextBox=graphBoxStart();
  if (ShiftLeftSegment)
  {
    if (currModalOpDispX<0)
      sL=sR='<';
    else if (currModalOpDispX==0)
    {
      sL='['; sR=']';
    }
    else
      sL=sR='>';
    graphText(messprintf("%c%d, %d%c (%d) [%d, %d]", sL, ctgLExtreme+N, markC+N, sR, N, markC, ctgRExtreme),
      pMapCueX+LeftXSpace, pMapCueY+LeftYSpace);
  }
  else
  {
    if (currModalOpDispX<0)
      sL=sR='<';
    else if (currModalOpDispX==0)
    {
      sL='['; sR=']';
    }
    else
      sL=sR='>';
    graphText(messprintf("[%d, %d] (%d) %c%d, %d%c", ctgLExtreme, markC, N, sL, markC+N, ctgRExtreme+N, sR),
      pMapCueX+LeftXSpace, pMapCueY+LeftYSpace);
  }
  graphRectangle(pMapCueX, pMapCueY, pMapCueX+42.8 /* - well count them: assume each int up to 6 chars*/, pMapCueY+RightYSpace);
  graphBoxEnd();
  graphBoxDraw(currModalOpTextBox, BLACK, YELLOW);
  return;
}

#undef NShift

#endif /* NEIL */

/*..... ..... ..... .....
*/
static void pMapPick(int box, double x, double y) 
{
  LOOKGET("pMapPick");

#ifdef NEIL

  if (IsSet(look, FLAG_EDIT_PANEL) && IsSet(look, FLAG_STRETCHING_CONTIG)) /*LOOSE MODAL op - doesn't stop all other ops*/
    /*in which case I can pick anywhere - it shouldn't be done like this but I can't get hold of the LEFT_DOWN*/
  {
    pMapCue(look, PMAP_CLEAR, NULL);
    if (INTERRUPT_REQUESTED)
    {
      ClearModalDragOp;
      return;
    }
    if ((currMark=CurrMark(look))==NULL)
      messout("First set a mark.  You can only open or close up the contig at the current mark.");
    else
    {
      float xorig, yorig, xext, yext;
      graphBoxDim(box, &xorig, &yorig, &xext, &yext);
        /*get the origin of the graph so I can get the correct coords of the pick: this crashes ACeDB if the box is not found*/

      SaveCurrentSelected(look); /*in case I drag beyond the range where current selection can be picked up again*/
      barX=ModelToGraphX(currMark->band); barY=ModelToGraphY(anchorRule); currModalOpLook=look;
      currModalOpX=slideContigRegionFromX=slideContigRegionToX=xorig+x;
      currModalOpY=Trunc(0.0+Y_ARM, yorig+y, barY-Y_ARM); currModalOpDispX=0.0;
      pMapContigExtremeCoords(look, &ctgLExtreme, &ctgRExtreme);
      DrawXorCross(look);
      DrawXorMark;
      leftDragR=graphRegister(LEFT_DRAG, leftShiftContigRegion);
      leftUpR=graphRegister(LEFT_UP, leftDropContigRegion);
    }
    return; /*leftDropContigRegion restores the status quo callbacks*/
  }
  else if (IsSet(look, FLAG_EDIT_PANEL) && IsSet(look, FLAG_POSITIONING_YAC)) /*LOOSE MODAL op*/
  {
    SEG *seg, *firstSeg;
    KEY k, contig;
    int ctgL, ctgR;
    OBJ Contig;

    if (box==0 || IsReservedBox(look, box))
    {
      return;
    }
    if (!(0<box && box<arrayMax(look->box2seg)))
    {
      messerror("pMapPick given invalid box %d.", box);
      return;
    }
    if (!(0<iskey(k=(seg=arr(look->box2seg, box, SEG*))->key) && class(k)==_VClone && !CloneIsYAC(seg)))
    {
      messout("Please pick a cosmid.");
      return;
    }
    if (currModalOp_nSelected==0) /*first one*/
    {
      currModalOpSelection=seg->key;
        /*save the key not the box, in case the user does anything to reorder the contig while this op is in progress*/
      currModalOp_nSelected++;
      pMapBoxDraw(box, MODAL_SELECTED, seg);
      if (IsSet(look, FLAG_EDIT_PANEL)) drawMarks(look);
      pMapCue(look, PMAP_CUE, "Now position the other end of the YAC by picking on a cosmid.");
      return;
    }
    else if (currModalOp_nSelected==1)
    {
      KeyToSeg(look->segs, currModalOpSelection, firstSeg); /*get the seg for the saved key*/
      if (k==currModalOpSelection /*same cosmid picked twice*/ || SegMidPt(seg)==SegMidPt(firstSeg) /*zero extent specified*/)
      {
        messout("Zero extent specified for YAC.");
        goto abortOp;
      }
      pMapBoxDraw(box, MODAL_SELECTED, seg);
      if (SegMidPt(seg)<SegMidPt(firstSeg))
      {
        ctgL=SegMidPt(seg); ctgR=SegMidPt(firstSeg);
      }
      else
      {
        ctgL=SegMidPt(firstSeg); ctgR=SegMidPt(seg);
      }
      if (CreateContigFromLook(look, contig, Contig))
        bsDestroy(Contig); /*just get the KEY for the contig*/
      else
      {
        messout("Couldn't get the contig from the graph. Please start again.");
        goto abortOp;
      }
      if (pMapUpdateClonePosition(AskCurrentOperand(look), NULL, contig, ctgL, ctgR, TRUE))
      {
        pMapRecalculateContig(look, AskCurrentOperand(look) /*make this the current selection*/, TRUE, FALSE, TRUE /*redraw here*/);
      }
      else
      {
        messout("Can't get clone %s from database, or can't update it. Please start again.", name(AskCurrentOperand(look)));
      }
abortOp:
      pMapCue(look, PMAP_CLEAR, NULL);
      ClearModalOp(look, FLAG_POSITIONING_YAC);
      return;
    }
    return;
  }
  else if (IsSet(look, FLAG_EDIT_PANEL) && IsSet(look, FLAG_POSITIONING_COSMID)) /*LOOSE MODAL op*/
  {
    return;
  }
  else if (IsSet(look, FLAG_EDIT_PANEL) && IsCueBox(look, box))
  {
    return;
  }
  else if (IsSet(look, FLAG_EDIT_PANEL) && (currMark=boxIsMarkBar(look, box))!=NULL)
  {

    SaveCurrentSelected(look); /*in case I drag beyond the range where current selection can be picked up again*/
    barX=ModelToGraphX(currMark->band); barY=ModelToGraphY(anchorRule+2.0);
    currModalOpX=ModelToGraphX(currMark->band); currModalOpLook=look;
    leftDragR=graphRegister(LEFT_DRAG, leftDrag);
    leftUpR=graphRegister(LEFT_UP, leftUp);
    return;
  }
  else if (IsSet(look, FLAG_EDIT_PANEL) && boxIsMarkText(look, box)!=NULL)
  {
    return;
  }
  else 
#endif /* NEIL */
  if (box==0) /*pick on background*/
  {
    pMapSelectBox(look, 0); /*deselect current selection*/
    return ;
  }
  else if (box==look->scrollBox)
  {
    graphBoxDrag (look->scrollBox, pMapScroll);
  }
  else if (arrayMax(look->box2seg)<box)
  {
    messerror("pMapPick received an invalid box number");
  } /*<<-- **NB I moved this clause from after if !box because scrollBox isn't a seg, surely??*/
  else if (box==look->activebox)  /*second pick on this box: display the object*/
  {
    pMapFollow(look, x, y);
  }
  else /*select this box and highlight it*/
  {
    pMapSelectBox (look,box) ;
    if (IsSet(look, FLAG_EDIT_PANEL)) drawMarks(look);
      /*
      because edit marks can be occluded by highlighting of abutted boxes, when highlighting changes
      parts of marks can be cleared
      */
  }
  return;
}

#undef X_ARM
#undef Y_ARM
#undef ShiftLeftSegment
#undef DrawXorCross
#undef DrawXorMark
#undef DragXorCrossTo
#undef DragXorMark
#undef ClearModalDragOp

/***********************************/

static void mapKbd (int k)
{
  int box, table ;
  SEG *seg ;
  LOOKGET("mapKbd") ;

  if (!look->activebox)
    return ;

  box = look->activebox ;
  if (!(seg = arr(look->box2seg,box,SEG*)))
    return ;
  table = class(seg->key) ;
  switch (k)
    {
    case LEFT_KEY :
      while ( --box > 0 &&
	     (seg = arr(look->box2seg,box,SEG*)) &&
	     class(seg->key) != table) ;
      break ;
    case RIGHT_KEY :
      while (++box < arrayMax(look->box2seg) &&
	     (seg = arr(look->box2seg,box,SEG*)) &&
	     class(seg->key) != table) ;
      break ;
    }

  if (box < arrayMax(look->box2seg) && arr(look->box2seg, box, SEG*))
    { pMapSelectBox (look, box) ;
      pMapFollow (look,0,0) ;
    }
}

/*..... ..... ..... .....
*/
extern void pMapEdMapKbd(int k)
{
  mark_t *p;
  LOOKGET("pMapEdMapKbd");
  if (EditingContig(look) && IsSet(look, FLAG_AUTOPOSITIONING_EDIT))
  {
    pMapPadKeyPress(k);
  }
      /*
      preserve the pad accelerator meanings even if the pointer
      strays into the edit panel: this means that while the
      autopositioning flag is set, the user can't use the arrow
      keys to move marks: but this flag is cleared if the user
      types "e" to edit the autopositioned clone's position
      */
  else if ((p=toCurrentMark(look))!=NULL)
    /*resets map display to centre on current mark position, unless mark is already visible*/
  {
    switch (k)
    {
    case LEFT_KEY: MoveMark(look, (-1)); break;
    case RIGHT_KEY: MoveMark(look, 1); break;
    default: break;
    }
  }
  return;
}

/**********************************/

static void findLimits(LOOK look)
{
  int i, min=1000000000, max= -1000000000;
  Array segs=look->segs;
  SEG *seg;

  for (i=1; i<arrayMax(segs); ++i)
  {
     seg=arrp(segs, i, SEG);
     if (seg->x0<min) min=seg->x0;
     if (max<seg->x1) max=seg->x1;
  }
  look->min=min;
  look->max=max;
  return;
}

/******************************/

extern void pMapDraw(LOOK look, KEY key)     /* key becomes activebox, unless reverting*/
{
  LOOK lookout;
  float y=0.0;
  Graph currGraph=graphActive();


  graphActivate(look->mainGraph); /*because I may be calling pMapDraw when the pad is active, eg*/
  lookout=pMapLookTry("pMapDraw", pMapDisplay);

  if (lookout!=look)
  {
    invokeDebugger();
  }

  graphClear();
  graphLinewidth(0.10);

  if (!arrayMax(look->segs))
    return;

  graphFitBounds(&look->winWidth, &look->winHeight);
  ++look->winWidth;

  if (IsSet(look, FLAG_EDIT_PANEL))
  {
/*for now*/
    pMapShowSegs(look, key); /*no little centre markers!*/
    graphRedraw();
    drawMarks(look);
  }
  else
  {
    graphLine(look->winWidth/2, y, look->winWidth/2, y+0.9);
    pMapShowSegs (look, key) ;
    graphLine(look->winWidth/2, look->winHeight-2.0, look->winWidth/2, look->winHeight-2.0+0.9); /*<<--temporarily*/
    graphRedraw();
  }
  graphActivate(currGraph);
  return;
}

/*..... ..... ..... .....
*/

extern void pMapAddTempSeg(LOOK look, KEY key, int ctgL, int ctgR)
{
  SEG *seg;
  int box;

  look->activebox=box=graphBoxStart();
  SetCurrentSelected(look, key); /*only called from within contig editor*/
  /*in case of a redraw..*/
  seg=arrayp(look->segs, arrayMax(look->segs), SEG);
  seg->key=seg->parent=key;
  seg->x0=ctgL; seg->x1=ctgR;
  ClearReg(seg);
  Set(seg, FLAG_FINGERPRINT);


  array(look->box2seg, box, SEG *)=arrayp(look->segs, arrayMax(look->segs)-1, SEG);
  DrawSegLine(ctgL, ctgR, MINUSINF);
  DrawSegText(name(key), 0.5*(ctgL+ctgR), MINUSINF);
  graphBoxEnd();
  pMapDrawGeneralEntity(box, class(key), SELECTED);
  return;
}

/*************************************************************/
/******** pmap show intersect with active keyset *************/
 
static void pMapShowAll(void) /*show pmap without any highlighting*/
{
  int i, n ;
  SEG *seg ;
 
  LOOKGET("pMapShowAll") ;

  seg = arrp(look->segs,0,SEG) ;
  i = arrayMax(look->segs) ;
  n = FLAG_HIDE | FLAG_HIGHLIGHT /**| FLAG_AUTOPOSITIONING_AGAINST**/; /*<<--*/
  while(i--)
    {
      seg->flag&= ~n; /*<<--reset these bits*/
      seg++;
    }

  Clear(look, FLAG_SHOW_WORK_REMARKS);
    /*make sure extra remarks aren't shown*/
  Set(look, FLAG_REVERT_DISPLAY);
    /*<<--this is just to communicate down the call stack without passing arg: it only happens here*/
  pMapDraw (look, 0);
  Clear(look, FLAG_REVERT_DISPLAY);
}

/**********/

static void pMapHide(void)
{
  int i, j, n ;
  SEG *seg ;
  void *dummy ;
  KEYSET kSet = 0 ;
 
  LOOKGET("pmapHide") ;

  if(!keySetActive(&kSet, &dummy))
    { messout("First select a keySet window, thank you.") ;
      return ;
    }
  seg = arrp(look->segs,0,SEG) ;
  i = arrayMax(look->segs) ;
  n = FLAG_HIDE ;
  while(i--)
    { seg->flag &= ~n ;  /* bitwise NOT */
      if (!arrayFind(kSet,&seg->key,&j, keySetOrder) &&
	  !arrayFind(kSet,&seg->parent,&j, keySetOrder))
	seg->flag |= FLAG_HIDE ;
      seg++ ;
    }
  pMapDraw (look, 0);
}


/*********************************************/
/******* local display routines **************/

/************* first a mini package for bumping text *************/

typedef struct bumpStruct
 { int n ;
   double y0 ;
   double *end ;
 } *BUMP ;

static BUMP bumpCreate (int n, double y0)
{
  int i ;
  BUMP bump = (BUMP) messalloc (sizeof (struct bumpStruct)) ;

  bump->n = n ;
  bump->y0 = y0 ;
  bump->end = (double*) messalloc (n*sizeof (double)) ;
  for (i = 0 ; i < n ; ++i)
    bump->end[i] = -100000000.0 ;
  return bump ;
}

static void bumpDestroy (BUMP bump)
{
  messfree (bump->end) ;
  messfree (bump) ;
}

static void bumpWrite (LOOK look, BUMP bump, char *text, double x)
{
  int i ;
  int len = strlen(text) ;
  int min ;
  
  for (i = 0 ; i < bump->n ; ++i)
    if (bump->end[i] < x)
      { graphText (text, ModelToGraphX(x)-3.5, ModelToGraphY(bump->y0 + i)) ;
	bump->end[i] = x + (len+1)/Xmagn ;
	return ;
      }

  min = 0 ;
  for (i = 1 ; i < bump->n ; ++i)
    if (bump->end[i] < bump->end[min])
      min = i ;

  graphText (text, ModelToGraphX(bump->end[min])-3.5, ModelToGraphY(bump->y0 + min)) ;
  bump->end[min] += (len+1)/Xmagn ;
}


static void bumpNoWrite (LOOK look, BUMP bump, char *text, double x)
{
  int i ;
  int len = strlen(text) ;
  int min ;
  for (i = 0 ; i < bump->n ; ++i)
    if (bump->end[i] < x)
      { bump->end[i] = x + (len+1)/Xmagn ;
	return ;
      }

  min = 0 ;
  for (i = 1 ; i < bump->n ; ++i)
    if (bump->end[i] < bump->end[min])
      min = i ;

  bump->end[min] += (len+1)/Xmagn;
}

/********* use middle button for cursor **********/

#define ACCEL (1./25.)

/*..... ..... ..... .....
*/
static void pMapMiddleDown (double x, double y) 
{ 
  LOOKGET("pMapMiddleDown"); lookDrag=look;

  oldx=x; oldy=y;
  if (IsSet(look, FLAG_EDIT_PANEL)) /*no scrollbox*/
  {
    SaveCurrentSelected(look); /*in case I drag a long way and the current selection is lost*/
    dragFast=FALSE;
    DrawCursor(look, oldx);
  }
  else if (anchorMiniContig < y + 8 && anchorMiniContig > y - 8)
  { float x1, x2, y1, y2;
    dragFast = TRUE ;
    graphBoxDim(look->scrollBox, &x1, &y1, &x2, &y2);
    oldDx=0.5*(x2-x1);
    oviewXoffset=ScrolledCentreOffset(oviewXmagn);
    graphXorLine(oldx-oldDx, anchorMiniContig-10, oldx-oldDx, anchorMiniContig+10);
    graphXorLine(oldx+oldDx, anchorMiniContig-10, oldx+oldDx, anchorMiniContig+10);
      /*
      anchorMiniContig is set in pMapShowSegs when the graph is constructed depending 
      on whether this is extendedPanel or not, so this covers both cases
      */
  }
  else if (IsSet(look, FLAG_SHOW_BURIED_CLONES))
  { dragFast = FALSE ;
    graphXorLine(oldx, anchorMiniContig+10, oldx, anchorMiniContig+50);
  }
  else /*default physical map display*/
  { dragFast = FALSE ;
    graphXorLine(oldx, 0, oldx, anchorMiniContig-10);
  }
  return;
}

/*..... ..... ..... .....
*/
static void pMapMiddleDrag (double x, double y) 
{
  LOOKGET("pMapMiddleDrag");
  if (IsSet(look, FLAG_EDIT_PANEL))
  {
    DrawCursor(lookDrag, oldx);
  }
  else if (dragFast)
  {
    graphXorLine(oldx-oldDx, anchorMiniContig-10, oldx-oldDx, anchorMiniContig+10);
    graphXorLine(oldx+oldDx, anchorMiniContig-10, oldx+oldDx, anchorMiniContig+10);
  }
  else if (IsSet(look, FLAG_SHOW_BURIED_CLONES))
  {
    graphXorLine(oldx, anchorMiniContig+10, oldx, anchorMiniContig+50);
  }
  else /*default physical map display*/
  {
    graphXorLine(oldx, 0, oldx, anchorMiniContig-10);
  }
  oldx=x;
  if (IsSet(look, FLAG_EDIT_PANEL))
  {
    DrawCursor(lookDrag, oldx);
  }
  else if (dragFast)
  {
    oldDx*=exp((oldy-y)*ACCEL);
    graphXorLine(oldx-oldDx, anchorMiniContig-10, oldx-oldDx, anchorMiniContig+10);
    graphXorLine(oldx+oldDx, anchorMiniContig-10, oldx+oldDx, anchorMiniContig+10);
  }
  else if (IsSet(look, FLAG_SHOW_BURIED_CLONES))
  {
    graphXorLine(oldx, anchorMiniContig+10, oldx, anchorMiniContig+50);
  }
  else
  {
    graphXorLine(oldx, 0, oldx, anchorMiniContig-10);
  }
  oldy=y;
  return;
}

/*..... ..... ..... .....
*/
static void pMapMiddleUp (double x, double y) 
{ 
  float x1, x2, y1, y2 ;
  LOOK look=lookDrag;

  x += .3 ; /* mieg */
  if (dragFast)
  {
    graphBoxDim(look->scrollBox, &x1, &y1, &x2, &y2);
    look->centre+=(x-look->winWidth*0.5)/oviewXmagn;
    Xmagn*=0.5*(x2-x1)/oldDx;
  }
  else
    look->centre+=(x-look->winWidth*0.5)/Xmagn;
  pMapDraw(look, 0);
  return;
}

#undef ACCEL

/*..... ..... ..... .....
*/
extern void pMapDrawGeneralEntity(int box, int classe, DrawType drawType)
{
  switch(drawType)
  {
  case HIGHLIT:
    graphBoxDraw(box, BLACK, MAGENTA); break;
  case SELECTED:
    graphBoxDraw(box, BLACK/*WHITE*/, RED); break;
  case MODAL_SELECTED:
    graphBoxDraw(box, BLACK, CYAN); break;
  case REGIONAL:
    graphBoxDraw(box, BLACK, LIGHTBLUE); break;
  case DEBUG:
    graphBoxDraw(box, BLACK, GREEN); break;
  case SISTER:
    graphBoxDraw(box, BLACK, LIGHTRED); break;
  case NORMAL:
    switch(classe)
    {
    case _VSequence:
      graphBoxDraw(box, BLACK, YELLOW); break;
    case _VClone:
      graphBoxDraw(box, BLACK, WHITE); break;
    case 0: /*explicit default, if drawType is NORMAL*/
    default:
      graphBoxDraw(box, BLACK, WHITE); break;
    }
    break;
  case BURIED_CLONE:
    graphBoxDraw(box, BLACK, LIGHTGRAY); break;
  default:
    break;
  }
  return;
}

/*..... ..... ..... .....
*/
static void pMapBoxDraw(int box, DrawType drawType, SEG *seg)
{
  if (seg)
    pMapDrawGeneralEntity(box, class(seg->key), drawType);
  else if (box==gMapBox)
    graphBoxDraw(box, BLACK, drawType==SELECTED? GREEN: LIGHTGREEN);

  return;
}

/*..... ..... ..... .....
*/
static void pMapScroll (float *px, float *py, BOOL isDone)
{
  LOOKGET("pMapScroll") ;

  *py = anchorMiniContig - 0.2 ;
  if (isDone)
    { look->centre += (*px - look->winWidth*0.45) / oviewXmagn ;
      pMapDraw (look, 0);
    }
}

/*..... ..... ..... .....
*/

static void pMapResize (void)
{
  LOOKGET("pMapResize");
  pMapDraw(look, 0);
  return;
}

/*..... ..... ..... ..... ..... ..... ..... ..... ..... ..... ..... .....display
*/
static void drawMiniContig(LOOK look, float fac)
{
  SEG *seg;
  float x, lastx=100000.0, height;
  int i;
#define DY 0.75

  Xoffset=ScrolledCentreOffset(Xmagn);
  oviewXoffset=ScrolledCentreOffset(oviewXmagn);

  graphFillRectangle(ModelToOviewX(look->min, fac), anchorMiniContig, ModelToOviewX(look->max, fac), anchorMiniContig+0.2);

  graphTextHeight(0.75);
  for (i=arrayMax(look->segs); --i; )
  {
    seg=arrp(look->segs, i, SEG);
    if (class(seg->key)==_VMyGene || class(seg->key)==_VMyLocus)
    {
      x=ModelToOviewX(SegMidPt(seg), fac);
      if (x<0) break;
      if (x>look->winWidth) continue;
      if (x+strlen(name(seg->key))<lastx)
	height=1;
      else
        height+=DY;
      graphLine(x, anchorMiniContig-height+DY, x, anchorMiniContig);
      graphText(name(seg->key), x,  anchorMiniContig-height);
      lastx=x ;
    }
  }
  graphTextHeight(0 /*back to default*/);

  look->scrollBox=graphBoxStart();

  graphRectangle(GraphToOviewX(0), anchorMiniContig-0.2, GraphToOviewX(look->winWidth), anchorMiniContig+0.4);
  graphBoxEnd();
  graphBoxDraw(look->scrollBox, BLACK, GREEN);
  return;
}

/*..... ..... ..... .....
*/
static void drawBandScale(LOOK look)

#define Resolve(__pix, __limRes, __units, __betterUnits, __pixelsApart) \
{ \
  int __i, __n=1; \
  for (__i=0; __i<100; __i++) \
  { \
    if ((__pix)*__n<(__limRes)) \
      __n*=5; \
    else \
    { \
      (__betterUnits)=__n; \
      (__pixelsApart)=__n*(__pix); \
      break; \
    } \
    if ((__pix)*__n<(__limRes)) \
      __n*=2; \
    else \
    { \
      (__betterUnits)=__n; \
      (__pixelsApart)=__n*(__pix); \
      break; \
    } \
  } \
}

{ 
  int minRes=2 /*pixels*/, uMinRes=50 /*tweak this to get labelling right*/;
  int mapUnits /*no of map units across window*/;
  float x, dy=0.25, pixelsPerUnit, pixelsApart, labelPixelsApart;
  int i, min, max, drawEach=0, labelEach=0;
  int Xshift=look->edit->virtualXorig;

  pixelsPerUnit=look->winWidth/(float) (mapUnits=(int) GraphToModelX(look->winWidth)-GraphToModelX(0));
  Resolve(pixelsPerUnit, minRes, 1, drawEach, pixelsApart);
  Resolve(pixelsApart, uMinRes, drawEach, labelEach, labelPixelsApart);

  for (i=(int) GraphToModelX(0)-Xshift; i%drawEach!=0; i++); /*scan to first drawable unit from left end*/
                                /*this is ok because I only call this routine from within Edit Contig*/

  for (min=(int) GraphToModelX(0)-Xshift, max=(int) GraphToModelX(look->winWidth)-Xshift; i<=max; i+=drawEach)
  {
    x=ModelToGraphX(i+Xshift); /*!*/
    if (i%labelEach==0) /*draw labelled line*/
    {
      graphLine(x, ModelToGraphY(anchorRule), x, ModelToGraphY(anchorRule+2*dy));
      if (!(i==min || i==max)) graphText(messprintf("%d", i), x/*-2.0*/, ModelToGraphY(anchorRule+dy+0.5));
    }
    else if (i%drawEach==0) /*draw little line*/
      graphLine(x, ModelToGraphY(anchorRule), x, ModelToGraphY(anchorRule+dy));
  }
  graphLine(0, ModelToGraphY(anchorRule), look->winWidth, ModelToGraphY(anchorRule));
  return;
}

#undef Resolve

/*..... ..... ..... .....
*/
extern void findWindowCoords(LOOK look)
{
  KEY contig=0;
  OBJ Ctg=NULL;
  edits_t *edit=look->edit;
  lexaddkey(name(look->key), &contig, _VContig);
  if (0<iskey(contig) && (Ctg=bsCreate(contig))!=NULL)
  {
    if (bsGetData(Ctg, _pMap, _Int, (void *) &(edit->modelLL)) && bsGetData(Ctg, _bsRight, _Int, (void *) &(edit->modelRR)))
    {
      edit->modelL=GraphToModelX(0);
      edit->modelR=GraphToModelX(look->winWidth);
      edit->virtualXorig=0;
    }
    else
    {
      messout("Couldn't get data for %s.", name(contig));
    }
    bsDestroy(Ctg);
  }
  else
  {
    messout("Invalid contig (key %d).", contig);
  }
  return;
}

/*..... ..... ..... .....
*/
static void drawMarks(LOOK look)
{
  markHdr_t *hdr=MarkHdr(look);
  mark_t *p;
  for (p=hdr->hd; p!=NULL /*only if empty list or error*/; p=p->next)
  {
      DrawMark(p, hdr->hd, look->edit); /*overwriting out-of-date box indexes*/
    if (p==hdr->tl) break;
  }
  return;
}

/*..... ..... ..... ..... ..... ..... ..... ..... ..... ..... ..... .....menu button hack
*/
#define ButtonOffset(__s) (1.0+strlen(__s))

#define ForceButton(__n, __state) \
{ \
  graphBoxDraw(Menu(look, (__n)).button, BLACK, (__state)? LIGHTGRAY: WHITE); \
  if (__state) /*ON - it becomes current:*/ \
  { \
    look->edit->currentMenu=(__n); \
    graphFreeMenu(pMapMenu, look->menu=Menu(look, (__n)).menu); \
  } \
}

#define SetMenu(__n) \
{ \
  ForceButton(look->edit->currentMenu, OFF); \
  ForceButton((__n), ON); \
}

/*..... ..... ..... .....
*/
static void utilMenuT(void)
{
  LOOKGET("utilMenuT");
  SetMenu(0);
  return;
}

/*..... ..... ..... .....
*/
static void autoMenuT(void)
{
  LOOKGET("autoMenuT");
  SetMenu(1);
  return;
}

/*..... ..... ..... .....
*/
static void markMenuT(void)
{
  LOOKGET("markMenuT");
  SetMenu(2);
  return;
}

/*..... ..... ..... .....
*/
static void clonesMenuT(void)
{
  LOOKGET("clonesMenuT");
  SetMenu(3);
  return;
}

/*..... ..... ..... .....
*/
static void contigMenuT(void)
{
  LOOKGET("contigMenuT");
  SetMenu(4);
  return;
}

/*..... ..... ..... .....
*/
static int nMenus=0;

static void fingerPrint(int box)  /* mieg janv 94 */
{ SEG* seg ;
  extern void fpDisplay(KEY clone) ;
  LOOKGET("fingerPrint");

   seg = array(look->box2seg, box, SEG*) ;
      if(seg && seg->key)
	fpDisplay(seg->key) ;
}

static MENUOPT cloneBoxMenu[] =  /* mieg janv 94 */
    { (VoidRoutine)fingerPrint,"Finger-Print",
	  0, 0 } ;

static void createButtonMenu(LOOK look, void (*actionR) (void), FREEOPT *menu)
  /*
  there is no serious limit to the no of menus: newEditBuf allocates space 
  on the edit panel buffer for PMAP_NMENUS;
  label for menu button is the menu title, held as menu[0].text;
  actionR - VoidRoutine triggered when button is switched to ON - since I can't get hold of the
  box no in this routine, I have to code one explicitly for each button
  menu - stored with the button so I can tell which one I need:

  createButtonMenu: just -
  create a button for the menu, create a menu on it, and add the menu locally so I know which one I'm using on this button:
  */
{
  subMenu_t *m= &Menu(look, nMenus);

  m->button=graphButton(menu->text, actionR, nMenus==0? pMapCueX: (pMapCueX+=ButtonOffset((m-1)->menu->text)), pMapCueY);
    /*
    advance the X coord proportionally to the length of the title of the previous menu (==button label)
    address arithmetic on the array abstraction under Menu is ok, Richard sez
    */
  graphBoxFreeMenu(m->button, (FreeMenuFunction) pMapMenu, menu);
  m->menu=(FREEOPT *) menu;
  nMenus++;

  return;
}

/*..... ..... ..... .....
*/
static void drawCueBar(LOOK look)
{
  /*..... .....menu bar (sort of)

  I need the box nos, for use in makeSisterList, so rather than getting graphButtons to write to an Array of int..
  */
  pMapCueX=0.5; pMapCueY=ModelToGraphY(anchorCueBar-1.5); nMenus=0;
  createButtonMenu(look, utilMenuT, utilMenu);
  createButtonMenu(look, autoMenuT, autoMenu);
  createButtonMenu(look, markMenuT, markMenu);
  createButtonMenu(look, clonesMenuT, clonesMenu);
  createButtonMenu(look, contigMenuT, contigMenu);
  pMapCueX+=ButtonOffset(contigMenu->text);

  if (look->edit->currentMenu==MINUSINF) /*first time*/
  {
    look->edit->currentMenu=1; /*Auto*//*2=marks*/
    ForceButton(1, ON);
  }
  else
    SetMenu(look->edit->currentMenu);
  /*..... .....position Cursor band coord output and cue box*/
  pMapCursorLocX=pMapCueX;
  pMapCueX=pMapCursorLocX+COORDLEN+1.0; /*6 characters + 1.0*/
  /*..... .....*/
  DrawCursorLocText(look); /*might as well leave it around for the user after a redraw*/
  /*..... .....*/
  pMapCue(look, look->edit->cue.cueType, look->edit->cue.cue); /*redraw cue, if any*/
  /*..... .....*/
  graphLine(0.0, ModelToGraphY(anchorCueBar), look->winWidth, ModelToGraphY(anchorCueBar)); /*make it look like a menu bar*/
  return;
}

#undef ButtonOffset
#undef ForceButton
#undef SetMenu

/*..... ..... ..... .....
*/
static char cueBuf[MAXCUE]="";
static char EIGHTYONE_SPACES[82]="                                                                                 ";

extern void pMapCue(LOOK look, cueType_t cueType, char *cue)
{
  if (cue)
    strncpy (cueBuf, cue, MAXCUE-1) ;
  else
    cueBuf[0] = 0 ;

  if (look!=NULL && IsSet(look, FLAG_EDIT_PANEL))
  {
    Graph currGraph=graphActive();
    if (currGraph!=look->mainGraph) graphActivate(look->mainGraph); /*where I want the cue to go*/

    look->edit->cue.cueBox=graphBoxStart(); /*clear previous cue*/
    graphText(messprintf(EIGHTYONE_SPACES), pMapCueX, pMapCueY);
    graphText(messprintf(EIGHTYONE_SPACES), pMapCueX, pMapCueY+LeftYSpace);
    graphBoxEnd();
    graphBoxDraw(look->edit->cue.cueBox, BLACK, WHITE);
    if (cueType==PMAP_CLEAR)
    {
      look->edit->cue.cue[0]='\0';
    }
    else
    {
      look->edit->cue.cueBox=graphBoxStart();
      graphText(messprintf("%s", cueBuf), pMapCueX+LeftXSpace, pMapCueY+LeftYSpace);
      graphRectangle(pMapCueX, pMapCueY, pMapCueX+(cue!=NULL? strlen(cueBuf): 0.5*MAXCUE /*nominally*/)+0.8, pMapCueY+RightYSpace);
      graphBoxEnd();
      graphBoxDraw(look->edit->cue.cueBox, BLACK, 
        cueType==PMAP_WORKING? LIGHTRED:
        cueType==PMAP_CUE? LIGHTBLUE:
        cueType==PMAP_INTERACTIVE? YELLOW:
        /*PMAP_INFORMATION*/ WHITE);

      strncpy(look->edit->cue.cue, cueBuf, MAXCUE);
      look->edit->cue.cue[(strlen(cueBuf)<MAXCUE-1)? strlen(cueBuf): MAXCUE-1]='\0';
    }
    look->edit->cue.cueType=cueType;
    if (currGraph!=look->mainGraph) graphActivate(currGraph);
  }
  else
    mainActivity(cue==NULL? NULL: cueBuf);
  return;
}

/*..... ..... ..... ..... ..... ..... ..... ..... ..... ..... ..... .....
*/
static int nYAC = 5 ;
/*  static int nReg = 10 ;	see up when buried */
static int nProbe = 2 ;

/*..... ..... ..... .....
*/
static void setDisplayDepths (void)
{
  if (messPrompt ("Numbers of lines for probes, virtual clones, fingerprinted clones:",
		  messprintf ("%d %d %d", nProbe, nYAC, nReg),
		  "iiiz"))
    { freeint (&nProbe) ;
      freeint (&nYAC) ;
      freeint (&nReg) ;
    }
}

/*..... ..... ..... .....
*/
static void pMapShowSegs(LOOK look, KEY key)
{
  int i, box, keybox=0;
  float xL, xR, oldxR, y;
  SEG *seg ;
  char *text ;
  BUMP remBump,geneBump,probeBump ;
  int iYac = 0, iReg = 0 ;
  int nYAC=5;

  /*..... ..... ..... .....find out what arrangement required*/
  if (IsSet(look, FLAG_EDIT_PANEL))
  {
    anchorCueBar=1;
    anchorSequence=anchorCueBar+1;
    anchorProbes=anchorSequence+nProbe;
    anchorYACs=anchorProbes+nYAC+1;
    anchorClones=anchorYACs+nReg+1;
    anchorGenes=anchorClones+2;
    anchorRule=anchorGenes+2;

    switch(remarksDisplay)
    {
    case 0:
      anchorRemarks=5000; /*take remarks off-screen*/
      /*
      in which case I don't want them displayed, but I do want to manipulate them (and I have to do this via graph 
      boxes (see eg makeSisterSegs))
      */
      break;
    case 1:
    case 2:
      anchorRemarks=anchorRule+4;
      break;
    }
  }
  else if (IsSet(look, FLAG_SHOW_BURIED_CLONES))
  {
    anchorCueBar=0; /*NONE*/
    anchorMiniContig=anchorCueBar+7;
    anchorScrollHelp=anchorMiniContig+1;
    anchorSequence=anchorScrollHelp+1;
    anchorProbes=anchorSequence+nProbe;
    anchorYACs=anchorProbes+nYAC+1;
    anchorClones=anchorYACs+nReg+1;
    anchorNavigBar=anchorRule=anchorClones+2;
    anchorGenes=anchorRule+1;
    anchorRemarks=anchorGenes+3;
  }
  else /*ordinary pmap display*/
  {
    anchorCueBar=0; /*NONE*/
    anchorProbes= -2.25+nProbe /*<<--0.75*/;
    anchorYACs=anchorProbes+nYAC+1;
    anchorClones=anchorYACs+nReg+1;
    anchorSequence=anchorClones+1;
    anchorNavigBar=anchorSequence+1;
    anchorGenes=anchorNavigBar+1;
    anchorRemarks=anchorGenes+2;
    anchorMiniContig=look->winHeight-2;
    anchorScrollHelp=look->winHeight-1; /*make sure it's visible whatever the window size*/
  }

  /*..... ..... ..... .....*/
  probeBump=bumpCreate(nProbe, anchorProbes);
  geneBump=bumpCreate(3, anchorGenes);
  remBump=bumpCreate(7, anchorRemarks);
  /*..... ..... ..... .....*/

          /* Direct interpolating move from the gmap */
          /* The position is passed approximately in pmap units */

  if (class(key)==_VCalcul)
  {
    xL=(int) KEYKEY(key)-(int) 0x800000;
    look->centre=xL=Trunc(look->min, xL, look->max);
  }
  else if (key)
  {
    for (i=arrayMax(look->segs); --i; )
    {
      if (arrp(look->segs, i, SEG)->key==key)
      {
        look->centre=SegMidPt(arrp(look->segs, i, SEG));
	break;
      }
    }
  }
  if (key==0)
   {

    if (IsSet(look, FLAG_EDIT_PANEL) && AskCurrentSelected(look)!=0)
      /*
      editing contig - I may have just sorted the seg array, and the box2seg array be completely wrong:
      if so, I saved the relevant seg on the edit buffer;
      however, if I select an object in the default pmap overview and then enter Edit Contig, I want
      to preserve that selection, so on Overview Physical Map I reset CurrentSelected on the edit buffer.
      If this is a new entity introduced by Attach Clone, eg, there is not yet a seg for it.
      */
    {
      key=AskCurrentSelected(look);
      KeyToSeg(look->segs, key, seg);
    }
    else if (look->activebox && (seg=arr(look->box2seg, look->activebox, SEG*)))
    {
      key=seg->key;
    }
  }

  look->activebox=0;
  arrayMax(look->box2seg)=0;

  Xoffset=ScrolledCentreOffset(Xmagn);

  /*..... ..... ..... .....draw mini-contig for default display: takes care of buried clones display*/
  if (IsntSet(look, FLAG_EDIT_PANEL))
  {
    drawMiniContig(look, 0.0); DrawScrollHelp(anchorScrollHelp);
  }
  /*..... ..... ..... .....*/
  if (IsSet(look, FLAG_EDIT_PANEL)) drawCueBar(look);
  /*..... ..... ..... .....*/
  if (IsSet(look, FLAG_EDIT_PANEL)) /*draw scale*/
  {
    drawBandScale(look);
  }
  else /*draw green bar - navigation box to the gMap*/
  {
    array(look->box2seg, gMapBox=graphBoxStart(), SEG*)=0;
    graphRectangle(0, ModelToGraphY(anchorNavigBar+0.2), look->winWidth, ModelToGraphY(anchorNavigBar+0.8));
    graphTextHeight(0.6);
    for (i=10; i<look->winWidth; i+=40)
    {
      graphText("Pick here to go to the genetic map", i, ModelToGraphY(anchorNavigBar+0.25));
    }
    graphTextHeight(0 /*back to default*/);
    graphBoxEnd();
    graphBoxDraw(gMapBox, BLACK, LIGHTGREEN);
  }
  /*..... ..... ..... .....*/
  oldxR = -10000001.0 ;
  for (i=1 /*seg[0] is a dummy so don't touch it*/; i<arrayMax(look->segs); ++i)
  {
    box = 0 ;
    seg = arrp(look->segs, i, SEG);

    if (IsSet(seg, FLAG_HIDE)) continue;
    xL=seg->x0; xR=SegRightEnd(seg);
    if (ModelToGraphX(xR)<0)
      /*off the left side of the window, but must ensure that the vertical placing of clones, remarks, genes etc. stays constant*/
    {
      switch (class(seg->key))
      {
      case _VClone:
	if (IsSet(seg, FLAG_FINGERPRINT))
	{
          if (oldxR<xL) iReg=0;
	  if (oldxR<xR) oldxR=xR;
	  if (IsSet(seg, FLAG_DISPLAY) || IsntSet(seg, FLAG_IS_BURIED_CLONE) || IsSet(look, FLAG_SHOW_BURIED_CLONES))
            /*FLAG_DISPLAY - display this buried clone*/
          {
             ++iReg;
	  }
	}
	else if (IsntSet(seg, FLAG_PROBE))
	  ++iYac ;
	break ;
      case _VMyGene:
      case _VMyLocus:
        bumpNoWrite (look, geneBump, name(seg->key), SegMidPt(seg));
        break ;
      case _VText:
        if (IsntSet(look, FLAG_SHOW_WORK_REMARKS) && IsSet(seg, FLAG_WORK_REMARK)) break;
        if (IsSet(seg, FLAG_MORE_TEXT))
	  text = messprintf("%s(*)", name(seg->key));
        else
	  text = name(seg->key);
	bumpNoWrite(look, remBump, text, SegMidPt(seg));
	break ;
      }
      continue; /*skip drawing*/
    }
    if (look->winWidth<ModelToGraphX(xL)) continue;	/* no need to be so careful */
    switch (class(seg->key))
    {
    case _VClone:
      if (IsSet(seg, FLAG_IS_BURIED_CLONE) && IsntSet(seg, FLAG_DISPLAY) && IsntSet(look, FLAG_SHOW_BURIED_CLONES))
      {
        break;
      }
      if (IsSet(seg, FLAG_FINGERPRINT))
      {
        if (oldxR<xL)
	{ 
          iReg=0; /* start new islands from bottom */
	  if (-10000000.0<oldxR)
	  {
            DrawLine(xL-7, anchorClones, xL-7, anchorClones+1);
	    DrawLine(xL-8, anchorClones, xL-8, anchorClones+1);
          }
	}
        box = graphBoxStart() ;
	graphBoxMenu(box, cloneBoxMenu) ;  /* mieg janv 94 */

        if (oldxR<xR) oldxR=xR;
	y = (anchorClones+0.5) - (iReg%nReg) - 0.5*((iReg/nReg)%2) ;
	if (IsSet(seg, FLAG_COSMID_GRID))
          if (IsSet(seg, FLAG_GIVEN_INTERIM_LOC))
	    DrawRectangle(xL, y-0.1, xR, y+0.1);
          else
	    DrawFilledRectangle(xL, y-0.1, xR, y+0.1);
	else
	  DrawLine(xL, y, xR, y);
	if (IsSet(seg, FLAG_CANONICAL))
	  text=messprintf("%s *", name(seg->key));
        else
	  text = name(seg->key) ;

        DrawSegText(text, SegMidPt(seg), y);

	++iReg ;
      }
      else if (IsSet(seg, FLAG_PROBE))
      {
        box = graphBoxStart() ;
	graphBoxMenu(box, cloneBoxMenu) ; /* mieg janv 94 */

        bumpWrite(look, probeBump, name(seg->key), SegMidPt(seg));
      }
      else /*YACs*/
      {
        y = anchorYACs /*was 1+5*/ - (iYac%nYAC) + 0.5*((iYac/nYAC)%2) ;
        if (IsSet(seg, FLAG_YAC_GRID))
          if (IsSet(seg, FLAG_GIVEN_INTERIM_LOC))
	    DrawRectangle(xL, y-0.1, xR, y+0.1);
          else
	    DrawFilledRectangle(xL, y-0.1, xR, y+0.1);
	else
	  DrawLine(xL, y, xR, y);
        box = graphBoxStart() ;
        text = messprintf ("(%s)", name(seg->key)) ;
        graphText(text, ModelToGraphX(SegMidPt(seg))-3.5, ModelToGraphY(y-0.4));
        ++iYac ;
      }
      graphBoxEnd() ;
      graphBoxDraw (box,BLACK, TRANSPARENT) ;
      break ;
    case _VSequence:
      box=graphBoxStart();
      DrawRectangle(xL, anchorSequence+0.2, xR, anchorSequence+0.8);
      graphBoxEnd();
      break;
    case _VMyGene:
    case _VMyLocus:
      box=graphBoxStart();
      bumpWrite(look, geneBump, name(seg->key), SegMidPt(seg));
      graphBoxEnd();
      break;
    case _VText:
      if (IsntSet(look, FLAG_SHOW_WORK_REMARKS) && IsSet(seg, FLAG_WORK_REMARK)) break;
      box = graphBoxStart() ;
      if (IsSet(seg, FLAG_MORE_TEXT))
        text = messprintf("%s(*)", name(seg->key));
      else
        text = name(seg->key) ;
      bumpWrite(look, remBump, text, SegMidPt(seg));
      graphBoxEnd();
      break ;
    default:
      messout ("Don't know how to put '%s' on a physical map.", name(seg->key));
    }
    if (box) /*something drawn*/
      {
/*for showing clones matched against in region while calculating positionClone:*/
      if (class(seg->key) != _VClone ||  /* unset clones are better drawn transparent */
	  IsSet(seg, FLAG_HIGHLIGHT) )
	  pMapBoxDraw(box, IsSet(seg, FLAG_HIGHLIGHT)? HIGHLIT:
		/**IsSet(seg, FLAG_AUTOPOSITIONING_AGAINST)? REGIONAL:**/ NORMAL, seg);
      array(look->box2seg, box, SEG*)=seg;
      if(seg->key==key) keybox=box;

    }
  }

  if (IsSet(look, FLAG_EDIT_PANEL))
  {
    /*draw marks*/
  }

  if (keybox && IsntSet(look, FLAG_REVERT_DISPLAY))
  {
    pMapSelectBox(look, keybox);
  }

  /*..... ..... ..... .....*/
  bumpDestroy(remBump);
  bumpDestroy(geneBump);
  bumpDestroy(probeBump);
  return;
}

/*..... ..... ..... ..... ..... ..... ..... ..... ..... ..... ..... .....*/

Array sisterBox=0, sisterSeg=0;

static void addPositiveYacs (OBJ obj, Array box2seg, int min, int max)
{
  static Array flat = 0 ;
  int nsib = arrayMax (sisterBox) ;
  KEY yac ;
  int i, j ;
  SEG *subseg ;

  flat = arrayReCreate (flat, 32, BSunit) ;

  if (bsFindTag (obj, _Hybridizes_to) && bsFlatten (obj, 2, flat))
    for (j = 0 ; j < arrayMax(flat) ; j += 2)
      { yac = arr(flat, j+1, BSunit).k ;
	for (i = min ; i <= max ; ++i)
	  if ((subseg = arr(box2seg,i,SEG*))->key == yac)
	    { array(sisterBox, nsib, int) = i ;
	      array(sisterSeg, nsib++, SEG*) = subseg ;
	    }
      }
}

static void addPositiveProbes (OBJ obj, Array box2seg, int min, int max)
{
  int nsib = arrayMax (sisterBox) ;
  KEY probe ;
  int i ;
  SEG *subseg ;

  if (bsGetKey (obj, _Positive_probe, &probe)) do
    for (i = min ; i <= max ; ++i)
      if ((subseg = arr(box2seg,i,SEG*))->key == probe)
	{ array(sisterBox, nsib, int) = i ;
	  array(sisterSeg, nsib++, SEG*) = subseg ;
	}
    while (bsGetKey (obj, _bsDown, &probe)) ;
}

/*..... ..... ..... .....
*/
extern void makeSisterList (LOOK look, Array box2seg, int activebox, int segmax)
{
  OBJ obj ;
  SEG	*seg, *subseg ;
  int	i, min, max, nsib = 0 ;

  if (sisterBox)
    arrayMax(sisterBox) = arrayMax(sisterSeg) = 0 ;
  else
    { sisterBox = arrayCreate (8, int) ;
      sisterSeg = arrayCreate (8, SEG*) ;
    }
 
  seg = arr(box2seg,activebox,SEG*) ;

  min = max = activebox ;

  for (i=activebox; 1<--i && arr(box2seg, i, SEG *)!=NULL && SegMidPt(seg)-250<SegMidPt(arr(box2seg, i, SEG *)); )
    /*<<--test arr(box2seg,i,SEG*) neil 16 Apr 92 - this failed before because when I drew the 
      navig box (box 2) at start of showSegs, it had no seg*/
    min = i ;
  for (i=activebox; ++i<segmax && arr(box2seg, i, SEG *)!=NULL && SegMidPt(arr(box2seg, i, SEG *))<SegMidPt(seg)+250; )
    /*<<--test arr(box2seg,i,SEG*) neil 16 Apr 92*/
    max = i ;

  for (i=min; i<=max; i++)
  {
    if (i!=activebox && (IsSet(look, FLAG_EDIT_PANEL)? !IsReservedBox(look, i): TRUE) &&
      (subseg=arr(box2seg, i, SEG*)) /*&& NeighbouringMidPt(seg, subseg, 2)*/ && subseg->parent==seg->parent)
      /*
      NeighbouringMidPt condition may be wrong when buried clones have autonomous positions: besides, it is the
      same parent condition which is important
      */
    {
      array(sisterBox, nsib, int) = i ;
      array(sisterSeg, nsib++, SEG*) = subseg ;
    }
  }
  if (class (seg->key) == _VClone && (obj = bsCreate (seg->key)))
    { addPositiveProbes (obj, box2seg, min, max) ;
      if (IsSet(seg, FLAG_PROBE))
	addPositiveYacs (obj, box2seg, min, max) ;
      bsDestroy (obj) ;
    }
}

/*********************************************************/

static void pMapSelectBox (LOOK look, int box)	/* switch activebox */
{
  int i ;
  SEG *seg ;

  if (look->activebox) /*deselect the current selection*/
  {
    seg=arr(look->box2seg, look->activebox, SEG*);
    pMapBoxDraw(
      look->activebox, 
      seg? (IsSet(seg, FLAG_HIGHLIGHT)? HIGHLIT: /**IsSet(seg, FLAG_AUTOPOSITIONING_AGAINST)? REGIONAL:**/ NORMAL): NORMAL,
      seg
    );
    if (seg) /*i.e. not for special boxes*/
    {
      makeSisterList(look, look->box2seg, look->activebox, arrayMax(look->box2seg));
      for (i=arrayMax(sisterBox); i--; )
      {
        SEG *sSeg=array(sisterSeg, i, SEG *);
        pMapBoxDraw(
          arr(sisterBox, i, int),
          IsSet(sSeg, FLAG_HIGHLIGHT)? HIGHLIT: /**IsSet(sSeg, FLAG_AUTOPOSITIONING_AGAINST)? REGIONAL:**/ NORMAL,
          sSeg
        );
      }
    }
  }
  if (box!=0) /*select the given box*/
  {
    seg=arr(look->box2seg, box, SEG*);
    pMapBoxDraw(box, SELECTED, seg);
    if (seg) /*i.e. not for special boxes*/
    {
      makeSisterList(look, look->box2seg, box, arrayMax(look->box2seg));
      for(i=arrayMax(sisterBox); i--; )
      {
        pMapBoxDraw(
          arr(sisterBox, i, int),
          ((arr(sisterSeg, i, SEG*))->flag & FLAG_IS_BURIED_CLONE)? BURIED_CLONE: SISTER,
          arr(sisterSeg, i, SEG*)
        );
      }
    }
  }  /*else just deselect whatever is currently selected*/
  if ((look->activebox=box)==0 || !seg)
    SetCurrentSelected(look, 0);
  else
    SetCurrentSelected(look, seg->key);
  return;
}

/*********************************************************/

extern int sequenceLength(KEY seq);

extern void pMapFollow(LOOK look, double x, double y)
{ KEY  from ;
  SEG  *seg ;

  if (look->activebox == gMapBox)
    { KEY contig ;

      if (!lexword2key(name(look->key), &contig, _VContig))
	return ;
      pMapToGMap (contig, 0, 
	  (int)(look->centre + (x - look->winWidth/2)/Xmagn)) ;
    }
  else
    { seg = arr(look->box2seg,look->activebox,SEG*) ;
      
      switch (class(seg->key))
	{
	case _VMyGene: 
	case _VMyLocus: 
	  display (seg->key, look->key, GMAP) ;
	  break ;
	case _VSequence:
	  if (SegLen(seg)!=0)
	    from=KEYMAKE(_VCalcul, sequenceLength(seg->key)*x/(20.0*Xmagn*(0.5*SegLen(seg))/*ie old dx*/));
	  else
	    from=0;
	  display(seg->key, from, 0);
	  break ;
	case _VClone:
	  display (seg->key, look->key, TREE) ;
	  break ;
	case _VText:
	  display (seg->parent, look->key, TREE) ;
	  break ;
	default:
	  display (seg->key, look->key, 0) ;
	}
    }
}

/*.......... ..... .....
*/

static void pMapHighlight(KEYSET k, KEY key)
{
  int i, j;
  SEG *seg;
  void *dummy;
  KEYSET kSet=0; 
  LOOKGET("pMapHighlight");

  if (k!=0 && keySetExists(k) /*I probably ought to check this more thoroughly*/) kSet=k;
  if (kSet==0 && !keySetActive(&kSet, &dummy))
  {
     messout("First select a keySet window, thank you.");
     return;
  }
  seg=arrp(look->segs, 0, SEG);
  i=arrayMax(look->segs);
  while (i--)
  {
    seg->flag&= ~FLAG_HIGHLIGHT; /*clear this bit on all segs, except for selected objects:*/
    if (keySetFind(kSet, seg->key, &j)) seg->flag|=FLAG_HIGHLIGHT;
    seg++;
  }
  pMapDraw(look, (k==0? 0: key));
  return;
}

/*..... ..... ..... .....external service call routines
*/
extern void pMapSetHighlightKeySet(KEYSET k)
{
  pMapPendingKeySet=k;
  return;
}

/*..... ..... ..... .....
*/
extern BOOL pMapGetCMapKeySet(KEY contig, int *xMin, int *xMax, KEYSET clones)
{
  messout("**pMapGetCMapKeySet not implemented.");
  return FALSE;
}

/*..... ..... ..... ..... ..... ..... ..... ..... ..... ..... ..... .....
*/
extern LOOK pMapLook(char *caller, void *at)
{
  LOOK look;
  if (!graphAssFind(at, &look))
    messcrash("%s couldn't find graph.", caller);
  if (!look)
    messcrash("%s received a null graph pointer.", caller);
  if (look->magic!=MAGIC)
    messcrash ("%s received an invalid graph pointer: check value %d not %d.", caller, look->magic, MAGIC);
  return look;
}

/*..... ..... ..... .....
*/
extern LOOK pMapLookTry(char *caller, void *at)
{
  LOOK look;
  return (!graphAssFind(at, &look) || !look || look->magic!=MAGIC)? NULL: look;
}

/****************************************************************/
/******** RMD copied from pmaped.c to make stand-alone **********/

extern BOOL pMapLiftBuriedClones(Array segs, SEG *canonical, BOOL setDisplayFlag)
{
  KEY child ;
  OBJ Parent, Child ;
  float x = SegMidPt(canonical) ;
  static BSMARK mark ;

  if (Parent = bsCreate(canonical->key))
    {
      if (bsGetKey (Parent, _Canonical_for, &child)) do
  /*at least one valid buried clone: get all buried clones for this seg*/
	{
	  int nBands, left, right ;
	  SEG *seg = arrayp(segs, arrayMax(segs), SEG) ; /* new seg */

	  mark = bsMark (Parent, mark) ;

	  seg->key = child ;
	  seg->parent = canonical->key ;
	  if (bsGetData (Parent, _bsRight, _Int, &left) &&
	      bsGetData (Parent, _bsRight, _Int, &right))
	    { seg->x0 = canonical->x0 + left ;
	      seg->x1 = canonical->x0 + right ;
	      Set(seg, FLAG_BURIED_IS_POSITIONED);
	    }
	  else
	    { if (Child = bsCreate(child))
		{ if (!bsGetData (Child, _Bands, _Int, &nBands) ||
		      !bsGetData (Child, _bsRight, _Int, &nBands))
		    nBands = 20 ;
		  bsDestroy (Child) ;
		}
	      else
		nBands = 20 ;
	      seg->x0 = x - nBands/2 ; 
	      seg->x1 = x + nBands/2 ;
	    }
	  Set(seg, FLAG_IS_BURIED_CLONE);
	  Set(seg, FLAG_FINGERPRINT);
	  if (setDisplayFlag) Set(seg, FLAG_DISPLAY);
	  bsGoto (Parent, mark) ;
	} while (bsGetKey (Parent, _bsDown, &child)) ;

      bsDestroy(Parent) ;
      return TRUE;
    }
  else
    { messout ("Couldn't find canonical clone %s", 
	       name(canonical->key));
      return FALSE;
    }
}

extern void displayAllBuriedClones(LOOK look)
{
  int i ;
  SEG *pseg ;

  if (IsntSet(look, FLAG_BURIED_CLONES_ATTACHED))
    { pMapCue (look, PMAP_WORKING, 
	       "Attaching buried clones: PLEASE WAIT...") ;
      
      for (i = 1 ; i < arrayMax (look->segs) ; ++i)
	{ pseg = arrp(look->segs, i, SEG) ;
	  if (IsntSet(pseg, FLAG_DISPLAY_BURIED_CLONES))
    /* if it is, then its buried clones are already loaded and checked */
	    pMapLiftBuriedClones(look->segs, pseg, FALSE) ;
	}

      arraySort(look->segs, pMapCompareSeg);
      Set(look, FLAG_BURIED_CLONES_ATTACHED);
      pMapCue(look, PMAP_CLEAR, NULL);
    }
  return;
}

/*..... ..... ..... ..... ..... ..... ..... ..... ..... ..... ..... .....end of file*/

