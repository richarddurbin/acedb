/*  File: maputils.c
 *  Author: Ed Griffiths (edgrif@sanger.ac.uk)
 *  Copyright (C) J Thierry-Mieg and R Durbin, 1992
 *-------------------------------------------------------------------
 * ZMap is free software; you can redistribute it and/or
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
 * 	Richard Durbin (MRC LMB, UK) rd@mrc-lmb.cam.ac.uk, and
 *	Jean Thierry-Mieg (CRBM du CNRS, France) mieg@kaa.cnrs-mop.fr
 *
 * Description: Utilities for fmap, gmap, pmap etc.
 *
 * Exported functions: See wh/maputils.h
 *              
 * HISTORY:
 * Last edited: Jan  6 17:21 2011 (edgrif)
 * Created: Thu Jan  6 17:01:58 2011 (edgrif)
 * CVS info:   $Id: maputils.c,v 1.1 2011-01-06 17:42:03 edgrif Exp $
 *-------------------------------------------------------------------
 */


#include <wh/acedb.h>
#include <wh/bindex.h>




/* Looks for the public_name key in the object represented by gene_key, returns
 * gene_key if the public_name key cannot be found. */
KEY geneGetPubKey(KEY gene_key)
{
  KEY pub_key ;
  KEY public_name_tag = str2tag("Public_name") ;

  pub_key = gene_key ;
  bIndexGetKey(gene_key, public_name_tag, &pub_key) ;	    /* N.B. pub_key is not changed if
							       public_name_tag cannot be found.*/
  return pub_key ;
}


