/*  File: graphAcedbInterface.c
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
 * Description: The graph package allows the registering of functions to
 *              'overload' some of it's functions. This is necessary for 
 *              ACEDB applications so that they can load users preferences
 *              about where windows should be placed etc.
 *              This file allows the graph package to be written without
 *              the need for #ifdef ACEDB macros within the graph library
 *              to change its behaviour.
 * Exported functions:
 * HISTORY:
 * Last edited: Nov 10 15:06 1999 (fw)
 * * Jan 28 14:17 1999 (fw): registering of mainActivity funtions now
 *              done in libfree.a, so it allows calls to mainActivity
 *              from anywhere in the code, but registering is only done
 *              in mainpick.c after the status box in the mainwindow exists
 * * Jan 25 16:04 1999 (edgrif): Remove register of getLogin name, not needed now.
 * Created: Wed Sep 16 11:58:55 1998 (edgrif)
 *-------------------------------------------------------------------
 */

#include <w2/graph_.h>


/* Strings used to search for various display types                          */
static GraphCharRoutine graphAcedbDisplayCreateFunc = NULL ;
static char *graphAcedbChronoName = NULL ;
static char *graphAcedbViewName = NULL ;
static char *graphAcedbFilqueryName = NULL ;


/* The printer menu that appears when you click on the print window.         */
static MENUOPT *graphAcedbPDMenu = NULL ;
static CharVoidRoutine graphAcedbStyleFunc = NULL ;


/* Postscript file printing.                                                 */
static char *graphAcedbSessionName = NULL ;
static VoidFILEStackIntRoutine graphAcedbGetFonts = NULL ;


/* BoxInfo routine to print extra acedb specific information.                */
static VoidACEOUTKEYRoutine graphAcedbClassPrintFunc = NULL ;

#ifndef ACEDB_GTK
/* graph to Xlib specific stuff.                                             */
static VoidIntRoutine graphAcedbXFontFunc = NULL ;
#endif

/* viewedit routines specific to acedb.                                      */
static VoidVIEWCOLCONTROLRoutine graphAcedbViewAnonFunc = NULL ;
static BOOLMENURoutine graphAcedbSaveViewFunc = NULL ;
static VoidMAPCONTROLVIEWRoutine graphAcedbSaveViewMsgFunc = NULL ;
static VoidMENUSPECRoutine graphAcedbViewMenuFunc = NULL ;


/* colcontrol routines specific to acedb.                                    */
static BOOLMAPCONTROLKEYRoutine graphAcedbSetViewFunc = NULL ;
static VoidKEYRoutine graphAcedbResetKeyFunc = NULL ;
static VoidCOLOBJRoutine graphAcedbResetSaveFunc = NULL ;
static VoidIntFREEOPT graphAcedbFreemenuFunc = NULL ;
static VoidOBJFUNCMAPCONTROL graphAcedbMapLocateFunc = NULL ;
static VoidOBJFUNCMAPCONTROL graphAcedbScaleFunc = NULL ;
static VoidOBJFUNCSPACERPRIV graphAcedbSpaceFunc = NULL ;
static VoidCharKEY graphAcedbAddMapFunc = NULL ;


/* Initialise the overloaded function pointers.                              */
/*                                                                           */
void initOverloadFuncs()
{

  /* No general overload functions currently...                              */

  return ;
}


/* Get/set the names for various displays, names are used by acedb code      */
/* to set up display types via the registered acedb display function.        */
/*                                                                           */
GraphCharRoutine setGraphAcedbDisplayCreate(GraphCharRoutine funcptr,
					    char *chrono, char *view, char *filquery)
{
  GraphCharRoutine old = graphAcedbDisplayCreateFunc ;
  graphAcedbDisplayCreateFunc = funcptr ;

  graphAcedbChronoName = chrono ;
  graphAcedbViewName = view ;
  graphAcedbFilqueryName = filquery ;

  return old ;
}

GraphCharRoutine getGraphAcedbDisplayCreate(void) { return graphAcedbDisplayCreateFunc ; }
char *getGraphAcedbChronoName(void) { return graphAcedbChronoName ; }
char *getGraphAcedbViewName(void) { return graphAcedbViewName ; }
char *getGraphAcedbFilqueryName(void) { return graphAcedbFilqueryName ; }


/* Set/get the graphprint stuff.                                             */
void setGraphAcedbPDMenu(MENUOPT *new_menu, CharVoidRoutine stylefunc)
{
  graphAcedbPDMenu = new_menu ;
  graphAcedbStyleFunc = stylefunc ;
  
  return ;
}
MENUOPT *getGraphAcedbPDMenu(void) { return graphAcedbPDMenu ; }
CharVoidRoutine getGraphAcedbStyle(void) { return graphAcedbStyleFunc ; }


