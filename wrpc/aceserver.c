/*  File: aceserver.c
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
 * Description:      Network Query Server   
 *     Check authorizations and times-out

 * added back , because we need to know to correctly connect (mieg)
 * [rd 970722] remove "rpc.*" argv[0] test for isDaem, because that was
 * hardwired in anyway.

    checkLife keeps track of the number of active clients.  
   If a client is not active for clientTimeOut seconds, it will be disconnected.
   If there are no more active clients, the server itself will disconnect
   after serverTimeOut seconds.
 
   If a time_out is set to 0 (zero) expiration is deactivated

   Note that, when saving, you save all clients at once, there
   is NO WAY to save independantly a single client session.
 *
 * Exported functions:
 * HISTORY:
 * Last edited: Mar 12 10:19 2001 (edgrif)
 * * Jul  6 16:21 2000 (edgrif): For ace_in data now use new doParseAceIn function.
 * * Nov 23 09:46 1999 (edgrif): Use the new 'option' values for admin cmds.
 * * Sep 24 14:49 1999 (edgrif): Removed '@' & '%' from freespecial, disallow ace comd files.
 * * Mar 22 14:45 1999 (edgrif): Replaced messErrorInit with messSetMsgInfo
 * * Dec  2 16:32 1998 (edgrif): Correct decl. of main, add code to
 *              record build time of this module.
 * * Oct 15 10:57 1998 (edgrif): Add new graph/acedb initialisation call.
 * * Feb 25 23:00 1996 (mieg)
 * * Mar 97 (mieg) SIGPIPE handler
 * Created: Wed Nov 25 12:26:06 1992 (mieg)
 * CVS info:   $Id: aceserver.c,v 1.116 2003-07-04 01:50:13 mieg Exp $
 *-------------------------------------------------------------------
 */

#include <signal.h>
#include <netinet/in.h>

#include <sys/types.h>
#include <sys/stat.h>		/* for umask */

#include <wh/acedb.h>
#include <wh/aceio.h>
#include <wh/help.h>
#include <wh/acelib.h>
#include <wh/session.h>
#include <wh/dbpath.h>
#include <wh/pick.h>
#include <wh/bs.h>
#include <wh/call.h>
#include <wh/aceclient.h>
#include <wh/dbpath.h>
#include <wh/parse.h>
#include <wh/version.h>
#include <wh/command.h>
#include <wh/keyset.h>
#include <wh/query.h>
#include <wh/a.h>
#include <wh/dump.h>
#include <wh/lex.h>
#include <wh/sigsubs.h>
#include <whooks/sysclass.h>
#include <whooks/systags.h>
#include <whooks/tags.h>

#ifdef GIFACESERVER
#include <wh/acedbgraph.h>
#include <wh/display.h>
#include <wh/graph.h>					    /* See stub routines at end of file. */
#endif

/************************************************************/

#define debug FALSE

static char usage[] = "Usage: aceserver database_dir [port_number [params]]\n"
		      "  params are clientTimeout:serverTimeout:maxKbytes:autoSaveInterval\n"
		      "  defaults are port             = 20000101\n"
		      "               clientTimeout    = 600 seconds\n"
		      "               serverTimeout    = 600 seconds\n"
		      "               maxKbytes        = 0 [no limit]\n"
		      "               autoSaveInterval = 600 seconds\n"
		      "  autoSave checks are only certain at serverTimeout intervals\n"
		      "  example:  aceserver /local1/worm  20000100  1200:1200:100\n"
		      "            aceserver /local1/worm d20000100  1200:1200:100\n" ;


/* reason for lack of -option notation is that inetd only allows 5 params in some 
   implementations 
*/

extern BOOL parseKeepGoing ;

/* Time outs etc. */
static int clientTimeOut = 600 ;
static int serverTimeOut = 600 ;
static int maxKbytes = 0;
static int autoSaveInterval = 600 ;
static int nActiveClients = 0; 


