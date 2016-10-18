/*  Last edited: Jun 13 14:39 2007 (edgrif) */
#include <wh/regular.h>
#include <wh/bs_.h>
#include <wh/acelib.h>
#include <wh/freeout.h>
#include <wh/bs.h>
#include <wh/command.h>
#include <wh/lex.h>
#include <whooks/sysclass.h>
#include <whooks/systags.h>
#include <wh/table.h>
#include <wh/bitset.h>
#include <wh/aql.h>
#include <wh/dna.h>
#include <wh/pick.h>
#include <wh/peptide.h>
#include <wh/session.h>
#include <wh/parse.h>

#include <wac/ac.h>
#include <wac/ac_.h>
#include <wac/acinside_.h>
#include <wh/aceio.h>
#include <wh/a.h>

#define ACE_NCBI_zzzz
static void ac_finalise (void *);

BITSET_MAKE_BITFIELD 	 /*	 define bitField for bitset.h ops */

/********************************************************************/
/********************************************************************/
/*
 * connect to the internal existing database 
 */

static int ac_open_counter = 0;
/*
 * counter to know when we should call aceQuit().
 */


static void ac_close_on_exit()
{
  if (ac_open_counter > 0)
    aceQuit(TRUE);
}

AC_DB ac_open_db (char *database, char **error)
{
  AC_DB db;

  /*
   * thisSession.session > 0 if the database is already open
   */
  if (thisSession.session != 0)
    {
      /*
       * If it is initialized already, we are embedded in tace or something
       * similiar, and we do not want to initialize the database.
       */
    }
  else
    {
      /*
       * If it is NOT initialized already, we are running in a standalone
       * Ace C program, and we need to initialize the database and see
       * that we de-initialize it later.
       */
      if (ac_open_counter == 0)
	{
	  aceInit(database);
	  atexit(ac_close_on_exit);
	}
      ac_open_counter++;
    }

  db = (AC_DB) halloc (sizeof (struct ac_db), 0) ;
  blockSetFinalise (db, ac_finalise) ;

  if (error) *error = 0 ; /* internal opening cannot fail */
  db->magic = MAGIC_BASE + MAGIC_AC_DB ;
  db->handle = ac_new_handle () ;
  db->command_stack = stackHandleCreate (1000, db->handle) ;
  stackTextOnly (db->command_stack) ;
  db->look = aceCommandCreate (7, 0xff) ;

  return db ;
}

/********************************************************************/

void ac_finalise (void *v)
{
  int magic = v ? *(int *)v : 0 ;
  int type = v && magic ? magic - MAGIC_BASE : 0 ;

  if (v && magic)
    switch (type)
      {
      case MAGIC_AC_DB:
	{
	  AC_DB db = (AC_DB) v ;
	  db->magic = 0 ;
	  aceCommandDestroy (db->look) ;
	  handleDestroy (db->handle) ;
	  if (ac_open_counter > 0)
	    {
	      ac_open_counter--;
	      if (ac_open_counter == 0)
		aceQuit(TRUE);
	    }
	}
	break ;
	
      case MAGIC_AC_KEYSET:
	{
	  AC_KEYSET aks = (AC_KEYSET) v ;
	  aks->magic = 0 ;
	  keySetDestroy (aks->ks) ;
	}
	break ;
	
      case MAGIC_AC_ITERATOR:
	{
	  AC_ITER iter = (AC_ITER)v ;
	  iter->magic = 0 ;
	  keySetDestroy (iter->ks) ;
          /* iter->ac_obj belongs to the client */
	}
	break ;
	
      case MAGIC_AC_OBJECT: /* AC_OBJ */
	{
	  AC_OBJ aobj = (AC_OBJ)v ;
	  aobj->magic = 0 ;
	  bsDestroy (aobj->obj) ;
	}
	break ;      
	
      default:
	messcrash ("Bad magic in ac_free, type = %d", type) ;
      }
  return ;
}

/********************************************************************/
/********************************************************************/
/*
 * raw command
 */

static unsigned char * ac_command_binary (AC_DB db, char *command, int *response_length, AC_HANDLE handle, BOOL bin)
{
  unsigned char *result = 0 ;
  Stack s = 0 ;

  if (!db || db->magic != MAGIC_BASE + MAGIC_AC_DB)
    messcrash ("ac_command received a null or invalid db handle") ;
  if (!command || ! *command)
    return 0 ;
  s = commandStackExecute (db->look, command) ;

  if (bin)
    {
      char *cp =  stackText (s, 0) ;
      int n = stackMark(s) ;

      cp++; n-- ;
      while (n > 0 && *cp != '\n') {cp++; n-- ;}
      cp++; n-- ;
      if (*cp == 'c') 
	{ 
	  cp++ ;
	  memcpy (&n, cp, 4) ;
	  cp += 5 ;
	  if (n > 0)
	    { 
	      result = halloc (n+1, handle) ;
	      memcpy (result, cp, n) ; 
	      result[n] = 0 ;
	    }
	  else
	    n = 0 ;
	}
       else
	 n = 0 ;
      if (response_length)
	*response_length = n ;
    }
  else
    {
      if (response_length)
	*response_length = stackMark(s) - 1;
      result = (unsigned char *) strnew (stackText (s, 0), handle) ;
    }

  stackDestroy (s) ;
  return result ;
}

unsigned char * ac_command (AC_DB db, char *command, int *response_length, AC_HANDLE handle)
{ 
  return ac_command_binary (db, command, response_length, handle, FALSE) ;
}

/********************************************************************/
/*
 * execute an ACEDB command and make a keyset of whatever the active
 * keyset is at the end of the command.  If iks is not NULL, it
 * is made the active keyset before executing the command.
 */
