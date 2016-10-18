/*  File: method.c
 *  Author: Richard Durbin (rd@sanger.ac.uk)
 *  Copyright (C) J Thierry-Mieg and R Durbin, 1994
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
 * Description: generic methods for sequence displays
 * Exported functions:
 * HISTORY:
 * Last edited: Dec 10 10:03 2009 (edgrif)
 * * Jun 24 13:37 1998 (edgrif): Fixed initialisation bug in methodInitialise,
 *      isDone was not being set. Made methodInitialise an internal routine.
 *	Inserted missing call to methodInitialise in method routine.
 * Created: Wed Jun  1 15:05:00 1994 (rd)
 * CVS info:   $Id: method.c,v 1.32 2009-12-10 10:04:29 edgrif Exp $
 *-------------------------------------------------------------------
 */

#include <wh/acedb.h>
#include <wh/method.h>
#include <wh/lex.h>
#include <wh/bs.h>
#include <whooks/classes.h>
#include <whooks/tags.h>
#include <whooks/systags.h>
#include <wh/colours.h>
#include <wh/parse.h>

/****** global information array *******/

Array methodInfo = 0 ;

/****** keys used here - do they need to be global? *****/

static KEY _CDS_colour ;
static KEY _Width ;
static KEY _Frame_sensitive ;
static KEY _Strand_sensitive ;
static KEY _Show_up_strand ;
static KEY _Blastn ;
static KEY _Blixem ;
static KEY _Blixem_X ;
static KEY _Blixem_N ;
static KEY _Blixem_P ;
static KEY _Belvu ;
static KEY _Bumpable ;
static KEY _Cluster ;
static KEY _Score_bounds ;
static KEY _Score_by_offset ;
static KEY _Score_by_width ;
static KEY _Score_by_histogram ;
static KEY _Percent ;
static KEY _Right_priority ;
static KEY _EMBL_dump ;
static KEY _Show_text;
static KEY _No_display ;
static KEY _Min_mag;
static KEY _Max_mag;
static KEY _Display_gaps;
static KEY _Join_blocks ;
static KEY _Allow_misalign ;
static KEY _Allow_clipping ;
static KEY _Map_gaps ;
static KEY _Coords ;
static KEY _String ;


/* Internal routines.  */
static magic_t METHOD_MAGIC = "METHOD";

static void methodInitialise (void) ;
static METHOD *methodCreate (KEY methodKey, char *methodName,
			     STORE_HANDLE handle);
static void methodFinalise (void *block);

static void methodInitFromObj (METHOD *meth, OBJ obj);
static int methodSaveToObj (METHOD *meth, OBJ obj);



/******************************************************************
 ************************* public functions ***********************
 ******************************************************************/


/* OLD CODE TO BE REMOVED SOON........NEEDS REPLACING IN JUST A FEW FILES */
/* methodAdd ()
 * General routine to handle cached information on methods in methodInfo
 * Will always ensure METHOD *method(method) exists
 *
 * Return values:
 * 0 if previously DONE
 * 1 if new and not in database (set to defaults)
 * 2 if new and read from database
 */
