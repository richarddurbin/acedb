/*  File: graphps.c
 *  Author: Richard Durbin (rd@mrc-lmb.cam.ac.uk)
 *  Copyright (C) J Thierry-Mieg and R Durbin, 1992
 * -------------------------------------------------------------------
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
 * 	Richard Durbin (MRC LMB, UK) rd@mrc-lmb.cam.ac.uk, and
 *	Jean Thierry-Mieg (CRBM du CNRS, France) mieg@kaa.cnrs-mop.fr
 *
 * Description: to provide postscript output for the graph package
 * Exported functions: graphPS(), graphPrint()
 * SCCS: $Id: graphps.c,v 1.64 2003-12-12 14:14:08 edgrif Exp $
 * HISTORY:
 * Last edited: Dec 12 11:35 2003 (edgrif)
 * * May 11 10:42 1999 (edgrif): Corrected my own bug in PS-Adobe header.
 * * Apr 29 16:05 1999 (edgrif): Added code for new graphdev layer,
 *              see Graph_Internals.html for details.
 * * Jan 21 14:34 1999 (edgrif): Added EPSF parameter to graphPS &
 *              graphPSDefaults + code to make non-EPSF files.
 * * Jan  5 14:11 1999 (edgrif): Make graphPStitle() internal as PStitle,
 *              it's not used anywhere. Correct code for checking string
 *              pointers in several places (i.e. NULL ptr check AND null
 *              string check required).
 * * Sep 25 16:52 1998 (edgrif): Replace #ifdef ACEDB with graph/acedb,
 *              two references to session and one to acedb PS fonts file.
 * * Oct 1997 (michel): fixed the date printing
 * * Apr 10 (mieg): set for color laser printer (add COLOUR in psfont.wrm)
 * * Aug 14 18:05 1992 (rd): COLOR_SQUARES
 * * Jul 25 12:28 1992 (mieg): TEXT_PTR_PTR
 * Created: Wed May 20 08:30:44 1992 (rd)
 * CVS info:   $Id: graphps.c,v 1.64 2003-12-12 14:14:08 edgrif Exp $
 *-------------------------------------------------------------------
 */

#include <wh/regular.h>
#include <wh/menu_.h>
#include <w2/graphdev.h>
#include <w2/graph_.h>
#include <wh/aceio.h>		
#include <time.h>
#include <w2/graphcolour.h>

typedef struct
{
  ACEOUT fo;
  Graph_ graph;
  BOOL isColor;
  int currColor;
  float currLineWidth;
  GraphLineStyle currLineStyle ;
  float currTextHeight;
  int currTextFormat;
} psContext;




static int psSetColor(psContext *ps, int col)
{
  float r, g, b, gr;

  if (ps->currColor == col)
    return  ESUCCESS;

  ps->currColor = col;

  graphGetColourAsFloat(col, &r, &g, &b, &gr);

  if (ps->isColor)
    return aceOutPrint(ps->fo, "%.2f %.2f %.2f setrgbcolor\n", r, g, b);
  else
    return aceOutPrint(ps->fo, "%.2f setgray\n", gr) ;
}

static int psSetLineWidth(psContext *ps, float width)
{
  if (width == ps->currLineWidth)
    return ESUCCESS;
  
  ps->currLineWidth = width;
  return aceOutPrint(ps->fo, "%f setlinewidth\n", width) ;
}

/* Set the line to be dashed, note that the X default is a dashed
 * pattern of 4 on, 4 off, I am not sure how this scales to th
 * poscript output, I have put "[1] for now.... */
static int psSetLineStyle(psContext *ps, GraphLineStyle style)
{
  if (style == ps->currLineStyle)
    return ESUCCESS;
  
  ps->currLineStyle = style;
  if (style == GRAPH_LINE_SOLID)
    return aceOutPrint(ps->fo, "[] 0 setdash\n") ;
  else
    return aceOutPrint(ps->fo, "[1] 0 setdash\n") ;
}


