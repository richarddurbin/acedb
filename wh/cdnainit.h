/*  File: cdnainit.h
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
 * Last edited: Aug 26 17:56 1999 (fw)
 * Created: Thu Aug 26 17:55:41 1999 (fw)
 * CVS info:   $Id: cdnainit.h,v 1.8 2000-04-14 21:46:38 mieg Exp $
 *-------------------------------------------------------------------
 */

#ifndef ACEDB_CDNAINIT_H
#define ACEDB_CDNAINIT_H

char B2[255] ;
KEY 
  _VTranscribed_gene, _Transcribed_gene,
  _VTranscript, _Transcript,
  _VAnnotation,
  _VcDNA_clone, _VSage, _Sage,
  _VClone_Group , _Clone_Group, _From_gene,
  _cDNA_clone, _Hit = 0,  _Forward, _Reverse, 
  _Confirmed_intron = 0, _Confirmed_exon = 0, _First_exon = 0, _Last_exon = 0, 
  _Splicing, _Read, 
  _Nb_possible_exons, _Nb_alternative_exons,
  _Nb_confirmed_introns, _Nb_confirmed_alternative_introns,
  _Intron, _Exon, _Gap,
  _Alternative_exon, _Alternative_first_exon, _Alternative_intron,
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
  _IntMap, _Hits, _Problem, _Gene_wall, _Mosaic, _Relabelled,

  _Discarded_cDNA, _Discarded_from, 
  _Derived_sequence,
  _CTF_File ;

void cDNAAlignInit (void)
{
  int i ; KEY key ;

  if (_Hit)
    return ;

  i = 256 ;  while (i--) B2[i] = 0 ;
  B2[A_] = 0x0 ; B2[T_] = 0x3 ;
  B2[G_] = 0x1 ; B2[C_] = 0x2 ;

  lexaddkey ("Transcribed_gene", &key, _VMainClasses) ; 
  _VTranscribed_gene = KEYKEY(key) ;
  lexaddkey ("Transcript", &key, _VMainClasses) ; 
  _VTranscript = KEYKEY(key) ;
  lexaddkey ("cDNA_clone", &key, _VMainClasses) ; 
  _VcDNA_clone = KEYKEY(key) ;
  lexaddkey ("Clone_Group", &key, _VMainClasses) ; 
  _VClone_Group = KEYKEY(key) ;
  lexaddkey ("Annotation", &key, _VMainClasses) ; 
  _VAnnotation = KEYKEY(key) ;
  lexaddkey ("Sage", &key, _VMainClasses) ; 
  _VSage = KEYKEY(key) ;

  _Transcribed_gene = str2tag ("Transcribed_gene") ;  
  _Transcript = str2tag ("Transcript") ;  
  _From_gene = str2tag ("From_gene") ;  
  _cDNA_clone = str2tag ("cDNA_clone") ; 
  _Sage = str2tag ("Sage") ; 
  _Hit = str2tag ("Hit") ; 
  _Forward = str2tag ("Forward") ;
  _Reverse = str2tag ("Reverse") ;
  _Confirmed_intron = str2tag ("Confirmed_intron") ;
  _Confirmed_exon = str2tag ("Confirmed_exon") ;
  _First_exon = str2tag ("First_exon") ;
  _Last_exon = str2tag ("Last_exon") ;
  _Splicing = str2tag ("Splicing") ;
  _Read = str2tag ("Read") ;
  _Clone_Group = str2tag ("Clone_Group") ;
  _Nb_possible_exons = str2tag ("Nb_possible_exons") ; 
  _Nb_alternative_exons = str2tag ("Nb_alternative_exons") ; 
  _Nb_confirmed_introns = str2tag ("Nb_confirmed_introns") ; 
  _Nb_confirmed_alternative_introns = str2tag ("Nb_confirmed_alternative_introns") ; 
  _Intron = str2tag ("Intron") ; 
  _Exon = str2tag ("Exon") ; 
  _Gap = str2tag ("Gap") ; 
  _Alternative_exon = str2tag ("Alternative_exon") ; 
  _Alternative_first_exon = str2tag ("Alternative_first_exon") ; 
  _Alternative_intron = str2tag ("Alternative_intron") ; 
  _Partial_exon = str2tag ("Partial_exon") ; 
  _Alternative_partial_exon = str2tag ("Alternative_partial_exon") ; 
  _3prime_UTR = str2tag ("3prime_UTR") ;
  _5prime_UTR = str2tag ("5prime_UTR") ; 
  _Begin_not_found = str2tag ("Begin_not_found") ; 
  /*   _End_not_found = str2tag ("End_not_found") ;  in tags.h */
  _Total_length = str2tag ("Total_length") ; 
  _Total_intron_length = str2tag ("Total_intron_length") ; 
  _Total_gap_length = str2tag ("Total_gap_length") ; 
  _Transpliced_to = str2tag ("Transpliced_to") ; 
  _Transcribed_from = str2tag ("Transcribed_from") ; 
  _Genomic_sequence = str2tag ("Genomic_sequence") ; 
  _PolyA_after_base = str2tag ("PolyA_after_base") ;  
  _Intron_boundaries = str2tag ("Intron_boundaries") ;  
  _Discarded_cDNA = str2tag ("Discarded_cDNA") ; 
  _Discarded_from = str2tag ("Discarded_from") ; 
  _Derived_sequence = str2tag ("Derived_sequence") ; 
  _CTF_File = str2tag ("CTF_File") ; 
  _Fake_internal_poly_A = str2tag ("Fake_internal_poly_A") ;  
  _Vector_clipping = str2tag ("Vector_clipping") ;  
  _Reversed_by = str2tag ("Reversed_by") ;  
  _Other = str2tag ("Other") ;  
  _Read_matchingLength_nerrors = str2tag ("Read_matchingLength_nerrors") ;  
  _PCR_product_size = str2tag ("PCR_product_size") ;  
  _Primed_on_polyA = str2tag ("Primed_on_polyA") ;  
  _Assembled_from_cDNA_clone = str2tag ("Assembled_from_cDNA_clone") ;  
  _Genomic_sequence = str2tag ("Genomic_sequence") ;  
  _Spliced_sequence = str2tag ("Spliced_sequence") ;  
  _From_gene = str2tag ("From_gene") ;  
  _NewName = str2tag ("NewName") ;  
  _3prime_UTR_length = str2tag ("3prime_UTR_length") ;  
  _Longest_cDNA_clone = str2tag ("Longest_cDNA_clone") ;  
  _Overlap_right = str2tag ("Overlap_right") ;  
  _Overlap_left = str2tag ("Overlap_left") ;  
  _Is_chain = str2tag ("Is_chain") ;  
  _Is_gap = str2tag ("Is_gap") ;  
  _Genomic = str2tag ("Genomic") ;  
  _IntMap = str2tag ("IntMap") ;  
  _Hits = str2tag ("Hits") ; 
  _Mosaic = str2tag ("Mosaic") ;  
  _Problem = str2tag ("Problem") ;  
  _Gene_wall = str2tag ("Gene_wall") ;
  _Relabelled = str2tag ("Relabelled") ;
  /*   _ = str2tag ("") ;  */
}
#endif /* !ACEDB_CDNAINIT_H */
