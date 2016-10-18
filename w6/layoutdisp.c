/*  File: layoutdisp.c
 *  Author: Fred Wobus (fw@sanger.ac.uk)
 *  Copyright (C) J Thierry-Mieg and R Durbin, 1999
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
 * Description:
 * Exported functions:
 * HISTORY:
 * Last edited: May  6 10:21 2003 (edgrif)
 * Created: Wed Apr 28 16:10:13 1999 (fw)
 *-------------------------------------------------------------------
 */

/************************************************************/

#include "acedb.h"
#include "aceio.h"
#include "lex.h"
#include "bitset.h"
#include "query.h"
#include "session.h"
#include "pick.h"
#include "whooks/sysclass.h"
#include "dbpath.h"

#include "display.h"
#include "main.h"
#include "layoutdisp.h"

static Graph cGraph = 0;
static int 
 cFirstBox = 0, cFirstBoxEnd = 0 , yLimit, firstXX, firstYY, 
  cSecondBox = 0, cSecondBoxEnd = 0 ,
  cZeroBox = 0, cZeroBoxEnd = 0, cSaveBox = 0 ;
static BitSet cBs = 0 ;
static KEYSET cBox2key = 0 ;
static KEY draggedKey = 0 ;
static Array cLayout = 0 ;
static BOOL cIsModified = FALSE;
static BOOL isSavePossible;  /* do we have writeaccess to layout.wrm ? */

BITSET_MAKE_BITFIELD	       /* Add bitField array for bitset.h ops. */

/************************************************************/

static void layoutDraw (void) ;
static void layoutSave (void);
static void layoutApply (void);
static void layoutDestroy (void);
static void layoutPick (int box, double x_unused, double y_unused, int modifier_unused);

static MENUOPT layoutButtons[] = 
{
  {graphDestroy,  "Quit"},
  {help,          "Help"},
  {graphPrint,    "Print"},
  {menuSpacer,    ""},
  {layoutApply,   "Apply"},
  {0,             0},	/* to be replaced by {layoutSave, "Save"} 
			   if write access to layout.wrm is possible */
  {0,0}
} ;

/************************************************************/

Array layoutRead (STORE_HANDLE handle)
     /* the array starts at position 1 */
{ 
  FILE *fil ;
  char *fName;
  LAYOUT *z ;
  int n = 0, level, x, y = 0 ;
  KEY key ;
  char *card, *word ;
  Array newLayout;

  newLayout = arrayHandleCreate (32, LAYOUT, handle) ;

  fName = dbPathStrictFilName("wspec", "layout","wrm","r", 0);

  if (fName)
    {
      fil = filopen(fName, 0, "r");
      messfree(fName);
    }
  else
    fil = NULL;
  
  if (fil)
    { 
      level = freesetfile (fil, 0) ;
      freespecial ("\n\t") ;
      while ((card = freecard (level)))
	{ 
	  while (TRUE)
	  { 
	    x = freepos() - card ;
	    if (!(word = freeword()))	/* exit loop */
	      break ;
	    if (lexword2key (word, &key, _VClass))
		{ 
		  z = arrayp(newLayout, n++, LAYOUT) ;
		  z->key = key ;
		  z->x = x ;
		  z->y = y ;
		}
/*
	      else
		messout ("%s in wspec/layout.wrm is not a class name", 
                          word) ;
*/
	    }
	  ++y ;
	}
    }
  else
    { 
      int x, i, ncol ;		/* old code using pickVocList */
      int graphWidth, graphHeight ;
      int maxCol[10] ;
      graphFitBounds (&graphWidth,&graphHeight) ;

      for (i = 1 ; i <= pickVocList[0].key ; ++i)
	if (lexword2key (pickVocList[i].text, &key, _VClass))
	  { z = arrayp(newLayout, n++, LAYOUT) ;
	    z->key = key ;
	    z->x = strlen(name(key)) + 2 ;
	  }
	else
	  messout ("pickVocList[%d] = %s is not a class name",
		   i, pickVocList[i].text) ;

      for (ncol = 10 ; ncol > 1 ; --ncol)
	{ for (i = 0 ; i < ncol ; ++i)
	    maxCol[i] = 0 ;
	  x = 0 ;
	  for (i = 0 ; i < arrayMax(newLayout) ; ++i)
	    { z = arrp(newLayout, i, LAYOUT) ;
	      if (z->x > maxCol[(i-1)%ncol])
		{ x += z->x - maxCol[(i-1)%ncol] ;
		  if (x > graphWidth)
		    break ;
		  maxCol[(i-1)%ncol] = z->x ;
		}
	    }
	  if (i == arrayMax(newLayout))
	    break ;
	}
      /* use it */
      x = 2 ;
      for (i = 0 ; i < arrayMax(newLayout) ; ++i)
	{ 
	  z = arrp(newLayout, i, LAYOUT) ;
	  z->x = x ;
	  z->y = y ;
	  if (i % ncol)
	    x += maxCol[(i-1)%ncol] ;
	  else
	    { x = 2 ; ++y ; }
	}

    }

  return newLayout;
} /* layoutRead */



