/*  File: biblio.c
 *  Author: Richard Durbin
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
 * Description: Displays any bibliographical information
 *               for current object.
 * Exported functions:
 *              biblioKey()
 *              biblioKeySet()
 * HISTORY:
 * Last edited: May  6 10:36 2003 (edgrif)
 * Created: Tue Dec 15 16:55:20 1998 (fw)
 *-------------------------------------------------------------------
 */

/* $Id: biblio.c,v 1.48 2003-05-06 13:13:37 edgrif Exp $ */

#include "acedb.h"
#include "aceio.h"
#include "lex.h"
#include "a.h"
#include "whooks/sysclass.h"
#include "whooks/classes.h"
#include "whooks/systags.h"
#include "whooks/tags.h"
#include "query.h"
#include "bs.h"
#include "biblio.h"
#include "pick.h"
#include "client.h"

#ifndef NON_GRAPHIC
#include "graph.h"
#include "display.h"

/************************************************************/

static magic_t BIBLIO_MAGIC = "BIBLIO_MAGIC";

typedef struct BiblioStruct {
  magic_t *magic;		/* == &BIBLIO_MAGIC */
  KEYSET bibSet ;
  Stack s ;
  char *cp ; 
  int zero, curr, width, numberRef, nbDiscard, format;
  int discardUp ;
  float pos ;
  BOOL showAbstracts, showNewNumbers ;
  Array discard, abstract, abstExists, newNumber, sortNumber, author, unDiscard ;
} *BIBLIO ;

static void biblioReDraw (void) ;
static void biblioDoReDraw (BIBLIO biblio) ;
static void biblioDraw80 (BIBLIO biblio) ;
static void biblioPrepare (BIBLIO biblio) ;
static void biblioFormatDump (ACEOUT biblio_out, BIBLIO biblio) ;
static void formatNormal (void) ;
static void formatCell (void) ;
static void formatGenDev (void) ;
static int nbBox = 5 ; /* nombre de boxes par reference */
static BOOL discarding = FALSE ;
#define graphBoxBox(_box) { \
               float _x1, _y1, _x2, _y2 ; \
               graphBoxDim (_box, &_x1, &_y1, &_x2, &_y2) ; \
               graphRectangle (_x1 - 0.4, _y1 - 0.1, _x2 + 0.4, _y2 + 0.1) ; \
                }


#define BIBLIOGET(name) BIBLIO biblio ; \
                      if (!graphAssFind (&BIBLIO_MAGIC, &biblio)) \
		        messcrash ("%s() could not find BIBLIO on graph",name) ; \
		      if (!biblio) \
                        messcrash ("%s() received NULL BIBLIO pointer",name) ; \
                      if (biblio->magic != &BIBLIO_MAGIC) \
                        messcrash ("%s received non-magic BIBLIO pointer",name)

/*************************************************************************/

static void biblioDestroy(void)
{
  BIBLIOGET ("biblioDestroy") ;

  keySetDestroy (biblio->bibSet) ;
  stackDestroy (biblio->s) ;
  arrayDestroy (biblio->discard) ;
  arrayDestroy (biblio->abstract) ;
  arrayDestroy (biblio->abstExists) ;
  arrayDestroy (biblio->newNumber) ;
  arrayDestroy (biblio->sortNumber) ;
  arrayDestroy (biblio->author) ;
  arrayDestroy (biblio->unDiscard) ;
  messfree (biblio->cp) ;
  biblio->magic = 0 ;
  messfree (biblio) ;

  return;
} /* biblioDestroy */

/*************************************************************************/

static void biblioAbstracts (void)
{
  int i ;
  BIBLIOGET ("BiblioAbstracts") ;

  if (!graphCheckEditors (graphActive(), 0))
      return ;  
  biblio->showAbstracts = !biblio->showAbstracts ; /* toggle */
  if (biblio->showAbstracts)
    for (i = 0; i < arrayMax (biblio->bibSet); i++)
      array (biblio->abstract, i, char) = 1 ;
  else
    for (i = 0; i < arrayMax (biblio->bibSet); i++)
      array (biblio->abstract, i, char) = 0 ;  
  biblioPrepare (biblio) ;
  biblioDoReDraw (biblio) ;

  return;
} /* biblioAbstracts */

/*************************************************************************/

static void biblioPrint80 (void)
{
  Graph old = graphActive();
  BIBLIOGET ("BiblioPrint80") ;

  if (!graphCheckEditors (graphActive(), 0))
    return ;

  if (!biblio->bibSet || keySetMax(biblio->bibSet) == 0)
    { messout ("Nothing to export, this biblio is empty, , sorry") ;
      return ;
    }
  biblioDraw80(biblio);
  graphActivate (old);  
  biblioReDraw();

  return;
} /* biblioPrint80 */

/*************************************************************************/


/* mhmp 06 mai 97 */
static void biblioPostBuffer (int ii)
{
  int i = 0 ;
  char *cp, *cq, cc ;
  BIBLIOGET ("biblioPostBuffer") ;

  cp = cq = stackText(biblio->s, 0) ;
  while (i <= ii)
    { while (*cq && *cq != '\n') cq++ ;
      cp++ ;
      if (*cp == '_')
	i++ ;
      cp = ++cq ;
    }
  cp ++ ;
  while (TRUE)
    { while (*cq && *cq != '\n') cq++ ;
      cq++ ;
      if (*cq == '_' || !*cq)
	{ cc = *cq ; *cq = 0 ;
	  graphPostBuffer (cp) ;
	  *cq++ = cc ; 
	  break ;
	}
    }

  return;
} /* biblioPostBuffer */

/*************************************************************************/

