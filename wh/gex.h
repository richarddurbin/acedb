/*  File: gex.c
 *  Author: Simon Kelley (srk@sanger.ac.uk)
 *  Copyright (C) J Thierry-Mieg and R Durbin, 1991
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
 *              Functions for the integration of GUI code using the
 *              graph package with GUI code using Gtk
 *		Depends on the Gtk back-end for the graph package
 *              
 * HISTORY:
 * Last edited: Jun 19 14:18 2008 (edgrif)
 * * Jul 20 16:00 1999 (fw): added signal call backs for gexRamptool
 *              and helpOn
 *------------------------------------------------------------------- */
#ifndef GEXH_INCLUDED

#include <gdk/gdk.h>
#include <gtk/gtk.h>
#include <wh/graph.h>

/**********************************************************************/
/******* be nice to C++ - patch from Steven Ness **********************/
/**********************************************************************/

#ifdef  __cplusplus
extern "C" {
#endif


/* stuff for gexTextEditorNew */
typedef void (*CBRoutine)(void *editor, void *arg1);

typedef struct
{
  GtkWidget *window;
  GtkWidget *view;
  GtkTextBuffer *text ;
  GtkWidget *ok, *cancel;
  void *data;
  struct PrintParams *printParams;
  CBRoutine okCB, cancelCB;
} GexEditorStruct, *GexEditor ;



/* return the packing boxes which are always put in standard graph Windows */
GtkWidget *gexGraphHbox(Graph graph);
GtkWidget *gexGraphVbox(Graph graph);

/* Like gtk_signal_connect, but ensures that the active graph is changed
   before the handler gets control */
void gexSignalConnect(Graph graph, GtkObject *widget, gchar *signal,
		      GtkSignalFunc func, gpointer data);

/* return the highest level widget corresponding to a graph. */
GtkWidget *gexGraph2Widget(Graph g);

/* return the widget that actually corresponds to the graphs window. */
GtkWidget *gexGraph2GraphWidget(Graph g);

/* create a top  level gtk window, like gtk_window_new, but registers
   for cursor changes abd clean-up */
GtkWidget *gexWindowCreate(void);

/* Simple call-back register, replaces graphRegister(DESTROY)
   for simle cases where people are allergic to the gtk signal system */
void gexDestroyRegister(GtkWidget *window, void (*func)(GtkWidget *));

/* like graphCleanup, save is analogous to active graph */
void gexCleanup(GtkWidget *save);

/* The grey-ramp stuff. creates ramptool applet, or pops it if already 
 * existing */
void gexRampTool();
/* Pops an existing applet, no-op if one doesn't exist */
void gexRampPop();

/* signal call back func, just to simplify menu/button setup code
 * user_data is ignored, it just calls gexRampTool() */
void gexRampToolSig (GtkWidget *widget, gpointer user_data);

/* Set the ramp and return the ramp parmeters */
void gexRampSet(int min, int max);
int gexRampGetMin(void);
int gexRampGetMax(void);

/* Call _after_ graphInit */
void gexInit (int *argcptr, char **argv);

/* Optionally call after gexInit */
void gexSetConfigFiles(char *rcFile, char *fontFile);

/* Set/reset messubs callbacks to gex defined routines.             */
void gexSetMessCB(void) ;				    /* Set all messNNN() callbacks to gex
							       defaults. */

/* Controls the messaging and switching between popups and the scrolled    */
/* list window.                                                            */
typedef void (*MessListCB)(BOOL use_msg_list) ;		    /* Callback to allow application to
							       get control when user clicks on
							       popup button to switch to message list. */
void gexSetMessPrefs(BOOL use_msg_list, int msg_list_length, MessListCB) ;
							    /* Set messout/error to popup or
							       message list window. */
void gexSetMessPopUps(BOOL use_msg_list, int msg_list_length) ;
							    /* Switch between popups and a message */
							    /* list window for messout/error(). */
void gexSetMessListSize(int list_length) ;		    /* Set length of message list. */
int gexGetDefListSize(int curr_value)   ;		    /* Get default message list size. */
BOOL gexCheckListSize(int curr_value) ; 		    /* Check the list size. */
void gexSetModalDialog(BOOL modal) ;			    /* toggle modality for all windows created. */


/* If (win != NULL) it becomes a modal window */
int gexMainLoop(GtkWidget *win);
void gexLoopReturn(int retval);
/* version of gexLoopReturn which can be called from a Gtk signal */
gint gexLoopReturnSig(GtkWidget *w, gpointer retval);
/* version of gexLoopReturn which can be called from a Gtk event handler */
gint gexLoopReturnEvent(GtkWidget *w, GdkEventAny *event, gpointer data);

/* set or remove  modality */
void gexSetModalWin(GtkWidget *win, BOOL modal);


/* text editor window */

char *gexTextEditor(char *title, char *initText, STORE_HANDLE handle); /* blocking version */

GexEditor gexTextEditorNew(char *title, char *initText, STORE_HANDLE handle,
			   void *data, 
			   void (*okCB)(void *, void *), 
			   void (*cancelCB)(void *, void *),
			   BOOL editable, BOOL word_wrap, BOOL fixed_font) ; /* non-blocking kit-of-parts */

char *gexEditorGetText(GexEditor editor, STORE_HANDLE handle) ; /* use messfree to free text. */


/* signal call back for menuitems/buttons linked to help pages */
void gexHelpOnSig (GtkWidget *widget, gpointer subject);

BOOL gexHtmlViewer (char* helpFilename, void *user_pointer);
/* function for helpContextStruct passed to helpOnRegister */

/* Put up a splash screen. Will remove any previous splash screen
   Called with NULL simply removes any existing splash screen.

   Note that any graphPackage or Gex call which creates as window
   will kill the splash screen. This is important to avoid obscuring
   dialog boxes, and is assumed by the main() code which doesn't 
   explicitly remove the splash screen. */

void gexSplash(char **splash_xpm);

/* Set the default icon for windows created by gex or graph */
void gexSetIconFromXPM(char *xpm);

/* return X window-id */
unsigned long gexGetWindowId(Graph graph);

#ifdef  __cplusplus
}
#endif

#define GEXH_INCLUDED
#endif









