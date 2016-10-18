/*  File: fmapcontrol.c
 *  Author: Richard Durbin (rd@sanger.ac.uk)
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
 * 	Richard Durbin (Sanger Centre, UK) rd@sanger.ac.uk, and
 *	Jean Thierry-Mieg (CRBM du CNRS, France) mieg@kaa.cnrs-mop.fr
 *
 * Description: display choice control for fmap
 * Exported functions:
 * HISTORY:
 * Last edited: Nov 23 11:54 2011 (edgrif)
 * * Mar 13 22:55 2002 (rd): fixed fMapFindSpan() to use sMapInverseMap() not SEGs
 *		and removed fMapFindZoneFather(), replacing calls with fMapFindSpan()
 * * Apr  3 14:33 2000 (edgrif): On behalf of Lincoln I have:
 *              "Reintegrated some of the GFF dumping features introduced in 4.7l."
 * * Mar 30 15:42 2000 (edgrif): Added code for "no redrawing" of columns while
 *              selecting them.
 * * Oct  5 16:30 1999 (fw): new methodcache system
 * * May 25 15:15 1999 (edgrif): Put column reuse into main menu.
 * * May 21 22:29 1999 (edgrif): Add support for retaining column selections on map reuse.
 * * Oct 21 11:09 1998 (edgrif): Corrected func. proto for fMapFollowVirtualMultiTrace
 *              a whole load of fmap functions are nulled for ACEMBLY way down the file.
 * * Jul 27 10:36 1998 (edgrif): Add calls to fMapIntialise() for externally
 *      called routines.
 * * Jul 16 10:06 1998 (edgrif): Introduce private header fmap_.h
 * * Jun 24 14:17 1998 (edgrif): Fix initialisation bug to make sure isDone
 *      is set in fmapInitialise. Also remove methodInitialise from fmapInitialise,
 *	this is not needed, method does its own intialisation.
 * * Dec 21 09:00 1995 (mieg)
 * * Jun 21 00:18 1995 (rd): fused with Jean - remaining discrepancies in
 *	readMethod(s) sections - probably he found a bug here with 1, 2 etc.
 *	and with HOMOL unshading in fMapSelect - I am quite confident here.
 * Created: Sat Jul 25 20:19:11 1992 (rd)
 * CVS info:   $Id: fmapcontrol.c,v 1.332 2012-10-30 09:55:00 edgrif Exp $
 *-------------------------------------------------------------------
 */

#undef FMAP_DEBUG					    /* change to #define for debug output. */

#include <glib.h>
#include <wh/acedb.h>
#include <wh/aceio.h>
#include <wh/chrono.h>
#include <wh/lex.h>
#include <wh/a.h>
#include <wh/bindex.h>
#include <whooks/sysclass.h>
#include <whooks/classes.h>
#include <whooks/systags.h>
#include <whooks/tags.h>
#include <wh/display.h>
#include <wh/dna.h>
#include <wh/peptide.h>
#include <wh/query.h>
#include <wh/matchtable.h>
#include <wh/keysetdisp.h>
#include <wh/gelmap.h>
#include <wh/methodcache.h>
#include <wh/smap.h>
#include <wh/smapconvert.h>
#include <wh/maputils.h>
#include <wh/embl.h>					    /* "EMBL dump" menu func.*/
#include <wh/pref.h>
#include <wh/pick.h>					    /* For pickWord2Class() which should
							       surely be in somwhere like
							       acedb.h as a fundamental key like
							       func. */


#include <w7/fmap_.h>
#include <w7/fmapsegdump.h>



/*************** globals within fMap Package *******************/

/* addresses of these two are used as unique identifiers */
/* find the FeatureMap struct on the active graph */
magic_t GRAPH2FeatureMap_ASSOC = "FeatureMap";

/* verify a generic pointer to be a FeatureMap */
magic_t FeatureMap_MAGIC = "FeatureMap";


/* Can be set to TRUE to turn on debugging.                                  */
BOOL fMapDebug = FALSE ;


/* TEMP for testing stuff for James...                                       */
static FMapCutDataType fMapCutCoords_G = FMAP_CUT_DNA ;



/********************** local functions***********************/
static BOOL reportErrors;
static consMapDNAErrorReturn callback(KEY key, int position);
static void fMapDrag(float y);
static void fMapSelectBox (FeatureMap look, int box, double x, double y);
static void fMapFollow (FeatureMap look, double x, double y) ;
static void fMapCentre (FeatureMap look, KEY key, KEY from) ;
static void fMapPick(int box, double x, double y, GraphKeyModifier modifier) ;
static void fMapCompHelp (void) ;
static void fMapReverse (void) ;
static BOOL fMapDoRecalculate (FeatureMap look, int start, int stop) ;
static void fMapZoneKeySet (void) ;
static void fMapKbd(int k, GraphKeyModifier modifier) ;

static FeatureMap fMapCreateLook(KEY seq, int start, int stop, BOOL reuse, BOOL extend, BOOL noclip,
				 FmapDisplayData *display_data);
static BOOL setStartStop(KEY seq, int *start_inout, int *stop_inout, KEY *key_out, BOOL extend, FeatureMap look) ;

static void fMapDefaultColumns (MAP map) ;

static BOOL checkSequenceLength(KEY seq) ;

static BOOL keyMapsFrom(KEY key, KEY from) ;

static void dumpstats(FeatureMap look) ;

void SetFeatureFlags(ACEIN command_in, unsigned int *features);

static BOOL pred(KEY key) ;
static BOOL predWrite(KEY key) ;


/* One day these should be in menu code.                                     */
static int getPositionInMenu(MenuBaseFunction func, MENUOPT *menu) ;
static BOOL removeFromMenu(MenuBaseFunction func, MENUOPT *menu) ;
static void toggleTextInMenu(MENUOPT *m, MENUFUNCTION menu_func, char *menu_text) ;

/* prototypes for menu funcs */
static void setDisplayPreserve(void) ;
static void toggleColumnButtons (void) ;
static void toggleReportMismatch(MENUITEM unused) ;
static void togglePreserveColumns(MENUITEM unused) ;
static void toggleRedrawColumns(MENUITEM unused) ;
static void toggleCutSelection(MENUITEM unused) ;
static void zoom1 (void) ;
static void zoom10 (void) ;
static void zoom100(void) ;
static void zoom1000 (void) ;
static void zoomAny(FeatureMap look, float zoom_factor) ;
static void fMapWhole (void) ;
static void fMapMenuToggleTranslation (void) ;
static void fMapMenuReverseComplement (void) ;
static void fMapMenuRecalculate (void) ;
static void fMapMenuComplementSurPlace (void) ;
static void fMapMenuComplement (void) ;
static void revComplement(FeatureMap look) ;
static void fMapMenuToggleHeader (void);         /* "Hide Header" */
static void fMapMenuDumpSeq (void);              /* "Export Sequence" */
static void fMapMenuDumpSegs (void) ;	         /* "Export Features" */
static void fMapMenuDumpSegsTest(void) ;		    /* Test "Export Features" */
static void geneStats (void) ;	                 /* "Statistics" */
static void fMapMenuReadFastaFile (void);        /* "Import Sequence" */


static void colourSiblings(FeatureMap look, KEY parent) ;


/* functions that manipulate the given FeatureMap look */
static void fMapToggleDna (FeatureMap look);
static BOOL fMapToggleTranslation (FeatureMap look);
static void fMapToggleHeader (FeatureMap look);
#ifdef ACEMBLY
static void fMapTraceJoin (void) ;
#endif

/* debug routines. */
static void fMapLookDataDump(FeatureMap look, ACEOUT dest) ;
static void printSeg(int segnum, SEG *seg, ACEOUT dest) ;


/* Functions that implement keyboard shortcuts.  */
static void boxFocus(FeatureMap look, int key) ;
static void boxSelect(FeatureMap look) ;
static void viewStartEnd(FeatureMap look, int key) ;
static void pageUpDown(FeatureMap look, int key) ;
static void creepUpDown(FeatureMap look, int key) ;
static void moveUpDown(FeatureMap look, int distance) ;


/* Strings that must be toggle in menu items, a better solution would be     */
/* toggle buttons for many items which I will do sometime.                   */
#define PRESERVE_COLS    "Preserve Current Columns"
#define UN_PRESERVE_COLS "Do not Preserve Current Columns"

#define NOREDRAW_COLS    "No Automatic Redraw of Columns"
#define REDRAW_COLS      "Automatic Redraw of Columns"

#define REPORT_MISMATCHES    "Report DNA mismatches"
#define NO_REPORT_MISMATCHES "Don't report DNA mismatches"

#define CUT_DNA         "Put DNA in cut buffer"
#define CUT_COORDS      "Put Object Coords in cut buffer"


/********************** locals *******************************/
static KEY _Feature ;
static KEY _Tm, _Temporary ;
static KEY _EMBL_feature ;
       KEY _Arg1_URL_prefix ;		/* really ugly to make global */
       KEY _Arg1_URL_suffix ;
       KEY _Arg2_URL_prefix ;
       KEY _Arg2_URL_suffix ;
static KEY _Spliced_cDNA, _Transcribed_gene ;
static KEY _VTranscribed_gene = 0, _VIST = 0 ;
/* mhmp 25.09.98 */

static int weight, seqDragBox, nbTrans ;

static FeatureMap selectedfMap = NULL ; 


/************************************************************/
/* Default map column settings for fmap.                                     */
static FeatureMapColSettingsStruct defaultMapColumns[] =
{
  /* all column position 100, -90 ... should be different */
#define STL_STATUS_0 
  {-100.0, TRUE, "Locator", fMapShowMiniSeq},
  {-90.0, TRUE, "Sequences & ends", fMapShowCanonical},

#ifdef STL_STATUS  /* mike holman status column */
  {-89.0, TRUE, "Cosmids by group", fMapShowOrigin},
#endif

  {-2.1, FALSE, "Up Gene Translation", fMapShowUpGeneTranslation},
  {-1.9,  TRUE, "-Confirmed introns", fMapShowSoloConfirmed},
  {-0.1,  TRUE, "Restriction map", fMapShowCptSites},

#ifdef ACEMBLY
  /* Contig bar supersedes locator */
  {0.0, FALSE, "Summary bar", fMapShowSummary},
#else
  {0.0,  TRUE, "Summary bar", fMapShowSummary},
#endif

  {0.1,  TRUE, "Scale", fMapShowScale},
  {1.9,  TRUE, "Confirmed introns", fMapShowSoloConfirmed},
  {3.0,  TRUE, "EMBL features", fMapShowEmblFeatures},

#ifdef STL_STATUS
  {3.1,  TRUE, "Cosmid Status", fMapShowStatus},
#endif

  {3.2, FALSE, "CDS Lines", fMapShowCDSLines},
  {3.25, FALSE, "CDS Boxes", fMapShowCDSBoxes},
  {3.3,  TRUE, "Alleles", fMapShowAlleles},
  {3.4, FALSE, "cDNAs", fMapShowcDNAs},
  {3.5, FALSE, "Gene Names", fMapShowGeneNames},
  {3.7,  TRUE, "Assembly Tags", fMapShowAssemblyTags},
  {3.8, TRUE, "Oligos", fMapOspShowOligo},
  {3.82, TRUE, "Oligo_pairs", fMapOspShowOligoPairs},
/* isFrame starts if either of next 2 are On */
  {4.0, FALSE, "3 Frame Translation", fMapShow3FrameTranslation},
  {4.05, FALSE, "ORF's", fMapShowORF},
  {4.1, TRUE, "Coding Frame", fMapShowCoding},	/* only shows if isFrame */
  {4.2, FALSE, "ATG", fMapShowATG},
/* frame dependent stuff ends */
  /* {4.98, FALSE, "Gene Translation", fMapShowGeneTranslation}, */
  {4.99, FALSE, "Down Gene Translation", fMapShowDownGeneTranslation},

#ifdef ACEMBLY
  {5.5,  TRUE, "Alignements", fMapShowAlignments},
  {5.6,  TRUE, "Previous Contigs", fMapShowPreviousContigs},
  {5.7,  TRUE, "Contigs", fMapShowContig},
  {5.8,  TRUE, "Trace Assembly", fMapShowTraceAssembly},
  {5.9,  TRUE, "Multiplets", fMapShowVirtualMultiplets},
#endif

  {6.0, FALSE, "Coords", fMapShowCoords},
  {6.1, FALSE, "DNA Sequence", fMapShowDNA},

#ifdef ACEMBLY
  {6.2, FALSE, "Assembly DNA", fMapShowVirtualDna},
#endif

  {6.5, FALSE, "Brief Identifications", fMapShowBriefID},
  {7.0, TRUE, "Text Features", fMapShowText},
  {0.0, FALSE, NULL, NULL}

} ;


/************************************************************/
/* Main fmap control buttons.                                                */
/*                                                                           */
/* The buttons...                                                            */
static MENUOPT buttonOpts[] = {

#ifndef ACEMBLY
  {toggleColumnButtons, "Columns"},
#endif  /* ACEMBLY */

  { mapZoomIn, "Zoom In.."},
  { mapZoomOut, "Zoom Out.."},

#ifdef ACEMBLY
  /* dna... called in fmaptrace */
  { fMapMenuReverseComplement, "Complement"},
  { dnaAnalyse, "Analysis.."},
  { fMapTraceJoin, "Assembly.."},
#else /* not ACEMBLY */
  { fMapClear, "Clear"},
  { fMapMenuReverseComplement, "Rev-Comp.."},
  { fMapMenuToggleDna, "DNA.."},
  { dnaAnalyse, "Analysis.."},
#endif /* not ACEMBLY */

  { fMapMenuAddGfSegs, "GeneFind.."},
  {0, 0}} ;

#define NUM_BUTTON_BOXES UtArraySize(buttonOpts)


/* The button menus...                                                       */
static MENUOPT fMapZoomOpts[] = {
  { zoom1, "1 bp/line"},
  { zoom10, "10 bp/line"},
  { zoom100, "100 bp/line"},
  { zoom1000, "1000 bp/line"},
  { fMapWhole, "Whole"},
  { 0, 0 }
} ;

static MENUOPT fMapDnaOpts[] = {
  { fMapMenuToggleDna, "DNA On-Off" }, 
  { fMapMenuToggleTranslation, "Translate Gene On-Off" },
  { fMapSetDnaWidth, "Set DNA Width" }, 
  { fMapToggleAutoWidth, "Toggle Auto-DNA-width" },
  { 0, 0 }
} ;

static MENUOPT fMapAnalysisOpts[] =
{
  { menuSpacer, "Built in tools"},
  { dnaAnalyse, "   Restriction Analysis, codon usage etc." }, 
  { fMapGelDisplay, "   Artificial gels" },
  { fMapMenuAddGfSegs, "   Genefinder (Green)" }, 
  { menuSpacer, "External tools"},
  { fMapBlastInit, "   BLAST: search external database (Altschul & al)" }, 
  { fMapMenesInit, "   BioMotif: search a complex motif (Menessier)" }, 
  { fMapOspInit, "   OSP: Oligos Selection Program (Hillier)" }, 
#ifdef ACEMBLY
  { dnaCptFindRepeats, "   Repeats: Printout repeat structure (Parsons)" },
  { abiFixFinish, "   Finish: select new reads, StLouis specific (Marth)" },
#endif
  { 0, 0}
} ;

static MENUOPT fMapComplementOpts[] =
{
  { fMapMenuReverseComplement, "Reverse-Complement" },
  { fMapReverse, "Reverse" },
  { fMapMenuComplement, "Complement" },
  { fMapMenuComplementSurPlace, "Complement in place" },
  { fMapCompHelp, "Help-Explanation" },
  { 0, 0}
} ;


/********************************************************************/
/***********             main menu                       ************/
/*                                                                  */
static MENUOPT fMapMenu[] = {
  { graphDestroy,           "Quit" },
  { help,                   "Help" },
  { graphPrint,             "Print Screen" },
  { mapPrint,               "Print Whole Map" },
  { setDisplayPreserve,     "Preserve" },
  { MENU_FUNC_CAST togglePreserveColumns,  UN_PRESERVE_COLS},
  { MENU_FUNC_CAST toggleRedrawColumns,    NOREDRAW_COLS},
  { MENU_FUNC_CAST toggleReportMismatch,   NO_REPORT_MISMATCHES},
  { MENU_FUNC_CAST toggleCutSelection,   CUT_COORDS},
  { menuSpacer,             "" },
  { fMapMenuRecalculate,    "Recalculate" },
  { fMapMenuToggleHeader,   "Hide Header" },
  { menuSpacer,             "" },
  { fMapMenuDumpSeq,        "Export Sequence" },
  { fMapMenuDumpSegs,       "Export Features" },

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
  { fMapMenuDumpSegsTest,   "Export Features - wormtest" },
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

  { fMapExportTranslations, "Export translations" },
  { fMapZoneKeySet,         "Make a key set from the active zone" }, 
  { emblDump,               "EMBL dump" },
  { fMapMenuReadFastaFile,  "Import Sequence" },
  { menuSpacer,             "" },
#ifdef ACEMBLY
  { mapColControl,          "Columns"} , 
#else  /* !ACEMBLY */
  { geneStats,              "Statistics" },
  { fMapAddSegments,        "Read Segments" },
  { fMapClearSegments,      "Clear Segments" },
#endif /* !ACEMBLY */
  { 0, 0 }
  } ;

#define FMAPMENU_SIZE sizeof(fMapMenu)

char* fMapSegTypeName[] = { 
  "MASTER", "ASSEMBLY_TAG",

  "SEQUENCE", "SEQUENCE_UP",

  "CDS", "CDS_UP",

  "INTRON", "INTRON_UP",
  "EXON", "EXON_UP",

  "EMBL_FEATURE", "EMBL_FEATURE_UP", 
  "FEATURE", "FEATURE_UP", 
  "ATG", "ATG_UP",
  "SPLICE3", "SPLICE3_UP",
  "SPLICE5", "SPLICE5_UP",
  "CODING", "CODING_UP",
  "TRANSCRIPT", "TRANSCRIPT_UP",
  "SPLICED_cDNA", "SPLICED_cDNA_UP",

  "VIRTUAL_SUB_SEQUENCE_TAG",  "VIRTUAL_SUB_SEQUENCE_TAG_UP",
  "VIRTUAL_MULTIPLET_TAG", "VIRTUAL_MULTIPLET_TAG_UP",
  "VIRTUAL_ALIGNED_TAG",  "VIRTUAL_ALIGNED_TAG_UP",
  "VIRTUAL_PREVIOUS_CONTIG_TAG",  "VIRTUAL_PREVIOUS_CONTIG_TAG_UP",

  "HOMOL", "HOMOL_UP",
  "HOMOL_GAP", "HOMOL_GAP_UP",

  "PRIMER", "PRIMER_UP",
  "OLIGO", "OLIGO_UP",
  "OLIGO_PAIR", "OLIGO_PAIR_UP",
  "TRANS_SEQ", "TRANS_SEQ_UP",

  "FEATURE_OBJ", "FEATURE_OBJ_UP",

  "ALLELE", "ALLELE_UP",

  "VISIBLE",
  "DNA_SEQ", "PEP_SEQ", "ORF",
 
  "VIRTUAL_PARENT_SEQUENCE_TAG",
  "VIRTUAL_CONTIG_TAG",

  "ASSEMBLY_PATH",

  "CLONE_END"
  } ;


/**************************************************************************/

/* Verify that a pointer actually points to a FeatureMap struct, note that this function.
 * either returns TRUE or messcrah's */
BOOL uVerifyFeatureMap(FeatureMap fmap, char *caller, char *filename, int line_num)
{
  char *err_site = NULL ;

  err_site = hprintf(0, "(called from %s() in %s at line %d)", caller, filename, line_num) ;

  if (!fmap)
    messcrash("verifyFeatureMap() received NULL FeatureMap pointer %s.", err_site);
  if (fmap && fmap->magic != &FeatureMap_MAGIC)
    messcrash("verifyFeatureMap() received non-magic FeatureMap pointer %s.", err_site);
  
  messfree(err_site) ;

  return TRUE ;
}


/* get the FeatureMap struct for the current graph and verify the pointer */
FeatureMap uCurrentFeatureMap(char *caller, char *filename, int line_num)
{
  FeatureMap fmap;
  char *err_site = NULL ;

  err_site = hprintf(0, "(called from %s() in %s at line %d)", caller, filename, line_num) ;

  if (!graphAssFind (&GRAPH2FeatureMap_ASSOC,&fmap))
    messcrash("currentFeatureMap() could not find FeatureMap on graph %s.", err_site);

  uVerifyFeatureMap(fmap, caller, filename, line_num) ;

  messfree(err_site) ;

  return fmap;
} /* currentFeatureMap */



/* This code loads the default columns for fmap which continue to puzzle users as
 * some can be overridden, some not.
 *
 * I have added code to allow overriding their position and whether they appear.
 * This is because just specifying a method with the right name may not work because
 * the code that draws these columns does not always look in the method object to
 * derive position etc.
 */
static void fMapDefaultColumns(MAP map)
{
  int i ;
  int method_class ;
  KEY priority_tag, no_display_tag ;

  method_class = pickWord2Class("method") ;
  priority_tag = str2tag("Right_priority") ;
  no_display_tag = str2tag("No_display") ;

  i = 0 ;
  while (defaultMapColumns[i].name != NULL && defaultMapColumns[i].func != NULL)
    {
      KEY method_key = KEY_UNDEFINED ;

      if (lexword2key(defaultMapColumns[i].name, &method_key, method_class))
	{
	  if (bIndexTag(method_key, priority_tag))
	    {
	      OBJ obj ;

	      if ((obj  = bsCreate(method_key)))
		{
		  float priority = 0.0 ;

		  if (bsGetData (obj, priority_tag, _Float, &priority))
		    defaultMapColumns[i].priority = priority ;

		  bsDestroy(obj) ;
		}
	    }

	  if (bIndexTag(method_key, no_display_tag))
	    defaultMapColumns[i].isOn = FALSE ;
	}

      mapInsertCol(map, defaultMapColumns[i].priority, defaultMapColumns[i].isOn,
		   defaultMapColumns[i].name, defaultMapColumns[i].func) ;
      i++ ;
   }

  return ;
}


/*************** tags and their initialisation ***************/

/* This routine is called whenever an fmap interface routine is called       */
/* to make sure that the fmap package is initialised, initialisation is      */
/* only done on the first call.                                              */
void fMapInitialise (void)
{
  static int isDone = FALSE ;
  KEY key ;

  if (!isDone)
    {
      isDone = TRUE ;

      _Feature = str2tag("Feature") ;
      _EMBL_feature = str2tag("EMBL_feature") ;
      _Arg1_URL_prefix = str2tag("Arg1_URL_prefix");
      _Arg1_URL_suffix = str2tag("Arg1_URL_suffix") ;
      _Arg2_URL_prefix = str2tag("Arg2_URL_prefix") ;
      _Arg2_URL_suffix = str2tag("Arg2_URL_suffix") ;
      _Spliced_cDNA  = str2tag ("Spliced_cDNA") ;
      _Transcribed_gene = str2tag ("Transcribed_gene") ;
      _Tm = str2tag ("Tm") ;
      _Temporary = str2tag ("Temporary") ;
      if (lexword2key ("Transcribed_gene", &key, _VMainClasses))
	_VTranscribed_gene = KEYKEY(key) ;
      if (lexword2key ("IST", &key, _VMainClasses))
	_VIST = KEYKEY(key) ;
  

      /* THIS IS GRIM, SURELY THIS SHOULD NOT BE SET HERE  ????              */
      /* Actually I now know why this is here...the zooming all gets screwed up initially
       * if its not....this is because all the topmargin gubbins is tied up with
       * the calculations of size of fmap etc. etc.....horribleness....
       * the topMargin is needed BEFORE all the top margin stuff is
       * drawn, but its then incorrect and this means the magnification is incorrect
       * which means loads of stuff is in the wrong place. The solution is really to
       * put all the button stuff in a separate window......*/
#ifdef ACEMBLY
      topMargin = 6.5 ;
#else
      topMargin = 4.5 ;		/* fixes minor bug from Sam */
#endif
    }

  return ;
}


/* Allows application to set default type of data placed in cut buffer. */
void fMapSetDefaultCutDataType(FMapCutDataType cut_data)
{
  fMapCutCoords_G = cut_data ;

  return ;
}





/***************************************************/
/******** Communication toolKit ********************/
static void fMapUnselect (void)
{
  if (selectedfMap)
    {
      if (selectedfMap->magic != &FeatureMap_MAGIC)
	messcrash ("fMapUnselect() received non-magic FeatureMap pointer");

      if (selectedfMap->selectBox)
	{
	  Graph hold = graphActive () ;
	  graphActivate (selectedfMap->graph) ;
	  graphBoxDraw (selectedfMap->selectBox, WHITE, WHITE) ;
	  graphActivate (hold) ;
	}
      selectedfMap = NULL ;
    }

  return ;
}

/***************/

static void fMapSelect (FeatureMap look)
{
  if (selectedfMap && selectedfMap != look)
    fMapUnselect() ;
  selectedfMap = look ;
  if (look->selectBox)
    graphBoxDraw (look->selectBox,BLACK,LIGHTRED) ;

  return ;
}

/***************/
/* can be called with NULL adresses for undesired parameters */
BOOL fMapActive(Array *dnap, Array *dnaColp, KEY *seqKeyp, FeatureMap* lookp)
{
  BOOL mapIsActive ;

  fMapInitialise() ;

  if (selectedfMap && graphExists(selectedfMap->graph))
    {
      if (selectedfMap->magic != &FeatureMap_MAGIC)
	messcrash("fMapActive has corrupt selectedfMap->magic");

      if (lookp) *lookp = selectedfMap ;
      if (seqKeyp) *seqKeyp = selectedfMap->seqKey ;
      if (dnap) *dnap = selectedfMap->dna ;
      if (dnaColp) *dnaColp = selectedfMap->colors ;
      mapIsActive = TRUE ;
    }
  else
    { selectedfMap = 0 ;
      if (lookp) *lookp = 0 ; 
      if (seqKeyp) *seqKeyp = 0 ; 
      if (dnap) *dnap = 0 ; 
      if (dnaColp) *dnaColp = 0 ; 
      mapIsActive = FALSE ;
    }

  return mapIsActive ;
}

