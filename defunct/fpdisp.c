/*  File: pmafp.c
 *  Author: Danielle et Jean Thierry-Mieg (mieg@mrc-lmb.cam.ac.uk)
 *  Copyright (C) J Thierry-Mieg and R Durbin, 1994
 *-------------------------------------------------------------------
 * This file is part of the ACEDB genome database package, written by
 * 	Richard Durbin (MRC LMB, UK) rd@mrc-lmb.cam.ac.uk, and
 *	Jean Thierry-Mieg (CRBM du CNRS, France) mieg@kaa.cnrs-mop.fr
 *
 * Description:
 * Exported functions:
 * HISTORY:
 * Last edited: Jan 15 19:13 1994 (rd)
 * Created: Thu Jan  6 11:12:36 1994 (mieg)
 *-------------------------------------------------------------------
 */

/* $Id: fpdisp.c,v 1.1 1994-01-20 21:55:56 acedb Exp $ */

#include "acedb.h"
#include "keyset.h"
#include "lex.h"
#include "sysclass.wrm"
#include "classes.wrm"
#include "tags.wrm"
#include "systags.wrm"
#include "bs.h"
#include "plot.h"
#include "session.h"
#include "graph.h"
static Array bands = 0 ;
static void   fpRead(void) ;
 
static int rounding = 1 ;

/******************************************/

static void  fpHisto(void)
{
  Array histo ;
  int i ;
  
  fpRead() ;
  if(!bands) 
    return ;
  
  histo  = arrayCreate(5000,int);
  i = arrayMax(bands);
  while(i--)
    array(histo,arr(bands,i,short),int)++;
  plotHisto ("Bands distribution", histo);
}
 
/******************************************/
 
static void   fpRead(void)
{
  static char fileName[FIL_BUFFER_SIZE] ;
  static char dirName[DIR_BUFFER_SIZE] ;
  static BOOL firstpass = TRUE ;
  int i = 0 , n ;
  void
    *vp = &n;
  unsigned int
    nb = 20000, nr,
    size = sizeof(short);
  
  FILE
    *fpfile, *corfile;
   
  if (arrayExists(bands))
    return ;
  
  if (firstpass)
    { firstpass = FALSE ;
      strcpy(dirName, filsetdir(0)) ;
      strcpy(dirName, "940103");
    }
  
  corfile = filqueryopen(dirName, fileName,"cor", "r","Finger-print rawdata file") ;
  if (!corfile)
    { messout("Cannot find the finger-print raw dat cor file, sorry") ;
      return ;
    }
  fclose(corfile) ;

  bands = arrayCreate(nb,short);
  if (!filexists(messprintf("%s/%s", dirName, fileName),"bnd"))
    { 
      messout("I reformat the ascii file %s.cor into a file of int  %s.bnd",
	      fileName, fileName);
      if (   !(corfile= filopen(messprintf("%s/%s", dirName, fileName),"cor","r"))
	  || !(fpfile= filopen(messprintf("%s/%s", dirName, fileName),"bnd","wb")))
	return ;
      
      array(bands,(++i)*nb,short) = 0; /* to create space */
      while (freeread(corfile))
	if(freeint(&n))
	  array(bands,i++,short) = n ;
	else
	  {
	    messout("Error parsing line %d of cor file : %s",
		    i, freeword());
	    break;
	  }
      
      fclose(corfile) ;
      fwrite(arrp(bands,0,short),size,arrayMax(bands),fpfile);
    }
  
  else
    { fpfile= filopen(messprintf("%s/%s", dirName, fileName),"bnd","rb") ;
      do
	{
	  array(bands,(++i)*nb,short) = 0; /* to create space */
	  vp = &arr(bands,nb*(i-1),short); /* possible relocation */
	  fprintf(stderr,"I read %d bands\n",i*nb);
	}
      while ((nr=fread(vp,size,nb,fpfile))==nb);
      
      arrayMax(bands) -= nb - nr; /* artificial space removed */
    }
  
  messout("I read %d gel bands", arrayMax(bands)) ;
  fpHisto() ;
  fclose(fpfile);
  return ;
}

