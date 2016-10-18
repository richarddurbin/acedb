/*  File: graphgdk.c
 *  Author: Simon Kelley(simon@kelley.demon.co.uk)
 *  Copyright (C) J Thierry-Mieg and R Durbin, 1991
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
 * Description: Low level stuff for graph package implemented on gdk.
 * Based on grapxlib.c, which is in turn
 * Based on code from Chris Lee.
 * HISTORY:
 * Last edited: May  8 18:33 2003 (edgrif)
 * * Jul 20 16:21 1999 (fw): devGtkRedraw avoid creating expose
 *              events for naked graphs, as that's done by widget_show
 * CVS info:   $Id: graphgdk.c,v 1.42 2003-10-08 15:26:00 srk Exp $
 *-------------------------------------------------------------------
 */

#include <gtk/gtk.h>
#include <gdk/gdk.h>
#include <gdk/gdkx.h>
#include <gdk/gdkprivate.h>
#include <wh/regular.h>
#include <wh/colours.h>
#include <wh/array.h>
#include <wh/menu_.h>
#include <wh/aceio.h>
#include <w2/graphdev.h>
#include <w2/graphgtk_.h>
#include <w2/graphcolour.h>


static GdkLineStyle dev2GdkLineStyle(GraphDevLineStyle style) ;

/******* statics for drawing *********/

static GdkGC	*gc, *gcInvert;
static GdkColor standardColorsSys[NUM_TRUECOLORS];
static GdkColor standardColorsGmap[NUM_TRUECOLORS];
static int currFontFormat ;
static int currFontHeight;

/* Grimness...as far as I can see, gdk does not allow you to change
 * the gc using a values struct and set of flags in the way the
 * underlying X Windows GC stuff does...this is _really_ poor !!**!!
 * So I'm using a static globals to store the line width/style...sigh... */
/* Actually this is only for GDK 1.2, in GDK 2.0 a set_values call is provided. */
static int currLineWidth_G ;
static GraphDevLineStyle currLineStyle_G ;

/************************************************************************/
/* grey ramp implementation - note that the globals will go into a
   per-window structure if we implement multiple ramptool capabilities */
/************************************************************************/
static void transformGreyRampImage(rampImage *im);

/* maps weight -> pixel value. fixed mapping in pseudo colour displays
   variable mapping in truecolor displays */
static gulong greyMap[256];

/* 256 grey colors, black->white, only used in true color displays */
static GdkColor greyRamp[256];

/*******************************************************/

void graphGdkGcInit (void) 
{
  GdkWindow *root_win = gdk_window_foreign_new (GDK_ROOT_WINDOW ());
  gc = gdk_gc_new(root_win);
  gdk_gc_set_line_attributes
    (gc, 0, GDK_LINE_SOLID, GDK_CAP_ROUND, GDK_JOIN_ROUND);
  
  gcInvert = gdk_gc_new(root_win);
  gdk_gc_set_function(gcInvert, GDK_INVERT);
  
}


#ifdef __CYGWIN__
static char *fontNames[4][6] =
{
  { 
    "-*-Courier New-*-*-normal-*-16-*-*-*-m-*-*-*",
    "-*-Courier New-*-*-normal-*-12-*-*-*-m-*-*-*",
    "-*-Courier New-*-*-normal-*-12-*-*-*-m-*-*-*",
    "-*-Courier New-*-*-normal-*-14-*-*-*-m-*-*-*",
    "-*-Courier New-*-*-normal-*-14-*-*-*-m-*-*-*",
    "-*-Courier New-*-*-normal-*-18-*-*-*-m-*-*-*" },
  { 
    "-*-Courier New-*-i-normal-*-16-*-*-*-m-*-*-*",
    "-*-Courier New-*-i-normal-*-12-*-*-*-m-*-*-*",
    "-*-Courier New-*-i-normal-*-12-*-*-*-m-*-*-*",
    "-*-Courier New-*-i-normal-*-14-*-*-*-m-*-*-*",
    "-*-Courier New-*-i-normal-*-14-*-*-*-m-*-*-*",
    "-*-Courier New-*-i-normal-*-18-*-*-*-m-*-*-*" },
  { 
    "-*-Courier New-bold-*-normal-*-16-*-*-*-m-*-*-*",
    "-*-Courier New-bold-*-normal-*-12-*-*-*-m-*-*-*",
    "-*-Courier New-bold-*-normal-*-12-*-*-*-m-*-*-*",
    "-*-Courier New-bold-*-normal-*-14-*-*-*-m-*-*-*",
    "-*-Courier New-bold-*-normal-*-14-*-*-*-m-*-*-*",
    "-*-Courier New-bold-*-normal-*-18-*-*-*-m-*-*-*" },
  {
    "-*-terminal-*-*-normal-*-*-*-*-*-m-78-*-* ",
    "-*-terminal-*-*-normal-*-*-*-*-*-m-45-*-* ",
    "-*-terminal-*-*-normal-*-*-*-*-*-m-50-*-* ",
    "-*-terminal-*-*-normal-*-*-*-*-*-m-55-*-* ",
    "-*-terminal-*-*-normal-*-*-*-*-*-m-65-*-* ",
    "-*-terminal-*-*-normal-*-*-*-*-*-m-90-*-* " }
};           
#else /* !CYGWIN */

