/*  File: arraysub.c
 *  Author: Jean Thierry-Mieg (mieg@mrc-lmba.cam.ac.uk)
 *  Copyright (C) J Thierry-Mieg and R Durbin, 1991
 * -------------------------------------------------------------------
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 * -------------------------------------------------------------------
 * This file is part of the ACEDB genome database package, written by
 * 	Richard Durbin (MRC LMB, UK) rd@mrc-lmba.cam.ac.uk, and
 *	Jean Thierry-Mieg (CRBM du CNRS, France) mieg@crbm.cnrs-mop.fr
 *
 * Description:
 *              Arbitrary length arrays, stacks, associators
 *              line breaking and all sorts of other goodies
 *              These functions are declared in array.h
 *               (part of regular.h - the header for libfree.a)
 * Exported functions:
 *              See Header file: array.h (includes lots of macros)
 * HISTORY:
 * Last edited: Aug 13 10:35 2007 (edgrif)
 * * May 19 13:51 1999 (fw): make MEM_DEBUG the default for all
 *              ops that demand memory, so we can track where the
 *              code made that request.
 * * Nov  1 16:11 1996 (srk)
 *		-	MEM_DEBUG code clean-up 
 *                      (some loose ends crept in from WIN32)
 *		-	int (*order)(void*,void*) prototypes used 
 *                                            uniformly throughout
 * * Jun  5 00:48 1996 (rd)
 * * May  2 22:33 1995 (mieg): killed the arrayReport at 20000
      otherwise it swamps 50 Mbytes of RAM
 * * Jan 21 16:25 1992 (mieg): messcrash in uArrayCreate(size<=0)
 * * Dec 17 11:40 1991 (mieg): stackTokeniseTextOn() tokeniser.
 * * Dec 12 15:45 1991 (mieg): Stack magic and stackNextText
 * Created: Thu Dec 12 15:43:25 1989 (mieg)
 * CVS info:   $Id: arraysub.c,v 1.72 2007-08-13 09:36:11 edgrif Exp $
 *-------------------------------------------------------------------
 */


   /* Warning : if you modify Array or Stack structures or 
      procedures in this file or array.h, you may need to modify 
      accordingly the persistent array package asubs.c.
   */


#include "regular.h"
#include <wh/bitset.h>

extern BOOL finalCleanup ;	/* in memsubs.c */

/********** Array : class to implement variable length arrays **********/

static int totalAllocatedMemory = 0 ;
static int totalNumberCreated = 0 ;
static int totalNumberActive = 0 ;
static Array reportArray = 0 ;
static void uArrayFinalise (void *cp) ;

Array uArrayCreate_dbg (int n, int size, STORE_HANDLE handle,
			const char *hfname,int hlineno) 
{
  int id = totalNumberCreated++ ;
  Array new = (Array) handleAlloc_dbg (uArrayFinalise, 
				       handle,
				       sizeof (struct ArrayStruct),
				       hfname, hlineno) ;

  if (!reportArray)
    { reportArray = (Array)1 ; /* prevents looping */
      reportArray = arrayCreate (512, Array) ;
    }
  if (size <= 0)
    messcrash("negative size %d in uArrayCreate", size) ;
  if (n < 1)
    n = 1 ;
  totalAllocatedMemory += n * size ;

  /* base-mem isn't allocated upon handle, it's free'd by finalisation */
  new->base = halloc_dbg (n*size, 0, hfname, hlineno) ;
  new->dim = n ;
  new->max = 0 ;
  new->size = size ;
  new->id = ++id ;
  new->magic = ARRAY_MAGIC ;
  totalNumberActive++ ;
  if (reportArray != (Array)1) 
    { if (new->id < 20000)
	array (reportArray, new->id, Array) = new ;
      else
	{ Array aa = reportArray ;
	  reportArray = (Array)1 ; /* prevents looping */
	  arrayDestroy (aa) ;
	}
    }
  return new ;
}

/**************/

int arrayReportMark (void)
{
  return reportArray != (Array)1 ?  
      arrayMax(reportArray) : 0 ;
}

/**************/

void arrayReport (int j)
{ int i ;
  Array a ;

 if (reportArray == (Array)1) 
   { fprintf(stderr,
	     "\n\n %d active arrays, %d created, %d kb allocated\n\n ",   
	     totalNumberActive,   totalNumberCreated , totalAllocatedMemory/1024) ;
     return ;
   }
  
  fprintf(stderr,"\n\n") ;
  
  i = arrayMax (reportArray) ;
  while (i-- && i > j)
    { a = arr (reportArray, i, Array) ;
      if (arrayExists(a))
	fprintf (stderr, "Array %d  size=%d max=%d\n", i, a->size, a->max) ;
    }
}

/**************/

void arrayStatus (int *nmadep, int *nusedp, int *memAllocp, int *memUsedp)
{ 
  int i ;
  Array a, *ap ;

  *nmadep = totalNumberCreated ; 
  *nusedp = totalNumberActive ;
  *memAllocp = totalAllocatedMemory ;
  *memUsedp = 0 ;

  if (reportArray == (Array)1) 
    return ;

  i = arrayMax(reportArray) ;
  ap = arrp(reportArray, 0, Array) - 1 ;
  while (ap++, i--)
    if (arrayExists (*ap))
      { a = *ap ;
	*memUsedp += a->max * a->size ;
      }
}

/**************/

