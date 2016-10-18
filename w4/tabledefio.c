/*  File: tabledefio.c  (formerly known as sprddef.c)
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
    Read write to disk
     and or
    Get Save in object
        a table definition
 * Exported functions:
 * HISTORY:
 * Last edited: Apr  6 16:16 2001 (edgrif)
 * * Jun 14 15:52 1999 (fw): cleaned up I/O handling of
 *              param char sequences \% and %%1 etc..
 * Created: Fri Dec 11 13:53:37 1992 (mieg)
 *-------------------------------------------------------------------
 */

/* $Id: tabledefio.c,v 1.14 2001-04-06 16:32:21 edgrif Exp $ */

#include "acedb.h"
#include "aceio.h"
#include "lex.h"
#include "java.h"
#include "dump.h"
#include "whooks/systags.h"
#include "whooks/tags.h"
#include "whooks/sysclass.h"
#include "spread_.h"

/*****************************************************/

static void convertCondBufferIn (char *to_buf, char *from_buf,
				 int to_max);
static char *convertCondBufferOut (char *from_buf);

/*****************************************************/
/******* Input Output of the definitions *************/ 

FREEOPT presenceChoice[] = { 
  {3,"Presence options"},
  {VAL_IS_OPTIONAL,   "Optional"},
  {VAL_IS_REQUIRED,   "Mandatory"},
  {VAL_MUST_BE_NULL,  "Null"}
} ;

FREEOPT extendChoice[] = { 
  {2,"Extend options"},
  {COLEXTEND_RIGHT_OF, "Right_of"},
  {COLEXTEND_FROM,     "From"}
} ;

FREEOPT hideChoice[] = { 
  {2,"Hide options"},
  {TRUE,   "Hidden"},
  {FALSE,  "Visible"}
} ;

/**********/

static FREEOPT showOpts[] = {
  {6, "Show options"},
  {1, "MIN"},
  {2, "MAX"},
  {3, "Average"},
  {4, "Variance"},
  {5, "Sum"},
  {6, "Count"}
} ;

/******************************************************************
 ************************* public functions ***********************
 ******************************************************************/

void spreadDumpDefinitions (SPREAD spread, ACEOUT dump_out)
     /* write out complete table_maker spreadsheet definition */
{
  char *cp ;

  aceOutPrint (dump_out, "// Spread sheet definition for the ACeDB software \n") ;
  aceOutPrint (dump_out, "// User: %s\n", getLogin(TRUE)) ;
  aceOutPrint (dump_out, "// Date: %s\n\n", timeShow (timeNow())) ;
  aceOutPrint (dump_out, "// %%n (%%%%n in the graphic) are parameter to be given on the command line in tace\n") ;
  aceOutPrint (dump_out, "// or by default by the Parameters given in this file\n") ;
  aceOutPrint (dump_out, "// \\%%n (%%n in the graphic) are substituted by the value of column n at run time\n") ;
  aceOutPrint (dump_out, "// Line starting with // are ignored, starting with # are comments\n\n") ;
      
  if (stackExists(spread->comments))
    { stackCursor(spread->comments, 0) ;
      while ((cp = stackNextText(spread->comments)))
	aceOutPrint (dump_out,  "# %s", cp) ;
    }

  if (strlen(spread->titleBuffer) > 0)
    aceOutPrint (dump_out, "Title %s\n\n", spread->titleBuffer) ;
  if (strlen(spread->paramBuffer) > 0)
    aceOutPrint (dump_out, "Parameters %s\n\n", spread->paramBuffer);
  if (spread->sortColonne)
    aceOutPrint (dump_out, "Sortcolumn %d\n\n", spread->sortColonne) ;
  if (spread->precompute)
    aceOutPrint (dump_out, "Precompute\n\n") ;

  spreadDumpColonneDefinitions (dump_out, spread->colonnes) ;
  aceOutPrint (dump_out, " \n\n// End of these definitions\n") ;

  return;
} /* spreadDumpDefinitions */

/******************************************************/

void spreadColSetPresence(SPREAD spread, int colNum, KEY presence)
     /* the key refers to the FREEOPT presenceChoice */
{
  COL *col = arrayp(spread->colonnes, colNum, COL);
  SpPresenceType oldValue = col->presence;

  if (oldValue != (SpPresenceType)presence)
    {
      col->presence = (SpPresenceType)presence;
      col->optionalp = freekey2text(col->presence, presenceChoice) ;
      spread->isModified = TRUE;
    }
     
  return;
} /* spreadColSetPresence */

