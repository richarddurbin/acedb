/*  File: graphcon.c
 *  Author: Richard Durbin (rd@mrc-lmb.cam.ac.uk)
 *  Copyright (C) J Thierry-Mieg and R Durbin, 1992
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
 * 	Richard Durbin (MRC LMB, UK) rd@mrc-lmb.cam.ac.uk, and
 *	Jean Thierry-Mieg (CRBM du CNRS, France) mieg@kaa.cnrs-mop.fr
 *
 * Description: device independent control routines for graph package
 * Exported functions: graphInit, Finish, Create, Destroy, Activate...
 * HISTORY:
 * Last edited: Jan  6 13:43 2011 (edgrif)
 * * Jul 20 14:54 1999 (fw): changed interface to graphNakedCreate
 *              to get widgets that grab keyboard focus on mouseover
 *              increased version to 1.4.0 (to signal GTK backend)
 * * May  5 14:01 1999 (edgrif): gMessagePopped not set when message popped up.
 * * Apr 29 14:36 1999 (edgrif): Major changes to support graph/graphdev
 *              layering (see Graphics_Internals.html), release is now 1.3.
 * * Feb  8 17:45 1999 (edgrif): Add string version of graph magic.
 * * Feb  4 09:21 1999 (edgrif): Change to includes for graphDev
 * * Jan 27 14:52 1999 (fw): increased version number to 1.2 to mark the
 *              introduction of the remote control mechanism
 * * Jan 21 14:27 1999 (edgrif): Small tidy up of globals.
 * * Jan  8 11:44 1999 (edgrif): Correct stupid error in busyCursorInactive_G
 * * Dec 15 23:26 1998 (edgrif): Added busy cursor to graphCreate.
 * * Dec  3 14:46 1998 (edgrif): Put in library version macros.
 * * Nov 19 15:03 1998 (edgrif): Fixed callback func. dec. for
 *              graphTextScrollEditor.
 * * Nov 18 13:51 1998 (edgrif): Added graph package version number.
 * * Oct 21 16:59 1998 (edgrif): Added a layer of message calls which
 *              then call device dependent code in xtsubs etc.. This
 *              allowed the addition of meaningful labels for buttons
 *              e.g. Continue, End etc. It has also reintroduced a
 *              layering that used to be in graph but had disappeared.
 * * Oct 21 11:27 1998 (edgrif): Removed outlandish isGraphics flag
 *              but also the internal isInitialised flag, documented this
 *              in wdoc/graph.html.
 * * Sep 11 09:30 1998 (edgrif): Add messExit function register.
 * * Aug 24 10:19 1998 (fw)
 *	-	graphDeleteContents(dying) code extracted from graphClear(), since the full
 *		graphClear() attempts (WIN32) illegal operations on the destroyed window device
 *	-	Changed "gActive" variables to "dying" midway through graphDestroy():
 *              seems more meaningful?
 * Jan 98  mieg: added in the call backs the value of e->box
       because of the garanteed ANSI-C way parameters are handled in C
       this is correct, the callback deposits on the stack box, val
       the old call-back func only dig for val, which is garanteed ok
 * * Jun 10 15:17 1996 (rd)
 * Created: Thu Jan  2 02:34:40 1992 (rd)
 * CVS info:   $Id: graphcon.c,v 1.107 2012-05-01 16:17:39 gb10 Exp $
 *-------------------------------------------------------------------
 */

#include <wh/regular.h>
#include <wh/version.h>					    /* Library version utils */
#include <wh/menu_.h>					    /* boxMenu reference. */
#include <wh/help.h>					    /* For helpOnRegister() */
#include <wh/key.h>
#include <w2/graphdev.h>				    /* interface to graphDevXxxx() */
#include <w2/graph_.h>					    /* Private graph header. */
#include <gtk/gtk.h>

/*---------------------------------------------------------------------------*/
/* graph package version and copyright string.                               */
/*                                                                           */
#define GRAPH_TITLE   "Graph library - GTK version"
#define GRAPH_DESC    "Sanger Centre Informatics graph library for window control"

#define GRAPH_VERSION 2
#define GRAPH_RELEASE 0
#define GRAPH_UPDATE  0
#define GRAPH_VERSION_NUMBER  UT_MAKE_VERSION_NUMBER(GRAPH_VERSION, GRAPH_RELEASE, GRAPH_UPDATE)

static const char *ut_copyright_string = UT_COPYRIGHT_STRING(GRAPH_TITLE, GRAPH_VERSION, GRAPH_RELEASE, GRAPH_UPDATE, GRAPH_DESC)
/*---------------------------------------------------------------------------*/




/* All our screen sizing etc. is based on the original Sun sizing of its     */
/* screens with a height of 900 pixels, therefore we base all our sizing on  */
/* a  900 * 900  screen which should therefore fit on any screen that acedb  */
/* may run on.                                                               */
/* NOTE that if you radically changes these sizes you will need to change    */
/* the below GIFF stuff and probably also much else in the graphics package. */
/*                                                                           */
#define GRAPH_BASE_WIDTH  900.0
#define GRAPH_BASE_HEIGHT 900.0


/* GIF graphs are "virtual" graphs in that there is no corresponding window  */
/* for the image to be displayed in. As such we have to just try and set     */
/* some sensible sizes. It would be nice to do this in a more principled     */
/* way...some time....                                                       */
/*                                                                           */
#define GIFF_VIRTUAL_SCREEN_WIDTH  1000.0
#define GIFF_VIRTUAL_SCREEN_HEIGHT 1000.0

#define GIFF_RELATIVE_WIDTH   (GIFF_VIRTUAL_SCREEN_WIDTH  / GRAPH_BASE_WIDTH)
#define GIFF_RELATIVE_HEIGHT  (GIFF_VIRTUAL_SCREEN_HEIGHT / GRAPH_BASE_HEIGHT)
#define GIFFONTH 13.0
#define GIFFONTW 8.0


/* screenx, screeny are for positioning and sizing created graphs
   and are based on the above  900 * 900  basic screen size,
   really we should get these from the actual screen size. BUT
   don't try this, the graphics package makes assumptions about this
   size all over the place.
*/
static float screenx = GRAPH_BASE_WIDTH ;
static float screeny = GRAPH_BASE_HEIGHT ;


/* maintain list of all open child process ids so we can clean them up */
typedef struct _ChildProcessData
{
  GPid pid;
  GChildWatchFunc destroyFunc;
  gpointer destroyData;
} ChildProcessData;

static GSList *childProcs = NULL;



typedef    BOOL (*EDIT_FUNC)() ;
typedef    BOOL (*FLOAT_EDIT_FUNC)(float,int) ;


/******* externals visible from elsewhere *******/

float graphEventX, graphEventY ;
	/* for users to get at event position, in user coords */
int menuBox ;			  


/* Types of graph that can be created.                                       */
/* typeName used for widget instance names - 
   corresponds to enum GraphType for resource setting
*/
static char* typeName[] = {"plain",
			   "text_scroll",
			   "text_fit",
			   "map_scroll",
			   "pixel_scroll",
			   "text_full_scroll",
			   "pixel_fit",
			   "text_full_edit"
			  } ;


/* for user selection of legal foreground colours */
FREEOPT  graphColors[] =
{ 
  { 32,   "Colours" },
  { WHITE,        "White" },
  { BLACK,        "Black" },
  { GRAY,         "Gray" },
  { PALEGRAY,     "Pale Gray" },
  { LIGHTGRAY,    "Light Gray" },
  { DARKGRAY,     "Dark Gray" },
  { RED,          "Red" },
  { GREEN,        "Green" },
  { BLUE,         "Blue" },
  { YELLOW,       "Yellow" },
  { CYAN,         "Cyan" },
  { MAGENTA,      "Magenta" },
  { LIGHTRED,     "Light Red" },
  { LIGHTGREEN,   "Light Green" },
  { LIGHTBLUE,    "Light Blue" },
  { DARKRED,      "Dark Red" },
  { DARKGREEN,    "Dark Green" },
  { DARKBLUE,     "Dark Blue" },
  { PALERED,      "Pale Red" },
  { PALEGREEN,    "Pale Green" },
  { PALEBLUE,     "Pale Blue" }, 
  { PALEYELLOW,   "Pale Yellow" },
  { PALECYAN,     "Pale Cyan" },
  { PALEMAGENTA,  "Pale Magenta" },
  { BROWN,        "Brown" },
  { ORANGE,       "Orange" },
  { PALEORANGE,   "Pale Orange" },
  { PURPLE,       "Purple" },
  { VIOLET,       "Violet" },
  { PALEVIOLET,   "Pale Violet" },
  { CERISE,       "Cerise" },
  { MIDBLUE,      "Mid Blue" }
} ;

int graphContrastLookup[] = { 
  BLACK, /* WHITE */
  WHITE, /* BLACK */
  BLACK, /* LIGHTGRAY */
  WHITE, /* DARKGRAY */
  WHITE, /* RED */
  BLACK, /* GREEN */
  WHITE, /* BLUE */
  BLACK, /* YELLOW */
  BLACK, /* CYAN */
  BLACK, /* MAGENTA */
  BLACK, /* LIGHTRED */
  BLACK, /* LIGHTGREEN */
  BLACK, /* LIGHTBLUE */
  WHITE, /* DARKRED */
  WHITE, /* DARKGREEN */
  WHITE, /* DARKBLUE */
  BLACK, /* PALERED */
  BLACK, /* PALEGREEN */
  BLACK, /* PALEBLUE */
  BLACK, /* PALEYELLOW */
  BLACK, /* PALECYAN */
  BLACK, /* PALEMAGENTA */
  WHITE, /* BROWN */
  BLACK, /* ORANGE */
  BLACK, /* PALEORANGE */
  WHITE, /* PURPLE */
  BLACK, /* VIOLET */
  BLACK, /* PALEVIOLET */
  BLACK, /* GRAY */
  BLACK, /* PALEGRAY */
  BLACK, /* CERISE */
  WHITE, /* MIDBLUE */
  0 , 0  /* NUM_TRUECOLOURS, TRANSPARENT, for safety */
  };

