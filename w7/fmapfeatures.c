/*  file: fmapfeatures.c
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
 * Description: feature display for fmap package
 * Exported functions:
 * HISTORY:
 * Last edited: Dec  9 15:58 2009 (edgrif)
 * * Oct 17 09:42 2001 (edgrif): Add support for calling blixem as a separate
 *              process.
 * * Apr 20 11:54 2000 (edgrif): Moved column pulldown drawing to mapcontrol.c,
 *              added 'access' functions so that mapcontrol can find out what
 *              sort of column it is dealing with for menu building etc.
 * * Sep 16 10:47 1998 (edgrif): Insert macro to define bitField for
 *              bitset.h ops.
 * * Jul 16 10:06 1998 (edgrif): Introduce private header fmap_.h
 * Created: Sun Jul 26 13:02:46 1992 (rd)
 * CVS info:   $Id: fmapfeatures.c,v 1.178 2009-12-10 10:06:24 edgrif Exp $
 *-------------------------------------------------------------------
 */

#include <glib.h>
#include <wh/acedb.h>
#include <wh/aceio.h>
#include <wh/call.h>
#include <wh/bump.h>
#include <wh/bitset.h>
#include <wh/a.h>
#include <wh/lex.h>
#include <wh/bindex.h>
#include <wh/query.h>
#include <wh/dna.h>
#include <wh/parse.h>
#include <wh/method.h>
#include <whooks/classes.h>
#include <whooks/sysclass.h>
#include <whooks/systags.h>
#include <whooks/tags.h>
#include <wh/pref.h>
#include <wh/display.h>
#include <wh/forest.h>
#include <wh/dbpath.h>
#include <wh/gex.h>

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
/* I'd like to just include this but it include colcontrol.h which clashes
 * with an include of map.h somewhere above. */
#include <wh/pepdisp.h>
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

#include <wh/peptide.h>
#include <w7/fmap_.h>
#include <wh/gex.h>
#define ACEDB			/* for this include of blxview */
#include <wh/blxview.h>
#undef ACEDB
#include <wh/dna.h>
#include <wh/client.h>


BITSET_MAKE_BITFIELD		/* define bitField for bitset.h ops */


/* defined here once as constants, because we refer to these labels
 * in various places to get at the menuitem by name */
#define LABEL_OVERLAP  "  Overlap"
#define LABEL_BUMPABLE "  Bump"
#define LABEL_CLUSTER  "  Cluster"
#define LABEL_WIDTH    "  Width"
#define LABEL_OFFSET   "  Offset"
#define LABEL_HIST     "  Histogram"

/************************************************************/

typedef enum { DEFAULT=0, WIDTH, OFFSET, HIST } BoxColModeType;

typedef struct 
{
  FeatureMap look ;
  
  METHOD *meth;			/* a method-struct from look->mcache */

  MethodOverlapModeType overlap_mode ;			    /* See wh/method.h */
  
  /* following not under user control */
  float offset ;
  BOOL isDown ;
  BOOL isFrame ;		/* TRUE if we're showing in 3 frames */
  BoxColModeType mode ;
  float fmax ;
  BUMP bump ;
  Associator cluster ;		/* only non-zero if METHOD_CLUSTER */
  int clusterCount ;
  float histBase ;		/* normalised meth->histBase, between 0 and 1 */
  int width ;

  MENU menu;
} BoxCol ;


/* logically you might think that there should be an "insertion" type that is not a transposon
 * but the code does not seem to do this....so ALLELE_DRAW_INSERTION is a bit redundant at the
 * moment..... */
typedef enum {ALLELE_DRAW_DELETION, ALLELE_DRAW_REPLACMENT,
	      ALLELE_DRAW_INSERTION, ALLELE_DRAW_TRANSPOSON} AlleleDrawType ;


static void bcMenuSetOverlap (MENUITEM menu_item);
static void bcMenuSetBump (MENUITEM menu_item);
static void bcMenuSetCluster (MENUITEM menu_item);
static void bcMenuSetWidth (MENUITEM menu_item);
static void bcMenuSetOffset (MENUITEM menu_item);
static void bcMenuSetHist (MENUITEM menu_item);
static void bcMenuDeleteMapColumn (MENUITEM menu_item);
static BoxCol *bcFromName (FeatureMap look, char *name, MENU *menu) ;
static void   bcCheck (BoxCol *bc);   /* checks before using a column */
static BOOL   bcTestMag (BoxCol *bc, float mag) ;
static int    bcDrawBox (BoxCol *bc, SEG *seg, float score, Array gaps) ;

static void addVisibleInfo (FeatureMap look, int i) ;

static void translationTitle (ACEOUT fo, KEY seq) ;

static void pfetchWindow (KEY key);

static void translateCDS(int box) ;
static void translateAll(int box) ;
static void translateGene(int box, BOOL CDS_only) ;
static void changeGeneMenu(FeatureMap look, SEG *seg, BOOL cds_only) ;

static void showCDSInPepDisp(int box) ;
static void showAllInPepDisp(int box) ;
static void showInPepDisp (int box, BOOL CDS_only) ;

static void exportCDSTranslation (int box);
static void exportCompleteTranslation (int box);
static void exportTranslation(int box, BOOL CDS_only);

static void fMapShowcDNA(int box) ;
static void fMapShowCDS(int box) ;
static void showDNA(int box, BOOL CDS_only) ;

static void fMapExportcDNA(int box) ;
static void fMapExportCDS(int box) ;
static void exportDNA(int box, BOOL CDS_only) ;

static void fMapColorIntronsExons (int box);
#ifndef MACINTOSH
static void fMaptRNAfold (int box) ;
#endif /* !MACINTOSH */

/* Pep homology menu functions */
static void fMapShowPepAlign (int box) ;
static void fMapShowPepAlignThis (int box) ;
static void fMapExpandAlign (int box) ;
static void fMapFetch (int box) ;
static void fMapEfetch (int box) ;
static void fMapPfetch (int box) ;

/* DNA homology menu functions */
static void fMapShowDNAAlign (int box) ;
static void fMapShowDNAAlignThis (int box) ;
static void fMapExpandAlign (int box) ;
static void fMapEfetch (int box) ;
static void fMapPfetch (int box) ;

/* Adjusts coords of features that get clipped by mapping or display.        */
static BOOL clipCoords(FeatureMap look, SEG *seg, float *y1_out, float *y2_out,
		       BOOL *off_top_out, BOOL *off_bottom_out) ;

/* Gradually more generalised drawing routines are being added..... */
static BOOL drawAlleleStyle(FeatureMap look, float *offset, SEG *seg,
			    AlleleDrawType allele_draw, int *box_out) ;
static void drawInterruptedRectangle(FeatureMap look, Array gaps,
				     float x1, float y1,  float x2, float y2) ;


/* Routines for controlling how blixem is called.                            */
static BOOL blixemGetPfetchParams(STORE_HANDLE handle, PfetchParams **out) ;
static BOOL blixemIsExternal(void) ;
static void blixemCallExternal(char *seq, char *seqname,
			       int dispStart, int offset, MSP *msp,
			       BOOL extended_format, char *opts,
			       PfetchParams *pfetch_params, char *align_types) ;
/* Generic functions for selecting and unselecting SEGs */
static void selectSeg (int box) ;
static void unselectSeg (int box) ;
 


/******************************************************/
/*************** gene menu ****************************/

/* If you alter this menu be sure that the indexs for CDS translation and
 * complete translation are changed if they need to be. */
#define CDS_TRANS_ON       "Show CDS translation"
#define CDS_TRANS_OFF      "Hide CDS translation"
#define COMPLETE_TRANS_ON  "Show Complete translation"
#define COMPLETE_TRANS_OFF "Hide Complete translation"

#define CDS_TRANS_INDEX 0
#define COMPLETE_TRANS_INDEX 4

static MENUOPT geneMenu[] = {
  { (GraphFunc)translateCDS,		CDS_TRANS_ON },
  { (GraphFunc)showCDSInPepDisp,        "Display CDS translation in protein window" },
  { (GraphFunc)exportCDSTranslation,	"Export CDS translation" },
  { menuSpacer,             "" },
  { (GraphFunc)translateAll,		COMPLETE_TRANS_ON },
  { (GraphFunc)showAllInPepDisp,        "Display complete translation in protein window" },
  { (GraphFunc)exportCompleteTranslation,"Export complete translation" },
  { menuSpacer,             "" },
  { (GraphFunc)fMapCurateCDS,		"Curate CDS" },
  { menuSpacer,             "" },
  { (GraphFunc)fMapShowcDNA,		"Display cDNA in new window" },
  { (GraphFunc)fMapExportcDNA,		"Export cDNA" },
  { menuSpacer,             "" },
  { (GraphFunc)fMapShowCDS,		"Display CDS in new window" },
  { (GraphFunc)fMapExportCDS,		"Export CDS" },
  { menuSpacer,             "" },
  { (GraphFunc)fMapColorIntronsExons,	"Highlight Exons" },
#ifndef MACINTOSH
  { (GraphFunc)fMaptRNAfold,		"Fold tRNA" },
#endif /* !MACINTOSH */
  { 0, 0 }
} ;

static MENUOPT pepHomolMenu[] = {
  { (GraphFunc)fMapShowPepAlign,     "Show multiple protein alignment in Blixem"},
  { (GraphFunc)fMapShowPepAlignThis, "Show multiple protein alignment of just this kind of homologies"},
  { (GraphFunc)fMapExpandAlign,      "Develop a table of these homologies" },  
  { (GraphFunc)fMapFetch,            "(slow) Import this protein via the web" },
  { (GraphFunc)fMapEfetch,           "EFetch this sequence" },
  { (GraphFunc)fMapPfetch,           "PFetch this sequence" },
  { (GraphFunc)selectSeg,            "Select this segment" },
  { (GraphFunc)unselectSeg,          "Unselect this segment" },
  { 0, 0 }
} ;

static MENUOPT dnaHomolMenu[] = {
  { (GraphFunc)fMapShowDNAAlign,     "Show multiple dna alignment"},
  { (GraphFunc)fMapShowDNAAlignThis, "Show multiple dna alignment of just this kind of homologies"},
  { (GraphFunc)fMapExpandAlign,      "Develop a table of these homologies" },  
  { (GraphFunc)fMapEfetch,           "EFetch this sequence" },
  { (GraphFunc)fMapPfetch,           "PFetch this sequence" },
  { (GraphFunc)selectSeg,            "Select this segment" },
  { (GraphFunc)unselectSeg,          "Unselect this segment" },
  { 0, 0 }
} ;


/*************************************/

/* column "Locator" */
void fMapShowMiniSeq (LOOK genericLook, float *offset, MENU *unused)
     /* This is of type MapColDrawFunc  */
{
  FeatureMap look = (FeatureMap)genericLook;
  int i ;
  float y, newoff, x0 ;
  SEG *seg ;

  newoff = *offset + 5 ;
  mapShowLocator(genericLook, &newoff, NULL) ;
  x0 = (look->flag & FLAG_REVERSE) ? look->map->max : 0 ;

     /* Position main Markers = loci just now */
  graphTextHeight (0.75) ;
  for (i = 1 ; i < arrayMax(look->segs) ; ++i)
    { seg = arrp(look->segs,i,SEG) ;
      if (class(seg->key) == _VLocus)
	{ y = MAP2WHOLE(look->map,(seg->x1 + seg->x2 - 2*x0)/2.0) ;
	  graphText (name(seg->key), *offset+0.5, y-0.4) ;
	}
    }
  graphTextHeight (0) ;
  *offset = newoff ;

  return;
} /* fMapShowMiniSeq */

/************************************/


void fMapShowScale (LOOK genericLook, float *offset, MENU *unused)
     /* This is of type MapColDrawFunc */
{
  FeatureMap look = (FeatureMap)genericLook;
  float unit, subunit ;
  int iUnit, iSubunit, type, unitType ;
  int x, xf, xr, width = 0 ;
  float y ;
  char *cp, unitName[] = { 0, 'k', 'M', 'G', 'T', 'P' }, buf[2] ;
  BOOL isReverse ;

  xf = look->origin - 1 ;
  xr = look->length - look->origin ;
  isReverse = ((look->flag & FLAG_REVERSE) != 0) ;

  unit = subunit = 1.0 ;
  mapFindScaleUnit (look->map, &unit, &subunit) ;
  iUnit = unit + 0.5 ;
  iSubunit = subunit + 0.5 ;

  if (isReverse)
    x = xr - iUnit * ((xr - look->min)/iUnit) ;
  else
    x = xf + iUnit * ((look->min - xf)/iUnit) ;
  if (x < look->min)
    x += unit ;

  for (type = 1, unitType = 0 ; unit > 0 && 1000 * type < unit && unitType < 5; 
       unitType++, type *= 1000) ;

  for ( ; x < look->max ; x += unit)
    {
      y = MAP2GRAPH(look->map, x) ;
      graphLine (*offset, y, *offset, y) ;
      buf[0] = unitName[unitType] ; buf[1] = 0 ;
      cp = messprintf ("%d%s", COORD(look,x)/type, buf) ;
      if (width < strlen (cp))
        width = strlen (cp) ;
      graphText (cp, *offset+1.5, y-0.5) ;
    }

  if (isReverse)
    x = xr - iSubunit * ((xr - look->min)/iSubunit) ;
  else
    x = xf + iSubunit * ((look->min - xf)/iSubunit) ;
  if (x < look->min)
    x += subunit ;
  for ( ; x < look->max ; x += subunit)
    {
      y = MAP2GRAPH(look->map, x) ;
      graphLine (*offset+0.5, y, *offset+1, y) ;
    }

  graphLine (*offset+1, topMargin+2, *offset+1, mapGraphHeight-0.5) ;
  *offset += width + 4 ;

  return;
} /* fMapShowScale */





/**********************************************************/
/**********************************************************/

/******************** geneMenu functions ******************/

/* Crude, but what else to do when the callback interface doesn't provide    */
/* for passing of application data...                                        */
static void translateCDS(int box)
{
  BOOL cds_only = TRUE ;

  translateGene(box, cds_only) ;

  return ;
}

static void translateAll(int box)
{
  BOOL cds_only = FALSE ;

  translateGene(box, cds_only) ;

  return ;
}

static void translateGene(int box, BOOL cds_only)
{
  BOOL isNewGene = FALSE, new_translation, no_dna = FALSE ;
  SEG *seg ;
  FeatureMap look = currentFeatureMap("translateGene") ;
  MapColSet column_set ;

  if (box >= arrayMax(look->boxIndex) || !(seg = BOXSEG(box)))
    {
      messerror ("problem picking boxes for translation");
      return ;
    }

  /* Has user asked for a change of which part of dna is translated ?        */
  /* i.e. CDS -> complete  or  complete -> CDS                               */
  if (arrp(look->seqInfo, seg->data.i, SEQINFO)->cds_info.cds_only ^ cds_only)
    new_translation = TRUE ;
  else
    new_translation = FALSE ;

  arrp(look->seqInfo, seg->data.i, SEQINFO)->cds_info.cds_only = cds_only ;

  if (look->translateGene)	/* there was a previous selection */
    {
      if (look->translateGene != seg->parent)
	{
	  /* the user has selected a different gene for translation */
	  isNewGene = TRUE ;
	  
	  /* May be we should always recompute to pick up errors... */
	  look->pleaseRecompute = TRUE ;
	}
    }
  else
    look->pleaseRecompute = TRUE ;



  /* select the gene for translation, from the selected SEG */
  look->translateGene = seg->parent ;

  /* Before fiddling with column settings we need to check whether we can    */
  /* translate the dna, we do this by trying to get the dna.                 */
  /* If this is too time consuming we could replace fMapGetDNA with a new    */
  /* routine that just checks whether its possible to get the DNA.           */
  if (new_translation)
    {
      Array cds, index ;

      if (!(fMapGetDNA(look, look->translateGene, &cds, &index, cds_only)))
	{
	  messout("Select a down gene to translate with the pulldown menu on the genes") ;
	  no_dna = TRUE ;
	  look->pleaseRecompute = TRUE ;		    /* Make sure we redraw. */
	}
      else
	{
	  no_dna = FALSE ;
	  arrayDestroy (cds) ;
	  arrayDestroy (index) ;
	}
    }


  /* We want the column drawn if a new gene is selected OR if section of     */
  /* gene to be drawn has changed.                                           */
  if (no_dna)
    column_set = MAPCOL_OFF ;
  else if (isNewGene || new_translation)
    column_set = MAPCOL_ON ;
  else
    column_set = MAPCOL_TOGGLE ;
    
  if (seg->type == EXON_UP)
    {
      mapColSetByName(look->map, "Down Gene Translation", MAPCOL_OFF) ;
      mapColSetByName(look->map, "Up Gene Translation", column_set);
    }
  else
    {
      mapColSetByName(look->map, "Down Gene Translation", column_set);
      mapColSetByName(look->map, "Up Gene Translation", MAPCOL_OFF) ;
    }

  fMapDraw (look, 0) ;

  return;
} /* translateGene */

/**************************************/

/* This is a bit clumsy but there is no built in mechanism in the graphmenu  */
/* stuff for passing through "application data" so what else to do...???     */
/*                                                                           */
static void showCDSInPepDisp(int box)
{
  showInPepDisp(box, TRUE) ;				    /* Translate the CDS only. */

  return ;
}

static void showAllInPepDisp(int box)
{
  showInPepDisp(box, FALSE) ;				    /* Translate all the DNA. */

  return ;
}

static void showInPepDisp(int box, BOOL CDS_only)
{ 
  SEG *seg ;
  Array pep ;
  FeatureMap look = currentFeatureMap("showInPepDisp") ;

  if (box >= arrayMax(look->boxIndex) || !(seg = BOXSEG(box)))
    { messerror ("Problem picking box to show in PEP disp");
      return;
    }

  if ((pep = peptideTranslate(seg->parent, CDS_only)))
    {
      PepDisplayData pep_create ;

      arrayDestroy(pep) ;

      pep_create.CDS_only = CDS_only ;

      displayApp(seg->parent, look->seqKey, "PEPMAP", (void *)&pep_create) ;
    }
  else
    messerror ("Can't generate protein sequence") ;

  return;
} /* showInPepDisp */


/**************************************/

/* These functions show the spliced DNA of a set of exons in a separate      */
/* fmap window. If our menu system catered for application data to be passed */
/* through we wouldn't need these cover functions.                           */
/*                                                                           */
static void fMapShowcDNA (int box)
{
  showDNA(box, FALSE) ;

  return ;
}

static void fMapShowCDS(int box)
{
  showDNA(box, TRUE) ;

  return ;
}

