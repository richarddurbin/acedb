/*  File: liste.h
 *  Author: Richard Durbin (rd@sanger.ac.uk)
 *  Copyright (C) J Thierry-Mieg and R Durbin, 1995
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
     You can add and remove from a liste in a loop
     without making the list grow larger then the max current number
     this is more economic than hashing, where removing is inneficient
     and faster than an ordered set

     This library will not check for doubles,
      i.e. it maintains a list, not a set.
 * Exported functions:
 * HISTORY:
 * Last edited: Oct 15 13:59 1999 (fw)
 * Created: oct 97 (mieg)
 *-------------------------------------------------------------------
 */

/* $Id: liste.h,v 1.8 2000-04-03 03:46:34 mieg Exp $ */
#ifndef LISTE_H
#define LISTE_H

#include "regular.h"

typedef struct ListeStruct *Liste ; /* opaque public type */

Liste listeCreate (STORE_HANDLE handle) ;
void uListeDestroy (Liste liste); /* just a stub for messfree */
#define listeDestroy(_l) (uListeDestroy(_l),(_l)=0)
int listeMax (Liste liste);
int listeFind (Liste liste, void *vp)  ;
int listeAdd (Liste liste, void *vp) ;
void listeRemove (Liste liste, void *vp, int i)  ;
BOOL listeNext (Liste liste, void **vp) ; /* call first with *vp=0 */

#endif /* ndef LISTE_H */
/******* end of file ********/
 
