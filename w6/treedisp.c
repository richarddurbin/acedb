/*  File: treedisp.c
 *  Author: Richard Durbin (rd@mrc-lmb.cam.ac.uk)
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
 * Description: display/editor for generic tree objects
 *              
 * Exported functions:
 *	treeDisplay
 *              
 * HISTORY:
 * * May 17 13:54 1999 (edgrif): Set blocking mode for tree chooser graph.
 * * Sep 28 14:24 1998 (fw): make timestamp boxes inactive, this 
 *              prevents seg-faults and invalid boxNum errors
 *              extra 'bs->up' tests in lookKbd() to prevent NULL pointer references?
 * * Aug 12 16:56 1996 (srk)
 * * Jun  3 12:42 1996 (rd)
 * * May  8 20:22 1993 (mieg): ON/OFF tag expansion
 * * May  8 20:21 1993 (mieg): updtateMode, to prevent expandText or ON/OFF
 * * Sep 20 01:05 1992 (rd): fixPath() handles new UNIQUE correctly
 * * Aug 16 10:59 1992 (mieg): tagChooser
 * * May  3 22:29 1992 (rd): redid MENU_PRESERVE (nl1 version lost, sorry)
 * * Feb 11 13:09 1992 (mieg): Introduced graphCompletionEntry
 * Created: Wed Oct 16 17:41:46 1991 (rd)
 * Last edited: Jan  6 17:55 2011 (edgrif)
 * CVS info:   $Id: treedisp.c,v 1.170 2012-10-23 13:48:48 edgrif Exp $
 *-------------------------------------------------------------------
 */

#define ACEDB_MAILER

#include <wh/acedb.h>
#include <wh/aceio.h>
#include <wh/lex.h>
#include <wh/a.h>
#include <wh/bs_.h>
#include <wh/bs.h>
#include <wh/bitset.h>		/* to remember open tags */
#include <wh/dump.h>
#include <whooks/systags.h>
#include <whooks/tags.h>		/* for E_mail */
#include <whooks/sysclass.h>
#include <wh/biblio.h>
#include <wh/session.h>
#include <wh/dbpath.h>
#include <wh/pick.h>
#include <wh/query.h>
#include <wh/pref.h>
#include <wh/help.h>		/* for helpOn() fw-981103 */
#include <wh/spread.h>
#include <wh/display.h>
#include <wh/fingerp.h>		/* for fpDisplay() fw-980928 */
#include <wh/parse.h>		/* for parseBuffer() fw-980928 */
#include <wh/main.h>		/* for ksetClassComplete */
#include <wh/tabledisp.h>
#include <wh/keysetdisp.h>
#include <wh/tree.h>
#include <wh/gex.h>
#include <wh/utils.h>
#include <wh/bindex.h>


extern void bsSubFuseType(OBJ obj, BS bs) ;


BITSET_MAKE_BITFIELD		/* define bitField for bitset.h ops */

/************************************************************/


/* The main state block for the tree display. */
typedef struct TreeDispStruct
{
  magic_t *magic ;        /* == &TreeDisp_MAGIC for validating tree structs. */

  KEY   key ;
  OBJ   objet ;
  Graph graph ;


  /* Cached box/bs, essential for navigation/selection etc. */
  int activebox ;

  BS first_box_BS ;					    /* Why don't I use boxes here ??? */
  BS last_box_BS ;

  int text_box ;
  Array content ;       /* content of each box: BS */
  Array attachedObjs ;

  Associator bs2box ;					    /* Maps all object nodes to display. */
  Associator tag2box;					    /* Maps just tags to the display */

  Array names ;         /* Find: text as it appears on the screen */ 
  Array boxes ;         /* boxes for the names */

  STORE_HANDLE handle;  /* used by Find function */
  BS    bsWait ;	  /* for use during update */
  Stack textStack ;	  /* for typing in things */
  int   classe ;        /* for completion mechanism */
  BOOL  editable,					    /* Is editing allowed at all ? */
    updateMode,						    /* Are we in edit mode ? */
    tagChooserMode,
    justKnownTags ;					    /* Only display tags already in the object. */
  Array longTextArray ;
  void *editor;
  char *tagWarp;        /* search pattern stripped of wildcards */
  char *pattern;        /* stores original search pattern */
  int   keyClicked;     /* last key clicked in Find function */
} *TreeDisp ;


typedef enum
  {
    PRESERVE_NONE = 0,					    /* Don't preserve any tree windows. */
    PRESERVE_SELECTED,					    /* Preserve any tree window clicked on by user. */
    PRESERVE_ALL					    /* Preserve all tree windows. */
  } TreePreserveType ;


typedef enum { UNDEFINED, LEADING, TRAILING, BOTH, NEITHER } AceWild;


#define LINE_LENGTH 40
#define SELECT       0
#define FIND         1


extern void  lookBiblio	       (OBJ objet, KEY k) ;

static void  lookDestroy       (void) ;
static void  lookRedraw        (void) ;
static void  lookMenu 	       (KEY k) ;
static void  lookPick          (int k, double x_unused, double y_unused, int modifier_unused) ;
static void  lookDraw          (TreeDisp look);
static void  updatePick        (int k, double x_unused, double y_unused, int modifier_unused) ;
static void newKbd(int k, int modifier) ;
static void  defuse            (BS bs) ;
static void  addComment        (char *string) ;
static void  addData           (char *string) ;
static void  addKeyText        (char *string) ;
static void  addKey            (KEY key) ;
static void  editEntry	       (char *text) ;
static void editorOK(void *ep, void *data);
static void editorCancel(void *ep, void *data);
static void  treeDump          (TreeDisp look) ;
static int   countSiblings(BS bs) ;
static void  detach1 (TreeDisp look) ;
static void  treeDispMailer (KEY key) ;	                  /* at end of file */
static int drawBS(TreeDisp look, BS bs, BS bsm, int x, int y) ;
static void  revertDisplayToDefault(TreeDisp look);       /* does what it says on the tin */
static void  FindObject        (TreeDisp look,            /* prompts for search pattern */
				AceFindDirection direction);
static BOOL  FindBox           (TreeDisp look,            /* locates the box within the tree */
                                AceFindDirection direction, 
				AceWild wildcard);
static BOOL  examineElement    (char *element, 
				char *pattern, 
				AceWild wildcard );
static int   intOrder          (void *a, void *b);         /* needed for arrayFind */
static void  updateSave        (TreeDisp look, BitSet bb);
static TreeDisp cancelEdit(TreeDisp look) ;

static BS goTagForward(TreeDisp look, BS curr) ;
static BS goTagReverse(TreeDisp look, BS curr) ;
static BS goTagTreeLeft(TreeDisp look, BS curr) ;
static BS getNextTagForward(TreeDisp look, BS bs) ;
static BS goTagLeftMost(TreeDisp look, BS curr) ;
static BS goTagUpMost(TreeDisp look, BS curr) ;
static BS tryRight(TreeDisp look, BS bs) ;
static BS tryDown(TreeDisp look, BS bs) ;

static void toggleTimeStamps(TreeDisp look) ;
static BOOL restoreActiveBS(TreeDisp look, BS active_BS) ;
static void knownTagsButton (void) ;
static void showTag(TreeDisp look, BS bs) ;
static void deleteNode(TreeDisp look) ;
static void editNode(TreeDisp look) ;

static void fixPath (TreeDisp look, BS bs) ;

static Stack getPathFromBS(BS bs) ;
static BS getBSFromPath(TreeDisp look, Stack path) ;

static BOOL isTag(BS bs) ;


static void attach (void) ;
static void detach (void) ;
static void attachRecursively (void) ;
static void attachHelp (void) ;


/* Local Globals...note that some of these are globals because their settings
 * are required in all instances of tree displays (e.g. the "preserve" settings),
 * i.e. are like "class" data in the OO sense. */

static MENUOPT attachMenu [] = {
  {attach, "Attach"},
  {detach, "Detach"},
  {attachRecursively, "Attach Recursively"},
  {attachHelp, "Help on attach"},
   {0, 0}
} ;

static TreeDisp drawLook ;
static int xmax, ymax ;
static BOOL isModel ;

static TreePreserveType autoPreserve_G = PRESERVE_NONE ;

static Array bbSetArray = 0 ;
static BOOL treeJustKnownTags = TRUE ;  /* global toggle, controls tag chooser */

static magic_t TreeDisp_MAGIC = "TreeDisp_MAGIC";
static magic_t GRAPH2TreeDisp_ASSOC = "TreeDisp_ASSOC";


static BOOL tree_edit_G = TRUE ;			    /* TRUE => tree views are editable. */
static BOOL  showTimeStamps = FALSE ;
static BOOL  showTagCount = FALSE ;
static int doAttachActionMenu;
static int doHorizontalLayout;

static int tagColour = BLACK ;

static BOOL dontExpand = FALSE ;

/*  I think the menus could be rationalied further, surely more options should be in both... */
static FREEOPT treeMenu[] =
  {
    { 15,"Tree display" },				    /* REMEMBER, the leading numeric _is_
							       the number of menu items following. */
    { 99,"Quit   ^W"  },
    { 98,"Help   ^H"} ,
    { 13,"Print   ^P"} ,
    {  0,""},
    {  9,"Update   ^U"} ,
    { 20,"Preserve"} ,
    { 21,"Set AutoPreserve Selected"} ,
    { 22,"Set AutoPreserve All   ^A"} ,
    { 23,"Unset AutoPreserve     ^A"} ,
    { 14,"Export data"} ,
    /* 
       { 23,"Biblio"} ,         see keySet menu
       { 24,"Neighbours"} ,     see keySet query
       { 25,"Contract"} ,       useless, just touch a tag
       { 26,"Expand"},          useless, just touch a tag
    */
    { 27,"Collapse Display"},           /* revert to default display */
    { 17,"Toggle Timestamps   ^T"} ,
    { 18,"Toggle TagCount"} ,
    { 41,"Find   ^F/^B"}, 
    {  5,"Show as Text"}
  } ;

static FREEOPT updateMenu[] =
  {
    {  12,"Tree Edit"} ,
    { 99,"Quit   ^W"} ,
    { 98,"Help   ^H"} ,
    {  0,""},
    {  3,"Save   ^U"} ,
    {  7,"Add Comment"} ,
    { 10,"Cancel   ^C"} ,
    {  8,"Delete   ^D"} ,
    { 11,"Edit     ^E"} ,
    { 25,"Contract"} ,
    { 26,"Expand"}, 
    { 28,"Show All Tags   ^L"}, 
    { 41,"Find   ^F/^B"}, 
  } ;




/**************************************************************/

void treeSetEdit(BOOL editable)
{
  tree_edit_G = editable ;

  return ;
}


static TreeDisp currentTreeDisp (char *caller)
{
  TreeDisp tree;
  Graph g = graphActive();

  if (!graphAssFind (GRAPH2TreeDisp_ASSOC,&tree))
    messcrash("%s() could not find TreeDisp on graph", caller);
  if (!tree)
    messcrash("%s() received NULL TreeDisp pointer", caller);
  if (tree->magic != &TreeDisp_MAGIC)
    messcrash("%s() received non-magic TreeDisp pointer", caller);

  if (g != tree->graph)
    messcrash("%s() found look from another graph", caller);

  return tree;
} /* currentTreeDisp */



static void colourBox (int box, BS bs, BOOL isActive)
{
  if (!bs)
    return ;

  if (isActive)
    {
      if (bs->size & (MODEL_FLAG | UNIQUE_FLAG))
	{
	  if (bs->size & SUBTYPE_FLAG)
	    {
	      graphBoxDraw (box, WHITE, GREEN) ;
	    }
	  else
	    {
	      graphBoxDraw (box, WHITE, BLUE) ;
	    }
	}
      else if (bsIsTag(bs))
	graphBoxDraw (box, WHITE, tagColour) ;
      else
	graphBoxDraw (box, WHITE, BLACK) ;
    }
  else
    {
      if (bs->size & MODEL_FLAG)
	{
	  if (bs->size & SUBTYPE_FLAG)
	    graphBoxDraw (box, BLACK, LIGHTGREEN) ;
	  else if (bsIsTag(bs))
	    graphBoxDraw (box, tagColour, CYAN) ;
	  else
	    graphBoxDraw (box, BLACK, CYAN) ;
	}
      else
	{
	  if (bsIsComment(bs))
	    graphBoxDraw (box, BLACK, LIGHTGRAY) ;
	  else if (bsIsTag(bs))
	    graphBoxDraw (box, tagColour, WHITE) ;
	  else
	    graphBoxDraw (box, BLACK, WHITE) ;
	}
    }

  return ;
}

static void colourActive (TreeDisp look, BOOL isOn)
{
  if (look->activebox > 0)
    colourBox(look->activebox, arr(look->content,look->activebox,BS), isOn) ;

  return ;
}

/************************************/

static void clearFlags (BS bs)
{
  bs->size = 0 ;
  if (bs->right)
    clearFlags (bs->right) ;
  if (bs->down)
    clearFlags (bs->down) ;
}

static void setOnFlag (BS bs)
{
  if (bs->right)
    { if (countSiblings(bs->right) > 6  ||
	  (pickExpandTag (bs->right->key) && countSiblings(bs->right) > 3))
	bs->size |= ON_FLAG ;
      setOnFlag (bs->right) ;
    }
  if (bs->down)
    setOnFlag (bs->down) ;
}


#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
static void setAllOnFlags(BS bs)
{
  if (bs)
    {
      bs->size &= ~ON_FLAG ;
      setAllOnFlags(bs->right);
      setAllOnFlags(bs->down);
    }
}
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */



static void setOnFlagModelOnly (BS bs)
{
  if (bs->right)
    { if ((bs->size & MODEL_FLAG) &&
	  (countSiblings(bs->right) > 6  ||
	   (pickExpandTag (bs->right->key) && countSiblings(bs->right) > 3)))
	bs->size |= ON_FLAG ;
      setOnFlagModelOnly (bs->right) ;
    }
  if (bs->down)
    setOnFlagModelOnly (bs->down) ;
}

static void getOnFlagSet (BS bs, BitSet bb)
{
  if (bs->right)
    { 
      if (!class(bs->key) && bs->key >= _Date)
	{
	  if (bs->size & ON_FLAG)
	    bitSet (bb, bs->key) ;
	  else
	    bitUnSet (bb, bs->key) ;
	}
      getOnFlagSet (bs->right, bb) ;
    }
  if (bs->down)
    getOnFlagSet (bs->down, bb) ;
}

static void setOnFlagSet (BS bs, BitSet bb)
{
  if (bs->right)
    { 
      if (!class(bs->key) && bs->key >= _Date)
	{
	  if (bit(bb,bs->key))
	    bs->size |= ON_FLAG ;
	  else
	    bs->size &= ~ON_FLAG ;
	}
      setOnFlagSet (bs->right, bb) ;
    }
  if (bs->down)
    setOnFlagSet (bs->down, bb) ;
}

