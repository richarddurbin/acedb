/*  File: ksetdisp.c
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
 * Description: Manipulate Key Sets.
      keyset windows have their own menus and  KEYSET. When they 
      are destroyed they (VERY IMPORTANT) destroy the keyset.
 * Exported functions:
 * HISTORY:
 * Last edited: May 15 13:48 2008 (edgrif)
 *			-	fn() to void(void) to please WIN32 compiler
 * * Jun 10 16:53 1996 (il)
 * * Oct 15 15:15 1992 (mieg): Mailer
 * * Nov  5 18:27 1991 (mieg): I introduced keySetAlpha
 * * Oct 23 18:53 1991 (mieg): changed name dump to a keySetDump
 * * May 1996 (il) : added scroll bar to keyset's and rearranged the menus.
 * *               : ability to select/highlight members of the keyset and 
 * *               : perform the usual operation's on them. 
 * Created: Wed Oct 23 18:52:00 1991 (mieg)
 * CVS info:   $Id: ksetdisp.c,v 1.143 2012-10-22 14:57:57 edgrif Exp $
 *-------------------------------------------------------------------
 */

#include <wh/acedb.h>
#include <wh/aceio.h>
#include <wh/lex.h>
#include <wh/dna.h>
#include <wh/a.h>
#include <wh/bs.h>
#include <wh/session.h>     
#include <wh/query.h>
#include <wh/pick.h>
#include <wh/peptide.h>
#include <wh/alignment.h>
#include <wh/help.h>
#include <wh/parse.h>
#include <wh/dump.h>
#include <wh/biblio.h>
#include <whooks/tags.h>		
#include <whooks/systags.h>
#include <whooks/sysclass.h>
#include <wh/qbe.h>
#include <wh/display.h>
#include <wh/keysetdisp.h>		/* our own public header */
#include <wh/main.h>
#include <wh/tree.h>
#include <wh/forest.h>
#include <wh/tabledisp.h>


/* keySetShow () displays a KEYSET (a sorted array of KEY's) in 
   multicolumn format, allowing the user to pick and move around 
   with arrow keys.  Objects are displayed with default displaytype.

   To initialise the system, call with handle==0.  It returns the new 
   handle.  To reuse the graph call first with (0,handle) to
   kill the alphabetic keyset. The aim of this is to
   allow the user to change the displayed KEYSET.

   keySetGetShowAs() let's you find out which "Show As"-displayType
   has been selected in a given keyset-window.
   It returns NULL by default. This return value can be used
   as the third argument for display(), where NULL also causes
   the default display type to be determined.

   Although we can save to permanent objects (by the menu Save option)
   and recover from them (via display() and keySetDispCreate()),
   a keyset display window is NEVER attached to an object - it always
   deals in copies of the permanent keyset.  This allows much greater
   flexibility in changing the contents of a window, avoids tangles
   with the cache, and is absolutely necessary.
*/


#define COLWIDTH 16
#define MAXOBS 1000
#define SCROLLBARGAP 4
#define SCROLLBARPOS 1.0
#define MINSCROLLBARLENGTH 0.7
#define BOTGAP 1
#define DEFAULT 0

/* check that these numbers are ok in menuOptions */
#define OR 21
#define AND 22
#define XOR 23
#define MINUS 24

#define UP TRUE
#define DOWN FALSE
#define MAX_CLASSES 200

/* rounding - nearest int */
#define nint(x) (((x) > 0) ? ((int)(x + .5)) : -((int)(.5 - x)) )

/************************************************************/

/* complete the opaque datastructure from keysetdisp.h */
struct KeySetDispStruct
{ 
  magic_t *magic;		/* == &KeySetDisp_MAGIC */
  KEYSET kSet , keySetAlpha ,selected;
  BOOL fullAlpha ; /* complete call to keySetAlphaHeap done */
  KEY   curr ;
  KEY   showtype;
  Graph graph, owner ;
  BOOL self_destruct ;					    /* TRUE means graphDestroy() after double click. */
  BOOL  isBlocking ;
  int   base, top ;		/* top is 1 + index of last displayed key */
  Array box2key ;
  BOOL showClass ;
  int showQuery; /* 0: no, 1:yes-nofocus, 2:yes-focus */
  float scrolllen;
  char txtBuffer[60], queryBuffer[256] ;
  int numperpage, colWidth[50];
  int txtbox,numselpage,queryBox;
  int scrollBarBox,upBox,downBox,firstBox,lastBox;
  int itemBoxStart,itemBoxEnd;
  int nitem, nselected ;
  char touched ;
  char *message ; 
  KEY keySetKey ; /* the key if displaying a KEYSET class object */
} ;

static magic_t KeySetDisp_MAGIC = "KeySetDisp";
static magic_t GRAPH2KeySetDisp_ASSOC = "KeySetDisp";

static BOOL isOtherTools = FALSE ; /* extended set of buttons for specialists */
static float TOPGAP ;
static int  N3rdLINE, NQueryLine ;
static int startbox,lastbox,numboxes,oldcol, oldy ;

static KeySetDisp selectedKeySet = 0 ;

static FREEOPT menuOptions[] = { 
  {19,"Key Set ?"},
  {99,"Quit"},
  {98,"Help"},
  {97, "Print"},
  {0,""},
  {7,"New empty keyset"},
  {10,"Import keyset"},
  {2,"Copy whole keyset"},
  {1,"Save as"},  
  {0,""},
  {6,"Add-Remove"},
  {OR,"OR"},  /* reserved 20-26 for these */
  {AND,"AND"},
  {MINUS,"MINUS"},
  {XOR,"XOR"},
  {0,""},
/* 47,"Show/Hide Classes", */
  {41,"Mail"},  
  {51,"Keyset Summary"},
  {45,"Show as Multimap"},
  {8,"Show as Text"},
} ;

static void localDestroy (void) ;
static void localPick (int box,double x,double y, int modifier_unused) ;
static void localKbd (int k, int modifier_unused) ;
static void ksetlocalMenu (KEY k, int unused) ;
static void pageUp(void), pageDown(void) ;
static KeySetDisp keySetShow2 (KEYSET kSet, KeySetDisp look) ;
static void ksetAddKey (KEY key) ;
static void ksetDeleteKey (KEY key) ;
static void ksetUnBlock (void) ;
static void controlLeftDown (double x, double y);
static void controlLeftUp (double x, double y);
static void controlLeftDrag (double x, double y);
static void selectedToNewKeySet(void);
static BOOL columnWidth(int startpos,int direction,int *width,int *endpos,int numrows);
static void drawMenus(void);
static void drawScrollBar(int max);
static void keySetMix(KeySetDisp qry, int combinetype);
static void keySetSummary(void);
static void changeShowtype(KEY key);
static void nextShowtype(void);
static void scrollMiddleUp (double x, double y);
static void keySetSelect (void);
static BOOL importKeySet(KEYSET kSet, ACEIN keyset_in);

/* Dumping routines.                                                         */
static void dumpAsDnaFasta(void) ;
static void dumpAsCDSDnaFasta(void) ;
static void dumpDnaFasta(BOOL cds_only) ;

static void dumpAsProtFasta(void) ;
static void dumpAsCDSProtFasta(void) ;
static void dumpProtFasta(BOOL cds_only) ;

/************************************************************/

static KeySetDisp currentKeySetDisp (char *caller)
{
  KeySetDisp ksdisp;

  if (!graphAssFind (&GRAPH2KeySetDisp_ASSOC,&ksdisp))
    messcrash("%s() could not find KeySetDisp on graph", caller);

  if (!ksdisp)
    messcrash("%s() received NULL KeySetDisp pointer", caller);
  if (ksdisp->magic != &KeySetDisp_MAGIC)
    messcrash("%s() received non-magic KeySetDisp pointer", caller);
  
  return ksdisp;
} /* currentKeySetDisp */


static int scrollTriangle(BOOL direction, float x, float y, int colour)
{ 
  int i = 3, box,col;
  float incr;
  Array temp ;

  if(direction)
    incr = -1.0;
  else
    incr = 1.0;

  box = graphBoxStart();
  col = graphColor(colour);

  temp = arrayCreate(2*i, float) ;
  array(temp, 0, float) = x;
  array(temp, 1, float) = y;
  array(temp, 2, float) = x+2.0;
  array(temp, 3, float) = y;
  array(temp, 4, float) = x+1.0;
  array(temp, 5, float) = y+incr;
  graphPolygon(temp);
  col = graphColor(col);

  graphLine(x, y, x+2.0, y);
  graphLine(x+2.0, y, x+1.0, y+incr);
  graphLine(x+1.0, y+incr, x, y);
  arrayDestroy(temp);
  
  graphBoxEnd();
  
  return box;
}

/******************************/

static int nItem (KeySetDisp look)
{ 
  int i, nitem = 0 ;

  if (!(look->touched & 2)) return look->nitem ;
  if (look->fullAlpha && keySetExists(look->keySetAlpha))
    return keySetMax(look->keySetAlpha) ;
  
  if (keySetExists(look->kSet))
    { KEY *kp = arrp (look->kSet, 0, KEY) - 1 ;
    
      i = keySetMax(look->kSet) ;
      while (kp++, i--)
	if (lexIsKeyVisible(*kp)) nitem++ ;
    }
  look->nitem = nitem ;
  look->touched &= 0x1 ;  /* not 2  */
  return nitem ;
}


