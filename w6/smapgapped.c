/*  File: smapgapped.c
 *  Author: Ed Griffiths (edgrif@sanger.ac.uk)
 *  Copyright (c) 2009: Genome Research Ltd.
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
 * Description: Contains functions that handle gapped alignment mapping.
 *              For homologies the mapping is in two stages: first the
 *              extent of an alignment is mapped and then the gapped
 *              alignment within that extent is mapped taking account
 *              of any clipping from the first alignment.
 *
 * Exported functions: See wh/smap.h
 *              
 * HISTORY:
 * Last edited: Jul 13 17:47 2012 (edgrif)
 * Created: Fri Jun 26 09:08:29 2009 (edgrif)
 * CVS info:   $Id: smapgapped.c,v 1.4 2012-10-30 09:52:08 edgrif Exp $
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


/* Need description of tags processed here... */





/* structs/types for processing various alignment string formats. */




/* We get align strings in several variants and then convert them into this common format. */
typedef struct
{
  char op ;
  int length ;
} AlignStrOpStruct, *AlignStrOp ;

typedef struct
{
  SMapAlignStrFormat format ;
  SMapConvType align_type ;
  GArray *align ;					    /* Of AlignStrOpStruct. */
} AlignStrCanonicalStruct, *AlignStrCanonical ;




static SMapAlignStrFormat alignStrGetFormat(char *match_str) ;
static BOOL alignStrVerifyStr(char *match_str, SMapAlignStrFormat align_format) ;
static AlignStrCanonical alignStrMakeCanonical(char *match_str, SMapAlignStrFormat align_format) ;
static SMapConvType alignStrGetType(int ref_start, int ref_end, AlignStrCanonical canon) ;
static BOOL alignStrCanon2Acedb(AlignStrCanonical canon, SMapStrand ref_strand, SMapStrand match_strand,
				STORE_HANDLE handle, OBJ obj,
				int p_start, int p_end, int c_start, int c_end,
				Array *local_map_out) ;

static BOOL exonerateVerifyVulgar(char *match_str) ;
static BOOL exonerateVerifyCigar(char *match_str) ;
static BOOL ensemblVerifyCigar(char *match_str) ;

static BOOL exonerateCigar2Canon(char *match_str, AlignStrCanonical canon) ;
static BOOL exonerateVulgar2Canon(char *match_str, AlignStrCanonical canon) ;
static BOOL ensemblCigar2Canon(char *match_str, AlignStrCanonical canon) ;
static int cigarGetLength(char **cigar_str) ;

static char *nextWord(char *str) ;
BOOL gotoLastDigit(char **cp_inout) ;
BOOL gotoLastSpace(char **cp_inout) ;

static int SMapMapOrder(void *va, void *vb) ;
static int SMapMapRevOrder(void *va, void *vb) ;


/*
 *                   External interface routines
 */


char *sMapGaps2AlignStr(Array gaps, SMapAlignStrFormat str_format)
{
  char *align_str = NULL ;






  return align_str ;
}


char *sMapAlignStrFormat2Str(SMapAlignStrFormat align_format)
{
  char *format = NULL ;
  char *strings[] = {"INVALID", "CIGAR", "VULGAR", "ENSEMBL_CIGAR"} ;

  if (align_format < ALIGNSTR_INVALID || align_format > ALIGNSTR_ENSEMBL_CIGAR)
    align_format = ALIGNSTR_INVALID ;

  format = strings[align_format] ;

  return format ;
}




/*
 *                  Package "external" routines
 */


/* We should get handed an OBJ with the cursor at the Match_string tag so we can
 * do a bsFlatten to get the match string data which is an array with three elements
 * per row:
 * 
 *   < RefFwd_HitFwd | RefRev_HitFwd | RefFwd_HitRev | RefRev_HitRev>  <"match format"> <"match string">
 * 
 * The match string will be in the format specified by the match format..
 * 
 *  */
