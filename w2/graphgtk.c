/*  File: graphgtk.c
 *  Author: Simon Kelley(simon.kelley@bigfoot.com)
 *  Copyright (C) J Thierry-Mieg and R Durbin, 1992
 *  Copyright (C) S Kelley, 1998
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
 * 	Richard Durbin (MRC LMB, UK) rd@mrc-lmb.cam.ac.uk, and
 *	Jean Thierry-Mieg (CRBM du CNRS, France) mieg@kaa.cnrs-mop.fr
 *
 * Description: Graph package implemented in gtk/gdk
 *              Based on graphxt.c and xtsubs.c 
 *              
 * Exported functions: many
 * HISTORY:
 * Last edited: Jan  4 11:25 2011 (edgrif)
 * CVS info:   $Id: graphgtk.c,v 1.90 2011-01-05 11:25:59 edgrif Exp $
 *-------------------------------------------------------------------
 */

/* #define BIG_SCROLL */

#include <gtk/gtk.h>
#include <gdk/gdk.h>
#include <gdk/gdkkeysyms.h>

#ifdef __CYGWIN__
#include <gdk/gdkx.h> /* for GDK_WINDOW_XWINDOW */
#endif
 
#include <wh/regular.h>
#include <wh/array.h>
#include <wh/menu_.h>
#include <wh/key.h>

#ifdef __CYGWIN__
#include <windows.h>
#endif

#include <w2/graphdev.h>
#include <w2/graphgtk_.h>


/*******/
static void destroyEvent (GtkWidget *w, gpointer data);
static void selection_received(GtkWidget *widget, 
			       GtkSelectionData *selection_data,
			       guint32 time,
			       gpointer data);
static gint messageDestroy (GtkWidget *w, gpointer data);
static void butClick(GtkButton *button, gpointer user_data) ;


static gint exposeEvent (GtkWidget *da, GdkEventExpose *ev, gpointer data);
static gint configureEvent (GtkWidget *da, GdkEventConfigure *ev, gpointer data);

static gint motionEvent(GtkWidget *da, GdkEventMotion *ev, gpointer data);
static gint buttonClickEvent(GtkWidget *da, GdkEventButton *ev, gpointer data);
static gint keyboardEvent (GtkWidget *da, GdkEventKey *ev, gpointer data);
static void enableKeyboardHandling(GraphDev dev) ;
static void disableKeyboardHandling(GraphDev dev) ;
static gint windowKeyEvent (GtkWidget *da, GdkEventKey *ev, gpointer data);
static gint enterEvent (GtkWidget *da, GdkEventCrossing *ev, gpointer data);
static gint propertyEvent(GtkWidget *topWin,
			   GdkEventProperty *ev, 
			   gpointer data);
static void installRemoteAtoms(GdkWindow *window);
static GtkWidget *menu2Widget(MENU menu);
static void changeCursor(void);
static BOOL devGtkInterruptCalled(BOOL forceInterrupt);
static void devGtkResetInterrupt(void);
static void setWindowSize(GraphDev dev, GraphDevGraphType graph_type, 
			  int width, int height, BOOL ignore_dev_window) ;
static void moveHorizScrollBar(GtkWidget *scroll, GtkScrollType scrolling, int key) ;


/*****************************************************************/

static int pollLevel = 0;
static Associator destroyed; 
/* to avoid destroying top level widgets more that once */

static GraphDev allDevs; /* ring of all devices */

static unsigned char *selection_string = NULL; 
/* the currently selected text */

static GdkPixmap *icon_pixmap; /* Icon applied to all windows */


/*****************************************************************/

void graphGtkInit (void)
{
  destroyed = assCreate();
  allDevs = NULL;
  icon_pixmap = NULL;
}


/* Selection stuff: */
/* This is the list of selection targets we can provide if asked */

typedef enum {
  SIMPLE_TEXT,
  STRING,
  COMPOUND_TEXT
} SelType;

static GtkTargetEntry targetlist[] = {
    { "STRING",        0, STRING },
    { "TEXT",          0, SIMPLE_TEXT },
    { "COMPOUND_TEXT", 0, COMPOUND_TEXT }
  };

static gint ntargets = sizeof(targetlist) / sizeof(targetlist[0]);

/******************************************************************/

/* Window set : keep track of all the windows  by linking their graphDev
   structures in a ring. These functions are exported to gex so that it
   can keep track of gtk windows too */

void graphGtkWinInsert(GraphDev dev)
{
  if (allDevs)
    {  
      GraphDev ringPtr = allDevs;
      while (ringPtr->ring != allDevs) 
	ringPtr = ringPtr->ring;
      ringPtr->ring = dev;
    }
  else
    allDevs= dev;
  
  dev->ring = allDevs;
  allDevs= dev;

  /* Set a default icon if there is one for all our windows */
  if (dev->topWindow && icon_pixmap)
    gdk_window_set_icon(dev->topWindow->window, NULL, icon_pixmap, NULL); 
}

void graphGtkWinRemove(GraphDev dev)
{
  GraphDev ringPtr = dev;
  while (ringPtr->ring != dev) 
    ringPtr = ringPtr->ring;
  if (ringPtr == dev)
    /* last one */
    allDevs = NULL;
  else
    allDevs = ringPtr->ring = dev->ring;
}

void graphGtkWinCleanup(GtkWidget *save)
{  
  BOOL gotone;
  
   do {
    GraphDev ringPtr = allDevs;
    gotone = FALSE;
    if (ringPtr)
      do { 
	if (ringPtr->topWindow && !(ringPtr->topWindow == save))
	  { 
	    gtk_widget_destroy(ringPtr->topWindow);
	    gotone = TRUE;
	    break;
	  }
	ringPtr = ringPtr->ring;
      } while (ringPtr != allDevs);
  } while (gotone);
}

/********** icons    ************/
/* This implements an icon for all windows created in gex/graph */
/* you can overide it for individual windows if needed */

void graphGtkSetIcon(char *icon)
{
  if (icon_pixmap)
    gdk_pixmap_unref(icon_pixmap);
  
  icon_pixmap = 
    gdk_pixmap_colormap_create_from_xpm_d(NULL, gdk_colormap_get_system(),
					  NULL, NULL, (gchar **)icon);
}

/********** creation ************/

static GraphDev devGtkCreate(GraphPtr graph, BOOL isNaked, BOOL onMouseGrab,
			     GraphDevGraphType graph_type, char *shell_name,
			     float x, float y, float w, float h,
			     int *iw, int *ih)
