/*  File: banner.h
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
 * Description: public header for banner.c
 * Exported functions:
 *              bannerMainStrings 
 *              bannerDataInfoStrings
 *              bannerWrite
 * HISTORY:
 * Last edited: Dec  4 14:51 1998 (fw)
 * Created: Thu Nov 19 15:38:21 1998 (fw)
 *-------------------------------------------------------------------
 */


#ifndef _BANNER_H
#define _BANNER_H

#include "regular.h"	   /* libutil header for Array and BOOL types */

Array bannerMainStrings (char *program,
			 BOOL isGraphical,
			 BOOL isAcembly) ;
Array bannerDataInfoStrings (void) ;
void bannerWrite (Array strings) ;

#endif /* _BANNER_H */
