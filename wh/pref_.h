/*  File: pref_.h
 *  Author: Fred Wobus (fw@sanger.ac.uk)
 *  Copyright (C) J Thierry-Mieg and R Durbin, 1999
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
 * Description: private header for preference module in acedb
 *              included by prefsubs.c and prefdisp.c
 * Exported functions:
 * HISTORY:
 * Last edited: Nov 18 14:55 2002 (edgrif)
 * Created: Sat May  1 10:42:50 1999 (fw)
 *-------------------------------------------------------------------
 */

#ifndef ACEDB_PREF__H
#define ACEDB_PREF__H

#include <wh/acedb.h>
#include <wh/pref.h>		/* include our public header */
#include <wh/colours.h>		/* provide enum Colours, so we can treat
				 * colour values as Integers */


/* N.B. funcs are only set in the code, not from the user defaults.          */
/* Conceivably we could use gmodule interface to get entry points of         */
/* functions using their string name...overkill for now.                     */
typedef struct
 {
   char name[32];
   PrefFieldType type;					    /* BOOL, int etc. */
   BOOL display;
   union value_tag
   {
     BOOL bval;
     float fval;
     int ival;						    /* used for int and colours */
     char *sval ;
   } value;

   int string_length ;					    /* Needed to set editor window length, */
							    /* graph expects this to be >= 16 */

   union deffunc_tag
   {
     PrefDefFuncBOOL   bfunc ;
     PrefDefFuncFloat  ffunc ;
     PrefDefFuncInt    ifunc ;				    /* used for int and colours */
     PrefDefFuncString sfunc ;
   } deffunc ;
   union editfunc_tag
   {
     PrefEditFuncFloat  ffunc ;				    /* N.B. no BOOL or colours possible. */
     PrefEditFuncInt    ifunc ;
     PrefEditFuncString sfunc ;
   } editfunc ;
   union func_tag
   {
    PrefFuncBOOL   bfunc ;
     PrefFuncFloat  ffunc ;
     PrefFuncInt    ifunc ;				    /* used for int and colours */
     PrefFuncString sfunc ;
   } func ;
} PREFLIST ;



/* Functions used by prefsubs.c and prefdisp.c */

Array prefGet (void);					    /* Array of PREFLIST */
void prefSave (void);
void prefRead (ACEIN pref_in, Array existing_pref_list) ;

#endif /* !ACEDB_PREF__H */



