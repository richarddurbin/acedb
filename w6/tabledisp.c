/*  File: spreaddata.c
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
 * Description:	graphical display of the (TABLE*) data-structure
 * Exported functions:
 * HISTORY:
 * Last edited: May  6 10:28 2003 (edgrif)
 * * Aug  4 10:55 1992 (mieg): Working on graph output
 * Created: Thu Jun 11 22:04:35 1992 (mieg)
 *-------------------------------------------------------------------
 */

/* @(#)sprddata.c	1.23 10/29/97 */

/*#define EDITBUTTON*/

#include "acedb.h"
#include <ctype.h>
#include "aceio.h"
#include "lex.h"
#include "pick.h"
#include "bs.h"
#include "query.h"
#include "whooks/systags.h"
#include "whooks/sysclass.h"
#include "spread.h"

#include "display.h"
#include "main.h"
#include "plot.h"
#include "keysetdisp.h"
#include "tabledisp.h"

BITSET_MAKE_BITFIELD	  /* define bitField for bitset.h ops */

/************************************************************/

static magic_t TableDisp_MAGIC = "TableDisp";

typedef struct TableDispStruct {
  magic_t *magic;
  Graph graph, mapGraph;
  char *title;
  TABLE *table;
  KEY trKey;			/* object in class _VTableResult */
  int maxRowDisplayed;
  ACEOUT dump_out;

  Array isFlipMapArray;		/* type BOOL */
  Array colMapping;		/* type int */
  Array maxColWidths;		/* type int */
  Array colHeaderBoxes;		/* type int */

  int activeCol;
  int activeRow;

  int boundingBox;
  int activeBox;
  int editModeBox;  /* used if #define EDITBUTTON */
  BOOL editMode ;   /* ditto */
} *TableDisp;

static TableDisp currentTableDisp (char *caller);
static void tableDisplayDraw (TableDisp tdisp);
static void tableDisplayDestroy (void);
static void tableDisplayPickObj (void);
static void tableDisplaySaveObj (void);
static void tableDisplayRotColonnes (void) ;
static void tableDisplaySwitchColonnes (void);
static void tableDisplaySortAsc (void) ;
static void tableDisplaySortDesc (void) ;
static void tableDisplayExportKeySet (void);
static void tableDisplayHistogram (void);
static void tableDisplayOpenMultiMap (void);
static void tableDisplayExportAsText (void);
static void tableDisplaySelectBox(TableDisp tdisp, int box) ;
static void tableDisplayPick(int box, double x, double y, int modifier_unused) ;
static void tableDisplayKeyBoard (int k, int modifier_unused) ;
#ifdef EDITBUTTON
static void spreadEditButton(void);
#endif /* EDITBUTTON */
static int showSubstrText (char *inText, int outLength, float x, float y, BOOL isBold);
static int getSafeTableLimit (TABLE *table, ACEOUT *dump_outp);

static MENUOPT tableDisplayMenu[] = {
  { graphDestroy,               "Quit"},
  { help,                       "Help"},
  { graphPrint,                 "Print"},
  { menuSpacer,                 "" },
  { tableDisplayPickObj,	"Pick Obj"}, /* load from object */
  { tableDisplaySaveObj,	"Save Obj"}, /* store in object */
  { menuSpacer,			""},
  { tableDisplayRotColonnes,    "Rotate columns"},
  { tableDisplaySwitchColonnes, "Switch columns"},
  { tableDisplaySortAsc,	"Sort asc"},
  { tableDisplaySortDesc,	"Sort desc"},
  { tableDisplayExportKeySet,   "Make KeySet"},
  { tableDisplayOpenMultiMap,   "Multi-Map"},
  { tableDisplayHistogram,      "Histogram"},
  { tableDisplayExportAsText,   "Export as text"},
  { 0, 0 }  
} ;

/******************************************************************
 ************************* public functions ***********************
 ******************************************************************/

/* DisplayFunc for TABLE objects in class _VTableResult */
BOOL tableResultDisplay (KEY key, KEY from, BOOL isOldGraph, void *unused)
{
  TABLE *table;
  TableDisp tdisp;

  if (class(key) != _VTableResult)
    return FALSE ;

  table = tableGet(key) ;

  if (!table)
    return FALSE ;

  if (isOldGraph)
    tableDisplayReCreate (table, name(key), 0, 0) ;
  else
    tableDisplayCreate (table, name(key), 0, 0);
      
  tableDestroy(table);		/* the display made it's own copy */

  tdisp = currentTableDisp("tableResultDisplay");
  tdisp->trKey = key;		/* remember that it came from an object */

  return TRUE;
} /* tableResultDisplay */

/************************************************************/

