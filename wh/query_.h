/*  File: query_.h 1.2
 *  Author: Gary Aochi LBL
 *  Copyright (C) J Thierry-Mieg and R Durbin, 1993
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
 * Description:
 * Exported functions:
 * HISTORY:
 * Last edited: Dec 11 10:05 1998 (fw)
 * 	-	changed fn() => fn(void) function prototypes to please compiler
 * * May 21 17:49 1994 (rd)
 * Created: Wed Apr 28 11:39:17 1993 (mieg)
 *-------------------------------------------------------------------
 * gha 04/28/93  removed line "#define graphTextScroll..." (jtm had added it)
 * gha 04/29/93  removed #define CLASSLIST_MAX, classlist static to *.c files
 * gha 06/14/93  split querybuild.c from querydisp.c
 */

/* $Id: query_.h,v 1.5 1999-09-01 11:17:24 fw Exp $ */


#ifndef DEF_QUERY__H
#define DEF_QUERY__H

#include "acedb.h"

#include "graph.h"

#define BUFFER_SIZE 81   /* size of char buffer (strings) +1 for NULL. */
#define QBUFF_MULT  5    /* factor by buffer_size, for query command buffer */


/* in querybuild.c */
extern int qbuild_selected_class ;  /* last entered/used class in a query */

/* in querydisp.c */
extern char resbuffer[QBUFF_MULT*BUFFER_SIZE]; /* for forming query commands */
extern KEYSET query_undo_set;

extern Graph querGraph ;
extern Graph qbeGraph ;
extern Graph qbuildGraph ;
     
#define ARR2STRING(a, i)  arrayp(array(a, i, Array), 0, char)
#include "pick.h"       /* for pickClass2Word(t) and pickVocList[] */


/* following defined in querydisp.c */
extern void queryCreate (void);
extern void queryWindowShowQuery(char *buffer) ;
extern void queryCommandEdit(void) ;
extern void queryCommandUndo(void) ;

/* following defined in querybuild.c */
extern void qbuildCreate (void);
extern int textAlphaOrder(void *a, void *b) ;
extern void destroyMenu(FREEOPT *menup, int max);
extern void qbuildNewClass(int classe);

/* following defined in qbedisp.c */
extern void qbeCreate (void);

#endif /* DEF_QUERY__H */

