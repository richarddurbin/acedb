/*  File: tqparse.y
 *  Author: Stefan Wiesmann and Richard Durbin (wiesmann,rd@sanger.ac.uk)
 *  Copyright (C) J Thierry-Mieg and R Durbin, 1996
 *-------------------------------------------------------------------
 * This file is part of the ACEDB genome database package, written by
 * 	Richard Durbin (MRC LMB, UK) rd@sanger.ac.uk, and
 *	Jean Thierry-Mieg (CRBM du CNRS, France) mieg@kaa.cnrs-mop.fr
 *
 * SCCS: $Id: tqparse.y,v 1.2 1996-11-05 22:10:06 rd Exp $
 * Description: yacc for TQL
 * Exported functions:
 * HISTORY:
 * Last edited: Nov  5 22:06 1996 (rd)
 * Created: Tue Oct 29 00:01:02 1996 (rd)
 *-------------------------------------------------------------------
 */
%{

#include "tq_.h"

#define zMake(type)	type* z = (type*)halloc(sizeof(type),qh) ; yyval.Ptr = z
#define zzFinal(x,type)	type* zz = (x) ; yyval.Ptr = zz ; while (zz->nxt) zz = zz->nxt
#define zzFinalMake(x,type)	zzFinal(x,type) ; zz->nxt = (type*)halloc(sizeof(type),qh) ; zz = zz->nxt

static STORE_HANDLE qh ;

void yyerror (char *s) ;

static  mytime_t timeMake (int i1, int i2, int i3, char *part)
{
  char buf[32] ;
  mytime_t t ;

  strcpy (buf, messprintf ("%d", i1)) ;
  if (i2 != -1) strcat (buf, messprintf ("-%d", i2)) ;
  if (i3 != -1) strcat (buf, messprintf ("-%d", i3)) ;
  if (part) strcat (buf, part) ;
  t = timeParse (buf) ;
  if (!t) yyerror ("Bad date/time literal") ;
  return t ;
}

%}
 
%union  {
  char		*String ;          
  int		 Int ;
  float		 Float ;
  int		 Command ;
  void		*Ptr ;		
}

%token <String>  Identifier
%token <Int>     Number
%token <Float>   FloatLiteral
%token <String>  StringLiteral
%token <String>  DatePart
%token <Int>     Comparator
%token <Int>	 DateFunc 
%token <Int>	 TableFunc 
%token <Int>	 Ordering 
%token <Command> SELECT FROM WHERE ORDER_BY TRUEtok FALSEtok
%token <Command> AS ACTIVE CLASS OBJECT EXISTS NAME ID
%token <Command> ARROW ASSIGN
%token <Command> NOW TODAY
%token <Command> AND OR XOR DIFF 

%left OR XOR
%left DIFF
%left AND
%left NOT
%left '+' '-'
%left '*' '/'
%nonassoc UMINUS

%%

query:        query_piece		{ zMake(TqQuery) ; z->handle = qh ; z->table = $<Ptr>1 ; }
            | query ';' query_piece	{ TqQuery *q ; zzFinal(q->table,TqTable) ; zz->nxt = $<Ptr>3 ; $<Ptr>$ = q ; }
              ;

query_piece: table_expr			{ $<Ptr>$ = $<Ptr>1 ; }
	    | '$' Identifier ASSIGN table_expr { TqTable *z = $<Ptr>4 ; z->keyName = $<String>2 ; z->isAssign = TRUE ; $<Ptr>$ = z ; }
	      ;

table_expr:   sfw 			{ zMake(TqTable) ; z->sfw = $<Ptr>1 ; }
	    | table_expr ORDER_BY sortlist { TqTable *z = $<Ptr>1 ; z->order = $<Ptr>3 ; $<Ptr>$ = z ; }
	    | '(' table_expr ')'	{ $<Ptr>$ = $<Ptr>2 ; }
	    | '$' Identifier 		{ zMake(TqTable) ; z->keyName = $<String>2 ; }
	    | ACTIVE			{ zMake(TqTable) ; z->isActive = TRUE ; }
/*
            | table_expr AND table_expr	{ zMake(TqTable) ; z->op = tAND ; z->left.expr = $<Ptr>1 ; z->right.expr = $<Ptr>3 ; }
            | table_expr OR table_expr	{ zMake(TqTable) ; z->op = tOR ; z->left.expr = $<Ptr>1 ; z->right.expr = $<Ptr>3 ; }
            | table_expr DIFF table_expr { zMake(TqTable) ; z->op = tDIFF ; z->left.expr = $<Ptr>1 ; z->right.expr = $<Ptr>3 ; }
*/
              ;

sfw:	      SELECT fieldlist FROM declarlist WHERE bool_expr { zMake(TqSFW) ; z->select = $<Ptr>2 ; z->from = $<Ptr>4 ; z->where = $<Ptr>6 ; }
            | SELECT fieldlist FROM declarlist { zMake(TqSFW) ; z->select = $<Ptr>2 ; z->from = $<Ptr>4 ; }
            | SELECT fieldlist WHERE bool_expr { zMake(TqSFW) ; z->select = $<Ptr>2 ; z->where = $<Ptr>4 ; }
            | SELECT fieldlist          { zMake(TqSFW) ; z->select = $<Ptr>2 ; }
