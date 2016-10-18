/*  File: acedbgraph.c
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
 * Description: Initialises the graph acedb interface which allows acedb
 *              to alter the graph package behaviour for certain required
 *              acedb functions (window position/printer preference etc).
 * Exported functions:
 * HISTORY:
 * Last edited: Jan  8 10:53 2003 (edgrif)
 * * Jan 28 14:15 1999 (fw): removed register for mainActivity, 
 *              now done by mainpick.c
 *               (which draws the status box, so that's the right place)
 * * Jan 25 16:00 1999 (edgrif): Removed getLogin(), now done by graph.
 * Created: Thu Sep 24 13:55:44 1998 (edgrif)
 *-------------------------------------------------------------------
 */

#include <wh/acedb.h>
#include <wh/aceio.h>
#include <wh/acedbgraph.h>				    /* ace -> graph header. */
#include <wh/graphAcedbInterface.h>			    /* graph -> ace header. */
#include <wh/pref.h>					    /* for message prefs. */
#include <wh/display.h>					    /* For displayCreate */
#include <wh/colcontrol_.h>				    /* Sadly we need this for the
							       VIEWWINDOW structure. */
#include <wh/lex.h>					    /* for lexaddkey */
#include <wh/session.h>					    /* write access calls etc. */
#include <whooks/sysclass.h>				    /* for _VMainClasses */
#include <whooks/systags.h>				    /* for _Float */
#include <wh/bs.h>					    /* For lots of bs calls. */
#include <wh/main.h>					    /* for checkWriteAccess() */

#ifdef ACEDB_GTK
#include <wh/gex.h>
#endif

#include <wh/dbpath.h>


/* This MUST be static so it persists between calls.                         */
/* This array has been moved from graphprint.c with the following comment    */
/* explaining why the array had to be changed from the default one in        */
/* graphprint.c                                                              */
/*                                                                           */
/* in ACEDB people got confused with OK  being the top 
     of the menu in that window, as  every other ACEDB 
     window has Quit there that does nothing and people 
     ended up with unwanted printouts */
#ifndef __CYGWIN__
/* windows doesn't have this. */
static MENUOPT pdMenu[] =
{
  {graphDestroy, "Quit"},
  {pdExecute, " OK "},
  {0, 0}
} ;
#endif

static char colorDefault (void) ;
static void getFonts(FILE **fil, Stack fontNameStack, int *nFonts) ;
static void classPrint(ACEOUT fo, KEY key) ;
#ifndef ACEDB_GTK
static void GetXlibFonts(int *level) ;
#endif

static void setAnon(VIEWWINDOW view, COLCONTROL control) ;
static BOOL setSaveView(MENU menu) ;
static void setSaveViewMsg(MAPCONTROL map, VIEWWINDOW view) ;
static void setViewMenu(MENUSPEC **menu) ;


/* Document routines moved from viewedit                                     */


/* Interface routines for colcontrol.c                                       */
static void resetkey(KEY *key) ;
static void resetsave(VoidCOLINSTANCEOBJ *savefunc) ;
static void menufree(int box, FREEOPT *viewmenu) ;
static void maplocate(OBJ init, VoidCOLINSTANCEOBJ *savefunc, MAPCONTROL map) ;
static void scale(OBJ init, VoidCOLINSTANCEOBJ *savefunc, MAPCONTROL map) ;
static void space(OBJ init, VoidCOLINSTANCEOBJ *spacefunc, SPACERPRIV private) ;
static void addmap(char *name, KEY *key) ;

/* These routines were in colcontrol.c and have been shifted here.           */
static KEY controlSaveView(MAPCONTROL map);
static BOOL controlSetView(MAPCONTROL map, KEY view);
static Array addToMenu(Array menu, STORE_HANDLE handle, KEY view) ;
static void viewMenuFunc(KEY key, int box) ;
static void mapLocatorSave(COLINSTANCE instance, OBJ obj) ;
static void mapScaleSave(COLINSTANCE instance, OBJ obj) ;
static void spacerSave(COLINSTANCE instance, OBJ obj) ;