/*******************************************************************/
/********************  public routines   *****************************/
/*************************************************************/

Array fingerPrintData(KEY clone)
{
  int from, i, n , *ip ;
  static mm = 0 ;
  Array fp = 0 ;
  OBJ Clone ;


  if(class(clone) != _VClone
     || !(Clone = bsCreate(clone)) )
    return 0 ;

  
  if(bsGetData(Clone,_Band_Values,_Int,&n))
    {
      fp = arrayCreate(20, int) ;
      i = 0 ;
      do
	{ array(fp, i++, int) = n ;
	} while (bsGetData(Clone, _bsRight, _Int, &n)) ;
    }
  else if(bsGetData(Clone,_Bands,_Int,&from)
	  && bsGetData(Clone,_bsRight,_Int,&n))
    {
      if(!bands)
	fpRead() ;
	
      if (bands)
	if(from+n <= arrayMax(bands))
	  { fp = arrayCreate(n,int) ;
	    while(n--)
	      array(fp,n,int) = array(bands,from+n,short) ;
	    if (arrayMax(fp) && isWriteAccess())
	      { bsDestroy(Clone) ;
		if (Clone = bsUpdate(clone))
		  { i = arr(fp, 0, int) ;
		    bsAddData (Clone, _Band_Values, _Int, &i) ;
		       /* first value allready stored */
		    for (n = 1 ; n < arrayMax(fp) ; n++)
		      { i = arr(fp, n, int) ;
			bsAddData(Clone, _bsRight, _Int, &i) ;
		      }
		    bsSave(Clone) ;
		    Clone = 0 ;
		  }		
	      }
	  }
	else
	  messout
	    ("Clone %s\n Band %d > Max %d of data file",
	     name(clone), from+n, arrayMax(bands)) ;
	  
    }
  else
    if (mm++ < 3)
      messout("Sorry, no finger print data known about clone %s",
	      name(clone)) ;

  if (Clone)
    bsDestroy(Clone) ;
  fprintf(stderr, "\nClone %s\n", name(clone)) ;
  
  if (rounding < 1) rounding = 1 ;
  if (fp)
    for (n = 0 , ip = arrp(fp, 0, int); n < arrayMax(fp) ; n++, ip++)
      { i = *ip / rounding ; 
	*ip = i * rounding ;
      }
  
  return fp ;
}

/*************************************************************/
/*************************************************************/

  /* donnees is a boolean matrix clones/lengths */
  /* donnees must be created as array(unsigned char before call */
void pmapCptGetData (Array donnees, KEYSET clones, KEYSET bands, 
		     int *nClones, int *nBands)
{
  KEY key = 0 , kb ;
  Array aa, aTrans, fp ;
  Associator assB ;
  void *vp, *dummy ;
  int  i, j, nB = 0 , nC, nBmax = 0 , nCmax ;

  if (!keySetActive(&aa, &dummy))
    { messout("First select a keyset containing clones") ;
      return ;
    }
  j = 0 ;
 
  for (i=0 ; i < keySetMax(aa) ; i++)
    { key = keySet(aa,i) ;
      if (class(key) == _VClone)
	keySet(clones,j++) = key ;
    }
  nCmax = keySetMax(clones) ;
  if (!nCmax)
    messout("First2 select a keyset containing clones") ;
  
  assB = assCreate() ;
  aTrans = arrayCreate(100,unsigned char) ;
                              /* Creation de la matrice transposee */
  for (nC = 0 ; nC < keySetMax(clones) ; nC++) 
    if (fp = fingerPrintData(keySet(clones, nC)))
      { j = arrayMax(fp) ;
	while (j--)
	  { lexaddkey(messprintf("%d", arr(fp, j, int)), &kb, _VCalcul) ;
	    if (!assFind ( assB, (void*)kb, &vp))
	      { vp = (void*) nBmax ;
		assInsert(assB, (void*)kb, vp) ;
		nB = nBmax ;
		keySet(bands, nBmax++) = kb ;
	      }
	    else
	      nB = (int) vp ;
	    array(aTrans, nB * nCmax + nC, unsigned char) = 1 ; /* found */
	  }
	arrayDestroy(fp) ;
      }
  
  i = nBmax * nCmax ;
  array(donnees, i-1, unsigned char) = 0 ;
  while(i--)/* Transposition de la matrice, et decalage, 1 = absent, 2 = present */
    arr(donnees, i, unsigned char) = array (aTrans, i/nBmax  + 
					    (i % nBmax) * nCmax, unsigned char) + 1 ;
  arrayDestroy(aTrans) ;
  assDestroy(assB) ;

  *nClones = keySetMax(clones) ;
  *nBands = keySetMax(bands) ;
      
}