/************************************/

BOOL treeDisplay(KEY key, KEY from, BOOL isOldGraph, void *unused)
{
  TreeDisp look ;

#ifdef DEBUG_TREEDISP
  printf("treeDisplay\n");
#endif

  look = (TreeDisp)messalloc(sizeof(struct TreeDispStruct)) ;
  look->magic = &TreeDisp_MAGIC;

  look->key = key;
  look->justKnownTags = treeJustKnownTags ;

  if (pickType (key) != 'B' || !(look->objet = bsCreateCopy(key)))
    goto abort ;

  clearFlags (look->objet->root) ;
  setOnFlag (look->objet->root) ;
  
  tagColour = prefInt ("TAG_COLOUR_IN_TREE_DISPLAY") ; 
  if (!tagColour)
    tagColour = BLACK ;

  /* fw, 990501 Jean likes this menu, but most people find it confusing,
     so it is now configurable, but switched off by default */
  doAttachActionMenu = prefValue("ACTION_MENU_IN_TREE_DISPLAY");

  /* mieg, may 30 96, try to lay out in a more vertical way
     without breaking inside data values - much better and
     is now the default */
  doHorizontalLayout = prefValue("HORIZONTAL_TREE");

  /* If we don't test autopreserve here then if the user clicked on an object in the keyset
   * window it will overwrite an existing autopreserved window. */
  if (autoPreserve_G != PRESERVE_ALL && isOldGraph)
    {
      lookDestroy () ;
      graphClear () ;
      graphGoto (0,0) ;
      graphRetitle (messprintf("%s: %s", className(key),name (key))) ;
    }
  else
    {
      if (!displayCreate ("TREE")) 
	goto abort ;

      graphRetitle (messprintf("%s: %s", className(key),name (key))) ;
      graphRegister (DESTROY, lookDestroy) ;
      graphRegister (PICK, lookPick) ;
      graphRegister (KEYBOARD, newKbd) ;
      graphRegister (RESIZE, lookRedraw) ;
      graphFreeMenu (lookMenu, treeMenu) ;
    }

  look->graph = graphActive() ;
  graphAssociate (GRAPH2TreeDisp_ASSOC, look) ;

  look->content = arrayCreate(32,BS);
  look->bs2box = assCreate () ;
  look->tag2box = assCreate(); 

  look->handle = handleCreate();              /* use a handle so we can destroy all in one go */
  look->names = arrayHandleCreate(32, char*, look->handle);
  look->boxes = arrayHandleCreate(32, int, look->handle);

  look->editable = tree_edit_G ;
  look->updateMode = FALSE ;
  look->tagChooserMode = FALSE ;
  lookDraw (look) ;

  return TRUE ;
  
abort:
  bsDestroy (look->objet) ;
  messfree (look) ;

  return FALSE ;
}

/************************************************************/

static void lookDestroy (void)
{
  TreeDisp look = currentTreeDisp("lookDestroy") ;
#ifdef DEBUG_TREEDISP
  printf("lookDestroy\n");
#endif
  if (look->bsWait && (look->bsWait->size & ADD_KEY_FLAG))
    displayUnBlock() ;	

  if (look->editor)
    messfree(look->editor);

  if (look->tagWarp)
    messfree(look->tagWarp);

  detach1 (look) ;

  if (look->objet && isCacheModified(look->objet->x)
      && (!strcmp("Tag-chooser", name(look->key))
	  || messQuery (messprintf("%s not saved, Save ?",name(look->key)))))
    {
      defuse (look->objet->root) ;
      bsSave (look->objet);
    }
  else
    {
      bsDestroy (look->objet);
    }

  arrayDestroy (look->content) ;

  assDestroy(look->bs2box) ;

  handleDestroy(look->handle);          /* clear boxpointers array & 
					** associated linked lists */

  if (look->textStack)
    stackDestroy (look->textStack) ;

  if (look->longTextArray)
    arrayDestroy (look->longTextArray);

  look->magic = 0 ;
  messfree(look) ;

  graphAssRemove (GRAPH2TreeDisp_ASSOC) ;

  return ;
}

/************************************************************/

static void lookRedraw (void)
{
  TreeDisp look = currentTreeDisp("lookRedraw") ;

  lookDraw(look) ;

  return ;
}

/*********************************************************/

static void lookPick (int box, double x_unused, double y_unused, int modifier_unused)
{ 
  BS bs ;
  TreeDisp look = currentTreeDisp("lookPick") ;

  if (autoPreserve_G)
    displayPreserve() ;

  if (!box)
    {
      colourActive(look, FALSE) ;
      look->activebox = 0 ;
      return ;
    }
  else if (box >= arrayMax (look->content))
    {
      messerror("LookPick received a wrong box number");
      return ;
    }

  bs = arr(look->content,box,BS) ;

  if (!bs)
    /* timestamp boxes have NULL bs attached, so they are inactive */
    return;

  /* the text of the currently picked box */
  graphPostBuffer(bs2Text(bs, dontExpand, isModel)) ; /* mhmp 11/04/97 */

  bs = arr(look->content, look->activebox, BS) ;

  if (box == look->activebox) /* a second hit - follow it */
    {
      if (bs->size & ON_FLAG) /* if hidden, show branch */
	{
	  bs->size ^= ON_FLAG ;
	  lookDraw(look) ;
	}
      else if (bs->key == _Pick_me_to_call && !look->updateMode && !look->tagChooserMode)
	externalDisplay(look->key) ;
      else if (bs->key == _Gel && !look->updateMode && !look->tagChooserMode)
	fpDisplay (look->key) ;
      else if (bs->key == _E_mail &&  !look->updateMode && !look->tagChooserMode)
	treeDispMailer (look->key) ;
      else if (class(bs->key))
	display (bs->key,look->key, 0) ;
      else if (bs->right && !look->updateMode && !look->tagChooserMode)
	{
	  bs->size ^= ON_FLAG ;
	  lookDraw(look) ;
	}
    }
  else                            /* change now */
    { 
      colourActive (look, FALSE) ;
      look->activebox = box ;
      look->keyClicked = SELECT;
      colourActive (look, TRUE) ;
    }

  return ;
}

/*****************/

/* Handles keyboard input for normal and update modes. */
static void newKbd(int k, int modifier)
{
  char *vp ;
  BS bs ;
  int box ;
  TreeDisp look = currentTreeDisp("newKbd") ;
  BOOL final_select = TRUE ;


  /* Should this be done for update mode as well ???? */
  if (!(look->updateMode) && isDisplayBlocked())
    return ;


  /* If user did anything in the window and "preserve" is on then preserve window. */
  if (autoPreserve_G)
    displayPreserve() ;


  /* Cntl key specials...NOTE that in edit mode the entrybox Cntl keys will override any
   * duplicates here. */
  if ((modifier & CNTL_MODKEY))
    {
      switch (k)
	{
	case A_KEY:
	  if (autoPreserve_G != PRESERVE_ALL)
	    autoPreserve_G = PRESERVE_ALL ;
	  else
	    autoPreserve_G = PRESERVE_NONE ;
	  break ;

	case B_KEY:
	  /* Find: enable Control/B to activate FindObject (backward)*/
	  FindObject(look, FIND_BACKWARD) ;
	  break ;

	case C_KEY:
	  look = cancelEdit(look) ;
	  break ;

	case D_KEY:
	  /* Delete a node. */
	  if (look->updateMode)
	    deleteNode(look) ;
	  break ;

	case E_KEY:
	  /* Edit a node. */
	  if (look->updateMode)
	    editNode(look) ;
	  break ;

	case F_KEY:
	  /* Find: enable Control/F to activate FindObject (forward)*/
	  FindObject(look, FIND_FORWARD);
	  break ;

	case L_KEY:
	  /* Show all tags. */
	  if (look->updateMode)
	    knownTagsButton() ;
	  break ;

	case DOT_KEY:
	  /* Find Next: enable Control/RightChevron to activate FindBox (forward)*/
	  FindBox(look, FIND_FORWARD, UNDEFINED);
	  break ;

	case COMMA_KEY:
	  /* Find Previous: enable Control/LeftChevron to activate FindBox (backward)*/
	  FindBox(look, FIND_BACKWARD, UNDEFINED);
	  break ;
    
	case T_KEY:
	  if (!(look->updateMode))			    /* Causes redraw so don't do in updatemode. */
	    toggleTimeStamps(look) ;
	  break ;

	case U_KEY:
	  /* Update Mode: enable Control/U to switch to Update Mode */
	  if (look->updateMode)
	    updateSave(look, 0);
	  else
	    treeUpdate();
	  break ;

	default:
	  /* Unknown Cntl key ? then just return. */
	  final_select = FALSE ;
	  break ;
	}
    }
  else
    {
      if (!look->activebox)
	{
	  /* If there is no active box we default to the first_box, effect is that user
	   * can press a key and arrive at the first box. */
	  if (look->first_box_BS)
	    {
	      if (!assFind(look->bs2box, look->first_box_BS, &vp))
		messcrash("first_box_BS out of step with bs2box associator.") ;
	      else
		{
		  look->activebox = assInt(vp) ;
		  colourActive(look, TRUE) ;
		}
	    }

	  final_select = FALSE ;
	} 
      else
	{
	  /* All of the below options require an active box.... */

	  /* Unselect currently selected box. */
	  if (!(look->updateMode))
	    colourActive(look, FALSE) ;

	  if (!(bs = arr(look->content, look->activebox, BS)))
	    {
	      final_select = FALSE ;
	    }
	  else
	    {
	      switch (k)
		{
		case RETURN_KEY:
		case SPACE_KEY :
		  if (look->updateMode)
		    {
		      updatePick(look->activebox, 0, 0, 0) ;
		    }
		  else
		    {
		      showTag(look, bs) ;
		    }
		  break ;


		  /* Cursor keys for left/right/up/down movement within branches. */
		case LEFT_KEY :
		  if ((modifier & ALT_MODKEY))
		    {
		      /* Jumps to leftmost tag of a branch to allow rapid movement up/down
		       * a tree. */
		      bs = goTagTreeLeft(look, bs) ;
		    }
		  else
		    {
		      if (bs != look->first_box_BS && bs->up && bs->up->right == bs)
			bs = bs->up ;
		      else
			{
			  BS tmp ;
			  
			  while ((tmp = tryRight(look, bs)))
			    bs = tmp ;
			}
		    }
		  break ;

		case RIGHT_KEY :
		  if (bs->right)
		    bs = getNextTagForward(look, bs) ;
		  else
		    {
		      while (bs != look->first_box_BS && bs->up && bs->up->right == bs)
			bs = bs->up ;
		    }
		  break ;

		case DOWN_KEY :
		  if (bs->down)
		    bs = bs->down ;
		  else while (bs->up && bs->up->down == bs)
		    bs = bs->up ;
		  break ;

		case UP_KEY :
		  if (bs->up && bs->up->down == bs)
		    bs = bs->up ;
		  else while (bs->down)
		    bs = bs->down ;
		  break ;

		case TAB_KEY:
		  /* Tab forwards/backwards through tree, within a branch but
		   * also from branch to branch as required. */
		  if ((modifier & ALT_MODKEY))
		    {
		      bs = goTagReverse(look, bs) ;
		    }
		  else
		    {
		      bs = goTagForward(look, bs) ;
		    }
		  break ;

		default:
		  return ;
		}
	    }
	}
    }


  if (final_select)
    {
      float x1, y1, x2, y2, scr_x1, scr_y1, scr_x2, scr_y2 ;

      if (assFind (look->bs2box, bs, &vp) && (box = assInt(vp)))
	{
	  /* Let's try making sure the box is actually on the screen..... */
	  graphBoxDim(box, &x1, &y1, &x2, &y2) ;

	  graphWhere(&scr_x1, &scr_y1, &scr_x2, &scr_y2) ;

	  if (x1 < scr_x1 || x2 > scr_x2 || y1 < scr_y1 || y2 > scr_y2)
	    graphGoto(x1, y1) ;

	  if (look->updateMode)
	    {
	      if (box != look->activebox)
		updatePick(box, 0, 0, 0) ;
	    }
	  else
	    {
	      Graph curr ;

	      look->activebox = box ;

	      curr = graphActive() ;
	      graphActivate(look->graph) ;
	      colourActive(look, TRUE) ;
	      graphActivate(curr) ;
	    }
	}
    }

  return ;
}


/*************************************************************/

/* 
 * A series of functions to move round a BS tree, could become part of the BS package ?
 */

/* Tries to go right within a branch, if that's not possible tries to go down or
 * if that's not possible it backs out of the current branch and goes down into
 * the next one. */
static BS goTagForward(TreeDisp look, BS curr)
{
  BS bs = curr ;

  if (bs == look->last_box_BS)
    {
      /* Wrap round from bottom to top of tree. */
      bs = look->first_box_BS ;
    }
  else if (bs->right)
    {
      bs = getNextTagForward(look, bs) ;
    }
  else if (bs->down)
    {
      bs = bs->down ;
    }
  else
    {
      while (bs->up && bs->up->up)
	{
	  while (bs->up && bs->up->down == bs)
	    bs = bs->up ;

	  if (bs->up && !bs->up->up)
	    break ;

	  bs = bs->up ;
	  if (bs->down)
	    {
	      bs = bs->down ;
	      break ;
	    }
	}
    }

  return bs ;
}


/* Tries to to up within a branch, if at the end of the branch then jumps to the _last_
 * element of the previous branch. */
static BS goTagReverse(TreeDisp look, BS curr)
{
  BS bs = curr ;

  if (bs == look->first_box_BS)
    {
      /* Wrap round from top to bottom of tree. */
      bs = look->last_box_BS ;
    }
  else
    {
      BS orig_bs ;

      orig_bs = bs ;

      if (bs->up)
	bs = bs->up ;

      /* If in new branch go as far right and as far down as possible. */
      if (bs->right != orig_bs)
	{
	  BS new_bs ;

	  while ((new_bs = tryRight(look, bs)))
	    {
	      bs = new_bs ;
		      
	      while ((new_bs = tryDown(look, bs)))
		bs = new_bs ;
	    }
	}
    }

  return bs ;
}


/* Goes to the very leftmost tag in the tree for a branch. */
static BS goTagTreeLeft(TreeDisp look, BS curr)
{
  BS bs = curr ;

  if (bs != look->first_box_BS)
    {
      while ((bs->up))
	{
	  BS upmost ;

	  /* Go as far left as we can. */
	  bs = goTagLeftMost(look, bs) ;
	  if (!(bs->up))
	    bs = bs->right ;

	  /* Go as far up (but _not_ left) as we can. */
	  if (bs->up)
	    upmost = goTagUpMost(look, bs) ;

	  /* If there is still more to the left of us then go left. */
	  if ((upmost->up) && upmost->up->right == upmost && (upmost->up->up))
	    bs = upmost ;
	  else
	    break ;
	}
    }

  return bs ;
}