AC_KEYSET ac_command_keyset (AC_DB db, char *command, AC_KEYSET iks,
			     AC_HANDLE handle) 
{
  AC_KEYSET aks = 0 ;
  unsigned char *cp = 0 ;
  
  if (!db || db->magic != MAGIC_BASE + MAGIC_AC_DB)
    messcrash ("ac_command_keyset received a null or invalid db handle") ;
  if (iks)
    aceCommandSwitchActiveSet (db->look, iks->ks, 0) ;

  cp = ac_command (db, command, 0, 0) ;
  messfree (cp) ;

  aks = ac_new_keyset (db, 0, handle) ;
  aceCommandSwitchActiveSet (db->look, 0, aks->ks) ;

  return aks ;
}

/********************************************************************/
/********************* KeySet ***************************************/
/* note that ac_keyset is created on the handle 
   but not ak->ks which is private to this package
   and freed inside ac_finalise
*/

AC_KEYSET ac_new_keyset (AC_DB db, AC_OBJ obj, AC_HANDLE handle)
{
  KEYSET ks = keySetCreate () ;
  AC_KEYSET aks = (AC_KEYSET) halloc (sizeof (struct ac_keyset), handle) ;
  blockSetFinalise (aks, ac_finalise) ;

  if (!db || db->magic != MAGIC_BASE + MAGIC_AC_DB)
    messcrash ("ac_new_keyset received a null or invalid db handle") ;
  aks->db = db ;
  aks->magic = MAGIC_BASE + MAGIC_AC_KEYSET ;
  aks->ks = ks ;

  if (obj)
    ac_keyset_add (aks, obj) ;

  return aks ;
}

/********************************************************************/

AC_KEYSET ac_copy_keyset (AC_KEYSET aksold, AC_HANDLE handle)
{
  KEYSET ks = keySetCopy (aksold->ks) ;
  AC_KEYSET aks = (AC_KEYSET) halloc (sizeof (struct ac_keyset), handle) ;
  blockSetFinalise (aks, ac_finalise) ;

  aks->db = aksold->db ;
  aks->magic = MAGIC_BASE + MAGIC_AC_KEYSET ;
  aks->ks = ks ;

  return aks ;
} /* ac_keyset_copy */

/********************************************************************/
/* performs a query over the initial keyset,  creates a keyset of the result */
static AC_KEYSET ac_doquery_keyset (AC_DB db,  char *queryText, AC_KEYSET initial_keyset,
			   AC_HANDLE handle )
{
  KEYSET ks = query (initial_keyset ? initial_keyset->ks : 0, queryText) ;
  AC_KEYSET aks = (AC_KEYSET) halloc (sizeof (struct ac_keyset), handle) ;
  blockSetFinalise (aks, ac_finalise) ;

  aks->db = db ;
  aks->magic = MAGIC_BASE + MAGIC_AC_KEYSET ;
  aks->ks = ks ;

  return aks ;
} /* ac_doquery_keyset  */

/* PUBLIC
 * performs a query, creates a keyset of the result
 */
AC_KEYSET ac_dbquery_keyset (AC_DB db, char *queryText, AC_HANDLE handle)
{
  if (!db || db->magic != MAGIC_BASE + MAGIC_AC_DB)
    messcrash ("ac_dbquery_keyset received a null or invalid db handle") ;
  return ac_doquery_keyset (db, queryText, 0, handle) ;
}

AC_KEYSET ac_ksquery_keyset (AC_KEYSET initial_keyset, char *queryText, AC_HANDLE handle)
{
  return ac_doquery_keyset (initial_keyset->db, queryText, initial_keyset, handle) ;
}

AC_KEYSET ac_objquery_keyset (AC_OBJ aobj, char *queryText,  AC_HANDLE handle) 
{
 AC_KEYSET aks1 = ac_new_keyset (aobj->db, aobj, handle) , aks2 ;

  if (queryText)
    {
      aks2 = ac_doquery_keyset (aobj->db, queryText, aks1, handle) ;
      ac_free (aks1) ;

      return aks2 ;
    }
  else
    return aks1 ;
}

/********************************************************************/
/*
 * read/write keyset in non-volatile storage.  In the current
 * client implementation, name is a file name on the server's
 * disk.
 * ac_write writes a standard ace file, 
 *   the name of the file becomes the name of the keyset
 * ac_read only imports the recognised keys
 *
 * ac_read_keyset returns NULL if there is no file with that name.
 */

/* returns NULL if there is no file with that name.
 * imports the recognised keys otherwise
 */
AC_KEYSET ac_read_keyset (AC_DB db, char *fname, AC_HANDLE handle)
{ 
  AC_KEYSET aks = 0  ;
  KEYSET ks = 0 ;
  int i ;

#ifdef ACE_NCBI
  FILE *f = 0 ;
  int level ;

  if (!db || db->magic != MAGIC_BASE + MAGIC_AC_DB)
    messcrash ("ac_read_keyset received a null or invalid db handle") ;

  if (!fname || !*fname ||
      ! (f = filopen (fname, 0, "r")))
    {
      freeOutf ("// Sorry, I could not open file %s\n", fname ? fname : "NULL") ;
    }
  else
    {
      level = freesetfile (f, "") ;
      ks = keySetRead (level) ; /* will close level */
      freeclose (level) ; /* redundant but more elegant */
      
      aks = ac_new_keyset (db, 0, handle) ;
      i = keySetMax (ks) ;
      if (i) while (i--) 
	keySet (aks->ks, i) = keySet (ks, i) ;
      keySetDestroy (ks) ;
    }
#else
  ACEIN fi = 0 ;

  if (!db || db->magic != MAGIC_BASE + MAGIC_AC_DB)
    messcrash ("ac_read_keyset received a null or invalid db handle") ;

  if (!fname || !*fname ||
      ! (fi = aceInCreateFromFile (fname, "r", 0, 0)))
    {
      freeOutf ("// Sorry, I could not open file %s\n", fname ? fname : "NULL") ;
    }
  else
    {
      Stack errors = stackCreate (1000) ;
      ACEOUT fo = aceOutCreateToStack (errors, 0) ;

      ks = keySetRead (fi, fo) ;
      aceOutDestroy (fo) ;
      stackDestroy (errors) ;  /* BUG: i should possibly look at the errors */
      aceInDestroy (fi) ;

      aks = ac_new_keyset (db, 0, handle) ;
      i = keySetMax (ks) ;
      if (i) while (i--) 
	keySet (aks->ks, i) = keySet (ks, i) ;
      keySetDestroy (ks) ;
    }
#endif
    
  return aks ;
}

