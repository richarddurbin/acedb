/*  File: fmapsequence.c
 *  Author: Richard Durbin (rd@mrc-lmb.cam.ac.uk)
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
 * Description: sequence display and manipulation for fmap
 * Exported functions:
 * HISTORY:
 * Last edited: Aug  1 14:00 2003 (edgrif)
 * * Apr 20 12:07 2000 (edgrif): Had to change MapColDrawFunc prototypes.
 * * Mar 18 11:43 1999 (edgrif): Added proper POSIX based min/max int values.
 * * Jul 27 10:34 1998 (edgrif): Add fMapInitialise() calls for externally
 *      called routines.
 * * Jul 16 10:07 1998 (edgrif): Introduce private header fmap_.h
 * Created: Sat Jul 25 22:36:47 1992 (rd)
 * CVS info:   $Id: fmapsequence.c,v 1.71 2003-08-01 13:03:15 edgrif Exp $
 *-------------------------------------------------------------------
 */

#include <wh/acedb.h>
#include <wh/aceio.h>
#include <wh/chrono.h>
#include <whooks/classes.h>
#include <whooks/tags.h>
#include <whooks/systags.h>
#include <wh/lex.h>
#include <wh/dna.h>
#include <wh/session.h>
#include <wh/peptide.h>
#include <wh/restriction.h>
#include <wh/method.h>
#include <wh/methodcache.h>
#include <w7/fmap_.h>
#include <wh/smap.h>

/*************************************************************/


static void showGeneTranslation(LOOK genericLook, float *offset,
				char *map_col_name, char *err_msg) ;
static void showSequence(FeatureMap look, float offset, int width,
			 BOOL isProtein, Array translationTable) ;


/* Bit masks for colours in the dna coloring code. These are significant 
   in graphColorSquares. If more than one bit is set, the first colour
   in this list actually gets displayed. 
   These were in graph.h until graphColorSquares was generalised.
   The following must correspond with TINT_* #defines in fmap.h.
*/

static int fMapTints[8] = { PALERED, LIGHTBLUE, RED, LIGHTGRAY, 
			    MAGENTA, CYAN, LIGHTGREEN, YELLOW } ;

static char *geneTranslation = 0 ;
static int *geneTransPos = 0 ;  /*mhmp 11.06.98 */
static BOOL upTranslation = FALSE ;


/*****************************************************************/
/* This is a hack for now, users don't like the pale pink highlighting
 * of the DNA, so allow them to set their own. The whole area of user
 * settings for fmap needs sorting out.... */
void fmapSetDNAHightlight(int new_hightlight)
{
  messAssert(new_hightlight >= WHITE && new_hightlight <= MIDBLUE) ;

  fMapTints[0] = new_hightlight ;

  return ;
}




/*****************************************************************/

/***************** show sequences/coordinates ********************/

/* Sets up parameters for code that displays the actual letters of a dna or  */
/* protein sequence in the fmap.                                             */
/* whoah, crazy code...no comments, I've looked at it but I'm still not sure */
/* I understand exactly what its trying to do, just the odd comment would    */
/* have been enough to help. I have been able to glean some bits:            */
/*                                                                           */
/* width = the number of characters to print                                 */
/* start = where to start reading the characters to be printed               */
/*  skip = number of characters to skip/truncate                             */
/*                                                                           */
/* The function will always return a width of 60 with decreasing skip until  */
/* until width = skip = 60, after this width and skip decrease together down */
/* to width = skip = 1                                                       */
/*                                                                           */
static void makeStartSkip(FeatureMap look, float offset,
			  int *width_inout, int *start_out, int *skip_out)
{
  int width = *width_inout ;
  int start, skip ;
  float dy ;

 lao:							    /* WATCH OUT for crazy looping... */
  start = ((COORD (look, look->min) - 1) / width) * width ;
  if (look->flag & FLAG_REVERSE)
    {
      start = look->length - look->origin - start - 1 ; 
      for (skip = width ; -skip * look->map->mag < 1.0 ; skip += width) ;
    }
  else
    {
      start += look->origin ;
      for (skip = width ; skip * look->map->mag < 1.0 ; skip += width) ;
    }

  if (start > look->min)
    start -= width ;

  look->dnaWidth = width ;	/* this is correct column width */

  if (skip > width && width > 3)
    width -= 3 ;		/* for the "..." */

  if (look->flag & FLAG_AUTO_WIDTH)
    {
      dy = skip ;
      dy = MAP2GRAPH (look->map, dy) - MAP2GRAPH (look->map, 0) ;
      if (dy * dy > 4.1 && width > 1)
	{
	  width /= 2 ;
	  if (width == 2)				    /* I'm not sure this ever happens. */
	    width = 1 ;
	  if (width > 2)
	    width = 3 * (width / 3) ;

	  goto lao ;
	}

      if (skip > width && skip > 2 && (width+=3) && width < 60)
	{
	  /* Don't know if this ever happens.                                */
	  width = (skip < 60 ? skip : 60) ;		    /* should be widthmax */
	  if (width > 1)
	    width = 3 * ((width + 2) / 3) ;
	  goto lao ;
	}
      else if (skip == 2 && width == 1)
	{
	  /* Loop code erroneously sets skip = 2, width = 1, this fixes it.  */
	  width = skip ;
	}
    }

  *width_inout = width ;
  *start_out = start ;
  *skip_out = skip ;
  look->dnaStart = start ;
  look->dnaSkip = skip ;

  return ;
}

/* Show the dna sequence within the fmap window.
 * If isProtein is TRUE and translation_table is non-NULL then translation_table
 * will be used to do the translation. */