Graph tableDisplayCreate (TABLE *table, char *title, 
			  Array isFlipMapArray, Array widthsArray)
     /* open a display "DtTableResult" */
     /* the table belongs to the caller, we'll make our own copy */
{
  TableDisp tdisp;
  int colNum;

  tdisp = (TableDisp)messalloc(sizeof(struct TableDispStruct));
  tdisp->magic = &TableDisp_MAGIC;
  tdisp->graph = displayCreate("DtTableResult");
  tdisp->table = tableCopy(table, 0);
  tdisp->title = strnew(title, 0);
  tdisp->dump_out = 0;
  tdisp->trKey = 0;

  if (isFlipMapArray)
    tdisp->isFlipMapArray = arrayCopy(isFlipMapArray);
  else
    tdisp->isFlipMapArray = 0;

  tdisp->colMapping = arrayCreate(tdisp->table->ncol, int);
  for (colNum = 0; colNum < tdisp->table->ncol; colNum++)
    array(tdisp->colMapping, colNum, int) = colNum;

  tdisp->maxColWidths = arrayCreate(tdisp->table->ncol, int);
  for (colNum = 0; colNum < tdisp->table->ncol; colNum++)
    if (widthsArray && (colNum<arrayMax(widthsArray)))
      array(tdisp->maxColWidths, colNum, int) = arr(widthsArray, colNum, int);
    else
      array(tdisp->maxColWidths, colNum, int) = 30;

  tdisp->colHeaderBoxes = arrayCreate(tdisp->table->ncol, int);

  tdisp->activeCol = -1;

  /* default for help, if undef in displays.wrm */
  if (graphHelp(0) == NULL)
    graphHelp("Table_Maker_Data");
 
  graphMenu(tableDisplayMenu);

  graphAssociate (&TableDisp_MAGIC, tdisp) ;

  graphRegister (DESTROY, tableDisplayDestroy) ;
  graphRegister (PICK, tableDisplayPick) ;
  graphRegister (KEYBOARD, tableDisplayKeyBoard) ;

  if (tableMax(tdisp->table) > 0)
    {
      tdisp->maxRowDisplayed =
	getSafeTableLimit(tdisp->table, &tdisp->dump_out);

      if (!tdisp->maxRowDisplayed)
	{
	  graphDestroy();
	  return 0;
	}
      }

  tableDisplayDraw(tdisp);

  return tdisp->graph;
} /* tableDisplayCreate */


void tableDisplayReCreate (TABLE *table, char *title, 
			   Array isFlipMapArray, Array widthsArray)
{
  TableDisp tdisp = currentTableDisp("tableDisplayReCreate");
  int colNum;

  messfree (tdisp->title);
  tdisp->title = strnew(title, 0);

  tableDestroy(tdisp->table);
  tdisp->table = tableCopy(table, 0);
 
  arrayDestroy(tdisp->isFlipMapArray);
  if (isFlipMapArray)
    tdisp->isFlipMapArray = arrayCopy(isFlipMapArray);
  else
    tdisp->isFlipMapArray = 0;

  tdisp->colMapping = arrayReCreate(tdisp->colMapping, 
				    tdisp->table->ncol, int);
  for (colNum = 0; colNum < tdisp->table->ncol; colNum++)
    array(tdisp->colMapping, colNum, int) = colNum;


  tdisp->maxColWidths = arrayReCreate(tdisp->maxColWidths, 
				      tdisp->table->ncol, int);
  for (colNum = 0; colNum < tdisp->table->ncol; colNum++)
    if (widthsArray && (colNum<arrayMax(widthsArray)))
      array(tdisp->maxColWidths, colNum, int) = arr(widthsArray, colNum, int);
    else
      array(tdisp->maxColWidths, colNum, int) = 30;


  if (tableMax(tdisp->table) > 0)
    {
      tdisp->maxRowDisplayed =
	getSafeTableLimit(tdisp->table, &tdisp->dump_out);

      if (tdisp->maxRowDisplayed == 0)
	{
	  /* we were asked to re-use the existing table-display,
	   * but the result contains no data, so we get rid of the
	   * table display that is displayed, so next time round
	   * when there is data available we'll get a fresh display */
	  graphDestroy();
	  return;
	}
      }

  tdisp->activeBox = 0;		/* make sure that the new display 
				 * doesn't refer back to a box number 
				 * that was displayed last time */

  tableDisplayDraw(tdisp);
  
  return;
} /* tableDisplayReCreate */

/******************************************************************
 ************************ private functions ***********************
 ******************************************************************/

