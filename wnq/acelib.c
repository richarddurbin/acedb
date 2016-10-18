/*  File: acelib.c
 *  Author: Jean Thierry-Mieg (mieg@crbm.cnrs-mop.fr)
 *  Copyright (C) J Thierry-Mieg and R Durbin, 1997
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
 *	Jean Thierry-Mieg (CRBM du CNRS, France) mieg@kaa.cnrs-mop.fr
 *
 * SCCS: $Id: acelib.c,v 1.34 2007-04-02 09:32:41 edgrif Exp $
 * Description: public programming interace to acedb
 * Exported functions:
 * HISTORY:
 * Last edited: Mar 30 14:49 2007 (edgrif)
 * Created: Sun Nov  9 23:02:41 1997 (rd)
 *-------------------------------------------------------------------
 */

/* First try at actual implementation of the public
ace interface

in this implemantation i suppose that i will link this
code into xace and use it to write say the genetic map

so i do not yet care about the ACE context which is the default

In a separate implementation, i will put all this in a clint
code and then have to open the database etc

*/

#include "acelib_.h"		/* private types for this module */
#include "aceio.h"
#include "lex.h"
#include "dna.h"
#include "whooks/systags.h"

#include "session.h"
#include <wh/log.h>
#include "pick.h"
#include "model.h"
#include "parse.h"
#include "bindex.h"
#include "status.h"
#include "query.h"

#include <ctype.h>  /* isdigit */

/*******************************************************************/

/* these decls shouldn't be necessary of we could include acelib.h
   here */
Bool aceQuit(BOOL commit);
BOOL aceCrash(void) ;
BOOL aceSetContext(AceContext* ace);
BOOL aceCloseContext(Bool commit);
#define aceFree(_vp) (uAceFree(_vp), _vp = 0)
BOOL uAceFree (void* vp);

/*******************************************************************/

static BOOL aceMayWrite = TRUE ;
static AceHandle activeHandle = 0 ;
static AceContext* activeContext = 0 ;
static Array aceContextArray = 0 ;
static AceHandle sessionHandle = 0 ;
static int aceClassNumber(char *classname) ;

static void aceSetErrorMessage(char* buffer) ;
static Array pushArrayOnHandle (Array a, AceHandle hh) ;
static KEYSET pushKeySetOnHandle (KEYSET ks,AceHandle hh) ;

static Bool doTheQuit(BOOL commit, BOOL crash) ;

/*******************************************************************/
/*********************  error handling *****************************/

int aceErrno = 0 ;
static char *activeErrorBuf = 0 ;

char*  aceErrorMessage(int* replyLength)
{
  char * cp = activeErrorBuf && *activeErrorBuf ? activeErrorBuf : "ok" ;
  
  if (!activeContext)
    cp = "No active context, please call:\n\t"
      "aceOpenContext(\"clientSignature\")\n"
      " before any other operation, merci." ;

  if (replyLength)
    *replyLength = strlen(cp) ;

  return strnew(cp,activeHandle) ;
}

static void aceSetErrorMessage(char* buffer)
{
  messfree (activeErrorBuf) ;
  if (!buffer || !*buffer || strlen(buffer) > 10000)
    activeErrorBuf = strnew ("ok", 0) ;
  else
    { if (!aceErrno) aceErrno = 1 ;
      activeErrorBuf = strnew (buffer, 0) ;
    }
}

/*******************************************************************/
/*******************  session managment ****************************/

void aceStatus (void)
{
  ACEOUT fo = aceOutCreateToStdout (0);
  tStatus (fo) ;
  aceOutDestroy (fo);
}

/*********/

/* called by acedbCrash() in case of a messcrash
 * and acedbExit() in case of a messExit to clean-up.
 * this is usually overridden by the user to provide their
 * own clean-up routine that may allow the user to save their work */
static void defaultAceCrashRoutine (void)
{ 
  aceCrash() ;						    /* do not save, just relase locks */

  fprintf (stderr,"// NB: changes made since the last explicit save "
	              "have NOT been saved\n") ;

  return ;
}

/*********/

Bool aceInit (char *ace_path)
{
  extern VoidRoutine messcrashroutine;

  sessionInit (ace_path) ;

  freesetfile(stdin,"") ; /* detach */

  sessionHandle = handleCreate() ;
  aceContextArray = arrayHandleCreate (20, AceContext*, sessionHandle) ;
  array(aceContextArray,0,AceContext*) = 0 ; /* avoid returning a zero */
  
  /* called by acedbCrash (which is called by messcrash)
     this function will perform the aceQuit procedures
     in case of a crash to close the ace-context & database down */
  messcrashroutine = defaultAceCrashRoutine;

  return TRUE ;
}

/*******************************************************************/

BOOL   aceGetWriteAccess(Bool doGet) /* if get=true, get it if poss.,
					if get=false: test if we 
					              could get it */
{
  CHECKACE ;

  if (!aceMayWrite)
    {
      aceSetErrorMessage("You cannot gain write access "
			 "after it has been permantly forbidden.") ;
      return FALSE ;
    }

  if (doGet)			/* get if possible */
    {
      if (sessionGainWriteAccess ())
	return TRUE ;
      aceSetErrorMessage("Sorry, you cannot gain write access.") ;
      return FALSE ;
    }
  else				/* just report if it would be possible */
    {
      return writeAccessPossible() ;
    }
}

/*******************************************************************/

Bool  aceForbidWriteAccess(void)    /* final, none of your context will have it */
{
  CHECKACE ;

  sessionReleaseWriteAccess();
  sessionForbidWriteAccess();

  aceMayWrite = FALSE ;
  return TRUE ;
}

/*******************************************************************/

