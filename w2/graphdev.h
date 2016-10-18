/*  File: graphDev.h
 *  Author: Ed Griffiths (edgrif@sanger.ac.uk)
 *  Copyright (c) J Thierry-Mieg and R Durbin, 1999
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
 * 	Richard Durbin (Sanger Centre, UK) rd@sanger.ac.uk, and
 *	Jean Thierry-Mieg (CRBM du CNRS, France) mieg@kaa.crbm.cnrs-mop.fr
 *
 * Description: Contains the definition of the device dependent interface
 *              that the public graph calls get mapped to. This header
 *              is included both by graph level code which uses the
 *              interface and the graphDev level code which implements it. 
 *              
 *              NOTE that in general parameters do not default in this
 *              interface, graph is the level to do defaulting, not here.
 *              
 *              This file MUST NOT include any Graph level types/headers
 *              etc., that would totally break the encapsulation.
 *              
 *              See also Graph_Internals.h
 *              
 * HISTORY:
 * Last edited: Jan  4 11:23 2011 (edgrif)
 * Created: Wed Jan 27 13:13:42 1999 (edgrif)
 * CVS info:   $Id: graphdev.h,v 1.17 2011-01-05 11:25:59 edgrif Exp $
 *-------------------------------------------------------------------
 */
#ifndef DEF_GRAPHDEV_H
#define DEF_GRAPHDEV_H


/*                                                                           */
/* State handles:                                                            */
/*  - Opaque types which point to Graph and GraphDev state, neither layer    */
/*    has access to the others state.                                        */
/*                                                                           */
typedef struct _GraphStruct *GraphPtr ;			    /* Opaque handle to a Graph. */
typedef struct _GraphDevStruct *GraphDev ;		    /* Opaque handles to a GraphDev.  */

/*                                                                           */
/* Data types.                                                               */
/*  - Some of these replicate the graph types but should not be removed      */
/*    because they enable Graph and GraphDev to vary independently.          */
/*                                                                           */
typedef struct _GraphDevFuncRec *GraphDevFunc ;
typedef struct _GraphDevCallbackRec *GraphDevCallback ;

typedef enum _GraphDevShapeStyle { 
  GRAPHDEV_SHAPE_FILL, 
  GRAPHDEV_SHAPE_OUTLINE
} GraphDevShapeStyle ;

typedef enum {GRAPHDEV_LINE_SOLID, GRAPHDEV_LINE_DASHED} GraphDevLineStyle ;

typedef enum _GraphDevStringDirection {
  GRAPHDEV_STRING_LTOR,
  GRAPHDEV_STRING_TTOB
} GraphDevStringDirection ;

typedef enum _GraphDevOrientation { 
  GRAPHDEV_HORIZONTAL, 
  GRAPHDEV_VERTICAL
} GraphDevOrientation ;

typedef enum _GraphDevKeyType {
  GRAPHDEV_NOKEY, 
  GRAPHDEV_NORMALKEY, 
  GRAPHDEV_FUNCKEY, 

  /* all these are case insensitive. */
  GRAPHDEV_COPYKEY,					    /* Cntl-C */
  GRAPHDEV_CUTKEY,					    /* Cntl-X */
  GRAPHDEV_PASTEKEY,					    /* Cntl-V */

  GRAPHDEV_HELPKEY,					    /* Cntl-H */
  GRAPHDEV_CLOSEKEY,					    /* Cntl-W */
  GRAPHDEV_PRINTKEY,					    /* Cntl-P */
  GRAPHDEV_SAVEKEY,					    /* Cntl-S */
  GRAPHDEV_QUITKEY,					    /* Cntl-Q */
} GraphDevKeyType ;

/* Do not change the order or values of these.                               */

typedef enum _GraphDevEventType {
  GRAPHDEV_LEFT_DOWN, GRAPHDEV_LEFT_DRAG, GRAPHDEV_LEFT_UP,
  GRAPHDEV_MIDDLE_DOWN, GRAPHDEV_MIDDLE_DRAG, GRAPHDEV_MIDDLE_UP,
  GRAPHDEV_RIGHT_DOWN, GRAPHDEV_RIGHT_DRAG, GRAPHDEV_RIGHT_UP,
  GRAPHDEV_PICK, GRAPHDEV_KEYBOARD, GRAPHDEV_RESIZE,
  GRAPHDEV_MESSAGE_DESTROY, GRAPHDEV_DESTROY
} GraphDevEventType ;