static void showSequence (FeatureMap look, float offset, int width,
			  BOOL isProtein, Array translationTable)
{ 
  BOOL isProt = FALSE, isFirst ;
  int decalCoeff = 0 ; int c1, c2;
  int start, skip, end, box, tframe = 0 ; int oldStart ;
  int ii= 0, oldDecal, decalCoord = UT_INT_MIN, jj = 0, delta ;
  float y ;
  int seqEnd, x ;
  register char *cp ;
  register int i ;
  char letter[2] ;
  SEG *seg ;
  BOOL doComplementSurPlace = (look->flag & FLAG_COMPLEMENT_SUR_PLACE) &&  ! isProtein ;
  
  if (isProtein && translationTable == NULL)
    {
      if (!(translationTable = pepGetTranslationTable (look->seqOrig, 0)))
	return ;
    }

  letter[1] = 0 ;
  if (isProtein && geneTranslation)
    {
      arrayDestroy(look->minDna) ;
      arrayDestroy(look->maxDna) ;
      arrayDestroy(look->decalCoord) ;
      arrayDestroy(look->tframe) ;
      look->minDna = arrayCreate(1000, int) ;
      look->maxDna = arrayCreate(1000, int) ;
      look->decalCoord = arrayCreate(1000, int) ;
      look->tframe = arrayCreate(1000, int) ; 
      if (look->flag & FLAG_REVERSE)
	array(look->maxDna, jj++, int) = UT_INT_MAX ;
      else
	array(look->maxDna, jj++, int) = UT_INT_MIN ;
      decalCoeff = -1 ;
      if (upTranslation)
	decalCoeff *= -1 ;
      if (look->flag & FLAG_REVERSE)      
	decalCoeff *= -1 ;    
    }

  makeStartSkip (look, offset, &width, &start, &skip) ;
  
  if (isProtein)
    {
      seqEnd = look->max - 2 ;
      if (!geneTranslation)
	while (start%3 != look->min%3)
	  ++start ; /*mhmp 08.10.98 ++  --> -- */
    }
  else
    seqEnd = look->max ;

  graphTextFormat (FIXED_WIDTH) ;
  /* mhmp modif 26.10 deb */
  oldStart = start ;
  for (; start < seqEnd ; start += 60)
    {
      end = start + 60 ;
      if (end > seqEnd)
	end = seqEnd + 2 ;

      if (isProtein && !geneTranslation && start%3 != look->min%3)
	continue ;

      tframe = 0 ;
      if (isProtein && geneTranslation)
	{
	  for (i = start ; i < end ; i++)
	    if (i>look->min &&  geneTranslation[i-look->min] != '%'  &&
		geneTranslation[i-look->min] != 0  &&
		geneTranslation[i-look->min] != '-')
	      break ;
	  if (i < end) /* success */
	    {
	      tframe = (i - start + look->min) % 3 ;
	      if (tframe < 0 )
		tframe += 3 ;
	    }
	}

      isFirst = TRUE ; delta = 1 ;
      if (isProtein && geneTranslation)
	{
	  for (i = start ; i < end ; i +=delta)
	    if (i >= look->min)
	      {
		if (geneTransPos[i-look->min + tframe])
		  { 
		    if (isFirst)
		      {
			isFirst = FALSE ;
			delta = 3 ;
		      }
		    oldDecal = decalCoord ;
		    decalCoord = COORD(look, i + tframe) + 3 * decalCoeff * geneTransPos[i-look->min + tframe] ;
		    if (oldDecal != decalCoord)
		      {
			c1 = COORD(look, i + tframe) ;
			array(look->minDna, ii, int) = COORD(look, i + tframe) ;
			array(look->decalCoord, ii, int) = decalCoord ;
			array(look->tframe, ii, int) = tframe ;
			ii++ ;
			if (ii > jj)  
			  {
			    array(look->maxDna, jj++, int) = COORD(look, i - 1 + tframe) ;
			  c2 =COORD(look, i + tframe);
			  } 
			isProt = TRUE ;
		      }
		  }
		else
		  {
		    if (isProt)
		      {
			c2 = COORD(look, i - 1 + tframe) ;
			array(look->maxDna, jj++, int) = COORD(look, i - 1 + tframe) ;
			isProt = FALSE ;
		      } 
		  }
	      }
	}
    }

  if (isProtein && geneTranslation) 
    if (arrayMax(look->maxDna) == arrayMax(look->minDna))
      array(look->maxDna,arrayMax(look->maxDna) , int) = COORD(look, seqEnd + tframe) ;

  start = oldStart ;
  /* mhmp modif 26.10 fin */ 
  for (; start < seqEnd ; start += skip)
    {
      int s ;

      y = MAP2GRAPH(look->map, start) - 0.5 ;
      if (y < topMargin)
	y = topMargin ;
      if (y > mapGraphHeight - 1)
	y = mapGraphHeight - 1 ;         /* mhmp 08.10.98 le +2 */
      end = start + width ;
      if (end > seqEnd)
	end = seqEnd + 2 ;
      if (isProtein && !geneTranslation && start%3 != look->min%3)
	continue ;

      /* tframe: translation frame, needed to pick the correct codon */
      tframe = 0 ;
      if (isProtein && geneTranslation)
	{
	  /* ATTENTION i - look->min peut etre negatif! mhmp 12.06.98 */
	  /* mais on teste i > look->min ?!? que je remets ici */
	  for (i = start ; i < end ; i++)
	    if (i>look->min &&  geneTranslation[i-look->min] != '%'  &&
		geneTranslation[i-look->min] != 0  &&
		geneTranslation[i-look->min] != '-')
	      /*    if (i>look->min) */
	      break ;
	  if (i < end) /* success */
	    { tframe = (i - start + look->min) % 3 ; if (tframe < 0 ) tframe += 3 ;}
	  for (i = start ; i < end ; i++)
	    if (i >= look->min)
	      if (geneTransPos[i-look->min + tframe])
		{
		  graphText (messprintf("%7d",geneTransPos[i-look->min + tframe]), offset, y) ;
		  break ;
		}
	  offset +=8 ; /*mhmp 12.06.98*/
	}

      box = graphBoxStart () ;
      if (isProtein)
	{
	  s = 3 ;
	  if (start + tframe >= 0)	/* fixes nasty colors at ends */
	    graphColorSquares (arrp(look->colors, start + tframe, char),
			       offset, y, (end-start)/s, s, fMapTints) ;
	  else if (end > 0)
	    graphColorSquares (arrp(look->colors, tframe, char),
			       offset-start, y, end/s, s, fMapTints) ;
	  else		/* can happen because width shortened by 3 */
	    {
	      graphBoxEnd () ;
	      continue ;
	    }
	}
      else
	{  /* mieg, in dna case use seqEnd not end */
	  int end2 = end < seqEnd ? end : seqEnd ;
	  if (start + tframe >= 0)	/* fixes nasty colors at ends */
	    graphColorSquares (arrp(look->colors, start + tframe, char),
			       offset, y, (end2-start), 1, fMapTints) ;
	  else if (end2 > 0)
	    graphColorSquares (arrp(look->colors, tframe, char),
			       offset-start, y, end2, 1, fMapTints) ;
	  else		/* can happen because width shortened by 3 */
	    {
	      graphBoxEnd () ;
	      continue ;
	    }
	}

      cp = arrp(look->dna, start, char) ; 
      for (x = 0, i = start ; i < end ; ++x)
	{
	  if (isProtein)
	    { 
	      if (geneTranslation)
		*letter = i < seqEnd ? geneTranslation[i-look->min + tframe] : 0 ;
	      else
		*letter =  i < seqEnd ? e_codon(cp, translationTable) : 0 ;
	      cp += 3 ;
	      i += 3 ;
	    }
	  else
	    { 
	      if (i < seqEnd)  /* end may be seqEnd+2 for protein sake */
		*letter = 
		  (doComplementSurPlace ?
		   dnaDecodeChar[(int)complementBase[(int)(*cp & 15)]] : 
		   dnaDecodeChar[(int)(*cp & 15)]) ;
	      else
		*letter = 0 ;
	      ++cp ;
	      ++i ;
	    }

	  if (*letter && i > look->min) /* mhmp 16.06.98 apres i +=3 ?!? */
	    graphText (letter, offset+x, y) ;
	}

      if (skip > width && width > 1 && i < seqEnd)
	graphText (isProtein ? "." : "...", offset+x, y) ;

      graphBoxEnd () ;
      if (isProtein && geneTranslation) 
	offset -=8 ; /*mhmp 12.06.98*/
      graphBoxDraw (box, BLACK, TRANSPARENT) ;
      array(look->boxIndex, box, int) = arrayMax(look->segs) ;
      seg = arrayp(look->segs, arrayMax(look->segs), SEG) ;
      seg->key = isProtein ? _Peptide : _DNA ; 
      seg->type = isProtein ? 
	(geneTranslation ? 
	 (upTranslation ? TRANS_SEQ_UP : TRANS_SEQ) : PEP_SEQ) : DNA_SEQ ;
      seg->parent = 0 ;
      seg->x1 = start + tframe ; seg->x2 = end-1 + tframe ;
    }

  graphColor (BLACK) ;
  graphTextFormat (PLAIN_FORMAT) ;

  return ;
}
  