static char *fontNames[4][6] =
{
  { "8x13", "5x8", "6x9", "6x10", "6x12", "9x15" },
  { "8x13", "5x8", "6x9", "6x10", "6x12", "9x15" },
  { "8x13bold", "5x8", "6x9", "6x10", "6x12", "9x15bold" },
  { "8x13", "5x8", "6x9", "6x10", "6x12", "9x15" }
};

#endif /* ! CYGWIN */  

void graphGdkFontInit (char *custom_font)
{
  if (custom_font)
    fontNames[0][0] = custom_font;
}

/* The higher level calls this to inform the bottom end where it's config
   files may be found. Note that this is optional, and must be done after
   calling gexInit, but before doing anything else, otherwise we might
   have already loaded the built-in default fonts. */

void graphGdkSetConfigFiles(char *rcFile, char *fontFile)
{
  ACEIN fi;
  
  if (rcFile)
    gtk_rc_parse(rcFile);
  
  if (fontFile && (fi = aceInCreateFromFile(fontFile, "r", "", 0))) 
    /* messerror if fails */
    {
      int i = 0, j = 0;
      char *cp;
      aceInSpecial (fi, "\n/\\") ;
      while (aceInCard(fi))
	while ((cp = aceInWord(fi)))
	  {
	    if (i < GRAPHDEV_FIXED_WIDTH)
	      fontNames[i][j] = strnew(cp, 0);
	    if (++j>=6)
	      { 
		j = 0;
		i++;
	      }
	  }
      aceInDestroy(fi);
    }
}
 
static GdkFont *getFont (int height, int format)
{
  static GdkFont *fns[4][6];
  static BOOL fontsDone = FALSE;
  
  int sizeindex, i, j;

  if (!fontsDone)
    {
      fontsDone = TRUE;
      for (i = 0 ; i <  GRAPHDEV_FIXED_WIDTH ; ++i)
	for(j = 0; j< 6; j++)
	  if (!(fns[i][j] = gdk_font_load(fontNames[i][j])))
	    {
	      if ((i == GRAPHDEV_PLAIN_FORMAT) && (j == 0))
		messcrash ("Can't load default font %s",
			   fontNames[GRAPHDEV_PLAIN_FORMAT][0]) ;
	      else
		fns[i][j] = fns[GRAPHDEV_PLAIN_FORMAT][0];
	    }
    }

  if (format == GRAPHDEV_FIXED_WIDTH)
    format = GRAPHDEV_PLAIN_FORMAT;
  
  switch (height)
    {
    case 0:			/* default font */
      sizeindex = 0;
      break ;
      
    case 1: case 2: case 3: case 4: case 5: case 6:
      sizeindex = 1;
      break;
    
    case 7: case 8:
      sizeindex = 2;
      break;
    
    case 9: case 10:
      sizeindex = 3;
      break;
    
    case 11: case 12:
      sizeindex = 4;
      break;
    
    case 13: case 14:
    default:
      sizeindex = 5;
      break;
    }

  return fns[format][sizeindex];
}

/* WARNING: this function may be called with a NULL ptr for dev, it is       */
/* currently unused but we need to allow this as the function may be called  */
/* before there are any graphs (and hence graphdevs) at the moment.          */
static BOOL devGtkFontInfo (GraphDev unused, int height, int* w, int* h)
	/* note that this depends on an integer height, and so
	   is only reliable in the long term for height 0 */
{
  GdkFont* fnt;
  
  if (!(fnt = getFont(height, 0)))
    return FALSE ;
  
  if (w) *w = gdk_string_measure(fnt, "aa") - gdk_string_measure(fnt, "a");
  if (h) *h = fnt->ascent + fnt->descent;
  
  return TRUE ;
}