static void showDNA(int box, BOOL cds_only)
{
  KEY key ;
  Array cds = 0 ;
  SEG *seg ; 
  Stack s ;
  FeatureMap look = currentFeatureMap("fMapShowcDNA");
  ACEOUT cdna_out;
  OBJ obj = NULL ;
  char *temp_suffix ;

  if (box >= arrayMax(look->boxIndex) || !(seg = BOXSEG(box)))
    {
      messerror ("problem picking boxes in fMapShowcDNA");
      return;
    }


  /* Can fail because caller asked for CDS DNA but object is not a CDS.      */
  if (!fMapGetDNA(look, seg->parent, &cds, 0, cds_only))
    {
      messerror("Either no DNA present or you have asked for CDS DNA"
		" and the object is not a CDS.") ;
      return ;
    }


  /* Create a temporary sequence object containing just the cDNA and display
   * it in an fmap window. This object will get deleted when the user quits
   * the fmap window. We pass on some of the tags from the object that was
   * used to create this seg because they are needed for examining the cDNA
   * in the new temporary object.
   * n.b. must have different suffixes to produce different temporary objects */
  if (cds_only)
    temp_suffix = "CDS.cDNA" ;
  else
    temp_suffix = "cDNA" ;
  lexaddkey(messprintf("%s.%s", name(seg->parent), temp_suffix), &key, _VSequence) ;

  s = stackCreate (500) ;
  cdna_out = aceOutCreateToStack (s, 0);

  aceOutPrint(cdna_out, "Sequence %s\ncDNA\n", name(key)) ;

  obj = bsCreate(seg->parent) ;

  if (bsFindTag(obj, _CDS))
    { 
      int cds_min = 0, cds_max = 0 ;

      aceOutPrint(cdna_out, "CDS") ;

      /* If we are only exporting the CDS section of the DNA we do not need  */
      /* the start/end.                                                      */
      if (!cds_only)
	{
	  if (bsGetData(obj, _bsRight, _Int, &cds_min))
	    {
	      aceOutPrint(cdna_out, " %d", cds_min) ;
	      if (bsGetData(obj, _bsRight, _Int, &cds_max))
		aceOutPrint(cdna_out, " %d", cds_max) ;
	    }
	}

      aceOutPrint(cdna_out, "\n") ;
    }
  else
    {
      /* Dreadful hack, fmapconvert() needs to find a CDS tag in order to do */
      /* two things: 1) set the drawing method correctly, 2) create a dummy  */
      /* source exon seg for the method to draw. So even though we know      */
      /* there is not a CDS I output a cds tag so that fmapconvert() will do */
      /* the drawing. A better way would be to create a dummy source exon    */
      /* here from the original objects Source_Exons and alter fmapconvert() */
      /* so that it would draw them, maybe there is code to do this but I    */
      /* don't know the correct tag to do this....the #define code below     */
      /* will do this when I think of the way to fix up fmapconvert().       */

      aceOutPrint(cdna_out, "CDS\n") ;


#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
      /* Dreadful hack, fmapconvert needs to find a CDS tag in order for it to   */
      /* make a dummy source exon to display in the fmap, if there is no CDS tag */
      /* the source exon doesn't get created and nothing is displayed. To    */
      /* avoid this, we fix up our own source exon to cover the whole of     */
      /* this object.                                                        */

      /* We should always be able to find the source exons because the user  */
      /* will have clicked on them to get this code executed.                */
      if (bsFindTag(obj, _Source_Exons))
	{
	  Array units = 0 ;
	  
	  units = arrayCreate(256, BSunit) ;

	  if (bsFlatten(obj, 2, units))
	    {
	      int i ;
	      int start, end ;

	      dnaExonsSort (units) ;

	      start = arr(units, 0, BSunit).i ;

	      for (i = 0, end = 0 ; i < arrayMax(units) ; i += 2)
		{
		  end += (arr(units, i+1, BSunit).i - arr(units, i, BSunit).i + 1) ;
		}

	      aceOutPrint(cdna_out, "Source_Exons %d %d\n", start, end) ;


	    }
	  else
	    {
	      messerror("Potential problem with database or acedb code, found source exons"
			" but cannot access them in object %s, fMapShowcDNA() failed.",
			name(seg->parent)) ;

	      arrayDestroy(units) ;
	      return ;
	    }

	  arrayDestroy(units) ;
	}
      else
	{
	  messerror("Potential problem with database or acedb code,"
		    " cannot find source exons in object %s, fMapShowcDNA() failed.",
		    name(seg->parent)) ;
	  return ;
	}
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

    }

  if (bsFindTag(obj, _Start_not_found))
    {
      int start = 0 ;

      aceOutPrint(cdna_out, "Start_not_found") ;

      if (bsGetData(obj, _bsRight, _Int, &start))
	aceOutPrint(cdna_out, " %d", start) ;

      aceOutPrint(cdna_out, "\n") ;
    }

  if (bsFindTag(obj, _End_not_found))
    {
      aceOutPrint(cdna_out, "End_not_found\n") ;
    }

  bsDestroy(obj) ;

  aceOutPrint(cdna_out, "\nDNA %s\n", name(key)) ;
  dnaDecodeArray (cds) ;
  aceOutPrint (cdna_out, "%s", arrp(cds,0,char)) ;
  aceOutPrint (cdna_out, "\n\n") ;

  aceOutDestroy (cdna_out);
  arrayDestroy (cds) ;

  if (parseBufferInternal(stackText(s,0), 0, FALSE))
    {
      FmapDisplayData create_data ;

      displayPreserve () ;

      create_data.destroy_obj     = TRUE ;
      create_data.features        = NULL ;
      create_data.include_methods = FALSE;
      create_data.include_sources = FALSE;
      create_data.methods_dict    = NULL ;
      create_data.sources_dict    = NULL ;

      displayApp(key, 0, 0, (void *)&create_data);
    }
  else
    messerror ("Parsing of %s to create cDNA object for fMapShowcDNA failed", name(key)) ;

  stackDestroy (s) ;

  return;
} /* fMapShowcDNA */

/**************************************/

static void fMapColorIntronsExons (int box)
{
  int i, j1, j2, j3, colour, length ;
  Array cds, index, colors ;
  SEG *seg ;
  FeatureMap look = currentFeatureMap("fMapColorIntronsExons");
  Array tt = NULL ;

  if (box >= arrayMax(look->boxIndex) || !(seg = BOXSEG(box)))
    {
      messerror ("problem picking boxes for colouring");
      return;
    }

  if (!fMapGetCDS(look, seg->parent, &cds, &index))
    {
      messerror ("ColorIntronsExons only works on coding genes") ;
      return ;
    }

  if (!(tt = pepGetTranslationTable(seg->parent, KEY_UNDEFINED)))
    return ;

  length = look->length ;

  if (arrayMax(cds) % 3)
    messout ("Coding length is %d mod 3", arrayMax(cds) % 3) ;

  fMapClearDNA (look) ;
  colors = look->colors ;

  j1 = arr(index,0,int) ;
  j2 = arr(index,arrayMax(index)-1,int) ;
  if (j2 < j1)
    {
      j3 = j1 ;
      j1 = j2 ;
      j2 = j3 ; 
    }
  if (j1 < 0)
    j1 = 0 ;
  if (j2 >= length)
    j2 = length-1 ;

  /* do not color the introns they print same as exons
    if CYAN and LIGHTGREEN
     for (i = j1 ; i <= j2 ; ++i)
    arr(colors,i,char) |= TINT_LIGHTGREEN ;
    */ 

  for (i = 0 ; i < arrayMax(index)-2 ; i += 3)
    {
      j1 = arr(index,i,int) ;
      j2 = arr(index,i+1,int) ;
      j3 = arr(index,i+2,int) ;
      if (j1 < 0 || j3 < 0 || j1 >= length || j3 >= length)
	continue ;

      if (j3 == j1+2 || j1 == j3+2)
	{
	  if (e_codon(arrp(cds,i,char), tt) == '*')
	    colour = TINT_RED ;
	  else
	    colour = TINT_YELLOW ; /* was  TINT_CYAN ; */
	}
      else
	colour = TINT_MAGENTA ;

      arr(colors,j1,char) |= colour ; 
      arr(colors,j2,char) |= colour ; 
      arr(colors,j3,char) |= colour ; 
    }

  arrayDestroy (cds) ;
  arrayDestroy (index) ;

  mapColSetByName(look->map, "DNA Sequence", MAPCOL_ON) ;
  look->activeBox = 0 ;
  fMapDraw (look, 0) ;
  
  return;
} /* fMapColorIntronsExons */


/**************************************/

/* This is a bit clumsy but there is no built in mechanism in the graphmenu  */
/* stuff for passing through "application data" so what else to do...???     */
/*                                                                           */
static void exportCDSTranslation(int box)
{
  exportTranslation(box, TRUE) ;			    /* Translate the CDS only. */

  return ;
}

static void exportCompleteTranslation(int box)
{
  exportTranslation(box, FALSE) ;			    /* Translate all the DNA. */

  return ;
}

static void exportTranslation(int box, BOOL cds_only)
{
  Array dna = NULL ;
  SEG *seg ;
  FeatureMap look = currentFeatureMap("exportTranslation") ;
  BOOL result_unused ;

  if (box >= arrayMax(look->boxIndex) || !(seg = BOXSEG(box)))
    {
      messerror ("Problem picking boxes to export translation");
      return;
    }	

  if (!fMapGetDNA(look, seg->parent, &dna, 0, cds_only))
    {
      messerror ("ExportTranslation only works on coding genes") ;
      return ;
    }

  result_unused = pepTranslateAndExport(seg->parent, dna) ;

  arrayDestroy(dna) ;

  return;
}


/**************************************/


/* These functions export the spliced DNA of a set of exons to a file.       */
/* If our menu system catered for application data to be passed through we   */
/* wouldn't need these cover functions.                                      */
/*                                                                           */
static void fMapExportcDNA (int box)
{
  exportDNA(box, FALSE) ;

  return ;
}

static void fMapExportCDS(int box)
{
  exportDNA(box, TRUE) ;

  return ;
}

static void exportDNA(int box, BOOL cds_only)
{
  static char fname[FIL_BUFFER_SIZE]="", dname[DIR_BUFFER_SIZE]="" ;
  ACEOUT cdna_out;
  Array cds = 0 ;
  int i ;
  SEG *seg ;
  FeatureMap look = currentFeatureMap ("fMapExportcDNA") ;
  char *feature = (cds_only ? "CDS" : "cDNA") ;
  char *file_ext = (cds_only ? "cds" : "cdna") ;
  char *chooser_txt = NULL ;

  if (box >= arrayMax(look->boxIndex) || !(seg = BOXSEG(box)))
    {
      messerror ("problem picking boxes to export %s", feature);
      return;
    }
    
  if (!fMapGetDNA(look, seg->parent, &cds, 0, cds_only))
    {
      messerror ("Export %s only works on coding genes", feature) ;
      return ;
    }

  chooser_txt = hprintf(0, "File to store %s in", feature) ;
  cdna_out = aceOutCreateToChooser (chooser_txt,
				    dname, fname, file_ext, "a", 0);
  messfree(chooser_txt) ;

  if (!cdna_out)
    {
      arrayDestroy (cds) ;
      return ;
    }

  translationTitle(cdna_out, seg->parent) ;

  for (i = 0 ; i < arrayMax(cds) ; ++i)
    {
      aceOutPrint (cdna_out, "%c", dnaDecodeChar[(int)arr(cds,i,char)]);
      if (((i+1) % 50) == 0)
	aceOutPrint (cdna_out, "\n");
    }
  if (i % 50)
    aceOutPrint (cdna_out, "\n");
  aceOutDestroy (cdna_out);

  arrayDestroy (cds) ;

  return;
} /* fMapExportcDNA */

/**********************************************************/
/**************** select functions ************************/

static void selectFeature (FeatureMap look, BOOL isSelect, SegType type, 
			   int x1, int x2)
{
  if (isSelect)
    { 
      assInsert (look->chosen, HASH (type, x2), assVoid(x1)) ;
      assRemove (look->antiChosen, HASH (type, x2)) ;
    }
  else
    {
      assRemove (look->chosen, HASH (type, x2)) ;
      assRemove (look->antiChosen, HASH (type, x2)) ;
    }

  return ;
}

static void selectChangeSeg (int box, BOOL isSelect)
{ 
  SEG *seg, *seg2 ;
  int i ;
  FeatureMap look = currentFeatureMap("intronConfirm") ;
  
  if (box >= arrayMax(look->boxIndex) || !(seg = BOXSEG(box)))
    { 
      messerror ("Problem picking box in selectSeg") ; 
      return ; 
    }
  
  switch (seg->type)
    { 
    case INTRON:
      selectFeature (look, SPLICE5, isSelect, seg->x1-1, seg->x1) ;
      selectFeature (look, SPLICE3, isSelect, seg->x2, seg->x2+1) ;
      break ;
      
    case CDS:
      selectFeature (look, ATG, isSelect, seg->x1, seg->x1+2) ;
      selectFeature (look, ORF, isSelect, seg->x2-5, seg->x2-3) ;
      break ;

    case EXON:   /* find CDS with same parent */
      for (i = 0 ; i < arrayMax(look->segs) ; ++i)
	{
	  seg2 = arrp(look->segs, i, SEG) ;
	  if (seg2->type == CDS && seg2->parent == seg->parent)
	    break ;
	}
      if (i == arrayMax (look->segs)) /* do nothing if no CDS */
	break ;
      if (seg->x1 > seg2->x1)
	selectFeature (look, SPLICE3, isSelect, seg->x1-1, seg->x1) ;
      else if (seg2->x1 < seg->x2)
	selectFeature (look, ATG, isSelect, seg2->x1, seg2->x1+2) ;
      if (seg->x2 < seg2->x2)
	selectFeature (look, SPLICE5, isSelect, seg->x2, seg->x2+1) ;
      else if (seg2->x2 > seg->x1)
	selectFeature(look, ORF, isSelect, seg2->x2-5, seg2->x2-3) ;
      break ;

    case HOMOL:        /* make splices at exact internal gaps > 25 */
      {
	HOMOLINFO *hinf = arrp(look->homolInfo, seg->data.i, HOMOLINFO) ;

	for (i = 0 ; i < arrayMax(look->segs) ; ++i)
	  {
	    seg2 = arrp (look->segs, i, SEG) ;
	    if (seg2->key == seg->key)
	      {
		if (seg2->x2 + 25 < seg->x1
		    && hinf->x1 == 1 + arrp(look->homolInfo, seg2->data.i, HOMOLINFO)->x2)
		  selectFeature (look, SPLICE3, isSelect, seg->x1-1, seg->x1) ;
		if (seg->x2 + 25 < seg2->x1
		    && hinf->x2 + 1 == arrp(look->homolInfo, seg2->data.i, HOMOLINFO)->x1)
		  selectFeature (look, SPLICE5, isSelect, seg->x2, seg->x2+1) ;
	      }
	  }
	break ;
      }

    default:
      break ;
    }

  return ;
}

static void selectSeg(int box)
{
  selectChangeSeg (box, TRUE) ;

  return ;
}

static void unselectSeg (int box)
{
  selectChangeSeg (box, FALSE) ;

  return ;
}
 

/***************** intronMenu functions *******************/

/* This routine used only to create "Confirmed Introns" on the forward strand for introns that
 * already existed but you can now confirm introns on either strand. */
static void intronConfirm (int box, int flag)
{
  SEG *seg ;
  OBJ obj ;
  KEY key ;
  int x, y ;
  SEQINFO *oldSinf, *newSinf ;
  FeatureMap look = currentFeatureMap("intronConfirm") ;
  
  if (box >= arrayMax(look->boxIndex) || !(seg = BOXSEG(box)))
    {
      messerror ("Problem picking box in intronConfirm") ; 
      return ; 
    }

  /* The coordinates are tricky here, we use seg coords for the fMap call which are 0-based, _but_
   * the coords in the database are 1-based so we must add one AND we must note the strand. */
  key = KEY_UNDEFINED ;
  if (SEG_IS_DOWN(seg))
    {
      x = seg->x1 ;
      y = seg->x2 ;
    }
  else
    {
      y = seg->x1 ;
      x = seg->x2 ;
    }
  if (!fMapFindSpanSequenceWriteable(look, &key, &x, &y))
    {
      messerror("Can't find any parent object to store confirmation data in to.") ;

      return ;
    }
  x++ ; y++ ;						    /* convert to 1-based coords. */


  if (!(obj = bsUpdate(key)))
    {
      messerror("Can't open object %s to store confirmation data in to.", nameWithClassDecorate(key)) ;

      return ;
    }

  bsAddData (obj, str2tag("Confirmed_intron"), _Int, &x) ;
  bsAddData (obj, _bsRight, _Int, &y) ;
  bsPushObj (obj) ;
  switch (flag)
    { 
    case SEQ_CONFIRM_EST: bsAddTag (obj, str2tag("EST")) ; break ;
    case SEQ_CONFIRM_HOMOL: bsAddTag (obj, str2tag("Homology")) ; break ;
    case SEQ_CONFIRM_CDNA: bsAddTag (obj, str2tag("cDNA")) ; break ;
    case SEQ_CONFIRM_UTR: bsAddTag (obj, str2tag("UTR")) ; break ;
    case SEQ_CONFIRM_FALSE: bsAddTag (obj, str2tag("False")) ; break ;
    case SEQ_CONFIRM_INCONSIST: bsAddTag (obj, str2tag("Inconsistent")) ; break ;
    case 0: bsPrune (obj) ; break ;
    default: messerror ("Unrecognised flag in intronConfirm") ; break ;
    }
  bsSave (obj) ;

  newSinf = arrayp(look->seqInfo, arrayMax(look->seqInfo), SEQINFO) ;
  oldSinf = arrp(look->seqInfo, seg->data.i, SEQINFO) ;	/* must follow in case seqInfo gets relocated. */
  seg->data.i = arrayMax(look->seqInfo)-1 ;
  *newSinf = *oldSinf ;
  if (flag)
    {
      newSinf->flags |= flag ;
      graphBoxDraw (box, -1, newSinf->flags & SEQ_CONFIRM_UTR ? YELLOW : GREEN) ;
    }
  else
    {
      newSinf->flags &= ~SEQ_CONFIRMED ; /* clear all CONFIRM flags */
      graphBoxDraw (box, -1, WHITE) ;
    }

  return;
}

static void intronConfirmEST (int box) 
{ intronConfirm (box, SEQ_CONFIRM_EST) ; }

static void intronConfirmHomol (int box)
{ intronConfirm (box, SEQ_CONFIRM_HOMOL) ; }

static void intronConfirmCDNA (int box)
{ intronConfirm (box, SEQ_CONFIRM_CDNA) ; }

static void intronConfirmUTR (int box)
{ intronConfirm (box, SEQ_CONFIRM_UTR) ; }

static void intronConfirmFalse (int box)
{ intronConfirm (box, SEQ_CONFIRM_FALSE) ; }

static void intronConfirmIncon (int box)
{ intronConfirm (box, SEQ_CONFIRM_INCONSIST) ; }

static void intronUnconfirm (int box)
{ intronConfirm (box, 0) ; }

static MENUOPT intronMenu[] = {
  { (GraphFunc)intronConfirmEST, "Confirm by EST" },
  { (GraphFunc)intronConfirmHomol, "Confirm by Homol" },
  { (GraphFunc)intronConfirmCDNA, "Confirm by CDNA" },
  { (GraphFunc)intronConfirmUTR, "Confirm in UTR" },
  { (GraphFunc)intronConfirmFalse, "Confirm as False" },
  { (GraphFunc)intronConfirmIncon, "Confirm as Inconsistent" },
  { (GraphFunc)intronUnconfirm, "Unconfirm" },
  { 0, 0 }
} ;

/*****************************/

static void fMapBoxInfo (FeatureMap look, int box, SEG *seg)
{ 
  /* only necessary in a non-interactive graphics session, e.g. giface */
  if (isGifDisplay)
    {
      fMapReportLine (look, seg, TRUE, 0) ;
      
      graphBoxInfo (box, seg->key ? seg->key : seg->parent, 
		    strnew (look->segTextBuf, look->segsHandle)) ;
    }

  return;
} /* fMapBoxInfo */


/*************************************/

/* THESE FUNCTIONS ARE CALLED FROM mapcontrol.c, basically to find out what  */
/* sort of method will be used to draw a column, seems crazy, mapcontrol     */
/* is doing the drawing but doesn't know the type of its own columns...      */
/* wierd encapsulation... It needs to, not only to draw the pull down menus, */
/* but also to do some frame related stuff...don't know what goes on in that */
/* bit.                                                                      */

/* Can a column have an interactive menu ??                                  */
/* This is a big hacky, I think that the cols themselves should "know" if    */
/* they are configurable, this would be much better. They really need to     */
/* retain more information than just the column drawing function pointer.    */
/* I'll have a think about this.                                             */
/*                                                                           */
BOOL fmapColHasMenu(MapColDrawFunc func)
{
  BOOL result = FALSE ;

  if (func == fMapShowSequence
      || func == fMapShowHomol
      || func == fMapShowFeature
      || func == fMapShowFeatureObj
      || func == fMapShowSplices
      || func == fMapShowCanonical)
    result = TRUE ;

  return result ;
}

/* Called from mapcontrol.c where comments are cryptic.                      */
BOOL fmapIsFrameFunc(MapColDrawFunc func)
{
  BOOL result = FALSE ;

  if (func == fMapShow3FrameTranslation
      || func == fMapShowORF)
    result = TRUE ;

  return result ;
}

/* called from mapcontrol.c where comments are cryptic.                      */
BOOL fmapIsGeneTranslationFunc(MapColDrawFunc func)
{
  BOOL result = FALSE ;

  if (func == fMapShowDownGeneTranslation
      || func == fMapShowUpGeneTranslation)
    result = TRUE ;

  return result ;
}




/*************************************/

/* This is of type MapColDrawFunc and is called to draw either a sequence    */
/* block or for a series of exons/introns.                                   */
/* Something to note about drawing of exons when there is a cds: we rely on  */
/* the fact that segs have been sorted so that the cds for a set of exons    */
/* will come _before_ the exons in the array of segs. This is achieved by    */
/* sorting in fMapConvert.                                                   */
/*                                                                           */
static void drawInterruptedRectangle(FeatureMap look, Array gaps,
				     float x1, float y1,  float x2, float y2)
{
  float xmid = (x1+x2)/2;
  float p1, p2;
  int j;
  SMapMap *block ;
  
  if (y1 > y2)
    {
      float tmp = y2;
      y2 = y1;
      y1 = tmp;
    } 

  block = arrp(gaps, 0, SMapMap) ;

  p2 = MAP2GRAPH(look->map, block->r1) ;
  for (j = 0 ; j < arrayMax(gaps) ; j++)
    {	      
      float s1, s2;

      block = arrp(gaps, j, SMapMap) ;

      p1 = MAP2GRAPH(look->map, block->r1);

      if (p1 < p2)
	{
	  s1 = p1;
	  s2 = p2;
	}
      else
	{ 
	  s1 = p2; 
	  s2 = p1;
	}

      if (s1 != s2 && s1 <= y2 && s2 >= y1)
	{
	  float k;

	  if (s1 < y1)
	    s1 = y1;

	  if (s2 > y2)
	    s2 = y2;

	  for (k = s1; k < s2; k++)
	    {
	      graphLine(xmid, k, xmid, k+0.6 < s2 ? k+0.6 : s2) ;
	    }
	}

      p2 = MAP2GRAPH(look->map, block->r2) ;
      if (p1 < p2)
	{
	  s1 = p1;
	  s2 = p2;
	}
      else
	{ 
	  s1 = p2; 
	  s2 = p1;
	}

      if (s1 <= y2 && s2 >= y1)
	{
	  if (s1 < y1)
	    s1 = y1;
	  if (s2 > y2)
	    s2 = y2;

	  graphRectangle(x1, s1, x2, s2) ;
	}
      
    }

  return ;
}

