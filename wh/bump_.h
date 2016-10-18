/*  File: bump_.h
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
 * Description: private header for the internals of the BUMP-package
 * Exported functions:  none
 *              completion of the BUMP structure to allow
 *              the inside if the BUMP-package to access the members
 *              of the structure.
 * HISTORY:
 * Last edited: Dec 17 16:20 1998 (fw)
 * Created: Thu Dec 17 16:17:32 1998 (fw)
 *-------------------------------------------------------------------
 */


#ifndef DEF_BUMP__H
#define DEF_BUMP__H

#include "bump.h"		/* include public header */

/* allow verification of a BUMP pointer */
extern magic_t BUMP_MAGIC;

/* completion of public opaque type as declared in bump.h */

struct BumpStruct {
  magic_t *magic ;		/* == &BUMP_MAGIC */
  int n ;         /* max x, i.e. number of columns */
  float *bottom ; /* array of largest y in each column */
  int minSpace ;  /* longest loop in y */
  float sloppy ;
  float maxDy ;  /* If !doIt && maxDy !=0,  Do not add further down */
  int max ;      /* largest x used (maxX <= n) */
  int xAscii, xGapAscii ;
  float yAscii ;
} ;

#endif /* DEF_BUMP_H */