void spreadColCyclePresence(SPREAD spread, int colNum)
{
  COL *col = arrayp(spread->colonnes, colNum, COL);
  KEY newValue = (col->presence+1) % presenceChoice->key ;

  spreadColSetPresence(spread, colNum, newValue);

  return;
} /* spreadColCyclePresence */

/************************************************************/

void spreadColSetExtend(SPREAD spread, int colNum, KEY extend)
     /* the key refers to the FREEOPT extendChoice */
{
  COL *col = arrayp(spread->colonnes, colNum, COL);
  SpExtendType oldValue = col->extend;

  if (oldValue != (SpExtendType)extend)
    {
      col->extend = (SpExtendType)extend;
      col->extendp = freekey2text(col->extend, extendChoice) ;
      spread->isModified = TRUE;
    }
     
  return;
} /* spreadColSetExtend */

void spreadColCycleExtend(SPREAD spread, int colNum)
{
  COL *col = arrayp(spread->colonnes, colNum, COL);
  KEY newValue = (col->extend+1) % extendChoice->key ;

  spreadColSetExtend(spread, colNum, newValue);

  return;
} /* spreadColCycleExtend */

/************************************************************/

void spreadColSetHidden(SPREAD spread, int colNum, KEY hidden)
     /* the key refers to the FREEOPT hideChoice */
{
  COL *col = arrayp(spread->colonnes, colNum, COL);
  BOOL oldValue = col->hidden;

  if (oldValue != (BOOL)hidden)
    {
      col->hidden = (BOOL)hidden;
      col->hiddenp = freekey2text(col->hidden, hideChoice) ;
      spread->isModified = TRUE;
    }
     
  return;  
} /* spreadColSetHidden */

void spreadColCycleHidden(SPREAD spread, int colNum)
{
  COL *col = arrayp(spread->colonnes, colNum, COL);
  KEY newValue = (col->hidden+1) % hideChoice->key ;

  spreadColSetHidden(spread, colNum, newValue);

  return;
} /* spreadColCycleHidden */


/******************************************************************
 *********************** protected functions **********************
 ******************************************************************/

void spreadDumpColonneDefinitions (ACEOUT dump_out, Array colonnes)
     /* print all colonne definitions into the dump_out */
{
  int j , maxCol;
  COL *c ;
  
  maxCol = arrayMax(colonnes) ;
  for(j = 0 ; j < maxCol; j++)
    { c = arrp(colonnes,j, COL) ;
      if (!c->type)
	continue ;
      
      aceOutPrint (dump_out, "Colonne %d \n", j + 1) ;
      if (*c->headerBuffer)
	aceOutPrint (dump_out, "Subtitle %s \n", c->headerBuffer) ;
      
      aceOutPrint (dump_out, "Width %d \n", c->width) ;
      aceOutPrint (dump_out, "%s \n",freekey2text(c->presence, presenceChoice)) ;
      aceOutPrint (dump_out, "%s \n",freekey2text(c->hidden, hideChoice)) ;
      if (c->typep)
	aceOutPrint (dump_out, "%s \n",c->typep) ;
      if ((c->type == 'k') && c->classe)
	aceOutPrint (dump_out, "Class %s \n",name(c->classe)) ;
      if ((c->type == 'K') && c->classe)
	aceOutPrint (dump_out, "Next_Key %s \n", name(c->classe)) ;
      if (c->type == 'c')
	aceOutPrint (dump_out, "Count \n") ;
      if (c->showType != SHOW_ALL)
	aceOutPrint (dump_out, "Extract %s\n", freekey2text (c->showType, showOpts)) ;
      aceOutPrint (dump_out, "%s %d \n", freekey2text(c->extend, extendChoice), c->from);

      if (c->tagp && *c->tagp) aceOutPrint (dump_out, "Tag %s \n",c->tagp) ;
      if (strlen(c->conditionBuffer) > 0)
	{
	  aceOutPrint (dump_out, "Condition ") ;

	  aceOutPrint (dump_out, "%s", convertCondBufferOut(c->conditionBuffer));
	  aceOutPrint (dump_out, "\n") ;
	}
      aceOutPrint (dump_out, " \n") ;
    }

  return;
} /* spreadDumpColonneDefinitions */