static void prefcb(BOOL msg_window) ;


#ifndef ACEDB_GTK
/* srk :- I call the components of graph package initialisation from
   main() directly - I need more control over order than this gives me. */

/* This routine initialises the graph package and the graph/acedb interface, */
/* it encapsulates the two so that they occur in the right order and with    */
/* the correct parameters. (In fact the order is not important at the moment */
/* but this may change later).                                               */
void acedbAppGraphInit(int *argcptr, char **argv)
  {

  /* Initialise the graphics package and register any overloaded functions.  */
  graphInit (argcptr, argv) ;	/* before calls to messcrash() */
							    /* Is this messcrash comment right ? */

  /* Initialise the acedb/graph package interface so that graph can do acedb */
  /* specific displays, this must follow immediately after graphInit().      */
  acedbGraphInit() ;

  return ;
  }
#endif /* ACEDB_GTK */

/* This routine is called by acedb applications which use the graph package  */
/* to set up the graph package so that it will do acedb specific things.     */
/*                                                                           */
void acedbGraphInit(void)
  {
  /* Register stuff for creating various types of graph but in acedb style.  */
  setGraphAcedbDisplayCreate(displayCreate, "DtChrono", "DtColControl", "DtFile_Chooser") ;

#ifndef __CYGWIN__
  /* Register stuff for graphprint                                           */
  setGraphAcedbPDMenu(&pdMenu[0], colorDefault) ;
#endif

  /* Register stuff for graphps                                              */
  setGraphAcedbPS(thisSession.name, getFonts) ;


  /* Set style of graphSelect window.                                        */
  graphSetSelectDisplayMode(GRAPH_SELECT_PLAIN) ;


  /* Register boxInfo classname print routine.                               */
  setGraphAcedbClassPrint(classPrint) ;

#ifndef ACEDB_GTK
  setGraphAcedbXFont(GetXlibFonts) ;
#endif

  /* Register viewedit routines.                                             */
  setGraphAcedbView(setAnon, setSaveView, setSaveViewMsg, setViewMenu) ;

  /* Register colcontrol routines.                                           */
  setGraphAcedbColControl(controlSetView, resetkey, resetsave,
			  menufree, maplocate, scale, space, addmap) ;
  return ;
  }


/* This routine initialises the messaging system for informational/error     */
/* messages to either be displayed in individual popup windows or in a       */
/* single scrolled list window.                                              */
/* Note that this routine interacts with the acedb "user preferences"        */
/* routines so that these are kept out of gex/graph which should be          */
/* independent of acedb stuff.                                               */
/* MUST be called AFTER sessionInit() which initialises the prefsubs stuff.  */
/*                                                                           */
void acedbInitMessaging(void)
{
  BOOL use_msg_list ;
  int list_length ;

  /* Set callback functions for preferences so we can react to user setting  */
  /* them.                                                                   */
  if (!prefSetDefFunc(MAX_MSG_LIST_LENGTH, (void *)gexGetDefListSize))
    messcrash("Unable to set callback function for default preference %s", MAX_MSG_LIST_LENGTH) ;

  if (!prefSetEditFunc(MAX_MSG_LIST_LENGTH, (void *)gexCheckListSize))
    messcrash("Unable to set callback function for checking preference %s", MAX_MSG_LIST_LENGTH) ;

  if (!prefSetFunc(USE_MSG_LIST, (void *)acedbSwitchMessaging))
    messcrash("Unable to set callback function for preference %s", USE_MSG_LIST) ;

  if (!prefSetFunc(MAX_MSG_LIST_LENGTH, (void *)gexSetMessListSize))
    messcrash("Unable to set callback function for preference %s", MAX_MSG_LIST_LENGTH) ;

  /* Now set up the messaging according to users preferences.                */
  use_msg_list = prefValue(USE_MSG_LIST) ;
  list_length = prefInt(MAX_MSG_LIST_LENGTH) ;

  gexSetMessPrefs(use_msg_list, list_length, prefcb) ;

  return ;
}

