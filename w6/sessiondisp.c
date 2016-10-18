/*  File: sessiondisp.c
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
 * Description: interactive session control
 * Exported functions:
 *              void sessionControl(void)
 * HISTORY:
 * Last edited: May  6 10:27 2003 (edgrif)
 * * Dec  3 14:45 1998 (edgrif): Change calls to new interface to aceversion.
 * * Nov 23 16:57 1998 (fw): functions moved here from session.c
 * Created: Mon Nov 23 16:57:22 1998 (fw)
 *-------------------------------------------------------------------
 */

/************************************************************/

#include "acedb.h"
#include "aceio.h"
#include "lex.h"
#include "lex_sess_.h"
#include "whooks/systags.h"	/* for _This_session tag */
#include "bs.h"
#include "disk.h"
#include "disk_.h"   /* defines BLOCKHEADER */
#include "session.h"
#include "session_.h"		/* uses BLOCKHEADER */

#include "display.h"
#include "main.h"

/************************************************************/
#define SESS_COL_READLOCKED ORANGE
#define SESS_COL_PERMANENT  GREEN
#define SESS_COL_SELECTED   RED
#define SESS_COL_KEPT_ALIVE YELLOW
#define SESS_COL_DESTROYED  LIGHTGRAY
#define SESS_COL_CURRENT    LIGHTBLUE

static void sessDispDraw(void);
static void sessDispRefresh(void);
static void sessDispDestroy(void);
static void sessDispSetStartSession(void);
static void sessDispFixSession(void);
static void sessDispUnFixSession(void);
static void sessDispDestroyLastSession(void);
static void sessDispToggleDeadShown (void);
static void sessDispPick(int box, double x, double y, int modifier_unused);
/************************************************************/

static Graph sessionSelectionGraph = 0;
static KEY sessionChosen = 0;
static Array sessionTree = 0;
static int selectedSessionBox = 0 ;
static Array box2sess = 0;	/* arrayp of ST */
static BOOL IsDeadSessionShown = FALSE;

static MENUOPT sessionMenu[] = {
  { graphDestroy,		"Quit"},
  { help,			"Help"},
  { menuSpacer,			""},
  { sessDispRefresh,		"Refresh Display"},
  { menuSpacer,			""},
  { sessDispSetStartSession,	"Start Session"},
  { sessDispFixSession,		"Fix Session"},
  { sessDispUnFixSession,	"Unfix Session"},
  { sessDispDestroyLastSession,	"Destroy Last Session"}, 
  { sessDispToggleDeadShown,	"Show dead sessions"},
  { 0,0 }
} ;
/************************************************************/



/*************************************************************/
/**************** Interactive session control ****************/
/*************************************************************/

void sessionControl(void)
{ 
  sessionChosen = 0;

  if (graphActivate(sessionSelectionGraph))
    graphPop() ;
  else
    sessionSelectionGraph = displayCreate("DtSession");

  sessionChangeRegister (sessDispDraw);

  sessDispDraw() ;
} /* sessionControl */

/*************************************************************/

static void sessDispDestroy(void)
{ 
  sessionChangeRegister (0);

  sessionSelectionGraph =  0 ;
  arrayDestroy(box2sess); 
  box2sess = 0;

  if (arrayExists(sessionTree))
    {
      sessionTreeDestroy (sessionTree);
      sessionTree = 0;
    }

  return;
} /* sessDispDestroy */

/*************************************************************/

static void sessDispSetStartSession(void)
{ 
  if(!sessionChosen)
   { messout("First pick a live session") ;
     return ;
   }

  if (isWriteAccess())
    { messout("You still have write access\n"
	      "First save your previous work") ;
      return ;
    }
  
  if (writeAccessPossible() && 
      !messQuery ("By starting again in a previous session "
		  "you will lose the ability to re-gain write access, "
		  "even by going back to the current session.\n"
		  "Continue ?"))
    return;
		  


  pickPopMain() ;
  graphCleanUp () ;		/* kill all but main graph */

  sessionDoClose () ;		/* To empty all caches */
  sessionForbidWriteAccess();	/* can never gain write access again */
  
  lexClear() ;

  sessionStart(sessionChosen);	/* link up the current session to
				   the selected session */
  sessionInitialize ();

  sessionControl();		/* re-display session control
				   it was killed by graphCleanUp */

  return;
} /* sessDispSetStartSession */