Array uArrayReCreate_dbg (Array a, int n, int size,
			  const char *hfname, int hlineno)
{
  if (!arrayExists(a))
    return  uArrayCreate_dbg(n, size, 0, hfname, hlineno) ;

  if(a->size != size)
    uMessSetErrorOrigin(hfname, hlineno),
      uMessCrash ("Type missmatch in arrayReCreate, you should always "
		  "call recreate using the same type") ;

  if (n < 1)
    n = 1 ;
  if (a->dim < n || 
      (a->dim - n)*size > (1 << 19) ) /* free if save > 1/2 meg */
    { totalAllocatedMemory -= a->dim * size ;
      messfree (a->base) ;
      a->dim = n ;
      totalAllocatedMemory += a->dim * size ;
      /* base-mem isn't alloc'd on handle, it's free'd by finalisation */
      a->base = halloc_dbg (a->dim*size, 0, hfname, hlineno) ;
    }
  memset(a->base,0,(mysize_t)(a->dim*size)) ;

  a->max = 0 ;
  return a ;
}

/**************/

void uArrayDestroy_dbg (Array a,
			const char *hfname, int hlineno)
/* Note that the finalisation code attached to the memory does the work, 
   see below */
{
  if (!a) return;

  if (a->magic != ARRAY_MAGIC)
    uMessSetErrorOrigin(hfname, hlineno),
      uMessCrash ("arrayDestroy received corrupt array->magic");

  messfree(a);
}

static void uArrayFinalise (void *cp)
{ Array a = (Array)cp;

  totalAllocatedMemory -= a->dim * a->size ;
  if (!finalCleanup) messfree (a->base) ;
  a->magic = 0 ;
  totalNumberActive-- ;
  if (!finalCleanup && reportArray != (Array)1) 
    arr(reportArray, a->id, Array) = 0 ;

  return;
} /* uArrayFinalise */

/******************************/

void arrayExtend_dbg (Array a, int n, const char *hfname,int hlineno) 
{
  char *new ;

  if (!a || n < a->dim)
    return ;

  totalAllocatedMemory -= a->dim * a->size ;
  if (a->dim*a->size < 1 << 23)  /* 2 megs of keys, or 8 megs of ram */
    a->dim *= 2 ;
  else
    a->dim += 1024 + ((1 << 23) / a->size) ;
  if (n >= a->dim)
    a->dim = n + 1 ;

  totalAllocatedMemory += a->dim * a->size ;

  /* base-mem isn't allocated upon handle, it's free'd by finalisation */
  new = halloc_dbg (a->dim*a->size, 0, hfname, hlineno) ;
  memcpy (new,a->base,a->size*a->max) ;
  messfree (a->base) ;
  a->base = new ;

  return;
} /* arrayExtend_dbg */

/***************/

char *uArray_dbg (Array a, int i, const char *hfname, int hlineno)
{
  if (i < 0)
    uMessSetErrorOrigin(hfname, hlineno),
      uMessCrash ("referencing array element %d < 0", i) ;

  if (!a)
    uMessSetErrorOrigin(hfname, hlineno),
      uMessCrash ("uArray called with NULL Array struc");

  if (i >= a->max)
    { if (i >= a->dim)
        arrayExtend (a,i) ;
      a->max = i+1 ;
    }
  return a->base + i*a->size ;
}

/***************/

char *uArrayCheck_dbg (Array a, int i, const char *hfname,int hlineno)
{
  if (i < 0)
    uMessSetErrorOrigin(hfname, hlineno),
      uMessCrash ("referencing array element %d < 0", i) ;

  return uArray_dbg(a, i, hfname, hlineno) ;
}

/***************/

char *uArrCheck_dbg (Array a, int i, const char *hfname, int hlineno)
{
  if (i >= a->max || i < 0)
    uMessSetErrorOrigin(hfname, hlineno),
      uMessCrash ("array index %d out of bounds [0,%d]",
		  i, a->max - 1) ;

  return a->base + i*a->size ;
}

/**************/

Array arrayCopy_dbg(Array old, STORE_HANDLE handle,
		    const char *hfname, int hlineno) 
{
  Array new ;
  
  if (arrayExists (old) && old->size)
    {
      new = uArrayCreate_dbg (old->dim, old->size, handle,
			      hfname, hlineno) ;
      memcpy(new->base, old->base, old->dim * old->size);
      new->max = old->max ;
      return new;
    }

  return (Array)0 ;
} /* arrayCopy_dbg */

/**************/

Array arrayTruncatedCopy_dbg (Array a, int x1, int x2,
			      const char *hfname, int hlineno)
{ Array b = 0 ;
  
  if (x1 < 0 || x2 < x1 || x2 > a->max)
    messcrash 
      ("Bad coordinates x1 = %d, x2 = %d in arrayTruncatedCopy",
       x1, x2) ;
  if (arrayExists (a) && a->size)
    { if (x2 - x1)
	{ b = uArrayCreate_dbg (x2 - x1, a->size, 0, hfname, hlineno) ;
	  b->max = x2 - x1 ;
	  memcpy(b->base, a->base + x1, b->max * b->size);
	}
      else
	b = uArrayCreate_dbg (10, a->size, 0, hfname, hlineno) ;
    }
  return b;
}

/**************/

void arrayCompress(Array a)
{
  int i, j, k , as ;
  char *x, *y, *ab  ;
  
  if (!a || !a->size || arrayMax(a) < 2 )
    return ;

  ab = a->base ; 
  as = a->size ;
  for (i = 1, j = 0 ; i < arrayMax(a) ; i++)
    { x = ab + i * as ; y = ab + j * as ;
      for (k = a->size ; k-- ;)		
	if (*x++ != *y++) 
	  goto different ;
      continue ;
      
    different:
      if (i != ++j)
	{ x = ab + i * as ; y = ab + j * as ;
	  for (k = a->size ; k-- ;)	 
	    *y++ = *x++ ;
	}
    }
  arrayMax(a) = j + 1 ;
}

/****************/