/* This routine is called when from gex and allows us to make sure that      */
/* user preferences are kept in step with message list/popup switching.      */
static void prefcb(BOOL msg_window)
{
  BOOL result ;

  result = setPreference(USE_MSG_LIST, PREF_BOOLEAN, &(msg_window)) ;

  return ;
}


/* Called by acedbInitMessaging() when preferences for messaging are first   */
/* set up after that its a callback from prefsubs.c for when user sets       */
/* message preference.                                                       */
void acedbSwitchMessaging(BOOL msg_list)
{
  int list_length ;

  if (msg_list)
    list_length = prefInt(MAX_MSG_LIST_LENGTH) ;
  else
    list_length = 0 ;

  gexSetMessPopUps(msg_list, list_length) ;

  return ;
}

/* Get PS fonts specific to an acedb database (used in printing files).*/
/*                                                                     */
static char colorDefault ()
{ FILE *fil ;
  int level ;
  char *cp ;
  BOOL isColor = FALSE ;
  char *fontfile = dbPathFilName("wspec", "psfonts", "wrm", "r", 0);

  if (fontfile)
    {
      if ((fil = filopen(fontfile, "", "r")))
	{ 
	  level = freesetfile(fil,"") ;
	  freespecial("\n/\\") ;
	  
	  while (freecard(level))
	    while ((cp = freeword ()))
	      if (!strcmp(cp,"COLOUR") || !strcmp (cp,"COLOR"))
		isColor = TRUE ;
     
	}
      messfree(fontfile);
    }

  return isColor ? 'c' : 'p' ;
}


/* Get PS fonts specific to an acedb database (used in saving files). */
/*                                                                    */
static void getFonts(FILE **fil, Stack fontNameStack, int *nFonts)
{
  char *fontfile = dbPathFilName("wspec", "psfonts", "wrm", "r", 0);
  
  if (fontfile)
    {
      
      if ((*fil = filopen(fontfile,"","r")))
	{
	  int level = freesetfile(*fil,"") ;
	  char *cp ;
	  
	  freespecial("\n/\\") ;
	  
	  while (freecard(level))
	    while ((cp = freeword ()))
	      if (!strcmp(cp,"COLOUR") || !strcmp (cp,"COLOR")) ; /* do nothing */
	      else
		{
		  pushText(fontNameStack,cp) ;
		  (*nFonts)++ ;
		}
	}
     
      messfree(fontfile);
    }
}


/* This code has been extracted from graphsub.c as it is acedb specific.     */
/* Lincoln Stein made some adjustments to the code so I've kept his date     */
/* stamp below.                                                              */
/*                                                                           */
static void classPrint (ACEOUT fo, KEY key)
{
  aceOutPrint (fo, "  %s:%s", className(key), freeprotect(name(key))) ;

  return ;
} /* classPrint */



#ifndef ACEDB_GTK
/* This code was extracted from graphxlib.c, it attempts to find the acedb   */
/* fonts file and sets this as input for freesubs calls in the graph         */
/* package.                                                                  */
/*                                                                           */
static void GetXlibFonts(int *level)
{
  char *fonts_file;
  FILE *fil ;

  fonts_file = dbPathFilName("wspec", "xfonts","wrm","r", 0);
  if (fonts_file)
    {
      fil = filopen(fonts_file,0,"r"); /* messerror if fails */
      if (fil)
	{
	  *level = freesetfile(fil,"") ;
	  freespecial ("\n/\\") ;
	}
      messfree(fonts_file);
    }
  else
    {
      messerror("Cannot find the Acedb Xfonts file wspec/xfonts.wrm."
		" The MIT default fonts will be used instead") ;
    }

  return ;
}
#endif /* ACEDB_GTK */


/*****************************************************************************/
/* Viewedit functions.                                                       */

