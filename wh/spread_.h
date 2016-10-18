/*  File: spread_.h
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
 * Description: private header for spreadsheet operations
 *              it completes the opaque SPREAD type
 *              and declares all sprdop + sprddef function used within
 *              the spreadPackage (basic ops + display ops)
 * Exported functions:
 * HISTORY:
 * Last edited: Nov 25 14:09 1999 (fw)
 * Created: Tue Dec 22 16:25:35 1998 (fw)
 *-------------------------------------------------------------------
 */


#ifndef ACEDB_SPREAD__H
#define ACEDB_SPREAD__H

#include "spread.h"		/* include public header */

#include "query.h"
#include "table.h"

/*************************************************************/
/* spread - structure containing the spreadsheet definitions */
/*************************************************************/

extern magic_t SPREAD_MAvGIC;	/* to verify a SPREAD pointer */

struct SpreadStruct {
  magic_t *magic;        /* == &SPREAD_MAGIC*/

  BOOL isModified;		/* have the definitions changed ? */

  char titleBuffer[60] ;
  char paramBuffer[180] ;	/* default parameters stored 
				 * with these definitions */
  char *extraParameters;	/* command line parameters
				 * additional to inlined params
				 * (unused in tablemaker) */
  int sortColonne;

  Array colonnes ; /* array of definitions for each column */

  BOOL showTitle ; /* dump title when printing from command.c */

  /* the definitions can be stored in an acedb-object in
   * class _VTable, if the defs came from such an object, 
   * the tableDefKey is valid (non-zero) */
  KEY tableDefKey ;

  /* the definitions can be stored in a text-file on disk,
   * which is useful for users without write-access, who can't add
   * database-objects. The filename is non-NULL if the definitions
   * were loaded from disk */
  char *filename;

  /* once the results is calculated, it is stored in the standard
  * TABLE datatype - the table will be sorted, and have no duplicate
  * rows, and it only contains the results of visible colonnes */
  TABLE *values;
  BOOL valuesModified;		/* has the results table changed ? */

  /* precompute system : the TABLE result can be stored in the database,
   * for quicker retrieval in the future, which avoids recomputation.
   * This is only possible if the definitions are stored in a 
   * _VTable object. */
  BOOL precompute;		/* set if the tag "Precompute" is set 
				 * in object */
  BOOL precomputeAvailable;	/* TRUE if the corresponding
				 * TableResult object 
				 * containing the ->values TABLE exists */

  KEY tableResultKey;		/* Array object in class _VTableResult
				 * that contains results TABLE */

  Stack comments ;
  BOOL interrupt ;

  BOOL isActiveKeySet ;		/* for server / command.c ??? */

} ;

/************************************************************/
/* colonne - structure for a single column definition       */
/************************************************************/

struct ColonneStruct
{
  int colonne ;			/* number */

  SpShowType showType ;
  SpPresenceType presence;
  SpExtendType extend;
  BOOL hidden, flipped;

  int 
    realType , /* one of k:KEY, b:BOOL, i:Int, f:Float, t:Text */
    type ;  /* idem or  count */
  KEY
    classe , /* class if type == 'k' */
    tag ;   /* tag in object I build from */
  BOOL nonLocal ; /* true if this column !has a correct objMark, i.e. average */
  Stack tagStack ;

  int width ;

  int from ;			/* 0 == master column of the table */

  Stack text ;			/* to stack the Text data */
  char headerBuffer[60] ;
  char conditionBuffer[360] ; /* additional restriction on the new object */
  char tagTextBuffer[360] ; /* To edit by hand the tag filed */

  BOOL condHasParam ;        /* so must reevaluate each time */
  int mark ;
  COND tagCondition, conditionCondition ;

  /* for the graphTextPtrPtr */
  char *hiddenp , *optionalp;
  char *showtypep, *classp , *extendp ;

  /* also used for input output of definitions */
  char *tagp, *typep;
} ;

/************************************************************/
/**** functions private within the spreadsheet package ******/
/************************************************************/

/* tabledefsubs.c */

/* to be used to clear spread->colonnes */
#define spreadDestroyColonnes(_c) (uSpreadDestroyColonnes(_c),_c=0)
void uSpreadDestroyColonnes (Array colonnes);

int spreadAddColonne (SPREAD spread); /* returns number of added colonne */
int spreadRemoveColonne (SPREAD spread, int colNum); /* returns new valid column number, or if 0 then it couldn't destroy, because colNum was 0 */

BOOL spreadCheckConditions(SPREAD spread);

/* tabledefio.c */
BOOL spreadReadDef (SPREAD spread, ACEIN def_io, char *inParams, 
		    BOOL isReadFromObj);
Stack getDefinitionFromObj (KEY tableKey);
void spreadDumpColonneDefinitions (ACEOUT dump_out, Array colonnes) ;

#endif /* !ACEDB_SPREAD__H */
