/*  file: graphprint.c
 *  Author: Danielle et Jean Thierry-Mieg (mieg@kaa.cnrs-mop.fr)
 *  Copyright (C) J Thierry-Mieg and R Durbin, 1994
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
 * 	Richard Durbin (MRC LMB, UK) rd@mrc-lmba.cam.ac.uk, and
 *	Jean Thierry-Mieg (CRBM du CNRS, France) mieg@kaa.cnrs-mop.fr
 *
 * Description: Makes the print dialog window, which allows printing to
 *              a file, a printer or the users mailbox.
 * Exported functions:
 * HISTORY:
 * Last edited: Jun 25 15:26 2003 (rnc)
 * * Apr 29 16:03 1999 (edgrif): Added new graphSetBlockMode.
 * * Jan 25 16:02 1999 (edgrif): Add new getLogin call to libfree routine.
 * * Jan 20 16:07 1999 (edgrif): Added support for GIF images and non-EPSF
 *              postscript files. Added new magic_t type for struct. Restructured
 *              to put all globals in printer control block.
 * * Sep 25 11:42 1998 (edgrif): Replaced #ifdef ACEDB with new graph/acedb
 *              interface calls. Sadly had to make pdExecute external to
 *              avoid contorted code.
 * * Jul  9 17:31 1998 (fw): for ACEDB I changed the order of the items in the
 *                           menu - the ACE-typical Quit is now top of the
 *                           list and does a "Cancel", "OK" will do print
 * Created: Tue Mar  1 11:56:08 1994 (mieg)
 * CVS info:   $Id: graphprint.c,v 1.65 2003-06-25 14:30:10 rnc Exp $
 *-------------------------------------------------------------------
 */

#include <wh/regular.h>
#include <w2/graph_.h>
#include <glib.h>	

#ifdef __CYGWIN__
/* The functionality here is implemented in graphgdi.c for windows */

void graphPrint(void)
{
  graphPrintGDI(gActive);
}

#else

#include "mydirent.h"					    /* for getwd below */
#include "aceio.h"

typedef unsigned char OptionFlag ;			    /* Print option flags control. */
static const unsigned char OPTION_OFF   = 0x0U ;
static const unsigned char OPTION_ON    = 0x1U ;
static const unsigned char OPTION_FLIP  = 0x1U ;


/* print dialog control block, set up first time graphPrint is called, used  */
/* for subsequent calls.                                                     */
typedef struct PrintStateStruct
{
  magic_t *magic ;					    /* == &PrintState_MAGIC */
  char status ;
  char style ;
  OptionFlag doEPSF ;
  int EPSFBox ;
  OptionFlag doCopy, doMail, doPrint ;
  Graph target ;
  Graph graph ;
  char printerBuffer[80] ;
  BOOL ACEDB_LPR ;
  char dirBuffer[MAXPATHLEN] ;
  char filBuffer[FIL_BUFFER_SIZE] ;
  char copyBuffer[MAXPATHLEN+FIL_BUFFER_SIZE+5] ; /* +5 for extension */
  char mailerBuffer[80] ;
  char titleBuffer[80] ;
  Array printers ;
  int printerNum ;
  int titleBox, mailerBox, printerBox, fileBox,
    firstBox ;
  int blueBox ;
  BOOL isRotate ;
  char scaleText[10] ;
  float scale ;
  int firstScaleBox ;
  int pages ;
  char pageText[5] ;
  int pageBox ;
  int blueBoxLine ;
} *PrintState;


/* Here is the global pointer our control block, only referenced in          */
/* graphPrint().                                                             */
static PrintState lastPd = NULL ;


/* We use the address of the these strings as unique ids for the graph       */
/* associator from which we retrieve the control block and the control block */
/* contains the magic tag which tries to show the control block is a) the    */
/* right one and b) not corrupted.                                           */
static magic_t GRAPH2PrintState_ASSOC = "Graph2PrintStateAssoc" ;
static magic_t PrintState_MAGIC = "GraphPrintState" ;



static const float PAGEWIDTH  = 558.0F ;
static const float PAGEHEIGHT = 756.0F ;

static void pdOK(void)
{
  graphLoopReturn(TRUE);
}

static void pdCancel(void)
{
  graphLoopReturn(FALSE);
}

  /* normal style menu for non-ACEDB programs lists menu
     choices in order in which the buttons appear 
     (fw likes that, 980709) */
static MENUOPT pdMenu[] = {
  {pdOK, " OK "},
  {pdCancel, "Cancel"},
  {0, 0} } ;



static void pdDraw (void) ;
static void pdDrawBlueBox (void) ;
static void copyNameEnd (char *in) ;



/*************************************************************************/

/* Retrieves pointer to our global print state control block from the graph  */
/* from which the user first clicked a print button.                         */
/*                                                                           */
PrintState getPrintState(char *caller)
  {
  PrintState result = NULL ;

  if (!graphAssFind (&GRAPH2PrintState_ASSOC, &result))
    messcrash ("%() could not retrieve PrintState pointer from graph.", caller) ;
  if (result == NULL)
    messcrash ("%s() retrieved the PrintState pointer from graph but it was NULL.", caller) ;
  if (result->magic != &PrintState_MAGIC)
    messcrash ("%s() retrieved the PrintState pointer from graph but the PrintState is corrupted", caller) ;

  return result ;
  }


/*************************************************************************/