typedef enum _GraphDevGraphType {
  GRAPHDEV_PLAIN, GRAPHDEV_TEXT_SCROLL, GRAPHDEV_TEXT_FIT,
  GRAPHDEV_MAP_SCROLL, GRAPHDEV_PIXEL_SCROLL,
  GRAPHDEV_TEXT_FULL_SCROLL, GRAPHDEV_PIXEL_FIT,
  GRAPHDEV_TEXT_FULL_EDIT, GRAPHDEV_PIXEL_VSCROLL, 
  GRAPHDEV_PIXEL_HSCROLL, GRAPHDEV_TEXT_HSCROLL
} GraphDevGraphType ;

typedef enum _GraphDevFontFormat {
  GRAPHDEV_PLAIN_FORMAT, GRAPHDEV_ITALIC,
  GRAPHDEV_BOLD, GRAPHDEV_GREEK, GRAPHDEV_FIXED_WIDTH
} GraphDevFontFormat ;




/*                                                                           */
/* Function prototypes.                                                      */
/*  - All GraphDev interface functions are prototyped here.                  */
/*                                                                           */

/* Initialisation, termination of Graph, and general functions.              */

typedef void (*GraphDevFinish)(void) ;
typedef int  (*GraphDevLoop)(GraphDev dev, BOOL block);
typedef void (*GraphDevLoopReturn)(GraphDev dev, int retval);
typedef void (*GraphDevSetWinModal)(GraphDev dev, BOOL modal, BOOL override);
typedef void (*GraphDevScreenSize)(float *sx, float *sy, float *fx, float *fy, int *px, int *py) ;
typedef BOOL (*GraphDevInterruptCalled)(BOOL force) ;
typedef void (*GraphDevResetInterrupt)(void) ;
typedef void (*GraphDevPasteBuffer)(GraphDev dev) ;
typedef void (*GraphDevPostBuffer)(GraphDev dev, char *text) ;
typedef void (*GraphDevBeep)(GraphDev dev) ;	
typedef BOOL (*GraphDevWebBrowser)(char *link) ;
typedef int  (*GraphDevRemoteCommands)(char *applicationName, int *cmdcptr, char **cmdv) ;
typedef void (*GraphDevMenuBar)(GraphDev dev, MENU menuBar);
typedef void (*GraphDevSetGreyRamp)(GraphDev dev, unsigned char *ramp, BOOL isDrag) ;
typedef void (*GraphDevKeyboardControl)(GraphDev dev) ;

/* Message routines.                                                         */
/* All parameters must be supplied for these calls, there are no defaults.   */
/*                                                                           */
typedef void (*GraphDevMessage)(GraphDev dev, char *text) ;
typedef void (*GraphDevUnMessage)(GraphDev dev) ;


/* Graph creation/destruction, interactions.                                 */
/*                                                                           */
typedef GraphDev (*GraphDevCreate)(GraphPtr graph, 
				   BOOL isNaked, BOOL onMouseGrab,
				   GraphDevGraphType graph_type,
				   char *shell_name,
				   float x, float y, float w, float h,
				   int *width, int *height) ;
typedef void (*GraphDevDestroy)(GraphDev dev) ;
typedef void (*GraphDevRetitle)(GraphDev dev, char *title) ;
typedef void (*GraphDevPop)(GraphDev dev) ;
typedef void (*GraphDevGoto)
     (GraphDev dev, int x, int y) ;
typedef void (*GraphDevRedraw)
     (GraphDev dev, int width, int height) ;  /* Redraw a window. */
typedef void (*GraphDevWhiteOut)(GraphDev dev) ;	    
     /* Set a window to its background colour. */
typedef void (*GraphDevClear)(GraphDev dev) ;
/* free all memory associated with what's drawn on the window */
typedef void (*GraphDevBusyCursorAll)() ; 
     /* Turn busy cursor on */
typedef void (*GraphDevCleanup)(GraphDev dev);

/* Getting/Setting sizes.                                                    */
/*                                                                           */
typedef void (*GraphDevGetScrollWinSize)
     (GraphDev dev, GraphDevOrientation orientation,
      float *size, float *position, float *shown) ;
typedef void (*GraphDevSetHScrollWinPos)
     (GraphDev dev, int curr_height, int new_height) ;
/* If either width or height is set to 0, they will be ignored.              */
typedef void (*GraphDevSetBaseWinSize)
     (GraphDev dev, GraphDevGraphType graph_type, int width, int height) ;