/********************************************************************/
/* ac_keyset_write writes a standard ace file, 
 *   the name of the file becomes the name of the keyset
 *   returns FALSE if !ks OR if the file cannot be opened
 */

BOOL ac_keyset_write (AC_KEYSET aks, char *name)
{ 
  KEYSET ks = aks->ks ;

#ifdef ACE_NCBI
  FILE *f = filopen (name, 0, "w") ;
  
  if (name && *name && aks && 
      (f = filopen (name, 0, "w")))
    {
      char *cp = name + strlen (name) - 1 ;
      while (cp > name && *cp != '/')
	cp-- ;
      if (*cp=='/') cp++ ;
      fprintf(f, "KEYSET %s\n", cp) ;
      if (keySetMax (ks))
	keySetDump (f, 0, ks) ;
      filclose (f) ;
      return TRUE ;
    }
#else
  ACEOUT fo = 0 ;
  if (name && *name && aks && 
      (fo = aceOutCreateToFile (name, "w", 0)))
    {
      char *cp = name + strlen (name) - 1 ;
      while (cp > name && *cp != '/')
	cp-- ;
      if (*cp=='/') cp++ ;
      aceOutPrint (fo, "KEYSET %s\n", cp) ;
      if (keySetMax (ks))
	keySetDump (fo, ks) ;
      aceOutDestroy (fo) ;
      return TRUE ;
    }
#endif
  return FALSE ;
}

/********************************************************************/

int ac_keyset_and (AC_KEYSET aks1, AC_KEYSET aks2 )
{
  KEYSET ks = keySetAND (aks1->ks, aks2->ks) ;

  keySetDestroy (aks1->ks) ;
  aks1->ks = ks ;

  return keySetMax (ks) ;
} /* ac_keyset_and */

/********************************************************************/

int ac_keyset_or (AC_KEYSET aks1, AC_KEYSET aks2 )
{
  KEYSET ks = keySetOR (aks1->ks, aks2->ks) ;

  keySetDestroy (aks1->ks) ;
  aks1->ks = ks ;

  return keySetMax (ks) ;
} /* ac_keyset_or */

/********************************************************************/

int ac_keyset_xor (AC_KEYSET aks1, AC_KEYSET aks2)
{
  KEYSET ks = keySetXOR (aks1->ks, aks2->ks) ;

  keySetDestroy (aks1->ks) ;
  aks1->ks = ks ;

  return keySetMax (ks) ;
} /* ac_keyset_xor */

/********************************************************************/

int ac_keyset_minus (AC_KEYSET aks1, AC_KEYSET aks2 )
{
  KEYSET ks = keySetMINUS (aks1->ks, aks2->ks) ;

  keySetDestroy (aks1->ks) ;
  aks1->ks = ks ;

  return keySetMax (ks) ;
} /* ac_keyset_minus */

/********************************************************************/

BOOL ac_keyset_add (AC_KEYSET aks, AC_OBJ aobj)
{
  KEY key = aobj->key ;

  return keySetInsert (aks->ks, key) ;
} /* ac_keyset_add */

/********************************************************************/

BOOL ac_keyset_remove (AC_KEYSET aks, AC_OBJ aobj)
{
  int i = 0, imax ;
  KEY *kp = 0, key = aobj->key ;
  BOOL found = FALSE ;
  
  imax = keySetMax (aks->ks) ;
  if (imax)
    for (i = 0, kp = arrp (aks->ks, 0, KEY) ; i < imax ; kp++, i++)
      if (key == *kp) { found = TRUE ; break ; }
  if (found)
    {
      imax -= 1 ;
      for (;i < imax ; i++, kp++)
	*kp = *(kp+1) ;
      aks->ks->max --;
    }
  return found ;
} /* ac_keyset_remove */

/********************************************************************/
/*
 * returns a subset strating at x0 of length nx
 * index start at zero and are ordered alphabetically
 * which is usefull to get slices for display
 */
AC_KEYSET ac_subset_keyset (AC_KEYSET aks, int x0, int nx, AC_HANDLE handle) 
{
  AC_KEYSET aks1 = 0  ;
  KEYSET kA ;
  int i, j ;

  aks1 = ac_new_keyset (aks1->db, 0, handle) ;
  if (aks->ks)
    {
      if (x0 < 1) 
	x0 = 1 ;
      if (nx > keySetMax (aks->ks) - x0 + 1) 
	nx = keySetMax (aks->ks) - x0 + 1 ; 
      kA = keySetAlphaHeap(aks->ks, x0 + nx - 1) ;

      for (i = x0 - 1, j = 0 ; nx > 0 ; i++, j++, nx--)
	keySet (aks1->ks, j) = keySet (kA, i) ;
      keySetSort (aks1->ks) ;
    }

  return aks1 ;
} /* ac_subset_keyset */

/********************************************************************/

int ac_keyset_count (AC_KEYSET aks)
{
  return  keySetMax (aks->ks) ;
}

