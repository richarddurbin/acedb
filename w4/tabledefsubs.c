/*  File: tabledefsubs.c   (formerly known as sprdop.c)
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
 * Description:
 * Exported functions:
 * HISTORY:
 * Last edited: Nov 25 17:58 1999 (fw)
 * * May 27 16:49 1999 (fw): calculation system rewritten, so
 *              results are written directly to TABLE type
 * * Mar 18 10:17 1999 (edgrif): Add proper non-valid int/float constants.
 * Created: Fri Dec 11 13:53:37 1992 (mieg)
 *-------------------------------------------------------------------
 */

/* $Id: tabledefsubs.c,v 1.17 2001-02-01 17:41:43 srk Exp $ */


#include "acedb.h"
#include "aceio.h"
#include "lex.h"
#include "session.h"
#include "pick.h"
#include "whooks/systags.h"
#include "whooks/tags.h"
#include "whooks/sysclass.h"
#include "query.h"
#include "bindex.h"
#include "java.h"
#include "client.h"
#include "spread_.h"

BITSET_MAKE_BITFIELD	  /* define bitField for bitset.h ops */

/************************************************************/

TABLE* (*externalTableMaker) (char *quer) = 0 ; /* allocation */
magic_t SPREAD_MAGIC = "SPREAD_MAGIC";

/************************************************************/

typedef struct SpreadCellStruct *SPREADCELL ;
struct SpreadCellStruct
{
  BOOL empty ;
  KEY key, parent, grandParent ;
  int iCol ;
  COL *col ; 
  SPREADCELL up; /* previous column */
  SPREADCELL scFrom; /* from/rightof: has the correct mark */
  SPREADCELL scGrandParent; /* has the correct obj */
  BSunit u ;
  OBJ obj ;
  BSMARK mark ;
} ;

/*****************************************************/

static SPREAD spreadCreate (void);
static void sortSpreadTable (TABLE *table, int sortCol);
static void spreadDestroyCol (COL* c);
static void spreadDestroyConditions(SPREAD spread);
static void storePrecomputedTable(KEY defKey, 
				  char *internalParams,
				  char *commandLineParams,
				  TABLE *table)  ;
static int scLoopRight (SPREAD spread, SPREADCELL sc) ;
static void initSpreadTable (SPREAD spread);
static SPREADCELL scAlloc (void);
static void scFree (SPREADCELL sc);
static BOOL isParamInString (char *cp);
static char* getSpreadTableTypes (Array colonnes, STORE_HANDLE handle);
static char* getPrecomputationName (KEY tableKey, 
				    char *internalParams,
				    char *commandLineParams,
				    STORE_HANDLE handle);

static Stack scStack = 0 ;

/******************************************************************
 ************************* public functions ***********************
 ******************************************************************/

SPREAD spreadCreateFromFile (char *filename, const char *spec, char *params)
{
  SPREAD spread = NULL;
  ACEIN spread_in;

  if (!filename)
    messcrash ("spreadCreateFromFile() - received NULL filename");

  /* first pass we read without parameter substitution */
  spread_in = aceInCreateFromFile (filename, spec, NULL, 0);
  if (spread_in)
    {
      spread = spreadCreate();
      
      spreadReadDef (spread, spread_in, params, FALSE);
      if (params)
	spread->extraParameters = strnew(params, 0);
      spread->filename = strnew(aceInGetURL(spread_in), 0);

      if (arrayMax (spread->colonnes) == 0)
	/* no column definitions - invalid table-def */
	spreadDestroy (spread);
      
      aceInDestroy (spread_in);
    }

  return spread;
} /* spreadCreateFromFile */

/*****************************************/

SPREAD spreadCreateFromAceIn (ACEIN spread_in, char *params)
{
  SPREAD spread;
  char *filename;

  spread = spreadCreate();
  filename = aceInGetURL(spread_in);
  if (filename && filename[0] == '/')	/* real file-name */
    spread->filename = strnew(filename, 0);

  spreadReadDef (spread, spread_in, params, FALSE);
  if (params)
    spread->extraParameters = strnew(params, 0);

  if (arrayMax (spread->colonnes) == 0)
    /* no column definitions - invalid table-def */
    spreadDestroy (spread);

  return spread;
} /* spreadCreateFromAceIn */

/*****************************************/

SPREAD spreadCreateFromStack (Stack defStack, char *params)
{
  SPREAD spread;
  ACEIN spread_in;

  spread = spreadCreate();

  spread_in = aceInCreateFromText (stackText(defStack,0), NULL, 0);

  spreadReadDef (spread, spread_in, params, FALSE);
  if (params)
    spread->extraParameters = strnew(params, 0);

  if (arrayMax (spread->colonnes) == 0)
    /* no column definitions - invalid table-def */
    spreadDestroy (spread);

  aceInDestroy (spread_in);

  return spread;
} /* spreadCreateFromStack */

/*****************************************/

SPREAD spreadCreateFromObj (KEY tableDefKey, char *params)
{
  SPREAD spread = NULL;
  ACEIN def_io;
  Stack objDefStack;

  objDefStack = getDefinitionFromObj (tableDefKey);
  if (objDefStack)
    {
      spread = spreadCreate();

      /* don't substitute parameters */
      def_io = aceInCreateFromText (stackText(objDefStack,0), NULL, 0);
      
      spreadReadDef (spread, def_io, params, TRUE);
      if (params)
	spread->extraParameters = strnew(params, 0);

      spread->tableDefKey = tableDefKey;

      if (arrayMax (spread->colonnes) == 0)
	/* no column definitions - invalid table-def */
	spreadDestroy (spread);

      aceInDestroy (def_io);
      stackDestroy (objDefStack);
    }

  return spread;
} /* spreadCreateFromObj */

/*****************************************/

void uSpreadDestroy (SPREAD spread)
     /* public function */     
     /* DON'T call directly, use the spreadDestroy() macro */

{    
  if (!spread)
    return;

  if (spread->magic != &SPREAD_MAGIC)
    messcrash ("uSpreadDestroy received corrupt spread->magic");

  if (spread->tableDefKey	/* the defs came from an object */
      && spread->precompute	/* where precomute was switched on */
      && spread->valuesModified	/* which has changed ... */
      && sessionGainWriteAccess())
    /* ... so we store the new precomputation */
    {
      storePrecomputedTable (spread->tableDefKey,
			     spread->paramBuffer,
			     spread->extraParameters,
			     spread->values);
    }

  tableDestroy (spread->values) ;
  spreadDestroyConditions(spread) ;
  spreadDestroyColonnes (spread->colonnes);
  stackDestroy (spread->comments) ;
  messfree (spread->filename);
  messfree (spread->extraParameters);

  spread->magic = 0 ;
  messfree (spread) ;

  return;
} /* uSpreadDestroy */

