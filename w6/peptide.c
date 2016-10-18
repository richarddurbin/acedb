/*  File: peptide.c
 *  Author: Richard Durbin (rd@sanger.ac.uk)
 *  Copyright (C) J Thierry-Mieg and R Durbin, 1994
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
 * 	Richard Durbin (MRC LMB, UK) rd@mrc-lmb.cam.ac.uk, and
 *	Jean Thierry-Mieg (CRBM du CNRS, France) mieg@kaa.cnrs-mop.fr
 *
 * Description:
 * Exported functions:
 * HISTORY:
 * Last edited: Dec 21 09:52 2009 (edgrif)
 * Created: Tue May 10 23:59:13 1994 (rd)
 * CVS info:   $Id: peptide.c,v 1.69 2009-12-21 10:49:37 edgrif Exp $
 *-------------------------------------------------------------------
 */

#include <wh/acedb.h>
#include <wh/aceio.h>
#include <wh/a.h>
#include <wh/bs.h>
#include <wh/peptide.h>
#include <wh/lex.h>
#include <wh/bindex.h>
#include <wh/pick.h>
#include <whooks/sysclass.h>
#include <whooks/classes.h>
#include <whooks/systags.h>
#include <whooks/tags.h>
#include <wh/dna.h>
#include <wh/java.h>
#include <wh/chrono.h>
#include <wh/smap.h>


/********************************************************/


enum {PEP_BASES = 4, PEP_CODON_LENGTH = 3, PEP_TOTAL_CODONS = PEP_BASES * PEP_BASES * PEP_BASES} ;


/* Used to construct a translation table for a particular genetic code.
 * The table is a 64 element array, the order of this string is vital since
 * translation of the DNA is done by an indexed lookup into this array
 * based on the way the DNA is encoded, so you should not reorder this array.
 * Each element also has two associated bools which control special processing
 * for the start/stop of the peptide which is required for some alternative 
 * starts and stops:
 * 
 * alternative_start = TRUE means that this amino acid should be replaced with
 *                          methionine when at the start of a transcript.
 *  alternative_stop = TRUE means that this amino acid should be replaced with
 *                          a stop when at the end of a transcript. */
typedef struct
{
  char amino_acid ;
  BOOL alternative_start ;
  BOOL alternative_stop ;
} PepTranslateStruct, *PepTranslate ;


/* This is the standard genetic code for most organisms, it is used to translate
 * dna into peptides unless an alternative has been specified in the database. */
#define STANDARD_GENETIC_CODE "KNKNIIMIRSRSTTTT*Y*YLFLF*CWCSSSSEDEDVVVVGGGGAAAAQHQHLLLLRRRRPPPP"



/********************************************************/


static void writeTranslationTitle(ACEOUT fo, KEY seq) ;
static BOOL pepDoDumpFastAKey(ACEOUT dump_out, KEY key, BOOL cds_only, char style) ;
static int pepDoDumpFastAKeySet(ACEOUT dump_out, KEYSET kSet, BOOL cds_only) ;
static void printTranslationTable(KEY genetic_code_key, Array translation_table) ;
static char E_codon(char *s, Array genetic_code, int *index_out) ;
static char E_reverseCodon (char* cp, Array genetic_code, int *index_out) ;
static char E_antiCodon (char* cp, Array genetic_code, int *index_out) ;
static Array doDNATranslation(Array code_table, Array obj_dna, BOOL encode, BOOL include_stop) ;


/********************************************************/


static BOOL pep_debug_G = FALSE ;			    /* Set to TRUE for debugging output. */



/* The naming of amino acids:

There are 4 names:
- a number: 0, 1, 2, etc.
- a single character: A, C, D, etc
- an abbreviated name, Ala, Cys, etc
- a full name, Alanine, Cysteine, etc

All but the number are standardized, though I have not actually seen
a standards document.  (ms - 7/2002)

The protein names:

0  A  Ala  Alanine
1  C  Cys  Cysteine
2  D  Asp  Aspartate (also Aspartic Acid)
3  E  Glu  Glutamate (also Glutamic Acid)
4  F  Phe  Phenylalanine
5  G  Gly  Glycine
6  H  His  Histidine
7  I  Ile  Isoleucine
8  K  Lys  Lysine
9  L  Leu  Leucine
10 M  Met  Methionine
11 N  Asn  Asparagine
12 P  Pro  Proline
13 Q  Gln  Glutamine
14 R  Arg  Arginine
15 S  Ser  Serine
16 T  Thr  Threonine
17 V  Val  Valine
18 W  Trp  Tryptophan
19 Y  Tyr  Tyrosine
20 X  Xxx  Unknown / Undetermined
21 .  ???  ??? ( nobody knows what this is doing here; only acedb has it. )
22 *  stp  Stop / Terminate transcription
23 Z  Glx  Glu/Gln
24 B  Asx  Asp/Asn
25 U  Sec  Selenocysteine ( defined recently, often missing from other lists )


  notes for maintaining these tables:

  'A' is 0x41
  '*' is 0x2a
  '.' is 0x2e
  '0' is 0x30

  don't know why digits and backspace are -4 in pepEncodeChar

pepDecodeChar[] turns a protein number into the printable character.
pepEncodeChar[] turns a protein character into the correct number.
pepName[] turns a protein character into the long name.
pepShortName[] turns a protein character into an abbreviated name.

*/


char pepDecodeChar[] =

/* 0    1    2    3    4    5    6    7    8    9   10   11   12 */

{ 'A', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'K', 'L', 'M', 'N', 'P',
  'Q', 'R', 'S', 'T', 'V', 'W', 'Y', 'X', '.', '*', 'Z', 'B', 'U'} ;

/* 13   14   15   16   17   18   19   20   21   22  23   24   25 */


signed char pepEncodeChar[] =
{ 
/* 00 */ -1, -1, -1, -1,  -1, -1, -1, -1,  -4, -1, -1, -1,  -1, -1, -1, -1,
/* 10 */ -1, -1, -1, -1,  -1, -1, -1, -1,  -1, -1, -1, -1,  -1, -1, -1, -1,
/* 20 */ -4, -1, -1, -1,  -1, -1, -1, -1,  -1, -1, 22, -1,  -1, -1, 21, -1,  /* '*' */
/* 30 */ -4, -4, -4, -4,  -4, -4, -4, -4,  -4, -4, -1, -1,  -1, -1, -1, -1,  /* digits */

/* 40 */ -1,  0, 24,  1,   2,  3,  4,  5,   6,  7, -1,  8,   9, 10, 11, -1,
/* 50 */ 12, 13, 14, 15,  16, 25, 17, 18,  20, 19, 23, -1,  -1, -1, -1, -1,
/* 60 */ -1, -1, -1, -1,  -1, -1, -1, -1,  -1, -1, -1, -1,  -1, -1, -1, -1,
/* 70 */ -1, -1, -1, -1,  -1, -1, -1, -1,  -1, -1, -1, -1,  -1, -1, -1, -1

/* 80 */ -1, -1, -1, -1,  -1, -1, -1, -1,  -1, -1, -1, -1,  -1, -1, -1, -1
/* 90 */ -1, -1, -1, -1,  -1, -1, -1, -1,  -1, -1, -1, -1,  -1, -1, -1, -1
/* a0 */ -1, -1, -1, -1,  -1, -1, -1, -1,  -1, -1, -1, -1,  -1, -1, -1, -1
/* b0 */ -1, -1, -1, -1,  -1, -1, -1, -1,  -1, -1, -1, -1,  -1, -1, -1, -1

/* c0 */ -1, -1, -1, -1,  -1, -1, -1, -1,  -1, -1, -1, -1,  -1, -1, -1, -1
/* d0 */ -1, -1, -1, -1,  -1, -1, -1, -1,  -1, -1, -1, -1,  -1, -1, -1, -1
/* e0 */ -1, -1, -1, -1,  -1, -1, -1, -1,  -1, -1, -1, -1,  -1, -1, -1, -1
/* f0 */ -1, -1, -1, -1,  -1, -1, -1, -1,  -1, -1, -1, -1,  -1, -1, -1, -1

} ;