/************************************************************/

static FREEOPT spreadReadMenu[] = 
{ 
  {45, "Spread Definitions"},
  {'#', "#"},
  {'a', "Table"}, /* first line if from object */
  {'T', "Title"},
  {'P', "Parameters"},
  {'s', "Subtitle"},
  {'c', "Colonne"}, 
  {'r', "From"},
  {'R', "Right_of"},
  {'g', "Tag"},
  {'v', "Visible"},
  {'h', "Hidden"},
  {'o', "Optional"},
  {'m', "Mandatory"},
  {'N', "Null"},
  {'C', "Condition"},
  {'k', "Class"},
  {'k', "A_Class"},
  {'i', "Integer"},
  {'i', "An_Int"},
  {'f', "Float"},
  {'f', "A_Float"},
  {'d', "Date"},
  {'d', "A_Date"},
  {'t', "Text"},
  {'t', "A_Text"},
  {'b', "Boolean"},
  {'B', "Show_Tag"},
  {'n', "Next_Tag"},
  {'K', "Next_Key"},
  {'S', "Sortcolumn"},
  {'E', "Extract"},
  {'w', "Width"}, 
  {'M', "FlipMap"},
  {'L', "Type"},
  {'L', "Presence"},
  {'L', "Visibility"},
  {'L', "Origin"},
  {'1', "All"},
  {'2', "Min"},
  {'3', "Max"},
  {'4', "Average"},
  {'5', "Variance"},
  {'6', "Sum"},
  {'7', "Count"},
  {'U', "Precompute"}
} ;

/************************************************************/