/*****************************************************/

char *spreadGetTitle (SPREAD spread)
{
  return spread->titleBuffer;
} /* spreadGetTitle */


Array spreadGetFlipInfo (SPREAD spread)
{
  Array isFlipped;
  int i;

  isFlipped = arrayCreate(arrayMax(spread->colonnes), BOOL);
  for (i = 0; i < arrayMax(spread->colonnes); i++)
    {
      if (arrp(spread->colonnes, i, COL)->flipped)
	array(isFlipped, i, BOOL) = TRUE;
    }
  return isFlipped;
} /* spreadGetFlipInfo */

Array spreadGetWidths (SPREAD spread)
{
  Array widthsArray;
  int i;

  widthsArray = arrayCreate(arrayMax(spread->colonnes), int);
  for (i = 0; i < arrayMax(spread->colonnes); i++)
    array(widthsArray, i, int) = arrp(spread->colonnes, i, COL)->width;
  
  return widthsArray;
} /* spreadGetWidths */

/*****************************************************/

KEYSET spreadGetMasterKeyset (SPREAD spread) 
     /* public function */
     /* return the alphabetically ordered 1st column of keys */
{
  KEYSET ksFiltered ;
  KEYSET ksTmp ; 
  COL *masterCol ;

  if (arrayMax(spread->colonnes) == 0)
    {
      messout("First define at least one column");
      return 0;
    }
  
  masterCol = arrp(spread->colonnes, 0, COL) ;
  if (freelower(masterCol->type) != 'k')
    { 
      messout ("Column 1 should be type Object") ;
      return 0 ;
    }
  if (!masterCol->classe)
    { 
      messout ("First choose the class of column 1") ;
      return 0 ;
    }
  
  if (*masterCol->conditionBuffer &&
      ! condCheckSyntax(messprintf(" %s", masterCol->conditionBuffer)))
    {
      messout ("First fix the syntax error in column 1's condition") ;
      return 0 ;
    }


  ksTmp = query (0, messprintf ("FIND %s %s", name(masterCol->classe), masterCol->conditionBuffer));
  
  ksFiltered = keySetAlphaHeap(ksTmp, keySetMax(ksTmp)) ;
  
  keySetDestroy (ksTmp) ;

  return ksFiltered ;
} /* spreadGetMasterKeyset */

TABLE* spreadCalculateOverKeySet (SPREAD spread, KEYSET ks, int *lastp)
     /* public function */
     /* compute the results table, based with the given
      * keyset for the first column
      * this function can be called multiple times on the same keyset,
      * every time a new table is calculated with new results
      * for another part of the 1st column :
      * last - where the calculation got to last time round
      * RETURNS :
      * the position in the master column where we have to continue
      * from next time, or 0 if the calculation finished */
{
  int i, new = 0, mx = keySetMax (ks) ;
  KEY key ;
  BOOL stop ;			/* fw - ?what?does?this?do? */
  int nn = 200 ;
  SPREADCELL sc ;
  COL *col;
  int colNum;
  int last;

  if (lastp)
    last = *lastp;
  else
    last = 0;

  if (last > 0 && spread->sortColonne == 1)
    stop = TRUE;
  else
    stop = FALSE;

  if (externalServer && spread->isActiveKeySet)
    {   /* export active keyset and compute on server side */
      Stack ss = 0 ;
      int n;
      char *cp ;
      ACEOUT dump_out;

      if (!spreadCheckConditions(spread))
	return 0 ;

      /* export active keyset was done by the filter call, 
       * compute on server side */
      ss = stackCreate (100) ;
      dump_out = aceOutCreateToStack (ss, 0);
      spreadDumpColonneDefinitions (dump_out, spread->colonnes) ;
      aceOutDestroy (dump_out);
      n = stackMark(ss) ;
      if (!n)
	{ stackDestroy (ss) ; 
	  return 0 ;
	}
      pushText (ss, "Table -active -a = ") ;
      cp =stackText (ss, 0) ;
      catText (ss, cp) ;
      cp = stackText(ss, n) ;
      spread->values = externalTableMaker (cp) ;
      stackDestroy (ss) ;
      spreadDestroyConditions(spread) ;
      return 0 ;
    }

  if (last == 1) last = 0 ;
  scStack = stackReCreate (scStack, 16) ;

  spread->interrupt = FALSE ;

  /* clean up the text-stacks carried by the columns */
  for (colNum = 0; colNum < arrayMax(spread->colonnes); colNum++)
    {
      col = arrp (spread->colonnes, colNum, COL) ;
      if (col->text) 
	{ 
	  col->text = stackReCreate (col->text, 100) ;
	  pushText (col->text, "toto") ; /* avoid zero */
	}
    }

  /* kill the old value-table */
  tableDestroy (spread->values);

  initSpreadTable (spread);

  messStatus ("TableCalculation, F4 to interrupt...");

  if (last > 0 ||
      spreadCheckConditions(spread)) /* no need to re-check 
				      * if last had been set
				      * by a previous run */
    {
      for (i = last ; i < mx ; i++)
	{
	  new = i + 1 ; /* for encore */
	  key = keySet(ks,i) ;
	  
	  sc = scAlloc () ;
	  sc->key = key ; sc->u.k = key ;
	  sc->parent = key ; 
	  
	  scLoopRight (spread, sc) ; 
	  scFree(sc) ;
	  if (spread->interrupt)
	    break ;
	  if (stop && --nn <= 0) /* will not stop if starts at 0 */
	    break ;
	  new = 0 ; /* remains 0 at end of loop */
	}
    }

  sortSpreadTable (spread->values, spread->sortColonne) ;

  spread->valuesModified = TRUE;

  if (last >= mx)
    new = 0 ;  /* destroy */

  if (!stop || !new)
    spreadDestroyConditions(spread) ;

  if (lastp)
    *lastp = new;

  return spread->values ;
} /* spreadCalculateOverKeySet */