/* mhmp 04.09.98 forcement necessaire comme nItem */
static int nSelected (KeySetDisp look)
{ 
  int i, nselected = 0 ;

  if (!(look->touched & 1)) return look->nselected ;
  if (keySetExists(look->selected))
    { KEY *kp = arrp (look->selected, 0, KEY) - 1 ;
    
      i = keySetMax(look->selected) ;
      while (kp++, i--)
	if (*kp && lexIsKeyVisible(*kp)) nselected++ ;
    }
  look->nselected = nselected ;
  look->touched &= 0x2 ;  /* not 1  */
  return nselected ;
}

static void shownItem (KeySetDisp look)
{

  if (look->touched)
    {
      nItem (look) ;
      nSelected (look) ;
    }
  if(look->numselpage == look->nselected)
    sprintf(look->txtBuffer,"%d items %d selected", look->nitem, look->nselected);
  else
    sprintf(look->txtBuffer,"%d items %d selected (%d off screen)", 
    look->nitem, look->nselected,look->nselected-look->numselpage) ;
}


/******************************/
/** fake biblio functions   **/
/** for model free kernel   **/
/******************************/
static BOOL setPage(float *z)
{
  int w,h ;
  float temp,top,bottom;
  int max;

  KeySetDisp look = currentKeySetDisp ("setPage") ;

  if (look != selectedKeySet)
    keySetSelect () ; 

  look->curr = 0;
  max = nItem (look) ;
  graphFitBounds(&w,&h);

  top  =  (float)TOPGAP;
  bottom = (float)(h - BOTGAP);

  if(*z<=top)
    look->base = 0;
  else if(*z+look->scrolllen >= bottom){    
    look->base = max;
    if (!look->fullAlpha &&
	keySetMax(look->keySetAlpha) < keySetMax(look->kSet) &&    /* load extra items needed */
	keySetMax(look->keySetAlpha) < look->base + 200)
      { keySetDestroy(look->keySetAlpha) ;
	look->keySetAlpha 
	  = keySetAlphaHeap(look->kSet, keySetMax(look->kSet)) ;
	look->fullAlpha = TRUE ;
      }
  }
  else{
    if(look->scrolllen==MINSCROLLBARLENGTH)
      temp = (*z-top)/(bottom-top-look->scrolllen);
    else
      temp = (*z-top)/(bottom-top);
    look->base = (int)(temp * max);
    if(look->base > max){
      look->base=max;
    }
  }
  return TRUE;
}

static void keyBandDrag(float *x, float *y, BOOL isUP)
{
  float top,bottom;
  int nx,ny;
  KeySetDisp look = currentKeySetDisp ("keyBandDrag") ;
  *x =  0.5; /* x is set to the middle */

  top  =  (float)TOPGAP;
  graphFitBounds (&nx, &ny) ;
  bottom = (float)(ny - BOTGAP);

  if(*y > bottom - look->scrolllen)
    *y = bottom - look->scrolllen;
  if(*y < top )
    *y = top;
  if(isUP) /* after scrolling has finished */
    {
      if(setPage(y)){
	if(look->base == keySetMax(look->keySetAlpha)){
	  --look->base;
	  pageUp();
	  return;
	}
	if (look != selectedKeySet)
	  keySetSelect () ; 
	keySetShow2 (look->kSet, look) ;
      }
    }
}

static void pageUp(void)
{
  int nx,ny,colX,startpos,endpos,numrows,cWidth;
  BOOL room= TRUE;
  KeySetDisp look = currentKeySetDisp ("pageUp") ;

  if (look->base)
    {
      graphFitBounds (&nx, &ny) ;
      colX = SCROLLBARGAP;
      startpos = look->base+1; /* il +1 to keep last item still on page */
      numrows = ny-(TOPGAP+BOTGAP);
      while(room)
	{    
	  startpos--;
	  room = columnWidth(startpos,FALSE,&cWidth,&endpos,numrows);
	  /* mhmp comme dans keySetShow2 */
	  if((cWidth && (colX + cWidth) < nx) || colX == SCROLLBARGAP)
	    { /* if it fits or is the first */ 
	      startpos= endpos;
	      look->base = endpos;
	    }
	  else
	    room = FALSE;
	  colX += cWidth +2;
	}
      if(look->base < 0) /* returns -1 if before start of first page */
	look->base = 0;
      look->curr = 0;
      keySetShow2 (look->kSet, look) ;
    }
} /* pageUp */

static void pageDown(void)
{
  KeySetDisp look = currentKeySetDisp ("pageDown") ;
  if (look->top+1 < keySetMax(look->keySetAlpha))
    { 
      look->base = look->top ; /* removed +1 to keep last item on next page */
      look->curr = 0;
      keySetShow2 (look->kSet, look) ;
    }
} /* pageDown */

static void localResize (void)
{
  KeySetDisp look = currentKeySetDisp("localResize") ;

  look->curr = 0;
  keySetShow2 (look->kSet, look) ;
}

static void none(void)
{
}

static void keySetDispDump (KeySetDisp look, 
			    BOOL isAceDump,
			    BOOL isTimeStamps)
			    
{
  /* .ace dump */
  static char directory[DIR_BUFFER_SIZE] = "";
  static char filename[FIL_BUFFER_SIZE] = "";
  ACEOUT dump_out;
  register int i;
  char *prompt;

  if (!look->keySetAlpha)
    return ;

  prompt = messprintf ("Where do you wish to export the %s ?",
		       isAceDump ? ".ace-file data" : "keyset list");

  dump_out = aceOutCreateToChooser (prompt,
				    directory, filename, "ace", "w", 0);
  if (!dump_out)
    return;

  if (!look->fullAlpha &&
      keySetMax(look->kSet) > keySetMax(look->keySetAlpha))
    { 
      keySetDestroy(look->keySetAlpha) ;
      look->keySetAlpha = 
	keySetAlphaHeap(look->kSet, keySetMax(look->kSet)) ;
      look->fullAlpha = TRUE ;
    }
  
  if (isAceDump)
    /* write out a full acedump of every object */
    {
      dumpTimeStamps = isTimeStamps ;
      
      aceOutPrint (dump_out, "// data dumped from keyset display\n\n") ;
      for (i = 0 ; i < keySetMax(look->keySetAlpha) ; ++i)
	if (!dumpKey (dump_out, keySet(look->keySetAlpha,i)))
	  if (!messQuery("Do you want to proceed ?"))
	    break ;
      
      dumpTimeStamps = FALSE;
    }
  else
    /* write out the acedump of the keyset itself */
    {
      aceOutPrint (dump_out, "KeySet : \"%s\"\n",filename) ;
      keySetDump(dump_out, look->keySetAlpha) ;
    }
  
  aceOutDestroy (dump_out);

  return;
} /* keySetDispDump */

/************************************************************/

static void dumpAsAce (void)
     /* menu/button func */
{
  KeySetDisp look = currentKeySetDisp("dumpAsAce");
  
  keySetDispDump (look, TRUE, FALSE);

  return;
} /* dumpAsAce */

/************************************************************/

static void dumpAsAceWithTimeStamps (void)
     /* menu/button func */
{
  KeySetDisp look = currentKeySetDisp("dumpAsAceWithTimeStamps");
  
  keySetDispDump (look, TRUE, TRUE);

  return;
} /* dumpAsAceWithTimeStamps */

/************************************************************/

static void dumpAsNames (void)
     /* menu/button func */
{
  KeySetDisp look = currentKeySetDisp("dumpAsNames");
  
  keySetDispDump (look, FALSE, FALSE);

  return;
} /* dumpAsNames */

/************************************************************/
/* FastA format sequence dump, either all of the dna or just the CDS part.   */
/*                                                                           */
/* menu/button func */
static void dumpAsDnaFasta (void)
{
  dumpDnaFasta(FALSE) ;

  return ;
}

/* menu/button func */
static void dumpAsCDSDnaFasta (void)
{
  dumpDnaFasta(TRUE) ;

  return ;
}

static void dumpDnaFasta(BOOL cds_only)
{
  ACEOUT dna_out;
  KeySetDisp look = currentKeySetDisp("dumpDnaFasta");

  if (!look->keySetAlpha)
    return ;

  if (!(dna_out = dnaFileOpen(0)))
    return ;

  if (!look->fullAlpha &&
      keySetMax(look->kSet) > keySetMax(look->keySetAlpha))
    {
      keySetDestroy(look->keySetAlpha) ;
      look->keySetAlpha = keySetAlphaHeap(look->kSet, keySetMax(look->kSet)) ;
      look->fullAlpha = TRUE ;
    }

  if (cds_only)
    dnaDumpCDSFastAKeySet(dna_out, look->keySetAlpha) ;
  else
    dnaDumpFastAKeySet(dna_out, look->keySetAlpha) ;

  aceOutDestroy(dna_out) ;

  return ;
} /* dumpAsDnaFasta */


/* FastA format protein dump, either all of the dna or just the CDS part.    */
/*                                                                           */

