/*  File: smapconvert.c
 *              
 *  Author: Simon Kelley (srk@sanger.ac.uk) & Ed Griffiths (edgrif@sanger.ac.uk)
 *  Copyright (c) J Thierry-Mieg and R Durbin, 2001
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
 * This file is part of the ACEDB genome database package, written by
 * 	Richard Durbin (Sanger Centre, UK) rd@sanger.ac.uk, and
 *	Jean Thierry-Mieg (CRBM du CNRS, France) mieg@kaa.crbm.cnrs-mop.fr
 *
 * Description: Uses the smap system to produce arrays of features,
 *              these arrays are then used by the GFF dumper or various
 *              display routines.
 *              
 * Exported functions: See smapconvert.h
 * HISTORY:
 * Last edited: Sep 29 11:42 2011 (edgrif)
 *
 * Created: Wed Dec 12 10:17:57 2001 (edgrif)
 * CVS info:   $Id: smapconvert.c,v 1.109 2012-11-07 13:53:10 edgrif Exp $
 *-------------------------------------------------------------------
 */

#undef FMAP_DEBUG

#include <glib.h>

#include <wh/acedb.h>
#include <whooks/classes.h>
#include <whooks/tags.h>
#include <whooks/systags.h>
#include <wh/smap.h>
#include <wh/query.h>
#include <wh/aceio.h>
#include <wh/bindex.h>
#include <wh/dna.h>
#include <wh/lex.h>
#include <wh/pick.h>
#include <wh/so.h>
#include <w7/smapconvert_.h>


/* We should be thinking about moving some of the fmap_.h stuff into a new header and 
 * only including that header in this file so that smap and fmap are truly split apart. */
#include <w7/fmap_.h>




typedef struct _SMapFeatureMapRec
{
  STORE_HANDLE handle ;
  KEY seqOrig ;
  KEY seqKey ;
  SMap *smap ;
  int start ;
  int stop ;
  int length ;

  int fullLength ;					    /* probably don't need this... */

  int origin ;
  int zoneMin, zoneMax ;

  Array dna ;
  KEYSET method_set ;
  DICT *featDict ;
  DICT *sources_dict;
  BOOL include_methods;
  BOOL include_sources;
  MethodCache mcache ;
  Array segs ;
  Array seqInfo ;
  Array homolInfo ;
  Array feature_info ;

  BitSet homolBury ;					    /* same index as homolInfo */
  BitSet homolFromTable ;				    /* MatchTable or Tree? (index as
							       homolInfo) */
} SMapFeatureMapRec ;



#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
/* Passed to each object conversion routine in turn, keeps all the state necessary for such
 * conversions. The alternative to this struct would be to have the routines all have a large
 * number of arguments which seems not very attractive. */
typedef struct
{
  KEY seq_orig ;					    /* id of original sequence to be
							       mapped. */

  SMap *smap ;						    /* Current smap. */
  SMapKeyInfo *info ;
  SMapStatus status ;
  int y1, y2 ;						    /* Position of current feature. */

  KEY key ;						    /* Key of current object. */
  BOOL is_Sequence_class ;				    /* Is current obj of class ?Sequence */
  OBJ obj ;						    /* Current object. */

  Array units ;						    /* Used for bsFlatten of features. */

  DICT *mapped_dict ;					    /* hash of features names that get mapped. */

  KEYSET method_set ;					    /* which methods are to be smap'ed. */
  MethodCache method_cache ;				    /* Fast access cache of method obj data. */
  DICT sources_dict;
  BOOL include_methods;
  BOOL include_sources;

  Array segs_array ;					    /* Segs array to add features to. */


  STORE_HANDLE segs_handle ;				    /* For any segs related allocation. */

  Array seqinfo ;					    /* Separate of info. describing sets
							       of segs. */

  Array homol_info ;

  Array feature_info ;

  BitSet homolBury ;					    /* same index as homolInfo */
  BitSet homolFromTable ;				    /* MatchTable or Tree? (index as
							       homolInfo) */

  int length ;						    /* only needed once, can we get rid of */
							    /* it altogether....*/


  /* These are needed for when one object produces a hierachy of segs (e.g. a sequence with exons)
   * and we need to keep track of the first seg that represents the "parent" of the subsequent
   * segs. */
  int parent_seg_index ;				    /* Index of sequence class seg from
							       which exons etc. are created. */
  int parent_seqinfo_index ;					    /* Index to current seqinfo. */

  /* Used to match up pairs of homology matches (e.g. EST 5' & 3'). */
  Array all_paired_reads ;
  Associator key2readpair ;

} SmapConvInfo ;


/* structs for holding read pair information (e.g. ESTs). */
typedef struct
{
  KEY key ;						    /* homol key. */
  int pos ;						    /* seg coord of homol nearest to its */
							    /* paired read. */
} ReadPosStruct, *ReadPos ;

typedef struct
{
  int homol_index ;					    /* a paired read homol seg, used for
							       copying parent key etc. */
  ReadPosStruct five_prime ;
  ReadPosStruct three_prime ;
} ReadPairStruct, *ReadPair ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */



/* Conversion routines for objects of different classes. */
static void convertObj(SmapConvInfo *conv_info, DICT *featDict) ;

/* Conversion routines for features hanging off an object. */
static BOOL convertSequence(SmapConvInfo *conv_info) ;
static BOOL convertFeatureObj(SmapConvInfo *conv_info) ;
static void convertHomol(SmapConvInfo *conv_info) ;
static void convertExons(SmapConvInfo *conv_inf, BOOL *exons_found) ;
static void convertCDS(SmapConvInfo *conv_info, int *cds_min_out, BOOL exons_found) ;
static void convertStart(SmapConvInfo *conv_info, int cds_min) ;
static void convertEnd(SmapConvInfo *conv_info) ;
static void convertFeature(SmapConvInfo *conv_info, DICT *featDict) ;
static void convertVisible(SmapConvInfo *conv_info) ;
static void convertAssemblyTags(SmapConvInfo *conv_info) ;
static void convertAllele(SmapConvInfo *conv_info) ;
static void convertCloneEnd(SmapConvInfo *conv_info, KEY clone_tag) ;
static void convertOligo(SmapConvInfo *conv_info) ;
static void convertConfirmedIntrons(SmapConvInfo *conv_info) ;
static void convertEMBLFeature(SmapConvInfo *conv_info) ;
static void convertSplices(SmapConvInfo *conv_info) ;



/* Utility functions.                                                        */
static BOOL isOldAlleleModel(OBJ obj) ;
static void setDefaultMethod(OBJ obj, SEQINFO *sinf) ;
static BOOL getCDSPosition(Array segs, Array seqinfo, KEY parent, Array *index) ;
static void addReadPair(Array all_paired_reads, Associator key2readpair,
			KEY this_read, KEY paired_read,	Array segs, int homol_index) ;
Array mergeIntronConfirmation(Array confirm_inout, Array confirm_in, STORE_HANDLE handle) ;
static BOOL segFindBounds(Array segs, SegType type, int *min, int *max) ;


#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
static consMapDNAErrorReturn callback(KEY key, int position) ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */


static KEYSET allMethods(void) ;

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
static void addToSet (ACEIN command_in, DICT *dict) ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */


static void makeMappedDict(DICT *dict, Array segs) ;


#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
/* For dumping seg information.                                              */
static void segDumpToStdout(Array segs) ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */





/*               Globals                  */


#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
static BOOL reportErrors_G ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */





/*                                                                           */
/* ----------------    External routines   --------------------------------- */
/*                                                                           */



/* The business: this is the routine that will construct the array of
 * features to be used by fmap, the GFF dumper etc.
 *
 * Some notes -
 *
 * This routine does _NOT_ know about fmap, if you introduce fmap into this
 * routine YOU WILL BE SHOT.
 *
 * The arrays passed in are expected to be allocated but empty and will be
 * filled with seg/seqinfo/homol data. This gives the caller the freedom to
 * create/recreate/destroy arrays on handles or not as they see fit.
 * 
 * The args here are crazy and should be reduced/simplfied, but one step at a
 * time.....
 */
BOOL sMapFeaturesMake(SMap *smap, KEY seqKey, KEY seqOrig,
		      int start, int stop, int length,
		      Array oldsegs, BOOL complement,
		      MethodCache mcache, DICT *mapped_dict_inout,
		      Array dna,
		      KEYSET method_set, BOOL include_methods,
		      DICT *featDict, DICT *sources_dict, BOOL include_sources,
		      Array segs_inout,
		      Array seqinfo_inout, Array homolInfo_inout, Array featureinfo_inout,
		      BitSet homolBury_inout, BitSet homolFromTable_inout,
		      BOOL keep_selfhomols, BOOL keep_duplicate_introns,
		      STORE_HANDLE segsHandle)
{

  KEYSET allKeys;
  int i;
  SEG *seg;
  Array segs = segs_inout ;
  Array seqinfo = seqinfo_inout ;
  Array units = NULL;
  /* some more thought needed about how this is filled in and persists in    */
  /* this function, some room for optimising.                                */
  SmapConvInfo conv_info ;
  int seq_class = pickWord2Class("Sequence") ;
  Array all_paired_reads ;
  Associator key2readpair ;


  /* Set up for paired reads of homols. */
  all_paired_reads = arrayCreate(30, ReadPairStruct) ;
  key2readpair = assCreate() ;


  allKeys = sMapKeys(smap, NULL);

  /* The master seg, n.b. has no parent.                                     */
  seg = arrayp(segs, arrayMax(segs), SEG);
  seg->parent = seg->source = 0;
  seg->key = seqKey;
  seg->x1 = start;
  seg->x2 = stop;
  seg->type = MASTER;
  seg->flags = SEGFLAG_UNSET ;

  /* Process all the objects that were smap'd successfully. */
  for (i = 0 ; i < keySetMax(allKeys) ; i++)
    {
      KEY key ;
      SMapKeyInfo *info ;
      SMapStatus status ;
      int y1, y2 ;

      key = keySet(allKeys, i) ;
      info = sMapKeyInfo(smap, key) ;
      status = sMapMap(info, 1, 0, &y1, &y2, NULL, NULL) ;

      /* If an object overlaps the smap'd area at all, then process it. */
      if (SMAP_STATUS_INAREA(status))
	{
 	  OBJ obj ;

	  if ((obj = bsCreate(key)))
	    {
	      /* Create segs for any features hanging off this object. */
	      units = arrayReCreate (units, 256, BSunit);
	      conv_info.smap = smap ;
	      conv_info.info = info ;
	      conv_info.key = key ;
	      if (class(conv_info.key) == seq_class)
		conv_info.is_Sequence_class = TRUE ;
	      else
		conv_info.is_Sequence_class = FALSE ;
	      conv_info.obj = obj ;
	      conv_info.status = status ;
	      conv_info.y1 = y1 ;
	      conv_info.y2 = y2 ;
	      conv_info.units = units ;
	      conv_info.mapped_dict = mapped_dict_inout ;
	      conv_info.method_set = method_set ;
	      conv_info.method_cache = mcache ;
	      conv_info.sources_dict = sources_dict;
	      conv_info.include_methods = include_methods;
	      conv_info.segs_array = segs ;
	      conv_info.segs_handle = segsHandle ;
	      conv_info.seqinfo = seqinfo ;
	      conv_info.homol_info = homolInfo_inout ;
	      conv_info.feature_info = featureinfo_inout ;

	      conv_info.homolBury = homolBury_inout ;
	      conv_info.homolFromTable = homolFromTable_inout ;

	      conv_info.length = length ;

	      /* Make these invalid so we fail if we look at this when we shouldn't */
	      conv_info.parent_seqinfo_index = -1 ;
	      conv_info.parent_seg_index = -1 ;

	      conv_info.all_paired_reads = all_paired_reads ;
	      conv_info.key2readpair = key2readpair ;

	      conv_info.seq_orig = seqOrig ;
	      
	      /* This is where it all happens, convert any features contained within object. */
	      convertObj(&conv_info, featDict) ;

	      bsDestroy(obj);
	    }
	}
    }


  /* Now process any paired reads so we can join them up with lines if required. */
  if (arrayMax(all_paired_reads) > 0)
    fmapMakePairedReadSegs(conv_info.segs_array, all_paired_reads, conv_info.homol_info) ;

  arrayDestroy(all_paired_reads) ;
  assDestroy(key2readpair) ;

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
  /* uncomment to see dump of all segs.... */
  segDumpToStdout(segs) ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */


  /* The previously displayed segs contain segs that were calculated
   * (e.g. genefinder), these must be transferred to the new segs. */
  if (oldsegs)
    fmapAddOldSegs(complement, segs_inout, oldsegs, mcache) ;

  arraySort(segs, fmapSegOrder) ;				    /* must sort for FindCoding and */
							    /* RemoveSelfHomol */

  /* Optionally add a dict of mapped features. */				      
  if (conv_info.mapped_dict)
    makeMappedDict(conv_info.mapped_dict, segs) ;


  fmapConvertCoding(&conv_info) ;				    /* make CODING segs */


  fmapConvertAssemblyPath(&conv_info) ;			    /* Make assembly path segs. */


  /* Unify confirmed introns with introns. */
  if (!keep_duplicate_introns)
    fmapRemoveDuplicateIntrons(&conv_info) ;


  /* Remove homols that are referenced by transcripts because they were
   * probably used to build the transcript ! */
  if (!keep_selfhomols)
    fmapRemoveSelfHomol(conv_info.segs_array, conv_info.homol_info) ;  


#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
  /* ACEMBLY only, add later.                                                */
  fMapTraceFindMultiplets (look) ;			    /* make virtual multiplets */
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */
 
  return TRUE ;
}


void sMapFeaturesComplement(int length, Array segs, Array seqinfo, Array homolinfo)
{
  int i, j, tmp, top = length - 1 ;
  SEG *seg ;
  SEQINFO *sinf ;

   /* Transform all the seg coordinates.                                      */
  seg = arrp(segs, 1, SEG) ;
  for (i = 1 ; i < arrayMax(segs) ; ++i, ++seg)
    {
      /* Do the complement.                                                  */
      tmp = seg->x1 ;
      seg->x1 = top - seg->x2 ;
      seg->x2 = top - tmp ;
      if (seg->type >= SEQUENCE && seg->type <= ALLELE_UP)
	{
	  seg->type ^= 1 ;

	  /* There must be a more elegant way to do this but I can't think of it at the moment... */
	  {
	    BOOL top = FALSE, bot = FALSE ;

	    if (seg->flags & SEGFLAG_CLIPPED_TOP)
	      {
		seg->flags &= !SEGFLAG_CLIPPED_TOP ;
		bot = TRUE ;
	      }
	    if (seg->flags & SEGFLAG_CLIPPED_BOTTOM)
	      {
		seg->flags &= !SEGFLAG_CLIPPED_BOTTOM ;
		top = TRUE ;
	      }

	    if (bot)
	      seg->flags |= SEGFLAG_CLIPPED_BOTTOM ;
	    if (top)
	      seg->flags |= SEGFLAG_CLIPPED_TOP ;
	  }
	}

      /* Now do some seg-type specific stuff. */
      if ((seg->type | 0x1) == SPLICED_cDNA_UP)
	seg->source = top - seg->source ;
      
    }

  /* Gaps arrays attached to (some) homols have coordinates in which must be munged. */
  for (i = 0 ; i < arrayMax(homolinfo) ; i++)
    {
      HOMOLINFO *hinf = arrp(homolinfo, i, HOMOLINFO) ;
      if (hinf->gaps)
	for (j = 0; j < arrayMax(hinf->gaps); j++) 
	  {
	    SMapMap *m = arrp(hinf->gaps, j, SMapMap);
	    m->r1 = top - m->r1 + 2;
	    m->r2 = top - m->r2 + 2;
	  }
    }


  /* Some exon segs have CDS information attached, this information is in    */
  /* the seqinfo array and includes coordinates that must be transformed.    */
  for (i = 0 ; i < arrayMax(seqinfo) ; i++)
    {
      sinf = arrp(seqinfo, i, SEQINFO) ;
	  
      if (sinf->flags & SEQ_CDS)
	{
	  tmp = sinf->cds_info.cds_seg.x1 ;
	  sinf->cds_info.cds_seg.x1 = top - sinf->cds_info.cds_seg.x2 ;
	  sinf->cds_info.cds_seg.x2 = top - tmp ;
	  sinf->cds_info.cds_seg.type ^= 1 ;

	  /* There must be a more elegant way to do this but I can't think of it at the moment... */
	  {
	    BOOL top = FALSE, bot = FALSE ;

	    if (sinf->cds_info.cds_seg.flags & SEGFLAG_CLIPPED_TOP)
	      {
		sinf->cds_info.cds_seg.flags &= !SEGFLAG_CLIPPED_TOP ;
		bot = TRUE ;
	      }
	    if (sinf->cds_info.cds_seg.flags & SEGFLAG_CLIPPED_BOTTOM)
	      {
		sinf->cds_info.cds_seg.flags &= !SEGFLAG_CLIPPED_BOTTOM ;
		top = TRUE ;
	      }

	    if (bot)
	      sinf->cds_info.cds_seg.flags |= SEGFLAG_CLIPPED_BOTTOM ;
	    if (top)
	      sinf->cds_info.cds_seg.flags |= SEGFLAG_CLIPPED_TOP ;
	  }

	}
    }

  return ;
}






