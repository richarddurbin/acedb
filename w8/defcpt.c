/*  File: defcpt.c
 *  Author: Ulrich Sauvage (ulrich@kaa.crbm.cnrs-mop.fr)
 *  Copyright (C) J Thierry-Mieg and R Durbin, 1994
 *-------------------------------------------------------------------
 * This file is part of the ACEDB genome database package, written by
 * 	Richard Durbin (MRC LMB, UK) rd@mrc-lmba.cam.ac.uk, and
 *	Jean Thierry-Mieg (CRBM du CNRS, France) mieg@kaa.cnrs-mop.fr
 *
 * Description:
 * Exported functions:
 * HISTORY:
 * Last edited: May  6 10:52 2003 (edgrif)
 * * Oct  9 10:09 2000 (edgrif): Fleshed out Simons init change, its
 *              also needed for Linux and in several routines not just defCptOpen()
 *	srk	static defCptOut initialized in defCptOpen() rather than file scope
 *	srk	return value for defCptDoJoinDiagonal
 * * Jul 24 14:19 1996 (ulrich) 
 * * Apr 16 19:01 1996 (ulrich)
 * Created: Tue Aug 30 13:09:34 1994 (ulrich)
 * CVS info:   $Id: defcpt.c,v 1.49 2003-05-06 13:13:52 edgrif Exp $
 *-------------------------------------------------------------------
 */

/*

#define CHRONO
#define ARRAY_CHECK 
#define MALLOC_CHECK
*/

#include <wh/acedb.h>
#include <wh/aceio.h>
#include <wh/command.h>

#ifndef NON_GRAPHIC
#include <wh/display.h>
#include <wh/chrono.h>
#include <wh/plot.h>
#endif /* !NON_GRAPHIC */

#include <wh/status.h>		/* for tStatus */
#include <wh/lex.h>
#include <wh/bs.h>
#include <wh/a.h>
#include <wh/dna.h>
#include <whooks/sysclass.h>
#include <whooks/classes.h>
#include <whooks/systags.h>
#include <whooks/tags.h>
#include <wh/pick.h>
#include <wh/topology.h>
#include <wh/query.h>
#include <wh/bump.h>
#include <wh/interval.h>
#include <wh/dnaalign.h>
#include <wh/fingerp.h>
#include <wh/session.h>
#include <wh/parse.h>
#include <wh/dbpath.h>

/***************************************************************/

typedef struct { int g1, g2 ; } GENE_PAIR ;
typedef struct { KEY key ; int tai ; } KEY_INT ;
typedef struct { KEY gene, ch ; float p ; } GMAPST ;
typedef struct { int marker, first ; float pos ; } SORT_DAT ;
typedef struct { int marker, nn ; } intPair ;

/***************************************************************/

static void defcptInit(void) ;
static void defCptOrderBySubclones (DEFCPT look) ;
static void lookArrayDestroy (DEFCPT look) ;
static void uLocalDoDestroy (DEFCPT look) ;
static void defCptRestart (DEFCPT look) ;

#ifndef NON_GRAPHIC
static void localDestroy (void) ;
static void defcomputeDraw (DEFCPT look, char *text) ;
static Graph myGraph = 0 ;
#endif /* !NON_GRAPHIC */

/***************************************************************/

#ifndef NON_GRAPHIC

#define DEFCPTGET(name) \
  DEFCPT look ; \
  if (!graphAssFind (&DEFCPTMAG, &look)) \
    messcrash ("defcpt-graph not found in %s", name) ; \
  if (look->magic != DEFCPTMAG) \
    messcrash ("%s received a wrong defcpt-pointer", name)

#endif /* !NON_GRAPHIC */

#define localDoDestroy(x) ((x) ? uLocalDoDestroy(x), x=0, TRUE : FALSE)

#define TESCOLO \
   if (!arrayExists(look->colOrder)) \
      { int i = look->nd, *ip ; \
	look->colOrder = arrayCreate(i, int) ; \
	ip = arrayp(look->colOrder, i - 1, int) ; \
	while(i--) \
	  *ip-- = i ; \
      }

#define TESLIN \
   if (!arrayExists(look->linOrder)) \
      { int i = look->nm, *ip ; \
	look->linOrder = arrayCreate(i, int) ; \
	ip = arrayp(look->linOrder, i - 1, int) ; \
	while(i--) \
	  *ip-- = i ; \
      }

/***************************************************************/

static BOOL DEBUG = FALSE ;    /* additional printouts */

static DEFCPT nonGraphicLook = 0 ;
static DEFCPT myLook = 0 ;

/* AAAGGGGGHHHHHHH, why repeat it...sigh....edgrif                           */
static int DEFCPTMAG  =  727830152 ;  /* repeated in blyDnaGet */

static int nmrid = 0 ;
static Associator assDefLook = 0 ;
static Stack diffaction = 0 ;

static FILE *defCptOut = NULL ;

static int nblook = 0 ;

static FREEOPT mkaction[] = {
    { 44, "Ace.mbly"},
    {'q', "Quit [<filename>] : Quits ace.mbly [and reports on file]"},
    {'?', "? : List of commands"},
    {'l', "Load -all | -active | name : Loads sequences, discarding previous set (ex: load a1*)"},
    {'A', "add query : Adds in result of the query (ex: Add >?Clone; >Cloning_Vector)"},
    {'n', "newSCF : Adds new_scf reads to current assembly"},
    {'b', "Basecall : Replace external by acembly base call for tace-active set"},   
    {'g', "get n : get n oligos per sequence (ex: get 12, sort 90, assemble 8)"},
    {'s', "sort d : sorts results of get to limit 50 <d<=100"},
    {'a', "assemble e : assemble results of sort at error rate e %%"},
    {'j', "join [diagonal] : Joins at low strigency preassembled contigs"},
    {'J', "Juxtapose : Juxtapose the contigs into an assembly"},
    {'F', "fix : Recomputes the consensus from the elementary traces"},
    {'i', "insert : Tries to assemble into the same contigs all other reads"},
    { 10, "extend : Extend the reads beyond the present contig"},
    {'L', "Align <reference> : Align on wild type (i.e. on a reference sequence)"},
    { 20, "Vector_Clipping [-f] : look for vector [-f forces reevaluation] in loaded or active set"},
    {'C',"Clip_on [Excellent, Good, Fair, Tile, Consolidate] : Evaluates quality, {then  move clips]"},
    { 21, "Order_by_Size"},
    { 22, "Order_by_Subclones"},
    { 23, "Make_Subclones : Identify reads differing only by endding as from same subclone"},
    { 13, "autoedit"},
/*   18, "TrainNN",  */
    {'R', "Rename name : Saves as name, destroy any previous sequence called name or name.#"},
    {'S', "save_as [<name>] : Saves as name, or by incrementing (ex: a5 --> saved as a6)"},
    {1001, "cDNA_1 align est on all genomic sequences"},
    {1002, "cDNA_2 align est on active genomic set"},
    {1003, "cDNA_3 align est on one sequence"},
    {1011, "cDNA_11 align est on all junctions of genonic sequences"},
    {1012, "cDNA_12 align est on active set of junctions"},
    {1013, "cDNA_13 align est on one junction"},
    {1005, "cDNA_5 [-f file] [-alter] [-nonSliding] export confirmed introns"},
    {1006, "cDNA_6 [-f file] export exon to search oligos"},
    {1071, "cDNA_71 realign all transcribed_genes"},
    {1073, "cDNA_73 realign active set of transcribed_genes"},
  {1020, "cDNA_20 export active set in .bly format"},
  {1031, "cDNA_31 allocate all cDNAs to best gene"},
  {1032, "cDNA_32 allocate active cDNA to best gene"},
  {1033, "cDNA_33 allocate one cDNA to best gene"},
  {1200, "saucisse find most frequent words"},

  {2001, "Align_1 align all reads on all reference sequences"},
  {2002, "Align_2 align active reads on reference sequence"},
  {2003, "Align_3 undefined yet"},

    {'T', "tace [-f <file>y] : execute tace commands until ^D"},
    {'r', "Status: reports on memory usage"},
    {16, "Report : Reports present length of contigs"},
     {'B', "Statistics : number the differences between sequences and choosen consensus"}
/*
      case 'v':
	key = dnaAlignDoMakeSuperLink (ksNew, 0) ;
	freeOut (messprintf ("\n// made Assembly %s\n", name (key))) ;
	break ;
*/
} ;


/***************************************************************/

/* I have set this up for the stdin initialisation, I expect there will be   */
/* other stuff.                                                              */
/*                                                                           */
static void defcptInit(void)
{
  static BOOL initialised = FALSE ;

  if (intialised == FALSE)
    {
      intialised = TRUE ;

      /* Note to Jean, where does all the stuff that goes to stdout go when  */
      /* the graphical version of this program is being run ??               */
      if (!defCptOut)
	defCptOut = stdout ; 
    }
 
  return ;
}


/***************************************************************/

static int mardefOrder(void *a,void *b)
{
  return
    ((*(KEY*)a) & F_WHO) > ((*(KEY*)b) & F_WHO)  ?
      1 : ((*(KEY*)a) & F_WHO) == ((*(KEY*)b) & F_WHO) ? 0 : - 1 ;
}

/***************************************************************/

#ifndef NON_GRAPHIC
static int intPairOrder(void *a, void *b)
{
  return
    ((intPair *) a)->nn - ((intPair *) b)->nn ;
}
#endif /* !NON_GRAPHIC */

/***************************************************************/

#ifndef NON_GRAPHIC
static Array defCptTranspMat(Array donnee)
{ Array result = 0 ;
  int *ip, i = arrayMax(donnee) ;

  result = arrayCreate(i, int) ;
  array(result, i - 1, int) = 0 ;
  ip = arrp(donnee, i - 1, int) + 1 ;
  while(ip--, i--)
    arr(result, *ip, int) = i ;
  return result ;
} /* defCptTranspMat */
#endif /* !NON_GRAPHIC */

/***************************************************************/

#ifndef NON_GRAPHIC
static void defTreeSortM(void)
{ KEYSET ksm, ksd ;
  KEY *keyp, *keyq ;
  int i, j, n, nm, k = 0, imax, jmax, kmax, *ip, who ;
  Array tm, newMarkers = 0, defOrder = 0 ;
  DEFCPTGET("defTreeSortM") ;

  tm = arrayCreate(look->nm, int) ;
  array(tm, look->nm - 1, int) = 0 ;
  defOrder = defCptTranspMat(look->colOrder) ;
  imax = look->nd ;
  ip = arrp(look->colOrder, 0, int) ;
  for (i = 0 ; i < imax ; ip++, i++)
    { ksm = arr(look->marInDef, *ip, KEYSET) ;
      jmax = keySetMax(ksm) ;
      keyp = arrp(ksm, 0, KEY) - 1 ;
      newMarkers = arrayReCreate(newMarkers, 32, intPair) ;
      nm = 0 ;
      while(keyp++, jmax--)
	{ if ((*keyp & F_NO) || arr(tm, *keyp & F_WHO, int) || (*keyp & F_V_FLAG))
	    continue ;
	  who = *keyp & F_WHO ;
	  ksd = arr(look->defInMar, who, KEYSET) ;
	  kmax = keySetMax(ksd) ;
	  keyq = arrp(ksd, 0, KEY) - 1 ;
	  n = 0 ;
	  while(keyq++, kmax--)
	    if (!(*keyq & F_NO) && (j = arr(defOrder, *keyq & F_WHO, int)) > i &&
		!(*keyq & F_V_FLAG))
	      n += j - i ;  /* on compte le poids des restant sur la ligne et non le nombre d'elements restant sur la ligne*/
	  arrayp(newMarkers, nm, intPair)->marker = who ;
	  arrp(newMarkers, nm++, intPair)->nn = n ;
	  arr(tm, who, int) = 1 ;
	}
      if (nm)
	arraySort(newMarkers, intPairOrder) ;
      for (j = 0 ; j < nm ; j++)
	arr(look->linOrder, k++, int) = arrp(newMarkers, j, intPair)->marker ;
    }
            /* add the last ones in */
  ip = arrp(tm, 0, int) ;
  imax = look->nm ;
  for (i = 0 ; i < imax ; ip++, i++)
    if (!(*ip))
      arr(look->linOrder, k++, int) = i ;
  arrayDestroy(tm) ;
  arrayDestroy(newMarkers) ;
  arrayDestroy(defOrder) ;
  look->mapStatus |= F_SOMAR ;
  look->mapStatus &= ~F_SHOWM ;
  defcomputeDraw (look, 0) ;
} /* defTreeSortM */
#endif /* !NON_GRAPHIC */

/***************************************************************/

#ifndef NON_GRAPHIC
static int defCptSortOrder(void *a, void *b)
{ int i ;

  i = (((SORT_DAT *)a)->pos > ((SORT_DAT *)b)->pos) ? 1 : 0 ;
  if (!i)
    i = (((SORT_DAT *)a)->pos < ((SORT_DAT *)b)->pos) ? -1 : 0 ;
  if (!i)
    i = (((SORT_DAT *)b)->first - ((SORT_DAT *)a)->first) ; /* ordre decroissant car on veut le premier d'abord */
  return i ;                  /* was a)->pos in last test ??? */
} /* defCptSortOrder */
#endif /* !NON_GRAPHIC */

/***************************************************************/

#ifndef NON_GRAPHIC
static void defCptSortMarkers(void)
{ KEYSET *ksmp ;
  KEY *keyp ;
  int i, j, n, nm, k, imin, imax, jmax, *ip, who ;
  Array tm, newMarkers = 0, defOrder = 0 ;
  DEFCPTGET("defCptSortMarkers") ;

  TESCOLO ;
  TESLIN ;
  tm = arrayCreate(look->nm, int) ;
  array(tm, look->nm - 1, int) = 0 ;
  defOrder = defCptTranspMat(look->colOrder) ;
  newMarkers = arrayCreate(look->nm, SORT_DAT) ;
  ksmp = arrp(look->defInMar, 0, KEYSET) ;
  imax = look->nm ;
  nm = 0 ;
  for (i = 0 ; i < imax ; ksmp++, i++)
    { if (keySet(*ksmp, 0) & F_V_FLAG)
	continue ;
      jmax = keySetMax(*ksmp) ;
      keyp = arrp(*ksmp, 0, KEY) - 1 ;
      n = 0 ;
      k = 0 ;
      imin = look->nd ;
      while(keyp++, jmax--)
	{ if ((*keyp & F_NO) || (*keyp & F_V_FLAG))
	    continue ;
	  who = *keyp & F_WHO ;
	  j = arr(defOrder, who, int) ;
	  n += j ;
	  k++ ;
	  if (j < imin)
	    imin = j ;
	}
      if (k)
	{ arrayp(newMarkers, nm, SORT_DAT)->marker = i ;
	  arrp(newMarkers, nm, SORT_DAT)->first = imin ;
	  arrp(newMarkers, nm++, SORT_DAT)->pos = n / k ;
	  arr(tm, i, int) = 1 ;
	}
    }
  if (nm)
    arraySort(newMarkers, defCptSortOrder) ;
  k = 0 ;
  for (j = 0 ; j < nm ; j++)
    arr(look->linOrder, k++, int) = arrp(newMarkers, j, SORT_DAT)->marker ;
            /* add the last ones in */
  ip = arrp(tm, 0, int) ;
  imax = look->nm ;
  for (i = 0 ; i < imax ; ip++, i++)
    if (!(*ip))
      arr(look->linOrder, k++, int) = i ;
  arrayDestroy(tm) ;
  arrayDestroy(newMarkers) ;
  arrayDestroy(defOrder) ;
  look->mapStatus |= F_SOMAR ;
  look->mapStatus &= ~F_SHOWM ;
  defcomputeDraw (look, 0) ;
} /* defCptSortMarkers */
#endif /* !NON_GRAPHIC */

/***************************************************************/

#ifndef NON_GRAPHIC
static int defCptWhoSup(Array donnee, Array result)
{ KEYSET *ksp ;
  int i, cc = 0 ;
  char *cp ;

  i = arrayMax(donnee) ;
  array(result, i - 1, char) = 0 ;
  cp = arrp(result, i - 1, char) + 1 ;
  ksp = arrp(donnee, i - 1, KEYSET) + 1 ;
  while(ksp--, cp--, i--)
    { 
      if (keySetMax(*ksp))
	{
	  if (keySet(*ksp, 0) & (((unsigned int)248) << 24)) /* flag autre que F_ZERO */
	    { cc++ ;
	      *cp = 1 ;
	    }
	}
      else   /* pas de marqueurs pour la def */
	{ cc++ ;
	  *cp = 1 ;
	}
    }
  return cc ;
} /* defCptWhoSup */
#endif /* !NON_GRAPHIC */

/***************************************************************/

#ifndef NON_GRAPHIC
static int dupCostYes(DEFCPT look)
{ int cost = - look->nd, i, imax, j, *ip, who, costmax = look->nd ;
  Array inter = 0, defSup = 0, marSup = 0 ;
  KEYSET *ksp ;
  KEY *keyp ;
  char *cp ;

  defSup = arrayCreate(look->nd, char) ;
  cost += defCptWhoSup(look->defInMar, defSup) ;
  marSup = arrayCreate(look->nm, char) ;
  defCptWhoSup(look->marInDef, marSup) ;
  inter = arrayCreate(look->nd, char) ;
  array(inter, look->nd, char) = 0 ;
  imax = look->nm ;
  ip = arrp(look->linOrder, 0, int) - 1 ;
  while(ip++, imax--)
    { 
      if (arr(defSup, *ip, char))
	{ costmax-- ;
	  continue ;
	}
      ksp = arrp(look->defInMar, *ip, KEYSET) ;
      j = keySetMax(*ksp) ;
      keyp = arrp(*ksp, 0, KEY) ;
      who = *keyp & F_WHO ;
      for (i = 0 ; i < look->nd ; i++)
	{ 
	  if (i == who)
	    { 
	      if (arr(marSup, who, char))
		continue ;
	      if (*keyp & F_NO)
		{ if (arr(inter, who, char))
		    { cost += 2 ; /* pour que les inconnues comptent comme 1/2 dans le cost */
		      arr(inter, who, char) = 0 ;
		    }
		}
	      else /* F_YES */
		arr(inter, who, char) = 1 ;
	      if (--j)
		{ keyp++ ;
		  who = *keyp & F_WHO ;
		}
	    }
	  else if (arr(inter, i, char) && !arr(marSup, i, char))
	    { cost++ ; /* inco donc 1/2 */
	      arr(inter, i, char) = 0 ;
	    }
	}
    }
  i = look->nd ;
  cp = arrp(inter, 0, char) ;
  while(i--)
    if (*cp++)
      cost +=2 ;
  arrayDestroy(defSup) ; 
  arrayDestroy(marSup) ; 
  arrayDestroy(inter) ;
  return (50 * cost) / costmax ; /* le cost est le % de trou dans chaque def */
} /* dupCostYes */
#endif /* !NON_GRAPHIC */

/***************************************************************/

#ifndef NON_GRAPHIC
static int dupCostNo(DEFCPT look)
{ int cost = - look->nd, i, imax, j, *ip, who, costmax = look->nd ;
  Array inter = 0, defSup = 0, marSup = 0 ;
  KEYSET *ksp ;
  KEY *keyp ;
  char *cp ;

  defSup = arrayCreate(look->nd, char) ;
  cost += defCptWhoSup(look->defInMar, defSup) ;
  marSup = arrayCreate(look->nm, char) ;
  defCptWhoSup(look->marInDef, marSup) ;
  inter = arrayCreate(look->nd, char) ;
  array(inter, look->nd - 1, char) = 0 ;
  imax = look->nm ;
  ip = arrp(look->linOrder, 0, int) - 1 ;
  while(ip++, imax--)
    { 
      if (arr(defSup, *ip, char))
	{ costmax-- ;
	  continue ;
	}
      ksp = arrp(look->defInMar, *ip, KEYSET) ;
      j = keySetMax(*ksp) ;
      keyp = arrp(*ksp, 0, KEY) - 1 ;
      while(keyp++, j--)
	{ who = *keyp & F_WHO ;
	  if (arr(marSup, who, char))
	    continue ;
	  if (*keyp & F_NO)
	    { if (arr(inter, who, char))
		{ cost++ ;
		  arr(inter, who, char) = 0 ;
		}
	    }
	  else /* F_YES */
	    arr(inter, who, char) = 1 ;
	}
    }
  i = look->nd ;
  cp = arrp(inter, 0, char) ;
  while(i--)
    if (*cp++)
      cost++ ;
  arrayDestroy(defSup) ; 
  arrayDestroy(marSup) ; 
  arrayDestroy(inter) ;
  return (100 * cost) / costmax ;
} /* dupCostNo */
#endif /* !NON_GRAPHIC */

/***************************************************************/

