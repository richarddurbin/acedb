/*  File: graphremote.c
 *  Author: Fred Wobus (fw@sanger.ac.uk)
 *  Copyright (C) J Thierry-Mieg and R Durbin, 1999
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
 * 	Richard Durbin (Sanger Centre, UK) rd@sanger.ac.uk, and
 *	Jean Thierry-Mieg (CRBM du CNRS, France) mieg@crbm.cnrs-mop.fr
 *
 * Description: platform independant top-level API for
 *              remote control of graphPackage-applications
 * Exported functions:
 *              graphRemoteRegister - called by applications during
 *			initialisation phase to register the application
 *			specific function that handles textual commands 
 *			and performs the appropriate actions.
 *              graphRemoteCommand - called by the remote-control client
 *			to place a command. The returned status gives an
 *			indication whether the command was received by 
 *			a graph-application and whether it could be
 *			correctly parsed and executed.
 * HISTORY:
 * Last edited: Nov 10 15:07 1999 (fw)
 * * Apr 29 16:05 1999 (edgrif): Changed call to xt level remote command
 *              to be graphdev call.
 * Created: Mon Jan 25 13:45:30 1999 (fw)
 *-------------------------------------------------------------------
 */

/* $Id: graphremote.c,v 1.9 2002-02-22 18:13:34 srk Exp $ */

#include "regular.h"
#include <w2/graph_.h>


/************************************************************/

static GraphRemoteHandlerFunc theRemoteRoutine = 0;

/* called by server-app before graphInit */
GraphRemoteHandlerFunc graphRemoteRegister (GraphRemoteHandlerFunc func)
/* func            - handler function of the application, 
 *                   which translates the text-commands into the
 *		     applications actions.
 */
{
  GraphRemoteHandlerFunc old = theRemoteRoutine;

  if (func)
    theRemoteRoutine = func;

  return old;
} /* graphRemoteRegister */

/************************************************************/

/* called by client */
int graphRemoteCommands (char *applicationName,
			 int *cmdc, char **cmdv)
     /* applicationName - the program we want to talk to */
{
  int response_status = 0;

  if (gDevFunc->graphDevRemoteCommands != NULL)
    response_status = gDevFunc->graphDevRemoteCommands(applicationName, 
						       cmdc, cmdv);

  return response_status;
} /* graphRemoteCommands */

/************************* eof ******************************/
