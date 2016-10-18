/*  File: idcurate.h
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
 * Description: interface for generic ID curation, with associated
 	name management.
 * Exported functions:
 * HISTORY:
 * Last edited: Apr  8 11:38 2002 (edgrif)
 * * Mar 20 17:06 2002 (rd): modifications following meeting with
 	Lincoln at CalTech
 * Created: Tue Jan 22 23:28:37 2002 (rd)
 * CVS info: $Id: idcurate.h,v 1.3 2002-04-09 13:35:48 edgrif Exp $
 *-------------------------------------------------------------------
 */

#ifndef IDCURATE_H
#define IDCURATE_H

typedef char* ID ;

typedef struct IdConnectionStruct *IdCnxn ;

typedef enum { 
  SUCCESS=0, NO_PERMISSION, BAD_CONNECTION, BAD_DOMAIN, 
  BAD_ID, BAD_TARGET, BAD_NAME_TYPE, BAD_NAME, 
  DUPLICATE_NAME, BAD_PASSWORD, UNIMPLEMENTED, 
  DATABASE_ERROR, INTEGRITY_ERROR } IdError ;

typedef struct {
  int  version ;		/* versions start at 1 */
  char *event ;			/* see below */
  char *userName ;
  char *date ;			/* 2002-03-20_09:22:13 style */
  ID   target ;			/* for merge and split */
  char *nameType ;
  char *name ;
} IdHistory ;

/* event is one of:
	created       addName       splitFrom
	killed        delName       mergedTo
	resurrected   splitTo       mergedFrom
*/

/* Conventions for functions:
   Lists (of char* or IDs) are returned as 0 terminated lists.
   The user owns these, and must free them.
   The package owns strings and IDs, and the user must copy
   them if he wants their scope to extend beyond the next call
   to the connection object.
   Where functions return a value, this is set to zero if there
   is an error, where a BOOL, this is FALSE for an error.
   Further information can be obtained by calling idError() or
   idErrorText() on the connection before any other call to the 
   connection.
*/

/************ connection: make this domain specific *************/

IdCnxn  idConnect (char *host, int port, char *user, char *password, IdError *err) ;

#define idConnectionDestroy(x) { (x) ? messfree(x), (x)=0, TRUE : FALSE ; }

IdError idError (IdCnxn ix) ;
char*   idErrorText (IdCnxn ix) ; /* text, maybe passing on error from DB or transport */

/* must initialise with this - domain 0 for administrator space */

/****************** user interface first ************************/

/* edit operations */

ID     idCreate (IdCnxn ix, char *domain) ;
BOOL   idKill (IdCnxn ix, char *domain, ID id) ;
BOOL   idResurrect (IdCnxn ix, char *domain, ID id) ;
BOOL   idMerge (IdCnxn ix, char *domain, ID source, ID target) ; /* merge source into target */
ID     idSplit (IdCnxn ix, char *domain, ID id) ;
BOOL   idAddName (IdCnxn ix, char *domain, ID id, char *nameType, char *name) ;
BOOL   idRemoveName (IdCnxn ix, char *domain, ID id, char *nameType, char *name) ;

/* access functions */

ID*    idGetByAnyName (IdCnxn ix, char *domain, char* name) ;
ID*    idGetByTypedName (IdCnxn ix, char *domain, char* nameType, char* name) ;
ID     idFind (IdCnxn ix, char *domain, char *nameType, char *name) ;
  /* valid only if nameType is PRIMARY, else BAD_NAME_TYPE error */

/* properties */

char*  idPublicName (IdCnxn ix, char *domain, ID id) ;
char** idTypedNames (IdCnxn ix, char *domain, ID id, char *nameType) ;
char*  idTypedName (IdCnxn ix, char *domain, ID id, char *nameType) ;
  /* valid only if nameType is UNIQUE, else BAD_NAME_TYPE error  */
int    idVersion (IdCnxn ix, char *domain, ID id) ;
BOOL   idLive (IdCnxn ix, char *domain, ID id) ;
ID     idUltimate (IdCnxn ix, char *domain, ID id) ;
ID*    idChildren (IdCnxn ix, char *domain, ID id) ; /* union of self merge_into split_from */
BOOL   idExists (IdCnxn ix, char *domain, ID id) ;   /* check a proper id */
  /* follows Merged_into recursively */

/* need also history stuff */

IdHistory* idHistory (IdCnxn ix, char *domain, ID id, int version) ;
		/* history changes since (not including) version */
ID*        idChanged (IdCnxn ix, char *domain, char *timeStamp) ; 
                /* lists IDs that changed */

/**************** administrator interface ****************/

IdError idCreateDomain (IdCnxn ix, char *domain) ;
IdError idSetTemplate (IdCnxn ix, char *domain,  char *template) ;
IdError idAddNameType (IdCnxn ix, char *domain, BOOL isUnique, BOOL isPrimary) ;
  /* isUnique if only one name per Id
     isPrimary if name unique in domain/nameType
  */
char**  idGetDomains (IdCnxn ix) ;
char**  idGetNameTypes (IdCnxn ix, char *domain) ;

/*********************************************************/
#ifdef FUTURE_MAYBE
BOOL  idUpdate (IdCnxn ix, ID id) ;	
  /* updates local copy of info; will be needed when server exists */
BOOL  idUpdateAll (IdCnxn ix) ;
#endif

#endif /* IDCURATE_H */
/******** end of file ********/