static int dnaWidth = 30 ;

void fMapSetDnaWidth (void)
{ 
  int i ;
  FeatureMap look = currentFeatureMap("fMapSetDnaWidth") ;
  ACEIN width_in;

  if (look->flag & FLAG_AUTO_WIDTH)
    messout("Warning, you need to turn off auto dna width "
	    "for this change to take effect.\n");

  if ((width_in = messPrompt ("Give new width: ", 
			      messprintf ("%d", dnaWidth),
			      "iz", 0)))
    { 
      aceInInt (width_in, &i);
      aceInDestroy (width_in);

      if (i >= 3)
	{ dnaWidth = i ; /* mieg: i think we should fix it here to i - i%3 ; */
	  fMapDraw (look, 0) ;
	}
    }
}

void fMapToggleAutoWidth (void)
{
  FeatureMap look = currentFeatureMap("fMapToggleAutoWidth") ;

  look->flag ^= FLAG_AUTO_WIDTH ;
  fMapDraw (look, 0) ;
}


/* column "DNA Sequence" */
/* This is of type MapColDrawFunc */
void fMapShowDNA (LOOK genericLook, float *offset, MENU *unused)
{ 
  FeatureMap look = (FeatureMap)genericLook;

  showSequence (look, *offset, dnaWidth, FALSE, NULL) ; 
  if (!(look->flag & FLAG_AUTO_WIDTH))
    graphButton ("Set DNA width", fMapSetDnaWidth, *offset, topMargin+0.1) ;
  *offset += look->dnaWidth + 3 ;
}


/* column "3 Frame Translation" */
/* This is of type MapColDrawFunc */
void fMapShow3FrameTranslation (LOOK genericLook, float *offset, MENU *unused)
{ 
  FeatureMap look = (FeatureMap)genericLook;

  fMapInitialise() ;
  
  dnaWidth -= (dnaWidth % 3) ; /* absolute necessity here */
  showSequence (look, *offset, dnaWidth, TRUE, NULL) ; 
  *offset += look->dnaWidth/3 + 1 ;
}


/* This is of type MapColDrawFunc */
void fMapShowCoords (LOOK genericLook, float *offset, MENU *unused)
{  
  FeatureMap look = (FeatureMap)genericLook;
  int width = dnaWidth ;
  int start, skip ;
  float y ;

  graphColor(BLACK) ;

  makeStartSkip (look, *offset, &width, &start, &skip) ;
  for (; start < look->max ; start += skip)
    {
      y = MAP2GRAPH(look->map, start) - 0.5 ;
      if (y < topMargin)
	y = topMargin ;
      if (y > mapGraphHeight - 1)
	y = mapGraphHeight - 1 ;
      graphText (messprintf("%7d", COORD(look, start)), *offset, y) ;
    }
  *offset += 8 ;

  return ;
}

/******************************************************************/
/*** to display the DNA as a yelllow rectangle with colored marks */


