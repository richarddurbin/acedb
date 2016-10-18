/*  File: longtextdisp.c
 *  Author: Fred Wobus (fw@sanger.ac.uk)
 *  Copyright (C) J Thierry-Mieg and R Durbin, 1999
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
 * Description: graphical display of longtext data
 * Exported functions:
 *              longTextDisplay - DisplayFunc for "DtLongText" type
 * HISTORY:
 * Last edited: May 16 09:46 2003 (edgrif)
 * * Jan 28 09:19 1999 (fw): created to accomodate graphical routines 
 *              from longtext.c
 * Created: Thu Jan 28 09:12:27 1999 (fw)
 *-------------------------------------------------------------------
 */

/* $Id: longtextdisp.c,v 1.20 2003-05-16 08:47:48 edgrif Exp $ */

#include <wh/acedb.h>
#include <wh/aceio.h>
#include <whooks/classes.h>
#include <whooks/sysclass.h>
#include <wh/bs.h>
#include <wh/a.h>
#include <wh/lex.h>
#include <wh/pick.h>
#include <wh/longtext.h>
#include <wh/query.h>
#include <wh/parse.h>
#include <wh/display.h>
#include <wh/keysetdisp.h>

/*****************************************************************/

typedef struct LOOKSTUFF
  { int   magic;        /* == MAGIC */
    KEY   key ;
    Graph graph ;
    int   activeBox,  editing ;
    Stack textStack ;
    int myTextStack ;
    KEYSET box2key ;
    Array segs,      	/* array of SEG's from the obj */
          boxIndex ;    /* if >256 a SEG, else something special */
  } *LOOK ;

typedef struct
  { KEY key ;
    float x, dx ;
    unsigned int flag ; /* I use shift right operators */
  } SEG ;
#define segFormat "k2fi"

static void longTextDestroy (void) ;
static void longTextEditor (void) ;
static void longTextRecover (void) ;
#ifdef ACEDB_MAILER
static void longTextMail (void) ;
#endif
static void longTextRead (void) ;
static void longTextDecorate (void) ;
static void generalGrep(void) ;
static void longTextPick(int box, double x_unused, double y_unused, int modifier_unused);
static void longTextDraw (LOOK look, BOOL reuse);

static MENUOPT longTextMenu[] = 
            { {graphDestroy, "Quit"},
		{help,"Help"},
		{graphPrint,"Print"},
		{displayPreserve,"Preserve"},
		{longTextDecorate, "Search"},
#if !defined(WIN32)
		{longTextEditor,"Edit"},
#endif
		{longTextRecover,"Save"},
#ifdef ACEDB_MAILER
		{longTextMail,"Mail"},
#endif
		{longTextRead,"Read"},
		{generalGrep,"General Grep"},
		{0, 0}
            } ;

static int MAGIC = 464565 ;	/* also use address as graphAss handle */
#define LOOKGET(name)     LOOK look ; \
                          if (!graphAssFind (&MAGIC, &look)) \
		            messcrash ("graph not found in %s",name) ; \
                          if (look->magic != MAGIC) \
                            messcrash ("%s received a wrong pointer",name)


/**************************************************************/

BOOL longTextDisplay (KEY key, KEY from, BOOL isOldGraph, void *unused)
{
  LOOK look=(LOOK)messalloc(sizeof(struct LOOKSTUFF)) ;
  KEY  curr = 0 ;
  
  look->magic = MAGIC;

  if (key && class(key) != _VLongText)
    goto abort ;

  if (! (look->textStack = stackGet(key)))
    goto abort ;

  look->key = key ;

  look->boxIndex = arrayCreate (64,SEG*) ;
  look->activeBox = 0 ;
  keySetDestroy(look->box2key) ;

  if (isOldGraph)
    { 
      longTextDestroy () ;
      graphClear () ;
      graphGoto (0,0) ;
      graphRetitle (name (key)) ;
      graphAssRemove (&MAGIC) ;
    }
  else 
    { 
      displayCreate("DtLongText");
      
      graphRetitle (name(key)) ;
      graphRegister (DESTROY, longTextDestroy) ;
      graphRegister (PICK, longTextPick) ;
/*
      graphRegister (KEYBOARD, longTextKbd) ;
      graphRegister (MIDDLE_DOWN, longTextMiddleDown) ;
 */
      
      graphMenu (longTextMenu) ;
    }

  look->graph = graphActive() ;
  look->myTextStack = stackExists(look->textStack) ;
  graphAssociate (&MAGIC, look) ;
  longTextDraw (look, curr) ;

  return TRUE ;

abort :
  messfree (look) ;
  return FALSE ;
} /* longTextDisplay */

/************************************************************/

