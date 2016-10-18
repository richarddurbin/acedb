/*  File: colcontrol_.h
 *  Author: Simon Kelley (srk@sanger.ac.uk)
 * Copyright (C) J Thierry-Mieg and R Durbin, 1994
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
 * Description: private header file for colcontrol.c and viewedit.c
 * HISTORY:
 * Last edited: Mar  8 09:18 2000 (edgrif)
 * * Jan 27 10:36 1999 (edgrif): added menu.h for MENU definition 
 * * Oct 13 14:25 1998 (edgrif): Removed ACEDB #if, added a few things
 *              needed for acedb/graph interface.
 * Created: Wed 1 Mar 1995 (srk)
 *-------------------------------------------------------------------
 */

/*  $Id: colcontrol_.h,v 1.10 2000-03-08 09:43:39 edgrif Exp $  */

#ifndef DEF_COLCONTROL__H
#define DEF_COLCONTROL__H

#include <wh/colcontrol.h>				    /* Our public header. */
#include <wh/menu.h>					    /* for menu definition. */

BOOL instanceExists(MAPCONTROL map, COLPROTO proto);
void viewWindowCreate(void *dummy);
COLINSTANCE controlInstanceCreate(COLPROTO proto,
				  MAPCONTROL map,
				  BOOL displayed,
				  OBJ init,
				  char *name);

/* These were static in viewedit.c but need to be exposed for the acedb-     */
/* graphinterface.                                                           */
VIEWWINDOW currentView (char *callerFuncName) ;
void viewWindowDraw(VIEWWINDOW view) ;


struct ConversionRecord {   /* record of a conversion routine to be called */
  void *(*convertRoutine)();
  void *params;
  void *results;
};

#define EDITLEN 60

struct ViewWindowStruct {
  magic_t *magic;		/* == &VIEWWINDOW_MAGIC */
  COLCONTROL control;
  MAPCONTROL currentMap;
  Graph graph, helpGraph;
  Associator boxToArrow;
  Associator boxToInstance;
  Associator boxToProto;
  Associator boxToButton;
  FREEOPT *mapMenu;
  COLPROTO currentProto;
  COLINSTANCE currentInstance;
  int currentInstanceIndex;
  BOOL editInstance;
  STORE_HANDLE handle;
  BOOL dirty;
  int suppressBox, submenusBox, cambridgeBox, hideBox;
  MENU menu;
  char buffer[EDITLEN+1];
} ;


/* ACEDB-GRAPH INTERFACE: Moved here from colcontrol.c itself, because we    */
/* need it for graph-acedb interface.                                        */
typedef struct SpacerPriv {
  int colour;
  float width;
} *SPACERPRIV;


#endif /* DEF_COLCONTROL__H */
