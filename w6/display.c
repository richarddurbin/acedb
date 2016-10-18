/*  File: display.c
 *  Author: Jean Thierry-Mieg (mieg@mrc-lmb.cam.ac.uk)
 *  Copyright (C) J Thierry-Mieg and R Durbin, 1991
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
 * 	Richard Durbin (MRC LMB, UK) rd@mrc-lmb.cam.ac.uk, and
 *	Jean Thierry-Mieg (CRBM du CNRS, France) mieg@kaa.cnrs-mop.fr
 *
 * Description:
 **       Class sensitive object display                           **
 *        Shares and reuse display windows.
 * Exported functions:
 * display 
 *   displays a Key according to diplayType 
 *   if displayType==0, it defaults according to classes.wrm.
 *   Each display type reuses its own window until that window has 
 *     been preserved with displayPreserve.
 *
 * displayBlock
 *   puts up a message over the current active window and blocks 
 *     the display system until a key has been picked somewhere.
 *   the rest of the program still runs.
 *   The registered function receives as parameter the picked key 
 *     and, implicitly, the graph it was picked from, which is 
 *     active.
 *
 * displayUnBlock
 *   Clears out.
 *
 * displayRepeatBlock
 *   Reactivates the current blocking function.
 *
 * isDisplayBlocked
 *   should be called by clients to prevent access to blocked displays
 *
 * displayPreserve
 *   preserves the object, so that the next object of that class
 *   will not reuse its window.
 * HISTORY:
 * Last edited: May 15 15:47 2008 (edgrif)
 * Created: Sun Dec  1 17:29:54 1991 (mieg)
 * CVS info:   $Id: display.c,v 1.60 2012-04-12 15:21:38 edgrif Exp $
 *-------------------------------------------------------------------
 */

#include <wh/acedb.h>
#include <wh/lex.h>
#include <wh/pick.h>
#include <wh/client.h>
#include <wh/pref.h>
#include <wh/bs.h>
#include <wh/session.h>
#include <wh/dbpath.h>
#include <wh/aql.h>
#include <whooks/systags.h>
#include <whooks/sysclass.h>
#include <wh/display.h>
#include <wh/dna.h>		/* for dnaAnalyse() */
#include <wh/query.h>		/* for queryCreate() */
#include <wh/sessiondisp.h>	/* for sessionControl() */
/*#include <wh/dendrogram.h>*/
#include <wh/querydisp.h>		/* for qbeCreate, qbuildCreate */
#include <wh/tree.h>
#include <wh/forest.h>
#include <wh/vmap.h>
#include <wh/pmap.h>
#include <wh/fmap.h>
/*#include <wh/gmap.h>*/
#include <wh/grid.h>
#include <wh/keysetdisp.h>
#include <wh/longtext.h>
#include <wh/tabledisp.h>

#ifdef ZMAP
#include <wh/zmap.h>
#include <wh/genebuilder.h>
#endif

/********************************************************************/
/**************** program-wide globals *******************************/
/*********************************************************************/





BOOL isGifDisplay = FALSE ; /* used to signal that the graphical session
			       is non-interactive. may be used to unset
			       useless buttons etc */


BOOL displayReportGif = TRUE ;	/* if TRUE and isGifDisplay == TRUE
				   then we comment on each display()
				   call to say which object was 
				   displayed and how */

/*********************************************************************/

typedef struct AcedbDisplayStruct  AcedbDisplay ;
struct AcedbDisplayStruct 
{ 
  char title[42], help[32] ;
  float x, y, width, height ;
  int type ;  /* from GraphType */
} ;

/************************************/

static BOOL doDisplay(KEY key, KEY from, char *displayName, void *display_data) ;
static void setGraphType (char *displayName, int type);
static void setGraphTypes (void);
static AcedbDisplay *getDp (char *displayName, KEY *displayKey);
static void readDisplayDefaults(void); /* displays.wrm */
static void setDisplayFunctions(void);

static Array dArray = 0 ;
static Array displayFuncArray = 0 ;

static Array previous = NULL ; 

static Graph blockGraph = GRAPH_NULL ;
static BlockFunc blockFunc = NULL ;

static BOOL isRepeatBlock = FALSE ;


#define MAXDISPMENU 30
static FREEOPT dispOptMenu[MAXDISPMENU] = {
  {1, ""},
  {0, "Graph"},   /* shorter name, not too inacurate */
  {0, "Text"},
};

static FREEOPT displayOptions[] =
{
  {11, "display options"},
  {'g', "GraphType"},		/* ignored in displays.wrm */
  {'t', "Title"},
  {'w', "Width"},
  {'h', "Height"},
  {'x', "Xposition"},
  {'y', "Yposition"},
  {'m', "Menu"},
  {'a', "Help"},
  {'c', "Columns"},
  {'r', "Rows"},
  {'o', "Overlap"}
  } ;


static float col_Priority_G = 0.01001 ;			    /* Threshold for overlapping cols in
							     * fmap, can be overidden in displays.wrm. */






