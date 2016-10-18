/*  File: graphAcedbInterface.h
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
 * Description: Defines functions to set up the interface between the
 *              graph package and acedb which allows acedb to alter
 *              the behaviour of the graph package.
 *              
 *              NOTE, this is not a general interface, it works for
 *              acedb but that is no guarantee it will work for
 *              anything else...you have been warned...
 *              
 * Exported functions:
 * HISTORY:
 * Last edited: Mar  8 09:20 2000 (edgrif)
 * * Feb  8 11:02 1999 (edgrif): Remove obsolete func. prototype for WCOMM stuff.
 * * Jan 25 15:58 1999 (edgrif): Removed need for getLogin.
 * * Oct  8 10:01 1998 (edgrif): ? (no change ?)
 * Created: Thu Sep 24 13:07:45 1998 (edgrif)
 *-------------------------------------------------------------------
 */


#ifndef DEF_GRAPHACEDBINTERFACE_H
#define DEF_GRAPHACEDBINTERFACE_H

#include <wh/regular.h>
#include <wh/aceiotypes.h>
#include <wh/graph.h>
#include <wh/colcontrol_.h>
#include <wh/menu.h>



/********************  Graph  <-->  Acedb interface  *************************/
/* Do not use this interface for anything other than setting up graph for    */
/* use with Acedb. It is here to enable the two to cooperate, not for others */
/* to make use of...you have been warned.                                    */
/*                                                                           */


/*---------------------------------------------------------------------------*/
/* Intialisation of the  Graph  <-->  Acedb  interface                       */
/*                                                                           */
							    /* Function types:                   */
typedef void  (*VoidCharRoutine)(char*) ;		    /* void funcname(char*) */
typedef Graph (*GraphCharRoutine)(char*) ;		    /* Graph funcname(char*)*/
typedef char  (*CharVoidRoutine)(void) ;		    /* char funcname(void) */
typedef void  (*VoidFILEStackIntRoutine)(FILE**, Stack, int*) ;
							    /* void funcname(FILE**, Stack, int*) */
typedef void  (*VoidFILEKEYRoutine)(FILE *, KEY) ;	    /* void funcname(FILE*, KEY) */
typedef void  (*VoidACEOUTKEYRoutine)(ACEOUT, KEY) ;	    /* void funcname(FILE*, KEY) */
typedef void  (*VoidIntRoutine)(int *level) ;		    /* void funcname(int*) */
typedef void  (*VoidVoidRoutine)(void) ;		    /* void funcname(void) */
typedef void  (*VoidVIEWCOLCONTROLRoutine)(VIEWWINDOW, COLCONTROL) ;
							    /* void funcname(VIEWWINDOW, COLCONTROL) */
typedef BOOL  (*BOOLMENURoutine)(MENU) ;

typedef BOOL  (*BOOLMAPCONTROLKEYRoutine)(MAPCONTROL, KEY) ;

typedef void  (*VoidMAPCONTROLVIEWRoutine)(MAPCONTROL, VIEWWINDOW) ;

typedef void  (*VoidMENUSPECRoutine)(MENUSPEC**) ;

typedef void  (*VoidKEYRoutine)(KEY*) ;

typedef void  (*VoidCOLINSTANCEOBJ)(COLINSTANCE, OBJ) ;	    /* Note the linked typedef. */
typedef void  (*VoidCOLOBJRoutine)(VoidCOLINSTANCEOBJ*) ;

typedef void  (*VoidIntFREEOPT)(int, FREEOPT*) ;

typedef void  (*VoidOBJFUNCMAPCONTROL)(OBJ, VoidCOLINSTANCEOBJ*, MAPCONTROL) ;

typedef void  (*VoidOBJFUNCSPACERPRIV)(OBJ, VoidCOLINSTANCEOBJ*, SPACERPRIV) ;

typedef void  (*VoidCharKEY)(char*, KEY*) ;


GraphCharRoutine setGraphAcedbDisplayCreate (GraphCharRoutine displayfunc,
					     char *chrono, char *view, char *filquery) ;

void setGraphAcedbPDMenu (MENUOPT *new_menu, CharVoidRoutine stylefunc) ;
void pdExecute (void) ;					    /* Not good, this is really internal
							       to the graph package in graphprint.c */


void setGraphAcedbPS (char *session_name, VoidFILEStackIntRoutine fontfunc) ;


VoidACEOUTKEYRoutine setGraphAcedbClassPrint (VoidACEOUTKEYRoutine classprintfunc) ;

VoidIntRoutine setGraphAcedbXFont (VoidIntRoutine xfontfunc) ;

void setGraphAcedbView (VoidVIEWCOLCONTROLRoutine anonfunc,
			BOOLMENURoutine saveviewfunc,
			VoidMAPCONTROLVIEWRoutine viewsavemsgfunc,
			VoidMENUSPECRoutine viewmenufunc) ;


void setGraphAcedbColControl (BOOLMAPCONTROLKEYRoutine setviewfunc,
			      VoidKEYRoutine resetkeyfunc,
			      VoidCOLOBJRoutine resetsavefunc,
			      VoidIntFREEOPT freemenufunc,
			      VoidOBJFUNCMAPCONTROL maplocatefunc,
			      VoidOBJFUNCMAPCONTROL scalefunc,
			      VoidOBJFUNCSPACERPRIV spacefunc,
			      VoidCharKEY addmap) ;


/*---------------------------------------------------------------------------*/
/* Graph routines specific to the   Graph  <-->  Acedb  interface            */
/*                                                                           */

void controlMakeColumns(MAPCONTROL map, COLDEFAULT colInit, KEY view, BOOL) ;
int controlGetColour(OBJ obj);
void controlSetColour(OBJ obj, int colour);



/*****************************************************************************/
#endif /* DEF_GRAPHACEDBINTERFACE_H */