/* NB currently ignore x, y  */
{
  GtkWidget *da;
  GtkWidget *scroll;
  GraphDev dev = (GraphDev) messalloc (sizeof(GraphDevStruct))  ;
  GdkColor white;
  GtkStyle *daStyle;

  graphGtkSetSplash(0); /* kill any splash screen */

  dev->type = graph_type ;

  *iw = (int)w;
  *ih = (int)h;
  
  /* the sizes called for by the graph package are for the drawing area,
     not for the window, so compensate. Note that this only compensates 
     for scrollbars, if you add more chrome, you're on your own */

  switch (graph_type)
    { 
    case GRAPHDEV_TEXT_SCROLL:
    case GRAPHDEV_PIXEL_VSCROLL:
      dev->width_fudge = 32;
      dev->height_fudge = 12;
      break;
      
    case GRAPHDEV_TEXT_FULL_SCROLL:
    case GRAPHDEV_PIXEL_SCROLL:
      dev->width_fudge = 32;
      dev->height_fudge = 32;
      break;

    case GRAPHDEV_PIXEL_HSCROLL:
    case GRAPHDEV_TEXT_HSCROLL:
      dev->width_fudge = 12;
      dev->height_fudge = 32;
      break;
      
    default:
      dev->width_fudge = 0;
      dev->height_fudge = 0;
      break;
    }
      
  /* Guard against making huge windows */
  if (*iw > (gdk_screen_width()-(30+dev->width_fudge)))
    *iw = gdk_screen_width()-(30+dev->width_fudge);
  
  if (*ih > (gdk_screen_height()-(30+dev->height_fudge)))
    *ih = gdk_screen_height()-(30+dev->height_fudge);

  dev->graph = graph;
  dev->doingExposure = FALSE;
  dev->keyboard_handling = TRUE ;
  dev->message = NULL;
  dev->menuBar = NULL;
  dev->greyRampImages = NULL;
  dev->hasChrome = FALSE;

  dev->hasGreyCmap = graphGtkSetGreyMap(FALSE); 
  /* FALSE doesn't install a colormap if it's never used */
  graphGtkSetGreyMap(dev->hasGreyCmap);
  /* restored to orignal value */ 
						   

  if (isNaked)
    dev->topWindow = NULL;
  else
    { 
      dev->topWindow = gtk_window_new(GTK_WINDOW_TOPLEVEL);
      dev->vbox = gtk_vbox_new(FALSE, 0);
      gtk_container_add(GTK_CONTAINER(dev->topWindow), dev->vbox);
      dev->hbox = gtk_hbox_new(FALSE, 0);
      gtk_container_add(GTK_CONTAINER(dev->vbox), dev->hbox);
      gtk_window_set_default_size(GTK_WINDOW(dev->topWindow), 
				  *iw + dev->width_fudge,
				  *ih + dev->height_fudge);
    }
  
  da = dev->drawingArea = gtk_drawing_area_new();
  gdk_color_white(gtk_widget_get_colormap(da), &white);
  daStyle = gtk_style_copy(gtk_widget_get_default_style());
  daStyle->bg[GTK_STATE_NORMAL] = white;
  /* graph package like background == white */
  gtk_widget_set_style(da, daStyle); 
  
  gtk_selection_add_targets (da, GDK_SELECTION_PRIMARY,
			     targetlist, ntargets);
 
  GTK_WIDGET_SET_FLAGS (da, GTK_CAN_FOCUS);

  assRemove(destroyed, da);
  
  if (graph_type == GRAPHDEV_TEXT_SCROLL || 
      graph_type == GRAPHDEV_TEXT_FULL_SCROLL ||
      graph_type == GRAPHDEV_PIXEL_SCROLL ||
      graph_type == GRAPHDEV_PIXEL_VSCROLL ||
      graph_type == GRAPHDEV_PIXEL_HSCROLL ||
      graph_type == GRAPHDEV_TEXT_HSCROLL)
    {
      dev->scroll = scroll = gtk_scrolled_window_new (NULL, NULL);
      gtk_container_set_resize_mode(GTK_CONTAINER(scroll), 
				    GTK_RESIZE_IMMEDIATE); 
      gtk_container_border_width (GTK_CONTAINER (scroll), 5);
      gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(scroll), da);


#ifdef BIG_SCROLL
      {
	GtkWidget *viewport = GTK_BIN(scroll)->child;

	/* decouple viewport from scrollbars, so we can do our own scrolling */
	gtk_viewport_set_vadjustment(GTK_VIEWPORT(viewport), NULL);
	gtk_viewport_set_hadjustment(GTK_VIEWPORT(viewport), NULL);
      }
#endif

      if (graph_type == GRAPHDEV_TEXT_SCROLL || graph_type == GRAPHDEV_PIXEL_VSCROLL)
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroll),
				       GTK_POLICY_NEVER,
				       GTK_POLICY_ALWAYS);
      else if (graph_type == GRAPHDEV_PIXEL_HSCROLL || graph_type == GRAPHDEV_TEXT_HSCROLL)
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroll),
				       GTK_POLICY_ALWAYS,
				       GTK_POLICY_NEVER);

      if (!isNaked)
	{
	  gtk_box_pack_start(GTK_BOX(dev->hbox), scroll, TRUE, TRUE, 0);
	}

      gtk_widget_show(scroll) ;

    }
  else
    { 
      dev->scroll = NULL;
      if (!isNaked)
	gtk_box_pack_start(GTK_BOX(dev->hbox), da, TRUE, TRUE, 0);
    }
  
  gtk_signal_connect(GTK_OBJECT(da), "selection_get",
		     GTK_SIGNAL_FUNC(graphGtkSelectionGet), NULL);
  gtk_signal_connect(GTK_OBJECT(da), "selection_received",
		     GTK_SIGNAL_FUNC(selection_received), (gpointer) dev);
  gtk_signal_connect(GTK_OBJECT(da), "button_press_event",
		     GTK_SIGNAL_FUNC(buttonClickEvent), (gpointer) dev);
  gtk_signal_connect(GTK_OBJECT(da), "button_release_event",
		     GTK_SIGNAL_FUNC(buttonClickEvent), (gpointer) dev);
  gtk_signal_connect(GTK_OBJECT(da), "expose_event",
		     GTK_SIGNAL_FUNC(exposeEvent), (gpointer) dev);
  gtk_signal_connect(GTK_OBJECT(da), "configure_event",
		     GTK_SIGNAL_FUNC(configureEvent), (gpointer) dev);
  gtk_signal_connect(GTK_OBJECT(da), "motion_notify_event",
		     GTK_SIGNAL_FUNC(motionEvent), (gpointer) dev);
  gtk_signal_connect(GTK_OBJECT(da), "destroy", 
		     GTK_SIGNAL_FUNC(destroyEvent), (gpointer) dev);
  if (onMouseGrab)
    gtk_signal_connect(GTK_OBJECT(da), "enter_notify_event",
		       GTK_SIGNAL_FUNC(enterEvent), (gpointer) dev);
  /* this need to be the final signal run so we can swallow keys,
     they don't then get passed to the window for widget navigation
     unless we want them to */
  gtk_signal_connect_after(GTK_OBJECT(da), "key_press_event",
		     GTK_SIGNAL_FUNC(keyboardEvent), (gpointer) dev);
  gtk_widget_set_events(da, 
			GDK_BUTTON_PRESS_MASK |
			GDK_BUTTON_RELEASE_MASK |
			GDK_ENTER_NOTIFY_MASK |
			GDK_EXPOSURE_MASK |
			GDK_POINTER_MOTION_HINT_MASK |
			GDK_BUTTON1_MOTION_MASK |
			GDK_BUTTON2_MOTION_MASK |
			GDK_BUTTON3_MOTION_MASK |
			GDK_KEY_PRESS_MASK );

  
 
  if (!isNaked)
    {
      gtk_signal_connect(GTK_OBJECT(dev->topWindow), "property_notify_event",
			 GTK_SIGNAL_FUNC(propertyEvent), (gpointer) dev);
      gtk_signal_connect(GTK_OBJECT(dev->topWindow), "key_press_event",
			 GTK_SIGNAL_FUNC(windowKeyEvent), (gpointer) dev);
      gtk_widget_set_events(dev->topWindow, 
			    GDK_PROPERTY_CHANGE_MASK | GDK_KEY_PRESS_MASK);
      gtk_widget_show(dev->topWindow);      
      gtk_widget_show(dev->vbox);
      gtk_widget_show(dev->hbox);
      gtk_widget_show(da);
#ifndef __CYGWIN__
      installRemoteAtoms(dev->topWindow->window);
#endif
    }
  
  gtk_widget_grab_focus(da);
  /* so we get keypresses when we have focus */
  gtk_widget_set_sensitive(da, TRUE);

  /* insert it into the ring of all graphs */
  graphGtkWinInsert(dev);
 
   /* Now it's there, we can make its cursor right */
  changeCursor();

  return dev;
}

static void devGtkDestroy (GraphDev dev)
{
  /* remove from the ring */
  graphGtkWinRemove(dev);
  
  if (dev->topWindow)
    graphGtkOnModalWinDestroy(dev->topWindow);
  
  if (dev->message)
    gtk_widget_destroy(dev->message);

  /* if the widget has already been destroyed, its address will be inserted 
     in destroyed by destroyEvent */
  if (assInsert(destroyed, dev->drawingArea, NULL))
    {
      if (dev->topWindow)
	gtk_widget_destroy(dev->topWindow);
      else if (dev->scroll)
	gtk_widget_destroy(dev->scroll);
      else
	gtk_widget_destroy(dev->drawingArea);
    }
  messfree(dev);
}

static void devGtkCleanup(GraphDev dev)
{  
  if (dev->topWindow)
    graphGtkWinCleanup(dev->topWindow);
}