static void devGtkScreenSize (float *sx, float *sy, float *fx, 
			      float *fy, int *px, int *py)
{

  int fw, fh ;  /* Font width and height */
  int width = gdk_screen_width();
  int height = gdk_screen_height();

  (void)devGtkFontInfo (NULL, 0, &fw, &fh) ;

  if (sx) *sx = (float)width/900.0 ;
  if (sy) *sy = (float)height/900.0 ;
 
  if (fx) *fx = (float)width/fw ;
  if (fy) *fy = (float)height/fh ;
  
  if (px) *px = width ;
  if (py) *py = height ;

}

/********** next color control - screen specific ***********/

static GdkColormap *insertGreyRamp (void)
{
  int i,j, ncols;
  gboolean success[256];
  GdkVisual *visual = gdk_visual_get_system();
  GdkColormap *cmap, *sys = gdk_colormap_get_system();
  
  if (visual->type == GDK_VISUAL_PSEUDO_COLOR) 
    {
      /* First try and get 128 read/write cells in the system colormap */
      ncols = gdk_colormap_alloc_colors(sys, greyRamp, 128, 
					TRUE, FALSE, success);
      if (ncols == 128)
	{
	  cmap = sys;
	  /* extract pixel values for use in translating the image */
	  for ( i=0; i<128; i++)
	    greyMap[i*2] = greyMap[i*2+1] = greyRamp[i].pixel;
	}
      else
	{
	  GdkColor col;
	  GdkColor dummies[32];

	  /* free any we did manage to allocate in the system map */
	  gdk_colormap_free_colors(sys, greyRamp, ncols);
	  
	  /* Then make our own */
	  cmap = gdk_colormap_new(visual, FALSE); 
	  
	  /* We allocate 32 cells at the start, sync them with the system 
	     colormap and then never touch them again, we hope this catches 
	     the colors used by the window manager and root window, 
	     and reduces color flashing */
	  gdk_colormap_alloc_colors(cmap, dummies, 32, TRUE, FALSE, success);
#ifndef GTK2
	  gdk_colormap_sync(sys, TRUE);
#endif
	  for (i=0; i<32; i++)
	    {
	      for (j=0; j<sys->size; j++)
		if (sys->colors[j].pixel == dummies[i].pixel)
		  {
		    col.red = sys->colors[j].red;
		    col.blue = sys->colors[j].blue;
		    col.green = sys->colors[j].green;
		    col.pixel = sys->colors[j].pixel;
		    gdk_color_change(cmap, &col);
		    break;
		  }
	    }

	  /* Now allocate the 32 standard colors in our cmap */
	  for (i = 0 ; i < NUM_TRUECOLORS ; ++i)
	    {
	      int r, g, b;
	      graphGetColourAsInt(i, &r, &g , &b, NULL);
	      standardColorsGmap[i].red = r<<8;
	      standardColorsGmap[i].green = g<<8;
	      standardColorsGmap[i].blue = b<<8;
	    }
	  gdk_colormap_alloc_colors(cmap,
				    standardColorsGmap,
				    NUM_TRUECOLORS,
				    FALSE,
				    TRUE,
				    success);
	  
	  /* Now allocate 128 writable colours for the grey ramp */
	  gdk_colormap_alloc_colors(cmap, greyRamp, 128, TRUE, FALSE, success);
	  
	  /* and finally extract pixel values for use 
	     in translating the image */
	  for ( i=0; i<128; i++)
	    greyMap[i*2] = greyMap[i*2+1] = greyRamp[i].pixel;
	  
	}
    }
  else
    { /* true color display */
      
      cmap = sys;
      
      for (i = 0; i<256; i++)
	{
	  greyRamp[i].red = i<<8;
	  greyRamp[i].green =  i<<8;
	  greyRamp[i].blue = i<<8;
	}
      
      gdk_colormap_alloc_colors(sys, greyRamp, 256, FALSE, TRUE, success);
    }

  return cmap;
}

