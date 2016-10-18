/*  File: alignment.c
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
 * Description:
 * Exported functions:
 *              (public)  BOOL alignDumpKey (ACEOUT dump_out, KEY key)
 *              (public)  int alignDumpKeySet (ACEOUT dump_out, KEYSET kSet)
 *              
 *              (private) ALIGNMENT *alignGet (KEY key)
 * HISTORY:
 * Last edited: Jul  3 14:26 2003 (edgrif)
 * Created: Thu Sep  7 21:41:15 1995 (rd)
 *-------------------------------------------------------------------
 */

/* $Id: alignment.c,v 1.10 2003-07-04 10:48:26 edgrif Exp $ */

#include "acedb.h"
#include "aceio.h"
#include "lex.h"
#include "bs.h"
#include "dna.h"
#include "peptide.h"

#include "alignment_.h"

#include "whooks/systags.h"

/*********************************************/

static KEY _Alignment, _Verbatim, _Extract, _DNA, _Gap, _Insert ;
static KEY _Peptide, _Seg, _Translate, _Protein ;

static void initialise (void)
{ 
  static BOOL isDone = FALSE ;

  if (isDone) 
    return ;
  
  lexaddkey ("Alignment", &_Alignment, 0) ;
  lexaddkey ("Verbatim", &_Verbatim, 0) ;
  lexaddkey ("Extract", &_Extract, 0) ;
  lexaddkey ("DNA", &_DNA, 0) ;
  lexaddkey ("Gap", &_Gap, 0) ;
  lexaddkey ("Insert", &_Insert, 0) ;
  lexaddkey ("Peptide", &_Peptide, 0) ;
  lexaddkey ("Seg", &_Seg, 0) ;
  lexaddkey ("Translate", &_Translate, 0) ;
  lexaddkey ("Protein", &_Protein, 0) ;

  isDone = TRUE ;
}

/************************************************/