int methodAdd (KEY methodKey)
{
  METHOD *meth;
  OBJ obj ;
  char *text;
  KEY k;

  methodInitialise() ;					    /* Make sure we are initialised. */

  methodKey = lexAliasOf(methodKey);			    /* use canonical key */

  if (class(methodKey) != _VMethod)
    messcrash ("methodAdd() - methodKey not in class 'Method'");

  if (!methodInfo)
    /* create cache structure */
    methodInfo = arrayCreate (lexMax(_VMethod), METHOD*) ;

  if (!(meth = array (methodInfo, KEYKEY(methodKey), METHOD*)))
    {
      meth = (METHOD*) messalloc (sizeof(METHOD)) ;
      array (methodInfo, KEYKEY(methodKey), METHOD*) = meth;
    }

  if (meth->flags & METHOD_DONE)
    return 0 ;

  if (!(obj = bsCreate (methodKey))) 
    return 1 ;

  /* reset to defaults */
  meth->flags &= ~METHOD_CALCULATED ;/* unset flag */
  meth->flags |= METHOD_DONE ;	/* set flag */
  meth->no_display = FALSE ;
  meth->colour = 0 ;
  meth->CDS_colour = 0 ;
  meth->upStrandColour = 0 ;
  meth->width = 0.0 ;
  meth->symbol = '\0' ;
  meth->priority = 0.0 ;
  meth->minScore = meth->maxScore = 0.0 ;

  meth->key = methodKey;

  if (bsGetKeyTags (obj, _Colour, &k))
    meth->colour = k - _WHITE ;				    /* colour-tag to colour-enum */
  if (bsGetKeyTags(obj, _CDS_colour, &k))
    meth->CDS_colour = k - _WHITE ;			    /* colour-tag to colour-enum */
  bsGetData (obj, _Width, _Float, &meth->width) ;
  bsGetData (obj, _Right_priority, _Float, &meth->priority) ;
  if (bsGetData (obj, _Symbol, _Text, &text))
    meth->symbol = *text;
  if (bsFindTag (obj, _Frame_sensitive))
    meth->flags |= METHOD_FRAME_SENSITIVE ;
  if (bsFindTag (obj, _Strand_sensitive))
    meth->flags |= METHOD_STRAND_SENSITIVE ;
  if (bsFindTag (obj, _Show_up_strand))
    {
      meth->flags |= METHOD_SHOW_UP_STRAND ;
      if (bsGetKeyTags(obj, _Show_up_strand, &k))
	meth->upStrandColour = k - _WHITE ;		    /* colour-tag to colour-enum */
    }
  if (bsFindTag (obj, _Blastn))
    meth->flags |= METHOD_BLASTN ;
  if (bsFindTag (obj, _Blixem_X))
    meth->flags |= METHOD_BLIXEM_X ;
  if (bsFindTag (obj, _Blixem_N))
    meth->flags |= METHOD_BLIXEM_N ;
  if (bsFindTag (obj, _Blixem_P))
    meth->flags |= METHOD_BLIXEM_P ;
  if (bsFindTag (obj, _Belvu))
    meth->flags |= METHOD_BELVU ;
  if (bsFindTag (obj, _Bumpable))
    meth->flags |= METHOD_BUMPABLE ;
  if (bsFindTag (obj, _Cluster))
    meth->flags |= METHOD_CLUSTER ;
  if (bsFindTag (obj, _EMBL_dump))
    meth->flags |= METHOD_EMBL_DUMP ;
  if (bsFindTag (obj, _Percent))
    {
      meth->flags |= METHOD_PERCENT ;
      meth->minScore = 25 ;	/* so actual display is linear */
      meth->maxScore = 100 ;	/* can override explicitly */
    }

  if (bsFindTag (obj, _Allow_misalign))
    meth->flags |= METHOD_ALLOW_MISALIGN ;
  if (bsFindTag (obj, _Allow_clipping))
    meth->flags |= METHOD_ALLOW_CLIPPING ;
  if (bsFindTag (obj, _Map_gaps))
    meth->flags |= METHOD_MAP_GAPS ;
  if (bsFindTag (obj, _Coords))
    meth->flags |= METHOD_EXPORT_COORDS ;
  if (bsFindTag (obj, _String))
    meth->flags |= METHOD_EXPORT_STRING ;

  if (bsGetData (obj, _Score_bounds, _Float, &meth->minScore)
      && bsGetData (obj, _bsRight, _Float, &meth->maxScore))
    {
      if (bsFindTag (obj, _Score_by_offset))
	meth->flags |= METHOD_SCORE_BY_OFFSET ;
      else if (bsFindTag (obj, _Score_by_width))
	meth->flags |= METHOD_SCORE_BY_WIDTH ;
      else if (bsFindTag (obj, _Score_by_histogram))
	{ 
	  meth->flags |= METHOD_SCORE_BY_HIST ;
	  meth->histBase = meth->minScore ;
	  bsGetData (obj, _bsRight, _Float, &meth->histBase) ;
	}
    }

  if (bsFindTag (obj, _Show_text))
    meth->isShowText = TRUE;
  else
    meth->isShowText = FALSE;

  if (bsFindTag(obj, _No_display))
    meth->no_display = TRUE;
  else
    meth->no_display = FALSE;

  meth->minMag = 0;
  bsGetData (obj, _Min_mag, _Float, &meth->minMag) ;

  meth->maxMag = 0;
  bsGetData (obj, _Max_mag, _Float, &meth->maxMag) ;

  bsDestroy (obj) ;
  return 2 ;
} /* methodAdd */




