/*  File: fmap_.h
 *  Author: Ed Griffiths (edgrif@sanger.ac.uk)
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
 * Description:
 *      This is the internal header file for the fmap package, this file
 *      _must_only_ be included by fmap source files. The public header for
 *      users of the fmap package is fmap.h.
 *
 * Exported functions:
 * HISTORY:
 * Last edited: Sep 20 10:44 2011 (edgrif)
 * * May 24 16:12 1999 (edgrif): Added fields for preserving column settings.
 * * Sep  2 10:32 1998 (edgrif): Correct declaration for fMapFollowVirtualMultiTrace,
 *      should be void, not BOOL.
 * * Jul 28 16:26 1998 (edgrif): Add fMapInitialise() to function declarations.
 * * Jul 27 10:48 1998 (edgrif): Finish comments/tidy up.
 * * Jul 23 09:13 1998 (edgrif): Move fMapFindDNA into public header for use
 *      by dna code. (dnacpt.c and others).
 * Created: Thu Jul 16 09:29:49 1998 (edgrif)
 * CVS info:   $Id: fmap_.h,v 1.73 2012-11-07 13:50:52 edgrif Exp $
 *-------------------------------------------------------------------
 */
#ifndef ACEDB_FMAP_P_H
#define ACEDB_FMAP_P_H

#include <wh/acedb.h>
#include <wh/dict.h>
#include <wh/bitset.h>
#include <wh/graph.h>
#include <wh/fmap.h>					    /* Our public header. */
#include <wh/methodcache.h>
#include <wh/smap.h>


/************************************************************/

extern BOOL fMapDebug ;					    /* If set TRUE, fmap will emit
							       debugging messages. */

/************************************************************/


/* All possibilities of strand */
typedef enum
  {
    FMAPSTRAND_INVALID,
    FMAPSTRAND_FORWARD,					    /* Also known as '+' */
    FMAPSTRAND_REVERSE,					    /* Also known as '-' */
    FMAPSTRAND_NONE,					    /* Also known as '.' */
    FMAPSTRAND_UNKNOWN					    /* Also known as '?' */
  } FMapStrand ;



/*                  SEG info.                     
 * Holds info. about a feature, or part of a feature, that should be drawn or
 * processed by the fmap/gff code etc. */


/* Segs are held in an array, all of which are valid segs, the first (zero) is the master seg.
 * This value can be used as an invalid seg index... */
enum {SEG_INVALID_INDEX = -1} ;


/* _very_important_, if you change this enum then you must:
 *
 *    - make sure the nnn_UP types remain odd numbered
 *
 *    - not change the order of the existing enums, much of the drawing
 *      code relies on this order.
 * 
 *    - correspondingly change the list of names in static char* fMapSegTypeName[]
 *      in fmapcontrol.c
 */
typedef enum
{ 
  MASTER=0,

  ASSEMBLY_TAG = 1,

  /* for the following types, strand matters, make sure that FIRST_UPPABLE and
   * LAST_UPPABLE are always equal to the first/last tags. Also make very sure that
   * they are numbered so that nnn_UP types are odd. */
  SEQUENCE = 2, FIRST_UPPABLE = SEQUENCE, SEQUENCE_UP,

  CDS, CDS_UP,						    /* Crucial that CDS is before EXONs */

  /* ensures we draw introns before exons which looks better. */
  INTRON, INTRON_UP,
  EXON, EXON_UP,

  EMBL_FEATURE, EMBL_FEATURE_UP, 
  FEATURE, FEATURE_UP,
  ATG, ATG_UP,
  SPLICE3, SPLICE3_UP,
  SPLICE5, SPLICE5_UP,
  CODING, CODING_UP,
  TRANSCRIPT, TRANSCRIPT_UP,
  SPLICED_cDNA, SPLICED_cDNA_UP,

  VIRTUAL_SUB_SEQUENCE_TAG,				    /* a subseq without recursion */ 
  VIRTUAL_SUB_SEQUENCE_TAG_UP,
  VIRTUAL_MULTIPLET_TAG, VIRTUAL_MULTIPLET_TAG_UP,
  VIRTUAL_ALIGNED_TAG, VIRTUAL_ALIGNED_TAG_UP,
  VIRTUAL_PREVIOUS_CONTIG_TAG, VIRTUAL_PREVIOUS_CONTIG_TAG_UP,

  /* Homol_gaps are the lines between homols of same match sequence so draw them first. */
  HOMOL_GAP, HOMOL_GAP_UP,
  HOMOL, HOMOL_UP,

  PRIMER, PRIMER_UP,					    /* useful for Directed sequencing */
  OLIGO, OLIGO_UP,
  OLIGO_PAIR, OLIGO_PAIR_UP,
  TRANS_SEQ, TRANS_SEQ_UP,

  FEATURE_OBJ, FEATURE_OBJ_UP,

  ALLELE, ALLELE_UP,
  LAST_UPPABLE = ALLELE_UP,				    /* Make sure this is _always_ the last 
							     type where strand matters. */

  VISIBLE,
  DNA_SEQ, PEP_SEQ, ORF,
 
  VIRTUAL_PARENT_SEQUENCE_TAG, /* mieg, a subseq, but without recursion */ 
  VIRTUAL_CONTIG_TAG,

  /* New tag for showing clone/assembly path. */
  ASSEMBLY_PATH,

  CLONE_END
} SegType ;



