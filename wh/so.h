/*  File: so.h
 *  Author: Ed Griffiths (edgrif@sanger.ac.uk)
 *  Copyright (c) Sanger Institute, 2011
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
 * This file is part of the ACEDB genome database package, originated by
 * 	Richard Durbin (Sanger Centre, UK) rd@sanger.ac.uk, and
 *	Jean Thierry-Mieg (CRBM du CNRS, France) mieg@kaa.crbm.cnrs-mop.fr
 *
 * Description: defines/functions to support SO usage in acedb.
 *              (see http://www.sequenceontology.org/)
 *              
 * HISTORY:
 * Last edited: Sep 29 11:36 2011 (edgrif)
 * Created: Wed Jan  9 10:08:20 2008 (edgrif)
 * CVS info:   $Id: so.h,v 1.1 2012-10-30 10:30:44 edgrif Exp $
 *-------------------------------------------------------------------
 */
#ifndef ACEDB_SO_H
#define ACEDB_SO_H



/* Keywords used by the code in ?SO class in models.wrm. */
#define SO_CLASS        "SO_term"
#define SO_NAME         "SO_name"


/* Keywords used by code in ?Ace2SO class in models.wrm. */
#define ACE2SO_CLASS          "Ace2SO"
#define ACE2SO_SUBPART        "SO_subparts"
#define ACE2SO_SUBPART_EXON   "Exons"
#define ACE2SO_SUBPART_INTRON "Introns"
#define ACE2SO_SUBPART_CODING "Coding_region"
#define ACE2SO_SUBPART_TAGS   "SO_subparts_tags"
#define ACE2SO_PARENT         "SO_parent_obj_tag"
#define ACE2SO_CHILDREN       "SO_children"


/* Keywords used by code in ?Method class in models.wrm. */
#define METHOD_GFF_NO_SPAN    "GFF_no_span"
#define METHOD_GFF_SO         "GFF_SO"



#endif /* ACEDB_SO_H */