static void biblioPick (int k, double x_unused, double y_unused, int modifier_unused)
{
  int b, bb, i, j, ii = -1 ;
  float x1, y1, x2, y2 ;
  BIBLIOGET ("BiblioPick") ;

  if (!graphCheckEditors (graphActive(), 0))
    return ;

  graphWhere (&x1, &y1, &x2, &y2);
  bb = k - biblio->zero ;
  b = bb / nbBox ;
  if (bb >= 0 && b < arrayMax (biblio->bibSet)) 
    { j = -1 ; 
      i = ii = -1 ;
      while (j != b)
	{ i = i + 1 ; ii = array (biblio->sortNumber, i, int) -1 ;
	  if (ii != -1 && !array (biblio->discard, ii, char))
	    j = j + 1 ;
	}
      biblio->pos = (y2 + y1)/2 ;
      if (!(bb % nbBox))
	{
	  biblioPostBuffer (b) ;
	  if (bb == biblio->curr && bb >= 0 && ii != -1)
	    display (keySet(biblio->bibSet, ii), 0, 0) ;
	  else 
	    { if (biblio->curr > -1)
		{ graphBoxDraw (biblio->zero + biblio->curr, BLACK, WHITE) ;
		  graphBoxDraw (biblio->zero + biblio->curr + 2, BLACK, WHITE) ;
		}
	      biblio->curr = bb ;
	      graphBoxDraw (biblio->zero + biblio->curr, BLACK, YELLOW) ;
	      if (ii != -1 && !array (biblio->abstExists, ii, char))
		graphBoxDraw (biblio->zero + biblio->curr + 2, BLACK, YELLOW) ;    
	    }
	}
      else
	if (!((bb - 1) % nbBox))
	  { array (biblio->discard, ii, char) = 1 ;
	    array (biblio->unDiscard, biblio->nbDiscard, int) = ii ;
	    biblio->nbDiscard ++ ;
	    discarding = TRUE ;
	    biblio->numberRef = biblio->numberRef - 1 ;
	    if (bb < biblio->curr)
	      {
		biblio->curr = biblio->curr - nbBox ;
		biblio->discardUp ++ ;
	      }
	    biblioPrepare (biblio) ;
	    biblioDoReDraw (biblio) ;
	  }
	else
	  if (!((bb - 2) % nbBox))
	    if (array (biblio->abstExists, ii, char))
	      { array (biblio->abstract, ii, char) = !array (biblio->abstract, ii, char) ;
		biblioPrepare (biblio) ;
		biblioDoReDraw (biblio) ;
	      }
	
    }
  else
    { if (biblio->curr > -1)
	{ graphBoxDraw (biblio->zero + biblio->curr, BLACK, WHITE) ;
	  graphBoxDraw (biblio->zero + biblio->curr + 2, BLACK, WHITE) ;
	}
      biblio->curr = -1 ;
    }

  return;
} /* biblioPick */

/*************************************************************************/

static void biblioSort (void)
{
  int i, j, rmin, aux ;
  float min ;
  BIBLIOGET ("biblioSort") ;

  if (!graphCheckEditors (graphActive(), 0))
      return ;
  for (i = 0; i < arrayMax (biblio->bibSet) - 1; i++)
    { min = array (biblio->newNumber, i, float);
      rmin = i ;
      for (j = i + 1; j < arrayMax (biblio->bibSet); j++)
	if (min > array (biblio->newNumber, j, float))
	  { min = array (biblio->newNumber, j, float) ;
	    rmin = j ;
	  }
      array (biblio->newNumber, rmin, float) = array (biblio->newNumber, i, float) ;
      array (biblio->newNumber, i, float) = min ;
      aux = array (biblio->sortNumber, i, int) ;
      array (biblio->sortNumber, i, int) = array (biblio->sortNumber, rmin, int) ;
      array (biblio->sortNumber, rmin, int) = aux ;
    }
  biblio->curr = -1 ;
  biblioPrepare (biblio) ;
  biblioDoReDraw (biblio) ;

  return;
} /* biblioSort */

/*************************************************************************/

static void biblioSortAuthor (void)
{
  int i, j, rmin, aux ;
  char *a1, *a2 ;
  float auxf ;
  BIBLIOGET ("biblioSortAuthor") ;

  if (!graphCheckEditors (graphActive(), 0))
      return ;
  for (i = 0; i < arrayMax (biblio->bibSet) - 1; i++)
    { a1 = array (biblio->author, i, char *) ;
      rmin = i ;
      for (j = i + 1; j < arrayMax (biblio->bibSet); j++)
	{ a2 = array (biblio->author, j, char *) ;
	  if (strcmp (a1, a2)>0)
	    { a1 = a2 ;
	      rmin = j ;
	    }
	}
      array (biblio->author, rmin, char *)  = array (biblio->author, i, char *) ;
      array (biblio->author, i, char *) = a1 ;
      auxf = array (biblio->newNumber, i, float) ;
      array (biblio->newNumber, i, float) = array (biblio->newNumber, rmin, float) ;
      array (biblio->newNumber, rmin, float) = auxf ;
      aux = array (biblio->sortNumber, i, int) ;
      array (biblio->sortNumber, i, int) = array (biblio->sortNumber, rmin, int) ;
      array (biblio->sortNumber, rmin, int) = aux ;
    }
  biblio->curr = -1 ;
  biblioPrepare (biblio) ;
  biblioDoReDraw (biblio) ;

  return;
} /* biblioSortAuthor */

/*************************************************************************/

static void biblioUnDiscard (void)
{
  int i, ii ;
  BIBLIOGET ("biblioUnDiscard") ;

  if (!graphCheckEditors (graphActive(), 0))
      return ;
  if (biblio->nbDiscard)
    { i = array (biblio->unDiscard, biblio->nbDiscard - 1, int) ;
      array (biblio->discard, i, char) = 0 ;
      biblio->nbDiscard -- ;
      biblio->numberRef ++ ;
      ii = 0 ;
      while (i + 1 != array(biblio->sortNumber, ii, int))
	{ ii++ ;
	}
      if (( ii-biblio->discardUp) * nbBox <= biblio->curr)
	 {
	   biblio->curr = biblio->curr + nbBox ;
	   biblio->discardUp -- ;
	 }
    }
  biblioPrepare (biblio) ;
  biblioDoReDraw (biblio) ;

  return;
} /* biblioUnDiscard */

/**********************************************************************/

static void biblioNewNumbers (void)
{
  BIBLIOGET ("biblioNewNumbers") ;

  biblio->showNewNumbers = !biblio->showNewNumbers ;
  if (!graphCheckEditors (graphActive(), 0))
    return ;
  biblioPrepare (biblio) ;
  biblioDoReDraw (biblio) ;

  return;
} /* biblioNewNumbers */
  
/**********************************************************************/

