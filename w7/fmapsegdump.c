/*  File: gff.c
 *  Author: Ed Griffiths (edgrif@sanger.ac.uk)
 *  Copyright (c) J Thierry-Mieg and R Durbin, 2002
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
 * Description: Contains routines needed to dump features from
 *              SEG's array.
 *              
 *              Main dumping routine extracts information from segs
 *              but then calls the appropriate callback function
 *              which does the output in one of the following
 *              supported formats:
 *              
 *              GFF version 2
 *              GFF version 3
 *              DAS version 1, using the "DASGFF" format (prototype)
 *              
 * Exported functions: See w7/fmapsegdump.h
 *              
 * HISTORY:
 * Last edited: Nov 23 15:58 2011 (edgrif)
 * Created: Fri Apr 19 10:04:23 2002 (edgrif)
 * CVS info:   $Id: fmapsegdump.c,v 1.15 2012-10-30 09:57:40 edgrif Exp $
 *-------------------------------------------------------------------
 */
#include <glib.h>
#include <wh/regular.h>
#include <wh/acedb.h>
#include <wh/aceio.h>
#include <wh/dna.h>
#include <wh/lex.h>
#include <wh/bindex.h>
#include <wh/pick.h>
#include <whooks/systags.h>
#include <wh/peptide.h>
#include <wh/smapconvert.h>
#include <w7/fmapsegdump.h>



/* Manage the dump line data. Currently internal functions... */
static FMapSegDumpLine FMapSegDumpLineCreate(void) ;
static void FMapSegDumpLineReset(FMapSegDumpLine dump_line) ;
static void FMapSegDumpLineDestroy(FMapSegDumpLine dump_line) ;


static char *cleanNewlines (char *s, STORE_HANDLE handle) ;
static char strandOfKey(FeatureMap look, KEY key, int rootReversed) ;
static BOOL getClone(SMap *smap, KEY curr, int start, int end, KEY *source_out, KEY *clone_out) ;
static BOOL isCloneObj(KEY key) ;

static GArray *getGapsArray(GArray *gaps_in, Array gaps, int offset) ;
static char *getGapsStr(Array gaps, SMapAlignStrFormat str_format) ;




/*
 * --------------------  EXTERNAL FUNCTIONS  --------------------
 */


/* Establishes the reference sequence, offset, start/stop of the target
 * key, start/stop of features to be output (maybe outside start/stop
 * of target key. This is a separate routine from the dump routine
 * because some of this information is needed by the caller before
 * dumping starts. */
BOOL FMapSegDumpRefSeqPos(FeatureMap look, int version,
			  KEY *refSeq, KEY *seqKey_out, BOOL *reversed_out,
			  int *offset_out,
			  int *key_start_out, int *key_end_out,
			  int *feature_start_out, int *feature_end_out)
{
  BOOL result = TRUE ;
  int i ;
  int key_start, key_end, offset = 0, feature_start, feature_end ;
  int x, y ;
  SEG *seg ;
  KEY seqKey ;


  messAssert(look && version > 1 && seqKey_out && reversed_out
	     && offset_out && key_start_out && key_end_out
	     && feature_start_out && feature_end_out) ;

  /* first establish seqKey and offset */
  seqKey = 0 ;
  if (refSeq && !*refSeq)
    *refSeq = look->seqOrig ;

  if (refSeq && *refSeq)
    { 
      for (i = 1 ; i < arrayMax(look->segs) ; ++i)
        {
	  seg = arrp(look->segs, i, SEG) ;

          if ((seg->type == SEQUENCE || seg->type == SEQUENCE_UP
	       || seg->type == FEATURE_OBJ || seg->type == FEATURE_OBJ_UP)
	      && seg->key == *refSeq)
	    break ;
	}

      if (i < arrayMax(look->segs)) 
        { 
	  if (seg->type == SEQUENCE || seg->type == SEQUENCE_UP
	      || seg->type == FEATURE_OBJ || seg->type == FEATURE_OBJ_UP)
	    {
	      seqKey = *refSeq ;

	      if (look->seqOrig == look->seqKey)
		{
		  if (look->noclip)
		    {
		      /* Note that we do not use zoneMin/Max here, they don't make sense if you
		       * have asked to see everything, why clip it with zoneMin/Max ?  */
		      offset = 0 ;
		      key_start = (look->start + (look->zoneMin + 1)) ;
		      key_end = (look->start + look->zoneMax) ;
		      feature_start = 0 ;
		      feature_end = (look->fullLength - 1) ;
		    }
		  else
		    {
		      offset = look->start ;
		      key_start = (offset + (look->zoneMin + 1)) ;
		      key_end = (offset + look->zoneMax) ;
		      feature_start =  look->zoneMin ;
		      feature_end = (look->zoneMax - 1) ;
		    }
		}
	      else
		{
		  /* Find position of seqOrig in seqKey and use that and start to calculate
		   * offset. */
		  int y1 = 0, y2 = 0 ;
		  KEY key = KEY_UNDEFINED ;
		  int begin, end ;

		  if (seg->type == SEQUENCE || seg->type == FEATURE_OBJ)
		    begin = 1, end = 0 ;
		  else
		    begin = 0, end = 1 ;


		  /* not sure if the above screws everything up or not... */
		  begin = 1, end = 0 ;

		  /* WHAT...WHAT IS THIS.... */
		  if (!sMapTreeRoot (look->seqOrig, begin, end, &key, &y1, &y2)
		      && key != look->seqKey)
		    messcrash("whoops") ;

		  if (look->noclip)
		    {
		      /* Note that we do not use zoneMin/Max here, they don't make sense if you
		       * have asked to see everything, why clip it with zoneMin/Max ?  */
		      offset = -(y1 - 1) ;

		      key_start = y1 + offset ;
		      key_end = y2 + offset ;

		      feature_start = 0 ;
		      feature_end = (look->fullLength - 1) ;
		    }
		  else
		    {
		      offset = (look->start + 1) - y1 ;

		      key_start = (offset + (look->zoneMin + 1)) ;
		      key_end = (offset + look->zoneMax) ;

		      feature_start =  look->zoneMin ;
		      feature_end = (look->zoneMax - 1) ;
		    }
		}

	      *reversed_out = (seg->type == SEQUENCE_UP ? TRUE : FALSE) ;
	    }
	}
      else
 	{
	  messout ("Can't find reference sequence %s", name (*refSeq)) ;
 	  return FALSE ;
 	}
    }

  if (!seqKey)			/* find minimal spanning sequence */
    {
      x = look->zoneMin ;
      y = look->zoneMax ;
      seqKey = 0 ; 
      if (!fMapFindSpanSequence(look, &seqKey, &x, &y))
	seqKey = look->seqKey ;
      if (x > y)			/* what if reversed? */
	{
	  messout("Can't GFF dump from reversed sequences for now.  Sorry.") ;
	  return FALSE ;
	}
      offset = x - look->zoneMin ;
    }

  if (refSeq)
    *refSeq = seqKey ;

  *seqKey_out = seqKey ;
  *key_start_out = key_start ;
  *key_end_out = key_end ;
  *offset_out = offset ;
  *feature_start_out = feature_start ;
  *feature_end_out = feature_end ;

  return result ;
}