/* Shows sequence and exon/intron objects.
 *  */
void fMapShowSequence (LOOK genericLook, float *offset, MENU *menu)
{
  FeatureMap look = (FeatureMap)genericLook;
  void  *xx ;
  int	i, box, x ;
  float y1, y2, yb ;
  SEG	*seg ;
  Associator seq2x = assCreate () ;
  SEQINFO *sinf ;
  BoxCol *bc ;
  BOOL isData = FALSE;
  BOOL firstError = TRUE ;
  BOOL off_top, off_bottom ;
  BOOL CDS_present = FALSE ;
  float CDS_top, CDS_bottom ;
  BOOL CDS_off_top, CDS_off_bottom ;
  float left, right ;


  fMapDebug = FALSE;

  if (fMapDebug)
    printf("\nstart:\n") ;

  /* Set up drawing style for sequence/exons/introns                         */
  bc = bcFromName (look, look->map->activeColName, menu) ;

  if (!bcTestMag (bc, look->map->mag)) 
    return ;

  bc->offset = *offset ;

  /* THIS IS NOT THE WAY TO SET DEFAULTS....AGGGGGGGHHHHHHHH........ */
  if (!bc->meth->width) 
    bc->width = 1.25 ;
  if (!bc->meth->colour)
    bc->meth->colour = BLUE ;
  if (!bc->meth->CDS_colour)
    bc->meth->CDS_colour = PURPLE ;


  /* Draw all the sequence/exon/intron segs.                                 */
  for (i = 1 ; i < arrayMax(look->segs) ; ++i)
    { 
      seg = arrp(look->segs, i, SEG) ;

      if (seg->type != SEQUENCE && seg->type != SEQUENCE_UP
	  && seg->type != EXON && seg->type != EXON_UP
	  && seg->type != INTRON && seg->type != INTRON_UP)
	continue ;

      if ((seg->type & 0x01) == bc->isDown)
	continue ;

      if (fMapDebug)
	printf("SEG: parent: %s, type: %s at %d, %d before clip coords\n",
	       name(seg->parent), fMapSegTypeName[seg->type], seg->x1, seg->x2) ;

      /* Set top/bottom coords for objects that will be clipped.             */
      if (!clipCoords(look, seg, &y1, &y2, &off_top, &off_bottom))
	continue ;

      if (fMapDebug)
	printf("SEG: parent: %s, type: %s at %d, %d after clip coords\n",
	       name(seg->parent), fMapSegTypeName[seg->type], seg->x1, seg->x2) ;

      switch (seg->type)
	{ 
	case SEQUENCE:
	case SEQUENCE_UP:
	  {
	    sinf = arrp(look->seqInfo, seg->data.i, SEQINFO) ;

	    if (lexAliasOf(sinf->method) != bc->meth->key)
	      break ;

	    x = 0 ;
	    yb = seg->x1 ; /* bump in seg coords, since increasing */

	    isData = TRUE ;
	    if (bc->bump) 
	      bumpItem (bc->bump, 1, (seg->x2-seg->x1+1), &x, &yb) ;
	    assInsert (seq2x, assVoid(seg->parent), assVoid(x+1)) ;
	    if (bc->meth->isShowText)
	      addVisibleInfo (look, i) ;

	    if (sinf->flags & SEQ_EXONS) /* will draw exons later */
	      break ;
	 	  
	    box = graphBoxStart () ;
	    array(look->boxIndex, box, int) = i ;

	    if ((bc->meth->flags & METHOD_DISPLAY_GAPS) && sinf->gaps)
	      {
		/* gapped */
		drawInterruptedRectangle(look, sinf->gaps,
					 *offset + (x + 0.25)*bc->width, y1,
					 *offset + (x + 0.75)*bc->width, y2);
	      }
	    else
	      {
		/* no gaps */
		graphRectangle (*offset + (x + 0.25)*bc->width, y1, 
				*offset + (x + 0.75)*bc->width, y2) ;
	      }
	  
	    graphBoxEnd() ;

	    fMapBoxInfo (look, box, seg) ;
	    graphBoxDraw (box, bc->meth->colour, WHITE) ;
	  
	    break ;
	  }
	case EXON: case EXON_UP:
	  {
	    sinf = arrp(look->seqInfo, seg->data.i, SEQINFO) ;
	    if (lexAliasOf(sinf->method) != bc->meth->key)
	      break ;

	    if (assFind (seq2x, assVoid(seg->parent), &xx))
	      x = assInt(xx) - 1 ;
	    else
	      {
		if (firstError)
		  messerror("Can't find exon offset for %s in fMapShowSequence", name(seg->parent)) ;
		firstError = FALSE ;
		break ;
	      }

	    if (sinf->flags & SEQ_CDS)
	      {
		/* If CDS is not on screen then don't draw it. */
		CDS_present = clipCoords(look, &(sinf->cds_info.cds_seg), &CDS_top, &CDS_bottom,
					 &CDS_off_top, &CDS_off_bottom) ;

		if (fMapDebug && CDS_present)
		  printf("%s -  CDS start: %d -> %f, end: %d -> %f\n",
			 nameWithClassDecorate(seg->parent), sinf->cds_info.cds_seg.x1, CDS_top,
			 sinf->cds_info.cds_seg.x2, CDS_bottom) ;
	      }
	    else
	      CDS_present = FALSE ;

	    /* Set stuff that is the same for all exons/introns.               */
	    left = *offset + ((x + 0.1) * bc->width) ;
	    right = *offset + ((x + 0.9) * bc->width) ;
	    changeGeneMenu(look, seg, sinf->cds_info.cds_only) ;

	    if (!CDS_present)
	      {
		fMapDrawBox(look, i, geneMenu, seg,
			    left, y1, right, y2, off_top, off_bottom,
			    bc->meth->colour, WHITE) ;
	      }
	    else
	      {
		float start_rna_top, start_rna_bot ;	    /* pre-CDS dna in exon. */
		float cds_top, cds_bot ;		    /* CDS dna in exon. */
		float end_rna_top, end_rna_bot ;	    /* post-CDS in exon. */
		/* following to record truncation */
		BOOL start_rna_offtop = FALSE ;
		BOOL start_rna_offbot = FALSE ; 
		BOOL cds_offtop = FALSE ;
		BOOL cds_offbot = FALSE ;
		BOOL end_rna_offtop = FALSE ;
		BOOL end_rna_offbot = FALSE ;
	       

		start_rna_top = start_rna_bot = cds_top = cds_bot
		  = end_rna_top = end_rna_bot = 0.0 ;	    /* initialised to 0 for drawing logic. */
			
		/* In drawing the CDS we have to cope with when the coords are */
		/* reversed, the -ve bit means we can keep the tests constant. */
		if (CDS_top > CDS_bottom)
		  {
		    if (y1 < y2)
		      {
			messerror("The exon and CDS coordinates for %s "
				  "are going in different directions. "
				  "Exon top: %d  bot: %d,  "
				  "CDS top: %d  bot: %d.",
				  name(seg->parent), seg->x1, seg->x2, 
				  sinf->cds_info.cds_seg.x1, sinf->cds_info.cds_seg.x2) ;

			break ;
		      }

		    CDS_top = -CDS_top, CDS_bottom = -CDS_bottom, y1 = -y1, y2 = -y2 ;
		  }

		/* Figure out how much of this exon is covered by CDS.         */
		/* Note that drawing will overlap because this is all in floats*/
		if (y1 > CDS_bottom || y2 < CDS_top)
		  {
		    /* no CDS in exon.                                         */
		    start_rna_top = y1 ;
		    start_rna_bot = y2 ;
		    start_rna_offtop = off_top, start_rna_offbot = off_bottom ;
		  }
		else if (y1 >= CDS_top && y2 <= CDS_bottom)
		  {
		    /* exon is all CDS.                                        */
		    cds_top = y1 ;
		    cds_bot = y2 ;
		    cds_offtop = off_top, cds_offbot = off_bottom ;
		  }
		else if (y1 < CDS_top && y2 <= CDS_bottom)
		  {
		    /* exon contains start of CDS.                             */
		    start_rna_top = y1 ;
		    start_rna_bot = CDS_top ;
		    cds_top = CDS_top ;
		    cds_bot = y2 ;
		    start_rna_offtop = off_top, start_rna_offbot = FALSE ;
		    cds_offtop = FALSE, cds_offbot = off_bottom ;
		  }
		else if (y1 < CDS_top && y2 > CDS_bottom)
		  {
		    /* exon contains start of CDS and, pathologically, end of  */
		    /* CDS as well.                                            */
		    start_rna_top = y1 ;
		    start_rna_bot = CDS_top ;
		    cds_top = CDS_top ;
		    cds_bot = CDS_bottom ;
		    end_rna_top = CDS_bottom ;
		    end_rna_bot = y2 ;
		    start_rna_offtop = off_top, start_rna_offbot = FALSE ;
		    cds_offtop = FALSE, cds_offbot = FALSE ;
		    end_rna_offtop = FALSE, end_rna_offbot = off_bottom ;
		  }
		else if (y1 >= CDS_top && y2 > CDS_bottom)
		  {
		    /* exon contains end of CDS.                               */
		    cds_top = y1 ;
		    cds_bot = CDS_bottom ;
		    end_rna_top = CDS_bottom ;
		    end_rna_bot = y2 ;
		    cds_offtop = off_top, cds_offbot = FALSE ;
		    end_rna_offtop = FALSE, end_rna_offbot = off_bottom ;
		  }
		else
		  messcrash("Internal error in calculating position of CDS in EXON."
			    "\n CDS coords: %f --> %f,"
			    "\nExon coords: %f --> %f",
			    CDS_top, CDS_bottom, y1, y2) ; 

		/* If we made coords negative for above tests, make them +ve.  */
		if (CDS_top < 0)
		  {
		    CDS_top = -CDS_top, CDS_bottom = -CDS_bottom, y1 = -y1, y2 = -y2 ;
		  
		    start_rna_top = -start_rna_top, start_rna_bot = -start_rna_bot,
		      cds_top = -cds_top, cds_bot = -cds_bot,
		      end_rna_top = -end_rna_top, end_rna_bot = -end_rna_bot ;
		  }


		if (fMapDebug)
		  {
		    printf("%s -  seg x1: %d -> %f, seg x2: %d -> %f\n",
			   nameWithClassDecorate(seg->parent), seg->x1, y1, seg->x2, y2) ;
		    printf("%s -  rna_start: %f --> %f, CDS: %f --> %f, rna_end: %f --> %f\n",
			   nameWithClassDecorate(seg->parent),
			   start_rna_top, start_rna_bot, cds_top, cds_bot, end_rna_top, end_rna_bot) ;
		  }


		/* Now draw any boxes that need to be drawn.                   */
		/* NOTE how all boxes drawn are associated with the same exon  */
		/* seg, _not_ the CDS seg, so we pick up the DNA, menus etc.   */
		/* Draw CDS last so it overlays the exon, looks better.        */
		/*                                                           */
		/* Actually this needs fixing, users would prefer to get to  */
		/* the CDS stuff when they click on the cds box.             */
		/*                                                           */
		if (start_rna_bot != 0)
		  fMapDrawBox(look, i, geneMenu, seg,
			      left, start_rna_top, right, start_rna_bot,
			      start_rna_offtop, start_rna_offbot,
			      bc->meth->colour, WHITE) ;
		if (end_rna_bot != 0)
		  fMapDrawBox(look, i, geneMenu, seg,
			      left, end_rna_top, right, end_rna_bot,
			      end_rna_offtop, end_rna_offbot,
			      bc->meth->colour, WHITE) ;
		if (cds_bot != 0)
		  fMapDrawBox(look, i, geneMenu, seg,
			      left, cds_top, right, cds_bot,
			      cds_offtop, cds_offbot,
			      bc->meth->CDS_colour, WHITE) ;
	      }
	    break ;
	  }

	case INTRON: case INTRON_UP:
	  {
	    float midway ;

	    sinf = arrp(look->seqInfo, seg->data.i, SEQINFO) ;
	    if (lexAliasOf(sinf->method) != bc->meth->key)
	      break ;
	    if (assFind (seq2x, assVoid(seg->parent), &xx))
	      x = assInt(xx) - 1 ;
	    else
	      {
		if (firstError) messerror ("Can't find intron offset for %s in fMapShowSequence().",
					   name(seg->parent)) ;  
		firstError = FALSE ;
		break ;
	      }

	    /* Draw the intron, line has no peak if its clipped at top/bot.  */
	    box = graphBoxStart () ;
	    array(look->boxIndex,box,int) = i ;
	    left = *offset + ((x + 0.5) * bc->width) ;
	    right = *offset + ((x + 0.9) * bc->width) ;
	    if (off_top)
	      graphLine(left, y2, right, y1) ;
	    else if (off_bottom)
	      graphLine(left, y1, right, y2) ;
	    else
	      {
		midway = 0.5 * (y1 + y2) ;
		graphLine(left, y1, right, midway) ;
		graphLine(right, midway, left, y2) ;
	      }
	    graphBoxEnd() ;

	    fMapBoxInfo (look, box, seg) ;
	    if (sinf->flags & SEQ_CONFIRMED)
	      graphBoxDraw (box, bc->meth->colour,
			    sinf->flags & SEQ_CONFIRM_UTR ? YELLOW : GREEN);
	    else
	      graphBoxDraw (box, bc->meth->colour, WHITE) ;

	    if (seg->type == INTRON)
	      graphBoxMenu (box, intronMenu) ;
	  }
	  break ;

	default:
	  break ;
	}
    }

  if (bc->bump) 
    { 
      *offset += (bc->width * bumpMax(bc->bump)) ;
      bumpDestroy (bc->bump);
    }
  else if (isData)
    *offset += bc->width ;
    
  assDestroy (seq2x) ;


  if (fMapDebug)
    printf("\nend\n") ;
  fMapDebug = FALSE ;

  return;
} /* fMapShowSequence */


/* Deal with coords that go off top/bottom of map or have been clipped
 * during mapping.
 * Returns FALSE and does not alter the X_out params if the coords are
 * not on the window at all, otherwise they get clipped to the top/bottom
 * of the window and off_top/bottom get set to indicate that top/bottom
 * have been clipped. */
static BOOL clipCoords(FeatureMap look, SEG *seg, float *y1_out, float *y2_out,
		       BOOL *off_top_out, BOOL *off_bottom_out)
{
  BOOL on_screen = TRUE ;
  int top, bot ;
  BOOL off_top, off_bottom ;

  off_top = off_bottom = FALSE ;
  top = seg->x1 ;
  bot = seg->x2 + 1 ;					    /* seg->x2+1 to cover full base */
  
  if (top > look->max || bot < look->min)
    {
      on_screen = FALSE ;
    }
  else
    {
      on_screen = TRUE ;

      if (seg->flags & SEGFLAG_CLIPPED_TOP)
	off_top = TRUE ;

      if (top < look->min)
	{
	  top = look->min ;
	  off_top = TRUE ;
	}

      if (seg->flags & SEGFLAG_CLIPPED_BOTTOM)
	off_bottom = TRUE ;

      if (bot > look->max)
	{
	  bot = look->max ;
	  off_bottom = TRUE ;
	}

      *y1_out = MAP2GRAPH(look->map, top) ;
      *y2_out = MAP2GRAPH(look->map, bot) ;
      *off_top_out = off_top ;
      *off_bottom_out = off_bottom ;
    }

  return on_screen ;
}


/* Sometimes in fmap we want to draw rectangles that are clipped off at the  */
/* top or bottom, this routine encapsulates this. I don't think its worth    */
/* putting this in the graph package because I don't want to clutter up      */
/* graph with this application specific stuff.                               */
/* This is an internal fmap routine and does not do basic checks, the caller */
/* should have done that.                                                    */
/*                                                                           */
void fMapDrawBox(FeatureMap look, int box_index, MENUOPT gene_menu[], SEG *seg,
		 float left, float top, float right, float bottom,
		 BOOL off_top, BOOL off_bottom,
		 int foreground, int background)
{
  int box ;

  box = graphBoxStart() ;
  array(look->boxIndex, box, int) = box_index ;
  if (!off_top && !off_bottom)
    {
      graphRectangle(left, top, right, bottom) ;
    }
  else
    {
      graphLine(left, top, left, bottom) ;
      graphLine(right, top, right, bottom) ;
      if (!off_bottom)
	graphLine(left, bottom, right, bottom) ;
      if (!off_top)
	graphLine(left, top, right, top) ;
    }
  graphBoxEnd() ;

  fMapBoxInfo(look, box, seg) ;

  graphBoxDraw(box, foreground, background) ;

  if (gene_menu)
    graphBoxMenu(box, gene_menu) ;

  return ;
}

/***************************************/
/***** dynamically change geneMenu *****/
static void changeGeneMenu(FeatureMap look, SEG *seg, BOOL cds_only)
{

  geneMenu[CDS_TRANS_INDEX].text = CDS_TRANS_ON ;
  geneMenu[COMPLETE_TRANS_INDEX].text = COMPLETE_TRANS_ON ;
  if (look->translateGene == seg->parent)
    /* we have that gene already selected for translation */
    {
      BOOL column_on = FALSE ;

      if ((seg->type == EXON_UP
	   && mapColSetByName(look->map, "Up Gene Translation", MAPCOL_QUERY))
	  || (seg->type == EXON
	      && mapColSetByName(look->map, "Down Gene Translation", MAPCOL_QUERY)))
	{
	  column_on = TRUE ;
	}

      if (cds_only)
	{
	  geneMenu[COMPLETE_TRANS_INDEX].text = COMPLETE_TRANS_ON ;

	  if (column_on)
	    geneMenu[CDS_TRANS_INDEX].text = CDS_TRANS_OFF ;
	  else
	    geneMenu[CDS_TRANS_INDEX].text = CDS_TRANS_ON ;
	}
      else
	{
	  geneMenu[CDS_TRANS_INDEX].text = CDS_TRANS_ON ;

	  if (column_on)
	    geneMenu[COMPLETE_TRANS_INDEX].text = COMPLETE_TRANS_OFF ;
	  else
	    geneMenu[COMPLETE_TRANS_INDEX].text = COMPLETE_TRANS_ON ;
	}
    }

  return ;
}

/* column "Confirmed introns" */
void fMapShowSoloConfirmed (LOOK genericLook, float *offset, MENU *unused)
     /* This is of type MapColDrawFunc */
{
  FeatureMap look = (FeatureMap)genericLook;
  int	i, box, x ;
  float y1, y2 ;
  SEG	*seg ;
  SEQINFO *sinf ;
  BUMP bump = bumpCreate (20, 0) ;
  BOOL isDown = (*look->map->activeColName == '-') ? FALSE : TRUE ;

  for (i = 1 ; i < arrayMax(look->segs) ; ++i)
    { seg = arrp(look->segs,i,SEG) ;
      if (seg->x1 > look->max || seg->x2 < look->min)
	continue ;
      if (!((seg->type == INTRON && isDown) ||
	    (seg->type == INTRON_UP && !isDown)))
	continue ;
      sinf = arrp(look->seqInfo, seg->data.i, SEQINFO) ;
      if (seg->parent || !(sinf->flags & SEQ_CONFIRMED))
	continue ;
      y1 = MAP2GRAPH(look->map,seg->x1) ;
      y2 = MAP2GRAPH(look->map,seg->x2+1) ;	/* to cover full base */
      if (y1 < 2 + topMargin) y1 = 2 + topMargin ;
      if (y2 < 2 + topMargin) y2 = 2 + topMargin ;
      x = 1 ; bumpItem (bump, 1, (y2-y1), &x, &y1) ;
      box = graphBoxStart() ;
      array(look->boxIndex,box,int) = i ;
      graphLine (*offset + x, y1, *offset + x + 0.5, 0.5*(y1+y2)) ;
      graphLine (*offset + x, y2, *offset + x + 0.5, 0.5*(y1+y2)) ;
      graphBoxEnd() ;
      graphBoxDraw (box, BLACK, GREEN) ;
      graphBoxMenu (box, intronMenu) ;
      fMapBoxInfo (look, box, seg) ;
    }
  *offset += bumpMax (bump) ;
  bumpDestroy (bump) ;

  return;
} /* fMapShowSoloConfirmed */


void fMapShowEmblFeatures (LOOK genericLook, float *offset, MENU *unused)
     /* This is of type MapColDrawFunc */
{
  FeatureMap look = (FeatureMap)genericLook;
  int	i, box, x ;
  float y1, y2 ;
  SEG	*seg ;
  BUMP bump = bumpCreate (20, 0) ;

  for (i = 1 ; i < arrayMax(look->segs) ; ++i)
    { seg = arrp(look->segs,i,SEG) ;
      if (seg->x1 > look->max || seg->x2 < look->min)
	continue ;
      y1 = MAP2GRAPH(look->map,seg->x1) ;
      y2 = MAP2GRAPH(look->map,seg->x2+1) ;	/* to cover full base */
      if (y1 < 2 + topMargin) y1 = 2 + topMargin ;
      if (y2 < 2 + topMargin) y2 = 2 + topMargin ;
      switch (seg->type)
	{ 
	case EMBL_FEATURE: case EMBL_FEATURE_UP:
	  x = 0 ; bumpItem (bump, 1, (y2-y1), &x, &y1) ;
	  box = graphBoxStart() ;
	  array(look->boxIndex,box,int) = i ;
	  graphRectangle (*offset + x + 0.25, y1, 
			  *offset + x + 0.75, y2) ;
	  graphBoxEnd() ;
	  fMapBoxInfo (look, box, seg) ;
	  graphBoxDraw (box, DARKRED, WHITE) ;
	  array(look->visibleIndices,arrayMax(look->visibleIndices),int) = i ;
	  break ;
	default:
	  break ;
	}
    }
  *offset += bumpMax (bump) ;
  bumpDestroy (bump) ;

  return;
} /* fMapShowEmblFeatures */

