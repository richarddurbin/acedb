/*  File: bsdumps.c
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
 * Last edited: May  4 14:17 2000 (edgrif)
 * * Mar 11 00:04 1992 (rd): asn dumps
 * Created: Sun Feb  9 21:02:19 1992 (mieg)
 * CVS info:   $Id: bsdumps.c,v 1.47 2000-05-08 13:48:46 edgrif Exp $
 *-------------------------------------------------------------------
 */

#include <wh/acedb.h>
#include <wh/aceio.h>
#include <wh/bs_.h>
#include <wh/bs.h>
#include <wh/dump.h>
#include <wh/lex.h>
#include <wh/pick.h>
#include <whooks/systags.h>
#include <whooks/sysclass.h>
#include <wh/chrono.h>
#include <wh/query.h>

BOOL dumpTimeStamps = FALSE;	/* global, in dump.h */
BOOL dumpComments = TRUE;	/* global, in dump.h */

static int BREAK = 0 ; 
#define DUMP_FLAG	0x0001 /* overload bs->size */
static BOOL dumpQueries = FALSE ; /* ACEDB_DUMP_ATTACH */

/**************************************************************/

void bsDump (OBJ obj) { bsTreeDump (obj->root) ; }

/********** ace dump package for BS trees *************/

/* RMD 31/12/90
   Strategy is to do a depth first search of the tree.
   When a leaf is found: if it is a tag, write it out,
   if not, pop back onto a temporary stack until one is found,
   then run forward again to write it out.
*/

static Stack 
  aceDumpWorkStack = 0,
  aceDumpTempStack = 0, 
  aceDumpText = 0 ;	/* used in recursion */

#define LINE_LENGTH 70
#define addText(xx) catText(aceDumpText,xx)

static KEY dumpCellKey ;

/* RD 970308 - made dumpCellKey static.
   dumpCellKey being static changes behaviour a bit, but for 
   the better - the old version allowed dumping tags via ATTACH
   from non-tag keys.
*/

/* RD 980713 - timeStamps now stored locally to cells - MUCH simpler
   Also, now loop down columns, and only recurse right - important!
*/

static void bsAceDumpCell (BS bs, BS bsm, ACEOUT dump_out, int level, BOOL noDown)
{
  for ( ; bs ; bs = bs->down) {	/* main loop, down the column */

				/* bizarre protection for Session */
  if (class(bs->key) == _VSession && class(dumpCellKey) != _VSession)
    continue ;

  if (bsIsComment(bs) && !dumpComments)
    continue ;

  push (aceDumpWorkStack, bs, BS) ;

  /* Note we do not check return from bsModelMatch so that tags that are in  */
  /* the database but _not_ in the model will still get dumped. This may not */
  /* be what the user wants but at least they won't lose any data.           */
  bsModelMatch (bs, &bsm) ;
  bs->size &= ~DUMP_FLAG ; /* a priori, do not dump */

  if (bs->right)		/* recurse */
    {
      BS bsr1 = bs->right ;

      if (bsm && bsm->right && bsIsType(bsm->right))
	bs->size |= DUMP_FLAG ;
      else if (bsIsTag (bs) && !bsIsTag (bsr1))
	bs->size |= DUMP_FLAG ;

      bsAceDumpCell (bsr1, bsModelRight(bsm), dump_out, level, FALSE) ;
    }
  else				/* dump */
    { 
      BS bst ;
      BOOL first ;

      bs->size |= DUMP_FLAG ;	/* always dump terminal leaves */

      if (dumpQueries &&	/* first deal with attachments */
	  bsm && bsm->right && class(bsm->right->key) == _VQuery)
	{ 
	  char *cq, *question = name(bsm->right->key) ;
	  OBJ obj2 = 0 ; BS bs1 = bs ;
	  KEYSET ks1 ; KEY key1 ;
	  int ii ;
	  BOOL wholeObj = !strcmp("WHOLE",question) ;
	  
	  cq = question + strlen(question) - 1 ;
	  while (cq > question && *cq != ';')
	    cq-- ;
	  if (*cq == ';')
	    cq++ ;
	  wholeObj = !strcmp("WHOLE",cq) ;
	  bs->size |= DUMP_FLAG ;
	  
	  /* position k1 upward on first true non-tag key */
	  while (bs1->up && 
		 ( pickType(bs1->key) != 'B' ||
		  !class(bs1->key) || bsIsComment(bs1)))
	    bs1 = bs1->up ;

	  ks1 = queryKey (bs1->key, !wholeObj ? question : "IS *") ;
	  if (wholeObj || queryFind (0, 0, cq)) 
	    for (ii = 0 ; ii < keySetMax (ks1) ; ii++)
	      { key1 = keySet (ks1, ii) ;
		if (!level)
		  {
		    obj2 = bsCreate(key1) ;
		    if (!obj2)
		      continue ;
		    if (wholeObj || queryFind (obj2, key1, 0)) 
		      bsAceDumpCell (obj2->curr->right, 
				     bsModelRight(obj2->modCurr),
				     dump_out, level + 1, FALSE) ;
		    bsDestroy (obj2) ;
		  }
	      }
	  keySetDestroy (ks1) ;
	}
				/* now get on to the real dumping */
				/* first reverse the stack */
      stackClear (aceDumpTempStack) ;
      while (!stackEmpty (aceDumpWorkStack))
	{ bst = pop (aceDumpWorkStack, BS) ;
	  push (aceDumpTempStack, bst, BS) ;
	}
      stackClear (aceDumpText) ;
      first = TRUE ;

      while (!stackEmpty (aceDumpTempStack))
	{ bst = pop (aceDumpTempStack, BS) ;
	  push (aceDumpWorkStack, bst, BS) ;

	  if (class(bst->key) == _VComment)
            { addText (" -C ");
              addText(freeprotect(name(bst->key))) ; 
            }
          else if (class(bst->key))
            { addText(" ") ; 
              addText(freeprotect(name(bst->key))) ; 
            }
	  else if (bst->key == _Int)
	    addText(messprintf (" %d", bst->n.i)) ;
	  else if (bst->key == _Float)
	    addText (messprintf (" %f", bst->n.f)) ;
	  else if (bst->key == _DateType)
	    { addText(" ");
	      addText (timeShow (bst->n.time)) ;
	    }
	  else if (bst->key <= _LastC)
	    { 
	      if (!first) /* avoid leading "" due to time stamps */
		{ addText(" ") ; 
		  addText(freeprotect(bsText(bst))) ; 
		}
	    }
	  else			/* a tag */
	    if (dumpTimeStamps || (bst->size & DUMP_FLAG))
	      if (first)
		{ addText(name(bst->key)) ; addText("\t") ; 
		  first = FALSE ;
		}
	      else
		{ addText(" ") ; addText(name(bst->key)) ; }

				/* now do timestamp if dumping them */
	  if (dumpTimeStamps && bst->timeStamp)
	    { addText(" -O ") ;
	      addText(freeprotect(name(bst->timeStamp))) ;
	    }
	}

      if (!stackEmpty(aceDumpText))
	{
	  catText (aceDumpText, "\n") ;
#ifdef DO_BREAK
	  if (dumpFile)
	    { if (BREAK)
		{ uLinesText (stackText(aceDumpText,0), BREAK) ;
		  fprintf (dumpFile, "%s", uNextLine(stackText(aceDumpText,0))) ;
		  while ((cp = uNextLine(stackText(aceDumpText,0))))
		    fprintf (dumpFile, "\\\n	%s", cp) ;
		}
	      else
		fputs (stackText(aceDumpText,0), dumpFile) ;
	    }
#endif

	  if (aceOutPrint (dump_out, "%s", stackText(aceDumpText,0)) != ESUCCESS)
	    messerror ("dump error : %s\n", messSysErrorText());
	}
    }

  pop (aceDumpWorkStack,BS) ;

  if (noDown)
    break ;
} /* end of main loop */

  return;
} /* bsAceDumpCell */