static void pdDestroy (void)
{ PrintState pd ;

 pd = getPrintState("pdDestroy") ;

 pd->graph = 0 ;

 graphActivate(pd->target) ;

 return ;
}


/*************************************************************************/

static void pdPick (int box, double x_unused, double y_unused, int modifier_unused)
{ PrintState pd ;

 pd = getPrintState("pdPick") ;

  if (!box)
    return ;

  if (pd->doPrint && arrayMax(pd->printers) && 
      box >= pd->firstBox && box < pd->firstBox + arrayMax(pd->printers))
    {
      /* restore old printerbox */
      graphBoxDraw (pd->firstBox + pd->printerNum, BLACK, WHITE) ;

      if (pd->ACEDB_LPR && box == pd->firstBox)
	strcpy (pd->printerBuffer, arr(pd->printers, 0, char*)) ;
      else
	{
	  sprintf (pd->printerBuffer, "lpr -P%s",
		   arr(pd->printers, box - pd->firstBox, char*)) ;
	}
      /* update the printcommand text */
      graphBoxDraw (pd->printerBox, -1, -1) ;

      /* activate new printer box */
      graphBoxDraw (box, BLACK, LIGHTBLUE) ;

      /* save the number of our current printer for redrawing */
      pd->printerNum = box - pd->firstBox ;
    }

  if (box >= pd->firstScaleBox && box <= pd->firstScaleBox+6)
    {
      int n = box - pd->firstScaleBox ;
      switch (n)
	{
	case 0:
	  pd->scale += 1 ;
	  break ;
	case 1:
	  if (pd->scale >= 1)
	    pd->scale -= 1 ;
	  break ;
	case 2:
	  pd->scale += 0.1 ;
	  break ;
	case 3:
	  if (pd->scale >= 0.1)
	    pd->scale -= 0.1 ;
	  break ;
	case 4:
	  pd->scale += 0.01 ;
	  break ;
	case 5:
	  if (pd->scale >= 0.02)
	    pd->scale -= 0.01 ;
	  break ;
	}
      if (pd->scale <= 0.01) pd->scale = 0.01 ;
      memset (pd->scaleText, 0, 10) ;
      if (pd->scale < 10.0) strcat (pd->scaleText, " ") ;
      strcat (pd->scaleText, messprintf ("%5.3f", pd->scale)) ;
      pd->scaleText[5] = 0 ;
      graphTextEntry (pd->scaleText, 0, 0, 0, 0) ;

      pdDrawBlueBox () ;
    }
}

/*************************************************************************/

void pdExecute (void)
{ BOOL isColor = FALSE ;
  PrintState pd ;

  pd = getPrintState("pdExecute") ;

  graphActivate (pd->target) ;

  switch (pd->style)
    {
    case 'a':
      graphASCII (pd->doCopy ? pd->copyBuffer : NULL ,
		  pd->doMail ? pd->mailerBuffer : NULL,
		  pd->doPrint ? pd->printerBuffer : NULL,
		  pd->titleBuffer) ;
      break ;
    case 'c':
      isColor = TRUE ; 
      /* fall thru */
    case 'p':
      /* TODO - support selecting other paper types */
      if (pd->doCopy)
	{
	  ACEOUT fo = aceOutCreateToFile(pd->copyBuffer, "w", 0);
	  if (fo)
	    {
	      graphPS(pd->target, fo, pd->titleBuffer, isColor, 
		      (pd->doEPSF == OPTION_ON ? TRUE : FALSE),
		      pd->isRotate, PAPERTYPE_A4,  pd->scale, pd->pages) ;

	      aceOutDestroy(fo);
	    }
	}
      if (pd->doPrint && 
	  (pd->pages < 3 || messQuery("The document has %d pages. "
				      "Do you really want to print it?",
				      pd->pages)))
	{
	  ACETMP tmp = aceTmpCreate("w", 0);
	  Stack s = stackCreate(100);
	  if (tmp)
	    {
	      graphPS(pd->target, aceTmpGetOutput(tmp), 
		      pd->titleBuffer, isColor, 
		      (pd->doEPSF == OPTION_ON ? TRUE : FALSE),
		      pd->isRotate, PAPERTYPE_A4,  pd->scale, pd->pages) ;
	      aceTmpClose(tmp);
	      pushText(s, pd->printerBuffer);
	      catText(s, " ");
	      catText(s, aceTmpGetFileName(tmp));
	      if (system(stackText(s, 0)))
		messout("Print command %s failed.", stackText(s, 0));
	      aceTmpDestroy(tmp);
	    }
	  stackDestroy(s);
	}
  if (pd->doMail)
	{
	  ACEOUT fo = aceOutCreateToMail(pd->mailerBuffer, 0);
	  if (fo)
	    {
	      graphPS(pd->target, fo, pd->titleBuffer, isColor, 
		      (pd->doEPSF == OPTION_ON ? TRUE : FALSE),
		      pd->isRotate, PAPERTYPE_A4,  pd->scale, pd->pages) ;
	      
	      aceOutDestroy(fo);
	    }
	}
      break ;
    case 'g':
      if (pd->doCopy)
	{
	  ACEOUT fo = aceOutCreateToFile(pd->copyBuffer, "w", 0);
	  if (fo)
	    {
	      graphGIF(pd->target, fo, FALSE);
	      aceOutDestroy(fo);
	    }
	}
      break ;
    }
}