static void biblioRenumber (void)
{
  int i, ii, jj ;
  BIBLIOGET ("biblioRenumber") ;

  jj = 0 ;
  for (i = 0; i < arrayMax (biblio->bibSet); i++) 
    { ii = array(biblio->sortNumber, i, int) - 1 ;
      if(!array(biblio->discard, ii, char))
	jj = jj+ 1 ;
      array (biblio->newNumber, i, float) = jj ;
    }
  biblioPrepare (biblio) ;
  biblioDoReDraw (biblio) ;

  return;
} /* biblioRenumber */

/************************************************************************/


static MENUOPT formatMenu[] = {
  { formatNormal,	"Normal" },
  { formatCell,		"Cell" },
  { formatGenDev,	"Gen.& Dev."},
  { 0,0}
};

/***********************************************************************/

static void biblioFormat (void)
{
  BIBLIOGET ("biblioFormat") ;

  biblio->format = -1 ;
  biblioDoReDraw (biblio) ; 
  graphMenu(formatMenu) ;
  graphButtons (formatMenu, 17.5, 4.3, biblio->width) ;

  return;
} /* biblioFormat */

/***********************************************************************/

static MENUOPT biblioMenu[] = {
  { graphDestroy,"Quit"},
  { help,"Help"},
  { biblioPrint80, "Print"},
  { biblioAbstracts, "Abstract on/off"}, 
  { biblioSortAuthor, "Sort by author"},
  { biblioRenumber, "Renumber 1 ... n"},
  { biblioSort, "Sort by number"},
  { biblioUnDiscard, "UnDiscard"},
  { biblioNewNumbers, "New numbers"},
  { biblioFormat, "Format"},
  {0,0}
};

/***********************************************************************/

static void formatNormal (void)
{ 
  BIBLIOGET ("formatNormal") ;

  biblio->format = 0 ;
  biblioPrepare (biblio) ;
  biblioDoReDraw (biblio) ; 
  graphMenu (biblioMenu) ;

  return;
} /* formatNormal */

/**********************************************************************/

static void formatCell (void)
{ 
  BIBLIOGET ("formatCell") ;

  biblio->format = 1 ;
  biblioPrepare (biblio) ;
  biblioDoReDraw (biblio) ; 
  graphMenu (biblioMenu) ;

  return;
} /* formatCell */

/*************************************************************************/  

static void formatGenDev (void)
{ 
  BIBLIOGET ("formatGenDev") ;

  biblio->format = 2 ;
  biblioPrepare (biblio) ;
  biblioDoReDraw (biblio) ; 
  graphMenu (biblioMenu) ;

  return;
} /* formatGenDev */

/*************************************************************************/  

static void biblioDraw80 (BIBLIO biblio)
{
  int box, n = 0, line = 0 ;
  char *cp, *cq, cc ;
  ACEOUT biblio_out;

  biblio->s = stackReCreate (biblio->s, 3000) ;
  biblio_out = aceOutCreateToStack (biblio->s, 0);
  biblio->width =  80;
  biblioFormatDump (biblio_out, biblio) ;
  aceOutDestroy (biblio_out);

  graphClear () ;
  graphText("Topic : ",1.,4.3) ;
  graphTextEntry(biblio->cp, 68, 9.,4.3,0) ;

  cp = cq = stackText(biblio->s, 0) ; n = 0 ;
  graphBoxStart () ;
  line = 6 ;
  while (TRUE)
    { while (*cq && *cq != '\n') cq++ ;
      cc = *cq ; *cq = 0 ;
      if (*cp == '_')
	{ cp++ ;
	  graphBoxEnd() ;
	  box = graphBoxStart () ; 
	  if (!n++)
	    biblio->zero = box ; 
	}
      graphText(cp, 4, line++) ;
      if (!cc)
	break ;
      *cq++ = cc ; cp = cq ; 
    }
  graphBoxEnd () ; 
  graphTextBounds (80, line) ;
  graphRedraw () ;

  graphPrint () ;
  
  return;
} /* biblioDraw80 */


static void biblioReDraw (void)
{
  BIBLIOGET ("BiblioReDraw") ;
  biblioDoReDraw (biblio) ;
}

/**********************************************************/
  
static void biblioDoReDraw (BIBLIO biblio)
{
  int box, i, ii, j, b, n = 0, line = 0 ;
  char *cp, *cq, cc ;

  graphClear () ;
  graphGoto ( 1, biblio->pos) ;
  graphText("Topic : ",1.,5.7) ;
  graphTextEntry(biblio->cp, 68, 9.,5.7,0) ;

  cp = cq = stackText(biblio->s, 0) ; n = 0 ;
  graphBoxStart () ;
  line = 7 ;
  while (TRUE)
    { 
      while (*cq && *cq != '\n') cq++ ;
      cc = *cq ; *cq = 0 ;
      if (*cp == '_')
	{ cp++ ;
	  graphBoxEnd() ;
	  box = graphBoxStart () ; 
	  if (!n++)
	    biblio->zero = box ; 

	  box = graphBoxStart () ;
	  graphText ("Discard", 30, line) ;
	  graphBoxBox (box) ; 
	  graphBoxEnd () ;

	  box = graphBoxStart () ;
	  b = (box - biblio->zero) / nbBox ;
	  j = -1 ;
	  i = ii = -1 ;
	  while (j != b)
	    { i = i + 1 ; ii = array(biblio->sortNumber, i, int) - 1 ;
	      if (!array (biblio->discard, ii, char))
		j = j + 1 ;
	    }


	  if ( array (biblio->abstExists, ii, char))
	    { if (ii != -1 && !array (biblio->abstract, ii, char))
		graphText ("Abstract on ", 40, line) ;
	      else
	        graphText ("Abstract off", 40, line) ;
	      graphBoxBox (box) ;
	    }
	  else
	    graphText ("               ", 40, line) ;
	  graphBoxEnd (); 
	  if (biblio->showNewNumbers)
	    graphFloatEditor ("", arrp(biblio->newNumber, i, float), 55, line, 0) ;
	  else
	    {
	      box = graphBoxStart() ;
	      graphBoxEnd() ;
	      box = graphBoxStart() ;
	      graphBoxEnd() ; 
	    }
	}

      graphText(cp, 4, line++) ;
      if (!cc)
	break ;
      *cq++ = cc ; cp = cq ; /* after the \n */
    }
  graphBoxEnd () ;
  if (!discarding)
    graphTextBounds (-1, line) ;
  else 
    discarding = FALSE ; 
  graphRedraw () ;

  if (biblio->curr < 0 || 
      biblio->curr >= arrayMax(biblio->bibSet) * nbBox ||
      biblio->zero + biblio->curr > n * nbBox)
    biblio->curr = -1 ;
  else
    { graphBoxDraw (biblio->zero + biblio->curr, BLACK, YELLOW) ;
      b = (biblio->curr) / nbBox ;
      j = -1 ;
      i = ii = -1 ;
      while (j != b)
	{ i = i + 1 ;  ii = array (biblio->sortNumber, i, int) -1 ;
	  if (!array (biblio->discard, ii, char))
	    j = j + 1 ;
	}
      if (ii != -1 && !array (biblio->abstExists, ii, char))
	graphBoxDraw (biblio->zero + biblio->curr + 2, BLACK, YELLOW) ;
    }
  box = graphButtons (biblioMenu, 10.0,1.0, biblio->width) ;
  if (biblio->showAbstracts)
    graphBoxDraw (box + 3, BLACK, LIGHTBLUE) ;
  if (biblio->nbDiscard)
    graphBoxDraw (box + 7, BLACK, LIGHTBLUE) ;
  if (biblio->showNewNumbers)
    graphBoxDraw (box + 8, BLACK, LIGHTBLUE) ;
  switch  (biblio->format)
    {
    case -1:
      graphText (" ", 17.5, 4.4) ;
      break ;
    case 0:
      graphText ("Normal", 17.5, 4.4) ;
      break ;
    case 1:
      graphText ("Cell", 17.5, 4.4) ;
      break ;
    case 2:
      graphText ("GENES & DEVELOPMENT", 17.5, 4.4) ;
      break ;
    }

  return;
} /* biblioDoReDraw */