Graph fMapActiveGraph (void)
{
  FeatureMap fmap; 

  fMapInitialise() ;
  return (fMapActive (0,0,0,&fmap) ? fmap->graph : 0) ;
}

BOOL fMapActivateGraph (void)
{
  FeatureMap fmap; 

  return (fMapActive (0,0,0,&fmap) && graphActivate (fmap->graph)) ;
}

MAP fMapGetMap (FeatureMap look) /* to get private member */
{ return look->map ;
}

/*****************************************************************/

static void zoom1 (void)
{ FeatureMap look = currentFeatureMap("zoom1") ; look->map->mag = 1 ; fMapDraw(look, 0) ; }

static void zoom10 (void)
{ FeatureMap look = currentFeatureMap("zoom10") ; look->map->mag = 0.1 ; fMapDraw(look, 0) ; }

static void zoom100 (void)
{ FeatureMap look = currentFeatureMap("zoom100") ; look->map->mag = 0.01 ; fMapDraw(look, 0) ; }

static void zoom1000 (void)
{ FeatureMap look = currentFeatureMap("zoom1000") ; look->map->mag = 0.001 ; fMapDraw(look, 0) ; }

/* Only complication is what "zoom_factor" means. If you give a +ve number then you
 * zoom in by this much, -ve means zoom out, but note that if the zoom factor is  < abs(1)  then
 * you end up going the opposite way to what you expect...think about it... */
static void zoomAny(FeatureMap look, float zoom_factor)
{
  if (zoom_factor >= 0)
    look->map->mag *= zoom_factor ;
  else
    look->map->mag /= -(zoom_factor) ;

  fMapDraw(look, KEY_UNDEFINED) ;

  return ;
}

/* simply show whole object */
static void fMapWhole (void)
{ 
  FeatureMap look = currentFeatureMap("fMapWhole") ;

  if (!look->length)					    /* This has to be a cr*p test ???? */
    return ;
 
  look->map->centre = look->length / 2 ;
  look->map->mag = (mapGraphHeight - topMargin - 5) / (1.05 * look->length) ;
  fMapDraw (look, 0) ;

  return ;
}

#ifdef ACEMBLY
extern MENUOPT fMapAcemblyOpts[] ;
static void fMapTraceJoin (void)
{ FeatureMap look = currentFeatureMap("fMapTraceJoin") ;
  messout("This is a menu.  Please use the right mouse button.") ;
}
#else  /* not ACEMBLY */
static void toggleColumnButtons (void)
{
  FeatureMap look = currentFeatureMap("toggleColumnButtons") ;
  look->flag ^= FLAG_COLUMN_BUTTONS ;
  fMapDraw (look, 0) ;
}
#endif /* not ACEMBLY */


/* Set fmap so that it is not reused again, in this case the menu items for  */
/* "Preserve" and "Preserve Cols" are now useless as the fmap will stay      */
/* preserved for ever more, so they are removed from the main menu.          */
static void setDisplayPreserve(void)
{
  FeatureMap look = currentFeatureMap("setDisplayPreserve") ;
  BOOL found ;

  found = removeFromMenu( MENU_FUNC_CAST togglePreserveColumns, look->mainMenu) ;
  found = removeFromMenu(setDisplayPreserve, look->mainMenu) ;

  displayPreserve() ;

  fMapDraw (look, 0) ;

  return ;
}


/* Turns on/off whether DNA mismatches get reported or not, needed because   */
/* sometimes user knows that there are lots of mismatches and doesn't want   */
/* to see them because they are fixing up a link object or some such.        */
static void toggleReportMismatch(MENUITEM unused)
{
  FeatureMap look = currentFeatureMap("toggleReportMismatch") ;
  MENUOPT *m = look->mainMenu;

  look->reportDNAMismatches = !(look->reportDNAMismatches) ;

  toggleTextInMenu(m, toggleReportMismatch,
		   look->reportDNAMismatches ? NO_REPORT_MISMATCHES : REPORT_MISMATCHES) ;

  graphMenu (look->mainMenu) ;

  return;
}


/* Turns on/off whether the whole fmap gets redrawn each time user clicks on */
/* a column to turn it on/off or not.                                        */
/*                                                                           */
static void toggleRedrawColumns(MENUITEM unused)
{
  FeatureMap look = currentFeatureMap("toggleRedrawColumns") ;
  MENUOPT *m = look->mainMenu;

  look->redrawColumns = !(look->redrawColumns) ;

  toggleTextInMenu(m, toggleRedrawColumns,
		   look->redrawColumns ? NOREDRAW_COLS : REDRAW_COLS) ;

  graphMenu (look->mainMenu) ;

  return;
}


/* Changes what fmap puts in the PRIMARY selection buffer, by default its    */
/* DNA but can be toggled to Coordinate information.                         */
/*                                                                           */
static void toggleCutSelection(MENUITEM unused)
{
  FeatureMap look = currentFeatureMap("toggleCutSelection") ;
  MENUOPT *m = look->mainMenu;
  char *menu_txt = NULL ;

  if (look->cut_data == FMAP_CUT_DNA)
    {
      look->cut_data = FMAP_CUT_OBJDATA ;
      menu_txt = CUT_DNA ;
    }
  else
    {
      look->cut_data = FMAP_CUT_DNA ;
      menu_txt = CUT_COORDS ;
    }

  toggleTextInMenu(m, toggleCutSelection, menu_txt) ;

  graphMenu (look->mainMenu) ;

  return;
}


/* Note that toggling the preserve columns options does NOT affect the       */
/* current column settings, it only affects whether the _next_ fmap shown    */
/* will keep the same columns.                                               */
static void togglePreserveColumns(MENUITEM unused)
{
  FeatureMap look = currentFeatureMap("togglePreserveColumns") ;
  MENUOPT *m = look->mainMenu;

  look->preserveColumns = !(look->preserveColumns) ;

  toggleTextInMenu(m, togglePreserveColumns,
		   look->preserveColumns ? UN_PRESERVE_COLS : PRESERVE_COLS) ;

  graphMenu (look->mainMenu) ;

  return;
}


/* toggle menu text - pretty ugly this way, but easiest way
    using the MENUOPT system (new menu system not final yet) */
static void toggleTextInMenu(MENUOPT *m, MENUFUNCTION menu_func, char *menu_text)
{
  while (m->text && m->f)
    {
      if (m->f == (MenuBaseFunction)menu_func)
	{
	  m->text = menu_text ;
	  break ;
	}
      ++m;
    }

  return ;
}

/****************************/

static void fMapShiftOrigin(void)
{
  KEY key ;
  int i,x0 ;
  FeatureMap look = currentFeatureMap("fMapShiftOrigin") ;

  fMapSelect(look) ;

  if (lexword2key (look->originBuf, &key, _VSequence))
    {
      for (i = 1 ; i < arrayMax(look->segs) ; ++i)
	{
	  if (arrp(look->segs,i,SEG)->key == key)
	    {
	      look->origin = arrp(look->segs,i,SEG)->x1 ;
	      goto done ;
	    }
	}

      messout ("Sorry, object %s is not being displayed in this graph", name(key)) ;
      return ;
    }
  else if (sscanf(look->originBuf, "%d", &x0))
    {
      look->origin +=  (x0 - 1) ;  /* -1 Plato dixit  */
      strcpy (look->originBuf, "1") ;
    }
  else
    {
      messout ("Give either a sequence name or a position in the "
	       "current units.") ;
      return ;
    }

 done:
  fMapDraw(look,0) ; /* since i want to rewrite the coordinate system */

  return ;
}


static BOOL fMapSetOriginFromKey (FeatureMap look, KEY key)
{
  int i ;

  for (i = 1 ; i < arrayMax(look->segs) ; ++i)
    {
      if (arrp(look->segs,i,SEG)->key == key)
	{
	  if (look->flag & FLAG_REVERSE) /* top of object is base 1 */
	    look->origin = look->length - arrp(look->segs,i,SEG)->x2 - 1 ;
	  else
	    look->origin = arrp(look->segs,i,SEG)->x1 ;

	  return TRUE ;
	}
    }

  return FALSE ;
}

static void fMapDoPickOrigin (KEY key)
{
  FeatureMap look = currentFeatureMap("fMapDoPickOrigin") ;

  fMapSelect(look) ;

  if (fMapSetOriginFromKey (look, key))
    fMapDraw(look,0) ; /* since i want to rewrite the coordinate system */
  else
    messout ("Sorry, %s is not being displayed in this window",
	     name(key)) ;
}

static void fMapOriginButton (void)
{
  graphRegister (MESSAGE_DESTROY, displayUnBlock) ; /*mhmp 15.05.98*/
  displayBlock (fMapDoPickOrigin, 
		"Pick an object on this window\n"
		"to set the first base of its sequence as base 1.\n"
		"Remove this message to cancel.") ;
}



/*                             ZONE SETTING CODE                                          */

static BOOL setZone (FeatureMap look, int newMin, int newMax)
{ 
  BOOL res = FALSE ;

  look->zoneMin = newMin ;
  if (look->zoneMin < 0)
    { look->zoneMin = 0 ; res = TRUE ; }

  look->zoneMax = newMax ;
  if (look->zoneMax > look->length)
    { look->zoneMax = look->length ; res = TRUE ; }

  return res ;
}

void fmapSetZoneUserCoords (FeatureMap look, int x0, int x1)
{ 
  int tmp ;

  if (look->flag & FLAG_REVERSE)
    {
      tmp = x0 ;
      x0 = look->length - look->origin - x1 ;
      x1 = look->length - look->origin - tmp + 1 ;
    }
  else
    {
      x0 = x0 + look->origin - 1 ;
      x1 = x1 + look->origin ;
    }

  if (x0 > x1)
    { tmp = x0 ; x0 = x1 ; x1 = tmp ;}

  if (setZone (look, x0, x1))
    messout ("The requested zone (%d, %d) extends beyond the sequence "
	     "and has been clipped to (%d, %d).",
	     x0, x1, look->zoneMin, look->zoneMax) ;

  if (look->flag & FLAG_REVERSE)
    strncpy (look->zoneBuf,
	     messprintf ("%d %d",
			 COORD(look, look->zoneMax)+1,
			 COORD(look, look->zoneMin)),
	     15) ;
  else
    strncpy (look->zoneBuf, 
	     messprintf ("%d %d",
			 COORD(look, look->zoneMin),
			 COORD(look, look->zoneMax)-1),
	     15) ;
  if (look->zoneBox) graphBoxDraw (look->zoneBox, -1, -1) ;
  fMapReDrawDNA (look) ;
}

void fMapSetZoneEntry (char *zoneBuf)
{
  int x0, x1 ;
  FeatureMap look = currentFeatureMap("fMapSetZoneEntry") ;

  fMapSelect(look) ;
  if (sscanf(zoneBuf,"%d %d", &x0, &x1) != 2)
    { messout ("The zone must be a pair of integers") ;
      return ;
    }

  fmapSetZoneUserCoords (look, x0, x1) ;

  /* mhmp 02.06.98  & 08.09.98 */
  graphEntryDisable () ;
  graphRegister (KEYBOARD, fMapKbd) ;
}

static void fMapDoSetZone (KEY key)
{
  int i ;
  FeatureMap look = currentFeatureMap("fMapDoSetZone") ;

  fMapSelect(look) ;

  for (i = 1 ; i < arrayMax(look->segs) ; ++i)
    if (arrp(look->segs,i,SEG)->key == key)
      { setZone (look, arrp(look->segs,i,SEG)->x1, 
		       arrp(look->segs,i,SEG)->x2 + 1) ;
	fMapDraw(look,0) ;
	return ;
      }
  messout ("Sorry, sequence %s is not being displayed in this window",
	   name(key)) ;
}

static void fMapSetZoneButton (void)
{
  graphRegister (MESSAGE_DESTROY, displayUnBlock) ; /*mhmp 15.05.98*/
  displayBlock (fMapDoSetZone, 
		"Pick an object in this window\n"
		"to set the limits of the active zone.\n"
		"Remove this message to cancel.") ;
}


BOOL fMapFindZone (FeatureMap look, int *min, int *max, int *origin)
{
  verifyFeatureMap(look, fMapFindZone) ;

  if (min)
    *min = look->zoneMin ;
  if (max)
    *max = look->zoneMax ;
  if (origin)
    *origin = look->origin ;

  return TRUE ;
}


/***********************************/


/*                            FIND SPAN FUNCTIONS                               */


/* Note that there are two functions here :
 *
 * fMapFindSpanSequence - find enclosing sequence
 * fMapFindSpanSequenceWriteable - like fMapFindSpanSequence, but 
 *                         sequence found must be either 
 *                         Genomic_canonical or have the Link
 *			   tag set - this ensures that it is
 *                         suitable for adding extra data to.
 *
 */

BOOL fMapFindSpanSequence(FeatureMap look, KEY *key_out, int *start_out, int *end_out)
{
  BOOL result ;
  int start, end ;

  verifyFeatureMap(look, fMapFindSpanSequence) ;
  messAssert(key_out != NULL && start_out != NULL && end_out != NULL) ;

  /* fmap coords run 0 -> (length - 1), smap coords run 1 -> length, so we
   * must correct input and output coords here. */
  start = *start_out + 1 ;
  end = *end_out + 1 ;

  if ((result  = sMapFindSpan(look->smap, key_out, &start, &end, pred)))
    {
      *start_out = start - 1 ;
      *end_out = end - 1 ;
    }

  return result ;
}

BOOL fMapFindSpanSequenceWriteable(FeatureMap look, KEY *key_out, int *start_out, int *end_out)
{
  BOOL result ;
  int start, end ;

  verifyFeatureMap(look, fMapFindSpanSequenceWriteable) ;
  messAssert(key_out != NULL && start_out != NULL && end_out != NULL) ;

  /* fmap coords run 0 -> (length - 1), smap coords run 1 -> length, so we
   * must correct input and output coords here. */
  start = *start_out + 1 ;
  end = *end_out + 1 ;

  if ((result = sMapFindSpan(look->smap, key_out, &start, &end, predWrite)))
    {
      *start_out = start - 1 ;
      *end_out = end - 1 ;
    }

  return result ;
}

/* Returns TRUE if key is a Sequence class object but does _not_ define a set of exons,
 * FALSE otherwise. */
static BOOL pred(KEY key)
{
  if (class(key) != _VSequence)
    return FALSE;

  if (bIndexTag(key, str2tag("Source_exons")))
    return FALSE;

  return TRUE;
}


/* Returns TRUE if key is a Sequence class object, does _not_ define a set of exons,
 * but is a Genomic or Link sequence object, FALSE otherwise. */
static BOOL predWrite(KEY key)
{
  if (class(key) != _VSequence)
    return FALSE;

  if (bIndexTag(key, str2tag("Source_exons")))
    return FALSE;

  if (bIndexTag(key, str2tag("Genomic_canonical")))
    return TRUE;

  if (bIndexTag(key, str2tag("Link")))
    return TRUE;

  return FALSE;
}



/***********************************/

/* Returns the bottom position of the header.                                */
static float drawHeader(FeatureMap look)
{
  int box, mainButs ;
  float bottom = 0 ;

  look->selectBox = graphBoxStart () ; 
    graphText ("Selected DNA",1.0,.5) ; 
  graphBoxEnd () ;

  if (look == selectedfMap)
    graphBoxDraw (look->selectBox,BLACK,LIGHTRED) ;
  else
    graphBoxDraw (look->selectBox,WHITE,WHITE) ;

  graphButton ("Origin:", fMapOriginButton, 16.5, .4) ;
  if (!strlen(look->originBuf))
    strcpy (look->originBuf,"1") ;
  look->originBox =
    graphTextEntry (look->originBuf, 14, 25, .5, 
		    TEXT_ENTRY_FUNC_CAST fMapShiftOrigin) ;

  graphButton ("Active zone:", fMapSetZoneButton, 41, .4) ;
  if (look->flag & FLAG_REVERSE)
    strncpy (look->zoneBuf,
	     messprintf ("%d %d",
			 COORD(look, look->zoneMax)+1,
			 COORD(look, look->zoneMin)),
	     15) ;
  else
    strncpy (look->zoneBuf, 
	     messprintf ("%d %d",
			 COORD(look, look->zoneMin),
			 COORD(look, look->zoneMax)-1),
	     15) ;
  look->zoneBox =
    graphTextScrollEntry (look->zoneBuf, 23, 14, 54.5, .5, 
			  fMapSetZoneEntry) ;

  /* mhmp 02.06.98 + 08.09.98 */
  graphEntryDisable() ;
  graphRegister (KEYBOARD, fMapKbd) ;			
  if (look->flag & FLAG_COMPLEMENT)
    graphText ("COMPLEMENT", 70, 0) ;
  if (look->flag & FLAG_COMPLEMENT_SUR_PLACE)
    {
      graphText ("COMP_DNA", 81, 0) ;
      graphText ("IN_PLACE", 81, 1) ;
    }
  if (((look->flag & FLAG_COMPLEMENT) && 
       !(look->flag & FLAG_REVERSE)) || 
      (!(look->flag & FLAG_COMPLEMENT) &&
       (look->flag & FLAG_REVERSE)))
    graphText ("REVERSED", 70, 1) ;

  look->segBox = graphBoxStart() ;
  graphTextPtr (look->segTextBuf, 2, 2, 126) ;
  graphBoxEnd() ;
  graphBoxDraw (look->segBox, BLACK, PALEBLUE) ;

  /* default is to put dna in cut buffer, alternative is coord data.         */
  if (look->cut_data == FMAP_CUT_OBJDATA)
    graphPostBuffer(look->segTextBuf) ;

  /* Draw fmap main control buttons and find out how far down they come.     */
  mainButs = graphBoxStart() ;
  box = graphButtons(buttonOpts, 1, 3.2, mapGraphWidth) ;
  graphBoxEnd() ;
  graphBoxDim(mainButs, 0, 0, 0, &bottom) ;

  /* menus for main buttons.                                                 */
#ifdef ACEMBLY

  graphBoxMenu (box + getPositionInMenu(mapZoomIn, buttonOpts), fMapZoomOpts) ;
  graphBoxMenu (box + getPositionInMenu(mapZoomOut, buttonOpts), fMapZoomOpts) ;
  graphBoxMenu (box + getPositionInMenu(dnaAnalyse, buttonOpts), fMapAnalysisOpts) ;
  graphBoxMenu (box + getPositionInMenu(fMapTraceJoin, buttonOpts), fMapAcemblyOpts) ;
  if (look->isGeneFinder)
    graphNewBoxMenu(box + getPositionInMenu(fMapMenuAddGfSegs, buttonOpts),
		    fmapGetGeneOptsMenu()) ;
  else
    graphBoxClear (box + getPositionInMenu(fMapMenuAddGfSegs, buttonOpts)) ;

#else

  mapColMenu (box) ;
  graphBoxMenu (box + getPositionInMenu(mapZoomIn, buttonOpts), fMapZoomOpts) ;
  graphBoxMenu (box + getPositionInMenu(mapZoomOut, buttonOpts), fMapZoomOpts) ;
  graphBoxMenu (box + getPositionInMenu(fMapMenuReverseComplement, buttonOpts), fMapComplementOpts) ;
  graphBoxSetMenu (box + getPositionInMenu(fMapClear, buttonOpts), FALSE); /* no menu */
  graphBoxMenu (box + getPositionInMenu(fMapMenuToggleDna, buttonOpts), fMapDnaOpts) ;
  graphBoxMenu (box + getPositionInMenu(dnaAnalyse, buttonOpts), fMapAnalysisOpts) ;
  graphNewBoxMenu(box + getPositionInMenu(fMapMenuAddGfSegs, buttonOpts), fmapGetGeneOptsMenu()) ;

#endif

  look->minLiveBox = 1 + graphBoxStart() ;
  graphBoxEnd() ;

  return(bottom) ;
}


/********************************************************************/
/* The menu stuff is all being changed, really I need a menu function to do  */
/* these things for me......                                                 */

/* This routine just returns the position (starting at 0) of the menu item   */
/* with a given menu function. It saves us from having to hard code a load of*/
/* menu positions.                                                           */
/* If it can't find the item it returns -1                                   */
static int getPositionInMenu(MenuBaseFunction func, MENUOPT *menu)
{
  int position = -1 ;
  int i = 0 ;

  while(position == -1 && menu[i].f != NULL)
    {
      if (menu[i].f == func)
	position = i ;
      else
	i++ ;
    }

  return position ;
}


/* This function removes an item from a menu by finding it and shuffling up  */
/* all the following items by one place.                                     */
/* If it can't find the item it returns FALSE.                               */
static BOOL removeFromMenu(MenuBaseFunction func, MENUOPT *menu)
{
  BOOL found = FALSE ;
  int i = 0 ;

  /* Find the position of the item.                                          */
  while(found == FALSE && menu[i].f != NULL)
    {
      if (func == menu[i].f)
	found = TRUE ;
      else
	i++ ;
    }

  /* Shuffle up the entries.                                                 */
  if (found == TRUE)
    {
      while(menu[i].text != NULL)
	{
	  menu[i].f = menu[i + 1].f ;
	  menu[i].text = menu[i + 1].text ;
	  i++ ;
	}
    }

  return found ;
}


/********************************************************************/
/* menu functions that manipulate the fMap look of the active graph */
/********************************************************************/

void fMapMenuToggleDna (void)
     /* public - also called from fmaptrace.c */
{
  FeatureMap look = currentFeatureMap("fMapMenuToggleDNA") ;
  fMapToggleDna (look) ;
  fMapDraw (look,0) ; 
}

static void fMapMenuToggleTranslation (void)
{
  FeatureMap look = currentFeatureMap("fMapMenuToggleTranslation") ;
  if (fMapToggleTranslation(look))
    fMapDraw(look,0) ;
}

static void fMapMenuToggleHeader (void)
{
  FeatureMap look = currentFeatureMap("fMapMenuToggleHeader") ;
  fMapToggleHeader(look);
  fMapDraw (look, 0) ;
}

void fMapMenuAddGfSegs (void)
     /* public - also called from fmapgene.c */
{
  FeatureMap look = currentFeatureMap("fMapMenuAddGfSegs") ;
  fMapAddGfSegs(look);		/* includes fMapDraw() */
}

static void fMapMenuDumpSeq (void)
{
  dnaAnalyse() ;
  dnacptDontUseKeySet () ;
  dnacptFastaDump() ;
}

static void fMapMenuReadFastaFile (void)
{ 
  KEY key ;
  FILE *fil ;
  int c ;
  ACEIN fi = 0 ;
  BOOL ok = FALSE ;
  static char directory[DIR_BUFFER_SIZE] = "";
  static char filename[FIL_BUFFER_SIZE] = "";
  char *unused ;

  if (!(fil = filqueryopen (directory, filename, "", "r", 
			    "DNA sequence file in plain or FASTA format")))
    return ;

  c = getc(fil) ;
  ungetc(c,fil) ; /* peek at first character */
  filclose (fil);

  fi = aceInCreateFromFile
    (messprintf("%s/%s", directory, filename), "r", "", 0);

  if (c == '>')
    aceInCard (fi) ;		/* throw out title line */

  lexaddkey ("temp_seq", &key, _VDNA) ;

  if (dnaParse(fi, key, &unused) == PARSEFUNC_OK)
    ok = TRUE ;

  aceInDestroy (fi) ;

  if (ok)
    {
      lexaddkey ("temp_seq", &key, _VSequence) ; /* now display it */
      display (key, 0, "FMAP") ;
    }

  return;
} /* fMapMenuReadFastaFile */


/**********************************************************/

static void fMapToggleDna (FeatureMap look)
{
  mapColSetByName (look->map, "DNA Sequence", 2) ; /* toggle */
  mapColSetByName (look->map, "Coords",  /* set to same state */
		   mapColSetByName (look->map, "DNA Sequence", -1)) ;

  return;
} /* fMapToggleDna */

static BOOL fMapToggleTranslation (FeatureMap look)
{
  int min, max ;
  Array cds, index ;

  if (fMapGetCDS (look, look->translateGene, &cds, &index))
    {
      min = arr(index,0,int) - look->min ; 
      max = arr(index,arrayMax(index)-1,int) - look->min ; 
      if (min > max) 
	mapColSetByName (look->map, "Up Gene Translation", 2) ; 
      else
	mapColSetByName (look->map, "Down Gene Translation", 2) ; 

      return TRUE;
    }

  messout ("Select a gene to translate with the "
	   "pulldown menu on the genes") ;
  return FALSE;
} /* fMapToggleTranslation */


static void fMapToggleHeader (FeatureMap look)
{
  MENUOPT *m = look->mainMenu;

  look->flag ^= FLAG_HIDE_HEADER ;

  /* toggle menu text - pretty ugly this way, but easiest way
   * using the MENUOPT system (new menu system not final yet) */
  while (m->text && m->f)
    {
      if (m->f == fMapMenuToggleHeader)
	{
	  if (look->flag & FLAG_HIDE_HEADER)
	    m->text = "Show Header";
	  else
	    m->text = "Hide Header";
	}
      ++m;
    }

  return;
} /* fMapToggleHeader */

/****************************************************************/

void fMapPleaseRecompute (FeatureMap look) /* set look->pleaseRecompute */
{
  messAssert(verifyFeatureMap(look, fMapPleaseRecompute)) ;

  /* Is this really necessary ? */
  if (graphExists (look->graph))
    look->pleaseRecompute = FMAP_RECOMPUTE_YES ;
    
  /* trace.c may called using a stale look, 
     i do not want to crash on that, the really correct solution
     would be inside fmapdestroy to deregister this look from
     the trace,c code, 
     i do not think it is worth the effort
  */
}

/**********************************************************************/
static int fromTrace = 0, fromTracePos = 0 ;
  /* specialised entry point */
void fMapDrawFromTrace (FeatureMap look, KEY from, int xx, int type)
{
  fMapInitialise() ;

  fromTrace = type ;
  fromTracePos = xx ;
  fMapDraw (look, from) ;
}

static void fMapDrag(float y)
{
  FeatureMap look = currentFeatureMap("drag");

  if (look && look->segBox)
    {
      sprintf(look->segTextBuf, "%d", (int)GRAPH2MAP(fMapGetMap(look),y));
      graphBoxDraw (look->segBox, BLACK, PALEBLUE) ;
    }
  
}

