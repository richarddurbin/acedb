/*  File: longtext.c
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
 * Description: ace-kernel functions to handle longtext data
 * Exported functions:
 * HISTORY:
 * Last edited: Nov 30 17:59 2001 (edgrif)
 * * Sep 15 15:22 1999 (fw): srk patch parsing empty longtext object
 *              deletes associated object
 * * Jan 28 09:19 1999 (fw): moved graphical part to longtextdisp.c
 * * Oct 15 15:18 1992 (mieg): Editing 
 * Created: Fri Mar 13 15:06:53 1992 (mieg)
 *-------------------------------------------------------------------
 */

/* $Id: longtext.c,v 1.36 2001-12-03 11:08:57 edgrif Exp $ */

#include "acedb.h"
#include "aceio.h"
#include <ctype.h>
#include "a.h"
#include "lex.h"
#include "whooks/sysclass.h"
#include "whooks/classes.h"
#include "pick.h"  /* needed in keysetdump */
#include "java.h"
#include "query.h"

/*****************************************************************/
/******************     non graphic routines *********************/
/*****************************************************************/

/* we could compress on the fly in dump, 
   but this is not longtetx specific 

  FILE *pipe ;
  extern FILE* popen() ;
  a = arrayCreate(stackMark(s), char) ;
  cp = cp0 = arrayp(a,0,char) ;
  stackCursor(s,0) ;
  pipe = popen ("compress", "rw") ;
  if (pipe)
    { register int i ; 
      while (cp = stackNextText(s))
	fprintf(pipe, "%s\n", cp) ;
      fprintf(pipe, "\n***LongTextEnd***\n") ;
      while((i = getc(pipe)) != EOF)
	*cp++ = i ;
      pclose(pipe) ;
      arrayMax(a) = cp - cp0 ;
      arrayStore(key, a,"c") ;
      stackDestroy(s) ;
      arrayDestroy(a) ;
      return TRUE ;
    }
 */    

BOOL longTextDump (ACEOUT dump_out, KEY k)
     /* DumpFuncType for class _VLongText */
{ 
  char  *cp ;
  Stack s = stackGet(k);

  if (s)
    {
      stackCursor(s,0) ;
      while ((cp = stackNextText(s)))
	{
	  aceOutPrint (dump_out, "%s", cp) ;
	  aceOutPrint (dump_out, "\n") ;
	}
    }
      

  aceOutPrint (dump_out, "***LongTextEnd***\n") ;

  stackDestroy(s) ;

  return TRUE ;
} /* longTextDump */

/************************************/

BOOL javaDumpLongText (ACEOUT dump_out, KEY key) 
{
  char *cp;
  Stack s = stackGet(key);

  if (!s)
    return FALSE ;
  stackCursor(s,0) ;

  aceOutPrint (dump_out, "?LongText?%s?\t?txt?", 
	       freejavaprotect(name(key)));

  while ((cp = stackNextText(s)) )
    { 
      aceOutPrint (dump_out, "%s", freejavaprotect(cp));
      aceOutPrint (dump_out, "\\n");
    }
  aceOutPrint (dump_out, "?\n\n");

  stackDestroy (s) ;

  return TRUE ;
} /* javaDumpLongText */

/************************************/

ParseFuncReturn longTextParse (ACEIN parse_io, KEY key, char **errtext)
     /* ParseFuncType */
{
  char *cp = 0 ;
  Stack s = stackCreate(500) ;
  BOOL foundEnd = FALSE ;
  ParseFuncReturn result ;

#ifdef ACEDB4
  stackTextOnly(s); /* since we save it. */
#endif

  aceInSpecial (parse_io, "\n\\") ;
  while ((cp = aceInCard (parse_io)))
    if (strcmp(cp,"***LongTextEnd***") == 0)
      { foundEnd = TRUE ;
	break ; 
      }
    else
      pushText(s, cp) ;

  if (!foundEnd)
    { messerror ("no ***LongTextEnd*** found when parsing LongText %s",
		 name(key)) ;
      return PARSEFUNC_ERR ;
    }
 
  s->textOnly = TRUE ;
  if (stackMark(s))
    {
      stackStore(key,s) ;
      result = PARSEFUNC_OK ;
    }
  else
    {
      /* srk - parsing an empty longtext object should
       * delete the associated object */
      arrayKill(key);
      result = PARSEFUNC_EMPTY ;
    }
  stackDestroy(s) ;

  return result ;
} /* longTextParse */

/************************************/

static BOOL longTextGrep (char *template, KEY key)
{
  char *cp = 0 ;
  Stack s ;
  static Stack s1 = 0 ;
  BOOL found = FALSE ;

  if (class(key) != _VLongText)
    return FALSE ;
      
  s = stackGet(key) ;
  if (!stackExists(s))
    return FALSE ;
  
  s1 = stackReCreate(s1, stackMark(s)) ;

  stackCursor(s, 0) ;
  while ((cp = stackNextText(s)))
    { catText(s1, cp) ;
      catText(s1, " ") ;
    }
  
  found = pickMatch(stackText(s1,0), template) ;

  stackDestroy(s) ;
  return found ;
}

/************************************/

KEYSET longGrep(char *template)
{ KEY key = 0 , key1 ;
  KEYSET ks = keySetCreate() ;
  int n = 0 ;
  
  while(lexNext(_VLongText, &key))
    { if (longTextGrep(template, key))
	{ if (!lexReClass(key, &key1, _VPaper))
	    key1 = key ;
	  keySet(ks, n++) = key1 ;
	}
    }
  keySetSort (ks) ;  /* needed because of ReClass calls */
  keySetCompress(ks) ;
  return ks ;
}

/************************** eof **********************************/