BOOL aceCommit(BOOL releaseLock )
{
  CHECKACE  ;
  
  aceGetWriteAccess (TRUE) ; /* try to grab it */
  if (isWriteAccess())
    {
      sessionClose (TRUE) ; /* cannot fail */ 
      if (!releaseLock) 
	aceGetWriteAccess (TRUE) ; 
      return TRUE ;
    }
  aceSetErrorMessage("Sorry, you do not have write access") ;
  return FALSE ;
}


/*******************************************************************/


/* complementary calls to aceInit() */
/* in case of a messcrash, the defaultAceCrashRoutine()
   or (whatever messcrashroutine, there is at the moment)
   is called instead by acedbCrash(), to cleanup
   tmp-files, read/write locks etc. */
Bool aceQuit(BOOL commit)
{
  BOOL result ;

  result = doTheQuit(commit, FALSE) ;

  return result ;
}

BOOL aceCrash(void)
{
  BOOL result ;

  result = doTheQuit(FALSE, TRUE) ;

  return result ;
}


/*******************************************************************/
/************************** Context managment **********************/

AceContext* aceOpenContext(char* clientSignature)
{ /* database allready initialised */
  AceContext* ace ;

  if (!clientSignature || !*clientSignature)
    {
      aceSetErrorMessage ("aceOpenContext called with null clientSignature") ;
      return 0 ;
    }
  activeHandle = handleHandleCreate (sessionHandle) ;
  ace = halloc(sizeof(struct aceContextStruct),activeHandle) ;
  activeContext = ace ;
  ace->handle = activeHandle ;

  ace->clientSignature = strnew(clientSignature,ace->handle) ;
  ace->magic = &ACEMAGIC ;
  ace->readLockListe = listeCreate (ace->handle) ;
  ace->writeLockListe = listeCreate (ace->handle) ;
  ace->instanceListe = listeCreate (ace->handle) ;
  ace->instanceId = 0 ;

  ace->id = arrayMax(aceContextArray) ;
  array(aceContextArray,ace->id, AceContext*) = ace ; 
  return  assVoid(ace->id) ;
}

/*******************************************************************/

BOOL aceSetContext(AceContext* ace)
{ 
  int i = assInt(ace) ;
  if (i<= 0 || !aceContextArray || i >= arrayMax(aceContextArray))
     {
       aceSetErrorMessage ("aceSetContext: unknown context, sorry") ;
       return FALSE ;
     }
  ace = array(aceContextArray, i, AceContext*) ;
  if (!ace ||  ace->magic != &ACEMAGIC)
     {
       aceSetErrorMessage ("aceSetContext: obsolete context, sorry") ;
       return FALSE ;
     }
  
  activeContext = ace ;
  activeHandle = ace->handle ;
  return TRUE ;
}

/*******************************************************************/

AceContext* aceGetContext(void)
{
  if (activeContext && activeContext->magic == &ACEMAGIC)
    return assVoid(activeContext->id) ;
  if (activeContext)
    messcrash ("aceGetContext, bad magic") ;
  aceSetErrorMessage ("aceGetContext: no context, sorry") ;
  return 0 ;
}

/*******************************************************************/

Bool acePushContext(void)
{ 
  int from ;
  CHECKACE ;

  from = activeContext->id ;
  if (aceOpenContext(activeContext->clientSignature))
    {
      activeContext->pushedFrom = from ;
      return TRUE ;
    }
  aceSetErrorMessage ("acePushContext: accidental failure, please report it, sorry") ;
  return FALSE ;
}
/*******************************************************************/

Bool acePopContext(Bool commit)
{   
  int from ;
  CHECKACE ;

  from = activeContext->pushedFrom ;
  aceCloseContext (commit) ;
  if (from)
    return aceSetContext(assVoid(from)) ;
  aceSetErrorMessage 
    ("acePopContext: you can only pop out of a pushed context, sorry") ;
  return FALSE ;
}

/*******************************************************************/

BOOL aceCloseContext(Bool commit)
{ 
  AceInstance ai ;
  void *vp = 0 ;
  CHECKACE ;
  
  while (listeNext(activeContext->instanceListe, &vp))
    {
      ai = (AceInstance) vp ;
      if (ai)
	{
	  if (ai->lock == INSTANCE_WRITE_LOCK) 
	    bsSave(ai->obj) ; /* resets ai->obj = 0 */
	  aceFree(ai) ;
	}
    }

  activeContext->magic = 0 ;
  array(aceContextArray,activeContext->id,AceContext*) = 0 ;
  aceFree (activeHandle) ;
  activeHandle = 0 ; activeContext = 0 ;
  return TRUE ;
}

/*********************************************************************/
/******************** memory managment *******************************/

AceHandle aceHandleCreate(AceHandle hh) 
{
  CHECKACE2 ;
  return handleHandleCreate (h1) ;
}

/*********************************************************************/

Array uAceArrayCreate (int dim, int size)
{
  return uArrayCreate_dbg(dim, size, activeHandle,__FILE__,__LINE__) ;
}

/*********************************************************************/

BOOL uAceFree (void* vp)
{
  CHECKACE ;
  messfree(vp) ;
  return TRUE ;
}

/**********************************************************************/
/******************* Database Administration **************************/
 
/* Public operations */
/* reads-commit a file, possible if you have write access */

#ifdef NEEDS_REWRITING_TO_WORK_WITH_ACEIN_SO_THIS_FUNCTION_ISNT_NECESSARY
KEYSET aceParseFile(FILE* f, AceHandle hh)
{
  KEYSET ks = 0 ;  
  aceSetErrorMessage("aceParseFile : this function is dead") ;

  CHECKACE2 ;

  if (!f)
    {
      aceSetErrorMessage("aceParseFile : called on null file handle") ;
      return 0 ;
    }

  ks = arrayHandleCreate(100,KEY,h1) ;
  /* last arg means parseKeepGoing = TRUE */
  parseFile(f, 0, ks, TRUE) ;        /* will close f */
  return ks ;
} /* aceParseFile */
#endif