void fMapDraw(FeatureMap look, KEY from)
{
  SEG *seg = 0 ;
  char *cp ;
  int i, fromIndex = 0 ;
  float absMag, x1, x2 ;
  float cols_end = 0 ;

  fMapInitialise() ;

  if (look && look->map && graphActivate(look->graph))
    graphPop() ;
  else
    return ;

  /* If user toggled a column on/off and they don't want the whole fmap      */
  /* redrawn, then just redraw the toggled column button.                    */
  if (mapColButPressed(look->map) && !(look->redrawColumns))
    {
      /* Only redraw the column button if column buttons are displayed.      */
      if (look->flag & FLAG_COLUMN_BUTTONS)
	mapRedrawColBut(look->map) ;

      return ;
    }


  absMag = (look->map->mag > 0) ? look->map->mag : -look->map->mag ;
  
  chrono("fMapDraw") ;


 restart:

  /* There's no key to draw an fmap from so try to use the activeBox instead. */
  if (!class(from)
      && look->activeBox && look->activeBox < arrayMax(look->boxIndex)
      && arr(look->boxIndex, look->activeBox, int) < arrayMax(look->segs)
      && BOXSEG(look->activeBox)->type != DNA_SEQ)
    {
      fromIndex = arr(look->boxIndex, look->activeBox, int) ;
      from = BOXSEG(look->activeBox)->parent ;
    }


  /* clear all DNA highlighting */
  if (arrayExists (look->colors))
    {
      cp = arrp(look->colors, 0, char);
      for( i=0; i<arrayMax(look->colors); i++)
	*cp++ &= ~(TINT_HIGHLIGHT1 |  TINT_HIGHLIGHT2);
    }

  graphWhere(&x1, 0, &x2, 0);
  graphClear () ;
  graphGoto((x2+x1)/2.0, 0.0);


  look->summaryBox = 0 ;

  graphFitBounds (&mapGraphWidth, &mapGraphHeight) ;

  /* Need to set up our very own look menu here....                          */
  graphMenu (look->mainMenu) ;

  /*  if(!look->isOspSelecting) */
  graphRegister (PICK,(GraphFunc) fMapPick) ;

  arrayMax(look->segs) = look->lastTrueSeg ;
  look->boxIndex = arrayReCreate(look->boxIndex,200,int) ;
  look->visibleIndices = arrayReCreate(look->visibleIndices,64,int) ;

  if (look->flag & FLAG_HIDE_HEADER) 
    {
      look->selectBox = look->originBox = look->zoneBox = look->segBox = 0 ;
      look->minLiveBox = 1 ;
      topMargin = 0 ;
    }
  else
    {
      topMargin = drawHeader (look) ;
      memset (look->segTextBuf, 0, 63) ;
    }

  halfGraphHeight = 0.5 * (mapGraphHeight - topMargin) ;

  if (class(from) == _VCalcul)
    {
      i = KEYKEY (from) ;
      look->map->centre = i ;
      from = 0 ;
    }    
  else if (fromTrace && arrayExists(look->segs))
    {
      for (i = 0, seg = arrp(look->segs,0,SEG) ; i < arrayMax(look->segs) ; ++i, ++seg)
        if (from == seg->key)
	  {
	    look->map->centre = seg->x1 + fromTracePos ;
	    break ;
	  }
      if (i == arrayMax(look->segs))
	look->pleaseRecompute = FMAP_RECOMPUTE_YES ;
      else if (fromTrace == 1)
	{ from = 0 ; fromIndex = 0 ; }     /* no highlighting */
    }
  fromTrace = 0 ;


  /* I guess this might happen as a result of one of the database reads from */
  /* above but should never happen as a result of user interaction.          */
  /* mhmp 05.02.98 */
  if (look->map->centre < look->map->min) 
    {
#ifdef FMAP_DEBUG
      printf("Clamping centre.\n");
#endif
      look->map->centre = look->map->min ;
    }
  if (look->map->centre > look->map->max) 
    {
      look->map->centre = look->map->max ;
#ifdef FMAP_DEBUG
      printf("Clamping centre.\n");
#endif
    }

  look->min = look->map->centre - (halfGraphHeight-2.5)/absMag ;
  look->max = look->map->centre + (halfGraphHeight-2.5)/absMag ;

  if (look->pleaseRecompute != FMAP_RECOMPUTE_ALREADY)
    {
#ifdef FMAP_DEBUG
      fMapPrintLook(look, "In fMapDraw");
#endif
      if (look->flag & FLAG_COMPLEMENT)
	{
	  if (look->pleaseRecompute == FMAP_RECOMPUTE_YES
	      || (look->min < 0 && look->stop < look->fullLength-1)
	      || (look->max > look->length && look->start))
	    {
	      look->pleaseRecompute = FMAP_RECOMPUTE_ALREADY ;

	      if (fMapDoRecalculate (look, look->min, look->max))
		goto restart ;
	      else
		{
		  messerror("Could not recalculate, Fmap will be removed.") ;
		  graphDestroy() ;
		  return ;
		}
	    }
	}
      else
	{
	  if (look->pleaseRecompute == FMAP_RECOMPUTE_YES
	      || (look->min < 0 && look->start)
	      || (look->max > look->length && look->stop < look->fullLength-1))
	    {
	      look->pleaseRecompute = FMAP_RECOMPUTE_ALREADY ;
	      if (fMapDoRecalculate (look, look->min, look->max))
		goto restart ;
	      else
		{
		  messerror("Could not recalculate, Fmap will be removed.") ;
		  graphDestroy() ;
		  return ;
		}
	    }
	}
    }
  look->pleaseRecompute = FMAP_RECOMPUTE_NO ;

  if (look->min < 0)
    look->min = 0 ;
  while (look->min % 3)		/* establish standard frame */
    ++look->min ;
  if (look->max >= look->length)
    look->max = look->length ;

  look->map->fixFrame = &look->min ;	/* fiddle for column drawing */
  seqDragBox = 0 ;/* mhmp 07.10.98 */



  /* THIS IS WHERE IT ALL HAPPENS,
   * all the columns now get drawn under control of mapcontrol.c */
  cols_end = mapDrawColumns(look->map) ;

  /* Check whether the bounds of all the columns has changed (e.g. user turned cols on/off)
   * and adjust graph size accordingly. */
  if (cols_end < mapGraphWidth)
    cols_end = mapGraphWidth ;

  graphFitBounds (&mapGraphWidth, &mapGraphHeight) ;


  /* Display column control buttons...this should be in a separate window.   */
  if (look->flag & FLAG_COLUMN_BUTTONS)
    {
      int first_box;
      
      first_box = mapDrawColumnButtons (look->map) ;
      
      /* switch off background menu over these column buttons */
      for (i = 0; i < arrayMax(look->map->cols); ++i)
	graphBoxMenu(first_box + i, NULL);
    }

  /* Try this here... */

  graphTextBounds(cols_end, mapGraphHeight) ;


  look->activeBox = 0 ;

  /* If there was a previous active box or only an active seg (can happen because active box
   * is off screen) then use one of these to select a box and/or highlight its siblings. */
  {
    BOOL box_found = FALSE ;

    /* No currently active box so use last recorded active seg. */
    if (!fromIndex && look->active_seg_index != SEG_INVALID_INDEX)
      {
	for (i = look->minLiveBox ; i < arrayMax(look->boxIndex) ; i++)
	  {
	    int box = arr(look->boxIndex, i, int) ;
	    
	    if (BOXSEG(i) == arrp(look->segs, look->active_seg_index, SEG))
	      {
		fromIndex = box ;
		break ;
	      }
	  }
      }

    /* If there is an active box (i.e. fromIndex > 0) then try to select it,
     * may fail because it may be off the screen. */
    if (fromIndex)
      {
	for (i = look->minLiveBox, box_found = FALSE ; i < arrayMax(look->boxIndex) ; ++i)
	  {
	    if (arr(look->boxIndex, i, int) == fromIndex)
	      {
		fMapSelectBox (look, i, 0, 0) ;
		box_found = TRUE ;
		break ;
	      }
	  }
      }	
    else
      {
	if (class(from))
	  {
	    /* No active box, but there is a "from" key to use, so try to select it. */
	    for (i = look->minLiveBox ; i < arrayMax(look->boxIndex) ; ++i)
	      {
		KEY box_key ;
		
		if (BOXSEG(i)->key)
		  box_key = BOXSEG(i)->key ;
		else
		  box_key = BOXSEG(i)->parent ;
		
		if (box_key == from)
		  {
		    fMapSelectBox (look, i, 0, 0) ;
		    box_found = TRUE ;
		    break ;
		  }
	      }
	  }
	else if (look->active_seg_index != SEG_INVALID_INDEX)
	  {
	    /* No active box, but there is a previous seg to use. */
	    for (i = look->minLiveBox ; i < arrayMax(look->boxIndex) ; ++i)
	      {
		if (BOXSEG(i) == arrp(look->segs, look->active_seg_index, SEG))
		  {
		    fMapSelectBox (look, i, 0, 0) ;
		    box_found = TRUE ;
		    break ;
		  }
	      }
	  }
      }

    /* If we couldn't find the active box, it may still have siblings on the screen
     * that need to be highlighted, must be a parent key to do this sensibly. */
    if (!box_found
	&& look->active_seg_index != SEG_INVALID_INDEX
	&& ((arrp(look->segs, look->active_seg_index, SEG))->parent))
      colourSiblings(look, (arrp(look->segs, look->active_seg_index, SEG))->parent) ;
  }

  chrono("f-graphRedraw") ;

  graphRedraw() ;

  fMapSelect(look) ;
  chronoReturn() ;

#ifdef FMAP_DEBUG
  fMapPrintLook(look, "At end of fMapDraw") ;
#endif



#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
  /* uncomment this to get a report of the total number of boxes drawn for an fmap. */
  printf("total_boxes = %d, minlivebox = %d,  =>  active boxes = %d\n",
	 arrayMax(look->boxIndex),
	 look->minLiveBox,
	 (arrayMax(look->boxIndex) - look->minLiveBox + 1)) ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */




  return ;
} /* fmapDraw */

/************************************************************/
/****************** Registered routines *********************/

void fMapDestroy (FeatureMap look)
{
  verifyFeatureMap(look, fMapDestroy) ;

  handleDestroy (look->handle) ;

  arrayDestroy (look->dna) ;
  arrayDestroy (look->dnaR) ;

  arrayDestroy (look->sites) ;	/* sites come from dnacpt.c */
  stackDestroy(look->siteNames) ;

  arrayDestroy (look->multiplets) ;
  arrayDestroy (look->oligopairs) ;

  mapDestroy (look->map) ;

  /* If this fmap is the "current" (i.e. non-preserved fmap), then remove it from the display
   * systems list of "current" displays. */
  if (look->graph == displayGetGraph("FMAP"))
    displaySetGraph("FMAP", GRAPH_NULL) ;


  if (graphActivate(look->graph))
    {
      graphAssRemove (&GRAPH2FeatureMap_ASSOC) ;
      graphAssRemove (&MAP2LOOK_ASSOC) ;
      graphRegister (DESTROY, 0) ; /* deregister */
    }

  
  fMapTraceDestroy (look) ;
  fMapOspDestroy (look) ;

  /* Sometimes fmap is called to display a temporarily created object, e.g.  */
  /* cDNA display. So now we are finished we need to delete the object.      */
  if (look->destroy_obj)
    {
      OBJ obj ;

      if ((obj = bsUpdate(look->seqKey)) != NULL)
	bsKill(obj) ;
    }


  if (graphActivate (look->selectGraph))   /* from fmapgene.c */
    graphDestroy () ;

  if (look == selectedfMap)
    selectedfMap = 0 ;
  look->magic = 0 ;

  messfree (look) ;

  return;
} /* fMapDestroy */

static void fMapDestroyCallback (void)	/* callback */
{
  FeatureMap look = currentFeatureMap("fMapDestroy") ;
  fMapDestroy (look) ;
} 

/*************************************************************/


/* Sets the start and stop coords for the selected sequence and returns the root
 * parent sequence.
 * Can be called in two ways:
 *     if look is NULL, then just works off the start/end coords and the key,
 *        this happens when we first create an fmap of a sequence.
 *     else tries to keep the active zone etc. in scope for recalculations of.
 *        an existing fmap.
 * Note that the start/stop can be  (n,0)  or  (0,n)  meaning "the sequence from
 * "n" to the end.
 * 
 * Sadly this routine seems to get more opaque as we try to show something sensible...
 * 
 *  */
static BOOL setStartStop(KEY seq, int *start_inout, int *stop_inout, KEY *key_out,
			 BOOL extend, FeatureMap look)
{
  BOOL result = FALSE ;
  int start, stop, length ;
  BOOL swop ;
  KEY *key_ptr ;

  start = *start_inout ;
  stop = *stop_inout ;
  if (key_out != NULL)
    key_ptr = key_out ;
  else
    key_ptr = NULL ;

  /* Only check coords if they are _not_ in form (n,0), in this form they can be passed
   * straight through to sMapTreeRoot(). */
  if (!(start == 0) && !(stop == 0))
    {
      swop = (start > stop) ;				    /* ascending order for easier arithmetic. */
      if (swop)
	{
	  int tmp ;
	  tmp = start, start = stop, stop = tmp ;
	}

      if (!look)
	{
	  if (extend)
	    {
	      int start_out = 0, stop_out = 0 ;
	      KEY root_key = KEY_UNDEFINED ;
	      int max_length ;

	      /* If we are constructing a new look, make it a decent length otherwise user may be
	       * given a ridiculously tiny map. We do this by extending the length in the root
	       * object coordinates to make sure that we don't go off the end, note that this
	       * means we do the final sMapTreeRoot() using the root key only. */
	      if (sMapTreeRoot(seq, start, stop, &root_key, &start_out, &stop_out)
		  && (max_length = sMapLength(root_key)))
		{
		  length = stop - start + 1 ;

		  if (length < 20000)
		    length = 20000 ;

		  start = start_out - length ;
		  stop = stop_out + length ;

		  if (start < 1)
		    start = 1 ;
		  if (stop > max_length)
		    stop = max_length ;

		  seq = root_key ;			    /* all coords in root key. */
		}
	    }
	}
      else
	{
	  /* Logic is: keep length the same or bigger, if start/stop are within an
	   * already calculated bit of sequence then just keep all coords the same,
	   * otherwise slide coords up to be centred around new start/stop. */

	  length = stop - start + 1 ;
	  if (length < look->length)
	    length = look->length ;
      
	  if ((start >= (look->start + 1) && stop <= (look->stop + 1)))
	    {
	      start = look->start + 1 ;
	      stop = look->stop + 1 ;
	    }
	  else
	    {
	      /* This doesn't look correct to me.......this seems to extend the coords,
	       * not slide them all up as the comment implies.... */

	      int extend_len = (length - (stop - start + 1)) / 2 ;
	  
	      start -= extend_len * 2 ;
	      stop += extend_len * 2 ;
	    }
	}


      /* smap coords start at 1 so make sure they do, no need to clamp the stop value, this is
       * done by sMapTreeRoot() */
      if (start < 1)
	start = 1 ;
      
      if (swop)
	{
	  int tmp ;
	  tmp = start, start = stop, stop = tmp ;
	}
    }

  /* Ok now find the actual start/stop for the sequence. */
  if (sMapTreeRoot(seq, start, stop, key_ptr, &start, &stop))
    {
      result = TRUE ;

      *start_inout = start ;
      *stop_inout = stop ;
    }

  return result ;
}



static BOOL fMapDoRecalculate(FeatureMap look, int start, int stop)
{
  BOOL result ;
  int diff, oldMax ;
  BOOL isComplement = (look->flag & FLAG_COMPLEMENT) != 0 ;
  
  /* NB. At this point start and stop are in the co-ordinate system if the
     calculated DNA. Below they get moved into the co-ordinate system of the 
     root object. The same variables are used to store both of these. Magic. */

#ifdef FMAP_DEBUG
  fMapPrintLook(look, "Before recalc.");
#endif

  arrayDestroy (look->dna) ;
  arrayDestroy (look->dnaR) ;
  fMapTraceDestroy (look) ;  
  fMapOspDestroy (look) ;
  sMapDestroy(look->smap);


  /* NB start and stop arguments are in the co-ordinate system of the current fMap:
     This code converts them to the co-ordinate system of the root object. */
  if (isComplement)
    {
      start = look->stop + 2 - start ;
      stop = look->stop + 2 - stop ;
    }
  else
    {
      start += look->start ;
      stop += look->start ;
    }

#ifdef FMAP_DEBUG
  printf("start = %d, stop = %d\n", start, stop);
#endif


  if (!setStartStop(look->seqKey, &start, &stop, NULL, FALSE, look))
    {
      messerror("fMapDoRecalculate() - Could not find start/stop of "
		"key: %s, start: %d  stop: %d", name(look->seqKey), start, stop) ;
      return FALSE ;
    }

#ifdef FMAP_DEBUG
  printf("After setStartStop: start = %d, stop = %d\n", start, stop);
#endif

  look->smap = sMapCreate(look->handle, look->seqKey, start, stop, NULL);

#ifdef FMAP_DEBUG
  {
    ACEOUT dest = aceOutCreateToStdout(0);
    aceOutPrint(dest, "In fMapDoRecalculate()\n") ;
    sMapDump(look->smap, dest);
    aceOutDestroy(dest);
  }
#endif

  reportErrors = look->reportDNAMismatches;
  if (!(look->dna = sMapDNA(look->smap, 0, callback)))
    {
      /* If we wander completely away from the DNA, sMapDNA returns NULL.
	 Since we need something to be able to darw a screen fake it up here.
	 This is not, pretty, but it works. */
      look->dna = arrayHandleCreate(abs(stop - start) + 1, unsigned char, 0);
      arrayMax(look->dna) = abs(stop - start);
    }

  /* sMapDNA might not get full range. */
  look->length = arrayMax(look->dna) ;
  if (isComplement)
    {
      if ((stop - look->length + 1) > start)
	start = stop - look->length + 1;
    }
  else
    {
      if (start + look->length - 1 < stop)
	stop = start + look->length - 1;
    }

  look->colors = arrayReCreate (look->colors, arrayMax(look->dna), char) ;
  arrayMax(look->colors) = arrayMax(look->dna) ;
  fmapSetDNAHightlight(prefColour(DNA_HIGHLIGHT_IN_FMAP_DISPLAY)) ;

  oldMax = look->map->max;
  look->map->max = arrayMax(look->dna) ;

  if (isComplement)
    diff = (start-1) - look->stop ;
  else
    diff = look->start - (start-1) ;

#ifdef FMAP_DEBUG
  printf("Diff  = %d\n", diff );
#endif
  
  look->map->centre += diff ;
  look->zoneMin +=diff ;
  if (look->zoneMin < 0)
    {
#ifdef FMAP_DEBUG
      printf("Clamping zoneMin.\n");
#endif
      look->zoneMin = 0;
    }
  look->zoneMax +=diff ;
  


  {
    /* Change offsets in choosen and antichosen arrays */
    Array vA = arrayCreate(40, char *);
    Array v1A = arrayCreate(40, char *);
    char  *v, *v1;
    int i;
#ifdef FMAP_DEBUG
    printf("Munging assocs.\n");
#endif

    v = 0;
    while (assNext (look->chosen, &v, &v1))
      {
	array(vA, arrayMax(vA), char *) = v;
	array(v1A, arrayMax(v1A), char *) = v1;
#ifdef FMAP_DEBUG
	printf("Chosen: %s: %d,%d (%d, %d)\n", fMapSegTypeName[UNHASH_TYPE(v)],
	       assInt(v1), UNHASH_X2(v), COORD(look, assInt(v1)), COORD(look, UNHASH_X2(v)));
#endif
      }
    look->chosen = assReCreate(look->chosen);
    for (i = 0 ; i < arrayMax(vA) ; ++i)
      {
	v = array(vA, i, char *);
	v1 = array(v1A, i, char *);
	assInsert(look->chosen, HASH(UNHASH_TYPE(v), UNHASH_X2(v) + diff), 
		  assVoid(assInt(v1)+diff));
      }
#ifdef FMAP_DEBUG
    v = 0;
    while (assNext (look->chosen, &v, &v1))
      printf("Chosen-out: %s: %d,%d (%d, %d)\n", fMapSegTypeName[UNHASH_TYPE(v)],
	     assInt(v1), UNHASH_X2(v), COORD(look, assInt(v1)), COORD(look, UNHASH_X2(v)));
#endif

    arrayMax(vA) = 0;
    arrayMax(v1A) = 0;
    
    v = 0;
    while (assNext (look->antiChosen, &v, &v1))
      {
	array(vA, arrayMax(vA), char *) = v;
	array(v1A, arrayMax(v1A), char *) = v1;
      }
    look->antiChosen = assReCreate(look->antiChosen);
    for (i = 0 ; i < arrayMax(vA) ; ++i)
      {
	v = array(vA, i, char *);
	v1 = array(v1A, i, char *);
	assInsert(look->antiChosen, HASH(UNHASH_TYPE(v), UNHASH_X2(v) + diff), 
		  assVoid(assInt(v1)+diff));
      }
    arrayDestroy(vA);
    arrayDestroy(v1A);
  }
    
  if (look->flag & FLAG_REVERSE)
    look->origin = look->length + 1 - look->origin ;
  look->length = arrayMax(look->dna) ;
  look->origin += diff ;
  if (look->flag & FLAG_REVERSE)
    look->origin = look->length + 1 - look->origin ;

#ifdef FMAP_DEBUG
  {
    char *v =0, *v1;
    while (assNext (look->chosen, &v, &v1))
      printf("Chosen-out-originshift: %s: %d,%d (%d, %d)\n", fMapSegTypeName[UNHASH_TYPE(v)],
	     assInt(v1), UNHASH_X2(v), COORD(look, assInt(v1)), COORD(look, UNHASH_X2(v)));
  }
#endif

  if (isComplement)
    { look->start = stop-1 ;
      look->stop = start-1 ;
    }
  else
    { look->start = start-1 ;
      look->stop = stop-1 ;
    }

  if (!fMapConvert(look))
    result = FALSE ;
  else
    {
      /* reset since segs array reorganised */
      look->activeBox = 0 ;

      look->active_seg_index = SEG_INVALID_INDEX ;

      seqDragBox = 0 ;

      result = TRUE ;
    }
#ifdef FMAP_DEBUG
  fMapPrintLook(look, "After recalc.");
#endif

  return result ;
}




static void fMapMenuRecalculate (void)
{
  int start, stop ;
  float absMag ;
  KEY curr ;
  FeatureMap look = currentFeatureMap("fMapMenuRecalculate") ;
  
  if (look->activeBox)
    {
      curr = BOXSEG(look->activeBox)->parent ;
      if (!class(curr))
	curr = 0 ;	/* avoids key being a tag */
    }
  else if (look->active_seg_index != SEG_INVALID_INDEX)
    {
      curr = (arrp(look->segs, look->active_seg_index, SEG))->parent ;
      if (!class(curr))
	curr = 0 ;	/* avoids key being a tag */
    }
  else
    curr = 0 ;


  absMag = (look->map->mag > 0) ? look->map->mag : -look->map->mag ;
  start = look->map->centre - (halfGraphHeight-2)/absMag ;
  stop = look->map->centre + (halfGraphHeight-2)/absMag ;

  /* For some not great reasons the halfGraphHeight stuff can mean that the start/stop
   * can be outside the maximum extent of the fmap so they must be clamped. */
  if (start < 1)
    start = 1 ;
  if (stop > look->fullLength)
    stop = look->fullLength ;

  
  if (fMapDoRecalculate (look, start, stop))
    fMapDraw (look, curr) ;
  else
    {
      messerror("Could not recalculate, Fmap will be removed.") ;
      graphDestroy() ;
    }

  return;
} /* fMapMenuRecalculate */

/***********************************/

static double oldx, oldy, oldx2, oldy2, oldx3, oldx4 ;
static float newx2, newy2 , xdeb, xfin, xlimit ;
static float newx, newy ; /*mhmp 22.04.98 */
static int oldbox ;
static FeatureMap oldlook ;

static void fMapLeftDrag (double x, double y)
{
  graphXorLine (0, oldy, mapGraphWidth, oldy) ;
  oldy = y ;
  graphXorLine (0, y, mapGraphWidth, y) ;
}

static void frameDNA (void)
{
  FeatureMap look = currentFeatureMap("frameDNA") ;

  if (look->flag & FLAG_REVERSE)
    {
      graphXorLine (newx2, newy2, xfin, newy2) ;
      graphXorLine (xfin, newy2, xfin, oldy2+1) ;
      graphXorLine (xfin, oldy2+1, oldx2, oldy2+1) ;
      graphXorLine (oldx2, oldy2+1, oldx2, oldy2) ;
      graphXorLine (oldx2, oldy2, xdeb, oldy2) ;
      graphXorLine (xdeb, oldy2, xdeb, newy2-1) ;  
      graphXorLine (xdeb, newy2-1, newx2, newy2-1) ;
      graphXorLine (newx2, newy2-1, newx2, newy2) ;
    }
  else
    {
      graphXorLine (oldx2, oldy2, xfin, oldy2) ;
      graphXorLine (xfin, oldy2, xfin, newy2-1) ;
      graphXorLine (xfin, newy2-1, newx2, newy2-1) ;
      graphXorLine (newx2, newy2-1, newx2, newy2) ;
      graphXorLine (newx2, newy2, xdeb, newy2) ;
      graphXorLine (xdeb, newy2, xdeb, oldy2+1) ;  
      graphXorLine (xdeb, oldy2+1, oldx2, oldy2+1) ;
      graphXorLine (oldx2, oldy2+1, oldx2, oldy2) ;
    }
}

static void fMapLeftDrag2 (double x, double y)
{
  float newx1, newy1, xx1, xx2, yy1, yy2 ;
  int box ;

  frameDNA() ;
  oldx3 = oldx3 + x - oldx4 ;
  box =graphBoxAt (x, y, &newx1, &newy1) ;

  if  (x < xlimit &&
       (box > oldbox || (box == oldbox && newx1 >= (int)oldx)))
    {
      seqDragBox = box ;
      newx = newx1 ;
      newy = newy1 ;
      graphBoxDim (seqDragBox, &xx1, &yy1, &xx2, &yy2) ;
      newy2 = (int)newy + yy1 + 1;
      newx2 = (int)newx + xx1 + 1;
    }
  oldx4 = x ;
  frameDNA() ;
}
static void fMapLeftDrag3 (double x, double y)
{
  float newx1, newy1, xx1, xx2, yy1, yy2 ;
  int box ;

  frameDNA() ;
  box =graphBoxAt (x, y, &newx1, &newy1) ;
  if  (x < xlimit &&
      (box > oldbox || (box == oldbox && newx1 >= (int)oldx)))
    {
      seqDragBox = box ;
      newx = newx1 ;
      newy = newy1 ;
      graphBoxDim (seqDragBox,&xx1, &yy1, &xx2, &yy2) ;
      /*   xdeb = xx1 ;
      xfin = xx2 ;*/
      newy2 = (int)newy + yy1 + 1;
      newx2 = (int)newx + xx1 + 1;
    }
  frameDNA() ;
}

