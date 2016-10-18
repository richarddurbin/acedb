/*  File: aceclient.h
 *  Author: Jean Thierry-Mieg
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
 * Last edited: Aug 26 16:43 1999 (fw)
 * Created: Thu Aug 26 16:42:43 1999 (fw)
 * CVS info:   $Id: aceclient.h,v 1.6 1999-09-01 11:01:41 fw Exp $
 *-------------------------------------------------------------------
 */

#ifndef ACEDB_ACECLIENT_H
#define ACEDB_ACECLIENT_H


#define DEFAULT_PORT 0x20000101

#define ACE_UNRECOGNIZED 100
#define ACE_OUTOFCONTEXT 200
#define ACE_INVALID      300
#define ACE_SYNTAXERROR  400

#define HAVE_ENCORE   -1
#define WANT_ENCORE   -1
#define DROP_ENCORE   -2

struct ace_handle {
	int clientId;
	int magic;
	void *clnt;
};
typedef struct ace_handle ace_handle;

extern ace_handle *openServer(char *host, unsigned long rpc_port, int timeOut);
extern void closeServer(ace_handle *handle);
extern int askServer(ace_handle *handle, char *request, char **answerPtr, int chunkSize) ; 
extern int askServerBinary(ace_handle *handle, char *request, unsigned char **answerPtr, 
			   int *answerLength, int *encorep, int chunkSize) ; 

/* do not write behind this line */
#endif /* !ACEDB_ACECLIENT_H */

