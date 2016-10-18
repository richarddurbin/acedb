/*  File: vtxt.c
 *  Author: Jean Thierry-Mieg (mieg@kaa.crbm.cnrs-mop.fr)
 *  Copyright (c) J Thierry-Mieg and R Durbin, 1999
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
 *	Jean Thierry-Mieg (CRBM du CNRS, France) mieg@kaa.crbm.cnrs-mop.fr
 *
 * Description: Contains variable length text interface
                to simplify the preparation of text documents
                The code is implemented over the acedb stack functions
 * Exported functions: see wh/vtxt.h
 * HISTORY:
 * Created: Thu May 8 2003: mieg
 *-------------------------------------------------------------------
 */

#include <wh/vtxt.h>

/*********************************************************************/
/* Typical usage
 *
 * we use recursive calls vtxt to fill 
 * chapters, paragraphs, sentences
 * and in case of success, we print the title
 * and return the whole thing


f1 (vTXT v1)
{
  vTXT v2 = vtxtCreate () ;
  if (ptr = f2 (s2,..)) // return 0 or stackText (s2, 0)
    {
      vtxtPrintf (v1, "Title\n") ;
      vtxtPrint (v1, ptr) ; // unformated, so we do not interpret % and \ a second time
    }
  vtxtDestroy (v2) ; // allways needed
}
*/

/*********************************************************************/
/* note that vtxtDestroy is a macro for stackDestroy */
vTXT vtxtCreate (void)
{
  Stack s = stackCreate (1024) ;
  
  stackTextOnly (s) ;
  return s ;
} /* vtxtCreate */

/*********************************************************************/
/* note that vtxtDestroy is a macro for stackDestroy */
vTXT vtxtHandleCreate (STORE_HANDLE h)
{
  Stack s = stackHandleCreate (1024, h) ;
  
  stackTextOnly (s) ;
  return s ;
} /* vtxtCreate */

/*********************************************************************/

BOOL vtxtClear (vTXT s)
{
  stackClear (s) ;
  return  stackExists(s) ;
} /* vtxtPtr */

/*********************************************************************/

char *vtxtPtr (vTXT s)
{
  return stackExists(s) && stackMark (s)? stackText (s, 0) : 0 ;
} /* vtxtPtr */

/*********************************************************************/

int vtxtLen (vTXT s)
{
  return stackExists(s) ? stackMark (s) : 0 ;
} /* vtxtLen */

/*********************************************************************/
/* the content strating at pos*/
char *vtxtAt (vTXT s, int pos) 
{
  char *ptr = vtxtPtr (s) ;
  return (ptr && vtxtLen (s) > pos) ? ptr + pos : 0 ;
} /* vtxtAt */

/*********************************************************************/
/* change a into b in the whole vTXT 
 * possibly realloc (therefore invalidates previous vtxtPtr return values
 *
 * returns the number of replaced strings
 */

int vtxtReplaceString (vTXT vtxt, char *a, char *b)
{
  char *cp, *cq, *buf = 0 ;
  int nn = 0 ;

  cp = vtxt ? vtxtPtr (vtxt) : 0 ;
  cq = cp && a ? strstr (cp, a) : 0 ;

  if (!cq)
    return 0 ;
  buf = cp = strnew (cp, 0) ;
  vtxtClear (vtxt) ;
  while ((cq = strstr (cp, a)))
    {
      nn++ ;
      *cq = 0 ;
      vtxtPrint (vtxt, cp) ;
      if (b && *b) vtxtPrint (vtxt, b) ;
      cp = cq + strlen (a) ;
    }
  vtxtPrint (vtxt, cp) ;
  messfree (buf) ;

  return nn ;
} /* vtxtReplaceString */

/*********************************************************************/
/* removes all occurences of begin...end,
 *
 * returns the number of removed strings
 */

int vtxtRemoveBetween (char *txt, char *begin, char *end)
{
  char *cp, *cq, *cr, *cs ;
  int nn = 0 ;

  cp = txt && *txt && begin && *begin && end && *end 
    ? strstr (txt, begin) : 0 ;

  while (cp && (cq = strstr (cp, end)))
    {
      nn++ ;
      cr = cp ; cs = cq + strlen (end) ;
      while ((*cp++ = *cr++)) ;
      cp = strstr (cp, end) ;
    }
  return nn ;
} /* vtxtRemoveBetween */

/*********************************************************************/

void vtextUpperCase (char *blkp) /* acts on a normal char buffer */
{
  char *cp = blkp ;
  while (*cp) { *cp = freeupper(*cp) ; cp++; }
} /* vtextUpperCase */

/*********************************************************************/

void vtextLowerCase (char *blkp) /* acts on a normal char buffer */
{
  char *cp = blkp ;
  while (*cp) { *cp = freelower(*cp) ; cp++; }
} /* vtextLowerCase */

/*********************************************************************/
/* equivalent to vstrFindReplaceSymbols(sBuf,sBuf,0,"\t \n"," ",0,1,1); */
/* change \n\t into space, collapse multiple spaces into single one
 * removes starting and trailing spaces 
 */
