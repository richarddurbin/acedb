/*  File: tqdebug.c
 *  Author: Richard Durbin (rd@sanger.ac.uk)
 *  Copyright (C) J Thierry-Mieg and R Durbin, 1996
 *-------------------------------------------------------------------
 * This file is part of the ACEDB genome database package, written by
 * 	Richard Durbin (MRC LMB, UK) rd@sanger.ac.uk, and
 *	Jean Thierry-Mieg (CRBM du CNRS, France) mieg@kaa.cnrs-mop.fr
 *
 * SCCS: $Id: tqdebug.c,v 1.2 1996-11-05 22:08:09 rd Exp $
 * Description:
 * Exported functions:
 * HISTORY:
 * Last edited: Nov  5 22:04 1996 (rd)
 * Created: Fri Oct 25 16:04:41 1996 (rd)
 *-------------------------------------------------------------------
 */

#include "tq_.h"
#include "freeout.h"

/****************************************************************/
/********** print out query structures, for debugging ***********/

/** little utility for indentation **/

void indent (int ival)
{ int i ; for (i = 0 ; i < ival ; ++i) freeOut ("  ") ; }

/************ Query **************/

void tqOutQuery (TqQuery *x)
{ 
  freeOutf ("Query parse for \"%s\"\n", x->text) ;
  tqOutTable (x->table, 1) ;
}

/************ Table **************/

static char* tableOpName[] = { 0, "tOR", "tAND", "tDIFF" } ;
static char* orderTypeName[] = { 0, "ASC", "DESC" } ;

void tqOutTable (TqTable *x, int ival)
{ 
  if (!x) return ;

  indent (ival) ;
  if (x->keyName)
    if (x->isAssign)
      { freeOutf ("table assign $%s\n", x->keyName) ;
	indent (ival) ;
      }
    else
      freeOutf ("table $%s\n", x->keyName) ;
  if (x->sfw)
    tqOutSFW (x->sfw, x, ival) ;
  if (x->op)
    { freeOutf ("table %s\n", tableOpName[x->op]) ;
      tqOutTable (x->left, ival+1) ;
      tqOutTable (x->right, ival+1) ;
    }
  if (x->isActive)
    freeOut ("  ACTIVE\n") ;
  if (x->order)
    { TqOrder *o ;
      freeOut ("  ORDER_BY") ;
      for (o = x->order ; o ; o = o->nxt)
	{ freeOutf (" %s", o->fieldName) ;
	  if (o->type)
	    freeOutf (" %s", orderTypeName[o->type]) ;
	  if (o->nxt) 
	    freeOut (",") ;
	}
      freeOut ("\n") ;	    
    }

  tqOutTable (x->nxt, ival) ;	      
}


/************ SFW ***************/

void tqOutSFW (TqSFW *x, TqTable *currTable, int ival)
{ 
  if (!x) return ;

  indent (ival) ; freeOut ("SELECT\n") ;
  tqOutField (x->select, currTable, ival+2) ;
  indent (ival+1) ; freeOut ("FROM\n") ;
  tqOutDecl (x->from, currTable, ival+2) ;
  indent (ival+1) ; freeOut ("WHERE\n") ;
  tqOutBool (x->where, currTable, ival+2) ;
}

/************ Field *************/

void tqOutField (TqField *x, TqTable *currTable, int ival)
{ 
  if (!x) return ;

  indent (ival) ;
  freeOut ("field") ;
  if (x->name)
    freeOutf ("  %s", x->name) ;
  freeOut ("\n") ;
  tqOutExpr (x->expr, currTable, ival+1) ;
  tqOutField (x->nxt, currTable, ival) ;
}

/************ Decl *************/

void tqOutDecl (TqDecl *x, TqTable *currTable, int ival)
{ 
  if (!x) return ;

  indent (ival) ;
  if (x->idName)
    freeOutf ("decl  %s  ", x->idName) ;
  else
    freeOutf ("decl  DEFAULT  ", x->idName) ;

  if (x->isActive)
    freeOut ("ACTIVE") ;
  if (x->className)
    freeOutf ("CLASS %s", x->className) ;
  if (x->loc)
    tqOutLoc (x->loc) ;
  freeOut ("\n") ;

  tqOutDecl (x->nxt, currTable, ival) ;
}

/************ Bool *************/

static char* boolOpName[] = { 0, "bNOT", "bOR", "bAND", "bXOR" } ;