METHOD *method (KEY methodKey)
     /* old code to be removed soon */
{
  METHOD *meth = 0;

  methodInitialise() ;		/* Make sure we are initialised. */

  methodKey = lexAliasOf(methodKey) ;

  if (methodKey && 
      (!methodInfo ||
       KEYKEY(methodKey) >= arrayMax(methodInfo) ||
       !(meth = arr(methodInfo, KEYKEY(methodKey), METHOD *))))
    {
      methodAdd (methodKey) ;
      meth = arr(methodInfo, KEYKEY(methodKey), METHOD *) ;
    }
  
  /* !! WARNING: Can be NULL !! */
  return meth ;
} /* method */





#ifdef ED_G_NEVER_INCLUDE_THIS_CODE

/* I CAN'T FIND ANYWHERE WHERE THIS IS CALLED, SO LETS EXCLUDE IT... ED_G    */
void methodSet (char *name, int col, unsigned int flags, float priority,
		float width, char symbol, float min, float max)
     /* old code to be removed soon */
{
  KEY key ;
  METHOD *meth ;

  methodInitialise() ;		/* Make sure we are initialised. */

  lexaddkey (name, &key, _VMethod) ;

  if (methodAdd (key) == 1)
    /* only set here if not in database */
    {
      /* made in methodAdd */
      meth = arr (methodInfo, KEYKEY(key), METHOD*) ;
      
      meth->flags = flags;
      meth->colour = col ;
      meth->priority = priority ;
      meth->width = width ;
      meth->symbol = symbol ;
      meth->minScore = min ;
      meth->maxScore = max ;
    }

  return ;
} /* methodSet */
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */



BOOL methodExists (METHOD *meth)
     /* Determine whether the METHOD* pointer is valid
      *  NOTE: every method is somehow represented
      *  by a database object and therefore 
      *  has a KEY in class Method
      * RETURNS
      *   TRUE if a valid non-NULL pointer
      *   otherwise FALSE
      *
      * public function - user-code wants to verify pointers too.
      */
{

  if (meth
      && meth->magic == &METHOD_MAGIC
      && meth->key != 0	
      && class(meth->key) == _VMethod 
      && meth->name != NULL)
    /* we could also check that name(meth->key) .eq. meth->name */
    return TRUE;

  return FALSE;
} /* methodExists */

/************************************************************/

METHOD *methodCreateFromObj (KEY methodKey, STORE_HANDLE handle)
     /* Loads the method of a given KEY into the struct,
      *  if the obj is in the database.
      *  all the fields are set according to the tags/values
      *  It is made sure that meth->key will be set.
      * RETURNS:
      *  the new struct (alloc'd on handle)
      *    OR
      *  NULL if the object doesn't exist in the DB.
      */
{
  METHOD *meth = NULL;
  OBJ obj;

  methodKey = lexAliasOf(methodKey); /* use canonical key */

  if ((obj = bsCreate (methodKey)))
    /* will fail if methodKey is an empty object */
    {
      meth = methodCreate (methodKey, name(methodKey), handle);

      methodInitFromObj (meth, obj);
      
      bsDestroy (obj) ;

    }
  else if (iskey(methodKey) == 1)
    /* object with methodKey exists, but empty */
    {
      /* all other code assumes that we can make a METHOD-struct
       * if the key is valid, so we have to return at least
       * an empty initialised struct */
      meth = methodCreate (methodKey, name(methodKey), handle);
    }

  if (meth && !methodExists(meth))
    messcrash("methodCreateFromObj() - created struct that is invalid, "
	      "check methodCreate() and methodExists()");

  return meth;
} /* methodCreateFromObj */

/************************************************************/

/* Tries to find the Method of the given name in
 *  the database. If it doesn't exist, the piece of ace-file 
 *  is prepended with the method-name and parsed into the 
 *  database, which will create the object.
 *  In either case we can then create the METHOD from the object
 *  on the given handle (see methodCreateFromObj)
 * RETURNS:
 *  the METHOD struct as created from the object, whether
 *   the object had to be created or was already in the database.
 *   That struct pointer is NEVER NULL!
 * NOTE:
 *  in the worst case where the object didn't exist and aceText
 *  is an empty string, the struct will be totally uninitialised.
 */
