/*  File: fmapsegdumpgff.c
 *  Author: Ed Griffiths (edgrif@sanger.ac.uk)
 *  Copyright (c) Sanger Institute, 2008
 *-------------------------------------------------------------------
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
 *-------------------------------------------------------------------
 * This file is part of the ACEDB genome database package, originated by
 * 	Richard Durbin (Sanger Centre, UK) rd@sanger.ac.uk, and
 *	Jean Thierry-Mieg (CRBM du CNRS, France) mieg@kaa.crbm.cnrs-mop.fr
 *
 * Description: Functions to dump GFF (versions 2 & 3) format data from
 *              an FMap segs array.
 *              
 * Exported functions: See fmap.h
 *              
 * HISTORY:
 * Last edited: Nov 23 18:53 2011 (edgrif)
 * Created: Wed Jan  9 10:08:20 2008 (edgrif)
 * CVS info:   $Id: fmapsegdumpgff.c,v 1.8 2012-10-30 09:59:42 edgrif Exp $
 *-------------------------------------------------------------------
 */

#include <glib.h>
#include <wh/regular.h>
#include <wh/aceio.h>
#include <wh/acedb.h>
#include <wh/lex.h>
#include <wh/bindex.h>
#include <wh/so.h>
#include <whooks/classes.h>
#include <whooks/systags.h>
#include <w7/fmapsegdump.h>


/* Some time this will change to be version 3....I guess.... */
enum
  {
    DEFAULT_GFF_VERSION = 2
  } ;



/*
 *                      GFF v3 data structs/types.
 * 
 * GFF v3 introduces SO accession numbers (SO_id below) as the standard nomenclature
 * for the "type" field with SOFA terms (SOFA_term below) as an alternative, therefore
 * SO numbers will be the primary key in my code.
 * 
 */



/* Let's migrate to Quarks as much as possible.... */
/* Used to make a table to translate various acedb/GFFv2 field combinations into so_terms, these
 * might be source/feature_name or source/classname etc. */
typedef struct
{
  char *key_1 ;						    /* part 1 of key. */
  char *key_2 ;						    /* part 2 of key */
  char *SO_id ;						    /* Corresponding SO id. */
} FeatureToSOStruct, *FeatureToSO ;




#ifdef ED_G_NEVER_INCLUDE_THIS_CODE

/* NOT USED CURRENTLY. */

/* Holds a mapping from a SO number to a SOFA term, this is a one way mapping because all
 * features have a SO number but not all features have a SOFA term. */
typedef struct
{
  GQuark SO_id ;
  GQuark SOFA_term ;
} SO2SOFAStruct, *SO2SOFA ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */



/* Holds a mapping from an FMap SegType to a SO id. */
typedef struct
{
  SegType seg_id ;
  SegType seg_id_up ;
  char *ace_2_so_tag ;
} Seg2SOStruct, *Seg2SO ;


/* Different SO_terms have different requirements, some imply parenthood (e.g. transcript, some are potentially
 * multi-line (e.g. CDS) and so on. */
typedef struct
{
  char *SO_id ;
  char *SO_name ;
  char *SOFA_term ;					    /* Is this different from SO_name ?? */
} SOPropertyStruct, *SOProperty ;


/* State data we need during dumping. */
typedef struct
{
  ACEOUT gff_out ;

  BOOL use_SO_acc ;					    /* TRUE => Use names, default to ids. */

  GHashTable *SO_terms ;

  char *id_prefix ;					    /* User-specified prefix for all feature IDs. */

  int dump_index ;					    /* Used when making a single index for
							       all features in a dump. */

  GHashTable *feature_2_GFFID ;				    /* Maps a named feature to its GFF
							       file ID. */

  GHashTable *SO_2_index ;				    /* Maps a SO_id to current index for
							       gff ids for that SO_id. */

  GHashTable *name_2_ID ;				    /* NOT sure we need this now.... */

  DICT *mapped_dict ;					    /* Names of mapped feature objects. */

} GFF3DumpDataStruct, *GFF3DumpData ;



typedef struct ParentDataStructName
{
  KEY parent_key ;
  KEY parent_method_key ;
  char *parent_id ;
} ParentDataStruct, *ParentData ;




/* gif related functions. */
static BOOL writeGFF2Line(void *app_data,
			  char *seqname, char *seqclass,
			  KEY method_obj_key, char *method,
			  BOOL is_structural,
			  FMapSegDumpAttr seg_attrs,
			  KEY subpart_tag,
			  KEY parent, SegType parent_segtype, char *parent_type,
			  KEY feature, SegType segType, char *feature_type,
			  KEY source_key,
			  FMapSegDumpBasicType seg_basic_type,
			  char *source,
			  int start, int end,
			  float *score, char strand, char frame,
			  char *attributes, char *comment,
			  FMapSegDumpLine dump_data,
			  char **err_msg_out) ;

static BOOL writeGFF3Line(void *app_data,
			  char *seqname, char *seqclass,
			  KEY method_obj_key, char *method,
			  BOOL is_structural,
			  FMapSegDumpAttr seg_attrs,
			  KEY subpart_tag,
			  KEY parent, SegType parent_segtype, char *parent_type,
			  KEY feature, SegType segType, char *feature_type,
			  KEY source_key,
			  FMapSegDumpBasicType seg_basic_type,
			  char *source,
			  int start, int end,
			  float *score, char strand, char frame,
			  char *attributes, char *comment,
			  FMapSegDumpLine dump_data,
			  char **err_msg_out) ;

static gboolean getObjSOName(KEY feature, SegType feature_segtype, KEY method_obj_key,
			     KEY *so_term_key_out, KEY *so_name_key_out, KEY *ace2so_key_out,
			     char **err_msg_out) ;
static gboolean getParentSOterm(KEY feature_key, SegType feature_segtype,
				KEY feature_ace2so_key,
#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
				KEY *parent_key, KEY *parent_method_key_out,
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */
				Array parents,
				char **err_msg_out) ;

static BOOL getSubpartSOterm(KEY ace2so_key, KEY subpart_tag) ;


GHashTable *createSOTerms(void) ;
SOProperty createSOList(void) ;


#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
char *segType2SO(SegType seg_type) ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

char *acedbFeature2SO(char *source, char *feature_type, char *class) ;

static GHashTable *makeAcedb2SOHash(void) ;
static GHashTable *makeSourceClass2SOHash(void) ;
static GHashTable *makeHash(FeatureToSO table) ;
void makeKeyStr(GString *key, char *feature_type, char *source) ;

static gboolean makeGapsString(GString *gaps_str, GArray *gaps,
			       int ref_start, int ref_end,
			       int match_start, int match_end, FMapSegDumpAlignType align_type) ;

static char *getGFFID(GHashTable *feature_2_GFFID, GHashTable *SO_2_index, int *dump_index_inout,
		      GString *key_str_inout, GString *id_str_inout,
		      KEY feature, KEY so_term_key, KEY so_name_key,
		      BOOL verbose) ;
static char *makeFeature2FileIDKeyStr(GString *str_inout, KEY feature, char *SO_term) ;
static char *makeFileID(GString *str_inout, GHashTable *SO_2_index, int *dump_index_inout,
			char *SO_term, char *label, BOOL verbose) ;
static char *makeAceFileID(GString *str_inout, KEY feature) ;

static char *getGFFUserID(GHashTable *feature_2_GFFID,
			  GString *key_str_inout, GString *id_str_inout,
			  KEY feature, char *user_str) ;
static char *makeFeature2FileUserIDKeyStr(GString *str_inout, KEY feature, char *user_str) ;
static char *makeUserFileID(GString *str_inout, KEY feature, char *user_str) ;


#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
static KEY findAce2SO(KEY feature) ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */


#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
static KEY segType2SOTag(SegType seg_type) ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */


static char *makeErrMsg(KEY feature, SegType feature_segtype, KEY method_obj_key,
			char *format, ...) ;





/* 
 *                    Globals
 */

static int gff_version_G = DEFAULT_GFF_VERSION ;




/* 
 *                    External interface
 */

/* Sets version of GFF to be dumped, returns TRUE if version is supported
 * otherwise returns FALSE and the version dumped is unchanged. */
BOOL fMapGifSetGFFVersion(int gff_version)
{
  BOOL result = FALSE ;

  if (gff_version == 2 || gff_version == 3)
    {
      gff_version_G = gff_version ;
      result = TRUE ;
    }

  return result ;
}