static BOOL isDaemon = FALSE ;
static BOOL isSGI = FALSE ;
static Array wspecTar = 0 , wspecTarEncoded = 0 ;

#if defined(WIN32)
#define DCESERVER
extern void my_Signal( void (*callback)(int arg) ) ;
extern void my_Alarm(unsigned int seconds) ;
#define signal(S,C) my_Signal(C) /* ignore the SIGALRM type */
#define alarm(T) my_Alarm(T)

typedef unsigned long u_long ;
extern void stopDCEServer(void) ;
#endif


static FILE *out = 0 ;
static BOOL refuseNewClient = FALSE ;
static Associator clientAss = 0 ;

/************************************************************/

typedef struct LOOKSTUFF { int writeMagic, readMagic ; 
			   int clientId ;
			   char *magicReadFileName ;
			   char *magicWriteFileName ;
			   mytime_t lastAccess;
			   AceCommand aceCommand ;
			   KEY lastCommand ;
			   int public ;
                           BOOL mayWrite ;
			 } *LOOK ;

/******************* INITIALISATION ***************************/
/**************************************************************/

static void clientDestroy (LOOK look)
{ char *cp, *cp0 = 0 ;

  fprintf (out, "\nClosing Client %d, %d active clients\n", 
	       look->clientId, --nActiveClients) ;
  cp = cp0 + look->clientId ;
  assRemove (clientAss, cp) ;
  if (look->aceCommand != NULL)
    aceCommandDestroy (look->aceCommand) ;
  messfree (look->magicReadFileName) ;
  messfree (look->magicWriteFileName) ;
  messfree (look) ;

  return;
} /* clientDestroy */

static void wormClose(void)
{ extern void closePortMap(void);
  
#if defined(DCESERVER)
  stopDCEServer() ;
#else
  if (!isDaemon)  /* in daemon mode, do not close port mapper 
		  *  this is needed on alpha, may be machine dependant ?
		  */
    {
      fprintf(out,"\n\n#### Closing port map\n") ;
      closePortMap();	
    }
#endif

  if (isWriteAccess())
    aceQuit (TRUE);
  else
    aceQuit (FALSE);

  fprintf(out,"\n\n#### Server normal exit %s\n  A bientot \n\n",
	  timeShow(timeNow())) ;
  if (out && out!= stderr)
    filclose(out) ;

  exit(0) ;
} /* wormClose */

extern void wait_for_client (u_long port, BOOL isDaemon) ;
static u_long port = DEFAULT_PORT ;


/**************************************************************/
#if !defined(DCESERVER)

  /* SIGPIPE, generated if say the client types CTRL-C 
   * i hope resuming operations in this way is ok, not sure
   * mieg: march 97
   */
static void sigPipeError (int dummy)  /* dummy:  to please signal() prototypes */
{ 
  fprintf(out, "// SIGPIPE error, hopefully on client side, ignore // %s\n", timeShow(timeNow())) ;
  wait_for_client(port, isSGI && isDaemon ) ;
}
#endif
/**************************************************************/
  /* checkLife is called by alarm, 
   * but also after every query including from unauthorised clients
   */
