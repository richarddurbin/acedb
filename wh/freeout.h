/*  File: freeout.h
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
 * Last edited: Aug 26 17:19 1999 (fw)
 * Created: Thu Aug 26 17:18:43 1999 (fw)
 * CVS info:   $Id: freeout.h,v 1.8 1999-09-01 11:09:27 fw Exp $
 *-------------------------------------------------------------------
 */

#ifndef ACEDB_FREEOUT_H
#define ACEDB_FREEOUT_H

#include "regular.h"

int freeOutSetFile (FILE *fil) ;
int freeOutSetStack (Stack s) ;

void freeOutInit (void) ;
void freeOut (char *text) ;
void freeOutf (char *format,...) ;
void freeOutxy (char *text, int x, int y) ; 
void freeOutBinary (char *data, int size) ;

void freeOutClose (int level) ;

int freeOutLine (void) ;
int freeOutByte (void) ;
int freeOutPos (void) ;

#endif /* !ACEDB_FREEOUT_H */
