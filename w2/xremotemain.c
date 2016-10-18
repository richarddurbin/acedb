/*  File: xremotemain.c
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
 * Description:
 * Exported functions:
 * HISTORY:
 * Last edited: Jun 23 17:43 1999 (fw)
 * * Mar 22 14:43 1999 (edgrif): Replaced messErrorInit with messSetMsgInfo
 * Created: Wed Jan 27 10:37:03 1999 (fw)
 *-------------------------------------------------------------------
 */


#include <stdio.h>
#include <string.h>
#include <stdlib.h>

/* vroot.h is a header file which lets a client get along with 
   `virtual root' window managers like swm, tvtwm, olvwm, etc.
   If you don't have this header file, you can find it at
   "http://home.netscape.com/newsref/std/vroot.h".
   If you don't care about supporting virtual root window managers,
   you can comment this line out.
 */
#include <w2/vroot.h>

/**********************************/

#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/Xmu/WinUtil.h>	/* for XmuClientWindow() */


/************************************************************/
static int xtRemoteCommands (Display *dpy, Window window, char **commands);
static Window xtRemoteFindWindow (char *applicationName, Display *dpy);
static int xtRemoteCheckWindow (Display *dpy, Window window);
static void xtRemoteInitAtoms (Display *dpy); 
/************************************************************/
Atom XA_XREMOTE_VERSION  = 0;
Atom XA_XREMOTE_COMMAND  = 0;
Atom XA_XREMOTE_RESPONSE = 0;
Atom XA_XREMOTE_APPLICATION = 0;
/************************************************************/
#define XREMOTE_VERSION_PROP   "_XREMOTE_VERSION"
#define XREMOTE_COMMAND_PROP   "_XREMOTE_COMMAND"
#define XREMOTE_RESPONSE_PROP  "_XREMOTE_RESPONSE"
#define XREMOTE_APPLICATION_PROP      "_XREMOTE_APPLICATION"

/* this string is used to tell apart different versions of the
 * XAtom-based remote control meachnism, just incase the protocol   
 * changes. This should not happen as it is far too general and
 * disconnected from the funtionality the specific application
 * provides for remote-controllability .
 */
#define XREMOTE_EXPECTED_VERSION_STR "1.0"

/* server-side errors */
#define XREMOTE_S_ERROR_PREFIX "X-Atom remote control : "

#define XREMOTE_S_ERROR_UNABLE_TO_READ_PROPERTY  XREMOTE_S_ERROR_PREFIX"unable to read %s property"
#define XREMOTE_S_ERROR_INVALID_DATA_ON_PROPERTY XREMOTE_S_ERROR_PREFIX"invalid data on %s of window 0x%x"


/* strings written to the XREMOTE_RESPONSE_PROP atom by the server */
#define XREMOTE_S_200_COMMAND_EXECUTED "200 command executed %s"
#define XREMOTE_S_500_COMMAND_UNPARSABLE "500 unparsable command %s"
#define XREMOTE_S_501_COMMAND_UNRECOGNIZED "501 unrecognized command %s"
#define XREMOTE_S_502_COMMAND_NO_WINDOW "502 no appropriate window for %s"

/************************************************************/

static void usage (char *progname)
{
  fprintf (stderr, "usage: %s [ options ... ]\n\
       where options include:\n\
\n\
       -help                     to show this message.\n\
       -display <dpy>            to specify the X server to use.\n\
       -app <progname>           the name of the application to which the -remote\n\
       -remote <remote-command>  to execute a command in an already-running\n\
                                 process.  See the manual for a\n\
                                 list of valid commands.\n\
                                 commands should be sent; required if -id is not used\n\
       -id <window-id>           the id of an X window to which the -remote\n\
                                 commands should be sent; if unspecified,\n\
                                 but -app is set, the first window found for\n\
                                 <progname> will be used.\n\
       -raise                    whether following -remote commands should\n\
                                 cause the window to raise itself to the top\n\
                                 (this is the default.)\n\
       -noraise                  the opposite of -raise: following -remote\n\
                                 commands will not auto-raise the window.\n\
",
	   progname);
}


