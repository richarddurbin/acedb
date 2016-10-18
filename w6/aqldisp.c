/*  File: aqldisp.c
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
 * Description:
 * Exported functions:
 * HISTORY:
 * Last edited: Jun 13 14:37 2007 (edgrif)
 * Created: Fri May 28 09:49:03 1999 (fw)
 *-------------------------------------------------------------------
 */

#include "acedb.h"
#include "aceio.h"
#include "aql.h"
#include "table.h"

#include "display.h"
#include "tabledisp.h"
#include "keysetdisp.h"

/************************************************************/

static magic_t AqlDisp_MAGIC = "AqlDisp";

typedef struct AqlDispStruct {
  magic_t *magic;

  TABLE *table;

  Graph graph, dataGraph;

  char query[1000];
  int queryEntryBox;

  Stack errStack;
} *AqlDisp;

static void aqlDisplayDestroy (void);
static void aqlDisplayDraw (AqlDisp adisp);
static AqlDisp currentAqlDisp (char *caller);
static void entryQuery (char *query);
static void runQuery (void);
static void clearQuery (void);

static MENUOPT aqlDisplayMenu[] = {
  { graphDestroy, "Quit"},
  { help,         "Help"},
  { menuSpacer,   ""},
  { runQuery,   "Run query"},
  { clearQuery,   "Clear query"},
  { 0, 0 }
};
/************************************************************/

void aqlDisplayCreate (void)
{
  AqlDisp adisp;

  adisp = (AqlDisp)messalloc(sizeof(struct AqlDispStruct));
  adisp->magic = &AqlDisp_MAGIC;
  adisp->graph = displayCreate("DtAqlQuery");

  /* defaults for title and help, if undef in displays.wrm */
  if (strcmp(displayGetTitle("DtAqlQuery"), "DtAqlQuery") == 0)
    graphRetitle("Aql Query");
  if (graphHelp(0) == NULL)
    graphHelp("AQL/index");

  graphMenu(aqlDisplayMenu);
  graphAssociate(&AqlDisp_MAGIC, adisp);
  graphRegister(DESTROY, aqlDisplayDestroy);

  aqlDisplayDraw(adisp);

  return;
} /* aqlDisplayCreate */

/***********************************************************************/
/*********************** private functions *****************************/
/***********************************************************************/

static AqlDisp currentAqlDisp (char *caller)
{
  /* find and verify AqlDisp struct on active graph */
  AqlDisp adisp = 0 ;

  if (!(graphAssFind(&AqlDisp_MAGIC, &adisp)))
    messcrash("%s() could not find AqlDispStruct on graph", caller);
  if (!adisp)
    messcrash("%s() received NULL AqlDispStruct pointer", caller);
  if (adisp->magic != &AqlDisp_MAGIC)
    messcrash("%s() received non-magic AqlDispStruct pointer", caller);
 
  return adisp; 
} /* currentAqlDisp */


static void aqlDisplayDestroy (void)
{
  AqlDisp adisp = currentAqlDisp("aqlDisplayDestroy");

  if (graphActivate(adisp->dataGraph))
    graphDestroy();

  tableDestroy(adisp->table);

  adisp->magic = 0;
  messfree(adisp);
  
  return;
} /* aqlDisplayDestroy */


static void aqlDisplayDraw (AqlDisp adisp)
{
  float drawLine = 1.0;
  int menuBox;

  graphClear();

  menuBox = graphBoxStart();
  graphButtons (aqlDisplayMenu, 1, drawLine, 50);
  graphBoxEnd();
  graphBoxDim (menuBox, 0, 0, 0, &drawLine);
  drawLine += .5;

  adisp->queryEntryBox = graphTextScrollEntry (adisp->query, 1000, 50, 1, drawLine, entryQuery);

  drawLine += 1.5;

  if (stackExists(adisp->errStack))
    {
      char *error = strnew(stackText(adisp->errStack, 0), 0);
      int len =  strlen(error);
      char *start = error;
      char *cp = start;
	      
      while (cp <= error + len)
	{
	  if (*cp == '\0' || *cp == '\n')
	    {
	      *cp = '\0';
	      graphText (start, 2, drawLine+0.3) ;
	      start = cp+1;
	      drawLine += 1.0;
	    }
	  cp++;
	}
      drawLine += 0.7 ;

      messfree(error);
    }


  graphRedraw();

  if (adisp->table)
    {
      if (graphActivate(adisp->dataGraph))
	tableDisplayReCreate(adisp->table, "results", 0, 0);
      else
	adisp->dataGraph = tableDisplayCreate(adisp->table,
					      "results", 0, 0);
    }
  else if (graphActivate(adisp->dataGraph))
    graphDestroy();

  return;
} /* aqlDisplayDraw */


static void entryQuery (char *query)
{
  runQuery();
} /* entryQuery */

static void clearQuery (void)
{
  AqlDisp adisp = currentAqlDisp("clearQuery");

  memset(adisp->query, 0, 1000);

  tableDestroy(adisp->table);

  aqlDisplayDraw(adisp);

  return;
} /* clearQuery */

static void runQuery (void)
{
  AqlDisp adisp = currentAqlDisp("runQuery");
  KEYSET activeKs;
  AQL aql;
  TABLE *activeTable;
  ACEOUT error_out;

  tableDestroy(adisp->table);
  adisp->errStack = stackReCreate(adisp->errStack, 100);

  error_out = aceOutCreateToStack (adisp->errStack, 0); /* receives errors etc. */

  if (keySetActive(&activeKs, 0))
    {
      /* make the selected keyset available to the aql query */
      activeTable = tableCreateFromKeySet(activeKs, 0);

      aql = aqlCreate (adisp->query, NULL, 0, 0, 0, "@active", (char *)NULL) ;
      adisp->table = aqlExecute (aql, NULL, NULL, NULL, "TABLE @active", activeTable, (char *)NULL);
      tableDestroy (activeTable);

      if (aqlIsError(aql))
	aceOutPrint (error_out, "%s", aqlGetErrorReport(aql));

      aqlDestroy(aql);
    }
  else
    {
      /* no context vars, quick func, errors to errStack */
      adisp->table = aqlTable (adisp->query, error_out, 0);
    }

  aceOutDestroy (error_out);

  aqlDisplayDraw (adisp);

  return;
} /* runQuery */

/************************* eof ******************************/
