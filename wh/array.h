/*  File: array.h
 *  Author: Richar Durbin (rd@sanger.ac.uk)
 *  Copyright (C) J Thierry-Mieg and R Durbin, 1998
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
 *	Jean Thierry-Mieg (CRBM du CNRS, France) mieg@crbm.cnrs-mop.fr
 *
 * Description: header for arraysub.c
 *              NOT to be included by the user, included by regular.h
 * Exported functions:
 *              the Array type and associated functions
 *              the Stack type and associated functions
 *              the Associator functions
 * HISTORY:
 * Last edited: August 27 1:30 2000 (lstein)
 * Created: Fri Dec  4 11:01:35 1998 (fw)
 *-------------------------------------------------------------------
 */

#ifndef DEF_ARRAY_H
#define DEF_ARRAY_H
 
unsigned int stackused (void) ;
 
/************* Array package ********/

/* #define ARRAY_CHECK either here or in a single file to
   check the bounds on arr() and arrp() calls
   if defined here can remove from specific C files by defining
   ARRAY_NO_CHECK (because some of our finest code
                   relies on abuse of arr!) YUCK!!!!!!!
*/

/* #define ARRAY_CHECK */

typedef struct ArrayStruct
  { char* base ;    /* char* since need to do pointer arithmetic in bytes */
    int   dim ;     /* length of alloc'ed space */
    int   size ;
    int   max ;     /* largest element accessed via array() */
    int   id ;      /* unique identifier */
    int   magic ;
  } *Array ;
 
    /* NB we need the full definition for arr() for macros to work
       do not use it in user programs - it is private.
    */

#define ARRAY_MAGIC 8918274
#define STACK_MAGIC 8918275
#define   ASS_MAGIC 8918276

Array   uArrayCreate_dbg (int n, int size, STORE_HANDLE handle,
			    const char *hfname,int hlineno) ;
void    arrayExtend_dbg (Array a, int n, const char *hfname,int hlineno) ;
Array	arrayCopy_dbg(Array a, STORE_HANDLE handle, const char *hfname,int hlineno) ; 
Array   uArrayReCreate_dbg (Array a, int n, int size, const char *hfname,int hlineno) ;
void    uArrayDestroy_dbg (Array a,
			   const char *hfname, int hlineno);
char    *uArray_dbg (Array a, int index,
		     const char *hfname, int hlineno);
char    *uArrCheck_dbg (Array a, int index,
			const char *hfname, int hlineno) ;
char    *uArrayCheck_dbg (Array a, int index,
			  const char *hfname, int hlineno) ;

#define arrayCreate(n,type)	         uArrayCreate_dbg(n,sizeof(type), 0, __FILE__, __LINE__)
#define arrayHandleCreate(n,type,handle) uArrayCreate_dbg(n, sizeof(type), handle, __FILE__, __LINE__)
#define arrayReCreate(a,n,type)	         uArrayReCreate_dbg(a,n,sizeof(type), __FILE__, __LINE__)

#define arrayExtend(a, n) arrayExtend_dbg(a, n, __FILE__, __LINE__)

#define arrayHandleCopy(a, h) arrayCopy_dbg(a, h, __FILE__, __LINE__)
#define arrayCopy(a) arrayCopy_dbg(a, 0, __FILE__, __LINE__)

#define arrayDestroy(x)		((x) ? uArrayDestroy_dbg(x, __FILE__, __LINE__), x=0, TRUE : FALSE)

#if (defined(ARRAY_CHECK) && !defined(ARRAY_NO_CHECK))
#define arrp(ar,i,type)	((type*)uArrCheck_dbg(ar,i, __FILE__, __LINE__))
#define arr(ar,i,type)	(*(type*)uArrCheck_dbg(ar,i, __FILE__, __LINE__))
#define arrayp(ar,i,type)	((type*)uArrayCheck_dbg(ar,i, __FILE__, __LINE__))
#define array(ar,i,type)	(*(type*)uArrayCheck_dbg(ar,i, __FILE__, __LINE__))
#else
#define arr(ar,i,type)	((*(type*)((ar)->base + (i)*(ar)->size)))
#define arrp(ar,i,type)	(((type*)((ar)->base + (i)*(ar)->size)))
#define arrayp(ar,i,type)	((type*)uArray_dbg(ar,i, __FILE__, __LINE__))
#define array(ar,i,type)	(*(type*)uArray_dbg(ar,i, __FILE__, __LINE__))
#endif /* ARRAY_CHECK */

     /* only use arr() when there is no danger of needing expansion */

