%{
/*  File: tqparse.l
 *  Author: Stefan Wiesmann and Richard Durbin (wiesmann,rd@sanger.ac.uk)
 *  Copyright (C) J Thierry-Mieg and R Durbin, 1996
 *-------------------------------------------------------------------
 * This file is part of the ACEDB genome database package, written by
 * 	Richard Durbin (MRC LMB, UK) rd@sanger.ac.uk, and
 *	Jean Thierry-Mieg (CRBM du CNRS, France) mieg@kaa.cnrs-mop.fr
 *
 * SCCS: $Id: tqparse.l,v 1.2 1996-11-05 22:10:28 rd Exp $
 * Description: lex for TQL
 * Exported functions:
 * HISTORY:
 * Last edited: Oct 29 00:01 1996 (rd)
 * Created: Tue Oct 29 00:01:02 1996 (rd)
 *-------------------------------------------------------------------
 */

/* # include "y.tab.h"  not necessary because C source will be included */

int lineno = 1 ;

/***** stuff to parse a string, not stdin *****/
/***** only valid for AT&T lex and derivatives ****/

#undef input			/* because macros for lex */
#undef unput

#define input() (*lexString++)
#define unput(ch) (lexString--)

%}

%%

SELECT 			return SELECT ;
FROM  			return FROM ;
WHERE   		return WHERE ;
ORDER_BY   		return ORDER_BY ;
AS   			return AS ;
ACTIVE   		return ACTIVE ;
CLASS   		return CLASS ;
OBJECT  		return OBJECT ;
EXISTS   		return EXISTS ;
NAME			return NAME ;
ID			return ID ;
NOW			return NOW ;
TODAY			return TODAY ;
NOT  			return NOT ;
AND  			return AND ;
OR   			return OR ;
XOR   			return XOR ;
DIFF   			return DIFF ;
TRUE   			return TRUEtok ;
FALSE  			return FALSEtok ;

YEARDIFF		{ yylval.Int = dYEARDIFF ; return DateFunc ; }
MONTHDIFF		{ yylval.Int = dMONTHDIFF ; return DateFunc ; }
WEEKDIFF		{ yylval.Int = dWEEKDIFF ; return DateFunc ; }
DAYDIFF			{ yylval.Int = dDAYDIFF ; return DateFunc ; }
HOURDIFF		{ yylval.Int = dHOURDIFF ; return DateFunc ; }
MINDIFF			{ yylval.Int = dMINDIFF ; return DateFunc ; }
SECDIFF			{ yylval.Int = dSECDIFF ; return DateFunc ; }

COUNT			{ yylval.Int = tCOUNT ; return TableFunc ; }
FIRST			{ yylval.Int = tFIRST ; return TableFunc ; }
LAST			{ yylval.Int = tLAST ; return TableFunc ; }
MIN			{ yylval.Int = tMIN ; return TableFunc ; }
MAX			{ yylval.Int = tMAX ; return TableFunc ; }
SUM			{ yylval.Int = tSUM ; return TableFunc ; }
AVG			{ yylval.Int = tAVG ; return TableFunc ; }

ASC   			{ yylval.Int = ASC ; return Ordering ; }
DESC   			{ yylval.Int = DESC ; return Ordering ; }

[A-Za-z][A-Za-z0-9_]*	{ yytext[yyleng-1] = 0 ;	/* don't understand ! */
			  yylval.String = strnew (yytext, qh) ;
			  return Identifier ;
			}

"//".*		;	 /* ignore comments */

[\;\:\.\$\(\)\[\]\+\-\*\/\,\_]	return yytext[0] ;

":="			return ASSIGN ;
"->"                    return ARROW ;

"="                     { yylval.Int = uEQ ; return Comparator ; }
"<"                     { yylval.Int = uLT ; return Comparator ; }
">"                     { yylval.Int = uGT ; return Comparator ; }
"!="                    { yylval.Int = uNE ; return Comparator ; }
"<="                    { yylval.Int = uLE ; return Comparator ; }
">="                    { yylval.Int = uGE ; return Comparator ; }

[0-9]+                  { sscanf (yytext, "%d", &yylval.Int) ;
                          return Number;
                        }

[0-9]+[eE][+-]?[0-9]+		|
[0-9]+"."[0-9]*[eE][+-]?[0-9]+	|
"."[0-9]*[eE][+-]?[0-9]+	|
[0-9]+"."[0-9]*			|
"."[0-9]+		{ sscanf (yytext, "%f", &yylval.Float) ;
			  return FloatLiteral ;
                        }

[0-2][0-9]:[0-5][0-9]:[0-5][0-9]	|
[0-2][0-9]:[0-5][0-9]			|
[0-2][0-9]              { yylval.String = strnew (yytext, qh) ;
                          return DatePart ;
                        }  

\"([^"]|\\["\n])*\"     { yytext[strlen(yytext)-1] = 0 ;
                          yylval.String = strnew (&yytext[1], qh) ;
                          return StringLiteral;
                        }

[ \t\r\f]+              ;  /* whitespace */

\n                      lineno++ ;

.                       yyerror ("invalid character") ;

%%

void yyerror (char *s)
{
    printf ("%d: %s at %s\n", lineno, s, yytext);
}

 
 
