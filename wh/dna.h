/*  File: dna.h
 *  Author: Ed Griffiths (edgrif@sanger.ac.uk)
 *  Copyright (C) J Thierry-Mieg and R Durbin, 1998
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
 * Last edited: Mar  2 10:20 2009 (edgrif)
 * * May 24 10:26 1999 (edgrif): Add some missing extern func decs.
 * * Jul 23 14:41 1998 (edgrif): Remove redeclarations of fmap functions.
 * * Jul 23 09:06 1998 (edgrif): I have added this header, which was
 *      missing (that's why it shows me as author). I have removed the
 *      declaration of fMapFindDNA which is now in fmaps public header.
 * Created: Thu Jul 23 09:06:15 1998 (edgrif)
 * CVS info:   $Id: dna.h,v 1.40 2009-03-02 13:20:51 edgrif Exp $
 *-------------------------------------------------------------------
 */
#ifndef DEF_DNA_H
#define DEF_DNA_H

#include <wh/regular.h>
#include <wh/aceiotypes.h>
#include <wh/keyset.h>

/*

We use the standard UPAC coding for representing DNA with a single character
per base:

Exactly known bases

A
T  (or U for RNA)
G
C

Double ambiguity

R	AG	~Y		puRine
Y	CT	~R		pYrimidine
M	AC	~K		aMino
K	GT	~M		Keto
S	CG	~S		Strong
W	AT	~w		Weak

Triple ambiguity

H	AGT	~D		not G	
B	CGT	~V		not A
V	ACG	~B		not T
D	AGT	~H		not C

Total ambiguity

N	ACGT	~N		unkNown

Possible existence of a base

-       0       padding

Note that:
We do NOT use U internally, but just use it for display if tag RNA is set

NCBI sometimes uses X in place of N, but we only use X for peptides

The - padding character is used when no sequencing is available and
therefore means that the exact length is unknown, 300 - might not mean
exactly 300 unkown bases.

In memory, we use one byte per base, the lower 4 bits correspond to
the 4 bases A_, T_, G_, C_. More tha one bit is set in the ambiguous cases.
The - padding is represented as zero. In some parts of the code, the upper 
4 bits are used as flags.

On disk, we store the dna as 4 bases per byte, if there is no ambiguity, or
2 bases per byte otherwise.
*/


#define A_ 1
#define T_ 2
#define U_ 2
#define G_ 4
#define C_ 8

#define R_ (A_ | G_)
#define Y_ (T_ | C_)
#define M_ (A_ | C_)
#define K_ (G_ | T_)
#define S_ (C_ | G_)
#define W_ (A_ | T_)

#define H_ (A_ | C_ | T_)
#define B_ (G_ | C_ | T_)
#define V_ (G_ | A_ | C_)
#define D_ (G_ | A_ | T_)

#define N_ (A_ | T_ | G_ | C_)

extern char dnaDecodeChar[] ;	/* this is the mapping used to decode a single base */
extern char dnaEncodeChar[] ;	/* this is the mapping used to encode a single base */
extern char rnaDecodeChar[] ;
extern char complementBase[] ;	/* complement of single base */
extern char *aaName[] ;		/* maps single letter code to full name */


typedef enum {DNA_INVALID, DNA_FAIL, DNA_QUERY, DNA_CONTINUE} DNAActionOnErrorType ;


char * dnaDecodeString(char *buf, char *cp, int n) ; /* Decodes n char from cp, result stored in buffer */
void dnaEncodeString(char *cp) ; /* reversed encoding, works en place */
void dnaDecodeArray (Array a) ;	/* works in place */
void rnaDecodeArray (Array a) ;	/* works in place */
void dnaEncodeArray (Array a) ;	/* works in place */

BOOL  dnaObjHasOwnDNA(KEY key) ;
Array dnaGet(KEY key) ;
Array dnaGetAction(KEY key, DNAActionOnErrorType error_action) ;
Array dnaHandleGet(KEY key, STORE_HANDLE handle) ;
Array dnaRawGet(KEY key) ;
Array dnaFromObject(KEY key, int dna_start, int dna_end) ;


