/*  File: graph_.h
 *  Author: Ed Griffiths (edgrif@sanger.ac.uk)
 *  Copyright (C) J Thierry-Mieg and R Durbin, 1998
 * -------------------------------------------------------------------
 * This file is part of the ACEDB genome database package, written by
 * 	Richard Durbin (MRC LMB, UK) rd@mrc-lmb.cam.ac.uk, and
 *	Jean Thierry-Mieg (CRBM du CNRS, France) mieg@kaa.cnrs-mop.fr
 *
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
 * 	Richard Durbin (Sanger Centre, UK) rd@sanger.ac.uk, and
 *	Jean Thierry-Mieg (CRBM du CNRS, France) mieg@kaa.crbm.cnrs-mop.fr
 *
 * Description: Internal header for the device independent part of the 
 *              graphics package. This header should remain the same
 *              regardless of which independent layer is used underneath
 *              it.
 *              (See graphdev.h for the device dependent interface.
 *               and see the various device dependent internal headers
 *               for their implementation details.
 *
 * Exported functions:
 * HISTORY:
 * Last edited: May 21 17:23 2009 (edgrif)
 * * Apr 29 14:24 1999 (edgrif): Lots of changes to support move to Graph
 *              GraphDev layering, see Graph_Internals.html for details.
 * * Feb  8 17:44 1999 (edgrif): Add String version of graph magic.
 * * Feb  8 16:48 1999 (edgrif): Remove isBlocked from graph struct, redundant
 *              use by sprdmap.c
 * * Feb  1 17:43 1999 (edgrif): Correct the opaque device structs declares.
 * * Jan 21 14:44 1999 (edgrif): minor changes to global names.
 * * Jan  5 13:48 1999 (edgrif): Move graphPS to public header.
 * * Dec 16 15:10 1998 (edgrif): Moved waitCursor calls to internal busyCursor calls.
 * * Oct 22 14:19 1998 (edgrif): Added message interface for use in providing
 *              device dependent routines: graphWinOut etc.
 * * Oct 13 14:19 1998 (edgrif): Added func decs for graph/acedb interface.
 *              Mostly these are access functions to get 'callback' type
 *              functions. Plus added this standard format header.
 * Created: Tue Oct 13 14:17:04 1998 (edgrif)
 * CVS info:   $Id: graph_.h,v 1.17 2009-05-29 13:13:29 edgrif Exp $
 *-------------------------------------------------------------------
 */
#ifndef DEF_GRAPH__H
#define DEF_GRAPH__H

#include <wh/regular.h>
#include <wh/graph.h>
#include <wh/graphAcedbInterface.h>
#include <wh/menu.h>
#include <w2/graphdev.h>


/* This struct holds graph package data that is common to all graphs, it can
 * be reached from all graphs via a pointer in the Graph_ struct. */
typedef struct
{
  char *title_prefix ;					    /* Common window title prefix. */

  /* Common callbacks for common actions, e.g. "close window", "quit app." etc. */
  GraphGlobalCallbacksStruct app_CBs ;

} GraphSystemStruct, *GraphSystem ;



/* See GraphEvent enums in graph.h, this is the number of enums defined      */
/* there, in GraphEvent the last event is always DESTROY.                    */
enum {NEVENTS = DESTROY + 1} ;


