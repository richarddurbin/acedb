/*  File: aceutils.c
 *  Author: Fred Wobus (fw@sanger.ac.uk)
 *  Copyright (c) J Thierry-Mieg and R Durbin, 1999
 *-------------------------------------------------------------------
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
 * Description: various highlevel utility functions for an acedb process
 * Exported functions: See acedb.h
 * HISTORY:
 * Last edited: Jul  6 10:58 2000 (edgrif)
 * Created: Wed Nov  3 15:09:10 1999 (fw)
 * CVS info:   $Id$
 *-------------------------------------------------------------------
 */

#include "acedb.h"
#include "aceio.h"

/******************************************************************/


/* used as user_func in context for messOutRegister,
 * when the message is to be written to ACEOUT in acedb-style */
void acedbPrintMessage (char *message, void *user_pointer)
{
  ACEOUT fo = (ACEOUT)user_pointer;
  char *string;
  char *start;
  char *cp;
  struct messContextStruct oldContext;
  struct messContextStruct nullContext = { NULL, NULL };

  /* It is possible that code called in this function
   * may call messout etc.
   * We need to make sure that we don't get back to this 
   * function to handle the output of messout, so
   * we de-register this function as the messout-handler
   * for the duration of this function.
   */
  oldContext = messOutRegister (nullContext);

  string = strnew (message, 0);
  start = string;
  cp = string;

  while (*cp)
    {
      if (*cp == '\n')
	{
	  *cp = '\0';
	  aceOutPrint (fo, "// %s\n", start);
	  start = cp+1;
	}
      ++cp;
    }

  /* I'm not even sure if this is necessary, the above code looks like it    */
  /* will stop when the last line is printed anyway...                       */
  if (*start)
    aceOutPrint (fo, "// %s\n", start); /* print last line */

  messfree (string);

  messOutRegister (oldContext);

  return;
} /* acedbPrintMessage */

/******************************************************************/

void simpleCrashRoutine (void)
     /* The least amount of cleanup work we need to do,
	this function is the default for the messcrashroutine,
	more elaborate functions will typically save the current
	session or close windows etc. */
{ 
  sessionReleaseWriteAccess() ;	/* must remove lock file */
  readlockDeleteFile ();	/* release readlock */

  filtmpcleanup () ;		/* deletes any temporary files made */

  messout ("NB: changes made since the last explicit save "
	   "have NOT been saved\n") ;
  return;
} /* simpleCrashRoutine */

/************************* eof ************************************/

