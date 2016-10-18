/*  File: idacedb.c
 *  Author: Richard Durbin (rd@sanger.ac.uk)
 *  Copyright (C) Richard Durbin, Genome Research Limited, 2002
 *-------------------------------------------------------------------
 * This file is part of the ACEDB genome database package
 *
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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111
 * or see the on-line version at http://www.gnu.org/copyleft/gpl.txt
 * ------------------------------------------------------------------
 * Description: Acedb database implementation of the idcurate.h 
     interface.  Also provides tace/aceserver interface.
     Administrator interface is not supported, since the models
     need to be touched.
 * Exported functions:
 * Outstanding:
     idaFind() should only return Live things, but then need to write an
     internal name for duplicates that finds dead things.  Or should we
     have separate functions for finding live only, or live+dead?
 * HISTORY:
 * Last edited: Apr 19 18:33 2002 (edgrif)
 * Created: Tue Jan 22 23:30:14 2002 (rd)
 * CVS info: $Id: idacedb.c,v 1.4 2002-04-19 17:52:11 edgrif Exp $
 *-------------------------------------------------------------------
 */

/*************** models required by this file 

  // in models.wrm for each supported domain

?<domain> Name_version UNIQUE Int
	  Name	Public_name UNIQUE ?<domain>_name XREF Public_name
			// not directly editable
		<nametype>_name [UNIQUE] ?<domain>_name XREF <nametype>_name
		...
  	  History Version_change Int UNIQUE DateType UNIQUE Text #ID_history_action
	  		 // Text above is the curator user name
		  Merged_into UNIQUE ?Gene XREF Acquires_merge
		  Acquires_merge ?Gene XREF Merged_into
		  Split_from UNIQUE ?Gene XREF Split_into
		  Split_into ?Gene XREF Split_from_parent
	  Live  // remove live to make gene "dead"

?<domain>_name <domain> Public_name ?<domain> XREF Public_name
                        <nametype>_name ?<domain> XREF <nametype>_name
	                ...

 // and in whooks/sysclass.c

#ID_history_action Event Created
                         Killed
	                 Resurrected
	                 Merged_into UNIQUE Text
	                 Acquires_merge UNIQUE Text
	                 Split_from UNIQUE Text
                         Split_into UNIQUE Text 
			 Add_name UNIQUE Text UNIQUE Text UNIQUE Text
			 Remove_name UNIQUE Text UNIQUE Text UNIQUE Text
			 Imported Text
// the three Texts are <id> <type> <name>
// Merged_into, Acquires_merge, Split_into, Split_from use <id>
// Created, Split_from, Add_name, Remove_name use <type> <name>
// if <type> is unique then Add_name replaces, as normal in acedb

?Class ...
       ... ID_local ID_template UNIQUE Text
                    ID_nametype Text #ID_nametype_qualifiers

#ID_nametype_qualifiers Is_unique
                        Is_primary
                        Users_with_write_access Text

****************************/

#include <wh/acedb.h>
#include <wh/lex.h>
#include <wh/bs.h>
#include <whooks/systags.h>
#include <wh/pick.h>
#include <wh/bindex.h>
#include <wh/keyset.h>
#include <wh/array.h>
#include <wh/dict.h>
#include <wh/aceio.h>
#include <wh/session.h>		/* for sessionUserSession2User() */
#include <w5/idacedb.h>


typedef struct {
  int objectClass ;
  int nameClass ;
  char *template ;
  Array types ;			/* of NameType */
  DICT *typeDict ;
} DomainType ;  

struct IdConnectionStruct {
  STORE_HANDLE handle ;		/* internal allocations on this */
  char *user ;
  IdError error ;
  DomainType *domain ;
} ;

typedef struct {
  char* text ;
  BOOL isUnique ;
  BOOL isPrimary ;
  KEY  tag ;
} NameType ;

static int  classClass = 0 ;

static void idaCurateInitialise (void) ;
static void idaFinalise (void* ix) ;
static KEY  newId (IdCnxn ix) ;
static BOOL setPublicName (DomainType *domain, KEY id) ;
static void updateHistory (KEY id, char* user,
			   char* event, KEY target, 
			   NameType *type, char* name) ;
static DomainType *getDomain (char *domain) ;

/*************************************************************/
/****** initialise a connection ******************************/
/*************************************************************/