/* 31.7.1995 dok408  added arraySortPos() - restricted sorting to tail of array */
void arraySort(Array a, int (*order)(void*, void*))
{
  arraySortPos(a, 0, order) ;

  return ;
}

void arraySortPos (Array a, int pos, int (*order)(void*, void*))
{
  typedef int (*CONSTORDER)(const void*, const void*) ;
  unsigned int n = a->max - pos, s = a->size ;
  void *v = a->base + pos * a->size ;
 
  if (pos < 0)
    messcrash("arraySortPos: pos = %d", pos);

  if (n > 1) 
    {
#ifdef SOLARIS
      /* Jean reported that qsort() crashes on solaris 5.7 */
      mSort (v, n, s, order) ;
#else
      qsort (v, n, s, (CONSTORDER)order) ;
#endif
    }

  return ;
}

/***********************************************************/

BOOL arrayIsEntry_dbg (Array a, int i, void *s,
		       const char *hfname, int hlineno)
{
  char *cp = uArray_dbg(a,i, hfname, hlineno), *cq = s ;
  int j = a->size;

  while (j--)
    if (*cp++ != *cq++) 
      return FALSE ;
  return TRUE;
}

/***********************************************/
       /* Finds Entry s from Array  a
        * sorted in ascending order of order()
        * If found, returns TRUE and sets *ip
        * if not, returns FALSE and sets *ip one step left
        */

BOOL arrayFind(Array a, void *s, int *ip, int (* order)(void*, void*))
{
  int ord;
  int i = 0 , j = arrayMax(a), k;

  if(!j || (ord = order(s,uArray_dbg(a,0, __FILE__, __LINE__)))<0)
    { if (ip)
	*ip = -1; 
      return FALSE;
    }   /* not found */

  if (ord == 0)
    { if (ip)
	*ip = 0;
      return TRUE;
    }

  if ((ord = order(s,uArray_dbg(a,--j, __FILE__, __LINE__)))>0 )
    { if (ip)
	*ip = j; 
      return FALSE;
    }
  
  if (ord == 0)
    { if (ip)
	*ip = j;
      return TRUE;
    }

  while(TRUE)
    { k = i + ((j-i) >> 1) ; /* midpoint */
      if ((ord = order(s, uArray_dbg(a,k, __FILE__, __LINE__))) == 0)
	{ if (ip)
	    *ip = k; 
	  return TRUE;
	}
      if (ord > 0) 
	(i = k);
      else
	(j = k) ;
      if (i == (j-1) )
        break;
    }
  if (ip)
    *ip = i ;
  return FALSE;
}

/**************************************************************/
       /* Removes Entry s from Array  a
        * sorted in ascending order of order()
        */

BOOL arrayRemove (Array a, void * s, int (* order)(void*, void*))
{
  int i;

  if (arrayFind(a, s, &i,order))
    {
      /* memcpy would be faster but regions overlap
       * and memcpy is said to fail with some compilers
       */
      char *cp = uArray_dbg(a,i, __FILE__, __LINE__),  *cq = cp + a->size ;
      int j = (arrayMax(a) - i)*(a->size) ;
      while(j--)
	*cp++ = *cq++;

      arrayMax(a) --;
      return TRUE;
    }
  else

    return FALSE;
}

/**************************************************************/
/* Insert Segment s in Array  a
 * in ascending order of s.begin
 * 
 * Note use of arrayFind() which returns:
 * 
 * -1 if the new element should at the front of the array
 *
 * 0 < arrayMax() if it should be somewhere in the array
 *
 * arrayMax() if it should be at the end of the array
 * 
 */
BOOL arrayInsert(Array a, void * s, int (*order)(void*, void*))
{
  int i, j, arraySize;

  if (arrayFind(a, s, &i,order))
    return FALSE;  /* no doubles */

  arraySize = arrayMax(a);
  j = arraySize + 1;


  uArray_dbg(a, j-1, __FILE__, __LINE__) ; /* to create space */

  /* avoid memcpy for same reasons as above */
  {
    char *cp, *cq;
    int k, index ;

    /* If the new element should be at the front of the array, we must set the index correctly
     * otherwise the copy below goes off the _front_ of the array because we are subtracting -1,
     * i.e. adding 1 to k which controls how many bytes we access/copy. */
    if (i == -1)
      index = 0 ;
    else
      index = i ;
    
    if (arraySize > 0)
      { 
	cp = uArray_dbg(a, j - 1,__FILE__, __LINE__) + a->size - 1 ;

	cq = cp - a->size ;

	k = (j - index - 1) * (a->size) ;
	while(k--)
	  *cp-- = *cq--;
      }
    
    cp = uArray_dbg(a,i+1,__FILE__,__LINE__); 
    cq = (char *) s; 
    k = a->size;
    while(k--)
      *cp++ = *cq++;
  }

  return TRUE;
}

/********* BitSet - inherits from Array **********/
/* mieg 2001, turn these into function to systematically make room
   allowing to use bit() without errors
   which is ok since anyway bitSetMax is not a defined method
*/
Array bitSetCreate (int n, STORE_HANDLE h)
{
  Array bb  = 0 ;
  if (n < 256) n = 256 ;
  bb = arrayHandleCreate (1 + (n >> 5), unsigned int, h) ;
  array (bb, (n >> 5), unsigned int) = 0 ;
  return bb ;
}

Array bitSetReCreate (Array bb, int n) 
{
  if (n < 256) n = 256 ;
  bb = arrayReCreate (bb, 1 + (n >> 5), unsigned int) ;
  array (bb, (n >> 5), unsigned int) = 0 ;
  return bb ;
}