static void checkLife (int dummy)  /* dummy:  to please signal() prototypes */
{ static BOOL dying = FALSE ;
  LOOK look;
  int dt ;
  char *cp = 0 ;
  mytime_t t = timeNow () ;
  static   mytime_t last_t = 0 ;

  if (refuseNewClient && !nActiveClients) 
    { fprintf(out,"// server shut down on request\n\n") ;
      wormClose () ;
    }

  if (dying && !nActiveClients)
    { if (timeDiffSecs (last_t, t, &dt) &&
	10*dt >= 9*serverTimeOut)  /* there seem to exist rounding errors, hence the necessity of 10/9 */
	{ fprintf(out,"// Reached timeout, server will shut down\n\n") ;
	  wormClose () ;
	}
      return ; /* do not update last_t in this case */
    }

  while (assNext (clientAss, &cp, &look)) /* need to loop over all clients */
    if (timeDiffSecs (look->lastAccess, t, &dt) &&
	dt > clientTimeOut) 
      { clientDestroy (look) ;
	fprintf(out, "// Destroying an idle client // %s\n", timeShow(timeNow())) ;
	cp = 0 ;    /* restart the search - needed to keep assNext() happy after assRemove */
      } 
    else
      /* printf("Not Destroying an idle client\n") */ ;

  sessionMark (FALSE, autoSaveInterval) ; /* save before restarting count down */
  signal (SIGALRM, checkLife) ;
  if (nActiveClients) 
    {
      if (clientTimeOut)
	alarm ((unsigned int) clientTimeOut) ;
      dying = FALSE ;
    } 
  else 
    { fprintf(out,"\nNo more active clients, starting to count-down: %d s // %s\n", 
	      serverTimeOut, timeShow(timeNow())) ;
      if (serverTimeOut)
	alarm ((unsigned int)serverTimeOut) ;
      dying = TRUE ;
    }
  last_t = t ;
}

/****************** Password file **********************/

static void setPassFile (LOOK look, 
			 int magicRead, int magicWrite,
			 int clientId)
{ 
  static char *magicWriteDir = 0 ;
  static char *magicReadDir = 0 ;
  static BOOL  firstPass = TRUE ;
  static int   public = 0 ;
  FILE *f = 0 ;
  char *cp ;
  int   level ;

  if (firstPass)
    { firstPass = FALSE ;
      cp = dbPathStrictFilName ("wspec", "server", "wrm", "r", 0) ;
      if (!cp)
	{ fprintf (out, "Server cannot open wspec/server.wrm") ;
	  messExit ("Server cannot open wspec/server.wrm") ;
	}

      if (!(f = filopen (cp, 0, "r")))
	/* should work if above check was successful */
	messcrash ("Cannot initialise server - Failed to open config file : %s", cp) ;

      messfree (cp) ;
      level = freesetfile(f,0) ;
      while (freecard(level))
	{ cp = freeword() ;
	  if (cp)
	    { 
	      if (!strcmp (cp,"WRITE_ACCESS_DIRECTORY") &&
		  (cp = freeword()) )
	        magicWriteDir = strnew (cp, 0) ;
	      else if (!strcmp (cp,"READ_ACCESS_DIRECTORY") &&
		(cp = freeword()))
		{ 
		  if (!strcmp(cp, "PUBLIC"))
		    public = 2 ;
		  else if (!strcmp(cp, "RESTRICTED"))
		    public = 0 ;
		  else
		    { public = 1 ; magicReadDir = strnew (cp, 0) ; }
		}
	    }
	}
      freeclose (level) ;
    }
  
  if (!public && !magicWriteDir)
    { fprintf (out, "Nobody can read or write, please set in the file wspec/server.wrm\n"
	       "the variables READ_ACCESS_DIRECTORY and WRITE_ACCESS_DIRECTORY") ;
      messExit ("Nobody can read or write, please set in the file wspec/server.wrm\n"
		 "the variables READ_ACCESS_DIRECTORY and WRITE_ACCESS_DIRECTORY") ;
    }

  look->magicReadFileName = 0 ;
  if (magicReadDir)
    {
      cp = filGetName (messprintf("%s/client.R.%d",magicReadDir,clientId), 0, "w", 0) ;
      if (!cp || !(f = filopen(cp, 0, "w")))
	{
	  fprintf (out, "Server cannot create a magic file in the READ_ACCESS_DIRECTORY %s", magicReadDir) ;
	  messExit ("Server cannot create a magic file in the READ_ACCESS_DIRECTORY %s", magicReadDir) ;
	}

      fprintf(f, "%d\n", magicRead)  ;
      filclose (f) ;
      look->magicReadFileName = cp ;
    }

  look->magicWriteFileName = 0 ;
  if (magicWriteDir)
    {
      cp = filGetName (messprintf("%s/client.W.%d",magicWriteDir,clientId), 0, "w", 0) ;
      if (!cp || !(f = filopen(cp, 0, "w")))
	{
	  fprintf (out, "Server cannot create a magic file in the WRITE_ACCESS_DIRECTORY %s", magicWriteDir) ;
	  messExit ("Server cannot create a magic file in the WRITE_ACCESS_DIRECTORY %s", magicWriteDir) ;
	}
      fprintf(f, "%d\n", magicWrite)  ;
      filclose (f) ;
      look->magicWriteFileName = cp ;
    }

  look->public = public ;

  return ;
}