#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
/* smap based gif get code, doesn't use fex but does use smap and NO fmap    */
/* look. It also                                                             */
/*                                                                           */
/* this is my version of fMapGifGet() which allows for selection of which    */
/* sets of features will be included.                                        */
/*                                                                           */
/* When we come to fex stuff we can easily add an smapfexget call which will */
/* be the same except we'll produce fex features instead of segs.            */
/*                                                                           */
/*                                                                           */
SMapFeatureMap sMapFeaturesCreate(ACEIN command_in, ACEOUT result_out, SMapFeatureMap map_in)
{
  SMapFeatureMap map = NULL ;
  KEY key ;
  int x1, x2 ;
  char *word ;
  STORE_HANDLE handle = NULL ;
  DICT *features_dict = NULL ;
  DICT *methods_dict  = NULL ;
  DICT *sources_dict  = NULL ;
  BOOL dna_get = FALSE ;
  BOOL already_features = FALSE;
  BOOL already_sources  = FALSE;
  BOOL already_methods  = FALSE;
  BOOL include_methods  = FALSE;
  BOOL include_sources  = FALSE;
  BOOL include_features = FALSE;

        
  if (map_in)
    sMapFeaturesDestroy(map_in) ;


  handle = handleCreate() ;

  /* try to get from graph */
  if (!(word = aceInWord(command_in)))
    {
      aceOutPrint(result_out, "// gif smapget error: no sequence specified.\n") ;
      goto usage ;
    }

  /* this test is probably wrong now, we probably want to do non-sequence    */
  /* objects...in fact that is sure to be true.                              */
  if (!lexword2key(word, &key, _VSequence))
    { 
      aceOutPrint (result_out, "// gif smapget error: Sequence %s not known\n", word) ;
      goto usage ;
    }


  already_features = FALSE ;
  x1 = 1 ; x2 = 0 ;					    /* default for whole sequence */
  while (aceInCheck (command_in, "w"))
    { 
      word = aceInWord(command_in) ;
      if (strcmp (word, "-dna") == 0)
	{ 
	  dna_get = TRUE ;
	}
      else if (strcmp (word, "-coords") == 0)
	{ 
	  if (!aceInInt (command_in, &x1) || !aceInInt (command_in, &x2) || (x2 == x1))
	    goto usage ;
	}
      else if (strcmp (word+1, "feature") == 0)
	{
	  if (word[0] == '+')
	    include_features = TRUE;
	  else
	    include_features = FALSE;

	  if (aceInCheck (command_in, "w"))
	    {
	      if (already_features)
		goto usage ;

	      features_dict = dictHandleCreate(16, handle) ;

	      addToSet(command_in, features_dict) ;
	      already_features = TRUE ;
	    }
	}
      else if (strcmp (word+1, "method") == 0)
	{
	  if (word[0] == '+')
	    include_methods = TRUE;
	  else
	    include_methods = FALSE;

	  if (aceInCheck (command_in, "w"))
	    {
	      if (already_methods)
		goto usage ;

	      methods_dict = dictHandleCreate(16, handle) ;

	      addToSet(command_in, methods_dict) ;
	      already_methods = TRUE ;
	    }
	}
      else if (strcmp (word+1, "source") == 0)
	{
	  if (word[0] == '+')
	    include_sources = TRUE;
	  else
	    include_sources = FALSE;

	  if (aceInCheck (command_in, "w"))
	    {
	      if (already_sources)
		goto usage ;

	      sources_dict = dictHandleCreate(16, handle) ;

	      addToSet(command_in, sources_dict) ;
	      already_sources = TRUE ;
	    }
	}
      else
	goto usage ;
    }


#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
  if (x1 < x2 || (x1 == 1 && x2 == 0))
    look = fMapCreateLook(key, x1, x2, FALSE, fmap_data) ;
  else
    look = fMapCreateLook(key, x2, x1, FALSE, fmap_data) ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */
  if (x1 < x2 || (x1 == 1 && x2 == 0))
    map = doFeatureMap(key, x1, x2, dna_get, 
		       methods_dict, sources_dict, features_dict, 
		       include_methods, include_sources, include_features) ;
  else
    map = doFeatureMap(key, x2, x1, dna_get,
		       methods_dict, sources_dict, features_dict, 
		       include_methods, include_sources, include_features) ;


  if (!map)
    { 
      aceOutPrint (result_out, "// gif smapget error: could not make map for %s\n", name(key)) ;
      if (handle)
	handleDestroy (handle) ;
      return NULL ;
    }


#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
  /* Unsure what to do here...fill in later...                               */
  /* Is the zone used in the gff dump ????? */

  /* set the zone */
  if (x1 == 1 && x2 == 0)
    {
      int i ;
      for (i = 1 ; i < arrayMax(look->segs) ; ++i)
	{
	  if (arrp(look->segs,i,SEG)->key == key)
	    {
	      setZone(look, arrp(look->segs,i,SEG)->x1, 
		      arrp(look->segs,i,SEG)->x2 + 1) ;
	      break ;
	    }
	}
    }
  else if (x1 > x2)
    fMapRC (look) ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */


  if (handle)
    handleDestroy (handle) ;

  return map ;

 usage:
  aceOutPrint (result_out, "// gif smapseqget error, usage:\n"
	       "\tsmapget [sequence [-coords x1 x2][-feature feat1|feat2]]\n") ;

  if (handle)
    handleDestroy (handle) ;

  return NULL ;
}
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */



void sMapFeaturesDestroy(SMapFeatureMap map)
{
  if (map)
    {
      handleDestroy(map->handle) ;
      messfree (map) ;
    }

  return;
}



/* This routine makes a keyset containing keys of method objects in one of
 * two ways:
 *          If include_methods = TRUE then we start with an empty keyset and add all the
 * keys of methods found in the database whose names match one of the names
 * in the "features" dict parameter.
 *          else we subtract from the set of all method keys, the keys of
 * those methods matching the names in the "features" dict.
 *
 * i.e. this routine can be used to include or exclude methods.
 *
 * Returns a valid keyset of method objects or NULL if no methods match the
 * supplied "feature" list. It is the callers responsibility to destroy the
 * keyset when finished with.
 */
KEYSET sMapFeaturesSet(STORE_HANDLE handle, 
		       DICT methods,
		       BOOL include_methods)
{
  KEYSET result = NULL ;
  KEYSET wild = NULL ;
  int i,j ;
  char *KSMethod;		/* KeySet method       */
  char CLMethod[255+1];		/* Command Line method */
  BOOL ThisMethodFound = FALSE;
  BOOL AnyMethodNotFound = FALSE;

  if (include_methods)
    result = keySetHandleCreate(handle) ;
  else
    result = allMethods() ;

  wild = allMethods() ;


  /* to be honest this routine is not written very well, it doesn't need to do all this
   * conversion to text of dict names etc., not so good.... */

  if (result && methods)            /* only if some methods have been specified */
    {
      Stack methods_not_found = NULL ;

      methods_not_found = stackCreate(1000) ;
      stackTextOnly(methods_not_found) ;
      pushText(methods_not_found, " ") ;

      for (i = 0 ; i < dictMax(methods) ; i++)
	{
	  KEY kp = KEY_UNDEFINED ;
	  ThisMethodFound = FALSE;

	  /* wildcard */
	  /* If the command line method includes a trailing asterisk, 
	   * then strip that off and search the entire keyset for methods 
	   * that start with that pattern.
	   */

	  strcpy(CLMethod, dictName(methods,i));

	  if (CLMethod[strlen(CLMethod)-1] == '*')
	    {
	      CLMethod[strlen(CLMethod)-1] = '\0';

	      for (j = 0; j<arrayMax(wild); j++)
	        { 
		  KSMethod = name(keySet(wild, j)); /* extract method from keyset */

	          if (strncasecmp(KSMethod, CLMethod, strlen(CLMethod)-1) == 0)
	            {
		      ThisMethodFound = TRUE;
	              lexword2key(KSMethod, &kp, _VMethod);
   		        
	              if (include_methods)
		        keySetInsert(result, kp) ;
	              else
		        keySetRemove(result, kp) ;
	            }	
	        }
	      if (!ThisMethodFound)
  	        {
		  catText(methods_not_found, CLMethod) ;
		  catText(methods_not_found, " ") ;

		  AnyMethodNotFound = TRUE;
  	        }
	    }
	  else					/* no wildcard */
  	    {

	      /* I think I need to check the col group stuff here otherwise the objects
	       * actual method will not be included.... */

	      /* I've added the lexaddkey call to test my column groups stuff... */
  	      if (lexword2key(CLMethod, &kp, _VMethod)
		  || lexaddkey(CLMethod, &kp, _VMethod))
  	        {
  	          if (include_methods)
  		    keySetInsert(result, kp) ;
  	          else
  		    keySetRemove(result, kp) ;
  	        }
  	      else
  	        {
		  catText(methods_not_found, CLMethod) ;
		  catText(methods_not_found, " ") ;

		  AnyMethodNotFound = TRUE;
  	        }
  	    }
	}

      if (AnyMethodNotFound)
	{
	  messerror("Can't find method(s): %s\n", popText(methods_not_found)) ;
  	}

      if (keySetMax(result) == 0)
	{
	  keySetDestroy(result) ;
	  result = NULL ;
	}

      stackDestroy(methods_not_found) ;
    }

  return result ;
}




/*
 * ----------------    Internal routines   ---------------------------------
 */


/* This routine sets the coords and type in a seg from the smap info and coords.
 *
 * Note that co-ords in segs are ZERO-based, with the first base of the
 * sequence in the MASTER seg being zero. The results of sMap are
 * ONE-based. Whenever we call sMap, we need to reduce the results by
 * one to convert to seg coords. Hmph.
 * Also note that the coords that come out of smap will be reversed for
 * features that are flipped, this means that they must be flipped back to
 * go into the seg. In segs _all_ coords are held the same way round:
 * smallest first, it is the "type" field (e.g. SEQUENCE, SEQUENCE_UP) that
 * determines the orientation.
 */
void fmapSMap2Seg(SMapKeyInfo *info, SEG *seg, int y1, int y2, SegType seg_type)
{
  if (y1 < y2)
    {
      seg->x1 = (y1 - 1), seg->x2 = (y2 - 1), seg->type = SEG_DOWN(seg_type) ;
    }
  else if (y1 > y2)
    {
      seg->x1 = (y2 - 1), seg->x2 = (y1 - 1), seg->type = SEG_UP(seg_type) ;
    }
  else
    {
      seg->x1 = (y1 - 1), seg->x2 = (y2 - 1),
	seg->type = ((sMapIsReverse(info, y1, y2)) ? SEG_UP(seg_type) : SEG_DOWN(seg_type)) ;
    }

  return ;
}





#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
static consMapDNAErrorReturn callback(KEY key, int position)
{
  BOOL OK;

  if (!reportErrors_G)
    return sMapErrorReturnContinue;

  OK = messQuery("Mismatch error at %d in %s.\n"
		 "Do you wish to see further errors?", 
		 position, name(key));
  
  return OK ? sMapErrorReturnContinue : sMapErrorReturnSilent;
}
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */




#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
/*
  if (x1 < x2 || (x1 == 1 && x2 == 0))
  look = fMapCreateLook(key, x1, x2, FALSE, fmap_data) ;
  else
  look = fMapCreateLook(key, x2, x1, FALSE, fmap_data) ;
*/
static SMapFeatureMap doFeatureMap(KEY seq, int start, int stop, BOOL get_dna, 
				   DICT *methods_dict, 
				   DICT *sources_dict,
				   DICT *features_dict,
				   BOOL include_methods,
				   BOOL include_sources,
				   BOOL include_features)
{
  SMapFeatureMap map = NULL ;
  BOOL status = TRUE ;
  BOOL make_map ;

  map = (SMapFeatureMap)messalloc(sizeof(SMapFeatureMapRec)) ;
  map->handle = handleCreate() ;
  map->seqOrig = seq ;

  if (sMapTreeRoot(seq, start, stop, &seq, &start, &stop))
    map->seqKey = seq ;
  else
    status = FALSE ;

  if (status)
    {
      map->smap = sMapCreate(map->handle, map->seqKey, start, stop, NULL);
    }

  if (status)
    {
      if (get_dna)
	{
#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
	  reportErrors_G = look->reportDNAMismatches;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

	  if (!(map->dna = sMapDNA(map->smap, map->handle, callback)))
	    status = FALSE ;
	}
    }

  if (status)
    {
      if (methods_dict)
	map->method_set = sMapFeaturesSet(map->handle,
					   methods_dict, 
					   include_methods) ;
      if (map->method_set == NULL)
	status = FALSE;

      map->include_methods = include_methods;

    }
  if (status)
    {
      if (sources_dict)
	map->sources_dict = sources_dict;

      map->include_sources = include_sources;

      map->length = sMapMax(map->smap) ;
      map->zoneMin = 0 ;
      map->zoneMax = stop - start + 1 ;
      map->start = start - 1 ;
      map->stop = stop - 1 ;

      /* just for testing, don't think this is needed for gif etc. style stuff.  */
      map->fullLength = sMapLength (seq) ;

      map->featDict = dictHandleCreate (1000, map->handle) ;
      dictAdd (map->featDict, "null", 0) ;			    /* so real entries are non-zero */

      map->mcache = methodCacheCreate(map->handle) ;
      map->segs = arrayHandleCreate(256, SEG, map->handle) ;
      map->homolInfo = arrayHandleCreate(256, HOMOLINFO, map->handle) ;
      map->homolBury = NULL ;
      map->homolFromTable = NULL ;
      map->seqInfo = arrayHandleCreate(32, SEQINFO, map->handle) ;

      map->sources_dict = sources_dict;
      map->include_sources = include_sources;
    }

  /* Produce the segs array.                                                 */
  if (status)
    {
      if (!(make_map = sMapFeaturesMake(map->smap, map->seqKey, map->seqOrig, map->start, map->stop, map->length,
					NULL, FALSE,
					map->mcache, NULL,
					map->dna, map->method_set, include_methods, map->featDict,
					sources_dict, include_sources,
					map->segs,
					map->seqInfo, map->homolInfo, map->feature_info,
					map->homolBury, map->homolFromTable,

					map->handle)))
	status = FALSE ;
    }

  if (!status)
    {
      sMapFeaturesDestroy(map) ;
      map = NULL ;
    }

  return map ;
}
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */



/* Actually this routine will need to be externally callable as I will want  */
/* to add features to an fmap on the fly....youch....                        */
/*                                                                           */
/* You should be aware when altering this code that some of the calls are    */
/* _order_ dependant because some routines need information from preceding   */
/* calls to decide what to do, e.g. the exon and cds routines.               */
/*                                                                           */
static void convertObj(SmapConvInfo *conv_info, DICT *featDict)
{
  static KEY _Feature = KEY_UNDEFINED, _Transcribed_gene, _Clone_left_end, _Clone_right_end,
    _Confirmed_intron, _EMBL_feature, _Splices, _Method ;
  int cds_min = 0 ;
  BOOL exons_found = FALSE ;
  BOOL AllRequired = FALSE;
  BOOL parent_obj_created = FALSE ;
  OBJ obj ;

  if (_Feature == KEY_UNDEFINED)
    {
      _Feature = str2tag("Feature") ;
      _Transcribed_gene = str2tag("Transcribed_gene") ;	    /* Appears to be unused..... */
      _Clone_left_end = str2tag("Clone_left_end") ;
      _Clone_right_end = str2tag("Clone_right_end") ;
      _Confirmed_intron = str2tag("Confirmed_intron") ;
      _EMBL_feature = str2tag("EMBL_feature") ;
      _Splices = str2tag("Splices") ;
      _Method = str2tag("Method");
    }

  obj = conv_info->obj ;

  /* If the parent object has a required method, then any child objects which
   * lack methods are also included.  If no methods specified, we want everything. */
  if (fmapIsRequired(conv_info)
      && (conv_info->key != conv_info->seq_orig  /* not for master seg */
	  || !conv_info->method_set
	  || !conv_info->include_methods))
      AllRequired = TRUE;


  /* Currently the sequence class has some "special case" processing that means that we
   * may need to create a sort of "virtual" sequence seg. NOTE also that this must be
   * done before the Source_exons and cds and start_not_found. */
  if (conv_info->is_Sequence_class)
    parent_obj_created = convertSequence(conv_info) ;
  else
    parent_obj_created = convertFeatureObj(conv_info) ;

  /* Only do this if the parent object was created. */
  if (parent_obj_created && AllRequired)
    {
      if (bsFindTag(obj, str2tag("Source_Exons")))
	convertExons(conv_info, &exons_found) ;
  
      if (bsFindTag(obj, str2tag("CDS")))
	convertCDS(conv_info, &cds_min, exons_found);

      if (bsFindTag(obj, str2tag("Start_not_found")))
	convertStart(conv_info, cds_min) ;

      if (bsFindTag(obj, str2tag("End_not_found")))
	convertEnd(conv_info) ;
    }


  if (bsFindTag(obj, _Homol))     /* homols need to look at methods at a deeper level */
    convertHomol(conv_info) ;


  if (bsFindTag(obj, _Feature))
    convertFeature(conv_info,featDict) ;

  if (AllRequired)
    {
      if (bsFindTag(obj, _Visible))
	convertVisible(conv_info) ;
  
      if (bsFindTag(obj, _Assembly_tags))
	convertAssemblyTags(conv_info) ;
  
      if (bsFindTag(obj, _Allele))
	convertAllele(conv_info) ;

      if (bsFindTag(obj, _Clone_left_end))
	convertCloneEnd(conv_info, _Clone_left_end) ;

      if (bsFindTag(obj, _Clone_right_end))
	convertCloneEnd(conv_info, _Clone_right_end) ;

      if (bsFindTag(obj, _Oligo))
	convertOligo(conv_info) ;
    }

  if (bsFindTag (obj, _Confirmed_intron))
    convertConfirmedIntrons(conv_info) ;

  if (AllRequired)
    {
      if (bsFindTag(obj, _EMBL_feature))
	convertEMBLFeature(conv_info) ;
    }

  if (bsFindTag(obj, _Splices))
    convertSplices(conv_info) ;


  return ;
}


/* Makes segs for the sequence class object itself. This is not straight forward because
 * sometimes the sequence object is a feature itself (e.g. a transcript), sometimes it is
 * not a feature itself but is a container for other features (e.g. homols). This function
 * handles the former where the sequence object is a feature itself. */
static BOOL convertSequence(SmapConvInfo *conv_info)
{
  BOOL result = FALSE ;
  KEY method_key = str2tag("Method");
  SEQINFO *sinf ;

  if (fmapIsRequired(conv_info))
    {
      SEG *seg ;

      /* Set up a seg for this object, record it's index because we need it for it's children. */
      conv_info->parent_seg_index = arrayMax(conv_info->segs_array);
      seg = arrayp(conv_info->segs_array, conv_info->parent_seg_index, SEG);
      seg->source = sMapParent(conv_info->info);
      seg->key = seg->parent = conv_info->key;
      seg->flags = SEGFLAG_UNSET ;

      fmapSMap2Seg(conv_info->info, seg, conv_info->y1, conv_info->y2, SEQUENCE) ;

      fmapSegSetClip(seg, conv_info->status, SEQUENCE, SEQUENCE_UP) ;



      /* Set up method, gaps etc. for this feature.
       * If the feature is gappy, hold a pointer to the Map array in the sMap */
      conv_info->parent_seqinfo_index = arrayMax(conv_info->seqinfo) ;
      sinf = arrayp(conv_info->seqinfo, conv_info->parent_seqinfo_index, SEQINFO) ;
      seg->data.i = conv_info->parent_seqinfo_index ;
      
      if (bsGetKey (conv_info->obj, method_key, &sinf->method))
	{
	  if (bsGetData (conv_info->obj, _bsRight, _Float, &sinf->score))
	    sinf->flags |= SEQ_SCORE ;
	}
      else
	{
	  fmapSetDefaultMethod(conv_info->obj, sinf) ;
	}

      /* MAY NEED TO MOVE THESE INTO MY ROUTINE THAT ALLOCATES SEGS..... */
      /* check when this is executed and if its ever used ?.......... */
      if (conv_info->status & SMAP_STATUS_INTERNAL_GAPS)
	sinf->gaps = arrayHandleCopy(sMapMapArray(conv_info->info), conv_info->segs_handle);
      
      if (bsFindTag(conv_info->obj, str2tag("Assembled_from")))	/* Not known in databases ? */
	sinf->flags |=  SEQ_VIRTUAL_ERRORS ;
	  
      if (bsFindTag(conv_info->obj, str2tag("Genomic_canonical")) ||
	  bsFindTag(conv_info->obj, str2tag("Genomic")))
	sinf->flags |=  SEQ_CANONICAL ;

      result = TRUE ;
    }


  return result ;
}


