/*  File: qbe.h
 *  Author: Ed Griffiths (edgrif@sanger.ac.uk)
 *  Copyright (c) J Thierry-Mieg and R Durbin, 2001
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
 * Description: General query construction code interface.
 * HISTORY:
 * Last edited: Sep 27 14:53 2001 (edgrif)
 * Created: Thu Sep 27 14:48:28 2001 (edgrif)
 * CVS info:   $Id: qbe.h,v 1.1 2001-09-27 13:53:43 edgrif Exp $
 *-------------------------------------------------------------------
 */
#ifndef DEF_QBE_H
#define DEF_QBE_H

void qbeCreate(void) ;
void qbeCreateFromKeySet(KEYSET ksAlpha, int i0) ;


#endif /* DEF_QBE_H */
