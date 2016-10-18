/*  File: tablemakerdisp.c
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
 * Description: Table_Maker - graphical editor to create and edit
 *              table-definitions
 * Exported functions:
 *              
 *              tableDisplay - DisplayFunc for table-definition objects
 *              
 *              tableMaker - top-level menu-func to start Table_Maker
 *				with blank definition
 * HISTORY:
 * Last edited: May  6 14:17 2003 (edgrif)
 * * May 27 09:54 1999 (fw): complete re-write to achieve independence
 *              from spread-struct.
 * Created: Thu Jun 11 22:04:35 1992 (mieg)
 * CVS info:   $Id: tablemakerdisp.c,v 1.19 2004-04-22 22:23:35 mieg Exp $
 *-------------------------------------------------------------------
 */

#include <ctype.h>
#include <wh/acedb.h>
#include <wh/aceio.h>
#include <wh/lex.h>
#include <wh/pick.h>
#include <wh/bs.h>
#include <wh/query.h>
#include <wh/session.h>
#include <wh/dbpath.h>
#include <whooks/systags.h>
#include <whooks/sysclass.h>
#include <wh/spread_.h>		/* needs spread-Package internals */
#include <wh/display.h>
#include <wh/tree.h>
#include <wh/keysetdisp.h>
#include <wh/main.h>
#include <wh/tabledisp.h>

/************************************************************/
static magic_t TableMaker_MAGIC = "TableMaker";

typedef struct TableMakerStruct {
  magic_t *magic;		/* == &TableMaker_MAGIC */

  SPREAD spread;		/* the table definitions */

  Graph graph, dataGraph;

  int paramBox;
  int titleBox;
  int sortColonneBox;
  int isPrecomputeBox;

  Array columnInfo;		/* of type ColumnInfo */

  int definitionBox ;		/* contains the colonne definitions */

  int defBoxPerCol;		/* how many boxes drawn per colonne */

  int activeColonne;

  char dirSelection[DIR_BUFFER_SIZE];
  char fileSelection[FIL_BUFFER_SIZE];
} *TableMaker;

/*************/

typedef struct ColumnInfoStruct {
  int number;

  int boundingBox;
  int colNumBox;
  int hideBox;
  int optionalBox;
  int typeBox;
  int headerEntryBox;
  int widthBox;
  int extendBox;
  int fromBox;
  int tagBox;
  int tagTextBox;
  int conditionBox;
} ColumnInfo;

/************************************************************/

static Graph tableMakerDisplayCreate (SPREAD spread);
static void tableMakerDisplayReCreate (SPREAD spread);
static void tableMakerDisplayDestroy (void);

static void tableMakerAddColonne (void);
static void tableMakerRemoveColonne (void);

static void tableMakerSearchActiveKeySet (void) ;
static void tableMakerSearchWholeClass(void); 

static void tableMakerDisplayDraw (BOOL doCheckEntries);


static void tableMakerPickObj (void);
static void tableMakerSaveObj (void);

static void tableMakerImportDefs (void);
static void tableMakerExportDefs (void);


static void spreadChooseTag(TableMaker tmdisp, int box,
			    COL *c, COL *fromC, BOOL continuation);

static MENUOPT spreadDefMenu[] = {
  { graphDestroy,		"Quit"},
  { help,			"Help"},
  { graphPrint,			"Print"},
  { menuSpacer,			""},
  { tableMakerPickObj,		"Pick Obj"}, /* from existing object */
  { tableMakerSaveObj,		"Save Obj"}, /* store in object */
  { menuSpacer,			""},
  { tableMakerImportDefs,	"Import Def"}, /* from *.def file */
  { tableMakerExportDefs,	"Export Def"}, /* to *.def file */
  { menuSpacer,			""},
  { tableMakerSearchWholeClass,	"Search Whole Class"},
  { tableMakerSearchActiveKeySet,"Search Active KeySet"},
  { menuSpacer,			""},
  { tableMakerAddColonne,	"Add Definition" }, 
  { tableMakerRemoveColonne,	"Remove Definition" }, 
  { 0, 0 }
} ;

/************************************************************/

/* draws an encasing rectangle around the box we've just drawn */
#define graphBoxBox(_box) { \
	       float _x1, _y1, _x2, _y2 ; \
	       graphBoxDim (_box, &_x1, &_y1, &_x2, &_y2) ; \
	       graphRectangle (_x1 - 0.4, _y1 - 0.1, _x2 + 0.4, _y2 + 0.1) ; \
		}	   

static FREEOPT *classMenuOptions; /* pointer to a dynamic array of 
				   * FREEOPT's of possible class names */

/************************************************************/

/* DisplayFunc for TableMaker definition objects in class _VTable */
BOOL tableDisplay (KEY key, KEY from, BOOL isOldGraph, void *unused)
{
  SPREAD spread;

  if (class(key) != _VTable)
    return FALSE ;

  /* read defs literally without parameter substitution */
  spread = spreadCreateFromObj (key, NULL);
 
  if (!spread)
    return FALSE;

  /***** get the window *****/

  if (isOldGraph)
    tableMakerDisplayReCreate(spread); /* replace table defs
					* on active graph */
  else
    tableMakerDisplayCreate(spread);	/* open new graph */

  return TRUE;
} /* tableDisplay */

/***********************************************************************/

void tableMaker (void)		/* top-level menu func to start 
				 * table maker with empty definition */
{
  SPREAD spread;
  Stack s = stackCreate (100);

  pushText (s,
	    "Sortcolumn 1\n"
	    "Colonne 1");

  spread = spreadCreateFromStack (s, "") ;

  stackDestroy (s);

  tableMakerDisplayCreate(spread);

  return;
} /* tableMaker */

/***********************************************************************/
/*********************** private functions *****************************/
/***********************************************************************/

static TableMaker currentTableMaker (char *caller)
{
  /* find and verify TableMaker struct on active graph */
  TableMaker tmdisp = 0 ;

  if (!(graphAssFind(&TableMaker_MAGIC, &tmdisp)))
    messcrash("%s() could not find TableMakerStruct on graph", caller);
  if (!tmdisp)
    messcrash("%s() received NULL TableMakerStruct pointer", caller);
  if (tmdisp->magic != &TableMaker_MAGIC)
    messcrash("%s() received non-magic TableMakerStruct pointer", caller);
 
  if (arrayMax(tmdisp->spread->colonnes) == 0)
    messcrash("%s() received a TableMaker->spread with no colonnes", caller);

  return tmdisp; 
} /* currentTableMaker */



