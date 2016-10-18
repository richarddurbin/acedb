/*  File: graphgdk_remote.c
 *  Author: Fred Wobus (fw@sanger.ac.uk)
 *  Copyright (C) J Thierry-Mieg and R Durbin, 1998
 * -------------------------------------------------------------------
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * -------------------------------------------------------------------
 * This file is part of the ACEDB genome database package, written by
 * 	Richard Durbin (MRC LMB, UK) rd@sanger.ac.uk, and
 *	Jean Thierry-Mieg (CRBM du CNRS, France) mieg@kaa.cnrs-mop.fr
 *
 * Description: Our implementation of the netscape remote control protocol.
 *
 *                This file is now completely re-written from the
 *		original 'remote.c' which is the official sample 
 *		implementation for the remote control of UNIX Netscape 
 *		Navigator under X11. 
 *
 *		Note that this module co-operates with, and requires, gdk.
 *		It uses gdk's X server connection, and grabs events back 
 *		from its queue with a filter. Is it not possible to
 *		do this stuff in pure gdk, because it needs to manipulate
 *		windows which gdk is not managing.
 *
 *		We include the source for XmuClientWindow verabatim, as this
 *		avoids linking Xmu, which, under some OS's (ie Solaris) 
 *		means linking Xt as well!
 *
 *		Documentation for the protocol which this code implements 
 *		may be found at:
 * 
 *		http://home.netscape.com/newsref/std/x-remote.html
 *
 * Exported functions: See graphgtk_.h
 * HISTORY:
 * Last edited: Oct 11 13:54 2006 (edgrif)
 * Created: Wed Nov 14 10:35:43 2001 (edgrif)
 * CVS info:   $Id: graphgdkremote.c,v 1.11 2006-10-11 14:21:43 edgrif Exp $
 *-------------------------------------------------------------------
 */


/* The following copyright refers to the implementation of XmuClientWindow
   ie the functions XmuClientWindow and TryChildren */

/* Copyright 1989, 1998  The Open Group

All Rights Reserved.

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
OPEN GROUP BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

Except as contained in this notice, the name of The Open Group shall not be
used in advertising or otherwise to promote the sale, use or other dealings
in this Software without prior written authorization from The Open Group.

*/

#ifndef __CYGWIN__

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include <X11/Xlib.h>
#include <X11/Xatom.h>

#include <gdk/gdk.h>
#include <gdk/gdkprivate.h>
#include <gtk/gtk.h>
#ifdef GTK2
#include <gdk/gdkx.h>
#endif
#include <wh/regular.h>
#include <wh/menu.h>

#include <w2/graphdev.h>
#include <w2/graphgtk_.h>

/* vroot.h is a header file which lets a client get along with `virtual root'
   window managers like swm, tvtwm, olvwm, etc.  If you don't have this header
   file, you can find it at "http://home.netscape.com/newsref/std/vroot.h".
   If you don't care about supporting virtual root window managers, you can
   comment this line out.
 */
#include <w2/vroot.h>

static BOOL devGtkWebBrowser(char *orig_link) ;
static BOOL UnixURLDisplay(char *orig_link) ;
static char *translateCommas(char *orig_link) ;

#define MOZILLA_VERSION_PROP   "_MOZILLA_VERSION"
#define MOZILLA_COMMAND_PROP   "_MOZILLA_COMMAND"
#define MOZILLA_RESPONSE_PROP  "_MOZILLA_RESPONSE"


static
Window TryChildren (dpy, win, WM_STATE)
    Display *dpy;
    Window win;
    Atom WM_STATE;
{
    Window root, parent;
    Window *children;
    unsigned int nchildren;
    unsigned int i;
    Atom type = None;
    int format;
    unsigned long nitems, after;
    unsigned char *data;
    Window inf = 0;

    if (!XQueryTree(dpy, win, &root, &parent, &children, &nchildren))
	return 0;
    for (i = 0; !inf && (i < nchildren); i++) {
	XGetWindowProperty(dpy, children[i], WM_STATE, 0, 0, False,
			   AnyPropertyType, &type, &format, &nitems,
			   &after, &data);
	if (type)
	    inf = children[i];
    }
    for (i = 0; !inf && (i < nchildren); i++)
	inf = TryChildren(dpy, children[i], WM_STATE);
    if (children) XFree((char *)children);
    return inf;
}

/* Our own copy of this - linking Xmu implies linking Xt on Solaris and
   we don't want to do that */

static Window XmuClientWindow (dpy, win)
    Display *dpy;
    Window win;
{
    Atom WM_STATE;
    Atom type = None;
    int format;
    unsigned long nitems, after;
    unsigned char *data;
    Window inf;

    WM_STATE = XInternAtom(dpy, "WM_STATE", True);
    if (!WM_STATE)
	return win;
    XGetWindowProperty(dpy, win, WM_STATE, 0, 0, False, AnyPropertyType,
		       &type, &format, &nitems, &after, &data);
    if (type)
	return win;
    inf = TryChildren(dpy, win, WM_STATE);
    if (!inf)
	inf = win;
    return inf;
}