static void fMapLeftUp (double x, double y)
{
  float pos, pos1, min = 1000000 ;/* mhmp 30.04*/
  int i ;
  SEG *seg ;
  KEY keymin = 0 ;
  FeatureMap look = currentFeatureMap("fMapLeftUp") ;

  graphXorLine (0, oldy, mapGraphWidth, oldy) ;
  graphRegister (LEFT_DRAG, 0) ;
  graphRegister (LEFT_UP, 0) ;

  if (x < (look->map->thumb.x - 1.5))	/* show closest clone or locus! */
    { pos1 =  WHOLE2MAP(look->map, y) ;
      for (i = 1 ; i < arrayMax(look->segs) ; ++i)
	{ seg = arrp(look->segs,i,SEG) ;
	  if (class(seg->key) == _VClone || class(seg->key) == _VLocus)
	    {
	      pos = (seg->x1 + seg->x2)*0.5 ;
	      if (abs (pos - pos1) < min)
		{ min = abs (pos - pos1) ; keymin = seg->key ;}
	    }
	}
      if (keymin)
	{
	  if (class(keymin) == _VClone)
	    display (keymin, 0, "PMAP") ;
	  else
	    display (keymin, 0, "GMAP") ; 
	}
    }

  return;
} /* fMapLeftUp */

static void fMapLeftUp2  (double x, double y)/*mhmp 22.04.98 */
{ 
  frameDNA() ;
  graphRegister (LEFT_UP, 0) ;
  graphRegister (LEFT_DRAG, 0) ;
  fMapSelectBox (oldlook, oldbox, oldx, oldy) ;
}

/***************************************************************/

static void fMapPick (int box, double x, double y, GraphKeyModifier modifier)
{
  float x1,x2,y1,y2 ;
  int xxx, ii ;
  FeatureMap look = currentFeatureMap("fMapPick") ;
  
  if (look != selectedfMap)
    fMapSelect(look) ; 

  if (!box)			/* cursor line */
    {
      graphBoxDim (box, &x1, &y1, &x2, &y2) ;
      y += y1 ;
      oldy = y ;
      graphColor (BLACK) ;
      graphXorLine (0, y, mapGraphWidth, y) ;
      graphRegister (LEFT_DRAG, (GraphFunc)fMapLeftDrag) ;
      graphRegister (LEFT_UP, (GraphFunc)fMapLeftUp) ;
    }
  else if (box == look->map->thumb.box)
    graphBoxDrag (look->map->thumb.box, mapThumbDrag) ;
  else if (box == look->originBox)
    graphTextEntry (look->originBuf,0,0,0,0) ;
  else if (box == look->zoneBox)
    graphTextEntry (look->zoneBuf,0,0,0,0) ;
  else if (box >= look->minLiveBox && box != look->summaryBox)
    {
      /* Box is not one of the command buttons or the yellow summary box but a data box. */

      if ((modifier & CNTL_MODKEY) && (modifier & ALT_MODKEY))
	{
	  /* I'm trying out some debugging aids which will all use Ctrl-Alt-XXX so I will
	   * sift them out first.... */
	  fmapPrintSeg(look, BOXSEG(box)) ;
	}
      else if (box == look->activeBox &&
	  BOXSEG(box)->type != DNA_SEQ &&
	  BOXSEG(box)->type != TRANS_SEQ &&
	  BOXSEG(box)->type != TRANS_SEQ_UP &&
	  BOXSEG(box)->type != PEP_SEQ)
	{
	  fMapFollow (look, x, y) ;
	}
      else if (BOXSEG(box)->type == DNA_SEQ ||
	       BOXSEG(box)->type == PEP_SEQ)
	{
	  graphBoxDim (box,&x1, &y1, &x2, &y2) ; 
	  graphColor (BLACK) ;
	  oldx = x ;
	  oldy = y;
	  oldlook = look ;
	  oldbox = box;
	  oldx2 = (int)x + x1 + 0.0001; /* mhmp 006.10.98 pb d'arrondi */
	  oldy2 = (int)y + y1;
	  newx2 = oldx2 +1;
	  newy2 = oldy2;
	  xfin = x2 ;
	  xlimit = x2 ;
	  xdeb = x1 ;
	  frameDNA() ;
	  oldx3 = oldx ;
	  oldx4 = oldx2 ;
	  fMapLeftDrag3 (oldx2, oldy2) ;
	  graphRegister (LEFT_DRAG, (GraphFunc)fMapLeftDrag3) ;
	  graphRegister (LEFT_UP, (GraphFunc)fMapLeftUp2) ;
	}
      else if ( BOXSEG(box)->type == TRANS_SEQ ||
		BOXSEG(box)->type == TRANS_SEQ_UP)
	{
	  if (look->flag & FLAG_REVERSE)
	    {
	      xxx = COORD (look, BOXSEG(box)->x1) - 3*(int)x ;
	      if (xxx < array(look->maxDna,arrayMax(look->maxDna) - 1 , int))
		return ;
	      for (ii=1; ii<arrayMax(look->maxDna); ii++)
		if (xxx > array(look->maxDna, ii, int))
		  {
		    if (xxx > array(look->minDna, ii-1, int))
		      return ;
		    break ;
		  }
	    }
	  else
	    {
	      xxx = COORD (look, BOXSEG(box)->x1) + 3*(int)x ;
	      if (xxx > array(look->maxDna,arrayMax(look->maxDna) - 1 , int))
		return ;
	      for (ii=1; ii<arrayMax(look->maxDna); ii++)
		if (xxx < array(look->maxDna, ii, int))
		  {
		    if (xxx < array(look->minDna, ii-1, int))
		      return ;
		    break ;
		  }
	    }
	  graphBoxDim (box,&x1, &y1, &x2, &y2) ;
	  graphColor (BLACK) ;
	  oldx = x ;
	  oldy = y;
	  oldlook = look ;
	  oldbox = box;
	  oldx2 = (int)x + x1 + 0.0001; /* mhmp 006.10.98 pb d'arrondi */
	  oldy2 = (int)y + y1;
	  newx2 = oldx2 +1;
	  newy2 = oldy2;
	  xfin = x2 ;
	  xlimit = x2 ;
	  xdeb = x1 ;
	  frameDNA() ;
	  oldx3 = oldx ;
	  oldx4 = oldx2 ;
	  fMapLeftDrag2 (oldx2, oldy2) ;
	  graphRegister (LEFT_DRAG, (GraphFunc)fMapLeftDrag2) ;
	  graphRegister (LEFT_UP, (GraphFunc)fMapLeftUp2) ;
	}
      else			/* not active and not sequence */
	fMapSelectBox (look, box, x, y) ;
    }

  return;
} /* fMapPick */

/***********************************/




/*
 * ----------------------- Keyboard routines -----------------------
 */

/* Implements keyboard actions, usually shortcuts.
 * N.B. sadly the GraphKeyModifier is a bit of a hack because actually at the GraphDev
 * layer it's an int...should really be some sort of convertor from graphDev to Graph.
 * on the todo list...honest.... */
static void fMapKbd(int k, GraphKeyModifier modifier)
{
  FeatureMap look = currentFeatureMap("fMapKbd") ;

  if(look != selectedfMap)
    fMapSelect(look) ; 


  switch (k)
    { 
#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
      /* Needs changes to way we hold the method for segs/homols etc..... */
    case 'b' :
      fMapSwitchColBump(look) ;
      break ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

    case 'd' :
      fMapMenuToggleDna() ;
      break ;

   case 'r' :
      fMapMenuReverseComplement() ;
      break ;

    case 'w' :
      fMapWhole() ;
      break ;

    case '+' :
    case '=' :
    case '-' :
      {
	/* It's really annoying to have to press the shift key to get at the '+' key all
	 * the time so I've made the '=' key do the same as the '+'. */
	float zoom_factor ;
	
	if (modifier & CNTL_MODKEY)
	  zoom_factor = 2 ;
	else
	  zoom_factor = 1.1 ;
	
	if (k == '-')
	  zoom_factor = -(zoom_factor) ;
	
	zoomAny(look, zoom_factor) ;
	break ;
      }
    case PAGE_UP_KEY:
    case PAGE_DOWN_KEY:
      {
	pageUpDown(look, k) ;
	break ;
      }
    case HOME_KEY:
    case END_KEY:
      {
	viewStartEnd(look, k) ;
	break ;
      }
    case SPACE_KEY:
      {
	fMapPleaseRecompute(look) ;

	fMapDraw (look, KEY_UNDEFINED) ;
	break ;
      }
    case RETURN_KEY:
      {
	boxSelect(look) ;

	break ;
      }
    case UP_KEY:
    case DOWN_KEY:
      {
	if (modifier ==  NULL_MODKEY)
	  {
	    creepUpDown(look, k) ;
	  }
	else if (modifier & CNTL_MODKEY)
	  {
	    pageUpDown(look, k) ;
	  }
	else if (modifier & SHIFT_MODKEY)
	  {
	    boxFocus(look, k) ;
	  }
	break ;
      }
    }

  return;
}


static void boxSelect(FeatureMap look)
{
  if (look->activeBox)
    {
      fMapPick(look->activeBox, 0, 0, NULL_MODKEY) ;
    }

  return ;
}

/* there is already a bug here....the routine selects all sorts of boxes that
 * that it shouldn't do....sigh.... */
static void boxFocus(FeatureMap look, int key)
{
  int box ;

  if (look->activeBox)
    {
      box = look->activeBox ;
  
      if (key == UP_KEY)
	{
	  if (box > look->minLiveBox)
	    --box ;
	}
      else
	{
	  if (box < arrayMax(look->boxIndex) - 1)
	    ++box ;
	}
      
      fMapSelectBox(look, box, 0, 0) ;
  
      if (look->activeBox != box)
	fMapFollow(look, 0., 0.) ;
    }

  return ;
}


/* Provides support for PageUp/PageDown keys, a page is the length of the fmap
 * at its current zoom level.
 * Note that this function does not change the zoom so if the current sequence
 * occupies nearly all the screen, these keys will jiggle the sequence up and down
 * endlessly. */
static void pageUpDown(FeatureMap look, int key)
{
  int creep ;

  creep = (look->max - look->min + 1) * 0.75 ;		    /* don't scroll an entire page, it's
							       too confusing. */

  if (key == PAGE_UP_KEY || key ==  UP_KEY)
    creep = -creep ;

  moveUpDown(look, creep) ;

  return ;
}


/* Provides support for arrow up/down keys which move the map up/down by one
 * scale unit which is related to the current zoom level which seems appropriate. */
static void creepUpDown(FeatureMap look, int key)
{
  float unused, subunit ;
  int creep ;

  unused = subunit = 1.0 ;
  mapFindScaleUnit(look->map, &unused, &subunit) ;
  creep = subunit ;

  if (key == UP_KEY)
    creep = -creep ;

  moveUpDown(look, creep) ;

  return ;
}

/* The general move routine, unit should be in the same units as look->min
 * and look->max, i.e. in sequence units, not in floating point screen units.
 * distance is the amount to move, -ve for up, +ve for down. */
static void moveUpDown(FeatureMap look, int distance)
{
  double centre = 0 ;					    /* Should always be >= 0 */
  int half_page ;

  /* We move the FMap by changing the position of the centre relative to the current
   * _top_ or _bottom_ of the map so we need the half length of the map, we also need it for
   * ceasing movement at either end of the sequence. */
  half_page = (look->max - look->min + 1) / 2 ;

  /* When we move we clamp movement to the maximum extent of this FMap. */
  if (distance == 0)
    {
      ;							    /* No movement so do nothing. */
    }
  if (distance < 0)
    {
      if (look->min > 0)
	{
	  centre = look->min + half_page + distance ;
	  
	  if (centre < half_page)
	    centre = half_page ;
	}
    }
  else
    {
      int bottom = look->fullLength - 1 ;

      if (look->max < bottom)
	{
	  centre = look->max - half_page + distance ;
	  
	  if (centre > (bottom - half_page))
	    centre = (bottom - half_page) ;
	  
	}
    }


  /* Only redraw if centre has been recalculated. */
  if (centre > 0)
    {
      look->map->centre = centre ;
      fMapDraw (look, KEY_UNDEFINED) ;
    }

  return ;
}



/* Function for moving to start/end of currently calculated view in response to
 * user pressing Home/End keys. */
static void viewStartEnd(FeatureMap look, int key)
{
  double centre = 0 ;					    /* Should always be >= 0 */
  int half_page ;

  /* We move the FMap by changing the position of the centre relative to the current
   * top or bottom, i.e. by half a page. */
  half_page = (look->max - look->min + 1) / 2 ;

  /* Remember, currently calculated section varies from:  0 -> (look->length - 1) */
  if (key == HOME_KEY)
    {
      if (look->min > 0)
	{
	  centre = half_page ;
	}
    }
  else
    {
      if (look->max < (look->length - 1))
	{
	  centre = look->length - half_page ;
	}
    }


  /* Only redraw if centre has been recalculated. */
  if (centre > 0)
    {
      look->map->centre = centre ;
      fMapDraw (look, KEY_UNDEFINED) ;
    }

  return ;
}





/************** little function to act as a callback ***********/

static void fMapRefresh (void)
{
  FeatureMap look = currentFeatureMap("fMapRefresh") ;

  fMapDraw (look, 0) ;
}

/*********************************************************/
/*********************************************************/


/* Check the length of the sequence to be viewed, main use of this is to     */
/* give the user a chance to stop if they have clicked on a huge sequence.   */
/*                                                                           */
/* Threshold for asking user if they want to continue is currently set at    */
/* a MBase.                                                                  */
/*                                                                           */
static BOOL checkSequenceLength(KEY seq)
{
  BOOL result = FALSE ;
  KEY parent = KEY_UNDEFINED ;
  int start = 0, stop = 0 ;
  enum {MAX_SEQ_SPAN = 1048576} ;

  if (!(result = sMapTreeRoot(seq, 1, 0, &parent, &start, &stop)))
    messerror("Cannot map this object.") ;
  else
    {
      int len = abs(stop - start) + 1 ;

      if (len > MAX_SEQ_SPAN)
	result = messQuery("You have asked to view a sequence that is %d bases long,"
			   " do you really want to continue ?", len) ;
      else
	result = TRUE ;
    }

  return result ;
}


/*********************************************************/

/* Callback routine for handling DNA mismatches. */
static consMapDNAErrorReturn callback(KEY key, int position)
{
  BOOL OK;

  if (!reportErrors)
    return sMapErrorReturnContinue;

  OK = messQuery("Mismatch error at %d in %s.\n"
		 "Do you wish to see further errors?", 
		 position, name(key));
  
  return OK ? sMapErrorReturnContinue : sMapErrorReturnSilent;
}


/* Create an fmap virtual sequence. */
static FeatureMap fMapCreateLook(KEY seq, int start, int stop, BOOL reuse, BOOL extend, BOOL noclip,
				 FmapDisplayData *display_data)
{
  FeatureMap look ;
  KEY seqOrig = seq ;


  /* do again - no problem if done already in fMapDisplay */
  if (!setStartStop(seq, &start, &stop, &seq, extend, NULL))
    {
      messerror("fMapCreateLook() - Could not find start/stop of key: \"%s\", start: %d  stop: %d",
		name(seq), start, stop) ;

      return NULL ;
    }


  look = (FeatureMap) messalloc (sizeof(struct FeatureMapStruct)) ;
  look->magic = &FeatureMap_MAGIC ;
  look->handle = handleCreate () ;
  look->segsHandle = handleHandleCreate (look->handle) ;
  look->seqKey = seq ;
  look->seqOrig = seqOrig ;


#ifdef FMAP_DEBUG
  printf("In create: start = %d, stop = %d\n", start, stop);
#endif


  /* Can either get mappings with features clipped to start, stop or get _all_ features that
   * overlap start/stop and hence extend beyond start/stop */
  if (noclip)
    {
      int begin, end ;


#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
      /* I think that the following is correct, if the start/stop are reversed then we
       * should be reversing the  */
      if (start < stop)
	begin = 1, end = 0 ;
      else
	begin = 0, end = 1 ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

      /* not sure if the above screws up things.... */
      begin = 1, end = 0 ;


      look->smap = sMapCreateEx(look->handle, look->seqKey, begin, end, start, stop, NULL) ;
    }
  else
    look->smap = sMapCreate(look->handle, look->seqKey, start, stop, NULL);



#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
  {
    ACEOUT dest = aceOutCreateToStdout(0);
    aceOutPrint(dest, "In fMapCreateLook()\n") ;
    sMapDump(look->smap, dest);
    aceOutDestroy(dest);
  }
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */




  if (look->smap)
    {
      look->noclip = noclip ;
    }
  else
    {
      messerror("Could not construct smap, key: %s, start: %d  stop: %d", look->seqKey, start, stop) ;
      return NULL ;
    }


  /* This can go once all old dna stuff is removed. */
  look->reportDNAMismatches = FALSE ;


  if (display_data)
    {
      look->keep_selfhomols = display_data->keep_selfhomols ;
      look->keep_duplicate_introns = display_data->keep_duplicate_introns ;
    }


  /* test code....                                                           */
  if (display_data && display_data->methods_dict)
    {
      look->method_set = sMapFeaturesSet(look->handle,
					  display_data->methods_dict,
					  display_data->include_methods); 
      if (!look->method_set)
	return NULL;

      look->include_methods = display_data->include_methods;
    }
  else
    {
      look->include_methods = FALSE; 
    }

  if (display_data && display_data->sources_dict)
    {
      look->sources_dict = display_data->sources_dict;
      look->include_sources = display_data->include_sources;
    }


  reportErrors = look->reportDNAMismatches;
  if (!(look->dna = sMapDNA(look->smap, 0, callback)))
    {
      messfree (look->handle) ;
      messfree (look) ;
      return NULL ;
    }



  /* start/stop can be reversed at this stage for reverse sequence genes... */
  look->zoneMin = 0 ;
  if (stop > start)
    look->zoneMax = stop - start + 1 ;
  else
    look->zoneMax = start - stop + 1 ;
  look->origin = 0 ;

  /* OK, now set start/stop to represent the length of sequence asked for originally. */
  stop = start + arrayMax(look->dna) - 1; 


  
  look->colors = arrayHandleCreate (arrayMax(look->dna), char, look->handle) ;
  arrayMax(look->colors) = arrayMax(look->dna) ;
  fmapSetDNAHightlight(prefColour(DNA_HIGHLIGHT_IN_FMAP_DISPLAY)) ;
  fMapClearDNA (look) ;		/* clear colors array to _WHITE */
  
  look->chosen = assHandleCreate (look->handle) ;
  look->antiChosen = assHandleCreate (look->handle) ;

  look->length = arrayMax(look->dna) ;
  look->fullLength = sMapLength (seq) ;

  look->start = start-1;
  look->stop = stop-1;


  look->featDict = dictHandleCreate (1000, look->handle) ;
  dictAdd (look->featDict, "null", 0) ; /* so real entries are non-zero */

  look->seqInfo = arrayHandleCreate (32, SEQINFO, look->handle) ;
  look->homolInfo = arrayHandleCreate (256, HOMOLINFO, look->handle) ;
  look->feature_info = arrayHandleCreate(256, FeatureInfo, look->handle) ;

  look->homolBury = bitSetCreate (256, look->handle) ;
  look->homolFromTable = bitSetCreate (256, look->handle) ;

  look->visibleIndices = arrayHandleCreate (64, int, look->handle) ;
  look->preserveColumns = TRUE ;
  look->redrawColumns = TRUE ;
  if (display_data)
    look->destroy_obj = display_data->destroy_obj ;
  else
    look->destroy_obj = FALSE ;
  
  look->mcache = methodCacheCreate (look->handle);
  
  /* must create map before convert, because it adds columns */
  look->map = mapCreate (fMapRefresh) ; /* map redraw func */
  look->map->drag = fMapDrag;
    
    
  /* no graph perhaps, so do not attach to graph! */
  fMapDefaultColumns (look->map) ;


  /* Optionally create a maptable which keeps a hash of all features mapped allowing quick
   * look up of mapped features for operations like GFFv3 dumping. */
  if (display_data && display_data->maptable)
    {
      look->mapped_dict = dictHandleCreate(4096, look->handle) ;
    }

  
  if (!fMapConvert(look))
    {
      messfree (look->handle) ;
      messfree (look) ;

      return NULL ;
    }


  look->boxIndex = arrayHandleCreate (64, int, look->handle) ;
  look->activeBox = 0 ;
  look->active_seg_index = SEG_INVALID_INDEX ;
  seqDragBox = 0 ;
  
  /* Some items of the main menu are specific to an individual look  */
  /* so we must copy the main menu so we can alter it.               */
  look->mainMenu = handleAlloc(NULL, look->handle, FMAPMENU_SIZE) ;
  memcpy(look->mainMenu, fMapMenu, FMAPMENU_SIZE) ;
  
  /* Some menu items need to be set according to current program     */
  /* state, e.g. cut data state can be set from command line.        */
  if (fMapCutCoords_G == FMAP_CUT_OBJDATA)		    /* call will reverse what we set here. */
    look->cut_data = FMAP_CUT_OBJDATA ;
  else
    look->cut_data = FMAP_CUT_DNA ;
  toggleTextInMenu(look->mainMenu, toggleCutSelection,
		   (look->cut_data == FMAP_CUT_DNA) ? CUT_COORDS : CUT_DNA) ;
  
  look->flag = 0 ;
  look->flag |= FLAG_AUTO_WIDTH ;  /* I honestly cannot understand why you would take that away */

 
  return look ;
} /* fMapCreateLook */

/************************************************************/

static void fMapAttachToGraph (FeatureMap look)
{ 
  graphAssociate (&GRAPH2FeatureMap_ASSOC, look) ; /* attach look for fmap package */
  graphAssociate (&MAP2LOOK_ASSOC, look) ;       /* attach look for map package */
}

/************************************************************/

/* this is being used a DisplayFunc called by display() */
BOOL fMapDisplay (KEY key, KEY from, BOOL isOldGraph, void *app_data)
{
  FmapDisplayData *create_data = (FmapDisplayData *)app_data ;
  int start, stop ;
  FeatureMap look = NULL, curr_look = NULL ;
  KEY  seq = 0 ;
  OBJ  obj ;
  BOOL shiftOrigin = FALSE ;
  BOOL reuse, reverse_strand ;
  char *window_title = NULL ;
  BOOL key_maps_from ;

  fMapInitialise() ;

  if (isOldGraph)
    curr_look = currentFeatureMap("fMapDisplay") ;

  if (class(key) == _VSequence || bIndexTag (key, str2tag("SMAP")))
    seq = key ;
  else if (class(key) == _VDNA)
    lexReClass (key, &seq, _VSequence) ;
  else if ((obj = bsCreate (key)))
    { 
      if ((!bsGetKey (obj, _Sequence, &seq))
	  && bsGetKey (obj, str2tag("Genomic_sequence"), &seq))
	shiftOrigin = TRUE ; 
      bsDestroy (obj) ;
    }

  if (key != seq)
    {
      from = key ;
      key = seq ;
    }

  if (!seq || iskey(seq) != 2)	/* iskey tests a full object */
    return FALSE ;


  /* Crude check of size of object to be displayed, gives user a chance to
   * stop if they accidentally try to display a huge sequence. */
  if (!isGifDisplay && !checkSequenceLength(seq))
    {
      return FALSE ;
    }

  /* recurse up to find topmost link - must do here rather than
   * wait for fMapCreateLook() so we can check if we have it already */
  sMapTreeRoot (seq, 1, 0, &seq, &start, &stop) ;

  /* smap keeps coords in the original order but fmap needs them to be in
   * ascending order. */
  if (start > stop)
    {
      int tmp ;
      tmp = start ;
      start = stop ;
      stop = tmp ;

      reverse_strand = TRUE ;
    }
  else
    reverse_strand = FALSE ;


  /* Reuse the current fmap graph ? */
  if (isOldGraph && graphAssFind (&GRAPH2FeatureMap_ASSOC, &look))
    reuse = TRUE ;
  else
    reuse = FALSE ;

  /* there is still a wrinkle to sort out here in that really we should make use of
   * key_maps_from in fMapCentre() as part of deciding zooming level. */
  if ((key_maps_from = keyMapsFrom(key, from)))
    {
      /* Swop "key" and "from" if "from" is actually mapped by "key". */
      KEY tmp = KEY_UNDEFINED ;

      window_title = g_strdup(name(key)) ;

      tmp = key ;
      key = from ;
      from = tmp ;
    }
  else if (reuse)
    {
      KEY kk = from ? from : key ;
      if (class(kk) == _VSequence || class(kk) == _VTranscribed_gene)
	window_title  = g_strdup(name(kk)) ;
      else if (from || look->seqKey)
	window_title = g_strdup((name(from ? from : look->seqKey))) ;
    }
  else
    window_title = g_strdup(name(from ? from : key)) ;


  /* Is the requested part of the fMap already on display ? */
  if (reuse && look && look->seqKey == seq
      && arrayExists (look->dna) && start-1 >= look->start && stop-1 <= look->stop)
    {
      look->seqOrig = key ;
      fMapCentre (look, key, from) ;
      if (shiftOrigin)
	fMapSetOriginFromKey (look, from) ;
      graphRetitle(window_title) ;

      if (reverse_strand && !FMAP_COMPLEMENTED(look))	    /* revcomp for up strand features. */
	revComplement(look) ;

        if (!reverse_strand && FMAP_COMPLEMENTED(look))	    /* revcomp for up strand features. */
	revComplement(look) ;


      fMapDraw (look, key) ;

      return TRUE ;
    }

  /* make a new look, can fail if key cannot be fmap'd or user aborts drawing. */
  look = fMapCreateLook (seq, start, stop, reuse, TRUE, FALSE, create_data) ;
  if (!look)
    {
      return FALSE ;
    }

  /* If the fmap is being reused the default is now to keep the      */
  /* currently selected active columns the same and to display       */
  /* the column buttons if they were being displayed.                */
  /* Otherwise column settings will revert to the default and the    */
  /* column buttons will not be displayed.                           */
  if (reuse)
    {
      if (curr_look->preserveColumns == TRUE)
	{
	  look->preserveColumns = TRUE ;
	  mapColCopySettings(curr_look->map, look->map) ;
	  
	  if (curr_look->flag & FLAG_COLUMN_BUTTONS)
	    look->flag |= FLAG_COLUMN_BUTTONS ;
	}
      else
	{
	  look->preserveColumns = FALSE ;
	  look->flag &= !FLAG_COLUMN_BUTTONS ;
	}
      
      /* Set menu texts for settings preserved between fmap recalcs. */
      toggleTextInMenu(look->mainMenu, togglePreserveColumns,
		       look->preserveColumns ? UN_PRESERVE_COLS : PRESERVE_COLS) ;
      /* This needs to be cleared up and integrated with new smapdna stuff... */
      toggleTextInMenu(look->mainMenu, toggleReportMismatch,
		       (look->reportDNAMismatches
			? NO_REPORT_MISMATCHES : REPORT_MISMATCHES)) ;
    }


  if (reuse)
    {
      graphRetitle(window_title) ;
      fMapDestroyCallback () ;				    /* kill look currently attached */
      graphRegister (DESTROY, fMapDestroyCallback) ;	    /* reregister */
      graphClear () ;
    }
  else							    /* open a new window */ 
    { 
      displayCreate ("FMAP");
      graphRetitle(window_title) ;
      graphRegister (RESIZE, fMapRefresh) ;
      graphRegister (DESTROY, fMapDestroyCallback) ;
      graphRegister (KEYBOARD, fMapKbd) ;
      graphRegister (MESSAGE_DESTROY, displayUnBlock) ;
    }

  look->seqOrig = key ;
  look->map->min = 0 ;
  look->map->max = look->length ;

  look->graph = graphActive() ;
  fMapAttachToGraph (look) ;
  mapAttachToGraph (look->map) ;

  fMapcDNAInit (look) ;


  /* centre on key if possible and choose magnification */
  fMapCentre (look, key, from) ;
  if (shiftOrigin)
    fMapSetOriginFromKey(look, from) ;

  if (!getenv("ACEDB_PROJECT") && (look->zoneMax - look->zoneMin < 1000))
    {
      mapColSetByName(look->map, "DNA Sequence", TRUE) ;
      mapColSetByName(look->map, "Coords", TRUE) ;
    }

#ifdef FMAP_DEBUG
  fMapPrintLook(look, "fMapDisplay");
#endif

  if (reverse_strand && !FMAP_COMPLEMENTED(look))	    /* revcomp for up strand features. */
    revComplement(look) ;

  fMapDraw(look, key) ;

  fMapSelect(look) ; 


  /* Clean up. */
  if (window_title)
    g_free(window_title) ;


  return TRUE ;
}



