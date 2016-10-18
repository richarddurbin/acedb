/*  File: update.h
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
 * Description: module to add official data updates to the database
 * Exported functions:
 *              updateData
 *              updateDoAction
 * HISTORY:
 * Last edited: Dec  9 14:59 1998 (fw)
 * Created: Wed Dec  9 14:50:47 1998 (fw)
 *-------------------------------------------------------------------
 */

#ifndef _UPDATE_H
#define _UPDATE_H

#include "acedb.h"

void updateData(void);		  /* open update control window */

void updateDoAction (BOOL doAll); /* do the update, 
				     either one-by-one (doAll == FALSE)
				     or all of them (doAll == TRUE) */


#endif /* _UPDATE_H */
