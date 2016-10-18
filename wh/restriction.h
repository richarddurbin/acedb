/*  File: restriction.h
 *  Author: Ed Griffiths (edgrif@sanger.ac.uk)
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
 *	Jean Thierry-Mieg (CRBM du CNRS, France) mieg@kaa.crbm.cnrs-mop.fr
 *
 * Description:
 * Exported functions:
 * HISTORY:
 * Last edited: Jun 12 13:53 2003 (edgrif)
 * * Jul 23 14:59 1998 (edgrif): First I added this header block, then
 *      I removed the redeclaration of an fmap function in fmap.h
 * Created: Thu Jul 23 14:59:08 1998 (edgrif)
 *-------------------------------------------------------------------
 */

/* $Id: restriction.h,v 1.10 2003-07-04 10:38:04 edgrif Exp $ */

#ifndef DEF_RESTRICTION
#define DEF_RESTRICTION

#include "regular.h"

typedef struct { int i ; int mark, type ;} Site ; /* see fmapsequence.c */
int siteOrder(void *a, void *b) ;

void dnacptMultipleMatch (Array sites, 
			  Array dna, Array protein, int frame,
			  Array colors, Stack s, Stack sname, 
			  int from, int length, 
			  BOOL amino, int maxError, int maxN,
			  Array peptide_translation_table) ;
     
Stack dnacptAnalyseRestrictionBox (char *text, BOOL amino, Stack *snp) ;
void dnaRepaint (Array colors) ;

void dnacptFingerPrintCompute (Array dna, int from, int to,
			       Array colors, Array hind3, Array sau3a,
			       Array fp);

#endif
