/*  File: help.h
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
 * Description: part of the utility-library that handles the
          on-line help package.
	  The system works on the basis that all help files are HTML
	  documents contained in one directory. Depending on
	  what display function is registered, they can be shown
	  as text, using the built-in simple browser or even
	  dispatched to an external browser.
 * Exported functions: see below
 * HISTORY:
 * Last edited: Nov  4 14:40 1999 (fw)
 * Created: Thu Oct  8 14:01:07 1998 (fw)
 *-------------------------------------------------------------------
 */

#ifndef _HELP_H
#define _HELP_H

#include "regular.h"		/* basic header for util-lib */

/************** public routines of the help-package *******************/

typedef BOOL (*HelpRoutine)(char *subject, void *user_pointer);

struct helpContextStruct {
  HelpRoutine user_func;
  void *user_pointer;
};

BOOL helpOn (char *subject);
/* displays help on the given subject.  Dispatches to registered
   display function. defaults to helpPrint (text-help) */

struct helpContextStruct helpOnRegister (struct helpContextStruct);
/* register any func with ptr to display help-page */

char *helpSetDir (char *dirname);
/* set the /whelp/ dir if possible, returns path to it */

char *helpGetDir (void);
/* find the /whelp/ dir if possible, returns pointer to path
   If called for the first time without prior helpSetDir(),
   it will try to init to whelp/, but return 0 if it is not
   accessible*/


BOOL  helpPrint (char *helpFilename, void *user_pointer);
/* function for helpContextStruct passed to helpOnRegister */
/* dump helpfile as text - user_pointer must be of type ACEOUT */

BOOL  helpWebBrowser(char *link);
/* counter-part to graphWebBrowser(), which remote-controls 
   netscape using the -remote command line option. Useful
   for textual applications running in an X11 environment,
   where x-apps can be called from within the applcation,
   but the Xtoolkit (used to drive netscape via X-atoms)
   shoiuldn't be linked in, because it is a textual app. */


char *helpSubjectGetFilename (char *subject);
/* Returns the complete file name of the html help
     file for a given subject. 
   Returns ? if subject was ? to signal, 
     that a dynamically created index
     or some kind of help should be displayed.
   Returns NULL of no helpfile is available. */

char *helpLinkGetFilename (char *link_href);
/* given a relative link in a page it returns the full
   pathname to the file that is being linked to.
   The pointer returned belongs to an internal static copy
   that is reused every tjis function is called */

#endif /* !def _HELP_H */
