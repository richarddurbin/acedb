/*  File: biblio.h
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
 * $Id: biblio.h,v 1.8 1999-11-12 16:00:33 fw Exp $
 * Description:
 * Exported functions:
 * HISTORY:
 * Last edited: Nov 12 12:07 1999 (fw)
 * Created: Thu Jan 25 21:29:10 1996 (rd)
 *-------------------------------------------------------------------
 */

#ifndef ACEDB_BIBLIO_H
#define ACEDB_BIBLIO_H

#include "acedb.h"
#include "aceiotypes.h"

  /* graphic func: display biblio associated to key or keyset */
void biblioKey (KEY key) ;
void biblioKeySet (char *title, KEYSET s) ;
  /* general: available from command.c */
void biblioDump (ACEOUT biblio_out, KEYSET bibSet, BOOL abstracts, int width);
KEYSET biblioFollow (KEYSET s) ; /* associated papers etc */
BOOL biblioKeyPossible (KEY k) ; /* true if biblio tags in key */
BOOL biblioPossible (KEYSET s) ; /* true if biblio tags in class(keySet(s,0)) */

#endif /* !ACEDB_BIBLIO_H */
