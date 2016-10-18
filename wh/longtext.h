/*  File: longtext.h
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
 * 	Richard Durbin (Sanger Centre, UK) rd@sanger.ac.uk, and
 *	Jean Thierry-Mieg (CRBM du CNRS, France) mieg@crbm.cnrs-mop.fr
 *
 * Description: header for longtext.c module
 * Exported functions:
 * HISTORY:
 * Last edited: Mar 15 11:06 2001 (edgrif)
 * Created: Wed Dec  2 17:01:49 1998 (fw)
 * CVS info:   $Id: longtext.h,v 1.8 2001-03-15 13:55:34 edgrif Exp $
 *-------------------------------------------------------------------
 */
#ifndef ACEDB_LONGTEXT_H
#define ACEDB_LONGTEXT_H

#include "acedb.h"

BOOL longTextDump (ACEOUT dump_out, KEY key); /* DumpFuncType */
ParseFuncReturn longTextParse (ACEIN fi, KEY key, char **errtext); /* ParseFuncType */

BOOL javaDumpLongText (ACEOUT dump_out, KEY key);
KEYSET longGrep(char *template);

/* the graphical DisplayFunc is longtextdisp.c */
BOOL longTextDisplay (KEY key, KEY from, BOOL isOldGraph, void *unused) ;

#endif /* !ACEDB_LONGTEXT_H */