/************************************************************/  

static void stBoxDraw (int box, ST *st, BOOL isSelected)
{
  if (st->isDestroyed && st->isReadlocked)
    {
      int i;
      int b = box + 1;
      BOOL toggle = TRUE;
      
      for (i = 0; i < st->len; i++)
	{
	  if (isSelected)
	    graphBoxDraw(b++, BLACK, SESS_COL_SELECTED);
	  else
	    {
	      graphBoxDraw(b++, BLACK, toggle ? SESS_COL_READLOCKED : SESS_COL_DESTROYED);
	      toggle = toggle ? FALSE : TRUE;
	    }
	}
    }
  else
    {
      if (isSelected)
	graphBoxDraw(box, BLACK, SESS_COL_SELECTED);
      else
	{
	  int box_colour;

	  /* colour the box according to the status of the session */
	  if (st->isReadlocked)
	    box_colour = SESS_COL_READLOCKED;
	  else if(st->isPermanent) 
	    box_colour = SESS_COL_PERMANENT;
	  else if (st->isDestroyed)
	    box_colour = SESS_COL_DESTROYED;
	  else
	    box_colour = SESS_COL_KEPT_ALIVE;
	  
	  if (st->key == _This_session)
	    box_colour = SESS_COL_CURRENT;

	  graphBoxDraw(box, BLACK, box_colour);
	}
    }

  return;
} /* stBoxDraw */



static void sessDispPick(int box, double x, double y, int modifier_unused)
{ 
  ST *st = NULL ;

  if (!box2sess)
    messcrash ("No boxes drawn yet for sessDispPick");

  /* redraw the old box in its original colours */
  if (selectedSessionBox)
    {
      st = arrp(box2sess, selectedSessionBox, ST);
      stBoxDraw(selectedSessionBox, st, FALSE);
    }

  if (box == 0)
    {
      /* click on background unselects the currently selected box */
      sessionChosen = 0;
      selectedSessionBox = 0;
      return;
    }

  /* box is not the background */

  st = arrayp(box2sess, box, ST); /* use of arrayp protects against
				     boxnumbers going beyond the 
				     bos2sess index */

  if (st->number)		/* clicked on a session object box */
    {
      if (!st->isDestroyed && st->key != _This_session)
	sessionChosen = st->key ;
      else
	sessionChosen = 0 ;
      
      /* draw the selection red */
      stBoxDraw(box, st, TRUE);

      /* double click displays the session object */
      if (box == selectedSessionBox && st->key != 1)
	display(st->key, 0, 0) ;
      
      selectedSessionBox = box ;
    }
  else
    {
      /* we clicked on a box that wasn't a session object's box */
      sessionChosen = 0 ;
      selectedSessionBox = 0;
    }

  return;
} /* sessDispPick */