/****************************************************************/
/******************** text features *****************************/

static BOOL showVisibleTagNames = FALSE ; /* suppress tag names in "Visible" column */

static int bumpOff ;

static void bumpWrite (BUMP bump, char *text, int x, float y)
{
  bumpItem (bump, strlen(text)+1, 1, &x, &y) ;
  graphText (text, bumpOff+x, y-0.5) ;
}

static BOOL isReversed ;
static FeatureMap x1Look ;	/* set in fMapShowText */

static int x1Order (void *a, void *b)
{
  int diff ;

  if (isReversed)
    diff = arrp(x1Look->segs, *(int*)b, SEG)->x2  -
      arrp(x1Look->segs, *(int*)a, SEG)->x2  ;
  else
    diff = arrp(x1Look->segs, *(int*)a, SEG)->x1  -
      arrp(x1Look->segs, *(int*)b, SEG)->x1  ;

  if (diff > 0) 
    return 1 ;
  else if (diff < 0)
    return -1 ;
  else
    return 0 ;
}


/* i is the index of a seg */
static void addVisibleInfo (FeatureMap look, int i)
{
  KEY key = arrp(look->segs,i,SEG)->key ;
  register int j ;
  register SEG *seg = 0 ;
  static int jmin, jmax ;	/* for efficiency cache start/end of VISIBLE segs */
				/* relies on segs not being reordered during drawing */

  if (!arrayMax(look->visibleIndices))
    fMapFindSegBounds (look, VISIBLE, &jmin, &jmax) ;
    
				/* first add the item itself */
  array(look->visibleIndices,arrayMax(look->visibleIndices),int) = i ;

				/* then related VISIBLEs */
  if (jmin < jmax) seg = arrp(look->segs,jmin,SEG) ;
  for (j = jmin ; j < jmax ; ++j, ++seg)
    if (seg->parent == key)
      array(look->visibleIndices,arrayMax(look->visibleIndices),int) = j ;

  return;
} /* addVisibleInfo */


void fMapShowText (LOOK genericLook, float *offset, MENU *unused)
     /* This is of type MapColDrawFunc */
{
  FeatureMap look = (FeatureMap)genericLook;
  int	i, j, box ;
  Stack textStack = stackCreate(100) ;
  float y ;
  SEG	*seg ;
  BUMP  bump ;

  bump = bumpCreate (40, 0) ;
  bumpOff = *offset ;

  isReversed = ((look->flag & FLAG_REVERSE) > 0) ;
  x1Look = look ;
  arraySort (look->visibleIndices, x1Order) ;

  graphColor (BLACK) ;

  for (j = 0 ; j < arrayMax(look->visibleIndices) ; ++j)
    { i = arr(look->visibleIndices, j, int) ;
      seg = arrp(look->segs, i, SEG) ;
      if (isReversed)
	y = MAP2GRAPH(look->map,seg->x2) ;
      else
	y = MAP2GRAPH(look->map,seg->x1) ;
      if (y < topMargin + 2)
	y = topMargin + 2 ;
      box = 0 ;
      switch (seg->type)
	{ 
	case SEQUENCE: case SEQUENCE_UP:
	  box = graphBoxStart() ;
	  bumpWrite (bump,name(seg->key), 0, y) ;
	  graphBoxEnd() ;
	  break ;

	case HOMOL: case HOMOL_UP:
	  box = graphBoxStart() ;
	  bumpWrite (bump,name(seg->key), 4, y) ;
	  graphBoxEnd() ;
	  break ;

	case FEATURE: case FEATURE_UP:
	  box = graphBoxStart() ;
	  bumpWrite (bump, dictName (look->featDict, -seg->parent), 4, y) ;
	  graphBoxEnd() ;
	  break ;
	  
	case VISIBLE:
	  box = graphBoxStart() ;
	  stackClear (textStack) ;
	  if (showVisibleTagNames && iskey (seg->data.k))  /* mieg: prevents (NULL KEY)) */
	    { catText (textStack, name(seg->data.k)) ;
	      catText (textStack, ":") ;
	    }
	  /* there is a problem if the model says: Visible foo Text
               this is the origin of the NULL KEY in the displays */
	  if (iskey (seg->key))   /* mieg: hack to prevents (NULL KEY)) */
	    catText (textStack, name(seg->key)) ;
	  bumpWrite (bump,stackText(textStack,0),10,y) ; 

	  graphBoxEnd() ;

	  if (iskey (seg->key))
	    graphBoxInfo (box, seg->key,
			  strnew (messprintf ("%s:%s",
					      iskey (seg->data.k) ? name(seg->data.k) : "" , 
					      name(seg->key)),
				  look->segsHandle)) ;

	  break ;
	  
	case ALLELE: case ALLELE_UP:
	case EMBL_FEATURE: case EMBL_FEATURE_UP:
	  box = graphBoxStart() ;
	  stackClear (textStack) ;
	  if (iskey (seg->key)) 
	    catText (textStack, name(seg->key)) ;
	  if (seg->data.s)
	    { catText (textStack, ":") ;
	      catText (textStack, seg->data.s) ;
	    }
	  bumpWrite (bump,stackText(textStack,0),10,y) ; 
	  graphBoxEnd() ;
	  break ;

	default:
	  break ;
	}
      if (box)
	{ array(look->boxIndex,box,int) = i ;
	  fMapBoxInfo (look, box, seg) ;
	}

    }

  *offset += bumpMax(bump) ;
  bumpDestroy (bump) ;
  stackDestroy (textStack) ;

  return;
} /* fMapShowText */



/*************************************************************/
/******** external stuff: efetch, blxview, trnafold ***********/

static void fMapEfetch (int box)
{
  SEG *seg ;
  FeatureMap look = currentFeatureMap("fMapEfetch") ;

  if ((seg = BOXSEG(box)))
    externalCommand(messprintf("efetch '%s' &", name(seg->key)));

  return ;
}

static void fMapPfetch (int box)
{
  SEG *seg ;
  FeatureMap look = currentFeatureMap("fMapPfetch") ;

  if ((seg = BOXSEG(box)))
    externalCommand(messprintf("pfetch -F '%s' &", name(seg->key))); 
  /*    pfetchWindow(seg->key); revert to prev version until gtk 2.2.2 implemented */

  return ;
}

static void fMapFetch (int box)
{
  static KEY _Database = 0 ;
  SEG *seg ;
  KEY lib ;
  OBJ obj = 0, libObj = 0 ;
  char *id, *pref, *suff, *url = 0 ;
  FeatureMap look = currentFeatureMap("fMapFetch") ;

  if (!_Database)
    lexaddkey ("Database", &_Database, 0) ;

  if (!(seg = BOXSEG(box)))
    return ;

  if ((obj = bsCreate (seg->key)) &&
      bsGetKey (obj, _Database, &lib) &&
      bsGetData (obj, _bsRight, _Text, &id) &&
      (libObj = bsCreate (lib)))
    {
      if (bsGetData (libObj, _Arg1_URL_prefix, _Text, &pref))
	{
	  if (bsGetData (libObj, _Arg1_URL_suffix, _Text, &suff))
	    url = messprintf ("%s%s%s", pref, id, suff) ;
	  else
	    url = messprintf ("%s%s", pref, id) ;
	}
      else if (bsGetData (obj, _bsRight, _Text, &id) &&
	       bsGetData (libObj, _Arg2_URL_prefix, _Text, &pref))
	{
	  if (bsGetData (libObj, _Arg2_URL_suffix, _Text, &suff))
	    url = messprintf ("%s%s%s", pref, id, suff) ;
	  else
	    url = messprintf ("%s%s", pref, id) ;
	}
    }
  bsDestroy (obj) ; bsDestroy (libObj) ; /* OK if 0 */

  if (!url)
    url = messprintf ("http://www.sanger.ac.uk/cgi-bin/seq-query?%s", 
		      name(seg->key)) ;
  
  graphWebBrowser (url) ;

  return;
} /* fMapFetch */


static void  showAlignAnticipate (KEYSET ks)
{
  KEYSET ks2 ;

  if (!externalServer)
    return ;
  keySetSort (ks) ;
  keySetCompress (ks) ;
  oldSetForServer = ks ;
  externalServer (-1, 0, 0, TRUE) ; /* get these sequences */
  ks2 = queryLocalParametrized(ks,"{FOLLOW DNA} $| {FOLLOW Peptide}",0) ;
  oldSetForServer = ks2 ;
  externalServer (-1, 0, 0, TRUE) ;
  oldSetForServer = 0 ;
  keySetDestroy (ks2) ;

  return;
} /* showAlignAnticipate */

static int scoreOrder(void *a, void *b)
{
  return
    (*(float*)a) < (*(float*)b)  ?
      1 : (*(float*)a) == (*(float*)b) ? 0 : -1 ;
}



/* Call blixem to show the alignments. */
static void doShowAlign(int x1, int x2, char *opts, KEY methodKey)
{
  SEG *seg, seg_copy ;
  int nks = 0, i, min, max, first, offset;
  char *seq, *cp, *cq ;  
  MSP *msp1, *msp, *msp2, *prevmsp=0;
  KEY key ;
  HOMOLINFO *hinf ;
  unsigned int methodFlag ;
  Array a ;
  KEYSET ks = NULL, as = NULL ;
  OBJ obj = NULL ;
  FeatureMap look = currentFeatureMap("fMapShowAlign") ;
  METHOD *meth;
  int scope = prefInt("BLIXEM_SCOPE");
  int cutoff = prefInt("BLIXEM_HOMOL_MAX");
  Array scores = arrayCreate(100, int);
  PfetchParams *pfetch_params = NULL ;
  KEY _Show_in_reverse_orientation = str2tag("Show_in_reverse_orientation") ;
  char *seq_name = NULL ;
  Stack align_types = NULL ;
  char *length_tag = "Length" ;
  KEY length_key = str2tag(length_tag) ;

  
  if (!scope)
    scope = 40000 ;					    /* Should be stored in prefs perhaps. */
  
  methodFlag = (*opts == 'X') ? METHOD_BLIXEM_X : METHOD_BLIXEM_N ;

  min = x1 - scope/2 ;
  max = x2 + scope/2 ;

  if (min < 0)
    min = 0 ;
  if (max >= look->length)
    max = look->length - 1 ;

  seq = (char*) messalloc (max-min+2) ;
  cp = seq ; cq = arrp(look->dna, min, char) ;
  for (i = min ; i++ <= max ; )
    {
      *cp++ = dnaDecodeChar[(int)*cq++] ;
    }
  *cp = 0 ;

  offset = min - look->origin ;
  first = x1 - min - 30 ;
  if (first <= 0)
    first = 1 ;

  align_types = stackCreate(1000) ;
  stackTextOnly(align_types) ;
  as = keySetCreate() ;

  ks = keySetCreate() ;
  nks = 0 ;
  msp = msp1 = (MSP*)messalloc(sizeof(MSP)) ;
  for (i = 1 ; i < arrayMax(look->segs) ; ++i)
    { 
      seg = arrp(look->segs, i, SEG) ;
      
      if (seg->x1 < min || seg->x2 < min || seg->x1 > max || seg->x2 > max)
	continue ;
	
      /* Some homologies (e.g. for EST read pairs) were swopped to the opposite
       * strand for display, now we come to blixem them, we need to temporarily
       * swop them back to their original strand, hence the seg copying below. */
      if (bIndexTag(seg->key, _Show_in_reverse_orientation))
	{
	  seg_copy = *seg ;				    /* n.b. struct copy. */
	  seg = &seg_copy ;
	  SEG_FLIP(seg) ;
	}

      /* Valid frames are 1, 2 or 3 so "0" is a good initialiser. */
      if (SEG_IS_UP(seg))	/* UP */
	{
	  msp->qstart = seg->x2 - min + 1 ;
	  msp->qend = seg->x1 - min + 1 ;

	  strcpy(msp->sframe, "(-0)") ;
	}
      else			/* DOWN */
	{
	  msp->qstart = seg->x1 - min + 1 ;
	  msp->qend = seg->x2 - min + 1 ;

	  strcpy(msp->sframe, "(+0)") ;
	}

      switch (seg->type)
	{
 	case HOMOL: case HOMOL_UP:
	  hinf = arrp(look->homolInfo, seg->data.i, HOMOLINFO) ;

	  if (methodKey && lexAliasOf(hinf->method) != lexAliasOf(methodKey))
	    break ;
	  if (!(meth = methodCacheGet(look->mcache, hinf->method, "")))
	    break;
	  if (!(meth->flags & methodFlag))
	    break ;

	  /* Concatenate align column names. */
	  if (!keySetFind(as, hinf->method, NULL))
	    {
	      if (keySetMax(as) > 0)
		catText(align_types, "/") ;

	      keySetInsert(as, hinf->method) ;

	      catText(align_types, name(hinf->method)) ;
	    }


	  msp->key = seg->key ;
	  msp->sname = messalloc(strlen(name(seg->key)) + 1) ;
	  strcpy(msp->sname, name(seg->key)) ;
	  keySet(ks, nks++) = seg->key ;

	  msp->score = hinf->score + 0.5 ;		    /* n.b. float to int convert */

	  if (SEG_IS_UP(seg))
	    strcpy(msp->qframe, messprintf ("(-%d)", 1 + ((max-min+1) - msp->qstart) % 3)) ;
	  else
	    strcpy(msp->qframe, messprintf ("(+%d)", 1 + (msp->qstart-1) % 3)) ;

	  msp->sstart = hinf->x1  ;
	  msp->send = hinf->x2 ;

	  /* Try and get match sequence length, not available in all DBs. */
	  if (bIndexTag(seg->key, length_key))
	    {
	      OBJ obj ;

	      if ((obj = bsCreate(seg->key)))
		{
		  bsGetData(obj, length_key, _Int, &(msp->slength)) ;

		  bsDestroy(obj) ;
		}
	    }
  
	  /* We only report frame if we can report it reliably for both strands for which we need
	   * length, BUT DOES FRAME MEAN ANYTHING FOR MATCH ??? */
	  if (hinf->strand == FMAPSTRAND_REVERSE)
	    strcpy(msp->sframe, messprintf("(-%d)",
					   ((msp->slength)
					    ? (1 + ((msp->slength) - msp->sstart) % 3)
					    : 0))) ;
	  else
	    strcpy(msp->sframe, messprintf("(+%d)",
					   ((msp->slength)
					    ? (1 + (msp->sstart-1) % 3)
					    : 0))) ;

	  if (hinf->gaps)
            {
              int i;
              msp->gaps = arrayCopy(hinf->gaps);
              for(i = 0; i < arrayMax(msp->gaps); i++) 
                {
                  SMapMap *m = arrp(msp->gaps, i, SMapMap);
                  m->r1 -= min;
                  m->r2 -= min;
                }
            }
	  break ;

	case CODING: case CODING_UP:
	  msp->sstart = (seg->data.i + 3) / 3 ;
	  msp->send = (seg->data.i + seg->x2 - seg->x1) / 3 ;
	  msp->score = -1 ;
	  msp->id = 100;
	  msp->sname = messalloc(strlen(name(seg->parent))+2) ;
	  sprintf (msp->sname, "%sx", name(seg->parent)) ;  /* x for exon */
	  keySet(ks, nks++) = seg->parent ;
	  if (!(seg->type & 0x1))
	    sprintf (msp->qframe, "(+%d)", 
		     1 + (msp->qstart - 1 - seg->data.i)%3) ;
	  else
	    sprintf (msp->qframe, "(-%d)", 
		     1 + ((max-min+1) - msp->qstart - seg->data.i)%3) ;
	  break ;

	case INTRON: case INTRON_UP:
	  {
	    /* Must check to see if introns are from a transcript with a CDS, otherwise
	     * in databases like worm which use duplicate trancripts to show the CDS
	     * and the transcript separately, you get two lots of identical intron records. */
	    if (bIndexTag(seg->parent, _CDS))
	      {
		msp->sstart = msp->send = msp->id = 0 ;
		msp->score = -2 ;
		msp->id = 100;
		msp->sname = messalloc(strlen(name(seg->parent)) + 2) ;
		sprintf(msp->sname, "%si", name(seg->parent)) ;   /* i for intron */
		keySet(ks, nks++) = seg->parent ;

		if (!(seg->type & 0x1))
		  strcpy (msp->qframe, "(+1)") ;
		else
		  strcpy (msp->qframe, "(-1)") ;
	      }
	    break ;
	  }
	default:
	  break ;
	}

      if (msp->score)		/* filled it */
	{
	  msp->next = (MSP*) messalloc (sizeof(MSP)) ;
          prevmsp = msp ;
	  msp = msp->next ;
	}
    }
				/* Free last MSP (empty) */
  if (prevmsp)
    {
      messfree (prevmsp->next) ;
      prevmsp->next = 0;
    }
  else /* here of none found. */
    { 
      messfree(msp1);
      msp1 = 0;
    }

  if (externalServer)
    showAlignAnticipate (ks) ;
  keySetDestroy (ks) ;


  /* Get any raw sequence that is stored in acedb for any of the matches. */
  if (*opts == 'X')		/* Protein */
    {
      for (msp = msp1 ; msp ; msp = msp->next)
	{
	  if (msp->qstart < 1)
	    {
	      msp->sstart += (3 - msp->qstart) / 3 ;
	      msp->qstart += 3 * ((3 - msp->qstart) / 3) ;
	    }
	  if (msp->qend > max-min+1)
	    {
	      msp->send -= (msp->qend + 2 - (max-min)) / 3 ;
	      msp->qend -= 3 * ((msp->qend + 2 - (max-min)) / 3) ;
	    }

	  /* Fetch sequences, desc from ACEDB */
	  if (!msp->sseq && lexword2key (msp->sname, &key, _VProtein))
	    {
	      if ((a = peptideGet(key)))
		{
		  pepDecodeArray (a) ;
		  msp->sseq = arrp(a, 0, char) ;
		  a->base = 0 ; arrayDestroy(a) ;
		}
	      else
		msp->sseq = seq ;	/* empty sseq marker */

	      if ((obj = bsCreate (key)))
		{
		  if (bsGetKey (obj, _Title, &key))
		    msp->desc = strnew (name(key), 0) ;
		  bsDestroy (obj) ;
		}

	      /* Avoid duplication */
	      for (msp2 = msp->next; msp2 ; msp2 = msp2->next)
		if (!strcmp(msp->sname, msp2->sname)) 
		  {
		    msp2->sseq = msp->sseq ;
		    msp2->desc = msp->desc ;
		  }
	    }
	}
    }
  else				/* DNA */
    {
      for (msp = msp1 ; msp ; msp = msp->next)
	{
	  if (msp->qstart < 1)
	    { msp->sstart += 1 - msp->qstart ;
	      msp->qstart = 1 ;
	    }

	  if (msp->qend > max-min+1)
	    { msp->send -= max-min+1 - msp->qend ;
	      msp->qend = max-min+1 ;
	    }

	  /* Fetch sequences, desc from ACEDB */
	  if (!msp->sseq && lexword2key(msp->sname, &key, _VSequence))
	    {
	      if ((a = dnaGet(key)))
		{
		  dnaDecodeArray (a) ;
		  msp->sseq = arrp(a, 0, char) ;
		  a->base = 0 ; arrayDestroy(a) ;
		}
	      else
		msp->sseq = seq ;	/* empty sseq marker */

	      if ((obj = bsCreate (key)))
		{
		  if (bsGetKey (obj, _Title, &key))
		    msp->desc = strnew (name(key), 0) ;
		  bsDestroy (obj) ;
		}

	      /* Avoid duplication */
	      for (msp2 = msp->next; msp2 ; msp2 = msp2->next)
		if (!strcmp(msp->sname, msp2->sname)) 
		  { msp2->sseq = msp->sseq ;
		    msp2->desc = msp->desc ;
		  }
	    }
	}
    }


  /* remove empty sequence markers and note the scores for pruning excess */
  /* sequence-free msps */
  for (msp = msp1 ; msp ; msp = msp->next)
    if (msp->sseq == seq)
      {
	msp->sseq = 0 ;
	array(scores, arrayMax(scores), int) = msp->score;
      }
  

  /* cut out of the chain and free any msps whose score is below
   * the cutoff: fun with linked-lists. */
  if (cutoff && (cutoff < arrayMax(scores)))
    {
      int chopped = 0, total = 0;
      int breakpoint;
      ACEIN score_in;
      STORE_HANDLE h = handleCreate();
      
      arraySort(scores, scoreOrder);
      breakpoint = arr(scores, cutoff, int);
      
      for (msp = msp1; msp; msp = msp->next)
	if (!msp->sseq && msp->id != 100)
	  {
	    total++;
	    if (msp->score < breakpoint)
	      chopped++;
	  }
      
      if ((score_in = messPrompt(hprintf(h, 
					"%d of %d homologies without sequence "
					"will be ommited: "
					"if desired edit cutoff score below, "
					"or Cancel to see all homologies.",
					chopped, total),
				 hprintf(h,"%d", breakpoint),
				 "i", h)))
	{
	  aceInInt(score_in, &breakpoint);
	  msp = msp1;
	  prevmsp = 0;
	  while (msp) 
	    {
	      if (!msp->sseq && msp->id != 100 && msp->score < breakpoint)
		{
		  if (msp->sname)
		    messfree(msp->sname);
		  if (msp->gaps)
		    messfree(msp->gaps);
		  if (prevmsp == 0) /* first in chain */
		    { 
		      msp1 = msp->next;
		      messfree(msp);
		      msp = msp1;
		    }
		  else
		    {
		      prevmsp->next = msp->next;
		      messfree(msp);
		      msp = prevmsp->next;
		    }
		}
	      else
		{
		  prevmsp = msp;
		  msp = msp->next;
		}
	    };
	  
	}
      
      handleDestroy(h);
    }
  
  arrayDestroy(scores);

  seq_name = g_strdup(name(look->seqKey)) ;
  
  /* Get blixem params from user preferences.                                */
  pfetch_params = NULL ;
  blixemGetPfetchParams(0, &pfetch_params) ;

  /* Blixem can either be called synchronously as part of this program or    */
  /* asychronously as a separate external program.                           */
  if (blixemIsExternal())
    {
      BOOL extended_format = TRUE ;			    /* Use extended exblx or seqbl format ? */

      blixemCallExternal(seq, seq_name, first, offset, msp1,
			 extended_format, opts,
			 pfetch_params, popText(align_types)) ;
    }
  else
    {
      blxview(seq, seq_name, first, offset, msp1, opts, pfetch_params, popText(align_types), FALSE) ;
			/* FALSE means it's not an external call */
    }

  stackDestroy(align_types) ;
  keySetDestroy(as) ;

  messfree(pfetch_params) ;

  g_free(seq_name) ;

  return ;
} /* doShowAlign */


