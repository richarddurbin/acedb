/*  File: smapmethods.c
 *  Author: Gemma Barson <gb10@sanger.ac.uk>
 *  Copyright (C) 2012 Genome Research Ltd
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
 * 	Richard Durbin (Sanger Institute, UK) rd@sanger.ac.uk, and
 *	Jean Thierry-Mieg (CRBM du CNRS, France) mieg@kaa.crbm.cnrs-mop.fr
 *
 * Description: See smapmethods.h
 *              
 * Exported functions: See smapmethods.h
 *
 *-------------------------------------------------------------------
 */


#include <wh/smapmethods.h>
#include <wh/bindex.h>
#include <wh/pick.h>
#include <whooks/tags.h>
#include <whooks/classes.h>
#include <whooks/systags.h>
#include <w7/fmap_.h>
#include <w7/smapconvert_.h>


/* Local function declarations */
//static void findTagMethods(KEY tag, SmapConvInfo *conv_info, SmapCallback callback_func, gpointer callback_data);
static BOOL findObjMethods(SmapConvInfo *conv_info, SmapCallback callback_func, gpointer callback_data, const char *debug_info);
static BOOL findSequenceMethods(SmapConvInfo *conv_info, SmapCallback callback_func, gpointer callback_data);
static BOOL findFeatureObjMethods(SmapConvInfo *conv_info, SmapCallback callback_func, gpointer callback_data);
static void findHomolMethods(SmapConvInfo *conv_info, SmapCallback callback_func, gpointer callback_data) ;
static void findMethods(SmapConvInfo *conv_info, DICT *featDict, SmapCallback callback_func, gpointer callback_data) ;
static void findFeatureMethods(SmapConvInfo *conv_info, DICT *featDict, SmapCallback callback_func, gpointer callback_data);
static void findAlleleMethods(SmapConvInfo *conv_info, SmapCallback callback_func, gpointer callback_data);
static void findSplicesMethods(SmapConvInfo *conv_info, SmapCallback callback_func, gpointer callback_data);



/* 
 * EXTERNAL ROUTINES
*/ 

/* Entry routine to use smap to find methods */
BOOL sMapFindMethods(SMap *smap, KEY seqKey, KEY seqOrig,
                     int start, int stop, int length,
                     Array oldsegs, BOOL complement, MethodCache mcache,
                     Array dna,
                     KEYSET method_set, BOOL include_methods,
                     DICT *featDict, DICT *sources_dict, BOOL include_sources,
                     Array segs_inout,
                     Array seqinfo_inout, Array homolInfo_inout, Array featureinfo_inout,
                     BitSet homolBury_inout, BitSet homolFromTable_inout,
                     BOOL keep_selfhomols, BOOL keep_duplicate_introns,
                     STORE_HANDLE segsHandle, SmapCallback callback_func, gpointer callback_data)
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
              conv_info.method_set = method_set ;
              conv_info.method_cache = mcache ;
              conv_info.sources_dict = sources_dict;
              conv_info.include_methods = include_methods;
              conv_info.include_sources = include_sources; /* is this right? wasn't previously initialised?? */
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
              findMethods(&conv_info, featDict, callback_func, callback_data) ;

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


/* 
 * INTERNAL ROUTINES
*/

