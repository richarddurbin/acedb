/*  File: matchtable.h
 *  Author: Richard Durbin (rd@sanger.ac.uk)
 *  Copyright (C) J Thierry-Mieg and R Durbin, 1996
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
 * SCCS: $Id: matchtable.h,v 1.7 2000-08-03 13:58:02 edgrif Exp $
 * Description:
 * Exported functions:
 * HISTORY:
 * Last edited: Jul 28 21:11 2000 (edgrif)
 * Created: Tue Nov 26 09:46:10 1996 (rd)
 *-------------------------------------------------------------------
 */

#ifndef ACEDB_MATCHTABLE_H
#define ACEDB_MATCHTABLE_H

#include "acedb.h"
#include "table.h"
#include "flag.h"

typedef struct {
  KEY key ;
  KEY meth ;
  float score ;
  int q1, q2, t1, t2 ;
  FLAGSET flags ;		/* flag set name is "Match" */
  KEY stamp ;
} MATCH ;

#define matchFormat "kkfiiiikk"

ParseFuncReturn matchTableParse (ACEIN parse_io, KEY key, char **errtext) ; /* ParseFuncType */
BOOL matchTableDump (ACEOUT dump_out, KEY k) ; /* DumpFuncType */
BOOL matchTableKill (KEY key) ;

TABLE *matchTable (KEY key, STORE_HANDLE h) ;

#endif /* !ACEDB_MATCHTABLE_H */

/********** end of file *************/
 
