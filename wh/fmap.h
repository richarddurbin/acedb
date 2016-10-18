/*  File: fmap.h
 *  Author: Richard Durbin (rd@mrc-lmb.cam.ac.uk)
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
 * 	Richard Durbin (MRC LMB, UK) rd@mrc-lmb.cam.ac.uk, and
 *	Jean Thierry-Mieg (CRBM du CNRS, France) mieg@kaa.cnrs-mop.fr
 *
 * Description: Public header file for fmap = DNA sequence package
 * Exported functions:
 * HISTORY:
 * Last edited: Jan  7 10:38 2011 (edgrif)
 * * Apr 19 15:34 2000 (edgrif): Remove some mapColFuncs and replace with
 *              access functions, called from mapcontrol.c
 * * Jan  8 11:27 1999 (edgrif): Added missing func dec for fMapGifAlign.
 * * Nov 19 09:11 1998 (edgrif): TINT_<colour> defs should be unsigned.
 * * Jul 27 10:38 1998 (edgrif): Add final declarations of external
 *      fmap routines.
 * * Jul 23 09:09 1998 (edgrif): The original fmap has mostly now gone
 *      into the private header fmap_.h. This header now includes only
 *      fmap functions that are used externally to fmap.
 * Created: Sat Jul 25 20:28:37 1992 (rd)
 * CVS info:   $Id: fmap.h,v 1.89 2012-10-30 10:03:28 edgrif Exp $
 *-------------------------------------------------------------------
 */
#ifndef ACEDB_FMAP_H
#define ACEDB_FMAP_H
/*                  NO CODE BEFORE THIS                                      */

#include <wh/acedb.h>				  /* necessary include files */
#include <wh/graph.h>
#include <wh/bs.h>
#include <wh/map.h>
#include <wh/dict.h>

/* Colours used for display by fMap, placed here for use by packages         */
/* cooperating with fMap.                                                    */
/* (internal note: tint values must match fMapTints array in fmapsequence.c) */
/* You will probably assign these to a char, make sure its declared as       */
/* unsigned.                                                                 */
#define TINT_HIGHLIGHT1 0x01U			  /* highest priority, dna highlighting.*/
#define TINT_HIGHLIGHT2 0x02U			  /* highlight friends.*/
#define TINT_RED        0x04U 
#define TINT_LIGHTGRAY  0x08U 
#define TINT_MAGENTA    0x10U 
#define TINT_CYAN       0x20U 
#define TINT_LIGHTGREEN 0x40U 
#define TINT_YELLOW     0x80U 


/* public opaque type which holds all fMap data per window */
typedef struct FeatureMapStruct *FeatureMap;


/* By default when the user clicks on an object, if dna is displayed, the
 * dna for that object is put in the cut buffer. Now the user can select to
 * put the objects coords in the cut buffer. I expect there will be other
 * types of cut data as well. */
typedef enum {FMAP_CUT_DNA, FMAP_CUT_OBJDATA} FMapCutDataType ;


/************************************************************/

/* fMap display data, controls various aspects of creation/destruction       */
/* of fMap. Passed via displayApp() to fMapDisplay() as a void *             */
typedef struct _FmapDisplayData
{
  BOOL destroy_obj ;					    /* TRUE: destroy "key" obj. passed to
							       fMapDisplay. */

  BOOL maptable ;					    /* TRUE: create a hash table of
							       feature names to segs mapped. */

  BOOL include_methods ;				    /* whether to include or exclude */
  BOOL include_sources ;				    /* whether to include or exclude */
  DICT *methods_dict ;					    /* the methods DICT.           */
  DICT *sources_dict ;					    /* the sources DICT            */
  DICT *features ;

  BOOL keep_selfhomols, keep_duplicate_introns ;

} FmapDisplayData ;

/************************************************************/

/* Public fmap functions.  */

	/* fmapcontrol.c */