/************************************************/
/************************************************/
    /* also called from fmapgene.c */
int fMapOrder (void *a, void *b)     
{
  SEG *seg1 = (SEG*)a, *seg2 = (SEG*)b ;
  int diff ;

  diff = seg1->type - seg2->type ;
  if (diff > 0)
    return 1 ;
  else if (diff < 0)
    return -1 ;

  if ((seg1->type | 0x1) == SPLICED_cDNA_UP)
    {
      if (!(seg1->source * seg2->source))
 	return seg1->source ? -1 : 1 ;

      diff = seg1->source - seg2->source ;
      if (diff)
	return diff ;

      diff = seg1->parent - seg2->parent ;
      if (diff)
	return diff ;

      {
	int x1, x2, y1, y2 ;
	x1 = seg1->data.i >> 14 & 0x3FFF ;
	x2 = seg1->data.i & 0x3FFF ;
	y1 = seg2->data.i >> 14 & 0x3FFF ;
	y2 = seg2->data.i & 0x3FFF ;
	
	if ((x1 - x2) * (y1 - y2) < 0)
	  return x1 < x2 ? -1 : 1 ;
      }
    }
    
  diff = seg1->x1 - seg2->x1 ;
  if (diff > 0)
    return 1 ;
  else if (diff < 0)
    return -1 ;

  diff = seg1->x2 - seg2->x2 ;
  if (diff > 0)
    return 1 ;
  else if (diff < 0)
    return -1 ;

  return 0 ;
}

/*****************************************************************/



/* This routine returns TRUE if the "from" object contains a reference to the "key"
 * object according to rules encapsulated in this routine, returns FALSE otherwise.
 * 
 * It used to be mostly true that sequence objects referred to objects that were their
 * children. It has become desireable though for some objects to refer to objects that
 * are their parents. An example would be an object that is a homology and not directly
 * SMap'd itself, but has an xref to the object that is its parent and _is_ SMap'd. When
 * the user displays one of these objects that are not directly mapped in tree display,
 * they will want to click the xref'd object and see the tree display object displayed 
 * in an fmap within the context of the xref'd object.
 * 
 * This routine will probably have to be updated as new ways of cross referencing between
 * objects are added. */
static BOOL keyMapsFrom(KEY key, KEY from)
{
  BOOL result = FALSE ;
  OBJ obj = NULL ;
  KEY source = KEY_UNDEFINED ;

  messAssert(!(key == KEY_UNDEFINED && from == KEY_UNDEFINED)) ;

  if (key == KEY_UNDEFINED || from == KEY_UNDEFINED)
    {
      result = FALSE ;
    }
  if (bIndexTag(from, str2tag("Source"))
      && (obj = bsCreate(from))
      && bsGetKeyTags(obj, str2tag("Source"), &source)
      && source == key)
    {
      result = TRUE ;
    }
  else if (bIndexTag(from, str2tag("S_Parent"))
	   && (obj = bsCreate(from))
	   && bsGetKeyTags(obj, str2tag("S_Parent"), NULL)
	   && bsGetKey(obj, _bsRight, &source)
	   && source == key)
    {
      result = TRUE ;
    }
  else if (class(key) == pickWord2Class("Homol_data"))
    {
      /* Expected tags for homol entry are:
       * Homol <tag2> <obj> <method> Float Int UNIQUE Int Int UNIQUE Int #Homol_info
       * in which we just look in "key" for an <obj> that is "from" */
      if (bIndexTag(key, str2tag("Homol")))
	{
	  OBJ obj = NULL ;
	  KEY homol = KEY_UNDEFINED, tag2 = KEY_UNDEFINED ;
	  BSMARK mark1 = 0 ;
	  BOOL found = FALSE ;

	  obj = bsCreate(key) ;

	  if (bsGetKeyTags(obj, str2tag("Homol"), &tag2)) do
	    {
	      mark1 = bsMark(obj, mark1);

	      if (bsGetKey(obj, _bsRight, &homol)) do
		{ 
		  if (homol == from)			    /* got it ! */
		    found = TRUE ;
		} while (found == FALSE && bsGetKey(obj, _bsDown, &homol));

	      bsGoto(obj, mark1);
	    } while (found == FALSE && bsGetKeyTags(obj, _bsDown, &tag2));

	  bsDestroy(obj) ;
	  result = found ;
	}
    }


  return result ;
}


/* Centre the fmap on "key", perhaps using "from" to decide zoom.
 * 
 * This is an old comment:
 *       "need key and from for Calcul positioning" */
static void fMapCentre(FeatureMap look, KEY key, KEY from)
{ 
  int i ;
  float nx, ny ;
  SEG *seg ;
  int top = INT_MAX, bottom = 0 ;

  graphFitBounds (&mapGraphWidth,&mapGraphHeight) ;
  halfGraphHeight = 0.5 * (mapGraphHeight - topMargin) ;
  nx = mapGraphWidth ;
  ny = mapGraphHeight - topMargin - 5 ;			    /* WHY IS IT  "- 5" ???????? */


  /* find top/bottom of feature to be mapped, default to master seg. */
  seg = NULL ;
  if (key)
    {
      SEG *seg1 ;

      for (i = 1 ; i < arrayMax (look->segs) ; ++i)
	{
	  seg1 = arrp (look->segs, i, SEG) ;

	  if (seg1->key == key && seg1->type <= ALLELE_UP)  /* Only use uppable tags to get extent. */
	    {
	      if (top > seg1->x1)
		top = seg1->x1 ;
	      if (bottom < seg1->x2)
		bottom = seg1->x2 ;
	      seg = seg1 ;
	    }
	}
    }

  if (seg == NULL)
    {
      seg = arrp(look->segs, 0 , SEG) ;
      top = seg->x1 ;
      bottom = seg->x2 ;
    }


  /* Centre on object. */
  i = KEYKEY(from)*10 ;
  if (class(from) == _VCalcul  &&  i < seg->x2 - seg->x1 + 1)
    {
      if (seg->type == SEQUENCE_UP)
	look->map->centre = seg->x2 - i ;
      else
	look->map->centre = seg->x1 + i ;
   /* Go to rather high magnification 
      look->map->mag = (ny-5)/ 5000.0 ; */
    }
  else
    look->map->centre = 0.5 * (top + bottom) ;

  /* Set zoom for object. */
  look->map->mag = ny/(bottom - top + 1.0) ;

  /* This seems hokey...if the segs are correct then this shouldn't happen.... */
  if (look->map->mag <= 0)  /* safeguard */
    look->map->mag = 0.05 ;

  /* If "from" probably contains "key" then zoom out slightly for better view of
   * "key" in context. */
  if (from)
    {
      if (class(from) == _VSequence
	  || (_VTranscribed_gene && class(from) == _VTranscribed_gene)
	  || (_VIST && class(from) == _VIST)
	  || (_VProtein && class(from) == _VProtein) /* so that protein homol get centered */
	  || (class(from) == pickWord2Class("Homol_data")))
	look->map->mag /= 1.4 ;
    }

  look->origin = top ;

  /* Bizarrely the zone is held as going from 0 -> length as opposed to 0 -> (length - 1)
   * or 1 -> length, as segs are 0-based we must increment "bottom" */
  setZone(look, top, (bottom + 1)) ;

  return ;
}


/*****************************************************************/

static void processMethod (FeatureMap look, KEY key, float offset,
			   MapColDrawFunc func)
{ 
  METHOD *meth = NULL;
  float p ;

  if (!(meth = methodCacheGet(look->mcache, key, "")))
    /* Note the string (now "") will eventually come from the 
     * aceDiff-Text in the view object for this FMAP */
    return;

  /* Some methods need to not be displayed so we don't even put them in a    */
  /* column....don't know if this is correct really.                         */
  if (meth->no_display)
    return ;

  if (meth->priority)
    p = meth->priority ;
  else if (meth->flags & METHOD_FRAME_SENSITIVE)
    p = 4 + offset ;
  else
    p = 5 + offset ;

  mapInsertCol (look->map, p, TRUE, name(key), func) ;

  if (meth->flags & METHOD_SHOW_UP_STRAND)
    mapInsertCol (look->map, -p, TRUE,
		  messprintf("-%s", name(key)), func) ;

  return;
} /* processMethod */

void fMapProcessMethods (FeatureMap look)
{
  int i;
  KEY key;

  if (!look->map)
    return ;
 
  for (i = 0 ; i < arrayMax(look->seqInfo) ; ++i)
    {
      key = arrp(look->seqInfo, i, SEQINFO)->method ;
      if (key)
	processMethod (look, key, -3.0, fMapShowSequence) ;
    }

  for (i = 0 ; i < arrayMax(look->homolInfo) ; ++i)
    {
      key = arrp(look->homolInfo, i, HOMOLINFO)->method ;
      if (key)
	processMethod (look, key, 0.5, fMapShowHomol) ;
    }

  for (i = 0 ; i < arrayMax(look->feature_info) ; ++i)
    {
      key = arrp(look->feature_info, i, FeatureInfo)->method ;
      if (key)
	processMethod (look, key, -2.5, fMapShowFeatureObj) ;
    }

  /* some segs have their seg->key set to their method, add these to the methods set. */
  for (i = 0 ; i < arrayMax(look->segs) ; ++i)
    {
      key = arrp(look->segs, i, SEG)->key ;
      if (class(key) == _VMethod)
	{
	  switch (arrp(look->segs, i, SEG)->type)
	    { 
	    case FEATURE: case FEATURE_UP: 
	      processMethod (look, key, 0.7, fMapShowFeature) ; 
	      break ;
	    case SPLICE5: case SPLICE5_UP:
	    case SPLICE3: case SPLICE3_UP:
	      processMethod (look, key, 0.9, fMapShowSplices) ; 
	      break ;
	    case TRANSCRIPT: case TRANSCRIPT_UP:
	      processMethod (look, key, 2.1, fMapcDNAShowTranscript) ; 
	      break ;
	    case SPLICED_cDNA: case SPLICED_cDNA_UP:
	      processMethod (look, key, 2.2, fMapcDNAShowSplicedcDNA) ; 
	      break ;
	    case ATG: case ATG_UP:
	      break ;		/* do nothing yet */
	    default:
	      messerror ("Method key %s for unknown seg type %s", 
			 name(key), 
			 fMapSegTypeName[arrp(look->segs, i, SEG)->type]) ;
	    }
	}
    }
  
  return;
} /* fMapProcessMethods */

/*****************************/


/* selective for CALCULATED segs */
static void addOldSegs (Array segs, Array oldSegs, MethodCache mcache)
{
  SEG *seg, *oldMaster, *newMaster ;
  int i, j, length, offset ;
  METHOD *meth;

  oldMaster = arrp(oldSegs,0,SEG) ;
  newMaster = arrp(segs,0,SEG) ;
  if (newMaster->key != oldMaster->key)
    return ;

  length = newMaster->x2 - newMaster->x1 + 1 ;
  offset = newMaster->x1 - oldMaster->x1 ;

  j = arrayMax(segs) ;
  for (i = 1 ; i < arrayMax(oldSegs) ; ++i)
    {
      seg = arrp(oldSegs, i, SEG) ;
      switch (seg->type)
	{
	case FEATURE: case FEATURE_UP:
	case ATG: case ATG_UP:
	case SPLICE5: case SPLICE5_UP:
	case SPLICE3: case SPLICE3_UP:
	  if (class(seg->key) != _VMethod)
	    messcrash ("Non-method key in addOldSegs") ;

	  meth = methodCacheGet(mcache, seg->key, "");
	  if (meth && meth->flags & METHOD_CALCULATED)
	    { 
	      seg->x1 -= offset ;  
	      seg->x2 -= offset ;
	      if (seg->x1 >= 0 && seg->x2 < length)
		array(segs, j++, SEG) = *seg ;
	    }
	  break ;
	default:		/* needed for picky compiler */
	  break ;
	}
    }

  return;
} /* addOldSegs */

KEY defaultSubsequenceKey (char* name, int colour, 
				  float priority, BOOL isStrand)
{
  KEY key ;
  OBJ obj ;
  
  lexaddkey (name, &key, _VMethod) ;  /* mieg, dec 27 */
  if (iskey(key)!=2) /* i.e. no object */
    if ((obj = bsUpdate (key)))	/* make it */
      { float width = 2 ;
        KEY colourKey = _WHITE + colour ;

        if (bsAddTag (obj, str2tag("Colour")))
	  { 
	    bsPushObj (obj) ;	/* get into sub-model #Colour */
	    bsAddTag (obj, colourKey) ;
	    bsGoto (obj, 0) ;
	  }
	bsAddData (obj, str2tag("Width"), _Float, &width) ;
	bsAddData (obj, str2tag("Right_priority"), _Float, &priority) ;
	if (isStrand) bsAddTag (obj, str2tag("Show_up_strand")) ;
	bsSave (obj) ;
      }
  return key ;
}

BOOL fMapFindCoding (FeatureMap look)
{
  BOOL result = TRUE ;
  int i, j, j1 ;
  Array index = NULL ;
  SEG *seg ;
  KEY parent ;


  for (i = 1 ; i < arrayMax(look->segs) ; ++i)
    {
      /* User can interrupt by pressing F4                                   */
      if (messIsInterruptCalled())
	{
	  if (messQuery("Finding coding - do you really want to interrupt fmap ?"))
	    {
	      if (index != NULL)
		arrayDestroy(index) ;
	      result = FALSE ;
	      break ;
	    }
	  else
	    messResetInterrupt() ;
	}

      seg = arrp(look->segs,i,SEG) ;

      if (seg->type == CDS && (parent = seg->parent) &&
	  fMapGetCDS (look, parent, 0, &index))
	{
	  j1 = 0 ;
	  for (j = 0 ; j < arrayMax(index) ; ++j)
	    if (j == arrayMax(index) - 1 ||
		arr(index,j+1,int) > arr(index,j,int)+1) /* boundary */
	      {
		seg = arrayp (look->segs, arrayMax(look->segs), SEG) ;
		seg->key = _CDS ;
		seg->type = CODING ;
		seg->x1 = arr(index, j1, int) ; 
		seg->x2 = arr(index, j, int) ;
		seg->parent = parent ;
		seg->data.i = j1 ;
		j1 = j+1 ;
	      }
	  arrayDestroy (index) ;
	  index = NULL ;
	}
      else if (seg->type == CDS_UP && (parent = seg->parent) &&
	       fMapGetCDS (look, parent, 0, &index))
	{
	  j1 = 0 ;
	  for (j = 0 ; j < arrayMax(index) ; ++j)
	    if (j == arrayMax(index) - 1 ||
		arr(index,j+1,int) < arr(index,j,int)-1) /* boundary */
	      {
		seg = arrayp (look->segs, arrayMax(look->segs), SEG) ;
		seg->key = _CDS ;
		seg->type = CODING_UP ;
		seg->x1 = arr(index,j,int) ; 
		seg->x2 = arr(index,j1,int) ;
		seg->parent = parent ;
		seg->data.i = j1 ;
		j1 = j+1 ;
	      }
	  arrayDestroy (index) ;
	  index = NULL ;
	}

      /* remove duplicate INTRON segs */
      if (seg->type == INTRON || seg->type == INTRON_UP)
	{
	  SEG *seg2 = seg ;
	  int flags = 0 ;
	  BOOL existsParent = FALSE ;
	  SEQINFO *sinf ;

	  while (i < arrayMax(look->segs)-1 && seg->type == seg2->type &&
		 seg->x1 == seg2->x1 && seg->x2 == seg2->x2)
	    {
	      sinf = arrp(look->seqInfo, seg->data.i, SEQINFO) ;
	      flags |= sinf->flags ;
	      if (seg->parent)
		existsParent = TRUE ;
	      ++i ; ++seg ;
	    }

	  for (--i, --seg ; seg2 <= seg ; ++seg2)
	    if (seg2->parent || !existsParent)
	      {
		sinf = arrp(look->seqInfo, seg2->data.i, SEQINFO) ;
		if (sinf->flags != flags)
		  {
		    sinf = arrayp(look->seqInfo, arrayMax(look->seqInfo), SEQINFO) ;
		    *sinf = arr(look->seqInfo, seg2->data.i, SEQINFO) ;
		    sinf->flags = flags ;
		    seg2->data.i = arrayMax(look->seqInfo)-1 ;
		  }
		existsParent = TRUE ;
	      }
	    else
	      {
		*seg2 = array(look->segs,arrayMax(look->segs)-1,SEG) ;
		--arrayMax(look->segs) ;
	      }
	}
    }


  return result ;
}
      
/************************************************/ 

/* NOTE, this small routine makes use of the new smap convert code to        */
/* make the array of segs. Eventually this routine should merge/replace      */
/* fMapConvert(). If you look at fMapConvert() you will see that that it     */
/* calls this routine and then returns immediately making the rest of        */
/* fMapConvert redundant.                                                    */
/*                                                                           */
/*                                                                           */
BOOL fMapSMapConvert(FeatureMap look, Array oldSegs)
{
  BOOL result = FALSE ;

  /* This routine now encapsulates the convert.                              */
  result = sMapFeaturesMake(look->smap, look->seqKey, look->seqOrig, look->start, 
			    look->stop, look->length,
			    oldSegs, (look->flag & FLAG_COMPLEMENT),
			    look->mcache, look->mapped_dict,
			    look->dna, look->method_set, look->include_methods, look->featDict, 
			    look->sources_dict, look->include_sources,
			    look->segs,
			    look->seqInfo, look->homolInfo, look->feature_info,
			    look->homolBury, look->homolFromTable,
			    look->keep_selfhomols, look->keep_duplicate_introns,
			    look->segsHandle);


  if (result)
    {
      /* The comment about these 2 routines not working is deeply distressing    */
      /* I'm not sure if its true or what....                                    */

      fMapProcessMethods (look) ;			    /* always do this - to pick up edits */
      arraySort(look->segs, fMapOrder) ;
      /* these 2 routines no longer work after reverse complement because RC calls sort
       * they should be written differently
       */

      fMapOspFindOligoPairs (look) ;    /* why  AFTER sorting ? not sure, but probably ok */
      look->lastTrueSeg = arrayMax(look->segs) ;

      if (fMapDebug)
	dumpstats(look) ;
    }

  return result;
}


/* This routine needs to be merged with the above routine to give a new      */
/* fmapconvert routine.                                                      */
/*                                                                           */
BOOL fMapConvert(FeatureMap look)
{
  Array segs = NULL ;
  Array oldSegs = NULL ;
  STORE_HANDLE oldSegsHandle ;
  OBJ	obj ;
  int	i, nsegs;
  KEY M_Transposon, M_SPLICED_cDNA, M_RNA ;
  KEY M_TRANSCRIPT, M_Pseudogene, M_Coding ;
  static Array units = 0 ;
  BOOL result = FALSE ;


  chrono("fMapConvert") ;
  
  fMapInitialise () ;

  M_Coding = defaultSubsequenceKey ("Coding", BLUE, 2.0, TRUE) ;
  M_RNA = defaultSubsequenceKey ("RNA", GREEN, 2.0, TRUE) ;
  M_Pseudogene = defaultSubsequenceKey ("Pseudogene", DARKGRAY, 2.0, TRUE) ;
  M_Transposon = defaultSubsequenceKey ("Transposon", DARKGRAY, 2.05, FALSE) ;
  M_TRANSCRIPT = defaultSubsequenceKey ("TRANSCRIPT", DARKGRAY, 2.1, FALSE) ;
  M_SPLICED_cDNA = defaultSubsequenceKey ("SPLICED_cDNA", DARKGRAY, 2.2, FALSE) ;

  if (!look || !look->seqKey)
    messcrash ("No sequence for fMapConvert") ;

  if (look->pleaseRecompute == FMAP_RECOMPUTE_YES)
    look->pleaseRecompute = FMAP_RECOMPUTE_NO ;

  if (!(obj = bsCreate (look->seqKey)))
    {
      messout ("No data for sequence %s", name(look->seqKey)) ;
      chronoReturn() ;
      return FALSE ;
    }
  bsDestroy (obj) ;


  messStatus ("Recalculating DNA window") ;

  /* Initialise handles, arrays and bitsets.                                 */
  oldSegs = look->segs ;
  oldSegsHandle = look->segsHandle ;
  look->segsHandle = handleHandleCreate (look->handle) ;
  segs = look->segs = arrayHandleCreate(oldSegs ? arrayMax(oldSegs) : 128,
					SEG, look->segsHandle) ;
  if (oldSegs)
    arrayMax(oldSegs) = look->lastTrueSeg ;

  units = arrayReCreate (units, 256, BSunit) ;
  for (i=0; i<arrayMax(look->homolInfo); i++)
    arrayDestroy(arr(look->homolInfo, i, HOMOLINFO).gaps);

  look->seqInfo = arrayReCreate (look->seqInfo, 32, SEQINFO) ;
  look->homolInfo = arrayReCreate (look->homolInfo, 256, HOMOLINFO) ;
  look->feature_info = arrayReCreate(look->feature_info, 256, FeatureInfo) ;

  look->homolBury = bitSetReCreate (look->homolBury, 256) ;
  look->homolFromTable = bitSetReCreate (look->homolFromTable, 256) ;
  nsegs = 0 ;						    /* Main seg index. */


  result = fMapSMapConvert(look, oldSegs);

  if (oldSegs)
    { 
      handleDestroy(oldSegsHandle) ;			    /* kills old segs and related stuff */
      oldSegs = NULL ;
    }

  return result ;
}

/*****************************************************************/

static char *title (KEY key)
{
  OBJ obj ;
  char *result ;
  KEY textKey ;

  result = name(key) ;

  if ((obj = bsCreate(key)))
    { if (bsGetKey (obj, _Title, &textKey))
	result = name(textKey) ;
      bsDestroy (obj) ;
    }

  return result ;
}

/**********************************************************/

/* Implements the line at the top of FMap which displays information about the
 * currently selected object.
 * 
 * If you add to this line you should be aware that segTextBuf is
 * defined as "segTextBuf[512]", i.e. a 511 char limit.
 * 
 *  */
