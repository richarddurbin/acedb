/*  File: gexramptool.c
 *  Author: Richard Durbin (rd@mrc-lmb.cam.ac.uk)
 *  Copyright (C) J Thierry-Mieg and R Durbin, 1991
 * this file written by F Wobus, with contributions from
 * R Durbin and E Sonnhammer.
 * Heavily re-written for the gtk port by S Kelley.
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
 * Description: greyRampTool for easy real-time manipulation
                on the colors used in graphPixel-images
 * Exported functions:  gexRampTool

 * HISTORY:
 * Last edited: May  6 10:15 2003 (edgrif)
 * * Jun  7 15:33 1996 (fw)
 *-------------------------------------------------------------------
 */


#include "regular.h"

#include "menu_.h"

#include <w2/graphdev.h>
#include <w2/graph_.h>
#include <gtk/gtk.h>
#include "gex.h"

static int rampMin = 0, rampMax = 255; 
static Graph rampGraph = 0 ;

#define MAXY 20
#define MINY 140
#define BORDER 20
#define SIZEX(x) ((int)(x))
#define SIZEY(x) ((int)(x))
#define UNSIZEX(x) ((int)(x))
#define UNSIZEY(x) ((int)(x))
#define CORRX(x) (((x) % 4 == 0) ? (x) : (((int)((x)/4))+1)*4 )

static int defMin, defMax, oldx ;
static int sliderBox;
static int rampBox;
static int maxBox, minBox, thresBox ;
static UCHAR *wedge;
static float CWINX = 350 ;
static float CWINY = 2*BORDER+(MINY-MAXY) ;
static BOOL isDragging;

static GtkWidget *topSpin, *bottomSpin;

static void colorPickSlider (int, double x_unused, double y_unused, int modifier_unused) ;
static void colorDragSlider (double,double) ;
static void colorUpSlider (double,double) ;
static void colorDragThres (double,double) ;
static gint colorUndo (GtkWidget *widg, gpointer data) ;
static gint bottomSpinChange(GtkWidget *widg, gpointer data);
static gint topSpinChange(GtkWidget *widg, gpointer data);
static gint colorSwap (GtkWidget *widg, gpointer data) ;
static void updateGreyMap (void);

void gexRampSet(int min, int max)
{
  rampMin = min;
  rampMax = max;

  /* have to work if there's a rampTool in existance or no */
  if (graphExists(rampGraph))
    {
      gtk_spin_button_set_value(GTK_SPIN_BUTTON(topSpin), (float)rampMax);
      gtk_spin_button_set_value(GTK_SPIN_BUTTON(bottomSpin), (float)rampMin);
    }
  else
    updateGreyMap();
}

int gexRampGetMin(void)
{ 
  return rampMin;
}

int gexRampGetMax(void)
{
  return rampMax;
}

void gexRampPop()
{
  if (graphActivate (rampGraph))
    graphPop();
}

void gexRampToolSig (GtkWidget *widget, gpointer user_data)
{
  gexRampTool();
}