/************************************************************/
/* Must be called before any other display functions. */
void displayInit (void)
{

  /* Set up the static arrays for holding display info. */
  dArray = arrayCreate (12, AcedbDisplay) ;
  displayFuncArray = arrayCreate (32, DisplayFunc) ;
  previous = arrayCreate (32, Graph) ;

  /* hook up the display types with their display functions,
   * This will init the class 'Display' with all the types
   * the code knows about, this way we can verify the validity
   * of a vertain displayName when we get to parse displays.wrm */
  setDisplayFunctions();

 /* parse options.wrm again, but now also set the displayKey for 
  * each class. We do that after we set the display functions. 
  * This makes sure that all displays we have at this point are 
  * actually represented by generic display functions in the code. */
  pickGetClassProperties(TRUE);

  sysClassDisplayTypes();	/* after parsing options.wrm
				 * to override user settings that
				 * would otherwise corrupt */

  /* The graphtypes are a code issue, so we set all the defaults
   * here, thus initialising all possible names in the class 'display' */
  setGraphTypes() ;

  /* now parse displays.wrm to allow the user to override the
   * defaults geometry etc. for each display. We do that after
   * setGraphTypes, where all the display types are added to 
   * the class 'Display' that the code knows about.
   * Options for other displays are ignored.*/
  readDisplayDefaults();
  
  return;
} /* displayInit */


/* Return the fmap column overlap threshold. */
float displayGetOverlapThreshold()
{
  return col_Priority_G ;
}




/*********************************************************************/
/* displayName is now the name of an element of the display sysclass */
/*                                                                           */
/*                                                                           */
/* This is the old function, it's used in most of the code but doesn't allow */
/* any data to be passed through by the caller which is sometimes necessary, */
/* hence the new display call.                                               */
/*                                                                           */
/* This old call always passes in NULL for the display_data.                 */
/*                                                                           */
/* the old and new functions are temporary to allow code to work while I     */
/* migrate from the old to the new call which allows user data to be passed  */
/* through.                                                                  */
/*                                                                           */
BOOL display(KEY key, KEY from, char *displayName)
{
  BOOL result ;
  
  result = doDisplay(key, from, displayName, NULL) ;

  return result ;
}

/* The new call which allows user data to be passed through to the actual    */
/* display routine.                                                          */
/*                                                                           */
BOOL displayApp(KEY key, KEY from, char *displayName, void *display_data)
{
  BOOL result ;

  result = doDisplay(key, from, displayName, display_data) ;

  return result ;
}

/* Both the above routines call this one to do the actual work.              */
/*                                                                           */
static BOOL doDisplay(KEY key, KEY from, char *displayName, void *display_data)
{
  BOOL result = FALSE ;
  KEY displayKey = KEY_UNDEFINED ;
  BOOL gReuse ;
  DisplayFunc theFunc ;
  Graph g ;

  if (!key && !from)
    messcrash("Call to display() with zero \"key\" and zero \"from\" and displayName = %s.",
	      displayName ? displayName : "(NULL)") ;

  if (!previous)
    messcrash("doDisplay() called before displayInit().") ;


  if (graphActivate(blockGraph))
    {
      /* Blockfunc will set the active graph to the one the user is interacting with
       * so save it so we can reset as the active graph below. */
      (*blockFunc)(key) ;
      g = graphActive() ;

      if (isRepeatBlock)
	{
	  isRepeatBlock = FALSE ;
	}
      else
	{ 
	  if (graphActivate(blockGraph))
	    graphUnMessage() ;

	  blockGraph = GRAPH_NULL ;
	}

      /* Reactivate users orginal graph. */
      graphActivate(g) ;	

      return FALSE ;
    }


  /*
   * Xclient stuff....get object and neighbours
   */  
  if (externalServer)
    externalServer(key, 0, 0, TRUE) ;

  if (iskey(key) != 2)
    return FALSE ;

  /* display must be TREE for the  model */
  if (!KEYKEY(key) && pickType(key) == 'B')
    displayName = "TREE" ;
  
  if (!displayName)
    displayKey = pickDisplayKey(key) ;
  else if (!lexword2key(displayName, &displayKey, _VDisplay))
    {
      messerror ("Display type %s unknown",
		 displayName ? displayName : "(NULL)") ;
      return FALSE ;    /* not displayable */
    }

  if (displayKey == 0) 
    return FALSE ;    /* not displayable */

  if ((g = array(previous, KEYKEY(displayKey), Graph)))
    {
      if (graphActivate(g))
	{
	  gReuse = TRUE ;
	}
      else
	{
	  array(previous, KEYKEY(displayKey), Graph) = 0 ;
	  gReuse = FALSE ;
	}
    }
  else 
    gReuse = FALSE;

  /* Call the display function, if it fails (e.g. because user did not select
   * an appropriate display type) then: if display type is not objects
   * natural display type ask user if they want to use the natural one,
   * otherwise try the tree display as its the most basic object display. */
  if ((theFunc = array(displayFuncArray,KEYKEY(displayKey), DisplayFunc)))
    {
      result = theFunc(key, from, gReuse, display_data) ;

      if (!result)
	{
	  KEY objdisp_key = pickDisplayKey(key) ;      
	  BOOL new_display = TRUE ;
	  
	  if (!key)					    /* Deal with missing key param. */
	    {
	      key = from ;
	      from = KEY_UNDEFINED ;
	    }

	  if (objdisp_key)
	    {
	      if (objdisp_key != displayKey)
		{
		  if ((new_display = messQuery("Sorry, object %s cannot be displayed with "
					       "the \"%s\" display. "
					       "Do you want to display it using the "
					       "\"%s\" display instead ?",
					       nameWithClassDecorate(key),
					       name(displayKey), name(objdisp_key))))
		    displayKey = objdisp_key ;
		}
	      else
		{
		  KEY treedisp_key ;

		  if (lexword2key("TREE", &treedisp_key, _VDisplay)
		      && displayKey != treedisp_key)
		    displayKey = treedisp_key ;
		}

	      if (new_display)
		{
		  if ((theFunc = array(displayFuncArray, KEYKEY(displayKey), DisplayFunc)))
		    {
		      if ((g = array(previous, KEYKEY(displayKey), Graph)))
			{
			  if (graphActivate(g))
			    {
			      gReuse = TRUE ;
			    }
			  else
			    {
			      array(previous, KEYKEY(displayKey), Graph) = 0 ;
			      gReuse = FALSE ;
			    }
			}
		      else 
			gReuse = FALSE;

		      if (!(result = theFunc(key, from, gReuse, display_data)))
			messerror ("Sorry, cannot display object %s with \"%s\" display.",
				   nameWithClassDecorate(key), name(displayKey)) ;
		    }
		}
	    }
	}

      if (result)
	{
	  array(previous, KEYKEY(displayKey), Graph) = graphActive() ;
	  graphPop () ;
	  if (isGifDisplay && displayReportGif)
	    messout("%s %s \"%s\"\n", name(displayKey), className(key), name(key));
	}
    }

  return result ;
}

  
/**********************************/

