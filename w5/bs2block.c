/*  File: bs2block.c
 *  Author: Richard Durbin (rd@sanger.ac.uk)
 *  Copyright (C) J Thierry-Mieg and R Durbin, 1995
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
 * SCCS: $Id: bs2block.c,v 1.15 1999-11-19 18:58:22 mieg Exp $ 
 * Description:
 * Exported functions:
 * HISTORY:
    mieg: jan 97 
          change double recursion to down loop +rigth recursion and
          tested the code
 * Last edited: Feb  8 14:29 1999 (edgrif)
 * * Feb  8 14:29 1999 (edgrif): Add #ifdef ACEDB5
 * Created: Wed Apr 12 18:28:47 1995 (rd)
 *-------------------------------------------------------------------
 */

#ifndef ACEDB5
void bs2blockDummy (void) 
{ return ; }
#else

#include "acedb.h"
#include "byteswap.h"
#include "block.h"
#include "bs_.h"
#include <whooks/systags.h>
#include "adisk.h"
#include "bstree.h"
#include "lex.h"

/*
   Strategy: 
     Store a B tree as two streams, one of node keys and data, and the 
        other of chars holding Text and the tree structure (one char per node).
     Hold both these in a mixed type array:
        the key stream runs up from 0 and the char stream runs down from max.
     When storing, first find the sizes of these arrays, so no array() tests
        needed when converting.
     These are key inner loop functions, for which efficiency is important.
        Use tight code and package statics to minimise recursion stack depth.
*/

/**********************************************/

static BSdata *bp ;		/* pointer in key/data stream */
static char   *cp ;		/* pointer in character stream */

static BS nodeUnPack (KEY currStamp)
{ 
  BS bs0 = BSalloc() ;
  BS bs = bs0 ;
  char c ;

  while (TRUE)			/* loop down */
    {
      bs->key = maybeSwapKEY((bp++)->key) ;
      if (bs->key <= _LastC)	/* Text */
	{ char *s ; int n ;
	  bs->bt = BTalloc() ;
	  for (s = cp, n = 0 ; *s-- ; n++ ) ;
	  s = bs->bt->cp = messalloc (n + 1) ; 
	  while ((*s++ = *cp--)) ;
	}
      else if (bs->key <= _LastN) /* number */
	bs->n.key = maybeSwapKEY((bp++)->key) ;
      
      c = *cp-- ;
      if (c & 0x02)
	currStamp = maybeSwapKEY((bp++)->key) ;
      bs->timeStamp = currStamp ;
      if (c & 0x10)
	{ bs->right = nodeUnPack (currStamp) ; /* recurse right */
	  bs->right->up = bs ;
	}
      if (c & 0x01)		/* NB -- on *cp here to move on pointer */
	{ bs->down = BSalloc() ;
	  bs->down->up = bs ;
	  bs = bs->down ;
	  continue ;
	}
      else
	break ;			/* exit point of loop */
    }

  return bs0 ;
}

/******* nodePack(): pack node bs and everything right and down *******/

static void nodePack (BS bs, KEY currStamp)
{
  while (bs)			/* loop down */
    {
      (bp++)->key =  maybeSwapKEY(bs->key) ;

      if (bs->key <= _LastC)	/* text */
	{ if (bs->bt && bs->bt->cp)
	    { char *s = bs->bt->cp ;
	      while ((*cp-- = *s++)) ;
	    }
	  else
	    *cp-- = 0 ;
	}
      else if (bs->key <= _LastN) /* number */
	(bp++)->key = maybeSwapKEY(bs->n.key) ;	/* swap as key is OK - same size */

      *cp = 0 ;
      if (bs->timeStamp != currStamp)
	{ *cp |= 0x02 ;
	  (bp++)->key = maybeSwapKEY(bs->timeStamp) ;
	  currStamp = bs->timeStamp ;
	}
      if (bs->right)
	*cp |= 0x10 ;
      if (bs->down)
	*cp |= 0x01 ;
      cp-- ;

      if (bs->right)
	nodePack (bs->right, currStamp) ;	/* recurse right */

      bs = bs->down ;
    }
}

/******** nodeSize():  precount space requirements *********/

static int nc ;			/* count for chars  */
static int nb ;			/* count for keys/numbers */

static void nodeSize (BS bs, KEY currStamp)
{ 
  while (bs)			/* loop down */
    { 
      nb++ ;			/* for the key */
      nc++ ;			/* for the tree structure encoding char */

      if (bs->key <= _LastC)
	{ bs->size = bs->bt && bs->bt->cp ? strlen(bs->bt->cp) + 1 : 1 ;
	  nc += bs->size ;	/* for the text */
	}
      else if (bs->key <= _LastN)
	nb++ ;			/* for the number */
      if (bs->timeStamp != currStamp)
	{ nb++ ;		/* for the timeStamp */
	  currStamp = bs->timeStamp ;
	}

      if (bs->right)
	nodeSize (bs->right, currStamp) ; /* recurse right */

      bs = bs->down ; 
    }
}

/****************************************************************************/
/************ external functions ********************************************/

#define BSDATASIZE sizeof(BSdata)

