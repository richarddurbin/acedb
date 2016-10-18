/*  File: table.c
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
 * SCCS: $Id: table.c,v 1.70 2004-04-09 22:24:24 mieg Exp $
 * Description:
 * Exported functions:
 * HISTORY:
 * Last edited: Jul  9 12:03 2003 (rnc)
 * * Jun 15 18:17 2000 (rdurbin): -x, -X, -y, -Y options for tableOut() produce XML (-x for simple HTML)
 * * Jun 15 18:16 2000 (rdurbin): -k, -K options as -j/-J variants
 * * Oct 20 16:06 1999 (fw): dumping of tables to ACEOUT inst. of freeOut
 * * Aug 26 18:19 1998 (fw): proper tableSort over multiple columns
 * * Aug 25 14:25 1998 (fw): tableOut now writes NULL for '0' column-values
 * * Aug 14 12:07 1998 (fw): fixed silly bug in tableCreateFromKeySet()
 * * Jul 15 16:48 1998 (fw): fixed silly bug in tableMax()
 * Created: Thu Oct 17 14:29:36 1996 (rd)
 *-------------------------------------------------------------------
 */

#include <ctype.h>
#include <wh/acedb.h>
#include <wh/aceio.h>
#include <wh/dict.h>
#include <wh/bitset.h>
#include <wh/java.h>
#include <wh/bs.h>
#include <wh/a.h>
#include <whooks/sysclass.h>
#include <wh/lex.h>
#include <wh/pick.h>
#include <wh/flag.h>
#include <wh/table.h>
#include <wh/utils.h>           /* for lexstrcmp */

BITSET_MAKE_BITFIELD		/* define bitField for bitset.h ops */

static magic_t TABLE_MAGIC = "TABLE";

static BOOL tableExists (TABLE *tt);
static void tableFinalise (void *block);
static TABLE *tableDoDoParse (ACEIN parse_io, char *table_name, char *format_type,
			      Array oldClass, char **errtext);
static char* tableParseFormat (ACEIN parse_io, Array oldClass);
static BOOL setColumnVisibility (TABLE *t, int colNum,
				 int on_off);

/******************************************************************
 ************************* public functions ***********************
 ******************************************************************/

TABLE *tableHandleCreate (int size, char *type, STORE_HANDLE parent)
     /* type-string is 0-terminated string with format-char for each
      * column. possibilities are "kgbifst"
      * upper-case type-character indicates that column is hidden */
{ 
  TABLE *t ;
  int i, ncol ;
				/* first check type is OK */
  if (!type || !*type)
    messcrash ("tableCreate: received null or empty type string") ;

  for (i = 0 ; i < strlen(type); ++i)
    switch (ACEIN_LOWER[(int)type[i]])
      { 
      case 'k': case 'g': case 'b': case 'i': case 'f': 
      case 's': case 't': case 'a': 
      case '0':
	break ;
      default:
	messcrash ("tableCreate: bad type %c for column %d", type[i], i+1) ;
      }

  if (sizeof(float) != sizeof (KEY) ||
      sizeof(int) != sizeof (KEY) ||
      sizeof(TABLETYPE) != sizeof (KEY) ||
      sizeof(mytime_t) != sizeof (KEY))
    messcrash 
      ("%s\n%s", "On this platform different TABLETYPEs have different byte size",
       " Rewrite the table.h pointer macros tabXxxp ! ") ;

  /* make the table and deal with handles */
  t = (TABLE *)halloc(sizeof(TABLE), parent) ;
  blockSetFinalise (t, tableFinalise) ;

				/* initialise TABLE */
  t->magic = &TABLE_MAGIC ;

  /* do not allocate the internal handle on the given handle,
   * the internal handle is killed by the finalisation function,
   * and this way allows free-floating allocation with a null-handle */
  t->handle = handleCreate () ;

  t->ncol = ncol = strlen(type) ;
  t->type = strnew(type, t->handle) ;
  t->name = (TABLETYPE*) halloc(ncol*sizeof(int), t->handle) ;
  t->visibility = (BOOL*) halloc(ncol*sizeof(int), t->handle) ;
  t->col = (Array*) halloc(ncol*sizeof(Array), t->handle) ;
  t->parents = (KEYSET*) halloc(ncol*sizeof(KEYSET), t->handle);
  t->grandParents = (KEYSET*) halloc(ncol*sizeof(KEYSET), t->handle);
  t->emptyBits = (BitSet*) halloc(ncol*sizeof(BitSet), t->handle) ;
  t->s = stackHandleCreate (4096, t->handle) ;
  t->s->textOnly = TRUE;

  pushText (t->s, type) ; /* ! occupy mark zero in  a most useful way */

  if (size < 64)
    size = 64 ;

  /* initialise each column */
  for (i = 0 ; i < ncol ; ++i)
    {
      /* make each character in the type-string lower-case
       * but preserve the visibility info given by upper-case
       * chars in an extra array */
      if (isupper((int)t->type[i]))
	{
	  t->type[i] = ACEIN_LOWER[(int)t->type[i]];
	  t->visibility[i] = FALSE;
	}
      else
	t->visibility[i] = TRUE;

      t->name[i].i = stackMark (t->s) ;
      pushText (t->s, "") ;

      t->col[i] = arrayHandleCreate (size, TABLETYPE, t->handle) ;
      t->parents[i] = arrayHandleCreate(size, KEY, t->handle);
      t->grandParents[i] = arrayHandleCreate(size, KEY, t->handle);
      t->emptyBits[i] = bitSetCreate (size, t->handle) ;
    }

  if (!tableExists(t))
    messcrash("tableHandleCreate() - just created invalid TABLE\n"
	      "  something seriously screwy - look at tableExists()");

  return t ;
} /* tableHandleCreate */

/************************************************************/

void uTableDestroy (TABLE *t)	/* call via tableDestroy macro */
{ 
  messfree (t) ;		/* trigger tableFinalise() */

  return;
} /* uTableDestroy */

/************************************************************/

TABLE *tableCreateFromKeySet (KEYSET keySet, STORE_HANDLE handle)
     /* make a table of KEYs from a given KEYSET (fw-980805) */
{
  TABLE *table;
  int row;

  if (!keySet)
    return 0;

  table = tableHandleCreate (keySetMax(keySet), "k", handle);

  if (keySetMax(keySet))
    {
      tableKey(table, keySetMax(keySet)-1, 0) = 0; /* set last item to extend size */
      for (row = 0; row < keySetMax(keySet); row++)
	tabKey(table, row, 0) = arr(keySet, row, KEY);
    }

  return table;
} /* tableCreateFromKeySet */


KEYSET  keySetCreateFromTable (TABLE *table, STORE_HANDLE handle)
/* makes a keyset from the KEY-type table column (fw-980805) */
{
  int c, i, keyCol=-1;
  KEYSET newKeySet;

  for (c = 0; c < table->ncol; ++c)
    if (table->type[c] == 'k') keyCol = c;

  if (keyCol == -1)
    return 0;		/* table contains no column of type KEY */

  newKeySet = keySetHandleCreate(handle);

  if (tableMax(table))
    {      
      array(newKeySet, tabMax(table,keyCol)-1, KEY) = 0; /* set last item to extend size */
      for (i = 0 ; i < tabMax(table,keyCol) ; ++i)
	arr(newKeySet,i,KEY) = tabKey(table,i,keyCol) ;
    }

  keySetSort(newKeySet) ;
  keySetCompress(newKeySet) ;

  return newKeySet;
} /* keySetCreateFromTable */

/************************************************************/

TABLE *tableCopy (TABLE *old, STORE_HANDLE handle)
{
  TABLE *new;
  int colNum;

  if (!tableExists (old))
    messcrash("tableCopy() - received invalid TABLE* pointer");

  new = tableHandleCreate(tableMax(old), old->type, handle);

  /* kill the empty arrays that have been initialised and replace
   * them with copies of the data in the old table */

  stackDestroy(new->s);
  new->s = stackCopy(old->s, new->handle);

  for (colNum = 0; colNum < old->ncol; colNum++)
    {
      arrayDestroy(new->col[colNum]);
      new->col[colNum] = arrayHandleCopy(old->col[colNum], new->handle);

      arrayDestroy(new->parents[colNum]);
      new->parents[colNum] = arrayHandleCopy(old->parents[colNum], new->handle);

      arrayDestroy(new->col[colNum]);
      new->col[colNum] = arrayHandleCopy(old->col[colNum], new->handle);

      bitSetDestroy(new->emptyBits[colNum]);
      new->emptyBits[colNum] = bitSetCopy(old->emptyBits[colNum], new->handle);

      new->name[colNum].i = old->name[colNum].i;
      new->visibility[colNum] = old->visibility[colNum];
    }

  return new;
} /* tableCopy */

/************************************************************/

int tableMax (TABLE *t)
/* return the largest number of rows over all columns */
{ 
  int colNum, j = 0, max = 0 ;

  if (!tableExists (t))
    messcrash ("tableMax() - received invalid TABLE* pointer");

  for (colNum = 0; colNum < t->ncol; colNum++)
    { 
      j = arrayMax(t->col[colNum]) ; 
      if (j > max) 
	max = j ; 
    }

  return max ;
} /* tableMax */

/************************************************************/

char *tableTypes (TABLE *t)
{
  if (!tableExists (t))
    messcrash("tableTypes() - received invalid TABLE* pointer");

  return t->type;
} /* tableTypes */

/************************************************************/

char *tableGetColumnName (TABLE *t, int i)
{
  if (!tableExists (t))
    messcrash("tableGetColumnName() - received invalid TABLE* pointer");

  if (i >= 0 && i < t->ncol)
    return stackText (t->s, t->name[i].i) ;

  return (char*)NULL;
} /* tableGetColumnName */

