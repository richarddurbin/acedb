/*  file: smap.c
 *  Author: Richard Durbin (rd@sanger.ac.uk)
 *  Copyright (c) J Thierry-Mieg and R Durbin, 1999
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
 *	Jean Thierry-Mieg (CRBM du CNRS, France) mieg@kaa.crbm.cnrs-mop.fr
 *
 * Description: 
 * Exported functions: See smap.h
 * HISTORY:
 * Last edited: Jul 13 17:59 2012 (edgrif)
 * * Aug 26 20:20 (rd): added support for Source_exons
 * * Aug 23 21:15 (rd): added sMapDNA and changed mismatch semantics
 * * Aug 23 19:50 (rd): added bsIndexFind calls for efficiency
 * 			these are VERY IMPORTANT
 * * Aug 23 19:34 (rd): added sMapKeySet()
 * Created: Wed Jul 28 22:11:20 1999 (rd)
 * CVS info:   $Id: smap.c,v 1.81 2012-10-30 09:51:44 edgrif Exp $
 *-------------------------------------------------------------------
 */

#include <glib.h>

#include <wh/regular.h>
#include <wh/array.h>
#include <wh/bs.h>
#include <whooks/classes.h>
#include <whooks/systags.h>
#include <wh/dna.h>
#include <wh/bindex.h>
#include <wh/aceio.h>
#include <wh/lex.h>
#include <w6/smap_.h>
#include <wh/status.h>


/* The SMap package supports building a sequence coordinate system out
   of multiple objects according to the following model:

in parent object
----------------

  S_Child <tag2> <key> XREF <ctag2> UNIQUE Int UNIQUE Int #SMap_info
  	     // Ints are start, end in parent

  #SMap_info Align Int UNIQUE Int UNIQUE Int	
	     // for each ungapped block, parent_start child_start [length]
	     // starts are starts with respect to child (relevant if sequences 
	     //    are opposite orientations
	     // if no length then until next block (so no double gap) 
	     // if no Align assume ungapped starting at 1 in child
             AlignDNAPep Int UNIQUE Int UNIQUE Int
             AlignPepDNA Int UNIQUE Int UNIQUE Int
             // These two tags are analogous to Align, but scale length
	     // for the case of a dna alignment to peptide or vice-versa.
             Mismatch Int UNIQUE Int	
	     // start end of mismatch region in child coords
	     // mismatches from this sequence or children are ignored
	     // if no end then only the specified base
	     // if no Ints then mismatches OK anywhere in this sequence
             Contents  Homols_only
                       Features_only
                       No_DNA

You will find the Align info represented like this as well:

  #Homol_info Align Int UNIQUE Int UNIQUE Int 
	      // not needed if full alignment is ungapped
	      // one row per ungapped alignment section
              // first int is position in query, second in target
              // third int is length - only necessary when gaps in both sequences align





in child object
---------------

	SMap  S_Parent UNIQUE <ptag2> UNIQUE <key>    // must be just _one_ parent.
                              <ptag2> UNIQUE <key
	      S_Date UNIQUE DateType                  // NOT SUPPORTED AT THE MOMENT
	...
	Source_exons Int UNIQUE Int			// start end
	

Note that if there are Source_exons, the final mapping should be the product of
that given in the parent and the child.

Note also that there was a thought originally that we could do away with Source_exons
completely but this has not proved not to be the case for acedb, they are just too
ubiquitous and the performance issues of having an object per exon are perhaps not
encouraging.

*/


/*******************************************************************/


typedef BOOL (*KeyTestFunc)(KEY key) ;



#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
static void callTest(void) ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

static SMapKeyInfo *sMapAddNode(SMap *smap, SMapKeyInfo * parentInfo, KEY key,
				int pStart, int pEnd, OBJ parentObj, BOOL invert) ;
static void sMapAddNeighbours (SMap *smap, SMapKeyInfo *keyInfo, char *aqlCondition, 
			       BOOL parentDone);
static SMapKeyInfo *sMapNewInfo (SMap* smap, KEY key, KEY parent) ;
static void sMapInsertNewInfo(SMap* smap, KEY key, SMapKeyInfo *info) ;

static BOOL sMapAddDNA(Array dna, SMapKeyInfo *info, BOOL full_traverse,
		       sMapDNAErrorCallback callback, consMapDNAErrorReturn *errorLocation) ;
static BOOL sMapAddRawDNA(Array dna, SMapKeyInfo *info,
			  sMapDNAErrorCallback callback, consMapDNAErrorReturn *errorLocation) ;

static SMapStatus sMapMapEx(SMap *smap, int obj_length, Array map, int x1, int x2,
			    struct SMapMapReturn *ret) ;


static SMapStatus sMapLocalMap(STORE_HANDLE h, 
			       OBJ obj, KEY feature,
			       int p_start, int p_end, int c_start, int c_end,
			       SMapAlignMap align_map,
			       int *child_start_out) ;
static SMapStatus sMapReMap(int z1, int z2, int y1, int y2, SMapAlignMap align_map) ;
static SMapStatus sMapComposeMaps(SMap *smap, Array localMap, int parent_length, Array parentMap,
				  SMapConvType conv_type,
				  SMapStrand ref_strand, SMapStrand match_strand,
				  STORE_HANDLE handle, Array *new_map_out) ;
static void sMapInvertMap (Array map) ;
static SMapConvType sMapGetAlignType(int p_start_in, int p_end_in, int c_start_in, int c_end_in) ;
static SMapStatus sMapClipAlign(SMapConvType conv_type,
				SMapStrand *ref_strand_inout, SMapStrand *match_strand_inout,
				int p_start_in, int p_end_in,
				int *p_clipped_start_inout, int *p_clipped_end_inout,
				int c_start_in, int c_end_in,
				int *c_clipped_start_out, int *c_clipped_end_out) ;

static SMapStatus verifyLocalMap(Array local_map, int c_start, int c_end,
				 SMapStrand ref_strand, SMapStrand match_strand) ;
static SMapStatus verifyBlock(SMapMap *block, SMapStrand ref_strand, SMapStrand match_strand) ;
static SMapStatus verifyInterBlock(SMapMap *curr, SMapMap *next, SMapStrand ref_strand, SMapStrand match_strand) ;


SMapStrand str2Strand(char *str) ;
static BOOL hasNoParent(KEY key) ;
static int getLengthInternal(KEY key) ;
static int getLengthOfChildren(KEY key) ;
static int getLengthInParent(OBJ obj) ;
static SMapMap *lowestUpperSeg (Array map, int x) ;
static SMapMap *highestLowerSeg (Array map, int x) ;
static void exonsSort (Array a) ;
static Array getExonMap(KEY key);
BOOL getStartEndCoords(OBJ parent_obj, char *position_tag, KEY child,
		       int *pos1_out, int *pos2_out, BOOL report_errors) ;

static void sMapDumpInfo(SMapKeyInfo *info, ACEOUT dest, int offset, BOOL only_dna) ;

static BOOL findAssembly(Array assembly, SMapKeyInfo *info) ;
static BOOL addAssembly(Array assembly, SMapKeyInfo *info) ;

static BOOL getMapStrands(Array map, SMapStrand *ref_strand_out, SMapStrand *match_strand_out) ;



/*
 *                   External interface routines
 */



/******************************************************************/

int sMapMax (SMap* smap)
{
  return smap->length ;
}

/******************************************************************/

/* Return the smap node for the given key. */
SMapKeyInfo* sMapKeyInfo(SMap* smap, KEY key)
{
  SMapKeyInfo *info = NULL ;
  
  if (smap->lastKeyInfo && smap->lastKeyInfo->key == key)
    return smap->lastKeyInfo ;
  
  if (!assFind(smap->key2index, assVoid(key), &info))
    return NULL ;

  smap->lastKeyInfo = info ;

#ifdef ACEDB4
  if (smap->lastKeyUnspliced)
    arrayDestroy(smap->lastKeyUnspliced) ;
#endif

  return info ;
}


/******************************************************************/

/* Return parent, or NULL of root of tree. Note that this is parent
   wrt this smap: it may be a child if the mapping was traversed backwards! */
KEY sMapParent(SMapKeyInfo* info)
{
  return info->parent ;
}


/* Return Map array for this key */
Array sMapMapArray(SMapKeyInfo* info)
{
  return info->map ;
}

/* fundamental coordinate conversion routine
 * x1, x2, y1, y2 as sMapConvert()
 * nx1, nx2 are the clipped values that actually map to y1, y2 - for code simplicity 
 * these are always set, even if NO_OVERLAP status is returned (in which case they 
 * are x1, x2)
 *
 * return values are all a mess currently, if sMapMapEx() fails then y1 & y2 will be
 * RUBBISH and yet they are still returned...sigh...what is going on here...
 *
 * return value is status as above
 */
SMapStatus sMapMap(SMapKeyInfo* info, int x1, int x2, 
		   int *y1, int *y2, int *nx1, int *nx2)
{
  struct SMapMapReturn ret;
  SMapStatus status ;

  status = sMapMapEx(info->smap, info->length, info->map, x1, x2, &ret);

  if (y1)
    *y1 = ret.y1;
  if (y2)
    *y2 = ret.y2;
  if (nx1)
    *nx1 = ret.nx1;
  if (nx2)
    *nx2 = ret.nx2;

  return status ;
}


/* and its equivalent for mapping the other way */
/* as for sMapMap(), but maps in the reverse direction */
SMapStatus sMapInverseMap (SMapKeyInfo* info, int x1, int x2, 
			   int *y1, int *y2, int *nx1, int *nx2)
{
  struct SMapMapReturn ret;
  SMapStatus status ;

  if (!info->inverseMap)
    {
      info->inverseMap = arrayHandleCopy (info->map, info->smap->handle) ;
      sMapInvertMap (info->inverseMap) ;
    }

  status = sMapMapEx(info->smap, info->length, info->inverseMap, x1, x2, &ret);

  if (y1)
    *y1 = ret.y1;
  if (y2)
    *y2 = ret.y2;
  if (nx1)
    *nx1 = ret.nx1;
  if (nx2)
    *nx2 = ret.nx2;

  /* SMAP_STATUS_OUTSIDE_AREA may get set in sMapMapEx. This is bogus for 
     inverse maps since we are mapping into object coordinates and not 
     Map coordinates. Just reset that bit here. */
  return status & ~SMAP_STATUS_OUTSIDE_AREA;
}

/* Is the mapping from this key  to the root on the reverse strand.
 * True means that for mapping from x1,x2 to y1,y2 gives y1>y2 when x1<x2.
 * Note that you can discover this from a call to sMapMap except in 
 * the pathalogical case that x1==x2 or an odd map which returns y1==y2.
 * This function is supplied to cover those cases. */
BOOL sMapIsReverse(SMapKeyInfo *info, int x1, int x2)
{
  SMapStrand strand ;

  /* interpret zeroes */
  if (x1 == 0) 
    x1 = x2+1;
  else if (x2 == 0)
    x2 = x1+1;

  strand = x1 <= x2 ? STRAND_REVERSE : STRAND_FORWARD;

  return info->strand == strand;
}



/* NOTE that we require the object length to be supplied because objects may be clipped
 * during mapping, if this happens then we _cannot_ place any child features correctly within
 * them when those features are _reversed_.
 * This is because for forward features we know the object starts at 1 and so can always recover
 * this information, but for reversed features we cannot now reliably recover the length
 * as it is now clipped, i.e. we must be told it. */
static SMapStatus sMapMapEx(SMap *smap, int obj_length, Array map,
			    int x1, int x2, struct SMapMapReturn *ret)
{ 
  int z1, z2 ;
  SMapMap *m1, *m2 ;
  BOOL isReverse = FALSE ;
  SMapStatus status = SMAP_STATUS_PERFECT_MAP ; /* default is perfect map without gaps */

  /* Passing a coord of zero gives the maximum extent of this object */
  if (x1 == 0)
    x1 = obj_length ;
  if (x2 == 0)
    x2 = obj_length ;

  
  /* set default nx1, nx2 before possible reverse
   * this means they are set even if NO_OVERLAP, but that seems OK */
  if (ret)
    {
      ret->nx1 = x1 ; 
      ret->nx2 = x2 ;
    }

  /* Is this really ok ? What about if  x1 == x2  ?? */
  if (x1 > x2)
    {
      int t = x1 ;
      x1 = x2 ;
      x2 = t ;
      isReverse = TRUE ;
    }



  /* I'M NOT TOO COMFORTABLE WITH THIS, WHAT IF ->s1 and ->s2 ARE REVERSED ? */
  /* Are coords completely outside the supplied map ? */
  if (x1 > arrp(map, arrayMax(map)-1, SMapMap)->s2)	    /* off end of last mapped segment */
    {
      return SMAP_STATUS_X2_NO_OVERLAP_EXTERNAL ;
    }
  else if (x2 < arrp(map, 0, SMapMap)->s1)		    /* off start of first mapped segment */
    {
      return SMAP_STATUS_X1_NO_OVERLAP_EXTERNAL ;
    }

  /* find map segment for x1 by binary search */
  /* could cache in KeyInfo for rapid return on dense sorted features? */
  m1 = lowestUpperSeg (map, x1) ;

  if (x1 < m1->s1)		/* clip */
    {
      z1 = m1->r1 ;

      if (m1 == arrp(map, 0, SMapMap)) 
	status |= SMAP_STATUS_X1_EXTERNAL_CLIP ;
      else 
	status |= SMAP_STATUS_X1_INTERNAL_CLIP ;

      if (ret)
	{
	  if (isReverse)
	    ret->nx2 = m1->s1;
	  else
	    ret->nx1 = m1->s1 ;
	}
    }
  else if (m1->r2 > m1->r1)
    z1 = m1->r1 + (x1 - m1->s1) ;
  else
    z1 = m1->r1 - (x1 - m1->s1) ;


  /* x2 likewise */
  m2 = highestLowerSeg (map, x2) ;

  /* completely contained in internal gap */
  if (m2 < m1)
    {
      return SMAP_STATUS_NO_OVERLAP_INTERNAL ;
    }
    
  if (m1 < m2)
    status |= SMAP_STATUS_INTERNAL_GAPS ;
  
  if (x2 > m2->s2)		/* clip */
    { z2 = m2->r2 ;
      if (m2 == arrp(map, arrayMax(map)-1, SMapMap))
	  status |= SMAP_STATUS_X2_EXTERNAL_CLIP ;
      else 
	  status |= SMAP_STATUS_X2_INTERNAL_CLIP ;
      
      if (ret)
	{
	  if (isReverse)
	    ret->nx1 = m2->s2 ;
	  else
	    ret->nx2 = m2->s2 ;
	}
    }
  else if (m2->r2 > m2->r1)
    z2 = m2->r2 - (m2->s2 - x2) ;
  else
    z2 = m2->r2 + (m2->s2 - x2) ;

  /* If an smap was also supplied then check whether coords are also completely outside
   * requested area, note that its possible for coords to be successfully mapped but
   * still be _outside_ the requested area. */
  if (smap)
    {
      if (((z1 <= z2) && (z2 < smap->area1))
	  || ((z1 <= z2) && (z1 > smap->area2))
	  || ((z1 > z2) && (z1 < smap->area1))
	  || ((z1 > z2) && (z2 > smap->area2)))
	status |= SMAP_STATUS_OUTSIDE_AREA;
    }

  /* done, now fill in results and return */
  if (ret)
    {
      if (isReverse)
	{ 
	  ret->y1 = z2 ;
	  ret->y2 = z1 ;
	}
      else
	{
	  ret->y1 = z1 ; 
	  ret->y2 = z2 ;
	}
    }

  return status;
}

