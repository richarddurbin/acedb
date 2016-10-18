/*  File: fmapsegdump.h
 *  Author: Ed Griffiths (edgrif@sanger.ac.uk)
 *  Copyright (c) Sanger Institute, 2008
 *-------------------------------------------------------------------
 * ZMap is free software; you can redistribute it and/or
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
 *-------------------------------------------------------------------
 * This file is part of the ACEDB genome database package, originated by
 * 	Richard Durbin (MRC LMB, UK) rd@mrc-lmb.cam.ac.uk, and
 *	Jean Thierry-Mieg (CRBM du CNRS, France) mieg@kaa.cnrs-mop.fr
 *
 * Description: Interface for generalised seg dumping from fmap.
 *              By plugging in different output routines then a number
 *              of different formats can be supported from the same
 *              fmap segs array. Currently GFF and DAS.
 *              
 *              
 * HISTORY:
 * Last edited: Nov 23 15:58 2011 (edgrif)
 * Created: Wed Jan  9 09:10:31 2008 (edgrif)
 * CVS info:   $Id: fmapsegdump.h,v 1.6 2012-10-30 09:58:15 edgrif Exp $
 *-------------------------------------------------------------------
 */
#ifndef DEF_FMAPSEGDUMP_H
#define DEF_FMAPSEGDUMP_H

#include <glib.h>
#include <w7/fmap_.h>


/* SEG's can represent the whole feature (e.g. an alignment) or part of a feature
 * (e.g. an exon), the SEG can have a logical parent (e.g. Gene object), logical
 * children (e.g. exons) and so on. These flags are used to represent these
 * different relationships. */
#define FMAPSEGDUMPATTRS_NONE            0x00000000
#define FMAPSEGDUMPATTRS_IS_FEATURE      0x00000001
#define FMAPSEGDUMPATTRS_HAS_SUBPART     0x00000002
#define FMAPSEGDUMPATTRS_IS_SUBPART      0x00000004
#define FMAPSEGDUMPATTRS_HAS_PARENT_OBJ  0x00000010
#define FMAPSEGDUMPATTRS_HAS_CHILD_OBJ   0x00000020
#define FMAPSEGDUMPATTRS_IS_MULTILINE    0x00000040
#define FMAPSEGDUMPATTRS_OMIT_NAME       0x00000100

typedef unsigned int FMapSegDumpAttr ;



/* acedb dumps segs in a predetermined order which may make writing the dumper
 * routine easier. In particular for compound objects consisting of a number of
 * segs, e.g. transcripts, a parent seg encompassing the whole feature is always
 * written out first before the subparts e.g. introns and exons. This makes it
 * much easier to output the "parent-child" information required by some formats
 * e.g. GFF V3. */


/* GFFv2 attributes separator */
#define V2_ATTRS_SEPARATOR " ; "


/* Fields with unknown value are given the value '.' */
#define FMAPSEGDUMP_UNKNOWN_FIELD "."



/* Macro to format the features name:
 * 
 * if USE_NAME         Name "class_name obj_name"
 * 
 * else if ALT_NAME    alt_name "class_name:obj_name"
 * 
 * else                class_name "obj_name"
 * 
 * Note:     USE_NAME is a boolean
 *           ALT_NAME is a string or the empty string "", _not_ NULL !!
 *           KEY is a valid acedb key representing an object
 * 
 *  */
#define FORMATFEATURENAME(GSTRING, USE_NAME, ALT_NAME, KEY)	\
  do {                                                          \
    if ((USE_NAME))                                             \
      g_string_append_printf((GSTRING),                         \
			     "Class \"%s\"" V2_ATTRS_SEPARATOR "Name \"%s\"" V2_ATTRS_SEPARATOR,	\
			     className((KEY)),			\
			     name((KEY))) ;                     \
    else if (*(ALT_NAME))                                       \
      g_string_append_printf((GSTRING), "%s \"%s:%s\"",	\
			     (char *)(ALT_NAME),		\
			     className((KEY)),			\
			     name((KEY))) ;			\
    else							\
      g_string_append_printf((GSTRING), "%s \"%s\"",		\
			     className((KEY)),			\
			     name((KEY))) ;			\
  } while (0)



/* FMapSegDumpDump can output a list of features or keys instead of the usual FMapSegDump output. */
typedef enum
  {
    FMAPSEGDUMP_INVALID,
    FMAPSEGDUMP_LIST_FEATURES,				    /* List all features that would be dumped. */
    FMAPSEGDUMP_LIST_KEYS,				    /* List all objects that would be dumped. */
    FMAPSEGDUMP_FEATURES				    /* Dump the whole features. */
  } FMapSegDumpListType ;



/* The type of landmark/feature: basic, transcript or alignment. */
typedef enum
  {
    FMAPSEGDUMP_TYPE_INVALID,
    FMAPSEGDUMP_TYPE_BASIC,
    FMAPSEGDUMP_TYPE_SEQUENCE,
    FMAPSEGDUMP_TYPE_ALIGNMENT,
    FMAPSEGDUMP_TYPE_TRANSCRIPT,
  } FMapSegDumpBasicType ;


/* Gives subparts of a transcript. */
typedef enum
  {
    FMAPSEGDUMP_TRANSSUB_INVALID,
    FMAPSEGDUMP_TRANSSUB_SEQUENCE,
    FMAPSEGDUMP_TRANSSUB_EXON,
    FMAPSEGDUMP_TRANSSUB_INTRON,
  } FMapSegDumpTranscriptSubPart ;


/* What sort of alignment of reference/match, not sure if PEPDNA is needed. */
typedef enum
  {
    FMAPSEGDUMP_ALIGN_INVALID,
    FMAPSEGDUMP_ALIGN_DNADNA,
    FMAPSEGDUMP_ALIGN_DNAPEP,
    FMAPSEGDUMP_ALIGN_PEPDNA
  } FMapSegDumpAlignType ;


