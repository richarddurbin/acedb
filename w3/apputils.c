/*  File: apputils.c
 *  Author: Ed Griffiths (edgrif@sanger.ac.uk)
 *  Copyright (c) J Thierry-Mieg and R Durbin, 2003
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
 *-------------------------------------------------------------------
 * This file is part of the ACEDB genome database package, written by
 * 	Richard Durbin (Sanger Centre, UK) rd@sanger.ac.uk, and
 *	Jean Thierry-Mieg (CRBM du CNRS, France) mieg@kaa.crbm.cnrs-mop.fr
 *
 * Description: Contains routines common to all main functions to avoid
 *              code repetition.
 *              
 *              In fact the main functions could mostly be condensed
 *              considerably themselves into a single big routine...
 *              one day...
 *              
 * Exported functions: See wh/apputils.h
 * HISTORY:
 * Last edited: Jan 16 10:06 2003 (edgrif)
 * Created: Tue Jan 14 10:54:35 2003 (edgrif)
 * CVS info:   $Id: apputils.c,v 1.3 2003-01-16 10:07:35 edgrif Exp $
 *-------------------------------------------------------------------
 */

#include <wh/regular.h>
#include <wh/acedb.h>
#include <wh/session.h>
#include <wh/apputils.h>
#include <wh/aceversion.h>


/* Checks whether user specified the "-version" command line option, and
 * prints out the version to stdout and exits. Note that we use a raw
 * printf here because messout may have been pointed to somewhere graphical
 * by xace etc. which we don't want. */
void appCmdlineVersion(int *argcp, char **argv)
{
  if (getCmdLineOption (argcp, argv, "-version", NULL))
    {
      printf("%s,  build dir: %s, %s\n",
	     aceGetVersionString(), aceGetReleaseDir(), aceGetLinkDateString()) ;
      exit(EXIT_SUCCESS) ;
    }

  return ;
}


/* Allows user to set an alternative to their userid for recording time stamp/session
 * information for the database. The userid must be alphanumerics. */
void appTSUser(int *argcp, char **argv)
{
  char *value = NULL ;

  if (getCmdLineOption(argcp, argv, "-tsuser", &value))
    {
      if (!sessionSetUserSessionUser(value))
	{
	  if (!messQuery("Command line option \"-tsuser %s\" is invalid, "
			 "the username must be alphanumeric chars only. "
			 "Do you wish to continue ?", value))
	    exit(EXIT_FAILURE) ;
	}
      messfree (value);
    }


  return ;
}
