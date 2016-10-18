/*  File: graphimage.h
 *  Author: Fred Wobus (fw@sanger.ac.uk)
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
 * Description: GraphImage Code CLH
 *              As described by RD, CLH, SEL, FEH 3/29/94
 * Exported functions: 
 * HISTORY:
 * Last edited: Aug 26 17:22 1999 (fw)
 * Created: Thu Aug 26 17:21:59 1999 (fw)
 * CVS info:   $Id: graphimage.h,v 1.3 1999-09-01 11:10:34 fw Exp $
 *-------------------------------------------------------------------
 */

#ifndef ACEDB_GRAPHIMAGE_H
#define ACEDB_GRAPHIMAGE_H

enum GraphImageType { Grey8, Colour8, Colour32 } ;
enum GraphImageFileType { kNoFile, kGraphPICT } ;

struct GraphImageStruct {
	void*		imagePtr ;
	int		imageType ;
	int		fileType ;
	int		width ;
	int 		height ;
	float		top;
	float		left;
	float		bottom;
	float		right;
	} ;
typedef struct GraphImageStruct GraphImageStruct,  *GraphImagePtr;

GraphImageStruct *graphImageRead (char *filename) ;		/* reads from file */
GraphImageStruct *graphImageCreate (int type, int width, int height) ;
void graphImageDestroy (GraphImageStruct *gim) ;

/*	Pushes the image on to the drawing stack	*/
void graphImageDraw (GraphImageStruct *gim, float x0, float y0, float x1, float y1);

GraphImageStruct *graphImageResize (GraphImageStruct *gim, float width, float height) ;

BOOL graphImageGetData (GraphImageStruct *gim, unsigned char *data, int type,
                        int x, int y, int w, int h, int lineLength) ;
				/* true color space - lineLength in pixels */
BOOL graphImageSetData (GraphImageStruct *gim,  unsigned char *data, int type,
                        int x, int y, int w, int h, int lineLength) ;

BOOL graphImageRamp (GraphImageStruct *gim, int min, int max) ;
				/* contrast, brightness - can fail, e.g. for Colour32 for now! */


#endif /* !ACEDB_GRAPHIMAGE_H */