static Graph tableMakerDisplayCreate (SPREAD spread)
     /* create a new Table_Maker window with an existing spread-sheet */
     /* the spread struct now belongs to this function */
{
  TableMaker tmdisp;
  char *cp;

  if (arrayMax(spread->colonnes) == 0)
    messcrash("tableMakerDisplayCreate() - "
	      "cannot accept empty spread->colonnes");

  tmdisp = (TableMaker)messalloc(sizeof(struct TableMakerStruct));
  tmdisp->magic = &TableMaker_MAGIC;
  tmdisp->graph = displayCreate ("DtSpreadSheet");
  tmdisp->spread = spread;
  tmdisp->activeColonne = arrayMax(spread->colonnes)-1;
  cp = dbPathStrictFilName("wquery", "", "", "rd", 0);
  if (cp)
    {
      strcpy (tmdisp->dirSelection, cp);
      messfree(cp);
    }
  else				/* set to root db-dir */
    {
      cp = dbPathMakeFilName("", "", "", 0);
      strcpy (tmdisp->dirSelection, cp);
      messfree(cp);
    }
  
  if (graphHelp(0) == NULL)
    graphHelp("Table_Maker");

  graphRegister (DESTROY, tableMakerDisplayDestroy) ;

  graphAssociate (&TableMaker_MAGIC, tmdisp) ;

  tableMakerDisplayDraw(FALSE); /* draw table_maker screen layout */

  return tmdisp->graph;
} /* tableMakerDisplayCreate */

/*****************************************/

static void tableMakerDisplayReCreate (SPREAD spread)
/* re-use the active graph, and replace the table definitions
 * with the new ones, the user may be asked to save
 * the old ones, if they were loaded from a file */
{
  TableMaker tmdisp = currentTableMaker("tableMakerDisplayReCreate");

  if (spread != tmdisp->spread)
    {
      spreadDestroy (tmdisp->spread);
      
      tmdisp->spread = spread;
      tmdisp->activeColonne = arrayMax(spread->colonnes) - 1;
    }

  tableMakerDisplayDraw(FALSE); /* draw table_maker screen layout */

  return;
} /* tableMakerDisplayReCreate */

/*****************************************/

static void tableMakerDisplayDestroy (void)
     /* called when "DtSpreadSheet" graph dies
      * it also removes its child windows */
{    
  TableMaker tmdisp = currentTableMaker("tableMakerDisplayDestroy");
  
  if (tmdisp->spread->isModified)
    {
      if (tmdisp->spread->filename 
	  && strlen(tmdisp->spread->filename)
	  && messQuery ("%s has been modified, Save ?", 
			filGetFilename(tmdisp->spread->filename)))
	{
	  ACEOUT dump_out;

	  dump_out = aceOutCreateToChooser ("Choose a File to store "
					    "the Table-definition",
					    tmdisp->dirSelection,
					    tmdisp->fileSelection,
					    "def", "w", 0);
	  if (dump_out)
	    {
	      spreadDumpDefinitions (tmdisp->spread, dump_out);
	      aceOutDestroy (dump_out);
	    }
	}
      else if (tmdisp->spread->tableDefKey)
	{
	  if (messQuery ("%s has been modified, Save ?", 
			 name(tmdisp->spread->tableDefKey)) 
	      && checkWriteAccess())
	    {
	      /* The definitions have changed and we update the object 
	       * that they origibated from and we have to make sure
	       * that the precomputation is updated as well */

	      spreadSaveDefInObj (tmdisp->spread,
				  tmdisp->spread->tableDefKey) ;

	      tmdisp->spread->valuesModified = TRUE; /* will update store precomputation if possible */
	    }
	  else
	    {
	      /* the definitions have changed, but we didn't
	       * want to save them, so we can't store a
	       * precomputations anymore */
	      tmdisp->spread->precompute = FALSE;
	    }
	}
    }


  spreadDestroy(tmdisp->spread) ;

  if (graphExists(tmdisp->dataGraph))
    {
      graphActivate(tmdisp->dataGraph) ;
      graphDestroy() ;
    }

  tmdisp->magic = 0;
  messfree(tmdisp);

  return;
} /* tableMakerDisplayDestroy */


/*****************************************************/
/*       Colonne Definition Interface                */ 
/*****************************************************/

static COL *activateColonne (TableMaker tmdisp, int colNum)
{
  ColumnInfo *old, *cinfo;
  COL *col;
  
  old = arrayp(tmdisp->columnInfo, tmdisp->activeColonne, ColumnInfo);
  graphBoxDraw(old->colNumBox, BLACK, WHITE);

  tmdisp->activeColonne = colNum;
  cinfo = arrayp(tmdisp->columnInfo, tmdisp->activeColonne, ColumnInfo);
  
  graphBoxDraw(cinfo->colNumBox, BLACK, LIGHTGRAY);
 
  col = arrayp(tmdisp->spread->colonnes, tmdisp->activeColonne, COL);

  return col;
} /* activateColonne */


static COL *activateColonneByBoxNum(TableMaker tmdisp, int box)
{
  COL *col;
  int colNum;

  if (box > tmdisp->definitionBox)
    {
      colNum = tmdisp->defBoxPerCol ? 
	(box - tmdisp->definitionBox) / tmdisp->defBoxPerCol : 0 ;
    }

  col = activateColonne(tmdisp, colNum);

  return col ;
} /* activateColonneByBoxNum */


/******** Optional ********/

static void menuCallSpreadColOptional (KEY newValue, int box)
{
  COL *col ;
  TableMaker tmdisp = currentTableMaker("menuCallSpreadColOptional");

  col = activateColonneByBoxNum(tmdisp, box);

  spreadColSetPresence (tmdisp->spread, col->colonne, newValue);

  graphBoxDraw(box, BLACK, WHITE) ;

  return;
} /* menuCallSpreadColOptional */

/******** Extend ********/