static void longTextPick(int box, double x_unused, double y_unused, int modifier_unused)
{
  LOOKGET("longTextPick") ;

  if (!box || ! keySetExists(look->box2key) || box >= keySetMax(look->box2key))
    return ;
  
  if (box == look->activeBox)         /* a second hit - follow it */
    display (keySet(look->box2key, box), 0, 0) ;
  else
    { if (look->activeBox)
	graphBoxDraw(box, BLACK, YELLOW) ;
      graphBoxDraw(box, BLACK, RED) ;
    }

  look->activeBox = box ;
}

/************************************/

static void longTextRead (void)
{
  char *cp = 0 ;
  Stack s ; 
  ACEIN text_in = 0 ;
  static char directory[DIR_BUFFER_SIZE] = "";
  static char filename[FIL_BUFFER_SIZE] = "";
  LOOKGET("longTextRead") ;

  text_in = aceInCreateFromChooser (".ace-file to read from ?",
				    directory, filename, "ace", "r", 0);
  if (!text_in)
    return;
				    
  if (look->editing &&
      messQuery ("You did not save the edited form of the former text, should I"))
    longTextRecover () ;    
  
  look->editing = FALSE ;
  s = stackCreate(500) ;

  aceInSpecial (text_in,"\n\t") ;
  while ((cp = aceInCard(text_in)))
    {
      if (strcmp(cp,"***LongTextEnd***") == 0)
	break ; 
      else
	pushText(s, cp) ;
    }
  aceInDestroy (text_in) ;

  stackDestroy(look->textStack) ;
  look->textStack = s ;

  return;
} /* longTextRead */

/**************************************************************/

static KEYSET getClassList(void)
{
  KEYSET classList = keySetCreate(), ks ;
  int i, j = 0 ;
  KEY key, key1, mySet ;
  unsigned char u ;
  KeySetDisp dummy ;

  if (keySetActive (&ks, &dummy))
    { i = keySetMax(ks) ; j = 0 ;
      while (i--)
	{ key = key1 = keySet(ks, i) ;
	  if (class(key1) == _VModel)
	    lexword2key (name(key1) + 1 , &key, _VClass) ;
	  if (class(key) == _VClass &&
	      pickIsA(&key, &u)
	      )
	    keySet(classList, j++) = key ;
	}
    }
  if (j > 0)
    return classList ;

  if (lexword2key("searchedClasses", &mySet, _VKeySet) &&
      (ks = arrayGet(mySet, KEY, "k")))
    { i = keySetMax(ks) ; j = 0 ;
      while (i--)
	{ key = key1 = keySet(ks, i) ;
	  if (class(key1) == _VModel)
	    lexword2key (name(key1) + 1 , &key, _VClass) ;
	  if (class(key) == _VClass &&
	      pickIsA(&key, &u)
	      )
	    keySet(classList, j++) = key ;
	}
    }

  if (j == 0)
    { messout("This button searches the text for names in selected classes. "
	      "Either those in the active keyset "
	      "or by default those in the keySet called \"searchedClasses\".\n"
	      "You must either select a keySet of classes "
	      "or construct the default keySet \"searchedClasses\".") ;
    }
  return classList ;
}

/**************************************************************/

static void longTextDecorate (void)
{ int x = 1 , y = 4, maxx = 0, n , box ;
  char *cp , c ;
  Stack s ; 
  KEY key, *classp=0 ;
  KEYSET classList = 0 ;
  LOOKGET("longTextDecorate") ;
  
  messStatus ("Searching the text") ;
  graphClear() ;
  graphButtons (longTextMenu, 3, 1.5, 60) ;

  s = look->textStack ;
  if (stackExists(s) != look->myTextStack)
    { messout("longTextDraw lost its stack, sorry") ;
      return ;
    }

  classList = getClassList() ;
  
  look->box2key = keySetReCreate(look->box2key) ;
  stackCursor(s, 0) ;

  while ((cp = stackNextText(s)))
    { 
      ACEIN text_io = aceInCreateFromText(cp, "", 0);

      aceInSpecial(text_io,"") ;
      aceInCard(text_io) ;
      while (*cp && *cp == ' ')	/* maintain indenting after decorate */
	{ ++x ; ++cp ; }
      while((cp = aceInWordCut(text_io, " .,/;:[]{}()!@#$%^&*+=\\|\"\'~",&c)) || c)
	{
	  if (cp)
	    { 
	      n = keySetMax(classList) ;
	      if (n)
		{
		  classp = arrp(classList, 0, KEY) ;
		  
		  while (n--)
		    if (lexword2key(cp, &key, *classp++))
		      { 
			keySet(look->box2key, box = graphBoxStart()) = key ;
			graphText (cp, x, y) ;
			graphBoxEnd() ;
			graphBoxDraw(box, BLACK, YELLOW) ;
			goto done ;
		      }
		}

	      graphText (cp, x, y) ;
	    done:
	      x += strlen(cp) ;
	      
	      if (x > maxx)
		maxx = x ;
	    }
	  if (c)
	    graphText(messprintf("%c", c), x++, y) ;
	}
      x = 1 ;
      y++;

      aceInDestroy (text_io);
    }
  graphTextBounds( maxx + 2 , y+3) ;
  keySetDestroy(classList) ;
  graphRedraw() ;

  return;
} /* longTextDecorate */

