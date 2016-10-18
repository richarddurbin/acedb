/*  file: mainpick.c
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
 * Exported functions:
 * HISTORY:
 * Last edited: May 21 17:34 2009 (edgrif)
 * * May  8 14:41 2000 (edgrif): Added "full reindex" option to Read Models.
 * * Dec  3 14:42 1998 (edgrif): Change calls to new interface to aceversion.
 * * Oct 22 11:45 1998 (edgrif): Remove dec. of pickDraw, put in acedb.h
 * * Jun 11 1:38 1996 (rbrusk):
 *		-	WIN32 letting double pickPick() do its thing in pickCreate()
 * * Jun  6 17:44 1996 (rd)
 * * Jun 5 1:38 1996 (rbrusk):
 *	-	TEXT_ENTRY_FUNC_CAST added to classDisplay() and grepDisplay()
 *		callback arguments to graphTextScrollEntry()
 * * Jun 5 1:38 1996 (rbrusk):
 *	-	WIN32 port changes to 4.3
 * * May 10 21:13 1996 (rd)
 * * Sep 12 17:45 1992 (rd): in classDisplay() added quick check if
	        template is a valid object name (Alan's suggestion)
 * * Dec  6 13:28 1991 (mieg): autocompletion system
 * * Dec  2 20:59 1991 (mieg): if(isdisplayBlock) no autoPick
 *              because this create a textBound on wrong graphtype somehow ?
 * * Nov  5 18:28 1991 (mieg): call to keySetShow changed
 * Created: Tue Nov  5 18:26:56 1991 (mieg)
 * CVS info:   $Id: mainpick.c,v 1.168 2009-05-29 13:14:51 edgrif Exp $
 *-------------------------------------------------------------------
 */

/*  #define CHRONO to get Chrono in the admin menu */
#include <wh/acelib.h>
#include <wh/acedb.h>
#include <wh/display.h>
#include <wh/main.h>
#include <wh/bitset.h>
#include <wh/bindex.h>
#include <wh/lex.h>
#include <wh/pick.h>
#include <wh/client.h>
#include <wh/pref.h>
#include <wh/parse.h>
#include <wh/help.h>
#include <wh/model.h>
#include <wh/dna.h>		/* for dnaAnalyse */
#include <wh/dendrogram.h>		/* for readTreeFile et al. */
#include <wh/query.h>
#include <wh/session.h>
#include <wh/longtext.h>
#include <wh/dump.h>
#include <wh/querydisp.h>
#include <wh/sessiondisp.h>
#include <wh/tabledisp.h>
#include <wh/keysetdisp.h>
#include <wh/layoutdisp.h>
#include <wh/statusdisp.h>
#include <whooks/sysclass.h>
#include <wh/sigsubs.h>

/************************************************************/

enum {ACTIVITY_LENGTH = 28,				    /* Max length of activity status
							       indicator */
      TEMPLATE_LENGTH = 256,
      OLD_TEMPLATE_LENGTH = 20} ;			    /* Must be smaller than TEMPLATE_LENGTH */

typedef struct PickStruct {
  magic_t *magic ;  /* == &Pick_MAGIC */
  int    curr, currBox ;
  KEY    currClasse ;
  char   template[TEMPLATE_LENGTH] ;
  int    templateBox ;
  char*  grepText ;
  int    grepBox , longGrepBox , m1, m2, m3, modeBox1, modeBox2 ;
  BOOL   longGrep ;
  char   whatdoIdoText[ACTIVITY_LENGTH] ;
  int    whatdoIdoBox, classListBox ;
  Graph  mainGraph ;
  Graph  ksdisp_graph ;		/* graph of KeySet window */
  KEYSET oblist ;
  KeySetDisp ksdisp_handle ;
  int admin_box ;					    /* Need to dynamically change the menu. */
} *Pick ;

#define PICKGET(myname)     Pick pick ;\
                                       \
                          if (!graphAssFind (&Pick_MAGIC, &pick)) \
		            messcrash ("%s can't find pointer",myname) ; \
                          if (pick->magic != &Pick_MAGIC) \
                            messcrash ("%s received a wrong pointer",myname)


/************************************************************/
static void prefMainRedraw(BOOL pref_value) ;
static void mainStatusDisplay (char *text, void *user_pointer);
static void mainWindowResize (void);
static void mainWindowPick (int k, double x, double y, int modifier_unused);
static void mainWindowDestroy (void) ;
static void mainWindowReReadClasses (void); /* upon model-change */
static void mainWindowCleanUp (void) ;

static int pickClassComplete (char *cp, int len) ;
static BOOL classListMenuCall (KEY k, int box) ;
static void pickDoDrawOld (void);
static void pickDoDrawNew (void) ;
static void pickModeChoice1 (void) ;
static void exitButtonAction (void);
static void oldmenuSaveAction (void);
static void oldmenuWriteAccess (void);

static void oldmenuReadModels (void);
static void oldmenuReadModelsAndReindex(void) ;
static void doReadModels(BOOL complete_reindex) ;

static void obDisplay(Pick pick, BOOL show1, int state, BOOL self_destruct);
static BOOL classDisplay (void);
static BOOL classDisplayNew (int curr);
static void grepDisplay (char *entry);
static void grepDisplayNew (int curr);

static void serverOn (void);
static void serverOff (void);

static void swopSigCatchMenuText(char *sig_txt, int admin_box) ;
static void saveAndKeepWriteAccess(void) ;

static void saveCB(void *unused) ;
static void quitCB(void *unused) ;


/* these externs have to be removed and replaced with includes
   that declare the prototypes for these functions */
extern void
             oxgridCreate(void),
	     alignMaps(void), 
             updateData(void),
	     addKeys(void),
             editPreferences(void),
             queryCreate(void), 
             qbuildCreate(void), 
             qbeCreate(void), 
	     alignMaps(void);

#ifdef ACEMBLY
 extern void blyControl (void);
#endif /* ACEMBLY */

#ifdef DEBUG
extern void acedbtest(void);
#endif /* DEBUG acedbtest */

#ifdef CHRONO
extern void chronoShow(void);
#endif

#ifndef ACEMBLY
/* oxgrid not included in acembly */
extern void oxgridCreate(void) ;
extern BOOL oxgridPossible(void) ;
#endif /* !ACEMBLY */

/************************************************************/

static magic_t Pick_MAGIC = "Pick";

static Pick  mainPick = 0 ;

static int   firstClassBox = 0 ;
static Array classList = 0 ;	/* Array of FREEOPT */
static Array classLayout = 0 ;	/* Array of LAYOUT */
static BOOL isNewLayout = TRUE ;    /* mieg:  new layout */
static VoidRoutine mainWindowQuitRoutine = 0;

static char *pickFirstClass = 0;
static int   pickFirstBox = 0;
static char *pickFirstTemplate = 0;


/* long main menu for old-style main-window */

static MENU  mainMenu ;

MENUOPT mainMenuSpec[] = {
            { exitButtonAction,   "Quit"},
            { help  ,             "Help"},
	    { graphCleanUp,       "Clean up"},
            { statusDisplayCreate,"Program status"},
#ifdef ACEMBLY
	    { blyControl,         "Acembly"},
#else
            { updateData,         "Add Update File"},
	    { editPreferences,    "Preferences"},
#endif
            { menuSpacer,         ""},
#if defined(WIN32)
	    { 0,		  "Query"},
#endif
            { queryCreate,        "Query"},   
#if !defined(MACINTOSH)
            { qbeCreate,          "Query by Examples"},   
#endif
            { qbuildCreate,       "Query builder"},   
            { tableMaker,         "Table Maker"},
            { aqlDisplayCreate,   "AQL Query"},
#if defined(WIN32)
	    { 0,  	  0},	/* submenu trailer */
#endif
            { dnaAnalyse,         "DNA Analysis"},
            { menuSpacer,         ""},
#ifndef ACEMBLY
#if defined(WIN32)
	/* (rbrusk) July 17, 1998 - Dendrogram functions */
	    { 0,		  "Phylogenetic Trees"},
#endif
	    { readTaxonomicTree,  "Read Taxonomy File"},   
	    { readDNATree,        "Read DNA Tree File"},   
	    { readProteinTree,    "Read Protein Tree File"},   
#if defined(WIN32)
	    { 0,  	  0}, /* submenu trailer */
#endif
 /* option below added by jld */
            { oxgridCreate,       "Comparative Maps"},
#endif /* !ACEMBLY */
            { menuSpacer,         ""},
            { addKeys,            "Add-Alias-Rename"},
            { parseControl,       "Read .ace files"},
            { alignMaps,          "Align maps"},   
            { oldmenuReadModels,  "Read models"},
            { oldmenuReadModelsAndReindex,  "Read models and full reindex"},

            { dumpAll,            "Dump"}, 
            { dumpReadAll,        "Read Dump Files"}, 

	    { sessionControl,     "Session control"},
 
#ifdef CHRONO
	    { menuSpacer,         ""},
            { chronoShow,         "Chronometer"}, 
#endif /* CHRONO */

#ifdef DEBUG
	    { menuSpacer,         ""},
            { acedbtest,          "Test subroutine"},
#endif /* DEBUG acedbtest */

            { menuSpacer  ,       ""},
            { oldmenuWriteAccess, "Write Access"} ,
            { menuSpacer  ,       "" },
            { oldmenuSaveAction,  "Save" },
            { 0, 0 }		/* end of menu marker */
};