/***********************************************************************/

static void biblioPrepare (BIBLIO biblio)
{
  ACEOUT biblio_out;

  biblio->s = stackReCreate (biblio->s, 3000) ;
  biblio_out = aceOutCreateToStack (biblio->s, 0);
  biblio->width = 80;		/* guess the size of the biblio graph
				 * its real size is configured
				 *  in displays.wrm */
  biblioFormatDump (biblio_out, biblio) ;
  aceOutDestroy (biblio_out);

  return;
} /* biblioPrepare */

/***********************************************************************/

static void cellAuthor (Array aut, int j, char *author) 
{ 
  char *cc ;
  int i, lastSpace, k = 0, nbDot = 0, nbMinus = 0 ;
  cc = name (keySet (aut, j)) ;
  lastSpace = strlen(cc) - 1 ;

/* cherche le blanc derriere le nom*/
  for (i = 0; i < strlen(cc); i++)
    if (cc[i] == ' ')
      lastSpace = i ; 

/* compte . et - dans les initiales*/
  for (i = lastSpace + 1; i < strlen(cc); i++)
    if (cc[i] == '-')
      nbMinus++ ;
    else if (cc[i] == '.')
      nbDot++ ;
  memset (author, 0, 200) ;

/* initiales ? */
  if (strlen (cc) - lastSpace > 4 + nbDot + nbMinus)
    lastSpace = strlen(cc) - 1 ;

/* le nom */
  for (i = 0; i <= lastSpace; i++)
    author[k++] = cc[i] ;
  if (author[k-1] == ' ')
    k = k -1 ;
  if (lastSpace != strlen(cc) - 1)
    {
      author[k++] = ',' ;
      author[k++] = ' ' ;
    }

/* les initiales */
  for (i=lastSpace + 1; i<strlen(cc); i++)
    { 
      if (cc[i] == '.') /* initiales L.S. des le depart*/
	continue ;
      author[k++] = cc[i] ;
      if (!nbMinus)
	author[k++] = '.' ; /* . derriere chaque initiale*/
    }
  author[k] = '\0' ;
} /* cellAuthor */

/********************************************************************/

