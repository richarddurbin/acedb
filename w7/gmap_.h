/*  File: gmap_.h
 *  Author: Ed Griffiths (edgrif@sanger.ac.uk)
 *  Copyright (c) 2008: Genome Research Ltd.
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
 * This file is part of the ZMap genome database package
 * originally written by:
 *
 * 	Ed Griffiths (Sanger Institute, UK) edgrif@sanger.ac.uk,
 *      Roy Storey (Sanger Institute, UK) rds@sanger.ac.uk
 *
 * Description: Private functions/structs etc that are shared between
 *              files in the GMap package.
 *              
 *              All private external function names should begin with "gmap"
 *              to distinguish them from public external functions which
 *              begin with "gMap".
 *
 * HISTORY:
 * Last edited: Jan  6 18:03 2011 (edgrif)
 * Created: Tue May 13 10:48:53 2008 (edgrif)
 * CVS info:   $Id: gmap_.h,v 1.3 2011-01-06 18:06:01 edgrif Exp $
 *-------------------------------------------------------------------
 */
#ifndef ACEDB_GMAP_PRIV_H
#define ACEDB_GMAP_PRIV_H

#include <wh/gmap.h>


BOOL gmapGeneIsPub(KEY gene_key, char *pub_name) ;
void gmapHighlightGene(int box) ;



#endif /* ACEDB_GMAP_PRIV_H */
