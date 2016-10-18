/*  File: stockace.c
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
 * Description: Analyse the cgcstrn data of Mark Edgley
 *		Run this as part 2 of the script wlex/strains
 *		Stand alone program 
 * Exported functions: 
 * HISTORY:
 * Last edited: Aug 26 16:37 1999 (fw)
 * Created: Thu Aug 26 16:34:15 1999 (fw)
 * CVS info:   $Id: stockace.c,v 1.4 1999-09-01 11:27:21 fw Exp $
 *-------------------------------------------------------------------
 */

#include "regular.h"
#include "array.h"
#include "ace.h"
#include <ctype.h>
 
static void complain(char * errmess) ;

/***********************************************************************/

#define  MAXTAG  5   /* Number of tags in the following list */

static char tag[40][36] =
{ "Stock", "Junk", "splitGenotype", "Males", "Reference_Allele","Genotype" };
enum {STOCK, JUNK, GENOTYPE, MALE, REF , JUNK2 } ;
static   FILE  *input, *outFile, *errFile , *rejectFile ;
static int line = 0 ;
static Stack stack[MAXTAG] ;
static   Stack s = 0 ;   /* general text stack */
typedef struct { int g; int a; int def ; int ch; int ref ;} FG ;
static Array genes = 0 ;
static Stack buffer = 0 ;

/***********************************************************/

/***********************************************************/

static BOOL matchTag(char *cp, int *jp)
{
  int i = MAXTAG ;
  
  while(i--)
    if(!strcmp(tag[i], cp))
      break ;
  *jp = i ;
  return (i>=0) ;
}

/***********************************************************/

static int getObj(void)
{
  int found = 0 , i ;
  char cutter, *cp ;

  stackClear(s) ;
  i = MAXTAG ;
  while (i--)
    stackClear(stack[i]) ;
  

  pushText(s,"Begin") ; /* avoids first stackMark(s) == 0 */
  while (++line, freeread(input))
    if(freenext() , cp = freewordcut(" :",&cutter))
    { 
      found = 2 ; /* I did get something, I assume it is good ! */
      if (!matchTag(cp,&i))
	{ complain(messprintf("Unknown tag %s", cp)) ;
	  return 1 ;
	}
      freenext() ;
      freestep(':') ;
      freenext() ;
      freestep('\"') ;
      freenext() ;
      cp = freewordcut("\"",&cutter) ;
      if (cp)
	pushText(stack[i], cp) ;
    }
  else
    if(found) /* Allows several empty lines between objects */
      break ;

  /* cleanGenotype() ;*/
  return found ;
}

/***********************************************************************/

static void complain(char * errmess)
{ Stack sC ;
  int i ;
  fprintf (errFile,"line %d : \"%s\" \n",
	   line,errmess) ;
  for(i=0; i<MAXTAG; i++)
    { stackCursor(sC = stack[i], 0) ;
      if(i != JUNK && i != JUNK2 && !stackAtEnd(sC) )
	outStack(rejectFile,tag[i],sC) ;
  }
  fprintf(rejectFile,"\n") ;
}
 
/***********************************************************************/

static int getCh(char *text)
{

  char *cp ; int found ;

  cp = text ;
  
  while(*cp)
    switch (*cp++)
      {
      case 'I': case 'X': case 'V': case 'f': case 'A': 
      case ',': case '(': case ';': case ')':
	break ;
      default:
	return FALSE ;
      }
  
  found =stackMark(s) ;
  pushText(s,text) ;
  
  return found ;
}

/***********************************************************************/

static int getAllele(char *text)
{
  char *cp, keep ;
  int found = 0 ;
  
  cp = text ;
  if (*cp == ',')
    { cp++ ; text++ ; }
  while(*cp && *cp !='(') 
    cp++ ;
  if(*cp == '(')
    { cp++ ; text = cp ;}
  else
    return 0 ;
  while(*cp && *cp != ')' && *cp != ',')
    cp++ ;
  if(*cp == ')')
    {
      keep = *cp ;
      *cp = 0 ;
      found = stackMark(s) ;
      pushText(s,text) ;
      *cp++ = keep ;
    }
  return found ;
}

/***********************************************************************/