void fMapGifFeatures(ACEIN command_in, ACEOUT result_out, FeatureMap look, BOOL quiet)
{
  KEY refseq = 0 ;
  int x1, x2, key_start, key_end ;
  int version = gff_version_G ;
  char *word ;
  STORE_HANDLE handle = handleCreate() ;
  DICT *sourceSet = dictHandleCreate (16, handle) ;
  DICT *featSet   = dictHandleCreate (16, handle) ;
  DICT *methodSet = dictHandleCreate (16, handle) ;
  FMapSegDumpListType gff_list = FMAPSEGDUMP_FEATURES ;
  BOOL result ;
  BOOL raw_methods = FALSE;
  BOOL include_source  = FALSE;
  BOOL include_feature = FALSE;
  BOOL include_method  = TRUE;
  BOOL already_source  = FALSE;
  BOOL already_feature = FALSE;
  BOOL already_method  = FALSE;
  BOOL zmap_dump = FALSE ;
  ACEOUT fo = NULL, dump_out = NULL ;
  BOOL rev_comp = FALSE ;
  BOOL use_SO_acc = FALSE ;
  BOOL extended_features = FALSE ;
  BOOL verbose = FALSE, report_all_errors = FALSE ; 
  char *id_prefix = NULL ;



  messAssert(command_in && result_out && look) ;	    /* _no_ defaults. */

  /* by default output goes to same place as normal command results */
  dump_out = aceOutCopy (result_out, 0);

  while (aceInCheck (command_in, "w"))
    { 
      word = aceInWord (command_in) ;
      if (strcmp (word, "-coords") == 0
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
	  /* replace default output with file output */
	  aceOutDestroy (dump_out);
	  dump_out = fo;
	}
      else if (strcmp (word, "-id_prefix") == 0
	       && aceInCheck (command_in, "w"))
	{
	  if ((word = aceInWord(command_in)))
	    id_prefix = strnew(word, 0) ;
	  else
	    aceInBack (command_in) ;
	}
      else if (strcmp (word, "-refseq") == 0
	       && aceInCheck (command_in, "w") 
	       && (lexword2key (aceInWord (command_in), &refseq, _VSequence)))
	{ ; }
      else if (strcmp (word, "-rawmethods") == 0)
	{
	  raw_methods = TRUE ;
	}
      else if (strcmp (word, "-so_acc") == 0)
	{
	  use_SO_acc = TRUE ;
	}
      else if (strcmp (word, "-verbose") == 0)
	{
	  verbose = TRUE ;
	}
      else if (strcmp (word, "-report_all_errors") == 0)
	{
	  report_all_errors = TRUE ;
	}
      else if (strcmp (word, "-extended") == 0)
	{
	  extended_features = TRUE ;
	}
      else if (strcmp (word, "-version") == 0 && aceInInt(command_in, &version))
	{
	  if (version != 2 && version != 3)
	    {
	      aceOutPrint(result_out, "// Unsupported GFF version: %d.", version) ;
	      goto usage ;
	    }
	}
      else if (strcmp (word, "-list") == 0)
	{ 
	  gff_list = FMAPSEGDUMP_LIST_FEATURES ;

	  if (aceInCheck(command_in, "w"))
	    {
	      word = aceInWord(command_in) ;
	      
	      if (strcmp (word, "features") == 0)
		gff_list = FMAPSEGDUMP_LIST_FEATURES ;
	      else if (strcmp (word, "keys") == 0)
		gff_list = FMAPSEGDUMP_LIST_KEYS ;
	      else
		aceInBack (command_in) ;
	    }
	}
      else if (strcmp (word+1, "source") == 0)
	{
	  if (already_source || already_method) goto usage;
	  already_source = TRUE;

	  include_source = FALSE;
	  if (word[0] == '+')               /* check first, as aceInCheck shifts by one word */
	    {
	      include_source = TRUE;
	    }
	  if (aceInCheck (command_in, "w"))
	    {
	      fmapAddToSet (command_in, sourceSet) ;
	    }
	}
      else if (strcmp (word+1, "feature") == 0)
	{
	  if (already_feature) goto usage;
	  already_feature = TRUE;

	  include_feature = FALSE;
	  if (word[0] == '+')
	    {
	      include_feature = TRUE;
	    }
	  if (aceInCheck (command_in, "w"))
	    {
	      fmapAddToSet (command_in, featSet) ;
	    }
	}
      else if (strcmp (word+1, "method") == 0)
	{
	  if (already_method || already_source) goto usage;
	  already_method = TRUE;

	  if (word[0] == '-')
	    {
	      include_method = FALSE;
	    }
	  if (aceInCheck (command_in, "w"))
	    {
	      fmapAddToSet (command_in, methodSet) ;
	    }
	}
      else if (strcmp (word+1, "zmap") == 0)
	{
	  zmap_dump = TRUE ;
	}
      else
	goto usage ;
    }


  /* The GFF dumper is not written to cope with an fmap that has been revcomp'd, so test
   * for it here and do the reverse if required. */
  if (look->flag & FLAG_COMPLEMENT)
    {
      fMapRC(look) ;
      rev_comp = TRUE ;
    }

  result = fMapDumpGFF(look, version, use_SO_acc, &refseq, &key_start, &key_end,
		       gff_list, raw_methods,
		       verbose, report_all_errors,
		       sourceSet, featSet,  methodSet,
		       include_source, include_feature, include_method,
		       id_prefix, zmap_dump, extended_features, dump_out) ;

  /* and return to where we were if required. */
  if (rev_comp)
    fMapRC(look) ;

  if (id_prefix)
    messfree(id_prefix) ;

  if (result && !quiet)
    aceOutPrint (result_out, "// FMAP_FEATURES \"%s\" %d %d\n", name(refseq), key_start, key_end) ;

  aceOutDestroy(dump_out) ;
  handleDestroy (handle) ;

  return ;

usage:
  aceOutDestroy (dump_out);

  aceOutPrint (result_out, "// gif seqfeatures error, usage: "
	       "SEQFEATURES [-coords x1 x2] [-file fname] [-refseq sequence] [-version 2] "
	       "[-list [features | keys] default is features] [+-feature feature(s)] "
	       "[+-source source(s) | +-method method(s)]\n") ;
  handleDestroy (handle) ;

  return ;
}





/* Dump in standard GFF format, note that the actual output of features is done
 * by a callback routine that we register with the GFFDump routine.
 * 
 * This function is also called from an fmap menu.
 *  */
BOOL fMapDumpGFF(FeatureMap look, int version, BOOL use_SO_acc,
		 KEY *refSeq, int *key_start_out, int *key_end_out,
		 FMapSegDumpListType gff_list, BOOL raw_methods,
		 BOOL verbose, BOOL report_all_errors,
		 DICT *sourceSet, DICT *featSet, DICT *methodSet,
		 BOOL include_source, BOOL include_feature, BOOL include_method,
		 char *id_prefix, BOOL zmap_dump, BOOL extended_features, ACEOUT gff_out)
{
  BOOL result ;
  void *app_data ;
  KEY seqKey ;
  int offset, feature_start, feature_end ;
  BOOL reversed ;
  STORE_HANDLE handle = NULL ;
  DICT *listSet = NULL ;
  Array stats = NULL ;
  FMapSegDumpCallBack gff_write_func ;
  GFF3DumpData gff_data = NULL ;


  messAssert(gff_list && (version == 2 || version == 3)) ;


  result = FMapSegDumpRefSeqPos(look, version,
				refSeq, &seqKey, &reversed,
				&offset, key_start_out, key_end_out,
				&feature_start, &feature_end) ;

  if (!result)                 /* if no segs mapped, return FALSE */
    return result;

  if (refSeq)
    *refSeq = seqKey ;

  if (gff_list == FMAPSEGDUMP_FEATURES)
    {
      char delim ;

      /* I don't know if tab delimiters are ok in a header in gff v2...check this out. */
      delim = ' ' ;

      aceOutPrint (gff_out, "##gff-version%c%d\n", delim, version) ;

      if (version == 2)
	{
	  aceOutPrint (gff_out, "##source-version%c%s:%s\n", delim,
		       messGetErrorProgram(), aceGetVersionString()) ;
	  aceOutPrint (gff_out, "##date%c%s\n", delim, timeShow(timeParse ("today"))) ;
	}

      aceOutPrint (gff_out, "##sequence-region%c%s%c%d%c%d",
		   delim, name(seqKey), 
		   delim, *key_start_out, delim, *key_end_out) ;

      if (version == 2)
	aceOutPrint (gff_out, "%c%s",
		     delim, reversed ? "(reversed)" : "") ;

      aceOutPrint(gff_out, "\n") ;
    }
  else
    {
      handle = handleCreate() ;
      listSet = dictHandleCreate(64, handle) ;

      if (gff_list != FMAPSEGDUMP_LIST_FEATURES)
	stats = arrayHandleCreate (64, int, handle) ;
    }

  if (version == 2)
    {
      gff_write_func = writeGFF2Line ;

      app_data = (void *)gff_out ;
    }
  else
    {
      GDestroyNotify keyFree = (GDestroyNotify)g_free ;
      GDestroyNotify valueFree = (GDestroyNotify)g_free ;

      gff_write_func = writeGFF3Line ;

      gff_data = g_new0(GFF3DumpDataStruct, 1) ;

      gff_data->gff_out = gff_out ;

      gff_data->use_SO_acc = use_SO_acc ;
      gff_data->SO_terms = createSOTerms() ;
      gff_data->id_prefix = g_strdup(id_prefix) ;
      gff_data->feature_2_GFFID = g_hash_table_new_full(g_str_hash, g_str_equal, keyFree, valueFree) ;
      gff_data->SO_2_index = g_hash_table_new_full(g_str_hash, g_str_equal, keyFree, NULL) ;
      gff_data->name_2_ID = g_hash_table_new_full(g_str_hash, g_str_equal, keyFree, valueFree) ;

      gff_data->mapped_dict = look->mapped_dict ;

      app_data = (void *)gff_data ;
    }

  result = FMapSegDump(gff_write_func, app_data,
		       look, version, 
		       *refSeq, offset, *key_start_out-1, *key_end_out-1,
		       gff_list, listSet, raw_methods, stats,
		       sourceSet, featSet, methodSet,
		       include_source, include_feature, include_method,
		       zmap_dump, extended_features,
		       verbose, report_all_errors) ;

  if (gff_list != FMAPSEGDUMP_FEATURES)
    {
      int i ;

      if (gff_list == FMAPSEGDUMP_LIST_FEATURES)
	{
	  for (i = 0 ; i < arrayMax(stats) ; i++)
	    {
	      aceOutPrint(gff_out, "%s\t%d\n", dictName (listSet, i), arr(stats,i,int)) ;
	    }
	}
      else
	{
	  for (i = 0 ; i < dictMax(listSet) ; ++i)
	    {
	      aceOutPrint(gff_out, "%s\n", dictName(listSet, i)) ;
	    }
	}

      handleDestroy(handle) ;
    }


  /* For GFF v3 dumping there is some clearing up to do. */
  if (version == 3)
    {
      g_hash_table_destroy(gff_data->name_2_ID) ;
      g_hash_table_destroy(gff_data->SO_2_index) ;
      g_hash_table_destroy(gff_data->feature_2_GFFID) ;
      g_hash_table_destroy(gff_data->SO_terms) ;

      g_free(gff_data) ;
    }


  return result ;
}