/* This struct describes all the data associated with a single graph window. */
typedef struct _GraphStruct
{
  magic_t       *magic ;				    /* check for is a graph */
  char          *name ;
  int           id ;					    /* unique graph number for process */
  int           type ;					    /* from enum GraphType */
  GraphSystem   graph_system ;				    /* Global graph data. */
  BOOL    hasLoop;					    /* associated graph Loop */
  float         ux,uy,uw,uh ;				    /* user boundaries */
  float         aspect ;				    /* x units per inch / y units per inch */
  int           xWin,yWin ;				    /* userToWin offsets */
  float         xFac,yFac ;				    /*  and scale factors */
  int           textX,textY ;				    /* text size in pixels */
  int           h,w ;					    /* device coords (0,0) to (w-1,h-1) */
  void          (*func[NEVENTS])() ;			    /* Used in graphxt with either 0 or 2 args */
  float         pointsize ;
  GraphLineStyle line_style ;
  float         linewidth ;
  float         textheight ;
  int           color ;
  int           textFormat ;
  Array         boxes ;
  Stack         stack ;
  int           nbox ;
  int           currbox ;
  BOOL          isClear ; 
  Stack         boxstack ;
  Associator    assoc ;
  char*         help ;
  Associator    buttonAss ;
  Array	  boxInfo ;
  Array	  editors ;
  STORE_HANDLE  handle ;	 /* freed on graphDestroy */
  STORE_HANDLE  clearHandle ;	    /* freed every time graphClear() called */

  MENU          menu0 ;      /* Handle for default menu */
  Associator boxMenus ;    /* links boxes with their own menus. */

  GraphDev      dev ;		    
  void          (*pasteCallBack)(char *); /* Call back of getting selection */

  BOOL  naked_resize ;					    /* resize calls take no notice of
							       whether there is a top level window. */

} GraphStruct, *Graph_ ;

#define GRAPH_SIZE      sizeof(GraphStruct)


/* Currently I use the graph magic to help with debugging when looking at the
 * graph struct, see graphcon.c for initialisation. */
#define GRAPH_MAGIC "Graph"
typedef struct
        { float     x1, y1, x2, y2 ;
          int       mark ;
          float     linewidth ;
	  GraphLineStyle line_style ;
          float     textheight ;
          float     pointsize ;
          unsigned char   fcol, bcol ;
	  unsigned char   flag, format;
#if defined(BETTER_BOX_SHIFT)	/* from rbrusk */
	  int	    id, parentid ;
#endif
        } BoxStruct, *Box ;

typedef struct
	{ KEY key ;
	  char *text ;
	} BOXINFO ;

enum GraphAction
        { LINE, RECTANGLE, FILL_RECTANGLE, CIRCLE, POINT, 
	  TEXT, TEXT_UP, TEXT_PTR, TEXT_PTR_PTR,
          COLOR, TEXT_FORMAT, TEXT_HEIGHT, LINE_WIDTH, LINE_STYLE, POINT_SIZE,
          BOX_START, BOX_END, PIXELS, PIXELS_RAW, COLOR_SQUARES, 
	  POLYGON, FILL_ARC, IMAGE, ARC, LINE_SEGS
        } ;

#define GRAPH_BOX_NOPICK_FLAG	0x01
#define GRAPH_BOX_MENU_FLAG	0x02
#define GRAPH_BOX_BUTTON_FLAG	0x04
#define GRAPH_BOX_INFO_FLAG	0x08
#define GRAPH_BOX_ENTRY_FLAG	0x10
#define GRAPH_BOX_TOGGLE_FLAG	0x20
#define GRAPH_BOX_NOMENU_FLAG	0x40			    /* Make box transparent to menu clicks. */


/* externals only for use within graph package: gXxxx */
/*                                                                           */
extern Graph_        gActive ;       /* active graph */
extern GraphDev      gDev ;          /* gActive->dev */
extern Box           gBox ;          /* active box from gActive */
extern Stack         gStk ;          /* gActive->stack */
/* GraphDev fills in a table of functions that that Graph can call to inter- */
/* act with the windowing system.                                            */
/* This is the global pointer to the table of GraphDev functions.            */
extern GraphDevFunc gDevFunc ;

extern int menuBox ;					    /* used to transmit ID of menu box */


/* Some useful defines for messages from Graph.                              */
#define GRAPH_INTERNAL_LOGIC_ERROR_MSG "Graph: Internal logic error, \
please report to acedb development, error was - "
#define GRAPH_RESTART_ERROR_MSG "You should save your work and restart \
the application as soon as possible."

