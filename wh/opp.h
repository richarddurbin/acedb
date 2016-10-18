/*  File: opp.h
 *  Author: 
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
 * Description: Required for complimenting a sequence
 * Exported functions: See XXXXXXXXXXXXX.h
 * HISTORY:
 * Last edited: Aug 26 17:37 1999 (fw)
 * * Aug 26 17:36 1999 (fw): added this header
 * * 15.01.90 (SD): Taken from seqIOEdit.h
 * Created: Thu Aug 26 17:36:39 1999 (fw)
 * CVS info:   $Id: opp.h,v 1.3 1999-09-01 11:15:34 fw Exp $
 *-------------------------------------------------------------------
 */
#ifndef _opp_h
#define _opp_h

#include "regular.h"
#include "seq.h"

/* ---- Exports ---- */

extern char opp[256]; /* complement of any given base */

extern void oppInitialize();

/* initializes the array which stores the complement 
   of any of the Staden nucleotides or ambiguity
   codes */


void complement_seq(Seq seq);

/* complement a sequence */

#endif  /*_opp_h*/





