/*  File: zmap_.h
 *  Author: Gemma Barson (gb10@sanger.ac.uk)
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
 *      This is the internal header file for the zmap package, this file
 *      _must_only_ be included by zmap source files. The public header for
 *      users of the zmap package is zmap.h.
 *
 *-------------------------------------------------------------------
 */
#ifndef ACEDB_ZMAP_P_H
#define ACEDB_ZMAP_P_H

#include <wh/acedb.h>
#include <wh/lex.h>
#include <wh/dict.h>
#include <wh/bitset.h>
#include <wh/graph.h>
#include <wh/fmap.h>					    /* Our public header. */
#include <wh/methodcache.h>
#include <wh/smap.h>


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

#define ZMAP_COMPLEMENTED(LOOK) ((LOOK)->flag & FLAG_COMPLEMENT)


/* All possibilities of strand */
typedef enum
  {
    FMAPSTRAND_INVALID,
    FMAPSTRAND_FORWARD,					    /* Also known as '+' */
    FMAPSTRAND_REVERSE,					    /* Also known as '-' */
    FMAPSTRAND_NONE,					    /* Also known as '.' */
    FMAPSTRAND_UNKNOWN					    /* Also known as '?' */
  } FMapStrand ;


/* Generalised feature struct. */
typedef struct
{
  KEY method ;
} FeatureInfo ;


/* Used to specify whether the fmap should be recomputed or not, FMAP_RECOMPUTE_ALREADY
 * is necessary to avoid recursion. */
typedef enum {FMAP_RECOMPUTE_NO, FMAP_RECOMPUTE_YES, FMAP_RECOMPUTE_ALREADY} FMapRecompute ;


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
  VIRTUAL_MULTIPLET_TAG,
  VIRTUAL_MULTIPLET_TAG_UP,
  VIRTUAL_ALIGNED_TAG,
  VIRTUAL_ALIGNED_TAG_UP,
  VIRTUAL_PREVIOUS_CONTIG_TAG,
  VIRTUAL_PREVIOUS_CONTIG_TAG_UP,

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


/* XXX This has to match the GfInfo struct in w9/gfcode.c XXX */
typedef struct
{
  int   min, max ;
  float *cum, *revCum ;
} GFINFO ;


/* All objects passed to zmap are represented by "segs", a combination of the
 * seg and information in the 'data' union is used to draw the feature. */
typedef struct
{
  KEY	  key ;						    /* object this seg is derived from. */
  KEY     parent ;					    /* Possibly a different parent object. */
  KEY     source ;					    /* object the field comes from */
  KEY     tag2 ;					    /* e.g. "Feature", "DNA_homol" etc. */

  SegType parent_type ;					    /* Type of parent seg, 0 if none. */
  SegType type ;					    /* ALLELE, SEQUENCE etc. */

  int	  x1, x2 ;					    /* Position in virtual sequence. */
  unsigned int flags ;					    /* for _per_ seg info. */

  unsigned char data_type ;				    /* Records type of data in the data union. */
  union
  {
    float  f ;
    int    i ;
    KEY    k ; 
    char  *s ;
  } data ;						    /* utility slot */

  /* I'd like to have different size segs for different things.... */
  union
  {
    int exon_phase ;
  } feature_data ;					    /* Feature specific data. */

} SEG ;


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


/* A sequence object that has Source_Exons may have a CDS defined within it,
 * this needs special handling for colour/position of CDS, protein translation etc. */
typedef struct
{
  /* should be able to just point to cds seg ???? */
  SEG cds_seg ;						    /* Position of CDS */

  BOOL cds_only ;					    /* TRUE => Protein translate only CDS */
							    /* exons. */
} CDS_Info ;


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
  char *match_str ;					    /* match string. */

  KEY cluster_key ;					    /* Key to be used for "cluster" column display. */
} HOMOLINFO ;




#endif /* ACEDB_ZMAP_P_H */
