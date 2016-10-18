/*  File: methodcache.c
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
 * Description: implementation of MethodCache class
 *              maintains a list of objects that represent
 *              object of class Method in memory.
 * Exported functions: See methodcache.h
 * HISTORY:
 * Last edited: Oct  5 16:15 1999 (fw)
 * Created: Thu Sep 23 10:40:39 1999 (fw)
 * CVS info:   $Id$
 *-------------------------------------------------------------------
 */

#include "acedb.h"
#include "method.h"
#include "methodcache.h"
#include "whooks/classes.h"



/* complete opaque MethodCacheStuct */
struct MethodCacheStruct {
  Array entries;
  STORE_HANDLE handle;		/* contains all METHOD structs whether
				 * contained in the cache-array or 
				 * derived from a cached struct */
} ;

static void methodCacheFinalise (void *block);
static BOOL methodCacheExists (MethodCache mcache);
static METHOD *methodCacheInsert (MethodCache mcache, 
				  METHOD *meth, char *aceDiff);

/******************************************************************
 ************************* public functions ***********************
 ******************************************************************/


MethodCache methodCacheCreate (STORE_HANDLE handle)
     /* Create a new method-cache upon the given handle
      *  There should be one cache per context, e.g.
      *  a cache per FMAP instance, so within this context
      *  we don't duplicate METHOD struct for the same objects 
      * RETURNS:
      *   a new cache-struct pointer (alloc'd upon handle) */
{
  MethodCache mcache;

  mcache = (MethodCache)halloc(sizeof(struct MethodCacheStruct), handle);
  blockSetFinalise (mcache, methodCacheFinalise);

  /* these will be explicitly destroyed by finalisation */
  mcache->entries = arrayCreate (lexMax(_VMethod), METHOD*);
  mcache->handle = handleCreate();

  return mcache;
} /* methodCacheCreate */

/************************************************************/

void methodCacheDestroy (MethodCache mcache)
     /* just to avoid a #define to messfree - must not do anymore
      * in this function - all work is done in methodCacheFinalise */
{
  messfree (mcache);
} /* methodCacheDestroy */

/************************************************************/

METHOD *methodCacheGet (MethodCache mcache, KEY methodKey,
			char *aceDiff)
     /* Find the method for the given key in the cache
      *  or create a new struct from it's object
      *  in the database and insert it into the cache.
      * If a valid string, aceDiff will be applied to derive 
      *  a slight variation to the actual database-OBJ.
      * RETURNS:
      *   the method struct
      *   This struct will be tied to the cache's memory finalisation
      *     OR
      *   NULL if Method of the given is neither in Cache
      *        nor in database
      */
{
  METHOD *meth = NULL;
  int index;

  if (!methodCacheExists(mcache))
    messcrash ("methodCacheGet() - reveived invalid mcache");

  if (!methodKey)
    return NULL;

  if (class(methodKey) != _VMethod)
    return NULL;

  /* OK, so the KEY seems allright */

  index = KEYKEY(lexAliasOf(methodKey));
  
  if (index < arrayMax(mcache->entries))
    meth = arr (mcache->entries, index, METHOD*);

  if (meth)
    /* method already in the cache */
    {
      if (!methodExists(meth))
	messcrash("methodCacheGet() - found invalid METHOD in cache");
      
      if (!meth->isCached)
	messcrash("methodCacheGet() - Method:%s at slot %d should "
		  "have been marked as cached!",
		  meth->name, index);
    }
  else
    /* wasn't in cache - load from DB
     * NOTE: the new struct is alloc'd on the cache's handle */
    meth = methodCreateFromObj (methodKey, mcache->handle);

  if (meth != NULL)
    /* loading from DB may have failed */
    meth = methodCacheInsert (mcache, meth, aceDiff);

  return meth;			/* WARNING - may be NULL */
} /* methodCacheGet */

/************************************************************/

