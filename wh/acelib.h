/*  File: acelib.h
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
 * Description: 
 * Exported functions: 
 * HISTORY:
 * Last edited: Aug 26 16:47 1999 (fw)
 * * Aug 26 16:47 1999 (fw): added this header
 * Created: Thu Aug 26 16:46:38 1999 (fw)
 * CVS info:   $Id: acelib.h,v 1.12 1999-09-01 11:02:10 fw Exp $
 *-------------------------------------------------------------------
 */

#ifndef ACELIB_H
#define ACELIB_H

#ifdef _CPLUSPLUS
extern "C" {
#endif

#define BLOCKING	   /* indicates routines which may be blocking */

#include "acetypes.h"
#include "aceversion.h"

/**************************************************************************
 *************************** Basic manipulations***************************
 *
 *   acelib is meant to be used in two ways:
 * -to link an application against an xace or tace kernel, 
 * -or to be part of a distant client code connecting via the network
 *
 *   The implementation of acelib differs for these situations, but
 *  the way acelib is used by the application is the same. First call
 *  aceInit() to connect to the database, possibly launching the server
 *  if it was not running before, then call aceOpenContext() to identify
 *  your session, finally aceQuit() to terminate the connection.
 *
 *  Bool aceInit(char* ACEDB)
 *
 *   ACEDB identifies the server.
 * -server side implementation, 
 *    The ACEDB parameter is the usual $ACEDB of xace, it consists of
 *  a colon concatenated file path, for example "/home/worm:/usr/local/acedb"
 *  the first member should contain the subdirectories database and wspec
 *  the other directories like wquery will be searched for in the whole path.
 * -client side implementation
 *    The client side implementation must handle the network. It will
 *  use the ACEDB parameter to locate the server on the network. If we
 *  write a CORBA implementation, we may connect to the actual server
 *  using the CORBA service broadcasting mechanism, but we may also write
 *  a socket implemetation and find the server port by a hard coded
 *  look up table.  It is up to the implementation whether it wants to
 *  transfer the data in larger chunks and buffer them, or in little
 *  pieces. The only required behaviour is to deliver the data as
 *  specified in the present API.
 *    The network handler will shake hand with the server and return
 *  true to the client if the server was found. The server side of the
 *  network handler will only itself need to call aceInit(FILE_PATH)
 *  if the database is not yet running.
 *
 *  AceContext* aceOpenContext (char* client_signature)
 *
 *  must be called by every application before getting any data from
 *  acelib. The context, see below for details, is used by the server
 *  to stamp data modifications and to manage memory allocation for
 *  the client. When the client calls aceClose(), the memory is
 *  garbage collected.However, it is polite to call aceFree() as soon
 *  as possible on any pointer received from acelib.
 *
 * Bool aceQuit(BOOL commit)
 *
 *  terminates all operations.
 *************************************************************************
*/

/*************************************************************************
 *********************** Working example *********************************
 *
 *  Save the following code as test.c and complile it  by the command
 *     cc -o test test.c -lace
 *  If your compiler is non-ANSI and complains about prototypes, try gcc
 *  in place of cc, or ask your system administrator. 
 *  
 *  To compile you need header files
 *    acelib.h acetypes.h array.h keyset.h regular.h 
 *  in your include path (set by -I<path>) and libace.a in your library path
 *  (set by -L<path>). If all these files are in the current directory a 
 *  pedantic command would be
 *     cc -o test -I. test.c -L. -lace
 *
 *  This program should connect to the acedb database indicated on its
 *  command line (default is the current directory) and return the list
 *  and size of all the data classes.

#include "acelib.h"

void main(int argc, char **argv)
{ 
  int n, i ;
  char *cp ;
  AceStringSet s ; 

  if (aceInit(argc > 1 ? argv[1] : 0))
    { aceOpenContext("test") ;
      s = aceClasses(0) ;
      for (i = 0 ; i < aceStringSetMax(s) ; i++)
	if (aceClassSize (aceStringSet(s,i), &n))
	  printf ("Number of items in class %s = %d\n", 
		  aceStringSet(s,i), n) ;
      aceQuit(FALSE) ;
    }
  else
    fprintf (stderr, "Sorry, no connection\n") ;
}

 *************************************************************************
*/

/*************************************************************************
 ************************** Error reports ********************************
 *
 * All routines return NULL or 0 if and only if they fail. In such a case
 * and error number and a message are available. Errors are enumerated in
 * aceerror.h.
 */

extern int aceErrno;                   /* error number, see aceerror.h */
char*  aceErrorMessage(int* bufsize);  /* hopefully, a short explanation */


/*************************************************************************
 ************************** Session managment ****************************
 * 
 * Always call aceInit() at the top of your application and aceQuit() at
 * the end. The commit parameter of aceQuit applies to all context which
 * are still active and have not call commit themselves.
 *
 * ?????????????????????????????????????????
 *. There is no roll
 * back after you call commit on a context, or is there ?
 * who calls acedb:sessionSave() ?
 * ?????????????????????????????????????????
 *
 */ 


Bool  aceInit(char* ace_path);       /* initialise network or disk access */	
Bool  aceGetWriteAccess(Bool get);   /* if get=true, get it, if get=false: would it be possible */
Bool  aceForbidWriteAccess(void);    /* final, none of your context will have it */

Bool  aceCommit(Bool releaseLock );  /* saves everything to disk */
Bool  aceQuit(Bool commit) ;         /* terminates server connection or closes disk */
void  aceStatus (void) ;             /* status (including memory requirements) of the database */

/*************************************************************************
 ************************** Context managment ****************************
 * 
 * A context is the way to identify the client from the server side. All
 * data operations raise an error if the current context is invalid, and all
 * data editions will be individually time stamped with the client_signature.
 *
 * From the client point of view, The context is the real owner of the
 * data. It owns the memory, the read and write locks and allows roll
 * backs. A client can open several contexts in parallel but can only
 * open a given AceInstance once per context.
 * 
 * The context is established by Open and remains implicitelly valid
 * untill closed or reset by a second Open or Set call. Get returns the
 * current context. Push/Pop must be balanced.
 *
 * Each context has a private handle, and all data allocated in a
 * context is hooked on that handle and garbage collected when the
 * context is closed.  To reuse a pointer after closing the context is
 * an access violation. In the present implementation this error would
 * not be detected but would possibly crash the acedb kernel and
 * provoke a core dump.  */

AceContext*  aceOpenContext(char *clientSignature);	
Bool  aceSetContext(AceContext*);
AceContext*  aceGetContext(void);
Bool  acePushContext(void);
Bool  acePopContext(Bool commit) ;
Bool  aceCloseContext(Bool commit) ;

/**************************************************************************
 ************************** Memory managment ******************************
 *
 * All memory allocated by acelib uses the ACeDB memory management
 * system.  To explicitly free ANY pointer received from acelib, be it a set
 * an instance or simply a character string, you MUST call aceFree(). Any
 * direct call to the standard C library free() would irrevocably crash the
 * acedb kernel.
 * 
 * Memory is allocated on a handle. Calling aceFree(handle) garbage
 * collects the handle and everything that was allocated on that
 * handle. However, you do not have to wait and you can aceFree any
 * individual item, which is then automatically detached from its
 * handle. Since handles are recursive, you can easilly tame your
 * memory requirements. Handles are also used in acelib for bulk data
 * managment, e.g. aceCommitHandle() described below, providing a form 
 * of multiple parallel micro transaction control.
 * 
 * The AceContext* will create a handle of its own, so all structures
 * generated in a context will be aceFree() on aceClose().  And of
 * course all contexts are destroyed on aceQuit().
 */

AceHandle aceHandleCreate(AceHandle h); /* get a handle (possibly recursively) */
Array  uAceArrayCreate(int size, int typesize) ; /* creates an array on the context handle */
#define aceArrayCreate(_size,_type) uAceArrayCreate(_size,sizeof(_type))

Bool      uAceFree(void*);	      /* (recursively) free any acelib pointer, resets to zero */
#define aceFree(_vp) (uAceFree(_vp), _vp = 0)


/***************************************************************************
 ********************* Database Administration *****************************
 *
 * The following operations work in the server context, outside of the
 * user context created by aceOpenContext. They can only be aborted and
 * rolled back by aborting the server session. This does not make
 * sense in a server client environment, but is ok when you run a
 * stand-alone.
 */

/* Public operations */
 /* reads-commit a file, possible if you have write access */
AceKeySet aceParseFile(FILE* f,AceHandle h); 

/* atomically declare/commit a new object name */
AceKey aceMakeKeyFromFormat(char* classname, char* format);

/* usupported operations */
Bool     aceAllowBlocking(Bool canBlock);  /* always fail */

/* Priviledged operations */
Bool  aceReadModels(void);      /* Reads models and other configuration files */
Bool  aceAddUpdate (Bool all) ; /* read acedb sequential update files */

/***************************************************************************/
/******************* Class Information **********************************/
/*
 * In acedb, every instance has a key. Every key belongs to a unique
 * class, and has a name unique in that class.When a full object name
 * is required, it should have the format className:name where
 * 
 * classsName:  
 *  a string without blanks or punctuation of the form [a-zA-Z][a-zA-Z0-9_]*  
 * should be the name of an existing acedb data class, which may be
 * listed by the function aceClasses();
 * 
 * name: 
 *  should be the name of an instance of the class, known or new, A
 * name is either of the same form or any non blank printable string
 * between double quotes "". Ace will actually remove the leading and
 * trailing spaces of names. 
 * 
 * The function aceComposeName() can be used to compose a valid name.  
 */


AceStringSet aceClasses(AceHandle h);       /* lists all class names */
Bool aceClassSize(char* classname, int* count) ; /* number of instances in class */
AceInstance aceModel(char* classname,AceHandle h);  /* sends back a model */

/*************************************************************************
 *************************** Naming **************************************/

char*    aceName(AceKey key);  /* name of the key */
char*    aceClassName(AceKey key); /* class name of the key */
char*    aceComposeName(char* classname, char* instancename, AceHandle h);

/* These 2 functions return 0, if the name is unknown and create = false.
 *. An error is raised if the name is unknown, create is true, and
 * you do not have write access 
 */

AceTag      aceTag(char* name, Bool create);  
AceKey      aceKey(char* fullname, Bool create);
AceKeySet   aceKeySet(AceStringSet fullnames, Bool create, AceHandle h);

/**************************************************************************
 ***************************** Data Access ********************************
 *
 * An AceInstance is a developped acedb object.  It is identified by
 * its key and must be accessed via its key, by the Open operations.
 *
 * A given key can only be opened ONCE in a given context. To gain
 * write access in an object, you must write-lock the corresponding
 * key, and this operation will fail if another user, or another of
 * your contexts has the lock. To prevent dead locks, you can gain
 * locks on sets in a single call
 * 
 * The handle operations act on the instances and the members of the
 * instance sets allocated on the handle.
 *
 * All these operations return the intance locator to the root of the
 * instance.
 * */

/* Several context can read lock the same key, but
 * a read lock can be promoted to a write lock only
 * by the owner of the read lock, if he is alone to own it.
 * If you own the read lock, you do not need to refresh.
 */
Bool aceReadLockKey(AceKey key) ;
Bool aceReadLockKeySet(AceKeySet ks) ;
Bool aceReadLockHandle(AceHandle h) ;
Bool aceReadLockInstance(AceInstance a) ;     
Bool aceReadLockInstanceSet(AceInstanceSet as) ;

/* Only a single context can own the write lock 
 */

Bool aceWriteLockKey(AceKey key) ;
Bool aceWriteLockKeySet(AceKeySet ks) ;
Bool aceWriteLockHandle(AceHandle h) ;   
Bool aceWriteLockInstance(AceInstance a) ;
Bool aceWriteLockInstanceSet(AceInstanceSet as) ;

/* Open will open the object without establishing
 * any lock. The instance is stable may may at some
 * point need to be refreshed
 */
AceInstance    aceOpenKey(AceKey k, AceHandle h) ;
AceInstanceSet aceOpenKeySet(AceKeySet ks, AceHandle h) ;

 
/* Open in write-locked mode 
*/
AceInstance    aceUpdateKey(AceKey k, AceHandle h) ;

/* Refresh/rollback the instances
 */
Bool aceRefreshHandle(AceHandle h) ;
Bool aceRefreshInstance(AceInstance a) ;
Bool aceRefreshInstanceSet(AceInstanceSet as) ;

/* Commit to disk, possibly keeping the writelock
 */
Bool aceCommitHandle(AceHandle h, Bool keepWriteLock) ;
Bool aceCommitInstance(AceInstance a, Bool keepWriteLock) ;
Bool aceCommitInstanceSet(AceInstanceSet as, Bool keepWriteLock) ;

/* aceFree may be called on handle, instance, instanceSet
 * to destroy the instances and release all locks on them
 */

/******************** Editions  *******************************/

/* editing objects */
int aceEditInstances(AceInstanceSet ks, char* command); /* -- unsupported, always fails */
Bool aceEditAceKeyset(AceKeySet ks, char* command);

/* killing objects */
Bool aceKillName(char* instancename);
Bool aceKillKey(AceKey key);
Bool aceKillAceKeyset(AceKeySet ks);

/******************* DNA Methods      ******************************/

Array aceDnaGet (AceKey key, AceHandle h) ;    /* get the dna associated to this key */

/******************* Grep and Queries  ******************************/

/* grep.  Optionally, do LongGrep. */
AceKeySet       aceGrep(char* pattern, Bool doLong, AceHandle h);
AceKeySet       aceGrepFrom(AceKeySet, char* pattern, Bool doLong,AceHandle h);

/* old style ace queries return a keyset */
AceKeySet    aceQuery(AceKeySet ks, char *queryString, AceHandle h) ; 
AceKeySet    aceQueryKey (AceKey key, char *cp, AceHandle hh) ;
/* filtering a set of keys/instances */
int          aceKeyHasTag(AceKey k, AceKey t) ; /* 0:no, 1:maybe, 2:yes */
Bool         aceFilterKey(AceKey k, char* queryString) ;
Bool         aceFilterInstance(AceInstance a, char* queryString) ;
AceKeySet    aceFilterKeySet(AceKeySet ks, char* queryString,AceHandle h);
AceInstanceSet  aceFilterInstanceSet( AceInstanceSet as, char* queryString,AceHandle h);

/* new style Ace Query Language, returns a table */
AceTable    aceAql(char *aql) ;

/* Object constructor, not yet supported */
AceTable aceMakeInstanceTable(AceInstance, int depth, AceHandle );

/*************************** Table methods ****************************/

Bool     aceTableCell(AceTable table, int i, int j);
AceTable aceTableByName(char* tablename, AceHandle hh);

/* output */
/* %a = ace %h = human %j = java %p = perl, etc.
 * a '*' following the format specifier indicates that either an
 * entire AceKeySet or an entire AceInstanceSet should be dumped.
 * a 'i' following the format specifier indicates that we are working
 * with an INSTANCE (or AceInstanceSet) instead of a AceKey or AceKeySet.
 */
Bool acefprintf(FILE*,char* format, ...); 
Bool acesprintf(char*,char* format, size_t buflen);

/**********************************************************************/
/******************** Tree Manipulations *******************************/
/**********************************************************************/

AceKey  aceKeyOfInstance(AceInstance ins) ;

/* tree manipulation */
Bool    aceTestInstance(AceInstance, char* predicate, 
                        Bool moveCurrent);
/* mark/goto */
AceMark aceMark( AceInstance, AceHandle ); 
/* create the AceMark for the caller */
Bool    aceGotoMark( AceInstance, AceMark target );
/* search object */
Bool    aceGotoTag( AceInstance, AceKey target );
Bool    aceHasTag( AceInstance, AceKey target );
/* navigate object */
Bool    aceNextChild( AceInstance );
Bool    aceGotoChild( AceInstance );
/* verify the current type */
Bool    aceCheckType( AceInstance , AceType );
AceType aceGetType( AceInstance );
/* verify the presence of a type in the current follow set */
Bool    aceFollowedBy( AceInstance , AceType );

Bool aceGetTag (AceInstance ai, AceKey *xk) ;
Bool aceGetKey (AceInstance ai, AceKey *xk) ;
Bool aceGetInteger (AceInstance ai, int *xip) ;
Bool aceGetFloat (AceInstance ai, float *xfp) ;
Bool aceGetDateType (AceInstance ai, mytime_t* xdtp) ;
Bool aceGetText (AceInstance ai, char **cpp) ;
Bool aceGetArray (AceInstance ai, AceTag target, Array units, int width) ;

/* in place modifications of the object 
 * was suggested by Miller, but this breaks acedb
 * data integrity since it is an open door for
 * multiple identical keys and wrong dependant tags
Bool     aceReplAceKey( AceInstance, AceKey );
Bool     aceReplAceTag( AceInstance, AceTag );
Bool     aceReplaceInteger( AceInstance, int );
Bool     aceReplaceText( AceInstance, char* );
Bool     aceReplaceFloat( AceInstance, float );
*/
/* remove current node and everything to the right 
 * example: if (aceGotoTag(ai,tag)) aceRemove(ai) ;
*/
Bool     aceRemove (AceInstance ai) ;
/* modify the position immediately to the right */
Bool     aceAddKey( AceInstance, AceKey );
Bool     aceAddTag( AceInstance, AceTag );
Bool     aceAddInteger( AceInstance, int );
Bool     aceAddText( AceInstance, char* );
Bool     aceAddFloat( AceInstance, float );
/* attach comments to the current position */
/* comments will no longer be inserted into the structure of the
 * object itself, but will be added along a third dimension.
 */
Bool     aceAddComment( AceInstance, char* );
char*    aceGetComment( AceInstance, AceHandle );
 
/* remove everything to the right of the current position,
 * as well as the current node.
 */
Bool     acePrune( AceInstance );

#ifdef _CPLUSPLUS
}
#endif

#endif /* !ACELIB_H */


 
 
