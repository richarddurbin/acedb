/*  File: tagcount.c
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
 * SCCS: $Id: tagcount.c,v 1.7 1999-11-10 09:41:38 fw Exp $
 * Description: little program to count usages of tags in each class
 * Exported functions:
 * HISTORY:
 * Last edited: Nov 10 09:41 1999 (fw)
 * * Dec  2 15:53 1998 (edgrif): Make declaration of main correct, add
 *              code to record build time of this module.
 * Created: Mon Sep 29 13:26:50 1997 (rd)
 *-------------------------------------------------------------------
 */

#include "acedb.h"
#include "acelib.h"
#include "pick.h"
#include "whooks/systags.h"
#include "lex.h"
#include "version.h"
#include "bs.h"
#include "bs_.h"

static Array a ;

static void addInfo (BS bs)
{
  if (!class(bs->key) && bs->key > _LastN)
    ++array(a,bs->key,int) ;
  if (bs->right)
    addInfo (bs->right) ;
  if (bs->down)
    addInfo (bs->down) ;
}

static void printUsage (void)
{
  fprintf (stdout, 
	   "Usage: tagcount [options] <acedbdir>\n"
	   "   Options :\n"
	   "       -u   only user classes\n"
	   "       -s   only system classes\n"
	   "       -a   all classes\n"
	   "   if no option is given, user classes are shown.\n") ;
}


/* Defines a routine to return the compile date of this file.                */
UT_MAKE_GETCOMPILEDATEROUTINE()


/************************************************************/

int main (int argc, char **argv)
{
  int c, i, n ;
  KEY k ;
  OBJ o ;
  BOOL IsSysClassShown = FALSE;
  BOOL IsUserClassShown = TRUE;

  --argc ; ++argv ;
  if (!argc)
    { 
      printUsage ();
      return (EXIT_FAILURE);
    }

  if (argv[0][0] == '-')
    {
      /* an option is specified */
      if (argv[0][1] == 'u')
	{
	  IsSysClassShown = FALSE;
	  IsUserClassShown = TRUE;
	}
      else if (argv[0][1] == 's')
	{
	  IsSysClassShown = TRUE;
	  IsUserClassShown = FALSE;
	}
      else if (argv[0][1] == 'a')
	{
	  IsSysClassShown = TRUE;
	  IsUserClassShown = TRUE;
	}
      else
	{
	  fprintf (stderr, "tagcount: illegal option %s\n", *argv);
	  printUsage ();
	  return (EXIT_FAILURE);
	}
      --argc ; ++argv ;
    }

  if (!argc)
    { 
      printUsage ();
      return (EXIT_FAILURE);
    }

  aceInit (*argv) ;

  a = arrayCreate (5000, int) ;

  for (c = 0 ; c < 256 ; ++c)
    { 
      if (pickList[c].type != 'B')
	continue;
      if ((IsSysClassShown && pickList[c].protected) ||
	  (IsUserClassShown && !pickList[c].protected))
	{
	  a = arrayReCreate (a, 5000, int) ;
	  k = 0 ;
	  n = 0 ;
	  while (lexNext(c,&k))
	    if ((o = bsCreate (k)))
	      { 
		addInfo (o->root) ;
		bsDestroy (o) ;
		++n ;
	      }
	  printf ("%s\tOBJECT_COUNT\t%d\n", pickClass2Word(c), n) ;
	  for (i = _LastN ; i < arrayMax (a) ; ++i)
	    if (arr(a,i,int))
	      printf ("%s\t%s\t%d\n", 
		      pickClass2Word(c), name(i), arr(a,i,int)) ;
	}
    }
  return (EXIT_SUCCESS) ;
}

/******************* end of file **********************/
 