#ifndef NON_GRAPHIC
static int dupCostED(DEFCPT look)
{ int cost = - look->nm, imax, j, *ip, who, costmax = look->nm ;
  Array inter = 0, defSup = 0, marSup = 0 ;
  KEYSET *ksp ;
  KEY *keyp ;

  defSup = arrayCreate(look->nd, char) ;
  defCptWhoSup(look->defInMar, defSup) ;
  marSup = arrayCreate(look->nm, char) ;
  cost += defCptWhoSup(look->marInDef, marSup) ;
  inter = arrayCreate(look->nm, char) ;
  array(inter, look->nm - 1, char) = 0 ;
  imax = look->nd ;
  ip = arrp(look->colOrder, 0, int) - 1 ;
  while(ip++, imax--)
    { 
      if (arr(marSup, *ip, char))
	{ costmax-- ;
	  continue ;
	}
      ksp = arrp(look->marInDef, *ip, KEYSET) ;
      j = keySetMax(*ksp) ;
      keyp = arrp(*ksp, 0, KEY) - 1 ;
      while(keyp++, j--)
	{ who = *keyp & F_WHO ;
	  if (arr(defSup, who, char))
	    continue ;
	  if ((*keyp & F_PERE) && !arr(inter, who, char))
	    { cost++ ;
	      arr(inter, who, char) = 1 ;
	    }
	  else if ((*keyp & F_MERE) && arr(inter, who, char))
	    { cost++ ;
	      arr(inter, who, char) = 0 ;
	    }
	}
    }
  arrayDestroy(defSup) ; 
  arrayDestroy(marSup) ; 
  arrayDestroy(inter) ;
  return (100 * cost) / costmax ;
} /* dupCostED */
#endif /* !NON_GRAPHIC */

/***************************************************************/
/* attention : Pour le moment on ne tient pas compte des brokenLink
   (sur les paires que l'on sait assemblees) et une def ne contenant
   que des NO (rare ?) compte pour cost -1 et non pas 0 */

#ifndef NON_GRAPHIC
static int dupCost(DEFCPT look)
{ if (look->method == M_EDWARD)
    return dupCostED(look) ;
  if (!(look->whatDis & F_E_YN) || (look->whatDis & 1)) 
    return dupCostYes(look) ;   /* only YES data or unknown outside */
  return dupCostNo(look) ;
} /* dupCost */
#endif /* !NON_GRAPHIC */

/***************************************************************/

#ifndef NON_GRAPHIC
static MENUOPT defMapGraphMenu[] = {
  { graphDestroy, "Quit"},
  { help, "Help"},
  { graphPrint, "Print"},
  { NULL, NULL  }} ;

/***************************************************************/

static void defMapDisplay(void)
{
  BUMP bp = bumpCreate(100, 0) ;
  int i, j, imax, line = 4, *ip, cost ;
  char *cp ;
  int x1 ;
  float y1 ;
  KEYSET *ksp ;
  KEY *keyp, kludge ;
  Array defOrder = 0, marOrder = 0 ;
  DEFCPTGET("defMapDisplay") ;

  messStatus("please wait for Display") ;
  TESCOLO ;
  TESLIN ;

  if (look->method == M_FINGPR) /* Finger Print ; dessin du gel */
    { fpClearDisplay() ;
      i = arrayMax(look->colOrder) ;
      ip = arrp(look->colOrder, 0, int) - 1 ;
      while(ip++, i--)
	fpDisplay(keySet(look->def, *ip)) ;
    }

  defOrder = defCptTranspMat(look->colOrder) ;
  if (!(look->method == M_EDWARD) && !(look->mapStatus & F_SOMAR))
    defCptSortMarkers() ;
  marOrder = defCptTranspMat(look->linOrder) ;
  if (graphActivate(look->defMapGraph))
    graphPop() ;
  else
    { look->defMapGraph = graphCreate(TEXT_FULL_SCROLL, "Interval/Locus Map", 0.0,0.25,0.4,0.65) ;
      graphMenu(defMapGraphMenu) ;
      graphHelp("Deficiency_map") ;
    }
  graphClear() ;

  if (look->selectedMap)
    graphText(messprintf("Map : %s", name(look->selectedMap)), 1., .2) ;

  cost = dupCost(look) ;
  graphText(messprintf("Cost : %d", cost), 1, 1) ;
  imax = look->nd ;
  ip = arrp(look->colOrder, 0, int) ;
  for (i = 0 ; i < imax ; ip++, i++)
    { cp = name(keySet(look->def, *ip)) ;
      x1 = 0 ; y1 = 2 * i ;
      bumpItem(bp, 1, strlen(cp) + 2.0, &x1, &y1) ;
      graphText(cp, 2 * i + 4, line + x1) ;
    }
  line += bumpMax(bp) ;
  graphBoxStart() ;
  line += 2 ;
  imax = look->nm ;
  x1 = look->nd ;
  ip = arrp(look->linOrder, 0, int) ;
  for (i = 0 ; i < imax ; ip++, i++)
    {
      if (class(keySet(look->mar, *ip)))
	graphText(name(keySet(look->mar, *ip)), 2 * x1 + 8, i + line) ;
      else if (look->method == M_ASSEMB)
	graphText(dnaAlignDecodeOligo(keySet(look->mar, *ip)), 2 * x1 + 8, i + line) ;
    }
  i = look->nd ;                  /* dessin */
  ip = arrp(defOrder, 0, int) - 1 ;
  ksp = arrp(look->marInDef, 0, KEYSET) - 1 ;
  while(ip++, ksp++, i--)
    { j = keySetMax(*ksp) ;
      keyp = arrp(*ksp, 0, KEY) - 1 ;
      if (keySet(*ksp, 0) & F_V_FLAG)
	kludge = 1 ;
      else
	kludge = 0 ;
      while(keyp++, j--)
	{ x1 = arr(marOrder, *keyp & F_WHO, int) ;
	  switch((*keyp & F_FLAG) | kludge)
	    {
	    case 0: case F_YES:
	      graphText("#", 2 *(*ip) + 4, line + .2 + x1) ;
	      break ;
	    case F_NO:
	      if ((look->whatDis & 7) != 1)
		graphText("-", 2 *(*ip) + 4, line + x1) ;
	      break ;
	    default:
	      switch(*keyp & F_PEME)
		{
		case 0: case F_YES:
		  graphText("+", 2 *(*ip) + 4, line - .1 + x1) ;
		  break ;
		case F_NO:
		  if ((look->whatDis & 7) != 1)
		    graphText("_", 2 *(*ip) + 4, line - .2 + x1) ;
		  break ; 
		}
	      break ;
	    }
	}
    }
  graphBoxEnd() ;
  look->mapStatus |= F_SHOWM ;
  bumpDestroy(bp) ;
  arrayDestroy(defOrder) ;
  arrayDestroy(marOrder) ;
  graphTextBounds(2 * look->nd + 48, look->nm + line + 5) ;
  graphRedraw() ;
  defcomputeDraw (look, 0) ;
}
#endif /* !NON_GRAPHIC */

/***************************************************************/
#ifndef NON_GRAPHIC
static void defCptHisto(char *text1, char *text2)
{ Array h1, h2, h3, h4, h5, defOrder ;
  int i, j, k, n, max, *ip ;
  KEYSET *ksp ;
  KEY *keyp ;
  DEFCPTGET("defCptHisto") ;

  i = look->nd ;              /* Histo des nombres de marqueurs par def */
  h1 = arrayCreate(i, int) ;
  h3 = arrayCreate(15, int) ; /* Histo des nombres de def par nombre de marqueur qu'ils contiennent */
  ip = arrayp(h1, i - 1, int) ;
  ksp = arrp(look->marInDef, i - 1, KEYSET) + 1 ;
  while(ksp--, i--)
    { j = keySetMax(*ksp) ;
      k = 0 ;
      keyp = arrp(*ksp, 0, KEY) - 1 ;
      while(keyp++, j--)
	if (!(*keyp & F_NO))
	  k++ ;
      *ip-- = k ;
      array(h3, k, int)++ ;
    }
  i = look->nm ;              /* Histo des nombres de defs par marqueur */
  h2 = arrayCreate(i, int) ;
  ip = arrayp(h2, i - 1, int) ;
  ksp = arrp(look->defInMar, i - 1, KEYSET) + 1 ;
  while(ksp--, i--)
    { j = keySetMax(*ksp) ;
      k = 0 ;
      keyp = arrp(*ksp, 0, KEY) - 1 ;
      while(keyp++, j--)
	if (!(*keyp & F_NO))
	  k++ ;
      *ip-- = k ;
    }
  TESCOLO ;
  h4 = arrayCreate(25, int) ;
  h5 = arrayCreate(25, int) ;
  defOrder = defCptTranspMat(look->colOrder) ;
  i = look->nm ;
  ksp = arrp(look->defInMar, 0, KEYSET) - 1 ;
  while(ksp++, i--)
    { k = look->nd ;
      max = - 1 ;
      j = keySetMax(*ksp) ;
      keyp = arrp(*ksp, j - 1, KEY) + 1 ;
      while(keyp--, j--)
	{ if (!(*keyp & F_NO))
	    {
	      if ((n = arr(defOrder, *keyp & F_WHO, int)) < k)
		k = n ;
	      if (n > max)
		max = n ;
	    }
	}
      array(h4, max - k, int)++ ;
      j = keySetMax(*ksp) ;
      while(keyp++, j--)
	if (!(*keyp & F_NO))
	  array(h5, arr(defOrder, *keyp & F_WHO, int) - k, int)++ ;
    }
  arrayDestroy(defOrder) ;
  plotHisto(messprintf("Number of %s per %s", text2, text1), h1) ;
  plotHisto(messprintf("Number of %s per %s", text1, text2), h2) ;
  plotHisto(messprintf("Number of %s per number of %s inside", text1, text2), h3) ;
  plotHisto(messprintf("Number of %s per interval between first & last %s", text2, text1), h4) ;
  plotHisto(messprintf("Number of %s at x %s from their first appearance", text2, text1), h5) ;
  look->mapStatus |= F_HIST ;
  defcomputeDraw (look, 0) ;
  /* note that h1, h2, h3, h4, and h5 will 
     be destroyed later by plotHisto */
}

/***************************************************************/

static void defCptRmHisto(void)
{
  DEFCPTGET("defCptRmHisto") ;
  
  plotHistoRemove() ;
  look->mapStatus &= ~F_HIST ;
  defcomputeDraw (look, 0) ;
}
#endif /* !NON_GRAPHIC */

/***************************************************************/
/*******************   Filter   Functions   ********************/
/***************************************************************/

#ifndef NON_GRAPHIC
static int myArrayOrder(void *a, void *b)
{ return *(int *)a - *(int *)b ;
}
#endif /* !NON_GRAPHIC */

/***************************************************************/

#ifndef NON_GRAPHIC
#define myabs(__x) ((__x) < 0 ? (-(__x)) : (__x))

static void defCptDoSuppChim(DEFCPT look)
{ KEYSET *ksp, *ksq, ks = 0 ;
  KEY *keyp, *keyq ;
  int i, j, k, n, m, max = look->nd, max2, nb = 0, *ip ;
  Array h1, h2, defOrder ;

  h1 = arrayCreate(look->nd, int) ;
  h2 = arrayCreate(look->nd, int) ;
  array(h2, look->nd - 1, int) = 0 ;
  defOrder = defCptTranspMat(look->colOrder) ;
  ksp = arrp(look->marInDef, 0, KEYSET) ;
  for (i = 0 ; i < look->nd ; ksp++, i++)
    { j = keySetMax(*ksp) ;
      keyp = arrp(*ksp, 0, KEY) - 1 ;
      k = 0 ;
      ks = keySetReCreate(ks) ;
      while(keyp++, j--)
	{ ksq = arrp(look->defInMar, *keyp & F_WHO, KEYSET) ;
	  n = keySetMax(*ksq) ;
	  keyq = arrp(*ksq, 0, KEY) - 1 ;
	  while(keyq++, n--)
	    keySet(ks, k++) = *keyq & F_WHO ;
	}
      keySetSort(ks) ;
      keySetCompress(ks) ;
      m = k = keySetMax(ks) ;
      j = 0 ;
      keyq = arrp(ks, 0, KEY) - 1 ;
      while(keyq++, k--)
	{
	  if ((n = myabs(arr(defOrder, i, int) - arr(defOrder, *keyq, int))) > 20)
	    array(h1, n, int)++ ;
	  j += n ;
	}
      arr(h2, i, int) = j / m ;
    }
  arraySort(h2, myArrayOrder) ;
  plotHisto("nouveau", h1) ;
  plotHisto("nouveau - bis", h2) ;
  arrayDestroy(defOrder) ;
  if (look->manEntry)
    { 
      ACEIN limit_in;
      if ((limit_in = messPrompt("Limit Value", "", "i", 0)))
	{ 
	  aceInInt(limit_in, &max2); 
	  aceInDestroy (limit_in);

	  if (max2 < max && max2 > 10)
	    max = max2 ;
	}
    }
  else max = look->dlimit ;
  ip = arrp(h2, 0, int) - 1 ;
  i = look->nd ;
  ksp = arrp(look->marInDef, 0, KEYSET) - 1 ;
  while(ip++, ksp++, i--)
    if (*ip >= max)
      { nb++ ;
	keySet(*ksp, 0) |= F_NON_DIAG ;
      }
  if (diffaction)
    catText(diffaction, messprintf("filter %d // -> supp %d seq\n", max, nb)) ;
  look->mapStatus = 1 ;
  look->mapStatus |= F_HIST ;
  arrayDestroy(h1) ;
  arrayDestroy(h2) ;
  defcomputeDraw (look, messprintf("I suppress %d sequences", nb)) ;
}
#endif /* !NON_GRAPHIC */

#ifndef NON_GRAPHIC
static void defCptSuppChim(void)
{ DEFCPTGET ("") ;
 defCptDoSuppChim(look) ;
}
#endif /* !NON_GRAPHIC */

/***************************************************************/

#ifndef NON_GRAPHIC
static void defCptSupOneFlag(DEFCPT look, KEY flag)
{ int i, j ;
  KEYSET *ksp ;
  KEY unflag = ~flag, *keyp ;

  i = look->nd ;
  ksp = arrp(look->marInDef, 0, KEYSET) - 1 ;
  while(ksp++, i--)
    keySet(*ksp, 0) &= unflag ;
  i = look->nm ;
  ksp = arrp(look->defInMar, 0, KEYSET) - 1 ;
  while(ksp++, i--)
    { keySet(*ksp, 0) &= unflag ;
      if ((look->whatDis & F_E_TFLAG) && (flag & F_NON_DIAG))
	{ j = keySetMax(*ksp) - 1 ;
	  keyp = arrp(*ksp, 0, KEY) ;
	  while(keyp++, j--)
	    *keyp &= unflag ;
	}
    }
  if ((look->whatDis & F_E_TFLAG) && (flag & F_NON_DIAG))
    look->whatDis &= ~F_E_TFLAG ;
  look->mapStatus &= ~(F_SHOWM | F_SORT | F_SOMAR) ;
  defcomputeDraw (look, 0) ;
} /* defCptSupOneFlag */
#endif /* !NON_GRAPHIC */

/***************************************************************/

#ifndef NON_GRAPHIC
static void defCptSupSing(void)
{ int i ;
  KEYSET *ksp ;
  DEFCPTGET("defCptSupSing") ;

  i = look->nd ;
  ksp = arrp(look->marInDef, 0, KEYSET) - 1 ;
  while(ksp++, i--)
    if (keySetMax(*ksp) < 2)
      keySet(*ksp, 0) |= F_SINGLE ;
  look->whatDis |= F_E_ZFLAG ;
  look->mapStatus &= ~(F_SHOWM | F_SORT | F_SOMAR) ;
  defcomputeDraw (look, 0) ;
} /* defCptSupSing */
#endif /* !NON_GRAPHIC */

/***************************************************************/

#ifndef NON_GRAPHIC
static void defTreeSupr(void)
{
  static float xx = .95 ;
  float x1 ;
  int i,j, nn = 0 , n , n0 = 0 ;
  Array nb = 0 ;
  double my , cur, cumul, u0  ;
  KEYSET *ksp ;
  KEY *keyp ;
  ACEIN level_in;
  DEFCPTGET("defTreeSupr") ;

  if (!(level_in = messPrompt ("Confidence Level (.5 ... 1)", messprintf("%f",xx), "f", 0)))
    return ;
  aceInFloat (level_in, &x1);
  aceInDestroy (level_in);

  if (x1 > 1 || x1 < .5 )
    { messout("Please, choose a value (%f) between 0.5 and 1", x1) ;
      return ;
    }
  nb = arrayCreate(look->nm, int) ;
  xx = x1 ;
  i = look->nm ;  /* compte les def par marqueur */
  ksp = arrp(look->defInMar, 0, KEYSET) - 1 ;
  while(ksp++, i--)
    { n = 0 ;
      j = keySetMax(*ksp) ;
      keyp = arrp(*ksp, 0, KEY) ;
      if (*keyp & F_SUP_OVER)
	*keyp &= ~F_SUP_OVER ;
      while (j--)
	if (!(*keyp++ & F_NO))
	  n++, nn++ ;
      array(nb, i, int) = n ;
    }
  my = nn / (double)look->nm ;
  cumul = u0 = exp(-my) ; 
  cur = 1 ;
  while(cumul < xx)
    { cur = cur * my / ++n0 ;
      cumul += cur * u0 ;
    }
  i = look->nm ;  /* compte les def par marqueur */
  while(i--)
    if (arr(nb, i, int) > n0)
      keySet(arr(look->defInMar, i, KEYSET), 0) |= F_SUP_OVER ;
  arrayDestroy(nb) ;
  look->whatDis |= F_E_ZFLAG ;
  look->mapStatus &= ~(F_SHOWM | F_SORT | F_SOMAR) ;
  defcomputeDraw (look, 0) ;
} /* defTreeSupr */
#endif /* !NON_GRAPHIC */

/***************************************************************/

#ifndef NON_GRAPHIC
static void defTreeNonDiag(void)
{ int i, j = 0, min, nmax ;
  Array defOrder ;
  KEYSET *ksp ;
  KEY *keyp ;
  ACEIN width_in;
  DEFCPTGET("non diag") ;

  if (!(width_in = messPrompt ("Please, choose a width", messprintf("%i",j), "i", 0)))
    return ;
  aceInInt(width_in, &nmax) ;
  aceInDestroy (width_in);

  TESCOLO ;
  defOrder = defCptTranspMat(look->colOrder) ;
  i = look->nm ;
  ksp = arrp(look->defInMar, 0, KEYSET) - 1 ;
  while(ksp++, i--)
    { j = keySetMax(*ksp) ;
      keyp = arrp(*ksp, 0, KEY) - 1 ;
      min = look->nd ;
      while(keyp++, j--)
	{ if (*keyp & F_NON_DIAG)
	    *keyp &= ~F_NON_DIAG ;
	  if (!(*keyp & F_NO) && arr(defOrder, *keyp & F_WHO, int) < min)
	    min = arr(defOrder, *keyp & F_WHO, int) ;
	}
      j = keySetMax(*ksp) ;
      while(keyp--, j--)
	if (!(*keyp & F_NO) && (arr(defOrder, *keyp & F_WHO, int) >= min + nmax))
	  *keyp |= F_NON_DIAG ;
    }
  look->whatDis |= F_E_TFLAG ;
  look->mapStatus &= ~(F_SHOWM | F_SORT | F_SOMAR) ;
  defcomputeDraw (look, 0) ;
} /* defTreeNonDiag */
#endif /* !NON_GRAPHIC */

/***************************************************************/

#ifndef NON_GRAPHIC
static void defSupFlag(void)
{ DEFCPTGET ("defSupFlag") ;

  if (look->whatDis & F_E_FLAG)
    { defCptSupOneFlag(look, F_V_FLAG) ;
      look->whatDis &= ~F_E_FLAG ;
      look->mapStatus &= ~(F_SHOWM | F_SORT | F_SOMAR) ;
    }
  defcomputeDraw (look, 0) ;
} /* defSupFlag */
#endif /* !NON_GRAPHIC */

/***************************************************************/
/**************************    Divers    ***********************/
/***************************************************************/

#ifndef NON_GRAPHIC
static int gmapOrder(void *a, void *b)
{ float x ;

  x = ((GMAPST *) a)->p - ((GMAPST *) b)->p ;
  return
    x < 0 ? - 1 : (x == 0 ? 0 : 1) ;
} /* gmapOrder */
#endif /* !NON_GRAPHIC */

/***************************************************************/

