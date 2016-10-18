/*  File: texthelp.c
 *  Author: Friedemann Wobus (fw@sanger.ac.uk)
 *          and contributions from Darren Platt (daz@sanger.ac.uk)
 * -------------------------------------------------------------------
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * -------------------------------------------------------------------
 * This file is part of the ACEDB genome database package, written by
 * 	Richard Durbin (MRC LMB, UK) rd@mrc-lmb.cam.ac.uk, and
 *	Jean Thierry-Mieg (CRBM du CNRS, France) mieg@kaa.cnrs-mop.fr
 *
 * Description:
     contains contains the code to display an HTML page as plain text
     Basic formatting is observed, but images and links are stripped.

 * Exported functions:
 **      helpPrint(char *helpFilename);
 * HISTORY:
 * Last edited: Nov  4 14:16 1999 (fw)
 * * Nov  4 14:01 1999 (fw): use ACEOUT for server
 * * Oct  8 16:01 1998 (fw): removed the declaration of 
			helpOn and help for #define MACINTOSH
 * * Aug 20 16:10 1998 (rd): removed refernces to old help-system
 * * Aug 18 17:17 1998 (fw): help-system split into 
		w1/helpsubs.c  (helpDir, HTML stuff etc).
		w1/texthelp.c  (non-graphical help for tace)
		w2/graphhelp.c (graphical help for xace,image etc.)
 * ----------------------------------------------------------------------
 * ---- major rework, these revision don't necessarily 
 * ---- affect code still left in this file
 * ----------------------------------------------------------------------
 * * May  2 01:07 1996 (rd): new implementation of 
                       helpMakeIndex() using filDirectory()
 * * May  2 18:24 1996 (mieg):
         fall back on oldhelp
         callMosaic if (http:)
         use freeout for server
         jaime's file name rotation
         remaining problem: help topic in tace should be case-insensitive
 * * May  1 18:24 1996 (fw): fixed freePage() to avoid mem leaks
 * * Apr 30 16:18 1996 (fw): fixed #ifdef NON_GRAPHICs for tace
 * * Apr 30 16:18 1996 (fw): added image dictionary
 * * Apr 29 12:37 1996 (fw): added handling of <DL> lists
 * * Apr 25 16:27 1996 (fw): added <IMG > tag
 * * Apr 22 17:43 1996 (fw): changed help system to HTML browser
 * Created: Thu Feb 20 14:49:50 1992 (mieg)
 *-------------------------------------------------------------------
 */

/* $Id: texthelp.c,v 1.20 2002-02-22 18:00:21 srk Exp $ */

#ifndef MACINTOSH
/********************************************************************/
#include "help_.h"
#include "aceio.h"
/********************************************************************/

typedef struct TextHelpStruct 
{ 
  ACEOUT fo;
  float xPos ;
  int indent ;
  int WINX ;
  char buf[10000] ;	/* text-buffer for wordwrapping */
  BOOL MODE_PREFORMAT ;
  BOOL MODE_HREF ;
  BOOL MODE_HEADER ; 
  BOOL FOUND_NOBULLET_IN_LIST_NOINDENT ;
  int itemNumber ;
  char *currentLink ;
} *TextHelp ;

static void htmlPagePrint (TextHelp th, HtmlPage *page) ;
static void printTextSection (TextHelp th, HtmlNode *node);

/********************************************************************/

/* dumps out help-page without images and markups */
BOOL helpPrint (char *helpFilename, void *user_pointer)
/* returns TRUE if a help page could successfully be displayed
   for the given subject, returns FALSE if no such page found */
{
  HtmlPage *page ;
  FilDir dirList;
  char *cp;
  int i,n,x;
  TextHelp th;
  ACEOUT help_out = (ACEOUT)user_pointer;

  th = (TextHelp) messalloc (sizeof (struct  TextHelpStruct)) ;
  th->fo = (ACEOUT)user_pointer;

  if ((page = htmlPageCreate (helpFilename, 0)))
    {
      /* found a page */
      htmlPagePrint (th, page);
      
      htmlPageDestroy (page);
      
      messfree (th) ;
      return TRUE;
    }
  
  if (!helpFilename)
    aceOutPrint (help_out, "Help subject not found\n");
  else
    aceOutPrint (help_out, "Help subject is ambiguous\n");
  
  aceOutPrint (help_out, "Try:\n  help\n");
  
  /* now show a list of possible files */
  if(!(dirList = filDirCreate(helpGetDir(), "html", "r")) )
    {
      messerror ("Can't open help directory %s\n"
	       "(%s)",
	       helpGetDir(), messSysErrorText()) ;
      messfree (th) ;
      return FALSE ;
    }
  
  for (i = 0, x = 0 ; i < filDirMax(dirList) ; i++)
    {
      cp = filDirEntry(dirList, i) ;
      if (!cp || !*cp || !strlen(cp))
	continue ;
      if (helpFilename)
	{
	  if (strncasecmp(filGetFilename(helpFilename),cp,
			  strlen(filGetFilename(helpFilename))) != 0)
	    continue;
	}
      
      n = strlen(cp) ;
      if (n > 5 && !strcmp(".html", cp + n - 5))
	*(cp + n - 5) = 0 ; 
      
      x += n + 1 ;
      if (x > 50)
	{ x = n + 1 ; aceOutPrint (help_out, "\n") ;}
      aceOutPrint (help_out, "%s  ", cp) ;
    }
  aceOutPrint (help_out, "\n") ;
  
  messfree (dirList);
  
  messfree (th) ;
  return FALSE;
} /* helpPrint */