/* Callback function (see fmapseqdump.h) which is called from GFFDump() to output GFF data. */
static BOOL writeGFF2Line(void *app_data,
			  char *seqname, char *seqclass,
			  KEY method_obj_key, char *method,
			  BOOL is_structural,
			  FMapSegDumpAttr seg_attrs,
			  KEY subpart_tag,
			  KEY parent, SegType parent_segtype, char *parent_type,
			  KEY feature, SegType feature_segtype, char *feature_type,
			  KEY source_key,
			  FMapSegDumpBasicType seg_basic_type,
			  char *source,
			  int start, int end,
			  float *score, char strand, char frame,
			  char *attributes, char *comments,
			  FMapSegDumpLine dump_data,
			  char **err_msg_out)
{
  BOOL result = TRUE ;					    /* Nothing to fail on currently. */
  ACEOUT gff_out = (ACEOUT)app_data ;
  static GString *dump_attributes = NULL ;
  BOOL use_name = FALSE ;

  if (dump_data->zmap_dump)
    {
      use_name = TRUE ;
    }


  if (!dump_attributes)
    dump_attributes = g_string_sized_new(2000) ;	    /* Guess at size, probably reasonable. */
  else
    dump_attributes = g_string_set_size(dump_attributes, 0) ; /* Reset the buffer. */

#ifdef DEBUG
    aceOutPrint (gff_out, "type: %12s,\tM: %18s,\t%s, \t%d, \t%d, \tS: %18s,\tF:%.20s",
		 typeStr, method, seqname, start, end, source, feature);
#else
  if (score)
    aceOutPrint (gff_out, "%s\t%s\t%s\t%d\t%d\t%g\t%c\t%c",
		 seqname, source, feature_type, 
		 start, end, *score, strand, frame) ;
  else  /* if no score, print a dot instead ---V */
    aceOutPrint (gff_out, "%s\t%s\t%s\t%d\t%d\t.\t%c\t%c",
		 seqname, source, feature_type,
		 start, end, strand, frame) ;

  /* will need changing as attribute stuff gets moved around. */
  if (attributes)
      aceOutPrint (gff_out, "\t") ;

  if (dump_data->type == FMAPSEGDUMP_TYPE_ALIGNMENT)
    {
      FMapSegDumpMatch match = &(dump_data->data.match) ;

      FORMATFEATURENAME(dump_attributes, use_name, "Target", match->target) ;

      if (dump_data->zmap_dump)
	{
	  int start, end ;

	  if (match->strand == '-')
	    {
	      end = match->start ;
	      start = match->end ;
	    }
	  else
	    {
	      start = match->start ;
	      end = match->end ;
	    }

	  g_string_append_printf(dump_attributes, "Align %d %d %c" V2_ATTRS_SEPARATOR, start, end, match->strand) ;
	}
      else
	{
	  g_string_append_printf(dump_attributes, " %d %d", match->start, match->end) ;

	  if (dump_data->extended_features)
	    g_string_append_printf(dump_attributes, " %c", match->strand) ;
	}


      if (dump_data->zmap_dump)
	{
	  if (match->str_format)
	    {


	    }

	  if (match->gaps->len > 0)
	    {
	      GArray *gaps = match->gaps ;
	      GString *str = g_string_new("Gaps \"");
	      int i;
		    
	      /* Gaps are output as start < end and ref coords followed by match coords. */
	      for (i = 0 ; i < match->gaps->len ; i++) 
		{
		  FMapSegDumpAlign gap = &g_array_index(gaps, FMapSegDumpAlignStruct, i)  ;
		  int ref_start, ref_end, match_start, match_end ;

		  if (gap->ref_start <= gap->ref_end)
		    {
		      ref_start = gap->ref_start ;
		      ref_end = gap->ref_end ;
		    }
		  else
		    {
		      ref_start = gap->ref_end ;
		      ref_end = gap->ref_start ;
		    }

		  if (gap->match_start <= gap->match_end)
		    {
		      match_start = gap->match_start ;
		      match_end = gap->match_end ;
		    }
		  else
		    {
		      match_start = gap->match_end ;
		      match_end = gap->match_start ;
		    }


		  g_string_append_printf(str, "%d %d %d %d,", ref_start, ref_end,
					 match_start, match_end) ;
		}

	      /* replace trailing comma with closing quote */
	      strcpy(str->str + (strlen(str->str) - 1), "\"") ;

	      g_string_append_printf(dump_attributes, "%s" V2_ATTRS_SEPARATOR, str->str);
	      
	      g_string_free(str, TRUE);
	    }

	  if (match->length)
	    {
	      g_string_append_printf(dump_attributes, "Length %d" V2_ATTRS_SEPARATOR, match->length) ;
	    }

	  if (match->own_sequence)
	    {
	      g_string_append(dump_attributes, "Own_Sequence TRUE" V2_ATTRS_SEPARATOR) ;
	    }
	}

      aceOutPrint (gff_out, "%s", dump_attributes->str) ;

    }

  if (attributes)
    aceOutPrint (gff_out, "%s", attributes) ;
  
  if (comments)
    aceOutPrint (gff_out, "%s", comments) ;

#endif

  aceOutPrint (gff_out, "\n") ;

  return result ;
}


/* Writes out lines in GFF version 3 format.
 * 
 * Function relies on receiving parent records before child records,
 * e.g. expects a "transcript" record _before_ any of the introns
 * or exons for that transcript.
 * 
 *  */
