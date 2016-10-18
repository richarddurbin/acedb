/*  File: action.c
 *  Author: Jean Thierry-Mieg (mieg@mrc-lmb.cam.ac.uk)
 *  Copyright (C) J Thierry-Mieg and R Durbin, 1992
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
 **    Calls an action application.
 * Exported functions:
 * HISTORY:
 * Last edited: May 20 15:46 2008 (edgrif)
 * * Dec  3 14:44 1998 (edgrif): Change calls to new interface to aceversion.
 * * Dec  2 15:56 1998 (edgrif): Fix minor bug in acedb version message.
 * * (I assume these changes made by rbrusk)
 *		-	'externalAsynchroneCommand' must return a value
 *		-	'SIGUSR1' : undefined in WIN32; user can user windows mgr "End Task"
 * * May  7 17:40 1996 (rd)
 * * May 12 17:39 1993 (mieg): pick_me_to_call now expects: command parameters
 * Created: Fri Jan 10 23:40:16 1992 (mieg)
 *-------------------------------------------------------------------
 */

/* $Id: action.c,v 1.47 2008-05-20 15:42:46 edgrif Exp $ */

#include <glib.h>

#include "acedb.h"
#include "aceio.h"
#include "bs.h"
#include "lex.h"
#include "whooks/systags.h"
#include "whooks/sysclass.h"
#include "whooks/tags.h"
#include "pick.h"
#include "call.h"
#include "dbpath.h"
#include "display.h"
#include "gex.h"

/************************************************************/


void externalDisplay (KEY key)  
{
  OBJ obj = bsCreate(key) ;
  char *prog = 0, *args = 0 ;
  char *progpath;

  if (!obj)
    messcrash ("Pick_me_to_call called on unkown obj %s", name(key)) ;
  if (!bsGetData (obj, _Pick_me_to_call, _Text, &prog))
    { messout ("Sorry, a program and its parameters are expected here") ;
      goto abort ;
    }
 
  bsGetData (obj, _bsRight, _Text, &args) ;  /* get the parameters */
  
  /* If the prog exists in wscripts, use that, else run the command
     bare and assume it's on the user's path */

  progpath = dbPathFilName(WSCRIPTS, prog, "", "x", 0);
  
  if (progpath)
    {
      callScript(progpath, args);
      messfree(progpath);
    }
  else
    callScript (prog, args) ; 
  
 abort:
  bsDestroy (obj) ;
}

/**************************************************************/
/**************************************************************/

ACETMP exportStackToMail (Stack s)
{
  ACETMP tmpFile = NULL;
  char *cp;
  
  tmpFile = aceTmpCreate ("w", 0);
  if (tmpFile)

  if (stackExists(s))
    { 
      stackCursor (s, 0) ;
      while ((cp = stackNextText(s)))
	aceOutPrint (aceTmpGetOutput(tmpFile), "%s\n", cp) ;
    }
  aceTmpClose (tmpFile);

  return tmpFile ;
} /* exportStackToMail */

/****************/

void acedbMailComments (void)
{
  Stack s ;
  ACETMP tmpFile;

  if (!messQuery ("Do you want to send a comment "
		  "to the authors of this program"))
    return ;

  s = stackCreate (100) ;
  pushText(s, messprintf("Code release:   %s,  %s\n",
			 aceGetVersionString(), aceGetLinkDateString())) ;
  tmpFile = exportStackToMail (s);
  stackDestroy (s);

  if (tmpFile)
    {
      char *script = dbPathFilName(WSCRIPTS, "acedb_mailer", "", "x", 0);
      if (script)
	{
	  callScript (script, 
		      messprintf("\"mieg@crbm.cnrs-mop.fr rd@sanger.ac.uk\" %s", aceTmpGetFileName(tmpFile))) ;
	  messfree(script);
	}
      aceTmpDestroy (tmpFile);
    }

  return;
} /* acedbMailComments */