/* Return human readable error as static string.
 * Note that at most one bit should be set in status, if more 
 * than one is set in your result, call this multiple times and 
 * cat the return values to taste. May return NULL for unused bits.
 * 
 * Used to provide string versions of return codes for a text based/command line interface to
 * smap (see "gif smap" command in gifcommand.c). */
char *sMapErrorString(SMapStatus status)
{
  char *result ;

  switch (status)
    {
    case SMAP_STATUS_PERFECT_MAP:
      result = "PERFECT_MAP"; break ;
    case SMAP_STATUS_ERROR:
      result = "ERROR"; break ;
    case SMAP_STATUS_INTERNAL_GAPS:
      result = "INTERNAL_GAPS";  break ;
    case SMAP_STATUS_OUTSIDE_AREA:
      result = "OUTSIDE_AREA";  break ;

    case SMAP_STATUS_X1_NO_OVERLAP_EXTERNAL:
      result = "X1_NO_OVERLAP_EXTERNAL"; break ;
    case SMAP_STATUS_X2_NO_OVERLAP_EXTERNAL:
      result = "X2_NO_OVERLAP_EXTERNAL"; break ;
    case SMAP_STATUS_NO_OVERLAP_INTERNAL:
      result = "NO_OVERLAP_INTERNAL"; break ;

    case SMAP_STATUS_X1_EXTERNAL_CLIP:
      result = "X1_EXTERNAL_CLIP"; break ;
    case SMAP_STATUS_X1_INTERNAL_CLIP:
      result = "X1_INTERNAL_CLIP"; break ;
    case SMAP_STATUS_X2_EXTERNAL_CLIP:
      result = "X2_EXTERNAL_CLIP"; break ;
    case SMAP_STATUS_X2_INTERNAL_CLIP:
      result = "X2_INTERNAL_CLIP"; break ;

    default:
      result = NULL; break ;
    }

  return result ;
}


/************************************************************/

/* standard coordinate conversion routine for a full SMap object */



/* Converts interval x1..x2 in key's coordinate system to y1..y2 in
   smap's.  Returns TRUE if succeeds, FALSE if e.g. key not known in smap,
   or both x1, x2 out of range (on the same side), or both fall in the same
   gap in the map.  If part of the interval is out of range then it clips. */
BOOL sMapConvert (SMap* smap, KEY key, int x1, int x2, int *y1, int *y2)
{
  BOOL result = FALSE ;
  SMapKeyInfo *info;
  struct SMapMapReturn ret;
  
  if ((info = sMapKeyInfo (smap, key))
      && (sMapMapEx(info->smap, info->length, info->map, x1, x2, &ret)
	  & SMAP_STATUS_NO_OVERLAP) == 0)
    {
      if (y1)
	*y1 = ret.y1 ;
      if (y2)
	*y2 = ret.y2 ;
      
      result = TRUE ;
    }
  
  return result ; 
}


#ifdef ACEDB4
/* As for sMapMap, but for objects with source exons,
   the conversion is from the unspliced co-ordinate system.
   Converts interval x1..x2 in key's coordinate system to y1..y2 in
   smap's.  Returns SMapStatus as above, note well that it will clip
   coords so you may end up with the returned y1,y2 being identical.
   Beware that this call is _more costly than sMapMap(). */
/* Map into unspliced co-ordinates even if an object has source exons.
   Currently first call on each object costs a bsCreate/Destroy 
   and two arrayCreate/Destroys.
   We could make this cheaper at the cost of holding more data in every node.
*/
SMapStatus sMapUnsplicedMap(SMapKeyInfo* info, int x1, int x2, 
			    int *y1, int *y2, int *nx1, int *nx2)
     
{
  SMapStatus result = SMAP_STATUS_ERROR ;
  SMap *smap = info->smap;
  struct SMapMapReturn ret;
  Array exonMap, unsplicedMap = smap->lastKeyUnspliced ;
  
  if (!unsplicedMap)
    {
      /* If no source exons we just do ordinary map */
      if (!(exonMap = getExonMap(info->key)))
	{
	  unsplicedMap = info->map;
	}
      else
	{
	  SMapStrand exon_ref_strand, exon_match_strand ;

	  /* spliced -> unspliced */
	  sMapInvertMap(exonMap);

	  if (getMapStrands(exonMap, &exon_ref_strand, &exon_match_strand))
	    {
	      /* root -> spliced -> unspliced */
	      result = sMapComposeMaps(smap, exonMap,
				       info->length, info->map,
				       SMAPCONV_DNADNA, exon_ref_strand, exon_match_strand,
				       smap->handle, &unsplicedMap) ;

	      smap->lastKeyUnspliced = unsplicedMap ;
	    }

	  arrayDestroy(exonMap);
	}
    }

  if (unsplicedMap)
    result = sMapMapEx(info->smap, info->length, unsplicedMap, x1, x2, &ret) ;
  
  if (y1)
    *y1 = ret.y1;
  if (y2)
    *y2 = ret.y2;
  if (nx1)
    *nx1 = ret.nx1;
  if (nx2)
    *nx2 = ret.nx2;
    
  return result ;
}
#endif /* ACEDB4 */

/*************************************************************/

static void sMapKeyAdd (KEYSET kset, SMapKeyInfo *info)
{
  int i ;

  keySet(kset, keySetMax(kset)) = info->key ;
  for (i = 0 ; i < arrayMax (info->children) ; ++i)
    {
      sMapKeyAdd (kset, arr(info->children, i, SMapKeyInfo*)) ;
    }

  return ;
}

/* this gives all keys that map onto this smap */
KEYSET sMapKeys (SMap* smap, STORE_HANDLE handle)
{
  KEYSET kset = keySetHandleCreate (handle) ;

  sMapKeyAdd (kset, smap->root) ;

  return kset ;
}


/*************************************************************/


/* Given an smap will return the assembly information used to construct
 * the virtual sequence and dna for that smap.
 * 
 * Returns an array of SMapAssemblyStruct, one per contiguous section used
 * in the mapping i.e. objects may appear more than once in the array.
 * 
 * Returns NULL if assembly could not be determined.
 *  */
Array sMapAssembly(SMap* smap, STORE_HANDLE handle)
{
  Array assembly ;

  assembly = arrayCreate(20, SMapAssemblyStruct) ;

  if (!findAssembly(assembly, smap->root))
    {
      arrayDestroy(assembly) ;
      assembly = NULL ;
    }

  return assembly ;
}


/* Given an smap, will return the dna sequence for that smap.
 * If the dna can be found it is returned as an array allocated on handle,
 * otherwise NULL is returned.
 * NOTE that there is no requirement for the dna to be complete, there may
 * be holes in the array which will be exported as a series of "-" chars.
 * 
 * sMapDNAErrorCallback may be NULL, which is equivalent to a callBack function which
 * always returns sMapErrorReturnFail.
 * 
 * NB sMapDNA is called by dnaNewGet() if there is no directly
 * attached DNA, and it calls dnaGet to get the dna for keys
 * with directly attached DNA.  We have been careful to ensure 
 * there is no loop here.
 */
Array sMapDNA(SMap* smap, STORE_HANDLE handle, sMapDNAErrorCallback callback)
{
  Array dna ;
  int dna_length ;
  BOOL full_traverse = FALSE ;				    /* Stop at first obj with DNA. */
  consMapDNAErrorReturn errorLocation = sMapErrorReturnContinue ;

  /* The dna length comes from the requested "area" of the smap, this would
   * correspond to the calculated area in say the fmap display. */
  dna_length = smap->area2 - smap->area1 + 1 ;

  /* this is where we build the result */ 
  dna = arrayHandleCreate(dna_length, unsigned char, handle) ;
  
  /* recursively add DNA */
  if (sMapAddDNA(dna, smap->root, full_traverse, callback, &errorLocation) &&
      errorLocation != sMapErrorReturnFail &&
      errorLocation != sMapErrorReturnContinueFail)
    {
      /* now fill in low byte from high byte if necessary */
      int i ;
      unsigned char *cp = arrp(dna, 0, unsigned char) ;
      for (i = 0 ; i < arrayMax(dna) ; i++, cp++)
	{
	  if (*cp & 0xf)
	    *cp &= 0xf ;
	  else
	    *cp >>= 4 ;
	}

      /* We must reset the arrayMax of the dna array to be original array
       * length because sMapAddDNA() leaves it as the last position that
       * it found dna for, we need to return the whole array. */
      arrayMax(dna) = dna_length ;
    }
  else
    {
      arrayDestroy(dna) ;
      dna = NULL ;
    }

  return dna ;
}


/* We return TRUE if the object itself contains dna or if any of its
 * children contain dna, otherwise FALSE.
 */
static BOOL sMapAddDNA(Array dna, SMapKeyInfo *info, BOOL full_traverse,
		       sMapDNAErrorCallback callback, 
		       consMapDNAErrorReturn *errorLocation)
{
  BOOL result = FALSE ;
  int i ;
  

  if (bIndexTag(info->key, str2tag("DNA")))
    {
      /* full_traverse means find all DNA in parents and their children and munge it together,
       * otherwise we stop traversing down as soon as find dna in an object and do not look
       * in its children.  */
      if (!full_traverse)
	{
	  return sMapAddRawDNA(dna, info, callback, errorLocation);
	}
      else
	{
	  if (sMapAddRawDNA(dna, info, callback, errorLocation))
	    result = TRUE ;
	}
    }

  /* This sequence has no dna directly attached so look in children.     */
  for (i = 0 ; i < arrayMax(info->children) ; ++i)
    {
      /* N.B. we return TRUE if one or more of the children contain dna. */
      if (sMapAddDNA(dna, arr(info->children, i, SMapKeyInfo*), full_traverse, 
		     callback, errorLocation))
	result = TRUE ;
    }
  
  return result;
}

static BOOL sMapAddRawDNA(Array dna, SMapKeyInfo *info,
			  sMapDNAErrorCallback callback, 
			  consMapDNAErrorReturn *errorLocation)
{
  /* Try and get this objects dna. */
  OBJ obj = NULL ;
  Array a = NULL ;
  KEY key = KEY_UNDEFINED ;
  unsigned char *cp, *cq ;
  int i, j, length ;
  int area1 = info->smap->area1;
  int area2 = info->smap->area2;

  if (!(obj = bsCreate (info->key)))
    return FALSE;
  
  if (!bsGetKey (obj, str2tag("DNA"), &key)
      || !(a = dnaRawGet(key)))
    {
      bsDestroy(obj);
      return FALSE;
    }
    
  /* take minimum of declared and actual lengths of DNA */
  if (!bsGetData (obj, _bsRight, _Int, &length) || (length > arrayMax(a)))
    length = arrayMax(a);
  if (length < 1)
    {
      arrayDestroy(a);
      bsDestroy(obj);
      return FALSE; /* don't have to worry about zero length later */
    } 
     
  /* shift sequence allowed to mismatch into high byte */
  if (info->mismatch)
    for (i = 0 ; i < arrayMax (info->mismatch) ; ++i)
      {
	SMapMismatch *mis = arrp(info->mismatch, i, SMapMismatch) ;
	for (j = mis->s1 - 1 ; (j < mis->s2 - 1) && (j < length) ; ++j)
	  arr(a,j,unsigned char) <<= 4 ;
      }
  /* strategy is to complain if low half-bytes don't match 
     then OR with new sequence, allowing high byte ambiguity codes */
  
  for (i = 0 ; i < arrayMax (info->map) ; ++i)
    {
      SMapMap *map = arrp(info->map, i, SMapMap);
      int s1 = map->s1;
      int s2 = map->s2;
      int r1 = map->r1;
      int r2 = map->r2;
  
      if (info->strand == STRAND_FORWARD)
	{
	  /* May not have enough DNA: adjust. */
	  if (s2 > length)
	    {
	      r2 -= s2 - length;
	      s2 = length;
	    }
	  
	  /* Now truncate based on Area and adjust r-coords to 
	     take it into account. area1 <= area2 always */
	  if (r1 < area1)
	    {
	      s1 += area1 - r1;
	      r1 = area1;
	    }
	  if (r2 > area2)
	    {
	      s2 -= r2 - area2;
	      r2 = area2;
	    }
	  
	  if (s1 > s2) /* no overlap with area at all. */
	    continue;

	  {
	    int max_length = r2 - area1 + 1 ;

	    if (arrayMax(dna) < max_length)
	      arrayMax(dna) = max_length ;
	  }

	  cp = arrp(dna, (r1 - area1), unsigned char) ;
	  cq = arrp(a, (s1 - 1), unsigned char) ;

	  for (j = 0; j <= s2-s1 ; j++)
	    {
	      if ((*cp & 0xf) && (*cp & 0xf) != (*cq & 0xf) &&
		  (*errorLocation == sMapErrorReturnContinue ||
		   *errorLocation == sMapErrorReturnContinueFail))
		*errorLocation = 
		  callback ? (*callback)(key,  s1 + j) : sMapErrorReturnFail;
	      *cp++ |= *cq++ ;
	    }
	}
      else
	{ /* up-strand version. */ 

	  /* May not have enough DNA: adjust. */
	  if (s2 > length)
	    {
	      r2 += s2 - length;
	      s2 = length;
	    }
	  
	  /* Now truncate based on Area and adjust r-coords to 
	      take it into account. area1 <= area2 always */
	  if (r2 < area1)
	    {
	      s2 -= area1 - r2;
	      r2 = area1;
	    }
	  if (r1 > area2)
	    {
	      s1 += r1 - area2;
	      r1 = area2;
	    }
	  
	  if (s1 > s2) /* no overlap with area at all. */
	    continue;
	  
	  if (arrayMax(dna) < r1 - area1 + 1)
	    arrayMax(dna) = r1 - area1 + 1;
	  cp = arrp(dna, (r1 - area1), unsigned char) ;
	  cq = arrp(a, (s1 - 1), unsigned char) ;

	  for (j = 0; j <= s2-s1 ; j++)
	    {
	      unsigned char base = complementBase[(int)(*cq & 0xf)] ;
	      if ((*cp & 0xf) && (*cp & 0xf) != base &&
		  (*errorLocation == sMapErrorReturnContinue ||
		   *errorLocation == sMapErrorReturnContinueFail))
		*errorLocation = 
		  callback ? (*callback)(key,  s1 + j) : sMapErrorReturnFail;
	      *cp-- |= (complementBase[(int)(*cq++ >> 4)] << 4) | base;
	    }
	}

    }

  bsDestroy (obj) ;
  arrayDestroy (a) ;
  
  return TRUE;
}