/************************************************************/

char *tableSetColumnName (TABLE *t, int i, char *name)
{ 
  char *old ;

  if (!tableExists (t))
    messcrash("tableSetColumnName() - received invalid TABLE* pointer");
  if (i < 0 || i >= t->ncol)
    messcrash ("tableSetColumnName() - col %d is out of bounds 0-%d",
	       i, t->ncol-1) ;

  if (name == NULL)
    messcrash ("tableSetColumnName() - name is NULL") ;

  old = stackText (t->s, t->name[i].i);

  t->name[i].i = stackMark(t->s) ;
  pushText (t->s, name) ;

  return old ;		
} /* tableSetColumnName */

/************************************************************/

BOOL tableSetColumnHidden (TABLE *t, int colNum)
{
  return setColumnVisibility (t, colNum, FALSE);
}

BOOL tableSetColumnVisible (TABLE *t, int colNum)
{
  return setColumnVisibility (t, colNum, TRUE);
}

BOOL tableColumnIsVisible (TABLE *t, int colNum)
{
  return setColumnVisibility (t, colNum, -1); /* don't set, just enquire */
} /* tableColumnIsVisible */


/************************************************************/

void tableSetString (TABLE *t, int i, int j, char *string)
{ 
  if (!tableExists (t))
    messcrash ("tableSetString() - received invalid TABLE* pointer");

  if (string && strlen(string) > 0)
    {
      tableInt(t,i,j) = stackMark(t->s) ;
      pushText (t->s, string) ;
    }
  else 
    tableInt(t,i,j) = 0 ;

  tableUnSetEmpty (t, i, j);	/* mark table field as non-NULL */

  return;
} /* tableSetString */

/************************************************************/

BOOL tabValueNull (TABLE *t, int i, int j)
{ 
  if (!tableExists (t))
    messcrash ("tableValueNull() - received invalid TABLE* pointer");

  if (i > tabMax(t,j))   
    return TRUE ;
  switch (t->type[j])
    {
    case 'k': case 'g': case 's': case 't':
      if (!tabKey(t,i,j)) 
	return TRUE ;
    case 'b': 
       if (tabBool(t,i,j) == NON_BOOL)
	 return TRUE ;
      break ;
    case 'a': 
       if (tabFlag(t,i,j) == NON_FLAG)
	 return TRUE ;
      break ;
    case 'i': 
       if (tabInt(t,i,j) == NON_INT)
	 return TRUE ;
      break ;
    case 'f': 
       if (tabFloat(t,i,j) == NON_FLOAT)
	 return TRUE ;
      break ;
    }

  return FALSE ;
} /* tabValueNull */

/************************************************************/

void tableCheckEmpty (TABLE *t, int column)
     /* set all values in column to NULL */
{
  int i ;

  if (!tableExists (t))
    messcrash ("tableCheckEmpty() - received invalid TABLE* pointer");

  for (i = 0 ; i < tabMax(t,column) ; ++i)
    if (tabValueNull (t, i, column))
      tableSetEmpty(t,i,column) ;
    else
      tableUnSetEmpty(t,i,column) ;

  return;
} /* tableCheckEmpty */

/************************************************************/

void tableClear (TABLE *t, int n)
     /* clear all values of the given table and reinitialise
      * to max-row-number of 'n' */
{
  int i ;

  if (!tableExists (t))
    messcrash ("tableClear() - received invalid TABLE* pointer");

  for (i = 0 ; i < t->ncol ; ++i)
    { 
      t->col[i] = arrayReCreate (t->col[i], n, TABLETYPE) ;
      t->parents[i] = arrayReCreate (t->parents[i], n, KEY) ;
      t->grandParents[i] = arrayReCreate (t->grandParents[i], n, KEY) ;
      t->emptyBits[i] = bitSetReCreate (t->emptyBits[i], n) ;
    }

  return;
} /* tableClear */

/************************************************************/

BOOL tableSetTypes (TABLE *t, char *types)
/* can only re-set '0' typed columns
   returns FALSE if one or more columns already had a type */
{
  int i ;

  if (!tableExists (t))
    messcrash ("tableSetTypes() - received invalid TABLE* pointer");

  for (i = 0 ; i < t->ncol ; ++i) /* check first */
    if (types[i] != '0' && types[i] != t->type[i] && t->type[i] != '0')
      return FALSE ;

  for (i = 0 ; i < t->ncol ; ++i)
    if (types[i] != '0' && types[i] != t->type[i])
      t->type[i] = types[i] ;

  strcpy (stackText(t->s, 0), t->type) ;

  return TRUE ;
} /* tableSetTypes */

/*********************************************************/

static TABLE *sortTable = 0 ;
static Array sortSpecList = 0;

static int tableCompare (const void* va, const void* vb)
/* comparison function used for qsort() in tableSort,
   that also rearranges an index array of row numbers,
   requires static sortSpec and sortTable to be assigned */
{
  register int a = *(int*)va ;
  register int b = *(int*)vb ;
  register int i, sortCol, sortSpec;


  for (i = 0; i < arrayMax(sortSpecList); ++i)
    {
      sortSpec = arr(sortSpecList,i,int);

      /* get the true sort column in C-numbers */
      if (sortSpec < 0)	/* descending */
	sortCol = -sortSpec - 1;
      else /* sortSpec > 0   -> ascending  */
	sortCol = sortSpec - 1;

      /* we make NULL the smallest value ever */
      if (tabEmpty(sortTable, a, sortCol))
	{
	  if (tabEmpty(sortTable, b, sortCol))
	    continue ;
	  return sortSpec>0 ? -1 : 1 ;
	}
      
      if (tabEmpty(sortTable, b, sortCol))
	return sortSpec>0 ? 1 : -1 ;

      if (tabKey(sortTable, a, sortCol) == tabKey(sortTable, b, sortCol))
	continue ;

      switch (sortTable->type[sortCol])
	{
	case 'k': case 'g': 
	  {
	    KEY
	      xk = tabKey(sortTable, a, sortCol),
	      yk = tabKey(sortTable, b, sortCol) ;
	    int r = keySetAlphaOrder(&xk, &yk);
	    if (r != 0)
	      return sortSpec>0 ? r : -r ;
	  }
	  break;

	case 'b':
	  {
	    BOOL 
	      ba = tabBool(sortTable, a, sortCol),
	      bb = tabBool(sortTable, b, sortCol) ;
	    int r = ba ? (bb ? -1 : 0 ) : ( bb ? 0 : 1 );
	    if (r != 0)
	      return sortSpec>0 ? r : -r ;
	  }
	  break;

	case 'i':
	  {
	    int 
	      ba = tabInt(sortTable, a, sortCol),
	      bb = tabInt(sortTable, b, sortCol) ;
	    int r = ba - bb;
	    if (r != 0)
	      return sortSpec>0 ? r : -r ;
	  }
	  break;

	case 'f':
	  {
	    float 
	      ba = tabFloat(sortTable, a, sortCol),
	      bb = tabFloat(sortTable, b, sortCol) ;
	    float rr = ba - bb;
	    int r = ((rr > 0)? 1: -1);
	    if (rr != 0)
	      return sortSpec>0 ? r : -r ;
	  }
	  break;

	case 's':
	  {
	    char 
	      *ba = tabString(sortTable, a, sortCol),
	      *bb = tabString(sortTable, b, sortCol) ;
	    int r = lexstrcmp(ba,bb) ;
	    if (r != 0)
	      return sortSpec>0 ? r : -r ;
	  }
	  break;

	case 't':
	  {
	    mytime_t
	      ba = tabDate(sortTable, a, sortCol),
	      bb = tabDate(sortTable, b, sortCol) ;
	    int r = ba - bb;
	    if (r != 0)
	      return sortSpec>0 ? r : -r ;
	  }
	  break;
	} /* end switch */
    }

  /* default keep original order */
  if (a > b) 
    return +1 ;

  return -1 ;
} /* tableCompare */
/************************************************************/