static void menuCallSpreadColExtend (KEY newValue, int box)
{
  COL *col;
  TableMaker tmdisp = currentTableMaker("menuCallSpreadColExtend");

  col = activateColonneByBoxNum(tmdisp, box) ; 

  spreadColSetExtend (tmdisp->spread, col->colonne, newValue);

  graphBoxDraw(box, BLACK, WHITE) ;

  return;
} /* menuCallSpreadColExtend */

/******** Hide ********/

static void menuCallSpreadColHide (KEY newValue, int box)
{
  COL *col;
  TableMaker tmdisp = currentTableMaker("menuCallSpreadColHide");

  col = activateColonneByBoxNum(tmdisp, box) ; 

  spreadColSetHidden(tmdisp->spread, col->colonne, newValue);

  graphBoxDraw(box, BLACK, WHITE) ;

  return;
} /* menuCallSpreadColHide */

/******** Type ********/

static FREEOPT typeChoice[] = { 
  {9, "Class ..."},
  {'k', "Class"},
  {'i', "Integer"},
  {'f', "Float"},
  {'d', "Date"},
  {'t', "Text"},
  {'b', "Show_Tag"},
  {'n', "Next_Tag"},
  {'K', "Next_Key"},
  {'5', "Count"}
} ;

static FREEOPT typeChoice2[] ={ 
  {10, "Class ..."},
  {'x',      "Show Data"},
  {'b',      "Show Tag"},
  {'n',      "Show Next Tag"},
  {'K',      "Show Next Key"},
  {SHOW_MIN, "Compute Min"},
  {SHOW_MAX, "Compute Max"},
  {SHOW_AVG, "Compute Average"},
  {SHOW_VAR, "Compute Variance"},
  {SHOW_SUM, "Compute Sum"}, 
  {'c',      "Count"}
} ;

static void menuCallSpreadColType (KEY newValue, int box)
{
  COL *col;
  KEY oldValue;
  TableMaker tmdisp = currentTableMaker("menuCallSpreadColType") ;

  col = activateColonneByBoxNum(tmdisp, box);
  oldValue = col->type ;

  switch (newValue)
    {
    case 'b':			/* "Show Tag" */
      col->nonLocal = FALSE ;
      break ;

    case 'x':			/* "Show Data" */
				/* use real datatype of value */
      col->nonLocal = FALSE ;
      col->showType = SHOW_ALL ;
      col->type = col->realType ;
      col->showtypep = freekey2text(newValue, typeChoice2) ;
      goto ok ;
      break ;

    case 'n':			/* "Show Next Tag" */
      col->classe = _VSystem ;
      col->nonLocal = FALSE ;
      break ;

    case 'K':			/* "Show Next Key" */
	{
	  KEY classeKey ;
	  
	  if (graphSelect (&classeKey, classMenuOptions))
	    col->classe = classeKey ;
	  else
	    col->classe = 0 ;
	}
	col->nonLocal = FALSE ;
      break ;	

    case SHOW_MIN:
    case SHOW_MAX:	
    case SHOW_AVG:	
    case SHOW_VAR:	
    case SHOW_SUM:
      switch (col->realType)
	{
	case 'i': case 'f': case 'd':
          col->showType = newValue ;
	  col->type = col->realType ;
	  col->nonLocal = TRUE ;  
	  col->showtypep = freekey2text(newValue, typeChoice2) ;
	  tmdisp->spread->isModified = TRUE ;
	  goto ok ;
	  break ;
	default:
	  return ;
	}

    case 'c':
      col->nonLocal = TRUE ;
      break ;
    }

  col->showType = SHOW_ALL ;
  col->type = newValue ;
  col->typep = freekey2text(col->type, typeChoice2) ;

  if (oldValue != newValue)
    tmdisp->spread->isModified = TRUE ;

ok:
  graphBoxDraw(box, BLACK, LIGHTBLUE) ;

  tableMakerDisplayDraw(FALSE);

  return;
} /* menuCallSpreadColType */



/********* Pick boxes ************/

static void tableMakerPick(int box, double x, double y, int modifier_unused)
{
  TableMaker tmdisp = currentTableMaker("tableMakerPick") ;
  int colNum;
  COL *col ;
  ColumnInfo *cinfo;

  if (!graphCheckEditors (graphActive(), 0))
    return ;

  if (box == 0)			/* background */
    return ;

  if (box == tmdisp->titleBox)
    {
      graphTextEntry (tmdisp->spread->titleBuffer,0,0,0,0) ;
      tmdisp->spread->isModified = TRUE; /* assuming, that user will edit */
      return ;
    }
  else if (box == tmdisp->paramBox)
    {
      graphTextEntry(tmdisp->spread->paramBuffer,0,0,0,0) ;
      return ;
    }
  else if (box == tmdisp->sortColonneBox) 
    return ;
   
  tmdisp->spread->isModified = TRUE; /* assume we'll change something */

  for (colNum = 0; colNum < arrayMax(tmdisp->spread->colonnes); colNum++)
    {
      cinfo = arrayp(tmdisp->columnInfo, colNum, ColumnInfo);

      if (box == cinfo->boundingBox || 
	  box == cinfo->colNumBox ||
	  box == cinfo->widthBox)
	{
	  activateColonne(tmdisp, colNum);
	  break;
	}
      else if (box == cinfo->fromBox)
	{
	  activateColonne(tmdisp, colNum);
	  if (tmdisp->activeColonne == 0)
	    messcrash("shouldn't see the from box in colonne 0");
	  break;
	}
      else if (box == cinfo->conditionBox)
	{
	  col = activateColonne(tmdisp, colNum);
	  graphTextScrollEntry(col->conditionBuffer,0,0,0,0,0) ;
	  break;
	}
      else if (box == cinfo->headerEntryBox)
	{
	  col = activateColonne(tmdisp, colNum);
	  graphTextEntry(col->headerBuffer,0,0,0,0) ;
	  break;
	}
      else if (box == cinfo->typeBox)
	{ 
	  KEY typeValue;
	  
	  col = activateColonne (tmdisp, colNum);
	  
	  typeValue = col->type;

	  if (tmdisp->activeColonne == 0)
	    {
	      /* select classe of master colonne */
	      if (graphSelect (&typeValue, classMenuOptions))
		{ 
		  col->type = col->realType = 'k' ;
		  col->classe = typeValue ;
		  col->nonLocal = FALSE ;
		  col->typep = freekey2text (col->classe, classMenuOptions) ;
		}
	    }
	  else
	    {
	      if (graphSelect (&typeValue, typeChoice2))
		menuCallSpreadColType (typeValue, box) ;
	    }
	  graphBoxDraw (box, BLACK, WHITE) ;
	  tableMakerDisplayDraw (FALSE);
	  break;
	}
      else if (box == cinfo->tagBox)
	{
	  col = activateColonne (tmdisp, colNum);
	  spreadChooseTag (tmdisp, box, col,
			   arrp(tmdisp->spread->colonnes, col->from-1 , COL),
			   col->extend) ;
	  break;
	}
      else if (box == cinfo->tagTextBox)
	{
	  col = activateColonne (tmdisp, colNum);
	  graphTextScrollEntry (col->tagTextBuffer,0,0,0,0,0) ;
	  break;
	}
      else if (box == cinfo->hideBox)
	{
	  activateColonne (tmdisp, colNum);
	  spreadColCycleHidden (tmdisp->spread, colNum);
	  graphBoxDraw (box, BLACK, WHITE) ;
	  break;
	}
      else if (box == cinfo->optionalBox)
	{
	  activateColonne (tmdisp, colNum);
	  spreadColCyclePresence (tmdisp->spread, colNum);
	  graphBoxDraw (box, BLACK, WHITE) ;
	  break;
	}
      else if (box == cinfo->extendBox)
	{
	  col = activateColonne (tmdisp, colNum);
	  spreadColCycleExtend (tmdisp->spread, colNum);
	  graphBoxDraw (box, BLACK, WHITE);
	  
	  if (col->extend == COLEXTEND_RIGHT_OF && col->from == 1)
	    col->from = col->colonne;
	  else 
	    if (col->extend == COLEXTEND_FROM && col->from != 1)
	      col->from = 1 ;
	  
	  tableMakerDisplayDraw(FALSE) ;
	  break;
	}
    }

  return;
} /* tableMakerPick */

