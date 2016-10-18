/*  File: translate.c
 *  Author: sre
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
 * Description: functions for translating nucleic acid sequence
 *              
 * Exported functions: see blixem_.h
 *              
 * HISTORY:
 * Last edited: Dec  4 10:40 2009 (edgrif)
 * Created: Tue Jan 12 11:27:29 1993 (SRE)
 * CVS info:   $Id: translate.c,v 1.13 2009-12-04 11:03:33 edgrif Exp $
 *-------------------------------------------------------------------
 */

#include <glib.h>
#include <wh/mystdlib.h>
#include <wh/regular.h>
#include <w9/blixem_.h>
#include <w9/iupac.h>


/* THIS FILE NEEDS RENAMING TO SOMETHING LIKE utils.c */

static char complement(char *seq) ;


/* For alignments from the forward strand strand of the match (subject) sequence
 * the sequence can simply be set in the MSP, for the reverse strand it must be
 * reverse complemented first. */
void blxSeq2MSP(MSP *msp, char *seq_in)
{
  char *seq = seq_in ;

  /* Make sure qstart is the lower value in the range and qend the upper value
   * if the q strand is forwards, or the opposite if reversed. Same for the s strand. */
  sortValues(&msp->qstart, &msp->qend, MSP_IS_FORWARDS(msp->qframe));
  sortValues(&msp->sstart, &msp->send, MSP_IS_FORWARDS(msp->sframe));

  /* We are always given the forward strand of the s sequence. Complement it if
   * the match is on the reverse strand. */
  if (!MSP_IS_FORWARDS(msp->sframe))
    {
      seq = strnew(seq_in, 0) ;
      blxComplement(seq) ;
    }
  
  msp->sseq = seq ;
}


/* Sorts the given values so that val1 is less than val2 if forwards is true,
 * or the reverse if forwards is false. */
void sortValues(int *val1, int *val2, gboolean forwards)
{
  if ((forwards && *val1 > *val2) || (!forwards && *val1 < *val2))
    {
      int temp = *val1;
      *val1 = *val2;
      *val2 = temp;
    }
}







/* Function: Translate(char *seq, char **code)
 * 
 * Given a ptr to the start of a nucleic acid sequence, and a genetic code, translate the sequence
 * into amino acid sequence.
 * 
 * code is an array of 65 strings, representing the translations of the 64 codons, arranged in
 * order AAA, AAC, AAG, AAU, ..., UUA, UUC, UUG, UUU.  '*' or '***' is used to represent
 * termination codons, usually. The final string, code[64], is the code for an ambiguous amino
 * acid.
 *
 * Because of the way space is allocated for the amino acid sequence, the amino acid strings
 * cannot be longer than 3 letters each. (I don't foresee using anything but the single- and
 * triple- letter codes.)
 * 
 * Returns a ptr to the translation string on success, or NULL on failure.
 */
char *blxTranslate(char  *seq, char **code)
{
  char *aaseq = NULL ;					    /* RETURN: the translation */
  int   codon;						    /* index for codon         */
  char *aaptr;						    /* ptr into aaseq */
  int   i;
  
  if (seq && *seq)
    {
      aaseq = (char *)messalloc(strlen(seq) + 1) ;

      for (aaptr = aaseq ; *seq != '\0' && *(seq+1) != '\0' && *(seq+2) != '\0'; seq += 3)
	{
	  /* calculate the lookup value for this codon */
	  codon = 0;
	  for (i = 0; i < 3; i++)
	    {
	      codon *= 4;

	      switch (*(seq + i))
		{
		case 'A': case 'a':             break;
		case 'C': case 'c': codon += 1; break;
		case 'G': case 'g': codon += 2; break;
		case 'T': case 't': codon += 3; break;
		case 'U': case 'u': codon += 3; break;
		default: codon = 64; break;
		}

	      if (codon == 64)
		break;
	    }

	  strcpy(aaptr, code[codon]);
	  aaptr += strlen(code[codon]);
	}
    }  

  return aaseq ;
}


/* All these calls need rationalising into a single function with options. */

