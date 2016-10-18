/*  File: alignment_.h
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
 *	Jean Thierry-Mieg (CRBM du CNRS, France) mieg@kaa.crbm.cnrs-mop.fr
 *
 * Description: private header for alignment package
 *              comprising of alignment.c (generic)
 *              and alignmentdisp.c (graphical display)
 * HISTORY:
 * Last edited: Nov 19 11:48 1998 (fw)
 * Created: Thu Nov 19 11:46:21 1998 (fw)
 *-------------------------------------------------------------------
 */
#ifndef _ALIGNMENT__H
#define _ALIGNMENT__H


#define GAP 21

typedef struct {		/* Alignment component structure */
  KEY key ;
  int start, end ;
  int flag ;
  char *seq ;
  int *pos ;
} ALIGN_COMP ;

typedef struct {		/* Alignment structure */
  KEY key ;
  Array comp ;			/* of ALIGN_COMP */
  STORE_HANDLE handle ;
} ALIGNMENT ;

ALIGNMENT *alignGet (KEY key);

#endif /* _ALIGNMENT__H */