KEYSET spreadFilterKeySet (SPREAD spread, KEYSET ks)
     /* public function */
     /* use the KEYSET ks and apply the condition of the master colonne
      * RETURNS:
      *  a new keyset containing the filtered version of ks */
{
  KEYSET ks1 = 0, ks2 = keySetCreate () ;
  COL *masterCol ;
 
  if (arrayMax(spread->colonnes) == 0)
    messcrash("spreadFilterKeySet() called with empty spread->colonnes");

  masterCol = arrp(spread->colonnes, 0, COL) ;
  if (!masterCol->classe)	/* class of mastercolumn undefined */
    return ks2 ;
  
  if (strlen(masterCol->conditionBuffer) > 0 &&
      ! condCheckSyntax(messprintf(" %s", masterCol->conditionBuffer)))
    return ks2 ;

  ks1 = query (ks, messprintf("CLASS %s", name(masterCol->classe))) ;

  if (strlen(masterCol->conditionBuffer) > 0)
    ks2 = query (ks1, masterCol->conditionBuffer) ;
  else
    { ks2 = ks1 ; ks1 = 0 ; }
  keySetDestroy (ks1) ;

  return ks2 ;
} /* spreadFilterKeySet */

/*****************************************************/

void uSpreadDestroyColonnes (Array colonnes)
     /* private within SpreadPackage */
     /* DON'T call directly, use the spreadDestroyColonnes() macro */
     /* clear all spread->colonnes */
{
  int i;

  if (arrayExists(colonnes))
    {
      i = arrayMax(colonnes);
      while (i--)
	spreadDestroyCol(arrp(colonnes,i,COL)) ; 

      arrayDestroy(colonnes);
    }

  return;
} /* uSpreadDestroyColonnes */


/*****************************************************/

COL *spreadColInit (SPREAD spread, int colNum)
     /* private within SpreadPackage */
     /* returns number of added colonne */
{
  COL *c ;

  c = arrayp(spread->colonnes, colNum, COL) ;  
  c->tagStack = 0 ;
  c->type = 0 ;
  c->realType = 0 ;
  c->colonne = colNum ;
  c->width = 12 ;
  c->flipped = FALSE ;
  c->showType = SHOW_ALL ;
  c->nonLocal = FALSE ;
  c->from = 1;

  c->presence = -1;		/* init with -1 ... */
  c->extend = -1;		/* ... to force change ... */
  c->hidden = -1;		/* ... in the functions below */

  spreadColSetPresence(spread, colNum, VAL_IS_OPTIONAL);
  spreadColSetExtend(spread, colNum, COLEXTEND_FROM);
  spreadColSetHidden(spread, colNum, FALSE);

  memset(c->headerBuffer, 0, 60);
  memset(c->conditionBuffer, 0, 360);
  memset(c->tagTextBuffer, 0, 360);
  

  spread->isModified = TRUE ;

  return c;
} /* spreadColInit */


/********************************/

int spreadRemoveColonne (SPREAD spread, int colNum)
{
  COL *c, *c1 ;
  int i ;

  if (colNum <= 0)
    return 0;		/* cannot remove master-column */

  c = arrayp(spread->colonnes, colNum, COL) ;  

  spreadDestroyCol(c) ;

  for (i = colNum, c = arrayp(spread->colonnes, i, COL);
       i < arrayMax(spread->colonnes) - 1 ; i++)
    {
      c1 = c++ ;
      memcpy(c1, c, sizeof(COL)) ;
    }

  arrayMax(spread->colonnes) -- ;


  for (i = colNum ; i < arrayMax(spread->colonnes) ; i++)
    {
      c = arrayp(spread->colonnes, i, COL) ;  
      
      if (c->from > colNum + 1)
	c->from -- ;
      else if (c->from == colNum + 1)
	messout ("You should modify colonne %d, which is derived from the colonne you just destroyed", i + 2) ;
    }

  spread->isModified = TRUE ;

  return colNum - 1;
} /* spreadRemoveColonne */

/*****************************************************/

TABLE *spreadCalculateOverWholeClass (SPREAD spread)
{
  KEYSET ks;
  int n ;
  char *cp ;

  if (!spread->isModified 
      && spread->precompute
      && spread->precomputeAvailable
      && spread->values)
    /* return the precomputed table */
    {
      printf ("returning existing results\n");
      return spread->values;
    }


  if (!spreadCheckConditions(spread))
    goto abort;

  if (externalServer) /* compute on server side */
    {
      Stack ss = stackCreate (1000);
      ACEOUT dump_out = aceOutCreateToStack (ss, 0);
      spreadDumpColonneDefinitions (dump_out, spread->colonnes) ;
      aceOutDestroy (dump_out);
      n = stackMark(ss) ;
      if (!n)
	{ stackDestroy (ss) ; 
	  return (TABLE*)NULL ;
	}
      pushText (ss, "Table -a = ") ;
      cp =stackText (ss, 0) ;
      catText (ss, cp) ;
      cp = stackText(ss, n) ;
      spread->values = externalTableMaker (cp) ;
      stackDestroy (ss) ;
      return spread->values ;
    }

  ks = spreadGetMasterKeyset (spread) ;
  if (!ks)
    goto abort ;

  /* if we get here we have at least one correct colonne def */
  spreadCalculateOverKeySet (spread, ks, NULL);
  keySetDestroy(ks) ;

  return spread->values;
    
 abort:
  return (TABLE*)NULL;
} /* spreadCalculateOverWholeClass */

/********************************/

