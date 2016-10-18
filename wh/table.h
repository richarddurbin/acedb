/*  File: table.h
 *  Author: Richard Durbin (rd@sanger.ac.uk)
 *  Copyright (C) J Thierry-Mieg and R Durbin, 1996
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
 * 	Richard Durbin (MRC LMB, UK) rd@sanger.ac.uk, and
 *	Jean Thierry-Mieg (CRBM du CNRS, France) mieg@kaa.cnrs-mop.fr
 *
 * SCCS: $Id: table.h,v 1.44 2004-04-09 22:23:23 mieg Exp $
 * Description: header for new table class
 *              NOTE: to use the empty/null values macros you will need
 *              to use the BITSET_MAKE_BITFIELD macro from bitset.h
 * Exported functions:
 * HISTORY:
 *   changed text to use a stack, allows read/write
 * Last edited: Mar 15 11:02 2001 (edgrif)
 * * May 26 11:21 1999 (fw): introduced tableCopy to complete the ADT
 * * May 26 11:20 1999 (fw): removed tableEmpty macro
 *              it seemed bugged and is obsolete setEmpty and unsetEmpty
 *              do that work.
 * * Mar 18 10:18 1999 (edgrif): Added defs for invalid ints/floats from regular.h
 * * Feb 24 16:38 1999 (fw): imply tableUnSetEmpty with every tableKey/Int/..
 *			     operation, so setting a value makes it non-NULL
 * Created: Thu Oct 17 14:27:59 1996 (rd)
 * CVS info:   $Id: table.h,v 1.44 2004-04-09 22:23:23 mieg Exp $
 *-------------------------------------------------------------------
 */
#ifndef TABLE_H_DEF
#define TABLE_H_DEF 

#include <wh/regular.h>
#include <wh/mytime.h>
#include <wh/aceiotypes.h>
#include <wh/bitset.h>					    /* _See macros below_ */
#include <wh/keyset.h>
#include <wh/parse.h>
#include <wh/dict.h>

typedef union { KEY k ; int i ; float f ; mytime_t t ;} TABLETYPE ;

typedef struct {
  magic_t *magic ;
  STORE_HANDLE handle ;
  int	ncol ;	
  TABLETYPE *name ;		/* TABLETYPE so can store easily */
  char  *type ;	/* string of type characters per column, see below */
  BOOL *visibility;		/* flag per column */
  Array *col ;	/* private: access only via tab*() macros below */
  KEYSET *parents;
  KEYSET *grandParents;
  BitSet *emptyBits ;		/* flags for isEmpty mirroring col */
  Stack s ;     /* to store strings */
  void  *data ; /* for particular package usage */
} TABLE ;

/* The user can look at any values except col, but should not set any 
** values directly.  They should be set using the interface functions
** below.
*/

/* legal types and corresponding characters are:
     k KEY
     g tag
     b bool
     i Int
     f Float
     s Text (string)
     t DateType  // d means DISK, kernel reserved
     a flagset   // flag set name is column name, minus "Flag" prefix if present
*/

/* Any type has a potential null value.  For k,t,s,d this is 0.  The
   following typedefs give null values for i,f (same as in spread.h and
   freesubs.c).
*/
#define NON_BOOL   (2)		/* always test b & TRUE , b & NON_BOOL */
#define NON_FLAG   (~0)		/* all bits - same as in flag.h */
#define NON_INT    UT_NON_INT				    /* int/float from regular.h */
#define NON_FLOAT  UT_NON_FLOAT

typedef struct tableOutControl {
    char separator; 
    char style;
    BOOL showEmpty;
    Array oldClass;
    DICT **colFlagDict;
    char **colFlagSet;
    int javatypeWatermark;
    char cStylePrefix [1] ;
} TABLEOUTCONTROL;

/************************* functions ****************************/
/****************************************************************/

/******* core functions ********/

TABLE *tableHandleCreate (int size, char *type, STORE_HANDLE h) ;
#define tableCreate(x,y) tableHandleCreate(x,y,0)