BOOL tableSort (TABLE *t, char *spec)
/* sorts elements in the table according to
   the sort specifier :
   spec should look like "-2+4+1" which would specify first priority
   column 2 reversed, then column 4, then column 1.

   returns FALSE if table is wrong, or spec is out of range
*/
{
  STORE_HANDLE scopeHandle;
  int i, numRows, specLen, sortCol;
  int *indexArray ;

  if (!tableExists (t))
    messcrash ("tableSort() - received invalid TABLE* pointer");
  if (!spec)
    messcrash ("tableSort() - received NULL sort specifier");

  specLen = strlen(spec);

  if ((specLen % 2))	/* has to have an even number of chars */
    return FALSE;

  sortSpecList = arrayReCreate(sortSpecList, (specLen / 2), int);
  for (i = 0; i < specLen/2; i++)
    {
      sscanf(spec+(i*2)+1, "%1d", &sortCol);

      if (sortCol < 1 || sortCol > t->ncol) 
	/* column numbers have to range from 1 to colNum */
	return FALSE;

      if (spec[i*2] == '+')
	array(sortSpecList, i, int) = sortCol;
      else if (spec[i*2] == '-')
	array(sortSpecList, i, int) = -sortCol;
      else
	return FALSE;		/* only + or - qualifier allowed */
    }

  numRows = tableMax(t);
  if (numRows <= 1) return TRUE; /* 1 or no elements are always sorted */

  /***************************************/

  scopeHandle = handleCreate();

  /* create the indices that keep track of which row went where
     during comparative sorting */
  indexArray = (int*)halloc (numRows * sizeof(int), scopeHandle);

  /* fill the indices with row numbers */
  for (i = 0; i < numRows; ++i)
    indexArray[i] = i;

  /* set global table-var used by compare() to be this one */
  sortTable = t;

  /* rearrange lines */
  qsort (indexArray, numRows, sizeof(int), tableCompare) ;
  
  /**** use index to rearrange column by column ****/

#ifdef NEW_RICHARD_CODE
  /* warning this new code is not adapted to deal
   * with parents/grandParents */
  {
    TABLETYPE tmp ;
    BitSet bs=0 ;
    int k, j;

    for (i = 0 ; i < t->ncol ; ++i)
      { 
	bs = bitSetReCreate (bs, tabMax(t,i));
	for (j = 0 ; j < tabMax(t,i) ; ++j)
	  if (!bit(bs,j))         /* not done yet */
	    if (indexArray[j] != j)
	      { 
		tmp = arr((t)->col[i],j,TABLETYPE) ;
		for (k = j ;
		     !bitSet(bs, indexArray[k]) ;
		     k = indexArray[k])
		  { 
		    arr((t)->col[i],k,TABLETYPE) = 
		      arr((t)->col[i], indexArray[k],TABLETYPE) ;
		    bitSet(bs,k) ;
		  }
		arr((t)->col[i],k,TABLETYPE) = tmp ;
		bitSet(bs,k) ;
	      }
	    else
	      bitSet(bs,j) ;
      }
    
    bitSetDestroy (bs);
  }
#else  /* !NEW_RICHARD_CODE */
 /****** still use previuos code, until we get the new one to work ****/
  {
    Array colArray, oldColArray ;
    KEYSET parentKeys, oldParentKeys, grandParentKeys, oldGrandParentKeys;
    BitSet bb, oldbb ;
    int ii, j, j1;
    
    for (ii = 0 ; ii < t->ncol ; ii++)
      {
	oldColArray = t->col[ii] ;
	colArray = t->col[ii] = arrayHandleCreate (numRows, TABLETYPE, t->handle) ;
	array(colArray,numRows-1,TABLETYPE).k = 0 ; /* make room */

	oldParentKeys = t->parents[ii];
	parentKeys = t->parents[ii] = arrayHandleCreate(numRows, KEY, t->handle);
	array(parentKeys, numRows-1, KEY) = 0; /* make room */

	oldGrandParentKeys = t->grandParents[ii];
	grandParentKeys = t->grandParents[ii] = arrayHandleCreate(numRows, KEY, t->handle);
	array(grandParentKeys, numRows-1, KEY) = 0; /* make room */

	oldbb = t->emptyBits[ii] ;
	bb =  t->emptyBits[ii] = bitSetCreate (numRows, t->handle) ;

	/* copy values and parent/empty info over in the order
	 * of the indexArray, created by the sorting function */
	for (j = 0; j < numRows ; j++)
	  {
	    j1 = indexArray[j] ;
	    arr(colArray,j,TABLETYPE).k = array(oldColArray, j1, TABLETYPE).k ;
	    arr(parentKeys, j, KEY) = array(oldParentKeys, j1, KEY);
	    arr(grandParentKeys, j, KEY) = array(oldGrandParentKeys, j1, KEY);
	    if (bit(oldbb,j1)) 
	      bitSet(bb,j) ;
	  }
	arrayDestroy(oldColArray) ;
	arrayDestroy(oldParentKeys) ;
	arrayDestroy(oldGrandParentKeys) ;
	bitSetDestroy (oldbb) ;
      }
  }
#endif /* !NEW_RICHARD_CODE */
  handleDestroy (scopeHandle);

  return TRUE;
} /* tableSort */
/************************************************************/

void tableMakeUnique (TABLE *t) 
     /* assumes that table has previously been sorted */
{
  int i1, ii, j, mx, nCol;

  if (!tableExists (t))
    messcrash("tableMakeUnique() - received invalid TABLE* pointer");

  mx = tableMax(t);
  nCol = t->ncol;
  
  if (mx < 2 || !nCol) return ;

  i1 = 0;

  for (ii=1 ; ii < mx ; ii++ )
    {
      /* dry pass */
      for (j=0; j<nCol ; j++)
	{
	  if (t->type[j] == 's')
	    {
	      if(strcmp(tabString(t,ii,j), tabString(t,i1,j)) != 0) 
		goto different ;
	    }
	  else
	    {
	      /* any other type we just compare the value
	       * if it was an integer - enough to reveal differences
	       * for ints, bools, even floats */
	      if(tabKey(t,ii,j) != tabKey(t,i1,j)) 
		goto different ;
	    }
	}
      continue ; /* line ii == line i1, drop line ii */
    different:
      i1++ ;   /* accept line ii */
      if (i1 < ii)  /* copy if needed */
	{
	  for (j=0; j<nCol ; j++) 
	    {
	      tabKey(t,i1,j) = tabKey(t,ii,j) ;
	      tabParent(t,i1,j) = tabParent(t,ii,j);
	      tabGrandParent(t,i1,j) = tabGrandParent(t,ii,j);
	      if (tabEmpty(t,ii,j))
		bitSet(t->emptyBits[j],i1) ;
	      else
		bitUnSet(t->emptyBits[j],i1) ;
	    }
	}
    }
  for (j=0; j<nCol ; j++)
    arrayMax(t->col[j]) = i1 + 1 ;

  return;
} /* tableMakeUnique */
/************************************************************/

/************************************************/
/******* table operator utility functions *******/
/************************************************/

/* Function to check whether two tables are compatible, that
   they can be used for the subsequent table operators together. */
/*
   NOTE: Once this check is performed on both tables the short version
   tabXXX() of the operations can be used. In case it is only a one-off
   statement that uses a table operation, the tableXXXX() routines 
   should be used to include the type checking from this function
*/
BOOL tableIsColumnCompatible (TABLE *leftTable, 
			      TABLE *rightTable)
{
  int colCount;

  if (leftTable == NULL || rightTable == NULL)
    return FALSE;

  /* we check that the value-types of the columns match in both tables */
  if (strcmp(leftTable->type, rightTable->type) != 0)
    return FALSE;

  /* check for unevaluated columns, ideally we'd just like to exclude them */
  for (colCount = 0; colCount < leftTable->ncol; colCount++)
    {
      if (leftTable->type[colCount] == '0' || rightTable->type[colCount] == '0')
	return FALSE;
    }

  return TRUE;
} /* tableIsColumnCompatible */

/************************************************************/

/* function to compare a complete row in a table with a row in a second table. */
/*
   Returns FALSE if tables are incompatible, type mismatch or uninitialised
   ONLY returns TRUE if full compatability and if all values in the two rows match
*/
BOOL tableIsRowEqual (int    rowCountLeft, 
		      TABLE *leftTable, 
		      int    rowCountRight, 
		      TABLE *rightTable)
{
  if (!tableIsColumnCompatible(leftTable, rightTable))
    return FALSE;

  return (tabIsRowEqual(rowCountLeft, leftTable, rowCountRight, rightTable));
} /* tableIsRowEqual */

/************************************************************/

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
		    TABLE *rightTable)
{
  int  colCount;
  BOOL isValueDifferent = FALSE;

  for (colCount = 0; colCount < leftTable->ncol; colCount++)
    {
      switch (leftTable->type[colCount])
	{
	case 'i':
	  if (tabInt(leftTable, rowCountLeft, colCount) != tabInt(rightTable, rowCountRight, colCount))
	    isValueDifferent = TRUE;
	  break;
	case 'f':
	  if (tabFloat(leftTable, rowCountLeft, colCount) != tabFloat(rightTable, rowCountRight, colCount))
	    isValueDifferent = TRUE;
	  break;
	case 'b':
	  if (tabBool(leftTable, rowCountLeft, colCount) != tabBool(rightTable, rowCountRight, colCount))
	    isValueDifferent = TRUE;
	  break;
	case 't':
	  if (tabDate(leftTable, rowCountLeft, colCount) != tabDate(rightTable, rowCountRight, colCount))
	    isValueDifferent = TRUE;
	  break;
	case 'k':		/* keys */
	case 'g':		/* tags */
	  if (tabKey(leftTable, rowCountLeft, colCount) != tabKey(rightTable, rowCountRight, colCount))
	    isValueDifferent = TRUE;
	  break;
	case 's':
	  if (strcmp(tabString(leftTable, rowCountLeft, colCount), tabString(rightTable, rowCountRight, colCount)) != 0)
	    isValueDifferent = TRUE;
	  break;
	}
    }
  if (!isValueDifferent)
    return TRUE;

  return FALSE;
} /* tabIsRowEqual */

/************************************************************/

STORE_HANDLE globalTableCreateHandle;

