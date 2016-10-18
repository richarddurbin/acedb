/*  File: cache_.h
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
 * Description: Private to cachesubs.c who handles the second cache.
 * Exported functions: 
 * HISTORY:
 * Last edited: Nov 17 13:37 2000 (edgrif)
 * Created: Thu Aug 26 17:02:45 1999 (fw)
 * CVS info:   $Id: cache_.h,v 1.5 2000-11-17 16:44:13 edgrif Exp $
 *-------------------------------------------------------------------
 */

#ifndef ACEDB_CACHE__H
#define ACEDB_CACHE__H


#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
/* we should be able to include the public header here, but this is not      */
/* possible at the moment...sigh...                                          */
#include <wh/cache.h>					    /* Our public header. */
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */



#ifndef DEF_CACHE_HANDLE
#define DEF_CACHE_HANDLE
  typedef void* CACHE_HANDLE ;
#endif


#define DEF_CACHE					    /* YUCH. */
/* This line should go in the public header but this clashes with bs_.h,     */
/* see public header for comments on this.                                   */
typedef struct scache *CACHE ;

struct scache {
  KEY key;
  int magic ;
  char type ;
  int lock ;
  int hasBeenModified;
  int refCount ;
  CACHE  up, next;
  CACHE_HANDLE x ;	
} ScacheRec ;


#define CACHEMAGIC 987634

#define SIZE_OF_CACHE  sizeof(struct scache) 
 
  /* Note that a straight lock of an cache opened read only is
     forbidden because of possible concurrent access */

/******************************************************************/

/* functions private to the cache package, 
   that allow objcachedisp.c access to cache system variables
   within this file */

CACHE cacheGetKnownTop (void);
CACHE cacheGetLockedTop (void);

#endif /* !ACEDB_CACHE__H */
/**************************** eof ************************************/