#ifndef NON_GRAPHIC
static void defCptMapStan(DEFCPT look)
{ Array gmapOrdered ;
  int i, j, *ip ;
  GMAPST *gmsp ;

  gmapOrdered = arrayCopy(look->gmap) ;
  arraySort(gmapOrdered, gmapOrder) ;
  i = look->nm ;
  look->linOrder = arrayCreate(i, int) ;
  gmsp = arrp(gmapOrdered, i - 1, GMAPST) + 1 ;
  ip = arrayp(look->linOrder, i - 1, int) ;
  while(gmsp--, i--)
    {
      if (keySetFind(look->mar, gmsp->gene, &j))
	*ip-- = j ;
      else
	messcrash("defCptMapStan, gene : %s", name(gmsp->gene)) ;
    }
  arrayDestroy(gmapOrdered) ;
} /* defCptMapStan */
#endif /* !NON_GRAPHIC */

/***************************************************************/

#ifndef NON_GRAPHIC
static void defCptGetPos(DEFCPT look)
{ int i = arrayMax(look->mar) ;
  float p ;
  OBJ obj ;
  GMAPST *gmsp ;
  KEY *keyp ;

  look->gmap = arrayCreate(i, GMAPST) ;
  gmsp = arrayp(look->gmap, i - 1, GMAPST) + 1 ;
  keyp = arrp(look->mar, i - 1, KEY) + 1 ;
  while(gmsp--, keyp--, i--)
    { gmsp->gene = *keyp ;
      obj = bsCreate(*keyp) ;
      if (obj)
	{ if (bsFindKey(obj, _Map, look->selectedMap))
	    { gmsp->ch = look->selectedMap ;
	      bsPushObj(obj) ;
	      if (bsGetData(obj, _Position, _Float, &p))
		gmsp->p = p ;
	    }
	  bsDestroy(obj) ;
	}
    }
  if (look->nm) defCptMapStan(look) ;
}
#endif /* !NON_GRAPHIC */

/***************************************************************/

static int defCptDoTree(DEFCPT look)
{ float lambda, mu ;
  TREE_DEF *seg ;
  int i ;

  if (!look->def || keySetMax (look->def) < 2)
    { fprintf (defCptOut, " // Nothing to sort") ;
      return 0 ;
    }

  arrayDestroy (look->colOrder) ;
  arrayDestroy (look->linOrder) ;
  TESCOLO ;
  TESLIN ;

  if (arrayExists(look->maillon))
    { i = arrayMax(look->maillon) ;
      seg = arrp(look->maillon, i - 1, TREE_DEF) + 1 ;
      while(seg--, i--)
	{ keySetDestroy(seg->ks) ;
	  keySetDestroy(seg->x) ;
	}
    }
  arrayDestroy(look->maillon) ;
  if (look->manEntry)
    { freeforcecard(look->distance) ;
      freefloat(&lambda) ;
      if (lambda > 1 || lambda <= 0)
	{ 
	  ACEIN limit_in;
	  messout("Please, choose a distance limit value (%f) between 0 and 1", lambda) ;
	ici:
	  if (!(limit_in = messPrompt("distance limit (0 ... 1)", messprintf("%f", lambda), "f", 0)))
	    return 0 ;
	  aceInFloat(limit_in, &mu) ;
	  aceInDestroy (limit_in);
	  if (mu > 1 || mu <= 0)
	    { messout("Please, choose a value (%f) between 0 and 1", mu) ;
	      goto ici ;
	    }
	  lambda = mu ;
	  sprintf(look->distance, "%f", lambda) ;
	}
      look->dlimit = (int)(100 * lambda) ;
    }
  messStatus("I Sort the data") ;
  if ((i = intCptTree(look)))
    {
      look->mapStatus |= (F_SORT | 3) ;
      if (look->maillon && arrayMax(look->maillon))
	look->mapStatus &= ~F_ASSSEG ;
      else arrayDestroy(look->maillon) ;
      look->mapStatus &= ~(F_SHOWM | F_SOMAR) ;
      if (diffaction)
	catText(diffaction, messprintf("sort %d // -> Find %d seg ; %s\n", look->dlimit, i, timeShow(timeNow()))) ;
    }
  else
    { if (look->display)
	messout("no distances < max") ;
      look->mapStatus = ((look->mapStatus & ~F_BOUT) | 6) ;
    }
#ifndef NON_GRAPHIC
  defcomputeDraw (look, messprintf("I find %d segment", i)) ;
#endif /* !NON_GRAPHIC */
  return i ;
}

/***************************************************************/

#ifndef NON_GRAPHIC
static void defCptTree(void)
{ DEFCPTGET("defCptTree") ;

  defCptDoTree(look) ;
} /* defCptTree */
#endif /* !NON_GRAPHIC */

/***************************************************************/

#ifndef NON_GRAPHIC
static void defCptTreatUnk(void)
{ 
  int i, j ;
  char buf[140] ;
  ACEIN mode_in;
  DEFCPTGET("defCptTreatUnk") ;

  j = look->whatDis & F_UNK_T ;
  sprintf(buf, "How do you want to consider unknown datas ?\n%s\n%s\n%s\n%s\n%s",
	  "unconsidered (0)", "outside (1)", "1/2 outside (2)",
	  "1/2 in - 1/2 outside (3)", "proportionally (4)") ;
  if (!(mode_in = messPrompt(buf, messprintf("%i", j), "i", 0)))
    return ;
  aceInInt(mode_in, &i) ;
  aceInDestroy (mode_in);

  if (i > 4 || i < 0)
    {
      messout("You are entering a bad value. Try again") ;
      return ;
    }
  look->whatDis &= ~F_UNK_T ;
  look->whatDis |= i ;
  look->mapStatus &= ~(F_SORT | F_SOMAR) ;
  defcomputeDraw (look, 0) ;
} /* defCptTreatUnk */
#endif /* !NON_GRAPHIC */

/***************************************************************/
/********************      Ace - mbly     **********************/
/***************************************************************/
#ifndef NON_GRAPHIC
void defCptAddSeqIn(KEY link, KEY key)
{
  DEFCPT look = defCptGetLook (link) ;

  defcptInit() ;

  dnaAlignAddSeqIn(look, key) ;

  return ;
}
#endif  /* !NON_GRAPHIC */

/***************************************************************/
/********************      Def  -  Dup     *********************/
/***************************************************************/

#ifndef NON_GRAPHIC
static void defDatum(KEY gene, Array cc, KEYSET dd)
{ OBJ obj = bsCreate(gene) ;
  int i, n = arrayMax(cc), dummy ;
  KEY def ;
  LINK *ca ;
  Array aa = 0 ;

  if(!obj) return ;
  aa  = arrayReCreate(aa, 30, BSunit) ;
  if( bsFindTag(obj, _Inside) &&
     bsFlatten(obj, 2, aa))
    for (i = 1 ; i < arrayMax(aa) ; i += 2)
      if (def = arr(aa, i, BSunit).k, keySetFind(dd, def, &dummy))
	{ ca = arrayp(cc, n++, LINK) ;
	  ca->a = def ;
	  ca->b = gene ;
	  ca->type = F_YES ;
	  ca->group = 0 ;
	}
  bsGoto(obj, 0) ;
  arrayMax(aa) = 0 ;
  if( bsFindTag(obj,_Outside) &&
     bsFlatten(obj, 2, aa))
    for (i = 1 ; i < arrayMax(aa) ; i += 2)
      if (def = arr(aa, i, BSunit).k, keySetFind(dd, def, &dummy))
	{ ca = arrayp(cc, n++, LINK) ;
	  ca->a = def ;
	  ca->b = gene ;
	  ca->type = F_NO ;
	  ca->group = 0 ;
	} 
  bsDestroy(obj) ;
  arrayDestroy(aa) ;
} /* defDatum */
#endif /* !NON_GRAPHIC */

/***************************************************************/

#ifndef NON_GRAPHIC
static BOOL defCptDoParseDB(DEFCPT look, KEYSET map2def, char *cp)
{ Array cc = 0, aa, histo, vertex ;
  LINK *c ;
  int n, box , line = 15, i, iLocus ;
  KEY map, locus ; 
  OBJ Map , Locus ; 
  KEYSET map2genes = 0, defs, genes ;

  if (map2def)
    map2genes = query(map2def, ">STS") ;
  else if (!lexword2key(cp, &map, _VMap) || !(Map = bsCreate(map)))
    { messout ("Unknown map, I can t proceed") ;
      return FALSE ;
    }
  else
    { map2genes = keySetCreate() ;
      map2def = keySetCreate() ;
      look->selectedMap = map ;
      aa = arrayCreate(100, BSunit) ;
      if (bsFindTag (Map, _Contains))
	bsFlatten(Map, 2, aa) ;
      for (iLocus = 1 ; iLocus < arrayMax(aa) ; iLocus += 2)
	{ locus = arr(aa, iLocus, BSunit).k ;
	  if ((Locus = bsCreate(locus)))
	    { 
	      if (bsFindKey(Locus, _Map, map) &&
		  bsPushObj(Locus))
		{
		  if (bsFindTag(Locus, _Position))
		    keySet(map2genes, keySetMax(map2genes)) = locus ;
		  else if (bsFindTag(Locus, _Left))
		    keySet(map2def, keySetMax(map2def)) = locus ;
		}
	      bsDestroy(Locus) ;
	    }
	}
      arrayDestroy(aa) ;
    }
  keySetSort(map2genes) ;
  keySetCompress(map2genes) ;
  keySetSort(map2def) ;
  keySetCompress(map2def) ;
  if (keySetMax(map2genes) && keySetMax(map2def))
    defcomputeDraw (look, messprintf("Map : %s%s%d %s, %d %s", 
			      look->selectedMap ? name(look->selectedMap) : "",
			      look->selectedMap ? ", " : "",
			      keySetMax(map2def), className(keySet(map2def, 0)),
			      keySetMax(map2genes), className(keySet(map2genes, 0)))) ;
  cc = arrayCreate(50, LINK) ;
  for (i = 0; i < keySetMax(map2genes) ; i++)
    defDatum(keySet(map2genes, i), cc, map2def) ;
  keySetDestroy(map2genes) ;
  keySetDestroy(map2def) ;
  defs  = keySetCreate() ;
  genes  = keySetCreate() ;
  n = arrayMax(cc) ;
  if (n) c = arrp(cc, 0, LINK) - 1 ;
  while(c++, n--)
    { keySetInsert(defs, c->a) ;
      keySetInsert(genes, c->b) ;
    }
  box = graphBoxStart() ;
  if (keySetMax(defs) && keySetMax(genes))
    graphText(messprintf(" %d data,  %d %s tested against %d %s",
			 arrayMax(cc), keySetMax(defs), className(keySet(defs, 0)),
			 keySetMax(genes), className(keySet(genes, 0))), 3, line++) ;
  graphText("I will now assemble these into contigs, please wait",
	    3,line++) ;
  graphBoxDraw(box, BLACK,WHITE) ;
  n = arrayMax(cc) ;
  vertex = arrayCreate(n/2, VERTEX) ;
  look->nbContig = topoConnectedComponents(cc, vertex) ;
  arrayMax(cc) = n ;
  box = graphBoxStart() ;
  graphText(messprintf("found %d contigs", look->nbContig), 2, line++) ;
  graphBoxDraw(box, BLACK,WHITE) ;
  histo = arrayCreate(20,int) ;
  n = arrayMax(vertex) ;
  while(n--)
    array(histo, arr(vertex,n,VERTEX).group, int)++ ;
  plotHisto("Genes or Def per  contig", histo) ;
  look->mapStatus |= F_HIST ;
  look->dataArray = cc ;
  if (!arrayMax(vertex))
    { arrayDestroy(look->dataArray) ;
      lookArrayDestroy(look) ;
      arrayDestroy(vertex) ;
      keySetDestroy(defs) ;
      keySetDestroy(genes) ;
      return FALSE ;
    }
  arrayDestroy(vertex) ;
  keySetDestroy(defs) ;
  keySetDestroy(genes) ;
  return TRUE ;
}
#endif /* !NON_GRAPHIC */

/***************************************************************/

#ifndef NON_GRAPHIC
static BOOL defCptParseDB(DEFCPT look)
{ KEYSET bb = 0, ks = 0 ;
  void *dummy ;
  char *cp = 0 ;
  BOOL ok;

  if (look->nbContig)
    messcrash("look pas reinitialise correctement") ;
  if (keySetActive(&bb, &dummy) && keySetMax(bb))
    { ks = query(bb, "CLASS YAC") ;
      if (!keySetMax(ks))
	keySetDestroy(ks) ;
    }
  graphActivate(look->defMapCtlGraph) ;
  graphPop() ;
  if (!ks)
    { 
      ACEIN name_in;
      if ((name_in = messPrompt("Give the name of the Map you want to analyse","","w", 0)))
	{
	  cp = strnew (aceInWord(name_in), 0);
	}
      else
	return FALSE ;
    }
  ok = defCptDoParseDB(look, ks, cp) ;

  messfree (cp);

  return ok;
} /* defCptParseDB */
#endif /* !NON_GRAPHIC */

/***************************************************************/

#ifndef NON_GRAPHIC
static void defCptDoGetData(DEFCPT look, int t)
{ 
  int i, j, n, box, line, no = 0 ;
  Array cc ; 
  LINK *c ;
  KEYSET *ksp ;
  KEY *keyp ;

  lookArrayDestroy(look) ;
  cc  = look->dataArray ;
  if(!cc)
    messcrash("defCptGetData receives a null Array") ;
  look->def = keySetCreate() ;
  look->mar = keySetCreate() ;
  n = arrayMax(cc) ;
  while(n--)
    { c = arrp(cc, n, LINK) ;
      if (t == c->group)
	{ keySetInsert(look->def, c->a) ;
	  keySetInsert(look->mar, c->b) ;
	}
    }
  look->nd = keySetMax(look->def) ;
  look->nm = keySetMax(look->mar) ;
  i = look->nd ;
  look->marInDef = arrayCreate(i, KEYSET) ;
  array(look->marInDef, i - 1, KEYSET) = 0 ;
  ksp = arrp(look->marInDef, 0, KEYSET) ;
  while(i--)
    *ksp++ = keySetCreate() ;
  i = look->nm ;
  look->defInMar = arrayCreate(look->nm, KEYSET) ;
  array(look->defInMar, i - 1, KEYSET) = 0 ;
  ksp = arrp(look->defInMar, 0, KEYSET) ;
  while(i--)
    *ksp++ = keySetCreate() ;
  n = arrayMax(cc) ;
  while(n--)
    { c = arrp(cc, n, LINK) ;
      if (t == c->group && c->type)
	{ 
	  if (keySetFind(look->def, c->a, &i) && keySetFind(look->mar, c->b, &j))
	    { ksp = arrp(look->marInDef, i, KEYSET) ;
	      keySet(*ksp, keySetMax(*ksp)) = j | (c->type) ;
	      ksp = arrp(look->defInMar, j, KEYSET) ;
	      keySet(*ksp, keySetMax(*ksp)) = i | (c->type) ;
	      if (c->type == F_NO)
		no = 1 ; /* pour savoir si que des in ou in et out */
	    }
	  else
	    messcrash("Cant find %d -> %s\n%d -> %s", i, name(c->a), j, name(c->b)) ;
	}
    }
  i = look->nd ;
  ksp = arrp(look->marInDef, 0, KEYSET) - 1 ;
  while(ksp++, i--)
    arraySort(*ksp, mardefOrder) ;
  i = look->nm ;
  ksp = arrp(look->defInMar, 0, KEYSET) - 1 ;
  while(ksp++, i--)
    arraySort(*ksp, mardefOrder) ;
  defCptGetPos(look) ;
  look->mapStatus = (1 | (look->mapStatus & F_HIST)) ;
  if (no)
    look->whatDis = (F_E_YN | 1) ;
  else /* que des yes => suppression des flag YES pour shortDistances */
    { i = arrayMax(look->marInDef) ;
      ksp = arrp(look->marInDef, 0, KEYSET) - 1 ;
      while(ksp++, i--)
	{ j = keySetMax(*ksp) ;
	  keyp = arrp(*ksp, 0, KEY) - 1 ;
	  while(keyp++, j--)
	    *keyp &= F_WHO ;
	}
      i = arrayMax(look->defInMar) ;
      ksp = arrp(look->defInMar, 0, KEYSET) - 1 ;
      while(ksp++, i--)
	{ j = keySetMax(*ksp) ;
	  keyp = arrp(*ksp, 0, KEY) - 1 ;
	  while(keyp++, j--)
	    *keyp &= F_WHO ;
	}
      look->whatDis = 1 ; /* que des Yes : unknown out (1) ou rien (0) ? */
    }
  if (look->selectedMap)
    defcomputeDraw (look, messprintf("Map : %s", name(look->selectedMap))) ;
  else defcomputeDraw (look, 0) ;
  line = 14 ;
  box = graphBoxStart () ;
  graphText(messprintf("Contig :  %d", t), 1, line);
  line += 2 ;
  if (look->nm && look->nd)
    graphText(messprintf("%d %s, %d %s",
			 look->nd, className(keySet(look->def, 0)),
			 look->nm, className(keySet(look->mar, 0))), 
	      12, line++) ;
  graphBoxDraw (box, BLACK, WHITE) ;
  look->Line = 8;
  if(!look->nm)
    { lookArrayDestroy(look) ;
      return ;
    }
} /* defCptDoGetData */
#endif /* !NON_GRAPHIC */

/***************************************************************/

#ifndef NON_GRAPHIC
static void defCptDoExportOrder(DEFCPT look, FILE *f, char buf[81])
{ char *buf2 ;
  int i, j, m, n, x, imax ;
  Array marOrder ;
  KEYSET *ksp ;
  KEY *keyp ;

  switch(look->method)
    { case M_EDWARD:
	imax = look->nd ;
	buf2 = className(keySet(look->def, 0)) ;
	for (i = 0 ; i < imax ; i++)
	  { j = arr(look->colOrder, i, int) ;
	    if (!(keySet(arr(look->marInDef, j, KEYSET), 0) & F_V_FLAG))
	      fprintf(f, "%s %s\nMap %s Position %d\n\n", buf2, name(keySet(look->def, j)), buf, i) ;
	  }
	look->method = M_FIN ;
#ifndef NON_GRAPHIC
	defcomputeDraw (look, 0) ;
#endif /* !NON_GRAPHIC */
	break ;
      default:
	imax = look->nm ;
	buf2 = className(keySet(look->mar, 0)) ;
	for (i = 0 ; i < imax ; i++)
	  { j = arr(look->linOrder, i, int) ;
	    fprintf(f, "%s %s\nMap %s Position %d\n\n", buf2, name(keySet(look->mar, j)), buf, i) ;
	  }
	buf2 = className(keySet(look->def, 0)) ;
	marOrder = defCptTranspMat(look->linOrder) ;
	ksp = arrp(look->marInDef, 0, KEYSET) ;
	for (i = 0 ; i < look->nd ; ksp++, i++)
	  { j = keySetMax(*ksp) ;
	    keyp = arrp(*ksp, 0, KEY) - 1 ;
	    x = look->nm ;
	    m = - 1 ;
	    while(keyp++, j--)
	      { if (!(*keyp & F_NO))
		  { if ((n = arr(marOrder, *keyp & F_WHO, int)) < x)
		      x = n ;
		    if (n > m)
		      m = n ;
		  }
	      }
	    if (m != -1)
	      { fprintf(f, "%s %s\nMap %s Left %d\n", buf2, name(keySet(look->def, i)), buf, x) ;
		fprintf(f, "Map %s Right %d\n\n", buf, m) ;
	      }
	  }
	break ;
      }
} /* defCptDoExportOrder */
#endif /* !NON_GRAPHIC */

/***************************************************************/

#ifndef NON_GRAPHIC
static void defCptExportOrder(void)
{ FILE *f ;
  KEY myMap, vMap ;
  char *tmpName  = 0 ;
  char *cp, buf[81], filname[FIL_BUFFER_SIZE] ;
  ACEIN name_in;
  DEFCPTGET("defCptExportOrder") ;

  TESLIN ;
  strcpy(filname, "best.tree") ;
  if (!(name_in = messPrompt("Please, give a name for the exported map", "Test", "w", 0)))
    return ;
  cp = aceInWord (name_in) ;
  strncpy(buf, cp, 81);
  buf[80] = '\0';
  aceInDestroy (name_in);

  lexaddkey (buf, &myMap, _VMap) ;
  f = filtmpopen(&tmpName,"w") ;
  if (!f)
    return ;
  defCptDoExportOrder(look, f, buf) ;
  filclose(f) ;
  /*
  if ((f = filopen (tmpName, 0, "r")))
    parseFile (f, 0, 0) ;
    */
  filtmpremove (tmpName) ;
  if (lexReClass(myMap, &vMap,_VvMap) &&
      !lexlock (vMap))
    arrayKill (vMap) ;
  display (myMap, 0, 0) ;
} /* defCptExportOrder */
#endif

/***************************************************************/

static int defCptContigOrder(void *a, void *b)
{ return
    ((KEY_INT*)b)->tai - ((KEY_INT*)a)->tai ;
}

/***************************************************************/