/*
            | fieldlist FROM declarlist WHERE bool_expr { zMake(TqSFW) ; z->select = $<Ptr>1 ; z->from = $<Ptr>3 ; z->where = $<Ptr>5 ; }
            | fieldlist FROM declarlist { zMake(TqSFW) ; z->select = $<Ptr>1 ; z->from = $<Ptr>3 ; }
            | fieldlist WHERE bool_expr { zMake(TqSFW) ; z->select = $<Ptr>1 ; z->where = $<Ptr>3 ; }
            | fieldlist                 { zMake(TqSFW) ; z->select = $<Ptr>1 ; }
*/
	      ;

fieldlist:    field			{ $<Ptr>$ = $<Ptr>1 ; }
	    | fieldlist ',' field	{ zzFinal($<Ptr>1,TqField) ; zz->nxt = $<Ptr>3 ; }
              ;

field:	      expr			{ zMake(TqField) ; z->expr = $<Ptr>1 ; }
	    | Identifier ':' expr	{ zMake(TqField) ; z->name = $<Ptr>1 ; z->expr = $<Ptr>3 ; }
              ;

declarlist:   declaration		{ $<Ptr>$ = $<Ptr>1 ; }
            | declarlist ',' declaration { zzFinal($<Ptr>1,TqDecl) ; zz->nxt = $<Ptr>3 ; }
              ;
                
declaration:  locator			{ zMake(TqDecl) ; z->loc = $<Ptr>1 ; }
            | CLASS Identifier		{ zMake(TqDecl) ; z->className = $<Ptr>2 ; }
            | ACTIVE			{ zMake(TqDecl) ; z->isActive = TRUE ; }
            | declaration AS Identifier { TqDecl *z = $<Ptr>1 ; z->idName = $<String>3 ; $<Ptr>$ = z ; }
            | declaration Identifier    { TqDecl *z = $<Ptr>1 ; z->idName = $<String>2 ; $<Ptr>$ = z ; }
              ;

sortlist:     sort_criterion		{ $<Ptr>$ = $<Ptr>1 ; }
            | sortlist ',' sort_criterion { zzFinal($<Ptr>1,TqOrder) ; zz->nxt = $<Ptr>3 ; }
              ;
 
sort_criterion: Identifier		{ zMake(TqOrder) ; z->fieldName = $<String>1 ; }
	      | Identifier Ordering	{ zMake(TqOrder) ; z->fieldName = $<String>1 ; z->type = $<Int>2 ; }
                ; 
            
locator:      Identifier		{ zMake(TqLoc) ; z->idString = $<String>1 ; }
     	    | OBJECT '(' Identifier ',' StringLiteral ')'
					{ zMake(TqLoc) ; z->obClassName = $<String>3 ; z->obName = $<String>5 ; }
            | '.' Identifier		{ zMake(TqLoc) ; z->idString = "_DEFAULT" ; { zzFinalMake(z,TqLoc) ; zz->localTagName = $<String>2 ; } }
            | ARROW Identifier		{ zMake(TqLoc) ; z->idString = "_DEFAULT" ; { zzFinalMake(z,TqLoc) ; zz->followTagName = $<String>2 ; } }
            | locator '.' Identifier	{ zzFinalMake($<Ptr>1,TqLoc) ; zz->localTagName = $<String>3 ; }
            | locator ARROW Identifier	{ zzFinalMake($<Ptr>1,TqLoc) ; zz->followTagName = $<String>3 ; }
            | locator '[' Number ']'	{ zzFinalMake($<Ptr>1,TqLoc) ; zz->relPos = $<Int>3 ; }
              ;     

bool_expr:    TRUEtok			{ zMake(TqBool) ; z->value = TRUE ; z->exists = TRUE ; }
            | FALSEtok			{ zMake(TqBool) ; z->value = FALSE ; z->exists = TRUE ; }
            | EXISTS locator		{ zMake(TqBool) ; z->locExists = $<Ptr>2 ; }
            | expr Comparator expr	{ zMake(TqBool) ; z->comp = $<Int>2 ; z->left.expr = $<Ptr>1 ; z->right.expr = $<Ptr>3 ; }
            | NOT bool_expr		{ zMake(TqBool) ; z->op = bNOT ; z->left.bool = $<Ptr>2 ; $<Ptr>$ = z ; }
            | bool_expr AND bool_expr	{ zMake(TqBool) ; z->op = bAND ; z->left.expr = $<Ptr>1 ; z->right.expr = $<Ptr>3 ; }
            | bool_expr OR bool_expr	{ zMake(TqBool) ; z->op = bOR ; z->left.expr = $<Ptr>1 ; z->right.expr = $<Ptr>3 ; }
            | bool_expr XOR bool_expr	{ zMake(TqBool) ; z->op = bXOR ; z->left.expr = $<Ptr>1 ; z->right.expr = $<Ptr>3 ; }
            | '(' bool_expr ')'		{ $<Ptr>$ = $<Ptr>2 ; }
              ;