/* Small function for viewedit, sets altered views to be anonymous.          */
/*                                                                           */
static void setAnon(VIEWWINDOW view, COLCONTROL control)
  {
  int i;

  /* if we've edited and not saved, make views anonymous */
  if (view->dirty)
    for (i=0; i<arrayMax(control->maps); i++)
      arr(control->maps, i, MAPCONTROL)->viewKey = 0;

  return ;
  }


static BOOL setSaveView(MENU menu)
  {
  BOOL result = FALSE ;

  if (writeAccessPossible())
    {
    menuUnsetFlags(menuItem(menu,"Save view"), MENUFLAG_DISABLED);
    menuUnsetFlags(menuItem(menu,"Save view as default"), MENUFLAG_DISABLED);
    result = TRUE ;
    }

  return result ;
  }

static void setSaveViewMsg(MAPCONTROL map, VIEWWINDOW view)
  {
  if (map->viewKey && !view->dirty)
    graphText(messprintf("View stored in: %s", name(map->viewKey)), 1, 1);
  else
    graphText("View not stored.", 1, 1);

  return ;
  }


/* All of these routines seem to be specific to acedb code...                */
/* They are all set in motion by a menu...so the menu is the key...          */

/* only called by other acedb-only routines...see below...                   */
/* Need to make currentView,viewWindowDraw  non-static,                      */
/* otherwise looks ok...                                                     */
/*                                                                           */
static void viewSaveView(KEY tag)
{
  VIEWWINDOW view = currentView("viewSaveView");
  MAPCONTROL map = view->currentMap;
  KEY viewKey;
  OBJ obj;
  KEY _View;
  
  lexaddkey("View", &_View, 0);
  
  viewKey = controlSaveView(map);
  if (!viewKey)
    return;

  view->dirty = FALSE;
  viewWindowDraw(view);
  
  obj = bsUpdate(map->key);
  if (bsAddTag(obj, _View))
    bsAddKey(obj, _View, viewKey);
  else
    messerror("No tag View in object %s - cannot complete save", 
	      name(map->key));

  if (tag)
    { if (bsAddTag(obj, tag))
	bsAddKey(obj, tag, viewKey);
      else  
	messerror("No tag %s in object %s - cannot complete save", 
		  name(tag),
		  name(map->key));
    }
  bsSave(obj);
}


/* only referenced in the acedb menu....only uses above function.            */
static void viewSaveMap(MENUITEM m)
{ viewSaveView(0);
}

/* only referenced in the acedb menu....only uses above function. */
static void viewSaveAsDefault(MENUITEM m)
{ KEY _Default_view; 
  lexaddkey("Default_view", &_Default_view, 0);
  viewSaveView(_Default_view);
}

/* only referenced in the acedb menu.... */
static void loadNamedView(MENUITEM m)
{
  KEY key, _VView;
  VIEWWINDOW view = currentView("loadNamedView");
  MAPCONTROL map = view->currentMap;
  char buffer[41];
  ACEIN view_in;

  lexaddkey("View", &key, _VMainClasses);
  _VView = KEYKEY(key);
  
  if (!(view_in = messPrompt("Load view named :", "", "t", 0)))
    return;
  
  strncpy (buffer, aceInWord(view_in), 40);
  buffer[40] = '\0';

  if (!lexword2key(buffer, &key, _VView))
    { 
      messout(messprintf("Can't find %s", buffer));
    }
  else
    {
      (void)controlSetView(map, key);
      
      view->dirty = FALSE;
      viewWindowDraw(view);
    }

  aceInDestroy (view_in);
  
  return;
} /* loadNamedView */