/* menu/button func */
static void dumpAsProtFasta(void)
{
  dumpProtFasta(FALSE) ;

  return ;
} /* dumpAsDnaFasta */

/* menu/button func */
static void dumpAsCDSProtFasta(void)
{
  dumpProtFasta(TRUE) ;

  return ;
} /* dumpAsDnaFasta */

static void dumpProtFasta(BOOL cds_only)
{
  KeySetDisp look = currentKeySetDisp("dumpAsProtFasta");
  ACEOUT pep_out;
  
  if (!look->keySetAlpha)
    return ;
  if (!(pep_out = pepFileOpen(0)))
    return ;
  if (!look->fullAlpha && keySetMax(look->kSet) > keySetMax(look->keySetAlpha))
    {
      keySetDestroy(look->keySetAlpha) ;
      look->keySetAlpha = keySetAlphaHeap(look->kSet, keySetMax(look->kSet)) ;
      look->fullAlpha = TRUE ;
    }

  if (cds_only)
    pepDumpCDSFastAKeySet(pep_out, look->keySetAlpha) ;
  else
    pepDumpFastAKeySet(pep_out, look->keySetAlpha) ;

  aceOutDestroy (pep_out);

  return ;
} /* dumpAsProtFasta */

/************************************************************/

static void dumpAsProtAlign (void)
     /* menu/button func */
{
  /* Alignment dump */
  KeySetDisp look = currentKeySetDisp("dumpAsProtAlign");
  ACEOUT pep_out;

  if (!(pep_out = pepFileOpen(0)))
    return ;
  alignDumpKeySet (pep_out, look->kSet) ;

  aceOutDestroy (pep_out);

  return ;
} /* dumpAsProtAlign */

/************************************************************/

static void selectAllKeySet(void)
{
  KeySetDisp look = currentKeySetDisp("SelectAllKeySet");
  
  keySetDestroy(look->selected);
  look->touched |= 1 ;
  look->selected = keySetCopy(look->kSet);  
  keySetShow2(look->kSet,look) ;
}

static void reverseSelectedKeySet(void)
{
  KEYSET temp;
  KeySetDisp look = currentKeySetDisp("SelectAllKeySet");

  look->curr = 0 ;
  look->touched |= 1 ;
  temp = keySetCopy(look->selected);
  look->selected = keySetXOR(temp,look->kSet);
  keySetDestroy(temp);  
  keySetShow2(look->kSet,look) ;

}

static void removeSelectedKeySet(void)
{
  KEYSET tmp;
  KeySetDisp look = currentKeySetDisp("removeSelectedKeySet");

  tmp =keySetCopy(look->kSet);

  look->kSet = keySetXOR(tmp,look->selected);

  look->curr = 0;
  look->touched |= 3 ;
  keySetDestroy(look->keySetAlpha) ;
  keySetReCreate(look->selected);

  keySetDestroy(tmp);
  keySetShow2(look->kSet,look) ;
}

static void clearSelected(void)
{
  KeySetDisp look = currentKeySetDisp("clearSelected");

  look->curr = 0 ;
  look->touched |= 1 ;
  keySetReCreate(look->selected);
  look->numselpage = 0;
  look->nselected = 0;
  keySetShow2(look->kSet,look) ;
  
}

static void editSelected(void)
{
  int i, max, max1;
  KEY *kp;
  char *cp ;
  KEYSET ksNew = 0 ;
  static char* buffer ;
  Stack localStack = 0 ;
  KeySetDisp look = currentKeySetDisp("editSelected");
  ACEIN command_in;

  if (!look->selected || !(max = keySetMax(look->selected)))
    { messout("first select the objects you want to edit") ;
      return ;
    }	
  i = max ;
  kp = arrp (look->selected, 0, KEY) - 1 ; max1 = 0 ;
  while (kp++, i--)
    if (pickType(*kp) == 'B')  /* don t edit array this way */
      max1++ ;
  if (max != max1)
    { messout("You have selected %d  non-text objects, \n"
	      "they cannot be edited in this way", max - max1) ;
    return ;
    }
  if(!checkWriteAccess())
    return;
  cp = messprintf("%s%d%s",
		 "Type a .ace command, it will be applied to the ",
		  max, " currently selected objects") ;
  if (!(command_in = messPrompt(cp, buffer ? buffer : "", "t", 0)))
    return ;
  cp = aceInWord (command_in) ; 
  if (!cp) 
    {
      aceInDestroy (command_in);
      return ;
    }
  messfree(buffer) ;
  buffer= strnew(cp, 0) ;

  aceInDestroy (command_in);

  i = max ;
  localStack = stackCreate (40*i) ;
  kp = arrp (look->selected, 0, KEY) - 1 ;
  while (kp++, i--)
    if (pickType(*kp) == 'B')  /* don t edit array this way */
      { catText (localStack, className (*kp)) ;
      catText (localStack, " ") ;
      catText (localStack, freeprotect(name (*kp))) ;
      catText (localStack, "\n") ;
      catText (localStack, buffer) ;
      catText (localStack, "\n\n") ;
      }

  ksNew = keySetCreate () ;
  parseBuffer (stackText (localStack,0), ksNew,FALSE) ;
  stackDestroy (localStack) ;  /* it can be quite big */
  i = keySetMax(ksNew) ;
  messout (messprintf ("// I updated %d objects with command\n %s\n", i, buffer)) ;
  /*  parseKeepGoing = FALSE ; */
  keySetDestroy (ksNew) ;
}
 
static void killSelected(void)
{
  int max;
  register int i;
  KEY key;

  KeySetDisp look = currentKeySetDisp("killSelected");

  if (!look->selected || !(max = keySetMax(look->selected)))
    { messout("first select the objects you want to kill") ;
      return ;
    }
  if(!checkWriteAccess())
    return;
  if (!messQuery (messprintf ("Do you really want to destroy %d items", max)))
    return ;
  look->touched |= 3 ;
  i= 0;
  while(i<max){
    key = keySet(look->selected,i) ;
    switch ( pickType(key))
      {
      case 'A':
	arrayKill (key) ;
	break ;
      case 'B':
	{ OBJ obj ;
	  if ((obj = bsUpdate (key)))
	    bsKill (obj) ;	      
	}
	break ;
      }
    { KEY *kp = arrp(look->kSet, 0, KEY) ;
      int max = keySetMax (look->kSet) ;
      
      while (max-- && *kp != key) kp++ ;
      while (kp++, max--)
	*(kp - 1) = *kp ;
    }
    keySetMax (look->kSet)-- ;
    i++;
  }
  keySetDestroy(look->keySetAlpha) ;
  keySetReCreate(look->selected);
  look->curr = 0; /* else crashes if remove last item */
  keySetShow2(look->kSet,look) ;
}

static void editAll (void)
{
  selectAllKeySet () ;
  editSelected () ;
}

static void killAll (void)
{
  selectAllKeySet () ;
  killSelected () ;
}

static void beforecombine (int type)
{
  KeySetDisp look = currentKeySetDisp("before Combine");

  if (selectedKeySet && selectedKeySet->magic == &KeySetDisp_MAGIC)
    {
      displayPreserve() ;
      look->owner = -1 ;
      clearSelected() ;
      look->touched  |= 3 ;
      look->curr = 0 ;
      look->fullAlpha = FALSE ;
      keySetMix(look, type) ;
    }
  else
    messout ("First select a key Set window by picking "
	     "its background with the left mouse button") ;

  return;
} /* beforecombine */


static void combineXOR (void)
{
  beforecombine(XOR);
}

static void combineAND(void)
{
  beforecombine(AND);
}

static void combineOR(void)
{
  beforecombine(OR);
}

static void combineMINUS(void)
{
  beforecombine(MINUS);
}

static void follow(void)
{
  int i;
  KEY key = 0;

  KeySetDisp look = currentKeySetDisp("follow");

  if (!look->keySetAlpha)
    return ;
  i = look->curr-2+look->base ;
  if (i < 0 || i > keySetMax(look->keySetAlpha))
    i = 0 ;
  if (!keySetMax(look->keySetAlpha))
    return ;
  key = keySet(look->keySetAlpha,i) ;
  if (pickType(key) != 'B')
    return ;
  { int type, tcl ;
    KEY tag ;
    Stack sta = stackCreate (50) ;
    
    if (treeChooseTagFromModel (&type, &tcl, class(key), &tag, sta, 0))
      { KEYSET tmp = query(look->kSet, messprintf(">%s",stackText (sta, 0))) ;
	keySetDestroy(look->kSet) ;	
	keySetDestroy(look->keySetAlpha) ;
	look->kSet = tmp ;
	look->touched |= 3 ;
	look->curr = 0 ;
	keySetShow2(look->kSet,look) ;
      }
    stackDestroy (sta) ;
  }
}