METHOD *methodCreateFromDef (char *methodName, char *aceText, STORE_HANDLE handle)
{
  KEY methodKey;
  KEYSET parsedKeySet;
  METHOD *meth;
  /* no methodInitialise, couldn't get past the assertions without it */

  if (!methodName || strlen(methodName) == 0)
    messcrash("methodCreateFromDef() - NULL or empty methodName");
  if (!aceText)
    messcrash("methodCreateFromDef() - aceText == NULL");

  /* try to get the KEY */
  if (!lexword2key (methodName, &methodKey, _VMethod))
    {
      char *parse_string ;

      parse_string = hprintf(0, "Method : %s\n%s", methodName, aceText) ;

      /* object doesn't exist yet - make the object
       * in the database and initialsie using the ace-file
       * snippet given as a string */
      parsedKeySet = keySetCreate();

      if (!(parseBufferInternal(parse_string, parsedKeySet, FALSE)))
	messcrash("methodCreateFromDef() - failed parse of Method obj \"%s\", "
		  "parse string was: \"%s\"", methodName, parse_string) ;

      if (keySetMax(parsedKeySet) != 1)			    /* Belt and braces ?? */
	messcrash("methodCreateFromDef() - wrong number of obj created: %d, "
		  "parse string was: \"%s\"", keySetMax(parsedKeySet), parse_string) ;
      keySetDestroy(parsedKeySet);

      /* now get the new KEY after we've parsed */
      if (!lexword2key (methodName, &methodKey, _VMethod))
	messcrash ("methodCreateFromDef() - could not find key for Method obj \"%s\", "
		   "parse string was: \"%s\"", methodName, parse_string) ;

      messfree(parse_string) ;
    }

  /* at this point methodKey is guaranteed to be correct */

  meth = methodCreateFromObj (methodKey, handle);

  if (!meth)
    messcrash("methodCreateFromDef() - couldn't create METHOD from obj");

  /* we have guaranteed now that we never return NULL */
  return meth;
} /* methodCreateFromDef */

/************************************************************/

void methodDestroy (METHOD *meth)
{
  /* just a stub to call messfree, to initiate the finalisation */
  messfree (meth);
} /* methodDestroy */

/************************************************************/

BOOL methodSave (METHOD *meth)
     /* Writes all the data contained in (meth) to an object
      *  meth->key which is in class Method.
      *  All data in an existing object will be overwritten.
      * RETURNS:
      *	   TRUE if we could save the data.
      *    FALSE if we couldn't open the object for update.
      */
{
  OBJ obj;
  int num_errors;
  /* no methodInitialise, couldn't get past the assertions without it */

  if (!methodExists(meth))
    messcrash ("methodSave() - received bad meth pointer");

  if (!(obj = bsUpdate (meth->key))) /* object may be blocked etc. */
    /* I'm not sure whether we should insist on write access */
    {
      messerror ("Failed to update object Method:%s", meth->name);
      return FALSE;
    }

  num_errors = methodSaveToObj (meth, obj);

  bsSave (obj);

  if (num_errors > 0)
    messerror ("%d errors occurred whilst updating object Method:%s",
	       num_errors, name (meth->key));

  return TRUE;
} /* methodSave */


/**********************************************************/

char *methodGetDiff (METHOD *meth, STORE_HANDLE handle)
     /* When method structs have been changed in memory
      *  we want to kep track of those changes without
      *  changing their respective database objects.
      *  That would create too many versions of similar Method objs.
      *  instead we want to store a diff of the struct versus its 
      *  DB-object.
      * RETURNS:
      *	 A string (allocated upon handle) which describes an ace-diff
      *  between the corresponding actual DB object and the struct,
      *  in the way that if the string was parsed for that object,
      *	 it would create an object, which correspeonds exactly to the 
      *  given METHOD struct.
      */
{
  /* We'll need a new function for this (rd)
   *     char *bsDiff (OBJ src, OBJ dest, STORE_HANDLE handle)
   * which will output an ace-diff that if applied to 'src' would
   * result in 'dest'.
   * Therefore methodGetDiff() would create a temporary object
   * from the given METHOD struct 'meth' which always has the same
   * name (probably using unprintable characters). That OBJ
   * would become 'dest' and we would bsCreate the original
   * object (from meth->key) to get the OBJ 'src'.
   * Then we can bsDestroy(src) and bsKill(dest)
   * and return the string we got from bsDiff().
   * This string would be stored under a ?Text tag in the ?View
   * class together with the ?Method key of the method
   * so the fmap can re-load METHODs from those view's and recreate 
   * the original METHOD struct at the ime of saving the View.
   */

  /* XXXXXXXXX stub XXXXXXXXX */
  return strnew("", handle);
} /* methodGetDiff */

/************************************************************/