void  uTableDestroy (TABLE *table) ;
#define tableDestroy(_t) (uTableDestroy(_t), _t=0)

char* tableSetColumnName (TABLE *table, int i, char *name) ; /* returns old value */
char* tableGetColumnName (TABLE *table, int i) ; 
		      			

BOOL tableSetColumnHidden (TABLE *t, int colNum); /* ret old val */
BOOL tableSetColumnVisible (TABLE *t, int colNum); /* ret old val */
BOOL tableColumnIsVisible (TABLE *t, int colNum); /* enquire */


int  tableMax (TABLE *t) ;		/* max in any column */
void tableClear (TABLE *t, int n) ;
char *tableTypes (TABLE *table) ; 	/* return the types, read-only */
BOOL tableSetTypes (TABLE *t, char *types) ; /* can only redefine '0' type */

BOOL tableSort (TABLE *t, char *spec); /* spec is "-2+4+1" for 
					  first priority column 2 reversed, 
					  then column 4, then column 1
					  returns FALSE if bad table or invalid spec */
void tableMakeUnique (TABLE *t) ; 	/* assumes sorted */

TABLE *tableCopy(TABLE *old, STORE_HANDLE handle);

/* convert between keysets and tables (for aql) */
TABLE* tableCreateFromKeySet (KEYSET ks, STORE_HANDLE h);
KEYSET keySetCreateFromTable (TABLE* t,  STORE_HANDLE h);


/* Next set of macros are central - they access the data in the columns.
   They are based on arr(), arrp(), array(), but we can specify the types 
   in the names, because they are limited.  I think this is nicer.
*/
#define tabMax(_t,_j)			arrayMax((_t)->col[_j])
		/* for simple read-access 
		 * (not intended for setting values, because emptyBits not handled) */
#define tabKey(_t,_i,_j)		arr((_t)->col[_j],_i,TABLETYPE).k
#define tabTag(_t,_i,_j)		tabKey(_t,_i,_j)
#define tabFlag(_t,_i,_j)		tabKey(_t,_i,_j)
#define tabBool(_t,_i,_j)		arr((_t)->col[_j],_i,TABLETYPE).i
#define tabInt(_t,_i,_j)		arr((_t)->col[_j],_i,TABLETYPE).i
#define tabFloat(_t,_i,_j)		arr((_t)->col[_j],_i,TABLETYPE).f
#define tabDate(_t,_i,_j)		arr((_t)->col[_j],_i,TABLETYPE).t
#define tabString(_t,_i,_j)		stackText((_t)->s,tabInt(_t,_i,_j))
		/* for pointers for serial access down a column */
#define tabKeyp(_t,_i,_j)		(KEY*)((_t)->col[_j]->base + _i)
#define tabTagp(_t,_i,_j)		tabKeyp(_t,_i,_j)
#define tabFlagp(_t,_i,_j)		tabKeyp(_t,_i,_j)
#define tabBoolp(_t,_i,_j)		(int*)((_t)->col[_j]->base + _i)
#define tabIntp(_t,_i,_j)		(int*)((_t)->col[_j]->base + _i)
#define tabpFloatp(_t,_i,_j)		(float*)((_t)->col[_j]->base + _i)
#define tabDatep(_t,_i,_j)		(mytime_t*)((_t)->col[_j]->base + _i)
/* #define tabpStringp(_t,i,j)  does not work with stacks */

		/* for setting values */
		/* the commands also perform unsetting of the emptyBits
		 * so the tableField is made non-empty (see tableUnSetEmpty) */
#define tableKey(_t,_i,_j)		bitUnSet((_t)->emptyBits[_j],_i),array((_t)->col[_j],_i,TABLETYPE).k
#define tableInt(_t,_i,_j)		bitUnSet((_t)->emptyBits[_j],_i),array((_t)->col[_j],_i,TABLETYPE).i
#define tableBool(_t,_i,_j)		bitUnSet((_t)->emptyBits[_j],_i),array((_t)->col[_j],_i,TABLETYPE).i
#define tableFloat(_t,_i,_j)		bitUnSet((_t)->emptyBits[_j],_i),array((_t)->col[_j],_i,TABLETYPE).f
#define tableDate(_t,_i,_j)		bitUnSet((_t)->emptyBits[_j],_i),array((_t)->col[_j],_i,TABLETYPE).t
		/* tableSetString() different because of memory handling */
