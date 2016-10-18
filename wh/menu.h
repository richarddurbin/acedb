/*  File: menu.h
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
 * Description: Public header for new menu facility.
 *
 * HISTORY:
 * Last edited: Jul 13 13:08 2001 (edgrif)
 * * Jan 28 16:09 1999 (edgrif): Redo opaque handles properly.
 * Created: Thu Jan 28 16:08:41 1999 (edgrif)
 * CVS info:   $Id: menu.h,v 1.9 2001-07-16 17:34:16 edgrif Exp $
 *-------------------------------------------------------------------
 */
#ifndef DEF_MENU_H
#define DEF_MENU_H

#include "regular.h"


/************* types *****************/


/* Public handles to opaque types.                                           */
typedef struct _MenuStruct *MENU ;
typedef struct _MenuItemStruct *MENUITEM ;


/* prototype for functions that can be attached to menu items.               */
typedef void (*MENUFUNCTION)(MENUITEM) ;


typedef struct menuspec
  { MENUFUNCTION f ;	/* NB can be 0 if using menuSetCall() below */
    char *text ;
  } MENUSPEC ;


/********** MENUITEM flags ***********/

#define MENUFLAG_DISABLED	0x01
#define MENUFLAG_TOGGLE		0x02
#define MENUFLAG_TOGGLE_STATE	0x04
#define MENUFLAG_START_RADIO	0x08
#define MENUFLAG_END_RADIO	0x10
#define MENUFLAG_RADIO_STATE	0x20
#define MENUFLAG_SPACER		0x40
#define MENUFLAG_HIDE		0x80

/************* functions *************/

MENU menuCreate (char *title) ;
	/* makes a blank menu */
MENU menuInitialise (char *title, MENUSPEC *spec) ;
	/* makes a simple menu from a spec terminated with label = 0 */
	/* if called on same spec, give existing menu */
MENU menuCopy (MENU menu) ;
	/* a copy that you can then vary */
void menuDestroy (MENU menu) ;
	/* also destroys items */

MENUITEM menuCreateItem (char *label, MENUFUNCTION func) ;
MENUITEM menuItem (MENU menu, char *label) ;  
	/* find item from label */
BOOL menuAddItem (MENU menu, MENUITEM item, char *beforeLabel) ;
	/* add an item; if before == 0 then add at end */
BOOL menuDeleteItem (MENU menu, char *label) ; 
	/* also destroys item */
BOOL menuSelectItem (MENUITEM item) ;
        /* triggers a call back and adjusts toggle/radio states */
        /* returns true if states changed - mostly for graph library to use */

	/* calls to set properties of items */
	/* can use by name e.g. menuSetValue (menuItem (menu, "Frame 3"), 3) */
BOOL menuSetLabel (MENUITEM item, char *label) ;
BOOL menuSetCall (MENUITEM item, char *callName) ;
BOOL menuSetFunc (MENUITEM item, MENUFUNCTION func) ;
BOOL menuSetFlags (MENUITEM item, unsigned int flags) ;
BOOL menuUnsetFlags (MENUITEM item, unsigned int flags) ;
BOOL menuSetValue (MENUITEM item, int value) ;
BOOL menuSetPtr (MENUITEM item, void *ptr) ;
BOOL menuSetMenu (MENUITEM item, MENU menu) ; /* pulldown for boxes */
	/* and to get properties */
unsigned int	menuGetFlags (MENUITEM item) ;
int	        menuGetValue (MENUITEM item) ;
void*		menuGetPtr (MENUITEM item) ;

	/* extra routines */

void menuSuppress (MENU menu, char *string) ; /* HIDE block */
void menuRestore (MENU menu, char *string) ; /* reverse of Suppress */

void menuSpacer (void) ; /* dummy routine for spaces in opt menus */

#endif /* DEF_MENU_H */