/* Returning/Testing/overloading of the type field. */

/* Return the "up" or "down" type */
/* SEG_UP, DOWN: check the type is strand-sensitive first */
#define SEG_UP(SEG_TYPE)        (((SEG_TYPE) < FIRST_UPPABLE || (SEG_TYPE) > LAST_UPPABLE) ? (SEG_TYPE) : ((SEG_TYPE) & 0x1) ? (SEG_TYPE) : ((SEG_TYPE) + 1))
#define SEG_DOWN(SEG_TYPE)      (((SEG_TYPE) < FIRST_UPPABLE || (SEG_TYPE) > LAST_UPPABLE) ? (SEG_TYPE) : ((SEG_TYPE) & 0x1) ? ((SEG_TYPE) - 1) : (SEG_TYPE))
/* Test the "up" or "down" type */
/* SEG_IS_DOWN, UP not reliable with strand-insensitive types */
#define SEG_IS_DOWN(SEG)        (!((SEG)->type & 0x1))
#define SEG_IS_UP(SEG)          ((SEG)->type & 0x1)
/* Flip the "up" or "down" type */
/* SEG_FLIP: check the type is strand-sensitive first */
#define SEG_FLIP(SEG)           (((SEG)->type < FIRST_UPPABLE || (SEG)->type > LAST_UPPABLE) ? (SEG)->type : SEG_IS_UP((SEG)) ? (((seg)->type)--) : (((seg)->type)++))
/* Sometimes only the top byte of the seg type int is used for the type....regrettable. */
#define HASH(x,y)		((char*)((char *)0)  + ((x) << 24) + (y))
#define SEG_HASH(seg)		(HASH((seg)->type,(seg)->x2))
#define UNHASH_TYPE(x)		((int)((x) - ((char *)0)) >> 24)
#define UNHASH_X2(x)		((int)((x) - ((char *)0)) & 0xffffff)



/* Use flags field in segs struct for extra drawing info. which is _per_ seg
 * and therefore cannot be held in an associated SEQINFO struct. */
#define SEGFLAG_UNSET              0x00000000U		    /* Initialise with this. */

#define SEGFLAG_CLIPPED_TOP        0x00000001U		    /* Top/bottom coord of this seg was */
#define SEGFLAG_CLIPPED_BOTTOM     0x00000002U		    /* clipped during mapping. */

#define SEGFLAG_LINE_DASHED        0x00000010U		    /* seg is dashed connecting line */

#define SEGFLAG_FLIPPED            0x00000100U		    /* seg was flipped for display. */
#define SEGFLAG_TWO_PARENT_CLUSTER 0x00000200U		    /* Some paired Homol matches that
							       should be joined are in
							       separate objects. */