/*                                                                           */
/*                                                                           */
/* This is a very dangerous interface in that there is no checking of whether*/
/* we run off the end of the MENUSPEC array, this should only be altered in  */
/* conjunction with the function makeViewMenu in viewedit.c.                 */
/*                                                                           */
static void setViewMenu(MENUSPEC **curr)
  {
  MENUSPEC *i = *curr ;					    /* easier to follow. */

  i++ ;							    /* Go past the current entry. */
  i->f = (MENUFUNCTION)help ;
  i->text = "Help" ;
 
  i++ ;
  i->f = (MENUFUNCTION)menuSpacer ;
  i->text = "" ;

  i++ ;
  i->f = loadNamedView ;
  i->text = "Load named view" ;

  i++ ;
  i->f = viewSaveMap ;
  i->text = "Save view" ;

  i++ ;
  i->f = viewSaveAsDefault ;
  i->text = "Save view as default" ;

  *curr = i ;						    /* Update pointer to last entry. */

  return ;
  }


/*****************************************************************************/
/* ColControl routines.                                                      */


/* Trivial, but helps retain semantics of original colcontrol code where     */
/* this was #def'd for ACEDB only.                                           */
/*                                                                           */
static void resetkey(KEY *key)
  {
  *key = 0 ;

  return ;
  }

static void resetsave(VoidCOLINSTANCEOBJ *savefunc)
  {
  savefunc = NULL ;

  return ;
  }


static void menufree(int box, FREEOPT *viewmenu)
  {

  graphBoxFreeMenu(box, viewMenuFunc, viewmenu);

  return ;
  }


static void maplocate(OBJ init, VoidCOLINSTANCEOBJ *savefunc, MAPCONTROL map)
  {
  KEY _Magnification, _Projection_lines_on;

  *savefunc = mapLocatorSave ;
 
  if (init)
    {
    lexaddkey("Magnification", &_Magnification, 0);
    lexaddkey("Projection_lines_on", &_Projection_lines_on, 0);
      
    if (bsFindTag(init, _Projection_lines_on))
      map->hasProjectionLines = TRUE;
    else
      map->hasProjectionLines = FALSE;

    if (bsGetData(init, _Magnification, _Float, &map->magConf))
      map->mag = map->magConf;
    }

  }

static void scale(OBJ init, VoidCOLINSTANCEOBJ *savefunc, MAPCONTROL map)
  {
  KEY _Cursor_unit, _Cursor_on, _Scale_unit;
  float f;

  *savefunc = mapScaleSave ;

  if (init)
    {
    lexaddkey("Cursor_unit", &_Cursor_unit, 0);
    lexaddkey("Cursor_on", &_Cursor_on, 0);
    lexaddkey("Scale_unit", &_Scale_unit, 0);
    if (bsGetData(init, _Cursor_unit, _Float, &f))
      {
	map->cursor.unit = f;
	if (map->cursor.unit < 0.001)
	  map->cursor.unit = 0.001;
      }
    if (bsGetData(init, _Scale_unit, _Float, &f))
      {
	map->scaleUnit = f;
	if (map->scaleUnit <0.0001)
	  map->scaleUnit = 0.0001;
      }
    if (bsFindTag(init, _Cursor_on))
      map->hasCursor = TRUE;
    }

  return ;
  }


static void space(OBJ init, VoidCOLINSTANCEOBJ* spacefunc, SPACERPRIV private)
  {
  KEY _Spacer_width, _Spacer_colour;
  float w;

  *spacefunc = spacerSave;

  if (init)
    {
    lexaddkey("Spacer_width", &_Spacer_width, 0);
    lexaddkey("Spacer_colour", &_Spacer_colour, 0);
    if (bsGetData(init, _Spacer_width, _Float, &w))
      private->width = w;
    if (bsFindTag(init, _Spacer_colour))
      private->colour = controlGetColour(init);
    }

  return ;
  }


static void addmap(char *name, KEY *key)
  {

  lexaddkey(name, key, 0) ;

  return ;
  }


/* These routines from from colcontrol.c, they are acedb specific.           */