/* This routine contains the loop that goes through the segs in an fmap look
 * and extracts information from them appropriately for seg dumps, it
 * then calls the supplied FMapSegDumpCallBack routine which does the actual output.
 * 
 * So far there are callback routines for GFF version 2 and DASGFF output. 
 *
 */
BOOL FMapSegDump(FMapSegDumpCallBack app_cb, void *app_data,
		 FeatureMap look, int version,
		 KEY seqKey, int offset, int feature_start, int feature_end,
		 FMapSegDumpListType list_type, DICT listSet, BOOL raw_methods, Array stats,
		 DICT *sourceSet, DICT *featSet, DICT *methodSet,
		 BOOL include_source, BOOL include_feature, BOOL include_method,
		 BOOL zmap_dump, BOOL extended_features,
		 BOOL verbose, BOOL report_all_errors)
{

  /* A LOT OF THESE VARS SHOULD BE IN THE LOOP.... */

  BOOL result = TRUE ;
  int i ;
  SEG *seg ;
  HOMOLINFO *hinf ;
  SEQINFO *sinf ;
  METHOD *meth ;
  KEY sourceKey = KEY_UNDEFINED ;
  char *sequence_name = NULL, *sequence_class = NULL ;
  char *sourceName = NULL, *featName = NULL ;
  char *methodName = NULL, *tagText = NULL ;
  int x, y ;
  SegType parent_type = 0, feature_type = 0 ; 
  int tmp = 0, reversed = 0 ;                /* used by AcePerl */
  float score ;
  char strand, phase ;
  char *comment = NULL ;
  BOOL flipped ;
  BOOL isScore ;
  STORE_HANDLE handle = handleCreate() ;
  Associator key2sinf = assHandleCreate (handle) ;	    /* bizarrely unused.... */
  Associator key2source = assHandleCreate (handle) ;
  Associator key2feat = assHandleCreate (handle) ;
  GString *attributes ;
  enum {FMAPSEGDUMP_ATTR_INITLEN = 1024} ;		    /* Vague guess at initial length. */
  BOOL is_structural ;
  FMapSegDumpLine dump_data ;

  BOOL use_name = FALSE ;


  messAssert(app_cb != NULL && look != NULL && seqKey && list_type) ;


  if (zmap_dump)
    {
      /* version 2 option.... */

      use_name = TRUE ;
    }



  sequence_name = name(seqKey) ;
  sequence_class = className(seqKey) ;


  /* I am unsure how to fill this in at the moment....probably it is meant to be
   * anything that is in the smap hierachy, if so this would require some extra
   * work to ascertain such as a bIndex search of the object for the smap tags.... */
  is_structural = FALSE ;


  /* Allocate reusable resources..important for performance not to reallocate each
   * time round the main loop. */

  attributes = g_string_sized_new(FMAPSEGDUMP_ATTR_INITLEN) ; /* Allocate reusable/appendable string
								 for attributes.*/

  dump_data = FMapSegDumpLineCreate() ;			    /* Reusable dump data struct. */


  dump_data->verbose = verbose ;
  dump_data->report_all_errors = report_all_errors ;

  for (i = 0 ; i < arrayMax(look->segs) ; ++i)
    {
      FeatureInfo *feat = NULL ;
      KEY parent_key, feature_key, source_key ;
      char *parent_name, *parent_class, *parent_realtype ;
      char *feature_name, *feature_class ;
      FMapSegDumpBasicType seg_basic_type ;
      FMapSegDumpAttr seg_attrs = FMAPSEGDUMPATTRS_NONE ;
      KEY subpart_tag ;
      KEY method_obj_key = KEY_NULL ;

      /* CHECK IF WE NEED THESE...MISSING FROM SO VERSION OF CODE... */
      BOOL is_parent = FALSE, is_child = FALSE, is_multiline = FALSE ;

      seg = arrp(look->segs, i, SEG) ;

      /* Exclude anything outside the range we are interested in. */
      if (!zmap_dump && (seg->x1+offset > feature_end || seg->x2+offset < feature_start))
	continue ;


      /* Exclude segs that are created to support FMap like stuff, but mean nothing in the
       * context of a seg dump. You should NOTE that seg inclusion/exclusion means
       * that some of the case(s) below will/will not be executed at all for some dumps. */
      if (seg->type == MASTER
	  || seg->type == VISIBLE
	  || (!zmap_dump && (seg->type == CDS || seg->type == CDS_UP))
	  || (zmap_dump && (seg->type == CODING || seg->type == CODING_UP))

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
	  || (version > 2 && (seg->type == CODING || seg->type == CODING_UP))
	  || (version > 2 && seg->type == CLONE_END)
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */



	  || seg->type == HOMOL_GAP || seg->type == HOMOL_GAP_UP
	  || seg->type == DNA_SEQ || seg->type == PEP_SEQ
	  || seg->type == ORF
	  || seg->type == TRANS_SEQ || seg->type == TRANS_SEQ_UP)
	continue ;


#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
      /* Reset reusable resources, this only works here because we do not 'continue' after
       * starting to use attributes.... */
      attributes = g_string_truncate(attributes, 0) ;	    /* Reset attribute string to empty. */

      FMapSegDumpLineReset(dump_data) ;			    /* Reset dump data to empty. */
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

      parent_key= feature_key = source_key = KEY_UNDEFINED ;
      parent_name = parent_class = parent_realtype = NULL ;
      feature_name = feature_class = NULL ;

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
      seg_basic_type = FMAPSEGDUMP_TYPE_BASIC ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */
      seg_basic_type = FMAPSEGDUMP_TYPE_INVALID ;


      dump_data->zmap_dump = zmap_dump ;

      dump_data->extended_features = extended_features ;


      /* Some segs have been flipped from one strand to the other
       * for display (usually for EST 3' reads), we flip them back to their original
       * orientation here. Otherwise people don't know where they are for the dump. */
      if (seg->flags & SEGFLAG_FLIPPED)
	{
	  /* We only do this for homols currently. */
	  hinf = arrp(look->homolInfo, seg->data.i, HOMOLINFO) ;

	  fmapSegFlipHomolStrand(seg, hinf) ;
	}

      /* Normalise the type and set the coords of the feature, note slightly tricky
       * code here, we only need to change things for features that are strand
       * specific _and_ are on the reverse strand. _All_ other features are
       * straight forward. */
      if (seg->type & 0x01 && (seg->type >= SEQUENCE && seg->type <= ALLELE_UP))
	{
	  feature_type = seg->type - 1 ;
	  if (seg->parent_type)
	    parent_type = seg->parent_type - 1 ;
	  x = seg->x2+1 ;
	  y = seg->x1+1 ;
	}
      else
	{
	  feature_type = seg->type ;
	  if (seg->parent_type)
	    parent_type = seg->parent_type ;
	  x = seg->x1+1 ;
	  y = seg->x2+1 ;
	}


      /* gff coords are always forward but we need to know if we have flipped them for
       * reverse strand features...especially for homols.... */
      flipped = FALSE ;
      if (x <  y)
	{
	  flipped = FALSE ;
	  strand = '+' ;				    /* Note that this might be overridden. */
	}
      else if (x > y)
	{
	  int tmp = x ; x = y ; y = tmp ;
	  flipped = TRUE ;
	  strand = '-' ;
	}
      else /* x = y */
	{ 

	  /* TEMP UNTIL WE SET STRAND FOR ALL FEATURES.... */
	  if (seg->type == HOMOL)
	    {
	      if (seg->strand == FMAPSTRAND_FORWARD)
		strand = '+' ;
	      else if (seg->strand == FMAPSTRAND_REVERSE)
		strand = '-' ;
	      else
		strand = '.' ;
	    }
	  else
	    {

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
	      /* THIS IS THE OLD CODE WHICH SEEMS NOT RIGHT NOW..... */

	      strand = strandOfKey(look, seg->source, reversed);
	      if (strand == '.')
		strand = strandOfKey(look, seg->parent, reversed);
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

	      /* Try this.... */
	      if ((strand = strandOfKey(look, seg->key, reversed)) == '.')
		{
		  if ((strand = strandOfKey(look, seg->source, reversed)) == '.')
		    strand = strandOfKey(look, seg->parent, reversed);
		}

	    }
	}

      x += offset ; y += offset ;

      /* Set some defaults. */
      seg_attrs = FMAPSEGDUMPATTRS_IS_FEATURE ;		    /* Most things are features, not subparts. */
      subpart_tag = KEY_NULL ;
      phase = '.' ;
      isScore = FALSE ;
      score = 0.0 ; 
      featName = NULL ;
      methodName = NULL ;
      sourceKey = KEY_UNDEFINED ;
      sourceName = "";					    /* LS otherwise everything ends up
							       being a SEQUENCE */

      feature_key = seg->key ;
      parent_key = seg->parent ;
      source_key = seg->source ;

      switch (feature_type)
	{
	case EXON:
	case INTRON:
	case SEQUENCE:
	  sinf = arrp(look->seqInfo, seg->data.i, SEQINFO) ;

 	  if (seg->key)
	    {
	      featName = className(seg->key);
	    }
	  else
	    {
	      featName = (feature_type == SEQUENCE) ? "sequence" : (feature_type == EXON) ? "exon" : "intron" ;
	    }

	  if (sinf->method)
	    {
	      method_obj_key = sourceKey = sinf->method ;
	    }

	  if (feature_type == SEQUENCE)
	    {
	      is_parent = TRUE ;

	      assInsert (key2sinf, assVoid(seg->key), sinf) ; 
	      if (sinf->flags & SEQ_SCORE)
		{
		  score = sinf->score ;
		  isScore = TRUE ;
		}

	      if (seg->key)
		{
		  if (bIndexTag(seg->key, str2tag("Source_exons"))
		      || bIndexTag(seg->key, str2tag("Coding"))

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
                      /* I don't know why this was here....it seems wrong... */
		      || bIndexTag(seg->key, str2tag("Transcript"))
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

                      )
		    seg_basic_type = FMAPSEGDUMP_TYPE_TRANSCRIPT ;
                  else
                    seg_basic_type = FMAPSEGDUMP_TYPE_SEQUENCE ;

		  if (bIndexTag(seg->key, str2tag("Source_exons")))
		    seg_attrs |= FMAPSEGDUMPATTRS_HAS_SUBPART ;
		}
	    }
	  else
	    {
	      is_child = TRUE ;

	      seg_attrs &= !(FMAPSEGDUMPATTRS_IS_FEATURE) ;

	      if (seg->key)
		parent_realtype = className(seg->key);
	      else
		parent_realtype = "sequence" ;

	      if (feature_type == EXON)
		{
		  if (version == 3)
		    {
		      seg_attrs |= FMAPSEGDUMPATTRS_IS_SUBPART ;
		      subpart_tag = seg->subfeature_tag ;
		    }
		}
	      else
		{
		  seg_attrs |= FMAPSEGDUMPATTRS_IS_SUBPART ;
		  subpart_tag = seg->subfeature_tag ;
		}

	      feature_key = seg->parent ;
	    }
	  break ;

	case CODING:
	  {
	    int exon_start ;

	    is_multiline = TRUE ;
	    is_child = TRUE ;

	    seg_attrs &= !(FMAPSEGDUMPATTRS_IS_FEATURE) ;

	    seg_attrs |= FMAPSEGDUMPATTRS_IS_SUBPART ;
	    subpart_tag = seg->subfeature_tag ;

	    seg_attrs |= FMAPSEGDUMPATTRS_IS_MULTILINE ;

	    featName = "coding_exon" ;
	    assFind (key2sinf, assVoid(seg->parent), &sinf) ;
	    if (sinf->method)
	      method_obj_key = sourceKey = sinf->method ;


#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
	    exon_start = 0 ;

	    /* May need to correct phase for any Start_not_found value that was set. */
	    if (sinf->start_not_found != 0)
	      exon_start = sinf->start_not_found - 1 ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */


#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
	    exon_start = seg->data.i % 3 ;

	    /* For CODING segs that represent the first exon we may need to correct frame for
	     * any Start_not_found value that was set. */
	    if (seg->data.i == 0 && sinf->start_not_found != 0)
	      exon_start = sinf->start_not_found - 1 ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */


	    exon_start = seg->data.i ;

	    switch (exon_start)
	      {
	      case 0:
		phase = '0' ;
		break ;
	      case 1:
		phase = '1' ;
		break ;
	      case 2:
		phase = '2' ;
		break ;
	      }

	    parent_realtype = className(seg->parent) ;

	    break ;
	  }

	case CDS:
	  /* Currently only dumped for zmap as standard behaviour is to derive CDS from CODING segs. */
	  if (zmap_dump)
	    {
	      int exon_start ;

	      seg_attrs &= !(FMAPSEGDUMPATTRS_IS_FEATURE) ;
	      seg_attrs |= FMAPSEGDUMPATTRS_IS_SUBPART ;
              subpart_tag = seg->subfeature_tag ;


	      featName = "CDS" ;
	      assFind (key2sinf, assVoid(seg->parent), &sinf) ;
	      if (sinf->method)
		method_obj_key = sourceKey = sinf->method ;

	      /* AGH...CHECK ALL THIS TOO.... */
	      exon_start = 0 ;

	      /* May need to correct phase for any Start_not_found value that was set. */
	      if (sinf->start_not_found != 0)
		exon_start = sinf->start_not_found - 1 ;

	      switch (exon_start)
		{
		case 0:
		  phase = '0' ;
		  break ;
		case 1:
		  phase = '1' ;
		  break ;
		case 2:
		  phase = '2' ;
		  break ;
		}
	    }
	  break ;


	case HOMOL:
	  {
	    hinf = arrp(look->homolInfo, seg->data.i, HOMOLINFO) ;
	    score = hinf->score ;
	    method_obj_key = sourceKey = hinf->method ;
	    featName = "similarity" ;
	    meth = methodCacheGet(look->mcache, hinf->method, "") ;

	    seg_basic_type = FMAPSEGDUMP_TYPE_ALIGNMENT ;

	    if (hinf->align_id)
	      seg_attrs |= FMAPSEGDUMPATTRS_IS_MULTILINE ;

	    seg_attrs |= FMAPSEGDUMPATTRS_OMIT_NAME ;	    /* Homol name already given in Target attr. */

	    if (meth->flags & METHOD_SCORE)
	      isScore = TRUE ;

	    break ;
	  }

	case FEATURE_OBJ:
	  /* Some things should always be true, e.g. there should always be a method. */
	  feat = arrp(look->feature_info, seg->data.i, FeatureInfo) ;

	  featName = "misc_feature" ;			    /* default to rubbish. */

	  method_obj_key = sourceKey = feat->method ;



#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
	  /* Probably not doing this.... */

	  seg_attrs |= FMAPSEGDUMPATTRS_OMIT_NAME ;	    /* features have no name. */

	  feat = arrp(look->feature_info, seg->data.i, FeatureInfo) ;

	  if (feat->interbase_feature)
	    {
	      /* For GFFv3 dumping  start == end  with start being the upstream base of the
	       * bounding bases of the splice site. */
	      featName = "junction" ;

	      y = x ;
	    }
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */


#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
	  /* IS THIS NEEDED......YES, WHEN WE COMBINE IT WITH MORE GENERAL "FEATURE" stuff. */
	  if (feat->flags & SEQ_SCORE)
	    {
	      score = sinf->score ;
	      isScore = TRUE ;
	    }
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

	  break ;


	case FEATURE:
	case ATG:
	case SPLICE5:
	case SPLICE3:
	  method_obj_key = sourceKey = seg->key ;	    /* Set to method by convert routine. */

	  if (feature_type == SPLICE3)
	    featName = "splice3" ;
	  else if (feature_type == SPLICE5)
	    featName = "splice5" ;
	  else if (feature_type == ATG)
	    featName = "atg" ;
	  else
            {
              featName = "misc_feature" ;
              seg_attrs |= FMAPSEGDUMPATTRS_OMIT_NAME ;	    /* Don't need name, given later anyway. */
            }

	  meth = methodCacheGet(look->mcache, seg->key, "") ;

	  if (meth->flags & METHOD_SCORE)
	    {
	      score = seg->data.f ;
	      isScore = TRUE ;
	    }

	  if (!zmap_dump)
	    {
	      if (!(meth->flags & METHOD_STRAND_SENSITIVE))
		strand = '.' ;
	    }

	  if (meth->flags & METHOD_FRAME_SENSITIVE)
	    phase = '0' ;
	  break ;

	case ASSEMBLY_TAG:
	  tagText = strnew(seg->data.s, handle) ;
	  sourceName = "assembly_tag" ;
	  featName = tagText ;
	  {
	    char *cp ;
	    for (cp = tagText ; *cp && *cp != ':' && 
		   *cp != ' ' && *cp != '\t' && *cp != '\n' ; ++cp) ;
	    if (*cp)
	      {
		*cp++ = 0 ;
		while (*cp == ' ' || *cp == '\t' || *cp == '\n') ++cp ;
	      }
	    tagText = cp ;
	  }
	  break ;

	case ALLELE:
	  if (seg->data.s)
	    {
	      if (*seg->data.s == '-')
		featName = "deletion" ;
	      else
		{
		  char *cp = seg->data.s ;
		  featName = "variation" ;
		  while (*cp)
		    {
		      if (!strchr ("AGCTagct", *cp++))
			featName = "insertion" ;
		    }
		}
	    }
	  break ;

	case EMBL_FEATURE:
	  featName = name(seg->key) ;
	  break ;

	case CLONE_END:
	  featName = name(seg->data.k) ;
	    strand = '.' ;
	  break ;

	case OLIGO:
	  featName = "oligo" ;

	  if (seg->data.i)
	    {
	      sinf = arrp(look->seqInfo, seg->data.i, SEQINFO) ;
	      method_obj_key = sourceKey = sinf->method ;
	    }

	  break ;

	case ASSEMBLY_PATH:
	  {
	    /* OK HERE IS TROUBLE....NEED A METHOD KEY HERE.....FOR ASSEMBLY LIKE STUFF... */

	    /* Dump SMap assembly segs, they have no strand because they are the assembly, not
	     * features placed on it. */
	    methodName = sourceName = ASSEMBLY_PATH_METHOD_NAME ;
	    featName = "region" ;
	    strand = '.' ;

	    break ;
	  }


	default:
	  /* stuff like oligos come out here. */
	  ;
	}


      /* Finalise source and feature fields. */
      if (sourceKey)
	{
	  char *tmp_featname = NULL ;

	  if (assFind (key2source, assVoid(sourceKey), &sourceName))
	    {
	      assFind (key2feat, assVoid(sourceKey), &tmp_featname) ;

	      /* CRAZY, THIS WAS MISSING SOMEHOW... */
	      methodName = sourceName ;
	    }
	  else
	    { 
	      OBJ obj = NULL ;
	      
	      sourceName = name(sourceKey) ;

	      if ((obj = bsCreate(sourceKey)))
		{
		  if (!raw_methods)
		    bsGetData(obj, str2tag("GFF_source"), _Text, &sourceName) ;
		  
		  if (bsGetText(obj, str2tag("GFF_feature"), &tmp_featname))
		    {
		      tmp_featname = strnew(tmp_featname, handle) ;
		      assInsert(key2feat, assVoid(sourceKey), tmp_featname) ;
		    }
		}

	      sourceName = strnew(sourceName, handle) ;
	      assInsert (key2source, assVoid(sourceKey), sourceName) ;
	      
	      methodName = sourceName ;
	      
	      if (obj)				    /* only after strnew(sourceName) !!! */
		bsDestroy(obj);
	    }
	  

	  /* If the method gives a feature name via the GFF_feature tag then use that
	   * instead of any derived name. BUT note that in cases where a seg is really a child seg
	   * as in exons/introns we want to keep the feature type we've already
	   * set (e.g. "exon"), the GFF_feature string applies only to the parent seg. */
	  if (tmp_featname
	      && (feature_type != EXON && feature_type != INTRON && feature_type != CODING))
	    {
	      if (!zmap_dump || feature_type != CDS)
		featName = tmp_featname ;
	    }
	}


      /* GFF requires the first 8 fields to be present, where there is no sensible value,
       * the field is set to "." */
      if (!sourceName || !*sourceName)
	sourceName = FMAPSEGDUMP_UNKNOWN_FIELD ;
      

      if (!methodName || !*methodName)      /* if still no methodName, do your best. */
        {
          if (sourceKey)
            {
	      methodName = name(sourceKey);
            }
          else
            methodName = FMAPSEGDUMP_UNKNOWN_FIELD ;
        }





#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
      if (g_ascii_strcasecmp(methodName, "RNASeq_splice") == 0)
        printf("found it\n") ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */





      /* ROB, I would like to remove this in the end but I think we need to leave it
       * in place for now..... */
      if (!featName)		/* from method or from seg->type above */
	featName = fMapSegTypeName[feature_type] ;


      /*
       * Now throw stuff away - if any restrictions apply, then
       *                          if we're excluding things and we find this one in the set
       *                             then exclude it. 
       *                          if we're including and this isn't what we want, exclude it.
       */

      if (featSet && dictMax(featSet))
	{
	  if (!include_feature && dictFind(featSet, featName, 0))
	    goto clean_up ;
	  if (include_feature && !dictFind(featSet, featName, 0))
	    goto clean_up ;
	}

      if (sourceSet && dictMax(sourceSet)) 
        {
	  if (!include_source && dictFind(sourceSet, sourceName, 0)) 
	    goto clean_up ;
	  if (include_source  && !dictFind(sourceSet, sourceName, 0)) 
	    goto clean_up ;
	}

      /* This is horrific coding by Rob Clack...oh gosh this needs sorting out....
       * I've plugged the object destroy holes.... */
      if (methodSet && dictMax(methodSet))
        {
	  if (!include_method) 
	    {
	      if (dictFind(methodSet, methodName, 0))
		{
		  goto clean_up ; ;
		}
	      else
		{
		  /* well, maybe its parent object has this method */
		  OBJ obj ;
		  KEY method_key = str2tag("Method") ;
		  KEY parent_method ;
		  
		  if ((obj = bsCreate(seg->parent)))
		    {
		      if (bsFindTag(obj, method_key)
			  && bsGetKey(obj, method_key, &parent_method ))
			{
			  if (dictFind(methodSet, name(parent_method), 0))
			    {
			      bsDestroy(obj) ;

			      goto clean_up ;;
			    }
			}

		      bsDestroy(obj) ;
		    }
		} 
	    }
	  else                     /* ie methods to be included */
	    {
	      if (!dictFind(methodSet, methodName, 0))
		{
		  /* well, maybe its parent object has this method */
		  OBJ obj ;
		  KEY method_key = str2tag("Method");
		  KEY parent_method;

		  if ((obj = bsCreate(seg->parent)))
		    {
		      if (bsFindTag(obj, method_key)
			  && bsGetKey(obj, method_key, &parent_method ))
			{
			  if (!dictFind(methodSet, name(parent_method), 0))
			    {
			      bsDestroy(obj) ;

			      goto clean_up ;
			    }
			}
		      else             /* nope, method not in this object or its parent */
			{
			  bsDestroy(obj);
			  goto clean_up ;
			}

		      bsDestroy(obj);
		    }
		  else
		    goto clean_up ;
		} 
	    }
	}
    

      /* IF.....user just wants list of source/features, just accumulate stats,
       * don't do ANYTHING ELSE. */
      if (list_type != FMAPSEGDUMP_FEATURES)
	{
	  int k ;

	  if (list_type == FMAPSEGDUMP_LIST_FEATURES)
	    {
	      dictAdd(listSet, messprintf("%s\t%s",sourceName,featName), &k) ;
	      ++array(stats, k, int) ;
	    }
	  else /* list_type == FMAPSEGDUMP_LIST_KEYS */
	    {
	      char *seg_name = NULL ;

	      if ((seg_name = fmapSeg2SourceName(seg)))
		dictAdd(listSet, seg_name, &k) ;
	    }

	  goto clean_up ;
	}

      /* ELSE...we go on to write out the line. */
       

      /* LS/AcePerl: fixup reversed reference sequence */
      if (reversed)
	{  
	  tmp = look->zoneMax + offset + 1 - y;
	  y   = look->zoneMax + offset + 1 - x;
	  x   = tmp;

	  if ( strand == '+' )
	    strand = '-';
	  else if ( strand == '-' )
	    strand = '+';
	}
 


      /* 
       * Add attributes field (i.e. all the extras that follow the 7 mandatory fields).
       */

      switch (feature_type)
	{
	case SEQUENCE:
	case EXON:
	case CODING:
	  if (seg->parent)
	    {

	      if (feature_type == SEQUENCE)
		{
		  feature_name = name(seg->parent) ;
		  feature_class = className(seg->parent) ;
		}
	      else if (feature_type == CODING)
		{
		  feature_key = parent_key ;		    /* set to parent, coding segs have _CDS as seg->key. */
		}
	      else
		{
		  feature_name = parent_name = name(seg->parent) ;
		  feature_class = parent_class = className(seg->parent) ;
		}

	      if (version == 2)
		FORMATFEATURENAME(attributes, use_name, "", seg->parent) ;

	      if (version == 3 || (zmap_dump && feature_type == SEQUENCE))
		{
		  /* Output the start/end_not_found information....needs to go...legacy... */
                  if (version == 2)
                    {
                      if (sinf->start_not_found)
                        g_string_append_printf(attributes, "start_not_found %d" V2_ATTRS_SEPARATOR,
                                               sinf->start_not_found) ;

                      if (sinf->end_not_found)
                        g_string_append_printf(attributes, "end_not_found" V2_ATTRS_SEPARATOR) ;
                    }


		  if (sinf->start_not_found)
		    {
		      dump_data->data.transcript.start_not_found = TRUE ;
		      dump_data->data.transcript.start_offset = sinf->start_not_found ;
		    }

		  if (sinf->end_not_found)
		    dump_data->data.transcript.end_not_found = TRUE ;
		}
	    }
	  break ;


	case CDS:
	  if (zmap_dump)
	    {
              /* ONLY WANT THIS FOR GFF2.... */


	      /* Output the CDS. */
	      if (seg->parent)
		{
		  feature_name = name(seg->parent) ;
		  feature_class = className(seg->parent) ;

		  if (version == 2)
		    FORMATFEATURENAME(attributes, use_name, "", seg->parent) ;
		}
	    }
	  break ;


	case INTRON:
	  {
	    BOOL isData = FALSE ; 

	    /* Introns derived from Source_exons always have a parent. Confirmed introns
	     * only have a parent if they have been combined with existing Introns. */
	    if (seg->parent)
	      {
		if (version == 2)
		  FORMATFEATURENAME(attributes, use_name, "", seg->parent) ;
		isData = TRUE ;

		parent_name = name(seg->parent) ;
		parent_class = className(seg->parent) ;
	      }

	    sinf = arrp(look->seqInfo, seg->data.i, SEQINFO) ;
	    if (sinf->flags & SEQ_CONFIRMED)
	      {
		int i ;

		for (i = 0 ; i < arrayMax(sinf->confirmation) ; i++)
		  {
		    ConfirmedIntronInfo confirm = arrayp(sinf->confirmation, i,
							 ConfirmedIntronInfoStruct) ;

		    g_string_append_printf(attributes, "Confirmed_%s", confirm->confirm_str) ;

		    if (confirm->confirm_sequence)
		      g_string_append_printf(attributes, " %s", name(confirm->confirm_sequence)) ;

		    g_string_append(attributes, V2_ATTRS_SEPARATOR) ;

		  }
	      }
	    break ;
	  }

	case HOMOL:
	  {
	    char *length_tag = "Length" ;
	    KEY length_key = str2tag(length_tag) ;
	    char *match_string_keyword = "Match_string" ;

	    feature_name = name(seg->key) ;
	    feature_class = className(seg->key) ;
	    
	    dump_data->type = FMAPSEGDUMP_TYPE_ALIGNMENT ;

	    dump_data->data.match.target = seg->key ;

	    hinf = arrp(look->homolInfo, seg->data.i, HOMOLINFO) ;

	    if (hinf->strand == FMAPSTRAND_FORWARD)
	      dump_data->data.match.strand = '+' ;
	    else if (hinf->strand == FMAPSTRAND_REVERSE)
	      dump_data->data.match.strand = '-' ;
	    else
	      dump_data->data.match.strand = '.' ;

	    dump_data->data.match.start = hinf->x1 ;
	    dump_data->data.match.end = hinf->x2 ;

	    dump_data->data.match.align_id = hinf->align_id ;

	    /* We can only dump Coords if the gaps were mapped because they are in the parent
	     * reference frame, strings are relative so can always be dumped. */
	    if (hinf->gaps)
	      {
		if (meth->flags & METHOD_EXPORT_COORDS)
		  {
		    dump_data->data.match.gaps = getGapsArray(dump_data->data.match.gaps, hinf->gaps, offset) ;
		  }
		else if (meth->flags & METHOD_EXPORT_STRING)
		  {
		    if (hinf->match_str)
		      {
			dump_data->data.match.gaps = getGapsArray(dump_data->data.match.gaps, hinf->gaps, offset) ;
		      }
		    else if (meth->flags & METHOD_EXPORT_STRING)
		      {
			if (hinf->match_str)
			  g_string_append_printf(attributes, "%s \"%s\"" V2_ATTRS_SEPARATOR,
						 hinf->match_type, hinf->match_str) ;
			else
			  messerror("%s %s: conversion of Gaps to String not supported.", sourceName, feature_name) ;
		      }
		    else
		      messerror("%s %s: conversion of Gaps to String not supported.", sourceName, feature_name) ;
		  }
	      }
	    else
	      {
		if ((meth->flags & METHOD_EXPORT_STRING) && hinf->match_str)
		  {
		    if ((meth->flags & METHOD_EXPORT_STRING) && hinf->match_str)
		      {
			g_string_append_printf(attributes, V2_ATTRS_SEPARATOR "%s \"%s\"",
					       hinf->match_type, hinf->match_str) ;
		      } 
		  }
	      }

	    if (version == 3 || zmap_dump)
	      {
		if (bIndexTag(seg->key, length_key))
		  {
		    OBJ obj ;

		    if ((obj = bsCreate(seg->key)))
		      {
			int length = 0 ;
			
			if (bsGetData(obj, length_key, _Int, &length))
			  {
			    dump_data->data.match.length = length ;
			  }

			bsDestroy(obj) ;
		      }
		  }

		if (class(seg->key) == pickWord2Class("protein"))
		  {
		    dump_data->data.match.align_type = FMAPSEGDUMP_ALIGN_DNAPEP ;

		    if (pepObjHasOwnPeptide(seg->key))
		      dump_data->data.match.own_sequence = TRUE ;
		  }
		else if (class(seg->key) == pickWord2Class("sequence"))
		  {
		    dump_data->data.match.align_type = FMAPSEGDUMP_ALIGN_DNADNA ;

		    if (dnaObjHasOwnDNA(seg->key))
		      dump_data->data.match.own_sequence = TRUE ;
		  }
		else /* for motifs and dna matches. */
		  {
		    dump_data->data.match.align_type = FMAPSEGDUMP_ALIGN_DNADNA ;
		  }
	      }

	    break ;
	  }

	case FEATURE_OBJ:
	  if (seg->parent)
	    {
	      feature_name = name(seg->key) ;
	      feature_class = className(seg->parent) ;

	      if (version == 2)
		FORMATFEATURENAME(attributes, use_name, "", seg->parent) ;
	    }
	  break ;

	case FEATURE:
	case SPLICE5:
	case SPLICE3:
	case ATG:
	  {
	    BOOL isTab = FALSE ;

	    /* kind of wierd stuff with this note thingy...... */	    
	    if (seg->parent)
	      {
		char *text ;

		text = cleanNewlines(dictName (look->featDict, -seg->parent), handle) ;

		if (version == 3 || zmap_dump)
		  {
                    if (version == 2)
                      g_string_append_printf(attributes, "Name \"%s\"" V2_ATTRS_SEPARATOR, text) ;
                    else
                      /* TOTAL HACK....REVISE...ASAP...SHOULD IN DUMP DATA... */
                      g_string_append_printf(attributes, "Name=%s", text) ;
		  }
		else if (version == 3)
		  {
		    g_string_append_printf(attributes, "%s", text) ;
		  }
		else
		  {
		    g_string_append_printf(attributes, "Note \"%s\"" V2_ATTRS_SEPARATOR, text) ;
		    isTab = TRUE ;
		  }
	      }

	    if (assFind (look->chosen, SEG_HASH(seg), 0))
	      g_string_append(attributes, "Selected" V2_ATTRS_SEPARATOR) ;
	    else if (assFind (look->antiChosen, SEG_HASH(seg), 0))
	      g_string_append(attributes, "Antiselected" V2_ATTRS_SEPARATOR) ;

	    break ;
	  }

	case ASSEMBLY_TAG:
	  if (tagText && *tagText)
	    {
	      g_string_append_printf(attributes, "Note \"%s\""  V2_ATTRS_SEPARATOR, cleanNewlines(tagText, handle)) ;
	    }
	  break ;

	case ALLELE:

	  feature_name = name(seg->key) ;
	  feature_class = "Allele" ;

	  g_string_append_printf(attributes, "Allele \"%s\"" V2_ATTRS_SEPARATOR, name(seg->key)) ;

	  if (seg->data.s && *seg->data.s != '-')
	    {
	      if (strcmp (featName, "variation") == 0)
		g_string_append_printf(attributes, "Variant \"%s\" " V2_ATTRS_SEPARATOR, cleanNewlines(seg->data.s, handle)) ;
	      else
		g_string_append_printf(attributes, "Insert \"%s\"" V2_ATTRS_SEPARATOR, cleanNewlines(seg->data.s, handle)) ;
	    }
	  break ;

	case EMBL_FEATURE:
	  if (seg->data.s)
	    {
	      g_string_append_printf(attributes, "Note \"%s\"" V2_ATTRS_SEPARATOR, cleanNewlines(seg->data.s, handle)) ;
	    }
	  break ;

	case CLONE_END:
	  g_string_append_printf(attributes, "Clone \"%s\"" V2_ATTRS_SEPARATOR, name(seg->key)) ;
	  break ;

	case ASSEMBLY_PATH:
	  {
	    /* Set up the attributes which give all the details we need of this assembly section. */
	    SMapAssembly fragment ;
	    int i ;

	    sinf = arrp(look->seqInfo, seg->data.i, SEQINFO) ;

	    fragment = &(sinf->assembly) ;

	    g_string_append_printf(attributes,
				   "Assembly_source \"%s:%s\"" V2_ATTRS_SEPARATOR
				   "Assembly_strand %c" V2_ATTRS_SEPARATOR
				   "Assembly_length %d" V2_ATTRS_SEPARATOR,
				   className(seg->key), name(seg->key),
				   (fragment->strand == STRAND_FORWARD ? '+' : '-'),
				   fragment->length) ;

	    if (fragment->map)
	      {
		g_string_append(attributes, "Assembly_regions ") ;
		for (i = 0 ; i < arrayMax (fragment->map) ; ++i)
		  {
		    SMapMap *map = arrp(fragment->map, i, SMapMap) ;
		    int start, end ;

		    if (map->r1 < map->r2)
		      {
			start = map->r1 ;
			end = map->r2 ;
		      }
		    else
		      {
			start = map->r2 ;
			end = map->r1 ;
		      }

		    if (i != 0)
		      g_string_append(attributes, ",") ;

		    g_string_append_printf(attributes, " %d %d", start + offset, end + offset) ;
		  }
		g_string_append(attributes, V2_ATTRS_SEPARATOR) ;
	      }

	    break ;
	  }

	default: ;
	}


      /* Dump extras for zmap, e.g. the sequence class stuff... */
      if (version == 3 || zmap_dump)
	{
	  if (feature_type == SEQUENCE)
	    {
	      char *web_str = "Web_location" ;
	      KEY web_tag = str2tag(web_str) ;
	      KEY url_key = KEY_UNDEFINED ;
	      char *locus_str = "Locus" ;
	      KEY locus_tag = str2tag(locus_str) ;
	      KEY locus_key = KEY_UNDEFINED, clone_key = KEY_UNDEFINED ;

	      if (bIndexGetKey(seg->key, web_tag, &url_key))
		{
		  char *url_str = "URL" ;
		  KEY url_tag = str2tag(url_str) ;
		  OBJ url_obj= NULL ;
		  
		  if (bIndexTag(url_key, url_tag)
		      && (url_obj = bsCreate(url_key)))
		    {
		      char *http_link = NULL ;
		  
		      if (bsGetData(url_obj, url_tag, _Text, &http_link)
			  && http_link && *http_link)
			{
			  g_string_append_printf(attributes, "%s \"%s\"" V2_ATTRS_SEPARATOR, 
						 url_str, http_link) ;
			}

		      bsDestroy(url_obj);
		    }
		}

	      if (bIndexGetKey(seg->key, locus_tag, &locus_key))
		{
		  g_string_append_printf(attributes, "%s \"%s\"" V2_ATTRS_SEPARATOR, 
					 locus_str, name(locus_key)) ;
		}

	      if (g_ascii_strcasecmp("genomic_canonical", name(sourceKey)) == 0
		  && bIndexGetKey(seg->key, str2tag("Clone"), &clone_key))
		{
                  if (version == 2)
                    g_string_append_printf(attributes, "%s \"%s\"" V2_ATTRS_SEPARATOR, 
                                           "Known_name", name(clone_key)) ;
                  else
                    dump_data->data.sequence.known_name = clone_key ;
		}
	    }
	  else if (feature_type == CLONE_END)
	    {
	      g_string_append_printf(attributes, "Clone_End \"%s\"" V2_ATTRS_SEPARATOR "Position %d %d" V2_ATTRS_SEPARATOR, 
				     name(seg->data.k), seg->x1, seg->x2) ;
	    }
	}


      dump_data->type = seg_basic_type ;


      /* Call the applications function to output the record. */
      {
	float *tmp_score = isScore ? &score : NULL ;	    /* float == NULL  => score not relevant. */
	BOOL result ;
	char *err_msg = NULL ;

	result = (*app_cb)(app_data,
			   sequence_name, sequence_class,
			   method_obj_key, methodName,
			   is_structural,
			   seg_attrs,
			   subpart_tag,
			   parent_key, parent_type, parent_realtype,
			   feature_key, feature_type, featName,
			   source_key,
			   seg_basic_type,
			   sourceName,
			   x, y,
			   tmp_score, strand, phase,
			   attributes->str, comment,
			   dump_data,
			   &err_msg) ;

	/* NOTE that call may suceed but produce warnings, fail but not produce warnings
	 * because verbose is FALSE or produce errors when failing. */
	if (err_msg && *err_msg)
	  {
	    messerror(err_msg) ;
	    messfree(err_msg) ;
	  }

      }


      /* Reset reusable resources, this only works here because we do not 'continue' after
       * starting to use attributes.... */
      attributes = g_string_truncate(attributes, 0) ;	    /* Reset attribute string to empty. */

      FMapSegDumpLineReset(dump_data) ;			    /* Reset dump data to empty. */


      /* Some state needs undoing again. */
	clean_up:


      /* We flipped some segs to dump them, now we flip them back. */
      if (seg->flags & SEGFLAG_FLIPPED)
	{
	  /* We only do this for homols currently. */

	  fmapSegFlipHomolStrand(seg, hinf) ;
	}

    }


  /* Clean up. */
      handleDestroy(handle) ;
      g_string_free(attributes, TRUE) ;
  FMapSegDumpLineDestroy(dump_data) ;


  return result ;
}