BOOL spreadSaveDefInObj (SPREAD spread, KEY tableKey)
{
  Stack s = 0 ;
  int ii, j , maxCol ; char *cp ;
  COL *c ;
  KEY tag ;
  char buf[2] ;
  OBJ tableObj ;
  
  buf[1] = 0 ; /* used in stackCat */
  if (class(tableKey) != _VTable)
    return FALSE ;
  
  if (!(tableObj = bsUpdate(tableKey)))
    return FALSE ;
  
  while (bsGoto (tableObj, 0), bsGetKeyTags (tableObj, _bsRight, 0))
    bsRemove (tableObj) ;
  
  if (stackExists(spread->comments))
    { 
      stackCursor(spread->comments, 0) ;
      while((cp = stackNextText(spread->comments)))
	bsAddData (tableObj, _Comment, _Text, cp) ;
    }
  
  if (strlen(spread->titleBuffer) > 0)
    bsAddData (tableObj, _Title, _Text, spread->titleBuffer) ;

  lexaddkey ("Precompute", &tag, 0) ;
  if (spread->precompute)
    bsAddTag (tableObj, tag) ;
      
  lexaddkey ("Sortcolumn", &tag, 0) ;
  if (spread->sortColonne)
    bsAddData (tableObj, tag, _Int, &spread->sortColonne) ;
      
  lexaddkey ("Parameters", &tag, 0) ;
  if (strlen(spread->paramBuffer) > 0)
    bsAddData (tableObj, tag, _Text, spread->paramBuffer) ;
      
  maxCol = arrayMax(spread->colonnes) ;
  for(j = 0 ; j < maxCol; j++)
    { c = arrp(spread->colonnes,j, COL) ;
      if (!c->type)
	continue ;
      
      ii = j + 1 ;
      lexaddkey ("Colonne", &tag, 0) ;
      bsAddData (tableObj, tag, _Int, &ii) ;
      bsPushObj(tableObj) ;
      
      if (c->headerBuffer && strlen(c->headerBuffer) > 0)
	bsAddData (tableObj, str2tag("Subtitle"), _Text, c->headerBuffer);

      lexaddkey ("Right_of", &tag, 0) ;
      if (c->extend == COLEXTEND_FROM)
	bsAddData (tableObj, _From, _Int, &c->from) ;
      else
	bsAddData (tableObj, tag, _Int, &c->from) ;
      
      lexaddkey ("Tag", &tag, 0) ;
      if (c->tagp && *c->tagp) 
	bsAddData (tableObj, tag, _Text, c->tagp) ;

      if (c->hidden)
	bsAddTag (tableObj, _Hidden) ;
      else
	bsAddTag (tableObj, _Visible) ;
      lexaddkey ("Width", &tag, 0) ;
      bsAddData (tableObj, tag, _Int, &c->width) ;
  
      switch (c->presence)
	{
	case VAL_MUST_BE_NULL:
	  lexaddkey ("Null", &tag, 0) ;
	  bsAddTag (tableObj, tag) ;
	  break ;
	case VAL_IS_OPTIONAL:
	  lexaddkey ("Optional", &tag, 0) ;
	  bsAddTag (tableObj, tag) ;
	  break ;
	case VAL_IS_REQUIRED:
	  lexaddkey ("Mandatory", &tag, 0) ;
	  bsAddTag (tableObj, tag) ;
	  break ;
	}

      if (strlen(c->conditionBuffer) > 0)
	{
	  lexaddkey ("Condition", &tag, 0) ;

	  cp = convertCondBufferOut(c->conditionBuffer);
	  bsAddData (tableObj, tag, _Text, cp) ;
	}

      switch (c->type)
	{
	case 'k':
	  lexaddkey ("A_Class", &tag, 0) ;
	  if (c->classe)
	    bsAddData (tableObj, tag, _Text, name(c->classe)) ;
	  break ;
	case 'K':
	  lexaddkey ("Next_Key", &tag, 0) ;
	  if (c->classe)
	    bsAddData (tableObj, tag, _Text, name(c->classe)) ;
	  break ;
	case 'i':
	  lexaddkey ("An_Int", &tag, 0) ;
	  bsAddTag (tableObj,tag) ;
	  break ;
	case 'f':
	  lexaddkey ("A_Float", &tag, 0) ;
	  bsAddTag (tableObj,tag) ;
	  break ;
	case 't':
	  lexaddkey ("A_Text", &tag, 0) ;
	  bsAddTag (tableObj,tag) ;
	  break ;
	case 'd':
	  lexaddkey ("A_Date", &tag, 0) ;
	  bsAddTag (tableObj,tag) ;
	  break ;
	case 'b':
	  lexaddkey ("Boolean", &tag, 0) ;
	  bsAddTag (tableObj,tag) ;
	  break ;
	case 'B':
	  lexaddkey ("Show_Tag", &tag, 0) ;
	  bsAddTag (tableObj,tag) ;
	  break ;
	case 'n':
	  lexaddkey ("Next_Tag", &tag, 0) ;
	  bsAddTag (tableObj,tag) ;
	  break ;
	case 'c':
	  lexaddkey ("Count", &tag, 0) ;
	  bsAddTag (tableObj,tag) ;
	  break ;
	}
      
      switch (c->type)
	{
	case 'c':
	  switch (c->realType)
	    {
	    case 'k':
	      lexaddkey ("A_Class", &tag, 0) ;
	      if (c->classe)
		bsAddData (tableObj, tag, _Text, name(c->classe)) ;
	      break ;
	    case 'i':
	      lexaddkey ("An_Int", &tag, 0) ;
	      bsAddTag (tableObj,tag) ;
	      break ;
	    case 'f':
	      lexaddkey ("A_Float", &tag, 0) ;
	      bsAddTag (tableObj,tag) ;
	      break ;
	    case 't':
	      lexaddkey ("A_Text", &tag, 0) ;
	      bsAddTag (tableObj,tag) ;
	      break ;
	    case 'd':
	      lexaddkey ("A_Date", &tag, 0) ;
	      bsAddTag (tableObj,tag) ;
	      break ;
	    case 'b':
	      lexaddkey ("Boolean", &tag, 0) ;
	      bsAddTag (tableObj,tag) ;
	      break ;
	    }
	default:
	  break ;
	}
      
      switch (c->showType)
	{
	case SHOW_ALL: /* this is the default */
	  break ;
	case SHOW_MIN:
	  bsAddTag (tableObj, _Min) ;
	  break ;
	case SHOW_MAX:
	  bsAddTag (tableObj, _Max) ;
	  break ;
	case SHOW_SUM:
	  lexaddkey ("Sum", &tag, 0) ;
	  bsAddTag (tableObj,tag) ;
	  break ;
	case SHOW_AVG:
	  lexaddkey ("Average", &tag, 0) ;
	  bsAddTag (tableObj,tag) ;
	  break ;
	case SHOW_VAR:
	  lexaddkey ("Variance", &tag, 0) ;
	  bsAddTag (tableObj,tag) ;
	  break ;
	}

      bsGoto (tableObj, 0) ;
    }

  bsSave (tableObj) ;
  stackDestroy (s) ;

  return TRUE ;
} /* spreadSaveDefInObj */