/***************************************************************/
/***************************************************************/


#include "map.h"
struct LookStruct { int magic ; 
		  MAP map ;
		  Graph graph ; } ;
typedef struct GelStruct { KEY clone ;
		   Array fp, bp ; 
		   int max ;
		  } GEL ;

#include "display.h"

static fpGraph = 0 ;
static BOOL isBp = FALSE ;
static MAP fpMap = 0 ;
static Array fpGels = 0 ;
static Array fpBoxIndex = 0 ;
static int fpActiveBox = 0, minLiveBox = 0 ;
static int fpActiveGel = 0 ;
static char fpRoundingBuffer[5] ;
static void fpDraw(void) ;
void fpDisplay(KEY clone) ;

static void fpDestroy(void)
{ int i ;
  if (graphExists(fpGraph))
    {
      fpGraph = 0 ;
      i = arrayMax (fpGels) ;
      while(i--)
	{ arrayDestroy(arrp(fpGels, i, GEL)->fp) ;
	  arrayDestroy(arrp(fpGels, i, GEL)->bp) ;
	}
      arrayDestroy(fpGels) ;
      arrayDestroy(fpBoxIndex) ;
      mapDestroy(fpMap) ;
    }     
}


static void fpRmClone(void)
{ int n , i ;
  if (!graphActivate(fpGraph))
    return ;
  n = fpActiveGel ;
  if (n >= arrayMax(fpGels))
    return ;
  arrayDestroy(arr(fpGels, n, GEL).fp) ;
  arrayDestroy(arr(fpGels, n, GEL).bp) ;
  for (i = n ; i < keySetMax(fpGels) - 1 ; i++)
      array(fpGels, i, GEL) = array(fpGels, i + 1, GEL) ; 
  arrayMax(fpGels)-- ;
  
  fpDraw() ;
}

static void fpUnBlock (void)
{
  displayUnBlock() ;
}

static void fpSwitch (void)
{ int  n = 0 , max ;
  GEL gel ;
   
  n = fpActiveGel ;
  max = arrayMax(fpGels) ;
  if (n >= max)  
    return ;
  if (n == max - 1)
    if (n>0) n-- ; else return ;
  gel = array(fpGels, n , GEL) ;
  array(fpGels, n , GEL) =  array(fpGels, n + 1, GEL) ;
  array(fpGels, n + 1, GEL) = gel ;
  fpDraw() ;  
}

static void fpDoAddClone(KEY clone)
{ 
  fpDisplay (clone) ;
  displayRepeatBlock () ;
}

static void fpAddClone(void)
{ 
  if (!graphActivate(fpGraph))
    return ;
  graphRegister (MESSAGE_DESTROY, fpUnBlock) ;
  displayBlock (fpDoAddClone,
	"Double-clicking a clone will show its finger-printing."
	"This will continue until you remove this message") ;
}

static void fpRounding(char *cp) 
{ int i, level ;
  
  level = freesettext(cp,"") ;
  freecard (level) ;
  if (freeint(&i) && (i>0))
    rounding = i ;
  freeclose(level) ;
  sprintf(fpRoundingBuffer, "%d", rounding) ;
  fpDraw() ;
}

static Array fpLength2bp(Array fp)
{ Array bp = arrayCreate(arrayMax(fp), int) ;
  double x ;
  int i = arrayMax(fp) ;
	   
  while(i--)
    { x = arr(fp, i, int) ;
      /* excellent Fit mathematica sur 79 data d'etalonage  */
      x = log(x) ;
      x = 133.164 - 54.4586 * x + 7.9092 * x * x - .389621 * x * x * x ;
      x = exp(x) ;
      array(bp, i, int) = x ;
    }
  return bp ;
}