static char* comparatorName[] = { 0,
   "uEQ", "kkEQ", "ksEQ", "skEQ", "ssEQ", "iiEQ", "ifEQ", "fiEQ", "ffEQ",
   "uNE", "kkNE", "ksNE", "skNE", "ssNE", "iiNE", "ifNE", "fiNE", "ffNE",
   "uLT", "iiLT", "ifLT", "fiLT", "ffLT",
   "uLE", "iiLE", "ifLE", "fiLE", "ffLE",
   "uGE", "iiGE", "ifGE", "fiGE", "ffGE",
   "uGT", "iiGT", "ifGT", "fiGT", "ffGT" } ;

void tqOutBool (TqBool *x, TqTable *currTable, int ival)
{
  if (!x) return ;

  indent (ival) ;
  freeOut ("bool_expr") ;
  if (x->op)
    freeOutf ("  %s", boolOpName[x->op]) ; 
  if (x->comp)
    freeOutf ("  %s", comparatorName[x->comp]) ;
  if (x->idExists)
    freeOutf ("EXISTS Id %d=\"%s\"", 
	      x->idExists, dictName(currTable->idDict, x->idExists)) ;
  if (x->locExists)
    { freeOut ("EXISTS ") ;
      tqOutLoc (x->locExists) ;
    }

  if (x->exists)
    if (x->value)
      freeOut ("  Value: TRUE") ;
    else
      freeOut ("  Value: FALSE") ;
  freeOut ("\n") ;

  if (x->op)
    { tqOutBool (x->left.bool, currTable, ival+1) ;
      tqOutBool (x->right.bool, currTable, ival+1) ;
    }
  if (x->comp)
    { tqOutExpr (x->left.expr, currTable, ival+1) ;
      tqOutExpr (x->right.expr, currTable, ival+1) ;
    }
}

/************ Expr *************/

static char* exprOpName[] = { 0, 
  "kID", "kNAME", "kCLASS",
  "uUMINUS", "iUMINUS", "fUMINUS",
  "uPLUS", "uMINUS", "uTIMES", "uDIV",
  "iiPLUS", "iiMINUS", "iiTIMES", "iiDIV",
  "ifPLUS", "ifMINUS", "ifTIMES", "ifDIV",
  "fiPLUS", "fiMINUS", "fiTIMES", "fiDIV",
  "ffPLUS", "ffMINUS", "ffTIMES", "ffDIV",
  "dYEARDIFF", "dMONTHDIFF", "dWEEKDIFF", "dDAYDIFF", 
  "dHOURDIFF", "dMINDIFF", "dSECDIFF" } ;

static char* tableFuncName[] = { 0, "COUNT", "FIRST", "LAST", "SUM", 
				  "MIN", "MAX", "AVG" } ;

void tqOutExpr (TqExpr *x, TqTable *currTable, int ival)
{
  if (!x) return ;

  indent (ival) ;
  freeOut ("expr") ;
  if (x->id)
    freeOutf ("  Id %d=\"%s\"", x->id, dictName(currTable->idDict, x->id)) ;
  if (x->loc)
    { freeOut ("  ") ; tqOutLoc (x->loc) ; }
  if (x->tableFunc)
    freeOutf ("  %s", tableFuncName[x->tableFunc]) ;
  if (x->op)
    freeOutf ("  %s", exprOpName[x->op]) ;
  if (x->type)
    freeOutf ("  Type: %c", x->type) ;
  if (x->exists)
    { freeOut ("  Value: ") ;
      switch (x->type)
	{ 
	case 'k': case 'g':
	  freeOutf ("%s:%s", className(x->value.k), name(x->value.k)) ;
	  break ;
	case 'i': freeOutf ("%d", x->value.i) ; break ;
	case 'f': freeOutf ("%g", x->value.f) ; break ;
	case 's': freeOutf ("\"%s\"", x->value.s) ; break ;
	case 't': freeOut (timeShow (x->value.t)) ; break ;
	}
    }
  freeOut ("\n") ;

  if (x->tableFunc)
    tqOutTable (x->table, ival+1) ;
  if (x->op)
    { tqOutExpr (x->left, currTable, ival+1) ;
      tqOutExpr (x->right, currTable, ival+1) ;
    }
}

/************ Loc *************/

void tqOutLoc (TqLoc *x)
{
  freeOut ("LOCATOR ") ;
  if (x->idString)
    freeOut (x->idString) ;
  if (x->obClassName)
    freeOutf ("%s:%s", x->obClassName, x->obName) ;
  for (x = x->nxt ; x ; x = x->nxt)
    if (x->relPos)
      freeOutf (" [%d]", x->relPos) ;
    else if (x->localTagName)
      freeOutf (" . %s", x->localTagName) ;
    else if (x->followTagName)
      freeOutf (" -> %s", x->followTagName) ;
}

/******************** end of file *******************/
 
