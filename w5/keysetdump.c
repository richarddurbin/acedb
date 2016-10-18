/*  File: keysetdump.c
 *  Author: Jean Thierry-Mieg (mieg@mrc-lmb.cam.ac.uk)
 *  Copyright (C) J Thierry-Mieg and R Durbin, 1991
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
 * Description: Needed in acequery and xace
 * Exported functions:
 * HISTORY:
 * Last edited: Jul  9 11:53 2003 (rnc)
 * * Aug 19 17:36 1998 (rd): changed condition on dumping keys from
 *	(!pickList[class(*kp)].protected && !lexIsKeyInvisible(*kp))
 *      to (lexIsKeyVisible (*kp)) and prevented lexaddkey() on parsing
 *	protected keys, silently.
 * * Nov 12 14:23 1991 (mieg): added heap system
 * * Oct 23 18:12 1991 (mieg): added keySetParse
 * Created: Mon Oct 21 19:55:34 1991 (mieg)
 *-------------------------------------------------------------------
 */


/* $Id: keysetdump.c,v 1.32 2003-07-22 02:59:05 mieg Exp $ */

#include "acedb.h"
#include "aceio.h"
#include "bs.h"	  /* need bsKeySet */
#include "lex.h"  /* needed in keysetdump and keysetorder */
#include "pick.h" /* needed in keysetdump */
#include "a.h"    /* needed in keySetParse */
#include "whooks/sysclass.h"
#include "java.h"
#include "dump.h"
#include "utils.h"           /* for lexstrcmp */

/************************************************************/
  /* Heap sort
   * source is an array to be partially sorted,
   * nn is the desired number of sorted objects,
   * order is the order function
   * min and max are extrema of the sorting functions
   */

/**************************************************************/
/************ Dedicated heap function *************************/

KEYSET  keySetHeap(KEYSET source, int nn, int (*order)(KEY *, KEY *))
{ KEYSET h, result ;
  int i , k = 1 , max , pos ;
  KEY  new ;
    
  if(!source || !arrayMax(source))
    return keySetCreate() ;
  
  if(!arrayExists(source))
    messcrash("keySetHeap received a non-magic source keySet") ;

  messStatus ("Heaping") ;
    /* Note that KEY 0 is used as maximal element */
  if(nn > arrayMax(source))
    nn = arrayMax(source) ;
  while( ( 1 << ++k) <= nn) ;
  max = 1 << k ;
  h = arrayCreate(max, KEY) ;
  arrayMax(h) = max ; 
  
  i = arrayMax(source) ;
  while (i --)
    { new = keySet(source, i) ;

        /*  HEAP_PUSH(new) */
      { pos = 1 ;
	while( ! arr(h, pos, KEY) ||
	      (order(&new,arrp(h, pos, KEY)) == -1 ) )
	  { if (pos>>1)
	      arr(h, pos>>1, KEY) = arr(h, pos, KEY) ;
	    arr(h, pos, KEY) = new ;
	    if ( (pos <<= 1) >= max )
	      break ;
	    if( ! arr(h, pos + 1, KEY) ||
	       ( order( arrp(h, pos, KEY), arrp(h, pos + 1 ,KEY)) == -1) )
	      pos++ ;
	  }
      }
    }

/* Test print out 
  for(i=0 ; i < arrayMax(h) ; i++)
    printf("%d %s \n", i, name(arr(h,i,KEY))) ;
*/  
              /* To dump,
	       * Create a minimal key
	       * and push it repeatedly
	       * collecting the top key 
	       */
  result = arrayCreate(max, KEY) ;
  arrayMax(result) = max - 1 ;
  
  i = max - 1 ;  /* pos 0 is not filled with any relevant data */
  new = KEYMAKE(0, lexMax(0)) ;
  while(i--)
    { arr(result, i, KEY) = arr(h, 1, KEY) ;
        /*  HEAP_PUSH(minimal key) */
      { pos = 1 ;
	while( TRUE ) /* I pretend to push a minimal key */
	  { if (pos>>1)
	      arr(h, pos>>1, KEY) = arr(h, pos, KEY) ;
	    arr(h, pos, KEY) = new ;
	    if ( (pos <<= 1) >= max )
	      break ;
	    if( arr (h, pos, KEY) == new ||
	       (order( arrp(h, pos, KEY), arrp(h, pos + 1 ,KEY)) == -1))
	      pos++ ;
	  }
      }
    }

  if(arrayMax(source) < arrayMax(result))
    (arrayMax(result) = arrayMax(source)) ;
  
  keySetDestroy(h) ;

  return result ;
}

