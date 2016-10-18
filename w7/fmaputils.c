/*  File: fmap_.h
 *  Author: Ed Griffiths (edgrif@sanger.ac.uk)
 *  Copyright (C) J Thierry-Mieg and R Durbin, 1998
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
 *	Jean Thierry-Mieg (CRBM du CNRS, France) mieg@crbm.cnrs-mop.fr
 *
 * Description:
 *      A set of utility functions for the fmap code.
 *
 * Exported functions: see fmap_.h
 *              
 * HISTORY:
 * Last edited: Dec 24 11:53 2010 (edgrif)
 * Created: Thu Jul 16 09:29:49 1998 (edgrif)
 * CVS info:   $Id: fmaputils.c,v 1.1 2012-10-30 10:32:41 edgrif Exp $
 *-------------------------------------------------------------------
 */

#include <glib.h>

#include <dict.h>
#include <w7/fmap_.h>



/* Noddy "cover" functions encapulating what we want to pass to dict package,
 * return values are as per dict calls. */
BOOL fMapAddMappedDict(DICT *dict, KEY obj)
{
  BOOL result = FALSE ;

  result = dictAdd(dict, nameWithClassDecorate(obj), NULL) ;

  return result ;
}


BOOL fMapFindMappedDict(DICT *dict, KEY obj)
{
  BOOL result = FALSE ;

  result = dictFind(dict, nameWithClassDecorate(obj), NULL) ;

  return result ;
}