/********************************************************************/
/********************************************************************/
/* Keyset Iterator functions */

AC_ITER ac_query_iter (AC_DB db, int fillhint, char *query, AC_KEYSET initial_keyset, AC_HANDLE handle) 
{ 
  KEYSET ks = 0 ;
  AC_ITER iter = (AC_ITER) halloc (sizeof (struct ac_iter), handle) ;
  blockSetFinalise (iter, ac_finalise) ;

  if (!db || db->magic != MAGIC_BASE + MAGIC_AC_DB)
    messcrash ("ac_query_iter received a null or invalid db handle") ;
  iter->db = db ;
  iter->magic = MAGIC_BASE + MAGIC_AC_ITERATOR ;
  ks = query (initial_keyset ? initial_keyset->ks : 0, query) ; 
  iter->ks = ks && keySetMax (ks) ? keySetAlphaHeap (ks, keySetMax (ks)) : keySetCreate () ;
  iter->curr = 0 ;
  iter->max = keySetMax (iter->ks) ;

  keySetDestroy (ks) ;
  return iter ;
} /* ac_query_iter */

/****************************************************************/
/* PUBLIC ac_dbquery_iter ()
 * make a query, dump resulting keyset directly into an iterator
 * Short hand 1 of previous function, fillhint = true by default
 *	db = database to query
 *	query = query to use
 */
AC_ITER ac_dbquery_iter (AC_DB db, char *query, AC_HANDLE handle)
{
  return ac_query_iter (db, TRUE, query, 0, handle) ;
} /* ac_dbquery_iter */

/****************************************************************/
/* PUBLIC ac_ksquery_iter ()
 * make a query, dump resulting keyset directly into an iterator
 * Short hand 2 of previous function, fillhint = true by default
 *	ks = keyset to query
 *	query = query to use
 */
AC_ITER ac_ksquery_iter (AC_KEYSET aks, char *query, AC_HANDLE handle)
{
  if (aks)
    return ac_query_iter (aks->db, TRUE, query, aks, handle) ;
  return 0 ;
} /* ac_ksquery_iter */

/****************************************************************/
/* PUBLIC ac_objquery_iter ()
 * make a query, dump resulting keyset directly into an iterator
 * Short hand 3 of previous function, fillhint = true by default
 *	obj = single object on which we initialise the query
 *	query = query to use
 */
AC_ITER ac_objquery_iter (AC_OBJ aobj, char *query, AC_HANDLE handle)
{
  AC_ITER iter = 0 ;
  if (aobj)
    {
      AC_KEYSET aks = ac_objquery_keyset (aobj, "IS *",  0) ;
      iter = ac_query_iter (aobj->db, TRUE, query, aks, handle) ;
      ac_free (aks) ;
    }
  return iter ;
} /* ac_objquery_iter */

/********************************************************************/

AC_ITER ac_keyset_iter (AC_KEYSET aks, int fillhint, AC_HANDLE handle) 
{ 
  AC_ITER iter = (AC_ITER) halloc (sizeof (struct ac_iter), handle) ;
  blockSetFinalise (iter, ac_finalise) ;

  iter->db = aks->db ;
  iter->magic = MAGIC_BASE + MAGIC_AC_ITERATOR ;
  iter->ks = aks->ks && keySetMax (aks->ks) ? keySetAlphaHeap (aks->ks, keySetMax (aks->ks)) : keySetCreate () ;
  iter->curr = 0 ;
  iter->max = keySetMax (iter->ks) ;

  return iter ;
} /* ac_keyset_iter */

/********************************************************************/

static AC_OBJ ac_key2obj (AC_DB db, KEY key, BOOL fillIt, AC_HANDLE handle)
{
  AC_OBJ aobj = (AC_OBJ) halloc (sizeof (struct ac_object), handle) ;
  blockSetFinalise (aobj, ac_finalise) ;

  aobj->db = db ;
  aobj->magic = MAGIC_BASE + MAGIC_AC_OBJECT ;
  aobj->key = key ;
  if (fillIt) aobj->obj = bsCreate (key) ;
  aobj->isEmpty = aobj->obj ? FALSE : TRUE ;

  return aobj ;
} /* ac_key2obj */

/********************************************************************/
/* fetch the next object in the keyset being iterated over */

AC_OBJ ac_next_obj (AC_ITER iter)
{
  /* do not ac_free (iter->ac_obj) ; it belongs to the application */
  if (iter->curr < iter->max)
    {
      iter->ac_obj = ac_key2obj (iter->db, keySet (iter->ks, iter->curr), TRUE, 0) ;
      iter->curr++ ;
      return iter->ac_obj ;
    }
  return 0 ;
} /* ac_next */

/********************************************************************/
/* resets the iterator to its top */
BOOL ac_iter_rewind (AC_ITER iter) 
{ 
  if (iter && iter->max)
    {
      ac_free (iter->ac_obj) ;
      iter->curr = 0 ;
      return TRUE ;
    }
  return FALSE ;
} /* ac_rewind  */

/********************************************************************/
/********************************************************************/
/* object related functions */

AC_OBJ ac_get_obj (AC_DB db, char *class, char *nam, AC_HANDLE handle)
{
  AC_OBJ aobj = 0 ;
  char buf[4096] ;
  KEYSET ks = 0 ;

  if (!db || db->magic != MAGIC_BASE + MAGIC_AC_DB)
    messcrash ("ac_get_obj received a null or invalid db handle") ;
  if (strlen(nam) < 3000)
    {
      sprintf (buf, "Find %s IS \"%s\"", class, nam) ;
      ks = query (0, buf) ;
    }
  else /* never use messprintf in this library */
    {
      char *cp = messalloc (30 + strlen(class) + strlen(nam)) ;
      sprintf (cp, "Find %s IS \"%s\"", class, nam) ;
      ks = query (0, cp) ;
      messfree (cp) ;
    }
  if (keySetMax (ks))
    aobj = ac_key2obj (db, keySet (ks, 0), FALSE, handle) ;
  keySetDestroy (ks) ;

  return aobj ;  
} /* ac_get */

