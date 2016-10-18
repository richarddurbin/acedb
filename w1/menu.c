/*  File: menu.c
 *  Author: Jean Thierry-Mieg (mieg@mrc-lmb.cam.ac.uk)
 *  Copyright (C) J Thierry-Mieg and R Durbin, 1992-1995
 * -------------------------------------------------------------------
 * This file is part of the ACEDB genome database package, written by
 * 	Richard Durbin (MRC LMB, UK) rd@mrc-lmb.cam.ac.uk, and
 *	Jean Thierry-Mieg (CRBM du CNRS, France) mieg@kaa.cnrs-mop.fr
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * -------------------------------------------------------------------
 * This file is part of the ACEDB genome database package, written by
 * 	Richard Durbin (MRC LMB, UK) rd@mrc-lmb.cam.ac.uk, and
 *	Jean Thierry-Mieg (CRBM du CNRS, France) mieg@kaa.cnrs-mop.fr
 *
 * Description:
 * Exported functions: all in menu.h
 * HISTORY:
 * Last edited: Oct  8 15:02 1999 (fw)
 * * Jul  7 13:35 1999 (edgrif): Added menuSetLabel to change menu label.
 * * Jan 28 16:07 1999 (edgrif): small changes caused by menu.h/menu_.h
 *              changes for opaque type defs.
 * * Jan  9 22:52 1995 (rd): complete replacement for new menu package
 * * Jul 23 21:53 1992 (rd): big change because of new menus
     must have a separate MENUOPT* for each combination of entries
 * Created: Mon Jun 15 14:43:55 1992 (mieg)
 *-------------------------------------------------------------------
 */

/* $Id: menu.c,v 1.14 2002-02-22 18:00:21 srk Exp $ */
 
#include "regular.h"
#include "call.h"		/* for call() and callExists() */
#include "menu_.h"		/* private header for menu package */

/****************** MENU manipulation *******************/

MENU menuCreate (char *title)
{
  MENU menu = (MENU) messalloc (sizeof (MenuStruct)) ;
  menu->title = title ;
  menu->items = 0 ;

  return menu ;
}

MENU menuCopy (MENU oldMenu)
{
  MENU newMenu = (MENU) messalloc (sizeof (MenuStruct)) ;
  MENUITEM newItem, oldItem ;

  newMenu->title = oldMenu->title ;
  for (oldItem = oldMenu->items ; oldItem ; oldItem = oldItem->down)
    { newItem = menuCreateItem (oldItem->label, oldItem->func) ;
      *newItem = *oldItem ;
      menuAddItem (newMenu, newItem, 0) ;
    }

  return newMenu ;
}

void menuDestroy (MENU menu)
{
#ifdef NO_WE_CACHE_MENUS_IN_ASS_BELOW__DONT_DESTROY_HERE
  MENUITEM item = menu->items ;
  while (item)
    { MENUITEM next = item->down ;
      messfree (item) ;
      item = next ;
    }

  messfree (menu) ;
#endif
}

MENU menuInitialise (char *title, MENUSPEC *spec)
{
  MENU menu ;
  MENUITEM item ;
  static Associator ass = 0 ;
  static MENUSPEC *localSpec ;

  /* getMutex ("menuInitialise") ; since we handle a static ass */
  if (!ass)
    ass = assCreate () ;

  if (assFind (ass, spec, &menu))
    return menu ;

  if (!spec)
    spec = localSpec ;
  menu = menuCreate (title) ;
  while (spec->text)
    { item = menuCreateItem (spec->text, spec->f) ; 
      if (!*spec->text)	/* convenience - blank menu items are spacer */
	menuSetFlags (item, MENUFLAG_SPACER) ;
      if (!spec->f)		/* a submenu */
	{ localSpec = spec+1 ;
	  item->submenu = menuInitialise (spec->text, 0) ;
	  spec = localSpec ;
	}
      else 
	item->submenu = 0 ;
      menuAddItem (menu, item, 0) ;
      ++spec ;
    }
  localSpec = spec ;

  /* releaseMutex ("menuInitialise") ; */
  return menu ;
}

/******************* MENUITEM manipulation ******************/

MENUITEM menuCreateItem (char *label, MENUFUNCTION func)
{
  MENUITEM item = (MENUITEM) messalloc (sizeof (MenuItemStruct)) ;
  memset (item, 0, sizeof (MenuItemStruct)) ;
  item->label = label ;
  item->func = func ;
  return item ;
}

MENUITEM menuItem (MENU menu, char *label)
{
  MENUITEM item ;
  for (item = menu->items ; item && strcmp (label, item->label) ; item = item->down) ;
  return item ;
}

BOOL menuAddItem (MENU menu, MENUITEM item, char *beforeLabel)
{
  MENUITEM below, above = 0 ;

  if (!item) return FALSE ;

  if (beforeLabel)
    { for (below = menu->items ; 
	   below && strcmp (below->label, beforeLabel) ;
	   above = below, below = below->down) ;
      if (!below)
	return FALSE ;
    }
  else
    for (below = menu->items ; below ; above = below, below = below->down) ;

  if (above)
    { above->down = item ;
      item->up = above ;
    }
  else
    { menu->items = item ;
      item->up = 0 ;
    }

  item->down = below ;
  if (below)
    below->up = item ;

  return TRUE ;
}