void fMapReportLine (FeatureMap look, SEG *seg, BOOL isGIF, float x)
{ 
  KEY obj_key = KEY_UNDEFINED ;
  char *cp ;
  HOMOLINFO *hinf ;
  SEQINFO *sinf ;
  METHOD *meth ;
  int i, i2, c1, ii ;
  SEG *seg2 ;  
  Array tt = NULL ;

  /* ALL THIS STRNCAT STUFF NEEDS REDOING....WE SHOULD HAVE AN UNLIMITED BUFFER OR
   * A PROPERLY LIMITED ONE...SIGH..... */

  memset(look->segTextBuf, 0, 63) ;

  /* Get feature name. */
  if (isGIF)
    {
      *look->segTextBuf = 0 ;
    }
  else if (iskey(seg->key))
    {
      KEY tmp_key ;

      tmp_key = obj_key = seg->key ;

      /* Gets the public_name key in an object if there is one. */
      tmp_key = geneGetPubKey(obj_key) ;

      strncpy (look->segTextBuf, nameWithClassDecorate(tmp_key), 40) ;
      look->segTextBuf[40] = '\0';
    }
  else if (seg->parent && iskey(seg->parent))
    {
      obj_key = seg->parent ;
      strncpy (look->segTextBuf, nameWithClassDecorate(seg->parent), 40) ;
      look->segTextBuf[40] = '\0';
    }
  else
    {
      *look->segTextBuf = 0 ;
    }
  

  /* Feature coordinates */
  if (look->flag & FLAG_REVERSE)
    strcat (look->segTextBuf, 
	    messprintf ("    %d %d (%d)  ", 
			COORD(look, seg->x2), COORD(look, seg->x1),
			1 - COORD(look, seg->x2) + COORD(look, seg->x1))) ;
  else
    strcat (look->segTextBuf, 
	    messprintf ("    %d %d (%d)  ", 
			COORD(look, seg->x1), COORD(look, seg->x2),
			1 - COORD(look, seg->x1) + COORD(look, seg->x2))) ;
  

  /* Additional information. */
  if (seqDragBox)
    seg2 = BOXSEG (seqDragBox) ;
  else
    seg2 = seg ;

  switch (seg->type)
    {
    case SEQUENCE: case SEQUENCE_UP:
    case EXON: case EXON_UP:
    case INTRON: case INTRON_UP:
      {
	sinf = arrp (look->seqInfo, seg->data.i, SEQINFO) ;
	if (sinf->method)
	  strncat (look->segTextBuf, messprintf(" %s ", name(sinf->method)), 40) ;
	if (sinf->flags & SEQ_SCORE)
	  strncat (look->segTextBuf, messprintf ("%.1f", sinf->score), 10) ;


#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
	/* At some time someone requested this but I can't remember who,
	 * I've removed it because it can cause a buffer overrun of segTextBuf. */

	/* confirmed intron stuff. */
	if (sinf->flags & SEQ_CONFIRMED)
	  {
	    int i ;

	    strcat(look->segTextBuf, "Confirmed") ;

	    for (i = 0 ; i < arrayMax(sinf->confirmation) ; i++)
	      {
		ConfirmedIntronInfo confirm = arrayp(sinf->confirmation, i,
						     ConfirmedIntronInfoStruct) ;
	      
		strcat(look->segTextBuf, messprintf("  %s %s", confirm->confirm_str,
						    (confirm->confirm_sequence
						     ? name(confirm->confirm_sequence) : ""))) ;
	      }
	  }
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */


	break ;
      }
    case DNA_SEQ:
      i = seg->x1 + (int)x ;
      i2 = seg2->x1 + (int)newx ;
      if (look->flag & FLAG_REVERSE)
	strcat (look->segTextBuf, 
		messprintf ("Selection %d ---> %d (%d)", 
			    COORD(look, i2), COORD(look, i),
			  1 - COORD(look, i2)+ COORD(look, i))) ;
      else
	strcat (look->segTextBuf, 
		messprintf ("Selection %d ---> %d (%d)", 
			    COORD(look, i), COORD(look, i2),
			    1 + COORD(look, i2)- COORD(look, i))) ;
      if (look->gf.cum && i >= look->gf.min && i < look->gf.max)
	strncat (look->segTextBuf,
		 messprintf ("  cumulative coding score %.2f",
			     look->gf.cum[i - look->gf.min]), 64) ;
      break ;
    case PEP_SEQ:
      {
	if ((tt = pepGetTranslationTable(obj_key, 0)))
	  {
	    i = seg->x1 + 3*(int)x ;
	    i2 = seg2->x1 + 3*(int)newx ;
	    if ((cp = pepName[(int)e_codon(arrp(look->dna, i, char), tt)]))
	      {
		if (look->flag & FLAG_REVERSE)
		  strncat (look->segTextBuf, 
			   messprintf ("%s at %d to %d Selection weight: %d",cp,
				       COORD(look, i+2), COORD(look,i), weight), 64) ;
		else
		  strncat (look->segTextBuf, 
			   messprintf ("%s at %d to %d Selection weight: %d",cp,
				       COORD(look, i), COORD(look,i+2), weight), 64) ;
	      }
	  }
	break ;
      }
    case TRANS_SEQ:
      {
	if ((tt = pepGetTranslationTable(obj_key, 0)))
	  {
	    i = seg->x1 + 3*(int)x ;
	    c1 =  COORD(look, i) ;
	    if ((cp = pepName[(int)e_codon(arrp(look->dna, i, char), tt)]))
	      {
		if (look->flag & FLAG_REVERSE)
		  { 
		    if(!seqDragBox) nbTrans = 1 ;
		    strncat (look->segTextBuf,
			     messprintf ("%s at %d ", cp, c1 -  nbTrans + 1), 64) ;
		    if (seqDragBox && c1 <= array(look->minDna, 0, int))
		      {
			for (ii=1; ii<arrayMax(look->maxDna); ii++)
			  if (c1 > array(look->maxDna, ii, int) &&
			      c1 <= array(look->minDna, ii- 1, int))
			    { 
			      c1 = (array(look->decalCoord, ii-1, int) - c1)/3 ;
			      break ;
			    }
			strcat (look->segTextBuf, 
				messprintf (" Selection %d ---> %d (%d) weight: %d", 
					    c1, c1 + nbTrans - 1, nbTrans, weight)) ;
		      }
		  }
		else
		  {
		    strncat (look->segTextBuf, 
			     messprintf ("%s at %d ", cp, c1), 64) ;
		    if (seqDragBox && c1 >= array(look->minDna, 0, int))
		      {
			for (ii=1; ii<arrayMax(look->maxDna); ii++)
			  if (c1 < array(look->maxDna, ii, int) &&
			      c1 >= array(look->minDna, ii- 1, int))
			    { 
			      c1 = (c1 - array(look->decalCoord, ii-1, int))/3 ;
			      break ;
			    }
			strcat (look->segTextBuf, 
				messprintf ( " Selection %d ---> %d (%d) weight: %d", 
					     c1, c1 + nbTrans - 1, nbTrans, weight)) ;
		      }
		  }
	      }
	  }
	break ;
      }
    case TRANS_SEQ_UP:
      {
	if ((tt = pepGetTranslationTable(obj_key, 0)))
	  {
	    i = seg->x1 + 3*(int)x ;
	    c1 =  COORD(look, i) ;
	    if ((cp =  pepName[(int)e_reverseCodon(arrp(look->dna, i-2, char), tt)]))
	      {
		if (look->flag & FLAG_REVERSE)
		  {
		    if(!seqDragBox) nbTrans = 1 ;
		    strncat (look->segTextBuf, 
			     messprintf ("up-strand %s at %d ", cp, c1 + nbTrans - 1), 64) ;
		    if (seqDragBox && c1 <= array(look->minDna, 0, int))
		      {
			for (ii=1; ii<arrayMax(look->maxDna); ii++)
			  if (c1 > array(look->maxDna, ii, int) &&
			      c1 <= array(look->minDna, ii- 1, int) )
			    { 
			      c1 = (c1 - array(look->decalCoord, ii-1, int))/3 ;
			      break ;
			    }
			strcat (look->segTextBuf, 
				messprintf (" Selection %d ---> %d (%d) weight: %d", 
					    c1, c1 - nbTrans + 1, nbTrans, weight)) ;
		      }
		  }	  
		else
		  {
		    strncat (look->segTextBuf, 
			     messprintf ("up-strand %s at %d ", cp, c1), 64) ;
		    if (seqDragBox && c1 >= array(look->minDna, 0, int))
		      {
			for (ii=1; ii<arrayMax(look->maxDna); ii++)
			  if (c1 < array(look->maxDna, ii, int) &&
			      c1 >= array(look->minDna, ii- 1, int))
			    { 
			      c1 = (array(look->decalCoord, ii-1, int) - c1)/3 ;
			      break ;
			    }
			strcat (look->segTextBuf, 
				messprintf ( " Selection %d ---> %d (%d) weight: %d", 
					     c1, c1 - nbTrans + 1, nbTrans, weight)) ;
		      }
		  }
	      }
	  }
	break ;
      }
    case ORF:
      strncat (look->segTextBuf, messprintf ("Frame %d", seg->data.i), 64) ;
      break ;
    case SPLICE3: case SPLICE5: case ATG:
      strncat (look->segTextBuf, fMapSegTypeName[seg->type], 32) ;
      strncat (look->segTextBuf, messprintf (" Score %f", seg->data.f), 32) ;
      break ;
    case ASSEMBLY_TAG:
      strncat (look->segTextBuf, messprintf ("%s", seg->data.s), 64);
      break;
    case ALLELE: case ALLELE_UP: 
      {
	char *alternate_name ;

	if ((alternate_name = pickGetAlternativeName(obj_key, "Public_name")))
	  strncat(look->segTextBuf, messprintf("%s", alternate_name), 64) ;
	else if (seg->data.s)
	  strncat (look->segTextBuf, messprintf ("%s", seg->data.s), 64) ;


	break ;
      }
    case EMBL_FEATURE: case EMBL_FEATURE_UP:
      if (seg->data.s)
	strncat (look->segTextBuf, messprintf ("%s", seg->data.s), 64) ;
      break ;
    case HOMOL: case HOMOL_UP:
      hinf = arrp(look->homolInfo, seg->data.i, HOMOLINFO) ;
      meth = methodCacheGet(look->mcache, hinf->method, "") ;
      if (meth->flags & METHOD_BLASTN)
	strncat (look->segTextBuf, 
		 messprintf("%s %.1f %d%% (%d - %d) ",
			    name(hinf->method),
			    hinf->score,
			    (int) (100 * (4 + hinf->score/(seg->x2-seg->x1+1)) / 9.0),
			    hinf->x1, hinf->x2),
		 64) ;
      else
	strncat (look->segTextBuf, 
		 messprintf("%s %.1f (%d - %d) ",
			    name(hinf->method),
			    hinf->score,
			    hinf->x1, hinf->x2),
		 64) ;
      if (!isGIF)		/* title() requires DB access */
	strncat (look->segTextBuf, title(seg->key), 
		 127 - strlen(look->segTextBuf)) ;
      break ;
    case FEATURE: case FEATURE_UP:
      meth = methodCacheGet(look->mcache, seg->key, "") ;
      if (meth->flags & METHOD_PERCENT)
	strcat (look->segTextBuf, messprintf ("%.0f%% ", seg->data.f)) ;
      else if (meth->flags & METHOD_SCORE)
	strcat (look->segTextBuf, messprintf ("%.3g ", seg->data.f)) ;
      if (seg->parent)
	strncat (look->segTextBuf, dictName (look->featDict, -seg->parent), 58) ;
      break ;
    case CODING:
      if (look->gf.cum && 
	  seg->x1 >= look->gf.min && seg->x2 < look->gf.max)
	strncat (look->segTextBuf,
		 messprintf ("Coding score %.2f",
			     look->gf.cum[seg->x2 + 1 - look->gf.min] -
			     look->gf.cum[seg->x1 - look->gf.min]), 64) ;
      break ;
    case SPLICED_cDNA: case SPLICED_cDNA_UP:
     	fMapcDNAReportLine (look->segTextBuf,seg,64) ;
      break ;
    case TRANSCRIPT: case TRANSCRIPT_UP:
     	fMapcDNAReportTranscript (look->segTextBuf,seg,64) ;
      break ;
    case VIRTUAL_MULTIPLET_TAG:  
      fMapSelectVirtualMultiplet (look, seg) ;
      break ;
    case CLONE_END:
      strncat (look->segTextBuf, nameWithClassDecorate(seg->data.k), 64) ; /* left/right */
      break ;

    case OLIGO: case OLIGO_UP:

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
      i =  (seg->data.i >> 16) & 0x3ff ;
      if (i > 0)
	strcat (look->segTextBuf, messprintf (" Tm: %3.1f ", (float)(i/10.0))) ;
      i =  seg->data.i & 0xfff ;
      if (i > 0)
	strcat (look->segTextBuf, messprintf (" Score: %d ", i)) ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */
      i =  (seg->feature_data.oligo_type >> 16) & 0x3ff ;
      if (i > 0)
	strcat (look->segTextBuf, messprintf (" Tm: %3.1f ", (float)(i/10.0))) ;
      i =  seg->feature_data.oligo_type & 0xfff ;
      if (i > 0)
	strcat (look->segTextBuf, messprintf (" Score: %d ", i)) ;

      break ;

    case OLIGO_PAIR: case OLIGO_PAIR_UP:
      fMapOspSelectOligoPair (look, seg) ;
      break ;
    case VISIBLE:
      if (iskey (seg->data.k))
	strcat (look->segTextBuf, messprintf (" (%s) ", nameWithClassDecorate(seg->data.k))) ; 
      break ;
    default:		/* needed for picky compiler */
      break ;
    }
}

/**********************************************************/

static void fMapSelectBox (FeatureMap look, int box, double x, double y)
{ 
  int i, i0, max , imax, w=0 ;
  SEG *seg, *seg2, *seg3;
  KEY obj_key = KEY_UNDEFINED ;
  KEY parent ;
  unsigned char *cp ;
  char *buf, *cq, *cq1 ;
  HOMOLINFO *hinf ;
  METHOD *meth ;
  static int finDna = 0 ;
  int ii, c1 ;
  BOOL isProt ;
  int nExon=0, tframe, oldTframe=0, diffTframe ;
  BOOL isTiret, calcWeight ;
  BOOL doComplementSurPlace = (look->flag & FLAG_COMPLEMENT_SUR_PLACE) ;

  if (box >= arrayMax(look->boxIndex) || box < look->minLiveBox)
    return ;

  if (look->activeBox)
    {
      FeatureInfo *feat ;

      seg = BOXSEG(look->activeBox) ;
      parent = seg->parent ;
      for (i = look->minLiveBox ; i < arrayMax(look->boxIndex) ; ++i)
	{
	  seg2 = BOXSEG(i) ;
	  if (seg2 == seg || (parent && seg2->parent == parent))
	    switch (seg2->type)
	      {
	      case DNA_SEQ: case PEP_SEQ: case TRANS_SEQ: case TRANS_SEQ_UP:
		break ;
	      case HOMOL: case HOMOL_UP:
		hinf = arrp(look->homolInfo, seg2->data.i, HOMOLINFO) ;
		meth = methodCacheGet(look->mcache, hinf->method, "") ;
		graphBoxDraw (i, -1, meth->colour) ;
		break ;
	      case FEATURE: case FEATURE_UP:
		meth = methodCacheGet(look->mcache, seg2->key, "") ;
		graphBoxDraw (i, -1, meth->colour) ; 
		break ;
	      case FEATURE_OBJ: case FEATURE_OBJ_UP:
		feat = arrp(look->feature_info, seg2->data.i, FeatureInfo) ;
		meth = methodCacheGet(look->mcache, feat->method, "") ;
		graphBoxDraw (i, -1, meth->colour) ; 
		break ;
	      case ASSEMBLY_TAG:
		{
		  int color ;
		  unsigned char *c = (unsigned char*) seg2->data.s;
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
		  graphBoxDraw (i, BLACK, color) ;
		}
		break;
	      case INTRON: case INTRON_UP:
		if (arrp(look->seqInfo,seg2->data.i,SEQINFO)->flags & SEQ_CONFIRMED)
		  graphBoxDraw (i, -1, 
				(arrp(look->seqInfo,seg2->data.i,SEQINFO)->flags
				 & SEQ_CONFIRM_UTR ? YELLOW : GREEN)) ;
		else
		  graphBoxDraw (i, -1, WHITE) ;
		break ;
	      case OLIGO: case OLIGO_UP:

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
		if (seg2->data.i & 0x4000)      /* old */
		  graphBoxDraw (i, -1, ORANGE) ;
		else if (seg2->data.i & 0x8000) /* selected */
		  graphBoxDraw (i, -1, GREEN) ;
		else
		  graphBoxDraw (i, -1, WHITE) ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */
		if (seg2->feature_data.oligo_type & 0x4000)      /* old */
		  graphBoxDraw (i, -1, ORANGE) ;
		else if (seg2->feature_data.oligo_type & 0x8000) /* selected */
		  graphBoxDraw (i, -1, GREEN) ;
		else
		  graphBoxDraw (i, -1, WHITE) ;

		break ;
	      default:
		/* Includes exon drawing.                                    */
		if (assFind (look->chosen, SEG_HASH(seg2), 0))
		  graphBoxDraw (i, -1, GREEN) ;
		else if (assFind (look->antiChosen, SEG_HASH(seg2), 0))
		  graphBoxDraw (i, -1, LIGHTGREEN) ;		    
		else
		  graphBoxDraw (i, -1, WHITE) ;
		break ;
	      }
	}
      i = seg->x1 ;  /* mhmp 11.06.98 + 08.09.98 */   
      if (look->isRetro)
	i -=2 ;
      if (i <=  0) i = 0 ;
      if (!finDna)
	finDna = (seg->x2 >= look->length) ? look->length - 1 : seg->x2 ;
      if (look->colors)
	{
	  cp = arrp(look->colors, i, unsigned char) ;
	  for ( ; i <= finDna && i < arrayMax(look->colors) ; ++i)
	    *cp++ &= ~(TINT_HIGHLIGHT1 | TINT_HIGHLIGHT2);
	}
      for (i = look->minLiveBox ; i < arrayMax(look->boxIndex) ; ++i)
	{ seg2 = BOXSEG(i) ;
	  if ((seg2->type == DNA_SEQ || seg2->type == PEP_SEQ || 
	       seg2->type == TRANS_SEQ || seg2->type == TRANS_SEQ_UP) && 
	      seg2->x1 <= finDna && seg2->x2 >= seg->x1)
	    graphBoxDraw (i, -1, -1) ;
	}
    }


  /* Set up all the active box/seg stuff used for selecting features. */
  look->activeBox = box ;
  seg = BOXSEG(look->activeBox) ;
  look->active_seg_index = BOX2SEG_INDEX(look->activeBox) ;
  if (iskey(seg->key))
    obj_key = seg->key ;
  else if (seg->parent && iskey(seg->parent))
    obj_key = seg->parent ;

  if (seqDragBox)
    seg3 = BOXSEG(seqDragBox) ;
  else
    seg3 = seg ;

  /* record exact dna postition for gf add splice site functions */
  if (seg->type == DNA_SEQ)
    look->DNAcoord = seg->x1 + (int)x; 
  i = (seg->x1 > 0) ? seg->x1 : 0 ;
  max = (seg->x2 >= look->length) ? look->length - 1 : seg->x2 ;
  finDna = max ;
  if (seg->type == DNA_SEQ && seg3->type == DNA_SEQ)
    { 
      i = i + (int)x ;
      i0 = i ;
      imax = seg3->x1 + (int)newx ;
      if (i0 <= imax) /* mhmp  27.04.98 A VOIR: i0 <--> imax*/
	{
	  finDna = (seg3->x2 >= look->length) ? look->length - 1 : seg3->x2 ;
	  cp = arrp(look->colors, i, unsigned char) ;
	  for ( ; i <= imax ; ++i)
	    *cp++ |= TINT_HIGHLIGHT1;
	  /* mhmp 27.04.98 dna en lignes de 50 */
	  buf = messalloc((imax - i0 + 2) *51/50) ; cq = buf ;
	  cq1 = arrp(look->dna, i0, char) ;
	  i = imax ; while(i-- >= i0 ) 
	    {
	       *cq++ =
		doComplementSurPlace ?
		dnaDecodeChar[(int)complementBase [(int)*cq1++]] :
		dnaDecodeChar[(int)*cq1++] ;
	      if ( !((imax - i) % 50) ) 
		*cq++ = '\n' ;
	    }
	  *cq++ = 0 ;
	  graphPostBuffer(buf) ;
	  messfree(buf) ;
	}
    }
  else  if (seg->type == PEP_SEQ || seg->type == TRANS_SEQ || seg->type == TRANS_SEQ_UP)
    { 
      i = i + 3*(int)x ;
      i0 = i ;
      imax = seg3->x1 + 3*(int)newx ;
      finDna = seg3->x2 + 2  ; /* mhmp  07.10.98 */
      finDna = (finDna >= look->length) ? look->length - 1 : finDna ;
      cp = arrp(look->colors, i, unsigned char) ;
      if (i0 <= imax) /* mhmp  27.04.98 A VOIR: i0 <--> imax*/
	{
	  if (i0 < imax)
	    for ( ; i <= imax + 2 ; ++i)
	      *cp++ |= TINT_HIGHLIGHT1;
	  else 
	    { 
	      cp = arrp(look->colors, seg->x1 + 3*(int)x, unsigned char) ;
	      if (seg->type == TRANS_SEQ_UP) /* mhmp 10.06.98 */
		{
		  *cp-- |= TINT_HIGHLIGHT1; 
		  *cp-- |= TINT_HIGHLIGHT2;
		  *cp-- |= TINT_HIGHLIGHT2;
		  look->isRetro = TRUE ;
		}
	      else
		{
		  *cp++ |= TINT_HIGHLIGHT1; 
		  *cp++ |= TINT_HIGHLIGHT2;
		  *cp++ |= TINT_HIGHLIGHT2;
		  look->isRetro = FALSE ;
		}
	    }
	  /* mhmp 27.04.98 proteines en lignes de 50 */
	  buf = messalloc((imax - i0 + 2) *51/50) ; cq = buf ;
	  cq1 = arrp(look->dna, i0, char) ;
	  i = i0 ; 
	  c1 = COORD (look, i);
	  if (seg->type != PEP_SEQ)
	    {
	      for (ii=1; ii<arrayMax(look->maxDna); ii++)
		if (look->flag & FLAG_REVERSE)
		  {
		    if (c1 > array(look->maxDna, ii, int) &&
			c1 <= array(look->minDna, ii- 1, int))
		      {
			nExon = ii - 1;
			oldTframe = array(look->tframe, ii- 1, int) ;
			break ;
		      }
		  }
		else
		  {
		    if (c1 < array(look->maxDna, ii, int) &&
			c1 >= array(look->minDna, ii- 1, int))
		      {
			nExon = ii - 1;
			oldTframe = array(look->tframe, ii- 1, int) ;
			break ;
		      }
		  }

	      isTiret = FALSE ;
	      nbTrans = 0 ;
	      calcWeight = TRUE ;
	      weight = 18 ;
	      while(i <= imax ) 
		{
		  isProt = FALSE ;
		  c1 = COORD (look, i);

		  for (ii = nExon; ii<arrayMax(look->maxDna); ii++)
		    if (look->flag & FLAG_REVERSE)
		      {
			if (c1 > array(look->maxDna, ii, int) &&
			    c1 <= array(look->minDna, ii- 1, int))
			  { 
			    isProt = TRUE ;
			    isTiret = FALSE ;
			    break ;
			  }
		      }
		    else if (c1 < array(look->maxDna, ii, int) &&
			     c1 >= array(look->minDna, ii- 1, int))
		      { 
			isProt = TRUE ;
			isTiret = FALSE ;
			break ;
		      }

		  if (isProt)
		    {
		      Array tt = NULL ;

		      if ((tt = pepGetTranslationTable(obj_key, 0)))
			{
			  if (seg->type == TRANS_SEQ)
			    *cq++ = doComplementSurPlace ? e_antiCodon(cq1, tt) : e_codon(cq1, tt) ;
			  else   /* TRANS_SEQ_UP */
			    *cq++ = doComplementSurPlace ? e_codon(cq1, tt) : e_antiCodon(cq1, tt) ;

			  nbTrans++ ;

			  if (calcWeight)
			    {
			      if (seg->type == TRANS_SEQ)
				w = doComplementSurPlace ? 
				  molecularWeight[(int) e_antiCodon(cq1, tt)] :
				molecularWeight[(int) e_codon(cq1, tt)] ;
			      else  /* TRANS_SEQ_UP */
				w = doComplementSurPlace ? 
				  molecularWeight[(int) e_codon(cq1, tt)] :
				molecularWeight[(int) e_antiCodon(cq1, tt)] ;
			    }
			  if (w < 0)
			    calcWeight = FALSE ;
			  else if (w == 0)
			    {
			      weight = 0 ;
			      calcWeight = FALSE ;
			    }
			  else
			    weight = weight + w - 18 ;
			}
		    }
		  else
		    if (!isTiret)
		      {
			nExon++ ;
			tframe =  array(look->tframe, nExon, int) ; 
			diffTframe = (tframe - oldTframe)%3 ;
			if (diffTframe < 0)
			  diffTframe +=3 ;
			i += diffTframe ;
			cq1 += diffTframe ;
			oldTframe = tframe ;
			isTiret = TRUE ;
		      }
		  cq1 += 3 ;
		  i += 3  ;	
		  if ( !(nbTrans % 50) ) 
		    *cq++ = '\n' ;
		}
	      *cq++ = 0 ;
	      graphPostBuffer(buf) ;
	      messfree(buf) ;
	    }
	  else /* PEPTIDE */
	    {
	      Array tt = NULL ;

	      if ((tt = pepGetTranslationTable(obj_key, 0)))
		{
		  i = imax ; 
		  while(i >= i0 ) 
		    {
		      *cq++ = doComplementSurPlace ? e_antiCodon(cq1, tt) : e_codon(cq1, tt) ;
		      cq1 += 3 ;
		      i -= 3  ;	
		      if ( !((imax - i) % 50) ) 
			*cq++ = '\n' ;
		    }
		  *cq++ = 0 ;
		  graphPostBuffer(buf) ;
		  messfree(buf) ;
		  weight = 18 ;
		  cq1 = arrp(look->dna, i0, char) ;
		  i = imax ; 
		  while(i >= i0 ) 
		    {
		      w = doComplementSurPlace ? 
			molecularWeight[(int) e_antiCodon(cq1, tt)] :
			molecularWeight[(int) e_codon(cq1, tt)] ;
		      if (w < 0)
			break ;
		      else if (w == 0)
			{
			  weight = 0 ;
			  break ;
			}
		      else
			weight = weight + w - 18 ;
		      cq1 += 3 ;
		      i -= 3 ;
		    }
		}
	    }
	}/* endif i0 */
    } 	/* endif seg->type */
  else
    { 
      cp = arrp(look->colors, i, unsigned char) ;
      for ( ; i <= max ; ++i)
	*cp++ |= TINT_HIGHLIGHT1;
    }

  /* write blue report line */
  if (!(look->flag & FLAG_HIDE_HEADER))
    {
      fMapReportLine (look, seg, FALSE, x) ;
      graphBoxDraw (look->segBox, BLACK, PALEBLUE) ;

      /* default is to put dna in cut buffer, alternative is coord data. */
      if (look->cut_data == FMAP_CUT_OBJDATA)
	graphPostBuffer(look->segTextBuf) ;
    }

  for (i = look->minLiveBox ; i < arrayMax(look->boxIndex) ; ++i)
    if (i != look->map->thumb.box)
      {
	seg2 = BOXSEG(i) ;
        if ((seg2->type == DNA_SEQ || seg2->type == PEP_SEQ
	     || seg2->type == TRANS_SEQ || seg2->type == TRANS_SEQ_UP)
	    && seg2->x1 <= finDna && seg2->x2 >= seg->x1)
	  graphBoxDraw (i, -1, -1) ; 
      }

  /* colour siblings */
  /* This is a _really_ tricksy test, it suggests that lots of the code above should not be
   * executed either if this fails...... */
  if ((parent = seg->parent))
    {
      colourSiblings(look, parent) ;
    }

  /* colour box itself over sibling colour */
  switch (seg->type)
    {
    case DNA_SEQ: case PEP_SEQ: case TRANS_SEQ: case TRANS_SEQ_UP:
      break ;
    case   VIRTUAL_SUB_SEQUENCE_TAG:
    case   VIRTUAL_SUB_SEQUENCE_TAG_UP:
      graphBoxDraw (box, -1, PALERED) ;
    default:
      if (assFind (look->chosen, SEG_HASH(seg), 0) ||
	  assFind (look->antiChosen, SEG_HASH(seg), 0))
	graphBoxDraw (box, -1, MAGENTA) ;
      else
	graphBoxDraw (box, -1, PALERED) ;
      break ;
    }

  return ;
}


