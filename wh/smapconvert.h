/*  File: smapconvert.h
 *  Author: Ed Griffiths (edgrif@sanger.ac.uk)
 *  Copyright (c) J Thierry-Mieg and R Durbin, 2002
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
 * Description: A perhaps temporary header to provide definitions of
 *              smap-based functions to prepare virtual contigs, get
 *              segs arrays for gifs etc. etc.
 * HISTORY:
 * Last edited: Jan  7 12:04 2011 (edgrif)
 *
 * Created: Wed Mar 20 13:29:04 2002 (edgrif)
 * CVS info:   $Id: smapconvert.h,v 1.20 2012-10-30 10:04:38 edgrif Exp $
 *-------------------------------------------------------------------
 */
#ifndef SMAPCONVERT_H
#define SMAPCONVERT_H

#include <wh/smap.h>
#include <wh/dict.h>

#include <wh/methodcache.h>				    /* uuugggghhh, don't know if we want
							       to this actually..... */


/* Some predefined builtin method names for certain feature types that
 * are not in one particular object, assembly_path. */
#define ASSEMBLY_PATH_METHOD_NAME "Assembly_path"


/* Opaque type containing context for smap conversions.                      */
typedef struct _SMapFeatureMapRec *SMapFeatureMap ;

/* Create a feature map according to request in command_in (see gifcommand.c)*/
/* and return errors in reply_out. Destroys map_in if non-NULL.              */
SMapFeatureMap sMapFeaturesCreate(ACEIN command_in, ACEOUT reply_out, SMapFeatureMap map_in) ;

void sMapFeaturesDestroy(SMapFeatureMap map) ;

/* Create a feature map from the given smap and seqKey, this is a lower level*/
/* version of sMapFeaturesCreate().                                          */
BOOL sMapFeaturesMake(SMap *smap, KEY seqKey, KEY seqOrig,
		      int start, int stop, int length,
		      Array oldsegs, BOOL complement,
		      MethodCache mcache,  DICT *mapped_dict_inout,
		      Array dna,
		      KEYSET method_set, BOOL include_methods,
		      DICT *featDict, DICT *sources_dict, BOOL include_sources,
		      Array segs_inout,
		      Array seqinfo_inout, Array homolInfo_inout, Array featureinfo_inout,
		      BitSet homolBury_inout, BitSet homolFromTable_inout,
		      BOOL keep_selfhomols, BOOL keep_duplicate_introns,
		      STORE_HANDLE segsHandle) ;

void sMapFeaturesComplement(int length, Array segs, Array seqinfo, Array homoinfo) ;

/* Yet to be finalised...                                                    */
KEYSET sMapFeaturesSet(STORE_HANDLE handle, 
		       DICT methods,
		       BOOL include_methods) ;


#endif /* SMAPCONVERT_H */
