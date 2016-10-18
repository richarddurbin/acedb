/*  File: embl.h
 *  Author: Ed Griffiths (edgrif@sanger.ac.uk)
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
 * Description: Header for embl dump code.
 * HISTORY:
 * Last edited: May 31 13:12 1999 (edgrif)
 * Created: Mon May 31 13:09:15 1999 (edgrif)
 * CVS info:   $Id: embl.h,v 1.2 1999-09-01 11:08:47 fw Exp $
 *-------------------------------------------------------------------
 */
#ifndef DEF_EMBL_H
#define DEF_EMBL_H


void emblDumpKey (KEY key) ;
void emblDumpKeySet (KEYSET kset) ;
void emblDumpKeySetFile (KEYSET kset, char *fileName) ;
void emblDump (void) ;



#endif /* DEF_EMBL_H */
