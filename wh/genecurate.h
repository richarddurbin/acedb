/*  File: genecurate.h
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
 * Description:
 * Exported functions:
 * HISTORY:
 * Last edited: Jan 23 02:24 2002 (rd)
 * Created: Tue Jan 22 23:28:37 2002 (rd)
 * CVS info: $Id: genecurate.h,v 1.1 2002-03-04 04:01:38 rd Exp $
 *-------------------------------------------------------------------
 */

/* this is all WormBase specific just now */

typedef struct GeneConnectionStruct *GeneConnection ;
typedef enum { CGC_NAME, SEQUENCE_NAME, OTHER_NAME, 
	       PUBLIC_NAME, ANY_NAME } GeneNameType ;

/* Initially implement the following locally, since I want them for 
   fmapcurate.c.
   Once a server exists they need reimplementing so they talk to
   the server, but I expect at least in the xace implementation
   that they will then keep a local copy of the info in this client
   synchronised by the client on request (see geneUpdate()).
*/

GeneConnection  geneConnection (char *host, char* user, KEY species,
				STORE_HANDLE handle) ; 
  /* must initialise with this; host can be "<host>:<port>" */
#define geneConnectionDestroy(x) { (x) ? messfree(x), (x)=0, TRUE : FALSE ; }

KEY   geneCreate (GeneConnection gx, GeneNameType type, char* name) ;
BOOL  geneKill (GeneConnection gx, KEY gene) ;
BOOL  geneResurrect (GeneConnection gx, KEY gene) ;
BOOL  geneMerge (GeneConnection gx, KEY gene, KEY target) ;
KEY   geneSplit (GeneConnection gx, KEY gene, 
		 GeneNameType type, char *name) ;

int   geneNameCount (GeneConnection gx, 
		     GeneNameType type, char *name) ;
KEY   geneNameGet (GeneConnection gx, 
		   GeneNameType type, char *name, int n) ;
  /* the second of these gets the n'th key for this name */

BOOL  geneDescendant (GeneConnection gx, KEY *gene) ; 
  /* follow Merged_into recursively, then return TRUE if live */

/* will probably want at some point the following */
#ifdef FUTURE

BOOL  geneUpdate (GeneConnection gx, KEY gene) ;	
  /* updates local copy of info; will be needed when server exists */
BOOL  geneUpdateAll (GeneConnection gx) ;

#endif

/************** end of file **************/