/* Return the next visible tag, where "next" means go right if possible, if not
 * then down, if not then go up and then try to go down, if not then return up.
 * The effect of this is to "tab" forward through the visible tags.
 *  */
static BS getNextTagForward(TreeDisp look, BS curr)
{
  BS bs = curr ;
  void *vp = NULL ;

  if (bs->right && assFind(look->bs2box, bs->right, &vp))
    {
      bs = bs->right ;
    }
  else if (bs->down)
    {
      bs = bs->down ;
    }
  else
    {
      bs = bs->up ;
      if (bs->down)
	bs = bs->down ;
    }

  return bs ;
}


/* Goes as far left in a branch as possible, will not go up. */
static BS goTagLeftMost(TreeDisp look, BS curr)
{
  BS bs = curr ;


#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
  printf("%s\n", bs2Text(bs, dontExpand)) ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

  while ((bs->up) && (bs->up->right == bs))
    {
      bs = bs->up ;

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
      printf("%s\n", bs2Text(bs, dontExpand)) ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */
    }

  return bs ;
}


/* Goes as far up a sub-branch as possible, will not go "left". */
static BS goTagUpMost(TreeDisp look, BS curr)
{
  BS bs = curr ;


#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
  printf("%s\n", bs2Text(bs, dontExpand)) ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */


  while ((bs->up) && (bs->up->down == bs))
    {
      bs = bs->up ;


#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
      printf("%s\n", bs2Text(bs, dontExpand)) ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

    }

  return bs ;
}





/* If there is a tag to the right AND it is visible then return it,
 * otherwise return NULL. */
static BS tryRight(TreeDisp look, BS curr)
{
  BS bs = NULL ;
  void *vp = NULL ;

  if (curr->right && assFind(look->bs2box, curr->right, &vp))
    bs = curr->right ;

  return bs ;
}


/* If there is a tag down AND it is visible then return it,
 * otherwise return NULL. */
static BS tryDown(TreeDisp look, BS curr)
{
  BS bs = NULL ;
  void *vp = NULL ;

  if (curr->down && assFind(look->bs2box, curr->down, &vp))
    {
      bs = curr->down ;
    }

  return bs ;
}


/*************************************************************/

void treeUpdate (void)
{
  BitSet bb = 0 ;
  TreeDisp look = currentTreeDisp("treeUpdate") ;
  KEY tag = 0 ;
  BS tag_bs = NULL ;
  void *vp ;

  if (!(look->editable))
    {
      messout ("Sorry, tree view editing has been disabled, see your database/systems administrator.") ;
      return ;
    }

  if(!checkWriteAccess())
    return ;

  if (!KEYKEY(look->key))
    {
      messout ("Sorry, you can not update models this way") ;
      return ;
    }

  if (pickList[class(look->key) & 255 ].protected)
    {
      messout ("Sorry, this is a protected class, you cannot update it interactively") ;
      return ;
    }

  if(!checkWriteAccess())
    return ;

  if (look->activebox && bsIsTag(arr(look->content, look->activebox, BS)))
    {
      tag = arr(look->content, look->activebox, BS)->key ;

      tag_bs = arr(look->content, look->activebox, BS) ;
    }


  detach1 (look) ;
  bsDestroy (look->objet) ;

  if (!(look->objet = bsUpdate (look->key)))
    {
      look->objet = bsCreateCopy (look->key) ;

      lookDraw (look) ;
    }
  else
    {
      cacheMark (look->objet->x) ;
      look->updateMode = TRUE ;

      clearFlags (look->objet->root) ;
      bsFuseModel (look->objet, 0, look->justKnownTags) ;
      setOnFlag (look->objet->root) ; /* RD put back after FuseModel */

      if (bbSetArray)   /* mieg, inertia in open tags */
	{
	  bb = array(bbSetArray, class(look->key), BitSet) ;
	  if (bb)
	    setOnFlagSet (look->objet->root, bb) ;
	}

      look->activebox = 0 ;
      graphPop() ;

      lookDraw(look) ;

      if (tag && assFind(look->tag2box, tag_bs, &vp))
	{
	  float x1, y1;

	  graphBoxDim(assInt(vp), &x1, &y1, NULL, NULL);
	  graphGoto(x1, y1);

	  lookPick(assInt(vp), 0, 0, 0);
	}

      graphRegister (PICK, updatePick) ;
      graphRegister (KEYBOARD, newKbd) ;

      graphFreeMenu (lookMenu, updateMenu) ;
      graphHelp ("Update") ;
    }

  return ;
}

static void lookMenu (KEY k)
{
  BS bs ;
  BitSet bb = 0 ;
  TreeDisp look = currentTreeDisp("lookMenu") ;

  /* if at end then check gActive is correct first */
  if (autoPreserve_G)
    displayPreserve () ;

  switch ((int) k)
    {

    case 3 :                        /* save */
      if (look->bsWait)
	{
	  messout ("Finish what you have started first, or cancel") ;
	  return ;
	}
      updateSave(look, bb);
      break ;

    case 4 :                            /* switch to Tree Display */
      graphDestroy();
      treeDisplay(look->key, 0, FALSE, 0);
      break;
      
    case 5 :
      if (!look->activebox)
	{
	  messout ("Please first select a node") ;
	  return ;
	}
      bs = arr(look->content,look->activebox,BS) ;
      if (iskey(bs->key) == 2 )
	{ 
	  if(pickType(bs->key) == 'B')
	    display(bs->key, look->key, "TREE") ;
	  else
	    messout("Sorry, I cannot display %s as a tree", name(bs->key)) ;
	}
      else
	messout("No data associated to %s", name(bs->key)) ;
	
      break ;

    case 7 :		/* add comment */
      if (look->bsWait)
	{
	  messout ("Please finish first what you have started, or cancel") ;
	  return ;
	}
      if (!look->activebox)
	{
	  messout ("Sorry, you must first select a node to add to (single-click)") ;
	  return ;
	}
      bs = arr(look->content,look->activebox,BS) ;
      if (bs->size & MODEL_FLAG)
	{
	  messout("You must add comments to the original object (black and white stuff)") ;
	  return ;
	}
      if (bsIsComment(bs))
	{
	  messout ("You can't add comments to a comment") ;
	  return ;
	}
      bs->size |= ADD_COMMENT_FLAG ;
      look->bsWait = bs ;

      lookDraw (look) ;
      break ;

    case 8 :		/* delete */
      deleteNode(look) ;

      break ;

    case 9 :		/* update */
      displayPreserve() ;
      treeUpdate () ;
      break ;

    case 10:		/* cancel operation */
      look = cancelEdit(look) ;
      break ;

    case 41:           /* Find object */
      FindObject(look,FIND_FORWARD);
      break;
	  
    case 11:		/* edit */
       editNode(look) ;
       break ;
       
    case 13 :			/* "Print" */
      graphPrint() ; break ;

      /* Window preserve settings */
    case 20 :			/* "Preserve" */
      displayPreserve() ;
      break ;

    case 21 :			/* "Set AutoPreserve Selected" */
      autoPreserve_G = PRESERVE_SELECTED ;
      break ;

    case 22 :			/* "Set AutoPreserve All" */
      autoPreserve_G = PRESERVE_ALL ;
      break ;

    case 23 :			/* "Unset AutoPreserve" */
      autoPreserve_G = PRESERVE_NONE ;
      break ;


    case 14:			/* "Export data" */
      treeDump (look) ;
      break ;
    
#ifdef JUNK
    case 23 :			/* "Biblio" */
      biblioKey (look->key) ; break ;

    case 24 :			/* "Neighbors" */
      {
	KEYSET nks =  bsKeySet(look->key) , new = keySetCreate() ;
	KEY *kp ;
	int j1 = 0 , j2 ;
	if (nks)
	  { j2 = keySetMax(nks) ; kp = arrp(nks,0,KEY) - 1 ;
	    while(kp++, j2--)
	      if (!pickXref(class(*kp)) && iskey(*kp) == 2)
		keySet(new, j1++) = *kp ;
	    keySetDestroy(nks) ;
	  }

	keySetNewDisplay(new, messprintf("Neighbours of %s", name(look->key))) ;
      }
      break ;

#endif

    case 17:			/* "Toggle Timestamps" */
      toggleTimeStamps(look) ;
      break ;

    case 18:
      showTagCount = showTagCount ? FALSE : TRUE ;
      look->activebox = 0;	/* don't carry box-selection 
				   over to new display style */
      lookDraw (look) ;
      break ;


    case 25 :			/* contract */
      if (!look->activebox || 
	  !(bs = arr(look->content,look->activebox,BS)))
	{
	  messout ("You must first select a node to edit (single-click)") ;
	  return ;
	}
      if (!(bs->size & ON_FLAG))
	{
	  bs->size |= ON_FLAG ;
	  lookDraw (look) ;
	}
      else
	messout ("That node was already contracted") ;

      break ;

    case 26 :			/* expand */
      if (!look->activebox || !(bs = arr(look->content,look->activebox,BS)))
	{
	  messout ("You must first select a node to edit (single-click)") ;
	  return ;
	}
      if (bs->size & ON_FLAG)
	{
	  bs->size ^= ON_FLAG ;
	  lookDraw (look) ;
	}
      else
	messout ("That node was already expanded") ;
      break ;

    case 27 :             /* revert to default display */
      revertDisplayToDefault(look);
      break;

    case 28:
      knownTagsButton() ;
      break ;

#ifdef JUNK_ACEDB_MAILER
      /* now that we can paste cut, just paste the adress to your favorite mailer */
    case 31 :  /* Mailer */
      { 
	OBJ  obj = bsCreate(look->key) ;
	char *mailAddress ;
	ACEIN addr_in = NULL;

	graphHelp ("Mailer") ;

	if (!obj)
	  break ;

	if (!bsGetData(obj, _E_mail, _Text,&mailAddress))
	  {
	    if ((addr_in = messPrompt ("Please specify an email address",
				       "","w", 0)))
	      { 
		OBJ obj1;
		
		mailAddress = aceInWord (addr_in);
		
		obj1 = bsUpdate(look->key) ;
		if (!obj1)
		  goto fin ;
		
		if (!bsAddData(obj1, _E_mail, _Text, mailAddress))
		  {
		    bsDestroy(obj1) ;
		    messout ("Sorry, I can't save your address, "
			     "no E_mail Tag in this class") ;
		    goto fin ;
		  }
		bsSave(obj1) ;
		
	      }
	    else
	      goto fin ;
	  }
	else
	  acedbMailer(look->key, 0,0) ;
      fin:
	bsDestroy(obj);
	if (addr_in)
	  aceInDestroy (addr_in);
	break ;
      }
#endif /* ACEDB_MAILER */
	
    case 98 : 
      { int nn = 0 ;
      Stack h = stackCreate (50) ;
      KEYSET ks1 = keySetCreate () ;
      KEY k1 ;
      
      if (look->activebox) 
	{
	  bs = arr(look->content,look->activebox,BS) ;
	  while (bs)   /* look upward for a tag */
	    { keySet (ks1, nn++) = bs->key ;
	    if (!class (bs->key) && bs->key >= 50)
	      break ;
	    bs = bs->up ;
	    } 
	}
      
      pushText (h, messprintf("%s_%s",graphHelp(0),className(look->key))) ;
      while (nn--)
	{ catText (h, "_") ;
	k1 = keySet(ks1,nn) ;
	catText (h, class(k1) ? className(k1) : name(k1)) ;
	}
      
      helpOn (stackText (h, 0)) ;
      stackDestroy (h) ;
      keySetDestroy (ks1) ;
      }
      break ;

    case 99 :
      graphDestroy () ; break ;
    }

  return ;
}

/****************************************************/

static void treeDump (TreeDisp look)
{
  ACEOUT fo;
  static char directory[DIR_BUFFER_SIZE] = "" ;
  static char filename[FIL_BUFFER_SIZE] = "object" ;

  if (!lexIsKeyVisible (look->key))
    { messout("Sorry, I don't know how to dump %s\n", name(look->key));
      return;
    }
  
  fo = aceOutCreateToChooser ("Where do you wish to ace dump this object ?",
			      directory, filename, "ace", "w", 0);
  if (!fo)
    return;

  aceOutPrint (fo, "// data dumped from tree display\n\n") ;
  dumpKey (fo, look->key);

  messout ("Object %s written to %s\n", 
	   name(look->key), aceOutGetURL(fo));

  aceOutDestroy (fo);

  return;
} /* treeDump */





/*************************************************************/
/*************** drawing package *****************************/
/*******************************************************/


/******************* attach subpackage ***********************/

static void doDetach(BS bs)
{
#ifdef DEBUG_TREEDISP
  printf("doDetach\n");
#endif
  if (bs->right)
    doDetach (bs->right) ;
  if (bs->down)
    doDetach (bs->down) ;
  if (bs->right && (bs->right->size & ATTACH_FLAG))
    { bsTreePrune (bs->down) ; bs->down = 0 ; }
}

static void detach1 (TreeDisp look)
{int i ;
  Array a = look->attachedObjs ;
#ifdef DEBUG_TREEDISP
  printf("detach1\n");  
#endif
  if (arrayExists(a))
    { doDetach (look->objet->root) ;
      i = arrayMax (a) ;
      while (i--)
	bsDestroy (array (a, i, OBJ)) ;
    }
  arrayDestroy (look->attachedObjs) ;
}
	
static void detach (void)
{ int box ;
  BS bs ;
  TreeDisp look = currentTreeDisp("detach") ;
#ifdef DEBUG_TREEDISP
  printf("detach\n");
#endif
  box = look->activebox ;
  if (!box)
    { messout ("Please first select a node") ;
      return ;
    }
  else if ( box >= arrayMax (look->content))
    { messerror("Sorry, detach received a wrong box number");
      return ;
    }

  bs = arr(look->content,box,BS) ;

  if (bs && bs->right)
    doDetach (bs->right) ;
 /* do not detach down */
  if (bs && bs->right && (bs->right->size & ATTACH_FLAG))
    bs->right = 0 ;

  lookDraw (look) ;
}