BOOL spreadCheckConditions(SPREAD spread)
     /* private within SpreadPackage */
{
  COL *colonne = arrp (spread->colonnes, 0, COL) - 1 ;
  COND cond = 0 ;
  int from, icol = 0 , max = arrayMax (spread->colonnes) ;
  int ctype ;

  while (icol++, colonne++, max--)
    { 
      colonne->tagCondition = colonne->conditionCondition = 0 ;
      
      from =  colonne->from - 1 ;
      if (icol > 1)
	{
	  if (from + 1 >= icol || from < 0)
	    {  
	      messout ("From field %d of column %d is wrong",
		       from + 1, icol) ;
	      return FALSE ;
	    }
	  ctype = arrp (spread->colonnes, from, COL)->type ;
	  if (colonne->extend == COLEXTEND_FROM && 
	      ctype != 'k' && ctype != 'K' && ctype != 'n')
	    { 	
	      messout ("Column %d is computed from column %d which "
		       "is not a list of objects",
		       icol, from + 1) ;
	      return FALSE ;
	    }
	}

      if (colonne->type == 't' && !colonne->text)
	{ colonne->text = stackReCreate(colonne->text, 100) ;
	  pushText(colonne->text, "\177NULL") ; /* so 0 means undefined */
	 }

      if (colonne->tagStack && stackText(colonne->tagStack, 0))
	{ if (!condConstruct (stackText(colonne->tagStack,0), &cond))
	  { messout ("syntax error in Tag field :%s",
		     stackText(colonne->tagStack, 0)) ;
	  return FALSE ;
	  }
	colonne->tagCondition = cond ;
	}
      else
	{
	  if (!colonne->tag)
	    cond = 0 ;
	  else if (!condConstruct (name(colonne->tag), &cond))
	    { messout ("syntax error in Tag field :%s",
		       name(colonne->tag)) ;
	    return FALSE ;
	    }
	  colonne->tagCondition = cond ;
	}
      
      colonne->condHasParam = isParamInString (colonne->conditionBuffer);
      colonne->conditionCondition = 0 ;
      if (!colonne->condHasParam)
	{
	  /****** no parameters *******/
	  if (colonne->conditionBuffer &&
	      *colonne->conditionBuffer)
	    {
	      if (!condConstruct (colonne->conditionBuffer, &cond))
		{ messout ("syntax error in Tag field :%s",
			   colonne->conditionBuffer) ;
		  return FALSE ;
		}
	      else
		colonne->conditionCondition = cond ;
	    }
	}
      else
	{
	  /********** do parameter substitution **********/
	  Stack  s ;
	  int i, i1, mine, n1, n2 ;
	  char cc, *cp = colonne->conditionBuffer, *cq ;
	  
	  /* must check here that substitution is compatible with syntax */
	  cp-- ;
	  while (*++cp) 
	    if (*cp == '%')
	      {
		cc = *(cp + 1) ;
		if (cc == '%')
		  { messout("Give a value to %% before running in column %d at:\n%s",
			    icol , cp) ;
		  return FALSE ;
		  }
		cq = cp + 1; i = 0 ; i1 = 1 ;
		while (*cq >= '0' && *cq <= '9')
		  { i *= 10 ; i += (*cq - '0') ; cq++ ; }
		if (i <=0 || i > icol)
		  { messout("Cannot substitute parameter in column %d at:\n%s",
			    icol, cp) ;
		  return FALSE ;
		  }
	      }
	  
	  s = stackCreate(500) ;
	  pushText (s, colonne->conditionBuffer) ;
	  n1 = stackMark(s) ; pushText (s, " ") ;
	  for (i = 0 ; i < icol ; i++)
	    { /* use same value as in scMatch */
	      switch (arrp (spread->colonnes, i, COL)->type)
		{
		case 'b':  case 'k': case'n': case 'K': case't':
		  catText (s," __toto__ ") ;
		  break ;
		case 'i': case 'f': case 'c':
		  catText (s," 1 ") ;
		  break ;
		case 'd':
		  catText (s," now ") ;
		  break ;
		}
	    }
	  mine = freesettext (stackText(s, 0), stackText(s,n1)) ;
	  freespecial (";\n\t/%\\") ;    /* No subshells ($ removed) */
	  n2 = stackMark(s) ;
	  while (freecard(mine))
	    pushText (s, freepos()) ;
	  if (!condCheckSyntax(stackText(s,n2)))
	    {
	      messout(
		      "After test substitution, I cannot parse condition of col %d :\n%s",
		      icol, stackText(s,n2)) ;
	      stackDestroy (s) ;
	      return FALSE ;
	    }
	  stackDestroy (s) ;
	  colonne->conditionCondition = 0 ; /* do not register in this case */
	}
    }

  return TRUE ;
} /* spreadCheckConditions */
  


BOOL spreadGetPrecomputation (SPREAD spread)
     /* find the stored table results for the tableDefinition object
      * and return the precomuted table.
      * no definition have to be loaded into 'spread' at this point.
      *
      * PARAMETERS:
      * spread - the table-maker definitions - 
      *          IN:  ->tableDefKey needs to be set
      *          OUT: ->values
      *               ->tableResultKey
      *               ->precomputeAvailable
      *                   may be initialised
      *
      * RETURNS :
      *   TRUE - if spread->tableDefKey is a valid object
      *          which has a 'params'-based precomputation stored
      *   FALSE - no precomputed table could be retrieved
      */
{	
  KEY resultKey = spread->tableResultKey;
  TABLE *table = 0;
  char *resultName = 0;

  resultName = getPrecomputationName(spread->tableDefKey,
				     spread->paramBuffer,
				     spread->extraParameters,
				     0);
  if (!resultName)
    goto unavailable;		/* tableDefKey may be invalid */

  lexword2key(resultName, &resultKey, _VTableResult) ;
  messfree(resultName);

  if (!resultKey)
    goto unavailable;

  table = tableGet(resultKey) ;

  if (!table)
    goto unavailable;

  messout ("Load precompute from TableResult : \"%s\"\n",
	   name(resultKey));

  spread->precomputeAvailable = TRUE;
  spread->tableResultKey = resultKey;

  if (spread->values)
    tableDestroy(spread->values);
  spread->values = table;

  return TRUE;

 unavailable:
  spread->precomputeAvailable = FALSE;
  return FALSE;
} /* spreadGetPrecomputation */




/******************************************************************
 ************************ private functions ***********************
 ******************************************************************/

static SPREAD spreadCreate (void)
{
  SPREAD spread ;

  spread = (SPREAD) messalloc(sizeof(struct SpreadStruct));

  spread->magic = &SPREAD_MAGIC; 
  spread->colonnes = arrayCreate(6, COL) ;

  spread->isModified = FALSE;	/* definitions changed yet */
  spread->valuesModified = FALSE; /* results haven't changed yet */

  /* we don't know yet were the definitions will be loaded from */
  spread->tableDefKey = 0;
  spread->filename = NULL;

  spread->extraParameters = NULL;

  return spread ;
} /* spreadCreate */

/*****************************************/

static void spreadDestroyCol (COL* c)
     /* clear just the specified colonne */
{
  stackDestroy(c->text) ;
  stackDestroy(c->tagStack) ;

  return;
} /* spreadDestroyCol */

/********************************/

static void spreadDestroyConditions(SPREAD spread)
{
  COL *colonne = arrp (spread->colonnes, 0, COL) - 1 ;
  int max = arrayMax (spread->colonnes)  ;
  while (colonne++, max--)
    { condDestroy (colonne->tagCondition) ;
      condDestroy (colonne->conditionCondition) ;
      colonne->tagCondition = colonne->conditionCondition = 0 ;
    }

  return;
} /* spreadDestroyConditions */

/********************************/