/* This routine creates segs for objects that are themselves features as opposed to the features 
 * specified by the magic tag "Feature" within an object. */
static BOOL convertFeatureObj(SmapConvInfo *conv_info)
{
  BOOL result = FALSE ;
  KEY method_tag = str2tag("Method");
  KEY method_key = KEY_UNDEFINED ;


  /* Note, no method key, means no processing.... */
  if (bIndexTag(conv_info->key, method_tag)
      && bsGetKey(conv_info->obj, method_tag, &method_key)
      && !fmapFilterOut(conv_info, method_key))
    {
      SEG *seg ;
      SegType seg_type ;
      int feature_index ;
      BOOL exons = FALSE, cds = FALSE ;

      /* This may well cause a storm but we'll see.....the worm people don't want CDS without
       * Source_exon's */
      exons = bsFindTag(conv_info->obj, str2tag("Source_Exons")) ;
      cds = bsFindTag(conv_info->obj, str2tag("CDS")) ;
      if (cds && !exons)
	{
	  messerror("Object %s has the CDS tag set but no Source_Exons defined.",
		    nameWithClassDecorate(conv_info->key)) ;
	}


      if (exons || cds)
	{
	  seg_type = SEQUENCE ;
	}
      else
	{
	  feature_index = arrayMax(conv_info->feature_info) ;
	  seg_type = FEATURE_OBJ ;
	}

      /* Set up a seg for this object, record it's index for use by child segs. */
      conv_info->parent_seg_index = arrayMax(conv_info->segs_array) ;
      seg = arrayp(conv_info->segs_array, conv_info->parent_seg_index, SEG) ;
      seg->source = sMapParent(conv_info->info) ;
      seg->key = seg->parent = conv_info->key ;
      seg->data_type = SEG_DATA_UNSET ;

      fmapSMap2Seg(conv_info->info, seg, conv_info->y1, conv_info->y2, seg_type) ;
      fmapSegSetClip(seg, conv_info->status, seg_type, SEG_UP(seg_type)) ;


      /* For exon/intron like objects, set up method, gaps etc. for this feature.
       * If the feature is gappy, hold a pointer to the Map array in the sMap */
      if (seg_type == SEQUENCE)
	{
	  SEQINFO *sinf ;

	  /* Set these up for features that are specified within this sequence object, they will
	   * need these indexes to get information from the sequence segs/seqinfo. */
	  conv_info->parent_seqinfo_index = arrayMax(conv_info->seqinfo) ;
	  sinf = arrayp(conv_info->seqinfo, conv_info->parent_seqinfo_index, SEQINFO) ;
	  seg->data.i = conv_info->parent_seqinfo_index ;

	  sinf->method = method_key ;
	  if (bsGetData (conv_info->obj, _bsRight, _Float, &sinf->score))
	    sinf->flags |= SEQ_SCORE ;

	  /* MAY NEED TO MOVE THESE INTO MY ROUTINE THAT ALLOCATES SEGS..... */
	  /* check when this is executed and if its ever used ?.......... */
	  if (conv_info->status & SMAP_STATUS_INTERNAL_GAPS)
	    sinf->gaps = arrayHandleCopy(sMapMapArray(conv_info->info), conv_info->segs_handle);


#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
	  printf("convertFeatureObj(), obj: %s at %d %d, parent: %s, method: %s\n",
		 nameWithClassDecorate(seg->key), conv_info->y1, conv_info->y2,
		 nameWithClassDecorate(seg->source),
		 sinf->method ? name(sinf->method) : "<unset>") ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */


	}
      else
	{
	  FeatureInfo *feat ;

	  seg->data.i = feature_index ;

	  feat = arrayp(conv_info->feature_info, feature_index, FeatureInfo) ;
	  feat->method = method_key ;
	}

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
      printf("convertFeatureObj(), obj: %s at %d %d, parent: %s\n",
	     nameWithClassDecorate(seg->key), conv_info->y1, conv_info->y2,
	     nameWithClassDecorate(seg->source)) ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

      result = TRUE ;
    }

  return result ;
}



/* There is a big loop in this routine which is because the underlying database
 * model cannot simply be bsFlatten'd:
 *   
 * Homol <tag2> <obj> <method> Float Int UNIQUE Int Int UNIQUE Int #Homol_info
 *   
 * where:
 * 
 * Homol homol_type query_seq method Score target_start target_end query_start query_end
 * 
 * and :
 *
 * #Homol_info Align Int UNIQUE Int UNIQUE Int 
 * // not needed if full alignment is ungapped
 * // one row per ungapped alignment section
 * // first int is position in query, second in target
 * // third int is length - only necessary when gaps in 
 * // both sequences align.
 * 
 * mark1 cursors down <obj>
 * mark2 cursors down <method>
 * mark3 cursors down Float
 * mark4 cursors down Int
 * mark5 cursors down Int
 * 
 * There are extra complications in that we record each set of homols processed and
 * add information to be used to join the homols up with lines and to join EST read pairs.
 * 
 */
static void convertHomol(SmapConvInfo *conv_info)
{
  static KEY Clone = KEY_UNDEFINED, Show_in_reverse_orientation, Paired_read, EST_5, EST_3  ;
  KEY key = conv_info->key ;
  SMapKeyInfo *info = conv_info->info ;
  OBJ obj = conv_info->obj ;
  BSMARK mark1 = 0, mark2 = 0, mark3 = 0, mark4 = 0, mark5 = 0, mark6 = 0 ;
  KEY method, homol, tag2 ;
  float score;
  int target1, target2, query1, query2;
  BOOL show_in_reverse = FALSE ;
  int homol_index = 0 ;
  BOOL homol_mapped = FALSE, gaps_mapped = FALSE ;
  BOOL paired_read = FALSE ;
  KEY paired_key = KEY_UNDEFINED, cluster_key = KEY_UNDEFINED ;



  if (Clone == KEY_UNDEFINED)
    {
      Clone = str2tag("Clone") ;
      Show_in_reverse_orientation = str2tag("Show_in_reverse_orientation") ;
      Paired_read = str2tag("Paired_read") ;
      EST_5 = str2tag("EST_5") ;
      EST_3 = str2tag("EST_3") ;
    }


  if (bsGetKeyTags(obj, _Homol, &tag2)) do 
    {
      mark1 = bsMark(obj, mark1) ;

      if (bsGetKey(obj, _bsRight, &homol)) do
	{ 
	  mark2 = bsMark(obj, mark2) ;
	  
	  /* Tags used for EST read pair processing. */
	  show_in_reverse = bIndexTag(homol, Show_in_reverse_orientation) ;
	  paired_read = bIndexGetKey(homol, Paired_read, &paired_key) ;

	  if (paired_read)
	    {
	      /* For paired reads we store the 5 prime key as the one to be used for cluster
	       * display in fmap, we must be able to find either 5 or 3 prime key otherwise
	       * we don't do the paired read bit. */
	      if (bIndexTag(homol, EST_5))
		cluster_key = homol ;
	      else if (bIndexTag(homol, EST_3))
		cluster_key = paired_key ;
	      else
		paired_read = FALSE ;
	    }

	  if (bsGetKey(obj, _bsRight, &method)) do
	    {
	      METHOD *meth ;

	      /* Get the method for this align. */

	      /* skip this one if filtering based on method and the method doesn't match. */
	      if (fmapFilterOut(conv_info, method))
		continue;

	      meth = methodCacheGet(conv_info->method_cache, method, "") ;
	      messAssert(meth) ;

	      mark3 = bsMark(obj, mark3);

	      if (bsGetData(obj, _bsRight, _Float, &score)) do
		{
		  mark4 = bsMark(obj, mark4);
		  if (bsGetData(obj, _bsRight, _Int, &target1)) do
		    { 
		      mark5 = bsMark(obj, mark5);
		      if (bsGetData(obj, _bsRight, _Int, &target2) &&
			  bsGetData(obj, _bsRight, _Int, &query1)) do
			    { 
			      mark6 = bsMark(obj, mark6);

			      if (bsGetData(obj, _bsRight, _Int, &query2))
				{
				  int y1 = 0, y2 = 0, clip_target1 = 0, clip_target2 = 0 ;
				  SMapStatus status ;
				  SMapStrand ref_strand = STRAND_INVALID, match_strand = STRAND_INVALID ;

				  /* Map the alignment sequence coords into reference sequence coord
				   * space, we only go on to process them if some part of the alignment is
				   * within the mapped space. */
				  status = sMapMap(info, target1, target2, &y1, &y2, &clip_target1, &clip_target2) ;
				  if (SMAP_STATUS_INAREA(status))
				    {
				      HOMOLINFO *hinf ;
				      SEG *seg ;
				      SMapAlignMapStruct align_map = {0} ;
				      gboolean allow_misalign, map_gaps, export_string ;


				      bsPushObj(obj);

				      allow_misalign = meth->flags & METHOD_ALLOW_MISALIGN ;
				      map_gaps = meth->flags & METHOD_MAP_GAPS ;
				      export_string = meth->flags & METHOD_EXPORT_STRING ;

				      /* this is no good, we need to sort the model...   */
				      if (!map_gaps)
					allow_misalign = TRUE ;

				      homol_mapped = FALSE ;
				      gaps_mapped = FALSE ;


				      /* Fetch the align string for later exporting if
				       * requested, no error if just no data. */
				      if (export_string)
					{
					  status = sMapFetchAlignStr(obj, &align_map) ;

					  if (status == SMAP_STATUS_NO_DATA) /* no data is not an error. */
					    status = SMAP_STATUS_PERFECT_MAP ;
					}

				      if (status == SMAP_STATUS_PERFECT_MAP)
					{
					  /* Map/clip coords and gaps if requested. */
					  status = sMapMapAlign(conv_info->segs_handle,
								obj, homol,
								allow_misalign,
								map_gaps,
								y1, y2,
								target1, target2, &clip_target1, &clip_target2,
								query1, query2, &query1, &query2,
								&align_map) ;

					  /* If the mapping worked set up the seg. */
					  if (status == SMAP_STATUS_PERFECT_MAP
					      || ((meth->flags & METHOD_ALLOW_CLIPPING) && SMAP_STATUS_ISCLIP(status)))
					    {
					      /* Align block is mapped so create and fill seg. */
					      ref_strand = align_map.ref_strand ;
					      match_strand = align_map.match_strand ;

					      homol_mapped = TRUE ;
					      homol_index = arrayMax(conv_info->segs_array) ;

					      seg = arrayp(conv_info->segs_array, homol_index, SEG);

					      seg->source = key ;
					      seg->key = seg->parent = homol ; /* query */
					      seg->subfeature_tag = tag2 ;

					      seg->strand = ref_strand ;

					      seg->data.i = arrayMax(conv_info->homol_info) ;
				      
					      hinf = arrayp(conv_info->homol_info, seg->data.i, HOMOLINFO) ;
					      hinf->method = method;
					      hinf->score = score;
					      hinf->align_id = align_map.align_id ;
					      hinf->cluster_key = KEY_UNDEFINED ;
					      hinf->gaps = NULL ;

					      if (paired_read)
						{
						  hinf->cluster_key = cluster_key ;
						  seg->flags |= SEGFLAG_TWO_PARENT_CLUSTER ;
						}

					      if (align_map.gap_map)
						{
						  gaps_mapped = TRUE ;
						  hinf->gaps = align_map.gap_map ;
						}

					      if (meth->flags & METHOD_EXPORT_STRING)
						{
						  hinf->match_type = align_map.align_type ;
						  hinf->match_str = align_map.align_str ;
						}


#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
					      if (!(match_strand == STRAND_FORWARD || match_strand == STRAND_REVERSE)
						  || (match_strand == STRAND_FORWARD && query1 > query2)
						  || (match_strand == STRAND_REVERSE && query1 < query2))
						printf("found bad one\n") ;

#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

					      /* Set up homol coords/strand. */
					      if (match_strand == STRAND_FORWARD || query1 < query2)
						hinf->strand = FMAPSTRAND_FORWARD ;
					      else
						hinf->strand = FMAPSTRAND_REVERSE ;

					      hinf->x1 = query1 ;
					      hinf->x2 = query2 ;

					      fmapSMap2Seg(conv_info->info, seg, y1, y2, HOMOL);


					      /* Some features (e.g. for EST read pairs) need to be shown
						 on the opposite strand so flip coords and flag it. */
					      if (show_in_reverse)
						{
						  fmapSegFlipHomolStrand(seg, hinf) ;

						  seg->flags |= SEGFLAG_FLIPPED ;
						}
					    }
					}
				      else
					{
					  char *homol_str ;

					  /* Report problems. */
					  homol_str = fmapMakeHomolErrString(tag2, method, homol,
									 target1, target2,
									 query1, query2) ;

					  if (status == SMAP_STATUS_MISALIGN || status == SMAP_STATUS_BADARGS)
					    messerror("%s Error in data, check that Reference -> Match coords"
						      " map onto each other, gaps data etc.",
						      homol_str) ;
					  else if (SMAP_STATUS_ISCLIP(status))
					    messerror("%s Alignment was clipped by mapping but method"
						      " does not have Allow_clipping tag set.",
						      homol_str) ;
					  else
					    fmapDoAlignErrMessage(homol_str, status, conv_info->key) ;

					  g_free(homol_str) ;
					}
				    }
				}
			      
			      bsGoto(obj, mark6);
			    } while (bsGetData(obj, _bsDown, _Int, &query1));

		      bsGoto(obj, mark5);
		    } while (bsGetData(obj, _bsDown, _Int, &target1));

		  bsGoto(obj, mark4);
		} while (bsGetData(obj, _bsDown, _Float, &score));

	      if (homol_mapped && (meth->flags & METHOD_JOIN_BLOCKS))
		{
		  if (paired_read)
		    addReadPair(conv_info->all_paired_reads, conv_info->key2readpair,
				homol, paired_key, conv_info->segs_array, homol_index) ;
		}

	      bsGoto(obj, mark3);
	    } while (bsGetKey(obj, _bsDown, &method));

	  bsGoto(obj, mark2);
	} while (bsGetKey(obj, _bsDown, &homol));

      bsGoto(obj, mark1);
    } while (bsGetKeyTags(obj, _bsDown, &tag2));


  /* Clear up. */
  bsMarkFree(mark1); bsMarkFree(mark2); bsMarkFree(mark3);
  bsMarkFree(mark4); bsMarkFree(mark5); bsMarkFree(mark6);


#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
  /* At the end of this there are two bitExtend calls which were part of the match
   * table stuff, without these homologies get excluded by the draw code. Simon
   * says this is all redundant...but need to check. */

  /* This was completely missing....aggggghhhhh.......and causes serious problems....
   * because homologies come and go like nobodies business.... */

  /* THIS COULD BE A SEPARATE ROUTINE AND PERHAPS I'LL SPLIT IT OFF ONCE ITS WORKING... */

  /* MatchTable - more homologies */

  { 
    Array ma ;
    MATCH *m ;
    HOMOLINFO *hinf ;
    KEY match_key ;

    if (bsGetKey(obj, str2tag("Match_table"), &match_key)
	&& (ma = arrayGet (match_key, MATCH, matchFormat)))
      {
	int i ;
	FLAGSET fBury = flag ("Match", "Bury") ;

	for (i = 0 ; i < arrayMax(ma) ; ++i)
	  {
	    m = arrp (ma, i, MATCH) ;
	    if (class(m->meth) != _VMethod || !m->q1 || !m->q2)
	      continue ;

	    seg = arrayp(conv_info->segs_array, arrayMax(conv_info->segs_array), SEG) ;
	    seg->source = match_key ;			    /* the match table key */
	    seg->key = seg->parent = m->key ;
	    seg->data.i = arrayMax(conv_info->homol_info) ;
	    
	    hinf = arrayp(conv_info->homol_info, seg->data.i, HOMOLINFO) ;
	    hinf->method = m->meth ;
	    hinf->score = m->score ; 
	    hinf->gaps = 0;
	    
	    bitSet (conv_info->homolFromTable, seg->data.i) ;

	    if (m->t1 > m->t2)
	      {
		pos1 = m->q2 ; pos2 = m->q1 ;
		hinf->x1 = m->t2 ; hinf->x2 = m->t1 ;
	      }
	    else
	      {
		pos1 = m->q1 ; pos2 = m->q2 ;
		hinf->x1 = m->t1 ; hinf->x2 = m->t2 ;
	      }
	    
	    POS_PROCESS(HOMOL,HOMOL_UP) ; 
	    
	    if (m->flags & fBury)
	      bitSet (conv_info->homolBury, seg->data.i) ;
	  }
	arrayDestroy (ma) ;
      }
  }
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

  /* necessary to ensure array long enough */
  bitExtend (conv_info->homolBury, arrayMax(conv_info->homol_info)) ;
  bitExtend (conv_info->homolFromTable, arrayMax(conv_info->homol_info)) ;
  
 
  return ;
}








/* Notes:
 * 
 *   exons can legitmately be 1 base long because they have been clipped during
 *   mapping, in this case SMAP2SEG may give seg the wrong type so we get type
 *   from the sequence which holds the exon information which will have the correct type.
 *   The same is done for introns.
 * 
 */
