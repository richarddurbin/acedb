/*  File: plot.h
 *  Author: mieg
 *  Copyright (c) J Thierry-Mieg and R Durbin, 1999
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
 *	Jean Thierry-Mieg (CRBM du CNRS, France) mieg@kaa.crbm.cnrs-mop.fr
 *
 * Description: header for plot.c and plot2d.c
 * Exported functions:
 * HISTORY:
 * Last edited: Aug 26 17:57 1999 (fw)
 * Created: Thu Aug 26 17:56:37 1999 (fw)
 * CVS info:   $Id: plot.h,v 1.7 1999-09-01 11:15:43 fw Exp $
 *-------------------------------------------------------------------
 */

#include "regular.h"

void plotHisto (char *title, Array a) ;
/* mhmp 10.03.99 float xmin*/
void plotShiftedHisto (char *title, char *subtitle, Array a, float xmin, float xmax, float scale, int xfac) ;
void plotHistoRemove (void) ;
typedef struct { KEY k ; float x, y ;} POINT2D ;
void plot2d (char *title, char *subtitleX, char *subtitleY, Array a) ;