void fMapShowSummary (LOOK genericLook, float *offset, MENU *unused)
     /* This is of type MapColDrawFunc */
{
  FeatureMap look = (FeatureMap)genericLook;
  float oldoff ;
  int i, i1 ;
  float xmin, xmax, x1, x2 , foundSites = 0 ;
  unsigned char *cp, old, older ;
  BOOL stype1 = FALSE ;
  Array sites = look->sites ; /* mhmp */
  Site *s ;

  if (look->summaryBox)
    { graphBoxDim (look->summaryBox, offset, &x1, &x1, &x1) ;
      if (arrayExists(sites))
	{
	  s = arrp(sites, 0, Site) - 1 ; i = arrayMax(sites); 
	  while (s++, i--)
	    if (s->type & 1)
	      { stype1 = TRUE ;
		break ;
	      }
	}
      *offset -= 0.4 ;
      if (stype1)
	*offset += 0.6 ;
      graphBoxClear (look->summaryBox) ;
    }

  look->summaryBox = graphBoxStart () ;

  xmin = MAP2GRAPH(look->map, look->min) ;
  xmax = MAP2GRAPH(look->map, look->max) ;

  graphColor(YELLOW) ;
  graphFillRectangle (*offset+0.4, xmin, *offset+1.6, xmax) ;
 
  old = older = WHITE; 
  cp = arrp(look->colors, look->min, unsigned char) ;
  for (i = i1 = look->min ; i < look->max ; i++, cp++)
    { int BMNeeded, color;
      BMNeeded = *cp & ~(TINT_HIGHLIGHT1 | TINT_HIGHLIGHT2);
      /* Ignore highlighting */
      switch(BMNeeded^(BMNeeded-1)) /* trick to find first 1 bit */
	{ 
	default: color = WHITE; break;
	case 0x07: color = RED; break;
	case 0x0f: color = DARKGRAY; break;
	case 0x1f: color = MAGENTA; break;
	case 0x3f: color = CYAN; break;
	case 0x7f: color = LIGHTGREEN; break;
	case 0xff: color = YELLOW; break;
	}
      if (color != old) 
	{ if (old != WHITE)
	    { if (old != older)
		{ older = old ;
		  graphColor(old);
		}
	      x1 = MAP2GRAPH(look->map, i1) ;
	      x2 = MAP2GRAPH(look->map, i + 1)  ;
	      graphFillRectangle (*offset+0.5, x1, *offset+1.5, x2) ;
	    }
	  old = color;
	  i1 = i ;
	}
    }
  if (old != WHITE)
    { if (old != older) graphColor(old);
      x1 = MAP2GRAPH(look->map, i1) ;
      x2 = MAP2GRAPH(look->map, i + 1)  ;
      graphFillRectangle (*offset+0.5, x1, *offset+1.5, x2) ;
    }

  x1 = MAP2GRAPH(look->map, look->zoneMin-1) ;
  x2 = MAP2GRAPH(look->map, look->zoneMax+1) ;
  if (x1 > x2)			/* reverse to simplify maths */
    { float tmp = x2 ; x2 = x1 ; x1 = tmp ;
      tmp = xmax ; xmax = xmin ; xmin = tmp ;
    }
  if (x1 > xmin || x2 < xmax)
    { graphColor (BLUE) ;
      if (x2 < xmin || x1 > xmax)
	graphFillRectangle (*offset+0.4, xmin, *offset+0.9, xmax) ;
      else 
	{ if (x1 > xmin)
	    graphFillRectangle (*offset+0.4, xmin, *offset+0.9, x1) ;
	  if (x2 < xmax)
	    graphFillRectangle (*offset+0.4, x2, *offset+0.9, xmax) ;
	}
    }


/******************************************************************/
/*** to display arrows along the DNA yelllow rectangle */

  if (arrayExists(sites))
    {
      s = arrp(sites, 0, Site) - 1 ; i = arrayMax(sites); 
      while (s++, i--)
	{
	  if (s->i > look->min && s->i < look->max)
	    { cp = arrp(look->colors, s->i, unsigned char) ;
	    { int BMNeeded = *cp & ~(TINT_HIGHLIGHT1 | TINT_HIGHLIGHT2);
	    /* Ignore highlighting */
	    switch(BMNeeded^(BMNeeded-1)) /* trick to find first 1 bit */
	      { 
	      default: graphColor(WHITE); break;
	      case 0x07: graphColor(RED); break;
	      case 0x0f: graphColor(DARKGRAY); break;
	      case 0x1f: graphColor(MAGENTA); break;
	      case 0x3f: graphColor(CYAN); break;
	      case 0x7f: graphColor(LIGHTGREEN); break;
	      case 0xff: graphColor(YELLOW); break;
	      }
	    }
	    x1 = MAP2GRAPH(look->map, s->i) ;
	    x2 = s->i + 1 ;
	    while (x2 < look->max && *cp == *(cp+1))
	      { x2++ ; cp++ ;}
	    x2 = MAP2GRAPH(look->map, x2) ;
	    if (s->type & 1)
	      graphFillRectangle (*offset-0.2, x1, *offset+0.5, x2) ;
	    if (s->type & 2)
	      { graphFillRectangle (*offset+1.5, x1, *offset+2.2, x2) ;
	      foundSites = .5 ;
	      }
	    }
	}
    }
  /***************************************/
  graphColor(BLACK) ;
  graphRectangle (*offset+0.4, xmin, *offset+1.6, xmax) ;

  graphBoxEnd () ;
  graphBoxDraw (look->summaryBox, -1, -1) ; /* make sure to redraw */

  *offset += 2.4 + foundSites ; 
  oldoff = *offset ;
  graphBoxDim (look->summaryBox, offset,&x1,&x1,&x1) ;
  *offset = oldoff ;
}


/******************************************************************/
/*** to display arrows along the DNA yellow rectangle */

/* column  "Restriction map" */
void fMapShowCptSites (LOOK genericLook, float *offset, MENU *unused)
     /* This is of type MapColDrawFunc */
{
  FeatureMap look = (FeatureMap)genericLook;
  Site *s ;
  Array sites = look->sites ;
  int i, maxlen = 0 ;
  char *cp ;

  if(!arrayExists(sites) || ! arrayMax(sites))
    return ;

  s = arrp(sites, 0, Site) - 1 , i = arrayMax(sites); 
  while (s++, i--)
    if (s->i > look->min && s->i < look->max)
      { cp = stackText(look->siteNames, s->mark) ;
	while(*cp == ' ') cp++ ;
	graphText(messprintf("%6.9s",cp),
		  *offset- 1.6, MAP2GRAPH(look->map, s->i + 3 ) - .5) ;
	if (strlen(cp) > maxlen)
	  maxlen = strlen(cp) ;
      }
  *offset += maxlen > 5 ? 7 : 1 + maxlen ;

  return;
} /* fMapShowCptSites */


void fMapRegisterSites(FeatureMap look, Array sites, Stack sname)
{
  fMapInitialise() ;
  
  graphActivate (look->graph) ;
  if (sites != look->sites)
    arrayDestroy(look->sites) ;
   if (sname != look->siteNames)
     stackDestroy(look->siteNames) ;
  look->sites = sites ;
  look->siteNames = sname ;
}

/* RD 970917 - removed fMapShowFinished: it relied on SEQUENCE seg->data.k
   values that have not been filled in for years.  We should generalise/
   use Mike Holman's code, if we want this back (for now we use the gmap
   for this).
*/

/***************************************************************/

void fMapReDrawDNA (FeatureMap look)
{ 
  int i ;
  float dummy ;
  Graph old = graphActive() ;

  fMapInitialise() ;

  graphActivate (look->graph) ;

  for (i = look->minLiveBox ; i < arrayMax(look->boxIndex) ; ++i)
    if (BOXSEG(i)->type == DNA_SEQ)
      graphBoxDraw (i, -1, -1) ;

  if (look->summaryBox)
    fMapShowSummary ((LOOK)look, &dummy, NULL) ; /* typecast, because it's a
					      MapColDrawFunc */

  graphActivate (old) ;
}