/*************************************************************************/

static void psButton (void)
{ PrintState pd ;

 pd = getPrintState("psButton") ;
  
  pd->style = 'p' ;

  pdDraw() ;
}

/*****************/

static void cpsButton (void)
{ PrintState pd ;

 pd = getPrintState("cpsButton") ;
  
  pd->style = 'c' ;

  pdDraw() ;
}

/*****************/

static void gifButton (void)
{ PrintState pd ;

 pd = getPrintState("gifButton") ;
  
  pd->style = 'g' ;

  /* We currently don't support printing or mailing gif images directly, all */
  /* you can do is save the image to a file. (GIF files are binary and so    */
  /* we need MIME or uuencode to include them in mail, this will come if     */
  /* there is demand.                                                        */
  if (pd->doPrint) pd->doPrint = OPTION_OFF ;
  if (pd->doMail) pd->doMail = OPTION_OFF ;
  pd->doCopy = OPTION_ON ;

  
  pdDraw() ;
}

/*****************/

static void asciiButton (void)
{ PrintState pd ;

 pd = getPrintState("asciiButton") ;
  
  pd->style = 'a' ;

  pdDraw() ;
}


/*****************/

static void EPSFButton (void)
{ PrintState pd ;

 pd = getPrintState("EPSFButton") ;

 if (pd->style == 'p' || pd->style == 'c')
   pd->doEPSF ^= OPTION_FLIP ;

 pdDraw() ;
}


/*****************/

static void printerButton (void)
{ PrintState pd ;

 pd = getPrintState("printerButton") ;
  

  if (pd->style == 'g')
    pd->doPrint = OPTION_OFF ; 
  else
    pd->doPrint ^= OPTION_FLIP ;

  pdDraw() ;

  if (pd->doPrint) /* mieg */
      graphBoxDraw(pd->printerBox, BLACK, WHITE) ;
}

/*****************/

static void mailerButton (void)
{ PrintState pd ;

 pd = getPrintState("mailerButton") ;

  if (pd->style == 'g')
    pd->doMail = OPTION_OFF ; 
  else
    pd->doMail ^= OPTION_FLIP ;

  pdDraw() ;

  if (pd->doMail)
    graphTextScrollEntry (pd->mailerBuffer, 0, 0, 0, 0, 0) ;
}

/*****************/

static void copyButton (void)
{ PrintState pd ;

 pd = getPrintState("copyButton") ;
      
  pd->doCopy ^= OPTION_FLIP ;
  pdDraw() ;
  if (pd->doCopy)
    graphTextScrollEntry(pd->copyBuffer, 0, 0, 0, 0, 0) ;
}

/*****************/

static void fileButton (void)
{ FILE *tmp_fil = 0 ;
  char *ending ;					    /* Print file suffix. */
  char *mode ;						    /* file write mode. */
  PrintState pd ;

  pd = getPrintState("fileButton") ;
  
  copyNameEnd (pd->copyBuffer) ; /* to clean up dir- and filBuffer */

  switch (pd->style)
    {
    case 'c':
      /* fall thru */
    case 'p':
      ending = "ps" ;
      mode = "w" ;
      break ;
    case 'g':
      ending = "gif" ;
      mode = "wb" ;
      break ;
    case 'a':
      ending = "txt" ;
      mode = "w" ;
      break ;
    }

  tmp_fil = filqueryopen (pd->dirBuffer, pd->filBuffer, ending,
		      mode, "Print File") ;

  sprintf (pd->copyBuffer, "%s/%s.%s", pd->dirBuffer, pd->filBuffer, ending) ;

  if (tmp_fil)
    {
      filclose (tmp_fil) ;	/* just interested in pathname */
      filremove (pd->copyBuffer, "") ;
    }

  pdDraw() ;

  return;
} /* fileButton */

static void landScapeButton (void)
{ PrintState pd ;

 pd = getPrintState("landScapeButton") ;
      
  pd->isRotate = TRUE ;
  pdDraw() ;
}

static void portraitButton (void)
{ PrintState pd ;

 pd = getPrintState("portraitButton") ;
      
  pd->isRotate = FALSE ;
  pdDraw() ;
}