/************************************************************/
/* counter-part to graphWebBrowser(), which remote-controls 
   netscape using the -remote command line option. Useful
   for textual applications running in an X11 environment,
   where x-apps can be called from within the application,
   but the Xtoolkit (used to drive netscape via X-atoms)
   shouldn't be linked in, because it is a textual app. */
/************************************************************/
BOOL helpWebBrowser(char *link)
{
  /* currently impossible, because it is hard to find out whether
     a netscape process is already running.
     Stupidly enough 'netscape -remote...' doesn't exit
     with code 1, if it can't connect to an existing process
*/
  return FALSE;
} /* helpWebBrowser */

/************************************************************/
/******************                   ***********************/
/****************** static functions  ***********************/
/******************                   ***********************/
/************************************************************/

static void htmlPagePrint (TextHelp th, HtmlPage *page)
{
  /* init screen-position parameters */

  th->WINX = 80 ;
  th->indent = 2 ;
  th->xPos = th->indent ;

  /* start recursively printing nodes */
  printTextSection (th, page->root) ;

  return;
} /* htmlPagePrint */

/************************************************************/

static void newTextLine (TextHelp th)
{
  int i ;
  
  /*  if (th->xPos != indent)*/
  {
    aceOutPrint (th->fo, "\n") ;
    for (i = 0; i < th->indent; ++i) aceOutPrint (th->fo, " ") ;
    th->xPos = th->indent ;
  }

  return;
} /* newTextLine */

/************************************************************/ 
static void blankTextLine (TextHelp th)
{
  int i ;

  aceOutPrint (th->fo, "\n") ;
  for (i = 0; i < th->indent; ++i) aceOutPrint (th->fo, " ") ;
  th->xPos = th->indent ;

  newTextLine (th) ;

  return;
} /* blankTextLine */
/************************************************************/