static BOOL writeGFF3Line(void *app_data,
			  char *seqname, char *seqclass,
			  KEY method_obj_key, char *method, /* I want the key to take over but
							       one step at a time... */
			  BOOL is_structural,
			  FMapSegDumpAttr seg_attrs,
			  KEY subpart_tag,
			  KEY parent, SegType parent_segtype, char *parent_type,
			  KEY feature, SegType feature_segtype, char *feature_type,
			  KEY source_key,
			  FMapSegDumpBasicType seg_basic_type,
			  char *source, int start, int end,
			  float *score, char strand, char frame,
			  char *attributes, char *comment,
			  FMapSegDumpLine dump_data,
			  char **err_msg_out)
{
  BOOL result = TRUE ;
  GFF3DumpData gff_data = (GFF3DumpData)app_data ;
  ACEOUT gff_out = gff_data->gff_out ;
  static GString *float_str = NULL ;
  static GString *so_gff_field_str = NULL ;

   /* Should be in the intial struct really so they can be g_free'd later... */
  static GString *parent_name_str = NULL ;
  static GString *feature_name_str = NULL ;
  static GString *gff_id_key_str = NULL ;
  static GString *feature_id_str = NULL ;
  static GString *parent_id_str = NULL ;
  static Array parents = NULL ;


#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
  char *parent_id = NULL ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

  char *feature_id = NULL ;				    /* Could be because feature is a
							       parent or because its multiline. */
  gboolean attr_delim ;
  static GString *dump_attributes = NULL ;
  gboolean use_SO_acc = gff_data->use_SO_acc ;		    /* TRUE => use SO accession, FALSE => use SO name. */
  KEY gff_so_id = KEY_NULL ;				    /* Set according to use_SO_acc */

  KEY so_term_key = KEY_NULL, so_name_key = KEY_NULL, ace2so_key = KEY_NULL ;

  /* We should keep a record of parent/child id creation and report any parent ids where we do not
   * end up outputting a record for the parent feature as this will violate GFFv3 rules. */
  BOOL report_all_errors = dump_data->report_all_errors ;
  BOOL verbose = dump_data->verbose ;
  gboolean debug = FALSE ;


  /* Set up some statics used for processing the stream of input. */
  if (!dump_attributes)
    dump_attributes = g_string_sized_new(2000) ;	    /* Guess at size, probably reasonable. */


  if (!feature_name_str)
    {
      /* Never free'd currently..... */
      float_str = g_string_sized_new(100) ;
      so_gff_field_str = g_string_sized_new(100) ;
      parent_name_str = g_string_sized_new(1000) ;
      feature_name_str = g_string_sized_new(1000) ;
      gff_id_key_str = g_string_sized_new(1000) ;

      feature_id_str = g_string_sized_new(1000) ;
      parent_id_str = g_string_sized_new(1000) ;

      parents = arrayCreate(20, ParentDataStruct) ;
    }


  /* We should be testing input consistency here but better really to rearrange input params so
   * they enforce consistency, e.g. put all parent stuff together etc etc.... */
  if (!method_obj_key)
    {
      if (report_all_errors)
	*err_msg_out = hprintf(0, "\"%s\", class \"%s\", type \"%s\" not dumped:"
			       " no method specified.",
			       name(feature), className(feature),
			       fMapSegTypeName[feature_segtype]) ;

      result = FALSE ;
    }



  /* Check whether the feature has all the right SO tags. */
  if (result && (result = getObjSOName(feature, feature_segtype, method_obj_key,
				       &so_term_key, &so_name_key, &ace2so_key, 
				       err_msg_out)))
    {
      BOOL no_span ;


#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
      no_span = keyFindTag(ace2so_key, str2tag(ACE2SO_NO_SPAN)) ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */
      no_span = keyFindTag(method_obj_key, str2tag(METHOD_GFF_NO_SPAN)) ;


      if (seg_attrs & FMAPSEGDUMPATTRS_IS_FEATURE)
	{
	  if (no_span)
	    {
	      /* This is a feature but ace2so says "don't output a spanning line for the feature", we
	       * need for Coding region features, sets of alignments and probably other things. */
	      result = FALSE ;
	    }
	  else if (seg_attrs & FMAPSEGDUMPATTRS_IS_FEATURE)
	    {
	      /* This line represents a full feature, we need to give it an ID if it has children
	       * or is a multiline feature. (or if ID is forced ?? by user, add as an option...??) */
	      gboolean has_children ;
	      gboolean has_so_parent, so_parent_valid ; 
	      KEY parent_key = KEY_NULL, parent_method_key = KEY_NULL ;
	      KEY parent_so_term_key = KEY_NULL, parent_so_name_key = KEY_NULL, parent_ace2so_key = KEY_NULL ;

	      has_children = (keyFindTag(ace2so_key, str2tag(ACE2SO_CHILDREN))
			      || keyFindTag(ace2so_key, str2tag(ACE2SO_SUBPART))) ;

	      if (has_children || (seg_attrs & FMAPSEGDUMPATTRS_IS_MULTILINE))
		{
		  if (dump_data->type == FMAPSEGDUMP_TYPE_ALIGNMENT && dump_data->data.match.align_id)
		    {
		      /* For alignments user can specify a qualifier ID and then this is used to
		       * group several alignment features together. */
		      feature_id = getGFFUserID(gff_data->feature_2_GFFID,
						gff_id_key_str, feature_id_str,
						feature, dump_data->data.match.align_id) ;
		    }
		  else
		    {
		      feature_id = getGFFID(gff_data->feature_2_GFFID, gff_data->SO_2_index, &(gff_data->dump_index),
					    gff_id_key_str, feature_id_str,
					    feature, so_term_key, so_name_key, verbose) ;
		    }
		}



	      /* HERE WE WILL NEED TO DEAL WITH A _SET_ OF PARENT'S, AND NEED TO PRODUCE THEM
	       * IN THE FORM:   "Parent_id=first_id,second_id,etc." THEY WILL ALL NEED TO BE CHECKED
	       * FOR VALIDITY, MAPPING ETC.   */

	      /* See if feature has a SO parent defined that we can reference via a Parent= tag. */
	      if ((has_so_parent = getParentSOterm(feature, feature_segtype,
						   ace2so_key,
#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
						   &parent_key, &parent_method_key,
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */
						   parents,
						   err_msg_out)))
		{
		  int i ;

		  for (i = 0 ; i < arrayMax(parents) ; i++)
		    {
		      parent_key = arr(parents, i, ParentDataStruct).parent_key ;
		      parent_method_key = arr(parents, i, ParentDataStruct).parent_method_key ;

		      if (!(so_parent_valid = getObjSOName(feature, feature_segtype, parent_method_key,
							   &parent_so_term_key, &parent_so_name_key,
							   &parent_ace2so_key,
							   err_msg_out)))
			{
			  if (has_so_parent && !so_parent_valid)
			    *err_msg_out = hprintf(0, "\"%s\", class \"%s\", type \"%s\","
						   " has %s set to %s,"
						   " but this object does not have a valid SO Name"
						   " so \"Parent=\" attribute cannot be added !",
						   name(feature), className(feature), fMapSegTypeName[feature_segtype],
						   ACE2SO_PARENT, nameWithClass(parent_key)) ;
			}
		      else
			{
			  /* We need to make sure the parent was mapped otherwise it won't have been included in
			   * the set of features for the dump. */
			  if (!fMapFindMappedDict(gff_data->mapped_dict, parent_key))
			    {
			      *err_msg_out = hprintf(0, "\"%s\", class \"%s\", type \"%s\", has %s set to %s,"
						     " but this object was not mapped so \"Parent=\" attribute cannot be added !",
						     name(feature), className(feature),
						     fMapSegTypeName[feature_segtype],
						     ACE2SO_PARENT, nameWithClass(parent_key)) ;
			    }
			  else
			    {
			      arr(parents, i, ParentDataStruct).parent_id = getGFFID(gff_data->feature_2_GFFID,
										     gff_data->SO_2_index,
										     &(gff_data->dump_index),
										     gff_id_key_str, parent_id_str,
										     parent_key,
										     parent_so_term_key,
										     parent_so_name_key, verbose) ;
			    }
			}
		    }
		}


	      /* Use SO accession if requested otherwise use SO term name. */
	      if (use_SO_acc)
		gff_so_id = so_term_key ;
	      else
		gff_so_id = so_name_key ;
	    }
	}
      else
	{
	  /* This line represents a subpart of a feature so needs a Parent tag and may need
	   * an ID tag as well. */
	  KEY subpart_SO_term = KEY_NULL, so_subpart_name_key = KEY_NULL ;

	  /* If it's a subpart the ace2so object must contain subpart tags. */
	  if (!(keyFindTag(ace2so_key, str2tag(ACE2SO_SUBPART))))
	    {
	      if (report_all_errors)
		*err_msg_out = hprintf(0, "Subfeature of \"%s\", class \"%s\", type \"%s\" not dumped:"
				       " cannot find %s tag in %s object.",
				       name(feature), className(feature),
				       fMapSegTypeName[feature_segtype],
				       ACE2SO_SUBPART, nameWithClass(ace2so_key)) ;

	      result = FALSE ;
	    }


	  /* The subparts tags must include an entry for the given subpart_tag pointing
	   * to the relevant SO term object, if it doesn't we report this if in verbose
	   * mode otherwise we silently don't dump it as it may have been deliberately
	   * omitted by the curator. */
	  if (result && (!(subpart_SO_term = getSubpartSOterm(ace2so_key, subpart_tag))))
	    {
	      if (report_all_errors)
		{
		  *err_msg_out = hprintf(0, "Subfeature of \"%s\", class \"%s\", type \"%s\" not dumped:"
					 " cannot find %s subpart tag in %s object.",
					 name(feature), className(feature),
					 fMapSegTypeName[feature_segtype],
					 name(subpart_tag), nameWithClass(ace2so_key)) ;
		}

	      result = FALSE ;
	    }


	  /* The SO term object must contain a SO_name for the type of object. */
	  if (result && (!bIndexGetKey(subpart_SO_term, str2tag(SO_NAME), &so_subpart_name_key)))
	    {
	      *err_msg_out = hprintf(0, "Subfeature of \"%s\", class \"%s\", type \"%s\" not dumped:"
				     " cannot find %s for in %s.",
				     name(feature), className(feature),
				     fMapSegTypeName[feature_segtype],
				     SO_NAME, nameWithClass(subpart_SO_term)) ;

	      result = FALSE ;
	    }

	  if (result)
	    {
	      /* Use SO accession if requested otherwise use SO term name. */
	      if (use_SO_acc)
		gff_so_id = subpart_SO_term ;
	      else
		gff_so_id = so_subpart_name_key ;
	    }

	  /* subparts always need a Parent tag. */
	  if (result)
	    {
	      if (!no_span)
		{
		  /* We know in this case there is only one parent as it's a subpart. That may change
		   * the future (e.g. exons with multiple parents) but let's wait until then. */
		  ParentData parent_data ;

		  parent_data = arrayp(parents, arrayMax(parents), ParentDataStruct) ;
	      

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
		  parent_data->parent_key = parent_key ;
		  parent_data->parent_method_key = parent_method_key ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

		  parent_data->parent_id = getGFFID(gff_data->feature_2_GFFID,
						    gff_data->SO_2_index, &(gff_data->dump_index),
						    gff_id_key_str, parent_id_str,
						    feature, so_term_key, so_name_key, verbose) ;
		}
	      else
		{
		  gboolean has_so_parent, so_parent_valid ; 
		  KEY parent_key = KEY_NULL, parent_method_key = KEY_NULL ;
		  KEY parent_so_term_key = KEY_NULL, parent_so_name_key = KEY_NULL, parent_ace2so_key = KEY_NULL ;

		  /* See if feature has a SO parent defined that we can reference via a Parent= tag. */
		  if ((has_so_parent = getParentSOterm(feature, feature_segtype,
						       ace2so_key,
#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
						       &parent_key, &parent_method_key,
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */
						       parents,
						       err_msg_out)))
		    {
		      int i ;

		      for (i = 0 ; i < arrayMax(parents) ; i++)
			{
			  parent_key = arr(parents, i, ParentDataStruct).parent_key ;
			  parent_method_key = arr(parents, i, ParentDataStruct).parent_method_key ;

			  if (!(so_parent_valid = getObjSOName(feature, feature_segtype, parent_method_key,
							       &parent_so_term_key, &parent_so_name_key,
							       &parent_ace2so_key,
							       err_msg_out)))
			    {
			      if (has_so_parent && !so_parent_valid)
				*err_msg_out = hprintf(0, "\"%s\", class \"%s\", type \"%s\","
						       " has %s set to %s,"
						       " but this object does not have a valid SO Name"
						       " so \"Parent=\" attribute cannot be added !",
						       name(feature), className(feature), fMapSegTypeName[feature_segtype],
						       ACE2SO_PARENT, nameWithClass(parent_key)) ;
			    }
			  else
			    {
			      /* We need to make sure the parent was mapped otherwise it won't have been included in
			       * the set of features for the dump. */
			      if (!fMapFindMappedDict(gff_data->mapped_dict, parent_key))
				{
				  *err_msg_out = hprintf(0, "\"%s\", class \"%s\", type \"%s\", has %s set to %s,"
							 " but this object was not mapped so \"Parent=\" attribute cannot be added !",
							 name(feature), className(feature),
							 fMapSegTypeName[feature_segtype],
							 ACE2SO_PARENT, nameWithClass(parent_key)) ;
				}
			      else
				{
				  arr(parents, i, ParentDataStruct).parent_id = getGFFID(gff_data->feature_2_GFFID,
											 gff_data->SO_2_index,
											 &(gff_data->dump_index),
											 gff_id_key_str, parent_id_str,
											 parent_key,
											 parent_so_term_key,
											 parent_so_name_key, verbose) ;
				}
			    }
			}
		    }
		}
	    }

	  /* subparts may need their own ID tag, e.g. for cds. */
	  if (result &&  (seg_attrs & FMAPSEGDUMPATTRS_IS_MULTILINE))
	    {
	      feature_id = getGFFID(gff_data->feature_2_GFFID, gff_data->SO_2_index, &(gff_data->dump_index),
				    gff_id_key_str, feature_id_str,
				    feature, subpart_SO_term, so_subpart_name_key, verbose) ;
	    }
	}
    }


  /* If all is ok then output the record. */
  if (result)
    {
      char *gff_so_field ;


      /*
       * Output the mandatory fields in GFFv3
       */

      if (score)
	g_string_printf(float_str, "%g", *score) ;

      /* Horrible...forced on us by some acedb features not having a method.... */
      if (!gff_so_id)
	gff_so_field = "." ;
      else
	gff_so_field = name(gff_so_id) ;


      if (!debug)
	aceOutPrint(gff_out, "%s\t%s\t%s\t%d\t%d\t%s\t%c\t%c",
		    seqname, source,
		    gff_so_field,
		    start, end,
		    (score ? float_str->str : "."),
		    strand, frame) ;


      /*
       * Output the extra attributes.
       */

      attr_delim = '\t' ;


      if (feature_id)
	{
	  aceOutPrint(gff_out, "%cID=%s%s",
		      attr_delim,
		      (gff_data->id_prefix ? gff_data->id_prefix : ""),
		      feature_id) ;
	  attr_delim = ';' ;
	}

      if (arrayMax(parents) > 0)
	{
	  int i ;

	  aceOutPrint(gff_out, "%cParent=",
		      attr_delim) ;

	  for (i = 0 ; i < arrayMax(parents) ; i++)
	    {
	      char *parent_id ;

	      parent_id = arr(parents, i, ParentDataStruct).parent_id ;

	      aceOutPrint(gff_out, "%s%s",
			  (gff_data->id_prefix ? gff_data->id_prefix : ""),
			  parent_id) ;

	      if (i < (arrayMax(parents) - 1))
		aceOutPrint(gff_out, "%c",
			    ',') ;
	    }

	  attr_delim = ';' ;
	}

      if (feature)
	{
	  /* We don't always needs the name stuff....but good for debugging.... */
	  if (((seg_attrs & FMAPSEGDUMPATTRS_IS_FEATURE)
	       && (!(seg_attrs & FMAPSEGDUMPATTRS_OMIT_NAME)))
	      || verbose)
	    {
	      aceOutPrint(gff_out, "%cName=%s", attr_delim, nameWithClass(feature)) ;
	      attr_delim = ';' ;
	    }
	}
  
      switch(dump_data->type)
	{
	case FMAPSEGDUMP_TYPE_BASIC:
	  {

	    break ;
	  }
	case FMAPSEGDUMP_TYPE_SEQUENCE:
	  {
            
	    FMapSegDumpSequence sequence = &(dump_data->data.sequence) ;

	    if (sequence->known_name)
	      aceOutPrint(gff_out, "%cknown_name=%s",
                          attr_delim, name(sequence->known_name)) ;

	    break ;
	  }
	case FMAPSEGDUMP_TYPE_ALIGNMENT:
	  {
	    FMapSegDumpMatch match = &(dump_data->data.match) ;

	    g_string_append_printf(dump_attributes, "%cTarget=%s %d %d %c",
				   attr_delim,
				   name(match->target),
				   match->start, match->end, match->strand) ;
	    attr_delim = ';' ;
	

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
            /* remove zmap opt...... */
	    if (dump_data->zmap_dump)
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

	      {
		gboolean debug = FALSE, gaps_ok ;

		if (debug)
		  {
		    if (match->gaps->len > 0)
		      {
			GArray *gaps = match->gaps ;
			GString *str ;
			int i;

                        str = g_string_sized_new(2000) ;

                        g_string_append_printf(str, "%cDebug_Gaps=", attr_delim) ;
		    
			for (i = 0 ; i < match->gaps->len ; i++) 
			  {
			    FMapSegDumpAlign gap = &g_array_index(gaps, FMapSegDumpAlignStruct, i)  ;

			    g_string_append_printf(str, "%d %d %d %d,", gap->match_start, gap->match_end,
						   gap->ref_start, gap->ref_end) ;
			  }

			g_string_append_printf(dump_attributes, "\n%s\n", str->str);
	      
			g_string_free(str, TRUE);
		      }
		  }


		if (match->gaps)
		  gaps_ok = makeGapsString(dump_attributes, match->gaps,
					   start, end,
					   match->start, match->end, match->align_type) ;

		if (match->length)
		  {
		    g_string_append_printf(dump_attributes, "%clength=%d", attr_delim, match->length) ;
		  }

		if (match->own_sequence)
		  {
		    g_string_append_printf(dump_attributes, "%cown_sequence=TRUE", attr_delim) ;
		  }
	      }

	    break ;
	  }
	case FMAPSEGDUMP_TYPE_TRANSCRIPT:
	  {
	    FMapSegDumpTranscript transcript = &(dump_data->data.transcript) ;

	    if (transcript->start_not_found)
	      aceOutPrint(gff_out, "%cstart_not_found=%d",
                          attr_delim, transcript->start_offset) ;

	    if (transcript->end_not_found)
	      aceOutPrint(gff_out, "%cend_not_found=TRUE", attr_delim) ;

	    break ;
	  }
	default:
	  {
            if (dump_data->type != FMAPSEGDUMP_TYPE_INVALID)
              messerror("Bad value for feature type: %d", dump_data->type) ;
	  }
	}

      if (attributes && *attributes)
	{
	  char *allowed_chars = " \"" ;
	  char *escaped_str ;

	  escaped_str = g_uri_escape_string(attributes, allowed_chars, FALSE) ;

          /* first part is a hack...needs sorting out badly....see fmapsegdump.c */
          if ((strstr(attributes, "Name=")))
            g_string_append_printf(dump_attributes, "%c%s", attr_delim, attributes) ;
          else
            g_string_append_printf(dump_attributes, "%cNote=%s", attr_delim, escaped_str) ;

	  g_free(escaped_str) ;
	}


      aceOutPrint (gff_out, "%s", dump_attributes->str) ;

      aceOutPrint(gff_out, "\n") ;
    }


  /* Reset reused strings. */
  dump_attributes = g_string_set_size(dump_attributes, 0) ;

  parents = arrayReCreate(parents, 20, ParentDataStruct) ;



  return result ;
}