/******************************************************/

BOOL spreadReadDef (SPREAD spread, ACEIN def_io, char *inParams, 
		    BOOL isReadFromObj)
/* spreadReadDef - parse definitions into existing spread struct
 *                 existing defs are overwritten
 * PARAMETERS:
 *     the definitions are read from the ACEIN def_io
 *     the user-supplied parameters are inParams, which will be
 *        combined with the parameters stored in the object itself
 *        if NULL - the defs are parsed literally
 *        if valid string - parameter substitution is performed
 *     the boolean isReadFromObj tells whether the def_io
 *        input was created from an object.
 *
 * RETURNS:
 *   TRUE - is no syntax error etc. was encountered
 *   FALSE - if there was some error, the validity of the data in
 *           spread is now undefined */
{
  int i;
  KEY option ;
  char *cp, *card ;
  COL *c=0 ;
  BOOL firstCol = TRUE, shift = TRUE ;
  BOOL ret = TRUE ; /*mhmp 26.11.98 */
  Stack s1 = 0, paramStack = 0 ;
  ACEIN parse_io = NULL;
  char parse_special[24];

  tableDestroy(spread->values);
  spreadDestroyColonnes (spread->colonnes);
  spread->colonnes = arrayCreate(8, COL) ; 

  spread->comments = stackReCreate(spread->comments, 120) ;
  spread->sortColonne = 1 ; /* default */
  memset (spread->titleBuffer, 0, 60) ;
  memset (spread->paramBuffer, 0, 180) ;

  

  /* the paramStack contains multiple characters parameters,
   * when the free-package reads them, it'll use freeword to
   * parse them in - this behaviour will strip basically unprotect
   * the values, so in order for the protected values to be
   * substituted into the query, we need to freeprotect every
   * param-value TWICE - this is because of an inconsistency in the
   * free-package which would be too volatile to change, so
   * we work around it here this way */
  paramStack = stackCreate(200) ; 
  if (inParams && strlen(inParams) > 0)
    /* should be freeprotected (quoted) already
     * is protected here again */
    catText(paramStack, freeprotect(inParams)) ;
  
  if (inParams == NULL)		/* don't substitute params */
    strcpy (parse_special, "\n\t/@\\");	/* exclude % */
  else
    strcpy (parse_special, "\n\t/@%\\"); /* include % */

  /* first parsing pass - 
   * (1) extract parameters which are stored  with the definitions, 
   *     so we can add those to the passed parameter
   * (2) build up text-stack 's1' containing the verbatim definitions */
  s1 = stackCreate (2000) ;
  aceInSpecial (def_io,"\n/\\") ;
  while ((card = aceInCard(def_io)))
    {
      cp = aceInPos(def_io) ;
      if (cp && *cp) 
	catText (s1, cp) ;
      catText (s1, "\n") ;
      if (aceInKey (def_io, &option, spreadReadMenu))
	{
	  if (option == 'P') /* only interested in embedded parameters */
	    {
	      cp = aceInPos(def_io);
	      if (cp && strlen(cp) > 0)
		{
		  char *param_word;
		  ACEIN param_io;
		  
		  if (isReadFromObj)
		    cp = freeunprotect(cp) ;
		  
		  /* add a list doubly freeprotected
		   * parameters onto the stack */
		  param_io = aceInCreateFromText(cp, "", 0);
		  aceInSpecial (param_io,"\n/\\") ;
		  aceInCard(param_io);
		  while ((param_word = aceInWord(param_io)))
		    {
		      catText (paramStack, "  ") ;
		      
		      param_word = freeprotect(freeprotect(param_word));
		      catText (paramStack, param_word);
		    }
		  aceInDestroy(param_io);
		}
	    }
	}
      else if (aceInWord(def_io))
	/* we didn't match any of the options, but there still
	 * is more to be read, so this will be an error then */
	{
	  ret = FALSE ;
	  goto fin ;
	}
    }

  /* second parsing pass - 
   * parse the definitions from the stack that we've just built-up */

  parse_io = aceInCreateFromText(stackText (s1,0), stackText (paramStack,0), 0);
  aceInSpecial (parse_io, parse_special);

  while ((card = aceInCard(parse_io)))
    { 
    lao:
      if (aceInKey (parse_io, &option, spreadReadMenu))
	switch (option)
	  {
	  case '#':
	    pushText(spread->comments, aceInPos(parse_io)) ;
	    break ;
	  case 'U':		/* "Precompute" */
	    spread->precompute = TRUE ;
	    break ;
	  case 'c':		/* "Colonne" */
	    if (aceInInt(parse_io, &i))
	      {
		if (firstCol &&  i == 0)
		  shift = FALSE ; /* because in acedb.1.x i saved i, now i+1 */
		if (shift)
		  i-- ;

		c = arrayp(spread->colonnes,i, COL) ; 
		if (!c->from)
		  c = spreadColInit(spread, i);

		if (isReadFromObj)
		  goto lao ; /* loop on the inside of the def */
	      }
	    break ;
	  case 'E':		/* Extract */
	    if (c && aceInKey (parse_io, &option, showOpts) && option >= 1 && option <= 6)
	      { 
		if (option < 6)
		  c->showType = (SpShowType) ( option ) ;
		else
		  c->type = 'c' ;
	        c->nonLocal = TRUE ;
		c->typep = freekey2text(option, showOpts) ;
	      }
	    break ;
	  case 'w':		/* "Width" */
	    if (c && aceInInt(parse_io, &i))
	      c->width = i ;
	    break ;
	  case 'N':		/* "Null" */
	    if (c)
	      spreadColSetPresence(spread, c->colonne, VAL_MUST_BE_NULL);
	    break ;
	  case 'o':		/* "Optional" */
	    if (c)
	      spreadColSetPresence(spread, c->colonne, VAL_IS_OPTIONAL);
	    break ;
	  case 'm':		/* "Mandatory" */
	    if (c)
	      spreadColSetPresence(spread, c->colonne, VAL_IS_REQUIRED) ;
	    break ;
	  case 'M':		/* "FlipMap" */
	    if (c)
	      c->flipped = TRUE ;
	    break ;
	  case 'v':		/* "Visible" */
	    if (c)
	      spreadColSetHidden(spread, c->colonne, FALSE);
	    break ;

	  case 'h':		/* "Hidden" */
	    if (c)
	      spreadColSetHidden(spread, c->colonne, TRUE);
	    break ;

	  case 'i':		/* "Integer" or "An_Int" */
	  case 'f':		/* "Float" or "A_Float" */
	  case 'd':		/* "Date" or "A_Date" */
	  case 't':		/* "Text" or "A_Text" */
	    if (c)
	      {
		c->type = c->realType = option ; 
		c->nonLocal = FALSE ;
	      }
	    break ;

	  case 'k':		/* "Class" or "A_Class" */
	  case 'K':		/* "Next_Key" */
	    if (c)
	      {
		c->type = c->realType = option ; 
		c->nonLocal = FALSE ;
		if ((cp = aceInPos(parse_io)) && *cp)
		  {
		    if (isReadFromObj)
		      cp = freeunprotect(cp) ;
		    lexword2key(cp, &c->classe,_VClass) ;
		  }		  
	      }
	    break ;

	  case 'b':		/* "Boolean" */
	  case 'B':		/* "Show_Tag" */
	  case 'n':		/* "Next_Tag" */
	    if (c)
	      {
		if (option == 'B') option = 'b' ;
		c->type = option ; 
		c->realType = 'b' ;
		c->classe = _VSystem ;
		c->nonLocal = FALSE ;
	      }
	    break ;

	  case 'r':		/* "From" */
	    if (c)
	      {
		spreadColSetExtend(spread, c->colonne, COLEXTEND_FROM) ;
		if (aceInInt(parse_io, &i))
		  c->from = i ;
	      }
	    break ;

	  case 'R':		/* "Right_of" */
	    if (c)
	      {
		spreadColSetExtend(spread, c->colonne, COLEXTEND_RIGHT_OF);
		if (aceInInt(parse_io,&i))
		  c->from = i ;
	      }
	    break ;
	  case 'g':		/* "Tag" */
	    if (c && (cp = aceInPos(parse_io)) && *cp)
	      {
		c->tagStack = stackReCreate(c->tagStack, 30) ;
		if (isReadFromObj)
		  cp = freeunprotect(cp) ;
		pushText(c->tagStack, cp) ;
		c->tagp = stackText(c->tagStack, 0) ;
	      }	    
	    break ;

	  case 'C':		/* "Condition" */
	    if (c && (cp = aceInPos(parse_io)) && *cp)
	      { 
		char *cond_text = strnew(cp, 0);

		if (isReadFromObj)
		  {
		    char *tmp_buf = NULL ;
		    ACEIN cond_io = NULL ;

		    /* row-params conditions are stored as \%<n>,
		     * for storage in ace-format (in an object)
		     * they've been freeprotected with quotes,
		     * so here we could get "\%1 > \%2", which looks
		     * like \"\\%1 > \\%2\" in c-style escaping. */
		    /* NOTE that to ensure that convertCondBufer does the    */
		    /* right thing, we need to only remove the quotes, not   */
		    /* the \\ !!!                                            */
		    cond_io = aceInCreateFromText(cond_text, NULL, 0);
		    aceInSpecial (cond_io, parse_special);
		    aceInCard(cond_io);
		    tmp_buf = aceInPos(cond_io) ;
		    tmp_buf = strnew(aceInUnprotectQuote(cond_io, tmp_buf), 0) ;
		    aceInDestroy(cond_io);

		    convertCondBufferIn(c->conditionBuffer, tmp_buf, 360);
		    messfree (tmp_buf);
		  }
		else
		  convertCondBufferIn(c->conditionBuffer, cond_text, 360);

		messfree(cond_text);
	      }
	    break ;

	  case 's':		/* "Subtitle" */
	    if (c && (cp = aceInPos(parse_io)) && *cp)
	      {
		if (isReadFromObj)
		  cp = freeunprotect(cp) ;
		strncpy(c->headerBuffer, cp, 59) ;
	      }
	    break ;

	  case 'T':		/* "Title" */
	    if ((cp = aceInPos(parse_io)) && *cp)
	      {
		if (isReadFromObj)
		  cp = freeunprotect(cp) ;
		strncpy(spread->titleBuffer, cp, 59) ;
	      }
	    break ;

	  case 'S':		/* "Sortcolumn" */
	    aceInInt (parse_io, &spread->sortColonne) ;
	    break ;
	  case 'L':		/* "Type", 
				   "Presence",
				   "Visibility",
				   "Origin" */
	    /* intermediate tags, loop */
	    goto lao ;
	    break ;
	  case 'P':		/* "Parameters" */
	    if ((cp = aceInPos(parse_io)) && *cp)
	      {
		if (isReadFromObj)
		  cp = freeunprotect(cp) ;
		strncpy(spread->paramBuffer, cp, 179) ;
	      }
	    break ;
	  case 'a':		/* "Table" */
	    /* hopefully isReadFromObj = TRUE */
	    break ;

	  case '1':		/* "All" */
	    if (c)
	      {
		c->showType = SHOW_ALL ;
		c->nonLocal = FALSE ;
	      }
	    break ;

	  case '2':		/* "Min" */
	    if (c)
	      {
		c->showType = SHOW_MIN ;
		c->nonLocal = TRUE ;
	      }
	    break ;

	  case '3':		/* "Max" */
	    if (c)
	      {
		c->showType = SHOW_MAX ;
		c->nonLocal = TRUE ;
	      }
	    break ;

	  case '4':		/* "Average" */
	    if (c)
	      {
		c->showType = SHOW_AVG ;
		c->nonLocal = TRUE ;
	      }
	    break ;

	  case '5':		/* Variance" */
	    if (c)
	      {
		c->showType = SHOW_VAR ;
		c->nonLocal = TRUE ;
	      }
	    break ;

	  case '6':		/* "Sum" */
	    if (c)
	      {
		c->showType = SHOW_SUM ;
		c->nonLocal = TRUE ;
	      }
	    break ;

	  case '7':		/* Count" */
	    if (c)
	      {
		c->type = 'c' ;  c->showType = SHOW_ALL ;
		c->nonLocal = TRUE ;
	      }
	    break ;
	  default:
	    messcrash ("spreadReadDef() - unknown option") ;
	  }
      else if (aceInWord(parse_io))
	{
	  ret = FALSE;
	  goto fin ;
	}
    }

fin:

  if (parse_io)
    aceInDestroy (parse_io);

  stackDestroy (paramStack) ;
  stackDestroy (s1) ;

  spread->isModified = FALSE ;

  if (ret &&
      (spread->sortColonne < 0 ||
       spread->sortColonne > arrayMax(spread->colonnes)))
    {
      /* ---- inconvienient to output error here -------
       *
       * messerror("sort-column %d out of range 0-%d",
       * 	spread->sortColonne, arrayMax(spread->colonnes));
       */
      spread->sortColonne = 0;	/* default order, sort left->right */
    }

  return ret ;
} /* spreadReadDef */


