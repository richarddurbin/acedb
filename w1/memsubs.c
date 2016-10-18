/*  File: memsubs.c
 *  Author: Richard Durbin (rd@sanger.ac.uk)
 *  Copyright (C) J Thierry-Mieg and R Durbin, 1998
 * -------------------------------------------------------------------
 * This file is part of the ACEDB genome database package, written by
 * 	Richard Durbin (MRC LMB, UK) rd@mrc-lmb.cam.ac.uk, and
 *	Jean Thierry-Mieg (CRBM du CNRS, France) mieg@kaa.cnrs-mop.fr
 *
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
 *
 * -------------------------------------------------------------------
 * This file is part of the ACEDB genome database package, written by
 * 	Richard Durbin (MRC LMB, UK) rd@sanger.ac.uk, and
 *	Jean Thierry-Mieg (CRBM du CNRS, France) mieg@kaa.cnrs-mop.fr
 *
 * Description:
 * Exported functions:
 * HISTORY:
 * Last edited: Nov 28 10:54 2007 (edgrif)
 * * Aug 25 15:58 2000 (edgrif): Add check for users "datasize" limit, sometimes
 *              users forget to increase this.
 * * sept 99, threadsafe, exept the reporter values totMessAllloc, never mind
 * * May 19 15:13 1999 (fw): changed all functions that will require
 *              memory to be allocated to receive filename and lineno
 *              from the pre-processor macro, so it's location
 *              in the user-code can be tracked.
 * Created: Thu Aug 20 16:54:55 1998 (rd)
 *-------------------------------------------------------------------
 */

/* define MALLOC_CHECK here to check mallocs - also in regular.h */

#include "regular.h"

#include <wh/aceio.h>


#if defined(NEXT) || defined(HP) || defined(MACINTOSH) 
extern void* malloc(mysize_t size) ;
#elif !defined(WIN32) && !defined(FreeBSD) && !defined (DARWIN)
#include <malloc.h>   /* normal machines  */
#endif


/* just test new datasize check code on linux and alphas for now....         */
#if defined(ALPHA) || defined(LINUX) || defined(OPTERON) || defined(SOLARIS)
#include <sys/time.h>
#include <sys/resource.h>
#endif



/********** primary type definition **************/

typedef struct _STORE_HANDLE_STRUCT {
  STORE_HANDLE next ;	/* for chaining together on Handles */
  STORE_HANDLE back ;	/* to unchain */
  void (*final)(void*) ;	/* finalisation routine */
  int size ;			/* of user memory to follow */
#ifdef MALLOC_CHECK
  int check1 ;			/* set to known value */
#endif
} STORE_HANDLE_STRUCT ;

/*********************************************************************/
/********** memory allocation - messalloc() and handles  *************/

static int numMessAlloc = 0 ;
static int totMessAlloc = 0 ;


  /* Calculate to size of an STORE_HANDLE_STRUCT rounded to the nearest upward
     multiple of sizeof(double). This avoids alignment problems when
     we put an STORE_HANDLE_STRUCT at the start of a memory block */

#define STORE_OFFSET ((((sizeof(STORE_HANDLE_STRUCT)-1)/MALLOC_ALIGNMENT)+1)\
                             * MALLOC_ALIGNMENT)


  /* macros to convert between a void* and the corresponding STORE_HANDLE */

#define toAllocUnit(x) (STORE_HANDLE) ((char*)(x) - STORE_OFFSET)
#define toMemPtr(unit)((void*)((char*)(unit) + STORE_OFFSET))

#ifdef MALLOC_CHECK

static BOOL handlesInitialised = FALSE;

#ifndef MULTITHREAD
static Array handles = 0 ;
#endif

  /* macro to give the terminal check int for an STORE_HANDLE_STRUCT */
  /* unit->size must be a multiple of sizeof(int) */
#define check2(unit)  *(int*)(((char*)toMemPtr(unit)) + (unit)->size)


