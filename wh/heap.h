/*  File: heap.h
 *  Author: Richard Durbin (rd@mrc-lmb.cam.ac.uk)
 *  Copyright (C) J Thierry-Mieg and R Durbin, 1991
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
 * Description: header file for heap package
 * Exported functions:
 * HISTORY:
 * Last edited: Feb  6 00:20 1993 (mieg)
 * Created: Sat Oct 12 21:30:43 1991 (rd)
 *-------------------------------------------------------------------
 */

/* $Id: heap.h,v 1.2 1999-09-01 11:11:13 fw Exp $ */

#ifndef HEAP_INTERNAL
typedef void* Heap ;
#endif

Heap heapCreate (int size) ;
void heapDestroy (Heap heap) ;
int  heapInsert (Heap heap, float score) ;
int  heapExtract (Heap heap, float *sp) ;

/* end of file */