GHashTable *createSOTerms(void)
{
  GHashTable *so_table = NULL ;
  SOProperty SO_terms, curr ;
  GDestroyNotify keyFree = (GDestroyNotify)g_free ;

  SO_terms = createSOList() ;

  so_table = g_hash_table_new_full(g_str_hash, g_str_equal, keyFree, NULL) ;

  curr = SO_terms ;
  while (curr->SO_id)
    {
      g_hash_table_insert(so_table, g_strdup(curr->SO_id), curr) ;

      curr++ ;
    }

  return so_table ;
}


/* Currently we return a fixed list but we may need to make this a dynamically appendable list. */
SOProperty createSOList(void)
{
  static SOPropertyStruct so_list[] =
    {
      {"SO:0000001", "region", NULL},
      {"SO:0000148", "supercontig", NULL},
      {"SO:0000149", "contig", NULL},
      {"SO:0000147", "exon", NULL},
      {"SO:0000188", "intron", NULL},
      {"SO:0000673", "transcript", NULL},
      {"SO:0000316", "CDS", NULL},
      {"SO:0000185", "primary_transcript", NULL},
      {"SO:0000336", "pseudogene", NULL},
      {"SO:0000836", "mRNA_region", NULL},
      {"SO:0000120", "protein_coding_primary_transcript", NULL},
      {NULL, NULL, NULL},
    } ;


  return so_list ;
}



#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
/* Maps the SegType used by FMap to a SO id, unfortunately there are a number of cases where
 * the SegType represents more than one type of feature (e.g. SEQUENCE) and therefore the
 * mapping is ambiguous so NULL is returned. */
