/*  File: vmap.h
 *  Author: Richard Durbin (rd@mrc-lmb.cam.ac.uk)
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
 * Description: header file for public vMap operations
 * Exported functions:
 * HISTORY:
 * Last edited: Mar 15 13:31 2001 (edgrif)
 * Created: Thu Nov 26 05:15:10 1992 (rd)
 * CVS info:   $Id: vmap.h,v 1.11 2001-03-15 13:56:17 edgrif Exp $
 *-------------------------------------------------------------------
 */
#ifndef _VMAP_H
#define _VMAP_H

#include "acedb.h"

/* public DisplayFunc for display type "VMAP" */
BOOL vMapDisplay(KEY key, KEY from, BOOL isOldGraph, void *unused) ;

/* public function to recalculate all maps */
void vMapMakeAll(void) ;

#endif /* _VMAP_H */

/********** end of file **********/
 