void displayPreserve (void)
{
  Graph g = graphActive() ;
  int i ;

  if (!previous)
    messcrash("displayPreserve() called before displayInit().") ;

  i = arrayMax(previous) ;
  while (i--)
    if (array(previous,i, Graph) == g)
      array(previous,i, Graph) = 0 ;

  return ;
}

/**************************************/

/* Cover function for full version. */
BOOL displayBlock(BlockFunc func, char *message)
{
  BOOL result = FALSE ;

  messStatus("Select by double-clicking") ; /* mhmp nov 97 */

  if (!graphExists(blockGraph))
    {
      blockGraph = graphActive() ;
      blockFunc = func ;

      isRepeatBlock = FALSE ;

      if(!prefValue("NO_MESSAGE_WHEN_DISPLAY_BLOCK"))
	{
	  graphMessage(messprintf ("%s\n\n%s",
				   "Select an object by double-clicking as\n"
				   "though you were going to display it.",
				   message ?  message : "")) ;
	}

      result = TRUE ;
    }

  return result ;
}

/**************************************/

void displayRepeatBlock (void)
{
  isRepeatBlock = TRUE ;
}

/**************************************/

void displayUnBlock(void)
{
  if (blockGraph)		/* prevent callback recursion */
    { blockGraph = 0 ;
      graphUnMessage () ;
    }
  isRepeatBlock = FALSE ;
  blockFunc = 0 ;
}

/**************************************/

BOOL isDisplayBlocked (void) 
{
  return (blockGraph != 0) ;
}

/******************************************/
/******************************************/

/**** next section supports some simple and generic display types ***/

BOOL displayAs (KEY key, KEY from, BOOL isOldGraph, void *app_data)
{ 
  OBJ obj ;
  KEY tag, newkey ;

  if (!(obj = bsCreate (key)))
    return FALSE ;

  if (bsFindTag (obj, str2tag ("Display_as")) &&
      bsGetKeyTags (obj, _bsRight, &tag) &&
      bsGetKey (obj, _bsRight, &newkey))
    { bsDestroy (obj) ;
      display (newkey, from, 0) ;
      return TRUE ;
    }
  
  bsDestroy (obj) ;
  display (key, from, "TREE") ;
  return FALSE ;
}


static int escapeUrl(char *old, char *new)
{ int len = 0;
  char c;
  while ((c = *old++))
    { 
      switch (c) 
	{ 
	case ' ':
	  c = '+'; /* fall through */
	default:
	  if (new) *new++ = c;
	  len++;
	  continue;
	case '?':
	case '=':
	case '+':
	case '/':
	case '#':
	case '&':
	case '%':
	case '(': /* these two get mangled by the netscape call interface */
	case ')':
	  if (new)
	    { sprintf(new, "%%%2.2X", c);
	      new += 3; 
	    }
	  len += 3;
	  continue;
	}
    }
  if (new) *new = 0;
  return len+1; /* +1 for null terminator */
}
	


