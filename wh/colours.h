/*  File: colours.h
 *  Author: Fred Wobus (fw@sanger.ac.uk)
 *  Copyright (C) J Thierry-Mieg and R Durbin, 1998
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
 *	Jean Thierry-Mieg (CRBM du CNRS, France) mieg@crbm.cnrs-mop.fr
 *
 * Description: enum for colour definitions
 *              they should be avilable for even for non-graphical
 *              programs that don't include graph.h
 * Exported functions:
 *              enum Colour {..};
 * HISTORY:
 * Last edited: Jun  9 15:10 1999 (edgrif)
 * Created: Thu Nov 19 15:49:30 1998 (fw)
 * CVS info:   $Id: colours.h,v 1.7 2000-04-07 15:44:53 srk Exp $
 *-------------------------------------------------------------------
 */

#ifndef _COLOURS_H
#define _COLOURS_H


/* the include sys/stream.h already uses TRANSPARENT, but
 * that flag isn't used by user code, so we undef it as a temporary
 * measure. To solve these clashes, all colour names need a prefix, 
 * such as COL_WHITE, COL_GREEN etc.., and the enum should be
 * preferably be a typedef as well. */

#undef TRANSPARENT


/* These colors match those declared in systags, they must appear in the same order */	
/* If you use more than 256 colours, code WILL break (see for instance the   */
/* 'priority/colour' packing code in griddisp.c). This should not be a       */
/* problem because that's a lot of colours and these colours are NOT used    */
/* for images.                                                               */
/*                                                                           */
enum Colour    {WHITE, BLACK, LIGHTGRAY, DARKGRAY,
                RED, GREEN, BLUE,
                YELLOW, CYAN, MAGENTA,
		LIGHTRED, LIGHTGREEN, LIGHTBLUE,
		DARKRED, DARKGREEN, DARKBLUE,
		PALERED, PALEGREEN, PALEBLUE,
		PALEYELLOW, PALECYAN, PALEMAGENTA,
		BROWN, ORANGE, PALEORANGE,
		PURPLE, VIOLET, PALEVIOLET,
		GRAY, PALEGRAY,
		CERISE, MIDBLUE,
		NUM_TRUECOLORS,
                TRANSPARENT,	/* pseudocolour only for background */
		FORECOLOR,	/* pseudocolor to force box->fcol after graphColor() */
		BACKCOLOR	/* pseudocolor to force box->bcol after graphColor() */
               } ;


#endif /* _COLOURS_H */