static void grepset (void)
{
  char *cp ;
  int i ;
  KEYSET ks = 0 ;
  Stack s = 0 ;
  KeySetDisp look = currentKeySetDisp("grepset");
  ACEIN word_in;

  word_in = messPrompt ("Type a few words (at least 3 letters long), "
			"upper/lower case do not matter","", "t", 0);
  if (!word_in)
    return ;

  s = stackCreate(50) ;
  catText (s, "*") ;
  while ((cp = aceInWord (word_in)))
    { catText (s, cp) ; catText (s, "*") ; }

  aceInDestroy (word_in);

  cp = stackText (s, 0) ; i = 0 ;
  while (*cp) { if (*cp != ' ' && *cp != '*') i++ ; cp++ ;}
  if (i >= 3) 
    {
      ks = queryGrep (look->kSet, stackText (s, 0)) ;
      keySetNewDisplay (ks, stackText(s,0)) ; /* will destroy ks */
      keySetSelect () ;
    }
  else
    messout ("word too short") ;
  stackDestroy (s) ;
}

static void relatedBiblio(void)
{
  KeySetDisp look = currentKeySetDisp("relatedBiblio");
  
  biblioKeySet ("Biblio for keyset", look->kSet) ;
}
 
KeySetDisp keySetNewDisplay (KEYSET ks, char *title)
{
  KeySetDisp look = 0 ;

  if (keySetExists(ks))
    { 
      Graph g = graphActive () ;

      displayCreate("DtKeySet") ;
      graphRetitle(title) ;
      look = keySetShow (ks,0) ;
      keySetSelect () ;
      graphActivate (g) ;
    }
  return look ;
}

static void keySetDoQuery(char *txt)
{
  KEYSET ks = 0 ;
  char *cp ;
  KeySetDisp look = currentKeySetDisp("keySetDoQuery");

  cp = look->queryBuffer ;
  while (*cp == ' ') cp++ ;
  if (!*cp) return ;
  ks = query (look->kSet, cp) ;
  keySetNewDisplay (ks, look->queryBuffer) ; /* will destroy ks */
  keySetSelect () ;
}

static void openQueryBox(void)
{
  KeySetDisp look = currentKeySetDisp("openQueryBox");
  if(look->curr)
    look->curr -= (look->showQuery ? 1 : -1 ) * NQueryLine ; 
  look->showQuery = look->showQuery ? 0 : 2 ; /* toggle */  
  keySetShow2 (look->kSet, look) ;
}

static void findneigh(void)
{ 
  KEYSET tmp ;
  KeySetDisp look = currentKeySetDisp("findneigh");
  if (!look->keySetAlpha)
    return ;
  tmp = look->kSet ;
  look->kSet = keySetNeighbours(tmp) ; /* will destroy tmp */

  look->touched |= 3 ;
  look->curr = 0 ;
  keySetDestroy(look->keySetAlpha) ;
  keySetShow2(look->kSet,look) ;
}



/* Find empty objects. */
static void findEmptyObj(void)
{
  KeySetDisp look = currentKeySetDisp("findEmptyObj") ;
  KEYSET kset_empty ;

  if ((kset_empty = keySetEmptyObj(look->kSet)))
    {
      keySetDestroy(look->selected) ;

      look->touched |= 1 ;

      look->selected = kset_empty ;
   
      keySetShow2(look->kSet,look) ;
    }

  return ;
}




static void queByExam(void)
{
  int i;
  KeySetDisp look = currentKeySetDisp("queByExam");

  if (!look->keySetAlpha)
    return ;
  i = look->curr-2+look->base ;
  if (i < 0 || i >= keySetMax (look->keySetAlpha))
    i = 0 ;
  qbeCreateFromKeySet (look->keySetAlpha,i - 1) ;
}

static void showClasses(void)
{
  KeySetDisp look = currentKeySetDisp("showClasses");

  look->curr = 0;
  look->showClass = !look->showClass;
  keySetShow2(look->kSet,look) ;
}

static KeySetDisp keySetShow2 (KEYSET kSet, KeySetDisp look)
{  
  int	i, iy, y, colX, col, nx, ny, numcol;
  int   max = ( kSet ? keySetMax(kSet) : 0 ) ;
  char	*cp ;
  KEY   key ;
  int   box=0;
  BOOL FIRST = TRUE, room = TRUE ;
  int startpos=0, cWidth=0, endpos=0, numrows=0 ;

  if (!look->isBlocking)	/* so as not to hide message */
    graphPop() ;

  if (look->base >= max)
    look->base = 0 ;
  
  if (!keySetActive (0, 0))
    selectedKeySet = look ;
  
  graphFitBounds (&nx, &ny) ;
  if (look->kSet != kSet)
    {
      look->showQuery = 0 ;
      if (keySetExists(look->keySetAlpha))
	keySetDestroy(look->keySetAlpha) ;
      /* don't destroy look->kSet here, calling routine should */
    }
  look->kSet = kSet ;
  look->touched |= 3 ;
  look->curr = 0 ;

  drawMenus();  /* here, hence after evaluation of biblio possible */
  if (max)
    {
      col = 1 + nx/2 ;	/* max number possible columns */
      if (!look->keySetAlpha)
        {
	  look->keySetAlpha = keySetAlphaHeap(kSet, look->base + 4*col*ny) ;
	  look->fullAlpha = FALSE ;
        }
      else
	if (!look->fullAlpha &&
	    keySetMax(look->keySetAlpha) < keySetMax(look->kSet) &&
	    keySetMax(look->keySetAlpha) < look->base + col*ny)
	  {
	    keySetDestroy(look->keySetAlpha) ;
	    look->keySetAlpha 
	      = keySetAlphaHeap(look->kSet, keySetMax(look->kSet)) ;
	    look->fullAlpha = TRUE ;
	  }
    }
  
  look->box2key = arrayReCreate(look->box2key, 100, KEY) ;

  TOPGAP += 1.7;		/* we leave a gap for the text 'n items selected..' */

  look->numselpage = 0;

  if (max)
    {
      colX = SCROLLBARGAP;
      endpos = look->base -1;
      numrows = (ny-TOPGAP)-BOTGAP;
      if (numrows < 0) 
	{
	  graphRedraw();
	  return look; /* window too small to start with */
	}
      
      i = numcol = 0;
      while (room)
	{    
	  startpos = endpos+1;
	  room = columnWidth(startpos,TRUE,&cWidth,&endpos,numrows);
	  look->colWidth[i++] = cWidth;
	  if (cWidth && ((colX + cWidth) < nx || colX == SCROLLBARGAP))
	    { /* if it fits or is the first */ 
	      look->top = endpos;
	      y = nint(TOPGAP);
	      numcol++;

	      while (startpos <= endpos)
		{
		  key = arr(look->keySetAlpha,startpos++,KEY) ;
		  cp = 0 ;

		  if (!nextName (key, &cp) || *cp == '\177')
		    continue ;
		  
		  box = graphBoxStart() ;
		  array (look->box2key, box, KEY) = key ;

		  if (iskey(key) == LEXKEY_IS_OBJ)
		    graphTextFormat(BOLD) ;  /* pickable object */
		  else
		    graphTextFormat(PLAIN_FORMAT) ;

		  if (look->showClass)
		    graphText(messprintf ("%s:%s", className(key), cp), colX, y++) ;
		  else
		    graphText(cp, colX, y++) ;

		  graphBoxEnd() ;
		  
		  if (FIRST)
		    { 
		      look->firstBox = box;
		      FIRST = FALSE;
		    }
		  if (keySetFind (look->selected, key, 0))
		    { 
		      graphBoxDraw(box,BLACK,LIGHTGRAY);
		      look->numselpage++;
		    }
		}

	      colX += (cWidth+2);
	    }
	  else if (cWidth != 0) /* Do not stop on blank keys. For some reason
				   some databases have multiple ""'s */
	    room = FALSE;
	}
      look->lastBox = box;
      
      /*  look->numperpage = numcol*numrows; mhmp */
      look->numperpage = look->lastBox - look->firstBox + 1 ;
      array(look->box2key, arrayMax(look->box2key), KEY) = 0 ; /* terminate */
      
      graphTextFormat (PLAIN_FORMAT) ;
      
      if(look->curr)
	{
	  if(look->curr >= look->firstBox && look->curr <= look->lastBox) /* current may not be on the page now */
	    graphBoxDraw(look->curr,WHITE,BLACK);                         /* due to other tools being expanded  */
	  else
	    look->curr = 0;
	}
      iy = look->base ? 2 : 0 ;
      
      shownItem (look) ;  

      /* mhmp 11.09.98 */
      if (numrows >= 1)
	drawScrollBar(look->nitem);
    }
  else
    shownItem (look) ;  

  look->txtbox = graphBoxStart();
  graphTextPtr(look->txtBuffer, (nx/2)-15, TOPGAP-1.5, 60) ; 
  graphBoxEnd();
  
  graphRedraw();
  messfree (look->message) ;
  return look ;
}

KeySetDisp keySetShow (KEYSET kSet, KeySetDisp handle)
{
  return keySetMessageShow (kSet, handle, 0, FALSE) ;
}

