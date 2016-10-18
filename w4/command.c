/*  File: command.c
 *  Author: Danielle et jean Thierry-Mieg (mieg@mrc-lmba.cam.ac.uk)
 *  Copyright (C) J Thierry-Mieg and R Durbin, 1995
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
 * 	Richard Durbin (MRC LMB, UK) rd@mrc-lmba.cam.ac.uk, and
 *	Jean Thierry-Mieg (CRBM du CNRS, France) mieg@frmop11.bitnet
 *
 * Description: Command language for acedb
 * Exported functions:
 * HISTORY:
 * Last edited: Dec 21 09:22 2009 (edgrif)
 * * Dec 16 11:28 1999 (edgrif): Added more admin cmds.
 * * Nov 23 09:45 1999 (edgrif): Insert new choixmenu values for admin cmds.
 * * Oct 15 12:38 1999 (fw): switch to second version of ACEIN API
 *              affected large portions of parse command code
 * * Dec  3 14:41 1998 (edgrif): Change calls to new interface to aceversion.
 * * Oct  8 11:44 1998 (fw): changed calls to helpPrintf to
 *                     helpOn() call. The helpsystem now uses
 *		       a registered funcion for whatever display
 *		       funcion we want to get at the help.
 *		       The default is obviously a textual dump.
 * * Aug 14 12:26 1998 (fw): AQL: context var for active keyset @active
 * * Aug  5 17:18 1998 (fw): added AQL functionality
 * * Nov 18 19:27 1997 (rd)
 *		- Read .zip files Assumes that xyz.zip file (only) 
 *		  contains a xyz.ace file; minor menu changes for "p" option
 *		- Used aceInPath() (see filsubs.c) instead of aceInWord() for filename
 *		  parses in commands "write", "Keyset read","parse","table reads" etc. 
 * * Jun 11 15:40 (rbrusk): #include "acedb.h" for name(), className() etc.
 * * Apr 23 16:45 1996 (srk)
 * Created: Sun Nov  5 17:41:29 1995 (mieg)
 * CVS info:   $Id: command.c,v 1.262 2009-12-21 10:50:08 edgrif Exp $
 *-------------------------------------------------------------------
 */

#define CHRONO

#include <wh/acedb.h>
#include <wh/aceio.h>
#include <wh/call.h>
#include <wh/bs.h>
#include <w4/command_.h>
#include <wh/spread_.h>		/* YUCK!!! HACK HACK HACK */
#include <wh/java.h>		/* for freejavaprotect() */
#include <wh/session.h>
#include <wh/a.h>
#include <wh/dna.h>
#include <wh/peptide.h>
#include <wh/dump.h>
#include <wh/lex.h>
#include <wh/pick.h>
#include <wh/biblio.h>
#include <wh/table.h>
#include <wh/parse.h>
#include <wh/aql.h>
#include <wh/bindex.h>
#include <wh/help.h>
#include <wh/alignment.h>
#include <wh/model.h>
#include <wh/longtext.h>				    /* for longGrep */
#include <wh/status.h>
#include <whooks/sysclass.h>
#include <whooks/systags.h>
#include <whooks/tags.h>
#include <whooks/classes.h>
#include <wh/dbpath.h>
#include <wh/basepad.h>
#include <wh/smap.h>
#include <wh/chrono.h>

/************************************************************/


static void taceTest (ACEIN command_in, ACEOUT res_out) ;
static BOOL aceCommandExists (AceCommand look);
static void showModel(ACEOUT fo, KEY key);
static BOOL doShowModel (ACEIN model_in, char *name, ACEOUT result_out) ;
static char getOnOff (ACEIN fi);
static void printUsage (ACEOUT fo, FREEOPT *qmenu, KEY option);
static void printCmd(ACEOUT fo, FREEOPT *qmenu, KEY option) ;
static int getDataVersion (void);


/* A small set of routines for supporting a "last modifed" timestamp for the */
/* database. If this proves too crude or we need access from elsewhere then  */
/* these could be moved into a separate module. For multi-threading this     */
/* would all require locking.                                                */
void setLastModified(void) ;
char *getLastModified(void) ;
void lastModified(char **new_time) ;


static void smapCmds(ACEIN command_in, ACEOUT result_out, int option) ;


/************************************************************/

static magic_t AceCommand_MAGIC = "AceCommand";


GifEntryFunc gifEntry = NULL ;	  /* global, set in giface code */


/******************************************************************
 ************************  public functions ***********************
 ******************************************************************/

void commandExecute (ACEIN fi, ACEOUT fo, 
		     unsigned int choix, unsigned int perms, KEYSET activeKs)
     /* warapper function for a command main-loop */
{ 
  AceCommand look;
  int maxChar = 0 ;
  int option ;

  look = aceCommandCreate(choix, perms) ;

  /* This is essential for at least Aquila, which needs the sub shells to    */
  /* $echo its logging info. to the screen/files, otherwise this logging info */
  /* is passed through to acedb !!!!!!!!!!                                   */
  if (!getenv("ACEDB_SUBSHELLS"))
    aceInSpecial (fi,"\n\t\\/@%") ;  /* forbid sub shells */
  else  
    aceInSpecial (fi,"\n\t\\/@%$") ; /* allow sub shells */


  if (aceInIsInteractive(fi) && !getenv("ACEDB_NO_BANNER"))
    aceOutPrint (fo, "\n\n// Type ? for a list of options\n\n") ;

  option = 0 ;
  look->ksNew = activeKs ? keySetCopy (activeKs) : keySetCreate() ;
  while (TRUE)
    {
      option = aceCommandExecute (look, fi, fo, option, maxChar) ;
    
      switch (option)
	{
	case 'q':
	  goto fin ;

	case 'B':
        case 'D':
	case 'm':
	case 'M':  /* automatic looping */	 
	  break ; 

	default:
	  option = 0 ;
	  break ;
	}
    }

 fin:
  aceCommandDestroy (look) ;

  return;
} /* commandExecute */

/*************************************************************/
/*************************************************************/
/* Ace-C: simply exe the command and return the result on the stack */

Stack commandStackExecute (AceCommand look, char *command)
{
  int option = 0, maxChar = 0 ;
  ACEIN fi = aceInCreateFromText (command, "", 0) ;
  ACEOUT fo ;
  Stack s = stackCreate (3000) ;

  stackTextOnly (s) ;
  fo = aceOutCreateToStack (s, 0) ;
  fi = aceInCreateFromText (command, "", 0) ;
  aceInSpecial (fi, "\n\t\\/%") ;  /* forbid sub shells and includescommand_ct*/
  
  option = 0 ;
  while (TRUE)
    {
      option = aceCommandExecute (look, fi, fo, option, maxChar) ;

      switch (option)
	{
	case 'q':
	  goto fin ;
	case 'B':
        case 'D':
	case 'm':
	case 'M':  /* automatic looping */	 
	  break ; 
	default:
	  option = 0 ;
	  break ;
	}
    }
 fin:

  aceInDestroy (fi) ; 
  aceOutDestroy (fo) ;
  
  return s ;
} /* commandStackExecute */

/*************************************************************/
/* Ace-C */
void aceCommandSwitchActiveSet (AceCommand look, KEYSET ks, KEYSET ks2)
{
  if (ks)
    {
      keySetDestroy (look->ksNew) ;
      look->ksNew = keySetCopy (ks) ;
    } 
  if (look->ksNew && ks2)
    {
      int i = keySetMax (look->ksNew) ;
      while (i--)
	keySet (ks2, i) = keySet (look->ksNew, i) ;
    }  
  return ;
} /* aceCommandSwitchActiveSet */

/*************************************************************/
/*************************************************************/

AceCommand aceCommandCreate (unsigned int choix, unsigned int perms)
{ 
  AceCommand look;

  look = (AceCommand)messalloc(sizeof(AceCommandStruct)) ;

  look->magic = &AceCommand_MAGIC ;
  look->choix = choix ;
  look->perms = perms ;
  look->aMenu = uAceCommandChoisirMenu(choix, perms) ;
  look->ksNew = keySetCreate () ;
  look->ksStack = stackCreate(5) ;

  look->user_interaction = TRUE ;
  look->showStatus = FALSE ;
  look->quiet = FALSE ;

  look->allowMismatches = FALSE ;
  look->spliced = TRUE ;
  look->cds_only = FALSE ;

  return look ;
} /* aceCommandCreate */


void aceCommandDestroy (AceCommand look)
{ 
  KEYSET ks ;

  if (!aceCommandExists(look))
    messcrash("aceCommandDestroy() - received non-magic look pointer");

  arrayDestroy (look->aMenu) ;
  keySetDestroy (look->ksNew) ;
  keySetDestroy (look->ksOld) ;
  keySetDestroy (look->kA) ;

  while (!stackAtEnd (look->ksStack))
    { 
      ks = pop (look->ksStack,KEYSET) ;
      if (keySetExists (ks))
	keySetDestroy (ks) ;
    }
  stackDestroy (look->ksStack) ;
  if (look->spread) 
    spreadDestroy (look->spread);

  if (look->showCond)
    condDestroy (look->showCond) ;

  if (look->dump_out)
    aceOutDestroy (look->dump_out);

  look->magic = 0 ;
  messfree (look) ;

  return;
} /* aceCommandDestroy */



/*************************************************************/
/*************************************************************/

#define CHECK    if (!look->ksNew || !keySetMax (look->ksNew)) \
                  { \
		    aceOutPrint (result_out, "// Active list is empty\n"); \
		    break; \
	          }

#define CHECK_WRITE if (look->noWrite) /* in readonly mode */ \
	             { \
		       aceOutPrint (result_out, "// Sorry, you do not have Write Access.\n") ; \
	               break ; \
	             } \
	            if (!sessionGainWriteAccess())	/* try to grab it */ \
	            { \
			/* may occur is somebody else grabbed it */ \
			aceOutPrint (result_out, "// Sorry, you cannot gain Write Access\n") ; \
			break ; \
	            }

#define UNDO  { keySetDestroy(look->ksOld) ; look->ksOld =  look->ksNew ; look->ksNew = 0 ; } /* save undo info */ 


/********************* MAIN LOOP *****************************/

