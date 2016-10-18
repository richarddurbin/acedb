/*  File: sigsubs.c
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
 * Description: signal catching, initialisation etc.
 *              currently not tested for Windows or Mac
 * Exported functions:
 *              signalCatchInit()
 * HISTORY:
 * Last edited: Mar  9 12:56 2009 (edgrif)
 * Created: Fri Apr 30 13:24:42 1999 (fw)
 *-------------------------------------------------------------------
 */

#if defined(WIN32) || defined(MACINTOSH)

/* Just a trivial stub, who knows if these systems actually do signals...    */
void signalCatchInit (BOOL initCtrlC)
{
  return ;
}


#else /* i.e. UNIX */

/************************************************************/
#include <sys/signal.h>
#include <termios.h>
#if defined(ALPHA) || defined(LINUX) || defined(OPTERON) || defined(SOLARIS)
#include <sys/resource.h>				    /* for user stack size check. */
#endif

#include <glib.h>

#include <wh/acedb.h>
#include <wh/session.h>
#include <wh/sigsubs.h>

/************************************************************/

static void signalSet(SignalHandler signal_handler, BOOL initCtrlC, char *fail_msg) ;
static void signalHandler(int sig) ;
static char *getSignalText(int sig_num) ;


/************************************************************/

/* By default signal catching is on, we need to remember whether its on or   */
/* off for menu purposes.                                                    */
static BOOL sig_catching_G = TRUE ;

static char *panicBuffer = 0;


/************************************************************/

/* Initialise signal handling, set up the signal mask and the handler.       */
/* For windowing apps (e.g. xace) we don't want Cntl-C set up here, we       */
/* handle it elsewhere via the windowing code.                               */
/*                                                                           */
void signalCatchInit(BOOL initCtrlC, BOOL no_catch)
{
  panicBuffer = malloc(5*1024);

  /* N.b. we don't need to do anything if signals are not going to be caught.*/
  if (no_catch)
    sig_catching_G = FALSE ;
  else
    {
      signalSet(signalHandler, initCtrlC, "Signal handler could not be turned on") ;
      sig_catching_G = TRUE ;
    }

  return;
} /* signalCatchInit */


/************************************************************/
/* Simply return current status of signal catching.                          */

BOOL signalCatchStatus(void)
{
  return sig_catching_G ;
}



/************************************************************/

/* Turn on all signal catching, read/write locks will be cleared up for most */
/* signals.                                                                  */
/*                                                                           */
void signalCatchOn(BOOL initCtrlC)
{
  signalSet(signalHandler, initCtrlC, "Signal handler could not be turned off") ;
  sig_catching_G = TRUE ;
  
  return;
}


/************************************************************/

/* Turn off all signal catching, the time we need this is if we have a       */
/* persistent problem such as seg faulting and need a dump.                  */
/* NOTE that once signals are turned off then if the program exits, the      */
/* readlocks will not be cleared up.                                         */
/*                                                                           */
void signalCatchOff(BOOL initCtrlC)
{
  signalSet(SIG_DFL, initCtrlC, "Signal handler could not be turned off") ;
  sig_catching_G = FALSE ;
  
  return;
}


/************************************************************/


/* Set signal handling on/off, if  signal_handler == SIG_DFL  then signal    */
/* handling will in effect be turned off by returning it to the default.     */
/*                                                                           */
/* NOTE:                                                                     */
/*   There are some signals that its good to try and clean up after such as  */
/*   SIGTERM, in our signal handler we try to shut down the database         */
/*   correctly.                                                              */
/*                                                                           */
/*   There are other signals after which NO CLEAN UP should be attempted and */
/*   we should NOT try to catch, the classic example is SIGABRT. If we get   */
/*   sent SIGABRT, it is because the user WANTS a core dump of where the     */
/*   process is NOW...not where it is after we try to clean up !!!!!!!       */
/*                                                                           */
static void signalSet(SignalHandler signal_handler, BOOL initCtrlC, char *fail_msg)
{
  struct sigaction sa;

  signalInitSigAction(&sa, signal_handler) ;

  /* signals that cause the program to terminate gracefully */
  if (sigaction(SIGHUP, &sa, 0) != 0 ||		/* hangup, generated when terminal disconnects */
      sigaction(SIGTERM, &sa, 0) != 0 ||	/* software termination signal */
      sigaction(SIGBUS, &sa, 0) != 0 ||		/* (*) bus error (specification exception) */
      sigaction(SIGFPE, &sa, 0) != 0 ||		/* (*) floating point exception */
      sigaction(SIGSEGV, &sa, 0) != 0 ||	/* (*) segmentation violation */
    /* interrupt signals */
      sigaction(SIGQUIT, &sa, 0) != 0)		/* Ctrl-\ */
    {
      messerror ("%s (%s)",
		 fail_msg, messSysErrorText());
    }

  /* do this seperately as graphical applications deal with Ctrl-C
   * in a special way */
  if (initCtrlC)
    if (sigaction(SIGINT, &sa, 0) != 0)		/* Ctrl-C */
      messerror ("%s (%s)",
		 fail_msg, messSysErrorText());
  
  return;
} /* signalCatchInit */