METHOD *methodCopy (METHOD *meth, char *aceDiff, STORE_HANDLE handle)
     /* makes a new METHDO struct from an old one, and applies
      * the diff in the process. if aceDiff is NULL, this
      * function just acts like a copy.
      * if aceDiff contains some ace-file directives they'll be 
      * applied to the new struct before returning.
      * RETURNS:
      *  the copied method struct (alloc'd on the handle)
      */
{
  METHOD *new_meth;
  OBJ obj;

  if (!methodExists(meth))
    messcrash("methodCopy - received in valid meth struct");

  new_meth = methodCreate (meth->key, meth->name, handle);

  new_meth->remark = strnew(meth->remark, 0);

  new_meth->flags = meth->flags;

  new_meth->colour = meth->colour;
  new_meth->CDS_colour = meth->CDS_colour;
  new_meth->upStrandColour = meth->upStrandColour;
  new_meth->minScore = meth->minScore;
  new_meth->maxScore = meth->maxScore;
  new_meth->minMag = meth->minMag;
  new_meth->maxMag = meth->maxMag;
  new_meth->width = meth->width;
  new_meth->symbol = meth->symbol;
  new_meth->priority = meth->priority;
  new_meth->histBase = meth->histBase;
  new_meth->isShowText = meth->isShowText ;
  new_meth->no_display = meth->no_display ;

  if (aceDiff && strlen(aceDiff) > 0 
      && (obj = bsCreateCopy (new_meth->key)))
    {
      methodSaveToObj (new_meth, obj);
      /* bsApplyDiff (obj, aceDiff); */
      methodInitFromObj (new_meth, obj);
      
      bsDestroy(obj);
    }

  return new_meth;
} /* methodCopy */



/******************************************************************
 ************************ private functions ***********************
 ******************************************************************/


static void methodInitialise (void)
     /* This is an internal routine, code using the method package does 
      * not need to worry about initialisation, all interface routines 
      * call this routine to make sure the package is initialised. 
      * However, some routine don't need to call this because
      * the entry conditions (messcrash assertions) couldn't
      * be TRUE without having called a function that calls
      * the initialisation anyway.
      */
{ 
  static int isDone = FALSE ;

  if (isDone == FALSE)
    {
      lexaddkey ("CDS_colour", &_CDS_colour, 0) ;
      lexaddkey ("Width", &_Width, 0) ;
      lexaddkey ("Frame_sensitive", &_Frame_sensitive, 0) ;
      lexaddkey ("Strand_sensitive", &_Strand_sensitive, 0) ;
      lexaddkey ("Show_up_strand", &_Show_up_strand, 0) ; /* mieg */
      lexaddkey ("Blastn", &_Blastn, 0) ;
      lexaddkey ("Blixem", &_Blixem, 0) ;
      lexaddkey ("Blixem_X", &_Blixem_X, 0) ;
      lexaddkey ("Blixem_N", &_Blixem_N, 0) ;
      lexaddkey ("Blixem_P", &_Blixem_P, 0) ;
      lexaddkey ("Belvu", &_Belvu, 0) ;
      lexaddkey ("Bumpable", &_Bumpable, 0) ;
      lexaddkey ("Cluster", &_Cluster, 0) ;
      lexaddkey ("Score_bounds", &_Score_bounds, 0) ;
      lexaddkey ("Score_by_offset", &_Score_by_offset, 0) ;
      lexaddkey ("Score_by_width", &_Score_by_width, 0) ;
      lexaddkey ("Score_by_histogram", &_Score_by_histogram, 0) ;
      lexaddkey ("Percent", &_Percent, 0) ;
      lexaddkey ("Right_priority", &_Right_priority, 0) ;
      lexaddkey ("Show_text", &_Show_text, 0) ;
      lexaddkey ("No_display", &_No_display, 0) ;
      lexaddkey ("Min_mag", &_Min_mag, 0) ;
      lexaddkey ("Max_mag", &_Max_mag, 0) ;
      lexaddkey ("Display_gaps", &_Display_gaps, 0) ;
      lexaddkey ("Join_blocks", &_Join_blocks, 0) ;
      lexaddkey ("Allow_misalign", &_Allow_misalign, 0) ;
      lexaddkey ("Allow_clipping", &_Allow_clipping, 0) ;
      lexaddkey ("Map_gaps", &_Map_gaps, 0) ;
      lexaddkey ("Export_coords", &_Coords, 0) ;
      lexaddkey ("Export_string", &_String, 0) ;


      isDone = TRUE  ;
    }

  return ;
} /* methodInitialise */

