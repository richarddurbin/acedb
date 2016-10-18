/*  File: peptide.h
 *  Author: Richard Durbin (rd@sanger.cam.ac.uk)
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
 * Last edited: Dec 21 09:08 2009 (edgrif)
 * Created: Wed May 11 01:49:18 1994 (rd)
 * CVS info:   $Id: peptide.h,v 1.27 2009-12-21 10:49:37 edgrif Exp $
 *-------------------------------------------------------------------
 */
#ifndef ACEDB_PEPTIDE_H
#define ACEDB_PEPTIDE_H


extern char pepDecodeChar[] ;	/* [0,21] -> A,C,D... */
extern signed char pepEncodeChar[] ;	/* A,C,D... -> [0,21] : -ve bad */
extern char *pepName[] ; /* A,C,D... -> Alanine, Cysteine... */
extern char *pepShortName[] ;	/* A,C,D... -> Ala, Cys... */
extern int  molecularWeight[] ;  /* A,C,D... -> 89, 121... */


/* k is either a genetic_code object to use for translation. 
 * or a sequence object, in which case we recurse through the
 * parents looking for the genetic_code to be used.
 * 
 * returns an array for use with e_codon().  do not free array.
 * returns key of Genetic_code in *geneticCodep if non-NULL. */
Array pepGetTranslationTable (KEY k, KEY *geneticCodep) ;


/* extended_codon, needed for non standard genetic codes (e.g. mithocondria)
 * s is dna to translate
 * translationTable is previously obtained from pepGetTranslationTable()
 * returns the (coded) amino acid */
char e_codon (char *s, Array translationTable) ; /* next 3 bases -> amino acid */
char e_reverseCodon (char *s, Array translationTable) ;
							    /* next 3 bases on opp strand -> amino acid */
char e_antiCodon (char *s, Array translationTable) ;



float pepPI (Array pep) ;   /* decoded array */
int pepWeight (Array pep) ; /* decoded array */

/* Does trivial check to see if obj has a peptide array. */
BOOL pepObjHasOwnPeptide(KEY protein) ;

/* extract peptide from/store peptide in object in database, this calls use encoded peptide.
 * arrays. */
Array peptideGet(KEY key) ;				    /* peptideTranslate() with CDS_only = TRUE */
Array peptideTranslate(KEY key, BOOL CDS_only) ;
BOOL peptideStore(KEY key, Array pep) ;			    /* fails if model is missing */


/* Translate dna that you have already retrieved from an object, the obj_key is only used to
 * check whether a non-standard genetic code was specified.
 * These routines return a decoded peptide array. */
Array peptideDNATranslate(KEY obj_key, Array obj_dna) ;
Array peptideDNATrans(KEY obj_key, Array obj_dna, BOOL include_stop) ;


/* Translate in-place   encoded <-> decoded   peptide arrays. */
void pepDecodeArray (Array pep) ;
void pepEncodeArray (Array pep) ;


BOOL pepSubClass (KEY protein, KEY *pepKeyp) ; /* gets the parent class of pep, by default a Proteinuence */
BOOL pepReClass (KEY pep, KEY *proteinp) ;  /* gets the parent class of pep, by default a Protein */


/* ace parsing/dumping routines */
BOOL peptideDump (ACEOUT dump_out, KEY k) ;/* DumpFuncType */
ParseFuncReturn peptideParse (ACEIN parse_in, KEY key, char **errtext) ; /* ParseFuncType */


/* FastA dumping routines */
ACEOUT pepFileOpen(STORE_HANDLE handle) ;
BOOL pepTranslateAndOutput(ACEOUT pep_out, KEY obj_key, Array obj_dna) ;
BOOL pepTranslateAndExport(KEY obj_key, Array obj_dna) ;
BOOL pepDumpFastA(Array a, int from, int to, char *text, ACEOUT dump_out, char style);
BOOL pepDumpFastAKey(ACEOUT dump_out, KEY key, char style);
int  pepDumpFastAKeySet(ACEOUT dump_out, KEYSET kSet);
int  pepDumpCDSFastAKeySet(ACEOUT dump_out, KEYSET kSet) ;


int hashArray(Array a);


/* THIS SHOULD NOT BE HERE...SIGH...BUT CAN'T PUT IT IN PEPDISP.H BECAUSE    */
/* OF HEADER CLASHES...                                                      */
/* Pep display create data, controls how pep display will be created, passed */
/* in as a void * to pepDisplay() via displayApp()                           */
typedef struct _PepDisplayData
{
  BOOL CDS_only ;					    /* TRUE: translate only CDS portion of */
							    /* objects DNA.*/
} PepDisplayData ;



#endif   /* !ACEDB_PEPTIDE_H */
/************** end of file **************/
 
 
