/*  File: tree.h
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
 * Description: public header for treedisp.c
 *              included in graphical acedb compilations
 * HISTORY:
 * Last edited: Apr 18 16:22 2007 (edgrif)
 * Created: Tue Nov 24 09:55:12 1998 (fw)
 * CVS info:   $Id: tree.h,v 1.6 2007-04-18 15:37:18 edgrif Exp $
 *-------------------------------------------------------------------
 */
#ifndef _TREE_H
#define _TREE_H

#include "acedb.h"

/************************************************************/

void treeSetEdit(BOOL editable) ;
void treeUpdate (void) ;
BOOL treeDisplay (KEY key, KEY from, BOOL isOldGraph, void *unused);
BOOL treeChooseTagFromModel(int *type, int *targetClass, int classe, 
			    KEY *tagp, Stack s, int continuation);

/************************************************************/

char *expandText (KEY key);	/* used by wcs */

#endif /* _TREE_H */

/**************************** eof ***************************/