static TableDisp currentTableDisp (char *caller)
{
  TableDisp tdisp = 0;

  if (!(graphAssFind(&TableDisp_MAGIC, &tdisp)))
    messcrash("%s() could not find TableDisp on graph", caller);
  if (!tdisp)
    messcrash("%s() received NULL TableDisp pointer", caller);
  if (tdisp->magic != &TableDisp_MAGIC)
    messcrash("%s() received non-magic TableDisp pointer", caller);
  
  return tdisp;
} /* currentTableDisp */

static int getSafeTableLimit(TABLE *table, ACEOUT *dump_outp)
{
  const int safeLimit = 1000;
  int max = tableMax(table);
  int maxDisplayed = max;

  if (tableMax(table) > safeLimit)
    {
      graphMessage (
		    "Attention, the graphic display is limited to 1000 lines, but the full table can be queried and printed") ;
      
      maxDisplayed = safeLimit;
    }
  return maxDisplayed;
} /* getSafeTableLimit */

static void tableDisplayDraw (TableDisp tdisp)
{
  char *windowTitle;
  int menuButtonBox;
  int headerBox;
  float drawLine, topLine, bottomLine = 0;
  float drawCol;
  KEY key;
  float colWidth;
  int maxWidth, len;
  int rowNum, colNum, colMapped, maxCol;

  if (!graphActivate (tdisp->graph))
    return;

  windowTitle = displayGetTitle("DtTableResult");
  if (strcmp(windowTitle, "DtTableResult") == 0)
    windowTitle = "Table Results"; /* if undef in displays.wrm */

  if (strlen(tdisp->title) > 0)
    graphRetitle(messprintf("%s : %s",
			    windowTitle, tdisp->title));
  else
    graphRetitle(messprintf("%s : untitled", windowTitle));
		 

  /***********************/

  if (tdisp->dump_out)
    {
      /* dump the full table to the outputfile, everytime the screen 
       * is re-drawn */
      aceOutRewind (tdisp->dump_out);

      tableSliceOut(tdisp->dump_out, 0, tableMax(tdisp->table),
		    tdisp->table,
		    '\t',	/* separator char */
		    '\0') ;	/* format like 'a', but no format */
    }

  /***********************/
  graphClear() ;

  /* activate first visible column */
  if (tdisp->activeCol > -1)
    {
      BOOL firstPass = TRUE;
    again:
      while (tdisp->activeCol < tdisp->table->ncol &&
	     !tableColumnIsVisible(tdisp->table, tdisp->activeCol))
	tdisp->activeCol++;
      
      if (tdisp->activeCol >= tdisp->table->ncol)
	{
	  if (firstPass)
	    {
	      tdisp->activeCol = 0;
	      firstPass = FALSE;
	      goto again;
	    }
	  tdisp->activeCol = -1; /* none can't be activated */
	}
    }
  drawLine = 1 ;

#ifdef EDITBUTTON
  tdisp->editModeBox = graphButton("Edit", spreadEditButton, 1, drawLine + 0.2) ;
  graphBoxDraw(tdisp->editModeBox,BLACK, 
	       tdisp->editMode ? LIGHTBLUE : WHITE ) ;
  drawLine += 1.5;
#endif

  menuButtonBox = graphBoxStart () ;
  graphButtons (tableDisplayMenu, 1, drawLine, 60) ;
  graphBoxEnd () ;
  /* find out far down the menu buttons reach */
  graphBoxDim (menuButtonBox, 0, 0, 0, &drawLine) ;
  drawLine++;

  graphLine(0, drawLine, 1000, drawLine);
  drawLine += 0.5;

  if (strlen(tdisp->title) > 0)
    {
      float xb1, xb2, yb;
      int b;
      /* draw the title text into a box */
      b = graphBoxStart();
      graphText(tdisp->title, 2, drawLine) ;
      graphBoxEnd();
      /* get the size of the box and underline it */
      graphBoxDim (b, &xb1, 0, &xb2, &yb) ;
      graphLine(xb1-0.1, yb+0.1, xb2+0.1, yb+0.1);
     
      drawLine += 2 ;
    }

  maxCol = tdisp->table->ncol ;
  topLine = drawLine;

  tdisp->boundingBox = graphBoxStart() ;
  
  /* draw the table-data line-by-line */
  drawCol = 4; 
  for(colNum = 0; colNum < maxCol; colNum++)
    {
      colMapped = arr(tdisp->colMapping, colNum, int);
      maxWidth = arr(tdisp->maxColWidths, colMapped, int);
      colWidth = 0;		/* to be calculated as 
				 * widest cell in this column
				 * (within bounds of maxWidth) */
      drawLine = topLine;
      
      headerBox = graphBoxStart();
      if (tableColumnIsVisible(tdisp->table, colMapped))
	{
	  /* draw table header */
	  colWidth = showSubstrText(tableGetColumnName(tdisp->table, colMapped),
				    maxWidth, drawCol, drawLine, FALSE) ;
	  if (colWidth < 1.0)
	    colWidth = 1.0;
	  graphRectangle(drawCol-0.2, topLine-0.1, drawCol+colWidth+0.2, topLine+1.1);
	}
      else
	{
	  /* draw hidden header box */
	  graphLine(drawCol+0.5, topLine, drawCol+1, topLine+0.5);
	  graphLine(drawCol+1.0, topLine+0.5, drawCol+0.5, topLine+1);
	  graphLine(drawCol+0.5, topLine+1, drawCol, topLine+0.5);
	  graphLine(drawCol, topLine+0.5, drawCol+0.5, topLine);
	  colWidth = 0.5;
	}
      graphBoxEnd();
      array(tdisp->colHeaderBoxes, colMapped, int) = headerBox;

      drawLine += 2;

      for (rowNum = 0 ; rowNum < tabMax(tdisp->table, colMapped) ; rowNum++)
	{      
	  len = 0;	  

	  if (rowNum == tdisp->maxRowDisplayed)
	    {
	      showSubstrText("...", maxWidth, drawCol, drawLine, FALSE);
	      break;
	    }

	  /* always draw a box even if value is empty or column hidden */
	  graphBoxStart();    
	  
	  if (tableColumnIsVisible(tdisp->table, colMapped))
	    {
	      if (!tabEmpty(tdisp->table, rowNum, colMapped))
		switch(tdisp->table->type[colMapped])
		  {
		  case '0':
		    break ;
		  case 'i': 
		    len = showSubstrText(messprintf("%d", tabInt(tdisp->table, rowNum, colMapped)),
					 maxWidth, drawCol, drawLine, FALSE) ;
		    break ;
		  case 'f':
		    len = showSubstrText(messprintf("%f", tabFloat(tdisp->table, rowNum, colMapped)),
					 maxWidth, drawCol, drawLine, FALSE) ;
		    break ;
		  case 't':
		    len = showSubstrText(timeShow (tabDate(tdisp->table, rowNum, colMapped)),
					 maxWidth, drawCol, drawLine, FALSE) ;
		    break ;
		  case 'k':
		  case 'g':
		    key = tabKey(tdisp->table, rowNum, colMapped);
		    if (iskey(key))
		      len = showSubstrText(name(key),
					   maxWidth, drawCol, drawLine, (iskey(key) == 2)) ;
		    break ;
		  case 's':
		    len = showSubstrText(tabString(tdisp->table, rowNum, colMapped),
					 maxWidth, drawCol, drawLine, FALSE) ;
		    break ;
		  default:
		    messcrash("Unknown type '%c' in tableDisplayCreate()",
			      tdisp->table->type[colMapped] ? tdisp->table->type[colMapped] : '_');
		  }
	      if (len > colWidth)
		colWidth = len;
	      ++drawLine ;
	      if (drawLine > bottomLine)
		bottomLine = drawLine;
	    }
	  graphBoxEnd() ;

	}

      drawCol = drawCol + colWidth + 1.0;
      /* a hidden column has colWidth=0.5 at this point, 
       * so 1.5 is minimum width */
    }
  graphBoxEnd() ;		/* boundingBox */

  if(tdisp->maxRowDisplayed == tableMax(tdisp->table)) 
    graphText(messprintf("%d lines", tdisp->maxRowDisplayed), 35, 0.0) ;
  else
    graphText(messprintf("%d lines, %d shown", tableMax(tdisp->table), tdisp->maxRowDisplayed), 35, 0.0) ;
  
  graphTextBounds (drawCol + 3, bottomLine + 3) ;  

  if (tdisp->activeBox > 0)
    {
      int tmpbox = tdisp->activeBox;
      tdisp->activeBox = 0;	/* to avoid double-click behaviour */
      tableDisplaySelectBox(tdisp, tmpbox);
    }

  graphRedraw() ;

  if (graphActivate(tdisp->mapGraph))
    { 
      /* redo the multimap */
      multiMapDisplayReCreate(tdisp->table, tdisp->title, tdisp->isFlipMapArray);

      graphActivate(tdisp->graph) ; /* re-activate our graph */
      graphPop() ;
    }
  
  return;
} /* tableDisplayDraw */

