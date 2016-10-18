/*  File: acedbgraph.h
 *  Author: Ed Griffiths (edgrif@sanger.ac.uk)
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
 *	Jean Thierry-Mieg (CRBM du CNRS, France) mieg@kaa.crbm.cnrs-mop.fr
 *
 * Description: This file describes the interface between a graphical
 *              ace application and the ace code responsible for
 *              initialising that interface. It hides away the grubby
 *              bits in setting up this interface.
 *              Currently there is not much in the interface but this
 *              will probably change as interactions between the ace
 *              code and the graph code change.
 * Exported functions:
 * HISTORY:
 * Last edited: Jun 28 10:44 2001 (edgrif)
 * Created: Tue Oct 13 11:55:47 1998 (edgrif)
 *-------------------------------------------------------------------
 */
#ifndef DEF_ACEDBGRAPH_H
#define DEF_ACEDBGRAPH_H


/* Functions to initialise the acedb <-> graph interface.                    */
/*                                                                           */


/* This function combines graphInit and acedbGraphInit to provide a single   */
/* initialisation function for acedb applications using the graph package.   */
void acedbAppGraphInit(int *argcptr, char **argv) ;

/* For those requiring finer control they can use graphInit followed by a    */
/* call to this function.                                                    */
void acedbGraphInit(void) ;


/* Control the style of messaging for informational/error messages: should   */
/* they be in individual popups or a single scrolling window. Set by user    */
/* via preferences mechanism.                                                */
void acedbInitMessaging(void) ;
void acedbSwitchMessaging(BOOL msg_list) ;



#endif /* DEF_ACEDBGRAPH_H */
