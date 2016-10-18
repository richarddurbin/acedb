/*  File: blxview.h
 *  Author: Erik Sonnhammer, 92-02-20
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
 * Description: External interface to blixem window code, this interface
 *              is used both by xace and the stand alone blixem.
 * HISTORY:
 * Last edited: Aug 21 13:57 2009 (edgrif)
 * * Aug 26 16:57 1999 (fw): added this header
 * Created: Thu Aug 26 16:57:17 1999 (fw)
 * CVS info:   $Id: blxview.h,v 1.41 2009-09-02 14:35:34 edgrif Exp $
 *-------------------------------------------------------------------
 */
#ifndef DEF_BLXVIEW_H
#define DEF_BLXVIEW_H

#include <wh/regular.h>
#include <wh/graph.h>


/* Only used in pephomolcol.c, would be good to get rid of this.... */
#define FULLNAMESIZE 255



/* blixem can use either efetch (default) or a pfetch server to get
 * sequences, to use pfetch node/port information must be specified. */
typedef struct
{
  char *net_id ;
  int port ;
} PfetchParams ;


/* Blixem can read a number of file formats which are documented in:
 * 
 * The following defines give keyword strings for these file formats
 * which should be used in code that writes these files.
 * 
 *  */
#define BLX_GAPS_TAG "Gaps"
#define BLX_DESCRIPTION_TAG "Description"
#define BLX_SEQUENCE_TAG "Sequence"



/* strand and frame are given in the form "([+|-][0|1|2|3])", these macros return
 * the strand or frame and should be used like this:.
 * 
 * int frame ;
 * frame = MSPFRAME(msp->qframe) ;
 * 
 * char strand ;
 * strand = MSPSTRAND(msp->qframe) ;
 *  */
#define MSPFRAME(MSP_FRAME) \
  (atoi((MSP_FRAME)[2]))

#define MSPSTRAND(MSP_FRAME) \
  ((MSP_FRAME)[1])

#define MSP_IS_FORWARDS(MSP_FRAME) \
  (MSPSTRAND((MSP_FRAME)) == '+' ? TRUE : FALSE)

/* Types of MSP */
typedef enum
  {
    BLX_MSP_INVALID,
    EXBLX, SEQBL,					    /* Old style sequence entries. */
    EXBLX_X, SEQBL_X,					    /* New style sequence entries with
							       gaps and match strand. */
    HSP,
    GSP, GSPdata, GFF,
    FSSEG,
    XY, XYdata,
    SEQ, SEQdata
  } BlxMSPType ;

typedef struct _MSP
{
  struct _MSP *next;
  BlxMSPType type;					    /* See enum above */
  int      score;
  int      id;

  char    *qname;		/* For Dotter, the MSP can belong to either sequence */
  char     qframe[8];		
  int      qstart;
  int      qend;

  char    *sname;	
	
  /* Change to sstrand, frame for subject makes no sense.... */
  char     sframe[8];

  int      slength ;
  int      sstart; 
  int      send;
  char    *sseq;

  char    *desc;
  int      box ;
  Graph    graph;

  BOOL     in_match_set ;				    /* MSP's in the match set are shown
							       with different colour. */

  int      color;   
  int      shape;   /* For SFS data, e.g. XY type PARTIAL or INTERPOLATE shapes */
  int      fs;      /* Ordinal number of the series that this MSP belongs to. */

  Array    xy;      /* For XY plot series */

  Array    gaps;    /* gaps in this homolgy */

#ifdef ACEDB
  KEY      key;
#endif
} MSP ;


/* Function to show blixem window, can be called from any application. */
Graph blxview (char *seq, char *seqname,
	       int dispStart, int offset, MSP *msp, char *opts, 
	       PfetchParams *pfetch, char *align_types, BOOL External) ;


#endif /*  !defined DEF_BLXVIEW_H */