/************************************************************/  
/*
 this would allow complete versioning, by removing te
 isOtherSesssion blocking of write access, but it
is both too complex to be useful, and bugged in
a complex way when destroying former sessions on various barnches

it may be usefull however to recover from a fatal crash
*/
static void sessDispDestroyLastSession (void)
{ KEY father, grandFather ;
  ST *st, *st1 ;
  int i ;
  OBJ Session ;
  int  a1 = 0, a3 = 0, b3 = 0, c3 = 0, as = 0 ;
  BLOCK theSuperBlock ;

  if (isWriteAccess())
    { messout("You still have write access\n"
	      "First save your previous work") ;
      return ;
    }
 
  if (!I_own_the_passwd("destroy a session"))
    return ;

  father = thisSession.from ; /* only allowed murder */
  if (!father)
    { messout ("First select a living session.") ;
      return ;
    }

  /***** find the predecessor of the last live session 
	 (grandFather of thisSession) *******************/

  if (sessionTree)
    sessionTreeDestroy(sessionTree);  
  sessionTree = sessionTreeCreate(FALSE);

  for (i = 0, st = arrp(sessionTree,0,ST);
       i < arrayMax(sessionTree) ; st++, i++)
    if (st->key == father)
      break ;

  if (i >= arrayMax(sessionTree))
    { 
      messout("Failed sorry") ;
      sessionTreeDestroy (sessionTree);
      sessionTree = 0;
      return ;
    }

  grandFather = st->father ;
  for (i = 0, st1 = arrp(sessionTree,0,ST);
       i < arrayMax(sessionTree) ; st1++, i++)
    if (st1->key == st->father)
      break ;

  if (i >= arrayMax(sessionTree))
    { 
      messout("Internal inconsistency\n"
	      "operation failed");
      sessionTreeDestroy (sessionTree);
      sessionTree = 0;
      return ;
    }

  if (!grandFather || !st1 || iskey(st1->key) != 2 || st1->isDestroyed)
    { messout ("You cannot destroy the last session, "
	       "because the previous one is dead") ;
      sessionTreeDestroy (sessionTree);
      sessionTree = 0;
      return ;
    }

  sessionTreeDestroy (sessionTree);
  sessionTree = 0;

  /***** found grandFather ******/

  if (!messQuery(messprintf("%s%s%s%s",
 "Do you really want to destroy the last session,\n",
 "this is an irreversible operation which is designed to help ",
 "recover from some rare fatal disk errors\n",
 "It is probably not useful in other cases."))) 
    return ;

  if (!messQuery(messprintf("%s%s%s",
 "Are you really sure ?\n\n",
 "If you proceed, acedb will destroy the last session, exit, ",
 "and you will have to restart the program.")))
      return ;

  if (!checkWriteAccess())
    return ;

  /* On we go, no possible recovery ! */
  messcrashroutine = simpleCrashRoutine ;  
  sessionForbidWriteAccess(); /* so we can never re-gain write access,
				 clears the readlock and
				 redraws if nec to change buttons etc. */

  Session = bsCreate(grandFather) ;
  if(!Session)
    messcrash("sessionDestroy() - Sorry, I cannot find grandfather '%s'",
	      name(grandFather));

  if(!bsGetData(Session,_GlobalLex,_Int,&a1) ||
     !bsGetData(Session,_Session,_Int,&as)||
     !bsGetData(Session,_VocLex,_Int,&a3) ||
     !bsGetData(Session,_bsRight,_Int,&b3)  ||
     !bsGetData(Session,_bsRight,_Int,&c3)  ||
         /* Hasher only introduced in release 1.4  !bsGetData(Session,_bsRight,_Int,&h0)  || */
     !bsGetData(Session,_CodeRelease,
	    _Int,&thisSession.mainCodeRelease) ||
     !bsGetData(Session,_bsRight,
		_Int,&thisSession.subCodeRelease) ||
     !bsGetData(Session,_DataRelease,
	    _Int,&thisSession.mainDataRelease) ||
     !bsGetData(Session,_bsRight,
	    _Int,&thisSession.subDataRelease) ) 
    messcrash("Anomaly while recovering info from previous session, sorry") ;

  bsDestroy(Session) ;

  /* proceed to change the superblock */
  /* decrement session number */

#ifdef ACEDB4      
  diskblockread (&theSuperBlock,1) ;
#else
  aDiskReadSuperBlock(&theSuperBlock);
#endif

  theSuperBlock.gAddress = thisSession.gAddress = a1 ;
#ifdef ACEDB4
  if (!theSuperBlock.mainRelease)
    theSuperBlock.alternateMainRelease = aceGetVersion() ;
  else
#endif
  theSuperBlock.mainRelease = aceGetVersion() ;
  theSuperBlock.subCodeRelease = aceGetRelease() ;

#ifdef ACEDB5   
  theSuperBlock.session =  as ;
#else
  theSuperBlock.h.session =  as ;
#endif
  diskWriteSuperBlock(&theSuperBlock) ;	/* writes and zeros the BATS */
  messdump ("DESTROYED SESSION %d %s\n", 
	    thisSession.session-1, timeShow(timeNow())) ;
  messout("Last session saved, I will now exit, please restart acedb") ;

  filtmpcleanup () ;		/* deletes any temporary files made */
				/* NOTE, readlocks already cleared */
  exit (0) ;
} /* sessDispDestroyLastSession */

