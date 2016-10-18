/*  File: cdna.h
 *  Author: mieg
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
 * Exported functions:
 * HISTORY:
 * Last edited: Nov 15 15:54 1999 (fw)
 * Created: Thu Aug 26 17:55:00 1999 (fw)
 * CVS info:   $Id: cdna.h,v 1.11 2000-04-14 21:47:04 mieg Exp $
 *-------------------------------------------------------------------
 */

#ifndef ACEDB_CDNA_H
#define ACEDB_CDNA_H

#include "acedb.h"
#include "whooks/sysclass.h"

extern Array fMapcDNAReferenceDna ;
extern Array fMapcDNAReferenceHits ;
extern KEY fMapcDNAReferenceGene ;
extern KEY fMapcDNAReferenceEst ;
extern int fMapcDNAReferenceOrigin ;

Array cDNAGetReferenceHits (KEY gene, int origin) ;
KEY cDNARealignGene (KEY gene, int z1, int z2) ;
void fMapcDNADoSelect(KEY k, KEYSET ks, KEYSET taceKs) ;
KEY cDNAMakeDna (KEY gene) ;

typedef struct hitStruct { KEY gene, cDNA_clone, est ; BOOL reverse ; 
  int a1, a2, x1, x2, clipTop, clipEnd ; int type, zone ; Array errArray ; } HIT ;


extern  char B2[255] ;
extern  KEY 
  _VTranscribed_gene, _Transcribed_gene,
  _VTranscript, _Transcript,
  _VAnnotation,
  _VcDNA_clone, _VSage, _Sage,
  _VClone_Group , _Clone_Group, _From_gene,
  _cDNA_clone, _Hit ,  _Forward, _Reverse, 
  _Confirmed_intron , _Confirmed_exon, 
  _First_exon, _Last_exon,
  _Splicing,  _Read,
  _Nb_possible_exons, _Nb_alternative_exons,
  _Nb_confirmed_introns, _Nb_confirmed_alternative_introns,
  _Intron, _Exon, _Gap,
  _Alternative_exon,  _Alternative_first_exon, _Alternative_intron,
  _Partial_exon,  _Alternative_partial_exon, 
  _3prime_UTR, _5prime_UTR,
  _Begin_not_found,  /* _End_not_found,  */
  _Total_length, _Total_intron_length, _Total_gap_length,
  _Transpliced_to, _Transcribed_from, _Genomic_sequence,
  _PolyA_after_base, _Intron_boundaries,

  _Fake_internal_poly_A, _Vector_clipping, _Reversed_by, _Other,
  _Read_matchingLength_nerrors, _PCR_product_size, _Primed_on_polyA,
  _Assembled_from_cDNA_clone, _Genomic_sequence,
  _Spliced_sequence, _From_gene, _NewName, _3prime_UTR_length, _Longest_cDNA_clone,
  _Overlap_right, _Overlap_left, _Is_chain, _Is_gap, _Genomic,
  _IntMap, _Hits, _Mosaic, _Problem, _Gene_wall, _Relabelled,

  _Discarded_cDNA, _Discarded_from,
  _Derived_sequence,
  _CTF_File ;

void cDNAAlignInit (void) ;
void cDNARemoveCloneFromGene (KEY clone, KEY gene); /* cdnaalign.c */

#endif /* !ACEDB_CDNA_H */
