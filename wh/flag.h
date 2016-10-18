/*  File: flag.h
 *  Author: Richard Durbin (rd@sanger.ac.uk)
 *  Copyright (C) J Thierry-Mieg and R Durbin, 1997
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
 * SCCS: $Id: flag.h,v 1.5 1999-09-01 11:08:59 fw Exp $
 * Description:
 * Exported functions:
 * HISTORY:
 * Last edited: Apr  3 12:12 1997 (rd)
 * Created: Sat Feb  1 12:53:00 1997 (rd)
 *-------------------------------------------------------------------
 */

#include "dict.h"

typedef unsigned int FLAGSET ;

#define NON_FLAG (~0)		/* all bits - same as in table.h */
#define MAXFLAG (sizeof(FLAGSET)*8)

FLAGSET flag (char *set, char *s) ;
char *flagName (char *set, FLAGSET flag) ;
DICT *flagSetDict (char *set) ;
char *flagNameFromDict (DICT *dict, FLAGSET flag) ;

/******************** end of file ***********************/
 
 
 
