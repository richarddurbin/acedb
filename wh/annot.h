/*  File: annot.h
 *  Author: Fred Wobus (fw@sanger.ac.uk)
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
 * Last edited: Aug 26 16:50 1999 (fw)
 * * Aug 26 16:50 1999 (fw): added this header
 * Created: Thu Aug 26 16:50:03 1999 (fw)
 * CVS info:   $Id: annot.h,v 1.3 1999-09-01 11:03:20 fw Exp $
 *-------------------------------------------------------------------
 */
#ifndef ACEDB_ANNOT_H
#define ACEDB_ANNOT_H

typedef struct noteStruct 
 { 
   int x; char *title ; char *tagName; KEY tag ; KEY type ; int length ; BSunit u ;
 } ANNOTATION ;

void annotate (KEY key, ANNOTATION *an) ;

#endif /* !ACEDB_ANNOT_H */