static void sortSpreadTable (TABLE *table, int sortCol)
     /* sort the results-table according to the sortCol */
{
  char *sortSpec = (char*)messalloc((table->ncol*2)+1);
  int i, n;

  if (sortCol == 0)
    {
      /* sort left to right */
      for (i = 0; i < table->ncol; i++)
	{
	  sortSpec[i*2] = '+';
	  sortSpec[i*2+1] = '1'+i;
	}
    }
  else
    {
      /* sortColonne has first sort priority, rest of columns in 
       * left-right order - all ascending */
      sortSpec[0] = '+';
      sortSpec[1] = '1'+(sortCol-1);
      n = 1;
      for (i = 0; i < table->ncol; i++)
	{
	  if (i == (sortCol-1))
	    continue;
	  sortSpec[n*2] = '+';
	  sortSpec[n*2+1] = '1'+i;
	  n++;
	}
    }

  tableSort(table, sortSpec);

  tableMakeUnique(table);

  messfree(sortSpec);

  return;
} /* sortSpreadTable */


/**************************************************************/

         /* self managed calloc */

static Stack freeScStack = 0 ;
static int nScUsed = 0 , nScAlloc = 0 ;    /* useful for debug */

static SPREADCELL scAlloc (void)       /* self managed calloc */
{
  static int blocSize = 256 ; /* i need no more SPREADCELL than columns */
  SPREADCELL p = 0 ;
  int i ;

  if (!freeScStack)
    { nScUsed = nScAlloc = 0 ;
      blocSize = 256 ; 
      freeScStack = stackCreate (sizeof(SPREADCELL)*blocSize) ;
    }
  if (stackEmpty (freeScStack))
    { p = (SPREADCELL) messalloc (blocSize * sizeof (struct SpreadCellStruct)) ;
      for (i = blocSize ; i-- ; ++p)
        push (freeScStack,p,SPREADCELL) ;
      nScAlloc += blocSize ;
      blocSize *= 2 ;
    }
  p = pop (freeScStack, SPREADCELL) ;
  memset (p, 0, sizeof (struct SpreadCellStruct)) ;
  nScUsed ++ ;

  return p ;
} /* scAlloc */

/********************************/

static void scFree (SPREADCELL sc)
{
  if (sc && freeScStack)
    { 
      if (sc->obj)
	bsDestroy (sc->obj) ;
      if (sc->mark) messfree (sc->mark) ;
      push (freeScStack, sc, SPREADCELL) ;
      nScUsed-- ;
    }
  else
    invokeDebugger() ;

  return;
} /* scFree */

/*****************************************************/

static BOOL isParamInString (char *cp)
{
  if (cp)		/* find '%' char in the condition-string */
    while (*cp)
      {
	if (*cp++ == '%')
	  return TRUE ;
      }
  return FALSE ;
} /* isParamInString */

/**********************************************************************/
/************************ Actual calculation **************************/
/**********************************************************************/

static BOOL scMatch (SPREADCELL sc, OBJ *objp, KEY key)
{  
  static Stack s = 0 ;
  SPREADCELL cc ;
  int mine, n0, n1, n2 ;
  BOOL found = FALSE ;
  char *cp = 0 ;

 switch (sc->col->type)
    {
    case 'i': case 'c':
      cp = strnew(messprintf("%d", sc->u.i), 0) ; key = _Text ;
      break ;
    case 'f':
      cp = strnew(messprintf("%g", sc->u.f), 0)  ; key = _Text ;
      break ;
    case 'd':
      cp = strnew(messprintf("\"%s\"", timeShow(sc->u.time)), 0) ; key = _Text ;
      break ;
    case 't':
      cp = strnew(stackText(sc->col->text, sc->u.i), 0) ; key = _Text ;
      break ;
    default:
      break ;
    }

  s = stackReCreate(s, 256) ;
  /* prepare the text of the query with param substituted */
  n0 = stackMark(s) ;
  pushText(s, sc->col->conditionBuffer) ;
  n1 = stackMark(s) ; pushText (s, " ") ; /* push now, so I can then cat */

  stackClear (scStack) ;
  cc = sc ;
  do
    { push (scStack, cc, SPREADCELL) ;
    } while ((cc = cc->up)) ;

  while (!stackEmpty(scStack))
    { 
      catText(s, " ") ;
      cc = pop(scStack, SPREADCELL) ; 
      if (!cc->iCol)
	{ catText(s, freeprotect (name(cc->u.k))) ;
	  continue ; 
	}
      if (cc->empty) /* use same value as in checkSyntax */
	switch (cc->col->type)
	  {
	  case 'i': case 'f': case 'c':
	    catText (s," 1 ") ;
	    break ;
	  case 'd':
	    catText (s," now ") ;
	    break ;
	  case 'b':  case 'k': case'n': case 'K': case't':
	  default:
	    catText (s," __toto__ ") ;
	    break ;
	  }
      else
	switch (cc->col->type)
	  {
	  case 'i': case 'c':
	    catText (s,messprintf("%d", cc->u.i) ) ;
	    break ;
	  case 'f':
	    catText (s,messprintf("%g", cc->u.f) ) ;
	    break ;
	  case 'd':
	    catText (s,messprintf("\"%s\"", timeShow(cc->u.time))) ;
	    break ;
	  case't':
	    catText(s, freeprotect (stackText(cc->col->text, cc->u.i))) ;
	    break ;
	  case 'b':  case 'k': case'n': case 'K':
	  default:
	    catText(s, freeprotect (name(cc->u.k))) ;
	    break ;
	  }

    }
  
  /* rely on freesubs to actually substitute the param in the query */
  mine = freesettext (stackText(s, n0), stackText(s,n1)) ;
  freespecial (";\n\t/%\\") ;    /* No subshells ($ removed) */
  n2 = stackMark(s) ;
  while (freecard(mine))
    pushText (s, freepos()) ;

   /* evaluate the query */
  if (condCheckSyntax(stackText(s,n2)) &&
      queryFind21(objp,key,stackText(s,n2), cp))
    found = TRUE ;
  messfree (cp) ;

  return found ;
} /* scMatch */

/*****************************************************/