/* This needs re-doing TODO */
static Stack psGetFontNames(void)
{ 
  int nFonts = 0 ;
  FILE *fil;
  Stack fontNameStack = stackCreate(50) ;
    
  /* ACEDB-GRAPH INTERFACE: Get acedb ps fonts if any registered.          */
  if (getGraphAcedbGetFonts() != NULL)
    (getGraphAcedbGetFonts())(&fil, fontNameStack, &nFonts) ;
  
  
  if (nFonts < 5)
    { 
      stackClear (fontNameStack) ;
      pushText (fontNameStack,"Helvetica") ;           /* normal */
      pushText (fontNameStack,"Helvetica-Oblique") ;   /* italic */
      pushText (fontNameStack,"Helvetica-Bold") ;      /* bold   */
      pushText (fontNameStack,"Courier") ;             /* fixed  */
      pushText (fontNameStack,"Times-Roman") ;         /* title  */
    }

  return fontNameStack;
}


/*****************************************/

static int psSetFont(psContext *ps, int format, float height)
{
  int err;

  if ((format == ps->currTextFormat) && (height == ps->currTextHeight))
    return ESUCCESS;

  ps->currTextFormat = format;
  ps->currTextHeight = height;
  
  switch (format)
    {
    case ITALIC:
      if (height)
	err = aceOutPrint(ps->fo, "italicFont %.4f scalefont setfont\n", 
			  height) ;
      else
	err = aceOutPrint(ps->fo, "italicFont setfont\n") ;
      break ;
    case BOLD:
      if (height)
	err = aceOutPrint(ps->fo, "boldFont %.4f scalefont setfont\n", 
			  height) ;
      else
	err = aceOutPrint(ps->fo, "boldFont setfont\n") ;
      break ;
    case FIXED_WIDTH:
      if (height)
	err = aceOutPrint(ps->fo, "fixedFont %.4f scalefont setfont\n", 
			  height) ;
      else
	err = aceOutPrint(ps->fo, "fixedFont setfont\n") ;
      break ;
    case PLAIN_FORMAT:
    default:
      if (height)
	err = aceOutPrint(ps->fo, "defaultFont %.4f scalefont setfont\n", 
			  height) ;
      else
	err = aceOutPrint(ps->fo, "defaultFont setfont\n") ;
      break ;
    }

  return err;
}

/***********************************************/