void bitExtend (Array bb, int n)
{
  if (n < 256) n = 256 ;
  arrayExtend(bb, n >> 5) ;
  array (bb, (n >> 5), unsigned int) = 0 ;
}

                  /* Returns the number of set bits */
int bitSetCount (BitSet a)
{
  register unsigned int *cp ;
  register unsigned int i, i1;
  register int j = arrayExists(a) ? arrayMax(a) : 0 , n = 0 ;

  if(!j)
    return 0 ;
  cp  = arrp(a,0,unsigned int) ;
  while (j--)
    { if(!*cp) n+=32 ;
      else if(*cp != 0xff)
	for (i=1, i1 = 0 ; i1 < 32 ; i1++, i <<= 1) if(~(*cp) & i) n++;
      cp++;
    }
  return 32*arrayMax(a) - n ;
}

/******** Stack : arbitrary Stack class - inherits from Array *********/

static void uStackFinalise (void *cp) ;

Stack stackHandleCreate_dbg (int n,  /* n is initial size */
			     STORE_HANDLE handle, 
			     const char *hfname, int hlineno)
{
  Stack s = (Stack) handleAlloc_dbg (uStackFinalise, 
				     handle,
				     sizeof (struct StackStruct),
				     hfname, hlineno) ;
  s->magic = STACK_MAGIC ;
  /* array is not allocated upon handle, it's free'd by finalisation */
  s->a = arrayCreate (n,char) ;

  s->pos = s->ptr = s->a->base ;
  s->safe = s->a->base + s->a->dim - 16 ; /* safe to store objs up to size 8 */
  s->textOnly = FALSE;

  return s ;
} /* stackHandleCreate_dbg */

void stackTextOnly(Stack s)
{ if (!stackExists(s))
    messcrash("StackTextOnly given non-exisant stack.");

  s->textOnly = TRUE;
}

/* performs b1 = b1 & b2, returns count (b1) */
int bitSetAND (BitSet b1, BitSet b2)
{
  register unsigned int *cp, *cq ;
  register unsigned int ii, i1;
  register int n = 0 ;
  register int i = arrayExists(b1) ? arrayMax(b1) : 0 ;
  register int j = arrayExists(b2) ? arrayMax(b2) : 0 ;

  if (i > j) i = j ;
  arrayMax (b1) = i ;
  if (i == 0)
    return 0 ;

  cp  = arrp(b1,0,unsigned int) ;
  cq  = arrp(b2,0,unsigned int) ;
  while (i--)
    { 
      *cp &= *cq ;
      if(!*cp) n+=32 ;
      else if(*cp != 0xff)
	for (ii=1, i1 = 0 ; i1 < 32 ; i1++, ii <<= 1) if(~(*cp) & ii) n++;
      cp++; cq++ ;
    }
  return 32*arrayMax(b1) - n ;
} /* bitSetAND */

/* performs b1 = b1 & b2, returns count (b1) */
int bitSetOR (BitSet b1, BitSet b2)
{
  register unsigned int *cp, *cq ;
  register unsigned int ii, i1;
  register int n = 0 ;
  register int i = arrayExists(b1) ? arrayMax(b1) : 0 ;
  register int j = arrayExists(b2) ? arrayMax(b2) : 0 ;

  if (i < j)
    {
      i = j ;
      array(b1,i - 1,unsigned int) = 0 ;
    }
  if (i > j)
    array(b2, i - 1,unsigned int) = 0 ;


  if (i == 0)
    return 0 ;

  cp  = arrp(b1,0,unsigned int) ;
  cq  = arrp(b2,0,unsigned int) ;
  while (i--)
    { 
      *cp |= *cq ; /* *cp OR *cq */
      if(!*cp) n+=32 ;
      else if(*cp != 0xff)
	for (ii=1, i1 = 0 ; i1 < 32 ; i1++, ii <<= 1) if(~(*cp) & ii) n++;
      cp++; cq++ ;
    }
  return 32*arrayMax(b1) - n ;
} /* bitSetOR */

/* performs b1 = b1 & b2, returns count (b1) */
int bitSetXOR (BitSet b1, BitSet b2)
{
  register unsigned int *cp, *cq ;
  register unsigned int ii, i1;
  register int n = 0 ;
  register int i = arrayExists(b1) ? arrayMax(b1) : 0 ;
  register int j = arrayExists(b2) ? arrayMax(b2) : 0 ;

  if (i < j)
    {
      i = j ;
      array(b1,i - 1,unsigned int) = 0 ;
    }
  if (i > j)
    array(b2, i - 1,unsigned int) = 0 ;


  if (i == 0)
    return 0 ;

  cp  = arrp(b1,0,unsigned int) ;
  cq  = arrp(b2,0,unsigned int) ;
  while (i--)
    { 
      *cp ^= *cq ; /* *cp XOR *cq */
      if(!*cp) n+=32 ;
      else if(*cp != 0xff)
	for (ii=1, i1 = 0 ; i1 < 32 ; i1++, ii <<= 1) if(~(*cp) & ii) n++;
      cp++; cq++ ;
    }
  return 32*arrayMax(b1) - n ;
} /* bitSetXOR */

/* performs b1 = b1 & b2, returns count (b1) */
int bitSetMINUS (BitSet b1, BitSet b2)
{
  register unsigned int *cp, *cq ;
  register unsigned int ii, i1;
  register int n = 0 ;
  register int i = arrayExists(b1) ? arrayMax(b1) : 0 ;
  register int j = arrayExists(b2) ? arrayMax(b2) : 0 ;

  if (i < j)
    {
      i = j ;
      array(b1,i - 1,unsigned int) = 0 ;
    }
  if (i > j)
    array(b2, i - 1,unsigned int) = 0 ;


  if (i == 0)
    return 0 ;

  cp  = arrp(b1,0,unsigned int) ;
  cq  = arrp(b2,0,unsigned int) ;
  while (i--)
    { 
      *cp = *cp ^ (*cp & *cq) ; /* *cp XOR ( *cp AND *cq) */
      if(!*cp) n+=32 ;
      else if(*cp != 0xff)
	for (ii=1, i1 = 0 ; i1 < 32 ; i1++, ii <<= 1) if(~(*cp) & ii) n++;
      cp++; cq++ ;
    }
  return 32*arrayMax(b1) - n ;
} /* bitSetMINUS */


