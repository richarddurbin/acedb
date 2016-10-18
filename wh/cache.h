/*  File: cache.h
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
 * Description: Prototypes of public functions of cache package 
 * Exported functions: 
 * HISTORY:
 * Last edited: Nov 17 16:09 2000 (edgrif)
 * * Aug 26 17:01 1999 (fw): added this header
 * Created: Thu Aug 26 17:01:35 1999 (fw)
 * CVS info:   $Id: cache.h,v 1.8 2000-11-17 16:43:14 edgrif Exp $
 *-------------------------------------------------------------------
 */

#ifndef ACEDB_CACHE_H
#define ACEDB_CACHE_H

#ifndef DEF_CACHE
#define DEF_CACHE
  typedef void* CACHE ;
#endif

#ifndef DEF_CACHE_HANDLE
  typedef void* CACHE_HANDLE ;
#endif


#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
/* The above disgusting mess of #defs should be replaced with this but sadly */
/* there is a conflict with bs_.h (!) which has its #define of these and     */
/* uses  DEF_CACHE & DEF_CACHE_HANDLE to get its versions declared, YUCH.    */
/*                                                                           */
/* One day I will clear it all up, this is really disgusting.                */
/*                                                                           */
typedef struct scache *CACHE1 ;

typedef void *CACHE_HANDLE ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */


  /* Note that a straight lock of an cache opened read only is
     forbidden because of possible concurrent access */

int cacheStatus (int *used, int *locked, int*known, int *modified, 
		 int *nc2rp, int *nc2wp, int *nccrp, int *nccwp) ;
CACHE cacheCreate(KEY k, CACHE_HANDLE *bsp, BOOL newCopy) ;
CACHE cacheUpdate(KEY k, CACHE_HANDLE *bsp, BOOL inXref) ;
void cacheClone(CACHE source, CACHE dest, CACHE_HANDLE *bsp) ;
void cacheDestroy(CACHE v) ;
void cacheSave(CACHE v) ;
void cacheCheck(KEY key, CACHE v) ;
void cacheUnlock(CACHE v) ;
void cacheKill (KEY key) ;

#endif /* !ACEDB_CACHE_H */
/******************************************************************/
 