static int psDrawBox (psContext *ps, Box box)
{
  float  x1,x2,y1,y2 ;
  int    action, err;
  float pointSize = box->pointsize;
  float textHeight = box->textheight;
  int textFormat = box->format;
  float lineWidth =  box->linewidth;
  GraphLineStyle lineStyle =  box->line_style ;
  int colour = box->fcol;

  if ((box->bcol != TRANSPARENT) &&
      (box->bcol != WHITE) && 
      (box->x1 <=  999999.0)) 
    { 
      err =  psSetColor(ps, box->bcol);
      if (err != ESUCCESS)
	return err;
      err = aceOutPrint(ps->fo, "%.4f %.4f %.4f %.4f rectangle fill\n",
			box->x1,box->y1,box->x2,box->y2) ;
      if (err != ESUCCESS)
	return err;
    }

  stackCursor (ps->graph->stack, box->mark) ;
  
  while (!stackAtEnd (ps->graph->stack))
    switch (action = stackNext (ps->graph->stack, int))
      {
      case BOX_END:
        return ESUCCESS;                        /* exit point */
      case BOX_START:
	err = psDrawBox (ps, arrp (ps->graph->boxes, 
				   stackNext(ps->graph->stack, int),
				   BoxStruct));
	if (err != ESUCCESS)
	  return err;
	break;
      case COLOR:
        colour = stackNext (ps->graph->stack,int) ;
	break;
      case TEXT_FORMAT:
        textFormat = stackNext (ps->graph->stack,int) ;
	break;
      case LINE_WIDTH:
	lineWidth = stackNext (ps->graph->stack,float);
	break ;
      case LINE_STYLE:
	lineStyle = stackNext (ps->graph->stack, int) ;
	break ;
      case TEXT_HEIGHT:
	textHeight = stackNext (ps->graph->stack,float) ;
	break ;
      case POINT_SIZE:
        pointSize = stackNext (ps->graph->stack,float) ;
        break ;
      case LINE: case RECTANGLE: case FILL_RECTANGLE:
        x1 = stackNext (ps->graph->stack,float) ;
        y1 = stackNext (ps->graph->stack,float) ;
        x2 = stackNext (ps->graph->stack,float) ;
        y2 = stackNext (ps->graph->stack,float) ;
        err =  psSetColor(ps, colour);
	if (err != ESUCCESS)
	  return err;
	err = psSetLineWidth(ps, lineWidth);
	if (err != ESUCCESS)
	  return err;
	err = psSetLineStyle(ps, lineStyle);
	if (err != ESUCCESS)
	  return err;
	switch (action)
          {
          case LINE:
            err = 
	      aceOutPrint(ps->fo,
			  "newpath  %.4f %.4f moveto  %.4f %.4f lineto  stroke\n",
			  x1,y1,x2,y2) ;
            break ;
          case RECTANGLE:
            err = 
	      aceOutPrint(ps->fo,"%.4f %.4f %.4f %.4f rectangle stroke\n",
			  x1,y1,x2,y2) ;
            break ;
          case FILL_RECTANGLE:
            err = 
	      aceOutPrint(ps->fo,"%.4f %.4f %.4f %.4f rectangle fill\n",
			  x1,y1,x2,y2) ;
            break ;
          }
	if (err != ESUCCESS)
	  return err;
	break ;
      case PIXELS: case PIXELS_RAW:
	{ unsigned char *ip, *pixels; 
	  int w, h, line, i, j;
#ifndef ACEDB_GTK
	  int reverse[256];
	  unsigned char hexVal[256]; 
	  float fac ;
#else
	  unsigned char *hexVal = graphGreyRampGetMap();
#endif
	  x1 = stackNext (ps->graph->stack, float) ;
	  y1 = stackNext (ps->graph->stack, float) ;
	  pixels = stackNext (ps->graph->stack, unsigned char*) ;
	  w = stackNext (ps->graph->stack, int) ;
	  h = stackNext (ps->graph->stack, int) ;
	  line = stackNext (ps->graph->stack, int) ;
	  if (action == PIXELS)
	    { 
	      /* PIXELS are not implemented yet */
	      x2 = stackNext (ps->graph->stack, float) ;
	      y2 = stackNext (ps->graph->stack, float) ;
	    }
	  else
	    { x2 = x1 + w/ps->graph->xFac ;
	      y2 = y1 + h/ps->graph->yFac ;
#ifndef ACEDB_GTK
	      graphRawMaps (0, reverse) ;
	      if (rampMin < rampMax)
		fac = 0xff / (float) (rampMax - rampMin) ;
	      else
		fac = 0xff / (float) (rampMin - rampMax) ;
	      
	      for (i = 0 ; i < 256 ; ++i)
		{ j = reverse[i] ;
		if (rampMin < rampMax)
		  {
		    if (j < rampMin)
		      hexVal[i] = 0 ;
		    else if (j >= rampMax)
		      hexVal[i] = 0xff ;
		    else
		      hexVal[i] = fac * (j - rampMin) ;
		  }
		else
		  {
		    if (j < rampMax)
		      hexVal[i] = 0xff ;
		    else if (j >= rampMin)
		      hexVal[i] = 0 ;
		    else
		      hexVal[i] = fac * (rampMin - j) ;
		  }
		}
#endif /* ACEDB_GTK */
	      err = aceOutPrint(ps->fo, "/picstring %d string def\n", w) ;
	      if (err != ESUCCESS)
		return err;
	      err = aceOutPrint(ps->fo, "  gsave") ;
	      if (err != ESUCCESS)
		return err;
	      err = aceOutPrint(ps->fo, " %.4f %.4f translate ", x1, y1) ;
	      if (err != ESUCCESS)
		return err;
	      err = aceOutPrint(ps->fo, " %.4f %.4f scale\n", x2-x1, y2-y1) ;
	      if (err != ESUCCESS)
		return err;
	      err = aceOutPrint(ps->fo, "  %d %d 8", w, h) ;
	      if (err != ESUCCESS)
		return err;
	      err = aceOutPrint(ps->fo, " [ %d 0 0 %d 0 0 ]", w, h) ;
	      if (err != ESUCCESS)
		return err;
	      err = aceOutPrint(ps->fo, " { currentfile picstring"
				" readhexstring pop } image") ;
	      ip = pixels ;
	      for (j = 0 ; j < h ; j++)
		{ 
		  err = aceOutPrint(ps->fo, "\n  ") ;
		  if (err != ESUCCESS)
		    return err;
		  for (i = 0 ; i < w ; )
		    { 
		      err = aceOutPrint(ps->fo, "%02x", hexVal[*ip++]) ;
		      if (err != ESUCCESS)
			return err;
		      if (!(++i % 35))
			{
			  err = aceOutPrint(ps->fo, "\n    ") ;
			  if (err != ESUCCESS)
			    return err;
			} 
		    }
		  if (i < line)
		    ip += (line-i) ;
		}
	      err = aceOutPrint(ps->fo, "  grestore\n") ;
	      if (err != ESUCCESS)
		return err;
	    }
	}
	break ;
      case POLYGON: case LINE_SEGS:
	{ 
	  int n = stackNext (ps->graph->stack, int) ;
	  err =  psSetColor(ps, colour);
	  if (err != ESUCCESS)
	    return err;
	  err = psSetLineWidth(ps, lineWidth);
	  if (err != ESUCCESS)
	    return err;
	  err = psSetLineStyle(ps, lineStyle);
	  if (err != ESUCCESS)
	    return err;
	  if (n-- > 2)
	    { 
	      err = aceOutPrint(ps->fo,   "newpath %.4f %.4f moveto\n",
				stackNext (ps->graph->stack, float), 
				stackNext (ps->graph->stack, float)) ;
	      if (err != ESUCCESS)
		return err;
	      while (n--)
		{
		  err = aceOutPrint(ps->fo, "        %.4f %.4f lineto\n",
				    stackNext (ps->graph->stack, float),
				    stackNext (ps->graph->stack, float)) ;
		  if (err != ESUCCESS)
		    return err;
		}
	      if (action == POLYGON)
		err = aceOutPrint(ps->fo,"        fill\n") ;
	      else
		err = aceOutPrint(ps->fo,"        stroke\n") ;
	      if (err != ESUCCESS)
		return err;
	    }
	  break ;
	}
      case TEXT: case TEXT_UP: case TEXT_PTR: case TEXT_PTR_PTR: 
	{
	  char *cp = 0;
	  
	  x1 = stackNext (ps->graph->stack,float) ;
	  y1 = stackNext (ps->graph->stack,float) ;
	  
	  err =  psSetColor(ps, colour);
	  if (err != ESUCCESS)
	    return err;
	  err = psSetLineWidth(ps, lineWidth);
	  if (err != ESUCCESS)
	    return err;
	  err = psSetLineStyle(ps, lineStyle);
	  if (err != ESUCCESS)
	    return err;
	  err = psSetFont(ps, textFormat, textHeight);
	  if (err != ESUCCESS)
	    return err;
	  if (action == TEXT_UP)
	    err = aceOutPrint(ps->fo, 
			      "gsave %f %f translate -90 rotate 0.6 1.0 scale"
			      " 0.2 0.0 moveto (",
			      x1, y1) ;
	  else
	    err = aceOutPrint (ps->fo, "%.4f %.4f moveto (",
			       x1, y1) ; 
	  if (err != ESUCCESS)
	    return err;
	  
	  switch (action)
	    {
	    case TEXT:
	    case TEXT_UP:
	      cp = stackNextText(ps->graph->stack) ;
	      break;
	    case TEXT_PTR:
	      cp = stackNext(ps->graph->stack,char*) ; 
	      break;
	    case TEXT_PTR_PTR:
	      cp = *stackNext(ps->graph->stack,char**) ;
	      break;
	    }
	  
	  if (cp) while(*cp)
	    { 
	      switch (*cp)
		{ 
		case '(': 
		case ')': 
		case '\\':
		  err = aceOutPrint(ps->fo, "\\") ;
		  if (err != ESUCCESS)
		    return err;
		  break ;
		}
	      /* use binary to avoid interpreting % */
	      err = aceOutBinary(ps->fo, cp++, 1) ;
	      if (err != ESUCCESS)
		return err;
	    }
 
	  if (action == TEXT_UP)
	    err = aceOutPrint(ps->fo, ") show grestore\n") ;
	  else
	    err = aceOutPrint(ps->fo, ") show\n") ;
	  if (err != ESUCCESS)
	    return err;
	}
	break ;
      case CIRCLE: case POINT: 
      case COLOR_SQUARES: case FILL_ARC: case ARC:
	x1 = stackNext (ps->graph->stack,float) ;
        y1 = stackNext (ps->graph->stack,float) ;
        err =  psSetColor(ps, colour);
	if (err != ESUCCESS)
	  return err;
	err = psSetLineWidth(ps, lineWidth);
	if (err != ESUCCESS)
	  return err;
	err = psSetLineStyle(ps, lineStyle);
	if (err != ESUCCESS)
	  return err;
	switch (action)

          {
          case CIRCLE:
            err = aceOutPrint(ps->fo,"%.4f %.4f moveto\n", x1, y1) ;
	    if (err != ESUCCESS)
	      return err; 
	    err = aceOutPrint(ps->fo,
			      "newpath %.4f %.4f %.4f 0 360 arc stroke\n",
			      x1, y1, 
			      stackNext (ps->graph->stack,float)) ;
            if (err != ESUCCESS)
	      return err;
	    break ;
	  case ARC:
	    { 
	      float r = stackNext (ps->graph->stack, float) ;
	      float ang = stackNext (ps->graph->stack, float) ;
	      float angdiff = stackNext (ps->graph->stack, float) ;
	      err = aceOutPrint(ps->fo, 
				"newpath %.4f %.4f %.4f %.4f %.4f arc stroke\n",
				x1, y1, r, -ang-angdiff, -ang) ;
	      if (err != ESUCCESS)
		return err;
	    }
            break ;
	  case FILL_ARC:
	    { 
	      float r = stackNext (ps->graph->stack, float) ;
	      float ang = stackNext (ps->graph->stack, float) ;
	      float angdiff = stackNext (ps->graph->stack, float) ;
	      err = aceOutPrint(ps->fo,"newpath %.4f %.4f moveto\n", x1, y1) ;
	       if (err != ESUCCESS)
		 return err;
	       err = aceOutPrint(ps->fo,"%.4f %.4f %.4f %.4f %.4f arc fill\n",
				 x1, y1, r, -ang-angdiff, -ang) ;
	       if (err != ESUCCESS)
		 return err;
	    }
            break ;
          case POINT:
            {
	      float psize2 = pointSize/2;
	      err = aceOutPrint(ps->fo,"%.4f %.4f %.4f %.4f rectangle fill\n",
				x1-psize2,y1-psize2,x1+psize2,y1+psize2) ;
	      if (err != ESUCCESS)
		return err;
	    }
	    break ;
	  case COLOR_SQUARES:
	    { 
	      char *text = stackNext (ps->graph->stack, char*) ;
	      int n = stackNext (ps->graph->stack, int) ;
	      int s = stackNext (ps->graph->stack, int) ;
	      int *tints = stackNext (ps->graph->stack, int*) ;
	      int x2 = 0;
	      int new = 0, old = colour ;
	      while (n--)
		{ 
		  switch(*text^((*text)-1))
		    {
		    case -1:   new = WHITE; break;
		    case 0x01: new = tints[0]; break;
		    case 0x03: new = tints[1]; break;
		    case 0x07: new = tints[2]; break;
		    case 0x0f: new = tints[3]; break;
		    case 0x1f: new = tints[4]; break;
		    case 0x3f: new = tints[5]; break;
		    case 0x7f: new = tints[6]; break;
		    case 0xff: new = tints[7]; break;
		    }
		  if (old != new)
		    {
		      if (x2 > 0)
			{
			  err = 
			    aceOutPrint(ps->fo,
					"%.4f %.4f %.4f %.4f rectangle fill\n",
					x1, y1, x1+x2, y1+1) ;
			  if (err != ESUCCESS)
			    return err;
			}
		      x1 += x2; x2 = 0;
		      err = psSetColor(ps, new);
		      if (err != ESUCCESS)
			return err;
		      old = new;
		    }
		  x2++;
		  text += s ;
		}
	      if (x2 > 0)
		{ 
		  err = aceOutPrint(ps->fo,
				    "%.4f %.4f %.4f %.4f rectangle fill\n",
				    x1, y1, x1+x2, y1+1) ;
		  if (err != ESUCCESS)
		    return err;
		}
	      
	      err =  psSetColor(ps, colour);
	      if (err != ESUCCESS)
		return err;
	    }
	    break ;
	  case IMAGE:
	    {
	      void* gim = stackNext (ps->graph->stack, void*) ; 
	      gim = 0 ;
	    }
	    break ;
	  default:
	    messout ("Invalid action %d received in drawPSBox",action) ;
	    break ;     
	  }
      }

  return ESUCCESS;
}