static void destroyEvent (GtkWidget *w, gpointer data)
{ 
  GraphDev dev = (GraphDev) data;
  
   assInsert(destroyed, dev->drawingArea, NULL); 
  /* so we don't try and destroy it again */ 

  graphCBDestroy(dev->graph);
}

/*****************************************************************/

static void devGtkGoto (GraphDev dev, int x, int  y)
     /* tries to center x,y in visible region */
{
  GtkAdjustment *v;
  GtkAdjustment *h;

  if (!dev || !dev->scroll)
    return;

  v = gtk_scrolled_window_get_vadjustment(GTK_SCROLLED_WINDOW(dev->scroll));
  h = gtk_scrolled_window_get_hadjustment(GTK_SCROLLED_WINDOW(dev->scroll));

  y -= v->page_size/2;
  x -= h->page_size/2;

  if (y<v->lower) y = v->lower;
  if (y>(v->upper-v->page_size)) y = v->upper-v->page_size;  
  gtk_adjustment_set_value(v, y);
  
  if (x<h->lower) x = h->lower;
  if (x>(h->upper-h->page_size)) x = h->upper-h->page_size;
  gtk_adjustment_set_value(h, x);
}
  

static void devGtkScrollWinSize(GraphDev dev, GraphDevOrientation orientation,
				float *size, float *position, float *shown)
/* Johan Sebastian would have been proud............... */
{ 
  GtkAdjustment *a;

  *size = 0;
  *position = 0;
  *shown = 1;

  if (dev->scroll)
    {
      gtk_widget_realize(dev->drawingArea);
      gtk_widget_realize(dev->scroll);
      if ( orientation == GRAPHDEV_HORIZONTAL)
	a = gtk_scrolled_window_get_hadjustment(GTK_SCROLLED_WINDOW(dev->scroll));
      else
	a = gtk_scrolled_window_get_vadjustment(GTK_SCROLLED_WINDOW(dev->scroll));
      *position = a->value;
      *shown = a->page_size;
    }
  else if (dev->topWindow) /* protection for naked graphs */ 
    {
      gint w, h;
      gdk_window_get_geometry(dev->topWindow->window,
			      NULL, NULL, &w, &h, NULL);
      if ( orientation == GRAPHDEV_HORIZONTAL)
	*size  = (float)(w-dev->width_fudge); 
      else
	*size = (float)(h-dev->height_fudge); 
    }
  else
    { 
      GtkRequisition req;
      gtk_widget_size_request(dev->drawingArea, &req);
      if ( orientation == GRAPHDEV_HORIZONTAL)
	*size = (float)(req.width);
      else
	*size = (float)(req.height);
    }
}

static void devGtkWindowSize (GraphDev dev, 
			      float *wx, float *wy, float *ww, float *wh)
{ gint w, h;

  *wx = 0;
  *wy = 0;

  if (dev->topWindow) /* protection for naked graphs */ 
    {
      gdk_window_get_geometry(dev->topWindow->window,
			      NULL, NULL, &w, &h, NULL);
      *ww = (float)(w-dev->width_fudge); 
      *wh = (float)(h-dev->height_fudge); 
    }
  else
    {
      GtkRequisition req;
      gtk_widget_size_request(dev->drawingArea, &req);
      *ww = (float)(req.width);
      *wh = (float)(req.height);
    }
  
}


/* sorry there is a gross hack here, window size setting has been upset for programs
 * like blixem that have several scrolled windows by the introduction of the 
 * dev->topWindow into the "if" test. I have added these cover routines that can be
 * called to circumvent this test. */
static void devGtkSetBaseWinSize(GraphDev dev, GraphDevGraphType graph_type, 
				 int width, int height)
{ 
  setWindowSize(dev, graph_type, width, height, FALSE) ;

  return ;
}

static void devGtkSetNakedWinSize(GraphDev dev, GraphDevGraphType graph_type, 
				  int width, int height)
{ 
  setWindowSize(dev, graph_type, width, height, TRUE) ;

  return ;
}

static void setWindowSize(GraphDev dev, GraphDevGraphType graph_type, 
			  int width, int height, BOOL ignore_dev_window)
{ 
  /* If the window cannot scroll in a particular direction, don't set the
     size in that dimension */
  if (width == 0)
    width = -2;

  if (height == 0)
    height = -2;

  if ((ignore_dev_window || (!ignore_dev_window && dev->topWindow)) &&
      graph_type != GRAPHDEV_TEXT_SCROLL &&
      graph_type != GRAPHDEV_TEXT_FULL_SCROLL &&
      graph_type != GRAPHDEV_PIXEL_SCROLL &&
      graph_type != GRAPHDEV_PIXEL_VSCROLL)
    height = -2;

  if ((ignore_dev_window || (!ignore_dev_window && dev->topWindow)) &&
      graph_type != GRAPHDEV_TEXT_HSCROLL &&
      graph_type != GRAPHDEV_TEXT_FULL_SCROLL &&
      graph_type != GRAPHDEV_PIXEL_SCROLL &&
      graph_type != GRAPHDEV_PIXEL_HSCROLL)
    width = -2;


#ifdef BIG_SCROLL
  if (dev->scroll)
    {
      GtkWidget *viewport = GTK_BIN(dev->scroll)->child;
      GtkRequisition req;
      req.width = width;
      req.height = height;

      gtk_widget_size_request(viewport, &req);
    }
  else
#endif


  gtk_widget_set_usize(dev->drawingArea, width, height);

  return ;
}

/***************************************************************************/ 
static void devGtkRetitle (GraphDev dev, char *title)
{
  GtkWidget *p = dev->drawingArea;
  while (p->parent && (!GTK_IS_WINDOW(p)))
    p = p->parent;
  if (GTK_IS_WINDOW(p))
    gtk_window_set_title(GTK_WINDOW(p), title);
}

static void devGtkPop (GraphDev dev)
{
  GtkWidget *p = dev->drawingArea;
  while (p->parent && (!GTK_IS_WINDOW(p)))
    p = p->parent;
  if (GTK_IS_WINDOW(p))
    gdk_window_show(p->window);

#ifdef __CYGWIN__
  SetFocus(GDK_WINDOW_XWINDOW(p->window));
#endif
  
}

/**************************************************************************/
/* XPM */
static char *info04[] = {
/* width height num_colors chars_per_pixel */
"    32    32        5            1",
/* colors */
". c #000000",
"# c #808080",
"a c None",
"b c #0000ff",
"c c #ffffff",
/* pixels */
"aaaaaaaaaaa########aaaaaaaaaaaaa",
"aaaaaaaa###acccccca###aaaaaaaaaa",
"aaaaaa##acccccccccccca##aaaaaaaa",
"aaaaa#acccccccccccccccca#aaaaaaa",
"aaaa#cccccccabbbbaccccccc.aaaaaa",
"aaa#ccccccccbbbbbbcccccccc.aaaaa",
"aa#cccccccccbbbbbbccccccccc.aaaa",
"a#acccccccccabbbbaccccccccca.aaa",
"a#cccccccccccccccccccccccccc.#aa",
"#acccccccccccccccccccccccccca.#a",
"#ccccccccccbbbbbbbccccccccccc.#a",
"#ccccccccccccbbbbbccccccccccc.##",
"#ccccccccccccbbbbbccccccccccc.##",
"#ccccccccccccbbbbbccccccccccc.##",
"#ccccccccccccbbbbbccccccccccc.##",
"#acccccccccccbbbbbcccccccccca.##",
"a#cccccccccccbbbbbcccccccccc.###",
"a#accccccccccbbbbbccccccccca.###",
"aa#ccccccccbbbbbbbbbccccccc.###a",
"aaa.cccccccccccccccccccccc.####a",
"aaaa.cccccccccccccccccccc.####aa",
"aaaaa.acccccccccccccccca.####aaa",
"aaaaaa..acccccccccccca..####aaaa",
"aaaaaaa#...acccccca...#####aaaaa",
"aaaaaaaa###...accc.#######aaaaaa",
"aaaaaaaaaa####.ccc.#####aaaaaaaa",
"aaaaaaaaaaaaa#.ccc.##aaaaaaaaaaa",
"aaaaaaaaaaaaaaa.cc.##aaaaaaaaaaa",
"aaaaaaaaaaaaaaaa.c.##aaaaaaaaaaa",
"aaaaaaaaaaaaaaaaa..##aaaaaaaaaaa",
"aaaaaaaaaaaaaaaaaa###aaaaaaaaaaa",
"aaaaaaaaaaaaaaaaaaa##aaaaaaaaaaa"
};