BOOL doWWWDisplay (KEY key, KEY from, int recursecount)
{
  OBJ obj, fromObj ;
  KEY template ;
  char *url = 0 ;
  STORE_HANDLE handle ;
  char *str, *new, *cname, *urlsave=0 ;	/* init for compiler happiness */
  BOOL result = FALSE ;
  Array a ;
  int i, len ;

  if (recursecount == 20)
    { messerror("Wild recursion detected in wwwDisplay!");
      return FALSE;
    }

  if (!(obj = bsCreate (key)))
    return FALSE;

  if (bsGetText (obj, str2tag ("URL"), &url)) 
    { 
      BOOL ret;
      ret = graphWebBrowser(url);
      bsDestroy (obj);
      return ret;
    }
  
  if (bsGetKey (obj, str2tag ("Web_location"), &template))
    { bsDestroy(obj) ;
      if (recursecount == 0)
	return doWWWDisplay (template, key, 1) ;
      else /* RD 010801 need to pass on original key as from */
	return doWWWDisplay (template, from, recursecount+1) ;
    }
  
  if (from == key)
    { bsDestroy(obj);
      return FALSE;
    }

  handle = handleCreate() ;
  a = arrayHandleCreate( 10, BSunit, handle) ;
  
	/* RD 010801 rewrote next section to consider options correctly */

  if (bsFindTag (obj, str2tag ("Reference_tag")) &&
      bsFlatten (obj, 2, a) &&
      (fromObj = bsCreate(from)))
    { for (i = 0 ; i < arrayMax(a) ; i += 2)
        if ((!arr(a,i+1,BSunit).s || 
	     !strcmp (className(from), arr(a,i+1,BSunit).s)) &&
	    bsGetText (fromObj, str2tag(arr(a,i,BSunit).s), &url))
	  {
	    url = strnew(url, handle);
	    break;
	  }
      bsDestroy(fromObj);
    }

  if (!url && bsFindTag(obj, str2tag("Use_name")))
    {
      if (bsGetText(obj, _bsRight, &cname))
	do { /* search for matching class */
	  if (strcmp(cname, className(from)) == 0)
	    { 
	      url = strnew(name(from), handle);
	      break;
	    }
	} while (bsGetText(obj, _bsDown, &cname));
      else
	url = strnew(name(from), handle) ; /* no class constraint */
    }
  
  if (!url)	/* RD 010801 last chance: try class object */
   {
     if (lexword2key (className(key), &template, _VClass))
       { 
	 bsDestroy (obj) ;
	 handleDestroy (handle) ;
	 if (recursecount == 0)
	   return doWWWDisplay (template, key, 1) ;
	 else
	   return doWWWDisplay (template, from, recursecount+1) ;
       }
     else  /* shouldn't get here because there should be a class key */
       messcrash ("Richard owes Simon a beer in doWWWdisplay") ;
   }
  
  if (bsFindTag (obj, str2tag("Rewrite")) && bsFlatten(obj, 4, a))
    for( i=0; i < arrayMax(a); i +=4)
      {  
        if ((str = arr(a, i+2, BSunit).s) &&
            strlen(url) >= strlen(str) &&
            strncmp(url, str, strlen(str)) == 0 )
          { urlsave = url; /* in case we fail on the next test */
            url += strlen(str);
          }
        else
          if (str) continue; /* no string matches */
      
        if ((str = arr(a, i+3, BSunit).s) &&
            (strlen(url) >= strlen(str) &&
             strcmp(url+strlen(url)-strlen(str), str) == 0))
          *(url+strlen(url)-strlen(str)) = 0;
        else
          if (str) /* no string matches */
            { url=urlsave;/* restore */
              continue; 
            }
        
        /* now url has pref/postfixes removed, escape url-dangerous chars */
        /* before adding new ones, only do this when using object name or  */
        /* reference tag, not a hard-specified URL */
        new = (char *)halloc(escapeUrl(url, 0), handle);
        (void)escapeUrl(url, new);
        url = new;
        
        /* now add prefix and postfix */
        len = 1+strlen(url);
	if (arr(a, i, BSunit).s) len += strlen(arr(a, i, BSunit).s);
    	if (arr(a, i+1, BSunit).s) len += strlen(arr(a, i+1, BSunit).s);
	new = (char *)halloc(len, handle);
        if (arr(a, i, BSunit).s) strcpy(new, arr(a, i, BSunit).s);
        strcat(new, url);
        if (arr(a, i+1, BSunit).s) strcat(new, arr(a, i+1, BSunit).s);
        result = graphWebBrowser (new) ;
        goto done ;
      }
  
done:
  bsDestroy (obj) ;
  handleDestroy (handle) ;
  return result ;
}

BOOL wwwDisplay (KEY key, KEY from, BOOL isOldGraph, void *unused)
{
  return doWWWDisplay( key, from, 0);
}


/******************************************/
/********* Display Customization  *********/
/******************************************/

Graph displayCreate(char *displayName)
{
 AcedbDisplay *dp = 0 ;
 Graph g ;
 float x, y ;
 static Array a = 0 ;
 int i ;
 KEY key = 0 ;

 if (!displayName)
   return 0 ;

 /* by this time the displayName is guaranteed to exist in the
  * 'Display' class unless a new name comes along */
 if (!(dp = getDp(displayName, &key)))
   messcrash("displayCreate() - unknown display %s\n"
	     "possible programmer typo in calling code or "
	     "the name isn't registered with the code, check "
	     "setGraphTypes() and setDisplayFunctions() !", displayName);

 /***** A trick to prevent window superposition ***/
 if (!a)
   a = arrayCreate(20, Graph) ;
 for (i = 0 ; i < arrayMax(a) ; i++)
   if (!graphExists(arr(a,i,Graph)))
     break ;

 x = dp->x + i/30.0 ;
 while (x > 1.0) x -= 1.0 ; /* prevent overflow HJC */

 y = dp->y + i/30.0 ;
 while (y > 1.0) y -= 1.0 ;

  /* This is useful if the window manager does not
   * prompt the user, and is called by displayCreate
   */

 g =  graphCreate(dp->type , dp->title, 
		  x, y, dp->width, dp->height) ;

 /* if  (dp->type == TEXT_HSCROLL)
    graphTextBounds(80, 24); */

 if (g)
   graphHelp(dp->help) ;
 
 array(a,i,Graph) = g ;

 return g ;
} /* displayCreate */