static int psTitle (psContext *ps, char *title, time_t *time)
{
  int err;

  err = aceOutPrint(ps->fo,
		    "%%%%title page begin\n"
		    "gsave\n"
		    "20 756 translate 7 -12 scale\n"
		    "0.0 setgray\n"
		    "0 titleFont setfont\n"
		    "0 -5 moveto\n"
		    "( %s) show\n", 
		    ctime(time)) ;
  
  if (err != ESUCCESS)
    return err;
  
  if (title && *title)
    { 
      err = aceOutPrint(ps->fo,
			"0 -4 moveto\n"
			"( %s) show\n",
			title) ;
      if (err != ESUCCESS)
	return err;
    }
  
  err = aceOutPrint(ps->fo,
		    "grestore\n"
		    "%%%%title page end\n") ;

  return err;
  
}

static int psCreateFonts(psContext *ps, Stack names)
{
  int dy;
  int err;
  float scale1, scale1fix, scale2;
  
  if (!gFontInfo (ps->graph, 0, 0, &dy)) 
    /* number of pixels of a character */
    messcrash ("Can't get font info for current font") ;
  
  /* srk - I think these should be yFac - but this is how it always was.  */
  scale1 = dy/ps->graph->xFac;
  scale1fix = dy*1.026/ps->graph->xFac; /* orig: esr 12/94 */
  scale2 = -((float)dy)/(ps->graph->aspect*ps->graph->xFac);
  
  
  err = aceOutPrint(ps->fo,
		    "/defaultFont	%% --\n"
		    "  /%s findfont 0 -0.7 matrix translate makefont\n"
		    "  %f %f matrix scale makefont def\n", 
		    stackNextText(names), scale1, scale2) ;
  if (err != ESUCCESS)
    return err;
    
  err = aceOutPrint(ps->fo,
		    "/italicFont	%% --\n"
		    "  /%s findfont 0 -0.7 matrix translate makefont\n"
		    "  %f %f matrix scale makefont def\n", 
		    stackNextText(names), scale1, scale2) ;
  if (err != ESUCCESS)
    return err;

  err = aceOutPrint(ps->fo,
		    "/boldFont	%% --\n"
		    "  /%s findfont 0 -0.7 matrix translate makefont\n"
		    "  %f %f matrix scale makefont def\n", 
		    stackNextText(names), scale1, scale2) ;
  if (err != ESUCCESS)
    return err;

  err = aceOutPrint(ps->fo,
		    "/fixedFont	%% --\n"
		    "  /%s findfont 0 -0.7 matrix translate makefont\n"
		    "  %f %f matrix scale makefont def\n", 
		    stackNextText(names), scale1fix, scale2) ;
  if (err != ESUCCESS)
    return err;

  err = aceOutPrint(ps->fo,
		    "/titleFont	%% --\n"
		    "  /%s findfont 0 -0.7 matrix translate makefont\n"
		    "  %f %f matrix scale makefont def\n", 
		    stackNextText(names), scale1, scale2) ;

  return err;
}
 