Array   arrayTruncatedCopy_dbg (Array a, int x1, int x2,
				const char *hfname, int hlineno) ;
#define arrayTruncatedCopy(a,x1,x2) arrayTruncatedCopy_dbg(a,x1,x2,__FILE__, __LINE__)

void    arrayStatus (int *nmadep,int* nusedp, int *memAllocp, int *memUsedp) ;
int     arrayReportMark (void) ; /* returns current array number */
void    arrayReport (int j) ;	/* write stderr about all arrays since j */
#define arrayMax(ar)            ((ar)->max)
#define arrayForceFeed(ar,j) (uArray_dbg(ar,j,__FILE__,__LINE__), (ar)->max = (j))
#define arrayExists(ar)		((ar) && (ar)->magic == ARRAY_MAGIC ? (ar)->id : 0 ) 
            /* JTM's package to hold sorted arrays of ANY TYPE */
typedef int array_order (void*, void*);
             /* call back function prototype for sorting arrays */
BOOL    arrayInsert(Array a, void * s, array_order *order);
BOOL    arrayRemove(Array a, void * s, array_order *order);
void    arraySort(Array a,  array_order *order) ;
void    arraySortPos (Array a, int pos, array_order *order);
void    arrayCompress(Array a) ;
BOOL    arrayFind(Array a, void *s, int *ip, array_order *order);
BOOL    arrayIsEntry_dbg(Array a, int i, void *s,
			 const char *hfname, int hlineno);
#define  arrayIsEntry(a,i,s) arrayIsEntry_dbg(a,i,s,__FILE__, __LINE__)

/************** Stack package **************/
 
typedef struct StackStruct      /* assumes objects <= 16 bytes long */
  { Array a ;
    int magic ;
    char* ptr ;         /* current end pointer */
    char* pos ;         /* potential internal pointer */

    char* safe ;        /* need to extend beyond here */
    BOOL  textOnly; /* If this is set, don't align the stack.
		       This (1) save space (esp on ALPHA) and
		       (2) provides stacks which can be stored and got
		       safely between architectures. Once you've set this,
		       using stackTextOnly() only pushText, popText, etc, 
		       no other types. */   
  } *Stack ;
 
        /* as with ArrayStruct, the user should NEVER access StackStruct
           members directly - only through the subroutines/macros
        */
Stack   stackHandleCreate_dbg (int n, STORE_HANDLE handle,
			       const char *hfname, int hlineno) ;
#define stackHandleCreate(n, handle) stackHandleCreate_dbg(n, handle, __FILE__, __LINE__)
#define stackCreate(n) stackHandleCreate_dbg(n, 0, __FILE__, __LINE__)

Stack   stackReCreate_dbg (Stack s, int n,
			   const char *hfname, int hlineno);
#define stackReCreate(s, n) stackReCreate_dbg(s, n, __FILE__, __LINE__)

Stack   stackCopy_dbg (Stack, STORE_HANDLE handle,
		       const char *hfname, int hlineno);
#define stackCopy(s,handle) stackCopy_dbg(s,handle,__FILE__, __LINE__)

void    stackTextOnly(Stack s);

void    uStackDestroy (Stack s);
#define stackDestroy(x)	 ((x) ? uStackDestroy(x), (x)=0, TRUE : FALSE)

void    stackExtend_dbg (Stack s, int n,
			 const char *hfname, int hlineno) ;
#define stackExtend(s,n) stackExtend_dbg(s,n,__FILE__, __LINE__)