/* Read a view and make the columns and put them in map. */
static BOOL controlSetView(MAPCONTROL map, KEY view)
  {
  OBJ View  = bsCreate(view);
  KEY _Columns, _Submenus, _Cambridge, _Nobuttons, _HideHeader, colKey;
  COLPROTO proto;
  int i, disp;
  BSMARK mark = 0;
  COLCONTROL control = map->control;
  COLINSTANCE instance;
  char *s;

  lexaddkey("Columns", &_Columns, 0);
  lexaddkey("Submenus", &_Submenus, 0);
  lexaddkey("Cambridge", &_Cambridge, 0);
  lexaddkey("No_buttons", &_Nobuttons, 0);
  lexaddkey("Hide_header", &_HideHeader, 0);

  if (!bsFindTag(View, map->viewTag) || !bsGetData(View, _Columns, _Text, &s))
    { messout("Cannot use View object %s, Tag %s not set, "
	      "or no columns found.", 
	      name(view), name(map->viewTag));
      bsDestroy(View);
      return FALSE;
    }

/* Delete any existing column instances for this map */   
  for (i=0; i<arrayMax(control->instances); i++)
    { instance = arr(control->instances, i, COLINSTANCE);
      if (instance->map == map)
	{ controlDeleteInstance(control, instance);
	  i--;
	}
    }

  map->viewKey = view;

  map->submenus = bsFindTag(View, _Submenus);
  map->cambridgeOptions = bsFindTag(View, _Cambridge);
  map->noButtons = bsFindTag(View, _Nobuttons);
  control->hideHeader = bsFindTag(View, _HideHeader);

  if (bsGetData(View, _Columns, _Text, &s))
    do
      { 
	mark = bsMark(View, mark);
	if  (bsGetData(View, _bsRight, _Int, &disp) && bsPushObj(View))
	  {
	    if (bsGetKeyTags(View, _bsRight, &colKey))
	      for (i = 0; i<arrayMax(map->protoArray); i++)
		{ proto = arrp(map->protoArray, i, struct ProtoStruct); 
		  if (proto->key == colKey)
		    { if (proto->unique && instanceExists(map, proto))
			{ messout("Cannot create two columns of type %s",
				  name(colKey));
			  break;
			}
		      instance = controlInstanceCreate(proto,
						       map,
						       disp,
						       View,
						       s); 
		      if (instance)
			array(map->control->instances, 
			      arrayMax(map->control->instances),
			      COLINSTANCE) = instance ;
		      break;
		    }
		}		
	    if (! proto) 
	      messout("Column prototype missing for %s", name(colKey));
	  }
	bsGoto(View, mark); 
      }
    while (bsGetData(View, _bsDown, _Text, &s));
  
  bsDestroy(View);
  bsMarkFree(mark);
  if (arrayMax(control->maps) > 1)
    control->hideHeader = FALSE; /* don't hide header on multiple maps - too
				    confusing */
  return TRUE;
  }