static struct queueRec { 
  Window window;
  char *command;
  struct queueRec *next;
} *queueHead;

static void remote_process(struct queueRec *rec)
{
  static Atom XA_MOZILLA_COMMAND  = 0;
#ifdef GTK2
  Display *gdk_display = gdk_x11_get_default_xdisplay();
#endif

  if (! XA_MOZILLA_COMMAND)
    XA_MOZILLA_COMMAND = XInternAtom(gdk_display, 
				     MOZILLA_COMMAND_PROP, False);
  
  XSelectInput (gdk_display, rec->window, 
		(PropertyChangeMask|StructureNotifyMask));
  
  XChangeProperty (gdk_display, rec->window, XA_MOZILLA_COMMAND, XA_STRING, 8,
		   PropModeReplace, (unsigned char *) rec->command,
		   strlen (rec->command));
}

static GdkFilterReturn remote_filter(GdkXEvent *xevent,
				     GdkEvent *gdkevent,
				     gpointer  data)
{
  XEvent *event = (XEvent *)xevent;
  BOOL done = FALSE;
  static Atom XA_MOZILLA_RESPONSE = 0;
#ifdef GTK2
  Display *gdk_display = gdk_x11_get_default_xdisplay();
#endif

  if (!queueHead || event->xany.window != queueHead->window)
    { return gdk_window_lookup(event->xany.window) 
	? GDK_FILTER_CONTINUE : GDK_FILTER_REMOVE;
    }
  
  if (! XA_MOZILLA_RESPONSE)
    XA_MOZILLA_RESPONSE = XInternAtom(gdk_display,
				      MOZILLA_RESPONSE_PROP, False);
  
  if (event->xany.type == DestroyNotify)
    done = TRUE;
  else if (event->xany.type == PropertyNotify &&
	   event->xproperty.state == PropertyNewValue &&
	   event->xproperty.atom == XA_MOZILLA_RESPONSE)
    {
      Atom actual_type;
      int actual_format;
      unsigned long nitems, bytes_after;
      unsigned char *data = 0;
      int result;
      
      result = XGetWindowProperty (event->xany.display, queueHead->window, 
				   XA_MOZILLA_RESPONSE,
				   0, (65536 / sizeof (long)),
				   True, /* atomic delete after */
				   XA_STRING,
				   &actual_type, &actual_format,
				   &nitems, &bytes_after,
				   &data);
      if (result != Success)
	done = TRUE;
      else if (!data || strlen((char *) data) < 5)
	done = TRUE;
      else if (*data == '1')	/* positive preliminary reply */
	done = FALSE;
      else if (!strncmp ((char *)data, "200", 3)) /* positive completion */
	done = TRUE;
      else if (*data == '2')    /* positive completion */
	done = TRUE;
      else if (*data == '3')	/* positive intermediate reply */
	done = TRUE;
      else if (*data == '4' ||	/* transient negative completion */
	       *data == '5')	/* permanent negative completion */
	done = TRUE;
      else
	done = TRUE;
      
      if (data)
	XFree (data);
    }
  
  if (done)
    { 
      struct queueRec *old = queueHead;
      XSelectInput (gdk_display, old->window, 0);
      queueHead = old->next;
      messfree(old->command);
      messfree(old);
      if (queueHead)
	remote_process(queueHead);
    }
  
  return GDK_FILTER_REMOVE;
}


static BOOL devGtkWebBrowser(char *orig_link)  
{
  BOOL result = FALSE ;

  if (orig_link && (strlen(orig_link) > 0))
    {
#ifdef DARWIN
      result = (system(messprintf("open %s &", orig_link)) == 0) ;
#else
      result = UnixURLDisplay(orig_link) ;
#endif
    }

  return TRUE;
}