void    stackClear (Stack s) ;
#define stackEmpty(stk)  ((stk)->ptr <= (stk)->a->base)
#define stackExists(stk) ((stk) && (stk)->magic == STACK_MAGIC ? arrayExists((stk)->a) : 0)


/* Stack alignment: we use two strategies: the smallest type we push is
   a short, so if the required alignment is to 2 byte boundaries, we 
   push each type to its size, and alignments are kept.

   Otherwise, we push each type to STACK_ALIGNMENT, this ensures 
   alignment but can waste space. On machines with 32 bits ints and
   pointers, we make satck alignment 4 bytes, and do the consequent unaligned
   access to doubles by steam.

   Characters and strings are aligned separately to STACK_ALIGNMENT.
*/ 
   

#if (STACK_ALIGNMENT<=2)
#define push(stk,x,type) ((stk)->ptr < (stk)->safe ? \
                           ( *(type *)((stk)->ptr) = (x) , (stk)->ptr += sizeof(type)) : \
			    (stackExtend (stk,16), \
			     *(type *)((stk)->ptr) = (x) , (stk)->ptr += sizeof(type)) )
#define pop(stk,type)    (  ((stk)->ptr -= sizeof(type)) >= (stk)->a->base ? \
			    *((type*)((stk)->ptr)) : \
                          (messcrash ("User stack underflow"), *((type*)0)) )
#define rot(stk,type,pos)   (  if ( (stk)->ptr - (pos * sizeof(type) ) >= (stk)->a->base) { \
                                 *(type *)((stk)->ptr) = *(type *) (stk)->ptr - (pos * sizeof(type)); \
                                 memmove((stk)->ptr - (pos * sizeof(type)),(stk)->ptr,sizeof(type)); \
                               } else { \
				  (messcrash ("User stack underflow"), *((type*)0)) \
			       } )
#define stackNext(stk,type) (*((type*)(  (stk)->pos += sizeof(type) )  - 1 )  )

#else

#define push(stk,x,type) ((stk)->ptr < (stk)->safe ? \
                           ( *(type *)((stk)->ptr) = (x) , (stk)->ptr += STACK_ALIGNMENT) : \
			    (stackExtend (stk,16), \
			     *(type *)((stk)->ptr) = (x) , (stk)->ptr += STACK_ALIGNMENT) )
#define pop(stk,type)    (  ((stk)->ptr -= STACK_ALIGNMENT) >= (stk)->a->base ? \
			    *((type*)((stk)->ptr)) : \
                          (messcrash ("User stack underflow"), *((type*)0)) )
#define rot(stk,type,pos)   ( (stk)->ptr - ((pos+1) * STACK_ALIGNMENT ) >= (stk)->a->base  ? \
			      ( memcpy((stk)->ptr,(stk)->ptr - ((pos+1) * STACK_ALIGNMENT),STACK_ALIGNMENT), \
				memmove((stk)->ptr - ((pos+1) * STACK_ALIGNMENT), \
					(stk)->ptr - (pos * STACK_ALIGNMENT) , \
					(pos+1) * STACK_ALIGNMENT) ) :  \
			      ( messcrash ("User stack underflow"), *((type*)0)) )
#define stackNext(stk,type) (*((type*)(  ((stk)->pos += STACK_ALIGNMENT ) - \
                                             STACK_ALIGNMENT ))  )
#endif

#if STACK_DOUBLE_ALIGNMENT > STACK_ALIGNMENT
void uStackDoublePush(Stack stk, double x);
double uStackDoublePop(Stack stk);
double uStackDoubleNext(Stack stk);
#define pushDouble(stk,x) uStackDoublePush(stk, x)
#define popDouble(stk) uStackDoublePop(stk)
#define stackDoubleNext(stk) uStackDoubleNext(stk) 
#else
#define pushDouble(stk,x) push(stk, x, double)
#define popDouble(stk) pop(stk, double)
#define stackDoubleNext(stk) stackNext(stk, double)
#endif  


void    pushText (Stack s, char *text) ;
char*   popText (Stack s) ;	/* returns last text and moves pointer before it */
void    catText (Stack s, char *text) ;  /* like strcat */
void	stackTokeniseTextOn(Stack s, char *text, char *delimiters) ; /* tokeniser */

