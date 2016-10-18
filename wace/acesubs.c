/*  File: acesubs.c
 *  Author: Richard Durbin  
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
 * Description: Utility routines for .ace file creating programs 
 * Exported functions: 
 * HISTORY:
 * Last edited: Aug 26 16:48 1999 (fw)
 * * Aug 26 16:48 1999 (fw): added this header
 * Created: Thu Aug 26 16:48:22 1999 (fw)
 * CVS info:   $Id: acesubs.c,v 1.3 1999-09-01 11:26:45 fw Exp $
 *-------------------------------------------------------------------
 */

#include "regular.h"
#include "array.h"
#include "ace.h"

/***** input line parser for cgc biblio and stock files ******/

char *nextField (char *pcutter)		/* relies on freecard() being set up */
{
 char *cp ;
static char ww[50][1000] ;		/* hard limit of a history of 50 */
static int  nfield = 0 ;

 if (nfield >= 50)
   nfield = 0 ;
 freewordcut ("|", pcutter) ;		/* get into start of field */
 if (cp = freewordcut ("|", pcutter))	/* take field */

   strcpy (ww[nfield],cp) ;
 else
   *ww[nfield] = 0 ;
 return ww[nfield++] ;
}

/***** two output routines - for a single item and a list ****/
/*************************************************************/

void outItem (FILE *fil, char *tag, char *text)
{
  int nused = strlen(tag) + 3 ;
  int nleft = 75 - nused ;
  int i,j,nlines ;

  if (nleft < 40)
    { fprintf (stderr,"Tag name too long : %s\nrethink and recompile\n",tag) ;
      exit (0) ;
    }
  
  fprintf (fil,"%s : ",tag) ;
  if (strlen(text) < nleft) 
    fprintf (fil,"\"%s\"\n",text) ;
  else
    { putc ('"',fil) ;
      nlines = uLinesText (text,nleft) ;
      for (i = nlines-1 ; i-- ;)
	{ fprintf (fil,"%s\\\n",uNextLine(text)) ;
	  for (j = nused ; j-- ;)
	    putc (' ',fil) ;
	}
      fprintf (fil,"%s\"\n",uNextLine(text)) ;
    }
}

/********************************************/


void outList (FILE *fil, char *tag, char **items, int n)
{
  int i = 0 ;

  if (!n)
    return ;
  if (strlen(tag) + strlen(*items) > 70)
    { outItem (fil,tag,*items) ;
      outList (fil,tag,++items,--n) ;
      return ;
    }
  i += fprintf (fil,"%s : ",tag) ;
  while (n)
    { if (i + strlen (*items) > 75)
	{ putc ('\n',fil) ;
	  outList (fil,tag,items,n) ;
	  return ;
	}
      i += fprintf (fil,"\"%s\" ",*items++) ;
      n-- ;
    }
  putc ('\n',fil) ;
}

/********************************************/
    /* JTM new outStack() 31/7/90 */
void outStack (FILE *fil, char *tag, Stack s)
{
  char * cp ;
  int i;

  if(stackAtEnd(s))
    return ;

  cp = stackNextText(s);

  debut :
    i = 0 ;
  if (strlen(tag) + strlen(cp) > 70)
    { outItem (fil,tag,cp) ;
      outStack (fil,tag,s) ;
      return ;
    }
  i += fprintf (fil,"%s : ",tag) ;
  while (cp && *cp)
    { if (i + strlen (cp) > 75)
        { putc ('\n',fil) ;
          goto debut;
        }
      i += fprintf (fil,"\"%s\" ",cp) ;
      if(stackAtEnd(s))
	 break ;
      cp = stackNextText(s);
    }
  putc ('\n',fil) ;
}

/*******************************************************/

#include "systags.wrm"
#include "tags.wrm"
#include "classes.wrm"
#include <ctype.h>

  /* Standardises the chromosomes names */

static int lexChromosome(char *text)
{
 int i=0, v=0, x=0;
 register char * cp;

 cp=text;


 if(!cp) return 0;
 while (*cp++); cp--;
 while(cp-- >text)
 {
 switch(*cp)
  {
   case ' ' :
   case 'L' :
   case 'R' :
   case 'C' : break;
   case 'I' : i++;
              break;
   case 'V' : v++;
              i=100*i;
              break;
   case 'X' : x++;
              break;
   default  : goto ok;
   }
  *cp=0;
  }
ok :
  if(x)
    { if(x==1 && i==0 && v==0) return 6;
                 else return(0);
     }
  if(v)
   {if(v==1)
    switch(i)
     {case 0 : return 5;
      case 1 : return 4;
      default : return 0;
      }
    else return 0;
    }

  if( i>0 && i<4 ) return i;
  return 0;
 }

/***********************************************/
       /* transforms names like Bond007 into Bond7 */
       /* and into Bond-7 if bond is a gene*/
static char newlexname[250] ;
static char* chromName[] = {"","I","II","III","IV","V","X"} ;

char *lexcleanup(char *name, int t)
{
 register char *cp, *cq;
 char  number[250];

 int i ;

 if (t == _VChromosome)
   if (i = lexChromosome(name))
     return chromName[i] ;
   else 
     messout ("Uncorrect chromosome name %s", cp);
 
 strcpy(newlexname, name);
 cp=cq=newlexname;
 while(*cq++);
 cq--;cq--;
 for(cp=number;cp<number+249;*cp++=0);
 cp=number;
 while(cq>=newlexname)
     {            /* clean the number part off*/
       if(*cq==' ');
         else if(isdigit(*cq)) *cp++=*cq;
                 else break;
      *cq--=0;
      }
 cp--;  
 if(*cq == '-')
   {
    if(t == _VGene) cq++;
   }
 else
   {
    cq++;
    if((t == _VGene ) && (cp>=number))
      *cq++ = '-';
  }

 while(cp>=number && *cp=='0') cp--;
 while(cp>=number) *cq++=*cp--;
 *cq=0;

 return newlexname;
}