/************************************************************/

/* Our signal handling function that attempts to clean up after a signal.    */
/*                                                                           */
static void signalHandler (int sig)
{
  struct messContextStruct nullContext = { NULL, NULL };

  if (sig == SIGINT)	/* usually caused by Ctrl-C */
    {
      BOOL result = TRUE;


      /* I don't know why Fred did all this stuff with the terminal, it      */
      /* actually makes the interface between this code and for instance     */
      /* the tace prompt code a bit yuch. Things don't behave as you might   */
      /* expect. I have improved it a bit, but I'm not sure why we couldn't  */
      /* just use readline here, or perhaps fgets() and fputs()              */
      /* sometime I'll try them.                                             */

      if (isatty(fileno(stdin)) > 0)	/* makes sure stdin that isatty() */
	{
	  struct termios term, copy;
	  int c;
	  
	  if (tcgetattr(fileno(stdin), &term) < 0)
	    messcrash("signalHandler() - can't get STDIN attributes");

	  /* set terminal mode to non-canonical - this means
	   * "read requests are satisfied directly from the input queue.
	   *  A read does not return until at least MIN bytes have been 
	   *  received or the timeout value TIME has expired between bytes."
	   * from "Advanced Programming in the UNIX Environment" 
	   * by W. Richard Stevens (c) 1993, page 339 */
	  copy = term;

	  /* switch to raw tty mode */

	  copy.c_lflag &= ~(ICANON | IEXTEN | ISIG | NOFLSH);
				/* canonical mode off,
				 * extended input processing off,
				 * signal chars off */
	  copy.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
				/* no SIGINT on BREAK,
				 * CR-to-NL off,
				 * input parity check off,
				 * don't strip 8th bit on input
				 * output flow control off */
	  copy.c_cflag &= ~(CSIZE | PARENB);
				/* clear size bits,
				 * parity checking off */
	  copy.c_cflag |= CS8;	/* set 8 bits per char */
#if defined(DARWIN)
	  copy.c_oflag &= ~(OPOST);
#else
	  copy.c_oflag &= ~(OPOST| ONOCR);
#endif
				/* output processing off,
				 * so we don't get a RETURN char after 
				 * every input, which keeps the STDIN
				 * input stream free */
	  copy.c_cc[VMIN] = 1;
	  copy.c_cc[VTIME] = 0;


	  if (tcsetattr(fileno(stdin), TCSAFLUSH, &copy) < 0)
	    messcrash("signalHandler() - can't set STDIN "
		      "attributes to raw tty-mode");

	  fprintf (stdout, "Do you want to exit the program? (y or n) ");
	  fflush (stdout);

	  /*	  read(fileno(stdin), &c, 1);*/
	  c = getc(stdin);

	  if ((char)c == 'y' || (char)c == 'Y')
	    result = TRUE;
	  else
	    result = FALSE;

	  /* set terminal mode back to what it was */
	  if (tcsetattr(fileno(stdin), TCSAFLUSH, &term) < 0)
	    messcrash("signalHandler() - can't set STDIN "
		      "attributes back to original mode");
	  
	  clearerr (stdin);	/* clear EOF on stdin */

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
	  /* I think this screws up the terminal:   if the user now          */
	  /* types Cntl-C again then this doesn't work as it should, we      */
	  /* don't see the 'y', that's for sure, probably because there is   */
	  /* a spare ' ' lying around.                                       */
	  ungetc (' ', stdin);
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

	  /* There is a problem here, we need to emit a newline otherwise    */
	  /* subsequent error messages etc. get mangled with the above exit  */
	  /* message. But note that if the user entered 'n', then they still */
	  /* have to press "return" before they get the acedb prompt back,   */
	  /* I'm not really sure why...                                      */
	  if (result)
	    {
	      fprintf (stdout, "\n") ;
	      fflush (stdout);
	    }

	}

      if (result) 
	messExit ("User initiated abort");

      /* else - just continue program execution */
    }
  else if (sig == SIGQUIT)	/* caused by Ctrl-\ */
    {
      messExitRegister(nullContext) ;
      messExit("User initiated abort") ;
    }
  else if (sig == SIGTERM)	/* caused by kill command */
    {
      ServerSessionCB serverCBs = NULL ;

      if ((serverCBs = sessionGetServerCB()) != NULL)
	{
	  serverCBs->sigterm.funcs.func(serverCBs->sigterm.data) ;
	}
      else
	{
	  messExitRegister(nullContext) ;
	  messExit("Program killed...") ;
	}
    }
  else if (sig == SIGHUP)	/* hang up */
    {
      ServerSessionCB serverCBs = NULL ;

      if ((serverCBs = sessionGetServerCB()) != NULL)
	{
	  serverCBs->sighup.funcs.func(serverCBs->sighup.data) ;
	}
      else
	{
	  messExitRegister(nullContext);
	  messExit(messprintf("ABORT: Signal %d caught", sig));
	}
    }
  else				/* other, more serious, signals that we catch */
    {
      char *user;
      char *signal_text = NULL ;

      /* I don't understand why we don't just try to do the normal clean up  */
      /* by calling messcrash, the below just confuses things by having yet  */
      /* another clean up routine. This is silly....                         */

      if (panicBuffer)		/* try to reclaim some memory
				 * for the cleanup procedure */
	{
	  free (panicBuffer);
	  panicBuffer = 0;
	}
      
      writeAccessChangeUnRegister(); /* don't do anything fancy anymore */
      sessionReleaseWriteAccess();
      readlockDeleteFile ();
      
      user = getLogin(TRUE) ;

      signal_text = getSignalText(sig) ;
      
      messdump ("ABORT : Fatal error, received signal %d, \"%s\" from operating system. (%s %s %s)",
		sig, signal_text,
		aceGetVersionString(), aceGetLinkDateString(), user);

      fprintf (stderr,
	       "\n// ABORT : Fatal error, program received signal %d, \"%s\", "
	       "from operating system. (%s %s %s)\n"
	       "//\n", sig, signal_text,
	       aceGetVersionString(), aceGetLinkDateString(), user) ;

#if defined(RLIMIT_DATA)
      /* DEC at least delivers a SIGSEGV if stack space is exhausted, but    */
      /* other systems may use a different one, so execute for all signals.  */
      {
	struct rlimit limit ;
	enum {MIN_STACK_SIZE = 1572864} ;

	/* Warning generally has proved unhelpful, so I'm only doing it for  */
	/* really low stack limits from now on...                            */
	/* We look at RLIMIT_STACK to see if users stack size can be raised, */
	/* and warn them.                                                    */
	/* To be really fancy we could call getrusage to look at actual      */
	/* stack usage, but this might not be portable since some signal     */
	/* handlers are not placed on the normal stack.                      */
	if (getrlimit(RLIMIT_STACK, &limit) != 0)
	  {
	    fprintf(stderr, "\n// Fatal error in %s at line %d:\n"
		    "// call to getrlimit() failed.", __FILE__, __LINE__) ;
	    exit(EXIT_FAILURE) ;
	  }
	else
 	  {
	    if (limit.rlim_cur < MIN_STACK_SIZE)
	      {
		fprintf(stderr,
			"// A possible cause of the error is that your stack size"
			" (currently %lu bytes)\n"
			"// is very low, you could raise it to %lu bytes%s using\n"
			"// the limit/unlimit or ulimit shell commands and then rerun %s.\n"
			"//\n",
			limit.rlim_cur, limit.rlim_max,
			limit.rlim_max == RLIM_INFINITY ? " (max. possible)" : "",
			messGetErrorProgram()) ;

		/* This fprintf() and the next one are alternative starts to */
		/* the beginning of the main fprintf() below.                */
		fprintf(stderr,	"// If this fails, then this ") ;
	      }
	    else
	      fprintf(stderr,	"// This ") ;
	  }
      }
#else
      fprintf(stderr,	"// This ") ;
#endif

      /* continues from last fprintfs in the section above.                  */
      fprintf (stderr,
	       "problem _may_ be caused by an error in the code and\n"
	       "//  the Acedb developers would like to know about it.\n"
	       "//\n"
	       "// Please email as full report as possible to acedb@sanger.ac.uk\n"
	       "//\n"
	       "// Try to include a description of your database and\n"
	       "//  details of the actions you were trying to perform\n"
	       "//  when this error occured. \n"
	       "//  Details on the input data that was used and \n"
	       "//  information about the operating system of your computer\n"
	       "//  may also help to analyse this problem.\n"
	       "//\n");


#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
      /* Theres a glib function to print a stack trace but its not that good,*/
      /* function just hangs after printing the stack trace or if            */
      /* it can't fork gdb, it would need to be recoded to use sigalrm and   */
      /* maybe sigchld so that it exitted rather then hanging as it does now.*/
      /* Not worth the hassle currently.                                     */
      fprintf (stderr,
	       "// The program will now try to print a record of which routine failed.\n"
	       "// If should stop with a line that reads:\n"
	       "//      \"<some text> in main (argc=2, argv=0x11fffd728) <more text>\"\n") :
      g_on_error_stack_trace(messGetErrorProgram()) ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

      /* We used to call abort() here to try to get a core file but this     */
      /* simply doesn't work on many systems as the signal handling is on a  */
      /* separate stack and this makes the core useless. Better is to get    */
      /* tell the user to turn off signal handling and repeat the problem    */
      /* to get a pucker dump.                                               */
      fprintf (stderr,
	       "// If you can, restart the program using the \"-nosigcatch\" option,\n"
	       "// e.g. \"tace -nosigcatch <database>\", and reproduce the problem.\n"
	       "// This will produce a \"core\" file in your current directory,\n"
	       "// please keep it as the developers can use it for debugging.\n"
	       "//\n\n");

      exit(EXIT_FAILURE) ;
    }
  
  return;
} /* signalHandler */