/*
 * --------------------  INTERNAL FUNCTIONS  --------------------
 */



FMapSegDumpLine FMapSegDumpLineCreate(void)
{
  FMapSegDumpLine dump_line = NULL ;

  dump_line = g_new0(FMapSegDumpLineStruct, 1) ;

  /* holds gaps data, large enough for coords for 50 gaps. */
  dump_line->data.match.gaps = g_array_sized_new(FALSE, TRUE, sizeof(FMapSegDumpAlignStruct), 200) ;

  return dump_line ;
}


void FMapSegDumpLineReset(FMapSegDumpLine dump_line)
{
  if (dump_line->type == FMAPSEGDUMP_TYPE_ALIGNMENT)
    {
      FMapSegDumpMatch match = &(dump_line->data.match) ;

      match->target = 0 ;
      match->align_type = FMAPSEGDUMP_ALIGN_INVALID ;
      match->start = match->end = 0 ;
      match->length = 0 ;
      match->strand = '.' ;
      match->gaps = g_array_set_size(match->gaps, 0) ;
      if (match->align_str)
	{
	  g_free(match->align_str) ;
	  match->align_str = NULL ;
	}
      match->own_sequence = FALSE ;
    }
  else if (dump_line->type == FMAPSEGDUMP_TYPE_TRANSCRIPT)
    {
      FMapSegDumpTranscript transcript = &(dump_line->data.transcript) ;

      transcript->start_not_found = 0 ;
      transcript->end_not_found = FALSE ;

    }



  return ;
}