/* function to copy all values in a row of the srcTable to the end of the destTable */
/*
   The destTable is created if it is not existent.
   The function returns FALSE, if an existing destTable is not type-compatible
   with the source table.
   Otherwise, the values in the specified row are appended to the end
   of the destTable and TRUE is returned.
*/
BOOL tableCopyRow (int    rowCountSrc, 
		   TABLE *srcTable, 
		   TABLE *destTable)
{
  if (!destTable)
    {
      /* we have to initialise the destTable to be of type and value of srcTable */
      destTable = tableHandleCreate (tableMax(srcTable), tableTypes(srcTable), globalTableCreateHandle) ;
    }
  else
    {
      /* a destination table exists, but it is compatible ? */
      if (!tableIsColumnCompatible(srcTable, destTable))
	return FALSE;
    }

  tabCopyRow (rowCountSrc, srcTable, destTable);

  return TRUE;
} /* tableCopyRow */

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
		 TABLE *destTable)
{
  int colCount;
  int rowCountDest = tableMax(destTable);

  for (colCount = 0; colCount < srcTable->ncol; colCount++)
    {
      switch (srcTable->type[colCount])
	{
	case 'i':
	  tableInt(destTable, rowCountDest, colCount) = tabInt(srcTable, rowCountSrc, colCount);
	  break;
	case 'f':
	  tableFloat(destTable, rowCountDest, colCount) = tabFloat(srcTable, rowCountSrc, colCount);
	  break;
	case 'b':
	  tableBool(destTable, rowCountDest, colCount) = tabBool(srcTable, rowCountSrc, colCount);
	  break;
	case 't':
	  tableDate(destTable, rowCountDest, colCount) = tabDate(srcTable, rowCountSrc, colCount);
	  break;
	case 'k':
	  tableKey(destTable, rowCountDest, colCount) = tabKey(srcTable, rowCountSrc, colCount);
	  break;
	case 's':
	  tableSetString(destTable, rowCountDest, colCount, tabString(srcTable, rowCountSrc, colCount));
	  break;
	case '0':
	  tableSetEmpty(destTable, rowCountDest, colCount);
	  break;

	}
    }
} /* tabCopyRow */
/************************************************************/

/* produce a combined table that contains rows from both
   tables but no duplicates */
/* fails with return code FALSE, if the two tables are incombinable
   or in case an existing table pointer was passed, this has
   already been assigned column types, otherwise a new table
   will be returned in *destTable 
*/
BOOL tableUnion (TABLE *leftTable,
		 TABLE *rightTable,
		 TABLE **destTable)
{
  int rowCountLeft, rowCountRight;
  int lengthLeft, lengthRight;
  BOOL isInLeftTable;

  if (!destTable)	   /* has to be called with a pointer to TABLE* 
			      in order to return results */
    messcrash ("table.c:tableUnion() - destTable pointer to TABLE* is NULL");


  if (!tableIsColumnCompatible(leftTable, rightTable))
    return FALSE;

  lengthLeft = tableMax(leftTable);
  lengthRight = tableMax(rightTable);

  if (!*destTable)		/* table it points to is still NULL */
    {
      *destTable = tableHandleCreate((lengthLeft + lengthRight/2),
				     leftTable->type, globalTableCreateHandle);
    }
  else				/* we got passed a pointer of an existing table */
    {
      if (strlen(leftTable->type) != strlen((*destTable)->type))
	messcrash ("table.c:tableUnion() - previously created destTable has incorrect column number");

      if (!tableSetTypes(*destTable, leftTable->type))
	return FALSE;		/* fails if types were already set */
    }

  /********/
  /* include all rows from leftTable and the ones from rightTable we haven't got already */
  /********/

  /* copy all rows from the left table */
  for (rowCountLeft = 0; rowCountLeft < lengthLeft; rowCountLeft++)
    {
      tabCopyRow(rowCountLeft, leftTable, *destTable);
    }
  
  /* only copy rows from right table that we haven't got already */
  for (rowCountRight = 0; rowCountRight < lengthRight; rowCountRight++)
    {
      /* look if we have this row already */
      isInLeftTable = FALSE;
      for (rowCountLeft = 0; rowCountLeft < lengthLeft; rowCountLeft++)
	if (tabIsRowEqual(rowCountLeft, leftTable,
			  rowCountRight, rightTable))
	  {
	    isInLeftTable = TRUE;
	    break;
	  }
      if (!isInLeftTable)
	tabCopyRow(rowCountRight, rightTable, *destTable);
    }


  return TRUE;
} /* tableUnion */
/************************************************************/



/* produce a table with rows that are in the first but 
   not the second table */
/* fails with return code FALSE, if the two tables are incombinable
   or in case an existing table pointer was passed, this has
   already been assigned column types, otherwise a new table
   will be returned in *destTable 
*/
BOOL tableDiff (TABLE *leftTable,
		TABLE *rightTable,
		TABLE **destTable)
{
  int rowCountLeft, rowCountRight;
  int lengthLeft, lengthRight;
  BOOL isInRightTable;

  if (!destTable)	   /* has to be called with a pointer to TABLE* 
			      in order to return results */
    messcrash ("tableDiff() - destTable pointer to TABLE* is NULL");


  if (!tableIsColumnCompatible(leftTable, rightTable))
    return FALSE;

  lengthLeft = tableMax(leftTable);
  lengthRight = tableMax(rightTable);

  if (!*destTable)		/* table it points to is still NULL */
    {
      *destTable = tableHandleCreate((lengthLeft - lengthRight/2),
				     leftTable->type, globalTableCreateHandle);
    }
  else				/* we got passed a pointer of an existing table */
    {
      if (strlen(leftTable->type) != strlen((*destTable)->type))
	messcrash ("tableDiff() - previously created destTable has incorrect column number");

      if (!tableSetTypes(*destTable, leftTable->type))
	return FALSE;		/* fails if types were already set */
    }

  /********/
  /* only include rows from the leftTable that aren't in the rightTable */
  /********/

  for (rowCountLeft = 0; rowCountLeft < lengthLeft; rowCountLeft++)
    {
      /* look for this row in the rightTable */
      isInRightTable = FALSE;
      for (rowCountRight = 0; rowCountRight < lengthRight; rowCountRight++)
	if (tabIsRowEqual(rowCountLeft, leftTable,
			  rowCountRight, rightTable))
	  {
	    isInRightTable = TRUE;
	    break;
	  }
      if (!isInRightTable)
	/* not found -> copy left row into destinationTable */
	tabCopyRow(rowCountLeft, leftTable, *destTable);
    }

  return TRUE;
} /* tableDiff */
/************************************************************/



/* produce a table with rows that are in the first but 
   not the second table */
/* fails with return code FALSE, if the two tables are incombinable
   or in case an existing table pointer was passed, this has
   already been assigned column types, otherwise a new table
   will be returned in *destTable 
*/
BOOL tableIntersect (TABLE *leftTable,
		     TABLE *rightTable,
		     TABLE **destTable)
{
  int rowCountLeft, rowCountRight;
  int lengthLeft, lengthRight;
  BOOL isInRightTable;

  if (!destTable)	   /* has to be called with a pointer to TABLE* 
			      in order to return results */
    messcrash ("tableIntersect() - destTable pointer to TABLE* is NULL");


  if (!tableIsColumnCompatible(leftTable, rightTable))
    return FALSE;

  lengthLeft = tableMax(leftTable);
  lengthRight = tableMax(rightTable);

  if (!*destTable)		/* table it points to is still NULL */
    {
      *destTable = tableHandleCreate((lengthLeft/2 + lengthRight/2),
				     leftTable->type, globalTableCreateHandle);
    }
  else				/* we got passed a pointer of an existing table */
    {
      if (strlen(leftTable->type) != strlen((*destTable)->type))
	messcrash ("tableIntersect() - previously created destTable has incorrect column number");

      if (!tableSetTypes(*destTable, leftTable->type))
	return FALSE;		/* fails if types were already set */
    }

  /********/
  /* only include rows that are shared between leftTable and rightTable */
  /********/

  for (rowCountLeft = 0; rowCountLeft < lengthLeft; rowCountLeft++)
    {
      /* look for this row in the rightTable */
      isInRightTable = FALSE;
      for (rowCountRight = 0; rowCountRight < lengthRight; rowCountRight++)
	if (tabIsRowEqual(rowCountLeft, leftTable,
			  rowCountRight, rightTable))
	  {
	    isInRightTable = TRUE;
	    break;
	  }
      if (isInRightTable)
	/* left was found in right -> copy left row into destinationTable */
	tabCopyRow(rowCountLeft, leftTable, *destTable);
    }

  return TRUE;
} /* tableIntersect */
/************************************************************/


/************************************************************/
/********** functions to output/export tables ***************/
/************************************************************/

int tableOut (ACEOUT dump_out, TABLE *t, char separator, char style)
     /* write whole table from row 0 till the end including headers */
{ 
  int n ;

  tableHeaderOut (dump_out, t, style) ;

  n = tableSliceOut (dump_out, 0, tableMax(t), t, separator, style) ;
  tableFooterOut (dump_out, t, style) ;

  return n ;
} /* tableOut */

/************************************************************/

/* This routine was torn out of tableSliceOut -- see how it is
   called from there, for how to use it. -- JCGS 20010911 */

void tableOutSetup(TABLE *t,
                   struct tableOutControl *taboutcontrol)
{
    DICT **colFlagDict = taboutcontrol->colFlagDict;
    char **colFlagSet = taboutcontrol->colFlagSet;
    Array oldClass = taboutcontrol->oldClass;

  int i;
  char *cp;
	/* preparation for flags */
	/* RD 000615 - this section was protected by 'if (doShowHeaders)' 
	   but that is nonsense.  It has nothing to do with headers.  It
	   is required if there are 'a' (flag) type columns to be shown.
	   I bet that 'a' columns are not used.
	*/
    for (i = 0 ; i < t->ncol ; ++i)
    {
        if (t->type[i] == 'a')
        {
            if (!(cp = tableGetColumnName(t,i)))
                cp = "Default" ;
            if (strncmp (cp, "Flag", 4) == 0 ||
                strncmp (cp, "flag", 4) == 0)
            { 
                cp += 4 ;
                if (*cp == 's')
                    ++cp ; /* remove "flags" as well as "flag" */
                if (!*cp)
                    cp = "Default" ;
            }
            colFlagSet[i] = cp ;
            colFlagDict[i] = flagSetDict (cp) ;
        }

/*
  RD 000615 - this looks wrong to me
  should always be -1 so you write class first time 
  so I am commenting it out

  if ((t->type[i] == 'k' || t->type[i] == 'g') && 
  tabMax(t,i) && !tabEmpty(t,0,i))
  array (oldClass, i, int) = class(tabKey(t,0,i));
  else
*/
        array (oldClass, i, int) = -1 ;

    }
}