void layoutOpenEditor (void) 
{
  Graph old = graphActive () ;
  int i ;
  LAYOUT *lItem ;
  char *cp;

  if (!graphActivate (cGraph))
    {
      cGraph = displayCreate("DtLayoutEditor");
      /* do these until all displays.wrm specify this type
       * or defaults are implemented elsewhere */
      if (graphHelp(0) == NULL)
	graphHelp("Layout");
      if (strcmp(displayGetTitle("DtLayoutEditor"), "DtLayoutEditor") == 0)
	graphRetitle("Class Layout Editor"); /* change default */
      
      if (!(cp = dbPathStrictFilName("wspec", "layout", "wrm", "w", 0)))
	{
	  /* writing not possible */
	  layoutButtons[5].f = 0;
	  layoutButtons[5].text = 0;
	  isSavePossible = FALSE;
	}
      else
	{
	  messfree(cp);
	  layoutButtons[5].f = layoutSave;
	  layoutButtons[5].text = "Save";
	  isSavePossible = TRUE;
	}
      
      graphMenu (layoutButtons);

      graphRegister (DESTROY, layoutDestroy) ;
      graphRegister (RESIZE, layoutDraw) ;
      graphRegister (PICK, layoutPick) ;
 

      bitSetDestroy (cBs) ;
      cBs = bitSetCreate (80, graphHandle()) ;
      cLayout = layoutRead(graphHandle());

      for (i = 0 ; i < arrayMax(cLayout) ; i++)
	{ 
	  lItem = arrp(cLayout, i, LAYOUT) ;
	  bitSet (cBs, KEYKEY(lItem->key)) ;
	}  

      cIsModified = FALSE;
    }
  layoutDraw() ;
  graphActivate (old) ;

  return;
} /* layoutOpenEditor */


static void layoutDestroy (void)
     /* called when graph dies */
{
  if (cIsModified &&
      isSavePossible &&
      messQuery("Do you want to save the default class layout ?"))
    {
      layoutSave();
    }

  cGraph = 0 ; cBs = 0 ; cBox2key = 0; cSaveBox = 0;
  cLayout = 0;			/* allocated on graphHandle so 
				 * destroyed automatically */
  cIsModified = FALSE;
}

static int cLayoutOrder(void *va, void *vb)
{
  LAYOUT *lla = (LAYOUT*)va,  *llb = (LAYOUT*)vb ;

  if (lla->key)
    {
      if (llb->key)
	return lla->y != llb->y  ? lla->y - llb->y  : lla->x - llb->x ;
      return -1 ;
    }
  return llb->key ? 1 : 0 ;
}


static void layoutApply (void)
{
  int i ;
  LAYOUT *lItem, *newItem ;
  Array newMainLayout = arrayCreate(32, LAYOUT);
  Graph old = graphActive();

  for (i = 0 ; i < arrayMax(cLayout) ; i++)
    { 
      lItem = arrp(cLayout, i, LAYOUT) ;
      if (bitt(cBs,KEYKEY(lItem->key)))
	{ 
	  newItem = arrayp(newMainLayout, i, LAYOUT) ;
	  newItem->x = lItem->x;
	  newItem->y = lItem->y;
	  newItem->key = lItem->key;
	}
    }  

  mainApplyNewLayout(newMainLayout);

  graphActivate (old);
} /* layoutApply */


