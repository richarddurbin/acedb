/*  File: smap.h
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
 * Description: The sMap interface. A set of database tags and
 *              code to construct virtual sequences and map features
 *              on to those sequences. The code handles gaps, mismatches
 *              and much else.
 *              You should *note* that sMap coords are 1-based, i.e. 
 *              sequences run from 1 -> length. This means you need to
 *              be very careful when converting coords to/from a 0-based
 *              system (e.g. fMap).
 *              
 * HISTORY:
 * Last edited: Jul 13 17:50 2012 (edgrif)
 * Created: Wed Jul 28 22:11:39 1999 (rd)
 * CVS info:   $Id: smap.h,v 1.46 2012-10-30 10:03:58 edgrif Exp $
 *-------------------------------------------------------------------
 */
#ifndef DEF_SMAP_H
#define DEF_SMAP_H

/* this is a signal to acedb code that uses smap, it should gradually go away. */
#define USE_SMAP  /* define this to use new smap code */

#include <wh/regular.h>
#include <wh/bs.h>


/* Opaque type representing an SMap. */
typedef struct SMapStruct SMap ;


/* Opaque type representing SMap information for a single smap'd key. */
typedef struct SMapKeyInfoStruct SMapKeyInfo ;


/* It is an error for strand to be STRAND_INVALID as all mappings should be to one or
 * the other strand. */
typedef enum {STRAND_INVALID, STRAND_FORWARD, STRAND_REVERSE} SMapStrand ;


/* Basic struct used throughout SMap to hold coord transform information, giving the
 * mapping between input and output space. (s for "subject", r for "reference"...???) */
typedef struct
{
  int s1, s2 ;						    /* coords in input/target/subject/match space */
  int r1, r2 ;						    /* coords in output/reference space */
} SMapMap ;



/* Struct used to hold the result of a gapped alignment.
 * The strand fields show which strand of the reference sequence the
 * alignment was made to and from which strand of the match sequence
 * the alignment was made from. */

/* Controls how coordinates are converted. */
typedef enum
  {
    SMAPCONV_INVALID,
    SMAPCONV_DNADNA,					    /* dna -> dna */
    SMAPCONV_DNAPEP,					    /* dna -> peptide */
    SMAPCONV_PEPDNA					    /* peptide -> dna */
  } SMapConvType ;


/* Defines the different align string formats supported. */
typedef enum {ALIGNSTR_INVALID, ALIGNSTR_EXONERATE_CIGAR, ALIGNSTR_EXONERATE_VULGAR,
	      ALIGNSTR_ENSEMBL_CIGAR} SMapAlignStrFormat ;

typedef struct
{
  SMapConvType conv_type ;

  SMapStrand ref_strand ;
  SMapStrand match_strand ;

  int ref_x1, ref_x2 ;
  int match_x1, match_x2 ;

  char *align_id ;					    /* Used to group aligns. */

  /* If alignment was gapped then either an array of alignment blocks is given or an alignment
   * string in the format indicated. */
  Array gap_map ;					    /* SMapMap array of gaps. */

  /* An align string, e.g. cigar. */
  char *align_type ;
  char *align_str ;
} SMapAlignMapStruct, *SMapAlignMap ;



/* Struct used to hold the assembly information for one section
 * of the virtual sequence for an smap. Often this would be part of
 * a clone used to construct a chromosome. */
typedef struct
{
  KEY key ;						    /* The object from which this section
							       was taken. */
  SMapStrand strand ;					    /* Which way round this section is in
							       the virtual sequence. */
  int length ;						    /* Total length of object. */
  int start, end ;					    /* Start/end of complete object in parent. */
  Array map ;						    /* Array of SMapMap giving mapped sections. */
} SMapAssemblyStruct, *SMapAssembly ;





/* Controls what parts of an smap get dumped. */
typedef enum
  {
    SMAPDUMP_INVALID   = 0x00,
    SMAPDUMP_STRUCTS   = 0x01,				    /* The smap struct data. */
    SMAPDUMP_SMAP      = 0x02,				    /* The smap itself. */
    SMAPDUMP_ASSEMBLY  = 0x04,				    /* The smap nodes that form the dna assembly. */
    SMAPDUMP_DNA       = 0x08,				    /* The smap dna. */
    SMAPDUMP_ALL       = 0x0F,				    /* All of the above. */
  } SMapDumpType ;