int     stackMark (Stack s) ;              /* returns a mark of current ptr */
int     stackPos (Stack s) ;              /* returns a mark of current pos, useful with stackNextText */
void    stackCursor (Stack s, int mark) ;  /* sets ->pos to mark */
#define stackAtEnd(stk)     ((stk)->pos >= (stk)->ptr)
char*   stackNextText (Stack s) ;
 
char *stackText_dbg (Stack s, int mark, const char *hfname, int hlineno);
#define stackText(s,mark) stackText_dbg(s,mark,__FILE__, __LINE__)

#define stackTextForceFeed(stk,j) (arrayForceFeed((stk)->a,j) ,\
                 (stk)->ptr = (stk)->pos = (stk)->a->base + (j) ,\
                 (stk)->safe = (stk)->a->base + (stk)->a->dim - 16 )
void    catBinary (Stack s, char* data, int size) ;
 
/********** Line breaking package **********/
 
int     uLinesText (char *text, int width) ;
char    *uNextLine (char *text) ;
char    *uPopLine (char *text) ;
char    **uBrokenLines (char *text, int width) ; /* array of lines */
char    *uBrokenText (char *text, int width) ; /* \n's intercalated */

/********** Associator package *************/

typedef struct AssStruct
  { int magic ;                 /* Ass_MAGIC */
    int id ;                    /* unique identifier */
    int n ;			/* number of items stored */
    int m ;			/* power of 2 = size of arrays - 1 */
    int i ;                     /* Utility state */
    void **in,**out ;
    unsigned int mask ;		/* m-1 */
  } *Associator ; 
 
#define assExists(a) ((a) && (a)->magic == ASS_MAGIC ? (a)->id : 0 )

Associator assHandleCreate_dbg (STORE_HANDLE handle,
				const char *hfname, int hlineno) ;
Associator assBigCreate_dbg (int size,
			     const char *hfname, int hlineno) ;
Associator assReCreate_dbg (Associator a,
			    const char *hfname, int hlineno) ;

#define assCreate()        assHandleCreate_dbg(0, __FILE__, __LINE__)
#define assHandleCreate(h) assHandleCreate_dbg(h, __FILE__, __LINE__)
#define assBigCreate(s)    assBigCreate_dbg(s, __FILE__, __LINE__)
#define assReCreate(a)     assReCreate_dbg(a, __FILE__, __LINE__)

void    uAssDestroy (Associator a) ;
#define assDestroy(x)  ((x) ? uAssDestroy(x), x = 0, TRUE : FALSE)
BOOL    uAssFind (Associator a, void* xin, void* *pout) ;
BOOL    uAssFindNext(Associator a, void* xin, void * *pout);
#define assFind(ax,xin,pout)    uAssFind((ax),(xin),(void**)(pout))
            /* if found, updates *pout and returns TRUE, else returns FALSE */
#define assFindNext(ax,xin,pout) uAssFindNext((ax),(xin),(void**)(pout))
BOOL    assInsert (Associator a, void* xin, void* xout) ;
            /* if already there returns FALSE, else inserts and returns TRUE */
void    assMultipleInsert(Associator a, void* xin, void* xout);
           /* allow multiple Insertions */
BOOL    assRemove (Associator a, void* xin) ;
            /* if found, removes entry and returns TRUE, else returns FALSE */
BOOL    assPairRemove (Associator a, void* xin, void* xout) ;
            /* remove only if both fit */
void    assDump (Associator a) ;
           /* for debug - uses fprintf to stderr */
void    assClear (Associator a) ;
BOOL    uAssNext (Associator a, void* *pin, void* *pout) ;
#define assNext(ax,pin,pout)	uAssNext((ax),(void**)(pin),(void**)pout)
/* convert an integer to a void * without generating a compiler warning */
#define assVoid(i) ((void *)(((char *)0) + (i))) 
#define assInt(v) ((int)(((char *)v) - ((char *)0)))

#endif /* !DEF_ARRAY_H */

/**************************** End of File ******************************/

 