/*************************************/

KEYSET keySetNeighbours(KEYSET oldSet)
{
  KEYSET new = keySetCreate() , nks ; KEY key, *kp ;
  int  i = keySetMax(oldSet) , j1 = 0, j2;
 
  i = keySetMax(oldSet) ;
  while(i--)
    { nks = (KEYSET) bsKeySet(keySet(oldSet, i)) ;
      if (nks)
	{ j2 = keySetMax(nks) ; kp = arrp(nks,0,KEY) - 1 ;
	  while(kp++, j2--)
	    if (!pickXref(class(*kp)) && 
		iskey(*kp) == 2 &&
		class(*kp) != _VUserSession)
	      keySet(new, j1++) = *kp ;
	  keySetDestroy(nks) ;
	}
      if (messIsInterruptCalled())
	break ;
    }  
  i = keySetMax(oldSet) ;
  while(i--)
    {
      key = keySet(oldSet, i) ;
      if (class(key) == _VKeySet &&
	  (nks = arrayGet (key,KEY,"k")))
	{ 
	  j2 = keySetMax(nks) ; kp = arrp(nks,0,KEY) - 1 ;
	  while(kp++, j2--)
	    if (!pickXref(class(*kp)) && 
		iskey(*kp) == 2 &&
		class(*kp) != _VUserSession)
	      keySet(new, j1++) = *kp ;
	  keySetDestroy(nks) ;
	}
    }
  keySetSort(new) ;
  keySetCompress(new) ;
  keySetDestroy (oldSet) ;
  return new ;
}

/**************************************************************/
/**************************************************************/

       /* first order classes */
     /* then compares KEYs acoording to their Word meaning*/

int keySetAlphaOrder(void *a,void *b)
{
  register int i = class(*(KEY*)a), j = class(*(KEY*)b) ;

  if (i != j)   
    return lexstrcmp (className(*(KEY *)a),
                   className(*(KEY *)b)  ) ;
  else
    return lexstrcmp(name(*(KEY *)a),
                   name(*(KEY *)b)  );
} /* keySetAlphaOrder */

/**************************************************************/

BOOL keySetDump (ACEOUT dump_out, KEYSET s)
{
  int i = keySetMax(s) ;
  KEY *kp  = arrp(s,0,KEY)  - 1 ; 

  if(s->size != sizeof(KEY))
    return FALSE ;

  while (kp++, i--)
    if (lexIsKeyVisible(*kp))
      aceOutPrint (dump_out, "%s : %s\n",
		   className(*kp),
		   freeprotect(name(*kp))) ;

  return TRUE ;
}

/************************************/

BOOL javaDumpKeySet (ACEOUT dump_out, KEY key) 
{
  KEYSET ks = arrayGet(key,KEY,"k") ;
  int i ;
  KEY *kp;
  BOOL first = TRUE ;

  if (!ks) 
    return FALSE;

  aceOutPrint (dump_out, "?Keyset?%s?\t", freejavaprotect(name(key)));
 
  i = keySetMax(ks) ;
  kp = arrp(ks,0,KEY) - 1 ;
  while (kp++,i--)
    if (lexIsKeyVisible(*kp)) 
      { 
	if (!first) 
	  aceOutPrint (dump_out, "\t") ;
	first = FALSE ;
	aceOutPrint (dump_out, "?%s?%s?\n", className(*kp),
		     freejavaprotect(name(*kp)));
      }
  
  aceOutPrint (dump_out, "\n");

  keySetDestroy (ks);

  return TRUE ;
} /* javaDumpKeySet */

/************************************/

