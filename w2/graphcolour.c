/*  File: graphcolour.c
 *  Author: Simon Kelley (srk@sanger.ac.uk)
 *  Copyright (C) J Thierry-Mieg and R Durbin, 1991
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
 * This file is part of the ACEDB genome database package, written by
 * 	Richard Durbin (MRC LMB, UK) rd@mrc-lmb.cam.ac.uk, and
 *	Jean Thierry-Mieg (CRBM du CNRS, France) mieg@kaa.cnrs-mop.fr
 *
 * Description: maps graph package colours to RGB / grey triplets/
 *-------------------------------------------------------------------
 */

#include <colours.h>
#include <w2/graphcolour.h>


/* NB grey values are arranged so that (eg Red |= Green != Blue to make them
   distinguishable on greyscale display */

static int colorTable[4*NUM_TRUECOLORS]= {
255,	255,	255,   255, 	   /* WHITE           */
0,	0,	0,     0,	   /* BLACK           */
200,	200,	200,   178,	   /* LIGHTGRAY       */
100,	100,	100,   76, 	   /* DARKGRAY        */
255,	0,	0,     102,        /* RED             */
0,	255,	0,     128,        /* GREEN           */
0,	0,	255,   142,	   /* BLUE            */
255,	255,	0,     229,	   /* YELLOW          */
0,	255,	255,   178,	   /* CYAN            */
255,	0,	255,   153, 	   /* MAGENTA         */
255,	160,	160,   191,	   /* LIGHTRED        */
160,	255,	160,   204,	   /* LIGHTGREEN      */
160,	200,	255,   216,	   /* LIGHTBLUE       */
175,	0 ,	0,     38, 	   /* DARKRED         */
0,	175,	0,     51,	   /* DARKGREEN       */
0,	0,	175,   63,         /* DARKBLUE        */
255,	230,	210,   229,	   /* PALERED         */ 
210,	255,	210,   229, 	   /* PALEGREEN       */
210,	235,	255,   229,	   /* PALEBLUE        */
255,	255,	200,   242,	   /* PALEYELLOW      */
200,	255,	255,   242,	   /* PALECYAN        */
255,	200,	255,   242,	   /* PALEMAGENTA     */
160,	80,	0,     102,	   /* BROWN           */
255,	128,	0,     153,	   /* ORANGE          */
255,	220,	110,   204,        /* PALEORANGE      */
192,	0,	255,   127,	   /* PURPLE          */
200,	170,	255,   178,	   /* VIOLET          */
235,	215,	255,   229,	   /* PALEVIOLET      */
150,	150,	150,   140,	   /* GRAY            */
235,	235,	235,   216,	   /* PALEGRAY        */
255,	0,	128,   127,	   /* CERISE          */
86,	178,	222,   127	   /* MIDBLUE         */
} ;


void graphGetColourAsFloat(int colour, float *r, float *g, float *b, float *gr)
{
  if (r)
    *r = ((float)colorTable[4*colour])/255.0;
  
  if (g)
    *g = ((float)colorTable[4*colour+1])/255.0;

  if (b) 
    *b = ((float)colorTable[4*colour+2])/255.0;

  if (gr)
    *gr = ((float)colorTable[4*colour+3])/255.0;


}

void graphGetColourAsInt(int colour, int *r, int *g, int *b, int *gr)
{
  if (r)
    *r = colorTable[4*colour];
  
  if (g)
    *g = colorTable[4*colour+1];

  if (b)
    *b = colorTable[4*colour+2];
  
  if (gr)
    *gr = colorTable[4*colour+3];
}

