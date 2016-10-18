/*  File: session_.h
 *  Author: Fred Wobus (fw@sanger.ac.uk)
 *  Copyright (C) J Thierry-Mieg and R Durbin, 1998
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
 *	Jean Thierry-Mieg (CRBM du CNRS, France) mieg@crbm.cnrs-mop.fr
 *
 * Description:
 * Exported functions:
 * HISTORY:
 * Last edited: Feb  5 12:57 2003 (edgrif)
 * Created: Mon Nov 23 17:21:38 1998 (fw)
 *-------------------------------------------------------------------
 */


#ifndef _SESSION__H
#define _SESSION__H

#ifdef ACEDB5

typedef struct block /* sizeof(block) must be < BLOC_SIZE */
  { 
    DISK gAddress ;
    int mainRelease ,  subDataRelease, subCodeRelease , subSubCodeRelease ;
    int session ;
    char dbName[32] ; /* added as of release 4.3 */
    int magic ; /* added feb 98: add between read and write */
  } BLOCK;   /* super block info */

#else  /* !ACEDB5 */

#define BLKMX (BLOC_SIZE - sizeof(BLOCKHEADER) - sizeof(DISK)\
	       -sizeof(int) -sizeof(int) -sizeof(int) - sizeof(int)\
	       - (32*sizeof(char)) -sizeof(int) -sizeof(int))

typedef struct block /* sizeof(block) must be < BLOC_SIZE */
  { BLOCKHEADER  h ;
    DISK gAddress ;
    int mainRelease ,  subDataRelease, subCodeRelease ;
    int byteSexIndicator;
    char dbName[32] ; /* added as of release 4.3 */
    char c[BLKMX] ;
    int magic ; /* added feb 98: add between read and write */
    int alternateMainRelease ; /* for timestamp enforcement */
  } BLOCK;   /*the transfer unit between disk and cache*/

#endif /* !ACEDB5 */


/******* session tree **********/
typedef struct STstructure *STP ;
typedef struct STstructure {
  int number;
  KEY key, father, ancester ;
  STP sta ;
  BOOL isDestroyed, isReadlocked, isPermanent ;
  char *user;
  char *date;
  char *title; 
  int generation, x, y, len;
} ST ;


/* Session state data, intent is that globals from session.c will gradually be
 * migrated into here and in the end the whole thing will become a context
 * and be passed into all session calls.
 * For now it is hanging off a global pointer which is retrieved via getContext(). */
typedef struct
{
  char *unused ;
} SessionContextStruct, *SessionContext ;

SessionContext getContext(void) ;


/*********** function private to the session package ********/

BOOL I_own_the_passwd(char *for_what);

Array sessionTreeCreate(BOOL IsDeadSessionShown); /* array of ST */
void sessionTreeDestroy(Array sessionTree);

void sessionInitialize (void);	/* set start time of session
				   and init userKey */
void sessionStart(KEY fromSession);

VoidRoutine sessionChangeRegister (VoidRoutine func);

BOOL sessionDoClose (void) ;	/* returns TRUE if 
				   modifications had been saved */


/************************************************************/

#endif /* _SESSION__H */