/*************************************************************/
/************ now the stuff to build smaps *******************/




/******************************/
/* For a given span find the sequence object that contains that span
 * without gaps. If sMapSpanPredicate is non-NULL, it gets used to filter the output. */
/* Finds smallest containing smap'd object that fits the predicate specified
 * by predicate function, or any smallest containing smap'd object if no
 * predicate.
 * 
 * This is richards comment about this routine when it was in fmapcontrol.c
 *
 *  "Mar 13 22:55 2002 (rd): fixed fMapFindSpan() to use sMapInverseMap() not SEGs
 *		and removed fMapFindZoneFather(), replacing calls with fMapFindSpan()"
 *
 * You should NOTE WELL that smap coords run 1 -> length (i.e. do not start at 0)
 *
 */
BOOL sMapFindSpan(SMap *smap, KEY *keyp, 
		  int *start, int *end,
		  sMapSpanPredicate predicate)
{
  BOOL found ;
  KEYSET ks ;
  int    i, min = INT_MAX ;
  int    x, y, xResult=0, yResult=0 ;
  KEY    key, result = KEY_UNDEFINED ;
  SMapStatus status ;
  SMapKeyInfo *info ;

  messAssert(smap != NULL && keyp != NULL && start != NULL && end != NULL) ;

  ks = sMapKeys(smap, 0) ;

  for (i = 0 ; i < keySetMax(ks) ; ++i)
    {
      int key_len ;

      key = keySet (ks, i) ;
      if (predicate && !(*predicate)(key))
        continue ;

      info = sMapKeyInfo (smap, key) ;

      status = sMapInverseMap (info, *start, *end, &x, &y, 0, 0) ;
      if (status == SMAP_STATUS_PERFECT_MAP && (key_len = sMapLength(key)) < min)
	{
	  result = key ;
	  xResult = x ;
	  yResult = y ;
	  min = key_len ;
	}
    }

  keySetDestroy(ks) ;

  if (!result)
    found = FALSE ;
  else
    {
      *keyp = result ;
      *start = xResult ;
      *end = yResult ;
      found = TRUE ;
    }

  return found ;
}



/******************************/


/* This function takes the original parent and child mapping and a clipped parent mapping.
 * It then clips the child mapping to match the clipped parent. For peptides this may mean
 * readjusting the parent clipping to map to whole codon boundaries.
 * 
 * If the strands are STRAND_INVALID then it does the best it can to set them. */
static SMapStatus sMapClipAlign(SMapConvType conv_type,
				SMapStrand *ref_strand_inout, SMapStrand *match_strand_inout,
				int p_start_in, int p_end_in,
				int *p_clipped_start_inout, int *p_clipped_end_inout,
				int c_start_in, int c_end_in,
				int *c_clipped_start_out, int *c_clipped_end_out)
{
  SMapStatus status = SMAP_STATUS_ERROR ;
  SMapStrand ref_strand, match_strand ;

  messAssert(p_start_in > 0 && p_end_in > 0
	     && p_clipped_start_inout && *p_clipped_start_inout > 0
	     && p_clipped_end_inout && *p_clipped_end_inout > 0) ;
  messAssert(c_start_in > 0 && c_end_in > 0
	     && c_clipped_start_out && c_clipped_end_out) ;


  
  if ((ref_strand = *ref_strand_inout) == STRAND_INVALID)
    {
      if (p_start_in <= p_end_in)
	ref_strand = STRAND_FORWARD ;
      else
	ref_strand = STRAND_REVERSE ;

      *ref_strand_inout = ref_strand ;
    }

  if ((match_strand = *match_strand_inout) == STRAND_INVALID)
    {
      if (c_start_in <= c_end_in)
	match_strand = STRAND_FORWARD ;
      else
	match_strand = STRAND_REVERSE ;

      *match_strand_inout = match_strand ;
    }


  if (p_start_in == *p_clipped_start_inout && p_end_in == *p_clipped_end_inout)
    {
      /* coords are not clipped so just fill in child clip coords from child coords,
       * this is worth doing because most alignments will not be clipped. */

      *c_clipped_start_out = c_start_in ;
      *c_clipped_end_out = c_end_in ;

      status = SMAP_STATUS_PERFECT_MAP ;
    }
  else
    {
      int DNAfac, pepFac ;
      int p_start, p_end, p_start_clip, p_end_clip, p_offset ;
      int c_start, c_end, c_start_clip, c_end_clip, c_offset ;

      switch(conv_type)
	{
	case SMAPCONV_DNADNA :
	  {
	    DNAfac = 1 ;
	    pepFac = 1 ;
	    break ;
	  }
	case SMAPCONV_DNAPEP:
	  {
	    DNAfac = 3 ;
	    pepFac = 1 ;
	    break ;
	  }
	case SMAPCONV_PEPDNA:
	  {
	    DNAfac = 1 ;
	    pepFac = 3 ;
	    break ;
	  }
	default:
	  {
	    messcrash("Bad smap align conversion type.") ;
	    break ;
	  }
	}

      /* Coords are reversed for reverse strand so normalise them and calculate offsets. */
      if (ref_strand == STRAND_FORWARD)
	{
	  p_start = p_start_in ;
	  p_end = p_end_in ;
	  p_start_clip = *p_clipped_start_inout ;
	  p_end_clip = *p_clipped_end_inout ;
	}
      else
	{
	  p_start = p_end_in ;
	  p_end = p_start_in ;
	  p_start_clip = *p_clipped_end_inout ;
	  p_end_clip = *p_clipped_start_inout ;
	}
      p_offset = p_start - 1 ;
      p_start_clip -= p_offset ;
      p_end_clip -= p_offset ;

      if (match_strand == STRAND_FORWARD)
	{
	  c_start = c_start_in ;
	  c_end = c_end_in ;
	  c_start_clip = 0 ;
	  c_end_clip = 0 ;
	}
      else
	{
	  c_start = c_end_in ;
	  c_end = c_start_in ;
	  c_start_clip = 0 ;
	  c_end_clip = 0 ;
	}
      c_offset = c_start - 1 ;


      /* Calculate where parent clip coords project into child,
       * easy for DNA match but must take account of codons for peptide match. */
      c_start_clip = ( ( (p_start_clip  + (DNAfac + 1)) / DNAfac)  * pepFac) ;
      c_end_clip =   ( ( (p_end_clip                  ) / DNAfac)  * pepFac) ;


      /* Now project child clip back into parent. */
      p_start_clip = ( ((c_start_clip * DNAfac) - (DNAfac - 1)) / pepFac) + p_offset ;
      p_end_clip   = ( ((c_end_clip   * DNAfac)               ) / pepFac) + p_offset ;


      /* You might think at this point that for peptide matches some adjustment should
       * be made according to which reference strand the peptide is mapped to. In fact
       * this is not so because the match is a complete number of codons therefore
       * the strand is irrelevant. */


      /* And return reversing coordinate order as needed. */
      c_start_clip += c_offset ;
      c_end_clip += c_offset ;
      if (match_strand == STRAND_FORWARD)
	{
	  *c_clipped_start_out = c_start_clip ;
	  *c_clipped_end_out = c_end_clip ;
	}
      else
	{
	  *c_clipped_start_out = c_end_clip ;
	  *c_clipped_end_out = c_start_clip ;
	}

      p_start_clip += p_offset ;
      p_end_clip += p_offset ;
      if (ref_strand == STRAND_FORWARD)
	{
	  *p_clipped_start_inout = p_start_clip ;
	  *p_clipped_end_inout = p_end_clip ;
	}
      else
	{
	  *p_clipped_start_inout = p_end_clip ;
	  *p_clipped_end_inout = p_start_clip ;
	}

      status = SMAP_STATUS_PERFECT_MAP ;
    }

  return status ;
}


/* Given parent and child start/ends return the alignment type, returns SMAPCONV_INVALID if
 * lengths not exactly right. */
static SMapConvType sMapGetAlignType(int p_start_in, int p_end_in, int c_start_in, int c_end_in)
{
  SMapConvType conv_type = SMAPCONV_INVALID ;
  int parent_length, child_length ;


  parent_length = abs(p_end_in - p_start_in) + 1 ;
  child_length = abs(c_end_in - c_start_in) + 1 ;

  if (parent_length == child_length)
    conv_type = SMAPCONV_DNADNA ;
  else if (parent_length / child_length == 3 && parent_length % child_length == 0)
    conv_type = SMAPCONV_DNAPEP ;
  else if (child_length / parent_length == 3 && child_length % parent_length == 0)
    conv_type = SMAPCONV_PEPDNA ;

  return conv_type ;
}



/* Remaps local_map from y1,y2 to z1,z2. Note that this may result in local_map being
 * clipped if it is longer than the span of these coords (which must be the same).
 * If successful, local_map_inout is _REPLACED_ with the new map.
 * 
 * NOTE how we use sMapComposeMaps() to do any clipping and detect any coord problems.
 * 
 * 
 *  */
static SMapStatus sMapReMap(int z1, int z2, int y1, int y2, SMapAlignMap align_map)
{
  SMapStatus status = SMAP_STATUS_PERFECT_MAP ;
  Array product_map = NULL ;
  Array parent_map ;
  int obj_length ;
  SMapMap *m ;
  SMapConvType conv_type = align_map->conv_type ;
  Array *local_map_inout = &(align_map->gap_map) ;


  /* Could optimise here by checking to see if array is already in parent coords... */


  obj_length = abs(z2 - z1) + 1 ;

  /* Make a map out of the parent coords to which we wish to map the local_map to. */
  parent_map = arrayCreate (1, SMapMap) ;
  m = arrayp(parent_map, 0, SMapMap) ;

  /* I don't feel comfortable about this, sMapComposeMaps() calls SMapMapEx() which
   * requires that m->s1 < m->s2 otherwise it fails...this requires further investigation... */
  if (y1 < y2)
    {
      m->r1 = z1 ;
      m->r2 = z2 ;
      m->s1 = y1 ;
      m->s2 = y2 ;
    }
  else
    {
      m->r1 = z2 ;
      m->r2 = z1 ;
      m->s1 = y2 ;
      m->s2 = y1 ;
    }


  /* Now "compose" the parent and local maps, i.e. put local_map into parent maps
   * coord frame. */
  status = sMapComposeMaps(NULL, *local_map_inout, obj_length, parent_map,
			   conv_type, align_map->ref_strand, align_map->match_strand,
			   0, &product_map) ;

  if (product_map)
    {
      arrayDestroy(parent_map) ;
      arrayDestroy(*local_map_inout) ;
      *local_map_inout = product_map ;
    }

  return status ;
}


/* Takes an alignment and maps it into the coordinate frame of the reference
 * sequence. The mapping must take account of any clipping given by
 * p_clipped_start_inout/p_clipped_end_inout, this may result in these
 * coordinates being adjusted to codon boundaries if the mapping is peptide
 * to DNA.
 * 
 * If there is no local gaps array then the mapping is simply a clip of the
 * coordinates. If there is a local gaps array this must be mapped into
 * the coordinate frame of the reference sequence and clipped. Clipping the
 * gaps array may result in quite large changes to the final coords if whole
 * sub blocks are excluded.
 * 
 * Note align_map_out struct is passed in by caller so can be allocated by caller
 * for efficiency as number of homols mapped may be large.
 */
SMapStatus sMapMapAlign(STORE_HANDLE handle,
			OBJ obj, KEY feature, BOOL allow_misalign, BOOL map_gaps,
			int ref_start, int ref_end,
			int p_start, int p_end,
			int *p_clipped_start_inout, int *p_clipped_end_inout,
			int c_start, int c_end,
			int *c_clipped_start_out, int *c_clipped_end_out,
			SMapAlignMap align_map_out)
{
  SMapStatus status = SMAP_STATUS_PERFECT_MAP ;
  Array map = NULL ;
  int child_start_out ;
  SMapAlignMapStruct align_map = {STRAND_INVALID} ; 


  if (p_start < 1 || p_end < 1
      || !p_clipped_start_inout || *p_clipped_start_inout < 1
      || !p_clipped_end_inout || *p_clipped_end_inout < 1
      || c_start < 1 || c_end < 1
      || !c_clipped_start_out || !c_clipped_end_out)
    {
      return SMAP_STATUS_BADARGS ;
    }


  /* If there is AlignXXX data, map the alignment array into p_start, p_end. NOTE that
   * we have to do this prior to clipping because the coords we read from the database
   * will not be clipped. This step should map perfectly otherwise there is an error
   * in the database. */
  if (map_gaps && (status = sMapLocalMap(handle,
					 obj, feature,
					 p_start, p_end, c_start, c_end,
					 &align_map, &child_start_out)) == SMAP_STATUS_PERFECT_MAP)
    {
      /* If aligns are mapped then clip the array reference/match coordinates
       * to p_clipped_start, p_clipped_end.*/
      status = sMapReMap(ref_start, ref_end,
			 *p_clipped_start_inout, *p_clipped_end_inout,
			 &align_map) ;

      /* All ok ? Then return the align_map. */
      if (status == SMAP_STATUS_PERFECT_MAP || (status & SMAP_STATUS_CLIP))
	{
	  *align_map_out = align_map ;
	}
      else
	{
	  /* Check error handling here now we changed code..... */
	  if (map)
	    arrayDestroy(map) ;
	}
    }
  else
    {
      /* No AlignXXX data. */
      static KEY Align_id = KEY_NULL ;
      char *align_id = NULL ;


      /* Check for other tags in sub-model... */
      if (Align_id == KEY_NULL)
	Align_id = str2tag("Align_id") ;

      if (bsFindTag(obj, Align_id))
	{
	  bsGetData(obj, Align_id, _Text, &align_id) ;
	  
	  align_map_out->align_id = strnew(align_id, 0) ;
	}


      if ((align_map_out->conv_type = sMapGetAlignType(p_start, p_end, c_start, c_end)) == SMAPCONV_INVALID
	  && !allow_misalign)
	{
	  status = SMAP_STATUS_MISALIGN ;
	}
      else
	{
	  SMapStrand ref_strand = STRAND_INVALID, match_strand = STRAND_INVALID ;

	  /* No align information so clip the overall alignment. */
	  if ((status = sMapClipAlign(align_map_out->conv_type,
				      &ref_strand, &match_strand,
				      p_start, p_end,
				      p_clipped_start_inout, p_clipped_end_inout,
				      c_start, c_end,
				      c_clipped_start_out, c_clipped_end_out)) == SMAP_STATUS_PERFECT_MAP)
	    {
	      align_map_out->ref_strand = ref_strand ;
	      align_map_out->match_strand = match_strand ;
	    }
	}
    }


  return status ;
}