/**************************************************************/

static void fMapShowPepAlign (int box)
{
  SEG *seg ;
  BOOL up ;
  FeatureMap look = currentFeatureMap("fMapShowPepAlign") ;

  if (!(seg = BOXSEG(box)))
    return ;

  if (seg->type == HOMOL_UP)
    up = TRUE ;
  else
    up = FALSE ;

  /*
    int x1, x2 ;
    
    if (look->flag & FLAG_REVERSE)  reverse back temporarily * /
    { x1 = look->length + 1 - seg->x2 ;
      x2 = look->length + 1 - seg->x1 ;
      fMapRC (look) ;
      look->origin = look->length + 1 - look->origin ;
      doShowAlign (x1, x2, up ? "X+BR" : "X-BR" , 0) ;
      fMapRC (look) ;
      look->origin = look->length + 1 - look->origin ;
    }
  else */


  doShowAlign(seg->x1, seg->x2, up ? "X-BR" : "X+BR", 0) ;


  return ;
}

static void fMapShowPepAlignThis (int box)
{
  SEG *seg ;
  HOMOLINFO *hinf ;
  BOOL up ;
  FeatureMap look = currentFeatureMap("fMapShowPepAlignThis") ;

  if (!(seg = BOXSEG(box)))
    return ;

  hinf = arrp(look->homolInfo, seg->data.i, HOMOLINFO) ;

  if (seg->type == HOMOL_UP)
    up = TRUE ;
  else
    up = FALSE ;

  /*
    int x1, x2 ;

    if (look->flag & FLAG_REVERSE) // reverse back temporarily * /
    { x1 = look->length + 1 - seg->x2 ;
      x2 = look->length + 1 - seg->x1 ;
      fMapRC (look) ;
      look->origin = look->length + 1 - look->origin ;
      doShowAlign (x1, x2, up ? "X+BR" : "X-BR", hinf->method) ;
      fMapRC (look) ;
      look->origin = look->length + 1 - look->origin ;
    }
  else */
    doShowAlign (seg->x1, seg->x2, up ? "X-BR" : "X+BR", hinf->method) ;
}

/* SHOULD WE DO UP/DOWN STUFF HERE......LIKE FOR PROTEINS ABOVE ????
 * If not, WHY NOT ? Oh for just a few comments in the code, life
 * would be SO MUCH SIMPLER...sigh.... */
static void fMapShowDNAAlign (int box)
{
  SEG *seg ;
  FeatureMap look = currentFeatureMap("fMapShowDNAAlign") ;

  if (!(seg = BOXSEG(box)))
    return ; 

  doShowAlign (seg->x1, seg->x2, "N+BR", 0) ;

  return;
} /* fMapShowDNAAlign */


static void fMapShowDNAAlignThis (int box)
{
  SEG *seg ;
  HOMOLINFO *hinf ;
  FeatureMap look = currentFeatureMap("fMapShowDNAAlignThis") ;

  if (!(seg = BOXSEG(box)))
    return ; 

  hinf = arrp(look->homolInfo, seg->data.i, HOMOLINFO) ;
  doShowAlign (seg->x1, seg->x2, "N+BR", hinf->method) ;

  return;
} /* fMapShowDNAAlignThis */


static void fMapExpandAlign (int box)
{
  SEG *seg ; 
  int i, j , xMin, xMax ;
  KEYSET ks ;
  SegType type ;
  FeatureMap look = currentFeatureMap("fMapExpandAlign") ;

  if (!(seg = BOXSEG(box)))
    return ; 
  type = seg->type | 0x1 ;

  ks = keySetCreate () ;
  xMin = look->zoneMin ; xMax = look->zoneMax ;
  for (i = 0, j = 0 ; i < arrayMax(look->segs) ; ++i)
    { 
      seg = arrp(look->segs, i, SEG) ;
      if (seg->x1 > xMax || seg->x2 < xMin)
	continue ;
      if ((seg->type | 0x1) != type)
	continue ;
      keySet(ks, j++) = seg->key ;
    }
  
  keySetSort (ks) ;
  keySetCompress (ks) ;
  if (keySetMax(ks))
    forestDisplayKeySet (0, ks, FALSE) ; /* will destroy ks */
  else
    keySetDestroy (ks) ;

  return;
} /* fMapExpandAlign */

/***************************************************************/

/* fMaptRNAfold uses nip to fold tRNA.  Currently doesn't combine exons - fix!
*/
static void fMaptRNAfold (int box)
{
  SEG *myseg;
  int i, min, max;
  char *seq, *cp, *cq ;  
  FeatureMap look = currentFeatureMap("fMapRNAfold") ;

  if (!(myseg = BOXSEG(box)))
    return ;

  min = myseg->x1;
  max = myseg->x2;

  seq = (char*) messalloc (max - min +2);
  for (i = min, cp = seq, cq = arrp(look->dna, min, char) ; i++ <= max ; )
    *cp++ = dnaDecodeChar[(int)*cq++] ;
  *cp = 0 ;

  externalPipeDisplay ("RNA fold", callScriptPipe ("tRNAfold", seq), 0) ;

  messfree (seq) ;

  return;
} /* fMaptRNAfold */


/******************** end of external section ***************/

void fMapRemoveSelfHomol (FeatureMap look)
{
  int i, max ;
  SEG *seg, *cds, *cdsMin, *cdsMax ;
  char *cp ;

  fMapFindSegBounds (look, CDS, &i, &max) ;
  cdsMin = arrp(look->segs, i, SEG) ;
  cdsMax = arrp(look->segs, max-1, SEG) + 1 ; /* -1+1 for arrayCheck */
  
  fMapFindSegBounds (look, HOMOL, &i, &max) ;
  for ( ; i < max && i < arrayMax(look->segs) ; ++i) 
    /* arrayMax(look->segs) can change */
    { seg = arrp(look->segs, i, SEG) ;
      for (cp = name(seg->key) ; *cp && *cp != ':' ; ++cp) ;
      if (*cp) 
	++cp ;
      else
	cp = name(seg->key) ;
      for (cds = cdsMin ; cds < cdsMax && cds->x1 < seg->x2 ; ++cds)
	if (cds->x1 <= seg->x1 && cds->x2 >= seg->x2 &&
	    !strcmp (cp, name(cds->parent)))
	  { *seg = array(look->segs,arrayMax(look->segs)-1,SEG) ;
	    --arrayMax(look->segs) ;
	    break ;
	  }
    }

  fMapFindSegBounds (look, CDS_UP, &i, &max) ;
  cdsMin = arrp(look->segs, i, SEG) ;
  cdsMax = arrp(look->segs, max-1, SEG) + 1 ; /* -1+1 for arrayCheck */
  
  fMapFindSegBounds (look, HOMOL_UP, &i, &max) ;
  for ( ; i < max && i < arrayMax(look->segs) ; ++i) 
    /* arrayMax(look->segs) can change */
    { seg = arrp(look->segs, i, SEG) ;
      for (cp = name(seg->key) ; *cp && *cp != ':' ; ++cp) ;
      if (*cp) 
	++cp ;
      else
	cp = name(seg->key) ;
      for (cds = cdsMin ; cds < cdsMax && cds->x1 < seg->x2 ; ++cds)
	if (cds->x1 <= seg->x1 && cds->x2 >= seg->x2 &&
	    !strcmp (cp, name(cds->parent)))
	  { *seg = array(look->segs,arrayMax(look->segs)-1,SEG) ;
	    --arrayMax(look->segs) ;
	    break ;
	  }
    }

  return;
} /* fMapRemoveSelfHomol */

/****************************************************/

/* This is of type MapColDrawFunc */
void fMapShowHomol (LOOK genericLook, float *offset, MENU *menu) 
{ 
  FeatureMap look = (FeatureMap)genericLook;
  int i, box ;
  SEG *seg ;
  int frame = look->min % 3 ;
  HOMOLINFO *hinf ;
  BoxCol *bc ;


  bc = bcFromName (look, look->map->activeColName, menu) ;

  if (!bcTestMag(bc, look->map->mag)) 
    return ;

  bc->offset = *offset ;


  for (i = 0 ; i < arrayMax(look->segs) ; ++i)
    {
      seg = arrp(look->segs, i, SEG) ;

      if (seg->x1 > look->max || seg->x2 < look->min)
	continue ;

      if (seg->type == HOMOL_GAP || seg->type == HOMOL_GAP_UP)
	{
	  if (!bc->cluster)
	    continue ;
	}

      if (seg->type == HOMOL || seg->type == HOMOL_GAP)
	{
	  if (!bc->isDown)
	    continue ;
	}
      else if (seg->type == HOMOL_UP || seg->type == HOMOL_GAP_UP)
	{ 
	  if (bc->isDown && (bc->meth->flags & METHOD_STRAND_SENSITIVE))
	    continue ;
	}
      else
	continue ;

      if (bit(look->homolBury,seg->data.i)) /* bury */
	continue ;

      hinf = arrp(look->homolInfo, seg->data.i, HOMOLINFO) ;
      if (lexAliasOf(hinf->method) != bc->meth->key)
	continue ;

      if (bc->isFrame && (seg->x1 % 3 != frame)) 
	  continue ;

      if ((bc->meth->flags & METHOD_DISPLAY_GAPS) && hinf->gaps)
	box = bcDrawBox(bc, seg, hinf->score, hinf->gaps) ;
      else
	box = bcDrawBox(bc, seg, hinf->score, NULL) ;

      if (box)
	{ 
	  array(bc->look->boxIndex, box, int) = i ;

	  if (bc->meth->flags & METHOD_BLIXEM_X)
	    graphBoxMenu (box, pepHomolMenu) ;
	  else if (bc->meth->flags & METHOD_BLIXEM_N)
	    graphBoxMenu (box, dnaHomolMenu) ;

	  if (bc->meth->isShowText)
	    addVisibleInfo (look, i) ;
	}
    }

  if (bc->bump)
    bumpDestroy (bc->bump);

  *offset += bc->fmax ;

  return;
} /* fMapShowHomol */

/****************************************************/

/* This is of type MapColDrawFunc, it draws boxes for:
 * 
 *   Lightweight Features, i.e. features that are represented by the 
 *                    Feature tag set which can be embedded in any object.
 * 
 *  */
void fMapShowFeature (LOOK genericLook, float *offset, MENU *menu) 
{ 
  FeatureMap look = (FeatureMap)genericLook;
  int i, box ;
  SEG *seg ;
  int frame = look->min % 3 ;
  BoxCol *bc ;

  bc = bcFromName (look, look->map->activeColName, menu) ;

  if (!bcTestMag (bc, look->map->mag)) 
    return ;

  bc->offset = *offset ;

  for (i = 0 ; i < arrayMax(look->segs) ; ++i)
    {
      seg = arrp(look->segs, i, SEG) ;

      if (seg->x1 > look->max || seg->x2 < look->min)
	continue ;

      if (!((bc->isDown
	     && (seg->type == FEATURE
		 || (seg->type == FEATURE_UP
		     && !(bc->meth->flags & METHOD_STRAND_SENSITIVE))))
	    || (!bc->isDown
		&& seg->type == FEATURE_UP
		&& bc->meth->flags & METHOD_STRAND_SENSITIVE)))
	continue ;

      if (lexAliasOf(seg->key) != bc->meth->key)
	continue ;

      if (bc->isFrame && (seg->x1 % 3 != frame)) 
	continue ;

      box = bcDrawBox (bc, seg, seg->data.f, NULL) ;
      if (box)
	{
	  array(bc->look->boxIndex, box, int) = i ;
	  if (bc->meth->isShowText && seg->parent) /* -seg->parent = featDict index */
	    addVisibleInfo (look, i) ;
	}
    }
  
  if (bc->bump)
    bumpDestroy (bc->bump);

  *offset += bc->fmax ;

  return;
} /* fMapShowFeature */

/********************************************/



/****************************************************/

/* This is of type MapColDrawFunc, it draws boxes for:
 * 
 *   Feature Objects, i.e. features that are represented by whole objects,
 *                    these could be alleles or anything.
 * 
 *  */
void fMapShowFeatureObj(LOOK genericLook, float *offset, MENU *menu) 
{ 
  FeatureMap look = (FeatureMap)genericLook;
  int i, box ;
  SEG *seg ;
  int frame = look->min % 3 ;
  BoxCol *bc ;
  FeatureInfo *feat ;

  bc = bcFromName(look, look->map->activeColName, menu) ;

  if (!bcTestMag (bc, look->map->mag)) 
    return ;

  bc->offset = *offset ;

  for (i = 0 ; i < arrayMax(look->segs) ; ++i)
    {
      seg = arrp(look->segs, i, SEG) ;

      if (seg->x1 > look->max || seg->x2 < look->min)
	continue ;

      if (!((bc->isDown
	     && (seg->type == FEATURE_OBJ
		 || (seg->type == FEATURE_OBJ_UP
		     && !(bc->meth->flags & METHOD_STRAND_SENSITIVE))))
	    || (!bc->isDown
		&& seg->type == FEATURE_OBJ_UP
		&& bc->meth->flags & METHOD_STRAND_SENSITIVE)))
	continue ;

      feat = arrp(look->feature_info, seg->data.i, FeatureInfo) ;
      if (lexAliasOf(feat->method) != bc->meth->key)
	continue ;

      if (bc->isFrame && (seg->x1 % 3 != frame)) 
	continue ;

      /* Either we do the point like drawing or box like drawing, this code
       * is experimental and quite a lot of tidying up needs doing. */
      if (bIndexTag(feat->method, str2tag("Point")))
	{
	  AlleleDrawType allele_style = ALLELE_DRAW_DELETION ;

	  if (bIndexTag(feat->method, str2tag("Substitution")))
	    allele_style = ALLELE_DRAW_REPLACMENT ;
	  else if (bIndexTag(feat->method, str2tag("Insertion")))
	    allele_style = ALLELE_DRAW_TRANSPOSON ;

	  box = 0 ;
	  drawAlleleStyle(look, offset, seg, allele_style, &box) ;
	}
      else
	{
	  box = bcDrawBox (bc, seg, 0.5, NULL) ;
	}

      if (box)
	{
	  array(bc->look->boxIndex, box, int) = i ;
	  if (bc->meth->isShowText && seg->parent) /* -seg->parent = featDict index */
	    addVisibleInfo (look, i) ;
	}
    }
  
  if (bc->bump)
    bumpDestroy (bc->bump);

  *offset += bc->fmax ;

  return;
}

/********************************************/



#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
/* This is of type MapColDrawFunc and displays allele features which have
 * different shapes according to whether they are insertions/deletions etc.
 * The major feature of the drawing is that at low magnification the allele
 * is always visible as one of these symbols and as you zoom in the symbol
 * changes to be a bracket that indicates the extent of the allele. */
void fMapShowAlleles(LOOK genericLook, float *offset, MENU *unused)
{
  static char DNAchars[] = "AGCTagct" ;
  static Array xy = 0 ;
  FeatureMap look = (FeatureMap)genericLook;
  char *cp ;
  int i, box ;
  SEG *seg ;
  BOOL isTransposon, isDeletion ;
  BOOL isDraw = FALSE ;
  
  if (!xy)
    {
      xy = arrayCreate (6, float) ;
      arrayMax(xy) = 6 ;
    }
	  
  for (i = 0 ; i < arrayMax(look->segs) ; ++i)
    {
      seg = arrp(look->segs, i, SEG) ;
      if (seg->type > ALLELE_UP)
	break ;
      if ((seg->type != ALLELE && seg->type != ALLELE_UP) ||
	  (seg->parent != seg->key)) /* no explicit coords given */
	continue ;

      if (look->map->mag * (seg->x2 - seg->x1) < 1) /* point symbol */
	{
	  float y = MAP2GRAPH(look->map, 0.5*(seg->x1+seg->x2)) ;

	  if (y > mapGraphHeight || y < topMargin)
	    continue ;
	  
	  isDeletion = isTransposon = FALSE ;
	  if ((cp = seg->data.s))
	    {
	      if (*cp == '-')
		isDeletion = TRUE ;
	      else
		{
		  while (*cp)
		    if (!strchr (DNAchars, *cp++))
		      isTransposon = TRUE ;
		}
	    }

	  box = graphBoxStart() ;
	  if (isDeletion)
	    {
	      float y1 = MAP2GRAPH(look->map, seg->x1) ;
	      float y2 = MAP2GRAPH(look->map, seg->x2) ;
	      graphLine (*offset, y1, *offset+1.0, y1) ;
	      graphLine (*offset+1.0, y1, *offset+2.0, y-0.7) ;
	      graphLine (*offset, y2, *offset+1.0, y2) ;
	      graphLine (*offset+1.0, y2, *offset+2.0, y+0.7) ;
	      graphLine (*offset+2.0, y-0.7, *offset+2.0, y+0.7) ;
	    }
	  else if (isTransposon)
	    {
	      arr(xy, 0, float) = *offset ; arr(xy, 1, float) = y ;
	      arr(xy, 2, float) = *offset + 1.0 ; arr(xy, 3, float) = y - 0.7 ;
	      arr(xy, 4, float) = *offset + 1.0 ; arr(xy, 5, float) = y + 0.7 ;
	      graphPolygon (xy) ;
	      graphLine (*offset+1.5, y-0.5, *offset+1.5, y+0.5) ;
	      if (seg->type == ALLELE_UP)
		{
		  graphLine (*offset+1.1, y, *offset+1.5, y-0.5) ;
		  graphLine (*offset+1.9, y, *offset+1.5, y-0.5) ;
		}
	      else
		{
		  graphLine (*offset+1.1, y, *offset+1.5, y+0.5) ;
		  graphLine (*offset+1.9, y, *offset+1.5, y+0.5) ;
		}
	    }
	  else
	    {
	      graphLine (*offset, y, *offset+0.5, y+0.4) ;
	      graphLine (*offset, y, *offset+0.5, y-0.4) ;
	      graphLine (*offset, y, *offset+1.1, y+0.2) ;
	      graphLine (*offset+1.1, y+0.2, *offset+0.9, y-0.2) ;
	      graphLine (*offset+2, y, *offset+0.9, y-0.2) ;
	    }
	}
      else			/* bracket to show region */
	{
	  float y1 = MAP2GRAPH(look->map, seg->x1) ;
	  float y2 = MAP2GRAPH(look->map, seg->x2) ;
	  if (y1 > mapGraphHeight || y2 < topMargin)
	    continue ;

	  box = graphBoxStart() ;
	  if (y1 < topMargin) 
	    graphLine (*offset+2.0, topMargin, *offset+2.0, y2) ;
	  else
	    {
	      graphLine (*offset+1.0, y1, *offset+2.0, y1) ;
	      graphLine (*offset+2.0, y1, *offset+2.0, y2) ;
	    }
	  graphLine (*offset+1.0, y2, *offset+2.0, y2) ;
	}
      graphBoxEnd() ;
      graphBoxDraw (box, BLACK, TRANSPARENT) ;

      isDraw = TRUE ;
      array(look->boxIndex, box, int) = i ;
      fMapBoxInfo (look, box, seg) ;
      array(look->visibleIndices,arrayMax(look->visibleIndices),int) = i ;
    }

  if (isDraw)
    *offset += 2.0 ;

  return;
}
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */


/* This is of type MapColDrawFunc and displays allele features which have
 * different shapes according to whether they are insertions/deletions etc.
 * The major feature of the drawing is that at low magnification the allele
 * is always visible as one of these symbols and as you zoom in the symbol
 * changes to be a bracket that indicates the extent of the allele. */