int main (int argc, char **argv)
     /* response_status code -
	0 - everything OK
	1 - display error
	2 - wrong window specified or application not running
	6 - specified window is not a graphPackage window
     */
     
     /* this function is responsible for distpatching the commands
      * passed by the argv array.
      * This implementation parses out command line options
      * to set the "display to use " and the "window to activate"
      * and then passes the commands on to xtRemoteCommand(),
      * where they are written to X-atoms that trigger response
      * on the server-end */
{
  int response_status = 0;
  int i;
  char *display_str = 0;
  Display *dpy;
  Window window = 0;
  unsigned long window_id = 0;
  char *applicationName = 0;
  char *progname;	

  progname = strrchr (argv[0], '/');
  if (progname)
    progname++;
  else
    progname = argv[0];

  for (i = 1; i < argc; i++)
    { 
      if (!strcasecmp (argv [i], "-h") ||
	  !strcasecmp (argv [i], "-help"))
	{
	  usage (progname);
	  exit (0);
	}
      
      if (!strcmp(argv[i],"-app"))
	{
	  if ((argc - i) < 2)
	    {
	      /* no argument to -app option */
	      fprintf (stderr, "%s: invalid `-app' option \"%s\"\n",
		       progname, argv[i] ? argv[i] : "");
	      usage (progname);
	      exit (-1);
	    }
	  else if (applicationName != 0)
	    {
	      fprintf (stderr,
		       "%s: only one `-app' option may be used.\n",
		       progname);
	      usage (progname);
	      exit (-1);
	    }
	  
	  applicationName = argv[i+1];
	  
	  /* clear both arguments from the list */
	  for(i+=2;i<argc;i++)
	    argv[i-2] = argv[i];
	  argv[argc-2] = 0;
	  argc -= 2;
        }
    }
  
  if (getenv("DISPLAY"))
    {
      display_str = (char*)malloc (strlen(getenv("DISPLAY")) + 1);
      strcpy (display_str, getenv("DISPLAY"));
    }

  for (i = 1; i < argc; i++)
    if (strcmp(argv[i], "-display") == 0)
      {
	if (display_str) free (display_str);
	display_str = (char*)malloc (strlen(argv[i+1]) + 1);
	strcpy (display_str, argv[i+1]);
	
	for(i += 2; i < argc; i++)
	  argv[i-2] = argv[i];
	argv[argc-2] = 0;
	(argc) -= 2;
	break;
      }
  
  dpy = XOpenDisplay (display_str);

  if (display_str) free (display_str);

  if(!dpy)
    return 1;			/* code 1 = no display */

  xtRemoteInitAtoms(dpy);

  /* look for the -id option */
  for (i = 1; i < argc; i++)
    if (strcmp(argv[i], "-id") == 0)
      {
	char c;
	
	if (argv[i+1] &&
	    1 == sscanf (argv[i+1], " %ld %c", &window_id, &c))
	  ;
	else if (argv[i+1] &&
		 1 == sscanf (argv[i+1], " 0x%lx %c", &window_id, &c))
	  ;
	else
	  return (-1); /* code -1 = bad command line */

	for(i += 2; i < argc; i++)
	  argv[i-2] = argv[i];
	argv[argc-2] = 0;
	argc -= 2;
	break;
      }
  
  window = (Window)window_id;
  
  if (window)
    {
      int response_code = xtRemoteCheckWindow (dpy, window);

      if (response_code != 0)
	return response_code;
    }
  else
    {
      /* didn't specify one, so find the first one for the application */
      window = xtRemoteFindWindow(applicationName, dpy);
	  
      if (!window)
	return 2;		/* code 2 = no window found */
    }

  {
    char **commands = (char**)malloc(argc * sizeof(char*));
    int isRemoteCommand = False;
    int remoteArgPos = 0, remoteArgNum = 0, j;

    /* zero the commands array */
    for (i = 0; i < argc; i++)
      commands[i] = NULL;

    /* search through arguments look for remote commands */
    for (i = 1; i < argc; i++)
      {
	/* list of command starts with -remote ... */
	if (strcmp(argv[i], "-remote") == 0)
	  {
	    isRemoteCommand = True;
	    remoteArgPos = i;
	    remoteArgNum++;
	  }
	else
	  {
	    if (!isRemoteCommand)
		continue;
	    
	    /* list of remote-commands ends with any other option */
	    if (*argv[i] == '-')
	      break;
	    
	    commands[remoteArgNum-1] = (char*)malloc(strlen(argv[i])+1);
	    strcpy (commands[remoteArgNum-1], argv[i]);

	    remoteArgNum++;
	  }
      }
	
    if (remoteArgNum > 0)
      {
	for (j = remoteArgPos + i; j < argc; j++)
	  argv[j-remoteArgNum] = argv[j];
	argv[argc-remoteArgNum] = 0;
	argc -= remoteArgNum;
      }


    /* should return 0 if everyting OK */
    response_status = xtRemoteCommands(dpy, window, commands);

    for (i = 0; i < argc; i++)
      if (commands[i]) 
	free (commands[i]);
    free (commands);
  }

  return response_status;
} /* main */