static BOOL UnixURLDisplay(char *orig_link)  
{			   
  int i;
  Window target;
  Window root = gdk_x11_get_default_root_xwindow();
  Window root2, parent, *kids;
  unsigned int nkids;
  static Atom XA_MOZILLA_VERSION  = 0;
  char *link = orig_link ;
  Display *gdk_display = gdk_x11_get_default_xdisplay();

  messAssert(orig_link && *orig_link) ;

  if (!XA_MOZILLA_VERSION)
    XA_MOZILLA_VERSION = XInternAtom(gdk_display, 
				     MOZILLA_VERSION_PROP, False);
  
  if (!XQueryTree (gdk_display, root, &root2, &parent, &kids, &nkids))
      return FALSE;
  
  /* root != root2 is possible with virtual root WMs. */

  if (! (kids && nkids))
    return FALSE;

  for (i = nkids-1; i >= 0; i--)
    {
      Atom type;
      int format, status;
      unsigned long nitems, bytesafter;
      unsigned char *version = 0;
      target = XmuClientWindow (gdk_display, kids[i]);
      status = XGetWindowProperty (gdk_display, target, XA_MOZILLA_VERSION,
				   0, (65536 / sizeof (long)),
				   False, XA_STRING,
				   &type, &format, &nitems, &bytesafter,
				   &version);
      if (version)
	{
	  XFree (version);
	  if (status == Success && type != None)
	    break;
	}
      target = 0;
    }


  if (target == 0)
    { 
      if (messQuery("Start Netscape to display %s ?", link))
	return (system(messprintf("netscape %s &", link)) == 0);
    }
  else
    {
      struct queueRec *rec = NULL ;
      char *tmp_link  = NULL ;

      /* Translate any "," to "%2C" see routine for why we need to...sigh.   */
      if ((tmp_link = translateCommas(link)))
	link = tmp_link ;

      rec = (struct queueRec *)messalloc(sizeof(struct queueRec));
      rec->window = target;
      rec->next = NULL;
      rec->command = messalloc(11+strlen(link));
      
      if (*link == SUBDIR_DELIMITER)
	sprintf(rec->command, "openFILE(%s)", link);
      else
	sprintf(rec->command, "openURL(%s)", link);
      
      if (queueHead)
	{ 
	  struct queueRec *p = queueHead;
	  while (p->next) p = p->next;
	  p->next = rec;
	}
      else
	{
	  queueHead = rec;
	  remote_process(rec); /* prime the pump */
	}

      if (tmp_link)
	messfree(tmp_link) ;
    } 

  return TRUE;
}




void graphGdkRemoteInit(void)
{  
  queueHead = NULL;
  gdk_window_add_filter(NULL, remote_filter, NULL); 
}


/* We use the netscape "OpenURL" remote command to display links, sadly the  */
/* syntax for this command is: "OpenURL (URL [, new-window])" and stupid     */
/* netscape will think that any "," in the url (i.e. lots of cgi links have  */
/* "," to separate args !) is the "," for its OpenURL command and will then  */
/* usually report a syntax error in the OpenURL command.                     */
/*                                                                           */
/* To get round this we translate any "," into "%2C" which is the standard   */
/* code for a "," in urls (thanks to Roger Pettet for this)....YUCH.         */
/*                                                                           */
static char *translateCommas(char *orig_link)
{
  char *result = NULL ;
  char *string = NULL ;
  int num_commas = 0 ;

  string = orig_link ;
  num_commas = 0 ;
  do 
    {
      if ((string = strstr(string, ",")))
	{
	  num_commas++ ;
	  string++ ;
	}
    } while (string != NULL) ;

  if (num_commas != 0)
    {
      char *comma = "%2C" ;
      int comma_len = 3 ;				    /* just the chars, not the \0 */
      int i, orig_len ;
      char *old, *new ;

      orig_len = strlen(orig_link) ;

      result = messalloc(orig_len + (num_commas * comma_len) + 1) ;

      for (i = 0, old = orig_link, new = result ; i < orig_len ; i++)
	{
	  if (*old != ',')
	    *new++ = *old++ ;
	  else
	    {
	      memcpy(new, comma, comma_len) ;
	      new += comma_len ;
	      old++ ;
	    }
	}
    }


  return result ;
}


#else /* CYGWIN */

#include <sys/cygwin.h>
#include <windows.h>
#include "regular.h"
#include "menu.h"
#include <w2/graphdev.h>

void graphGdkRemoteInit(void)
{
  return;
}

/* This works by making an "Internet shortcut" in tmp and then 
   setting the Shell on it. No doubt there are better ways involving
   OLE and bells and whistles, but this has the advantage of simplicity,
   and compatability with any browser, not just IE.

   NOTE that we can't delete the temp files after calling ShellExecute 
   since that's asynchronous and they'll disappear before needed.
   They will all be cleared up by the acedb temp file system on program
   exit. */

static BOOL devGtkWebBrowser(char *link)
{
  char *tmpname;
  FILE *shortcut = filTmpOpenWithSuffix(&tmpname, "url", "w");
  char winname[MAX_PATH];

  fprintf(shortcut, "[internetshortcut]\n");
  fprintf(shortcut, "url=%s\n", link);
  filclose(shortcut);

  cygwin_conv_to_full_win32_path(tmpname, winname);

  ShellExecute(0, "open", winname, 0, 0, 0);

  return TRUE;
}
  
  
  
#endif /* CYGWIN */

void graphgdkremote_switch_init(GraphDevFunc functable)
{ 
  functable->graphDevWebBrowser = devGtkWebBrowser ;
}