/*********Tag Chooser 1 *******************/

static void spreadChooseTag(TableMaker tmdisp, int box,
			    COL *c, COL *fromC, BOOL continuation)
{
  int classe ;
  COL *c1 = c ;
  Stack s1 = 0 ;
  KEY tag1 = 0, tag2 ;
  int type1 ;

  while (c1->extend == COLEXTEND_RIGHT_OF && 
	 fromC->from > 0  && 
	 fromC->from < arrayMax(tmdisp->spread->colonnes))
    { 
      c1 = fromC ;
      if (class(fromC->tag) || (fromC->tag && fromC->tag < _Date ))
	tag1 = 0 ;
      else
	{
	  tag1 = fromC->tag ;
	  if (lexword2key (c1->tagp, &tag2, _VSystem))
	    tag1 = tag2 ;
	}
      fromC = arrayp(tmdisp->spread->colonnes, fromC->from - 1, COL) ; 
    }

  if (fromC->type == 'K' && !fromC->classe)
    { messout("First define the class of column %d",c->from) ;
      return ;
    }
  if (freelower(fromC->type) != 'k' || !fromC->classe)
    { messout("First define the colonne you construct from") ;
      return ;
    }
  if (pickList[superClass(fromC->classe)].type != 'B')
    { messout("You cannot select a tag in non B-class: %s",
	      name(fromC->classe)) ;
      return ;
    }
  /* if user cancels, do not touch anything in c->  */
  s1 = stackCreate (32) ; 
  type1 = c->type ; 
  if (!treeChooseTagFromModel(&type1, &classe, fromC->classe, &tag1, s1
			      , continuation ? 2 : 1))
    return ;
  c->tag = tag1 ; c->type = type1 ;
  stackDestroy (c->tagStack) ;
  c->tagStack = s1 ;
  c->nonLocal = FALSE ; c->showType = SHOW_ALL ; c->realType = c->type ; 
  lexword2key(pickClass2Word(classe), &c->classe, _VClass) ;
  c->tagp = stackText(c->tagStack, 0) ;

  tmdisp->spread->isModified = TRUE ;
  tableMakerDisplayDraw(FALSE) ;

  return;
} /* spreadChooseTag */


static void menuCallSpreadColClass (KEY newValue, int box)
{
  COL *col;
  int oldValue; 
  TableMaker tmdisp = currentTableMaker("menuCallSpreadColClass") ;

  col = activateColonneByBoxNum(tmdisp, box) ;
  oldValue = col->classe ; 

  col->classe = newValue ;
  col->classp = name(col->classe) ;
  col->type = col->realType = 'k' ;
  col->nonLocal = FALSE ;
  
  tableMakerDisplayDraw(FALSE) ;
  if (newValue != oldValue)
    tmdisp->spread->isModified = TRUE ;

  return;
} /* menuCallSpreadColClass */

/************************************************/

static void entrySpreadTitle (char *entry)
     /* entryfunc for text-entry of colonne->titleBuffer */
{ 
  char *cp ;
  TableMaker tmdisp = currentTableMaker("entrySpreadTitle") ;

  cp = tmdisp->spread->titleBuffer ;
  while (*cp) cp ++ ;
  while (--cp >= tmdisp->spread->titleBuffer && *cp == ' ')
    *cp = 0 ;

  tmdisp->spread->isModified = TRUE ;

  tableMakerDisplayDraw(FALSE);	/* may want to retitle */

  return;
} /* entrySpreadTitle */
  
/************************************************/

static void entrySpreadParam (char *entry)
     /* entryfunc for text-entry of colonne->paramBuffer */
{ 
  char *cp ;
  TableMaker tmdisp = currentTableMaker("entrySpreadParam") ;

  cp = tmdisp->spread->paramBuffer ;
  while (*cp) cp ++ ;
  while (--cp >= tmdisp->spread->paramBuffer && *cp == ' ')
    *cp = 0 ;

  tmdisp->spread->isModified = TRUE ;

  return;
} /* entrySpreadParam */

/************************************************/