SMapStatus smapStrFetchAlign(OBJ obj, 
			     SMapStrand *ref_strand_out, SMapStrand *match_strand_out,
			     char **match_type_out, char **match_str_out)
{
  SMapStatus status = SMAP_STATUS_PERFECT_MAP ;
  static KEY ff_tag = KEY_NULL, fr_tag, rf_tag, rr_tag, match_string ;
  Array cigar_array ;
  BSunit *cigar_data ;
  KEY strand_key ;
  char *match_type, *match_str ;
  SMapStrand ref_strand = STRAND_FORWARD, match_strand = STRAND_FORWARD ;

  /* Check/verify strand specification. */
  if (!ff_tag)
    {
      ff_tag = str2tag("RefFwd_HitFwd") ;
      fr_tag = str2tag("RefFwd_HitRev") ;
      rf_tag = str2tag("RefRev_HitFwd") ;
      rr_tag = str2tag("RefRev_HitRev") ;
      match_string = str2tag("Match_string") ;
    }


  cigar_array = arrayCreate(5, BSunit) ;
  if (bsGetArray(obj, match_string, cigar_array, 4))
    {
      cigar_data = arrp(cigar_array, 0, BSunit) ;
      strand_key = cigar_data[0].k ;
      match_type = cigar_data[1].s ;
      match_str = cigar_data[2].s ;
    }
  else
    {
      status = SMAP_STATUS_ERROR ;
    }
  arrayDestroy(cigar_array) ;


  if (status != SMAP_STATUS_ERROR)
    {
      if (strand_key == ff_tag)
	{
	  ref_strand = STRAND_FORWARD ;
	  match_strand = STRAND_FORWARD ;
	}
      else if (strand_key == fr_tag)
	{
	  ref_strand = STRAND_FORWARD ;
	  match_strand = STRAND_REVERSE ;
	}
      else if (strand_key == rf_tag)
	{
	  ref_strand = STRAND_REVERSE ;
	  match_strand = STRAND_FORWARD ;
	}
      else if (strand_key == rr_tag)
	{
	  ref_strand = STRAND_REVERSE ;
	  match_strand = STRAND_REVERSE ;
	}
      else
	{
	  status = SMAP_STATUS_ERROR ;
	}
    }

  /* All ok ? Then return results. */
  if (status == SMAP_STATUS_PERFECT_MAP)
    {
      *ref_strand_out = ref_strand ;
      *match_strand_out = match_strand ;
      *match_type_out = g_strdup(match_type) ;
      *match_str_out = g_strdup(match_str) ;
    }

  return status ;
}



/* We should get handed a bsFlatten'd array (cigar_data) which has just two elements
 * per row:
 * 
 *   < RefFwd_HitFwd | RefRev_HitFwd | RefFwd_HitRev | RefRev_HitRev>  < "match string" >
 * 
 * The match string could be in exonerate cigar or vulgar format or ensembl cigar format.
 * 
 *  */
SMapStatus smapStrCreateAlign(STORE_HANDLE handle, OBJ obj,
			      int p_start, int p_end, int c_start, int c_end,
			      SMapConvType *conv_type_out, Array *local_map_out,
			      SMapStrand *ref_strand_out, SMapStrand *match_strand_out, char **match_str_out)
{
  SMapStatus status = SMAP_STATUS_PERFECT_MAP ;
  static KEY ff_tag = KEY_NULL, fr_tag, rf_tag, rr_tag, match_string ;
  Array cigar_array ;
  BSunit *cigar_data ;
  KEY strand_key ;
  char *match_str ;
  SMapStrand ref_strand = STRAND_FORWARD, match_strand = STRAND_FORWARD ;
  AlignStrCanonical align_canon ;
  SMapAlignStrFormat align_format ;
  Array local_map = NULL ;


  /* Check/verify strand specification. */
  if (!ff_tag)
    {
      ff_tag = str2tag("RefFwd_HitFwd") ;
      fr_tag = str2tag("RefFwd_HitRev") ;
      rf_tag = str2tag("RefRev_HitFwd") ;
      rr_tag = str2tag("RefRev_HitRev") ;
      match_string = str2tag("Match_string") ;
    }


  cigar_array = arrayCreate(5, BSunit) ;
  if (bsGetArray(obj, match_string, cigar_array, 3))
    {
      cigar_data = arrp(cigar_array, 0, BSunit) ;
      strand_key = cigar_data[0].k ;
      match_str = cigar_data[1].s ;
    }
  else
    {
      status = SMAP_STATUS_ERROR ;
    }
  arrayDestroy(cigar_array) ;


  if (status == SMAP_STATUS_PERFECT_MAP)
    {
      if (strand_key == ff_tag)
	{
	  ref_strand = STRAND_FORWARD ;
	  match_strand = STRAND_FORWARD ;
	}
      else if (strand_key == fr_tag)
	{
	  ref_strand = STRAND_FORWARD ;
	  match_strand = STRAND_REVERSE ;
	}
      else if (strand_key == rf_tag)
	{
	  ref_strand = STRAND_REVERSE ;
	  match_strand = STRAND_FORWARD ;
	}
      else if (strand_key == rr_tag)
	{
	  ref_strand = STRAND_REVERSE ;
	  match_strand = STRAND_REVERSE ;
	}
      else
	{
	  status = SMAP_STATUS_ERROR ;
	}
    }

  /* We need to get the format, e.g. exonerate cigar/vulgar, ensembl cigar */
  /* Verify the string according to the format (optional step ?) */
  if (status == SMAP_STATUS_PERFECT_MAP)
    {
      if (!(align_format = alignStrGetFormat(match_str)))
	{
	  status = SMAP_STATUS_BADARGS ;
	}

      /* We are not currently supporting vulgar so return bad_args... */
      if (align_format == ALIGNSTR_EXONERATE_VULGAR)
	status = SMAP_STATUS_BADARGS ;
    }

  /* Verify the string according to the format (optional step ?) */
  if (status == SMAP_STATUS_PERFECT_MAP)
    {
      if (!(alignStrVerifyStr(match_str, align_format)))
	{
	  status = SMAP_STATUS_ERROR ;
	}
    }

  /* make a canonical representation of the string to do ops on */
  if (status == SMAP_STATUS_PERFECT_MAP)
    {
      if (!(align_canon = alignStrMakeCanonical(match_str, align_format)))
	{
	  status = SMAP_STATUS_ERROR ;
	}
    }

  /* Set the type of the alignment, i.e. dna -> dna or pep -> dna */
  if (status == SMAP_STATUS_PERFECT_MAP)
    {
      if (!(align_canon->align_type = alignStrGetType(p_start, p_end, align_canon)))
	status = SMAP_STATUS_ERROR ;
    }

  if (status == SMAP_STATUS_PERFECT_MAP)
    {
      if (!alignStrCanon2Acedb(align_canon, ref_strand, match_strand,
			       handle, obj, p_start, p_end, c_start, c_end, &local_map))
	status = SMAP_STATUS_ERROR ;
    }

  /* All ok ? Then return results. */
  if (status == SMAP_STATUS_PERFECT_MAP)
    {
      *ref_strand_out = ref_strand ;
      *match_strand_out = match_strand ;

      *conv_type_out = align_canon->align_type ;
      *local_map_out = local_map ;
      *match_str_out = match_str ;
    }

  return status ;
}