/* READ THIS.......this code is called by graphMessage() and graphUnMessage() only.
 * These functions have been replaced largely by gexNNN() routines that are called using the
 * messNNN callback system. These functions are for compatibility with systems such
 * as the Image system which probably still use these calls. There are about 20 places
 * in acedb code that use these calls and should be converted to use messNNN() calls.
 * IN OTHER WORDS, DON'T EXPEND LOADS OF EFFORT ON THIS CODE, INSTEAD IMPROVE THE
 * gexNNN() ROUTINES !! */


static GraphDev devWithMessage = NULL;

static gint messageDestroy (GtkWidget *w, gpointer data)
{
  GraphDev dev = (GraphDev) data ;
  dev->message = 0 ;
  graphCBMessageDestroy(dev->graph);
  devWithMessage = NULL;
  return TRUE;
}



/* Memo: this is non-blocking */
static void devGtkMessage (GraphDev dev, char *text)
{ 
  GtkWidget *button, *label_button, *dialog;
  GdkPixmap *pixmap;
  GtkStyle  *style;
  GdkBitmap *mask;
  GtkWidget *pixmapwid;
  GtkWidget *hbox;


  if (devWithMessage)
    {
      /* Already exists, just update text */


      /* If another graph had popped a message: we can only have one, 
       * so we tell it that it it's message has gone before re-using */
      if (devWithMessage != dev)
	{
	  graphCBMessageDestroy(devWithMessage->graph);
	  dev->message = devWithMessage->message;
	  devWithMessage->message = NULL;
	  devWithMessage = dev;
	}
      

      label_button = GTK_WIDGET(gtk_object_get_data(GTK_OBJECT(dev->message), 
						    "_label_widget"));

      gtk_object_set(GTK_OBJECT(label_button), "GtkButton::label", text, NULL) ;
    }
  else
    {
      /* Put main dialog text in its own button so it can be cut/pasted. */
      label_button = gtk_button_new_with_label(text);
      gtk_button_set_relief(GTK_BUTTON(label_button), GTK_RELIEF_NONE) ;
      gtk_selection_add_targets(label_button, GDK_SELECTION_PRIMARY,
				targetlist, ntargets);
      gtk_signal_connect(GTK_OBJECT(label_button), "clicked",
			 GTK_SIGNAL_FUNC(butClick), label_button) ;
      gtk_signal_connect(GTK_OBJECT(label_button), "selection_get",
			 GTK_SIGNAL_FUNC(graphGtkSelectionGet), NULL);
            
      dialog = gtk_dialog_new();

      gtk_object_set_data(GTK_OBJECT(dialog), "_label_widget", 
			  (gpointer)label_button);

      gtk_widget_set_name(dialog, "acedb message popup");
      gtk_widget_realize(dialog);
      
      style = gtk_widget_get_style(GTK_WIDGET(dialog));
      pixmap = gdk_pixmap_create_from_xpm_d(dialog->window,  &mask,
					    &style->bg[GTK_STATE_NORMAL],
					    (gchar **)info04 );
      pixmapwid = gtk_pixmap_new( pixmap, mask );
      
      hbox = gtk_hbox_new(FALSE, 0);
      gtk_box_pack_start(GTK_BOX(hbox), pixmapwid, FALSE, FALSE, 5);
      gtk_box_pack_start(GTK_BOX(hbox), label_button, FALSE, FALSE, 10);
      
      gtk_window_set_title(GTK_WINDOW(dialog), "Acedb message");
      
      button = gtk_button_new_with_label("Remove");
      
      gtk_signal_connect_object(GTK_OBJECT(button), "clicked",
				(GtkSignalFunc) gtk_widget_destroy,
				GTK_OBJECT(dialog));
      gtk_signal_connect(GTK_OBJECT(dialog), "destroy",
			 (GtkSignalFunc) messageDestroy,
			 (gpointer) dev);
      gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->action_area), button,
			 TRUE, TRUE, 0);
      
      gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->vbox), hbox,
			 TRUE, TRUE, 0);
      
      GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);
      gtk_widget_grab_default(button);
      gtk_widget_grab_focus(button);

      gdk_pixmap_unref(pixmap);
      gdk_pixmap_unref(mask);
      gtk_widget_show_all(dialog);
      dev->message = dialog;
      
    }
}

static void devGtkUnMessage (GraphDev dev) 
{ 
  if (dev && dev->message)
    { 
      gtk_widget_destroy(dev->message) ;
      dev->message = 0 ;
   }
}

/* When user clicks on message text itself, we put the text in the cut buffer. */
static void butClick(GtkButton *button, gpointer user_data)
{
  GtkWidget *label ;
  char *label_text = NULL ;

  label = GTK_BIN(button)->child ;

  gtk_label_get(GTK_LABEL(label), &label_text) ;

  graphGtkSetSelectionOwner(GTK_WIDGET(button), label_text) ;

  return ;
}



/****************************************************************************/
static gint enterEvent (GtkWidget *da, GdkEventCrossing *ev, gpointer data)
{
  if (ev->mode == GDK_CROSSING_NORMAL)
    gtk_widget_grab_focus(da);
  
  return TRUE;
}


static gint exposeEvent (GtkWidget *da, GdkEventExpose *ev, gpointer data)
{
  GraphDev dev = (GraphDev) data;

  if (!dev->doingExposure)
    {
      dev->doingExposure = TRUE;
      dev->xmin = ev->area.x ; 
      dev->xmax = dev->xmin + ev->area.width ;
      dev->ymin = ev->area.y ;
      dev->ymax = dev->ymin + ev->area.height ;
    }
  else
    {
      if (dev->xmin > ev->area.x) 
	dev->xmin = ev->area.x ;
      if (dev->xmax < ev->area.x + ev->area.width) 
	dev->xmax = ev->area.x + ev->area.width ;
      if (dev->ymin > ev->area.y) 
	dev->ymin = ev->area.y ;
      if (dev->ymax < ev->area.y + ev->area.height) 
	dev->ymax = ev->area.y + ev->area.height ;
    }

  if (ev->count == 0)
    {
      graphCBExposeRedraw(dev->graph,
			  dev->xmin, dev->ymin, dev->xmax, dev->ymax);

      dev->doingExposure = FALSE;
    }
    
  return TRUE;
}


static gint configureEvent (GtkWidget *da, 
			    GdkEventConfigure *ev, gpointer data)
{
  /* Note that we can see spurious events with width == 1 or height == 1 as the
     viewport sizing sorts itself out. Don't pass these on as some
     programs barf on them. */
  GraphDev dev = (GraphDev) data;

  if ((ev->width > 1) && (ev->height > 1))
    graphCBResize(dev->graph, ev->width, ev->height);

  return TRUE;
}




/* Don't like the default behaviour, so we position down and to the right 
   a few pixels: this stops the first menuitem getting selected until 
   one moves the mouse */
static void menuPosition(GtkMenu *menu, gint *x, gint *y, gpointer data)
{
  gint screen_width;
  gint screen_height;
  GtkRequisition requisition;
  
  gtk_widget_size_request (GTK_WIDGET(menu), &requisition);
  screen_width = gdk_screen_width ();
  screen_height = gdk_screen_height ();
  
  *x += 2;
    
  if (((*x) + requisition.width) > screen_width)
    *x -= (((*x) + requisition.width) - screen_width);
  if (*x < 0)
    *x = 0;
  if (((*y) + requisition.height) > screen_height)
    *y -= (((*y) + requisition.height) - screen_height);
  if (*y < 0)
	*y = 0;
}


