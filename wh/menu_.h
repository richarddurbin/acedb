/*  File: menu_.h
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
 * Description: private header for menu package with full structures
 * Exported functions:
 * HISTORY:
 * Last edited: Mar  7 16:11 2000 (edgrif)
 * * Jan 28 16:10 1999 (edgrif): rename structs to support opaque types.
 * Created: Mon Jan  9 22:54:36 1995 (rd)
 * CVS info:   $Id: menu_.h,v 1.8 2000-03-08 09:44:02 edgrif Exp $
 *-------------------------------------------------------------------
 */
#ifndef DEF_MENU_PRIV_H
#define DEF_MENU_PRIV_H

#include <wh/menu.h>


typedef struct _MenuItemStruct {
  char*		label ;
  MENUFUNCTION	func ;
  unsigned int	flags ;
  char*		call ;
  int		value ;
  void*		ptr ;
  MENU		submenu ;
  MENUITEM	up, down ;
} MenuItemStruct ;

typedef struct _MenuStruct {
  char *title ;
  MENUITEM items ;
} MenuStruct ;



#endif /* DEF_MENU_PRIV_H */
 