void acedbMailer (KEY key, KEYSET ks, Stack sText)
{ 
  char *cp ;
  Stack  sm = stackCreate(50) ;
  int i, nmails = 0 ;
  OBJ obj ;
  int manip = 0 ;
  ACETMP tmpFile;

  tmpFile = exportStackToMail (sText);
  if (!tmpFile)
    return;

  if (!keySetExists(ks))
    { ks = keySetCreate() ;  /* I will destroy at end */
      manip = 10 ;
    }
  if (key)
    { keySet(ks, keySetMax(ks)) = key ;
      manip++ ;
    }
  
  for (i = 0 ; i < keySetMax(ks) ; i++)
    { if (pickType(keySet(ks,i)) != 'B')
	continue ;
      obj = bsCreate(keySet(ks,i)) ;
  
      if (!obj)
	continue ;
      if (bsGetData(obj, _E_mail, _Text,&cp))
	do
	  { 
	    if (nmails++)
	      catText(sm, ", ") ;
	    else
	      pushText(sm,"\'") ;
	    catText(sm, cp) ;
	  } while (bsGetData(obj, _bsDown, _Text,&cp)) ;
      if (stackMark(sm) > 800)
	{ messout ("%d recipients is enough, I stop at %s",
		   nmails, name(keySet(ks,i))) ;
	  break ;
	}

      bsDestroy(obj) ;
    }

  catText (sm, "\'  ") ;
  catText (sm, aceTmpGetFileName(tmpFile)) ;
  /*  catText (sm, " & ") ; */
  /* must wait for script to finish, so we can safely remove the
   * temp-file after its termination */
  
  if (!nmails)
    messout("No E_mail address in this keySet, sorry") ;
  else
     {
       char *script = dbPathFilName(WSCRIPTS, "acedb_mailer", "", "x", 0);
       if (script)
	 {
	   callScript (script, stackText(sm,0)) ;
	   messfree(script);
	 }
     }

  aceTmpDestroy (tmpFile);

  stackDestroy(sm) ;

  if (manip >= 10)
    keySetDestroy(ks) ;
  else if (manip == 1)
    keySetMax(ks)-- ;

  return;
} /* acedbMailer */

/**************************************************************/


/* call an external shell command and print output in a text_scroll window
 *
 * This is a replacement for the old graph based text window, it has the advantage
 * that it uses gtk directly and provides cut/paste/scrolling but...it has the
 * disadvantage that it will use more memory as it collects all the output into
 * one string and then this is _copied_ into the text widget.
 * 
 * If this proves to be a problem I expect there is a way to feed the text to the
 * text widget a line a time. */
static void doExternalFileDisplay(char *title, FILE *f, Stack s, BOOL isPipe)
{
  char *cp ;
  Graph old ; 
  GString *text ;

  old = graphActive() ; 

  text = g_string_new(NULL) ;

  if (!title)
    title = "<untitled>" ;

  if (stackExists(s))
    {
      stackCursor(s, 0) ;

      while ((cp = stackNextText (s)))
	{
	  g_string_append_printf(text, "%s\n", cp) ;
	}
    }
  else if (f)
    {
      int level = 0 ;

      level = isPipe ? freesetpipe(f,"")  : freesetfile(f,"") ;

      if (level)
	{
	  freespecial ("\n\t") ;

	  while ((cp = freecard(level))) /* closes the file */
	    {
	      g_string_append_printf(text, "%s\n", cp) ;
	    }

	  freeclose (level) ;
	}
    }


  gexTextEditorNew(title, text->str, 0,
		   NULL, NULL, NULL,
		   FALSE, TRUE, TRUE);

  g_string_free(text, TRUE) ;

  graphActivate (old) ;

  return ;
}


void externalFileDisplay (char *title, FILE *f, Stack s)
{
  doExternalFileDisplay(title, f, s, FALSE) ;

  return ;
}

void externalPipeDisplay (char *title, FILE *f, Stack s)
{
  doExternalFileDisplay(title, f, s, TRUE) ;

  return ;
}

void externalCommand (char *command)
{
  char *cp = command ;

  while (*cp && *cp != ' ') cp++ ;
  *cp++ = 0 ; /* first space or terminal 0 */

#if !defined(MACINTOSH)
  doExternalFileDisplay (command, callScriptPipe(command, cp), 0, TRUE) ;
#endif

  return ;
}



#if defined(MACINTOSH)
BOOL externalAsynchroneCommand (char *command, char *parms,
                                void *look, void(*g)(ACEIN fi, void *lk))
{
  return ;
}

#else

#include <signal.h>

/* This system is very simple minded, 
   We create a communication file xx->tmp_file
   then call the command in the background
   and test for the existence of xx->tmp_file.out 
   when a SIGUSER1 signal is received.
    If the outfile exists, the registered function is
   called on that output file, and cleanup is done
   */
   
    
static Array xxA = 0 ;

typedef struct xxLookStruct 
{ 
  int i ; /* index in xxA, 0: reusable */
  ExternalFuncType g;
  /*  void (*g)(FILE *f, void *lk) ;*/
  char *command ;
  ACETMP tmp_file;
  void *look ;
} XX ;

static void xxDestroy (XX *xx)
{ 	  /* I think that xx->look should be destroyed in the calling code */
  xx->i = 0 ;  /* slot can be reused */
  messfree(xx->command) ;

  /* remove signaling file /tmp/xxxx.out */
  filremove (aceTmpGetFileName(xx->tmp_file), "out") ;

  aceTmpDestroy (xx->tmp_file);
}