void fMapShowAlleles(LOOK genericLook, float *offset, MENU *unused)
{
  static char DNAchars[] = "AGCTagct" ;
  FeatureMap look = (FeatureMap)genericLook ;
  char *cp ;
  int i ;
  SEG *seg ;
  BOOL isDraw = FALSE ;
  int box ;
  
  for (i = 0 ; i < arrayMax(look->segs) ; ++i)
    {
      AlleleDrawType allele_draw ;

      seg = arrp(look->segs, i, SEG) ;

      if (seg->type > ALLELE_UP)
	break ;

      if ((seg->type != ALLELE && seg->type != ALLELE_UP) ||
	  (seg->parent != seg->key)) /* no explicit coords given */
	continue ;

      /* Set the type of allele....this surely is hokey models/code stuff..... */
      allele_draw = ALLELE_DRAW_REPLACMENT ;
      if ((cp = seg->data.s))
	{
	  if (*cp == '-')
	    allele_draw = ALLELE_DRAW_DELETION ;
	  else
	    {
	      while (*cp)
		if (!strchr(DNAchars, *cp++))
		  allele_draw = ALLELE_DRAW_TRANSPOSON ;
	    }
	}


      if ((isDraw = drawAlleleStyle(look, offset, seg, allele_draw, &box)))
	{
	  array(look->boxIndex, box, int) = i ;
	  fMapBoxInfo (look, box, seg) ;
	  array(look->visibleIndices,arrayMax(look->visibleIndices),int) = i ;
	}
    }

  if (isDraw)
    *offset += 2.0 ;

  return ;
}


/* Draw in the allele style....different symbols for different allele features.
 * The major feature of the drawing is that at low magnification the allele
 * is always visible as one of these symbols and as you zoom in the symbol
 * changes to be a bracket that indicates the extent of the allele. */
static BOOL drawAlleleStyle(FeatureMap look, float *offset,
			    SEG *seg, AlleleDrawType allele_draw, int *box_out)
{
  BOOL box_drawn = FALSE ;
  static Array xy = 0 ;
  int box ;
  
  if (!xy)
    {
      xy = arrayCreate (6, float) ;
      arrayMax(xy) = 6 ;
    }
	  
  if (look->map->mag * (seg->x2 - seg->x1) < 1)
    {
      /* Less than one unit so draw as a point symbol */
      float y = MAP2GRAPH(look->map, 0.5*(seg->x1+seg->x2)) ;

      if (y > mapGraphHeight || y < topMargin)		    /* UGH..... */
	return FALSE ;

      box = graphBoxStart() ;
      if (allele_draw == ALLELE_DRAW_DELETION)
	{
	  float y1 = MAP2GRAPH(look->map, seg->x1) ;
	  float y2 = MAP2GRAPH(look->map, seg->x2) ;
	  graphLine (*offset, y1, *offset+1.0, y1) ;
	  graphLine (*offset+1.0, y1, *offset+2.0, y-0.7) ;
	  graphLine (*offset, y2, *offset+1.0, y2) ;
	  graphLine (*offset+1.0, y2, *offset+2.0, y+0.7) ;
	  graphLine (*offset+2.0, y-0.7, *offset+2.0, y+0.7) ;
	}
      else if (allele_draw == ALLELE_DRAW_TRANSPOSON)
	{
	  arr(xy, 0, float) = *offset ; arr(xy, 1, float) = y ;
	  arr(xy, 2, float) = *offset + 1.0 ; arr(xy, 3, float) = y - 0.7 ;
	  arr(xy, 4, float) = *offset + 1.0 ; arr(xy, 5, float) = y + 0.7 ;
	  graphPolygon (xy) ;
	  graphLine (*offset+1.5, y-0.5, *offset+1.5, y+0.5) ;
	  if (seg->type == ALLELE_UP)
	    {
	      graphLine (*offset+1.1, y, *offset+1.5, y-0.5) ;
	      graphLine (*offset+1.9, y, *offset+1.5, y-0.5) ;
	    }
	  else
	    {
	      graphLine (*offset+1.1, y, *offset+1.5, y+0.5) ;
	      graphLine (*offset+1.9, y, *offset+1.5, y+0.5) ;
	    }
	}
      else /* allele_draw == ALLELE_DRAW_REPLACMENT */
	{
	  graphLine (*offset, y, *offset+0.5, y+0.4) ;
	  graphLine (*offset, y, *offset+0.5, y-0.4) ;
	  graphLine (*offset, y, *offset+1.1, y+0.2) ;
	  graphLine (*offset+1.1, y+0.2, *offset+0.9, y-0.2) ;
	  graphLine (*offset+2, y, *offset+0.9, y-0.2) ;
	}
    }
  else
    {
      /* otherwise draw as a bracket to show region */
      float y1 = MAP2GRAPH(look->map, seg->x1) ;
      float y2 = MAP2GRAPH(look->map, seg->x2) ;

      if (y1 > mapGraphHeight || y2 < topMargin)
	return FALSE ;					    /* UGH..... */

      box = graphBoxStart() ;
      if (y1 < topMargin) 
	graphLine (*offset+2.0, topMargin, *offset+2.0, y2) ;
      else
	{
	  graphLine (*offset+1.0, y1, *offset+2.0, y1) ;
	  graphLine (*offset+2.0, y1, *offset+2.0, y2) ;
	}
      graphLine (*offset+1.0, y2, *offset+2.0, y2) ;
    }
  graphBoxEnd() ;
  graphBoxDraw(box, BLACK, TRANSPARENT) ;

  box_drawn = TRUE ;
  *box_out = box ;

  return box_drawn ;
}



/********************************************/

static void showSplices (FeatureMap look, SegType type, BoxCol *bc, float origin)
{
  char  *v ;
  int i, box, background ;
  SEG *seg ;
  float y=0, delta=0, x=0 ; /* delta: mieg: shows frame by altering the drawing of the arrow */
  float fac;

  fac = bc->width / (bc->meth->maxScore - bc->meth->minScore);
  
  for (i = 0 ; i < arrayMax(look->segs) ; ++i)
    { seg = arrp(look->segs, i, SEG) ;
      if (seg->x1 > look->max
	  || seg->x2 < look->min
	  || (lexAliasOf(seg->key) != bc->meth->key)
	  || seg->type != type)
	continue ;
      y = MAP2GRAPH(look->map, seg->x2) ;
      if (seg->data.f <= bc->meth->minScore) 
	x = 0 ;
      else if (seg->data.f >= bc->meth->maxScore) 
	x = bc->width ;
      else 
	x = fac * (seg->data.f - bc->meth->minScore) ;
      box = graphBoxStart() ;
      if (x > origin + 0.5 || x < origin - 0.5) 
	graphLine (bc->offset+origin, y, bc->offset+x, y) ;
      else if (x > origin)
	graphLine (bc->offset+origin-0.5, y, bc->offset+x, y) ;
      else
	graphLine (bc->offset+origin+0.5, y, bc->offset+x, y) ;
      switch (type)
	{
	case SPLICE5:
	  delta = (look->flag & FLAG_REVERSE) ? -0.5 : 0.5 ;
	  break ;
	case SPLICE3:
	  delta = (look->flag & FLAG_REVERSE) ? 0.5 : -0.5 ;
	  break ;
        default:
	  messcrash ("Bad type %d in showSplices", type) ;
	}
      graphLine (bc->offset+x, y, bc->offset+x, y+delta) ;
      graphBoxEnd() ;
      v = SEG_HASH (seg) ;
      if (assFind (look->chosen, v, 0))
	background = GREEN ;
      else if (assFind (look->antiChosen, v, 0))
	background = LIGHTGREEN ;
      else
	background = TRANSPARENT ;
      switch (seg->x2 % 3)
	{
	case 0:
	  graphBoxDraw (box, RED, background) ; break ;
	case 1:
	  graphBoxDraw (box, BLUE, background) ; break ;
	case 2:
	  graphBoxDraw (box, DARKGREEN, background) ; break ;
	}
      array(look->boxIndex, box, int) = i ;
      fMapBoxInfo (look, box, seg) ;
      graphBoxFreeMenu (box, fMapChooseMenuFunc, fMapChooseMenu) ;
    }

  return;
} /* showSplices */


void fMapShowSplices (LOOK genericLook, float *offset, MENU *menu)
     /* This is of type MapColDrawFunc */
{
  FeatureMap look = (FeatureMap)genericLook;
  float x, y1, y2, origin ;
  BoxCol *bc ;

  bc = bcFromName (look, look->map->activeColName, menu) ;
	/* NB old default diff was 5 - now using bc system it is 1 */
  if (!bcTestMag (bc, look->map->mag))
    return ;
  bc->offset = *offset ;

  y1 = MAP2GRAPH(look->map,look->min) ;
  y2 = MAP2GRAPH(look->map,look->max) ;

  graphColor(LIGHTGRAY) ;
  x = *offset + 0.5 * bc->width ;  graphLine (x, y1, x, y2) ;
  x = *offset + 0.75 * bc->width ;  graphLine (x, y1, x, y2) ;

  graphColor(DARKGRAY) ;
  if (bc->meth->minScore < 0 && 0 < bc->meth->maxScore)
    origin = bc->width *
      (-bc->meth->minScore / (bc->meth->maxScore - bc->meth->minScore)) ;
  else
    origin = 0 ;
  graphLine (*offset + origin, y1, *offset + origin, y2) ;

  graphColor (BLACK) ;
  showSplices (look, SPLICE5, bc, origin) ;
  showSplices (look, SPLICE3, bc, origin) ;

  *offset += bc->width + 1 ;

  return;
} /* fMapShowSplices */

/*************************************************/

void fMapShowCDSBoxes (LOOK genericLook, float *offset, MENU *unused)
     /* This is of type MapColDrawFunc */
{
  FeatureMap look = (FeatureMap)genericLook;
  int i ;
  SEG *seg ;
  float y1, y2 ;

  graphText ("Genes", *offset-1, topMargin+1.5) ;
  *offset += 2.5 ;
  graphLine (*offset, topMargin+2, *offset, mapGraphHeight-0.5) ;

  for (i = 1 ; i < arrayMax(look->segs) ; ++i)
    { seg = arrp(look->segs,i,SEG) ;
      if (seg->x1 > look->max || seg->x2 < look->min)
	continue ;
      y1 = MAP2GRAPH(look->map,seg->x1) ;
      y2 = MAP2GRAPH(look->map,seg->x2+1) ;	/* to cover full base */
      if (y1 < 2 + topMargin) y1 = 2 + topMargin ;
      if (y2 < 2 + topMargin) y2 = 2 + topMargin ;
      if (seg->type == CDS)
	graphFillRectangle (*offset, y1, *offset+1, y2) ;
      if (seg->type == CDS_UP)
	graphFillRectangle (*offset, y1, *offset-1, y2) ;
    }
  *offset += 2.5 ;

  return;
} /* fMapShowCDSBoxes */


void fMapShowCDSLines (LOOK genericLook, float *offset, MENU *unused)
     /* This is of type MapColDrawFunc */
{
  FeatureMap look = (FeatureMap)genericLook;
  int i ;
  SEG *seg ;
  float y ;

  for (i = 1 ; i < arrayMax(look->segs) ; ++i)
    { seg = arrp(look->segs,i,SEG) ;
      if (seg->x1 > look->max || seg->x2 < look->min)
	continue ;
      y = 0.5 * (MAP2GRAPH(look->map,seg->x1) +
		 MAP2GRAPH(look->map,seg->x2+1)) ;
      if (y < 2 + topMargin) y = 2 + topMargin ;
      if (seg->type == CDS || seg->type == CDS_UP)
	graphLine (*offset+1, y, *offset+3, y) ;
    }
  *offset += 5 ;

  return;
} /* fMapShowCDSLines */

static void generalBump (FeatureMap look, float *offset, int width, KEY tag)
{
  int i, box ;
  SEG *seg ;
  float y ;
  BUMP bump = bumpCreate (width, 0) ;

  bumpOff = *offset ;

  graphColor (BLACK) ;
  i = (look->flag & FLAG_REVERSE) ? arrayMax(look->segs)-1 : 1 ;
  while (i && i < arrayMax(look->segs))
    { seg = arrp(look->segs,i,SEG) ;
      if (seg->x1 > look->max || seg->x2 < look->min)
	goto loop ;
      y = 0.5 * (MAP2GRAPH(look->map,seg->x1) +
		 MAP2GRAPH(look->map,seg->x2+1)) ;
      if (y < 2 + topMargin) y = 2 + topMargin ;
      if (seg->type == VISIBLE && 
	  seg->data.k == tag) 
	{ box = graphBoxStart() ;
	  bumpWrite (bump, name(seg->key),0,y) ;
	  graphBoxEnd () ;
	  array(look->boxIndex, box, int) = i ;
	  fMapBoxInfo (look, box, seg) ;
	}
    loop: if (look->flag & FLAG_REVERSE) --i ; else ++i ;
    }

  *offset += bumpMax(bump) ;
  bumpDestroy (bump) ;

  return;
} /* generalBump */


void fMapShowGeneNames (LOOK genericLook, float *offset, MENU *unused)
     /* This is of type MapColDrawFunc */
{
  FeatureMap look = (FeatureMap)genericLook;

  generalBump (look, offset, 6, _Locus) ;
}

void fMapShowBriefID (LOOK genericLook, float *offset, MENU *unused)
     /* This is of type MapColDrawFunc */
{
  FeatureMap look = (FeatureMap)genericLook;

  generalBump (look, offset, 50, _Brief_identification) ;
}

/****** code for fMapShowCanonical ******/

void keyZoneAdd (Array a, KEY key, float y1, float y2, int iseg)
{ 
  int i ;
  KEYZONE *z ;

  for (i = 0 ; i < arrayMax(a) ; ++i)
    if ((z = arrp(a, i, KEYZONE))->key == key)
      return ;

  z = arrayp(a, arrayMax(a), KEYZONE) ;
  z->key = key ; z->y1 = y1 ; z->y2 = y2 ; z->iseg = iseg ;
}

int keyZoneOrder (void *va, void *vb)
{
  KEYZONE *a = (KEYZONE*) va, *b = (KEYZONE*) vb ;
  return (a->y1 + a->y2 > b->y1 + b->y2) ? 1 : -1 ;
}


/* This is of type MapColDrawFunc.
 * Column "Sequences & ends", ususally over to the left, sequences are vertical
 * white bars which are stepped relative to one another, next to them are arrows
 * to show clone left/right ends. */
void fMapShowCanonical(LOOK genericLook, float *offset, MENU *menu)
{
  FeatureMap look = (FeatureMap)genericLook;
  int i ;
  SEG *seg, *oldseg = 0 ;
  BOOL isDown ;
  int x, box ;
  float y1, y2, y ;
  BoxCol *bc;
  BUMP bump = bumpCreate (3, 0) ;
  Array a = arrayCreate (8, KEYZONE) ;
  KEY methodKey = 0;
  char *method_name ;

  /* Look to see if user has created a method in the DB for the sequence/ends column,
   * use a built in name if not. */
  method_name = look->map->activeColName ;
  if (!lexword2key(method_name, &methodKey, _VMethod))
    method_name = "FMAP_Sequences_and_ends" ;

  bc = bcFromName(look, method_name, menu) ;


  /* fw: shouldn't we call bcTextMag ? */
  bc->offset = *offset ;
  if (!bc->meth->width) 
    bc->width = 1.0 ;
  if (!bc->meth->colour)
    bc->meth->colour = BLACK ;


  /*
   * first the bars
   */

  ++*offset ;

  i = (look->flag & FLAG_REVERSE) ? arrayMax(look->segs)-1 : 1 ;
  while (i && i < arrayMax(look->segs))
    {
      seg = arrp(look->segs,i,SEG) ;

      if (seg->x1 > look->max || seg->x2 < look->min)
	goto loop1 ;

      y1 = MAP2GRAPH(look->map,seg->x1) ;
      y2 = MAP2GRAPH(look->map,seg->x2 + 1) ;		    /* "+ 1" to cover full base */

      if (y1 < 2 + topMargin)
	y1 = 2 + topMargin ;
      if (y1 > mapGraphHeight)
	y1 = mapGraphHeight ;
      if (y2 < 2 + topMargin)
	y2 = 2 + topMargin ;
      if (y2 > mapGraphHeight)
	y2 = mapGraphHeight ;

      if (seg->type == SEQUENCE || seg->type == SEQUENCE_UP)
	{
	  SEQINFO *sinf = arrp(look->seqInfo, seg->data.i, SEQINFO) ;

	  if (sinf->flags & SEQ_CANONICAL)
	    {
	      float x1, x2 ;

	      x = 0 ;
	      y = seg->x1 ;	/* bump in DNA coords so OK reversed */

	      bumpItem(bump, 1, seg->x2 - seg->x1+1, &x, &y) ;

	      x1 = *offset + (x * bc->width) ;
	      x2 = *offset + ((x + 0.75) * bc->width) ;

	      box = graphBoxStart() ;

	      if ((bc->meth->flags & METHOD_DISPLAY_GAPS) && sinf->gaps)
		{
		  /* gapped */
		  drawInterruptedRectangle(look, sinf->gaps, x1, y1, x2, y2) ;
		}
	      else
		{
		  /* no gaps */
		  graphRectangle(x1, y1, x2, y2) ;
		}

	      graphBoxEnd() ;
	      
	      array(look->boxIndex, box, int) = i;

	      fMapBoxInfo (look, box, seg) ;
	      addVisibleInfo (look, i) ;

	      if (bc->meth->isShowText)
		addVisibleInfo (look, i) ;
	      keyZoneAdd (a, seg->key, y1, y2, i) ;

	      graphBoxDraw (box, bc->meth->colour, WHITE) ;
	    }
	}

    loop1:
      if (look->flag & FLAG_REVERSE)
	--i ;
      else
	++i ;
    }
  *offset += (bc->width * bumpMax(bump)) ;

  bumpDestroy (bump) ;


  /*
   * then the ends
   */

  bump = bumpCreate (3, 0) ;

  /* determine isDown: rely on MASTER running Left->Right */
  seg = arrp(look->segs, 0, SEG) ;
  isDown = (seg->x2 > seg->x1) ;
  if (look->flag & FLAG_REVERSE)
    isDown = !isDown ;
  if (look->flag & FLAG_COMPLEMENT) /* because Clone_*_end tags not flipped by fMapRC() */
    isDown = !isDown ;

  i = (look->flag & FLAG_REVERSE) ? arrayMax(look->segs)-1 : 1 ;
  while (i && i < arrayMax(look->segs))
    {
      seg = arrp(look->segs,i,SEG) ;

      if (seg->type != CLONE_END || seg->x1 > look->max || seg->x2 < look->min)
	goto loop2 ;

      if (oldseg && seg->key == oldseg->key && seg->x1 == oldseg->x1)
	goto loop2 ;

      oldseg = seg ;
      y = MAP2GRAPH(look->map,seg->x1) ;
      box = graphBoxStart() ;
      array(look->boxIndex, box, int) = i;
      graphLine (*offset, y, *offset+1, y) ;

      if ((seg->data.k == _Clone_left_end && isDown) || (seg->data.k == _Clone_right_end && !isDown))
	{
	  graphLine (*offset+0.5, y, *offset+0.5, y+2) ;
	  graphLine (*offset, y+1, *offset+0.5, y+2) ;
	  graphLine (*offset+1, y+1, *offset+0.5, y+2) ;
	}
      else
	{
	  graphLine (*offset+0.5, y, *offset+0.5, y-2) ;
	  graphLine (*offset, y-1, *offset+0.5, y-2) ;
	  graphLine (*offset+1, y-1, *offset+0.5, y-2) ;
	}

      graphBoxEnd() ;
      fMapBoxInfo (look, box, seg) ;
      keyZoneAdd (a, seg->parent, y, y, i) ;

    loop2:
      if (look->flag & FLAG_REVERSE)
	--i ;
      else
	++i ;
    }
  *offset += bumpMax(bump) + 1 ;

  bumpDestroy (bump) ;


  /* 
   *then the text next to the clones
   */

  bump = bumpCreate (12, 0) ;
  bumpOff = *offset ;
  arraySort (a, keyZoneOrder) ;
  for (i = 0 ; i < arrayMax(a) ; ++i)
    {
      KEYZONE *z = arrp(a, i, KEYZONE) ;

      box = graphBoxStart() ;
      array(look->boxIndex,box,int) = z->iseg ;
      bumpWrite (bump, name(z->key), 0, 0.5*(z->y1 + z->y2)) ;
      graphBoxEnd() ;
      fMapBoxInfo (look, box, seg) ;
    }
  *offset += bumpMax(bump) ;

  bumpDestroy (bump) ;


  arrayDestroy(a) ;


  return ;
} /* fMapShowCanonical */

/***********************************************************************/