/************************************************************/

/* some of the functions below are based on the original file 'remote.c'
 * which carried this notice : */
/*****************************************************************************/
/* -*- Mode:C; tab-width: 8 -*-
 * remote.c --- remote control of Netscape Navigator for Unix.
 * version 1.1.3, for Netscape Navigator 1.1 and newer.
 *
 * Copyright (c) 1996 Netscape Communications Corporation, all rights reserved.
 * Created: Jamie Zawinski <jwz@netscape.com>, 24-Dec-94.
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation.  No representations are made about the suitability of this
 * software for any purpose.  It is provided "as is" without express or 
 * implied warranty.
 *
 *************
 * NOTE:
 * the file has been modified, to use a different protocol from the one
 * described in http://home.netscape.com/newsref/std/x-remote.html
 *************
 */

/* based on mozilla_remote_init_atoms() */
static void xtRemoteInitAtoms (Display *dpy)
{
  if (! XA_XREMOTE_VERSION)
    XA_XREMOTE_VERSION = XInternAtom (dpy, XREMOTE_VERSION_PROP, False);
  if (! XA_XREMOTE_COMMAND)
    XA_XREMOTE_COMMAND = XInternAtom (dpy, XREMOTE_COMMAND_PROP, False);
  if (! XA_XREMOTE_RESPONSE)
    XA_XREMOTE_RESPONSE = XInternAtom (dpy, XREMOTE_RESPONSE_PROP, False);
  if (! XA_XREMOTE_APPLICATION)
    XA_XREMOTE_APPLICATION = XInternAtom (dpy, XREMOTE_APPLICATION_PROP, False);
} /* xtRemoteInitAtoms */


