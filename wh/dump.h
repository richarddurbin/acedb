/*  File: dump.h
 *  Author: Fred Wobus (fw@sanger.ac.uk)
 *  Copyright (c) J Thierry-Mieg and R Durbin, 1999
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
 *	Jean Thierry-Mieg (CRBM du CNRS, France) mieg@kaa.crbm.cnrs-mop.fr
 *
 * Description: 
 * Exported functions:
 * HISTORY:
 * Last edited: Oct 20 07:54 2006 (edgrif)
 * Created: Thu Aug 26 17:18:17 1999 (fw)
 * CVS info:   $Id: dump.h,v 1.19 2006-10-20 06:55:39 edgrif Exp $
 *-------------------------------------------------------------------
 */

#ifndef ACEDB_DUMP_H
#define ACEDB_DUMP_H

#include "keyset.h"
#include "query.h"
#include "bs.h"

extern BOOL dumpTimeStamps;
extern BOOL dumpComments;

extern DumpFuncType dumpFunc[] ;

void dumpObj(OBJ obj) ;					    /* noddy util., dumps obj to stdout. */

void dumpAll(void) ;					    /* Menu func. */
void dumpAllNonInteractive (char *directory, BOOL split) ;

void dumpReadAll(void) ;				    /* Menu func. */
BOOL dumpDoReadAll(ACEIN parse_in, ACEOUT result_out, BOOL interactive,
		   char *dirSelection, char *fileSelection) ;

BOOL dumpClassPossible (int classe, char style) ; /* 'a' for ace style */
BOOL dumpKey (ACEOUT dump_out, KEY key);
BOOL dumpKeyBeautifully (ACEOUT dump_out, KEY key, char style, COND cond);

void niceDump (ACEOUT dump_out, OBJ objet, char style);

#endif /* !ACEDB_DUMP_H */
 
