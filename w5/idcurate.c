/*  File: idcurate.c
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
 * Description: implementation of idcurate.h
 		if local passes on to ida* in idacedb.c
		else connects to some server (to be worked out!)
		doesn't look very interesting
 * Exported functions:
 * HISTORY:
 * Last edited: Apr  8 13:09 2002 (edgrif)
 * Created: Fri Mar  8 19:12:02 2002 (rd)
 * CVS info: $Id: idcurate.c,v 1.3 2002-04-09 13:35:10 edgrif Exp $
 *-------------------------------------------------------------------
 */

#include <wh/acedb.h>
#include <w5/idacedb.h>					    /* local acedb implementation */

char* idErrorString[] = { /* must match IdError enum */
  "SUCCESS", "NO_PERMISSION", "BAD_CONNECTION", "BAD_DOMAIN", 
  "BAD_ID", "BAD_TARGET", "BAD_NAME_TYPE", "BAD_NAME", 
  "DUPLICATE_NAME", "BAD_PASSWORD", "UNIMPLEMENTED", 
  "DATABASE_ERROR", "INTEGRITY_ERROR" } ;

/******************************************************************/
/** first the pass-through functions to server or local ida*() ****/

IdCnxn  idConnect (char *host, int port, 
		   char *user, char *password, IdError *err)
{ return idaConnect (host, port, user, password, err) ; }

IdError idError (IdCnxn ix)
{ return idaError (ix) ; }

char* idErrorText (IdCnxn ix)
{ return idErrorString[idaError(ix)] ; }

ID      idCreate (IdCnxn ix, char *domain)
{ return idaCreate (ix, domain, 0) ; }

BOOL    idKill (IdCnxn ix, char *domain, ID id)
{ return idaKill (ix, domain, id, 0) ; }

BOOL    idResurrect (IdCnxn ix, char *domain, ID id)
{ return idaResurrect (ix, domain, id, 0) ; }

BOOL    idMerge (IdCnxn ix, char *domain, ID source, ID target)
{ return idaMerge (ix, domain, source, target, 0) ; }

ID      idSplit (IdCnxn ix, char *domain, ID id)
{ return idaSplit (ix, domain, id, 0) ; }

BOOL    idAddName (IdCnxn ix, char *domain, ID id, char*nameType, char *name)
{ return idaAddName (ix, domain, id, nameType, name, 0) ; }

BOOL    idRemoveName (IdCnxn ix, char *domain, ID id, char*nameType, char *name)
{ return idaRemoveName (ix, domain, id, nameType, name, 0) ; }

ID*     idGetByAnyName (IdCnxn ix, char *domain, char* name)
{ return idaGetByName (ix, domain, 0, name) ; }

ID*     idGetByTypedName (IdCnxn ix, char *domain, char* nameType, char* name)
{ return idaGetByName (ix, domain, nameType, name) ; }

ID      idFind (IdCnxn ix, char *domain, char *nameType, char *name)
{ return idaFind (ix, domain, nameType, name) ; }

char*   idPublicName (IdCnxn ix, char *domain, ID id)
{ return idaPublicName (ix, domain, id) ; }

char**  idTypedNames (IdCnxn ix, char *domain, ID id, char *nameType)
{ return idaTypedNames (ix, domain, id, nameType) ; }

char*   idTypedName (IdCnxn ix, char *domain, ID id, char *nameType)
{ return idaTypedName (ix, domain, id, nameType) ; }

int     idVersion (IdCnxn ix, char *domain, ID id)
{ return idaVersion (ix, domain, id) ; }

BOOL    idLive (IdCnxn ix, char *domain, ID id)
{ return idaLive (ix, domain, id) ; }

ID      idUltimate (IdCnxn ix, char *domain, ID id)
{ return idaUltimate (ix, domain, id) ; }

ID*    idChildren (IdCnxn ix, char *domain, ID id)
{ return idaChildren (ix, domain, id) ; }

BOOL   idExists (IdCnxn ix, char *domain, ID id)
{ return idaExists  (ix, domain, id) ; }

IdHistory* idHistory (IdCnxn ix, char *domain, ID id, int version)
{ return idaHistory (ix, domain, id, version) ; }

ID* idChanged (IdCnxn ix, char *domain, char* timeStamp)
{ return idaChanged (ix, domain, timeStamp) ; }

IdError idCreateDomain (IdCnxn ix, char *domain)
{ return idaCreateDomain (ix, domain) ; }

IdError idSetTemplate (IdCnxn ix, char *domain,  char *template)
{ return idaSetTemplate (ix, domain, template) ; }

IdError idAddNameType (IdCnxn ix, char *domain,
			BOOL isUnique, BOOL isPrimary)
{ return idaAddNameType (ix, domain, isUnique, isPrimary) ; }

/*********** end of file **************/