static void copyNameEnd (char *input)
{
  char *cp, *c1, *c2 ;
  char *ending ;
  PrintState pd ;

  pd = getPrintState("copyNameEnd") ;

  strcpy (pd->dirBuffer, input) ;
  for (cp = pd->dirBuffer; *cp ; ++cp) ;
  --cp ;
  while (*cp != '/' && cp > pd->dirBuffer) --cp ;

  strcpy (pd->filBuffer, ++cp) ;	/* save the name */
  *--cp = 0 ;	     /* cut off the '/' at the end of "dir" */

/* if dir-name contains ../ - handle this as a cd .. inside */
  cp = pd->dirBuffer ;
  while ((c1 = strstr (cp,"/../"))) /* /../ is important to handle ../../ */
    {
      c2 = c1 ; --c2 ;		/* start one char before "/" */
      /* search backwards for next "/" */
      while (*c2 != '/' && c2 > pd->dirBuffer)
	--c2 ;
      *c2 = 0 ;			/* mark this position as the end */
      c1 += 3 ;			/* skip "/.." in append string */
      strcat (c2, c1) ;		/* put both together */
    }
/* now remove /. from dir-name */
  cp = pd->dirBuffer ;
  while ((c1 = strstr (cp, "/./")))
    {
      c2 = c1 ; *c1 = 0 ;
      c2 += 2 ;
      strcat (c1, c2) ;
    }
  if (pd->dirBuffer[0] == '.' && pd->dirBuffer[1] == '/')
    { c1 = pd->dirBuffer ; c1 += 2 ;
      strcpy (pd->dirBuffer, c1) ;
    }
/* remove double slashes from the dir-name */
  cp = pd->dirBuffer ;
  while ((c1 = strstr (cp, "//")))
    {
      c2 = c1 ; *c1 = 0 ;
      c2 += 1 ;
      strcat (c1, c2) ;
    }

  /* now remove the ending from the filname */
  for (cp = pd->filBuffer; *cp ; ++cp) ; --cp ;
  while (*cp != '.' && cp > pd->filBuffer) --cp ;
  if (cp > pd->filBuffer)
    {
      ++cp ;
      if (strcmp(cp, "ps") == 0 || strcmp(cp, "gif") == 0 || strcmp(cp, "txt") == 0)
	*--cp = 0 ;
    }
  if (!strcmp(pd->filBuffer, ".ps")
      || !strcmp(pd->filBuffer, ".gif")
      || !strcmp(pd->filBuffer, ".txt"))
    *pd->filBuffer = 0 ;
  
  /* assemble the copyBuffer-path */
  /* avoid a double slash at the end */
  switch (pd->style)
    {
    case 'c':
      /* fall thru */
    case 'p':
      ending = "ps" ;
      break ;
    case 'g':
      ending = "gif" ;
      break ;
    case 'a':
      ending = "txt" ;
      break ;
    }

  if (pd->dirBuffer[strlen(pd->dirBuffer)-1] == '/')
    sprintf (pd->copyBuffer, "%s%s.%s", 
	     pd->dirBuffer, pd->filBuffer, ending) ;
  else
    sprintf (pd->copyBuffer, "%s/%s.%s", 
	     pd->dirBuffer, pd->filBuffer, ending) ;
  
  graphTextScrollEntry(pd->copyBuffer, 0, 0, 0, 0, 0) ;
}

static void scaleTextEnd (char *input)
{
  PrintState pd ;

  pd = getPrintState("scaleTextEnd") ;

  pd->scale = atof (pd->scaleText) ;
  if (pd->scale <= 0) pd->scale = 0.01 ;

  pdDrawBlueBox () ;
}




/*************************************************************************/

Array getPrinters (void)
{
  FILE *printcap ;
  enum {MAXC = 100} ;
  char buf[MAXC], *cp ;
  int n ;
  Array pl ;
  int c, i = 0;;


  pl = arrayCreate (10, char*) ;
  n = 0 ;
  if ((cp = getenv ("ACEDB_LPR")))
    {
      array (pl, n, char*) = (char*)messalloc (strlen(cp)+1) ;
      strcpy (arr(pl, n, char*), cp) ;
      n++ ;
    }

  if (
      (filCheckName("/etc/printcap", 0, "r") &&
       (printcap = filopen("/etc/printcap", 0, "r")))
      ||
      (filCheckName("/etc/printers.conf", 0, "r") &&
       (printcap = filopen("/etc/printers.conf", 0, "r")))
      )
    {
      while ((c = getc(printcap)) != EOF)
	{
	  if (c == '\\') 
	    while ((c == '\\' || c == ' ' || c == '\n' || c == '\t')
		   && c != EOF)
	      c = getc(printcap);
	  else if (c == ':' || c == '|' || c == '#' )
	    {
	      if ( c == ':' || c == '|')
		{
		  char *s = (char *) messalloc(i+1);
		  *s = 0;
		  array (pl, n, char*) = s;
		  strncat (s, buf, i) ;
		  i = 0;
		                            /* ignore blank entries */
		  if (strlen(g_strstrip(s)) > 0)
		    n++ ;
		  else
		    messfree(s);
		}
	      while (c != '\n' && c != EOF)
		if ((c = getc(printcap)) == '\\')
		  while ((c == '\\' || c == ' ' || c == '\n' || c == '\t') 
			 && c != EOF)
		    c = getc(printcap);
	    }
	  else if (i != MAXC && c != '\n' && c != '\t' && c != ' ')
	    buf[i++] = c;
	}

      filclose (printcap);
    }
  return pl ;
}

/*************************************************************************/