/**************************************************************/

static void longTextDraw (LOOK look, BOOL reuse)
{ int x = 1 , y = 4, maxx = 0  ;
  char *cp ;
  Stack s = look->textStack ;
  
  graphClear() ;
  graphButtons (longTextMenu, 3, 1.5, 60) ;

  if (stackExists(s) != look->myTextStack)
    { messout("longTextDraw lost its stack, sorry") ;
      return ;
    }
  stackCursor(s, 0) ;
  while ((cp = stackNextText(s)))
    {
      graphText (cp, x, y) ;
      x += strlen(cp) + 1 ;
      if (x > maxx)
	maxx = x ;
      x = 1 ;
      y++;
    }
  graphTextBounds( maxx + 2 , y+3) ;
  graphRedraw() ;
}

/**********************************/

#if !defined (WIN32)
static void longTextEditor (void) 
{
  FILE *f = NULL ;
  static int n = 1 ;
  Stack  s ;
  char *cp ;
  LOOKGET("longTextEditor") ;
  
  s = look->textStack ;
  if (!stackExists(s))
    return ;

  /* It does this solely to bump up n to make a unique filename....ugh... */
  while(filCheckName(messprintf("/tmp/acedb.editor.%d",n),"","r")
	|| filCheckName(messprintf("/tmp/acedb.editor.%d.done",n),"","r"))
    {
      n++ ;
    }

  f = filopen(messprintf("/tmp/acedb.editor.%d",n),"","w") ;
  
  if (!f)
    {
      messprintf("Sorry, i cannot create the file %s",
		 messprintf("/tmp/acedb.editor.%d",n),"w") ;
      return ;
    }

  stackCursor(s, 0) ;
  while ((cp = stackNextText(s)))
    fprintf(f, "%s\n", cp) ;
  filclose(f) ;
 
  system(messprintf("acedb_editor /tmp/acedb.editor.%d &", n)) ;
  look->editing = n++ ;
  displayPreserve() ;

  return ;
}
#endif

/**********************************/

#ifdef ACEDB_MAILER
static void longTextMail (void) 
{ 
  KEYSET ks ;
  KeySetDisp dummy;
  LOOKGET("longTextEditor") ;
 
  if (!stackExists(look->textStack))
    return ;
  
  if (!keySetActive(&ks, &dummy))
    { messout ("First select a keySet containing the recipients") ;
      return ;
    }
  acedbMailer(0, ks, look->textStack) ;
}
#endif

/**********************************/

static void longTextRecover (void) 
{
  Stack s ;
  char *cp ;
  ACEIN text_io;
  ParseFuncReturn parsing ;
  char *err_txt ;
  LOOKGET("longTextRecover") ;

  
  if (look->editing == 0)
    return ;
 lao:

  text_io = aceInCreateFromFile 
    (messprintf("/tmp/acedb.editor.%d.done",look->editing), "r", "", 0);
  
  if (!text_io)
    {
      if (!messQuery ("To store the edited text, first exit the text editor.\n"
		      " Are you ready ?"))
	return ;
      else
	goto lao ;
    }

  aceInSpecial (text_io,"\n\t") ;

  parsing = longTextParse(text_io, look->key, &err_txt) ;
  if (parsing == PARSEFUNC_OK && (s = stackGet(look->key)))
    { 
      stackClear(look->textStack) ;
      stackCursor(s, 0) ;
      while ((cp = stackNextText(s)))
	pushText(look->textStack,cp) ;
      stackDestroy(s) ;
    }
  else if (parsing == PARSEFUNC_ERR)
    messout("Failed to recover the file /tmp/acedb.editor.%d.done, "
	    "longText parse failed, reason was: %s", look->editing, err_txt) ;
  else
    messout("I failed to recover the file /tmp/acedb.editor.%d.done",
	    look->editing) ;

  aceInDestroy (text_io) ;
  look->editing = 0 ;
  longTextDraw (look, 0) ;

  return;
} /* longTextRecover */

/**********************************/

/************************************************************/
/***************** Registered routines *********************/

