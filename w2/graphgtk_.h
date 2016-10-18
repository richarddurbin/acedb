/*  File: graphgtk_.h
 *  Author: Simon Kelley(simon.kelley@bigfoot.com)
 *  Copyright (c) J Thierry-Mieg and R Durbin, 1999
 *  Copyright (c) S Kelley, 1998
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
 * Description: Private header for the gtk
 *              implementation of the device dependent layer of the
 *              graph package (this is a replacement for graphxt_.h)
 *
 * HISTORY:
 * Last edited: Apr 23 14:15 2003 (edgrif)
 *-------------------------------------------------------------------
 */
#ifndef DEF_GRAPHGTK_PRIV_H
#define DEF_GRAPHGTK_PRIV_H

/* NOTE: this struct is used to keep track of three types of thing.
   1) Graph windows - topWindow!=NULL && graph!=NULL
   2) Naked graphs - topWindow==NULL && graph!=NULL
   3) gtk windows created via gex - topWindow!=NULL && graph==NULL
*/

typedef struct _GraphDevStruct
{
  GraphDevGraphType type ;				    /* What type are we ? */
  GraphPtr  graph;                /* pointer back up to devind struct */
  GraphDev  ring;                 /* all the graphs linked together */
  GtkWidget *topWindow ;	  /* outer parent */
  GtkWidget *vbox ;               /* allows graph package clients
				     to add chrome top and bottom */
  GtkWidget *hbox ;
  BOOL hasChrome ;                /* Set if vbox or hbox used, 
				     toggles handling of tab key between
				     graph package comptability  mode
				     and GTK mode (switches active widget) */
  GtkWidget *scroll;
  GtkWidget *drawingArea ;
  GtkWidget *message;            
  Associator greyRampImages ;
  GtkWidget *menuBar;
  BOOL doingExposure;
  BOOL keyboard_handling ;				    /* FALSE means don't intercept main
							       keyboard events, e.g. arrow keys. */
  int xmin, ymin, xmax, ymax ;    /* area to be re-drawn on exposure */
  int width_fudge, height_fudge ; /* to compensate for scrollbars */
  void (*destroyFunc)(GtkWidget *); /* used for gexDestroyFunc */
  BOOL hasGreyCmap;               /* Possibly has installed cmap */
} GraphDevStruct ;


typedef struct {
  GdkWindow *window;
  GdkGC *gc;
  int xbase, ybase;
  int line_len;
  GdkImage *image;
  unsigned char *source_data;
} rampImage;


/* Functions used to populate the function switch which is graph's interface
   to the bottom end: one for each file: called from gexInit */
void graphgdk_switch_init(GraphDevFunc functable);
void graphgdkremote_switch_init(GraphDevFunc functable);
void graphgtk_switch_init(GraphDevFunc functable);

/* These functions are exported from the bottem end to gex _only_).
   They (and the definition of GraphDevStruct) constitute the interface
   between the two */

void graphGdkGcInit (void) ;
void graphGdkFontInit(char *font);
void graphGdkRemoteInit(void);
void graphGtkInit (void);
void graphGdkColorInit (void);
void graphGtkLoopReturn(int retval);
void graphGtkSetModalWin(GtkWidget *win, BOOL modal);
int  graphGtkMainLoop(GtkWidget *win);
void graphGtkWinInsert(GraphDev dev);
void graphGtkWinRemove(GraphDev dev);
void graphGtkWinCleanup(GtkWidget *save);
void graphGtkOnModalWinDestroy(GtkWidget *win);
BOOL graphGtkSetGreyMap(BOOL arg);
void graphGdkSetConfigFiles(char *rcFile, char *fontFile);
void graphGtkSetSplash(GtkWidget *splash);
void graphGtkSetIcon(char *icon);

/* cut/paste routines for use by gex and gtk code, these only implement the "cut",
 * another call will need to be added for the "paste". */
void graphGtkSetSelectionOwner(GtkWidget *widget, char *text) ;
void graphGtkSelectionGet(GtkWidget *widget, GtkSelectionData *selection_data,
			  guint info, guint time, gpointer data) ;


#endif
/****** end of file ******/
 