static void togglePrecompute (void)
     /* only possible on spread-defs that come from an object */
{
  TableMaker tmdisp = currentTableMaker("entrySpreadParam") ;

  if (tmdisp->spread->precompute)
    {
      tmdisp->spread->precompute = FALSE;
      graphBoxDraw (tmdisp->isPrecomputeBox, BLACK, WHITE);
    }
  else
    {
      tmdisp->spread->precompute = TRUE;
      graphBoxDraw (tmdisp->isPrecomputeBox, BLACK, LIGHTBLUE);
    }

  tmdisp->spread->isModified = TRUE;

  return;
} /* togglePrecompute */
  
/************************************************/

static BOOL checkSpreadSortColumn (int n)
     /* check function for graphIntEditor on spread->sortColumn */
{
  TableMaker tmdisp = currentTableMaker("checkSpreadSortColumn") ;

  if (n < 0 || n > arrayMax(tmdisp->spread->colonnes))
    {
      messout ("Error: Sort column - value must be between a valid "
	       "column number or 0, which means left "
	       "to right ordering.") ;
      return FALSE ;
    }
  return TRUE ;
} /* checkSpreadSortColumn */

/************************************************/

static BOOL checkSpreadColFrom (int n)
     /* check function for graphIntEditor on colonne->from */
{ 
  TableMaker tmdisp = currentTableMaker("checkSpreadColFrom") ;

  if (tmdisp->activeColonne == 0)
    return TRUE ;

  /* make sure it's a valid colonne number */
  if (n <= 0 ||
      n >= arrayMax(tmdisp->spread->colonnes))
    { 
      messout("Colonne %d is built \"From\" invalid colonne %d",
	      tmdisp->activeColonne, n) ;
      return FALSE ;
    }

  return TRUE ;
} /* checkSpreadColFrom */

/************************************************/

static BOOL checkSpreadColWidth (int n)
     /* check function for graphIntEditor on colonne->width */
{ 
  return (n > 0) ;
}

/************************************************/

static void entrySpreadColHeader (char *entry)
     /* entryfunc for text-entry of colonne->headerBuffer */
{
  COL *col ;
  char *cp ;
  TableMaker tmdisp = currentTableMaker("entrySpreadColHeader") ;

  col = arrayp(tmdisp->spread->colonnes, tmdisp->activeColonne, COL) ;  
  cp = col->headerBuffer ;
  while (*cp) cp ++ ;
  while (--cp >= col->headerBuffer && *cp == ' ')
    *cp = 0 ;

  tmdisp->spread->isModified = TRUE ;

  return;
} /* entrySpreadColHeader */
  
/************************************************/

static void entrySpreadColTagText(char *entry)
{
  COL *col ;
  char *cp ;
  TableMaker tmdisp = currentTableMaker("entrySpreadColTagText") ;

  col = arrayp(tmdisp->spread->colonnes, tmdisp->activeColonne, COL) ;  
  cp = col->tagTextBuffer ;
  if (!*cp)
    strcpy(cp, "?")  ;
  col->tagStack = stackReCreate(col->tagStack, 32) ;
  
  pushText(col->tagStack, cp) ;
  col->tagp = stackText(col->tagStack, 0) ;

  tmdisp->spread->isModified = TRUE ;

  tableMakerDisplayDraw(FALSE) ;

  return;
} /* entrySpreadColTagText */

  
/************************************************/

static void entrySpreadColCondition (char *entry)
     /* entryfunc for text-entry of colonne->conditionBuffer */
{
  COL *col ;
  char *cp ;
  TableMaker tmdisp = currentTableMaker("entrySpreadColCondition") ;

  col = arrayp(tmdisp->spread->colonnes, tmdisp->activeColonne, COL) ;  
  cp = col->conditionBuffer ;
  while (*cp) cp ++ ;
  while (--cp >= col->conditionBuffer && *cp == ' ')
    *cp = 0 ;
  if (*col->conditionBuffer &&
      !condCheckSyntax(messprintf(" %s", col->conditionBuffer)))
    messout("Please correct this syntax error") ;

  tmdisp->spread->isModified = TRUE ;

  return;
} /* entrySpreadColCondition */
  
/************************************************/

static void tableMakerAddColonne (void)
{
  TableMaker tmdisp = currentTableMaker("tableMakerAddColonne") ;
  COL *col;

  col = spreadColInit(tmdisp->spread,
		      arrayMax(tmdisp->spread->colonnes));
  tmdisp->activeColonne = col->colonne;

  tableMakerDisplayDraw(FALSE) ; /* re-draw */

  return;
} /* tableMakerAddColonne */

/************************************************/

static void tableMakerRemoveColonne (void)
{
  TableMaker tmdisp = currentTableMaker("tableMakerRemoveColonne") ;

  if (tmdisp->activeColonne == 0)
    {
      messout ("You cannot remove the first colonne") ;
      return;
    }

  if (!messQuery(messprintf("Do you really want to remove colonne %d",
			    tmdisp->activeColonne + 1)))
    return ;

  tmdisp->activeColonne = spreadRemoveColonne(tmdisp->spread, tmdisp->activeColonne);

  tableMakerDisplayDraw(FALSE) ;

  return;
} /* tableMakerRemoveColonne */

/***********************************************************/
/*********** Input Output of the definitions ***************/ 
/* The actual operation are in a separate non graphic file */


static void selectPickedDefinitionObj (KEY key)
     /* return from the display-block to  "Pick a table object" */
{
  displayUnBlock () ;

  if (class(key) != _VTable)
   messout ("Sorry, you must pick a Table object") ;
  else if (!iskey(key))
    messout ("Sorry, this object is empty.") ;
  else
    {
      /* display this tableObject and re-use existing graph */
      tableDisplay(key, 0, TRUE, NULL);

      graphPop();
    }

  return;
} /* selectPickedDefinitionObj */


static void tableMakerPickObj (void)
{
  KEYSET ks = query (0,"FIND Table") ;

  keySetNewDisplay (ks, "Table definitions") ;

  displayBlock (selectPickedDefinitionObj,
		"Pick a Table object") ;

  return;
} /* tableMakerPickObj */