/* Retrieves align string info without verifying/mapping it. */
SMapStatus sMapFetchAlignStr(OBJ obj, SMapAlignMap align_map_out)
{
  SMapStatus status = SMAP_STATUS_NO_DATA ;
  static KEY Match_string = KEY_NULL ;
  BSMARK mark ;
  SMapStrand ref_strand = STRAND_INVALID, match_strand = STRAND_INVALID ;
  char *match_type, *match_string ;

  if (Match_string == KEY_NULL)
    {
      Match_string = str2tag("Match_string") ;
    }

  mark = bsMark(obj, 0) ;

  if (!bsFindTag(obj, Match_string))
    {
      /* No align data so just return. */
      bsGoto (obj, mark) ;
      bsMarkFree (mark) ;
      status = SMAP_STATUS_NO_DATA ;
    }
  else
    {
      bsGoto(obj, mark) ;
      bsMarkFree(mark) ;

      /* Misnomer here...it's not a perfect map but simply something that worked. */
      if ((status = smapStrFetchAlign(obj, &ref_strand, &match_strand, &match_type, &match_string))
	  == SMAP_STATUS_PERFECT_MAP)
	{
	  align_map_out->ref_strand = ref_strand ;
	  align_map_out->match_strand = match_strand ;
	  align_map_out->align_type = match_type ;
	  align_map_out->align_str = match_string ;
	}
    }

  return status ;
}



/* Maps the coordinates given in the format: "AlignXXX  Target_pos Query_pos [Length]" 
 * and returns an array of SMapMap for that data in _local_ coords (i.e. the Align tag coords).
 *
 * The returned status indicates whether the mapping was successful.
 *
 * Note that p_start, p_end and c_start, c_end are the coords from the child to its _direct_
 * parent, i.e. you _cannot_ give child coords in one object and then give parent
 * coords for an object much higher up in the smap hierachy. Hence these coords should
 * be taken directly from a database record of the form:
 *
 * Tag Tag2 obj method score p_start, p_end, c_start, c_end Align p_start c_start [length]
 *
 * You should note that p_start/p_end are only used to decide whether the map should be forward
 * or reverse. No clipping is done and the only guarantees are that the map is colinear,
 * non-overlapping and fits within c_start, c_end.
 * 
 * Use sMapReMap() to remap/clip this local map to a parent in the mapping hierachy.
 * 
 * Args:
 *    handle             Return array on this
 *    obj                In parent located at #SmapInfo
 *    p_start, p_end     Start/end coords in parent, ONLY used for deriving direction that.
 *                       local_map should be composed in.
 *    c_start, c_end     Start/end coord in child, needed if no length specified for last
 *                       align block and also for error detection.
 *    local_map_out      The returned array of SMapMap giving mapping of align data.
 *    strand_out         Direction of mapping (needed for 1 base long features where strand
 *                       is specified by hack in Align coords....)
 *    child_start_out    This is the child start (i.e. offset in the child) specified in the
 *                       set of Align coords given by obj, the caller will need this if the
 *                       the current feature they are local mapping is a true S_Child and
 *                       that child has features that will subsequently be mapped using
 *                       sMapLocalMap (ugh).
 * 
 * The word "mapping" in a child/parent context is slightly misleading here, it's better
 * to think of it as an alignment without the child/parent overtones.
 *  */
static SMapStatus sMapLocalMap(STORE_HANDLE handle,
			       OBJ obj, KEY feature,
			       int p_start, int p_end, int c_start, int c_end,
			       SMapAlignMap align_map,
			       int *child_start_out)
{
  SMapStatus status = SMAP_STATUS_PERFECT_MAP ;
  static KEY Align = KEY_NULL, AlignDNAPep, AlignPepDNA, Align_id, Match_string ;
  Array local_map = NULL ;
  BSMARK mark ;
  SMapStrand ref_strand = STRAND_INVALID, match_strand = STRAND_INVALID ;
  int child_start ;
  SMapConvType conv_type = SMAPCONV_INVALID ;
  BOOL align_style_tag = TRUE ;
  char *match_str = NULL ;
  char *align_id = NULL ;


#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
  if ((strstr(name(feature), "Eds")))
    {
      printf("%s\n", name(feature)) ;
    }
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

  if (Align == KEY_NULL)
    {
      Align = str2tag("Align") ;
      AlignDNAPep = str2tag("AlignDNAPep") ;
      AlignPepDNA = str2tag("AlignPepDNA") ;
      Align_id = str2tag("Align_id") ;
      Match_string = str2tag("Match_string") ;
    }


  /* Look for Align or match string data and other align tags. */
  mark = bsMark(obj, 0) ;

  bsGoto(obj, mark) ;

  /* Look for any align id....used to group separate alignments together. */
  if (bsFindTag(obj, Align_id))
    {
      bsGetData(obj, Align_id, _Text, &align_id) ;
    }

  bsGoto(obj, mark) ;

  if (bsFindTag(obj, Align) || bsFindTag(obj, AlignDNAPep) || bsFindTag(obj, AlignPepDNA))
    align_style_tag = TRUE ;
  else if (bsFindTag(obj, Match_string))
    align_style_tag = FALSE ;
  else
    {
      /* No align data so just return. */
      bsGoto (obj, mark) ;
      bsMarkFree (mark) ;
      status = SMAP_STATUS_NO_DATA ;

      return status ;
    }

  /* Ooops...why is this here...surely a mistake ??????? actually we then do a bsgetarray on a tag.... */
  bsGoto(obj, mark) ;
  bsMarkFree(mark) ;

  if (align_style_tag)
    {
      status = smapAlignCreateAlign(handle, obj,
				    p_start, p_end, c_start, c_end, &conv_type,
				    &local_map, &ref_strand, &match_strand, &child_start) ;
    }
  else
    {
      status = smapStrCreateAlign(handle, obj,
				  p_start, p_end, c_start, c_end,
				  &conv_type, &local_map,
				  &ref_strand, &match_strand, &match_str) ;
    }



#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
  /* debug..... */
  if (status == SMAP_STATUS_PERFECT_MAP && (strstr(name(feature), "Eds")))
    {
      printf("%s\n", name(feature)) ;

      for (i = 0 ; i < arrayMax(local_map) ; ++i)
	{
	  m = arrp(local_map, i, SMapMap) ;
	  
	  printf("r%c s%c  s->r  %d, %d  ->  %d, %d\n",
		 (ref_strand == STRAND_FORWARD ? '+' : '-'),
		 (match_strand == STRAND_FORWARD ? '+' : '-'),
		 m->s1, m->s2, m->r1, m->r2) ;
	}
    }
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */



  /* OK, verify the local map for how it fits in child coords and colinearity and
   * return whatever caller requests if all ok. */
  if (status == SMAP_STATUS_PERFECT_MAP)
    {
      if ((status = verifyLocalMap(local_map, c_start, c_end, ref_strand, match_strand)) == SMAP_STATUS_ERROR)
	{
	  arrayDestroy(local_map) ;
	  local_map = NULL ;
	}
    }

  if (status == SMAP_STATUS_PERFECT_MAP)
    {
      align_map->ref_strand = ref_strand ;
      align_map->match_strand = match_strand ;

      if (!align_style_tag)
	align_map->align_str = match_str ;

      if (align_id)
	align_map->align_id = strnew(align_id, 0) ;

      align_map->gap_map = local_map ;
      align_map->conv_type = conv_type ;

      if (child_start_out)
	*child_start_out = child_start ;
    }

  return status ;
}



/* Crude, string must be a single char that is '+' or '-'. Should really cope with white space... */
SMapStrand str2Strand(char *str)
{
  SMapStrand strand = STRAND_INVALID ;

  if (str && strlen(str) == 1)
    {
      strand = (*str == '+' ? STRAND_FORWARD
		: (*str == '-' ? STRAND_REVERSE
		   : STRAND_INVALID)) ;
    }


  return strand ;
}

/******************************/


/* Warning: a picture helps A LOT if you want to follow this function
 *
 * Notation:
 *    m1 in localMap maps x-space to y-space
 *    m2 in parentMap maps y-space to z-space
 *    m3 in the productMap maps x-space to z-space
 * 
 */
static SMapStatus sMapComposeMaps(SMap *smap,
				  Array localMap, int parent_length, Array parentMap,
				  SMapConvType conv_type,
				  SMapStrand ref_strand, SMapStrand match_strand,
				  STORE_HANDLE handle, Array *new_map_out)
{
  SMapStatus status = SMAP_STATUS_PERFECT_MAP ;
  SMapStatus tmp_status ;
  Array productMap ;
  SMapMap *m1, *m2, *m3 ;
  struct SMapMapReturn mapRet ;
  int i;
  int DNAfac, pepFac ;


  switch(conv_type)
    {
    case SMAPCONV_DNADNA :
      {
	DNAfac = 1 ;
	pepFac = 1 ;
	break ;
      }
    case SMAPCONV_DNAPEP:
      {
	DNAfac = 3 ;
	pepFac = 1 ;
	break ;
      }
    case SMAPCONV_PEPDNA:
      {
	DNAfac = 1 ;
	pepFac = 3 ;
	break ;
      }
    default:
      {
	messAssertNotReached("Bad smap align conversion type.") ;
	break ;
      }
    }

  productMap = arrayHandleCreate((arrayMax(localMap) + arrayMax(parentMap) - 1), SMapMap, handle) ;

  /* First loop over localMap, creating 0, 1 or more segments in productMap for each localMap
   * segment. Treat separately the two cases where localMap is reversed or not reversed. */
  if (arrp(localMap, 0, SMapMap)->r1 < arrp(localMap, (arrayMax(localMap) - 1), SMapMap)->r2)
    {
      int diff ;

      /* localMap is forward */

      for (i = 0 ; i < arrayMax(localMap) ; ++i)
	{
	  m1 = arrp(localMap, i, SMapMap) ;


	  /* Map the segment into the parent, note segment may get clipped and need
	   * to be adjusted for codon boundaries for peptide alignments (the mapRet
	   * does not adjust for this, perhaps it should...). */
	  tmp_status = sMapMapEx(smap, parent_length, parentMap, m1->r1, m1->r2, &mapRet) ;
	  if (tmp_status == SMAP_STATUS_ERROR)
	    break ;
	  else if (tmp_status & SMAP_STATUS_NO_OVERLAP)
	    {
	      status = SMAP_STATUS_CLIP ;		    /* Hack, we should really record which
							       end clip was. */
	      continue ;
	    }
	  else if (tmp_status != SMAP_STATUS_PERFECT_MAP)
	    status |= tmp_status ;			    /* Record if mapping was clipped. */


	  m3 = arrayp(productMap, arrayMax(productMap), SMapMap) ; /* make at least one segment */

	  m3->s1 = m1->s1 + (mapRet.nx1 - m1->r1) ;
	  m3->r1 = mapRet.y1 ;

	  if (!(tmp_status & SMAP_STATUS_INTERNAL_GAPS))
	    {
	      /* no gaps, easy case */
	      m3->s2 = m1->s2 - ( (((m1->r2 - mapRet.nx2) + (DNAfac - 1)) / DNAfac)  * pepFac) ;

	      
	      if (match_strand == STRAND_FORWARD)
		diff = m3->s2 - m3->s1 ;
	      else
		diff = m3->s1 - m3->s2 ;

	      if (mapRet.y1 < mapRet.y2)
		m3->r2 = m3->r1 + ( ((diff * DNAfac) + (DNAfac - 1))  / pepFac ) ;
	      else
		m3->r2 = m3->r1 - ( ((diff * DNAfac) + (DNAfac - 1))  / pepFac ) ;
	    }
	  else
	    {
	      /* complex case when INTERNAL_GAPS */
	      m2 = lowestUpperSeg (parentMap, m1->r1) ;

	      while (m2->s2 < mapRet.nx2) /* all the rest of m2 is in the m1 alignment */
		{
		  /* finish this segment */
		  m3->s2 = m1->s1 + ( (((m2->s2 - m1->r1) + (DNAfac - 1)) / DNAfac) * pepFac) ;
		  m3->r2 = m2->r2 ; 

		  ++m2 ;

		  /* and start another */
		  m3 = arrayp(productMap, arrayMax(productMap), SMapMap) ;
		  m3->s1 = m1->s1 + (m2->s1 - m1->r1) ;
		  m3->r1 = m2->r1 ;
		}

	      /* finish final segment */
	      m3->s2 = m1->s2 - ( (((m1->r2 - mapRet.nx2) + (DNAfac - 1)) / DNAfac) * pepFac) ;

	      if (match_strand == STRAND_FORWARD)
		diff = m3->s2 - m3->s1 ;
	      else
		diff = m3->s1 - m3->s2 ;

	      if (mapRet.y1 < mapRet.y2)
		m3->r2 = m3->r1 + ( ((diff * DNAfac) + (DNAfac - 1))  / pepFac) ;
	      else
		m3->r2 = m3->r1 - ( ((diff * DNAfac) + (DNAfac - 1))  / pepFac) ;
	    }
	}
    }
  else
    {
      /* localMap is reverse */

      for (i = 0 ; i < arrayMax(localMap) ; ++i)
	{
	  int diff ;

	  m1 = arrp(localMap, i, SMapMap) ;


	  tmp_status = sMapMapEx(smap, parent_length, parentMap, m1->r1, m1->r2, &mapRet) ;
	  if (tmp_status == SMAP_STATUS_ERROR)
	    break ;
	  else if (tmp_status & SMAP_STATUS_NO_OVERLAP) 
	    {
	      status = SMAP_STATUS_CLIP ;		    /* Hack, we should really record which
							       end clip was. */
	      continue ;
	    }
	  else if (tmp_status != SMAP_STATUS_PERFECT_MAP)
	    status |= tmp_status ;			    /* Record if mapping was clipped. */


	  m3 = arrayp(productMap, arrayMax(productMap), SMapMap) ; /* make at least one segment */

	  m3->s1 = m1->s1 + (m1->r1 - mapRet.nx1) ;
	  m3->r1 = mapRet.y1 ;

	  if (!(tmp_status & SMAP_STATUS_INTERNAL_GAPS))
	    {
	      /* easy case, no gaps. */
	      m3->s2 = m1->s2 - ( (((mapRet.nx2 - m1->r2) + (DNAfac - 1)) / DNAfac) * pepFac) ;

	      if (match_strand == STRAND_FORWARD)
		diff = m3->s2 - m3->s1 ;
	      else
		diff = m3->s1 - m3->s2 ;

	      if (mapRet.y1 < mapRet.y2)
		m3->r2 = m3->r1 + ( ((diff * DNAfac) + (DNAfac - 1))  / pepFac) ;
	      else
		m3->r2 = m3->r1 - ( ((diff * DNAfac) + (DNAfac - 1))  / pepFac) ;
	    }
	  else
	    {
	      /* complex case when INTERNAL_GAPS */
	      m2 = highestLowerSeg (parentMap, m1->r1) ;

	      while (m2->s1 > mapRet.nx2) /* all the rest of m2 is in the m1 alignment */
		{ 
		  /* finish this segment */
		  m3->s2 = m1->s1 + ( (((m1->r1 - m2->s1) + (DNAfac - 1)) / DNAfac) * pepFac) ;
		  m3->r2 = m2->r1 ;

		  --m2 ;

		  /* and start another */
		  m3 = arrayp(productMap, arrayMax(productMap), SMapMap) ;
		  m3->s1 = m1->s1 + (m1->r1 - m2->s2) ;
		  m3->r1 = m2->r2 ;
		}

	      /* finish final segment */
	      m3->s2 = m1->s2 - ( (((mapRet.nx2 - m1->r2) + (DNAfac - 1)) / DNAfac) * pepFac) ;

	      if (match_strand == STRAND_FORWARD)
		diff = m3->s2 - m3->s1 ;
	      else
		diff = m3->s1 - m3->s2 ;

	      if (mapRet.y1 < mapRet.y2)
		m3->r2 = m3->r1 + ( ((diff * DNAfac) + (DNAfac - 1))  / pepFac) ;
	      else
		m3->r2 = m3->r1 - ( ((diff * DNAfac) + (DNAfac - 1))  / pepFac) ;
	    }
	}
    }

  if (tmp_status == SMAP_STATUS_ERROR)
    {
      status = tmp_status ;
      arrayDestroy(productMap) ;
      *new_map_out = NULL ;
    }
  else if (arrayMax(productMap) == 0)
    {
      status = SMAP_STATUS_NO_OVERLAP ;
      arrayDestroy(productMap) ;
      *new_map_out = NULL ;
    }
  else
    {
      *new_map_out = productMap ;
    }

  return status ;
}