/* based on mozilla_remote_find_window() */
static Window xtRemoteFindWindow (char *applicationName, 
				  Display *dpy)
     /* the client remote control driver has specified the
	application and we need to find a window of that
	application so we can send the commands there */
     /* if `applicationName' is NULL the first window
      * with the remote control atoms will be returned,
      * otherwise we're looking for a window created by the said
      * application */
{
  int i;
  Window root = RootWindowOfScreen (DefaultScreenOfDisplay (dpy));
  Window root2, parent, *kids;
  unsigned int nkids;
  Window result = 0;
  Window tenative = 0;
  unsigned char *tenative_versionStr = 0;

  xtRemoteInitAtoms (dpy);

  if (! XQueryTree (dpy, root, &root2, &parent, &kids, &nkids))
    {
      fprintf (stderr, "remote: XQueryTree failed on display %s\n",
	       DisplayString (dpy));
      return (Window)0;		/* causes status == 2 */
    }

  /* root != root2 is possible with virtual root WMs. */

  if (! (kids && nkids))
    {
      fprintf (stderr, "remote: root window has no children on display %s\n",
	       DisplayString (dpy));
      return (Window)0;		/* causes status == 2 */
    }

  for (i = nkids-1; i >= 0; i--)
    {
      Atom type;
      int format;
      unsigned long nitems, bytesafter;
      unsigned char *versionStr = 0;
      unsigned char *appNameStr = 0;
      Window w = XmuClientWindow (dpy, kids[i]);
      int p_status = XGetWindowProperty (dpy, w, XA_XREMOTE_APPLICATION,
					 0, (65536 / sizeof (long)),
					 False, XA_STRING,
					 &type, &format, &nitems,
					 &bytesafter, &appNameStr);
      int v_status = XGetWindowProperty (dpy, w, XA_XREMOTE_VERSION,
					 0, (65536 / sizeof (long)),
					 False, XA_STRING,
					 &type, &format, &nitems,
					 &bytesafter, &versionStr);
      if (!versionStr || !appNameStr)
	{
	  if (versionStr) XFree (versionStr);
	  if (appNameStr) XFree (appNameStr);
	  /* we can only use this window if we find both properties and
	     if the the program-property matches the requested name */
	  continue;
	}
      if (!applicationName ||	/* any window will do */
	  strcmp((char*)appNameStr, applicationName) == 0)
	{
	  /* applicationName matches, check the version */

	  XFree (appNameStr);

	  if (strcmp((char*)versionStr, XREMOTE_EXPECTED_VERSION_STR) != 0 &&
	      !tenative)
	    {
	      /* non-matching version, but probably usable */
	      tenative = w;
	      tenative_versionStr = versionStr;
	      continue;
	    }
	  
	  /* Window w belongs to the correct application and 
	     carries the same version stamp as requested */
	  XFree (versionStr);
	  if (p_status == Success &&
	      v_status == Success && type != None)
	    {
	      result = w;
	      break;
	    }
	}
      else
	XFree (appNameStr);	/* appName doesn't match */
    }

  if (result && tenative)
    {
      fprintf (stderr,
	       "remote: %s warning: both version %s (0x%x) and version\n"
	       "\t%s (0x%x) are running.  Using version %s.\n",
	       applicationName ? applicationName : "",
	       tenative_versionStr, (unsigned int) tenative,
	       XREMOTE_EXPECTED_VERSION_STR, (unsigned int) result,
	       XREMOTE_EXPECTED_VERSION_STR);
      XFree (tenative_versionStr);
      return result;
    }
  else if (tenative)
    {
      /* found a window of the requested application, but
	 the version string didn't match */
      fprintf (stderr,
	       "remote: warning: expected version %s but found version\n"
	       "\t%s (0x%x) instead.\n",
	       XREMOTE_EXPECTED_VERSION_STR,
	       tenative_versionStr, (unsigned int) tenative);
      XFree (tenative_versionStr);
      return tenative;
    }
  else if (result)
    {
      return result;
    }
  else
    {
      if (applicationName)
	fprintf (stderr, "remote: %s not running on display %s\n", 
		 applicationName, DisplayString (dpy));
      else
	fprintf (stderr, "remote: no remote-controllable running on display %s\n", 
		 DisplayString (dpy));

      return (Window)0;		/* causes response_status == 2 */
    }
} /* xtRemoteFindWindow */

static int xtRemoteCheckWindow (Display *dpy, Window window)
     /* if a standalone program has specified a specific window
	using the -id <window> command line parameter,
	we need to check that we can remote control this window. */
{
  Atom type;
  int format;
  unsigned long nitems, bytesafter;
  unsigned char *versionStr = 0;
  int x_status;

  xtRemoteInitAtoms (dpy);

  x_status = XGetWindowProperty (dpy, window, XA_XREMOTE_VERSION,
				 0, (65536 / sizeof (long)),
				 False, XA_STRING,
				 &type, &format, &nitems, &bytesafter,
				 &versionStr);

  if (x_status != Success || !versionStr)
    {
      fprintf (stderr,
	       "Warning : window 0x%x is not a valid remote-controllable window.\n",
		 (unsigned int) window);
      return 2;			/* failure */
    }

  if (strcmp ((char*)versionStr, XREMOTE_EXPECTED_VERSION_STR) != 0)
    {
      fprintf (stderr, "Warning : remote controllable window 0x%x uses "
	       "different version %s of remote control system, expected "
	       "version %s.",
	       (unsigned int) window,
	       versionStr, XREMOTE_EXPECTED_VERSION_STR);
    }

  XFree (versionStr);

  return 0;			/* success */
} /* xtRemoteCheckWindow */

/* call to send single command to XA_XREMOTE_COMMAND atom 
 * then wait for a response - this bit is run by the remote-client */