static void convertExons(SmapConvInfo *conv_info, BOOL *exons_found)
{
  int pos1, pos2 ;
  int i;
  SEG *seg;
  int y1, y2;
  BOOL have_exon = FALSE;
  OBJ obj = conv_info->obj ;
  Array units = conv_info->units ;
  static KEY source_intron = KEY_NULL, source_exon ;

  /* Introns are implicitly represented by Source_exons so let's add an implicitly created tag. */
  if (!source_intron)
    {
      source_intron = str2tag(ACE2SO_SUBPART_INTRON) ;
      source_exon = str2tag(ACE2SO_SUBPART_EXON) ;
    }

  if (bsFlatten (obj, 2, units))
    {
      KEY key =  conv_info->key ;
      int seg_index, sinf_index ;
      SEG *parent_seg ;
      SEQINFO *sinf ;
      BOOL exon_no_overlap = FALSE ;
      int bases ;


      seg_index = conv_info->parent_seg_index ;

      sinf_index = conv_info->parent_seqinfo_index ;
      sinf = arrayp(conv_info->seqinfo, sinf_index, SEQINFO) ;

      dnaExonsSort (units) ;

      /* Correct coords for when first exon doesn't start at 1, since sMap will have truncated 
	 to the boundaries of the exons. Cannot do anything about premature end of last exon 
	 at the moment */
      if (arr(units, 0, BSunit).i > 1)
	{
	  parent_seg = arrp(conv_info->segs_array, seg_index, SEG) ;

	  if (parent_seg->type == SEQUENCE)
	    parent_seg->x1 -= arr(units, 0, BSunit).i - 1;
	  else if (parent_seg->type == SEQUENCE_UP)
	    parent_seg->x2 += arr(units, 0, BSunit).i - 1;
	}


      bases = 0 ;
      for (pos1 = 1, i = 0 ; i < arrayMax(units) ; i += 2)
	{
	  SMapStatus status ;
	  
	  pos2 = arr(units, i, BSunit).i ;
	  if ((have_exon && (pos2 > (pos1+1))) || exon_no_overlap == TRUE) /* Exons may abut */
	    {
	      status = sMapUnsplicedMap(conv_info->info, pos1, pos2, &y1, &y2, NULL, NULL) ;
	      if (SMAP_STATUS_OVERLAP(status))
		{
		  seg = arrayp (conv_info->segs_array, arrayMax(conv_info->segs_array), SEG);
		  parent_seg = arrp(conv_info->segs_array, seg_index, SEG) ; /* Redo in case of relocation. */

		  seg->key = 0 ;
		  seg->parent = seg->source = key ;
		  seg->subfeature_tag = source_intron ;
		  seg->parent_type = parent_seg->type ;
		  seg->data.i = sinf_index ;
		  fmapSMap2Seg(conv_info->info, seg, y1, y2, INTRON) ;
		  fmapSegSetClip(seg, status, INTRON, INTRON_UP) ;

		  /* Adjust intron coords so they lie _between_ exon coords. Note that
		   * coords can be clipped down to just one or two pixels by SMap mapping so
		   * must be careful in adjusting coords. */
		  if (seg->x2 - seg->x1 < 2)
		    {
		      seg->x2 = seg->x1 ;
		    }
		  else
		    {
		      seg->x1++ ;
		      seg->x2-- ;
		    }

		  if (fMapDebug)
		    printf("%s\t%c\tintron:\t%d --> %d\n",
			   name(seg->parent), (SEG_IS_DOWN(seg) ? '+' : '-'),
			   seg->x1, seg->x2) ;
		}
	    }

	  pos1 = pos2 ;
	  pos2 = arr(units, i+1, BSunit).i ;
	  
	  if (pos2)
	    {
	      status = sMapUnsplicedMap(conv_info->info, pos1, pos2, &y1, &y2, NULL, NULL) ;
	      if (SMAP_STATUS_OVERLAP(status))
		{
		  int len, start, end, cds_phase, exon_phase ;

		  seg = arrayp (conv_info->segs_array, arrayMax(conv_info->segs_array), SEG);
		  parent_seg = arrp(conv_info->segs_array, seg_index, SEG) ; /* Redo in case of relocation. */

		  seg->key = 0 ;
		  seg->parent = seg->source = key ;
		  seg->subfeature_tag = source_exon ;
		  seg->parent_type = parent_seg->type ;
		  seg->data.i = sinf_index ;
		  seg->flags = SEGFLAG_UNSET ;
		  fmapSMap2Seg(conv_info->info, seg, y1, y2, EXON) ;
		  fmapSegSetClip(seg, status, EXON, EXON_UP) ;

		  have_exon = TRUE ;
		  exon_no_overlap = FALSE ;

		  /* Move along the CDS giving the phase for each exon, remember that the phase
		   * we want is the phase given by the split of the codon between exons. */
		  len = (pos2 - pos1 + 1) ;
		  start = bases + 1 ;
		  end = bases + len ;
		  bases = end ;
		  cds_phase = (start - 1) % 3 ;
		  exon_phase = (3 - cds_phase) % 3 ;

		  seg->feature_data.exon_phase = exon_phase ;

		  if (fMapDebug)		  
		    {
		      printf("%s\t%c\texon\t%d, %d\t(%d)\t%d, %d --> %d, %d\n",
			     nameWithClassDecorate(seg->parent), (SEG_IS_DOWN(seg) ? '+' : '-'),
			     start, end, exon_phase,
			     pos1, pos2, seg->x1, seg->x2) ;
		    }
		}
	      else
		{
		  exon_no_overlap = TRUE ;
		}
	    }
	  else
	    messerror ("Missing pos2 for exon in %s", nameWithClassDecorate(key)) ;
	  
	  pos1 = pos2 ;
	}

      sinf->flags |= SEQ_EXONS ;
    }

  *exons_found = have_exon;
 

  return ;
}


/* OK, pay attention, when the CDS gets drawn (fMapShowSequence), the
 * CDS seg must come _before_ the exon segs, this is guaranteed by
 * the numerical value of its seg->type (fmap_.h) combined with the arraySort at
 * the end of smapconvert.
 * 
 * If the CDS start or stop positions are set then we will need to
 * convert positions for finding the spliced DNA, e.g. for later
 * protein translation.
 */
static void convertCDS(SmapConvInfo *conv_info, int *cds_min_out, BOOL exons_found)
{
  KEY key =  conv_info->key ;
  SMapKeyInfo *info = conv_info->info ;
  OBJ obj = conv_info->obj ;
  int cds_min, cds_max ;
  int y1, y2 ;
  SMapStatus status ;
  BOOL cds_coords = FALSE ;
  static KEY coding_region = KEY_NULL, source_exon ;


  /* Introns are implicitly represented by Source_exons so let's add an implicitly created tag. */
  if (!coding_region)
    {
      coding_region = str2tag(ACE2SO_SUBPART_CODING) ;
      source_exon = str2tag(ACE2SO_SUBPART_EXON) ;
    }

  cds_min = 1 ;
  cds_max = 0 ;
  if (bsGetData(obj, _bsRight, _Int, &cds_min))
    {
      cds_coords = TRUE ;
      bsGetData(obj, _bsRight, _Int, &cds_max) ;
    }
  
  /* Try to map the CDS, if we fail because of bad start/end in database     */
  /* then redo as whole of source exons, otherwise just fail.                */
  status = sMapMap(info, cds_min, cds_max, &y1, &y2, NULL, NULL) ;
  if (status & SMAP_STATUS_NO_OVERLAP)
    {
      
#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
      if (cds_coords)
	{
	  int tmp_min = cds_min, tmp_max = cds_max ;
	  
	  cds_min = 1 ;
	  cds_max = 0 ;
	  status = sMapMap(info, cds_min, cds_max, &y1, &y2, NULL, NULL) ;
	  if (status == SMAP_STATUS_PERFECT_MAP || status == SMAP_STATUS_INTERNAL_GAPS)
	    {
	      messerror("The start and/or end of the CDS in object %s"
			" is outside of the spliced DNA coordinates for that object"
			" CDS start = %d & end = %d)."
			" Make sure the CDS positions have been specified"
			" in spliced DNA, not Source_exon, coordinates."
			" CDS start/end positions have been reset to start/end of"
			" spliced DNA.",
			nameWithClassDecorate(key), tmp_min, tmp_max) ;
	    }
	  else
	    {
	      messerror("Cannot smap CDS for object %s, please check your data for this object.",
			nameWithClassDecorate(key)) ;
	    }
	  
	}
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */
      
    }
  
  
  if (SMAP_STATUS_INAREA(status))
    {
      SEQINFO *sinf ;
      SEG *seg ;

      seg = arrayp(conv_info->segs_array, arrayMax(conv_info->segs_array), SEG) ;
      seg->key = _CDS ;
      seg->parent = seg->source = key ;
      seg->subfeature_tag = coding_region ;
      seg->flags = SEGFLAG_UNSET ;
      fmapSMap2Seg(conv_info->info, seg, y1, y2, CDS) ;
      fmapSegSetClip(seg, status, CDS, CDS_UP) ;

      /* TRY THIS HERE.... */
      seg->data.i = conv_info->parent_seqinfo_index ;

      /* We need to record position and other CDS information for later  */
      /* drawing of the CDS.                                             */
      sinf = arrayp(conv_info->seqinfo, conv_info->parent_seqinfo_index, SEQINFO) ;

      sinf->flags |= SEQ_CDS ;
      sinf->cds_info.cds_seg = *seg ;
      sinf->cds_info.cds_only = TRUE ;
      
      /* record cds_min for use in convertStart()                            */
      *cds_min_out = cds_min ;
      
      if (fMapDebug)
	printf("CDS: %d --> %d\n\n", seg->x1, seg->x2) ;
      
      /* Possible that database may specify a CDS without any accompanying   */
      /* source exons so synthesize an exon which covers the whole of the    */
      /* object.                                                             */
      if (!exons_found)
	{
	  SEG *seg ;
	  
	  seg = arrayp(conv_info->segs_array, arrayMax(conv_info->segs_array), SEG) ;
	  *seg = arr(conv_info->segs_array, conv_info->parent_seg_index, SEG) ;
	  /* n.b. struct copy. */
	  seg->parent = seg->source = key ;
	  seg->key = 0 ;
	  seg->subfeature_tag = source_exon ;

	  /* unusual because we are copying from parent seg...               */
	  fmapSMap2Seg(conv_info->info, seg, (seg->x1 + 1), (seg->x2 + 1), EXON) ;
	  
	  sinf->flags |= SEQ_EXONS ;
	}
    }
  
  return ;
}



/* N.B. should be called after convertCDS() as it may need cds_min for       */
/* checking.                                                                 */
/*                                                                           */
static void convertStart(SmapConvInfo *conv_info, int cds_min)
{
  SEQINFO *sinf ;
  KEY key =  conv_info->key ;
  OBJ obj = conv_info->obj ;
	

  sinf = arrayp(conv_info->seqinfo, conv_info->parent_seqinfo_index, SEQINFO) ;


  /* Tag signals that this transcript is incomplete and that translation
   * should start N bases into the cds where  1 <= N <= 3  where  N == 1
   * is the _first_ base of the cds (i.e. same as if start_not_found was
   * not specified). If there is a CDS, then it must start at position 1
   * for this tag to be valid. */
  if (sinf->cds_info.cds_only && (cds_min != 1))
    {
      messerror("Data inconsistency: the Start_not_found tag is set for object %s,"
		" but the CDS begins at position %d instead of at position 1"
		" of the first exon, Start_not_found tag will be ignored",
		name(key), cds_min) ;
    }
  else
    {
      sinf->start_not_found = 1 ;

      /* Record any value found in seqinfo for later use.            */
      if (bsGetData(obj, _bsRight, _Int, &sinf->start_not_found))
	{
	  if (sinf->start_not_found < 1 || sinf->start_not_found > 3)
	    {
	      messerror("Data inconsistency: the Start_not_found tag for object %s"
			" should be given the value 1, 2 or 3 but has been"
			" set to %d,"
			" tag will be ignored",
			name(key), sinf->start_not_found) ;

	      sinf->start_not_found = 0 ;
	    }
	}
    }


  return ;
}



static void convertEnd(SmapConvInfo *conv_info)
{
  SEQINFO *sinf ;
	
  sinf = arrayp(conv_info->seqinfo, conv_info->parent_seqinfo_index, SEQINFO) ;

  /* Tag signals that this object is incomplete, there is more downstream somewhere. */
  sinf->end_not_found = TRUE ;

  return ;
}



/* I HOPED TO REWRITE BSFLATTEN TO RETURN SUB-OBJECTS TOO BUT IT DOESN'T WORK LIKE THAT,
   YOU DON'T HAVE THE OBJECT POSITION, JUST THE DATA IN THE BSFLATTEN ARRAY SO YOU CAN'T
   EASILY RETURN SOMETHING THAT IS USEFUL TO THE CALLER...I THINK I WILL JUST HAVE TO 
   REWRITE THIS IN THE STYLE OF THE HOMOL CODE AND CHUG ROUND STUFF.....

   IT ALL FEELS A BIT BOLLOCKS REALLY....BE BETTER TO HAVE SOME FUNCTION THAT WOULD
   CHUG THROUGH AND CALL CALLBACKS AS YOU DID SO....
*/

/* Processes the following model fragment which can be embedded in any class:
 * 
 *	  Feature ?Method Int Int UNIQUE Float UNIQUE Text #Feature_info
 *		// Float is score, Text is note
 *		// note is shown on select, and same notes are neighbours
 *		// again, each method has a column double-click shows the method.
 *		// Absorb Assembly_tags?
 *
 */
static void convertFeature(SmapConvInfo *conv_info, DICT *featDict)
{
  static KEY _InterBase = KEY_UNDEFINED ;
  KEY key = conv_info->key ;
  SMapKeyInfo *info = conv_info->info ;
  OBJ obj = conv_info->obj ;
  BSMARK mark1 = 0, mark2 = 0, mark3 = 0, mark4 = 0, mark5 = 0 ;
  KEY method, tag2 = KEY_NULL ;
  float score;
  int start, end ;
  char *text = NULL ;

  if (!_InterBase)
    {
      _InterBase = str2tag("InterBase") ;
    }


  bsGetKeyTags(obj, _bsHere, &tag2) ;

  if (bsGetKey(obj, _bsRight, &method))
    do
      { 
	METHOD *meth ;

	/* Check method to see if we need to filter on it. */
	if (fmapFilterOut(conv_info, method))
	  continue;

	meth = methodCacheGet(conv_info->method_cache, method, "") ;
	messAssert(meth) ;

	mark1 = bsMark(obj, mark1) ;
	  
	if (bsGetData(obj, _bsRight, _Int, &start))
	  do
	    {
	      mark2 = bsMark(obj, mark2);

	      if (bsGetData(obj, _bsRight, _Int, &end))
		do
		  {
		    SEG *seg ;
		    int y1, y2 ;
		    SMapStatus status ;

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
		    int feature_index ;
		    FeatureInfo *feat ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */


		    mark3 = bsMark(obj, mark3);


		    /* If get this far we make a seg for display/dumping. */
		    status = sMapMap(info, start, end, &y1, &y2, NULL, NULL) ;
		    if (!SMAP_STATUS_INAREA(status))
		      continue;

		    seg = arrayp(conv_info->segs_array, arrayMax(conv_info->segs_array), SEG);
		    seg->source = key;
		    seg->key = method ; 	    /* n.b. assigns Method obj to key. */
		    if (tag2)
		      seg->subfeature_tag = tag2 ;

		    fmapSMap2Seg(conv_info->info, seg, y1, y2, FEATURE);


#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
		    /* OK, CAN'T DO THIS, NEED A NEW ARRAY OTHERWISE WHEN DRAW FUNCTIONS
		     * ARE ALLOCATED (mapInsertCol()) THEN THE WRONG DRAW FUNCTION IS
		     * ALLOCATED...AND FEATURES ARE NOT DRAWN..... */

		    feature_index = arrayMax(conv_info->feature_info) ;
		    seg->data.i = feature_index ;
		    feat = arrayp(conv_info->feature_info, feature_index, FeatureInfo) ;
		    feat->method = method ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

		    if (bsGetData(obj, _bsRight, _Float, &score))
		      do
			{ 
			  mark4 = bsMark(obj, mark4);


			  seg->data.f = score ;


#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
			  feat->score = score ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */


			  if (bsGetData(obj, _bsRight, _Text, &text))
			    do
			      {
				mark5 = bsMark(obj, mark5) ;


				if (text)
				  {
				    int tmp;

				    /* The value returned by dictAdd is later used by the column bumping code as a hash key
				     * for cluster bumping. The value is made negative to avoid collisions in the
				     * cluster hash with real object keys (see bcDrawBox() in fmapfeatures.c).  */
				    dictAdd(featDict, text, &tmp);
				    seg->parent = -tmp;
				  }
				else
				  {
				    seg->parent = 0;
				  }

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
				if (bsPushObj(obj))
				  {
				    if (bsFindTag(obj, _InterBase))
				      feat->interbase_feature = TRUE ;

				    bsGoto(obj,0) ;
				  }
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */


				bsGoto(obj, mark5) ;
			      } while (bsGetData(obj, _bsDown, _Text, &text)) ;


			  bsGoto(obj, mark4) ;
			} while (bsGetData(obj, _bsDown, _Float, &score));

		    bsGoto(obj, mark3);
		  } while (bsGetData(obj, _bsDown, _Int, &end)) ;

	      bsGoto(obj, mark2);
	    } while (bsGetData(obj, _bsDown, _Int, &start)) ;

	bsGoto(obj, mark1);
      } while (bsGetKey(obj, _bsDown, &method));



  /* Clear up. */
  bsMarkFree(mark1) ;
  bsMarkFree(mark2) ;
  bsMarkFree(mark3) ;
  bsMarkFree(mark4) ;


  return ;
}








#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
static void convertFeature(SmapConvInfo *conv_info, DICT *featDict)
{
  KEY key =  conv_info->key ;
  SMapKeyInfo *info = conv_info->info ;
  OBJ obj = conv_info->obj ;
  Array units = conv_info->units ;
  KEY tag2 = 0 ;

  bsGetKeyTags(obj, _bsHere, &tag2) ;

  if (bsFlatten (obj, 5, units))
    {
      int j ;

      for (j = 0 ; j < arrayMax(units) ; j += 5)
	{ 
	  BSunit *u = arrayp(units, j, BSunit);
	  SEG *seg;
	  int y1, y2;
	  SMapStatus status ;

	  if (class(u[0].k != _VMethod) || !u[1].i || !u[2].i)
	    continue;

	  /* skip this one if methods specified & it doesn't match
	   * Note that bsFlatten has created a one-dimensional array
	   * in which the 5 elements of the Feature tag are loaded
	   * into consecutive elements of the units array.  We don't
	   * use j to index into it as we point u at the set we want. */
	  if (fmapFilterOut(conv_info, u[0].k))
	    continue;

	  status = sMapMap(info, u[1].i, u[2].i, &y1, &y2, NULL, NULL) ;
	  if (!SMAP_STATUS_INAREA(status))
	    continue;

	  seg = arrayp(conv_info->segs_array, arrayMax(conv_info->segs_array), SEG);
	  seg->source = key;
	  seg->key = u[0].k;				    /* n.b. assigns Method obj to key. */
	  if (tag2)
	    seg->subfeature_tag = tag2 ;

	  seg->data.f = u[3].f;

	  fmapSMap2Seg(conv_info->info, seg, y1, y2, FEATURE);

	  if (u[4].s)
	    {
	      int tmp;

	      /* The value returned by dictAdd is later used by the column bumping code as a hash key
	       * for cluster bumping. The value is made negative to avoid collisions in the
	       * cluster hash with real object keys (see bcDrawBox() in fmapfeatures.c).  */
	      dictAdd(featDict, u[4].s, &tmp);
	      seg->parent = -tmp;
	    }
	  else
	    {
	      seg->parent = 0;
	    }
	}
    }

  return ;
}
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */





static void convertVisible(SmapConvInfo *conv_info)
{
  KEY key =  conv_info->key ;
  SMapKeyInfo *info = conv_info->info ;
  OBJ obj = conv_info->obj ;
  Array units = conv_info->units ;

  if (bsFlatten(obj, 2, units))
    {
      int j ;

      for (j = 0 ; j < arrayMax(units) ; j += 2)
	{
	  int y1, y2 ;
	  SEG *seg ;
	  SMapStatus status ;

	  status = sMapMap(info, 1, 0, &y1, &y2, NULL, NULL);	    /* cannot fail */
	  if (!SMAP_STATUS_INAREA(status))
	    continue ;
	  
	  if (!iskey (arr(units,j,BSunit).k) || class(arr(units,j,BSunit).k))
	    continue ;
	  /* should be a tag */
	  if (!iskey (arr(units,j+1,BSunit).k))
	    continue ;
	  /* there is a problem if the model says: Visible foo Text
	     a hack because iskey() could be true for a text pointer
	     should store these as different type, e.g. VISIBLE_TEXT
	  */

	  seg = arrayp(conv_info->segs_array, arrayMax(conv_info->segs_array), SEG);
	  seg->source = seg->parent = key;
	  seg->key = arr(units, j+1, BSunit).k;
	  seg->data.k = arr(units, j, BSunit).k;
	  fmapSMap2Seg(conv_info->info, seg, y1, y2, VISIBLE) ;
	}
    }

  return ;
}



/* Conversion of assembly data, the sub-part of the Sequence class is:       */
/*                                                                           */
/*  Assembly_tags	Text Int Int Text // type, start, stop, comment      */
/*                                                                           */
/* I make the assumption that if the start or stop are not given then we do  */
/* _NOT_ map the feature, it _must_ have these at least to be mapped. This   */
/* is a change in behaviour from the old code.                               */
/*                                                                           */
void convertAssemblyTags(SmapConvInfo *conv_info)
{
  KEY key = conv_info->key ;
  SMapKeyInfo *info = conv_info->info ;
  OBJ obj = conv_info->obj ;
  Array units = conv_info->units ;
  int pos1, pos2 ;
  int y1, y2 ;
  SMapStatus status ;

  if (bsFlatten (obj, 4, units))
    {
      int j ;

      for (j = 0 ; j < arrayMax(units)/4 ; ++j)
	{
	  int length;
	  char *string;
	  char *type_text, *text_text;

	  pos1 = arr(units, 4*j+1, BSunit).i;
	  pos2 = arr(units, 4*j+2, BSunit).i;

	  /* At this point we could report errors if required.               */
	  if (pos1 == 0 || pos2 == 0)
	    continue ;

	  status = sMapMap(info, pos1, pos2, &y1, &y2, NULL, NULL) ;
	  if (SMAP_STATUS_INAREA(status))
	    {
	      SEG *seg ;

	      seg = arrayp(conv_info->segs_array, arrayMax(conv_info->segs_array), SEG);
	      seg->parent = seg->source = key;
	      seg->key = _Assembly_tags;

	      /* Although assembly tags are assumed to be non-stranded, in fact their
	       * coords are sometimes reversed so we set them using fmapSMap2Seg() and
	       * then reset the type. */
	      fmapSMap2Seg(info, seg, y1, y2, ASSEMBLY_TAG) ;
	      seg->type = ASSEMBLY_TAG ;

	      /* Get the text */
	      length = 0;
	      type_text = arr(units, 4*j, BSunit).s;
	      if (type_text)
		length += strlen(type_text);
	      text_text = arr(units, 4*j+3, BSunit).s;
	      if (text_text)
		length += strlen(text_text);
	      seg->data.s = string = halloc(length+3, conv_info->segs_handle);
	      string[0] = '\0';
	      if (type_text)
		strcpy(string, type_text) ;
	      strcat(string, ": ");
	      if (text_text) 
		strcat(string, text_text);
	    }
	}
    }

  return ;
}



#ifdef ED_G_NEVER_INCLUDE_THIS_CODE

/* I THINK THE FIRST WAY IS NOW REDUNDANT..... */

/* Conversion of allele data.
 * 
 * Alleles are now represented in two ways which are both processed by this
 * routine. This routine reads allele data in both forms and converts the data
 * into ALLELE segs.
 *
 * The object coming in will be either a Sequence class object or some other
 * SMap'd class of object.
 * 
 * If the class is _not_ a Sequence class object then the position has already
 * been mapped and we just need to check that the Allele tag format is correct:
 *
 *         Allele
 *
 * and then check that there is a method. In the new world anything without a
 * method will _NOT_ be mapped.
 *
 * If the class is a Sequence class object then we must check that the data
 * following the Allele tag is in the old format:
 *
 *  Allele ?Allele XREF Sequence UNIQUE Int UNIQUE Int UNIQUE Text
 *
 * Note I make the assumption that if the start or stop are not given then we do
 * _NOT_ map the allele, it _must_ have these at least to be mapped. As with
 * old code we allow the Text to be NULL.
 *
 * Its important to check the format because some databases have reused the
 * Allele tag as part of their SMap tags:
 *
 *  Allele ?Allele XREF Sequence UNIQUE Int UNIQUE Int #SMap_info
 *  
 */
static void convertAllele(SmapConvInfo *conv_info)
{
  KEY key =  conv_info->key ;
  SMapKeyInfo *info = conv_info->info ;
  OBJ obj = conv_info->obj ;
  Array units = conv_info->units ;
  KEY method_key = str2tag("Method");


  if (!(conv_info->is_Sequence_class))
    {
      /* non-sequence class objects. */
      SEG *seg ;
      KEY method = KEY_UNDEFINED ;

      /* Really we should check that the Allele tag is in the correct format here,
       * there is a big question about how/if we want to go down this road.....
       * We don't want to do this sort of checking at this level because the
       * performance implications are BAD....but we do have a problem because with Smap
       * the same tag may be used in one object as part of the Smap tags and in another
       * to signify that an object is an allele etc.... */

      /* To be shown must have a method +  if filtering based on method, method must match. */
      if (bsGetKey(conv_info->obj, method_key, &method)
	  && !fmapFilterOut(conv_info, method))
	{
	  seg = arrayp(conv_info->segs_array, arrayMax(conv_info->segs_array), SEG) ;
	  seg->source = sMapParent(conv_info->info) ;
	  seg->key = conv_info->key ;
	  seg->parent = KEY_UNDEFINED ;
	  seg->data_type = SEG_DATA_UNSET ;
	  seg->data.i = 0 ;
	  if (conv_info->y1 || conv_info->y2) 
	    seg->parent = seg->key ;			    /* flag to draw it */
	  else
	    seg->parent = seg->source ;			    /* couple with parent */
	  fmapSMap2Seg(conv_info->info, seg, conv_info->y1, conv_info->y2, ALLELE) ;
	}
    }
  else
    {
      if (isOldAlleleModel(obj) && bsFlatten (obj, 4, units))
	{
	  int i ;
	  
	  for (i = 0 ; i < arrayMax (units) ; i += 4)
	    { 
	      BSunit *u = arrayp(units, i, BSunit) ;
	      SEG *seg ;
	      char *string ;
	      int y1, y2 ;
	      SMapStatus status ;
	      
	      /* The indices here are hardcoded as we point u to the set of
	       * array elements we're interested in.
	       */
	      /* Only process Allele data if there is position information and that
	       * position is within smap'd sequence. */
	      if (u[1].i == 0 || u[2].i == 0)
		continue ;
	      
	      status = sMapMap(info, u[1].i, u[2].i, &y1, &y2, NULL, NULL) ;
	      if (!SMAP_STATUS_INAREA(status))
		continue;
	      
	      seg = arrayp(conv_info->segs_array, arrayMax(conv_info->segs_array), SEG) ;
	      seg->source = key ;
	      seg->key = u[0].k ;
	      seg->parent = 0 ;
	      seg->data_type = SEG_DATA_UNSET ;
	      seg->data.i = 0 ;
	      if (y1 || y2) 
		seg->parent = seg->key ;			    /* flag to draw it */
	      else
		seg->parent = seg->source ;			    /* couple with parent */
	      fmapSMap2Seg(conv_info->info, seg, y1, y2, ALLELE) ;
	      

	      if ((string = u[3].s))
		{
		  seg->data.s = strnew(string, conv_info->segs_handle) ;
		  seg->data_type = SEG_DATA_STRING ;
		}
	    }
	}
    }

  return ;
}
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */



static void convertAllele(SmapConvInfo *conv_info)
{
  KEY key =  conv_info->key ;
  SMapKeyInfo *info = conv_info->info ;
  OBJ obj = conv_info->obj ;
  Array units = conv_info->units ;


  if (isOldAlleleModel(obj) && bsFlatten (obj, 4, units))
    {
      int i ;
	  
      for (i = 0 ; i < arrayMax (units) ; i += 4)
	{ 
	  BSunit *u = arrayp(units, i, BSunit) ;
	  SEG *seg ;
	  char *string ;
	  int y1, y2 ;
	  SMapStatus status ;
	      
	  /* The indices here are hardcoded as we point u to the set of
	   * array elements we're interested in.
	   */
	  /* Only process Allele data if there is position information and that
	   * position is within smap'd sequence. */
	  if (u[1].i == 0 || u[2].i == 0)
	    continue ;
	      
	  status = sMapMap(info, u[1].i, u[2].i, &y1, &y2, NULL, NULL) ;
	  if (!SMAP_STATUS_INAREA(status))
	    continue;
	      
	  seg = arrayp(conv_info->segs_array, arrayMax(conv_info->segs_array), SEG) ;
	  seg->source = key ;
	  seg->key = u[0].k ;
	  seg->parent = 0 ;
	  seg->data_type = SEG_DATA_UNSET ;
	  seg->data.i = 0 ;
	  if (y1 || y2) 
	    seg->parent = seg->key ;			    /* flag to draw it */
	  else
	    seg->parent = seg->source ;			    /* couple with parent */
	  fmapSMap2Seg(conv_info->info, seg, y1, y2, ALLELE) ;
	      

	  if ((string = u[3].s))
	    {
	      seg->data.s = strnew(string, conv_info->segs_handle) ;
	      seg->data_type = SEG_DATA_STRING ;
	    }
	}
    }

  return ;
}



/* Expects to be located at Allele tag in current object and then checks to 
 * see if model looks like:
 * 
 *  Allele ?Allele XREF Sequence UNIQUE Int UNIQUE Int UNIQUE Text
 * 
 * .......although only checks the final Text field, returns TRUE if final
 * field is "Text", FALSE otherwise. */
static BOOL isOldAlleleModel(OBJ obj)
{
  BOOL result = TRUE ;
  BS bsm = NULL ;
  KEY node_key = KEY_UNDEFINED ;
  char *text = NULL ;
  
  bsm = bsModelCurrent(obj) ;
  node_key = bsModelKey(bsm) ;

  if (strcmp(name(node_key), "Allele") != 0)
    messcrash("Wrongly located, supposed to be at Allele tag but actually at: %s",
	      name(node_key)) ;
  
  while ((bsm = bsModelRight(bsm)))
    {
      node_key = bsModelKey(bsm) ;
      text = name(node_key) ;
    }
  if (text && strcmp(text, "Text") == 0)
    result = TRUE ;
  else
    result = FALSE ;

  return result ;
}



/* Conversion of Clone End data, the sub-part of the Sequence class is:
 *
 *    Clone_left_end ?Clone XREF Clone_left_end UNIQUE Int
 *    Clone_right_end ?Clone XREF Clone_right_end UNIQUE Int
 *
 */
static void convertCloneEnd(SmapConvInfo *conv_info, KEY clone_tag)
{
  KEY key =  conv_info->key ;
  SMapKeyInfo *info = conv_info->info ;
  OBJ obj = conv_info->obj ;
  Array units = conv_info->units ;


  if (bsFlatten(obj, 2, units))
    {
      int i ;
      int y1, y2 ;

      for (i = 0 ; i < arrayMax(units) ; i += 2)
	{
	  BSunit *u = arrayp(units, i, BSunit) ;
	  SMapStatus status ;

	  status = sMapMap(info, u[1].i, u[1].i, &y1, &y2, NULL, NULL) ;
	  if (SMAP_STATUS_INAREA(status))
	    {
	      SEG *seg ;
	      KEY seqKey ;

	      seg = arrayp(conv_info->segs_array, arrayMax(conv_info->segs_array), SEG) ;
	      seg->source = key ;
	      seg->key = u[0].k ;			    /* Clone */
	      fmapSMap2Seg(conv_info->info, seg, y1, y2, CLONE_END) ;
	      seg->data.k = clone_tag ;
	      lexaddkey(name(seg->key), &seqKey, _VSequence) ;
	      seg->parent = seqKey ;
	    }
	}
    }

  return ;
}


/* Conversion of oligo data, the sub-part of the Sequence class is:
 * 
 *  Oligo ?Oligo XREF In_sequence Int UNIQUE Int [?Method]
 * 
 * ?Method is relatively new and will not be in many databases, code must
 * handle this.
 * 
 * 
 *                                                                           */
static void convertOligo(SmapConvInfo *conv_info)
{
  static KEY _Tm = KEY_UNDEFINED, _Temporary = KEY_UNDEFINED ;
  KEY key =  conv_info->key ;
  SMapKeyInfo *info = conv_info->info ;
  OBJ obj = conv_info->obj ;
  Array units = conv_info->units ;

  if (_Tm == KEY_UNDEFINED)
    {
      _Tm = str2tag ("Tm") ;
      _Temporary = str2tag ("Temporary") ;
    }

  if (bsFlatten (obj, 4, units))
    {
      int i ;

      for (i = 0 ; i < arrayMax (units) ; i += 4)
	{
	  BSunit *u = arrayp(units, i, BSunit) ;
	  int y1, y2 ;
	  SMapStatus status ;
	  KEY method_key = u[3].k ;

	  /* The old code used to call fMapOspPositionOligo()                */
	  /* I'm not sure we really want fMapOspPositionOligo(), it actually */
	  /* alters the database...or tries to, to insert the oligo position.*/


	  if (u[1].i == 0 || u[2].i == 0)
	    continue ;


	  /* If there is a method but we don't want it then don't include this feature.... */
	  if (method_key && fmapFilterOut(conv_info, method_key))
	    continue ;


	  status = sMapMap(info, u[1].i, u[2].i, &y1, &y2, NULL, NULL) ;
	  if (SMAP_STATUS_INAREA(status))
	    {
	      SEG *seg ;
	      OBJ obj1 ;

	      seg = arrayp(conv_info->segs_array, arrayMax(conv_info->segs_array), SEG) ;
	      seg->source = key ;
	      seg->key = u[0].k ;
	      seg->parent = seg->key  ; 

	      /* If we have a method then record it in the sinf array. */
	      if (!method_key)
		{
		  seg->data.i = 0 ;
		}
	      else
		{
		  SEQINFO *sinf ;

		  conv_info->parent_seqinfo_index = arrayMax(conv_info->seqinfo) ;
		  sinf = arrayp(conv_info->seqinfo, conv_info->parent_seqinfo_index, SEQINFO) ;
		  seg->data.i = conv_info->parent_seqinfo_index ;

		  sinf->method = method_key ;
		}

	      fmapSMap2Seg(conv_info->info, seg, y1, y2, OLIGO) ;

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
	      if ((obj1 = bsCreate(seg->key)))
		{
		  float xf ; int xi ;

		  if (bsGetData(obj1, _Tm, _Float, &xf))
		    {
		      xi = 10 * xf ;
		      seg->data.i = (xi & 0x3FF) << 16 ;
		    }

		  if (bsGetData(obj1, _Score, _Float, &xf))
		    {
		      xi = xf ;
		      seg->data.i |= (xi & 0xfff) ;
		    }     

		  if (!bsFindTag(obj1, _Temporary)) 
		    seg->data.i |=  0x4000 ; /* old */

		  bsDestroy (obj1) ;
		}
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */
	      if ((obj1 = bsCreate(seg->key)))
		{
		  float xf ; int xi ;

		  if (bsGetData(obj1, _Tm, _Float, &xf))
		    {
		      xi = 10 * xf ;
		      seg->feature_data.oligo_type = (xi & 0x3FF) << 16 ;
		    }

		  if (bsGetData(obj1, _Score, _Float, &xf))
		    {
		      xi = xf ;
		      seg->feature_data.oligo_type |= (xi & 0xfff) ;
		    }     

		  if (!bsFindTag(obj1, _Temporary)) 
		    seg->feature_data.oligo_type |=  0x4000 ; /* old */

		  bsDestroy (obj1) ;
		}

	    }
	}
    }

  return ;
}


/* Converts this tag set:
 * 
 *      Confirmed_intron  Int Int #Splice_confirmation
 * 
 * where:
 *
 *      #Splice_confirmation cDNA ?Sequence
 *                           EST ?Sequence
 *                           Homology
 *                           UTR ?Sequence
 *                           False ?Sequence
 *                           Inconsistent ?Sequence
 *
 * BUT note that the Sequence is a new addition and may not be there in many
 * databases.
 * 
 *  */
