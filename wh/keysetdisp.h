/*  File: keysetdisp.h
 *  Author: Fred Wobus (fw@sanger.ac.uk)
 *  Copyright (C) J Thierry-Mieg and R Durbin, 1999
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
 * Description: public header for operations ion the KeySet Display
 * Exported functions:
 * HISTORY:
 * Last edited: May  9 15:41 2008 (edgrif)
 * Created: Mon Mar  1 15:25:23 1999 (fw)
 * CVS info:   $Id: keysetdisp.h,v 1.10 2008-05-16 13:56:53 edgrif Exp $
 *-------------------------------------------------------------------
 */
#ifndef ACE_KEYSETDISP_H
#define ACE_KEYSETDISP_H

#include "acedb.h"

/************************************************************/

/* public opaque type - it is a handle to a keyset-window */
typedef struct KeySetDispStruct *KeySetDisp;

/************************************************************/

/* top-level DisplayFunc for KeySet objects */
BOOL keySetDisplay (KEY key, KEY from, BOOL isOldGraph, void *unused) ;

/* open a new KeySet-window for the given keyset
 * NOTE the currently active graph remains unchanged ! */
KeySetDisp keySetNewDisplay (KEYSET ks, char *title) ;

/************************************************************/

/* KeySetActive - function useful to retrieve a keyset from a window
 * select by the user. If a keyset-window is the active-keyset
 * it returns TRUE - 
 *  kset_p   - will be set to point at the KEYSET
 *             in the current active keyset-window.
 *  ksdisp_p - will be set to the KeySetDisp handle of the
 *             active keyset-window.
 *
 * if FALSE is returned no active keyset-window could be found
 *  the pointers are zeroed if they are non-NULL.
 *
 * if any pointer that is passed is NULL the assignment is ignored.
 *
 * NOTE  The returned keyset must never be destroyed by the calling 
 *       routine unless, a new keyset is provided to keySetShow on the 
 *       same lookp (which will replace the look->kset) with the new one
 *
 * Example scenario which replaces the keyset in the active window
 * with a new one, e.g. a filtered version :
 *
 *  KEYSET ks, ksnew;
 *  KeySetDisp ksdisp;
 *  if (keySetActive (&ks, &ksdisp)
 *    { ksnew = filterfunc(ks);
 *      keySetDestroy(ks);
 *      keySetShow(ksnew, ksdisp);
 *    }
 */
BOOL keySetActive(KEYSET *kset_p, KeySetDisp *ksdisp_p);


/* two functions which re-use an existing keyset-window, and pass
 * a new keyset. The window is identified by the handle, which can
 * be retrieved using keySetActive().
 * The user is responsible for destroying the keyset, that was formerly
 * displayed in that widnow (see above). */
KeySetDisp keySetShow(KEYSET kSet, KeySetDisp handle);
KeySetDisp keySetMessageShow(KEYSET kSet, KeySetDisp handle, char *message, BOOL self_destruct);

/************************************************************/

/* which "Show As"-displayType has been selected in a 
 * given keyset-window, currently only used in mainpick.c */
char *keySetGetShowAs (KeySetDisp ksdisp, KEY key);



#endif /* ACE_KEYSETDISP_H */
