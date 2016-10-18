/*  File: bump.h
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
 * Description:
 * Exported functions:
 * HISTORY:
 * Last edited: May 13 13:51 2008 (edgrif)
 * Created: Thu Aug 20 10:42:03 1992 (mieg)
 *-------------------------------------------------------------------
 */

/* $Id: bump.h,v 1.11 2008-05-16 13:56:24 edgrif Exp $ */

#ifndef DEF_BUMP_H
#define DEF_BUMP_H

#include <wh/regular.h>


/* forward declaration of opaque type */
typedef struct BumpStruct *BUMP;

BUMP bumpCreate (int ncol, int minSpace);
BUMP bumpReCreate (BUMP bump, int ncol, int minSpace);
void uBumpDestroy (BUMP bump);
#define bumpDestroy(b) (uBumpDestroy(b), b=0)
float bumpSetSloppy( BUMP bump, float sloppy);

/* Bumper works by resetting x,y
   bumpItem inserts and fills the bumper
   bumpTest restes x,y, but does not fill bumper
     this allows to reconsider the wisdom of bumping
   bumpRegister (called after bumpTest) fills 
     Test+Register == Add 
   bumpText returns number of letters that
     can be bumped without *py moving more than 3*dy  
*/

#define bumpItem(_b,_w,_h,_px,_py) bumpAdd(_b,_w,_h,_px,_py,TRUE)
#define bumpTest(_b,_w,_h,_px,_py) bumpAdd(_b,_w,_h,_px,_py,FALSE)
			
BOOL bumpAdd (BUMP bump, int wid, float height, int *px, float *py, BOOL doIt);
void bumpRegister (BUMP bump, int wid, float height, int *px, float *py) ;
int bumpText (BUMP bump, char *text, int *px, float *py, float dy, BOOL vertical) ;
int bumpMax(BUMP bump) ;
void asciiBumpItem (BUMP bump, int wid, float height, 
                                 int *px, float *py) ;
                                /* works by resetting x, y */


#endif /* DEF_BUMP_H */