static void externalCompletion (int result)
{
  ACEIN fi = 0 ;
  XX *xx ;
  int i ;
  char *filename;

  if (!arrayExists (xxA))
    return ;

  for (i = 0 ; i < arrayMax(xxA) ; i++)
    {
      xx = arrp (xxA, i, XX) ;

      filename = filGetName(aceTmpGetFileName(xx->tmp_file), "out", "r", 0);
      if (xx->i &&                               /* not destroyed */
	  filename &&     /* no messout if absent  */
	  (fi = aceInCreateFromFile (filename, "r", "", 0))) /* would messout if absent */
	{
	  if (xx->g)  /* must destroy xx->look */
	    (*(xx->g))(fi, xx->look) ; /* will destroy fi */
	  xxDestroy (xx) ;
	}
    }
  
  if (filename)
    messfree(filename);

  return;
} /* externalCompletion */

BOOL externalAsynchroneCommand (char *command, char *parms,
                                void *look, ExternalFuncType g)/*void(*g)(ACEIN fi, void *lk))*/
{
  int i ;
  ACETMP tmp_file;
  XX  *xx ; 
  Stack s = 0 ;
  int pid = getpid() ;
  BOOL ret;

  if (!xxA)
    xxA = arrayCreate (20, XX) ;

  tmp_file = aceTmpCreate ("w", 0);
  if (!tmp_file)
    return FALSE ;

  aceOutPrint(aceTmpGetOutput(tmp_file), "// Call to: %s\n", command) ;
  aceTmpClose (tmp_file) ;

  for (i = 1 ; i < arrayMax(xxA) ; i++) /* start at i = 1 ! */
    if (!array (xxA, i, XX).i)
      break ;
  xx = arrayp (xxA, i, XX) ;  
  xx->i = i ;
  xx->command = dbPathFilName(WSCRIPTS, command, "",  "x", 0) ;
  xx->look = look ;
  xx->g = g ;
  xx->tmp_file = tmp_file ;

  signal (SIGUSR1, &externalCompletion) ;  /* could be called only once */

  s  = stackCreate (250) ;
  pushText (s, messprintf(" %d ", pid)) ;
  catText (s, aceTmpGetFileName(xx->tmp_file)) ;
  catText (s, "  ") ;
  if (*parms) catText (s, parms) ;
  catText (s, " &") ;
  
  /* because of &background, callScript always returns TRUE */
  if (xx->command)
    {
      callScript (command, stackText (s, 0));  
      ret = TRUE ;
    }
  else
    {
      messout ("Cannot find script %s", command) ;  
      if (xx->i && xx->g)
	(*(xx->g))(0, xx->look) ;
      xxDestroy (xx) ;
      ret = FALSE ;
    }
  
  stackDestroy(s);
  return ret ;
}

#endif /* MACINTOSH */

/***************************************************************/
/************ action display (currently unused) **************/

typedef struct LOOKSTUFF
  { int   magic;        /* == MAGIC */
    KEY   key ;
    Graph graph ;
  } *LOOK ;

static MENUOPT actionMenu[] = {
  {graphDestroy, "Quit"},
  {help,"Help"},
  {graphPrint,"Print"},
  {displayPreserve,"Preserve"},
   {0, 0}
} ;

static int MAGIC = 723544 ;	/* use also for graphAssociate pointer */

#define LOOKGET(name)     LOOK look ; \
                          if (!graphAssFind (&MAGIC, &look)) \
		            messcrash ("graph not found in %s", name) ; \
			  if (!look) \
                            messcrash ("%s received a null pointer", name) ; \
                          if (look->magic != MAGIC) \
                            messcrash ("%s received a wrong pointer", name)

static BOOL actionExecute (LOOK look)
{
  graphText(messprintf ("Action on %s not yet code sorry", 
			name(look->key)), 3, 3) ;
  graphRedraw() ;
  return TRUE ;
}

static void actionDestroy (void)
{
  LOOKGET("actionDestroy") ;

  look->magic = 0 ;
  messfree (look) ;
  
  graphAssRemove (&MAGIC) ;
}

BOOL actionDisplay (KEY key, KEY from, BOOL isOldGraph)
{
  LOOK look ;
  OBJ Action = 0 ;
  int _VAction = 0 ;
  KEY akey ;

  lexword2key("Action", &akey, _VMainClasses) ;
  _VAction = KEYKEY(akey) ;

  if (!key || !_VAction || class(key) != _VAction ||
      !(Action = bsCreate(key)))
    return FALSE ;

  bsDestroy(Action) ;

  look=(LOOK)messalloc(sizeof(struct LOOKSTUFF)) ;
  look->key = key ;
  look->magic = MAGIC;

  if (isOldGraph)
    { actionDestroy () ;
      graphClear () ;
      graphGoto (0,0) ;
      graphRetitle (name(key)) ;
      graphAssRemove (&MAGIC) ;
    }
  else 
    {
      displayCreate ("DtAction");
      
      graphRetitle (name(key)) ;
      graphRegister (DESTROY, actionDestroy) ;
      graphMenu (actionMenu) ;
    }
  graphAssociate (&MAGIC, look) ;
  look->graph = graphActive () ;
  actionExecute (look) ;
  
  return TRUE ;
}

/**************************************************************/