/******************************/

static void sMapInvertMap (Array map)
{
  int i, t ;
  SMapMap *m, *n ;

  /* N.B. we compare the first segment with the _last_ to detect reverse     */
  /* coords, comparing within a segment does not if r1 == r2 !               */
  if (arrp(map, 0, SMapMap)->r1 < arrp(map, (arrayMax(map) - 1), SMapMap)->r2)
    for (i = 0 ; i < arrayMax(map) ; ++i)
      {
	m = arrp(map, i, SMapMap) ;
        t = m->s1 ; m->s1 = m->r1 ; m->r1 = t ;
        t = m->s2 ; m->s2 = m->r2 ; m->r2 = t ;
      }
  else
    for (i = 0 ; i < arrayMax(map) ; ++i)
      {
	m = arrp(map, i, SMapMap) ;
        n = arrp(map, arrayMax(map)-1-i, SMapMap) ;
        t = m->s1 ; m->s1 = n->r2 ; n->r2 = t ;
        t = m->s2 ; m->s2 = n->r1 ; n->r1 = t ;
      }
}

/*********************************************************/

/* Create a new smap node. */
static SMapKeyInfo *sMapNewInfo (SMap* smap, KEY key, KEY parent)
{
  SMapKeyInfo *info = NULL ;

  info = (SMapKeyInfo*)halloc(sizeof(SMapKeyInfo), smap->handle) ;

  info->smap = smap ;
  info->key = key ;
  info->parent = parent ;
  info->start_from_parent = 1 ;				    /* default is start at first position. */
  info->children = arrayHandleCreate (8, SMapKeyInfo*, smap->handle) ;

  return info ;
}


/*****************/

/* Add a new node info. to the smap associator array for fast lookup. */
static void sMapInsertNewInfo(SMap* smap, KEY key, SMapKeyInfo *info)
{
  assInsert(smap->key2index, assVoid(key), info) ;

  return ;
}


/*****************/

/* Add a key into the smap. */
static SMapKeyInfo *sMapAddNode(SMap *smap, 
				SMapKeyInfo *parentInfo,
				KEY key,    /* key to add */
				int pStart, /* start coord in parent */
				int pEnd,   /* end coord in parent */
				OBJ parentObj, /* located at #SmapInfo */
				BOOL invert)
{
  SMapKeyInfo *info = NULL;
  Array localMap = NULL, u = NULL ;
  SMapMismatch *mis = NULL ;
  SMapMap *m = NULL ;
  int i ;
  SMapStatus status ;
  SMapAlignMapStruct align_map = {STRAND_INVALID} ;
  SMapStrand strand ;
  int our_offset ;


  /* Keys can only occur once in a map so only add if it's not already there. */
  if (!assFind (smap->key2index, assVoid(key), 0))
    {
      /* Allocate a new keyinfo struct for this node. */
      info = sMapNewInfo(smap, key, parentInfo->key) ;

      /* We need to keep a record of the proper length of the child so that we can calculate
       * the position of reverse strand features properly. pStart/pEnd may be the same as length
       * but don't have to be (e.g. there may be Align information that
       * means that pStart and pEnd are inside the childs true length). */
      if (!(info->length = getLengthInternal(key)))
	{
	  info->length = getLengthOfChildren(key) ;
      
	  /* No need to get parent coords we already have them. */
	  if (info->length < abs(pEnd - pStart) + 1)
	    info->length = abs(pEnd - pStart) + 1 ;
	}

      /* make local map then if there is no error compose with parent map to give map */
      status = sMapLocalMap(0, parentObj, key,
			    pStart, pEnd, 1, info->length,
			    &align_map, &our_offset) ;


      if (status != SMAP_STATUS_ERROR)
	{
	  SMapStrand ref_strand, match_strand ;


	  /* If there was no local map then make this nodes map from the start/end coords. */
	  if (status == SMAP_STATUS_NO_DATA)
	    {
	      localMap = arrayCreate (1, SMapMap) ;

	      m = arrayp (localMap, 0, SMapMap) ;
	      m->s1 = 1 ;
	      m->r1 = pStart ;
	      m->r2 = pEnd ;

	      /* THIS LOOKS LIKE A BAD IDEA TO DEFAULT LIKE THIS.... */
	      if (pEnd >= pStart) /* default for 1 base block is forward strand */
		{
		  m->s2 = pEnd - pStart + 1 ;
		  strand = STRAND_FORWARD ;
		}
	      else
		{
		  m->s2 =  pStart - pEnd + 1 ;
		  strand = STRAND_REVERSE ;
		}

	      /* Following the code just above I default to forward strand...sigh... */
	      if (!getMapStrands(localMap, &ref_strand, &match_strand))
		{
		  ref_strand = match_strand = STRAND_FORWARD ;
		}
	    }
	  else
	    {
	      info->start_from_parent = our_offset ;

	      localMap = align_map.gap_map ;

	      strand = align_map.ref_strand ;
	      ref_strand = align_map.ref_strand ;
	      match_strand = align_map.match_strand ;
	    }

	  if (strand == STRAND_REVERSE)
	    info->strand = invertStrand(parentInfo->strand);
	  else
	    info->strand = parentInfo->strand;

#ifdef ACEDB4	
	  if (!invert)
	    {
	      Array exonMap;

	      if ((exonMap = getExonMap(key)))
		{
		  Array unspliceMap ;

		  status = sMapComposeMaps(smap, localMap, parentInfo->length, parentInfo->map,
					   SMAPCONV_DNADNA, ref_strand, match_strand, 
					   smap->handle, &unspliceMap) ;

		  if (unspliceMap)
		    {
		      SMapStrand exon_ref_strand, exon_match_strand ;

		      if (getMapStrands(exonMap, &exon_ref_strand, &exon_match_strand))
			{
			  status = sMapComposeMaps(smap, exonMap, info->length, unspliceMap,
						   SMAPCONV_DNADNA, exon_ref_strand, exon_match_strand,
						   smap->handle, &(info->map)) ;
			}
		      else
			{
			  status = SMAP_STATUS_ERROR ;
			}

		      arrayDestroy(unspliceMap);
		    }

		  arrayDestroy(exonMap);
		}
	      else
		{
		  status = sMapComposeMaps(smap, localMap, parentInfo->length, parentInfo->map,
					   SMAPCONV_DNADNA, ref_strand, match_strand,
					   smap->handle, &(info->map)) ;
		}
	    }
	  else
	    {
	      /* The next bit is REALLY TRICKY. If we are following a mapping backwards 
		 and the parent (really child) has source exons, then the mapping we have
		 for the parent will include the mapping from the source exons. But the 
		 child (really parent) maps to the unspliced (ie no exons) version.
		 So before doing anything else we have to remove the source exons
		 mapping. Then we apply the reverse SE mapping (if any) for the child (really
		 parent) and finally the smap mapping. You are not expected to understand this.
	      */
	      Array parentExonMap;

	      sMapInvertMap(localMap) ;

	      if ((parentExonMap = getExonMap(parentInfo->key)))
		{
		  Array unspliceMap ;
		  SMapStrand exon_ref_strand, exon_match_strand ;

		  sMapInvertMap(parentExonMap) ;

		  if (!getMapStrands(parentExonMap, &exon_ref_strand, &exon_match_strand))
		    {
		      status = SMAP_STATUS_ERROR ;
		    }
		  else
		    {
		      status = sMapComposeMaps(smap, parentExonMap, parentInfo->length, parentInfo->map,
					       SMAPCONV_DNADNA, exon_ref_strand, exon_match_strand,
					       smap->handle, &unspliceMap);

		      if (unspliceMap)
			{
			  status = sMapComposeMaps(smap, localMap, info->length, unspliceMap,
						   SMAPCONV_DNADNA, ref_strand, match_strand,
						   smap->handle, &(info->map)) ;

			  arrayDestroy(unspliceMap) ;
			}
		    }

		  arrayDestroy(parentExonMap) ;
		}
	      else
		{
		  status = sMapComposeMaps(smap, localMap,
					   parentInfo->length, parentInfo->map,
					   SMAPCONV_DNADNA, ref_strand, match_strand,
					   smap->handle, &(info->map)) ;
		}
	    }
  
#else
	  /* NEVER RUN.... PROBABLY SHOULD REMOVE ALL THESE.... */
	  if (invert)
	    sMapInvertMap(localMap);
  
	  status = sMapComposeMaps(smap, localMap, parentInfo->length,
				   parentInfo->map, smap->handle, &(info->map)) ;
  
#endif /* !ACEDB4 */
  
	  arrayDestroy(localMap);
	}

      /* Must be able to map the node cleanly. */
      if (status != SMAP_STATUS_ERROR && status != SMAP_STATUS_NO_OVERLAP)
	{
	  /* get and store info->mismatch */
	  u = arrayCreate (16, BSunit) ;
	  if (bsGetArray (parentObj, str2tag("Mismatch"), u, 2))
	    {
	      info->mismatch = arrayHandleCreate (arrayMax(u)/2, SMapMismatch, smap->handle) ;
	      for (i = 0 ; i < arrayMax(u) ; i += 2)
		{ 
		  mis = arrayp(info->mismatch, arrayMax(info->mismatch), SMapMismatch) ;
		  mis->s1 = arr(u,i,BSunit).i ;
		  mis->s2 = arr(u,i+1,BSunit).i ? arr(u,i+1,BSunit).i : mis->s1 ;
		}
	    }
	  else if (bsFindTag (parentObj, str2tag("Mismatch"))) /* whole sequence */
	    { 
	      info->mismatch = arrayHandleCreate (1, SMapMismatch, smap->handle) ;
	      mis = arrayp(info->mismatch, 0, SMapMismatch) ;
	      mis->s1 = arrp(info->map, 0, SMapMap)->s1 ;
	      mis->s2 = arrp(info->map, arrayMax(info->map), SMapMap)->s2 ;
	    }
	  else
	    info->mismatch = NULL;
  
	  arrayDestroy(u) ;

	  if (info->mismatch && !arrayMax (info->mismatch))
	    arrayDestroy(info->mismatch) ;


	  /* Insert the new node into the smap and into it's parent. */
	  sMapInsertNewInfo(smap, key, info) ;

	  array(parentInfo->children, arrayMax(parentInfo->children), SMapKeyInfo*) = info ;

	  smap->nodes_allocated++ ;
	}
      else
	{
	  arrayDestroy(info->children) ;
	  messfree(info) ;
	  info = NULL ;
	}

    }

  return info ;
}



/* ParentDone flag for efficiency, set when going down tree, cleared going up.
 * aqlCondition does nothing yet */
static void sMapAddNeighbours(SMap *smap, SMapKeyInfo *keyInfo, char *aqlCondition, BOOL parentDone)
{
  int pos1, pos2 ;
  BSMARK mark1 = 0, mark2 = 0 ;
  OBJ obj ;
  KEY child, key = keyInfo->key ;
  SMapKeyInfo *newInfo;


  /* check if any children first before opening */
  if (bIndexTag (key, str2tag("S_Child")) 
      || (!parentDone && bIndexTag(key, str2tag("S_Parent")))
#ifdef ACEDB4
      || bIndexTag (key, str2tag("Subsequence"))
      || (!parentDone && bIndexTag(key, str2tag("Source")))
#endif
      ) 
    {
      
      if (!(obj = bsCreate (key)))
	{
	  return ;
	}
      
      if (bsGetKeyTags (obj, str2tag("S_Child"), 0))
	do
	  {
	    mark1 = bsMark (obj, mark1) ;

	    if (bsGetKey (obj, _bsRight, &child))
	      do
		{ 
		  mark2 = bsMark (obj, mark2) ;

		  if (getStartEndCoords(obj, "S_Child", child, &pos1, &pos2, smap->report_errors)
		      && !(sMapMapEx(smap, keyInfo->length, keyInfo->map, pos1, pos2, NULL)
			   & (SMAP_STATUS_NO_OVERLAP | SMAP_STATUS_OUTSIDE_AREA)))
		    { 
		      bsPushObj (obj) ; 
		      if ((newInfo = sMapAddNode(smap, keyInfo, child, pos1, pos2, obj, FALSE)))
			sMapAddNeighbours (smap, newInfo, aqlCondition, TRUE) ;
		    }

		  bsGoto (obj, mark2) ;
		} while (bsGetKey (obj, _bsDown, &child)) ;

	    bsGoto (obj, mark1) ;
	  } while (bsGetKeyTags (obj, _bsDown, 0)) ;
      
#ifdef ACEDB4	/* support Subsequence not under S_Child temporarily */
      bsGoto (obj, 0) ;
      if (bsGetKey (obj, str2tag("Subsequence"), &child))
	do
	  { 
	    mark1 = bsMark (obj, mark1) ;

	    if (getStartEndCoords(obj, "Subsequence", child, &pos1, &pos2, smap->report_errors)
		&& !(sMapMapEx(smap, keyInfo->length, keyInfo->map, pos1, pos2, NULL)
		     & (SMAP_STATUS_NO_OVERLAP | SMAP_STATUS_OUTSIDE_AREA)))
	      { 
		bsPushObj (obj) ; 
		if ((newInfo = sMapAddNode(smap, keyInfo, child, pos1, pos2, obj, FALSE)))
		  sMapAddNeighbours (smap, newInfo, aqlCondition, TRUE) ;
	      }

	    bsGoto (obj, mark1) ;
	  } while (bsGetKey (obj, _bsDown, &child)) ;
#endif /* ACEDB4 */
      
      if (!parentDone)
	{
	  KEY parent = 0;
	  OBJ parentObj;

	  if (bsGetKeyTags(obj, str2tag("S_Parent"), 0) && bsGetKey(obj, _bsRight, &parent))
	    {
	      if ((parentObj = bsCreate(parent)))
		{
		  /* Note child, parent swap here */
		  if (bsFindKey2 (parentObj, str2tag("S_Child"), key, 0)
		      && getStartEndCoords(parentObj, "S_Child", parent, &pos1, &pos2, smap->report_errors)
		      && (newInfo = sMapAddNode(smap, keyInfo, parent, pos1, pos2, parentObj, TRUE)))
		    sMapAddNeighbours (smap, newInfo, aqlCondition, FALSE) ;

		  bsDestroy(parentObj);
		}
	    }
#ifdef ACEDB4    
	  else if (bsGetKey (obj, str2tag("Source"), &parent))
	    {
	      if ((parentObj = bsCreate(parent)))
		{ 
		  /* Note child, parent swap here */
		  if (bsFindKey (parentObj, str2tag("Subsequence"), key)
		      && getStartEndCoords(parentObj, "Subsequence", parent, &pos1, &pos2, smap->report_errors)
		      && (newInfo = sMapAddNode(smap, keyInfo, parent, pos1, pos2, parentObj, TRUE)))
		    sMapAddNeighbours (smap, newInfo, aqlCondition, FALSE) ;
		  bsDestroy(parentObj);

		}
	    }
#endif		  
	}

      bsMarkFree (mark1) ;
      bsMarkFree (mark2) ;

      bsDestroy (obj) ;
    } 

  return ;
}