typedef void (*GraphDevGetWindowSize)
     (GraphDev dev, float *wx, float *wy, float *ww, float *wh) ;


/* Font calls.                                                               */
/* These calls are a temporary measure, in the end font/point size needs to  */
/* be abstacted in a different way...                                        */
typedef void (*GraphDevSetFontHeight)(GraphDev dev, int height) ;
typedef void (*GraphDevSetFont)(GraphDev dev, int height) ;
typedef void (*GraphDevSetFontFormat)(GraphDev dev, GraphDevFontFormat format) ;
typedef BOOL (*GraphDevFontInfo)(GraphDev dev, int height, int* xw, int* h) ;
typedef void (*GraphDevGetFontSize)(GraphDev dev, int *width, int *height, int *dy) ;


/* Drawing calls.                                                            */
/*                                                                           */
typedef void (*GraphDevSetBoxDefaults)
     (GraphDev dev, GraphDevLineStyle style, int linewidth, int textheight) ;
typedef void (*GraphDevSetBoxClip)
     (GraphDev dev, int clipx1, int clipy1, int clipx2, int clipy2) ;
typedef void (*GraphDevUnsetBoxClip)(GraphDev dev);
typedef BOOL  (*GraphDevSetGreyMap)
     (BOOL arg) ;
typedef void (*GraphDevSetColours)
     (GraphDev dev, int foreground, int background) ;
typedef void (*GraphDevSetLineWidth)
     (GraphDev dev, int width) ;
typedef void (*GraphDevSetLineStyle)
     (GraphDev dev, GraphDevLineStyle style) ;
typedef void (*GraphDevDrawLine)
     (GraphDev dev, int x1, int y1, int x2, int y2) ;
typedef void (*GraphDevDrawRectangle)
     (GraphDev dev, GraphDevShapeStyle style, 
      int x1, int y1, int width, int height) ;
typedef void (*GraphDevDrawString)
     (GraphDev dev, GraphDevStringDirection direction,
      int x, int y, char *string, int length) ;
typedef void (*GraphDevDrawImage)
     (GraphDev dev,
      unsigned char *pixels, int w, int h, int len, 
      int xbase, int ybase,
      unsigned int *colors, int ncols) ;
typedef void (*GraphDevDrawRampImage)
     (GraphDev dev,
      unsigned char *pixels, int w, int h, int len, 
      int x1, int y1, int x2, int y2,
      int xbase, int ybase) ;
typedef void (*GraphDevDrawPolygon)
     (GraphDev dev, GraphDevShapeStyle style,
      int *x_vertices, int *y_vertices, int num_vertices) ;
typedef void (*GraphDevDrawArc)
     (GraphDev dev, GraphDevShapeStyle style,
      int x, int y, int width, int height, int start_angle, int end_angle) ;
typedef void (*GraphDevXorLine)
     (GraphDev dev, int x1, int y1, int x2, int y2) ;
typedef void (*GraphDevXorBox)
     (GraphDev dev, int x1, int y1, int x2, int y2) ;


/*                                                                           */
/* Function declarations:                                                    */
/*  - With the exception of the graphDevInit, all GraphDev functions are     */
/*    called via function pointers from the GraphDevFuncRec structure.       */
/*    The structure is filled in by GraphDev when graphDevInit is called.    */
/*  - GraphDev need not provide all functions, Graph must check to make sure */
/*    a particular function pointer is non-NULL before calling it. It is     */
/*    graphDevInits responsibility to allocate the table and fill each       */
/*    slot with NULL or a valid function pointer.                            */
/*                                                                           */

/* This function must be provided by ALL device layers, it is the only one   */
/* not in the function pointer table because it sets up the function table,  */
/* passing back a pointer to it in functable.                                */
/*                                                                           */
void graphDevInit(GraphDevFunc *functable,  char *custom_font,
		  int *argcptr, char **argv) ;


