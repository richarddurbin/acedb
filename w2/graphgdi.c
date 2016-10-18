/*  File: graphgdi.c
 *  Author: Simon Kelley (srk@sanger.ac.uk)
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
 * Description: to provide windows printer support
 * SCCS: $Id: graphgdi.c,v 1.11 2002-06-21 15:28:27 srk Exp $
 * HISTORY:
 * Last edited: Jun 20 11:38 2002 (edgrif)
 * CVS info:   $Id: graphgdi.c,v 1.11 2002-06-21 15:28:27 srk Exp $
 *-------------------------------------------------------------------
 */

#ifndef __CYGWIN__
void graphGdiPleaseCompiler (void) { return ; } 
/* cc does not allow empty files */
#else

#include <windows.h>

/********* Dec 19 16:00 1995 (rbrusk): the GraphAction POINT enum member
 * re#define'd to acePOINT to avoid conflict with a Win32 API typedef;
 * Also need a typedef workaround for use of the 
 * WIN32 POINT types in our ACEDB code 
 * modified this for use in CYGWIN port - srk
 ************************************************************/
typedef POINT 	WIN_PT ;
#define POINT acePOINT

int winTransparent = TRANSPARENT;
#undef TRANSPARENT


#include <wh/regular.h>
#include <wh/menu_.h>
#include <w2/graphdev.h>
#include <w2/graph_.h>
#include <w2/graphcolour.h>
#include <wh/aceio.h>		
#include <time.h>

#define CHECK(x)
#if 0
#define CHECK(x) if (((int)(x)) > 32767) \
     messdump("co-ordinate size overflow in graphgdi (%d)", (int)(x));
#endif 

/* 
We scale co-ordinates so that the size of the graph fits within these.
The 16-bit GDI on win 95/98 limits these to 32768, but we go lower, since the 
uh and uw fields in the Graph structure are not reliable.
*/

#define XGDIMAX 4096
#define YGDIMAX 4096

typedef struct {
  float xFac, yFac;
  HDC dc;
  HPEN pen;
  HBRUSH brush;
  HFONT font;
  BOOL fillMode;
  BOOL rotateMode;
  Graph_ graph;
  BOOL isColor;
  int currColor;
  int currLineWidth;
  GraphLineStyle currLineStyle;
  float currTextHeight;
  int currTextFormat;
} gdiContext;

static COLORREF col2ref(gdiContext *gdi, int col)
{
  int r, g, b, gr;

  graphGetColourAsInt(col, &r, &g, &b, &gr);

  if (gdi->isColor)
   return r | (g<<8) | (b<<16);
 else
   return gr | (gr<<8) | (gr<<16);
}

static int graph2PenStyle(GraphLineStyle style)
{
  int pen_style ;

  switch(style)
    {
    case GRAPH_LINE_SOLID :   pen_style = PS_SOLID ; break ;
    case GRAPH_LINE_DASHED :  pen_style = PS_DASH ; break ;
    default : messcrash("internal error, illegal graph line style: %d", style) ;
    }

  return pen_style ;
}


static COLORREF gdiMakePen(gdiContext *gdi)
{
  COLORREF colref;
  int col;

  col = gdi->currColor;
  
  colref = col2ref(gdi, col);

  if (gdi->pen)
    DeleteObject(gdi->pen);

  gdi->pen = CreatePen(graph2PenStyle(gdi->currLineStyle), gdi->currLineWidth, colref);

  SelectObject(gdi->dc, gdi->pen);

  return colref;
}

static void gdiSetColor(gdiContext *gdi, int col, float width, GraphLineStyle style)
{
  int w = (int)(width * gdi->xFac);

  CHECK(w);

  messAssert(style == GRAPH_LINE_SOLID || style == GRAPH_LINE_DASHED) ;

  if ((gdi->currColor != col) || (w != gdi->currLineWidth) || (style != gdi->currLineStyle)
      || gdi->fillMode )
    {
      gdi->currColor = col;
      gdi->currLineWidth = w;
      gdi->currLineStyle = style ;
      gdiMakePen(gdi);
      SelectObject(gdi->dc, GetStockObject(NULL_BRUSH));
      if (gdi->brush)
	DeleteObject(gdi->brush);
      gdi->fillMode = FALSE;
    }
}