/* Recursively builds map down from key between start and stop inclusive
 * in key's coordinate system. 
 * The idea is that the aqlCondition is applied to test whether to open 
 * each object, but this is not implemented yet. */
SMap *sMapCreate(STORE_HANDLE handle, KEY key, int x1, int x2, char *aqlCondition)
{
  SMap *smap = NULL ;
  int area;

  messAssert(x1 != 0 && x2 != 0) ;

  if (x1 < x2)
    area = 1 + x2 - x1;
  else
    area = 1 + x1 - x2; 
 
  smap = sMapCreateEx(handle, key, x1, x2, 1, area, aqlCondition) ;

  return smap ;
}

/* area1 and area2 are limits in maps coordinate system. (ie 1 based )
   Only objects which
   overlap area1 to area2 are included, (but they are not truncated to that
   area if they partialy overlap). DNA is only returned from
   area1 to area2 The first base if DNA is at coordinate area1 */
SMap* sMapCreateEx(STORE_HANDLE handle, KEY key,
		   int x1, int x2, int area1, int area2,
		   char *aqlCondition)
{
  STORE_HANDLE newHandle ;
  SMap* smap ;
  SMapMap *m ;
  int end ;
  
  /* removed handle from messAssert as it's sometimes legitimately null */
  messAssert( key != KEY_UNDEFINED
	      && !(x1 == 0 && x2 == 0) 
	      && !(area1 == 0 && area2 == 0)) ;


  newHandle = handleHandleCreate(handle);

  smap = (SMap*)handleAlloc(0, newHandle, sizeof(SMap)) ;

  smap->handle = newHandle;
  smap->key2index = assHandleCreate (newHandle) ;
  smap->report_errors = FALSE ;


  /* Can be reversed if caller has got the coords of a reverse strand feature and is setting
   * the area from those coords. */
  if (area1 < area2)
    {
      smap->area1 = area1 ;
      smap->area2 = area2 ;
    }
  else
    {
      smap->area1 = area2 ;
      smap->area2 = area1 ;
    }
  
  /* insert self into smap as root */
  smap->root = sMapNewInfo (smap, key, 0) ;
  sMapInsertNewInfo(smap, key, smap->root) ;


  smap->root->map = arrayHandleCreate (1, SMapMap, smap->handle) ;
  m = arrayp(smap->root->map, 0, SMapMap) ;

  smap->root->length = sMapLength(key) ;
  if (x1 == 0)
    x1 = smap->root->length ;
  else if (x2 == 0)
    x2 = smap->root->length ;

  if (x2 > x1)
    {
      smap->root->strand = STRAND_FORWARD;
      m->s1 = x1 ;
      m->s2 = x2 ;
      m->r1 = 1 ;
      m->r2 = end = x2 - x1 + 1 ;
    }
  else
    {
      smap->root->strand = STRAND_REVERSE;
      m->s1 = x2 ;
      m->s2 = x1 ;
      m->r1 = end = x1 - x2 + 1 ;
      m->r2 = 1 ;
    }
  smap->length = end ;

  /* now recursively add all neighbours */
  sMapAddNeighbours (smap, smap->root, aqlCondition, FALSE) ;

  return smap ;
}

void sMapDestroy (SMap* smap)
{
  handleDestroy(smap->handle) ;	

  return ;
}



/******************************************************************/

/* binary searches to find segments used in sMapMap and sMapComposeMaps */

static SMapMap *lowestUpperSeg (Array map, int x)
{
  int i = 0, j = arrayMax(map)-1 ;
 
  while (i < j)
    if (x > arrp(map, (i+j)/2, SMapMap)->s2)
      i = (i+j)/2 + 1 ;
    else
      j = (i+j)/2 ;

  return arrp(map, i, SMapMap) ;
} 

static SMapMap *highestLowerSeg (Array map, int x)
{
  int i = 0, j = arrayMax(map)-1 ;
 
  while (i < j)
    if (x < arrp(map, (i+j)/2+1, SMapMap)->s1)
      j = (i+j)/2 ;
    else
      i = (i+j)/2 + 1 ;

  return arrp(map, j, SMapMap) ;
}



/************************************************************************/
/******** routines to be used independently/prior to SMAP creation ******/
/*
 *   The remaining functions don't need SMap structure - they can be used
 *   independently, e.g. sMapTreeRoot() to find the ultimate ancestor prior to 
 *   calling sMapCreate().
 *
 */



/* find parent and return map between key and parent.
   inobj may be bsCreate(key) or NULL */
static Array mapToParent(KEY key, OBJ keyObj, KEY *kp, int *length_in_parent) 
{
  OBJ obj = 0;
  KEY parent ;
  int z1, z2;
  Array map ;
  int obj_length ;
  SMapAlignMapStruct align_map = {STRAND_INVALID} ;
  SMapStatus status ;
  SMapStrand strand ;


  /* find self in parent and get our length as recorded in the parent.
   * slightly convoluted to allow use of Source #ifdef ACEDB4 */
  parent = 0;
  if (bsGetKeyTags (keyObj, str2tag("S_Parent"), 0))
    bsGetKey (keyObj, _bsRight, &parent) ;
  
  if (!(parent && (obj = bsCreate (parent)) &&
	bsFindKey2 (obj, str2tag("S_Child"), key, 0) &&
	bsGetData (obj, _bsRight, _Int, &z1) &&
	bsGetData (obj, _bsRight, _Int, &z2)))
    {
      if (obj)
	bsDestroy (obj) ;

#ifndef ACEDB4
      return NULL;
#else	/* accept Source not under S_Parent temporarily */

      obj = bsCreate (key) ;

      parent = 0;
      bsGetKey (obj, str2tag("Source"), &parent);
      bsDestroy (obj) ;
      if (!(parent && (obj = bsCreate (parent)) &&
	    bsFindKey (obj, str2tag("Subsequence"), key) &&
	    bsGetData (obj, _bsRight, _Int, &z1) &&
	    bsGetData (obj, _bsRight, _Int, &z2)))
	{
	  if (obj) bsDestroy (obj) ;
	  return NULL; 
	}
#endif /* ACEDB4 */
    }

  obj_length = abs(z2 - z1) + 1 ;


  /* Go into #SMap_info to get local map from Align data. */
  bsPushObj (obj) ;

  status = sMapLocalMap(0, obj, key,
			z1, z2, 1, obj_length,
			&align_map, NULL) ;
  if (status == SMAP_STATUS_ERROR || status == SMAP_STATUS_NO_DATA) 
    {
      SMapMap *m ;
      map = arrayCreate (1, SMapMap) ;
      m = arrayp (map, 0, SMapMap) ;
      m->r1 = z1 ;
      m->r2 = z2 ;
      m->s1 = 1 ;
      m->s2 = (z2 > z1) ? (z2 - z1 + 1) : (z1 - z2 + 1) ;
    }
  else
    {
      map = align_map.gap_map ;
      strand = align_map.ref_strand ;
    }

  bsDestroy (obj) ;


  /* Need sMapComposeMaps here......????? */
  
#ifdef ACEDB4
  /* This routine maps from child coords back into parent coords.
   * If the child has Source_exons tags we need that mapping too. */
  {
    Array temp, exonMap ;

    if ((exonMap = getExonMap(key)))
      {
	status = sMapComposeMaps(NULL, exonMap, obj_length, map,
				 SMAPCONV_DNADNA, align_map.ref_strand, align_map.match_strand,
				 0, &temp);

	if (temp)
	  {
	    arrayDestroy(map);

	    map = temp;
	  }

	arrayDestroy(exonMap);
      }
  }
#endif

  if (kp) 
    *kp = parent ;

  if (length_in_parent)
    *length_in_parent = obj_length ;

  return map ;
}



/* finds next enclosing sequence, with coords in that sequence
 * returns FALSE if no enclosing object, or can't map coords into it
 * x2 == 0 means end of sequence */
/* finds next enclosing sequence, with coords in that sequence
 * N.B. does _not_ return SMAP_STATUS_OUTSIDE_AREA because does
 * not involve an smap. */
BOOL sMapEnclosedBy(KEY key, int x1, int x2, 
		    KEY *kp, int *y1, int *y2)
{
  Array map;
  KEY parent;
  SMapStatus status;
  struct SMapMapReturn ret;
  OBJ obj;
  int length_in_parent ;

  /* Is there an enclosing object (i.e. parent) ??                           */
  if (hasNoParent(key))
    return FALSE;
  
  if (!(obj = bsCreate(key)))
    return FALSE;
  
  if (!(map = mapToParent(key, obj, &parent, &length_in_parent)))
    {
      bsDestroy(obj);
      return FALSE;
    }

  /* We only fail if there is no overlap at all, otherwise we return coords  */
  /* that may be clipped.                                                    */
  status = sMapMapEx(NULL, length_in_parent, map, x1, x2, &ret);
   
  arrayDestroy (map) ;
  bsDestroy(obj);
  
  if (status & SMAP_STATUS_NO_OVERLAP)
    return FALSE ;
  else 
    {
      if (kp)
	*kp = parent ;
      if (y1)
	*y1 = ret.y1 ;
      if (y2)
	*y2 = ret.y2 ;
    }

  return TRUE ;
}

/*****************/

/* Finds x1,x2 coords of key in top parent of keys smap tree by iteration of
 * sMapEnclosedBy() until FALSE,  
 * returns TRUE if first call to sMapEnclosedBy returns TRUE */
/* Finds x1,x2 coords of key in top parent of smap tree that contains key.
 * Returns TRUE if key _and_ coords map, FALSE otherwise.
 * 
 * Note that this is just a cover function for sMapTree(), this function
 * has been retained because its used in quite a few places in the code.
 */
BOOL sMapTreeRoot(KEY key, int x1, int x2, KEY *kp, int *y1, int *y2)
{
  BOOL result = FALSE ;
  KEY target_key = KEY_UNDEFINED ;

  /* sMapTree() will go to root key if target_key is KEY_UNDEFINED, note
   * we only change *kp if we succeed. */
  if ((result = sMapTreeCoords(key, x1, x2, &target_key, y1, y2)))
    {
      if (kp)
	*kp = target_key ;
    }

  return result ;
}


/* Finds the x1,x2 coords of "key" in "parent" and returns them in y1,y2.
 * If "parent" is KEY_UNDEFINED it does the same as sMapTreeRoot().
 *
 * Returns FALSE if the coords are completely outside or there is some
 * other problem like "parent" is not in the smap that contains "key".
 *
 * NOTE: sMapLength() must _not_ call this function, otherwise there will be recursion...
 */
BOOL sMapTreeCoords(KEY key, int x1, int x2, KEY *parent, int *y1, int *y2)
{
  BOOL result = FALSE ;
  static Associator hashSet = NULL ;
  KEY next_parent = KEY_UNDEFINED, tmp_parent = KEY_UNDEFINED ;
  int k1 = 0, k2 = 0 ;
  int length ;
  
  if (iskey(key) != LEXKEY_IS_OBJ || x1 < 0 || x2 < 0 || (x1 == 0 && x2 == 0)
      || (*parent != KEY_UNDEFINED && iskey(*parent) != LEXKEY_IS_OBJ))
    return FALSE ;

  /* simplifies coding to have dummy variables for results if not supplied by caller. */
  if (!parent)
    parent = &tmp_parent ;
  if (!y1)
    y1 = &k1 ;
  if (!y2)
    y2 = &k2 ;
  
  if (sMapEnclosedBy(key, x1, x2, &next_parent, y1, y2))
    {
      /* Enclosing parent so look for coords in that parent.      */
      /* use hashSet to break loops in object tree. */
      hashSet = assReCreate (hashSet) ;
      assInsert (hashSet, assVoid(key), assVoid(1)) ;
      while (next_parent != *parent
	     && assInsert(hashSet, assVoid(next_parent), assVoid(1))
	     && sMapEnclosedBy(next_parent, *y1, *y2, &next_parent, y1, y2)) ;

      if (next_parent == *parent)
	result = TRUE ;
      else if (*parent == KEY_UNDEFINED)
	{
	  result = TRUE ;
	  *parent = next_parent ;
	}
    }
  else if ((length = sMapLength(key))
	   && (x1 > 0 || x2 > 0))
    {
      /* No enclosing object/parent so return key + coords of this object. */
      /* In some ways we should really do an sMapMap() here for purity but */
      /* perhaps this short cut is acceptable. */  

      *parent = key ;
      
      if (x1 < 0)
	x1 = 0 ;
      if (x2 < 0)
	x2 = 0 ;

      if (x1 < x2 && x1 == 0)
	{
	  *y1 = length ;
	  *y2 = x2 ;
	}
      else if (x2 < x1 && x2 == 0)
	{
	  *y1 = x1 ;
	  *y2 = length ;
	}
      else if (x1 <= x2)
	{
	  if (x1 < 0)
	    *y1 = 1 ;
	  else
	    *y1 = x1 ;
	  
	  if (x2 > length)
	    *y2 = length ;
	  else
	    *y2 = x2 ;
	}
      else					    /* x2 <= x1 */
	{
	  if (x2 < 0)
	    *y2 = 1 ;
	  else
	    *y2 = x2 ;
	  
	  if (x1 > length)
	    *y1 = length ;
	  else
	    *y1 = x1 ;
	}
      result = TRUE ;
    }
  
  return result ;
}