static void pdDrawBlueBox (void)
{ 
  int line ;
  float x1, y1, x2, y2, w, h ;
  PrintState pd ;

  pd = getPrintState("pdDrawBlueBox") ;

  line = pd->blueBoxLine ;

  graphActivate (pd->target) ;

  /* The arithmetic below should be bracketed....sigh...                     */
  if (pd->isRotate)
    {
      x1 = 2 + 0.2 ;
      y1 = line+11-8 + 0.1 ;
      w = gActive->uw / gActive->aspect * pd->scale ;
      h = gActive->uh * pd->scale ;
      x2 = x1 + (35.7/PAGEHEIGHT * w) ;
      y2 = y1 + (16.3/PAGEWIDTH * h) ;

      pd->pages = 1 + gActive->uh * pd->scale/PAGEWIDTH ;
     }
  else
    {
      x1 = 2+18-13 + 0.1 ;
      y1 = line + 0.1 ;
      w = gActive->uw / gActive->aspect * pd->scale ;
      h = gActive->uh * pd->scale ;
      x2 = x1 + (26.4/PAGEWIDTH * w) ;
      y2 = y1 + (22/PAGEHEIGHT* h) ;

      pd->pages = 1 + gActive->uh * pd->scale/PAGEHEIGHT ;
    } 

  graphActivate (pd->graph) ;

  if (pd->blueBox)
    {
      graphBoxClear (pd->blueBox) ;
      graphBoxClear (pd->blueBox+1) ;
      graphBoxClear (pd->blueBox+2) ;
      graphBoxClear (pd->blueBox+3) ;
    }

  pd->blueBox = graphBoxStart () ;
  graphLine (x1, y1, x2, y1) ;
  graphBoxEnd () ;
  graphBoxStart () ;
  graphLine (x2, y1, x2, y2) ;
  graphBoxEnd () ;
  graphBoxStart () ;
  graphLine (x1, y2, x2, y2) ;
  graphBoxEnd () ;
  graphBoxStart () ;
  graphLine (x1, y1, x1, y2) ;
  graphBoxEnd () ;
  graphBoxDraw (pd->blueBox, BLUE, TRANSPARENT) ;
  graphBoxDraw (pd->blueBox+1, BLUE, TRANSPARENT) ;
  graphBoxDraw (pd->blueBox+2, BLUE, TRANSPARENT) ;
  graphBoxDraw (pd->blueBox+3, BLUE, TRANSPARENT) ;

  sprintf (pd->pageText, "%d", pd->pages) ;

  graphBoxDraw (pd->pageBox, -1, -1) ;

  graphRedraw () ;
}




static MENUOPT* pdGetMenu (void)
{
  /* ACEDB-GRAPH INTERFACE: if acedb has registered a different print menu   */
  /* use that, otherwise use default internal one.                           */
  if (getGraphAcedbPDMenu() != NULL) 
    return (getGraphAcedbPDMenu()) ;
  else 
    return (pdMenu) ;
} /* pdGetMenu */