char *pepName[] =
{  
/* 00 */ 0,  0,  0,  0,   0,  0,  0,  0,   0,  0,  0,  0,   0,  0,  0,  0, 
/* 10 */ 0,  0,  0,  0,   0,  0,  0,  0,   0,  0,  0,  0,   0,  0,  0,  0, 
/* 20 */ 0,  0,  0,  0,   0,  0,  0,  0,   0,  0,  "Stop",
						       0,   0, 0,  "???",  0,
/* 30 */ 0,  0,  0,  0,   0,  0,  0,  0,   0,  0,  0,  0,   0,  0,  0,  0, 

/* 40 */ 0, 		"Alanine", 	 "Asp/Asn", 	"Cysteine", 
/* 44 */"Aspartate", 	"Glutamate", 	 "Phenylalanine","Glycine", 
/* 48 */"Histidine", 	"Isoleucine", 	 0, 		"Lysine", 
/* 4c */"Leucine", 	"Methionine", 	 "Asparagine", 	0,

/* 50 */"Proline", 	"Glutamine", 	 "Arginine", 	"Serine", 
/* 54 */"Threonine", 	"Selenocysteine" ,"Valine", 	"Tryptophan", 
/* 58 */"Unknown", 	"Tyrosine", 	 "Glu/Gln",  	0,  
/* 5a */ 0,  		0,  		0,  		0, 

/* 60 */ 0,  0,  0,  0,   0,  0,  0,  0,   0,  0,  0,  0,   0,  0,  0,  0, 
/* 70 */ 0,  0,  0,  0,   0,  0,  0,  0,   0,  0,  0,  0,   0,  0,  0,  0, 

/* 80 */ 0,  0,  0,  0,   0,  0,  0,  0,   0,  0,  0,  0,   0,  0,  0,  0, 
/* 90 */ 0,  0,  0,  0,   0,  0,  0,  0,   0,  0,  0,  0,   0,  0,  0,  0, 
/* a0 */ 0,  0,  0,  0,   0,  0,  0,  0,   0,  0,  0,  0,   0,  0,  0,  0, 
/* b0 */ 0,  0,  0,  0,   0,  0,  0,  0,   0,  0,  0,  0,   0,  0,  0,  0, 

/* c0 */ 0,  0,  0,  0,   0,  0,  0,  0,   0,  0,  0,  0,   0,  0,  0,  0, 
/* d0 */ 0,  0,  0,  0,   0,  0,  0,  0,   0,  0,  0,  0,   0,  0,  0,  0, 
/* e0 */ 0,  0,  0,  0,   0,  0,  0,  0,   0,  0,  0,  0,   0,  0,  0,  0, 
/* f0 */ 0,  0,  0,  0,   0,  0,  0,  0,   0,  0,  0,  0,   0,  0,  0,  0, 
} ;		


char *pepShortName[] =
{  
/* 00 */ 0,  0,  0,  0,   0,  0,  0,  0,   0,  0,  0,  0,   0,  0,  0,  0, 
/* 10 */ 0,  0,  0,  0,   0,  0,  0,  0,   0,  0,  0,  0,   0,  0,  0,  0, 
/* 20 */ 0,  0,  0,  0,   0,  0,  0,  0,   0,  0,  "stp",  0,   0, 0,  "???",  0,
/* 30 */ 0,  0,  0,  0,   0,  0,  0,  0,   0,  0,  0,  0,   0,  0,  0,  0, 

/* 40 */      0, "Ala", "AsX", "Cys",    "Asp", "Glu", "Phe", "Gly", 
/* 48 */  "His", "Ile",     0, "Lys",    "Leu", "Met", "Asn",     0,
/* 50 */  "Pro", "Gln", "Arg", "Ser",    "Thr", "Sec", "Val", "Trp", 
/* 58 */  "Xxx", "Tyr", "Glx",     0,        0,     0,     0,     0, 
 
/* 60 */ 0,  0,  0,  0,   0,  0,  0,  0,   0,  0,  0,  0,   0,  0,  0,  0, 
/* 70 */ 0,  0,  0,  0,   0,  0,  0,  0,   0,  0,  0,  0,   0,  0,  0,  0, 

/* 80 */ 0,  0,  0,  0,   0,  0,  0,  0,   0,  0,  0,  0,   0,  0,  0,  0, 
/* 90 */ 0,  0,  0,  0,   0,  0,  0,  0,   0,  0,  0,  0,   0,  0,  0,  0, 
/* a0 */ 0,  0,  0,  0,   0,  0,  0,  0,   0,  0,  0,  0,   0,  0,  0,  0, 
/* b0 */ 0,  0,  0,  0,   0,  0,  0,  0,   0,  0,  0,  0,   0,  0,  0,  0, 
/* c0 */ 0,  0,  0,  0,   0,  0,  0,  0,   0,  0,  0,  0,   0,  0,  0,  0, 
/* d0 */ 0,  0,  0,  0,   0,  0,  0,  0,   0,  0,  0,  0,   0,  0,  0,  0, 
/* e0 */ 0,  0,  0,  0,   0,  0,  0,  0,   0,  0,  0,  0,   0,  0,  0,  0, 
/* f0 */ 0,  0,  0,  0,   0,  0,  0,  0,   0,  0,  0,  0,   0,  0,  0,  0
} ;		


/***************************************************************/

/* mhmp 23.09.98 */
/* Antobodies A Laboratory Manual  Ed Harlow David Lane p. 661*/
/* Cold Spring Harbor Laboratory 1988 page 661 */
/* je mets le Stop a -1 et les Asp/Asn,  Glu/Gln  et Unknown a 0 */

/* Protein Molecular Weight Section, after Jim Ostell */

/* Values are in  pepDecodeChar order:
   B is really D or N, but they are close so is treated as D
   Z is really E or Q, but they are close so is treated as E
   X is hard to guess, so the calculation fails on X

  A  C  D  E  F  G  H  I  K  L  M  N  P  Q  R  S  T  V  W  Y  X  .  *  Z  B  U
*/