/********************************************************************/

AC_OBJ ac_copy_obj (AC_OBJ aobj, AC_HANDLE handle)
{
  return aobj ? ac_key2obj (aobj->db, aobj->key, FALSE, handle) : 0 ;
} /* ac_dup */

/********************************************************************/

char *ac_name (AC_OBJ aobj)
{
  return aobj ? name (aobj->key) : "(NULL)" ;
} /* ac_name */

/********************************************************************/
   
char *ac_class (AC_OBJ aobj)
{
  return aobj ?  className (aobj->key) : "(NULL)" ;
} /* ac_class */

/********************************************************************/

BOOL ac_has_tag	(AC_OBJ aobj, char * tagName)
{
  KEY tag = 0 ;
  if (!lexword2key (tagName, &tag, 0))
    return FALSE ;
  
  if (!aobj)
    return FALSE ;
  else if (aobj->obj)
    return bsFindTag (aobj->obj, tag) ;
  else
    return keyFindTag (aobj->key, tag) ;
}  /* ac_has_tag */

/********************************************************************/

static BOOL ac_getTag (AC_OBJ aobj, char *tagName, KEY *tagp)
{
  if (!lexword2key (tagName, tagp, 0))
    return FALSE ;
  if (!aobj || (!aobj->obj && !keyFindTag (aobj->key, *tagp)))
    return FALSE ;
  if (!aobj->obj)
    aobj->obj = bsCreate (aobj->key) ;
  if (aobj->obj)
    return bsFindTag (aobj->obj, *tagp) ;
  
  return FALSE ; 
} /* ac_getTag */

/********************************************************************/

ac_type ac_tag_type (AC_OBJ aobj, char *tagName)
{
  /*
   * returns the type of the data after the tag, or ac_none
   * if the tag is not there or there is no data item there
   */ 
  KEY tag = 0, key = 0 ;
  
  if (ac_getTag (aobj, tagName, &tag) &&
      (key = bsType (aobj->obj, _bsRight)))
    {
      if (class(key) == _VText) return ac_type_text ;
      if (class(key)) return ac_type_obj ;
      if (key == _Int) return ac_type_int ;
      if (key == _Float) return ac_type_float ;
      if (key == _DateType) return ac_type_date ;
      if (key == _Text) return ac_type_text ;
      if (key) return ac_type_tag ;

    }
  return ac_type_empty ;
} /* ac_tag_type */

/********************************************************************/
/*
 * returns integer after the tag, or dflt if the tag
 * is not there or the data is not an integer
 * (Should it convert float?)
 */
int ac_tag_int (AC_OBJ aobj, char *tagName, int dflt)
{
  KEY tag = 0 ;
  int x = dflt ;

  if (ac_getTag (aobj, tagName, &tag))
    bsGetData (aobj->obj, tag, _Int, &x) ;
  return x ;
} /* ac_tag_int */

/********************************************************************/

float ac_tag_float (AC_OBJ aobj, char *tagName, float  dflt)
{
  KEY tag = 0 ;
  float x = dflt ;

  if (ac_getTag (aobj, tagName, &tag))
    bsGetData (aobj->obj, tag, _Float, &x) ;
  return x ;
} /* ac_tag_float */

/********************************************************************/

char *ac_tag_text (AC_OBJ aobj, char *tagName, char *dflt)
{
  KEY tag = 0, key = 0 ;
  char *cp ;
  
  if (ac_getTag (aobj, tagName, &tag))
    {
      if (bsGetKey (aobj->obj, tag, &key) &&
	  class (key) == _VText)
	return name(key) ;
      if (bsGetData (aobj->obj, tag, _Text, &cp))
	return cp ;
    }
  return dflt ;
} /* ac_tag_text */

/********************************************************************/

mytime_t ac_tag_date (AC_OBJ aobj, char *tagName, mytime_t dflt)
{
  KEY tag = 0 ;
  mytime_t x = dflt ;
  
  if (ac_getTag (aobj, tagName, &tag))
    bsGetData (aobj->obj, tag, _DateType, &x) ;
  return x ;
} /* ac_tag_float */

/********************************************************************/

AC_OBJ ac_tag_obj (AC_OBJ aobj, char *tagName, AC_HANDLE handle)
{
  KEY tag = 0, key = 0 ;

  if (ac_getTag (aobj, tagName, &tag) &&
      bsGetKey (aobj->obj, tag, &key))
    return ac_key2obj (aobj->db, key, FALSE, handle) ;
  return 0 ;
} /* ac_tag_obj */

/********************************************************************/

char * ac_tag_printable( AC_OBJ aobj, char *tagName, char *dflt )
{
  /*
   * returns the type of the data after the tag, or ac_none
   * if the tag is not there or there is no data item there
   */ 
  static char b[100];
  KEY tag = 0, key = 0 ;
  
  if (ac_getTag (aobj, tagName, &tag) &&
      (key = bsType (aobj->obj, _bsRight)))
    {
      if (class(key)) 
	{
	  KEY key2 = 0 ;
	  bsGetKey (aobj->obj, tag, &key2) ;
	  return name(key2);	/* it is an object */
	}
      else if (key == _Int)
	{ 
	  int i;
	  bsGetData (aobj->obj, tag, _Int, &i) ;
	  sprintf(b,"%d",i);
	  return b;
	}
      else if (key == _Float)
	{ 
	  float f;
	  bsGetData (aobj->obj, tag, _Float, &f) ;
	  sprintf(b,"%g",f);
	  return b;
	}
      else if (key == _DateType) 
	{ 
	  int i;
	  bsGetData (aobj->obj, tag, _DateType, &i) ;
	  return timeShow(i);
	}
      else if (key == _Text) 
	{
	  char *cp;
	  if (bsGetData (aobj->obj, tag, _Text, &cp))
	    return cp ;
	}
      else if (key) 
	return name(key);	/* it is a tag */

      /* bug: where is type bool ? */

    }
  return dflt;
} /* ac_tag_printable */