static KEY controlSaveView(MAPCONTROL map)
{ KEY _VView, _Columns, _Submenus, _Cambridge, _Nobuttons, _HideHeader;
  KEY protoTag;
  COLCONTROL control = map->control;
  COLINSTANCE instance; 
  char buffer[41];
  KEY key;
  int i;
  BSMARK mark = 0 ;
  OBJ obj;
  ACEIN view_in;
  
  lexaddkey("View", &key, _VMainClasses);
  _VView = KEYKEY(key);
  lexaddkey("Columns" , &_Columns, 0);
  lexaddkey("Submenus", &_Submenus, 0);
  lexaddkey("Cambridge", &_Cambridge, 0);
  lexaddkey("No_buttons", &_Nobuttons, 0);
  lexaddkey("Hide_header", &_HideHeader, 0);

  if (!checkWriteAccess())
      return 0;

  view_in = messPrompt(messprintf("Save config of %s as:", map->name),
		       map->viewKey ? name(map->viewKey) : "", "t", 0);

  if (!view_in)
    return 0;

  strncpy(buffer, aceInWord(view_in), 40);
  buffer[40] = '\0';
  aceInDestroy (view_in);

  if (strlen(buffer) == 0)
    return 0;

  if (lexword2key(buffer, &key, _VView) && 
      !messQuery(messprintf("Overwrite existing %s?", buffer)))
    return 0;

  
  lexaddkey(buffer, &key, _VView);
  
  if (!key || !(obj = bsUpdate(key)))
    return 0;

  map->viewKey = key;

  bsKill(obj) ; obj = bsUpdate (key) ; /* kill off old version */

  bsAddTag(obj, map->viewTag);

  if (map->submenus)
    bsAddTag(obj, _Submenus);
  if (map->cambridgeOptions)
    bsAddTag(obj, _Cambridge);
  if (map->noButtons)
    bsAddTag(obj, _Nobuttons);
  if (control->hideHeader)
    bsAddTag(obj, _HideHeader);

  for (i=0; i<arrayMax(control->instances); i++)
    { instance = arr(control->instances, i, COLINSTANCE);
      if (instance->map != map)
	continue ;
      mark = bsMark(obj, mark) ;
      if (!bsAddData (obj, _Columns, _Text, instance->name) ||
	  !bsAddData (obj, _bsRight, _Int, &instance->displayed) ||
	  !bsPushObj (obj))
	{ messerror ("Invalid View model - need Columns Text Int #Column") ;
	  break ;
	}
      lexaddkey (instance->proto->name, &protoTag, 0) ;
      if (!bsAddTag (obj, protoTag))
	{ messerror ("Column type %s not a tag in ?Column -- can't store",
		     instance->proto->name) ;
	  bsGoto (obj, 0) ;
	  bsAddData (obj, _Columns, _Text, instance->name) ;
	  bsRemove (obj) ;
	  continue ;
	}
      if (instance->save)
	(*instance->save)(instance, obj) ;
      bsGoto(obj, mark) ;
    }
  bsSave (obj) ;

  bsMarkFree (mark) ;

  return key;
}


int controlGetColour(OBJ obj)
{ BSMARK mark = bsMark(obj, 0);
  KEY col;
  int result = WHITE;

  if (bsPushObj(obj) && bsGetKeyTags(obj, _bsRight, &col))
    result = col - _WHITE;
  
  bsGoto(obj, mark);
  bsMarkFree(mark);
  return result;
}

void controlSetColour(OBJ obj, int colour)      
{ BSMARK mark = bsMark(obj, 0);
  if (bsPushObj(obj))
    bsAddTag(obj, _WHITE + colour);
  bsGoto(obj, mark);

  bsMarkFree(mark);
}



static Array addToMenu(Array menu, STORE_HANDLE handle, KEY view)
{ char *viewnam;
  KEY _Name;
  int i, j;
  OBJ v;

  lexaddkey("Name", &_Name, 0);
  
  if (menu)
    { for (i = 0; i<arrayMax(menu); i++) /* ignore duplicates */
	if ( array(menu, i, FREEOPT).key == view)
	  return menu;
    }
  else
    { menu = arrayHandleCreate(10, FREEOPT, handle);
      array(menu, 0, FREEOPT).text = "Views";
    }
  
  if (!view) /* view = 0 means call control window */
    viewnam = "View control";
  else
    { char *s;
      if (!(v = bsCreate(view)) || !bsGetData(v, _Name, _Text, &s))
	s = name(view); /* default */
      viewnam = handleAlloc(0, handle, 1+strlen(s));
      strcpy(viewnam, s);
      bsDestroy(v); 
    }

  j = arrayMax(menu);
  array(menu, j, FREEOPT).text = viewnam;
  array(menu, j, FREEOPT).key = view;
  array(menu, 0, FREEOPT).key = j;
  
  return menu;
}





