/*  File: grid.h
 *  Author: Richard Durbin (rd@sanger.ac.uk)
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
 * Description: header file for griddisp functions
 * Exported functions: gridCluster(), GRIDMAP structure
 *              (DisplayFunc) gridDisplay
 *              gridCluster
 *              gridClusterKey
 *              the GRIDMAP struct (also used by pmapconvert.c/cmapdisp.c)
 * HISTORY:
 * Last edited: Mar 15 11:05 2001 (edgrif)
 * Created: Mon Jan  6 18:58:11 1992 (rd)
 * CVS info:   $Id: grid.h,v 1.7 2001-03-15 13:55:21 edgrif Exp $
 *-------------------------------------------------------------------
 */
#ifndef _GRID_H
#define _GRID_H

#include "acedb.h"

typedef struct {
  KEY ctg ;
  float x1 ;
  float x2 ;
  int clump ;	   /* pos->clump filled with map index */
} GRIDMAP ;

BOOL gridDisplay (KEY key, KEY from, BOOL isOldGraph, void *unused) ;
/* the public DisplayFunc */

void gridCluster (Array pos, Array map, float range) ; 
BOOL gridClusterKey (KEY key, Array map, float range) ;
/* range is the maximum gap size tolerated in maintaining a cluster */

#endif /* _GRID_H */