void FMapSegDumpLineDestroy(FMapSegDumpLine dump_line)
{
  if (dump_line->data.match.align_str)
    {
      g_free(dump_line->data.match.align_str) ;
      dump_line->data.match.align_str = NULL ;
    }

  g_array_free(dump_line->data.match.gaps, TRUE) ;

  g_free(dump_line) ;

  return ;
}



static char *cleanNewlines (char *s, STORE_HANDLE handle)
{ 
  int n = 0 ;
  char *cp, *cq, *copy ;

  for (cp = s ; *cp ; ++cp)
    if (*cp == '\n') ++n ;

  if (!n)
    return s ;

  copy = halloc (cp-s+n+1, handle) ;
  for (cp = s, cq = copy ; *cp ; ++cp)
    if (*cp == '\n') 
      { *cq++ = '\\' ; *cq++ = 'n' ; }
    else
      *cq++ = *cp ;
  *cq = 0 ;

  return copy ;
}

/***********************************/

/* Determine direction of key in Dump. If the dump is of a sequence
 * which is reversed wrt to the root of the fMap, rootReversed == 1) */
static char strandOfKey(FeatureMap look, KEY key, int rootReversed)
{
  SMapKeyInfo *info = sMapKeyInfo(look->smap, key);
  BOOL isReverse;

  if (!info)
    return '.';
  
  isReverse = sMapIsReverse(info, 1, 0);

  if (isReverse)
    return rootReversed == 0 ? '-' : '+';
      
  return rootReversed == 1 ? '-' : '+';
}