static void convertConfirmedIntrons(SmapConvInfo *conv_info)
{
  static KEY _EST = KEY_UNDEFINED, _cDNA, _Homology, _UTR, _False, _Inconsistent ;
  KEY key =  conv_info->key ;
  SMapKeyInfo *info = conv_info->info ;
  OBJ obj = conv_info->obj ;
  Array units = conv_info->units ;
  SEG *seg ;

  /* if we're including methods, then we don't want anything
  * that doesn't have a method at all...think this might be redundant...
  */
  if (!conv_info->method_set || conv_info->include_methods == FALSE)
    {
      if (_EST == KEY_UNDEFINED)
	{
	  _EST = str2tag("EST") ;
	  _cDNA = str2tag("cDNA") ;
	  _Homology = str2tag("Homology") ;
	  _UTR = str2tag("UTR") ;
	  _False = str2tag("False") ;
	  _Inconsistent = str2tag("Inconsistent") ;
	}
      
      if (bsFlatten (obj, 4, units))
	{
	  int i ;
	  SEQINFO *sinf ;
	  BOOL intron_was_mapped ;

	  for (i = 0, intron_was_mapped = FALSE, sinf = NULL ; i < arrayMax(units) ; i += 4)
	    {
	      BSunit *u ;
	      int y1 = 0, y2 = 0 ;
	      ConfirmedIntronInfo confirm = NULL ;
	      
	      u = arrayp(units, i, BSunit) ;
	      
	      if (!u[0].i || !u[1].i)			    /* No coords, can't do anything. */
		continue ;

	      /* Each intron can be confirmed by multiple sources, we just want one
	       * SEG and seqinfo for each intron but must record the multiple confirmations
	       * for each intron, i.e. intron coords stay same but confirmation data changes. */
	      if (i == 0 || u[-4].i != u[0].i || u[-3].i != u[1].i)
		{
		  SMapStatus status ;
		  
		  status = sMapMap(info, u[0].i, u[1].i, &y1, &y2, NULL, NULL) ;
		  if (SMAP_STATUS_INAREA(status))
		    {
		      intron_was_mapped = TRUE ;

		      seg = arrayp(conv_info->segs_array, arrayMax(conv_info->segs_array), SEG) ;
		      
		      seg->source = key ;
		      seg->key = 0 ;
		      seg->data.i = 0 ;
		      fmapSMap2Seg(conv_info->info, seg, y1, y2, INTRON) ;
		      fmapSegSetClip(seg, conv_info->status, INTRON, INTRON_UP) ;

		      /* If we allocated a new seg then allocate a new seqinfo. */
		      seg->data.i = arrayMax(conv_info->seqinfo) ;
		      sinf = arrayp(conv_info->seqinfo, seg->data.i, SEQINFO) ;

		      sinf->confirmation = arrayHandleCreate(SEQ_CONFIRM_TYPES,
							     ConfirmedIntronInfoStruct,
							     conv_info->segs_handle) ;
		    }
		  else
		    {
		      intron_was_mapped = FALSE ;
		      continue ;
		    }
		}

	      /* If previous intron was _not_ mapped and current intron has same coords as
	       * previous intron then we should ignore the current intron as well. */
	      if (!intron_was_mapped)
		continue ;

	      /* Record the confirmation information. */
	      confirm = arrayp(sinf->confirmation, arrayMax(sinf->confirmation),
			       ConfirmedIntronInfoStruct) ;

	      /* I'm making all the strings be just the type otherwise it makes parsing of 
	       * confirmed introns in GFF etc. harder. */
	      if (u[2].k == _EST)
		{
		  sinf->flags |= SEQ_CONFIRM_EST ; 
		  confirm->confirm_str = "EST" ;
		  if (u[3].k)
		    confirm->confirm_sequence = u[3].k ;
		}
	      else if (u[2].k == _cDNA)
		{
		  sinf->flags |= SEQ_CONFIRM_CDNA ; 
		  confirm->confirm_str = "cDNA" ;
		  if (u[3].k)
		    confirm->confirm_sequence = u[3].k ;
		}
	      else if (u[2].k == _Homology)
		{
		  sinf->flags |= SEQ_CONFIRM_HOMOL ; 
		  confirm->confirm_str = "homology" ;
		  if (u[3].k)
		    confirm->confirm_sequence = u[3].k ;
		}
	      else if (u[2].k == _UTR)
		{
		  sinf->flags |= SEQ_CONFIRM_UTR ; 
		  confirm->confirm_str = "UTR" ;
		  if (u[3].k)
		    confirm->confirm_sequence = u[3].k ;
		}
	      else if (u[2].k == _False)
		{
		  sinf->flags |= SEQ_CONFIRM_FALSE ; 
		  confirm->confirm_str = "false" ;
		  if (u[3].k)
		    confirm->confirm_sequence = u[3].k ;
		}
	      else if (u[2].k == _Inconsistent)
		{
		  sinf->flags |= SEQ_CONFIRM_INCONSIST ; 
		  confirm->confirm_str = "inconsistent" ;
		  if (u[3].k)
		    confirm->confirm_sequence = u[3].k ;
		}
	      else
		{
		  sinf->flags |= SEQ_CONFIRM_UNKNOWN ; 
		  confirm->confirm_str = "unknown" ;
		}
	    }
	}
    }

  return ;
}


static void convertEMBLFeature(SmapConvInfo *conv_info)
{
  SMapKeyInfo *info = conv_info->info ;
  OBJ obj = conv_info->obj ;
  Array units = conv_info->units ;


  if (bsFlatten(obj, 4, units))
    {
      int i ;
      for (i = 0 ; i < arrayMax (units) ; i += 4)
	{
	  BSunit *u = arrayp(units, i, BSunit) ;
	  int y1, y2 ;
	  SMapStatus status ;

	  status = sMapMap(info, u[1].i, u[2].i, &y1, &y2, NULL, NULL) ;

	  if (SMAP_STATUS_INAREA(status))
	    {
	      char *string ;
	      KEY key ;
	      SEG *seg ;

	      seg = arrayp(conv_info->segs_array, arrayMax(conv_info->segs_array), SEG) ;
	      seg->source = conv_info->key ;
	      seg->parent = arrayMax(conv_info->segs_array) ;
	      seg->key = u[0].k ;
	      fmapSMap2Seg(conv_info->info, seg, y1, y2, EMBL_FEATURE) ;

	      /* 4'th column: can't distinguish KEY from Text securely 
		 assume if iskey() that it is a key,  e.g. ?Text
	      */
	      if (iskey(key = u[3].k))
		seg->data.s = strnew (name(key), conv_info->segs_handle) ;
	      else if ((string = u[3].s))
		seg->data.s = strnew(string, conv_info->segs_handle) ;
	      else
		seg->data.s = NULL ;
	    }
	}
    }
      
  return ;
}



static void convertSplices(SmapConvInfo *conv_info)
{
  static KEY _Predicted_5 = KEY_UNDEFINED, _Predicted_3 ;
  KEY key =  conv_info->key ;
  SMapKeyInfo *info = conv_info->info ;
  OBJ obj = conv_info->obj ;
  Array units = conv_info->units ;

  if (_Predicted_5 == KEY_UNDEFINED)
    {
      _Predicted_5 = str2tag("Predicted_5") ;
      _Predicted_3 = str2tag("Predicted_3") ;
    }
  
  if (bsFlatten(obj, 5, units))
    {
      int i ;
      
      for (i = 0 ; i < arrayMax(units) ; i += 5)
	{
	  BSunit *u = arrayp(units, i, BSunit) ;
	  int y1, y2, unused ;
	  SMapStatus status ;

	  if (u[0].k != _Predicted_5 && u[0].k != _Predicted_3)
	    continue ;
	  if (class(u[1].k) != _VMethod || !u[2].i || !u[3].i)
	    continue;
	  if (conv_info->method_set
	      && !(keySetFind(conv_info->method_set, u[1].k, &unused)))
	    continue ;

	  status = sMapMap(info, u[2].i, u[3].i, &y1, &y2, NULL, NULL) ;

	  if (SMAP_STATUS_INAREA(status))
	    {
	      SEG *seg ;
	      
	      seg = arrayp(conv_info->segs_array, arrayMax(conv_info->segs_array), SEG) ;
	      seg->source = key ;
	      seg->key = u[1].k ;
	      if (u[0].k == _Predicted_5)
		fmapSMap2Seg(conv_info->info, seg, y1, y2, SPLICE5) ;
	      else
		fmapSMap2Seg(conv_info->info, seg, y1, y2, SPLICE3) ;
	      seg->data.f = u[4].f ;
	    }
	}
    }

  return ;
}



#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
/* This encapsulates all the data from Acembly */
/* NOTE that this code is currently untested, and will probably need some ajustment of models.
   I will liase with Jean */
static void convertMieg(SmapConvInfo *conv_info)
{
  KEY key = conv_info->key ;
  SMapKeyInfo *info = conv_info->info ;
  OBJ obj = conv_info->obj ;
  Array units = conv_info->units ;
  int i;
  SEG *seg;
  KEY M_TRANSCRIPT = defaultSubsequenceKey ("TRANSCRIPT", DARKGRAY, 2.1, FALSE) ;
  static KEY _Transcribed_gene = KEY_UNDEFINED ;

  if (_Transcribed_gene == KEY_UNDEFINED)
    {
      _Transcribed_gene = str2tag("Transcribed_gene") ;
    }
  
  if (bsFindTag (obj, _Primer) && bsFlatten (obj, 1, units))
    for (i = 0 ; i < arrayMax(units) ; ++i)
      { 
	seg = arrayp (conv_info->segs_array, arrayMax(conv_info->segs_array), SEG);
 	seg->parent = seg->source = key;
	seg->key = arr(units,i, BSunit).k ;
	seg->type = PRIMER ;
      }
  
  if (bsFindTag (obj, _SCF_File))
    { 
      seg = arrayp (conv_info->segs_array, arrayMax(conv_info->segs_array), SEG);
      seg->parent = seg->source = key;
      seg->type = VIRTUAL_SUB_SEQUENCE_TAG ;

      /* TODO     if (!isDown)
	 seg->type = VIRTUAL_SUB_SEQUENCE_TAG_UP ; */
    }
  
  if (bsFindTag (obj, _Transcribed_gene) && 
      bsFlatten (obj, 3, units))
    { 
      for (i = 0 ; i < arrayMax(units) ; i += 3)
	{ 
	  int y1, y2;
	  KEYSET tmp;
	  BSunit *u = arrp(units,i,BSunit) ;
	  SMapStatus status ;

	  if (!u[0].k || !u[1].i || !u[2].i)
	    continue ;

	  status = sMapMap(info, u[1].i, u[2].i, &y1, &y2, NULL, NULL) ;
	  if (!SMAP_STATUS_INAREA(status))
	    continue;

	  y1--; y2--;
	  if (y2 < 0 || y1 >= conv_info->length)
	    continue;
	  
	  seg = arrayp (conv_info->segs_array, arrayMax(conv_info->segs_array), SEG);
	  seg->key = seg->parent = seg->source = u[0].k ;
	  seg->data.i = 0 ;
	  seg->type = y1 > y2 ? TRANSCRIPT : TRANSCRIPT_UP;

	  tmp = queryKey (seg->key, ">Annotations Fully_edited") ; 
	  if (keySetMax (tmp))
	    seg->data.i = 0x1 ;
	  keySetDestroy (tmp) ;
	  tmp = queryKey(seg->key, "Transpliced_to = SL1") ; 
	  if (keySetMax (tmp))
	    seg->data.i |= 2 ;
	  keySetDestroy (tmp) ;
	  tmp = queryKey(seg->key, "Transpliced_to = SL2") ; 
	  if (keySetMax (tmp))
	    seg->data.i |= 4 ;
	  keySetDestroy (tmp) ;
	  if (seg->data.i >= 6)
	  seg->data.i ^= 0x0F ;  /* leave bit 1, zero bit 2 and 4 , set bit 8 */
	}
    }
	      
  /* transcript pieces,  mieg */
  if (bsFindTag (obj, str2tag("Splicing")) &&
      bsFlatten (obj, 4, units))
    { 
      seg = arrayp (conv_info->segs_array, arrayMax(conv_info->segs_array), SEG);
      seg->key = M_TRANSCRIPT ;
      /* TODO seg->type = (isDown) ? TRANSCRIPT : TRANSCRIPT_UP ; */
      for (i = 0 ; i < arrayMax(units) ; i += 4)
	{ 
	  int y1, y2;
	  BSunit *u = arrayp(units, i, BSunit);
	  char *cp ;
	  SMapStatus status ;

	  if (arrayMax(units) < i + 3)
	    continue ;

	  status = sMapMap(info, u[0].i, u[1].i, &y1, &y2, NULL, NULL) ;
	  if (!SMAP_STATUS_INAREA(status))
	    continue;

	  seg = arrayp (conv_info->segs_array, arrayMax(conv_info->segs_array), SEG);
	  seg->key = u[2].k ;
	  seg->parent = seg->source = key;
	  /* TODO seg->type = (isDown) ? TRANSCRIPT : TRANSCRIPT_UP ; */
	  cp =  arr(units, i + 3, BSunit).s ;
	  if (cp && *cp && 
	      (seg->key == str2tag("Intron") ||
	       seg->key == str2tag("Alternative_Intron")))
	    if (!lexword2key(cp, &(seg->data.k), 0))
	      lexaddkey ("Other", &seg->data.k,0) ;
	}
    }
}
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */




