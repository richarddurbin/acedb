/*  File: acelibtest.c
 *  Author: Ed Griffiths (edgrif@sanger.ac.uk)
 *  Copyright (c) J Thierry-Mieg and R Durbin, 1998
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
 * Last edited: Dec  2 16:29 1998 (edgrif)
 * * Dec  2 16:29 1998 (edgrif): Added this header + code to record
 *              build time of this executable.
 * Created: Wed Dec  2 16:28:37 1998 (edgrif)
 * CVS info:   $Id: acelibtest.c,v 1.5 1999-09-01 11:28:22 fw Exp $
 *-------------------------------------------------------------------
 */
#include "acelib.h"
#include "version.h"


/* Defines a routine to return the compile date of this file.                */
UT_MAKE_GETCOMPILEDATEROUTINE()


void main(int argc, char **argv)
{ 
  int n, i ;
  char *cp ;
  AceStringSet s ; 
  KEYSET ks ;

  if (aceInit(argc>1 ? argv[1] : 0))
    { aceOpenContext("test") ;
      s = aceClasses(0) ;
      for (i = 0 ;  i < arrayMax(s) ; i++)
	if (aceClassSize (arr(s,i,char*), &n))
	  if (n>0)printf ("number of items in class %s = %d\n", 
		  arr(s,i,char*), n) ;
      
       ks = aceQuery(0, "FIND Author b*",0) ;
       for (i=0; i < keySetMax(ks) ; i++)
	 printf("author:: %s\n", aceName(keySet(ks,i))) ;

      aceQuit(FALSE) ;
    }
  else
    fprintf (stderr, aceErrorMessage(0)) ;
}

 

 