/********************************************************************/
/********************************************************************/
/* recursive  dump cell  routine */
static int ac_tagBS (AC_DB db, BS bs, BS bsm, AC_TABLE t, int row, int col)
{
  int oldrow ;
  char *cp ;

  while (bs)
    {
      oldrow = row ;
      bsModelMatch (bs, &bsm) ;

      /* dump right */
      if (bs->right)
	row = ac_tagBS (db, bs->right, bsModelRight(bsm), t, row, col + 1) ;
      for (; oldrow >= 0 && oldrow <= row ; oldrow++)
	{ /* fill in all the intermediate lines in my column */
	  if (bs->key <= _LastC)
	    ac_table_insert_text (t, oldrow, col, bsText(bs)) ;
	  else if (bs->key == _Int)
	    ac_table_insert_type (t, oldrow, col, &(bs->n), ac_type_int) ;
	  else if (bs->key == _Float)
	    ac_table_insert_type (t, oldrow, col, &(bs->n), ac_type_float) ;
	  else if (bs->key == _DateType)
	    ac_table_insert_type (t, oldrow, col, &(bs->n), ac_type_date) ;
	  else if (class(bs->key))
	    {
	      AC_OBJ aobj = ac_key2obj (db, bs->key, 0, 0) ;
	      ac_table_insert_type (t, oldrow, col, &(aobj), ac_type_obj) ;
	      ac_free (aobj) ;
	    }
	  else if (!class(bs->key)) /* a tag */
	    {
	      cp = name(bs->key) ;
	      ac_table_insert_type (t, oldrow, col, &cp, ac_type_tag) ;
	    }
	}
      /* move down, dump in the cuurent while loop */
      bs = bs->down ;	;
      if (!bs)
	break ;	
      else
	row++ ;
    }
  return row ;
}


/** fetch a table from a tag in an object   */
AC_TABLE ac_tag_table (AC_OBJ aobj, char *tagName,  AC_HANDLE handle)
{
  AC_TABLE atable = 0 ;

  if (tagName == NULL)
    {
      BS model = bsModelRoot (aobj->obj);
      atable = ac_db_empty_table (aobj->db, 1, 1, handle) ;
      if (!aobj->obj)
	aobj->obj = bsCreate (aobj->key) ;
      ac_tagBS( aobj->db, aobj->obj->root->right,
		bsModelRight(model), atable, 0, 0);
      return atable;
    }

  if (ac_tag_type (aobj, tagName) == ac_type_empty)
    return 0 ;

  atable = ac_db_empty_table (aobj->db, 1, 1, handle) ;

  if (aobj->obj->curr &&
      aobj->obj->curr->right) 
    ac_tagBS (aobj->db, aobj->obj->curr->right, bsModelRight(aobj->obj->modCurr), atable, 0, 0) ;
  
  return atable ;
} /* ac_tag_table */

/********************************************************************/
/* performs an AQL query */
static AC_TABLE ac_newAql (AC_DB db, AC_KEYSET aks, char *query, AC_HANDLE handle)
{
  AC_TABLE atable = 0 ;
  AQL aql = 0 ;
  TABLE *result = 0 ;
  int row, col, maxrow, maxcol ;
  char *cp, *types ;

  if (!aks)
    {
      aql = aqlCreate (query, NULL, 0, 0, 0, (char *)NULL) ;
      result = aqlExecute (aql, NULL, NULL, NULL, (char *)NULL) ;
    }
  else
    {
      TABLE *activeTable = tableCreateFromKeySet (aks->ks ? aks->ks : keySetCreate(), 0);
      aql = aqlCreate (query, 0, 0, 0, 0, "@active", (char *)NULL) ; 
      result = aqlExecute (aql,  NULL, NULL, NULL, "TABLE @active", activeTable, (char *)NULL);
      tableDestroy (activeTable);
    }

  if (!(aqlIsError(aql)))
    { 
      atable = ac_db_empty_table (db, maxrow = tableMax (result), maxcol = result->ncol, handle) ;
      types = tableTypes(result) ;
      for (row = 0 ; row < maxrow ; row++)
	for (col = 0 ; col < maxcol ; col++)
	  {
 	    if (tabEmpty(result,row,col))
	      {
		/* if the cell is empty, we should not insert any data - it will remain ac_type_empty */
	      }
	    else
	      {
		switch (types[col])
		  {
		  case 'k': case 'K':
		    {
		      AC_OBJ aobj = ac_key2obj (db, (tabKey (result, row, col)), 0, 0) ;
		      ac_table_insert_type (atable, row, col, &(aobj), ac_type_obj) ;
		      ac_free (aobj) ;
		    }
		    break ;
		  case 'g':
		    cp = name(tabTag (result, row, col)) ;
		    ac_table_insert_type (atable, row, col, &cp, ac_type_tag) ;
		    break ;
		  case 'b':
		    ac_table_insert_type (atable, row, col, &(tabBool (result, row, col)), ac_type_bool) ;
		    break ;
		  case 'i':
		    ac_table_insert_type (atable, row, col, &(tabInt (result, row, col)), ac_type_int) ;
		    break ;
		  case 'f':
		    ac_table_insert_type (atable, row, col, &(tabFloat (result, row, col)), ac_type_float) ;
		    break ;
		  case 't':
		    ac_table_insert_type (atable, row, col, &(tabDate (result, row, col)), ac_type_date) ;
		    break ;
		  case 's':
		    cp = tabString (result, row, col) ;
		    ac_table_insert_text (atable, row, col, cp) ;
		    break ;
		  }
	      }
	  }
    }
  else
    {
      freeOutf ("\n// Aql: error %d : %s\n", 
		aqlGetErrorNumber(aql), aqlGetErrorMessage(aql)) ;
    }
  aqlDestroy (aql) ;
  tableDestroy (result) ;

  return (AC_TABLE)atable ;
} /* ac_newAql */