/* selective for CALCULATED segs */
void fmapAddOldSegs (BOOL complement, Array segs, Array oldSegs, MethodCache mcache)
{
  SEG *seg, *oldMaster, *newMaster ;
  int i, j, length, offset ;
  METHOD *meth;

  oldMaster = arrp(oldSegs,0,SEG) ;
  newMaster = arrp(segs,0,SEG) ;
  if (newMaster->key != oldMaster->key)
    return ;

  length = newMaster->x2 - newMaster->x1 + 1 ;
  if (complement)
    offset = oldMaster->x2 - newMaster->x2 ;
  else
    offset = newMaster->x1 - oldMaster->x1 ;
#ifdef FMAP_DEBUG
  printf("In add oldsegs, offset = %d length = %d\n", offset, length);
  printf("Old: %d,%d  New: %d,%d\n", oldMaster->x1, oldMaster->x2,
	 newMaster->x1, newMaster->x2);
#endif

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




#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
/* Need to look at original fMapFindCoding() to see what the code used to    */
/* do.....                                                                   */
/* This was the fMapFindCoding() routine in the old fmap code.               */
/*                                                                           */
void fmapConvertCoding(SmapConvInfo *conv_info)
{
  int i, j, j1 ;
  Array index = NULL ;
  SEG *seg ;
  KEY parent ;
  static KEY source_coding = KEY_NULL ;

  /* Coding region exons are derived from Source_exons so add an implicitly created tag. */
  if (!source_coding)
    {
      source_coding = str2tag(ACE2SO_SUBPART_CODING) ;
    }


  /* May have to modify beginning of dna for translation because obj may */
  /* have begun in a previous (not present) exon so we don't have the beginning, the   */
  /* Start_not_found allows us to correct the reading frame by setting   */
  /* a new start position. Default is start of exon (start_not_found = 1) */
    

  for (i = 1 ; i < arrayMax(conv_info->segs_array) ; ++i)
    {
      seg = arrp(conv_info->segs_array, i, SEG) ;

      if (seg->type == CDS && (parent = seg->parent)
	  && getCDSPosition(conv_info->segs_array, conv_info->seqinfo, parent, &index))
	{
	  int cds_len ;
	  BOOL first_exon ;

	  if ((cds_len = arrp(conv_info->seqinfo, seg->data.i, SEQINFO)->start_not_found) != 0)
	    cds_len-- ;					    /* start_not_found is in range 1 -> 3,
							       we need 0 -> 2 for our loop. */

	  for (j1 = j = 0, first_exon = TRUE ; j < arrayMax(index) ; ++j)
	    {
	      if (j == arrayMax(index) - 1 || arr(index,j+1,int) > arr(index,j,int)+1) /* boundary */
		{
		  seg = arrayp (conv_info->segs_array, arrayMax(conv_info->segs_array), SEG) ;
		  seg->key = _CDS ;
		  seg->type = CODING ;
		  seg->x1 = arr(index, j1, int) ; 
		  seg->x2 = arr(index, j, int) ;
		  seg->parent = parent ;
		  seg->subfeature_tag = source_coding ;

		  seg->data.i = cds_len + 1 ;

		  if (first_exon && cds_len != 0)
		    {
		      /* If start_not_found set then set up cds start so phase can be correctly
		       * calculated from it and reset cds_len to ensure phase for subsequent
		       * exons is calculated taking start_not_found into account. */
		      switch (cds_len + 1)
			{
			case 1:
			  seg->data.i = 1 ;
			  break ;
			case 2:
			  seg->data.i = 0 ;
			  break ;
			case 3:
			  seg->data.i = 2 ;
			  break ;
			}

		      cds_len = ((j - (j1 + cds_len)) + 1) ;
		    }
		  else
		    {
		      seg->data.i = cds_len + 1 ;

		      cds_len += ((j - j1) + 1) ;
		    }
		  first_exon = FALSE ;

		  j1 = j+1 ;
		}
	    }

	  arrayDestroy (index) ;
	  index = NULL ;
	}
      else if (seg->type == CDS_UP && (parent = seg->parent)
	       && getCDSPosition(conv_info->segs_array, conv_info->seqinfo, parent, &index))
	{
	  int cds_len ;
	  BOOL first_exon ;

	  if ((cds_len = arrp(conv_info->seqinfo, seg->data.i, SEQINFO)->start_not_found) != 0)
	    cds_len-- ;					    /* start_not_found is in range 1 -> 3,
							       we need 0 -> 2 for our loop. */
	  for (j1 = j = 0, first_exon = TRUE ; j < arrayMax(index) ; ++j)
	    {
	      if (j == arrayMax(index) - 1 || arr(index,j+1,int) < arr(index,j,int)-1) /* boundary */
		{
		  seg = arrayp (conv_info->segs_array, arrayMax(conv_info->segs_array), SEG) ;
		  seg->key = _CDS ;
		  seg->type = CODING_UP ;
		  seg->x1 = arr(index,j,int) ; 
		  seg->x2 = arr(index,j1,int) ;
		  seg->parent = parent ;
		  seg->subfeature_tag = source_coding ;

		  if (first_exon && cds_len != 0)
		    {
		      switch (cds_len + 1)
			{
			case 1:
			  seg->data.i = 1 ;
			  break ;
			case 2:
			  seg->data.i = 0 ;
			  break ;
			case 3:
			  seg->data.i = 2 ;
			  break ;
			}

		      /* If start_not_found set then reset cds_len to ensure phase for subsequent
		       * exons is calculated taking start_not_found into account. */
		      cds_len = ((j - (j1 + cds_len)) + 1) ;
		    }
		  else
		    {
		      seg->data.i = cds_len + 1 ;

		      cds_len += ((j - j1) + 1) ;
		    }
		  first_exon = FALSE ;

		  j1 = j+1 ;
		}
	    }

	  arrayDestroy (index) ;
	  index = NULL ;
	}
    }

  return ;
}
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */
void fmapConvertCoding(SmapConvInfo *conv_info)
{
  int i, j, j1 ;
  Array index = NULL ;
  SEG *seg ;
  KEY parent ;
  static KEY source_coding = KEY_NULL ;

  /* Coding region exons are derived from Source_exons so add an implicitly created tag. */
  if (!source_coding)
    {
      source_coding = str2tag(ACE2SO_SUBPART_CODING) ;
    }


  /* May have to modify beginning of dna for translation because obj may */
  /* have begun in a previous (not present) exon so we don't have the beginning, the   */
  /* Start_not_found allows us to correct the reading frame by setting   */
  /* a new start position. Default is start of exon (start_not_found = 1) */
    

  for (i = 1 ; i < arrayMax(conv_info->segs_array) ; ++i)
    {
      seg = arrp(conv_info->segs_array, i, SEG) ;

      if (seg->type == CDS && (parent = seg->parent)
	  && getCDSPosition(conv_info->segs_array, conv_info->seqinfo, parent, &index))
	{
	  int cds_split = 0 ;				    /* Trailing codons from previous exon. */
	  BOOL first_exon ;


	  for (j1 = j = 0, first_exon = TRUE ; j < arrayMax(index) ; ++j)
	    {
	      if (j == arrayMax(index) - 1 || arr(index,j+1,int) > arr(index,j,int)+1) /* boundary */
		{
		  int cds_len, cds_phase ;

		  if (first_exon)
		    {
		      /* Correct for start_not_found (if specified). */
		      if ((cds_phase = arrp(conv_info->seqinfo, seg->data.i, SEQINFO)->start_not_found) != 0)
			{
			  /* start_not_found is in range 1 -> 3, we need 0 -> 2 for cds phase. */
			  cds_phase-- ;
			}

		      first_exon = FALSE ;
		    }
		  else
		    {
		      /* cds_split is mod 3 so these are the only possible values. */
		      switch (cds_split)
			{
			case 0:
			  cds_phase = 0 ;
			  break ;
			case 1:
			  cds_phase = 2 ;
			  break ;
			case 2:
			  cds_phase = 1 ;
			  break ;
			}
		    }

		  seg = arrayp (conv_info->segs_array, arrayMax(conv_info->segs_array), SEG) ;
		  seg->key = _CDS ;
		  seg->type = CODING ;
		  seg->x1 = arr(index, j1, int) ; 
		  seg->x2 = arr(index, j, int) ;
		  seg->parent = parent ;
		  seg->subfeature_tag = source_coding ;

		  seg->data.i = cds_phase ;

		  cds_len = (seg->x2 - seg->x1) + 1 ;
		  cds_len -= cds_phase ;
		  cds_split = cds_len % 3 ;

		  j1 = j+1 ;
		}
	    }

	  arrayDestroy(index) ;
	  index = NULL ;
	}
      else if (seg->type == CDS_UP && (parent = seg->parent)
	       && getCDSPosition(conv_info->segs_array, conv_info->seqinfo, parent, &index))
	{
	  int cds_split = 0 ;				    /* Trailing codons from previous exon. */
	  BOOL first_exon ;

	  for (j1 = j = 0, first_exon = TRUE ; j < arrayMax(index) ; ++j)
	    {
	      if (j == arrayMax(index) - 1 || arr(index,j+1,int) < arr(index,j,int)-1) /* boundary */
		{

		  int cds_len, cds_phase ;

		  if (first_exon)
		    {
		      /* Correct for start_not_found (if specified). */
		      if ((cds_phase = arrp(conv_info->seqinfo, seg->data.i, SEQINFO)->start_not_found) != 0)
			{
			  /* start_not_found is in range 1 -> 3, we need 0 -> 2 for cds phase. */
			  cds_phase-- ;
			}

		      first_exon = FALSE ;
		    }
		  else
		    {
		      /* cds_split is mod 3 so these are the only possible values. */
		      switch (cds_split)
			{
			case 0:
			  cds_phase = 0 ;
			  break ;
			case 1:
			  cds_phase = 2 ;
			  break ;
			case 2:
			  cds_phase = 1 ;
			  break ;
			}
		    }

		  seg = arrayp (conv_info->segs_array, arrayMax(conv_info->segs_array), SEG) ;
		  seg->key = _CDS ;
		  seg->type = CODING_UP ;
		  seg->x1 = arr(index,j,int) ; 
		  seg->x2 = arr(index,j1,int) ;
		  seg->parent = parent ;
		  seg->subfeature_tag = source_coding ;

		  seg->data.i = cds_phase ;

		  cds_len = (seg->x2 - seg->x1) + 1 ;
		  cds_len -= cds_phase ;
		  cds_split = cds_len % 3 ;


		  j1 = j+1 ;
		}
	    }

	  arrayDestroy (index) ;
	  index = NULL ;
	}
    }

  return ;
}




/* Add segs to represent the assembly/tiling path as constructed for the sequence
 * DNA by the smap code. */
void fmapConvertAssemblyPath(SmapConvInfo *conv_info)
{
  Array assembly ;
  KEY method ;

  if (lexword2key(ASSEMBLY_PATH_METHOD_NAME, &method, _VMethod) && (!fmapFilterOut(conv_info, method)))
    {
      /* Call SMap func to get the path from the current smap. */
      if ((assembly = sMapAssembly(conv_info->smap, NULL)))
	{
	  int i ;

	  for (i = 0 ; i < arrayMax(assembly) ; i++)
	    {
	      SMapAssembly fragment ;
	      SEG *seg ;
	      SEQINFO *sinf ;

	      fragment = arrp(assembly, i, SMapAssemblyStruct) ;

	      seg = arrayp (conv_info->segs_array, arrayMax(conv_info->segs_array), SEG) ;
	      seg->source = seg->key = fragment->key ;
	      seg->parent = conv_info->seq_orig ;

	      fmapSMap2Seg(conv_info->info, seg, fragment->start, fragment->end, ASSEMBLY_PATH) ;

	      seg->data.i = arrayMax(conv_info->seqinfo) ;
	      sinf = arrayp(conv_info->seqinfo, seg->data.i, SEQINFO) ;

	      sinf->assembly = *fragment ;			    /* struct copy. */
	    }
	}
    }

  return ;
}



/* We create intron segs when we process the Source_exons tag data but also when we process
 * the Confirmed_introns tag, these may have identical positions, in which case we unify them
 * into a single seg. */
void fmapRemoveDuplicateIntrons(SmapConvInfo *conv_info)
{
  int i ;
  SEG *seg ;


  /* The lack of commenting is as usual depressing,
   * why am I having to decode all this, just a few comments would be so very helpful... */
  for (i = 1 ; i < arrayMax(conv_info->segs_array) ; ++i)
    {
      seg = arrp(conv_info->segs_array, i, SEG) ;

      /* Only process intron segs. */
      if (seg->type == INTRON || seg->type == INTRON_UP)
	{
	  SEG *seg2 = seg ;
	  int flags = 0 ;
	  Array confirmation = NULL ;
	  BOOL existsParent = FALSE ;
	  SEQINFO *sinf ;


	  /* Run through all the intron segs that have the same position as the current one,
	   * recording flags, intron confirmation data, if there is a parent etc. */
	  while (i < arrayMax(conv_info->segs_array) - 1
		 && seg->type == seg2->type
		 && (seg->x1 == seg2->x1 && seg->x2 == seg2->x2))
	    {
	      sinf = arrp(conv_info->seqinfo, seg->data.i, SEQINFO) ;

	      /* Merge flag and other information for duplicate introns. */
	      flags |= sinf->flags ;

	      confirmation = mergeIntronConfirmation(confirmation, sinf->confirmation,
						     conv_info->segs_handle) ;

	      if (seg->parent)
		existsParent = TRUE ;

	      i++ ;
	      seg++ ;
	    }

	  for (--i, --seg ; seg2 <= seg ; ++seg2)
	    {
	      if (seg2->parent || !existsParent)
		{
		  sinf = arrp(conv_info->seqinfo, seg2->data.i, SEQINFO) ;
		  if (sinf->flags != flags || confirmation)
		    {
		      sinf = arrayp(conv_info->seqinfo, arrayMax(conv_info->seqinfo), SEQINFO) ;
		      *sinf = arr(conv_info->seqinfo, seg2->data.i, SEQINFO) ;
		      sinf->flags = flags ;
		      
		      sinf->confirmation = confirmation ;
		      
		      seg2->data.i = arrayMax(conv_info->seqinfo)-1 ;
		    }
		  existsParent = TRUE ;
		}
	      else
		{
		  *seg2 = array(conv_info->segs_array, arrayMax(conv_info->segs_array)-1, SEG) ;
		  --arrayMax(conv_info->segs_array) ;
		}
	    }
	}
    }


  return ;
}



/* routine runs through the list and removes all homols obeying the following condition:
 *
 * The homol overlaps a CDS and the homol homol parent has the same name as the cds
 * parent but after the embedded ":" in the homol homol name.
 *
 *
 * e.g. from Michaels example....
 *
 *
 * CDS PPA01167 has smap parent "Ppa_Contig25"
 *
 *
 * The disappearing homol data was in Homol_data
 *
 * "blat_pristionchus_cdnas:Ppa_Contig25"
 *
 * note the "Ppa_Contig25" in common....
 *
 *
 *
 * I can wave my hands around and come up with a reasonably plausible reason for wanting
 * to do this: if the CDS was actually built from those very homologies you may not want
 * them displayed in fmap....errr...maybe....
 *
 */
void fmapRemoveSelfHomol(Array segs, Array homolinfo)
{
  int i, max ;
  SEG *seg, *cds, *cdsMin, *cdsMax ;
  char *cp ;

  segFindBounds(segs, CDS, &i, &max) ;
  cdsMin = arrp(segs, i, SEG) ;
  cdsMax = arrp(segs, max-1, SEG) + 1 ; /* -1+1 for arrayCheck */
  
  segFindBounds(segs, HOMOL, &i, &max) ;

  for ( ; i < max && i < arrayMax(segs) ; ++i) 
    /* arrayMax(segs) can change */
    {

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
      HOMOLINFO *hinf ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */


      seg = arrp(segs, i, SEG) ;



#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
      if (keyIsStr(seg->key, "PPA01167"))
	{
	  hinf = arrp(homolinfo, seg->data.i, HOMOLINFO) ;

	  printf("%s (%s): %d -> %d\n",
		 name(seg->key),
		 name(hinf->method),
		 hinf->x1, hinf->x2) ;

	  printf("found it\n") ;
	}
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */


      for (cp = name(seg->key) ; *cp && *cp != ':' ; ++cp) ;

      if (*cp) 
	++cp ;
      else
	cp = name(seg->key) ;

      for (cds = cdsMin ; cds < cdsMax && cds->x1 < seg->x2 ; ++cds)
	if (cds->x1 <= seg->x1 && cds->x2 >= seg->x2 &&
	    !strcmp (cp, name(cds->parent)))
	  {
	    *seg = array(segs,arrayMax(segs)-1,SEG) ;
	    --arrayMax(segs) ;
	    break ;
	  }
    }





  segFindBounds(segs, CDS_UP, &i, &max) ;
  cdsMin = arrp(segs, i, SEG) ;
  cdsMax = arrp(segs, max-1, SEG) + 1 ; /* -1+1 for arrayCheck */

  segFindBounds(segs, HOMOL_UP, &i, &max) ;

  for ( ; i < max && i < arrayMax(segs) ; ++i) 
    /* arrayMax(segs) can change */
    {

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
      HOMOLINFO *hinf ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */


      seg = arrp(segs, i, SEG) ;


#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
      if (keyIsStr(seg->key, "PPA01167"))
	{
	  hinf = arrp(homolinfo, seg->data.i, HOMOLINFO) ;

	  printf("%s (%s): %d -> %d\n",
		 name(seg->key),
		 name(hinf->method),
		 hinf->x1, hinf->x2) ;

	  printf("found it\n") ;
	}
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */




      for (cp = name(seg->key) ; *cp && *cp != ':' ; ++cp) ;

      if (*cp) 
	++cp ;
      else
	cp = name(seg->key) ;

      for (cds = cdsMin ; cds < cdsMax && cds->x1 < seg->x2 ; ++cds)
	if (cds->x1 <= seg->x1 && cds->x2 >= seg->x2 &&
	    !strcmp (cp, name(cds->parent)))
	  {
	    *seg = array(segs,arrayMax(segs)-1,SEG) ;
	    --arrayMax(segs) ;
	    break ;
	  }
    }

  return;
}


/* Set up a default method for a feature, this code should gradually      
 * shrink to nothing. */
void fmapSetDefaultMethod(OBJ obj, SEQINFO *sinf)
{
  static KEY M_Transposon = KEY_UNDEFINED, M_SPLICED_cDNA, M_RNA,
             M_TRANSCRIPT, M_Pseudogene, M_Coding ;

  if (M_Transposon == KEY_UNDEFINED)
    {
      M_Coding = defaultSubsequenceKey ("Coding", BLUE, 2.0, TRUE) ;
      M_RNA = defaultSubsequenceKey ("RNA", GREEN, 2.0, TRUE) ;
      M_Pseudogene = defaultSubsequenceKey ("Pseudogene", DARKGRAY, 2.0, TRUE) ;
      M_Transposon = defaultSubsequenceKey ("Transposon", DARKGRAY, 2.05, FALSE) ;
      M_TRANSCRIPT = defaultSubsequenceKey ("TRANSCRIPT", DARKGRAY, 2.1, FALSE) ;
      M_SPLICED_cDNA = defaultSubsequenceKey ("SPLICED_cDNA", DARKGRAY, 2.2, FALSE) ;
    }

  if (bsFindTag (obj, _Pseudogene))
    sinf->method = M_Pseudogene ;
  else if (bsFindTag (obj, _Transposon))
    sinf->method = M_Transposon ;
  else if (bsFindTag (obj, _tRNA) || 
	   bsFindTag (obj, _rRNA) || 
	   bsFindTag (obj, _snRNA))
    sinf->method = M_RNA ;
  else if (bsFindTag (obj, _CDS))
    sinf->method = M_Coding ;
  else
    sinf->method = KEY_UNDEFINED ;

  return ;
}


/* Calculates the coords of the CDS section of the transcripts exons as an
 * array of the coords (on the ref. sequence). This could be done differently
 * but I've just taken existing code and modified it.
 * 
 * NOTE, this code only uses information in the segs and does not reread the
 * the database, therefore the user must do an fmap recalculate if they
 * change the cds/start_not_found in the database .
 * 
 *  */
static BOOL getCDSPosition(Array segs, Array seqinfo, KEY parent, Array *index)
{ 
  BOOL result = FALSE ;
  int   i, j=0, jCode = 0, cds1, cds2, tmp ;
  SEG   *seg = NULL ;
  BOOL isUp = FALSE ;
  BOOL first_exon ;

  if (iskey(parent))
    {
      cds1 = cds2 = 0 ;					    /* used to check if CDS found. */

      /* Get the cds coords for the transcript. */
      for (i = 1 ; i < arrayMax(segs) ; ++i)
	{
	  seg = arrp(segs,i,SEG) ;

	  if (seg->parent == parent && (seg->type == CDS || seg->type == CDS_UP))
	    {
	      cds1 = seg->x1 ;			    /* N.B. these are coords on displayed DNA. */
	      cds2 = seg->x2 ;

	      if (seg->type == CDS_UP)
		isUp = TRUE ;
	    }
	}

      /* Get the coords of the bases in the cds. */
      if (index)
	*index = arrayCreate (1000, int) ;

      for (i = 1, first_exon = TRUE ; i < arrayMax(segs) ; ++i)
	{
	  seg = arrp(segs,i,SEG) ;

	  if (seg->parent == parent && (seg->type == EXON || seg->type == EXON_UP))
	    {
	      j = seg->x1 ;

	      for( ; j <= seg->x2 ; ++j)
		{
		  if (index)
		    array(*index,jCode,int) = j ;

		  ++jCode ;
		}
	    }
	}

      /* Reverse is on reverse strand. */
      if (isUp)
	{
	  if (index)
	    for (j = 0, i = jCode-1 ; j < i ; ++j, --i)
	      {
		tmp = arr(*index,j,int) ;
		arr(*index,j,int) = arr(*index,i,int) ;
		arr(*index,i,int) = tmp ;
	      }
	}

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
      printf("%s - ", nameWithClass(parent)) ;
      for (i = 0 ; i < arrayMax(*index) ; i++)
	{
	  printf(" %d", arr(*index, i, int)) ;
	}
      printf("\n") ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

      result = TRUE ;
    }

  return result ;
}





/* Code to add an EST object to the array of read pairs.
 * In the array of read pairs, each pair holds the keys of the EST objects and
 * also the seg coordinates of the ends of those objects nearest each other. From this
 * we can create a HOMOL_GAP seg to span the gap between the EST read pair (the seg.
 * has a flag set so that a dashed line is used).
 * 
 * We use an associator to find out if the pair for any one read is already in the array
 * and return its index in the array if it is. The EST homol segs (homols_start to
 * homols_end) are position sorted so we can record the relevant top/bottom of the
 * EST object by whether it is 5' or 3' and record it + what the read pair is.
 * If the pair of this EST is already in the array, we just add this one to it,
 * if not we create a new array element with this EST's information. For the latter
 * we make a copy of the HOMOL seg for the EST object so that we get all the
 * parent etc. information. */

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
static void addReadPair(Array all_paired_reads, Associator key2readpair,
			KEY this_read, KEY paired_read, Array segs, int homol_index)
{
  static KEY EST_5 = KEY_UNDEFINED, EST_3 ;
  int read_index = -1 ;
  KEY orientation ;
  ReadPair readpair = NULL ;


  if (EST_5 == KEY_UNDEFINED)
    {
      EST_5 = str2tag("EST_5") ;
      EST_3 = str2tag("EST_3") ;
    }

  if (bIndexFind(this_read, EST_5))
    orientation = EST_5 ;
  else if (bIndexFind(this_read, EST_3))
    orientation = EST_3 ;
  else
    orientation = KEY_UNDEFINED ;			    /* Complain ?? */



  if (orientation)
    {
      BOOL found ;					    /* n.b. has more than one use below. */
      void *read_index_ptr = NULL ;
      SEG *seg ;

      seg = arrayp(segs, homol_index, SEG) ;

      /* If we can't find this EST's read pair in the array, add a new array element. */
      if (!(found = assFind(key2readpair, assVoid(paired_read), &read_index_ptr)))
	{
	  read_index = arrayMax(all_paired_reads) ;
	}
      else
	{
	  read_index = assInt(read_index_ptr) ;
	}	

      readpair = arrayp(all_paired_reads, read_index, ReadPairStruct) ;

      if (!found)
	memset(readpair, 0, sizeof(ReadPairStruct)) ;	    /* Needed for KEY_UNDEFINED tests. */

      /* Add EST to read pair array according to orientation and add coord. info.  */
      if (orientation == EST_5 && readpair->five_prime.key == KEY_UNDEFINED)
	{
	  found = TRUE ;
	  readpair->five_prime.key = this_read ;
	  

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
	  readpair->five_prime.pos = arrayp(segs, homol_index, SEG)->x2 ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */
	  readpair->five_prime.pos = seg->x2 ;
	  readpair->homol_index = homol_index ;



#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
	  printf("addReadPair() - 5' EST: on strand %c, at index %d, %s at %d\n",
		 SEG_IS_DOWN(seg) ? '+' : '-',
		 read_index,
		 name(readpair->five_prime.key),
		 readpair->five_prime.pos) ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */


	}
      else if (orientation == EST_3 && readpair->three_prime.key == KEY_UNDEFINED)
	{
	  found = TRUE ;
	  readpair->three_prime.key = this_read ;

	  readpair->three_prime.pos = arrayp(segs, homol_index, SEG)->x2 ;

	  readpair->homol_index = homol_index ;



#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
	  printf("addReadPair() - 3' EST: on strand %c, at index %d, %s at %d\n",
		 SEG_IS_DOWN(seg) ? '+' : '-',
		 read_index,
		 name(readpair->three_prime.key),
		 readpair->three_prime.pos) ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */


	}
      else
	{
	  found = FALSE ;				    /* complain ? */
	}

      /* Update the associator with the new ESTs -> array index. */
      if (found)
	assInsert(key2readpair, assVoid(this_read), assVoid(read_index)) ;
    }

  return ;
}
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */


static void addReadPair(Array all_paired_reads, Associator key2readpair,
			KEY this_read, KEY paired_read, Array segs, int homol_index)
{
  static KEY EST_5 = KEY_UNDEFINED, EST_3 ;
  int read_index = -1 ;
  KEY orientation = KEY_UNDEFINED ;
  ReadPair readpair = NULL ;


  if (EST_5 == KEY_UNDEFINED)
    {
      EST_5 = str2tag("EST_5") ;
      EST_3 = str2tag("EST_3") ;
    }


  if (bIndexFind(this_read, EST_5))
    orientation = EST_5 ;
  else if (bIndexFind(this_read, EST_3))
    orientation = EST_3 ;
  /* should we complain if there is no orientation ? check with worm group. */


  if (orientation)
    {
      BOOL found ;					    /* n.b. has more than one use below. */
      void *read_index_ptr = NULL ;
      SEG *seg ;

      seg = arrayp(segs, homol_index, SEG) ;


#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
      printf("addReadPair() EST\t%s\t%c'\t%c\t%d,%d\n",
	     name(this_read),
	     (orientation == EST_5 ? '5' : '3'),
	     SEG_IS_DOWN(seg) ? '+' : '-',
	     seg->x1, seg->x2) ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

      /* If we can't find this EST's read pair in the array, add a new array element. */
      if (!(found = assFind(key2readpair, assVoid(paired_read), &read_index_ptr)))
	{
	  read_index = arrayMax(all_paired_reads) ;
	}
      else
	{
	  read_index = assInt(read_index_ptr) ;
	}	

      readpair = arrayp(all_paired_reads, read_index, ReadPairStruct) ;

      if (!found)
	memset(readpair, 0, sizeof(ReadPairStruct)) ;	    /* Needed for KEY_UNDEFINED tests. */

      /* Add EST to read pair array according to orientation and add coord. info.  */
      if (orientation == EST_5 && readpair->five_prime.key == KEY_UNDEFINED)
	{
	  found = TRUE ;
	  readpair->five_prime.key = this_read ;
	  

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
	  readpair->five_prime.pos = arrayp(segs, homol_index, SEG)->x2 ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */
	  readpair->five_prime.pos = seg->x2 ;
	  readpair->homol_index = homol_index ;



#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
	  printf("addReadPair() - 5' EST: on strand %c, at index %d, %s at %d\n",
		 SEG_IS_DOWN(seg) ? '+' : '-',
		 read_index,
		 name(readpair->five_prime.key),
		 readpair->five_prime.pos) ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */


	}
      else if (orientation == EST_3 && readpair->three_prime.key == KEY_UNDEFINED)
	{
	  found = TRUE ;
	  readpair->three_prime.key = this_read ;

	  readpair->three_prime.pos = arrayp(segs, homol_index, SEG)->x2 ;

	  readpair->homol_index = homol_index ;



#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
	  printf("addReadPair() - 3' EST: on strand %c, at index %d, %s at %d\n",
		 SEG_IS_DOWN(seg) ? '+' : '-',
		 read_index,
		 name(readpair->three_prime.key),
		 readpair->three_prime.pos) ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */


	}
      else
	{
	  found = FALSE ;				    /* complain ? */
	}

      /* Update the associator with the new ESTs -> array index. */
      if (found)
	assInsert(key2readpair, assVoid(this_read), assVoid(read_index)) ;
    }

  return ;
}


/* Make homol line segs to join up each EST read pair recorded. The segs
 * have the dashed line flag set. */
void fmapMakePairedReadSegs(Array segs, Array all_paired_reads, Array homol_info)
{
  int i ;

  for (i = 0 ; i < arrayMax(all_paired_reads) ; i++)
    {
      ReadPair readpair ;

      readpair = arrayp(all_paired_reads, i, ReadPairStruct) ;
      if (readpair->five_prime.key && readpair->three_prime.key)
	{
	  SEG *homol_seg ;

	  homol_seg = arrayp(segs, readpair->homol_index, SEG) ;

	  if ((SEG_IS_DOWN(homol_seg) && (readpair->five_prime.pos > readpair->three_prime.pos))
	      || (SEG_IS_UP(homol_seg) && (readpair->five_prime.pos < readpair->three_prime.pos)))
	    {

	      /* Currently do nothing, Kerstin doesn't want all the messages... */

	      messerror("%s (5' match) and %s (3' match) are in wrong orientation.",
			nameWithClassDecorate(readpair->five_prime.key),
			nameWithClassDecorate(readpair->three_prime.key)) ;

	    }
	  else
	    {
	      SEG *seg ;

	      seg = arrayp(segs, arrayMax(segs), SEG) ;

	      *(seg) = *(arrayp(segs, readpair->homol_index, SEG)) ;	/* struct copy */
	  
	      if (SEG_IS_DOWN(seg))
		{
		  seg->type = HOMOL_GAP ;
		  seg->x1 = readpair->five_prime.pos ;
		  seg->x2 = readpair->three_prime.pos ;
		}
	      else
		{
		  seg->type = HOMOL_GAP_UP ;
		  seg->x1 = readpair->three_prime.pos ;
		  seg->x2 = readpair->five_prime.pos ;
		}


#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
	      {
		HOMOLINFO *hinf = arrp(homol_info, seg->data.i, HOMOLINFO) ;

		printf("makePairedReadSegs() - Seg from %s & %s (cluster on: %s) : %s at %d %d\n",
		       name(readpair->five_prime.key), name(readpair->three_prime.key),
		       name(hinf->cluster_key),
		       fMapSegTypeName[seg->type],
		       seg->x1, seg->x2) ;
	      }
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */


	      seg->flags |= SEGFLAG_LINE_DASHED ;
	    }
	}
    }

  return ;
}



/* Merge two confirmed intron structs, returning the first one as a result of the merge.
 * Returns:
 *             NULL if both input arrays are NULL,
 *             if confirm_inout is NULL then returns copy of confirm_in 
 *             if confirm_in is NULL just returns confirm_inout
 *             otherwise returns a merge of the two in confirm_inout
 * 
 * The handle param points to a short-coming in the array copy, it should do the copy on
 * the handle of the array it is copying by default......
 *  */
Array mergeIntronConfirmation(Array confirm_inout, Array confirm_in, STORE_HANDLE handle)
{
  Array merged_confirm = NULL ;
  ConfirmedIntronInfo merge, orig ;
  int i, j ;

  if (!confirm_inout && !confirm_in)
    {
      merged_confirm = NULL ;
    }
  else if (!confirm_inout)
    {
      merged_confirm = arrayHandleCopy(confirm_in, handle) ;
    }
  else if (!confirm_in)
    {
      merged_confirm = confirm_inout ;
    }
  else
    {
      for (i = 0 ; i < arrayMax(confirm_in) ; i++)
	{
	  int max_merge ;

	  orig = arrayp(confirm_in, i, ConfirmedIntronInfoStruct) ;

	  for (j = 0, max_merge = arrayMax(confirm_inout) ; j < max_merge ; j++)
	    {
	      merge = arrayp(confirm_inout, j, ConfirmedIntronInfoStruct) ;

	      /* We add in the new confirmation evidence if the types are different or if
	       * the confirming sequence is different. */
	      if (strcmp(orig->confirm_str, merge->confirm_str) != 0
		  || orig->confirm_sequence != merge->confirm_sequence)
		{
		  ConfirmedIntronInfo new_confirm = arrayp(confirm_inout, arrayMax(confirm_inout),
							   ConfirmedIntronInfoStruct) ;

		  new_confirm->confirm_str = orig->confirm_str ;
		  new_confirm->confirm_sequence = orig->confirm_sequence ;
		}
	    }
	}

      merged_confirm = confirm_inout ;
    }

  return merged_confirm ;
}


/* Just a copy of fMapOrder, main seg sorting routine which sorts by type
 * and position */
int fmapSegOrder(void *a, void *b)
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


static BOOL segFindBounds(Array segs, SegType type, int *min, int *max)
{
  for (*min = 1 ; *min < arrayMax(segs) ; ++*min)
    {
      if (arrp(segs, *min, SEG)->type == type)
	break ;
    }

  for (*max = *min ; *max < arrayMax(segs) ; ++*max)
    {
      if (arrp(segs, *max, SEG)->type != type)
	break ;
    }

  return (*min < arrayMax(segs)) ;
}



/* Translate the smap clipping flags into seg clipping for use by drawing    */
/* code.                                                                     */
void fmapSegSetClip(SEG *seg, SMapStatus status, SegType feature, SegType feature_up)
{
  if (SMAP_STATUS_SET(status, SMAP_STATUS_CLIP))
    {
      if (SMAP_STATUS_SET(status, SMAP_STATUS_CLIP))
	{
	  if (SMAP_STATUS_SET(status, SMAP_STATUS_X1_CLIP))
	    {
	      if (seg->type == feature)
		seg->flags |= SEGFLAG_CLIPPED_TOP ;
	      else
		seg->flags |= SEGFLAG_CLIPPED_BOTTOM ;
	    }
	  if (SMAP_STATUS_SET(status, SMAP_STATUS_X2_CLIP))
	    {
	      if (seg->type == feature)
		seg->flags |= SEGFLAG_CLIPPED_BOTTOM ;
	      else
		seg->flags |= SEGFLAG_CLIPPED_TOP ;
	    }
	}
    }
  else
    seg->flags = SEGFLAG_UNSET ;

  return ;
}



/*                                                                           */
/* Internal routines for setting up the keyset of methods, and hence,        */
/* features that should be included in the smapconvert()                     */
/*                                                                           */

/* This routine finds all the Method objects in the database and returns     */
/* them as a keyset.                                                         */
/*                                                                           */
/* The routine returns a keyset if there are any method objects and NULL if  */
/* not. It is the callers responsibility to destroy the keyset when          */
/* finished with.                                                            */
/*                                                                           */
static KEYSET allMethods(void)
{
  KEYSET result = NULL ;

  /* Get all the methods.                                                    */
  result = query(NULL, "FIND method") ;

  /* A refinement here would be to check all the objects to see if they any  */
  /* tags defined in them, I assume that if they don't then they are useless */
  /* but this might not be so.                                               */
  /* A more thorough approach would be to see if a method object was         */
  /* referenced by any sequence objects, if not then there would no point in */
  /* showing it.                                                             */

  return result ;
}




#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
/* Adding features to the dictionary.                                        */
static void addToSet (ACEIN command_in, DICT *dict)
{ 
  char cutter, *cutset, *word ;

  if (aceInStep (command_in, '"'))
    cutset = "|\"" ;
  else
    cutset = "| \t" ;
  while ((word = aceInWordCut (command_in, cutset, &cutter)))
    { 
      dictAdd (dict, word, 0) ;
      if (cutter != '|') break ;
    }

  return;
}
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */




/* This function determines whether or not to create a seg.
 * I did design some really snappy code to condense the    
 * decision into a few lines, but it was so opaque
 * I threw it away, preferring dozens of lines you can 
 * understand easily to a few you can't. */
BOOL fmapIsRequired(SmapConvInfo *conv_info)
{
  BOOL IsRequired = FALSE;
  KEY method_key = str2tag("Method"), method ;
  KEY source_key = str2tag("GFF_source") ;
  int unused;

  if (!conv_info->method_set && !conv_info->sources_dict)
    {
      IsRequired = TRUE;
    }
  else
    {
      if (conv_info->method_set)                                       /* is there a method set?  */
	{
	  if (conv_info->key == conv_info->seq_orig)	           /* is this the parent seg? */
	    {
	      IsRequired = TRUE;                            
	    }
	  else 
	    {
	      if (bIndexTag(conv_info->key, method_key)                 /* is there a method tag? */
		  && bsGetKey(conv_info->obj, method_key, &method))        /* can I get the key?  */
		{
		  char *column_tag = "Column_group" ;
		  KEY column_key = str2tag(column_tag) ;
		  OBJ obj ;

		  if (keySetFind(conv_info->method_set, method, &unused))   /* is it in the set?  */
		    {
		      IsRequired = TRUE;
		    }
		  else if (bIndexTag(method, column_key))
		    {
		      char *column_name = NULL ;

		      if ((obj = bsCreate(method)))
			{
			  KEY col_group_key = KEY_UNDEFINED ;

			  if (bsGetData(obj, column_key, _Text, &column_name)
			      && column_name && *column_name
			      && lexword2key(column_name, &col_group_key, _VMethod)
			      && keySetFind(conv_info->method_set, col_group_key, &unused))
			    {
			      IsRequired = TRUE ;
			    }
		  
			  bsDestroy(obj);
			}
		    }
		  else                                                       /* ie not in the set */
		    {
		      IsRequired = FALSE;
		    }
		}
	      else                                                                /* ie no method */
		{
		  if (conv_info->include_methods)
		    IsRequired = FALSE;
		  else
		    IsRequired = TRUE;
		}

	    }  /* if (key == seqKey) */
	}  /* if (method_set) */

      /* it may be I can store some of this stuff so I don't have to create an object  */
      /* each time, but for now I'll go along with this.                               */
      if (conv_info->sources_dict)	                                   /* is there a list of sources? */
	{
	  if (bIndexTag(conv_info->key, method_key)                    /* does obj have a method tag? */ 
	      && bsGetKey(conv_info->obj, method_key, &method))        /* can I get the key?          */
	    {
	      OBJ method_obj;
	      char *source;

	      if ((method_obj = bsCreate(method)))
		{
		  if (bsFindTag(method_obj, source_key)                       /* is there a GFF_source tag?  */
		      && bsGetData(method_obj, source_key, _Text, &source) 	  /* can I get the data from it? */
		      && dictFind(conv_info->sources_dict, source, &unused))  /* can I find it in the set?   */
		    {
		      if (conv_info->include_sources)
			IsRequired = TRUE;
		      else
			IsRequired = FALSE;
		    }

		  bsDestroy(method_obj) ;
		}
	      else
		{
		  if (conv_info->include_sources)
		    IsRequired = FALSE;
		  else
		    IsRequired = TRUE;
		}
	    }
	  else
	    {
	      if (conv_info->include_sources)
		IsRequired = FALSE;
	      else
		IsRequired = TRUE;
	    }
	}
    }

  return IsRequired ;
}

/* This is rather similar to the IsRequired() function and I may want to merge the two.
 * This is only called for objects that have methods.
 * Note that filtering on source has not been fully implemented, so don't expect it to be right.
 * Filtering on method and source are currently mutually exclusive, so one function is enough.  
 */
BOOL fmapFilterOut(SmapConvInfo *conv_info, KEY method)
{
  BOOL result = FALSE;
  int unused;

  /* check method */  
  if (conv_info->method_set)
    { 
      char *column_tag = "Column_group" ;
      KEY column_key = str2tag(column_tag) ;

      if (!(keySetFind(conv_info->method_set, method, &unused)))
	result = TRUE ;

      if (result && bIndexTag(method, column_key))
	{
	  OBJ obj ;
	  char *column_name = NULL ;

	  if ((obj = bsCreate(method)))
	    {
	      KEY col_group_key = KEY_UNDEFINED ;
	      
	      if (bsGetData(obj, column_key, _Text, &column_name)
		  && column_name && *column_name
		  && lexword2key(column_name, &col_group_key, _VMethod)
		  && keySetFind(conv_info->method_set, col_group_key, &unused))
		{
		  result = FALSE ;
		}
	      
	      bsDestroy(obj);
	    }
	}
    }      


  /* check source */
  if (conv_info->sources_dict)
    {
      OBJ method_obj ;
      char *source = NULL ;
      KEY source_key = str2tag("GFF_source") ;
      
      if ((method_obj = bsCreate(method)))
	{
	  if (bsFindTag(method_obj, source_key)                       /* is there a GFF_source tag?  */
	      && bsGetData(method_obj, source_key, _Text, &source)    /* can I get the data from it? */
	      && dictFind(conv_info->sources_dict, source, &unused))  /* can I find it in the set?   */
	    {
	      if (conv_info->include_sources == FALSE)
		result = TRUE ;
	    }
	  else
	    if (conv_info->include_sources == TRUE)
	      result = TRUE ;
	}
    }

  return result ;
}


/* Returns a standard string to help user identify the a particular homology record in the
 * database. */
char *fmapMakeHomolErrString(KEY tag2, KEY method, KEY homol,
                         int target1, int target2, int query1, int query2)
{
  char *err_str = NULL ;
  char *tag2_name, *method_name, *homol_name ;

  /* Must all be done separately because the nameXXX classes return ptrs to statics. */
  tag2_name = g_strdup(name(tag2)) ;
  method_name = g_strdup(name(method)) ;
  homol_name = g_strdup(nameWithClassDecorate(homol)) ;

  err_str = g_strdup_printf("%s %s %s (reference/match coords \"%d %d %d %d\") was not mapped, reason was:",
			    tag2_name, method_name, homol_name, 
			    target1, target2, query1, query2) ;


  g_free(homol_name) ;
  g_free(method_name) ;
  g_free(tag2_name) ;

  return err_str ;
}


void fmapDoAlignErrMessage(char *homol_str, SMapStatus status, KEY seq_key)
{
  char *source_name, *error ;
					    
  if (status == SMAP_STATUS_ERROR)
    error = "were not mapped" ;
  else
    error = "were clipped" ;
						    
  source_name = g_strdup(nameWithClassDecorate(seq_key)) ;
						    
  messerror("%s Align gaps %s. "
	    "This may be caused by invalid data, "
	    "check gaps in Align data with coords in parent "
	    " %s.",
	    homol_str, error,
	    source_name) ;
						    
  g_free(source_name) ;

  return ;
}



/* Only record features where the seg represents an object, not part of an object. */
static void makeMappedDict(DICT *dict, Array segs)
{
  int i ;

  for (i = 1 ; i < arrayMax(segs) ; ++i)
    {
      SEG *seg ;

      seg = arrp(segs, i, SEG) ;

      /* some quite yuch code here to deal with idiosyncracies of seg stuff.... */

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
      if ((seg->type != VISIBLE && seg->type != EMBL_FEATURE)
	  && ((seg->key) && seg->key != _CDS && seg->key != _Assembly_tags))
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */
      if ((seg->type != VISIBLE && seg->type != EMBL_FEATURE)
	  && ((seg->key) && seg->key != _Assembly_tags))

	{
	  BOOL result ;

	  result = fMapAddMappedDict(dict, seg->key) ;

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
	  if (result)
	    {
	      printf("added %s\n", buf->str) ;
	    }
	  else
	    {
	      printf("duplicate %s\n", buf->str) ;
	    }
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */
	}
    }

  return ;
}




/*
 *                       Some debug utilities.
 * 
 */


#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
static void segDumpToStdout(Array segs)
{
  ACEOUT dest ;

  dest = aceOutCreateToStdout(0) ;
  fmapDumpSegs(segs, dest) ;
  aceOutDestroy(dest) ;

  return ;
}
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */






/**********************************end of file**********************************************/	      