static void doAttach (BS bs, BOOL whole, Array a, KEYSET ks, 
		      BOOL recursive, char *question)
{ OBJ obj ;
  int i, dummy ;
  KEY key ;
  KEYSET ks1 = 0 ;
  BS bs1 = bs , bs2 = bs ;
#ifdef DEBUG_TREEDISP
  printf("doAttach\n");
#endif
  while (bs1->up && 
	 ( pickType(bs1->key) != 'B' ||
	  !class(bs1->key) || class(bs1->key) == _VComment ||
	  class(bs1->key) == _VUserSession))
    bs1 = bs1->up ;
	
  if (bs->right) goto more ;

  ks1 = queryKey (bs1->key, question ? question : "IS *") ;
  for (i = 0 ; i < keySetMax (ks1) ; i++)
    { key = keySet (ks1, i) ;
      if ((recursive < 2 || !keySetFind (ks, key, &dummy)) &&
	  (obj = bsCreateCopy (key)))
      { 
	if (whole || queryFind (obj, key, 0))
	  { 
	    bs1 = obj->curr ;
	    if (whole && bs1->right) 
	      { 
		bs1 = bs1->right ;
		if (bs1->down && !bs1->right) bs1 = bs1->down ;
	      }
	    else if (bs1->down) /* mieg, dec 98  else needed*/
	      {
		bsTreePrune (bs1->down) ; 
		bs1->down = 0 ;
	      }

	    if (bs1->up)
	      {
		if (bs1->up->down == bs1) 
		  bs1->up->down = 0 ;
		else if (bs1->up->right == bs1) 
		  bs1->up->right = 0 ;
	      }

	    if (bs2 == bs)
	      bs2->right = bs1 ;
	    else  
	      bs2->down = bs1 ;

	    bs1->up = bs2 ;
	    bsDestroy (obj) ; /* destroy the now stripped tmp obj */
	  }
	else
	  { bsDestroy (obj) ;
	    goto more ;
	  }
	clearFlags (bs1) ;
	setOnFlag (bs1) ;
	bs1->size |= ATTACH_FLAG ;
	array (a, arrayMax (a), OBJ) = obj ;	
	if (ks && recursive > 1)
	  keySetInsert (ks, key) ;
      }
    }
  keySetDestroy (ks1) ;
 more:
  if (bs->right)
    if (recursive || !(bs->right->size & ATTACH_FLAG))
      doAttach (bs->right, whole, a, ks, recursive ? recursive + 1 : 0, question) ;
  if (bs->down)
    doAttach (bs->down, whole, a, ks, recursive, question) ;
}

static void attach1 (int box, BOOL recursive) 
{ BS bs, bs1, down ;
  int type, tcl ; 
  KEY tag ;
  char *cp ;
  Stack sta = 0 ;
  KEYSET ks = 0 ;
  TreeDisp look = currentTreeDisp("attach") ;
#ifdef DEBUG_TREEDISP
  printf("attach1\n");
#endif
  if (!box)
    { messout ("Please first select a node") ;
      return ;
    }
  else if ( box >= arrayMax (look->content))
    { messerror("Sorry, attach received a wrong box number");
      return ;
    }

  bs = arr(look->content,box,BS) ;
  bs1 = bs ;
  while (bs1->right && !class(bs1->key)) bs1 = bs1->right ;
  if (pickType(bs1->key) != 'B')
    { messout ("First object to the right is not a Tree. I can't attach, sorry") ;
      return ;
    }
  sta = stackCreate (50) ;
  if (treeChooseTagFromModel (&type, &tcl, class(bs1->key), &tag, sta, 0))
    { if (!arrayExists(look->attachedObjs))
	look->attachedObjs = arrayCreate (12, OBJ) ;
      if (recursive)
	ks = keySetCreate () ; /* against recursive attach runaway */
      down = bs->down ;
      bs->down = 0 ;
      cp = stackText (sta, 0) ;
      if (!strcmp ("WHOLE", cp))
	doAttach (bs, TRUE, look->attachedObjs, ks, recursive, 0) ;
      else
	{ queryFind (0, 0, cp) ; /* Initialise queryFind */
	  doAttach (bs, FALSE, look->attachedObjs, ks, recursive, cp) ;
	}
      bs->size &= ~ON_FLAG ;
      stackDestroy (sta) ;
      keySetDestroy (ks) ;
      bs->down = down ;
    }
  lookDraw (look) ;
}

static void attach (void) 
{ TreeDisp look = currentTreeDisp("attach") ;
 printf("attach\n");
  attach1(look->activebox, FALSE) ;
}

static void attachRecursively (void) 
{ TreeDisp look = currentTreeDisp("attachRecursivelly") ;
 printf("attachRecursively\n");
  attach1(look->activebox, TRUE) ;
}

static void attachQuery (BS bs, KEY qr)
{ char *cp0, *cp = name (qr) ;
  TreeDisp look = currentTreeDisp("attachQuery") ;

  if (!arrayExists(look->attachedObjs))
    look->attachedObjs = arrayCreate (12, OBJ) ;

  cp0 = cp ;
  cp += strlen (cp0) ;
  while (*(cp - 1 ) != ';' && cp > cp0) cp-- ;
  if (!strcmp ("WHOLE", cp))
    doAttach (bs, TRUE, look->attachedObjs, 0, 0, 0) ;
  else
    { queryFind (0, 0, cp) ; /* Initialise queryFind */
      doAttach (bs, FALSE, look->attachedObjs, 
		0, FALSE, cp0) ;
    }
}

static void attachHelp (void) 
{ helpOn ("attach") ;
}

/*******************************************************************/
static FREEOPT directTreeActionMenu[] = {
  {  1, "directTreeAction menu"},
  {'d', "Eliminate this node and everything on its right"}
} ;

static void directTreeAction (KEY key, int box)
{
  BS bs, bs1 ;
  Stack s, s1, t ;
  OBJ obj ;
  TreeDisp look = currentTreeDisp("directTreeAction") ;
#ifdef DEBUG_TREEDISP
  printf("directTreeAction\n");
#endif
  if (arrayExists(drawLook->content) && box > 0 &&
      box < arrayMax(drawLook->content))
    bs = array (drawLook->content, box, BS) ;
  else
    return ;
  if (!checkWriteAccess ())
    return ;
  if (pickList[class(look->key) & 255 ].protected)
    { messout ("Sorry, this is a protected class, you cannot update it interactively") ;
    return ;
    }
  if (!class(look->key))
    return ;
  obj = bsUpdate(look->key) ;
  if (!obj) return ;
  bsSave(obj) ;
  switch (key)
    {
    case 'd': /* eliminate */
      s = stackCreate (64) ; s1 = stackCreate (1024) ; 
      t = stackCreate (64) ;
      push (s, bs, BS) ; /* but not on t */
      bs1 = bs ; bs = bs->up ;
      while (bs)
	{ 
	  if (bs->right == bs1) 
	    { push (s, bs, BS) ; push (t, bs, BS) ; }
	  bs1 = bs ;
	  bs = bs->up ; 
	}
      pushText (s1, messprintf("%s %s\n-D  ", className(look->key),
		freeprotect(name(look->key)))) ;	  
      bs = pop (s, BS) ; /* jump root node */
      while (!stackAtEnd(s))
	{
	  bs = pop (s, BS) ;
	  if (class(bs->key) == _VUserSession)
	    continue ;
	  if (class(bs->key) == _VComment)
	     {
	       catText (s1, " -C ") ;
	       catText (s1,  freeprotect (name(bs->key))) ;
	     }
	  else if (class(bs->key))
	    catText (s1,  freeprotect (name(bs->key))) ;
	  else if (bs->key <= _LastC)
	    catText (s1,  freeprotect (bsText(bs))) ;
	  else if (bs->key == _Int)
	    catText (s1, messprintf ("%d", bs->n.i)) ;
	  else if (bs->key == _Float)
	    catText (s1, messprintf ("%g", bs->n.f)) ;
	  else if (bs->key == _DateType)
	    catText (s1, timeShow (bs->n.time)) ;
	  else
	    catText (s1, name (bs->key)) ;
	  catText (s1, " ") ;
	} 
      catText (s1, "\n") ;
      /* now reestablish upper node to counteract rd's bizare system  */
      bs = pop (t, BS) ; /* jump root node */
      while (!stackAtEnd(t))
	{
	  bs = pop (t, BS) ;
	  if (class(bs->key) == _VUserSession)
	    continue ;
	  if (class(bs->key) == _VComment)
	     {
	       catText (s1, " -C ") ;
	       catText (s1,  freeprotect (name(bs->key))) ;
	     }
	  else if (class(bs->key))
	    catText (s1,  freeprotect (name(bs->key))) ;
	  else if (bs->key <= _LastC)
	    catText (s1,  freeprotect (bsText(bs))) ;
	  else if (bs->key == _Int)
	    catText (s1, messprintf ("%d", bs->n.i)) ;
	  else if (bs->key == _Float)
	    catText (s1, messprintf ("%g", bs->n.f)) ;
	  else if (bs->key == _DateType)
	    catText (s1, timeShow (bs->n.time)) ;
	  else
	    catText (s1, name (bs->key)) ;
	  catText (s1, " ") ;
	}
      parseBuffer (stackText (s1,0), 0, TRUE) ;
      stackDestroy (s) ;
      stackDestroy (s1);
      bsDestroy (look->objet) ;
      look->objet = bsCreateCopy(look->key) ; 
      clearFlags (look->objet->root) ;
      setOnFlag (look->objet->root) ;

      lookDraw (look) ;
      break ;
    default: break ;
    }
}

/*******************************************************************/

static int treeClassComplete (char *cp, int len)
{
#ifdef DEBUG_TREEDISP
  printf("treeClassComplete\n");
#endif
  if (drawLook->classe)
    return ksetClassComplete(cp, len, drawLook->classe, TRUE) ;	/* TRUE means keyset will self-destroy. */
  else
    return 0 ;
}

static int drawTextEntry(int classe, int len, int x, int y, void (*fn)(char*), int *box_out)
{
  int box ;

#ifdef DEBUG_TREEDISP
  printf("drawTextEntry\n");
#endif

  box = graphCompScrollEntry (treeClassComplete, 
			      stackText(drawLook->textStack, 0), 
			      len, 32, x, y, fn) ;

  array (drawLook->content, box, BS) = 0 ;

  if (x + 32 > xmax)	/* RD 9402 - 32 chars appear on screen */
    xmax = x + 32 ;

  drawLook->classe = classe ;

  if (box_out)
    *box_out = box ;

  return y+1 ;
}


static int countSiblings (BS bs)
{ int n = 0 ;
  
  while (bs)
    { if (class(bs->key) != _VUserSession)
	n++ ; 
      bs = bs->down ; 
    }
  return n ;
}

static void drawTriangle(BS bs, int x, int y)
{
  int n = countSiblings(bs->right) ;
  graphColor(BLUE) ;
  graphText(messprintf("  -----> %d ",n), x, y) ;
  graphColor(BLACK) ;
}

static void treeDispTagCount (int cl, BS bs, BOOL mask)	/* recursive routine */
{
  
  while (bs)
    {
      if (!class (bs->key) && bs->key >= _Date)
	{
	  bs->tagCount = bIndexTagCount (cl, bs->key) ;
	  /* -1, dont know in bindex
	   *  0, not evaluated
	   */
	}      
      else if (bs->key != _UNIQUE)
	mask = TRUE ;
       if (mask) 
	bs->tagCount = -1 ;
      if (bs->right)
	treeDispTagCount (cl, bs->right, mask) ;
      bs = bs->down ;
    }
}


/* recursive routine: draws a node and then calls itself to draw the next node. */
static int drawBS(TreeDisp look, BS bs, BS bsm, int x, int y)
{
  int yMe = y ;
  int xPlus = 0 ;
  char *text, *cp, *vp, *textstr ;
  int box ;
  int oldTextFormat = PLAIN_FORMAT ;
  static int i = 0;

#ifdef DEBUG_TREEDISP
  printf("drawBS\n");
#endif

  if (i > arrayMax(drawLook->names))  /* reset i after arrays have been reallocated */
    i = 0;


  bsModelMatch(bs, &bsm) ; /* will fail on Time stamp, ignore returned value */


  if ((bs->size & ATTACH_FLAG) && bs->up && bs->up->key == bs->key)
    goto suite ; /* because, i don t draw the attached node itself */

  if (bs->size & ADD_DATA_FLAG)
    {
      y = drawTextEntry(0, bs->key < _LastC ? 2500 : 32, x, y, addData, &(look->text_box)) ;
    }
  else if (bs->size & ADD_KEY_FLAG)
    {
      y = drawTextEntry (class(bs->key), 2500, x, y, addKeyText, &(look->text_box)) ;

      /* Call to display will result in keyset window being shown if user TAB completes */
      displayBlock(addKey, "Or you can type in the name.\n"
		   "If you press the TAB key you will autocomplete\n"
		   "or get a list of possible entries.") ;
    }

  text = bs2Text (bs, dontExpand, isModel) ;

  yMe = y ;  /* must do this here again: y can change */

  if (bs->size & EDIT_NODE_FLAG)		/* edit entry */
    {
      stackTextForceFeed (drawLook->textStack, strlen(text) + 2560) ;
      stackClear (drawLook->textStack) ;
      pushText (drawLook->textStack, text) ;
      y = drawTextEntry(class(bs->key), strlen(text) + 2500, x, y, editEntry, &(look->text_box)) ;
    }
  else if (!drawLook->updateMode && 
	   !drawLook->tagChooserMode &&
	   class(bs->key) == _VLongText &&
	   KEYKEY(bs->key) && iskey(bs->key) == 2)
    {
      text = messprintf ("<see below %d>", arrayMax(drawLook->longTextArray)+1) ;
      graphText (text, x, yMe++) ;
      if (strlen(text) > xPlus)
	xPlus = strlen(text) ;
      array(drawLook->longTextArray,
	    arrayMax(drawLook->longTextArray), BS) = bs ;
    }
  else
    {
      box = graphBoxStart() ;

      array (drawLook->content, box, BS) = bs ;
      vp = (char *)0 + box ;

      if (bsIsTag(bs))
	{
	  assInsert(drawLook->tag2box, bs, vp);
	}

      textstr = strnew(text, drawLook->handle);
      array(drawLook->names, i, char*) = textstr;  /* store text in names array              */
      array(drawLook->boxes, i++, int) = box ;      /* store corresponding box in boxes array */

      /* Update cached bs/box data. */
      assInsert(drawLook->bs2box, bs, vp) ;

      /* Record first/last BS's for cursor movement, e.g. tab-wrapping etc */
      if (!(look->first_box_BS))
	look->first_box_BS = bs ;
      look->last_box_BS = bs ;				    /* Eventually this will be set to last one. */

      if ((iskey (bs->key) == 2 && class(bs->key) != _VText) || ((bs->size & ON_FLAG) && bs->right))
	oldTextFormat = graphTextFormat(BOLD) ;
      if (bs->key == _Greek)
	oldTextFormat = graphTextFormat(GREEK) ;

      uLinesText (text, LINE_LENGTH) ;

      /* Text fields can legitimately be null so to make sure null entries are drawn and 
       * hence selectable for update we insert a token, this relies on bs2Text()
       * only returning NULL for text fields. */
      if (!(cp = uNextLine(text)))
	cp = "<empty>" ;

      graphText (cp,x,yMe++) ;				    /* write out this node */
      if (strlen(cp) > xPlus)
	xPlus = strlen(cp) ;
      while ((cp = uNextLine(text)))			    /* indent following lines */
	{
	  graphText (cp,x+2,yMe++) ;
	  if (strlen(cp)+2 > xPlus)
	    xPlus = strlen(cp)+2 ;
	}

      if ((bs->size & ON_FLAG) && bs->right )
	drawTriangle(bs, x+xPlus, yMe-1) ;
      if ((iskey (bs->key) == 2 && class(bs->key) != _VText) ||
	  ( (bs->size & ON_FLAG) && bs->right  ) ||
	  bs->key == _Greek )
	graphTextFormat(oldTextFormat) ;
      graphBoxEnd() ;
      graphBoxInfo (box, bs->key, 0) ;
      if (box == drawLook->activebox)
	colourBox (box, bs, TRUE) ;
      else
	colourBox (box, bs, FALSE) ;

      if (doAttachActionMenu &&
	  !drawLook->updateMode && class(bs->key) != _VUserSession)
	{
	  BS bs1 = drawLook->attachedObjs ? bs : 0 ;
	  while (bs1)
	    { if (bs1->size & ATTACH_FLAG) break ; bs1 = bs1->up ; }
	  if (!bs1)
	    graphBoxFreeMenu (box, directTreeAction, directTreeActionMenu) ;
	}
    }

 suite:
  xPlus += x ;
  if (xPlus > xmax)
    xmax = xPlus ;
  xPlus += 2 ; /* spacing */

  if (bs->size & ADD_COMMENT_FLAG)	/* add comment box */
    {
      y = drawTextEntry(_VComment, 2500, xPlus, y, addComment, &(look->text_box)) ;
    }
  else if (!dontExpand && showTimeStamps)
    {
      if (bs->timeStamp)
	{
	  if ( (bs->size & ON_FLAG) && bs->right ) /* if collapsed */
	    xPlus += 9+2;	/* length of drawTriangle()-string+2 */

	  box = graphBoxStart() ;
	  graphText (name(bs->timeStamp), xPlus, y) ;
	  graphBoxEnd () ;
	  graphBoxDraw (box, BLACK, LIGHTGRAY) ;
	  array (drawLook->content, box, BS) = 0 ;
				/* do not attach box to a bs */
	}

      y = yMe ; xPlus = x+2 ;	/* force vertical layout */
    }
  else if (!dontExpand && showTagCount && bsIsTag(bs) && bs->tagCount >= 0)
    { 
      if ( (bs->size & ON_FLAG) && bs->right ) /* if collapsed */
	xPlus += 9+2;	/* length of drawTriangle()-string+2 */
      
      box = graphBoxStart() ;
      graphText (messprintf ("%d", bs->tagCount), xPlus, y) ;
      graphBoxEnd () ;
      graphBoxDraw (box, BLACK, bs->tagCount > 0 ? GREEN : RED) ;
      array (drawLook->content, box, BS) = 0 ;
				/* do not attach box to a bs */

      y = yMe ; xPlus = x+2 ;	/* force vertical layout */
    }

  if (bs->right && !(bs->size & ON_FLAG))
    {
      if ((!doHorizontalLayout)
	  && bsIsTag(bs)
	  && bs->key != _XREF
	  && (bsIsTag(bs->right) || (class (bs->right->key) && countSiblings (bs->right) > 1)))
	{
	  if (yMe > y)
	    y = yMe ;	
	  y = drawBS(look, bs->right, bsm ? bsModelRight(bsm) : 0, x + 2, y) ;  /* below, at x + 2 */
	}
      else
	{
	  y = drawBS(look, bs->right, bsm ? bsModelRight(bsm) : 0, xPlus, y) ;  /* to the right at same y */
	}
    }
  
  if (!bs->right && !(bs->size & ON_FLAG) &&
      bsm && bsm->right &&
      class (bsm->right->key) == _VQuery &&
      KEYKEY (bsm->right->key) != 1)  /* a hack, because treedisp overloads 1 as #type */
    {
      attachQuery (bs, bsm->right->key) ;

      if (bs->right)
	y = drawBS(look, bs->right, bsm ? bsModelRight (bsm) : 0 , xPlus, y) ;  /* to the right at same y */
    }

  if (yMe > y)
    y = yMe ;

  if (bs->down)
    y = drawBS(look, bs->down, bsm, x, y) ;		/* below at new y location */

  return y ;
}