void gexRampTool (void)
{
  int i,j;
  GtkWidget *vbox;
  GtkWidget *quitButton, *undoButton, *swapButton;
  GtkObject *topAdj, *bottomAdj;
  BOOL mapSave;
  
  if (graphActivate (rampGraph)) /* checks graphExists() first */
    { 
      graphPop () ;
      return;
    }

  mapSave = graphSetInstallMap(TRUE);

  rampGraph = graphCreate (PIXEL_FIT, "Greyramp Tool", 0, 0,
			   CWINX/900.0, CWINY/900.0) ;
  
  graphActivate (rampGraph) ;
  
  /* Note that the initial values of the adjustments must be anything _but_
     the current values of rampMin and rampMax so that the value changed signal
     gets emitted further down, to sync the display with the current value */
  
  topAdj = gtk_adjustment_new((float) rampMax+1, 0.0, 255.0, 1.0, 1.0, 1.0);
  topSpin = gtk_spin_button_new((GtkAdjustment *)topAdj, 0.5, 0);
  bottomAdj = gtk_adjustment_new((float)rampMin+1, 0.0, 255.0, 1.0, 1.0, 1.0);
  bottomSpin = gtk_spin_button_new((GtkAdjustment *)bottomAdj, 0.5, 0);

  quitButton = gtk_button_new_with_label("Close");
  swapButton = gtk_button_new_with_label("Swap");
  undoButton = gtk_button_new_with_label("Undo");

  vbox = gtk_vbox_new(TRUE, 0);
  gtk_container_border_width(GTK_CONTAINER(vbox), 5);
  gtk_box_pack_start(GTK_BOX(gexGraphHbox(graphActive())), 
		     vbox, FALSE, FALSE, 0);

  gtk_box_pack_start(GTK_BOX(vbox), topSpin, TRUE, FALSE, 5);
  gtk_box_pack_start(GTK_BOX(vbox), quitButton, TRUE, FALSE, 5);
  gtk_box_pack_start(GTK_BOX(vbox), swapButton, TRUE, FALSE, 5);
  gtk_box_pack_start(GTK_BOX(vbox), undoButton, TRUE, FALSE, 5);
  gtk_box_pack_start(GTK_BOX(vbox), bottomSpin, TRUE, FALSE, 5);

  gtk_widget_show_all(vbox);
  
  gtk_signal_connect_object(GTK_OBJECT(quitButton), "pressed",
			    GTK_SIGNAL_FUNC(gtk_widget_destroy),
			    GTK_OBJECT(gexGraph2Widget(graphActive()))); 
  gexSignalConnect(graphActive(),
		   GTK_OBJECT(undoButton), "pressed",
		   (GtkSignalFunc)colorUndo, NULL);
  gexSignalConnect(graphActive(),
		   GTK_OBJECT(swapButton), "pressed",
		   (GtkSignalFunc)colorSwap, NULL);
  gexSignalConnect(graphActive(),
		   GTK_OBJECT(topAdj), "value_changed",
		   GTK_SIGNAL_FUNC(topSpinChange), NULL);
  gexSignalConnect(graphActive(),
		   GTK_OBJECT(bottomAdj), "value_changed",
		   GTK_SIGNAL_FUNC(bottomSpinChange), NULL);
  
  graphRegister (PICK,		colorPickSlider) ;

  wedge = (UCHAR *) halloc
    (CORRX(SIZEX(256)) * SIZEY((MINY-MAXY-20)) * sizeof(UCHAR),
     graphHandle()) ;
  
  for (j = 0 ; j < SIZEY(MINY-MAXY-20) ; j++)
    for (i = 0 ; i < CORRX(SIZEX(256)) ; i++)
      wedge[j*CORRX(SIZEX(256))+i] = UNSIZEX(i) ; 

  rampBox = graphBoxStart();
  graphPixelsRaw (wedge, SIZEX(256), SIZEY(MINY-MAXY-20),
		  CORRX(SIZEX(256)), SIZEX(BORDER), SIZEY(MAXY+10)) ;
  
  graphBoxEnd();

  graphColor (BLACK);

  graphRectangle (SIZEX(BORDER-9), SIZEY(MAXY-10),
		  SIZEX(BORDER+255+9), SIZEY(MINY+10));
  
  maxBox = graphBoxStart () ;
  graphLine (SIZEX(BORDER-5), SIZEY(MAXY-5),
	     SIZEX(BORDER), SIZEY(MAXY+5));
  graphLine (SIZEX(BORDER), SIZEY(MAXY+5),
	     SIZEX(BORDER+5), SIZEY(MAXY-5));
  graphLine (SIZEX(BORDER-5), SIZEY(MAXY-5),
	     SIZEX(BORDER+5), SIZEY(MAXY-5));
  graphBoxEnd ();

  minBox = graphBoxStart () ;
  graphLine (SIZEX(BORDER-5), SIZEY(MINY+5),
	     SIZEX(BORDER), SIZEY(MINY-5));
  graphLine (SIZEX(BORDER), SIZEY(MINY-5),
	     SIZEX(BORDER+5), SIZEY(MINY+5));
  graphLine (SIZEX(BORDER-5), SIZEY(MINY+5),
	     SIZEX(BORDER+5), SIZEY(MINY+5));
  graphBoxEnd ();
  
  thresBox = graphBoxStart () ;
  graphRectangle (SIZEX(BORDER-5),
		  SIZEY(((MINY-MAXY)/2+MAXY)-5),
		  SIZEX(BORDER+5),
		  SIZEY(((MINY-MAXY)/2+MAXY)+5) ) ;
  graphBoxEnd () ;
  graphBoxDraw (thresBox, RED, TRANSPARENT) ;
  
  isDragging = FALSE;

  gtk_spin_button_set_value(GTK_SPIN_BUTTON(topSpin), (float)rampMax);
  gtk_spin_button_set_value(GTK_SPIN_BUTTON(bottomSpin), (float)rampMin);
  
  defMin =  0; defMax =  255;

  graphRedraw ();
  
  graphSetInstallMap(mapSave); 
}

static void colorPickSlider (int box, double x_unused, double y_unused, int modifier_unused)
{
  if (box == maxBox || box == minBox || box == thresBox)
    {
      isDragging = TRUE;
      defMin =  rampMin ; defMax =  rampMax ;
      sliderBox = box ;		/* for colorUpSlider */
      graphRegister (LEFT_DRAG, 
		     box == thresBox ? colorDragThres : colorDragSlider) ;
      graphRegister (LEFT_UP,   colorUpSlider) ;
      graphBoxDraw (sliderBox,GREEN,TRANSPARENT);
    }
}