char *segType2SO(SegType seg_type)
{
  char *SO_id = NULL ;
  Seg2SOStruct seg_map[] =
    {
      {MASTER, MASTER, NULL},
      {ASSEMBLY_TAG, ASSEMBLY_TAG, NULL},
      {SEQUENCE, SEQUENCE_UP, NULL},
      {CDS, CDS_UP, "SO:0000316"},
      {INTRON, INTRON_UP, "SO:0000188"},
      {EXON, EXON_UP, "SO:0000147"},
      {EMBL_FEATURE, EMBL_FEATURE_UP,  NULL},
      {FEATURE, FEATURE_UP, NULL},
      {ATG, ATG_UP, NULL},
      {SPLICE3, SPLICE3_UP, NULL},
      {SPLICE5, SPLICE5_UP, NULL},
      {CODING, CODING_UP, NULL},
      {TRANSCRIPT, TRANSCRIPT_UP, "SO:0000673"},
      {SPLICED_cDNA, SPLICED_cDNA_UP, NULL},
      {VIRTUAL_SUB_SEQUENCE_TAG, VIRTUAL_SUB_SEQUENCE_TAG_UP, NULL},
      {VIRTUAL_MULTIPLET_TAG, VIRTUAL_MULTIPLET_TAG_UP, NULL},
      {VIRTUAL_ALIGNED_TAG, VIRTUAL_ALIGNED_TAG_UP, NULL},
      {VIRTUAL_PREVIOUS_CONTIG_TAG, VIRTUAL_PREVIOUS_CONTIG_TAG_UP, NULL},
      {HOMOL_GAP, HOMOL_GAP_UP, NULL},
      {HOMOL, HOMOL_UP, NULL},
      {PRIMER, PRIMER_UP,  NULL},
      {OLIGO, OLIGO_UP, NULL},
      {OLIGO_PAIR, OLIGO_PAIR_UP, NULL},
      {TRANS_SEQ, TRANS_SEQ_UP, NULL},
      {FEATURE_OBJ, FEATURE_OBJ_UP, NULL},
      {ALLELE, ALLELE_UP,  NULL},
      {VISIBLE, VISIBLE, NULL},
      {DNA_SEQ, DNA_SEQ, NULL},
      {PEP_SEQ, PEP_SEQ, NULL},
      {ORF, ORF, NULL},
      {VIRTUAL_PARENT_SEQUENCE_TAG, VIRTUAL_PARENT_SEQUENCE_TAG, NULL},
      {VIRTUAL_CONTIG_TAG, VIRTUAL_CONTIG_TAG, NULL},
      {ASSEMBLY_PATH, ASSEMBLY_PATH, NULL},
      {CLONE_END, CLONE_END, NULL},
      {CLONE_END + 1, CLONE_END + 1, NULL},
    } ;
  Seg2SO curr ;

  curr = seg_map ;
  while (curr->seg_id < CLONE_END + 1)
    {
      if ((seg_type == curr->seg_id || seg_type == curr->seg_id_up) && curr->ace_2_so_tag)
	{
	  SO_id = curr->ace_2_so_tag ;
	  break ;
	}

      curr++ ;
    }

  return SO_id ;
}
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */




/* This routine has to use adhoc rules to determine the SO term from the information
 * in acedb. There probably never will be a really clean way to do it unless acedb
 * databases start including SO terms themselves.
 * 
 *  */
char *acedbFeature2SO(char *source, char *feature_type, char *class)
{

  char *so_term = NULL ;
  static GHashTable *acedb2SO = NULL ;
  static GHashTable *source_class2SO = NULL ;
  static GString *key = NULL ;

  if (!acedb2SO)
    {
      acedb2SO = makeAcedb2SOHash() ;

      source_class2SO = makeSourceClass2SOHash() ;

      key = g_string_sized_new(500) ;			    /* 500 should be enough. */
    }


  /* some sources have an obvious translation, others do not, for those we keep a table. */
  if (g_ascii_strcasecmp(source, "exon") == 0 || g_ascii_strcasecmp(source, "coding_exon") == 0)
    so_term = "SO:0000147" ;
  else if (g_ascii_strcasecmp(source, "intron") == 0)
    so_term = "SO:0000188" ;
  else if (g_ascii_strcasecmp(feature_type, "coding_exon") == 0)
    so_term = "SO:0000316" ;
  else if (g_ascii_strcasecmp(feature_type, "CDS") == 0)
    so_term = "SO:0000316" ;
  else
    {
      makeKeyStr(key, feature_type, class) ;

      if (!(so_term = g_hash_table_lookup(source_class2SO, key->str)))
	{
	  makeKeyStr(key, feature_type, source) ;

	  so_term = g_hash_table_lookup(acedb2SO, key->str) ;
	}
    }



  return so_term ;
}


/* Makes a hash table to convert acedb source/feature names into SO terms.
 * Basis of hash is concatenation of "source==feature", the "==" is not allowed
 * in acedb so this ensures a unique string which hashes to the SO term.
 * In addition the string is lower cased. */
static GHashTable *makeAcedb2SOHash(void)
{
  GHashTable *acedb2SO = NULL ;
  FeatureToSOStruct featureSO[] =
    {
      {"sequence", "link", "SO:0000148"},
      {"region", "genomic_canonical", "SO:0000001"},
      {"sequence", "genomic_canonical", "SO:0000149"},

      {"sequence", "curated", "SO:0000185"},
      {"transcript", "coding_transcript", "SO:0000185"},
      {"coding_exon", "curated", "SO:0000316"},
      {"exon", "curated", "SO:0000316"},
      {"exon", "coding_transcript", "SO:0000316"},
      {"intron", "curated", "SO:0000188"},
      {"intron", "coding_transcript", "SO:0000188"},
      {"CDS", "curated", "SO:0000316"},

      {"pseudogene", "pseudogene", "SO:0000336"},
      {"sequence", "pseudogene", "SO:0000336"},

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
      {"exon", "psuedogene", "decayed_exon"},
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */



#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
      {"experimental", "cDNA_for_RNAi", "RNAi_reagent"},
      {"experimental", "RNAi", "RNAi_reagent"},


      {"repeat", "inverted", "inverted_repeat"},
      {"repeat", "tandem", "tandem_repeat"},

      {"similarity", "RepeatMasker", "nucleotide_motif"},
      {"similarity", "wublastx", "protein_match"},
      {"similarity", "wublastx_fly", "protein_match"},
      {"similarity", "waba_weak", "cross_genome_match"},
      {"similarity", "waba_strong", "cross_genome_match"},
      {"similarity", "waba_coding", "cross_genome_match"},

      {"transcription", "BLASTN_TC1", "nucleotide_match"},

      {"structural", "BLASTN_TC1", "nucleotide_match"},
      {"structural", "GenePair_STS", "PCR_product"},

      {"UTR", "UTR", "UTR"},

      {"trans-splice_acceptor", "UTR", "UTR"},
      {"trans-splice_acceptor", "SL1", "trans_splice_acceptor_site"},

      {"SNP", "Allele", "SNP"},

      {"reagent", "Orfeome_project", "PCR_product"},
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */


      {NULL, NULL, NULL}				    /* Last one must be NULL. */
    } ;

  acedb2SO = makeHash(featureSO) ;

  return acedb2SO ;
}


/* Makes a hash table to convert acedb source/class names into SO terms.
 * Basis of hash is concatenation of "source==feature", the "==" is not allowed
 * in acedb so this ensures a unique string which hashes to the SO term.
 * In addition the string is lower cased. */
static GHashTable *makeSourceClass2SOHash(void)
{
  GHashTable *source_class2SO = NULL ;
  FeatureToSOStruct sourceclassSO[] =
    {
      {"similarity", "protein", "protein_match"},
      {"genomic_canonical", "sequence", "region"},

      {NULL, NULL, NULL}				    /* Last one must be NULL. */
    } ;

  source_class2SO = makeHash(sourceclassSO) ;

  return source_class2SO ;
}


static GHashTable *makeHash(FeatureToSO table)
{
  GHashTable *hash = NULL ;
  FeatureToSO curr ;
  static GString *buffer = NULL ; 

  if (!buffer)
    buffer = g_string_sized_new(500) ;			    /* Should never require reallocation. */

  hash = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, g_free) ;

  curr = table ;

  while (curr->key_1)
    {
      char *key, *value ;

      makeKeyStr(buffer, curr->key_1, curr->key_2) ;
      key = g_strdup(buffer->str) ;

      value = g_strdup(curr->SO_id) ;

      g_hash_table_insert(hash, key, value) ;

      curr++ ;
    }

  return hash ;
}





void makeKeyStr(GString *key, char *feature_type, char *source)
{
  char *key_pattern = "%s==%s" ;			    /* hash of  feature_type"=="source */

  g_string_printf(key, key_pattern, feature_type, source) ;
  key = g_string_ascii_down(key) ;

  return ;
}




/* Appends gap string to gaps_str, caller is responsible for free'ing of data
 * etc. etc. */
static gboolean makeGapsString(GString *gaps_str, GArray *gaps,
			       int ref_start, int ref_end,
			       int match_start, int match_end,
			       FMapSegDumpAlignType align_type)
{
  gboolean result = FALSE ;

  if (gaps->len > 0)
    {
      FMapSegDumpAlign start, end ;

      /* Check the sanity of the coords a bit.... */
      start = &g_array_index(gaps, FMapSegDumpAlignStruct, 0)  ;
      end = &g_array_index(gaps, FMapSegDumpAlignStruct, gaps->len - 1)  ;

      if (start->match_start == match_start && start->ref_start == ref_start
	  && end->match_end == match_end && end->ref_end == ref_end)
	{
	  int i ;
	  int curr_ref, curr_match ;
	  int align_factor ;

	  switch(align_type)
	    {
	    case FMAPSEGDUMP_ALIGN_DNADNA:
	      align_factor = 1 ;
	      break ;
	    case FMAPSEGDUMP_ALIGN_DNAPEP:
	    case FMAPSEGDUMP_ALIGN_PEPDNA:
	      align_factor = 3 ;
	      break ;
	    default:
	      messcrash("bad logic") ;
	      break ;
	    }

	  g_string_append_printf(gaps_str, "%s", ";Gap=") ;

	  curr_ref = ref_start - 1 ;
	  curr_match = match_start - 1 ;
	  for (i = 0 ; i < gaps->len ; i++) 
	    {
	      FMapSegDumpAlign gap = &g_array_index(gaps, FMapSegDumpAlignStruct, i)  ;
	      int gap_ref, gap_match, frame_ref ;

	      /* N.B. it's "- 1" because we want the length _in between_ the matches. */
	      gap_ref = gap->ref_start - curr_ref - 1 ;
	      gap_match = gap->match_start - curr_match - 1 ;
	      frame_ref = gap_ref % 3 ;

	      /* This doesn't handle frame shifts which we need to do....
	       * finish this to do frame refs... */
	      if (frame_ref)
		g_string_append_printf(gaps_str, "%s ", "***FRAME_REF****") ;


	      if (gap_ref > 0 && gap_match > 0)
		g_string_append_printf(gaps_str, "|%d ", gap_match) ;
	      else if (gap_ref > 1)
		g_string_append_printf(gaps_str, "I%d ", gap_ref / align_factor) ;
	      else if (gap_match > 1)
		g_string_append_printf(gaps_str, "D%d ", gap_match) ;

	      g_string_append_printf(gaps_str, "M%d ", gap->match_end - gap->match_start + 1) ;

	      curr_ref = gap->ref_end ;
	      curr_match = gap->match_end ;
	    }

	  result = TRUE ;
	}
    }

  return result ;
}