KEY aceCommandExecute (AceCommand look, ACEIN command_in, ACEOUT result_out, int option, int maxChar)
     /* used also by aceserver, jadeserver */

     /* PARAMETERS:
      *  
      * if option == 0 we need to get a new command
      * else we already have an option and can execute it straight away
      */
{ 
  KEYSET ksTmp = 0 ;
  int nn = 0 , i, ii = 0, j ;
  KEY key, *kp, tag ; 
  char *cp = 0, *cq =0, c ;
  BOOL isLongGrep = FALSE;
  BOOL parseKeepGoing = FALSE ;
  FREEOPT *qmenu = arrp (look->aMenu, 0, FREEOPT) ;
  STORE_HANDLE tmp_handle = handleCreate();
  ACEIN tmp_command_in = NULL ;

  if (!aceCommandExists(look))
    messcrash("aceCommandExecute() - received non-magic look pointer");

  if (!option)
    {
      aceInOptPrompt (command_in, &option, qmenu);
      /* NOTE: that if option is still ZERO, no choice was made */
    }


  if (!option)
    {
      /* No choice made, or unknown command or ambiguous abbrev.             */
      if (aceInIsInteractive(command_in))
	aceOutPrint (result_out, "// Please type ? for a list of commands.\n") ;
    }
  else
    /* Valid command.                                                        */
    switch (option)
      {
	/**** end of input / quit command ****/
      case -1:
#ifdef KS_ADDED
	{ 
	  extern KEYSET ksAdded ; /* from dnasubs.c */
	  if (ksAdded)
	    { FILE *f = fopen ("seqused.ace","w") ;
	      keySetDump (f, 0, ksAdded) ;
	    }
	}
#endif
	if (aceInIsInteractive (command_in))
	  aceOutPrint (result_out, "\n\n") ;

	option = 'q';
	break;

      case 'H':   /* Chrono */
	cp = aceInWord (command_in) ;
	if (cp && !strcasecmp (cp, "start"))
	  chronoStart () ;
	else if (cp && !strcasecmp (cp, "stop"))
	  chronoStop () ;
	else if (cp && !strcasecmp (cp, "show"))
	  chronoReport () ;
	else
	  aceOutPrint (result_out, "// Usage: Chrono { start | stop | show}\n") ;
	break ;

	/**** list commands / print usage ****/
      case '?':
	{
	  char *arg;
	  KEY command_key_in_qmenu;

	  arg = aceInPos(command_in) ;

	  if (arg && strlen(arg) > 0)
	    {
	      /* User typed "? <some_cmd>", so try to print usage for it.    */

	      if (aceInKey(command_in, &command_key_in_qmenu, qmenu))
		{
		  /* Found unambiguous single command.                       */
		  aceOutPrint(result_out, "// ACEDB %scommand: %s\n",
			      ((look->choix & CHOIX_SERVER) ? "Server " : ""),
			      arg);
		  printCmd(result_out, qmenu, command_key_in_qmenu);
		}
	      else
		{
		  /* Look for any commands which partially match.            */
		  char *cq = messprintf("%s*", arg) ;
		  int cmds_index[250] ;			    /* Should be tied to command array size */
		  int i, j ;

		  i = 1 ;
		  j = 0 ;
		  while (qmenu[i].text != NULL)
		    {
		      if (pickMatch (qmenu[i].text, cq))
			{
			  cmds_index[j] = i ;
			  j++ ;
			}
		      i++ ;
		    }

		  if (j > 0)
		    {
		      aceOutPrint(result_out, "// ACEDB %slist of commands matching %s:\n",
				  ((look->choix & CHOIX_SERVER) ? "Server " : ""),
				  cq) ;

		      for (i = 0 ; i < j ; i++)
			{
			  aceOutPrint(result_out, "%s\n", qmenu[cmds_index[i]].text) ;
			}
		    }
		  else if (aceInIsInteractive(command_in))
		    {
		      aceOutPrint(result_out,
				  "// Cannot describe command %s.\n"
				  "// Please type ? for a list of commands.\n",
				  arg) ;
		    }
		}
	    }
	  else
	    {
	      /* User typed "?", so print description of all commands.       */
	      aceOutPrint(result_out,
			  "// ACEDB %slist of commands:\n",
			  ((look->choix & CHOIX_SERVER) ? "Server " : "")) ;

	      for (i = 1 ; i <= qmenu[0].key ; i++)
		aceOutPrint(result_out, "%s\n", qmenu[i].text) ;

	      aceOutPrint(result_out, "// ? command for options of just that command\n") ;
	    }
	} /* end-case '?' */
	break ;   
	



	/**** help ****/
      case 'h':
	if ((cp = aceInWord(command_in)))
	  helpOn (cp) ;
	else
	  { 
	    aceOutPrint
	      (result_out, 
	       "This program maintains an internal list of current objects. \n "
	       "You can\n"
	       "-Create and modify the list with the simple commands: \n"
	       "       Find, Follow, Grep, LongGrep, Biblio, Is, Remove, Undo \n"
	       "-Visualise in several format the content of the list with:\n"
	       "       List, Show, Count, Dna, Dump\n"
	       "-Perform complex queries and print relational tables with: \n"
	       "       Query, Table \n"
	       "-Modify the data with:\n"
	       "       New, Edit, EEdit, Parse, PParse, Kill\n"
	       "-See the schema and the dataload with\n"
	       "       Model, Classes, Status, Time_stamps\n"
	       "-Manipulate several lists a la polonaise with\n"
	       "       clear, spush, spop, swap, sand, sor, sxor, sminus\n"
	       " \n"
	       "Commands are not case sensitive and can be abbreviated.\n"
	       "They can be read from a command file with parameters (noted %%1 %%2...) with\n"
	       "   @file_name parm1 parm2 parm3...\n"
	       "if ACEDB_SUBSHELLS is set, lines starting with $ run as an interactive subshell.\n"
	       "Everything following // on a line is ignored (comment).\n"
	       "To escape any of the special characters use a backslash, eg \\@.\n\n"
	       
	       "To see the syntax of all commands type: ?\n"
	       "For further help type: help topic\n"
	       );
	  }
	break ;
	

      case 'u':   /* undo */
	if (look->ksOld)
	  { 
	    keySetDestroy (look->ksNew) ;
	    look->ksNew = look->ksOld ;
	    look->ksOld = 0 ;
	    aceOutPrint (result_out, "\n// Recovered %d objects\n", keySetMax(look->ksNew)) ;
	  }
	else
	  aceOutPrint (result_out, "// No more previous list available\n") ;
	break ;
	

      case '/':   /* ignore  comments */
	break ;
	

      case 'N':   /* Count, always part of stdout    */
	keySetDestroy(look->kA) ;
	if (look->ksNew) 
	  look->kA = keySetAlphaHeap(look->ksNew, keySetMax(look->ksNew)) ;
	aceOutPrint (result_out, "// Active KeySet holds %d object names\n", look->kA ? keySetMax(look->kA) : 0); 
	keySetDestroy(look->kA) ;
	break ;

      case '<':   /* Drop write access permanently */
	{
	  BOOL previousIsWriteAccess = isWriteAccess();
	  
	  sessionReleaseWriteAccess();
	  sessionForbidWriteAccess(); /* can never re-gain */


	/* this used to call sessionDropWriteAccess(),
	   which calls sessionDoClose(FALSE), which in turn
	   will just release write access and return
	   and then it would set the dropAccess flag to true
	   so we can never re-gain write access again, 
	   so logic is preserved */

	  if (isWriteAccess())
	    messerror ("// Write access couldn't be dropped\n");
	  else
	    {

	      if (!previousIsWriteAccess)
		aceOutPrint (result_out, "// Write access dropped permanently\n") ;
	      else
		aceOutPrint (result_out, "// Write access dropped permanently, recent modifs will not be saved\n") ;
	    }
	  break ;
	}

      case 'S':   /* Save */
	{
	  BOOL regain = FALSE ;

	  while ((cp = aceInWord(command_in)))
	    {
	      if (strcmp (cp, "-drop") == 0)
		regain = FALSE ;
	      else if (strcmp(cp, "-regain") == 0)
		regain = TRUE ;
	    }

	  CHECK_WRITE ;
	  if (!isWriteAccess())
	    {
	      option = 0 ;
	      aceOutPrint (result_out, "// Nothing to save\n") ;
	    }
	  else
	    {
	      sessionMark (TRUE, 0) ;
	      setLastModified() ;

	      if (regain)
		{
		  /* There is a small window where someone else could grab write access. */
		  if (!sessionGainWriteAccess())
		    aceOutPrint (result_out, "// Sorry, you cannot gain Write Access\n") ;
		}
	    }

	  break ;
	}

      case CMD_NOSAVE:   /* NoSave */
	{
	  BOOL save = FALSE ;

	  while ((cp = aceInWord(command_in)))
	    {
	      if (strcmp (cp, "-stop") == 0)
		save = FALSE ;
	      else if (strcmp(cp, "-start") == 0)
		save = TRUE ;
	    }

	  if (!writeAccessPossible())
	    {
	      option = 0 ;
	      aceOutPrint (result_out, "// You cannot get write access to save anyway.\n") ;
	    }
	  else
	    {
	      sessionSetSaveStatus(save) ;
	    }

	  break ;
	}

      case CMD_WRITEACCESS:   /* Writeaccess */
	{
	  BOOL gain = TRUE, save = TRUE ;
	  char *msg = NULL ;

	  while ((cp = aceInWord(command_in)))
	    {
	      if (strcmp(cp, "-gain") == 0)
		gain = TRUE ;
	      else if (strcmp (cp, "-drop") == 0)
		{
		  gain = FALSE ;
		  while ((cp = aceInWord(command_in)))
		    {
		      if (strcmp(cp, "save") == 0)
			save = TRUE ;
		      else if (strcmp(cp, "nosave") == 0)
			save = FALSE ;
		    }
		}
	    }

	  if (gain)
	    {
	      if (sessionGainWriteAccess())
		msg = "// You have write access\n" ;
	      else
		msg = "// Sorry, you cannot gain Write Access\n" ;
	    }
	  else
	    {
	      if (save)
		{
		  sessionMark (TRUE, 0) ;
		  setLastModified() ;
		}
	      else
		sessionReleaseWriteAccess() ;
	      msg = "// Write access released\n" ;
	    }
	  aceOutPrint (result_out, msg) ;
	  break ;
	}

      case 'V':   /* Test */
	taceTest (command_in, result_out) ;
	break ;
	
      case 'Z':   /* Status command */
	{
	  BOOL usage = FALSE ;
	  StatusType type = STATUS_EMPTY ;
	  BOOL onoff_set = FALSE ;

	  while ((cp = aceInWord(command_in)))
	    {
	      if (strcmp (cp, "on") == 0)
		{
		  onoff_set = TRUE ;
		  look->showStatus = TRUE ;
		}
	      else if (strcmp(cp, "off") == 0)
		{
		  onoff_set = TRUE ;
		  look->showStatus = FALSE ;
		}
	      else if (!(tStatusSetOption(cp, &type)))
		usage = TRUE ;
	    }

	  if (usage)
	    {
	      printUsage(result_out, qmenu, option) ;
	    }
	  else
	    {
	      if (type != STATUS_EMPTY)
		tStatusSelect(result_out, type) ;
	      else
		tStatus (result_out) ;

	      if (onoff_set && look->showStatus)
		aceOutPrint (result_out, "// Memory statistics on, some commands will dump statistics periodically\n") ; 
	      else if (onoff_set && !look->showStatus)
		aceOutPrint (result_out, "// Memory statistics off\n") ;
	    }
	}
	break ;
	
      case 'J':   /* Date */
	aceOutPrint (result_out, "\n// %s \n",timeShow(timeNow())) ;
	break ;

      case 'I':   /* Time Stamps creation */
	switch (getOnOff(command_in))
	  {
	  case 't':
	    isTimeStamps = TRUE ;		/* GLOBAL for using timeStamps */
	    aceOutPrint (result_out, "// Time-stamps now ON: Each modified data will be time stamped\n") ;
	    break ;

	  case 'f':
	    isTimeStamps = FALSE ;		/* GLOBAL for using timeStamps */
	    aceOutPrint (result_out, "// Time-stamps now OFF\n") ;
	    break ;

	  case '?':
	    printUsage(result_out, qmenu, option) ;
	    break ;

	  default:					    /* No cmdline flag. */
	    aceOutPrint (result_out, "// Time-stamps currently %s\n",
			 isTimeStamps ? "ON" : "OFF") ;
	  }
	break ;
	
	/******************* Stand Alone   *****************************/
	
      case 'c':  /* "Classes" */
	aceOutPrint (result_out,
		     "These are the known classes and "
		     "the number of objects in each class\n") ;

	for (i = 0; i < 256; i++)
	  {
	    if (pickClass2Word(i)
		&& !pickList[i].private
		&& lexMaxVisible(i)) /* 2nd call to lexMaxVisible() 
				      * is cheap */
	      aceOutPrint (result_out, "%35s %d\n", 
			   pickClass2Word(i), lexMaxVisible(i)) ;
	   
	  }
	break ;
	

      case 'z':   /* "Model" */
	cp = aceInWord(command_in) ;
	if (!cp) 
	  { 
	    aceOutPrint (result_out,
			 "// Usage Model [-jaq] Class : "
			         "Shows the model of given class,\n"
			 "//accepts wildcards\n") ;
	    break ;
	  }
	{ BOOL isJaq = FALSE ;
	  BOOL found = FALSE ;

	  if (strcmp (cp, "-jaq") == 0)
	    { isJaq = TRUE ;
	      cp = aceInWord(command_in) ;
	      if (!cp) 
		{ 
		  aceOutPrint (result_out, 
			       "// Usage Model [-jaq] Class : Shows the model of given class,\n"
			       "// accepts wildcards\n") ;
		  break ;
		}
	    }

	  i = 0 ;
	  key = 0;
	  while (lexNext(_VModel, &key))
	    {
	      if (pickMatch (name(key)+1, cp))
		{
		  if (isJaq)
		    showModelJaq (result_out, key) ;
		  else
		    showModel (result_out, key);
		  found = TRUE ;
		}
	    }
	
	  if (!isJaq)
	    for (key = 0 ; lexNext(_VClass, &key) ; )
	      if (pickMatch (name(key), cp) )
		{
		  KEY classe = key ;
		  unsigned char mask ;   
		  
		  pickIsA (&classe, &mask) ;
		  if (mask)
		    {
		      found = TRUE ;
		      aceOutPrint
			(result_out,
			 "// %s is a sub class of %s,"
			 " here is its filter\n",
			 name(key), pickClass2Word(classe)) ;
		      { 
			OBJ obj = bsCreate (key) ;
			if (obj) 
			  { 
			    if (bsGetData (obj, _Filter, _Text, &cp))
			      aceOutPrint (result_out, 
					   "// %s\n\n", cp);
			    bsDestroy (obj) ;
			  }
		      }
		    }
		}

	  if (!found)
	    aceOutPrint (result_out,
			 "// Sorry, I can t find a model for : %s\n",
			 cp);
	}
	break ;
	


      case 'R':      /* "Read-models" */
	if (look->choix & CHOIX_SERVER)
	  {
	    /* To remove this restriction will mean a number of changes to   */
	    /* code in session.c and bindex.c because of the user inter-     */
	    /* action required by these routines.                            */
	    aceOutPrint (result_out, "// Sorry, you must use tace or xace to Read-models\n") ;
	    break ;
	  }

	if (!isWriteAccess())
	  {
	    CHECK_WRITE;
	    sessionReleaseWriteAccess () ; /* don t get it just before read-mo */
	  }

	cp = aceInWord(command_in) ;
	if (cp)
	  {
	    if (strcmp(cp,"-index") == 0)
	      {
		/* calls readModels() ; which eventually calls bIndexInit() */
		bIndexInit(BINDEX_AFTER_READMODELS) ;
	      }
	    else
	      {
		aceOutPrint (result_out, "// Usage: Read-models [-index]\n") ;
		break ;
	      }
	  }
	else
	  readModels() ;
	setLastModified() ;
	aceOutPrint (result_out, "// New models read, save your work before quitting\n") ;
	break ;
	
      case 'F':   /* New Class format */
	{
	  char *class_name;
	  KEY class_key;
	  int class_index;
	  char *format;
	  Stack textStack;
	  BOOL isAddOK;
	  KEY added_key;

	  class_name = aceInWord(command_in) ;
	  if (!class_name) 
	    { 
	      aceOutPrint (result_out, "// Usage New Class Format: Construct a new name"
			   " according to C-language printf(format,n) \n") ;
	      aceOutPrint (result_out, "// Example: New Oligo xxx.\\%%d\n") ;
	      break ;
	    }

	  if (!lexword2key(class_name, &class_key, _VMainClasses))
	    { 
	      aceOutPrint (result_out, "//! Unkown class %s\n", class_name) ;
	      break ;
	    }

	  class_index = KEYKEY(class_key) ;
	  if (pickList[class_index].protected)
	    { 
	      aceOutPrint (result_out, "//! Protected class\n", class_name) ;
	      break ;
	    }
	  
	  format = aceInWord(command_in) ;
	  if (!format) 
	    {
	      aceOutPrint (result_out, "// Usage New Class Format: Construct a new name"
		       " according to C-language printf(format,n) \n") ;
	      aceOutPrint (result_out, "// Example: New Oligo xxx.\\%%d\n") ;
	      break ;
	    }

	  CHECK_WRITE ;

	  textStack = stackCreate(80) ;
	  pushText(textStack, format) ;
	
	  i = -2 ; /* important flag */
	  cp = stackText(textStack,0) ;
	  while (*cp && *cp != '%') cp++ ;
	  if (*cp++ == '%')
	    { 
	      while(*cp && isdigit((int)*cp)) cp++ ;  
	      if (*cp++ == 'd')
		{ 
		  i = 1 ;
		  while (*cp && *cp != '%') cp++ ;
		  if (!*cp)
		    goto okformat ;
		}
	      aceOutPrint (result_out, "// Only allowed format should contain zero or once %%[number]d \n") ;
	      stackDestroy (textStack);
	      break ;	/* end-case 'F' */
	    }
	okformat:
	  added_key = 0;
	  isAddOK = FALSE;
	  cp = stackText(textStack,0);
	  while (i < lexMax(class_index) + 3 && /* obvious format obvious problem */
		 !(isAddOK = lexaddkey(messprintf(cp,i), &added_key, class_index)))
	    if (i++ < 0) break ;  /* so i break at zero f format has no %d */

	  if (isAddOK)
	    {
	      aceOutPrint (result_out, "%s  \"%s\"\n", className(added_key), name(added_key)) ;
	      setLastModified() ;
	    }
	  else
	    aceOutPrint (result_out, "// Name %s already exists\n", cp) ;

	  stackDestroy (textStack);
	}
	break;	/* end-case 'F' new class format */
	
	/******************* Parsers      *****************************/

      case 'K':  /* "KeySet-Read" */
	{
	  ACEIN keyset_in = NULL;
	  char *arg, *assigned_text = 0;

	  if (!aceInCheck(command_in, "w"))
	    /* no other word on command line - read from stdin then */
	    {
	      if (aceInIsInteractive(command_in))
		{
		  if (look->choix & CHOIX_SERVER)
		    messcrash("KeySet-Read: server cannot open stdin");

		  keyset_in = aceInCreateFromStdin (TRUE, "", 0) ;
		}
	      else
		{
		  aceOutPrint
		    (result_out,
		     "// Sorry, I cannot read from terminal\n"
		     "//  please specify the keyset on the command line (after = sign).\n"
		     "//  or give a file name as command line argument\n");
		  printUsage (result_out, qmenu, option);
		}
	    }
	  else
	    /* there is an argument on the command line */
	    {
	      arg = aceInWord (command_in);
	      if (strcmp (arg, "=") == 0)
		/* direct assignment on command-line */
		/* we currently have a dilemma here - it would be nice
		 * to be able to submit tace-commands with aceInSpecial ';'
		 * so a whole list of commands can be submitted on one line.
		 * This means we can do 'find sequence ; show -a' even from
		 * xace -remote.
		 * This however makes it impossible for this command
		 * to separate multiple keys by the ';' as that was
		 * already seen as the end of the whole command-line. */
		{
		  aceInNext(command_in) ;
		  cp = aceInPos(command_in) ;
		  if (cp && strlen(cp) > 0) 
		    assigned_text = strnew(cp, tmp_handle);
		  else 
		    /* reset active list to empty */
		    assigned_text = strnew("   ", tmp_handle);

		  keyset_in = aceInCreateFromText (assigned_text, "", 0);
		  /* recognise \n and ; as line-breaker
		   * recognise \ as line-continuation 
		   * and parse // as comments 
		   */
		  aceInSpecial (keyset_in,"\n\\/;");
		}
	      else
		/* arg must be filename - read from file */
		{ 
		  aceInBack (command_in);
		  arg = aceInPath (command_in);

		  keyset_in = aceInCreateFromFile (arg, "r", "", 0);

		  if (!keyset_in)
		    aceOutPrint (result_out,
				 "// Sorry, I could not open file %s\n", arg) ;
		}
	    }

	  if (keyset_in)
	    {
	      UNDO ;
	      look->ksNew = keySetRead (keyset_in, result_out);
	      aceInDestroy(keyset_in);
	      
	      aceOutPrint (result_out, "// Recognised %d objects\n",
			   keySetMax(look->ksNew)) ;
	    }
	}
	break ;

	/* I do not KNOW HOW THIS CASE GETS CALLED...ITS BEYOND ME....       */
      case '_':  /* server parser */
	CHECK_WRITE ;
	UNDO ;
	look->ksNew = keySetCreate () ;

	parseAceCBuffer (aceInPos (command_in), 0, result_out, look->ksNew) ;
	setLastModified() ;

	break ;

	/* admin users can get files parsed on the server side, note that the  */
	/* client will prevent any normal parse/pparse requests coming through,*/
	/* they get translated into raw ace data being passed across.        */
	/* ACTUALLY, this won't work, we will have to do the "parse =" trick */
	/* for this as well, except that we need to allow for the xref flag  */
	/* which Jean does not....actually code here will be all right, its  */
	/* the client code that doesn't do it properly....                   */
	/* Note also that the final parse at the bottom of this will need to */
	/* call doParseAceIn with an alternative output, this is not correct */
	/* at the moment, we need to set a flag for server type comms, also  */
	/* all the prints should be either logged or sent to the alternative */
	/* output place, actually looks like we just use "result_out" to do  */
	/* this.                                                             */

      case CMD_REMOTEPPARSE:
      case 'P':
	parseKeepGoing = TRUE ;	 
	/* fall thru */

      case CMD_REMOTEPARSE:
      case 'p':
	{
	  char *assigned_text = NULL;
	  char *filename = NULL;
	  BOOL isAssign = FALSE;
	  ACEIN parse_io = NULL;
	  BOOL full_stats = FALSE ;

	  look->noXref = FALSE ;

	  /* Get command options. */
	  while (TRUE)
	    {
	      /* treat all -options in random order */
	      aceInNext(command_in) ;

	      /* "parse =" is inserted by saceclient and the following text is ace data
	       * gathered from stdin on the client side. */
	      if (aceInStep(command_in, '=')) 
		{ 
		  isAssign = TRUE;
		  break ;				    /* N.B. leaves loop... */
		}

	      if (aceInStep(command_in,'-'))
		{ 
		  cp = aceInWord(command_in) ;

		  if (cp && strcmp(cp, "NOXREF") == 0)
		    look->noXref = TRUE ;
		  else if (cp && strcmp(cp, "v") == 0)
		    full_stats = TRUE ;
		  else if (cp && strcmp(cp, "c") == 0)
		    { 
		      isAssign = TRUE;
		      break ;				    /* N.B. leaves loop... */
		    }
 
		  continue ;
		}

	      /* word after the -options is a filename */
	      if ((cp = aceInPath(command_in)))
		filename = strnew(cp, tmp_handle);
	      break ;

	    }


	  /* Set up text/file data stream. */
	  CHECK_WRITE ;
	  UNDO ;
	  look->ksNew = keySetCreate () ;

	  if (isAssign)
	    {
	      aceInNext(command_in) ; 
	      if ((cp = aceInPos(command_in)))
		assigned_text = strnew(cp, tmp_handle);
	      
	      /* Can only be from client/server....                          */
	      if (assigned_text && strlen(assigned_text) > 0)
		{
		  parse_io = aceInCreateFromText(assigned_text, "", 0);
		  aceInSpecial (parse_io, "\n\t\\;/") ; /* recognize ';' as separator, allow continuation lines  */
		}

	      if (aceInIsInteractive(command_in) && parse_io)
		aceOutPrint(result_out, "// Accepting data on command line\n") ;
	    }
	  else if (filename)
	    {
	      /* filename is full absolute pathname of .ace-file */
	      
	      if (filCheckName(filename, "", "r")) /* check to avoid warning by filopen */
		{
		  cq = filename + strlen(filename) - 2; 
		  if (cq > filename && strcmp(cq, ".Z") == 0) /* a compressed file */
		    parse_io = aceInCreateFromScriptPipe ("zcat", filename, "", 0) ;
		  else if (cq-- && cq > filename && strcmp(cq, ".gz") == 0)
		    parse_io = aceInCreateFromScriptPipe ("gunzip", messprintf(" -c %s", filename), "", 0) ;
		  else
		    parse_io = aceInCreateFromFile (filename, "r", "", 0);
		}

	      if (parse_io)
		{
		  messdump("// Parsing file %s %s\n",
			   look->noXref ? "NOXREF" : "",
			   filename) ;
		  aceOutPrint (result_out, "// Parsing file %s%s\n",
			       look->noXref ? "NOXREF " : "",
			       filename);
		}
	      else
		aceOutPrint (result_out, "// Sorry, I could not open file %s\n", filename) ;
	    }
	  else
	    { 
	      /* no command argument and no assigned parse-text
	       * we'll parse STDIN, this is for _non_ client/server only */

	      if (aceInIsInteractive (command_in))
		{
		  if (look->choix & CHOIX_SERVER) /* shouldn't happen */
		    messcrash("server can't open stdin");
		  
		  parse_io = aceInCreateFromStdin (TRUE, "", 0);
		  aceOutPrint (result_out, "// Type in your data in .ace format, then CTRL-D to finish\n");
		}
	      else
		{
		  aceOutPrint (result_out, "// Sorry, cannot parse from terminal input.\n");
		  if (strstr (messGetErrorProgram(), "server") != NULL)
		    aceOutPrint (result_out, "// Use '-ace_in' option to read data on the client side\n");
		}

	      if (parse_io)
		messdump("// Parsing stdin\n") ;
	    }

	  /* Finally we can actually do the parsing.                         */
	  if (parse_io) 
	    { 
	      /* xref stuff is a global in bssubs.c...purpose unknown to me. */
	      BOOL oldXref = XREF_DISABLED_FOR_UPDATE ;

	      XREF_DISABLED_FOR_UPDATE = look->noXref ;
	      doParseAceIn(parse_io, NULL, look->ksNew, parseKeepGoing, full_stats) ;
	      XREF_DISABLED_FOR_UPDATE = oldXref;

	      setLastModified() ;
	    }
	  
	  parseKeepGoing = FALSE ;
	}
	break ;			/* end-parse */
	
	/******************* Constructors  *****************************/
	
      case 'C':   /* Clear */
	UNDO ;
	look->ksNew = keySetCreate () ;
	break ;
	
      case 'j':  /* Dump */
	{
	  BOOL isCommandLineError = FALSE;
	  BOOL isSplit = FALSE ;
	  char dumpDir[DIR_BUFFER_SIZE] = "" ;

	  /* get full-path of default dir, so we can report back 
	   * to user, where the dump will go if no other dir given */
	  if (filCheckName(dumpDir, "", "rd"))
	    {
	      strcpy (dumpDir, cp = filGetName(dumpDir, "", "rd", 0));
	      messfree(cp);
	    }

	  while ((cp = aceInWord(command_in)))
	    {
	      if (strcmp (cp, "-s") == 0)
		isSplit = TRUE ;
	      else if (strcmp(cp, "-T") == 0)
		dumpTimeStamps = TRUE ;
	      else if (strcmp(cp, "-C") == 0)
		dumpComments = FALSE ;
	      else
		{ 
		  aceInBack (command_in);
		  cp = aceInPath (command_in);

		  /* if it is a directory continue */
		  if (filCheckName(cp, "", "rd"))
		    {
		      strcpy (dumpDir, cp); 

		      isCommandLineError = FALSE;
		    }
		  else
		    { 
		      aceOutPrint (result_out, 
				   "// Error: Could not open directory %s\n",
				   cp) ;
		      isCommandLineError = TRUE;
		    }
		  break;
		}
	    }

	  if (isCommandLineError)
	    printUsage (result_out, qmenu, option);
	  else
	    {
	      aceOutPrint(result_out, "// Dump directory : %s\n",
			  dumpDir);

	      /* make sure the directory ends with a '/' */
	      if (dumpDir[strlen(dumpDir)-1] != SUBDIR_DELIMITER)
		strcat (dumpDir, SUBDIR_DELIMITER_STR);
	      dumpAllNonInteractive (dumpDir, isSplit) ;
	    }
	}
	break; /* end-case 'j' "Dump" */

      case 'Q':  /* Dumpread */
	{
	  if (!(cp = aceInWord(command_in)))
	    printUsage(result_out, qmenu, option);
	  else
	    {
	      BOOL result = FALSE ;
	      ACEIN parse_in = NULL ;

	      parse_in = aceInCreateFromFile(cp, "r", NULL, 0) ; /* Gets destroyed by dump call. */
	      if (parse_in)
		{
		  char *dirname = NULL ;

		  dirname = filGetDirname(cp) ;
		  if (dirname == NULL)
		    dirname = "." ;

		  result = dumpDoReadAll(parse_in,  result_out, aceInIsInteractive(command_in),
					 filGetDirname(cp), filGetFilename(cp)) ;
		}
	    }
	}
	break; /* end-case 'Q' "Dumpread" */

      case 'n':   /* FIND */
	{
	  char *class_name;
	  KEY class_key;
	  Stack queryStack;

	  class_name = aceInWord(command_in) ;
	  if (!class_name) 
	    {
	      aceOutPrint
		(result_out, 
		 "// Usage Find Class [name]: Construct a new current list\n"
		 " // formed with all objects belonging to class \n"
		 " // Example: New Sequence a\\%%.b   // may give a7.b\n") ;
	      break ;
	    }

	  queryStack = stackCreate(80) ;
	  class_key = 0 ;
	  lexword2key (class_name, &class_key, _VClass) ;
	  pushText(queryStack, class_name) ;
	  aceInNext(command_in) ;
	  i = 0 ;  /* no name */

	  cp = aceInPos(command_in);
	  if (cp && strlen(cp) > 0)
	    { 
	      BOOL isProtected = FALSE ;
	      
	      if (*cp == '"') { isProtected = TRUE ; } /* remove flanking " if there */
	      cq = cp + strlen(cp) - 1 ;
	      while (cq > cp && (*cq == ' ' || *cq == '\t')) *cq-- = 0 ;
	      i = 1 ; /* a name */
	      cq = cp - 1 ;
	      while (*++cq)
		if (*cq == '?' || *cq == '*') i = 2 ; /* a regexp */
	      if (class_key && i == 1)
		{
		  stackClear (queryStack) ;
		  pushText (queryStack, isProtected ? freeunprotect (cp) : cp) ;
		}
	      else
		{
		  catText (queryStack, " IS ") ;
		  catText (queryStack, isProtected ? cp : freeunprotect(cp)) ; 
		}
	    }	   
	  
	  UNDO ;
	  
	  switch (i)
	    {
	    case 0: case 2:
	      look->ksNew = query(0, messprintf("FIND %s", stackText(queryStack, 0))) ;
	      break ;
	    case 1:  /* exact word */
	      look->ksNew = keySetCreate () ;
	      if (lexword2key (stackText(queryStack, 0), &key, class_key))
		keySet (look->ksNew, 0) = key ; 
	      break ;
	    }
	  aceOutPrint (result_out, "\n// Found %d objects in this class\n", keySetMax(look->ksNew)) ;

	  stackDestroy (queryStack);
	}
	break; /* end-case 'n' "find" */


      case CMD_NEIGHBOURS:				    /* "Neighbours" - Find neighbours */
	{
	  if (!(keySetMax(look->ksNew)))
	    {
	      aceOutPrint(result_out, "%s", "\n// Sorry, no active objects.\n") ;
	    }
	  else
	    {
	      KEYSET tmp ;

	      /* Preserve current keyset. */
	      tmp = keySetCopy(look->ksNew) ;

	      UNDO ;

	      look->ksNew = keySetNeighbours(tmp) ; /* will destroy tmp */
	    }

	  break ;
	}
	
      case 'x':   /* Performs a complex query */
	{
	  char *query_text;
	  if (!(cp = aceInWordCut(command_in, "",&c)))
	    break ;
	  query_text = strnew (cp, tmp_handle) ;
	  UNDO ;			/* back up ksNew into ksOld */
	  look->ksNew = query(look->ksOld, query_text) ;
	  aceOutPrint (result_out, "\n// Found %d objects\n", keySetMax(look->ksNew)) ;
	}
	break ;
	
      case 'G':
	isLongGrep = TRUE ;

	/* NOTE: fall-through */

      case 'g':   /* Match == Grep */
	{
	  char *grep_query;
	  UNDO ;
	  if (!(cp = aceInWord(command_in)))
	    {
	      printUsage (result_out, qmenu, option);
	      break ;
	    }
	  if (strcmp(cp, "-active") == 0)
	    { 
	      ksTmp = look->ksOld ; 
	      aceInNext(command_in) ;
	    }
	  else
	    { 
	      aceInBack(command_in) ;
	      ksTmp = 0 ;
	    }

	  if (!(cp = aceInWordCut(command_in,"",&c)))
	    break ;
	  grep_query = strnew (messprintf("*%s*",cp), tmp_handle);

	  aceOutPrint (result_out, "// I search for texts or object names matching %s\n", grep_query);

	  look->ksNew = queryGrep(ksTmp, grep_query);
	  ksTmp = 0 ;

	  if (isLongGrep)
	    { 
	      KEYSET ks1 = look->ksNew, ks2 = longGrep(grep_query);
	      look->ksNew = keySetOR(ks1, ks2) ;
	      keySetDestroy(ks1) ;
	      keySetDestroy(ks2) ; 
	      isLongGrep = FALSE ;
	    }
	  
	  aceOutPrint (result_out, "\n// Found %d objects\n", keySetMax(look->ksNew)) ;
	}
	break ;
	
      case 'T':			/* Table-Maker */
	{
	  char *tableSaveObjName = 0;
	  Stack defStack = 0;	/* read defs from text on command line */
	  KEY tableDefKey = 0;	/* read defs from obj in database */
	  char *tableDefFileName = 0; /* read defs from file */
	  SPREAD spread = 0;
	  BOOL isCommandLineError = FALSE;
	  char *tableParams = 0;
	  BOOL useActiveKeySet = FALSE;

	  if (look->dump_out)
	    aceOutDestroy (look->dump_out);
	  /* by default write results to the same stream as 
	   * the rest of the command interactions */
	  look->dump_out = aceOutCopy (result_out, 0);
	  
	  cp = 0 ;

	  look->minN = 0 ;
	  look->maxN = -1 ;
	  look->beginTable = TRUE ;
	  look->beauty = '\0';	/* default format - like 'a'(.ace-style), 
				 * but no format line */
	  look->showTableTitle = FALSE;
	  
	  aceInNext(command_in) ;
	  while (aceInStep(command_in,'-'))
	    { 
	      aceInNext(command_in) ;
	      cp = aceInWord(command_in) ;
	      if (!cp)
		break ;
	      
	      if (strcasecmp(cp, "active") == 0)
		/* use active keyset as column 1 */
		useActiveKeySet = TRUE;
	      else if (strcasecmp(cp, "title") == 0)
		/* output table-title as well as data when dumping */
		look->showTableTitle = TRUE;
	      else if (strcasecmp(cp, "j") == 0) /* java style */
		look->beauty = 'j' ;
	      else if (strcasecmp(cp, "k") == 0) /* fixed 'java' style for aceperl */
		look->beauty = 'k' ;
	      else if (strcasecmp(cp, "p") == 0 || /* perl style */
		       strcasecmp(cp, "perl") == 0)
		look->beauty = 'p' ;
	      else if (strcasecmp(cp, "h") == 0) /* human style */
		look->beauty = 'h' ;   
	      else if (strcasecmp(cp, "a") == 0) /* ace style */
		look->beauty = 'a' ;   
	      else if (strcmp(cp, "-x") == 0)  /* default xml, content only */
		look->beauty = 'x' ;
	      else if (strcmp(cp, "-xclass") == 0)  /* xml giving class as attribute */
		look->beauty = 'X' ;
	      else if (strcmp(cp, "-xall") == 0)  /* xml with class and value attributes and content */
		look->beauty = 'y' ;
	      else if (strcmp(cp, "-xnocontent") == 0)  /* xml with class and value attributes, no content */
		look->beauty = 'Y' ;
	      else if (strcasecmp(cp, "n") == 0) /* load def from _VTable object */
		{
		  cp = aceInWord(command_in); /* the name object to load defs from */

		  if (!cp || *cp == '-')
		    {
		      aceOutPrint (result_out, "// Error : missing obj-name for -n option\n");
		      printUsage (result_out, qmenu, option);
		      isCommandLineError = TRUE;
		      break ;
		    }

		  if (!lexword2key (cp, &tableDefKey, _VTable) ||
		      !iskey (tableDefKey))
		    { 
		      aceOutPrint (result_out, "// Sorry, I could not find table %s\n", cp);
		      isCommandLineError = TRUE;
		      break ;
		    }
		}
	      else if (strcasecmp(cp, "s") == 0) /* store in _VTable object */
		{ 
		  cp = aceInWord(command_in); /* name of the object to store defs in */

		  if (!cp || *cp == '-')
		    {
		      aceOutPrint (result_out, "// Error : missing obj-name for -s option\n");
		      printUsage (result_out, qmenu, option);
		      isCommandLineError = TRUE;
		      break ;
		    }
		    
		  tableSaveObjName = strnew(cp, tmp_handle);
		}
	      else if (strcmp(cp, "C") == 0)
		look->beauty = 'C';
	      else if (strcmp(cp, "c") == 0) /* max count */
		{ 
		  if (aceInInt (command_in,&i) && i >= 0)
		    look->maxN = i ;
		  else
		    {
		      aceOutPrint (result_out, "// Error : no number after -c option\n");
		      printUsage (result_out, qmenu, option);
		      isCommandLineError = TRUE;
		      break;
		    }
		}
	      else if (strcasecmp(cp, "b") == 0) /* start count */
		{ 
		  if (aceInInt (command_in,&i) && i >= 0)
		    look->minN = i ;
		  else
		    {
		      aceOutPrint (result_out, "// Error : no number after -b option\n");
		      printUsage (result_out, qmenu, option);
		      isCommandLineError = TRUE;
		      break;
		    }
		}
	      else if (strcasecmp(cp, "o") == 0) /* output results to file */
		{ 
		  cp = aceInPath (command_in); /* the filename */
		  
		  if (!cp)
		    {
		      aceOutPrint (result_out, "// Error : missing filename for -o option\n") ;
		      printUsage (result_out, qmenu, option);
		      isCommandLineError = TRUE;
		      break ;
		    }
		  else
		    {
		      ACEOUT dump_out;

		      dump_out = aceOutCreateToFile (cp, "w", 0);
		      if (!dump_out)
			{ 
			  aceOutPrint (result_out, "// Sorry, cannot write output to %s\n",
				       cp) ;
			  isCommandLineError = TRUE;
			  break ;
			}

		      /* replace default output stream with fileoutput */
		      aceOutDestroy (look->dump_out);
		      look->dump_out = dump_out;
		    }
		}
	      aceInNext(command_in) ;
	    }      

	  if (isCommandLineError) /* there was an error */
	    break;
	  
	  /****** get the definitions and its parameters *********/

	  if (aceInStep(command_in, '='))
	    { 
	      /* text following the = sign is read as a literal 
	       * table definition from the command line */
	      
	      aceInNext(command_in) ;
	      cp = aceInPos(command_in) ;

	      if (!cp || strlen(cp) == 0)
		{
		  aceOutPrint (result_out, "// Error : no definitions specified after = sign\n");
		  break;
		}
	      
	      /* create a stack containing the definition
	       * from the command line */
	      defStack = stackHandleCreate(100, tmp_handle) ;
	      pushText (defStack, cp);

	      /* command-line assign cannot specify parameters
	       * eventual params will be substituted with blank string */
	      tableParams = strnew("", tmp_handle);
	    }
	  else
	    /* read defs from object, or from file */
	    {
	      if (!tableDefKey)
		{
		  /* the next word has to be a file name then */
		  cp = aceInPath(command_in);
		  if (!cp)
		    {
		      /* This means we neither have an object name nor a file name */
		      aceOutPrint (result_out, "// Error, no table definition specified\n");
		      printUsage (result_out, qmenu, option);
		      break;
		    }
		  
		  tableDefFileName = strnew (cp, tmp_handle);
		}
	  
	      /* get parameters */
	      aceInNext(command_in) ;
	      cp = aceInPos(command_in) ;
	      tableParams = strnew (cp ? cp : "", tmp_handle) ;		  
	    }
	  
	  if (tableSaveObjName)
	    /* if saving the definition straight to an object,
	     * no parameter substitution should be performed */
	    tableParams = (char*)NULL;

	  /***************************************************************/
	  /* we have now established the source of the table definitions
	   * - read from either defStack/tableDefKey/tableDefFilename    */

	  /******** read the table definitions ************/

	  if (defStack)
	    {
	      spread = spreadCreateFromStack (defStack, tableParams);
	      if (!spread)
		{
		  aceOutPrint (result_out, "// Sorry, Bad table definition following = sign\n") ;
		  break ;
		}
	    }
	  else if (tableDefFileName)
	    {
	      spread = spreadCreateFromFile (tableDefFileName, "r", tableParams);
	      if (!spread)
		{
		  aceOutPrint (result_out, "// Sorry, could not read table definitions file %s\n",
			       tableDefFileName);
		  break;
		}
	    }
	  else if (tableDefKey)
	    {
	      spread = spreadCreateFromObj (tableDefKey, tableParams);
	      if (!spread)
		{
		  aceOutPrint (result_out, "// Sorry, could not read table definitions obj %s\n",
			       name(tableDefKey));
		  break;
		}
	    }
	  else
	    {
	      /* We shouldn't be able to get here ... */
	      aceOutPrint (result_out, "// Error, no table definition specified\n");
	      printUsage (result_out, qmenu, option);
	      break;
	    }

	  if (tableSaveObjName)		/* set by -s option to save */
	    {
	      KEY tableSaveKey;
	      
	      lexaddkey (tableSaveObjName, &tableSaveKey, _VTable) ;
		  
	      CHECK_WRITE ;
		  
	      spreadSaveDefInObj (spread, tableSaveKey) ;
	      
	      aceOutPrint (result_out, "\n// Table saved\n") ;
	      spreadDestroy (spread);
		  
	      setLastModified() ;

	      option = 'T';
	      
	      break ;
	    }

	  
	  /* now get the active Keyset (look->kA)
	   * over which we can execute the table-maker definition */
	  keySetDestroy (look->kA) ;

	  if (useActiveKeySet)	/* search active keyset (option -active) */
	    { 
	      spread->precompute = FALSE ; /* cannot store precomputed 
					    * table, if active keyset is 
					    * used for master column */
	      spread->isActiveKeySet = TRUE ;
	      keySetDestroy (ksTmp) ;
	      ksTmp = spreadFilterKeySet (spread, look->ksNew) ;
	      look->kA = keySetAlphaHeap(ksTmp, keySetMax(ksTmp)) ;
	      keySetDestroy (ksTmp) ;
	    }
	  else			/* search whole keyset as defined in colonne 1 */
	    { 
	      UNDO ;
	      
	      if (spreadGetPrecomputation(spread))
		/* we have the precomputed spread->values 
		 * we now also need the active keyset to
		 * be the first column of key-values in that table */
		look->kA = keySetCreateFromTable(spread->values, 0);
	      else
		look->kA = spreadGetMasterKeyset (spread) ;

	      /*
	       * the tace-active-keyset should now be the first
	       * object-typed colonne of values for the table-definitions
	       */
	      
	      if (!look->kA)
		{
		  aceOutPrint (result_out, "// This table has a problem, "
			   "please debug it in graphic mode\n") ;
		  look->kA = keySetCreate () ;
		}
	      look->ksNew = keySetCopy (look->kA) ;
	      keySetSort (look->ksNew) ;
	    }
	  
	  if (look->spread)
	    /* kill old spread-defs */
	    spreadDestroy(look->spread);

	  look->spread = spread ;

	  look->lastSpread = look->minN + 1 ;
	  if (look->maxN >= 0)
	    look->maxN += look->minN ;
	  look->cumulatedTableLength  = 0 ;
	  
	  
	  if (keySetMax(look->kA) == 0)
	    break;		/* no need to dump table (is empty) */

	  look->lastCommand = 'B' ;
	  /* fall thru to case encore to dump table */
	}
	
      case 'B':			/* Table (encore) : dump the table */
	{
	  int last = look->spread ? look->lastSpread : 0 ;

	  if (look->lastCommand != 'B')
	    break ;
	  
	  if (last &&		/* if we've fallen right through 
				 * here from option 'T', last == 1 */
	      look->spread)
	    { 
	      if (look->spread->precompute
		  && look->spread->precomputeAvailable 
		  && look->spread->values)
		/* table of values was already loaded from
		 * precomputed results, so we can just output that */
		{ 
		  if (look->beginTable)
		    {
		      aceOutPrint(result_out, "\n");
		      tableHeaderOut (result_out, 
				    look->spread->values, 
				    look->beauty) ;
		    }

	          look->cumulatedTableLength += 
		    tablePartOut (look->dump_out, 
				  &last, look->spread->values, '\t',
				  look->beauty) ;
		}
	      else
		/* we need to compute the results */
		{
		  spreadCalculateOverKeySet (look->spread, 
					     look->kA, &last) ;
		  
		  if (look->beginTable)
		    {
		      aceOutPrint(result_out, "\n");
		      tableHeaderOut (result_out, 
				    look->spread->values, 
				    look->beauty) ;
		    }
	          look->cumulatedTableLength += 
		    tableSliceOut(look->dump_out,
				  0, tableMax(look->spread->values),
				  look->spread->values,
				  '\t', look->beauty) ;
		}

	      look->beginTable = FALSE ;
	      look->showTableTitle = FALSE ;
	    } 

	  if (look->maxN >= 0 &&
	      look->cumulatedTableLength >= look->maxN)
	    {
	      /* if we have a maxmimum line-count set and
	       * the table-length has reached it, we stop dumping */
	      last = 0 ;
	      look->spread->precompute = FALSE ; /* cannot store ! */
	    }


	  /* finished with this table */
	  if (last == 0)
	    { 
	      tableFooterOut (result_out, look->spread->values, look->beauty) ;

              if (look->spread)
		spreadDestroy(look->spread);

	      aceOutPrint(result_out, "\n//# %d lines in this table\n",
			  look->cumulatedTableLength) ;

	      /* Close the file so the results get written out. */
	      if (look->dump_out)
		{
		  aceOutDestroy(look->dump_out);
		  look->dump_out = NULL ;
		}
	    }

	  look->lastSpread = last ;
	  if (last > 0)
	    option = 'B';	/* we can do another 'encore' */
	  else
	    option = 'T';
	}
	break ;
	
	/******************* Server        *****************************/
	
      case CMD_SHUTDOWN:				    /* shUtdown [now] */
      case CMD_WHO:					    /* who */
      case CMD_PASSWD:
      case CMD_USER:
      case CMD_GLOBAL:
      case CMD_DOMAIN:
      case CMD_SERVERLOG:
	break ;
      case CMD_VERSION:					    /* data release */
	aceOutPrint (result_out, 
		     "version %d = data_version, see wspec/server.wrm\n", 
		     getDataVersion ()) ;
	break ;
	
	/******************* Editors       *****************************/
	
      case 'E':  /* EEdit */
	CHECK ;
	parseKeepGoing = TRUE ;
      case 'e': /* edit */
	{
	  char *edit_text;
	  ACEIN edit_io;
	  Stack textStack;

	  CHECK ;

	  aceInNext(command_in) ;
	  cp = aceInPos(command_in) ;
	  if (!cp || !*cp)
	    { 
	      aceOutPrint (result_out, "// Usage Edit ace_command e.g. \n"
		       "// Edit Author Tom, adds author tom to the whole active set\n") ;
	      parseKeepGoing = FALSE ;
	      break ;
	    }
	  edit_text = strnew (cp, tmp_handle) ;

	  CHECK_WRITE ;

	  edit_io = aceInCreateFromText(edit_text, "", 0);
	  tag = 0 ;
	  if (aceInCard(edit_io) 
	      && (cp = aceInWord(edit_io)) 
	      && strcmp (cp, "-D") == 0
	      && (cp = aceInWord(edit_io)))
	    tag = str2tag (cp) ;

	  aceInDestroy (edit_io) ;

	  i = keySetMax (look->ksNew) ;
	  textStack = stackCreate (40*i) ;
	  kp = arrp (look->ksNew, 0, KEY) - 1 ;
	  while (kp++, i--)
	    if (pickType(*kp) == 'B')  /* don t edit array this way */
	      { 
		if (tag && !bIndexTag(*kp, tag)) /* no deletion needed */
		  continue ;
		catText (textStack, className (*kp)) ;
		catText (textStack, " ") ;
		catText (textStack, freeprotect(name (*kp))) ;
		catText (textStack, "\n") ;
		catText (textStack, edit_text) ;
		catText (textStack, "\n\n") ;
	      }
	  ksTmp = keySetReCreate (ksTmp) ; /* do not UNDO, keep same active set */

	  if (strlen(stackText (textStack,0)) > 0)
	    parseBuffer (stackText (textStack,0), ksTmp, parseKeepGoing) ;

	  setLastModified() ;

	  keySetDestroy (ksTmp) ;
	  stackDestroy (textStack) ;  /* it can be quite big */

	  j = keySetMax(look->ksNew) ;
	  aceOutPrint (result_out, "// I updated %d objects with command %s\n", j, edit_text);
	  parseKeepGoing = FALSE ;
	  sessionMark (FALSE, 0) ;
	  break ;
	} /* end-case 'e' edit */

      case 'k':   /* Kill objects */
	{
	  int num;

	  CHECK ;
	  CHECK_WRITE ;
	  
	  num = keySetMax (look->ksNew);

	  /* interactive inputs are given the chance to break here */
	  if (aceInIsInteractive(command_in)
	      && !messQuery
	      ("// Do you really want to destroy %s %d object%s", 
	       num == 1 ? "this" : "these", num, num == 1 ? "" : "s"))
	    break ;
	  keySetKill (look->ksNew) ;

	  setLastModified() ;

	  look->ksNew = keySetReCreate (look->ksNew) ; /* no possible undo */
	}
	break ;
	  
      case 'i':   /* IS : keeps names matching template */
	{
	  char *query_text;

	  CHECK ;
	  UNDO ;
	  if ((cp = aceInPos(command_in)))
	    { 
	      query_text = strnew (messprintf ("IS %s", cp), tmp_handle);
	      look->ksNew = query (look->ksOld, query_text) ;
	    }
	  else
	    aceOutPrint (result_out, 
			 "Usage IS  [ < <= > >= ~ ] template :"
			 " keeps names from current list matching template\n") ;
	}
	break ;
	
      case 'r':   /* Remove : names matching template */
	{
	  char *query_text;

	  CHECK ;
	  UNDO ;
	  if ((cp = aceInWord(command_in)))
	    { 
	      query_text = strnew (messprintf ("IS %s", cp), tmp_handle);

	      keySetDestroy (ksTmp) ;
	      ksTmp = query (look->ksOld, query_text);
	      look->ksNew = keySetMINUS (look->ksOld, ksTmp) ;
	      keySetDestroy (ksTmp) ;
	    }
	  else
	    aceOutPrint (result_out, "// Usage Remove  [ < <= > >= ~ ] template : remove names matching template\n") ;
	}
	break ;

      case 't':   /* Tag follow */
	CHECK ;
	UNDO ;
	tag = 0 ;
	if ((cp = aceInWord(command_in)))
	  { /* Autocomplete to a tag */
	    while (lexNext(0, &tag))
	      if(pickMatch(name(tag), cp))
		{ if (strcmp(cp, name(tag)))
		    aceOutPrint (result_out, "// Autocompleting %s to %s\n",
			      cp, name(tag)) ;
		  goto goodTag ;
		}
	    aceOutPrint (result_out, "// Usage Follow tag-name, change to list of objects pointed by tag \n") ;
	    aceOutPrint (result_out, "// Sorry, Tag %s unknown \n", cp) ;
	    aceOutPrint (result_out, "// type:  model class to see the model of the class\n\n") ;
	    break ;
	  }
	else
	  { aceOutPrint (result_out, "// Follow without tag-name, enters active keySets\n") ;
	  }
      goodTag:
	ksTmp = keySetReCreate (ksTmp) ;
	for (i = 0, j = 0 ; i < keySetMax(look->ksOld) ; i++)
	  if (class(keySet(look->ksOld,i)) == _VKeySet) 
	    { KEY k = keySet (look->ksOld, i) ;
	      int i1 = 0 ;
	      KEYSET ks = arrayGet (k, KEY, "k") ;
	      
	      if (ks)
		{
		  for (i1 = 0 ; i1 < keySetMax(ks) ; i1++)
		    keySet(ksTmp, j++) = keySet(ks, i1) ;
		}
	      keySetDestroy (ks) ;
	    }
	look->ksNew = tag ? query (look->ksOld, messprintf(">%s", name(tag))) : keySetCreate() ;
	if (j)
	  {
	    KEYSET ks = keySetOR (look->ksNew, ksTmp) ;
	    keySetDestroy (look->ksNew) ;
	    look->ksNew = ks ;
	    keySetSort (look->ksNew) ;
	    keySetCompress (look->ksNew) ;
	  }
	keySetDestroy (ksTmp) ;
	aceOutPrint (result_out, "\n// Found %d objects\n", keySetMax(look->ksNew)) ;
	break ;
	
      case 'b':			/* biblio */
	{
	  BOOL isAbstracts = FALSE;
	  CHECK ;
	  UNDO ;

	  if ((cp = aceInWord(command_in)))
	    { 
	      if (!strcmp(cp,"-abstracts"))
		isAbstracts = TRUE ;
	      else
		aceInBack(command_in) ;
	    }
	  look->kA = biblioFollow (look->ksOld) ;
	  look->ksNew = keySetCopy (look->kA) ;
	  keySetSort (look->ksNew) ; /* ! backwards */

	  biblioDump (result_out, look->kA, isAbstracts, 80) ;
	  aceOutPrint (result_out, "\n");
	}
	break ;

/********************    Dumpers   *******************/

      case 'l':			/* List : names matching template */
	{
	  BOOL isCommandLineError = FALSE;
	  char *template = 0;

	  CHECK ;		/* break if active keyset is empty */

	  if (look->dump_out)
	    aceOutDestroy (look->dump_out);

	  /* by default write to same output as all other interactions */
	  look->dump_out = aceOutCopy (result_out, 0);

	  look->minN = 0 ;
	  look->maxN = -1 ;
	  look->notListable = 0 ;
	  look->beauty = 'h' ;   

	  cp = aceInWord(command_in);
	  while(cp)
	    {
	      if (strncmp(cp, "-a", 2) == 0)  /* ace format required */
		look->beauty = 'a' ;
	      else if (strcmp(cp, "-p") == 0 || strcmp(cp, "-perl") == 0)  /* perl format required */
		look->beauty = 'p' ;
	      else if (strncmp(cp, "-j", 2) == 0)  /* java format required */
		look->beauty = 'j'  ;
	      else if (strncmp(cp, "-J", 2) == 0)  /* java format required */
		look->beauty = 'J' ;
	      else if (strncmp(cp, "-k", 2) == 0)  /* fixed 'java' format for aceperl */
		look->beauty = 'k'  ;
	      else if (strncmp(cp, "-K", 2) == 0)  /* fixed 'java' format for aceperl */
		look->beauty = 'K' ;
	      else if (strncmp(cp, "-h", 2) == 0)  /* human format explicitly required */
		look->beauty = 'H' ;
	      else if (strncmp(cp, "-C", 2) == 0)  /* Ace C binary format */
		look->beauty = 'C' ;
	      else if (strcmp(cp, "-x") == 0)  /* default xml, content only */
		look->beauty = 'x' ;
	      else if (strcmp(cp, "-xclass") == 0)  /* xml giving class as attribute */
		look->beauty = 'X' ;
	      else if (strcmp(cp, "-xall") == 0)  /* xml with class and value attributes and content */
		look->beauty = 'y' ;
	      else if (strcmp(cp, "-xnocontent") == 0)  /* xml with class and value attributes, no content */
		look->beauty = 'Y' ;
	      else if (strncmp(cp, "-b", 2) == 0)  /* minN required */
		{
		  if (aceInInt (command_in,&i) && i >= 0)
		    look->minN = i ;
		}
	      else if (strncmp(cp, "-c", 2) == 0)  /* maxN required */
		{ 
		  if (aceInInt (command_in,&i) && i >= 0)
		    look->maxN = i ;
		}
	      else if (strncmp(cp, "-f", 2) == 0)  /* -f filename */
		{
		  cp = aceInPath(command_in);  /* output filename */
		  if (!cp)
		    {
		      aceOutPrint (result_out, "// Error : missing file name for -f option\n") ;
		      printUsage (result_out, qmenu, option);
		      isCommandLineError = TRUE;
		      break ;
		    }
		  else
		    {
		      char *outfilename;
		      ACEOUT dump_out;

		      if (look->beauty == 'h')  /* change default */
			look->beauty = 'a' ;
		      if (strcmp(cp, "-") == 0)
			goto toutpret ;

		      outfilename = strnew(cp, tmp_handle);

		      if ((dump_out = aceOutCreateToFile (outfilename, "w", 0)))
			{
			  aceOutDestroy (look->dump_out); /* kill default output */
			  look->dump_out = dump_out; /* replace with this file output */
			}
		      else
			{
			  aceOutPrint (result_out, "// Error : can't write to file %s\n",
				       outfilename) ;
			  isCommandLineError = TRUE;
			  break ;
			}
		    }
		} 
	      else if (strncmp(cp, "-t", 2) == 0) /* -t [template] */
		{
		  if ((cp = aceInWord(command_in)))
		    template = strnew (cp, tmp_handle) ;
		} 
	      else			/* assume a tag */
		template = strnew (cp, tmp_handle) ;

	      aceInNext(command_in) ;
	      cp = aceInWord(command_in); 
	    }

	toutpret:
	  if (isCommandLineError) /* there was an error */
	    break;

	  /* We now have an output-handle look->dump_out
	   * to which the answer is written */

	  keySetDestroy(look->kA) ;
	  if (template)
	    { 
	      ksTmp = arrayCreate (keySetMax (look->ksNew), KEY) ;
	      for (i = 0, j = 0 ; i < keySetMax(look->ksNew) ; i++)
		if (pickMatch(name(keySet(look->ksNew,i)), template))
		  keySet(ksTmp, j++) = keySet(look->ksNew, i) ;
	      look->kA = keySetAlphaHeap(ksTmp, keySetMax(ksTmp)) ;
	      keySetDestroy (ksTmp) ;
	    }
	  else
	    look->kA = keySetAlphaHeap(look->ksNew, keySetMax(look->ksNew)) ;

	  look->nextObj = look->minN; 
	  if (look->maxN >= 0) look->maxN += look->minN ;
	  
	  if (look->beauty != 'C')
	    aceOutPrint (look->dump_out, "\nKeySet : Answer_%d\n",++nn) ;
 	  else
	    {
	    int n;
	    aceOutBinary( look->dump_out, "c", 1);
	    n = keySetMax(look->kA);
	    aceOutBinary( look->dump_out, (char *)&n, 4); 
	    }

	  look->lastCommand = 'm'; 
	} /* end-case 'l' */

	/* FALL THROUGH */

      case 'm':			/* List (encore) */
	if (look->lastCommand != 'm')
	  break ;
	/* stolen from keySetDump so can count char */
	{ 
	  int oldc = -1, newc, ii = 0 ;

	  i = look->nextObj ; kp  = arrp (look->kA,i,KEY) ;
	  if (i) oldc = class(*kp) ;
	  ii = aceOutByte(look->dump_out) + maxChar ;
	  for (;i < keySetMax(look->kA) ; i++, kp++) 
	    {
	      char *class_name, *class_value ;

	      if (maxChar && aceOutByte (look->dump_out) > ii)
		break;
	      if (look->maxN >=0 && i >= look->maxN)
		break ;
	      if (look->beauty != 'C' && !lexIsKeyVisible (*kp))
		{ look->notListable++ ; continue ; }
	      
	      switch (look->beauty)
		{

		case 'a':
		case 'p':
		  {
		  cp = messprintf("%s : %s\n", className(*kp), 
				  freeprotect(name(*kp))) ;
	          aceOutPrint (look->dump_out, "%s", cp);
		  }
		  break ;

		case 'C':
                  {
                    char buf0[2] = {0, '\n'} ;
                    char buf [3] ;

                    if (i == look->minN)
                      buf[0] = '>' ;
                    else
                      buf[0] = '.' ;
                    if (iskey (*kp) == 2)
                      buf[1] = 'K' ;
                    else
                      buf[1] = 'k' ;
                    buf[2] = class (*kp) ;
                    aceOutBinary ( look->dump_out, buf, 3) ;
                    cp = name (*kp) ;
                    aceOutBinary ( look->dump_out, cp, strlen(cp)) ;
                    aceOutBinary ( look->dump_out, buf0, 2) ;
                  }
                  break ;

		case 'j': case 'k':
		  {
		  cp = messprintf("?%s?%s?\n",className(*kp),
				  freejavaprotect(name(*kp))) ;
	          aceOutPrint (look->dump_out, "%s", cp);
		  }
                  break ;

		case 'J': case 'K':
		  {
		  newc = class (*kp) ;
		  if (oldc != newc)
		    { oldc = newc ;
		      aceOutPrint (look->dump_out, "?%s", className (*kp)) ;
		    }
		  else
		    aceOutPrint (look->dump_out, "#") ;
		  if (iskey(*kp)==2)
		    cp = messprintf("?%s?\n",
				    freejavaprotect(name(*kp))) ;
		  else
		    cp = messprintf("!%s?\n",
				    freejavaprotect(name(*kp))) ;
	          aceOutPrint (look->dump_out, "%s", cp);
		  }
                  break ;

		case 'x': case 'X': case 'y': case 'Y':	    /* XML formats. */
		  {
		  class_name = className(*kp) ;
		  class_value = name(*kp) ;
		  
		  switch(look->beauty)
		    {
		      case 'x':
			cp = messprintf("<%s>%s</%s>\n", class_name, class_value, class_name) ;
			break ;
		      case 'X':
			cp = messprintf("<%s class=\"%s\">%s</%s>\n",
					class_name, class_name, class_value, class_name) ;
			break ;
		      case 'y':
			cp = messprintf("<%s class=\"%s\" value=\"%s\">%s</%s>\n",
					class_name, class_name, class_value, class_value, class_name) ;
			break ;
		      case 'Y':
			cp = messprintf("<%s class=\"%s\" value=\"%s\" />\n",
					class_name, class_name, class_value) ;
			break ;
		    }
	          aceOutPrint (look->dump_out, "%s", cp);
		  }
		  break ;

	        default:
		  {
		  newc = class (*kp) ;
		  if (oldc != newc)
		    { 
		      oldc = newc ;
		      aceOutPrint (look->dump_out, "%s:\n", className (*kp)) ;
		    }
		  cp = messprintf (" %s\n", name (*kp)) ;
	          aceOutPrint (look->dump_out, "%s", cp);
		  }
		  break ;

		}
	    }  
	  
	  if (i < keySetMax(look->kA) && (look->maxN <0 || i < look->maxN)) 
	    { 
	      look->nextObj = i; /* start here on more */
	      option = 'm' ;
	    }
	  else
	    { 
	      option = 'l' ;

	      if (look->beauty != 'C')
		{
	        aceOutPrint (look->dump_out, "\n\n");
	        aceOutDestroy (look->dump_out);
  
	        if (look->notListable)
		  aceOutPrint (result_out, "// %d object listed, %d not listable\n", 
			       i - look->minN - look->notListable, look->notListable) ;
	        else
		  aceOutPrint (result_out, "// %d object listed\n", i - look->minN) ;
		}
	    }
	}
	break;	/* end-case 'm' list more */
	
      case 'w':			/* Write : now synonym to show -a -f */
	/* Horribleness...original code used aceInForceCard to stick the args*/
	/* for "show" into command_in as below:                              */
	/*    aceInForceCard (command_in, messprintf ("-a -f %s", cp)) ;     */
	/*                                    - this doesn't work now because*/
	/* aceInForceCard does not work like freeforcecard does. Hence this  */
	/* disgusting hack where we create a false command_in...sigh...      */
	cp =  aceInPos(command_in) ;
	tmp_command_in = aceInCreateFromText(messprintf ("-a -f %s", cp), "", 0) ;

	/* Must empty original command_in of words so OK for next call...    */
	while(cp)
	  cp = aceInWord(command_in) ;

	/* Substitute our hacked up command_in for the original.             */
	command_in = tmp_command_in ;
	cp = aceInCard(command_in);

	/* NOTE fall through setting "-a -f <rest of old line>" */
	   
      case 's':			/* Show : [-p | -j | -k | -a | -h ] [-f filename] [tag] */
	{
	  BOOL isCommandLineError = FALSE;

	  CHECK ;		/* break if active list is empty */

	  keySetDestroy(look->kA) ;
	  if (look->ksNew && keySetMax(look->ksNew))
	    look->kA = keySetAlphaHeap(look->ksNew, keySetMax(look->ksNew)) ;
	  if (!look->kA)
	    { 
	      aceOutPrint (result_out, "// Active list is empty\n") ;
	      break ;
	    }

	  if (look->dump_out)
	    aceOutDestroy (look->dump_out);
	  /* by default output table results to the same stream
	   * as the rest of the command interactions */
	  look->dump_out = aceOutCopy (result_out, 0);

	  look->minN = 0 ;
	  look->maxN = -1 ;
	  look->beauty = 'h' ;
	  look->dumpTimeStamps = FALSE ;
	  look->dumpComments = TRUE ;
	  if (look->showCond)
	    { 
	      condDestroy (look->showCond) ;
	      look->showCond = 0 ;
	    }
	  
	  cp = aceInWord(command_in);
	  while (cp) 
	    {
	      if (strcmp (cp, "-p") == 0
		  || strcmp (cp, "-perl") == 0)
		look->beauty = 'p' ;
	      else if (strcmp (cp, "-C") == 0)
		look->beauty = 'C' ;
	      else if (strcmp (cp, "-j") == 0)
		look->beauty = 'j' ;
	      else if (strcmp (cp, "-J") == 0)
		look->beauty = 'J' ;
	      else if (strcmp (cp, "-k") == 0)
		look->beauty = 'k' ;
	      else if (strcmp (cp, "-K") == 0)
		look->beauty = 'K' ;
	      else if (strcmp (cp, "-a") == 0)
		look->beauty = 'a' ;
	      else if (strcmp (cp, "-h") == 0)
		look->beauty = 'H' ;
	      else if (strcmp(cp, "-x") == 0)  /* default xml, content only */
		look->beauty = 'x' ;
	      else if (strcmp(cp, "-xclass") == 0)  /* xml giving class as attribute */
		look->beauty = 'X' ;
	      else if (strcmp(cp, "-xall") == 0)  /* xml with class and value attributes and content */
		look->beauty = 'y' ;
	      else if (strcmp(cp, "-xnocontent") == 0)  /* xml with class and value attributes, no content */
		look->beauty = 'Y' ;
	      else if (strcmp(cp, "-T") == 0)  /* time stamps */
		look->dumpTimeStamps = TRUE ;
	      else if (strcmp(cp, "-C") == 0)  /* comments */
		look->dumpComments = FALSE ; 
	      else if (strcmp(cp, "-b") == 0)  /* maxN required */
		{ 
		  if (aceInInt (command_in, &i) && i >= 0)
		    look->minN = i ;
		  else
		    { 
		      aceOutPrint (result_out, "// Error: integer required after -b\n") ;
		      isCommandLineError = TRUE;
		      break ;
		    }
		}
	      else if (strcmp(cp, "-c") == 0)  /* maxN required */
		{ 
		  if (aceInInt (command_in, &i) && i >= 0)
		    look->maxN = i ;
		  else
		    {
		      aceOutPrint (result_out, "// Error: integer required after -c\n") ;
		      isCommandLineError = TRUE;
		      break ;
		    }
		}
	      else if (strcmp(cp, "-t") == 0) /* -t [tag] */
		{
		  if ((cp = aceInWord(command_in)))
		    {
		      if (!condConstruct (cp, &look->showCond))
			{ 
			  aceOutPrint (result_out, "// Unknown tag -t %s \n", cp) ;
			  isCommandLineError = TRUE;
			  keySetMax(look->kA) = 0 ;
			}
		    }
		  else
		    { 
		      aceOutPrint (result_out, "// Error: tag name required after -t\n") ;
		      isCommandLineError = TRUE;
		      break ;
		    }
		} 
	      else if (strcmp(cp, "-f") == 0)  /* -f filename */
		{	
		  cq = aceInPath(command_in);
		  if (!cq) /* filename */
		    {
		      aceOutPrint (result_out, "// Error: option -f requires filename argument\n") ;
		      isCommandLineError = TRUE;
		      break ;
		    }
		  else
		    {
		      ACEOUT dump_out;

		      dump_out = aceOutCreateToFile (cq, "w", 0);
		      if (!dump_out)
			{ 
			  aceOutPrint (result_out, "// Error: cannot write to %s\n",
				       cq) ;
			  isCommandLineError = TRUE;
			  break ;
			}
		      
		      /* replace default output stream with fileoutput */
		      aceOutDestroy (look->dump_out);
		      look->dump_out = dump_out;
		    }
		}
	      else              /* assume a tag */
		{ 
		  if (cp[0] == '-')
		    {
		      aceOutPrint (result_out, "// Error: invalid option %s \n"
				   " // valid options are: -a -h -p -j -J -k -K -x -xclass -xall -xnocontent -C -b -c -t -f\n",cp);
		      isCommandLineError = TRUE;
		      break;
		    }

		  if (!condConstruct (cp, &look->showCond))
		    {
		      aceOutPrint (result_out, "// Error: invalid condition  %s \n", cp) ;
		      keySetMax(look->kA) = 0 ;
		      isCommandLineError = TRUE;
		    }
		}

	      cp = aceInWord(command_in);
	    }

	  if (isCommandLineError) /* there was an error in command line */
	    break ;

	  aceOutPrint (look->dump_out, "\n") ;

	  look->nextObj = look->minN ;
	  if (look->maxN >= 0) look->maxN += look->minN ;
	  look->lastCommand = 'M' ;
	} /* end-case 's' - show */

	/* NOTE: Fall-through to case m */

      case 'M':			/* Show (encore) */
	if (look->lastCommand != 'M')
	  break ;
	
	/* set globals */
	dumpTimeStamps = look->dumpTimeStamps ;	
	dumpComments = look->dumpComments ;

	ii = aceOutByte (look->dump_out)  + maxChar ;

	/* dump all object in the active keyset */
	for (j = 0 , i = look->nextObj ; i < keySetMax(look->kA) ; ++i) 
	  { 
	    if (maxChar && aceOutByte (look->dump_out) > ii)
	      break;
	    if (look->maxN >=0 && i >= look->maxN)
	      break ;
	    
	    if (look->showStatus && ! (j%1000))
	      {
		ACEOUT status_out = aceOutCreateToStderr (0);
		aceOutPrint (status_out, "// %d: %s\n",
			     i, name(keySet(look->kA,i))) ;
		tStatus (status_out) ;
		aceOutDestroy (status_out);
	      }

	    dumpKeyBeautifully (look->dump_out, 
				keySet(look->kA,i), 
				look->beauty, 
				look->showCond) ;
	  }

	/* reset globals */
	dumpTimeStamps = FALSE ;
	dumpComments = TRUE ;

	if (i < keySetMax(look->kA) && (look->maxN <0 || i < look->maxN)) 
	  {
	    look->nextObj = i; 

	    option = 'M' ;
	  }
	else
	  {
	    int num_dumped = i - look->minN;

	    aceOutPrint (result_out,
			 "// %d object%s dumped\n", 
			 num_dumped,
			 num_dumped == 1 ? "" : "s") ;

	    aceOutDestroy (look->dump_out);

	    option = 's' ;
	  }
	break;	/* end-case 'M' more(show) */
	


      case 'd':			/* DNA : dump to screen or (-f) filename */
      case 'v':			/* Peptide : dump to screen or (-f) filename */
	{
	  char *out_file ;
	  CHECK ;

	  keySetDestroy(look->kA) ;
	  if (look->ksNew && keySetMax(look->ksNew))
	    look->kA = keySetAlphaHeap(look->ksNew, keySetMax(look->ksNew)) ;
	  if (!look->kA)
	    { 
	      aceOutPrint (result_out, "// Active list is empty\n") ;
	      break ;
	    }

	  look->nextObj = 0;
	  look->beauty = 'a' ;
	  look->allowMismatches = FALSE ;
	  look->spliced = TRUE, look->cds_only = FALSE ;
	  look->minN = look->maxN = 0 ;

	  /* User is allowed to put "-f filename" or just "filename", sadly.... */
	  out_file = NULL ;
	  while ((cp = aceInWord(command_in)))
	    {
	      if (strcmp (cp, "-p") == 0
		  || strcmp (cp, "-perl") == 0)
		look->beauty = 'p' ;
	      else if (!strcmp (cp, "-C"))
		look->beauty = 'C' ;
	      else if (!strcmp (cp, "-u"))
		look->beauty = 'u' ;
	      else if (!strcmp (cp, "-o"))
		look->beauty = 'o' ;
	      else if (strcmp (cp, "-mismatch") == 0)
		look->allowMismatches = TRUE ;
	      else if (strcmp (cp, "-unspliced") == 0)
		look->spliced = FALSE, look->cds_only = FALSE ;
	      else if (strcmp (cp, "-spliced") == 0)
		look->spliced = TRUE, look->cds_only = FALSE ;
	      else if (strcmp (cp, "-cds") == 0)
		look->spliced = TRUE, look->cds_only = TRUE ;
	      else if (!strcmp (cp, "-x1"))
		{
		  if (!aceInInt (command_in, &look->minN))
		    {
		      if ((cp = aceInWord(command_in)))
			{
			  if (!strcasecmp (cp, "begin"))
			    look->minN = 1 ;
			  else if (!strcasecmp (cp, "end"))
			    look->minN = -1 ;
			  else
			    {
			      aceOutPrint (result_out, "-x1 should be \"begin\" OR \"end\" OR a  positive number") ;
			      look->minN = -2 ;
			      break ;
			    }
			}
		    }
		  else
		    {
		      if (look->minN <= 0)
			{
			  aceOutPrint (result_out, "-x2 should be \"begin\" OR \"end\" OR a  positive number") ;
			  look->minN = -2 ;
			  break ;
			}
		    }
		}
	      else if (!strcmp (cp, "-x2"))
		{
		  if (!aceInInt (command_in, &look->maxN))
		    {
		      if ((cp = aceInWord(command_in)))
			{
			  if (!strcasecmp (cp, "begin"))
			    look->maxN = 1 ;
			  else if (!strcasecmp (cp, "end"))
			    look->maxN = -1 ;
			  else
			    {
			      aceOutPrint (result_out, "-x2 should be begin | end | positive number") ;
			      look->maxN = -2 ;
			      break ;
			    }
			}
		    }
		  else
		    {
		      if (look->maxN <= 0)
			{
			  aceOutPrint (result_out, "-x2 should be begin | end | positive number") ;
			  look->maxN = -2 ;
			  break ;
			}
		    }
		}
	      else if (strcmp(cp, "-f") == 0)
		out_file = strnew(aceInPath(command_in), 0) ;
	      else if (cp)
		{
		  aceInBack(command_in) ;
		  out_file = strnew(aceInPath(command_in), 0) ;
		}
	    }

	  if (look->minN == -2 || look->maxN == -2)
	    break ;
	  if (look->minN && look->minN == look->maxN)
	    {
	      aceOutPrint (result_out, "-x1 -x2 are equal, i cannot determine orientation") ;
	      break ;
	    }
	  if ((look->minN && !look->maxN) || (!look->minN && look->maxN))
	    {
	      aceOutPrint (result_out, "-x1 -x2 must be both specified or absent");
	      break ;
	    }

	  /* by default write results to the same stream as the rest of the command interactions */
	  if (look->dump_out)
	    aceOutDestroy (look->dump_out);
	  look->dump_out = aceOutCopy (result_out, 0);

	  if (out_file)
	    {
	      /* replace default output stream with fileoutput */
	      ACEOUT dump_out;

	      dump_out = aceOutCreateToFile(out_file, "w", 0);
	      messfree(out_file) ;

	      if (!dump_out)
		{
		  aceOutPrint (result_out, "// Error: cannot write to %s\n", cp) ;
		  break ;
		}

	      aceOutDestroy (look->dump_out);
	      look->dump_out = dump_out;
	    }

	  look->fastaCommand = option ;
	  look->lastCommand = 'D' ;
	}

	/****** Fall thru to case d ******/
	
      case 'D':
	if (look->lastCommand != 'D')
	  break ;
	
	ii = aceOutByte (look->dump_out)  + maxChar ;
	for (i = look->nextObj ; i < keySetMax(look->kA) ; ++i) 
	  {
	    if (maxChar && aceOutByte (look->dump_out) > ii)
	      break;
	    
	    key = keySet (look->kA, i) ;
	  
	    if (look->beauty == 'p')
	      {
		if (look->fastaCommand == 'd')
		  { 
		    Array dna = NULL ;
		    DNAActionOnErrorType error_action ;

		    if (look->user_interaction)
		      error_action = DNA_QUERY ;
		    else
		      error_action = DNA_FAIL ;

		    dna = dnaGetAction(key, error_action) ;

		    if (!dna)
		      continue ;

		    dnaDecodeArray(dna) ;

		    array(dna, arrayMax(dna), char) = 0 ;   /* vital if strlen() is to terminate... */
		    if (!strlen(arrp(dna, 0, char)))
		      {
			arrayDestroy (dna) ;
			continue ;
		      }
		    aceOutPrint (look->dump_out, "{ty=>dna,va=>%s,DNA=>", name(key)) ;
		    aceOutPrint (look->dump_out, arrp(dna, 0, char)) ;
		    aceOutPrint (look->dump_out, "}\n") ;
		    arrayDestroy (dna) ;
		  }
		else
		  { 
		    Array pep = 0 ;
		    if (!(pep = peptideGet (key)))
		      continue ;
		    pepDecodeArray (pep) ;
		    array(pep, arrayMax(pep), char) = 0 ; /* safeguard */
		    if (!strlen(arrp(pep, 0, char)))
		      {
			arrayDestroy (pep) ;
			continue ;
		      }
		    aceOutPrint (look->dump_out, "{ty=>pep,va=>%s,PEPTIDE=>", name(key)) ;
		    aceOutPrint (look->dump_out, arrp(pep, 0, char)) ;
		    aceOutPrint (look->dump_out, "}\n") ;
		    arrayDestroy (pep) ;
		  }
	      }
	    else if (look->fastaCommand == 'd')
	      {
		DNAActionOnErrorType error_action ;

		/* If user_interaction is blocked then just fail for dna otherwise depends on settings. */
		if (look->user_interaction)
		  {
		    if (look->allowMismatches)
		      error_action = DNA_CONTINUE ;
		    else
		      error_action = DNA_QUERY ;
		  }
		else
		  {
		    error_action = DNA_FAIL ;
		  }

		dnaZoneDumpFastAKey(look->dump_out, key, error_action,
				    look->cds_only, look->spliced, look->minN, look->maxN,
				    look->beauty) ;

		if (!look->quiet)
		  aceOutPrint(look->dump_out, "\n") ;
	      }
	    else
	      {
		pepDumpFastAKey(look->dump_out, key, look->beauty) ;
	      }
	  }

	if (i < keySetMax(look->kA))
	  { 
	    option = 'D' ;
	    look->nextObj = i; 
	  }
	else
	  { 
	    /* finished */
	    option = look->fastaCommand ;
	    look->fastaCommand = 0 ;

	    aceOutDestroy (look->dump_out);
	    
	    if (!look->quiet)
	      aceOutPrint (result_out, "// %d object dumped\n", keySetMax (look->kA));
	  }

	break ;

      case 'a':			/* Array class:name - 
				 * formatted acedump of A class objs */
	CHECK ;
	/*
	   if (lexClassKey (aceInWord(fi), &key)
	   && (fmt = aceInWord(fi))
	   && (a = uArrayGet(key, sizeOfFormattedArray(fmt), fmt)))
	   { dumpFormattedArray (.. should be freeOut...myFile, ...myStack, a, fmt) ;
	   arrayDestroy (a) ;
	   }
	   */
	break ;
	
      case 'L':			/* Alignment : dump alignments in fancy format */
	{
	  BOOL isCommandLineError = FALSE;
	  char *outfilename;
	  CHECK ;

	  if (look->dump_out)
	    aceOutDestroy (look->dump_out);
	  look->dump_out = aceOutCopy (result_out, 0);

	  outfilename = aceInPath(command_in) ;
	  if (outfilename)
	    {
	      ACEOUT dump_out;

	      dump_out = aceOutCreateToFile (outfilename, "w", 0);
	      if (dump_out)
		{
		  aceOutDestroy (look->dump_out);
		  look->dump_out = dump_out;
		}
	      else
		{
		  aceOutPrint (result_out,
			       "// Error : cannot write to %s\n", 
			       outfilename);
		  isCommandLineError = TRUE;
		}
	    }

	  if (!isCommandLineError)
	    {
	      keySetDestroy(look->kA) ;
	      look->kA = keySetAlphaHeap(look->ksNew, keySetMax(look->ksNew));
	      alignDumpKeySet (look->dump_out, look->kA);
	    }

	  aceOutDestroy (look->dump_out);
	}
	break ;

      case '-':			/* depad an assembly */
	{
	  char *arg = 0;

	  cp = aceInWord(command_in) ;
	  if (cp)
	    arg = strnew (cp, tmp_handle);

	  CHECK_WRITE ;

	  depad (arg) ;

	  setLastModified() ;
	}
	break ;

      case '*':			/* pad an assembly */
	{
	  char *arg = 0;
	  
	  cp = aceInWord(command_in) ;
	  if (cp)
	    arg = strnew (cp, tmp_handle);

	  CHECK_WRITE ;

	  pad (arg) ;

	  setLastModified() ;
	}
	break ;

      case CMD_WSPEC:			/* export wspec */
        aceOutPrint (result_out, "\n// wspec directory exported\n") ;

	break ;
      case 401:			/* export layout */
	{
	  char *filename = dbPathStrictFilName("wspec", "layout","wrm","r", 0);
	  ACEIN layout_io = NULL;
	  
	  if (filename)
	    {
	      layout_io = aceInCreateFromFile(filename, "r", "", 0);
	      messfree(filename);
	    }

	  if (layout_io)
	    { 
	      while (aceInCard(layout_io))
		aceOutPrint (result_out, "%s\n", aceInPos(layout_io)) ;
	      aceOutPrint (result_out, "\n// end of layout file\n") ;
	      aceInDestroy(layout_io);
	    }
	  else
	    aceOutPrint (result_out, "\n// Sorry, no file wspec/layout.wrm available \n") ;
	}
	break ;

      case '#':			/* GIF : entry point to drawing submenu */
	/* If the command set was initialised to contain this item then we   */
	/* MUST have the code to do it.                                      */
	if (!gifEntry)
	  {
	    messcrash("internal error: code is trying to run the gif command sub-menu,\n"
		      "// but the sub-menu callback routine has not been set.") ;
	  }
	else
	  { 
	    ACEIN gifcommand_in = NULL ;

	    /* commands on one line separated by ';' */
	    if (aceInCheck (command_in,"w"))
	      { 
		char *gif_command_text = NULL ;
		char *cp = NULL ;

		gif_command_text = strnew(aceInPos(command_in), tmp_handle);
		cp = gif_command_text;

		/* Nasty hack: in order to be able to get semicolons in the
		   gifcommand, they have to be escaped with \.
		   Strip off the \ here so that they are interpreted by
		   gifcommand  processing. */
		while (*cp)
		  {
		    if ((*cp == '\\') && (*(cp+1) == ';'))
		      *cp = ' ';
		    cp++;
		  }

 	        gifcommand_in = aceInCreateFromText (gif_command_text, "", 0) ;
		aceInSpecial (gifcommand_in, "\n\\/%;") ; /* recognize ';' as line breaker */
	      }
	    else if (aceInIsInteractive(command_in))
	      { 
		/* user entered "gif" so set up gif sub menu... */
		if (look->choix & CHOIX_SERVER) /* shouldn't happen */
		  messcrash("server cannot open stdin");

		gifcommand_in = aceInCreateFromStdin (TRUE, "", 0) ;  
	      }  
	    else 
	      { 
		aceOutPrint(result_out, "// Please provide the full gif command on a single line\n") ;

 	        gifcommand_in = aceInCreateFromText (" ? ", "", 0) ;
	      }

	    (*gifEntry)(look->ksNew, gifcommand_in, result_out, look->quiet) ;

	    aceInDestroy (gifcommand_in) ;
	  }
	break ;

      case CMD_LASTMODIFIED:	 /* Return DB last modified time. */
	aceOutPrint(result_out, "%s\n", getLastModified()) ;
	break ;

      case CMD_QUIET:					    /* Set quiet mode for commands a la Unix. */
	while ((cp = aceInWord(command_in)))
	  {
	    if (strcmp (cp, "-on") == 0)
	      look->quiet = TRUE ;
	    else if (strcmp(cp, "-off") == 0)
	      look->quiet = FALSE ;
	  }
	break ;

      case CMD_INTERACTION:				    /* Set user interaction for commands. */
	while ((cp = aceInWord(command_in)))
	  {
	    if (strcmp (cp, "-on") == 0)
	      look->user_interaction = TRUE ;
	    else if (strcmp(cp, "-off") == 0)
	      look->user_interaction = FALSE ;
	  }
	break ;

      case CMD_MSGLOG:					    /* Set display/logging for information/error msgs. */
	{
	  while ((cp = aceInWord(command_in)))
	    {
	      BOOL format, status, display ;

	      format = FALSE ;
	      if (strcmp(cp, "-msg") == 0)
		{
		  format = TRUE ;
		  display = TRUE ;
		}
	      else if (strcmp(cp, "-log") == 0)
		{
		  format = TRUE ;
		  display = FALSE ;
		}

	      if (format && (cp = aceInWord(command_in)))
		{
		  format = FALSE ;

		  if ((strcmp (cp, "on") == 0))
		    {
		      format = TRUE ;
		      status = TRUE ;
		    }
		  else if ((strcmp (cp, "off") == 0))
		    {
		      format = TRUE ;
		      status = FALSE ;
		    }
		}

	      if (format)
		{
		  if (display)
		    messSetShowMsgs(status) ;
		  else
		    messSetLogMsgs(status) ;
		}
	    }

	  break ;
	}

      case CMD_IDSUBMENU:      /* ID : entry point to id submenu */
	{ 
	  extern void idControl (KEYSET ks, ACEIN fi, ACEOUT fo) ;
	  idControl (look->ksNew, command_in, result_out);
	}
	break ;

      case CMD_NEWLOG:					    /* Start a new log or reopen current */
	{						    /* log. */
	  BOOL reopen = FALSE ;
	  char *logname = NULL ;

	  cp = aceInWord(command_in);
	  if (cp)
	    {
	      if (strncmp(cp, "-f", 2) == 0)		    /* -f log_name */
		{
		  if ((cp = aceInWord(command_in)))
		    logname = cp ;
		  else
		    {
		      aceOutPrint (result_out, "// Error: option -f requires filename argument\n") ;
		      printUsage(result_out, qmenu, option) ;
		      break ;
		    }
		}
	      else if (strcmp(cp, "-reopen") == 0)
		reopen = TRUE ;
	      else
		{
		  aceOutPrint (result_out, "// Error: unknown option.\n") ;
		  printUsage(result_out, qmenu, option) ;
		  break ;
		}


	    }
	  if (reopen)
	    {
	      if (!sessionReopenLogFile())
		messerror("Could not reopen existing log file.") ;
	    }
	  else
	    {
	      if (!sessionNewLogFile(logname))
		messerror("Could not start a new log file.") ;
	    }
	}
	break ;
	
	/******************* Stack operations  *****************************/
	
      case '1':			/* stack push */
	look->kA = keySetCopy(look->ksNew) ;
	aceOutPrint (result_out, "// Pushed the active set on stack\n") ; 
	goto finStack ;
	
      case '2': case '3': case '4': case '5': case '6': case '7': case '8':
	if (!look->nns)
	  { 
	    aceOutPrint (result_out, "// Sorry, the stack is empty\n") ;
	    break ;
	  }
	    switch (option)
	      {
	      case '2':  /* stack pop */
		UNDO ;
		look->ksNew = pop(look->ksStack,KEYSET) ; look->nns-- ;
		if (look->nns) 
                  { look->kA = pop(look->ksStack,KEYSET) ; look->nns-- ;
                  }
		else
		  look->kA = 0 ;
		break ;

	      case '3':  /* stack swap */
		look->kA = look->ksNew ;
		look->ksNew = pop(look->ksStack,KEYSET) ; look->nns-- ;
		break ;

	      case '4': case '5': case '6': case '7':
		ksTmp = pop (look->ksStack,KEYSET) ; look->nns-- ;
		switch (option)
		  { 
		  case '4':  /* stack AND */
		    look->kA = keySetAND (ksTmp, look->ksNew) ;  
		    break ;
		  case '5':  /* stack OR */
		    look->kA = keySetOR (ksTmp, look->ksNew) ;  
		    break ;
		  case '6':  /* stack XOR */
		    look->kA = keySetXOR (ksTmp, look->ksNew) ;  
		    break ;
		  case '7':  /* stack MINUS */
		    look->kA = keySetMINUS (ksTmp, look->ksNew) ;  
		    break ;
		  }
		keySetDestroy (ksTmp) ; 
		break ;

              case '8': /* stack pick */
		if (!aceInInt (command_in, &i)){
		  aceOutPrint (result_out,"// Error: missing integer for spick\n") ;
		  break;
		}
		if (i < 0 ) { 
		  aceOutPrint (result_out, "// Error: spick argument must be >= 0\n") ;
		  break ;
		}
		if (i >= look->nns ) { 
		  aceOutPrint (result_out, "// Error: spick argument greater than size of stack\n") ;
		  break ;
		}

		rot(look->ksStack,KEYSET,i);
		look->ksNew = pop(look->ksStack,KEYSET) ; 
		look->nns-- ;
		
		if (look->nns > 0) {
		  look->kA = pop(look->ksStack,KEYSET) ; 
		  look->nns-- ;
		} else {
		  look->kA = 0;
		}
		
		break;
	      }


	  finStack:
	    if (look->kA) 
	      { push(look->ksStack, look->kA, KEYSET) ; look->nns++ ;
		aceOutPrint (result_out, "// The stack now holds %d keyset(s), top of stack holds %d objects\n", 
			  look->nns, look->kA ? keySetMax (look->kA) : 0 ) ;
		look->kA = 0 ;
	      }
	    else if (look->nns == 0)
	      aceOutPrint (result_out, "// The stack is now empty \n") ;
	break ;

      case '9': /* subset */
	{
	  int x0, nx, i, j ;
	  
	  if (!aceInInt (command_in,&x0) ||
	      !aceInInt (command_in,&nx) ||
	      x0 < 1 ||
	      nx < 1)
	    aceOutPrint (result_out, "// Usage: subset x0 nx, : returns the subset of the alphabetic active keyset from x0>=1 of length nx") ;
	  else
	    {	      
	      keySetDestroy(look->kA) ;
	      if (look->ksNew) 
		{
		  if (x0 < 1) 
		    x0 = 1 ;
		  if (nx > keySetMax (look->ksNew) - x0 + 1) 
		    nx = keySetMax (look->ksNew) - x0 + 1 ; 
		  look->kA = keySetAlphaHeap(look->ksNew, x0 + nx - 1) ;
		}
	      else 
		nx = 0 ;
	      UNDO ;
	      look->ksNew = keySetCreate () ;
	      for (i = x0 - 1, j = 0 ; nx > 0 ; i++, j++, nx--)
		keySet (look->ksNew, j) = keySet (look->kA, i) ;
	      keySetSort (look->ksNew) ;
	      keySetDestroy (look->kA) ;
	    }	  
	}
	break ;	

	/******************* named Stack operations  *****************************/
	
 	/*
	* kstore - save the active keyset in a named place.
	* (This code copied from NCBI acedb 5/2003 by Mark S.  I guessed
	*  at the changes to make it work here.)
	*/
	case 701:  /* kstore name */
	    {
	    cp = aceInWord(command_in);
            if (!cp)
              aceOutPrint ( result_out, "// usage: kstore name :  store active keyset in internal tmp space as name\n") ;
            else
              {
                KEYSET nKs = arrayHandleCopy (look->ksNew, NULL_HANDLE) ;
    
                if (!look->kget_dict)
                  look->kget_dict = dictHandleCreate (32, NULL_HANDLE) ;
                if (!look->kget_namedSets)
                  look->kget_namedSets = arrayHandleCreate (32, Array, NULL_HANDLE) ;
                if (dictFind (look->kget_dict, cp, &nn) &&
                    nn < arrayMax (look->kget_namedSets))
                  keySetDestroy (arr (look->kget_namedSets, nn, Array)) ;
                else
                  dictAdd (look->kget_dict, cp, &nn) ;
                array (look->kget_namedSets, nn, Array) = nKs ;
    
                aceOutPrint ( result_out, "// Stored named set %s\n", cp) ;
              }
	    }
            break ;

	/*
	* kget name - load the active keyset from a named keyset.
	* (This code copied from NCBI acedb 5/2003 by Mark S.  I guessed
	*  at the changes to make it work here.)
	*/
      case 702:
	    {
	    cp = aceInWord(command_in);
            if (!cp)
              aceOutPrint ( result_out, "// usage: kget name  : copy named set as active keyset\n");
            else
              {
                int nn = 0 ;
                KEYSET nKs = 0 ;
    
                if (look->kget_dict && dictFind (look->kget_dict, cp, &nn) &&
                    look->kget_namedSets &&
                    nn < arrayMax (look->kget_namedSets) &&
                    (nKs = arr (look->kget_namedSets, nn, Array)) &&
                    keySetExists (nKs))
                  {
                    UNDO ;
                    keySetDestroy (look->kA) ;
                    look->ksNew = keySetCopy (nKs) ;
                    aceOutPrint ( result_out, "// Recovered named set %s\n", cp) ;
                  }
                else
                  {
                  aceOutPrint ( result_out, "// empty set %s\n", cp) ;
                  UNDO ;
                  keySetDestroy (look->kA) ;
                  look->kA = keySetCreate();
                  }
              }
	    }
            break ;

	/*
	* kclear name - destroy a named keyset
	* (This code copied from NCBI acedb 5/2003 by Mark S.  I guessed
	*  at the changes to make it work here.)
	*/
	case 703:
	    {
	    cp = aceInWord(command_in);
            if (!cp)
              aceOutPrint ( result_out, "// usage:  clear named set\n") ;
            else
              {
                int nn = 0 ;
    
                if (look->kget_dict && dictFind (look->kget_dict, cp, &nn) &&
                    look->kget_namedSets &&
                    nn < arrayMax (look->kget_namedSets) &&
                    (ksTmp = arr (look->kget_namedSets, nn, Array)) &&
                    keySetExists (ksTmp))
                  {
                    keySetDestroy (ksTmp) ;
                    arr (look->kget_namedSets, nn, Array) = 0 ;
                    aceOutPrint ( result_out, "// Cleared named set %s\n", cp) ;
                  }
                else
                  aceOutPrint (result_out, "// Sorry, unkown set %s\n", cp) ;
              }
	    }
            break ;


      /* SMap calls...... */
      case CMD_SMAP:
      case CMD_SMAPDUMP:
      case CMD_SMAPLENGTH:
	{
	  /* by default write to same output as all other interactions */
	  if (look->dump_out)
	    aceOutDestroy (look->dump_out);
	  look->dump_out = aceOutCopy (result_out, 0);

	  if (option == CMD_SMAPDUMP)
	    {
	      if ((cp = aceInWord(command_in)) && strncmp(cp, "-f", 2) != 0)  /* -f filename */
		{
		  aceInBack(command_in) ;		    /* Put the first word back if its not -f */
		}
	      else
		{
		  if (!(cp = aceInPath(command_in)))	    /* output filename */
		    {
		      aceOutPrint (result_out, "// Error : missing file name for -f option\n") ;
		      printUsage (result_out, qmenu, option);

		      break ;
		    }
		  else
		    {
		      char *outfilename;
		      ACEOUT dump_out;

		      outfilename = strnew(cp, tmp_handle);

		      if ((dump_out = aceOutCreateToFile(outfilename, "w", 0)))
			{
			  aceOutDestroy(look->dump_out); /* kill default output */
			  look->dump_out = dump_out; /* replace with this file output */
			}
		      else
			{
			  aceOutPrint(result_out, "// Error : can't write to file %s\n",
				       outfilename) ;

			  break ;
			}
		    }
		} 
	    }

	  smapCmds(command_in, look->dump_out, option) ;

	  /* Close the file so the results get written out. */
	  if (look->dump_out)
	    {
	      aceOutDestroy(look->dump_out);
	      look->dump_out = NULL ;
	    }

	  break ;
	}

  
	/******************* external calls *****************************/
	    
      case 'Y':			/* "tace" - run command file */
	/* universal + non-server */
	{
	  ACEIN tace_cmd_in;
	  char *filename = NULL;
	  char *params = NULL;
	  BOOL isCommandLineError = FALSE;

	  if (look->choix & CHOIX_SERVER)	/* not possible for server */
	    break ;

	  /* -f filename */
	  if ((cp = aceInWord(command_in)) &&
	      strncmp(cp, "-f", 2) == 0)
	    {
	      if ((cp = aceInPath(command_in)))
		filename = strnew(cp, tmp_handle);
	      else
		isCommandLineError = TRUE;
	    }
	  else
	    isCommandLineError = TRUE;

	  if (isCommandLineError)
	    {
	      printUsage (result_out, qmenu, option);
	    }
	  else
	    {
	      aceInNext(command_in) ;  /* get parameters */
	      if ((cp = aceInPos(command_in)) && strlen(cp) > 0)
		params = strnew(cp, tmp_handle) ;
	      
	      if ((tace_cmd_in = aceInCreateFromFile(filename, "r", params, 0)))
		{
		  /* forbid sub shells */
		  aceInSpecial (tace_cmd_in, "\n\t\\/@%");
		  
		  aceOutPrint (result_out,
			       "// Executing tace -f %s\n", filename) ;
		  
		  /* why isn't the choix stuff set to choix->look ??         */
		  commandExecute (tace_cmd_in, result_out,
				  CHOIX_UNIVERSAL | CHOIX_NONSERVER,
				  look->perms,
				  look->ksNew) ;
		  setLastModified() ;
		  aceInDestroy (tace_cmd_in);
		}
	      else
		{ 
		  aceOutPrint (result_out,
			       "// Sorry, I cannot read file \"%s\"\n", 
			       filename) ;
		}
	    }
	}
	break ;

#ifdef ACEMBLY
      case 'A':	
	{
	  ACEIN acembly_in = NULL;
	  char *filename = NULL, *params = NULL;
	  BOOL oldIsTS = isTimeStamps ;

	  if (look->choix & CHOIX_SERVER) /* server should never open  stdin */
	    break ;
	  
	  if ((cp = aceInWord(command_in))) /* -f filename */
	    { 
	      if (strncmp(cp, "-f", 2) == 0)
		{
		  if ((cp = aceInPath(command_in))) /* get filename */
		    filename = strnew(cp, tmp_handle);
		  else
		    {
		      aceOutPrint (result_out, "// Error : no filename after -f option\n");
		      break;
		    }
		}
	      else
		aceInBack(command_in) ; 
	    }

	  aceInNext(command_in) ;  /* get parameters */
	  params = strnew(aceInPos(command_in), tmp_handle) ;
	  CHECK_WRITE ;

	  if (filename)
	    {
	      acembly_in = aceInCreateFromFile (filename, "r", params, tmp_handle);
	      if (!acembly_in)
		{ 
		  aceOutPrint (result_out, "Sorry: I cannot read file \"%s\"\n",
			       filename) ;
		  break ;
		}
	    }
	  else
	    acembly_in = aceInCreateFromStdin (aceInIsInteractive(command_in), params, tmp_handle);

	  isTimeStamps  = FALSE ;   /* to accelerate the code */

	  defComputeTace (acembly_in, result_out, look->ksNew) ;

	  setLastModified() ;

	  isTimeStamps = oldIsTS ;
	}
	break ;
#endif /* ACEMBLY */
	
	/****************************************************************/
	
      case 450:			/* AQL */
      case 451:
	{
	  char *infilename = 0;
	  char *querytext = 0;
	  BOOL isCommandLineError = FALSE;
	  AQL aql;
	  TABLE *activeTable;
	  TABLE *result;

	  if (look->dump_out)
	    aceOutDestroy (look->dump_out);
	  /* by default output table results to the same stream
	   * as the rest of the command interactions */
	  look->dump_out = aceOutCopy (result_out, 0);

	  aceInNext(command_in) ;
	  look->beauty = 'a' ;

	  if (option == 450)
	    while ((cp = aceInWord(command_in)))   /* get aql options */
	      {
		if (*cp != '-')
		  { aceInBack (command_in) ; break ; }
		if (strncmp (cp, "-s", 2) == 0)	/* silent */
		  look->beauty = 0;
		else if (strncmp (cp, "-a", 2) == 0)
		  look->beauty = 'a';
		else if (strncmp (cp, "-A", 2) == 0)
		  look->beauty = 'A';
		else if (strncmp (cp, "-h", 2) == 0)
		  look->beauty = 'h';
		else if (strncmp (cp, "-j", 2) == 0)
		  look->beauty = 'j';
		else if (strncmp (cp, "-J", 2) == 0)
		  look->beauty = 'J';
		else if (strncmp (cp, "-k", 2) == 0)
		  look->beauty = 'k';
		else if (strncmp (cp, "-K", 2) == 0)
		  look->beauty = 'K';
		else if (strncmp (cp, "-C", 2) == 0)
		  look->beauty = 'C';
		else if (strcmp(cp, "-x") == 0)  /* default xml, content only */
		  look->beauty = 'x' ;
		else if (strcmp(cp, "-xclass") == 0)  /* xml giving class as attribute */
		  look->beauty = 'X' ;
		else if (strcmp(cp, "-xall") == 0)  /* xml with class and value attributes and content */
		  look->beauty = 'y' ;
		else if (strcmp(cp, "-xnocontent") == 0)  /* xml with class and value attributes, no content */
		  look->beauty = 'Y' ;
		else if (strncmp (cp, "-f", 2) == 0)  /* -f infilename */
		  { 
		    if ((cq = aceInPath(command_in)))
		      infilename = strnew(cq, tmp_handle);
		    else
		      {
			aceOutPrint (result_out, "// Error: option -f requires filename argument\n") ;
			isCommandLineError = TRUE;
			break;
		      }
		  }
		else if (strncmp (cp, "-o", 2) == 0)
		  { 
		    char *outfilename = aceInPath(command_in);
		    if (outfilename)
		      {
			ACEOUT dump_out;

			dump_out = aceOutCreateToFile (outfilename, "w", 0);
			if (!dump_out)
			  { 
			    aceOutPrint (result_out,
					 "// Error: cannot write to %s\n",
					 outfilename) ;
			    isCommandLineError = TRUE;
			    break ;
			  }

			/* replace default output stream with fileoutput */
			aceOutDestroy (look->dump_out);
			look->dump_out = dump_out;
		      }
		    else
		      {
			aceOutPrint (result_out, "// Error: option -o requires filename argument\n") ;
			isCommandLineError = TRUE;
			break;
		      }
		  }
		else
		  isCommandLineError = TRUE;
	      } /* end while options */
	  
	  if (isCommandLineError)
	    {
	      printUsage (result_out, qmenu, option);
	      aceOutDestroy (look->dump_out);
	      break;
	    }

	  if (infilename)
	    /**** get query from input file ****/
	    {

	      ACEIN aql_in = NULL;

	      aql_in = aceInCreateFromFile (infilename, "r", "", 0);
	      if (aql_in)
		{
		  Stack s = stackCreate(100);
		  char *line;
		  
		  aceInSpecial(aql_in, "\n");
		  while ((line = aceInCard(aql_in)))
		    {
		      catText(s, line);
		      catText(s, " ");
		    }
		  querytext = strnew(stackText(s, 0), tmp_handle);
		  stackDestroy(s);
		  
		  aceInDestroy(aql_in);
		}
	      else
		{
		  aceOutPrint (result_out, "// Error : cannot open query-file %s\n",
			       infilename);
		  isCommandLineError = TRUE;
		}
	    }
	  else
	    /**** get query from command line ****/
	    {
	      cp = aceInPos (command_in) ;         /* query is rest-of-line */

	      if (option == 450)
		querytext = strnew(cp, tmp_handle);
	      else if (option == 451)
		/* select has already been swallowed, so replace */
		querytext = strnew(messprintf("select %s", cp), tmp_handle);
	    }

	  if (isCommandLineError)
	    {
	      printUsage (result_out, qmenu, option);
	      aceOutDestroy (look->dump_out);
	      break;
	    }

	  if (strlen(querytext) == 0)
	    { 	    
	      aceOutPrint (result_out, "// Error: no query\n") ;
	      aceOutDestroy (look->dump_out);
	      break;
	    }

	  activeTable = tableCreateFromKeySet(look->ksNew, 0);/*ksNew, mieg aug 5, 2004 */

	  {
	    static int aql_debug_level = -1;

	    if (aql_debug_level < 0)
	      {
		char *ad_string = getenv("AQL_DEBUG_LEVEL");
		if (ad_string != NULL)
		  {
		    aql_debug_level = atoi (ad_string);
		  }
		else
		  {
		    aql_debug_level = 0;
		  }
	      }
          

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
	    aql = aqlCreate(querytext, look->dump_out,
			    look->beauty, aql_debug_level, aql_debug_level, "@active", 0) ;

	    result = aqlExecute(aql, 0, &(look->ksNew), 0, "TABLE @active", activeTable, 0);
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

	    aql = aqlCreate(querytext, look->dump_out,
			    look->beauty, aql_debug_level, aql_debug_level, "@active", (char *)NULL) ;

	    result = aqlExecute(aql, NULL, &(look->ksNew), NULL, "TABLE @active", activeTable, (char *)NULL);
	  }

	  tableDestroy (activeTable);
	  
	  if (!(aqlIsError(aql)))
	    {
	      /* dump the table */
	      /* beauty == 0 means silent */
	      if (look->beauty)
		tableOut (look->dump_out, result, '\t', look->beauty) ;

	      UNDO ;
	      look->ksNew = keySetCreateFromTable (result, 0) ;
	      
	      tableDestroy (result) ;
	    }
	  else
	    {
	      aceOutPrint (result_out, "%s", aqlGetErrorReport(aql));
	    }

	  aqlDestroy (aql) ;
	  
	  aceOutDestroy (look->dump_out);
	}
	break;	/* end-case 450/451 AQL query */

      case 402:	/* AceC system_list */
	{
	  KEY tag, cl ;

          char buf0[2] = {0, '\n'} ;
          char buft [2] ;
          unsigned int ii ;
          char *cp ;

          ii = 0x12345678 ;
          aceOutBinary (result_out, (char*)&ii,4) ;
          ii = lexMax(0) ;
          aceOutBinary (result_out, (char*)&ii,4) ;

          buft[0] = '>' ;
          buft[1] = 'g' ;
          for (tag = 0 ; tag < lexMax(0) ; tag++)
            {
              aceOutBinary (result_out, buft,2) ;
              cp = name(tag) ;
              aceOutBinary (result_out, cp, strlen(cp)) ;
              aceOutBinary (result_out, buf0,2) ;
              buft[0] = '.' ;
            }
          buft[1] = 'k' ;
          for (cl = 0 ; cl < 256 ; cl++)
            { 

              aceOutBinary (result_out, buft,2) ;
              if (pickList[cl].name)
                {
                  cp = pickClass2Word (cl) ;
                  aceOutBinary (result_out, cp, strlen(cp)) ;
                }
              aceOutBinary (result_out, buf0,2) ;
            }
          aceOutBinary (result_out, "#\n",2) ;
	}
	break;

      default:
	aceOutPrint(result_out, "// Sorry, option not written yet\n") ;
	break;
      }	/* end-switch (option) */

  look->lastCommand = option ;
  
  switch (option)
    {
    case 'B': case 'm': case 'M': case 'D':
      /* these are the options for 'more' statements */
      break ;

    case 'q':
      /* quit command */
      break;

    default:
      /* after any other command we output the number of objects
       * in the active keyset */
      if (!look->quiet)
	aceOutPrint(result_out, "// %d Active Objects\n", 
		    keySetCountVisible(look->ksNew)) ;
    }

  /* Clean up.                                                               */
  handleDestroy (tmp_handle);

  /* End of a disgusting hack, see  case 'w'  above, truly yuck...           */
  if (tmp_command_in)
    aceInDestroy(tmp_command_in) ;


  return option ;
} /* aceCommandExecute */

