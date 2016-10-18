/*  File: zmaputils.h
 *  Author: Gemma Barson <gb10@sanger.ac.uk>
 *  Copyright (C) 2011 Genome Research Ltd
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
 * Description: Generic utilities used by the zmap control code
 *-------------------------------------------------------------------
 */

#ifndef _ZMAP_UTILS_H_
#define _ZMAP_UTILS_H_

#include <gtk/gtk.h>
#include <wh/fmap.h>


/* zmap error domain */
#define XACE_ZMAP_ERROR g_quark_from_string("ZMap error")


/* Debug logging macros. #define DEBUG to enable debug output. */
#ifdef DEBUG
#define DEBUG_OUT(format, args...) debugLogLevel(0); printf(format, ##args);
#else
#define DEBUG_OUT(format, args...)
#endif

#ifdef DEBUG
#define DEBUG_ENTER(format, args...) debugLogLevel(1); printf("-->"); printf(format, ##args); printf("\n");
#else
#define DEBUG_ENTER(format, args...)
#endif

#ifdef DEBUG
#define DEBUG_EXIT(format, args...) debugLogLevel(-1); printf("<--"); printf(format, ##args); printf("\n");
#else
#define DEBUG_EXIT(format, args...)
#endif


#ifdef DEBUG
void		      debugLogLevel(const int increaseAmt);
#endif

char**               convertListToArgv(GSList *list);
char*                createTempFile(const char *file_desc, GError **error);
char*                createTempDir(const char *dir_desc, GError **error);
BOOL                 setStartStop(KEY seq, int *start_inout, int *stop_inout, KEY *key_out, BOOL extend, FeatureMap look);
void                 zMapCentre(FeatureMap look, KEY key, KEY from);
FeatureMap           zMapCreateLook(KEY seq, int start, int stop, BOOL reuse, BOOL extend, BOOL noclip, FmapDisplayData *display_data);
void                 zMapInitialise (void);
void                 zMapRevComplement(FeatureMap look);
void                 propagatePrefixedError(GError **dest, GError *src, char *formatStr, ...);
void                 propagateError(GError **dest, GError *src);



#endif /* _ZMAP_UTILS_H_ */