static int getGene(char *text)
{
  char *cp ;
  int found = 0 ;
  
  cp = text ;
  if(      *cp
     &&    islower(*cp++) 
     &&    islower(*cp++) 
     &&    islower(*cp++) 
     &&    (*cp++ == '-')
     )
    {
      found = stackMark(s) ;
      pushText(s,text) ;
    }
    
  return found  ;
}

/***********************************************************************/

static int getDef(char *text)
{
  char *cp, keep ;
  int found = 0 , type = 0;
  
  cp = text ;
          /* Lab identification */
  if (*cp == ',' || *cp == '/')
    { cp++ ; text++ ; }

  while(*cp  &&    islower(*cp))
    cp++ ;

  if(*cp == 'D' && *(cp+1) == 'f')  /* Rearrangement */
    { cp += 2 ;} 
  else if(*cp == 'D' && *(cp+1) == 'p')  /* Duplication */
    { cp += 2 ; type = 1 ;} 
  else if(*cp == 'C')    /* Compound */
    { cp++ ; }
  else if(*cp == 'T')    /* Translocation */
    { cp++ ; type = 3 ; }
  else
    return 0 ;

  while(*cp  &&  isdigit(*cp))
    { cp++ ; }

  keep = *cp ;
  *cp = 0 ;
  found = stackMark(s) ;
  pushText(s,text) ;
  *cp++ = keep ;
  
  return found ;
}

/***********************************************************************/

static BOOL  analyse(Array reverse)
{ char c = 'o', *cp ; int nGenes = arrayMax(genes) ;
  int i = arrayMax(reverse), allele, gene, ch, ch1, def ;

  allele = gene = def = 0 ;
  c = 'o' ;
  while(i--)
    { cp = arr(reverse, i, char*) ;
      switch (c)
      { case 'o':  /* out */
	  ch = getCh(cp) ;
          if(!ch)
	   { complain(messprintf("No chromosome : %s",cp)) ;
	     return FALSE ;
	   }
	  c = 'c' ;
	  break ;
	case 'c':
	  if (ch1 =getCh(cp))
	    { ch = ch1 ;
	      break ;
	    }
	  if (def = getDef(cp))
	    { array(genes,nGenes,FG).ch = ch ;
	      array(genes,nGenes,FG).def = def ;
	      allele = gene = def = 0 ;
	      c = 'c' ;
	      nGenes++ ;
	      
	      break ;
	    }
	  if (allele = getAllele(cp))
	    { c = 'a' ;
	      break ;
	    }
	  if (getGene(cp))
	    { array(genes,nGenes,FG).ch = ch ;
	      array(genes,nGenes,FG).a = allele ;
	      array(genes,nGenes,FG).g = gene ;
	      nGenes++ ;
	      allele = gene = def = 0 ;
	      c = 'c' ;
	      break ;
	    }
	    break ;
	case 'a':
	  gene = getGene(cp) ;
	  if (!gene)
	    { complain(messprintf("No gene : %s",cp)) ;
	      return FALSE ;
	    }
	  array(genes,nGenes,FG).ch = ch ;
	  array(genes,nGenes,FG).a = allele ;
	  array(genes,nGenes,FG).g = gene ;
          nGenes++ ;
	  allele = gene = def = 0 ;
	  c = 'c' ;
	}
    }
  return TRUE ;
}
/***********************************************************************/

static BOOL goodGenotype(void)
{
  Stack sG = stack[GENOTYPE] ;
  static Stack sG1 = 0 ;
  char *cp , c ; int i , np ;
  Array reverse = 0 ;
  BOOL beginning ;

  sG1 = stackReCreate(sG1, 100) ;
  stackCursor(sG,0) ;
  if(stackAtEnd(sG))
    return FALSE ;
    
  cp = stackText(sG, 0) ;
  pushText(sG1, cp) ;
    { freeforcecard(cp) ;     /* break on the ; */
      
      beginning = TRUE ; np = 0 ;
      while ((cp = freewordcut("(;)/.[]+ ", &c)) || c)
	switch (c)
	  { 
	  case '(': 
	    if (cp)
	      pushText(sG1, cp) ;
	    pushText(sG1, "(") ;
	   
	    if (cp = freewordcut(")", &c))
	      { catText(sG1, cp) ;
		catText(sG1,")") ;
	      }
	    else 
	      { complain ("Unbalanced or double parenthesis") ;
		return FALSE ;
	      }
	    break ;
	  case ')': 
	    complain ("Unbalanced or double parenthesis") ;
	    return FALSE ;
	    
	  case '[':   /* Comment, drop it from further analysis */
	    if (cp)
	      pushText(sG1, cp) ;
		
	    if (!freewordcut("]",&c))
	      { complain("Unbalanced []") ;
		return FALSE ;
	      }
	    break ;	
	  case ']':
	    complain("Unbalanced []") ;
	    return FALSE ;
	  default:
	    if (cp)
	      pushText(sG1, cp) ;
	    break ;
	  }
    }
	   
  stackDestroy(sG) ;
  sG = stack[GENOTYPE] = sG1 ; sG1 = 0 ;
  stackCursor (sG, 0) ;
  stackNextText(sG) ; /* jump full genotype */
  reverse = arrayReCreate(reverse, 24, char *) ;
  i = 0 ;
  while(cp = stackNextText(sG))
    array(reverse, i++, char*) = cp ;
 
  return
    analyse(reverse) ;
}