/************************************************************/

/* drop write access for this look */
void aceCommandNoWrite(AceCommand look)
{ 
  if (!aceCommandExists(look))
    messcrash("aceCommandNoWrite() - received non-magic look pointer");

  look->noWrite = TRUE ;

  return ;
}



void aceCommandSetInteraction(AceCommand look, BOOL user_interaction)
{ 
  if (!aceCommandExists(look))
    messcrash("aceCommandNoWrite() - received non-magic look pointer");

  look->user_interaction = user_interaction ;

  return ;
}




/*************************************************************/
/******************************************************************
 ************************ private functions ***********************
 ******************************************************************/


/****************************************************************/

static BOOL aceCommandExists (AceCommand look)
{
  if (look && look->magic == &AceCommand_MAGIC)
    return TRUE;

  return FALSE;
} /* aceCommandExists */

/****************************************************************/

static void taceTest (ACEIN command_in, ACEOUT res_out)
{ 
  /* don't leave anything dodgy in here when you check it back in */
  KEY key;
  STORE_HANDLE handle = handleCreate();
  int length;
  SMap *smap;
  char *word = aceInWord(command_in);
  
  if (lexword2key(word, &key, _VSequence))
    {
      length = sMapLength(key);
      smap = sMapCreate(handle, key, 1, length, NULL);
      sMapDump(smap, res_out, SMAPDUMP_ALL);
    }

  handleDestroy(handle);
  return ;
}