static void biblioDumpCell (ACEOUT biblio_out, KEYSET bibSet,
			    BOOL showAbstracts, int width)
{
  static Array aut = 0 ;
  int i, j, max, mmax, line, ix, ii, nbLines, nbEmpty, iabst ;
  char *cp, abst ;
  char author [200] ;
  KEY text, kk;
  OBJ Ref ;
  KEY ref ;
  Stack abs, buf = stackCreate (1000) ;
#ifndef NON_GRAPHIC
  BIBLIOGET ("biblioDumpCell");
#endif
 
 if (width <= 0) width = 1 << 24 ;
  if (!keySetExists(bibSet) || !(max = keySetMax(bibSet)))
    { 
      aceOutPrint (biblio_out, "Sorry, no related bibliography") ;
      return ;
    }
  mmax = max ;
#ifndef NON_GRAPHIC
  mmax = biblio->numberRef ;
#endif
  aceOutxy (biblio_out, messprintf("Found %d ref", mmax),1,1);
  
  aut = arrayReCreate (aut, 20, BSunit) ;
  line = 3 ;
  for (i = 0; i < max; i++)
    { 
      abst = 0 ;
      ii = i ;
#ifndef NON_GRAPHIC
      ii = array (biblio->sortNumber, i, int) - 1 ;
      array( biblio->author, i, char *) = "zzzzzz" ;  
      if (array (biblio->discard, ii, char)) continue ;

#endif
      line++ ;
      ref=arr(bibSet,ii,KEY)  ;
      j = i ;

#ifndef NON_GRAPHIC
      j = array (biblio->newNumber, i, float) - 1;
#endif

      aceOutxy (biblio_out, messprintf("_%d) ", j + 1), 0, line++) ;

      aceOutPrint (biblio_out, "%s", name(ref)) ;
      Ref = bsCreate(ref) ;
      if (!Ref) continue ;
      stackClear (buf) ;
      pushText (buf, "") ;
      if (bsFindTag (Ref, _Author) &&
	  bsFlatten (Ref, 1, aut))
	{ stackClear (buf) ;
	  for (j=0 ; j < arrayMax(aut) ; j++)
	    { 
	      cellAuthor (aut, j, author) ;
	      catText  (buf, messprintf ("%s", author)) ;
	      if (j ==  arrayMax(aut) - 2)
		catText (buf, ", and ") ;
	      else
		if (j != arrayMax (aut) - 1)
		    catText (buf, ", ") ;

#ifndef NON_GRAPHIC
		if (!j)
		  array( biblio->author, i, char *) = name (keySet (aut, j)) ; 
#endif
	    }
	}
      if (bsGetData (Ref,_Year,_Int,&ix))
	catText (buf, messprintf (" (%d). ", ix)) ;

      if(bsGetKey (Ref,_Title,&text))
	catText (buf, messprintf ("%s ", name(text))) ;

      if (bsGetKey (Ref, _Journal, &kk))
	catText (buf, name (kk)) ;
 
      if (bsGetData (Ref, _Volume, _Int, &ix))
	{ 
	  catText (buf, messprintf (" %d",ix)) ;
	  while (bsGetData (Ref,_bsRight,_Text,&cp))
	    catText (buf, messprintf (" %s,", cp)) ;
	}
      else if (bsGetData (Ref, _Volume, _Text, &cp))
	{ 
	  catText (buf, messprintf (" %s,", cp)) ;
	  while (bsGetData (Ref,_bsRight,_Text,&cp))
	    catText (buf, messprintf (" %s", cp)) ;
	}
     
      if (bsGetData (Ref, _Page, _Int, &ix))
	{ 
	  catText (buf, messprintf (" %d",ix)) ;
	  while (bsGetData (Ref,_bsRight,_Int,&ix))
	    catText (buf, messprintf ("-%d", ix)) ;
	  catText (buf, ".") ;
	}
      else if (bsGetData (Ref,_Page, _Text, &cp))
	{ 
	  catText (buf, messprintf (" %s", cp)) ;
	  while (bsGetData (Ref,_bsRight,_Text,&cp))
	    catText (buf, messprintf ("-%s",cp)) ;
	  catText (buf, ".") ;
	}
      uLinesText (stackText(buf,0),width - 4) ;
      while ((cp = uNextLine(stackText(buf,0))))
	aceOutxy (biblio_out, cp, 4, line++);
#ifndef NON_GRAPHIC
      if (showAbstracts || array (biblio->abstExists, ii, char))
	{ if (!array (biblio->abstract, ii, char)) 
	  { bsDestroy (Ref) ;
	    continue ;
	  }
	  abst = 1;
	}
#endif
      if (showAbstracts || abst)
	{ if (bsGetKey (Ref, _Abstract, &text) &&
	      (abs = stackGet (text)))
	    { stackClear (buf) ;
	      stackCursor (abs, 0) ;
	      nbLines = 0 ;
	      nbEmpty = 0 ;
	      while ((cp = stackNextText (abs)))
		{ catText (buf, cp) ;
		  catText (buf, "\n") ;	
		  nbLines++ ;
		  if (strlen(cp))
		    nbEmpty = 0 ;
		  else
		    nbEmpty++ ;
		}
	      stackDestroy (abs) ;
	      uLinesText (stackText(buf,0),width - 6) ;
	      for ( iabst = 1 ; iabst <= nbLines - nbEmpty ; iabst++)
		{ cp = uNextLine(stackText(buf,0)) ;
	          aceOutxy (biblio_out, cp, 6, line++);
		}
	    }
	}
      bsDestroy(Ref) ;
    }
  arrayDestroy (aut);
  stackDestroy (buf) ;

  return;
} /* biblioDumpCell */