KeySetDisp keySetMessageShow (KEYSET kSet, 
			      KeySetDisp old_ksdisp,
			      char *message, BOOL self_destruct)
{
  Graph old = graphActive() ;
  KeySetDisp ksdisp;

  if (!old_ksdisp)
    {
      ksdisp = (KeySetDisp) messalloc (sizeof (struct KeySetDispStruct)) ;
      ksdisp->magic = &KeySetDisp_MAGIC ;
      ksdisp->graph = graphActive() ;
      ksdisp->self_destruct = self_destruct ;
      ksdisp->owner = old ;
      ksdisp->showtype = DEFAULT;
      ksdisp->selected = keySetCreate();
      ksdisp->showClass = FALSE;
      ksdisp->showQuery = 0 ;
      ksdisp->curr = 0;
      ksdisp->numselpage = 0;
      graphRegister (DESTROY, localDestroy) ;
      graphRegister (MESSAGE_DESTROY, ksetUnBlock) ;
      graphRegister (PICK, localPick) ;
      graphRegister (KEYBOARD, localKbd) ;
      graphRegister (RESIZE, localResize) ;
      graphRegister (MIDDLE_UP, scrollMiddleUp) ;
      graphFreeMenu (ksetlocalMenu, menuOptions) ;

      graphAssociate (&GRAPH2KeySetDisp_ASSOC, ksdisp) ;
      if (!selectedKeySet)
	selectedKeySet = ksdisp ;
    }
  else
    { 
      ksdisp = old_ksdisp;

      if (ksdisp->magic != &KeySetDisp_MAGIC)
	messcrash("keySetShow called with corrupted KeySetDisp pointer") ;
      if (!graphActivate (ksdisp->graph))
	messcrash ("keySetMessageShow() - keyset window lost its graph");

      ksdisp->curr = 0 ;
      ksdisp->base = 0 ;
      messfree (ksdisp->message) ;
      keySetDestroy(ksdisp->keySetAlpha) ;
      keySetReCreate(ksdisp->selected);

      ksdisp->self_destruct = self_destruct ;
    }

  if (message) 
    ksdisp->message = strnew (message, 0) ;
  ksdisp->touched |= 3 ;
  ksdisp->curr = 0 ;
  return keySetShow2(kSet, ksdisp) ;
}

/*****************************/

static BOOL importKeySet (KEYSET kSet, ACEIN keyset_in)
{
  int table, nbad = 0 ;
  char *word ;
  KEY key = 0;

  keySetMax(kSet) = 0 ;
  while (aceInCard (keyset_in))
    if ((word = aceInWord(keyset_in)))
      { 
	if (!(table = pickWord2Class (word)))
	{ ++nbad ; continue ; }
      if (table == _VKeySet && !keySetMax(kSet))
	continue ;
      aceInStep (keyset_in, ':') ;
      if (!(word = aceInWord(keyset_in)))
	{ 
	  messout ("No object name, line %d", 
		   aceInStreamLine (keyset_in)) ;
	  continue ;
	}
      if (lexword2key (word, &key, table))
	keySet(kSet, keySetMax(kSet)) = key ;
      else
	++nbad ;
      }
  if (nbad)
    messout ("%d objects not recognized", nbad) ;
  keySetSort(kSet) ;
  keySetCompress(kSet) ;

  return TRUE;
}


/***********************************/

BOOL keySetDisplay (KEY key, KEY from, BOOL isOldGraph, void *unused)
{
  KEYSET kset ;
  KeySetDisp look ;
  KEY keySetKey;

  if (class(key) == _VKeySet)
    {
      if (!(kset = arrayGet (key,KEY,"k")))
	return FALSE ;
      keySetKey = key;
    }
  else
    {
      /* key is not a KeySet object, so we make a keyset and put the
	 key in it as the only element and then display that */
      kset = keySetCreate();
      keySet(kset, 0) = key;
      keySetKey = 0;
    }

  if (isOldGraph)
    localDestroy () ;
  else
    displayCreate("DtKeySet") ;

  graphRetitle (name(key)) ;
  
  look = keySetShow (kset, 0) ;
  look->keySetKey = keySetKey ;

  return look ? TRUE : FALSE ;
} /* keySetDisplay */

/***********************************/

char *keySetGetShowAs (KeySetDisp ksdisp, KEY key)
{
  /* which "Show As"-displayType has been selected in a given keyset-window */
  if (ksdisp->magic != &KeySetDisp_MAGIC)
    messcrash ("keySetGetShowAs() - received non-magic KeySetDisp pointer") ;
    
  if (ksdisp->showtype)
    return name(ksdisp->showtype) ;

  return (char*)0;
} /* keySetGetShowAs */

/*************************************/

static void localDestroy (void)
{
  KeySetDisp look = currentKeySetDisp("keySetDestroy") ;

  look->magic = 0 ;
  if (selectedKeySet == look)
    selectedKeySet = 0 ;
  keySetDestroy (look->kSet) ;
  keySetDestroy (look->keySetAlpha) ;
  messfree (look) ;

  graphAssRemove (&GRAPH2KeySetDisp_ASSOC) ;
}

/*************************************/

static void keySetSelect (void)
{
  KeySetDisp look = currentKeySetDisp ("keySetSelect") ;

  if (    selectedKeySet 
      &&  selectedKeySet != look
      && selectedKeySet->magic == &KeySetDisp_MAGIC
      && graphExists(selectedKeySet->graph))
    { Graph hold = graphActive () ;
      graphActivate (selectedKeySet->graph) ;
      
      graphBoxDraw (1,BLACK,WHITE) ;
      graphActivate (hold) ;
    }
  graphBoxDraw (1,BLACK,PALERED) ;
  selectedKeySet = look ;
}

/***************/

BOOL keySetActive(KEYSET *setp, KeySetDisp* lookp) 
{
  if (selectedKeySet && selectedKeySet->magic == &KeySetDisp_MAGIC
      && graphExists(selectedKeySet->graph)
      && arrayExists(selectedKeySet->kSet))
    { if (setp) *setp = selectedKeySet->kSet ;
      if (lookp) *lookp = selectedKeySet ;
      return TRUE ;
    }
  selectedKeySet = 0 ;
  if (setp) *setp = 0 ; 
  if (lookp) *lookp = 0 ;
  return FALSE ;
}

/***************/

static void localPick (int box,double x,double y, int modifier_unused)
{

  KeySetDisp look = currentKeySetDisp("keySetPick");

  if (look != selectedKeySet)
    keySetSelect () ;
  if (look->showQuery)
    {
      if (box == look->queryBox) 
	{
	  graphTextScrollEntry (look->queryBuffer,0,0,0,0,0) ;
	  look->showQuery = 2 ; /* keep focus */
	  return ;
	}
      else
	{
	  graphEntryDisable() ;
	  graphRegister (KEYBOARD, localKbd) ;
	  look->showQuery = 1 ;/* release focus */
	}
    }

  if (!box) 
    return ;

  if(box == look->upBox)
    pageUp();
  else if(box == look->downBox)
    pageDown();
  else if(box == look->scrollBarBox)
    graphBoxDrag(box, keyBandDrag);
  else
    {
      if (box == look->curr)                  /* double click */
	{
	  KEY key = array(look->box2key, look->curr, KEY) ;
	  if (key)
	    {
	      if (look->isBlocking)
		ksetDeleteKey(key) ;
	      else 
		{  
		  graphPostBuffer (name(key)) ;

		  if(look->showtype != 0)
		    display(key,0,name(look->showtype));
		  else
		    display(key,0,0);

		  if (look->self_destruct)
		    {
		      Graph g ;

		      g = graphActive() ;

		      graphActivate(look->graph) ;
		      graphDestroy() ;

		      graphActivate(g) ;
		    }
		}
	    }
	}
      else
	{ 
	  if(look->curr > 1)
	    graphBoxDraw(look->curr,BLACK,LIGHTGRAY);
	  look->curr=box;
	  if(look->curr >1)
	    {
	      startbox = box;
	      controlLeftDown (x,y);
	      if (look->curr < arrayMax(look->box2key))
		graphPostBuffer (name(array(look->box2key, look->curr, KEY))) ;
	    }
	}
    }
} /* localPick */

/***********************************/

static void keySetMix(KeySetDisp qry, int combinetype)
{
  KEYSET s=0;
  KeySetDisp  q = selectedKeySet ;
  
  if(combinetype == AND) 
      s = keySetAND(q->kSet,qry->kSet) ;
  else if(combinetype  == OR)
      s = keySetOR(q->kSet,qry->kSet) ;
  else if(combinetype == MINUS)
      s = keySetMINUS(qry->kSet,q->kSet) ;
  else if(combinetype == XOR)
      s = keySetXOR(q->kSet,qry->kSet) ;
  else
    messcrash("keySetMix received wrong operator %d",combinetype) ;
  arrayDestroy(qry->kSet) ;
  qry->kSet = s ;
  qry->touched |= 3 ;
  qry->curr = 0 ;

  keySetDestroy(qry->keySetAlpha) ;
  keySetShow2(qry->kSet,qry ) ;
}

/***********************************/