static void gdiSetFill(gdiContext *gdi, int col)
{
  COLORREF colref;
  
  if ((gdi->currColor!=col) || (gdi->currLineWidth!=0) || !gdi->fillMode )
    {
      gdi->currColor = col;
      gdi->currLineWidth = 0;
      colref = gdiMakePen(gdi);
      gdi->fillMode = TRUE;
      if (gdi->brush)
	DeleteObject(gdi->brush);
  
      gdi->brush = CreateSolidBrush(colref);

      SelectObject(gdi->dc, gdi->brush);
    }
}


/*****************************************/

static void gdiSetFont(gdiContext *gdi, float height, 
		       int format, int col, BOOL rotate)
{
  LOGFONT font;
 
  SetTextColor(gdi->dc, col2ref(gdi, col));
  
  if ((format != gdi->currTextFormat) || 
      (height != gdi->currTextHeight) ||
      (rotate != gdi->rotateMode))
    {
      gdi->currTextFormat = format;
      gdi->currTextHeight = height;
      gdi->rotateMode = rotate;

      font.lfHeight = gdi->yFac*-height;
      font.lfWidth = 0;
      font.lfEscapement = rotate ? 2700 : 0;
      font.lfOrientation = rotate ? 2700 : 0;
      font.lfWeight = (format == BOLD) ? 700 : 0;
      font.lfItalic = (format == ITALIC) ? 1 : 0;
      font.lfUnderline = 0;
      font.lfStrikeOut = 0;
      font.lfCharSet = (format == GREEK) ? GREEK_CHARSET : 0;
      font.lfOutPrecision = 0;
      font.lfClipPrecision = 0;
      font.lfQuality = 0;
      font.lfPitchAndFamily = FIXED_PITCH | FF_MODERN;
      *font.lfFaceName = 0;

      if (gdi->font)
	DeleteObject(gdi->font);

      gdi->font = CreateFontIndirect(&font);
      
      SelectObject(gdi->dc, gdi->font);
      
    }

}

/***********************************************/

