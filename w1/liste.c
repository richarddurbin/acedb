/*  File: liste.c
 *  Author: Richard Durbin (rd@sanger.ac.uk)
 *  Copyright (C) J Thierry-Mieg and R Durbin, 1995
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
     You can add and remove from a liste in a loop
     without making the list grow larger then the max current nimber
     this is more economic than hashing, where removing is inneficient
     and faster than an ordered set

     This library will not check for doubles,
      i.e. it maintains a list, not a set.
 * Exported functions:
     Liste listeCreate (STORE_HANDLE hh) ;
     listeDestroy(_ll) 
     listeMax(_ll) 
     int listeFind (Liste liste, void *vp)  ;
     int listeAdd (Liste liste, void *vp) ;
     void listeRemove (Liste liste, void *vp, int i)  ;

 * HISTORY:
 * Last edited: Oct 19 16:19 1999 (fw)
 * Created: oct 97 (mieg)
 *-------------------------------------------------------------------
 */

/* $Id: liste.c,v 1.13 2002-02-22 18:00:21 srk Exp $ */

#include "liste.h"

/* A liste is a list of void*
 * It is less costly than an associator if
 * the things in it are transient
 * because in an associator, you cannot easilly destroy
 * The liste does NOT check for doubles.
 */

static magic_t Liste_MAGIC = "Liste" ;
static void listeFinalise (void *block);
static BOOL listeExists (Liste liste);

/* complete opaque struct type declared in public header */
struct ListeStruct {
  magic_t *magic ;		/* == &Liste_MAGIC */
  int i ;     /* probably lowest empty slot */
  int curr ;  /* used by listeNext enumerator */
  Array a ;   /* the actual liste */
};

/******************************************************************
 ************************* public functions ***********************
 ******************************************************************/

Liste listeCreate (STORE_HANDLE handle)
{
  Liste liste;

  liste = (Liste)halloc (sizeof(struct ListeStruct), handle) ;
  blockSetFinalise(liste,listeFinalise) ;
  
  liste->magic = &Liste_MAGIC ;
  liste->i = 1 ;

  /* NOTE: the array is a freefloating allocation and must not be 
   * allocated upon the given handle - 
   * it is destroyed explicitly by finalisation */
  liste->a = arrayCreate (32, void*) ;

  array (liste->a,0,void*) = 0 ; /* avoid zero */

  return liste ;
} /* listeCreate */

void uListeDestroy (Liste liste)
{
  if (!listeExists(liste))
    messcrash("uListeDestroy() - received invalid Liste pointer");

  messfree (liste);

  return;
} /* uListeDestroy */

int listeMax (Liste liste)
{
  int max;

  if (!listeExists(liste))
    messcrash("listeMax() - received invalid Liste pointer");

  max = arrayMax(liste->a) - 1;

  return max;
} /* listeMax */

void listeRemove (Liste liste, void *vp, int i) 
{
  if (!listeExists(liste))
    messcrash("listeRemove() - received invalid Liste pointer");
  if (vp != array(liste->a, i, void*))
    messcrash ("listeRemove() - inconsistency detected") ;

  array (liste->a,i,void*) = 0 ;

  if (i < liste->i)
    liste->i = i ;

  return;
} /* listeRemove */
  
int listeAdd (Liste liste, void *vp) 
{
  void **vpp;
  int n;
  int i;

  if (!listeExists(liste))
    messcrash("listeAdd() - received invalid Liste pointer");

  i = liste->i ;
  vpp = arrayp(liste->a, i, void*) ;
  n = arrayMax(liste->a) ;  /* comes after arrayp of above line ! */

  while (*vpp && i++ < n) vpp++ ;

  array (liste->a,i,void*) = vp ;

  return i ;
} /* listeAdd */
  
int listeFind (Liste liste, void *vp) 
{
  int i ;
  void **vpp ;

  if (!listeExists(liste))
    messcrash("listeFind() - received invalid Liste pointer");

  i = arrayMax (liste->a) ;
  vpp = arrayp(liste->a, i - 1, void*) + 1 ;

  while (vpp--, i--)
    if (vp == *vpp) return i ;
  return 0 ;
} /* listeFind */
  
BOOL listeNext (Liste liste, void **vpp) 
{
  if (!listeExists(liste))
    messcrash("listeFind() - received invalid Liste pointer");

  if (!*vpp)
    liste->curr = 1 ;  /* avoid zero */
  if (liste->curr < arrayMax (liste->a))
    {
      *vpp = arrayp(liste->a, liste->curr++, void*) ;
      return TRUE ;
    }
  return FALSE ;
} /* listeNext */
  
/******************************************************************
 ************************ private functions ***********************
 ******************************************************************/

static void listeFinalise (void *block)
{
  Liste liste = (Liste)block ;
  
  if (!listeExists (liste))
    messcrash("listeFinalise() - received invalid block pointer");

  arrayDestroy(liste->a) ;
  liste->magic = 0 ;

  return;
} /* listeFinalise */

static BOOL listeExists (Liste liste)
     /* verify validity of a Liste-pointer */
{
  if (liste && liste->magic == &Liste_MAGIC)
    return TRUE;

  return FALSE;
} /* listeExists */

/******************** end of file **********************/

 
 