/************************************************************/

/* This routine was torn out of tableSliceOut -- see how it is
   called from there, for how to use it. -- JCGS 20010911 */

/* It might be good to set up a closure structure containing
   the control arguments to this, but on the other hand, I
   have no great motivation to do so, other than the sheer
   number of little fiddly arguments. */
void tableRowOut (ACEOUT dump_out, 
                  int row, 
                  TABLE *t, 
                  struct tableOutControl *taboutcontrol)
{ 
    int column;
    char sepString[2] ;
    KEY xk ;
    int xi ; float xf ; mytime_t xt ;
    char *cp ; 
    char style = taboutcontrol->style;
    char bufvoid[1] = {'v'} ;
    char bufinteger[1] = {'i'} ;
    char buffloat[1] = {'f'} ;
    char bufdate[1] = {'d'} ;
    char buftext[1] = {'t'} ;
    char bufk[2] = {'k', 0} ;
    char bufK[2] = {'K', 0} ;
    char buftag[1] = {'g'} ;

    sepString[0] = taboutcontrol->separator ;
    sepString[1] = 0 ;
    
    if (style == 'x' || style == 'X' || style == 'y' || style == 'Y')
        aceOutPrint(dump_out, "<tr> ") ;

    for (column = 0 ; column < t->ncol ; ++column)
	{
#if 0
        aceOutPrint(dump_out, "tableRowOut(row=%d, column=%d)\n", row, column);
#endif
        if (!tableColumnIsVisible(t, column))
            continue;

	if (style == 'C')
	  {
	    aceOutBinary(dump_out, taboutcontrol->cStylePrefix, 1);
	    taboutcontrol->cStylePrefix[0] = '>' ;
	  }
        else if (column > 0)
	  aceOutPrint (dump_out, "%s", sepString) ;

        if (row >= tabMax(t,column) || tabEmpty(t,row,column) || t->type[column] == '0')
	    {
            if (taboutcontrol->showEmpty) 
                switch (style)
                {
                case 'j':  case 'J': case 'k':  case 'K':
                    aceOutPrint (dump_out,"???") ;
                    break ;
                case 'x': case 'y': case 'X':	/* xml with content, like html */
                    aceOutPrint (dump_out,"<td>&nbsp;</td>") ;
                    break ;
                case'Y':	/* xml without content */
                    aceOutPrint (dump_out,"<td/>") ;
                    break ;
		case 'C':
                    aceOutBinary(dump_out, bufvoid, 1) ;
		    break;
                default:
                    aceOutPrint (dump_out,"NULL") ;
                    break ;
                }
	    }
        else
	    { 
#if 0
        aceOutPrint(dump_out, "t->type[%d]=%c\n", column, t->type[column]);
#endif
            switch (t->type[column])
            { 

            case 'k':
            case 'g':
                xk = tabKey(t,row,column) ;
                switch (style)
                {
                case 'h': 
                    aceOutPrint (dump_out, "%s", name(xk)) ;
                    break ;
		case 'C':
		  switch (t->type[column])
		    {
		    case 'g':
		      aceOutBinary ( dump_out, buftag,1) ;
		      aceOutBinary ( dump_out, (char*)&xk, 4) ;
		      break ;
		    case 'k':
		      if (iskey (xk) == 2)
			{ bufK[1] = class (xk) ; aceOutBinary ( dump_out,bufK,2) ; }
		      else
			{ bufk[1] = class (xk) ; aceOutBinary ( dump_out, bufk,2) ; }
		      cp = name(xk) ;
		      aceOutBinary ( dump_out, cp, strlen(cp) + 1) ;
		      break ;
		    }			  
		  break;
		case 'j': case 'k':
		  aceOutPrint (dump_out, "?%s?%s?", className(xk), 
			       freejavaprotect(name(xk))) ;
		  break ;
                case 'J': case 'K':
                    if (column > taboutcontrol->javatypeWatermark || class(xk) != array (taboutcontrol->oldClass, column, int)) 
                        aceOutPrint (dump_out, "?%s?%s?", className(xk), 
                                     freejavaprotect(name(xk))) ;
                    else
                        aceOutPrint (dump_out, "?%s?", 
                                     freejavaprotect(name(xk))) ;
                    array (taboutcontrol->oldClass, column, int) = class(xk) ;
                    break ;
                case 'a': 
		  if (class(xk) == array (taboutcontrol->oldClass, column, int)) 
                    {
		      aceOutPrint (dump_out, "%s", freeprotect(name(xk))) ;
                        break ;
                    }
		  array (taboutcontrol->oldClass, column, int) = class(xk) ;
		  /* else fall through */			
                case 'A': 
		  aceOutPrint (dump_out, "%s:%s", className(xk), 
			       freeprotect(name(xk))) ;
		  break ;
                case 'x':
                    aceOutPrint (dump_out, "<td>%s</td>", 
                                 freeXMLprotect (name(xk))) ; 
                    break ;
                case 'X':
                    aceOutPrint (dump_out, "<td class=\"%s\">%s</td>", 
                                 class(xk) ? className(xk) : "#Tag", 
                                 freeXMLprotect (name(xk))) ; 
                    break ;
                case 'y':
                    aceOutPrint (dump_out, "<td class=\"%s\" value=\"%s\">%s</td>", 
                                 class(xk) ? className(xk) : "#Tag", 
                                 freeXMLprotect (name(xk)), 
                                 freeXMLprotect (name(xk))) ; 
                    break ;
                case 'Y':
                    aceOutPrint (dump_out, "<td class=\"%s\" value=\"%s\"/>", 
                                 class(xk) ? className(xk) : "#Tag", 
                                 freeXMLprotect (name(xk))) ; 
                    break ;
                default:
                    aceOutPrint (dump_out, "%s", freeprotect(name(xk))) ;
                    break ;
                }
                break ;

            case 'a':
                cp = flagNameFromDict(taboutcontrol->colFlagDict[column], tabFlag(t,row,column)) ;
                switch (style)
                {
                case 'j': case 'k':
                    aceOutPrint (dump_out, "?flag%s?%s?", taboutcontrol->colFlagSet[column], cp) ;
                    break ;
                case 'J': case 'K':
                    if (column > taboutcontrol->javatypeWatermark) aceOutPrint (dump_out, "flag%s", taboutcontrol->colFlagSet[column]) ;
                    aceOutPrint (dump_out, "?%s?", cp) ;
                    break ;
                case 'C':
		  aceOutBinary ( dump_out, buftext,1) ;
		  aceOutBinary ( dump_out, cp, strlen(cp) + 1) ;
		  break ;
                case 'x':
		  aceOutPrint (dump_out, "<td>%s</td>", cp) ; 
		  break ;
                case 'X':
                    aceOutPrint (dump_out, "<td flags=\"%s\">%s</td>", 
                                 taboutcontrol->colFlagSet[column], cp) ;
                    break ;
                case 'y':
                    aceOutPrint (dump_out, "<td flags=\"%s\" value=\"%s\">%s</td>", 
                                 taboutcontrol->colFlagSet[column], cp, cp) ;
                    break ;
                case 'Y':
                    aceOutPrint (dump_out, "<td flags=\"%s\" value=\"%s\"/>", 
                                 taboutcontrol->colFlagSet[column], cp) ;
                    break ;
                default:
                    aceOutPrint (dump_out, "%s", cp) ;
                    break ;
                }
                break ;

            case 'b':
                cp = tabBool(t,row,column) ? "TRUE" : "FALSE" ;
                switch (style)
                {
                case 'j':
                    aceOutPrint (dump_out, "?BOOL?%s?", cp) ;
                    break ;
                case 'J': 
                    if (column > taboutcontrol->javatypeWatermark) aceOutPrint (dump_out, "BOOL") ;
                    aceOutPrint (dump_out, "?%s?", cp) ;
                    break ;
                case 'C':
		  aceOutBinary ( dump_out, bufinteger,1) ;
		  if (tabBool(t,row,column))
		    xi = 1 ;
		  else
		    xi = 0 ;
		  aceOutBinary ( dump_out, (char*)&xi, 4) ;
		  break ;
                case 'k':
                    aceOutPrint (dump_out, "?#Bool?%s?", cp) ;
                    break ;
                case 'K': 
                    if (column > taboutcontrol->javatypeWatermark) aceOutPrint (dump_out, "#Bool") ;
                    aceOutPrint (dump_out, "?%s?", cp) ;
                    break ;
                case 'x':
                    aceOutPrint (dump_out, "<td>%s</td>", cp) ; 
                    break ;
                case 'X':
                    aceOutPrint (dump_out, "<td class=\"#Bool\">%s</td>", cp) ;
                    break ;
                case 'y':
                    aceOutPrint (dump_out, "<td class=\"#Bool\" value=\"%s\">%s</td>", cp, cp) ;
                    break ;
                case 'Y':
                    aceOutPrint (dump_out, "<td class=\"#Bool\" value=\"%s\"/>", cp) ;
                    break ;
                default:
                    aceOutPrint (dump_out, "%s", cp) ;
                    break ;
                }
                break ;

            case 'i':
                xi = tabInt(t,row,column) ;
                switch (style)
                {
                case 'j': 
                    aceOutPrint (dump_out, "?int?%d?", xi) ;
                    break ;
                case 'J': 
                    if (column > taboutcontrol->javatypeWatermark) aceOutPrint (dump_out, "int") ;
                    aceOutPrint (dump_out, "?%d?", xi) ;
                    break ; 
                case 'C':
                    aceOutBinary ( dump_out, bufinteger,1) ;
                    aceOutBinary ( dump_out, (char*)&xi, 4) ;
                    break ;
                case 'k': 
                    aceOutPrint (dump_out, "?#Int?%d?", xi) ;
                    break ;
                case 'K': 
                    if (column > taboutcontrol->javatypeWatermark) aceOutPrint (dump_out, "#Int") ;
                    aceOutPrint (dump_out, "?%d?", xi) ;
                    break ; 
                case 'x':
                    aceOutPrint (dump_out, "<td>%d</td>", xi) ; 
                    break ;
                case 'X':
                    aceOutPrint (dump_out, "<td class=\"#Int\">%d</td>", xi) ;
                    break ;
                case 'y':
                    aceOutPrint (dump_out, "<td class=\"#Int\" value=\"%d\">%d</td>", xi, xi) ;
                    break ;
                case 'Y':
                    aceOutPrint (dump_out, "<td class=\"#Int\" value=\"%s\"/>", xi) ;
                    break ;
                case 'A': 
                    aceOutPrint (dump_out, "Int:%d", xi) ;
                    break ;
                default:
                    aceOutPrint (dump_out, "%d", xi) ;
                    break ;
                }
                break ;

            case 'f':
                xf = tabFloat (t,row,column) ;
                switch (style)
                {
                case 'j': 
                    aceOutPrint (dump_out, "?float?%g?", xf) ;
                    break ;
                case 'J': 
                    if (column > taboutcontrol->javatypeWatermark) aceOutPrint (dump_out, "float") ;
                    aceOutPrint (dump_out, "?%g?", xf) ;
                    break ;
                case 'C':
                    aceOutBinary ( dump_out, buffloat,1) ;
                    aceOutBinary ( dump_out, (char*)&xf, 4) ;
                    break ;
                case 'k': 
                    aceOutPrint (dump_out, "?#Float?%g?", xf) ;
                    break ;
                case 'K': 
                    if (column > taboutcontrol->javatypeWatermark) aceOutPrint (dump_out, "#Float") ;
                    aceOutPrint (dump_out, "?%g?", xf) ;
                    break ;
                case 'x':
                    aceOutPrint (dump_out, "<td>%g</td>", xf) ; 
                    break ;
                case 'X':
                    aceOutPrint (dump_out, "<td class=\"#Float\">%g</td>", xf) ;
                    break ;
                case 'y':
                    aceOutPrint (dump_out, "<td class=\"#Float\" value=\"%g\">%g</td>", xf, xf) ;
                    break ;
                case 'Y':
                    aceOutPrint (dump_out, "<td class=\"#Float\" value=\"%g\"/>", xf) ;
                    break ;
                case 'A': 
                    aceOutPrint (dump_out, "Float:%g", xf) ;
                    break ;
                default:
                    aceOutPrint (dump_out, "%g", xf) ;
                    break ;
                }
                break ;

            case 's':
                cp = tabString(t,row,column) ; 
                switch (style)
                {
                case 'j': 
                    aceOutPrint (dump_out, "?txt?%s?", freejavaprotect(cp)) ; 
                    break ;
                case 'J': 
                    if (column > taboutcontrol->javatypeWatermark) aceOutPrint (dump_out, "txt") ;
                    aceOutPrint (dump_out, "?%s?", freejavaprotect(cp)) ; 
                    break ; 
                case 'C':
                    aceOutBinary (dump_out, buftext,1) ;
                    aceOutBinary (dump_out, cp, strlen(cp) + 1) ;
                    break ;
                case 'k': 
                    aceOutPrint (dump_out, "?#Text?%s?", freejavaprotect(cp)) ; 
                    break ;
                case 'K': 
                    if (column > taboutcontrol->javatypeWatermark) aceOutPrint (dump_out, "#Text") ;
                    aceOutPrint (dump_out, "?%s?", freejavaprotect(cp)) ; 
                    break ; 
                case 'h': 
                    aceOutPrint (dump_out, "%s", cp) ;
                    break ;
                case 'A': 
                    aceOutPrint (dump_out, "Text:%s", freeprotect(cp)) ; 
                    break ;
                case 'x':
                    aceOutPrint (dump_out, "<td>%s</td>", freeXMLprotect(cp)) ; 
                    break ;
                case 'X':
                    aceOutPrint (dump_out, "<td class=\"#Text\">%s</td>", 
                                 freeXMLprotect(cp)) ;
                    break ;
                case 'y':
                    aceOutPrint (dump_out, "<td class=\"#Text\" value=\"%s\">%s</td>",
                                 freeXMLprotect(cp), freeXMLprotect(cp)) ;
                    break ;
                case 'Y':
                    aceOutPrint (dump_out, "<td class=\"#Text\" value=\"%s\"/>", 
                                 freeXMLprotect(cp)) ;
                    break ;
                default:
                    aceOutPrint (dump_out, "%s", freeprotect(cp)) ;
                    break ;
                }
                break ;

            case 't':
                xt = tabDate(t,row,column) ;
                switch (style)
                {
                case 'j': 
                    aceOutPrint (dump_out, "?date?%s?", timeShowJava(xt)) ; 
                    break ;
                case 'J': 
                    aceOutPrint (dump_out, "date") ;
                    aceOutPrint (dump_out, "?%s?", timeShowJava(xt)) ; 
                    break ; 
                case 'C':
                    aceOutBinary (dump_out, bufdate,1) ;
                    aceOutBinary (dump_out, (char*)&xt, 4) ;
                    break ;
                case 'k': 
                    aceOutPrint (dump_out, "?#Date?%s?", timeShow(xt)) ; 
                    break ;
                case 'K': 
                    aceOutPrint (dump_out, "#Date") ;
                    aceOutPrint (dump_out, "?%s?", timeShow(xt)) ; 
                    break ; 
                case 'h': 
                    aceOutPrint (dump_out, "%s", timeShow(xt)) ;
                    break ; 
                case 'A': 
                    aceOutPrint (dump_out, "Date:%s", timeShow(xt)) ; 
                    break ;
                case 'x':
                    aceOutPrint (dump_out, "<td>%s</td>", timeShow(xt)) ; 
                    break ;
                case 'X':
                    aceOutPrint (dump_out, "<td class=\"#Date\">%s</td>", timeShow(xt)) ;
                    break ;
                case 'y':
                    aceOutPrint (dump_out, "<td class=\"#Date\" value=\"%s\">%s</td>", 
                                 timeShow(xt), timeShow(xt)) ;
                    break ;
                case 'Y':
                    aceOutPrint (dump_out, "<td class=\"#Date\" value=\"%s\"/>", timeShow(xt)) ;
                    break ;
                default:
                    aceOutPrint (dump_out, freeprotect(timeShow(xt))) ;
                    break ;
                }
                break ;

            default:
                messcrash ("tableRowOut() - invalid column type %c", t->type[column]) ;
                break ;
            }
            if (column > taboutcontrol->javatypeWatermark) taboutcontrol->javatypeWatermark = column ; /* column's type has now been exported to java */
	    }
	}
    