static void pdDraw(void)
{
  int graphWidth, graphHeight ;
  int boxp, boxc, boxg, boxa, boxm, boxl, box, b ;
  int line ;
  float dy = .1 ;
  double x  ;
  double y  ;
  int n=0, len, ix, iy ;
  float ux, uy, x0, y0, xmax ;
  char *ending ;
  PrintState pd ;


  pd = getPrintState("pdDraw") ;
  
  graphActivate (pd->graph) ;
  graphPop () ;
  
  graphClear() ;
  graphFitBounds (&graphWidth,&graphHeight) ;

  graphMenu (pdGetMenu());

  line = 1 ;

  if (pd->doCopy || pd->doMail || pd->doPrint)
    {
      graphButton (" OK ", pdOK, 2, line) ;
      graphButton ("Cancel", pdCancel, 8, line) ;
    }
  else
    {
      graphButton ("Cancel", pdCancel, 8, line) ;
    }
  line += 2 ;

  graphLine (0, line, graphWidth, line) ;

  line += 1 ;

  graphText ("Document Title :", 2, line) ;
  pd->titleBox = graphTextScrollEntry (pd->titleBuffer, 79, 40, 20,line,0) ;
  line += 2 ;
/* mhmp ex-ligne des formats */

  pd->printerBox = 0 ;
  pd->mailerBox = 0 ;
  pd->blueBox = 0 ;
  pd->pageBox = 0 ;
  pd->EPSFBox = 0 ;

  boxp = graphButton ("Print", printerButton, 2, line - dy) ;
  if (pd->doPrint)
    {
      graphBoxDraw (boxp, BLACK, LIGHTBLUE) ;
      graphText("Print Command:", 10, line) ;

	  x = x0 = 25; y = y0 = line - dy ;
      if (arrayMax(pd->printers))
	{
	  xmax = graphWidth ;
	  
	  graphTextInfo (&ix, &iy, &ux, &uy) ;
	  uy += YtoUrel(7) ; 
	  n = 0 ;
	  while (n < arrayMax(pd->printers))
	    { 
	      len = strlen (pd->ACEDB_LPR && n == 0 ? "ACE default" :
			    arr(pd->printers, n, char*)) ;
	      if (x + ux*len + XtoUrel(15) > xmax && x != x0)
		{ x = x0 ; y += uy ;}
	      box = graphBoxStart () ;
	      graphText (pd->ACEDB_LPR && n == 0 ? "ACE default" :
			 arr(pd->printers, n, char*),
			 x + XtoUrel(3), y + YtoUrel(2)) ;
	      graphRectangle (x, y, gBox->x2 + XtoUrel(3), gBox->y2) ;
	      graphBoxEnd () ;
	      x += ux*len + XtoUrel(15) ; 
	      ++n ;
	    }
	  pd->firstBox = box + 1 - n ;
	  graphBoxDraw (pd->firstBox + pd->printerNum, 
			BLACK, LIGHTBLUE) ;
	}
      line = y + dy ;
      line += 2 ;
      pd->printerBox = graphBoxStart() ;
      graphTextPtr (pd->printerBuffer, 10, line, 50) ;
      graphBoxEnd () ;
    }
  else
    {
      graphBoxDraw (boxp, BLACK, WHITE) ;
      b = graphBoxStart () ;

      graphText("Print Command:", 10, line) ;

      x = x0 = 25; y = y0 = line - dy ;
      if (arrayMax(pd->printers))
	{
	  xmax = graphWidth ;
	  
	  graphTextInfo (&ix, &iy, &ux, &uy) ;
	  uy += YtoUrel(7) ; 
	  n = 0 ;
	  while (n < arrayMax(pd->printers))
	    { 
	      len = strlen (pd->ACEDB_LPR && n == 0 ? "ACE default" :
			    arr(pd->printers, n, char*)) ;
	      if (x + ux*len + XtoUrel(15) > xmax && x != x0)
		{ x = x0 ; y += uy ;}
	      box = graphBoxStart () ;
	      graphText (pd->ACEDB_LPR && n == 0 ? "ACE default" :
			 arr(pd->printers, n, char*),
			 x + XtoUrel(3), y + YtoUrel(2)) ;
	      graphRectangle (x, y, gBox->x2 + XtoUrel(3), gBox->y2) ;
	      graphBoxEnd () ;
	      graphBoxDraw (box, GRAY, WHITE) ;
	      x += ux*len + XtoUrel(15) ; 
	      ++n ;
	    }
	  pd->firstBox = box + 1 - n ;
	}
      line = y + dy ;
      line += 2 ;
      graphText (pd->printerBuffer, 10, line) ;

      graphBoxEnd () ;
      graphBoxDraw (b, GRAY, WHITE) ;
      graphBoxMenu (b, pdGetMenu());
    }


  line += 2 ;
  boxm = graphButton ("Mail", mailerButton, 2, line - dy) ;
  if (pd->doMail)
    { 
      graphBoxDraw (boxm, BLACK, LIGHTBLUE) ;
      graphText("Address:", 10, line) ;
      line += 2 ;
      pd->mailerBox = graphTextScrollEntry(pd->mailerBuffer,
					   60, 50, 10, line, 0) ;
    }
  else
    {
      graphBoxDraw (boxm, BLACK, WHITE) ;
      b = graphBoxStart () ;
      graphText("Address:", 10, line) ;
      line += 2 ;
      graphText(pd->mailerBuffer, 10, line) ;
      graphBoxEnd () ;
      graphBoxDraw (b, GRAY, WHITE) ;
      graphBoxMenu (b, pdGetMenu());
    }
  
  line += 2 ;
  boxc = graphButton ("Copy", copyButton, 2, line - dy) ;

  switch (pd->style)
    {
    case 'c':
      /* fall thru */
    case 'p':
      ending = "ps" ;
      break ;
    case 'g':
      ending = "gif" ;
      break ;
    case 'a':
      ending = "txt" ;
      break ;
    }
  if (pd->doCopy)
    {
      graphBoxDraw (boxc, BLACK, LIGHTBLUE) ;
      graphText("keep a copy in file:", 10, line) ;
      graphButton("File chooser", fileButton, 32, line - dy) ;
      line += 2 ;
      sprintf (pd->copyBuffer, "%s/%s.%s", 
	       pd->dirBuffer, pd->filBuffer, ending) ;
      pd->fileBox = graphTextScrollEntry(pd->copyBuffer,
					 128, 50, 10, line, copyNameEnd) ;
    }
  else
    {
      graphBoxDraw (boxc, BLACK, WHITE) ;
      b = graphBoxStart () ;
      graphText("keep a copy in file:", 10, line) ;
      graphText ("File chooser", 32, line - dy) ;
      line += 2 ;
      sprintf (pd->copyBuffer, "%s/%s.%s", 
	       pd->dirBuffer, pd->filBuffer, ending) ;
      graphText (pd->copyBuffer,10, line) ;
      graphBoxEnd () ;
      graphBoxDraw (b, GRAY, WHITE) ;
      graphBoxMenu (b, pdGetMenu());
    }

  graphTextEntry (pd->titleBuffer, 0, 0, 0, 0) ;


  /* Image format options.                                                   */
  line += 2 ;						    /* mhmp 28/04/97 */
  graphText ("Format:", 2, line - dy) ;
  boxp = graphButton ("PostScript", psButton, 11, line - dy) ;
  boxc = graphButton ("Color PostScript", cpsButton, 23, line - dy) ;
  boxg = graphButton ("GIF", gifButton, 41, line - dy) ;
  boxa = graphButton ("Text Only", asciiButton, 46, line - dy) ;

  if (pd->style == 'p')
    graphBoxDraw (boxp, BLACK, LIGHTBLUE) ;
  else if (pd->style == 'c')
    graphBoxDraw (boxc, BLACK, LIGHTBLUE) ;
  else if (pd->style == 'g')
    graphBoxDraw (boxg, BLACK, LIGHTBLUE) ;
  else if (pd->style == 'a')
    graphBoxDraw (boxa, BLACK, LIGHTBLUE) ;

  line += 2 ;
  graphText ("Options:", 2, line - dy) ;
  pd->EPSFBox = graphButton ("EPSF", EPSFButton, 11, line - dy) ;

  if (pd->style == 'p' || pd->style == 'c')
    {
    if (pd->doEPSF == OPTION_ON)
      graphBoxDraw (pd->EPSFBox, BLACK, LIGHTBLUE) ;
    else
      graphBoxDraw (pd->EPSFBox, BLACK, WHITE) ;
    }
  else
    graphBoxDraw (pd->EPSFBox, GRAY, WHITE) ;



  /* Outline of image on the paper.                                          */
  line += 2 ;
  graphLine (0, line, graphWidth, line) ;
  line += 1.5 ;
  graphColor (GRAY) ;
  graphFillRectangle (2, line, 2+36, line+22) ;

  if (pd->isRotate)
    {
      /* landscape */
      graphColor (WHITE) ;
      graphFillRectangle (2, line+11-8, 2+36, line+11+8) ;
      graphColor (BLACK) ;
      graphRectangle (2, line+11-8, 2+36, line+11+8) ;
    }
  else
    {
      /* portrait */
      graphColor (WHITE) ;
      graphFillRectangle (2+18-13, line, 2+18+13, line+22) ;
      graphColor (BLACK) ;
      graphRectangle (2+18-13, line, 2+18+13, line+22) ;
    }
  
  boxp = graphButton ("Portrait", portraitButton, 44, line+5) ;
  boxl = graphButton ("Landscape", landScapeButton, 44, line+7) ;
  graphBoxDraw (boxp, BLACK, pd->isRotate ? WHITE : LIGHTBLUE) ;
  graphBoxDraw (boxl, BLACK, pd->isRotate ? LIGHTBLUE : WHITE) ;

  graphColor (BLACK) ;
  graphRectangle (2, line, 2+36, line+22) ;

  graphText ("Scale :", 44, line+10) ;
  memset (pd->scaleText, 0, 10) ;
  if (pd->scale < 10.0) strcat (pd->scaleText, " ") ;
  strcat (pd->scaleText, messprintf ("%5.3f", pd->scale)) ;
  pd->scaleText[5] = 0 ;
  graphTextEntry (pd->scaleText, 6, 44+8, line+10, scaleTextEnd) ;
  graphText ("Pages : ", 44, line+13) ;



  pd->firstScaleBox = graphBoxStart () ;
  graphRectangle (44+8, line+9, 44+10, line+10) ;
  graphLine (44+8, line+10, 44+9, line+9) ;
  graphLine (44+9, line+9, 44+10, line+10) ;
  graphBoxEnd () ;
  graphBoxStart () ;
  graphRectangle (44+8, line+11, 44+10, line+12) ;
  graphLine (44+8, line+11, 44+9, line+12) ;
  graphLine (44+9, line+12, 44+10, line+11) ;
  graphBoxEnd () ;

  graphBoxStart () ;
  graphRectangle (44+11, line+9, 44+12, line+10) ;
  graphLine (44+11, line+10, 44+11.5, line+9) ;
  graphLine (44+11.5, line+9, 44+12, line+10) ;
  graphBoxEnd () ;
  graphBoxStart () ;
  graphRectangle (44+11, line+11, 44+12, line+12) ;
  graphLine (44+11, line+11, 44+11.5, line+12) ;
  graphLine (44+11.5, line+12, 44+12, line+11) ;
  graphBoxEnd () ;

  graphBoxStart () ;
  graphRectangle (44+12, line+9, 44+13, line+10) ;
  graphLine (44+12, line+10, 44+12.5, line+9) ;
  graphLine (44+12.5, line+9, 44+13, line+10) ;
  graphBoxEnd () ;
  graphBoxStart () ;
  graphRectangle (44+12, line+11, 44+13, line+12) ;
  graphLine (44+12, line+11, 44+12.5, line+12) ;
  graphLine (44+12.5, line+12, 44+13, line+11) ;
  graphBoxEnd () ;


  sprintf (pd->pageText, "%d", pd->pages) ;
  pd->pageBox = graphBoxStart () ;
  graphTextPtr (pd->pageText, 44+10, line+13, 3) ;
  graphBoxEnd () ;

  pd->blueBoxLine = line ;

  pdDrawBlueBox () ;		/* does a redraw */
}


