/*  File: status.h
 *  Author: Fred Wobus (fw@sanger.ac.uk)
 *  Copyright (C) J Thierry-Mieg and R Durbin, 1999
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
 * Description: Reports the status of various parts of AceDB.
 * Exported functions: tStatus
 * HISTORY:
 * Last edited: Nov 26 17:11 2007 (edgrif)
 * Created: Wed May 12 15:43:36 1999 (fw)
 * CVS info:   $Id: status.h,v 1.5 2007-11-26 17:12:26 edgrif Exp $
 *-------------------------------------------------------------------
 */
#ifndef ACEDB_STATUS_H
#define ACEDB_STATUS_H

#include "aceiotypes.h"

/* Values for setting flag to control which stats are printed out.           */
typedef enum _StatusType {STATUS_EMPTY = 0,
			  STATUS_CODE = 1, STATUS_DATABASE = 2,
			  STATUS_DISK = 4, STATUS_LEXIQUES = 8,
			  STATUS_HASHES = 16, STATUS_INDEXING = 32,
			  STATUS_CACHE = 64, STATUS_CACHE1 = 128, STATUS_CACHE2 = 256,
			  STATUS_ARRAYS = 512, STATUS_MEMORY = 1024,
#ifdef ACEMBLY
			  STATUS_DNA = 2056,
#endif
			  STATUS_ALL = 4096} StatusType ;

/* command line options that correspond to above values, (looks like duplication but some macro
 * parsers can't take a coniditional macro within a macro definition...).  */
#ifdef ACEMBLY
#define STATUS_OPTIONS_STRING   "-code -database "        \
                                "-disk -lexiques "        \
                                "-hash -index "           \
                                "-cache -cache1 -cache2 " \
                                "-array -memory "         \
                                "-dna"                    \
                                "-all"
#else
#define STATUS_OPTIONS_STRING   "-code -database "        \
                                "-disk -lexiques "        \
                                "-hash -index "           \
                                "-cache -cache1 -cache2 " \
                                "-array -memory "         \
                                "-all"
#endif


/* Output all stats.                                                         */
void tStatus(ACEOUT fo) ;

void tStatusWithTitle(ACEOUT fo, char *title) ;

/* Pass in cmdline option and set type appropriately for passing into        */
/* tStatusSelect(), returns TRUE if option known, FALSE otherwise.           */
BOOL tStatusSetOption(char *cmdline_option, StatusType *type) ;

/* Output selected sets of stats according to type flag.                     */
void tStatusSelect(ACEOUT fo, StatusType type) ;



#endif /* !ACEDB_STATUS_H */