static int xtRemoteCommand (Display *dpy, Window window, 
			   const char *command)
{
  int result;
  Bool isDone = False;

#ifdef XREMOTE_DEBUG_PROPS
  fprintf (stderr, "remote: (writing " XREMOTE_COMMAND_PROP
	   " \"%s\" to 0x%x)\n",
	   command, (unsigned int) window);
#endif

  XChangeProperty (dpy, window, XA_XREMOTE_COMMAND, XA_STRING, 8,
		   PropModeReplace, (unsigned char *) command,
		   strlen (command));

  /* wait for something to appear in the response Atom */
  while (!isDone)
    {
      XEvent event;
      XNextEvent (dpy, &event);
      if (event.xany.type == DestroyNotify &&
	  event.xdestroywindow.window == window)
	{
	  /* Print to warn user...*/
	  fprintf (stderr, "remote : window 0x%x was destroyed.\n",
		   (unsigned int) window);
	  result = 6;		/* invalid window */
	  goto DONE;
	}
      else if (event.xany.type == PropertyNotify &&
	       event.xproperty.state == PropertyNewValue &&
	       event.xproperty.window == window &&
	       event.xproperty.atom == XA_XREMOTE_RESPONSE)
	{
	  Atom actual_type;
	  int actual_format;
	  unsigned long nitems, bytes_after;
	  unsigned char *commandResult = 0;
	  int x_status;
	  
	  x_status = XGetWindowProperty(dpy, window, XA_XREMOTE_RESPONSE,
					0, (65536 / sizeof (long)),
					True, /* atomic delete after */
					XA_STRING,
					&actual_type, &actual_format,
					&nitems, &bytes_after,
					&commandResult);
#ifdef XREMOTE_DEBUG_PROPS
	  if (x_status == Success && commandResult && *commandResult)
	    {
	      fprintf (stderr, "remote: (server sent " XREMOTE_RESPONSE_PROP
		       " \"%s\" to 0x%x.)\n",
		       commandResult, (unsigned int) window);
	    }
#endif

	  if (x_status != Success)
	    {
	      fprintf (stderr, "remote: failed reading " XREMOTE_RESPONSE_PROP
		       " from window 0x%0x.\n",
		       (unsigned int) window);
	      result = 6;	/* invalid window */
	      isDone = True;
	    }
	  else
	    {
	      if (commandResult && *commandResult)
		{
		  fprintf (stdout, "%s", commandResult);
		  fflush (stdout);
		  result = 0;	/* everything OK */
		  isDone = True;
		}
	      else
		{
		  fprintf (stderr, "remote: invalid data on " XREMOTE_RESPONSE_PROP
			   " property of window 0x%0x.\n",
			   (unsigned int) window);
		  result = 6;	/* invalid window */
		  isDone = True;
		}
	    }
	  
	  if (commandResult)
	    XFree (commandResult);
	}
#ifdef XREMOTE_DEBUG_PROPS
      else if (event.xany.type == PropertyNotify &&
	       event.xproperty.window == window &&
	       event.xproperty.state == PropertyDelete &&
	       event.xproperty.atom == XA_XREMOTE_COMMAND)
	{
	  fprintf (stderr, "remote: (server 0x%x has accepted "
		   XREMOTE_COMMAND_PROP ".)\n",
		   (unsigned int) window);
	}
#endif /* XREMOTE_DEBUG_PROPS */
    }

 DONE:

  return result;  
} /* xtRemoteCommand */



/* process list of commands */
/* used by either linked-in and standalone remote-control client,
   once we've found a suitable window to which to send the commands */
static int xtRemoteCommands (Display *dpy, Window window, char **commands)
     /* similar to mozilla_remote_commands, but we can't cope
	with window == NULL, because at this point we lost the
	information about the name of the graphPackage app we want
	to talk to */
{
  int response_status;

  xtRemoteInitAtoms (dpy);

  response_status = xtRemoteCheckWindow (dpy, window);

  if (response_status != 0)
    /* has to succeed, the (Window)window was derived previously */
    {
      fprintf (stderr,
	       "xtRemoteCommands() - received invalid Window pointer");
      return response_status;
    }

  XSelectInput (dpy, window, (PropertyChangeMask|StructureNotifyMask));

  while (*commands && *commands[0])
    {
      response_status = xtRemoteCommand (dpy, window, *commands);

      if (response_status != 0)
	break;
      commands++;
    }
  
  return response_status;
} /* xtRemoteCommands */

/************************* eof ******************************/