/********************************************************************/

AC_TABLE ac_aql_table (AC_DB db, char *query, AC_KEYSET initial_keyset, AC_HANDLE handle)
{
  if (!db || db->magic != MAGIC_BASE + MAGIC_AC_DB)
    messcrash ("ac_aql_table received a null or invalid db handle") ;
  
  return ac_newAql (db, initial_keyset, query, handle) ;
} /* ac_aql_table */

/********************************************************************/
#if 0
char * ac_keyname (AC_DB db, KEY key)
{
  return name (key) ;
} /* ac_db_keyname */
#endif
/********************************************************************/
/********************************************************************/

/* BUG not implemented */
/*
 * Table maker related functions
 *
 * Three constructors:
 *	tablemaker query
 *	aql query
 *	extract data from object
 */
/*
enum ac_tablemaker_where
{ 
ac_tablemaker_db, 
ac_tablemaker_file, 
ac_tablemaker_server_file, 
ac_tablemaker_text 
};
*/
/*
 * AC_TABLEmaker_db => query is name of tablemaker query in database
 * AC_TABLEmaker_file => query is file name known by server
 *		Not a particularly secure feature
 * AC_TABLEmaker_text => query is the actual text of a tablemaker
 *	query that you read into your source code.
 */

AC_TABLE ac_tablemaker_table (AC_DB db, char *queryNam, AC_KEYSET initial_keyset,
	 enum ac_tablemaker_where where,
	 char *parameters , AC_HANDLE handle)
{
#if 0
  char **error_messages = 0 ;
  char *error_message = 0 ;
  AC_HANDLE h = ac_new_handle () ;
  vTXT txt = vtxtHandleCreate (h) ;
  Stack defStack = 0 ;
  KEY tableDefKey = 0 ;

  if (iks)
    aceCommandSwitchActiveSet (db->look, iks->ks, 0) ;
  /* client version
  switch (where)
    {
    case ac_tablemaker_file:
      vtxtPrintf ("table %s -C -f %s %s"
		  , initial_keyset ? "-active" : ""
		  , queryNam
		  , parameters ? parameters : ""
		  ) ;
      break ;
    case ac_tablemaker_db:
      vtxtPrintf ("table %s -C -n %s %s"
		  , initial_keyset ? "-active" : ""
		  , queryNam
		  , parameters ? parameters : ""
		  ) ;
      break ;
    default:
      messerror (" BUG tablemaker is not yet not implemented for acinside.c ") ;
      break ;
    }
  ac_command_keyset (db, vtxtPtr (txt), iks, h) ; 
  */
  switch (where)
    {
    case ac_tablemaker_file:
      
      spread = spreadCreateFromFile (queryNam, "r", parameters);
      if (!spread)
	{
	  error_message = "// Sorry, could not read table definitions file %s\n" ;
	  goto abort ;
	}
      break ;
    case ac_tablemaker_text:
      defStack = stackHandleCreate (h) ;
      stackTextOnly (defStack) ;
      pushText (defStack, queryNam) ;
      spread = spreadCreateFromStack (defStack, parameters);
      if (!spread)
	{
	  error_message = "// Sorry, Bad table definition passed to ac_tablemaker" ;
	  goto abort ;
	}
      break ;
    case ac_tablemaker_db:
      if (lexword2key (queryNam, &tableDefKey, _VTable))
	spread = spreadCreateFromObj (tableDefKey, tableParams);
      if (!spread)
	{
	  error_message = "// Sorry, could not read table definitions obj %s\n" ;
	  goto abort ;
	}
      break ;
    }

  if (iks)
    {	      
      spread->precompute = FALSE ; /* cannot store precomputed 
				    * table, if active keyset is 
				    * used for master column */
      spread->isActiveKeySet = TRUE ;
      ksTmp = spreadFilterKeySet (spread, iks->ks) ;
      kA = keySetAlphaHeap(ksTmp, keySetMax(ksTmp)) ;
      keySetDestroy (ksTmp) ;

    }
  else
    kA = spreadGetMasterKeyset (spread) ;

  /* ace9 : they have lost the capacity to compute a little slice 	
     spreadCalculateOverKeySet (spread, kA, &last) ;
     gives a table in  look->spread->values 
     
     tableSliceOut(look->dump_out,
     0, tableMax(look->spread->values),
     look->spread->values,
     '\t', look->beauty) ;
  */
  spreadRecomputeKeySet (spread, kA, 0, 0, keySetMax (kA)) ;
  /* now we must convert spread->table into AceC */ 
  {
    TABLE *tt = tableCreate () ;
    spreadTable2Tableau (spread, tt) ;
  }

  spreadDestroy (spread) ;
  if (*error_messages) *error_messages = error_message ;
  ac_free (h) ;
#endif
  return ac_empty_table (1, 1, handle) ;
}
/*
 * perform tablemaker query:
 *	db is the database
 * 	query is the query
 * 	where describes how to interpret the value of query
 *	parameters are parameters to substitute in the query
 */