IdCnxn idaConnect (char *host, int port,
		  char *user, char *password, IdError *err)

  /* ignore host, port and password in this implementation */
{
  IdCnxn ix ;

  idaCurateInitialise () ;

  ix = (IdCnxn) messalloc (sizeof(struct IdConnectionStruct)) ;
  ix->handle = handleCreate () ;
  blockSetFinalise (ix, idaFinalise) ;

  if (!user || !*user)
    { if (err) *err = BAD_PASSWORD ; messfree (ix) ; return 0 ; }
  else
    ix->user = strnew (user, ix->handle) ;

  if (err) *err = SUCCESS ;
  return ix ;
}

static void idaFinalise (void* ix)
{ handleDestroy (((IdCnxn)ix)->handle) ; }

IdError idaError (IdCnxn ix)
{
  if (!ix) return BAD_CONNECTION ;
  return ix->error ;
}

/*************************************************************/
/****** check utilities for tops of functions ****************/
/*************************************************************/

static BOOL ixCheck (IdCnxn ix, char *domain, BOOL isEdit)
{
  if (!ix)
    return FALSE ;
  ix->error = SUCCESS ;
  if (isEdit && !(isWriteAccess () || sessionGainWriteAccess ()))
    { ix->error = NO_PERMISSION ; return FALSE ; }
  if (!(ix->domain = getDomain (domain)))
    { ix->error = BAD_DOMAIN ; return FALSE ;}
  return TRUE ;
}

static BOOL idCheck (IdCnxn ix, ID id, KEY *key, OBJ *obj, 
		     BOOL live, BOOL isEdit)
{
  if (!lexword2key (id, key, ix->domain->objectClass) ||
      (live && !bIndexTag (*key, str2tag("Live"))) ||
      (!live && bIndexTag (*key, str2tag("Live"))))
    { ix->error = BAD_ID ; return FALSE ; }
  if (isEdit && !(*obj = bsUpdate (*key)))
    { ix->error = NO_PERMISSION ; return FALSE ; }
  if (!isEdit && !(*obj = bsCreate (*key)))
    { ix->error = BAD_ID ; return FALSE ; }
  return TRUE ;
}

static BOOL typeCheck (IdCnxn ix, char *nameType, NameType **type)
{
  int t ;

  if (!nameType || !dictFind (ix->domain->typeDict, nameType, &t))
    { ix->error = BAD_NAME_TYPE ; return FALSE ; }
  *type = arrayp(ix->domain->types, t, NameType) ;
  return TRUE ;
}

/*************************************************************/
/****** primary actions to create, merge, split, kill etc. ***/
/*************************************************************/

ID idaCreate (IdCnxn ix, char* domain, mytime_t date)
{
  KEY idKey ;

  if (!ixCheck (ix, domain, TRUE)) return 0 ;

  idKey = newId (ix) ;
  updateHistory (idKey, ix->user, "Created", 0, 0, 0) ;

  return (ID) name(idKey) ;
}

/**************************************************************/

BOOL idaKill (IdCnxn ix, char* domain, ID id, mytime_t date)
{
  OBJ obj ;
  KEY idKey ;

  if (!ixCheck (ix, domain, TRUE)) return 0 ;
  if (!idCheck (ix, id, &idKey, &obj, TRUE, TRUE)) return 0 ;

  if (!bsFindTag (obj, str2tag("Live")))
    messcrash ("screwup in idKill for %s", id) ;
  bsPrune (obj) ;

  bsSave (obj) ;
  updateHistory (idKey, ix->user, "Killed", 0, 0, 0) ;
  return TRUE ;
}

/**************************************************************/

BOOL idaResurrect (IdCnxn ix, char *domain, ID id, mytime_t date)
{
  OBJ obj ;
  KEY idKey ;

  if (!ixCheck (ix, domain, TRUE)) return 0 ;
  if (!idCheck (ix, id, &idKey, &obj, FALSE, TRUE)) return 0 ;

  if (bIndexTag (idKey, str2tag("Merged_into")))
    { ix->error = BAD_ID ; return 0 ; }

  if (!bsAddTag (obj, str2tag("Live")))
    messcrash ("screwup in idResurrect for %s", id) ;

  bsSave (obj) ;
  updateHistory (idKey, ix->user, "Resurrected", 0, 0, 0) ;
  return TRUE ;
}

/**************************************************************/

BOOL idaMerge (IdCnxn ix, char *domain, ID id, ID target, mytime_t date)
{
  OBJ obj ;
  KEY idKey, targetKey ;

  if (!ixCheck (ix, domain, TRUE)) return 0 ;
  if (!lexword2key (target, &targetKey, ix->domain->objectClass) ||
      !bIndexTag (targetKey, str2tag("Live")))
    { ix->error = BAD_TARGET ; return FALSE ; }
  if (!idCheck (ix, id, &idKey, &obj, TRUE, TRUE)) return 0 ;

  if (!bsFindTag (obj, str2tag("Live")))
    messcrash ("screwup in idMerge for %s", id) ;
  bsPrune (obj) ;
  bsSave (obj) ;

  updateHistory (idKey, ix->user, "Merged_into", targetKey, 0, 0) ;
  updateHistory (targetKey, ix->user, "Acquires_merge", idKey, 0,0) ;
  return TRUE ;
}