static gint motionEvent(GtkWidget *da, GdkEventMotion *ev, gpointer data)
{
  GraphDev dev = (GraphDev)data;
  int x,y;
  GdkModifierType state;
  
  gdk_window_get_pointer(ev->window, &x, &y, &state);
  
  if (state & GDK_BUTTON1_MASK)
    {
      /* Shift-leftbutton == middle button */
      if (state & GDK_SHIFT_MASK)
	graphCBMouse(dev->graph, x, y, GRAPHDEV_MIDDLE_DRAG);
      else
	graphCBMouse(dev->graph, x, y, GRAPHDEV_LEFT_DRAG);
    }
  else if (state & GDK_BUTTON2_MASK)
    graphCBMouse(dev->graph, x, y, GRAPHDEV_MIDDLE_DRAG);
  else if (state & GDK_BUTTON3_MASK)
    graphCBMouse(dev->graph, x, y, GRAPHDEV_RIGHT_DRAG);

  return TRUE;
}

static gint buttonClickEvent(GtkWidget *da, GdkEventButton *ev, gpointer data)
{ 
  GraphDev dev = (GraphDev) data;
  MENU menu = 0;
  static GraphDevEventType stickyButton = GRAPHDEV_LEFT_UP;
  GraphKeyModifier modifier ;
    
  if (pollLevel != 0)
    return TRUE; /* ignore re-entrancy */
  
  /* Set up modifiers, if any.... */
  modifier = NULL_MODKEY ;
  if ((ev->state & GDK_CONTROL_MASK))
    modifier |= CNTL_MODKEY ;
  if ((ev->state & GDK_SHIFT_MASK))
    modifier |= SHIFT_MODKEY ;
  if ((ev->state & GDK_MOD1_MASK))
    modifier |= ALT_MODKEY ;

  if (ev->button == 1 && ev->type == GDK_BUTTON_PRESS)
    {
      /* For two-button mice, make shift-left button = middle button */
      if (ev->state & GDK_SHIFT_MASK)
	{
	  if (graphCBMiddle(dev->graph, ev->x, ev->y))
	    gtk_grab_add(da);
	  /* The stickyButton code make the up event seem to come from the
	     middle button also */
	  stickyButton = GRAPHDEV_MIDDLE_UP;
	}
      else
	{
	  if (graphCBPick(dev->graph, ev->x, ev->y, modifier))
	    gtk_grab_add(da);
	  stickyButton = GRAPHDEV_LEFT_UP;
	}
    }
  else if (ev->button == 1 && ev->type == GDK_BUTTON_RELEASE)
     {
       gtk_grab_remove(da);
       graphCBMouse(dev->graph, ev->x, ev->y, stickyButton);
       stickyButton = GRAPHDEV_LEFT_UP;
     }
  else if (ev->button == 2 && ev->type == GDK_BUTTON_PRESS)
    { 
     if (graphCBMiddle(dev->graph, ev->x, ev->y))
       gtk_grab_add(da);
    }
  else if (ev->button == 2 && ev->type == GDK_BUTTON_RELEASE)
    {
      gtk_grab_remove(da);
      graphCBMouse(dev->graph, ev->x, ev->y, GRAPHDEV_MIDDLE_UP);
    }
  else if (ev->button == 3 && ev->type == GDK_BUTTON_PRESS)
    {
      menu = graphCBMenuPopup(dev->graph, ev->x, ev->y);
      if (menu)
	{
	  GtkWidget *gMenu;
	  /* Use the same colormap for the menu - looks better */
	  BOOL oldCm = graphGtkSetGreyMap(dev->hasGreyCmap);
	  gMenu = menu2Widget(menu);
	  /* The menu we make contains the seeds of its own destruction */
	  gtk_signal_connect_object(GTK_OBJECT(gMenu), "selection-done",
				    GTK_SIGNAL_FUNC(gtk_widget_destroy),
				    GTK_OBJECT(gMenu));
	  gtk_menu_popup(GTK_MENU(gMenu), NULL, NULL, 
			 (GtkMenuPositionFunc)menuPosition, NULL, 
			 ev->button, ev->time);
	  graphGtkSetGreyMap(oldCm);
	}
    }
  return TRUE;
}

static gint propertyEvent(GtkWidget *topWin,
			   GdkEventProperty *ev, 
			   gpointer data)
{
  GraphDev dev = (GraphDev) data;
  static GdkAtom remoteCommandAtom = 0;
  static GdkAtom stringAtom = 0;
  static GdkAtom responseAtom = 0;
  GdkAtom actualType;
  gint actualFormat;
  gint nitems;
  guchar *commandText;

  if (!remoteCommandAtom)
    remoteCommandAtom = gdk_atom_intern("_XREMOTE_COMMAND", FALSE);
  if (!stringAtom)
    stringAtom = gdk_atom_intern("STRING", FALSE);
  if (!responseAtom)
    responseAtom = gdk_atom_intern("_XREMOTE_RESPONSE", FALSE);
  
  if (ev->atom != remoteCommandAtom)
    return FALSE; /* not for us */
  
  if (ev->state != 0) /* zero is the value for PropertyNewValue, in X.h
			 there's no gdk equivalent that I can find. */
    return TRUE;

  if (!gdk_property_get(ev->window,
			remoteCommandAtom,
			stringAtom,
			0,
			(65536/ sizeof(long)),
			TRUE,
			&actualType,
			&actualFormat,
			&nitems,
			&commandText))
    {
      messerror("X-Atom remote control : unable to read "
		"_XREMOTE_COMMAND property");
      return TRUE;
    }
  else if (!commandText || nitems == 0)
    {
      messerror("X-Atom remote control : invalid data on property");
      return TRUE;
    }
  else
    { 
      guchar *responseText;
      char *copyCommand = messalloc(nitems+1);
      /* commandText is no zero terminated */
      memcpy(copyCommand, commandText, nitems);
      copyCommand[nitems] = 0;
      graphCBSetActive(dev->graph);
      responseText = (guchar *)graphCBRemote((char *)copyCommand);
      if (!responseText)
	messcrash("propertyCall - NULL response from command call");
      gdk_property_change(ev->window,
			  responseAtom,
			  stringAtom,
			  8,
			  GDK_PROP_MODE_REPLACE,
			  responseText,
			  strlen((const char *)responseText));
      
      messfree(responseText);
      messfree(copyCommand);
    }

  if (commandText)
    g_free(commandText);

  return TRUE;
}

static void installRemoteAtoms(GdkWindow *window)
{
  char *versionString = "1.0";
  char *appName = messGetErrorProgram();
  static GdkAtom versionAtom = 0;
  static GdkAtom stringAtom = 0;
  static GdkAtom applicationAtom = 0;

  if (!stringAtom)
    stringAtom = gdk_atom_intern("STRING", FALSE);
  if (!versionAtom)
    versionAtom = gdk_atom_intern("_XREMOTE_VERSION", FALSE);
  if (!applicationAtom)
    applicationAtom = gdk_atom_intern("_XREMOTE_APPLICATION", FALSE);

  gdk_property_change(window,
		      versionAtom,
		      stringAtom,
		      8, 
		      GDK_PROP_MODE_REPLACE,
		      (unsigned char *)versionString,
		      strlen(versionString)) ;

  gdk_property_change(window,
		      applicationAtom,
		      stringAtom,
		      8, 
		      GDK_PROP_MODE_REPLACE,
		      (unsigned char *)appName,
		      strlen(appName)) ;
}