Stack stackReCreate_dbg (Stack s, int n,  /* n is initial size */
			 const char *hfname, int hlineno)
{
  if (!stackExists(s))
    return stackHandleCreate_dbg(n, 0, hfname, hlineno) ;

  /* array is not allocated upon handle, it's free'd by finalisation */
  s->a = uArrayReCreate_dbg (s->a,n,sizeof(char), hfname, hlineno) ;

  s->pos = s->ptr = s->a->base ;
  s->safe = s->a->base + s->a->dim - 16 ; /* safe to store objs up to size 8 */

  return s ;
} /* stackReCreate_dbg */

Stack stackCopy_dbg (Stack old, STORE_HANDLE handle,
		     const char *hfname, int hlineno)
{
  Stack new ;
  int ptr, pos;

  if (!stackExists(old))
    return 0 ;

  ptr = old->ptr - old->a->base;
  pos = old->pos - old->a->base;

  new = (Stack) handleAlloc_dbg (uStackFinalise, handle,
                                 sizeof (struct StackStruct),
                                 hfname, hlineno) ;
  new->magic = STACK_MAGIC;

  /* array is not allocated upon handle, it's free'd by finalisation */
  new->a = arrayCopy_dbg (old->a, 0, hfname, hlineno) ;

  new->ptr = new->a->base + ptr;
  new->pos = new->a->base + pos;
  new->safe = new->a->base + new->a->dim - 16 ;
  new->textOnly = old->textOnly;

  return new ;
} /* stackCopy_dbg */

#ifdef FW_990519_NOT_DECLARED_OR_USED_ANYWHERE
Stack arrayToStack (Array a)
{ 
  Stack s ;
  int n ;

  if (!arrayExists(a) || a->size != 1 )
    return 0 ;
    
  n = arrayMax(a) ;
  s = stackCreate(n  + 32) ;
              
  memcpy(s->a->base, a->base, n) ;
                
  s->pos = s->a->base ;
  s->ptr = s->a->base + n ;
  s->safe = s->a->base + s->a->dim - 16 ; /* safe to store objs up to size 8 */
  while ((long)s->ptr % STACK_ALIGNMENT)
    *(s->ptr)++ = 0 ;   
  return s ;
}
#endif /* FW_990519_NOT_DEFINE_OR_USED_ANYWHERE */

void uStackDestroy(Stack s)
{ if (s && s->magic == STACK_MAGIC) messfree(s);
} /* the rest is done below as a consequence */

static void uStackFinalise (void *cp)
{ Stack s = (Stack)cp;
  if (!finalCleanup) arrayDestroy (s->a) ;
  s->magic = 0 ;
}

void stackExtend_dbg (Stack s, int n,
		      const char *hfname, int hlineno)
{
  int ptr, pos;

  if (!stackExists(s))
    uMessSetErrorOrigin (hfname, hlineno), uMessCrash("stackExtend() - called with invalid stack");

  ptr = s->ptr - s->a->base;
  pos = s->pos - s->a->base ;

  s->a->max = s->a->dim ;	/* since only up to ->max copied over */
  arrayExtend_dbg (s->a,ptr+n+16, hfname, hlineno);	/* relies on arrayExtend mechanism */
  s->ptr = s->a->base + ptr ;
  s->pos = s->a->base + pos ;
  s->safe = s->a->base + s->a->dim - 16 ;

  return;
} /* stackExtend_dbg */

int stackMark (Stack s)
{
  int mark;

  if (!stackExists(s))
    messcrash("stackMark() - received invalid stack");

  mark = (s->ptr - s->a->base);
  
  return mark;
} /* stackMark */

int stackPos (Stack s)
{ 
  int pos;

  if (!stackExists(s))
    messcrash("stackPos() - received invalid stack");
  
  pos = (s->pos - s->a->base);

  return pos;
} /* stackPos */

void stackCursor (Stack s, int mark)
{ 
  if (!stackExists(s))
    messcrash("stackCursor() - received invalid stack");

  s->pos = s->a->base + mark ;

  return;
} /* stackCursor */

char *stackText_dbg (Stack s, int mark, const char *hfname, int hlineno)
{
  char *text;

  if (!stackExists(s))
    uMessSetErrorOrigin (hfname, hlineno), uMessCrash("stackText() - called with invalid stack");

  text = (char*)(s->a->base + mark);

  return text;
} /* stackText */

/************************************************************/

/* access doubles on the stack by steam on systems where we don't
   align the stack strictly enough, this code assumes that ints are 32bits
   and doubles 64 */

union mangle { double d;
	       struct { int i1;
			int i2;
		      } i;
	     };

void uStackDoublePush(Stack stk, double x)
{ union mangle m;

  m.d = x;
  push(stk, m.i.i1, int);
  push(stk, m.i.i2, int);
}

double uStackDoublePop(Stack stk)
{ union mangle m;

  m.i.i2 = pop(stk, int);
  m.i.i1 = pop(stk, int);

  return m.d;
}

double uStackDoubleNext(Stack stk)
{ union mangle m;

  m.i.i1 = stackNext(stk, int);
  m.i.i2 = stackNext(stk, int);

  return m.d;
}