#ifndef NON_GRAPHIC
static void defCptAllContigs(DEFCPT look)
{ int i, j = 0 ;
  char *cp, buf[81], *buf2, *tmpName = 0 ;
  KEY link, myMap, vMap ;
  Array contig = 0 ;
  FILE *f ;
  ACEIN name_in;
  KEY_INT *kip ;

  if (!(name_in = messPrompt("Please, give a name for The Maps", "Test", "w", 0)))
    { look->mapStatus |= F_NO_CONTIG ;
      defcomputeDraw (look, 0) ;
      return ;
    }
  cp = aceInWord(name_in);
  strncpy(buf, cp, 81) ;
  buf[80] = '\0';
  aceInDestroy (name_in);

  f = filtmpopen(&tmpName, "w") ;
  if (!f)
    return ;
  look->display = FALSE ;
  lexaddkey(messprintf("_Assembly.%s", buf), &link, _VMap) ;
  contig = arrayCreate(look->nbContig, KEY_INT) ;
  arrayp(contig, look->nbContig - 1, KEY_INT)->key = 0 ;
  kip = arrp(contig, 0, KEY_INT) ;
  for (i = 1 ; i <= look->nbContig ; i++)
    { defCptDoGetData(look, i) ;
      defCptDoTree(look) ;
      defCptSortMarkers() ;
      buf2 = messprintf("%s.%d", buf, i) ;
      lexaddkey(buf2, &myMap, _VMap) ;
      kip->key = myMap ;
      (kip++)->tai = look->nm - 1 ;
      defCptDoExportOrder(look, f, buf2) ;
    }
  look->display = TRUE ;
  arraySort(contig, defCptContigOrder) ;
  i = look->nbContig ;
  kip = arrp(contig, 0, KEY_INT) - 1 ;
  while(kip++, i--)
    { if (kip->tai)
	{ fprintf(f, "Map %s\nMap %s Left %d\n", name(kip->key), name(link), j) ;
	  j += kip->tai ;
	  fprintf(f, "Map %s Right %d\n\n", name(link), j) ;
	  j += 20 ;
	}
    }
  filclose(f) ;
  arrayDestroy(contig) ;
  /*
  if (filopen(tmpName, 0, "r"))
    parseFile(f, 0, 0) ;
    */
  filtmpremove(tmpName) ;
  if (lexReClass(link, &vMap, _VvMap) && !lexlock(vMap))
    arrayKill(vMap) ;
  display(link, 0, 0) ;
  look->mapStatus |= F_NO_CONTIG ; /* a voir ou il faut retomber */
  defcomputeDraw (look, 0) ;
}
#endif /* !NON_GRAPHIC */

/***************************************************************/

#ifndef NON_GRAPHIC
static void defCptGetData(void)
{ 
  int t = 0 ;
  ACEIN num_in = 0; 
  DEFCPTGET("defCptGetData") ;

  look->mapStatus |= ~F_NO_CONTIG ;
  messStatus("Get Data") ;
  if (!look->dataArray && !defCptParseDB(look))
    { messout("I have no data to proceed") ;
      defcomputeDraw (look, 0) ;
      return ;
    }
  if (look->nbContig == 1)
    t = 1 ;
  else if (!(num_in = messPrompt("Def contig number ? (0 for All)","1","i", 0))
	   || !aceInInt(num_in, &t) || t > look->nbContig)
    {
      look->mapStatus |= F_NO_CONTIG ;
      if (t > look->nbContig)
	messout(messprintf("there are only %d contigs", look->nbContig)) ;
      if (look->selectedMap)
	defcomputeDraw (look, messprintf("Map : %s", name(look->selectedMap))) ;
      else 
	defcomputeDraw (look, 0) ;

      if (num_in) aceInDestroy (num_in);
      return ;
    }
  if (t)
    defCptDoGetData (look, t) ;
  else
    defCptAllContigs(look) ;

  if (num_in) aceInDestroy (num_in);
  return;
} /* defCptGetData */
#endif /* !NON_GRAPHIC */

/***************************************************************/

#ifndef NON_GRAPHIC
static void defCptDefDupHisto(void)
{ defCptHisto("Deficiency", "marker") ;
}
#endif /* !NON_GRAPHIC */

/***************************************************************/
/********************    Finger   Print    *********************/
/***************************************************************/
#ifndef NON_GRAPHIC
static void defCptGetClones(void) /* provisoirement pas de version tace de fp */
{
  extern void pmapCptGetData(Array marInDef, Array defInMar,
			     KEYSET clones, KEYSET bands) ;
  DEFCPTGET("defCptGetClones") ;

  messStatus("GetClones") ;
  look->def = keySetCreate() ;
  look->mar = keySetCreate() ;
  look->marInDef = arrayCreate(100, KEYSET) ;
  look->defInMar = arrayCreate(100, KEYSET) ;
  pmapCptGetData(look->marInDef, look->defInMar, look->def, look->mar) ;
  look->nm = keySetMax(look->mar) ;
  if (!look->nm)
    { lookArrayDestroy(look) ;
      return ;
    }
  look->nd = keySetMax(look->def) ;
  defCptGetPos(look) ;
  look->mapStatus = 1 ;
  look->whatDis = 1 ;
  defcomputeDraw (look, messprintf("Found %d Clones and %d Bands", look->nd, look->nm)) ;
} /* defCptGetClones */
#endif /* !NON_GRAPHIC */

/***************************************************************/

#ifndef NON_GRAPHIC
static void defCptFingPrHisto(void)
{ defCptHisto("Clone", "Band") ;
}
#endif /* pas de version tace pour le finger print */
/***************************************************************/
/********************  Sequence Alignment  *********************/
/***************************************************************/

static void defCptNbOligoChoice (DEFCPT look)
{ int i = 0 ;

  if ((i = keySetMax (look->def)))
    i = 1000 / i ;
  if (i < 2) i = 2 ;
  if (i > 60) i = 60 ;
  look->whatDis = (unsigned char)i ;
  sprintf (look->nboligo, "%d", i) ;
}

/***************************************************************/

#ifndef NON_GRAPHIC
static void defCptDoInsertBad(DEFCPT look)
{ dnaAlignInsertBad(look) ;
  if (diffaction)
    catText(diffaction, messprintf("insert // -> %d seq\n", keySetMax(look->def))) ;
  defcomputeDraw (look, messprintf("Found %d sequences", keySetMax(look->def))) ;
  defCptNbOligoChoice (look) ;
}
#endif /* !NON_GRAPHIC */

#ifndef NON_GRAPHIC
static void defCptInsertBad(void)
{ DEFCPTGET("insert Bad") ;

  defCptDoInsertBad(look) ;
}
#endif /* !NON_GRAPHIC */

/***************************************************************/

static void defCptDoFixSegConsensus(DEFCPT look, BOOL new)
{ KEYSET maille ;

  messStatus("Fix consensus") ;
  if (new || !new)  /* ulrich's fix */
    {
      maille = look->def ;
      look->def = keySetCreate() ;
      dnaAlignFixSegConsensus(look, maille) ;
      keySetDestroy(maille) ;
    }
  else
    {
    }
  if (diffaction)
    catText(diffaction, messprintf("fix // %s\n", timeShow(timeNow()))) ;
  look->mapStatus &= ~F_BOUT ;
  look->mapStatus |= (F_ASSSEG | 4 | F_IS_FIXED) ;
#ifndef NON_GRAPHIC
  defcomputeDraw (look, 0) ;
#endif /* !NON_GRAPHIC */
} /* defCptDoFixSegConsensus */

/***************************************************************/

#ifndef NON_GRAPHIC
static void defCptFixSegConsensus(void)
{ DEFCPTGET("fix seg") ;

  defCptDoFixSegConsensus(look, FALSE) ;
}
#endif /* !NON_GRAPHIC */

/***************************************************************/
/***************************************************************/

static void defCptDoTrainNNonKeySet (KEYSET aa)
{ int i ; 
  KEY key ;

  messStatus("AutoEdit") ;
  if (!aa)
    return ;
  for (i = 0 ; i < keySetMax(aa) ; i++)
    { key = keySet(aa, i) ;
      if (class (key) == _VDNA) 
	lexReClass (key, &key, _VSequence) ;
      if (class(key) == _VSequence)
	if (!nnContigTrain (key))
	  break ;
    }
  nnContigTrain (0) ; /* close export file */
  if (diffaction)
    catText(diffaction, messprintf("TrainNN // TrainNN contigs ; %s\n",
				   timeShow(timeNow()))) ;
}

/***************************************************************/
/***************************************************************/

static int defCptDoPatchKeySet (KEYSET aa)
{ int i, n, nn = 0 ; 
  KEY key ;

  messStatus("AutoEdit") ;
  if (!aa)
    return 0 ;
  for (i = 0 ; i < keySetMax(aa) ; i++)
    { key = keySet(aa, i) ;
      if (class (key) == _VDNA) 
	lexReClass (key, &key, _VSequence) ;
      n = 0 ;
      if (class(key) == _VSequence)
	if (!baseCallPatchContig (key, &n)) /* FALSE si F4 */
	  { nn += n ; break ; }
      nn += n ;
    }
  if (diffaction)
    catText(diffaction, messprintf("autoedit // autoedit contigs ; %s\n", 
				   timeShow(timeNow()))) ;
  return nn ;
}

/***************************************************************/

static int defCptDoBaseCall (KEYSET aa)
{ int i, n, nn = 0 ; 
  KEY key ;

  messStatus("defCptDoBaseCall") ;
  if (!aa)
    return 0 ;
  for (i = 0 ; i < keySetMax(aa) ; i++)
    { key = keySet(aa, i) ;
      if (class (key) == _VDNA) 
	lexReClass (key, &key, _VSequence) ;
      n = 0 ;
      if (class(key) == _VSequence)
	if (!baseCallRedoBaseCall (key, &n)) /* FALSE si F4 */
	  { nn += n ; break ; }
      nn += n ;
    }
  return nn ;
}

/***************************************************************/
#ifndef NON_GRAPHIC
/***************************************************************/

static void defCptPatchKeySet (void)
/* graphic */
{ DEFCPTGET ("defCptPatchKeySet") ;

  defCptDoPatchKeySet (look->def) ;
}

/***************************************************************/

#ifdef THIS_CODE_IS_NEVER_USED

static void defCptPatchActiveKeySet (void)
/* graphic */
{
  KEYSET aa ;
  int nn ;
  void *dummy ;
  DEFCPTGET("defCptPatchActiveKeySet") ;

  if (!isWriteAccess ())	
    { messout("Sorry, you do not have write access") ;
      return ;
    }

  if (!keySetActive(&aa, &dummy))
    { messout("First select a keyset containing sequences") ;
      return ;
    }
  nn = defCptDoPatchKeySet (aa) ;
  defcomputeDraw (look, messprintf ("Performed %d auto-editions", nn)) ;
}


#endif /* THIS_CODE_IS_NEVER_USED */

/***************************************************************/

static void defCptChoosePaire(KEY key)
/* graphic */
{ DEFCPTGET("Choose Paire") ;

  array(look->dataArray, arrayMax(look->dataArray), KEY) = key ;
  keySetSort(look->dataArray) ;
  keySetCompress(look->dataArray) ;
  if (arrayMax(look->dataArray) != 2)
    { displayRepeatBlock () ;
      return ;
    }
  if (arrayMax (look->dataArray) != 2)
    messcrash("Impossible Max pour dataArray") ;
  dnaAlignAsmbPaire (look, arr(look->dataArray, 0, KEY),
		    arr(look->dataArray, 1, KEY)) ;
  arrayDestroy (look->dataArray) ;
  defcomputeDraw(look, 0) ;
}

/***************************************************************/

static void defCptPaireDestroy(void)
/* graphic */
{ DEFCPTGET ("Paires Destroy") ;

  displayUnBlock () ;
  arrayDestroy(look->dataArray) ;
}

/***************************************************************/

static void defCptAsmbPaireActive (void)
/* graphic */
{ DEFCPTGET ("Asmb Paire") ;

  if (!isWriteAccess ())	
    { messout("Sorry, you do not have write access") ;
      return ;
    }
  if (look->dataArray)
    messcrash("look->dataArray should not exit") ;
  look->dataArray = arrayCreate(2, KEY) ;
  graphRegister(MESSAGE_DESTROY, defCptPaireDestroy) ;
  displayBlock (defCptChoosePaire, "Pick on sequence") ;
}

/***************************************************************/

static void defCptMakeSuperLink (void)
/* graphic */
{ KEY link ;
  KEYSET ks ;

  if (!isWriteAccess ())	
    { messout("Sorry, you do not have write access") ;
      return ;
    }
  if (!keySetActive (&ks, 0))
    { messout ("First select a keySet containing sequences") ;
      return ;
    }
  link = dnaAlignDoMakeSuperLink (ks, 0) ;
  display(link, 0, 0) ;
}
/***************************************************************/

static void defCptFixActKeyset (void)
/* graphic */
{ DEFCPTGET("fix keyset") ;

  dnaAlignFixActKeyset () ;
  look->mapStatus |= F_IS_FIXED ;
  defcomputeDraw (look, 0) ;
}

/***************************************************************/
#endif /* !NON_GRAPHIC */
/***************************************************************/

static void defCptDoExtendReadsExt (DEFCPT look)
{ messStatus("Extend Reads (ext)") ;
  abiFixExtend (look->link, look->def) ;
  look->mapStatus = (F_ASSSEG | 5 | (look->mapStatus & F_HIST)) ;
/* defCptDoFixSegConsensus (look) ; */
  if (diffaction)
    catText(diffaction, messprintf("extend  // %s ; extend reads ext\n", timeShow(timeNow()))) ;
#ifndef NON_GRAPHIC
  defcomputeDraw (look, 0) ;
#endif /* !NON_GRAPHIC */
} /* defCptDoExtendReadsExt */

#ifndef NON_GRAPHIC
static void defCptExtendReadsExt (void)
{ DEFCPTGET("extend read") ;

  defCptDoExtendReadsExt (look) ;
}
#endif /* !NON_GRAPHIC */

/***************************************************************/

static void defCptDoGetOligos(DEFCPT look)
{ int i ;
  KEYSET *ksp ;

  if (look->manEntry)
    { freeforcecard(look->nboligo) ;
      freeint(&i) ;
      if (i > 0 && i < 256)
	look->whatDis = (unsigned char)i ;
    }
  if (!look->def)
    look->def = keySetCreate () ;
  look->mar = keySetReCreate(look->mar) ;
  if (arrayExists(look->marInDef))
    { if ((i = arrayMax(look->marInDef)))
	{ ksp = arrp(look->marInDef, 0, KEYSET) - 1 ;
	  while(ksp++, i--)
	    keySetDestroy(*ksp) ;
	}
    }
  look->marInDef = arrayReCreate(look->marInDef, 100, KEYSET) ;
  if (arrayExists(look->defInMar))
    { if ((i = arrayMax(look->defInMar)))
	{ ksp = arrp(look->defInMar, 0, KEYSET) - 1 ;
	  while(ksp++, i--)
	    keySetDestroy(*ksp) ;
	}
    }
  look->defInMar = arrayReCreate(look->defInMar, 100, KEYSET) ;
  messStatus("Get Sequences") ;
  dnaAlignGetData(look) ;
  if ((look->nd = keySetMax(look->def)) < 2)
    { messout("GetOligo: Only %d sequence, i can't get", look->nd) ;
      if (!look->mapStatus) /* i e c'est le premier tour et l'utilisateur */
	{ keySetDestroy(look->def) ; /* a charge 1 seq. => retour au depart */
	  keySetDestroy(look->mar) ;
	}
#ifndef NON_GRAPHIC
      defcomputeDraw (look, 0) ;
#endif /* !NON_GRAPHIC */
      return ;
    }
  if(!(look->nm = keySetMax(look->mar)))
    { messout("I haven't found any oligo") ; /* idem */
      if (!look->mapStatus) /* attention look->mapStatus ne doit jamais etre nul en dehors du depart */
	{ keySetDestroy(look->def) ;
	  keySetDestroy(look->mar) ;
	}
#ifndef NON_GRAPHIC
      defcomputeDraw (look, 0) ;
#endif /* !NON_GRAPHIC */
      return ;
    }
  if (diffaction)
    catText(diffaction, messprintf("get %d // %s\n", look->whatDis, timeShow(timeNow()))) ; /* pour faire le fichier des actions */
  arrayDestroy(look->colOrder) ;
  arrayDestroy(look->linOrder) ;
  TESCOLO ;
  TESLIN ;
  look->mapStatus &= ~F_BOUT ;
  look->mapStatus |= (F_HIST | 1) ;
#ifndef NON_GRAPHIC
  defcomputeDraw (look, 0) ;
#endif /* !NON_GRAPHIC */
} /* defCptDoGetOligos */

/***************************************************************/

#ifndef NON_GRAPHIC
static void defCptGetOligos(void)
{ DEFCPTGET("get sequences") ;

  defCptDoGetOligos(look) ;
}
#endif /* !NON_GRAPHIC */

/***************************************************************/

#ifndef NON_GRAPHIC
static void defCptGetMoreSequences(void)
{
  int i, j ;
  char buf[1000] ;
  ACEIN num_in;
  DEFCPTGET("more oligo") ;

  sprintf(buf, "How many oligos do you want") ;
  j = (look->whatDis + 2) ;
 ici:
  if ((num_in = messPrompt(buf, messprintf("%d", j), "i", 0)))
    { 
      aceInInt(num_in, &i) ;
      aceInDestroy (num_in);
      if (i < 0 || i > 256)
	{ sprintf(buf, messprintf("Bad value (%d) ; please try again", i)) ;
	  goto ici ;
	}
      look->whatDis = (unsigned char)i ;
      sprintf(look->nboligo, "%d", i) ;
    }
  else 
    return ;

  look->manEntry = FALSE ; /* kludge pour imposer un nb oligos a GetOligos */
  defCptDoGetOligos(look) ;
  look->manEntry = TRUE ;
  
  return;
} /* defCptGetMoreSequences */
#endif /* !NON_GRAPHIC */

/***************************************************************/
/*  probablement inutile
static void defCptMakeOneSegment(void)
{ int i, j, k = 0, imax, jmax ;
  TREE_DEF maille, *ksmp ;

  if (arrayMax(look->maillon) == 1)
    { messout("This work is already done") ;
      return ;
    }
  imax = arrayMax(look->maillon) ;
  maille.ks = keySetCreate() ;
  maille.x = keySetCreate() ;
  ksmp = arrp(look->maillon, 0, TREE_DEF) ;
  for (i = 0 ; i < imax ; ksmp++, i++)
    { jmax = keySetMax(ksmp->ks) ;
      for (j = 0 ; j < jmax ; j++)
	{ keySet(maille.ks, k) = keySet(ksmp->ks, j) ;
	  keySet(maille.x, k++) = keySet(ksmp->x, j) ;
	}
      keySetDestroy(ksmp->ks) ;
      keySetDestroy(ksmp->x) ;
    }
  look->maillon = arrayReCreate(look->maillon, 1, TREE_DEF) ;
  array(look->maillon, 0, TREE_DEF) = maille ;
  if (diffaction)
    catText(diffaction, "one\n") ;
}
*/
/***************************************************************/

static BOOL defCptFixErrorRate (DEFCPT look)
{
  int mu ;
  ACEIN rate_in;

  if (look->manEntry)
    { freeforcecard(look->choix) ;
      freeint(&mu) ;
      if (mu > 50 || mu < 0)
	{ if (mu < 0)
	    messout("Please, choose a rate (%d) between 0 and 50", mu) ;
	  if (mu > 50)
	    messout("Are you sure to want %d %% error ; please confirm", mu) ;
	ici:
	  if (!(rate_in = messPrompt("error rate", messprintf("%d", mu), "i", 0)))
	    return FALSE ;
	  aceInInt(rate_in, &mu) ;
	  aceInDestroy (rate_in);
	  if (mu < 0)
	    { messout("Please, choose a value (%d) between 0 and 50", mu) ;
	      goto ici ;
	    }
	  sprintf(look->choix, "%d", mu) ;
	}
      look->taux = mu ;
    }
  return TRUE ;
} /* defCptFixErrorRate */

/***************************************************************/

static void defCptEndAssfunc(DEFCPT look, KEYSET ks)
{ look->mapStatus = (F_ASSSEG | 5 | (look->mapStatus & F_HIST)) ;
#ifndef NON_GRAPHIC
  defcomputeDraw (look, messprintf("Found %d contigs, rejected %d Baddies",
				   keySetMax(look->def), look->rejected ?
				   keySetMax (look->rejected) : 0)) ;
#endif /* !NON_GRAPHIC */
  if (keySetMax(look->def))
    keySetDestroy(ks) ;
  else
    { keySetDestroy(look->def) ;
      look->def = ks ;
    }
  keySetDestroy(look->mar) ;
  defCptNbOligoChoice (look) ;
}

/***************************************************************/