static char C_atoms[26] =
{ 3, 3, 4, 5, 9, 2, 6, 6, 6, 6, 5, 4, 5, 5, 6, 3, 4, 5,11, 9, 0, 0, 0, 5, 4, 3};
static char H_atoms[26] =
{ 5, 5, 5, 7, 9, 3, 7,11,12,11, 9, 6, 7, 8,12, 5, 7, 9,10, 9, 0, 0, 0, 7, 5, 5};
static char N_atoms[26] =
{ 1, 1, 1, 1, 1, 1, 3, 1, 2, 1, 1, 2, 1, 2, 4, 1, 1, 1, 2, 1, 0, 0, 0, 1, 1, 1};
static char O_atoms[26] =
{ 1, 1, 3, 3, 1, 1, 1, 1, 1, 1, 1, 2, 1, 2, 1, 2, 2, 1, 1, 2, 0, 0, 0, 3, 3, 1};
static char S_atoms[26] =
{ 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
static char Se_atoms[26] =
{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1};


/* This seems only to be referenced from fmapcontrol.c, something to be phased
 * out ???? */
int  molecularWeight[] =
{  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, -1,  0,  0,
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 

   0, 89, 133, 121, 133, 147, 165, 75, 155, 131, 0,146, 131, 149, 132, 0,
   115, 146, 174, 105, 119, 168, 117, 204, 120, 181, 147,  0,  0,  0,  0,  0, 
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 

   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0
} ;	


/**************************************************************/
	
static float NH2pK[] =
{  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, -1,  0,  0,
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 

   0, 9.87, 0, 10.78, 9.6, 9.67, 9.24, 9.6, 8.97, 9.76, 0, 8.9, 9.6, 
   9.21, 8.8, 0,
   10.6, 9.13, 9.09, 9.15, 9.12, 10.78, 9.72, 9.39, 0, 9.11, 0,  0,  0, 0,  0,  0, 
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 

   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0
} ;		

static float COOHpK[] =
{  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, -1,  0,  0,
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 

   0, 2.35, 0, 1.71, 1.88, 2.19, 2.58, 2.34, 1.78, 2.32, 0,2.2, 2.36,
   2.28, 2.02, 0,
   1.99, 2.17, 2.18, 2.21, 2.15, 1.71, 2.29, 2.38, 0, 2.2, 0,  0,  0,  0,  0,  0, 
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 

   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0
} ;		

static float LateralpK[] =
{  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, -1,  0,  0,
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 

   0, 0, 0, 8.3, 3.9, 4.3, 0, 0, 6.0, 0, 0, 10.8, 0, 0, 0, 0,
   0, 0, 12.5, 0, 0, 0, 0, 0, 0, 10.9, 0,  0,  0,  0,  0,  0, 
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 

   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0
} ;


static unsigned char dnaStrictEncodeChar[] =
{  0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,

   0,   1,   0,   4,   0,   0,   0,   3,   0,   0,   0,   0,   0,   0,   0,   0,
   0,   0,   0,   0,   2,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
   0,   1,   0,   4,   0,   0,   0,   3,   0,   0,   0,   0,   0,   0,   0,   0,
   0,   0,   0,   0,   2,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
} ;



/**************************************************************/


/* _VProtein is declared in classes.h/class.c */
static BOOL pepInitialise (void)
{ 

  KEY key ;
  if (!lexword2key("?Protein", &key, _VModel) && iskey(key)!=2)
    { 
      messerror("No model for Proteins, please edit wspec/models.wrm") ;
      return FALSE ;
    }

  return TRUE ;
}


/*
 * Extended Codon translator
 *
 * This translates 3 dna bases into the equivalent amino acid. Since mappings
 * between bases and amino acids can change between different organisms, between
 * cell organelles (nucleus v.s. mitochondria, or between transcripts (see
 * Selenocysteine usage) the appropriate mapping must be selected for each
 * piece of DNA to be translated. This is achieved as follows:
 *
 * The Sequence object in the database contains a tag named Genetic_code.
 * That field refers to an object of class Genetic_code that defines the
 * mapping.  You need two steps to make the translation:
 *
 * 1) use pepGetTranslationTable() to translate the genetic_code key into
 *    a char * that is used for the translation.  (returns null on error)
 * 2) pass the char * to e_codon() along with the 3 bases to translate.
 *
 * You never free the value returned by pepGetTranslationTable(); it is
 * cached internally.
 *
 */


/* The e_NNNN calls allow external callers to do simple translation of a codon to
 * an amino acid, no account is taken of alternative start/stop information.
 * 
 * The E_NNNN calls are internal to peptide.c and return information that can be
 * used by the peptide routines to take account of alternative start/stops when
 * translating an entire transcript. */

/* External translation functions. */
char e_codon(char *s, Array genetic_code)
{
  return E_codon(s, genetic_code, NULL) ;
}

char e_reverseCodon(char* cp, Array genetic_code)
{
  return E_reverseCodon(cp, genetic_code, NULL) ;
}

char e_antiCodon(char* cp, Array genetic_code)
{
  return E_antiCodon(cp, genetic_code, NULL) ;
}



/* E_codon() - do the actual translation
 * 
 * s points at the 3 bytes of DNA code to translate, encoded as in dna.h
 *
 * genetic_code is an array containing the 64 codon map to use.  Get it
 * from pepGetTranslationTable()
 *
 * The return value is the single character protein identifier or X if
 * the codon cannot be translated (e.g. this happens when the DNA contains.
 * IUPAC ambiguity codes for instance).
 *
 * If index_out is non-NULL the array index of the codon translation is
 * returned, this enables the calling routine to deal with alternative
 * starts and stops, if the codon cannot be translated, -1 is returned.
 * (It would be easy enough to return a list of all the proteins it
 *  could be, if that were of interest. )
 *
 * This function always examines all 64 possibilities.  You could
 * optimize it by recognizing specific bit patterns and duplicating
 * the loop bodies, but it probably isn't worth bothering.
 */
static char E_codon(char *s, Array genetic_code, int *index_out)
{
  int x, y, z ;
  char it = 0 ;
  int index = -1 ;

  for (x=0 ; x < 4 ; x++)
    {
      if (s[0] & (1<<x))
	{
	  for (y=0 ; y < 4 ; y++)
	    {
	      if (s[1] & (1<<y))
		{
		  for (z=0 ; z < 4 ; z++)
		    {
		      if (s[2] & (1<<z))
			{
			  PepTranslate amino =
			    arrayp(genetic_code, ((x<<4)|(y<<2)|z), PepTranslateStruct) ;

			  if (!it)
			    {
			      it = amino->amino_acid ;
			      index = ((x<<4)|(y<<2)|z) ;
			    }

			  if (amino->amino_acid != it)
			    {
			      if (index_out)
				*index_out = -1 ;
			      return 'X' ;
			    }
			}		     
		    }
		}
	    }
	}
    }
  
  if (!it)						    /*  I think this never happens */
    {
      it = 'X' ;
      index = -1 ;
    }

  if (index_out)
    *index_out = index ;

  return it ;
}

static char E_reverseCodon (char* cp, Array genetic_code, int *index_out)
{
  char temp[3] ;

  temp[0] = complementBase[(int)cp[2]] ;
  temp[1] = complementBase[(int)cp[1]] ;
  temp[2] = complementBase[(int)cp[0]] ;

  return E_codon(temp, genetic_code, index_out) ;
}

static char E_antiCodon (char* cp, Array genetic_code, int *index_out)
{
  char temp[3] ;

  temp[0] = complementBase[(int)cp[0]] ;
  temp[1] = complementBase[(int)cp[-1]] ;
  temp[2] = complementBase[(int)cp[-2]] ;

  return E_codon (temp, genetic_code, index_out) ;
}



/* Returns a protein molecular weight of an encoded peptide array
 * If it cannot calculate the value it returns -1.0
 */
int pepWeight (Array pep)  /* decoded array */
{
  char *cp ;
  int i, residue, w = -1 ;

  int Ccnt = 0;  /* initialize counters */
  int Hcnt = 2;  /* always start with water */
  int Ocnt = 1;  /* H20 */
  int Ncnt = 0;
  int Scnt = 0;
  int Secnt = 0;
  
  
  if (pep && (i = arrayMax(pep)))
  {
    cp = arrp (pep, 0, char) - 1 ;
    while (cp++, i--)
      {
	residue = *cp ;

	if (H_atoms[residue] == 0)  /* unsupported AA */
	  return w ;                /* bail out */
	Ccnt += C_atoms[residue];
	Hcnt += H_atoms[residue];
	Ncnt += N_atoms[residue];
	Ocnt += O_atoms[residue];
	Scnt += S_atoms[residue];
	Secnt += Se_atoms[residue];
      }
    }
  
  w = (12.01115 * Ccnt) + (1.0079 * Hcnt) +
    (14.0067 * Ncnt) + (15.9994 * Ocnt) +
    (32.064 * Scnt) + (78.96 * Secnt);
  
  return w ;
}




/****************************************/

float pepPI (Array pep) /* decoded array */
{
  float x, w = 7.0 ;
  int i, n = 2 ;
  char *cp ;
  
  if (pep && (i = arrayMax(pep)))
    {
      cp = arrp(pep,0,char) ; ;
      w =  NH2pK[(int)*cp] + 
	COOHpK[(int)*(cp + i - 1)] ;
      cp-- ;
      while (cp++, i--)
	{ 
	  x = LateralpK[(int)*cp] ;
	  if (x > 1) {  w += x ;  n++ ;}
	} 
      w = w/arrayMax(pep) ;
    }

  return w ;
}


/****************************************/

void pepDecodeArray (Array pep)
{
  int i = arrayMax(pep) ;
  char *cp = arrp(pep,0,char) ;

  while (i--)
    {
      *cp = pepDecodeChar[(int)*cp] ;
      ++cp ;
    }

  return ;
}

void pepEncodeArray (Array pep)
{
  int i = arrayMax(pep) ;
  signed char *cp = arrp(pep,0, signed char) ;

  while (i--)
    {
      *cp = pepEncodeChar[*cp] ;
      ++cp ;
    }

  return ;
}

/*****************************/

static void peptideDoDump (ACEOUT dump_out, Array pep)
{
  register int i, k, k1, fin = arrayMax(pep) ;
  register char *cp, *cq ;
  char buffer [4100] ;

  i = fin ;
  cp = arrp(pep,0,char) ;
  cq = buffer ;

  while(i > 0)
    {
      cq = buffer ;
      memset(buffer,0,4100) ;
      for (k=0 ; k < 4000/50  ; k++)
        if (i > 0)
          { 
            k1 = 50 ;

            while (k1--  && i--)
              *cq++ = pepDecodeChar[*cp++ & 0xff] ;

            *cq++ = '\n' ; *cq = 0 ;
          }
      aceOutPrint (dump_out, "%s", buffer) ;
    }

  return;
} /* peptideDoDump */

/*****************************/

/* DumpFuncType for class _VPeptide */
BOOL peptideDump (ACEOUT dump_out, KEY k) 
{
  BOOL result = FALSE ;
  Array pep = 0 ;

  if ((pep = arrayGet(k, char, "c")))
    {
      peptideDoDump(dump_out, pep) ;
      
      arrayDestroy(pep);
      
      result = TRUE ;
    }

  return result ;
}

/************************************/

BOOL javaDumpPeptide (ACEOUT dump_out, KEY key) 
{
  BOOL result = FALSE ;
  Array pep ;
 
  if ((pep = arrayGet(key, char, "c")))
    {
      aceOutPrint (dump_out, "?Peptide?%s?\t?peptide?",
		   freejavaprotect(name(key)));
      
      pepDecodeArray(pep) ;
      array(pep,arrayMax(pep),char) = 0 ; /* ensure 0 terminated */
      --arrayMax(pep) ;
      aceOutPrint (dump_out, "%s", arrp(pep,0,char)) ;
      aceOutPrint (dump_out, "?\n\n");
      
      arrayDestroy(pep) ;
      result = TRUE ;
    }

  return result ;
} /* javaDumpPeptide */

/************************************/

/* ParseFuncType */
ParseFuncReturn peptideParse(ACEIN parse_io, KEY key, char **errtext)
{
  ParseFuncReturn result = PARSEFUNC_OK ;
  char *cp, c = 0 ;
  signed char c1 = 0 ;
  static Array pep = 0 ;


  messAssert(parse_io && key != KEY_UNDEFINED && errtext) ;

  if (class(key) != _VPeptide)
    messcrash ("peptideParse called on a non-peptide key") ;

  pep = arrayReCreate (pep, 1000, char) ;

  while (result == PARSEFUNC_OK && aceInCard(parse_io) && (cp = aceInWord(parse_io)))
    {
      do
	{
	  while (result == PARSEFUNC_OK && (c = *cp++))
	    { 
	      c1 = pepEncodeChar[((int)c) & 0x7f] ;
	      if (c1 >= 0)
		array(pep,arrayMax(pep),signed char) = c1 ;
	      else if (c1 == -1)
		{
		  *errtext = messprintf("  peptideParse error at line %d in %s : "
					"bad char %c (0x%x)", 
					aceInStreamLine(parse_io), name(key), c, c) ;
		  result = PARSEFUNC_ERR ;
		}
	    }
	}
      while (result == PARSEFUNC_OK && (cp = aceInWord(parse_io))) ;
    }

  if (result == PARSEFUNC_OK)
    {
      /* The CDS should include the stop codon ('*') so remove it from the translation. */
      if (c == '*')
	--arrayMax(pep) ;
      
      if (arrayMax(pep))		/* don't delete if nothing there */
	{
	  if (peptideStore (key, pep))
	    result = PARSEFUNC_OK ;
	  else
	    {
	      *errtext = messprintf(" failed to store peptide %s", name(key)) ;
	      result = PARSEFUNC_ERR ;
	    }
	}
      else
	result = PARSEFUNC_EMPTY ;
    }

  return result ;
}



/*******************************/

int hashArray(Array a)
{
#define  SIZEOFINT (8 * sizeof (int))
  int n = 32;
  unsigned int i, h = 0 , j = 0,
    rotate = 13 ,
    leftover = SIZEOFINT - rotate ;

  chrono("hashText") ;  
  for(i=0;i<arrayMax(a);i++)
    { h = freeupper(pepDecodeChar[(int)arr(a,i,signed char)]) ^  /* XOR*/
        ( ( h >> leftover ) | (h << rotate)) ; 
    }
  /* compress down to n bits */
  for(j = h, i = n ; i< SIZEOFINT; i+=n)
    j ^= (h>>i) ;
  chronoReturn() ;

  return j &  0xffffffff ;
}


/**************************************/

/* Checks a protein object to see if it has a peptide in the database, we do NOT verify the
 * peptide because this is expensive.
 */
BOOL pepObjHasOwnPeptide(KEY key)
{
  BOOL result = FALSE ;
  OBJ obj ;

  if ((class(key) == pickWord2Class("protein")) && (obj = bsCreate (key)))
    {
      if (bsGetKey (obj, _Peptide, &key) && class(key) == _VPeptide)
	{
	  result = TRUE ;
	}

      bsDestroy (obj) ;
    }

  return result ;
}


/* This is the old peptideGet() which only translates the object if it is a
 * CDS. Its just a cover function for the newer peptideTranslate() which
 * will translate either just the CDS or the whole object.
 * See the routine below for more verbose comments.....
 */
Array peptideGet(KEY key)
{
  Array result = NULL ;


  result = peptideTranslate(key, TRUE) ; 


  return result ;
}

/*******************************/

/* This function returns an encoded peptide array unlike peptideDNATranslate() which
 * returns the actual character array. This implies to me that perhaps there is some
 * work to be done in finding out whether we need to return encoded peptides at all.
 * The main place this is used is in pepdisp.c, fmapfeatures only uses the call to 
 * see if it can translate the peptide, not for any other reason...seems rather
 * wasteful..... */
Array peptideTranslate(KEY key, BOOL CDS_only)
{ 
  Array pep = NULL ;
  KEY genetic_code = KEY_UNDEFINED ;
  OBJ obj = NULL ;
  int dna_min = 0, dna_max = 0 ;
  Array dna = NULL ;
  Array code_table ;

  if (class(key) == _VPeptide)
    {
      pep = arrayGet(key, char, "c") ;
      goto done ;
    }

  if (!(obj = bsCreate (key)))
    return NULL ;

  if (bsGetKey (obj, _Peptide, &key) && class(key) == _VPeptide)
    {
      bsDestroy (obj) ;
      obj = NULL ;
      pep = arrayGet(key, char, "c") ;
      if (!pep)
	return NULL ;
      else
	goto done ;
    }

  if (bsGetKey (obj, _Corresponding_DNA, &key))	/* try that */
    {
      bsDestroy (obj) ;
      if (!(obj = bsCreate (key)))
	return NULL ;
    }

  if (!(code_table = pepGetTranslationTable(key, &genetic_code)))
    {
      bsDestroy (obj) ;
      return NULL ;
    }

  /* Get min/max of object. */
  dna_min = 1 ;
  dna_max = sMapLength(key) ;

  /* Caller may have asked for just the CDS to be translated so we need to adjust coords
   * to get just DNA for the CDS, we also need to check Start_not_found which may
   * optionally set the frame for the start of translation so we need to adjust the
   * start of the dna for that as well. */
  if (CDS_only)
    {
      int cds_min = 0, cds_max = 0 ;

      /* If you only want the CDS translated then there'd better be one !    */
      if (!bsFindTag (obj, _CDS))
	{
	  bsDestroy (obj) ;

	  return NULL ;
	}

      /* Retrieve the start/stop of the cds section of dna.                   */
      cds_min = dna_min ;
      cds_max = dna_max ;
      if (bsGetData(obj, _bsRight, _Int, &cds_min))
	{
	  bsGetData(obj, _bsRight, _Int, &cds_max) ;

	  /* We need to check the coords for the cds start/stop, these can be     */
	  /* changed by hand and its easy to forget and specify them in          */
	  /* Source_exons coordinates instead of spliced DNA coordinates.        */
	  /* We warn the user if they have got it wrong.                         */
	  if (cds_min < dna_min || cds_max > dna_max)
	    {
	      messerror("The start/stop positions set for the CDS in object %s"
			" are outside the spliced DNA coordinates for that object."
			" Make sure the CDS positions have been specified"
			" in spliced DNA, not Source_exon, coordinates."
			" (spliced DNA coords are %d to %d, current CDS coords are %d to %d)",
			name(key), dna_min, dna_max, cds_min, cds_max) ;

	      bsDestroy (obj) ;

	      return NULL ;
	    }
	}

      dna_min = cds_min ;
      dna_max = cds_max ;
    }


  /* May have to modify beginning of dna for translation because coding may  */
  /* have begun in a previous exon so we don't have the beginning, the       */
  /* Start_not_found allows us to correct the reading frame by setting a new */
  /* start position.                                                         */
  /* NOTE that if Start_not_found and CDS are set then the CDS MUST start at */
  /* position 1 spliced exon coordinates.                                    */
  if (bsFindTag(obj, _Start_not_found))
    {
      int no_start = 1 ;				    /* default to 1 if no values specified */
      

      /* Need to put CDS tag stuff in here and just use it to look for inconsistencies... */
      if (CDS_only && dna_min != 1)
	{
	  messerror("Data inconsistency: the Start_not_found tag is set for object %s,"
		    " but the CDS begins at position %d instead of at 1.",
		    name(key), dna_min) ;
	  
	  bsDestroy(obj) ;

	  return NULL ;
	}

      if (bsGetData(obj, _bsRight, _Int, &no_start))
	{
	  if (no_start < 1 || no_start > 3)
	    {
	      messerror("Data inconsistency: the Start_not_found tag for the object %s"
			" should be given the value 1, 2 or 3 but has been"
			" set to %d.",
			name(key), no_start) ;
	      
	      bsDestroy (obj) ;

	      return NULL ;
	    }
	}
      
      dna_min += (no_start - 1) ;
    }
  

  if (!(dna = dnaFromObject(key, dna_min, dna_max)))
    {
      bsDestroy (obj) ;
      return NULL ;
    }


  pep = doDNATranslation(code_table, dna, TRUE, FALSE) ;

 done:
  /* This is a hack to make the peptide into a valid C string.... */
  array(pep, arrayMax(pep), signed char) = 0 ;
  --arrayMax(pep) ;

  if (dna)
    arrayDestroy(dna) ;

  if (obj)
    bsDestroy(obj) ;

  return pep ;
}


/*******************************/

BOOL peptideStore (KEY key, Array pep)
{ 
  OBJ obj ;
  KEY seq ;
  int len, len1 ;
  int checksum, ck1;

  if (!pepInitialise())
    return FALSE ;

  if (class(key) == _VProtein)
    lexaddkey (name(key), &key, _VPeptide) ;
  else if (class(key) != _VPeptide)
    messcrash ("peptideStore called on a non-peptide key") ;

  lexaddkey (name(key), &seq, _VProtein) ;

  if (!pep || !(len = arrayMax(pep)))
    { 
      arrayKill (key) ;
      if ((obj = bsCreate (seq)))
	{
	  if (bsFindKey (obj, _Peptide, key))
	    { 
	      bsDestroy (obj) ;
	      if ((obj = bsUpdate(seq)))
		{
		  if (obj && bsFindKey (obj, _Peptide, key))
		    bsRemove (obj) ;
		  bsSave (obj) ;
		}
	    }
	  else
	    bsDestroy (obj) ;
	}
    }
  else
    { 
      arrayStore (key, pep, "c") ;
      checksum = hashArray(pep);
      if ((obj = bsCreate (seq)))
	{  /* check first for sake of client server speed */
	  if (bsFindKey (obj, _Peptide, key)  &&
	      bsGetData (obj, _bsRight, _Int, &len1) &&
	      len1 == len &&
	      ( bsType(obj, _bsRight) != _Int ||
		( bsGetData (obj,_bsRight, _Int, &ck1) && ck1 == checksum)
		))
	    {
	      bsDestroy (obj) ;
	      return TRUE ;
	    } 
	  bsDestroy (obj) ;
	}

      if ((obj = bsUpdate (seq)))
	{
	  bsAddKey (obj, _Peptide, key) ;
	  bsAddData (obj, _bsRight, _Int, &len) ;
	  if(bsType(obj, _bsRight) == _Int)
	    bsAddData (obj,_bsRight, _Int, &checksum);
	  bsSave(obj) ;
	}
    }

  return TRUE ;
}




/*
 * Some functions to translate a given dna sequence and output the translation
 * in FASTA format.
 *
 */


/* I've grabbed this from fmapsequence.c where it was an external function as its
 * also called in fmapfeatures.c...agggghhhh,
 * really we should have a single function for doing FASTA header lines. */
static void writeTranslationTitle(ACEOUT fo, KEY seq)
{
  OBJ obj ;
  KEY key ;

  aceOutPrint (fo, ">%s", name(seq)) ;

  if ((obj = bsCreate(seq)))
    {
      if (bsGetKey (obj, _Locus, &key))
	aceOutPrint (fo, " %s:", name(key)) ;
      if (bsGetKey (obj, _Brief_identification, &key))
	aceOutPrint (fo, " %s", name(key)) ;
      if (bsFindTag (obj, _Start_not_found))
	aceOutPrint (fo, " (start missing)") ;
      if (bsFindTag (obj, _End_not_found))
	aceOutPrint (fo, " (end missing)") ;
      bsDestroy (obj) ;
    }
  else
    messerror ("Cannot open object for sequence \"%s\" to set full peptide dump title",
	       name(seq)) ;

  aceOutPrint (fo, "\n") ;

  return;
}



/* Given an object and some dna for that object, translate the dna and output
 * it to a file chosen by the user. */
BOOL pepTranslateAndExport(KEY obj_key, Array obj_dna)
{
  BOOL result = FALSE ;
  static char fname[FIL_BUFFER_SIZE]="", dname[DIR_BUFFER_SIZE]="" ;
  ACEOUT pep_out ;

  /* Translate and write out to file. */
  if ((pep_out = aceOutCreateToChooser("File to add translation to",
				       dname, fname, "pep", "a", 0)))
    {
      result = pepTranslateAndOutput(pep_out, obj_key, obj_dna) ;

      aceOutDestroy (pep_out);
    }

  return result ;
}



/* Given an object and some dna for that object, translate the dna and output
 * it to the supplied aceout stream. */
BOOL pepTranslateAndOutput(ACEOUT pep_out, KEY obj_key, Array obj_dna)
{
  BOOL result = TRUE ;
  KEY genetic_code = KEY_UNDEFINED ;
  Array code_table ;
  int i ;
  Array pep ;

  if (!(code_table = pepGetTranslationTable(obj_key, &genetic_code)))
    {
      messerror("Could not get a translation table for \"%s\"", name(obj_key)) ;
      return FALSE ;
    }

  if (!(pep = doDNATranslation(code_table, obj_dna, FALSE, FALSE)))
    {
      messerror("Could not translate DNA for \"%s\"", name(obj_key)) ;
      return FALSE ;
    }

  writeTranslationTitle(pep_out, obj_key) ;

  /* Start at 1 to get the % 50 right. */
  for (i = 1 ; i <= arrayMax(pep) ; i++)
    {
      aceOutPrint(pep_out, "%c", array(pep, (i - 1), char)) ;
      if (i % 50 == 0)
	aceOutPrint (pep_out, "\n");
    }
  if (i % 50)
    aceOutPrint (pep_out, "\n");



  return result ;
}






/*
 * Some functions to output translations in FASTA format.
 *
 */


ACEOUT pepFileOpen (STORE_HANDLE handle)
{
  static char directory[DIR_BUFFER_SIZE]= "";
  static char filename[FIL_BUFFER_SIZE] = "";
  ACEOUT pep_out = NULL;

  pep_out = 
    aceOutCreateToChooser ("Choose a file for export in fasta format",
			   directory, filename, "pep", "w", handle);

  return pep_out;
} /* pepFileOpen */

/**********************************************************/

/* called also from dnacptfastaDump */
static void pepDoDump (ACEOUT dump_out, Array a, int from, int to, char style)
{
  char buf [55] ;
  register int i, j, k ;
  
   for (i = from ; i < to ;)
    { 
      k = 0 ;

      for (j = 50 ; i < to && j-- ;)
	{
	  buf [k++] = pepDecodeChar[((int)arr(a, i++, char)) & 0xff] ;
	}

      if (style != 'u')
	buf [k++] = '\n' ;

      buf[k++] = 0 ;
      aceOutPrint (dump_out, "%s", buf) ;
    }

   return;
} /* pepDoDump */

/**********************************************************/

BOOL pepDumpFastA (Array a, int from, int to, char *text, ACEOUT dump_out, char style)
{ 
  if (!a)
    return FALSE ;

  if (from < 0)
    from = 0 ;

  ++to ;
  if (to > arrayMax(a))
    to = arrayMax(a) ;

  if (style != 'u')
    aceOutPrint (dump_out, ">%s\n", text) ;
  pepDoDump(dump_out, a, from, to, style) ;

  return TRUE ;
} /* pepDumpFastA */

/**********************************************************/

BOOL pepDumpFastAKey (ACEOUT dump_out, KEY key, char style)
{ 
  BOOL result = FALSE ;

  result = pepDoDumpFastAKey(dump_out, key, FALSE, style) ;

  return result ;
}

static BOOL pepDoDumpFastAKey(ACEOUT dump_out, KEY key, BOOL cds_only, char style)
{ 
  BOOL result = FALSE ;
  Array a = NULL ;

  a = peptideTranslate(key, cds_only) ;
  if (a)
    {
      result = pepDumpFastA(a, 0, arrayMax(a)-1, name(key), dump_out, style);
      arrayDestroy(a) ;
    }

  return result ;
} /* pepDumpFastAKey */



/* Dump the peptides for a keyset, either the full peptides for the RNA's
 * or just the CDS portions. */

int pepDumpFastAKeySet(ACEOUT dump_out, KEYSET kSet)
{
  int result = 0 ;

  result = pepDoDumpFastAKeySet(dump_out, kSet, FALSE) ;

  return result ;
}

int pepDumpCDSFastAKeySet(ACEOUT dump_out, KEYSET kSet)
{
  int result = 0 ;

  result = pepDoDumpFastAKeySet(dump_out, kSet, TRUE) ;

  return result ;
}

static int pepDoDumpFastAKeySet(ACEOUT dump_out, KEYSET kSet, BOOL cds_only)
{
  KEYSET alpha ;
  int i, n = 0 ;

  if (!keySetExists(kSet) || keySetMax(kSet) == 0)
    return 0 ;

  alpha = keySetAlphaHeap (kSet, keySetMax(kSet)) ;

  for (i = 0 ; i < keySetMax(alpha) ; ++i)
    {
      if (pepDoDumpFastAKey(dump_out, keySet(alpha, i), cds_only, '\0'))
	++n ;
    }

  keySetDestroy (alpha) ;

  messout ("I wrote %d sequences", n) ;

  return n ;
} /* pepDumpFastAKeySet */




/**********************************************************/
/*
* helper function for pepGetTranslationTable()
*/
static int t_codon_code (char *s)
{
  int result ;
  unsigned char a, b, c;
    
  a = dnaStrictEncodeChar [(int)s[0]] ;
  b = dnaStrictEncodeChar [(int)s[1]] ;
  c = dnaStrictEncodeChar [(int)s[2]] ;

  if (!a-- || !b-- || !c--)
    result = -1 ;
  else
    result = (a<<4) | (b<<2) | c ;

  return result ;
}

/********************************/

/* Look to see if the object or any of its parents specify a genetic code to be used
 * for translation. There may sometimes be different genetic codes for individual objects
 * to cope with rare cases like Selenocysteine transcripts. */
static KEY pepGetGeneticCode (KEY key) 
{
  KEY gKey = KEY_UNDEFINED ;

  if (class(key) == _VGenetic_code)
    gKey = key ;
  else
    {
      while (gKey == KEY_UNDEFINED)
	{
	  if (!bIndexGetKey(key, _Genetic_code, &gKey))
	    {
	      if (!bIndexGetKey(key, _Source, &key))	    /* standard system */
		{
		  if (!bIndexGetTag2Key(key, _S_Parent, &key)) /* smap parent */
		    break ;
		}
	    }
	}
    }

  return gKey ;
}

/********************************/


/* Returns the translation table for an object or NULL if there was some
 * kind of error, if there was an error then geneticCodep is not changed.
 *
 * The genetic code will either be the standard genetic code, or, if an alternative
 * code was specified via a Genetic_code object in the object or any of the objects
 * parents in the sequence or SMap'd hierachy, then that is returned.
 *
 * If a valid genetic code was found, then if geneticCodep is non-null and
 * if a Genetic_code object was found, its key is returned, otherwise KEY_UNDEFINED
 * is returned (i.e. the standard genetic code was used).
 *
 * Genetic_code model:
 *
 * ?Genetic_code   Other_name ?Text
 *                 Remark Text
 *                 Translation UNIQUE Text
 *                 Start UNIQUE Text
 *                 Stop   UNIQUE Text
 *                 Base1 UNIQUE Text
 *                 Base2 UNIQUE Text
 *                 Base3 UNIQUE Text
 */
Array pepGetTranslationTable(KEY key, KEY *geneticCodep)
{
  Array translationTable = NULL ;
  KEY gKey = KEY_UNDEFINED ;
  int index = 0 ;
  static Array maps = NULL ;

  /* First time through create cached array of maps. */
  if (!maps)
    {
      Array standard_code ;
      int i ;

      maps = arrayCreate(120, Array) ;

      /* zero'th entry is standard genetic code, note this all works because we use
       * KEYKEY of Genetic_code obj to index into array and no keys will have value zero
       * so we are safe to put standard code in zero'th element. */
      standard_code = arrayCreate(PEP_TOTAL_CODONS, PepTranslateStruct) ;

      for (i = 0 ; i < PEP_TOTAL_CODONS ; i++)
	{
	  PepTranslate amino = arrayp(standard_code, i, PepTranslateStruct) ;
	  
	  amino->amino_acid = STANDARD_GENETIC_CODE[i] ;
	  amino->alternative_start = amino->alternative_stop = FALSE ;
	}

      array(maps, 0, Array) = standard_code ;
    }


  /* Find the correct genetic_code from the set of cached codes or get it from the database,
   * remember that if gKey is KEY_UNDEFINED (i.e. zero) then we automatically use the standard
   * code. */
  gKey = pepGetGeneticCode(key) ;


  /* Lets hope we don't encounter a long running database where loads of genetic code objects
   * have been created/deleted otherwise our array may be v. big.....one to watch... */
  index = KEYKEY(gKey) ;
  
  if (!(translationTable = array(maps, index, Array)))
    {
      OBJ obj ;
      
      if ((obj = bsCreate(gKey)))
	{
	  char *translation, *start, *stop, *base1, *base2, *base3 ;
	  char result[4*4*4+1] ;
	  int x,n ;
	  
	  /* These fields encode the translation table, they must all be correct. */
	  if (bsGetData(obj, str2tag("Translation"), _Text, &translation)
	      && bsGetData(obj, str2tag("Start"), _Text, &start)
	      && bsGetData(obj, str2tag("Stop"), _Text, &stop)
	      && bsGetData(obj, str2tag("Base1"), _Text, &base1)
	      && bsGetData(obj, str2tag("Base2"), _Text, &base2)
	      && bsGetData(obj, str2tag("Base3"), _Text, &base3)
	      && strlen(translation) == 64
	      && strlen(start) == 64
	      && strlen(stop) == 64
	      && strlen(base1) == 64
	      && strlen(base2) == 64
	      && strlen(base3) == 64)
	    {
	      Array genetic_code ;

	      memset(result,0,4*4*4) ;
	      result[4*4*4] = 0 ;
	      
	      genetic_code = arrayCreate(PEP_TOTAL_CODONS, PepTranslateStruct) ;
	      
	      /* walk through the combinations and do the conversion.  I don't
	       * assume that the bases are in a particular order, even though it
	       * looks like it from the database. */
	      for (x = 0 ; x < 64 ; x++)
		{
		  char xl[3] ;
		  
		  xl[0] = base1[x] ;
		  xl[1] = base2[x] ;
		  xl[2] = base3[x] ;
		  
		  n = t_codon_code(xl) ;
		  if (n >= 0)
		    {
		      PepTranslate amino = arrayp(genetic_code, n, PepTranslateStruct) ;
		      result[n] = translation[x] ;
		      
		      amino->amino_acid = translation[x] ;
		      if (start[x] == 'M')
			amino->alternative_start = TRUE ;
		      else
			amino->alternative_start = FALSE ;

		      if (stop[x] == '*')
			amino->alternative_stop = TRUE ;
		      else
			amino->alternative_stop = FALSE ;
		    }
		  else
		    break ;
		}
		      
	      /* Must have set all 64 translations for valid genetic code. */
	      if (x != 64)
		arrayDestroy(genetic_code) ;
	      else
		translationTable = genetic_code ;
	    }
	}

      bsDestroy(obj) ;
    }

  if (translationTable)
    {
      if (geneticCodep)
	*geneticCodep = gKey ;

      if (pep_debug_G)
	printTranslationTable(gKey, translationTable) ;
    }
  else
    {
      messerror("%s does not specify genetic code correctly.", nameWithClassDecorate(gKey));
    }
  
  return translationTable ;
}



/************************************************************/


static BOOL pepDumpCStyle (ACEOUT dump_out, KEY key, Array pep, int from, int to)
{
  int ii = to - from + 1 ;
  char buf[2] ;
  char *cp = name(key) ;

  if (!pep || from < 0 || from >= to || to >= arrayMax(pep))
    return FALSE ;

  pepDecodeArray (pep) ;
  buf[0] = 'N' ; buf[1] = class(key) ;
  aceOutBinary (dump_out, buf, 2) ;
  aceOutBinary (dump_out, cp, strlen(cp) + 1) ;

  buf[0] = '\n' ;
  buf[1] = 'c' ;
  aceOutBinary (dump_out, buf,2) ;

  aceOutBinary (dump_out, (char*) &ii, 4) ;

  buf[0] = '\n' ;
  buf[1] = 'p' ;
  aceOutBinary (dump_out, buf,2) ;

  aceOutBinary (dump_out, arrp (pep, from, char), ii) ;

  buf[0] = '\n' ;
  buf[1] = '#' ;
  aceOutBinary (dump_out, buf,2) ;

  return TRUE ;
}

BOOL peptideDumpKeyCstyle (ACEOUT dump_out, KEY key)
{
  Array a = peptideGet (key) ;
  BOOL result = FALSE ;

  if (a)
    {
      result = pepDumpCStyle (dump_out, key, a, 0, arrayMax(a) - 1) ;
      arrayDestroy (a) ;
    }

  return result ;
}




/*
 * Some routines for the case where you already have the dna from the object and you
 * want to translate it. The obj_key is only used for checking for an alternative
 * genetic code to do the translation with.
 */

/* Just a cover function for peptideDNATrans() which does the real work,
 * see peptideDNATrans() header for more information. */
Array peptideDNATranslate(KEY obj_key, Array obj_dna)
{
  Array pep ;

  pep = peptideDNATrans(obj_key, obj_dna, FALSE) ;

  return pep ;
}


/* Given an object key and its dna, returns an array of chars
 * which is the peptide translation of the dna. If include_stop is TRUE
 * then the terminating "*" (if there is one) is included in the array.
 * Returns NULL if the genetic code for translating this object cannot be retrieved
 * correctly from the database or there is some problem in translating
 * the dna.
 * NOTE that this function returns a decoded peptide array (i.e. the actual chars
 * representing the peptide) unlike peptideGet/Translate() which return an encoded peptide array. */
Array peptideDNATrans(KEY obj_key, Array obj_dna, BOOL include_stop)
{
  Array pep = 0 ;
  KEY genetic_code = KEY_UNDEFINED ;
  Array code_table ;

  if (!(code_table = pepGetTranslationTable(obj_key, &genetic_code)))
    {
      messerror("Could not get a translation table for \"%s\"", name(obj_key)) ;
    }
  else if (!(pep = doDNATranslation(code_table, obj_dna, FALSE, include_stop)))
    {
      messerror("Could not translate DNA for \"%s\"", name(obj_key)) ;
    }

  return pep ;
}




/* This function encapsulates the translation of DNA to peptide and returns either an encoded
 * or decoded peptide array with or without the stop codon (if there is one).
 * The rules for translation are a bit more complex now that alternative start/stop's can be
 * specified. */
static Array doDNATranslation(Array code_table, Array obj_dna, BOOL encode, BOOL include_stop)
{
  Array pep ;
  int dna_min, dna_max ;
  int bases, pepmax, x, code_table_index ;
  char cc ;

  messAssert(code_table && obj_dna) ;

  dna_min = 1 ;
  dna_max = arrayMax(obj_dna) ;

  /* Incomplete codons at the end of the sequence are represented by an X so we make sure.
   * the peptide array is long enough for this. */
  bases = dna_max - dna_min + 1 ;
  x = pepmax = bases / 3 ;
  if (bases % 3)
    x++ ;

  pep = arrayCreate((x + 1), char) ;

  dna_min-- ;
  x = cc = 0 ;

  /* Deal with alternative start codons. */
  cc = E_codon(arrp(obj_dna, dna_min, char), code_table, &code_table_index) ;
  if (cc != 'X' && cc != 'M')
    {
      PepTranslate amino = arrayp(code_table, code_table_index, PepTranslateStruct) ;
      if (amino->alternative_start)
	cc = 'M' ;
    }
  array(pep, x, signed char) = encode ? pepEncodeChar[(int)cc] : cc ;
  x++ ;
  dna_min += 3 ;

  /* Deal with the rest. */
  while (x < pepmax)
    {
      cc = E_codon(arrp(obj_dna, dna_min, char), code_table, &code_table_index) ;
      array(pep, x, signed char) = encode ? pepEncodeChar[(int)cc] : cc ;
      x++ ;
      dna_min += 3 ;
    }
  x-- ;

  /* Deal with incomplete or alternative end codons. */
  if (bases % 3)
    array(pep, (x + 1), signed char) = encode ? pepEncodeChar[(int)'X'] : 'X' ;
  else
    {
      if (cc != 'X' && cc != '*')
	{
	  PepTranslate amino = arrayp(code_table, code_table_index, PepTranslateStruct) ;
	  if (amino->alternative_stop)
	    {
	      array(pep, x, signed char) = encode ? pepEncodeChar[(int)'*'] : '*' ;
	      cc = '*' ;
	    }
	}
     
      if (!include_stop && cc == '*')
	--arrayMax(pep) ;
    }


  return pep ;
}




/* For debugging, outputs the translation table the code has constructed from the
 * Genetic_code object. */
static void printTranslationTable(KEY genetic_code_key, Array translationTable)
{
  int i ;

  printf("Translation table derived from %s\n", nameWithClassDecorate(genetic_code_key)) ;
  printf(" peptides = ") ;
  for (i = 0 ; i < 64 ; i++)
    {
      printf("%c", arrayp(translationTable, i, PepTranslateStruct)->amino_acid) ;
    }
  printf("\n") ;

  printf("   starts = ") ;
  for (i = 0 ; i < 64 ; i++)
    {
      printf("%c",
	     (arrayp(translationTable, i, PepTranslateStruct)->alternative_start ? 'M' : '-')) ;
    }
  printf("\n") ;

  printf("     stops = ") ;
  for (i = 0 ; i < 64 ; i++)
    {
      printf("%c",
	     (arrayp(translationTable, i, PepTranslateStruct)->alternative_stop ? '*' : '-')) ;
    }
  printf("\n") ;

  return ;
}


/************************** eof *****************************/