/****** externals only for use within graph package *****/

Graph_       gActive = 0 ;
GraphDev     gDev = NULL ;
Box          gBox = 0 ;
Stack        gStk = 0 ;
GraphDevFunc gDevFunc ;

/****** statics for this file ******/

/* This int is used to ensure that each graph has a unique id by which the   */
/* application can refer to a graph. Note that _no_ graphs will be given     */
/* the id of GRAPH_NULL, this is a special NULL value for graphs.            */
/* Note also that ngraph will also be the total number of graphs created     */
/* by the process during its lifetime (ids are not reused).                  */
static int      ngraph = GRAPH_NULL ;

static Associator  graphIdAss = 0 ;


/* Struct contains data common to all graphs, use getGraphSystem() to access. */
static GraphSystem graph_system_G = NULL ;



/* local functions.                                                          */
/*                                                                           */

static void unRegisterGraphFunctions(void) ;
static GraphSystem getGraphSystem(void) ;
static void closeAllChildProcesses();


/***********************************************************/
/***************** initialisation and finish ****************/

/* MUST be called before any other graph functions:                          */
/*  if this routine is called more than once the results are undefined       */
/*  if other graph routines are called before this the results are undefined */
/*                                                                           */

void graphInit (int *argcptr, char **argv)
{
  /* NB args not used - left in in case we ever need them again */
  
  /* If this is not true then it is a gross coding error by a developer.     */
  /* this sort of thing should be via assert calls.                          */
  if (NEVENTS != DESTROY+1)
    messcrash("Internal code error: number of graph register events mismatch") ;

  /* common data for all graphs. */
  graph_system_G = messalloc(sizeof(GraphSystemStruct)) ;

  graphIdAss = assCreate() ;

  /* ACEDB-GRAPH INTERFACE: initialise graph package overloadable            */
  /* functions.                                                              */
  initOverloadFuncs() ;
  
  /* long loops that check messIsInterruptCalled() 
     can now be terminated by pressing F4, or the F4 can be reset, e.g. if   */
  /* user changes their mind.                                                */
  messIsInterruptRegister (graphInterruptCalled);
  messResetInterruptRegister(graphResetInterrupt) ;

  gDevFunc = NULL; /* may be set later by gexInit
		      via a graphCBSetFuncTable call */
 
}


/* Application can register some "global" routines to be called from any graph window
 * for certain actions, e.g. Cntl-S will result in a call to app_save_routine. */
void graphRegisterGlobalCallbacks(GraphGlobalCallbacks app_callbacks)
{
  GraphSystem graph_sys ;

  graph_sys = getGraphSystem() ;

  graph_sys->app_CBs = *app_callbacks ;			    /* n.b. struct copy here. */

  return ;
}


char *graphVersion(void)
{
  return (char *)ut_copyright_string ;
}



/******* main loop call *************************************************/

int graphLoop (BOOL isBlock)
{
  gActive->hasLoop = TRUE;
  
  if (gDev && gDevFunc->graphDevLoop)
    return gDevFunc->graphDevLoop(gDev, isBlock) ;
  
  return 0;
}
 
BOOL graphLoopReturn (int retval)
{
  gActive->hasLoop = FALSE;
  
  if (gDev && gDevFunc->graphDevLoopReturn)
    gDevFunc->graphDevLoopReturn(gDev, retval) ;
    
  
  return TRUE; /* backwards compatability */
}

/*****************************************************************************/
/* graphCBnnnn functions                                                     */
/*                                                                           */
/* These functions are called from the GraphDev layer, this allows the       */
/* Graph layer to respond to events etc. that are handled initially by the   */
/* GraphDev layer.                                                           */
/*                                                                           */

/* Called from gexInit to connect the graph package to the gex package.
   Note that graphInit MUST be called _before_ gexInit   */
void graphCBSetFuncTable(GraphDevFunc functable)
{
  gDevFunc = functable;
}


/* Gets called when some catatrophic error has occurred in the GraphDev      */
/* layer and we need to exit making sure of the following:                   */
/*                                                                           */
/* 1) We do not issue any more windowing calls                               */
/*                                                                           */
/* 2) An error message is output via messubs (using stderr + to logfile)     */
/*                                                                           */
/* 3) Any cleanup routines that the application may have registered with     */
/*    the messubs package get run.                                           */
/*                                                                           */
/*                                                                           */
void graphCBCrashExit(char *message)
{
  
  unRegisterGraphFunctions() ;

  messcrash(message) ;

  return ;					     /* Never get here. */
}

GraphDev graphCBGraph2Dev(Graph id ) 
{
  Graph_ g ;
  
  if (!id)
    return NULL ;
  else if (!assFind(graphIdAss, assVoid(id), &g))
    return NULL ;
  else
    return g->dev;
}
 
void graphCBMouse(GraphPtr graph, int x, int y, GraphDevEventType mouse_event)
{
  int myEv ;  

  
  if (mouse_event == GRAPHDEV_LEFT_DRAG) myEv = LEFT_DRAG ;
  else if (mouse_event == GRAPHDEV_LEFT_UP) myEv = LEFT_UP;
  else if (mouse_event == GRAPHDEV_MIDDLE_DRAG) myEv = MIDDLE_DRAG ;
  else if (mouse_event == GRAPHDEV_MIDDLE_UP) myEv = MIDDLE_UP;
  else if (mouse_event == GRAPHDEV_RIGHT_DRAG) myEv = RIGHT_DRAG;
  else myEv = -1 ;
  
  if (myEv == -1)
    messout ("Bad mouse event %d", mouse_event) ;
  else 
    {
      graphActivate(graph->id) ;
      if (gActive->func[myEv])
	{
	  graphEventX = XtoUabs(x) ;
	  graphEventY = YtoUabs(y) ;
	  
	  
	  if (mouse_event == GRAPHDEV_LEFT_UP || 
	      mouse_event == GRAPHDEV_MIDDLE_UP)
	    graphBusyCursorAll ();
	  
	  (*gActive->func[myEv])(graphEventX, graphEventY) ; 
	}
    }
}



BOOL graphCBPick(GraphPtr graph, int x, int y, int modifier)
{
  if (!graphActivate(graph->id))
    return FALSE;

  graphEventX = XtoUabs(x) ;
  graphEventY = YtoUabs(y) ;
  gLeftDown(graphEventX, graphEventY, modifier) ; 

  /* Tell the bottom layer if we're interested in drag actions */
  return gActive->func[LEFT_DRAG] != 0;
}


BOOL graphCBMiddle(GraphPtr graph, int x, int y)
{
  if (!graphActivate(graph->id)) 
    return FALSE;
  
  graphEventX = XtoUabs(x) ;
  graphEventY = YtoUabs(y) ;
  gMiddleDown (graphEventX, graphEventY) ; 

  /* Tell the bottom layer if we're interested in drag actions */
  return gActive->func[MIDDLE_DRAG] != 0;
}


void graphCBResize(GraphPtr graph, int width, int height)
{
  int isRedraw, isResize = FALSE ;

  graphActivate(graph->id) ;

  switch (graph->type)
    {
    case PLAIN :
      isRedraw = (width <= graph->w && height <= graph->h &&
		  (width < graph->w || height < graph->h)) ;
      graph->w = width ;
      graph->h = height ;
      graphFacMake () ;
      if (isRedraw)
	graphRedraw () ;
      break ;
    case TEXT_FIT: case PIXEL_FIT:
      if (graph->w != width || graph->h != height)
	isResize = TRUE ;
      graph->w = width ;
      graph->h = height ;
      graph->uw = width / graph->xFac ;
      graph->uh = height / graph->yFac ;
      break ;
    case TEXT_SCROLL:
    case PIXEL_VSCROLL:
      if (gActive->w != width)
	isResize = TRUE ;
      gActive->w = width ;
      break;
    case PIXEL_HSCROLL:
    case TEXT_HSCROLL:
       if (gActive->h != height)
	isResize = TRUE ;
      gActive->h = height ;
      break;
    case PIXEL_SCROLL:
    case TEXT_FULL_EDIT:
    case TEXT_FULL_SCROLL:
      break ;
    case MAP_SCROLL:
      /* MAP_SCROLL windows are obsolete and not implemented */
      break ;
    }

  /* Application may have registered a callback routine for resizes, 
     so set busy cursors and call it.        */
  if (isResize && gActive->func[RESIZE])
    {
      graphBusyCursorAll ();
      (*gActive->func[RESIZE])() ;
    }
  
}

