/*  File: bstree.h
 *  Author: Jean Thierry-Mieg (mieg@mrc-lmb.cam.ac.uk)
 *  Copyright (C) J Thierry-Mieg and R Durbin, 1992
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
 * Description: Packing unpacking kernel routines
 * Exported functions:
 * HISTORY:
 * Last edited: Jan 16 11:03 2003 (edgrif)
 * Created: Tue Mar 10 03:40:07 1992 (mieg)
 * CVS info:   $Id: bstree.h,v 1.9 2003-01-22 14:56:01 edgrif Exp $
 *-------------------------------------------------------------------
 */
#ifndef ACEDB_BSTREE_H
#define ACEDB_BSTREE_H
 
#include <wh/bs.h>

/*** in bstools.c ***/

void bsTreePrune (BS bs) ;	/* prunes bs and everything beyond from tree */
BS   bsTreeCopy (BS bs) ;
void bsTreeDump (BS bs) ;	/* for debugging */
int  bsTreeSize (BS bs) ;	/* counts the number of nodes in the branch */

/*** in bstree.c - old system ***/

BS   bsTreeGet  (KEY key) ;
void bsTreeStore (BS bs) ;	/* implies destruction since fools the tree */
void bsTreeKill (KEY key) ;	/* removes key from first cache */

/*** in bs2block.c - new system for store/get ***/

int  bsTreeArraySize (BS bs, KEY timeStamp) ;  /* in BSdata units */

	/* get/store whole B object from/to disk */
BS   bsTreeArrayGet (KEY key) ;
void bsTreeArrayStore (BS bs, BOOL wait) ;  /* non destructive */
void bsTreeArrayFlush (void) ;	/* flush the waiting objects */

	/* get/store B tree from/to array */
BS   bsTreeGetFromArray (Array a, int pos, KEY timeStamp) ;
int  bsTreeAddToArray (BS bs, Array a, int pos, int size, KEY timeStamp) ;
				/* store in a starting at pos
				   if size==0 then finds for itself */
#endif /* !ACEDB_BSTREE_H */