void vtextCollapseSpaces (char *buf) /* acts on a normal char buffer */
{
  char *cp, *cq ;
  BOOL isWhite = TRUE ; /* to remove starting spaces */
  
  cp = cq = buf ; cp-- ;
  if (buf)
    while (*++cp)
      switch (*cp)
	{
	case ' ': case '\n': case '\t':
	  if (!isWhite) *cq++ = ' ' ;
	  isWhite = TRUE ;
	  break ;
	default:
	  if (cp != cq) *cq = *cp ;
	  cq++ ; 
	  isWhite = FALSE ;
	  break ;
	}
  *cq = 0 ;
  while (--cq > buf && *cq == ' ') *cq = 0 ; /* remove trailing space */
} /* vtextCollapseSpaces  */

/*********************************************************************/
/* change a into b in the whole of buf */
void vtextReplaceSymbol (char *buf, char a, char b) /* acts on a normal char buffer */
{
  char *cp, *cq ;
  
  cp = cq = buf ; cp-- ;
  if (cp)
    while (*++cp)
      {
	if (*cp == a)
	  { if (b) *cq++ = b ; }
	else
	  {
	    if (cp != cq) *cq = *cp ;
	    cq++ ;
	  } 
      }
  *cq = 0 ;
} /* vtextReplaceSymbol */

/*********************************************************************/
/* removes html markup, i.e. everything inside <> */
void vtxtCleanHtml (vTXT vtxt)
{
  char *cp, *cq, *cp0 ;
  int inHtml = 0 ;
  int inQuote = 0 ;
  int i ;
  
  cp = cq = cp0 = vtxtPtr (vtxt) ; cp-- ;
  if (cq && *cq)
    while (*++cp)
      {
	if (!inQuote && *cp == '<')
	  inHtml++ ;
	if (*cp == '\"')
	  {
	    for (i = 1 ; cp - i >= cp0 && *(cp-i) == '\\' ; i++) ;
	    if (i%2)  /* distinguish "  \"  \\" \\\" */
	      inQuote = 1 - inQuote ;
	  }
	if (!inHtml)
	  *cq++ = *cp ; /* copy one char */
	if (inHtml && !inQuote && *cp == '>')
	  inHtml-- ;
      }
  while (cp >= cq) *cp-- = 0 ;
} /* vtextCleanHtml */

/*********************************************************************/
/* Unformatted print, appends to what is already there */

int vtxtPrint (vTXT s, char *txt)
{
  int nn, len ;
  char *cp ;
  
  nn = stackMark (s) ;
  if (txt && (len = strlen (txt)))
    {
      /* programmer's note
       * this function differs from catText or catBinary
       * if the stack is empty and we vtxtPrint (s,"aa")
       * as a result we have aa0  (zero terminated)
       * and s->ptr, used for the next vtxtPrint or vtxtPrintf
       * points on that zero
       * whereas in catBinary, we point on the 4th char, after the zero
       * but the next catBinary or catText will move back one char
       * so the 2 types of calls should not be mixed
       * although a VTXT is implemented as a Stack
       */
      stackExtend (s, len + 1) ;
      cp = stackText (s, nn) ;
      
      memcpy (cp, txt, len) ;
      *(cp + len) = 0 ;
      s->ptr += len ;
    }

  return nn ;
} /* vtxtPrint */

/*********************************************************************/
/* General formatted printf, appends to what is already there */
int vtxtPrintf (vTXT s, char * format, ...)
{
  int nn, len ;
  char *cp ;
  
  va_list ap;
  va_start(ap, format);
  len = utPrintfSizeOfArgList (format, ap) ;
  va_end (ap) ;

  stackExtend (s, len + 1) ;
  nn = stackMark (s) ;
  cp = stackText (s, nn) ;

  va_start(ap, format);
  len = vsprintf(cp, format, ap) ;
  va_end (ap) ;

  s->ptr += len;
  return nn ;
} /* vtxtPrintf */

char *vtxtPrintWrapped (vTXT s, char *text, int lineLength) 
{
  char cc, *cp, *cq ;
  int ii ;

  cp = text ;
  while (*cp)
    {
      ii = lineLength ; cq = cp ;
      while (ii-- && *cq) cq++ ;
      cc = *cq ; *cq = 0 ;
      vtxtPrintf (s, "%s%s", cp, cc ? "\n" : "") ;
      *cq = cc ;
      cp = cq ;
    }
  return stackText (s, 0) ;
}

/*
asndump.c:830: warning: implicit declaration of function `vstrPrintfWrapped'
asndump.c:1235: warning: implicit declaration of function `vstrCleanEnds ~  vtextCollapseSpaces'
asndump.c:3090: warning: implicit declaration of function `vstrSeparateSubstringFromString'
asndump.c:5106: warning: implicit declaration of function `vstrClnMarkup'
asndump.c:5108: warning: implicit declaration of function `vstrFindReplaceStrings'
*/

/*********************************************************************/
/*********************************************************************/