void pushText (Stack s, char* text)
{
  while (s->ptr + strlen(text)  > s->safe) 
    stackExtend (s,strlen(text)+1) ;
  while ((*(s->ptr)++ = *text++)) ;
  if (!s->textOnly)
    while ((long)s->ptr % STACK_ALIGNMENT)
      *(s->ptr)++ = 0 ;   
}

char* popText (Stack s)
{
  char *base = s->a->base ;

  while (s->ptr > base && !*--(s->ptr)) ;
  while (s->ptr >= base && *--(s->ptr)) ;
  return ++(s->ptr) ;
}

void catText (Stack s, char* text)
{
  while (s->ptr + strlen(text) > s->safe)
    stackExtend (s,strlen(text)+1) ;
  *s->ptr = 0 ;
  while (s->ptr >= s->a->base && *s->ptr == 0)
    s->ptr -- ;
  s->ptr ++ ;
  while ((*(s->ptr)++ = *text++)) ;
  if (!s->textOnly)
    while ((long)s->ptr % STACK_ALIGNMENT)
      *(s->ptr)++ = 0 ;   
}

void catBinary (Stack s, char* data, int size)
{
  int total;
  total = size + 1;

  while (s->ptr + total > s->safe)
    stackExtend (s,size+1) ;
  memcpy(s->ptr,data,size);
  s->ptr += size;
}

char* stackNextText (Stack s)
{ char *text = s->pos ;
  if (text>= s->ptr)
    return 0 ;  /* JTM, so while stackNextText makes sense */
  while (*s->pos++) ;
  if (!s->textOnly)
    while ((long)s->pos % STACK_ALIGNMENT)
      ++s->pos ;
  return text ;
}

/*********/
     /* Push text in stack s, after breaking it on delimiters */
     /* You can later access the tokens with command
	while (token = stackNextText(s)) work on your tokens ;
	*/
void  stackTokeniseTextOn(Stack s, char *text, char *delimiters)
{
  char *cp, *cq , *cend, *cd, old, oldend ;
  int i, n ;

  if(!stackExists(s) || !text || !delimiters)
    messcrash("stackTextOn received some null parameter") ;

  n = strlen(delimiters) ;
  cp = cq  = text ;
  while(TRUE)
    {
      while(*cp == ' ')
	cp++ ;
      cq = cp ;
      old = 0 ;
      while(*cq)
	{ for (cd = delimiters, i = 0 ; i < n ; cd++, i++)
	    if (*cd == *cq)
	      { old = *cq ;
		*cq = 0 ;
		goto found ;
	      }
	  cq++ ;
	}
    found:
      cend = cq ;
      while(cend > cp && *--cend == ' ') ;
      if (*cend != ' ') cend++ ;
      oldend = *cend ; *cend = 0 ;
      if (*cp && cend > cp)
	pushText(s,cp) ;
      *cend = oldend ;
      if(!old)
	{ stackCursor(s, 0) ;
	  return ;
	}
      *cq = old ;
      cp = cq + 1 ;
    }
}

void stackClear (Stack s)
{ 
  if (!s) messcrash("stackClear() - NULL stack");
  if (!stackExists(s)) messcrash("stackClear() - non-magic Stack");

  memset(s->a->base,0,(mysize_t)(s->a->dim*sizeof(char))) ;

  s->pos = s->ptr = s->a->base;
  s->a->max = 0;

  return;
} /* stackClear */

/****************** routines to set text into lines ***********************/

static char *linesText ;
static Array lines ;
static Array textcopy ;
static int kLine, popLine ;

/**********/

int uLinesText (char *text, int width)
{
  char *cp,*bp ;
  int i ;
  int nlines = 0 ;
  int length = strlen (text) ;
  int safe = length + 2*(length/(width > 0 ? width : 1) + 1) ; /* mieg: avoid zero divide */
  static int isFirst = TRUE ;

  if (isFirst)
    { isFirst = FALSE ;
      lines = arrayCreate(16,char*) ;
      textcopy = arrayCreate(128,char) ;
    }

  linesText = text ;
  array(textcopy,safe,char) = 0 ;   /* ensures textcopy is long enough */

  if (!*text)
    { nlines = popLine = kLine = 0 ;
      array(lines,0,char*) = 0 ;
      return 0 ;
    }

  cp = textcopy->base ;
  nlines = 0 ;
  for (bp = text ; *bp ; ++bp)
    { array(lines,nlines++,char*) = cp ;
      for (i = 0 ; (*cp = *bp) && *cp != '\n' ; ++i, ++cp, ++bp)
        if (i == width)		/* back up to last space */
          { while (i--)
	      { --bp ; --cp ;
		if (*cp == ' ' || *cp == ',' || *cp == ';')
		  goto eol ;
	      }
	    cp += width ;	/* no coma or spaces in whole line ! */
	    bp += width ;
	    break ;
	  }
eol:  if (!*cp)
        break ;
      if (*cp != '\n')
	++cp ;
      *cp++ = 0 ;
    }
  kLine = 0 ;			/* reset for uNextLine() */
  popLine = nlines ;
  array(lines,nlines,char*) = 0 ; /* 0 terminate */

  return nlines ;
 }

char *uNextLine (char *text)
 {
   if (text != linesText)
     messout ("Warning : uNextLine being called with bad context") ;
   return array(lines,kLine++,char*) ;
 }

char *uPopLine (char *text)
 {
   if (text != linesText)
     messout ("Warning : uPopLine being called with bad context") ;
   if (popLine)
     return array(lines,--popLine,char*) ;
   else return 0;
 }

char **uBrokenLines (char *text, int width)
{
  uLinesText (text,width) ;
  return (char**)lines->base ;
}