static void defCptDoSegments(DEFCPT look)
{ KEYSET defbis = 0 ;

  messStatus("Assembling") ;
  if (!defCptFixErrorRate(look))
    return ;
  defbis = look->def ;
  look->def = keySetCreate() ;
  if (!look->maillon || !arrayMax (look->maillon))
    { messout ("sorry no assembly") ;
      goto abort ;
    }
  dnaAlignCptSegments(look) ;

  if (diffaction)
    catText(diffaction, messprintf("assemble %d // -> Found %d contigs, rejected %d Baddies ; %s\n",
				   look->taux, keySetMax(look->def),
				   look->rejected ?
				   keySetMax(look->rejected) : 0, 
				   timeShow(timeNow()))) ;
 abort:
  defCptEndAssfunc(look, defbis) ;
}

/***************************************************************/

static Array defCptCollectDummy (KEY join, KEYSET jj, int niveau)
{ KEY key ;
  BSunit *u ;
  OBJ Join ;
  Array units = 0, new = 0, daughter = 0 ;
  int p1, p2, x1 , x2,  dummy, i, i1, j = 0 ;

  if (class(join) == _VDNA)
    lexReClass(join, &join, _VSequence) ;
  Join = bsCreate(join) ;
  if (!Join)
    return 0 ;
  units = arrayCreate (90, BSunit) ;
  new = arrayCreate (90, BSunit) ;
  if (bsFindTag (Join, _Previous_contig) &&
      bsFlatten (Join, 3, units))
    { for (i = 0 ; i < arrayMax(units) ; i += 3)
	{ u = arrp(units,i,BSunit) ;
	  key = u[0].k ;
	  p1 = u[1].i ; p2 = u[2].i ;
	  if (keySetFind (jj, key, &dummy))
	    { daughter = defCptCollectDummy (key, jj, niveau + 1) ;
	      if (daughter) for (i1 = 0 ; i1 < arrayMax(daughter) ; i1 += 3)
		{ u = arrp(daughter,i1,BSunit) ;
		  array(new, j++, BSunit).k = u[0].k ;
		  if (p1 < p2)
		    { x1 = p1 + u[1].i - 1 ; x2 = p1 + u[2].i - 1 ; }
		  else
		    { x1 = p1 - u[1].i + 1 ; x2 = p1 - u[2].i + 1 ; }		    
		  array(new, j++, BSunit).i = x1 ;
		  array(new, j++, BSunit).i = x2 ;
		}
	      arrayDestroy (daughter) ;
	    }
	  else
	    { array(new, j++, BSunit).k = key ;
	      array(new, j++, BSunit).i = p1 ;
	      array(new, j++, BSunit).i = p2 ;
	    }
	  
	}
    }
  arrayDestroy (units) ;
  bsDestroy (Join) ;
  if (niveau)
    return new ;
  Join = bsUpdate(join) ;
  if (Join && bsAddArray (Join, _Previous_contig, new, 3))
    bsSave (Join) ;
  else
    bsDestroy (Join) ;
  arrayDestroy (new) ;
  return 0 ;    
}

/***************************************************************/

static int defCptDoJoinDiagonal (DEFCPT look)
{ int i, j = 0, nj = 0, fl, imax ;
  KEY key1, key2, join, link ;
  KEYSET ks = 0, ks1 = 0, ks2 = 0, ks3 = 0 ;
  Array subseq = 0, flag = 0 ;
  OBJ obj = 0 ;
  mytime_t tim ;
  char *cp ;

  messStatus ("Joining") ;
  if (!look || !look->link)
    return 0 ;
  if (class (look->link) == _VSequence)
    cp = dnaAlignNewName (name (look->link)) ;
  else
    cp = dnaAlignNewName (0) ;
  subseq = arrayCreate (20, BSunit) ;
  if (!(obj = bsCreate (look->link)) || !bsFindTag (obj, _Subsequence) ||
      !bsFlatten (obj, 1, subseq) || (arrayMax (subseq) < 2))
    goto abort ;
  bsDestroy (obj) ;
  i = arrayMax (subseq) ;
  flag = arrayCreate (i, int) ;
  array (flag, i-1, int) = 0 ;
  for (i = 1 ; i < arrayMax (subseq) ; i++)
    { key1 = arrp (subseq, i - 1, BSunit)->k ;
      key2 = arrp (subseq, i, BSunit)->k ;
      fl = arr (flag, i - 1, int) + 2 * arr (flag, i, int) ;
      if (!key1 || !key2)
	continue ;
/* verification qu'il y a au moins un subclone en commun */
      ks1 = queryKey (key1, ">Assembled_from ; >Subclone") ;
      ks2 = queryKey (key2, ">Assembled_from ; >Subclone") ;
      ks3 = keySetAND (ks1, ks2) ;
      imax = keySetMax (ks3) ;
      keySetDestroy (ks1) ;
      keySetDestroy (ks2) ;
      keySetDestroy (ks3) ;
      if (!imax)
	continue ;
/* fin de verif */

      join = dnaAlignAsmbPaire21 (key1, key2, 0, 0, 7, fl, cp) ;
      if (!join)
	join = dnaAlignAsmbPaire21 (key1, key2, 0, 0, 4, fl, cp) ;
      if (!join)
	join = dnaAlignAsmbPaire21 (key1, key2, 0, 0, -1, fl, cp) ;
      if (!join)
	continue ;
      nj++ ;
      arrp (subseq, i - 1, BSunit)->k = 0 ;
      arrp (subseq, i, BSunit)->k = join ;
      arr (flag, i, int) = 1 ;
    }
  if (!nj)
    goto abort ;
  lexaddkey (cp, &link, _VSequence) ;
  tim = timeParse ("today") ;
  if ((obj = bsUpdate (link)))
    if (bsAddKey (obj, _Derived_from, look->link))
      bsAddData (obj, _bsRight, _DateType, &tim) ;
  bsSave (obj) ;
  defCptChangeLook (look, look->link, link) ;
  ks = keySetCreate () ;
  look->def = keySetReCreate (look->def) ;
  for (i = 0 ; i < arrayMax (subseq) ; i++)
    if ((join = arrp (subseq, i, BSunit)->k))
      { 
	if (arr (flag, i, int))
	  dnaAlignFixContig (look->link, join) ;
	else if (!dnaAlignCopyContig (join, &join, cp, 0))
	  continue ;
	keySet (ks, j) = join ;
	lexReClass (join, &join, _VDNA) ;
	keySet (look->def, j++) = join ;
      }
  alignToolsAdjustLink (look->link, ks, 0) ;
  keySetDestroy (ks) ;
  keySetSort (look->def) ;
 abort:
  messfree (cp) ;
  bsDestroy (obj) ;
  arrayDestroy (subseq) ;
  return nj ;
}

/***************************************************************/

static int defCptDoJoinSegments(DEFCPT look)
{ KEY key, key1, join ;
  int i, j, max, nj = 0 ;
  Array defOrd = 0 ;
  OBJ obj = 0 ;
  KEYSET def, aa = keySetCreate() , jj = keySetCreate () ;

  messStatus ("Joining") ;
  def = look->def ;
  if (!def || !arrayMax(def))
    return 0 ;
  if (!look->tour)		/* premier passage */
    alignToolsDestroy_Segs (look->id) ;
  look->tour++ ;
  defOrd = arrayCreate (arrayMax (def), KEY_INT) ;
  j = 0 ; nj = 0 ;
  for (i = 0 ; i < keySetMax(def) ; i++)
    { key1 = keySet(def, i) ;
      if (class(key1) == _VDNA)
	lexReClass(key1, &key1, _VSequence) ;
      keySet(def, i) = key1 ;
      if ((obj = bsCreate (key1)) && bsGetKey (obj, _DNA, &key) &&
	  bsGetData (obj, _bsRight, _Int, &max))
	{ arrayp (defOrd, j, KEY_INT)->key = key1 ;
	  arrp (defOrd, j++, KEY_INT)->tai = max ;
	}
      bsDestroy (obj) ;
    }
  arraySort (defOrd, defCptContigOrder) ;
  for (i = 0 ; i < arrayMax(defOrd) ; i++)
    { key = arrp (defOrd, i, KEY_INT)->key ;
      if (!key)
	continue ;
      if (arrp (defOrd, i, KEY_INT)->tai < 2000) /* end the loop */
	break ;
      for (j = i + 1 ; j < arrayMax(defOrd) ; j++)
      { key1 = arrp (defOrd, j, KEY_INT)->key ;
	if (!key1)
	  continue ;
	join = dnaAlignAsmbPaire21 (key, key1, look->id, look->tour, 7, 0, 0) ;
	if (!join)
	  continue ;
	keySet (jj, nj++) = join ;
	if (class(join) == _VDNA)
	  lexReClass(join, &join, _VSequence) ;
	key = arrp (defOrd, i, KEY_INT)->key = join ;
	arrp (defOrd, j, KEY_INT)->key = 0 ;
	j = i ; /* reiterate the inner j loop */
      }
    }
  /* collect the contigs */
  aa = keySetCreate () ; j = 0 ;
  for (i = 0 ; i < arrayMax(defOrd) ; i++)
    { key = arrp (defOrd, i, KEY_INT)->key ;
      if (key)
	{ if (class(key) == _VSequence)
	    lexReClass(key, &key, _VDNA) ;
	  keySet (aa, j++) = key ;
	}
    }
  arrayDestroy (defOrd) ;

  if (look->mapStatus && arrayMax(def))
    { if (nj)
	look->mapStatus = (F_ASSSEG | 5 | (look->mapStatus & F_HIST)) ;
#ifndef NON_GRAPHIC
      defcomputeDraw (look, 
	 messprintf(" // I made %d joins -> %d contigs", nj, keySetMax (look->def))) ;
#endif /* !NON_GRAPHIC */
    }
  if (diffaction)
    catText(diffaction, messprintf("join // -> Made %d joins ; %s\n",
				   nj, timeShow(timeNow()))) ;
  keySetSort(aa) ;
  keySetCompress(aa) ;
  keySetDestroy (def) ;
  look->def = arrayCopy (aa) ;
  if (look->display)
    dnaDispGraph (aa, look->tour) ;
  for (i = 0 ; i < arrayMax(aa) ; i++)
    defCptCollectDummy (keySet (aa, i), jj, 0) ;
  keySetDestroy (jj) ;
  if (!nj) /* no join */
    look->tour-- ; /*en reponse au ++ du haut de la fonction */
  return nj ;
}

/***************************************************************/

#ifndef NON_GRAPHIC
static void defCptJoinSegments(void)
{ DEFCPTGET("Joining Segments") ;
  defCptDoJoinSegments (look) ;
}
#endif /* !NON_GRAPHIC */

/***************************************************************/

#ifndef NON_GRAPHIC
static void defCptSegments(void)
{ DEFCPTGET("Compute Segments") ;

  defCptDoSegments(look) ;
}
#endif /* !NON_GRAPHIC */

/***************************************************************/
/* tri les segments qui restent dans l'ordre decroissant de la taille
   et essaye de les assembler dans cet ordre */
#ifndef NON_GRAPHIC
static void defCptDoTryAll(DEFCPT look)
{ KEYSET defbis ;

  messStatus("Assembling all") ;
  if (!defCptFixErrorRate(look))
    return ;
  defbis = look->def ;
  look->def = keySetCreate() ;
  dnaAlignTryAll (look) ;
  if (diffaction)
    catText(diffaction, messprintf("tryall %d // -> Found %d contigs, rejected %d Baddies ; %s\n",
				   look->taux, keySetMax(look->def),
				   look->rejected ? 
				   keySetMax (look->rejected) : 0,
				   timeShow(timeNow()))) ;
  defCptEndAssfunc(look, defbis) ;
} /* defCptDoTryAll */
#endif /* !NON_GRAPHIC */

/***************************************************************/

#ifndef NON_GRAPHIC
static void defCptTryAll(void)
{ DEFCPTGET("Try All") ;

  defCptDoTryAll(look) ;
}
#endif /* !NON_GRAPHIC */

/***************************************************************/

#ifndef NON_GRAPHIC
static void defCptDoNewTryPaires(DEFCPT look)
{ KEYSET defbis ;

  messStatus("Assembling Paires") ;
  if (!defCptFixErrorRate(look))
    return ;
  defbis = look->def ;
  look->def = keySetCreate() ;
  dnaAlignNewTryPaires(look) ;
  if (diffaction)
    catText(diffaction, messprintf("newtrypaires %d // -> Found %d contigs, rejected %d Baddies ; %s\n",
				   look->taux, keySetMax(look->def),
				   look->rejected ? 
				   keySetMax (look->rejected) : 0,
				   timeShow(timeNow()))) ;
  defCptEndAssfunc(look, defbis) ;
}
#endif /* !NON_GRAPHIC */

/***************************************************************/

#ifndef NON_GRAPHIC
static void defCptNewTryPaires(void)
{ DEFCPTGET ("Try Paires") ;

  defCptDoNewTryPaires(look) ;
}
#endif /* !NON_GRAPHIC */

/***************************************************************/

static BOOL defCptReport (KEY link, Array tgdContig, int tglimit, int *nTGcontig, int *nTGreads,
			  int *nTGbase, int limit, int *nGcontig, int *nGreads, int *nGbase, 
			  int *nPcontig, int *nPreads, int *nPbase)
{ OBJ obj = 0 ;
  Array units = 0, unit2 = 0 ;
  BSunit *u ;
  int i, j = 0, max, nbr, ntgc = 0, ntgr = 0, ntgb = 0, ngc = 0, ngr = 0, ngb = 0 ;
  int npc = 0, npr = 0, npb = 0 ;
  KEY dnakey ;

  units = arrayCreate (30, BSunit) ;
  if (!(obj = bsCreate (link)) || !bsFindTag (obj, _Subsequence) ||
      !bsFlatten (obj, 1, units) || !(i = arrayMax (units)))
    { arrayDestroy (units) ;
      bsDestroy (obj) ;
      return FALSE ;
    }
  bsDestroy (obj) ;
  u = arrp (units, 0, BSunit) - 1 ;
  while (u++, i--)
    {
      if (!(obj = bsCreate (u->k)))
	continue ;
      if (bsGetKey (obj, _DNA, &dnakey) && 
	  bsGetData (obj, _bsRight, _Int, &max))
	{ unit2 = arrayReCreate (unit2, 200, BSunit) ;
	  if (bsFindTag (obj, _Assembled_from) && bsFlatten (obj, 1, unit2))
	    nbr = arrayMax (unit2) ;
	  else nbr = 1 ; /* should not happen */
	  if (max > tglimit)
	    { array (tgdContig, j++, int) = max ;
	      array (tgdContig, j++, int) = nbr ;
	      ntgc++ ;
	      ntgr += nbr ;
	      ntgb += max ;
	    }
	  else if (max > limit) 
	    { ngc++ ;
	      ngr += nbr ;
	      ngb += max ;
	    }
	  else
	    { npc++ ;
	      npr += nbr ;
	      npb += max ;
	    }
	}
      bsDestroy (obj) ;
    }
  arrayDestroy (units) ;
  arrayDestroy (unit2) ;
  *nTGcontig = ntgc ;
  *nTGreads = ntgr ;
  *nTGbase = ntgb ;
  *nGcontig = ngc ;
  *nGreads = ngr ;
  *nGbase = ngb ;
  *nPcontig = npc ;
  *nPreads = npr ;
  *nPbase = npb ;
  return TRUE ;
}

/***************************************************************/

static void defCptPreReport (DEFCPT look)
{ int i, ntgc, ntgr, ntgb, ngc, ngr, ngb, npc, npr, npb, nrr ;
  Array tgdContig ;
  KEYSET ks ;

  if (!(look->link))
    { if (look->def)
	fprintf (defCptOut, "%d Contigs", keySetMax (look->def)) ;
      else
	fprintf (defCptOut, "No sequences loaded") ;
      return ;
    }
  tgdContig = arrayCreate (30, int) ;
  if (defCptReport (look->link, tgdContig, 3000, &ntgc, &ntgr, &ntgb, 1000,
		    &ngc, &ngr, &ngb, &npc, &npr, &npb))
    { fprintf (defCptOut, "\n// Resultat :\n") ;
      fprintf (defCptOut, "// Assembly %s\n", name (look->link)) ;
      if (arrayMax (tgdContig))
	{ fprintf (defCptOut, "// Contig > 3 kb : %d\n", arrayMax (tgdContig) / 2) ;
	  fprintf (defCptOut, "//         Total : %d bases, %d reads\n", ntgb, ntgr) ;
	  for (i = 0 ; i < arrayMax (tgdContig) ; i += 2)
	    fprintf (defCptOut, "//      %d bases en %d reads\n", arr(tgdContig, i, int),
		     arr (tgdContig, i + 1, int)) ;
	}
      ks = query (0,"Find Read") ; nrr = keySetMax(ks) ; keySetDestroy (ks) ;
      fprintf (defCptOut, "// Contig between 3 and 1 kb : %d\n", ngc) ;
      fprintf (defCptOut, "//             Total : %d bases, %d reads\n", ngb, ngr) ;
      fprintf (defCptOut, "// Contig <  1 kb : %d\n", npc) ;
      fprintf (defCptOut, "//             Total : %d bases, %d reads\n", npb, npr) ;
      fprintf (defCptOut, "// Total number of contigs            : %8d\n", ntgc+ngc+npc) ;
      fprintf (defCptOut, "// Total number of incorporated reads : %8d/%d present in database\n", ntgr+ngr+npr, nrr) ;
      fprintf (defCptOut, "// Total number of bases              : %8d\n", ntgb+ngb+npb) ;
    }
  arrayDestroy (tgdContig) ;
}

/***************************************************************/

#ifndef NON_GRAPHIC
static void defCptSaveAs(void)
{
  char *cq = 0 ;
  DEFCPTGET("defCptSaveAs") ;

  dnaAlignSaveAs(look, &cq) ;
  look->step = 0 ;
  if (cq)
    {
      if (diffaction)
	catText(diffaction, messprintf("save %s // save consensus as ; %s", cq, timeShow(timeNow()))) ;
      messfree(cq) ;
    } /* faut-il stocker le nom du consensus dans le game ou non ? */
} /* defCptSaveAs */
#endif /* !NON_GRAPHIC */

/***************************************************************/

#ifndef NON_GRAPHIC
static void defCptAssHisto(void)
{ defCptHisto("Sequence", "Oligo") ;
}
#endif /* !NON_GRAPHIC */

/***************************************************************/
/********************   Genes  Alignment   *********************/
/***************************************************************/

#ifndef NON_GRAPHIC
static void defCptFilter(DEFCPT look, int limit)
{ int i, ng ;
  KEYSET *kstmp ;

  ng = arrayMax(look->defInMar) ;
  i = arrayMax(look->marInDef) ;
  kstmp = arrp(look->marInDef, 0, KEYSET) - 1 ;
  while(kstmp++, i--)
    if ((100 * keySetMax(*kstmp)) / ng < limit)
      keySet(*kstmp, 0) |= F_ISOLE ;
} /* defCptFilter */
#endif /* !NON_GRAPHIC */

/***************************************************************/

/* dans ce cas les def sont des alleles de genes et les genes sont les
animaux ou on a teste les alleles comme issus du pere ou de la mere */