/****************************************************************/

/* printUsage puts a "// " in front of every line of a command description,  */
/* this is different from the way "?" usually displays this information, so  */
/* I have added printCmd() which doesn't add the "// ".                      */
/*                                                                           */
/* Note that both of these routines are expecting to find the command,       */
/* really they should return a BOOL if they can't find it...                 */
/*                                                                           */
static void printUsage (ACEOUT fo, FREEOPT *qmenu, KEY option)
{
  int i;
  char *line;
  ACEIN usage_io;

  for (i = 1; i < (qmenu[0].key+1); i++)
    if (qmenu[i].key == option)
      {
	aceOutPrint (fo, "// Usage : \n");

	usage_io = aceInCreateFromText(qmenu[i].text, "", 0);
	aceInSpecial(usage_io,"\n");
	while ((line = aceInCard(usage_io)))
	  aceOutPrint (fo, "// %s\n", line);
	aceInDestroy(usage_io);

	break;
      }

  return;
} /* printUsage */

static void printCmd(ACEOUT fo, FREEOPT *qmenu, KEY option)
{
  int i ;

  for (i = 1; i < (qmenu[0].key+1); i++)
    {
      if (qmenu[i].key == option)
	{
	  aceOutPrint (fo, "%s\n", qmenu[i].text) ;
	  break ;
	}
    }

  return ;
}