/* callback called from graphdev layer to handle all keyboard stuff. */
BOOL graphCBKeyboard(GraphPtr graph, GraphDevKeyType key_type, int kval, int modifier)
{
  BOOL result = FALSE ;

  graphActivate(graph->id) ;

  if (key_type == GRAPHDEV_NORMALKEY)
    {
      /* Normal key presses. */

      if (kval && gActive->func[KEYBOARD])
	{
	  (*gActive->func[KEYBOARD])(kval, modifier) ;
	  result = TRUE ;
	}
    }
  else
    {
      /* Standard windowing short cuts: Cntl-X, Cntl-Q etc., in some cases we force an
       * alternative, in others we let it default. */
      GraphGlobalCallbacks app_CBs = &(graph->graph_system->app_CBs) ;

      if (key_type == GRAPHDEV_COPYKEY)
	{
	  if (app_CBs->copy.func)
	    {
	      (*app_CBs->copy.func)(app_CBs->copy.data) ;

	      result = TRUE ;
	    }
	  else if (kval && gActive->func[KEYBOARD])
	    {
	      (*gActive->func[KEYBOARD])(kval, modifier) ;
	      result = TRUE ;
	    }
	}
      else if (key_type == GRAPHDEV_CUTKEY)
	{
	  if (app_CBs->cut.func)
	    {
	      (*app_CBs->cut.func)(app_CBs->cut.data) ;

	      result = TRUE ;
	    }
	  else if (kval && gActive->func[KEYBOARD])
	    {
	      (*gActive->func[KEYBOARD])(kval, modifier) ;
	      result = TRUE ;
	    }
	}
      else if (key_type == GRAPHDEV_PASTEKEY)
	{
	  if (app_CBs->paste.func)
	    {
	      (*app_CBs->paste.func)(app_CBs->paste.data) ;

	      result = TRUE ;
	    }
	  else if (kval && gActive->func[KEYBOARD])
	    {
	      (*gActive->func[KEYBOARD])(kval, modifier) ;
	      result = TRUE ;
	    }
	}
      else if (key_type == GRAPHDEV_HELPKEY)
	{
	  if (app_CBs->help.func)
	    (*app_CBs->help.func)(app_CBs->help.data) ;
	  else
	    help() ;
	  
	  result = TRUE ;
	}
      else if (key_type == GRAPHDEV_CLOSEKEY)
	{
	  if (app_CBs->close.func)
	    (*app_CBs->close.func)(app_CBs->close.data) ;
	  else
	    graphDestroy() ;

	  result = TRUE ;
	}
    else if (key_type == GRAPHDEV_PRINTKEY)
	{
	  if (app_CBs->print.func)
	    (*app_CBs->print.func)(app_CBs->print.data) ;
	  else
	    graphPrint() ;

	  result = TRUE ;
	}
    else if (key_type == GRAPHDEV_SAVEKEY)
	{
	  if (app_CBs->save.func)
	    {
	      (*app_CBs->save.func)(app_CBs->save.data) ;

	      result = TRUE ;
	    }
	}
    else if (key_type == GRAPHDEV_QUITKEY)
	{
	  if (app_CBs->quit.func)
	    {
	      (*app_CBs->quit.func)(app_CBs->quit.data) ;

	      result = TRUE ;
	    }
	}
    }

  return result ;
}


void graphCBSetActive(GraphPtr graph)
{
  graphActivate(graph->id) ;
}

void graphCBExposeRedraw(GraphPtr graph, int xmin, int ymin, int xmax, int ymax)
{
  Graph gsave = gActive->id ;
  
  graphActivate(graph->id) ;
  graphClipDraw(xmin,ymin,xmax,ymax) ;
  graphActivate(gsave) ;
  
  
  return ;
}

void graphCBDestroy(GraphPtr graph)
{
  graphActivate(graph->id) ;
  
  graphDestroy () ;
  
  return ;
}

char *graphCBRemote(char *commandText)
{
  char *responseText = NULL ;

  responseText = (graphRemoteRegister(0))(commandText) ;

  return responseText ;
}

void graphCBRampChange(GraphPtr graph, BOOL isDrag)
{
  Graph gsave = gActive->id ;

  graphActivate(graph->id) ;
  if (gActive->func[RAMP_CHANGE])
    (*gActive->func[RAMP_CHANGE])(isDrag);
  graphActivate(gsave) ;
  
}

/*                                                                           */
/* General graph routines.                                                   */
/*                                                                           */


/* Set/Get window title prefix for all graph windows (except some pop-ups). */
void graphSetTitlePrefix(char *prefix)
{
  if (prefix && *prefix)
    graph_system_G->title_prefix = hprintf(0, "%s - ", prefix) ;

  return ;
}

char *graphGetTitlePrefix(void)
{
  return graph_system_G->title_prefix ;
}

/* Just use supplied text for window title, do not apply any graph package prefix. */
void graphRetitlePlain(char *title)
{
  if (title && *title && gDev && gDevFunc->graphDevRetitle)
    {
      gDevFunc->graphDevRetitle(gDev, title) ;
    }

  return ;
}

/* Use text + any graph prefix as window title. */
void graphRetitle(char *title)
{
  if (title && *title && gDev && gDevFunc->graphDevRetitle)
    {
      char *full_title = NULL ;

      full_title = hprintf(0, "%s%s",
			   graph_system_G->title_prefix ? graph_system_G->title_prefix : "",
			   title) ;

      gDevFunc->graphDevRetitle(gDev, full_title) ;

      messfree(full_title) ;
    }

  return ;
}





void graphPop (void)
{
  if (gDev && gDevFunc->graphDevPop)
    gDevFunc->graphDevPop (gDev) ;
}


/* sends an event over the "wire" to active window
 * currently can only handle printable ascii events
 * and mouse events
 */
void graphEvent (int action, float x, float y)
{
  if (action < 0 || action > 127) 
    {
      messcrash ("graphEvent() can only handle actions between 0 and 127\n");
    }
  
  if (action <= RIGHT_UP) /* means a mouse event */ 
    {
      /* Button up or down */
      BOOL isRelease =  (action % 3) == 2;
      int tmp, button = ((tmp = action / 3 )== 0 ? 1 :
			 (tmp == 1 ? 2 : 3 )) ;
      
      if (action%3 == 1) fprintf (stderr, "Sorry, no drag events yet \n") ; 

      if (button == 1 && !isRelease)
	graphCBPick(gActive, uToXabs(x), uToYabs(y), NULL_MODKEY);
      else if (button == 1 && isRelease)
	graphCBMouse(gActive, uToXabs(x), uToYabs(y), GRAPHDEV_LEFT_UP);
      else if (button == 2 && !isRelease)
	graphCBMiddle(gActive, uToXabs(x), uToYabs(y));
      else if (button == 2 && isRelease)
	graphCBMouse(gActive, uToXabs(x), uToYabs(y), GRAPHDEV_MIDDLE_UP);
    }

  /* keyboard events */
  if(action > RIGHT_UP )
    graphCBKeyboard(gActive, GRAPHDEV_NORMALKEY, action, NULL_MODKEY);

  return ;
}

/*****************************************************************/
/* tries to center x,y in visible region */
void graphGoto (float x, float y)
{
 if (gDev && gDevFunc->graphDevGoto)
   gDevFunc->graphDevGoto(gDev, gActive->xFac*x, gActive->yFac*y) ;
}


/* Note that this is a kind of implicit interface, the caller must know the  */
/* type of window they have created in order for what is returned from here  */
/* make sense...it could be the position of a scrollbar within the scrolled  */
/* window, or the size of the window if there are no scrollbars....          */
void graphWhere (float *x1, float *y1, float *x2, float *y2)
{
  float position, shown, size;

  /* if we're a naked graph, the full size of the sub-window
   * (which is embedded in other widgets) is not yet established,
   * because the event-loop hasn't finished assigning the
   * geometry to all the widgets, because they arent yet painted.
   * We need to make sure this is done before grabbing determining
   * the size of the scroll window. */
  (void) graphInterruptCalled();
  
  if (gDev && gDevFunc->graphDevGetScrollWinSize)
    gDevFunc->graphDevGetScrollWinSize(gDev, GRAPHDEV_HORIZONTAL, 
				       &size, &position, &shown) ;
  else
    { 
      position = 0; shown = 1; size = 0;
    }
  
  if (size == 0) {
    if (x1) *x1 = gActive->ux + (position /gActive->xFac) ;
    if (x2) *x2 = gActive->ux + ((position + shown) / gActive->xFac) ;
  }
  else {
    if (x1) *x1 = gActive->ux ;
    if (x2) *x2 = gActive->ux + (size / gActive->xFac) ;
  }

  if (gDev && gDevFunc->graphDevGetScrollWinSize)
    gDevFunc->graphDevGetScrollWinSize(gDev, GRAPHDEV_VERTICAL, 
				       &size, &position, &shown) ;
  else
    {
      position = 0; shown = 1; size = 0;
    }
  if (size == 0) {
    if (y1) *y1 = gActive->uy + (position / gActive->yFac) ;
    if (y2) *y2 = gActive->uy + ((position + shown) / gActive->yFac) ;
  }
  else {
    if (y1) *y1 = gActive->uy ;
    if (y2) *y2 = gActive->uy + (size / gActive->yFac) ;
  }
}



/* 
 * Some routines for setting graph sizes
 */

/* Text relative size setting. */
void graphTextBounds (int nx, int ny)
{
  /* nx ==0 or ny == 0 means "don't change" */
  
  if (gActive->uw == nx)
    nx = 0;
  else if (nx != 0)
    gActive->uw = nx ;
  
  if (gActive->uh == ny)
    ny = 0;
  else if (ny != 0)
    gActive->uh = ny ;

  if (nx) 
    gActive->w = nx * gActive->xFac;
  if (ny) 
    gActive->h = ny * gActive->yFac;

  if (gDev && gDevFunc->graphDevSetBaseWinSize && gDevFunc->graphDevSetNakedWinSize)
    {
      if (gActive->naked_resize)
	gDevFunc->graphDevSetNakedWinSize(gDev, gGetGraphDevGraphType(gActive->type),
					  nx * gActive->xFac,
					  ny * gActive->yFac);
      else
	gDevFunc->graphDevSetBaseWinSize(gDev, gGetGraphDevGraphType(gActive->type),
					 nx * gActive->xFac,
					 ny * gActive->yFac);
    }

  return ;
}


/* fake modif, used in forestdisp.c to prepare a longer printout
 * and restore old at the end. */