BOOL graphGtkSetGreyMap(BOOL arg)
{
#ifndef GTK2
  static GtkStyle *greyStyle, *normalStyle;
#endif
  static GdkColormap *cmap = NULL;
  static usingPrivate = FALSE;
  BOOL old = usingPrivate;

  usingPrivate = arg;
  
  /* Note that cmap may equal the system map, in which case we do nothing after
     calling insertGreyRamp the first time arg is TRUE. */
  
  if (arg)
    {
      if (!cmap)
	{
	  /* initialise */
	  cmap = insertGreyRamp();
	  gdk_colormap_ref(gdk_colormap_get_system());
	  if  (gdk_colormap_get_system() != cmap)
	    {
	      gdk_colormap_ref(cmap);
#ifndef GTK2
	      normalStyle = gtk_widget_get_default_style();
	      greyStyle = gtk_style_copy(normalStyle);
#endif
	    }
	}
       if  (gtk_widget_get_default_colormap() != cmap)
	 {
	   gtk_widget_set_default_colormap(cmap);
#ifndef GTK2
	   gtk_widget_set_default_style(greyStyle);
#endif
	 }
    }
  else
    {
      if  (gtk_widget_get_default_colormap() != gdk_colormap_get_system())
	{
	  gtk_widget_set_default_colormap(gdk_colormap_get_system());
#ifndef GTK2
	  gtk_widget_set_default_style(normalStyle);
#endif
	}
    }
  /* The style stuff  is a workaround for a GTK bug. 
     The mechanism which allows the
     same style to be attached to widgets with different colormaps or visuals
     is broken, and results in failed assertions and accesses to freed memory.
     We work around the problem here by setting different default styles
     as we set different default colormaps.  Note that .gtkrc files could
     provoke the problem again. */

   return old;
}


/* allocate the standard acedb colors in the system colormap */
void graphGdkColorInit (void)
{ 
  gboolean success[NUM_TRUECOLORS];
  int i, num_colours ;
  
  for (i = 0 ; i < NUM_TRUECOLORS ; ++i)
    {
      int r, g, b;
      graphGetColourAsInt(i, &r, &g , &b, NULL);
      standardColorsSys[i].red = r<<8;
      standardColorsSys[i].green = g<<8;
      standardColorsSys[i].blue = b<<8;
    }

  if ((num_colours = gdk_colormap_alloc_colors(gdk_colormap_get_system(),
					       standardColorsSys,
					       NUM_TRUECOLORS,
					       FALSE,
					       TRUE,
					       success) != 0))
    messerror("Could not allocate all required colours "
	      "(got %d out of %d), "
	      "windows may be displayed in the wrongly",
	      (NUM_TRUECOLORS - num_colours), NUM_TRUECOLORS) ;

  return ;
}
  
static void devGtkSetColours(GraphDev dev, int fcol, int bcol)
{
  GdkColormap *ourMap = gtk_widget_get_colormap(dev->drawingArea); 
  if (ourMap != gdk_colormap_get_system())
    {
      gdk_gc_set_foreground (gc, &standardColorsGmap[fcol & 0x1f]) ;
      gdk_gc_set_background (gc, &standardColorsGmap[bcol & 0x1f]) ;
    }
  else
    {
      gdk_gc_set_foreground (gc, &standardColorsSys[fcol & 0x1f]) ;
      gdk_gc_set_background (gc, &standardColorsSys[bcol & 0x1f]) ;
    }
}

static void devGtkSetLineWidth(GraphDev dev, int width)
{
  currLineWidth_G = width ;
  gdk_gc_set_line_attributes(gc, currLineWidth_G, currLineStyle_G, GDK_CAP_ROUND, GDK_JOIN_ROUND);

  return ;
}

static void devGtkSetLineStyle(GraphDev dev, GraphDevLineStyle style)
{
  currLineStyle_G = dev2GdkLineStyle(style) ;
  gdk_gc_set_line_attributes(gc, currLineWidth_G, currLineStyle_G, GDK_CAP_ROUND, GDK_JOIN_ROUND) ;

  return ;
}

static void devGtkSetFont(GraphDev unused, int height)
{
  currFontHeight = height ;
}

static void devGtkSetFontFormat(GraphDev unused, GraphDevFontFormat format)
{
  currFontFormat = format ;
}

/******* now draw boxes **********/


static void devGtkSetBoxDefaults(GraphDev dev, GraphDevLineStyle style, int linewidth,
				 int textheight)
{
  currFontHeight = textheight ;   
  currFontFormat = GRAPHDEV_PLAIN_FORMAT;
  currLineWidth_G = linewidth ;
  currLineStyle_G = dev2GdkLineStyle(style) ;
  gdk_gc_set_line_attributes(gc, currLineWidth_G, currLineStyle_G, GDK_CAP_ROUND, GDK_JOIN_ROUND);

  return ;
}