/************************************************************/

static MENUOPT pickNewMenu[] = {
  { help,	 "Help"},
  { graphPrint,	 "Print"},
#ifdef ACEMBLY
  { menuSpacer,	 ""},
  { blyControl,	 "Acembly"},
#endif
  { graphDestroy,"Exit"},
  { 0,0}
} ;

#ifndef ACEMBLY
/* OXGRIDs not included in Acembly */
static MENUOPT pickNewOxMenu[] = 
{
  { graphDestroy, "Exit"},
  { help,         "Help"},
  { graphPrint,   "Print"},
  { menuSpacer,   ""},
  { oxgridCreate, "Ox Grids"},
  { 0,0 }
} ;
#endif

/*************************************/

static FREEOPT queryButtonFreeMenu[] = 
{
  {8,"Query menu"},
  {'h',      "Help on Query"},
  {0, ""},
  { 'q',     "Query"},   
#if !defined(MACINTOSH)
  { 'b',     "Query by Examples"},  
#else  /* always keep same number of lines */
  {0, ""},
#endif
  { 'u',     "Query builder"},   
  { 's',     "Table Maker"},
  { 'a',     "AQL Query"},
  { 'd',     "DNA Analysis"},
} ;

/*************************************/

/* Stuff for toggling signal catching, default is to catch signals so that   */
/* acedb clear up is done.                                                   */
#define SIG_ON_TEXT  "Turn Signal Catching On"
#define SIG_OFF_TEXT "Turn Signal Catching Off"


static FREEOPT adminButtonFreeMenu[] = 
{
#if (defined CHRONO && defined DEBUG)
  {11, "Admin menu"},
#elif (defined CHRONO || defined DEBUG)
  {9, "Admin menu"},
#else
  {7, "Admin menu"},
#endif /* !CHRONO !DEBUG */
  {'s', "Program Status"},
  {'S', "Session Control"},
  {'p', "Preferences"},
  {'x', SIG_OFF_TEXT},
  { 0,  ""},
  {'d', "Dump All"},
  {'D', "Read Dump Files"},

#ifdef CHRONO
  {0,""},
  {'c', "Chronometer"},
#endif /* CHRONO */

#ifdef DEBUG
  {0,""}
  {'t', "Test subroutine"},
#endif /* DEBUG acedbtest */
} ;

/*************************************/

/* this menu is only displayed if write access is possible */
/* NOTE: 
 * (1) if you change the number of items adjust the
 *     integer in the first line to hold the new number of lines
 * (2) if you change the order of items be sure to adjust the
 *     code which fiddles the menu-items 'Gain write access'
 *     in pickDoDrawNew()! */
static FREEOPT editButtonFreeMenu[] = 
{
  {12,  "Edit menu"},
  {'u', "Add Update"},
  {'A', "Align maps"},
  {0,""},
  {'r', "Read .ace file"},
  {'a', "Add/Delete/Rename/Alias objects"},
  {0,""},
  {'T', "Create New Dendrogram"},
  {'O', "Comparative Maps"},
  {0,""},
  {'m', "Read Models"},
  {'M', "Read Models and full reindex"},
  {'w', "Gain write access"}
} ;

/*************************************/

static FREEOPT saveButtonFreeMenu[] = 
{
  {3, "Save menu"},
  {'s', "Save session and keep write access"},
  {'t', "Save session and lose write access"},
  {'d', "Drop write access (no save)"},
} ;


/******************************************************************
 ************************* public functions ***********************
 ******************************************************************/

void mainSetInitClass (char *initclass)
{
  pickFirstClass = strnew (initclass, 0);

  return;
} /* mainSetInitClass */

void mainSetInitTemplate (char *inittemplate)
{
  pickFirstTemplate = strnew (inittemplate, 0);

  return;
} /* mainSetInitTemplate */

/************************************************************/

VoidRoutine mainWindowQuitRegister (VoidRoutine func)
{
  VoidRoutine old = mainWindowQuitRoutine;

  if (func)
    mainWindowQuitRoutine = func;

  return old;
} /* mainWindowQuitRegister */

/****************************************************************/

/* These two short functions are application wide callbacks that get registered with graph
 * and are called from any window when the user presses Cntl-S for save or Cntl-Q for quit.
 * They get registered by pickCreate() and don't actually use the pointer passed in at the
 * moment. */
static void saveCB(void *unused)
{
  saveAndKeepWriteAccess() ;

  return ;
}

static void quitCB(void *unused)
{
  exitButtonAction() ;

  return ;
}




Graph pickCreate (void)
{
  BOOL selectFirst = !isNewLayout ;
  struct messContextStruct statusContext;
  GraphGlobalCallbacksStruct acedb_callbacks = {{NULL, NULL}} ;
 
  /* Init our general callbacks. */
  acedb_callbacks.save.func = saveCB ;
  acedb_callbacks.quit.func = quitCB ;

  writeAccessPossible();   /* establish access status, get
			      possible warning message out of the way */

  mainPick = (Pick) messalloc (sizeof(struct PickStruct));
  mainPick->magic = &Pick_MAGIC ;
  mainPick->grepText = (char*) messalloc (256) ; /* free'd in mainWindowDestroy */


  /* Register a callback with prefsubs so that when user changes main        */
  /* window style we get called.                                             */
  if (!prefSetFunc("OLD_STYLE_MAIN_WINDOW", (void *)prefMainRedraw))
    messcrash("Unable to set callback function for preference %s", "OLD_STYLE_MAIN_WINDOW") ;

  mainPick->mainGraph = displayCreate ("DtMain");

  graphAssociate (&Pick_MAGIC, mainPick) ;
  graphRegister (PICK, mainWindowPick) ;
  graphRegister (RESIZE, mainWindowResize) ;
  graphRegister (DESTROY, mainWindowDestroy) ;
  graphRegisterGlobalCallbacks(&acedb_callbacks) ;


  pickFirstBox = 0; /* gets set if pickFirstClass matches a class */  
  pickDraw() ;
  messfree (pickFirstClass);
  messfree (pickFirstTemplate); /* pickDraw gets called again sometimes */
  
  if (!pickFirstBox) 
    pickFirstBox  = 1 + firstClassBox ;    /* pick the first voc */
  else 
    selectFirst = TRUE;

  mainPick->curr = 0 ;


  if (selectFirst) /* srk - replace this with select if desired behaviour
		      is no keyset window _except_ when -initclass 
		      flag used */
    {
      /* fake a double click - call twice to achieve selection */
      mainWindowPick (pickFirstBox, 0, 0, 0) ;
      mainWindowPick (pickFirstBox, 0, 0, 0) ;
    }


  /* We have now drawn the mainwindow and the whatdoIdoBox,
   * so we can now register the routine that receives the 
   * status strings by messStatus calls anywhare in the code */
  statusContext.user_func = mainStatusDisplay;
  statusContext.user_pointer = (void*)mainPick;
  messStatusRegister (statusContext);
  

  /* We now have a main-window, which we can register to be
   * redrawn if write access changes and the buttons
   * and menus need to be redraw */
  writeAccessChangeRegister (pickDraw);

  /* change class menu when models change -> the new models
   * may have added a new class to the list so we need to update it */
  modelChangeRegister (mainWindowReReadClasses);


  graphActivate(mainPick->mainGraph) ;

  return mainPick->mainGraph ;
} /* pickCreate */

/************************************************************/