/**************************************************************/

ID idaSplit (IdCnxn ix, char *domain, ID id, mytime_t date)
{
  KEY idKey, child ;
  OBJ obj ;

  if (!ixCheck (ix, domain, TRUE)) return 0 ;
  if (!idCheck (ix, id, &idKey, &obj, TRUE, TRUE)) return 0 ;
  bsDestroy (obj) ;		/* don't update here */

  child = newId (ix) ;

  updateHistory (child, ix->user, "Split_from", idKey, 0, 0) ;
  updateHistory (idKey, ix->user, "Split_into", child, 0, 0) ;

  return (ID) name(child) ;
}

/**************************************************************/

BOOL idaAddName (IdCnxn ix, char *domain, ID id, 
		 char*nameType, char *newName, mytime_t date)
{
  KEY idKey, nameKey ;
  OBJ obj ;
  NameType *type ;

  if (!ixCheck (ix, domain, TRUE)) return FALSE ;
  if (!typeCheck (ix, nameType, &type)) return FALSE ;
  if (!name || !*name ||
      (type->isPrimary && idaFind (ix, domain, nameType, newName)))
    { ix->error = DUPLICATE_NAME ; return 0 ; }
  if (!idCheck (ix, id, &idKey, &obj, TRUE, TRUE)) return FALSE ;

  lexaddkey (newName, &nameKey, ix->domain->nameClass) ;
  bsAddKey (obj, type->tag, nameKey) ;
  bsSave (obj) ;
  setPublicName (ix->domain, idKey) ;

  updateHistory (idKey, ix->user, "Add_name", 0, type, newName) ;
  return TRUE ;
}

/*************************************************************/

BOOL idaRemoveName (IdCnxn ix, char *domain, ID id, 
		    char*nameType, char *name, mytime_t date)
{
  KEY idKey, nameKey ;
  OBJ obj ;
  NameType *type ;

  if (!ixCheck (ix, domain, TRUE)) return FALSE ;
  if (!typeCheck (ix, nameType, &type)) return FALSE ;
  if (!idCheck (ix, id, &idKey, &obj, TRUE, TRUE)) return FALSE ;

  if (!name || !*name ||
      !lexword2key (name, &nameKey, ix->domain->nameClass) ||
      !bsFindKey (obj, type->tag, nameKey))
    { ix->error = BAD_NAME ;
      bsDestroy (obj) ;
      return FALSE ;
    }
  bsPrune (obj) ;
  bsSave (obj) ;
  setPublicName (ix->domain, idKey) ;

  updateHistory (idKey, ix->user, "Remove_name", 0, type, name) ;
  return TRUE ;
}

/*************************************************************/
/****** access functions *************************************/
/*************************************************************/

ID* idaGetByName (IdCnxn ix, char *domain, char* nameType, char* nam)
{
  KEY nameKey, key ;
  OBJ obj ;
  ID *idList ;
  NameType *type ;
  int i ;
  static Array a = 0 ;

  a = arrayReCreate (a, 16, KEY) ;

  if (!ixCheck (ix, domain, FALSE)) return 0 ;
  if (!nam || !*nam)
    { ix->error = BAD_NAME ; return 0 ; }

  if (lexword2key (nam, &nameKey, ix->domain->nameClass) &&
      (obj = bsCreate (nameKey)))
    { if (!*nameType)		/* legal - use all types */
        { for (i = 1 ; i < arrayMax(ix->domain->types) ; ++i)
	    if (bsGetKey (obj, arrp(ix->domain->types, i, NameType)->tag, 
			  &key))  
	      do 
		if (bIndexTag(key, str2tag("Live")))
		  array (a, arrayMax(a), KEY) = key ;
	      while (bsGetKey (obj, _bsDown, &key)) ;
	}
      else if (!strcmp (nameType, "Public"))
	{ if (bsGetKey (obj, str2tag("Public_name"), &key))  
	    do 
	      if (bIndexTag(key, str2tag("Live")))
		array (a, arrayMax(a), KEY) = key ;
	    while (bsGetKey (obj, _bsDown, &key)) ;
	}
      else
	{ if (!typeCheck (ix, nameType, &type))
	    { bsDestroy (obj) ; return FALSE ; }
	  if (bsGetKey (obj, type->tag, &key))  
	    do
	      if (bIndexTag(key, str2tag("Live")))
		array (a, arrayMax(a), KEY) = key ;
	    while (bsGetKey (obj, _bsDown, &key)) ;
	}
      bsDestroy (obj) ;
    }

  idList = (ID*) messalloc ((arrayMax(a)+1)*sizeof(ID)) ;
  for (i = 0 ; i < arrayMax(a) ; ++i)
    idList[i] = name(arr(a,i,KEY)) ;
  return idList ;
}