/****************************************************************/

static int getDataVersion (void)
     /* utility for client server */
{
  static int version = -1 ;

  if (version == -1)
    { 
      char *cp ;
      int ii;
      ACEIN server_in = NULL;
      char *filename = dbPathStrictFilName("wspec", "server", "wrm", "r", 0) ;

      if (filename)
	{ 
	  server_in = aceInCreateFromFile (filename, "r", "", 0);
	  messfree(filename);
	}
      
      version = 0 ;

      if (server_in) 
	{
	  while (aceInCard(server_in))
	    { 
	      cp = aceInWord(server_in);
	      if (cp 
		  && strcmp (cp,"DATA_VERSION") == 0
		  && aceInInt (server_in, &ii))
		{ version = ii ; break ; }
	    }
	  aceInDestroy (server_in);
	}
    }

  return version ;
} /* getDataVersion */

/****************************************************************/

static char getOnOff (ACEIN fi)
{ 
  char *cp ;
  
  aceInNext (fi) ;
  if ((cp = aceInWord (fi)))
    {
      if (strcasecmp (cp, "off") == 0)
	return 'f' ;
      else if (strcasecmp (cp, "on") == 0)
	return 't' ;
      else
	return '?' ;
    }
  return '\0' ;
} /* getOnOff */

/************************************************************/