/* This routine expects the object cursor to be positioned on one of the AlignXXX tags, this is
 * not checked for performance reasons. */
SMapStatus smapAlignCreateAlign(STORE_HANDLE handle,
				OBJ obj, int p_start, int p_end, int c_start, int c_end,
				SMapConvType *conv_type_out, Array *local_map_out,
				SMapStrand *ref_strand_out, SMapStrand *match_strand_out,
				int *child_start_out)
{
  SMapStatus status = SMAP_STATUS_PERFECT_MAP ;
  char *align_id = NULL ;
  Array local_map = NULL ;
  SMapMap *m ;
  Array u ;
  BSunit *ui ;
  int i ;
  int DNAfac, pepFac ;
  SMapStrand ref_strand = STRAND_INVALID, match_strand = STRAND_INVALID ;
  int child_start ;
  SMapConvType convtype = SMAPCONV_INVALID ;
  int diff ;


  u = arrayCreate(20, BSunit) ;

  if (bsGetArray(obj, str2tag("Align"), u, 3))
    {
      /* DNA -> DNA */
      DNAfac = 1 ;
      pepFac = 1 ;
      convtype = SMAPCONV_DNADNA ;
    }
  else if (bsGetArray(obj, str2tag("AlignDNAPep"), u, 3))
    {
      /* Peptide -> DNA */
      DNAfac = 3 ;
      pepFac = 1 ;
      convtype = SMAPCONV_DNAPEP ;
    }
  else if (bsGetArray(obj, str2tag("AlignPepDNA"), u, 3))
    {
      /* DNA -> Peptide */
      DNAfac = 1 ;
      pepFac = 3 ;
      convtype = SMAPCONV_PEPDNA ;
    }
  else
    {
      status = SMAP_STATUS_ERROR ;

      goto clear_up ;					    /* NOTE we return from here. */
    }


  /* Set alignment direction if possible for parent and child, i.e. we can't determine the strand
   * for p_start == p_end or c_start == c_end yet, we need to check the Align info. */
  if (p_end > p_start)					    /* forward alignment */
    ref_strand = STRAND_FORWARD ; 
  else if (p_end < p_start)
    ref_strand = STRAND_REVERSE ;

  if (c_end > c_start)
    match_strand = STRAND_FORWARD ; 
  else if (c_end < c_start)
    match_strand = STRAND_REVERSE ;


  /* Treat 1 base long alignments as a special case. */
  if (p_start == p_end && c_start == c_end)
    {
      /* negative length means invert, but only when p_start == p_end,
       * this in the database:
       *        s_child s_sequence 100 100 Align 100 1 -1 
       *    gives a single base sequence on reverse strand. */

      ui = arrp(u, 0, BSunit) ;

      if (arrayMax(u) != 3
	  || (!abs(ui[0].i) || !abs(ui[1].i))
	  || (abs(ui[0].i) != p_start || abs(ui[1].i) != c_start))
	{
	  status = SMAP_STATUS_ERROR ;

	  goto clear_up ;				    /* NOTE we return from here. */
	}
      else
	{
	  local_map = arrayHandleCreate(arrayMax(u)/3, SMapMap, handle) ;

	  /* Set strands first from sign of position. */
	  if (ui[0].i > 0)
	    ref_strand = STRAND_FORWARD ;
	  else
	    ref_strand = STRAND_REVERSE ;

	  if (ui[1].i > 0)
	    match_strand = STRAND_FORWARD ;
	  else
	    match_strand = STRAND_REVERSE ;


	  /* Note that the mapping is from  (Query_pos [Length]) -> (Target_pos) so
	   * m->s1 is the query coord and m->r1 is the target coord. */
	  m = arrayp(local_map, 0, SMapMap) ;
	  m->r1 = m->r2 = abs(ui[0].i) ;
	  m->s1 = m->s2 = abs(ui[1].i) ;
	}
    }
  else
    {
      /* Next convert u into local_map. Data following Align tag is "Target_pos Query_pos [Length]"
       * so this will fill in all ->s1 and ->r1 values and any ->s2 values where "length" has been
       * specified. */
      local_map = arrayHandleCreate(arrayMax(u)/3, SMapMap, handle) ;
      for (i = 0, child_start = 1 ; i < (arrayMax(u) / 3) ; ++i)
	{
	  ui = arrp(u, 3 * i, BSunit) ;

	  /* UM...I'M NOT SURE WE NEED TO DO THIS ANY MORE..... */
	  /* We need to find out the start of our alignment in our containing object to return
	   * to our caller. */
	  if (i == 0)
	    {
	      child_start = ui[1].i ;
	    }


	  /* Note that the mapping is from  (Query_pos [Length]) -> (Target_pos) so
	   * m->s1 is the query coord and m->r1 is the target coord. */
	  m = arrayp(local_map, i, SMapMap) ;
	  m->r1 = ui[0].i ;
	  m->s1 = ui[1].i ;


	  /* If a query length was specified then use it to calculate the length of the
	   * alignment and set m->s2. */
	  if (ui[2].i)
	    {
	      /* I really, really, really don't understand the below code and comment....
	       * why don't we check for
	       * p_start == p_end if its only supposed to happen then, if p_start == p_end then...
	       * there can only be one Align length surely....so we shouldn't need to loop ???? */

	      /* negative length means invert, but only when p_start == p_end,
	       * this in the database:
	       *        s_child s_sequence 100 100 Align 100 1 -1 
	       *    gives a single base sequence on reverse strand. */
	      if (ui[2].i < 0)
		{
		  ref_strand = STRAND_REVERSE;
		  m->s2 = m->s1 - ui[2].i - 1 ;
		}
	      else if (match_strand == STRAND_REVERSE)
		m->s2 = m->s1 - (ui[2].i - 1) ;
	      else
		m->s2 = m->s1 + ui[2].i - 1 ;
	    }
	}


      /* It is an error if we can't determine the strand so we clear up and return.... */
      if (ref_strand == STRAND_INVALID)
	{
	  arrayDestroy(local_map) ;
	  status = SMAP_STATUS_ERROR ;

	  return status ;					    /* NOTE we return from here. */
	}


      /* Sort is vital because if length is omitted we calculate the length from the start of
       * the next block.
       * n.b. sorting does not mean that things are colinear necessarily because we may get
       * overlaps if the data is faulty.... */
      if (match_strand == STRAND_FORWARD)
	arraySort(local_map, SMapMapOrder) ;
      else
	arraySort(local_map, SMapMapRevOrder) ;


      /* Fill in any blank ->s2 and all ->r2 values for all but the last one which
       * is a special case as if there is no s2 length it cannot simply be inferred from
       * the next block. Note that each r2/s2 is inferred from the r1 or s1 of the next
       * block depending on which has the smaller gap. */
      for (i = 0 ; i < arrayMax(local_map) - 1 ; ++i)
	{
	  int r_diff, s_diff ;

	  m = arrp(local_map, i, SMapMap) ;


	  if (m->s2)
	    {
	      /* s2 set by user so use it to set diff for r2 calc. */

	      diff = abs(m->s2 - m->s1) ;
	    }
	  else
	    {
	      /* No s2 so calculate number of bases in between the r1's/s1's of adjacent blocks
	       * and use the smaller diff to calculate s2 and r2. Note that diff's are "- 1"
	       * because we went to go up to the base _before_ the next block. */

	      r_diff = abs((m+1)->r1 - m->r1) - 1 ;
	      s_diff = abs((m+1)->s1 - m->s1) - 1 ;

	      if ((s_diff / pepFac) < (r_diff / DNAfac))
		diff = s_diff ;
	      else
		diff = (r_diff / DNAfac) * pepFac ;


	      if (match_strand == STRAND_FORWARD)
		m->s2 = m->s1 + diff ;
	      else
		m->s2 = m->s1 - diff ;
	    }


	  if (ref_strand == STRAND_FORWARD)
	    m->r2 = m->r1 + ((diff * DNAfac) / pepFac) ;
	  else
	    m->r2 = m->r1 - ((diff * DNAfac) / pepFac) ;
	}

      /* Fill in the last one, we use the child end coord as the last ->s2 if s2 is zero
       * and then fill in the last ->r2 as above, note calculation is 1 different because
       * we need to go up to and _including_ the final coord. */
      m = arrp(local_map, arrayMax(local_map) - 1, SMapMap) ;

      if (!m->s2)
	m->s2 = c_end ;

      if (match_strand == STRAND_FORWARD)
	diff = m->s2 - m->s1 + 1 ;
      else
	diff = m->s1 - m->s2 + 1 ;

      if (ref_strand == STRAND_FORWARD)
	m->r2 = m->r1 + (((diff * DNAfac) / pepFac) - 1) ;
      else
	m->r2 = m->r1 - (((diff * DNAfac) / pepFac) - 1) ;
    }


  /* If all is good then return the data. */
  if (status == SMAP_STATUS_PERFECT_MAP)
    {
      if (conv_type_out)
	*conv_type_out = convtype ;

      if (local_map_out)
	*local_map_out = local_map ;
      else
	arrayDestroy(local_map) ;

      if (ref_strand_out)
	*ref_strand_out = ref_strand ;

      if (match_strand_out)
	*match_strand_out = match_strand ;

      if (child_start_out)
	*child_start_out = child_start ;
    }


  /* Clear up..... */
 clear_up:
  arrayDestroy(u) ;

  return status ;
}