static BOOL scQuery (SPREAD spread, SPREADCELL s2, SPREADCELL sc)
{
  KEY k = 0, direction = _bsRight ;
  OBJ obj = s2->obj ;
  int nn = 0 , xi, xxi = 0, xxi2 = 0 ;
  float xf = 0, xxf = 0, xxf2 = 0  ;
  mytime_t xtm , xxtm = 0 ;
  char *xt ;

  if (!sc->iCol)    /* accept column one */
    return TRUE ;
  if (sc->mark) 
    { 
      if (!obj) messcrash("Error 1 in scQuery, sorry") ;
      bsGoto (obj, sc->mark) ;
      direction = _bsDown ;
    }
  else
    { 
      if (sc->col->extend == COLEXTEND_RIGHT_OF)  /* start inside obj */
	{
	  if (!obj && sc->scFrom->mark) messcrash("Error 1 in scQuery, sorry") ;
	  if (!obj && !(obj = s2->obj = bsCreate (sc->scGrandParent->key)))
	    return FALSE ;
	  bsGoto (obj, sc->scFrom->mark) ;
	}
      else                  /* start at root */
	{
	  if (obj)
	    bsGoto (obj, 0) ;
	}
      if (!obj && sc->col->tag && !bIndexFind(s2->key, sc->col->tag))
	return FALSE ;
      if (!queryFind3(sc->col->tagCondition, &(s2->obj), sc->scGrandParent->key))
	return FALSE ;  
      obj = s2->obj ; /* in case we just created obj */
      if (!obj && !(obj = s2->obj = bsCreate (sc->scGrandParent->key)))
	return FALSE ; /* if queryFind3 did not create the object */
      direction = _bsRight ;
    }
     
  /* do not iterate inside a column on tags or on arithmetic evaluators */
  if (sc->mark && 
      (sc->col->type == 'b' || sc->col->showType != SHOW_ALL))
    return FALSE ;

  while (TRUE)
    {
      switch (sc->col->type)
	{
	case 'b':
	  if (sc->mark ||
	      !bsGetKeyTags (obj, _bsHere, &k))
	    return nn ? TRUE : FALSE ; 
	  direction = _bsDown ;
	  while (k &&
		 ((class(k) == _VComment) || (class(k) == _VUserSession))
		 )
	    { k = 0 ; bsGetKeyTags (obj, _bsHere, &k) ; }
          if (!k)   
	    return nn ? TRUE : FALSE ; 
	  sc->u.k = sc->key = k ;
	  break ;
	case 'c': /* count */
	  if (sc->mark ||
	      !bsGetKeyTags (obj, direction, &k))
	    return FALSE ;     
	  direction = _bsDown ; nn = 0 ;
	  do
	    {
	      if (class(k) != _VComment &&
		  class(k) != _VUserSession) nn++ ;
	    } while (bsGetKeyTags (obj, direction, &k)) ;
	  if (!nn) return FALSE ;
	  sc->u.i = nn ;
	  goto done ;
	  break ;
	case 'k': case'n': case 'K': 
	  if (!bsGetKeyTags (obj, direction, &k))
	    return nn ? TRUE : FALSE ;
	  direction = _bsDown ;
	  while (k &&
		 ((class(k) == _VComment) || (class(k) == _VUserSession))
		 )
	    { k = 0 ; bsGetKeyTags (obj, _bsHere, &k) ; }
          if (!k)   
	    return nn ? TRUE : FALSE ; 
	  sc->u.k = sc->key = k ;
	  break ;
	case 'i':
	  if (!bsGetData (obj, direction, _Int, &xi))
	    return nn ? TRUE : FALSE ;
	  sc->u.i = xi ;
	  break ;
	case 'f':
	  if (!bsGetData (obj, direction, _Float, &xf))
	    return nn ? TRUE : FALSE ;
	  sc->u.f = xf ;
	  break ;
	case 'd':
	  if (!bsGetData (obj, direction, _DateType, &xtm))
	    return nn ? TRUE : FALSE ;
	  sc->u.time = xtm ;
	  break ;
	case 't':
	{
	  /* this is a hack, but it seems that a Text is preceded by a cell
of type Text but with no text inside it, at least sometimes */
	encore:
	  if (!bsGetData (obj, direction, _Text, &xt))
	    return nn ? TRUE : FALSE ;
	  if (!xt || !*xt)
	    { direction = _bsDown ; goto encore ; }
	}
	  sc->u.i = stackMark(sc->col->text) ;
	  pushText (sc->col->text, xt) ;
	  break ;
	}
      /* SHOW_ALL=0, SHOW_MIN, SHOW_MAX, SHOW_AVG */
      switch (sc->col->showType)
	{
	case SHOW_ALL:
	  goto done ;
	case SHOW_MIN:
	  switch (sc->col->type)
	    {
	    case 'i':
	      if (!nn || xxi > xi) xxi = xi ;
	      sc->u.i = xxi ; /* reset */
	      break ;
	    case 'f':
	      if (!nn || xxf > xf) xxf = xf ;
	      sc->u.f = xxf ;
	      break ;
	    case 'd':
	      if (!nn || xxtm > xtm) xxtm = xtm ;
	      sc->u.time = xxtm ;
	      break ;
	    default: /* undefined on other types */
	      return FALSE ;
	    }
	  break ;
	case SHOW_MAX:
	  switch (sc->col->type)
	    {
	    case 'i':
	      if (!nn || xxi < xi) xxi = xi ;
	      sc->u.i = xxi ; /* reset */
	      break ;
	    case 'f':
	      if (!nn || xxf < xf) xxf = xf ;
	      sc->u.f = xxf ;
	      break ;
	    case 'd':
	      if (!nn || xxtm < xtm) xxtm = xtm ;
	      sc->u.time = xxtm ;
	      break ;
	    default: /* undefined on other types */
	      return FALSE ;
	    }
	  break ;
	case SHOW_AVG:
	  switch (sc->col->type)
	    {
	    case 'i':
	      if (!nn) xxi = 0 ;
	      xxi += xi ;
	      sc->u.i = xxi/(nn + 1) ; /* reset */
	      break ;
	    case 'f':
	      if (!nn) xxf = 0 ;
	      xxf += xf ;
	      sc->u.f = xxf/(nn + 1) ;
	      break ;
	    default: /* undefined on other types */
	      return FALSE ;
	    }
	  break ;
	case SHOW_VAR:
	  switch (sc->col->type)
	    {
	    case 'i':
	      if (!nn) { xxi = 0 ; xxi2 = 0 ; sc->u.i = 0 ; }
	      xxi += xi ; xxi2 += xi * xi ;
	      if (nn > 1)
		sc->u.i = (xxi2  -(xxi*xxi/((float)nn)))/(nn - 1.0); /* reset */
	      break ;
	    case 'f':
	      if (!nn) { xxf = 0 ; xxf2 = 0 ; sc->u.f = 0 ; }
	      xxf += xf ; xxf2 += xf * xf ;
	      if (nn > 1)
		sc->u.f = (xxf2 - xxf*xxf/nn)/(nn - 1.0) ;
	      break ;
	    default: /* undefined on other types */
	      return FALSE ;
	    }
	  break ;
	case SHOW_SUM:
	  switch (sc->col->type)
	    {
	    case 'i':
	      if (!nn) xxi = 0 ;
	      xxi += xi ;
	      sc->u.i = xxi ; /* reset */
	      break ;
	    case 'f':
	      if (!nn) xxf = 0 ;
	      xxf += xf ;
	      sc->u.f = xxf ;
	      break ;
	    default: /* undefined on other types */
	      return FALSE ;
	    }
	  break ;
	default:
          break ;
	}
      nn++ ;
      direction = _bsDown ;
    }
done:
  sc->mark = bsMark (obj, sc->mark) ;
  sc->parent = bsParentKey(obj) ;

  return TRUE ;
} /* scQuery */