void pickDraw (void)
{ 
  Graph oldGraph ;
  char *first_part, *second_part ;
  char *window_title ;
  char *aceversion, *db_title, *db_name ;


  oldGraph = graphActive() ;

  if (!graphActivate (mainPick->mainGraph))
    messcrash ("pickDraw() called before pickCreate() !");

  {
    BOOL oldLayoutStyle = isNewLayout;
    isNewLayout = !prefValue ("OLD_STYLE_MAIN_WINDOW") ;
    if (oldLayoutStyle != isNewLayout)
      { 
	mainPick->whatdoIdoBox = 0;
	mainPick->currBox = 0 ;
	mainPick->curr = 0 ;
      }
  }


  /* Set up the main window title. */
  aceversion = aceGetVersionString() ;

  if (!(db_title = sessionDBTitle()))
    db_title = displayGetTitle("DtMain") ;

  db_name = sessionDbName() ;

  if ((first_part = graphGetTitlePrefix()))
    {
      first_part = hprintf(0, "%s", graphGetTitlePrefix()) ;

      second_part = hprintf(0, "%s, %s",
			    aceversion,
			    db_title) ;
    }
  else
    {
      first_part = hprintf(0, "%s, %s, %s",
			   aceversion,
			   db_title,
			   db_name ? db_name : "") ;

      if (thisSession.subDataRelease)
	second_part = hprintf(0, "%d-%d", thisSession.mainDataRelease, thisSession.subDataRelease) ;
      else
	second_part = hprintf(0, "%s", "") ;
    }
      
  window_title = hprintf(0, "%s %s", first_part, second_part) ;

  graphRetitlePlain(window_title) ;

  messfree(first_part) ;
  messfree(second_part) ;
  messfree(window_title) ;


  if (!classLayout)
    classLayout = layoutRead (0) ;

  classList = classListCreate (classList, TRUE, TRUE, FALSE, TRUE);

  if (isNewLayout) 
    pickDoDrawNew() ;
  else
    pickDoDrawOld() ;


   graphActivate (oldGraph) ;
} /* pickDraw */

/************************************************************/

void pickPopMain (void)
{
  if(graphActivate(mainPick->mainGraph))
    graphPop() ;
}

/************************************************************/

BOOL checkWriteAccess(void)
     /* interactive way to check, whether we have write access.
      * Used in by functions that need write access, but want to
      * enable the user to grab it, if we don't have it already,
      * and otherwise (when it returns FALSE) return from that func */
{ 
  if (isWriteAccess())
    return TRUE;

  if (writeAccessPossible())
    { 
      if (messQuery("You do not have Write Access, "
		    "shall I get it for you?"))
	{
	  /* try to get it */
	  if (sessionGainWriteAccess())
	    /* it worked */
	    return TRUE;
	  else
	    /* we couldn't get it */
	    return FALSE;
	}
      else
	/* didn't want to gain write access */
	return FALSE;		
    }

  
  /* impossible to get write access */
  messout("Sorry, you cannot gain Write Access");
  return FALSE;
} /* checkWriteAccess */

/************************************************************/

void mainApplyNewLayout (Array newLayout)
     /* accept new layout from layoutEditor and re-draw */
{
  arrayDestroy (classLayout) ;
  classLayout = newLayout;
  
  pickDoDrawNew() ;

  return;
} /* mainApplyNewLayout */

/************************************************************/

void mainKeySetComplete (KEYSET ks, char *text, int len, BOOL self_destruct)
     /**** Auto completion system *********/
{
  int n, i ;
  char *cp, *cq ;
  KEYSET ks2 ;
  
  if (!keySetMax(ks))		/* no match */
    { messbeep() ;
      goto ok ;
    }
  else if (keySetMax(ks) == 1)	/* unique match */
    { strncpy (text, name(keySet(ks,0)), len-1) ;
    goto ok ;
    }
				/* else multiple matches */
  n = strlen(text) ;
  for (i = 0 ; i < n ; ++i)
    text[i] = 0 ;

  ks2 = keySetAlphaHeap(ks,keySetMax(ks)) ;
  cp = name (keySet (ks2, 0)) ;
  cq = name (keySet (ks2, keySetMax(ks2)-1)) ;
  keySetDestroy(ks2) ;

  for (i = 0 ; *cp && *cp == *cq && i < len-1 ; ++i, ++cp, ++cq)
    text[i] = *cp ;
  text[i] = 0 ;

  if (i > n)		/* length increase */
    goto ok ;
 
ok:
  if (mainPick)
    { Graph oldGraph = graphActive() ;
      
      if (ks != mainPick->oblist) /* probably a useless kludge */
	{ keySetDestroy (mainPick->oblist) ;
	  mainPick->oblist = ks ;
	}
      if (graphActivate (mainPick->mainGraph))
	obDisplay(mainPick, FALSE, 3, self_destruct) ; /* no automatic display() if 1 */
    
      graphActivate (oldGraph) ;
    }
}

/************************************************************/

/* return number of possible completions */
int ksetClassComplete (char *text, int len, int classe, BOOL self_destruct)
{
  static KEYSET ks = 0 ;
  static int previousClass = 0 ;
  static int myKeySetId = 0 ;
  static char buffer[80] ;
  KEYSET ks2 ;
  char *cp, *cq ;
  int i, j , n ;

  if (myKeySetId != keySetExists(ks))
    ks = 0 ;
  if (!text)
    { keySetDestroy(ks) ;
      return 0 ;
    }

  if (ks)
    {
      if (previousClass != classe)
	keySetDestroy(ks) ;
      else
	{     /* check if search field just got longer */
	  cp = buffer ; cq = text ; n = 80 ; /* size of buffer */
	  while (--n && *cp && *cq == *cp){cp++; cq++;} 
	  if (!n || *cp)
	    keySetDestroy (ks) ;
	}
    }
  
  strncpy(buffer, text, 79) ;
  if (!ks)
    {
      if (class(classe) == _VClass)
	ks = query (0, messprintf ("FIND %s IS \"%s*\"", 
				   name(classe), text)) ;
      else			/* assume a true classe */
	ks = query (0, messprintf ("FIND %s IS \"%s*\"", 
				   pickClass2Word(classe), text)) ;
    }
  else
    { 
      ks2 = arrayCreate(keySetMax(ks), KEY) ;
      text[len-2] = 0 ;		/* prevents running off end */
      cp = text + strlen(text) ;
      *cp = '*' ; *(cp+1) = 0 ;
      for (i=0, j=0 ; i < keySetMax(ks) ; i++ )
	if (pickMatch (name(keySet(ks,i)), text))
	  keySet(ks2,j++) = keySet(ks,i) ;
      *cp = 0 ;			/* remove * from end */
      keySetDestroy(ks) ;
      ks = ks2 ;
    }

  mainKeySetComplete(ks, text, len, self_destruct) ;
    
  myKeySetId = keySetExists(ks) ;
  previousClass = classe ;
  strncpy(buffer, text, 79) ;

  return keySetMax(ks) ;
} /* ksetClassComplete */

/************************************************************/

Array classListCreate (Array oldList, 
		       BOOL listHidden,
		       BOOL listVisible, 
		       BOOL listBuried,
		       BOOL listAllTypes) /* if FALSE must have model */
{ int max, i, j ;
  KEYSET ks, ksa ;
  Array newList;
  Stack s = stackCreate(50);
  BOOL arg = FALSE;
  unsigned char mask ;
  KEY key, model, table ;

  catText(s, "FIND Class");

  if (listHidden)
    { catText(s, " HIDDEN"); arg = TRUE; }

  if (listVisible)
    { if (arg) catText(s, " OR");
      catText(s, " VISIBLE"); arg = TRUE;
    }

  if (listBuried)
    { if (arg) catText(s, " OR");
      catText(s, " BURIED");
    }

  ks = queryLocalParametrized(0, stackText(s, 0), "");

  stackDestroy(s);

  max = keySetMax (ks) ;
  ksa = keySetAlphaHeap (ks, max) ;
  keySetDestroy (ks) ;

  if (oldList)
    /* replace old array */
    newList = arrayReCreate (oldList, max, FREEOPT) ;
  else
    newList = arrayCreate(max, FREEOPT);

  j = 1 ;
  for (i = 0 ; i < max ; i++)
    { 
      key = table = keySet(ksa, i) ;
      pickIsA (&table, &mask) ;
      if (listAllTypes ||
	  (pickList [table].type == 'B' && 
	   (model = pickList [table].model) && iskey(model) == 2))
	{
	  arrayp(newList, j, FREEOPT)->key = key ;
	  arrayp(newList, j, FREEOPT)->text = name(key) ;
	  j++ ;
	}
    }

  arrayp(newList, 0, FREEOPT)->key = j - 1 ;
  arrayp(newList, 0, FREEOPT)->text = "Classes" ;

  keySetDestroy (ksa) ;
  
  return newList;
} /* classListCreate */


