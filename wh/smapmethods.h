/*  File: smapmethods.h
 *  Author: Gemma Barson <gb10@sanger.ac.uk>
 *  Copyright (C) 2012 Genome Research Ltd
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
 * 	Richard Durbin (Sanger Institute, UK) rd@sanger.ac.uk, and
 *	Jean Thierry-Mieg (CRBM du CNRS, France) mieg@kaa.crbm.cnrs-mop.fr
 *
 * Description: Uses the smap system to find methods for various feature 
 *              types
 *              
 *-------------------------------------------------------------------
 */

#ifndef SMAPMETHODS_H
#define SMAPMETHODS_H

#include <wh/method.h>
#include <wh/smap.h>
#include <wh/dict.h>
#include <wh/bitset.h>
#include <wh/methodcache.h>				    
#include <gtk/gtk.h>


/* Callback functions for smap methods use this function definition. */
typedef void (*SmapCallback)(METHOD *meth, gpointer user_data);

BOOL sMapFindMethods(SMap *smap, KEY seqKey, KEY seqOrig,
		      int start, int stop, int length,
		      Array oldsegs, BOOL complement, MethodCache mcache,
		      Array dna,
		      KEYSET method_set, BOOL include_methods,
		      DICT *featDict, DICT *sources_dict, BOOL include_sources,
		      Array segs_inout,
		      Array seqinfo_inout, Array homolInfo_inout, Array featureinfo_inout,
		      BitSet homolBury_inout, BitSet homolFromTable_inout,
		      BOOL keep_selfhomols, BOOL keep_duplicate_introns,
                     STORE_HANDLE segsHandle, SmapCallback callback_func, gpointer callback_data);


#endif /* SMAPMETHODS_H */