static void layoutSave (void)
{
  int i ;
  LAYOUT *ll ;  
  ACEOUT layout_out;
  char *filename;

  if (cIsModified)
    {
      messStatus ("Saving class layout");
      
      arraySort (cLayout, cLayoutOrder) ;
      
      filename = dbPathStrictFilName("wspec", "layout", "wrm", "w", 0);
      layout_out = aceOutCreateToFile (filename, "w", 0);
      messfree(filename);
      
      if (!layout_out)
	return ;
      
      for (i = 0 ; i < arrayMax(cLayout) ; i++)
	{ 
	  ll = arrp(cLayout, i, LAYOUT) ;
	  if (bitt(cBs,KEYKEY(ll->key)))
	    { 
	      aceOutxy (layout_out, name(ll->key), ll->x, ll->y) ;
	    }
	}  
      aceOutDestroy (layout_out);
      
      cIsModified = FALSE;

      layoutApply();
    }

  graphBoxDraw(cSaveBox, GRAY, TRANSPARENT);
  graphRedraw();

  return;
} /* layoutSave */


static void cBoxDrag (float *x, float *y, BOOL isDone)
{
  int i, ii ;
  LAYOUT *ll = 0 ;
  int lmax = arrayMax(cLayout);

  if (!isDone)
    return ;
  *x += .2 ; *y -= .2 ;/* before rounding to integer,  easier interface */

  if (*y > yLimit)
    { 
      /* add new line */

      if (class(draggedKey))  /* not on tools */
	{
	  if (bitt (cBs, KEYKEY(draggedKey)))
	    {
	      bitUnSet (cBs, KEYKEY(draggedKey));
	      cIsModified = TRUE;
	    }
	}
      else
	{
	  if (draggedKey == 10)  /* suppress the whole column */
	    {
	      for (i = 0; i <= lmax ; i++)
		{
		  ll = arrayp(cLayout, i, LAYOUT) ;
		  if (ll->key && ll->x > 1)
		    {
		      ll->x -- ;
		      cIsModified = TRUE;
		    }
		}
	    }
	  if (draggedKey >= 100)  /* suppress the whole line */
	    {
	      ii = draggedKey - 100 - 1 ;
	      for (i = 0; i <= lmax ; i++)
		{
		  ll = arrayp(cLayout, i, LAYOUT) ;
		  if (ll->key && ll->y == ii)
		    {
		      if (bitt(cBs, KEYKEY(ll->key)))
			{
			  bitUnSet (cBs, KEYKEY(ll->key));
			  cIsModified = TRUE;
			}
		    }
		  if (ii > -1 && ll->key && ll->y > ii)
		    {
		      ll->y -- ;
		      cIsModified = TRUE;
		    }
		}
	    }
	}
    }
  else
    {
      /* moved within existing layout area */

      if (class(draggedKey))  /* not on tools */
	{
	  ii = *y - firstYY ;
	  if (ii < -1) ii = -1 ;
	  bitSet (cBs, KEYKEY(draggedKey));
	  if (ii<0) /* shift everybody down one line */
	    { ii ++ ;
	      for (i = 0; i <= lmax ; i++)
		{
		  ll = arrayp(cLayout, i, LAYOUT) ;
		  if (ll->key)
		    {
		      cIsModified = TRUE;
		      ll->y += 1 ;
		    }
		}
	    }
	  if (*x < firstXX)   /* shift everybody right */
	    {
	      for (i = 0; i <= lmax ; i++)
		{
		  ll = arrayp(cLayout, i, LAYOUT) ;
		  if (ll->key)
		    {
		      if (firstXX - *x != 0)
			cIsModified = TRUE;
		      ll->x += firstXX - *x ;
		    }
		}
	      *x = firstXX - 1 ;
	    }
	  for (i = 0; i <= lmax ; i++)
	    {
	      ll = arrayp(cLayout, i, LAYOUT) ;
	      if (!ll->key || ll->key == draggedKey)
		{ 
		  if (ll->key == draggedKey &&
		      ll->x == (int)(*x - firstXX + 1) &&
		      ll->y == ii)
		    {
		      ; /* nothing's changed */
		    }
		  else
		    {
		      ll->key = draggedKey ;
		      ll->x = *x - firstXX + 1 ;
		      ll->y = ii ;
		      
		      cIsModified = TRUE;
		    }
		  break ;
		}
	    }
	}
      else
	{
	  switch (draggedKey)
	    {
	    case 1:   /* add empty line */
	      ii = *y - firstYY ; 
	      for (i = 0; i <= lmax ; i++)
		{
		  ll = arrayp(cLayout, i, LAYOUT) ;
		  if (ll->key && ll->y >= ii)
		    {
		      cIsModified = TRUE;
		      ll->y++ ;
		    }
		}

	      break ;
	    }
	}
    }
  layoutDraw() ;
}