static void checkUnit (STORE_HANDLE unit) ;
static int handleOrder (void *a, void  *b)
{ return (*((STORE_HANDLE *)a) == *((STORE_HANDLE *)b)) ? 0 : 
	  (*((STORE_HANDLE *)a) > *((STORE_HANDLE *)b)) ? 1 : -1 ;
}
#endif

#if defined(MALLOC_CHECK)
static STORE_HANDLE_STRUCT handle0 ;
#endif



/************** halloc(): key function - messalloc() calls this ****************/

void *halloc_dbg(int size, STORE_HANDLE handle, const char *hfname, int hlineno) 
{ 
  STORE_HANDLE unit ;
  int offset ;
  int malloc_size ;


  /* Basic check on size which _must_ be greater than zero.                  */
  if (size <= 0)
    {
      uMessSetErrorOrigin(hfname, hlineno),
	uMessCrash(HANDLE_ALLOC_FAILED_STR
		   " caused by serious internal error, acedb requested zero or fewer bytes: %d.",
		   size) ;
    }


  /* Memory is retuned with a small block at the front for handle package, offset in the block
   * gives user their pointer to the memory. */
  offset = STORE_OFFSET ;
  malloc_size = offset + size ;

  
#ifdef MALLOC_CHECK

  if (!handlesInitialised)		/* initialise */
    {
      handlesInitialised = TRUE;

      /* BEWARE, arrayCreate calls handleAlloc, line above must precede
         following line to avoid infinite recursion */
#ifndef MULTITHREAD
      handles = arrayCreate (16, STORE_HANDLE) ;
      array (handles, 0, STORE_HANDLE) = &handle0 ;
#endif
      handle0.next = 0 ;
    }

  /* so check2 alignment is OK */
  while (size % INT_ALIGNMENT)
    size++ ;

  unit = (STORE_HANDLE) malloc(offset + size + sizeof(int)) ;
  if (unit)
    memset (unit, 0, offset + size + sizeof(int)) ;

#else  /* !MALLOC_CHECK */

  unit = (STORE_HANDLE)malloc(malloc_size) ;
  if (unit)
    memset(unit, 0, malloc_size) ;

#endif  /* !MALLOC_CHECK */

  /* Oh, oh, out of memory so messcrash, if possible check users process     */
  /* datasize limit to see if they could increase it.                        */
  if (!unit)
    {
#if defined(RLIMIT_DATA)
      struct rlimit limit ;
      char buffer[512] ;				    /* Don't use dynamic buffer because of */
							    /* potential recursion.*/
#endif
      char *user_limit = "" ;

#if defined(RLIMIT_DATA)				    /* Usually in sys/resource.h */
      /* We look at RLIMIT_DATA to see what size data area the users process */
      /* is allowed, this includes initialised, uninitialised and heap data. */
      /* Depending on this value we give the user advice about what to do.   */
      /* Note that we can't be too clever here because we call code that does*/
      /* lots of mallocs that are not accounted for in our totMessAlloc.     */
      /* NOTE: if any call fails (sprintf etc), we don't output this bit.    */
      if (getrlimit(RLIMIT_DATA, &limit) == 0)
	{
	  if (limit.rlim_cur == RLIM_INFINITY)
	    user_limit = " Your machine seems to have run out of memory "
	      "as your current process datasize limit is set to \"unlimited\"." ;
	  else if (limit.rlim_cur == limit.rlim_max)
	    {
	      if (sprintf(&(buffer[0]),
			  " You may need to ask systems support to "
			  "raise your process \"datasize\" limit which is currently %lu bytes.",
			  limit.rlim_cur) > 0)
		user_limit = &(buffer[0]) ;
	    }
	  else
	    {
	      if (sprintf(&(buffer[0]),
			  " Your process datasize is currently %lu bytes "
			  "but you could raise it to %lu bytes%s using "
			  "the limit/unlimit or ulimit shell commands.",
			  limit.rlim_cur, limit.rlim_max,
			  limit.rlim_max == RLIM_INFINITY ? " (max. possible)" : "") > 0)
		user_limit = &(buffer[0]) ;
	    }
	}
#endif

      uMessSetErrorOrigin(hfname, hlineno),
	uMessCrash (HANDLE_ALLOC_FAILED_STR
		    " when acedb requested %d bytes, acedb has already allocated %d bytes.%s", 
		    size, totMessAlloc, user_limit) ;
    }

#ifdef MALLOC_CHECK
#ifndef MULTITHREAD
  if (!handle)
    handle = &handle0 ;
#endif
#endif /* MALLOC_CHECK */

  if (handle) 
    { unit->next = handle->next ;
      unit->back = handle ;
      if (handle->next) (handle->next)->back = unit ;
      handle->next = unit ;
    }

  unit->size = size ;
#ifdef MALLOC_CHECK
  unit->check1 = 0x12345678 ;
  check2(unit) = 0x12345678 ;
#endif /* MALLOC_CHECK */

  /* not thread safe */
  ++numMessAlloc ;
  totMessAlloc += size ;

  return toMemPtr(unit) ;
}

