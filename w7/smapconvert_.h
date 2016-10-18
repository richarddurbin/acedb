/*  File: smapconvert_.h
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
 * 	Richard Durbin (Sanger Centre, UK) rd@sanger.ac.uk, and
 *	Jean Thierry-Mieg (CRBM du CNRS, France) mieg@kaa.crbm.cnrs-mop.fr
 *
 * Description: Internal header for smapconvert functions
 *              
 *-------------------------------------------------------------------
 */
#ifndef _SMAPCONVERT__H_
#define _SMAPCONVERT__H_

#include <gtk/gtk.h>

#include <wh/method.h>
#include <wh/smap.h>
#include <wh/dict.h>
#include <wh/bitset.h>
#include <wh/methodcache.h>				    
#include <wh/smapconvert.h>
#include <w7/fmap_.h>


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


BOOL fmapIsRequired(SmapConvInfo *conv_info);
BOOL fmapFilterOut(SmapConvInfo *conv_info, KEY method);
void fmapFmapsMap2Seg(SMapKeyInfo *info, SEG *seg, int y1, int y2, SegType seg_type);
void fmapSetDefaultMethod(OBJ obj, SEQINFO *sinf);
void fmapSegSetClip(SEG *seg, SMapStatus status, SegType feature, SegType feature_up);
void fmapMakePairedReadSegs(Array segs, Array all_paired_reads, Array homol_info) ;
void fmapAddOldSegs (BOOL complement, Array segs, Array oldSegs, MethodCache mcache) ;
int  fmapSegOrder(void *a, void *b) ;
void fmapSMap2Seg(SMapKeyInfo *info, SEG *seg, int y1, int y2, SegType seg_type) ;
void fmapConvertCoding(SmapConvInfo *conv_info) ;
void fmapConvertAssemblyPath(SmapConvInfo *conv_info) ;
void fmapRemoveDuplicateIntrons(SmapConvInfo *conv_info) ;
void fmapRemoveSelfHomol(Array segs, Array homolinfo) ;
char *fmapMakeHomolErrString(KEY tag2, KEY method, KEY homol, int pos1, int pos2, int target1, int target2) ;
void fmapDoAlignErrMessage(char *homol_str, SMapStatus status, KEY seq_key) ;


#endif /* _SMAPCONVERT__H_ */