static char *getGFFID(GHashTable *feature_2_GFFID, GHashTable *SO_2_index, int *dump_index_inout,
		      GString *key_str_inout, GString *id_str_inout,
		      KEY feature, KEY so_term_key, KEY so_name_key,
		      BOOL verbose)
{
  char *gff_id = NULL ;
  char *key_str ;
  BOOL use_ace_file_id = TRUE ;


  key_str = makeFeature2FileIDKeyStr(key_str_inout, feature, name(so_name_key)) ;

  if (!(gff_id = g_hash_table_lookup(feature_2_GFFID, key_str)))
    {
      /* OK, if we don't have a gff_id we must make one up. */
      if (use_ace_file_id == TRUE)
	gff_id = makeAceFileID(id_str_inout, feature) ;
      else
	gff_id = makeFileID(id_str_inout, SO_2_index, dump_index_inout,
			    name(so_term_key), name(so_name_key),
			    verbose) ;

      g_hash_table_insert(feature_2_GFFID, g_strdup(key_str), g_strdup(gff_id)) ;
    }

  return gff_id ;
}



/* Creates a string used as the key for a particular feature for looking up that features
 * GFF file id. String is created in GString str_inout and as a convenience function
 * returns a pointer to key string held in str_inout. */
char *makeFeature2FileIDKeyStr(GString *str_inout, KEY feature, char *SO_term)
{
  char *key_str = NULL ;

  g_string_printf(str_inout, "%s|%s|%s", name(feature), className(feature), SO_term) ;
  key_str = str_inout->str ;

  return key_str ;
}


/* Creates a string used as the GFF file ID for a particular feature for parent/child & multiline
 * features. This is the id looked up in a hash using the key created by makeFeature2FileIDKeyStr().
 * String is created in GString str_inout and as a convenience function returns a pointer to key string
 * held in str_inout.
 * 
 *  */
static char *makeAceFileID(GString *str_inout, KEY feature)
{
  char *id_str = NULL ;

  g_string_printf(str_inout, "%s", nameWithClass(feature)) ;

  id_str = str_inout->str ;

  return id_str ;
}



/* Creates a string used as the GFF file ID for a particular feature for parent/child & multiline
 * features. This is the id looked up in a hash using the key created by makeFeature2FileIDKeyStr().
 * String is created in GString str_inout and as a convenience function returns a pointer to key string
 * held in str_inout.
 * 
 * Note: if label is supplied then that is used instead of the SO_term in the ID.
 *  */
static char *makeFileID(GString *str_inout, GHashTable *SO_2_index, int *dump_index_inout,
			char *SO_term, char *label, BOOL verbose)
{
  char *id_str = NULL ;
  int feature_index ;

  /* For each feature which needs an ID: 
   *  readable form - we use it's SO term and a unique index to feature to keep
   *     an index for each SO_term which is incremented as we parse each new feature
   *     whose type is that SO_term.
   *
   *  minimal form  - we use just a number for each new feature.
   * 
   *  */
  if (verbose)
    {
      if (!(feature_index = GPOINTER_TO_INT(g_hash_table_lookup(SO_2_index, SO_term))))
	feature_index = 1 ;
      else
	feature_index++ ;
    }
  else
    {
      *dump_index_inout += 1 ;				    /* assumes dump_index initialised to zero. */

      feature_index = *dump_index_inout ;
    }

  g_hash_table_insert(SO_2_index, g_strdup(SO_term), GINT_TO_POINTER(feature_index)) ;

  /* Now construct the GFF v3 id, either in readable form: SO_term and unique index
   * or minimal form: the index. */
  if (verbose)
    g_string_printf(str_inout, "%s.%d", label, feature_index) ;
  else
    g_string_printf(str_inout, "%d", feature_index) ;

  id_str = str_inout->str ;

  return id_str ;
}



static char *getGFFUserID(GHashTable *feature_2_GFFID,
			  GString *key_str_inout, GString *id_str_inout,
			  KEY feature, char *user_str)
{
  char *gff_id = NULL ;
  char *key_str ;

  key_str = makeFeature2FileUserIDKeyStr(key_str_inout, feature, user_str) ;

  if (!(gff_id = g_hash_table_lookup(feature_2_GFFID, key_str)))
    {
      /* OK, if we don't have a gff_id we must make one up. */
      gff_id = makeUserFileID(id_str_inout, feature, user_str) ;

      g_hash_table_insert(feature_2_GFFID, g_strdup(key_str), g_strdup(gff_id)) ;
    }

  return gff_id ;
}


static char *makeFeature2FileUserIDKeyStr(GString *str_inout, KEY feature, char *user_str)
{
  char *key_str = NULL ;

  g_string_printf(str_inout, "%s.%s", name(feature), user_str) ;
  key_str = str_inout->str ;

  return key_str ;
}

static char *makeUserFileID(GString *str_inout, KEY feature, char *user_str)
{
  char *id_str = NULL ;

  g_string_printf(str_inout, "%s.%s",
		  name(feature),
		  user_str) ;
  id_str = str_inout->str ;

  return id_str ;
}


#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
/* Find key for Ace2SO object for given feature. */
static KEY findAce2SO(KEY feature)
{
  KEY ace2so_key = KEY_NULL ;
  KEY method_key ;

  if (bIndexGetKey(feature, str2tag("Method"), &method_key))
    {
      bIndexGetKey(method_key, str2tag("Ace2SO"), &ace2so_key) ;
    }

  return ace2so_key ;
}
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */




#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
/* Maps a SegType to the tag used in the Ace2SO class to find the SO term for
 * that SegType, e.g. if the SegType is EXON then the tag in the class is "Exon"
 * (which in the end gets mapped to the SO_term object SO:0000147 if the user
 * has set things up correctly). */
static KEY segType2SOTag(SegType seg_type)
{
  KEY ace_2_so_tag = KEY_NULL ;
  enum {INVALID_SEG = MASTER - 1} ;
  Seg2SOStruct seg_map[] =
    {
      {INTRON, INTRON_UP, "Intron"},
      {EXON, EXON_UP, "Exon"},

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
      /* I'm leaving these out as wormbase now has cds objects... */

      {CODING, CODING_UP, "Exon"},
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

      {INVALID_SEG, INVALID_SEG, NULL}
    } ;
  Seg2SO curr ;

  curr = seg_map ;
  while (curr->seg_id != INVALID_SEG)
    {
      if ((seg_type == curr->seg_id || seg_type == curr->seg_id_up) && curr->ace_2_so_tag)
	{
	  ace_2_so_tag = str2tag(curr->ace_2_so_tag) ;

	  break ;
	}

      curr++ ;
    }

  return ace_2_so_tag ;
}
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */




/* Function to check that we have the required tags for processing.
 * 
 * The method tags/fields are:
 * 
 * ?Method
 *       GFF_SO UNIQUE ?SO_term UNIQUE ?Ace2SO
 * 
 * SO_term is mandatory, the Ace2SO optional.
 * 
 * 
 * The ace2so object tags/fields are:
 * 
 * ?Ace2SO Description UNIQUE Text
 *       SO_subpart_tags Text ?SO_Term
 *       SO_parent_obj_tag UNIQUE Text
 *       SO_children
 * 
 * Code checks for the SO_term and for an Ace2SO object.
 * 
 * Returns FALSE if there is no SO_term, TRUE otherwise.
 * 
 * 
 *  */

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
static gboolean getObjSOName(KEY feature, SegType feature_segtype, KEY method_obj_key,
			     KEY *so_term_key_out,
			     KEY *so_name_key_out,
			     KEY *ace2so_key_out,
			     char **err_msg_out)
{
  gboolean result = TRUE ;
  KEY so_term_key = KEY_NULL, so_name_key = KEY_NULL, ace2so_key = KEY_NULL ;


  /* The feature's method object must reference a SO_term object. */
  if (result && !bIndexGetKey(method_obj_key, str2tag(METHOD_SO), &so_term_key))
    {
      *err_msg_out = hprintf(0, "\"%s\", class \"%s\", type \"%s\", method \"%s\" not dumped:"
			     " no %s obj in method.",
			     name(feature), className(feature),
			     fMapSegTypeName[feature_segtype], name(method_obj_key),
			     SO_CLASS) ;
      result = FALSE ;
    }


  /* The SO_term object must contain a SO_name for the type of object otherwise we cannot name it
   * in the dump. */
  if (result && !bIndexGetKey(so_term_key, str2tag(SO_NAME), &so_name_key))
    {
      *err_msg_out = hprintf(0, "\"%s\", class \"%s\", type \"%s\", method \"%s\" not dumped:"
			     " cannot find feature's %s in %s object.",
			     name(feature), className(feature),
			     fMapSegTypeName[feature_segtype], name(method_obj_key),
			     SO_NAME, SO_CLASS) ;

      result = FALSE ;
    }


  /* The feature's method object can optionally reference an Ace2SO object. */
  if (result && !(bIndexGetKey(method_obj_key, _bsRight, &ace2so_key)))
    {

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
      /* It's not an error not to have this.... */

      *err_msg_out = hprintf(0, "\"%s\", class \"%s\", type \"%s\", method \"%s\" not dumped:"
			     " no %s object in method.",
			     name(feature), className(feature),
			     fMapSegTypeName[feature_segtype],
			     name(method_obj_key),
			     ACE2SO_CLASS) ;
      result = FALSE ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

    }


  /* all ok so return the results. */
  if (result)
    {
      *so_term_key_out = so_term_key ;
      *so_name_key_out = so_name_key ;
      *ace2so_key_out = ace2so_key ;
    }

  return result ;
}
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */



