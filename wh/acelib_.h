/*  File: acelib_.h
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
 * Last edited: Oct 15 15:51 1999 (fw)
 * * Aug 26 16:48 1999 (fw): added this header
 * Created: Thu Aug 26 16:47:39 1999 (fw)
 * CVS info:   $Id: acelib_.h,v 1.12 1999-10-15 15:09:03 fw Exp $
 *-------------------------------------------------------------------
 */

#ifndef ACELIB__H
#define  ACELIB__H

#include "acetypes.h"

#include "acedb.h"
#include "liste.h"
#include "table.h"
#include "whooks/sysclass.h"
#include "bs.h"

static int ACEMAGIC = 0 ;
static int INSMAGIC = 0 ;

typedef struct aceContextStruct { 
  char *clientSignature ;
  STORE_HANDLE  handle ; 
  void *magic ;
  int id ; /* returns this number as the handle, safest system ! */
  int instanceId ;
  int pushedFrom ;  /* push/popContext */
  Liste writeLockListe, readLockListe, instanceListe ;
  char *errMessage;
  int errNb ;
} AceContext ;

typedef enum { INSTANCE_NO_LOCK, 
	       INSTANCE_READ_LOCK,
	       INSTANCE_WRITE_LOCK,
	       INSTANCE_DIRTY_LOCK } InstanceLockType;

typedef struct instanceStruct { 
  void *magic ;
  InstanceLockType lock;
  int id ; /* returns this number as the handle, safest system ! */
  KEY key ;
  OBJ obj ;
  int slot ;  /* slot in instanceListe */
} *Instance ;

#define CHECKACE if(!activeContext) {NOCONTEXT;}

#define CHECKACE2 AceHandle h1 = hh ? hh : activeHandle ; \
                   if(!activeContext) {NOCONTEXT;}

#define CHECKINS(_ins) if(!(_ins) || (_ins)->magic != &INSMAGIC || \
			  (_ins)->id != activeContext->id) \
		   return FALSE 

#define CHECKWINS(_ins) CHECKINS(_ins) ;\
                        if((_ins)->lock < 2) \
                          { aceSetErrorMessage("Unlocked instance") ;\
			  return FALSE ; }

#define NOCONTEXT  aceSetErrorMessage("No Active Aced context") ;\
		   aceErrno = NOCONTEXTERR ; \
		   return FALSE  
		   
#define NOTDONE    aceSetErrorMessage("sorry, not yet supported")  ; \
		   aceErrno = NOTDONEERR ; \
                   return FALSE  

#define Bool BOOL
#define BLOCKING 

typedef Array    AceStringSet;
typedef Array    AceInstanceSet;
typedef KEYSET   AceKeySet;
typedef STORE_HANDLE   AceHandle;
typedef Instance AceInstance;
typedef BSMARK   AceMark;
typedef TABLE*   AceTable;
typedef KEY      AceKey;
typedef KEY      AceTag;
typedef mytime_t AceDateType;

#endif /* !ACELIB__H */
 
