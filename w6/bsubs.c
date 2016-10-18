/*  File: bsubs.c
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
 * Last edited: Sep 24 14:28 1999 (fw)
 * Created: Sun Jul  5 12:55:14 1992 (mieg)
 *-------------------------------------------------------------------
 */

/* $Id: bsubs.c,v 1.11 1999-09-24 13:30:56 fw Exp $ */

      /***************************************************************/
      /***************************************************************/
      /**  File Bsubs.c :                                           **/
      /**  Handles the B blocks of  the ACeDB program.              **/
      /***************************************************************/
      /***************************************************************/
      /*                                                             */
      /*     14xs routines are public :                              */
      /*     init, get, dump, prune,                                 */
      /*     Bnode2str/str2node,                                     */
      /*     Bnextkey,                                               */
      /*     Bbranchlen, freelen, newnode.                           */
      /*          .Durbin & J.Thierry-Mieg.                          */
      /*                    last modified  4/1/1991 by JTM.          */
      /*                                                             */
      /***************************************************************/

#ifdef ACEDB5
void compilersDoNotLike3 (void *emptyFiles) {return ; }
#else  /* !ACEDB5 */
/* only before ACEDB 5 */

#include "acedb.h"
#include "bs_.h"   /* Defines BS */

#include "b_.h"
#include "lex.h"
#include "pick.h"
#include "block.h"
#include "byteswap.h"

static void Bprune2(BP bp, NODE nn);

/* #define TESTBSUBS */

#ifdef TESTBSUBS
#include "bs.h"  /* bsCreate in Bdump */
#endif /* TESTBSUBS */


#ifdef ACEDB4
void swapBBlock(BP bp)
{ int i;
  bp->hat.h.disk = swapDISK(bp->hat.h.disk);
  bp->hat.h.nextdisk = swapDISK(bp->hat.h.nextdisk);
  bp->hat.h.session = swapInt(bp->hat.h.session);
  bp->hat.h.key = swapKEY(bp->hat.h.key);
  bp->hat.top = swapNODE(bp->hat.top);
  bp->hat.free = swapNODE(bp->hat.free);
  
  for( i=0; i<BNODEMAX; i++)
    { bp->n[i].d1 = swapNODE(bp->n[i].d1);
      bp->n[i].d2 = swapNODE(bp->n[i].d2);
    }


}
#endif /* ACEDB4 */

/**************************************************************/
                          /*initialise a new B */
                          /*called by B2BSstore2 */
     /* must not touch the block header */
void Binit (BP bp)
{ static struct bblock Bnull;
  int i;
  static int firstpass=1;
  
  if (firstpass)
    {     /*initialise Bnull*/
      memset (&Bnull, BLOC_SIZE, 0) ;
      Bnull.hat.top = 0 ;
      Bnull.hat.free = 1 ;
      Bnull.hat.h.type = 'B';
      for (i = 1 ; i < BNODEMAX-1 ; i++)
	Bnull.n[i].d1 = i+1 ;
      firstpass = 0 ;
    }
  *bp = Bnull ;
}

/**************************************************************/

                  /*given a key
                  * finds the origin of the correct branch
                  *  public; called by Bshow in BSsubs
                  */

void Bget (BP *bp, NODE *nn, KEY key)
{
  NODEP r;
  if (iskeyold (key))
    {
      blockpinn(key,bp) ;
      *nn=(*bp)->hat.top;
      
      while(*nn) {r=(*bp)->n+*nn;
#ifdef ACEDB4
		  if(maybeSwapKEY(r->ck.key)==key) break;
#else /* !ACEDB4 */
		  if(r->ck.key==key) break;
#endif /* !ACEDB4 */
		  *nn=r->d1;
		}
      
      /* if (*nn==0) on n a pas trouve alors que l on avait le bloc*/
      if (!*nn)
	messcrash("Block error in Bget(%s)",
		  name(key)) ;
    }
  else
    messcrash("Bget called on not old key %s",
		  name(key)) ;
}

/**************************************************************/

void Bdump (KEY key)
{
#ifdef TESTBSUBS
  int j ;
  BP bp;
  NODEP r;
  NODE i;
  int nblocks ;

  bp=0;
  if (!iskeyold(key))
    { messout("key %lu=%s never saved",key,name(key)) ;
      return ;
    }
  nblocks = blockpinn(key,&bp) ;

  do
    { printf ("Block dump of key %s:%s\n", className(key), name(key)) ;
      printf( " disk : %d   session : %d\n", bp->hat.h.disk, bp->hat.h.session),
      if (bp->hat.h.type == 'A')
	printf("  Array\n") ;
      else if (bp->hat.h.type == 'B')
	{ printf ("Bdump    bp=%ld, top=%d,free=%d, freelen=%d  searching %lu : \n",
		  bp,bp->hat.top,bp->hat.free,Bfreelen(bp),
		  key) ;
	  for (i = 1 ; i < BNODEMAX ; i++)
	    { r = bp->n + i ;
	      printf ("%3d l=%3d r=%3d  ", i, r->d1, r->d2) ;
	      for (j = 0 ; j < NODEX ; j++)
		printf ("%02x ", r->ck.c[j])
	      printf (" cp=%6s ",r->ck.c) ;
#ifdef ACEDB4
	      printf (" k=%lu=%s\n", 
		      maybeSwapKEY(r->ck.key), maybeSwapKEY(r->ck.key)) ;
#else  /* !ACEDB4 */
	      printf (" k=%lu=%s\n", r->ck.key, r->ck.key) ;
#endif /* !ACEDB4 */
	    }
	}
    }
  while (blockNext (&bp)) ;  /* do-while construction */
  blockunpinn (key) ;
  if (pickType (key) == 'B')
    { OBJ obj = bsCreate (key) ;
      if (obj) 
	bsDump (obj) ;
      bsDestroy (obj) ;
    }
#else  /* !TESTSUBS */
  messout ("To see the blocks recompile bsubs.c with #define TESTBSUBS") ;
#endif /* !TESTSUBS */
}