/************ table.menu.wrm **************

also see whelp/TreeTableMenu.html for details

********************************************/

static Array class2tableMenu = 0 ;
/* this is an Array of Arrays of type FREEOPT, it is indexed by the classNumber.
   A correct FREEOPT Array is generated for each class for which
   a table menu entry exists in table.menu.wrm, such an entry looks like this :

   Person
   Email_addresses : wquery/e_mail.def
   Publications :    wquery/papers.def

   Author
   Co_writers :      wquery/writers.def


   The first word in each block is the class. All tree displays for that class
   will have a table menu with the items that follow.
   If the class Person is compiled as class Number 75, the class2tableMenu
   Array has an entry at index 75, which corresponds to this statis definition

   static FREEOPT xxx = {
     {  2, "Person"},
     {  1, "Email_addresses : wquery/e_mail.def"},
     {  2, "Publications :    wquery/papers.def"},
     {  0, 0 }
   };

   The last word in each entry is translated to a file name for the saved
   Table_Maker query.
*/

static FREEOPT* getTableMenu(int theClassNum)
{
  FILE *f ;
  int classNum = 0, level ;
  char *aWord, *theLine ; 
  BOOL inside = FALSE ;
  int tablesInThisClass = 0;
  Array tableMenu = 0;
  char *menufile;

  if (!class2tableMenu)		/* init menus */
    {
      class2tableMenu = arrayCreate(64, Array) ;
      
      menufile = dbPathFilName("wspec", "table.menu", "wrm", "r", 0);
      if (!menufile)
	return 0;
      if (!(f = filopen (menufile, "", "r")))
	{
	  messfree(menufile);
	  return 0 ;
	}
      messfree(menufile);

      level = freesetfile(f,"") ;
      
      while(freecard(level))
	{
	  theLine = freepos() ; aWord = freeword() ;
	  if (aWord)
	    {
	      if (!inside)
		{ 
		  /* first word is a class name */
		  if ((classNum = pickWord2Class(aWord)))
		    { 
		      inside = TRUE ;

		      tableMenu = array(class2tableMenu, classNum, Array);
		      if (arrayExists(tableMenu))
			/* a menu already exists, so append to it. */
			tablesInThisClass = arr(tableMenu, 0, FREEOPT).key;
		      else
			{
			  tableMenu = arrayCreate(3, FREEOPT) ;
			  tablesInThisClass = 0;
			}
		    }
		}
	      else
		{ 
		  tablesInThisClass++ ;
		  array(tableMenu,tablesInThisClass, FREEOPT).key = tablesInThisClass ;
		  array(tableMenu,tablesInThisClass, FREEOPT).text = strnew(theLine, 0);
		  array(tableMenu,tablesInThisClass+1, FREEOPT).key = 0;
		  array(tableMenu,tablesInThisClass+1, FREEOPT).text = 0;
		}
	    }
	  else
	    {
	      if (inside)		
		{ 
		  inside = FALSE ;
		  
		  /* finish off menu for this class */
		  if (tablesInThisClass > 0)
		    {
		      array(tableMenu, 0, FREEOPT).key = tablesInThisClass;
		      array(tableMenu, 0, FREEOPT).text = strnew(pickClass2Word(classNum), 0);
		      
		      array(class2tableMenu, classNum, Array) = tableMenu;
		    }
		  else
		    arrayDestroy(tableMenu);
		}
	    }
	}
    } /* end init */
  
  if (class2tableMenu)
    tableMenu = array(class2tableMenu, theClassNum, Array);
  
  if (arrayExists(tableMenu))
    return arrayp(tableMenu, 0, FREEOPT) ;
  
  return  NULL ;
} /* getTableMenu */

static void tableMenuOpenDisplay (char *menuitem, KEY objKey)
{
  char *fname;
  SPREAD spread;
  TABLE *results = 0;
  Array flipped, widths;
  char *params;

  fname = menuitem ;  /* last word is file name */
  while(*fname) fname++ ;
  if (menuitem < fname) fname-- ;
  while(fname > menuitem && *(fname - 1) != ' ') fname-- ;
      
  /* the parameter to the table-definition is the object name in quotes,
   * the definition will refer to the object as %1
   *      (shown as %%1 in TableMaker) */
  params = strnew(freeprotect(name(objKey)), 0);

  spread = spreadCreateFromFile (fname, "r", params);

  if (!spread)
    return;

  results = spreadCalculateOverWholeClass(spread);

  flipped = spreadGetFlipInfo(spread);
  widths = spreadGetWidths(spread);

  if (results)
    tableDisplayCreate (results, spreadGetTitle(spread), flipped, widths) ;

  spreadDestroy (spread);
  arrayDestroy (flipped);
  arrayDestroy (widths);
  messfree (params);

  return;
} /* tableMenuOpenDisplay */


static void tableMenuSelector(void)
{ 
  FREEOPT* ff = 0 ;
  KEY k ;
  TreeDisp look = currentTreeDisp("tableMenuSelector") ;

  ff = getTableMenu(class(look->key)) ;
  if (ff && graphSelect(&k, ff))
    tableMenuOpenDisplay (freekey2text(k, ff), look->key);

  return;
} /* tableMenuSelector */


static void tableMenuCall(KEY k, int box)
{
  FREEOPT* ff = 0 ;
  TreeDisp look = currentTreeDisp("tableMenuCall") ;

  ff = getTableMenu(class(look->key)) ;
  if (ff)
    tableMenuOpenDisplay (freekey2text(k, ff), look->key);

  return;
} /* tableMenuCall */



static float drawLongText (float ymax)
{
  int box ;
  BS bs ;
  int i, x ;
  char *cp ;
  Stack text, path ;

  if (!arrayMax (drawLook->longTextArray))
    return ymax ;

  path = stackCreate(64) ;
  for (i = 0; i < arrayMax (drawLook->longTextArray); i++)
    { bs = arr(drawLook->longTextArray, i, BS) ;
      if (!(text = stackGet (bs->key)))
	continue ;
				/* draw a spacer line */
      ymax += 1.0 ;
      graphLine (0, ymax, 1000, ymax) ;
      ymax += 1.0 ;
				/* draw the path to this node */
      stackClear (path) ;
      do
	{ push (path, bs, BS) ;
	  while (bs->up->down == bs) bs = bs->up ;
	  bs = bs->up ;
	} while (bs->up) ;	/* until reach root */
      x = 0 ; 
      cp = messprintf ("%d.  ", i+1) ;
      graphText (cp, x, ymax) ;
      x += strlen(cp) ;
      while (!stackEmpty (path))
	{
	  cp = bs2Text((bs = pop (path, BS)), dontExpand, isModel) ;

	  if (stackEmpty (path))
	    { box = graphBoxStart() ;
	      array (drawLook->content, box, BS) = bs ;
	      graphTextFormat (BOLD) ;
	    }
	  graphText (cp, x, ymax) ;
	  x += strlen(cp) + 1 ;
	}
      graphBoxEnd () ;
      graphTextFormat (PLAIN_FORMAT) ;

      ymax += 2 ;
      if (xmax < 12) xmax = 12 ;
      stackCursor (text, 0) ;
      while ((cp = stackNextText(text)))
	{ graphText (cp, 1, ymax++) ;
	  if (1 + strlen (cp) > xmax)
	    xmax = 1 + strlen(cp) ;
	}
      stackDestroy (text) ;
    }

  stackDestroy (path) ;
  return ymax ;
}


/* Toggles between showing all known tags and all used tags. */
static void knownTagsButton (void)
{
  TreeDisp look = currentTreeDisp("knownTagButton") ;
  BS active_BS = NULL ;

  /* If there's an active box remember it's BS. */
  if (look->activebox)
    active_BS = arr(look->content, look->activebox, BS) ;

  treeJustKnownTags = look->justKnownTags = look->justKnownTags ? FALSE : TRUE ;

  defuse(look->objet->root) ;

  bsFuseModel(look->objet, 0, treeJustKnownTags) ;

  lookDraw(look) ;


  /* Now restore the active BS. */
  if (active_BS)
    {
      if (!restoreActiveBS(look, active_BS))
	messcrash("Could not restore active BS on graph.") ;
    }

  return ;
}

static void biblioButton (void)
{
  TreeDisp look = currentTreeDisp("biblioButton") ;

  biblioKey (look->key) ;

  return ;
}


static void lookDraw (TreeDisp look)
{
  FREEOPT* ff = 0 ;
  int box ;
  float x1, y1, x2, y2 ;

  graphActivate (look->graph);
   
  if (look->activebox)
    graphBoxDim (look->activebox, &x1, &y1, &x2, &y2); 
  else
    y1 = -1;
      

  graphClear() ;


  /* graphClear() will NULL any keyboard callback if a text window is shown so we need to
   * reset the update keyboard function if in update mode. */
  if (look->updateMode)
    graphRegister(KEYBOARD, newKbd) ;


  arrayMax(look->content) = 0 ;

  assClear(look->bs2box) ;
  look->first_box_BS = look->last_box_BS = NULL ;

  assClear(look->tag2box) ;

  handleDestroy(look->handle);          /* Clear boxpointers array & associated linked lists. */
  look->handle = handleCreate();        /* Recreate everything */
  look->names = arrayHandleCreate(32, char*, look->handle);
  look->boxes = arrayHandleCreate(32, int , look->handle);

  look->tagWarp = NULL;

  box = graphBoxStart() ;
  array (look->content, box, BS) = look->objet->root ;
  graphText (name(look->key),0,0) ;
  graphBoxEnd () ;

  isModel = class(look->key) == _VModel  || !KEYKEY(look->key)  ?
    TRUE : FALSE ; 

  if (showTagCount)
    {
      int cl = isModel ? pickWord2Class (name(look->key)+1) : class (look->key) ;

      if (cl)
	treeDispTagCount (cl, look->objet->root->right, FALSE) ;
    }

  if (look->objet->root && look->objet->root->right)
    {
      drawLook = look ;
      drawLook->textStack = stackReCreate (drawLook->textStack, 2600) ;
      drawLook->longTextArray = arrayReCreate (drawLook->longTextArray, 8, BS);
      xmax = 0 ;
      dontExpand = look->updateMode || look->tagChooserMode ;

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
      {
	BS bsm ;

	bsm = bsModelRoot(look->objet) ;

	printf("\nthis model node is: %s\n", nameWithClass(bsm->key)) ;

	if (bsIsType(bsm))
	  printf("\nthis model node is a subtype: %s\n", nameWithClass(bsm->key)) ;
      }
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

      /* Recursively draws the object tree. */
      ymax = drawBS(look, look->objet->root->right, bsModelRoot(look->objet)->right, 2, 4) ;

      /* show any long text at the bottom. */
      ymax = drawLongText (ymax) ;
    }
  else
    {
      xmax = 1 ; 
      ymax = 2 ;		/* for buttons */
    }

#if !defined(macintosh)
  graphButton("Quit", graphDestroy, 60-1 - 5, .4) ;
  if (xmax < 60) xmax = 60 ;
#endif
  
  if (!look->updateMode && !look->tagChooserMode)
    {
      box = graphButton ("Attach...", attach, 60-1 - 11 - 5, .4) ;
      if (xmax < 52) xmax = 52 ;
      graphBoxMenu (box, attachMenu) ;
    }
  else
    {
      if (treeJustKnownTags)
	box = graphButton ("Show all tags", knownTagsButton, 60 - 22, .4) ;
      else
	box = graphButton ("Limit to known tags", knownTagsButton, 60 - 26, .4) ;
      if (xmax < 52) xmax = 52 ;
    }
  
  if ((ff = getTableMenu(class(look->key))))
    {
      box = graphButton("Tables...", tableMenuSelector, 60-1 - 10, 3.4) ;
      if (xmax < 40) xmax = 40 ;
      graphBoxFreeMenu(box, tableMenuCall, ff) ;
    }
  
  if (biblioKeyPossible(look->key))
    graphButton("Biblio", biblioButton, 60-1 - 8 - 11 - 5, .4) ;
  
  graphTextBounds (xmax,ymax) ;
 
  if (y1 > 0)
    graphGoto (x1, y1) ;
  
  graphRedraw() ;

  displaySetSizeFromWindow ("TREE") ;

  if (look->activebox >= arrayMax(look->content))
    look->activebox = 0 ;

  isModel = FALSE ;

  return ;
}