expr:         StringLiteral		{ zMake(TqExpr) ; z->type = 's' ; z->value.s = $<String>1 ; z->exists = TRUE ; }
            | Number       		{ zMake(TqExpr) ; z->type = 'i' ; z->value.i = $<Int>1 ; z->exists = TRUE ; }
            | FloatLiteral		{ zMake(TqExpr) ; z->type = 'f' ; z->value.f = $<Float>1 ; z->exists = TRUE ; }
            | locator          		{ zMake(TqExpr) ; z->loc = $<Ptr>1 ; }
            | locator '.' NAME		{ zMake(TqExpr) ; z->op = kNAME ; z->loc = $<Ptr>1 ; }
            | locator '.' ID		{ zMake(TqExpr) ; z->op = kID ; z->loc = $<Ptr>1 ; }
            | locator '.' CLASS		{ zMake(TqExpr) ; z->op = kCLASS ; z->loc = $<Ptr>1 ; }
	    | TableFunc '(' query ')'	{ zMake(TqExpr) ; z->tableFunc = $<Int>1 ; z->table = $<Ptr>3 ; }
            | DateFunc '(' date ',' date ')' { zMake(TqExpr) ; z->op = $<Int>1 ; z->left = $<Ptr>3 ; z->right = $<Ptr>5 ; }
            | expr '+' expr		{ zMake(TqExpr) ; z->op = uPLUS ; z->left = $<Ptr>1 ; z->right = $<Ptr>3 ; }
            | expr '-' expr		{ zMake(TqExpr) ; z->op = uMINUS ; z->left = $<Ptr>1 ; z->right = $<Ptr>3 ; }
            | expr '*' expr		{ zMake(TqExpr) ; z->op = uTIMES ; z->left = $<Ptr>1 ; z->right = $<Ptr>3 ; }
            | expr '/' expr		{ zMake(TqExpr) ; z->op = uDIVIDE ; z->left = $<Ptr>1 ; z->right = $<Ptr>3 ; }
            | '-' expr %prec UMINUS	{ zMake(TqExpr) ; z->op = uUMINUS ; z->left = $<Ptr>2 ; }
            | '(' expr ')'		{ $<Ptr>$ = $<Ptr>2 ; }
              ;

date:	      NOW			{ zMake(TqExpr) ; z->type = 't' ; z->value.t = timeParse ("now") ; z->exists = TRUE ; }
            | TODAY			{ zMake(TqExpr) ; z->type = 't' ; z->value.t = timeParse ("today") ; z->exists = TRUE ; }
	    | locator			{ zMake(TqExpr) ; z->loc = $<Ptr>1 ; }
	    | Number			{ zMake(TqExpr) ; z->type = 't' ; 
					  z->value.t = timeMake ($<Int>1, -1, -1, 0) ; z->exists = (z->value.t != 0) ; }
	    | Number '-' Number		{ zMake(TqExpr) ; z->type = 't' ;
					  z->value.t = timeMake ($<Int>1, $<Int>3, -1, 0) ; z->exists = (z->value.t != 0) ; }
	    | Number '-' Number '-' Number	
					{ zMake(TqExpr) ; z->type = 't' ;
					  z->value.t = timeMake ($<Int>1, $<Int>3, $<Int>5, 0) ; z->exists = (z->value.t != 0) ; }
	    | Number '-' Number '-' Number '_' DatePart 
					{ zMake(TqExpr) ; z->type = 't' ;
					  z->value.t = timeMake ($<Int>1, $<Int>3, $<Int>5, $<String>6) ; z->exists = (z->value.t != 0) ; }
              ;  

            
%%

/************ entry point: tqParse() and main() for debug ***********************/

static char *lexString ;

TqQuery* tqParse (char *text)
{
  TqQuery *q ;
  
  lexString = text ;
  yyparse () ;

  q = (TqQuery *) yyval.Ptr ;
  q->text = strnew (text, qh) ;

  return q ;
}

#ifndef ACEDB

#include "freeout.h"

extern FILE *yyin;

void main (int argc, char **argv)
{
  int i ;
  char buf[1000] ;
  
  freeOutInit() ;
  qh = handleCreate () ;

  *buf = 0 ;
  for (i = 1 ; i < argc ; ++i)
    { strcat (buf, argv[i]) ;
      if (i+1 < argc) strcat (buf, " ") ;
    }

  tqOutQuery (tqParse (buf)) ;

  handleDestroy (qh) ;
}

char *name (KEY x) { return messprintf ("key%x", x) ; }
char *className (KEY x) { return "noclass" ; }

#endif /* ndef ACEDB */

/***************** lex code ************************/

#include "lex.yy.c"

/***************** end of file **********************/
 
 