/* SMap status can be one of:                                           
 *                                                                          
 *   SMAP_STATUS_PERFECT_MAP for mapping with no gaps, no errors.                      
 *   SMAP_STATUS_ERROR       for serious error, e.g. invalid args.
 *   SMAP_STATUS_NO_DATA     not able to map because no coordinate data available.
 *   or some combination of other bits indicating mapping result.
 * 
 * For NO_OVERLAP_EXTERNAL and CLIP the bit settings indicate which end of the
 * input coords the no overlap or clip happens.
 */  
typedef enum
  {
    SMAP_STATUS_PERFECT_MAP            = 0x0000U, /* Perfect map without gaps. */
    SMAP_STATUS_INTERNAL_GAPS          = 0x0001U, /* Map with gap(s) within alignment */
    SMAP_STATUS_OUTSIDE_AREA           = 0x0002U, /* Mapped, but not in area. */
    SMAP_STATUS_MISALIGN               = 0x0004U, /* Not mapped because ref and match coords have
						     different spans. */

    SMAP_STATUS_ERROR                  = 0x0008U, /* No map, serious problem e.g. invalid smap. */
    SMAP_STATUS_BADARGS	               = 0x000cU, /* No map, bad args to function. */
    SMAP_STATUS_NO_DATA                = 0x000eU, /* Not mapped because no data, e.g. no Align tags. */


    SMAP_STATUS_X1_NO_OVERLAP_EXTERNAL = 0x0010U, /* failure: [x1,x2] outside map range at x1 end. */
    SMAP_STATUS_X2_NO_OVERLAP_EXTERNAL = 0x0020U, /* failure: [x1,x2] outside map range at x2 end. */
    SMAP_STATUS_NO_OVERLAP_EXTERNAL    = 0x0030U, /* either of the above. */
    SMAP_STATUS_NO_OVERLAP_INTERNAL    = 0x0040U, /* failure: [x1,x2] within a gap in map */
    SMAP_STATUS_NO_OVERLAP	       = 0x0070U, /* any of the above: y1, y2 unchanged */

    SMAP_STATUS_X1_EXTERNAL_CLIP       = 0x0100U, /* x1 off end of map */
    SMAP_STATUS_X1_INTERNAL_CLIP       = 0x0200U, /* x1 in internal gap in map */
    SMAP_STATUS_X1_CLIP		       = 0x0300U, /* either of the above: implies *nx1 != x1 */
    SMAP_STATUS_X2_EXTERNAL_CLIP       = 0x0400U, /* etc. */
    SMAP_STATUS_X2_INTERNAL_CLIP       = 0x0800U,
    SMAP_STATUS_X2_CLIP                = 0x0c00U,
    SMAP_STATUS_CLIP                   = 0x0f00U
  } SMapStatus ;






/* Is a flag set with no error signalled ?                                   */
#define SMAP_STATUS_SET(STATUS, STATUS_FLAG) \
( ((STATUS) & (STATUS_FLAG)) && ((STATUS) != SMAP_STATUS_ERROR) )

/* Most common tests are for overlap or for overlap + in area. */
#define SMAP_STATUS_OVERLAP(STATUS) \
( (!((STATUS) & SMAP_STATUS_NO_OVERLAP)) && ((STATUS) != SMAP_STATUS_ERROR) )

#define SMAP_STATUS_ISCLIP(STATUS) \
  ( (((STATUS) & SMAP_STATUS_CLIP)) && ((STATUS) != SMAP_STATUS_ERROR) )

#define SMAP_STATUS_INAREA(STATUS) \
( (!((STATUS) & SMAP_STATUS_NO_OVERLAP)) && (!((STATUS) & SMAP_STATUS_OUTSIDE_AREA)) \
  && ((STATUS) != SMAP_STATUS_ERROR) )


/* Possible return codes currently used only from DNA callback, but could also be used
 * to decide what to do for errors in smap creation. */
