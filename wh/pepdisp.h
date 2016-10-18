/*  File: pepdisp.h
 *  Author: Clive Brown (cgb@sanger.ac.uk)
 *  Copyright (C) J Thierry-Mieg and R Durbin, 1995
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
 * 	Richard Durbin (MRC LMB, UK) rd@sanger.ac.uk, and
 *	Jean Thierry-Mieg (CRBM du CNRS, France) mieg@crbm.cnrs-mop.fr
 *
 * Description:
 * Exported functions:
 * HISTORY:
 * Last edited: Mar 15 13:28 2001 (edgrif)
 * Created: Tue Nov  7 14:40:38 1995 (cgb)
 * CVS info:   $Id: pepdisp.h,v 1.10 2001-03-15 13:55:40 edgrif Exp $
 *-------------------------------------------------------------------
 */
#ifndef ACEDB_PEPDISP_H
#define ACEDB_PEPDISP_H

#define ARRAY_CHECK

#include <assert.h>
#include <wh/acedb.h>
#include <wh/a.h>
#include <wh/bs.h>
#include <wh/lex.h>
#include <whooks/classes.h>
#include <whooks/sysclass.h>
#include <whooks/systags.h>
#include <whooks/tags.h>

#include <wh/peptide.h>

#include <wh/display.h>
#include <wh/colcontrol.h>


#define BIG_FLOAT 10000000
#define SMALL 0.0000001
#define PEP_DEBUG
#define DEF_WRAP 7
#define DEF_RES ""
#define MAXMESSAGETEXT 200
#define MAXACTIVETEXT 12
#define activecol CYAN
#define friendcol PALECYAN
#define keyfriendcol PALECYAN


/* the LOOK structure for peptide display */

typedef struct {
  MAPCONTROL map ;		/* the corresponding map */
  Array pep ;/* actual peptide sequence - coded 0..20 */
  int messageBox,aboutBox;
  char messageText[MAXMESSAGETEXT];
  int activeregionBox,activeStart,activeEnd;
  char activeText[MAXACTIVETEXT];
  KEY titleKey;
  int startcol;
  int endcol;
  BOOL block;
} PEPLOOK ;



#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
/* HAD TO PUT THIS IN peptide.h FOR NOW BECAUSE OF HEADER CLASHES WITH       */
/* MAPCONTROL AND COLCONTROL...sigh...                                       */

/* Pep display create data, controls how pep display will be created, passed */
/* in as a void * to pepDisplay() via displayApp()                           */
typedef struct _PepDisplayData
{
  BOOL CDS_only ;					    /* TRUE: translate only CDS portion of */
							    /* objects DNA.*/
} PepDisplayData ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */



extern BOOL pepDisplay(KEY, KEY, BOOL, void *app_data) ;

extern void colBox(Array *colMap);
extern BOOL pepSequenceCreate (COLINSTANCE, OBJ );
extern BOOL hydrophobCreate (COLINSTANCE, OBJ );
extern BOOL homolCreate (COLINSTANCE, OBJ );
extern BOOL homolNameCreate (COLINSTANCE, OBJ );
extern BOOL pepFeatureCreate (COLINSTANCE, OBJ );
extern BOOL activeZoneCreate (COLINSTANCE, OBJ );
extern void * homolConvert (MAPCONTROL, void *);
extern void * FeatureConvert (MAPCONTROL, void *);

#endif /* !ACEDB_PEPDISP_H */