/****************************************************************/

static void sessDispFixSession(void)
{ 
  OBJ Session ;
  char *cp = 0 ;
  ST *st;
  ACEIN name_in;

  st = arrp (box2sess, selectedSessionBox, ST);

  if (!st->number)
    {
      messout ("No session selected.");
      return;
    }

  if (st->isDestroyed)
    {
      messout ("Cannot fix a dead session.") ;
      return ;
    }

  if (st->isPermanent)
    { messout ("Selected session is already permanent.") ;
      return ;
    }

  if (iskey(sessionChosen)!=2)	/* check that it is still in a 
				   cache or on disk */
    { 
      messout ("Cache inconsistency:\n"
	       "This session no longer exists!") ;
      sessDispDraw() ;		/* re-draw */
      return ;
    }

  if (!checkWriteAccess())	/* will output a message */
    return ;

  /* I save once here to let the system a chance to crash if need be */
  saveAll() ; 
  Session = bsUpdate(sessionChosen) ;

  if (bsFindTag(Session, _Destroyed_by_session))
    { 
      messerror("selected session should not be dead!") ;
      bsDestroy(Session) ;
      sessDispDraw() ;
      return ;
    }

  if (st->number != 1 && !bsFindTag(Session, _Up_linked_to))
    /* for the first session it is OK not to have an uplink */
    { 
      messout("I cannot rename a session created before code 1-6") ;
      bsDestroy(Session) ;
      return ;
    }

  cp = 0 ;
  bsGetData(Session, _Session_Title,_Text,&cp) ;
  
  name_in = messPrompt("Rename this session and make it permanent",
		       cp ? cp : "","t", 0);
  if (name_in)
    {
      if ((cp = aceInWord(name_in)))
	{
	  if(strlen(cp) > 80)
	    *(cp + 80) = 0 ;
	  if (strlen(cp) > 0)
	    {
	      bsAddTag(Session, _Permanent_session) ;
	      bsAddData(Session, _Session_Title, _Text, cp) ;
	    }
	}
      aceInDestroy (name_in);
    }
  bsSave(Session) ;		/* also destroys the OBJ */
  /* This save will rewrite Session to disk without altering the BAT */
  saveAll() ;

  sessDispRefresh() ;
} /* sessDispFixSession */

/****************************************************************/

static void sessDispUnFixSession(void)
{ 
  OBJ Session ;
  ST *st;

  st = arrp (box2sess, selectedSessionBox, ST);

  if (!st->number)
    {
      messout ("No session selected.");
      return;
    }

  if (st->isDestroyed)
    { 
      messout ("Cannot unfix a dead session.");
      return;
    }

  if (!st->isPermanent)
    { 
      messout ("Can only unfix a permanent session.");
      return;
    }

  if (st->number == 1)
    {
      messout ("Cannot unfix the first session.");
      return;
    }

  if (st->isReadlocked)
    { 
      messout ("Somebody is still working on this session\nCannot unfix this session now.");
      return;
    }

  if (iskey(sessionChosen)!=2)	/* check that it is still in a 
				   cache or on disk */
    { 
      messout ("Cache inconsistency:\n"
	       "This session no longer exists!") ;
      sessDispDraw() ;		/* re-draw */
      return ;
    }

  if (!checkWriteAccess())	/* will output a message */
    return ;

  /* I save once here to let the system a chance to crash if need be */
  saveAll() ; 
  Session = bsUpdate(sessionChosen) ;

  /* this check should not be necessary, but we better make sure,
     in case the database was updated under our feet or so */
  
  if (!bsFindTag(Session, _Destroyed_by_session))
    {
      if (bsFindTag(Session,_Permanent_session))
	{
	  /* remove the tag that was accessed last - the permanent-tag */
	  bsRemove(Session);
	  bsSave(Session);	/* also destroys the OBJ */
	  /* This save will rewrite Session to disk 
	     without altering the BAT */
	  saveAll() ; 

	  st->isPermanent = FALSE;
	}
      else
	{			/* oops, it is no longer permanent */
	  messerror("selected session should be permanent!") ;
	  bsDestroy(Session) ;
	}
    }
  else
    {				/* oops it has since been destroyed */
      messerror("selected session should not be dead!") ;
      bsDestroy(Session) ;
    }

  sessDispRefresh() ;

  return;
} /* sessDispUnFixSession */

