/*  File: homonym.c
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
 *      The pupose of this stand alone is to resolve the orthograph conflicts
 *      in author names. It takes as  arguments, a homonym file of the form
 * 
 * correct_name AA =
 * alias1 posibly with several words =
 * alias2 no double quotes expected 
 * dummyentry
 * newname =
 * newalias
 * 
 *    and reads from an ace file.
 *    It will act only on the author entries of the ace file and, on these
 *    will replace the occurences of alias by the correct name 
 * 
 * Exported functions:
 * HISTORY:
 * Last edited: Aug 26 17:25 1999 (fw)
 * * Aug 26 17:25 1999 (fw): added this header
 * Created: Thu Aug 26 17:24:38 1999 (fw)
 * CVS info:   $Id: homonym.c,v 1.3 1999-09-01 11:26:55 fw Exp $
 *-------------------------------------------------------------------
 */

#define ARRAY_NO_CHECK

#include "regular.h"
#include "array.h"

static Stack ss = 0 ;
static Array alias = 0 ;
static int line = 0, transfo = 0 , nnew = 0 ;

typedef struct {int bad; int good; } ALIAS ;

/****************************************************************/

static void scanInput(void)
{  char *cp , cutter ; register int i , n ;
   FILE *new = fopen("newnames","w") ;
   
   if (!new)
     messcrash("Can't open the newname file") ;
 fprintf(stderr,"I begin the scan line %d\n", line) ;
  while(line++ , freeread(stdin))
    { cp = freeword() ;
      if(!cp)
	{ printf("\n") ; continue ;}
      if (   strcmp("Author",cp)
          && strcmp("Mapper",cp)
          && strcmp("From_Author",cp)
          && strcmp("Representative",cp)
	  )  /* Just copy the whole line to the output */
	{ freeback() ;
	  printf(freewordcut("\n",&cutter)) ;
	  printf("\n") ;
	  continue ;
	}
          /* Found author, do check */
      printf("%s : \"",cp) ;
      freenext() ; freestep(':') ; freenext() ;
      cp = freeword() ;
      if(cp)
	{ i = arrayMax(alias) ;
	  while(i--)
	    if(!strcmp(cp,stackText(ss,arr(alias,i,ALIAS).good)))
	      goto found;
	    else if( (n = arr(alias,i,ALIAS).bad) &&
		      !strcmp(cp,stackText(ss,n)))
	      { cp = stackText(ss,arr(alias,i,ALIAS).good) ;
		transfo++ ;
		goto found ;
	      }
	  nnew++ ;
	  fprintf(new,"%s\n",cp) ;
	found: 
	  printf(cp) ;
	}
      printf("\"\n") ;
    }
   filclose(new) ;
}


/****************************************************************/

static BOOL readRules(char *name)
{
  FILE *f = fopen(name,"r") ;
  int nA = 0 , n ;
  char *cp , *cq , cutter ;
  BOOL found = FALSE ;
  int lastnA = 0 ;

  if(!f)
    { fprintf(stdout, "Cannot open the homonym file %s", name) ;
      return FALSE ;
    }

  ss = stackCreate(1000) ;
  pushText(ss,"dummy") ;
  alias = arrayCreate(100, ALIAS) ;

  while (freeread(f))
   if(cp = freewordcut("\n",&cutter))
     { cq = cp + strlen(cp) - 1 ;
       while(cq > cp && *cq == ' ') cq--;
       if(*cq == '=')
	 { *cq = ' ' ;
           found = TRUE ;
	   while(cq > cp && *cq == ' ') cq--;
	   *++cq = 0 ;
	   n = stackMark(ss) ;
	   pushText(ss,cp) ;
           array(alias, nA++,ALIAS).bad = n ;
	 }
       else
	 if (found)
	   { found = FALSE ;
	     *++cq = 0 ;
	     n = stackMark(ss) ;
	     pushText(ss,cp) ;
	     for ( ;lastnA <nA; lastnA++)
	       array(alias, lastnA,ALIAS).good = n ;
	   }
	 else
	   { array(alias, nA++,ALIAS).bad = 0 ;
	     *++cq = 0 ;
	     n = stackMark(ss) ;
	     pushText(ss,cp) ;
	     for ( ;lastnA <nA; lastnA++)
	       array(alias, lastnA,ALIAS).good = n ;
	   }
     }
  fprintf(stderr,"Found %d alias to eliminate\n",nA) ;
  filclose(f) ;
  return nA > 0 ? TRUE : FALSE ;
}

/****************************************************************/

void main (int argc, char **arg)
{
  if (argc != 2)
    { fprintf (stderr,
	       "Usage : homonym homoFile < input > output \n") ;
      return ;
    }
 
  freeinit() ;
  if(readRules(arg[1]))
    scanInput() ; 

  fprintf(stderr,"\n done, scanned %d lines, did %d transformations\n found %d new names ",
	  line, transfo, nnew ) ;
}

/****************************************************************/
/****************************************************************/



