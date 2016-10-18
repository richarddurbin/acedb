/*  File: bstools.c
 *  Author: Richard Durbin (rd@sanger.ac.uk)
 *  Copyright (c) J Thierry-Mieg and R Durbin, 1991
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
 * Exported functions: See bstree.h
 * HISTORY:
 * Last edited: Nov 28 09:46 2007 (edgrif)
 * Created: Mon Feb  8 00:11:24 1999 (rd)
 * CVS info:   $Id: bstools.c,v 1.17 2007-11-28 10:59:49 edgrif Exp $
 *-------------------------------------------------------------------
 */

#include "acedb.h"

#include "bs_.h"
#include "dump.h"
/* #define CHRONO */
#include "chrono.h"
#include "parse.h"

#include <ctype.h>

extern int MAXKNOWNCACHE ;   /* sizeof cache2, defined in blocksub*/




/**************************************************************/

/* Manage BS allocation
 * hope they speed things up/prevent fragmentation
 * in any case, they will help monitoring memory usage
 * NB they must be paired.
 * 
 * Something to note is that the BSalloc memory can never be freed during
 * the lifetime of the program as we do not keep hold of the pointers to
 * the original memory blocks.
*/


/* Referenced from: w5/objcache.c */
int BS_Cache_limit = 0 ;


static Stack freeBSstack = NULL ;

static int nBSused = 0, nBSalloc = 0 ;        /* useful for debug */

static BOOL BS_Cache_is_Full = FALSE ;

static BOOL bs_debug_G = FALSE ;


/* self managed calloc */
BS BSalloc(void)
{
  BS p = NULL ;
  static int blocSize = 2048 ;
  int bsshow_size = sizeof(struct BShowStruct) ;
  int n ;
 

  chrono("bsAlloc") ;

  
  if (!freeBSstack)
    { 
      BS_Cache_limit = (MAXKNOWNCACHE << 10) / bsshow_size ;

      freeBSstack = stackCreate (4 * blocSize) ;

      if (bs_debug_G)
	printf("sizeof(struct BShowStruct): %d\tMAXKNOWNCACHE: %d\n", bsshow_size, MAXKNOWNCACHE) ;
    }

  if (stackEmpty(freeBSstack))
    {
      if (stackEmpty(freeBSstack))
	{
	  if (bs_debug_G)
	    printf("BEFORE -\tBS_Cache_limit: %d (%d)\tnBSalloc: %d (%d)\tnBSused: %d (%d)\tblocSize: %d (%d)\n",
		   BS_Cache_limit, BS_Cache_limit * bsshow_size,
		   nBSalloc, nBSalloc * bsshow_size,
		   nBSused, nBSused * bsshow_size,
		   blocSize, blocSize * bsshow_size) ;


	  if (nBSalloc + blocSize < BS_Cache_limit)
	    {
	      n = blocSize ; 

	      /* It's possible to tune this a bit by dividing by a larger number so that smaller
	       * allocations are made earlier but the gains are minimal. */
	      if (blocSize < BS_Cache_limit / 4)
		blocSize *= 2 ;
	    }
	  else
	    {
	      n = BS_Cache_limit / 8 ;
	    }

	  nBSalloc += n ;

	  p = (BS) messalloc(n * bsshow_size) - 1 ;

	  while (p++, n--)
	    push (freeBSstack,p,BS) ;

	  if (bs_debug_G)
	    printf("AFTER -\tBS_Cache_limit: %d (%d)\tnBSalloc: %d (%d)\tnBSused: %d (%d)\tblocSize: %d (%d)\n",
		   BS_Cache_limit, BS_Cache_limit * bsshow_size,
		   nBSalloc, nBSalloc * bsshow_size,
		   nBSused, nBSused * bsshow_size,
		   blocSize, blocSize * bsshow_size) ;
	}
    }

  p = pop(freeBSstack, BS) ;

  memset(p, 0, sizeof (struct BShowStruct)) ;

  if (++nBSused > BS_Cache_limit)
    BS_Cache_is_Full = TRUE ;

  chronoReturn() ;

  return p ;
}

void BSfree (BS bs)
{
  chrono("BSfree") ;

  if (bs->bt)
    BTfree(bs->bt) ;

  push(freeBSstack,bs,BS) ;

  if (--nBSused < BS_Cache_limit)
    BS_Cache_is_Full = FALSE ;

  chronoReturn() ;

  return ;
}

void BSstatus (int *used, int *alloc, int *memp)
{
  *used = nBSused ;
  *alloc = nBSalloc ;
  *memp = nBSalloc * sizeof (struct BShowStruct) ;

  return ;
}