#ifndef NON_GRAPHIC
static void defCptDoGetGenes(DEFCPT look, KEYSET aa)
{ KEYSET tmp, tmq, *tmr ;
  KEY key, newkey ;
  int i, j, k, limit ;
  OBJ obj ;
  KEY _LocusP, _LocusM ;
  ACEIN answer_in;

  lexaddkey ("LocusP", &_LocusP, 0) ;
  lexaddkey ("LocusM", &_LocusM, 0) ;

  /* aa is a set of Loci */  
  look->mar = query(aa, "{> GameteP } $| {> GameteM}") ;
  if (!keySetMax(look->mar))
    { keySetDestroy(look->mar) ;
      messout("First (2) select a keyset containing gametes") ;
      return ;
    }
  messStatus("Get Genes") ;
  chrono("Matrice") ;
  tmp = query(look->mar, messprintf("> LocusP")) ;
  tmq = query(look->mar, messprintf("> LocusM")) ;
  look->def = keySetOR(tmp, tmq) ;
  keySetDestroy(tmp) ;
  keySetDestroy(tmq) ;
  if (!keySetMax(look->def))
    { keySetDestroy(look->mar) ;
      keySetDestroy(look->def) ;
      messout("First (3) select a keyset containing gametes") ;
      return ;
    }
  keySetSort(look->mar) ;
  keySetCompress(look->mar) ;
  keySetSort(look->def) ;
  keySetCompress(look->def) ;
  look->nd = keySetMax(look->def) ;
  look->nm = keySetMax(look->mar) ;
  i = look->nd ;
  look->marInDef = arrayCreate(i, KEYSET) ;
  array(look->marInDef, i - 1, KEYSET) = 0 ;
  tmr = arrp(look->marInDef, 0, KEYSET) - 1 ;
  while(tmr++, i--)
    *tmr = keySetCreate() ;
  i = look->nm ;
  look->defInMar = arrayCreate(i, KEYSET) ;
  array(look->defInMar, i - 1, KEYSET) = 0 ;
  tmr = arrp(look->defInMar, 0, KEYSET) - 1 ;
  while(tmr++, i--)
    *tmr = keySetCreate() ;
  for (j = 0 ; j < look->nm ; j++)
    { key = keySet(look->mar, j) ;
      tmq = arr(look->defInMar, j, KEYSET) ;
      if ((obj = bsCreate(key)))
	{ k = F_PERE ;
	  if (bsGetKey(obj, _LocusP, &newkey))
	    do { keySetFind(look->def, newkey, &i) ;
		 tmp = arr(look->marInDef, i, KEYSET) ;
		 keySet(tmp, keySetMax(tmp)) = (j | k) ;
		 keySet(tmq, keySetMax(tmq)) = (i | k) ;
	       } while(bsGetKey(obj, _bsDown, &newkey)) ;
	  k = F_MERE ;
	  if (bsGetKey(obj, _LocusM, &newkey))
	    do { keySetFind(look->def, newkey, &i) ;
		 tmp = arr(look->marInDef, i, KEYSET) ;
		 keySet(tmp, keySetMax(tmp)) = (j | k) ;
		 keySet(tmq, keySetMax(tmq)) = (i | k) ;
	       } while(bsGetKey(obj, _bsDown, &newkey)) ;
	  bsDestroy(obj) ;
	}
    }
  i = look->nd ;
  tmr = arrp(look->marInDef, 0, KEYSET) - 1 ;
  while(tmr++, i--)
    arraySort(*tmr, mardefOrder) ;
  i = look->nm ;
  tmr = arrp(look->defInMar, 0, KEYSET) - 1 ;
  while(tmr++, i--)
    arraySort(*tmr, mardefOrder) ;
  arrayDestroy (look->colOrder) ;
  arrayDestroy (look->linOrder) ;
  TESCOLO ;
  TESLIN ;
  chronoReturn() ;
 ici:
  if (answer_in = messPrompt("unknown limit", "", "i", 0))
    { aceInInt(answer_in, &limit) ;
      aceInDestroy(answer_in);
      if (limit < 0 || limit > 100)
	{ messout("please choose a value between 0 and 100") ;
	  goto ici ;
	}
      defCptFilter(look, 100 - limit) ;
    }
  chrono("tree") ;
  look->dlimit = 100 ;
  intCptTree(look) ;
  chronoReturn() ;
  if (look->defMapCtlGraph)
    graphActivate(look->defMapCtlGraph) ;
  defCptSortMarkers() ;
  defMapDisplay() ;
  look->mapStatus = 1 ;
  defcomputeDraw (look, 0) ;
}
#endif /* !NON_GRAPHIC */

/***************************************************************/

#ifndef NON_GRAPHIC
static void defCptGetGenes(void)
{ KEYSET aa ;
  void *dummy ;
  DEFCPTGET("Get Genes") ;

  if (!keySetActive(&aa, &dummy))
    { messout("First (1) select a keyset containing gametes") ;
      return ;
    }
  defCptDoGetGenes(look, aa) ;
}
#endif

/***************************************************************/

KEY lastAssembly (void) 
{
  KEY last = 0 ;
  int i ;
  KEYSET ks ;

  defcptInit() ;
  
  ks = query (0, ">?Assembly NOT IS _*") ;
  i = keySetMax (ks) ;
  if (i) last = keySet (ks, i - 1) ;
  keySetDestroy (ks) ;

  return last ;
}

/***************************************************************/

static void defCptJuxtapose (DEFCPT look)
{ int n ;
  KEYSET ks0, ks1, ks2, ks3, ks4 ;
  KEY key ;

  ks0 = keySetCreate () ;
  for (n = 0 ; n < keySetMax(look->def) ; n++)
    { lexReClass (keySet(look->def, n), &key, _VSequence) ;
      keySet (ks0, n) = key ; 
    }
  ks1 = query (ks0, "{NOT Subsequence } $| {> Subsequence}") ;
  keySetDestroy (look->def) ;

  ks2 = query (ks1, "!Assembled_from AND DNA") ;
  ks3 = query (ks1, "Assembled_from") ;
  ks4 = keySetOR (ks2, ks3) ;
  keySetSort (ks4) ;
  keySetCompress (ks4) ;
  contigLengthSort (ks4) ;
  look->def = ks4 ; ks4 = 0 ;
  if (!look->link)
    lexaddkey ("Phrap", &look->link, _VSequence) ;
  alignToolsAdjustLink (look->link, look->def, 0) ;
  keySetDestroy (ks0) ;
  keySetDestroy (ks1) ;
  keySetDestroy (ks2) ;
  keySetDestroy (ks3) ;

  return;
} /* defCptJuxtapose */

/***************************************************************/
#ifndef NON_GRAPHIC
static void defCptMakeStatErreur (void)
{ KEYSET aa ;
  void *dummy ;

  if (!keySetActive(&aa, &dummy))
    { messout("First select a keyset containing sequences") ;
      return ;
    }
  statisticsMakeErreur (aa) ;
}

/***************************************************************/

static void defCptMakeStatBarreaux (void)
{ KEYSET aa ;
  void *dummy ;

  if (!keySetActive(&aa, &dummy))
    { messout("First select a keyset containing sequences") ;
      return ;
    }
  statisticsCountGroup (aa) ;
}
#endif /* !NON_GRAPHIC */

/***************************************************************/
/********************    Menu   options    *********************/
/***************************************************************/

extern int assBounce , assFound , assNotFound , assInserted , assRemoved  ;

static void defCptDoReadAction (ACEIN command_in, ACEOUT result_out,
				DEFCPT look)
{ int i, j, n, dt = 0 , dt1 = 0 ;
  char *cp, *cq = 0 ;
  int opt;
  KEY key ;
  OBJ obj = 0 ;
  KEYSET ks = 0 ;
  FILE *f = 0 ;
  BOOL subclone ;
  mytime_t tim, tim1 ;

  tim  = tim1 = timeNow () ;
  if (diffaction)
    catText(diffaction, messprintf("Action %%%%1* // %s\n", timeShow(tim))) ;

  look->manEntry = FALSE ;
  while(TRUE)
    {
      subclone = FALSE ;
      if (aceInOptPrompt(command_in, &opt, mkaction))
	{
	  cp = freekey2text(opt, mkaction) ;
	  while (cp && *cp && *cp != ' ')
	    fputc (*cp++, defCptOut) ;

	  switch (opt)
	    {
	    case -1:
	      goto done ;

	    case '?':
	      {
		int i;
		aceOutPrint (result_out, "Acembly list of command :\n");
		for (i = 1 ; i <= mkaction[0].key ; i++)
		  aceOutPrint (result_out, "%s\n", mkaction[i].text) ;
	      }

	    case 1001: case 1002: case 1003:
	    case 1011: case 1012: case 1013:
	    case 1005: case 1006: 
	    case 1031: case 1032: case 1033:
	    case 1071: case 1073:

	      fMapcDNADoSelect (command_in, result_out, 
				opt - 1000, look->actif, look->taceActif);
	      break ;
	    case 1020:
	      ctfTraceExportKeySet (look->actif, result_out) ;
	      break ;
	    case 1200:
	      saucisseTest (look->actif, result_out) ;
	      break ;
	    case 2001: case 2002: case 2003:
	      fMapcDNADoSelect (command_in, result_out,
				opt - 1000, look->actif, look->taceActif);
	      break ;
            case 'q':
	      ks = query (0, "FIND Sequence _*") ;
	      for (i = 0 ; i < keySetMax(ks) ; i++)
	      if ((obj = bsUpdate(keySet(ks, i))))
		bsKill (obj) ;
	      keySetDestroy (ks) ;
	      ks = query (0, "FIND DNA _*") ;
	      for (i = 0 ; i < keySetMax(ks) ; i++)
		arrayKill (keySet(ks, i)) ;
	      keySetDestroy (ks) ;
	      goto done ;
	    case 19: subclone = TRUE ;   /* charge les dogs, pas au point */
	      /* fall thru to load */
	    case 'l':
	      aceInNext (command_in) ;
	      cp = aceInPos (command_in) ;
	      if (strlen(cp) == 0)
		{ 
		  fprintf (defCptOut, " // Load requires a sequence name. I quit") ;
		  goto done ;
		}
	      else
		{ KEY last ;
		  if (strncmp (cp, "-last", 5) == 0
		      && (last = lastAssembly ())) 
		    cp = name (last) ;
		  else if (strncmp (cp, "-all", 4) == 0)
		    cp = " CLASS Read " ; /* SCF_File AND NOT Assembled_from AND NOT Vector AND NOT IS _*  */
		  fprintf (defCptOut, "  %s  ", cp) ;
		}
	      cq = (char *)messalloc(1256) ;
	      if (strncasecmp (cp, "-active", 7) == 0)
		sprintf (cq, "-active") ;
	      else if (!subclone)
		sprintf (cq,">? Sequence %s", cp) ;
	      else
		sprintf (cq,">? Clone IS %s ; >Read", cp) ;
              printf (" // load got: ##%s##\n", cq) ;
	      if (look->def) /* donc load au milieu du fichier => stop lecture */
		{ 
		  fprintf (defCptOut, " // restarting ") ;
		  defCptRestart (look) ;
		}
	      i = dnaAlignLoad (look, cq) ;
	      messfree (cq) ;
	      if (i)
		fprintf (defCptOut, "  // %d sequences loaded ", i) ;
	      else
		fprintf (defCptOut, " // No Sequence, sorry") ;
	      break ;
	    case 'A':  /* add */
	      i = look->def ? keySetMax (look->def) : 0 ;
	      if (!i)
		break ;
	      cp = aceInPos(command_in) ;
	      fprintf (defCptOut, " %s ", cp) ;
	      i = dnaAlignLoad (look, cp) ;
	      fprintf (defCptOut, "  // %d sequences added ", i) ;
	      break ;
	    case 'n':  /* newScf */
	      i = dnaAlignLoad (look, 0) ; /* 0 => baseCallNewScf() */
	      fprintf (defCptOut, "  // %d new scf reads added ", i) ;
	      break ;
	    case 'g': /* get */
	      if (!look->def || !keySetMax (look->def))
		break ;
	      if (!aceInInt(command_in, &i))
		i = 2 ;
	      fprintf (defCptOut, " %d ", i) ;
	      look->whatDis = (unsigned char)i ;
	      defCptDoGetOligos(look) ;
	      if (!look->def) /* || keySetMax(look->def) < 2) */
		goto done ;
	      break ;
	    case 's': /* sort */
	      if (!look->def || keySetMax (look->def) < 2) /* no sort for less than 2 seq */
		break ;
	      if (!aceInInt(command_in, &i)) /* dans le fichier il faut 100*distancemax !! */
		i = 100 ;
	      fprintf (defCptOut, " %d // %d sequences", i, keySetMax(look->def)) ;
 	      look->dlimit = i ;
	      i = defCptDoTree(look) ;
	      if (i > 1)
		fprintf (defCptOut, "  // Planted %d trees", i) ;
	      else
		fprintf (defCptOut, "  // Planted %d tree", i) ;
	      break ;
	    case 'a': /* assemble */
	      if (!look->def || !keySetMax (look->def))
		break ;
	      aceInInt(command_in, &i) ;
	      fprintf (defCptOut, " %d ", i) ;
	      look->taux = i ;
	      defCptDoSegments(look) ;
	      break ;
	    case 'j': /* join */
	      if (!look->def || !keySetMax (look->def))
		break ;
	      if ((cp = aceInWord (command_in))
		  && strncasecmp (cp, "diag", 4) == 0)
		{
		  i = defCptDoJoinDiagonal (look) ;
		  fprintf (defCptOut, " diagonal") ;
		}
	      else
		i = defCptDoJoinSegments (look) ;
	      fprintf (defCptOut, "  // I made %d joins ", i) ;
	      break ;
	    case 'J': /* juxtapose */
	      if (!look->def || !keySetMax (look->def))
		break ;
	      defCptJuxtapose (look) ;
	      break ;
	    case 'F': /* Fix */
	      if (!look->def || !keySetMax (look->def))
		break ; 
	      if ((cp = aceInWord (command_in))
		  && strncasecmp (cp, "-new", 4) == 0)
		defCptDoFixSegConsensus(look, TRUE) ;
	      else
		defCptDoFixSegConsensus(look, FALSE) ;
	      break ;

	    case 'i':
	      aceInNext (command_in);
	      cp = aceInPos (command_in) ;
	      if (strlen(cp) == 0)
		ks = query (0, ">?Read IS *") ;
	      else if (strncasecmp (cp, "-subclone", 9) == 0)
		{ 
		  cp += 9 ;
		  ks = query (0, messprintf (">?Subclone ; >Read %s", cp)) ;
		}
	      else
		ks = query (0, messprintf (">?Read %s", cp)) ;
	      if (look->link) dnaAlignDoReInsertLoners (look->link, ks) ;
	      keySetDestroy (ks) ;
	      break ;

/*	    case 7:
	      if (!look->def || !keySetMax (look->def))
		break ;
	      printf("I filter the data\n") ;
	      i = 0 ; freeint(&i) ;
	      fprintf (defCptOut, " %d ", i) ;
	      j = look->dlimit ;
	      look->dlimit = i ;
	      defCptDoSuppChim(look) ;
	      look->dlimit = j ;
	      break ;
*/

	    case 20:
	      ks = look->def ;
	      if (!ks || !keySetMax(ks))
		ks = look->actif ;
	      if (!ks || !keySetMax(ks))
		break ; 
	      if ((cp = aceInWord(command_in))
		  && strcmp (cp, "-f") == 0)
		i = trackVector (ks, 0, TRUE) ;
	      else
		i = trackVector (ks, 0, FALSE) ;
	      fprintf (defCptOut, "  // %d vector clips found ",i) ;
	      break ;

	    case 'C': /* clip_on */
	      if (!look->def || !keySetMax (look->def))
		break ;
	      j = 0 ;
	      if ((cp = aceInWord(command_in)))
		switch (freeupper(*cp))
		  { case 'G': case 'F': case 'E':
		      key = (KEY)(*cp) ;
		      fprintf (defCptOut, " %s ", cp) ; j = 0 ;
		      i = baseCallUnclipKeySet (look->link, look->def, key, &j) ;
		      fprintf (defCptOut, " // %d clips moved by an average of %d bases ",
			       i, i > 0 ? j/i : 0 ) ;
		      break ;
		    case 'M':
		      fprintf (defCptOut, " %s ", cp) ;
		      i = 0 ; aceInInt (command_in, &i) ; j = 0 ;
		      fprintf (defCptOut, "%d ", i) ;
		      n = baseCallClipContig2Max (look->link, i, &j) ;
		      fprintf (defCptOut, " // %d clips moved by an average of %d bases ",
			       n, n > 0 ? j/n : 0 ) ;
		      break ;
		    case 'T': /* Tile */
		    case 'D': /* Double Tile */
		      key = (KEY)(*cp) ;
		      fprintf (defCptOut, " %s ", cp) ;
		      j = 0 ;
		      i = baseCallTileContigs (look->def, key, &j) ;
		      fprintf (defCptOut, " // %d clips moved by an average of %d bases ",
			       i, i > 0 ? j/i : 0 ) ;
		      break ;
		    case 'C': /* Consolidate */
		      key = (KEY)(*cp) ;
		      fprintf (defCptOut, " %s ", cp) ;
		      j = 0 ;
		      i = abiFixDouble (look->link, look->def, &j) ;
		      fprintf (defCptOut, " // %d clips moved by an average of %d bases ",
			       i, i > 0 ? j/i : 0 ) ;
		      break ;
		    default:
		      baseCallUnclipKeySet (look->link, look->def, 0, &j) ;
		      fprintf (defCptOut, "%s // Usage to move Clips: Clip_on [Excellent | Good | Fair]", cp) ;
		    }
	      else
		{ i = baseCallUnclipKeySet (look->link, look->def, 0, &j) ;
		  fprintf (defCptOut, " // Clips Evaluated") ;
		}
	      break ;
	    case 10:
	      if (!look->def || !keySetMax (look->def))
		break ;
	      defCptDoExtendReadsExt (look) ;
	      break ;
	    case 'L': /* align <target> */    
	      if (!look->def || !keySetMax (look->def))
		break ;   
	      aceInNext (command_in) ;
	      cp = aceInPos (command_in) ;
	      if (strlen(cp) == 0)
		{ 
		  fprintf (defCptOut, "// Usage: Align <reference_sequence>") ;
		  break ;
		}
	      if (!lexword2key(cp, &key, _VDNA) || /* get a seq with dna */
		  !lexword2key(cp, &key, _VSequence))  
		{ 
		  fprintf (defCptOut, "// Sorry, unknown reference_sequence %s", cp) ;
		  break ;
		}  
	      doAssembleAllTraces (key, look->def, 'n') ;
	      break ;
	    case 21:  /* order by size */
	      if (!look->link)
		break ;
	      ks = queryKey (look->link, ">Subsequence") ;
	      contigLengthSort (ks) ;
	      alignToolsAdjustLink (look->link, ks, 0) ;
	      keySetDestroy (ks) ;
	      break ;
	    case 22: /* order by subclone */
	      if (look->link)
		defCptOrderBySubclones (look) ;
	      break ;
	    case 23: /* order by subclone */
	      if (!look->def || !keySetMax (look->def))
		break ;
	      baseCallMakeSubclones (look->def) ;
	      break ;
	    case 13:
	      if (!look->def || !keySetMax (look->def))
		break ;
	      i = defCptDoPatchKeySet (look->def) ;
	      fprintf (defCptOut, " // %d auto-editions performed ", i) ;
	      break ;
	    case 'b':   /* base call */
	      if (!look->taceActif || !keySetMax (look->taceActif))
		break ;
	      i = defCptDoBaseCall (look->taceActif) ;
	      fprintf (defCptOut, " // %d traces called ", i) ;
	      break ;
	    case 18:
	      if (!look->def || !keySetMax (look->def))
		break ;
	      defCptDoTrainNNonKeySet (look->def) ;
	      break ;
	    case 'S': /* Save_as */
	      if (!look->def || !keySetMax (look->def))
		break ;
	      look->step = 0 ;
	      cp = aceInWord (command_in) ; 
	      if (cp)
		{
		  fprintf (defCptOut, " %s ", cp) ;
		  cq = strnew(cp, 0) ;
		  dnaAlignSave (look, cq, TRUE) ; /* pas a moi => peut changer */
		}
	      else
		{
		  cq = dnaAlignSaveDefault (look) ;
		  fprintf (defCptOut, " %s ", cq) ;
		}
	      messfree(cq) ;
	      sessionDoSave (TRUE) ;
	      break ;

	    case 'R': /* Rename */
	      if (!look->def || !keySetMax (look->def))
		break ;
	      look->step = 0 ;
	      cp = aceInWord (command_in) ; 
	      if (!cp)
		break ;
	      fprintf (defCptOut, " %s ", cp) ;
	      cq = strnew(cp, 0) ;
	      { 
		KEYSET ksq2;
		KEYSET ksq = 
		  query (0, messprintf
			 ("{FIND Sequence IS %s || IS %s.*}", cq, cq)) ;
	        ksq2 = query (ksq, "> DNA") ;
		i = keySetMax(ksq) ;
		while (i--)
		  { if ((obj = bsUpdate(keySet(ksq,i))))
		    bsKill (obj) ;
		  }
		keySetDestroy (ksq) ;	
		i = keySetMax(ksq2) ;
		while (i--)
		  { monDnaForget (look, keySet(ksq2,i)) ;
		    arrayKill (keySet(ksq2,i)) ;
		  }
		keySetDestroy (ksq2) ;
	      }
	      dnaAlignSave (look, cq, TRUE) ; /* pas a moi => peut changer */
	      messfree(cq) ;
	      sessionDoSave (TRUE) ;
	      break ;

	    case 'T':  /* tace */
	      {
		ACEIN tace_in = command_in;
		unsigned int command_set = CHOIX_UNDEFINED ; /* MUST be unsigned ints because later */
		unsigned int perm_set = PERM_UNDEFINED ;    /* we do logic on these variable. */

		if ((cp = aceInWord(command_in)) /* -f filename */
		    && strncmp(cp, "-f", 2) == 0
		    && aceInCheck (command_in, "w"))
		  { 
		    ACEIN fi;
		    if ((cp = aceInPath (command_in)) /* filename */
			&& (fi = aceInCreateFromFile (cp, "r", "", 0)))
		      {
			tace_in = fi;
		      }
		    else
		      break;
		  }
		aceInNext (command_in) ;  /* get parameters */
		cq = strnew(aceInPos(command_in), 0) ;

		command_set = (CHOIX_UNIVERSAL | CHOIX_NONSERVER) ;
		if (writeAccessPossible())
		  perm_set = (PERM_READ | PERM_WRITE | PERM_ADMIN) ;
		else
		  perm_set = PERM_READ ;

		commandExecute (tace_in,
				result_out, /* outfil */
				command_set, perm_set,
				0) ; /* no active keyset */
		messfree (cq) ;
		
		if (tace_in != command_in)
		  aceInDestroy (command_in);
	      }
	      break ;

	    case 'r':
	      tStatus (result_out) ;
	      break ;

	    case 16:
	      defCptPreReport (look) ;
	      break ;

	    case 'B':
	      if ((cp = aceInWord(command_in))
		  && lexword2key (cp, &key, _VSequence))
		statisticsDoMakeErreur (key, look->def) ;
	      else
		statisticsMakeErreur (look->def) ;
	      break ;
	    default:
	      fprintf (defCptOut, " // Unknown command, sorry") ;
	      break ;
	    }
	  if (DEBUG)
	    { int nmessalloc, messalloctot, aMade, aUsed, aAlloc, aReal, dnaMade, dnaTotal ;
	      nmessalloc = messAllocStatus (&messalloctot) ;
	      monDnaReport (&dnaMade, &dnaTotal) ;
	      arrayStatus (&aMade, &aUsed, &aAlloc, &aReal) ;
	      fprintf (defCptOut, " // nMessAlloc %d totMessAlloc %d kb nArr %d totArr %d kb, nMondna %d totMonDna %d kb\n",
		       nmessalloc, messalloctot/1024, aMade, aAlloc/1024, dnaMade, dnaTotal/1024 ) ;
	      fprintf (defCptOut, " // %d assInserted, %d assRemoved, %d assFound, %d assNotFound, %d assBounce\n",
		       assInserted, assRemoved, assFound, assNotFound, assBounce) ;
	    }
	  timeDiffSecs (tim, timeNow (), &dt) ;
	  timeDiffSecs (tim1, timeNow (), &dt1) ;
	  fprintf (defCptOut, " // + %d = %d seconds\n", dt1, dt) ;
	  tim1 = timeNow () ;
	}
    }
 done:
  timeDiffSecs (tim, timeNow (), &dt) ;
  fprintf (defCptOut," // end of ace-mbly  %s, total time %d seconds\n\n",
	   timeShow(timeNow()), dt) ;

  look->manEntry = TRUE ;
  return ;
} /* defCptDoReadAction */