/***************************************************************/
/**************** code to zip together DNA sequences ***********/
#ifndef USE_SMAP
/* Find the length of the dna sequence associated with an object, if there   */
/* is no sequence then 0 is returned.                                        */
/*                                                                           */
int sequenceLength (KEY seq)
{
  OBJ obj = NULL ;
  KEY dnaKey = KEY_UNDEFINED ;
  Array dna = NULL ;
  int i, length = 0 ;
  static Array aa = NULL ;

  if ((obj = bsCreate(seq)))
    {
      if (bsGetKey (obj, _DNA, &dnaKey) && 
	  (bsGetData (obj, _bsRight, _Int, &length) || 
	   (dna = dnaGet (dnaKey))))
	{
	  if (dna)
	    {
	      length = arrayMax(dna) ;
	      arrayDestroy (dna) ;
	    }
	}
      else
	{
	  aa = arrayReCreate (aa, 12, BSunit) ;
#ifdef ACEDB1
	  if (bsFindTag(obj,_Contains) && bsFlatten(obj, 4, aa))
	    for (i=0; i < arrayMax(aa); i+= 4)
	      { if (arr(aa,i+2,BSunit).i > length)
		length = arr(aa,i+2,BSunit).i ;
	      if (arr(aa,i+3,BSunit).i > length)
		length = arr(aa,i+3,BSunit).i ;
	      }
#else
	  if (bsGetArray (obj,str2tag("S_Child"), aa, 4))
	    for (i=0 ; i < arrayMax(aa) ; i += 4)
	      { if (arr(aa,i+2,BSunit).i > length)
		length = arr(aa,i+2,BSunit).i ;
	      if (arr(aa,i+3,BSunit).i > length)
		length = arr(aa,i+3,BSunit).i ;
	      }
	  else if (bsFindTag(obj,_Subsequence) && bsFlatten(obj, 3, aa))
	    for (i=0 ; i < arrayMax(aa) ; i += 3)
	      { if (arr(aa,i+1,BSunit).i > length)
		length = arr(aa,i+1,BSunit).i ;
	      if (arr(aa,i+2,BSunit).i > length)
		length = arr(aa,i+2,BSunit).i ;
	      }
#endif
	}
  
      bsDestroy (obj) ;
    }

  return length ;
}

/* Gets DNA for a specified sequence object for a given start/stop range.    */
/*                                                                           */
/* This routine _WILL_ be called with start/stop coords that are _outside_   */
/* the sequence length of the sequence object. This is because one of the    */
/* ways this routine is called is from an fmap callback and the user may     */
/* have scrolled partly off the end of the object, so for instance you       */
/* might get a -ve start value. (I expect there are other ways this happens) */
/*                                                                           */
/* The routine seems to be called with stop set to 0 which seems to mean     */
/* "get the whole thing".                                                    */
/*                                                                           */
/* I have rewritten the code that attempts to clamp the values of start/stop */
/* to something reasonable so that it will be more robust.                   */
/*                                                                           */
/* It would seem a good idea to return an error if the user asks for coords  */
/* that are way off the end but this seems to produce errors in other parts  */
/* of the code.                                                              */
/*                                                                           */
Array fMapFindDNA(KEY seq, int *start, int *stop, BOOL findMismatches)
{
  int length ;
  int seqLen = 0 ;
  Array assembly ;
  int my_start, my_stop ;


  /* aaaaaaaaghhhhhhhhh, surely this is not needed......                     */
  fMapInitialise() ;

  /* Try and find out the sequence length for this object.                   */
  seqLen = sequenceLength(seq) ;

  if (seqLen == 0)
    {
      /* I think this is correct behaviour, but its a little hard to tell as */
      /* the code has changed many times and its not clear what is meant to  */
      /* be right.                                                           */

      messerror("Cannot determine DNA length to retrieve from "
		"object %s which has a sequence length of %d.",
		name(seq), seqLen) ;
      return NULL ;
    }


  /* stop = 0 means "do the whole thing".                                    */
  if (*stop == 0)
    *stop = seqLen ;


  /* Warn user that coords are outside of sequence but then clamp them to    */
  /* lie within DNA.                                                         */
  if ((*start < 1 && *stop < 1)
      || (*start > seqLen && *stop > seqLen))
    {
      messerror("Requested start/stop coordinates (%d, %d) for sequence object %s"
		" lie outside of objects DNA sequence (length %d), they will be"
		" adjusted to lie within this length.",
		*start, *stop, name(seq), seqLen) ;
    }


  /* OK, THERE IS A LURKING PROBLEM HERE OF WHAT HAPPENS WHEN THE SEQUENCE   */
  /* LENGTH BECOMES GREATER THAN AN INT CAN HOLD.....I.E. ABOUT 2GB          */
  /* I have fudged this below by checking for overflow by looking for some   */
  /* unexpected numbers after addition/subtraction, but it's only a stopgap. */

  /* What code does is to "munge" coordinates to a) clamp them to lie within */
  /* the sequence length and b) make sure the coordinates are a decent       */
  /* distance apart.                                                         */

  /* Coords may be either way round.                                         */
  if (*start < *stop)
    {
      my_start = *start ;
      my_stop = *stop ;
    }
  else
    {
      my_start = *stop ;
      my_stop = *start ;
    }

  length = my_stop - my_start ;
  if (length < 10000)
    length = 10000 ;
  
  /* Some of the conditions look a little wierd but we may run in to a       */
  /* problem with ints overflowing here...really this should be explicitly   */
  /* sorted at the parsing in of ace files stage.                            */
  my_start -= length ; 
  if (my_start < 1 || my_start > seqLen)
    my_start = 1 ;
  
  my_stop += length ; 
  if (my_stop > seqLen || my_stop < my_start)
    my_stop = seqLen ;

  assembly = arrayCreate((my_stop - my_start + 2), char) ;
  array(assembly, (my_stop - my_start), char) = 0 ;

  /* Swop back coords.                                                       */
  if (*start < *stop)
    {
      *start = my_start ;
      *stop = my_stop ;
    }
  else
    {
      *start = my_stop ;
      *stop = my_start ;
    }

  /* So this is all bizarre really, this call can fail (returns 1), but we   */
  /* just carry blithely on to produce an fmap. Somewhere else in the code   */
  /* we detect that no dna was found and if the user asks to see the dna     */
  /* we just print dashes...I think producing the fmap is OK, but I would    */
  /* thought that we should have recorded in some state somewhere that there */
  /* was no dna....                                                          */
  /* allow mismatch (dna_result == 0),  */
  if ((dnaDoAdd(assembly, seq, (*start - 1), (*stop - 1), FALSE, findMismatches)
       == -1))
    {
      arrayDestroy(assembly) ;
      assembly = NULL ;
    }

  return assembly ;
}  