typedef struct FMapSegDumpBasicStructType
{
  char *dummy_name ;

} FMapSegDumpBasicStruct, *FMapSegDumpBasic ;

typedef struct FMapSegDumpSequenceStructType
{
  KEY known_name ;

} FMapSegDumpSequenceStruct, *FMapSegDumpSequence ;

/* Represents a section of ungapped alignment, reference/match nomenclature is as
 * for GFF3, i.e. the "match" is aligned to the "reference". */
typedef struct FMapSegDumpAlignStructType
{
  int ref_start, ref_end ;
  int match_start, match_end ;
} FMapSegDumpAlignStruct, *FMapSegDumpAlign ;

typedef struct FMapSegDumpTranscriptStructType
{
  FMapSegDumpTranscriptSubPart sub_part ;
  gboolean start_not_found, end_not_found ;
  int start_offset ;
} FMapSegDumpTranscriptStruct, *FMapSegDumpTranscript ;


typedef struct FMapSegDumpMatchStructType
{
  KEY target ;
  FMapSegDumpAlignType align_type ;
  int start, end ;
  int length ;
  char strand ;						    /* '+'/'-' for DNA aligns, '.'
							       otherwise. */

  char *align_id ;					    /* Grouping id for gffv3 output. */

  /* If !format then gaps give gapped alignment, otherwise align_str gives alignment in format
   * given by format (e.g. CIGAR etc). */
  SMapAlignStrFormat str_format ;
  GArray *gaps ;					    /* Array of FMapSegDumpFeatureStruct. */
  char *align_str ;					    /* align string. */

  gboolean own_sequence ;				    /* Target object contains it's raw sequence. */

} FMapSegDumpMatchStruct, *FMapSegDumpMatch ;





/* Extra's for the line dump. */
typedef struct FMapSegDumpLineStructType
{
  BOOL verbose ;
  BOOL report_all_errors ;

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE

  /* Probably we don't want all of these in here.... */
  KEY   seqKey;
  char *methodName;
  BOOL is_structural;
  char *sourceName;
  char *featName;
  int   x, y;
  float score;
  BOOL  isScore;
  char  strand;
  char  frame;
  char *attributes;

#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

  char *comment ;

  /* A bit hacky until gffv3 comes along when we can remove these..... */
  gboolean zmap_dump ;					    /* hack for now.... */
  gboolean extended_features ;


  /* subpart information for the various types. "type" says which one of these is valid. */
  FMapSegDumpBasicType type ;
  union
  {
    FMapSegDumpBasicStruct basic ;
    FMapSegDumpSequenceStruct sequence ;
    FMapSegDumpMatchStruct match ;
    FMapSegDumpTranscriptStruct transcript ;
  } data ;



} FMapSegDumpLineStruct, *FMapSegDumpLine ;




/* Seg dumping functions. */
BOOL fMapDumpGFF(FeatureMap look, int version, BOOL use_ids,
		 KEY *refSeq, int *key_start_out, int *key_end_out,
		 FMapSegDumpListType gff_list, BOOL raw_methods,
		 BOOL verbose, BOOL report_all_errors,
		 DICT *sourceSet, DICT *featSet, DICT *methodSet,
		 BOOL include_source, BOOL include_feature, BOOL include_method,
		 char *id_prefix, BOOL zmap_dump, BOOL extended_features, ACEOUT gff_out) ;




/* Callback for FMapSegDump dumping, caller must pass a function that will take the processed
 * FMapSegDump record and output it in a suitable style. app_data may be used to pass information
 * through to this callback.
 * 
 * Returns TRUE on success, FALSE and an error message in err_msg_out otherwise (use
 * messfree to free err_msg_out when finished with).
 *  */
typedef BOOL (*FMapSegDumpCallBack)(void *app_data,
				    char *seqname, char *seqclass,
				    KEY method_obj_key, char *method,
				    BOOL mapped,
				    FMapSegDumpAttr seg_attrs,
				    KEY subpart_tag,
				    /* we need to canonicalise these two types..... */
				    KEY parent, SegType parent_segtype, char *parent_type,
				    KEY feature, SegType feature_segtype, char *feature_type,
				    KEY source_key,

				    FMapSegDumpBasicType seg_basic_type,
				    char *source,
				    int start, int end,
				    float *score, char strand, char frame,
				    char *attribute, char *comment,
				    FMapSegDumpLine dump_data,
				    char **err_msg_out) ;







/* Get span of features etc. ready for input into FMapSegDumpDump. */
BOOL FMapSegDumpRefSeqPos(FeatureMap look, int version,
			  KEY *refSeq, KEY *seqKey_out, BOOL *reversed_out,
			  int *offset_out,
			  int *key_start_out, int *key_end_out,
			  int *feature_start_out, int *feature_end_out) ;

/* Output the data, note that app_cb must be supplied. */
BOOL FMapSegDump(FMapSegDumpCallBack app_cb, void *app_data,
		 FeatureMap look, int version,
		 KEY seqKey, int offset, int feature_start, int feature_end,
		 FMapSegDumpListType list_type, DICT listset, BOOL raw_methods, Array stats,
		 DICT *sourceSet, DICT *featSet, DICT *methodSet,
		 BOOL include_source, BOOL include_feature, BOOL include_method,
		 BOOL zmap_dump, BOOL extended_features,
		 BOOL verbose, BOOL report_all_errors) ;


BOOL fMapGifSetGFFVersion(int gff_version) ;



#endif /*  !defined DEF_FMAPSEGDUMP_H */
