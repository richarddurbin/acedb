/*  File: call.c
 *  Author: Richard Durbin (rd@sanger.ac.uk)
 *  Copyright (C) J Thierry-Mieg and R Durbin, 1994
 * -------------------------------------------------------------------
 * This file is part of the ACEDB genome database package, written by
 * 	Richard Durbin (MRC LMB, UK) rd@mrc-lmb.cam.ac.uk, and
 *	Jean Thierry-Mieg (CRBM du CNRS, France) mieg@kaa.cnrs-mop.fr
 *
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
 * -------------------------------------------------------------------
 * This file is part of the ACEDB genome database package, written by
 * 	Richard Durbin (MRC LMB, UK) rd@mrc-lmb.cam.ac.uk, and
 *	Jean Thierry-Mieg (CRBM du CNRS, France) mieg@kaa.cnrs-mop.fr
 *
 * Description: provides hooks to optional code, basically a call by
 			name dispatcher
	        plus wscripts/ interface
 * Exported functions:
 * HISTORY:
 * Last edited: Oct 16 12:59 2001 (edgrif)
 * * Sept 3, mieg, suppressed all statics, phread safe.
 * * Mar  3 15:51 1996 (rd)
 * Created: Mon Oct  3 14:05:37 1994 (rd)
 *-------------------------------------------------------------------
 */

/* $Id: call.c,v 1.36 2002-02-22 18:00:20 srk Exp $ */

#include "acedb.h"
#include "call.h"
#include "mytime.h"
#include <ctype.h>		/* for isprint */

/************************* call by name package *********************/

typedef struct
{ char *name ;
  CallFunc func ;
} CALL ;

static Array calls ;   /* array of CALL to store registered routines */

/************************************/

static int callOrder (void *a, void *b) 
{ return strcmp (((CALL*)a)->name, ((CALL*)b)->name) ; }


void callRegister (char *name, CallFunc func)
{
  CALL c ;

   /* getMutex ("callRegister") ; */
  if (!calls)
    calls = arrayCreate (16, CALL) ;
  c.name = name ; c.func = func ;
  if (!arrayInsert (calls, &c, callOrder))
    messcrash ("Duplicate callRegister with name %s", name) ;
  /* releaseMutex ("callRegister") ; */
}

BOOL callExists (char *name)
{
  CALL c ;
  int i;
  
  /* getMutex ("callExists") ; */
  
  if (calls)
    {
      c.name = name ;
      return arrayFind (calls, &c, &i, callOrder) ;
    }
 /* releaseMutex ("callExists") ; */

  return FALSE ;
}


#include <stdarg.h>   /* for va_start */
BOOL call (char *name, ...)
{
  va_list args ;
  CALL c ;
  int i ;

  c.name = name ;
  
  /* getMutex ("call") ; */
  if (calls && arrayFind (calls, &c, &i, callOrder))
    { va_start(args, name) ;
      (*(arr(calls,i,CALL).func))(args) ;
      va_end(args) ;
      /* releaseMutex ("call") ; */
      return TRUE ;
    }
  /* releaseMutex ("call") ; */
  return FALSE ;
}

/***************** routines to run external programs *******************/

/* ALL calls to system() and popen() should be through these routines
** First, this makes it easier for the Macintosh to handle them.
** Second, by using wscripts as an intermediate one can remove system
**   dependency in the name, and even output style, of commands.
** Third, if not running in ACEDB it does not look for wscripts...
*/

static char *buildCommand (Stack command, char *dir, char *script, char *args)
{
  char *cmd_string = NULL ;

  /* don't use messprintf() - often used to make args */
  command = stackReCreate (command, 128) ;

  if (dir)
    {
      catText (command, "cd ") ;
      catText (command, dir) ;
      catText (command, "; ") ;
    }

  catText (command, "\"");
  catText (command, script) ;
  catText (command, "\"");

  if (args)
    {
      catText (command, " ") ;
      catText (command, args) ;
    }

  cmd_string = stackText (command, 0) ;

  return cmd_string ;
}

int callCdScript (char *dir, char *script, char *args)
{
  Stack s = 0 ;
  int nn = 0 ;

  if (!script)
    return -1;
  
  nn = system (buildCommand (s, dir, script, args)) ;
  
  stackDestroy (s) ;
  return nn ;
}

int callScript (char *script, char *args)
{
  return callCdScript (0, script, args) ;
}

FILE* callCdScriptPipe (char *dir, char *script, char *args)
{
  Stack s = 0 ;
  char *command;
  FILE *pipe = 0 ;
  int peek ;

  if (!script) 
    return 0;

  command = buildCommand (s, dir, script, args) ;
  pipe = popen (command, "r" ) ;
  
  if (pipe)
    {
      peek = fgetc (pipe) ;		/* first char from popen on DEC
					   seems often to be -1 == EOF!!!
					   */
      if (isprint(peek)) 
	ungetc (peek, pipe) ;
    }
#ifdef DEBUG
  printf ("First char on callCdScriptPipe is %c (0x%x)\n", peek, peek) ;
#endif
  stackDestroy (s) ;
  return pipe ;
}

FILE* callScriptPipe (char *script, char *args)
{
  return callCdScriptPipe (0, script, args) ;
}