/* THE CORRECT LOGIC WOULD SEEM TO BE SOMETHING LIKE:
 * 
 *     If there is internal length use that
 *     else if max(s_parent length, s_child length)
 * 
 * REMEMBER that this is not relative to an smap, its just an attempt to get the length
 * of an object.......
 * 
 * Remember also that any internal length (ie DNA tag) overrules anything else I think otherwise
 * the database is in error....if we don't do this I think everything becomes a bit of a nonsense,
 * possible to do but a bit of a nonsense....
 *
 */


/* Try various strategies (DNA, S_parent, S_children) to find the sequence length of an object
 * could do all and check consistency?
 * N.B. does _not_ return SMAP_STATUS_OUTSIDE_AREA because does
 * not involve an smap.
 *
 * NOTE: You should not call sMapTreeRoot() from here either directly or indirectly
 * othewise there will be recursion in a run away sense.....*/
int sMapLength (KEY key)
{
  int max = 0 ;
  OBJ obj = NULL ;

  if (!bIndexTag (key, str2tag("DNA")) &&
      !bIndexTag (key, str2tag("S_Parent")) &&
      !bIndexTag (key, str2tag("S_Child"))
#ifdef ACEDB4
      && !bIndexTag (key, str2tag("Source"))
      && !bIndexTag (key, str2tag("Subsequence"))
#endif
      )
    {
      max = 0 ;
    }
  else if (!(obj = bsCreate (key)))
    {
      max = 0 ;
    }
  else if ((max = getLengthInternal(key)))
    {
      /* check first if object has its own intrinsic length, e.g. has its own DNA */
      max = max ;
    }
  else
    {
      /* Otherwise we record whichever is the greater of the child and parent lengths. */
      int child, parent ;

      child = getLengthOfChildren(key) ;
      parent = getLengthInParent(obj) ;

      max = (child > parent ? child : parent) ;
    }

  if (obj)
    bsDestroy(obj) ;

  return max ;
}



/*
 * We may wish to promote the following set of getLengthXXX() routines to be part
 * of the external smap interface at some time but for now they will be an
 * internal routines only.
 *
 */


/* Looks only in the object itself for tags that might explicitly give the length of
 * this object, i.e. we do not look at child or parent lengths. */
static int getLengthInternal(KEY key)
{
  int result = 0 ;
  KEY dna_key ;
  OBJ obj = NULL ;

  /* Look for dna, model fragment:   "DNA UNIQUE ?DNA UNIQUE Int" */
  if (bIndexGetKey(key, str2tag("DNA"), &dna_key))
    {
      int length = 0 ;

      if ((obj = bsCreate(key))
	  && bsFindKey(obj, str2tag("DNA"), dna_key)
	  && bsGetData(obj, _bsRight, _Int, &length))
	{
	  result = length ;
	}
    }

  if (obj)
    bsDestroy (obj) ;

  return result ;
}


/* Looks in the object for S_Child information that implicitly could give the length of
 * this object, i.e. we derive the length of this object from the maximum span of the children. */
static int getLengthOfChildren(KEY key)
{
  int result = 0 ;
  BOOL s_child, subsequence ;

  if ((s_child = bIndexTag(key, str2tag("S_Child")))
#ifdef ACEDB4
      || (subsequence = bIndexTag(key, str2tag("Subsequence")))
#endif /* ACEDB4 */
      )
    {
      OBJ obj ;

      if ((obj = bsCreate(key)))
	{
	  int length = 0, i ;
	  Array units = NULL ;

	  units = arrayCreate(48, BSunit) ;

	  if (s_child && bsGetArray(obj, str2tag("S_Child"), units, 4))
	    {
	      /* find highest coord of all the S_Child'ren */
	      for (i = 2 ; i < arrayMax (units) ; i += 4)
		{
		  if (arr(units,i,BSunit).i > length)
		    length = arr(units,i,BSunit).i ;
		  if (arr(units,i+1,BSunit).i > length)
		    length = arr(units,i+1,BSunit).i ;
		}
	    }
	  
#ifdef ACEDB4
	  /* Note, there may be a mixture of S_Child and Subsequence tags,
	   * it is vital to check both, do not ignore the Subsequences if
	   * S_Childs exist. */
	  if (bsGetArray(obj, str2tag("Subsequence"), units, 3)) /* note bsGetArray zeros array for us. */
	    {
	      /* or Subsequence in ACEDB 4 */
	      for (i = 1 ; i < arrayMax (units) ; i += 3)
		{
		  if (arr(units,i,BSunit).i > length)
		    length = arr(units,i,BSunit).i ;
		  if (arr(units,i+1,BSunit).i > length)
		    length = arr(units,i+1,BSunit).i ;
		}
	    }
#endif /* ACEDB4 */

	  if (length > 0)
	    result = length ;

	  arrayDestroy(units) ;
	  bsDestroy(obj) ;
	}
    }

  return result ;
}


/* actually , is this correct even ? Why do we look for clipped coords ???? */

/* Maps object into parent to get length of object as recorded in parent. */
static int getLengthInParent(OBJ obj)
{
  int result = 0, length_in_parent ;
  Array map ;

  if ((map = mapToParent(bsKey(obj), obj, NULL, &length_in_parent)))
    {
      /* next if S_parent */
      struct SMapMapReturn ret ;

      if (!(sMapMapEx(NULL, length_in_parent, map, 1, 0, &ret) & SMAP_STATUS_NO_OVERLAP))
	result = (ret.nx2 > ret.nx1) ? (ret.nx2 - ret.nx1 + 1) : (ret.nx1 - ret.nx2 + 1) ;

      arrayDestroy(map) ;
    }

  return result ;
}




#ifdef ACEDB4
/***********************************************************/
/******* a utility needed for Source_exons sorting *********/

static int dnaExonsOrder (void *a, void *b)
{
  ExonStruct *ea = (ExonStruct *)a ;
  ExonStruct *eb = (ExonStruct *)b ;
  return ea->x - eb->x ;
}


/* can't just arraySort, because a is in BSunits, not
 * pairs of BSunits */
static void exonsSort (Array a)
{
  int i, n ;
  Array b = 0 ;

  n = arrayMax(a) / 2 ;
  b = arrayCreate (n, ExonStruct) ;
  for (i = 0 ; i < n ; ++i)
    {
      arrayp(b,i,ExonStruct)->x = arr(a,2*i,BSunit).i ;
      arrp(b,i,ExonStruct)->y = arr(a,2*i+1,BSunit).i ;
    }

  arraySort (b, dnaExonsOrder) ;

  for (i = 0 ; i < n ; ++i)
    {
      arr(a,2*i,BSunit).i = arrp(b,i,ExonStruct)->x ;
      arr(a,2*i+1,BSunit).i = arrp(b,i,ExonStruct)->y ;
    }

  arrayDestroy (b) ;

  return ;
}


static Array getExonMap(KEY key)
{
  Array u, exonMap = 0;
  OBJ obj = 0 ;
  int x, i ;
  SMapMap *m;

  u = arrayCreate (16, BSunit) ;
  if (bIndexTag(key, str2tag("Source_exons"))
      && (obj = bsCreate(key))
      && bsGetArray (obj, str2tag("Source_exons"), u, 2))
    {
      exonsSort (u) ;
      exonMap = arrayCreate(arrayMax(u)/2, SMapMap) ;
      x = 1 ;
      for (i = 0 ; i < arrayMax(u) ; i += 2)
	{
	  m = arrayp (exonMap, i/2, SMapMap) ;
	  m->r1 = arr(u,i,BSunit).i ;
	  m->r2 = arr(u,i+1,BSunit).i ;
	  m->s1 = x ;
	  m->s2 = x + (m->r2 - m->r1) ;
	  x += m->r2 - m->r1 + 1 ;
	}
    }
  if (obj)
    bsDestroy (obj) ;

  arrayDestroy(u);

  return exonMap;
}
#endif


/* THIS ROUTINE IS FLAWED....IF THE PARENT HAS NO COORD DATA IN IT THEN IT CANNOT BE USED
 * FOR MAPPING I THINK.......PERHAPS ITS MEANT TO DEFAULT BUT IT DOESN'T AT THE MOMENT
 * SO NEED TO CHECK THIS BEHAVIOUR.......WE SHOULD CERTAINLY CHECK IF PARENT HAS REVERSE
 * TAGS MENTIONING THE CHILD WHICH WE DON'T........ */
/* Is this object at the top of a hierachy (i.e. has no parent tags) ?
 *                                                                           */
static BOOL hasNoParent(KEY key)
{
  BOOL result = FALSE ;

  if (bIndexTag(key, str2tag("S_Parent")))
    result = FALSE ;
  else
    result = TRUE ;

#ifdef ACEDB4
  /* Objects either have S_Parent or Source, not both...                     */
  if (result == TRUE)
    {
      if (bIndexTag(key, str2tag("Source")))
	result = FALSE ;
      else
	result = TRUE ;
    }
#endif

  return result ;
}


/* Runs through a map produced by SMapLocalMap() checks start/end coords and colinearity,
 * returns an smap status.
 *
 * Note we assume that child coords are always forward as they should be. */
static SMapStatus verifyLocalMap(Array local_map, int c_start, int c_end,
				 SMapStrand ref_strand, SMapStrand match_strand)
{
  SMapStatus status = SMAP_STATUS_PERFECT_MAP ;
  SMapMap *first, *last ;

  first = arrp(local_map, 0, SMapMap) ;
  last = arrp(local_map, arrayMax(local_map) - 1, SMapMap) ;

  if ((match_strand == STRAND_FORWARD && (first->s1 < c_start || last->s2 > c_end))
      || (match_strand == STRAND_REVERSE && (first->s1 < c_end || last->s2 > c_start)))
    {
      /* If child start/end does not agree this is an error, this should just be correct... */
      status = SMAP_STATUS_ERROR ;
    }
  else
    {
      /* If not colinear then this is an error. We check within an SMapMap block and check
       * the block against the next block. */
      SMapMap *curr, *next ;
      int i ;
      int max ;

      curr = arrp(local_map, 0, SMapMap) ;
      next = NULL ;
      max = arrayMax(local_map) ;

      for (i = 0 ; i < max ; i++)
	{
	  curr = arrp(local_map, i, SMapMap) ;
	  
	  if (i < max - 1)
	    next = arrp(local_map, i + 1, SMapMap) ;
	  else
	    next = NULL ;

	  if ((status = verifyBlock(curr, ref_strand, match_strand)) != SMAP_STATUS_PERFECT_MAP)
	    break ;

	  if (next && ((status = verifyInterBlock(curr, next, ref_strand, match_strand))
		       != SMAP_STATUS_PERFECT_MAP))
	    break ;
	}
    }

  return status ;
}

static SMapStatus verifyBlock(SMapMap *block, SMapStrand ref_strand, SMapStrand match_strand)
{
  SMapStatus status = SMAP_STATUS_PERFECT_MAP ;

  if ((match_strand == STRAND_FORWARD
       ? (block->s1 > block->s2 ? TRUE : FALSE)
       : (block->s1 < block->s2 ? TRUE : FALSE))
      || (ref_strand == STRAND_FORWARD
	  ? (block->r1 > block->r2 ? TRUE : FALSE)
	  : (block->r1 < block->r2 ? TRUE : FALSE)))
    status = SMAP_STATUS_ERROR ;

  return status ;
}

static SMapStatus verifyInterBlock(SMapMap *curr, SMapMap *next, SMapStrand ref_strand, SMapStrand match_strand)
{
  SMapStatus status = SMAP_STATUS_PERFECT_MAP ;


  if ((match_strand == STRAND_FORWARD
       ? (curr->s2 > next->s1 ? TRUE : FALSE)
       : (curr->s2 < next->s1 ? TRUE : FALSE))
      || (ref_strand == STRAND_FORWARD
	  ? (curr->r2 > next->r1 ? TRUE : FALSE)
	  : (curr->r2 < next->r1 ? TRUE : FALSE)))
    status = SMAP_STATUS_ERROR ;

  return status ;
}





#ifdef ED_G_NEVER_INCLUDE_THIS_CODE

/* not used...perhaps for future function ?? */

/* Runs through a map produced by SMapLocalMap() and clips it to the parent start/end */
static SMapStatus clipLocalMap(int p_start, int p_end, Array local_map, SMapStrand strand,
			       int DNAfac, int pepFac)
{
  SMapStatus status = SMAP_STATUS_PERFECT_MAP ;
  int i ;
  SMapMap *m ;

  /* Clip blocks to p_start/p_end, we remove completely any blocks that are not mapped. */
  for (i = 0 ; i < arrayMax(local_map) ; i++)
    {
      int diff ;
      BOOL remove_element ;

      diff = 0 ;
      remove_element = FALSE ;
      m = arrp(local_map, i, SMapMap) ;

      if (strand == STRAND_FORWARD)
	{
	  if (m->r1 > p_end)
	    {
	      remove_element = TRUE ;

	      status |= SMAP_STATUS_X2_EXTERNAL_CLIP ;
	    }
	  else if (m->r2 < p_start)
	    {
	      remove_element = TRUE ;

	      status |= SMAP_STATUS_X1_EXTERNAL_CLIP ;
	    }
	  else
	    {
	      if (m->r1 < p_start)
		{
		  diff = p_start - m->r1 ;
		  
		  m->r1 +=  diff ;				    /* i.e. m->r1 = p_start */
		  m->s1 += ((diff / DNAfac) * pepFac) ;

		  status |= SMAP_STATUS_X1_EXTERNAL_CLIP ;
		}
	      
	      if (m->r2 > p_end)
		{
		  diff = m->r2 - p_end ;
		  
		  m->r2 -= diff ;				    /* i.e. m->r2 = p_end */
		  m->s2 -= ((diff / DNAfac) * pepFac) ;

		  status |= SMAP_STATUS_X2_EXTERNAL_CLIP ;
		}
	    }

	}
      else /* strand == STRAND_REVERSE */
	{
	  if  (m->r1 < p_end)
	    {
	      remove_element = TRUE ;

	      status |= SMAP_STATUS_X2_EXTERNAL_CLIP ;
	    }
	  else if (m->r2 > p_start)
	    {
	      remove_element = TRUE ;

	      status |= SMAP_STATUS_X1_EXTERNAL_CLIP ;
	    }
	  else
	    {
	      if (m->r1 > p_start)
		{
		  diff = m->r1 - p_start ;
		  
		  m->r1 -=  diff ;				    /* i.e. m->r1 = p_end */
		  m->s1 += ((diff / DNAfac) * pepFac) ;

		  status |= SMAP_STATUS_X1_EXTERNAL_CLIP ;
		}

	      if (m->r2 < p_end)
		{
		  diff = p_end - m->r2 ;
		  
		  m->r2 += diff ;				    /* i.e. m->r2 = p_start */
		  m->s2 -= ((diff / DNAfac) * pepFac) ;

		  status |= SMAP_STATUS_X2_EXTERNAL_CLIP ;
		}

	    }

	}

      if (remove_element)
	{
	  messCheck(arrayRemove(local_map, (void *)m, SMapMapOrder), != TRUE,
		    "Align gap pointer not found in gaps array.") ;
	  i-- ;						    /* arrayRemove() shifts whole array
							       down one so move index back one. */
	}
    }


  return status ;
}
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */




/*
 *     Routines for dumping a human readable smap.....
 */


/* human-readable dump of an smap to dest */
void sMapDump(SMap *smap, ACEOUT dest, SMapDumpType dump_options)
{
  SMapKeyInfo *root ;
  KEY rootkey;
  int i, k, k1, start, rx1, rx2 ;
  char *cp, *cq ;
  char buffer [8000] ;
  Array dna;

  aceOutPrint(dest, "sMap %s\n\n", nameWithClassDecorate(smap->root->key)) ;

  if ((dump_options & SMAPDUMP_STRUCTS) || (dump_options & SMAPDUMP_ALL))
    {
      aceOutPrint(dest, "sMap struct:\n") ;
      aceOutPrint(dest, "\tsMap->length = %d\n", smap->length) ;
      aceOutPrint(dest, "\tsMap->area1 = %d\n", smap->area1) ;
      aceOutPrint(dest, "\tsMap->area2 = %d\n", smap->area2) ;
      aceOutPrint(dest, "\tsMap->nodes_allocated = %d\n\n", smap->nodes_allocated) ;

      root = smap->root ;
      aceOutPrint(dest, "sMap root:\n") ;
      aceOutPrint(dest, "\troot->key = %s\n",  nameWithClassDecorate(root->key)) ;
      aceOutPrint(dest, "\troot->parent = %s\n",  (root->parent ? nameWithClassDecorate(root->parent) : "<none>")) ;
      aceOutPrint(dest, "\troot->strand = %s\n", sMapIsReverse(root, 1, 1) ? "-" : "+") ;
      aceOutPrint(dest, "\troot->start_from_parent = %d\n", root->start_from_parent) ;
      aceOutPrint(dest, "\troot->length = %d\n\n", root->length) ;

      aceOutPrint(dest, "sMap tree root:\n") ;
      if (!sMapTreeRoot(smap->root->key, 1, 0, &rootkey, &rx1, &rx2))
	aceOutPrint(dest, "\tNO TREE ROOT\n\n");
      else
	{
	  aceOutPrint(dest, "\ttree root = %s\n", nameWithClassDecorate(rootkey));
	  aceOutPrint(dest, "\tcoords in tree root = %d, %d\n", rx1, rx2);
	  aceOutPrint(dest, "\tlength in tree root = %d\n\n", sMapLength(smap->root->key));
	}
    }


  if ((dump_options & SMAPDUMP_ASSEMBLY) || (dump_options & SMAPDUMP_ALL))
    {
      aceOutPrint(dest, "sMap assembly:\n") ;
      sMapDumpInfo(smap->root, dest, 0, TRUE);
      aceOutPrint(dest, "\n\n");
    }

  if ((dump_options & SMAPDUMP_SMAP) || (dump_options & SMAPDUMP_ALL))
    {
      aceOutPrint(dest, "sMap map:\n") ;
      sMapDumpInfo(smap->root, dest, 0, FALSE);
      aceOutPrint(dest, "\n\n");
    }

  if ((dump_options & SMAPDUMP_DNA) || (dump_options & SMAPDUMP_ALL))
    {
      aceOutPrint(dest, "sMap dna:\n") ;
      if (!(dna = sMapDNA(smap, 0, NULL)))
	aceOutPrint(dest, "NO DNA\n");
      else
	{
	  i = arrayMax(dna) ;

	  aceOutPrint(dest, "Length = %d", i) ;
	  if (i > 1000)
	    {
	      aceOutPrint(dest, " (only first 1,000 bases printed)", i);
	      i = 1000 ;
	    }
	  aceOutPrint(dest, "\n") ;
      
	  cp = arrp(dna,0,char) ;
	  cq = buffer ;
      
	  while(i > 0)
	    {
	      cq = buffer ;
	      for (k=0 ; k < 4000/(50 + 3) ; k++)
		if (i > 0)
		  {
		    start = cp - arrp(dna,0,char) + smap->area1;
		    cq += sprintf(cq, "%5d ", start);
		    k1 = 50 ;
		    while (k1--  && i--)
		      *cq++ = dnaDecodeChar[*cp++ & 0xff] ;
		    *cq++ = '\n' ;
		    *cq = 0 ;
		  }
	      aceOutPrint(dest, "%s", buffer) ;
	    }

	  arrayDestroy(dna);
	}
    }
  
  aceOutPrint(dest, "\n");

  return ;
}
  
  
static void sMapDumpInfo(SMapKeyInfo *info, ACEOUT dest, int offset, BOOL only_dna)
{
  int i;
  BOOL has_dna = FALSE ;

  if (only_dna)
    has_dna = dnaObjHasOwnDNA(info->key) ;


  if (!only_dna || (only_dna && has_dna))
    {
      aceOutPrint(dest, "\n");
      for (i=0; i<offset; i++)
	aceOutPrint(dest, " ");

      aceOutPrint(dest, "*** %s, orient = %s,", nameWithClassDecorate(info->key),
		  sMapIsReverse(info, 1, 1) ? "-" : "+");

      if (info->map)
	{
	  aceOutPrint(dest, " map = ");

	  for (i=0; i<arrayMax(info->map); i++)
	    {
	      SMapMap *m = arrayp(info->map, i, SMapMap);
	      aceOutPrint(dest, "[%d,%d -> %d,%d] ", m->s1, m->s2, m->r1, m->r2);
	    }
	}

      if (info->mismatch)
	{
	  aceOutPrint(dest," mis = ");

	  for (i=0; i<arrayMax(info->map); i++)
	    {
	      SMapMismatch *m = arrayp(info->mismatch, i, SMapMismatch);
	      aceOutPrint(dest, "%d,%d ", m->s1, m->s2);
	    }
	}
    }

  if (!only_dna  || (only_dna && !has_dna))
    {
      if (info->children)
	{
	  for (i=0; i<arrayMax(info->children); i++)
	    {
	      SMapKeyInfo *inf = array(info->children, i, SMapKeyInfo *);
	      sMapDumpInfo(inf, dest, offset+3, only_dna);
	    }
	}
    }


  return ;
}



#ifdef ED_G_NEVER_INCLUDE_THIS_CODE

/* some test case debugging.... */
static void callTest()
{
  SMapStatus status ;
  SMapConvType conv_type ;
  int tp_start, tp_end, tp_clipped_start, tp_clipped_end,
    tc_start, tc_end, tc_clipped_start, tc_clipped_end ;

  /* DNA forward -> DNA forward */
  conv_type = SMAPCONV_DNADNA ;
  tp_start = 1 ;
  tp_end = 100 ;
  tp_clipped_start = 3 ;
  tp_clipped_end = 99 ;
  tc_start = 1001 ;
  tc_end = 1100 ;
  tc_clipped_start = 0 ;
  tc_clipped_end = 0 ;

  status = sMapClipAlign(conv_type, tp_start, tp_end, &tp_clipped_start, &tp_clipped_end,
			 tc_start, tc_end, &tc_clipped_start, &tc_clipped_end) ;


  /* DNA forward -> peptide */
  conv_type = SMAPCONV_DNAPEP ;
  tp_start = 1 ;
  tp_end = 99 ;
  tp_clipped_start = 2 ;
  tp_clipped_end = 97 ;
  tc_start = 1 ;
  tc_end = 33 ;
  tc_clipped_start = 0 ;
  tc_clipped_end = 0 ;

  status = sMapClipAlign(conv_type, tp_start, tp_end, &tp_clipped_start, &tp_clipped_end,
			 tc_start, tc_end, &tc_clipped_start, &tc_clipped_end) ;


  /* DNA forward -> peptide */
  conv_type = SMAPCONV_DNAPEP ;
  tp_start = 101 ;
  tp_end = 199 ;
  tp_clipped_start = 102 ;
  tp_clipped_end = 197 ;
  tc_start = 201 ;
  tc_end = 233 ;
  tc_clipped_start = 0 ;
  tc_clipped_end = 0 ;

  status = sMapClipAlign(conv_type, tp_start, tp_end, &tp_clipped_start, &tp_clipped_end,
			 tc_start, tc_end, &tc_clipped_start, &tc_clipped_end) ;


  /* DNA reverse -> peptide */
  conv_type = SMAPCONV_DNAPEP ;
  tp_start = 99 ;
  tp_end = 1 ;
  tp_clipped_start = 97 ;
  tp_clipped_end = 2 ;
  tc_start = 1 ;
  tc_end = 33 ;
  tc_clipped_start = 0 ;
  tc_clipped_end = 0 ;

  status = sMapClipAlign(conv_type, tp_start, tp_end, &tp_clipped_start, &tp_clipped_end,
			 tc_start, tc_end, &tc_clipped_start, &tc_clipped_end) ;


  return ;
}
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */


/* Returns TRUE and the start/end coords in pos1_out, pos2_out if both are specified and both are
 * both > 0, FALSE otherwise. */
BOOL getStartEndCoords(OBJ obj, char *position_tag, KEY child,
		       int *pos1_out, int *pos2_out, BOOL report_errors)
{
  BOOL result = FALSE ;
  int pos1, pos2 ;
  char *err_msg = NULL ;

  if (!bsGetData (obj, _bsRight, _Int, &pos1)
      || !bsGetData (obj, _bsRight, _Int, &pos2))
    {
      if (report_errors)
	err_msg = hprintf(0, "%s", "start and/or end coord missing") ;
    }
  else if (pos1 < 1 || pos2 < 1)
    {
      if (report_errors)
	err_msg = hprintf(0, "start and/or end coord < 1, start: %d, end: %d", pos1, pos2) ;
    }
  else
    {
      *pos1_out = pos1 ;
      *pos2_out = pos2 ;

      result = TRUE ;
    }

  if (!result && report_errors)
    {
      messerror("%s: %s tag for child %s has %s",
		nameWithClassDecorate(bsKey(obj)), position_tag, name(child), err_msg) ; 
      messfree(err_msg) ;
    }


  return result ;
}





/* Look for objects used in assembly...just like dna call. */
static BOOL findAssembly(Array assembly, SMapKeyInfo *info)
{
  BOOL result = FALSE ;
  int i ;
  

  if (bIndexTag(info->key, str2tag("DNA")))
    {
      return addAssembly(assembly, info) ;
    }

  /* This sequence has no dna directly attached so look in children.     */
  for (i = 0 ; i < arrayMax(info->children) ; ++i)
    {
      /* N.B. we return TRUE if one or more of the children are on the assembly path. */
      if (findAssembly(assembly, arr(info->children, i, SMapKeyInfo*)))
	result = TRUE ;
    }
  
  return result;
}


/* If object has right tags etc then add it to assembly array, the tag testing code should be
 * be merged with that in the dna routines.... */
static BOOL addAssembly(Array assembly, SMapKeyInfo *info)
{
  BOOL result = FALSE ;
  OBJ obj ;

  if ((obj = bsCreate(info->key)))
    {
      KEY key = KEY_UNDEFINED ;
      int i, length = 0, clip = 0 ;

      /* We look specifically for objects that actually contain DNA...check this works for James's
       * database... */
      if (bsGetKey(obj, str2tag("DNA"), &key)
	  && bsGetData(obj, _bsRight, _Int, &length)
	  && length > 0)
	{
	  SMapAssembly fragment ;

	  /* Special for worm, they don't use the smap tags to construct a golden pathway, they
	   * map the whole clones, the golden path section of the clone is from 1 to
	   * (Overlap_right - 1) so if specified we clip the end. */
	  if (bsFindTag(obj, str2tag("Overlap_right"))
	      && bsFindTag(obj, _bsRight))  
	    bsGetData(obj, _bsRight, _Int, &clip) ;


	  fragment = arrayp(assembly, arrayMax(assembly), SMapAssemblyStruct) ;	  
	  fragment->key = info->key ;
	  fragment->strand = info->strand ;
	  fragment->length = length ;
	  fragment->map = arrayCopy(info->map) ;

	  for (i = 0 ; i < arrayMax (fragment->map) ; ++i)
	    {
	      SMapMap *map = arrp(fragment->map, i, SMapMap);
	      int s1 = map->s1;
	      int s2 = map->s2;
	      int r1 = map->r1;
	      int r2 = map->r2;
 
	      /* We need to work out overall start/end using a mapping. */
	      if (!(fragment->start))
		{
		  if (r1 < r2)
		    {
		      fragment->start = r1 ;
		      fragment->end = r2 ;
		    }
		  else
		    {
		      fragment->start = r2 ;
		      fragment->end = r1 ;
		    }

		  fragment->start = fragment->start - (s1 - 1) ;
		  fragment->end = fragment->end + (fragment->length - s2) ;
		}

	      if (info->strand == STRAND_FORWARD)
		{
		  if (clip)
		    {
		      int diff = s2 - clip + 1 ;

		      s2 -= diff ;
		      r2 -= diff ;
		    }
		}
	      else
		{
		  if (clip && s2 < clip)
		    {
		      int diff = clip - s2 + 1 ;

		      s2 += diff ;
		      r2 += diff ;

		    }
		}

	      if (s1 > s2) /* no overlap with area at all. */
		{
		  arrayDestroy(fragment->map) ;
		  fragment->map = NULL ;
		  
		  result = TRUE ;
		  break ;
		}

	      map->s2 = s2;
	      map->r2 = r2;
	    }

	  result = TRUE ;
	}

      bsDestroy (obj) ;
    }
  
  return result ;
}


/* Given a map of SMapMap returns TRUE if the reference and match strands can be determined,
 * FALSE otherwise. Note...for maps of features of length 1 this function returns FALSE. */
static BOOL getMapStrands(Array map, SMapStrand *ref_strand_out, SMapStrand *match_strand_out)
{
  BOOL result = TRUE ;
  SMapMap *start, *end ;
  SMapStrand ref_strand, match_strand ;

  start = arrayp(map, 0, SMapMap) ;
  end = arrayp(map, (arrayMax(map) - 1), SMapMap) ;

  if (result)
    {
      if (start->s1 < end->s2)
	match_strand = STRAND_FORWARD ; 
      else if (start->s1 > end->s2)
	match_strand = STRAND_REVERSE ;
      else
	result = FALSE ;
    }

  if (result)
    {
      if (start->r1 < end->r2)
	ref_strand = STRAND_FORWARD ; 
      else if (start->r1 > end->r2)
	ref_strand = STRAND_REVERSE ;
      else
	result = FALSE ;
    }

  if (result)
    {
      *ref_strand_out= ref_strand ;
      *match_strand_out = match_strand ;
    }

  return result ;
}

  

/*********************** end of file ************************/