/* new unique objects */
AceKey aceMakeKeyFromFormat(char* classname, char* format)
{
  char *cp = format ;
  int i, classe = -1 ;
  KEY key, kk ;
  static Stack localStack = 0 ;

  classe = aceClassNumber(classname) ;
  if (classe < 0)
    { 
      aceSetErrorMessage(messprintf ("// Bad classname %s", classname)) ;
      return 0 ;
    }
  if (pickList[classe].protected)
    { 
      aceSetErrorMessage(messprintf ("// Class %s is protected", classname)) ;
      return 0 ;
    }

  if (!cp) 
    { 
      aceSetErrorMessage(messprintf ("// Usage New Class Format: Construct a new name"
				     " according to C-language printf(format,n) \n"
				     "// Example: New Oligo xxx.\\%%d\n")) ;
      return 0 ;
    }
  if (!aceGetWriteAccess (FALSE) )
    { 
      aceSetErrorMessage("// Sorry, you do not have write access") ;
      return 0 ;
    }
  localStack = stackReCreate(localStack, 80) ;
  pushText(localStack, cp) ;
  
  i = -2 ; /* important flag */
  cp = stackText(localStack,0) ;
  while (*cp && *cp != '%')
  cp++ ;
  if (*cp++ == '%')
    { 
      while(*cp && isdigit((int)*cp)) cp++ ;  
      if (*cp++ == 'd')
	{ i = 1 ;
	while (*cp && *cp != '%') cp++ ;
	if (!*cp)
	  goto okformat ;
	}
      aceSetErrorMessage ("// Only allowed format should contain zero or once %%[number]d") ;
      return 0 ;
    }
okformat:
  key = kk = 0 ;
  cp = stackText(localStack,0) ;
  while (i < lexMax(classe) + 3 && /* obvious format obvious problem */
	 !(kk = lexaddkey(messprintf(cp,i), &key, classe)))
    if (i++ < 0) break ;  /* so i break at zero f format has no %d */
  if (kk)
    return key ;
  aceSetErrorMessage (messprintf ("Name %s allready exists\n", cp)) ;
  return 0 ;
}

/* usupported operations */
Bool     aceAllowBlocking(Bool canBlock)   /* always fail */
{   
  aceSetErrorMessage("aceAllowBlocking: usupported operation, sorry") ;
  return FALSE ; 
} 

/* Priviledged operations */
/* ask the database you are connected to to re-read its models */
BOOL aceReadModels(void)
{
  CHECKACE ;
  aceCommit (FALSE) ;    /* need to do that first */
  if (readModels())
    return TRUE ; 
  else
    aceSetErrorMessage("Failed to read models, sorry") ;
  return FALSE ;
}

Bool  aceAddUpdate (Bool all)  /* read acedb sequential update files */
{
  aceSetErrorMessage("Update command unimplemented, sorry") ;
  return FALSE ;
}


/***************************************************************************/
/******************* Class inforrmation **********************************/

/* list the ace classes */
AceStringSet aceClasses(AceHandle hh)  /* a null-terminated list of C-strings */
{
  Array aa = 0 ;
  int i, j = 0 ;
  CHECKACE2 ;

  aa = arrayHandleCreate (20, Array,h1) ;
  for (i=0, j=0 ; i < 256 ; i++)  
    if(pickType(KEYMAKE(i,0)))
      array(aa, j++, char*) = strnew(pickClass2Word(i), h1) ;
  return aa ;
}

/*********************************************************************/

static int aceClassNumber(char *classname)
{
  int i = pickWord2Class(classname) ;

   if (i)
     return i ;
   if (!strcmp(classname,"System"))
     return 0 ;
   return -1 ;
}

/*********************************************************************/

Bool aceClassSize (char* classname, int* np)  /* number of instances in class */
{
  int i = aceClassNumber(classname) ;
  CHECKACE ;

  if (!np) return FALSE ;
  if (i >= 0)
     { int n = lexMax(i) ;
       if(pickList[i].type == 'B')
	 n-- ;
       n-- ;
       *np = n ;
     return TRUE ;
     }
    return FALSE ; 
}

/*********************************************************************/

KEYSET aceClassExtent(char* classname,AceHandle hh)
{ 
  int i ;
  KEYSET ks = 0 , ks1 ;
  CHECKACE2 ;
  
  i = aceClassNumber(classname) ;
  if (i >= 0) /* recognized class */
    {
      ks1 = query(0,messprintf("FIND %s",classname)) ;
      ks = pushKeySetOnHandle(ks1, h1) ;
      keySetDestroy(ks1) ;
      return ks ;
    }
  aceSetErrorMessage(messprintf
	  ("aceClassExtent: unrecognized classname %s",classname)) ;
  return 0 ;
}

/*********************************************************************/
/***************************** Names **********************************/

char* aceName(KEY key)   /* may say "nullkey.." but never returns 0 */
{
  CHECKACE ;
  return name(key) ; /* to copy is too costly */
}

/*********************************************************************/

char* aceClassName (KEY key)
{
  CHECKACE ;
  return className(key) ; /* copied, so completelly stable */
}


/*********************************************************************/
/* return a valid printable object name for a known class */

static BOOL aceCheckObjName(char *cin, char **crp)
{
  /* note that this returns messalloced memory */
  char *cr = lexcleanup(cin, 0) ;
  if (!cr || !strlen(cr))
    return FALSE ;
  *crp = cr ;
  return TRUE ;
}

