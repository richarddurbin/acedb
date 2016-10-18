/*  File: dotter_.h
 *  Author: esr
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
 * Description: Private include file for dotter
 * Exported functions: 
 * HISTORY:
 * Last edited: Sep 15 08:36 2006 (edgrif)
 * Created: Thu Aug 26 17:17:58 1999 (fw)
 * CVS info:   $Id: dotter_.h,v 1.7 2006-09-15 10:12:19 edgrif Exp $
 *-------------------------------------------------------------------
 */

#include "dotter.h"

#define AAID_MIN	1	/* Smallest letter value in binary alphabet */
#define AAID_MAX	24	/* Maximum letter value in binary alphabet */
#define AAID_CNT	24	/* Number of letters in alphabet */
#define AAID_NAR	(AAID_MAX+1)
#define AAID_IGNORE	(AAID_MAX+3)

#define NA 24 		        /* Not A residue */
#define EL (NA+1)		/* End of Line */
#define ES (NA+2)		/* End of Sequence */
#define IC AAID_IGNORE	/* Ignore this Character */

#define UNKNOWN_AA_CHR	'X'
#define STOP_AA_CHR	'*'
#define GAP_AA_CHR '-'

#define NR 23 		        /* Not A residue */
#if !defined(NAMESIZE)
#define NAMESIZE 10
#endif

char *translate(char *seq, char **code);
extern char *stdcode1[];        /* 1-letter amino acid translation code */

int winsizeFromlambdak(int mtx[24][24], int *tob, int abetsize, char *qseq, char *sseq, 
		       double *exp_res_score, double *Lambda);

#if !defined BELVU
/* Horrifically the headers for belvu, dotter and blixem all include each other for
 * inappropriate reasons..there should some common headers/.o files and all would be
 * well.... */
void fatal(char *format, ...);
#endif

void argvAdd(int *argc, char ***argv, char *s);