void mainPickReport (int n)
{
  BOOL bb = FALSE ;
  float line = .5 ;
  static int boxOn = 0, boxOff ; 
  char *cp = 0 ;
  if (!externalServerState)
    return ;

  switch (n)
    {
    case -1: /* ask and draw */
      bb = externalServerState (-1) ; /* ask */
      graphText ("Server:", .5, line) ;
      boxOn = graphButton ("On", serverOn, 8.5, line) ;
      boxOff = graphButton ("Off", serverOff, 12.0, line) ;
      break ;
    case 1:
      bb = TRUE ;
      break ;
    case 0: case 2:
      bb = FALSE ;
      break ;
    default:
      break ;
    }
  if (!boxOn) return ;

  if (externalServerName) 
    cp = externalServerName () ;
  if (cp)
    graphText(cp, 18, line);
  
  graphBoxDraw(boxOn, BLACK, bb ? PALEBLUE : WHITE) ;
  graphBoxDraw(boxOff, BLACK, !bb ? (n == 2 ? RED : PALEBLUE) : WHITE) ;      
} /* mainPickReport */


/******************************************************************
 ************************ private functions ***********************
 ******************************************************************/

/* Callback function, called from prefsubs when user changes style of main   */
/* window.                                                                   */
static void prefMainRedraw(BOOL pref_value)
{
  pickDraw() ;

  return ;
}


static void mainWindowResize (void)
{   
  isNewLayout = !prefValue ("OLD_STYLE_MAIN_WINDOW") ;
  if (!isNewLayout) 
    {
      arrayDestroy (classLayout) ;
      classLayout = 0;		/* force a re-create */
    }
  pickDraw() ;

  return;
} /* mainWindowResize */

static void mainWindowDestroy (void)
{ 
  Graph g = graphActive() ;
  struct messContextStruct nullStatusContext = { NULL, NULL };
  PICKGET("obDestroy") ;

  /* prevent any more status display, as this window is dying */
  messStatusRegister (nullStatusContext);

  /* cannot allow any more drawing into the main window */
  writeAccessChangeUnRegister ();

  /*******************************/

  if (graphActivate(pick->ksdisp_graph))
    graphDestroy () ;

  if (graphActivate(pick->mainGraph))
    graphDestroy () ;

  graphActivate (g) ;
  graphAssRemove (&Pick_MAGIC) ;

  pick->magic = 0 ;
  if (pick->grepText) 
    messfree (pick->grepText);

  messfree (pick) ;
  mainPick = 0 ;

  return;
} /* mainWindowDestroy */

/************************************************************/

static void mainWindowPick (int k, double x, double y, int modifier_unused)
{ 
  PICKGET("mainWindowPick") ;
  
  if (k == pick->templateBox)
    { graphCompletionEntry (pickClassComplete, pick->template,0,0,0,0) ; 
      return ;
    }

  if (!k) 
    {
      if (pick->currBox)
	graphBoxDraw (pick->currBox, BLACK, WHITE) ;
      pick->curr = 0 ; pick->currBox = 0 ;
      return ;
    }

  if (k == pick->grepBox)
    {
      if (pick->currBox)
	graphBoxDraw (pick->currBox, BLACK, WHITE) ;
      pick->curr = 0 ; pick->currBox = 0 ;
      graphTextScrollEntry (pick->grepText,0,0,0,0,0) ;
    }
  else if (k == pick->classListBox)
    { KEY key ;
      if (graphSelect (&key, arrp (classList, 0, FREEOPT)))
	classListMenuCall (key, 0) ;
    }

  if (k > firstClassBox && k <= firstClassBox + arrayMax(classLayout))
    { 
      
      pickModeChoice1() ;
      if (k != pick->currBox)
	{ if (pick->currBox)
	    graphBoxDraw (pick->currBox, BLACK, WHITE) ;
	  pick->currBox = k ;
	  if (isNewLayout)graphBoxDraw (pick->currBox, BLACK,PALEBLUE) ;
	  else graphBoxDraw (pick->currBox, WHITE, BLACK) ;
	  pick->curr = k - firstClassBox ;
	  graphCompletionEntry (pickClassComplete,pick->template,0,0,0,0) ;
	}
      else if (!classDisplay () && strcmp (pick->template,"*"))
	{ 		/* inappropriate template - display whole list */
	  strcpy (pick->template,"*") ;
	  graphCompletionEntry (pickClassComplete,pick->template,0,0,0,0) ;
	  classDisplay () ;
	}
    }

  return;
} /* mainWindowPick */

static void pickLongGrep (void)
{    
  PICKGET("pickLong") ;
  
  pick->longGrep = TRUE ;
  grepDisplay("dummy");

  return;
} /* pickLongGrep */

/************************************************************/

static BOOL classListMenuCall (KEY classe, int box)
{
  char *cp ;
  KEY key ;
  KEYSET ob ;
  BOOL resul ;
  char *nom ;
  int n, state = 1, ii , i, j ;
  PICKGET ("classListMenuCall") ;

  pick->currClasse = classe ;
  pick->oblist = keySetCreate() ; j = 0 ;
lao:
  /* classe is a real Classe-key in _VClass */
  for (ii = 0 ; ii < 256 ; ii++)
    {
      if (!classe && (pickType(ii) == 'X' || pickList[ii].visible == 'h' || !pickList[ii].name))
	continue ;

      if (classe) 
	{ ii = classe ; nom = name(classe) ; }
      else
	nom = className(ii << 24) ;

      if (!strcmp("*", pick->template))
	{
	  if (classe && lexword2key (pick->template, &key, classe))
	    { 
	      ob = keySetCreate () ;
	      keySet(ob, 0) = key ;
	    }
	  else
	    { cp = messprintf( "FIND %s", nom) ;
	    if (externalServer) 
	      { 
		ob = externalServer (200, cp, 0, FALSE) ;
	      if (keySetMax(pick->oblist) == 200)
		messout("I limited the importation to 200, type ** to get all %s(s)", name(classe)) ;
	      }
	    else
	      ob = query(0, cp) ;
	    }
	}
      else
        ob = query(0, messprintf( "FIND %s IS %s",nom,
				  freeprotect(pick->template))) ;

      n = keySetMax(ob) ;
      for (i = 0 ; i < n ; i++)
	{
	  keySet(pick->oblist,j++) = keySet(ob,i) ;
	}

      if (externalServer && !strcmp("*", pick->template) && j >= 200)
	{ 
	  messout("I limited the importation to 200, type ** to get all %s(s)", nom) ;
	  break ;
	}

      if (classe)
	break ;
    }
 
  if (state == 1 && !keySetMax(pick->oblist))
    {
      cp = pick->template ;
      n = strlen(cp) ;
      if (n == 0 || (*(cp + n - 1) != '*' && n < (OLD_TEMPLATE_LENGTH - 1)))
	{
	  strcat (pick->template, "*") ;
	  graphBoxDraw (pick->templateBox, -1, -1) ;
	  state = 2 ;
	  goto lao ;
	}
    }
  resul = (keySetMax(pick->oblist) != 0) ;
  obDisplay (pick, TRUE, state, FALSE) ;

  return resul ;
} /* classListMenuCall */



/***********************************/

static void mainWindowCleanUp (void)
{
  graphCleanUp() ;
  pickDraw() ;
}

/*************************************/

static void queryButtonFreeAction (KEY key, int box)
{
  switch (key)
    {
    case 0: break ;
    case 'h': helpOn ("Query") ; break ;
    case 'q': queryCreate() ; break ;
    case 'b': qbeCreate() ; break ;
    case 'u': qbuildCreate() ; break ;
    case 's': tableMaker() ; break ;
    case 'a': aqlDisplayCreate() ; break ;
    case 'd': dnaAnalyse() ; break ;
    }
}

static void querySelector (void)
{
  KEY k ;
  
  if (!graphSelect (&k,queryButtonFreeMenu))
    return ;
  queryButtonFreeAction(k,0) ;
}

/***********************************/