char *uBrokenText (char *text, int width)
{
  char *cp ;

  uLinesText (text,width) ;
  uNextLine (text) ;
  while ((cp = uNextLine (text)))
    *(cp-1) = '\n' ;
  return arrp(textcopy,0,char) ;
}

/*************************************************************/
/* perfmeters */
int assBounce = 0, assFound = 0, assNotFound = 0, assInserted = 0, assRemoved = 0 ;

/* Associator package is for associating pairs of pointers. 
   Assumes that an "in" item is non-zero and non -1.
   Implemented as a hash table of size 2^m.  
   
   Originally grabbed from Steve Om's sather code by Richard Durbin.

   Entirelly rewritten by mieg, using bouncing by relative primes
   and deletion flagging.

   User has access to structure member ->n = # of pairs
*/

#define VSIZE (sizeof(void*))
#define VS5 (VSIZE/5)
#define VS7 (VSIZE/7)
#define moins_un ((void*) (-1))

#define HASH(_xin) { register long z = VS5, x = (char*)(_xin) - (char*)0 ; \
		  for (hash = x, x >>= 5 ; z-- ; x >>= 5) hash ^= x ; \
		  hash &= a->mask ; \
		}

#define DELTA(_xin)   { register long z = VS7, x = (char*)(_xin) - (char*)0 ; \
		       for (delta = x, x >>= 7 ; z-- ; x >>= 7) delta ^= x ; \
		       delta = (delta & a->mask) | 0x01 ; \
		   }  /* delta odd is prime relative to  2^m */

/**************** Destruction ****************************/

static void assFinalise(void *cp)
{ Associator a = (Associator)cp;

  a->magic = 0 ;
  if (!finalCleanup)
    { messfree(a->in) ;
      messfree(a->out);
    }
}

void uAssDestroy (Associator a)
{ if (assExists(a))
    messfree (a) ;  /* calls assFinalise */
}

/**************** Creation *******************************/

static Associator assDoCreate_dbg (int nbits, STORE_HANDLE handle,
					const char *hfname, int hlineno)
{ static int nAss = 0 ;
  Associator a ;
  int size = 1 << nbits,  /* size must be a power of 2 */
      vsize = size * VSIZE ; 
  a = (Associator) handleAlloc_dbg(assFinalise, 
				   handle, 
				   sizeof (struct AssStruct),
				   hfname, hlineno) ;
  a->in = (void**) halloc_dbg(vsize, 0, hfname, hlineno) ;
  a->out = (void**) halloc_dbg(vsize, 0, hfname, hlineno) ;
  a->magic = ASS_MAGIC ;
  a->id = ++nAss ;
  a->n = 0 ;
  a->i = 0 ; /* start up position for recursive calls */
  a->m = nbits ;
  a->mask = size - 1 ;
  return a ;
}

/*******************/

Associator assBigCreate_dbg(int size, const char *hfname, int hlineno)
{
  int n = 2 ; /* make room, be twice as big as needed */

  if (size <= 0) 
    uMessSetErrorOrigin(hfname, hlineno),
      uMessCrash ("assBigCreate called with size = %d <= 0", size) ;

  --size ;
  while (size >>= 1) n++ ; /* number of left most bit + 1 */
  return assDoCreate_dbg(n, 0, hfname, hlineno) ;
}

/*******************/

Associator assHandleCreate_dbg(STORE_HANDLE handle,
			       const char *hfname,int hlineno)
{ return assDoCreate_dbg(5, handle, hfname, hlineno) ; }

/*******************/

void assClear (Associator a)
{ mysize_t sz ;
  if (!assExists(a)) 
    return ;
  
  a->n = 0 ;
  sz = ( 1 << a->m) *  VSIZE ;
  memset(a->in, 0, sz) ;
  memset(a->out, 0, sz) ;
}

/********************/

Associator assReCreate_dbg (Associator a,
			    const char *hfname, int hlineno)
{ if (!assExists(a))
    return assHandleCreate_dbg(0, hfname, hlineno) ;
  else
    { assClear (a) ;  return a ; }
}

/********************/

static void assDouble (Associator a)
{ int oldsize, newsize, nbits ;
  register int hash, delta ;
  void **old_in = a->in, **old_out = a->out, **xin, **xout ;
  int i ;

  nbits = a->m ;
  oldsize = 1 << nbits ;
  newsize = oldsize << 1 ; /* size must be a power of 2 */

  a->n = 0 ;
  a->i = 0 ; /* start up position for recursive calls */
  a->m = nbits + 1 ;
  a->mask = newsize - 1 ;
  a->in  = (void**) messalloc (newsize * VSIZE) ;
  a->out = (void**) messalloc (newsize * VSIZE) ;

  for (i = 0, xin = old_in, xout = old_out ; i < oldsize ; i++, xin++, xout++)
    if (*xin && *xin != moins_un)
      { HASH(*xin) ;
        while (TRUE)
          { if (!a->in[hash])  /* don't need to test moins_un */
              { a->in[hash] = *xin ;
                a->out[hash] = *xout ;
                ++a->n ;
                assInserted++ ;
                break ;
              }
            assBounce++ ;
	    DELTA(*xin) ;	/* redo each time - cheap overall */
            hash = (hash + delta) & a->mask ;
          }
      }

  messfree (old_in) ;
  messfree (old_out) ;
}

/************************ Searches  ************************************/

