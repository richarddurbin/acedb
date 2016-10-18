/*  File: method.h
 *  Author: Richard Durbin (rd@sanger.ac.uk)
 *  Copyright (C) J Thierry-Mieg and R Durbin, 1992
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
 *	Jean Thierry-Mieg (CRBM du CNRS, France) mieg@kaa.cnrs-mop.fr
 *
 * Description: methods header file for fmap & pepmap = sequence displays
 * Exported functions:
 * HISTORY:
 * Last edited: Dec  9 15:59 2009 (edgrif)
 * * Jul 27 09:34 1998 (edgrif): Removed #define METHOD_HOMOL, it's not
 *      found anywhere else....
 * * Jun 24 13:40 1998 (edgrif): Removed reference to methodInitialise,
         this is now an internal routine.
 * Created: Sat Jul 25 20:28:37 1992 (rd)
 * CVS info:   $Id: method.h,v 1.34 2009-12-10 10:04:29 edgrif Exp $
 *-------------------------------------------------------------------
 */

#ifndef ACEDB_METHOD_H
#define ACEDB_METHOD_H

#include <wh/acedb.h>

/* Defines the way columns are displayed when there are lots of features overlapping within
 * a column. */
typedef enum {METHOD_OVERLAP_COMPLETE, METHOD_OVERLAP_BUMPED, METHOD_OVERLAP_CLUSTER} MethodOverlapModeType ;


/* WE SHOULD JUST USE A BIT FIELD FOR ALL OF THIS... */
/* Flags for boolean properties of Method. */
#define METHOD_FRAME_SENSITIVE	0x00000004U /* if tag "Frame_sensitive" */
#define METHOD_STRAND_SENSITIVE	0x00000008U /* if tag "Stand_sensitive" */
#define METHOD_SHOW_UP_STRAND	0x00010000U /* if tag "Show_up_strand" */

#define METHOD_SCORE_BY_OFFSET  0x00000010U /* if tag "Score_by_offset" */
#define METHOD_SCORE_BY_WIDTH   0x00000020U /* if tag "score_by_width" */
#define METHOD_SCORE_BY_HIST	0x00002000U /* if tag "Score_by_histogram"*/
#define METHOD_PERCENT          0x00000400U /* if tag "Percent"
					      defaults Score_bounds
					      min/max to 25/100 
					      (can be overridden) */

#define METHOD_BUMPABLE		0x00000200U /* if tag "Bumpable" */
#define METHOD_CLUSTER          0x00020000U /* if tag "Cluster" */

#define METHOD_BLASTN		0x00000001U /* if tag "BlastN" -
					      can calculate percent 
					      from score */
#define METHOD_BLIXEM_X		0x00000040U /* if tag "Blixem_X" */
#define METHOD_BLIXEM_N		0x00000080U /* if tag "Blixem_N" */
#define METHOD_BLIXEM_P		0x00008000U /* if tag "Blixem_P" */

#define METHOD_BELVU    	0x00004000U /* if tag "Belvu"
					       esr, for PEPMAP */

#define METHOD_EMBL_DUMP	0x00001000U /* if tag "EMBL_dump" */

#define METHOD_DONE		0x00000002U /* if method already cached */
#define METHOD_CALCULATED	0x00000800U /* used in addOldSegs */
#define METHOD_DISPLAY_GAPS     0x00040000U /* Show gapped sequences and homols */
#define METHOD_JOIN_BLOCKS      0x00080000U /* Join all blocks of a single match with lines. */

#define METHOD_ALLOW_MISALIGN   0x00100000U		    /* Allows mapping when ref/match coords different. */
#define METHOD_ALLOW_CLIPPING   0x00200000U		    /* Allows mapping when align clipped. */
#define METHOD_MAP_GAPS         0x00400000U		    /* Align gaps are mapped. */
#define METHOD_EXPORT_COORDS    0x00800000U		    /* Gaps exported in acedb "Gaps" form. */
#define METHOD_EXPORT_STRING    0x01000000U		    /* Gaps exported in original string form. */

/*#define METHOD_CACHED		0x00000100U * set if Method exists
					    * in cache and has a
					    * real obj in the DB */




/* convenience flag */
#define METHOD_SCORE		(METHOD_SCORE_BY_OFFSET | \
				 METHOD_SCORE_BY_WIDTH | \
				 METHOD_SCORE_BY_HIST)




typedef struct 
{ 
  magic_t *magic;		/* == &METHOD_MAGIC */

  char *name;			/* name of Method,
				 * freefloating allocation */
  KEY key;			/* KEY of this obj in class ?Method
				 * must always be lexAliasOf-key !!! */

  char *remark;			/* Text from tag "Remark"
				 * freefloating allocation */

  /* Display params */
#ifdef PROTOTYPED_CODE_BY_FW_NOT_USED_YET
  MethodOverlapModeType overlap_mode;/* not used yet - needs changes to method.c & methodcache.c */
#endif /* PROTOTYPED_CODE_BY_FW_NOT_USED_YET */

  unsigned int flags ;

  int colour;						    /* Colour after Display->Colour tag */
  int CDS_colour;					    /* Colour after "CDS_colour" tag */
  int upStrandColour ;					    /* Colour after Show_up_strand tag */
  float minScore, maxScore;				    /* Floats after tag "Score_bounds" */
  float minMag, maxMag;					    /* Float after "Min/Max_Mag" tag */
  float width ;						    /* Float after "Width" tag */
  char symbol ;						    /* first char of Text after "Symbol" tag */
  float priority ;					    /* Float after tag "Right_priority" */
  float histBase ;					    /* Float after tag "Score_by_histogram"  defaults to meth->minScore */
  BOOL isShowText;					    /* existence of tag "Show_Text" */

  BOOL no_display ;					    /* If TRUE then object is _never_
							       displayed. */

  /***************/
  BOOL isCached;
} METHOD ;

extern Array methodInfo ;	/* of METHOD* indexed by KEYKEY(method) */


/**************** old functions *******************************/

int methodAdd (KEY method) ;
METHOD *method(KEY method);
void methodSet (char *name, int col, unsigned int flags, float priority,
		float width, char symbol, float min, float max);

/**************** new method routines *******************/

/* verify struct */
BOOL methodExists (METHOD *meth);

/* make & init struct */
METHOD *methodCreateFromObj (KEY methodKey, STORE_HANDLE handle);
METHOD *methodCreateFromDef (char *methodName, char *aceText, 
			     STORE_HANDLE handle);

/* kill struct (simple messfree on the pointer is sufficient also) */
void methodDestroy (METHOD *meth);

/* store struct in DB */
BOOL methodSave (METHOD *meth);

/* get & apply diffs between structs and DB-objs */
char *methodGetDiff (METHOD *meth, STORE_HANDLE handle);
METHOD *methodCopy (METHOD *meth, char *aceDiff, STORE_HANDLE handle);

#endif /* !ACEDB_METHOD_H */

/**********************************************************/
 
 
 
 
 
 
 