/* Find all methods in the given object and call the given callback on them */
static void findMethods(SmapConvInfo *conv_info, DICT *featDict, SmapCallback callback_func, gpointer callback_data)
{
  static KEY _Feature = KEY_UNDEFINED, _Transcribed_gene, _Clone_left_end, _Clone_right_end,
    _Confirmed_intron, _EMBL_feature, _Splices, _Method ;
  //int cds_min = 0 ;
  //BOOL exons_found = FALSE ;
  BOOL AllRequired = FALSE;
  BOOL parent_obj_created = FALSE ;

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
    parent_obj_created = findSequenceMethods(conv_info, callback_func, callback_data) ;
  else
    parent_obj_created = findFeatureObjMethods(conv_info, callback_func, callback_data) ;

  /* Only do this if the parent object was created. */
  if (parent_obj_created && AllRequired)
    {
//      if (bsFindTag(obj, str2tag("Source_Exons")))
//	findExonMethods(conv_info, callback_func, callback_data);
//  
//      if (bsFindTag(conv_info->obj, str2tag("CDS")))
//        findExonMethods(conv_info, callback_func, callback_data);
//
//      if (bsFindTag(obj, str2tag("Start_not_found")))
//	convertStart(conv_info, cds_min) ;
//
//      if (bsFindTag(obj, str2tag("End_not_found")))
//	convertEnd(conv_info) ;
}

  if (bsFindTag(conv_info->obj, _Homol))     /* homols need to look at methods at a deeper level */
    findHomolMethods(conv_info, callback_func, callback_data) ;
  
  if (bsFindTag(conv_info->obj, _Feature))
    findFeatureMethods(conv_info, featDict, callback_func, callback_data) ;

  if (AllRequired)
    {
//      if (bsFindTag(obj, _Visible))
//	convertVisible(conv_info) ;
//  
//      if (bsFindTag(obj, _Assembly_tags))
//	convertAssemblyTags(conv_info) ;
//  
      if (bIndexFind(conv_info->key, _Allele))
        {
          findAlleleMethods(conv_info, callback_func, callback_data) ;
        }
//
//      if (bsFindTag(obj, _Clone_left_end))
//	convertCloneEnd(conv_info, _Clone_left_end) ;
//
//      if (bsFindTag(obj, _Clone_right_end))
//	convertCloneEnd(conv_info, _Clone_right_end) ;
//
//      if (bsFindTag(obj, _Oligo))
//	convertOligo(conv_info) ;
    }

  //  if (bsFindTag (obj, _Confirmed_intron))
//    convertConfirmedIntrons(conv_info) ;
//
//  if (AllRequired)
//    {
//      if (bsFindTag(obj, _EMBL_feature))
//	convertEMBLFeature(conv_info) ;
//    }
//

  if (bsFindTag(conv_info->obj, _Splices))
    findSplicesMethods(conv_info, callback_func, callback_data) ;

  return ;
}


/* This function looks for a method tag in a given object and, if found, 
 * calls the given callback on the method. */
static BOOL findObjMethods(SmapConvInfo *conv_info, 
                           SmapCallback callback_func, 
                           gpointer callback_data,
                           const char *debug_info)
{
  BOOL result = FALSE;
  
  KEY method_key = str2tag("Method");
  KEY key = KEY_UNDEFINED;
    
  /* skip this one if filtering based on method and the method doesn't match. */
  if (bsGetKey(conv_info->obj, method_key, &key) && !fmapFilterOut(conv_info, key))
    {
      METHOD *meth = methodCacheGet(conv_info->method_cache, key, "") ;

      if (meth && callback_func)
        callback_func(meth, callback_data);

      result = TRUE;
    }

  return result;
}


///* This function creates an object for the given tag, looks for a method tag in 
// * that object and, if found, calls the given callback on the method. */
//static void findTagMethods(KEY tag, 
//                           SmapConvInfo *conv_info, 
//                           SmapCallback callback_func, 
//                           gpointer callback_data)
//{
//  OBJ obj = bsCreate(tag);
//
//  findObjMethods(obj, conv_info, callback_func, callback_data);
//  
//  bsDestroy(obj);
//}
//

/* Find methods for the sequence class object itself. This is not straight forward because
 * sometimes the sequence object is a feature itself (e.g. a transcript), sometimes it is
 * not a feature itself but is a container for other features (e.g. homols). This function
 * handles the former where the sequence object is a feature itself. */
static BOOL findSequenceMethods(SmapConvInfo *conv_info, SmapCallback callback_func, gpointer callback_data)
{
  BOOL result = FALSE ;

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


      result = findObjMethods(conv_info, callback_func, callback_data, "Sequence");
    }

  return result ;
}



/* This routine finds methods for objects that are themselves features as opposed to the features 
 * specified by the magic tag "Feature" within an object. */
