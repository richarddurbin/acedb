/*  File: call.h
 *  Author: Richard Durbin (rd@sanger.ac.uk)
 *  Copyright (C) J Thierry-Mieg and R Durbin, 1994
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
 * Description: Header file for message system to allow calls by name
 * Exported functions:
 * HISTORY:
 * Last edited: Nov  1 16:58 1999 (fw)
 * * Nov  3 16:15 1994 (mieg): callCdScript, first cd to establish 
     the pwd  of the command, needed for ghostview etc.
 * Created: Mon Oct  3 14:57:16 1994 (rd)
 *-------------------------------------------------------------------
 */

/* $Id: call.h,v 1.14 1999-11-01 17:00:11 fw Exp $ */


#ifndef DEF_CALL_H
#define DEF_CALL_H
 
#include "regular.h"
#include "aceiotypes.h"

typedef int MESSAGERETURN ;
typedef void (*CallFunc)() ;


void callRegister (char *name, CallFunc func) ;
BOOL call (char *name, ...) ;
BOOL callExists (char *name) ;

int callScript (char *script, char *args) ;
int callCdScript (char *dir, char *script, char *args) ; 
FILE* callScriptPipe (char *script, char *args) ;
FILE* callCdScriptPipe (char *dir, char *script, char *args) ;

/* action.c */
typedef void (*ExternalFuncType)(ACEIN fi, void *lk);
BOOL externalAsynchroneCommand (char *command, char *parms,
                                void *look, ExternalFuncType g);
void externalFileDisplay (char *title, FILE *f, Stack s) ;
void externalPipeDisplay (char *title, FILE *f, Stack s) ;
void acedbMailComments(void) ;
void externalCommand (char* command) ;

#endif /* !DEF_CALL_H */

