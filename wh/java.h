/*  File: java.h
 *  Author: mieg
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
 * Last edited: Jun 15 22:01 2000 (rdurbin)
 * Created: Thu Aug 26 17:29:24 1999 (fw)
 * CVS info:   $Id: java.h,v 1.5 2000-06-15 22:09:11 rdurbin Exp $
 *-------------------------------------------------------------------
 */

#ifndef ACEDB_JAVA_H
#define ACEDB_JAVA_H

#include "acedb.h"
#include "aceiotypes.h"

char* freejavaprotect (char* text) ; /* in freesubs.c */
BOOL javaDumpLongText (ACEOUT dump_out, KEY key)  ; /* in longtext.c */
BOOL javaDumpKeySet (ACEOUT dump_out, KEY key) ; /* in keysetdump.c */
BOOL javaDumpDNA (ACEOUT dump_out, KEY key) ; /* in dnasubs.c */
BOOL javaDumpPeptide (ACEOUT dump_out, KEY key) ; /* in peptide.c */
char *timeShowJava (mytime_t t) ; /* in timesubs.c */

char* freeXMLprotect (char* text) ; /* in freesubs.c */

#endif /* !ACEDB_JAVA_H */