/* TEST CODE TO TRY EXPORTING GFF WITHOUT DOING THE DNA ARRAY....            */
int fMapGetDNALength(KEY seq, int *start, int *stop, BOOL findMismatches)
{
  int dna_length ;
  int length ;
#ifdef USE_SMAP
  int seqLen = sMapLength(seq) ;
#else
  int seqLen = sequenceLength(seq) ;
#endif
  if (!*stop)
    *stop = seqLen ;
  if (!*stop)
    return 0 ;

  if (*start < *stop)
    {
      length = *stop - *start ;
      if (length < 10000)
	length = 10000 ;

      *start -= length ; 
      if (*start < 1)
	*start = 1 ;
      *stop += length ; 
      if (*stop > seqLen) 
	*stop = seqLen ;

      if (*stop-*start <=0)
	return 0 ; /* mieg a protection */

      /* HUGE POTENTIAL FOR OFF BY ONE HERE....NEED TO CHECK HOW THIS IS     */
      /* USED....this code is confusing, seems to imply that the array ends  */
      /* being one longer than needed and then the char before the last one  */
      /* is set to zero...duhh....                                           */
#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
      assembly = arrayCreate (*stop-*start+2, char) ;
      array(assembly,*stop-*start,char) = 0 ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

      /* This needs to be only  "+ 1" to match later code....                */
      dna_length = *stop - *start + 1 ;
    }
  else
    {
      length = *start - *stop ;
      if (length < 10000)
	length = 10000 ;

      *stop -= length ; 
      if (*stop < 1)
	*stop = 1 ;
      *start += length ; 
      if (*start > seqLen)
	*start = seqLen ;

      if (*start-*stop <=0)
	return 0 ; /* mieg a protection */

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
      assembly = arrayCreate (*start-*stop+2, char) ;
      array(assembly,*start-*stop,char) = 0 ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

      dna_length = *start - *stop + 1 ;
    }
    
  return dna_length ;
}  

#endif /* !USE_SMAP */
/********************************************/

void fMapClearDNA (FeatureMap look) /* clear colors array to _WHITE */
{ 
  register int i ;
  char *cp ;
  
  fMapInitialise() ;
  
  if (look->colors)
    { cp = arrp(look->colors, 0, char) ;
      for (i = arrayMax(look->colors) ; i-- ;)
	*cp++ = WHITE ;
    }
}

void fMapClear (void)
{ 
  FeatureMap look = currentFeatureMap("fMapClear");
  
  fMapClearDNA (look) ;
  look->chosen = assReCreate(look->chosen) ;
  look->antiChosen = assReCreate(look->antiChosen) ;
  arrayDestroy(look->sites) ;
  stackDestroy(look->siteNames) ;
  fMapDraw (look, 0) ;
}


/*********************************************************/
/********************* translate genes *******************/

BOOL fMapGetCDS(FeatureMap look, KEY parent, Array *cds, Array *index)
{
  BOOL result ;

  result = fMapGetDNA(look, parent, cds, index, TRUE) ;

  return result ;
}

/* Worth noting that this routine does _not_ query the database but instead  */
/* uses whatever is stored in the segs, corrolary of this is that user must  */
/* do an fmap recalculate to see effects of any DB changes they make.        */
BOOL fMapGetDNA(FeatureMap look, KEY parent, Array *cds, Array *index, BOOL cds_only)
{ 
  int   i, j=0, jCode = 0, cds1, cds2, tmp, dnaMax ;
  SEG   *seg = NULL ;
  BOOL isUp = FALSE ;
  Array dna ;
  BOOL first_exon ;

	/* RD 020303 remove unnecessary check class(parent) == _VSequence */

  if (!iskey(parent) || !look->dna) 
    return FALSE ;

  dna = look->dna ;
  dnaMax = arrayMax(look->dna) ;

  cds1 = cds2 = 0 ;					    /* used to check if CDS found. */
  for (i = 1 ; i < arrayMax(look->segs) ; ++i)
    {
      seg = arrp(look->segs,i,SEG) ;
      if (seg->parent == parent)
	{
	  switch (seg->type)
	    {
	    case CDS: case CDS_UP:
	      if (cds_only)
		{
		  cds1 = seg->x1 ;			    /* N.B. these are coords on displayed */
		  cds2 = seg->x2 ;			    /* DNA. */
		}
	      break ;
	    case SEQUENCE:
	      isUp = FALSE ;
	      break ;
	    case SEQUENCE_UP:
	      isUp = TRUE ;
	      break ;
	    default: break ;
	    }

	}
    }

  /* Couldn't find the required CDS so no translation done.                  */
  if (cds1 == cds2 && cds_only)
    return FALSE ;

  if (cds)
    *cds = arrayCreate (1000, char) ;			    /* wild guess */
  if (index)
    *index = arrayCreate (1000, int) ;

  for (i = 1, first_exon = TRUE ; i < arrayMax(look->segs) ; ++i)
    {
      seg = arrp(look->segs,i,SEG) ;
      if (seg->parent == parent)
	switch (seg->type)
	  {			/* pick cds */
	  case EXON: case EXON_UP:
	    {
	      if (first_exon)
		{
		  /* May have to modify beginning of dna for translation because obj may */
		  /* have begun in a previous exon so we don't have the beginning, the   */
		  /* Start_not_found allows us to correct the reading frame by setting   */
		  /* a new start position. Default is start of obj (start_not_found = 1).*/
		  int new_start ;

		  first_exon = FALSE ;
		  new_start = arrp(look->seqInfo, seg->data.i, SEQINFO)->start_not_found ;
		  if (new_start != 0)
		    new_start-- ;
		  cds1 += new_start ;
		  j = seg->x1 + new_start ;
		}
	      else
		j = seg->x1 ;

	      for( ; j <= seg->x2 ; ++j)
		{
		  if (cds_only && (j < cds1 || j > cds2))
		    continue ;
		    
		  if (cds)
		    {
		      if (j >= 0 && j < dnaMax)
			array(*cds,jCode,char) = arr(dna,j,char) ;
		      else
			array(*cds,jCode,char) = 0 ;
		    }

		  if (index)
		    array(*index,jCode,int) = j ;

		  ++jCode ;
		}
	    }
	    break ;
	  default: break ;
	  }
    }

  if (isUp)
    {
      if (cds)
	reverseComplement(*cds) ;
      if (index)
	for (j = 0, i = jCode-1 ; j < i ; ++j, --i)
	  {
	    tmp = arr(*index,j,int) ;
	    arr(*index,j,int) = arr(*index,i,int) ;
	    arr(*index,i,int) = tmp ;
	  }
    }

  return TRUE ;
}