static void adminButtonFreeAction (KEY key, int box)
{
  PICKGET("adminButtonFreeAction") ;

  switch (key)
    {
    case 0: break ;
    case 's': statusDisplayCreate(); break ;
    case 'S': sessionControl() ; break ;
    case 'p': editPreferences() ; break ;
    case 'x':
      {
	/* Toggle signal handling on/off.                                    */
	char *sig_txt ;

	if (signalCatchStatus())
	  {
	    sig_txt = SIG_ON_TEXT ;
	    signalCatchOff(FALSE) ;
	  }
	else
	  {
	    sig_txt = SIG_OFF_TEXT ;
	    signalCatchOn(FALSE) ;
	  }
	swopSigCatchMenuText(sig_txt, pick->admin_box) ;
	
	break ;
      }
    case 'd': dumpAll() ; break ;
    case 'D': dumpReadAll() ; break ;
#ifdef CHRONO
    case 'c': chronoShow () ; break ;
#endif
#ifdef DEBUG
    case 't': acedbtest() ; break ;
#endif /* DEBUG acedbtest */
    }
} /* adminButtonFreeAction */

static void adminSelector(void)
{
  KEY k ;
  
  if (graphSelect(&k,adminButtonFreeMenu))
    adminButtonFreeAction(k,0) ;

  return ;
} /* adminSelector */

/***********************************/

static void editButtonFreeAction(KEY key, int box)
{
  switch (key)
    {
    case 0: break ;
    case 'r':			/* Read .ace file */
      if (checkWriteAccess())
	parseControl(); 
      break ;

    case 'a':			/* Add/Delete/Rename/Alias */
      if (checkWriteAccess())
	addKeys() ;
      break ;

    case 'A':			/* Align maps */
      if (checkWriteAccess())
	alignMaps() ;
      break ;

    case 'w': 
      sessionGainWriteAccess();	/* Gain Write Access */
      /* will re-draw if write access status changes */
      break;

    case 'T':                    /* Dendrogram trees */
      dendrogramDisplay (0,0,0, NULL) ;
      break;

    case 'O':                    /* Oxgrid */
      oxgridCreate() ;
      break;

    case 'm':
      doReadModels(FALSE) ;				    /* Just read models. */
      break ;
    case 'M':
      doReadModels(TRUE) ;				    /* Read models + full reindex. */
      break ;

    case 'u':
      updateData() ;
      break ;
    }
  pickDoDrawNew() ;
} /* editButtonFreeAction */

static void editSelector(void)
{
  KEY k ;
  
  /*   graphEvent (RIGHT_DOWN, 3,3) ; */
  if (!graphSelect (&k,editButtonFreeMenu))
    return ;
  editButtonFreeAction(k,0) ;
}

/***********************************/

static void saveAndKeepWriteAccess(void)
     /* used by save button freemenu and the save button itself */
{
  if (isWriteAccess())
    {


#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
      /* This results in the log file being closed, this is a temporary fix
       * as the logfile v.s. session close stuff needs sorting out. */

      sessionClose (TRUE);	/* close and save session,
				   nothing will happen, 
				   if we don't have write access */
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */
      sessionMark (TRUE, 0) ;
      

      if (!sessionGainWriteAccess()) /* re-gain write access */
	messout ("Sorry, it was not possible to re-gain write access "
		 "after saving the last session.");
    }
  else
    {
      messout ("You don't have write access. Nothing to save.");
    }
  
} /* saveAndKeepWriteAccess */

/***********************************/

static void saveAndLoseWriteAccess(void)
     /* used by save button freemenu */
{
#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
  /* This results in the log file being closed, this is a temporary fix
   * as the logfile v.s. session close stuff needs sorting out. */
  
  sessionClose (TRUE) ;		/* close and save session,
				   nothing will happen, 
				   if we don't have write access */
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */
  sessionMark (TRUE, 0) ;

} /* saveAndLoseWriteAccess */

/***********************************/

static void saveMenuFreeAction(KEY key, int box)
{
  switch (key)
    {
    case 's':
      saveAndKeepWriteAccess();
      break ;

    case 't':
      saveAndLoseWriteAccess();
      break ;

    case 'd':
      sessionReleaseWriteAccess(); /* don't save, drop write access */
      break;

    default:
      break;
    }
} /* saveMenuFreeAction */

/***********************************/

static void exitButtonAction (void)
{
  if (thisSession.session != 1 &&
      !messQuery ("Do you really want to quit acedb?"))
    return;

  if (mainWindowQuitRoutine)
    (*mainWindowQuitRoutine)();
  else
    {
      /* default action that needs to be performed when xace exits */

      graphCleanUp () ;  /* kills all open windows,
			    forces displayed objects to cache */
      
      writeAccessChangeUnRegister(); /* avoid unnecessary re-draws
					just before exiting */

      /* release read/write locks and clean up temp-files 
	 is all done by aceQuit() */
      if (isWriteAccess() && 
	  messQuery ("You did not save your work, should I ?"))
	sessionClose (TRUE);
      else
	sessionClose (FALSE);
      
      if (!getenv("ACEDB_NO_BANNER"))
	printf ("\n\nA bientot !\n") ;
    }
  exit (EXIT_SUCCESS);
} /* exitButtonAction */

/***********************************/

static void serverOn (void)
{
  BOOL bb=FALSE;
  if (externalServerState)
    bb = externalServerState (1) ; /* set */
  mainPickReport (bb ? 1 : 2) ;
}

static void serverOff (void)
{
  BOOL bb=FALSE;
  if (externalServerState)
    bb = externalServerState (0) ; /* unset */
  mainPickReport (bb ? 1 : 0) ;
}

/************************************************************/
/****************  old main menu ****************************/
/************************************************************/

static void oldmenuSaveAction (void)
{
  if (isWriteAccess())
    { BOOL reGet = messQuery("Shall I retain Write Access after saving?");
      sessionClose (TRUE) ;
      if (reGet)
	sessionGainWriteAccess();
    }
  else
    sessionGainWriteAccess () ; /* try to grab it */
} /* menuSaveAction */


static void oldmenuWriteAccess (void)
{
  /* can't use this function directly in menu definition,
     it returns a boolean success value (ignored here) */
  sessionGainWriteAccess();
} /* menuWriteAccess */

static void oldmenuReadModels (void)
{
  doReadModels(FALSE) ;

  return ;
} /* menuReadModels */

static void oldmenuReadModelsAndReindex (void)
{
  doReadModels(TRUE) ;

  return ;
} /* menuReadModels */

/* Just calling readModels will not do a complete reindex, this sometimes    */
/* seems to cause problems because queries start to not work correctly and   */
/* do not find everything they should. The problem seems to go away after a  */
/* complete reindex which takes longer. This code follows that in command.c  */
/* for the "Read-models" command. So now users can do the complete reindex   */
/* from xace as well as tace.                                                */
static void doReadModels(BOOL complete_reindex)
{
  if (complete_reindex)
    {
      if (messQuery("A complete reindex can take a long time for a large database, "
		    "during this time no one else will be able to update the database. "
		    "do you wish to continue ?"))
	bIndexInit(BINDEX_AFTER_READMODELS) ;
    }
  else
    readModels() ;

  return ;
}


