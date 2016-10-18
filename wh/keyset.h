/*  File: keyset.h
 *  Author: R Durbin (rd@sanger.ac.uk)
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
 * Description: public header for keyset operations.
 *              This file is part of acedb.h and NOT to be included
 *              by other source files.
 *              The KEYSET operations are built upon the Array ops
 *              provided by the utilities library libfree.a
 * Exported functions:
 * HISTORY:
 * Last edited: Jul 28 20:59 2000 (edgrif)
 * Created: Fri Dec 11 09:42:41 1998 (fw)
 * CVS info:   $Id: keyset.h,v 1.19 2012-10-22 14:57:58 edgrif Exp $
 *-------------------------------------------------------------------
 */
#ifndef ACEDB_KEYSET_H
#define ACEDB_KEYSET_H

#include <wh/acedb.h>


/***************************************************************/
/*  a KEYSET is an ordered array of KEYs.                         */
/***************************************************************/

typedef Array KEYSET ;  /* really KEYSET = array(,,KEY) always ordered */

/* ghastly hack, parse.h needs KEYSET, keyset.h needs parse.h etc. etc.      */
#include <wh/parse.h>					    /* for parsefunc stuff. */

#define keySetCreate()		arrayCreate(32,KEY)
#define keySetHandleCreate(h)	arrayHandleCreate(32,KEY,h)
#define keySetReCreate(s)	arrayReCreate(s,32,KEY)
#define keySet(s,i)		array(s,i,KEY)
#define keySetDestroy(s)	arrayDestroy(s)

#define keySetInsert(s,k)	arrayInsert(s,&(k),keySetOrder)
#define keySetRemove(s,k)	arrayRemove(s,&(k),keySetOrder)
#define keySetSort(s)		arraySort((s),keySetOrder) 
#define keySetCompress(s)	arrayCompress(s)
#define keySetFind(s,k,ip)	arrayFind ((s),&(k),(ip),keySetOrder)
#define keySetMax(s)		arrayMax(s)
#define keySetExists(s)		(arrayExists(s) && (s)->size == sizeof(KEY))
#define keySetCopy(s)		arrayCopy(s)

KEYSET  keySetAND (KEYSET x, KEYSET y) ;
KEYSET  keySetOR (KEYSET x, KEYSET y) ;
KEYSET  keySetXOR (KEYSET x, KEYSET y) ;
KEYSET  keySetMINUS (KEYSET x, KEYSET y) ;
int     keySetOrder (void *a, void*b) ;
int     keySetAlphaOrder (void *a, void*b) ;
KEYSET  keySetHeap (KEYSET source, int nn, int (*order)(KEY *, KEY *)) ;
KEYSET  keySetNeighbours (KEYSET ks) ;
KEYSET  keySetEmptyObj(KEYSET kset) ;
KEYSET  keySetAlphaHeap (KEYSET ks, int nn) ;    /* jumps aliases/deletes */
KEYSET  keySetAlphaHeapAll (KEYSET ks, int nn) ; /* do not jump aliases */
int	keySetCountVisible (KEYSET ks) ;

void    keySetKill (KEYSET ks) ; /* kills all 'A' and 'B' members of ks */

/* in keysetdump.c */
BOOL keySetDump (ACEOUT dump_out, KEYSET s);

/* DumpFuncType for class _VKeyset
 * dumps arrayGet(key) */
BOOL keySetDumpFunc (ACEOUT dump_out, KEY key) ; 

ParseFuncReturn keySetParse (ACEIN fi, KEY key, char **errtext) ; /* parseFunc */
KEYSET keySetRead (ACEIN keyset_in, ACEOUT error_out)  ; /* recognises know objects */


#endif /* !ACEDB_KEYSET_H */
/*************************************************************/