typedef enum
 {
   sMapErrorReturnFail,					    /* abort, sMapDNA returns NULL. */
   sMapErrorReturnContinue,				    /* continue, further mismatches cause
							       further callbacks DNA returned. */
   sMapErrorReturnSilent,				    /* continue, no futher callbacks even
							       when more error encountered. */
   sMapErrorReturnContinueFail				    /* further matches cause further
							       callbacks: call to sMapDNA returns NULL */
 } consMapDNAErrorReturn;


/* Called by sMapDNA when a non-allowed mismatch happens. */
typedef consMapDNAErrorReturn (*sMapDNAErrorCallback)(KEY key, int position) ;


/* Application defined function that can be used to filter parent objects when looking for 
 * the enclosing parent of an object. */
typedef BOOL (*sMapSpanPredicate)(KEY key) ;




/*
 *        General routines that can be used to obtain information which
 *        can then be used to create an SMap, e.g. sMapTreeRoot() will
 *        find the ultimate ancestor of a key prior to calling sMapCreate().
 *
 */

/* finds next enclosing sequence, with coords in that sequence
 * returns FALSE if no enclosing object, or can't map coords into it
 * x2 == 0 means end of sequence */
BOOL sMapEnclosedBy (KEY key, int x1, int x2, KEY *kp, int *y1, int *y2) ;

/* Finds x1,x2 coords of key in top parent of keys smap tree by iteration of
 * sMapEnclosedBy() until FALSE,  
 * returns TRUE if first call to sMapEnclosedBy returns TRUE */
BOOL sMapTreeRoot(KEY key, int x1, int x2, KEY *kp, int *y1, int *y2) ;

/* Finds x1,x2 coords of key in target_parent, which must be a parent of
 * of key in keys smap tree, if target_parent is set to KEY_UNDEFINED it
 * does the same as sMapTreeRoot(). */
BOOL sMapTreeCoords(KEY key, int x1, int x2, KEY *target_parent, int *y1, int *y2) ;

/* try various strategies (DNA, S_parent, S_children) to find the sequence 
 * length of an object */
int sMapLength (KEY key) ;




/* 
 *             SMap Create/Destroy  + other routines that operate on an SMap.
 */


/* Recursively builds map down from key between start and stop inclusive
   in key's coordinate system. 
   The idea is that the aqlCondition is applied to test whether to open 
   each object, but this is not implemented yet.
   NB x2 can not be 0 here: must explicitly get length first. */
SMap *sMapCreate (STORE_HANDLE handle, KEY key, int x1, int x2, char *aqlCondition) ;


/* area1 and area2 are limits in maps coordinate system. (ie 1 based )
   Only objects which
   overlap area1 to area2 are included, (but they are not truncated to that
   area if they partialy overlap). DNA is only returned from
   area1 to area2 The first base if DNA is at coordinate area1 */
SMap *sMapCreateEx (STORE_HANDLE handle, KEY key, int x1, int x2,
		    int area1, int area2, char *aqlCondition) ;


/* this gives all keys that map onto this smap */
KEYSET sMapKeys (SMap *smap, STORE_HANDLE handle) ;

Array sMapAssembly(SMap* smap, STORE_HANDLE handle) ;


/* Gets the DNA in acedb units.
 * sMapDNAErrorCallback may be NULL, which is equivalent to a callBack function which
 * always returns sMapErrorReturnFail. */
Array sMapDNA (SMap *smap, STORE_HANDLE handle, sMapDNAErrorCallback) ;



/* Converts interval x1..x2 in key's coordinate system to y1..y2 in
   smap's.  Returns TRUE if succeeds, FALSE if e.g. key not known in smap,
   or both x1, x2 out of range (on the same side), or both fall in the same
   gap in the map.  If part of the interval is out of range then it clips. */
BOOL sMapConvert (SMap *smap, KEY key, int x1, int x2, int *y1, int *y2) ;


/* For a given span find the sequence object that contains that span
 * without gaps. If sMapSpanPredicate is non-NULL, it gets used to filter the output. */
BOOL sMapFindSpan(SMap *smap, KEY *keyp, int *start, int *end, sMapSpanPredicate) ;

/* Return maximum extent of an SMap. */
int sMapMax(SMap *smap) ;

void sMapDestroy(SMap *smap) ;