static gint keyboardEvent(GtkWidget *da, GdkEventKey *ev, gpointer data)
{
  gint result = TRUE ;
  GraphDev dev = (GraphDev)data ;
  int kval ;
  GraphDevKeyType key_type = GRAPHDEV_NOKEY ;
  GraphKeyModifier modifier ;


  /* Translate dev-specific modifiers to graph_dev modifiers. */
  modifier = NULL_MODKEY ;
  if ((ev->state & GDK_CONTROL_MASK))
    modifier |= CNTL_MODKEY ;
  if ((ev->state & GDK_SHIFT_MASK))
    modifier |= SHIFT_MODKEY ;
  if ((ev->state & GDK_MOD1_MASK))
    modifier |= ALT_MODKEY ;


#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
  /* For naked windows, never deal with control and alt
     keys - leave them for accelerators. Note that this is not a complete
     solution, I think that we will probably have to remove keyboard handling
     completely from the graph package long term - it just can't co-exist
     with GTK effectively */
  if ((!dev->topWindow) && (ev->state & (GDK_CONTROL_MASK | GDK_MOD1_MASK)))
    return FALSE;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

  /* Let's try something a bit better.... */
  /* For non-acedb windows we don't handle Alt and we only handle Cntl if there are
   * "global" callbacks registered, otherwise the key presses are passed on to the application. */
  if (!(dev->topWindow) && (modifier & ALT_MODKEY))
    {
      /* we have no registered callbacks for the Alt key so let the application handle it. */
      return FALSE ;
    }


  /* We assume that if  ev->length > 0 then the key is a printable char, otherwise its
   * a control char. */
  if (ev->length)
    {
      /* Key is printable char. */

      kval = ev->string[0] ;

      if (modifier & CNTL_MODKEY)
	{
	  switch (ev->keyval)
	    {
	    case GDK_C:
	    case GDK_c: 
	      key_type = GRAPHDEV_COPYKEY;
	      break;
	    case GDK_X:
	    case GDK_x: 
	      key_type = GRAPHDEV_CUTKEY;
	      break;
	    case GDK_V:
	    case GDK_v: 
	      key_type = GRAPHDEV_PASTEKEY;
	      break;

	    case GDK_H:
	    case GDK_h: 
	      key_type = GRAPHDEV_HELPKEY;
	      break;
	    case GDK_W:
	    case GDK_w: 
	      key_type = GRAPHDEV_CLOSEKEY;
	      break;
	    case GDK_P:
	    case GDK_p: 
	      key_type = GRAPHDEV_PRINTKEY;
	      break;
	    case GDK_S:
	    case GDK_s: 
	      key_type = GRAPHDEV_SAVEKEY;
	      break;
	    case GDK_Q:
	    case GDK_q: 
	      key_type = GRAPHDEV_QUITKEY;
	      break;
	    }
	}
    }
  else
    {
      /* Key is control char. */

      switch (ev->keyval)
	{
	case GDK_Page_Up:
	  kval = PAGE_UP_KEY ; break ;
	case GDK_Page_Down:
	  kval = PAGE_DOWN_KEY ; break ;
	case GDK_F4:
	case 0xFF03: /* ctrl-C */
	  return FALSE; /* leave it for the window handler */
	case GDK_F1: 
	case GDK_F10: 
	case 65386:
	  key_type = GRAPHDEV_HELPKEY;
	  break;
	case GDK_F2:
	case GDK_F9:
	  /*    case 65478: duplicate */
	  key_type = GRAPHDEV_PRINTKEY;
	  break;
	  /* hard coding keys r1 r2 r3 r5 as arrow keys for the sun under openlook */
	case GDK_Delete:
	  kval = DELETE_KEY ; break ;
	case GDK_BackSpace:
	  kval = BACKSPACE_KEY ; break ;

	case 65491: 
	case GDK_Up:
	  kval = UP_KEY ; break ;
	case 65494: 
	case GDK_Down:
	  kval = DOWN_KEY ; break ;

	case 65490:
	case GDK_Left:
	case 65492:
	case GDK_Right:
	  {
	    /* Only do scrolling at gtk level if keyboard handling is enabled. */
	    if (dev->keyboard_handling && dev->type == GRAPHDEV_TEXT_HSCROLL
		&& (modifier == NULL_MODKEY || modifier == CNTL_MODKEY))
	      {
		GtkScrollType scrolling ;

		if (modifier == CNTL_MODKEY)
		  scrolling = GTK_SCROLL_PAGE_FORWARD ;
		else
		  scrolling = GTK_SCROLL_STEP_FORWARD ;

		moveHorizScrollBar(dev->scroll, scrolling, ev->keyval) ;
		
		return TRUE ;				    /* N.B. we don't call graph layer. */
	      }
	    else
	      {
		if (ev->keyval == GDK_Left || ev->keyval == 65490)
		  kval = LEFT_KEY ;
		else
		  kval = RIGHT_KEY ;
	      }
	    break ;
	  }
	case GDK_Home:
	  kval = HOME_KEY ; break ;
	case GDK_End:
	  kval = END_KEY ; break ;
	case GDK_Tab:
	  kval = TAB_KEY; break ;
	case GDK_Return:
	  kval = RETURN_KEY; break ;
	case 65379:
	  kval = INSERT_KEY ; break ;
	default:
	  kval = 0 ; break ;
	}
    }


  /* TODO SRK - i've elided some nasty stuff to do with busy cursors here -
     gtk forces a re-think of that anyway, and given the nastyness currently
     involved that's good. */

  /* Some keys are "special" because they cause application-wide actions to happen,
   * all others are treated as "normal" keys to be passed directly to the application. */
  if (key_type == GRAPHDEV_NOKEY)
    key_type = GRAPHDEV_NORMALKEY;
  
  /* if we have extra widgets don't handle Tab so it can be used to move the focus */
  if (dev->hasChrome && (kval == TAB_KEY))
    {
      result = FALSE ;
    }
  else if (kval)
    {
      result = graphCBKeyboard(dev->graph, key_type, kval, modifier) ;
    }
  
  return result ;
}

/* A pair of functions to enable/disable keyboard handling of some events
 * at the gtk level, e.g. horizontal scrolling. This is necessary because
 * otherwise gtk intefers with some graph level code that makes use of the
 * arrow keys etc., for instance in the text entry box. */
static void disableKeyboardHandling(GraphDev dev)
{
  dev->keyboard_handling = FALSE ;
  
  return ;
}

static void enableKeyboardHandling(GraphDev dev)
{
  dev->keyboard_handling = TRUE ;
  
  return ;
}



/****************************************************************************/
/* Busy cursors, interrupt flags, and main loops. This lot all poke each
   other via the globals below. Try and avoid having to understand this 
   if you can. */


static BOOL seenF4 = FALSE;  
static BOOL resetTimer = FALSE;
static BOOL busySet = FALSE;
static BOOL isModal = FALSE;
static GtkWidget *modalWin;
static GtkWidget *savedModalWin = NULL;
static int loopReturn;

/* this gets called on any key, not just when the drawing area has focus */
static gint windowKeyEvent(GtkWidget *tw, GdkEventKey *ev, gpointer data)
{ 
  if (ev->keyval == GDK_F4)
    { 
      seenF4 = TRUE;
      return TRUE;
    }
#ifdef __CYGWIN__
  if ((ev->state & GDK_CONTROL_MASK) && (ev->keyval == 'C')) /* ctrl-C */
    {
      int size = selection_string ? strlen(selection_string) : 0;
      HGLOBAL hdata = GlobalAlloc (GHND, size+1);
      char *ptr = GlobalLock (hdata);
      if (selection_string)
	strcpy(ptr, selection_string);
      GlobalUnlock(hdata);
      if (OpenClipboard(NULL))
	{
	  if (EmptyClipboard())
	    SetClipboardData(CF_TEXT, hdata);
	  CloseClipboard();
	}
      return TRUE;
    }
#endif

  return FALSE;
}

#define nenty_width 16
#define nenty_height 16
static unsigned char nenty_bits[] = {
  0xe0, 0x07, 0xf8, 0x1f, 0xfc, 0x3e, 0x1e, 0x78, 0x0e, 0x7c, 0x07, 0xee,
  0x07, 0xe7, 0x83, 0xe3, 0xc7, 0xc1, 0xe7, 0xe0, 0x77, 0xe0, 0x3e, 0x70,
  0x1e, 0x78, 0x7c, 0x3f, 0xf8, 0x1f, 0xe0, 0x07, };

static void changeCursor(void)
{
  GraphDev ringPtr = allDevs;
  static GdkCursor *LeftArrow;
  static GdkCursor *NoEntry;
  static GdkCursor *Watch ;

  if (!ringPtr) return;

  if (!LeftArrow) 
    LeftArrow  = gdk_cursor_new(GDK_TOP_LEFT_ARROW);
  if (!Watch) 
    Watch = gdk_cursor_new(GDK_WATCH);
  if (!NoEntry) 
    {
      GdkColor black;
      GdkPixmap *bm = gdk_bitmap_create_from_data (NULL, nenty_bits, 
                                                   nenty_width, nenty_height);
      gdk_color_black (gdk_colormap_get_system (), &black);
      NoEntry = gdk_cursor_new_from_pixmap(bm, bm, &black, &black, 1, 1);
      gdk_pixmap_unref(bm);
    }

  do { 
    if (ringPtr->topWindow && ringPtr->topWindow->window)
      {
        if (isModal && (modalWin != ringPtr->topWindow))
          gdk_window_set_cursor(ringPtr->topWindow->window, NoEntry) ;
        else if (busySet)
	  gdk_window_set_cursor(ringPtr->topWindow->window, Watch) ;
	else
          gdk_window_set_cursor(ringPtr->topWindow->window, LeftArrow) ;
        
      }
    ringPtr = ringPtr->ring;
  } while (ringPtr != allDevs);
}