static void printTextSection (TextHelp th, HtmlNode *node)
/* part specific to the text-help system, which uses freeOut
   to print arsed HTML as plain text */
{
  int i, len ;
  char *cp, *start ;

  switch (node->type)
    {
    case HTML_SECTION:
      printTextSection (th, node->left) ;
      if (node->right) printTextSection (th, node->right) ;
      break ;
    case HTML_COMMENT:
      /* do nothing */
      break ;
    case HTML_DOC:
    case HTML_HEAD:
    case HTML_BODY:
      if (node->left) printTextSection (th, node->left) ;
      break ;
    case HTML_TITLE:
      for (i = 0; i < strlen(node->text)+4; ++i)
	aceOutPrint (th->fo, "*") ;
      aceOutPrint (th->fo, "\n* %s *\n", node->text) ;
      for (i = 0; i < strlen(node->text)+4; ++i)
	aceOutPrint (th->fo, "*") ;
      blankTextLine(th) ;
      break ;
    case HTML_HEADER:
      {
	th->MODE_HEADER = TRUE ;

	th->indent = node->hlevel*2 ;

	blankTextLine (th) ;
	
	/* check, in case some bozo has done a thing like <H1></H1> */
	if (node->left) printTextSection (th, node->left) ; 

	aceOutPrint (th->fo, "\n") ;
	for (i = 0; i < th->xPos; ++i)
	  {
	    if (i < th->indent) aceOutPrint (th->fo, " ") ;
	    else  aceOutPrint (th->fo, "*") ;
	  }

	blankTextLine (th) ;
	
	th->MODE_HEADER = FALSE ;
      }
      break ;

    case HTML_LIST:
      if (node->lstyle == HTML_LIST_BULLET || 
	  node->lstyle == HTML_LIST_NUMBER)
	th->indent += 2 ;
      else if (node->lstyle == HTML_LIST_NOINDENT)
	th->indent -= 2 ;
      newTextLine (th) ;

      th->itemNumber = 0 ;

      /* a list might not have a leftnode (a list item) */
      if (node->left) printTextSection (th, node->left) ;

      if (node->lstyle == HTML_LIST_BULLET || 
	  node->lstyle == HTML_LIST_NUMBER)
	th->indent -= 2 ;
      else if (node->lstyle == HTML_LIST_NOINDENT)
	th->indent += 2 ;
      if (node->lstyle == HTML_LIST_NOINDENT &&
	  th->FOUND_NOBULLET_IN_LIST_NOINDENT)
	{
	  th->indent -= 4 ;
	  th->FOUND_NOBULLET_IN_LIST_NOINDENT = FALSE ;
	}

      blankTextLine (th) ;
      break ;

    case HTML_LISTITEM:
      ++(th->itemNumber) ;
      if (node->left)
	{
	  if (node->lstyle == HTML_LIST_NOINDENT_NOBULLET)
	    {
	      /* if we are in a <DL> list and went to indentation
		 because of a <DD> item, a <DT> item brings back
		 the old indent-level (noindent for <DL>'s) */
	      if (th->FOUND_NOBULLET_IN_LIST_NOINDENT)
		{
		  th->indent -= 6 ;
		  th->FOUND_NOBULLET_IN_LIST_NOINDENT = FALSE ;
		  newTextLine (th) ;
		  aceOutPrint (th->fo, "  ") ;
		}
	    }
	  else
	    newTextLine (th) ;
	  if (node->lstyle == HTML_LIST_BULLET ||
	      node->lstyle == HTML_LIST_NOINDENT)
	    {
	      aceOutPrint (th->fo, "* ") ;
	      th->indent += 2 ;
	      th->xPos  = th->indent ;
	    }
	  else if (node->lstyle == HTML_LIST_NUMBER)
	    {
	      aceOutPrint (th->fo, "%d. ", th->itemNumber) ;
	      th->indent += strlen(messprintf ("%d. ", th->itemNumber)) ;
	      th->xPos  = th->indent ;
	    }
	  else if (node->lstyle == HTML_LIST_NOBULLET)
	    {
	      /* part of a <DL> noindented list, but a <DD>
		 item becomes indented, but no bullet */
	      /* if we come across the first NO_BULLET item, in
		 a LIST_NOINDENT, the LIST becomes indented */
	      if (!th->FOUND_NOBULLET_IN_LIST_NOINDENT)
		{
		  th->indent += 6 ;
		  th->xPos = th->indent ;
		  aceOutPrint (th->fo, "      ") ;
		  fflush (stdout) ;

		  th->FOUND_NOBULLET_IN_LIST_NOINDENT = TRUE ;
		}
	    }
	  printTextSection (th, node->left) ;
	}
      if (node->lstyle == HTML_LIST_BULLET ||
	  node->lstyle == HTML_LIST_NOINDENT)
	{
	  th->indent -= 2 ;
	}
      else if (node->lstyle == HTML_LIST_NUMBER)
	{
	  th->indent -= strlen(messprintf ("%d. ", th->itemNumber)) ;
	}
      else if (node->lstyle == HTML_LIST_NOBULLET)
	{
	  if (!th->FOUND_NOBULLET_IN_LIST_NOINDENT)
	    th->indent -= 6 ;
	}
      
      if (node->right)
	{
	  printTextSection (th, node->right) ;
	}
      break ;

    case HTML_HREF:
      if (node->link)
	{
	  th->MODE_HREF = TRUE ;
	  th->currentLink = node->link ;
	}
      /* we have to check for leftnode, in case we have a thing
	 like <A HREF=...></A>. The HREF-node doesn't have a TEXT
	 node attached, and it would crash otherwise */
      if (node->left)printTextSection (th, node->left) ; 

      if (node->link)
	{
	  th->MODE_HREF = FALSE ;
	  th->currentLink = 0 ;
	}
      break ;
    case HTML_TEXT:
      cp = node->text ;
      if (!th->MODE_PREFORMAT)
	htmlStripSpaces (node->text) ;
      /* for th->MODE_PREFORMAT keeps all controls chars */

      while (*cp)
	{
	  len = 0 ;
	  start = cp ;
	  
	  if (!th->MODE_PREFORMAT)
	    {
	      while (*cp && !isspace((int)*cp)) { ++(cp) ; ++len ; }
	      if (*cp) ++cp ;	/* skip whitespace */
	    }
	  else
	    {
	      while (*cp && *cp != '\n') { ++(cp) ; ++len ; }
	      if (*cp) 
		{
		  ++cp ;	/* skip RETURN */
		  ++len ;	/* so we copy the RETURN into buf */
		}
	    }

	  memset (th->buf, 0, 10000) ;
	  strncpy (th->buf, start, len) ;
	  th->buf[len] = 0 ;

	  /* linewrapping of words/lines longer than WINX */
	  if (strlen(th->buf) > th->WINX)
	    {
	      cp = start + (int)(th->WINX) ;
	      th->buf[(int)th->WINX] = 0 ;
	      len = (int)th->WINX ;
	    }
	  
	  /* word wrapping if not in preformatting mode */
	  if (!th->MODE_PREFORMAT)
	    {
	      if (th->xPos != th->indent)  /* not at start of line ... */
		{
		  th->xPos += 1 ;	   /* ... one space before the word */
		  aceOutPrint (th->fo, " ") ;
		  fflush (stdout) ;
		}
	      if (th->xPos + len > th->WINX)
		{
		  newTextLine (th) ;
		}
	      aceOutPrint (th->fo, "%s", th->buf) ;
	      th->xPos += strlen(th->buf) ; /* place th->xPos at the end of word */
	    }
	  else if (th->MODE_PREFORMAT)
	    {
	      int oldpos, stringpos, screenpos, ii ;
	      i = 0 ;

	      /* replace TABs with appropriate number of spaces */
	      while (th->buf[i])
		{
		  if (th->buf[i] == '\t')
		    {
		      /* oldpos is the position, that this TAB char
			 would go on the screen without TABifying
			 NOTE: xPos is always at least "indent" 
			 (to leave a left margin) */
		      oldpos = (th->xPos - th->indent) + i ;

		       /* screenpos is the position of the TAB char 
			  after inserting spaces
			  NOTE: the TAB itself will be
			  overwritten by one space */
		      screenpos = (((oldpos/8)+1)*8) - 1 ;

		       /* stringpos is where the TAB should go
			  in the string, where it'll turn into a space
			  at that position */
		      stringpos = screenpos - (th->xPos-th->indent) ;

		      /* shift all text from current position "i"
			 onwards */
		      for (ii = strlen(th->buf)-1; ii >= i ; --ii)
			th->buf[ii+(stringpos-i)] = th->buf[ii] ;

		      /* fill gap with spaces and also overwrite TAB
			 with a space */
		      for (ii = i; ii <= stringpos; ++ii)
			th->buf[ii] = ' ' ;

		      i = stringpos ;
		    }
		  ++i ;
		}

      /* don't use len, it might have changed when inserting spaces */
	      if (th->buf[strlen(th->buf)-1] == '\n')
		{
		  th->buf[strlen(th->buf)-1] = 0 ;
		  aceOutPrint (th->fo, "%s", th->buf) ;
		  th->xPos += strlen(th->buf) ;
		  newTextLine (th);	/* for the '\n' */
		}
	      else
		{
		  aceOutPrint (th->fo, "%s", th->buf) ;
		  th->xPos += strlen(th->buf) ;
		}
	    }
	}
      break ;
    case HTML_GIFIMAGE:
      {
	aceOutPrint (th->fo, " [IMAGE] ") ;
	th->xPos += 9 ;
      }
      break ;
    case HTML_NOIMAGE:
      break ;
    case HTML_RULER:
      {
	newTextLine (th) ;
	for (i = th->indent; i < th->WINX; ++i)
	  aceOutPrint (th->fo, "-") ;
	th->xPos = th->WINX ;
	newTextLine (th) ;
      }
      break ;
    case HTML_PARAGRAPH:
      blankTextLine (th) ;
      break ;
    case HTML_LINEBREAK:
      newTextLine (th) ;
      break ;
    case HTML_BOLD_STYLE:
    case HTML_STRONG_STYLE:
    case HTML_ITALIC_STYLE:
    case HTML_CODE_STYLE:
    case HTML_UNDERLINED_STYLE:
      {
	if (node->left)	printTextSection (th, node->left) ;
      }
      break ;
    case HTML_STARTBLOCKQUOTE:
      newTextLine (th) ;
      th->indent += 3 ;
      th->xPos = th->indent ;
      for (i = 0; i < th->indent; ++i) aceOutPrint (th->fo, " ") ;
      /* fflush (stdout) ; we deal with freeOutf, not with stdout */
      break ;
    case HTML_ENDBLOCKQUOTE:
      th->indent -= 3 ;
      blankTextLine (th) ;
      break ;
    case HTML_STARTPREFORMAT:
      th->MODE_PREFORMAT = TRUE ;
      newTextLine (th) ;
      break ;
    case HTML_ENDPREFORMAT:
      th->MODE_PREFORMAT = FALSE ;
      break ;
    case HTML_UNKNOWN:
      break;			/* compiler happiness */
    }

  return;
} /* printTextSection */
/************************************************************/

#endif /* !def MACINTOSH */
 
 
 
 