static void tableMakerSaveObj (void)
{
  char *previous_name;
  char *nam = 0;
  TableMaker tmdisp = currentTableMaker("tableMakerSaveObj") ;
  ACEIN name_in;

  if (!graphCheckEditors (graphActive(), 0))
    return ;

  if (tmdisp->spread->tableDefKey)
    previous_name = name(tmdisp->spread->tableDefKey);
  else
    previous_name = "";

  name_in = messPrompt ("Please enter a Name to save this "
			"table-definition as an acedb object",
			previous_name, "w", 0);

  if (!name_in)
    goto abort;

  nam = strnew (aceInWord(name_in), 0) ;
  aceInDestroy (name_in);

  if (lexword2key (nam, &tmdisp->spread->tableDefKey, _VTable))
    { 
      if (!messQuery ("This table-definition already exists, "
		      "do you want to overwrite it ?"))
	goto abort ;
    }

  if (checkWriteAccess())
    {
      lexaddkey (nam, &tmdisp->spread->tableDefKey, _VTable) ;
      spreadSaveDefInObj (tmdisp->spread, tmdisp->spread->tableDefKey) ;

      tmdisp->spread->isModified = FALSE;
    }

abort: 
  messfree (nam) ;

  return;
} /* tableMakerSaveObj */


static void tableMakerImportDefs (void)
{
  TableMaker tmdisp = currentTableMaker("tableMakerDisplayDestroy");
  SPREAD newSpread;
  ACEIN spread_in;

  spread_in = aceInCreateFromChooser ("Choose a Table-definition file",
				      tmdisp->dirSelection,
				      tmdisp->fileSelection,
				      "def","r", 0);
  if (!spread_in)
    return ;

  newSpread = spreadCreateFromAceIn (spread_in, NULL);
  aceInDestroy (spread_in);

  if (!newSpread)
    return;

  tableMakerDisplayReCreate(newSpread);

  if (!graphCheckEditors (graphActive(), 0))
    {
      /* the definitions have been drawn, and some values
       * in the editor boxes don't check out */
      messout ("There are errors in some entry fields") ;
    }

  return;
} /* tableMakerImportDefs */


static void tableMakerExportDefs (void)
{
  TableMaker tmdisp = currentTableMaker("tableMakerExportDefs") ;
  ACEOUT dump_out;

  if (!graphCheckEditors (graphActive(), 0))
    return ; 

  dump_out = 
    aceOutCreateToChooser ("Choose a File to store the Table-definition",
			   tmdisp->dirSelection,
			   tmdisp->fileSelection,
			   "def", "w", 0);
  if (dump_out)
    {
      spreadDumpDefinitions (tmdisp->spread, dump_out);
      aceOutDestroy (dump_out);

      tmdisp->spread->isModified = FALSE;     
    }

  return;
} /* tableMakerExportDefs */


/**********************************************************/

static void tableMakerJumpToTop (void)
{
  graphGoto (1,1) ;
}

/**********************************************************/

static void showResultTable (TABLE *table, char *title, 
			     Array flipped, Array widths)
{
  TableMaker tmdisp = currentTableMaker("showResultTable") ;

  if (table)
    {
      if (graphActivate(tmdisp->dataGraph))
	tableDisplayReCreate (table, title, flipped, widths);
      else
	tmdisp->dataGraph = 
	  tableDisplayCreate(table, title, flipped, widths);  
    }

  if (graphActivate(tmdisp->dataGraph))
    graphPop();

  return;
} /* showResultTable */

static void tableMakerSearchWholeClass(void)
{ 
  TableMaker tmdisp = currentTableMaker("tableMakerSearchWholeClass") ;
  SPREAD calcSpread = 0;
  TABLE *table = 0;
  char *title_p;
  Array flipped, widths;

  if (!graphCheckEditors (graphActive(), 0))
    return ;

  tableDestroy(tmdisp->spread->values);

  /* try to pre-load */
  if (!tmdisp->spread->isModified &&
      spreadGetPrecomputation(tmdisp->spread))
    {
      printf ("loaded precomputation\n");

      table = tmdisp->spread->values;
      title_p = &tmdisp->spread->titleBuffer[0];
      flipped = spreadGetFlipInfo(tmdisp->spread);
      widths = spreadGetWidths(tmdisp->spread);
    }
  else
    {
      Stack s;
      ACEOUT def_out;

      s = stackCreate(1000);
      def_out = aceOutCreateToStack (s, 0);
      spreadDumpDefinitions (tmdisp->spread, def_out);
      aceOutDestroy (def_out);
      /* now read defs with parameter substitution (params != NULL)
       * but only using the params already given in the 
       * Parameter field */
      calcSpread = spreadCreateFromStack (s, "");
      stackDestroy(s);

      /* For the title for the tableDisplay, we use the
       * title of the definitions where parameter
       * substitution was performed. */
      title_p = &calcSpread->titleBuffer[0];
      flipped = spreadGetFlipInfo(calcSpread);
      widths = spreadGetWidths(calcSpread);

      if (spreadCalculateOverWholeClass(calcSpread))
	{
	  table = calcSpread->values;
	  tmdisp->spread->values = tableCopy(calcSpread->values, 0);

	  tmdisp->spread->valuesModified = TRUE;
	}
    }
    
  if (table)
    showResultTable (table, title_p, flipped, widths);

  arrayDestroy (flipped);
  arrayDestroy (widths);

  if (calcSpread)
    spreadDestroy(calcSpread);

  return;
} /* tableMakerSearchWholeClass */

/**********************************************************/