static void colourSiblings(FeatureMap look, KEY parent)
{
  int i ;
  SEG *seg2 ;

  for (i = look->minLiveBox ; i < arrayMax(look->boxIndex) ; i++)
    {
      if (i == look->map->thumb.box)
	continue;
      seg2 = BOXSEG(i) ;
      if (seg2->parent == parent)
	switch (seg2->type)
	  {
	  case DNA_SEQ: case PEP_SEQ: case TRANS_SEQ: case TRANS_SEQ_UP:
	    break ;
	  case HOMOL: case HOMOL_UP:
	    graphBoxDraw (i, -1, PALERED) ;
	    break ;
	  default:
	    if (assFind (look->chosen, SEG_HASH(seg2), 0))
	      graphBoxDraw (i, -1, CYAN) ;
	    else if (assFind (look->antiChosen, SEG_HASH(seg2), 0))
	      graphBoxDraw (i, -1, DARKRED) ;
	    else
	      graphBoxDraw (i, -1, PALEBLUE) ;
	    break ;
	  }
    }

  return ;
}


/**************************************/

/* This routine will need revising because it assumes a basic catch all that objects that should
 * be TREE displayed are of class Sequence, this is no longer true now that we can SMap any class
 * into an fmap. Some work necessary here.
 *
 * I have put in a temporary fix in so that any key that is smapped (i.e. contains the
 * S_Parent tag will be shown in the Tree display.
 */
static void fMapFollow (FeatureMap look, double x, double y)
{
  SEG* seg = BOXSEG(look->activeBox) ;
  KEY key = seg->key ;
  int key_class = class(key) ;

  if (!key_class || key_class == _VText)
    {
      key = seg->parent ;
      key_class = class(key) ;
    }

  if ((seg->type | 0x1) == SPLICED_cDNA_UP ||
      (seg->type | 0x1) == TRANSCRIPT_UP )
    {
      if (fMapcDNAFollow (look->activeBox))
	return ;
    }
  else if ((seg->type | 0x1) == TRANSCRIPT_UP)
    {
      display (seg->parent, look->seqKey, "TREE") ;
      return ;
    }
  else if (seg->type == VIRTUAL_SUB_SEQUENCE_TAG ||
	   seg->type == VIRTUAL_SUB_SEQUENCE_TAG_UP ||
	   seg->type == VIRTUAL_PARENT_SEQUENCE_TAG ||
	   seg->type == VIRTUAL_CONTIG_TAG)
    {
      if (fMapFollowVirtualMultiTrace (look->activeBox))
	return ;
    }

  if (key_class == _VSequence
      || bIndexTag(key, str2tag("S_Parent")))
    display (key, look->seqKey, "TREE") ;
  else if (key_class)
    display (key, look->seqKey, 0) ;

  return ;
}

/*************************************************************/
/************* a couple of general utilities *****************/

BOOL fMapFindSegBounds (FeatureMap look, SegType type, int *min, int *max)
{
  for (*min = 1 ; *min < arrayMax(look->segs) ; ++*min)
    if (arrp(look->segs, *min, SEG)->type == type)
      break ;
  for (*max = *min ; *max < arrayMax(look->segs) ; ++*max)
    if (arrp(look->segs, *max, SEG)->type != type)
      break ;
  return (*min < arrayMax(look->segs)) ;
}

float fMapSquash (float value, float midpoint)
{
  if (value < 0)
    value = 0 ;
  value /= midpoint ;
  return (1 - 1/(value + 1)) ;
}


/*************************************************************/
/**************** Reverse complement code ********************/

static void fMapCompHelp (void)
{
  graphMessage (
"Reverse-Complement: does what you expect (I hope!)\n"
"Complement: keep coordinates, view opposite strand,\n"
"    which runs bottom to top 5' to 3'.\n"
"Reverse: reverse coordinates, view the same strand,\n"
"    which now runs bottom to top 5' to 3'.\n"
" In both the last two cases the DNA display is still\n"
" read 5' to 3' left to right.  Doing both Complement\n"
" and Reverse is equivalent to Reverse-Complement.\n"
"Complement DNA in place: Don't change coords or\n"
"    direction.  DNA display swaps G-C, A-T.  Now\n"
"    the sequence reads 3' to 5' left to right.") ;
}

static void fMapMenuComplementSurPlace (void)
{
  FeatureMap look = currentFeatureMap("fMapMenuComplementSurPlace") ;

  look->flag ^= FLAG_COMPLEMENT_SUR_PLACE ;
  fMapDraw (look, 0) ;
}

static void fMapReverse (void)
{
  FeatureMap look = currentFeatureMap("fMapReverse") ;

  look->map->mag = -look->map->mag ;
  look->flag ^= FLAG_REVERSE ;

  look->origin = look->length + 1 - look->origin ;
  fMapDraw (look, 0) ;
}


/* This is the guts of the reverse complement, you should note that the code */
/* only knows about the seg coords and only operates on those. If you have   */
/* other features that have coordinates stored else where you need to add    */
/* code to translate them here (see CDS stuff below).                        */
/*                                                                           */
void fMapRC (FeatureMap look)
{
  int top = look->length - 1 ;
  int tmp ;
  char *ci, *cj, ctmp ;
  float *ftmp ;

  sMapFeaturesComplement(look->length, look->segs, look->seqInfo, look->homolInfo) ;
  
  tmp = arrayMax(look->segs) ;
  arrayMax(look->segs) = look->lastTrueSeg ;
	/* avoid mixing temporary and permanent segs */
  arraySort (look->segs, fMapOrder) ;
  arrayMax(look->segs) = tmp ;

  if (look->dnaR)
    {
      Array tmp = look->dna ;
      look->dna = look->dnaR ;
      look->dnaR = tmp ;
    }
  else
    {
      ci = arrp(look->dna, 0, char) ; 
      cj = arrp(look->dna, top, char) ;
      while (ci <= cj)
	{
	  ctmp = *ci ;
	  *ci++ = complementBase[(int)*cj] ;
	  *cj-- = complementBase[(int)ctmp] ;
	}
    }

  if (look->gf.cum)
    {
      tmp = look->gf.max ;
      look->gf.max = top + 1 - look->gf.min ;
      look->gf.min = top + 1 - tmp ;
      ftmp = look->gf.cum ;
      look->gf.cum = look->gf.revCum ; /* already reversed */
      look->gf.revCum = ftmp ;
    }

  look->map->centre = top - look->map->centre ;
  tmp = look->zoneMin ;
  look->zoneMin = look->length - look->zoneMax ;
  look->zoneMax = look->length - tmp ;
  look->origin = look->length + 1 - look->origin ;
  
  look->flag ^= FLAG_COMPLEMENT ;
  fMapClearDNA (look) ;


  /* The sMap must be the same way round as the displayed sequence,
   * so we re-create it here. It would be much easier to re-calculate from
   * scratch to reverse-complement. */
  sMapDestroy(look->smap);

#ifdef FMAP_DEBUG
  fMapPrintLook(look, "In RC.");
#endif

  /* Note that smap uses 1-based coords so we must increment start/stop by 1. */
  if (look->flag & FLAG_COMPLEMENT)
    look->smap = sMapCreate(look->handle, look->seqKey,
			    look->stop + 1, look->start + 1, NULL);
  else
    look->smap = sMapCreate(look->handle, look->seqKey,
			    look->start + 1, look->stop + 1, NULL);

#ifdef FMAP_DEBUG
  {
    ACEOUT dest = aceOutCreateToStdout(0);
    aceOutPrint(dest, "In fMapRC()\n") ;
    sMapDump(look->smap, dest);
    aceOutDestroy(dest);
  }
#endif

  return ;
}

static void fMapCompToggleTranslation (FeatureMap look)
{
  if ((mapColSetByName (look->map, "Up Gene Translation", -1)) ||
      (mapColSetByName (look->map, "Down Gene Translation", -1)))
    {
      mapColSetByName (look->map, "Up Gene Translation", 2) ;
      mapColSetByName (look->map, "Down Gene Translation", 2) ; 
    }

  return ;
}

static void revComplement(FeatureMap look)
{
  fMapCompToggleTranslation (look) ;
  fMapRC (look) ;

  return ;
}

static void fMapMenuComplement (void)
{
  FeatureMap look = currentFeatureMap("fMapMenuComplement");

  revComplement(look) ;
  fMapReverse ()  ;		/* does a lookDraw() */

  return ;
}

static void fMapMenuReverseComplement (void)
{
  FeatureMap look = currentFeatureMap("fMapMenuReverseComplement") ;

  revComplement(look) ;
  fMapDraw (look, 0) ;

  return ;
}




/********* entry point to fMapDumpGFF for fmap menu *********/

static void fMapMenuDumpSegs (void)
{
  static char directory[DIR_BUFFER_SIZE] = "";
  static char filename[FIL_BUFFER_SIZE] = "";
  ACEOUT gff_out;
  FeatureMap look = currentFeatureMap("fMapMenuDumpSegs") ;
  /*
  ** We do nothing here with these two ints, but GFFRefSeqPos()
  ** gff.c:62 needs to receive valid pointers, else it goes bang.
  */
  int key_start_out, key_end_out;
  KEY refSeq =  KEY_UNDEFINED ;

  gff_out = aceOutCreateToChooser ("File to write features into",
				   directory, filename, "gff", "w", 0);
  if (gff_out)
    {
      fMapDumpGFF (look, 2, FALSE,
		   &refSeq, &key_start_out, &key_end_out,
		   FMAPSEGDUMP_FEATURES, FALSE,
		   FALSE, FALSE,
		   NULL, NULL, NULL,
		   FALSE, FALSE, FALSE,
		   NULL, FALSE, FALSE, gff_out);

      aceOutDestroy (gff_out);
    }

  return;
} /* fMapMenuDumpSegs */


/********* entry point to fMapDumpGFF for fmap menu *********/

/* For testing align options etc.... */
static void fMapMenuDumpSegsTest(void)
{
  static char directory[DIR_BUFFER_SIZE] = "";
  static char filename[FIL_BUFFER_SIZE] = "";
  ACEOUT gff_out;
  FeatureMap look = currentFeatureMap("fMapMenuDumpSegs") ;
  /*
  ** We do nothing here with these two ints, but GFFRefSeqPos()
  ** gff.c:62 needs to receive valid pointers, else it goes bang.
  */
  int key_start_out, key_end_out;
  KEY refSeq =  KEY_UNDEFINED ;

  gff_out = aceOutCreateToChooser("File to write features into",
				  directory, filename, "gff", "w", 0) ;
  if (gff_out)
    {
      fMapDumpGFF (look, 2, FALSE,
		   &refSeq, &key_start_out, &key_end_out,
		   FMAPSEGDUMP_FEATURES, FALSE,
		   FALSE, FALSE,
		   NULL, NULL, NULL,
		   FALSE, FALSE, FALSE,
		   NULL, TRUE, FALSE, gff_out);

      aceOutDestroy (gff_out);
    }

  return;
} /* fMapMenuDumpSegs */


/********* entry point to fMapDumpGFF from dnacpt.c *********/

void fMapDumpSegsKeySet (KEYSET kSet, ACEOUT gff_out)
{
  int i ;
  KEY key;
  FeatureMap look ;

  fMapInitialise() ;

  look = (FeatureMap) messalloc (sizeof(struct FeatureMapStruct)) ;
  look->origin = 0 ;
  look->zoneMin = 0 ;
  for (i = 0 ; i < keySetMax(kSet) ; ++i)
    { 
      key = keySet(kSet, i) ;
      if (!sMapTreeRoot (key, 1, 0, 
			 &look->seqKey, &look->start, &look->stop))
	{ 
	  if ((look->stop = sMapLength (key)))
	    { 
	      look->seqKey = key ;
	      look->start = 1 ;
	    }
	  else
	    continue ;		/* can't get limits */
	}
      --look->start ; --look->stop ;
      look->fullLength = look->length = look->stop - look->start + 1 ;
      look->zoneMax = look->length - 1 ;

      if (fMapConvert(look))
	fMapDumpGFF (look, 2, FALSE,
		     NULL, NULL, NULL,
		     FMAPSEGDUMP_FEATURES, FALSE,
		     FALSE, FALSE,
		     NULL, NULL, NULL,
		     FALSE, FALSE, FALSE,
		     NULL, FALSE, FALSE, gff_out);

      arrayDestroy (look->segs) ;

      /* THIS LOOKS LIKE A MEMORY HOLE TO ME, WE NEED BETTER CLEANUP...
	 we should at least do a   messfree (look->handle) ;
	 and probably much else besides, there needs to be a proper
	 routine, separate from fmapdestroy() to do this clear up.
      */
    }

  messfree (look) ;

  return;
} /* fMapDumpSegsKeySet */

/****************/

static void fMapDumpAlign (FeatureMap look, BOOL isPep, ACEOUT dump_out)
{
  int i, ii, bufMax ;
  char *cp, *cq ;
  Array dna = 0 ;
  SEG *seg ;
  HOMOLINFO *hinf = 0 ;
  KEY seqKey = look->seqOrig ;
  int a1, a2, x1, x2, x, y ;
  char *buf = 0 ;
  BOOL first = TRUE ;
  STORE_HANDLE handle = handleCreate() ;

  x = look->zoneMin ;
  y = look->zoneMax ;
  bufMax = y - x + 1 ;
  if (isPep) bufMax /= 3 ;
  messfree(buf) ;
  buf = messalloc (bufMax + 1) ;
  buf[bufMax] = 0 ;
				/* extras */
  for (ii = 0 ; ii < arrayMax(look->segs) ; ++ii)
    { seg = arrp(look->segs, ii, SEG) ;
      if (seg->x1 > look->zoneMax || seg->x2 < look->zoneMin)
	continue ;

      switch (seg->type)
	{
	case HOMOL:
	  if (first)
	    {
	      Array tt = NULL ;

	      if ((tt = pepGetTranslationTable(seqKey, 0)))
		{
		  first = FALSE ;
		  dna = arrayCopy (look->dna) ;
		  x = look->zoneMin ;
		  y = look->zoneMax ;  
		  memset (buf, (int)'.', bufMax) ;
		  aceOutPrint (dump_out, "%s/%d-%d ", name(seqKey), 
			       isPep ? (x + 1 - look->origin)/3 : x + 1 - look->origin , 
			       isPep ? (y - look->origin)/3  : y - look->origin) ;
		  if (x > 0 && x < y && y < arrayMax(dna))
		    {
		      for (i = x , cp = arrp(dna, i, char), cq = buf ; i < y ; i++, cp++, cq++)
			if (isPep)
			  {
			    *cq = e_codon(cp, tt) ;
			    i +=2 ;
			    cp += 2 ; 
			  }
			else
			  *cq = dnaDecodeChar[(int)*cp] ;
		    }
		  aceOutPrint (dump_out, "%s %s\n", buf, name(seqKey)) ;
		  arrayDestroy (dna) ;
		}
	    }
	  dna = dnaGet (seg->key) ;
	  if (dna && arrayMax(dna))
	    {
	      hinf = arrp(look->homolInfo, seg->data.i, HOMOLINFO) ;
	      if (hinf->x1 > hinf->x2)   /* flipped) */
		{
		}
	      else
		{
		  Array tt = NULL ;
		  
		  if ((tt = pepGetTranslationTable(seg->key, 0)))
		    {
		      x1 = hinf->x1 ;  x2 = hinf->x2 ;
		      a1 = seg->x1 ; a2 = seg->x2 ;
		      i = look->zoneMin - a1 ;
		      if (i > 0) { a1 += i ; x1 += i ; }
		      i = seg->x2 - look->zoneMax ;
		      if (i > 0) { a2 -= i ; x2 -= i ; }
		      if (x1 >= 0 && x1 < x2 && x2 < arrayMax(dna))
			{
			  aceOutPrint (dump_out, "%s/", name(seg->key)) ;
			  aceOutPrint (dump_out, "%d-%d ", isPep ? x1/3 : x1, isPep ? x2/3 : x2 ) ;
			  
			  memset (buf, (int)'.', bufMax) ;
			  for (i = a1 , cp = arrp(dna, x1 - 1, char) , 
				 cq = (isPep ? buf + (a1 - look->zoneMin)/3 : buf + a1 - look->zoneMin) ; 
			       i <= a2 ; i++, cp++, cq++)
			    if (isPep)
			      {
				*cq = e_codon(cp, tt) ;
				i += 2 ;
				cp += 2 ;
			      }
			    else
			      *cq = dnaDecodeChar[(int)*cp] ;
			  aceOutPrint (dump_out, "%s %s\n",buf, name(seg->key)) ;
			}
		    }
		}
	      arrayDestroy (dna) ;
	    }
	  break ;
	default: ;
	}
    }
  messfree (buf) ;
  
  handleDestroy (handle) ;

  return;
} /* fMapDumpAlign */

/*************************************/

static void geneStats (void)
{
#if !defined(MACINTOSH)
  int i ;
  SEG *seg ;
  int len = 0 ;
  int nIntron = 0 ;
  int nExon = 0 ;
  int nCDS = 0 ;
  int intronLength = 0 ;
  int exonLength = 0 ;
  int cdsLength = 0 ;
  int nLoci = 0 ;
  int ncDNA = 0 ;
  int nMatch = 0 ;
  char *n ;
  FeatureMap look = currentFeatureMap("geneStats") ;

  for (i = 1 ; i < arrayMax(look->segs) ; ++i)
    { seg = arrp(look->segs, i, SEG) ;
      n = name (seg->parent) ;
      if (n[strlen(n)-1] == 'a')
	continue ;
      switch (seg->type)
	{
	case INTRON: case INTRON_UP:
	  ++nIntron ;
	  intronLength += (seg->x2 - seg->x1 + 1) ;
	  break ;
	case EXON: case EXON_UP:
	  ++nExon ;
	  exonLength += (seg->x2 - seg->x1 + 1) ;
	  break ;
	case CDS: case CDS_UP:
	  ++nCDS ;
	  cdsLength += (seg->x2 - seg->x1 + 1) ;
	  printf ("  %s\n", name(seg->parent)) ;
	  break ;
	case VISIBLE:
	  printf ("    %s: %s %s\n", name(seg->parent), 
		  name(seg->data.k), name(seg->key)) ;
	  if (seg->data.k == _Locus) ++nLoci ;
	  if (seg->data.k == _Matching_cDNA) ++ncDNA ;
	  if (seg->data.k == _Brief_identification) ++nMatch ;
	  break ;
	default:		/* needed for picky compiler */
	  break ;
	}
    }
  for (i = 0, n = arrp(look->dna,0,char) ; i < arrayMax(look->dna) ; ++i)
    if (*n++)
      ++len ;

  printf ("%d bp sequenced DNA from %d in LINK\n", len, look->length) ;
  printf ("%d introns, total %d bp\n", nIntron, intronLength) ;
  printf ("%d exons, total %d bp\n", nExon, exonLength) ;
  printf ("%d CDS (predicted genes), total %d bp\n", nCDS, cdsLength) ;
  printf ("%d database matches\n", nMatch) ;
  printf ("%d cDNAs\n", ncDNA) ;
  printf ("%d Genetic loci\n", nLoci) ;
#else
  messout ("Sorry, this does nothing on a macintosh "
           "(you aren't missing much - it was a quick hack)") ;
#endif
}

#ifndef ACEMBLY			/* trace, assembly stuff */
void fMapTraceDestroy (FeatureMap look) { ;}
void fMapShowTraceAssembly (LOOK look, float *offset) { ; }
BOOL fMapFollowVirtualMultiTrace (int box) { return FALSE ; }
void abiFixFinish (void) { ; }
void fMapGelDisplay (void) { gelDisplay () ; }
void fMapTraceFindMultiplets (FeatureMap look) { ; }
void fMapSelectVirtualMultiplet (FeatureMap look, SEG *seg) { ; } 
#endif


/****************** giface hooks *****************/


/* Construct a sequence but don't display it. */
FeatureMap fMapGifGet (ACEIN command_in, ACEOUT result_out, FeatureMap look)
{
  KEY key ;
  int x1, x2 ;
  char *word = NULL ;
  STORE_HANDLE handle = NULL ;
  DICT *methods_dict  = NULL ;
  DICT *sources_dict  = NULL;
  FmapDisplayData *fmap_data = NULL ;
  BOOL already_sources  = FALSE;
  BOOL already_methods  = FALSE;
  BOOL include_methods  = FALSE;
  BOOL include_sources  = FALSE;
  BOOL noclip = FALSE, maptable = TRUE ;

  messAssert(command_in && result_out) ;

  fMapInitialise() ;

  if (look)
    {
      fMapDestroy (look) ;				    /* could check for possible reuse */
    }

  if (!(word = aceInWord(command_in)))  /* if no params try to get sequence(?) from graph */
    {
      if (fMapActive(0, 0, 0, &look))
	return look ;
      else
	{ 
	  aceOutPrint (result_out, "// gif seqget error: no sequence attached to active window\n") ;
	  return NULL ;
	}
    }

  /* first parameter must be whatever you're examining */
  if (!strstr(word, ":"))
    {
      if (!lexword2key (word, &key, _VSequence))
	{ 
	  aceOutPrint (result_out, "// gif seqget error: Sequence %s not known\n", word) ;
	  goto usage ;
	}
    }
  else
    {
      if (!lexClassKey(word, &key))
	{
	  aceOutPrint (result_out, "// gif seqget error: object \"%s\" not known\n", word) ;
	  goto usage ;
	}
    }


  x1 = 1 ; x2 = 0 ;				           /* default for whole sequence */
  handle = handleCreate();

                                                 /* now process the command-line options */
  while (aceInCheck (command_in, "w"))
    {
      word = aceInWord(command_in);

      if (strcmp(word, "-coords") == 0)
	{ 
	  if (!aceInInt (command_in, &x1) || !aceInInt (command_in, &x2) || (x2 == x1))
	    {
	      aceOutPrint (result_out, "// gif seqget error:  -coords args incorrect\n") ;
	      goto usage ;
	    }
	}
      else if (strcmp(word, "-noclip") == 0)
	{
	  noclip = TRUE ;
	}


#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
      /* I'm turning this on by default....I don't think it will use too much memory... */

      else if (strcmp(word, "-maptable") == 0)
	{
	  maptable = TRUE ;
	}
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */




      /* The logic of next two blocks differs as there's always a method_dict, but there's
       * only a sources_dict if the user has actually specified sources on the command line. 
       */
      else if (strcmp (word+1, "source") == 0)
	{
	  if (already_sources || already_methods)       /* sources & methods mutually exclusive */
	    goto usage ;
	  already_sources = TRUE ;

	  if (word[0] == '+') 
	    include_sources = TRUE;         /* set now as aceInCheck() shifts args along */

	  if(aceInCheck (command_in, "w"))
	    {
	      sources_dict = dictHandleCreate(16, handle);
	      fmapAddToSet(command_in, sources_dict);
	    }

	  break ;					    /* Must be last option. */
	}
      else if (strcmp (word, "-keep") == 0)
	{
	  if (aceInCheck(command_in, "w"))
	    {
	      word = aceInWord(command_in) ;
	      
	      if (strstr(word, "selfhomol") == 0)
		fmap_data->keep_selfhomols = TRUE ;

	      if (strstr(word, "introns") == 0)
		fmap_data->keep_duplicate_introns = TRUE ;
	    }
	}
      else if (strcmp (word+1, "method") == 0)
	{
	  if (already_sources || already_methods)       /* sources & methods mutually exclusive */
	    goto usage ;
	  already_methods = TRUE ;

	  if (word[0] == '+') 
	    include_methods = TRUE;         /* set now as aceInCheck() shifts args along */

	  if(aceInCheck (command_in, "w"))
	    {
	      methods_dict = dictHandleCreate(16, handle);
	      fmapAddToSet(command_in, methods_dict);
	    }

	  break ;					    /* Must be last option. */
	}
      else
	goto usage;
    }

  fmap_data = (FmapDisplayData *)halloc(sizeof(FmapDisplayData), handle) ;
  fmap_data->destroy_obj = FALSE ;

  fmap_data->maptable = maptable ;


  if (sources_dict)
    {
      fmap_data->sources_dict = sources_dict ;
    }
  else
    {
      fmap_data->sources_dict = NULL ;
    }
    
  if (methods_dict)
    {
      fmap_data->methods_dict = methods_dict ;
    }
  else
    {
      fmap_data->methods_dict = NULL ;
    }
  
  fmap_data->include_sources = include_sources ;
  fmap_data->include_methods = include_methods ;


  if (x1 < x2 || (x1 == 1 && x2 == 0))
    look = fMapCreateLook (key, x1, x2, FALSE, FALSE, noclip, fmap_data) ;
  else
    look = fMapCreateLook (key, x2, x1, FALSE, FALSE, noclip, fmap_data) ;

  if (look)
    {
      /* Sequence may need to be reverse complemented.... */
      if (x2 != 0 && x1 > x2)
	{
	  fMapRC (look) ;
	}
    }
  else
    {
      aceOutPrint (result_out,
		   "// gif seqget error: could not create virtual sequence for: %s\n", name(key)) ;
      if (handle)
	handleDestroy (handle) ;
      return NULL;
    }

  return look;

 usage:
  aceOutPrint (result_out, "// gif fexseqget error: usage: "
	       "seqget [sequence [-coords x1 x2][+-source source(s) or +-method method(s)]\n") ;

  if (handle)
    handleDestroy (handle) ;

  return NULL ;

}