void psGetPaperSize(PaperType t, float *pageWidth, float *pageHeight)
{
  /* page sizes cribbed from netscape print window
     and scaled from original A4 numbers */
  switch (t)
    {
    case PAPERTYPE_A4:
      *pageWidth = 558.0F; *pageHeight = 756.0F;
      break;
    case PAPERTYPE_LETTER:
     *pageWidth = 573.6F; *pageHeight = 710.0F;
      break;
    case PAPERTYPE_LEGAL:
      *pageWidth = 573.6F; *pageHeight = 905.0F;
      break;
    case PAPERTYPE_EXECUTIVE:
      *pageWidth = 506.1F; *pageHeight = 646.5F;
      break;
    }
}    
/****************************************************************/


void graphPSdefaults (Graph gId, PaperType paper, 
		      BOOL *pRot, float *pFac, int *pPage)
{
  Graph_ graph;
  float pageWidth, pageHeight; 
  float h, w;
  float xfac, yfac, fac ;
  BOOL isRotate = FALSE ;
  
  graph = gGetStruct(gId);
  if (!graph)
    return;

  h = graph->uh ;
  w = graph->uw / graph->aspect ;
  
  psGetPaperSize(paper, &pageWidth, &pageHeight);
   
  switch (graph->type)
    { 
    case TEXT_SCROLL:  
    case TEXT_FULL_SCROLL:
    case TEXT_FIT:
    case TEXT_HSCROLL:
      if (12 * w < pageWidth) /* reduce, but never blow up */
	fac = 12.0 ;
      else if (10 * w < pageWidth)
	fac = 10.0 ;
      else 
	{ if (w > h)
	   { float x = w ; w = h ; h = x ; 
	     isRotate = TRUE ; 
	     if (12 * w < pageWidth) /* reduce, but never blow up */
	       fac = 12.0 ;
	     else if (10 * w < pageWidth)
	       fac = 10.0 ;
	     else
	       fac = .95 * pageWidth / w;
	   }
	  else
	    fac = .95 * pageWidth / w;
	}
      break ;
    case MAP_SCROLL: case PIXEL_SCROLL: case PIXEL_FIT:
    case PIXEL_VSCROLL: case PIXEL_HSCROLL:
    case PLAIN:
      if (w > h)
	 { float x = w ; w = h ; h = x ; isRotate = TRUE ; }
      xfac = .996 * pageWidth / w ;
      yfac = .996 * pageHeight / h ;
      fac = (xfac < yfac) ? xfac : yfac ;
      break ;
    default:
      messcrash ("Unknown graph type in graphPS") ;
      return ;
    }

  *pRot = isRotate ;
  *pFac = fac ;
  *pPage = 1 + h * fac/pageHeight ;

}



