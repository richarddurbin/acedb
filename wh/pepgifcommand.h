/*  File: pepdisp.h
 *  Author: Fred Wobus
 *  Copyright (C) J Thierry-Mieg and R Durbin, 1995
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
 *	Jean Thierry-Mieg (CRBM du CNRS, France) mieg@crbm.cnrs-mop.fr
 *
 * Description:
 * Exported functions:
 * HISTORY:
 * Last edited: Oct 22 14:39 1999 (fw)
 * Created: Fri Oct 22 14:35:13 1999 (fw)
 *-------------------------------------------------------------------
 */

#ifndef ACEDB_PEPGIFCOMMAND_H
#define ACEDB_PEPGIFCOMMAND_H

#include "acedb.h"
#include "aceiotypes.h"

/******* peptide giface commands ****************/

typedef struct PepGifCommandStruct *PepGifCommand ; /* opaque public handle */

/* command callbacks for giface */
PepGifCommand pepGifCommandGet (ACEIN fi, ACEOUT fo, PepGifCommand pepLook);/* will destroy existing pepLook */
void pepGifCommandSeq (ACEIN fi, ACEOUT fo, PepGifCommand pepLook);
void pepGifCommandAlign (ACEIN fi, ACEOUT fo, PepGifCommand pepLook);
void pepGifCommandDestroy (PepGifCommand pepLook);


#endif /* !ACEDB_PEPGIFCOMMAND_H */