/**************************************************************/

void fMapGifDisplay (ACEIN command_in, ACEOUT result_out, FeatureMap look)
{
  int x1 = 0, x2 = 0 ;
  char *word ;
  Graph oldGraph = graphActive() ;
  char *title = NULL ;
  BOOL new = FALSE ;
        
  if (!look || !command_in || !result_out)
    messcrash("Bad args to fMapGifDisplay") ;

  while ((word = aceInWord(command_in)))
    {
      if (strcmp(word, "-visible_coords") == 0)
	{ 
	  if (!aceInInt (command_in, &x1) || !aceInInt (command_in, &x2) || (x2 == x1))
	    goto usage ;
	  if (x1 > x2)
	    { int t = x1 ; x1 = x2 ; x2 = t ; }
	}
      else if (strcmp(word, "-new") == 0)
	new = TRUE ;
      else if (strcmp(word, "-title") == 0)
	{
	  if ((word = aceInWord(command_in)))
	    title = strnew(word, 0) ;
	  else
	    goto usage ;
	}
      else
	goto usage ;
    }

  if (!look->graph || !graphActivate(look->graph))
    { 
      Graph curr_fmap = GRAPH_NULL ;

      curr_fmap = displayGetGraph("FMAP") ;
      if (curr_fmap && !graphActivate(curr_fmap))
	messcrash("Could not activate current fmap, graph id: %d.", curr_fmap) ;

      if (!new && curr_fmap)
	{
	  /* Replace existing current fmap with new one. */
	  fMapDestroyCallback() ;			    /* kill look currently attached */
	  graphRegister(DESTROY, fMapDestroyCallback) ;	    /* reregister */
	  graphClear() ;
	  look->graph = curr_fmap ;
	  displaySetGraph("FMAP", look->graph) ;	    /* reset fmap in display system, was
							     * removed by fMapDestroyCallback() */
	}
      else
	{
	  /* Keep existing fmap if a new one is required.  */
	  if (new && curr_fmap)
	    displayPreserve() ;

	  /* Create a new fmap graph */
	  look->graph = displayCreate("FMAP");
	  graphActivate(look->graph) ;
	  graphRegister(RESIZE, fMapRefresh) ;
	  graphRegister(DESTROY, fMapDestroyCallback) ;
	  graphRegister(KEYBOARD, fMapKbd) ;
	  graphRegister(MESSAGE_DESTROY, displayUnBlock) ;

	  /* If there was no current graph, we need to set it up in "display" system. */
	  if (!curr_fmap)
	    displaySetGraph("FMAP", look->graph) ;
	}

      if (!title)
	title = name(look->seqOrig) ;

      fMapAttachToGraph (look) ;
      mapAttachToGraph (look->map) ;
      look->map->min = 0 ;
      look->map->max = look->length ;
      if (!x1 && !x2)	/* default to show the active zone */
	{
	  if (look->flag & FLAG_REVERSE)
	    {
	      x1 = COORD(look, look->zoneMax) + 1 ;
	      x2 = COORD(look, look->zoneMin) ;
	    }
	  else
	    {
	      x1 = COORD(look, look->zoneMin) ;
	      x2 = COORD(look, look->zoneMax) - 1 ;
	    }
	}
      if (!getenv("ACEDB_PROJECT") && (look->zoneMax - look->zoneMin < 1000))
	{
	  mapColSetByName(look->map, "DNA Sequence", TRUE) ;
	  mapColSetByName(look->map, "Coords", TRUE) ;
	}
    }

  if (title)
    graphRetitle(title) ;

  graphFitBounds (&mapGraphWidth,&mapGraphHeight) ;

  if (x1 || x2)			/* show this region */
    {
      halfGraphHeight = 0.5*(mapGraphHeight - topMargin) ;
      look->map->centre = look->origin + (x1+x2) / 2.0  - 1 ;
      look->map->mag = (mapGraphHeight - topMargin - 5) / (1.05 * (x2 - x1)) ;
      if (x1 > x2)
	{
	  look->map->mag = -look->map->mag ;
	  fMapRC (look) ; 
	}
    }

  fMapDraw (look, 0) ;
  fMapSelect (look) ; 

  {
    float d = (mapGraphHeight - topMargin - 5.0) / (look->map->mag * 1.05) ;
    float xx2 = look->map->centre + 0.5*d ;
    x1 = (int)(xx2 - d + 0.5) ;
    x2 = (int)(xx2 + 0.5) ;
    aceOutPrint(result_out, "// FMAP_COORDS %s %d %d", 
		freeprotect (name(look->seqKey)), x1, x2) ;
    aceOutPrint(result_out, " -visible_coords %d %d\n", COORD(look,x1), COORD(look, x2)) ;
  }

  /* clumsy...uuggghhh */
  goto end ;

 usage:
  aceOutPrint (result_out, "// gif seqdisp error: usage: seqdisp [-visible_coords x1 x2]\n") ;

 end:
  graphActivate (oldGraph) ;

  return ;
} /* fMapGifDisplay */

/**************************************************************/

void fMapGifActions (ACEIN command_in, ACEOUT result_out, FeatureMap look)
{
  char *word;

  if (!look)
    messcrash("Bad args to fMapGifActions") ;

  while (aceInCheck (command_in, "w"))
    { 
      word = aceInWord (command_in) ;

      if (strcmp (word, "-dna") == 0)
	{
	  fMapToggleDna (look) ;
	  fMapDraw (look,0) ; 
	}
      else if (strcmp (word, "-gf_features") == 0)
	{
	  BOOL redraw = TRUE ;

	  word = aceInWord(command_in) ;
	      
	  if (strcmp (word, "no_draw") == 0)
	    redraw = FALSE ;
	  else
	    aceInBack (command_in) ;

	  fMapAddGfSegsFull(look, redraw) ;
	}
      else if (strcmp (word, "-hide_header") == 0)
	{
	  fMapToggleHeader(look);
	  fMapDraw (look, 0) ;
	}
      else if (strcmp (word, "-rev_comp") == 0)
	{
	  fMapCompToggleTranslation (look) ;
	  fMapRC (look) ;
	  fMapDraw (look, 0) ;
	}
      else
	goto usage ;
    }

  return ;

usage:
  aceOutPrint (result_out, "// gif seqactions error: usage: SEQACTIONS [-dna] [-gf_features] [-hide_header] [-rev_comp]\n") ;

  return;
} /* fMapGifActions */

/**************************************************************/

void fMapGifColumns (ACEIN command_in, ACEOUT result_out, FeatureMap look)
{
  char *word;

  if (!look || !look->map) return ;

  while (aceInCheck (command_in, "w"))
    { 
      word = aceInWord (command_in) ;
      if (strcmp (word, "-on") == 0)
	{ 
	  word = aceInWord (command_in) ;
	  if (!word) goto usage ;
	  if (!mapColSetByName (look->map, word, TRUE))
	    aceOutPrint (result_out, "// gif seqcolumns error: unknown column %s", word);
	}
      else if (strcmp (word, "-off") == 0)
	{ 
	  word = aceInWord (command_in) ;
	  if (!word) goto usage ;
	  if (!mapColSetByName (look->map, word, FALSE))
	    aceOutPrint (result_out, "// gif seqcolumns error: unknown column %s", word);
	}
      else if (strcmp (word, "-list") == 0)
	{
	  STORE_HANDLE hh = handleCreate();
	  Array colList = mapColGetList(look->map, hh);
	  int i;
	  BOOL status;
	  char *name;

	  for (i = 0; i < arrayMax(colList); i++)
	    {
	      name = array(colList, i, char*);
	      status = mapColSetByName(look->map, name, -1);
	      aceOutPrint (result_out, "%s \"%s\"\n", status ? "+" : "-", name);
	    }
	  handleDestroy(hh);
	}
      else
	goto usage ;
    }

  return ;

usage:
  aceOutPrint (result_out, "// gif seqcolumns error: usage: SEQCOLUMNS  {-on name} {-off name} {-list}\n") ;

  return;
} /* fMapGifColumns */

/**************************************************************/

void fMapGifAlign (ACEIN command_in, ACEOUT result_out, FeatureMap look)
{
  int x1, x2, pepFrame = 0 ;
  char *word ;
  BOOL isPep = FALSE ;
  ACEOUT fo = 0, dump_out = 0;

  if (!look) return;

  /* by default output goes to same place as normal command results */
  dump_out = aceOutCopy (result_out, 0);

  while (aceInCheck (command_in, "w"))
    { 
      word = aceInWord (command_in) ;
      if (strcmp (word, "-coords") == 0
	  && aceInInt (command_in, &x1)
	  && aceInInt (command_in, &x2) && (x2 != x1))
	{
	  fmapSetZoneUserCoords (look, x1, x2) ;
	}
      else if (strcmp (word, "-file") == 0
	       && aceInCheck (command_in, "w") 
	       && (fo = aceOutCreateToFile (aceInPath (command_in), "w", 0)))
	{
	  /* replace default output with file-output */
	  aceOutDestroy (dump_out);
	  dump_out = fo;
	}
      else if (strcmp (word, "-peptide") == 0)
	isPep = TRUE ;
      else
	goto usage ;
    }

  pepFrame = (pepFrame + 999) % 3 ;
  fMapDumpAlign (look, isPep, dump_out) ;

  aceOutDestroy (dump_out);
  return ;

usage:
  aceOutDestroy (dump_out);
  aceOutPrint (result_out, "// gif seqalign error: usage: SEQALIGN [-coords x1 x2] [-file fname] \n") ;

  return;
} /* fMapGifAlign */

/**************************************************************/


/* Totally assumes that the string coming in is of the form:
 * 
 * "some text|some more text|and|more text"
 * 
 * the text is split _only_ on the "|" symbols so the above would be parsed into:
 * 
 * "some text", "some more text", "and", "more text"
 * 
 * 
 *  */
void fmapAddToSet (ACEIN command_in, DICT *dict)
{ 
  char cutter, *cutset, *word ;

  cutset = "|" ;

  while ((word = aceInWordCut (command_in, cutset, &cutter)))
    { 
      dictAdd (dict, word, 0) ;

      if (cutter != '|')
	break ;
    }

  return ;
}


/**************************************************************/

void fMapGifDNA (ACEIN command_in, ACEOUT result_out, FeatureMap look, BOOL quiet)
{
  KEY key ;
  int x1, x2 ;
  ACEOUT fo = 0, dump_out = 0;
  char *word ;
  BOOL raw = FALSE ;

  if (!look)
    return;

  /* by default output goes to same place as normal command results */
  dump_out = aceOutCopy (result_out, 0);

  while (aceInCheck (command_in, "w"))
    {
      word = aceInWord (command_in) ;
      if (strcmp (word, "-raw") == 0)
	  {
	    raw = TRUE ;
	  }
      else if (strcmp (word, "-coords") == 0
	  && aceInInt (command_in, &x1)
	  && aceInInt (command_in, &x2)
	  && (x2 != x1))
	{
	  fmapSetZoneUserCoords (look, x1, x2) ;
	}
      else if (strcmp (word, "-file") == 0
	       && aceInCheck (command_in, "w")
	       && (fo = aceOutCreateToFile (aceInPath (command_in), "w", 0)))
	{
	  /* write to file instead of default output */
	  aceOutDestroy (dump_out);
	  dump_out = fo;
	}
      else
	goto usage ;
    }


#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
  key = 0 ; x1 = look->zoneMin ; x2 = (look->zoneMax - 1) ;
  if (!fMapFindSpanSequence(look, &key, &x1, &x2))
    key = look->seqKey ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

  key = look->seqKey ; x1 = look->start ; x2 = look->stop ;

  /* If we extend this to do spliced and dna stuff then
   * WE NEED TO CHECK OF look->seq is same as look->seqorig and if not, we should output
   * look->seqorig for the spliced dna _only_.....*/


  x1++, x2++ ;						    /* fMap keeps 0-based coords and we need to
							       report in 1-based coords. */

  if (raw)
    dnaRawDump('u', key, look->dna, look->zoneMin, look->zoneMax-1, dump_out, FALSE) ;
  else if (quiet)
    dnaRawDump('u', key, look->dna, look->zoneMin, look->zoneMax-1, dump_out, TRUE) ;
  else
    dnaDumpFastA(look->dna, look->zoneMin, look->zoneMax-1,
		 messprintf("%s/%d-%d", name(key), x1, x2),
		 dump_out) ;

  if (!quiet)
    aceOutPrint(result_out, "// FMAP_DNA %s %d %d\n", freeprotect (name(key)), x1, x2) ;

  aceOutDestroy (dump_out);

  return;

 usage:
  aceOutDestroy (dump_out);
  aceOutPrint (result_out, "// gif seqdna error: usage: seqdna [-raw] [-coords x1 x2] [-file fname]\n") ;

  return;
} /* fMapGifDNA */


/**************************************************************/

void fMapGifRecalculate(ACEIN command_in, ACEOUT result_out, FeatureMap look)
{
  Graph oldGraph = graphActive() ;
  Graph display_graph = GRAPH_NULL ;
    
  /* Actually look is almost irrelevant...I think ??? */
  if (!command_in || !result_out)
    messcrash("Bad args to fMapGifRecalculate") ;

  if (!look)
    {
      if ((display_graph = displayGetGraph("FMAP")))
	{
	  graphActivate(display_graph) ;
	  graphAssFind(&GRAPH2FeatureMap_ASSOC, &look) ;
	}
    }

  if (look)
    {
      fMapPleaseRecompute(look) ;
      fMapDraw (look, 0) ;
    }
  else
    aceOutPrint (result_out, "// gif seqrecalc error: could not find fmap to recalculate\n") ;

  graphActivate (oldGraph) ;

  return ;
}


/* Called from giface, reports to result_out the current active zone of the
 * virtual fmap in the coord frame of the top parent of the smap and also
 * reports the strand of the fmap, i.e. whether it is reverse complemented or not,
 * e.g.
 * 
 *  acedb> gif seqget b0250 ; seqgetactivezone
 *  ActiveZone Sequence:SUPERLINK_CB_V 10995378 11034593 +
 *
 */
void fMapGifActiveZone(ACEIN command_in, ACEOUT result_out, FeatureMap look)
{
  Graph oldGraph ;
  Graph display_graph = GRAPH_NULL ;

  oldGraph = graphActive() ;

  if (!command_in || !result_out)
    messcrash("Bad args to fMapGifActiveZone") ;

  if (!look)
    {
      if ((display_graph = displayGetGraph("FMAP")))
	{
	  graphActivate(display_graph) ;
	  graphAssFind(&GRAPH2FeatureMap_ASSOC, &look) ;

	  if (!look)
	    aceOutPrint(result_out, "// gif seqgetactivezone error: could not find fmap to retrieve active zone\n") ;
	}
      else
	{
	    aceOutPrint(result_out, "// gif seqgetactivezone error: no current fmap window\n") ;
	}
    }

  if (look)
    {
      KEY to, from ;
      int in_x, in_y ;

      in_x = look->zoneMin ;
      in_y = look->zoneMax ;
      to = KEY_UNDEFINED ;
      from = look->seqOrig ;

      /* agh...hack, zoneMin and zoneMax are initially 0 -> seqlength, smap uses a
       * one based coord system so we need to trap this.... */
      if (in_x == 0)
	in_x = 1 ;

      /* We use sMapTreeRoot() to get the top object, not for mapping (see below). */
      if (sMapTreeRoot(from, in_x, in_y, &to, NULL, NULL))
	{
	  int out_x = 0, out_y = 0 ;

	  /* We calculate the coords like this because fmaps internal coords are for a notional virtual
	   * sequence that is nothing to do with the smapped coords. We can go from them to the
	   * smap parent using look->start. */
	  out_x = look->start + (look->zoneMin + 1) ;	    /* zoneMin is 0 based so bump up by one. */
	  out_y = look->start + look->zoneMax ;

	  aceOutPrint(result_out, "ActiveZone %s %d %d %c\n",
		      nameWithClass(to),
		      out_x, out_y,
		      (FMAP_COMPLEMENTED(look) ? '-' : '+')) ;
	}
      else
	{
	  aceOutPrint (result_out,
		       "// gif seqgetactivezone error: Cannot find position of \"-from %s\""
		       " (coords %d, %d)%s%s\n",
		       nameWithClass(from), in_x, in_y,
		       (to ? " in parent " : ""),
		       (to ? name(to) : "")) ;
	}
    }

  graphActivate (oldGraph) ;

  return ;
}


/**************************************************************/

static void fMapZoneKeySet (void)
{  
  SEG *seg ;
  int i, n = 0 ;
  KEYSET ks = keySetCreate () ;
  FeatureMap look = currentFeatureMap("fMapZoneKeySet") ;

  for (i = 0 ; i < arrayMax(look->segs) ; ++i)
    { seg = arrp(look->segs, i, SEG) ;
      if (seg->x1 > look->zoneMax || seg->x2 < look->zoneMin)
	continue ;
      if (class(seg->key))
	keySet(ks, n++) = seg->key ;
    }
  keySetSort(ks) ;
  keySetCompress (ks) ;

  if (!keySetMax(ks))
    { messout ("The active zone does not contain any key") ;
      keySetDestroy (ks) ;
    }
  else 
    keySetNewDisplay (ks, messprintf("%s %d %d", 
				     name(look->seqKey),look->zoneMin - look->origin + 1, 
				     look->zoneMax - look->origin)) ;
}


/*
 *                   Some debugging/information routines
 */


/* THIS LOT SHOULD BE RATIONALISED WITH STUFF IN SMAPCONVERT.C */

/* Sometimes needs to flip a seg from one strand to the other, e.g. for readpair data
 * one of the sequences may have been sequenced in reverse and hence needs to be flipped
 * for display. */
void fmapSegFlipHomolStrand(SEG *seg, HOMOLINFO *hinf)
{
  int tmp ;

  /* If you want to lift this restriction then add code to support the types you want to flip. */
  messAssert(seg->type == HOMOL || seg->type == HOMOL_UP) ;

  SEG_FLIP(seg) ;

  if (hinf->strand == STRAND_FORWARD)
    {
      hinf->strand = STRAND_REVERSE ;
    }
  else
    {
      hinf->strand = STRAND_FORWARD ;
    }


  /* Currently we need to flip the hinf coords, now we have strand stored we should stop
   * doing this... */
  tmp = hinf->x1 ;
  hinf->x1 = hinf->x2 ;
  hinf->x2 = tmp ;

  /* I don't think we need to flip the gaps array.... */


  return ;
}


/* Returns the name of the object that the seg was derived from.
 * The reason for having this routine is that this is _not_ simply
 * name(seg->key)
 */
char *fmapSeg2SourceName(SEG *seg)
{
  char *source_name = NULL ;
  SegType type ;

  type = SEG_DOWN(seg->type) ;				    /* normalise type to just DOWN types */

  switch(type)
    {
    case EMBL_FEATURE:					    /* Doesn't seem to be anything sensible. */
      source_name = NULL ;
      break ;

    case SEQUENCE:
    case ALLELE:
    case FEATURE:
    case CLONE_END:
    case OLIGO:
    case SPLICE5:
    case SPLICE3:
      source_name = name(seg->key) ;
      break ;

    case EXON:
    case CDS:
    case CODING:
    case ASSEMBLY_TAG:
      source_name = name(seg->parent) ;
      break ;

    case INTRON:
      /* Confirmed introns are recorded with type == INTRON but do not have seg->parent
       * set, so use seg->source instead. */
      if (seg->parent != KEY_UNDEFINED)
	source_name = name(seg->parent) ;
      else
	source_name = name(seg->source) ;
      break ;

    default:						    /* last chance saloon. */
      if (seg->key != KEY_UNDEFINED && iskey(seg->key) == LEXKEY_IS_OBJ)
	source_name = name(seg->key) ;
      else if (seg->parent != KEY_UNDEFINED && iskey(seg->parent) == LEXKEY_IS_OBJ)
	source_name = name(seg->parent) ;
      else if (seg->source != KEY_UNDEFINED && iskey(seg->source) == LEXKEY_IS_OBJ)
	source_name = name(seg->source) ;
      else
	source_name = NULL ;
    }

  return source_name ;
}


/**************************************************************/


/* 
 *                       Debugging utilities.
 * 
 */


/* Print out a seg with as much extra information as possible... */
void fmapPrintSeg(FeatureMap look, SEG *seg)
{
  Stack text_stack ;
  ACEOUT dest ;

  text_stack = stackCreate(100) ;
  dest = aceOutCreateToStack(text_stack, 0) ;

  printSeg(-1, seg, dest) ;				    /* -1 means "don't try and print the segnum" */

  messout("%s", popText(text_stack)) ;

  aceOutDestroy(dest) ;
  stackDestroy(text_stack) ;

  return ;
}


/* Print out a seg with as much extra information as possible... */
void fmapPrintStdOutSeg(SEG *seg)
{
  ACEOUT dest ;

  dest = aceOutCreateToStdout(0);

  printSeg(-1, seg, dest) ;				    /* -1 means "don't try and print the segnum" */

  aceOutDestroy(dest);

  return ;
}


void fmapDumpSegs(Array segs, ACEOUT dest)
{
  int i;
  
  for (i=0; i< arrayMax(segs); i++)
    {
      SEG * seg = arrp(segs, i, SEG);

      printSeg(i, seg, dest) ;
    }

  return ;
}


/* Print out a seg, we could use the seg type to print out even more info. */
static void printSeg(int segnum, SEG *seg, ACEOUT dest)
{

  if (segnum < 0)
    aceOutPrint(dest, "%s: %d,%d  ",
		fMapSegTypeName[seg->type],
		seg->x1, seg->x2) ;
  else
    aceOutPrint(dest, "%d %s: %d,%d  ",
		segnum,
		fMapSegTypeName[seg->type],
		seg->x1, seg->x2) ;
  aceOutPrint(dest, "key=\"%s\", ",
	      (seg->key ? nameWithClassDecorate(seg->key) : "<undefined>")) ;
  aceOutPrint(dest, "parent=\"%s\", ",
	      (seg->parent ? nameWithClassDecorate(seg->parent) : "<undefined>")) ;
  aceOutPrint(dest, "source=\"%s\", ",
	      (seg->source ? nameWithClassDecorate(seg->source) : "<undefined>")) ;
  aceOutPrint(dest, "\n");

  return ;
}


static void fMapLookDataDump(FeatureMap look, ACEOUT dest)    
{
  aceOutPrint(dest, "seqKey = %s\n", name(look->seqKey));
  aceOutPrint(dest, "mapKey = %s\n", name(look->mapKey));
  aceOutPrint(dest, "dnaKey = %s\n", name(look->dnaKey));
  aceOutPrint(dest, "start=%d stop=%d min=%d max=%d length=%d\n", 
	      look->start, look->stop, look->min, look->max, look->length);
  aceOutPrint(dest, "fullLength=%d origin=%d zoneMin=%d zoneMax=%d\n",
	      look->fullLength, look->origin, look->zoneMin, look->zoneMax);
  aceOutPrint(dest, "dnaStart=%d dnaWidth=%d dnaSkip=%d\n",
	      look->dnaStart, look->dnaWidth, look->dnaSkip);
}

void fMapLookDump(FeatureMap look)
{
  ACEOUT dest = aceOutCreateToStdout(0);
  fMapLookDataDump(look, dest);
  fmapDumpSegs(look->segs, dest);
  aceOutDestroy(dest);

  return ;
}



/* Print out the fmap control struct so you can see what is going on.....  */
void fMapPrintLook(FeatureMap look, char *prompt)
{
  printf("\n") ;

  if (prompt)
    printf("------- %s ---------\n", prompt);

  printf("seqKey = %s, seqOrig = %s\n", name(look->seqKey), name(look->seqOrig)) ;
  printf("start = %d, stop = %d\n", look->start, look->stop) ;
  printf("min = %d, max = %d\n", look->min, look->max) ;
  printf("length = %d, fullLength = %d\n", look->start, look->stop) ;
  printf("origin = %d, zoneMin = %d, zoneMax = %d\n", look->origin, look->zoneMin, look->zoneMax) ;
  printf("map->centre = %f, map->mag = %f\n", look->map->centre,look->map->mag) ;
  printf("map->min = %f, map->max = %f\n", look->map->min,look->map->max) ;
  printf("pleaseRecompute = %s\n", 
	 (look->pleaseRecompute == FMAP_RECOMPUTE_NO ? FMAP_STR(FMAP_RECOMPUTE_NO)
	  : (look->pleaseRecompute == FMAP_RECOMPUTE_YES ? FMAP_STR(FMAP_RECOMPUTE_YES)
	     : FMAP_STR(FMAP_RECOMPUTE_ALREADY)))) ;

  printf("\n") ;

  return ;
}






/* Routine to dump out stats for a look.
 * The intention is augment this routine to produce more comprehensive and
 * selectable stats to aid with debugging/performance testing etc. */
static void dumpstats(FeatureMap look)
{
  int seg_totals[CLONE_END + 1] = {0} ;
  int i ;
  SEG *seg ;
  int seg_size = sizeof(SEG) ;
  float megabyte = 1048576.0 ;

  for (i = 0 ; i < arrayMax(look->segs) ; i++)
    {
      seg = arrp(look->segs,i,SEG) ;
      seg_totals[seg->type]++ ;
    }

  printf("\n\t\tStats for FMap of %s\n" , name(look->seqKey)) ;

  for (i = 0 ; i <= CLONE_END ; i++)
    {
      if (seg_totals[i])
	{
	  printf("%15s: %15d\t\t(%2d%%)\n",
		 fMapSegTypeName[i], seg_totals[i], (seg_totals[i] * 100)/arrayMax(look->segs)) ;
	}
    }

  printf("%15s: %15d\t\t(%.1fMB)\n", "Total segs",
	 arrayMax(look->segs), (((float)seg_size * (float)arrayMax(look->segs)) / megabyte)) ;

  return ;
}


/****************** end of file *****************/