void graphGtkSetModalWin(GtkWidget *win, BOOL modal)
{ 
  /* we save one level of modal windows; that gets restored
     when an exiting one loses it */

  if (modal)
    {
      if (isModal && (modalWin != win))
	{
	  savedModalWin = modalWin;
	  gtk_window_set_modal(GTK_WINDOW(modalWin), FALSE);
	}
      gtk_window_set_modal(GTK_WINDOW(win), TRUE); 
      modalWin = win;
      isModal = TRUE;
    }
  else
    { 
      if (win)
	gtk_window_set_modal(GTK_WINDOW(win), FALSE);
      if (savedModalWin)
	{  
	  gtk_window_set_modal(GTK_WINDOW(savedModalWin), TRUE);
	  modalWin = savedModalWin;
	  savedModalWin = NULL;
	  isModal = TRUE;
	}
      else
	isModal = FALSE;
    }
	    
  changeCursor();
}

void graphGtkOnModalWinDestroy(GtkWidget *win)
{ 
  if (win == savedModalWin)
    savedModalWin = NULL;
  if (isModal && (win == modalWin))
    graphGtkSetModalWin(NULL, FALSE); /* win may be destroyed by now */
}
 
int graphGtkMainLoop(GtkWidget *win)
{
  if (win)
    graphGtkSetModalWin(win, TRUE);

  gtk_main();

  graphGtkOnModalWinDestroy(win); /* If the window is still around, 
				     remove the modality */
  
  return loopReturn;
}

void graphGtkLoopReturn(int retval)
{
   loopReturn = retval;
   gtk_main_quit();
}

static void devGtkSetModalWin(GraphDev dev, BOOL modal, BOOL override)
{
  if (dev->topWindow)
    { 
      if (override && !isModal)
	return;
      graphGtkSetModalWin(dev->topWindow, modal);
    }
}

static int devGtkLoop(GraphDev dev,  BOOL isBlock)
{ 
  if (dev && dev->topWindow && isBlock)
    return graphGtkMainLoop(dev->topWindow);
  else
    return graphGtkMainLoop(NULL);
}

static void devGtkLoopReturn (GraphDev dev, int retval)
{ 
  graphGtkLoopReturn(retval);
}

static gint unsetBusy(gpointer data)
{  
  /* The main-loop iteration called from devGtkInteruptCalled allows us to run
     but we don't want to re-set the busy pointer then, so we detect this 
     situation (pollLevel>0) and set a flag which devInterruptCalled notices
     and therefore re-sets the callback, so we get called again.
     Note that merely returning TRUE here results in an infinite loop with
     devInterruptCalled never completing. */

  if (pollLevel) 
    { resetTimer = TRUE;
      return FALSE;
    }

  if (!busySet) 
    return FALSE;

  busySet = FALSE;
  changeCursor();
  messStatus (NULL);
  
  return FALSE;
}


static BOOL devGtkInterruptCalled(BOOL forceInterrupt)
{ 
  pollLevel++;

  while (gtk_events_pending())
    gtk_main_iteration();

  pollLevel--;

  if ((pollLevel == 0) && resetTimer)
    {
      g_idle_add_full(G_PRIORITY_HIGH, unsetBusy, NULL, NULL);
      resetTimer = FALSE;
    }

  if (forceInterrupt)
    seenF4 = TRUE;

  return seenF4;
}

/* This is needed so application can allow user to change their mind about   */
/* f4 interrupt.                                                             */
static void devGtkResetInterrupt(void)
{
  seenF4 = FALSE;

  return ;
}

static void devGtkBusyCursorAll ()
{
  if (busySet) 
    return;
  
  g_idle_add_full(G_PRIORITY_HIGH, unsetBusy, NULL, NULL);
  seenF4 = FALSE;
  busySet = TRUE;
  
  changeCursor();
  gdk_flush();
}



/********* cut and paste stuff ************/


/* These dev routines are called by graphPost/Paste() */
static void devGtkPostBuffer (GraphDev dev, char *text)
{
  if (dev)
    graphGtkSetSelectionOwner(dev->drawingArea, text) ;

  return ;
}

static void devGtkPasteBuffer(GraphDev dev)
{
  GdkAtom type;

#ifdef __CYGWIN__
  type = gdk_atom_intern ("CLIPBOARD", FALSE);
#else
  type = GDK_SELECTION_PRIMARY;
#endif
  
  gtk_selection_convert(dev->drawingArea, 
			type,
			gdk_atom_intern("STRING", FALSE),
			GDK_CURRENT_TIME);
}


/* These routines are called by the dev routines but also by gex/graphgtk code. */
void graphGtkSelectionGet (GtkWidget *widget, 
			   GtkSelectionData *selection_data,
			   guint      info,
			   guint      time,
			   gpointer   data)
{
  gint len;
  GdkAtom type = GDK_NONE;

  if (!selection_string)
    len = 0;
  else
    len = strlen((char *)selection_string);

  switch (info)
    {
    case COMPOUND_TEXT:
    case SIMPLE_TEXT:
      type = gdk_atom_intern("COMPOUND_TEXT", FALSE);
    case STRING:
      type = gdk_atom_intern("STRING", FALSE);
    }
  
  gtk_selection_data_set (selection_data, type, 8, selection_string, len);
}

void graphGtkSetSelectionOwner(GtkWidget *widget, char *text)
{
  if (selection_string)
    messfree(selection_string); /* lose old */
      
  selection_string = (unsigned char *)strnew(text, 0);

  /* This stuff is a bit borken under windows for two reasons.
     1) The version of GTK we currently use cannot set clipboard data.
     2) On windows we don't want to mung the clipboard until the user hits
     ctl-C (UI consistency, don'tcha know?)
     So we just set selection string here, and there's code in the
     keyboard handler which calls the windows API directly to 
     actually poke the text into the clipboard.
  */

#ifndef __CYGWIN__
  gtk_selection_owner_set (widget,
			   GDK_SELECTION_PRIMARY,
			   GDK_CURRENT_TIME);
#endif

  return ;
}

/* This is a callback on the gtk window that itself calls back into the graph
 * package to hand over the pasted data. */
static void selection_received(GtkWidget *widget, 
			       GtkSelectionData *selection_data,
			       guint32 time,
			       gpointer data)
{
  GraphDev dev = (GraphDev) data;

  if ((selection_data->length >=  0) && 
      (selection_data->type == gdk_atom_intern("STRING", FALSE)))
    graphCBPaste(dev->graph, selection_data->data);

  return;
}
  

/******************************* menus *************************/

static gint menuSelect (GtkWidget *w, gpointer data)
{
  MENUITEM item = (MENUITEM)data;

  graphCBMenuSelect(item) ;

  /*
  if (item->flags & MENUFLAG_TOGGLE)
    {
      if (item->flags & MENUFLAG_TOGGLE_STATE)
	gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(w), TRUE);
      else
	gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(w), FALSE);
    }
  */

  return TRUE;
} /* menuSelect */

static gint menuBarActivate(GtkWidget *widg, gpointer data)
{ /* called when a menu bar menu pops up, make sure we activate the right
     graph before the menu select call-back,*/

  graphCBSetActive((GraphPtr)data);
  return TRUE;
}

