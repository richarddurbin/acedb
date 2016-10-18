/**********************************************************************
 * File: graph.h 
 * Authors: Richard Durbin plus 
 *		 Jean Thierry-Mieg
 *		 and Christopher Lee
 * Copyright (C) J Thierry-Mieg and R Durbin, 1991-97
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
 * Richard Durbin (MRC LMB, UK) rd@mrc-lmba.cam.ac.uk, and
 * Jean Thierry-Mieg (CRBM du CNRS, France) mieg@kaa.cnrs-mop.fr
 *
 * Description: Public header file for the graph package, this package
 *              is independent of the acedb database code but requires
 *		the array and messubs packages.
 *
 * Exported functions:
 * HISTORY:
 * Last edited: May 21 17:18 2009 (edgrif)
 * * Feb  8 16:34 1999 (edgrif): Add calls to support sprdmap.c
 * * Jan 27 20:31 1999 (edgrif): Remove WIN32 specific graphTextAlign.
 * * Jan 27 10:32 1999 (edgrif): Remove type definitions for menu stuff (!!)
 * * Jan 21 14:43 1999 (edgrif): Added new prototype for graphPS,
 *              graphPSDefaults, graphGIF.
 * * Jan 20 16:25 1999 (edgrif): Changed funcdef for graphPS/graphPrint
 * * Jan  8 11:31 1999 (edgrif): Add correct func. dec. for graphPS.
 * * Dec 16 15:10 1998 (edgrif): Removed waitCursor calls, now done internally.
 * * Nov 19 15:02 1998 (edgrif): : Fixed callback func. dec. for
 *              graphTextScrollEditor.
 * * Oct 22 14:15 1998 (edgrif): Added message output functions that are
 *              independent of device level (X Windows etc), e.g. graphOut.
 *              Removed isGraphics flag.
 * * Oct 13 14:22 1998 (edgrif): Removed some acedb specific functions that
 *              do not belong here.
 * * May 14 08:00 1997 (rbrusk):
 *              - introduced GRAPH_FUNC_DCL symbol; see mystdlib.h
 *              - added graphSysBeep(), extern int menuBox
 *              - added an extra void* parameter to graphInit() to help
 *                provide for other program initialization data (in WIN32)
 *              - help() & helpOn() moved from acedb.h to graph.h w/ GRAPH_FUNC_DCL
 * * Oct 21 16:54 1996 (il) 
 * Created: Jan  1992 (rd)
 * CVS info:   $Id: graph.h,v 1.136 2012-05-01 16:17:39 gb10 Exp $
 *-------------------------------------------------------------------
 */
#ifndef DEF_GRAPH_H
#define DEF_GRAPH_H

#include <wh/regular.h>		/* libutil header */
#include <wh/aceiotypes.h>
#include <wh/colours.h>		/* for enum Colour{..} */
#include <wh/menu.h>
#include <wh/key.h>
#include <gtk/gtk.h>

