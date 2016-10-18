/* $Id: main.h,v 1.19 2008-05-16 13:57:13 edgrif Exp $ */
/*  File: main.h
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
 *	Jean Thierry-Mieg (CRBM du CNRS, France) mieg@kaa.crbm.cnrs-mop.fr
 *
 * Description: include for mainpick.c
 *           (note, only for graphical builds of acedb)
 * Exported functions:
 * HISTORY:
 * Last edited: May  9 15:39 2008 (edgrif)
 * Created: Wed Nov 18 16:52:27 1998 (fw)
 *-------------------------------------------------------------------
 */

#ifndef ACEDB_MAIN_H
#define ACEDB_MAIN_H

#include "acedb.h"
#include "graph.h"		/* for Graph type */

/************************************************************/

		/* init mainWindow display */
void mainSetInitClass (char *initclass);
void mainSetInitTemplate (char *inittemplate);
VoidRoutine mainWindowQuitRegister (VoidRoutine func);

Graph pickCreate (void);
void pickDraw (void);	      /* redraw main-window
			       * also called when write access changes */
void pickPopMain(void);

BOOL checkWriteAccess (void);	/* grab write access if possible 
				 * and user wants it */

void mainApplyNewLayout (Array newLayout);

void mainPickReport (int n) ;	/* used by mainpick.c & xclient.c */


/* if oldList is non-NULL it is replaced by the new list which is returned
 *  if listAllTypes is FALSE, the returned classes will have a 
 *      valid B-tree model */
Array classListCreate (Array oldList, 
		       BOOL listHidden,
		       BOOL listVisible, 
		       BOOL listBuried,
		       BOOL listAllTypes);

int ksetClassComplete (char *text, int len, int classe, BOOL self_destruct);
void mainKeySetComplete (KEYSET ks, char *text, int len, BOOL self_destruct);

/************************************************************/

#endif /* !ACEDB_MAIN_H */
