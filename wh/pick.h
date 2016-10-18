/*  File: pick.h
 *  Author: Ed Griffiths (edgrif@sanger.ac.uk)
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
 *	Jean Thierry-Mieg (CRBM du CNRS, France) mieg@kaa.crbm.cnrs-mop.fr
 *
 * Description:
 * Exported functions:
 * HISTORY:
 * Last edited: Sep 27 10:42 2010 (edgrif)
 * * Oct 22 14:11 1998 (edgrif): Added this header, added dec for pickRegisterConstraints
 * Created: Thu Oct 22 14:11:20 1998 (edgrif)
 *-------------------------------------------------------------------
 */

/* $Id: pick.h,v 1.26 2010-10-15 12:23:37 edgrif Exp $ */

#ifndef DEFINE_PICK_H
#define DEFINE_PICK_H

#include "acedb.h"

typedef  struct { 
                 int 	name, alias ;
		 char	visible ;
		 char	type ;
                 KEY	displayKey ;
		 BOOL	isCaseSensitive ;
		 BOOL	protected ;  /* do not parse or addkey */
		 unsigned char mask ;
		 KEY    model ;
		 Array	conditions ;
		 void*	constraints ;
		 Stack	filters ;
                 BOOL	Xref ;
                 BOOL	known ;
		 BOOL	sybase ; /* experimental sybase storage */
		 KEY    tag, symbol ;
		 float	width, height ;
                 BOOL   private ; /* private to kernel, do not dump, invisibleKey */
                 KEY    classe ;  /* corresponding entry in class Class */
	       } PICKLIST ;

extern PICKLIST pickList[] ;
extern FREEOPT pickVocList[] ;	/* in picksubs.c */

void pickPreInit(void);
void pickInit(void);

char *pickGetAlternativeName(KEY obj_key, char *alternative_tag_name) ;

                           /* returns # of class Word */
                           /* used by parse and readmodels */
int pickWord2Class(char *word);
char *pickClass2Word(int classe) ;
                            /* returns name of class */
                   /* used by lexaddkey to initialise the vocs */

int pickMakeClass (char* cp);
void pickCheckConsistency(void);


#define pickXref(t)               (t<255 && pickList[(t) & 255].Xref)
#define pickVisible(t)               (t<255 && freelower(pickList[(t) & 255].visible) == 'v')

#define pickCheck(t)               (pickList[(t) & 255].check)
#define pickKnown(t)               (pickList[(t) & 255].known)

#define pickDisplayKey(key)      pickList[(class(key)) & 255 ].displayKey
#define pickType(key)             pickList[(class(key)) & 255 ].type
#define pickModel(key)             pickList[(class(key)) & 255 ].model
#define pickCaseSensitive(key)    pickList[(class(key)) & 255 ].isCaseSensitive
#define pickExpandTag(key)        (KEYKEY(key) ?  \
				   pickList[(class(key)) & 255 ].tag : 0)

/* kernel function to init system classes */
void pickSetClassOption (char *nom, char type, char v, 
			 BOOL protected, BOOL private);

/* kernel function to init system classes (for graphical apps) */
void pickSetClassDisplayType (char *nom, char *displayName);

/* standard init called with FALSE, after displayInit we can set
 * the displaytypes in the pickList and call it with TRUE to
 * parse options.wrm again and set the displayKey for each class */
void pickGetClassProperties (BOOL doReadDisplayType);

void pickRegisterConstraints (void) ;

int  pickMatch (char *c, char *template) ;
int pickMatchCaseSensitive (char *cp,char *tp, BOOL caseSensitive) ;

BOOL pickIsA (KEY *classp, unsigned char *maskp) ;

int superClass (KEY classe) ;  /* class < 256 and class in ?Class both recognised */

int classModel2Class(KEY class_model_key) ;		    /* Given Class model key returns int class */


Array pickIsComposite(KEY classe);
BOOL pickBelongsToComposite(KEY classe, KEY key);

#endif