static void devGtkSetBoxClip(GraphDev dev,
			     int clipx1, int clipy1, int clipx2, int clipy2)
{
  GdkRectangle clipRect[1] ;
  
  clipRect->x = clipx1 ; clipRect->y = clipy1 ;
  clipRect->width = clipx2 - clipx1 + 1 ; 
  clipRect->height = clipy2 - clipy1 + 1 ;
  
  gdk_gc_set_clip_rectangle(gc, clipRect) ;
}

static void devGtkUnsetBoxClip(GraphDev dev)
{ 
  gdk_gc_set_clip_rectangle(gc, NULL) ;
}

static void devGtkRedraw(GraphDev dev, int width, int height)
{
  GtkWidget *wid = dev->scroll  ? 
    GTK_WIDGET(dev->scroll) : GTK_WIDGET(dev->drawingArea);
  
  gtk_widget_queue_draw_area(wid, 0, 0, width, height);
}

static void devGtkWhiteOut(GraphDev dev)
{
  GdkColormap *ourMap = gtk_widget_get_colormap(dev->drawingArea); 

  if (!dev->drawingArea->window)
    return;

  if (ourMap != gdk_colormap_get_system())
    gdk_window_set_background(dev->drawingArea->window, 
			      &standardColorsGmap[WHITE]);
  else
     gdk_window_set_background(dev->drawingArea->window, 
			      &standardColorsSys[WHITE]);

  gdk_window_clear(dev->drawingArea->window);
}



static void devGtkXorLine (GraphDev dev, int x1, int y1, int x2, int y2)
{
  gdk_draw_line(dev->drawingArea->window, gcInvert, x1, y1, x2, y2) ;
}


static void devGtkXorBox(GraphDev dev, int x1, int y1, int x2, int y2)
{
  gdk_draw_rectangle (dev->drawingArea->window, gcInvert, 0, 
		      x1, y1, x2, y2);
}


static void devGtkDrawLine(GraphDev dev, int x1, int y1, int x2, int y2)
{
  gdk_draw_line (dev->drawingArea->window, gc, x1, y1, x2, y2);

}


static void devGtkDrawRectangle(GraphDev dev, GraphDevShapeStyle style,
				int x1, int y1, int width, int height)
{     
  gdk_draw_rectangle(dev->drawingArea->window, gc, 
		     style == GRAPHDEV_SHAPE_FILL ? 1 : 0,
		     x1, y1, width, height) ;
}



static void devGtkGetFontSize(GraphDev dev, int *width, int *height, int *dy)
{
  GdkFont *fnt = getFont(currFontHeight, currFontFormat);

  *width = gdk_string_measure(fnt, "aa") - gdk_string_measure(fnt, "a");
  *height =  fnt->ascent + fnt->descent;
  *dy = fnt->ascent;

}


static void devGtkDrawString(GraphDev dev, GraphDevStringDirection direction,
			     int x, int y, char *string, int length)
{
  if (direction == GRAPHDEV_STRING_LTOR)
    gdk_draw_string(dev->drawingArea->window, 
		    getFont(currFontHeight, currFontFormat),
		    gc,
		    x, y, string);
  else
    {
      int i ;
      char buf[2];
      GdkFont *fnt = getFont(currFontHeight*0.6, currFontFormat);
      
      for (i = 0 ; i < length ; ++i)
	{
	  buf[0] = string[i];
	  buf[1] = 0;
	  gdk_draw_string(dev->drawingArea->window, 
			  fnt,
			  gc, x,
			  (y-(length-i-1)*currFontHeight*0.8)
			  -(currFontHeight*0.2),
			  buf) ;
	}
      
      
    }  
}


static void devGtkDrawPolygon(GraphDev dev, GraphDevShapeStyle style,
			      int *x_vertices, int *y_vertices, 
			      int num_vertices)
{
  GdkPoint *XPts = (GdkPoint *) messalloc (num_vertices * sizeof(GdkPoint)) ;
  GdkPoint *XPt = XPts ;
  int i ;
  
  for (i = 0 ; i < num_vertices ; i++)
    {
      XPt->x = x_vertices[i] ;
      XPt->y = y_vertices[i] ;
      XPt++;
    }
  
  if (style == GRAPHDEV_SHAPE_FILL)
    /* NOTE this assumes the polygons are of the simplest
     * possible type, with no intersecting lines */
    gdk_draw_polygon(dev->drawingArea->window, gc,
		     TRUE,	/* fill */
		     XPts, num_vertices) ;
  else
    gdk_draw_lines(dev->drawingArea->window, gc, XPts, num_vertices) ;

  messfree (XPts) ;
}
 