static void pickDoDrawOld (void)
{
  int box, i, y ; float line;
  LAYOUT *z ;

  if (!graphActivate (mainPick->mainGraph))
    return ; 
  graphClear() ;

  if (!mainMenu)
    {
      mainMenu = menuInitialise ("Main menu", (MENUSPEC*)mainMenuSpec) ;
      
      if (!writeAccessPossible())
	{
	  menuSuppress (mainMenu, "Save") ;
	  menuSuppress (mainMenu, "Write Access") ;
	  menuSuppress (mainMenu, "Add-Alias-Rename") ;
	}
    }

  if (writeAccessPossible())
    {
      if (isWriteAccess())
	{
	  menuSetFlags (menuItem (mainMenu, "Write Access"), MENUFLAG_DISABLED) ;
	  menuUnsetFlags (menuItem (mainMenu, "Save"), MENUFLAG_DISABLED) ;
	}
      else 
	{
	  menuUnsetFlags (menuItem (mainMenu, "Write Access"), MENUFLAG_DISABLED) ;
	  menuSetFlags (menuItem (mainMenu, "Save"), MENUFLAG_DISABLED) ;
	}
    }
  graphNewMenu (mainMenu);


  /* drawing */
  line = 0.5 ;
  if (externalServerState)
    { mainPickReport (-1) ; line += 2 ; }
  graphText ("Search: ", 0.5, line) ;

  box = graphButton("Help",help,35.,line) ;
  graphBoxMenu (box, 0) ;

  box = graphButton("Exit",exitButtonAction,41.,line) ;
  graphBoxMenu (box, 0) ;

  if (pickFirstTemplate)
    strcpy(mainPick->template, pickFirstTemplate);
  else
    strcpy (mainPick->template,"*") ;
  mainPick->templateBox = graphCompletionEntry(pickClassComplete, mainPick->template, 
					       OLD_TEMPLATE_LENGTH, 12.0, line, 
					       TEXT_ENTRY_FUNC_CAST classDisplay) ;
  line += 1.5 ;

  strcpy(mainPick->whatdoIdoText, "");
  mainPick->whatdoIdoBox = graphBoxStart() ;
  graphTextPtr(mainPick->whatdoIdoText, 18, line, ACTIVITY_LENGTH) ;
  graphBoxEnd() ;

  line += .5 ;
  graphText ("In Class:", 0.5, line) ;
  mainPick->classListBox = box = graphMenuTriangle(TRUE, 9.7, line) ;
  graphBoxFreeMenu(box, (FreeMenuFunction) classListMenuCall, 
		   arrp (classList, 0, FREEOPT) ) ;
  line += 1 ;
  y = 0 ;
  firstClassBox = graphBoxStart () ;
  for (i = 0 ; i < arrayMax(classLayout) ; ++i)
    { int b = graphBoxStart () ;
      z = arrp(classLayout, i, LAYOUT) ;
      if (z->y > y) y = z->y ;
      graphText (name(z->key), z->x, z->y + line) ;
      if (pickFirstClass
	  && (strcasecmp(name(z->key), pickFirstClass) == 0))
	{
	  pickFirstBox = b;
	}
      graphBoxEnd () ;
    }
  graphBoxEnd () ;
  graphNewBoxMenu(firstClassBox, mainMenu);

  line += y + 2 ;

  graphBoxStart() ;
  graphText ("Global Search: ", 0.5, line) ;
  graphBoxEnd () ;

  strcpy (mainPick->grepText, "");
  mainPick->grepBox = graphTextScrollEntry (mainPick->grepText,
					    255, 16, 14.5, line,
					    grepDisplay) ; 
  if (lexMax(_VLongText) > 2)
    mainPick->longGrepBox = graphButton ("Long Search",
					 pickLongGrep, 32.,line) ;

  messStatus ("dummy");	/* force redraw of whatdoIdoBox */

  graphRedraw () ;
} /* pickDoDrawOld */



static void pickModeChoice (int m1, int m2, int m3)
{ 
  int i ;

  if (!isNewLayout)
    return;

  if (m1)
    { 
      if (mainPick->m1 == m1) { m1-- ; if (!m1) m1 = 1 ; }
      if (!m2)
	{ if (m1 == 1) m2 = 1 ; else m2 = 2 ; }
      i = 3 ; while (i--) 
	graphBoxDraw (mainPick->modeBox1 + i + 1, BLACK, WHITE) ;    
      mainPick->m1 = m1 ;
      i = m1 ; while (i--)
	graphBoxDraw (mainPick->modeBox1 + i + 1, BLACK, PALEBLUE) ;
    }
  if (m2)
    {
      graphBoxDraw (mainPick->modeBox2 + mainPick->m2, BLACK, WHITE) ;    
      mainPick->m2 = m2 ;
      graphBoxDraw (mainPick->modeBox2 + mainPick->m2, BLACK, PALEBLUE) ;
      if (m2 == 2)
	mainWindowPick (0, 0, 0, 0) ;
    }

  return;
} /* pickModeChoice */

static void pickModeChoice10 (void) { pickModeChoice(1,0,1) ; }
static void pickModeChoice20(void) { pickModeChoice(2,0,3) ; }
static void pickModeChoice30(void) { pickModeChoice(3,0,7) ; }
static void pickModeChoice1 (void) { pickModeChoice(0,1,0) ; }
static void pickModeChoice2 (void) { pickModeChoice(0,2,0) ; }
/*
static void pickModeChoice11 (void) { pickModeChoice(1,1,1) ; }
static void pickModeChoice12 (void) { pickModeChoice(1,2,2) ; }
static void pickModeChoice22 (void) { pickModeChoice(2,2,3) ; }
static void pickModeChoice32 (void) { pickModeChoice(3,2,4) ; }
*/

static void modeSearchEntry (char *dummy)
{  
  mainPick->longGrep = FALSE ;

  switch (10 * mainPick->m1 + mainPick->m2)
    {
    case 11: /* names, selected class */ 
      if (!mainPick->curr)
	{
	  messout("Please select a class or search \"in all classes\"") ;
	  return ;
	}
      classDisplayNew (mainPick->curr) ;
      break ;
    case 12: /* names, all classes */
      classDisplayNew (0) ;
      break ;
    case 21: /* text, selected class */  
      if (!mainPick->curr)
	{ 
	  messout("Please select a class or search \"in all classes\"") ;
	  return ;
	}
      strcpy (mainPick->grepText, mainPick->template) ;
      grepDisplayNew (mainPick->curr) ;
      break ;
    case 22: /* text, all classes */  
      strcpy (mainPick->grepText, mainPick->template) ;
      grepDisplayNew (0) ;
      break ;
    case 31:
    case 32: /* long texts */
      mainPick->longGrep = TRUE ;
      strcpy (mainPick->grepText, mainPick->template) ;
      grepDisplayNew (0) ;
      break ;
    }
}

static void otherClassChoice (void)
{
  KEY key ;
  if (graphSelect (&key, arrp (classList, 0, FREEOPT)))
    classListMenuCall (key, 0) ;
}


/************************************************************/