void controlMakeColumns(MAPCONTROL map, COLDEFAULT colInit, KEY override, BOOL needMinimal)
{ 
  /* Add a map to the current window.
     If a view is avialable, then use it to display columns, else 
     colInitString says which columns should be instantiated,
     a view supplied as arg takes priority over any found in the map object.
     Columns are added at the RHS of any existing in the window */

  OBJ o ;
  KEY view;
  KEY def = 0;
  KEY min = 0;
  KEY _View, _Default_view, _Minimal_view, _From_map;
  Array menu = 0;
  KEY key = map->key;


  
  lexaddkey("View", &_View, 0);
  lexaddkey("Default_view", &_Default_view, 0);
  lexaddkey("Minimal_view", &_Minimal_view, 0);
  lexaddkey("From_map", &_From_map, 0);


  menu = addToMenu(menu, map->handle, 0); /* call vieweditor, for mac */
  
  while (key && (o = bsCreate(key)))
    { 
      if (bsGetKey(o, _Default_view, &view))/* look for Default_view */
	{ if (!def) /* first default we find, we use */
	    def = view;
	  menu = addToMenu(menu, map->handle, view);
	}
      
      if (bsGetKey(o, _Minimal_view, &view))/* look for Default_view */
	{ if (!min) /* first default we find, we use */
	    min = view;
	  menu = addToMenu(menu, map->handle, view);
	}
      
      if (bsGetKey(o, _View, &view))
	do { 
	  menu = addToMenu(menu, map->handle, view);
	}  while (bsGetKey(o, _bsDown, &view));
    
      if (!bsGetKey(o, _From_map, &key)) /* do any we inherit from */
	key = 0;
      bsDestroy(o);
    }

  if (menu)
    map->viewMenu = arrayp(menu, 0, FREEOPT);

  if (!override)
    { if (needMinimal && min)
	override = min;
      else
	override = def;
    }
  if (!override  || !controlSetView(map, override)) 
    /* if there's a default view use it */
    controlReadConfig(map, colInit);

}

static void viewMenuFunc(KEY key, int box)
{
  COLCONTROL control = currentColControl("viewMenuFunc");

  if (key == 0)
    viewWindowCreate(0);
  else
    {
      controlSetView(control->currentMap, key);
      if (control->viewWindow) 
	{
	  /* destroy all memory associated with the viewWindow */
	  handleDestroy(control->viewWindow->handle);
	  /* kill the viewWindow struct and cause viewWindowFinalise() */
	  messfree (control->viewWindow);/* destroys the window */
	}
      controlDraw();
    }

  return;
} /* viewMenuFunc */



static void mapLocatorSave(COLINSTANCE instance, OBJ obj)
{ KEY _Magnification, _Projection_lines_on;

  lexaddkey("Magnification", &_Magnification, 0);
  lexaddkey("Projection_lines_on", &_Projection_lines_on, 0);
  
  if (instance->map->magConf != 0.0)
    bsAddData(obj, _Magnification, _Float, &instance->map->mag);
  if (instance->map->hasProjectionLines)
    bsAddTag(obj, _Projection_lines_on);
}




static void mapScaleSave(COLINSTANCE instance, OBJ obj)
{  KEY _Cursor_unit, _Cursor_on, _Scale_unit;
   MAPCONTROL map = instance->map;

   lexaddkey("Cursor_unit", &_Cursor_unit, 0);
   lexaddkey("Cursor_on", &_Cursor_on, 0);
   lexaddkey("Scale_unit", &_Scale_unit, 0);
   
   bsAddData(obj, _Cursor_unit, _Float, &map->cursor.unit);
   bsAddData(obj, _Scale_unit, _Float, &map->scaleUnit);
   if (map->hasCursor)
     bsAddTag(obj, _Cursor_on);
}


static void spacerSave(COLINSTANCE instance, OBJ obj)
{ KEY _Spacer_width, _Spacer_colour;
  SPACERPRIV private = instance->private;

  lexaddkey("Spacer_Width", &_Spacer_width, 0);
  lexaddkey("Spacer_colour", &_Spacer_colour, 0);

  bsAddData(obj, _Spacer_width, _Float, &private->width);
  bsAddTag(obj, _Spacer_colour);
  controlSetColour(obj, private->colour);
}