static void fpTransform(void)
{ int i, n = keySetMax(fpGels), max ;
  double x ;
  Array fp, bp ;

  isBp = !isBp ;
  
  max = 0 ;
  n = arrayMax(fpGels) ;
  while(n--)
    { fp = isBp ?
	arrp(fpGels, n ,GEL)->fp : 	arrp(fpGels, n ,GEL)->bp ;
      i = fp ? arr(fp, arrayMax(fp) - 1 , int) : 0 ;
      if (max < i)
	max = i ;
    }
  if (!max)
    max = 100 ;
  fpMap->max = max ;
  fpDraw() ;
}

/***********************************************************/

static void fpMainLines (LOOK look, float *offset)
{
  float y ;
  int i ;

  graphTextHeight (0.75) ;
  for (i = 0 ; i < fpMap->max ; i += 200)
    { y = MAP2WHOLE(fpMap, i) ;
      graphText (messprintf("%d", i),*offset + 0.5,y-0.25) ;
    }
  graphTextHeight (0) ;
  *offset += 4 ;
}

/*************************************/

static void fpPick (int box,  double x , double y) 
{ int n , nn ;
  KEY clone ;
  if (!box)
    return ;
  if (box == fpMap->cursor.pickBox)
    graphBoxDrag (fpMap->cursor.box, mapCursorDrag) ;
  else if (box == fpMap->thumb.box)
    graphBoxDrag (fpMap->thumb.box, mapThumbDrag) ;
  else if (box > minLiveBox)
    { n = box - minLiveBox - 1 ;
      nn = n & 1 ; n /= 2 ;
      if (n >= arrayMax(fpGels))
	return ;
      clone = array(fpGels, n, GEL).clone ;
      if (nn)
	display(clone, 0, 0) ;
      if (fpActiveGel != n)
	{ /* graphBoxDraw(minLiveBox + 2 + 2*fpActiveGel,
		       BLACK, WHITE) ;
	  graphBoxDraw(minLiveBox + 2 + 2*fpActiveGel,
		       BLACK, WHITE) ;
	  */
	  fpActiveGel = n ;
	  fpDraw() ;
	}
    }
}

static void fpDrawGels(LOOK look, float *offset)
{ float x = *offset, y, dx ;
  Array fp ;
  int i , j, box, max ;

  max = arrayMax(fpGels) ;
  if (!max)
    return ;
  dx = (graphWidth - x - 6)/(1.2 * max) ;
  minLiveBox= graphBoxStart() ;
  graphBoxEnd() ;
  for (i = 0 ; i < arrayMax(fpGels) ; i++)
    { graphBoxStart() ;
      if (isBp)
	fp = array(fpGels, i, GEL).bp ;
      else
	fp = array(fpGels, i, GEL).fp ;
      j = arrayMax(fp) ;
      while(j--)
	{ y = array(fp, j, int) ;
	  y = MAP2GRAPH(fpMap, y) ;
	  if (y > topMargin + 2)
	    graphLine(x , y , x + dx, y) ;
	}
      graphBoxEnd() ;
      box = graphBoxStart() ;
      graphText (name(array(fpGels, i, GEL).clone), x , topMargin) ;
      graphBoxEnd() ;
      if (i == fpActiveGel)
	graphBoxDraw(box, BLACK, LIGHTBLUE) ;
      x += dx * 1.2 ;
    }
  graphText(isBp ? "Base Pairs" : "mm/10", *offset + 1 , graphHeight - 2) ;
  *offset = x ;
}

static MapColRec fpCols[] =
{ 
  TRUE, "Main Bands", fpMainLines,
  TRUE, "Locator", mapShowLocator,
  TRUE, "Scale", mapShowScale,
  TRUE, "Bands", fpDrawGels,
    0, 0, 0 } ;