/* Getting hold of the text corresponding to a signal number is highly non-portable.
 * but Glib has come to rescue and we use their routine now. */
static char *getSignalText(int sig_num)
{
  char *sig_text ;

  sig_text = (char *)g_strsignal(sig_num) ;

  return sig_text ;
}


/************************************************************/

/* Some small signal utilities.                                              */

/* Initialise a sigaction structure with a signal handler and an empty mask. */
/*                                                                           */
void signalInitSigAction(struct sigaction *sigact, SignalHandler sig_func)
{
  sigact->sa_handler = sig_func ;
  sigemptyset(&(sigact->sa_mask)) ;
  sigact->sa_flags = 0 ;

  return ;
}


/************************************************************/

/* Disable signal catching by our signal handler for a specific signal, you  */
/* might want to do this prior to issuing an abort() in a clean up routine   */
/* to avoid recursing.                                                       */
/* N.B. even thought the sigNNN calls made here can only fail with EINVAL    */
/* implying this routine is in error, we don't crash, this is because we     */
/* could be called from a clean up routine which was called because there    */
/* has already been been a crash.                                            */
/*                                                                           */
BOOL signalSetNoCatch(int sig)
{
  BOOL result = TRUE ;
  struct sigaction sigact ;

  /* Create a new signal set with all signals set to the default action,     */
  /* this will ensure we do not catch the signal.                            */
  signalInitSigAction(&sigact, SIG_DFL) ;

  /* Add the callers signal to the set of signals blocked while processing   */
  /* the callers signal. (I DON'T THINK WE NEED TO DO THIS...)               */
  if (result && sigaddset(&(sigact.sa_mask), sig) < 0)
      result = FALSE ;
  
  /* Install the action for the callers signal, this will mean that the      */
  /* operating system will carry out the default action for this signal,     */
  /* the application will not catch it.                                      */
  if (result && sigaction(sig, &sigact, NULL) < 0)
      result = FALSE ;

  return result ;
}


#endif /* Unix */



