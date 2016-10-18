/*  File: idacedb.h
 *  Author: Ed Griffiths (edgrif@sanger.ac.uk)
 *  Copyright (c) J Thierry-Mieg and R Durbin, 2002
 *-------------------------------------------------------------------
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
 *-------------------------------------------------------------------
 * This file is part of the ACEDB genome database package, written by
 * 	Richard Durbin (Sanger Centre, UK) rd@sanger.ac.uk, and
 *	Jean Thierry-Mieg (CRBM du CNRS, France) mieg@kaa.crbm.cnrs-mop.fr
 *
 * Description: 
 * Exported functions: See XXXXXXXXXXXXX.h
 * HISTORY:
 * Last edited: Apr 29 08:42 2002 (edgrif)
 * Created: Mon Apr  8 11:27:29 2002 (edgrif)
 * CVS info:   $Id$
 *-------------------------------------------------------------------
 */

#ifndef IDCURATE_ACEDB_H
#define IDCURATE_ACEDB_H

#include <wh/idcurate.h>				    /* curate interface header. */

ID idaCreate (IdCnxn ix, char* domain, mytime_t date) ;
IdCnxn idaConnect (char *host, int port,
		   char *user, char *password, IdError *err) ;
ID idaSplit (IdCnxn ix, char *domain, ID id, mytime_t date) ;
ID idaFind (IdCnxn ix, char *domain, char *nameType, char *nam) ;
ID* idaGetByName (IdCnxn ix, char *domain, char* nameType, char* nam) ;
char* idaPublicName (IdCnxn ix, char *domain, ID id) ;
char* idaTypedName (IdCnxn ix, char *domain, ID id, char *nameType) ;
char** idaTypedNames (IdCnxn ix, char *domain, ID id, char *nameType) ;
ID idaUltimate (IdCnxn ix, char *domain, ID id) ;
ID* idaChildren (IdCnxn ix, char *domain, ID id) ;
IdHistory* idaHistory (IdCnxn ix, char *domain, ID id, int version) ;
ID* idaChanged (IdCnxn ix, char *domain, char* timeStamp) ;

#endif /* !IDCURATE_ACEDB_H */
