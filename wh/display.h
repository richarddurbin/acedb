/*  File: display.h
 *  Author: Ed Griffiths (edgrif@sanger.ac.uk)
 *  Copyright (C) J Thierry-Mieg and R Durbin, 1998
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
 * 	Richard Durbin (Sanger Centre, UK) rd@sanger.ac.uk, and
 *	Jean Thierry-Mieg (CRBM du CNRS, France) mieg@kaa.crbm.cnrs-mop.fr
 *
 * Description: header for display.c, 
 *              a module that drives acedb windowing displays
 * Exported functions:
 * HISTORY:
 * Last edited: May  9 15:48 2008 (edgrif)
 * * Mar  1 16:17 1999 (fw): removed prototypes for keyset display
 *              (now in keysetdisp.h)
 * * Sep 17 16:34 1998 (edgrif): Removed displayCreate(int d) definition,
 *              should be redundant now.
 * Created: Thu Sep 17 16:34:52 1998 (edgrif)
 * CVS info:   $Id: display.h,v 1.38 2012-04-12 15:21:38 edgrif Exp $
 *-------------------------------------------------------------------
 */
#ifndef ACEDB_DISPLAY_H
#define ACEDB_DISPLAY_H

#include <wh/acedb.h>
#include <wh/graph.h>
#include <wh/menu.h>		/* new libfree menu package */
#include <wh/key.h>		/* keyboard-codes */

/************************************/

extern BOOL isGifDisplay ;				    /* used to unset useless buttons etc */


/* parses displays.wrm, re-reads options.wrm after displayKeys are set up,
 * hooks up types displayFuncs */
void    displayInit(void) ;

float displayGetOverlapThreshold(void) ;


/* display is the original call, displayApp is a new call which allows the   */
/* passing of display data through from the caller of displayApp down to the */
/* eventual display routine called by displayApp(). Look in the various      */
/* display headers (pepdisp.h, fmap.h etc.) to see what to pass as           */
/* display_data.                                                             */
BOOL	display(KEY key, KEY from, char *displayName) ;
BOOL    displayApp(KEY key, KEY from, char *displayName, void *display_data) ;

Graph   displayCreate(char *displayName) ;

void	displayPreserve (void) ;

BOOL	displayBlock (BlockFunc func, char * message) ;	/* call func on next picked key */
void    displayUnBlock (void) ;

void    displayRepeatBlock (void) ;

BOOL	isDisplayBlocked (void) ;

/* Get/set graph for current (unpreserved) display type. */
Graph   displayGetGraph(char *displayname) ;
void    displaySetGraph(char *displayname, Graph graph) ;



/* the menu with show methods 'Text' 'Graph' 'WWW' etc.. */
FREEOPT* displayGetShowTypesMenu(void);

char*   displayGetTitle(char *displayName);

/* set window size default for next displayCreate 
   for every display type - dimensions in pixels */
void    displaySetSizes (int wpixels, int hpixels);

/* set window size default for next displayCreate on the given type
 * will force creation a new window for that type 
 * to avoid use of old window of old size - dimensions in pixels */
void    displaySetSize (char *displayName, int wpixels, int hpixels);

/* register current window size for next display create */
void    displaySetSizeFromWindow (char *displayName);


/* only returns true if the given name is a valid display type
 * for generic object displays, i.e. a display with registered function */
BOOL    displayIsObjectGeneric(char *displayName, KEY *keyp);

/* is true if the given name is any valid display name */
BOOL    displayIsValid(char *displayName, KEY *keyp);

/************ from action.c **********/
/***** should they be here ???????? */

void externalDisplay (KEY key) ;
void acedbMailer (KEY key, KEYSET ks, Stack sText);

#endif /* ACEDB_DISPLAY_H */