static void localKbd (int k, int modifier_unused)
{
  float x0,y0,x1,y1,x2,y2,x3,y3,x4,y4 ;
  int max, before ;
  KEY key;
  KeySetDisp look = currentKeySetDisp("keySetPick") ;

  if (look->curr < 2)
    return ;

  max = keySetMax (look->kSet) ;
  
  key = arr(look->box2key,look->curr, KEY);
  graphBoxDraw(look->curr, BLACK, LIGHTGRAY);

  switch (k)
    {
    case UP_KEY :
      if (arr(look->box2key, look->curr-1, KEY))
	--look->curr ;
      else if (look->base)
	{ pageUp () ;
	  for (look->curr = arrayMax(look->box2key)-1 ;
	       look->curr && !arr(look->box2key, look->curr, KEY) ;
	       --look->curr) ;
	}
      break ;
    case DOWN_KEY :
      if(arr(look->box2key,look->curr,KEY) !=  arrayMax(look->kSet)){ /* do not want to go down if this is the last key */
	if (arr(look->box2key, look->curr+1, KEY))
	  ++look->curr ;
	else if (look->top < max-1)
	  { pageDown () ;
	    for (look->curr = 1 ;
		 look->curr < arrayMax (look->box2key) && 
		 !arr(look->box2key, look->curr, KEY) ;
		 ++look->curr) ;
	  }
      }
      break ;
    case RIGHT_KEY :
      graphBoxDim (look->curr,&x0,&y0,&x2,&y2) ;
      graphBoxDim (look->lastBox,&x3,&y3,&x4,&y4);
      if(x0 != x3 || look->lastBox == look->curr){
	y1 = y0 ; x1 = x0 ;
	while (arr(look->box2key,look->curr+1,KEY) &&
	       (y1 < y0-0.5 || x1 == x0))
	  graphBoxDim (++look->curr,&x1,&y1,&x2,&y2) ;
	if ((y1 < y0 - 0.5 || x1 == x0) && look->top < max-1)
	  { pageDown() ;
	    for (look->curr = 1 ;
		 look->curr < arrayMax (look->box2key) && 
		 !arr(look->box2key, look->curr, KEY) ;
		 ++look->curr) ;
	  }
      }
      break ;
    case LEFT_KEY :
      graphBoxDim (look->curr,&x0,&y0,&x2,&y2) ;
      graphBoxDim (look->firstBox,&x3,&y3,&x4,&y4); 
      if(x3 != x0 || look->curr == look->firstBox){ /* i.e. the first column */
	y1 = y0 ; x1 = x0 ;
	before = look->curr;
	while (look->curr > 2 && (y1 > y0+0.5 || x1 == x0) &&look->curr > look->firstBox)
	  graphBoxDim (--look->curr,&x1,&y1,&x2,&y2) ;
	if (
	    ((y1 > y0+0.5 || x1 == x0) && look->base) ||
	    ( (before == look->firstBox) && 
	      (arr(look->box2key,before,KEY)!= arr(look->keySetAlpha,0,KEY))
	      )
	    )
	  { pageUp () ;
	    for (look->curr = arrayMax(look->box2key)-1 ;
		 look->curr && !arr(look->box2key, look->curr, KEY) ;
		 --look->curr) ;
	  }
	if(look->curr < look->firstBox)
	  look->curr = look->firstBox;
      }
      break ;
    case HOME_KEY :
      if (look->base)
	{ look->base =0;
	  keySetShow2 (look->kSet, look) ;
	}
      for (look->curr = 1 ;
	   look->curr < arrayMax (look->box2key) && 
	   !arr(look->box2key, look->curr, KEY) ;
	   ++look->curr) ;
      break ;
    default:
      graphBoxDraw(look->curr, WHITE, BLACK) ;
      return ;
    }

  key = arr(look->box2key,look->curr, KEY);
  if ( key && !keySetFind (look->selected, key, 0))
    { keySetInsert (look->selected, key) ;
      look->numselpage ++ ;
      look->touched |= 1 ;
    }
  graphBoxDraw (look->curr, WHITE, BLACK) ; 
  shownItem (look) ;

  if (look->txtbox)
    graphBoxDraw(look->txtbox,BLACK,WHITE);

  graphPostBuffer(name(key));

  {
    KEY key = array(look->box2key, look->curr, KEY) ;
    Graph g = graphActive () ;
    
    if(look->showtype != 0)
      display(key,0,name(look->showtype));
    else
      display(key,0,0);

    /* display() calls graphPop on the window it creates,
       which can change the keyboard focus in click-to-type 
       window managers (and MS-windows) We move the focus back
       to the keyset window here so that repeated arrow-key presses
       work. */
    graphActivate(g);
    graphPop(); 
  }
}

/***************************/

static void ksetAddKey (KEY key)
{
  KeySetDisp look = currentKeySetDisp("ksetAddKey") ;

  if (key) keySetInsert(look->kSet,key) ;
  look->touched |= 3 ;
  keySetDestroy(look->keySetAlpha) ;

  keySetShow2(look->kSet,look) ;

  displayRepeatBlock () ;
}

/***************************/

static void ksetDeleteKey (KEY key)
{
  KeySetDisp look = currentKeySetDisp("ksetDeleteKey") ;

  keySetRemove(look->kSet,key) ;
  look->touched |= 3 ;
  keySetRemove(look->selected,key) ; /* mhmp add-remove */
  keySetDestroy(look->keySetAlpha) ;

  look->curr = 0; /* otherwise if it is the last box it crashes */
  keySetShow2(look->kSet,look) ;

  displayRepeatBlock () ;
}

/*****************************/

static void ksetUnBlock (void)
{
  KeySetDisp look = currentKeySetDisp("ksetUnBlock") ;

  if (look->isBlocking)
    { look->isBlocking = FALSE ;
      displayUnBlock() ;
    }
}

/*************************************/

static void selectedToNewKeySet(void)
{
  Graph  g ;
  KeySetDisp look = currentKeySetDisp("selectedToNewKeySet") ;

  g = graphActive () ;
  displayCreate("DtKeySet") ;
  graphRetitle ("Copied selected keyset") ;
  keySetShow (keySetCopy (look->selected),0) ;
  displayPreserve() ;
  graphActivate (g) ;
}


/* dummy is used in the prototype so that the ksetlocalMenu matches the      */
/* FreeMenuFunction type.                                                    */
static void ksetlocalMenu (KEY k, int unused)
{
  extern void Bdump (KEY key) ;
  Graph  g ;
  KEY	 key = 0 ;
  char *word = 0 ;
  KeySetDisp look = currentKeySetDisp("ksetlocalMenu") ;
  ACEIN name_in;

  switch (k)
    {
    case 99: 
      graphDestroy () ; 
      break ;
    case 98: 
      helpOn("KeySet") ;
      break ;
    case 97: 
      graphPrint() ;
      break ;
    case 51:
      keySetSummary();
      break;
    case 1:	/* save */
      if (!look->keySetAlpha)
	break ;
      if(!checkWriteAccess())
	return ;

      word = look->keySetKey ? name(look->keySetKey) : "" ;
      while (!key && 
	     (name_in = messPrompt ("Give a name", word, "t", 0)))
	{
	  if (!(word = aceInWord(name_in)) || !*word ||
	      (!lexaddkey (word,&key,_VKeySet) &&
	       !messQuery ("Overwrite existing keyset ?")))
	    {
	      aceInDestroy (name_in);
	      return ;
	    }
	  aceInDestroy (name_in);
	}
      
      if (!key)
	break ;
      
      if (!lexlock(key))
	{ messout ("Sorry, key is locked by someone else") ;
	  break ;
	}
      arrayStore (key,look->kSet,"k") ;
      lexunlock (key) ;
      look->keySetKey = key ;

      break ;
    case 2:	/* copy */
      g = graphActive () ;
      displayCreate("DtKeySet");
      graphRetitle ("Copied keyset") ;
      keySetShow (keySetCopy (look->kSet),0) ;
      displayPreserve() ;
      graphActivate (g) ;
      break ;
     case 8: 
      if (look->curr > 1 && look->curr < arrayMax(look->box2key))
	{ key = array(look->box2key, look->curr, KEY) ;
	  if (key) 
	    display(key, 0, "TREE");
	}
      break;
    case 10:	/* Read from file */
      {
	ACEIN keyset_in;
	static char directory[DIR_BUFFER_SIZE] = "";
	static char filename[FIL_BUFFER_SIZE] = "";

	keyset_in = aceInCreateFromChooser ("File of object names ?",
					    directory, filename,
					    "ace", "r", 0);
	if (keyset_in)
	  {
	    importKeySet(look->kSet, keyset_in);
	    aceInDestroy (keyset_in);
	    
	    keySetDestroy(look->keySetAlpha) ;
	    keySetShow2(look->kSet,look) ;
	  }
      }
      break ;
    case 6:     /* Add key */
      look->isBlocking = TRUE ;
      displayPreserve() ; look->owner = -1 ;
      displayBlock (ksetAddKey,
		    "Double-clicking in a different KeySet window will add entries,\n"
		    "inside the current window will delete them from this KeySet.\n"
		    "This will continue until you remove this message\n") ;
      break ;
    case 7:			/* "New empty KeySet" */
      keySetNewDisplay (keySetCreate(), "New KeySet") ;
      break ;
    case 41 :  /* Mailer */
      if (!look->keySetAlpha)
	break ;
      acedbMailer(0, look->kSet, 0) ;
      break ;
    case 42:     /* JTM Hexadecimal dump of disk blocks */
      if (!look->keySetAlpha)
	break ;
      if (look->curr < 2)
	break ;
#ifdef ACEDB4
      Bdump(keySet(look->keySetAlpha,look->curr-2+look->base)) ;
#endif
      break ;
    case 45:
      if (!look->keySetAlpha)
	break ;
      multiMapDisplayKeySet (0, look->kSet, 0, 0, 0, 2) ;
      break ;
    case 46:
     if (!look->keySetAlpha)
       break ;
     forestDisplayKeySet (0, keySetCopy(look->kSet), FALSE) ;
     break ; 
    case 47:
      look->curr = 0;
      look->showClass = !look->showClass;
      keySetShow2(look->kSet,look) ;
      break ; 
    case 48:
     findneigh() ;
     break ;
    case 49:
     findEmptyObj() ;
     break ;
   case OR:  
     beforecombine(OR);
     break ;
   case AND:  
     beforecombine(AND);
     break ;
   case MINUS:  
     beforecombine(MINUS);
     break ;
   case XOR:  
     beforecombine(XOR);
     break ;
    default: 
      break ;
    }
} /* ksetlocalMenu */