void fMapShowAssemblyTags (LOOK genericLook, float *offset, MENU *unused)
     /* This is of type MapColDrawFunc */
{
  FeatureMap look = (FeatureMap)genericLook;
  int i;
  SEG *seg ;
  float y1, y2 ;
  BUMP bump = bumpCreate (30, 0) ;

				/* first the bars */
  ++*offset ;
  i = (look->flag & FLAG_REVERSE) ? arrayMax(look->segs)-1 : 1 ;
  while (i && i < arrayMax(look->segs))
    { seg = arrp(look->segs,i,SEG) ;
      if (seg->x1 > look->max || seg->x2 < look->min)
	goto loop ;
      y1 = MAP2GRAPH(look->map,seg->x1) ;
      y2 = MAP2GRAPH(look->map,seg->x2+1) ;	/* to cover full base */
      if (y1 < 2 + topMargin) y1 = 2 + topMargin ;
      if (y2 < 2 + topMargin) y2 = 2 + topMargin ;
      if (seg->type == ASSEMBLY_TAG)
	{ int box = graphBoxStart();
	  int x = 0 ;
	  int color;
	  unsigned char *c = (unsigned char *)seg->data.s;
	  switch(c[0] ^ c[1] ^ c[2] ^ c[3])
	    {
	    case 'a'^'n'^'n'^'o': color = BLACK; break;
	    case 'c'^'o'^'m'^'m': color = BLUE; break;
	    case 'o'^'l'^'i'^'g': color = YELLOW; break;
	    case 'c'^'o'^'m'^'p': color = RED; break;
	    case 's'^'t'^'o'^'p': color = RED; break;
	    case 'r'^'e'^'p'^'e': color = GREEN; break;
	    case 'c'^'o'^'s'^'m': color = LIGHTGRAY; break;
	    case 'A'^'l'^'u'^' ': color = GREEN; break;
	    case 'i'^'g'^'n'^'o': color = LIGHTGRAY; break;
	    default:              color = LIGHTBLUE; break;
	    }
	  y2 -= y1 ;
	  bumpItem (bump, 1, y2, &x, &y1) ;
	  graphRectangle (*offset+x, y1, *offset+x+0.8, y1+y2) ;
	  graphBoxEnd();
	  fMapBoxInfo (look, box, seg) ;
	  graphBoxDraw(box, BLACK, color);
	  array(look->boxIndex, box, int) = i;
	}
    loop: if (look->flag & FLAG_REVERSE) --i ; else ++i ;
    }
  *offset += bumpMax (bump) ;
  bumpDestroy (bump) ;

  return;
} /* fMapShowAssemblyTags */

/*******************************************/

void fMapShowcDNAs (LOOK genericLook, float *offset, MENU *unused)
     /* This is of type MapColDrawFunc */
{
  FeatureMap look = (FeatureMap)genericLook;
  int i ;
  SEG *seg ;
  int x ;
  float y ;
  BUMP bump = bumpCreate (3, 0) ;

  i = (look->flag & FLAG_REVERSE) ? arrayMax(look->segs)-1 : 1 ;
  while (i && i < arrayMax(look->segs))
    { seg = arrp(look->segs,i,SEG) ;
      if (seg->x1 > look->max || seg->x2 < look->min)
	goto loop ;
      y = 0.5 * (MAP2GRAPH(look->map,seg->x1) +
		 MAP2GRAPH(look->map,seg->x2+1)) ;
      if (y < 2 + topMargin) y = 2 + topMargin ;
      if (seg->type == VISIBLE &&
	  seg->data.k == _Matching_cDNA)
	{ x = 0 ;
	  bumpItem (bump, 1, 0.5, &x, &y) ;
	  graphFillRectangle (*offset+x, y-0.25, *offset+x+0.8, y+0.25) ;
	}
    loop: if (look->flag & FLAG_REVERSE) --i ; else ++i ;
    }

  *offset += bumpMax (bump) ;
  bumpDestroy (bump) ;

  return;
} /* fMapShowcDNAs */

/**********************************************/

static FREEOPT segOpts[] = {
  { 8, "User controllable segments" },
  { SPLICE3, "SPLICE3" },
  { SPLICE5, "SPLICE5" },
  { FEATURE, "FEATURE" },
  { HOMOL, "HOMOL" },
  { SPLICE3_UP, "SPLICE3_UP" },
  { SPLICE5_UP, "SPLICE5_UP" },
  { FEATURE_UP, "FEATURE_UP" },
  { HOMOL_UP, "HOMOL_UP" }
} ;

void fMapAddSegments (void)
{
  static char dirName[DIR_BUFFER_SIZE] ="", fileName[FIL_BUFFER_SIZE]="";
  static Associator seq2seg = 0 ;
  void *v ;
  int i, x1, x2, n, level, seqIndex ;
  float score ;
  SEG *seg, *seqSeg ;
  KEY seq, type ;
  FILE *fil ;
  char *word ;
  FeatureMap look = currentFeatureMap("fMapAddSegments") ;

  if (!(fil = filqueryopen (dirName, fileName, "useg","r",
	   "File of new items: sequence x1 x2 type [score|text...]")))
    return ;

	/* first delete temp segs */
  arrayMax(look->segs) = look->lastTrueSeg ;
	/* make seq2seg */
  seq2seg = assReCreate (seq2seg) ;
  for (i = 1, seg = arrp(look->segs, 1, SEG) ; i < arrayMax(look->segs) ; ++i, ++seg)
    if (seg->type == SEQUENCE || seg->type == SEQUENCE_UP)
      assInsert (seq2seg, assVoid(seg->key), assVoid(i)) ;

  n = 0 ;
  level = freesetfile (fil,0) ;
  while (freecard (level))
    { if (messIsInterruptCalled())
	break ;
      if (!(word = freeword()))
	continue ;
      if (!lexword2key (word, &seq, _VSequence))
	{ messout ("Can't find sequence %s line %d - F4 to interrupt",
		   word, freestreamline(level)) ;
	  continue ;
	}
      if (!assFind (seq2seg, assVoid(seq), &v))
	continue ;
      seqIndex = assInt(v) ;
      if (!freeint (&x1) || !freeint (&x2) || !freekey (&type, segOpts))
	{ messout ("No coords then segtype line %d - F4 to interrupt", 
		   freestreamline(level)) ;
	  continue ;
	}
      if (x1 > x2)
	{ i = x1 ; x1 = x2 ; x2 = i ; 
	  if (type & 0x01) --type ; else ++type ;
	}
      seg = arrayp(look->segs, arrayMax(look->segs), SEG) ;
      seg->parent = 0 ;
      seqSeg = arrp(look->segs, seqIndex, SEG) ;
      if (seqSeg->type == SEQUENCE)
	{ seg->x1 = seqSeg->x1 + x1 - 1 ;
	  seg->x2 = seqSeg->x1 + x2 - 1 ;
	}
      else			/* SEQUENCE_UP */
	{ seg->x1 = seqSeg->x2 - x2 + 1 ;
	  seg->x2 = seqSeg->x2 - x1 + 1 ;
	  if (type & 0x01) --type ; else ++type ;
	}
      seg->type = type ;
      ++n ;

#define ABORT(z) { --arrayMax(look->segs) ; \
	      messout ("%s line %d - F4 to interrupt", z, freestreamline(level)) ; \
	      continue ; }

      switch (type)
	{
	case SPLICE3: case SPLICE3_UP: 
	case SPLICE5: case SPLICE5_UP:
	case FEATURE: case FEATURE_UP:
	  if (!(word = freeword()))
	    ABORT("No method") ;
	  if (lexaddkey (word, &seg->key, _VMethod))
	    messout ("Previously unknown method %s for FEATURE line %d", 
		     word, freestreamline(level)) ;

	  methodCacheGetByName (look->mcache, word,
				/* simple defaults */
				"Colour WHITE\n"
				"Right_priority 0\n"
				"Width   0.5\n",
				/* no diff yet */
				"");
	  seg->data.f = freefloat(&score) ? score : 0.0 ;
	  break ;
	case HOMOL: case HOMOL_UP:
	  { HOMOLINFO *hinf ;
	    seg->data.i = arrayMax(look->homolInfo) ;
	    hinf = arrayp (look->homolInfo, seg->data.i, HOMOLINFO) ;
	    if (!(word = freeword()))
	      ABORT("No method for HOMOL") ;
	    if (lexaddkey (word, &hinf->method, _VMethod))
	      messout ("Previously unknown method %s for HOMOL line %d", 
		       word, freestreamline(level)) ;
	    
	    methodCacheGetByName (look->mcache, word,
				  /* simple defaults */
				  "Colour WHITE\n"
				  "Right_priority 0\n"
				  "Width   0.5\n",
				  /* no diff yet */
				  "");
	    if (!freefloat(&hinf->score))
	      ABORT("No score for HOMOL") ;
	    if (!(word = freeword()))
	      ABORT("No sequence for HOMOL") ;
	    lexaddkey (word, &seg->key, _VSequence) ;
	    if (!freeint(&hinf->x1) || !freeint(&hinf->x2))
	      ABORT("No target coords for HOMOL") ;
	  }
	  break ;
	}
    }
  freeclose (level) ;
  messout ("Added %d new segments", n) ;

  arraySort (look->segs, fMapOrder) ;
  look->lastTrueSeg = arrayMax(look->segs) ;

  fMapProcessMethods (look) ;
  fMapDraw (look, 0) ;

  return;
} /* fMapAddSegments */


void fMapClearSegments (void)
{
  int i ;
  KEY method ;
  SEG *seg ;
  FeatureMap look = currentFeatureMap("fMapClearSegments") ;
  ACEIN name_in;
  BOOL found;

  name_in = messPrompt ("Give method of segment to clear", "", "wz", 0);
  if (!name_in)
    return;

  found = lexword2key (aceInWord(name_in), &method, _VMethod);
  aceInDestroy (name_in);

  if (!found)
    return ;

	/* first delete temp segs */
  arrayMax(look->segs) = look->lastTrueSeg ;

  for (i = 1 ; i < arrayMax(look->segs) ; )
    if ((seg = arrp(look->segs, i, SEG))->key == method)
      *seg = arr(look->segs, --arrayMax(look->segs), SEG) ;
    else
      ++i ;

  arraySort (look->segs, fMapOrder) ;
  look->lastTrueSeg = arrayMax(look->segs) ;

  fMapDraw (look, 0) ;

  return;
} /* fMapClearSegments */


/************************************************************/

void fMapSwitchColBump(FeatureMap look)
{
  int box = look->activeBox ;
  SEG* seg ;
  
  if (box >= arrayMax(look->boxIndex) || !(seg = BOXSEG(box)))
    {
      messerror("problem picking boxes in fMapSwitchColBump()") ;
      return ;
    }


  /* We need to be able to go from   box -> seg -> method   the first part is ok,
   * but going from seg to method is not straightforward...need to think about this
   * a bit..... */

  seg = BOXSEG(look->activeBox) ;


  return ;
}



/************************************************************/
/******* generic "box" column - for homols, features ********/

/* checks before using a column */
static void bcCheck (BoxCol *bc)
{
  METHOD* meth = bc->meth ;

  if (bc->look->map->isFrame
      && meth->flags & METHOD_FRAME_SENSITIVE)
    bc->isFrame = TRUE;
  else
    bc->isFrame = FALSE;

  if (meth->flags & METHOD_SCORE)
    {
      bc->mode = WIDTH ;				    /* n.b. default. */

      if (meth->flags & METHOD_SCORE_BY_OFFSET && !bc->look->map->isFrame)
	bc->mode = OFFSET ;
      else if (meth->flags & METHOD_SCORE_BY_HIST)
	bc->mode = HIST ;

      /* checks to prevent arithmetic crash */
      if (bc->mode == OFFSET)
	{
	  if (!meth->minScore)
	    meth->minScore = 1 ;
	}
      else
	{
	  if (meth->maxScore == meth->minScore)
	    meth->maxScore = meth->minScore + 1 ;
	}
    }
  else
    {
      /* none of the SCORE_BY_xxx flags set */
      bc->mode = DEFAULT ;
    }

  if (meth->width)
    {
      if (bc->mode == WIDTH
	   && meth->flags & METHOD_SCORE_BY_OFFSET
	   && meth->width > 2)
        bc->width = 2 ;
      else
	bc->width = meth->width ;
    }
  else
    {
      /* the method doesn't set the width */
      if (bc->mode == OFFSET)
	bc->width = 7 ;
      else
	bc->width = 2 ;
    }

  if (bc->mode == HIST)
    { 
      /* normalise the BoxCol's histBase */
      if (meth->minScore == meth->maxScore)
	bc->histBase = meth->minScore ;
      else
	bc->histBase = (meth->histBase - meth->minScore) / 
	               (meth->maxScore - meth->minScore) ;

      if (bc->histBase < 0) bc->histBase = 0 ;
      if (bc->histBase > 1) bc->histBase = 1 ;
      
      bc->fmax = (bc->width * bc->histBase) + 0.2 ;
    }
  else
    bc->fmax = 0 ;


  if (bc->bump)
    bumpDestroy (bc->bump);
  if (bc->cluster)
    assDestroy (bc->cluster);

  if (bc->overlap_mode == METHOD_OVERLAP_BUMPED)
    {
      bc->bump = bumpCreate (1000, 0);
    }
  else if (bc->overlap_mode == METHOD_OVERLAP_CLUSTER)
    {
      bc->cluster = assHandleCreate (bc->look->handle) ;
      bc->clusterCount = 0 ;
    }


  /* set the toggle_states in the bcMenu according to this BoxCol */
  if (bc->overlap_mode == METHOD_OVERLAP_BUMPED)
    {
      menuUnsetFlags (menuItem(bc->menu, LABEL_OVERLAP), MENUFLAG_TOGGLE_STATE);
      menuSetFlags (menuItem(bc->menu, LABEL_BUMPABLE), MENUFLAG_TOGGLE_STATE);
      menuUnsetFlags (menuItem(bc->menu, LABEL_CLUSTER), MENUFLAG_TOGGLE_STATE);
    }
  if (bc->overlap_mode == METHOD_OVERLAP_CLUSTER)
    {
      menuUnsetFlags (menuItem(bc->menu, LABEL_OVERLAP), MENUFLAG_TOGGLE_STATE);
      menuUnsetFlags (menuItem(bc->menu, LABEL_BUMPABLE), MENUFLAG_TOGGLE_STATE);
      menuSetFlags (menuItem(bc->menu, LABEL_CLUSTER), MENUFLAG_TOGGLE_STATE);
    }
  else
    {
      menuSetFlags (menuItem(bc->menu, LABEL_OVERLAP), MENUFLAG_TOGGLE_STATE);
      menuUnsetFlags (menuItem(bc->menu, LABEL_BUMPABLE), MENUFLAG_TOGGLE_STATE);
      menuUnsetFlags (menuItem(bc->menu, LABEL_CLUSTER), MENUFLAG_TOGGLE_STATE);
    }

  switch (bc->mode)
    {
    case WIDTH:
      menuSetFlags (menuItem(bc->menu, LABEL_WIDTH), MENUFLAG_TOGGLE_STATE);
      menuUnsetFlags (menuItem(bc->menu, LABEL_OFFSET), MENUFLAG_TOGGLE_STATE);
      menuUnsetFlags (menuItem(bc->menu, LABEL_HIST), MENUFLAG_TOGGLE_STATE);
      break;

    case OFFSET:
      menuUnsetFlags (menuItem(bc->menu, LABEL_WIDTH), MENUFLAG_TOGGLE_STATE);
      menuSetFlags (menuItem(bc->menu, LABEL_OFFSET), MENUFLAG_TOGGLE_STATE);
      menuUnsetFlags (menuItem(bc->menu, LABEL_HIST), MENUFLAG_TOGGLE_STATE);
      break;

    case HIST:
      menuUnsetFlags (menuItem(bc->menu, LABEL_WIDTH), MENUFLAG_TOGGLE_STATE);
      menuUnsetFlags (menuItem(bc->menu, LABEL_OFFSET), MENUFLAG_TOGGLE_STATE);
      menuSetFlags (menuItem(bc->menu, LABEL_HIST), MENUFLAG_TOGGLE_STATE);
      break;

    case DEFAULT:		/* all options off */
      menuUnsetFlags (menuItem(bc->menu, LABEL_WIDTH), MENUFLAG_TOGGLE_STATE);
      menuUnsetFlags (menuItem(bc->menu, LABEL_OFFSET), MENUFLAG_TOGGLE_STATE);
      menuUnsetFlags (menuItem(bc->menu, LABEL_HIST), MENUFLAG_TOGGLE_STATE);
      break;
    }

  return;
} /* bcCheck */

static void bcFinalise (void *block)
{
  /* ?? how does the bc->menu get finalised ? ?? */

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
  BoxCol *bc = (BoxCol*)block;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

  /* What happened to the clean up ???                                       */
  /* When does this all get cleaned up ?? When would be logical, when the    */
  /* fmap disappears ??                                                      */

  return;
} /* bcFinalise */


/* We need to return the menu we build here because it will become a submenu */
/* in a cascading menu for all overlapping columns at one screen position.   */
/*                                                                           */
static BoxCol *bcFromName(FeatureMap look, char *name, MENU *menu_out)
{
  int i ;
  BoxCol *bc ;
  MENUITEM menu_item;

  if (!look->bcDict)
    {
      look->bcDict = dictHandleCreate (64, look->handle) ;
      look->bcArray = arrayHandleCreate (64, BoxCol*, look->handle) ;
    }

  if (dictFind(look->bcDict, name, &i))
    {
      bc = arr(look->bcArray,i,BoxCol*) ;
    }
  else
    {				/* initialise from method */
      bc = (BoxCol*)halloc(sizeof(BoxCol), look->handle) ;
      blockSetFinalise (bc, bcFinalise);

      dictAdd(look->bcDict, name, &i) ;
      array(look->bcArray,i,BoxCol*) = bc ;
      
      bc->look = look ;

      if (name[0] == '-')
	{ 
	  bc->isDown = FALSE ;
	  bc->meth = methodCacheGetByName(look->mcache, name+1, 
					  "",		    /* no aceText-defaults */
					  "");		    /* no aceDiff-variation */
	}
      else
	{ 
	  bc->isDown = TRUE ;
	  bc->meth = methodCacheGetByName(look->mcache, name, 
					  "",		    /* no aceText-defaults */
					  "");		    /* no aceDiff-variation */
	}

      if (bc->meth->flags & METHOD_BUMPABLE)
	bc->overlap_mode = METHOD_OVERLAP_BUMPED ;
      else if (bc->meth->flags & METHOD_CLUSTER)
	bc->overlap_mode = METHOD_OVERLAP_CLUSTER ;
      else
	bc->overlap_mode = METHOD_OVERLAP_COMPLETE ;

      /********************/

      /* Create the menu */
      bc->menu = menuCreate(name);

      menu_item = menuCreateItem ("Handle overlapping entries by:", NULL);
      menuSetFlags (menu_item, MENUFLAG_DISABLED);
      menuAddItem (bc->menu, menu_item, NULL);

      menu_item = menuCreateItem (LABEL_OVERLAP, bcMenuSetOverlap);
      menuSetPtr (menu_item, bc);
      menuSetFlags (menu_item, MENUFLAG_TOGGLE);
      menuAddItem (bc->menu, menu_item, NULL);

      menu_item = menuCreateItem (LABEL_BUMPABLE, bcMenuSetBump);
      menuSetPtr (menu_item, bc);
      menuSetFlags (menu_item, MENUFLAG_TOGGLE);
      menuAddItem (bc->menu, menu_item, NULL);

      menu_item = menuCreateItem (LABEL_CLUSTER, bcMenuSetCluster);
      menuSetPtr (menu_item, bc);
      menuSetFlags (menu_item, MENUFLAG_TOGGLE);
      menuAddItem (bc->menu, menu_item, NULL);

      menu_item = menuCreateItem ("Show score by:", NULL);
      menuSetFlags (menu_item, MENUFLAG_DISABLED);
      menuAddItem (bc->menu, menu_item, NULL);

      menu_item = menuCreateItem (LABEL_WIDTH, bcMenuSetWidth);
      menuSetPtr (menu_item, bc);
      menuSetFlags (menu_item, MENUFLAG_TOGGLE);
      menuAddItem (bc->menu, menu_item, NULL);

      menu_item = menuCreateItem (LABEL_OFFSET, bcMenuSetOffset);
      menuSetPtr (menu_item, bc);
      menuSetFlags (menu_item, MENUFLAG_TOGGLE);
      menuAddItem (bc->menu, menu_item, NULL);

      menu_item = menuCreateItem (LABEL_HIST, bcMenuSetHist);
      menuSetPtr (menu_item, bc);
      menuSetFlags (menu_item, MENUFLAG_TOGGLE);
      menuAddItem (bc->menu, menu_item, NULL);

      menu_item = menuCreateItem ("", NULL);
      menuSetFlags (menu_item, MENUFLAG_SPACER);
      menuAddItem (bc->menu, menu_item, NULL);

      menu_item = menuCreateItem ("Switch off column", bcMenuDeleteMapColumn);
      menuSetPtr (menu_item, name);
      menuAddItem (bc->menu, menu_item, NULL);      
    }

  /* run consistency checks - establish valid modes/flags */
  bcCheck(bc) ;

  /* Return the menu for this column.                                        */
  *menu_out = bc->menu ;

  return bc ;
} /* bcFromName */

static BOOL bcTestMag (BoxCol *bc, float mag)
{
  if (mag < 0) 
    mag = -mag ;

  mag = 1.0 / mag ;

  if (bc->meth->minMag && mag < bc->meth->minMag)
    return FALSE ;

  if (bc->meth->maxMag && mag > bc->meth->maxMag) 
    return FALSE ;

  return TRUE ;
} /* bcTestMag */


