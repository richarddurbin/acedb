/*  File: bu.h
 *  Author: Fred Wobus (fw@sanger.ac.uk)
 *  Copyright (c) J Thierry-Mieg and R Durbin, 1999
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
 * Description: A buTree is a bsTree plus surrounding ferns managing paths and Xref
 * Exported functions: 
 * HISTORY:
 * Last edited: Aug 26 17:01 1999 (fw)
 * * Aug 26 17:01 1999 (fw): added this header
 * Created: Thu Aug 26 17:00:53 1999 (fw)
 * CVS info:   $Id: bu.h,v 1.3 1999-09-01 11:05:08 fw Exp $
 *-------------------------------------------------------------------
 */


#ifndef ACEDB_BU_H
#define ACEDB_BU_H

      /* Functions to transfer a buTree  in/out the block cache */      
OBJ_HANDLE    	buGet     (KEY key) ;
void  	buStore     (KEY key, OBJ_HANDLE  x) ;
                              /* does not imply destructiony */
void buStoreNoCopy  (KEY key, OBJ_HANDLE  x) ;
                              /* implies bsTreePrune(x->root) */

     /* Functions for the manipulations of whole buTrees */
void 		buDestroy(OBJ_HANDLE  x) ;
OBJ_HANDLE   	buCopy   (OBJ_HANDLE  x) ;


#endif /* !ACEDB_BU_H */








