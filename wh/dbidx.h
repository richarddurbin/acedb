/*  File: dbidx.h
 *  Author: Erik Sonnhammer, 930209
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
 * Description: Header file for EMBL sequence database indexing system
 * Exported functions:
 * HISTORY:
 * Last edited: Apr  3 16:04 2000 (edgrif)
 * Created: Thu Aug 26 17:11:13 1999 (fw)
 * CVS info:   $Id: dbidx.h,v 1.10 2004-08-12 20:21:50 mieg Exp $
 *-------------------------------------------------------------------
 */

/* LMB-MRC:
#define DEFAULT_SWDIR     "/nfs/al/pubseq/pubseq/seqlibs/swiss/"
#define DEFAULT_PIRDIR    "/nfs/al/pubseq/pubseq/seqlibs/pir/"
#define DEFAULT_WORMDIR   "/nfs/al/pubseq/pubseq/seqlibs/wormpep/"
#define DEFAULT_EMBLDIR   "/nfs/al/pubseq/pubseq/seqlibs/embl/"
#define DEFAULT_GBDIR     "/nfs/al/pubseq/pubseq/seqlibs/genbank/"
*/

/* SANGER: */
#define DEFAULT_SWDIR      "/nfs/disk3/pubseq/swiss/"
#define DEFAULT_PIRDIR     "/nfs/disk3/pubseq/pir/"
#define DEFAULT_WORMDIR    "/nfs/disk3/pubseq/wormpep/"
#define DEFAULT_EMBLDIR    "/nfs/disk3/pubseq/embl/"
#define DEFAULT_GBDIR      "/nfs/disk3/pubseq/genbank/"
#define DEFAULT_PRODOMDIR  "/nfs/disk3/pubseq/prodom/"
#define DEFAULT_PROSITEDIR "/nfs/disk3/pubseq/prosite/"


#define DEFAULT_IDXFILE   "entrynam.idx"
#define DEFAULT_DIVFILE   "division.lkp"  
#define DEFAULT_DBFILE    "seq.dat"
#define DEFAULT_ACTRGFILE "acnum.trg"
#define DEFAULT_ACHITFILE "acnum.hit"

#define max(a,b) (a > b ? a : b)

#include <stdio.h>

#ifdef ACEDB
#include <mystdlib.h>
#if (!defined(WIN32)&&!defined(HP)&&!defined(LINUX)&&!defined(OPTERON)&&!defined(FreeBSD))
  extern char  toupper(char),
               tolower(char); /* Otherwise I have to include regular.h */
#endif
#else
#include <stdlib.h>
#ifdef SUN
  extern void  fclose(FILE*),
              *memcpy(),
              *memset(),
               rewind();

  extern int   strlen(),
               strcmp(),
               strncmp(),
               getopt(),
               fprintf(),
               fwrite(),
               printf(),
               sscanf(),
               fseek(),
               fread(),
               fputc(),
               fputchar();
               
  extern char *strcpy(char*, char*),
              *strchr(),
               toupper(char),
               tolower(char);

#endif
#endif

#if !defined(max) /* should already be in (my)stdlib.h */
#define max(a,b) ((a) > (b) ? (a) : (b))
#endif

/* DATE as described in 3.4.2 in the EMBL CD-ROM Indices documentation 1.4
*/
typedef struct _Date 
{
  char noll;
  char year;
  char month;
  char day;

} Date;


/* HEADER contains the fields of the index file header
   as described in 3.4.2 in the EMBL CD-ROM Indices documentation 1.4
*/
typedef struct _Header {

  unsigned long  file_size;
  unsigned long  nrecords;
  unsigned short record_size;
  unsigned short entry_name_size;
           char  db_name[21];
           char  db_relnum[11];
           Date  db_reldate[4];

} Header;


/* ENTRYNAM contains a record of the index file ENTRYNAM.IDX
*/
typedef struct _Entrynam {

  char entry_name[32];  /* The actual size is calculated from the header of 
			   the index file:  size = record_size - 10 */
  unsigned long  ann_offset;
  unsigned long  seq_offset;
  unsigned short div_code;

} Entrynam;


/* ACNUM contains a record of the index file ACNUM.TRG
*/
typedef struct _Acnum {

  unsigned long  nhits;
  unsigned long  hit_rec_num;
  char acnum[32];  /* The actual size is calculated from the header of 
			   the index file:  size = record_size - 10 */
} Acnum;


/* ACHIT contains a record of the index file ACNUM.HIT
*/
typedef struct _Achit {

  unsigned long  entry_rec_num;

} Achit;



/* DIVISION contains a record of the division lookup table division.lkp
*/
typedef struct _Division {

  unsigned short div_code;
  char file_name[128]; /* This may vary. EMBL uses 12 */

} Division;


extern int 
    Reversebytes, 
    search, 
    display_head, 
    fasta,
    seqonly,
    raw_seq, 
    PIR_seq,
    ACsearch,
    AC2entryname,
    Exact_match,
    Mixdb,
    Mini;

Header *readheader(FILE *idx);
Entrynam *readEntrynam(FILE *idx, Header *head, int pos);
Entrynam *readnextEntrynam(FILE *idx, Header *head);
Acnum *readActrg(FILE *idx, Header *head, int pos);
void printEntrynam(Entrynam *rec, Header *head);
char *getname(FILE *idx, Header *head, int pos);
char *getdbfile(int div_code, char *divfile);
int bin_search(char *query, FILE *idx, Header *head, char *(*getname)(FILE *idx, Header *head, int pos) );
char *getAcname(FILE *idx, Header *head, int pos);

void printActrg(),
     printPIRseq(),
     printGBseq(),
     printEMBLseq(),
     printAnn(),
     printheader();

char *printFastaseq();

int  getAcentry_rec_num();


/********************** end of file *********************/
 