static BOOL doShowModel (ACEIN model_in, char *name, ACEOUT result_out)
{
  BOOL result ;
  char *card, *word ;

  aceInSpecial(model_in,"\n") ;

  /* look for ?className minus the leading "?" */
  while ((card = aceInCard (model_in)))
    if ((word = aceInWord(model_in)) && strcmp (word+1, name) == 0)
      break ;

  /* If found, output the whole class which will be terminated by a blank line,
   * the start of a new class, or # definition. */
  if (card)
    {
      do 
	aceOutPrint (result_out, "%s\n", card) ;
      while ((card = aceInCard(model_in)) && aceInWord(model_in)
	     && !(*card == '?' || *card == '#')) ;

      result = TRUE;
    }
  else
    result = FALSE;

  return result ;
}

static void showModel (ACEOUT result_out, KEY key)
{
#ifdef JUNK
  /* this methos ios much simpler and does export the system models
     but the layout is a bit off and the comments are lost
     
     it is equivalent to 
          find model ?mmm ; show 
	  */

  dumpKeyBeautifully (result_out, key, 'h', 0) ;
#else
  ACEIN model_in = 0 ;
  char *filename;
  Stack s, sysModelStack;
  BOOL found = FALSE;
  char *linep;

  /* sysModels() returns a braindead stack where each line is pushed
   * onto the stack rather than concatenated with newline at EOL */
  s = sysModels ();
  sysModelStack = stackCreate(1000); pushText (sysModelStack, "\n");
  stackCursor (s, 0) ;
  while ((linep = stackNextText(s)))
    {
      catText (sysModelStack, linep) ;
      catText (sysModelStack, "\n") ;
    }
  stackDestroy (s);

  model_in = aceInCreateFromText (stackText(sysModelStack, 0), "", 0);
  found = doShowModel (model_in, name(key)+1, result_out);
  stackDestroy (sysModelStack);

  if (!found)
    {
      filename = dbPathStrictFilName("wspec", "models", "wrm", "r", 0);
      if (filename)
	{
	  model_in = aceInCreateFromFile (filename, "r", "", 0);
	  messfree(filename);
	}
      else
	model_in = NULL;

      if (model_in)
	{
	  found = doShowModel (model_in, name(key)+1, result_out);
	  aceInDestroy (model_in) ;
	}
      else
	{
	  /* How did we get so far ? models.wrm is part of the
	   * database specification and although an initialised
	   * database can quite happily exist without it and rely
	   * in internal Model-objects, the user should be using
	   * the database without it. */
	  messerror ("The database must have a valid model file "
		     "in wspec/models.wrm !");
	  return;		/* no need to continue */
	}
    }

  if (!found)
    {
      /* This is disturbing! A model for which there exists a valid key
       * should be found in sys-models or models.wrm !
       * The models.wrm file may have been updated without Read-models
       * But we can still dump the internal representation
       * of the model. */
      dumpKeyBeautifully (result_out, key, 'h', 0) ;

      /* Although we have provided an answer to the user it
       * may still be handy to signal that things have gone stale
       * let's assume that sysmodels are always up-to-date and blame
       * all inconsistencies on models.wrm */
      aceOutPrint (result_out, "// File wspec/models.wrm out of sync, "
		   "consider to 'Read-models'.");
    }
#endif

  return;
} /* showModel */