/************************************************************/
/************************************************************/

/* Updating:
   The plan is to build a new tree containing the union of the model
   and the current object.  We will rebuild every time an action changes the
   structure.  
   Parts of the model not yet represented in the object will be coloured
   blue.  The XREF, UNIQUE etc bits of the model will not be shown. Class
   slots (?Class) will appear in cyan at the bottom of each column of objects,
   so you can add new members (or replace if unique).
   slots (#Class) will not be shown.
   If you double-click on a cyan tag it will be added to your object.
   If you double click on a (cyan) ?Class then you will be prompted for a name,
   but you can also pick something from that class elsewhere in the system.  If
   you give a name that is a new lex entry it will ask for confirmation before
   adding.
   When displayBlocked you can not display any new objects, nor get the biblio
   etc.

   We will use bs->size to hold the information about whether an entry is
   in the model or not.  Adding and subtracting the model is done by two
   routines bsFuseModel() in bssubs.c and defuse() below.
   Codes for ->size are:
   	1	part of model (painted cyan)
	2	unique part of original tree
	4	fake node to add data (TextEntry)
	8	fake node to add comment (TextEntry)
	16	fake node to add key (TextEntry and DisplayBlock)
	32      edit current node - rather than enter a new one
	64	special node to add Type (TextEntry and DisplayBlock)
	   all from 4 on are in fact mutually exclusive - there
	   should be at most one at any one time (set to bsWait).
       128	ON_FLAG  to contract/uncontract
       256	ATTACH_FLAG
       512	new node this session

   We disable the keyboard routine (used for data entry), and enable a new
   pick routine.  The menu should change to contain Save (returns to standard
   form), Delete, and Add comment.  We should replace Save in the standard
   menu by Update.  Perhaps the standard menu could also contain Add comment?
*/

static void updatePick (int box, double x_unused, double y_unused, int modifier_unused)
{
  BS bs ;

  TreeDisp look = currentTreeDisp("updatePick") ;

  if (autoPreserve_G)
    displayPreserve () ;

  if (!box)
    {
      colourActive (look, FALSE) ;
      look->activebox = 0 ;
      return ;
    }
  else if (box >= arrayMax (look->content))
    {
      messerror("updatePick received a wrong box number %d",box);
      return ;
    }

  if (box == look->activebox)         /* a second hit - follow it */
    {
      bs = arr(look->content,look->activebox,BS) ;

      if (!bs)	      /* not a bs box, e.g. inside a textEntry */
	return ;

      if (bs->size & ON_FLAG) /* if hidden, show branch */
	{
	  bs->size ^= ON_FLAG ;
	  lookDraw(look) ;
	}
      else if (look->bsWait)
	messout ("Finish what you are doing first (or cancel)") ;
      else if (isDisplayBlocked())	/* from different update window */
	messout ("First deal with the other object you are waiting on") ;
      else if (bsIsTag(bs))
	{
	  if (bs->size & MODEL_FLAG)
	    fixPath (look, bs) ;
	  else
	    { 
	      bs->size |= ON_FLAG ;
	      lookDraw(look) ;
	    }
	}
      else if (!(bs->size & MODEL_FLAG)) /* in original object */
	{
	  look->bsWait = bs ;
	  bs->size |= EDIT_NODE_FLAG ;	/* edit flag */
	  lookDraw (look) ;
	}
      else if (bsIsTag(bs->up) || !(bs->up->size & MODEL_FLAG))
	{
	  if (bs->size & SUBTYPE_FLAG)	/* a type */
	    {
	      bsSubFuseType (look->objet, bs) ;
	      lookDraw (look) ;
	    }
	  else
	    { 
	      look->bsWait = bs ;

	      if (class(bs->key))
		bs->size |= ADD_KEY_FLAG ;	/* displayBlock() + textEntry */
	      else
		bs->size |= ADD_DATA_FLAG ;	/* textEntry only */

	      lookDraw (look) ;
	    }
	}
      else
	{
	  messout("Sorry - you must be next to a tag or an existing part of the object") ;
	}
    }
  else                              /* change highlighted entry */
    {
      colourActive (look, FALSE) ;
      look->activebox = box ;
      bs = arr(look->content,box,BS) ;

      if (!bs)		/* not a bs box */
	look->activebox = 0 ;

      colourActive (look, TRUE) ;

      if (bs)
	graphPostBuffer (bs2Text (bs, dontExpand, isModel)) ;
    }
}

/********************************/

static void defuse (BS bs)	/* recursive */
{
  BS temp ;

  bs->size = (bs->size & (NEW_FLAG | ON_FLAG)) ;

  while (bs->right && (bs->right->size & MODEL_FLAG))
    {
      temp = bs->right ;
      bs->right = bs->right->down ;
      if (bs->right)
	bs->right->up = bs ;
      temp->up = 0 ;
      temp->down = 0 ;
      bsTreePrune (temp) ;
    }

  if (bs->right)
    defuse (bs->right) ;

  while (bs->down && (bs->down->size & MODEL_FLAG))
    {
      temp = bs->down ;
      bs->down = bs->down->down ;
      if (bs->down)
	bs->down->up = bs ;
      temp->up = 0 ;
      temp->down = 0 ;
      bsTreePrune (temp) ;
    }

  if (bs->down)
    defuse (bs->down) ;

  return ;
}

/***********************************/

static void fixPath (TreeDisp look, BS bs)
{
  BS start = bs ;

  while (bs->size & MODEL_FLAG)	/* code for belonging to model */
    {
      bs->size &= ~MODEL_FLAG ;
      bs->size |= NEW_FLAG ;
      bs->timeStamp = sessionUserKey() ;

      while (bs->up && bs->up->down == bs)	/* go to top of column */
	bs = bs->up ;

      if (bs->up)		/* go to entry in previous column */
	bs = bs->up ;
    }

  defuse (look->objet->root) ;	/* leaves new stuff */

  bs = start ;
  for (bs = start ; bs->up ; bs = bs->up)
    {
      if (bs->bt && bs->bt->bsm && 
	  bs->bt->bsm->n.key & UNIQUE_BIT) /* UNIQUE */
	{
	  while (bs->down)
	    {
	      look->objet->curr = bs->down ;
	      look->objet->modCurr = bs->bt->bsm ;
	      bsRemove (look->objet) ;
	    }

	  while (bs->up->down == bs && !bsIsComment(bs->up))
	    {
	      look->objet->curr = bs->up ;
	      look->objet->modCurr = bs->bt->bsm ;
	      bsRemove (look->objet) ;
	    }
	  }

      while (bs->up->down == bs)	/* go to top of column */
	bs = bs->up ;
    }

  bsFuseModel (look->objet, 0, treeJustKnownTags) ;
  setOnFlagModelOnly (look->objet->root) ;
  look->objet->flag |= OBJFLAG_TOUCHED ;

  lookDraw(look) ;

  return ;
}

/*********************************/

static void addComment (char* text)
{ 
  BS bs ;
  TreeDisp look = currentTreeDisp("addComment") ;
  Stack path_to_updated ;
  BS active_bs ;

  if (!look->bsWait)
    messcrash ("screwup in addComment - no bs to add to") ;

  path_to_updated = getPathFromBS(look->bsWait) ;

  look->objet->curr = look->bsWait ;	/* point to add to */
  bsAddComment (look->objet, text, 'C') ;
  
  look->bsWait->size &= ~ADD_COMMENT_FLAG ;
  bs = look->bsWait ;
  look->bsWait = NULL ;

  fixPath (look, bs) ;

  /* Reset activebox from the path that we recorded at the start,
   * for comments we just stay where we are. */
  if ((active_bs = getBSFromPath(look, path_to_updated)))
    {
      restoreActiveBS(look, active_bs) ;    
    }
  stackDestroy(path_to_updated) ;



  return ;
}

static void addKey(KEY key)
{ KEY old ;
  BS bs, bsm ;
  OBJ obj ;
  TreeDisp look ;
  Stack path_to_updated ;
  BS active_bs ;

  look = currentTreeDisp("addKey") ;

  if (!key)		/* cancellation */
    return ;

  if (!(bs = look->bsWait))
    messcrash ("screwup in addKey - bs is missing") ;

  path_to_updated = getPathFromBS(bs) ;

  if (bsIsComment (bs))		/* no matching bsm */
    {
      if (class(key) != class(bs->key))
	{
	  messout ("Sorry, that is not a comment you selected") ;
	  return ;
	}
      else
	bs->key = key ;
    }
  else
    { 
      old = bs->key ;
      bs->key = key ;		/* try to enter the key */

      if (!bs->bt || !(bsm = bs->bt->bsm))
	messcrash ("screwup in addKey - bsm is missing") ;

      if (!bsModelMatch (bs, &bsm))
	{ 
	  messout ("Sorry - that key is not in the right class") ;
	  bs->key = old ;
	  return ;
	}
    }

  /* X-ref */
  if (pickXref(class(key)) || KEYKEY(bsm->n.key))	
    { 
      obj = look->objet ;
      if (!obj->xref) obj->xref = stackCreate (64) ;
      push (obj->xref, key, KEY) ;

      if (pickXref(class(key)))
        push (obj->xref, (_Quoted_in), KEY) ;
      else
	push (obj->xref, KEYKEY(bsm->n.key), KEY) ;
    }

  bs->size |= NEW_FLAG ;
  bs->timeStamp = sessionUserKey() ;
  bs->size &= ~(ADD_KEY_FLAG | EDIT_NODE_FLAG) ; /* must do this since display block is freed on return */

  look->text_box = 0 ;

  look->bsWait = NULL ;

  fixPath(look, bs) ;		/* clear "model" flags and redisplay */


  /* Reset activebox from the path that we recorded at the start, if we can move right or down
   * from the data we added then we do, otherwise we just stay where we are. */
  if ((active_bs = getBSFromPath(look, path_to_updated)))
    {
      if (active_bs->right)
	active_bs = active_bs->right ;
      else if (active_bs->down)
	active_bs = active_bs->down ;
  
      restoreActiveBS(look, active_bs) ;
    }
  stackDestroy(path_to_updated) ;


  /* Make sure treedisplay window is the active graph. */
  graphActivate(look->graph) ;

  return ;
}

static void addKeyText (char *text)
{ 
  BS bs ;
  KEY key ;
  int table ;
  TreeDisp look = currentTreeDisp("addKeyText") ;
#ifdef DEBUG_TREEDISP
  printf("addKeyText\n");
#endif

  if (!(bs = look->bsWait))
    messcrash ("Screwup in addKeyText - bs missing") ;

  table = class(bs->key) ;
  if (!*text || *text == '*' || !lexIsGoodName(text))
    {
      while(*text) 
	*text++ = 0 ;  /* necessary to set totally to 0 for graphTextEntry */
      graphRedraw() ;

      return ;
    }

  if (!lexword2key(text,&key,table))
    {
      if (table != _VText && 
	  !messQuery 
	  (messprintf("Unknown name - do you want to create a new %s called:%s", 
		      className(KEYMAKE(table,0)), text)))
	{ 
#ifdef WHY_IS_THIS_NEEDED___SHOULDNT_WE_LEAVE_TEXT_TO_CORRECT_TYPO	  
	  while(*text) 
	    *text++ = 0 ;  /* necessary to set totally to 0 for graphTextEntry */ 
	  graphRedraw() ;
#endif
	  return ;
	}
      else
	lexaddkey (text,&key,table) ;
    }

  display (key,0,0) ;					    /* calls addKey via displayBlock() */

  return ;
}

static void addData (char* text)
{
  BS bs ;
  int i ;
  float f ;
  mytime_t tm ;
  TreeDisp look = currentTreeDisp("addData") ;
  Stack path_to_updated ;
  BS active_bs ;

#ifdef DEBUG_TREEDISP
  printf("addData\n");
#endif

  if (!(bs = look->bsWait))
    messcrash ("screwup in addData - no bs to add to") ;


  if (bs->key < _LastC)
    { bs->bt->cp = (char*) messalloc (strlen (text) + 1) ;
      strcpy (bs->bt->cp, text) ;
    }
  else if (bs->key == _Int)
    {
      if (sscanf (text,"%d",&i))
	bs->n.i = i ;
      else
	{ messout ("Sorry - not a good integer") ; return ; }
    }
  else if (bs->key == _Float)
    {
      if (sscanf (text,"%f",&f))
	bs->n.f = f ;
      else
	{ messout ("Sorry - not a good floating point number") ; return ; }
    }
  else if (bs->key == _DateType)
    {
      if ((tm = timeParse (text)))
	bs->n.time = tm ;
      else
	{ messout ("Sorry - not a good date") ; return ; }
    }
  else
    messcrash ("Bad data type %d in treedisp addData",bs->key) ;


  path_to_updated = getPathFromBS(bs) ;


  bs->size |= NEW_FLAG ;
  bs->timeStamp = sessionUserKey() ;
  bs->size &= ~(ADD_DATA_FLAG | EDIT_NODE_FLAG) ;	/* only get here if we added OK */
  look->bsWait = NULL ;

  look->text_box = 0 ;

  fixPath (look, bs) ;


  /* Reset activebox from the path that we recorded at the start, if we can move right or down
   * from the data we added then we do, otherwise we just stay where we are. */
  if ((active_bs = getBSFromPath(look, path_to_updated)))
    {
      if (active_bs->right)
	active_bs = active_bs->right ;
      else if (active_bs->down)
	active_bs = active_bs->down ;
  
      restoreActiveBS(look, active_bs) ;
    }
  stackDestroy(path_to_updated) ;

  graphRegister (KEYBOARD, newKbd) ;

  return ;
}

