/*  File: tabledisp.h
 *  Author: Fred Wobus (fw@sanger.ac.uk)
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
 *	Jean Thierry-Mieg (CRBM du CNRS, France) mieg@crbm.cnrs-mop.fr
 *
 * Description: public header for display functions, which deal
 *              with table definitions and the display of table objects
 *              and the multimap functions
 * Exported functions:
 * HISTORY:
 * Last edited: Mar 15 13:30 2001 (edgrif)
 * Created: Tue Dec 22 16:25:35 1998 (fw)
 * CVS info:   $Id: tabledisp.h,v 1.8 2001-03-15 13:56:04 edgrif Exp $
 *-------------------------------------------------------------------
 */
#ifndef ACEDB_TABLEDISP_H
#define ACEDB_TABLEDISP_H

#include "acedb.h"
#include "table.h"

/************************************************************/

/* DisplayFunc for TABLE* objects in class _VTableResult */
BOOL tableResultDisplay(KEY key, KEY from, BOOL isOldGraph, void *unused) ;

/* DisplayFunc for objects in class _VTable (opens table_maker) */
BOOL tableDisplay(KEY key, KEY from, BOOL isOldGraph, void *unused) ;

/* MenuFunction to open Table_Maker window with blank definition */
void tableMaker (void);

/************************************************************/

/* MenuFunction to open blank Aql Query window */
void aqlDisplayCreate (void);

/************************************************************/


/* open display of type "DtTableResult" for a given table */
/* isFlipMap is an Array of type BOOL, with a flag per column which 
 * states whether the multimap-display of this column shall be flipped,
 * this information originates in the table-maker definitions,
 * we have to pass this info to the tableDisplay in case the user
 * calls up the multimap display
 * this parameter may be NULL, if such information is unavailable *
 * Widtharray is analogous to isFlipMap but for starting columns widths */
Graph tableDisplayCreate (TABLE *table, char *title, 
			  Array isFlipMap, Array widthsArray);

/* re-use existing window of type "DtTableResult" for new table */
void tableDisplayReCreate (TABLE *table, char *title, 
			   Array isFlipMap, Array widthsArray);

/************************************************************/


/* DisplayFunc for objects in class _VMultiMap */
BOOL multiMapDisplay (KEY key, KEY from, BOOL isOldGraph, void *unused);

  
/* create multimap display from keyset of Map objects */
BOOL multiMapDisplayKeySet (char *title, KEYSET ks, int cl, 
			    KEY anchor_group, KEY anchor, int minMap);


/* open display of type "DtMULTIMAP" */
/* isFlipMap is an Array of type BOOL, with a flag per column which 
 * states whether the multimap-display of this column shall be flipped,
 * this information originates in the table-maker definitions
 * this parameter may be NULL, if such information is unavailable */
Graph multiMapDisplayCreate (TABLE *table, char *title, Array isFlipMap);
/* redraw on active graph */
void multiMapDisplayReCreate (TABLE *table, char *title, Array isFlipMap);
  
            
#endif /* !ACEDB_TABLEDISP_H */