float graphFakeBounds (float ny)
{
  float old = gActive->uh ;
  gActive->uh = ny ;
  gActive->h = ny * gActive->yFac ;

  if (gDev && gDevFunc->graphDevSetBaseWinSize)
    gDevFunc->graphDevSetBaseWinSize(gDev, 
				     gGetGraphDevGraphType(gActive->type),
				     0, gActive->h) ;

  return old ;
}


/* Pixel relative size setting. */
void graphPixelBounds (int nx, int ny)
{
  if ((gActive->type != PIXEL_SCROLL) &&
      (gActive->type != PIXEL_HSCROLL) &&
      (gActive->type != PIXEL_VSCROLL))
    messcrash ("pixelBounds called on invalid graph type %d",
	       gActive->type) ;

  graphRawBounds(nx, ny) ;

  return ;
}


/* This is really the same as graphPixelBounds(), but will work on any graph type.
 * the caller must ensure that the sizes they give make sense. */
void graphRawBounds(int nx, int ny)
{
  if (gActive->uw == nx)
    nx = 0;
  else if (nx != 0)
    gActive->uw = nx ;
  
  if (gActive->uh == ny)
    ny = 0;
  else if (ny != 0)
    gActive->uh = ny ;
  
  if (ny)
    gActive->h = ny ;
  if (nx)
    gActive->w = nx ;
  
  if (gDev && gDevFunc->graphDevSetBaseWinSize)
    gDevFunc->graphDevSetBaseWinSize(gDev, 
				     gGetGraphDevGraphType(gActive->type),
				     gActive->w, 
				     gActive->h) ;

  return ;
}

void graphGetBounds(int *nx, int *ny)
{
  *nx = gActive->w ;
  *ny = gActive->h ;

  return ;
}



void graphScreenSize (float *sx, float *sy, 
		      float *fx, float *fy, 
		      int *px, int *py)
{
  if (gDevFunc && gDevFunc->graphDevScreenSize)
    gDevFunc->graphDevScreenSize(sx, sy, fx, fy, px, py) ;
  else 
    {
      /* sensible defaults for the virtual graph case */
      if (sx) *sx = GIFF_RELATIVE_WIDTH ;
      if (sy) *sy = GIFF_RELATIVE_HEIGHT ;
      
      if (fx) *fx = GIFF_VIRTUAL_SCREEN_WIDTH  / GIFFONTW ;
      if (fy) *fy = GIFF_VIRTUAL_SCREEN_HEIGHT / GIFFONTH ;

      if (px) *px = GIFF_VIRTUAL_SCREEN_WIDTH ;
      if (py) *py = GIFF_VIRTUAL_SCREEN_HEIGHT ;
    }
}


BOOL graphWindowSize (float *wx, float *wy, float *ww, float *wh)
{
  if (gDev && gDevFunc->graphDevGetWindowSize)
    {
      float x, y, width, height ;
      
      gDevFunc->graphDevGetWindowSize(gDev, &x, &y, &width, &height) ;
      
      if (wx) *wx = x ; if (ww) *ww = width/(float)screenx ; 
      if (wy) *wy = y ; if (wh) *wh = height/(float)screeny ; 
      return TRUE ;
    }
  else
    return FALSE ;
}



void graphWhiteOut (void)
{

  if (gDev && gDevFunc->graphDevWhiteOut)
    gDevFunc->graphDevWhiteOut(gDev) ;
 
  return ;
}

/****************************************************************************/

/* However the message is got rid of, this routine is called so its OK to    */
/* tidy up in here.                                                          */
void graphCBMessageDestroy(GraphPtr graph_opaque)
{
  Graph_ g = (Graph_) graph_opaque ;
  Graph old = graphActive () ;

  if (graphExists (g->id) && 
      g->func[MESSAGE_DESTROY] && 
      graphActivate (g->id))
    { 
      (*g->func[MESSAGE_DESTROY])() ;
      graphActivate (old) ;
    }
}


void graphMessage (char *text)
{
  if (gDev && gDevFunc->graphDevMessage)
    gDevFunc->graphDevMessage(gDev, text) ;
}

void graphUnMessage (void) 
{ 
  if (gDev && gDevFunc->graphDevUnMessage)
    gDevFunc->graphDevUnMessage(gDev) ;
  
}


/***********************/
/* Sound the system beeper.    TODO may gex? - srk                           */
void graphSysBeep(void)
{
  
  if (gDevFunc && gDevFunc->graphDevBeep)
    gDevFunc->graphDevBeep(gDev) ;
  
  return ;
}




/***********************/

void graphFinish ()
{
  
  while (gActive)
    graphDestroy () ;
  
  if (gDevFunc && gDevFunc->graphDevFinish)
    gDevFunc->graphDevFinish() ;
  
  assDestroy(graphIdAss) ;
  graphIdAss = 0 ;
  
  unRegisterGraphFunctions() ;
  
  return ;
}

/***********************/

Graph_ gGetStruct(Graph id ) 
{
  Graph_ g ;
  
  if (!id)
    return NULL ;
  else if (!assFind(graphIdAss, assVoid(id), &g))
    return NULL ;
  else
    return g;
}
   
BOOL graphActivate (Graph gId)
{ 
  Graph_ g ;
  BOOL result = FALSE ;
  
  if (!gId)
    result = FALSE ;
  else if (!assFind(graphIdAss, assVoid(gId), &g))
    result = FALSE ;
  else
    {
      gActive = g ;
      gDev = gActive->dev ;
      gStk = gActive->stack ;
      gBox = gActive->nbox ? gBoxGet (gActive->currbox) : 0 ;
      result = TRUE ;
    }
  
  return result ;
}


Graph graphActive (void) { return gActive ? gActive->id : 0 ; }


/*       GRAPH_BLOCKABLE - normal window (default), input blocked when a     */
/*                         modal dialog is posted.                           */
/*   GRAPH_NON_BLOCKABLE - grabs the block even when another window has it.
     Block returned to other window when this one dies. If no window has
     the block when this is called, it is a no-op.    Used to make
     help from a modal window work.
*/
/*        GRAPH_BLOCKING - modal dialog window, cannot itself be blocked,    */
/*                         there should only be one of these at a time.      */
/*                                                                           */
void graphSetBlockMode(GraphWindowBlockMode mode)
{

  if (gDevFunc && gDevFunc->graphDevSetWinModal)
    gDevFunc->graphDevSetWinModal(gDev, 
				  mode != GRAPH_BLOCKABLE,
				  mode == GRAPH_NON_BLOCKABLE) ;
  
}



/*****************************************************************************/
/*                  User Interrupt support.                                  */
/*                                                                           */
/* User should be able to interrupt operations by pressing F4, this is _not_ */
/* a hardware interrupt, our software has to periodically check whether F4   */
/* has been pressed by calling this function.                                */
/* The gtk back-end executes events during this call, this may be change     */
/* the active graph, so we save and restore here. I'm not sure if this may   */
/* have other bad consequences: time will tell                               */
/*                                                                           */
/* Caller can also reset the interrupt, this is needed because xace offers   */
/* the user the chance to carry on and because of timing stuff we may need   */
/* to reset the interrupt.                                                   */
/*                                                                           */
BOOL graphInterruptCalled(void)
{
  BOOL result = FALSE ;
  Graph gsave = gActive ? gActive->id : 0;

  if (gDevFunc && gDevFunc->graphDevInterruptCalled)
    result = gDevFunc->graphDevInterruptCalled(FALSE) ;
  
  if (gsave != 0) graphActivate(gsave);
  return result ;
}

void graphResetInterrupt(void)
{
  if (gDevFunc && gDevFunc->graphDevResetInterrupt)
    gDevFunc->graphDevResetInterrupt() ;

  return ;
}


/******* process all outstanding events in the queue *******/
/* this routine is unused in acedb but is used in the image code.            */
/* TODO define as graphInterruptCalled and remove ? */
void graphProcessEvents (void)
{
  Graph gsave = gActive ? gActive->id : 0;
  
  if (gDevFunc && gDevFunc->graphDevInterruptCalled)
    (void)gDevFunc->graphDevInterruptCalled(FALSE) ; 

  if (gsave != 0) graphActivate(gsave);
  return ;
}
/****** Force an interrupt *********************************/
void graphForceInterrupt(void)
{
  Graph gsave = gActive ? gActive->id : 0;
  
  if (gDevFunc && gDevFunc->graphDevInterruptCalled)
    (void)gDevFunc->graphDevInterruptCalled(TRUE) ;
  
  if (gsave != 0) graphActivate(gsave);
}

/********************* create and destroy ************************/

static MENUOPT defaultMenu [] = { 
  { graphDestroy,	"Quit" },
  { help,		"Help" },
  { graphPrint,		"Print"},
  { 0, 0 }
} ;

/* TODO - elide this? */
void setTextAspect (Graph_ g)	/* used in WIN32 code */
{
  int dx,dy ;

  if (gDevFunc && gDevFunc->graphDevFontInfo)
    {
    if (!(gDevFunc->graphDevFontInfo(g->dev, 0, &dx, &dy)))
      messcrash ("Can't get font info for default font") ;
    }
  else
    {
      dx = GIFFONTW;
      dy = GIFFONTH;
    }
 
  g->aspect = dy / (float) dx ;
  g->xFac = dx + 0.00001 ; /* bit to ensure correct rounding */
  g->yFac = dy + 0.00001 ;
}