/**************************************/



/* WHO PUT THIS IN....AAGGGGHHHHH....                                        */
/* some display modules don't have public includes 
   for the DisplayFunc yet */
extern BOOL

pepDisplay (KEY, KEY, BOOL, void *),
        gMapDisplay (KEY, KEY, BOOL, void *), /* can't include gmap.h as it clashes with other includes */
        alignDisplay (KEY, KEY, BOOL, void *),
        fingerPrintDisplay (KEY, KEY, BOOL, void *),
	multiTraceDisplay (KEY, KEY, BOOL, void *),
	metabDisplay (KEY, KEY, BOOL, void *),
        cMapDisplay (KEY, KEY, BOOL, void *),
	dendrogramDisplay(KEY, KEY, BOOL, void *),
        imageDisplay (KEY, KEY, BOOL, void *) ;


/**************************************/

/* some Get/Set functions for display data. */

FREEOPT *displayGetShowTypesMenu(void)
{ 
  return dispOptMenu;
} /* displayGetShowTypesMenu */


char *displayGetTitle(char *displayName)
{ 
  char *title = NULL ;
  AcedbDisplay *dp;

  if ((dp = getDp(displayName, 0)))
    title = dp->title ;

  return title ;
} /* displayGetTitle */


/* Get the graph for the "current" display of the named display type
 * (e.g. "FMAP"), this may be GRAPH_NULL if there is no current display
 * of that type or all displays of that type are "preserved". */
Graph displayGetGraph(char *displayName)
{
  Graph display_graph = GRAPH_NULL ;
  KEY displayKey = KEY_UNDEFINED ;

  if (!displayName || !*displayName || !lexword2key(displayName, &displayKey, _VDisplay))
    messcrash("Invalid display type name: %s",
	      (!displayName ? "NULL ptr" :
	       (!*displayName ? "NULL string" : displayName))) ;

  display_graph = array(previous, KEYKEY(displayKey), Graph) ;

  return display_graph ;
}

/* Set the graph  for the "current" display of the named display type
 * (e.g. "FMAP"), graph may be GRAPH_NULL to remove a particular displa
 * type from the list. */
void displaySetGraph(char *displayName, Graph graph)
{
  KEY displayKey = KEY_UNDEFINED ;

  if (!displayName || !*displayName || !lexword2key(displayName, &displayKey, _VDisplay))
    messcrash("Invalid display type name: %s",
	      (!displayName ? "NULL ptr" :
	       (!*displayName ? "NULL string" : displayName))) ;

  array(previous, KEYKEY(displayKey), Graph) = graph ;

  return ;
}

/* set default size for all displays, to be used after displayInit */
/* used by 'dimensions' in giface */
void displaySetSizes (int wpixels, int hpixels)
{
  TABLE *displayList;
  int i;
  KEY d;

  displayList = aqlTable("select all class Display", (ACEOUT)NULL, 0);
  if (displayList)
    for (i = 0; i < tabMax(displayList, 0); i++)
      {
	d = KEYKEY(tabKey(displayList, i, 0));
	
	if (arr(displayFuncArray, d, DisplayFunc))
	  {
	    /* this is a real object display, with a DisplayFunc */
	    displaySetSize(name(tabKey(displayList, i, 0)),
			   wpixels, hpixels);
	  }
	
      }

  tableDestroy (displayList);

  return;
} /* displaySetSizes */


void displaySetSize (char *displayName, int wpixels, int hpixels)
{ 
  KEY displayKey ;
  Graph existingGraph, oldGraph = graphActive(); 
  AcedbDisplay *dp;
  static float xfac = -1, yfac;
  float sw, sh, wfraction, hfraction ;
  int pw, ph;

  /* the width and height factors required by the graphPackage 
   * are fractions given relative to window size of 900x900,
   * example window size of 450x450 pixels requires 
   * wfraction, hfraction = 0.5 */
  if (xfac == -1)
    {
      /* init only once, screen size doesn't change */
      graphScreenSize (&sw, &sh, 0, 0, &pw, &ph) ;
      xfac = sw/pw ; yfac = sh/ph ;
    }
  wfraction = xfac * wpixels; hfraction = yfac * hpixels;

  dp = getDp(displayName, &displayKey);
  if (!dp) 
    return;

    /* we're resetting dimensions, so next time we display a new
     * window should be opened with the new dimensions, we there
     * erase the record of a previous open window, to avoid reuse
     * of an old window with the old size. */
  if ((existingGraph = array(previous,KEYKEY(displayKey), Graph)) &&
      graphActivate(existingGraph))
    {
      if (wfraction != dp->width ||
	  hfraction != dp->height)
	/* if we've changed the parameters, then force a new 
	   window next time, so the new dimension take effect */
	array(previous,KEYKEY(displayKey), Graph) = 0;
    }

  dp->width = wfraction;
  dp->height = hfraction;

  graphActivate(oldGraph);

  return;
} /* displaySetSize */


