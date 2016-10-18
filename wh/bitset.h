/*  File: bitset.h
 *  Author: Richard Durbin (rd@sanger.ac.uk)
 *  Copyright (C) J Thierry-Mieg and R Durbin, 1997
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
 * 	Richard Durbin (MRC LMB, UK) rd@sanger.ac.uk, and
 *	Jean Thierry-Mieg (CRBM du CNRS, France) mieg@kaa.cnrs-mop.fr
 *
 * SCCS: $Id: bitset.h,v 1.9 2004-05-02 03:18:50 mieg Exp $
 * Description: transformation of array package for bits
 * Exported functions: a pure header - no code
 * HISTORY:
 * Last edited: May 24 17:50 1999 (fw)
 * * Sep 15 09:18 1998 (edgrif): Add a macro to define bitField so that
 *              those that want it can define it (avoids compiler warnings).
 * Created: Sat Feb  1 11:07:23 1997 (rd)
 *-------------------------------------------------------------------
 */

#ifndef BITSET_H
#define BITSET_H

#include "regular.h"

/* Some of the below macros operate on a array called 'bitField', to use     */
/* these macros code this macro first at the appropriate level in your       */
/* .c file.                                                                  */
#define BITSET_MAKE_BITFIELD                                                                  \
static unsigned int bitField[32] = {0x1, 0x2, 0x4, 0x8,                                \
				    0x10, 0x20, 0x40, 0x80,                            \
				    0x100, 0x200, 0x400, 0x800,                        \
				    0x1000, 0x2000, 0x4000, 0x8000,                    \
				    0x10000, 0x20000, 0x40000, 0x80000,                \
				    0x100000, 0x200000, 0x400000, 0x800000,            \
				    0x1000000, 0x2000000, 0x4000000, 0x8000000,        \
				    0x10000000, 0x20000000, 0x40000000, 0x80000000 } ;

typedef Array BitSet ;

Array bitSetCreate (int n, STORE_HANDLE h) ;
Array bitSetReCreate (Array bb, int n)  ;
void bitExtend (Array bb, int n) ;
#define bitSetDestroy(_x)	arrayDestroy(_x)

#define bitSetCopy(_x, _handle) arrayHandleCopy(_x, _handle)

/* the following use array() and so are "safe" */

#define bitSet(_x,_n)	(array((_x), (_n) >> 5, unsigned int) |= \
			 bitField[(_n) & 0x1f])
#define bitUnSet(_x,_n)	(array((_x), (_n) >> 5, unsigned int) &= \
			 ~bitField[(_n) & 0x1f])
#define bitt(_x,_n)	(array((_x), (_n) >> 5, unsigned int) & \
			 bitField[(_n) & 0x1f])

/* bit() uses arr() for optimal performance */

#define bit(_x,_n)	(arr((_x), (_n) >> 5, unsigned int) & \
			 bitField[(_n) & 0x1f])

                  /* Returns the number of set bits */
int bitSetCount (BitSet a) ;

           /* performs b1 = b1 OPERATOR b2, returns count (b1) */
int bitSetAND (BitSet b1, BitSet b2) ;
int bitSetOR (BitSet b1, BitSet b2) ;
int bitSetXOR (BitSet b1, BitSet b2) ;
int bitSetMINUS (BitSet b1, BitSet b2) ;
#endif

/***************** end of file *****************/
 
 