/***********************************************************************/

static void tableDisplayDestroy (void)
     /* called when table-maker window is is destroyed */
{
  TableDisp tdisp = currentTableDisp("tableDisplayDestroy");

  if (tdisp->dump_out)
    aceOutDestroy (tdisp->dump_out);
 
  tableDestroy (tdisp->table);
  messfree(tdisp->title);

  arrayDestroy(tdisp->isFlipMapArray);
  arrayDestroy(tdisp->colMapping);
  arrayDestroy(tdisp->maxColWidths);
  arrayDestroy(tdisp->colHeaderBoxes);

  if (graphActivate(tdisp->mapGraph))
    graphDestroy();

  tdisp->magic = 0;
  messfree(tdisp);

  return;
} /* tableDisplayDestroy */


static int showSubstrText (char *inText, int outLength, float x, float y, BOOL isBold)
{
  static char line[1001];
  int oldFormat;

  if (outLength > 1000) 
    outLength = 1000;
  strncpy(line, inText, 1000);

  if (strlen(line) > outLength)
    {
      int maxLength = outLength;

      if (maxLength < 3)
	maxLength = 3;
      strcpy(&line[maxLength-3], "...");
    }

  line[outLength] = '\0';
  oldFormat = graphTextFormat(isBold ? BOLD : PLAIN);
  graphText(line, x, y);
  graphTextFormat(oldFormat);
  
  return strlen(line);
} /* showSubstrText */