/* revcomp.c
 * 
 * Reverse complement of a IUPAC character string
 * 
 */
/* Ratinalise this with my func. below..... */
#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
char *revcomp(char *comp, char *seq)
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */
char *revComplement(char *comp, char *seq)
{
  long  bases;
  char *bckp, *fwdp;
  int   idx;
  long  pos;
  int   c;

  if (comp == NULL)
    return NULL;
  if (seq == NULL)
    return NULL;

  bases = strlen(seq);

  fwdp = comp;
  bckp = seq + bases -1;
  for (pos = 0; pos < bases; pos++)
    {
      c = *bckp;
      c = freeupper(c);

      for (idx = 0; c != iupac[idx].sym && idx < IUPACSYMNUM; idx++);

      if (idx > IUPACSYMNUM)
	{
	  *fwdp = '\0';
	  return NULL;
	}
      else
	{
	  *fwdp = iupac[idx].symcomp;
	}

      if (islower(*bckp))
	*fwdp = freelower(*fwdp);

      fwdp++;
      bckp--;
    }
  *fwdp = '\0';

  return comp;
}
  


/* Reverse complement of a IUPAC character string. If revcomp_seq_out
 * is NULL the reverse complement is done _inplace_, otherwise a new
 * reverse-complemented sequence is returned in revcomp_seq_out.
 * 
 * No validating of the input sequence is done, if a symbol cannot be
 * translated it is simply ignored, this behaviour preserves common
 * gap characters like '-'.
 *  */
BOOL blxRevComplement(char *seq_in, char **revcomp_seq_out)
{
  BOOL result = FALSE ;
  char *seq, *start , *end ;
  int  bases ;


  if (seq_in && *seq_in)
    {
      int i ;

      if (revcomp_seq_out)
	seq = *revcomp_seq_out = g_strdup(seq_in) ;
      else
	seq = seq_in ;

      bases = strlen(seq) ;

      start = seq ;
      end = seq + bases - 1 ;

      for (i = 0 ; i < (bases / 2) ; i++, start++, end--)
	{
	  char cs, ce ;

	  cs = complement(start) ;
	  ce = complement(end) ;

	  *end = cs ;
	  *start = ce ;
	}

      if (bases % 2 != 0)				    /* Do middle base if there is one. */
	*start = complement(start) ;

      result = TRUE ;
    }

  return result ;
}
  

/* compl.c
 * 
 * Just complement of a IUPAC character string
 * 
 * Note that it overwrites calling string!!!! (revcomp doesn't)
 */

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
void compl(char *seq)
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */
void blxComplement(char *seq)
{
  char *fwdp;
  int   idx;
  long  pos;
  int   c;

  if (seq == NULL)
    return ;

  fwdp = seq;
  for (pos = 0; pos < strlen(seq); pos++)
    {
      c = freeupper(*fwdp);

      for (idx = 0; c != iupac[idx].sym && idx < IUPACSYMNUM; idx++);

      if (idx > IUPACSYMNUM)
	{
	  *fwdp = '\0';
	  return;
	}

      else c = iupac[idx].symcomp;

      if (islower(*fwdp))
	*fwdp = freelower(c);
      else
	*fwdp = c;

      fwdp++;
    }

  *fwdp = '\0';

  return ;
}
  



/* 
 *                Internal routines
 */


/* Takes a string and complements it using IUPAC coding.
 * 
 * Any symbols in the string that are not IUPAC codes are left unchanged,
 * this preserves symbols used for missing sequence etc e.g. '-'.
 * 
 * Case of the input string is maintained.
 *  */
static char complement(char *seq)
{
  char comp = ' ' ;
  int   idx ;

  comp = *seq ;
  comp = freeupper(comp) ;

  for (idx = 0 ; idx < IUPACSYMNUM ; idx++)
    {
      if (comp == iupac[idx].sym)
	{
	  comp = iupac[idx].symcomp ;
	  break ;
	}
    }

  if (islower(*seq))
    comp = freelower(comp) ;

  return comp ;
}