    switch (style)
      {
      case 'x':
      case 'X':
      case 'y':
      case 'Y':
        aceOutPrint (dump_out, "</tr>\n") ;
  	break;
      case 'C':
	{
	  char bfret[3];
	  
	  bfret[0] = 'l';
	  xi = t->ncol-1 ;
	  while (xi > 0 && xi > 255)
	    {
	      bfret[1]= 255; aceOutBinary( dump_out, bfret, 2) ;
	      xi -= 255;
	    }
	  
	  bfret[1] = xi ; bfret[2] = '\n' ;
	  aceOutBinary( dump_out, bfret, 3) ;
	  taboutcontrol->cStylePrefix[0] = '.';
	}
	break;
      default:
	aceOutPrint (dump_out, "\n") ;
      }
}

/************************************************************/

int tableSliceOut (ACEOUT dump_out, 
                   int begin, 
                   int count, 
                   TABLE *t, 
                   char separator, 
                   char style)
{
    int row = 0, nMax, end;
    struct tableOutControl outcontrol;

    outcontrol.showEmpty = FALSE ;
 
    if (style == 'a' || 
        style == 'j' ||  style == 'J' ||
        style == 'x' || style == 'X' || style == 'y' || style == 'Y' ||
        style == 'k' ||  style == 'K' ||
	style == 'C' )
        outcontrol.showEmpty = TRUE ; 

    outcontrol.style = style;

    if (!t) return 0;
    if (t->magic != &TABLE_MAGIC)
        messcrash ("tableSliceOut() received non-magic TABLE* pointer") ;
    if (t->ncol <= 0) return 0 ;

    nMax = tableMax(t);

    /* limit the begin-value to be within bounds */
    begin = (begin < 0) ? 0 : begin;
    if (nMax)
        begin = (begin > nMax-1) ? nMax-1 : begin;
    /* limit the number of rows to be within bounds */
    count = (count < 0) ? 0 : count;
    /* calc the end of the table */
    end = begin + count;
    end = (end > nMax) ? nMax : end;

    outcontrol.oldClass = arrayCreate (t->ncol, int) ;

    outcontrol.colFlagDict = (DICT**) messalloc (t->ncol * sizeof(DICT*)) ;
    outcontrol.colFlagSet = (char**) messalloc (t->ncol * sizeof(char*)) ;
    outcontrol.separator = separator;
    outcontrol.cStylePrefix [0] = '>' ;

    tableOutSetup(t, &outcontrol);
    
    /* now dump the rows */
    
    
    outcontrol.javatypeWatermark = -1 ;
    for (row = begin ; row < end ; row++)
      {
        tableRowOut(dump_out, 
                    row, 
                    t,
                    &outcontrol);
      }
    
    if (style == 'C')
      {
	/*
	 * We are now at the end of an encore block.  At the beginning of the
	 * next encore block, we will move the cursor right one before depositing
	 * data, so here we move the cursor left to get it back to column -1.
	 */
	aceOutBinary ( dump_out, "l\001.",3);
      }
    
    messfree (outcontrol.colFlagSet) ;
    messfree (outcontrol.colFlagDict) ;
    arrayDestroy (outcontrol.oldClass) ;

    return row ;			/* last row written */
} /* tableSliceOut */