static void longTextDestroy (void)
{
  LOOKGET("longTextDestroy") ;

  if (look->editing &&
      messQuery ("You did not save the edited form of this text, should I")
      )
    longTextRecover () ;    
  
  arrayDestroy(look->segs) ;
  arrayDestroy(look->boxIndex) ;
  stackDestroy(look->textStack) ;
  keySetDestroy(look->box2key) ;

  look->magic = 0 ;
  messfree (look) ;
  
  graphAssRemove (&MAGIC) ;
}

/**************************************************************/
/**************************************************************/


   /* general LongText Analyser */

static void generalGrep(void)
{
  static char dName[DIR_BUFFER_SIZE]="", filName[FIL_BUFFER_SIZE]="" ;
  KEY  _Abstract, *ppp, *classp, key, *kp, ltk;
  char *cp, cutter;
  Stack s ;
  OBJ obj = 0 ;
  int i, nX = 0 , nLT = 0 , xn ;
  KEYSET xs = 0 , ks=0, ks1=0, classList=0 ;
  FILE *g, *f ; 
  KeySetDisp dummy ;

  if (!lexword2key("Abstract",&_Abstract,0))
    { messout ("Tag Abstract is absent from this database, "
	       "nothing I can report, sorry") ;
      return ;
    }
  if (!keySetActive(&ks1, &dummy))
    { messout ("First select a keyset containing Papers") ;
      return ;
    }

  if(!(classList = getClassList()))
    return ;


  if (!messQuery("This is a general grepper on all long texts,\n"
		 "You ll be asked for on output file name.\n"
		 "do you want to proceed ?"))
    return ;

  ks = query(ks1,"CLASS Paper ; Abstract") ;
  if (!keySetMax(ks))
    { messout ("First select a keyset containing paper with abstracts") ;
      keySetDestroy (ks) ;
      return ;
    }

  f = filqueryopen(dName, filName, "ace", "w", 
		   "I need a place to export the Grep results") ;
  g = filopen(filName, "bad","w") ;
  if (!f || !g)
    { 
      messout("Got a cancel file, i quit") ;
      if (f) filclose(f) ;
      if (g) filclose(g) ;
      keySetDestroy (ks) ;
      return ;
    }
  
  i = keySetMax(ks) ;
  ppp = arrp(ks, 0, KEY) - 1 ;
  while (ppp++, i--)
    { 
      obj = bsCreate(*ppp) ;
      if (!obj) continue ;
      ltk = 0 ;
      bsGetKey (obj,_Abstract,&ltk) ;
      bsDestroy (obj) ;
      if (!ltk) continue ;

      s = stackGet(ltk) ;
      if (!stackExists(s))
	continue ;
      nLT++ ;
      stackCursor(s,0) ;
      xs = keySetReCreate(xs) ; xn = 0 ;

      while (s && (cp = stackNextText(s)))
	{
	  ACEIN fi = aceInCreateFromText (cp, "", 0);
	  	  
	  aceInSpecial(fi,"") ;
	  aceInCard(fi) ;
	  	  
	  while((cp = aceInWordCut(fi," .,/;:[]{}()!@#$%^&*+=\\|\"\'~\n",&cutter)) || cutter)
	    if (cp)
	      { int n = keySetMax(classList) ;
		classp = arrp(classList, 0, KEY) ;
		while (n--)
		  if (lexword2key(cp, &key, *classp++))
		    { 
		      keySet(xs, xn++) = key ;
		      break ;
		    }
		if (islower((int)*cp) &&
		    islower((int)*(cp+1)) &&
		    islower((int)*(cp+2))&&
		    (*(cp+3) == '-') &&
		    isdigit((int)*(cp+4)) &&
		    !lexword2key(cp, &key, _VLocus))
		  fprintf(g, "Unknown Gene : %s\n", cp) ;
	      }
	  aceInDestroy (fi) ;
	}


      if (xn)
	{ 
	  keySetSort(xs) ;
	  keySetCompress(xs) ;
	  
	  fprintf(f, "\nPaper : %s\n", name(*ppp)) ;
	  xn = keySetMax(xs) ;
	  nX += xn ;
	  kp = arrp(xs, 0, KEY) - 1 ;
	  while (kp++, xn--)
	    fprintf(f, "%s : %s \n", className(*kp), name(*kp)) ;
	  
	}
      stackDestroy(s) ;
    }

  messout ("#*#*#*# Found %d XREF in %d LongTexts \n", nX, nLT) ;

  keySetDestroy(ks) ;

  filclose(f) ;
  filclose(g) ;
  keySetDestroy(classList) ;
  keySetDestroy(xs) ;
}

/************************** eof *******************************/