void tableSetString (TABLE *table, int i, int j, char *string) ;

/* functions to keep track of the origin of each data-element
 * so we know which object to display for each cell in the table
 * these values only make sense if the cell-value itself in non-NULL */
#define tabParent(_t,_i,_j)		arr((_t)->parents[_j],_i,KEY)
#define tabGrandParent(_t,_i,_j)	arr((_t)->grandParents[_j],_i,KEY)

#define tableParent(_t,_i,_j)		array((_t)->parents[_j],_i,KEY)
#define tableGrandParent(_t,_i,_j)	array((_t)->grandParents[_j],_i,KEY)

/* Routines to handle empty/null values.
   Changed in version 1 to allow set/unset/read empty independently from
   the values, using bitset.  We think this will be much nicer.
   NOTE: to use these macros you must code the BITSET_MAKE_BITFIELD macro
   from bitset.h to locally define the array used by the bitset operations.
*/
#define tableSetEmpty(_t,_i,_j)		bitSet((_t)->emptyBits[_j],_i)
#define tableUnSetEmpty(_t,_i,_j)	bitUnSet((_t)->emptyBits[_j],_i) /* shouldn't be needed as implied by the tableKey/Int/.. commands */
#define tabEmpty(_t,_i,_j)		bit((_t)->emptyBits[_j],_i)

	/* old routines based on values (version 0) */
BOOL tabValueNull (TABLE *t, int i, int j) ;   /* true if NON_type */
	/* conversion from old to new -- public because may be useful elsewhere */
void tableCheckEmpty (TABLE *t, int j) ; /* sets empty for col j from null values */

/******* import/export *******/

int tableOut (ACEOUT dump_out, TABLE *table, char sep, char style) ; /* uses freeout */
int tablePartOut (ACEOUT dump_out, int *lastp, TABLE *table, char sep, char style) ; 
void tableOutSetup(TABLE *t, struct tableOutControl *taboutcontrol);
void tableRowOut (ACEOUT dump_out, int row, TABLE *t, struct tableOutControl *taboutcontrol);
int tableSliceOut (ACEOUT dump_out, int begin, int count, 
		   TABLE *t, char separator, char style) ;
BOOL tableHeaderOut (ACEOUT dump_out, TABLE *table, char style) ;
void tableFooterOut (ACEOUT dump_out, TABLE *table, char style) ;

TABLE *tableCreateFromAceIn (ACEIN fi, char *table_name, char **errtext);

ParseFuncReturn tableResultParse (ACEIN parse_in, KEY key, char **errtext);
                                                       /* ParseFuncType - reads and saves */
BOOL tableResultDump (ACEOUT dump_out, KEY key); /* DumpFuncType */

/******* store/get *******/

TABLE	*tableHandleGet (KEY key, STORE_HANDLE h) ;
#define tableGet(key)	tableHandleGet(key,0)
BOOL	tableStore (KEY key, TABLE *table) ;
#define tableKill(key)	arrayKill(key)    /* kills the lex entry */      


/*********************************************************************/
/**************** table operator utilities (fw-980721) ***************/
/*********************************************************************/


/* Function to check whether two tables are compatiable, that
   they can be used for the subsequent table operators together. */
/*
   NOTE: Once this check is performed on both tables the short version
   tabXXX() of the operations can be used. In case it is only a one-off
   statement that uses a table operation, the tableXXXX() routines 
   should be used to include the type checking from this function
*/
BOOL tableIsColumnCompatible (TABLE *leftTable, 
			      TABLE *rightTable);

/**********************************************************************************/