void displaySetSizeFromWindow (char *displayName)
/* register current window size for next display create */
{
  float sx, sy, sw, sh ;
  int pw, ph;
  static float xfac = -1, yfac;

  if (xfac == -1)
    {
      /* init only once, screen size doesn't change */
      graphScreenSize (&sw, &sh, 0, 0, &pw, &ph) ;
      xfac = sw/pw ; yfac = sh/ph ;
    }
  if (graphWindowSize (&sx, &sy, &sw, &sh))
    displaySetSize (displayName, sw / xfac, sh / yfac) ;

  return;
} /* displaySetSizeFromWindow */


BOOL displayIsObjectGeneric(char *displayName, KEY *keyp)
/* only returns true if the given name is a valid display type
 * for generic object displays,
 * i.e. a display with registered function */
{
  KEY d, displayKey;

  if (lexword2key(displayName, &displayKey, _VDisplay))
    {
      if (keyp) *keyp = displayKey;

      d = KEYKEY(displayKey);
      
      if (arr(displayFuncArray, d, DisplayFunc))
	return TRUE;
    } 

  if (keyp) *keyp = 0;
  return FALSE;
} /* displayIsObjectGeneric */


BOOL displayIsValid(char *displayName, KEY *keyp)
/* is true if the given name is any valid display name */
{
  KEY displayKey;

  if (lexword2key(displayName, &displayKey, _VDisplay))
    {
      if (keyp) *keyp = displayKey;
      return TRUE;
    }

  if (keyp) *keyp = 0;
  return FALSE;
} /* displayIsValid */


/**************** private functions *****************/

static void readDisplayDefaults(void)	/* parse displays.wrm */
{
  AcedbDisplay *dp ;
  int line = 0 ;
  char *cp ; float x ;
  KEY option, displayKey, treeKey;
  FILE *fil = 0;
  char *filename = dbPathFilName("wspec", "displays", "wrm", "r", 0); 
  
  if (filename) /* check first to avoid messerror */
    {
      fil = filopen(filename, 0, "r");
      messfree(filename);
    }

  if (!fil)
    {
      messerror("Cannot find display definition file : wspec/displays.wrm");
      return;
    }

  dispOptMenu[0].key = 2; /* since this gets called twice... */
  if (!lexword2key("TREE", &treeKey, _VDisplay))
    messcrash("readDisplayDefaults() - can't find Display TREE to provide as display default");
  dispOptMenu[2].key = treeKey ; /* provide a tree default unless overridden */

  /* Defaults */

  while(line ++ ,freeread(fil))
    /* read _D... */
    if ((cp = freeword()) && *cp++ == '_' && *cp++ == 'D')
      {
	if(!*cp)
	  messExit("Error parsing line %d of wspec/displays.wrm", line);
	
	if (!(dp = getDp(cp, &displayKey)))
	  {
	    /* unrecognised display type */
#ifdef ABORT_ON_BAD_TYPE
	    TABLE *displayList = aqlTable("select all class Display order", 0);
	    Stack s = stackCreate(300);
	    int i;

	    /* present a list of types that have been defined before */

	    for (i = 0; i < tableMax(displayList); ++i)
	      {
		catText(s, messprintf("%s\n",
				       name(tabKey(displayList, i, 0))));
	      }
	    
	    messExit("Error in wspec/displays.wrm at line %d : "
		     "unrecognised display type %s!\n"
		     "Choose from possible types :\n"
		     "%s",
		     line, cp, popText(s));
#else
	    continue;
#endif /* ABORT_ON_BAD_TYPE */
	  }
	
	while (TRUE)
	  {
	    freenext() ;
	    if (!freestep('-')) 
	      {
		if ((cp = freeword()))
		  messExit ("Error in wspec/displays.wrm at line %d : "
			    "option %s has to start with dash ('-') !",
			    line, freepos()) ;
		else
		  break ;
	      }
	    
	    freenext() ;
	    if (!freekey(&option, displayOptions))
	      messExit ("Error in wspec/displays.wrm at line %d : "
			"unknown option %s", line , freepos()) ;
	    
	    switch (option) 
	      {
	      case 'm':
		cp = freeword();
		if (dispOptMenu[0].key < MAXDISPMENU-1)
		  { int where;
		    if (displayKey == treeKey)
		      where = 2;
		    else
		      where = ++dispOptMenu[0].key;
		    dispOptMenu[where].key = displayKey;
		    dispOptMenu[where].text = strnew(cp, 0);
		  }
		break;
	      case 't':
		cp = freeword() ;
		strncpy(dp->title, cp, 41) ;
		break ;
	      case 'a':
		if ((cp = freeword()))
		  strncpy(dp->help, cp, 31) ;
		break ;
	      case 'g':                       /* GraphType (enum) IGNORED */
		freeword() ;	/* skip */
		/* we don't allow the user anymore to set this parameter -
		 * the graphType is a code issue, and it would be too
		 * easy to crash the code by setting an unexpected
		 * graphType in displays.wrm! */
		break ;
	      case 'x':
		if (!freefloat(&x))
		  messExit("Error in wspec/displays.wrm at line %d : "
			   "Missing float x value at %s",
			   line,  freepos()) ;
		if (x < 0 || x > 1.3)
		  messout ("Error in wspec/displays.wrm at line %d : "
			   "x value %f out of range 0.0 .. 1.3",
			   line, x ) ;
		else
		  dp->x  = x ;
		break ;
	      case 'w': 
		if (!freefloat(&x))
		  messExit("Error in wspec/displays.wrm at line %d : "
			   "Missing float width value %s",
			   line,  freepos()) ;
		if (x > 1.3)
		  messout ("Error in wspec/displays.wrm at line %d : "
			   "width value %f out of range 0.0 .. 1.3",
			   line, x ) ;
		else
		  dp->width  = x ;
		break ;
	      case 'c': 
		if (!freefloat(&x))
		  messExit("Error in wspec/displays.wrm at line %d : "
			   "Missing float columns value %s",
			   line,  freepos()) ;
		/* negative width is interpreted in char/pixel units 
		   by the graph package. */
		dp->width  = -x ;
		break ;
	      case 'y':
		if (!freefloat(&x))
		  messExit("Error in wspec/displays.wrm at line %d : "
			   "Missing float y value at %s",
			   line,  freepos()) ;
		if (x < 0 || x > 1.0)
		  messout ("Error in wspec/displays.wrm at line %d : "
			   "y value %f out of range 0.0 .. 1.0",
			   line, x ) ;
		else
		  dp->y  = x ;
		break ;
	      case 'h':
		if (!freefloat(&x))
		  messExit("Error in wspec/displays.wrm at line %d : "
			   "Missing float height value at %s",
			   line,  freepos()) ;
		if (x > 1.0)
		  messout ("Error in wspec/displays.wrm at line %d : "
			   "height value %f out of range 0.0 .. 1.0",
			   line, x ) ;
		else
		  dp->height  = x ;
		break ; 
	      case 'r': 
		if (!freefloat(&x))
		  messExit("Error in wspec/displays.wrm at line %d : "
			   "Missing float rows value %s",
			   line,  freepos()) ;
		/* negative width is interpreted in char/pixel units 
		   by the graph package. */
		dp->height  = -x ;
		break ;
	      case 'o': 
		if (!freefloat(&x))
		  messExit("Error in wspec/displays.wrm at line %d : "
			   "Missing float value %s",
			   line,  freepos()) ;
		col_Priority_G = x ;
		break ;
	      }
	  }
      }
  filclose (fil);

  return;
} /* readDisplayDefaults */