/**************************************************************/

void Bnode2str(char *cq, BP bp, NODEP *r)
{
  register char *cp;
  register int i ;

  /*    ((*r)->ck.key <=_LastC) Must be garanteed in the call */

  while ((*r)->d2)
    { 
      *r=bp->n+(*r)->d2 ;
      cp=(*r)->ck.c;
      i=NODEX;
      while (i--) 
	if(*cp) *cq++ = *cp++ ;	/* RD: ambiguous without spaces on SGI */
    }
  *cq = 0;    /*finishes the cq string*/
}

/**************************************************************/
      /* Called only from BSsubs*/


void Bstr2node(BS bs, BP bp, NODEP *r)
{
  NODEP rnew ;
  register char *cp, *cq;
  register int i=0 ;
  char nulChar = 0;
  
  if(bs->bt && bs->bt->cp)
    cp = bs->bt->cp ;
  else
    cp = &nulChar ;

     /* This is not while (*cp) because i want to
	go thru once on an empty string */
  while (TRUE)  
   {
     rnew = Bnewnode(bp)  ;
     (*r)->d2 = (NODE) (rnew - bp->n) ;
     *r = rnew ;
     cq=(*r)->ck.c;

/*     if ((*r)->ck.x.i)   
         invokeDebugger() ;  This check fails many times
*/     
     i=NODEX;
     while (*cp && i--) 
       *cq++ = *cp++ ;	/* RD: ambiguous without spaces on SGI */
      /* recall that Bnewnode memset *cq to 0 */
     if (!*cp)
       break ;
   }
  
  if (i > 0)
    *cq = 0 ;
}

/***************************************************************************/
void Bprune(BP bp, NODE nn,BOOL  bothsides)
{
  NODEP r=bp->n+nn;
  NODE n=bp->hat.top, nright = r->d1, n1 ;
  
  if(bothsides)
      nright = 0 ;
  else
      r->d1=0;

  if(n==nn) 
    bp->hat.top = nright ;
  else
    {
      while(n && (nn != (n1 = (bp->n+n)->d1 )))
	n  = n1 ;
      if (!n)
	messcrash ("Link list error in Bprune") ;
      (bp->n + n)->d1 = nright ;
    }

  Bprune2(bp,nn) ;
}

/***************************************************************************/

static void Bprune2(BP bp, NODE nn)
{
 NODEP r=bp->n+nn;

 if(r->d2)Bprune2(bp,r->d2);
 if(r->d1)Bprune2(bp,r->d1);
 r->d2=0;
 r->d1=bp->hat.free;
 bp->hat.free=nn;
 }

/**************************************************************/
NODEP Bnewnode(BP bp)
{
  NODEP r;
  BHAT b = &(bp->hat);	/* RD: ambiguous without spaces on SGI */
  
  if (!b->free)
    messcrash ("Bnew node : block overflow") ;
  r = bp->n + b->free ;
  b->free = r->d1 ;
  memset ((char *) r, 0, sizeof (NODE)) ;  
  return r ;
}

/***************************************************************************/
int Bbranchlen(BP bp,NODE nn)
{
 NODEP r=bp->n+nn;
 return( nn ?

       ( ( r->d1 ? Bbranchlen(bp,r->d1) : 0 )
        +( r->d2 ? Bbranchlen(bp,r->d2) : 0 )
        + 1
        )
              : 0   );
}
/***************************************************************************/
int Bfreelen(BP bp)
{
 register int i=0;
 register NODE nn=bp->hat.free;
 while(nn)
         {i++;
          nn=(bp->n+nn)->d1;
          }

return(i);
}
/***************************************************************************/
                   /* Gives, one by one the list of the
                    * keys of p, de la branche ainee.
                    * probably, the objects in p.
                    */

BOOL Bnextkey(void **it, BP p, KEY *kp)
{
  struct iterator {
    NODE nn;
    BP bp;
  } *iterator_real;
  NODEP r;
  
  if (!p) /* Abort iteration */
    {
      iterator_real = (struct iterator *)*it;
      messfree(iterator_real);
      *it = NULL;
      return FALSE;
    }

  if (*kp)
   {
     iterator_real = (struct iterator *)*it;
     if (!iterator_real || (iterator_real->bp != p)) 
       messcrash("WARNING : bad call to Bnextkey /n %s",
		 name(*kp));
   }
 else
   {
     iterator_real = messalloc(sizeof(struct iterator));
     iterator_real->bp = p;
     iterator_real->nn = p->hat.top;
     *it= (void *)iterator_real;
   }
  
  
  if (iterator_real->nn)
    { 
      r=p->n+iterator_real->nn;
      iterator_real->nn=r->d1;
#ifdef ACEDB4
      *kp=maybeSwapKEY(r->ck.key);
#else
      *kp=r->ck.key;
#endif
      return TRUE;
    }
  
  *kp=0;
  messfree(iterator_real);
  *it = NULL;

  return FALSE;
} /* Bnextkey */

#endif /* !ACEDB5 */


/************************* eof ******************************/