#define uToXrel(x) (int)(gActive->xFac*(x))
#define uToYrel(x) (int)(gActive->yFac*(x))
#define uToXabs(x) (-gActive->xWin + uToXrel(x))
#define uToYabs(x) (-gActive->yWin + uToYrel(x))
#define XtoUrel(x) ((x) / gActive->xFac)
#define YtoUrel(x) ((x) / gActive->yFac)
#define XtoUabs(x) XtoUrel((x) + gActive->xWin)
#define YtoUabs(x) YtoUrel((x) + gActive->yWin)

#define UtextX (gActive->textX/gActive->xFac)
#define UtextY (gActive->textY/gActive->yFac)

void graphFacMake (void) ;

void graphDeleteContents (Graph_ theGraph); /* kills boxes/stacks etc. */


void graphASCII (char *myfilname, char *mail, char *print, char *title) ;
#ifdef __CYGWIN__
void graphPrintGDI (Graph_ graph);
#endif
Box  gBoxGet (int k) ;             /* gets Box pointer for index k */
void gLeftDown(float x, float y, int modifier) ;
void gMiddleDown (float x, float y) ;
void gBoxClear (Box box) ;
Graph_ gGetStruct(Graph id ) ;
BOOL gFontInfo (Graph_ graph, int height, int* w, int* h) ;
void gUpdateBox0 (void) ;



void graphDevTextFacSet (void) ;   /* sets fac according to text */



/* Convertor functions to go from Graph level types to corresponding
 * GraphDev level types. */
GraphDevGraphType gGetGraphDevGraphType(int graph_type) ;
GraphDevEventType gGetGraphDevEventType(int event_type) ;
GraphDevFontFormat gGetGraphDevFontFormat(int font_format) ;
GraphDevLineStyle gGetGraphDevLineStyle(GraphLineStyle style) ;



/********* in filquery.c, to be registered in graphInit() **********/



/********************  Graph  <-->  Acedb interface  *************************/
/* Do not use this interface for anything other than setting up graph for    */
/* use with Acedb. It is here to enable the two to cooperate, not for others */
/* to make use of...you have been warned.                                    */
/*                                                                           */
void initOverloadFuncs() ;

VoidCharRoutine  getGraphAcedbMainActivity(void) ;

GraphCharRoutine getGraphAcedbDisplayCreate(void) ;
char *getGraphAcedbChronoName(void) ;
char *getGraphAcedbViewName(void) ;
char *getGraphAcedbFilqueryName(void) ;

MENUOPT *getGraphAcedbPDMenu(void) ;
CharVoidRoutine getGraphAcedbStyle(void) ;
char *getGraphAcedbLogin(void) ;

char *getGraphAcedbSessionName(void) ;
VoidFILEStackIntRoutine getGraphAcedbGetFonts(void) ;

VoidACEOUTKEYRoutine getGraphAcedbClassPrint(void) ;

VoidIntRoutine getGraphAcedbXFont(void) ;


VoidVIEWCOLCONTROLRoutine getGraphAcedbViewAnon(void) ;
BOOLMENURoutine getGraphAcedbSaveView(void) ;
VoidMAPCONTROLVIEWRoutine getGraphAcedbSaveViewMsg(void) ;
VoidMENUSPECRoutine getGraphAcedbViewMenu(void) ;

BOOLMAPCONTROLKEYRoutine getGraphAcedbGetSetView(void) ;
VoidKEYRoutine getGraphAcedbResetKey(void) ;
VoidCOLOBJRoutine getGraphAcedbResetSave(void) ;
VoidIntFREEOPT getGraphAcedbFreemenu(void) ;
VoidOBJFUNCMAPCONTROL getGraphAcedbMapLocate(void) ;
VoidOBJFUNCMAPCONTROL getGraphAcedbScale(void) ;
VoidOBJFUNCSPACERPRIV getGraphAcedbSpace(void) ;
VoidCharKEY getGraphAcedbAddMap(void) ;

/*****************************************************************************/



#endif /* DEF_GRAPH__H */

/***** end of file ********/
 