static AcedbDisplay *getDp (char *displayName, KEY *displayKey)
{
  AcedbDisplay *dp ;
  KEY d, dKey;

  if (!lexword2key(displayName, &dKey, _VDisplay))
    return 0;
  d = KEYKEY (dKey) ;

  if (!dArray)
    messcrash("getDp() called before displayInit");

  dp = arrayp(dArray, d, AcedbDisplay) ;
  
  if (!dp->width && !dp->height)
    {
      dp->x = .001 ; dp->y = 0.001 ;
      dp->width = .5  ; dp->height = .3 ;
      strncpy(dp->title, displayName, 31) ;
      strncpy(dp->help, "", 31) ;
    }

  if (displayKey) *displayKey = dKey;

  return dp;
} /* getDp */


static void setDisplayFunc(char *displayName, DisplayFunc f)
{
  KEY displayKey = 0 ;	

  /* although these displays may not have been defined in
   * displays.wrm, we make sure that they exist in the
   * class 'Display', because these are types that the code
   * is guaranteed to know about, because it links in functions
   * to deal with their display (hence lexaddkey...) */
  lexaddkey (displayName, &displayKey, _VDisplay) ;
  array (displayFuncArray , KEYKEY (displayKey), DisplayFunc) = f ;

  return;
} /* setDisplayFunc */

/************************************************************/

/* For each display type that is a generic object display, 
   you must provide a display function and register it
   by a line of code in the following function :
*/
static void setDisplayFunctions (void)
{
  /* System hooks */
  setDisplayFunc ("TREE",              treeDisplay) ;
  setDisplayFunc ("DtKeySet",          keySetDisplay) ;
  setDisplayFunc ("DtForest",          forestDisplay) ;
  setDisplayFunc ("DtLongText",        longTextDisplay) ;
  setDisplayFunc ("DISPLAY_AS",        displayAs) ;
  setDisplayFunc ("WWW",               wwwDisplay) ;
  setDisplayFunc ("DtSpreadSheet",     tableDisplay) ;
  setDisplayFunc ("DtTableResult",     tableResultDisplay) ;

  /* Application hooks */
  setDisplayFunc ("PMAP",              pMapDisplay) ;
  setDisplayFunc ("GMAP",              gMapDisplay) ;
  setDisplayFunc ("VMAP",              vMapDisplay) ;
  setDisplayFunc ("FMAP",              (DisplayFunc)fMapDisplay) ;
#ifdef ZMAP
  //setDisplayFunc ("ZMAP",              genebuilderDisplay) ; 
   setDisplayFunc ("ZMAP",              (DisplayFunc)zMapDisplay) ;
#endif
  setDisplayFunc ("CMAP",              cMapDisplay) ;
  setDisplayFunc ("DtMULTIMAP",        multiMapDisplay) ;
  setDisplayFunc ("DtPmapFingerprint", fingerPrintDisplay) ;
  setDisplayFunc ("PEPMAP",            (DisplayFunc)pepDisplay) ;
  setDisplayFunc ("DtAlignment",       alignDisplay) ;
  setDisplayFunc ("GRID",              gridDisplay) ;
  setDisplayFunc ("DtImage",           imageDisplay) ;
  setDisplayFunc ("METAB",             metabDisplay) ;
  setDisplayFunc ("DtDendrogram",      dendrogramDisplay) ;
#ifdef ACEMBLY
  setDisplayFunc ("DtMultiTrace",      multiTraceDisplay) ;
#endif

  return;
} /* setDisplayFunctions */