#ifndef NEW_SMAP_DNA
/* Use the second of these if you want to control whether mismatches are     */
/* reported or not. (dnaAdd resolves to a call to dnaDoAdd anyway).          */
/* Both return:  -1: error, 0: mismatch, 1: absent, 2: success               */
int dnaAdd (Array a, KEY seq, int start, int stop, BOOL noMismatch) ;
int dnaDoAdd(Array a, KEY seq, int start, int stop, BOOL noMismatch, BOOL reportErrors) ;

void dnaStoreDestroy (KEY key, Array dna) ; 
			/* beware - sets length in sequence object */
void dnaExonsSort (Array a) ;	/* a is of BSunit from flattening Source_exons */
#endif

char codon (char *cp) ;		/* next 3 bases -> amino acid */
char reverseCodon (char *cp) ;	/* next 3 bases on opp strand -> amino acid */
char antiCodon (char* cp);

Array dnaCopy (Array dna) ; /* arrray copy, but adding a terminal zero, avoids subsequnet doubling  */
void  reverseComplement(Array dna) ; /* acts in place */

				/* Handles for dnacpt package */
void dnaAnalyse (void) ;
int dnacptPickMatch (char *cp, int n, char *tp, int maxError, int maxN) ;
void dnaCptFindRepeats(void) ;


/* Dump the sequence, either broken into 50 base lines or as one long string terminated with a
 * line break. If style == 'u' then just the sequences is dumped, if style == 'o' then each
 * line has three white space separated words:     <seqname> <seqlength> <sequence>
 */
BOOL dnaRawDump(char style, KEY key, Array dna, int from, int to, ACEOUT dump_out , BOOL formatted) ;

/* FastA dumping routines */
BOOL dnaRawDumpFastA(char *seqname, char *dna, ACEOUT dump_out) ;
BOOL dnaDumpFastA (Array a, int from, int to, char *text, ACEOUT dump_out);
BOOL dnaDumpFastAKey (ACEOUT dump_out, KEY key);
BOOL dnaDumpFastAKeyWithErrors (ACEOUT dump_out, KEY key);
BOOL dnaZoneDumpFastAKey(ACEOUT dump_out, KEY key, DNAActionOnErrorType error_action,
			 BOOL cds_only, BOOL spliced, int x1, int x2, char style) ;
int dnaDumpFastAKeySet(ACEOUT dump_out, KEYSET kSet);
int dnaDumpCDSFastAKeySet(ACEOUT dump_out, KEYSET kSet) ;
void dnacptFastaDump(void);


void dnacptDontUseKeySet (void) ;

ACEOUT dnaFileOpen (STORE_HANDLE handle); /* ACEOUT allocated upon given handle */

/* ace dump routines */
BOOL dnaDump (ACEOUT dump_out, KEY key); /* DumpFuncType */
ParseFuncReturn dnaParse (ACEIN parse_in, KEY key, char **errtext); /* ParseFuncType */

BOOL baseQualityDump (ACEOUT dump_out, KEY key) ; /* DumpFuncType */
ParseFuncReturn baseQualityParse (ACEIN parse_in, KEY key, char **errtext) ; /* ParseFuncType */

BOOL basePositionDump (ACEOUT dump_out, KEY key); /* DumpFuncType */
ParseFuncReturn basePositionParse (ACEIN parse_in, KEY key, char **errtext) ; /* ParseFuncType */


/* the text values are filled in dnasubs.c

G	Guanine
A	Adenine 
T	Thymine
C	Cytosine
R	Purine			A,G
Y	Pyramidine		T,C
M	Amino			A,C
K	Ketone			G,T
S	Strong interaction	C,G
W	Weak interaction	A,T
H	not-G			A,C,T
B	not-A			G,C,T
V	not-T			A,C,G
D	not-C			A,G,T
N	any                     G,A,T,C

*/

/*                  NO CODE AFTER THIS                                       */
#endif /* DEF_DNA_H */