static MENUOPT fpMenu[] = {
  graphDestroy, "Quit",
  help, "Help",
  graphPrint, "Print Screen",
  mapPrint,"Print Whole Map",
  fpRmClone, "Remove Clone",
  fpAddClone, "Add Clone",
  fpSwitch, "Switch",
  mapColControl, "Columns",
  0, 0 } ;

static MENUOPT buttonOpts[] = {
  mapWhole, "Whole", 
  mapZoomIn, "Zoom In",
  mapZoomOut, "Zoom Out",
  fpRmClone, "Remove Clone",
  fpAddClone, "Add Clone",
  fpSwitch, "Switch",
  fpTransform, "millimetre/10 <-> base pairs",
  0, 0 } ;

void fpDraw(void)
{ float x1, x2, y1, y2 ;
  int n ;

  if (!graphActivate(fpGraph))
    return ;
  
  n = keySetMax(fpGels) ;
  
  graphClear () ;

  graphFitBounds (&graphWidth,&graphHeight) ;
  
  halfGraphHeight = 0.5 * (graphHeight - topMargin) ; 

  if (graphHeight < 10 + topMargin)
    { messout ("Sorry, this window is too small for a finger print data") ;
      return ;
    }

  fpBoxIndex = arrayReCreate (fpBoxIndex,50,int) ;
  fpActiveBox = 0 ;

  graphButtons(buttonOpts, 2, 2, graphWidth * .9) ;
  graphText("Rounding :", 10, 1) ;
  graphTextEntry(fpRoundingBuffer, 4, 22, 1, fpRounding) ;
  graphBoxDim (0, &x1, &y1, &x2, &y2) ;
  topMargin = y2 + 1 ;
  mapDrawColumns (fpMap) ;
  graphMenu (fpMenu) ;
  
  graphRedraw () ;
}
  
void fpDisplay (KEY clone)
{ int n ;
  Array fp = fingerPrintData(clone) ;
  
  if (!fp || !arrayMax(fp))
    return ;
  
  if (!graphExists(fpGraph))
    { fpGraph = graphCreate (TEXT_FIT,"Finger Printing", .2, .2, .3, .8) ;
      if (!fpGraph)
	return ;
      isBp = FALSE ;
      fpMap = mapCreate(fpCols, fpDraw) ;
      mapCursorCreate(fpMap, 1, 0) ; 
      graphRegister(DESTROY, fpDestroy) ;
      graphRegister(RESIZE, fpDraw) ;
      graphRegister(PICK, fpPick) ;
      graphAssociate(MAP_LOOK, 0) ;
      graphMenu(fpMenu) ;
      fpGels = arrayCreate(12, GEL) ;
      array(fpGels, 0, GEL).clone = clone ;
      fpActiveGel = 0 ;
      array(fpGels, 0, GEL).fp = fp ;
      array (fpGels, 0, GEL).bp = fpLength2bp(fp) ;
      fpMap->mag = 1 ;
      fpMap->centre = 500 ;
      sprintf(fpRoundingBuffer, "%d", rounding) ;
    }
  else
    { graphActivate (fpGraph) ;
      graphPop() ;
      n = keySetMax(fpGels) ;
      while(n--)
	if (arrp(fpGels, n, GEL)->clone == clone)
	  return ;
      n = keySetMax(fpGels) ;	
      fpActiveGel = n ;  
      arrayp(fpGels, n, GEL)->clone = clone ;
      arrp(fpGels, n, GEL)->fp = fp ;
      array (fpGels, n, GEL).bp = fpLength2bp(fp) ;
    }
  
  fpMap->min = 0 ;
  n = arrayMax(fp) ;
  while(n--)
    if (arr(fp, n, int) > fpMap->max)
      fpMap->max = arr(fp, n, int) ;

  graphFitBounds (&graphWidth,&graphHeight) ;
  halfGraphHeight = 0.5 * (graphHeight - topMargin) ;

  fpMap->centre = (fpMap->max + fpMap->min) / 2 ;  
  fpMap->mag = (graphHeight - topMargin - 5) /
    			(1.05 * (fpMap->max - fpMap->min)) ;

  fpDraw() ;
}
  
  
 