static void colorDragSlider (double x,double y)
{
  x = UNSIZEX(x) ;  y = UNSIZEY(y) ;

  if (x < BORDER) x = BORDER; 
  if (x > BORDER+255) x = BORDER+255;

  if (sliderBox == maxBox)
    {
      rampMax= x-BORDER ;
      if (rampMin == rampMax)
	{ if (x<oldx)
	    x--;
	else  
	  x++;
	  if (x < BORDER) x = BORDER+1; 
	  if (x > BORDER+255) x = BORDER+255-1;
	  rampMax = x-BORDER ;
	}
      gtk_spin_button_set_value(GTK_SPIN_BUTTON(topSpin), (float)rampMax);
    }
  else
    {
      rampMin = x-BORDER ;
      if (rampMin == rampMax)
	{ 
	  if (x < oldx)
	    x-- ;
	  else  
	    x++ ;
	  if (x < BORDER) x = BORDER+1; 
	  if (x > BORDER+255) x = BORDER+255-1;
	  rampMin = x-BORDER ;
	}
      gtk_spin_button_set_value(GTK_SPIN_BUTTON(bottomSpin), (float)rampMin) ;
    }

  oldx = x; 
}

static void colorUpSlider (double x,double y)
{
  isDragging = FALSE;
  updateGreyMap(); /* one last time with isDragging == FALSE */

  if (sliderBox != thresBox)
    graphBoxDraw (sliderBox,BLACK,TRANSPARENT) ;
  
  graphBoxDraw (thresBox,RED,TRANSPARENT) ;
  
  graphRegister (LEFT_DRAG, 0) ;
  graphRegister (LEFT_UP,   0) ;
}

static void colorDragThres (double x,double y)
{
  int d = abs(rampMin-rampMax)/2 ;

  x = UNSIZEX(x) ;  y = UNSIZEY(y) ;
  x -= BORDER ;
  if (d > x) x = d ;
  if (255-d < x) x = 255 - d ;

  if (rampMin < rampMax)
    { rampMin = x-d ; rampMax = x+d ;
      if (rampMin == rampMax) rampMax = rampMin+1 ;
    }
 else
    { rampMin = x+d ; rampMax = x-d ;
      if (rampMin == rampMax) rampMin = rampMax+1 ;
    } 
  if (rampMin == 256 || rampMax == 256) 
    { --rampMin ; --rampMax ;}

  gtk_spin_button_set_value(GTK_SPIN_BUTTON(bottomSpin), (float)rampMin) ;
  gtk_spin_button_set_value(GTK_SPIN_BUTTON(topSpin), (float)rampMax);

}


static gint colorUndo (GtkWidget *widg, gpointer data) /* restore old values */
{
  int m ;
  m = rampMin; rampMin = defMin; defMin = m;
  m = rampMax; rampMax = defMax; defMax = m;
  gtk_spin_button_set_value(GTK_SPIN_BUTTON(bottomSpin), (float)rampMin) ;
  gtk_spin_button_set_value(GTK_SPIN_BUTTON(topSpin), (float)rampMax); 
  return TRUE;
}

static gint colorSwap (GtkWidget *widg, gpointer data)
{
  int mem;
  mem = rampMin; rampMin = rampMax; rampMax = mem;
  gtk_spin_button_set_value(GTK_SPIN_BUTTON(bottomSpin), (float)rampMin) ;
  gtk_spin_button_set_value(GTK_SPIN_BUTTON(topSpin), (float)rampMax);
  return TRUE;
}

static gint bottomSpinChange(GtkWidget *widg, gpointer data)
{ 
  rampMin = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(bottomSpin));
  graphBoxShift (minBox, SIZEX(rampMin+BORDER-5), SIZEY(MINY-5)) ;
  updateGreyMap ();
  graphBoxShift (thresBox,
		 SIZEX(BORDER+((rampMin+rampMax)/2)-5),
		 SIZEY(((MINY-MAXY)/2+MAXY)-5) ); 
  return TRUE;
}

static gint topSpinChange(GtkWidget *widg, gpointer data)
{ 
  rampMax = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(topSpin));
  graphBoxShift (maxBox, SIZEX(rampMax+BORDER-5), SIZEY(MAXY-5)) ;
  updateGreyMap ();
  graphBoxShift (thresBox,
		 SIZEX(BORDER+((rampMin+rampMax)/2)-5),
		 SIZEY(((MINY-MAXY)/2+MAXY)-5) ); 
  return TRUE;
}

static void updateGreyMap (void)
{
  int i ;
  float fac ;
  int min, max;
  unsigned char *ramp = messalloc(256 * sizeof(unsigned char));
  
  min = rampMin <= 0 ? 0 : rampMin ;
  max = rampMax >=255 ? 255 : rampMax ;
  if (min == max)
    max = min+1 ;

  if (min < max)
    {
      for (i = 0 ; i < min ; ++i)
	ramp[i] = 0x0 ;
      fac = 0xff / (float)(max-min) ;
      for ( ; i < max && i < 256 ; ++i)
	ramp[i] =  fac * (i - min) ; 
      for ( ; i < 256 ; ++i)
	ramp[i] = 0xff ;
    }
  else
    { 
      for (i = 0 ; i < max ; ++i)
	ramp[i] = 0xff ;
      fac = 0xff / (float)(min-max) ;
      for ( ; i < min && i < 256 ; ++i)
	ramp[i] = fac * (min - i) ; 
      for ( ; i < 256 ; ++i)
	ramp[i] = 0x0 ;
    }
  
  graphSetGreyMap(ramp, isDragging);
  
  messfree(ramp);
}
/******************/
/******************/
 