/* 
 *                  Internal routines.
 */



/* Inspects match_str to find out which format its in. If the format is not known
 * returns ALIGNSTR_INVALID.
 * 
 * Supported formats are (in regexp ignoring greedy matches):
 * 
 *  exonerate cigar:       (([DIM]) ([0-9]+))*              e.g. "M 24 D 3 M 1 I 86"
 * 
 * exonerate vulgar:       (([DIMCGN53SF])  ([0-9]+) ([0-9]+))*  e.g. "M 24 24 D 0 3 M 1 1 I 86 0"
 * 
 *    ensembl cigar:       (([0-9]+)?([DIM]))*              e.g. "24MD3M86I" or "MD3M86"
 * 
 * NOTE exonerate labels are dependent on the alignment type, it would appear that "I" could be
 * Intron or Insert....
 * 
 *  */
static SMapAlignStrFormat alignStrGetFormat(char *match_str)
{
  SMapAlignStrFormat align_format = ALIGNSTR_INVALID ;

  messAssert(match_str && *match_str) ;

  if (*(match_str + 1) != ' ')				    /* Crude but effective ? */
    {
      align_format = ALIGNSTR_ENSEMBL_CIGAR ;
    }
  else
    {
      char *cp = match_str ;

      if (g_ascii_isalpha(*cp)
	  && ((cp = nextWord(cp)) && g_ascii_isdigit(*cp))
	  && (cp = nextWord(cp)))
	{
	  if (!g_ascii_isdigit(*cp))
	    align_format = ALIGNSTR_EXONERATE_CIGAR ;
	  else
	    align_format = ALIGNSTR_EXONERATE_VULGAR ;
	}
    }

  return align_format ;
}


