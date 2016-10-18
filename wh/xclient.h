/*  File: sxclient.h
 *  Author: Ed Griffiths (edgrif@sanger.ac.uk)
 *  Copyright (c) J Thierry-Mieg and R Durbin, 2000
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
 * Description: Interface to code to make xace into a client/server
 *              version.
 *              
 * HISTORY:
 * Last edited: Apr 27 16:21 2000 (edgrif)
 * Created: Wed Apr 26 13:31:22 2000 (edgrif)
 * CVS info:   $Id: xclient.h,v 1.5 2000-04-27 17:12:37 edgrif Exp $
 *-------------------------------------------------------------------
 */
#ifndef DEF_XCLIENT_H
#define DEF_XCLIENT_H



/* Must be called by client main after sessionInit(), sets callbacks so that */
/* xaceclient will call the server instead of looking in its local database. */
void xClientInit (void) ;

/* Close connection to the server.                                           */
void xClientClose (void) ;


#endif /* DEF_XCLIENT_H */