/****************************************/
static void genDevAuthor (Array aut, int j, char *author) 
{ 
  char *cc ;
  int i, lastSpace , k = 0, nbDot = 0, nbMinus = 0 ;
  cc = name (keySet (aut, j)) ;
  lastSpace = strlen(cc) - 1 ;

  /* cherche le blanc derriere le nom */
  for (i=0; i<strlen(cc); i++)
    if (cc[i] == ' ')
      lastSpace = i ;

  /* compte . et - dans les initiales*/
  for (i=lastSpace + 1; i<strlen(cc); i++)
    if (cc[i] == '-')
      nbMinus++ ;
    else if (cc[i] == '.')
      nbDot++ ;

  /* Jesus Christ mais pas Capet L.S. ou Guevara Che*/
  if (strlen (cc) - lastSpace > 4 + nbDot + nbMinus)
    lastSpace = strlen(cc) - 1 ;
  
  memset (author, 0, 200) ;

  /* initiales*/
  for (i=lastSpace + 1; i<strlen(cc); i++)
    { 
      if (cc[i] == '.')
	continue ;
      author[k++] = cc[i] ;
      if (!nbMinus)
	author[k++] = '.' ; /* . derriere chaque initiale*/      
    }
  if (lastSpace != strlen(cc) -1)/* initiales ? */
      author[k++] = ' ' ;

  /* le nom */
  for (i=0; i<=lastSpace; i++)
    author[k++] = cc[i] ;
  if (author[k-1] == ' ')
    k = k -1 ;
  author[k] = '\0' ;
}
/*************************************************************************/
static void biblioDumpGenDev (ACEOUT biblio_out, KEYSET bibSet,
			      BOOL showAbstracts, int width)
{
  static Array aut = 0 ;
  int i, j, max, mmax, line, ix, ii, nbLines, nbEmpty, iabst ;
  char *cp, abst ;
  char author [200] ;
  KEY text, kk;
  OBJ Ref ;
  KEY ref ;
  Stack abs, buf = stackCreate (1000) ;
#ifndef NON_GRAPHIC
  BIBLIOGET ("biblioDumpGenDev");
#endif

  if (width <= 0) width = 1 << 24 ;
  if (!keySetExists(bibSet) || !(max = keySetMax(bibSet)))
    {
      aceOutPrint (biblio_out, "Sorry, no related bibliography") ;
      return ;
    }
  mmax = max ;
#ifndef NON_GRAPHIC
  mmax = biblio->numberRef ;
#endif
  aceOutxy (biblio_out, messprintf("Found %d ref", mmax), 1, 1);
  
  aut = arrayReCreate (aut, 20, BSunit) ;
  line = 3 ;
  for (i = 0; i < max ; i++)
    { 
      abst = 0 ;
      ii = i ;
#ifndef NON_GRAPHIC
      ii = array (biblio->sortNumber, i, int) - 1 ;
      array( biblio->author, i, char *) = "zzzzzz" ;  
      if (array (biblio->discard, ii, char)) continue ;

#endif
      line++ ;
      ref=arr(bibSet,ii,KEY)  ;
      j = i ;

#ifndef NON_GRAPHIC
      j = array (biblio->newNumber, i, float) - 1;
#endif

      aceOutxy (biblio_out, messprintf("_%d) ", j + 1), 0, line++) ;

      aceOutPrint (biblio_out, "%s", name(ref)) ;
      Ref = bsCreate(ref) ;
      if (!Ref) continue ;
      stackClear (buf) ;
      pushText (buf, "") ;
      if (bsFindTag (Ref, _Author) &&
	  bsFlatten (Ref, 1, aut))
	{ 
	  stackClear (buf) ;
	  for (j=0 ; j < arrayMax(aut) ; j++)
	    { 
	      if (!j)
		cellAuthor (aut, j, author) ;
	      else
		genDevAuthor (aut, j, author) ;
	      catText  (buf, messprintf ("%s", author)) ;
	      if (j ==  arrayMax(aut) - 2)
		{
		  if (arrayMax (aut) > 2)
		    catText (buf, ", and ") ;
		  else
		    catText (buf, " and ") ;
		}
	      else
		{
		  if (j != arrayMax (aut) - 1)
		    catText (buf, ", ") ;
		}
	      
#ifndef NON_GRAPHIC
		if (!j)
		  array( biblio->author, i, char *) = name (keySet (aut, j)) ; 
#endif /* !NON_GRAPHIC */
	    }
	  if (arrayMax(aut) > 1)
	     catText (buf, ".") ;
	}
      if (bsGetData (Ref,_Year,_Int,&ix))
	catText (buf, messprintf (" %d. ", ix)) ;

      if(bsGetKey (Ref,_Title,&text))
	catText (buf, messprintf ("%s ", name(text))) ;

      if (bsGetKey (Ref, _Journal, &kk))
	catText (buf, name (kk)) ;
 
      if (bsGetData (Ref, _Volume, _Int, &ix))
	{ 
	  catText (buf, messprintf (" %d",ix)) ;
	  while (bsGetData (Ref,_bsRight,_Text,&cp))
	    catText (buf, messprintf (" %s", cp)) ;
	}
      else if (bsGetData (Ref, _Volume, _Text, &cp))
	{ 
	  catText (buf, messprintf (" %s:", cp)) ;
	  while (bsGetData (Ref,_bsRight,_Text,&cp))
	    catText (buf, messprintf (" %s", cp)) ;
	}
     
      if (bsGetData (Ref, _Page, _Int, &ix))
	{ 
	  catText (buf, messprintf (" %d:",ix)) ;
	  while (bsGetData (Ref,_bsRight,_Int,&ix))
	    catText (buf, messprintf ("-%d", ix)) ;
	  catText (buf, ".") ;
	}
      else if (bsGetData (Ref,_Page, _Text, &cp))
	{ 
	  catText (buf, messprintf (" %s", cp)) ;
	  while (bsGetData (Ref,_bsRight,_Text,&cp))
	    catText (buf, messprintf ("-%s",cp)) ;
	  catText (buf, ".") ;
	}
      uLinesText (stackText(buf,0),width - 4) ;
      while ((cp = uNextLine(stackText(buf,0))))
	aceOutxy (biblio_out, cp,4,line++);
#ifndef NON_GRAPHIC
      if (showAbstracts || array (biblio->abstExists, ii, char))
	{ if (!array (biblio->abstract, ii, char)) 
	  { bsDestroy (Ref) ;
	    continue ;
	  }
	  abst = 1;
	}
#endif
      if (showAbstracts || abst)
	{ if (bsGetKey (Ref, _Abstract, &text) &&
	      (abs = stackGet (text)))
	    { stackClear (buf) ;
	      stackCursor (abs, 0) ;
	      nbLines = 0 ;
	      nbEmpty = 0 ;
	      while ((cp = stackNextText (abs)))
		{ catText (buf, cp) ;
		  catText (buf, "\n") ;	
		  nbLines++ ;
		  if (strlen(cp))
		    nbEmpty = 0 ;
		  else
		    nbEmpty++ ;
		}
	      stackDestroy (abs) ;
	      uLinesText (stackText(buf,0),width - 6) ;
	      for ( iabst = 1 ; iabst <= nbLines - nbEmpty ; iabst++)
		{ cp = uNextLine(stackText(buf,0)) ;
	          aceOutxy (biblio_out, cp, 6, line++);
		}
	    }
	}
      bsDestroy(Ref) ;
    }
  arrayDestroy (aut);
  stackDestroy (buf) ;

  return;
} /* biblioDumpGenDev */

/*************************************************************************/