char* aceComposeName
(char* classname, char* instance, AceHandle hh)
{
  char *cr ;
  CHECKACE2 ;

  if (aceClassNumber (classname) < 0)
    {
      aceSetErrorMessage
	(messprintf("aceComposeName: bad class name %s",classname)) ;
      return 0 ;
    }
  if (!aceCheckObjName (instance, &cr))
    {
      aceSetErrorMessage
	(messprintf("aceComposeName: bad instance name %s",instance)) ;
      return 0 ;
    }
  return strnew(messprintf("%s:%s",classname,cr), h1) ;
}

/*********************************************************************/

AceTag aceTag(char* tagName, BOOL create)  /*look up a string in the system lex*/
{
  KEY k = 0 ;
  CHECKACE ;
  
  if (create)
    {
      char *cr="" ; /* clean tag name */

      if (!isWriteAccess())
	{
	  aceSetErrorMessage("aceTag called with create=true when you do not have write access") ;
	  return FALSE ;
	}
      lexaddkey(cr,&k,_VSystem) ;
      return  k;
    }
  else
    {
      if (lexword2key(tagName,&k,_VSystem))
	return k ;
      aceSetErrorMessage(messprintf("Tag %s unknown", tagName)) ;
      return FALSE ;
    }
}

/*********************************************************************/

static BOOL fullNameSplit(char **cpp, char **cqq, char *ff)
{
  char *cp = 0, *cr = 0 ;
  if (!ff || !*ff)
    return FALSE ;
  cp = ff ;
  while (cp && *cp != ':') cp++ ;
  if (*cp != ':' || !*(cp+1))
    return FALSE ;
  *cp++ = 0 ;
  
  if (aceClassNumber (ff) < 0 ||
      !aceCheckObjName (cp, &cr))
    return FALSE ;
  if (aceClassNumber (ff) < 0)
    {
      aceSetErrorMessage
	(messprintf("fullNameSplit: bad class name %s",ff)) ;
      return FALSE ;
    }
  if (!aceCheckObjName (cp, &cr))
    {
      aceSetErrorMessage
	(messprintf("fullNameSplit: bad instance name %s",cp)) ;
      return FALSE ;
    }

  *cpp = ff ;
  *cqq = cr ;

  return TRUE ;
}

/*********************************************************************/
 /*look up a string in the system lex*/
AceKey aceKey(char* fullKeyName, BOOL create) 
{
  KEY k = 0 ;
  int cc = 0 ;
  char* cp, *cq, *ff = strnew(fullKeyName,0) ; /* copy to be allowed to insert 0 */
  CHECKACE ;
  
  if (!fullNameSplit(&cp,&cq,ff) || (cc = aceClassNumber(cp)) < 0)
    { messfree (ff) ; return 0 ; } /* error message set by fulnamesplit */
  
  if (create)
    {
      if (!isWriteAccess())
	{
	  aceSetErrorMessage("aceKey called with create=true when you do not have write access") ;
	  return FALSE ;
	}
      lexaddkey(cq,&k,cc) ;
      return  k;
    }
  else
    {
      if (lexword2key(cq,&k,cc))
	return k ;
      aceSetErrorMessage(messprintf("%s unknown", fullKeyName)) ;
      return FALSE ;
    }
}  

  
KEYSET aceKeySet (AceStringSet names, BOOL create, AceHandle hh)
{
  int i, j=0 ;
  KEYSET ks = 0 ;
  KEY key ;

  CHECKACE2;

  ks = arrayHandleCreate(10,KEY,h1) ;
  for (i=0 ; i < arrayMax(names) ; i++)
    {
      key = aceKey(array(names,i,char*), create) ;
      if (key)
       keySet(ks,j++) = key ;
      else
       break ;
    }
  if (i != j)
    keySetDestroy (ks) ;
  
  return ks ;
}

/**************************************************************************/
/***************************** Data Access ********************************/
/**************************************************************************/
/* instance wrapper */
static void instanceFinalize(void *vp)
{
  Instance ins = (Instance) vp ;
  if (ins && ins->magic == &INSMAGIC)
    {
      bsDestroy(ins->obj) ; ins->obj = 0 ;
      listeRemove (activeContext->instanceListe, ins, ins->slot) ;
      ins->magic = 0 ;
    }
} /* instanceFinalize */

static Instance instanceCreate(KEY key, OBJ obj, AceHandle handle) 
{
  Instance ins;

  ins = (Instance)halloc(sizeof(struct instanceStruct), handle) ;
  blockSetFinalise(ins,instanceFinalize) ;

  ins->magic = &INSMAGIC ;
  ins->obj = obj ;
  ins->id = activeContext->id ; 
  ins->key = key ;
  ins->slot = listeAdd (activeContext->instanceListe, ins) ;

  return ins ;
} /* instanceCreate */

/*********************************************************************/
/* readlock is too simple minded because we should count the number
 * of read locks, (fw - a reference counter)
 */

#define READLOCKSTATUS (unsigned char)128

static void aceDropReadLock (KEY key) 
{
  int n = listeFind (activeContext->readLockListe, assVoid(key)) ;
  if (n)
    {
      listeRemove (activeContext->readLockListe, assVoid(key), n) ;
      lexUnsetStatus (key,LOCKSTATUS) ; /* should count  */
    }
}