/************************************************************/

static METHOD *methodCreate (KEY methodKey, char *methodName,
			     STORE_HANDLE handle)
     /* creates new METHOD struct on the given handle
      * in a consistent way so the returned struct will pass the
      * test of methodExists().
      *
      * This function is private as it shouldn't be called by
      * user-code - Method's used by user-code should always
      * be initialised somehow - either from an existing object
      * or from a new object created by a piece of aceText.
      */
{
  METHOD *meth;
  methodInitialise() ;		/* Make sure we are initialised. */

  if (class(methodKey) != _VMethod)
    messcrash("methodCreate() - methodKey not in class Method");

  if (!methodName || strlen(methodName) == 0)
    messcrash("methodCreate() - NULL or empty methodName");

  meth = (METHOD*)halloc (sizeof(METHOD), handle);
  blockSetFinalise (meth, methodFinalise);
  meth->magic = &METHOD_MAGIC;

  meth->key = lexAliasOf(methodKey);
  meth->name = strnew(name(meth->key), 0); /* free'd in methodFinalise */

  /* set some default values for the new method */
  
  meth->flags &= METHOD_CALCULATED;  /* unset flag */
  meth->colour = 0 ;
  meth->CDS_colour = 0 ;
  meth->upStrandColour = 0 ;
  meth->width = 0.0 ;
  meth->symbol = '\0' ;
  meth->priority = 0.0 ;
  meth->minScore = meth->maxScore = 0.0 ;
  meth->no_display = FALSE ;

  meth->isCached = FALSE;

  /* the meth-struct is now guaranteed to be valid */
  return meth;
} /* methodCreate */

/************************************************************/

static void methodInitFromObj (METHOD *meth, OBJ obj)
{
  char *text;
  KEY k;
  /* no methodInitialise, couldn't get here without it */

  /* The following code was mostly copied from methodAdd, 
   * which may become obsolete at some point. */
 
  messfree (meth->remark);
  if (bsGetData (obj, _Remark, _Text, &text))
    meth->remark = strnew(text, 0); /* free'd in methodFinalise */
  if (bsGetKeyTags (obj, _Colour, &k))
    meth->colour = k - _WHITE ;				    /* colour-tag to colour-enum */
  if (bsGetKeyTags (obj, _CDS_colour, &k))
    meth->CDS_colour = k - _WHITE ;			    /* colour-tag to colour-enum */
  bsGetData (obj, _Width, _Float, &meth->width) ;
  bsGetData (obj, _Right_priority, _Float, &meth->priority) ;
  if (bsGetData (obj, _Symbol, _Text, &text))
    meth->symbol = *text;
  if (bsFindTag (obj, _Frame_sensitive))
    meth->flags |= METHOD_FRAME_SENSITIVE ;
  if (bsFindTag (obj, _Strand_sensitive))
    meth->flags |= METHOD_STRAND_SENSITIVE ;
  if (bsFindTag (obj, _Show_up_strand))
    {
      meth->flags |= METHOD_SHOW_UP_STRAND ;
      if (bsGetKeyTags(obj, _Show_up_strand, &k))
	meth->upStrandColour = k - _WHITE ;		    /* colour-tag to colour-enum */
    }
  if (bsFindTag (obj, _Blastn))
    meth->flags |= METHOD_BLASTN ;
  if (bsFindTag (obj, _Blixem_X))
    meth->flags |= METHOD_BLIXEM_X ;
  if (bsFindTag (obj, _Blixem_N))
    meth->flags |= METHOD_BLIXEM_N ;
  if (bsFindTag (obj, _Blixem_P))
    meth->flags |= METHOD_BLIXEM_P ;
  if (bsFindTag (obj, _Belvu))
    meth->flags |= METHOD_BELVU ;
  if (bsFindTag (obj, _Bumpable))
    meth->flags |= METHOD_BUMPABLE ;
  if (bsFindTag (obj, _Cluster))
    meth->flags |= METHOD_CLUSTER ;
  if (bsFindTag (obj, _EMBL_dump))
    meth->flags |= METHOD_EMBL_DUMP ;
  if (bsFindTag (obj, _Display_gaps))
    meth->flags |= METHOD_DISPLAY_GAPS ;
  if (bsFindTag (obj, _Join_blocks))
    meth->flags |= METHOD_JOIN_BLOCKS ;
  if (bsFindTag (obj, _Percent))
    { 
      meth->flags |= METHOD_PERCENT ;
      meth->minScore = 25 ;	/* so actual display is linear */
      meth->maxScore = 100 ; /* can override explicitly */
    }
  
  if (bsFindTag (obj, _Allow_misalign))
    meth->flags |= METHOD_ALLOW_MISALIGN ;
  if (bsFindTag (obj, _Allow_clipping))
    meth->flags |= METHOD_ALLOW_CLIPPING ;
  if (bsFindTag (obj, _Map_gaps))
    meth->flags |= METHOD_MAP_GAPS ;
  if (bsFindTag (obj, _Coords))
    meth->flags |= METHOD_EXPORT_COORDS ;
  if (bsFindTag (obj, _String))
    meth->flags |= METHOD_EXPORT_STRING ;

  if (bsGetData (obj, _Score_bounds, _Float, &meth->minScore)
      && bsGetData (obj, _bsRight, _Float, &meth->maxScore))
    {
      if (bsFindTag (obj, _Score_by_offset))
	meth->flags |= METHOD_SCORE_BY_OFFSET ;
      else if (bsFindTag (obj, _Score_by_width))
	meth->flags |= METHOD_SCORE_BY_WIDTH ;
      else if (bsFindTag (obj, _Score_by_histogram))
	{ 
	  meth->flags |= METHOD_SCORE_BY_HIST ;
	  meth->histBase = meth->minScore ;
	  bsGetData (obj, _bsRight, _Float, &meth->histBase) ;
	}
    }

  if (bsFindTag (obj, _Show_text))
    meth->isShowText = TRUE;
  else
    meth->isShowText = FALSE;
  
  if (bsFindTag (obj, _No_display))
    meth->no_display = TRUE;
  else
    meth->no_display = FALSE;

  meth->minMag = 0;
  bsGetData (obj, _Min_mag, _Float, &meth->minMag) ;
  
  meth->maxMag = 0;
  bsGetData (obj, _Max_mag, _Float, &meth->maxMag) ;
  meth->flags |= METHOD_DONE;  /* set flag (finished setting data) */
  
  return;
} /* methodInitFromObj */