static void biblioDisplay (char *title, KEYSET bibSet)
{
  BIBLIO biblio ;
  int i ;
  OBJ obj ;
  KEY text ;

  if (!title || !keySetExists (bibSet))
    messcrash ("biblioDisplay - received NULL title or "
	       "invalid KEYSET bibSet") ;
  if (!keySetMax (bibSet))
    { 
      messout ("Sorry, no associated biblio") ;
      return ;
    }

  biblio = (BIBLIO) messalloc (sizeof(struct BiblioStruct)) ;
  biblio->magic = &BIBLIO_MAGIC ;
  biblio->bibSet = bibSet ;
  biblio->s = stackCreate (3000) ;
  biblio->curr = -1 ;
  biblio->nbDiscard = 0 ;
  biblio->discardUp = 0 ;
  biblio->format = -1 ;
  biblio->numberRef = arrayMax(biblio->bibSet) ;
  biblio->cp = messalloc(70) ;
  biblio->discard = arrayCreate (arrayMax(biblio->bibSet), char) ;
  biblio->abstract = arrayCreate (arrayMax(biblio->bibSet), char) ;
  biblio->abstExists = arrayCreate (arrayMax(biblio->bibSet), char) ;
  biblio->newNumber = arrayCreate (arrayMax(biblio->bibSet), float) ;
  biblio->sortNumber = arrayCreate (arrayMax(biblio->bibSet), int) ;
  biblio->author = arrayCreate (arrayMax(biblio->bibSet), char *) ;
  biblio->unDiscard = arrayCreate (arrayMax(biblio->bibSet), int) ;
  for (i = 0; i< arrayMax(biblio->bibSet); i++) 
       { array (biblio->newNumber, i, float) = i + 1 ;
	 array (biblio->sortNumber, i, int) = i + 1 ;
	 if ((obj = bsCreate (arr (bibSet, i, KEY))))
	   { if (bsGetKey (obj, _Abstract, &text) && iskey(text)==2)
	     array (biblio->abstExists, i, char) = 1 ;
	   bsDestroy (obj) ;
	   }
       }
  strncpy(biblio->cp, title, 70);
  biblio->cp[70-1] = '\0';
  

  displayCreate ("DtBiblio") ;

  graphRetitle (title) ;
  graphMenu (biblioMenu) ;
  graphRegister (DESTROY, biblioDestroy) ;
  graphRegister (PICK, biblioPick) ;
  graphRegister (RESIZE, biblioReDraw) ;
  graphAssociate (&BIBLIO_MAGIC, biblio) ;

  biblioPrepare (biblio) ;
  biblioDoReDraw (biblio) ;  

  return;
} /* biblioDisplay */

/***********************************************************************/

static void biblioFormatDump (ACEOUT biblio_out, BIBLIO biblio)
{
  switch (biblio->format)
    {
    case -1 :
    case 0 :
      biblioDump (biblio_out, biblio->bibSet, biblio->showAbstracts, biblio->width) ;
      break ;

    case 1 :
      biblioDumpCell (biblio_out, biblio->bibSet, biblio->showAbstracts, biblio->width) ;
      break ;

    case 2 :
      biblioDumpGenDev (biblio_out, biblio->bibSet, biblio->showAbstracts, biblio->width) ;
      break ;
    }

 return; 
} /* biblioFormatDump */

/***********************************************************************/
/***************  Public Graphic display functions *********************/

void biblioKeySet (char *title, KEYSET s)
{ 
  char *cr;

  cr = strnew (title ? title : "Biblio", 0);
  biblioDisplay (cr, biblioFollow (s)) ;
  messfree (cr) ;

  return;
} /* biblioKeySet */

/***********/

void biblioKey (KEY key)
{
  KEYSET set = keySetCreate() ;

  keySet(set,0) = key ;
  biblioKeySet (messprintf("Bibliography attached to %s",
			 name(key)), set) ;
} /* biblioKey */

#endif /* NON_GRAPHIC */

/***********************************************************************/
/***************  Public elementary functions  *************************/

static BOOL biblioKeyPossible2 (KEY key, BOOL doOpen)
{ 
  KEYSET ks = 0 ;
  int i ;
  KEY *kp ;
  BOOL found = FALSE ;

  if (!key || !class(key) || (pickType(key) != 'B'))
    return FALSE ;
  else if (class(key) == _VPaper || class(key) == _VLongText)
    return TRUE ;
  else if (!bsIsClassInClass (class(key), _VPaper))
    return FALSE ;
  if (!doOpen) return TRUE ;
  ks =  bsKeySet(key) ;
  if (!ks) return FALSE ;
  kp = arrp(ks, 0, KEY) - 1 ;
  i = keySetMax(ks) ;
  while (kp++, i--)
    if (class(*kp) == _VPaper)
      { found = TRUE ; break ; }
  keySetDestroy (ks) ;

  return found ;
} /* biblioKeyPossible2 */

BOOL biblioKeyPossible (KEY key)
{ return  biblioKeyPossible2 (key, TRUE) ; }

BOOL biblioPossible (KEYSET s)
{ 
  KEY key = 0 ;
  int i , dx, max ;

  if (!keySetExists (s) || !(max = keySetMax (s)))
    return FALSE ;
  /* try just on class names, very cheap */
  dx = max/60 ; if (!dx) dx = 1 ;
  for (i = 0; i < max ; i += dx)
    {
      key = keySet(s, i) ;
      if (class(key) == _VPaper || class(key) == _VLongText)
	return TRUE ;
    }
  /* try again, opening just the models */
  dx = max/10 ; if (!dx) dx = 1 ;
  for (i = 0; i < max ; i += dx)
    {
      key = keySet(s, i) ;
      if (biblioKeyPossible2 (key, FALSE))
	goto maybe ;
    }
  return FALSE ;
 
maybe:
   if ((TRUE || externalServer)
       && max > 1 ) return TRUE ; /* do not wait on server, or probably on any machine */
 /* try again, possibly opening objects */
  dx = max/10 ; if (!dx) dx = 1 ;
  for (i = 0; i < max ; i += dx)
    {
      key = keySet(s, i) ;
      if (biblioKeyPossible2 (key, TRUE))
	return TRUE ;
    }
  return FALSE ;
} /* biblioPossible */


KEYSET biblioFollow (KEYSET s)
{
  KEYSET bib1, bib2, bib3, biba ;

  bib1 = query (s, "NEIGHBOURS") ;  /* was {>Paper} $| {>Reference} $| {>Quoted_in}") ; */
  bib2 = keySetOR (s, bib1) ;
  bib3 = query (bib2, "CLASS Paper") ; 
  biba = keySetAlphaHeap(bib3, keySetMax(bib3)) ;

  keySetDestroy(bib1);
  keySetDestroy(bib2);
  keySetDestroy(bib3);

  return biba ;
} /* biblioFollow */


/***********************************************************************/

