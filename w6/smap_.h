/*  File: smap_.h
 *  Author: Ed Griffiths (edgrif@sanger.ac.uk)
 *  Copyright (c) Sanger Institute, 2003
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
 * 	Richard Durbin (Sanger Centre, UK) rd@sanger.ac.uk, and
 *	Jean Thierry-Mieg (CRBM du CNRS, France) mieg@kaa.crbm.cnrs-mop.fr
 *
 * Description: Data structs etc. internal to SMap.
 * HISTORY:
 * Last edited: Jul 13 17:48 2012 (edgrif)
 * Created: Fri Oct 31 09:45:55 2003 (edgrif)
 * CVS info:   $Id: smap_.h,v 1.12 2012-10-22 15:16:05 edgrif Exp $
 *-------------------------------------------------------------------
 */
#ifndef DEF_SMAP_P_H
#define DEF_SMAP_P_H

#include <wh/smap.h>					    /* External header. */


/********** data structures ****************************************/

typedef struct
{
  int s1, s2 ;	/* start/end of segments allowed to disagree, 
		   in source coordinates */
} SMapMismatch ;



/* For each key or node in the smap tree the keyInfo record gives an array for the map and an
   array of mismatch information, which can be null.

   It also records:

   - which strand of smap root we are mapped to, this is needed because otherwise we cannot
   tell the strand of features that are 1 base long.

   - The length of this key in its parent is also recorded because the coordinates
   in the "map" array may have been clipped and hence cannot always be used
   for positioning of child features that are reversed with respect to this
   keys coordinates (i.e. we always know the key starts at "1" and hence clipping
   of the start does not matter, but if the end is clipped we have no way of knowing
   the clipped length unless we record it separately.

   - In a similar way we also need to know how we were aligned into our parent,
   we may have been specified with an  S_Child.........Align parent_start child_start.
   We need this child_start information so we can give sMapLocalMap() all the information it needs
   to work.

   Each SMapMap structure in map defines a set of intervals in the 
   coordinate system of key, each interval being mapped as an ungapped
   block to a corresponding interval in the SMap's coordinates.
   
   Each [s1,s2] pair in mismatch defines a set of intervals in the 
   coordinate system of key that can contain mismatches to the canonical 
   sequence.

   Both ->map and ->mismatch arrays are sorted in increasing order of s1, 
   and s2 >= s1 always.  We also assume here that the sequences are 
   colinear.
*/
struct SMapKeyInfoStruct
{
  KEY key, parent ;

  /* Which strand of the ultimate smap parent we were mapped to. */
  SMapStrand strand ;					    /* vital for features of length = 1 */

  /* The start and length of the section of this feature which was mapped into the smap,
   * we can be clipped at either end.  */
  int start_from_parent ;
  int length ;

  SMap *smap ;						    /* Back pointer to our parent smap. */

  Array map ;						    /* of SMapMap */
  Array inverseMap ;					    /* of SMapMap - filled lazily */
  Array mismatch ;					    /* of SMapMisMatch */
  Array children ;					    /* of SMapKeyInfo* */
} ;



/* This is the main structure representing an SMap, this points to all the other information
 * for an SMap. */
struct SMapStruct
{
  STORE_HANDLE handle ;					    /* the SMap's own handle. */
  Associator key2index ;				    /* For quick lookup of node from key. */
  SMapKeyInfo *root ;
  int length ;						    /* Max length of this smap. */
  int area1, area2;					    /* Subpart actually constructed. */

  /* following is a simple cache for key2index mapping */
  SMapKeyInfo *lastKeyInfo ;

#ifdef ACEDB4
  Array lastKeyUnspliced;
#endif

  int nodes_allocated ;

  BOOL report_errors ;					    /* If TRUE (default) then mapping errors are
							       reported and logged. */

} ;


/* Struct used to return result of mapping, y1 & y2 are the original coordinates remapped into
 * root coordinates. nx1 & nx2 are the original coordinates truncated to fit into the root
 * coordinate system, if no truncation was necessary then they will be original coordinates. */
struct SMapMapReturn
{
  int y1, y2;						    /* root co-ordinates */
  int nx1, nx2;						    /* truncated local co-ordinates */
};


/* Used by internal exon sorting routines. */
typedef struct { int x, y ; } ExonStruct ;


#define invertStrand(x) ((x) == STRAND_FORWARD ? STRAND_REVERSE : STRAND_FORWARD)

#define convTypeIsValid(CONV_TYPE) \
  ((CONV_TYPE == SMAPCONV_DNADNA || CONV_TYPE == SMAPCONV_DNAPEP || CONV_TYPE == SMAPCONV_PEPDNA))

#define strandIsValid(STRAND) \
  ((STRAND == STRAND_FORWARD || STRAND == STRAND_REVERSE))





/* SMap package internal routines that are at file scope are prefixed with "smap" to distinguish
 * them from SMap external interface routines that are prefixed with "sMap". */

SMapStatus smapAlignCreateAlign(STORE_HANDLE handle,
				OBJ obj, int p_start, int p_end, int c_start, int c_end,
				SMapConvType *conv_type_out, Array *local_map_out,
				SMapStrand *ref_strand_out, SMapStrand *match_strand_out,
				int *child_start_out) ;

SMapStatus smapStrCreateAlign(STORE_HANDLE handle, OBJ obj,
			      int p_start, int p_end, int c_start, int c_end,
			      SMapConvType *conv_type_out, Array *local_map_out,
			      SMapStrand *ref_strand_out, SMapStrand *match_strand_out, char **match_str_out) ;

SMapStatus smapStrFetchAlign(OBJ obj, SMapStrand *ref_strand_out, SMapStrand *match_strand_out,
			     char **match_type_out, char **match_str_out) ;




#endif /* DEF_SMAP_P_H */