BOOL  fMapDisplay (KEY key, KEY from, BOOL isOldGraph, void *display_data) ;
Graph fMapActiveGraph(void) ;
BOOL  fMapActive(Array *dnap, Array *dnaColp, KEY *seqKeyp, FeatureMap* lookp);
BOOL  fMapFindZone(FeatureMap look, int *min, int *max, int *origin) ;
BOOL  fMapFindSpanSequence(FeatureMap look, KEY *keyp, int *start, int *end) ;
BOOL  fMapFindSpanSequenceWriteable(FeatureMap look, KEY *keyp, int *start, int *end) ;
void  fMapDraw(FeatureMap look, KEY from) ;
void  fMapDumpSegsKeySet (KEYSET kSet, ACEOUT gff_out);
MAP   fMapGetMap (FeatureMap look) ;
void  fMapSetDefaultCutDataType(FMapCutDataType cut_data) ;
void  fMapDestroy (FeatureMap look) ;


	/* fmapsequence.c */
void fMapReDrawDNA(FeatureMap look) ;
Array fMapFindDNA(KEY seq, int *start, int *stop, BOOL reportMismatches) ;

/* Used by gifcontrol.c                                                      */
FeatureMap fMapGifGet (ACEIN command_in, ACEOUT result_out, FeatureMap look) ;	/* returns a handle used by other fMapGif*() */
void fMapGifDisplay (ACEIN command_in, ACEOUT result_out, FeatureMap look) ;
void fMapGifActions (ACEIN command_in, ACEOUT result_out, FeatureMap look) ;
void fMapGifColumns (ACEIN command_in, ACEOUT result_out, FeatureMap look) ;
void fMapGifAlign (ACEIN command_in, ACEOUT result_out, FeatureMap look) ;
void fMapGifFeatures (ACEIN command_in, ACEOUT result_out, FeatureMap look, BOOL quiet) ;
void fMapGifDNA (ACEIN command_in, ACEOUT result_out, FeatureMap look, BOOL quiet);
void fMapGifsMap(ACEIN command_in, ACEOUT result_out) ;
void fMapGifRecalculate(ACEIN command_in, ACEOUT result_out, FeatureMap look) ;
void fMapGifActiveZone(ACEIN command_in, ACEOUT result_out, FeatureMap look) ;

/* testing... */
FeatureMap fMapEdGifGet(ACEIN command_in, ACEOUT result_out, FeatureMap look) ;

/* Used by gfcode.c & hexcode.c                                              */
void fMapAddGfSite (int type, int pos, float score, BOOL comp) ;
void fMapAddGfCodingSeg (int pos1, int pos2, float score, BOOL comp) ;
void fMapAddGfSeg (int type, KEY key, int x1, int x2, float score) ;

/* Used by map stuff in w7 at least.  */
/* A bit of a hack, we need to find out if our columns will be drawn by a    */
/* function of a particular type, this is necessary in mapcontrol.c for      */
/* instance, or if they will need a pull down menu for column configuration. */
/*                                                                           */
/* Really this should be overhauled and will probably have to be as we move  */
/* towards interactive configuration.                                        */
/*                                                                           */
BOOL fmapColHasMenu(MapColDrawFunc func) ;
BOOL fmapIsFrameFunc(MapColDrawFunc func) ;
BOOL fmapIsGeneTranslationFunc(MapColDrawFunc func) ;


/* Used in a number of fmap routines + in geldisp.c                          */
void fMapRegisterSites(FeatureMap look, Array sites, Stack sname) ;

/* Used in plotseq.c, trace.c and other wabi programs...   */
void fMapClearDNA (FeatureMap look) ;
void fMapPleaseRecompute (FeatureMap look) ;
void fMapDrawFromTrace (FeatureMap look, KEY from, int xx, int type) ;

void fMapTraceReassembleAll (void) ; /* used in xacembly */

/*                  NO CODE AFTER THIS                                       */
#endif /* ACEDB_FMAP_H */
 