Bool aceReadLockKey(AceKey key)
{
  unsigned char cc ;
  CHECKACE ; 
  if (!key) return 0 ;
  key = lexAliasOf(key) ;
  if (pickType(key) != 'B')
    { aceSetErrorMessage
	(messprintf("aceReadLockKey called on non B key %s:%s",
		    className(key), name(key))) ;
    return FALSE ;
    }
  if (KEYKEY(key) == 0)
    { aceSetErrorMessage
	(messprintf("aceReadLockKey called on non protected key %s:%s",
		    className(key), name(key))) ;
    return FALSE ;
    }
  
  
  cc = lexGetStatus (key) ;
  if (cc & LOCKSTATUS)
      { 
        if (listeFind (activeContext->readLockListe, assVoid(key)) )
	  return TRUE ;
	aceSetErrorMessage
	  (messprintf("aceReadLockKey: key %s:%s is allready write-locked, sorry",
		      className(key),name (key))) ;
	return FALSE ;
      }
	
  lexSetStatus (key,LOCKSTATUS) ;
  listeAdd (activeContext->readLockListe, assVoid(key)) ;
  
  return TRUE ;
}

Bool aceReadLockKeySet(AceKeySet ks) 
{
  int i, n ;
  KEYSET ks1 = 0 ;
  KEY key ;
  CHECKACE ;

  if(!keySetExists(ks))
    {
      aceSetErrorMessage("aceReadLockKeySet, bad keySet");
      return FALSE ;
    }
  n = keySetMax(ks) ;
  ks1 = keySetCopy (ks) ;
  for (i=0; i < n ; i++)
    {
      key = keySet (ks,i) ;
      if (listeFind (activeContext->readLockListe, assVoid(key)) )
	keySet (ks1,i) = 0 ;
      else if (!aceReadLockKey(key))
	goto abort ;
    }
  keySetDestroy (ks1) ;
  return TRUE ;
abort:
  while (i--)
    {
      key = keySet (ks1,i) ;
      if (key)
	aceDropReadLock (key) ;
    }

  keySetDestroy (ks1) ;
  return FALSE ;
}

Bool aceReadLockHandle(AceHandle h) 
{
  CHECKACE ;
  aceSetErrorMessage("unimplemented, sorryy") ;
  return FALSE ;
}

Bool aceReadLockInstance(AceInstance ai) 
{
  CHECKACE ;
  return aceReadLockKey (ai->key) ;
}     

Bool aceReadLockInstanceSet(AceInstanceSet ais) 
{
  int i ;
  KEYSET ks ;
  BOOL bb ;
  CHECKACE ;
  
  ks = keySetCreate () ;
  for (i=0; i < arrayMax(ais) ; i++)
    keySet (ks,i) = arr(ais,i,AceInstance)->key ;
  bb = aceReadLockKeySet (ks) ;
  keySetDestroy (ks) ;
  /* set ai->lock */
  return bb ;
}

Bool aceWriteLockKey(AceKey key)
{
  unsigned char cc ;
  CHECKACE ; 

  /* ?? should we test for isWriteAccess ??, i think not */
  if (!key) return 0 ;
  key = lexAliasOf(key) ;
  if (pickType(key) != 'B')
    { aceSetErrorMessage
	(messprintf("aceWriteLockKey called on non B key %s:%s",
		    className(key), name(key))) ;
    return FALSE ;
    }
  if (KEYKEY(key) == 0)
    { aceSetErrorMessage
	(messprintf("aceWriteLockKey called on non protected key %s:%s",
		    className(key), name(key))) ;
    return FALSE ;
    }
  
  
  cc = lexGetStatus (key) ;
  if (cc & LOCKSTATUS)
    { int i = 0 ;
      if (!(i = listeFind (activeContext->readLockListe, assVoid(key))))
	{
	  aceSetErrorMessage
	    (messprintf("aceWriteLockKey: key %s:%s is allready locked, sorry",
			className(key),name (key))) ;
	  return FALSE ;
	}
      listeRemove (activeContext->readLockListe, assVoid(key), i) ;
    }
  
  lexlock (key) ;
  listeAdd (activeContext->writeLockListe, assVoid(key)) ;
  
  return TRUE ;
}

Bool aceWriteLockKeySet(AceKeySet ks) 
{
  int i ;
  CHECKACE ;
  i = keySetMax(ks) ;
  while (i--)
    if (!aceWriteLockKey(keySet(ks,i)))
      {
	for (i++ ; i < keySetMax(ks) ; i++)
	  lexunlock(keySet(ks,i)) ;
	return FALSE ;
      }
  return TRUE ;
}

Bool aceWriteLockHandle(AceHandle hh) 
{
  CHECKACE ;
  aceSetErrorMessage("unimplemented, sorry") ;
  return FALSE ;
}   

Bool aceWriteLockInstance(AceInstance ai) 
{
  CHECKACE ;
  if (ai->lock == INSTANCE_WRITE_LOCK) return TRUE ;
  if (!aceWriteLockKey(ai->key))
    return FALSE ;
  bsDestroy (ai->obj) ;
  lexunlock(ai->key) ;
  ai->obj = bsUpdate(ai->key) ; /* will re-lexlock */
  ai->lock = INSTANCE_WRITE_LOCK ;
  return TRUE ;
}

Bool aceWriteLockInstanceSet(AceInstanceSet as) 
{
  int i ; AceInstance ai ;
  CHECKACE ;
  i = arrayMax(as) ;
  while (i--)
    if (!aceWriteLockInstance(array(as,i,AceInstance)))
      {
	for (i++ ; i < arrayMax(as) ; i++)
	  {
	    ai = array(as,i,AceInstance) ;
	    bsDestroy (ai->obj) ;
	    bsCreate (ai->key) ;
	  }
	return FALSE ;
      }
  return TRUE ;
}

