/*  dendrogram.h : header file for dendrogram.c
 *  Author: Richard Bruskiewich	(rbrusk@octogene.medgen.ubc.ca)
 *  Copyright (C) R. Bruskiewich, J Thierry-Mieg and R Durbin, 1996
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
 * 	Richard Durbin (MRC LMB, UK) rd@mrc-lmba.cam.ac.uk, and
 *	Jean Thierry-Mieg (CRBM du CNRS, France) mieg@kaa.cnrs-mop.fr
 *
 * Description: Defines the class model behaviors for new ?Tree class
 *		which can draw dendrograms of phylogenetic or taxonomic trees.
 *
 * Exported:
 *	void readTreeFile(int treeType) // reads in a New Hampshire formatted tree
 *	void readTaxonomicTree(void) ; // Specialized versions of the above...
 *	void readDNATree(void);
 *	void readProteinTree(void) ;
 *
 * HISTORY:
 * Last edited: Mar 15 11:24 2001 (edgrif)
 * Created: Jun 9 15:22 1998 (rbrusk)
 * CVS info:   $Id: dendrogram.h,v 1.5 2001-03-15 13:54:47 edgrif Exp $
 *-------------------------------------------------------------------
 */
#ifndef ACEDB_DENDROGRAM_H
#define ACEDB_DENDROGRAM_H

/* Some possible treeType -- one can add to these, 
   but one should modify the dendrogram.c code accordingly */

#define NO_OF_TREE_TYPES 5

#define UNKNOWN_TREE     0x00
#define GENERIC_TREE     0x01
#define TAXONOMIC_TREE   0x02
#define DNA_TREE         0x03
#define PROTEIN_TREE     0x04
#define CELL_LINEAGE     0x10

KEY  readTreeFile(int treeType) ; /* treeType is one of the above */
void readTaxonomicTree(void) ;   
void readDNATree(void);   
void readProteinTree(void) ;   
BOOL dendrogramDisplay (KEY key, KEY from, BOOL isOldGraph, void *unused) ;

#endif /* !ACEDB_DENDROGRAM_H */