/*******************/

static void editorOK(void *editor, void *data)
{
   TreeDisp look = (TreeDisp)data;
   
   char *newText = gexEditorGetText(editor, 0);
   graphActivate(look->graph); /* for editEntry */
   editEntry(newText);
   /* look->bsWait zeroed by addKey via editEntry */
   messfree(newText);
   messfree(editor);
   look->editor = 0;

   return ;
}

static void editorCancel(void *editor, void *data)
{
  TreeDisp look = (TreeDisp)data;
   
  look->bsWait = 0;
  messfree(editor);
  look->editor = 0;
  
  return ;
}

static void editEntry (char *text)
{
  BS bs, bsm=0 ;
  KEY key ;
  OBJ obj ;
  int table ;
  TreeDisp look = currentTreeDisp("editEntry") ;

  /* This test means that it is not possible to enter either a null string or blanks
   * (that's what lexIsGoodName tests basically). */
  if (!*text || !lexIsGoodName(text))
    {
      while(*text) 
	*text++ = 0 ;  /* necessary to set totally to 0 for graphTextEntry */
      graphRedraw() ;

      return ;
    }

  if (!(bs = look->bsWait))
    messcrash ("screwup in editEntry - no bs to edit") ;

  if (bs->key < _LastN)
    {
      addData (text) ;
    }
  else
    {
      table = class(bs->key) ;

      if (!lexword2key(text,&key,table))
	{
	  if (!strcmp(text,"*") || !strcmp(text,"?"))
	    {
	      messout("Please do not use ? or * as the name of an object, it would confuse subsequent searches") ;

	      return ;
	    }
	  else if (!pickXref(table) && !messQuery ("Unknown name - do you want to create a new object? "))
	    return ;
	  else
	    lexaddkey (text,&key,table) ;
	}

      if (pickXref(table))	/* especially comments! */
	{
	  obj = look->objet ;
	  if (!obj->xref)
	    obj->xref = stackCreate (64) ;
	  push (obj->xref, bs->key, KEY) ;
	  push (obj->xref, (_Quoted_in | DELETE_BIT), KEY) ;
	}
      else 
	{
	  if (!bs->bt || !(bsm = bs->bt->bsm))
	    messcrash ("screwup in editEntry - bsm is missing") ;

	  if (KEYKEY(bsm->n.key))		/* must delete XREF */
	    {
	      obj = look->objet ;
	      if (!obj->xref)
		obj->xref = stackCreate (64) ;
	      push (obj->xref, bs->key, KEY) ;
	      push (obj->xref, (KEYKEY(bsm->n.key) | DELETE_BIT), KEY) ;
	    }
	}

      addKey (key) ;
    }

  graphRegister (KEYBOARD, newKbd) ;

  return ;
} /* editEntry */

/************************************************************/
/************************************************************/

static void chooseTagCancel(void)
{ TreeDisp look = currentTreeDisp("chooseTagCancel") ;

  if (look->bsWait)
     return ;
  
  graphLoopReturn(FALSE) ;
}

static void choosePick(int box, double x_unused, double y_unused, int modifier_unused)
{ BS bs ;
  TreeDisp look = currentTreeDisp("choosePick") ;


  if (look->bsWait)
     return ;

  if (!box)
    return ;

  if (box >= arrayMax (look->content))
    {
      messerror("updatePick received a wrong box number %d",box);
      return ;
    }


  if (box == look->activebox)         /* a second hit - follow it */
    {
      bs = arr(look->content,look->activebox,BS) ;

      if (!bs)	      /* not a bs box, e.g. inside a textEntry */
	return ;

      if (bs == look->objet->root)
	return ;

      if (isDisplayBlocked())	/* from different update window */
	{
	  messout ("First deal with the other object you are waiting on") ;
	  return ;
	}

      if (bs->size & SUBTYPE_FLAG)        /* a type */
	{
	  bsSubFuseType (look->objet, bs) ;
	  lookDraw (look) ;
	}
      else 
	{
	  while (bs->size & MODEL_FLAG)	/* as in fixPath, but no UNIQUE problems */
	    {
	      bs->size &= ~MODEL_FLAG ;
	      while (bs->up && bs->up->down == bs)
		bs = bs->up ;
	      if (bs->up)
		bs = bs->up ;
	    }
	  graphLoopReturn(TRUE) ;
	}
     }
  else                              /* change highlighted entry */
    {
      colourActive (look, FALSE) ;
      look->activebox = box ;

      bs = arr(look->content,box,BS) ;

      if (!bs)		/* not a bs box */
	look->activebox = 0 ;

      colourActive (look, TRUE) ;
    }

  return;
} /* choosePick */

/************/

static MENUOPT chooseMenu[] = {
  { chooseTagCancel, "Cancel" },
  { help,            "Help" },
  { graphPrint,      "Print" },
  { 0, 0 }
} ;

/************/

static TreeDisp chooseCreate (KEY key, OBJ objet, Graph oldGraph)
{ 
  TreeDisp look=(TreeDisp)messalloc(sizeof(struct TreeDispStruct)) ;

  look->graph = displayCreate ("TREE");

  graphSetBlockMode(GRAPH_BLOCKING) ; /* User must answer tree choose dialog. */
  graphRetitle (messprintf("Tag chooser: Class %s", className(key))) ;
  graphRegister (DESTROY, lookDestroy) ;
  graphRegister (PICK, choosePick) ;
  graphMenu (chooseMenu) ;
  graphHelp("Tag-chooser") ;

  look->magic = &TreeDisp_MAGIC;
  look->key = key;
  look->objet = objet ;
  look->tagChooserMode = TRUE ;

  graphAssociate (GRAPH2TreeDisp_ASSOC, look) ;

  look->content = arrayReCreate(look->content, 32,BS);

  look->bs2box = assReCreate (look->bs2box) ;
  look->first_box_BS = look->last_box_BS = NULL ;

  look->tag2box = assReCreate(look->tag2box);

  if (look->textStack)
    stackDestroy (look->textStack) ;

  lookDraw (look) ;
  graphButtons (chooseMenu, 10, 1, 55) ;
  if (xmax < 65)
    graphTextBounds (65, ymax) ;


  return look ;
} /* chooseCreate */

/************/

BOOL treeChooseTagFromModel(int *type, int *targetClass, int classe, 
			    KEY *tagp, Stack s, int continuation)
{ Graph oldGraph = graphActive() ;
  Stack s1 ; Array toto ;
  BOOL lastTag = FALSE, contNeeded = FALSE ;
  TreeDisp look ;
  KEY key ; int i ;
  OBJ obj ;
  BS bs ; char *cp ;
  
  lexaddkey("Tag-chooser", &key, classe) ;
  obj = bsUpdate (key) ;

  while (bsGetKeyTags (obj, _bsRight, 0))
    bsRemove (obj) ;
  if (continuation && *tagp)
    bsAddTag (obj, *tagp) ;
  cacheMark (obj->x) ;
  bsFuseModel (obj, *tagp, treeJustKnownTags ? continuation : 0) ;
  
  look = chooseCreate(key, obj, 0) ;
  /* setOnFlag (look->objet->root) ;  */
  if (!look)
    { bsKill(obj) ;
      return FALSE ;
    }

  messStatus ("Please choose a Tag") ;

  if (!graphLoop(TRUE))		/* blocking loop was cancelled */
    { 
      if (graphActivate(look->graph))
	{
	  /* if we got here by graphLoopReturn(FALSE), which is called
	   * by the cancel button, our graph is still there,
	   * and we have to kill it ourselves.. */
	  look->objet = 0 ; /* no silly questions in graphDestroy */
	  look->tagChooserMode = FALSE ;
	  graphDestroy();
	}
      /* Otherwise we got here, because the window was graphDestroy'ed
       * which implies the graphLoopReturn(FALSE), but we've already 
       * done everything we need to do, i.e. lookDestroy etc. */

      if (obj->magic)
	bsKill(obj) ;
      graphActivate(oldGraph) ;
      graphPop() ;
      return FALSE ;
    }

  /* blocking loop returned TRUE */
  
  look->objet = 0 ; /* no silly questions in graphDestroy */
  look->tagChooserMode = FALSE ;
  if (graphActivate(look->graph))
    graphDestroy();

  defuse(obj->root) ;
  bs= obj->root ;
  lastTag = FALSE ;
  
  s1 = stackCreate(32) ;
  while (bs->right)
    bs = bs->right ;
  if (!bs->up)
    messcrash("No bs->up in treeChooseTagFromModel") ;
  
  if (bsIsTag(bs))
    { *type = 'b' ;
      lastTag = TRUE ;
      *tagp = bs->key ;
      pushText (s1, name(bs->key)) ;
    }
  else if (bs->key == _Text)
    *type = 't' ;
  else if (bs->key == _Int)
    *type = 'i' ;
  else if (bs->key == _Float)
    *type = 'f' ;
  else if (bs->key == _DateType)
    *type = 'd' ;
  else if (class(bs->key))
    { *type = 'k' ;  
      *targetClass = class(bs->key) ;
    }

  while (bs = bs->up , bs->up)
    { if (bsIsTag(bs))
	{ if (!lastTag)
	    pushText (s1, name(bs->key)) ;
	  *tagp = bs->key ;
	  lastTag = TRUE ;
	}
      else 
	{ if (lastTag)
	    pushText (s1, " HERE  # ") ;
	  else
	    pushText (s1, " HERE ") ;	    
	  contNeeded = TRUE ; 
	  break ;
	}
    }

  stackCursor(s1, 0) ;
  toto = arrayCreate(32, char*) ;
  i = 0 ;
  while ((cp = stackNextText(s1)))
    array(toto, i++, char*) = cp ;
  
  if (i--)
    pushText(s, arr(toto,i,char*)) ;
  while(i--)
    catText(s, arr(toto,i,char*)) ;
  stackDestroy(s1) ;
  arrayDestroy(toto) ;

  bsKill(obj) ;

  if (contNeeded && continuation != 2)
    {
      messout ("Except from a derived column of the table maker, "
	       "You must choose a rooted tag") ;
      return FALSE ;
    }

  graphActivate(oldGraph) ;
  graphPop() ;

  return TRUE ;
} /* treeChooseTagFromModel */

/*********************************************************/

static void treeDispMailer (KEY key)
     /* called by clicking on the special action tag 'E-mail' */
{
  OBJ  obj = bsCreate(key), obj1 ;
  ACEIN addr_in = NULL;
	
  if (!obj)
    return ;

  if (!bsGetData (obj, _E_mail, _Text, 0))
    { 
      addr_in = messPrompt  ("Please specify an email address",
			     "", "w", 0);
      if (!addr_in)
	goto fin;
      if (!(obj1 = bsUpdate (key)))
	goto fin ;
	
      if (!bsAddData (obj1, _E_mail, _Text, aceInWord(addr_in)))
	{ 
	  bsDestroy (obj1) ;
	  messout ("Sorry, I can't save your address") ;
	  goto fin ;
	}
      bsSave (obj1) ;
    }

  acedbMailer (key, 0, 0) ;

 fin:
  bsDestroy (obj);
  if (addr_in)
    aceInDestroy (addr_in);
}

/* At present, the Find function only scans the tree as it is currently 
** displayed.  It doesn't expand any nodes which are folded away. 
**
** Broad functionality: Control/F or menu to open a dialogue box where
** you enter the string you're looking for.  Or you can click an object
** and Next/Prev to others.  Next/Prev looks for the next/prev occurrence
** of the current string.  At either the end of the tree you get a message
** but if you Next/Prev again the search wraps.
**
** Leading and trailing wildcards are supported, though Lead* processing is 
** simplistic and could be improved.
*/
static void FindObject(TreeDisp look, AceFindDirection direction)
{
  ACEIN name_in;
  AceWild wildcard = NEITHER;
  char *pattern;                           /* used in stripping out wildcards */
  
  /* messPrompt displays whatever is in param2 as the default */
  if ((name_in = messPrompt("Find", look->pattern, "w", 0)))
    {  
      if (look->tagWarp)
	messfree(look->tagWarp);

      look->keyClicked = FIND;  
      
      look->tagWarp = strnew(aceInWord(name_in), 0);
      look->pattern = strnew(look->tagWarp,0);      /* save for next time the prompt box shows */
  
      /* strip out wildcards */
      if (look->tagWarp[0] == '*')
	{
	  wildcard = LEADING;
	  pattern = strnew(look->tagWarp+1,0);
	  strcpy(look->tagWarp, pattern);
	}
      if (look->tagWarp[strlen(look->tagWarp)-1] == '*')
	{
	  look->tagWarp[strlen(look->tagWarp)-1] = '\0';
	  if (wildcard == LEADING)
	    wildcard = BOTH;
	  else
	    wildcard = TRAILING;
	}
      if (strstr(look->tagWarp,"*"))
	{
	  messout("* not permitted embedded in search pattern");
	  return;
	}

      /* find the box in the tree display */
      if (!FindBox(look, direction, wildcard))
	messout("Can't find object %s", look->pattern);

      aceInDestroy(name_in);
    }
  return;
}


