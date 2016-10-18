/*  File: das.h
 *  Author: Ed Griffiths (edgrif@sanger.ac.uk)
 *  Copyright (c) J Thierry-Mieg and R Durbin, 2003
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
 * Description: Functions to support the DAS protocol.
 *              (Currently DAS version 1 only is supported.)
 * HISTORY:
 * Last edited: Feb  7 12:19 2003 (edgrif)
 * Created: Fri Feb  7 12:13:27 2003 (edgrif)
 * CVS info:   $Id: das.h,v 1.1 2003-02-07 18:10:08 edgrif Exp $
 *-------------------------------------------------------------------
 */
#ifndef DEF_DAS_H
#define DEF_DAS_H




/* Dumping routines, they each output one of the DAS data types, e.g. dasGFF outputs
 * feature data in the DAS "GFF" format. */
void dasGFF(ACEIN command_in, ACEOUT result_out, FeatureMap look) ;
void dasDNA (ACEIN command_in, ACEOUT result_out, FeatureMap look) ;




#endif /*  !defined DEF_DAS_H */
