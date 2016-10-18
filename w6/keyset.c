/*  File: keyset.c
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
 *              A KEYSET is an orderd Array of KEYs                  
 *
 *              public:  keySetAND OR XOR MINUS Order
 *              In addition keyset.h defines as macros                 
 *              keySetInsert/Remove, Create/Destroy , Max, Sort, Find  
 *              consider also w7/ksetdisp.c                             
 *
 * Exported functions: see wh/keyset.h
 *              
 * HISTORY:
 * Last edited: Apr 22 12:35 1999 (fw)
 * Created: Fri Jun  5 16:18:58 1992 (mieg)
 * CVS info:  $Id: keyset.c,v 1.7 2012-10-22 14:57:57 edgrif Exp $
 *-------------------------------------------------------------------
 */

#include <wh/acedb.h>
#include <wh/lex.h>
#include <wh/keyset.h>



#define KEYSIZE sizeof(KEY)



/**************************************************************/
/* Pure chronologic order */
int keySetOrder(void *a,void *b)
{
  return
    (*(KEY*)a) > (*(KEY*)b)  ?
      1 : (*(KEY*)a) == (*(KEY*)b) ? 0 : -1 ;
}


/**************************************************************/
/* This routine returns */
/* the intersect (logical AND) of the first and second KEYSET */

KEYSET keySetAND(KEYSET x, KEYSET y)
{ 
  register int i = 0 , j = 0, k= 0 ;
  KEYSET z = keySetCreate();

  if(!x || !y )
    return z ;
  
  if ((x && x->size != KEYSIZE) || (y && y->size != KEYSIZE))
     messcrash ("keySetOR called on non KEY Array") ;

  while((i<keySetMax(x)) && (j<keySetMax(y)))
    { 
                      /*success, skip further in the 3 index */
      if(arr(x,i,KEY) == arr(y,j,KEY))
	{ array(z,k++,KEY) = arr(x,i++,KEY) ; 
	  j++ ;
	}
      else      /*recall that every index is in increasing order*/
	(keySetOrder(arrp(x,i,KEY),arrp(y,j,KEY)) < 0) ?  i++ : j++ ;
    }
 return z ;
}

/**************************************************************/
  /* This routine returns */
  /* the union (logical OR) of the first and second KEYSET */

KEYSET keySetOR(KEYSET x, KEYSET y)
{ 
  register int i, j, k ;
  KEYSET z = 0 ;

  if(!x && !y )
    return  keySetCreate() ;
  
  if ((x && x->size != KEYSIZE) || (y && y->size != KEYSIZE))
     messcrash ("keySetOR called on non KEY Array") ;

  if(!x)
    { 
      z = arrayCopy(y) ;
      return z ;
    }

  if(!y)
    { 
      z = arrayCopy(x) ;
      return z ;
    }

  z = keySetCreate() ;
  i = j = k = 0 ;
  while((i < keySetMax(x)) && (j < keySetMax(y)))
    { 
                      /*no doubles, skip further in the 3 index */
      if(arr(x,i,KEY) == arr(y,j,KEY))
	{ array(z,k++,KEY) = arr(x,i++,KEY) ; 
	  j++ ;
	}
      else      /*recall that every index is in increasing order*/
	if (keySetOrder(arrp(x,i,KEY),arrp(y,j,KEY)) < 0) 
	  array(z,k++,KEY) =  arr(x,i++,KEY) ;
	else
	  array(z,k++,KEY) =  arr(y,j++,KEY) ;
    }

  if(i == keySetMax(x))
    while (j < keySetMax(y))
      array (z, k++, KEY) =  arr (y, j++, KEY) ;
  else if (j == keySetMax(y))
    while(i < keySetMax(x))
      array (z, k++, KEY) =  arr (x, i++, KEY) ;
  
  return z ;
}

/**************************************************************/
  /* This routine returns */
  /* the exclusive union (logical XOR) of the first and second KEYSET */

KEYSET keySetXOR(KEYSET x, KEYSET y)
{ 
  register int i = 0 , j = 0, k= 0 ;
  KEYSET z = 0 ;


  if(!x && !y )
    return keySetCreate() ; 
  
  if ((x && x->size != KEYSIZE) || (y && y->size != KEYSIZE))
     messcrash ("keySetXOR called on non KEY Array") ;

  if(!x)
    { 
      z = arrayCopy(y) ;
      return z ;
    }

  if(!y)
    { 
      z = arrayCopy(x) ;
      return z ;
    }

  z = keySetCreate() ;
  while((i < keySetMax(x)) && (j < keySetMax(y)))
    { 
                /*discard the intersect */
      if(arr(x,i,KEY) == arr(y,j,KEY))
	{ i++; j++ ;}	
      else      /*recall that every index is in increasing order*/
	{ 
	  if (keySetOrder(arrp(x,i,KEY),arrp(y,j,KEY)) < 0)
	    array(z,k++,KEY) =  arr(x,i++,KEY) ;
	  else
	    array(z,k++,KEY) =  arr(y,j++,KEY) ;
	}
    }

 if(i == keySetMax(x))
   while(j<keySetMax(y))
     array(z,k++,KEY) =  arr(y,j++,KEY) ;
 else if(j == keySetMax(y))
   while(i<keySetMax(x))
     array(z,k++,KEY) =  arr(x,i++,KEY) ;

 return z ;
}

/**************************************************************/
  /* This routine returns */
  /* the difference (logical AND NOT) of the first and second KEYSET */

KEYSET keySetMINUS(KEYSET x, KEYSET y)
{ 
  register int i = 0 , j = 0, k= 0 ;
  KEYSET z = 0 ;

  if(!x)
    return keySetCreate();
  
  if(!y)
    { 
      z = arrayCopy(x) ;
      return z ;
    }

  if ((x && x->size != KEYSIZE) || (y && y->size != KEYSIZE))
     messcrash ("keySetOR called on non KEY Array") ;

  z = keySetCreate();
  while((i<keySetMax(x)) && (j<keySetMax(y)))
     { 
                      /*no doubles, skip further in x y */
      if(arr(x,i,KEY) == arr(y,j,KEY))
	{ i++ ; j++ ;
	}
      else      /*recall that every index is in increasing order*/
	if (keySetOrder(arrp(x,i,KEY),arrp(y,j,KEY)) < 0) 
	  array(z,k++,KEY) =  arr(x,i++,KEY) ;
	else
	  j++ ;
    }

  while(i<keySetMax(x))
     array(z,k++,KEY) =  arr(x,i++,KEY) ;
 
 return z ;
}


/**************************************************************/

/* This routine returns a new keyset containing all the objects in
 * the input keyset that are "empty" objects, i.e. only have an entry
 * in the VOC.
 * 
 * If the input keyset is NULL or an empty keyset or does not contain
 * any empty objects then returns NULL, otherwise returns a keyset
 * containing all the empty objects.
 * 
 */
KEYSET keySetEmptyObj(KEYSET kset)
{ 
  KEYSET empty_kset = NULL ;
  int set_max ;

  if (kset && (set_max = keySetMax(kset)))
    {
      int i = 0 ;
      KEY curr_key ;

      empty_kset = keySetCreate() ;

      for (i = 0 ; i < set_max ; i++)
	{ 
	  curr_key = arr(kset, i, KEY) ;

	  if (iskey(curr_key) == LEXKEY_IS_VOC)
	    keySetInsert(empty_kset, curr_key) ;
	}

      if (!(keySetMax(empty_kset)))
	{
	  keySetDestroy(empty_kset) ;
	  empty_kset = NULL ;
	}
    }
 
  return empty_kset ;
}


/**************************************************************/
/**************************************************************/