static void typeInitialise (Graph_ g, int type)
	/* This is called before the window is generated, and
	   so can not refer to w,h.
	   After the window is generated the resize procedure
	   will be called once - so any initialisation for that
	   must be placed here.
	   After that xFac,yFac,xWin,yWin must be set, so that
	   drawing can take place.
	*/
{
  g->type = type ;

  switch (type)
    {
    case PLAIN: case MAP_SCROLL:
      g->ux = g->uy = 0.0 ;
      g->uw = g->uh = g->aspect = 1.0 ;
      break ;
    case TEXT_FULL_EDIT:/* default 80x25 text window */
    case TEXT_SCROLL:
    case TEXT_FULL_SCROLL:
    case TEXT_HSCROLL:
      g->uw = 80 ;
      g->uh = 25 ;	/* note deliberate fall through here */
    case TEXT_FIT:
      g->ux = g->uy = 0 ;
      setTextAspect (g) ;
      g->xWin = g->yWin = 0 ;
      break ;
    case PIXEL_SCROLL:
    case PIXEL_HSCROLL:
    case PIXEL_VSCROLL:
      g->uw = g->uh = 100 ;
    case PIXEL_FIT:
      g->ux = g->uy = 0 ;
      g->aspect = 1.0 ;
      g->xFac = g->yFac = 1.0 ;
      g->xWin = g->yWin = 0 ;
      break ;
    default: 
      messcrash ("Invalid graph type %d requested",type) ;
    }
}



/* Access graph system struct. */
static GraphSystem getGraphSystem(void)
{
  return graph_system_G ;
}



static void unRegisterGraphFunctions(void)
{
  struct messContextStruct messNullContext = { NULL, NULL };
  struct helpContextStruct helpNullContext = { NULL, NULL };

  messOutRegister (messNullContext) ;
  messErrorRegister (messNullContext) ;
  messExitRegister (messNullContext) ;
  messCrashRegister (messNullContext) ;
  messQueryRegister (0) ;
  messPromptRegister (0) ;
  messIsInterruptRegister (0);
  filQueryOpenRegister (0) ;
  helpOnRegister (helpNullContext);

  return ;
} /* unRegisterGraphFunctions */



/* 
 *               Set of functions to create different types of graph.
 */

/* Create/initialise the graph struct. */
Graph_ graphCreateStruct (int type)
{ 
  STORE_HANDLE handle = handleCreate () ;
  Graph_ graph = (Graph_) handleAlloc (0, handle, GRAPH_SIZE) ;
  int i ;
  static magic_t Graph_MAGIC = GRAPH_MAGIC ;


  graph->magic = &Graph_MAGIC ;

  /* N.B. this code gives the first graph created an id of (GRAPH_NULL + 1), */
  /* this is important as _no_ graphs should be given the id of GRAPH_NULL.  */
  graph->id = ++ngraph ;

  graph->graph_system = getGraphSystem() ;

  assInsert(graphIdAss, assVoid(graph->id), (void *)graph) ;
  graph->handle = handle ;
  graph->clearHandle = handleHandleCreate (handle) ;

  graph->boxes = arrayHandleCreate (64, BoxStruct, handle) ;
  graph->stack = stackHandleCreate (1024, handle) ;
  graph->boxstack = stackHandleCreate (32, handle) ;
  graph->nbox = 0 ;
  graph->assoc = assHandleCreate(handle) ;
  typeInitialise (graph, type) ;

        /* initialise statics for the graph */
  graph->linewidth = 0.002 ;
  graph->line_style = GRAPH_LINE_SOLID ;
  graph->pointsize = 0.005 ;
  graph->textheight = 0.0 ;
  graph->color = FORE_COLOR ;
  graph->textFormat = PLAIN_FORMAT ;
  for (i = 0 ; i < NEVENTS ; i++)
    graph->func[i] = 0 ;

  return graph ;
}


/* the routine that really creates the graphs, called by various "cover" functions
 * in the graph interface that hide some of the details from the application. */
static Graph realCreate (int type, char *nam, 
			 BOOL isNaked, BOOL onMouseGrab, BOOL isVirtual, BOOL naked_resize,
			 float x, float y, float w, float h)
{ 
  Graph_ graph;
  GraphDevGraphType graph_type =  gGetGraphDevGraphType(type) ;
  int iw, ih;

  graph = graphCreateStruct (type) ;

  graph->naked_resize = naked_resize ;

  if (!nam)
    nam = messprintf ("graph:%d", ngraph) ;
  graph->name = strnew(nam, graph->handle);
 
  /* Size calculations: if the size we get passed in negative, it's in
     graph user-units, if it positive, it's in units of 900 pixels. This
     is for backward compatibilty. Don't ask. */

  if (w<0)
    w *= -graph->xFac;
  else
    w *= screenx;

  if (h<0)
    h *= -graph->yFac;
  else
    h *= screeny;

  /* Actually create the new window, note how we get returned an opaque ptr  */
  /* to the underlying device.  */
  if (!isVirtual && gDevFunc && gDevFunc->graphDevCreate)
    {
        gDev = graph->dev = 
	  gDevFunc->graphDevCreate (graph, isNaked, onMouseGrab, 
				    graph_type, typeName[graph->type],
				    x, y, w, h, &iw, &ih);
    }
  else
    {
      /* Virtual graphs have graph->dev == NULL, this happens if 
	 a virtual graph was requested, or is forced if there is no back-end
	 (ie if gDevFunc == NULL). 
	 Note the graph->dev and therefore gDev != NULL implies that
	gDevFunc != NULL, so we don't have to test seperately for that. 
      */
      gDev = graph->dev = NULL;
      iw = w;
      ih = h;
    }
  
  /* Set up the size fields in the graph structure, and set the size of the
   * underlying device window according to the type of graph. */
  graph->w = iw;
  graph->h = ih;

  if (graph->naked_resize)
    gDevFunc->graphDevSetNakedWinSize(gDev, graph_type, graph->w, graph->h) ;
  else if (gDev && gDevFunc->graphDevSetBaseWinSize)
    gDevFunc->graphDevSetBaseWinSize(gDev, graph_type, graph->w, graph->h) ;



  graphActivate (graph->id);
  if (graph->type == PLAIN)
    graphFacMake();
  
  graphRetitle(graph->name);
  
  /* Set a busy cursor on all windows so that user will be warned not to use */
  /* the application until window is completely drawn, */
  graphBusyCursorAll ();
  
   graphClear () ; /* initialises box, stack structures*/
   
#if !defined(MACINTOSH)
   if (!isNaked)
     graphMenu (defaultMenu) ;
#endif
   
   return graph->id ;
}

/* Classic graphCreate */
Graph graphCreate (int type, char *n, float x, float y, float w, float h)
{ 
  return realCreate(type, n, FALSE, FALSE, FALSE, FALSE, x, y, w, h);
}

/* This one makes a graph which has a gtk widget, but no window, The widget
   is retrieved with gexGraph2Widget(graph) and then packed into a gtk window
   the name is useful for the print-window, if it pops up for that graph
   if focusOnMouseOver is TRUE, the widget will grab keyboard focus,
     if the mouse is over the drawing area widget, 
     so the graphRegister(KEYBOARD,.. system can be used.
*/
Graph graphNakedCreate(int type, char *name, float w, float h, BOOL focusOnMouseOver) 
{ 
  return realCreate(type, name, TRUE, focusOnMouseOver, FALSE, FALSE, 0.0, 0.0, w, h);
}


/* As for graphNakedCreate() but window is resized only depending on the graph type. */
Graph graphNakedResizeCreate(int type, char *name, float w, float h, BOOL focusOnMouseOver) 
{ 
  return realCreate(type, name, TRUE, focusOnMouseOver, FALSE, TRUE, 0.0, 0.0, w, h);
}


/* This one make a virtual window, it doesn't appear on the screen, but one
   may draw onto it's stack and then dump the results as postscript
   or GIF. Most non-stack operations are no-ops */
Graph graphVirtualCreate(int type, char *name, float w, float h)
{
  return realCreate(type, name, FALSE, FALSE, TRUE, FALSE, 0.0, 0.0, w, h);
}




/*******************************/

/* Convertor functions for going from Graph level type values to GraphDev    */
/* level type values.                                                        */
/*                                                                           */
GraphDevEventType gGetGraphDevEventType(int event_type)
{
  GraphDevEventType graphdev_event ;

  switch (event_type)
    {
    case LEFT_DOWN :            graphdev_event = GRAPHDEV_LEFT_DOWN ; break ;
    case LEFT_DRAG :            graphdev_event = GRAPHDEV_LEFT_DRAG ; break ;
    case LEFT_UP :              graphdev_event = GRAPHDEV_LEFT_UP ; break ;
    case MIDDLE_DOWN :          graphdev_event = GRAPHDEV_MIDDLE_DOWN ; break ;
    case MIDDLE_DRAG :          graphdev_event = GRAPHDEV_MIDDLE_DRAG ; break ;
    case MIDDLE_UP :            graphdev_event = GRAPHDEV_MIDDLE_UP ; break ;
    case RIGHT_DOWN :           graphdev_event = GRAPHDEV_RIGHT_DOWN ; break ;
    case RIGHT_DRAG :           graphdev_event = GRAPHDEV_RIGHT_DRAG ; break ;
    case RIGHT_UP :             graphdev_event = GRAPHDEV_RIGHT_UP ; break ;
    case PICK :                 graphdev_event = GRAPHDEV_PICK ; break ;
    case KEYBOARD :             graphdev_event = GRAPHDEV_KEYBOARD ; break ;
    case RESIZE :               graphdev_event = GRAPHDEV_RESIZE ; break ;
    case MESSAGE_DESTROY :      graphdev_event = GRAPHDEV_MESSAGE_DESTROY ; break ;
    case DESTROY :              graphdev_event = GRAPHDEV_DESTROY ; break ;
    default : messcrash("internal error, attempt to lookup unknown event type: %d", event_type) ;
    }
  return graphdev_event ;
}