/* Type of data in "data" union. */
typedef enum {SEG_DATA_UNSET, SEG_DATA_FLOAT, SEG_DATA_INT, SEG_DATA_KEY,
	      SEG_DATA_STRING, SEG_DATA_SEQINFO, SEG_DATA_HOMOLINFO} FMapSegDataType ;


/* All objects drawn by fmap are represented by "segs", a combination of the
 * seg and information in the 'data' union is used to draw the feature. */
typedef struct
{
  KEY	  key ;						    /* Object this seg directly represents. */
  KEY     parent ;					    /* Semantic parent object, e.g. "Gene" */
  KEY     source ;					    /* SMap mapping parent object. */

  /* WE SHOULD HAVE A METHOD SLOT HERE.... */

  /* Are these needed any more ??? check this.... */
  SegType parent_type ;					    /* Type of parent seg, 0 if none. */
  SegType type ;					    /* ALLELE, SEQUENCE etc. */


  KEY     subfeature_tag ;				    /* "Feature", "DNA_homol", "Source_exon" etc. */

  /* CHECK WHY WE NEED THIS....MUST BE FOR GFFv3 dumps.... */
  KEY     tag2 ;


  FMapStrand strand ;					    /* Strand on the _reference_ sequence. */
  int	  x1, x2 ;					    /* Position in virtual sequence. */

  unsigned int flags ;					    /* for _per_ seg info. */


  /* utility slot */
  FMapSegDataType data_type ;				    /* Records type of data in the data union. */
  union
  {
    float  f ;
    int    i ;
    KEY    k ; 
    char  *s ;
    unsigned int index ;
  } data ;


#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
  /* Index into an array that holds extra data for the seg.
   * 
   * should move this into the union above.... */
  int data_index ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

  /* Feature specific data. */
  union
  {
    int exon_phase ;

    int oligo_type ;
  } feature_data ;

} SEG ;



#define segFormat "kkiiif"				    /* ???????????????? */





/*                 Extra Feature Info
 *
 * SEG's have only the basic structure to support feature processing, dumping
 * and drawing. Extra info. is provided in separate structs that are allocated
 * only if needed. Note for objects that may generate a number of SEG's
 * e.g. transcript, then the extra information may be shared amongst all those
 * SEG's saving quite a bit of storage.
 * 
 * As a rough guideline if you think you will have a lot of features all needing
 * just a small amount of per-feature info then it's probably best to just add
 * this to the SEG struct itself. It's not worth having a whole other struct per
 * SEG.
 * 
 *  */


/* SEQINFO, represents a number of features, was the first extra info. struct. */

/* Lots of these are old stuff and may no longer be needed, probably need reviewing. */
#define SEQ_CANONICAL		0x00000001
#define SEQ_VISIBLE		0x00000002
#define SEQ_EXONS		0x00000004
#define SEQ_VIRTUAL_ERRORS	0x00000008
#define SEQ_SCORE		0x00000010
#define SEQ_CDS                 0x00000020

#define SEQ_CONFIRM_UNKNOWN	0x00010000
#define SEQ_CONFIRM_EST		0x00020000
#define SEQ_CONFIRM_HOMOL	0x00040000
#define SEQ_CONFIRM_CDNA	0x00080000
#define SEQ_CONFIRM_UTR		0x00100000
#define SEQ_CONFIRM_FALSE       0x00200000
#define SEQ_CONFIRM_INCONSIST   0x00400000
#define SEQ_CONFIRMED		0x007f0000

enum {SEQ_CONFIRM_TYPES = 8} ;				    /* good but not essential to keep this
							       in step with total number of
							       SEQ_CONFIRM flags.*/



/* WHY DO WE HAVE A CDS SEG....REDUNDANT I THINK....SHOULD JUST RECORD ALL INFO HERE...
 * perhaps that's what happens anyway.... */

/* A sequence object that has Source_Exons may have a CDS defined within it,
 * this needs special handling for colour/position of CDS, protein translation etc. */
typedef struct
{
  /* should be able to just point to cds seg ???? */
  SEG cds_seg ;						    /* Position of CDS */

  BOOL cds_only ;					    /* TRUE => Protein translate only CDS */
							    /* exons. */
} CDS_Info ;