static void pickDoDrawNew (void)
{
  int box, i, y ;
  float line;
  LAYOUT *z ;
  int graphWidth, graphHeight ;

  if (!graphActivate (mainPick->mainGraph))
    return ; 
  graphClear() ;

#ifndef ACEMBLY  
  if (oxgridPossible())
    graphMenu (pickNewOxMenu) ;
  else
#endif
    /* background menu */
    graphMenu (pickNewMenu) ;

  graphFitBounds (&graphWidth,&graphHeight) ;

  /* drawing */
  line = 0.5 ;
  mainPick->curr = 0 ; mainPick->currBox = 0 ;

  /* server/client */
  if (externalServerState)
    { mainPickReport (-1) ; line += 2 ; }

  /* menu bar */
  if (writeAccessPossible())
    {
      if (isWriteAccess())
	{ 
	  box = graphButton("Save..", saveAndKeepWriteAccess, 1., line) ;
	  graphBoxFreeMenu (box, saveMenuFreeAction,
			    saveButtonFreeMenu) ;

	  /* exclude last menu option "Gain write access" */
	  editButtonFreeMenu[0].key = 11;
	}
      else
	{
	  /* include "gain write access" */
	   editButtonFreeMenu[0].key = 12;
	}

      box = graphButton("Edit..", editSelector,9.,line) ;
      graphBoxFreeMenu (box, editButtonFreeAction,
			editButtonFreeMenu) ;
    }

  box = graphButton("Query..", querySelector,17.,line) ;
  graphBoxFreeMenu (box, queryButtonFreeAction, queryButtonFreeMenu) ;

  mainPick->admin_box = box = graphButton("Admin..", adminSelector,26.,line) ;
  swopSigCatchMenuText((signalCatchStatus() ? SIG_OFF_TEXT : SIG_ON_TEXT), box) ;
  graphBoxFreeMenu (box, adminButtonFreeAction, adminButtonFreeMenu) ;

  box = graphButton("Clear", mainWindowCleanUp,37.,line) ;
  graphBoxMenu(box, 0);

  box = graphButton("Help", help, 44.,line + 1.4) ;
  graphBoxMenu(box, 0);

  box = graphButton("Exit", exitButtonAction, 44.,line) ;
  graphBoxMenu(box, 0);

  line += 1.5 ;

  /* status */
  *mainPick->whatdoIdoText = 0 ;
  graphText ("Status:", 1, line) ;
  mainPick->whatdoIdoBox = graphBoxStart() ;
  graphTextPtr(mainPick->whatdoIdoText, 10, line, ACTIVITY_LENGTH) ;
  graphBoxEnd() ;
  line += 1.8 ;

  /* search box */


  /*
  strcpy (mainPick->template,"*") ;
    mainPick->templateBox = graphCompScrollEntry (pickClassComplete, mainPick->template, 
					    250, 28,10.0, line, 
					    TEXT_ENTRY_FUNC_CAST modeSearch) ;
					    */
  graphText ("Search", 1, line +.1) ;
  mainPick->m1 = 1 ; mainPick->m2 = 1 ; mainPick->m3 = 1 ; 

  mainPick->modeBox2 = graphBoxStart () ;  
  box = graphButton ("in the selected class", pickModeChoice1, 8,line) ;
  graphBoxMenu(box, 0);
  graphText ("or", 31, line +.1) ;
  box = graphButton ("in all classes", pickModeChoice2, 34,line) ;
  graphBoxMenu(box, 0);
  graphBoxEnd () ;
  graphBoxDraw (mainPick->modeBox2 + mainPick->m2, BLACK, PALEBLUE) ;

  line += 1.6 ;
  mainPick->modeBox1 = graphBoxStart () ;
  graphText ("objects", .5, line + .1) ;
  box = graphButton ("named", pickModeChoice10, 8,line) ;
  graphBoxMenu(box, 0);
  graphText ("or", 14.9, line + .1) ;
  box = graphButton ("related to", pickModeChoice20, 17.9,line) ;
  graphBoxMenu(box, 0);
  if (lexMax(_VLongText) > 2)
    {
      /* graphText ("+",31.2, line) ;  */
      box = graphButton ("even in long texts", pickModeChoice30, 30.2,line) ;
      graphBoxMenu(box, 0);
    } 
  else { graphBoxStart () ;  graphBoxEnd () ;}/* empty box needed for counts */

 graphBoxEnd () ;
  i = mainPick->m1 ;
  while (i--)
    graphBoxDraw (mainPick->modeBox1 + i + 1, BLACK, PALEBLUE) ;

  line += 1.5 ;
   
  if (pickFirstTemplate)
    strcpy(mainPick->template, pickFirstTemplate);
  else
    strcpy (mainPick->template,"*") ;
  mainPick->templateBox = graphCompScrollEntry (pickClassComplete, mainPick->template, 
					    250, 30,10.0, line, modeSearchEntry) ;


  mainPick->grepBox = mainPick->templateBox; /* in new style window both boxes
				      * in one */
  mainPick->longGrepBox = 0;    /* don't have this box in new style window */

  line += 1.5 ;
  /* class list */
  graphLine (0,line, graphWidth, line) ; line += .7 ;

  graphText ("Select a class:", 0.5, line) ;

  box = graphButton ("Other class ...", otherClassChoice, 22, line - .1);
  graphBoxFreeMenu (box,(FreeMenuFunction) classListMenuCall,
		    arrp (classList, 0, FREEOPT)) ;

  box = graphButton ("Selection", layoutOpenEditor, 39, line - .1) ;
  graphBoxMenu(box, 0);

  line += 1.3 ;
  y = 0 ;
  firstClassBox = graphBoxStart () ;
  for (i = 0 ; i < arrayMax(classLayout) ; ++i)
    { int b = graphBoxStart () ;
      z = arrp(classLayout, i, LAYOUT) ;
      if (z->y > y) y = z->y ;
      graphText (name(z->key), z->x, z->y + line) ;
      if (pickFirstClass
	  && (strcasecmp(name(z->key), pickFirstClass) == 0))
	{
	  pickFirstBox = b;
	}
      graphBoxEnd () ;
    }
  graphBoxEnd () ;

  /* the first classbox that contain the class-chooser boxes forms
     part of the background as well, so attach the bg-menu as well */
#ifndef ACEMBLY  
  if (oxgridPossible())
    graphBoxMenu (firstClassBox, pickNewOxMenu) ;
  else
#endif
    graphBoxMenu (firstClassBox, pickNewMenu) ;

  line += y + 2 ;

  *(mainPick->grepText) = 0 ;

  messStatus ("Ready");	/* force redraw of whatdoIdoBox */

  graphRedraw () ;
} /* pickDoDrawNew */



/************************************************************/

static void mainWindowReReadClasses (void)
     /* called when models change */
{
  arrayDestroy (classLayout) ;
  classLayout = layoutRead (0);

  classList = classListCreate (classList,
			       TRUE, /* listHidden */
			       TRUE, /* listVisible */
			       FALSE, /* !listBuried */
			       TRUE); /* listAllTypes */

  return;
} /* mainWindowReReadClasses */

/************************************************************/

static void mainStatusDisplay (char *text, void *user_pointer)
/* registered using mainActivityRegister() as soon as the
 * mainGraph is properly drawn and the whatDoIdoBox is a valid box */
{
#ifndef MACINTOSH
  /* non-MACINTOSH */
  Pick pick = (Pick)user_pointer ;
  static char *old = "dummy" ; /* some impossible value */
  Graph hold ;

  if (!pick)
    /* we don't have a mainWindow yet */
    return ;

  if (pick->magic != &Pick_MAGIC)
    messcrash("messStatus func mainStatusDisplay() - received in valid "
	      "user_pointer");

  if (!text && !old)
    /* we're asking to reset, but it is already in 'Ready' state,
     * so we have nothing more to do in this function */
    return;

  hold = graphActive() ;
  graphActivate(pick->mainGraph) ;

  if (!text)
    { old = 0 ;
      strcpy(pick->whatdoIdoText, "Ready") ;
      graphBoxDraw(pick->whatdoIdoBox,BLACK,PALEGREEN) ;
    }
  else if (text != old)
    { old = text ;
      strncpy(pick->whatdoIdoText, text, ACTIVITY_LENGTH-1) ;
      if (pick->whatdoIdoBox)
	graphBoxDraw(pick->whatdoIdoBox,BLACK,RED) ;
    }
  
  graphActivate(hold) ;
#else
  /* MACINTOSH */
  extern void StartAsyncSpinning (short period) ;
  extern void StopAsyncSpinning (void) ;

  static char *old = "dummy" ; /* some impossible value */

  if (old != text )
  {
    if (!text)
      { old = 0 ;
        StopAsyncSpinning () ;
      }
    else if (text != old)
      { old = text ;
        StartAsyncSpinning (8) ;
      }
  }
#endif /* MACINTOSH */
} /* mainStatusDisplay */


/* state = 1 - template unchanged
 *         2 - added a star at end
 *         3 - autocomplete
 */
/* then switch to the display graph (make if necessary) */
static void obDisplay (Pick pick, BOOL show1, int state, BOOL self_destruct)
{
  char *message;
  BOOL auto_display ;

  /* default message */
  if (pick->currClasse)
    message = messprintf("%s:%s",name(pick->currClasse),pick->template) ;
  else
    message = messprintf("All classes:%s",pick->template) ;

  switch (state)
    {
    case 1: 
      if (!keySetMax(pick->oblist))
	{
	  if (pick->currClasse)
	    message = messprintf("No %s:%s %s",
				 name(pick->currClasse),
				 pick->template, 
				 "is the class correct ? ") ;
	  else
	    message = messprintf("No object named %s in any class", 
				 pick->template) ;
	}
      break ;

    case 2: 
      if (keySetMax(pick->oblist))
	{
	  if (pick->currClasse)
	    message =  messprintf("%s:%s %s",
				  name(pick->currClasse),
				  pick->template, 
				  "No exact match, * appended") ;
	  else
	    message =  messprintf("%s %s",
				  pick->template, 
				  "No exact match in any class, * appended") ;
	}
      else
	{
	  if (pick->currClasse)
	    message = messprintf("No %s:%s %s",
				 name(pick->currClasse),
				 pick->template, 
				 " is the class correct ? ") ;
	  else
	    message = messprintf("No object named %s in any class",
				 pick->template) ;
	}
      break ;

    case 3:
      if (keySetMax(pick->oblist) == 0)
	message = "No possible completion in this class" ;
      break;

    case 4:
      message = 
	"To search text please specify a search string \n" 
	"  with at least 3 letters or digits.\n"
	"Authorized wild chars are\n"
	" * (any string), ? (single char)." ;
      break;

    case 5: 
      if (!keySetMax(pick->oblist))
	{ state = 4 ; message = messprintf("%s Not found",pick->grepText) ; }
      else
	message = messprintf("Search %s",pick->grepText) ;
      break ;
    case 6: 
      if (!keySetMax(pick->oblist))
	{
	  state =  4 ;
	  message = messprintf("%s Not found",pick->grepText) ;
	}
      else
	message = messprintf("Long Search %s",pick->grepText) ;
      break ;

    case 7: 
      if (!keySetMax(pick->oblist))
	{ 
	  state = 4 ; 
	  message = messprintf("Text %s Not found in any class",
			       pick->grepText) ; }
      else
	message = messprintf("Search %s in all classes",pick->grepText) ;
      break ;

    case 8: 
      if (!keySetMax(pick->oblist))
	{ 
	  state =  4 ; 
	  message = messprintf("%s Not found in any class",
			       pick->grepText) ;
	}
      else
	message = messprintf("Long Search %s in all classes",
			     pick->grepText) ;
      break ;

    case 9: 
      message = messprintf("%s not found hit in selected class, \n"
			   "searched all classes (no hit)",
			   pick->grepText) ;
      break ;

    case 10: 
      message = messprintf("%s not found in selected class, \n"
			   "long-searched all classes (no hit)",
			   pick->grepText) ;
      break ;
    }

  switch (state)
    {
    case 5: 
    case 6: 
    case 7: 
    case 8:
      {
	/* costly search, reply in new window */

	displayCreate("DtKeySet");
	graphRetitle(message) ;
	keySetMessageShow (pick->oblist,0,message, FALSE) ;
	break ;
      }
    default:
      {
	/* fast search, reply in main window */
	if (graphExists (pick->ksdisp_graph))
	  keySetMessageShow (pick->oblist, pick->ksdisp_handle, message, self_destruct) ;
	else 
	  { 
	    pick->ksdisp_graph = displayCreate("DtKeySet");
	    graphRetitle("Main KeySet") ;
	    pick->ksdisp_handle = keySetMessageShow(pick->oblist,0,message, self_destruct) ;
	  }
	break ;
      }
    }


  /* User can set preference to prevent automatic display of keysets where there is a single
   * object. This is useful for superlinks/chromosomes and the like where display is v. slow
   * or impossible due to memory constraints. */
  auto_display = prefValue("AUTO_DISPLAY") ;
  if (auto_display && keySetMax(pick->oblist) == 1 && show1)
    { 
      KEY key = keySet(pick->oblist,0) ;
      
      display(key, 0, keySetGetShowAs (pick->ksdisp_handle, key));
    }

  pick->oblist = 0 ;		/* since given to keySetShow */

#if !defined(WIN32)  /* The WIN32 user interface is nicer
			if I don't do this */
   graphActivate (pick->mainGraph) ;
#endif
}