/* n.b. I wonder if it is appreciated that different bounding sets of pixels */
/* are drawn in the below commands:                                          */
/*                                                                           */
/* XDrawRectangle  vs. XFillRectangle                                        */
/*                                                                           */
/* XDrawArc vs. XFillArc ???                                                 */
/*                                                                           */


/* Straight copy of XDrawArc parameters which are a little bizarre:          */
/*                    x,y: top left of rectangle containing the arc.         */
/*          width, height: major/minor axes of the arc.                      */
/* start_angle, end_angle: start is measured from 0 degrees (= 3 o'clock)    */
/*                         end is measured relative to start                 */
/*                         (+ve angle = counter clockwise !! & vice versa)   */
static void devGtkDrawArc(GraphDev dev, GraphDevShapeStyle style,
			  int x, int y, int width, int height, 
			  int start_angle, int end_angle)
{
  gdk_draw_arc(dev->drawingArea->window, gc, style == GRAPHDEV_SHAPE_FILL,
	       x, y, width, height, start_angle, end_angle);
}

void devGtkBeep(GraphDev dev)
{
  gdk_beep();
}

static void devGtkImage(GraphDev dev, 
			unsigned char *data, 
			int im_width, int im_height, int len,
			int xbase, int ybase,
			unsigned int *colors, int ncols)
     
{ 
  GdkRgbCmap *rgbcmap;
  
  gdk_rgb_init();
    
  rgbcmap = gdk_rgb_cmap_new(colors, ncols);
  gdk_draw_indexed_image(dev->drawingArea->window, gc, xbase, ybase, 
			 im_width, im_height, GDK_RGB_DITHER_NORMAL,
			 data, len, rgbcmap);
  gdk_rgb_cmap_free(rgbcmap);
}

/************************************************************************/
static void devGtkSetGreyRamp(GraphDev dev, unsigned char *ramp, BOOL isDrag)
{
  GdkVisual *visual = gdk_visual_get_system();
  int i;

  /* It is not possible to update the writeable entries in a colour
     map using gdk, except one at a time, using gdk_color_change, which
     is far to slow. Hence we have to got to Xlib directly. Sorry */
  
  if (visual->type == GDK_VISUAL_PSEUDO_COLOR)
#ifdef GTK2
    {  /* pseudo-color */
      static GdkColormap *cmap = NULL;
      if (!cmap)
	cmap = gdk_colormap_new(visual, FALSE);
      for(i = 0; i<128; i++)
	{
	  cmap->colors[i].pixel = greyRamp[i].pixel;
	  cmap->colors[i].red = greyRamp[i].red = ramp[i*2] << 8;
	  cmap->colors[i].green = greyRamp[i].green = ramp[i*2] << 8;
	  cmap->colors[i].blue= greyRamp[i].blue = ramp[i*2] << 8;
	}
      gdk_colormap_change(cmap, 128);
    }
#elif !defined (__CYGWIN__)
    {
      GdkColormap *cmap = gtk_widget_get_colormap(dev->drawingArea);
      /* TODO - make this work on 8-bit windows displays */
      XColor *palette = (XColor *)messalloc(sizeof(XColor)*128);
      for(i = 0; i<128; i++)
	{
	  palette[i].pixel = greyRamp[i].pixel;
	  palette[i].red = greyRamp[i].red = ramp[i*2] << 8;
	  palette[i].green = greyRamp[i].green = ramp[i*2] << 8;
	  palette[i].blue= greyRamp[i].blue = ramp[i*2] << 8;
	  palette[i].flags = DoRed | DoGreen | DoBlue;
	}
      XStoreColors(gdk_display,
		   ((GdkColormapPrivate*)cmap)->xcolormap, 
		   palette, 128);
      messfree(palette);
    }
#else /* __CYGWIN__ */
    {
    }
#endif  
  else
    { /* true-color */
      unsigned char *data;
      rampImage *im;
      GraphDev ringPtr;
      
      /* First calculate the new mapping from weight to pixels */
      for (i = 0; i<256; i++)
	greyMap[i] = greyRamp[ramp[i]].pixel;
      
      /* Now look for all graphs which have a greyramp image and
	 re-transform them and re-display them */ 

      ringPtr = dev;
      do { 
	if (ringPtr->greyRampImages)
	  { data = NULL;
	    while (assNext(ringPtr->greyRampImages, &data, &im))
	      {
		transformGreyRampImage(im);
		gdk_draw_image(im->window, im->gc, im->image,
			       0, 0,
			       im->xbase, im->ybase,
			       im->image->width, im->image->height); 
	      }
	    /* do callback if needed */
	    graphCBRampChange(ringPtr->graph, isDrag);
	    /* next one */
	  }
	ringPtr = ringPtr->ring;
      } while (ringPtr != dev);	  
    }
}