/****************************************/

ID idaFind (IdCnxn ix, char *domain, char *nameType, char *nam)
{
  KEY nameKey ;
  NameType *type ;
  KEY key = 0 ;
  OBJ obj ;

  if (!ixCheck (ix, domain, FALSE)) return 0 ;
  if (!nam || !*nam)
    { ix->error = BAD_NAME ; return 0 ; }
  if (!typeCheck (ix, nameType, &type)) return 0 ;
  if (!type->isPrimary)
    { ix->error = BAD_NAME_TYPE ; return 0 ; }

  if (lexword2key (nam, &nameKey, ix->domain->nameClass) &&
      (obj = bsCreate (nameKey)))
    { bsGetKey (obj, type->tag, &key) ;
      bsDestroy (obj) ;
    }

  if (key) return (ID) name(key) ;
  else return 0 ;
}

/*************************************************************/
/****** properties of ID objects *****************************/
/*************************************************************/

char* idaPublicName (IdCnxn ix, char *domain, ID id)
{
  KEY idKey ;
  OBJ obj ;
  KEY key ;

  if (!ixCheck (ix, domain, FALSE)) return 0 ;
  if (!idCheck (ix, id, &idKey, &obj, TRUE, FALSE)) return 0 ;
  bsGetKey (obj, str2tag("Public_name"), &key) ;
  bsDestroy (obj) ;

  if (key) return name(key) ;
  else return 0 ;
}

/****************************************/

char** idaTypedNames (IdCnxn ix, char *domain, ID id, char *nameType)
{
  KEY idKey, key ;
  OBJ obj ;
  NameType *type ;
  char **nameList ;
  static Array a = 0 ;
  int i ;

  a = arrayReCreate (a, 16, KEY) ;

  if (!ixCheck (ix, domain, FALSE)) return 0 ;
  if (!typeCheck (ix, nameType, &type)) return 0 ;
  if (!idCheck (ix, id, &idKey, &obj, TRUE, FALSE)) return 0 ;
  if (bsGetKey (obj, type->tag, &key))  
    do array (a, arrayMax(a), KEY) = key ;
    while (bsGetKey (obj, _bsDown, &key)) ;
  bsDestroy (obj) ;

  nameList = (char**) messalloc ((arrayMax(a)+1)*sizeof(char*)) ;
  for (i = 0 ; i < arrayMax(a) ; ++i)
    nameList[i] = name(arr(a,i,KEY)) ;
  return nameList ;
}

/****************************************/

char* idaTypedName (IdCnxn ix, char *domain, ID id, char *nameType)
{
  KEY idKey ;
  OBJ obj ;
  NameType *type ;
  KEY key = 0 ;

  if (!ixCheck (ix, domain, FALSE)) return 0 ;
  if (!typeCheck (ix, nameType, &type)) return 0 ;
  if (!type->isUnique)
    { ix->error = BAD_DOMAIN ; return 0 ; }
  if (!idCheck (ix, id, &idKey, &obj, TRUE, FALSE)) return 0 ;
  bsGetKey (obj, type->tag, &key) ;
  bsDestroy (obj) ;

  if (key) return name(key) ;
  else return 0 ;
}

/****************************************/

int idaVersion (IdCnxn ix, char *domain, ID id)
{
  KEY idKey ;
  OBJ obj ;
  int result = 0 ;

  if (!ixCheck (ix, domain, FALSE)) return 0 ;
  if (!idCheck (ix, id, &idKey, &obj, TRUE, FALSE)) return 0 ;
  if (!bsGetData (obj, str2tag("Name_version"), _Int, &result))
    { ix->error = BAD_ID ; return 0 ; }
  return result ;
}

/****************************************/

