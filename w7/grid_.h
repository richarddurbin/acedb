/*  File: grid_.h
 *  Author: Ed Griffiths (edgrif@sanger.ac.uk)
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
 * Description: Private header for grid display.
 * HISTORY:
 * Last edited: Jul 12 15:39 2001 (edgrif)
 * Created: Tue Jun  8 13:48:11 1999 (edgrif)
 * CVS info:   $Id: grid_.h,v 1.4 2001-07-16 17:32:02 edgrif Exp $
 *-------------------------------------------------------------------
 */
#ifndef DEF_GRID_PRIV_H
#define DEF_GRID_PRIV_H


/* Use this prefix in all griddisp error messages.                           */
#define GRID_PREFIX "Grid -  "


/* Priority, colour and surround colour are packed into an unsigned integer, */
/* this only works because integers are just about always 32 bits and we     */
/* need no more than 256 colours which are coded as enums starting at 0.     */
/* Hence the compiler warning below....                                      */
/*                                                                           */
/* Format:         |  unused  | priority |  colour  | scolour  |             */
/*                 |    0     |    1     |    2     |    3     |             */
/*                                                                           */
#if (UINT_MAX < 4294967295U)

#error "Compilation of griddisp.c has failed because the code relies on unsigned integers being at least 32 bits."

#else

#define GRID_PACK(PRIORITY, COLOUR, SCOLOUR) \
(((PRIORITY + 128) << 16) + (COLOUR << 8) + SCOLOUR)

#define GRID_PRIORITY(PACKED_INT) \
(((PACKED_INT) >> 16) - 128)

#define GRID_COLOUR(PACKED_INT) \
(((PACKED_INT) >> 8) & 0xff)

#define GRID_SCOLOUR(PACKED_INT) \
((PACKED_INT) & 0xff)


#endif

typedef struct _GridDimensionStruct
{
  float box_width ;
  float box_height ;
  float horiz_space ;
  float vert_space ;
} GridDimensionStruct, *GridDimension ;


#endif /* DEF_GRID_PRIV_H */