static void transformGreyRampImage(rampImage *im)
{
  /* Note1 : here we stick to client byte-order, and rely on Xlib to swap
     if the server is different */

  /* Note2: The exact function of this routine depends on the visual. If it's
     a pseudo color visual, we're just getting the pixel values for the 
     greyramp colormap entries. These values don't change, but the colors
     they represent do. If it's a true-colour visual, the map entries 
     are changed to give the correct values. It so happens that the 
     transformation here is the same in both cases, this is not true 
     for changing the grey-ramp. */

  GdkImage *gim = im->image;
  int row, col;
#if G_BYTE_ORDER == G_BIG_ENDIAN
  BOOL byterev = (gim->byte_order == GDK_LSB_FIRST);
#else
  BOOL byterev = (gim->byte_order == GDK_MSB_FIRST);
#endif
  
  switch (gim->bpp)
    { 
    case 1:
      for (row = 0; row < gim->height; row++)
	{ 
	  guint8 *ptr = ((guint8 *)gim->mem) + row*gim->bpl;
	  guint8 *sptr = ((guint8 *)im->source_data) +row*im->line_len; 
	  for (col = 0 ; col < gim->width; col++)
	    *ptr++ = (guint8) greyMap[*sptr++];
	}
	break;
    case 2:
      for (row = 0; row < gim->height; row++)
	{ 
	  guint16 *ptr = (guint16 *)(((guint8 *)(gim->mem))+row*gim->bpl);
	  guint8 *sptr = ((guint8 *)im->source_data) +row*im->line_len; 
	  if (byterev)
	    for (col = 0 ; col < gim->width; col++)
	      { 
		register guint32 pixel = greyMap[*sptr++];
		*ptr++ = ((pixel & 0xff00) >> 8) | ((pixel & 0xff) << 8);
	      }
	  else
	    for (col = 0 ; col < gim->width; col++)
	      *ptr++ = (guint16) greyMap[*sptr++];
	}
      break;
    case 3:
      for (row = 0; row < gim->height; row++)
	{ 
	  guint8 *ptr = ((guint8 *)gim->mem) + row*gim->bpl;
	  guint8 *sptr = ((guint8 *)im->source_data) +row*im->line_len; 
	  for (col = 0 ; col < gim->width; col++)
	    {
	      register guint32 pixel = greyMap[*sptr++]; 
	      *ptr++ = (guint8)pixel;
	      *ptr++ = (guint8)(pixel>>8);
	      *ptr++ = (guint8)(pixel>>16); 
	    }
	}
      break;
    case 4:
      for (row = 0; row < gim->height; row++)
	{ 
	  guint32 *ptr = (guint32 *)(((guint8 *)gim->mem) + row*gim->bpl);
	  guint8 *sptr = ((guint8 *)im->source_data) +row*im->line_len; 
	  if (byterev)
	    for (col = 0 ; col < gim->width; col++)
	      { 
		register guint32 pixel = greyMap[*sptr++];
		*ptr++ = 
		  ((pixel & 0xff000000) >> 24) |
		  ((pixel & 0xff0000) >> 8) |
		  ((pixel & 0xff00) << 8) |
		  ((pixel & 0xff) << 24);
	      }
	  else
	    for (col = 0 ; col < gim->width; col++)
	      *ptr++ = (guint32) greyMap[*sptr++];
	}
      break;
    }
}


static void devGtkGreyRampImage(GraphDev dev, 
				unsigned char *data, 
				int im_width, int im_height, int len,
				int x1, int y1, int x2, int y2,
				int xbase, int ybase)