ALIGNMENT *alignGet (KEY key)
     /* private to alignment package */
{ 
  OBJ obj = 0 ;
  ALIGNMENT *al = 0 ;
  STORE_HANDLE handle ;
  KEY tag, item ;
  int start, end ;
  BOOL isReverse ;
  BSMARK mark1 = 0, mark2 = 0, mark3 = 0 ;
  Array translationTable = NULL ;

  initialise () ;

  if (!(obj = bsCreate (key)))
    goto abort ;
  if (!bsFindTag (obj, _Alignment))
    goto abort ;

  if (!(translationTable = pepGetTranslationTable (key, 0)))
    goto abort ;

  handle = handleCreate () ;
  al = (ALIGNMENT*) halloc (sizeof(ALIGNMENT), handle) ;
  al->key = key ;
  al->handle = handle ;
  al->comp = arrayHandleCreate (64, ALIGN_COMP, handle) ;

  if (bsGetKeyTags (obj, _bsRight, 0))
    do
      {
	mark1 = bsMark(obj, mark1) ;
	if (bsGetKey (obj, _bsRight, &item))
	  do
	    {
	      mark2 = bsMark(obj, mark2) ;

	      if (bsGetData (obj, _bsRight, _Int, &start))
		do
		  {
		    mark3 = bsMark(obj, mark3) ;

		    if (bsGetData (obj, _bsRight, _Int, &end) && bsPushObj (obj))
		      {
			/* MAKE A NEW COMPONENT */

			ALIGN_COMP *c = arrayp (al->comp, arrayMax(al->comp), ALIGN_COMP) ;
			char *cp ;

			c->key = item ;
			c->start = start ;
			c->end = end ;

			isReverse = (end < start) ;
			if (isReverse && tag == _Peptide)
			  {
			    messerror ("Can not reverse peptide %s from %d to %d in alignment %s",
				       name(item), start, end, name(key)) ;
			    goto abort ;
			  }

			if (bsGetData (obj, _Verbatim, _Text, &cp))
			  {
			    /* SIMPLE CASE - VERBATIM */

			    c->seq = strnew (cp, al->handle) ;
			  }
			else if (bsGetKeyTags (obj, _Extract, &tag))
			  {
			    Array a ;		/* to build in */
			    ALIGNMENT *ali ;
			    KEY action ;

			    if (((tag == _DNA || tag == _Translate) && (a = dnaGet (item))) ||
				(tag == _Peptide && (a = peptideGet (item))))
			      {
				/* EXTRACT FROM DNA/PROTEIN */
				int i ;
				Array b = arrayCreate (2*(end-start+1), char) ;
	  
				while (bsPushObj (obj) && bsGetKeyTags (obj, _bsRight, &action))
				  if (action == _Gap && bsGetData (obj, _bsRight, _Int, &start))
				    while (start-- > 0)
				      array(b,arrayMax(b),char) = GAP ;
				  else if (action == _Insert && bsGetData (obj, _bsRight, _Int, &start))
				    while (start-- > 0)
				      array(b,arrayMax(b),char) = (tag == _Peptide) ? 20 : N_ ;
				  else if (action == _Seg &&
					   bsGetData (obj, _bsRight, _Int, &start) &&
					   bsGetData (obj, _bsRight, _Int, &end))
				    {
				      if (isReverse)
					for (i = --start ; i >= end-1 ; --i)
					  array(b,arrayMax(b),char) = complementBase[(int)arr(a,i,char)] ;
				      else
					for (i = --start ; i < end ; ++i)
					  array(b,arrayMax(b),char) = arr(a,i,char) ;
				    }
				  else
				    {
				      messerror ("Unknown or incomplete Extract action for "
						 "%s in alignment %s", name(item), name(key)) ;
				      goto abort ;
				    }
				if (tag == _Translate)
				  {
				    cp = c->seq = halloc (1 + arrayMax(b)/3, al->handle) ;
				    for (i = 0 ; i < arrayMax(b)-2 ; i += 3)
				      {
					if (arr(b,i,char) == GAP)
					  *cp++ = '.' ;
					else
					  *cp++ = e_codon (arrp (b, i, char), translationTable) ;
				      }
				    *cp = 0 ;
				  }
				else
				  {
				    if (tag == _Peptide)
				      pepDecodeArray (b) ;
				    else if (tag == _DNA)
				      dnaDecodeArray (b) ;
				    else
				      messcrash ("Oops in alignGet() - unknown tag") ;
				    array(b, arrayMax(b), char) = 0 ;
				    c->seq = strnew (arrp(b,0,char), al->handle) ;
				  }
				arrayDestroy (a) ;
				arrayDestroy (b) ;
			      }
			    else if (tag == _Alignment && (ali = alignGet (item)))
			      {
				/* EXTRACT FROM ALIGNMENT */
				int i, j, n = 0 ;
				int start0 = start, end0 = end ;
				int j0 = arrayMax(al->comp) - 1 ;
				ALIGN_COMP *ci ;

				BSMARK mark = bsMark (obj, 0) ;
				while (bsPushObj (obj) && bsGetKeyTags (obj, _bsRight, &action))
				  if (action == _Gap && 
				      bsGetData (obj, _bsRight, _Int, &start))
				    n += start ;
				  else if (action == _Insert &&
					   bsGetData (obj, _bsRight, _Int, &start))
				    n += start ;
				  else if (action == _Seg &&
					   bsGetData (obj, _bsRight, _Int, &start) &&
					   bsGetData (obj, _bsRight, _Int, &end))
				    n += end - start + 1 ;
				  else
				    { messerror ("Unknown or incomplete Extract action for "
						 "%s in alignment %s", name(item), name(key)) ;
				    goto abort ;
				    }
				bsGoto (obj, mark) ; bsMarkFree (mark) ;

				for (j = 0 ; j < arrayMax(ali->comp) ; ++j)
				  {
				    c = arrayp (al->comp, j0+j, ALIGN_COMP) ;
				    ci = arrp(ali->comp, j, ALIGN_COMP) ;
				    c->key = ci->key ;
				    c->seq = (char*) halloc (n+1, al->handle) ;
				    c->start = start0 ;
				    c->end = end0 ;
				    /* how to do start/end? - need pos */
				  }

				n = 0 ;
				while (bsPushObj (obj) && bsGetKeyTags (obj, _bsRight, &action))
				  if (action == _Gap && 
				      bsGetData (obj, _bsRight, _Int, &start))
				    {
				      for (j = 0 ; j < arrayMax(ali->comp) ; ++j)
					{
					  c = arrp(al->comp, j0+j, ALIGN_COMP) ;
					  for (i = 0 ; i < start ; ++i)
					    c->seq[n+i] = '.' ;
					}
				      n += start ;
				    }
				  else if (action == _Insert &&
					   bsGetData (obj, _bsRight, _Int, &start))
				    {
				      for (j = 0 ; j < arrayMax(ali->comp) ; ++j)
					{
					  c = arrp(al->comp, j0+j, ALIGN_COMP) ;
					  for (i = 0 ; i < start ; ++i)
					    c->seq[n+i] = 'X' ;
					}
				      n += start ;
				    }
				  else if (action == _Seg &&
					   bsGetData (obj, _bsRight, _Int, &start) &&
					   bsGetData (obj, _bsRight, _Int, &end))
				    {
				      --start ;
				      for (j = 0 ; j < arrayMax(ali->comp) ; ++j)
					{
					  c = arrp(al->comp, j0+j, ALIGN_COMP) ;
					  ci = arrp(ali->comp, j, ALIGN_COMP) ;
					  for (i = start ; i < end ; ++i)
					    c->seq[n+i-start] = ci->seq[i] ;
					}
				      n += end - start ;
				    }
				messfree (ali) ;
			      }
			  }
			
		      }
		    bsGoto (obj, mark3) ;
		  } while (bsGetData (obj, _bsDown, _Int, &start)) ;
	      
	      bsGoto (obj, mark2) ;
	    } while (bsGetKey (obj, _bsDown, &item)) ;
	
	bsGoto (obj, mark1) ;
      } while (bsGetKeyTags (obj, _bsDown, 0)) ;
  
  bsMarkFree (mark1) ; bsMarkFree (mark2) ; bsMarkFree (mark3) ;

  return al ;

  /* Clear up after errors. */
 abort:
  if (obj)
    bsDestroy (obj) ;
  if (al)
    messfree (al) ;

  return 0 ;
}