/*************************************************/
/************* left button for thumb **********/

static void controlLeftDrag (double x, double y)
{
  int i,nx,ny,ix,numrows,incr,iy,col,colour,addbit,index;
  BOOL inlimits=TRUE;
  float right,ncol;
  KEY key;
  KeySetDisp look = currentKeySetDisp("controlLeftDrag");

  if(oldcol >= 0){
    graphFitBounds (&nx, &ny) ;
    
    iy = (int)y;
    numrows = ny-(TOPGAP+BOTGAP);
    col = 0;
    if(x < SCROLLBARGAP)
      x= (float)SCROLLBARGAP+1.0;

    right = (float)SCROLLBARGAP;
    ncol = ((float)look->numperpage/(float)numrows);
    i=0;
    while(i <= ncol){
      right+=(float)look->colWidth[i++]+2.0; 
    }
    if(x > right)
      x= right;
    ix = SCROLLBARGAP;
    while(ix < x){
      ix +=look->colWidth[col++] + 2;
    }
    
    if(iy  > ny-BOTGAP-1)
      iy = ny-BOTGAP-1;
    if(iy < TOPGAP-1)
      iy = TOPGAP-1;
    incr = iy-oldy;
    oldy=iy;
    incr += (col-oldcol)*numrows;
    oldcol = col;
    numboxes += incr;
    if(incr > 0)
      addbit = -1;
    else
      addbit =-2;
    if(incr != 0){
      while(lastbox != startbox+(numboxes+addbit) && inlimits){
	if((incr>0 && lastbox > startbox) || (incr<0 && lastbox < startbox) || lastbox == startbox)
	  colour = LIGHTGRAY;
	else{
	  key = arr(look->box2key,lastbox,KEY);
	  if(!keySetFind(look->selected,key,&index))
	    colour = WHITE;
	  else
	    colour = LIGHTGRAY;
	}
	if(lastbox >= look->firstBox && lastbox <=  look->lastBox)
	  graphBoxDraw(lastbox,BLACK,colour);
	else
	  inlimits = FALSE;
	if(incr > 0)
	  lastbox++;
	else
	  lastbox--;
      }
    }
  }
  else{
    col = 0;
    ix = SCROLLBARGAP;
    while(ix < x){
      ix +=look->colWidth[col++] + 2;
    }
    oldy = (int)y-1;
    oldcol = col;
  }    
}    

static void controlLeftUp (double x, double y)
{
  KEY key;
  int endbox=0,index ;
  KeySetDisp look = currentKeySetDisp("controlLeftUp");

  look->curr = 0;
  if(numboxes > 1){
    numboxes--;
    endbox = startbox+numboxes-1;
  }
  if(numboxes < 1){
    numboxes--;
    endbox = startbox;
    startbox = startbox + numboxes;
  }
  else if(numboxes == 1){
    endbox = startbox;
    look->curr = startbox;
  }
  while(startbox <= endbox){
    key = arr(look->box2key,startbox, KEY);
    if(key && !keySetFind(look->selected,key,&index))
      {
	keySetInsert(look->selected,key); 
	look->touched |= 1 ;
	look->numselpage++;
      }
    startbox++;
  }
  graphRegister (LEFT_DRAG, none) ; 
  graphRegister (LEFT_UP, none) ;

  shownItem (look) ;

  graphBoxDraw(look->txtbox,BLACK,WHITE);
  if(numboxes==1 && look->curr > 0){
    graphBoxDraw(look->curr,WHITE,BLACK);
    graphPostBuffer(name(arr(look->box2key,look->curr, KEY)));
  }
}

static void controlLeftDown (double x, double y) 
{ 
  numboxes = 1;
  oldcol = -1;
  lastbox = startbox;
  graphBoxDraw(lastbox,BLACK,LIGHTGRAY);
  graphRegister (LEFT_DRAG, controlLeftDrag) ; 
  graphRegister (LEFT_UP, controlLeftUp) ;
}


static void nextShowtype(void)
{
  FREEOPT *menu;
  int i ;
  KeySetDisp look = currentKeySetDisp("changeShowtype");
  
  menu = displayGetShowTypesMenu();
  for(i=1;i<=menu[0].key;i++)
    if(look->showtype == menu[i].key)
      { i++ ; if (i > menu[0].key) i = 1 ;
      look->showtype = menu[i].key ;
      keySetShow2(look->kSet,look) ;
      break ;
      }
}

static void changeShowtype(KEY key)
{
  KeySetDisp look = currentKeySetDisp("changeShowtype");
  
  look->showtype = key;
  keySetShow2(look->kSet,look) ;
}

#define MAXLEN 45
static void keySetSummary(void)
{
  int	i, *summary ;
  KEY	*kp ;
  char	*buf ;
  KeySetDisp look = currentKeySetDisp("keySetSummary") ;

  summary = (int *) messalloc ((sizeof(int) * MAX_CLASSES)) ;
  
  if (look->kSet)
    for (i = keySetMax(look->kSet),
	 kp = arrp (look->kSet, 0, KEY) - 1 ; i-- ; kp++)
      if (lexIsKeyVisible(*kp))
	summary[class(*kp)]++;
  
  buf = (char *) messalloc ((MAX_CLASSES+1)*MAXLEN) ;
  sprintf (buf,"KeySet Class Information\n");
  for (i = 0 ; i < MAX_CLASSES ; i++)
    if (summary[i] != 0)
      strcat (buf, messprintf ("%s %d items\n",
			       pickClass2Word(i), summary[i])) ;
  graphMessage (buf) ;
  messfree (buf) ;

  messfree (summary) ;
}

static BOOL columnWidth (int startpos, int direction,
			 int *cWidth, int *endpos, int numrows)
{
  int k=0,i,max,j;
  char *cp,*cp2,*cp3;
  KEY key;
  KEYSET kSet;
  KeySetDisp look = currentKeySetDisp("columnWidth");

  if (numrows < 1)
    return FALSE;

  kSet = look->kSet;
  i= startpos;
  *cWidth = 0;
  max = keySetMax(look->keySetAlpha);
  
  while(k<numrows){  
    if(i >= max){               /* check end if moving forward */
      return FALSE;
    }
    if(i < 0)                   /* check for start if going back a page */
      return FALSE; 
    cp = cp2 =0;
    key = arr(look->keySetAlpha,i,KEY) ;
    if (!nextName (key, &cp) || *cp == '\177'){
      if(direction) 
	*endpos = i++;
      else
	*endpos = i--;
      continue ;
    }  
    if(look->showClass){
      cp2 = className (key);	    
      cp3 = strnew(messprintf("%s:%s",cp2,cp),0);
      /*sprintf(cp3,"%s:%s\n",cp2,cp);*/
      if ((j = strlen(cp3)) > *cWidth){
	*cWidth = j ;
      }
      messfree(cp3);
    }
    else{
      if ((j = strlen(cp)) > *cWidth){
	*cWidth = j ;
      }
    }
    if(direction)
      *endpos = i++;
    else
      *endpos = i--;
    k++;
  }
  return TRUE;
}

static void drawScrollBar(int max)
{
  int colX,nx,ny,k;
  float xpt,top,bottom,boxstart,boxend,len;
  KeySetDisp look = currentKeySetDisp("drawScrollBar");
  
  graphFitBounds (&nx, &ny) ;

  if(max > look->numperpage && max != 0)
    {
      colX = SCROLLBARGAP ;
      top  =  (float)TOPGAP;
      bottom = (float)(ny - BOTGAP);
      
      xpt = SCROLLBARPOS;
      k = graphLinewidth(0.4);
      graphLine(xpt,top,xpt,bottom); 
      k= graphLinewidth(k);
      
      if(look->base !=0)
	look->upBox = scrollTriangle(UP,xpt-1.0,top,GREEN);
      else
	look->upBox =  scrollTriangle(UP,xpt-1.0,top,WHITE);

      if(look->top != max-1)
	look->downBox = scrollTriangle(DOWN,xpt-1.0,bottom,GREEN)  ;
      else
	look->downBox = scrollTriangle(DOWN,xpt-1.0,bottom,WHITE)  ; 
      
      len =  (((float)look->numperpage)/(float)max)*(bottom-top);
      if(len < MINSCROLLBARLENGTH)
	len = MINSCROLLBARLENGTH;
      look->scrolllen = len;
      
      if(look->base == 0)
	boxstart = top;
      else
	{
	  if(look->scrolllen == MINSCROLLBARLENGTH)
	    boxstart = (((float)look->base/(float)max)*(bottom-top-look->scrolllen)) + top;
	  else
	    boxstart = (((float)look->base/(float)max)*(bottom-top)) + top;
	}
      boxend = boxstart + len;
      
      if(boxend > bottom)
	{
	  boxend =  bottom;
	  boxstart = bottom-len;
	}

      look->scrollBarBox = graphBoxStart();
      
      graphRectangle(xpt-0.5,boxstart,xpt+0.5,boxend);
      graphBoxEnd();
      graphBoxDraw(look->scrollBarBox,BLACK,GREEN);
    } 
  else
    look->scrollBarBox = look->upBox = look->downBox = 0; 
} /* drawScrollBar */

