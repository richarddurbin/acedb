/*  File: genebuilder.c
 *  Author: Gemma Barson <gb10@sanger.ac.uk>
 *  Copyright (C) 2012 Genome Research Ltd
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
 * 	Richard Durbin (Sanger Institute, UK) rd@sanger.ac.uk, and
 *	Jean Thierry-Mieg (CRBM du CNRS, France) mieg@kaa.crbm.cnrs-mop.fr
 *
 * Description: Functions for starting a ZMap child process
 *-------------------------------------------------------------------
 */

/************************************************************/
/* Includes and defines */

#include <wh/zmap.h>
#include <wh/smap.h>
#include <wzmap/zmaputils.h>
#include <gtk/gtk.h>


#define DEFAULT_WINDOW_WIDTH_FRACTION	 0.3  /* what fraction of the screen size the window width defaults to */
#define DEFAULT_WINDOW_HEIGHT_FRACTION	 0.3  /* what fraction of the screen size the window height defaults to */
#define DEFAULT_WINDOW_BORDER_WIDTH      1    /* used to change the default border width around the window */


/************************************************************/
/* Local functions */

/* Set various properties for the window */
static void setStyleProperties(GtkWidget *widget)
{
  /* Set the initial window size based on some fraction of the screen size */
  GdkScreen *screen = gtk_widget_get_screen(widget);
  const int width = gdk_screen_get_width(screen) * DEFAULT_WINDOW_WIDTH_FRACTION;
  const int height = gdk_screen_get_height(screen) * DEFAULT_WINDOW_HEIGHT_FRACTION;
  
  gtk_window_set_default_size(GTK_WINDOW(widget), width, height);
  
  gtk_container_set_border_width (GTK_CONTAINER(widget), DEFAULT_WINDOW_BORDER_WIDTH); 
  gtk_window_set_mnemonic_modifier(GTK_WINDOW(widget), GDK_MOD1_MASK); /* MOD1 is ALT on most systems */
}


static BOOL createGenebuilder(const char *seq_name,
                              const char *parent_name,
                              const int start,
                              const int stop)
{
  GtkWidget *window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
  gtk_window_set_title(GTK_WINDOW(window), seq_name);
  setStyleProperties(window);
  
  GtkWidget *vbox = gtk_vbox_new(FALSE, 0);
  
  gtk_container_add(GTK_CONTAINER(window), vbox);
  
  char *text = g_strdup_printf("Coords: %d-%d", start, stop);
  GtkWidget *label = gtk_label_new(text);
  g_free(text);
  
  gtk_container_add(GTK_CONTAINER(vbox), label);

  
  gtk_widget_show_all(window);
  gtk_window_present(GTK_WINDOW(window));

  return TRUE;
}


static BOOL openGenebuilder(KEY keyIn, KEY from, BOOL isOldGraph)
{
  KEY seq = from ? from : keyIn;
  //  OBJ obj = bsCreate(key);
  //bsDestroy(obj);
  
  int start = 1;
  int stop = 0;
  KEY key;
  BOOL extend = TRUE;

  setStartStop(seq, &start, &stop, &key, extend, NULL);

  char *seq_name = name(seq);
  char *parent_name = name(key);

return createGenebuilder(seq_name, parent_name, start, stop);
}


/************************************************************/
/* Public functions */

BOOL genebuilderDisplay(KEY keyIn, KEY from, BOOL isOldGraph, void *unused)
{
  return openGenebuilder(keyIn, from, isOldGraph) ;
}