BOOL idaLive (IdCnxn ix, char *domain, ID id)
{
  KEY idKey ;
  OBJ obj ;
  BOOL result ;

  if (!ixCheck (ix, domain, FALSE)) return 0 ;

	/* can't use idCheck() because it requires to know 
	   whether live or not before calling */
  if (!lexword2key (id, &idKey, ix->domain->objectClass))
    { ix->error = BAD_ID ; return FALSE ; }
  if (!(obj = bsCreate (idKey)))
    { ix->error = NO_PERMISSION ; return FALSE ; }
  result = bsFindTag (obj, str2tag("Live")) ;
  bsDestroy (obj) ;

  return result ;
}

/****************************************/

ID idaUltimate (IdCnxn ix, char *domain, ID id)
{
  KEY idKey ;
  OBJ obj = 0 ;

  if (!ixCheck (ix, domain, FALSE)) return 0 ;
	/* can't use idCheck() because it requires to know 
	   whether live or not before calling */
  if (!lexword2key (id, &idKey, ix->domain->objectClass))
    { ix->error = BAD_ID ; return 0 ; }

  while ((obj = bsCreate (idKey)))
    { if (!bsGetKey (obj, str2tag ("Merged_into"), &idKey))
	{ bsDestroy (obj) ;
	  break ;
	}
      bsDestroy (obj) ;
    }

  return (ID) name(idKey) ;
}

/****************************************/

static void addChildren (Array a, KEY key)
{
  OBJ obj = 0 ;

  array (a, arrayMax(a), KEY) = key ;

  if ((obj = bsCreate (key)))
    { if (bsGetKey (obj, str2tag ("Merged_into"), &key))
	addChildren (a, key) ;
      if (bsGetKey (obj, str2tag ("Split_into"), &key)) do
	addChildren (a, key) ;
      while (bsGetKey (obj, _bsDown, &key)) ;
      bsDestroy (obj) ;
    }
}

ID* idaChildren (IdCnxn ix, char *domain, ID id)
{
  KEY idKey ;
  ID *idList ;
  int i ;
  static Array a = 0 ;

  a = arrayReCreate (a, 16, KEY) ;

  if (!ixCheck (ix, domain, FALSE)) return 0 ;
	/* can't use idCheck() because it requires to know 
	   whether live or not before calling */
  if (!lexword2key (id, &idKey, ix->domain->objectClass))
    { ix->error = BAD_ID ; return 0 ; }

  addChildren (a, idKey) ;

  idList = (ID*) messalloc ((arrayMax(a)+1)*sizeof(ID)) ;
  for (i = 0 ; i < arrayMax(a) ; ++i)
    idList[i] = name(arr(a,i,KEY)) ;
  return idList ;
}

/****************************************/

BOOL idaExists (IdCnxn ix, char *domain, ID id)
{
  KEY idKey ;

  if (!ixCheck (ix, domain, FALSE)) return 0 ;
	/* can't use idCheck() because it requires to know 
	   whether live or not before calling and sets ix->error */
  if (lexword2key (id, &idKey, ix->domain->objectClass))
    return TRUE ;
  else
    return FALSE ;
}

/****************************************/

IdHistory* idaHistory (IdCnxn ix, char *domain, ID id, int version)
{ ix->error = UNIMPLEMENTED ; return 0 ; }

ID* idaChanged (IdCnxn ix, char *domain, char* timeStamp)
{ ix->error = UNIMPLEMENTED ; return 0 ; }

/*************************************************************/
/****** administrator functions ******************************/
/*************************************************************/

IdError idaCreateDomain (IdCnxn ix, char *domain)
{ return (ix->error = UNIMPLEMENTED) ; }

IdError idaSetTemplate (IdCnxn ix, char *domain, char *template)
{ return (ix->error = UNIMPLEMENTED) ; }

IdError idaAddNameType (IdCnxn ix, char *domain, 
		       BOOL isUnique, BOOL isPrimary)
{ return (ix->error = UNIMPLEMENTED) ; }

/*************************************************************/
/****** local functions **************************************/
/*************************************************************/

static void idaCurateInitialise (void)
{
  int isDone = FALSE ;

  if (isDone) return ;

  if (!(classClass = pickWord2Class ("Class")))
    messcrash ("No Class class") ;

  isDone = TRUE ;
}

/*************************************************************/

static KEY newId (IdCnxn ix)
{
  KEY idKey ;
  static int n = 0 ;		/* NB static to save position */
  OBJ obj ;

  while (!lexaddkey (messprintf (ix->domain->template, ++n), 
		     &idKey, ix->domain->objectClass)) ;

  if ((obj = bsUpdate (idKey)))
    { bsAddTag (obj, str2tag("Live")) ;
      bsSave (obj) ;
      setPublicName (ix->domain, idKey) ;
      return idKey ;
    }
  else
    { ix->error = BAD_ID ;
      return 0 ;
    }
}