static void selectPickedTableObj (KEY key)
{
  displayUnBlock () ;

  if (class(key) != _VTableResult)
   messout ("Sorry, you must pick a TableResult object") ;
  else if (!iskey(key))
    messout ("Sorry, this object is empty.") ;
  else
    {
      /* display this obj and re-use our existing graph */
      tableResultDisplay(key, 0, TRUE, NULL);

      graphPop();
    }

  return;
} /* selectPickedTableObj */


static void tableDisplayPickObj (void)
{
  KEYSET ks = query (0, "FIND TableResult") ;

  keySetNewDisplay (ks, "Table results") ;

  displayBlock (selectPickedTableObj,
		"Pick a TableResult object") ;

  return;
} /* tableDisplayPickObj */


static void tableDisplaySaveObj (void)
{
  char *nam;
  TableDisp tdisp = currentTableDisp("tableDisplaySaveObj");
  ACEIN name_in;

  nam = tdisp->trKey ? name(tdisp->trKey) : "" ;

  name_in = messPrompt ("Please enter a Name to save this "
			"table-result as an acedb object",
			nam, "w", 0);
  if (!name_in)
    return ;

  nam = strnew (aceInWord(name_in), 0) ;
  aceInDestroy (name_in);

  if (lexword2key (nam, &tdisp->trKey, _VTableResult))
    { 
      if (!messQuery ("This table-result already exists, "
		      "do you want to overwrite it ?"))
	goto abort ;
    }

  if (checkWriteAccess())
    {
      lexaddkey (nam, &tdisp->trKey, _VTableResult) ;
      tableStore (tdisp->trKey, tdisp->table) ;
    }

 abort:
  messfree(nam);

  return;
} /* tableDisplaySaveObj */


static void tableDisplayExportKeySet (void)
/* make a keyset from the object values in the active tableau column */
{
  KEYSET ks ; 
  TableDisp tdisp = currentTableDisp("tableDisplayExportKeySet");

  if (tableTypes(tdisp->table)[tdisp->activeCol] != 'k')
    { 
      messout ("The active column does not contain keys") ;
      return ;
    }

  ks = keySetCopy(tdisp->table->col[tdisp->activeCol]) ;
  keySetSort(ks) ;
  keySetCompress(ks) ;

  if (!arrayMax(ks))
    {
      messout ("The active colonne does not contain keys") ;
      keySetDestroy(ks) ;
    }
  else 
    { 
      keySetNewDisplay (ks, "Exported keyset") ;
    }

  return;
} /* tableDisplayExportKeySet */

/*****************************************************/

static void tableDisplayOpenMultiMap (void)
{
  TableDisp tdisp = currentTableDisp("tableDisplayOpenMultiMap");

  tdisp->mapGraph = multiMapDisplayCreate(tdisp->table, tdisp->title, 
					  tdisp->isFlipMapArray);

  return;
} /* tableDisplayOpenMultiMap */

/*****************************************************/
  
static void tableDisplayRotColonnes (void)
{
  TableDisp tdisp = currentTableDisp("tableDisplayRotColonnes");
  int colNum;
  int first = arr(tdisp->colMapping, 0, int);

  for (colNum = 0; colNum < arrayMax(tdisp->colMapping)-1; colNum++)
    {
      arr(tdisp->colMapping, colNum, int) = arr(tdisp->colMapping, colNum+1, int);
    }
  arr(tdisp->colMapping, arrayMax(tdisp->colMapping)-1, int) = first;

  tdisp->activeBox = 0;

  tableDisplayDraw(tdisp);

  return;
} /* tableDisplayRotColonnes */