void tableMakerSearchActiveKeySet (void)
     /* derive the first master column from the active keyset
      * rather than the selected class */
{
  KEYSET ks = 0, ks2 = 0 ;
  KeySetDisp look ;
  COL *col ;
  TableMaker tmdisp = currentTableMaker("tableMakerSearchActiveKeySet") ;
  SPREAD calcSpread = 0;
  Array flipped;
  Array widths;
  Stack s;
  ACEOUT def_out;

  if (!graphCheckEditors (graphActive(), 0))
    return ;

  if (!spreadCheckConditions(tmdisp->spread))
    return ;

  col = arrp(tmdisp->spread->colonnes, 0, COL) ;
  if (!col->classe)
    {
      messout ("First choose the class of column 1 ") ;
      return ;
    }
  
  if (strlen(col->conditionBuffer) > 0 &&
      ! condCheckSyntax(messprintf(" %s", col->conditionBuffer)))
    { messout ("First fix the syntax error in column 1's condition") ;
      return ;
    }

  if (!keySetActive(&ks, &look))
    {
      messout("First select a KeySet, thank you") ;
      return ;
    }

  tableDestroy (tmdisp->spread->values);

  /***************************************************************
   * run the calculation on definitions where we've performed parameter
   * substitution, so %<n> strings aren't left in conditionBuffers etc.
   * So we dumpt the definitions into a stack and parse them back in
   * whilst performing parameter substitution. */

  s = stackCreate(1000);

  def_out = aceOutCreateToStack (s, 0);
  /* dump the definitions */
  spreadDumpDefinitions (tmdisp->spread, def_out);
  aceOutDestroy (def_out);

  /* and read them in with no extra parameters, other than the ones
   * given in the definition already */
  calcSpread = spreadCreateFromStack (s, "");
  stackDestroy(s);
  
  ks2 = spreadFilterKeySet (calcSpread, ks) ;

  calcSpread->isActiveKeySet = TRUE ;
  spreadCalculateOverKeySet (calcSpread, ks2, NULL);
  calcSpread->isActiveKeySet = FALSE ;

  flipped = spreadGetFlipInfo (calcSpread);
  widths = spreadGetWidths (calcSpread);
  
  if (calcSpread->values)
    {
      showResultTable (calcSpread->values, calcSpread->titleBuffer, flipped,
		       widths);
      tmdisp->spread->values = tableCopy (calcSpread->values, 0);

      /* This tableResult is based on an active keyset and therefor
       * unsuitable to be saved as a precomputed result.
       * We set valuesModified to false, which will make sure
       * the tmdisp->spread->values aren't saved on destruction
       * of tmdisp->spread. */
      tmdisp->spread->valuesModified = FALSE;
    }

  arrayDestroy(flipped);
  arrayDestroy(widths);

  keySetDestroy (ks2) ;

  spreadDestroy(calcSpread);

  return;
} /* tableMakerSearchActiveKeySet */

/************************************************************/

extern FREEOPT presenceChoice[], hideChoice[], extendChoice[];