static BOOL findFeatureObjMethods(SmapConvInfo *conv_info, SmapCallback callback_func, gpointer callback_data)
{
  BOOL result = FALSE ;
  KEY method_tag = str2tag("Method");

  /* Note, no method key, means no processing.... */
  if (bIndexTag(conv_info->key, method_tag))
    {
      SEG *seg ;
      SegType seg_type = MASTER;
      BOOL exons = FALSE, cds = FALSE ;
      int feature_index ;

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

      /* Set up a seg for this object, record it's inde fxor use by child segs. */
      conv_info->parent_seg_index = arrayMax(conv_info->segs_array) ;
      seg = arrayp(conv_info->segs_array, conv_info->parent_seg_index, SEG) ;
      seg->source = sMapParent(conv_info->info) ;
      seg->key = seg->parent = conv_info->key ;
      seg->data_type = SEG_DATA_UNSET ;

      fmapSMap2Seg(conv_info->info, seg, conv_info->y1, conv_info->y2, seg_type) ;
      fmapSegSetClip(seg, conv_info->status, seg_type, SEG_UP(seg_type)) ;

      result = findObjMethods(conv_info, callback_func, callback_data, "FeatureObj");
    }

  return result ;
}


/* Specific implementation of findObjMethods for objects that are homols */
static void findHomolMethods(SmapConvInfo *conv_info, SmapCallback callback_func, gpointer callback_data)
{
  SMapKeyInfo *info = conv_info->info ;
  OBJ obj = conv_info->obj ;
  BSMARK mark1 = 0, mark2 = 0, mark3 = 0, mark4 = 0, mark5 = 0, mark6 = 0 ;
  KEY method, homol, tag2 ;
  float score;
  int target1, target2, query1, query2;
  BOOL homol_mapped = FALSE, gaps_mapped = FALSE ;
     
  if (bsGetKeyTags(obj, _Homol, &tag2)) do 
    {
      mark1 = bsMark(obj, mark1) ;

      if (bsGetKey(obj, _bsRight, &homol)) do
	{ 
	  mark2 = bsMark(obj, mark2) ;

	  if (bsGetKey(obj, _bsRight, &method)) do
	    {
	      METHOD *meth ;

	      /* Get the method for this align. */

	      /* skip this one if filtering based on method and the method doesn't match. */
	      if (fmapFilterOut(conv_info, method))
		continue;

              meth = methodCacheGet(conv_info->method_cache, method, "") ;
	      messAssert(meth) ;

              /* We need to check that we actually have a homol before
               * we call the callback for this method. */
              gboolean found = FALSE;

	      mark3 = bsMark(obj, mark3);

	      if (!found && bsGetData(obj, _bsRight, _Float, &score)) do
		{
                  mark4 = bsMark(obj, mark4);
                  if (!found && bsGetData(obj, _bsRight, _Int, &target1)) do
                    { 
                      mark5 = bsMark(obj, mark5);
                      if (!found &&
                          bsGetData(obj, _bsRight, _Int, &target2) &&
                          bsGetData(obj, _bsRight, _Int, &query1)) do
                      {
                        mark6 = bsMark(obj, mark6);
                        
                        if (bsGetData(obj, _bsRight, _Int, &query2))
                          {
                            int y1 = 0, y2 = 0, clip_target1 = 0, clip_target2 = 0 ;
                            SMapStatus status ;
                            
                            /* Map the alignment sequence coords into reference sequence coord
                             * space, we only go on to process them if some part of the alignment is
                             * within the mapped space. */
                            status = sMapMap(info, target1, target2, &y1, &y2, &clip_target1, &clip_target2) ;
                            if (SMAP_STATUS_INAREA(status))
                              {
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
                                    found = TRUE;
                                  }
                                else
                                  {
                                    /* Report problems. */
                                    char *homol_str = fmapMakeHomolErrString(tag2, method, homol, target1, target2, query1, query2) ;
                                    
                                    if (status == SMAP_STATUS_MISALIGN || status == SMAP_STATUS_BADARGS)
                                      messerror("%s Error in data, check that Reference -> Match coords map onto each other, gaps data etc.", homol_str) ;
                                    else if (SMAP_STATUS_ISCLIP(status))
                                      messerror("%s Alignment was clipped by mapping but method does not have Allow_clipping tag set.", homol_str) ;
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
             
              /* Found a valid homol for this method, so call the callback */
              if (callback_func)
                callback_func(meth, callback_data);
              
            } while (bsGetKey(obj, _bsDown, &method));
          
	  bsGoto(obj, mark2);
	} while (bsGetKey(obj, _bsDown, &homol));
      
      bsGoto(obj, mark1);
    } while (bsGetKeyTags(obj, _bsDown, &tag2));
  
  
  /* Clear up. */
  bsMarkFree(mark1); bsMarkFree(mark2); bsMarkFree(mark3);
  bsMarkFree(mark4); bsMarkFree(mark5); bsMarkFree(mark6);

  /* necessary to ensure array long enough */
  //  bitExtend (conv_info->homolBury, arrayMax(conv_info->homol_info)) ;
  //bitExtend (conv_info->homolFromTable, arrayMax(conv_info->homol_info)) ;
  
 
  return ;
}


/* Extracts the method from the following model fragment, which can be embedded in any class:
 * 
 *	  Feature ?Method Int Int UNIQUE Float UNIQUE Text #Feature_info
 *		// Float is score, Text is note
 *		// note is shown on select, and same notes are neighbours
 *		// again, each method has a column double-click shows the method.
 *		// Absorb Assembly_tags?
 *
 */
static void findFeatureMethods(SmapConvInfo *conv_info, DICT *featDict, SmapCallback callback_func, gpointer callback_data)
{
  KEY key =  conv_info->key ;
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

	  if (class(u[0].k != _VMethod) || !u[1].i || !u[2].i)
	    continue;

	  /* skip this one if methods specified & it doesn't match
	   * Note that bsFlatten has created a one-dimensional array
	   * in which the 5 elements of the Feature tag are loaded
	   * into consecutive elements of the units array.  We don't
	   * use j to index into it as we point u at the set we want. */
	  if (!fmapFilterOut(conv_info, u[0].k))
	    {
              SMapStatus status = sMapMap(conv_info->info, u[1].i, u[2].i, &y1, &y2, NULL, NULL) ;
              if (!SMAP_STATUS_INAREA(status))
                continue;

              seg = arrayp(conv_info->segs_array, arrayMax(conv_info->segs_array), SEG);
              seg->source = key;
              seg->key = u[0].k;				    /* n.b. assigns Method obj to key. */
              if (tag2)
                seg->tag2 = tag2 ;
              
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
                seg->parent = 0;
              
              METHOD *meth = methodCacheGet(conv_info->method_cache, u[0].k, "") ;
              if (callback_func)
                callback_func(meth, callback_data);
            }
	}
    }

  return ;
}



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
static void findAlleleMethods(SmapConvInfo *conv_info, SmapCallback callback_func, gpointer callback_data)
{
  if (!(conv_info->is_Sequence_class))
    {
      /* non-sequence class objects. */
	{
          SEG *seg ;

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

     findObjMethods(conv_info, callback_func, callback_data, "Allele");
    }
  else
    {
      /* to do */
    }

  return ;
}


static void findSplicesMethods(SmapConvInfo *conv_info, SmapCallback callback_func, gpointer callback_data)
{
  static KEY _Predicted_5 = KEY_UNDEFINED, _Predicted_3 ;
  Array units = conv_info->units ;

  if (_Predicted_5 == KEY_UNDEFINED)
    {
      _Predicted_5 = str2tag("Predicted_5") ;
      _Predicted_3 = str2tag("Predicted_3") ;
    }
  
  OBJ obj = conv_info->obj;

  if (bsFlatten(obj, 5, units))
    {
      int i ;
      
      for (i = 0 ; i < arrayMax(units) ; i += 5)
	{
	  BSunit *u = arrayp(units, i, BSunit) ;

	  if ((u[0].k == _Predicted_5 || u[0].k == _Predicted_3) &&
	     class(u[1].k) == _VMethod && u[2].i && u[3].i)
	    {
              METHOD *meth = methodCacheGet(conv_info->method_cache, u[1].k, "") ;
              if (callback_func)
                callback_func(meth, callback_data);
            }
	}
    }

  return ;
}