static void setGraphType (char *displayName, int type)
{ 
  KEY displayKey = 0 ;	
  AcedbDisplay *dp ;

  lexaddkey (displayName, &displayKey, _VDisplay) ;
  dp = getDp(displayName, 0);
  dp->type = type ;

  return;
} /* setGraphType */


static void setGraphTypes (void)
/* set here the type of all graphs to the types 
 * that the code copes with */
{
  /* system display routines */
  setGraphType ("TREE", TEXT_FULL_SCROLL) ;
  setGraphType ("DtKeySet", TEXT_FIT) ;
  setGraphType ("DtForest", TEXT_FIT) ;
  setGraphType ("DtChrono", TEXT_FULL_SCROLL) ;
  setGraphType ("DtLongText", TEXT_FULL_SCROLL) ;
  /* DISPLAY_AS and WWW have no graphs as such */

  /* graphical display extensions */
  setGraphType ("PMAP", TEXT_FIT) ;
  setGraphType ("GMAP", TEXT_FIT) ;
  setGraphType ("VMAP", TEXT_FIT) ;
  setGraphType ("FMAP", TEXT_HSCROLL) ;
  setGraphType ("CMAP", TEXT_FIT) ;
  setGraphType ("DtMULTIMAP", TEXT_FIT) ;
  setGraphType ("DtPmapFingerprint", TEXT_FULL_SCROLL) ;
  setGraphType ("PEPMAP", TEXT_FIT) ;
  setGraphType ("DtAlignment", TEXT_FULL_SCROLL) ;
  setGraphType ("GRID", TEXT_FULL_SCROLL) ;
  setGraphType ("DtImage", PIXEL_SCROLL) ;
  setGraphType ("METAB", TEXT_FULL_SCROLL) ;
  setGraphType ("DtDendrogram", TEXT_FULL_SCROLL) ;
#ifdef ACEMBLY
  setGraphType ("DtMultiTrace", TEXT_FIT) ;
#endif


  /* these display types are not generic, they can't be used for 
     displaying objects, they are just here, so their size can be
     configured in displays.wrm */
  setGraphType ("DtMain", TEXT_FIT) ;
  setGraphType ("DtLayoutEditor", TEXT_FIT) ;
  setGraphType ("DtPreferenceEditor", TEXT_SCROLL) ;
  setGraphType ("DtFile_Chooser", TEXT_SCROLL) ;
  setGraphType ("DtHelp", TEXT_FULL_SCROLL) ;
  setGraphType ("DtAce_Parser", TEXT_SCROLL) ;
  setGraphType ("DtDump", TEXT_SCROLL) ;
  setGraphType ("DtSession", TEXT_FULL_SCROLL) ;
  setGraphType ("DtStatus", TEXT_SCROLL) ;
  setGraphType ("DtUpdate", TEXT_SCROLL) ;
  setGraphType ("DtQuery", TEXT_SCROLL) ;
  setGraphType ("DtQueryBuilder", TEXT_FULL_SCROLL) ;
  setGraphType ("DtQueryByExample", TEXT_FULL_SCROLL) ;
  setGraphType ("DtSpreadSheet", TEXT_SCROLL) ;	/* table-maker window */
  setGraphType ("DtTableResult", TEXT_FULL_SCROLL) ;
  setGraphType ("DtAqlQuery", TEXT_FIT) ;
  setGraphType ("DtAction", TEXT_FIT) ;
  setGraphType ("DtBiblio", TEXT_FULL_SCROLL) ;
  setGraphType ("DtColControl", TEXT_FIT) ;
  setGraphType ("DtAlign", TEXT_FULL_SCROLL) ;
  setGraphType ("DtCodons", TEXT_FIT) ;
  setGraphType ("DtDnaTool", TEXT_SCROLL) ;
  setGraphType ("DtGel", TEXT_FIT) ;
  setGraphType ("DtAlias", TEXT_FIT) ;
  setGraphType ("DtHistogram", TEXT_FIT) ;
  setGraphType ("OXGRID", TEXT_FULL_SCROLL) ;
  setGraphType ("PAIRMAP", TEXT_FULL_SCROLL) ;
  setGraphType ("O2MMAP", TEXT_FULL_SCROLL) ;
  setGraphType ("OxgridChoose", TEXT_FULL_SCROLL) ;
  setGraphType ("SPECGRID", TEXT_FULL_SCROLL) ;
#ifdef CHRONO
  setGraphType ("DtChrono", TEXT_SCROLL) ;
#endif /* CHRONO */

  return;
} /* setGraphTypes */


/************ end of file ****************/