/**************************************************************/
/**************************************************************/

         /* idem for BT */

static Stack freeBTstack = 0 ;
static int nBTused = 0 , nBTalloc = 0 ;    /* useful for debug */

BT BTalloc (void)       /* self managed calloc */
{
  static int blocSize = 2048 ;
  BT p ;
  int i ;

  if (!freeBTstack)
    freeBTstack = stackCreate (4*blocSize) ;
  if (stackEmpty (freeBTstack))
    { p = (BT) messalloc (blocSize * sizeof (struct BTextStruct)) ;
      for (i = blocSize ; i-- ; ++p)
        push (freeBTstack,p,BT) ;
      nBTalloc += blocSize ;
      blocSize *= 2 ;
    }
  p = pop (freeBTstack,BT) ;
  memset (p, 0, sizeof (struct BTextStruct)) ;
  ++nBTused ;
  return p ;
}

void BTfree (BT bt)
{
  if(bt->cp)
    messfree(bt->cp) ;
  push (freeBTstack,bt,BT) ;
  --nBTused ;
}


void BTstatus (int *used, int *alloc, int *memp)
{ *used = nBTused ; *alloc = nBTalloc ; *memp = nBTalloc * sizeof (struct BTextStruct) ;
}

/******************************************************************/
/******************************************************************/

static void   bsTreePrune2(BS bs)
{
  BS bs1 ;

  while (bs)
    { if (bs->right && bs != bs->right) /* happens on purpose in models */      
	bsTreePrune2 (bs->right) ;
      bs1 = bs ; bs = bs->down ;
      BSfree(bs1); 
    }
}

/*******************/

void bsTreePrune (BS bs)
{
  if (!bs) return ;
  
  chrono("bsTreefree") ;

  if (bs->up)          /* unhook */
    { if (bs->up->down == bs)
	bs->up->down = 0 ;
      else if (bs->up->right == bs)
	bs->up->right = 0 ;
      else 
	messcrash("double link error in bsTreePrune") ;
      bs->up = 0 ;
    }

  if (bs) bsTreePrune2(bs);

  chronoReturn() ;

}

/************************************************************************/
/************************************************************************/
       /* BS Copying system */
/*************************************************************************/

static BS bsDoTreeCopy (BS bs, BS bsCopyUp)
{ BS bsCopy = BSalloc(), bsCopyTop = bsCopy ;
/* I BSalloc out of loop to singularise bsCopyTop */

  while (bs)
    { *bsCopy = *bs ;
      bsCopy->up = bsCopyUp ;
                                       /* local work */
      if(bs->bt)
	{ bsCopy->bt = BTalloc() ;
	  *bsCopy->bt = *bs->bt ;
	  if(bs->bt->cp) 
	    bsCopy->bt->cp = strnew (bs->bt->cp, 0) ;
	}
                                      /* right recursion */
      if (bs->right)
	bsCopy->right = bsDoTreeCopy (bs->right, bsCopy) ; 

                                      /* down loop */
      if (!(bs = bs->down))
	break ;

      bsCopyUp = bsCopy ;
      bsCopy->down = BSalloc () ;
      bsCopy = bsCopy->down ;
    }
      
  return bsCopyTop ;
}

/**************************/
    /* copies a branch of a tree */
BS bsTreeCopy(BS bs)
{ return bs ? bsDoTreeCopy (bs, 0) : 0 ;
}

/*********************************/
/* called from objcache status function */
int  bsTreeSize (BS bs)	  /* counts the number of nodes in the branch */
{ int n = 0 ;
 
  if (!bs) return 0 ;

  while (bs) /* down loop, right recursion */
    { if (bs->right) n +=  bsTreeSize (bs->right) ;
      n++ ;
      bs = bs->down ;
    }
  return n ;
}

/*****************************************************/

void bsTreeDump (BS bs)
{
  static int depth = 0 ;
  int i ;

  while (bs)
    { 
      for (i = 2*depth ; i-- ;)
	fputc (' ', stderr) ;
      /* keep stderr here since it is for debugging */
      fprintf (stderr,"%8lx : k.%d(%s)   up.%lx down.%lx right.%lx \n", 
	       (unsigned long) bs,
                               bs->key,
	                       name(bs->key), 
	       (unsigned long) bs->up, 
	       (unsigned long) bs->down, 
	       (unsigned long) bs->right) ;
      
      if (bs->right)
	{ ++depth ;
	bsTreeDump (bs->right) ;
	--depth ;
	}
      /* was:  if (bs->down)    bsTreeDump (bs->down) ; */
      bs = bs->down ; /* mieg: replaced by the while(bs) loop */
    }
}

/************************************************************************/
/************************************************************************/