/*************************************************************/

static BOOL setPublicName (DomainType *domain, KEY idKey)
{
  OBJ obj = bsUpdate (idKey) ;
  KEY nameKey ;
  BOOL result = FALSE ;
  BOOL gotit = FALSE ;
  int i ;

  if (!obj) 
    return FALSE ;

  for (i = 0 ; i < arrayMax(domain->types) ; ++i)
    if (bsGetKey (obj, arrp(domain->types, i, NameType)->tag, &nameKey))
      { result = bsAddKey (obj, str2tag("Public_name"), nameKey) ;
	gotit = TRUE ;
      }

  if (!gotit)
    { lexaddkey (name(idKey), &nameKey, domain->nameClass) ;
      bsAddKey (obj, str2tag("Public_name"), nameKey) ;
    }
  
  bsSave (obj) ;
  return result ;
}

/*************************************************************/

static void updateHistory (KEY id, char* user,
			   char* event, KEY target, 
			   NameType *type, char* newName)
{
  OBJ obj = bsUpdate (id) ;
  int version = 0 ;
  mytime_t time = timeParse ("now") ;

  if (!obj) 
    { messerror ("empty obj in updateHistory") ;
      return ;
    }

  bsGetData (obj, str2tag("Name_version"), _Int, &version) ;
  ++version ;
  if (!bsAddData (obj, str2tag("Name_version"), _Int, &version))
    messerror ("updateHistory could not update Name_version") ;

  if (bsAddData (obj, str2tag("Version_change"), _Int, &version) &&
      bsAddData (obj, _bsRight, _DateType, &time) &&
      bsAddData (obj, _bsRight, _Text, user) &&
      bsPushObj (obj))
    { if (!bsAddTag (obj, str2tag(event)))
        messerror ("Couldn't add event %s in updateHistory()", 
		   event) ;
      if (target && !bsAddData (obj, _bsRight, _Text, name(target)))
	messerror ("Couldn't add target %s in updateHistory()", 
		   target) ;
      if (type && !target && !bsAddData (obj, _bsRight, _Text, ""))
	messerror ("Couldn't add dummy target in updateHistory()") ;
      if (type && !bsAddData (obj, _bsRight, _Text, name(type->tag)))
        messerror ("Couldn't add nameType %s in updateHistory()", 
		   name(type->tag)) ;
      if (newName && !bsAddData (obj, _bsRight, _Text, newName))
        messerror ("Couldn't add name %s in updateHistory()", 
		   newName) ;
    }
  else
    messerror ("updateHistory could not add Version_change") ;

  bsGoto (obj, 0) ;		/* drop out of subtype */

  if (!strcmp (event, "Merged_into"))
    bsAddKey (obj, str2tag(event), target) ;
  else if (!strcmp (event, "Split_from"))
    bsAddKey (obj, str2tag(event), target) ;
		/* don't need reciprocals since done by XREF */

  bsSave (obj) ;
  return ;
}

/*************************************************************/
/****** command.c interface **********************************/
/*************************************************************/

/** Basic structure is acquired from gifcontrol, but this
 ** is considerably simpler.
 **
 ** I decided to make it stateless, passing the class every time.
 ** This makes server/client interaction MUCH simpler.
 **
 ** All commands (except ? and quit) return a status line first,
 ** then other return values only if status was SUCCESS.
 **/

static FREEOPT options[] =
{ { 16, "acedb-id" },
  { '?', "? : this list of commands.  Multiple commands ';' separated accepted" },
  { 'C', "create <domain> <nametype> <name> : returns id" },
  { 'K', "kill <domain> <id>" },
  { 'R', "resurrect <domain> <id> : only if killed, not merged" },
  { 'M', "merge <domain> <id> <target_id>" },
  { 'S', "split <domain> <id> <nametype> <name> : returns id" },
  { '+', "add_name <domain> <id> <nametype> <name>" }, 
  { '-', "remove_name <domain> <id> <nametype> <name>" },
  { 'g', "get_by_name <domain> <nametype> <name> : returns id list" },
  { 'p', "public_name <domain> <id> : returns name" },
  { 't', "typed_name <domain> <id> <nametype> : returns name list" },
  { 'v', "version <domain> <id> : returns version" },
  { 'l', "live <domain> <id> : returnus TRUE of FALSE" },
  { 'u', "ultimate <domain> <id> : follows Merged_into recursively" },
  { 'c', "children <domain> <id> : self and all recursive Merged_into and Split_into children" },
  { 'e', "exists <domain> <id> : TRUE or FALSE" }
} ;