static void tableMakerDisplayDraw (BOOL doCheckEntries)
     /* draw table_maker screen layout */
{ 
  static Array classMenuArray = 0;
  char *windowTitle;
  COL *col , *fromCol ;
  ColumnInfo *cinfo;
  int i, max, from;
  int lastBoxInCol, menuBox;
  float line, centralLine = 1 ;
  TableMaker tmdisp = currentTableMaker("tableMakerDisplayDraw") ;

  if (doCheckEntries && !graphCheckEditors (graphActive(), 0))
    return ;

  if (!arrayMax(tmdisp->spread->colonnes))
    messcrash("tableMakerDisplayDraw - no colonnes");

  /* set title, observing settings from displays.wrm if avail */
  windowTitle = displayGetTitle("DtSpreadSheet");
  if (strcmp(windowTitle, "DtSpreadSheet") == 0)
    windowTitle = "Table Maker"; /* if undef in displays.wrm */

  if (strlen(tmdisp->spread->titleBuffer) > 0)
    graphRetitle(messprintf("%s : %s",
			    windowTitle, 
			    tmdisp->spread->titleBuffer));
  else
    graphRetitle(messprintf("%s : untitled", windowTitle));


  /* re-do the menulist for the choice of possible classes */
  classMenuArray = classListCreate(classMenuArray, 
				   /* hidden visible or buried
				    * have to be B-classes with model */
				   TRUE, TRUE, TRUE, FALSE);
  classMenuOptions = arrp(classMenuArray, 0, FREEOPT);

  graphClear() ;
  graphMenu(spreadDefMenu) ;
  graphRegister (PICK, tableMakerPick) ;

  menuBox = graphBoxStart() ;
  graphButtons (spreadDefMenu, 1, 1, 75) ;
  graphBoxEnd() ;
  /* find out how far down the menu-buttons have been drawn */
  graphBoxDim (menuBox, 0, 0, 0, &line) ;
  line += 1 ;

  /* separator line */
  graphLine(0, line, 1000, line);  line += 0.5;

  /***************************************************************/
  /*           draw table header fields                          */
  /***************************************************************/
  graphText ("Sort column:", 3, line) ;
  tmdisp->sortColonneBox =
    graphIntEditor ("", &tmdisp->spread->sortColonne, 16, line, checkSpreadSortColumn) ;
  graphText ("F4 to interrupt", 40, line) ;
  line += 1.5 ;

  graphText ("Title:", 3, line) ;
  tmdisp->titleBox = 
    graphTextEntry (tmdisp->spread->titleBuffer, 59, 16, line, entrySpreadTitle) ;
  line += 1.5 ;

  graphText ("Parameters:", 3, line) ;
  tmdisp->paramBox = 
    graphTextScrollEntry (tmdisp->spread->paramBuffer, 179, 59, 16, line, entrySpreadParam) ;
  line += 1.5 ;

  if (tmdisp->spread->tableDefKey)
    {
      tmdisp->isPrecomputeBox = graphButton ("Precompute", togglePrecompute,
					     3, line);
      if (tmdisp->spread->precompute)
	graphBoxDraw (tmdisp->isPrecomputeBox, BLACK, LIGHTBLUE);
      graphText ("... to store the results in the database.", 16, line);
      line += 1.5 ;
    }

  /* separator line */
  graphLine(0, line, 1000, line); line += 0.5;


  /***************************************************************/
  /*           draw table colonne definitions                    */
  /***************************************************************/
  tmdisp->definitionBox = graphBoxStart() ;
  graphText ("Column", 1, line) ;
  line += 1.5 ;

  tmdisp->defBoxPerCol = 0;
  max = arrayMax(tmdisp->spread->colonnes) ;
  tmdisp->columnInfo = arrayReCreate(tmdisp->columnInfo, max, ColumnInfo);
  for (i = 0; i < max ; i++)
    {
      col = arrayp(tmdisp->spread->colonnes, i, COL);
      cinfo = arrayp(tmdisp->columnInfo, i, ColumnInfo);

      cinfo->boundingBox = graphBoxStart();

      cinfo->colNumBox = graphBoxStart();
      graphText(messprintf("%3d", i + 1), 2, line+2.5) ;
      graphRectangle(2   -0.4, line   -0.1,
		     2+3 +0.6, line+6 +0.1);
      graphBoxEnd();
 

      if (i == tmdisp->activeColonne)
	{
	  graphBoxDraw(cinfo->colNumBox, BLACK, LIGHTGRAY);
	  centralLine = line ;
	}

      graphText ("Header", 7, line) ;
      cinfo->headerEntryBox = 
	graphTextEntry (col->headerBuffer, 58, 17, line, 
			entrySpreadColHeader) ;
  
      graphText ("Width", 7, line += 1.3) ;
      cinfo->widthBox = 
	graphIntEditor ("", &col->width, 14, line, checkSpreadColWidth) ;
  
      cinfo->hideBox = graphBoxStart() ;
      graphTextPtrPtr(&col->hiddenp, 24, line, 7) ;
      graphBoxEnd() ;
      graphBoxBox(cinfo->hideBox) ;
      graphBoxFreeMenu(cinfo->hideBox, menuCallSpreadColHide, hideChoice);
      
      cinfo->optionalBox = graphBoxStart() ;
      graphTextPtrPtr(&col->optionalp, 33, line, 9) ;
      graphBoxEnd() ;
      if (i > 0) 
	{
	  graphBoxBox(cinfo->optionalBox) ;
	  graphBoxFreeMenu(cinfo->optionalBox, menuCallSpreadColOptional, presenceChoice) ;
	}
      else
	graphBoxClear(cinfo->optionalBox) ;


      cinfo->typeBox = graphBoxStart() ;
      switch (col->showType)
	{
	case SHOW_ALL:
	  if (col->type != 'c')
	    {
	      graphTextPtrPtr(&col->typep, 45, line, 7) ;
	      graphTextPtrPtr(&col->classp, 53, line, 22) ;   
	    }
	  else 
	    {  
	      col->showtypep = "COUNT" ;
	      col->realType = col->type ; /*mhmp 22.02.99 */
	      graphTextPtrPtr(&col->showtypep, 45, line, 12) ;
	    }
	  break ;
	case SHOW_MIN:
	case SHOW_MAX:
	case SHOW_AVG:
	case SHOW_VAR:
	case SHOW_SUM:
	  col->showtypep = freekey2text (col->showType, typeChoice2) ;
	  graphTextPtrPtr(&col->showtypep, 41, line, 12) ;
	  break ;
	}
      col->typep = freekey2text(col->type, typeChoice) ;
      col->classp = col->classe && freelower(col->type) == 'k' ?
	name(col->classe) : 0 ;
      graphBoxEnd() ;
      graphBoxBox(cinfo->typeBox) ;
      if (i > 0)
	graphBoxFreeMenu(cinfo->typeBox, menuCallSpreadColType, typeChoice2) ;
      else
	graphBoxFreeMenu(cinfo->typeBox, menuCallSpreadColClass, classMenuOptions) ;

      line += 1.5;

      cinfo->extendBox = graphBoxStart() ;
      graphTextPtrPtr(&col->extendp, 8, line, 8) ;
      graphBoxEnd() ;
      if (i > 0)
	{
	  graphBoxBox(cinfo->extendBox);
	  graphBoxFreeMenu(cinfo->extendBox, menuCallSpreadColExtend, extendChoice) ;
	}
      else
	graphBoxClear(cinfo->extendBox) ;

      from = col->from;

      if (i > 0)
	cinfo->fromBox = graphIntEditor ("", &col->from,
					 17, line, checkSpreadColFrom) ;
      else
	{
	  /* editor not really needed in master colonne, but we still
	   * draw the box without check-func to get the same number of boxes
	   * in each definition block */
	  cinfo->fromBox = graphIntEditor ("", &col->from,
					   17, line, 0) ;
	  graphBoxClear(cinfo->fromBox); /* clear the main editor box, 
					  * NOTE: cursor box still around */
	}
 	


      cinfo->tagBox = graphBoxStart() ;
      graphText ("Tag:" , 29, line) ;
      graphBoxEnd() ;
      if (i > 0)
	graphBoxBox(cinfo->tagBox) ;

      if (from > 0)
	fromCol = arrp(tmdisp->spread->colonnes, from - 1, COL) ;
      else 
	fromCol = 0 ;

      if (col->extend == COLEXTEND_FROM)
	if (i == 0 ||
	    !fromCol ||
	    freelower(fromCol->type) != 'k' ||
	    !fromCol->classe)
	  graphBoxClear(cinfo->tagBox) ;
      

      if (!stackExists(col->tagStack))
	strcpy(col->tagTextBuffer, "?") ;
      else
	strncpy(col->tagTextBuffer, stackText(col->tagStack,0), 359) ;
	
      cinfo->tagTextBox =
	graphTextScrollEntry(col->tagTextBuffer, 359, 
			     41, 34, line, 
			     entrySpreadColTagText) ;
      if (i == 0)
	{
	  graphBoxClear(cinfo->tagTextBox) ;
	  graphText ("To restrict the search, use the condition box "
		     "or search a keyset", 9 , line) ;
	}

      graphText ("Condition", 7, line += 1.3) ;
      cinfo->conditionBox = 
	graphTextScrollEntry (col->conditionBuffer, 359, 
			      58, 17, line, 
			      entrySpreadColCondition) ;

      graphBoxEnd();		/* boundingBox */

      /* last paragraph of the loop */
      lastBoxInCol = graphBoxStart() ;
      graphBoxEnd() ;


      if (tmdisp->defBoxPerCol == 0)
	tmdisp->defBoxPerCol = lastBoxInCol - tmdisp->definitionBox ;

      line += 2.5;
    }
  graphBoxEnd() ;

  graphButton("Add Definition", tableMakerAddColonne, 2, line) ;
  graphButton("Page top", tableMakerJumpToTop, 18, line) ;
  graphButton("Search Whole Class", tableMakerSearchWholeClass, 28, line) ;
  graphButton("Search Active KeySet", tableMakerSearchActiveKeySet, 49, line) ;
  graphTextBounds (88, line + 5) ;

  graphRedraw() ;
  graphGoto (1, centralLine) ;

  return ;
} /* tableMakerDisplayDraw */

/*********************** eof ********************************/