GraphDevGraphType gGetGraphDevGraphType(int graph_type)
{
  GraphDevGraphType graphdev_type ;

  switch (graph_type)
    {
    case PLAIN :            graphdev_type = GRAPHDEV_PLAIN ; break ;
    case TEXT_SCROLL :      graphdev_type = GRAPHDEV_TEXT_SCROLL ; break ;
    case TEXT_HSCROLL:      graphdev_type = GRAPHDEV_TEXT_HSCROLL ; break;
    case TEXT_FIT :         graphdev_type = GRAPHDEV_TEXT_FIT ; break ;
    case MAP_SCROLL :       graphdev_type = GRAPHDEV_MAP_SCROLL ; break ;
    case PIXEL_SCROLL :     graphdev_type = GRAPHDEV_PIXEL_SCROLL ; break ;
    case PIXEL_HSCROLL:     graphdev_type = GRAPHDEV_PIXEL_HSCROLL; break;
    case PIXEL_VSCROLL:     graphdev_type = GRAPHDEV_PIXEL_VSCROLL; break;
    case TEXT_FULL_SCROLL : graphdev_type = GRAPHDEV_TEXT_FULL_SCROLL ; break ;
    case PIXEL_FIT :        graphdev_type = GRAPHDEV_PIXEL_FIT ; break ;
    case TEXT_FULL_EDIT :   graphdev_type = GRAPHDEV_TEXT_FULL_EDIT ; break ;
    default : messcrash("internal error, attempt to lookup unknown graph type: %d", graph_type) ;
    }
  return graphdev_type ;
}

GraphDevFontFormat gGetGraphDevFontFormat(int font_format)
{
  GraphDevGraphType graphdev_format ;

  switch (font_format)
    {
    case PLAIN_FORMAT :            graphdev_format = GRAPHDEV_PLAIN_FORMAT ; break ;
    case BOLD :                    graphdev_format = GRAPHDEV_BOLD ; break ;
    case ITALIC :                  graphdev_format = GRAPHDEV_ITALIC ; break ;
    case GREEK :                   graphdev_format = GRAPHDEV_GREEK ; break ;
    case FIXED_WIDTH :             graphdev_format = GRAPHDEV_FIXED_WIDTH ; break ;
    default : messcrash("internal error, attempt to lookup unknown font format: %d", font_format) ;
    }
  return graphdev_format ;
}


GraphDevLineStyle gGetGraphDevLineStyle(GraphLineStyle style)
{
  GraphDevLineStyle graphdev_style ;

  switch(style)
    {
    case GRAPH_LINE_SOLID :   graphdev_style = GRAPHDEV_LINE_SOLID ; break ;
    case GRAPH_LINE_DASHED :  graphdev_style = GRAPHDEV_LINE_DASHED ; break ;
    default : messcrash("internal error, illegal graph line style: %d", style) ;
    }

  return graphdev_style ;
}

/*******************************/

void graphDestroy ()
{ 
  Graph_ dying = gActive ;
  Graph_ g;
  void *v = 0;
  static Associator ass = 0 ;            /* used to store set of dying graphs */

  if (!gActive)
    return ;
  if (!ass)
    ass = assCreate () ;
  if (!assInsert (ass,dying,&g))        /* dying already there */
    return ;

  graphActivate(dying->id);
  
  graphDeleteContents(dying) ; /* kills anything attached to boxes */
  
  if (dying->dev) 
    {
      if (dying->hasLoop) 
	graphLoopReturn(0); 
      
      if (gDevFunc->graphDevDestroy)
	gDevFunc->graphDevDestroy(gDev) ;
      
      dying->dev = gDev = 0;
    }
  
  if (dying->func[DESTROY])           	/* user registered functions */
    (*(dying->func[DESTROY]))() ;     	/* can not use graph, as gDev gone */
  
  
  assRemove(graphIdAss, assVoid(dying->id)) ; 
  
  /* The code really can't cope with having no active graph */
  if (assNext(graphIdAss, &v, &g))
    graphActivate(g->id);
  else
    {
      gActive = 0 ; gStk = 0 ; gBox = 0 ;
    }
  
  dying->magic = 0 ;
  assRemove (ass, dying) ;
  if (dying->handle)
    { STORE_HANDLE h = dying->handle;
      dying->handle = 0; /* for possible recursion */
      handleDestroy (h) ;
    }
}

/*********************************/

BOOL graphExists (Graph gId)
{ Graph_ g ;
  
  return  
    gId && assFind(graphIdAss, assVoid(gId), &g) ;
}

/*********************************/


void graphCleanUp (void)  /* kill all windows except current active one */
{ 

  if (gDev && gDevFunc->graphDevCleanup)
    gDevFunc->graphDevCleanup(gDev);

  /* kill any spawned processes */
  closeAllChildProcesses();
}


/**** routine to register the Help  ****/

char *graphHelp (char *item)
{ char *old = gActive->help ;
  if (item && *item)
    gActive->help = item ;

  return old ;
}

/**** routine to register functions for events (e.g. mouse) ****/

GraphFunc uGraphRegister (int i, GraphFunc proc) /* i is a GraphEvent */
{
  GraphFunc old = gActive->func[i] ;
  gActive->func[i] = proc ;
  return old ;
}

/**** button package ****/

int graphButton (char* text, VoidRoutine func, float x, float y)
{ 
  int k = graphBoxStart () ;  

  graphText (text, x + XtoUrel(3), y + YtoUrel(2)) ;
  graphRectangle (x, y, gBox->x2 + XtoUrel(3), gBox->y2) ;
  gBox->flag |= GRAPH_BOX_BUTTON_FLAG ;	/* Added for mac CLH */
  graphBoxEnd () ;
  graphBoxDraw (k, BLACK, WHITE) ;
  if (!gActive->buttonAss)
    gActive->buttonAss = assHandleCreate (gActive->handle) ; 
  assInsert (gActive->buttonAss, assVoid(k*4), (void*) func) ;

  graphBoxInfo(k, 0, 
	       hprintf(gActive->handle, "BUTTON:\"%s\"", text)) ;

  return k ;
}

int graphColouredButton (char *text,
			 ColouredButtonFunc func,
			 void *arg,
			 int fg,
			 int bg,
			 float x,
			 float y)
{ int k = graphBoxStart();
  
  graphText(text, x + XtoUrel(3), y + YtoUrel(2));
  graphRectangle(x, y, gBox->x2 + XtoUrel(3), gBox->y2);
  gBox->flag |= GRAPH_BOX_BUTTON_FLAG; /* Added for mac CLH */
  graphBoxEnd();
  graphBoxDraw (k, fg, bg);
  if (!gActive->buttonAss)
    gActive->buttonAss = assHandleCreate (gActive->handle);
  assInsert(gActive->buttonAss, assVoid(k*4), (void*) func) ;
  assInsert(gActive->buttonAss, assVoid(k*4 + 1), arg);
  assInsert(gActive->buttonAss, assVoid(k*4 + 2), assVoid(fg));
  assInsert(gActive->buttonAss, assVoid(k*4 + 3), assVoid(bg));
  graphBoxInfo(k, 0, 
	       hprintf(gActive->handle, "BUTTON:\"%s\"", text)) ;

  return k;
}

int graphButtons (MENUOPT *buttons, float x0, float y0, float xmax)
{
  double x = x0 ;
  double y = y0 ;
  int n=0, box = 0, len, ix, iy ;
  float ux, uy ;
  
  if (!buttons) return 0 ;

  graphTextInfo (&ix, &iy, &ux, &uy) ;
  uy += YtoUrel(8) ;

  while (buttons->text)
    { 
      len = strlen (buttons->text) ;
      
      if (!(len == 0 && buttons->f == menuSpacer))
	{ /* menu option is not a separator */
	  if (x + ux*len + XtoUrel(15) > xmax && x != x0)
	    { x = x0 ; y += uy ; }
	  box = graphButton (buttons->text, buttons->f, x, y) ;
	  x += ux*len + XtoUrel(15) ; 
	  ++n ;
	}
       ++buttons ;
    }

  return box + 1 - n ;
}

int graphColouredButtons (COLOUROPT *buttons, float x0, float y0, float xmax)
{
  double x = x0 ;
  double y = y0 ;
  int n=0, box = 0, len, ix, iy ;
  float ux, uy ;
  COLOUROPT *old= buttons;

  if (!buttons) return 0 ;

  graphTextInfo (&ix, &iy, &ux, &uy) ;
  uy += YtoUrel(8) ;

  while (buttons->text) /* terminate on NULL text */
    { 
      len = strlen (buttons->text) ;
      
      if (x + ux*len + XtoUrel(15) > xmax && x != x0)
	{ x = x0 ; y += uy ; }
      box = graphColouredButton (buttons->text, 
				 buttons->f,
				 buttons->arg,
				 buttons->fg,
				 buttons->bg,
				 x, y) ;
      x += ux*len + XtoUrel(15) ; 
      ++n ;
      if (buttons->next == buttons || buttons->next == old) break;
      if (buttons->next) 
	buttons = buttons->next;
      else 
	buttons++;
    }  

  return box + 1 - n ;
}

/************** graph associator stuff ***************/

BOOL uGraphAssociate (void* in, void* out)
{
  if (gActive)
    return assInsert (gActive->assoc,in,out) ;
  else
    return FALSE ;
}

BOOL uGraphAssFind (void *in, void* *out)
{
  if (gActive)
    return assFind (gActive->assoc,in,out) ;
  else
    return FALSE ;
}

BOOL graphAssRemove (void* in)
{
  if (gActive)
    return assRemove (gActive->assoc,in) ;
  else
    return FALSE ;
}

/********** utility functions to let users get at handles **********/

STORE_HANDLE graphHandle (void)  /* gets freed when graph is freed */
{ if (gActive)
    return gActive->handle ;
  else
    return 0 ;
}