extern char* idErrorString[] ;		/* indexed on IdError */

				/* two utilities */
static BOOL statusOK (ACEOUT fo, IdCnxn ix) ;
static char* nextWord (ACEIN fi, STORE_HANDLE h) ;

/******** temporary hack for server until userSessions fixed ********/

#ifndef NEW_SESSION_CODE
char* idUser (char* user)
{
  static char* userID = 0 ;

  if (user)
    { if (userID) messfree (userID) ;
      userID = strnew (user, 0) ;
    }
  else
    userID = getLogin (TRUE) ;

  return userID ;
}
#endif

void idControl (KEYSET ks, ACEIN fi, ACEOUT fo)
{ 
  KEY option ;
  int i ;

  if (aceInKey (fi, &option, options)) /* something on line */
    { if (option == '?')	/* help */
	for (i = 1 ; i <= options[0].key ; i++)
	  aceOutPrint (fo, "// %s\n", options[i].text) ;

      else			/* real commands  */
	{
	  STORE_HANDLE h = handleCreate () ;
	  IdCnxn ix ;
	  IdError err ;
	  char *domain = nextWord (fi, h) ;

#ifdef NEW_SESSION_CODE
	  ix = idaConnect (0, 0, sessionUserSession2User(0), 0, &err) ;
#else
	  ix = idaConnect (0, 0, idUser(0), 0, &err) ;
#endif

	  if (err)
	    { aceOutPrint (fo, "STATUS %s\n", idErrorString[err]) ;
	      return ;
	    }

	  switch (option)
	    { 
	    case 'C':		/* create */
	      { ID id = idaCreate (ix, domain, 0) ;
	        if (statusOK (fo, ix))
		  aceOutPrint (fo, "%s\n", (char*)id) ;
	      }
	      break ;

	    case 'K':		/* kill */
	      idaKill (ix, domain, nextWord(fi,h), 0) ;
	      statusOK (fo, ix) ;
	      break ;

	    case 'R':		/* resurrect */
	      idaResurrect (ix, domain, nextWord(fi,h), 0) ;
	      statusOK (fo, ix) ;
	      break ;

	    case 'M':		/* merge */
	      { char *id1 = nextWord(fi,h) ;
		char *id2 = nextWord(fi,h) ;
		idaMerge (ix, domain, id1, id2, 0) ;
		statusOK (fo, ix) ;
	      }
	      break ;

	    case 'S':		/* split */
	      { char *id1 = nextWord(fi,h) ;
		ID id = idaSplit (ix, domain, id1, 0) ;
	        if (statusOK (fo, ix))
		  aceOutPrint (fo, "%s\n", (char*)id) ;
	      }
	      break ;

	    case '+':		/* add_name */
	      { char *id = nextWord(fi,h) ;
		char *type = nextWord(fi,h) ;
		char *nam = nextWord(fi,h) ;
		idaAddName (ix, domain, id, type, nam, 0) ;
		statusOK (fo, ix) ;
	      }
	      break ;

	    case '-':		/* remove_name */
	      { char *id = nextWord(fi,h) ;
		char *type = nextWord(fi,h) ;
		char *nam = nextWord(fi,h) ;
		idaRemoveName (ix, domain, id, type, nam, 0) ;
		statusOK (fo, ix) ;
	      }
	      break ;

	    case 'g':		/* get_by_name */
	      { char *type = nextWord(fi,h) ;
		char *nam = nextWord(fi,h) ;
		ID* idList = idaGetByName (ix, domain, type, nam) ;
	        ID* ip = idList ;
	        if (statusOK (fo, ix))
		  { while (*ip) 
		      aceOutPrint (fo, "%s\n", (char*)*ip++) ;
		    messfree (idList) ;
		  }
	      }
	      break ;

	    case 'p':		/* public_name */
	      { char* cp = idaPublicName (ix, domain, nextWord(fi,h)) ;
	        if (statusOK (fo, ix))
		  aceOutPrint (fo, "%s\n", cp) ;
	      }
	      break ;

	    case 't':		/* typed_name */
	      { char *id = nextWord(fi,h) ;
		char *type = nextWord(fi,h) ;
		char** namList = idaTypedNames (ix, domain, id, type) ;
	        char** nam = namList ;
	        if (statusOK (fo, ix))
		  { while (*nam) 
		      aceOutPrint (fo, "%s\n", *nam++) ;
		    messfree (namList) ;
		  }
	      }
	      break ;

	    case 'v':		/* version */
	      { int v = idaVersion (ix, domain, nextWord(fi,h)) ;
	        if (statusOK (fo, ix))
		  aceOutPrint (fo, "%d\n", v) ;
	      }
	      break ;

	    case 'l':		/* live */
	      { BOOL x = idaLive (ix, domain, nextWord(fi,h)) ;
	        if (statusOK (fo, ix))
		  aceOutPrint (fo, "%s\n", x ? "TRUE" : "FALSE") ;
	      }
	      break ;

	    case 'u':		/* ultimate */
	      { ID id = idaUltimate (ix, domain, nextWord(fi,h)) ;
	        if (statusOK (fo, ix))
		  aceOutPrint (fo, "%s\n", (char*)id) ;
	      }
	      break ;

	    case 'c':		/* children */
	      { ID* idList = idaChildren (ix, domain,nextWord(fi,h));
	        ID* ip = idList ;
	        if (statusOK (fo, ix))
		  { while (*ip) 
		      aceOutPrint (fo, "%s\n", (char*)*ip++) ;
		    messfree (idList) ;
		  }
	      }
	      break ;

	    case 'e':		/* exists */
	      { BOOL res = idaExists (ix, domain, nextWord(fi,h)) ;
	        if (statusOK (fo, ix))
		  if (res) 
		    aceOutPrint (fo, "TRUE\n") ;
		  else
		    aceOutPrint (fo, "FALSE\n") ;
	      }
	      break ;

	    } /* end of switch */

	  handleDestroy (h) ;	/* frees memory */
	  idConnectionDestroy (ix) ;

	} /* end of else (a real command) */
    }
  else	/* nothing typed on line */
    aceOutPrint (fo, "// ID command expects a subcommand - use ID ? for more help\n") ;
}

