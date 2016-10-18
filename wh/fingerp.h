/*  File: fingerp.h
 *  Author: Ulrich Sauvage (ulrich@kaa.cnrs-mop.fr) 
 *  Copyright (C) J Thierry-Mieg and R Durbin, 1993
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
 * 	Richard Durbin (MRC LMB, UK) rd@mrc-lmba.cam.ac.uk, and
 *	Jean Thierry-Mieg (CRBM du CNRS, France) mieg@kaa.cnrs-mop.fr
 *
 * Description:
 * Exported functions:
 * HISTORY:
 * Last edited: Oct 19 05:36 1994 (mieg)
 * Created: Wed Oct 13 12:56:32 1993 (ulrich)
 *-------------------------------------------------------------------
 */

/* $Id: fingerp.h,v 1.3 1999-09-01 11:08:54 fw Exp $ */

#ifndef DEFINE_fingerp
#define DEFINE_fingerp

  /* donnees is a boolean matrix clones/lengths */
  /* donnees must be created as Array(unsigned char) before call */
void pmapCptGetData (Array marInDef, Array defInMar, KEYSET clones, KEYSET bands) ;
void fpClearDisplay (void) ;
void fpDisplay (KEY clone) ;

#endif