static void removePassFile (LOOK look)
{
  if ( look->magicReadFileName)
    filremove ( look->magicReadFileName, 0) ;
  if ( look->magicWriteFileName)
    filremove ( look->magicWriteFileName, 0) ;
}

/****************** MAIN LOOP **********************/

 /* By convention, each reply starts with a message ending on /n
    which will be deleted if aceclient runs silently rather than verbose 
    */

/* DWB - int maxChar, int *encore added to parameter list for processQueries */

Stack processQueries (int *clientIdp, int *clientMagic, char *question, int maxChar, int *encore)
{ 
  static Stack s = 0 ;
  static int nClients = 0 , nTransactions = 0 ;
  KEY option ;
  char *cp ;
  int magic1, magic2, magic3, magicRead, magicWrite ;
  LOOK look ;
  unsigned int command_set = CHOIX_UNDEFINED ;		    /* MUST be unsigned ints because later */
  unsigned int perm_set = PERM_UNDEFINED ;		    /* we do logic on these variable. */


  if (debug) fprintf(out,"ProcessQueries: %s\n", question ? question : "null") ;

  /* DWB - don't let client ask for more than server limit */
  if ((maxKbytes > 0) && (maxChar > (maxKbytes*1024)))
    maxChar = maxKbytes*1024;
/* security system:
   the client is given magic1
   but should later return magic3
   which he contruct by reading the groupreadable file 
   containing magic2
*/
  /* JC1 switch of timeOut alarm , 
     we switch it back on at the end of this routine,
     in the call to checkLife */
  alarm((unsigned int)0);

  s = stackReCreate(s, 20000) ;
  nTransactions++ ;

  if (!*clientIdp)
    { 
      if (refuseNewClient)
	{ pushText (s, "Too late, I am dyyyiiing") ;
	  checkLife(0) ;
	  return s ;
	}

      look = (LOOK) messalloc(sizeof(struct LOOKSTUFF)) ;
      *clientIdp = look->clientId = ++nClients ;
      magic1 = ((int) ((char*)look - (char*)0)) & 0xffffffff ; /* unrelated to rand() */
      magic2 =  randint() ^ ((char*)(&cp) - (char*)0) ;
      magic3 = randint() ^  (int)(timeNow()) ;
      if (magic1 < 0) magic1 *= -1 ;
      if (magic2 < 0) magic2 *= -1 ;
      if (magic3 < 0) magic3 *= -1 ;
      if (!magic1) magic1 = 1 ;       if (!magic2) magic2 = 1 ;      if (!magic3) magic3 = 1 ;
      *clientMagic = magic1 ;
      magicRead  = magic1 * magic2 % 73256171 ;
      magicWrite = magic1 * magic3 % 43532334 ;
      setPassFile (look, magic2, magic3, *clientIdp) ;
      if (debug) fprintf(stderr,"magic1 = %d, magic2 = %d, magic3 = %d, magicRead = %d, magicWrite = %d\n",
			 magic1, magic2, magic3, magicRead, magicWrite) ;
      look->readMagic = look->writeMagic = 0 ;
      if (look->magicReadFileName) look->readMagic = magicRead ;
      if (look->magicWriteFileName) look->writeMagic = magicWrite ;

      look->aceCommand = NULL ;

      cp = (char *)0 + nClients ;
      assInsert (clientAss, cp, look) ;
      fprintf (out, "\nNew Client %d, %d active clients\n", 
	      *clientIdp, ++nActiveClients) ;

      if (look->magicWriteFileName) 
	{ 
	  look->mayWrite = TRUE ;
	  pushText (s, look->magicWriteFileName) ;
	}
      else
	{ 
	  look->mayWrite = FALSE ;
	  pushText (s, "NON_WRITABLE") ;
	}
      catText (s, " ") ;
      if (look->magicReadFileName)
	catText (s, look->magicReadFileName) ;
      else
	catText(s, look->public ? "PUBLIC" : "RESTRICTED") ;

      /* store time of last access. Used for the killing of 
         dead clients */
      look->lastAccess = timeNow() ;

      /* If a user has write access they get to see more commands than if they   */
      /* can only read.                                                          */
#ifndef GIFACESERVER 
      command_set = (CHOIX_UNIVERSAL | CHOIX_RPC) ;
#else  
      command_set = (CHOIX_UNIVERSAL | CHOIX_RPC | CHOIX_GIF) ;
#endif
      if (look->mayWrite)
	perm_set = (PERM_READ | PERM_WRITE | PERM_ADMIN) ;
      else
	perm_set = PERM_READ ;
      look->aceCommand = aceCommandCreate(command_set, perm_set) ;
  
      if (look->mayWrite == FALSE)				    /* belt and braces... */
	aceCommandNoWrite(look->aceCommand) ;

      checkLife(0);
      if (debug) fprintf(stderr, stackText(s,0)) ;
      return s ;
    }     
  else if ((cp = (char *)0 + *clientIdp), 
	   !assFind (clientAss, cp, &look))
    { 
      messout("Aceserver received a bad client id = %d", *clientIdp) ;
      pushText(s, "//! Unauthorised access_1 : closing connection\n") ;
      checkLife(0);
      return 0 ;
    }

  if (debug) fprintf(out,"ProcessQueries passed auth: %s\n", question ? question : "null") ;

  look->lastAccess = timeNow (); /* last access, to kill  dead clients */

  removePassFile (look) ; /* single guess */

  switch(look->public)
    {
    case 0: /* no read_access_dir, you must have write access */
     if ( look->mayWrite && look->writeMagic &&
	  *clientMagic == look->writeMagic)   /* ok, you can have read/write access */
	break ;
     goto nasty ;

    case 1:  /* there is a read_access_dir, you must read it */
      if ( look->mayWrite && look->writeMagic &&
	   *clientMagic == look->writeMagic)   /* ok, you can have read/write access */
	break ;
      if (*clientMagic == look->readMagic)   /* ok, you can have read access */
	 {
	   look->mayWrite = FALSE ;              /* mouse trap */
	   break ;
	 }
      goto nasty ;
      
    case 2: /* read access is public */
      if ( look->mayWrite && look->writeMagic &&
	   *clientMagic == look->writeMagic)		    /* ok, you can write access */
	break ;
      look->mayWrite = FALSE ;				    /* mouse trap */
      look->public = 2 ;
      break ;
    }
  

  /* Process the users command.                                              */
  /*                                                                         */

  /* client sending data via -ace_in option.                                 */
  if (*encore == 3)
    { KEYSET ks ;
      if (!look->mayWrite)
	pushText (s,"// Sorry, you do not have write access.\n") ;
      else
	 { 
	   if (!sessionGainWriteAccess())
	     pushText (s,"// Sorry, you do not have write access.\n") ;
	   else
	     { 
	       ACEIN fi = aceInCreateFromText (question, "", 0);
	       ACEOUT fo = aceOutCreateToStack(s, 0) ;

	       aceInSpecial (fi,"\n/\\\t") ;
	       ks = keySetCreate () ;

	       /* We pass in the request in fi, and get the output back in   */
	       /* fo which we then return to the client. First TRUE means    */
	       /* "keep going" even if there are errors, all errors are      */
	       /* reported back in fo. FALSE means only show brief stats.    */
	       doParseAceIn(fi, fo, ks, TRUE, FALSE) ;		    /* Will destroy fi */

	       pushText (s, messprintf ("// Read %d objects \n", keySetMax (ks))) ;
	       keySetDestroy (ks) ;

	       aceOutDestroy(fo) ;
	     }
	 }
      look->lastAccess = timeNow() ;
      goto fin ;
    }
	 
  fprintf (out, "Client %6d (%d active). Query: %s\n",  *clientIdp, nActiveClients, question) ;
  if (strncmp(question, "wspec",5) != 0)
    {
      ACEIN command_in;
      ACEOUT result_out;
      struct messContextStruct newMessContext;
      struct helpContextStruct newHelpContext;
      struct messContextStruct oldMessContext;
      struct helpContextStruct oldHelpContext;

      command_in = aceInCreateFromText (question, "", 0);
      /* forbid sub shells & cmd files, allow param */
      aceInSpecial (command_in, "\n\t\\/%") ; 

      result_out = aceOutCreateToStack (s, 0);

      /* register output context for messout, messerror and helpOn
       * for the duration of aceCommandExecute */
      newMessContext.user_func = acedbPrintMessage;
      newMessContext.user_pointer = (void*)result_out;
      newHelpContext.user_func = helpPrint;
      newHelpContext.user_pointer = (void*)result_out;
      
      oldMessContext = messOutRegister (newMessContext);
      messErrorRegister (newMessContext);
      oldHelpContext = helpOnRegister (newHelpContext);

      /*************/
      option = aceCommandExecute (look->aceCommand, 
				  command_in, result_out, 
				  *encore ? look->lastCommand : 0, maxChar) ;
      /************/

      /* revert back to original registers */
      messOutRegister (oldMessContext);
      messErrorRegister (oldMessContext);
      helpOnRegister (oldHelpContext);



      *encore = 0 ;
      look->lastAccess = timeNow() ;
      
      switch (option)
	{
	case 'q':
	  clientDestroy(look) ;
	  *clientIdp = 0 ;
	  pushText(s, "// A bientot") ;
	  break ;
	  
	case 'm':
	case 'D':
	case 'B':
	case 'M':
	  look->lastCommand = option ;
	  *encore = 2 ;  /* automatic looping */
	  break ;
	  
	case CMD_WHO:  /* who */
	  if (!look->mayWrite)
	    {
	      pushText (s,"// Sorry, you do not have write access.\n") ;
	      break ;
	    }
	  pushText(s,messprintf("%d active Clients, %d transactions so far",
				nActiveClients, nTransactions)) ;
	  break ;
	  
	case CMD_SHUTDOWN:  /* shUtdown [now] */
	  if (!look->mayWrite)
	    { 
	      pushText (s,"// Sorry, you do not have write access.\n") ;
	      break ;
	    }
	  refuseNewClient = TRUE ;
	  cp = aceInWord (command_in);
	  if (cp && strcasecmp(cp, "now") == 0)
	    wormClose () ;
	  else
	    pushText (s,"// The server will now refuse any new connection \n") ;
            catText (s, "// and shutdown when the last client quits\n") ;
	    catText(s,messprintf("// Now %d active Client(s)\n",
				nActiveClients)) ;

	  break ;
	} /* end-switch option */

      aceInDestroy (command_in);
      aceOutDestroy (result_out);
    }
  else if (!strcmp (question, "wspec1"))
    {  /* export wspec 
	  does not work, i do not know how to untar at the client side
      catBinary (s, arrp (wspecTar, 0, char), arrayMax(wspecTar)) ;
      */
    }
  else if (!strcmp (question, "wspec"))
    {  /* export wspec */
      catBinary (s, arrp (wspecTarEncoded, 0, char), arrayMax(wspecTarEncoded)) ;
    }

/*  if (oldAnswer != stackText(s, 0))
    messout("Relocation: length(answer) = %d",  stackMark(s)) ;
    oldAnswer = stackText(s, 0) ;

*/
  
fin:
  checkLife(0);
  return s ;
  
nasty:
  fprintf (out,
	   "Unauthorised access by client %d, magic = %d != look->writeMagic = %d, look->readMagic = %d\n",
	   *clientIdp, *clientMagic, look->writeMagic, look->readMagic) ;
  clientDestroy (look) ;
  *clientIdp = 0 ; 
  pushText(s, "//! Unauthorised access_2: closing connection\n") ;
  goto fin ;
}