/***************************************************************/

#ifndef NON_GRAPHIC
static void defCptReadAction(void)
{ 
  static char directory[DIR_BUFFER_SIZE] = "";
  static char filename[FIL_BUFFER_SIZE] = "";
  ACEIN fi = 0 ;
  ACEOUT fo;
  DEFCPTGET("defCptReadAction") ;

  fi = aceInCreateFromChooser ("Command File ?",
			       directory, filename, "smb", "r", 0);
  if (!fi)
    { 
      defcomputeDraw (look, 0) ;
      return ;
    }

  fo = aceOutCreateToStdout (0);

  defCptDoReadAction(fi, fo, look);

  aceInDestroy (fi) ;
  aceOutDestroy (fo);

  return;
} /* defCptReadAction */
#endif /* !NON_GRAPHIC */

/***************************************************************/

#ifndef NON_GRAPHIC
static void defCptWriteAction(void)
{ FILE *ff ;
  static char fileName[FIL_BUFFER_SIZE], dirName[DIR_BUFFER_SIZE] ;
  STORE_HANDLE handle = handleCreate();

  if (!diffaction)
    return ;
  strcpy(dirName, dbPathMakeFilName("", "", "", handle)) ;
  handleDestroy(handle);
  if (!(ff = filqueryopen(dirName, fileName, "smb", "w", 
			 "Name of Ace-mbly session ?")))
    return ;
  fprintf(ff, stackText(diffaction, 0)) ;
  filclose(ff) ;
} /* defCptWriteAction */
#endif /* !NON_GRAPHIC */

/***************************************************************/

#ifndef NON_GRAPHIC
static void pleaseAssemble(void)
{ DEFCPTGET ("Assemble") ;

  look->method = M_ASSEMB ;
  look->whatDis = 2 ; /* in this case number of oligos */
  look->taux = 2 ;
  sprintf(look->nboligo, "%d", (int)2) ;
  sprintf(look->choix, "%d", (int)3) ;
  *look->param = 0 ;
  diffaction = stackCreate(1024) ;
  graphActivate(look->defMapCtlGraph) ;
  graphHelp("Sequence_Assembly") ;
  graphRetitle("Sequence Assembly") ;
  defcomputeDraw (look, 0) ;
}

/***************************************************************/

static void pleaseFingerPrint(void)
{ DEFCPTGET ("FingPr") ;

  look->method = M_FINGPR ;
  graphHelp("Finger_Print_Mapping") ;
  graphRetitle("Finger Print Mapping") ;
  defcomputeDraw (look, 0) ;
}

/***************************************************************/

static void pleaseDefDup(void)
{ DEFCPTGET ("Def-Dup") ;

  look->method = M_DEFDUP ;
  /* look->whatDis initialise dans defCptGetData */
  graphHelp("Genetic_Mapping_by_Markers_and_Segments") ;
  graphRetitle("Genetic Mapping by Markers and Segments") ;
  defcomputeDraw (look, 0) ;
}

/***************************************************************/

static void pleaseAlignment(void)
{ DEFCPTGET ("Align") ;

  look->method = M_EDWARD ;
  graphHelp("Alignment_of_Genes") ;
  graphRetitle("Alignment of Genes") ;
  defcomputeDraw (look, 0) ;
}

/***************************************************************/

static MENUOPT qhpMenu[] = {
  { graphDestroy, "Quit"},
  { help, "Help"},
  { NULL, NULL }} ;

static MENUOPT mapOrAssembleMenu[] = {
  { pleaseAssemble, "Sequence Assembly"},
  { pleaseFingerPrint, "FingerPrint"},
  { pleaseDefDup, "Def/Dup genetic mapping"},
  { pleaseAlignment, "Gene Alignment"},
  { NULL, NULL }} ;
#endif

/***************************************************************/

#ifndef NON_GRAPHIC
static void defcomputeDraw (DEFCPT look, char *text)
{ int line = 3 ;

  if (!graphActivate(look->defMapCtlGraph))
    return ;
  graphPop() ;
  graphClear() ;
  graphText(text, 2, 2) ;
  graphButtons (qhpMenu, 0.25, 0.25, 60) ;
  switch (look->method)
    {
    case 0:
      graphButtons (mapOrAssembleMenu, 0.25, 2.25, 60) ;
      break ;
    case M_ASSEMB:
      switch(look->mapStatus & F_BOUT)
	{
	case 6:
	  goto ici ;
	case 5:
	  graphButton ("Fix Present Consensus", defCptFixSegConsensus,  3, 11.25) ;
	case 4:
	  if (look->rejected && keySetMax (look->rejected))
	    graphButton ("Re Insert Bad Sequences", defCptInsertBad, 25, 11.25) ;
	  goto ici ;
	case 3:case 2:
	  if (!(look->mapStatus & F_SHOWM))
	    graphButton ("Show Matrix",defMapDisplay, 18.25,5.25) ;
	  graphButton ("More Oligos", defCptGetMoreSequences, 23.25,7.25) ;
/*	  graphButton ("Make One Segment", defCptMakeOneSegment, 35.63, 7.25) ; */
	  graphButton ("Try All Assembly", defCptTryAll, 35.63, 7.25) ;
	  graphButton ("Try Spec Paires", defCptNewTryPaires, 25, 16.5) ;
	  graphButton ("Filter", defCptSuppChim, 53, 7.25) ;
	  if (!(look->mapStatus & F_ASSSEG))
	    { graphButton ("3: Assemble Sequences", defCptSegments, .25, 9.25) ;
	      graphText("Error Rate : ", 30, 9.25) ;
	      look->choixBox = graphTextEntry(look->choix, 4, 47, 9.25, 0) ;
	    }
	case 1:
	  if ((look->mapStatus & F_BOUT) < (unsigned char)3)
	    { graphButton ("2: Sort Sequences", defCptTree, 0.25,5.25) ;
	      graphText("Distance limit : ", 22, 5.25) ;
	      look->disBox = graphTextEntry(look->distance, 5, 43, 5.25, 0) ;
	      graphButton ("2.bis: Join Sequences", defCptJoinSegments, 52, 9.25) ;
	    }
/*        defCptChoixBox = graphButton ("Adjust Parameters", defCptChoix, 3.25,7.25) ;
	  graphBoxDraw(defCptChoixBox, BLACK, defchoix ? GREEN : WHITE) ; */
	  graphButton ("Histograms", defCptAssHisto, 28.25, 1.25) ;
	case 0:
	  if (!look->mapStatus)
	    { graphButton ("Read a File", defCptReadAction, 0.25, 6.25) ;
	      graphText("Name of Sequences to load :", 13, 6.25) ;
	      look->paramBox = graphTextEntry(look->param, 25, 40, 6.25, 0) ;
	      graphButton ("1: Get KeySet of Sequences", defCptGetOligos, 0.25,3.25) ;
	      graphText("Number of Oligos : ", 30, 3.25) ;
	      look->nboligBox = graphTextEntry(look->nboligo, 4, 50, 3.25, 0) ;
	    }
	ici:
	  if ((look->mapStatus & F_BOUT) > (unsigned char)3 && keySetMax(look->def) > 1)
	    { graphButton ("4.bis: Join Sequences", defCptJoinSegments, 52, 9.25) ;
	      graphButton ("4: Iterate the assembly", defCptGetOligos, 0.25,3.25) ;
	      graphText("Number of Oligos : ", 30, 3.25) ;
	      look->nboligBox = graphTextEntry(look->nboligo, 4, 50, 3.25, 0) ;
	    }	      
	  line = 12 ;
	  graphButton("Save Game", defCptWriteAction, 13, .25) ;
	  graphText ("Other tools", .25, line += 2) ;
	  /* graphButton ("Fit Sequences To Target", oldAssembleAllTraces, 2, line += 1.5) ; */
	  graphButton ("Fit Sequences To Target", newAssembleAllTraces, 2, line) ;
	  graphButton ("Make Statistics", defCptMakeStatErreur, 15, line + 1.5) ;
	  graphButton ("Count Group", defCptMakeStatBarreaux, 55, line + 1.5) ;
	  /*	  graphButton ("Check Oligos", statisticsTestOligo, 55, line + 3) ;*/
	  graphButton ("Fix Active Keyset", defCptFixActKeyset, 48, line) ;
	  graphButton ("Autoedit", defCptPatchKeySet, 36.5, line) ;
	  graphButton ("Cpt One Paire", defCptAsmbPaireActive, 2, line + 1.5) ;
	  if (look->method & 3)
	    graphButton ("Make SuperLink", defCptMakeSuperLink, 35, line + 1.5) ;
	  if (look->mapStatus & F_IS_FIXED)
	    { graphButton ("Save As", defCptSaveAs, 53, line + 1.5) ;
	    }	      
	  if ((look->mapStatus & F_BOUT) /*  == 4  */)
	    graphButton ("Extend Reads", defCptExtendReadsExt,  23, 12.75) ;
	  if (look->mapStatus & F_HIST)
	    graphButton("Clean Histograms", defCptRmHisto, 40.50, 1.25) ;
	  break ;
	}
      break ;
    case M_FINGPR:
      if (!look->mapStatus)
	{ graphButton("Get Clones", defCptGetClones, 2, 2) ;
	  break ;
	}
      if (!(look->mapStatus & F_SORT))
	graphButton("Sort Segment", defCptTree, 2, 3.5) ;
      graphText("Filter Functions", .25, 5) ;
      graphButton("Suppress overpresent bands", defTreeSupr, 2, 6) ;
      if (look->mapStatus & F_SORT)
	graphButton("Suppress off diagonal bands", defTreeNonDiag, 2, 7.5) ;
      if (look->whatDis & F_E_FLAG)
	graphButton("use all Data", defSupFlag, 33, 7.5) ;
      graphText("Display Functions", .25, 9) ;
      if (!(look->mapStatus & F_SHOWM))
	graphButton("Show Matrix", defMapDisplay, 2, 10) ;
      graphButton("Histograms", defCptFingPrHisto, 15, 10) ;
      if (look->mapStatus & F_HIST)
	graphButton("Clean Histograms", defCptRmHisto, 27, 10) ;
      break ;
    case M_DEFDUP:
      if (!look->mapStatus)
	{ graphButton ("Select a Map", defCptGetData, 2, 4) ;
	  break ;
	}
      if (look->mapStatus & F_NO_CONTIG)
	{ graphButton("Select a Contig", defCptGetData, 2, 4) ;
	  break ;
	}
      graphButton ("Change Contig", defCptGetData, 2, 4) ;
      if (!(look->mapStatus & F_SORT))
	graphButton("Sort Segment", defCptTree, 2, 5.5) ;
      else
	{ graphButton("Show Map", defCptExportOrder, 2, 5.5) ;
	  if (!(look->mapStatus & F_SOMAR))
	    { graphButton("Sort Marker", defCptSortMarkers, 50, 5.5) ;
	      graphButton("Old Sort Marker", defTreeSortM, 46, 12) ;
	    }
	}
      if (look->whatDis & F_E_YN)
	graphButton("Treatment of Unknown", defCptTreatUnk, 25, 5.5) ;
      graphText("Filter Functions", .25, 7) ;
      graphButton("Suppress trivial segments", defCptSupSing, 2, 8) ;
      graphButton("Suppress overpresent markers", defTreeSupr, 33, 8) ;
      if (look->mapStatus & F_SORT)
	graphButton("Suppress off diagonal markers", defTreeNonDiag, 2, 9.5) ;
      if (look->whatDis & F_E_FLAG)
	graphButton("use all Data", defSupFlag, 33, 9.5) ;
      graphText("Display Functions", .25, 11) ;
      if (!(look->mapStatus & F_SHOWM))
	graphButton("Show Matrix", defMapDisplay, 2, 12) ;
      graphButton("Histograms", defCptDefDupHisto, 15, 12) ;
      if (look->mapStatus & F_HIST)
	graphButton("Clean Histograms", defCptRmHisto, 27, 12) ;
      break ;
    case M_EDWARD:
      if (!(look->mapStatus))
	{ 
	  graphButton("Get Data", defCptGetGenes, 0.25, 3.25) ;
	}
      else if (look->mapStatus == 1)
	graphButton("Ace-Dump Allele Order", defCptExportOrder, 2, 3.5) ;
      break ;
    default:
      break ;
    }
  graphTextBounds (100, 17) ;
  graphRedraw() ;
}
#endif /* !NON_GRAPHIC */

/***************************************************************/
/* destruction des tableaux de look sauf dataArray (pour changer de contig) */
/* ? pour knownPairs et knownOrder */
static void lookArrayDestroy(DEFCPT look)
{ int i ;
  KEYSET *ksp ;
  TREE_DEF *seg ;

  look->nd = look->nm = 0 ;
  arrayDestroy (look->linOrder) ;
  arrayDestroy (look->colOrder) ;
  if (arrayExists (look->maillon))
    { if ((i = arrayMax (look->maillon)))
	{ seg = arrp (look->maillon, i - 1, TREE_DEF) + 1 ;
	  while(seg--, i--)
	    { keySetDestroy (seg->ks) ;
	      keySetDestroy (seg->x) ;
	    }
	}
      arrayDestroy (look->maillon) ;
    }
  if (arrayExists(look->defInMar))
    { if ((i = arrayMax(look->defInMar)))
	{ ksp = arrp(look->defInMar, 0, KEYSET) - 1 ;
	  while(ksp++, i--)
	    keySetDestroy(*ksp) ;
	}
      arrayDestroy (look->defInMar) ;
    }
  if (arrayExists (look->marInDef))
    { if ((i = arrayMax(look->marInDef)))
	{ ksp = arrp(look->marInDef, 0, KEYSET) - 1 ;
	  while(ksp++, i--)
	    keySetDestroy(*ksp) ;
	}
      arrayDestroy (look->marInDef) ;
    }
  arrayDestroy (look->gmap) ;
  keySetDestroy (look->def) ;
  keySetDestroy (look->mar) ;
  keySetDestroy (look->rejected) ;
}

/***************************************************************/

static void defCptRestart (DEFCPT look)
{ lookArrayDestroy(look) ;
  intrinsicTreeDestroy() ;
  dnaAlignDestroy(look) ;
#ifndef NON_GRAPHIC
  plotHistoRemove() ;
  if (graphActivate(look->defMapGraph))
    graphDestroy() ;
  if (graphActivate(look->defTreeGraph))
    graphDestroy() ;
#endif
  look->assDnaGet = assBigCreate (10000) ;
}

/***************************************************************/

static void uLocalDoDestroy (DEFCPT look)
{ char *kp = 0 ;
#ifndef NON_GRAPHIC
  Graph gActive = graphActive () ; /* some destruction affect Graphs ; we want keep the active one */
#endif

  if (!look || !look->magic)
    return ;
  if (look->magic != DEFCPTMAG)
    messcrash ("Destroy received a wrong look pointer") ;
  nblook-- ;
  look->magic = 0 ;
  lookArrayDestroy(look) ;
  intrinsicTreeDestroy() ;
  dnaAlignDestroy (look) ;
  keySetDestroy (look->actif) ;
  arrayDestroy(look->dataArray) ;
  arrayDestroy(look->knownPairs) ;
  arrayDestroy(look->knownOrder) ;
  if (assDefLook && look->link)
    assRemove (assDefLook, kp + look->link) ;
#ifndef NON_GRAPHIC
  plotHistoRemove () ;
  if (graphActivate (look->defMapGraph))
    graphDestroy() ;
  if (graphActivate (look->defTreeGraph))
    graphDestroy() ;
  graphActivate (gActive) ;	/* restore context */
#endif /* !NON_GRAPHIC */
  messfree (look) ;
  if (diffaction)
    stackDestroy(diffaction) ;
}

/***************************************************************/

#ifndef NON_GRAPHIC
static void localDestroy(void)
{ DEFCPTGET (look) ;

  localDoDestroy (look) ;
}
#endif /* !NON_GRAPHIC */

/***************************************************************/
/***************************************************************/
/* attention ne sortir de tace que en detruisant look (localDestroy) */

static DEFCPT defComputeNewLook (void)
{ DEFCPT look = (DEFCPT)messalloc(sizeof(struct DEFCPTSTUFF)) ;
  nblook++ ;
  look->magic = DEFCPTMAG ;
  look->id = ++nmrid ;
  look->Line = 8 ;
  look->dlimit = 100 ;
  look->manEntry = FALSE ;
  look->display = FALSE ;
  look->assDnaGet = assBigCreate (10000) ;
  sprintf(look->distance, "%f", (float)1.) ;
  return look ;
} /* defComputeNewLook */

/***************************************************************/

void defComputeTace (ACEIN command_in, ACEOUT result_out, KEYSET ks)
     /* called by command.c (for tacembly) */
{
  defcptInit() ;

  if (!sessionGainWriteAccess()) /* try to grab it */
    { 
      /* may occur is somebody else grabbed it */
      messout ("Sorry, you cannot gain write access") ;
      return ;
    }
  nonGraphicLook = defComputeNewLook () ;

  aceOutPrint (result_out, " // start ace-mbly; %s\n", 
	       timeShow(timeNow())) ;
  nonGraphicLook->taceActif = ks ;
  nonGraphicLook->actif = keySetExists (ks) ? query (ks, "CLASS Sequence") : 0 ;
  defCptDoReadAction (command_in, result_out, nonGraphicLook) ;
  localDoDestroy (nonGraphicLook) ;

  return;
} /* defComputeTace */

/***************************************************************/
#ifndef NON_GRAPHIC

static void defCptPick(int box, double x_unused, double y_unused, int modifier_unused)
{ DEFCPTGET ("defCptPick") ;

  if (look->nboligBox && box == look->nboligBox)
    graphTextEntry(look->nboligo, 0, 0, 0, 0) ;
  else if (look->disBox && box == look->disBox)
    graphTextEntry(look->distance, 0, 0, 0, 0) ;
  else if (look->choixBox && box == look->choixBox)
    graphTextEntry(look->choix, 0, 0, 0, 0) ;
  else if (look->paramBox && box == look->paramBox)
    graphTextEntry(look->param, 0, 0, 0, 0) ;
  else
    defcomputeDraw (look, 0) ;

  return;
} /* defCptPick */