/* This set of routines implements a crude "last modified" timestamp for the */
/* database. This WILL NOT WORK for xace because these routines are only     */
/* called from command.c, i.e. for the command interface. A more thorough    */
/* way to do this would be to call these routines from somewhere like        */
/* bsSave(), _but_ there are some subtleties in that route that require      */
/* some work to make sure its reliable...                                    */
/*                                                                           */
/* So, lastModified() actually holds the time, to be threadsafe this routine */
/* would need to mutex etc., set/getLastModified call lastModified to set/get*/
/* the last modified time. At various points in  aceCommandExecute()  we     */
/* call setLastModified(), and every time the routine receives the           */
/* "lastmodified" command it calls getLastModified() to return the time.     */
/*                                                                           */
void setLastModified(void)
{

  lastModified(NULL) ;

  return ;
}


char *getLastModified(void)
{
  char *result = NULL ;

  lastModified(&result) ;

  return result ;
}

void lastModified(char **new_time)
{
  static char time_last_modified[25] = "" ;
  
  if (new_time == NULL)
    timeShow2(time_last_modified, timeNow()) ;
  else
    {
      if (!*time_last_modified)
	timeShow2(time_last_modified, timeNow()) ;
      *new_time = time_last_modified ;
    }

  return ;
}


/* This function implements the various commands for the command line interface to smap. */
static void smapCmds(ACEIN command_in, ACEOUT result_out, int option)
{ 

  switch (option)
    {
      case CMD_SMAP:
	{
	  commandDoSMap(command_in, result_out) ;

	  break ;
	}
      case CMD_SMAPLENGTH:
      case CMD_SMAPDUMP:
	{
	  KEY key = KEY_UNDEFINED ;
	  int length ;
	  char *word, *cmd ;
	  STORE_HANDLE handle ;

	  handle = handleCreate() ;
	  cmd = (option == CMD_SMAPDUMP ? "dump" : "length") ;

	  if (!(word = strnew(aceInWord(command_in), handle)))
	    aceOutPrint(result_out, "// smap%s error: no object specified\n", cmd) ;
	  else if (!lexClassKey(word, &key))
	    aceOutPrint(result_out, "// smap%s error: object \"%s\" does not exist.\n", cmd, word) ;
	  else
	    {
	      length = sMapLength(key) ;

	      if (option == CMD_SMAPDUMP)
		{
		  SMap *smap ;

		  smap = sMapCreate(handle, key, 1, length, NULL) ;
		  sMapDump(smap, result_out, SMAPDUMP_ALL) ;
		}
	      else
		{
		  aceOutPrint(result_out, "SMAPLENGTH %s %d\n",
			      nameWithClassDecorate(key), length) ;
		}
	    }

	  handleDestroy(handle);

	  break ;
	}
    default:
      {
	messcrash("Bad option in smapCmds()") ;
      }
    }


  return ;
}



