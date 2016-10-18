/*  File: acetypes.h
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
 * Last edited: Aug 26 16:49 1999 (fw)
 * * Aug 26 16:49 1999 (fw): added this header
 * Created: Thu Aug 26 16:49:03 1999 (fw)
 * CVS info:   $Id: acetypes.h,v 1.9 1999-09-01 11:02:30 fw Exp $
 *-------------------------------------------------------------------
 */
#ifndef ACETYPES_H
#define ACETYPES_H

typedef enum { _Int, _Text, _Float, _DateType, _Key, _Tag } AceType;
typedef enum 
{ OK=0, 
  NOCONTEXTERR,
  NOTDONEERR
} AceErrType;

#ifndef  ACELIB__H   /* private kernel definitions */

typedef void  AceContext ;
typedef int   Bool;

#ifndef TRUE
#define TRUE 1
#endif
#ifndef FALSE
#define FALSE 0
#endif

#include "regular.h"
#include "mytime.h"		/* not included in standard libutil header */

#include "keyset.h"

typedef Array    AceStringSet;
typedef Array    AceInstanceSet;
typedef KEYSET   AceKeySet;
typedef void*    AceHandle;
typedef void*    AceInstance;
typedef void*    AceMark;
typedef void*    AceTable;
typedef KEY      AceKey;
typedef KEY      AceTag;
typedef unsigned int AceDateType;

typedef union			/* structure for unknown types */
  { int	  i ;
    float f ;
    KEY   k ;
    char* s ;
    AceDateType time ;
  } AceUnit ;
 

#endif /* !ACELIB__H */

#endif /* !ACETYPES_H */
 