/************************************************************/

static int methodSaveToObj (METHOD *meth, OBJ obj)
     /* Write all fields of METHOD struct into 
      *  OBJ. The obj has to have been opened for update
      *  and the caller needs to save/destroy it afterwards.
      *  errors will not terminate the updating, merely increase a counter
      * RETURNS:
      *  the number of errors that occured.
      */
{
  int num_errors = 0;

  if (!methodExists(meth))
    messcrash ("methodSaveToObj() - received bad meth pointer");

  if (!obj)
    messcrash ("methodSaveToObj() - received NULL obj");

  /******************************************************
   * replace all data within the object with the values *
   * of the METHOD structure                            *
   ******************************************************/

  if (bsFindTag (obj, _Remark))
    bsRemove (obj);
  if (meth->remark)
    {
      if (!bsAddData (obj, _Remark, _Text, meth->remark))
	++num_errors;
    }

  if (bsFindTag(obj, _Display))
    /* kill all information about how to display the Method,
     * This saves us from removing every tag individually */
    bsRemove (obj);

  /*if (meth->colour) colour in enum may be 0==WHITE */
    {
      if (bsAddTag (obj, _Colour) && bsPushObj (obj))
	bsAddTag (obj, (meth->colour + _WHITE));
      else
	++num_errors;
    }

  /*if (meth->CDS_colour) colour in enum may be 0==WHITE */
    {
      if (bsAddTag (obj, _CDS_colour) && bsPushObj (obj))
	bsAddTag (obj, (meth->CDS_colour + _WHITE));
      else
	++num_errors;
    }

  if (meth->flags & METHOD_FRAME_SENSITIVE)
    {
      if (!bsAddTag (obj, _Frame_sensitive))
	++num_errors;
    }

  if (meth->flags & METHOD_STRAND_SENSITIVE)
    {
      if (!bsAddTag (obj, _Strand_sensitive))
	++num_errors;
      else
	{
	  if (meth->flags & METHOD_SHOW_UP_STRAND)
	    {
	      if (!bsAddTag (obj, _Show_up_strand))
		++num_errors;
	      else
		{
		  if (meth->upStrandColour && bsPushObj(obj))
		    bsAddTag(obj, meth->upStrandColour);
		}
	    }
	}
    }

  if (meth->minScore != 0 || meth->maxScore != 0)
    /* the logic in methodCreateFromObj seems to suggest that 
     * scoreXxxx-fields are only needed, if score_bounds are used */
    {
      /* set score_bounds */
      if (bsAddData (obj, _Score_bounds, _Float, &meth->minScore) &&
	  bsAddData (obj, _bsRight, _Float, &meth->maxScore))
	{
	  /* add tags for score mode */
	  if (meth->flags & METHOD_SCORE_BY_OFFSET)
	    {
	      if (!bsAddTag (obj, _Score_by_offset))
		++num_errors;
	    }
	  else if (meth->flags & METHOD_SCORE_BY_WIDTH)
	    {
	      if (!bsAddTag (obj, _Score_by_width))
		++num_errors;
	    }
	  else if (meth->flags & METHOD_SCORE_BY_HIST)
	    {
	      if (!bsAddTag (obj, _Score_by_histogram))
		++num_errors;
	      else
		{
		  if (meth->histBase != 0.0 &&
		      !bsAddData (obj, _bsRight, _Float, &meth->histBase))
		    ++num_errors;
		}
	    }
	    
	}
      else
	++num_errors;
    }

  if (meth->flags & METHOD_PERCENT)
    /* Percent tag may be set without use of score_bounds
     * I guess this may be useful so score_bounds will
     * be initialised when the object is read in next time */
    {
      if (!bsAddTag (obj, _Percent))
	++num_errors;
    }

  /* overlap mode */
  if (meth->flags & METHOD_CLUSTER)
    {
      if (!bsAddTag (obj, _Cluster))
	++num_errors;
    }
  else if (meth->flags & METHOD_BUMPABLE)
    {
      if (!bsAddTag (obj, _Bumpable))
	++num_errors;
    }

  if (meth->width != 0.0)
    {
      if (!bsAddData(obj, _Width, _Float, &meth->width))
	++num_errors;
    }

  if (meth->symbol)		/* Note, only one character */
    {
      char buf[2];
      buf[0] = meth->symbol;
      buf[1] = '\0';

      if (!bsAddData (obj, _Symbol, _Text, buf))
	++num_errors;
    }

  if (meth->priority != 0)
    {
      if (!bsAddData (obj, _Right_priority, _Float, &meth->priority))
	++num_errors;
    }

  if (meth->minMag != 0)
    {
      if (!bsAddData (obj, _Min_mag, _Float, &meth->minMag))
	++num_errors;
    }

  if (meth->maxMag != 0)
    {
      if (!bsAddData (obj, _Max_mag, _Float, &meth->maxMag))
	++num_errors;
    }

  if (meth->isShowText)
    {
      if (!bsAddTag (obj, _Show_text))
	++num_errors;
    }

  if (meth->no_display)
    {
      if (!bsAddTag (obj, _No_display))
	++num_errors;
    }

  if (bsFindTag (obj, _Blastn))
    bsRemove (obj);
  if (meth->flags & METHOD_BLASTN)
    {
      if (!bsAddTag (obj, _Blastn))
	++num_errors;
    }

  /* Blixem */
  if (bsFindTag (obj, _Blixem))
    bsRemove(obj);
  if (meth->flags & METHOD_BLIXEM_X)
    {
      if (!bsAddTag (obj, _Blixem_X))
	++num_errors;
    }
  if (meth->flags & METHOD_BLIXEM_N)
    {
      if (!bsAddTag (obj, _Blixem_N))
	++num_errors;
    }
  if (meth->flags & METHOD_BLIXEM_P)
    {
      if (!bsAddTag (obj, _Blixem_P))
	++num_errors;
    }

  if (meth->flags & METHOD_BELVU)
    {
      if (!bsAddTag (obj, _Belvu))
	++num_errors;
    }
  else
    /* flag unset - remove tag */
    if (bsFindTag (obj, _Belvu))
      bsRemove(obj);

  return num_errors;
} /* methodSaveToObj */

/************************************************************/

static void methodFinalise (void *block)
     /* clear up memory allocated for fields inside the method-struct */
{
  METHOD *meth = (METHOD*)block;

  if (!methodExists(meth))
    messcrash("methodFinalise() - received invalid METHOD block");

  /* make the fields in this struct invalid, before the struct 
   * itself is free'd - so another attempt to call this function
   * on the same pointer will cause the above assertion to fail */
  meth->key = 0;
  messfree (meth->name);
  meth->magic = 0;
  meth->flags = 0;		/* do not leave any flags set */

  return;
} /* methodFinalise */



/*************************** end of file *******************************/