/* Called from gif submenu currently, could be made static if we could get rid of this dependency.. */
void commandDoSMap(ACEIN command_in, ACEOUT result_out)
{
  char *cp, *cp1;
  BOOL gotCoords = FALSE, dump = FALSE, gotArea=FALSE;
  int x1, x2, y1, y2, nx1, nx2, a1, a2;
  STORE_HANDLE handle ;
  KEY to = KEY_UNDEFINED, from = KEY_UNDEFINED ;
  SMap* smap = NULL ;
  SMapKeyInfo *info = NULL ;
  SMapStatus status = SMAP_STATUS_ERROR ;
  int out_x = 0, out_y = 0 ;


  messAssert(command_in && result_out) ;

  handle = handleCreate();

  while ((cp = strnew(aceInWord(command_in), handle)))
    {
      if (strcmp(cp, "-dump") == 0)
	dump = TRUE;
      else if (strcmp(cp, "-coords") == 0)
	{ 
	  if (!aceInInt (command_in, &x1) || !aceInInt (command_in, &x2) || (x2 == x1))
	    goto usage ;
	  gotCoords = TRUE;
	}
      else if (strcmp(cp, "-area") == 0)
	{ 
	  if (!aceInInt (command_in, &a1) || !aceInInt (command_in, &a2))
	    goto usage ;
	  gotArea = TRUE;
	}
      else if (strcmp(cp, "-to") == 0)
	{
	  if (!(cp1 = strnew(aceInWord(command_in), handle)))
	    goto usage ;
	  if (!lexClassKey(cp1, &to))
	    {
	      aceOutPrint (result_out, "// smap error: invalid to object\n") ;
	      goto abort ;
	    }
	}
      else if (strcmp(cp, "-from") == 0)
	{
	  if (!(cp1 = strnew(aceInWord(command_in), handle)))
	    goto usage ;
	  if (!lexClassKey(cp1, &from))
	    {
	      aceOutPrint (result_out, "// smap error: invalid from object\n") ;
	      goto abort ;
	    }
	}
      else
	goto usage ;
    }
  
  if (from == KEY_UNDEFINED)
    {
      aceOutPrint (result_out, "// smap error: must have \"-from\"\n") ;
      goto abort ;
    }
  
  if (!gotCoords)
    {
      x1 = 1 ;
      x2 = 0 ;						    /* Drop through to code to find length. */
    }

  if (x2 == 0)
    {
      if (!(x2 = sMapLength(from)))
	{
	  aceOutPrint (result_out,
		       "// smap error: Cannot find length of \"-from\" object: \"%s\"\n",
		       name(from)) ;
	  goto abort ;
	}
    }


  if (dump)
    {
      if (!gotArea)
	{
	  a1 = 1;
	  a2 = x2 - x1 + 1;
	}

      if (!(smap = sMapCreateEx(handle, from, x1, x2, a1, a2, NULL)))
	{
	  aceOutPrint (result_out, "// smap error: Cannot create smap\n") ;
	  goto abort ;
	}
      
      sMapDump(smap, result_out, SMAPDUMP_ALL);
      goto abort;
      
    }
    

  /* Only create the smap over the area we want, _not_ the full smap which can be v. slow. */
  if (to == KEY_UNDEFINED)
    {
      if (!sMapTreeRoot(from, x1, x2, &to, &out_x, &out_y))
	{
	  aceOutPrint (result_out,
		       "// smap error: Cannot find position of \"-from %s\""
		       " (coords %d, %d)%s%s\n",
		       nameWithClass(from), x1, x2,
		       (to ? " in parent " : ""),
		       (to ? name(to) : "")) ;
	  goto abort ;
	}
    }
  else
    {
      if (!sMapTreeCoords(from, x1, x2, &to, &out_x, &out_y))
	{
	  aceOutPrint (result_out,
		       "// smap error: Cannot find position of \"-from %s\""
		       " (coords %d, %d) in \"-to %s\"\n",
		       nameWithClass(from), x1, x2, nameWithClass(to)) ;
	  goto abort ;
	}
    }
  
  /* I have inserted this optimisation because its very expensive to do the the mapping if
   * the object is big as in a chromosome and the caller may not always know what the top
   * object is and so may ask for a mapping of the object to itself inadvertantly. */
  if (from == to)
    {
      y1 = nx1 = x1 ;
      y2 = nx2 = x2 ;

      status = SMAP_STATUS_PERFECT_MAP ;
    }
  else
    {
      if (!(smap = sMapCreateEx(handle, to,  1, sMapLength(to), out_x, out_y, NULL)))
	{
	  aceOutPrint (result_out, "// smap error: Cannot create smap\n") ;
	  goto abort ;
	}

      if (!(info = sMapKeyInfo(smap, from)))
	{
	  aceOutPrint (result_out, "// smap error: -from object not in sMap\n") ;
	  goto abort ;
	}
  
      status = sMapMap(info, x1, x2, &y1, &y2, &nx1, &nx2);
    }

  aceOutPrint(result_out, "SMAP %s", className(to));
  aceOutPrint(result_out, ":%s", name(to));
  aceOutPrint(result_out, " %d %d %d %d ", y1, y2, nx1, nx2);
  if (status == SMAP_STATUS_PERFECT_MAP)
    aceOutPrint(result_out, "%s", sMapErrorString(status));
  else
    {
      int bit = 1;
      char *sep="";
      while (bit)
	{
	  char *string;
	  if ((status & bit) && (string = sMapErrorString(status&bit)))
	    {
	      aceOutPrint(result_out, "%s%s", sep, string);
	      sep = ",";
	    }
	  bit = bit<<1;
	}
    }
  aceOutPrint(result_out, "\n");
  handleDestroy(handle);
  return;
  
    usage:
  aceOutPrint (result_out, "// smap error: usage: "
	       "smap -coords x1 x2 -from class:object -to class:object\n") ;
    abort:
  handleDestroy(handle);

  return ;
}





/****************************** eof *****************************/