/*************************************************************************/
static PrintState pdInit(void)
{ 
  PrintState result = NULL ; 
  char *cp ;

  result = (PrintState) messalloc (sizeof(struct PrintStateStruct)) ;
  result->magic = &PrintState_MAGIC ;
  result->graph = 0 ;

  /* ACEDB-GRAPH INTERFACE:                                                  */
  /* Acedb provides colour printing style.                                   */
  /* This should be replaced with a straight call to the graph package to    */
  /* set colour, printer fonts etc.                                          */
  if (getGraphAcedbStyle() != NULL)
    {
    result->style = (getGraphAcedbStyle())() ; 
    strcpy (result->mailerBuffer, getLogin(TRUE));
    }
  else
    {
    result->style = 'p' ;
    strcpy (result->mailerBuffer, "");
    }

  if (result->style == 'p' || result->style == 'c')
    result->doEPSF = OPTION_ON ;
  else
    result->doEPSF = OPTION_OFF ;


  if ((cp = getenv ("ACEDB_LPR")))
    result->ACEDB_LPR = TRUE ;
  else
    result->ACEDB_LPR = FALSE ;

  result->printers = getPrinters() ;

  if (arrayMax(result->printers))
    result->doPrint = OPTION_ON ;
  else
    result->doMail = OPTION_ON ; /* without attached printers: mail=default */

  if (result->ACEDB_LPR)
    strcpy (result->printerBuffer, arr(result->printers, 0, char*)) ;
  else if (arrayMax(result->printers))
    sprintf (result->printerBuffer, "lpr -P%s",
	     arr(result->printers, 0, char*)) ;
  else				/* default if everything fails */
    strcpy (result->printerBuffer, "lpr") ;

  result->printerNum = 0 ;

  if (filCheckName ("PS", "", "wd"))
    {
      strcpy (result->dirBuffer, cp = filGetName ("PS", "", "wd", 0)) ;
      messfree(cp);
    }
  else if (getenv("PWD"))
    strcpy (result->dirBuffer, getenv("PWD")) ;
  else if (!getwd (result->dirBuffer))
    strcpy (result->dirBuffer, ".") ;

  return result ;
}