/* Takes a match string and format and verifies that the string is valid
 * for that format. */
static BOOL alignStrVerifyStr(char *match_str, SMapAlignStrFormat align_format)
{

  /* We should be using some kind of state machine here, setting a model and
   * then just chonking through it......
   *  */

  BOOL result = FALSE ;

  switch (align_format)
    {
    case ALIGNSTR_EXONERATE_CIGAR:
      result = exonerateVerifyCigar(match_str) ;
      break ;
    case ALIGNSTR_EXONERATE_VULGAR:
      result = exonerateVerifyVulgar(match_str) ;
      break ;
    case ALIGNSTR_ENSEMBL_CIGAR:
      result = ensemblVerifyCigar(match_str) ;
      break ;
    default:
      messcrash("bad  AlignStrFormat: %d", align_format) ;
      break ;
    }

  return result ;
}



/* Takes a match string and format and converts that string into a canonical form
 * for further processing. */
AlignStrCanonical alignStrMakeCanonical(char *match_str, SMapAlignStrFormat align_format)
{
  AlignStrCanonical canon = NULL ;
  BOOL result = FALSE ;

  canon = messalloc(sizeof(AlignStrCanonicalStruct)) ;
  canon->format = align_format ;
  canon->align = g_array_new(FALSE, TRUE, sizeof(AlignStrOpStruct)) ;

  switch (align_format)
    {
    case ALIGNSTR_EXONERATE_CIGAR:
      result = exonerateCigar2Canon(match_str, canon) ;
      break ;
    case ALIGNSTR_EXONERATE_VULGAR:
      result = exonerateVulgar2Canon(match_str, canon) ;
      break ;
    case ALIGNSTR_ENSEMBL_CIGAR:
      result = ensemblCigar2Canon(match_str, canon) ;
      break ;
    default:
      messcrash("bad  AlignStrFormat: %d", align_format) ;
      break ;
    }

  if (!result)
    {
      g_array_free(canon->align, TRUE) ;
      messfree(canon) ;
      canon = NULL ;
    }

  return canon ;
}