static void gdiDrawBox (gdiContext *gdi, Box box)
{
  int  x1,x2,y1,y2 ;
  int  action;
  float pointSize = box->pointsize;
  float textHeight = box->textheight;
  int textFormat = box->format;
  float lineWidth =  box->linewidth;
  GraphLineStyle lineStyle = box->line_style ;
  int colour = box->fcol;

  if ((box->bcol != TRANSPARENT) &&
      (box->bcol != WHITE) && 
      (box->x1 <=  999999.0)) 
    { 
      gdiSetFill(gdi, box->bcol);
      Rectangle(gdi->dc, 
		(int)(gdi->xFac * box->x1),
		(int)(gdi->yFac * box->y1),
		(int)(gdi->xFac * box->x2),
		(int)(gdi->yFac * box->y2)) ;
      
    }

  stackCursor (gdi->graph->stack, box->mark) ;
  
  while (!stackAtEnd (gdi->graph->stack))
    switch (action = stackNext (gdi->graph->stack, int))
      {
      case BOX_END:
        return;                        /* exit point */
      case BOX_START:
	gdiDrawBox (gdi, arrp (gdi->graph->boxes, 
			       stackNext(gdi->graph->stack, int),
			       BoxStruct));
	break;
      case COLOR:
        colour = stackNext (gdi->graph->stack,int) ;
	break;
      case TEXT_FORMAT:
        textFormat = stackNext (gdi->graph->stack,int) ;
	break;
      case LINE_WIDTH:
	lineWidth = stackNext (gdi->graph->stack,float);
	break ;
      case LINE_STYLE:
	lineStyle = stackNext (gdi->graph->stack,float);
	break ;
      case TEXT_HEIGHT:
	textHeight = stackNext (gdi->graph->stack,float) ;
	break ;
      case POINT_SIZE:
        pointSize = stackNext (gdi->graph->stack,float) ;
        break ;
      case LINE: case RECTANGLE: case FILL_RECTANGLE:
        x1 = (int)(gdi->xFac * stackNext (gdi->graph->stack,float));
        y1 = (int)(gdi->yFac * stackNext (gdi->graph->stack,float));
        x2 = (int)(gdi->xFac * stackNext (gdi->graph->stack,float));
        y2 = (int)(gdi->yFac * stackNext (gdi->graph->stack,float));
	CHECK(x1); CHECK(y1); CHECK(x2); CHECK(y2);
	switch (action)
          {
          case LINE:
            gdiSetColor(gdi, colour, lineWidth, lineStyle);
	    MoveToEx(gdi->dc, x1, y1, NULL);
	    LineTo(gdi->dc, x2, y2);
            break ;
          case RECTANGLE:
	    gdiSetColor(gdi, colour, lineWidth, lineStyle);
	    Rectangle(gdi->dc, x1, y1, x2, y2) ;
	    break ;
	  case FILL_RECTANGLE:
	    gdiSetFill(gdi, colour);
	    Rectangle(gdi->dc, x1, y1, x2, y2) ;
	    break ;
	  }
	break ;
      case PIXELS: case PIXELS_RAW:
	{ 
	  unsigned char *ip, *pixels; 
	  int w, h, line, i, j;
	  x1 = (int)(gdi->xFac * stackNext (gdi->graph->stack,float));
	  y1 = (int)(gdi->yFac * stackNext (gdi->graph->stack,float));
	  CHECK(x1); CHECK(y1);
	  pixels = stackNext (gdi->graph->stack, unsigned char*) ;
	  w = stackNext (gdi->graph->stack, int) ;
	  h = stackNext (gdi->graph->stack, int) ;
	  line = stackNext (gdi->graph->stack, int) ;
	  if (action == PIXELS)
	    { 
	      /* PIXELS are not implemented yet */
	      x2 = stackNext (gdi->graph->stack, float) ;
	      y2 = stackNext (gdi->graph->stack, float) ;
	    }
	  else
	    { 
	      struct {
		BITMAPINFOHEADER bh;
		RGBQUAD cols[256];
	      } im;
	      
	      unsigned char *data = messalloc(h*line);
	      
	      im.bh.biSize = sizeof(BITMAPINFOHEADER);
	      im.bh.biWidth = w;
	      im.bh.biHeight = h;
	      im.bh.biPlanes = 1;
	      im.bh.biBitCount = 8;
	      im.bh.biCompression = 0;
	      im.bh.biSizeImage = 0;
	      im.bh.biXPelsPerMeter = 100;
	      im.bh.biYPelsPerMeter = 100;
	      im.bh.biClrUsed = 256;
	      im.bh.biClrImportant = 0;

	      for (i=0; i<256; i++)
		{ 
		  unsigned char c =  graphGreyRampPixelIntensity(i);
		  im.cols[i].rgbBlue = c;
		  im.cols[i].rgbGreen = c;
		  im.cols[i].rgbRed = c;
		}

	      for (ip = data, j = h-1; j>=0; j--)
		{
		  unsigned char *lineaddr = pixels + (line*j);
		  memcpy(ip, lineaddr, line);
		  ip += line;
		}

	      StretchDIBits(gdi->dc, x1, y1,
			    (int)(gdi->xFac * (float)w/gdi->graph->xFac),
			    (int)(gdi->yFac * (float)h/gdi->graph->yFac),
			    0, 0, w, h,
			    (CONST VOID *)data, 
			    (BITMAPINFO *)&im, DIB_RGB_COLORS, SRCCOPY);

	      CHECK((int)(gdi->xFac * (float)w/gdi->graph->xFac));
	      CHECK((int)(gdi->yFac * (float)h/gdi->graph->yFac));
	      messfree(data);
	    }
	  
	}
	break ;
      case POLYGON: case LINE_SEGS:
	{ 
	  int n = stackNext (gdi->graph->stack, int) ;
	  if (n >= 2) 
            {         
	      WIN_PT *points, *p;
              int i;
              
              points = (WIN_PT *)messalloc((n) * sizeof(WIN_PT));
              for(i=0,p=points;i<n;i++,p++) 
                {
                  p->x = (LONG)(gdi->xFac * 
				stackNext (gdi->graph->stack, float));
                  p->y = (LONG)(gdi->yFac * 
				stackNext (gdi->graph->stack, float));
		  CHECK(p->x); CHECK(p->y);
                }
              if (action == POLYGON)
                {
		  gdiSetFill(gdi, colour);
		  Polygon(gdi->dc, points, n);
		}
	      else
		{
		  gdiSetColor(gdi, colour, lineWidth, lineStyle);
		  Polyline(gdi->dc, points, n);
		}
	      messfree(points);
            }
	  break ;
	}
      case CIRCLE: case POINT: 
      case TEXT: case TEXT_UP: case TEXT_PTR: case TEXT_PTR_PTR: 
      case COLOR_SQUARES: case FILL_ARC: case ARC:
	x1 = (int)(gdi->xFac * stackNext (gdi->graph->stack,float));
	y1 = (int)(gdi->yFac * stackNext (gdi->graph->stack,float));
	CHECK(x1); CHECK(y1);
	switch (action)
          {
          case CIRCLE:
            {
	      int r = (int)(gdi->xFac * 
			    stackNext(gdi->graph->stack,float));
	      CHECK(r);
	      gdiSetColor(gdi, colour, lineWidth, lineStyle);
	      Ellipse(gdi->dc, x1-r, y1-r, x1+r, y1+r);
	    }
	    break ;
	  case ARC:
	    { 
	      DWORD r = (DWORD)(gdi->xFac *
				stackNext (gdi->graph->stack, float)) ;
	      float ang = stackNext (gdi->graph->stack, float) ;
	      float angdiff = stackNext (gdi->graph->stack, float) ;
	      gdiSetColor(gdi, colour, lineWidth, lineStyle);
	      AngleArc(gdi->dc, x1, y1, r, ang, angdiff);
	    }
            break ;
	  case FILL_ARC:
	    { 
	      float r = stackNext (gdi->graph->stack, float) ;
	      /* get angles in radians */
	      float ang = stackNext (gdi->graph->stack, float) * 0.1745 ;
	      float angdiff = stackNext (gdi->graph->stack, float) * 0.1745 ;
	      gdiSetFill(gdi, colour);
	      Pie(gdi->dc, x1-r, y1-r, x1+r, y1+r,
		  x1+(int)(1000.0*sin((double)ang)),
		  y1+(int)(1000.0*cos((double)ang)),
		  x1+(int)(1000.0*sin((double)(ang+angdiff))),
		  y1+(int)(1000.0*cos((double)(ang+angdiff))));
	    }
            break ;
          case POINT:
            {
	      int psize = (int)(pointSize * gdi->xFac) ;
	      gdiSetFill(gdi, colour);
	      Ellipse(gdi->dc, x1-psize, y1-psize, x1+psize, y1+psize);
	    }
	    break ;
	   case TEXT:
	    { 
	      char *text = stackNextText(gdi->graph->stack);
	      gdiSetFont(gdi, textHeight, textFormat, colour, FALSE); 
	      TextOut(gdi->dc, x1, y1, text, strlen(text));
	      break;
	    }
	  case TEXT_UP:
	    { 
	      char *text = stackNextText(gdi->graph->stack);
	      gdiSetFont(gdi, textHeight * 0.6, textFormat, colour, TRUE); 
	      TextOut(gdi->dc, x1+gdi->xFac,
		      y1 - strlen(text) * gdi->yFac * textHeight * 0.6,
		      text, strlen(text));
	      CHECK(x1+gdi->xFac);
	      break;
	    }
	  case TEXT_PTR:
	    { 
	      char *text = stackNext(gdi->graph->stack, char *);
	      gdiSetFont(gdi, textHeight, textFormat, colour, FALSE);
	      TextOut(gdi->dc,x1,y1,text,strlen(text));
	      break;
	    }
	  case TEXT_PTR_PTR:
	    {
	      char *text = *stackNext(gdi->graph->stack, char **);
	      gdiSetFont(gdi, textHeight, textFormat, colour, FALSE); 
	      if (text && *text)
		gdImageString(gdi->dc,x1,y1,text, strlen(text));
	      break;
	    }
	  case COLOR_SQUARES:
	    { 
	      char *text = stackNext (gdi->graph->stack, char*) ;
	      int n = stackNext (gdi->graph->stack, int) ;
	      int s = stackNext (gdi->graph->stack, int) ;
	      int *tints = stackNext (gdi->graph->stack, int*) ;
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
			  Rectangle(gdi->dc, x1, y1, 
				    x1+ x2, y1+gdi->yFac);
			  CHECK(y1+gdi->yFac);
			}			
		      x1 += x2; x2 = 0;
		      gdiSetFill(gdi, new);
		      old = new;
		    }
		  x2 +=  gdi->xFac;
		  text += s ;
		}		 
	      if (x2 > 0)
		{
		  Rectangle(gdi->dc, x1, y1, x1+ x2, y1+gdi->yFac);
		  CHECK(y1+gdi->yFac);
		}
	    }
	    break ;
	  case IMAGE:
	    {
	      void* gim = stackNext (gdi->graph->stack, void*) ; 
	      gim = 0 ;
	    }
	    break ;
	  default:
	    messout ("Invalid action %d received in graphGDIdrawBox",action) ;
	    break ;     
	  }
      }
}