METHOD *methodCacheGetByName (MethodCache mcache, char *methodName, 
			      char *aceText, char *aceDiff)
     /* Find a METHOD struct in the cache by its name
      *  if it isn't found, a new one struct will be created
      *  by using the aceText to initilise the object,
      *  and it will be inserted into the cache then.
      * If a valid string, aceDiff will be applied to derive 
      *  a slight variation to the actual database-OBJ.
      *  NOTE: safer way to access the cache - never returns NULL!
      * RETURNS:
      *   struct pointer of METHOD, which is tied into the cache.
      *   This struct will be tied to the cache's memory finalisation
      *   That pointer is NEVER NULL!
      */
{
  KEY methodKey = 0;
  METHOD *meth = NULL;

  if (!methodCacheExists(mcache))
    messcrash ("methodCacheGetByName() - reveived invalid mcache");

  if (lexword2key (methodName, &methodKey, _VMethod))
    /* obj exists in DB - se we get it either from cache or load it
     * the aceDiff will be applied in the process */
    meth = methodCacheGet (mcache, 
			   methodKey, aceDiff);/* may return NULL */

  if (meth == NULL)
    /* couldn't get it from the cache and  it doesn't exist in the DB */
    {
      /* by allocating it on the cache's handle it'll be tied into the 
       * cache's memory finalisation, the user must not destroy the 
       * struct explicitly, it gets killed when the cache goes */
      meth = methodCreateFromDef (methodName, 
				  aceText, mcache->handle);
      meth = methodCacheInsert (mcache, meth, aceDiff);
    }
  
  return meth;
} /* methodCacheGetByName */









/******************************************************************
 ************************ private functions ***********************
 ******************************************************************/

static BOOL methodCacheExists (MethodCache mcache)
     /* determine whether the MethodCache pointer is valid
      * RETURNS
      *   TRUE if a valid non-NULL pointer
      *   otherwise FALSE
      */
{
  if (mcache && arrayExists(mcache->entries))
    /* we could use an extra ->magic in this struct, but
     * it's enough for now to verify the Array which uses a magic */
    return TRUE;

  return FALSE;
} /* methodCacheExists */

/************************************************************/


static void methodCacheFinalise (void *block)
     /* clear up memory allocated for the method-cache
      * we kill all the method-structs in the cache
      * and then kill the cache-array */

{
  MethodCache mcache = (MethodCache)block;

  if (!methodCacheExists(mcache))
    messcrash("methodCacheFinalise() - received invalid pointer");

  /* This'll kill all METHOD structs that are in the cache-array
   * or were derived from a cached struct */
  handleDestroy (mcache->handle);

  arrayDestroy (mcache->entries);

  return;
} /* methodCacheFinalise */

/************************************************************/

static METHOD *methodCacheInsert (MethodCache mcache,
				  METHOD *meth, char *aceDiff)
     /* Add the given struct into the given method-cache.
      *  The structs are kept in an array indexed by the
      *  number of the object in the class Method,
      *  i.e. KEYKEY(meth->key)
      *  cached objects are marked and not cached again.
      * If aceDiff is a not-empty string, an additional copy is 
      *  of the struct is made. This copy will be tied into the
      *  cache's memory finalisation, but not added to the cache
      *  directly as it would end up at the same index as the original
      *  struct that correspons to the database object.
      * RETURNS:
      *  The cached struct or it's diffed copy.
      */
{
  int index;
  METHOD *existing_meth = NULL;
  METHOD *inserted_meth = NULL;

  if (!methodCacheExists(mcache))
    messcrash ("methodCacheInsert() - reveived invalid mcache");

  if (!methodExists (meth))
    messcrash("methodCacheInsert() - received invalid meth");

  index = KEYKEY(meth->key);

  /* There may already be a method cached at this slot */
  if ((existing_meth = array(mcache->entries, index, METHOD*)))
    {
      if (!methodExists(existing_meth))
	messcrash("methodCacheInsert() - found invalid METHOD in cache");
      if (!existing_meth->isCached)
	messcrash("methodCacheInsert() - Method:%s at slot %d should "
		  "have been marked as cached!",
		  existing_meth->name, index);

      inserted_meth = existing_meth;
    }
  else
    /* slot not taken yet - add this method to the cache */
    {
      array (mcache->entries, index, METHOD*) = meth;
      
      meth->isCached = TRUE; /* set flag (just for assertions) */

      inserted_meth = meth;
    }
  
  if (aceDiff && strlen(aceDiff) > 0)
    /* create a copy of the struct by applying the aceDiff-string */
    {
      METHOD *copy_meth;
      copy_meth = methodCopy (meth, aceDiff, mcache->handle);
      inserted_meth = copy_meth; /* return copy instead */

      /* NOTE, that the new copy will not be inserted into the cache
       * as it would be placed at the same index as the original
       * struct (which corresponds to the database-obj).
       * However, it will be tied into the cache in terms of
       * memory allocation, as it'll die if the cache is finalised.
       */
    }

  return inserted_meth;
} /* methodCacheInsert */

/*************************** end of file *******************************/