/* Takes a string in one of the formats given by align_format and works out
 * whether its a dna->dna or pep->dna alignment. It does this by adding up
 * the length of the reference as given in the match string and comparing
 * it to the length as given by the reported start/end in the reference sequence. */
static SMapConvType alignStrGetType(int ref_start, int ref_end, AlignStrCanonical canon)
{
  SMapConvType align_type = SMAPCONV_INVALID ;
  int ref_length, align_length, i ;
  GArray *align = canon->align ;

  if (ref_start < ref_end)
    ref_length = ref_end - ref_start + 1 ;
  else
    ref_length = ref_start - ref_end + 1 ;

  align_length = 0 ;

  for (i = 0 ; i < align->len ; i++)
    {
      AlignStrOp op ;

      op = &(g_array_index(align, AlignStrOpStruct, i)) ;

      if (op->op == 'M' || op->op == 'D')
	align_length += op->length ;
    }

  if (ref_length == align_length)
    align_type = SMAPCONV_DNADNA ;
  else if (ref_length == (align_length * 3))
    align_type = SMAPCONV_PEPDNA ;
  else if (ref_length == (align_length / 3))
    align_type = SMAPCONV_DNAPEP ;
  else
    align_type = SMAPCONV_INVALID ;

  return align_type ;
}


/* Convert the canonical "string" to an acedb gaps array, note that we do this blindly
 * assuming that everything is ok because we have verified the string....
 * 
 * Note that this routine does not support vulgar unless the vulgar string only contains
 * M, I and G (instead of D).
 * 
 * 
 *  */
static BOOL alignStrCanon2Acedb(AlignStrCanonical canon, SMapStrand ref_strand, SMapStrand match_strand,
				STORE_HANDLE handle, OBJ obj,
				int p_start, int p_end, int c_start, int c_end,
				Array *local_map_out)
{
  BOOL result = TRUE ;
  int curr_ref, curr_match ;
  int i, j ;
  Array local_map = NULL ;
  GArray *align = canon->align ;

  local_map = arrayHandleCreate(10, SMapMap, handle) ;

  j = 0 ;
  curr_ref = p_start ;
  curr_match = c_start ;
  for (i = 0 ; i < align->len ; i++)
    {
      /* If you alter this code be sure to notice the += and -= which alter curr_ref and curr_match. */
      AlignStrOp op ;
      int curr_length ;
      SMapMap *map ;

      op = &(g_array_index(align, AlignStrOpStruct, i)) ;

      curr_length = op->length ;

      switch (op->op)
	{
	case 'D':					    /* Deletion in reference sequence. */
	case 'G':
	  {
	    if (ref_strand == STRAND_FORWARD)
	      curr_ref += curr_length ;
	    else
	      curr_ref -= curr_length ;

	    break ;
	  }
	case 'I':					    /* Insertion from match sequence. */
	  {
	    if (match_strand == STRAND_FORWARD)
	      curr_match += curr_length ;
	    else
	      curr_match -= curr_length ;

	    break ;
	  }
	case 'M':					    /* Match. */
	  {
	    map = arrayp(local_map, j, SMapMap) ;		

	    map->r1 = curr_ref ;

	    if (ref_strand == STRAND_FORWARD)
	      map->r2 = (curr_ref += curr_length) - 1 ;
	    else
	      map->r2 = (curr_ref -= curr_length) + 1 ;

	    map->s1 = curr_match ;

	    if (match_strand == STRAND_FORWARD)
	      map->s2 = (curr_match += curr_length) - 1 ;
	    else
	      map->s2 = (curr_match -= curr_length) + 1 ;

	    j++ ;					    /* increment for next smapmap element. */
	    break ;
	  }
	default:
	  {
	    messcrash("unreachable code reached, bad cigar op: '%c' !", op->op) ;

	    break ;
	  }
	}
    }

  *local_map_out = local_map ;

  return result ;
}