Stack getDefinitionFromObj (KEY tableKey)
     /* read from the object 'tableKey' in class _VTable
      * and print the definitions onto a stack.
      * the parameters that are from the object are combined
      * with the parameters passed into the functions (parms)
      * and returned in a newly allocated string newParams 
      *
      * RETURNS a text-stack with the definitions */
{ 
  char *zq ;
  char oneCharBuf[2];
  ACEOUT dump_out;
  Stack dump_stack = 0, defStack = 0 ;

  if (class(tableKey) != _VTable)
    return NULL ;


  dump_stack = stackCreate (1000) ;
  dump_out = aceOutCreateToStack (dump_stack, 0);
  dumpKeyBeautifully (dump_out, tableKey, 'a', 0);
  aceOutDestroy (dump_out);

  /* when the parameter sequences
   * % (command param) and \% (row param) are read back from an object
   * which was dumped in ace-style into the database,
   * they come back as \% and \\\% respectively, so we convert back :
   * \%   -> %
   * \\\% -> \%
   * just as if they had come from a file */
  zq = stackText(dump_stack,0) ;
  oneCharBuf[1] = '\0';
  defStack = stackCreate(1000);
  while (*zq)
    {
      if (strncmp(zq,"\\\\\\%",4) == 0)	/* \\% literally */
	{ 
	  zq +=4 ;
	  catText(defStack, "\\%"); /* \% literally */
	}
      else if (strncmp(zq,"\\%", 2) == 0) /* \% literally */
	{
	  zq += 2;
	  catText(defStack, "%");
	}
      else 
	{
	  oneCharBuf[0] = *zq;
	  catText(defStack, oneCharBuf);
	  zq++ ;
	}
    }
  stackDestroy (dump_stack) ;

  return defStack ;
} /* getDefinitionFromObj */