BOOL uAssFind (Associator a, void* xin, void** pout)
/* if found, updates *pout and returns TRUE, else returns FALSE	*/
{ int hash, delta = 0 ;
  void* test ;

  if (!assExists(a))
    messcrash ("assFind received corrupted associator") ;
  if (!xin || xin == moins_un) return FALSE ;

  HASH(xin) ;
  while (TRUE)
    { test = a->in[hash] ;
      if (test == xin)
	{ if (pout)
	    *pout = a->out[hash] ;
	  assFound++ ;
	  a->i = hash ;
	  return TRUE ;
	}
      if (!test)
	break ;
      assBounce++ ;
      if (!delta) DELTA(xin) ;
      hash = (hash + delta) & a->mask ;
    }
  assNotFound++ ;
  return FALSE ;
}

/********************/
  /* Usage: if(uAssFind()){while (uAssFindNext()) ... } */
BOOL uAssFindNext (Associator a, void* xin, void** pout)
/* if found, updates *pout and returns TRUE, else returns FALSE	*/
{ int	hash, delta ;
  void* test ;

  if (!assExists(a))
    messcrash ("assFindNext received corrupted associator") ;
  if (!xin || xin == moins_un || !a->in[a->i]) 
    return FALSE ;
  if (a->in[a->i] != xin)
    messcrash ("Non consecutive call to assFindNext") ;

  hash = a->i ;
  DELTA(xin) ;
  while (TRUE)
    { test = a->in[hash] ;
      if (test == xin)
	{ if (pout)
	    *pout = a->out[hash] ;
	  while (TRUE) /* locate on next entry */
	    { hash = (hash + delta) & a->mask ; 
	      test = a->in[hash] ;
	      if (!test || test == xin)
		break ;
	      assBounce++ ;
	    }
	  a->i = hash ; /* points to next entry or zero */
	  assFound++ ;
	  return TRUE ;
	}
      if (!test)
	break ;
      assBounce++ ;
      hash = (hash + delta) & a->mask ;
    }
  assNotFound++ ;
  return FALSE ;
}

/************************ Insertions  ************************************/

     /* if already there returns FALSE, else inserts and returns TRUE */
static BOOL assDoInsert (Associator a, void* xin, void* xout, BOOL noMultiples)
{ int	hash, delta = 0 ;
  void* test ;

  if (!assExists(a))
    messcrash ("assInsert received corrupted associator") ;

  if (!xin || xin == moins_un) 
    messcrash ("assInsert received forbidden value xin == 0") ;

  if (a->n >= (1 << (a->m - 1))) /* reaching floating line */
    assDouble (a) ;

  HASH (xin) ;
 
  while (TRUE)
    { test = a->in[hash] ;
      if (!test || test == moins_un)  /* reuse deleted slots */
	{ a->in[hash] = xin ;
	  a->out[hash] = xout ;
	  ++a->n ;
	  assInserted++ ;
	  return TRUE ;
	}
      if (noMultiples && test == xin)		/* already there */
	return FALSE ;
      assBounce++ ;
      if (!delta) DELTA (xin) ;
      hash = (hash + delta) & a->mask ;
    }
}

/*****************/
     /* This one does not allow multiple entries in one key. */
BOOL assInsert (Associator a, void* xin, void* xout)
{ return assDoInsert (a, xin, xout, TRUE) ;
}
 
/*****************/

void assMultipleInsert (Associator a, void* xin, void* xout)
     /* This one allows multiple entries in one key. */
{ assDoInsert (a, xin, xout, FALSE) ;
}
 
/************************ Removals ************************************/
   /* if found, removes entry and returns TRUE, else returns FALSE	*/
   /* No a->n--, because the entry is blanked but not actually removed */
BOOL assRemove (Associator a, void* xin)
{ if (assExists(a) && uAssFind (a, xin, 0))
    { a->in[a->i] = moins_un ;
      assRemoved++ ;
      return TRUE ;
    }
  else
    return FALSE ;
}

/*******************/

/* if found, removes entry and returns TRUE, else returns FALSE	*/
/* Requires both xin and xout to match */
BOOL assPairRemove (Associator a, void* xin, void* xout)
{ if (!assExists(a) || !xin || xin == moins_un) return FALSE ;
  if (uAssFind (a, xin, 0))
    while (uAssFindNext (a, xin, 0))
      if (a->out[a->i] == xout)
	{ a->in[a->i] = moins_un ;
	  assRemoved++ ;
	  return TRUE ;
	}
  return FALSE ;
}

/************************ dumpers ********************************/
     /* lets you step through all members of the table */
BOOL uAssNext (Associator a, void* *pin, void* *pout)
{ int size ;
  void *test ;

  if (!assExists(a))
     messcrash("uAssNext received a non existing associator") ;
  size = 1 << a->m ;
  if (!*pin)
    a->i = -1 ;
  else if (*pin != a->in[a->i])
    { messerror ("Non-consecutive call to assNext()") ;
      return FALSE ;
    }

  while (++a->i < size)
    { test = a->in[a->i] ;
      if (test && test != moins_un) /* not empty or deleted */
	{ *pin = a->in[a->i] ;
	  if (pout)
	    *pout = a->out[a->i] ;
	  return TRUE ;
	}
    }
  return FALSE ;
}

/*******************/

void assDump (Associator a)
{ int i ; 
  void **in, **out ;
  char *cp0 = 0 ;

  if (!assExists(a)) return ;

  i = 1 << a->m ;
  in = a->in - 1 ; out = a->out - 1 ;
      /* keep stderr here since it is for debugging */
  fprintf (stderr,"Associator %lx : %d pairs\n",(unsigned long)a,a->n) ;
  while (in++, out++, i--)
    if (*in && *in != moins_un) /* not empty or deleted */
      fprintf (stderr,"%lx - %lx\n",
	       (long)((char*)(*in) - cp0),(long)( (char *)(*out) - cp0)) ;
}

/************************  end of file ********************************/
/**********************************************************************/
 
 
 
 
 