/****************************************************************/

static void sessDispToggleDeadShown (void)
{
  if (IsDeadSessionShown)
    {
      IsDeadSessionShown = FALSE;
      sessionMenu[9].text = "Show dead sessions";
    }
  else
    {
      IsDeadSessionShown = TRUE;
      sessionMenu[9].text = "Hide dead sessions";
    }
  sessDispDraw() ;
} /* sessDispToggleDeadShown */

/****************************************************************/

static void sessDispDrawBranch(Array sessionTree, ST* st, 
			       Array xStart, Array xEnd, int y0)
{ 
  int i;
  int treeMax = arrayMax(sessionTree) ;
  ST *st1, *stnew ;
  int box;

  /* Draw self */
  if (IsDeadSessionShown || !st->isDestroyed || st->isReadlocked)
    {
      int gen = st->generation;

      box = graphBoxStart() ;
      stnew = arrayp(box2sess, box, ST);
      memcpy (stnew, st, sizeof(ST));

      if(!array(xStart, gen, int))
	/* nothing drawn on level `g' yet */
	{
	  if (gen < 2)
	    /* start at the left edge */
	    array(xStart, gen, int) = 4 ;
	  else
	    /* start below the rightmost box of the previous generation */
	    array(xStart, gen, int) = array(xStart, gen-1, int) ;
	}
      else
	/* something drawn at that level yet */
	{
	  /* start this box just right of the end of the
	     rightmost box at that level */
	  array(xStart, gen, int) = array(xEnd, gen, int) + 2 ;
	}

      array(xEnd, gen, int) = array(xStart, gen, int) + st->len;

      st->x = array(xStart, gen, int) ;
      st->y = y0 + 3*gen ;

      if (st->isDestroyed && st->isReadlocked)
	{
	  char letter[2];
	  int b;

	  letter[1] = 0;
	  for (i = 0; i < st->len; i++)
	    {
	      letter[0] = st->title[i];
	      b = graphBoxStart();
	      graphText(letter, st->x + i, st->y);
	      graphBoxEnd();
	      graphBoxSetPick(b, FALSE);
	    }
	  
	  graphBoxEnd() ;
	}
      else
	{
	  graphText(st->title, st->x, st->y);
	  graphBoxEnd() ;
	}

      stBoxDraw(box, st, FALSE);

      /* draw the connecting lines */
      for (i = 0, st1 = arrp(sessionTree,0,ST); i < treeMax; st1++, i++)
	if ((st->ancester == st1->key) && st1->x && st1->y &&
	    (IsDeadSessionShown || !st1->isDestroyed || st1->isReadlocked))
	  graphLine(st1->x + st1->len/2, st1->y + 1.0, 
		    st->x + st->len/2 ,  st->y - .0) ;
    }

  /* Recursion */
  for (i = 0, st1 = arrp(sessionTree, treeMax - 1,ST); 
       i < treeMax; st1--, i++)
     if (st1->sta == st)
       sessDispDrawBranch(sessionTree, st1, xStart, xEnd, y0) ;

  return;
} /* sessDispDrawBranch */

/****************************************************************/