/* Set/get the graphps stuff.                                                */
void setGraphAcedbPS(char *session_name, VoidFILEStackIntRoutine fontfunc)
{
  graphAcedbSessionName = session_name ;
  graphAcedbGetFonts = fontfunc ;
  
  return ;
}
char *getGraphAcedbSessionName(void) { return graphAcedbSessionName ; }
VoidFILEStackIntRoutine getGraphAcedbGetFonts(void) { return graphAcedbGetFonts ; }



/* Set/get boxInfo Class print routine.                                      */
/*                                                                           */
VoidACEOUTKEYRoutine setGraphAcedbClassPrint(VoidACEOUTKEYRoutine classprintfunc)
{
  VoidACEOUTKEYRoutine old = graphAcedbClassPrintFunc ;

  graphAcedbClassPrintFunc = classprintfunc ;

  return old ;
}
VoidACEOUTKEYRoutine getGraphAcedbClassPrint(void) { return graphAcedbClassPrintFunc ; }



#ifndef ACEDB_GTK
/* Set/get X font routine                                                    */
/*                                                                           */
VoidIntRoutine setGraphAcedbXFont(VoidIntRoutine xfontfunc)
{
  VoidIntRoutine old = graphAcedbXFontFunc ;

  graphAcedbXFontFunc = xfontfunc ;

  return old ;
}
VoidIntRoutine getGraphAcedbXFont(void) { return graphAcedbXFontFunc ; }
#endif


/* Set/Get routines for viewedit functions.                                  */
/*                                                                           */
void setGraphAcedbView(VoidVIEWCOLCONTROLRoutine anonfunc,
		       BOOLMENURoutine saveviewfunc,
		       VoidMAPCONTROLVIEWRoutine viewsavemsgfunc,
		       VoidMENUSPECRoutine viewmenufunc)
{
  graphAcedbViewAnonFunc = anonfunc ;
  graphAcedbSaveViewFunc = saveviewfunc ;
  graphAcedbSaveViewMsgFunc = viewsavemsgfunc ;
  graphAcedbViewMenuFunc = viewmenufunc ;

  return ;
}
VoidVIEWCOLCONTROLRoutine getGraphAcedbViewAnon(void) { return graphAcedbViewAnonFunc ; } 
BOOLMENURoutine getGraphAcedbSaveView(void) { return graphAcedbSaveViewFunc ; }
VoidMAPCONTROLVIEWRoutine getGraphAcedbSaveViewMsg(void)
                                                             { return graphAcedbSaveViewMsgFunc ; }

VoidMENUSPECRoutine getGraphAcedbViewMenu(void) { return graphAcedbViewMenuFunc ; }


/* Set/Get routines for colcontrol functions.                                */
/*                                                                           */
void setGraphAcedbColControl(BOOLMAPCONTROLKEYRoutine setviewfunc,
			     VoidKEYRoutine resetkeyfunc,
			     VoidCOLOBJRoutine resetsavefunc,
			     VoidIntFREEOPT freemenufunc,
			     VoidOBJFUNCMAPCONTROL maplocatefunc,
			     VoidOBJFUNCMAPCONTROL scalefunc,
			     VoidOBJFUNCSPACERPRIV spacefunc,
			     VoidCharKEY addmap)
{
  graphAcedbSetViewFunc = setviewfunc ;
  graphAcedbResetKeyFunc = resetkeyfunc ;
  graphAcedbResetSaveFunc = resetsavefunc ;
  graphAcedbFreemenuFunc = freemenufunc ;
  graphAcedbMapLocateFunc = maplocatefunc ;
  graphAcedbScaleFunc = scalefunc ;
  graphAcedbSpaceFunc = spacefunc ;
  graphAcedbAddMapFunc = addmap ;
  return ;
}
BOOLMAPCONTROLKEYRoutine getGraphAcedbGetSetView(void) { return graphAcedbSetViewFunc ; }
VoidKEYRoutine getGraphAcedbResetKey(void) { return graphAcedbResetKeyFunc ; }
VoidCOLOBJRoutine getGraphAcedbResetSave(void) { return graphAcedbResetSaveFunc ; }
VoidIntFREEOPT getGraphAcedbFreemenu(void) { return graphAcedbFreemenuFunc ; }
VoidOBJFUNCMAPCONTROL getGraphAcedbMapLocate(void) { return graphAcedbMapLocateFunc ; }
VoidOBJFUNCMAPCONTROL getGraphAcedbScale(void) { return graphAcedbScaleFunc ; }
VoidOBJFUNCSPACERPRIV getGraphAcedbSpace(void) { return graphAcedbSpaceFunc ; }
VoidCharKEY getGraphAcedbAddMap(void) { return graphAcedbAddMapFunc ; }
