/*  File: parse_.h
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
 * Description: 
 * Exported functions:
 * HISTORY:
 * Last edited: Jul  6 16:13 2000 (edgrif)
 * Created: Wed Apr 21 13:43:29 1999 (fw)
 * CVS info:   $Id: parse_.h,v 1.1 2000-09-13 12:43:20 edgrif Exp $
 *-------------------------------------------------------------------
 */
#ifndef ACEDB_PARSE_PRIV_H
#define ACEDB_PARSE_PRIV_H

#include <wh/parse.h>					    /* Our public header. */

/* parse ace-data from given stream. 
 *    It allows parsing data for protected classes.
 *    Therefore not to be used for user-defined streams, only 
 *    server-side wspec/ files!!
 * ks - if non-NULL it will add the keys of any objects parsed onto it
 *
 * WILL destroy the given ACEIN
 * It will break upon encountering parse-errors */

BOOL parseAceInProtected (ACEIN parse_io, KEYSET ks);


#endif /* !ACEDB_PARSE_PRIV_H */
/********* end of file ***********/