/* Export peptide translations of all objects that have a CDS within an fmap. */
void fMapExportTranslations (void)
{
  static char fname[FIL_BUFFER_SIZE]="", dname[DIR_BUFFER_SIZE]="" ;
  ACEOUT pep_out;

  if ((pep_out = aceOutCreateToChooser ("File to add translation to",
					dname, fname, "pep", "a", 0)))
    {
      FeatureMap look = currentFeatureMap("fMapExportTranslations") ;
      SEG *seg ;
      KEY seq ;
      Array cds = NULL ;
      int j ;

      for (j = 1 ; j < arrayMax(look->segs) ; ++j)
	{
	  seg = arrp(look->segs, j, SEG) ;
	  
	  if ((seg->type == CDS || seg->type == CDS_UP) &&
	      seg->x1 >= look->zoneMin && seg->x2 <= look->zoneMax)
	    {
	      seq = seg->parent ;

	      if (cds)
		{
		  arrayDestroy (cds) ;
		  cds = NULL ;
		}

	      if (!fMapGetCDS(look, seq, &cds, 0))
		{
		  messout("screwup finding CDS %s", name(seq)) ;
		  continue ;
		}
	      
	      if (!pepTranslateAndOutput(pep_out, seq, cds))
		{
		  messout("Could no translate %s", name(seq)) ;
		  continue ;
		}
	    }
	}

      aceOutDestroy(pep_out) ;
    }

  return ;
}


/*********************/

/* column "Up Gene Translation" */
/* This is of type MapColDrawFunc */
void fMapShowUpGeneTranslation (LOOK genericLook, float *offset, MENU *unused)
{
  showGeneTranslation(genericLook, offset,
		      "Up Gene Translation",
		      "Select an up gene to translate with the pulldown menu on the genes") ;

  return;
} /* fMapShowUpGeneTranslation */


/* column "Down Gene Translation" */
/* This is of type MapColDrawFunc */
void fMapShowDownGeneTranslation (LOOK genericLook, float *offset, MENU *unused)
{
  showGeneTranslation(genericLook, offset,
		      "Down Gene Translation",
		      "Select a down gene to translate with the pulldown menu on the genes") ;

  return;
} /* fMapShowDownGeneTranslation */


/* The actual translation routine.                                           */
static void showGeneTranslation(LOOK genericLook, float *offset,
				char *map_col_name, char *err_msg)
{ 
  FeatureMap look = (FeatureMap)genericLook;
  int i, j, window, min, max ;
  char c ;
  Array cds, index ;
  int k = 1;
  BOOL cds_only = FALSE ;
  SEG *seg ;
  int pep_index ;
  Array pep = NULL ;

  
  /* I expect there is a way to go from  look to seg->parent more cheaply
   * but I can't find it at the moment... */
  for (i = 1 ; i < arrayMax(look->segs) ; i++)
    {
      seg = arrp(look->segs,i,SEG) ;
      if (seg->parent == look->translateGene)
	{
	  cds_only = arrp(look->seqInfo, seg->data.i, SEQINFO)->cds_info.cds_only ;
	  break ;
	}
    }

  /* If there isn't any dna for an object then just turn the column off,     */
  /* otherwise display the dna.                                              */
  if (!(fMapGetDNA(look, look->translateGene, &cds, &index, cds_only)))
    {
      messout(err_msg) ;
      mapColSetByName(look->map, map_col_name, FALSE) ; 
      look->pleaseRecompute = TRUE ;			    /* no dna so make sure we turn column
							       off in fmap. */
    }
  else if ((pep = peptideDNATrans(look->translateGene, cds, TRUE)))
    {
      window = look->max - look->min ;
      geneTranslation = (char*) messalloc (window) ;
      geneTransPos = (int*) messalloc (window * sizeof(int)) ;
      min = arr(index,0,int) - look->min ; 
      max = arr(index,arrayMax(index)-1,int) - look->min ;
      upTranslation = FALSE ;
      if (min > max)
	{
	  upTranslation = TRUE ;
	  i = min ;
	  min = max ;
	  max = i ;
	}

      if (min < 0)
	min = 0 ;

      if (max > window)
	max = window ;

      for (j = min ; j < max ; ++j)
	{
	  geneTranslation[j] = '-' ;
	  geneTransPos[j] = 0 ;
	}

      for (i = 0, pep_index = 0 ; i < arrayMax(index) ; i+=3, k++, pep_index++)
	{
	  c = array(pep, pep_index, char) ;

	  j = arr(index,i,int) - look->min ;

	  if (j >= 0 && j+2 < window)
	    { 
	      geneTranslation[j] = c ;
	      geneTranslation[j+1] = '%' ; /* base is "out of frame" letter value */
	      geneTranslation[j+2] = '%' ;
	      geneTransPos[j] = k ;
	      geneTransPos[j+1] = k ;
	      geneTransPos[j+2] = k ;
	    }
	}

      showSequence(look, *offset, dnaWidth, TRUE, NULL) ; 

      messfree(geneTranslation) ;
      messfree(geneTransPos) ;
      arrayDestroy(cds) ;
      arrayDestroy(index) ;
      arrayDestroy(pep) ;
      *offset += look->dnaWidth/3 + 1 + 8 ;
    }

  return ;
} /* fMapShowGeneTranslation */

/********************/

void fMapShowCoding (LOOK genericLook, float *offset, MENU *unused) /* width 0.5 */
     /* This is of type MapColDrawFunc */
{
  FeatureMap look = (FeatureMap)genericLook;
  float y1, y2 ;
  int i ;
  SEG *seg ;
  int frame = look->min % 3 ;

  if (!look->map->isFrame) 
    return ;

  graphColor (BLUE) ;

  for (i = 1 ; i < arrayMax(look->segs) ; ++i)
    { seg = arrp(look->segs,i,SEG) ;
      if (seg->type != CODING ||
	  (seg->x1 - seg->data.i)%3 != frame ||
	  seg->x1 > look->max || 
	  seg->x2 < look->min)
	continue ;
      y1 = MAP2GRAPH(look->map,seg->x1) ;
      y2 = MAP2GRAPH(look->map,seg->x2+1) ;	/* to cover full base */
      if (y1 < 2 + topMargin) y1 = 2 + topMargin ;
      if (y2 < 2 + topMargin) y2 = 2 + topMargin ;
      
      array (look->boxIndex, graphBoxStart(), int) = i ;
      graphRectangle (*offset, y1, *offset+0.4, y2) ;
      graphBoxEnd () ;
    }

  graphColor (BLACK) ;
  *offset += 0.5 ;
}