static BOOL statusOK (ACEOUT fo, IdCnxn ix)
{
  if (ix)
    { aceOutPrint (fo, "STATUS %s\n", idErrorString[ix->error]) ;
      return ix->error == SUCCESS ? TRUE : FALSE ;
    }
  else
    { aceOutPrint (fo, "STATUS BAD_CONNECTION\n") ;
      return FALSE ;
    }
}

static char* nextWord (ACEIN fi, STORE_HANDLE h)
{ 
  char *word = aceInWord (fi) ;
  if (word) return strnew (word, h) ;
  else return 0 ;
}


/***************************************************************/
/********* domain cache ****************************************/

static DICT *domainDict = 0 ;
static Array domains ;

static DomainType *getDomain (char *domain)
{
  int n ;
  KEY key ;
  OBJ obj = 0 ;			/* suppresses compiler warning */
  char *text, *temp ;
  BSMARK mark = 0 ;
  NameType *type ;
  DomainType *dom ;

  if (!domainDict)
    { domainDict = dictCreate (16) ;
      domains = arrayCreate (16, DomainType) ;
    }
  
  if (!domain || !*domain)
    return 0 ;

  if (dictFind (domainDict, domain, &n))
    return arrp(domains, n, DomainType) ;

  /* domain is the name of a class, 
     which must have a template and at least one name type
     */
  if (!pickWord2Class (domain) ||
      !pickWord2Class (messprintf ("%s_name", domain)) ||
      !lexword2key (domain, &key, classClass) ||
      !(obj = bsCreate (key)) || 
      !bsGetData (obj, str2tag("Id_template"), _Text, &temp) ||
      !bsGetData (obj, str2tag("Id_nametype"), _Text, &text))
    { if (obj) bsDestroy (obj) ;
      return 0 ;
    }
  
  dictAdd (domainDict, domain, &n) ;
  dom = arrayp (domains, n, DomainType) ;
  dom->objectClass = pickWord2Class (domain) ;
  dom->nameClass = pickWord2Class (messprintf ("%s_name", domain)) ;
  dom->template = strnew (temp, 0) ;
  dom->typeDict = dictCreate (16) ;
  dom->types = arrayCreate (16, NameType) ;
  do	/* get information about name types */
    { dictAdd (dom->typeDict, text, &n) ;
      type = arrayp (dom->types, n, NameType) ;
      type->text = strnew (text, 0) ;
      type->tag = str2tag (messprintf ("%s_name", text)) ;
      mark = bsMark (obj, mark) ;
      bsPushObj (obj) ;
      type->isUnique = bsFindTag (obj, str2tag ("Is_unique")) ;
      type->isPrimary = bsFindTag (obj, str2tag ("Is_primary")) ;
      bsGoto (obj, mark) ;
    } while (bsGetData (obj, _bsDown, _Text, &text)) ;
  
  bsDestroy (obj) ;		/* the classClass object */

  return dom ;
}

/********************* end of file ******************/