int graphPS (Graph gId, ACEOUT fo, char *title, 
	     BOOL doColor, BOOL isEPSF, BOOL isRotate, PaperType paper,
	     float fac, int pages)
{ 
  psContext ps;
  Stack fontNames;
  int err, i;
  float pageWidth, pageHeight; 
  static char *psHeader = "%!PS-Adobe-1.0" ;		   
  static char *EPSFHeader = "EPSF-1.2" ;
  time_t now;

  ps.graph = gGetStruct(gId);
  if (!ps.graph)
    return EINVAL;

  time(&now);
  
  /* Set defaults, mostly set so that they will be changed the first time they are
   * queried with the psGetXXXX routines. */
  psGetPaperSize(paper, &pageWidth, &pageHeight);
  ps.fo = fo;
  ps.isColor = doColor;
  ps.currColor = -1;
  ps.currLineWidth = -1.0;
  ps.currLineStyle = GRAPH_LINE_SOLID ;
  ps.currTextFormat = 0;
  ps.currTextHeight = -1.0;

  if (isEPSF)
    err = aceOutPrint(fo, "%s    %s\n", psHeader, EPSFHeader) ;
  else
    err = aceOutPrint(fo, "%s\n", psHeader) ;
  
  if (err != ESUCCESS)
    return err;
    
  if (title && *title) 
    err = aceOutPrint(fo,"%%%%Title: %s\n",title);
  else 
    err = aceOutPrint(fo,"%%%%Title: acedb generated document\n");
  
  if (err != ESUCCESS)
    return err;
  
  
  err = aceOutPrint(fo, "%%%%CreationDate: %s\n", ctime(&now)) ;
  if (err != ESUCCESS)
    return err;
  
  if ((pages == 1) && isEPSF)
    { 
      float bx1, bx2, by1, by2 ;
      float ux1, ux2, uy1, uy2 ;
      
      graphBoxDim (0, &ux1, &uy1, &ux2, &uy2) ;
      ux1 -= gActive->ux ; ux2 -= gActive->ux ;
      uy1 -= gActive->uy ; uy2 -= gActive->uy ;
      ux1 *= fac/ps.graph->aspect ; ux2 *= fac/ps.graph->aspect ;
      uy1 *= -fac ; uy2 *= -fac ;
      if (isRotate)
	{ 
	  bx1 = uy1 ; bx2 = uy2 ;
	  by1 = -ux1 ; by2 = -ux2 ;
	  bx1 += pageWidth ; bx2 += pageWidth ;
	  by1 += pageHeight ; by2 += pageHeight ;
	}
      else
	{
	  bx1 = 18 + ux1 ; bx2 = 18 + ux2 ;
	  by1 = pageHeight + uy2 ;
	  by2 = pageHeight + uy1 ;
	}
      err = aceOutPrint (fo,"%%%%BoundingBox: %.1f %.1f %.1f %.1f\n",
			 bx1, by1, bx2, by2) ;
      if (err != ESUCCESS)
	return err;
    }
  
  err = aceOutPrint (fo,"%%%%EndComments\n");
  if (err != ESUCCESS)
    return err;
  
  err = 
    aceOutPrint(fo,
		"/rectangle    %% x1 y1 x2 y2 ==> _\n"
		" {/y2 exch def  /x2 exch def /y1 exch def /x1 exch def\n" 
		"  newpath\n"
		"    x1 y1 moveto x2 y1 lineto x2 y2 lineto x1 y2 lineto\n"
		"  closepath\n"
		" } def\n") ;
  if (err != ESUCCESS)
    return err;
  
  fontNames = psGetFontNames();
  stackCursor(fontNames,0) ; /* RD adopted Jean's font fix */
  err = psCreateFonts(&ps, fontNames);
  stackDestroy(fontNames);
  if (err != ESUCCESS)
    return err;
  
  if (title && *title) 
    {
      err = psTitle (&ps, title, &now); 
      if (err != ESUCCESS)
	return err;
    }
  
  if (isRotate)
    err = aceOutPrint(fo, "gsave\n%f %f translate -90 rotate  ",
		      pageWidth, pageHeight) ;
  else
    err = aceOutPrint(fo, "gsave\n18 %f translate  ",
		      pageHeight) ;
  if (err != ESUCCESS)
    return err;
  
  err = aceOutPrint(fo, "%.4f %.4f scale  ", fac/ps.graph->aspect, -fac) ;
  if (err != ESUCCESS)
    return err; 
  err = aceOutPrint(fo,
		    "%.4f %.4f translate\n",
		    -(ps.graph->ux), -(ps.graph->uy)) ;
  if (err != ESUCCESS)
    return err; 
  
  for (i = 0 ; i < pages ; i++)
    { 
      err = psDrawBox (&ps, arrp(ps.graph->boxes, 0, BoxStruct));
      if (err != ESUCCESS)
	return err; 
      
      err = aceOutPrint(fo, "\ngsave showpage grestore\n\n") ;
      if (err != ESUCCESS)
	return err; 
      
      if (isRotate)
	err = aceOutPrint(fo,
			  "%f 0 translate\n",
			  -(ps.graph->aspect)*pageHeight/fac) ;
      else
	err = aceOutPrint(fo, "0 %f translate\n", -pageHeight/fac) ;
      
      if (err != ESUCCESS)
	return err; 
      
    }
  
  err = aceOutPrint(fo, 
		    "grestore\n"
		    "%%%%Trailer\n");
  
  if (err != ESUCCESS)
    return err; 
  
  
  return err;
}

/******** Public function *************/
/***** end of file *****/


 
 
 
 