void graphPrint (void) 
{
  char *cp, fb[FIL_BUFFER_SIZE] ;
  Graph targetGraph ;
  char *targetGraphName ;
  char *ending ;
  static int nn = 1 ;
 
  /* First time we are called create the control block.                      */
  /* (this is the only routine we refer to lastPd in, this should make it    */
  /* easier to prepare for context/thread like implementation)               */
  if (lastPd == NULL)
    lastPd = pdInit() ;

  /* Is there a print window already displayed ??                            */
  if (lastPd->graph)
    {
      messout ("Print selector is already open on another graph, sorry") ;
      return ;
    }


  strcpy (fb, (cp = graphHelp(0)) ? cp : "acedb" ) ;

  /* Check all is OK with the graph to be printed.                           */
  targetGraph = gActive->id ;
  targetGraphName = gActive->name ;
  if (!graphExists (targetGraph))
    {
      messout ("The graph you wish to print seems to have disappeared from the screen,"
               "please try again") ;
      return ;
    }

  /* The test below is grim, it means that the print graph has disappeared   */
  /* without us noticing....                                                 */
  if (!graphExists(lastPd->graph))
    { 
      /* calculate the size of the window, depending on the printers
	 that are all drawn to fit on the screen, we load the printers
	 and increase the window size by 20 pixels for each additional
	 (more than 1) line of printer buttons, the printer buttons
	 are drawn into a window 196 pixels wide (from character 
	 position 25 to 62 on the text window) */
      Array tmpPr  = getPrinters() ;
      float line = 600, x=0, xmax=296;
      int n=0, len ;
      
      while (n < arrayMax(tmpPr))
	{ 
	  len = strlen (getenv ("ACEDB_LPR") && n == 0 ?
			"ACE default" :	arr(tmpPr, n, char*)) ;
	  if (x + 8*len + 15 > xmax && x != 0)
	    { x = 0 ; line += 20 ; }
	  x += 8*len + 15 ; 
	  ++n ;
	}
      arrayDestroy (tmpPr) ;

      lastPd->graph = graphCreate (TEXT_FIT,
			       targetGraphName == 0 ? "Print/Mail Selection" :
			       messprintf ("Print/Mail Selection  ( %s )", targetGraphName),
			       .4, .25, 500.0/900.0, (line)/900.0) ;

      graphRegister (DESTROY, pdDestroy) ;
      graphRegister (RESIZE, pdDraw) ;
      graphRegister (PICK, pdPick) ;

      /* ACEDB-GRAPH INTERFACE: if acedb has registered a different print    */
      /* menu use that, otherwise use default internal one.                  */
      if (getGraphAcedbPDMenu() != NULL) graphMenu(getGraphAcedbPDMenu()) ;
      else graphMenu (pdMenu) ;

      graphHelp ("Printer_Selection") ;

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
      graphEnableBlock () ;				    /* EG: don't know why this is
							       here... */
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

      graphAssociate (&GRAPH2PrintState_ASSOC, lastPd) ;
    }
  else
    { graphActivate (lastPd->graph) ;
      graphPop () ;
    }

  strcat (fb, messprintf(".%d", nn++)) ;
  strcpy (lastPd->filBuffer, fb) ;

  switch (lastPd->style)
    {
    case 'c':
      /* fall thru */
    case 'p':
      ending = "ps" ;
      break ;
    case 'g':
      ending = "gif" ;
      break ;
    case 'a':
      ending = "txt" ;
      break ;
    }
  sprintf (lastPd->copyBuffer, "%s/%s.%s", lastPd->dirBuffer, lastPd->filBuffer, ending) ;

  lastPd->target = targetGraph ;
  memset (lastPd->titleBuffer, 0, 80) ;

  /* srk - TODO provide letter size option. */
  graphPSdefaults (lastPd->target, TRUE, 
		   &lastPd->isRotate, &lastPd->scale, &lastPd->pages) ;
  

  graphActivate (lastPd->graph) ;

  pdDraw () ;

  messStatus ("Please Finish Print") ;
  
  if (graphLoop (TRUE))		/* blocking */
    pdExecute();
  
  if (graphActivate (lastPd->graph))
    graphDestroy ();

  graphActivate(lastPd->target); /* status quo ante */

  return ;
}
#endif /* !__CYGWIN__ */

void graphBoundsPrint (float uw, float uh, void (*drawFunc)(void))
{
  int olduw = gActive->uw ;
  int oldw = gActive->w ;
  int olduh = gActive->uh ;
  int oldh = gActive->h ;

  gActive->uw = uw ; gActive->w = gActive->uw * gActive->xFac ;
  gActive->uh = uh ; gActive->h = gActive->uh * gActive->yFac ;

  if (drawFunc)
    (*drawFunc) () ;

  graphPrint () ;

  gActive->uw = olduw ;
  gActive->w = oldw ;
  gActive->uh = olduh ;
  gActive->h = oldh ;

  return ;
}

/*********************** end of file ***************************/
 
 
 
 
 
