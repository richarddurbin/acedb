/*  File: version.h
 *  Author: Ed Griffiths (edgrif@mrc-lmba.cam.ac.uk)
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
 * 	Richard Durbin (MRC LMB, UK) rd@mrc-lmba.cam.ac.uk, and
 *	Jean Thierry-Mieg (CRBM du CNRS, France) mieg@crbm1.cnusc.fr
 *
 * Description: Declares functions in the new acedb_version module.
 *              These functions allow the retrieval of various parts
 *              of the current acedb version number or string.
 * Exported functions: See descriptions below.
 * HISTORY:
 * Last edited: Apr 13 14:04 2004 (rnc)
 * * Dec  3 14:39 1998 (edgrif): Changed the interface to fit in with
 *              libace.
 * Created: Wed Apr 29 13:46:41 1998 (edgrif)
 *-------------------------------------------------------------------
 */


#ifndef ACE_VERSION_H
#define ACE_VERSION_H


/* Use this set of functions to return the individual parts of the ACEDB release numbers,  */
/* including version number, release number, update letter and build date/time.            */
int   aceGetVersion(void) ;
int   aceGetRelease(void) ;
int   aceGetUpdate(void) ;
char *aceGetReleaseDir(void) ;
char *aceGetLinkDate(void) ;

/* Use this set of functions to return standard format string versions of the ACEDB        */
/* release version and build date.                                                         */
/* Version string is in the form:                                                          */
/*      "ACEDB <version>_<release><update>"  e.g.  "ACEDB 4_6d"                            */
/*                                                                                         */
/* LinkDate string is in the form:                                                         */
/*      "compiled on: __DATE__ __TIME__"  e.g.    "compiled on: Dec  3 1998 13:59:07"      */
/*                                                                                         */
char *aceGetVersionString(void) ;
char *aceGetLinkDateString(void) ;


/* Checks for the "-version" command line option, if found, prints version   */
/* information to stdout and exits, so should be called early on in a        */
/* programs execution.                                                       */
void checkForCmdlineVersion(int *argcp, char **argv) ;


#endif	/* end of ACE_VERSION_H */
