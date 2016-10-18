/*  File: zmap.h
 *  Author: Gemma Barson <gb10@sanger.ac.uk>
 *  Copyright (C) 2011 Genome Research Ltd
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
 * Description: Public header file for zmap
 *-------------------------------------------------------------------
 */
#ifndef ACEDB_ZMAP_H
#define ACEDB_ZMAP_H
/*                  NO CODE BEFORE THIS                                      */

#include <wh/acedb.h>
#include <wh/dict.h>

/************************************************************/

/* fMap display data, controls various aspects of creation/destruction       */
/* of fMap. Passed via displayApp() to fMapDisplay() as a void *             */
typedef struct _ZmapDisplayData
{
  BOOL destroy_obj ;			    /* TRUE: destroy "key" obj. passed to
							       zMapDisplay. */

  BOOL include_methods ;		    /* whether to include or exclude */
  BOOL include_sources ;		    /* whether to include or exclude */
  DICT *methods_dict ;			    /* the methods DICT.           */
  DICT *sources_dict ;                      /* the sources DICT            */
  DICT *features ;
  BOOL keep_selfhomols, keep_duplicate_introns ;

} ZmapDisplayData ;

/************************************************************/

/* Public zmap functions.  */

	/* zmapdisplay.c */
BOOL zMapDisplay (KEY key, KEY from, BOOL isOldGraph, void *app_data);


/*                  NO CODE AFTER THIS                                       */
#endif /* ACEDB_ZMAP_H */
