/*  File: helpsubs_.h
 *  Author: Fred Wobus (fw@sanger.ac.uk)
 *  Copyright (C) J Thierry-Mieg and R Durbin, 1998
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
 * 	Richard Durbin (MRC LMB, UK) rd@sanger.ac.uk, and
 *	Jean Thierry-Mieg (CRBM du CNRS, France) mieg@kaa.cnrs-mop.fr
 *
 * SCCS: %W% %G%
 * Description: private headerfile for the help-system.
 * Exported functions: none
 * HISTORY:
 * Last edited: May  4 16:06 1999 (fw)
 * * May  4 16:04 1999 (fw): removed HELP_FILE_EXTENSION, because
 *              we can have html and shtml...
 * * Oct  8 14:15 1998 (fw): renamed from helpsubs_.h to help_.h
 * * Oct  8 11:35 1998 (fw): introduced macro for HELP_FILE_EXTENSION
 * Created: Tue Aug 18 17:38:27 1998 (fw)
 *-------------------------------------------------------------------
 */

#ifndef UTIL_HELP__H
#define UTIL_HELP__H

#include "help.h"

#include <ctype.h>		/* for isspace etc.. */

/************************************************************/

/* forward declaration of struct type */
typedef struct HtmlPageStruct HtmlPage;
typedef struct HtmlNodeStruct HtmlNode;

/************************************************************/
/********** routines shared by the help-package *************/

HtmlPage *htmlPageCreate (char *helpFilename, STORE_HANDLE handle);
/* parse the HTML page for the given file */

HtmlPage *htmlPageCreateFromFile (FILE *fil, STORE_HANDLE handle);
/* parse the HTML source from an opened file */

void htmlPageDestroy (HtmlPage *page);
/* clear all memory taken up by the page, 
 * only wise if not allocated on a handle already */

void htmlStripSpaces (char *cp);
/* utility : remove whitespaces from free text in non-<PRE> mode */


/************************************************************/

typedef enum { 
  HTML_SECTION=1, 
  HTML_COMMENT, 
  HTML_DOC, 
  HTML_BODY, 
  HTML_HEAD,
  HTML_TITLE, 
  HTML_HEADER, 
  HTML_TEXT, 
  HTML_HREF, 
  HTML_RULER, 
  HTML_LINEBREAK, 
  HTML_PARAGRAPH, 
  HTML_LIST, 
  HTML_LISTITEM, 
  HTML_GIFIMAGE,
  HTML_BOLD_STYLE, 
  HTML_UNDERLINED_STYLE, 
  HTML_STRONG_STYLE, 
  HTML_ITALIC_STYLE, 
  HTML_CODE_STYLE,
  HTML_STARTPREFORMAT, 
  HTML_ENDPREFORMAT,
  HTML_STARTBLOCKQUOTE, 
  HTML_ENDBLOCKQUOTE,
  HTML_UNKNOWN, 
  HTML_NOIMAGE 
} HtmlNodeType ;
 
typedef enum {
  HTML_LIST_BULLET=1, 
  HTML_LIST_NUMBER, 
  HTML_LIST_NOINDENT, 
  HTML_LIST_NOBULLET, 
  HTML_LIST_NOINDENT_NOBULLET
} HtmlListType ;
/* a <UL> node and its <LI> items are LIST_BULLET
   a <OL> node and its <LI> items are LIST_NUMBER
   a <DL> node is LIST_NOINDENT,
               its <LI> node are also LIST_NOINDENT
               but <DD> items are LIST_NOBULLET
	       and <DT> items are LIST_NOINDENT_NOBULLET
*/

/************************************************************/


struct HtmlNodeStruct {
  HtmlNodeType type ;
  HtmlNode *left, *right ;
  char *text ;
  char *link ;
  int hlevel ;
  HtmlListType lstyle ;
  BOOL isNameRef ;
};


struct HtmlPageStruct {
  char *htmlText;		/* source text */
  HtmlNode *root;		/* root node of parsetree */
  STORE_HANDLE handle;
};


#endif /* !UTIL_HELP__H */