/* Format of an exonerate vulgar string is:
 * 
 * "operator number number" repeated as necessary. Operators are D, I or M.
 * 
 * A regexp (without dealing with greedy matches) would be something like:
 *
 *                           (([MCGN53ISF]) ([0-9]+) ([0-9]+))*
 * 
 *  */
static BOOL exonerateVerifyVulgar(char *match_str)
{
  BOOL result = TRUE ;
  typedef enum {STATE_OP, STATE_SPACE_OP,
		STATE_NUM1, STATE_SPACE_NUM1, STATE_NUM2, STATE_SPACE_NUM2} VulgarStates ;
  char *cp = match_str ;
  VulgarStates state ;
  
  state = STATE_OP ;
  do 
    {
      switch (state)
	{
	case STATE_OP:
	  if (*cp == 'M' || *cp == 'C' || *cp == 'G'
	      || *cp == 'N' || *cp == '5' || *cp == '3'
	      || *cp == 'I' || *cp == 'S' || *cp == 'F')
	    state = STATE_SPACE_OP ;
	  else
	    result = FALSE ;
	  break ;
	case STATE_SPACE_OP:
	  if (gotoLastSpace(&cp))
	    state = STATE_NUM1 ;
	  else
	    result = FALSE ;
	  break ;
	case STATE_NUM1:
	  if (gotoLastDigit(&cp))
	    state = STATE_SPACE_NUM1 ;
	  else
	    result = FALSE ;
	  break ;
	case STATE_SPACE_NUM1:
	  if (gotoLastSpace(&cp))
	    state = STATE_NUM2 ;
	  else
	    result = FALSE ;
	  break ;
	case STATE_NUM2:
	  if (gotoLastDigit(&cp))
	    state = STATE_SPACE_NUM2 ;
	  else
	    result = FALSE ;
	  break ;
	case STATE_SPACE_NUM2:
	  if (gotoLastSpace(&cp))
	    state = STATE_OP ;
	  else
	    result = FALSE ;
	  break ;
	}

      if (result)
	cp++ ;
      else
	break ;

    } while (*cp) ;

  return result ;
}


/* Format of an exonerate cigar string is:
 * 
 * "operator number" repeated as necessary. Operators are D, I or M.
 * 
 * A regexp (without dealing with greedy matches) would be something like:
 *
 *                           (([DIM]) ([0-9]+))*
 * 
 *  */
static BOOL exonerateVerifyCigar(char *match_str)
{
  BOOL result = TRUE ;
  typedef enum {STATE_OP, STATE_SPACE_OP, STATE_NUM, STATE_SPACE_NUM} CigarStates ;
  char *cp = match_str ;
  CigarStates state ;
  
  state = STATE_OP ;
  do 
    {
      switch (state)
	{
	case STATE_OP:
	  if (*cp == 'M' || *cp == 'D' || *cp == 'I')
	    state = STATE_SPACE_OP ;
	  else
	    result = FALSE ;
	  break ;
	case STATE_SPACE_OP:
	  if (gotoLastSpace(&cp))
	    state = STATE_NUM ;
	  else
	    result = FALSE ;
	  break ;
	case STATE_NUM:
	  if (gotoLastDigit(&cp))
	    state = STATE_SPACE_NUM ;
	  else
	    result = FALSE ;
	  break ;
	case STATE_SPACE_NUM:
	  if (gotoLastSpace(&cp))
	    state = STATE_OP ;
	  else
	    result = FALSE ;
	  break ;
	}

      if (result)
	cp++ ;
      else
	break ;

    } while (*cp) ;

  return result ;
}



/* Format of an ensembl cigar string is:
 * 
 * "[optional number]operator" repeated as necessary. Operators are D, I or M.
 * 
 * A regexp (without dealing with greedy matches) would be something like:
 *
 *                           (([0-9]+)?([DIM]))*
 * 
 *  */