/********************************************************************/
/********************************************************************/
/********************************************************************/


/*
 * PUBLIC ac_keyset_table
 *
 * Create a 1 column table containing all objects in the keyset.
 */

AC_TABLE ac_keyset_table (AC_KEYSET ks, int min, int max, int fillhint, AC_HANDLE handle )
{
  AC_TABLE tbl;
  AC_ITER i;
  int x;

  if (max == -1)
    max = ac_keyset_count(ks);

  tbl = ac_empty_table(max, 1, handle);

  /*
   * create an iterator.  we cheat by poking our min value
   * into the iterator at the start.  We can do this because
   * the read-ahead buffer is still empty.
   */
  i = ac_keyset_iter (ks, fillhint, 0);
  i->curr = min;


  /*
   * fetch objects from min to max and put them in the table.
   */
  x = min;
  while (x < max)
    {
      AC_OBJ o;
      o = ac_next_obj (i);
      if (!o)
        break;  /* never happens */
      ac_table_insert_type (tbl, x, 0, &o, ac_type_obj);
      ac_free(o);
      x++;
    }

  ac_free(i);

  return tbl;
}



/* x1 x2 in biolog coords [1,max] */
char *ac_zone_dna (AC_OBJ aobj, int x1, int x2, AC_HANDLE h)
{
  Stack s = 0 ;
  Array dna = dnaGet (aobj->key) ;
  char* result = 0;

  if (!dna) return 0 ;
  if (!x1 && !x2) { x1 = 1 ; x2 = arrayMax (dna) ; }
  if (x1 > x2)
    {
      reverseComplement (dna) ;
      x1 = arrayMax(dna) - x1 + 1 ;
      x2 = arrayMax(dna) - x2 + 1 ;
    }
  if (dna && x1 >= 1 && x2 >= 1 && x1 <= arrayMax(dna) && x2 <= arrayMax(dna) && x1 != x2)
    {
      dnaDecodeArray (dna) ;
      arr (dna, x2, char) = 0 ;
      result = strnew (arrp (dna, x1 - 1, char), h) ;
    }
  arrayDestroy (dna) ;
  stackDestroy (s) ;
  return result ;
}

char *ac_obj_dna (AC_OBJ aobj, AC_HANDLE h)
{
  return ac_zone_dna (aobj, 0, 0, h) ;
}

char *ac_obj_peptide (AC_OBJ aobj, AC_HANDLE h)
{
  Array pep = peptideGet (aobj->key) ;
  int nn = pep ? arrayMax (pep) : 0 ;
  char* result = 0;

  if (nn)
    {
      pepDecodeArray (pep) ;
      result =  strnew (arrp (pep, 0, char), h) ;
    }
  arrayDestroy (pep) ;
  return result ;
}

char *ac_longtext (AC_OBJ aobj, AC_HANDLE h)
{
  Stack s = aobj ? stackGet (aobj->key) : 0 ;
  char *cp, *cq = 0 ;

  if (s)
    {
      cq = halloc (stackMark (s) + 10, h) ;
      stackCursor(s,0) ;
      while ((cp = stackNextText(s)))
	 {
	   sprintf(cq, "%s\n", cp) ; /* \n replaces one or more zero, so we do not go over size */
	   cq += strlen (cq) ;
	 }
    }
  return cq ;
}

unsigned char *ac_a_data (AC_OBJ aobj, int *len, AC_HANDLE h) 
{
  /*
   * INCOMPLETE unless you add code in dump.c !!
   * nothing will get exported except for DNA Peptide
   *
   * other type A object
   *
   * If the object is of type A :
   *  - The returned value is the an array of bytes, allocated on handle
   *  - If len is not NULL, it is filled in with the number of bytes.
   *  - There is a \0 after the returned string (not included in the length).
   * Otherwise:
   *  - returns NULL and *len is undefined.
   *
   * The content of of the returned array is not specified, in
   * practice it is "whatever show -C does" and usually contains
   * formating characters that should be cleansed in the client.
   *	
   * Properly speaking, this should not be considered as a public function
   * but as a startup point if you want to handle a new type of A data.
   * A data types are declared on the server side in whooks/quovadis.c
   * and require registering dedicated code. In the same way one
   * should register here new AceC code to handle the specifics of that
   * class.
   */
  unsigned char *buf = 0 ;
  char *command ;

  if (aobj && pickType (aobj->key) == 'A')
    {
      command = hprintf (0, "query find %s IS \"%s\"", className (aobj->key), name(aobj->key)) ;
      buf = ac_command (aobj->db, command, len, h) ;
      messfree (buf) ;
      messfree (command) ;
      buf = ac_command_binary (aobj->db, "show -C", len, h, TRUE) ;
    }
  return buf ;
}

BOOL ac_parse (AC_DB db,
	       char *text,
	       char **error_messages,
	       AC_KEYSET *nks,
	       AC_HANDLE h)
{
  AC_KEYSET ks;
  BOOL success = TRUE;

  if (!db || db->magic != MAGIC_BASE + MAGIC_AC_DB)
    messcrash ("ac_parse received a null or invalid db handle") ;
  if (! sessionGainWriteAccess())
    {
      if (error_messages)
	*error_messages = strnew ("You do not have write access", 0) ;
      return FALSE ;
    }
  
  ks = ac_new_keyset (db, NULL, h);
  success = parseAceCBuffer (text, db->command_stack, 0, ks->ks) ;
  
  if (strstr( stackText(db->command_stack, 0), "error") )
    success = FALSE;
  
  if (error_messages)
    *error_messages = strnew( stackText(db->command_stack, 0), h);
  
  if (nks)
    *nks = ks;
  else
    ac_free(ks);
  
  return success;
}