STORE_HANDLE graphClearHandle (void) /* freed when graph is cleared */
{ if (gActive)
    return gActive->clearHandle ;
  else
    return 0 ;
}

/*******************************************************************/
/****** graph editors - single item labelled textEntry boxes ******/

typedef struct {
  char *text ;
  char *format ;
  void *p ;
  int len ;
  int box ;
  int index; /* for radio buttons */
  union { BOOL (*i)(int,int) ; BOOL (*s)(char*,int) ;BOOL (*f)(float,int) ;} func ;
} EDITOR ;

static BOOL callNoError ;

static void callEditor (char *text, Graph_ g)
{ 
  EDITOR *e ;
  union {int i ; float f ; char *s ;} x ;
  int i ;

  if (!g->editors)
    messcrash ("callEditor() called without editors") ;

  for (i = 0 ; i < arrayMax(g->editors) ; ++i)
    if ((e = arrp(g->editors, i, EDITOR))->text == text)
      { if(e->format[0] != 'b') freeforcecard (text) ;
	if (!freecheck (e->format) && e->format[0] != 'b')
	  { messout ("Format does not check for entry \"%s\"", text) ;
	    callNoError = FALSE ;
	  }
	else
	  switch (*e->format)
	    { 
	    case 'b':
	      break;
	    case 'i':
	      freeint (&x.i) ;
	      if (e->func.i && !(*e->func.i)(x.i,e->box))
		{ messout ("Entry \"%s\" does not check", text) ;
		  callNoError = FALSE ;
		}
	      else
		*(int*)e->p = x.i ;
	      break ;
	    case 'f':
	      freefloat (&x.f) ;
	      if (e->func.f && !(*e->func.f)(x.f,e->box))
		{ messout ("Entry \"%s\" does not check", text) ;
		  callNoError = FALSE ;
		}
	      else
		*(float*)e->p = x.f ;
	      break ;
	    case 'w':
	    case 't':
	      x.s = freeword() ;
	      if (strlen (x.s) > e->len)
		{ messout ("Entry \"%s\" is too long (max %d)", x.s, e->len) ;
		  callNoError = FALSE ;
		}
	      else if (e->func.s && !(*e->func.s)(x.s,e->box))
		{ messout ("Entry \"%s\" does not check", x.s) ;
		  callNoError = FALSE ;
		}
	      else
		strncpy (e->text, x.s, e->len) ;
	      break ;
	    }
	return ;
      }

  messcrash ("callEditor called on bad text") ;
}

void callBackEditor (char *text) { callEditor (text, gActive) ; }

static int drawEditor (char *label, char *text, int len, int wlen,
		       float x, float y)
{
  graphText (label, x, y) ;
  x += strlen (label) + 0.5 ;
  return graphTextScrollEntry (text, len, wlen, x, y, callBackEditor) ;
}

static EDITOR *addEditor (char *text, void *p, char *format, int len)
{ 
  EDITOR *e ;

  if (!gActive->editors)
    gActive->editors = arrayHandleCreate (8, EDITOR, gActive->clearHandle) ;

  e = arrayp(gActive->editors, arrayMax(gActive->editors), EDITOR) ;
  e->text = text ;
  e->format = format ;
  e->p = p ;
  e->len = len ;

  return e ;
}

static void drawAltEditor(char *label, float x, float y, EDITOR *e)
{
  int box1, box2 ;

  graphText(label, x+1.5, y);
/*  x += strlen (label) + 0.5 ;*/
  
  box2 = graphBoxStart();
  graphFillArc(x+0.5, y+0.5, 0.4, 0, 360);
  graphBoxEnd();
 
  box1 = graphBoxStart();
  graphArc(x+0.5, y+0.5, 0.7, 0, 360);
  gBox->flag |= GRAPH_BOX_TOGGLE_FLAG;
  graphBoxEnd();
  
  graphAssociate(assVoid(box1),assVoid(arrayMax(gActive->editors)-1));
  e->box = box1 ;

  graphBoxDraw(box1, BLACK, TRANSPARENT);
  if (*(BOOL*)e->p)
    graphBoxDraw(box2, BLACK, TRANSPARENT);
  else
    graphBoxDraw(box2, WHITE, TRANSPARENT);

  graphBoxSetPick(box2, FALSE);
}


/******* public routines ********/

void editorToggleChange(int box)
{
  EDITOR *e,*e1;
  int i;

  graphAssFind(assVoid(box), &i);
  e = arrp(gActive->editors,i,EDITOR);
  if(e->len < 1000){  /* if it's a toggle button */
    *(BOOL*)e->p = !*(BOOL*)e->p;
    
    graphBoxDraw(box, BLACK, TRANSPARENT);
    if (*(BOOL*)e->p)
      graphBoxDraw(box-1, BLACK, TRANSPARENT);
    else
      graphBoxDraw(box-1, WHITE, TRANSPARENT);
  }
  else{ /* if it's a radio button */
    for(i=0;i<arrayMax(gActive->editors);i++){
      e1 = arrp(gActive->editors,i,EDITOR);
      if(e1->len == e->len) /* i.e. same tag */
	graphBoxDraw(e1->box, WHITE, TRANSPARENT);
    }
    graphBoxDraw(e->box,BLACK,TRANSPARENT);
    graphAssFind(assVoid(e->len), &i);
    e1 = arrp(gActive->editors,i,EDITOR);
    
    *(int*)e1->p = e->index; 
  }
}

void graphToggleEditor (char *label, BOOL *p, float x, float y)
{
  char *text = (char*) halloc (16, gActive->clearHandle) ;
  EDITOR *e = addEditor(text,(void*)p,"b", 0);

  strncpy (text,label, 15) ;

  drawAltEditor(label,x,y,e);
}

static void colourChange(KEY key,int box)
{
  int i;
  EDITOR *e;

  if(graphAssFind(assVoid(box), &i)){
    e = arrp(gActive->editors,i,EDITOR);
    *(int*)e->p = key;
    if(e->len != -2)
      graphBoxDraw(box,graphContrastLookup[*(int*)e->p], *(int*)e->p);
    else
      graphBoxDraw(e->box,BLACK,*(int*)e->p);
  }
}
void graphRedoColourBoxes()
{
  int i;
  EDITOR *e;

  for(i=0;i<arrayMax(gActive->editors);i++){
    e = arrp(gActive->editors,i,EDITOR);
    if(e->format[0] == 'b' && e->len == -1)
      graphBoxDraw(e->box,graphContrastLookup[*(int*)e->p], *(int*)e->p);
    else if(e->format[0] == 'b' && e->len == -2)
      graphBoxDraw(e->box,BLACK,*(int*)e->p);
  }
}

void graphColourEditor(char *label, char *text, int *p,float x, float y)
{
  EDITOR *e;
  char *text2 = (char*) halloc (strlen(label)+1, gActive->clearHandle) ;

  strcpy(text2,label);
  e = addEditor(text2,(void*)p,"b", 0);

  e->len = -1;
  e->box = graphBoxStart();
  graphRectangle(x,y,x+3+strlen(text),y+1);
  graphText(text,x+1.0,y);
  graphBoxEnd();
  graphBoxFreeMenu(e->box,colourChange,graphColors);
  if(text[0] != ' '){
    e->len = -1;
    graphBoxDraw(e->box,graphContrastLookup[*(int*)e->p],*(int*)e->p);
  }
  else{
    graphBoxDraw(e->box,BLACK,*(int*)e->p);    
    e->len = -2;
  }
  x += strlen(text) + 4;
  graphText (label,x,y);
  graphAssociate(assVoid(e->box),assVoid(arrayMax(gActive->editors)-1));

}			

/***********************************************************************************************************/
/*
*p will contain the index for the chosen radio button
 tag is the reference number for a radio button.
*/
int graphRadioCreate(char *text,int *p,float x, float y)
{
  int tag = 1000,i;
  EDITOR *e = addEditor(text,0,"b", 0);

  graphText(text,x,y);
  for(i=0;i<arrayMax(gActive->editors);i++){
    e = arrp(gActive->editors,i,EDITOR);
    if(e->format[0] == 'b' && e->len > 1000){
      tag = e->len +1;
    }
  }
  e = addEditor (text, (void*)p, "b", 0) ;
  e->len = tag;
  graphAssociate(assVoid(tag),assVoid(arrayMax(gActive->editors)-1));
  return tag;
}
/***********************************************************************************************************/
/*
tag ->   reference number for the radio button.
index -> this will be returned to e->p in the create routine if this radio is chosen.
*/
 
void graphAddRadioEditor(char *text, int tag, int index, float x, float y)
{
  EDITOR *e = addEditor(text,0,"b", 0);
  int box1;
  
  e->len = tag;

  graphText(text, x, y);
  x += strlen (text) + 0.5 ;
  
  e->text = 0;
  e->box  = graphBoxStart();
  graphFillArc(x+0.5, y+0.5, 0.4, 0, 360);
  graphBoxEnd();
 
  box1 = graphBoxStart();
  graphArc(x+0.5, y+0.5, 0.7, 0, 360);
  gBox->flag |= GRAPH_BOX_TOGGLE_FLAG;
  graphBoxEnd();
  
  graphAssociate(assVoid(box1),assVoid(arrayMax(gActive->editors)-1));

  e->index = index;
  graphBoxDraw(box1, BLACK, TRANSPARENT);
  if (e->index == 0)
    graphBoxDraw(e->box, BLACK, TRANSPARENT);
  else
    graphBoxDraw(e->box, WHITE, TRANSPARENT);

  graphBoxSetPick(e->box, FALSE);
}