static void tableDisplaySwitchColonnes (void)
{
  TableDisp tdisp = currentTableDisp("tableDisplaySwitchColonnes");
  int colNum, tmp;
  
  if (!tdisp->activeBox)
    return;

  for (colNum = 0; colNum < arrayMax(tdisp->colMapping)-1; colNum++)
    if (arr(tdisp->colMapping, colNum, int) == tdisp->activeCol)
      goto switchit;
  
  return;
  
 switchit:
  
  tmp = arr(tdisp->colMapping, colNum, int);
  arr(tdisp->colMapping, colNum, int) = 
    arr(tdisp->colMapping, colNum+1, int);
  arr(tdisp->colMapping, colNum+1, int) = tmp;

  tdisp->activeBox = 0;

  tableDisplayDraw(tdisp);

  return;
} /* tableDisplaySwitchColonnes */

/************************************************************/

static void tableDisplaySort (TableDisp tdisp, char spec)
{
  int colNum, colMapped, j;
  char *sortSpecString = (char*)messalloc ((tdisp->table->ncol*2) + 1);

  j = 0;
  for (colNum = 0; colNum < arrayMax(tdisp->colMapping); colNum++)
    {
      if (tableColumnIsVisible(tdisp->table, colNum))
	{
	  colMapped = arr(tdisp->colMapping, colNum, int);
	  sortSpecString[j*2] = spec;
	  sortSpecString[(j*2)+1] = '1'+colMapped;
	  j++;
	}
    }
  tableSort(tdisp->table, sortSpecString);

  messfree (sortSpecString);

  tableDisplayDraw(tdisp);

  return;
} /* tableDisplaySort */


static void tableDisplaySortAsc (void)
{
  TableDisp tdisp = currentTableDisp("tableDisplaySortAsc");

  tableDisplaySort(tdisp, '+');

  return;
} /* tableDisplaySortAsc */

static void tableDisplaySortDesc (void)
{
  TableDisp tdisp = currentTableDisp("tableDisplaySortDesc");

  tableDisplaySort(tdisp, '-');

  return;
} /* tableDisplaySortDesc */


/*****************************************************/

static void tableDisplayHistogram (void)
{
  int n, colNum, value ;
  Array a = 0 ;
  float x, scale = 1 ;
  TableDisp tdisp = currentTableDisp("tableDisplayHistogram");

  colNum = tdisp->activeCol ;

  switch (tableTypes(tdisp->table)[colNum])
    {
    case 'i':
    case 'f':
      break ;
    default:
      messout ("First select a numeric column") ;
      return ;
    }
  n = tabMax (tdisp->table, colNum) ;
  a = arrayCreate (1000, int) ;

  switch (tableTypes(tdisp->table)[colNum])
    {
    case 'i':
      while (n--)
	{
	  if (!tabEmpty(tdisp->table, n, colNum))
	    {
	      value = tabInt(tdisp->table, n, colNum);
	      value = MAX(value, 0); /* protect against referring to array element < 0 */
	      array(a, value, int)++ ;
	    }
	}
      break ;
    case 'f':
      scale = 0 ; 
      while (n--) 
	{ 
	  if (!tabEmpty(tdisp->table, n, colNum))
	    {
	      x = tabFloat(tdisp->table, n, colNum);
	      if (scale < x) 
		scale = x ; 
	    }
	}
      scale = scale > 0 ? 1000/scale : 0 ; 

      n = tabMax (tdisp->table, colNum) ;
      while (n--)
	{
	  if (!tabEmpty(tdisp->table, n, colNum))
	    {
	      value = (int)(scale * tabFloat(tdisp->table, n, colNum));
	      array(a, value, int)++ ; 
	    }
	}
      break ;
    default:
      return ;
    }

  plotHisto (tableGetColumnName(tdisp->table, colNum), a) ;

  return;
} /* tableDisplayHistogram */

/*****************/

static void tableDisplayExportAsText(void)
{
  static char directory[DIR_BUFFER_SIZE]="";
  static char filename[FIL_BUFFER_SIZE]="";
  static char sep = ';' ; 
  char *cp ;
  TableDisp tdisp = currentTableDisp("tableDisplayExportAsText") ;
  ACEIN sep_in;
  ACEOUT dump_out;

  sep_in = messPrompt ("Separator character "
		       "(e.g. ; , leave blank for TAB) ?",
		       messprintf("%c",sep), "t", 0);
  if (!sep_in)
    return;

  if ((cp = aceInPos (sep_in)))
    sep = *(cp + 1) ;
  aceInDestroy (sep_in);
  

  dump_out = 
    aceOutCreateToChooser ("Where should I ascii-dump this table ?",
			   directory, filename, "txt", "w", 0);
  if (!dump_out)
    return;

  tableSliceOut(dump_out, 0, tableMax(tdisp->table),
		tdisp->table,
		sep == '\0' ? '\t' : sep, /* separator char */
		'\0') ;	/* format like 'a', but no format */

  aceOutDestroy (dump_out);

  return;
} /* tableDisplayExportAsText */