/*****************************************************/

static BOOL scCondition (SPREAD spread, SPREADCELL sc)
{
  BOOL found = FALSE;
  BOOL isCond = *sc->col->conditionBuffer ? TRUE : FALSE;
  BOOL hasParam = sc->col->condHasParam;
  OBJ obj = 0 ;
  KEY key ;
  char *cp = 0 ;

  if (!isCond) return TRUE ;
  switch (sc->col->type)
    {
    case 'b': 
      return TRUE ;
      break ;
    case 'n': case 'k': 
      if (!lexIsInClass(sc->key, sc->col->classe))
	return FALSE ;
      key = sc->key ;
      break ;
    case 'K':  /* no class check in case K */
      key = sc->key ;
      break ;
    case 'i': case 'c':
       cp = strnew(messprintf("%d", sc->u.i),0 ) ; key = _Text ;
      break ;
    case 'f':
       cp = strnew(messprintf("%g", sc->u.f),0 ) ; key = _Text ;
      break ;
    case 'd':
      cp = strnew(messprintf("\"%s\"", timeShow(sc->u.time)),0) ; key = _Text ;
      break ;
    case 't':
      cp = strnew(stackText(sc->col->text, sc->u.i),0) ; key = _Text ;
      break ;
    default:
      key = _Text ;
      break ;
    }

  if (hasParam)
    found = scMatch (sc, &obj, key) ;
  else
    found = sc->col->conditionCondition && 
      queryFind31(sc->col->conditionCondition, &obj, key, cp) ;
  messfree (cp) ;
  bsDestroy (obj) ;

  return found ;
} /* scCondition */

/*****************************************************/

static int scLoopDown (SPREAD spread, SPREADCELL s2, SPREADCELL sc)
{ 
  int n = 0;
  SpPresenceType mand = sc->col->presence ;
  
  if (!(sc->scFrom && sc->scFrom->empty) &&
      !(sc->scFrom && sc->scFrom->col && sc->scFrom->col->nonLocal))
    while (TRUE) /* loop on the whole column */
    {
      if (messIsInterruptCalled())
	{ 
	  spread->interrupt = TRUE ;
	  return n ;
	}

      if (!scQuery (spread, s2, sc))
	break ;  /* no more target */
      if (mand == VAL_MUST_BE_NULL)
	return 1 ; /* do not export */
      if (scCondition(spread, sc))  /* target acceptable */
	n += scLoopRight (spread, sc) ; /* expand and export */
      if (sc->col->showType != SHOW_ALL) 
	break ;
    }

  if (n == 0 && mand != VAL_IS_REQUIRED) /* null or optional */
   {
     sc->empty = TRUE ;
     n = scLoopRight (spread, sc) ; /* explore once */
   }
  return n ;  
} /* scLoopDown */
  
/*****************************************************/

static void scExport (SPREAD spread, SPREADCELL sc)
     /* finished to calclulate the row for one value in the
      * first column - now add the resulting row of values
      * to the results table */
{ 
  int i = 0;
  COL *colonne ;

  stackClear (scStack) ;

  for (i = sc->iCol + 1, colonne = arrp (spread->colonnes, sc->iCol, COL) + 1 ;
       i < arrayMax(spread->colonnes) ; i++, colonne++) 
    if (colonne->presence == VAL_IS_REQUIRED)
      return ; /* missing non optional column, do not export */


  {
    COL *colonne;
    SPREADCELL cc ;
    int rowNum, colNum;

    rowNum = tableMax(spread->values);
    colNum = spread->values->ncol - 1;
    cc = sc;
    do {
      colonne = cc->col;
      if (!colonne)
	/* master-column has cc->col == NULL */
	colonne = arrayp (spread->colonnes, 0, COL);

      if (colonne && colonne->type && !colonne->hidden)
	{
	  if (cc->empty)
	    tableSetEmpty(spread->values, rowNum, colNum);
	  else
	    switch(spread->values->type[colNum])
	      {
	      case '0':
		tableSetEmpty(spread->values, rowNum, colNum); break;
	      case 'i':
		tableInt (spread->values, rowNum, colNum) = cc->u.i; break;
	      case 'f':
		tableFloat (spread->values, rowNum, colNum) = cc->u.f; break;
	      case 'k':
		tableKey (spread->values, rowNum, colNum) = cc->u.k; break;
	      case 't':
		tableDate (spread->values, rowNum, colNum) = cc->u.time; break;
	      case 's':
		if (stackExists(cc->col->text))
		  tableSetString (spread->values, rowNum, colNum, stackText(cc->col->text, cc->u.i));
		else
		  tableSetEmpty (spread->values, rowNum, colNum);
		break;
	      default:
		messcrash("scExport() - unkown table->type");
	      }
	  
	  
	  tableParent(spread->values, rowNum, colNum) = cc->parent;
	  tableGrandParent(spread->values, rowNum, colNum) = cc->scFrom ?  cc->scFrom->key : cc->key ;
	  
	  colNum--;
	}
    } while ((cc = cc->up));
  }

  return;
} /* scExport */

/*****************************************************/