void blockSetFinalise(void *block, void (*final)(void *))
{
  STORE_HANDLE unit = toAllocUnit(block);

  if (unit->final)
    messcrash("blockSetFinalise() trying to overwrite finalisation func ptr");

  unit->final = final ;

  return ;
}  

/***** handleAlloc() - does halloc() + blockSetFinalise() - archaic *****/

void *handleAlloc_dbg (void (*final)(void*), STORE_HANDLE handle, int size,
		       const char *hfname, int hlineno)
{
  void *result = halloc_dbg(size, handle, hfname, hlineno) ;
  if (final) 
    blockSetFinalise(result, final);

  return result;
}

/****************** useful utility ************/

char *strnew_dbg(char *old, STORE_HANDLE handle, const char *hfname, int hlineno)
{ char *result = 0 ;
  if (old)
    { result = (char *)halloc_dbg(1+strlen(old), handle, hfname, hlineno) ;
      strcpy(result, old);
    }
  return result;
}

/****************** messfree ***************/

void uMessFree (void *cp)
{
  STORE_HANDLE unit = toAllocUnit(cp) ;

#ifdef MALLOC_CHECK
  checkUnit (unit) ;
  unit->check1 = 0x87654321; /* test for double free */
#endif

  if (unit->final)
    (*unit->final)(cp) ;

  if (unit->back) 
    { (unit->back)->next = unit->next;
      if (unit->next) (unit->next)->back = unit->back;
    }
  
  --numMessAlloc ;
  totMessAlloc -= unit->size ;
  free (unit) ;
}




/* Utility for reporting memory usage, useful for debugging areas of code suspected
 * of having memory leaks. */
void messMemStats(char *title, BOOL newline)
{
  ACEOUT stats_out ;
  int numMessAlloc_tmp, totMessAlloc_tmp ;

  numMessAlloc_tmp = numMessAlloc ;
  totMessAlloc_tmp = totMessAlloc ;

  /* Should really write to a unique file.... */
  stats_out = aceOutCreateToFile("/var/tmp/ACEDB_MEM_STATS.txt", "a", 0) ;

  if (title)
    aceOutPrint(stats_out, "%s:", title) ;

  aceOutPrint(stats_out, "\t%d\t%d", numMessAlloc_tmp, totMessAlloc_tmp) ;

  if (newline)
    aceOutPrint(stats_out, "%s", "\n") ;

  aceOutDestroy(stats_out) ;

  return ;
}



/************** create and destroy handles **************/

/* NOTE: handleDestroy is #defined in regular.h to be messfree */
/* The actual work is done by handleFinalise, which is the finalisation */
/* routine attached to all STORE_HANDLEs. This allows multiple levels */
/* of free-ing by allocating new STORE_HANDLES on old ones, using */
/* handleHandleCreate. handleCreate is simply defined as handleHandleCreate(0) */