/****************************************************************/

/* wantRotate = 0 -> NO, 1 -> YES. -1 -> DON'T CARE */
static void graphGDIcalc(int wantRotate, int graphType, float w, float h,
			 short paper, float *pFac, float *pStep, BOOL *pRot)
{
  float pageWidth, pageHeight; 
  float xfac, yfac, fac ;
  BOOL isRotate;

  switch (paper)
    {
    case DMPAPER_A4:
      pageWidth = 558.0F; pageHeight = 756.0F;
      break;
    case DMPAPER_LETTER:
      pageWidth = 573.6F; pageHeight = 710.0F;
      break;
    case DMPAPER_LEGAL:
      pageWidth = 573.6F; pageHeight = 905.0F;
      break;
    case DMPAPER_EXECUTIVE:
      pageWidth = 506.1F; pageHeight = 646.5F;
      break;
    }
  
  if (wantRotate == -1)
    { 
      if ((10 * w < pageWidth) &&
	  (graphType == TEXT_SCROLL ||
	   graphType == TEXT_FULL_SCROLL ||
	   graphType == TEXT_FIT ||
	   graphType == TEXT_HSCROLL
	   ))
	isRotate = FALSE;
      else
	isRotate = (w > h);
    }
  else if (wantRotate == 0)
    isRotate = FALSE;
  else
    isRotate = TRUE;
  
  if (isRotate)
    { 
      float x = w ; 
      w = h ; 
      h = x ; 
    }

  switch (graphType)
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
	fac = .95 * pageWidth / w;
      break ;
    case MAP_SCROLL: case PIXEL_SCROLL: case PIXEL_FIT:
    case PIXEL_VSCROLL: case PIXEL_HSCROLL:
    case PLAIN:
      xfac = .996 * pageWidth / w ;
      yfac = .996 * pageHeight / h ;
      fac = (xfac < yfac) ? xfac : yfac ;
      break ;
    default:
      messcrash ("Unknown graph type in graphGDI") ;
      return ;
    }

  *pRot = isRotate ;
  *pFac = fac ;
  *pStep = pageHeight ;

}

