/*  Last edited: May  4 15:42 1999 (edgrif) */
/*
 * -------------------------------------------------------------------
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * -------------------------------------------------------------------
 *	 File: graphimage.c
 *  Author: Cyrus Harmon (sly@fly2.berkeley.edu)
 *  Copyright (C) 1994
 *-------------------------------------------------------------------
*/

/* $Id: graphimage.c,v 1.6 2002-02-22 18:13:33 srk Exp $ */

#include "regular.h"
#include <w2/graph_.h>					    /* defines externals within graph package */
#include "graphimage.h"

#if defined(MACINTOSH)
#include "graphpict.h"
#endif


/********* GraphImage Routines CLH 3/29/94 ***/

/* reads from file */
GraphImagePtr graphImageRead (char *filename)
{
	GraphImagePtr	theGImage;
	
	theGImage = (GraphImagePtr)messalloc ( sizeof	( *theGImage ));
	
	if ( theGImage != nil )
	{
		theGImage->imagePtr = nil;
		theGImage->imageType = Colour32;
		theGImage->width = 0;
		theGImage->height = 0;
	}		

#if defined(macintosh)
 
 graphPICTOpen	(	theGImage, filename	);

#endif

	return theGImage;
}

GraphImagePtr graphImageCreate (int type, int width, int height)
{
	GraphImagePtr	theGImage;
	
	theGImage = (GraphImagePtr)messalloc ( sizeof	( *theGImage ));
	
	if ( theGImage != nil )
	{
		theGImage->imagePtr = nil;
		theGImage->imageType = Colour32;
		theGImage->fileType = kNoFile;
		theGImage->width = 0;
		theGImage->height = 0;
	}		
	
	return theGImage;
}

void graphImageDestroy (GraphImagePtr gim)
{

	if ( gim != nil )
	{

#if defined(macintosh) 

		graphPICTClose	(	gim	);

#endif

		messfree ( gim );
	}
	return;

}

/* displays it */
void graphImageDraw (GraphImagePtr gim, float x0, float y0, float x1, float y1)
{
  gim->top = y0;
  gim->left = x0;
  gim->bottom = y1;
  gim->right = x1;
  push (gStk, IMAGE, int) ;
  push (gStk, gim, GraphImagePtr) ;
  return;
}

/* Should this be a void and change gim or should it copy gim and return the new one??? */
GraphImagePtr graphImageResize (GraphImagePtr gim, float width, float height)
{
	return	nil;
}

/* true color space - lineLength in pixels */
BOOL graphImageGetData (GraphImagePtr gim, unsigned char *data, int type,
                        int x, int y, int w, int h, int lineLength)
{
	return	false;
}

BOOL graphImageSetData (GraphImagePtr gim,  unsigned char *data, int type,
                        int x, int y, int w, int h, int lineLength)
{
	return	false;
}

/* contrast, brightness - can fail, e.g. for Colour32 for now! */

BOOL graphImageRamp (GraphImagePtr gim, int min, int max)
{
	return	false;
}