/* 
 *         SMapKeyInfo + other routines that operate on parts of an SMap.
 * 
 *         The routines provide complex support for coordinate conversion.
 *         These are called by sMapConvert(), and are more efficient than
 *         using sMapConvert().
 */


/* gives handle for use with SMapMap */
SMapKeyInfo *sMapKeyInfo (SMap *smap, KEY key) ;


/* Return human readable error as static string.
   Note that at most one bit should be set in status, if more 
   than one is set in your result, call this multiple times and 
   cat the return values to taste. May return NULL for unused bits. */
char *sMapErrorString(SMapStatus status);


/* Converts interval x1..x2 to the ultimate coord system held in keyinfo and returns in y1..y2,
 * nx1..nx2 are the clipped values that were actually mapped to y1, y2 (- for code simplicity 
 * these are always set, even if NO_OVERLAP status is returned in which case they 
 * are x1, x2). nx1..nx2 are required to detect which end(s) was clipped.
 * return value is status as above. */
SMapStatus sMapMap(SMapKeyInfo *info, int x1, int x2,
		   int *y1, int *y2, int *nx1, int *nx2) ;

/* as for sMapMap(), but maps in the reverse direction */
SMapStatus sMapInverseMap(SMapKeyInfo *info, int x1, int x2, 
			  int *y1, int *y2, int *nx1, int *nx2) ;


#ifdef ACEDB4
/* As for sMapMap, but for objects with source exons,
   the conversion is from the unspliced co-ordinate system.
   Converts interval x1..x2 in key's coordinate system to y1..y2 in
   smap's.  Returns SMapStatus as above, note well that it will clip
   coords so you may end up with the returned y1,y2 being identical.
   Beware that this call is _more costly than sMapMap(). */
SMapStatus sMapUnsplicedMap(SMapKeyInfo *info, int x1, int x2, 
			    int *y1, int *y2, int *nx1, int *nx2) ;

#endif


/* Map the coordinates given in a set of AlignNNN tag data read from obj
 * into the coords frame given by ref_start, ref_end. The map is returned in
 * align_map_out.
 *
 * Note that local_map may be empty, clipped or complete depending on the mapping into the
 * smap coord frame.
 *
 * Note that:
 *
 *    ref_start, ref_end, p_clipped_start and p_clipped_end
 *         will normally come directly from an sMapMap() call.
 *
 *    p_start, p_end, c_start and c_end will normally be read directly from the database.
 */
SMapStatus sMapMapAlign(STORE_HANDLE handle,
			OBJ obj, KEY feature,
			BOOL allow_misalign, BOOL map_gaps,
			int ref_start, int ref_end,
			int p_start, int p_end,
			int *p_clipped_start_inout, int *p_clipped_end_inout,
			int c_start, int c_end,
			int *c_clipped_start_out, int *c_clipped_end_out,
			SMapAlignMap align_map_out) ;


char *sMapGaps2AlignStr(Array gaps, SMapAlignStrFormat str_format) ;
char *sMapAlignStrFormat2Str(SMapAlignStrFormat align_format) ;
SMapStatus sMapFetchAlignStr(OBJ obj, SMapAlignMap align_map_out) ;



/* Is the mapping from this key  to the root on the reverse strand.
   True means that for mapping from x1,x2 to y1,y2 gives y1>y2 when x1<x2.
   Note that you can discover this from a call to sMapMap except in 
   the pathological case that x1==x2 or an odd map which returns y1==y2.
   This function is supplied to cover those cases. */
BOOL sMapIsReverse(SMapKeyInfo *info, int x1, int x2);


/* Return parent, or NULL of root of tree. Note that this is parent
   wrt this smap: it may be a child if the mapping was traversed backwards! */
KEY sMapParent(SMapKeyInfo *info);

/* Return Map array for this key */
Array sMapMapArray(SMapKeyInfo *info);



/* human-readable dump of an smap to dest */
void sMapDump(SMap *smap, ACEOUT dest, SMapDumpType dump_options) ;

KEY defaultSubsequenceKey (char* name, int colour, 
			   float priority, BOOL isStrand);

void sMapFeaturesComplement(int length, Array segs, Array seqinfo, Array homolinfo);


#endif /* DEF_SMAP_H */
/***************************************************************************/