BOOL menuDeleteItem (MENU menu, char *label)
{
  MENUITEM item ;

  if (!(item = menuItem (menu, label)))
    return FALSE ;

  if (item->up)
    item->up->down = item->down ;
  else
    menu->items = item->down ;

  if (item->down)
    item->down->up = item->up ;

  if (item->submenu)
    menuDestroy (item->submenu) ;

  messfree (item) ;

  return TRUE ;
}

BOOL menuSelectItem (MENUITEM item)
{
  BOOL changed = FALSE ;

  if (item->flags & (MENUFLAG_DISABLED | MENUFLAG_SPACER))
    return FALSE ;

  if (item->flags & MENUFLAG_TOGGLE)
    { 
      if (item->flags & MENUFLAG_TOGGLE_STATE)/* if flag is set */
	item->flags &= ~MENUFLAG_TOGGLE_STATE; /* unset flag */
      else
	item->flags |= MENUFLAG_TOGGLE_STATE; /* set flag */

      changed = TRUE ;
    }
  else if (!(item->flags & MENUFLAG_RADIO_STATE))
    { 
      MENUITEM first, last ;

      for (first = item ; 
	   first && !(first->flags & MENUFLAG_START_RADIO) ; 
	   first = first->up) ;
      for (last = item ; 
	   last && !(last->flags & MENUFLAG_END_RADIO) ; 
	   last = last->down) ;
      if (first && last)
	{ while (first != last)
	    { first->flags &= ~MENUFLAG_RADIO_STATE ;
	      first = first->down ;
	    }
	  item->flags |= MENUFLAG_RADIO_STATE ;
	  changed = TRUE ;
	}
    }

  /* Call user func with new menu flags and states, 
   * so the user has a chance to change them if the code requires so.
   * Lazy code relying on the implied logic of toggles and radios
   * need to do nothing to the menuitem-flags in the registered func */
  if (item->func)
    (*item->func)(item) ;
  else if (item->call)
    call (item->call, item) ;

  return changed ;
} /* menuSelectItem */

/***************** set item properties *******************/
BOOL menuSetLabel (MENUITEM item, char *label)
{ if (!item) return FALSE ;
  item->label = label ; return TRUE ;
}

BOOL menuSetCall (MENUITEM item, char *callName)
{ if (!item || !callExists (callName)) return FALSE ;
  item->call = callName ; return TRUE ;
}

BOOL menuSetFunc (MENUITEM item, MENUFUNCTION func)
{ if (!item) return FALSE ;
  item->func = func ; return TRUE ;
}

BOOL menuSetFlags (MENUITEM item, unsigned int flags)
{ if (!item) return FALSE ;
  item->flags |= flags ; return TRUE ;
}

BOOL menuUnsetFlags (MENUITEM item, unsigned int flags)
{ if (!item) return FALSE ;
  item->flags &= ~flags ; return TRUE ;
}

BOOL menuSetValue (MENUITEM item, int value)
{ if (!item) return FALSE ;
  item->value = value ; return TRUE ;
}

BOOL menuSetPtr (MENUITEM item, void *ptr)
{ if (!item) return FALSE ;
  item->ptr = ptr ; return TRUE ;
}

BOOL menuSetMenu (MENUITEM item, MENU menu)
{ if (!item || !menu) return FALSE ;
  item->submenu = menu ; return TRUE ;
}

/***************** get item properties *******************/

unsigned int menuGetFlags (MENUITEM item)
{ if (!item) return 0 ;
  return item->flags ;
}

int menuGetValue (MENUITEM item)
{ if (!item) return 0 ;
  return item->value ;
}

void* menuGetPtr (MENUITEM item)
{ if (!item) return 0 ;
  return item->ptr ;
}

/**************** utilities *******************/

void menuSuppress (MENU menu, char *string)
{
  BOOL inBlock = FALSE ;
  BOOL hide = FALSE ;
  MENUITEM item ;

  for (item = menu->items ; item ; item = item->down)
    { if (!inBlock && !strcmp (item->label, string))
	hide = TRUE ;
      if (hide)
	item->flags |= MENUFLAG_HIDE ;
      if (item->flags & MENUFLAG_SPACER)
	{ inBlock = FALSE ;
	  hide = FALSE ;
	}
      else
	inBlock = TRUE ;
    }
}

void menuRestore (MENU menu, char *string)
{
  BOOL inBlock = FALSE ;
  BOOL unHide = FALSE ;
  MENUITEM item ;

  for (item = menu->items ; item ; item = item->down)
    { if (!inBlock && !strcmp (item->label, string))
	unHide = TRUE ;
      if (unHide)
	item->flags &= ~MENUFLAG_HIDE ;
      if (item->flags & MENUFLAG_SPACER)
	{ inBlock = FALSE ;
	  unHide = FALSE ;
	}
      else
	inBlock = TRUE ;
    }
}

#if !defined(MACINTOSH)
void menuSpacer (void) { return ; }
#endif

/***************** end of file ****************/