static int bcDrawBox(BoxCol *bc, SEG *seg, float score, Array gaps)
{
  int box, xoff ;
  float x1 = 0, x2 = 0, y1 = 0, y2 = 0, dx = 0 ;
  float numerator, denominator ;
  double logdeux = log((double)2.0) ;


  if (seg->x1 < bc->look->min)
    y1 = MAP2GRAPH(bc->look->map, bc->look->min) ;
  else
    y1 = MAP2GRAPH(bc->look->map, seg->x1) ;
  if (seg->x2+1 > bc->look->max)
    y2 = MAP2GRAPH(bc->look->map, bc->look->max) ;
  else
    y2 = MAP2GRAPH(bc->look->map, seg->x2+1) ;

  if (y1 >= mapGraphHeight || y2 <= topMargin)
    return 0 ;


  /* Calculate the box size/position according to the style of column the
   * user has selected. */

  box = graphBoxStart() ;

  numerator = score - bc->meth->minScore ;
  denominator = bc->meth->maxScore - bc->meth->minScore ;

  switch (bc->mode)
    {
    case OFFSET:
      if (denominator == 0)				    /* catch div by zero */
	{
	  if (numerator < 0)
	    dx = 0.8 ;
	  else if (numerator > 0)
	    dx = 3 ;
	}
      else
	{
	  dx = 1 + (((float)numerator)/ ((float)(denominator))) ;
	}
      if (dx < .8) dx = .8 ; if (dx > 3) dx = 3 ; /* allow some leeway & catch dx == 0 */
      dx = bc->width * log((double)dx)/logdeux ;
      
      x1 = bc->offset + dx ;
      x2 = bc->offset + dx + .9 ;

      if (bc->fmax < dx + 1.5)
	bc->fmax = dx + 1.5 ;
      break ;

    case HIST:
      if (denominator == 0)				    /* catch div by zero */
	{
	  if (numerator < 0)
	    dx = 0 ;
	  else if (numerator > 1)
	    dx = 1 ;
	}
      else
	{
	  dx = numerator / denominator ;
	  if (dx < 0) dx = 0 ;
	  if (dx > 1) dx = 1 ;
	}
      x1 = bc->offset + (bc->width * bc->histBase) ;
      x2 = bc->offset + (bc->width * dx) ;

      if (bc->fmax < bc->width*dx)
	bc->fmax = bc->width*dx ;
      break ;

    case WIDTH:
      if (denominator == 0)				    /* catch div by zero */
	{
	  if (numerator < 0)
	    dx = 0.25 ;
	  else if (numerator > 0)
	    dx = 1 ;
	}
      else
	{
	  dx = 0.25 + ((0.75 * numerator) / denominator) ;
	}
      if (dx < 0.25) dx = 0.25 ;
      if (dx > 1) dx = 1 ;

      /* !WARNING! fall-through */

    case DEFAULT:
      if (bc->mode == DEFAULT)
	dx = 0.75 ;

      xoff = 1 ;
      if (bc->bump)
	{
	  if (bc->look->map->mag < 0)
	    {
	      y1 = -y1 ; y2 = -y2 ;
	      bumpItem (bc->bump, 2, (y2-y1), &xoff, &y1) ;
	      y1 = -y1 ; y2 = -y2 ;
	    }
	  else
	    bumpItem (bc->bump, 2, (y2-y1), &xoff, &y1) ;
	}
      else if (bc->cluster)	/* one subcolumn per key */
 	{
	  void *v ;
	  BOOL two_parent = seg->flags & SEGFLAG_TWO_PARENT_CLUSTER ;
	  BOOL found ;
	  KEY cluster_key = seg->parent ;

	  /* Some homol matches are paired (e.g. ESTs) but the matches are in
	   * different objects, for this we look in hinf for the key to cluster on. */
	  if (two_parent)
	    {
	      HOMOLINFO *hinf = arrp(bc->look->homolInfo, seg->data.i, HOMOLINFO) ;

	      cluster_key = hinf->cluster_key ;
	    }
	  else
	    {
	      /* Code in convertFeatures() in smapconvert.c hashes a text field in the feature
	       * into seg->parent using dictAdd() because these features do not have their
	       * own object key. Unfortunately dictAdd can return "1" which is converted by
	       * convertFeatures() to "-1" which is illegal for the associator code, hence we
	       * subtract 1 from all such hash values to avoid this situation...agh.... */

	      int key_value = (int)cluster_key ;

	      /* only do this for hashed text, not real object keys which are all +ve. */
	      if (key_value < 0)
		cluster_key-- ;
	    }

	  if ((found = assFind (bc->cluster, assVoid(cluster_key), &v)))
	    xoff = assInt(v) ;

	  if (!found)
	    {
	      xoff = ++bc->clusterCount ;
	      assInsert (bc->cluster, assVoid(cluster_key), assVoid(xoff)) ;
	    }
	}

      x1 = bc->offset + (0.5 * bc->width * (xoff - dx)) ;
      x2 = bc->offset + (0.5 * bc->width * (xoff + dx)) ;

      if (bc->fmax < 0.5*bc->width*(xoff+1))
	bc->fmax = 0.5*bc->width*(xoff+1) ;
      break ;

    default:
      messcrash("Unknown bc->mode in  bcDrawBox(): %d", bc->mode) ;
    }


  /* OK, now draw the homology line/box/gapped box. */

  if (seg->type == HOMOL_GAP || seg->type == HOMOL_GAP_UP)
    {
      /* Some homol lines are drawn dashed (e.g. between EST read pairs), note 
       * that because the homol line box only has a line drawn in, we have to
       * artificially set its width to make it clickable. */
      BOOL change_line_style = seg->flags & SEGFLAG_LINE_DASHED ;
      GraphLineStyle prev_style ;
      float mid_x, width ;
      float left, right ;

      if (change_line_style)
	prev_style = graphLineStyle(GRAPH_LINE_DASHED) ;

      mid_x = (x1 + x2) * 0.5 ;
      graphLine(mid_x, y1, mid_x, y2) ;

      if (change_line_style)
	graphLineStyle(prev_style) ;

      width = (x2 - x1) * 0.25 ;
      left = (mid_x - width) ; right = (mid_x + width) ; 
      graphBoxSetDim(box, &left, NULL, &right, NULL) ;
    }
  else
    {
      if (gaps)
	drawInterruptedRectangle(bc->look, gaps, x1, y1, x2, y2) ;
      else
	graphRectangle(x1, y1, x2, y2) ;
    }

  graphBoxEnd() ;

  if (seg->type == HOMOL_GAP || seg->type == HOMOL_GAP_UP)
    graphBoxDraw (box, bc->meth->colour, WHITE) ;
  else
    graphBoxDraw (box, BLACK, bc->meth->colour) ;

  fMapBoxInfo (bc->look, box, seg) ;

  return box ;
} /* bcDrawBox */


/************************************************************
 **************** BoxCol menu functions *********************
 ************************************************************/

static void bcMenuSetOverlap (MENUITEM menu_item)
{
  BoxCol *bc = (BoxCol*)menuGetPtr (menu_item);

  bc->overlap_mode = METHOD_OVERLAP_COMPLETE ;

  if (bc->look->redrawColumns)
    fMapDraw (bc->look, 0) ;

  return;
} /* bcMenuSetOverlap */

static void bcMenuSetBump (MENUITEM menu_item)
{
  BoxCol *bc = (BoxCol*)menuGetPtr (menu_item);

  bc->overlap_mode = METHOD_OVERLAP_BUMPED ;

  if (bc->look->redrawColumns)
    fMapDraw (bc->look, 0) ;

  return;
} /* bcMenuSetBump */

static void bcMenuSetCluster (MENUITEM menu_item)
{
  BoxCol *bc = (BoxCol*)menuGetPtr (menu_item);

  bc->overlap_mode = METHOD_OVERLAP_CLUSTER ;

  if (bc->look->redrawColumns)
    fMapDraw (bc->look, 0) ;

  return;
} /* bcMenuSetCluster */

static void bcMenuSetWidth (MENUITEM menu_item)
{
  BoxCol *bc = (BoxCol*)menuGetPtr (menu_item);

  bc->meth->flags ^= METHOD_SCORE_BY_WIDTH;   /* toggle flag */
  bc->meth->flags &= ~METHOD_SCORE_BY_OFFSET; /* unset flag */
  bc->meth->flags &= ~METHOD_SCORE_BY_HIST;   /* unset flag */

  bc->mode = WIDTH;

  if (bc->look->redrawColumns)
    fMapDraw (bc->look, 0) ;

  return;
} /* bcMenuSetWidth */

static void bcMenuSetOffset (MENUITEM menu_item)
{
  BoxCol *bc = (BoxCol*)menuGetPtr (menu_item);

  bc->meth->flags &= ~METHOD_SCORE_BY_WIDTH; /* unset flag */
  bc->meth->flags ^= METHOD_SCORE_BY_OFFSET; /* toggle flag */
  bc->meth->flags &= ~METHOD_SCORE_BY_HIST; /* unset flag */

  bc->mode = OFFSET;
 
  if (bc->look->redrawColumns)
    fMapDraw (bc->look, 0) ;

  return;
} /* bcMenuSetOffset */

static void bcMenuSetHist (MENUITEM menu_item)
{ 
  BoxCol *bc = (BoxCol*)menuGetPtr (menu_item);

  bc->meth->flags &= ~METHOD_SCORE_BY_WIDTH; /* unset flag */
  bc->meth->flags &= ~METHOD_SCORE_BY_OFFSET; /* unset flag */
  bc->meth->flags ^= METHOD_SCORE_BY_HIST; /* toggle flag */

  bc->mode = HIST;

  if (bc->look->redrawColumns)
    fMapDraw (bc->look, 0) ;

  return;
} /* bcMenuSetHist */

static void bcMenuDeleteMapColumn (MENUITEM menu_item)
{
  char *column_name = (char*)menuGetPtr(menu_item);
  FeatureMap look = currentFeatureMap("bcMenuDeleteMapColumn");

  mapColSetByName (look->map, column_name, MAPCOL_OFF) ;

  if (look->redrawColumns)
    fMapDraw (look, 0) ;

  return;
} /* bcMenuDeleteMapColumn */


/* BLIXEM routines                                                           */
/*                                                                           */

/* Whether the blixem call is internal or external the user may have set     */
/* preferences to use pfetch instead of efetch from blixem.                  */
/* Returns TRUE and a pointer to a filled in PfetchParams struct if pfetch   */
/* is required, FALSE otherwise.                                             */
/* NOTE: caller must free "out" using messfree().                            */
/*                                                                           */
static BOOL blixemGetPfetchParams(STORE_HANDLE handle, PfetchParams **out)
{
  BOOL result = FALSE ;
  PfetchParams *params = NULL ;
 
  if ((result = prefValue(BLIXEM_PFETCH)))
    {
      params = (PfetchParams *)halloc(sizeof(PfetchParams), handle) ;

      params->net_id = prefString(BLIXEM_NETID) ;
      params->port = prefInt(BLIXEM_PORT_NUMBER) ;

      *out = params ;
    }

  return result ;
}


/* Has the user chosen to use blixem as an external program ?                */
/*                                                                           */
static BOOL blixemIsExternal(void)
{
  BOOL result = FALSE ;

  result = prefValue("BLIXEM_EXTERNAL") ;

  return result ;
}


/* Call blixem as a stand alone external program.
 * The args to this program are identical to the synchronous call to blxview,
 * we convert the data into the input file formats that the standalone
 * blixem expects and then call it using the system() call but with an "&"
 * so that we don't wait. NOTE that this means that we have NO idea whether
 * blixem works, we can only tell that the shell started successfully.
 */
static void blixemCallExternal(char *seq, char *seqname,
			       int start, int offset, MSP *msp,
			       BOOL extended_format, char *opts, 
			       PfetchParams *pfetch_params, char *align_types)
{
  STORE_HANDLE handle = NULL ;
  BOOL status = TRUE ;
  char *script = NULL ;
  char *subdir = NULL ;
  ACETMP fasta_sequence = NULL, exblx_data = NULL, seqbl_data = NULL ;
  char *fasta_file = NULL, *exblx_file = NULL , *seqbl_file = NULL ;
  ACEOUT fasta_out = NULL, exblx_out = NULL, seqbl_out = NULL ;
  BOOL seqbl_written_to = FALSE ;

  handle = handleCreate() ;

  /* Can we find the cover script for blixem ? */
  script = prefString("BLIXEM_SCRIPT") ;
  if (strcmp(script, "") == 0)
    script = dbPathFilName(WSCRIPTS, "blixem_standalone", "", "x", handle) ;
  if (!script)
    {
      messout("Could not find the script to launch blixem: %s", script);
      status = FALSE ;
    }

  /* Create some tmp files which will receive the input data that blixem     */
  /* needs to do its stuff.                                                  */
  if (status)
    {
      subdir = hprintf(handle, "%s_ACEDB_BLIXEM", getLogin(TRUE)) ;
    }
  if (status)
    {
      fasta_sequence = aceTmpCreateDir(subdir, "w", handle) ;
      if (fasta_sequence)
	{
	  aceTmpNoRemove(fasta_sequence, TRUE) ;
	  fasta_file = aceTmpGetFileName(fasta_sequence) ;
	  fasta_out = aceTmpGetOutput(fasta_sequence) ;
	}
      else
	{
	  messerror("Could not create temporary file for Blixem Fasta sequence.") ;
	  status = FALSE ;
	}
    }
  if (status)
    {
      exblx_data = aceTmpCreateDir(subdir, "w", handle) ;
      if (exblx_data)
	{
	  aceTmpNoRemove(exblx_data, TRUE) ;
	  exblx_file = aceTmpGetFileName(exblx_data) ;
	  exblx_out = aceTmpGetOutput(exblx_data) ;
	}
      else
	{
	  messerror("Could not create temporary file for Blixem MSP/exblx data.") ;
	  status = FALSE ;
	}
    }
  if (status)
    {
      seqbl_data = aceTmpCreateDir(subdir, "w", handle) ;
      if (seqbl_data)
	{
	  aceTmpNoRemove(seqbl_data, TRUE) ;
	  seqbl_file = aceTmpGetFileName(seqbl_data) ;
	  seqbl_out = aceTmpGetOutput(seqbl_data) ;
	}
      else
	{
	  messerror("Could not create temporary file for Blixem  MSP/seqbl data.") ;
	  status = FALSE ;
	}
    }


  /* Write out the sequence to the Fasta file.                               */
  if (status)
    {
      if (!dnaRawDumpFastA(seqname, seq, fasta_out))
	  messerror("Could not write to blixem input sequence file: %s", fasta_file) ;
    }

  /* Write out the MSPCrunch exblx and seqbl format files.                   */
  if (status)
    {
      if (aceOutPrint(exblx_out, "%s\n%s%c\n",
		      (extended_format ? "# exblx_x" : "# exblx"),
		      "# blast", *opts) != ESUCCESS)
	status = FALSE ;

      if (aceOutPrint(seqbl_out, "%s\n%s%c\n",
		      (extended_format ? "# seqbl_x" : "# seqbl"),
		      "# blast", *opts) != ESUCCESS)
	status = FALSE ;


      /* If we already have the sequence data for the homology from acedb    */
      /* then write the data to a seqbl file, otherwise write it to an exblx */
      /* file.                                                               */
      if (status)
	{
	  for ( ; msp && status ; msp = msp->next)
	    {
	      ACEOUT out ;
	      char *seqstring ;

	      if (msp->sseq)
		out = seqbl_out ;
	      else
		out = exblx_out ;

	      /* Write the standard 8 cols. */
	      if (aceOutPrint(out, "%d\t%s\t%d\t%d\t%s\t%d\t%d\t%s",
			      msp->score,
			      msp->qframe,
			      msp->qstart, msp->qend,
			      (extended_format ? msp->sframe : ""),
			      msp->sstart, msp->send,
			      msp->sname) != ESUCCESS)
		status = FALSE ;
	      
	      /* For extended format export gaps if there are any. */
	      if (status && extended_format)
		{
		  if (msp->gaps)
		    {
		      int i ;

		      if (aceOutPrint(out, "%s", "\tGaps ") != ESUCCESS)
			status = FALSE ;
		      
		      for (i = 0 ; status && i < arrayMax(msp->gaps) ; i++)
			{
			  SMapMap *m = arrp(msp->gaps, i, SMapMap);
			  
			  if (aceOutPrint(out, " %d %d %d %d",
					  m->s1, m->s2, m->r1, m->r2) != ESUCCESS)
			    status = FALSE ;
			}

		      if (aceOutPrint(out, "%s", " ;") != ESUCCESS)
			status = FALSE ;
		    }
		}

	      if (status)
		{
		  char *tag = NULL ;

		  if (msp->sseq)
		    {
		      if (extended_format)
			tag = "Sequence" ;
		      seqstring = msp->sseq ;
		      seqbl_written_to = TRUE ;
		    }
		  else
		    {
		      if (extended_format)
			tag = "Description" ;
		      seqstring = msp->desc ;
		    }

		  if (seqstring)
		    {
		      if (aceOutPrint(out, "\t%s %s ;",
				      (extended_format ? tag : ""),
				      seqstring) != ESUCCESS)
			status = FALSE ;
		    }
		}
	      
	      if (status)
		{
		  if (aceOutPrint(out, "\n") != ESUCCESS)
		    status = FALSE ;
		}
	    }
	}


      if (!status)
	messerror("Could not write to one or both of blixem data files: %s, %s",
		  exblx_file, seqbl_file) ;
    }

  /* Close the files to make sure they are ready for the blixem process to   */
  /* read.                                                                   */
  if (status)
    {
      aceTmpClose(fasta_sequence) ;
      aceTmpClose(exblx_data) ;
      aceTmpClose(seqbl_data) ;

      /* If no data written to seqbl file make sure it will be removed when  */
      /* we finish.                                                          */
      if (!seqbl_written_to)
	aceTmpNoRemove(seqbl_data, FALSE) ;
    }

  /* Construct the command line and start up blixem, NOTE that by appending
   * an "&" we do not find out if the blixem worked, only that it was
   * forked off successfully. */
  if (status)
    {
      char *cmd_string = NULL ;
      char *pfetch_arg = NULL ;
      char *align_arg = NULL ;

      if (pfetch_params)
	pfetch_arg = hprintf(handle, "-P %s:%d ", pfetch_params->net_id, pfetch_params->port) ;

      if (align_types)
	align_arg = hprintf(handle, "-a \'%s\'", align_types) ;

      cmd_string = hprintf(handle, "%s %s %s -S %d -O %d -o \"%s\" %s %s %s %s &",
			   (align_arg ? align_arg : ""),
			   (pfetch_arg ? pfetch_arg : ""),
			   prefValue("BLIXEM_TEMPFILES") ? "" : "-r", /* Remove temporary files ? */
			   start,
			   offset,
			   opts,
			   (seqbl_written_to ? "-x" : ""),
			   (seqbl_written_to ? seqbl_file : ""),
			   fasta_file,
			   exblx_file) ;

      printf("%s\n", cmd_string) ;

      if (callScript(script, cmd_string) != 0)
	messerror("Could not start blixem, check that %s exists and is executable by you.",
		  script) ;
    }


  /* Clean up.                                                               */
  handleDestroy(handle) ;

  return ;
}

/********************/

/* This duplicates a function in peptide.c, they should be amalgamated into one in a new
 * fasta.c file.... */
static void translationTitle (ACEOUT fo, KEY seq)
{
  OBJ obj = bsCreate (seq) ;
  KEY key ;

  aceOutPrint (fo, ">%s", name(seq)) ;
  if (obj)
    { if (bsGetKey (obj, _Locus, &key))
	aceOutPrint (fo, " %s:", name(key)) ;
      if (bsGetKey (obj, _Brief_identification, &key))
	aceOutPrint (fo, " %s", name(key)) ;
      if (bsFindTag (obj, _Start_not_found))
	aceOutPrint (fo, " (start missing)") ;
      if (bsFindTag (obj, _End_not_found))
	aceOutPrint (fo, " (end missing)") ;
      bsDestroy (obj) ;
    }
  else
    messerror ("Missing object for %s in translationTitle", name(seq)) ;
  aceOutPrint (fo, "\n") ;

  return;
}


static void pfetchWindow (KEY key)
{
  ACEIN pipe;
  char  cmd[50+1] = "pfetch";
  char  title[50+1];
  char *lineBuf, *textBuf;
  STORE_HANDLE handle = handleCreate();

  sprintf(cmd, "pfetch -F '%s' &", name(key));
  sprintf(title, "pfetch: %s", name(key));

  pipe = aceInCreateFromPipe(cmd, "r", NULL, handle);
  if (pipe)
    {
      textBuf = strnew("", handle);
      aceInSpecial(pipe, "\n\t");

      while ((lineBuf = aceInCard(pipe)))
	  textBuf = g_strconcat(textBuf,"\n", lineBuf, NULL);

      /* call gexTextEditor with the pfetch output as fixed width text. */
      gexTextEditorNew(title, textBuf, 0,
		       NULL, NULL, NULL,		    /* No buttons/actions. */
		       FALSE, FALSE, TRUE) ;		    /* readonly, fixed width. */
    }

  handleDestroy(handle);
}
/********************** end of file **************************/
