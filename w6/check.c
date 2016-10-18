/*  File: check.c
 *  Author: Jean Thierry-Mieg (mieg@mrc-lmb.cam.ac.uk)
 *  Copyright (C) J Thierry-Mieg and R Durbin, 1992
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
 * 	Richard Durbin (MRC LMB, UK) rd@mrc-lmb.cam.ac.uk, and
 *	Jean Thierry-Mieg (CRBM du CNRS, France) mieg@kaa.cnrs-mop.fr
 *
 * Description:
 * Exported functions:
 * HISTORY:
 * Last edited: Nov  9 13:59 1999 (fw)
 * Created: Wed Sep 16 13:33:07 1992 (mieg)
 *-------------------------------------------------------------------
 */

/* $Id: check.c,v 1.6 2004-04-22 22:26:12 mieg Exp $ */

#include <ctype.h>

#include "acedb.h"
#include "whooks/systags.h"
#include "check.h"


BOOL checkRegExpSyntax (char *cp)
{
  if (!cp || ! *cp)
    return FALSE ;

  return TRUE ;
}

BOOL checkText (char *text, KEY cst)
{ char c, *cp, *cq = text ;

  cp = name (cst) ;

  if (!strncmp(cp, "FORMAT ",7)) 
    { cp += 7 ;
      
      while ((c = *cp++))
	switch (c)
	  {
	  case 'a':
	    if (!isalpha((int)*cq))
	      return FALSE ;
	    cq++ ;
	    break ;
	  case 'n':
	    if (!isdigit((int)*cq))
	      return FALSE ;
	    cq++ ;
	    break ;
	  default:
	    if (c != *cq)
	      return FALSE ;
	    cq++ ;
	    break;  
	  }
    }
  return TRUE ;
}

/************************************************/

BOOL checkKey (KEY new, KEY cst)
{ 

  
  
  return TRUE ;
}

/************************************************/

BOOL checkData (void *xp, KEY type, KEY cst)
{ int i ; 
  float x ;
  mytime_t tm ;

  switch (type)
    {
    case _Int: 
      i = *(int*) xp ;
      x = i ;
      break ;
    case _Float: 
      i = *(float*) xp ;
      x = i ;
      break ;
    case _DateType: 
      tm = *(mytime_t*) xp ;
      x = tm ;
      break ;
    default:
      messcrash ("badtype in checkData") ;
    }

  
  
  
  return TRUE ;
}

/************************************************/
/************************************************/
