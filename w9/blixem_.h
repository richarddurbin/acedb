/*  File: blixem_.h
 *  Author: Ed Griffiths (edgrif@sanger.ac.uk)
 *  Copyright (c) J Thierry-Mieg and R Durbin, 2001
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
 * Description: Internal header for blixem code.
 * HISTORY:
 * Last edited: Aug 26 09:09 2009 (edgrif)
 * Created: Thu Nov 29 10:59:09 2001 (edgrif)
 * CVS info:   $Id$
 *-------------------------------------------------------------------
 */
#ifndef DEF_BLIXEM_P_H
#define DEF_BLIXEM_P_H

#include <gtk/gtk.h>
#include <wh/version.h>
#include <wh/dict.h>
#include <wh/smap.h>
#include <wh/blxview.h>


/*            blixem program version and information.                        */
#define BLIXEM_TITLE   "Blixem program"
#define BLIXEM_DESC    "Sequence alignment tool."

#define BLIXEM_VERSION 3
#define BLIXEM_RELEASE 1
#define BLIXEM_UPDATE  1
#define BLIXEM_VERSION_NUMBER  UT_MAKE_VERSION_NUMBER(BLIXEM_VERSION, BLIXEM_RELEASE, BLIXEM_UPDATE)
#define BLIXEM_VERSION_STRING  UT_MAKE_VERSION_STRING(BLIXEM_VERSION, BLIXEM_RELEASE, BLIXEM_UPDATE)
#define BLIXEM_TITLE_STRING    UT_MAKE_TITLE_STRING(BLIXEM_TITLE, BLIXEM_VERSION, BLIXEM_RELEASE, BLIXEM_UPDATE)
#define BLIXEM_VERSION_COMPILE BLIXEM_VERSION_STRING "  " __TIME__ " "__DATE__


/* This will probably never be completed but I want to start creating a blixem context....which
 * will require the following steps:
 * 
 * 1) Create a context struct which is initially a "global", i.e. is not passed from function
 *    to function but just accessed directly.
 * 
 * 2) Put all new fields into this struct.
 * 
 * 3) gradually move other fields into it.
 * 
 * 4) gradually change function signatures so that the struct is passed to functions that need it.
 * 
 *  */
typedef struct BlixemViewStructName
{


  BOOL match_set ;


} BlixemViewStruct, *BlixemView ;



/* Really the buffers that use this should be dynamic but I'm not going to do that, this
 * code is so poor that it doesn't warrant the effort.... */
#define NAMESIZE    12
#define LONG_NAMESIZE 1000

#define INITDBSEQLEN 50000				    /* Initial estimate of max database sequence length */

#define MAXLINE 10000


/* Fundamental type of sequence. */
typedef enum {BLXSEQ_INVALID, BLXSEQ_DNA, BLXSEQ_PEPTIDE} BlxSeqType ;


/* Types and struct to support retrieving data from the window systems clipboard. */
typedef enum
  {
    BLXPASTE_INVALID,
    BLXPASTE_MATCHSET,					    /* Data is a set of matche names. */
    BLXPASTE_MOVETO					    /* Data is a single match. */
  } BlxPasteType ;

typedef struct
{
  BlxPasteType type ;					    /* defines type of data in union. */
  union
  {
    char **match_set ;
    struct
    {
      char *match ;
      int start, end ;
    } move_to ;
  } data ;
} BlxPasteDataStruct, *BlxPasteData ;



/* remove ?? */
#define max(a,b)        (((a) > (b)) ? (a) : (b))
#define min(a,b)        (((a) < (b)) ? (a) : (b))


#define selectFeaturesStr     "Feature series selection tool"
#define FS(msp) (msp->type == FSSEG || msp->type == XY)
#define XY_NOT_FILLED -1000  /* Magic value meaning "value not provided" */

/* Shapes of XY data */
enum { XY_PARTIAL, XY_INTERPOLATE, XY_BADSHAPE };

  
typedef struct featureSeries_ {
    char  *name;
    int    nr;
    int    on;
    float  x;	      /* Series offset on x axis, to bump series on the screen */
    float  y;	      /* Series offset on y axis */
    int    xy;	      /* Flag for XY plot series */
} FEATURESERIES;