/**********************************************************************/
/******* be nice to C++ - patch from Steven Ness **********************/
/**********************************************************************/
#ifdef  __cplusplus
extern "C" {
#endif

  /* library EXPORT/IMPORT symbols */


/* Applications refer to graphs by Graph ids which hide the actual graph     */
/* structure from the caller.                                                */
/* A Graph id may be safely initialised to GRAPH_NULL as no graphs are       */
/* created with this id.                                                     */
/* (developers: you should _not_ change the value of GRAPH_NULL without      */
/*  searching through the source code extensively as just about all of it    */
/*  uses "0" rather than GRAPH_NULL)                                         */
typedef int Graph ;
enum {GRAPH_NULL = 0} ;



extern int graphContrastLookup[];
#define graphContrast(x) (graphContrastLookup[(x)])

enum TextFormat { PLAIN_FORMAT, BOLD, ITALIC, GREEK, 
		   FIXED_WIDTH} ; /* The implementation is machine dependant */

#define BACK_COLOR WHITE
#define FORE_COLOR BLACK
	/* max 256 colours (box->fcol, bcol are unsigned char) */

/* GraphEvent note the mouse events are required to be continuous in
   this way. Can't touch the order.
   DESTROY must be last event and no values must be assigned to the
   enums so that they are contiguous.
*/
enum GraphEvent {LEFT_DOWN, LEFT_DRAG, LEFT_UP,
		 MIDDLE_DOWN, MIDDLE_DRAG, MIDDLE_UP,
		 RIGHT_DOWN, RIGHT_DRAG, RIGHT_UP,
		 PICK, KEYBOARD, RESIZE, 
		 MESSAGE_DESTROY,
		 RAMP_CHANGE, 
		 DESTROY
} ;


/* Specific typedefs for old style callback funcs, note they have no "app-data" ptrs. */
typedef void (*GraphFunc)(void) ;
typedef void (*MouseFunc)(double x, double y) ;
typedef void (*KeyBoardFunc)(int key, int modifier) ;
typedef void (*PickFunc)(int box, double x, double y, int modifier) ;
typedef void (*EntryFunc)(char*) ;
typedef BOOL (*ToggleFunc)(BOOL) ;
typedef void (*ColouredButtonFunc)(void *arg) ;
typedef int  (*GraphCompletionFunc)(char *text, int len) ; /* return # */
typedef char* (*GraphRemoteHandlerFunc)(char *command) ; /* return textual result */


enum GraphType  { PLAIN, TEXT_SCROLL, TEXT_FIT, MAP_SCROLL, 
		  PIXEL_SCROLL, TEXT_FULL_SCROLL, PIXEL_FIT,
		  TEXT_FULL_EDIT, PIXEL_VSCROLL, PIXEL_HSCROLL,
		  TEXT_HSCROLL,
		  NUMGRAPHTYPES } ; /* always last */

typedef enum { 
  PAPERTYPE_A4,
  PAPERTYPE_LETTER,
  PAPERTYPE_LEGAL,
  PAPERTYPE_EXECUTIVE
} PaperType;
  

extern float    graphEventX, graphEventY ;
extern FREEOPT  graphColors[] ;         /* defined in graphDevSubs.c */



/* Application can register callback routines that are common to many windowing systems
 * and the common function will then be called for all graph windows. If no callback is
 * specified then the event will be passed on so that any separate handlers registered
 * by the application will be called. */
typedef void (*GraphCallBack)(void *app_data) ;

typedef struct
{
  GraphCallBack func ;					    /* func to be called. */
  void *data ;						    /* app date that will be passed to func. */
} GraphCallBackRegStruct, *GraphCallBackReg ;

typedef struct
{
  GraphCallBackRegStruct copy ;				    /* Called for Cntl-C */
  GraphCallBackRegStruct cut ;				    /* Called for Cntl-X */
  GraphCallBackRegStruct paste ;			    /* Called for Cntl-V */

  GraphCallBackRegStruct help ;				    /* Called for Cntl-H */
  GraphCallBackRegStruct close ;			    /* Called for Cntl-W */
  GraphCallBackRegStruct print ;			    /* Called for Cntl-P */
  GraphCallBackRegStruct save ;				    /* Called for Cntl-S */
  GraphCallBackRegStruct quit ;				    /* Called for Cntl-Q */
} GraphGlobalCallbacksStruct, *GraphGlobalCallbacks ;




/**********************************************************/
/************ functions used only by the Kernel ***********/
/**********************************************************/ 
/* register a handler routine that deals with remote-control commands,
 *  before graphInit, which can dispatch actions, if a -remote arg 
 *  was detected in the command-line arguments. */
GraphRemoteHandlerFunc graphRemoteRegister (GraphRemoteHandlerFunc func);


void    graphInit (int *argcptr, char **argv) ;
void    graphFinish (void) ;
void    graphProcessEvents (void) ; /* process events in queue */
				/* NB should use graphLoop instead */

void graphRegisterGlobalCallbacks(GraphGlobalCallbacks app_global_callbacks) ;
							    /* Register routines to be called back by
							       graph from any window.*/

/* graphSelect can either show selections in a scrolled subwindow (default)  */
/* or in plain window, graphSetSelectDisplayMode() is used to set the mode.  */
/*                                                                           */
typedef enum _GraphSelectDisplayMode {GRAPH_SELECT_SCROLLED,
				      GRAPH_SELECT_PLAIN} GraphSelectDisplayMode ;
void graphSetSelectDisplayMode(GraphSelectDisplayMode mode) ;
BOOL graphSelect (KEY *kpt, FREEOPT *options) ;


/**********************************************************/
/********** functions used by the applications ************/
/**********************************************************/


/* Make graphs of various kinds, type is PLAIN, TEXT_SCROLL etc.,
 * title is window title,  x,y,w,h are in units of screen height = 1.0 
 * or may be negative, in which case they are in units of -1 char or pixel */
Graph graphCreate(int type, char *title, float x, float y, float w, float h) ;
							    /* Standard graph. */
Graph graphNakedCreate(int type, char *title, float w, float h, BOOL focusOnMouseOver) ;
							    /* Creates a frame with no graph for
							       delayed addition of graph. */
Graph graphNakedResizeCreate(int type, char *title, float w, float h, BOOL focusOnMouseOver) ;
							    /* As for "Naked" but window size is
							       evaluated later. */
Graph graphVirtualCreate(int type, char *title, float w, float h) ;
							    /* Has no windows at all, can draw
							       graphics into it and then dump them
							       as postscript etc. */


BOOL    graphActivate (Graph graph) ;	    /* returns graphExists() */
BOOL    graphActivateChild (void) ;	    /* retruns graphActivate gActive->children */
Graph   graphActive (void) ;		    /* returns active graph */
BOOL    graphExists (Graph g) ;		    /* is g a valid graph? */

/* Graphs can be of three types for purposes of user interactions:           */
/*       GRAPH_BLOCKABLE - normal window (default), input blocked when a     */
/*                         modal dialog is posted.                           */
/*   GRAPH_NON_BLOCKABLE - normal window but input is not blocked when a     */
/*                         modal dialog is posted.                           */
/*        GRAPH_BLOCKING - modal dialog window, cannot itself be blocked,    */
/*                         there can only be one of these at a time.         */
/*                                                                           */
typedef enum _GraphWindowBlockMode {GRAPH_BLOCKABLE, GRAPH_NON_BLOCKABLE,
				    GRAPH_BLOCKING} GraphWindowBlockMode ;
void graphSetBlockMode(GraphWindowBlockMode mode) ;


/************** subgraphs ***************/

Graph graphSubCreate (int type, float ux, float uy, float uw, float uh) ;
Graph graphParent (void) ;	/* 0 if not a subgraph */
BOOL  graphContainedIn (Graph parent) ;

/***************************************************************/ 
/******** everything from here on acts on active graph *********/

void    graphDestroy (void) ;
int     graphLoop (BOOL isBlock) ; /* X event loop - now reentrant */
BOOL	graphLoopReturn (int retval) ; /* returns */
				/* old names for these functions */
#define graphStart(g)	graphLoop(FALSE)
#define graphBlock()	graphLoop(TRUE)
#define graphUnBlock(x)	graphLoopReturn(x)

STORE_HANDLE graphHandle (void) ; /* gets freed when graph is freed */
STORE_HANDLE graphClearHandle (void) ; /* freed when graph is cleared */

void    graphCleanUp (void) ;	/* kills all but the active graph */

BOOL    uGraphAssociate (void* in, void* out) ;
#define graphAssociate(xin,xout)   uGraphAssociate((void*)(xin),(void*)(xout))
BOOL    uGraphAssFind (void *in, void* *out) ;
#define graphAssFind(xin,pxout)	uGraphAssFind(xin,(void**)(pxout))
BOOL    graphAssRemove (void *in) ;

void    graphGetBounds(int *nx, int *ny) ;
void    graphRawBounds(int nx, int ny) ;
void    graphPlainBounds (float ux, float uy, float uw, float uh, float aspect) ;
void    graphMapBounds (float ux, float uy, float uw, float uh, float aspect) ;
void    graphTextBounds (int nx, int ny) ;
void    graphPixelBounds (int nx, int ny) ;
float   graphFakeBounds (float ny) ; /* in lines, to prepare a long print on a virtual window */
void    graphFitBounds (int *nx, int *ny) ;  /* no scroll - in text coords */
void    graphTextInfo (int *dx, int *dy, float *cw, float *ch) ;
void    graphScreenSize (float *sx, float *sy, float *fx, float *fy, int *px, int *py) ;
		/* screen size in graphCreate units, TEXT units, pixels */
BOOL    graphWindowSize (float *xp, float *yp, float *wp, float *hp) ; 
                /* window position and size in graphCreate units */

void    graphGoto (float x, float y) ;  /* tries to center (x,y) in screen */
void    graphWhere (float *x1, float *y1, float *x2, float *y2) ;
		/* coords of visible section (when scrollbars) */

/* whole window manipulations                                                */
void    graphRedraw (void) ;		    /* complete redraw. */
void    graphClear (void) ;		    /* Clear a window. */
void    graphWhiteOut (void) ;		    /* Set window to background colour. */

void graphSetTitlePrefix(char *prefix) ;
char *graphGetTitlePrefix(void) ;
void graphRetitlePlain(char *title) ;
void graphRetitle (char *name) ;


void    graphPop (void) ;
void    graphEvent (int action, float x, float y) ;     /* posts an event */

void graphSetGreyMap(unsigned char *map, BOOL isDrag);
BOOL graphSetInstallMap(BOOL arg);
int graphGreyRampPixelIntensity(int weight);
unsigned char *graphGreyRampGetMap(void);
/* in the GTK version the greyramp tool is provided by the gex-package (see wh/gex.h) */


/* Non blocking messages, applications generally use displayBlock */
void    graphMessage (char *text) ;     
void    graphUnMessage (void) ;         


/* Interface to message list code which displays messout/messerror messages  */
/* in a scrolled list, this is set up by graphSetMessList() routine which    */
/* sets messout/messerror callbacks to point to mesgListAdd(). There is a    */
/* maximum number of messages after which old messages are lost off the top. */

/* Get the default number of messages for the list.                          */
int mesgListGetDefaultListSize(int curr_value) ;

/* Check the message list size.                                              */
BOOL mesgListCheckListSize(int curr_value) ;

/* Create a new list or refresh an existing one, you _must_ call this before */
/* calling any of the below routines, behaviour is undefined if you don't.   */
void mesgListCreate(GraphFunc app_destroy_db) ;

/* Set the maximum number of messages that the list can hold before it starts*/
/* throwing away messages off the top.                                       */
void mesgListSetSize(int size) ;

/* Add a new message to the list of displayed messages.                      */
void mesgListAdd(char *msg, void *user_pointer) ;

/* Reset the list to empty.                                                  */
void mesgListReset(void) ;

/* Get rid of the message list window.                                       */
void mesgListDestroy(void) ;



void graphSysBeep(void) ;

/* Box manipulations                                                         */

int     graphLastBox(void); 
int     graphBoxStart(void) ;           /* returns box number */
void    graphBoxEnd (void) ;
BOOL    graphBoxClear (int box) ;
BOOL    graphBoxMarkAsClear (int box) ;
void    graphBoxDim(int box, float *x1, float *y1, float *x2, float *y2) ;
void    graphBoxSetDim(int k, float *x1, float *y1, float *x2, float *y2) ;
int     graphBoxAt (float x, float y, float *rx, float *ry);
void    graphBoxDraw (int box, int fcol, int bcol) ;
void    graphClipDraw (int x1, int y1, int x2, int y2);
				      /* expose action */

void    graphBoxDrag (int box, void (*clientRoutine)(float*,float*,BOOL)) ;
void    graphBoxShift (int box, float xbase, float ybase) ;
void    graphBoxSetPick (int box, BOOL pick) ;
void    graphBoxSetMenu (int k, BOOL menu) ;

void    graphLine (float x0, float y0, float x1, float y1) ;
void    graphRectangle (float x0, float y0, float x1, float y1) ;
void    graphFillRectangle (float x0, float y0, float x1, float y1) ;
void    graphPolygon (Array pts) ;	/* filled polygon */
void    graphLineSegs (Array pts) ;	/* line segment */
void    graphCircle (float xcen, float ycen, float rad) ;
void    graphArc (float xcen, float yxen, float rad,
		  float ang, float angDiff) ; 
		/* degrees anticlockwise from 3 o'clock */
void    graphFillArc (float xcen, float yxen, float rad,
		      float ang, float angDiff) ; 
		/* degrees anticlockwise from 3 o'clock */
void    graphPoint (float x, float y) ;
void    graphText (char *text, float x0, float y0) ;
void    graphTextUp (char *text, float x0, float y0) ;
     	   /* updatable text - length is length to extend the box */
void    graphTextPtr (char *text, float x0, float y0, int length) ; 
void    graphTextPtrPtr (char **text, float x0, float y0, int length) ;
void    graphColorSquares (char *cols, float x0, float y0, int length, 
			   int skip, int *tints) ;

void	graphPixels (unsigned char *pixels, int w, int h, 
		     int lineWidth,
		     float x0, float y0, 
		     unsigned int *colors, int ncols) ;
void	graphPixelsRaw (unsigned char *pixels, int w, int h, 
			int lineWidth, float x0, float y0) ;

BOOL    graphRawMaps (unsigned char *forward, int *reverse) ;
   /* forward[] maps [0,255] -> PixelsRaw values, 
      reverse[] inverts, giving max value so can test for 255 
      return FALSE if pixel images do not exist */
void    graphColorMaps (float *red, float *green, float *blue) ;
   /* colormap values are real in [0,1], and map from [0,255] */

BOOL    graphSetColorMaps (unsigned char *red,
			   unsigned char *green,
			   unsigned char *blue) ;
   /* set colors with the given maps */
   /* arrays are 128 bytes long      */
 
   /* the next routines does not add to the box : used for cursors */
void    graphXorLine (float x0, float y0, float x1, float y1) ;
void    graphXorBox (int kbox, float x, float y) ;

   /* The following routines all return the old value, and just return
      the current value if given a negative argument (except 
      graphRegister). */
float   graphPointsize (float x) ;
float   graphLinewidth (float x) ;
float   graphTextHeight (float height) ; /* if 0 then standard text */
int     graphTextFormat(int format) ; 
typedef enum {GRAPH_LINE_SOLID = 0, GRAPH_LINE_DASHED} GraphLineStyle ; 
GraphLineStyle graphLineStyle(GraphLineStyle style) ;	    /* Negative arg: just returns current
							       value */


/* Specifies the location of the alignment origin for x,y coordinates of text.
   LEFT, CENTRE or RIGHT, and TOP or BOTTOM are permitted alignment values.
   Note: Logical OR ("|") can be used to combine flags:
   LEFT, CENTRE or RIGHT can be combined with either TOP or BOTTOM
   The default alignment is "TOP|LEFT" */
enum TextAlignment { GRAPH_TEXT_LEFT = 0, GRAPH_TEXT_CENTRE = 1, GRAPH_TEXT_RIGHT = 2,
		     GRAPH_TEXT_TOP = 4, GRAPH_TEXT_BOTTOM = 8 } ; 
int     graphTextAlign (int textAlignment) ;


int     graphColor (int color) ;         /* color is a GraphColor */

#define  graphRegister(k,f) uGraphRegister(k, (GraphFunc)(f))
GraphFunc uGraphRegister (int k, GraphFunc) ;   /* k is a GraphEvent */
char*   graphHelp (char* item) ;

BOOL    graphWebBrowser (char* link);
	/* remote-controlled netscape via X-atoms */

/************************************************************/

typedef struct colouropt
{ void (*f)(void *arg);
  void *arg;
  int fg;
  int bg;
  char *text;
  struct colouropt *next;
} COLOUROPT;



/* Menu types and functions.                                                 */
/*                                                                           */

/* Base menu ptr. for all menu functions and                                 */
/* typecast for MenuFunction's => struct menuopt x->f VoidRoutine's */
typedef void (*MenuBaseFunction)(void) ;
#define MENU_FUNC_CAST (MenuBaseFunction)

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
#define MENU_FUNC_CAST (void(*)(void))
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */


typedef void (*MenuRoutine)(KEY) ;
typedef void (*MenuFunction)(int) ;
typedef void (*FreeMenuFunction)(KEY, int) ;

typedef struct menuopt
  { MenuBaseFunction f ;
    char *text ;
  } MENUOPT ;


extern int menuBox ;		/* Global used by menu routines. */

void    graphBoxFreeMenu (int box, FreeMenuFunction proc, FREEOPT *options) ;
#define   graphFreeMenu(x,y)  graphBoxFreeMenu(0,(FreeMenuFunction)(x),(y))
void	graphBoxMenu (int box, MENUOPT *menu) ;
#define   graphMenu(x)  graphBoxMenu(0,(x))
int     graphMenuTriangle (BOOL filled, float x, float y) ;
	/* graphics routine for drawing a pulldown symbol */

void    graphBoxInfo (int box, KEY key, char *text) ;
void    graphBoxInfoFile (ACEOUT info_out) ;
void    graphBoxInfoWrite (void) ;
 
int     graphButton (char* text, VoidRoutine func, float x, float y) ;
int     graphColouredButton (char *text,
			     ColouredButtonFunc func,
			     void *arg,
			     int fg,
			     int bg,
			     float x,
			     float y);
int     graphButtons (MENUOPT *buttons, float x0, float y0, float xmax) ;
int     graphColouredButtons (COLOUROPT *buttons, float x0, float y0, float xmax) ;

/**************** new menus ***********/
void	graphNewBoxMenu (int box, MENU menu) ;
#define graphNewMenu(x) graphNewBoxMenu(0,(x))



#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
/* These routines appear not to be coded or referenced anywhere....          */
/*                                                                           */
int	graphNewButton (MENUITEM item, float x, float y) ;
int	graphNewButtons (MENU menu, float x0, float y0, float xmax) ;

/* graphics routine for drawing a pulldown symbol - moved from menu.c/h */
int     menuDownSelTriangle (BOOL filled, float x, float y) ;

#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */


/************** text entry ***************/

typedef void (*GraphEntryFunc)(char*) ;

int     graphTextEntry (char* cp, int len, float x, float y, GraphEntryFunc fn) ;
int     graphTextScrollEntry (char* text, int len, int wlen, 
	 				  float x, float y, GraphEntryFunc fn) ;
int     graphCompletionEntry (GraphCompletionFunc f, char* text, int len, 
		 			 float x, float y, GraphEntryFunc fn) ;
int     graphCompScrollEntry (GraphCompletionFunc f, char* text, int len, int wlen, 
					 float x, float y, GraphEntryFunc fn) ;
void    graphEntryDisable (void) ; /* disables TextEntries */

#define TEXT_ENTRY_FUNC_CAST (void (*)(char*))

/******** interaction with rest of workspace *******/


void    graphBusyCursorAll (void) ;

BOOL    graphInterruptCalled(void) ; /* use messIsInterruptCalled */
void    graphResetInterrupt(void) ; /* use messResetInterrupt */
void    graphForceInterrupt(void) ; /* set flag for interruptCalled */

void    graphPostBuffer (char *text) ; /* write to screen cut/paste buffer */
void    graphPasteBuffer (void (*callback)(char *));


/******** editors - single item labelled textEntry boxes *********/
void    editorToggleChange(int box);
void    graphToggleEditor (char *label, BOOL *p, float x, float y);
void    graphAddRadioEditor(char *text, int tag, int index, float x, float y);
void    graphSetRadioEditor(int tag,int index);
int     graphRadioCreate(char *text,int *p,float x, float y);
void    graphColourEditor(char *label, char *text, int *p,float x, float y);


int     graphIntEditor (char *label, int *p, float x, float y, 
			BOOL (*checkFunc)(int)) ;
int     graphFloatEditor (char *label, float *p, float x, float y,
		     BOOL (*checkFunc)(float)) ;
int     graphWordEditor (char *label, char *text, int len, float x, float y,
			 BOOL (*checkFunc)(char*)) ;
int     graphTextEditor (char *label, char *text, int len, float x, float y,
			 BOOL (*checkFunc)(char*)) ;
int     graphTextScrollEditor (char *label, char *text, int len, int wlen, float x, float y,
			       BOOL (*checkFunc)(char* text, int box)) ;
BOOL    graphCheckEditors (Graph graph, BOOL callCheckFunctions) ;
BOOL    graphUpdateEditors (Graph graph) ;


/*****                 PostScript images                                ******/

int graphPS (Graph gId, ACEOUT fo, char *title, 
	     BOOL doColor, BOOL isEPSF, BOOL isRotate, PaperType paper,
	     float fac, int pages);
void graphPSdefaults (Graph gId, PaperType paper, 
		      BOOL *pRot, float *pFac, int *pPage);
							    /* Get defaults for PostScript image. */


/***** GIF images, using Tim Boutell's gd package (c) Cold Spring Harbor *****/
/*                                                                           */
int  graphGIF (Graph gId, ACEOUT out, BOOL do_size) ;
void    graphGIFLeftDown (int x, int y) ;


/*****                 Image printing                                   ******/
/*                                                                           */
void    graphPrint(void);		    /* Image print dialog. */
void    graphBoundsPrint (float uw, float uh, void (*draw)(void)) ;
				/* to cheat the graph size for multipage prints */



/* Use of the following routines is severely deprecated, they are just for   */
/* an incursion by sprdmap.c into graph internals. These routines will be    */
/* withdrawn when possible.                                                  */
void    graphGetSprdmapData(int *h, float *uh, int *type, float *yfac) ;
void    graphSetSprdmapCoords(int h, float uh) ;
void    graphSetSprdmapType(int type) ;


/********* help function prototypes  ******/

/* this void-void function is used in menustructures or for graphButtons
   to bring up the help-topic currently registered for this window */
/* all other help-system stuff is in help.h (not graph-specific) */
void    help (void) ;

#ifdef  __cplusplus
}
#endif

/********* don't know where to put these, so sticking them at the end ***********/
Array getPrinters (void);
BOOL spawnChildProcess(char **argv, GChildWatchFunc callbackFunc, gpointer callbackData, GError **error);
void onChildProcessClosed(GPid child_pid, gint status, gpointer data);

#endif /*  !defined DEF_GRAPH_H */

 
 