/************************************************************/

void fMapShowORF (LOOK genericLook, float *offset, MENU *unused) /* width 2 */
     /* This is of type MapColDrawFunc */
{
  FeatureMap look = (FeatureMap)genericLook;
  char *cp ;
  float y1 = 0, y2 ;
  int i1, i, box ;
  int frame ;
  Array translationTable ;
  SEG *seg ;

  fMapInitialise() ;

  if (!(translationTable = pepGetTranslationTable (look->seqOrig, 0)))
    return ;


  frame = look->min % 3 ;

  cp = arrp (look->dna, look->min, char) ;
  for (i1 = look->min ; i1 >= 0 ; i1-=3, cp-=3)
    if (e_codon(cp, translationTable) == '*')
      break ;
  i1 += 3 ;
  y1 = MAP2GRAPH(look->map,look->min) ;

  cp = arrp (look->dna, look->min+3, char) ;
  for (i = look->min+3 ; i1 < look->max ||
       			 i < arrayMax(look->dna)-3 ; i+=3, cp+=3)
    if (i >= arrayMax(look->dna)-3 || e_codon(cp, translationTable) == '*')
      {
	y2 = MAP2GRAPH(look->map, i) ;
	if (i > i1)
	  { box = graphBoxStart() ;
	    if (i1 > look->min)
	      graphLine (*offset+0.25, y1, *offset+1.75, y1) ;
	    else
	      { graphLine (*offset+0.25, y1, *offset+0.5, y1) ;
		graphLine (*offset+1.5, y1, *offset+1.75, y1) ;
	      }
	    if (i < look->max)
	      graphLine (*offset+0.25, y2, *offset+1.75, y2) ;
	    else
	      { y2 = MAP2GRAPH(look->map, look->max) ;
		graphLine (*offset+0.25, y2, *offset+0.5, y2) ;
		graphLine (*offset+1.5, y2, *offset+1.75, y2) ;
	      }
	    graphBoxEnd () ;
	    array(look->boxIndex, box, int) = arrayMax(look->segs) ;

	    seg = arrayp(look->segs, arrayMax(look->segs), SEG) ;
	    seg->key = _ORF ;
	    seg->type = ORF ;
	    seg->x1 = i1 ; seg->x2 = i-1 ;
	    seg->data.i = frame ;
	    seg->parent = arr(look->boxIndex, box, int) ;

	    if (assFind (look->chosen, SEG_HASH(seg), 0))
	      graphBoxDraw (box, BLACK, GREEN) ;
	    else if (assFind (look->antiChosen, SEG_HASH(seg), 0))
	      graphBoxDraw (box, BLACK, LIGHTGREEN) ;
	    else
	      graphBoxDraw (box, BLACK, WHITE) ;
	    graphBoxFreeMenu (box, fMapChooseMenuFunc, 
			      fMapChooseMenu) ;
	  }
	else
	  graphLine (*offset+0.25, y2, *offset+1.75, y2) ;
	y1 = y2 ;
	i1 = i+3 ;
	if (i1 > look->max)
	  break ;
      }
  *offset += 2 ;
}

/******************************************/


void fMapShowATG (LOOK genericLook, float *offset, MENU *unused) /* width 1 */
     /* This is of type MapColDrawFunc */
{
  FeatureMap look = (FeatureMap)genericLook;
  char *cp ;
  int i, box, iseg ;
  int frame;
  int atg, atgMax ;
  float y, width, height ;
  SEG *seg, *atgSeg ;
  METHOD *atg_meth;
  Array translationTable ;

  if (!(translationTable = pepGetTranslationTable (look->seqOrig, 0)))
    return ;

  /* init - method ATG in the methodCache */
  atg_meth = methodCacheGetByName(look->mcache, "ATG",
				  /* defaults in ace-format */
				  "Colour   YELLOW\n"
				  "Frame_sensitive\n"
				  "Strand_sensitive\n"
				  "Right_priority  4.195\n"
				  /* no diff used so far */
				  ,"");

  frame = look->min % 3 ;
  *offset += 0.5 ;		/* centre line */
 
  fMapFindSegBounds (look, ATG, &atg, &atgMax) ;

  cp = arrp (look->dna, look->min, char) ;
  for (i = look->min ; i < look->max &&
		       i < arrayMax(look->dna)-3 ; i+=3, cp+=3)
    { 
      while (atg < atgMax && 
	     (atgSeg = arrp(look->segs, atg, SEG))->x1 < i)
	++atg;
      if (atg < atgMax && atgSeg->x1 == i)
	{ iseg = atg ;
	  seg = atgSeg ;
	}
      else if (e_codon(cp, translationTable) == 'M')
	{ iseg = arrayMax(look->segs) ;
	  seg = arrayp(look->segs, arrayMax(look->segs), SEG) ;
	  seg->key = atg_meth->key;
	  seg->parent = 0 ;
	  seg->type = ATG ;
	  seg->x1 = i ; seg->x2 = i+2 ;
	  seg->data.f = 0.0 ;
	}
      else 
	continue;

      width = 0.5 * fMapSquash (seg->data.f, 1.0) ;
      box = graphBoxStart();
      height = look->map->mag * 3 ;
      if (height < 0.3) 
	height = 0.3; ;
      if (width < 0.2) 
	width = 0.2 ;
      y = MAP2GRAPH(look->map, i) ;
      graphRectangle (*offset-width, y, 
		      *offset+width, y+height) ;
      graphBoxEnd () ;
      if (assFind (look->chosen, SEG_HASH(seg), 0))
	graphBoxDraw (box, BLACK, GREEN) ;
      else if (assFind (look->antiChosen, SEG_HASH(seg), 0))
	graphBoxDraw (box, BLACK, LIGHTGREEN) ;
      else
	graphBoxDraw (box, BLACK, YELLOW) ;
      graphBoxFreeMenu (box, fMapChooseMenuFunc, fMapChooseMenu) ;
      array(look->boxIndex, box, int) = iseg ;
  
    }
  *offset += 0.5 ;
}

/************************************************/
/************************************************/