{ int src_x = x1 - xbase;
  int src_y = y1 - ybase;
  int dest_x = x1;
  int dest_y = y1;
  int w = (x2 < x1 + im_width) ? x2-x1+1 : x2-x1;
  int h = (y2 < y1 + im_height) ? y2-y1+1 : y2-y1;

  /* We may need to re-draw this asyncronously if the greyRamp changes, 
     so keep everything needed to do so here */

  rampImage *ourImage;
  GdkWindow *window = dev->drawingArea->window;

  if (!dev->greyRampImages)
      dev->greyRampImages = assCreate();

  if (!assFind(dev->greyRampImages, data, &ourImage))
    {
      GdkImage *im = gdk_image_new(GDK_IMAGE_NORMAL,
				   gdk_visual_get_system(),
				   im_width-1, im_height-1 );
      if (!im) return;

      ourImage = (rampImage *)messalloc(sizeof(rampImage));
      assInsert(dev->greyRampImages, data, ourImage);

      ourImage->window = window;
      ourImage->gc = gc;
      ourImage->xbase= xbase;
      ourImage->ybase = ybase;
      
      ourImage->line_len = len;
      ourImage->source_data = data;
      ourImage->image = im;

      transformGreyRampImage(ourImage);
    }
  
  gdk_draw_image(window, gc, ourImage->image, src_x, src_y, 
		 dest_x, dest_y, w, h);
}

static void devGtkClear(GraphDev dev)
{ 
  /* free everything associated with greyRamp Images */
  if (dev->greyRampImages)
    { 
      unsigned char *data = 0;
      rampImage *im;
      while (assNext (dev->greyRampImages, &data, &im))
	{ 
	  gdk_image_destroy(im->image);
	  messfree(im);
	}
      assDestroy(dev->greyRampImages);
    }
  
  dev->greyRampImages = NULL;

}

static GdkLineStyle dev2GdkLineStyle(GraphDevLineStyle style)
{
  GdkLineStyle gdk_style ;

  switch (style)
    {
    case GRAPHDEV_LINE_SOLID :   gdk_style = GDK_LINE_SOLID ; break ;
    case GRAPHDEV_LINE_DASHED :  gdk_style = GDK_LINE_ON_OFF_DASH ; break ;
    default : messcrash("internal error, illegal graphDev line style: %d", style) ;
    }

  return gdk_style ;
}

/***************************************************************************/

/* This initialises the function switch tables which are our interface to the
   device independent stuff. My stategy is this:
   Each function defined in a file gets stuffed into the function switch by
   a routine at the end of that file, called <file_name>_switch_init
   That way I don't have to declare that damn things in a header as well as 
   in the definition of the switches, in fact they can be declared static!

   The switch_init functions are called from graphDevInit

   I don't know if Ed approves......

   Here is graphgdk_switch_init
*/

void graphgdk_switch_init(GraphDevFunc functable)
{ 
  functable->graphDevFontInfo =  devGtkFontInfo;
  functable->graphDevSetColours = devGtkSetColours;
  functable->graphDevSetLineWidth = devGtkSetLineWidth;
  functable->graphDevSetLineStyle = devGtkSetLineStyle;
  functable->graphDevSetFont =  devGtkSetFont;
  functable->graphDevSetFontFormat =  devGtkSetFontFormat;
  functable->graphDevSetBoxDefaults = devGtkSetBoxDefaults;
  functable->graphDevSetBoxClip = devGtkSetBoxClip;
  functable->graphDevUnsetBoxClip =  devGtkUnsetBoxClip;
  functable->graphDevRedraw = devGtkRedraw;
  functable->graphDevWhiteOut = devGtkWhiteOut;
  functable->graphDevXorLine = devGtkXorLine;
  functable->graphDevXorBox = devGtkXorBox;
  functable->graphDevDrawLine = devGtkDrawLine;
  functable->graphDevDrawRectangle = devGtkDrawRectangle;
  functable->graphDevGetFontSize = devGtkGetFontSize;
  functable->graphDevDrawString = devGtkDrawString;
  functable->graphDevDrawPolygon = devGtkDrawPolygon;
  functable->graphDevDrawArc = devGtkDrawArc;
  functable->graphDevBeep = devGtkBeep;
  functable->graphDevSetGreyRamp = devGtkSetGreyRamp;
  functable->graphDevDrawRampImage = devGtkGreyRampImage;
  functable->graphDevScreenSize = devGtkScreenSize;
  functable->graphDevDrawImage = devGtkImage;
  functable->graphDevClear = devGtkClear;
  functable->graphDevSetGreyMap = graphGtkSetGreyMap;
}


/************* end of file ****************/