static void layoutPick (int box, double x_unused, double y_unused, int modifier_unused)
{
  int n ;

  n = box - cFirstBox - 1 ;
  if (box > cZeroBox && box < cZeroBoxEnd)
    {
      n = box - cZeroBox - 1 ; 
      if (n<0 || !keySetExists(cBox2key) || n >= keySetMax(cBox2key))
	return ;
      draggedKey = keySet(cBox2key,box) ;
      graphBoxDrag (box, cBoxDrag) ;
    }
  else if (box > cFirstBox && box < cFirstBoxEnd)
    {
      n = box - cFirstBox - 1 ;
      if (n<0 || !keySetExists(cBox2key) || n >= keySetMax(cBox2key))
	return ;
      draggedKey = keySet(cBox2key,box) ;
      graphBoxDrag (box, cBoxDrag) ;
    }
  else if (box > cSecondBox && box < cSecondBoxEnd)
    {
      n = box - cSecondBox - 1 ;
      if (n<0 || !keySetExists(cBox2key) || n >= keySetMax(cBox2key))
	return ;
      draggedKey = keySet(cBox2key,box) ; 
      graphBoxDrag (box, cBoxDrag) ;
    }
}

static void layoutDraw (void)
{
  int box, i, j, x, xmax, ymax ;
  float line = 1, y = 0, dy = 0 ;
  KEYSET ks, ksa ;
  KEY key ;
  int graphWidth, graphHeight ;
  LAYOUT *ll = 0 ;

  graphFitBounds (&graphWidth,&graphHeight) ;
  line += 1 ;
  ks = query (0, "FIND Class NOT Buried") ;
  ksa = keySetAlphaHeap(ks, keySetMax(ks)) ;
  keySetDestroy (cBox2key) ;
  cBox2key = arrayHandleCreate(30,KEY,graphHandle()) ;
  y = 0 ;

  graphClear () ; 
  line = 1.2 ;
  cSaveBox = graphButtons (layoutButtons, 1, line, graphWidth) ;
  if (isSavePossible)
    {
      cSaveBox += 4;
      graphBoxDraw(cSaveBox, cIsModified ? BLACK : GRAY, TRANSPARENT);
    }
  else
    cSaveBox = 0;

  line += 1.8 ;
  graphText("Drag the lines and classes with the left mouse button", 1,line); 
  line += 2 ;
  graphText("In the yellow area is your current class layout:", 1,line) ; line += 1.5 ;

  arraySort (cLayout, cLayoutOrder) ;

  /* go to next line to prevent overlappings */
  xmax = 0 ; ymax = 0 ; x = 0 ; y = 0 ; dy = 0 ;
  for (i = 0; i < arrayMax(cLayout) ; ++i)
    { 
      ll = arrp(cLayout, i, LAYOUT) ;
      if (!ll->key || !bitt(cBs,KEYKEY(ll->key)))
	continue ;
      if (ll->y > y) { x = 0 ; y = ll->y ; }
      if (ll->x < x) { x = 0 ; dy++ ; } 
      if (ll->x + strlen(name(ll->key)) + 2 > xmax)
	xmax = ll->x + strlen(name(ll->key)) + 2 ;  
      if (ll->key && ll->y + 2 > ymax)
	ymax = ll->y + 2 ;
      x = ll->x + strlen(name(ll->key)) + 2 ;
      ll->y += dy ;
    }
  /* search min x, y , max = */
  x = y = 1000000 ; 
  for (i = 0; i < arrayMax(cLayout) ; ++i)
    { 
      ll = arrp(cLayout, i, LAYOUT) ;
      if (!ll->key || !bitt(cBs,KEYKEY(ll->key)))
	continue ;
      if (ll->y < y) y = ll->y ; 
      if (ll->x < x) x = ll->x ;  
    }
  /* adjust min x, y , to be 2, 0 */
  x -= 2 ;
  for (i = 0 ; i < arrayMax(cLayout) ; ++i)
    { 
      ll = arrp(cLayout, i, LAYOUT) ;
      if (!ll->key || !bitt(cBs,KEYKEY(ll->key)))
	continue ;
      ll->x -= x ;
      ll->y -= y ;
    }
  /* search xmax ymax */
  xmax = 0 ; ymax = 0 ; 
  for (i = 0 ; i < arrayMax(cLayout) ; ++i)
    { 
      ll = arrp(cLayout, i, LAYOUT) ;
      if (!ll->key || !bitt(cBs,KEYKEY(ll->key)))
	continue ;
      if (ll->x + strlen(name(ll->key)) + 2 > xmax)
	xmax = ll->x + strlen(name(ll->key)) + 2 ;  
      if (ll->y + 2 > ymax)
	ymax = ll->y + 2 ;
    }
  if (!ymax) ymax = 2 ; if (!xmax) xmax = 12 ;

  cZeroBox = graphBoxStart () ;  graphBoxEnd() ; 

  for (j = 0 ; j <= ymax ; j++)
    {
      keySet(cBox2key,box = graphBoxStart ()) = j + 100 ; /* allow 100 other tools */
      for (i = 0 ; i < xmax ; i++)
	graphText("_", i+2, line+j) ;
      graphBoxEnd() ;
      graphBoxDraw(box,BLACK,PALEYELLOW) ;
    }
  cZeroBoxEnd = graphBoxStart () ;  graphBoxEnd() ; 

  cFirstBox = graphBoxStart () ;  graphBoxEnd() ;  
  firstXX = 3 ; firstYY = line + 1 ;

  for (i = 0 ; i < arrayMax(cLayout) ; ++i)
    { 
      ll = arrp(cLayout, i, LAYOUT) ;
      if (!ll->key || !bitt(cBs,KEYKEY(ll->key)))
	continue ;
      keySet(cBox2key,box = graphBoxStart ()) = ll->key ;
      graphText(name(ll->key),ll->x + 2, line + ll->y + 1) ;
      graphBoxEnd() ;
    }  
  cFirstBoxEnd = graphBoxStart () ;  graphBoxEnd() ; 
  line += ymax + 2 ; 
  yLimit = line ;
  graphLine (0,line,200,line) ;
  line += 1 ;
  
  graphText("These are the remaining classes:", 1,line) ;
  cSecondBox = graphBoxStart () ;  graphBoxEnd() ; 
  keySet(cBox2key,box = graphBoxStart ()) = 1 ;
  graphText ("___add_line___", 38,line) ;
  graphBoxEnd () ;
  graphBoxDraw (box, BLACK, PALEYELLOW) ;
  line += 1.5 ;
  for (i = 0 , xmax = 1 ; i < keySetMax (ksa) ; i++)
    { 
      key = keySet (ksa, i) ; 
      if (strcmp(name(key), "System") == 0)
	continue;
      if (bitt(cBs, KEYKEY(key)))
	continue ;
      x = xmax + strlen(name(key)) ;
      if (x > graphWidth)
	{ xmax = 1 ; line++ ; }
      keySet(cBox2key,box = graphBoxStart ()) = key ;
      graphText (name(key), xmax, line) ;
      graphBoxEnd () ;
      xmax += strlen(name(key)) +1 ;
      xmax = ((xmax + 7)/8) * 8 ;
      if (bitt(cBs, KEYKEY(key)))
	graphBoxDraw (box, BLACK, PALEBLUE) ;
    } 
  cSecondBoxEnd = graphBoxStart () ;  graphBoxEnd() ; 
  graphRedraw () ;
  keySetDestroy (ks) ;
  keySetDestroy (ksa) ;

  return;
} /* layoutDraw */


/************************ eof *******************************/