AceInstance aceOpenKey(AceKey key, AceHandle hh) 
{
  OBJ obj = 0 ; int lock = 0 ; AceInstance ins = 0 ;
  CHECKACE2 ;
  if (!key)
    return FALSE ;
  if (listeFind(activeContext->writeLockListe,assVoid(key)))
    {  
      lexunlock(key) ;
      obj = bsUpdate (key) ;  /* will re-lexlock */
      lock = 2 ; 
    }
  else
    { obj = bsCreate(key) ; 
      lock = listeFind(activeContext->readLockListe,assVoid(key)) ?  1 : 0 ;
    }
  if (obj)
    { ins = instanceCreate (key,obj,h1) ; ins->lock = lock ; }
  else
    aceSetErrorMessage(messprintf("Cannot open the object %s, sorry",
				  className(key),name(key))) ;
  return ins ;
}

AceInstanceSet aceOpenKeySet(AceKeySet ks, AceHandle hh) 
{
  int i ;
  AceInstanceSet ais = 0 ;AceInstance ai ;
  CHECKACE ;
  i = keySetMax(ks) ;
  ais = arrayCreate (i,AceInstance) ;
  while (i--)
    if (!(ai =aceOpenKey(keySet(ks,i),hh)))
      {
	for (i++ ; i < keySetMax(ks) ; i++)
	  aceFree(array(ais,i,AceInstance)) ;
	arrayDestroy (ais) ;
	return 0 ;
      }
    else
      array(ais,i,AceInstance) = ai ; 
  return ais ;
}

AceInstance aceUpdateKey(AceKey key, AceHandle hh) 
{
  if (!aceWriteLockKey(key))
    return 0 ;
  return aceOpenKey (key, hh) ;
}

Bool aceRefreshHandle(AceHandle hh) 
{
  CHECKACE ;
  aceSetErrorMessage("unimplemented, sorryy") ;
  return FALSE ;
}

Bool aceRefreshInstance(AceInstance ai) 
{
  CHECKACE ;
  if (!ai->obj)
    {
      aceSetErrorMessage("You cannot refresh a not-open instance") ;
      return FALSE ;
    }
  bsDestroy (ai->obj) ; 
  if (ai->lock >= INSTANCE_WRITE_LOCK)
    { bsUpdate (ai->key) ; ai->lock = INSTANCE_WRITE_LOCK ; }
  else
    bsCreate (ai->key) ;
  return TRUE ;
}

Bool aceRefreshInstanceSet(AceInstanceSet as) 
{
  int i ; 
  CHECKACE ;
  i = arrayMax(as) ;
  while (i--)
    if (!aceRefreshInstance (array(as,i,AceInstance)))
      {
	return FALSE ;
      }
  return TRUE ;
}

Bool aceCommitHandle(AceHandle h, Bool keepWriteLock)
{
  CHECKACE ;
  aceSetErrorMessage("unimplemented, sorry") ;
  return FALSE ;
}

Bool aceCommitInstance(AceInstance ai, Bool keepWriteLock) 
{
  int n ;
  CHECKWINS(ai) ;
  bsSave(ai->obj) ;
  if (keepWriteLock)
    { 
      ai->obj = bsUpdate(ai->key) ; 
      ai->lock = INSTANCE_WRITE_LOCK ;
    }
  else  /* eventually keep read-lock */
    { 
      ai->obj = bsCreate(ai->key) ; 
      ai->lock &= INSTANCE_READ_LOCK ; 
      n = listeFind (activeContext->writeLockListe, assVoid(ai->key)) ;
      listeRemove (activeContext->writeLockListe, assVoid(ai->key), n) ;
    }
  return TRUE ;
}

Bool aceCommitInstanceSet(AceInstanceSet as, Bool keepWriteLock)
{
  int i ; 
  CHECKACE ;
  i = arrayMax(as) ;
  while (i--)
    if (!aceCommitInstance(array(as,i,AceInstance),keepWriteLock))
      return FALSE ;
  return TRUE ;
}

/******************** Editions  *******************************/

/* editing objects */

/* -- unsupported, always fails */
int aceEditInstances(AceInstanceSet ks, char* command);
Bool aceEditAceKeySet(AceKeySet ks, char* command);

/* killing objects */
Bool aceKillName(char* instancename);
Bool aceKillKey(AceKey key);
Bool aceKillAceKeyset(AceKeySet ks);

/******************* DNA Methods      ******************************/

Array aceDnaGet (AceKey key, AceHandle hh)
{
  Array a = 0, b = 0 ;
  CHECKACE2 ;

  a = dnaGet (key) ;
  if (a) 
    {
      b = pushArrayOnHandle (a, h1) ;
      arrayDestroy (a) ;
      return b ;
    }
  else
    aceSetErrorMessage
      (messprintf("No dna associated to %s:%s", 
		  aceClassName(key), aceName(key))) ;
  return 0 ;
}

/******************* Grep and Queries  ******************************/

/* grep.  Optionally, do LongGrep. */
AceKeySet  aceGrep(char* pattern, Bool doLong, AceHandle hh)
{
  KEYSET ks, ks1 ;
  CHECKACE2 ;

  ks = queryGrep(0, pattern) ;
  ks1 = pushKeySetOnHandle(ks, h1) ;
  keySetDestroy (ks) ;
  return ks1 ;
}

AceKeySet aceGrepFrom(AceKeySet old, char* pattern, Bool doLong,AceHandle hh)
{
  KEYSET ks, ks1 ;
  CHECKACE2 ;

  ks = queryGrep(old, pattern) ;
  ks1 = pushKeySetOnHandle(ks, h1) ;
  keySetDestroy (ks) ;
  return ks1 ;
}