/**********************************************************/

static void tableDisplaySelectBox(TableDisp tdisp, int box)
{ 
  int n = box - tdisp->boundingBox - 1;
  int boxesPerRow = tdisp->maxRowDisplayed + 1;	/* account for one extra header-box */
  int j, headerBox;
  
  if (box > tdisp->boundingBox)
    {
      if (tdisp->activeBox)
	graphBoxDraw (tdisp->activeBox, BLACK, WHITE) ;
   
      tdisp->activeBox = box ;
      tdisp->activeRow = n % boxesPerRow ;
      tdisp->activeRow--;		/* top box is the header */
      tdisp->activeCol = arr(tdisp->colMapping, (n / boxesPerRow), int) ;

      if (tdisp->activeRow >= 0)
	graphBoxDraw (tdisp->activeBox, WHITE, BLACK) ; /* invert selection */
      
      /* clear activeCol highlights */
      for (j = 0; j < tdisp->table->ncol; ++j)
	{
	  headerBox = arr(tdisp->colHeaderBoxes, j, int);
	  graphBoxDraw(headerBox, BLACK, WHITE);
	}
      
      /* highlight the activeCol */
      headerBox = arr(tdisp->colHeaderBoxes, tdisp->activeCol, int);
      if (tdisp->activeRow == -1)
	graphBoxDraw(headerBox, LIGHTBLUE, BLACK);
      else
	graphBoxDraw(headerBox, BLACK, LIGHTBLUE);
    }

  return;
} /* tableDisplaySelectBox */

#ifdef EDITBUTTON
static void spreadEditButton(void)
{
  SPREAD spread = currentSpread ("spreadEdit") ;

  spread->editMode = spread->editMode  ? FALSE : TRUE ;
  graphBoxDraw(spread->editModeBox,BLACK, 
	       spread->editMode ? LIGHTBLUE : WHITE ) ;
}


static Graph sprdAddKeyGraph = 0 ;
static BOOL isAddingKey = FALSE ;
static void sprdAddKey1 (KEY key)
{ 
  COL* c; 
  BSunit *u ;
  SPREAD spread = currentSpread ("sprdAddkey1") ;
  
  c = arrp( spread->colonnes, spread->activeColonne, COL) ;
  u = &(arr(spread->activeLine, spread->activeColonne, SPCELL).u) ; 
  if (class(key) != class(u->k))
    { messout("The selected object is not in the correct class, sorry") ;
      return ;
    }
  if (u->k != key)
    { u->k = key ;
      spreadReorder(table, sortCol) ;
      spreadDataDisplayCreate(spread) ;
    }

  return;
} /* sprdAddKey1 */


static void sprdAddKey (KEY key)
{ 
  graphActivate(sprdAddKeyGraph) ;
  graphRegister (MESSAGE_DESTROY, 0) ;
  graphUnMessage() ; /* this if i came from short cut sprdPick */
  displayUnBlock() ;
  isAddingKey = FALSE ;
  sprdAddKey1(key) ;

  return;
} /* sprdAddKey */


static void sprdAddKeyByName1 (void)
{
  char *cp ;
  KEY kk ;
  COL* c; 
  BSunit *u ;
  SPREAD spread = currentSpread ("sprdAddkey1") ;
  
  c = arrp( spread->colonnes, spread->activeColonne, COL) ;
  u = &(arr(spread->activeLine, spread->activeColonne, SPCELL).u) ; 
  if (!messPrompt("New value ", name(u->k),"w"))
    return ;
  cp = freeword() ;
		    
  if (!lexword2key(cp, &kk, class(u->k)))
    { if (messQuery("Unknown object, do you want to create it ?"))
	lexaddkey(cp, &kk, class(u->k)) ;
      else
	return ;
    }
  u->k = kk ;
  
  spreadReorder(spread) ;
  spreadDataDisplayCreate(spread) ;
  
  return;
} /* sprdAddKeyByName1 */


static void sprdAddKeyByName (void)
{ 
  graphActivate(sprdAddKeyGraph) ;
  isAddingKey = FALSE ;
  displayUnBlock() ;
  sprdAddKeyByName1() ;
  
  return;
} /* sprdAddKeyByName */

#endif /* EDITBUTTON */

