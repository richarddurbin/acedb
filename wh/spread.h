/*  File: spread.h
 *  Author: Jean Thierry-Mieg (mieg@mrc-lmb.cam.ac.uk)
 *  Copyright (C) J Thierry-Mieg and R Durbin, 1992
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
 * Description: public header for spreadsheet operations
 * Exported functions:
 * HISTORY:
 * Last edited: Nov 25 16:04 1999 (fw)
 * Created: Thu Aug  6 13:40:24 1992 (mieg)
 *-------------------------------------------------------------------
 */

/* $Id: spread.h,v 1.45 2000-03-14 14:33:05 srk Exp $ */
  
#ifndef DEF_SPREAD_H  
#define DEF_SPREAD_H

#include "acedb.h"

#include "table.h"

/************************************************************/

typedef struct SpreadStruct *SPREAD; /* opaque forward declaration */
typedef struct ColonneStruct COL; /* opaque forward declaration */

typedef enum { SHOW_ALL=0,
	       SHOW_MIN,
	       SHOW_MAX,
	       SHOW_AVG ,
	       SHOW_VAR,
	       SHOW_SUM } SpShowType ;


typedef enum { VAL_MUST_BE_NULL=0,
	       VAL_IS_OPTIONAL=1,
	       VAL_IS_REQUIRED=2 } SpPresenceType ;

typedef enum { COLEXTEND_FROM=0,
	       COLEXTEND_RIGHT_OF=1 } SpExtendType ;

/************************************************************/

/* tabledefsubs.c */

/* constructors - 
 * if params == NULL, the definitions will be loaded
 *        literally without parameter substitution 
 * if params == "" only the internal paramaters under Parameter-tag
 *        will be used in the parameter substitution. */
SPREAD spreadCreateFromAceIn (ACEIN spread_in, char *params);
SPREAD spreadCreateFromFile (char *filename, const char *spec, char *params);
SPREAD spreadCreateFromStack (Stack defStack, char *params);
SPREAD spreadCreateFromObj (KEY tableDefKey, char *params);

/* destructor */
void uSpreadDestroy (SPREAD spread) ;
#define spreadDestroy(spread) (uSpreadDestroy(spread),(spread)=0)


/* the master keyset is the list of objects in the first column */
KEYSET spreadGetMasterKeyset (SPREAD spread) ;

KEYSET spreadFilterKeySet (SPREAD spread, KEYSET ks);

char *spreadGetTitle (SPREAD spread);

Array spreadGetFlipInfo (SPREAD spread);
Array spreadGetWidths (SPREAD spread);

/* recalc results over the whole class in colonne 1
 * The function returns the pointer to spread->values or NULL,
 * if an error occured. */
TABLE *spreadCalculateOverWholeClass (SPREAD spread);

/* recalc results using the keyset, if lastp is a valid pointer
 * it will start at *lastp-position in the keyset, which is
 * where we left off last, the new value will be returned in *lastp
 * The function returns the pointer spread->values, (cannot be NULL) */
TABLE *spreadCalculateOverKeySet (SPREAD spread, KEYSET ks, int *lastp);

/**************/

BOOL spreadGetPrecomputation (SPREAD spread);
/* will try to load precomputed table - 
 * only works if definitions were loaded from object
 * where the "Precompute" tag was set and a 
 * matching "TableResult" obj exists */



/* tabledefio.c */
void spreadDumpDefinitions (SPREAD spread, ACEOUT dump_out);
BOOL spreadSaveDefInObj (SPREAD spread, KEY tableKey) ;


/**** function to interface with the internals of the definitions ****/

COL *spreadColInit(SPREAD spread, int colNum);

void spreadColSetPresence(SPREAD spread, int colNum, KEY presence);
void spreadColSetExtend(SPREAD spread, int colNum, KEY extend);
void spreadColSetHidden(SPREAD spread, int colNum, KEY hidden);

void spreadColCyclePresence(SPREAD spread, int colNum);
void spreadColCycleExtend(SPREAD spread, int colNum);
void spreadColCycleHidden(SPREAD spread, int colNum);

#endif /* DEF_SPREAD_H */
 