/* old style ace queries return a keyset */
AceKeySet aceQueryKey (AceKey key, char *cp, AceHandle hh)
{ 
  KEYSET ks = 0, ks1 ;
  CHECKACE2;
  
  ks = queryKey (key, cp) ;
  ks1 = pushKeySetOnHandle (ks,h1) ;
  keySetDestroy (ks) ;
  return ks1 ;
}

AceKeySet aceQuery(AceKeySet ks0, char *cp, AceHandle hh)
{
  KEYSET ks = 0, ks1 ;
  CHECKACE2;
  
  ks = query (ks0, cp) ;
  ks1 = pushKeySetOnHandle (ks,h1) ;
  keySetDestroy (ks) ;
  return ks1 ;
}

/* filtering a set of keys/instances */
Bool  aceFilterKey(AceKey k, char* queryString)
{
  int i ;
  KEYSET ks ;
  CHECKACE ;
  ks = queryKey (k,queryString) ;
  i = keySetMax(ks) ;
  keySetDestroy (ks) ;
  return i ? TRUE : FALSE ;
}

int aceKeyHasTag (AceKey key, AceKey tag)
{
  CHECKACE ;
  return bIndexFind(key,tag) ;
}

Bool  aceFilterInstance(AceInstance a, char* queryString) ;
AceKeySet aceFilterKeySet(AceKeySet ks, char* queryString,AceHandle hh)
{ 
  KEYSET ks1 = 0, ks2 = 0 ;
  CHECKACE2;
  
  ks1 = query(ks,queryString) ;
  ks2 = pushKeySetOnHandle (ks1,h1) ;
  keySetDestroy (ks1) ;
  return ks2 ;
}

AceInstanceSet  aceFilterInstanceSet( AceInstanceSet as, char* queryString,AceHandle hh);

/* new style Ace Query Language, returns a table */
AceTable aceAql(char *aql) 
{
  return (AceTable)0 ; 
}

/* Object constructor, not yet supported */
AceTable aceMakeInstanceTable(AceInstance ai, int depth, AceHandle hh)
{
  return (AceTable)0 ;
}

/*************************** Table methods ****************************/

Bool     aceTableCell(AceTable table, int i, int j)
{
  NOTDONE ;
}

AceTable aceTableByName(char* tablename, AceHandle hh)
{
  NOTDONE ;
}

/* output */
/* %a = ace %h = human %j = java %p = perl, etc.
 * a '*' following the format specifier indicates that either an
 * entire AceKeySet or an entire AceInstanceSet should be dumped.
 * a 'i' following the format specifier indicates that we are working
 * with an INSTANCE (or AceInstanceSet) instead of a AceKey or AceKeySet.
 */
/* ???????? don t understand this, mieg */
Bool acefprintf(FILE* f,char* format, ...) 
{
  NOTDONE ;
}

Bool acesprintf(char*f,char* format, size_t buflen)
{
  NOTDONE ;
}

/*********************************************************************/
/*********************************************************************/
/* models */

AceInstance aceOpenModel(char* classname,AceHandle hh)
{
  KEY model ;
  AceInstance ins = 0 ;
  CHECKACE2 ;
 
  if (!lexword2key(classname,&model,_VModel))
    aceSetErrorMessage
      (messprintf("aceOpenModel(%s): unknown model, sorry",
		  classname)) ;
  if (model)
    ins = aceOpenKey (model,h1) ;
  return ins ;
}

/**********************************************************************/
/******************** Tree Manipulations *******************************/
/*********************************************************************/

AceKey aceKeyOfInstance(AceInstance ai)
{
  CHECKINS(ai) ;
  return ai->key ;
}


/* tree manipulation */
Bool aceTestInstance (AceInstance ai, char* predicate, Bool moveCurrent)
{
  NOTDONE ;
}

/* mark/goto */
/* create the AceMark for the caller */
AceMark aceMark (AceInstance ai, AceHandle hh)
{
  AceHandle h1 ;
  CHECKINS(ai) ;
  h1 = hh ? hh : activeHandle ;
  return bsHandleMark(0, ai->obj,h1) ;
}


Bool  aceGotoMark (AceInstance ai, AceMark mark)
{
  CHECKINS(ai) ;
  bsGoto (ai->obj,mark) ;
  return TRUE ;
}

/* search object */
Bool    aceGotoTag (AceInstance ai, AceKey tag)
{
  CHECKINS(ai) ;
  if (bsFindTag(ai->obj,tag))
    return TRUE ;
  return FALSE ;
}

Bool aceHasTag (AceInstance ai, AceKey tag)
{
  static BSMARK mark = 0 ;
  BOOL bb ;
  CHECKINS(ai) ;
  mark = bsMark(mark, ai->obj) ;
  bb = bsFindTag(ai->obj,tag) ;
  bsGoto (ai->obj, mark) ;
  return bb ;
}


/* navigate object */
/* what about session obj, comments etc ??? */
Bool aceNextChild (AceInstance ai)
{
  return aceGotoTag (ai,_bsDown) ;
}

Bool aceNextBrother (AceInstance ai)
{
  return aceGotoTag (ai,_bsDown) ;
}


Bool aceGotoChild (AceInstance ai)
{
  return aceGotoTag (ai,_bsRight) ;
}

/* verify the current type */
Bool    aceCheckType( AceInstance , AceType );
AceType aceGetType( AceInstance );
/* verify the presence of a type in the current follow set */
Bool    aceFollowedBy( AceInstance , AceType );

Bool aceGetTag (AceInstance ai, AceKey *xkp)
{
  CHECKINS(ai) ;
  return bsGetKeyTags (ai->obj, _bsHere, xkp) ;  
}

Bool aceGetKey (AceInstance ai, AceKey *xkp)
{
  CHECKINS(ai) ;
  return bsGetKey(ai->obj, _bsHere, xkp) ;  
}