void defCompute (void)
{ DEFCPT look = 0 ;

  if (graphActivate (myGraph))
    { if (!graphAssFind (&DEFCPTMAG, &look)) 
	messcrash ("graph not found in %s", name) ; 
      if (look->magic != DEFCPTMAG) 
	messcrash ("%s received a wrong pointer", name) ;
      defcomputeDraw (look, 0) ;
      myLook = look ;
      return ;
    }
  look = defComputeNewLook () ;
  look->display = TRUE ;
  look->manEntry = TRUE ;
  look->defMapCtlGraph = myGraph =
    graphCreate (TEXT_SCROLL, "Mapping and Assembly",
				      0.60,0.0,0.65,0.27) ;
  graphRegister(DESTROY, localDestroy) ;
  graphRegister(PICK, defCptPick) ;
  graphAssociate (&DEFCPTMAG , look) ;
  graphMenu(qhpMenu) ;
  graphHelp("Mapping and Assembly") ;
  graphColor (BLACK) ;
  defcomputeDraw (look, 0) ;
  myLook = look ;
}

/***************************************************************/

void defCptOpen (KEY link)
{
  defcptInit() ;
  
  defCompute () ; /* reopen the window */
  if (myLook->link == link)
    return ;

  if (myLook->link && myLook->link != link &&
      !messQuery
      (messprintf
       ("You are already assembling %s, %s %s",
	name(myLook->link), 
	"should we restart on ", name(link))))
      return ;
  defCptChangeLook (myLook, myLook->link, link) ;
  myLook->def = queryKey (link, ">Subsequence ; >DNA") ;
  pleaseAssemble () ;
}

/***************************************************************/

   /* call with com == 0 to start up the command window */
KEY defCptExecuteCommand (KEY link, KEYSET def, KEYSET actif, char *com, char *param)
{ int level = 0 ;
  DEFCPT look = 0 ;
  KEY key = 0 ;
  ACEIN fi =  0 ; /* aceInCreateFromStdin (0); */

  if (!link)
    link = 1 ; /* to be associated */
  /*
    if (com && *com)
    level = aceInSetText(fi, com, param) ;
    */
/* else   ? level n'etait pas initialise et pourquoi le else ? 
    look->display = TRUE ; */
  return 0 ; /* level shouldbe defined */
  look = defCptGetLook (link) ;
  lookArrayDestroy(look) ; /* restart */
  intrinsicTreeDestroy() ;
  if (def)
    look->def = def ;
  else if (actif)
    look->actif = actif ;
  else
    { look->def = queryKey (link, ">Subsequence ; >DNA") ;
/*    look->rejected = queryKey (link, ">Rejected ; >DNA") ; */
    }

  if (level)
    { key = look->link ;
      if ((look->def && arrayMax (look->def)) || (look->actif && arrayMax (look->actif)))
	defCptDoReadAction(fi, look, level) ;
      if (key == 1)
	{ key = look->link ;
	  defCptDestroyLook (key) ;
	}
      else if (key != look->link)
	{ defCptChangeLook (look, key, look->link) ; /* normally done before */
	  key = look->link ;
	}
    }
  else
    key = 0 ;
  aceInDestroy (fi) ;
  return key ;
}
#endif
/***************************************************************/
/***************************************************************/
/***************************************************************/

static void defCptGiveDirection (KEY key1, KEY key2, KEY tag, int *sens1, int *sens2)
{ int i1, i2, x1, x2 ;
  Array dog1 = 0, dog2 = 0 ;
  OBJ obj1 = 0, obj2 = 0 ;
  BSunit *u1, *u2 ;

  dog1 = arrayCreate (20, BSunit) ;
  obj1 = bsCreate (key1) ;
  if (!obj1 || !bsFindTag (obj1, tag) || !bsFlatten (obj1, 2, dog1))
    { *sens1 = 0 ; *sens2 = 0 ;
      goto abort ;
    }
  dog2 = arrayCreate (20, BSunit) ;
  obj2 = bsCreate (key2) ;
  if (!obj2 || !bsFindTag (obj2, tag) || !bsFlatten (obj2, 2, dog2))
    { *sens1 = 0 ; *sens2 = 0 ;
      goto abort ;
    }
  x1 = 0 ;
  x2 = 0 ;
  i1 = arrayMax (dog1) / 2 ;
  u1 = arrp (dog1, 0, BSunit) ;
  while (i1--)
    { i2 = arrayMax (dog2) / 2 ;
      u2 = arrp (dog2, 0, BSunit) ;
      while (i2--)
	{ if (u1->k == u2->k)
	    { x1 += (u1+1)->i ;
	      x2 += (u2+1)->i ;
	    }
	  u2 += 2 ;
	}
      u1 += 2 ;
    }
  *sens1 = x1 > 0 ? 1 : (x1 < 0 ? -1 : 0) ; /* sens 1 puis 2 ils doivent se faire front */
  *sens2 = x2 < 0 ? 1 : (x2 > 0 ? -1 : 0) ;
 abort:
  bsDestroy (obj1) ;
  bsDestroy (obj2) ;
  arrayDestroy (dog1) ;
  arrayDestroy (dog2) ;
}

/***************************************************************/

static BOOL defCptPolarFirst (TREE_DEF ksm, Array bilan, KEY tag, int *dir)
{ int i, max, x1, x2 ;
  KEY key1, key2 ;

  i = keySetMax (ksm.ks) ;
  max = arrayMax (bilan) ;
  if (i < 2)
    { array (bilan, max++, BSunit).k = keySet (ksm.ks, 0) ;
      array (bilan, max, BSunit).i = 1 ;
      return FALSE ;
    }
  key1 = keySet (ksm.ks, 0) ;
  key2 = keySet (ksm.ks, 1) ;
  for (i = 1 ; i < keySetMax (ksm.ks) && keySet (ksm.x, i) ; i++) ;
  if (i < keySetMax (ksm.ks))
    key2 = keySet (ksm.ks, i) ;
  defCptGiveDirection (key1, key2, tag, &x1, &x2) ;
  *dir = x1 ;
  return TRUE ;
}

/***************************************************************/

static void defCptPolarNext (TREE_DEF ksm, Array bilan, KEY tag, int fath, int dir)
{ int i, j, k, max, xmax, dirf, dir1, dir2, x2 ;
  KEY key1, key2, done = 1 << 24 ;
  BOOL pass = TRUE ;

  i = keySetMax (ksm.ks) ;
  key1 = keySet (ksm.ks, fath) ;
  xmax = keySet (ksm.x, fath) ;
  dirf = dir ;
  k = fath ;
 again:
  for (j = k + 1 ; j < i ; j++)
    { x2 = keySet (ksm.x, j) ;
      if (x2 == done)
	continue ;
      if (x2 < xmax)
	{ if (keySet (ksm.x, k) != done)
	    { max = arrayMax (bilan) ;
	      array (bilan, max++, BSunit).k = key1 ;
	      array (bilan, max, BSunit).i = dirf ;
	      keySet (ksm.x, k) = done ;
	    }
	  return ;
	}
      key2 = keySet (ksm.ks, j) ;
      defCptGiveDirection (key1, key2, tag, &dir1, &dir2) ;
      if (!dirf)
	dirf = dir1 ;
      if (pass && (x2 > xmax))
	{ if (dirf * dir1 < 0)
	    defCptPolarNext (ksm, bilan, tag, j, - dir2) ;
	  continue ;
	}
      if (keySet (ksm.x, k) != done)
	{ max = arrayMax (bilan) ;
	  array (bilan, max++, BSunit).k = key1 ;
	  array (bilan, max, BSunit).i = dirf ;
	  keySet (ksm.x, k) = done ;
	  j = k ;
	  pass = FALSE ;
	  continue ;
	}
      if (x2 > xmax)
	{ defCptPolarNext (ksm, bilan, tag, j, dir2) ;
	  continue ;
	}
      k = j ;
      dirf = dir2 ;
      key1 = key2 ;
    }
  if (keySet (ksm.x, k) != done)
    { max = arrayMax (bilan) ;
      array (bilan, max++, BSunit).k = key1 ;
      array (bilan, max, BSunit).i = dirf ;
      keySet (ksm.x, k) = done ;
      if (pass)
	{ pass = FALSE ;
	  goto again ;
	}
    }
}

/***************************************************************/

static void defCptPolarity (DEFCPT look, KEY sub, KEY tag)
{ int i, dir ;
  TREE_DEF ksm ;
  Array bilan = 0 ;

  i = arrayMax (look->maillon) ;
  if (!i)
    return ;
  bilan = arrayCreate (2*keySetMax (look->def), BSunit) ;
  for (i = 0 ; i < arrayMax (look->maillon) ; i++)
    { ksm = arr (look->maillon, i, TREE_DEF) ;
      if (defCptPolarFirst (ksm, bilan, tag, &dir))
	defCptPolarNext (ksm, bilan, tag, 0, dir) ;
    }
  alignToolsAdjustLink (look->link, 0, bilan) ;
}

/***************************************************************/
/* attention look->def est ici un keySet de Sequence */
static BOOL defCptDoOrderKeySet (DEFCPT look, KEY tag, KEY sub)
{ int i, j, k, nmd ;
  Array temp = 0, *ap ;
  OBJ obj = 0 ;
  KEY *keyp ;
  KEYSET *ksp, ks, resu ;
  BSunit *up ;

  if (!keySetExists (look->def) || !(i = keySetMax (look->def)))
    return FALSE ;
  keySetSort (look->def) ; /* normalement fait dans la fonction appelante ? */
  keySetCompress (look->def) ;
  i = keySetMax (look->def) ;
  temp = arrayCreate (i, Array) ;
  array (temp, i - 1, Array) = 0 ;
  ap = arrp (temp, 0, Array) - 1 ;
  keyp = arrp (look->def, 0, KEY) - 1 ;
  look->mar = keySetCreate () ;
  while (ap++, keyp++, i--)
    { *ap = arrayCreate (20, BSunit) ;
      if ((obj = bsCreate (*keyp)) && bsFindTag (obj, tag) && 
	  bsFlatten (obj, 1, *ap))
	{ j = arrayMax (*ap) ;
	  up = arrp (*ap, 0, BSunit) - 1 ;
	  k = keySetMax (look->mar) ;
	  while (up++, j--)
	    keySet (look->mar, k++) = up->k ;
	}
      bsDestroy (obj) ;
    }
  keySetSort (look->mar) ;
  keySetCompress (look->mar) ;
  if (!keySetMax (look->mar))
    { i = arrayMax (temp) ;
      ap = arrp (temp, 0, Array) - 1 ;
      while (ap++, i--)
	arrayDestroy (*ap) ;
      arrayDestroy (temp) ;
      keySetDestroy (look->mar) ;
      return FALSE ;
    }
  look->nd = keySetMax (look->def) ;
  look->nm = keySetMax (look->mar) ;
  look->whatDis = 1 ;
  look->method = M_DEFDUP ;
  i = look->nd ;
  look->marInDef = arrayCreate(i, KEYSET) ;
  array(look->marInDef, i - 1, KEYSET) = 0 ;
  i = look->nm ;
  look->defInMar = arrayCreate(i, KEYSET) ;
  array(look->defInMar, i - 1, KEYSET) = 0 ;
  ksp = arrp(look->defInMar, 0, KEYSET) ;
  while(i--)
    *ksp++ = keySetCreate() ;
  ap = arrp (temp, 0, Array) ;
  ksp = arrp(look->marInDef, 0, KEYSET) ;
  for (i = 0 ; i < look->nd ; ksp++, ap++, i++)
    { *ksp = keySetCreate () ;
      nmd = 0 ;
      j = arrayMax (*ap) ;
      if (j) /* mieg */
	{ up = arrp (*ap, 0, BSunit) - 1 ;
	  while (up++, j--)  
	    { if (!keySetFind (look->mar, up->k, &k))
		messcrash ("Le marqueur doit exister a ce niveau") ;
	      keySet (*ksp, nmd++) = k ;
	      ks = arr (look->defInMar, k, KEYSET) ;
	      keySet (ks, keySetMax (ks)) = i ;
	    }
	}
      arrayDestroy (*ap) ;
    }
  arrayDestroy (temp) ;
  i = look->nd ;
  ksp = arrp(look->marInDef, 0, KEYSET) - 1 ;
  while(ksp++, i--)
    arraySort(*ksp, mardefOrder) ;
  i = look->nm ;
  ksp = arrp(look->defInMar, 0, KEYSET) - 1 ;
  while(ksp++, i--)
    arraySort(*ksp, mardefOrder) ;
  arrayDestroy (look->colOrder) ;
  arrayDestroy (look->linOrder) ;
  TESCOLO ;
  TESLIN ;
  if (!intCptTree (look))
    return FALSE ;
  resu = keySetCopy (look->def) ;
  for (i = 0 ; i < look->nd ; i++)
    keySet (look->def, i) = keySet (resu, arr (look->colOrder, i, int)) ;
  keySetMax (look->def) = i ;
  if (look->link)
    defCptPolarity (look, sub, tag) ;
  keySetDestroy (resu) ;
  return TRUE ;
}

/***************************************************************/
/* restart look */
/*                                                                           */
/* This is not currently called from anywhere....                            */
/*                                                                           */
BOOL defCptOrderKeySet (KEYSET aTrier, KEY tag, KEY link, KEY sub)
{
  BOOL end = FALSE, isCreate = FALSE ;
  KEY i = 1 ;
  char *kp = 0 ;
  DEFCPT look = 0 ;

  defcptInit() ;

  if (!keySetExists (aTrier) || !keySetMax (aTrier))
    return FALSE ;
  keySetSort (aTrier) ; /* normalement fait dans la fonction appelante ? */
  keySetCompress (aTrier) ;
  if (!link && assDefLook)
    while (assFind (assDefLook, kp + i, &look))
      i++ ;
  else i = link ;
  if (assDefLook && !assFind (assDefLook, kp + i, &look))
    isCreate = TRUE ;
  look = defCptGetLook (i) ;
  lookArrayDestroy (look) ; /* restart look */
  look->def = aTrier ;
  look->link = link ;
  if (defCptDoOrderKeySet (look, tag, sub))
    end = TRUE ;
  look->def = 0 ;
  if (isCreate)
    { look->link = i ;
      localDoDestroy (look) ;
    }
  else
    lookArrayDestroy (look) ;
  return end ;
}

/***************************************************************/

static void findMultipletsInContigs (KEY contig, KEY subcl, KEY mult)
{ OBJ Seq, Contig = bsUpdate (contig) ;
  int x1, x2, lng, old ;
  KEY seq, dog ;
  BSMARK mark = 0, mark2 = 0 ;
  
  if (!Contig)
    return ;
  if (bsFindTag (Contig, mult))
    bsRemove (Contig) ;

  if (bsGetKey (Contig, _Assembled_from, &seq))
    do
      { if ((Seq = bsCreate (seq)))
	  { if (bsGetKey (Seq, subcl, &dog))
	      { lng = 0 ;
		mark = bsMark (Contig, mark) ;
		if (bsGetData (Contig, _bsRight, _Int, &x1) &&
		    bsGetData (Contig, _bsRight, _Int, &x2))
		  lng = x2 - x1 ;
		bsAddKey (Contig, mult, dog) ;
		if (lng)
		  { mark2 = bsMark (Contig, mark2) ;
		    old = 0 ;
		    if (bsGetData (Contig, _bsRight, _Int, &old))
		      lng += old ; /* so conflicting directions average out */
		    bsGoto (Contig, mark2) ;
		    bsAddData (Contig, _bsRight, _Int, &lng) ;
		  }
		bsGoto (Contig, mark) ;
	      }
	    bsDestroy (Seq) ;
	  }
      } while (bsGetKey (Contig, _bsDown, &seq)) ;
  bsSave (Contig) ;
  bsMarkFree (mark) ;
  bsMarkFree (mark2) ;
}

/***************************************************************/

static void defCptMoveLocally (DEFCPT look, KEY tag, KEY sub)
{ int i, p1, p2, d1, d2, s1, s2, max = 0 ;
  KEY key1, key2 ;
  Array order = 0, unit = 0 ;
  OBJ Link = 0 ;
  BSunit *u ;

  Link = bsCreate (look->link) ;
  if (!Link)
    return ;
  unit = arrayCreate (90, BSunit) ;
  if (!bsFindTag (Link, sub) || !bsFlatten (Link, 3, unit) || !arrayMax (unit))
    goto abort ;
  key1 = arrp (unit, 0, BSunit)->k ;
  p1 = arrp (unit, 1, BSunit)->i ;
  p2 = arrp (unit, 2, BSunit)->i ;
  order = arrayCreate (90, BSunit) ;
  for (i = 3 ; i < arrayMax (unit) ; i += 3)
    { u = arrp (unit, i, BSunit) ;
      key2 = u[0].k ;
      d1 = u[1].i ;
      d2 = u[2].i ;
      defCptGiveDirection (key1, key2, tag, &s1, &s2) ;
      if ((s1 * (p2 - p1) < 0) && (s2 * (d2 - d1) < 0))
	{ array (order, max++, BSunit).k = key2 ;
	  array (order, max++, BSunit).i = d2 - d1 ;
	  continue ;
	}
      array (order, max++, BSunit).k = key1 ;
      array (order, max++, BSunit).i = p2 - p1 ;
      key1 = key2 ; p1 = d1 ; p2 = d2 ;
    }
  array (order, max++, BSunit).k = key1 ;
  array (order, max++, BSunit).i = p2 - p1 ;
  alignToolsAdjustLink (look->link, 0, order) ;
  arrayDestroy (order) ;
 abort:
  arrayDestroy (unit) ;
  bsDestroy (Link) ;
}

/***************************************************************/
/* attention change le contenu du look ; normalement appelee par execute command 
   avec un look vide a part look->def */
static void defCptOrderBySubclones (DEFCPT look)
{ int i ;

  keySetDestroy (look->def) ;
  if (look->link)
    look->def = queryKey (look->link, ">Subsequence") ;
  i = keySetMax (look->def) ;
  if (!i) return ;
  while (i--)
    findMultipletsInContigs (keySet (look->def, i), _Subclone, _Multiplet) ;
  if (defCptDoOrderKeySet (look, _Multiplet, _Subsequence))
    defCptMoveLocally (look, _Multiplet, _Subsequence) ;
}

/***************************************************************/

void defCptForget (KEY link, KEY key)
{
  DEFCPT look = 0 ;
  char *kp = 0 ;

  defcptInit() ;

  if (!link)
    { if (assDefLook)
	while (assNext (assDefLook, &kp, &look))
	  dnaAlignForget (look, key) ;
      if (nonGraphicLook)
	dnaAlignForget (nonGraphicLook, key) ;
      if (myLook)
	dnaAlignForget (myLook, key) ;
    }
  else if (assFind (assDefLook, kp + link, &look))
    dnaAlignForget (look, key) ;
}

/***************************************************************/

void defCptChangeLook (DEFCPT look, KEY oldLink, KEY newLink)
{ char *kp = 0 ;
  KEY link ;

  defcptInit() ;

  if (!assDefLook)
    { assDefLook = assCreate () ;
      isTimeStamps = FALSE ;
    }
  if (oldLink)
    link = oldLink ;
  else
    link = look->link ;
  if (link) /* voir le cas pour le look static de la fonction */
    assRemove (assDefLook, kp + link) ;
  look->link = newLink ;
  assInsert (assDefLook, kp + newLink, look) ;
}

/***************************************************************/
/* Si look->link = 0 association avec la cle 1 qui ne peut etre un link (ancien look statique) */
DEFCPT defCptGetLook (KEY link)
{
  DEFCPT look = 0 ;
  char *kp = 0 ;

  defcptInit() ;

  if (!assDefLook)
    { assDefLook = assCreate () ;
      isTimeStamps = FALSE ;
    }
  if (!link)
    { if (nonGraphicLook)
	look = nonGraphicLook ;
#ifndef NON_GRAPHIC
      else
	graphAssFind (&DEFCPTMAG, &look) ;
#endif      
      if (!look)
	messcrash ("Look non initialise") ;
    }
  else if (!assFind (assDefLook, kp + link, &look))
    { look = defComputeNewLook () ;
      assInsert (assDefLook, kp + link, look) ;
      look->link = link ;
    }
  if (look->magic != DEFCPTMAG)
    messcrash ("%s received a wrong pointer in defCptGetLook", "") ;
  return look ;
}

/***************************************************************/

void defCptDestroyLook (KEY link)
{
  DEFCPT look = 0 ;
  char *kp = 0 ;

  defcptInit() ;

  if (!assDefLook)
    { isTimeStamps = FALSE ;
      return ;
    }
  if (link && assFind (assDefLook, kp + link, &look) && look)
    { 
#ifndef NON_GRAPHIC
      if (myLook && myLook->link == link) /* i e same look as defcpt's window */
	{ if (myLook != look)
	    messerror ("Should be the same look") ;
	  return ;
	}
#endif
      if (link != look->link)
	messerror ("Pb dans le look -> voir pour le destroy") ;
      if (myLook == look) myLook = 0 ;
      localDoDestroy (look) ;
    }
}

/***************************************************************/
/***************************************************************/
/***************************************************************/
 
