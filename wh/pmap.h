/*  File: pmap.h
 *  Author: Neil Laister (nl1@mrc-lmb.cam.ac.uk)
 *  Copyright (C) J Thierry-Mieg and R Durbin, 1992
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
 * Description:
 * Exported functions:
 * HISTORY:
 * Last edited: Mar 15 13:29 2001 (edgrif)
 * Created: Fri Jan 17 15:43:30 1992 (nl1)
 * CVS info:   $Id: pmap.h,v 1.4 2001-03-15 13:55:53 edgrif Exp $
 *-------------------------------------------------------------------
 */
#ifndef _PMAP_H
#define _PMAP_H

#include "acedb.h"

void pMapSetHighlightKeySet (KEYSET k); /*argument NULL causes reset*/
BOOL pMapDisplay(KEY key, KEY from, BOOL isOldGraph, void *unused) ; /* DisplayFunc */


#endif /* _PMAP_H */