/***********************************************************************/

static BOOL goodRef(void)
{
  Stack sRef = stack[REF] ;
  char *cp, *cq, keep ;
  int i, n ;


  stackCursor(sRef,0) ;
  if(stackAtEnd(sRef))
    return TRUE ;
  cp = stackText(sRef,0) ;
  
  if (!strncmp(cp,"Class ", 6))   /* Class 3 */
    return TRUE ;
    
  if(!strncmp(cp, "REFERENCE ", 10))
    cp += 10 ;
  
  if(!strncmp(cp, "REF ", 4))
    cp += 4 ;
  
  if (!strncmp(cp,"Class ", 6))  /* REFERENCE Class 3 also exists */
    return TRUE ;
    
  if(!strncmp(cp, "ALLELE ", 6)) /* hope unique allele */
    { if ( arrayMax(genes) == 1 )
	{ arr(genes,0, FG).ref = TRUE ;
	  return TRUE ;
	}
      complain("REF ALLELE with not one gene ") ;
      return FALSE ;
    }

  cq = cp ; keep = 0 ;
  while(*cq)
    if (*cq == ',' || *cq == ' ')
      { keep = *cq ; *cq = 0 ; break ;}
    else
      cq++ ;
  for(i=0;i<arrayMax(genes); i++)
    if((n = arr(genes,i,FG).g ) || ( n = arr(genes,i,FG).def))
      if(!strcmp(cp,stackText(s,n)))
	{ arr(genes,i,FG).ref = TRUE ;
	  *cq = keep ;
	  return TRUE ;
	}
  *cq = keep ;
  fprintf(errFile,"line %d : Unrecognized reference : %s\n",line,cp) ;
  return FALSE ;
}

/***********************************************************************/

static BOOL goodMale(void)
{
  Stack sM = stack[MALE] ;
  char *cp ;

  stackCursor(sM,0) ;
  if(stackAtEnd(sM))
    return TRUE ;

  cp = stackText(sM,0) ;

  if(!strcmp(cp,"M") || !strcmp(cp,"MF"))
    return TRUE ;

  complain(messprintf("Bad male tag ##%s##",cp)) ;
  return FALSE ;
}

/***********************************************************************/

static BOOL goodStock(void)
{
  Stack sS = stack[STOCK] ;
  stackCursor(sS,0) ;
  return !stackAtEnd(sS) ;
}

/***********************************************************************/

static BOOL goodObj(void)
{ 
  genes = arrayReCreate(genes,10,FG) ;
  
  return
    goodStock() && goodMale() && goodGenotype() && goodRef();
}

/***********************************************************************/

