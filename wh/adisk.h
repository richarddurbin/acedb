/*  File: adisk.h
 *  Author: Richard Durbin (rd@sanger.ac.uk)
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
 * Description: prototypes for new way to store data for acedb5
 		replaces disk/block
 * HISTORY:
 * Last edited: Feb  8 00:34 1999 (rd)
 * Created: Mon Feb  8 00:32:17 1999 (rd)
 * CVS info:   $Id: adisk.h,v 1.7 1999-11-23 14:21:25 mieg Exp $
 *-------------------------------------------------------------------
 */

#ifndef DEFINE_DISK_H
#define DISK_H

#ifndef  DEF_DISK       
typedef KEY  DISK; 
#define DEF_DISK           
#endif

void aDiskArrayStore (KEY key, KEY parent, Array a, Array b) ;
BOOL aDiskArrayGet (KEY key, KEY *pp, Array *ap, Array *bp) ;

void aDiskKill (KEY key) ;

DISK aDiskAssign (KEY key, 
		int n1, int s1,
		int n2, int s2) ;

int  aDiskGetGlobalAdress(void)  ;

#endif