/* function to compare a complete row in a table with a row in a second table. */
/*
   Returns FALSE if tables are incompatible, type mismatch or uninitialised
   ONLY returns TRUE if full compatability and if all values in the two rows match
*/
BOOL tableIsRowEqual (int    rowCountLeft, 
		      TABLE *leftTable, 
		      int    rowCountRight, 
		      TABLE *rightTable);

/**********************************************************************************/

/* simple function to compare a complete row in a table with a row in a second table. */
/*
   NOTE: no type checking for column compatability is performed
   this function is to be used for quick access in long loops,
   for sporadic occasional row-comparison that includes all type checking
   please use tableIsRowEqual().
*/
BOOL tabIsRowEqual (int    rowCountLeft, 
		    TABLE *leftTable, 
		    int    rowCountRight, 
		    TABLE *rightTable);

/**********************************************************************************/

/* function to copy all values in a row of the srcTable to the end of the destTable */
/*
   The destTable is created if it is not existent.
   The function returns FALSE, if an existing destTable is not type-compatible
   with the source table.
   Otherwise, the vaues in the specified row are appended to the end
   of the destTable and TRUE is returned.
*/
BOOL tableCopyRow (int    rowCountSrc, 
		   TABLE *srcTable, 
		   TABLE *destTable);

/**********************************************************************/

/* simple function to copy all values in a row of the srcTable to the end of the destTable */
/*
   NOTE: 
   (1) No type checking for column compatability is performed.
   (2) The destTable is assumed to be initialised with the same types as the srcTable.

   This function is to be used for quick access in long loops,
   for sporadic occasional row-copy that includes all type checking
   and initialisation please use tableCopyRow().
*/
void tabCopyRow (int    rowCountSrc, 
		 TABLE *srcTable, 
		 TABLE *destTable);

/* produce a combined table that contains rows from both
   tables but no duplicates */
/* fails with return code FALSE, if the two tables are incombinable
   or in case an existing table pointer was passed, this has
   already been assigned column types, otherwise a new table
   will be returned in *destTable 
*/
BOOL tableUnion (TABLE *leftTable,
		 TABLE *rightTable,
		 TABLE **destTable);


/* produce a table with rows that are in the first but 
   not the second table */
/* fails with return code FALSE, if the two tables are incombinable
   or in case an existing table pointer was passed, this has
   already been assigned column types, otherwise a new table
   will be returned in *destTable 
*/
BOOL tableDiff (TABLE *leftTable,
		TABLE *rightTable,
		TABLE **destTable);


/* produce a table with rows that are in the first but 
   not the second table */
/* fails with return code FALSE, if the two tables are incombinable
   or in case an existing table pointer was passed, this has
   already been assigned column types, otherwise a new table
   will be returned in *destTable 
*/
BOOL tableIntersect (TABLE *leftTable,
		     TABLE *rightTable,
		     TABLE **destTable);


/**********************************************************************************/

/*-----------------------------------------------------------------*/

#ifdef EXAMPLE

void main (int argc, char **argv)
{
  ACEOUT fo = aceOutCreateToStdout (0);
  TABLE *t = tableCreate (20, "ifs") ;
  int i ;

  for (i = 0 ; i < 10 ; ++i)
    { tableInt(t,0,i) = i ;
      tableFloat(t,1,i) = 1.0 / (float)i ;
      tableSetString (t,2,i,messprintf("a%d",i)) ;
    }

  tableOut (fo, t, '\t', 'a') ;

  { int sum = 0 ;
    for (i = 0 ; i < tabMax(t,0) ; ++i)
      sum += tabInt(t,0,i) ;
    printf ("Sum of column 0 is %d\n", sum) ;
  }

  { float sum = 0 ;
    float *fp = tabFloatp(t,1) ;
    for (i = 0 ; i < tabMax(t,1) ; ++i)
      sum += *fp++ ;
    printf ("Sum of column 1 is %f\n", sum) ;
  }

  tableDestroy (t) ;
  aceOutDestroy (fo);
}

#endif /* EXAMPLE */

#endif /* TABLE_H_DEF  */

/***************** end of file ****************/
 
 