/* For confirmed introns we need to record not only a string representing the confirmation
 * tag but also possibly a sequence to go with that tag. */
typedef struct
{
  char *confirm_str ;
  KEY confirm_sequence ;
} ConfirmedIntronInfoStruct, *ConfirmedIntronInfo ;


typedef struct
{
  KEY method ;
  float score ;
  Array gaps;						    /* set if this structure is gappy */

  KEY flags ;

  /* We should have a union here of types of info. for each type of feature. */

  /* For confirmed introns */
  Array confirmation ;					    /* of ConfirmedIntronInfoStruct */

  /* For cds stuff. */
  CDS_Info cds_info ;

  int start_not_found ;					    /* Values: 0 => not set,
							       1, 2, 3 => offset for translation. */
  BOOL end_not_found ;

  SMapAssemblyStruct assembly ;

} SEQINFO ;


/* Attached to segs that represent matches. Each seg usually corresponds to a single HSP. */
typedef struct
{
  int x1, x2 ;						    /* Always forwards, i.e. x1 <= x2. */
  FMapStrand strand ;					    /* Strand of the _match_ sequence. */
  float score ;
  KEY method ;

  Array gaps;						    /* Gaps within a single HSP
							       alignment. */

  /* An align string, e.g. cigar. */
  char *match_type ;
  char *match_str ;

  char *align_id ;					    /* Used to group separate alignments. */

  KEY cluster_key ;					    /* Key to be used for "cluster" column display. */
} HOMOLINFO ;



#ifdef ED_G_NEVER_INCLUDE_THIS_CODE

/* Not implemented yet.... */

/* For features that are objects in their own right. */
typedef struct
{
  
  KEY method ;
  float score ;
  BOOL interbase_feature ;
} FeatureObjInfo ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */



/* For features that are sub-objects and are represented by the Feature tag set. */
typedef struct
{
  
  KEY method ;
  float score ;
  BOOL interbase_feature ;
} FeatureInfo ;


typedef struct
{
  KEY key ;
  float y1, y2 ;
  int iseg ;
} KEYZONE ;


/* XXX This has to match the GfInfo struct in w9/gfcode.c XXX */
typedef struct
{
  int   min, max ;
  float *cum, *revCum ;
} GFINFO ;


/* Used to specify whether the fmap should be recomputed or not, FMAP_RECOMPUTE_ALREADY
 * is necessary to avoid recursion. */
typedef enum {FMAP_RECOMPUTE_NO, FMAP_RECOMPUTE_YES, FMAP_RECOMPUTE_ALREADY} FMapRecompute ;


#define FMAP_STR(STRING) #STRING


/************************************************************/
/* Use an array of these structs to hold the default values for setting up   */
/* the map columns associated with fmap.                                     */
typedef struct FeatureMapColSettingsStruct_
{
  float priority ;
  BOOL isOn ;
  char *name ;
  MapColDrawFunc func ;
} FeatureMapColSettingsStruct ;



/************ fMap structure holds all data about the window ***********/
/*
  Some comments on fmap stuff....



    fullLength - the length of the topmost parent of the selected sequence
    =
    |
    |
    |-if you zoom or scroll into this area then recalculation must be done
    | to produce new segs, _note_ that you cannot scroll and zoom beyond
    | this area.
    |
    |
    |length - the length of the sequence selected for display
    |=
    ||
    ||
    || 0 <= zoneMin < zoneMax <= length
    ||   sets active region for fmap operations within here.
    ||
    ||<- in here you can zoom and scroll without recalculation as segs have
    ||   been done for all data in this section.
    ||
    ||
    ||
    ||
    ||
    ||
    |=
    |
    |
    |
    |
    =

 */
struct FeatureMapStruct
{
  magic_t *magic;			/* == &FeatureMap_MAGIC */
  STORE_HANDLE handle, segsHandle ;
  KEY   seqKey ;
  KEY   seqOrig ;					    /* the orginal Sequence key that
							       fmapCreateLook() was called on */
  KEY   mapKey ;
  KEY   dnaKey ;