/* 
 * config file groups/keywords, these should not be changed willy nilly as they
 * are used external programs and users when constructing config files.
 */

/* For overall blixem settings. */
#define BLIXEM_GROUP               "blixem"
#define BLIXEM_DEFAULT_FETCH_MODE  "default-fetch-mode"


/* For http pfetch proxy fetching of sequences/entries */
#define PFETCH_PROXY_GROUP      "pfetch-http"
#define PFETCH_PROXY_LOCATION   "pfetch"
#define PFETCH_PROXY_COOKIE_JAR "cookie-jar"
#define PFETCH_PROXY_MODE       "pfetch-mode"
#define PFETCH_PROXY_PORT       "port"

/* For direct pfetch socket fetching of sequences/entries */
#define PFETCH_SOCKET_GROUP  "pfetch-socket"
#define PFETCH_SOCKET_NODE   "node"
#define PFETCH_SOCKET_PORT   "port"






/* Fetch programs for sequence entries. */
#define BLX_FETCH_PFETCH      PFETCH_SOCKET_GROUP
#ifdef PFETCH_HTML 
#define BLX_FETCH_PFETCH_HTML PFETCH_PROXY_GROUP
#endif
#define BLX_FETCH_EFETCH      "efetch"
#define BLX_FETCH_WWW_EFETCH  "WWW-efetch"
#ifdef ACEDB
#define BLX_FETCH_ACEDB       "acedb"
#define BLX_FETCH_ACEDB_TEXT  "acedb text"
#endif





/* Dotter/Blixem Package-wide functions */
Graph blxreadhsp(FILE *seqfile, FILE *exblxfile, char *featurefile, char *qname, 
		 int dispstart, int qoffset, char *opts, int *argc, char **argv);
char *blxTranslate(char *seq, char **code);
char *revcomp(char *comp, char *seq);
void *compl(char *seq);
void  argvAdd(int *argc, char ***argv, char *s);
void  loadFeatures(FILE* fil, MSP **msp);
float fs2y(MSP *msp, float *maxy, float height);
char  Seqtype(char *seq);
void  blviewRedraw(void);
void  selectFeatures(void);
float fsTotalHeight(MSP *msplist);
void  parseFS(MSP **MSPlist, FILE *file, char *opts,
	      char **seq1, char *seq1name, char **seq2, char *seq2name) ;
void insertFS(MSP *msp, char *series);
char *readFastaSeq(FILE *seqfile, char *qname);

void blxPfetchEntry(char *sequence_name) ;
void blxDisplayMSP(MSP *msp) ;
char *blxFindFetchMode(void) ;
void blxSetFetchMode(char *fetch_mode) ;
char *blxGetFetchMode(void) ;
MENUOPT *blxPfetchMenu(void) ;
char *blxGetFetchProg(void) ;

BOOL blxGetSseqsPfetchHtml(MSP *msplist, DICT *dict, BlxSeqType seqType) ;
BOOL blxGetSseqsPfetch(MSP *msplist, DICT *dict, char* pfetchIP, int port, BOOL External) ;
void blxAssignPadseq(MSP *msp) ;

BOOL blxInitConfig(char *config_file, GError **error) ;
GKeyFile *blxGetConfig(void) ;
gboolean blxConfigSetPFetchSocketPrefs(char *node, int port) ;
gboolean blxConfigGetPFetchSocketPrefs(char **node, int *port) ;
char *blxConfigGetFetchMode(void) ;

void blxSeq2MSP(MSP *msp, char *seq) ;

BOOL blxRevComplement(char *seq_in, char **revcomp_seq_out) ;
char *translate(char  *seq, char **code) ;
char *revComplement(char *comp, char *seq) ;
void blxComplement(char *seq) ;

void sortValues(int *val1, int *val2, gboolean forwards) ;

/* Dotter/Blixem Package-wide variables...........MORE GLOBALS...... */
extern char *blixemVersion ;
extern char *stdcode1[];        /* 1-letter amino acid translation code */
extern int   aa_atob[];
extern int PAM120[23][23];
extern Array fsArr;		/* in dotter.c */
extern Graph dotterGraph;
extern float fsPlotHeight;
extern GtkWidget *blixemWindow;




#endif /*  !defined DEF_BLIXEM_P_H */