static void aceObj(void)
{
  int i ;
  
  stackCursor(stack[GENOTYPE], 0) ;

  stackCursor(stack[STOCK], 0) ;

  outStack(outFile,"\n\nStrain",stack[STOCK]) ;
  fprintf (outFile,"-D Genotype\n") ;
  outItem (outFile,"Genotype",stackText(stack[GENOTYPE],0)) ;
  outStack (outFile,"Males",stack[MALE]) ;
  outStack (outFile,"Reference_Allele",stack[REF]) ;
  fprintf (outFile,"Location : CGC\n") ;

  if(arrayMax(genes))
    { 
      for(i=0; i< arrayMax(genes); i++)
	{
	  if(arr(genes,i,FG).g)
	    fprintf (outFile,"Gene : \"%s\"\n",
		     stackText(s,arr(genes,i,FG).g));
	  if(arr(genes,i,FG).a)
	    fprintf (outFile,"Allele : \"%s\"\n",
		     stackText(s,arr(genes,i,FG).a));
	  if(arr(genes,i,FG).def)
	      fprintf (outFile,"Rearrangement : \"%s\"\n", 
		       stackText(s,arr(genes,i,FG).def)) ;
	}
    }
 
       /* aceout the gene allele correspondance */
  if(arrayMax(genes))
    {
      for (i = 0 ; i < arrayMax(genes) ; i++)
	if(arr(genes,i,FG).g)
	  {
	    outItem(outFile,"\nGene",stackText(s,arr(genes,i,FG).g));
	    if (arr(genes,i,FG).a)
	      outItem(outFile,"Allele",stackText(s,arr(genes,i,FG).a));
	    if(arr(genes,i,FG).ch)
	      outItem(outFile,"gMap", 
		      stackText(s,arr(genes,i,FG).ch));
	    if (arr(genes,i,FG).ref)
	      outItem(outFile,"Reference_Allele",
		      stackText(s,arr(genes,i,FG).a));
	  }
	else if(arr(genes,i,FG).def)
	  {
	    outItem(outFile,
		    "\nRearrangement",
		    stackText(s,arr(genes,i,FG).def)) ;
	    
	    if(arr(genes,i,FG).ch)
		{ char *cp = stackText(s,arr(genes,i,FG).ch), *cq, old ;
		  int pass = 0 ;
		  if (*cp != '(')
		    outItem(outFile,"gMap",cp) ;	
		  else
		    while (*cp)
		      { cp++ ;
			cq = cp ;
			while (*cq && (*cq == 'A' || *cq == 'f' ||
				       *cq == 'I' || *cq == 'X' || *cq == 'V'))
			  cq++ ;
			old = *cq ; *cq = 0 ;
			if (!pass)
			  { if (cp && *cp) outItem(outFile,"gMap",cp) ;	 }
			else
			  { if (cp && *cp) outItem(outFile,"gMap2",cp) ;	}
			cp = cq ; *cq = old ; pass++ ;
		      }
		}
	    if (arr(genes,i,FG).ref)
	      outItem(outFile,"Reference_Strain",
		      stackText(stack[STOCK],0)) ;
	  }

      for (i = 0 ; i < arrayMax(genes) ; i++)
	if(arr(genes,i,FG).a && arr(genes,i,FG).ref)
	  {
	    outItem(outFile,"\nAllele",stackText(s,arr(genes,i,FG).a));
	    fprintf(outFile,"Reference_Allele\n");
	  }
      fprintf (outFile,"\n") ;
    }
}

/***********************************************************************/
 
void main (void)
{
  int nObj =0 , nErr = 0 ;
  char * acedb_data ;
  int i ;

  if (!(acedb_data = getenv ("ACEDB_DATA")))
    messcrash ("Env. variable ACEDB_DATA must be set to raw data directory") ;

  printf("Searching directory ACDEB_DATA: %s\n", acedb_data) ;
  filsetdir (acedb_data) ;

  input = stdin ;
  if (!(input = filopen ("cgcstrn","preace","r"))) return ;
  if (!(outFile = filopen ("cgcstrn","ace","w"))) return ;
  if (!(errFile = filopen ("cgcstrn","err","w"))) return ;
  if (!(rejectFile = filopen ("cgcstrn","rj","w"))) return ;
 
  buffer = stackCreate(1000) ;
  s = stackCreate(100) ;
  i = MAXTAG ;
  while (i--)
    stack[i] = stackCreate(8000) ;
      /* to be sure it is not relocated */

  printf("Begin stockace\n") ;
  while(TRUE)
    switch(getObj())
      {
      case 0:        /* end of file */
	goto fin ;

      case 1:        /* error */
	nObj ++;
	nErr++ ;
	break ;

      case 2:        /* good input */
	nObj ++;
	if(goodObj())
	  aceObj();
	else
	  nErr++ ;
	break ;
      }
  
 fin:
  printf ("%d entries processed\n%d errors\n",nObj, nErr) ;
  if (nErr)  printf ("See .err file)\n") ;
 
  fclose (input) ;
  fclose (outFile) ;
  fclose (errFile) ;
  fclose (rejectFile) ;
}
 
 
/****************************************************************/ 
/****************************************************************/ 