/**************************************************/
/********* ACEDB non graphic Query Server  ********/
/**************************************************/

static void copy_wspec (BOOL encode)
{
  FILE *f = 0 ;
  char c, *cp ;
  int n = 0 ;
  Array aa = arrayCreate (20000, char) ; 

  cp = dbPathStrictFilName("wspec", "", 0 ,"rd", 0) ;
  if (!cp) 
    messcrash ("Server cannot open wspec, sorry") ;
  
  printf("\n");/*freeOut("\n") ;*/

  if (encode)
    f = popen(messprintf("cd %s ; tar chf - %s %s | uuencode server.wspec.tar",
			 cp, "cachesize.wrm cachesize.wrm database.wrm displays.wrm ../wgf",
			 "layout.wrm models.wrm options.wrm psfonts.wrm subclasses.wrm xfonts.wrm"),"r") ;
  else
    f = popen(messprintf("cd %s ; tar chf - %s %s",
			 cp, "cachesize.wrm cachesize.wrm database.wrm displays.wrm ../wgf",
			 "layout.wrm models.wrm options.wrm psfonts.wrm subclasses.wrm xfonts.wrm"),"r") ;
  while (c = getc(f), c  != (char)EOF)
    array (aa, n++, char) = c ;
  pclose (f) ;
  if (encode)
    wspecTarEncoded = aa ;
  else
    wspecTar = aa ;

  messfree(cp);
}