  SMap *smap;						    /* _The_ virtual sequence map. */
  BOOL  noclip ;					    /* TRUE means smap (and hence segs)
							       is _not_ clipped to start/stop, so
							       anything that overlaps will be mapped. */

  DICT *mapped_dict ;					    /* Optional hash of feature names that
							       are mapped. */

  int   start, stop ;					    /* limits of information for current convert. */
  int   min, max ;					    /* min and max position on screen */
  int   length ;					    /* The length of the sequence
							       associated with the key to be displayed. */
  int   fullLength ;					    /* The length of the ultimate parent
							       of the key to be displayed. */

  int   origin ;
  int   zoneMin, zoneMax ;				    /* Active zone where genefinder
							       etc. will operate. */
  int   activeBox, minLiveBox ;				    /* Current active boxes/segs. */

  int   active_seg_index ;				    /* Needed for holding onto active
							     * box when box goes off screen. */

  Array boxIndex ;		/* SEG if >minLiveBox, <maxLiveBox */

  int   summaryBox, selectBox, zoneBox, originBox, segBox ;

  int   dnaWidth, dnaStart, dnaSkip ;		  /* set in makeSartSkip, used in virtualDna */
  char  originBuf[24], zoneBuf[24], oligoNameBuffer [16] ;
  char  segNameBuf[64], segTextBuf[512] ;

  Graph graph ;

  Array segs ;			/* array of SEG's from the obj */
  int   lastTrueSeg ;

  Array dna, dnaR ;		/* dnaR in acembly */
  Array colors ;
  Array sites ;
  Stack siteNames ;
  Associator chosen, antiChosen ;
  Array multiplets, contigCol ;

  Array seqInfo ;
  Array homolInfo ;
  Array feature_info ;


  Array minDna, maxDna ;
  Array decalCoord ;
  Array tframe ;

  MENUOPT *mainMenu ;					    /* Need our own copy of the main menu. */
  int   flag ;

  FMapRecompute pleaseRecompute ;			    /* Force recompute. */
  BOOL  preserveColumns ;				    /* Keep same column settings reused fmap. */
  BOOL  redrawColumns ;					    /* Should fmap be redrawn each time a
							       column is turned on/off ? */
  BOOL  reportDNAMismatches ;				    /* Should any dna mismatches be
							       reported ? */
  BOOL  destroy_obj ;					    /* TRUE: destroy "key" object on
							       reuse or quit of fmap. */
  FMapCutDataType  cut_data ;				    /* Type of data put into PRIMARY
							       selection buffer by button1 click. */

  BOOL keep_selfhomols, keep_duplicate_introns ;	    /* Controls weeding out of segs after smapconvert. */


  MAP   map ;
  MethodCache mcache;

  GFINFO gf ;
  KEY translateGene ;
  Graph selectGraph ;					    /* graph for showing selections */

  Associator virtualErrors, taggedBases ;
  int DNAcoord ;					    /* valid when
							       BOXSEG(look->activeBox)->type == DNA_SEQ */
  Array oligopairs ;
  BOOL isGeneFinder ;					    /* triggers the genefinder menu in
							       acembly */
  BOOL isGFMethodsInitialised;	                            /* if GF methods init'd in
							       mcache of this look */

  BitSet homolBury ;					    /* same index as homolInfo */
  BitSet homolFromTable ;				    /* MatchTable or Tree? (index as
							       homolInfo) */
  DICT *featDict ;
  DICT *bcDict ;
  Array bcArray ;					    /* for local column info */

  KEYSET method_set ;					    /* which methods are to be fmapped. */
  BOOL   include_methods;
  DICT   sources_dict ;                                     /* which sources are to be fmapped */
  BOOL   include_sources ;                                  /* whether to include or exclude */

  Array visibleIndices ;
  BOOL isRetro ;					    /* needed to correctly paint the
							       triplet corresponding to an amino acid */
  BOOL isOspSelecting ;					    /* set/unset in fmaposp, to prevent
							       deregistering PICK */
} ;

/************************************************************/