BOOL keySetDumpFunc (ACEOUT dump_out, KEY key)
     /* DumpFuncType for class _VKeyset */
{ 
  KEYSET ks = arrayGet(key, KEY, "k");
  BOOL result;

  if (!ks)
    return FALSE;

  result = keySetDump (dump_out, ks);
  keySetDestroy (ks);

  return result;
} /* keySetDumpFunc */

/************************************/

ParseFuncReturn keySetParse (ACEIN parse_io, KEY key, char **errtext)
     /* ParseFuncType */
{
  char *cp = 0 , c;
  KEYSET kS = keySetCreate() ;
  int  classe ; KEY k ;
  ParseFuncReturn result ;

  while (aceInCard(parse_io) && (cp = aceInWordCut(parse_io, ":-/\" ",&c)))
    {
      classe = pickWord2Class(cp) ;
      if (!classe)
	goto abort ;
      if (pickList[classe].protected) /* can't create protected keys */
	continue ;
      aceInStep(parse_io, ':') ;
      if((cp = aceInWord(parse_io)))
	{ 
	  lexaddkey(cp,&k,classe) ;
	  keySetInsert(kS,k) ;
	}
      else
	goto abort ;
    }

  if (keySetMax(kS))
    {
      arrayStore(key,kS,"k") ;
      result = PARSEFUNC_OK ;
    }
  else
    result = PARSEFUNC_EMPTY ;

  keySetDestroy(kS) ;

  return TRUE ;
  
 abort:
  *errtext = messprintf("keySetParse error at  Line %7d  KeySet %s, bad word %s\n", 
			aceInStreamLine(parse_io), name(key),
			cp ? cp : "void line") ;
  keySetDestroy(kS) ;

  return PARSEFUNC_ERR ;
} /* keySetParse */

/****************************************************************/

KEYSET keySetRead (ACEIN keyset_in, ACEOUT error_out) 
     /* read a .ace formatted keyset listing as generated by the List 
      * command and return it as a keyset data structure
      * original by Detlef Wolf, DKFZ
      * new version 951017 from Detlef adpated in minor ways by rd
      * 
      * accept  Locus a;b;c;Sequence e;f  // all on one or multiple lines
     */
{ 
  KEYSET ks = keySetCreate() ;
  int n, table = 0 , tt;
  int nbad = 0 ;
  KEY key;
  char *word ;
    
  while (aceInCard (keyset_in))   /* will close file */
    { 
      while ((word = aceInWord(keyset_in)))
        { 
	  if ((tt = pickWord2Class (word)))
	    {
	      table = tt ;
	      aceInStep (keyset_in,':') ;
	      word = aceInWord(keyset_in) ;
	    }
	  if (tt) table = tt ;
	  if (!table)
            { ++nbad ; continue ; }
          if (!word)
            { 
	      aceOutPrint (error_out, 
			   "// No object name, line %d\n", 
			   aceInStreamLine (keyset_in)) ;
              continue ;
            }
          if (table == _VKeySet && !keySetMax(ks))
            continue ;
          if (lexword2key (word, &key, table))
            keySet(ks, keySetMax(ks)) = key ;
          else
            ++nbad ;
        }
    }

  if (nbad)
    aceOutPrint (error_out, 
		 "// %d objects not recognized\n", nbad) ;
				/* finally sort new keyset */
  n = keySetMax(ks) ;
  keySetSort(ks) ;
  keySetCompress(ks) ;

  if (n - keySetMax(ks)) 
    aceOutPrint (error_out, "// %d duplicate objects removed\n",
		 n - keySetMax(ks)) ;

  return ks ;
} /* keySetRead */

/**************************************************************/
/* kill all 'a' and 'b' type members of given keyset */
void keySetKill (KEYSET ks) 
{
  int i ;
  KEY key ;

  i = keySetExists (ks) ? keySetMax (ks) : 0 ;
   while (i--)
    { key = keySet (ks, i) ;
    switch ( pickType(key))
      {
      case 'A':
        arrayKill (key) ;
        break ;
      case 'B':
        { OBJ obj ;
        if ((obj = bsUpdate (key)))
          bsKill (obj) ;              
        }
        break ;
      }
    }
}

/************************ eof *********************************/