/**************** text action **********************/

static BOOL classDisplay (void)  /* displays current option list */
{ 
  char *cp ;
  PICKGET("classDisplay") ;
  
  if (!pick->curr)
    { messout("Please pick a class") ;
      return FALSE ;
    }

  cp = pick->template ;
  while (*cp == ' ') ++cp ;
  if (!*cp)
    { strcpy (pick->template, "*") ;
      graphBoxDraw (pick->templateBox, -1, -1) ;
    }

  graphCompletionEntry (pickClassComplete,pick->template,0,0,0,0) ;

  return classListMenuCall
    (arrp(classLayout,pick->curr-1,LAYOUT)->key, 0) ;
} /* classDisplay */

/**************** text action **********************/

static BOOL classDisplayNew (int curr)  /* displays current option list */
{ 
  char *cp ;
  PICKGET("classDisplay") ;

  cp = pick->template ;
  while (*cp == ' ') ++cp ;
  if (!*cp)
    { strcpy (pick->template, "*") ;
      graphBoxDraw (pick->templateBox, -1, -1) ;
    }

  graphCompletionEntry (pickClassComplete,pick->template,0,0,0,0) ;

  return classListMenuCall
    (curr ? arrp(classLayout,curr-1,LAYOUT)->key : 0, 0) ;
} /* classDisplayNew */

/*************************************************/

static void grepDisplayNew (int curr)
{
  KEYSET ob1 = 0, ob2 = 0, ob3 = 0;
  int n , state ;
  static int confirm = 0 ;
  char *cp,*cq ;
  static char localText[256] , lastText[256] ;
  PICKGET("grepDisplay") ;
  
/* first check that the search text is non-trivial */

  for (n = 0, cp = pick->grepText ; *cp ; ++cp)
    if (isprint((int)*cp) && *cp != '*' && *cp != '?')
      ++n ;
  if (n < 3)
    {
      /*
	messout ("To search text please specify a search string " 
	       "with at least 3 letters or digits."
	       "   Authorized wild chars are\n"
	       "* (any string), ? (single char).") ;
	       */
      
      pick->longGrep = FALSE ;
      if (pick->longGrepBox)
	graphBoxDraw(pick->longGrepBox, BLACK, WHITE) ;
      pick->oblist = keySetCreate () ;
      obDisplay (pick, FALSE, 4, FALSE) ;
      return ;
    }

/* make localText with a '*' in front of and behind the grep text */
  
  cp = pick->grepText ;
  if (*cp == '*')
    cq = localText ;
  else
    { *localText = '*' ;
      cq = localText+1 ;
    }
  while ((*cq++ = *cp++)) ;
  if (cq[-2] != '*')
    { cq[-1] = '*' ; *cq = 0 ; }

/* If the text has not changed, wait for confirmation */
  if(!strcmp(localText,lastText))
    if(!confirm++)
      { 
	if (pick->longGrepBox)
	  graphBoxDraw(pick->longGrepBox, BLACK, WHITE) ;
	return ;
      }
  strncpy(lastText,localText,255) ;
  strncpy(pick->grepText,localText,255) ;
  confirm = 0 ;

/* then do the search */
  graphBoxDraw(pick->grepBox,BLACK,RED) ;
  if (pick->longGrep && pick->longGrepBox)
    graphBoxDraw(pick->longGrepBox, BLACK, YELLOW) ;
  if (externalServer)
    ob1 = externalServer (0, 0, localText, FALSE) ;
  else 
    ob1 = queryGrep(0, localText) ;
  if (pick->longGrep)
    { 
      graphBoxDraw(pick->grepBox,BLACK,YELLOW) ;
      if (pick->longGrepBox)
	graphBoxDraw(pick->longGrepBox, BLACK, RED) ;
      ob2 = longGrep(localText) ;
      ob3 = keySetOR(ob1, ob2) ;
    }
  else
    { ob3 = ob1 ; ob1 = 0 ; }

  keySetDestroy(ob1) ;
  keySetDestroy(ob2) ; 

  graphBoxDraw(pick->grepBox,BLACK,YELLOW) ;
  state = pick->longGrep ? 6 : 5 ;
  pick->longGrep = FALSE ;
  if (pick->longGrepBox)
    graphBoxDraw(pick->longGrepBox, BLACK, WHITE) ;

  if (keySetMax(ob3) && curr) /* filter by single class */
    {
      int n2 = keySetMax(ob3), n3 = 0 ;
      KEY *kp2, *kp3 ;
      KEY cc = 	arrp(classLayout, curr-1, LAYOUT)->key ;
     
      /* it is better to filter locally than to query, becausee of client/server */
      ob2 = arrayCopy (ob3) ;
      kp2 = arrp(ob2, 0, KEY) ; kp3 = arrp(ob3, 0, KEY) ;
      while (n2--)
	{
	  if (lexIsInClass(*kp2, cc))
	    { *kp3++ = *kp2 ; n3++ ;}
	  kp2++ ;
	}
      keySetMax(ob3) = n3 ;
      if (n3)
	keySetDestroy (ob2) ;
      else
	{
	  state += 4 ; /* report enlarged search */
	  keySetDestroy (ob3) ;
	  ob3 = ob2 ; ob2 = 0 ;
	}
    }
  else
    state += 2 ; /* report general failure */
  pick->oblist = ob3 ;
  obDisplay (pick, FALSE, state, FALSE) ;
}

static void grepDisplay (char *entry)
{  grepDisplayNew(0) ; }

/**************** pick Action ************/

static int pickClassComplete (char *cp, int len)
{ 
  int i;
  PICKGET("pickClassComplete") ;

  if (!pick->curr)
    return 0 ;

  i = ksetClassComplete (cp, len, arrp(classLayout, pick->curr-1, LAYOUT)->key, FALSE) ;

  return i;
} /* pickClassComplete */

/* disgusting menu stuff...should use the new menu package....       */
static void swopSigCatchMenuText(char *sig_txt, int admin_box)
{
  int i = 0 ;

  while (adminButtonFreeMenu[i].key != 'x')
    i++ ;

  adminButtonFreeMenu[i].text = sig_txt ;

  graphBoxFreeMenu(admin_box, adminButtonFreeAction, adminButtonFreeMenu) ;

  return ;
}


/****************************************/
/****************************************/