#define FLAG_RNA			0x0001
#define FLAG_COMPLEMENT 		0x0002
#define FLAG_REVERSE			0x0004
#define FLAG_COLOR_HID			0x0008
#define FLAG_HAVE_GF			0x0010
#define FLAG_COMPLEMENT_SUR_PLACE 	0x0020
#define FLAG_HIDE_HEADER 		0x0040
#define FLAG_AUTO_WIDTH 		0x0080
#define FLAG_VIRTUAL_ERRORS 		0x0100
#define FLAG_HIDE_SMALL_CONTIGS 	0x0200
#define FLAG_COLOR_CONTIGS		0x0400
#define FLAG_COLUMN_BUTTONS		0x0800

#define FMAP_COMPLEMENTED(LOOK) ((LOOK)->flag & FLAG_COMPLEMENT)



/************************************************************/

#define COORD(look, x) \
   ((look->flag & FLAG_REVERSE) ? \
   look->length - (x) - look->origin \
   : (x) - look->origin + 1)

/* boxIndex maps box numbers to the corresponding seg in the seg array via the index
 * of the appropriate seg.
 * BOXSEG() returns the SEG*
 * BOX2SEG_INDEX() returns the segs in index in the seg array. */
#define BOXSEG(i) \
  arrp(look->segs, arr(look->boxIndex, (i), int), SEG)

#define BOX2SEG_INDEX(i) \
arr(look->boxIndex, (i), int)


/************************************************************/

/* addresses of the following are unique ids */
extern magic_t GRAPH2FeatureMap_ASSOC ; /* for graph -> FeatureMap  */
extern magic_t FeatureMap_MAGIC ; /* for verification of FeatureMap pointer */



/**************** tags and classes ***********************/

extern KEY _Arg1_URL_prefix ;
extern KEY _Arg1_URL_suffix ;
extern KEY _Arg2_URL_prefix ;
extern KEY _Arg2_URL_suffix ;


/**********************************************************/

/* Private, non-static routines.                                             */
/*                                                                           */
				/* fmapcontrol.c */
void fMapInitialise (void) ;

/* Verify FeatureMap pointer. */
BOOL uVerifyFeatureMap(FeatureMap fmap_ptr, char *caller, char *filename, int line_num) ;