/************************************************************/

/*
* bug: returns a BOOL but nobody ever looks at it
*/

BOOL tableHeaderOut (ACEOUT dump_out, TABLE *table, char style)
{
  int i, j ;
  KEY key ;

  if (!table || ! tableMax(table))
    return FALSE ;
    
  switch (style)
    { 
    case 'a': case 'h':
      aceOutPrint (dump_out, "Format ") ;
      break ;
    case 'x': case 'X': case 'y': case 'Y':
      aceOutPrint (dump_out, "<table>\n<th>") ;
      break ;
    case 'C':
      return FALSE;
    default:
      return FALSE ;			/* NOTE return HERE */
    }

  for(j = 0 ; j < table->ncol; j++)
    {
      if (!tableColumnIsVisible(table, j))
	continue;

      switch (tableTypes(table)[j])
	{
	case 'k':
	case 'g':
	  for (i = 0 ; i < tabMax(table, j) ; i++)
	    if (!tabEmpty(table,i,j) && class(tabKey(table,i,j)))
	      {
		key = tabKey(table,i,j) ;
		aceOutPrint (dump_out, " %s ", class(key) ? className(key) : "Tag") ; 
		break ;
	      }
	  if (i == tabMax(table,j)) /* nothing found */
	    aceOutPrint (dump_out, " Key ") ;
	  continue ;
	case 'b':
	  aceOutPrint (dump_out, " Bool ") ;
	  continue ;
	case 'i':
	  aceOutPrint (dump_out, " Int ") ;
	  continue ;
	case 'f':
	  aceOutPrint (dump_out, " Float ") ;
	  continue ;
	case 's':
	  aceOutPrint (dump_out, " Text ") ;
	  continue ;
	case 't':
	  aceOutPrint (dump_out, " Date ") ;
	  continue ;
	case 'a':
	  aceOutPrint (dump_out, " Flag ") ;
	  continue ;
	}
    }
  if (style == 'x' || style == 'X' || style == 'y' || style == 'Y')
    aceOutPrint (dump_out, "</th>") ;
  aceOutPrint (dump_out, "\n") ;

  return TRUE ;
} /* tableHeaderOut */

void tableFooterOut (ACEOUT dump_out, TABLE *t, char style)
{ 
  switch (style)
    { 
    case 'x': case 'X': case 'y': case 'Y':
      aceOutPrint (dump_out, "</table>\n") ;
    }
}

int tablePartOut (ACEOUT dump_out, int *lastp, TABLE *t, char separator, char style)
     /* then *lastp will be set to the row-number that was last written
      * if output is finished *lastp is set to 0 */
     /* global for now, should go by chunk */
{ 
  int last_row;

  *lastp = 0 ;  /* global dump for now - 0 means output was finished */

  last_row = tableOut (dump_out, t, separator, style);

  return last_row;
} /* tablePartOut */


BOOL tableResultDump (ACEOUT dump_out, KEY k) 
     /* DumpFuncType for class _VTableResult */
{
  TABLE *table = tableGet(k) ; 

  if (!table || ! tableMax(table))
    return FALSE ;

  tableOut (dump_out, table, '\t', 'a') ;
  aceOutPrint (dump_out, "\n") ;
  tableDestroy (table) ;
  
  return TRUE ;
} /* tableResultDump */

static TABLE *tableDoDoParse (ACEIN parse_io, char *table_name, char *format_type,
			      Array oldClass, char **err_text)
{
  char *cp, *cq, *ctype ;
  TABLE *tt = 0 ;
  int xi ; float xf ; mytime_t xt ; KEY xk ;
  int ii, nn ;
  char *errText ;

  ctype = format_type - 1 ; ii = -1 ;
  while (ii++, *++ctype)
    switch (*ctype)
      {
      case 'k': case 'g': case 'b': case 'i': case 'f': case 's': case 't': 
	continue ;
      case 'a':
	errText = "tableDoDoParse cannot parse a type: flags" ;
	goto abort ;
      default:
	*err_text = NULL ;
	return NULL ;
      }

  tt = tableHandleCreate (100, format_type, 0);

  nn = -1 ; /* line number */
  while (aceInCard(parse_io))
    {
      nn++ ;
      ctype = format_type - 1 ; ii = -1 ;
      while (ii++, *++ctype)
	{
	  aceInNext (parse_io) ;
	  switch (*ctype)
	    {
	    case 'k':
	    case 'g':
	      cp = aceInWord(parse_io) ;
	      if (!cp) 
		{
		  if (ii) 
		    { errText = "missing object name" ; 
		      goto abort ;
		    }
		  else 
		    return tt ;
		}
	      if (strcasecmp(cp,"NULL") == 0)
		{ tableKey (tt, nn,ii) = 0 ;
		  break ;
		}
	      cq = cp ;
	      while (*cq && *cq != ':') cq++ ;
	      if (*cq == ':')
		{
		  *cq = 0 ;
		  if ((xi = pickWord2Class(cp)) &&
		      !pickList[xi].protected)
		    { *cq = ':' ;
		      array(oldClass, ii, int) = xi ;
		       /* but beware of "" around name */
		      aceInBack(parse_io) ;
		      cp = aceInPos(parse_io) ;
		      while(*cp != ':') *cp++ = ' ' ; /* garanteed to work */
		      *cp = ' ' ;
		      aceInNext(parse_io) ;
		      cp = aceInWord(parse_io) ;
		    }
		  else
		    *cq = ':' ;
		}
	      if (array(oldClass,ii,int) == -1)
		{ errText = "missing or unrecognized or protected class name" ; 
		  goto abort ;
		}
	      if (!lexIsGoodName(cp))
		{ errText = "missing object name" ; 
		  goto abort ;
		}
	      if (!array(oldClass,ii,int) &&
		  !lexword2key (cp, &xk,array(oldClass,ii,int)))
		{ errText = "Unrecognized Tag name" ; 
		  goto abort ;

		}
	      if (pickList[array(oldClass,ii,int)].known &&
		  !lexword2key (cp, &xk,array(oldClass,ii,int)))
		{ errText = "Unrecognized Tag name" ; 
		  goto abort ;
		}
	      lexaddkey(cp, &xk, array(oldClass,ii,int)) ;
	      tableKey(tt,nn, ii) = xk ;
	      break ;
	    case 'b':
	      cp = aceInWord(parse_io) ;
	      if (!cp) 
		{
		  if (ii) 
		    { errText = "missing bool data" ; 
		      goto abort ;
		    }
		  else 
		    return tt ;
		}
	      if (strcasecmp(cp,"true") == 0)
		tableBool(tt, nn,ii) = TRUE ;
	      else if (strcasecmp(cp,"false") == 0)
		tableBool(tt, nn,ii) = FALSE ;
	      else if (strcasecmp(cp,"NULL") == 0)
		tableBool(tt, nn,ii) = NON_BOOL ;
	      else
		{ errText = "Wrong Bool value, should be TRUE or FALSE" ;
                  goto abort ;
		}
	      break ;
	    case 'i':
	      if (aceInInt(parse_io, &xi)) 
		tableInt(tt,nn, ii) = xi ;
	      else if ((cp = aceInWord(parse_io)) && 
		       strcasecmp(cp,"NULL") == 0)
		tableInt(tt,nn, ii) = NON_INT ;
	      else
		if (ii || cp) 
		  { errText = "wrong or missing  Int value" ;
		    goto abort ;
		  }
		else 
		  return tt ;
	      break ;
	    case 'f':
	      if (aceInFloat (parse_io, &xf)) 
		tableFloat(tt,nn, ii) = xf ;
	      else if ((cp = aceInWord(parse_io)) &&
		       strcasecmp(cp,"NULL") == 0)
		tableFloat(tt,nn, ii) = NON_FLOAT ;
	      else
		if (ii || cp) 
		  { errText = "wrong or missing Float value" ;
		    goto abort ;
		  }
		else 
		  return tt ;
	      break ;
	    case 's':
	      cp = aceInWord(parse_io) ;
	      if (!cp) 
		{
		  if (ii) 
		    { errText = "Missing text" ;
		      goto abort ;
		    }
		  else 
		    return tt ;
		}
	      if (strcasecmp(cp,"NULL") == 0)
		tableDate(tt,nn,ii) = 0 ;
	      else
		tableSetString (tt, nn, ii, cp) ;
	      break ;
	    case 't':
	      cp = aceInWord(parse_io) ;
	      if (!cp) 
		{
		  if (ii) 
		    { errText = "missing date" ;
		      goto abort ;
		    }
		  else 
		    return tt ;
		}
	      if (strcasecmp(cp,"NULL") == 0)
		tableDate(tt,nn,ii) = 0 ;
	      else if ((xt = timeParse(cp)))
		tableDate (tt,nn, ii) = xt ;
	      else
		{ errText = "wrong Date value" ;
		  goto abort ;
		}
	      break ;
	    }
	}
    }
  return tt ;

abort: 
  *err_text = messprintf("  TableResult parse error at line %7d column %d in %.25s : %s\n", 
			 aceInStreamLine(parse_io) -1, ii + 1, table_name, errText) ;
  tableDestroy(tt) ;

  return NULL ;
} /* tableDoDoParse */