void biblioDump (ACEOUT biblio_out, KEYSET bibSet, BOOL showAbstracts, int width)
{
  static Array aut = 0 ;
  int i, j, max, mmax, line, ix, ii, nbLines, nbEmpty, iabst ;
  char *cp, abst ;
  KEY text, kk;
  OBJ Ref ;
  KEY ref ;
  Stack abs, buf = stackCreate (3000) ;
#ifndef NON_GRAPHIC
  /* do not use BIBLIOGET here, cause it is used by gifaceserver */
  BIBLIO biblio = 0 ;

  if (graphActive())
    { 
      if (!graphAssFind (&BIBLIO_MAGIC, &biblio))
	biblio = 0 ;
      if (biblio && biblio->magic != &BIBLIO_MAGIC)
	biblio = 0 ;
    }
#endif

  if (width <= 0) width = 1 << 24 ;
  if (!keySetExists(bibSet) || !(max = keySetMax(bibSet)))
    {
      aceOutPrint (biblio_out, "Sorry, no related bibliography") ;
      return ;
    }
  mmax = max ;
#ifndef NON_GRAPHIC
  mmax = biblio ? biblio->numberRef : max ;
#endif
  aceOutxy (biblio_out, messprintf("Found %d ref", mmax),1,1);
  
  aut = arrayReCreate (aut, 20, BSunit) ;
  line = 3;
  for (i = 0; i < max; i++)
    { 
      abst = 0 ;
      ii = i ;
#ifndef NON_GRAPHIC
      if(biblio)
	{ ii = array (biblio->sortNumber, i, int) - 1 ;
	array( biblio->author, i, char *) = "zzzzzz" ;  
	if (array (biblio->discard, ii, char)) continue ;
	}

#endif
      line++ ;
      ref=arr(bibSet,ii,KEY)  ;
      j = i ;

#ifndef NON_GRAPHIC
      if (biblio)
	j = array (biblio->newNumber, i, float) - 1;
#endif

      aceOutxy (biblio_out, messprintf("_%d) ", j + 1), 0, line++) ;

      aceOutPrint (biblio_out, "%s", name(ref)) ;
      Ref = bsCreate(ref) ;
      if (!Ref) continue ;
      
      stackClear (buf) ;
      if(bsGetKey (Ref,_Title,&text))
	{ 
	  pushText (buf, name(text)) ;
	  uLinesText (stackText(buf,0),width - 4) ;
	  while ((cp = uNextLine(stackText(buf,0))))
	    aceOutxy (biblio_out, cp, 4, line++);
	}

      if (bsFindTag (Ref, _Author) &&
	  bsFlatten (Ref, 1, aut))
	{ stackClear (buf) ;
	  for (j=0 ; j < arrayMax(aut) ; j++)
	    { if (j)
		catText (buf, messprintf (", %s", name(keySet(aut,j)))) ;
	    else
	      { catText (buf, name(keySet(aut,j))) ;
#ifndef NON_GRAPHIC
		if (biblio)
		  array( biblio->author, i, char *) = name (keySet (aut, j)) ; 
#endif
	      }
	    }
	  catText (buf, ".") ;
	  uLinesText (stackText(buf,0),width - 4) ;
	  while ((cp = uNextLine(stackText(buf,0))))
	    aceOutxy (biblio_out, cp, 4, line++);
	}

      j = 0 ;
      if (bsGetKey (Ref, _Journal, &kk))
	{ j = 1 ; aceOutxy (biblio_out, name(kk), 4, line++) ; }
 
      if (bsGetData (Ref, _Volume, _Int, &ix))
	{
	  if (!j) 
	    { j = 1 ; aceOutxy (biblio_out, "  ", 4, line++) ; }

	  aceOutPrint (biblio_out, " %d",ix) ;
	  while (bsGetData (Ref,_bsRight,_Text,&cp))
	    aceOutPrint (biblio_out, " %s", cp) ;
	}
      else if (bsGetData (Ref, _Volume, _Text, &cp))
	{ 
	  if (!j) 
	    { j = 1 ; aceOutxy (biblio_out, "  ", 4, line++) ; }
	  aceOutPrint (biblio_out, " ") ; 
	  aceOutPrint (biblio_out, "%s", cp) ;
	  while (bsGetData (Ref,_bsRight,_Text,&cp))
	    aceOutPrint (biblio_out, " %s", cp) ;
	}
      
      if (bsGetData (Ref, _Page, _Int, &ix))
	{
	  if (!j) 
	    { j = 1 ; aceOutxy (biblio_out, "  ", 4, line++) ; }
	  aceOutPrint (biblio_out, ": %d",ix) ;
	  while (bsGetData (Ref,_bsRight,_Int,&ix))
	    aceOutPrint (biblio_out, "-%d", ix) ;
	}
      else if (bsGetData (Ref,_Page, _Text, &cp))
	{
	  if (!j) 
	    { j = 1 ; aceOutxy (biblio_out, "  ", 4, line++) ; }
	  aceOutPrint (biblio_out, ", %s", cp) ;
	  while (bsGetData (Ref,_bsRight,_Text,&cp))
	    aceOutPrint (biblio_out, "-%s",cp) ;
	}
      if (bsGetData (Ref,_Year,_Int,&ix))
	{ 
	  if (!j) 
	    { j = 1 ; aceOutxy (biblio_out, "  ", 4, line++) ; }
	  aceOutPrint (biblio_out, " (%d)",ix) ;
	}
#ifndef NON_GRAPHIC
      if (biblio &&(showAbstracts || array (biblio->abstExists, ii, char)))
	{ if (!array (biblio->abstract, ii, char)) 
	  { bsDestroy  (Ref) ;
	    continue ;
	  }
	abst = 1;
	}
#endif
      if (showAbstracts || abst)
	{
	  if (bsGetKey (Ref, _Abstract, &text) &&
	      (abs = stackGet (text)))
	    { stackClear (buf) ;
	      stackCursor (abs, 0) ;
	      nbLines = 0 ;/* mhmp 07/05/97 */
	      nbEmpty = 0 ;
	      while ((cp = stackNextText (abs)))
		{
		  catText (buf, cp) ;
		  catText (buf, "\n") ;
		  nbLines++ ;
		  if (strlen(cp))
		    nbEmpty = 0 ;
		  else
		    nbEmpty++ ;
		}
	      stackDestroy (abs) ;
	      uLinesText (stackText(buf,0),width - 6) ;
	      for ( iabst = 1 ; iabst <= nbLines - nbEmpty ; iabst++)
		{ cp = uNextLine(stackText(buf,0)) ;
		  aceOutxy (biblio_out, cp, 6, line++);
		}
	    }
	}	
      bsDestroy(Ref) ;
    }
  arrayDestroy (aut);
  stackDestroy (buf) ;

  return;
} /* biblioDump */

/***********************************************************************/
/***********************************************************************/