static void sessDispRefresh()
{
  if (sessionTree)
    sessionTreeDestroy(sessionTree);  
  sessionTree = sessionTreeCreate(TRUE);

  sessDispDraw();
  
  return;
} /* sessDispRefresh */

static void sessDispDraw() 
{
  int i, max = 0 , maxGen = 0 , n, y0 ;
  int box;
  ST *st ;
  Array levelSessBoxStarts = arrayCreate(12, int) ;
  Array levelSessBoxEnds = arrayCreate(12, int) ;

  if (!sessionTree)
    sessionTree = sessionTreeCreate(TRUE);

  messStatus ("Drawing sessions");

  if (!graphActivate(sessionSelectionGraph))
    messcrash ("sessDispDraw() - Can't activate graph.");
      
  graphClear() ;
  graphRegister(DESTROY, sessDispDestroy) ;
  graphRegister(RESIZE, sessDispDraw) ;
  graphRegister(PICK,  sessDispPick) ;

  graphMenu(sessionMenu) ;

  selectedSessionBox = 0;
  sessionChosen = 0;

#ifdef RESIZE_NOT_WORKING_ON_TEXT_FULL_SCROOL
  {
    float screenw, screenh, fw, fh, winx, winy, winw, winh ;
    int winlength;

    graphScreenSize (&screenw, &screenh, &fw, &fh, 0, 0) ;
    graphWindowSize(&winx, &winy, &winw, &winh);

    winlength = winw / screenw * fw; /* in user-coordinates */

    graphButtons(sessionMenu, 3.0, 2.0, winlength-2) ;
  }  
#else
  graphButtons(sessionMenu, 3.0, 2.0, 50) ;
#endif

  y0 = 9 ;
  graphText("Double click on any previous session for text info.", 3, y0++) ;
  graphText("Restart from any live session (in read only mode)", 3, y0++) ;
  graphText("Fix any live session, to prevent its future destruction", 3, y0++) ;
  y0++ ;
  graphText("Colours: ", 3, y0) ;

  box = graphBoxStart ();
  graphText("live (permanent)", 12, y0);
  graphBoxEnd();
  graphBoxDraw(box, BLACK, SESS_COL_PERMANENT);

  box = graphBoxStart ();
  graphText("live (still in use)", 31, y0);
  graphBoxEnd();
  graphBoxDraw(box, BLACK, SESS_COL_READLOCKED);

  y0 += 2;

  box = graphBoxStart ();
  graphText("live (kept alive)", 12, y0);
  graphBoxEnd();
  graphBoxDraw(box, BLACK, SESS_COL_KEPT_ALIVE);

  box = graphBoxStart ();
  graphText("dead", 31, y0);
  graphBoxEnd();
  graphBoxDraw(box, BLACK, SESS_COL_DESTROYED);

  box = graphBoxStart ();
  graphText("selected", 37, y0++);
  graphBoxEnd();
  graphBoxDraw(box, BLACK, SESS_COL_SELECTED);

  graphPop() ;

  /* maps box numbers to session tree objects */
  box2sess = arrayReCreate(box2sess, 50, ST);

  n = arrayMax (sessionTree) ;
  for (i = 0, st = arrp(sessionTree, n - 1, ST); i < n; st--, i++)
    { 
      if (st->generation == 1)
	sessDispDrawBranch(sessionTree, st,
			  levelSessBoxStarts, levelSessBoxEnds, y0) ;
      if (st->generation > maxGen)
	maxGen = st->generation ;
    }

  max = 65;
  for (i = 0; i < arrayMax(levelSessBoxEnds); i++)
    if (max < (arr(levelSessBoxEnds, i, int) + 3))
      max = (arr(levelSessBoxEnds, i, int) + 3);
 
  graphTextBounds (max, y0 + 3 * maxGen + 2) ;
  graphRedraw() ;

  arrayDestroy (levelSessBoxStarts);
  arrayDestroy (levelSessBoxEnds);

  return;
} /* sessDispDraw */

/************************* eof ******************************/