/***************************************************************/

BOOL alignDumpKey (ACEOUT dump_out, KEY key)
{ 
  ALIGNMENT *al ;
  ALIGN_COMP *c ;
  int i, xmax = 0, ymax = 0, x, y ;

  if (!(al = alignGet (key)))
    return FALSE ;

  aceOutPrint (dump_out, "#NAME %s\n", name(key)) ;

  c = arrp(al->comp, 0, ALIGN_COMP) ;
  for (i = arrayMax(al->comp) ; i-- ; ++c)
    { x = strlen (messprintf ("%d - %d", c->start, c->end)) ;
      if (x > xmax)
	xmax = x ;
      y = strlen (name (c->key)) ;
      if (y > ymax)
	ymax = y ;
    }
  xmax += ymax + 2 ;

  c = arrp(al->comp, 0, ALIGN_COMP) ;
  for (i = arrayMax(al->comp) ; i-- ; ++c)
    {
      long int before, after;
      int num_bytes_written;

      y = strlen (messprintf ("%d - %d", c->start, c->end)) ;

      before = aceOutByte(dump_out);
      aceOutPrint (dump_out, "%-*s  %d - %d", 
		   ymax, name(c->key), c->start, c->end) ;
      after = aceOutByte(dump_out);
      num_bytes_written = (int)(after - before);
      aceOutPrint (dump_out, "%*s  %s\n", xmax-num_bytes_written, "",
		   c->seq ? c->seq : "") ;
    }

  messfree (al) ;

  return TRUE ;
} /* alignDumpKey */

/**************/

int alignDumpKeySet (ACEOUT dump_out, KEYSET kSet)
{
  KEYSET alpha ;
  int i, n = 0 ;

  if (!kSet || !keySetMax(kSet)) 
    return 0 ;

  alpha = keySetAlphaHeap (kSet, keySetMax(kSet)) ;
  for (i = 0 ; i < keySetMax(alpha) ; ++i)
    if (alignDumpKey (dump_out, keySet(alpha, i)))
      ++n ;
  keySetDestroy (alpha) ;

  messout ("I wrote %d alignments", n) ;
  return n ;
}

/********************* end of file *********************/