/******************************************************************
 ************************ private functions ***********************
 ******************************************************************/

/* convert when loading from file into memory
 *         \% -> %    (row condition params)
 *         %  -> %%   (command line params)
 * Also, we try to get rid of lots of '\\' that appear due to
 * some problem in the free stuff which means that stuff gets
 * double quoted to protect it. I hope this is correct, the aquila
 * results for the "tables" tests seem to be correct now.
 */
static void convertCondBufferIn (char *to_buf, char *from_buf,
				 int to_max)
{
  int to_pos = 0;
  char *from_p = from_buf;

  while (to_pos < to_max && *from_p != '\0')
    { 
      if (*from_p == '\\' && *(from_p + 1) == '%')
	{
	  /* unprotect the \% char -> % */
	  from_p++ ;
	}
      else if (*from_p == '\\')
	{
	  /* try to strip out the \\ junk...                                 */
	  from_p++ ;
	}
      else if (*from_p == '%')
	{ 
	  /* double the single % char -> %%  */
	  to_buf[to_pos++] = '%';
	}

      to_buf[to_pos++] = *from_p ;
      from_p++ ;
    }
  to_buf[to_pos] = '\0' ;

  return;
} /* convertCondBufferIn */

static char *convertCondBufferOut (char *from_buf)
     /* convert when writing memory into file/object
      *         %  -> \%  (row condition params)
      *         %% -> %   (command line params)
      */
{
  static char *out_buf = 0;
  char *from_p = from_buf;
  char *out_p;

  if (out_buf) messfree(out_buf);

  out_buf = (char*)messalloc(strlen(from_buf) * 2 + 1);
  out_p = out_buf;

  while (*from_p)
    {
      if (*from_p == '%')
	{
	  if (*(from_p + 1) == '%')
	    from_p++;		/* skip one % to export %% as % */
	  else
	    {
	      *out_p = '\\';
	      out_p++;
	    }
	}
      *out_p++ = *from_p++;
    }

  return out_buf;
} /* convertCondBufferOut */

/******************************************************/