/* Look for a parent of "curr" that is a "clone". The acedb definition of a "clone"
 * is generally taken to be something that has DNA and a "Clone" tag. The start/end
 * coords of "curr" are needed to check that it maps into any given smap'd key. */
static BOOL getClone(SMap *smap, KEY curr, int start, int end, KEY *source_out, KEY *clone_out)
{
  BOOL result = FALSE ;
  KEY source = KEY_UNDEFINED, clone = KEY_UNDEFINED ;

  if (sMapFindSpan(smap, &source, &start, &end, isCloneObj))
    {
      if (bIndexGetKey(source, str2tag("Clone"), &clone))
	{
	  *source_out = source ;
	  *clone_out = clone ;
	  result = TRUE ;
	}
    }

  return result ;
}

/* Callback from sMapFindSpan() for getClone(). */
static BOOL isCloneObj(KEY key)
{
  BOOL result = FALSE ;

  if (bIndexTag(key, str2tag("DNA")) && bIndexTag(key, str2tag("Clone")))
    result = TRUE ;

  return result ;
}



static GArray *getGapsArray(GArray *align_gaps_in, Array gaps_in, int offset)
{
  GArray *align_gaps ;
  Array gaps ;
  int i ;

  gaps = arrayCopy(gaps_in);

  align_gaps = g_array_set_size(align_gaps_in, arrayMax(gaps)) ;

  for (i = 0 ; i < arrayMax(gaps) ; i++) 
    {
      SMapMap *m = arrp(gaps, i, SMapMap) ;
      FMapSegDumpAlign dump_gaps = &g_array_index(align_gaps, FMapSegDumpAlignStruct, i)  ;

      dump_gaps->ref_start =  m->r1 + offset ;
      dump_gaps->ref_end = m->r2 + offset ;
      dump_gaps->match_start = m->s1 ;
      dump_gaps->match_end = m->s2 ;
    }

  arrayDestroy(gaps);

  return align_gaps ;
}


static char *getGapsStr(Array gaps, SMapAlignStrFormat str_format)
{
  char *align_str = NULL ;


  align_str = sMapGaps2AlignStr(gaps, str_format) ;


  return align_str ;
}