void graphSetRadioEditor(int tag,int index)
{
  EDITOR *e;
  int i;

  for(i=0;i<arrayMax(gActive->editors);i++){
    e = arrp(gActive->editors,i,EDITOR);
    if(e->format[0] == 'b' && e->len == tag && e->index == index){
      editorToggleChange(e->box+1);
    }
  }
}

int graphIntEditor (char *label, int *p, float x, float y, 
		    BOOL (*checkFunc)(int))
{
  char *text = (char*) halloc (16, gActive->clearHandle) ;
  EDITOR *e = addEditor (text, (void*)p, "iz", 0) ;
  e->func.i = (EDIT_FUNC) checkFunc ;
  sprintf (text, "%d", *p) ;
  return  e->box = drawEditor (label, text, 15, 8, x, y) ;
}

int graphFloatEditor (char *label, float *p, float x, float y,
		    BOOL (*checkFunc)(float))
{
  char *text = (char*) halloc (16, gActive->clearHandle) ;
  EDITOR *e = addEditor (text, (void*)p, "fz", 0) ;
  e->func.f = (FLOAT_EDIT_FUNC) checkFunc ;
  sprintf (text, "%.4g", *p) ;
  return e->box = drawEditor (label, text, 15, 8, x, y) ;
}

int graphWordEditor (char *label, char *text, int len, float x, float y,
		    BOOL (*checkFunc)(char*))
{
  EDITOR *e = addEditor (text, 0, "wz", len) ;
  e->func.s = (EDIT_FUNC) checkFunc ;
  return  e->box = drawEditor (label, text, len, (len < 16 ? len : 16), x, y) ;
}

int graphTextEditor (char *label, char *text, int len, float x, float y,
		    BOOL (*checkFunc)(char*))
{
  EDITOR *e = addEditor (text, 0, "tz", len) ;
  e->func.s = (EDIT_FUNC) checkFunc ;
  return  e->box = drawEditor (label, text, len, (len < 16 ? len : 16), x, y) ;
}

int graphTextScrollEditor (char *label, char *text, int len, int wlen, float x, float y,
		    BOOL (*checkFunc)(char *text, int box))
{
  EDITOR *e = addEditor (text, 0, "tz", len) ;
  e->func.s = (EDIT_FUNC) checkFunc ;
  return  e->box = drawEditor (label, text, len, wlen, x, y) ;
}

BOOL graphCheckEditors (Graph graph, BOOL unused)
{ 
  int i ;
  Graph old = 0 ;
  Graph_ g ;

  if (!graph || !assFind(graphIdAss, assVoid(graph), &g)) 
    return FALSE ;
  if (!g->editors) 
    return TRUE ;
  if (g != gActive) 
    old = gActive->id ;

  callNoError = TRUE ;
  for (i = 0 ; i < arrayMax(g->editors) ; ++i)
    callEditor (arrp(g->editors, i, EDITOR)->text, g) ;
  
  if (old) graphActivate (old) ;

  return callNoError ;
}

BOOL graphUpdateEditors(Graph graph)
{ 
  int i ;
  Graph old = 0 ;
  Graph_ g ;
  EDITOR *e ;

  if (!graph || !assFind(graphIdAss, assVoid(graph), &g)) 
    return FALSE ;
  if (!g->editors) 
    return TRUE ;

  if (g != gActive) 
    { old = gActive->id ;
      graphActivate (g->id) ;
    }

  for (i = 0 ; i < arrayMax(g->editors) ; ++i)
    {
      e = arrp(g->editors, i, EDITOR) ;
      switch (*e->format)
	{ 
	case 'i':
	  sprintf(e->text, "%d", *(int*)e->p) ;
	  graphTextScrollEntry (e->text, 0, 0, 0, 0, 0) ;
	  break ;
	case 'f':
	  sprintf(e->text, "%.4g", *(float*)e->p) ;
	  graphTextScrollEntry (e->text, 0, 0, 0, 0, 0) ;
	  break ;
	case 'b':
	  {
	    if (e->len < 0)				    /* ghastly => colour editor...sigh. */
	      {
		/* colour editor box.                                        */
		if(e->len != -2)
		  graphBoxDraw(e->box,graphContrastLookup[*(int*)e->p], *(int*)e->p);
		else
		  graphBoxDraw(e->box,BLACK,*(int*)e->p);
	      }
	    else
	      {
		/* Normal toggle box.                                        */
		graphBoxDraw(e->box, BLACK, TRANSPARENT);
		if (*(BOOL*)e->p)
		  graphBoxDraw((e->box)-1, BLACK, TRANSPARENT);
		else
		  graphBoxDraw((e->box)-1, WHITE, TRANSPARENT);
	      }
	    break ;
	  }
	}
    }

  if (old)
    graphActivate (old) ;

  return callNoError ;
}

/********* utility to draw a triangle for menu selection *******/

int graphMenuTriangle (BOOL filled, float x, float y)
{
  int i = 3, box;

  box = graphBoxStart();

  graphLine(x, y+0.25, x+1, y+0.25);
  graphLine(x+1, y+0.25, x+0.5, y+0.75);
  graphLine(x+0.5, y+0.75, x, y+0.25);

  if (filled)
    {
      Array temp = arrayCreate(2*i, float) ;

      array(temp, 0, float) = x;
      array(temp, 1, float) = y+0.25;
      array(temp, 2, float) = x+1;
      array(temp, 3, float) = y+0.25;
      array(temp, 4, float) = x+0.5;
      array(temp, 5, float) = y+0.75;
      graphPolygon(temp);
      arrayDestroy(temp);
    }

  graphBoxEnd();

  return box;
}
/************************************************************/
void help (void)
{
  /* display help on subject that is registered 
     with currently active window */
  /* NOTE: This function has the correct prototype for a MENUOPT */
  char *subject = graphHelp(0);

  if (!subject)
    {
      messout ("Sorry, no help subject registered for this window !") ;
      return ;
    }

  helpOn (subject);

  return;
} /* help */


/************************************************************/
/* Standard callback for when a child process is closed; removes it from
 * our internal list of child processes */
void onChildProcessClosed(GPid child_pid, gint status, gpointer data)
{
  /* This is not very efficient but we won't have many in the list; 
   * search through the list and find this child_pid, then free the
   * data and remove it from the list. 
   * We use a list rather than a hash table because we can't change
   * a hash table while we're iterating through it (which happens when
   * called from closeAllChildProcesses. We can't get around this by
   * changing the list outside the loop in closeAllChildProcesses either,
   * because this callback also gets called from the gtk main loop). */

  GSList *childItem = childProcs;

  for ( ; childItem; childItem = childItem->next)
    {
      ChildProcessData *itemData = (ChildProcessData*)childItem->data;
      
      if (itemData->pid == child_pid)
        {
          printf("Child process %d closed\n", child_pid);
          childProcs = g_slist_remove(childProcs, itemData);
          g_free(itemData);
          break;
        }
    }
  
  g_spawn_close_pid(child_pid); /* cleans up GPid memory, if required */
}


/* Close all child processes that we have spawned */
static void closeAllChildProcesses()
{
  /* We can't use a for loop here because each iteration removes 
   * something from the list and the list changes under our feet.
   * Instead, repeatedly remove the first item in the list until 
   * the list is empty. */
  
  GSList *firstItem = childProcs;
  
  while (childProcs)
    {
      ChildProcessData *data = (ChildProcessData*)childProcs->data;

      kill(data->pid, SIGKILL);
      
      /* The destroy function should be called automatically here
       * by gtk but it doesn't seem to work in all cases (namely when
       * the parent process is shutting down). Instead, we call waitpid
       * ourselves (which stops the gtk callback happening for those 
       * cases where it DOES work) and call the destroy function manually. */
      int status;
      waitpid(data->pid, &status, 0);
      data->destroyFunc(data->pid, status, data->destroyData);

      /* The list should have changed when the child item was destroyed;
       * catch the error if it hasn't otherwise we'll loop forever. */
      if (childProcs == firstItem)
        {
          g_critical("Error closing process %d; child processes may not have been cleaned up correctly.\n", data->pid);
          break;
        }
    }
}


/* Spawn a child process. Calls the given callback function when the
 * child is closed, if given; otherwise calls the standard callback
 * onChildProcessClosed. */
BOOL spawnChildProcess(char **argv, 
                       GChildWatchFunc callbackFunc, 
                       gpointer callbackData,
                       GError **error)
{
  GPid child_pid;

  gboolean status = g_spawn_async(NULL, argv, NULL, 
                                  G_SPAWN_SEARCH_PATH | G_SPAWN_DO_NOT_REAP_CHILD,
                                  NULL, NULL, &child_pid, error);

  if (status)
    {
      /* add a callback to be called when the child closes; required 
       * with the do_not_reap option above. Use the given callback if
       * provided, otherwise our standard callback */
      if (callbackFunc)
        g_child_watch_add_full(G_PRIORITY_HIGH_IDLE,
                               child_pid, 
                               callbackFunc, 
                               callbackData,
                               NULL);
      else
        g_child_watch_add(child_pid, onChildProcessClosed, NULL);

      /* Add process id to our list of spawned processes */
      /* Ideally the callbacks would all be taken care of by
       * gtk but this doesn't seem to work when the parent application
       * is shutting down, so I get around this by storing the destroy
       * function callback and data in the list too and calling it manually
       * in closeAllChildProcesses */
      ChildProcessData *data = g_malloc(sizeof *data);
      data->pid = child_pid;
      data->destroyFunc = callbackFunc;
      data->destroyData = callbackData;
      
      childProcs = g_slist_append(childProcs, data);

      printf("Spawned child process %d:", child_pid);

      int i = 0;
      for ( ; argv[i]; ++i)
        printf(" %s", argv[i]);
      printf("\n");
    }

  return status;
}


/************ end of file *****************/