BOOL FindBox(TreeDisp look, AceFindDirection direction, AceWild wildcard)
{
  float x1, y1;                         /* screen coordinates of box                 */
  static char *text;                    /* text we seek                              */
  static BOOL AtEnd = FALSE;            /* whether we're at the end of the list      */
  static int PrevDir = FIND_BACKWARD;   /* search direction last time we were here   */
  static AceWild PrevWild = NEITHER;    /* the nature of any wildcard                */
  int box = -1;
  static int PrevBox = 0;
  int ClickedBox = 0;                   /* if user clicks box & Next/Prev        */
  static int i = 0;                     /* remembers where you are in the arrays */
  char *ActiveText;                     /* the text from the current box         */

  /* if there's no active box and no tagWarp (ie find target) 
  ** then we don't know what we seek, so mak that plain. 
  */
  if ( !look->tagWarp && look->activebox == 0 )                 
    messout("Please use Find before trying Next/Previous");
  else  
    {       
      /* if the user has clicked a box and hit Next/Prev, 
      ** then we'llhave an activebox, but no tagWarp.  */
      if (look->keyClicked == SELECT && look->activebox && look->activebox != PrevBox)
	{
	  arrayFind(look->boxes, &look->activebox, &i, intOrder);        /* must be there! */
	  text = strnew(array(look->names,i,char*),0);     /* store the text of active box */
	  ClickedBox = look->activebox;                 /* skip the one you're actually on */
	  look->pattern = strnew(text,0);               /* store search string for msg box */
	  AtEnd = FALSE;                          
	  wildcard = NEITHER;
	}
      /* if user clicked a box and is now Next/Prev'ing, text will be set */
      else if (!text || look->keyClicked == FIND)
	{  
	  text = strnew(look->tagWarp,0);
	  AtEnd = TRUE;                                 /* force it to start again */
	}

      if (wildcard == UNDEFINED)
	wildcard = PrevWild;
      PrevWild = wildcard;

      /* if we're looking again for the same thing 
      ** and we're not at the end of the array...
      */
      if ((text && *text && look->tagWarp && strcasecmp(text,look->tagWarp)==0) && !AtEnd)
	{
	  if (direction == FIND_FORWARD)
	    {
	      for (; i < arrayMax(look->names); i++)  /* i should already be set */
		if (examineElement(arr(look->names,i,char*), text, wildcard))
		  {
		    box = arr(look->boxes, i, int);
		    if (box != PrevBox)
		      {
			PrevBox = box;
			break;          
		      }   
		  }
	      if (i >= arrayMax(look->names))
		{
		  messout("That was the last %s", look->pattern);
		  AtEnd = TRUE;
		}
	    }
	  else
	    {
	      for (; i >= 0; i--)                        /* i should already be set */
		if (examineElement(arr(look->names,i,char*), text, wildcard))
		  {
		    box = arr(look->boxes, i, int);
		    if (box != PrevBox)
		      {
			PrevBox = box;
			break;          
		      }   
		  }
	      if (i <= 0)
		{
		  messout("That was the first %s", look->pattern);
		  AtEnd = TRUE;
		}
	    }
	}
      else /* either we have a new target or we're at the end of the array */
	{ 
	  if (look->tagWarp)
	    messfree(look->tagWarp);                                
	  look->tagWarp = strnew(text,0);         /* store target in look */

	  if (direction == FIND_FORWARD)
	    {
	      for (i = 0; i < arrayMax(look->names); i++)
		{
		  if (look->activebox && look->activebox == arr(look->boxes, i, int))
		    ActiveText = arr(look->names, i, char*);

		  if (examineElement(arr(look->names,i,char*), text, wildcard))
		    {
		      box = arr(look->boxes, i, int);
		      if (box != PrevBox && box != ClickedBox)
			{
			  PrevBox = box;
			  break;          
			}   
		      else
			box = -1;
		    }
		}
	      if (i >= arrayMax(look->names))
		{
		  if (look->activebox && ActiveText && box == -1)
		    {
		      if (strcmp(ActiveText, text)==0)
			messout("That was the only %s", look->pattern);
		      else
			messout("Can't find %s", look->pattern);

		      AtEnd = TRUE;
		    }
		}
	      else
		AtEnd = FALSE;

	    }
	  else
	    {
	      for (i = arrayMax(look->names)-1; i >= 0; i--)
		{
		  if (look->activebox && look->activebox == arr(look->boxes, i, int))
		    ActiveText = arr(look->names, i, char*);

		  if (examineElement(arr(look->names,i,char*), text, wildcard))
		    {
		      box = arr(look->boxes, i, int);
		      if (box != PrevBox && box != ClickedBox)
			{
			  PrevBox = box;
			  break;          
			}  
		      else
			box = -1; 
		    }
		}
	      if (i <= 0)
		{
		  if (look->activebox && ActiveText && box == -1)
		    {
		      if (strcmp(ActiveText, text)==0)
			messout("That was the only %s", look->pattern);
		      else
			messout("Can't find %s", look->pattern);

		      AtEnd = TRUE;
		    }
		}
	      else
		AtEnd = FALSE;
	    }
	}
      
      PrevDir = direction;                  /* remember which direction we're going */
      
      if (box >= 0)
	{
	  graphBoxDim(box, &x1, &y1, NULL, NULL);
	  graphGoto(x1, y1);
	  colourActive(look, FALSE);         /* unhighlight box */
	  look->activebox = 0;               /* change selected box */
	  lookPick(box, 0, 0, 0);
	  return TRUE;
	}
    }
  return FALSE;
}

BOOL examineElement(char *element, char *pattern, AceWild wildcard )
{
  if (wildcard == LEADING)
    {
      if (strcasestr(element, pattern))
	return TRUE;
    }
  else if (wildcard == TRAILING)
    {
      if (g_strncasecmp(element, pattern, strlen(pattern)) ==0)
	return TRUE;
    }
  else
    if (strcasecmp(element,pattern) == 0)
      return TRUE;

  return FALSE;
}


static void revertDisplayToDefault(TreeDisp look)  /* does what it says on the tin */
{
  look->activebox = 0;
  clearFlags(look->objet->root);
  setOnFlag(look->objet->root);
  lookDraw(look);
  return;
}

/* lifted straight from fmaposp.c */
static int intOrder(void *a, void *b)
{
  return
    (int)  ( (*(int *) a) - (*(int *) b) ) ;
} /* intOrder */


/* treeUpdate Save function moved here so it can be called directly by keyboard shortcut */
static void updateSave(TreeDisp look, BitSet bb)
{
  defuse (look->objet->root) ;

  if (!bbSetArray)   /* mieg, inertia in open tags */
    bbSetArray = arrayCreate (256, BitSet) ;
  bb = array(bbSetArray, class(look->key), BitSet) ;

  if (!bb)
    bb =  array(bbSetArray, class(look->key), BitSet) = bitSetCreate(lexMax(0),0) ;
  getOnFlagSet (look->objet->root, bb) ;

  bsSave (look->objet) ;
  look->objet = bsCreateCopy (look->key) ;

  /* If the object exists only in the cache, and is empty, it will
     evaporate, if so put it back there (see displayKey in newkey.c) */
  if (!look->objet)
    { 
      OBJ obj = bsUpdate(look->key) ;   /* to give it a cache entry */
      bsDestroy (obj) ;
      look->objet = bsCreateCopy (look->key) ;
    }  

  clearFlags (look->objet->root) ;
  setOnFlagSet (look->objet->root, bb) ;

  look->activebox = 0 ;
  look->updateMode = FALSE ;

  lookDraw(look) ;

  graphRegister (PICK, lookPick) ;
  graphRegister (KEYBOARD, newKbd) ;

  graphFreeMenu (lookMenu, treeMenu) ;
  graphHelp ("Tree") ;

  return ;
}

/* If we are not in update mode we just return the current look.
 * otherwise we cancel the current edit if one is active,
 * if none is active we exit update mode without saving and return
 * a new look (this seems excessive but perhaps clearing up is just
 * too complex...???). */
static TreeDisp cancelEdit(TreeDisp look_in)
{
  TreeDisp look = look_in ;

  if (look->updateMode)
    {
      if (look->bsWait)
	{
	  if (look->bsWait->size & ADD_KEY_FLAG)
	    displayUnBlock() ;				    /* clears display block */

	  look->bsWait->size &= ~WAITING_FLAGS ;	    /* clears edit and fake node flags */

	  look->bsWait = 0 ;

	  lookDraw(look) ;
	}
      else
	{
	  KEY orig_key = look->key ;

	  graphDestroy();

	  treeDisplay(orig_key, 0, FALSE, 0);

	  look = currentTreeDisp("cancelEdit") ;
	}
    }

  return look ;
}


static void toggleTimeStamps(TreeDisp look)
{
  BS active_BS = NULL ;

  showTimeStamps = showTimeStamps ? FALSE : TRUE ;

  /* If there's an active box remember it's BS. */
  if (look->activebox)
    active_BS = arr(look->content, look->activebox, BS) ;

  lookDraw(look) ;

  /* Now restore the active BS. */
  if (active_BS)
    {
      if (!restoreActiveBS(look, active_BS))
	messcrash("Could not restore active BS on graph.") ;
    }

  return ;
}


static BOOL restoreActiveBS(TreeDisp look, BS active_BS)
{
  BOOL result = FALSE ;
  char *vp ;

  if (assFind(look->bs2box, active_BS, &vp))
    {
      int box ;

      box = assInt(vp) ;

      colourActive(look, FALSE) ;

      look->activebox = box ;

      colourActive(look, TRUE) ;

      result = TRUE ;
    }

  return result ;
}


/* Attempts to do something sensible with the current tag, if it's collapsed it expands it,
 * if it's an object it displays it and so on.... */
static void showTag(TreeDisp look, BS bs)
{

  if (bs->size & ON_FLAG) /* if hidden, show branch */
    {
      bs->size ^= ON_FLAG ;
      lookDraw(look) ;
    }
  else if (bs->key == _Pick_me_to_call && !look->updateMode && !look->tagChooserMode)
    {
      externalDisplay(look->key) ;
    }
  else if (bs->key == _Gel && !look->updateMode && !look->tagChooserMode)
    {
      fpDisplay(look->key) ;

      /* Make sure our graph is still active otherwise user loses interactivity ! */
      graphActivate(look->graph) ;
    }
  else if (bs->key == _E_mail &&  !look->updateMode && !look->tagChooserMode)
    {
      treeDispMailer (look->key) ;
    }
  else if (class(bs->key))
    {
      /* Must preserve because if preserve is "none" but this display call ends up as treeview
       * display we can inadvertantly reuse our own window and hence destroy this look ! */
      displayPreserve() ;

      display(bs->key,look->key, 0) ;

      /* Make sure our graph is still active otherwise user loses interactivity ! */
      graphActivate(look->graph) ;
    }
  else if (bs->right && !look->tagChooserMode)
    {
      bs->size ^= ON_FLAG ;
      lookDraw(look) ;
    }


  return ;
}


static void deleteNode(TreeDisp look)
{
  if (look->bsWait)
    {
      messout("Finish what you have started first, or cancel") ;
    }
  else if (!look->activebox)
    {
      messout("You must first select a node to prune (single-click)") ;
    }
  else
    {
      BS bs = NULL ;

      bs = arr(look->content,look->activebox,BS) ;

      if (bs->size & MODEL_FLAG)
	{
	  messout("You can only delete parts of the original object") ;
	}
      else
	{
	  look->objet->curr = bs ;
	  look->objet->modCurr = (bs->bt) ? bs->bt->bsm : 0 ;

	  defuse (look->objet->root) ;
	  bsRemove (look->objet) ;
	  bsFuseModel (look->objet, 0, look->justKnownTags) ;
	  setOnFlagModelOnly (look->objet->root) ;

	  lookDraw (look) ;
	}
    }

  return ;
}


static void editNode(TreeDisp look)
{
  BS bs = NULL, p = NULL ;

  if (look->bsWait)
    { 
      messout("Finish what you have started first, or cancel") ;
    }
  else if (!look->activebox)
    {
      messout("You must first select a node to edit (single-click)") ;
    }
  else
    {
      bs = arr(look->content,look->activebox,BS) ;
     
      if (bsIsTag(bs))
	{ 
	  messout("You can not edit tags - only object names or data") ;
	}
      else
	{
	  BOOL edit_field = TRUE ;
	  char *text = NULL ;

	  if (!(bs->size & MODEL_FLAG))
	    {
	      if (class(bs->key))
		{ 
		  if (strcmp(className(bs->key), "Text") != 0)
		    {
		      /* Object name other than ?Text */

		      look->bsWait = bs;
		      bs->size |= ADD_KEY_FLAG ;	
		      lookDraw (look) ;

		      edit_field = FALSE ;
		    }
		} 
	      else if ((bs->key >= _LastC))		    /* Don't edit ints and floats this way */
		{ 

		  look->bsWait = bs;
		  bs->size |= ADD_DATA_FLAG ;
		  lookDraw (look) ;

		  edit_field = FALSE ;
		}

	      if (edit_field)
		text = bs2Text(bs, dontExpand, isModel) ;
	    }
	  else /* (bs->size & MODEL_FLAG) */
	    { 
	      if (class(bs->key) && (strcmp(className(bs->key), "Text") != 0))
		{ 
		  messout ("You can only edit parts of the original object") ;
		  edit_field = FALSE ;
		}
	      else if (!bsIsTag(bs->up) && (bs->up->size & MODEL_FLAG))
		{
		  edit_field = FALSE ;
		}
	      else if (bs->size & SUBTYPE_FLAG)		    /* a type */
		{
		  edit_field = FALSE ;
		}
	    }
      
	  if (edit_field)
	    {
	      p = bs;
	      while (p->up && p->up->down == p)
		p = p->up;
	      if (p->up && p->up->up)
		p = p->up;
	      look->bsWait = bs ;

	      look->editor = gexTextEditorNew(messprintf("Edit text for tag %s", name(p->key)),
					      text, 0,
					      look, 
					      editorOK, editorCancel,
					      TRUE, TRUE, TRUE) ;
	    }
	}
    }
              
  return ;
}




/* Takes a BS and records the path from it back up to the next Tag using the
 * _bsRight and _bsDown keys, the final key recorded is the Tag key itself.
 * The path is pushed on to a stack so that it can be read in reverse to go
 * from the Tag back down to the BS.
 * 
 * OBVIOUSLY we assume that the tree created by the treedisplay code does not
 * change in between calls to getPathFromBS() and getBSFromPath() !!
 *  */
static Stack getPathFromBS(BS bs)
{
  Stack path = NULL ;
  BS curr ;

  path = stackCreate(30) ;				    /* Should be big enough. */

  curr = bs ;

  while (!isTag(curr) && curr->up)
    {
      BS parent ;
      KEY direction ;

      parent = curr->up ;

      if (parent->right == curr)
	direction = _bsRight ;
      else if (parent->down == curr)
	direction = _bsDown ;
      else
	messcrash("agh......bad bs") ;

      push(path, direction, KEY) ;

      curr = parent ;
    }

  push(path, curr, BS) ;

  return path ;
}

/* Unwinds the stack created from getPathFromBS() to return to the BS originally supplied
 * to that function. (See caveats in comments of that function.) */
static BS getBSFromPath(TreeDisp look, Stack path)
{
  BS bs = NULL ;
  BS start ;
  KEY direction ;

  void *vp ;
  int box ;

  /* First key is bs to locate on.... */
  start = pop(path, BS) ;

  if (assFind(look->tag2box, start, &vp))
    {
      box = assInt(vp) ;
      bs = arr(look->content, box, BS) ;

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
      printf("at %s\n", bs2Text(bs, dontExpand)) ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

      while (!stackEmpty(path) && (direction = pop(path, KEY)))
	{
	  if (direction == _bsRight)
	    bs = bs->right ;
	  else if (direction == _bsDown)
	    bs = bs->down ;
	  else
	    messcrash("agh......bad bs") ;

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
	  printf("%s to %s\n", (direction == _bsRight ? "right" : "down"), bs2Text(bs, dontExpand)) ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */
	}
    }
  else
    {
      messerror("Could not find tag \"%s\" in tree display, cannot restore active box.", name(start->key)) ;
    }

  return bs ;
}



/* looking for a tag in the treeview version of the BS is not the same as looking
 * in an object because the treeview code uses the "size" field to signal things
 * like "this node should be drawn as part of the model" etc etc.... */
static BOOL isTag(BS bs)
{
  BOOL result = FALSE ;

  if (bs->size == MODEL_FLAG || bsIsTag(bs))
    result = TRUE ;
  else
    result = FALSE ;

  return result ;
}






/************************ end of file ********************/