static void handleFinalise (void *p)
{
  STORE_HANDLE handle = (STORE_HANDLE)p;
  STORE_HANDLE next, unit = handle->next ;

/* do handle finalisation first */  
  if (handle->final)
    (*handle->final)((void *)handle->back);

      while (unit)
    { 
#ifdef MALLOC_CHECK
      checkUnit (unit) ;
      unit->check1 = 0x87654321; /* test for double free */
#endif
      if (unit->final)
	(*unit->final)(toMemPtr(unit)) ;
      next = unit->next ;
      --numMessAlloc ;
      totMessAlloc -= unit->size ;
      free (unit) ;
      unit = next ;
    }

#ifdef MALLOC_CHECK
#ifndef MULTITHREAD
      arrayRemove (handles, &p, handleOrder) ;
#endif
#endif

/* This is a finalisation routine, the actual store is freed in messfree,
   or another invokation of itself. */
}
  
void handleSetFinalise(STORE_HANDLE handle, void (*final)(void *), void *arg)
{ handle->final = final;
  handle->back = (STORE_HANDLE)arg;
}

/* create a handle that is allocated upon another handle */
STORE_HANDLE handleHandleCreate_dbg(STORE_HANDLE handle,
				    const char *hfname, int hlineno)
{ 
  STORE_HANDLE res = (STORE_HANDLE) handleAlloc_dbg(handleFinalise, 
						    handle,
						    sizeof(STORE_HANDLE_STRUCT),
						    hfname, hlineno);
#ifdef MALLOC_CHECK
  /* NB call to handleAlloc above ensures that handles is initialised here */
#ifndef MULTITHREAD
  arrayInsert (handles, &res, handleOrder) ;
#endif
#endif /* MALLOC_CHECK */
  res->next = res->back = 0 ; /* No blocks on this handle yet. */
  res->final = 0 ; /* No handle finalisation */
  return res ;
}

BOOL finalCleanup = FALSE ;

#ifdef MALLOC_CHECK
void handleCleanUp (void) 
{ finalCleanup = TRUE ;
  handleFinalise ((void *)&handle0) ;
}
#endif /* MALLOC_CHECK */


/************** checking functions, require MALLOC_CHECK *****/

#ifdef MALLOC_CHECK
static void checkUnit (STORE_HANDLE unit)
{
  if (unit->check1 == 0x87654321)
    messerror ("Block at %x freed twice - bad things will happen.",
	       toMemPtr(unit));
  else
    if (unit->check1 != 0x12345678)
      messerror ("Malloc error at %x length %d: "
		 "start overwritten with %x",
		 toMemPtr(unit), unit->size, unit->check1) ;
  
  if (check2(unit) != 0x12345678)
    messerror ("Malloc error at %x length %d: "
	       "end overwritten with %x",
	       toMemPtr(unit), unit->size, check2(unit)) ;
}

void messalloccheck (void)
{
#ifndef MULTITHREAD

  int i ;
  STORE_HANDLE unit ;

  if (!handles) return ;

  for (i = 0 ; i < arrayMax(handles) ; ++i) 
    for (unit = arr(handles,i,STORE_HANDLE)->next ; unit ; unit=unit->next)
      checkUnit (unit) ;

#endif
}
#else  /* !MALLOC_CHECK */
void messalloccheck (void) {}
#endif /* !MALLOC_CHECK */

/******************* status monitoring functions ******************/

void handleInfo (STORE_HANDLE handle, int *number, int *size)
{
  STORE_HANDLE unit = handle->next;

  *number = 0;
  *size = 0;

  while (unit)
    { ++*number ;
      *size += unit->size ;
      unit = unit->next ;
    }
}

/* not thread safe, tant pis */

int messAllocStatus (int *mem)
{ 
  *mem = totMessAlloc ;
  return numMessAlloc ;
}

/*************************** end of file ************************/
/****************************************************************/