void graphPrintGDI (Graph_ graph)
{ 
  gdiContext gdi;
  int pagestart, pages, i;
  float step, vx, vy; 
  DOCINFO docInfo;
  float h, w, fac;
  PRINTDLG pd;
  DEVMODE *dev;
  short paper;
  BOOL isRotate;
  char *fileName = NULL;
  char selectedPath[MAX_PATH];

  if (!graph)
    return;

  pd.lStructSize = sizeof(PRINTDLG);
  pd.Flags = PD_RETURNDEFAULT;
  pd.hwndOwner = NULL;
  pd.hDC = NULL;
  pd.hDevMode = NULL;
  pd.hDevNames = NULL;

  if (PrintDlg(&pd) == 0)
    { 
      messcrash("Errno = %d. Cannot find a printer.", CommDlgExtendedError());
      return;
    }
  
  /* WTF? see Petzold pp729 */
  dev = (DEVMODE*) GlobalLock(pd.hDevMode);  
  
  if (dev->dmFields & DM_PAPERSIZE)
    paper = dev->dmPaperSize;
  else
    paper = DMPAPER_A4;

  h = graph->uh ;
  w = graph->uw / graph->aspect;

  graphGDIcalc(-1, graph->type, w, h, paper, &fac, &step, &isRotate);
  pages = 1 + h * fac/step;

  dev->dmFields |= DM_ORIENTATION | DM_SCALE;    
  dev->dmOrientation = isRotate ? DMORIENT_LANDSCAPE : DMORIENT_PORTRAIT;
  dev->dmScale = 100;
  
  GlobalUnlock(pd.hDevMode);

  pd.hwndOwner = NULL; /* TODO fix this */
  pd.hDC = NULL;
  pd.Flags = PD_RETURNDC | PD_NOSELECTION | 
    PD_ALLPAGES | PD_USEDEVMODECOPIESANDCOLLATE;
  pd.nCopies = 1;
  pd.nMinPage = 1;
  pd.nMaxPage = pages;
  pd.nFromPage = 1;
  pd.nToPage = pages;
  pd.hInstance = NULL;
  pd.lCustData = 0l;
  pd.lpfnPrintHook = NULL;
  pd.lpfnSetupHook = NULL;
  pd.lpPrintTemplateName = NULL;
  pd.lpSetupTemplateName = NULL;
  pd.hPrintTemplate = NULL;
  pd.hSetupTemplate = NULL;
  
  if (PrintDlg(&pd) == 0)
     return;

  if (pd.Flags & PD_PRINTTOFILE)
    {
      OPENFILENAME ofn;
      DWORD flags = OFN_EXTENSIONDIFFERENT | OFN_LONGNAMES |  OFN_HIDEREADONLY;
      
      ofn.lpstrFilter = NULL;
      ofn.lStructSize = sizeof(OPENFILENAME);
      ofn.hwndOwner = NULL; 
      ofn.hInstance = NULL;
      ofn.lpstrCustomFilter = NULL;
      ofn.nMaxCustFilter = 0;
      ofn.nFilterIndex = 0;
      ofn.lpstrFile = selectedPath;
      ofn.nMaxFile = MAX_PATH;
      ofn.lpstrFileTitle = NULL;
      ofn.lpstrInitialDir = NULL;
      ofn.lpstrTitle = "File to print to";
      ofn.Flags = flags;
      ofn.lpstrDefExt = NULL;
      
      if (!GetOpenFileName(&ofn))
	    return;
      
      fileName = selectedPath;
    }

  /* Recalulate, once the user has altered selections */
 
  dev = (DEVMODE *)GlobalLock(pd.hDevMode);
  
  if (dev->dmFields & DM_PAPERSIZE)
    paper = dev->dmPaperSize;
  else
    paper = DMPAPER_A4;
  graphGDIcalc(dev->dmOrientation == DMORIENT_LANDSCAPE ? 1 : 0,
	       graph->type, w, h, paper, &fac, &step, &isRotate);
  
  fac = fac * (((float)dev->dmScale)/100.0);
  pages = 1 + h * fac/step;

  if (pd.nToPage < pages)
    pages = pd.nToPage;

  if (pd.nFromPage > pages)
    pagestart = 1;
  else
    pagestart = pd.nFromPage;

  gdi.graph = graph;
  gdi.dc = pd.hDC;
  gdi.pen = NULL;
  gdi.font = NULL;
  gdi.fillMode = FALSE;
  gdi.rotateMode = FALSE;
  gdi.brush = NULL;
  if ((dev->dmFields & DM_COLOR) && (dev->dmColor == DMCOLOR_COLOR))
    gdi.isColor = TRUE;
  else
    gdi.isColor = FALSE;
  gdi.currColor = -1;
  gdi.currLineWidth = -1.0;
  gdi.currLineStyle = GRAPH_LINE_SOLID;
  gdi.currTextFormat = 0;
  gdi.currTextHeight = -1.0;
 
  /* These scaling factors are calculated to keep the integer
     co-ordinates calculated during drawing in the range 0-XGDIMAX, to
     avoid windows limitations */
  gdi.xFac = ((float)XGDIMAX)/w;
  gdi.yFac = ((float)YGDIMAX)/h;
  
  vx = w * (((float)GetDeviceCaps(gdi.dc, LOGPIXELSX))/72.0) * 
    fac/graph->aspect;
  vy = h * (((float)GetDeviceCaps(gdi.dc, LOGPIXELSY))/72.0) * fac;
  
  docInfo.cbSize = sizeof(docInfo);
  docInfo.lpszDocName = graph->name;
  docInfo.lpszOutput = fileName;
  docInfo.lpszDatatype = NULL;
  docInfo.fwType = 0;

  GlobalUnlock(pd.hDevMode);

  if (StartDoc(gdi.dc, &docInfo) < 0)
    messout("Error creating print job");
  else
    for (i = pagestart ; i <= pages ; i++)
      {
	StartPage(gdi.dc);
	/* On win9* StartPage re-sets the mapping mode and de-selects
	   objects, so we have to do the following on every page iteration */
	ResetDC(gdi.dc, pd.hDevMode);
	if (gdi.brush)
	  SelectObject(gdi.dc, gdi.brush);
	if (gdi.pen)
	  SelectObject(gdi.dc, gdi.pen);
	if (gdi.font)
	  SelectObject(gdi.dc, gdi.font);
	 
	SetMapMode(gdi.dc, MM_ANISOTROPIC);
	SetWindowExtEx(gdi.dc, XGDIMAX, YGDIMAX, NULL);
	SetViewportExtEx(gdi.dc, (int)vx, (int)vy, 0);
	SetBkMode(gdi.dc, winTransparent);
	
	if (isRotate)
	  SetWindowOrgEx(gdi.dc, 
			 (i-1) * (int)(step*gdi.xFac/fac), 0, NULL); 
	else
	  SetWindowOrgEx(gdi.dc, 
			 0, (i-1) * (int)(step*gdi.yFac/fac), NULL);
	gdiDrawBox (&gdi, arrp(gdi.graph->boxes, 0, BoxStruct));
	EndPage(gdi.dc);
      }
  
  EndDoc(gdi.dc);

  if (gdi.pen)
    DeleteObject(gdi.pen);
  if (gdi.brush)
    DeleteObject(gdi.brush);
  if (gdi.font)
    DeleteObject(gdi.font);
  DeleteDC(gdi.dc);
}

#endif /* __CYGWIN__ */
/******** Public function *************/
/***** end of file *****/


 
 
 
 
