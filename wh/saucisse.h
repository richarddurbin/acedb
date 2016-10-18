/*  File: saucisse.h
 *  Author: mieg
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
 * Description: 
 * Exported functions:
 * HISTORY:
 * Last edited: Nov 16 17:32 1999 (fw)
 * Created: Thu Aug 26 17:39:01 1999 (fw)
 * CVS info:   $Id: saucisse.h,v 1.6 2000-04-12 18:42:24 mieg Exp $
 *-------------------------------------------------------------------
 */


#ifndef SAUCISSE_DEF
#define SAUCISSE_DEF

#ifndef _SAUCISSE_DEF /* private to saucisse.c */
typedef void* Saucisse ;
#endif 

Saucisse saucisseCreate (int dim, int wordLength, BOOL isDna) ;

void uSaucisseDestroy (Saucisse saucisse) ;
#define saucisseDestroy(_saucisse)  (uSaucisseDestroy(_saucisse),(_saucisse) = 0)

void saucisseShow (Saucisse s, int threshHold, ACEOUT dump_out) ;
void  saucisseFill (Saucisse s, Array a) ;

#endif