int bsTreeArraySize (BS bs, KEY timeStamp)
{
  nb = nc = 0 ;			/* will hold return values of nodeSize() */
  nodeSize (bs, timeStamp) ;	/* get BSdata and char array sizes */
  return (nb + (nc/BSDATASIZE + 1)) ;
}

/****************************************************************************/
/***** primary package to Store/Get full BS objects to/from disk ************/

#include "a.h"
#include "pick.h"

/****** bsTreeArrayGet(): get B object from disk ****************************/

BS bsTreeArrayGet (KEY key) 
{ 
  BS bs ;
  Array a = 0 ;		/* of BSdata */

  key = lexAliasOf(key) ;
  if (!key || (pickType(key) != 'B'))
    messcrash ("bad call to bsTreeArrayGet : key 0x%x = %s:%s", 
	       key, className(key), name(key)) ;
  
  if (!aDiskArrayGet (key, 0, &a, 0) || !a || !arrayMax(a)) 
    { bs = BSalloc() ;
      bs->key = key ;
    }
  else
    { bp = arrp (a, 0, BSdata) ;
      cp = (char*)(arrp (a, arrayMax(a)-1, BSdata)) + BSDATASIZE - 1 ;
      bs = nodeUnPack (0) ;
    }

  if (a) arrayDestroy (a) ;
  return bs ;
}

/*** bsTreeArrayStore(): save B object to disk *******************************/
/*** option doWait puts into waitingSet, to be flushed by bsTreeArrayFlush() */

static Array waitingSet = 0 ;
typedef struct { KEY key ; Array a ; DISK d ; } WaitInfo ;

void bsTreeArrayStore (BS bs, BOOL doWait)
{ 
  int size ;
  KEY key = bs->key ;
  Array a ;			/* of BSdata */

  key = lexAliasOf(key) ;
  if (!key || (pickType(key) != 'B') || !bs)
    messcrash ("bad call to bsTreeArrayGet: key 0x%x%s:%s", 
	       className(key), name(key)) ;

  size = bsTreeArraySize (bs, 0) ;
  a = arrayCreate (size, BSdata) ; 
  array (a, size-1, BSdata).key = 0 ; /* to set arrayMax() to nt */

  bp = arrp (a, 0, BSdata) ;
  cp = (char*)(arrp (a, size-1, BSdata)) + BSDATASIZE - 1 ;
  nodePack (bs, 0) ;

  if (doWait)
    { WaitInfo *ww ;

      if (!waitingSet) 
	waitingSet = arrayCreate (128, WaitInfo) ;

      ww = arrayp(waitingSet, arrayMax(waitingSet), WaitInfo) ;
      ww->key = key ; 
      ww->a = a ; 
      ww->d = aDiskAssign (key, arrayMax(a), BSDATASIZE, 0, 0) ;  
    }
  else
    { aDiskArrayStore (key, 0, a, 0) ;
      arrayDestroy (a) ;
    }
}

/***********************************************/

static int waitInfoDiskOrder (void *a, void *b) /* sort function */
{ return ((WaitInfo*)a)->d - ((WaitInfo*)b)->d ; } 

void bsTreeArrayFlush (void)
{
  WaitInfo *ww ; 
  int i ;

  if (waitingSet && arrayMax(waitingSet))
    { arraySort (waitingSet, waitInfoDiskOrder) ;
      for (i = 0 ; i < arrayMax(waitingSet) ; ++i)
	{ ww = arrp(waitingSet, i, WaitInfo) ;
	  aDiskArrayStore (ww->key, 0, ww->a, 0) ;
	  arrayDestroy (ww->a) ;
	}
      arrayMax (waitingSet) = 0 ;
    }
}

/****************************************************************************/
/**** package to save/get a tree structure to/from within a larger array ****/

int bsTreeAddToArray (BS bs, Array a, int pos, int size, KEY timeStamp)
{
  if (a->size != BSDATASIZE)
    messcrash ("a->size %d != BSDATASIZE %d", a->size, BSDATASIZE) ;

  if (!size)
    size = bsTreeArraySize (bs, timeStamp) ;

  array (a, pos, BSdata).key = size ;
  array (a, pos+size, BSdata).key = 0 ; /* to ensure arrayMax() >= size */

  bp = arrp (a, pos+1, BSdata) ;
  cp = (char*)(arrp (a, pos+size, BSdata)) + BSDATASIZE - 1 ;
  nodePack (bs, timeStamp) ;

  return size ;		/* amount of space used */
}

BS bsTreeGetFromArray (Array a, int pos, KEY timeStamp)
{
  int size ;

  if (pos >= arrayMax(a))
    messcrash ("pos %d out of bounds %d", pos, arrayMax(a)) ;
  if (a->size != BSDATASIZE)
    messcrash ("a->size %d != BSDATASIZE %d", a->size, BSDATASIZE) ;

  size = array(a, pos, BSdata).i ;
  if (pos+size >= arrayMax(a))
    messcrash ("pos %d + size %d out of bounds %d", pos, size, arrayMax(a)) ;

  bp = arrp (a, pos+1, BSdata) ;
  cp = (char*)(arrp (a, pos+size, BSdata)) + BSDATASIZE - 1 ;
  return nodeUnPack (timeStamp) ;
}


#endif /* ACEDB5 */
/**************** end of file *****************/
