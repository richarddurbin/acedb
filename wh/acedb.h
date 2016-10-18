/*  File: acedb.h
 *  Author: Jean Thierry-Mieg (mieg@mrc-lmb.cam.ac.uk)
 *  Copyright (C) J Thierry-Mieg and R Durbin, 1991
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
 * 	Richard Durbin (MRC LMB, UK) rd@mrc-lmb.cam.ac.uk, and
 *	Jean Thierry-Mieg (CRBM du CNRS, France) mieg@kaa.cnrs-mop.fr
 *
 * Description: general include for any acedb code
 *              can only declare general stuff, 
 *              i.e. not graph-specific things
 * HISTORY:
 * Last edited: May 14 17:38 2009 (edgrif)
 * * Apr 21 13:23 1999 (fw): removed mainActivity decls (gone to regular.h)
 * * Nov 18 17:00 1998 (fw): added decl for mainActivityRegister stuff
 * * Nov 18 16:59 1998 (fw): moved pickDraw, getPickArgs to main.h
 *              as they are for graphical versions
 * * Oct 22 11:43 1998 (edgrif): Add dec. of pickDraw.
 * * Sep 17 09:43 1998 (edgrif): Add declaration of pickGetArgs function.
 * * Oct 21 14:01 1991 (mieg): added overflow  protection in KEYMAKE 
 * Created: Mon Oct 21 14:01:19 1991 (mieg)
 *-------------------------------------------------------------------
 */

/* $Id: acedb.h,v 1.28 2009-05-29 13:16:23 edgrif Exp $ */
 
/***************************************************/
/* acedb.h                                         */
/* global definitions used by every ace-sourcefile */
/***************************************************/
 
#ifndef ACEDB_ACEDB_H
#define ACEDB_ACEDB_H
 
#include <wh/regular.h>
#include <wh/aceiotypes.h>		/* for ACEIN/ACEOUT type */
#include <wh/mytime.h>		/* for mytime_t type */
#include <wh/keyset.h>		/* for KEYSET type */
#include <wh/aceversion.h>


/***************************************************************/

/* define the direction of searching for find functions */
typedef enum { FIND_FORWARD,  FIND_BACKWARD } AceFindDirection;


/****************************************************************/

#define KEYMAKE(t,i)  ((KEY)( (((KEY) (t))<<24) | ( ((KEY) (i)) & 0xffffffL) ))
#define KEYKEY(kk)  ((KEY)( ((KEY) (kk)) & ((KEY) 0xffffffL) ))
#define class(kk)  ((int)( ((KEY) (kk))>>24 ))


/* For efficiency these functions return pointers to internal acedb data so don't mess with the
 * strings !! If they can't find the key/class they return "NULL KEY".
 *  */
char *name(KEY k);					    /* returns   name */
char *className(KEY k);					    /* returns   class */
char *nameWithClass(KEY k) ;				    /* returns   class:name   */
char *nameWithClassDecorate(KEY k) ;			    /* returns   class:"name" */

BOOL keyIsStr(KEY key, char *str) ;			    /* TRUE if str matches key (case insensitive) */
BOOL keyIsExactStr(KEY key, char *str) ;		    /* TRUE if str matches key (case sensitive) */
BOOL keyHasStr(KEY key, char *sub_str) ;		    /* TRUE if sub_str is in key (case sensitive) */

KEY str2tag(char* tagName) ;				    /* returns   key of tag. */

/****************************************************************/

/* The new display function allows user data to be passed by the caller of   */
/* display() to the eventual display routine, hence the new function type    */
/* below. Currently only pmap and fmap have display_data that can be passed  */
/* in but this is likely to grow.                                            */
typedef BOOL (*DisplayFunc)(KEY key, KEY from, BOOL isOld, void *display_data) ;

typedef BOOL (*DumpFuncType)(ACEOUT dump_out, KEY key) ;
typedef BOOL (*KillFunc)(KEY k) ;
typedef void (*BlockFunc)(KEY) ;


/********* some utilities for acedb programs (aceutils.c) *******/

extern VoidRoutine messcrashroutine ;

/* assign messcrash routine to this to get minimum cleanup 
 * functionality in case of crash */
void simpleCrashRoutine (void);
    
/* used as user_func in context for messOutRegister,
 * acedb-style output to (ACEOUT)user_pointer */
void acedbPrintMessage (char *message, void *user_pointer);


#endif /* !ACEDB_ACEDB_H */


/*************************** eof ********************************/