#define verifyFeatureMap(FMAP_PTR, CALLER) \
uVerifyFeatureMap(FMAP_PTR, #CALLER, __FILE__, __LINE__)

/* find FeatureMap struct on graph and verify pointer */
FeatureMap uCurrentFeatureMap(char *caller, char *filename, int line_num) ;

#define currentFeatureMap(CALLER_FUNCNAME) \
uCurrentFeatureMap(CALLER_FUNCNAME, __FILE__, __LINE__)


BOOL fMapFindSegBounds (FeatureMap look, SegType type, int *min, int *max);
void fMapSetZoneEntry (char *zoneBuf) ;
float fMapSquash (float value, float midpoint) ;


BOOL fMapSMapConvert(FeatureMap look, Array oldSegs) ;
BOOL fMapConvert (FeatureMap look) ;

BOOL fMapAddMappedDict(DICT *dict, KEY obj) ;
BOOL fMapFindMappedDict(DICT *dict, KEY obj) ;

int fMapOrder (void *a, void *b) ;
void fMapRC (FeatureMap look) ;
extern char* fMapSegTypeName[] ;
void fMapProcessMethods (FeatureMap look) ;
BOOL fMapActivateGraph (void) ;
void fMapReportLine (FeatureMap look, SEG *seg, BOOL isGIF, float x) ;
BOOL fMapFindCoding(FeatureMap look) ;

void fMapMenuToggleDna (void) ;	/* menu/button function */
void fMapMenuAddGfSegs (void) ;	/* top-level menu func to start genefinder */
void fMapGelDisplay (void) ;
				/* fmapblast.c */
void fMapBlastInit (void) ;

				/* fmapmenes.c */
void fMapMenesInit (void) ;

				/* fmapsequence.c */
void fmapSetDNAHightlight(int new_hightlight) ;
#ifndef USE_SMAP
int sequenceLength (KEY seq) ;
#endif
void fMapSetDnaWidth (void) ;
void fMapToggleAutoWidth (void) ;
void fMapClear (void) ;
void fMapExportTranslations (void) ;

/* fMapGetCDS() calls fMapGetDNA with cds_only = TRUE.                       */
BOOL fMapCheckCDS(KEY key, int dna_min, int dna_max, char **err_msg_out) ;
BOOL fMapGetCDS (FeatureMap look, KEY parent, Array *cds, Array *index);
BOOL fMapGetDNA(FeatureMap look, KEY parent, Array *cds, Array *index, BOOL cds_only) ;

/* the functions for type MapColDrawFunc */
void fMapShowORF (LOOK genericLook, float *offset, MENU *menu) ;
void fMapShowDownGeneTranslation (LOOK genericLook, float *offset, MENU *menu) ;
void fMapShowUpGeneTranslation (LOOK genericLook, float *offset, MENU *menu) ;/*mhmp 17.06.98*/
void fMapShow3FrameTranslation (LOOK genericLook, float *offset, MENU *menu) ;
void fMapShowCoords (LOOK genericLook, float *offset, MENU *unused) ;
void fMapShowSummary (LOOK genericLook, float *offset, MENU *unused) ;
void fMapShowCptSites (LOOK genericLook, float *offset, MENU *unused) ;
void fMapShowCoding (LOOK genericLook, float *offset, MENU *unused) ;
void fMapShowOrigin (LOOK genericLook, float *offset, MENU *unused) ;
void fMapShowStatus (LOOK genericLook, float *offset, MENU *unused) ;
void fMapShowATG (LOOK genericLook, float *offset, MENU *unused) ;
void fMapShowDNA (LOOK genericLook, float *offset, MENU *unused) ;


				/* fmapfeatures.c */
void fMapAddSegments (void) ;
void fMapClearSegments (void) ;
void keyZoneAdd (Array a, KEY key, float y1, float y2, int iseg);
int  keyZoneOrder (void *va, void *vb) ;
void fMapRemoveSelfHomol (FeatureMap look) ;
 	/* the functions for type MapColDrawFunc */
void fMapShowMiniSeq (LOOK genericLook, float *offset, MENU *menu) ;
void fMapShowScale (LOOK genericLook, float *offset, MENU *menu) ;
void fMapShowSequence (LOOK genericLook, float *offset, MENU *menu) ;
void fMapShowSoloConfirmed (LOOK look, float *offset, MENU *menu) ;
void fMapShowEmblFeatures (LOOK look, float *offset, MENU *menu) ;
void fMapShowText (LOOK look, float *offset, MENU *menu) ;
void fMapShowFeature (LOOK look, float *offset, MENU *menu) ;
void fMapShowFeatureObj (LOOK look, float *offset, MENU *menu) ;
void fMapShowHomol (LOOK look, float *offset, MENU *menu) ;
void fMapShowBriefID (LOOK look, float *offset, MENU *menu) ;
			/* mieg, shift right by score */
void fMapShowGF_seg (LOOK look, float *offset, MENU *menu) ;
void fMapShowSplices (LOOK look, float *offset, MENU *menu) ;
void fMapShowAssemblyTags (LOOK look, float *offset, MENU *menu) ;
void fMapShowCDSBoxes (LOOK look, float *offset, MENU *menu) ;
void fMapShowCanonical (LOOK look, float *offset, MENU *menu) ;
void fMapShowGeneNames (LOOK look, float *offset, MENU *menu) ;
void fMapShowCDSLines (LOOK look, float *offset, MENU *menu) ;
void fMapShowCDNAs (LOOK look, float *offset, MENU *menu) ;
void fMapShowText (LOOK look, float *offset, MENU *menu) ;
void fMapShowUserSegments (LOOK look, float *offset) ;
void fMapShowAlleles (LOOK look, float *offset, MENU *menu) ;
void fMapDrawBox(FeatureMap look, int box_index, MENUOPT gene_menu[], SEG *seg,
		 float left, float top, float right, float bottom,
		 BOOL off_top, BOOL off_bottom,
		 int foreground, int background) ;

				/* fmapgene.c */
extern FREEOPT fMapChooseMenu[] ;
void fMapChooseMenuFunc (KEY key, int box) ;
extern MENUOPT fMapGeneOpts[] ;


void fMapAddGfSegs(FeatureMap look) ;
void fMapAddGfSegsFull(FeatureMap look, BOOL redraw) ;

				/* fmaposp.c */
void fMapOspInit (void) ;
void fMapOspDestroy (FeatureMap look) ;
BOOL fMapOspPositionOligo (FeatureMap look, SEG *seg, KEY oligo, int *a1p, int *a2p) ;
void fMapOspFindOligoPairs (FeatureMap look) ;    /* make virtual multiplets */
void fMapOspSelectOligoPair (FeatureMap look, SEG *seg) ;
void fMapOspShowOligoPairs (LOOK genericLook, float *offset, MENU *unused) ; /* MapColDrawFunc */
void fMapOspShowOligo (LOOK genericLook, float *offset, MENU *unused) ; /* MapColDrawFunc */
				/* fmapassembly.c */
void fMapShowAlignments (LOOK look, float *offset) ;
void fMapShowPreviousContigs (LOOK look, float *offset) ;
void fMapShowContig (LOOK look, float *offset) ;
void fMapShowTraceAssembly (LOOK look, float *offset) ;
void fMapShowVirtualDna (LOOK look, float *offset) ;
void fMapShowVirtualMultiplets (LOOK look, float *offset) ;
BOOL fMapFollowVirtualMultiTrace (int box) ;
void fMapTraceDestroy (FeatureMap look) ;
void fMapTraceFindMultiplets (FeatureMap look) ;  /* make virtual multiplets */
void fMapSelectVirtualMultiplet (FeatureMap look, SEG *seg) ;
void fMapShowCloneEnds (LOOK look, float *offset) ;
void fMapTraceForget (KEY key) ;
void fMapTraceForgetContig (KEY contig) ;

void fMapcDNAShowSplicedcDNA (LOOK look, float *offset, MENU *unused) ;
void fMapcDNAShowTranscript (LOOK look, float *offset, MENU *unused) ;
void fMapShowcDNAs (LOOK look, float *offset, MENU *unused) ;

BOOL fMapcDNAFollow (int box) ;
void fMapcDNAInit (FeatureMap look) ;
BOOL fMapcDNADoFillData(SEG *seg) ;
void fMapcDNAReportLine (char *buffer, SEG *seg1, int maxBuf) ;
void fMapcDNAReportTranscript (char *buffer, SEG *seg1, int maxBuf) ;

BOOL hexAddSegs (char *name, int type, KEY key, 
		 int step, float thresh, BOOL isRC,
		 char *dna, int len, float* partial) ;
BOOL geneFinderAce (char *seq, void *gf) ;

void abiFixFinish (void) ;

				/* fmapcurate.c */
void fMapCurateCDS (int box) ;


MENU fmapGetGeneOptsMenu(void) ;



/* this should also be in an fmaputils.c file, I want all non-static routines that are
 * internal to fmap to be named consistently and differently from the external routines,
 * how about fMapNNNN for externals and fmapNNNN for internals ?? */



/* This routine returns the name of the object from which this seg was derived, this
 * is _not_ just name(seg->key) */
char *fmapSeg2SourceName(SEG *seg) ;

/* As the name imples, this only works for homol strands currently, if you want it to work for others then add
 * the code to do that but note that it may not be simple is other coords hang off the seg. */
void fmapSegFlipHomolStrand(SEG *seg, HOMOLINFO *hinf) ;


/* Currently in smapconvert.c, should be in fmapcontrol probably, or better  */
/* an fmaputils.c file.                                                      */
void fMapLookDump(FeatureMap look) ;


/* debug functions... */
void fmapPrintSeg(FeatureMap look, SEG *seg) ;
void fmapPrintStdOutSeg(SEG *seg) ;
void fmapDumpSegs(Array segs, ACEOUT dest) ;


void fmapAddToSet (ACEIN command_in, DICT *dict) ;
void fmapSetZoneUserCoords (FeatureMap look, int x0, int x1) ;

#endif /* ACEDB_FMAP_P_H */