static BOOL ensemblVerifyCigar(char *match_str)
{
  BOOL result = TRUE ;
  typedef enum {STATE_OP, STATE_NUM} CigarStates ;
  char *cp = match_str ;
  CigarStates state ;
  
  state = STATE_NUM ;
  do 
    {
      switch (state)
	{
	case STATE_NUM:
	  if (!gotoLastDigit(&cp))
	    cp-- ;

	  state = STATE_OP ;
	  break ;
	case STATE_OP:
	  if (*cp == 'M' || *cp == 'D' || *cp == 'I')
	    state = STATE_NUM ;
	  else
	    result = FALSE ;
	  break ;
	}

      if (result)
	cp++ ;
      else
	break ;

    } while (*cp) ;

  return result ;
}


/* Blindly converts, assumes match_str is a valid exonerate cigar string. */
BOOL exonerateCigar2Canon(char *match_str, AlignStrCanonical canon)
{
  BOOL result = TRUE ;
  char *cp ;

  cp = match_str ;
  do
    {
      AlignStrOpStruct op = {'\0'} ;

      op.op = *cp ;
      cp++ ;
      gotoLastSpace(&cp) ;
      cp++ ;

      op.length = cigarGetLength(&cp) ;			    /* N.B. moves cp on as needed. */

      gotoLastSpace(&cp) ;
      if (*cp == ' ')
	cp ++ ;

      canon->align = g_array_append_val(canon->align, op) ;
    } while (*cp) ;

  return result ;
}


/* Not straight forward to implement, wait and see what we need.... */
BOOL exonerateVulgar2Canon(char *match_str, AlignStrCanonical canon)
{
  BOOL result = FALSE ;


  return result ;
}


/* Blindly converts, assumes match_str is a valid ensembl cigar string. */
BOOL ensemblCigar2Canon(char *match_str, AlignStrCanonical canon)
{
  BOOL result = TRUE ;
  char *cp ;

  cp = match_str ;
  do
    {
      AlignStrOpStruct op = {'\0'} ;

      if (g_ascii_isdigit(*cp))
	op.length = cigarGetLength(&cp) ;		    /* N.B. moves cp on as needed. */
      else
	op.length = 1 ;

      op.op = *cp ;

      canon->align = g_array_append_val(canon->align, op) ;

      cp++ ;
    } while (*cp) ;

  return result ;
}



/* We could do more in here to check more thoroughly..... */
int cigarGetLength(char **cigar_str)
{
  int length = 0 ;
  char *old_cp = *cigar_str, *new_cp = NULL ;

  errno = 0 ;
  length = strtol(old_cp, &new_cp, 10) ;
  messAssert(!errno) ;

  if (new_cp == old_cp)
    length = 1 ;

  *cigar_str = new_cp ;

  return length ;
}


/* Put in utils somewhere ????
 * 
 * Whether str is on a word or on space, moves to the _next_ word.
 * Returns NULL if there is no next word or at end of string.
 * 
 *  */
static char *nextWord(char *str)
{
  char *word = NULL ;

  if (str && *str)
    {
      char *cp = str ;

      while (*cp)
	{
	  if (*cp == ' ')
	    {
	      break ;
	    }

	  cp++ ;
	}

      while (*cp)
	{
	  if (*cp != ' ')
	    {
	      word = cp ;
	      break ;
	    }

	  cp++ ;
	}
    }

  return word ;
}


BOOL gotoLastDigit(char **cp_inout)
{
  BOOL result = FALSE ;
  char *cp = *cp_inout ;

  if (g_ascii_isdigit(*cp))
    {
      cp++ ;

      while (*cp && g_ascii_isdigit(*cp))
	cp++ ;
	      
      cp-- ;					    /* position on last digit. */

      *cp_inout = cp ;
      result = TRUE ;
    }


  return result ;
}

BOOL gotoLastSpace(char **cp_inout)
{
  BOOL result = FALSE ;
  char *cp = *cp_inout ;

  if (*cp == ' ')
    {
      cp++ ;

      while (*cp && *cp == ' ')
	cp++ ;
	      
      cp-- ;					    /* position on last space. */

      *cp_inout = cp ;
      result = TRUE ;
    }


  return result ;
}


static int SMapMapOrder(void *va, void *vb)
{
  SMapMap *a = (SMapMap*)va ;
  SMapMap *b = (SMapMap*)vb ;

  return (a->s1 - b->s1) ;
}

static int SMapMapRevOrder(void *va, void *vb)
{
  SMapMap *a = (SMapMap*)va ;
  SMapMap *b = (SMapMap*)vb ;

  return (b->s1 - a->s1) ;
}



  

/*********************** end of file ************************/