static void beforeForest (void)
{     
  KeySetDisp look = currentKeySetDisp ("beforeForest") ;

  if (!look->keySetAlpha || !look->kSet || !keySetMax(look->kSet))
    return ;
  forestDisplayKeySet (0, keySetCopy(look->kSet), FALSE) ;
}

static void otherTools (void)
{
  KeySetDisp look = currentKeySetDisp("otherTools") ;

/* changing the number of buttons changes the current selected */

  if(look->curr)
    look->curr -= (isOtherTools ? 1 : -1 ) * N3rdLINE ;
  isOtherTools = !isOtherTools ;
  keySetShow2(look->kSet,look) ;
}

static void beforeAddRemove(void)
{
  ksetlocalMenu (6, 0) ;
}

static void beforeSaveAs(void)
{
  checkWriteAccess() ;
  if (!isWriteAccess())
    return ;
  ksetlocalMenu (1, 0) ;
}

static void beforeEmpty(void)
{
  ksetlocalMenu (7, 0) ;
}


static void beforeCopy(void)
{
  ksetlocalMenu (2, 0) ;
}

static void drawMenus(void)
{
  int i,nx,ny,menuregion, box;
  float line ;
  static int wp = 0 ;
  FREEOPT *menu;
  KEYSET kSet;

  static MENUOPT dumpAs[] = {
    {menuSpacer,"Mail/Write data as:.."}, 
    {dumpAsAce,			"  ace file"},
    {dumpAsAceWithTimeStamps,	"  ace file with timestamps"},
    {dumpAsNames,		"  list of names"},
    {dumpAsDnaFasta,		"  DNA in FASTA format"},
    {dumpAsCDSDnaFasta,		"  CDS DNA in FASTA format"},
    {dumpAsProtFasta,		"  Protein in FASTA format"},
    {dumpAsCDSProtFasta,	"  CDS Protein in FASTA format"},
    {dumpAsProtAlign,		"  Protein alignment"},
    {0,0}
  };

  static MENUOPT combine[] = {
    {combineOR,"OR"},
    {combineAND,"AND"},
    {combineMINUS,"MINUS"},
    {combineXOR,"XOR"},
    {0,0}
  };

  static MENUOPT query[] = {
    {openQueryBox, "Query"},
    {follow,"Follow"},
    {findneigh,"Find Neighbours"}, /* redundant on morer info -> background ? */
    {findEmptyObj, "Find Empty Obj"},
    {queByExam,"Query by Example"},
    {grepset,"Text search"},
    {0,0}
  };
  
  /*  static MENUOPT selectMenu[] = { 
    {clearSelected,"Clear Selection"},
    {selectAllKeySet,"Select All"},
    {reverseSelectedKeySet,"Reverse"},
    {removeSelectedKeySet,"Remove Selected"},
    {selectedToNewKeySet,"Selected To New Keyset"},
    {0,0}
  };
  */

  static MENUOPT modifSelectMenu[] = {
    {beforeAddRemove, "Add/remove items one by one"}, 
    {menuSpacer,""},
    {beforeEmpty,"New empty keyset"},
    {beforeCopy,"Copy whole keyset"},    
    {beforeSaveAs,"Save current keyset as"},
    {menuSpacer,""},
    {selectAllKeySet,"Select All"},
    {clearSelected,"Clear Selection"},
    {reverseSelectedKeySet,"Reverse selection"},
    {removeSelectedKeySet,"Remove selected items"},
    {selectedToNewKeySet,"Copy selected items"},
    {0,0}
  };

  static MENUOPT editMenu[] = {  
    {editAll,"Select and Edit all these objects"},
    {killAll,"Kill all these objects"},
    {editSelected,"Edit only the selected objects"},
    {killSelected,"Kill only the selected objects"},
    {0,0}
  } ;

  KeySetDisp look = currentKeySetDisp("drawMenus");

  kSet = look->kSet;
  if (!wp) wp = writeAccessPossible() ? 1 : 2 ;
  graphRegister(LEFT_DOWN, controlLeftDown);
  graphClear () ;
  graphFitBounds (&nx, &ny) ;
  
  /* draw a line at the bottom of the window to extend box0
   * to the dimensions of the window */
  graphLine(0.0, (float)ny+1, (float)nx+1.0, (float)ny+1);

  /**********************************************/

  menuregion = graphBoxStart();
  
  /****  first line of buttons ****/
  graphLine(0.0, 0.0, (float)nx+1.0, 0.0);

  line = .5 ;
  box = graphButton("Show As..:", nextShowtype, 1.0 ,line); 
  menu = displayGetShowTypesMenu();
  graphBoxFreeMenu(box, (FreeMenuFunction)changeShowtype,menu);

  for(i=1;i<=menu[0].key;i++)
    if(look->showtype == menu[i].key)
      {
        graphText(menu[i].text, 13, line + .2) ;
	break;
      }
  
  if (biblioPossible(kSet))
    {
      box = graphButton("Biblio",relatedBiblio,23.5,line);
      graphBoxMenu(box, 0);
    }
    
  box = graphButton("More Info",beforeForest,31.1,line) ;
  graphBoxMenu(box, 0);

  box = graphButton("Quit",graphDestroy,42.0,line);
  graphBoxMenu(box, 0);

  line += 1.7;			/* end of first line */

  /****  second line of buttons ****/

  box = graphButton("Query..",openQueryBox,1.0,line);
  graphBoxMenu(box, query);

  box = graphButton("Select/Modif..",clearSelected,9.9,line) ; /* was 0.5); */
  graphBoxMenu(box, modifSelectMenu) ;

  box = graphButton("Export..",none,25.5,line) ; 
  graphBoxMenu(box, dumpAs);

  box = graphButton("Other",otherTools,35.1,line) ; /* was 0.5); */
  graphBoxMenu(box, 0);

  box = graphButton("Help",help,42.0,line);
  graphBoxMenu(box, 0);

  line += 1.7 ;			/* end of second line */

  /****  optional third line of buttons ****/
  N3rdLINE = wp ? 3 : 2 ;  /* number of boxes of third line */
  if (isOtherTools)
    { 
      /* ATTENTION, adjust N3rdLINE if you edit this piece of code
       * ++ it for each additional box */
  
      box = graphButton("Combine..",none,1.0,line); 
      graphBoxMenu(box, combine);

      graphButton(look->showClass ? "Hide Class" : "Class Names" ,showClasses,12.0,line) ;

      if (wp)
	box = graphButton("Edit/Kill..",editSelected,25.5,line); 
      graphBoxMenu(box, editMenu);

      line += 1.7 ;
    } 

  NQueryLine = 2 ; /* number of boxes of query line */
  if (look->showQuery)
    { 
      graphText ("Query: ", 3, line) ;
      look->queryBox = graphTextScrollEntry (look->queryBuffer,254, nx-10, 9, line, keySetDoQuery) ;  

      line += 1.7;
    }
  if (look->showQuery != 2) /* 2 means focus desired */
    {
      graphEntryDisable() ;
      graphRegister (KEYBOARD, localKbd) ;
    }

  graphLine(0.0, line, (float)nx+1.0, line);
  graphBoxEnd();		/* end of menuregion box */

  if (look->message)
    {
      int len =  strlen(look->message);
      char *start = look->message;
      char *cp = start;

	      
      while (cp <= look->message + len)
	{
	  if (*cp == '\0' || *cp == '\n')
	    {
	      *cp = '\0';
	      graphText (start, 2, line+0.3) ;
	      start = cp+1;
	      line += 1.0;
	    }
	  cp++;
	}
      line += 0.7 ;
    }  

  TOPGAP = line;

  /************/

  if (look == selectedKeySet)
    graphBoxDraw (menuregion,BLACK,PALERED) ;
  else
    graphBoxDraw (menuregion,BLACK,WHITE) ;

  return;
} /* drawMenus */

static void scrollMiddleUp(double x, double y)
     /* re-center scrollbar */
{
  float newy;
  KeySetDisp look = currentKeySetDisp("scrollMiddleUp");
  
  if(look->keySetAlpha && look->scrollBarBox)
    {
      newy = (float)y-(look->scrolllen/2.0);
      setPage(&newy);
      
      if(look->base == keySetMax(look->keySetAlpha))
	{ 
	  --look->base;
	  pageUp();
	  return;
	}
      if (look != selectedKeySet)
	keySetSelect () ; 

      keySetShow2 (look->kSet, look) ; 
    }
}
 
/************************** eof *****************************/