Bool aceGetInteger (AceInstance ai, int *xip)
{
  CHECKINS(ai) ;
  return bsGetData(ai->obj, _bsHere, _Int, xip) ;  
}

Bool aceGetFloat (AceInstance ai, float *xfp)
{
  CHECKINS(ai) ;
  return bsGetData(ai->obj, _bsHere, _Float, xfp) ;  
}

Bool aceGetDateType (AceInstance ai, mytime_t* xdtp)
{
  CHECKINS(ai) ;
  return bsGetData(ai->obj, _bsHere, _DateType, xdtp) ;  
}

Bool aceGetText (AceInstance ai, char **cpp)
{
  CHECKINS(ai) ;
  return bsGetData(ai->obj, _bsHere, _Text, cpp) ;
}

Bool aceGetArray (AceInstance ai, AceTag target, Array units, int width)
{
  if (arrayExists(units))
    arrayMax(units) = 0 ;
  else
    {
      aceSetErrorMessage ("arrayCreate units before calling aceGetArray") ;
      return FALSE ;
    }
  CHECKINS(ai) ;
  
  return bsGetArray  (ai->obj, target, units, width) ;
}

/* in place modifications of the object 
 * forbidden, may break data integrity by creating doubles 
 *
Bool     aceReplAceKey( AceInstance, AceKey );
Bool     aceReplAceTag( AceInstance, AceTag );
Bool     aceReplaceInteger( AceInstance, int );
Bool     aceReplaceText( AceInstance, char* );
Bool     aceReplaceFloat( AceInstance, float );
*/

/* removes curr and subtree to the right 
 * returns curr to the root of the tree
 */
Bool aceRemove (AceInstance ai)
{
  CHECKWINS (ai) ;

  bsRemove (ai->obj) ;
  return TRUE ;
}

Bool aceAddTag (AceInstance ai, AceTag tag)
{
  CHECKWINS (ai) ;

  bsAddTag (ai->obj, tag) ;
  return bsFindKey (ai->obj, _bsHere, tag) ;
}

/* modify the position immediately to the right */

Bool aceAddKey(AceInstance ai,  AceKey key)
{
  CHECKWINS (ai) ;

  /* if check that key is accetable */
  bsAddKey (ai->obj, _bsRight, key) ;
  return TRUE ;
}

Bool aceAddInteger (AceInstance ai,  int nn)
{
  CHECKWINS (ai) ;

  /* if check that int is accetable */
  bsAddData (ai->obj, _bsRight, _Int, &nn) ;
  return TRUE ;
}

Bool aceAddFloat (AceInstance ai,  float xx)
{
  CHECKWINS (ai) ;

  /* if check that float is accetable */
  bsAddData (ai->obj, _bsRight, _Float, &xx) ;
  return TRUE ;
}

Bool aceAddText (AceInstance ai,  char *cp)
{
  CHECKWINS (ai) ;

  /* if check that Text is acceptable */
  bsAddData (ai->obj, _bsRight, _Text, &cp) ;
  return TRUE ;
}

Bool aceAddDate (AceInstance ai,  AceDateType dd)
{
  CHECKWINS (ai) ;

  /* if check that Date is acceptable */
  bsAddData (ai->obj, _bsRight, _DateType, &dd) ;
  return TRUE ;
}

/* attach comments to the current position */
/* comments will no longer be inserted into the structure of the
 * object itself, but will be added along a third dimension.
 */
Bool     aceAddComment( AceInstance, char* );
char*    aceGetComment( AceInstance, AceHandle );
 
/* remove everything to the right of the current position,
 * as well as the current node.
 */
Bool acePrune (AceInstance ai)
{
  CHECKWINS (ai) ;
  bsPrune(ai->obj) ;
  ai->lock = INSTANCE_DIRTY_LOCK;
  return TRUE ;
}

/*******************************************************************/
/****************** Utilities **************************************/

static Array pushArrayOnHandle (Array a, AceHandle hh)
{
  int i ; 
  Array new = 0 ;

  if (!a) return 0 ;
  i = arrayMax (a) ;
  new = uArrayCreate_dbg(i + 1,a->size,hh,__FILE__,__LINE__) ; /* ensures not zero, and zero termination   */

  arrayForceFeed(new, i) ;
  memcpy (new->base, a->base, i*a->size) ;
  return new ;
}

static KEYSET pushKeySetOnHandle (KEYSET ks, AceHandle hh)
{ return pushArrayOnHandle (ks,hh) ; }



/* Needs work, in client server mode, you would not
   exit(), just close connection.
   also if commit = true and this does not work, there should
   be an error message 
*/
static Bool doTheQuit(BOOL commit, BOOL crash)
{
  BOOL result = FALSE ;
  BOOL committed = TRUE ;
  int n ;

  if(activeContext)
   {    
     n =  arrayMax(aceContextArray) ;
     while (n--)
       if (aceSetContext (assVoid(n)))
	 committed &= aceCloseContext(commit) ;
   }

  /* close session, save session if commit==TRUE, release write access if necc.*/
  if (crash)
    sessionCloseExit(commit, ACELOG_CRASH_EXIT) ;
  else
    sessionClose(commit) ;

  filtmpcleanup () ;		/* deletes any temporary files made */
  readlockDeleteFile ();	/* release readlock */

  messfree (sessionHandle) ;

  sessionShutDown () ;   /* fully deallocates memory of the different libs */

  if (!commit || committed)
    { 
      result = TRUE ;
    }
  else
    {
      aceSetErrorMessage ("aceQuit: commit failed on at least one context") ;
      result = FALSE ;
    }

  return result ;
}


/*******************************************************************/
/*******************************************************************/
/*******************************************************************/