static void daemonize(void)
{
#ifdef JUNK 
  /* on alpha htis creates an infinity of server process, bad ! */
 int pid ; /* detach from terminal */

  pid = fork() ;
 if (pid < 0)
   exit (0) ; /* messcrash("Cannot fork") ; */
 if (pid > 0)
   exit (0) ; /* parent suicides */
#endif

 setsid () ;   /* reset neutral environment */
 chdir ("/") ;
 umask (0) ;
}

static void openOutFile (void)
{ char *fileName = dbPathMakeFilName ("", "server", "log", 0) ;
 
 out = filopen (fileName, 0, "a") ;
 if (!out)
   out = filopen (fileName, 0, "w") ;
 if (!out)
   out = stderr ;
 setbuf (out, NULL) ;
 /* setbuf (stdout, NULL) ; this flat crashes under daemon on new linux */
 messfree(fileName);   
}


/* Defines a routine to return the compile date of this file. */
UT_MAKE_GETCOMPILEDATEROUTINE()

/************************************************************/

int main (int argc, char **argv)
{
  BOOL nosigcatch ;
  u_long p ;
  int n, t ;
  char *cp, *dir ;
  /* Declarations for inetd detection LDS 7/2/98 */
  struct sockaddr_in saddr;
  int asize = sizeof (saddr);


  /* Does not return if user typed "-version" option.                    
     checkForCmdlineVersion(&argc, argv) ;
  */

  /* Check for "no signal catching" option.                                  */
  nosigcatch = getCmdLineOption(&argc, argv, NOSIGCATCH_OPT, NULL) ;


#ifdef GIFACESERVER
  gifEntry = gifControl ; 

  freeinit () ;			/* must come before graphInit */

  /* Initialise the graphics package and its interface for acedb.            */
  graphInit (&argc, argv);
  acedbGraphInit() ; 


  /* init message system details.                                            */
  messSetMsgInfo("gifaceserver", aceGetVersionString()) ;
#else
  messSetMsgInfo("aceserver", aceGetVersionString()) ;
#endif



  out = stderr ; /* this static cannot be initialized at file scope in WIN32? */

  /* This code determines whether we are running from inetd.
     It relies on the fact that if we are running under inetd,
     file descriptor 0 will be a socket.
     LDS 7/2/98 */
  if (getsockname(0, (struct sockaddr *)&saddr, &asize) == 0)
    isDaemon = TRUE ;
  else
    isDaemon = FALSE ;

#ifdef SGI
  /* in sgi case, i call rpc_init etc with diferent param (zero)
   * in the daemon case, 
   */
  isSGI = TRUE ;
#endif

  n = 2;
  if (argc > n && (!strcmp(argv[n],"-port")))
    { /* ignore, optional -port */
      n++;
    }
  if (argc > n && (sscanf(argv[n],"%lu",&p) == 1))
    { port = p;
      n++;
    }
  if (argc > n)
    {
      cp = argv[n] ;
      if (sscanf(cp,"%d",&t) == 1)
	{ 
	  clientTimeOut = t ;
	  if ((cp = strstr(argv[n],":")) &&
	      sscanf(++cp,"%d",&t) == 1)
	    { serverTimeOut = t;
	    if ((cp = strstr(cp,":")) &&
		sscanf(++cp,"%d",&t) == 1)
	      { maxKbytes = t;
	      if ((cp = strstr(cp,":")) &&
		  sscanf(++cp,"%d",&t) == 1)
		autoSaveInterval = t;
	      }
	    }
	  n++;
	}
    }
  if (argc < 2 || argc > n)
    { fprintf (out, usage) ;
      fprintf (stderr, usage) ;
      exit (EXIT_FAILURE) ;
    }

  aceInit (argc>1 ? argv[1]: 0) ;  /* Chooses the database directory */

  /* once the database is open it is important to deal with signals */
  signalCatchInit(TRUE,					    /* init Ctrl-C as well */
 		  nosigcatch) ;				    /* catch all signals. */

#ifdef GIFACESERVER
  displayInit();	     /* init acedb displays,
				parse wspec/displays.wrm etc,
				comes after aceInit (it uses the db) */
#endif

  /*  isInteractive = TRUE ;*/
  clientAss  = assCreate() ;
  signal (SIGALRM, checkLife) ;
  /* start timer countDown */
  if (serverTimeOut)
    alarm((unsigned int)serverTimeOut);

  openOutFile() ;
  cp = getenv ("HOST") ; if (!cp) cp = "(unknown)" ;
  fprintf(out,"\n\n#### Server starts %s\n", timeShow(timeNow())) ;
  fprintf(out,"#### host=%s  port=%lu  ACEDB=%s\n",  
	  cp, port, dir = dbPathMakeFilName("","","",0)) ;
  messfree(dir);
  fprintf(out,"#### clientTimeout=%d serverTimeout=%d maxKbytes=%d autoSaveInterval=%d\n",
	  clientTimeOut, serverTimeOut, maxKbytes, autoSaveInterval); 

#if !defined(DCESERVER)
  signal (SIGPIPE, sigPipeError) ;
#endif

  copy_wspec (TRUE) ;
  /*
    i do not know how to reconstruct the non encoded file at the 
    other hand, sorry copy_wspec (FALSE) ;
    */
  daemonize() ;
  
  wait_for_client(port, isSGI && isDaemon) ;
  messcrash("Acedb network server error: main loop returned") ;

  return(EXIT_FAILURE) ;	/* We should not reach here... */
}


/***************************************************************************/


#ifdef GIFACESERVER
/* Stubs to make gifaceserver link. Once the display code has been enhanced to
   allow provide interactive and display-only sections of data displays
   this should go. */
Graph dotter ()
{
  return 0;
}

Graph blxview()
{ 
  return 0;
}

char *gexTextEditor()
{
  return NULL;
}
#endif

void *gexTextEditorNew()
{
  return NULL;
}

char *gexEditorGetText()
{
  return NULL;
}


/************************ eof *******************************/

/**************************************************/
/**************************************************/