/* Currently this is only called from within this module or from xclient.c   */
/* in wrpc and wsocket. The interface is a hack, it can end up returning     */
/* NULL + errtext which means an error                                       */
/* NULL + no errtext, which means there was no data in parse_io              */
/* or a valid tt.                                                            */
/* Need to know about the "no data" for parse result reporting, see func     */
/* tableResultParse()                                                        */
TABLE *tableCreateFromAceIn(ACEIN parse_io, char *table_name, char **errtext) 
{
  TABLE *tt = NULL ;
  char *format_type ;
  Array oldClass = arrayCreate(12, int) ;

  format_type = tableParseFormat(parse_io, oldClass) ;
  if (format_type && strlen(format_type) > 0)
    tt = tableDoDoParse(parse_io, table_name, format_type, oldClass, errtext);
  arrayDestroy (oldClass) ;

  return tt ;
} /* tableCreateFromAceIn */


ParseFuncReturn tableResultParse(ACEIN parse_io, KEY key, char **errtext) 
     /* ParseFuncType */
{
  TABLE *tt ;
  ParseFuncReturn result ;

  tt = tableCreateFromAceIn (parse_io, name(key), errtext) ;

  if (tt)
    { 
      tableStore (key, tt) ; 
      tableDestroy (tt) ;
      result = PARSEFUNC_OK ;
    }
  else if (*errtext == NULL)
    result = PARSEFUNC_EMPTY ;
  else
    result = PARSEFUNC_ERR ;

  return result ;
} /* tableResultParse */

/*************************************************/
/****************** Get/Store   ******************/
/*************************************************/

BOOL tableStore (KEY tKey,  TABLE *t) 
{
  int i, max ;
  Array aa = 0 ;
  TABLETYPE *uu, *vv; 

  if (t->magic != &TABLE_MAGIC)
    messcrash ("tableStore() received non-magic TABLE* pointer");

  if(t->ncol <= 0)
    messcrash ("tableStore() received TABLE with no columns");

  if (!tKey || pickType(tKey) != 'A')
    messcrash ("tableStore() called with a non-valid key") ;

  max = tableMax(t) ;

  aa = arrayCreate ((max+1) * t->ncol, TABLETYPE) ;
  uu = arrayp (aa, (max+1) * t->ncol - 1, TABLETYPE) ; /* make room */

  memcpy (arrp(aa, 0, TABLETYPE), t->name, t->ncol*sizeof(TABLETYPE)) ;

  if (max)
    for (i = 0 ; i < t->ncol ; i++)
      { uu = arrp (aa, t->ncol + i * max, TABLETYPE) ;
	vv = arrp(t->col[i], 0, TABLETYPE)  ; 
	memcpy (uu, vv, tabMax(t,i) * sizeof(TABLETYPE)) ;
      }

  arrayStackStore (tKey, aa, "a", t->s) ; /* crashes or succeeds */

  arrayDestroy (aa) ;

  return TRUE ;
} /* tableStore */

/*************************************************/

TABLE *tableHandleGet (KEY tKey, STORE_HANDLE h)
{
  Stack s = 0 ;
  Array aa = 0 ;
  int i = 0, max, ncol, maxBitArray = 0 ;
  TABLETYPE *uu, *vv ;
  TABLE *t = 0 ;
  char *types ; 
  int  version ;
  char *vs ;

  if (!tKey || pickType(tKey) != 'A')
    messcrash ("tableHandleGet called with a non-valid key") ;

  if (!arrayStackHandleGet (tKey, &aa, "a", &s, h))
    goto abort ;

  if (!aa || !s || !arrayMax(aa) || !stackMark(s))
    messcrash ("missing data in tableHandleGet") ;

  types = stackText (s, 0) ;
  if (!types || !(ncol = strlen(types)))
    goto abort ;
  if (arrayMax(aa) % ncol)
    messcrash ("inconsistent array size in tableHandleGet") ;
  max = arrayMax(aa)/ncol - 1 ; /* 1 for the names */

  version = 0 ;
  if (!stackAtEnd(s) && (vs = stackNextText(s)) &&
      *vs == '\177')
    version = vs[1] ;		/* 256 possible versions */
  if (version > 0)
    { maxBitArray = 1 + (max-1)/(1 + 32) ;
      max -= maxBitArray ;
      if (sizeof(TABLETYPE) != 32)
	messcrash ("sizeof(TABLETYPE) = %d != 32 in tableGet: rethink!",
		   sizeof(TABLETYPE)) ;
    }

  t = tableHandleCreate (max, types, h) ;
  stackDestroy (t->s) ; t->s = s ;
  memcpy (t->name, arrp(aa, 0, TABLETYPE), ncol*sizeof(TABLETYPE)) ;

  if (max)
    for (i = 0 ; i < ncol ; i++)
      { uu = arrp (aa, ncol + i * max, TABLETYPE) ;
	array(t->col[i], max-1, TABLETYPE).i = 0 ; /* make room */
	array(t->parents[i], max-1, KEY) = 0; /* make room */
	array(t->grandParents[i], max-1, KEY) = 0; /* make room */
	vv = arrp(t->col[i], 0, TABLETYPE)  ; 
	memcpy (vv, uu, max*sizeof(TABLETYPE)) ;
      }
  if (version > 0)
    for (i = 0 ; i < ncol ; i++)
      { uu = arrp (aa, ncol*(1+max) + i*maxBitArray , TABLETYPE) ;
	bitUnSet(t->emptyBits[i], max-1) ; /* make room */
	vv = arrp(t->emptyBits[i], 0, TABLETYPE)  ; 
	memcpy (vv, uu, maxBitArray*sizeof(TABLETYPE)) ;
      }
  else
    for (i = 0 ; i < ncol ; i++)
      tableCheckEmpty (t,i) ;
  arrayDestroy (aa) ;
  return t ;  
  
 abort:
  stackDestroy (s) ;
  arrayDestroy (aa) ;
  tableDestroy (t) ;
  return 0 ;
}

/******************************************************************
 ************************ private functions ***********************
 ******************************************************************/

static BOOL tableExists (TABLE *tt)
{
  if (tt && tt->magic == &TABLE_MAGIC && stackExists (tt->s))
    return TRUE;

  return FALSE;
} /* tableExists */

static void tableFinalise (void *block)
{
  TABLE *t = (TABLE*)block;

  if (!tableExists (t))
    messcrash ("tableFinalise() received non-magic TABLE* pointer") ;
  t->magic = 0 ;

  handleDestroy (t->handle) ;

  return;
} /* tableFinalise */

/************************************************************/

static BOOL setColumnVisibility (TABLE *t, int colNum,
				 int on_off) /* TRUE/FALSE to set
					      * other value to enquire */
{
  BOOL old;
  char *type_p;

  if (!t)
    messcrash("setColumnVisibility() - received NULL TABLE* pointer");
  if (colNum < 0 || colNum >= t->ncol)
    messcrash ("setColumnVisibility() - col %d is out of bounds 0-%d",
	       colNum, t->ncol-1) ;

  old = t->visibility[colNum];
 
  if (on_off == 0 || on_off == 1)
    /* set the new value */
    {
      t->visibility[colNum] = on_off;

      /* the type-string for the table is preserved
       * at the first stack position, it is also used to preserve
       * the visibility info for each column, by using upper-case
       * format characters for hidden columns.
       * Here we manipulate the stack to switch those characters,
       * the visibility info gets stored retrieved properly. */
      type_p = stackText(t->s, 0);
      type_p += colNum;
      if (on_off)
	*type_p = ACEIN_LOWER[(int)*type_p];
      else
	*type_p = ACEIN_UPPER[(int)*type_p];
    }

  return old;
} /* setColumnVisibility */

/************************************************************/

static char* tableParseFormat (ACEIN parse_io, Array oldClass)
{
 char *cp ;
 static char type[1000] ;
 int nn, i = 1000 ;
 while(i--) type[i] = 0 ;

 if (!aceInCard(parse_io))
   return 0 ;
 cp = aceInWord(parse_io) ;
 if (!cp || strcasecmp("format", cp) != 0)
   return 0 ;
 i = -1 ;

 while (i < 1000 && (cp = aceInWord(parse_io)))
   { 
     i++ ; 
     array(oldClass,i, int) = 0 ;

     if (strcasecmp(cp,"bool") == 0)
       type[i] = 'b' ;
     else if (strcasecmp(cp,"int") == 0)
       type[i] = 'i' ;
     else if (strcasecmp(cp,"float") == 0)
       type[i] = 'f' ;
     else if (strcasecmp(cp,"text") == 0)
       type[i] = 's' ;
     else if (strcasecmp(cp,"date") == 0)
       type[i] = 't' ;
     else if (strcasecmp(cp,"key") == 0)
       { 
	 type[i] = 'k' ;
	 array(oldClass,i, int) = -1 ;
       }
     else if (strcasecmp(cp,"tag") == 0) /* tag means _VSystem */
       { 
	 type[i] = 'k' ;
	 array(oldClass,i, int) = 0 ;
       }
     else if ((nn = pickWord2Class (cp)))
       { 
	 type[i] = 'k' ;
         array(oldClass,i, int) = nn ;
       }
     else
       return 0 ;
   }
 return type ;
} /* tableParseFormat */
 
/*************************************************/
/****************** end of file ******************/
/*************************************************/ 
 