static void tableDisplayFetchBox(TableDisp tdisp, int box)
{
#ifdef EDITBUTTON
  COL* c = arrp( spread->colonnes, spread->activeColonne, COL) ;
  BSunit *u = &(arr(spread->activeLine, spread->activeColonne, SPCELL).u) ; 
  BOOL touched = FALSE ;

  if (spread->editMode)
    /****************  edit table item in selected box ***********/
    switch (c->type)
      {
      case 'b':
	u->k = u->k ? 0 : c->tag ;
	touched = TRUE ;
	break ;
      case 'k': case'n': case 'K':
	isAddingKey = TRUE ;
	sprdAddKeyGraph = graphActive() ;
	displayBlock (sprdAddKey, "Or cancel, you will then be prompted to type the name.\n"
		      "If you press the TAB key you will autocomplete\n"
		      "or get a list of possible entries.\n") ;
	graphRegister (MESSAGE_DESTROY, sprdAddKeyByName) ;
	return ;
      case 'i':
        if (messPrompt("New value ", messprintf("%d",u->i),"i"))
	  freeint(&(u->i)) ;
	touched = TRUE ;
	break ;
      case 'f':
        if (messPrompt("New value ", messprintf("%g",u->f),"f"))
	  freefloat(&(u->f)) ;
	touched = TRUE ;
	break ;
      case 't':
        if (messPrompt("New value ",
		       stackExists(c->text) && u->i ? stackText(c->text, u->i) : "" , 
		       "t"))
	  { u->i = stackMark(c->text) ;
	    pushText(c->text, freepos()) ;
	  }
	touched = TRUE ;
	break ;
      default:
	break ;
      }
  else
#endif /* EDITBUTTON */
    {
      /************* fetch table item in selected box *************/
      KEY cellKey, cellOriginKey;

      if (tdisp->activeRow >= 0)
	{
	  cellKey = tabParent(tdisp->table, tdisp->activeRow, tdisp->activeCol);
	  cellOriginKey = tabGrandParent(tdisp->table, tdisp->activeRow, tdisp->activeCol);
	  
	  if (cellKey)
	    display(cellKey, cellOriginKey, 
		    tdisp->activeCol > 0 ? (char*)0 : "TREE") ;
	}
      else
	{
	  /* second click on column-header box - toggle visibility */
	  if (tableColumnIsVisible(tdisp->table, tdisp->activeCol))
	    tableSetColumnHidden(tdisp->table, tdisp->activeCol);
	  else
	    tableSetColumnVisible(tdisp->table, tdisp->activeCol);

	  tdisp->activeBox = 0;
	  tableDisplayDraw(tdisp);
	}
    }

#ifdef EDITBUTTON
  if(touched)
   { spreadReorder(spread) ;
     spreadDataDisplayCreate(spread) ;
   }  
#endif /* EDITBUTTON */

  return;
} /* tableDisplayFetchBox */



static void tableDisplayKeyBoard (int k, int modifier_unused)
{ 
  /* does nothing yet, but I'd want it to allow moving of the 
   * red selection box with cursor keys (similar to KeySet window) */
  return ;
}

static void tableDisplayPick (int box, double x, double y, int modifier_unused)
{
  TableDisp tdisp = currentTableDisp("tableDisplayPick") ;
  
  if (box == 0)			/* picked background */
    return ;

  if (box > tdisp->boundingBox) 
     {
#ifdef EDITBUTTON
       if (isAddingKey)
	 { int 
	     j, j1, n = box - spread->boundingBox - 1 ,
	     max = spread->numberDisplayedColonnes ,
	     maxCol = arrayMax(spread->colonnes) ;
	   COL *c ;
	   char *cp ;
	   Array aa ;
	   
	   j1 = n / max ;
	   for(j = 0 , cp = arrp(spread->flags, 0, char) ; 
	       j < arrayMax(spread->tableau); cp++, j++)
	     if (!*cp)
	       { if (!j1--)
		   { aa = arr(spread->tableau, j, Array) ;
		     break ;
		   }
	       }
	   n = n%max ;
	   for(j = 0 ; j < maxCol; j++)
	     { c = arrp(spread->colonnes,j1 = arr(pos2col_G,j, int) ,COL) ;
	       if (!c->hidden)
		 { if (!n--)
		     { 
		       break ;
		     }
		 }
	     }
	
	   if (freelower(c->type) == 'k' || c->type == 'n')
	     sprdAddKey ((arr(aa, j1, SPCELL).u).k) ;
	   return ;
	 }
#endif /* EDITBUTTON */
       
	
       if (box == tdisp->activeBox) /* a second hit - follow it */
	 tableDisplayFetchBox (tdisp, box) ;
       else
	 tableDisplaySelectBox (tdisp, box) ; /* single click */
     }

  return;
} /* tableDisplayPick */

/************************* eof ********************************/