void bsAceDump (OBJ obj, ACEOUT dump_out, BOOL isInternal)
{
  BS bs, bsm, bs1 ;
  static BOOL isFirst = TRUE ;

  if (isFirst)
    { char *s ;
      if ((s = getenv ("ACEDB_BREAK")))
	{ BREAK = atoi(s) ;
	  if (!BREAK)
	    BREAK = 80 ;
	}
      if (getenv ("ACEDB_DUMP_ATTACH"))
        dumpQueries = TRUE ;
      isFirst = FALSE ;
    }
  /*  dumpFile = fil ; dumpStack = s ;*/
  aceDumpTempStack = stackReCreate (aceDumpTempStack,64) ;
  aceDumpWorkStack = stackReCreate (aceDumpWorkStack,64) ;
  aceDumpText = stackReCreate(aceDumpText,64) ;

  dumpCellKey = obj->key ;

  if (!isInternal) /* normal case, dump whole object */
    { bs = obj->root->right ;
      bsm = bsModelRight (bsModelRoot (obj)) ;
      bsAceDumpCell (bs, bsm, dump_out, 0, FALSE) ;
    }
  else     /* a query and we are positioned at the node answering it
              we should push back untill we hit a tag then restart from
              the current position and right, but not down

               ithink that is what we want, although i am not too sure of how to
               achieve it
	       */
    {
      bs = obj->curr ;  
      bsm = obj->modCurr ;

      bs1 = bs ;
      while (TRUE) /* move up to nearest tag and dump that path */
	{
	  if (bs != bs1 &&
	      (dumpTimeStamps ||  class(bs1->key) != _VUserSession))
	    { push (aceDumpTempStack, bs1, BS) ; 
	      bs1->size |= DUMP_FLAG ; 
	    }
	  if ((class(bs1->key) == _VSystem && bs1->key > _LastN) || 
	      !bs1->up)
	    break ;
	  bs1 = bs1->up ;
	}

      while (!stackEmpty (aceDumpTempStack))
	{ bs1 = pop (aceDumpTempStack, BS) ;
	push (aceDumpWorkStack, bs1, BS) ;
	}
      stackClear (aceDumpTempStack) ;

      bsAceDumpCell (bs, bsm, dump_out, 0, TRUE) ;
    }

  /* this used to write an extra 2 lines to a stack-output
   * do we want this appended to the ACEOUT too ??
  if (s)
    catText (s, "\n\n") ;
  */

  return;
} /* bsAceDump */

/********************************************************************/
/********************************************************************/