static void devGtkMenuBar(GraphDev dev, MENU menuBar)
{ 
  MENUITEM item;
  MENU menu;
  GtkWidget *root;
  GtkWidget *bar;

  if (dev->menuBar)
    gtk_widget_destroy(dev->menuBar);

  dev->menuBar = bar = gtk_menu_bar_new();
  gtk_box_pack_start(GTK_BOX(dev->vbox), bar, FALSE, FALSE, 0);
  gtk_box_reorder_child(GTK_BOX(dev->vbox), bar, 0);

  for (item = menuBar->items; item; item = item->down)
    if (item->submenu) /* anything else we ignore */
      { 
	menu = item->submenu;
	root = gtk_menu_item_new_with_label(menu->title);
	gtk_menu_item_set_submenu(GTK_MENU_ITEM(root), menu2Widget(menu));
	gtk_signal_connect(GTK_OBJECT(root), "activate",
			   GTK_SIGNAL_FUNC(menuBarActivate),
			   (gpointer)dev->graph);
	gtk_menu_bar_append(GTK_MENU_BAR(bar), root);
	gtk_widget_show(root);
      }

  gtk_widget_show(bar);
 
}
	
static GtkWidget *menu2Widget(MENU menu)
{ 
  MENUITEM item;
  GtkWidget *entry;
  GtkWidget *gMenu = gtk_menu_new();
  BOOL inRadio = FALSE;

  if (menu->title && strlen(menu->title) > 0)
    {
      GtkWidget *accel_label;

      entry = gtk_menu_item_new ();
      accel_label = gtk_accel_label_new (menu->title);
      gtk_misc_set_alignment (GTK_MISC (accel_label), 0.5, 0.5); /* center justified */

      gtk_container_add (GTK_CONTAINER (entry), accel_label);
      gtk_accel_label_set_accel_widget (GTK_ACCEL_LABEL (accel_label), entry);
      gtk_widget_show (accel_label);


      /* This is not good, it makes the title look faded, I need the gtk     */
      /* book to do something about this...                                  */
      gtk_widget_set_sensitive (entry, FALSE); /* title item unselectable */

      gtk_menu_append(GTK_MENU(gMenu), entry);
      gtk_widget_show(entry);
    }

  for (item = menu->items ; item ; item = item->down)
    { 
      unsigned int flags = item->flags ;
      if (flags & MENUFLAG_HIDE) 
	continue ;
      if (flags & MENUFLAG_SPACER)
	{ 
	  entry = gtk_menu_item_new();
	  gtk_menu_append(GTK_MENU(gMenu), entry);
	  gtk_widget_show(entry);
	  continue;
	}

      if (flags & MENUFLAG_TOGGLE)
	{
	  entry = gtk_check_menu_item_new_with_label(item->label);
	  gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(entry),
					 (flags & MENUFLAG_TOGGLE_STATE) ? TRUE : FALSE);
	  gtk_check_menu_item_set_show_toggle(GTK_CHECK_MENU_ITEM(entry), TRUE);
	}
      else if ((flags & MENUFLAG_START_RADIO) || inRadio)
	{ 
	  entry = gtk_check_menu_item_new_with_label(item->label);
	  gtk_check_menu_item_set_show_toggle(GTK_CHECK_MENU_ITEM(entry),
					      TRUE);
	  gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(entry),
					 (flags & MENUFLAG_RADIO_STATE) ? TRUE : FALSE);
	  inRadio = !(flags & MENUFLAG_END_RADIO);
	}
      else
	entry = gtk_menu_item_new_with_label(item->label);
      
      if (item->submenu)
	gtk_menu_item_set_submenu(GTK_MENU_ITEM(entry), 
				  menu2Widget(item->submenu));
      
      gtk_menu_append(GTK_MENU(gMenu), entry);
       
      if (flags & MENUFLAG_DISABLED)
	gtk_widget_set_state(GTK_WIDGET(entry), GTK_STATE_INSENSITIVE);
      else
	gtk_signal_connect(GTK_OBJECT(entry), "activate",
			   GTK_SIGNAL_FUNC(menuSelect), 
			   (gpointer) item);
      gtk_widget_show(entry);
    }

  return gMenu;
} /* menu2Widget */

void graphGtkSetSplash(GtkWidget *splash)
{
  static GtkWidget *displayed = NULL;

  if (displayed)
    gtk_widget_destroy(displayed);

  displayed = splash;
}




/* 
 * routines to add keyboard shortcuts at this level (where possible) rather than
 * at the Graph package level.
 *
 */


/* Implements left/right scrolling by poking the horizontal scroll bar in a scrolled
 * window widget according to what key is sent to it (it expects GDK_Left or GDK_right
 * but this is not checked).
 * I find it INCREDIBLE that I have to do so much myself to get the pigging scroll
 * bar to do what I want and not go off the right hand end, Motif provided an
 * interface that just did this for free...sigh, this interface is an enormously
 * _retrograde_ step....grimness....... */
static void moveHorizScrollBar(GtkWidget *scroll, GtkScrollType scrolling, int key)
{
  GtkAdjustment *h_adjust ; 
  gfloat lower, upper, step, page_size, value ;

  /* Find out where the scroll bar is now. */
  h_adjust = gtk_scrolled_window_get_hadjustment(GTK_SCROLLED_WINDOW(scroll)) ;
  lower = GTK_ADJUSTMENT(h_adjust)->lower ;		    /* 0, we hope... */
  upper = GTK_ADJUSTMENT(h_adjust)->upper ;		    /* total width of fmap */
  if (scrolling == GTK_SCROLL_PAGE_FORWARD)		    /* amount to move slider by. */
    step = GTK_ADJUSTMENT(h_adjust)->page_increment ;
  else
    step = GTK_ADJUSTMENT(h_adjust)->step_increment ;
  page_size = GTK_ADJUSTMENT(h_adjust)->page_size ;	    /* width of fmap visible in scrwindow. */
  value = GTK_ADJUSTMENT(h_adjust)->value ;		    /* current position of _left_ end of
							       slider. */

  /* Send it left or right, being careful to clamp to the min/max, it WON'T do this
   * for you ! */
  if (key == GDK_Left)
    {
      if (value > lower)
	{
	  value -= step ;
	  if (value < lower)
	    value = lower ;
	}
    }
  else
    {
      if ((value + page_size) < upper)
	{
	  if ((value + page_size + step) > upper)
	    value += (upper - (value + page_size)) ;
	  else
	    value += step ;
	}
    }

  if (value != GTK_ADJUSTMENT(h_adjust)->value)		    /* only set if value changed. */
    gtk_adjustment_set_value(h_adjust, value) ;

  return ;
}


 
/****************************************************************************/
/* This initialises the function switch tables which are our interface to the
   device independent stuff. My stategy is this:
   Each function defined in a file gets stuffed into the function switch by
   a routine at the end of that file, called <file_name>_switch_init
   That way I don't have to declare that damn things in a header as well as 
   in the definition of the switches, in fact they can be declared static!

   The switch_init functions are called from graphDevInit

   I don't know if Ed approves......

   Here is graphgtk_switch_init
*/

void graphgtk_switch_init(GraphDevFunc functable)
{ 
  functable->graphDevCreate = devGtkCreate;
  functable->graphDevDestroy = devGtkDestroy;
  functable->graphDevDisableKeyboard = disableKeyboardHandling ;
  functable->graphDevEnableKeyboard = enableKeyboardHandling ;
  functable->graphDevMessage = devGtkMessage;
  functable->graphDevUnMessage = devGtkUnMessage;
  functable->graphDevPop = devGtkPop;
  functable->graphDevRetitle = devGtkRetitle;
  functable->graphDevGoto = devGtkGoto;
  functable->graphDevGetScrollWinSize = devGtkScrollWinSize;
  functable->graphDevSetBaseWinSize = devGtkSetBaseWinSize ;
  functable->graphDevSetNakedWinSize = devGtkSetNakedWinSize ;
  functable->graphDevGetWindowSize = devGtkWindowSize;
  functable->graphDevInterruptCalled = devGtkInterruptCalled;
  functable->graphDevResetInterrupt = devGtkResetInterrupt;
  functable->graphDevPostBuffer = devGtkPostBuffer;
  functable->graphDevPasteBuffer = devGtkPasteBuffer;
  functable->graphDevBusyCursorAll = devGtkBusyCursorAll;
  functable->graphDevLoop = devGtkLoop;
  functable->graphDevLoopReturn = devGtkLoopReturn;
  functable->graphDevCleanup = devGtkCleanup;
  functable->graphDevMenuBar = devGtkMenuBar;
  functable->graphDevSetWinModal = devGtkSetModalWin;
}

/************ end of file *************/
