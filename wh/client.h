/*  File: client.h
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
 * Description:  communications used by the xace and tace clients 
 *              These func pointers are allocated in session.c
 *		and initialised in xclient.c
 * Exported functions: 
 * HISTORY:
 * Last edited: Aug 26 17:06 1999 (fw)
 * Created: Thu Aug 26 17:05:49 1999 (fw)
 * CVS info:   $Id: client.h,v 1.7 1999-09-01 11:06:37 fw Exp $
 *-------------------------------------------------------------------
 */

#ifndef ACEDB_CLIENT_H
#define ACEDB_CLIENT_H

#include "table.h"

/* 0: close, 1:open, -1:ask state of connection to server */
extern BOOL   (*externalServerState) (int nn) ;
/* Triggers a query on the server side, parses the returned ace file */
extern KEYSET (*externalServer) (KEY key, char *query, char *grep, BOOL getNeighbours) ;
/* A set of calls to export an ace diff of any edition
 * could be used more generally to produce a journal
 */
extern void (*externalSaver) (KEY key, int action) ;
extern void (*externalAlias) (KEY key, char *oldname, BOOL keepOldName) ;
/* sends the journal of ace diffs back to the server */
extern void (*externalCommit) (void) ;
extern KEYSET oldSetForServer ; /* declared in queryexe.c */
extern TABLE* (*externalTableMaker) (char *quer) ;
extern char*  (*externalServerName) (void) ; 

#endif /* !ACEDB_CLIENT_H */