static int scLoopRight (SPREAD spread, SPREADCELL sc)
{ 
  int n = 0, from ;
  SPREADCELL sr = 0, s2, srFrom = 0 ;
  
  if(!spread->interrupt &&
     (sc->iCol == 0 || (sc->col &&  sc->col->type)) &&
     sc->iCol + 1 < arrayMax(spread->colonnes))
    { 
      /* go to the right */
      sr = scAlloc () ;
      sr->up = sc ; 
      sr->iCol = sc->iCol + 1 ;
      sr->col = arrp (spread->colonnes, sr->iCol, COL) ;
      srFrom = sr ;
      from = sr->col->from - 1 ;
      while (srFrom->iCol != from && srFrom->up)
	srFrom = srFrom->up ;

      if (srFrom == sr || srFrom->iCol != from)
	{ messerror ("Confusion 1 in scLoopRight") ; return 1 ; }
      sr->scFrom = srFrom ;
      s2 = sr ;
      while (s2 && s2->col && s2->col->extend == COLEXTEND_RIGHT_OF) 
	s2 = s2->scFrom ;  
      if (s2 && s2->col)
	s2 = s2->scFrom ; 
      sr->scGrandParent = s2 ;

      if (sr->col && sr->col->type)
	n = scLoopDown (spread, s2, sr) ;

      scFree (sr) ;
    }
  if (n == 0) /* then export self */
    {
      n = 1 ;
      scExport (spread, sc) ;
    }

  bsDestroy (sc->obj) ; /* release memory NOW */

  return n ;
} /* scLoopRight */

/********************************/

static void initSpreadTable (SPREAD spread)
{
  COL *col;
  int colNum, i;
  char *typeString;
  
  typeString = getSpreadTableTypes(spread->colonnes, 0);

  spread->values = tableCreate(50, typeString);

  messfree (typeString);

  i = 0;
  for (colNum = 0; colNum < arrayMax(spread->colonnes); colNum++)
    {
      col = arrp(spread->colonnes, colNum, COL);

      if (!col->type) 
	continue;

      if (col->hidden)
	continue;

      if (col->headerBuffer == NULL)
	messcrash("initSpreadTable() - col->headerBuffer == NULL");
      tableSetColumnName(spread->values, i, col->headerBuffer);

      i++;
     } 

  return;
} /* initSpreadTable */

/******************************************************/

static char* getSpreadTableTypes (Array colonnes, STORE_HANDLE handle)
{
  int i, colNum, colMax = arrayMax(colonnes) ;
  COL *col ;
  char *types = (char*)halloc (colMax + 1, handle) ;

  i = 0;
  for(colNum = 0 ; colNum < colMax; colNum++)
    { 
      col = arrp(colonnes, colNum, COL) ;
      if (!col->hidden)
	switch (col->type)
	  {
	  case 'c': 
	  case 'i':
	    types[i++] = 'i' ; break ;
	    
	  case 'f':
	    types[i++] = 'f' ; break ;
	    
	  case 'd':
	    types[i++] = 't' ; break ; /* dates */
	    
	  case 'k':
	  case 'K':
	  case 'n':
	  case 'b':
	    types[i++] = 'k' ; break ;
	    
	  case 't':
	    types[i++] = 's' ; break ; /* text */
	    
	  case 0:
	    /* type undefined (colonne definition incomplete)
	     * -> no column in result-table here */
	    break;
	    
	  default:
	    messcrash("getSpreadTableTypes() - invalid col->type");
	    break;
	  }
    }
  types[i] = 0 ;

  if (i == 0)
    /* this means that there is no colonne with a valid type
     * this can happen is all column are hidden.
     * Since the table package barfs on zero-column tables
     * we fake something up. */
    {
      types[0] = 't';
      types[1] = 0;
    }
    

  return types ;
} /* getSpreadTableTypes */


/*****************/

static char* getPrecomputationName (KEY tableKey, 
				    char *internalParams,
				    char *commandLineParams,
				    STORE_HANDLE handle)
{
  char *cp;
  char *fullname = 0;

  if ((class(tableKey) == _VTable))
    {
      Stack s = stackCreate(100);
      ACEIN param_in;
      int mark;

      pushText(s, name(tableKey));
      catText (s, "#");

      /* construct the full name of the TableResult object where the
       * result-table is stored. This name incorporates the parameters */
      param_in = aceInCreateFromText
	(messprintf("%s %s",
		    internalParams ? internalParams : "",
		    commandLineParams ? commandLineParams : ""),
	 NULL, 0);
      aceInSpecial (param_in, "");
      aceInCard (param_in);

      mark = stackMark(s);
      while ((cp = aceInWord(param_in)))
	{ 
	  if (strncmp(stackText(s, mark), "#", 1) != 0)
	    catText(s, "#");
	  catText(s, cp);
	  mark = stackMark(s);
	}
      aceInDestroy (param_in);

      fullname = strnew(stackText(s, 0), handle);

      stackDestroy (s);
      /*
      fullname = halloc ((strlen(name(tableKey)) +
			  (params ? strlen(params) : 0) + 8), handle);
      
      cp = fullname ; cq = name(tableKey) ;
      while ((*cp++ = *cq++)) ; 
      if (params) { cp-- ; *cp++ = '#' ;}
      cq = params ? params : "" ; 
      while (*cq == ' ' && *cq) cq++ ;
      while(*cq)
	{
	  if (*cq == ' ') { *cp++ = '#' ; while (*cq == ' ') cq++ ; }
	  *cp++ = *cq++ ;
	}
      */
    }
  
  return fullname;
} /* getPrecomputationName */

/******************************************************/

static void storePrecomputedTable(KEY defKey, 
				  char *internalParams,
				  char *commandLineParams,
				  TABLE *table)
{
  OBJ obj = 0 ;
  KEY _Precompute ;
  KEY resultKey;
  char *resultName;


  if (!defKey ||	
      (class(defKey) != _VTable))
    /* can only store TableResult, if the definitions are 
     * stored in a Table-object */
    return;

  if (!table)			/* has to have a results table */
    return;

  if (tableMax(table) == 0)
    return;			/* don't store empty results-tables */

  /* init the TableResult object which will store the precomputed table */
  resultName = getPrecomputationName (defKey, 
				      internalParams, 
				      commandLineParams, 0);
  lexaddkey(resultName, &resultKey, _VTableResult) ;
  messfree(resultName);

  /* add a tag to the definition object to remember 
   * where the precomputed table was stored */
  if (!(obj = bsUpdate(defKey)))
    return ;

  messout ("Store precompute in TableResult : \"%s\"\n",
	   name(resultKey));

  lexaddkey("Precompute", &_Precompute, 0) ;
  bsAddKey (obj, _Precompute, resultKey) ; 
  bsSave (obj) ;

  /* write the table-data to the TableResult-object */
  tableStore (resultKey, table) ;

  return;
} /* storePrecomputedTable */



/******************** eof **********************************/
 