/* Function pointer table filled in by device code to indicate which         */
/* functions have been implemented. Any NULL members indicate that that      */
/* function has not been implemented so graph should not call that funciton. */
/*                                                                           */
typedef struct _GraphDevFuncRec
{
  GraphDevFinish               graphDevFinish ;
  GraphDevLoop                 graphDevLoop;
  GraphDevLoopReturn           graphDevLoopReturn;
  GraphDevSetWinModal          graphDevSetWinModal;
  GraphDevScreenSize           graphDevScreenSize ;
  GraphDevInterruptCalled      graphDevInterruptCalled ;
  GraphDevResetInterrupt       graphDevResetInterrupt ;
  GraphDevPasteBuffer          graphDevPasteBuffer ;
  GraphDevPostBuffer           graphDevPostBuffer ;
  GraphDevBeep                 graphDevBeep ;
  GraphDevWebBrowser           graphDevWebBrowser ;
  GraphDevRemoteCommands       graphDevRemoteCommands ;
  GraphDevMenuBar              graphDevMenuBar;
  GraphDevSetGreyRamp          graphDevSetGreyRamp;
  GraphDevKeyboardControl      graphDevDisableKeyboard ;
  GraphDevKeyboardControl      graphDevEnableKeyboard ;

  GraphDevMessage              graphDevMessage ;
  GraphDevUnMessage            graphDevUnMessage ;

  GraphDevCreate               graphDevCreate ;
  GraphDevDestroy              graphDevDestroy ;
  GraphDevRetitle              graphDevRetitle ;
  GraphDevPop                  graphDevPop ;
  GraphDevGoto                 graphDevGoto ;
  GraphDevRedraw               graphDevRedraw ;
  GraphDevWhiteOut             graphDevWhiteOut ;
  GraphDevClear                graphDevClear ;
  GraphDevBusyCursorAll        graphDevBusyCursorAll ;
  GraphDevCleanup              graphDevCleanup ;

  GraphDevGetScrollWinSize     graphDevGetScrollWinSize ;
  GraphDevSetHScrollWinPos     graphDevSetHScrollWinPos ;
  GraphDevSetBaseWinSize       graphDevSetBaseWinSize ;
  GraphDevSetBaseWinSize       graphDevSetNakedWinSize ;
  GraphDevGetWindowSize        graphDevGetWindowSize ;

  GraphDevSetFontHeight        graphDevSetFontHeight ;
  GraphDevSetFont              graphDevSetFont ;
  GraphDevSetFontFormat        graphDevSetFontFormat ;
  GraphDevFontInfo             graphDevFontInfo ;
  GraphDevGetFontSize          graphDevGetFontSize ;

  GraphDevSetBoxDefaults       graphDevSetBoxDefaults ;
  GraphDevSetBoxClip           graphDevSetBoxClip ;
  GraphDevUnsetBoxClip         graphDevUnsetBoxClip ;
  GraphDevSetColours           graphDevSetColours ;
  GraphDevSetLineWidth         graphDevSetLineWidth ;
  GraphDevSetLineStyle         graphDevSetLineStyle ;
  GraphDevDrawLine             graphDevDrawLine ;
  GraphDevDrawRectangle        graphDevDrawRectangle ;
  GraphDevDrawString           graphDevDrawString ;
  GraphDevDrawImage            graphDevDrawImage ;
  GraphDevDrawRampImage        graphDevDrawRampImage ;
  GraphDevDrawPolygon          graphDevDrawPolygon ;
  GraphDevDrawArc              graphDevDrawArc ;
  GraphDevXorLine              graphDevXorLine ;
  GraphDevXorBox               graphDevXorBox ;
  GraphDevSetGreyMap           graphDevSetGreyMap  ;

} GraphDevFuncRec ;



/*                                                                           */
/* GraphDev callbacks.                                                       */
/*                                                                           */

void *GraphDevCBCrashExit(char *message) ;
void graphCBSetActive(GraphPtr graph) ;
void graphCBExposeRedraw(GraphPtr graph, int xmin, int ymin, int xmax, int ymax) ;
void graphCBResize(GraphPtr graph, int width, int height) ;
void graphCBDestroy(GraphPtr graph) ;
void graphCBMouse(GraphPtr graph, int x, int y, GraphDevEventType mouse_type) ;
MENU graphCBMenuPopup(GraphPtr graph, int x, int y) ;
BOOL graphCBKeyboard(GraphPtr graph, GraphDevKeyType key_type, int kval, int modifier) ;
void graphCBMenuSelect(MENUITEM data) ;
char *graphCBRemote(char *command) ;
void graphCBMessageDestroy(GraphPtr graph) ;
BOOL graphCBPick(GraphPtr graph, int x, int y, int modifier) ;
BOOL graphCBMiddle(GraphPtr graph, int x, int y);
void graphCBRampChange(GraphPtr graph, BOOL isDrag);
GraphDev graphCBGraph2Dev(int graphid);
void graphCBSetFuncTable(GraphDevFunc functable);
void graphCBPaste(GraphPtr graph, char *text);

#endif /* DEF_GRAPHDEV_H */