/* Look in a features method obj for SO_term and Ace2SO, relevant parts of
 * the model are:
 *
 * ?Method
 * 		GFF_SO SO_term UNIQUE ?SO_term UNIQUE ?Ace2SO
 *  
 * ?SO_term SO_name ?Text
 * 
 * 
 * Returns:
 * 
 * so_term_key_out = key of the ?SO_term object
 * so_name_key_out = key of the ?Text object following SO_name in the ?SO_term object.
 *  ace2so_key_out = key of the ?Ace2SO object
 * 
 * 
 */
static gboolean getObjSOName(KEY feature, SegType feature_segtype, KEY method_obj_key,
			     KEY *so_term_key_out,
			     KEY *so_name_key_out,
			     KEY *ace2so_key_out,
			     char **err_msg_out)
{
  gboolean result = TRUE ;
  KEY so_term_key = KEY_NULL, so_name_key = KEY_NULL, ace2so_key = KEY_NULL ;
  OBJ method_obj = NULL ;


  /* Can we open the method object ? */
  if (!(method_obj = bsCreate(method_obj_key)))
    {
      *err_msg_out = makeErrMsg(feature, feature_segtype, method_obj_key,
				"cannot open method obj for feature: \"%s\", class \"%s\".",
				name(method_obj_key), className(method_obj_key)) ;

      result = FALSE ;
    }

  /* The feature's method object _MUST_ reference a SO_term object. */
  if (result && !bsGetKey(method_obj, str2tag(METHOD_GFF_SO), &so_term_key))
    {
      *err_msg_out = makeErrMsg(feature, feature_segtype, method_obj_key,
				"not dumped: no %s obj in method.",
				SO_CLASS) ;
      result = FALSE ;
    }


  /* The SO_term object _MUST_ contain a SO_name for the type of object otherwise we cannot name it
   * in the dump. */
  if (result && !bIndexGetKey(so_term_key, str2tag(SO_NAME), &so_name_key))
    {
      *err_msg_out = makeErrMsg(feature, feature_segtype, method_obj_key,
				"not dumped, cannot find feature's %s in %s object.",
				SO_NAME, SO_CLASS) ;

      result = FALSE ;
    }


  /* The feature's method object can optionally reference an Ace2SO object. */
  if (result && !(bsGetKey(method_obj, _bsRight, &ace2so_key)))
    {

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
      /* It's not an error not to have this.... */

      *err_msg_out = hprintf(0, "\"%s\", class \"%s\", type \"%s\", method \"%s\" not dumped:"
			     " no %s object in method.",
			     name(feature), className(feature),
			     fMapSegTypeName[feature_segtype],
			     name(method_obj_key),
			     ACE2SO_CLASS) ;
      result = FALSE ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

    }


  if (method_obj)
    bsDestroy(method_obj) ;

  /* all ok so return the results. */
  if (result)
    {
      *so_term_key_out = so_term_key ;
      *so_name_key_out = so_name_key ;
      *ace2so_key_out = ace2so_key ;
    }

  return result ;
}







/* Retrieve the key for the SO term object where Text in the model
 * equals the given subpart_tag. The relevant part of ?Ace2SO model is:
 * 
 *         SO_subpart_tags Text ?SO_term
 * 
 * Returns KEY_NULL on failure. */
static KEY getSubpartSOterm(KEY ace2so_key, KEY subpart_tag)
{
  KEY subpart_so_term_key = KEY_NULL ;
  OBJ ace2so_obj ;
  Array subparts ;

  subparts = arrayCreate(20, BSunit) ;

  if ((ace2so_obj = bsCreate(ace2so_key))
      && bsFindTag(ace2so_obj, str2tag(ACE2SO_SUBPART))
      && bsFlatten(ace2so_obj, 2, subparts)) 
    {
      int i ;

      for (i = 0 ; i < arrayMax(subparts) ; i += 2)
	{
	  if (arr(subparts, i, BSunit).k == subpart_tag)
	    {
	      /* should check this is valid.....??? */
	      subpart_so_term_key = arr(subparts, (i + 1), BSunit).k ;
	      break ;
	    }
	}
    }

  arrayDestroy(subparts) ;

  return subpart_so_term_key ;
}


/* Look in features ace2so obj for SO_parent_obj_tag key:
 *
 * 
 * ?Ace2SO Description UNIQUE Text
 * 	No_span
 *         SO_subparts Exons UNIQUE ?SO_term
 *                     Introns UNIQUE ?SO_Term
 *                     Coding_regions UNIQUE ?SO_term
 *         SO_parent_obj_tag UNIQUE Text  <<<<<<<<< here !
 *         SO_children
 * 
 * 
 * then convert text following SO_parent_obj_tag into a Tag and look for that tag
 * in the feature object. Following the tag will be the parent's object key.
 * Then open the parent object and look for the method key.
 *  
 */
static gboolean getParentSOterm(KEY feature_key, SegType feature_segtype,
				KEY feature_ace2so_key,

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
				KEY *parent_key_out, KEY *parent_method_key_out,
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */
				Array parents,
				char **err_msg_out)
{
  gboolean result = FALSE ;
  KEY parent_tag = str2tag(ACE2SO_PARENT) ;
  OBJ ace2so_obj ;
  char *so_parent_text = NULL ;
#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
  KEY parent_key = KEY_NULL, parent_method_key = KEY_NULL ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */
  static Array units = NULL ;

  if (keyFindTag(feature_ace2so_key, parent_tag)
      && (ace2so_obj = bsCreate(feature_ace2so_key))
      && bsGetData(ace2so_obj, parent_tag, _Text, (void *)&(so_parent_text))
      && bIndexTag(feature_key, str2tag(so_parent_text)))
    {
      OBJ obj ;

      units = arrayReCreate(units, 10, BSunit) ;

      if ((obj = bsCreate(feature_key))
	  && bsFindTag(obj, str2tag(so_parent_text))
	  && bsFlatten(obj, 1, units))
	{
	  int i ;

	  for (i = 0 ; i < arrayMax(units) ; i++)
	    {
	      KEY parent_key = KEY_NULL, parent_method_key = KEY_NULL ;

	      parent_key = arr(units, i, BSunit).k ;

	      if (bIndexGetKey(parent_key, str2tag("Method"), &parent_method_key))
		{
		  ParentData parent_data ;

		  parent_data = arrayp(parents, arrayMax(parents), ParentDataStruct) ;

		  parent_data->parent_key = parent_key ;
		  parent_data->parent_method_key = parent_method_key ;

		  result = TRUE ;
		}
	    }
#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
	  *parent_key_out = parent_key ;
	  *parent_method_key_out = parent_method_key ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

	  bsDestroy(obj) ;
	}
    }

  return result ;
}

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
static gboolean getParentSOterm(KEY feature_key, SegType feature_segtype,
				KEY feature_ace2so_key,
				KEY *parent_key_out, KEY *parent_method_key_out,
				char **err_msg_out)
{
  gboolean result = FALSE ;
  KEY parent_tag = str2tag(ACE2SO_PARENT) ;
  OBJ ace2so_obj ;
  char *so_parent_text = NULL ;
  KEY parent_key = KEY_NULL, parent_method_key = KEY_NULL ;


  if (keyFindTag(feature_ace2so_key, parent_tag)
      && (ace2so_obj = bsCreate(feature_ace2so_key))
      && bsGetData(ace2so_obj, parent_tag, _Text, (void *)&(so_parent_text))
      && bIndexGetKey(feature_key, str2tag(so_parent_text), &parent_key)
      /* Need a bsFlatten() here....... */
      && bIndexGetKey(parent_key, str2tag("Method"), &parent_method_key))

    {
      *parent_key_out = parent_key ;
      *parent_method_key_out = parent_method_key ;

      result = TRUE ;
    }

  return result ;
}
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */




/* Adds object details to supplied GString. */


#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
void zMapUtilsDebugPrintf(FILE *stream, char *format, ...)
{
  va_list args1, args2 ;

  va_start(args1, format) ;
  G_VA_COPY(args2, args1) ;

  vfprintf(stream, format, args2) ;
  fflush(stream) ;

  va_end(args1) ;
  va_end(args2) ;

  return ;
}
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */


/* There's a lot of faffing about with copies of strings etc but it means that the calling code
 * does not have to be converted from acedb's hprintf() to glibs GString stuff. */
static char *makeErrMsg(KEY feature, SegType feature_segtype, KEY method_obj_key,
			char *format, ...)
{
  char *err_msg = NULL ;
  GString *msg ;
  va_list args1, args2 ;

  msg = g_string_sized_new(1000) ;

  va_start(args1, format) ;
  G_VA_COPY(args2, args1) ;

  g_string_append_printf(msg,
			 "\"%s\", class \"%s\", type \"%s\", method \"%s\" - ",
			 name(feature), className(feature),
			 fMapSegTypeName[feature_segtype], name(method_obj_key)) ;

  g_string_append_vprintf(msg, format, args2) ;

  va_end(args1) ;
  va_end(args2) ;

  err_msg = strnew(msg->str, 0) ;

  g_string_free(msg, TRUE) ;

  return err_msg ;
}



