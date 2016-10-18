/*  File: gex.c
 *  Author: Simon Kelley (srk@sanger.ac.uk)
 *  Copyright (C) J Thierry-Mieg and R Durbin, 1991
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
 * 	Richard Durbin (MRC LMB, UK) rd@mrc-lmb.cam.ac.uk, and
 *	Jean Thierry-Mieg (CRBM du CNRS, France) mieg@kaa.cnrs-mop.fr
 *
 * Description: 
 *              Functions for the integration of GUI code using the
 *		graph package with GUI code using Gtk
 *		Depends on the Gtk back-end for the graph package
 *              
 * Exported functions:  see wh/gex.h
 *			
 * HISTORY:
 * Last edited: Jun 19 14:05 2008 (edgrif)
 * CVS info:   $Id: gex.c,v 1.81 2012-10-05 12:09:51 edgrif Exp $
 *-------------------------------------------------------------------
 */

#ifdef __CYGWIN__
#define COBJMACROS
#include <signal.h>
#include <windows.h>

#include <commdlg.h>
#include <shlobj.h>

typedef POINT 	WIN_PT ;
#define POINT acePOINT
#undef TRANSPARENT


#endif /* CYGWIN */

#include <wh/regular.h>
#include <wh/aceio.h>
#include <wh/menu_.h>
#include <wh/help.h>
#include <wh/pref.h>
#include <wh/utils.h>                     /* for arrstrcmp */
#include <gtk/gtk.h>
#include <gdk/gdk.h>                      /* for GdkFont */
#include <gdk/gdkkeysyms.h>               /* for hotkeys */

#ifdef GTK2
#include <pango/pango-font.h>
#endif

#ifndef __CYGWIN__
#include <gdk/gdkx.h>
#endif

#include <w2/graph_.h>
#include <w2/graphdev.h>
#include <w2/graphgtk_.h>
#include <wh/gex.h>

  
static int editorWidth = 750;
static int editorHeight = 500;

/* PrintParams is used to pass various parameters
** around the print selection windows. */
struct PrintParams {
    GtkWidget *docTitle;
    GtkWidget *printlabel;
    GtkWidget *sWindow;
    GtkWidget *clist;
    GtkWidget *addrlabel;
    GtkWidget *addrentry;
    GtkWidget *filelabel;
    GtkWidget *chooserbutton;
    GtkWidget  *fileentry;
    GtkTextBuffer *text;
    GtkWidget *fileSelWindow;
    Array      printers;
    gint       printer_index;
    BOOL       PRINT_SELECTED;
    BOOL       MAIL_SELECTED;
    BOOL       FILE_SELECTED;
    char      *file_path;
    char      *file_prefix;
    gint       file_number;
    char      *file_suffix;
    char      *mail_address;
    char      *options;
    char      *title;
  };



static void intProc (int sig);
static FILE *gexQueryOpen (char *dname, char *fname, char *end, 
			   char *spec, char *title);
static void gexOut (char *mesg_buf, void *user_pointer);
static void gexExit (char *mesg_buf, void *user_pointer);
static BOOL gexQuery (char *text);
static ACEIN gexPrompt(char *prompt, char *dfault, char *fmt, STORE_HANDLE handle);
static BOOL modalDialog(char *text, char *buttonLabel, BOOL query, BOOL msgopt) ;
static void butClick(GtkButton *button, gpointer user_data) ;

#ifdef GTK2
static void tbPrint         (GtkWidget *w, gpointer callback_data);
static void gexPrint        (GtkWidget *widget, gpointer callback_data);
static void editSelectAll   (gpointer data, GtkWidget *w);
static gint printerSelection(GtkWidget *sWindow, GdkEventButton *event, struct PrintParams *printParams);
static gint PrintSelected   (GtkWidget *widget, struct PrintParams *printParams);
static gint MailSelected    (GtkWidget *widget, struct PrintParams *printParams);
static gint CopySelected    (GtkWidget *widget, struct PrintParams *printParams);
static gint FileChooser     (GtkWidget *widget, struct PrintParams *printParams);
static gint FileChooserOK   (GtkWidget *widget, struct PrintParams *printParams);
static gint FileChooserCancel(GtkWidget *widget, struct PrintParams *printParams);
static gint printOK         (GtkWidget *widget, struct PrintParams *printParams);

static gint ButtonPressCallback(GtkWidget *widget, 
				GdkEventButton *event,
				gpointer callback_data);
#endif /* ifdef GTK2 */


static gboolean getFixedWidthFont(GtkWidget *widget, 
				  GList *pref_families, gint points, PangoWeight weight,
				  PangoFont **font_out, PangoFontDescription **desc_out) ;



/* 
 *                local globals
 */


/*static gint FormatSelected  (GtkWidget *widget, gpointer params);
**static gint EPSFSelected    (GtkWidget *widget, gpointer params);
*/
/* default locations to store persistent information for 
   the file chooser */
static char defaultDir[DIR_BUFFER_SIZE], defaultFile[FIL_BUFFER_SIZE];


/* Ugghhh, static for message popup vs. list stuff, I think there should be  */
/* a gex context to hold info. like this.                                    */
static BOOL msg_use_list_G = FALSE ;			    /* message list window or popup ? */
static int msg_list_length_G = 0 ;			    /* length of message list. */
static MessListCB msg_prefcb_G = NULL ;			    /* callback to handle switch to
							       message window. */


static BOOL modal_dialogs_G = TRUE ;			    /* Toggle modality. */




/* Selection stuff:
 * simply copied from graphgtk.c...in the end all selection stuff will be here...
 * This is the list of selection targets we can provide if asked, this all needs
 * to be kept in step.
 */

typedef enum {
  SIMPLE_TEXT,
  STRING,
  COMPOUND_TEXT
} SelType;

static GtkTargetEntry targetlist[] = {
    { "STRING",        0, STRING },
    { "TEXT",          0, SIMPLE_TEXT },
    { "COMPOUND_TEXT", 0, COMPOUND_TEXT }
  };

static gint ntargets = sizeof(targetlist) / sizeof(targetlist[0]);



/************************************************************/

void gexInit (int *argcptr, char **argv)
{ 
  char *custom_font = NULL ;
  uid_t savuid = geteuid(); /* preserve effective uid */
  GraphDevFunc functable;
  int i;

  *defaultDir = 0;
  *defaultFile = 0;
  
  seteuid(getuid()); 
     /* set the effective uid to be the real uid here, so that 
        X access control can read the .Xauthority file of the
        user. Note that this executes before the user id munging
        in session.c, we restore the status quo ante so as not
        to upset that code. */

  gtk_init(argcptr, &argv);

  seteuid(savuid); /* restore */

  signal (SIGINT, intProc) ;

  for(i=1;i<*argcptr;i++)
    {
      if (!strcmp(argv[i],"-acefont"))
	{
	  if ((*argcptr - i) < 2)
	    {
	      /* no argument to -acefont option */
	      messExit ("No argument specified for -acefont option");
	    }
	  custom_font = strnew(argv[i+1], 0);

	  /* clear both arguments from the list */
	  for(i+=2;i<*argcptr;i++)
	    argv[i-2] = argv[i];
	  argv[*argcptr-2] = 0;
	  (*argcptr) -= 2;
	  break;
        }
    }

  functable = (GraphDevFunc)messalloc(sizeof(GraphDevFuncRec));
  graphgdk_switch_init(functable);
  graphgdkremote_switch_init(functable);
  graphgtk_switch_init(functable);

  /* tell the graph package that we exist */
  graphCBSetFuncTable(functable);
  
  graphGdkFontInit(custom_font);
  graphGdkGcInit();
  graphGdkColorInit();
  graphGdkRemoteInit();
  graphGtkInit();

  gexSetMessCB() ;

  /* internal slim-line browser as default help-viewer */
  {
    struct helpContextStruct helpContext = { gexHtmlViewer, NULL };
    helpOnRegister (helpContext); 
  }

  /* register graphical filechooser */
  filQueryOpenRegister (gexQueryOpen) ; 
  
  return;
} /* gexInit */

void gexSetConfigFiles(char *rcFile, char *fontFile)
{
  graphGdkSetConfigFiles(rcFile, fontFile);
}


/***************************************************************************/

/* Allows an application using gex to set/reset the messubs callbacks, it    */
/* may need to reset them after temporarily altering them for some reason.   */
/*                                                                           */
void gexSetMessCB(void)
{
  struct messContextStruct outContext = { gexOut, NULL };
  struct messContextStruct exitContext = { gexExit, NULL };

  /* use the graphical dialog to output any messages */
  messOutRegister (outContext);
  messErrorRegister (outContext);
  messExitRegister (exitContext);
  messCrashRegister (exitContext);

  messQueryRegister (gexQuery);
  messPromptRegister (gexPrompt);

  return ;
}


/***************************************************************************/
/* Routines to handle switching between a single popup for each informational
 * message and a scrolled list of messages. */

/* Set messout/messerror callbacks to output messages as popup dialogs OR
 * as a message list depending on user preferences.
 * MUST be called BEFORE gexSetMessPopUps(). */
void gexSetMessPrefs(BOOL use_msg_list, int msg_list_length, MessListCB prefcb)
{
  msg_use_list_G = use_msg_list ;
  msg_list_length_G = msg_list_length ;
  msg_prefcb_G = prefcb ;

  gexSetMessPopUps(msg_use_list_G, msg_list_length_G) ;

  return ;
}


/* Called from mesg list code on destroy of mesg list by user. We just set msg_use_list_G
 * FALSE so that we do try to destroy the message list again, it will already have been
 * destroyed. */
static void gexMesgListCloseCB(void)
{
  msg_use_list_G = FALSE ;

  /* This allows the application to take control of the switch to the    */
  /* message window, in acedbs case we need this to keep user prefs. in  */
  /* step with message window switching, otherwise we just switch to popups. */
  if (msg_prefcb_G)
	msg_prefcb_G(msg_use_list_G) ;
  else
    gexSetMessPopUps(msg_use_list_G, msg_list_length_G) ;

  return ;
}

/* Called by gexSetMessPrefs when preferences for messaging are first set up,
 * after that its a called from acedb code which controls the switching of
 * message styles. */
void gexSetMessPopUps(BOOL msg_list, int list_length)
{
  /* These gex routines manage the flip-flopping between the popup and list  */
  /* messages, this is important because the list code expects the caller    */
  /* to manage this bit.                                                     */
  if (msg_list)
    {
      struct messContextStruct graphContext = {mesgListAdd, NULL} ;

      /* ok, need to init system, set up contexts etc.                           */
      mesgListCreate(gexMesgListCloseCB) ;

      msg_use_list_G = msg_list ;			    /* must be before gexSetMessListSize() */
      msg_list_length_G = list_length ;

      gexSetMessListSize(msg_list_length_G) ;

      /* use the graphical dialog to output any messages */
      messOutRegister(graphContext) ;
      messErrorRegister(graphContext) ;
    }
  else
    {
      /* use the graphical dialog to output any messages */
      struct messContextStruct gexContext = { gexOut, NULL };

      /* If there is a message list then destroy it.                         */
      if (msg_use_list_G)
	{
	  mesgListDestroy() ;
	}
      msg_use_list_G = msg_list ;			    /* must be after mesgListDestroy() */

      messOutRegister (gexContext);
      messErrorRegister (gexContext);
    }

  return ;
}


/* Called by gexSetMessPrefs when preferences for messaging are first set up,*/
/* after that its a callback from prefsubs.c for when user sets message      */
/* list size.                                                                */
/*                                                                           */
void gexSetMessListSize(int list_length)
{
  if (msg_use_list_G)
    {
      mesgListSetSize(list_length) ;
    }

  return ;
}


/* Cover function for graph level routine, called by prefsubs during         */
/* initialisation.                                                           */
int gexGetDefListSize(int curr_value)
{
  return (mesgListGetDefaultListSize(curr_value)) ;
}


/* Cover function for graph level routine called by preference code to check */
/* int value for preference.                                                 */
BOOL gexCheckListSize(int curr_value)
{
  return (mesgListCheckListSize(curr_value)) ;
}


void gexSetModalDialog(BOOL modal)
{
  modal_dialogs_G = modal ;

  return ;
}






/***************************************************************************/
/* Signal handling - we cannot call gtk from a signal handler, so we set
   an idle loop function to do it. For the case of a tight loop, where the
   idle function never gets called, or where the user is lazy, an second
   signal will exit without attempting to get further confirmation. */

static BOOL awaitingQuery = FALSE;
static BOOL queryShown = FALSE;
  
static gint abortP(gpointer data)
{ 
  queryShown = TRUE;
  
  if (messQuery ("Do you want to abort?"))
    messExit ("User initiated abort") ; 
  
  queryShown = awaitingQuery = FALSE;

  return FALSE;
  
}

static void intProc (int sig)
{
  if (queryShown)
    return;
  
  if (awaitingQuery)
    { 
      struct messContextStruct nullExitContext = { NULL, NULL };
      messExitRegister (nullExitContext);
      messExit ("User initiated abort") ;
    }
  awaitingQuery = TRUE;
  gtk_idle_add(abortP,  NULL);
} 

/***********************************************************************/

int gexMainLoop(GtkWidget *win)
{ 
  return graphGtkMainLoop(win);
}

void gexLoopReturn(int retval)
{
  graphGtkLoopReturn(retval);
}


/* Used to detect whether user selected to switch to popup informational/    */
/* error messages.                                                           */
/*                                                                           */
static gint msgWindowSig(GtkWidget *w, gpointer data)
{
  BOOL *msg_state = (BOOL *)data ;

  *msg_state = !(*msg_state) ;

  return FALSE ;
}


gint gexLoopReturnSig(GtkWidget *w, gpointer data)
{ 
  graphGtkLoopReturn((BOOL)data);
  return TRUE;
}

gint gexLoopReturnEvent(GtkWidget *w, GdkEventAny *event, gpointer data)
{ 
  graphGtkLoopReturn((BOOL)data);
  return TRUE;
}

void gexSetModalWin(GtkWidget *win, BOOL modal)
{ 
  graphGtkSetModalWin(win, modal);
}

GtkWidget *gexGraphHbox(Graph graph)
{
  GraphDev dev = graphCBGraph2Dev(graph);
  if (dev)
    dev->hasChrome = TRUE; /* change TAB key function to gtk widget select */
  return dev ? dev->hbox : NULL;
}

GtkWidget *gexGraphVbox(Graph graph)
{
  GraphDev dev = graphCBGraph2Dev(graph);
  if (dev)
    dev->hasChrome = TRUE; /* change TAB key function to gtk widget select */
  return dev ? dev->vbox : NULL;
}


static gint activateGraph(GtkWidget *widget, gpointer data)
{ 
  graphActivate(((Graph)(GPOINTER_TO_INT(data)))) ;

  return FALSE; /* So that the real signal handler gets called too */
}

/* arrange that when signal is emitted in widget, the active graph is set */
void gexSignalConnect(Graph graph, GtkObject *widget, gchar *signal,
		      GtkSignalFunc func, gpointer data)
{ 
  gtk_signal_connect(widget, signal, 
		     (GtkSignalFunc) activateGraph,
		     GINT_TO_POINTER(graph)) ;

  gtk_signal_connect(widget, signal, func, data);

  return ;
}


/* Returns the first widget it finds in the dev widget hierachy:
 * 
 *           topWindow -> scroll ->  drawingArea 
 */
GtkWidget *gexGraph2Widget(Graph g)
{
  GraphDev dev = graphCBGraph2Dev(g);

  if (!dev) 
    return NULL;
  else if (dev->topWindow) 
      return dev->topWindow;
  else if (dev->scroll)
    return dev->scroll;
  else
    return dev->drawingArea;
}


/* Always returns the widget that actually represents the graph window,
 * i.e. the drawingArea. */
GtkWidget *gexGraph2GraphWidget(Graph g)
{
  GraphDev dev = graphCBGraph2Dev(g);

  if (!dev || !(dev->drawingArea)) 
    return NULL ;
  else
    return dev->drawingArea ;
}


/* does the same as gtk_window_new(GTK_WINDOW_TOPLEVEL)
   but registers the window so that busy cursors and graphCleanup() work.
   Also registers a call-back on the destroy signal which de-registers */
static gint deregister(GtkObject *widget, gpointer data)
{
  GraphDev dev = (GraphDev) data;
  if (dev->destroyFunc)
    dev->destroyFunc(GTK_WIDGET(widget));
  graphGtkWinRemove(dev);
  graphGtkOnModalWinDestroy(dev->topWindow); /* in case is has a grab */
  messfree(dev);
  return TRUE;
}

static void gexConnectWindow(GtkWidget *window)
{ 
  GraphDev dev = (GraphDev) messalloc (sizeof(GraphDevStruct))  ;
  
  graphGtkSetSplash(0); /* kill any splash screen */
  graphGtkWinInsert(dev);
  /* for gexDestroyRegister */
  gtk_object_set_data(GTK_OBJECT(window), "_dev_struct", (gpointer)dev);
  dev->topWindow = window;
  dev->graph = NULL;
  dev->destroyFunc = NULL;
  
  gtk_signal_connect(GTK_OBJECT(window), "destroy", 
		     GTK_SIGNAL_FUNC(deregister), (gpointer) dev);
  
}

GtkWidget *gexWindowCreate(void)
{ 
  GtkWidget *window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
  gexConnectWindow(window);

  return window;
}

void gexDestroyRegister(GtkWidget *window, void (*func)(GtkWidget *))
{
  /* Simple call-back register, replaces graphRegister(DESTROY)
     for simle cases where people are allergic to the gtk signal system */
  GraphDev dev = 
    (GraphDev)gtk_object_get_data(GTK_OBJECT(window), "_dev_struct");
  
  if (dev)
    dev->destroyFunc = func;
}

void gexCleanup(GtkWidget *save)
/* Like graphcleanup, except that the one window not destroyed
   is the the GtkWindow specified in the "save" argument. NOTE: this cleans
   up both graph windows and gtk windows created using gexWindowCreate. */
{ 
 graphGtkWinCleanup(save);
}



/* Put up a splash screen. 

   Note that any graphPackage or Gex call which creates as window
   will kill the splash screen. This is important to avoid obscuring
   dialog boxes, and is assumed by the main() code which doesn't 
   explicitly remove the splash screen. */

void gexSplash(char **splash_xpm)
{
  GtkWidget *window = NULL;
  GdkBitmap *mask;
  GdkPixmap *pixmap;
  GtkWidget *pixmapwidg;

  window= gtk_window_new(GTK_WINDOW_POPUP);
  gtk_window_set_position(GTK_WINDOW(window), GTK_WIN_POS_CENTER);
  gtk_widget_realize(window);
  pixmap = gdk_pixmap_create_from_xpm_d(window->window,
					&mask,
					NULL,
					(gchar **)splash_xpm);
  pixmapwidg = gtk_pixmap_new( pixmap, mask );
  gdk_pixmap_unref(pixmap);
  gdk_pixmap_unref(mask);
  gtk_container_add(GTK_CONTAINER(window), pixmapwidg);
  
  gtk_widget_show(pixmapwidg);
  gtk_widget_show_now(window);
  
  while(g_main_iteration(FALSE));

  graphGtkSetSplash(window);
}

/* Set the default icon for windows created by gex or graph */
void gexSetIconFromXPM(char *xpm)
{
  graphGtkSetIcon(xpm);
}

#ifndef __CYGWIN__
unsigned long gexGetWindowId(Graph graph)
{
  GraphDev dev = graphCBGraph2Dev(graph);
  if (dev && dev->topWindow)
    return (unsigned long)GDK_WINDOW_XWINDOW(dev->topWindow->window);
  else
    return 0;
}
#endif

/**************************************************************************/
/* File chooser, on unix we use the standard GTK one, but extend it a bit */
/* On windows we use native controls (God help us.)                       */
/**************************************************************************/

#ifdef __CYGWIN__

static FILE *gexQueryOpen(char *dname, char *fname, char *end, 
			  char *spec, char *title)
{
  char selectedPath[MAX_PATH];
  char winPath[MAX_PATH];
  char *dirName = dname ? dname : defaultDir;
  char *fileName = fname ? fname : defaultFile;
  char path[FIL_BUFFER_SIZE+DIR_BUFFER_SIZE];

  graphBusyCursorAll(); /* while creating */
  graphGtkSetSplash(0); /* kill any splashscreen */
  
  if (spec[1] == 'd')
    {
      BROWSEINFO bi;
      LPITEMIDLIST pidlBrowse;
      LPMALLOC shell_malloc; 
      
      (void)SHGetMalloc(&shell_malloc); 

      bi.hwndOwner = NULL;
      bi.pidlRoot = NULL;
      bi.pszDisplayName = winPath;
      bi.lpszTitle = title;
      bi.ulFlags = BIF_RETURNONLYFSDIRS;
      bi.lpfn = NULL;
      
      pidlBrowse = SHBrowseForFolder(&bi);

      if (!pidlBrowse)
	return NULL;
      
      if (!SHGetPathFromIDList(pidlBrowse, selectedPath))
	{
	  IMalloc_Free(shell_malloc, pidlBrowse);
	  return NULL;
	}
      
      IMalloc_Free(shell_malloc, pidlBrowse);

      cygwin_conv_to_full_posix_path(selectedPath, dirName);

      if (!filCheckName(dirName, 0, spec))
	{ 
	  messout("Cannot access %s", fname);
	  return NULL;
	}
      /* It doesn't make sense to return a FILE * for directories
	 as these are not permitted in general (and break under cygwin)
	 We return 0x1 to indicte success, filclose will notice this
	 and ignore it. Note that all of this is a temporary kludge,
	 Fred will upgrade all of this as part of the aceIn/Out effort */
      return (FILE *) 0x1;
    }
  else
    {
      OPENFILENAME ofn;
      DWORD flags = OFN_EXTENSIONDIFFERENT | OFN_LONGNAMES |  OFN_HIDEREADONLY;
      unsigned char *cp;
      char types_buff[400]; /* So sue me */
     
      if (spec[0] == 'w')
	flags |= OFN_OVERWRITEPROMPT;
      else
	flags |= OFN_FILEMUSTEXIST;
      
      strcpy(selectedPath, fileName);
      
      cygwin_conv_to_full_win32_path(dirName, winPath);
      
      if (end && *end)
	{
	  sprintf(types_buff, "%s Files (*.%s)\01*.%s\01"
		  "All Files (*.*)\01*.*\01", 
		  end, end, end);
	  *types_buff = toupper(*types_buff);
	  
	  for (cp = types_buff; *cp; cp++)
	    if (*cp == 1)
	      *cp = 0;
	  
	  ofn.lpstrFilter = types_buff;
	}
      else 
	ofn.lpstrFilter = NULL;
      
      ofn.lStructSize = sizeof(OPENFILENAME);
      ofn.hwndOwner = NULL; 
      ofn.hInstance = NULL;
      ofn.lpstrCustomFilter = NULL;
      ofn.nMaxCustFilter = 0;
      ofn.nFilterIndex = 0;
      ofn.lpstrFile = selectedPath;
      ofn.nMaxFile = MAX_PATH;
      ofn.lpstrFileTitle = NULL;
      ofn.lpstrInitialDir = winPath;
      ofn.lpstrTitle = title;
      ofn.Flags = flags;
      ofn.lpstrDefExt = end;
      
      if (spec[0] == 'w')
	{
	  if (!GetSaveFileName(&ofn))
	    return NULL;
	}
      else
	{
	  if (!GetOpenFileName(&ofn))
	    return NULL;
	}
      
      selectedPath[ofn.nFileOffset-1] = 0;
      
      cygwin_conv_to_full_posix_path(selectedPath, dirName);
      strcpy(fileName, selectedPath+ofn.nFileOffset);
      
      sprintf(path, "%s%s%s", 
	      dirName, 
	      (strlen(dirName) && strlen(fileName)) ? "/" : "",
	      fileName);
      
      return filopen(path, "", spec);
    }
}

#else /* ! __CYGWIN__ */

static char *dirName, *fileName;
static FILE *filep;
static char *file_spec;
static char *file_end;

static gint fileSelOk(GtkWidget *w, gpointer data)
{
  int i,j;
  char *fname = (char *)gtk_file_selection_get_filename (GTK_FILE_SELECTION (data));
  
  i = strlen(fname);

  if (!g_file_test(fname, G_FILE_TEST_IS_DIR))
    {
      while (i && (fname[i-1] != '/'))
	{
	  i--;
	}
      strcpy(fileName, &fname[i]);
    }

  if (fname[i] == '/')
    i-- ;
  for (j = 0 ; j < i ; j++)
    {
      dirName[j] = fname[j]; 
    }
  dirName[j] = 0;

  if (file_spec[1] == 'd')
    {
      /* Picking a directory */
      if (strlen(fileName))
	return TRUE; /* picked a file - ignore */
      if (!filCheckName(fname, 0, file_spec))
	{ 
	  messout("Cannot access %s", fname);
	  return TRUE;
	}
      /* It doesn't make sense to return a FILE * for directories
	 as these are not permitted in general (and break under cygwin)
	 We return 0x1 to indicte success, filclose will notice this
	 and ignore it. Note that all of this is a temporary kludge,
	 Fred will upgrade all of this as part of the aceIn/Out effort */
      filep = (FILE *) 0x1;
    }
  else if (!strlen(fileName))
    return TRUE; /* picked a directory  - ignore */
  else if (file_spec[0] == 'w' 
	   && filCheckName(fname, 0, "r")
	   && !messQuery(messprintf("Overwrite %s?", fname)))
    filep = NULL;
  else
    {
      if (!filCheckName(fname, 0, file_spec))
	{ 
	  messout("Failed to open %s", fname);
	  return TRUE;
	}
      filep = filopen(fname, 0, file_spec);
      if (!filep)
	return TRUE; /* failed to open, don't destroy picker */
    }

  gexLoopReturn(TRUE);
 
  return TRUE;
}

static void add_template(GtkWidget *filew)
{
  GtkWidget *entry = GTK_FILE_SELECTION(filew)->selection_entry;
  
  if (file_end && strlen(file_end))
    {
      if (file_spec[0] == 'r')
	gtk_entry_set_text(GTK_ENTRY(entry), messprintf("*.%s", file_end));
      else if ((file_spec[0] == 'a') || (file_spec[0] == 'w'))
	{
	  gtk_entry_set_text(GTK_ENTRY(entry), messprintf(".%s", file_end));
	  gtk_entry_set_position(GTK_ENTRY(entry),0);
	}
    }
}


static void ace_file_selection_dir_button (GtkWidget *widget,
			       gint row, 
			       gint column, 
			       GdkEventButton *bevent,
			       gpointer user_data)
{
 
  if (bevent && bevent->type == GDK_2BUTTON_PRESS)
    add_template(GTK_WIDGET(user_data));
}



#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
/* THIS FUNCTION SEEMS TO BE SUPERFLUOUS WITH GTK2, ONCE MORE USER TESTING HAS BEEN DONE
 * I'LL REMOVE IT. */
static void ace_file_selection_dir_button_first (GtkWidget *widget,
				     gint row, 
				     gint column, 
				     GdkEventButton *bevent,
				     gpointer user_data)
{
  graphBusyCursorAll();
}
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */



static gint fileSelMail(GtkWidget *w, gpointer data)
{
  ACEIN addr_in;

  if ((addr_in = messPrompt("Please give email address.", "", "t", 0)))
    {
      filep = filmail(aceInWord(addr_in));
      aceInDestroy (addr_in);
      gexLoopReturn(TRUE);
    }
  return TRUE;
}

static FILE *gexQueryOpen(char *dname, char *fname, char *end, char *spec, char *title)
{
  GtkFileSelection  *filew ;
  GtkWidget *mailButton;
  char path[FIL_BUFFER_SIZE+DIR_BUFFER_SIZE];
  BOOL isOk;
  Graph old ;

  old = graphActive() ;					    /* Remember previous active graph. */


  /* We use the GTK file selection widget. */
  filew = GTK_FILE_SELECTION(gtk_file_selection_new(title));

  gtk_window_set_default_size(GTK_WINDOW(filew), 600, 400);
  gexConnectWindow(GTK_WIDGET(filew));
  graphGtkSetModalWin(GTK_WIDGET(filew), TRUE);
  graphBusyCursorAll(); /* while creating */
   
  /* provide defaults */
  dirName = dname ? dname : defaultDir ;
  fileName = fname ? fname : defaultFile ; 

  file_end = end ;
  file_spec = spec ;
  filep = NULL;						    /* in case of destroy or cancel */
  

  /* I have altered the code here so that if we just have a directory then   */
  /* we still append a "/" to the end, if you don't do this then             */
  /* the selection box shows the directory _below_ the one you want.         */
  sprintf(path, "%s%s%s", 
	  dirName, 
	  (strlen(dirName)) ? "/" : "",
	  fileName) ;

  /* Set default file path.                                                  */
  gtk_file_selection_set_filename (filew, path);

  /* But also set the file as a pattern to be matched.                       */
  if (file_end && strlen(file_end))
    gtk_file_selection_complete (filew, messprintf("*.%s", file_end));
      
  add_template(GTK_WIDGET(filew));

  gtk_signal_connect(GTK_OBJECT(filew->cancel_button),
		     "clicked", GTK_SIGNAL_FUNC(gexLoopReturnSig),
		     (gpointer) FALSE);
  
  gtk_signal_connect(GTK_OBJECT(filew), "delete_event", 
		     (GtkSignalFunc) gexLoopReturnEvent,
		     (gpointer) FALSE);
  
  gtk_signal_connect(GTK_OBJECT(filew->ok_button),
		     "clicked", (GtkSignalFunc) fileSelOk,
		     (gpointer) filew);



#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
  /* THIS CALLBACK/FUNCTION SEEMS NOW TO BE REDUNDANT....REMOVE ONCE I'M SURE.... */

  gtk_signal_connect(GTK_OBJECT(filew->dir_list),
		     "button_press_event",
		     (GtkSignalFunc) ace_file_selection_dir_button_first, 
		     (gpointer) filew);

  /* this may be defunct as well, it doesn't work in gtk2, we'll see if people miss it... */
  gtk_signal_connect_after(GTK_OBJECT(filew->dir_list),
			   "select_row",
			   (GtkSignalFunc) ace_file_selection_dir_button, 
			   (gpointer) filew);
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */



  if (strcmp(spec, "w") == 0)
    { 
      mailButton =  gtk_button_new_with_label("Send as Mail");
      gtk_box_pack_start (GTK_BOX (GTK_FILE_SELECTION(filew)->button_area), 
			  mailButton, TRUE, TRUE, 0);
      gtk_widget_show(mailButton);
      gtk_signal_connect(GTK_OBJECT(mailButton),
			 "clicked", (GtkSignalFunc) fileSelMail,
			 (gpointer) filew);
    }
  
  gtk_widget_show(GTK_WIDGET(filew));

  isOk = gexMainLoop(GTK_WIDGET(filew));

  gtk_widget_destroy(GTK_WIDGET(filew));
  
  /* Reset to avoid obscure behaviour.                                       */
  *defaultDir = 0;
  *defaultFile = 0;

  graphActivate(old);

  return (isOk ? filep : NULL) ;
}

#endif /* !__CYGWIN__ */
 



/*
 * ----------------------   Modal dialogs   ----------------------
 */

/* I know gtklabels can split text, but this does it better */
static char *split_message(char *text)
{  
  char *cp, *cp1, *ourtext;
  int j, i=0;
  
  cp = ourtext = strnew(text, 0);
  /* break long messages */
  while (*(++cp))
    { 
      if ((*cp) == '\n') i = 0;
      if ((i++>30) && (*cp == ' '))
        { 
          cp1 = cp;
          for( j = 0; j<10; j++,cp1++)
            if ((!(*cp1)) || ((*cp1) == '\n'))
              goto skip; 
          i=0;
          *cp = '\n';
        skip:;
        }
    }
  return ourtext;
}

/* XPM */
static char *q_mark3[] = {
/* width height num_colors chars_per_pixel */
"    35    34        5            1",
/* colors */
". c #000000",
"# c #808080",
"a c None",
"b c #ffff00",
"c c #ffffff",
/* pixels */
"aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa",
"aaaaaaaaaaaa........aaaaaaaaaaaaaaa",
"aaaaaaaaaa..cbcbcbcb..aaaaaaaaaaaaa",
"aaaaaaaaa.bcbcbcbcbcbc.aaaaaaaaaaaa",
"aaaaaaaa.........bcbcbb.aaaaaaaaaaa",
"aaaaaaa.#bbbbbbbb#.cbbbb.aaaaaaaaaa",
"aaaaaa.bbbbbbbbbbbb.bbba.aaaaaaaaaa",
"aaaaa.bbbbbbbbbbbbbb.bab#.aaaaaaaaa",
"aaaa.bbbbbbbbbbbbbbbb.b#b.aaaaaaaaa",
"aaaa.bbbbbbbbbbbbbbbb##b#.aaaaaaaaa",
"aaa.bbbbbbbbbbbbbbbbbb.#b.aaaaaaaaa",
"aaa.bbbbbbb...bbbbbbbb.b#.aaaaaaaaa",
"aa.bbbbbbb.#.a.bbbbbbb.##.aaaaaaaaa",
"aa.bbbbbb.#b.aa.bbbbbb.#.aaaaaaaaaa",
"aa.bbbbbb.b.aaa.bbbbbb.#.aaaaaaaaaa",
"aa.bbbbbb..aaaa.bbbbbb..aaaaaaaaaaa",
"aa........aaaa.bbbbbb#.aaaaaaaaaaaa",
"aaaaaaaaaaaaa.bbbbbbb.aaaaaaaaaaaaa",
"aaaaaaaaaaaa.bbbbbbb............aaa",
"aaaaaaaaaaa.bbbbbbb..............aa",
"aaaaaaaaaaa#bbbbbb.b.............aa",
"aaaaaaaaaa.bbbbbb#.#.............aa",
"aaaaaaaaaa.bbbbbb.#b.aaaa........aa",
"aaaaaaaaaa.bbbbbb.b.aa.........aaaa",
"aaaaaaaaaa.bbbbbb..#........aaaaaaa",
"aaaaaaaaaa........b.......aaaaaaaaa",
"aaaaaaaaaaa.bbbbbb.#....aaaaaaaaaaa",
"aaaaaaaaaa........#b...aaaaaaaaaaaa",
"aaaaaaaaaa.bbbbbb.b#..aaaaaaaaaaaaa",
"aaaaaaaaaa.bbbbbb.#b.aaaaaaaaaaaaaa",
"aaaaaaaaaa.bbbbbb.b.aaaaaaaaaaaaaaa",
"aaaaaaaaaa.bbbbbb..aaaaaaaaaaaaaaaa",
"aaaaaaaaaa........aaaaaaaaaaaaaaaaa",
"aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
};

/* XPM */
static char *warning1[] = {
/* width height num_colors chars_per_pixel */
"    30    29        4            1",
/* colors */
". c #000000",
"# c None",
"a c #ff0000",
"b c #ffffff",
/* pixels */
"##############################",
"#############aaaa#############",
"############aaaaa#############",
"############aaaaaa############",
"###########aaaaaaa############",
"###########aaaaaaaa###########",
"##########aaaaaaaaa###########",
"##########aaaaaaaaaa##########",
"#########aaaaaaaaaaa##########",
"#########aaaaaaaaaaaa#########",
"########aaaaaabbaaaaa#########",
"########aaaaaabbaaaaaa########",
"#######aaaaaab..baaaaa########",
"#######aaaaaab..baaaaaa#######",
"######aaaaaabb..bbaaaaa#######",
"######aaaaaabb..bbaaaaaa######",
"#####aaaaaabbb..bbbaaaaa######",
"#####aaaaaabbb..bbbaaaaaa#####",
"####aaaaaabbbb..bbbbaaaaa#####",
"####aaaaaabbbbbbbbbbaaaaaa####",
"###aaaaaabbbbb..bbbbbaaaaa####",
"###aaaaaabbbbb..bbbbbaaaaaa###",
"##aaaaaabbbbbbbbbbbbbbaaaaa###",
"##aaaaaaaaaaaaaaaaaaaaaaaaaa##",
"#aaaaaaaaaaaaaaaaaaaaaaaaaaaa#",
"#aaaaaaaaaaaaaaaaaaaaaaaaaaaa#",
"aaaaaaaaaaaaaaaaaaaaaaaaaaaaa#",
"#aaaaaaaaaaaaaaaaaaaaaaaaaaa##",
"##############################"
};



/* Display all acedb popup messages.
 * query: some messages need "Yes" and "No" buttons.
 * msgopt: some messages should offer the user the chance to see all further
 *         messages in a scrolled window list. */
static BOOL modalDialog (char *text, char *buttonLabel, BOOL query, BOOL msgopt)
{
  GdkPixmap *pixmap;
  GtkStyle  *style;
  GdkBitmap *mask;
  GtkWidget *pixmapwid;
  GtkWidget *topbox;
  GtkWidget *hbox;
  GtkWidget *checkbox ;
  GtkWidget *yes;
  GtkWidget *no = NULL;
  GtkWidget *dialog ;
  GtkWidget *button ;
  char *ourtext ;
  char **icon = query ? q_mark3 : warning1;
  BOOL isOk;
  BOOL msg_window = FALSE ;
  Graph old_graph ;
  

  old_graph = graphActive();				    /* vital if we are not to lose the
							       current graph. */

  graphGtkSetSplash(0);					    /* kill any splash screen */


  /* Set up the message text in a button widget so that we can implement cut/paste of
   * message text for bug reporting. */
  ourtext = split_message(text);
  button = gtk_button_new_with_label(ourtext);
  messfree(ourtext);					    /* button makes copy of text so don't
							       need ours. */
  gtk_button_set_relief(GTK_BUTTON(button), GTK_RELIEF_NONE) ;
  gtk_selection_add_targets(button, GDK_SELECTION_PRIMARY,
			    targetlist, ntargets);
  gtk_signal_connect(GTK_OBJECT(button), "clicked",
		     GTK_SIGNAL_FUNC(butClick), button) ;
  gtk_signal_connect(GTK_OBJECT(button), "selection_get",
		     GTK_SIGNAL_FUNC(graphGtkSelectionGet), NULL);

  dialog = gtk_dialog_new() ;
  GTK_WINDOW(dialog)->type = GTK_WINDOW_TOPLEVEL;     /* GTK_WINDOW_DIALOG gone from GTK 2.0 */
  
  /* make sure the underlying X resources exist before doing the pixmap */
  gtk_widget_realize(dialog);

  style = gtk_widget_get_style(GTK_WIDGET(dialog));
  pixmap = gdk_pixmap_create_from_xpm_d(dialog->window,  &mask,
					&style->bg[GTK_STATE_NORMAL],
					(gchar **)icon );
  pixmapwid = gtk_pixmap_new( pixmap, mask );

  /* Holds popup message + maybe other buttons.                              */
  topbox = gtk_vbox_new(FALSE, 0);

  /* Holds the message + icon.                                               */
  hbox = gtk_hbox_new(FALSE, 0);
  gtk_box_pack_start(GTK_BOX(hbox), pixmapwid, FALSE, FALSE, 5);
  gtk_box_pack_start(GTK_BOX(hbox), button, FALSE, FALSE, 10);
  gtk_box_pack_start(GTK_BOX(topbox), hbox, FALSE, FALSE, 5);

  /* If required, create a checkbox to allow user to switch to a scrolled    */
  /* message window.                                                         */
  if (msgopt)
    {
      checkbox = gtk_check_button_new_with_label("Switch to Scrolled Message Window") ;
      gtk_box_pack_start(GTK_BOX(topbox), checkbox, TRUE, TRUE, 5);
      gtk_signal_connect(GTK_OBJECT(checkbox), "toggled",
			 (GtkSignalFunc)msgWindowSig, (gpointer)&msg_window);
    }

  if (query)
    { 
      yes = gtk_button_new_with_label("Yes");
      no = gtk_button_new_with_label("No");
    }
  else
    yes = gtk_button_new_with_label(buttonLabel);

  gtk_window_set_title(GTK_WINDOW(dialog), "Please Reply");
 
  gtk_signal_connect(GTK_OBJECT(dialog), "delete_event", 
		     (GtkSignalFunc) gexLoopReturnEvent,
		     (gpointer)FALSE);

  gtk_signal_connect(GTK_OBJECT(yes), "clicked",
		     (GtkSignalFunc) gexLoopReturnSig,
		     (gpointer) TRUE);
  gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->action_area), yes,
		     TRUE, TRUE, 0);
  if (query)
    {
      gtk_signal_connect(GTK_OBJECT(no), "clicked",
			 (GtkSignalFunc) gexLoopReturnSig,
			 (gpointer) FALSE);
      
      gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->action_area), no,
			 TRUE, TRUE, 0);
    }

  gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->vbox), topbox,
		     TRUE, TRUE, 5);


  gdk_pixmap_unref(pixmap);
  gdk_pixmap_unref(mask);
  gtk_window_position(GTK_WINDOW(dialog), GTK_WIN_POS_CENTER);
  gtk_widget_show_all(dialog);


  /* Try to make the "yes" button the default.... */
#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
  /* BIZARRELY THE BUTTON WIDGETS CANNOT DEFAULT....SIGH....SO THIS HAS NO EFFECT... */
  if (GTK_WIDGET_CAN_DEFAULT(yes))
    {
      GTK_WIDGET_SET_FLAGS(yes, GTK_CAN_DEFAULT) ;
      gtk_widget_grab_default(yes) ;
    }
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

  /* BUT THIS SEEMS TO DO THE TRICK..... */
  if (GTK_WIDGET_CAN_FOCUS(yes))
    gtk_widget_grab_focus(yes) ;



#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
  /* Might be possible to use this to set the default but would require rewriting quite a lot of
   * this to use the new dialog styles.... */
  gtk_dialog_set_default_response (GtkDialog *dialog,
				   gint response_id);
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */



  isOk = gexMainLoop(dialog);

  gtk_widget_destroy(dialog);

  /* Switch to scrolled message window if required.                          */
  if (msg_window)
    {
      msg_use_list_G = TRUE ;

      /* This allows the application to take control of the switch to the    */
      /* message window, in acedbs case we need this to keep user prefs. in  */
      /* step with message window switching.                                 */
      if (msg_prefcb_G)
	msg_prefcb_G(msg_window) ;
    }
  
  graphActivate(old_graph);
  return isOk;
}


/* When user clicks on the message itself, we put the message text in the cut buffer. */
static void butClick(GtkButton *button, gpointer user_data)
{
  GtkWidget *label ;
  char *label_text = NULL ;

  label = GTK_BIN(button)->child ;

  gtk_label_get(GTK_LABEL(label), &label_text) ;

  graphGtkSetSelectionOwner(GTK_WIDGET(button), label_text) ;

  return ;
}


static BOOL gexQuery (char *text)
{ 
  return modalDialog(text, 0, TRUE, FALSE);
}

/* Handles informational/error messages, and sadly sometimes crash messages... */
static void gexOut(char *mesg_buf, void *user_pointer)
{
  /* this is a dreadful hack, acedb registers a crash handler that then
   * calls messout, so it completely subverts my attempt to detect the
   * exit/crash cases....sigh... */
  if (strstr(mesg_buf, "FATAL ERROR") != NULL
      || strstr(mesg_buf, "EXIT") != NULL
      || strstr(mesg_buf, "Sorry for the crash") != NULL)
    (void)modalDialog(mesg_buf, "Exit", FALSE, FALSE);
  else
    (void)modalDialog(mesg_buf, "Continue", FALSE, TRUE);

  return ;
}

/* Handles fatal messages from messExit, messcrash. */
static void gexExit(char *mesg_buf, void *user_pointer)
{
  (void)modalDialog(mesg_buf, "Exit", FALSE, FALSE) ;

  return ;
}


static ACEIN promptAnswer_in;
static STORE_HANDLE promptAnswer_handle;
static char *promptFormat;
static BOOL promptDoneError;

static gint graphModalDialogCheck (GtkWidget *w, gpointer data)
{ 
  GtkWidget *entry = (GtkWidget *)data;
  GtkWidget *label;
  char *input = (char *)gtk_entry_get_text(GTK_ENTRY(entry));

  promptAnswer_in = aceInCreateFromText (input, "", promptAnswer_handle);
  aceInSpecial (promptAnswer_in, "");
  aceInCard (promptAnswer_in);

  if (aceInCheck(promptAnswer_in, promptFormat))
    {
      gexLoopReturn(TRUE);
    }
  else if (!promptDoneError)
    { 
      GtkWidget *dialog = entry;
      while (dialog && !GTK_IS_DIALOG(dialog))
	dialog = dialog->parent;
      label = gtk_label_new("Sorry, invalid response. Try again or cancel");
      gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->vbox), label,
			 TRUE, TRUE, 0);
      gtk_widget_show(label);
      promptDoneError = TRUE;
    }
  
  return TRUE;
}


static ACEIN gexPrompt(char *prompt, char *dfault, char *fmt, STORE_HANDLE handle)
{
  GtkWidget *dialog, *ok, *cancel, *entry, *label ;
  char *ourtext ;
  BOOL isOk ;
  Graph old_graph ;

  old_graph = graphActive() ;

  promptAnswer_in = NULL;
  promptFormat = fmt;
  promptDoneError = FALSE;
  promptAnswer_handle = handle;

  dialog = gtk_dialog_new() ;
  gtk_window_set_title(GTK_WINDOW(dialog), "Please Reply") ;
  gtk_signal_connect(GTK_OBJECT(dialog), "delete_event", 
		     (GtkSignalFunc) gexLoopReturnEvent,
		     (gpointer)FALSE);

  ourtext = split_message(prompt);
  label = gtk_label_new(ourtext) ;
  gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->vbox), label,
		     TRUE, TRUE, 5);
  messfree(ourtext);

  entry = gtk_entry_new() ;
  gtk_signal_connect(GTK_OBJECT(entry), "activate",
		     (GtkSignalFunc) graphModalDialogCheck,
		     (gpointer) entry);
  gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->vbox), entry,
		     TRUE, TRUE, 0);

  cancel = gtk_button_new_with_label("Cancel") ;
  gtk_signal_connect(GTK_OBJECT(cancel), "clicked",
		     (GtkSignalFunc) gexLoopReturnSig,
		     (gpointer) FALSE);
  gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->action_area), cancel,
		     TRUE, TRUE, 0);

  ok = gtk_button_new_with_label("Ok") ;
  gtk_signal_connect(GTK_OBJECT(ok), "clicked",
		     (GtkSignalFunc) graphModalDialogCheck,
		     (gpointer) entry);
  gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->action_area), ok,
		     TRUE, TRUE, 0);


  gtk_container_border_width(GTK_CONTAINER(GTK_DIALOG(dialog)->vbox), 10);

  if (dfault)
    gtk_entry_set_text(GTK_ENTRY(entry), dfault);

  gtk_widget_grab_focus(entry);
  
  GTK_WIDGET_SET_FLAGS (ok, GTK_CAN_DEFAULT);
  gtk_widget_grab_default(ok);


  gtk_window_position(GTK_WINDOW(dialog), GTK_WIN_POS_CENTER);

  gtk_widget_show_all(dialog);

  if (modal_dialogs_G)
    isOk = gexMainLoop(dialog);
  else
    isOk = gexMainLoop(NULL);

  gtk_widget_destroy(dialog);
  
  if (!isOk && promptAnswer_in)
    aceInDestroy(promptAnswer_in) ;

  graphActivate(old_graph);

  return promptAnswer_in ;
} /* gexPrompt */





/***********************************************************************/
/* Text editor window: This is just the GtkText widget, wrapped up
   for acedb ease-of-use */

static void editorFinalise(void *p)
{
  GexEditor editor = (GexEditor)p ;
  
  gdk_window_get_size(editor->window->window, &editorWidth, &editorHeight);
  gtk_widget_destroy(editor->window);
}

/* editorDeleteEvent ********************************************
** Activated by X button, top right corner.  Call the received
** Cancel routine and return.  If no Cancel routine received,
** destroy the top level text window widget and return.
****************************************************************/
static gint editorDeleteEvent(GtkWidget *w, GdkEventAny *event, gpointer data)
{
  GexEditor editor = (GexEditor)data ;
  CBRoutine CB;
  
  if (editor->cancelCB)
    {
      CB = editor->cancelCB;
      (*CB)(editor, editor->data);
    }
  else
    gtk_widget_destroy(editor->window);
  
  return TRUE;
}

/* editorOK *****************************************************
** call the received OK routine & return.  If no OK routine
** received, destroy the top level text window widget and return.
****************************************************************/
gint editorOK(GtkWidget *w, gpointer data)
{
  GexEditor editor = (GexEditor)data ;
  CBRoutine CB;
  
  if (editor->okCB)
    {
      CB = editor->okCB;
      (*CB)(editor, editor->data);
    }
  else
    gtk_widget_destroy(editor->window);
  
  return TRUE;
}

/* editorCancel **************************************************
** call the received Cancel routine and return.  If no Cancel
** routine received, destroy top level text window widget and return.
*****************************************************************/
gint editorCancel(GtkWidget *w, gpointer data)
{
  GexEditor editor = (GexEditor)data ;
  CBRoutine CB;
 
  if (editor->cancelCB)
    {
      CB = editor->cancelCB;
      (*CB)(editor, editor->data);
    }
  else
    {
      gtk_widget_destroy(editor->window);
    }
  
  return TRUE;
}

#ifdef GTK2

/* Quit ***********************************************************
** destroy the top level text window widget and exit
******************************************************************/
static void Quit(GtkWidget *widget,  gpointer data)
{
  GexEditor editor = (GexEditor)data ;
  CBRoutine CB;


  if (editor->cancelCB)
    {
      CB = editor->cancelCB;
      (*CB)(editor, editor->data);
    }
  else if (editor->window)
    {
      gtk_widget_destroy(editor->window);
    }
  else
    {
      gtk_widget_destroy(gtk_widget_get_toplevel(widget));
    }

  return;
}



/* tbPrint tbOK pmQuit *****************************************
** NB keep these functions together, so this comment only 
** has to be written once.
**
** Toolbar buttons and popup menu entries send the parameters
** to their callback functions in opposite order, so we have 
** to have wrapper functions if we want to have alternative
** ways of calling the same function.  This is really tacky.
**
** Toolbar buttons call their functions via these wrappers,
** while popup menu buttons call them directly.
****************************************************************/
static void tbPrint(GtkWidget *widget, gpointer data)
{
  gexPrint(widget, data) ;

  return;
}

static void tbOK(GtkWidget *widget, gpointer data)
{
  editorOK(widget, data) ;

  return;
}

static void pmQuit(gpointer data, GtkWidget *widget)
{
  GexEditor editor = (GexEditor) data ;

  Quit(editor, widget);

  return;
}

static void tbSelectAll(GtkWidget *widget, gpointer data)
{
  editSelectAll(data, widget);

  return;
}

#endif  /* ifdef GTK2 */


/* gexTextEditorNew
 * 
 * Create text window and display text received in initText.
 *
 */
GexEditor gexTextEditorNew(char *title, char *initText, STORE_HANDLE handle,
			   void *data, CBRoutine okCB, CBRoutine cancelCB,
			   BOOL editable, BOOL word_wrap, BOOL fixed_font)
{

#ifdef GTK2

  GexEditor editor ;
  GtkWidget *toolbar;
  GtkWidget *vbox;
  GtkWidget *hbox;
  GtkWidget *sWindow;
  struct PrintParams *printParams ;

  editor = (GexEditor)handleAlloc(editorFinalise, handle, sizeof(GexEditorStruct)) ;

  printParams = (struct PrintParams *)messalloc(sizeof(struct PrintParams)) ;

  printParams->file_number   = 1;
  printParams->title = strnew(title, 0);

  editor->printParams = printParams;
  editor->data = data ;

  if (okCB)
    editor->okCB = okCB ;
  if (cancelCB)
    editor->cancelCB = cancelCB ;
  
  /* top level window */
  editor->window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
  gtk_window_set_title(GTK_WINDOW(editor->window), title);
  gtk_window_set_default_size(GTK_WINDOW(editor->window), 
			      editorWidth, editorHeight);
  gtk_widget_set_name(editor->window, "acedb editor window");

  /* vbox to hold everything */
  vbox = gtk_vbox_new(FALSE, 1);
  gtk_container_add(GTK_CONTAINER(editor->window), vbox);

  /* toolbar at top of vbox */
  toolbar = gtk_toolbar_new();
  gtk_box_pack_start(GTK_BOX(vbox), toolbar, FALSE, TRUE, 0);
  gtk_toolbar_append_item(GTK_TOOLBAR(toolbar),
			  "Quit",                     /* text on button  */
			  NULL,                       /* tooltip         */
			  NULL,                       /* private tooltip */
			  NULL,                       /* icon            */
			  (GtkSignalFunc)(Quit),      /* callback func   */
			  editor);			    /* user data       */

  gtk_toolbar_append_space(GTK_TOOLBAR(toolbar));

  if (editable)  /* implies we've been called from treedisp.c */
    {
      gtk_toolbar_append_item(GTK_TOOLBAR(toolbar), "OK",
			      NULL, NULL, NULL,
			      (GtkSignalFunc)tbOK,
			      (gpointer)editor);
    }

  gtk_toolbar_append_item(GTK_TOOLBAR(toolbar),
			  "Print", NULL, NULL, NULL,
			  (GtkSignalFunc)(tbPrint), editor);
  gtk_toolbar_append_item(GTK_TOOLBAR(toolbar),
			  "Select All", NULL, NULL, NULL,
			  (GtkSignalFunc)(tbSelectAll), editor);
  gtk_toolbar_append_item(GTK_TOOLBAR(toolbar),
			  "Help", NULL, NULL, NULL,
			  (GtkSignalFunc)(help), NULL);


  /* hbox to hold scrolling window and vertical scrollbar */
  hbox = gtk_hbox_new(FALSE, 0);
  gtk_box_pack_start(GTK_BOX(vbox), hbox, TRUE, TRUE, 0) ;


  /* scrollable window to hold the text. N.B. GTK_POLICY_ALWAYS appears to be vital to get the text to
   * justify in the way that you would intuitively expect. */
  sWindow = gtk_scrolled_window_new(NULL, NULL);
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(sWindow), GTK_POLICY_ALWAYS, GTK_POLICY_ALWAYS) ;
  gtk_box_pack_start(GTK_BOX(hbox), sWindow, TRUE, TRUE, 0);


  /* set up editor */
  editor->view = gtk_text_view_new();

  editor->text = gtk_text_view_get_buffer(GTK_TEXT_VIEW(editor->view)) ;
  if (initText)
    gtk_text_buffer_set_text(GTK_TEXT_BUFFER(editor->text), initText, -1) ;

  if (editable)
    gtk_text_view_set_editable(GTK_TEXT_VIEW(editor->view), TRUE);
  else
    gtk_text_view_set_editable(GTK_TEXT_VIEW(editor->view), FALSE);

  /* If you don't do this the text goes right up to the edge of the box and makes it look like
   * some is missing. */
  gtk_text_view_set_left_margin(GTK_TEXT_VIEW(editor->view), 5);
  gtk_text_view_set_right_margin(GTK_TEXT_VIEW(editor->view), 5);

  /* Should text be wrapped, e.g. as window is resized. */
  if (word_wrap)
    gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(editor->view), GTK_WRAP_WORD);

  /* Try to set a fixed width font from a list of possibles. */
  if (fixed_font)
    {
      GList *fixed_font_list = NULL ;
      PangoFont *font ;
      PangoFontDescription *font_desc ;

      fixed_font_list = g_list_append(fixed_font_list, "Monospace") ;
      fixed_font_list = g_list_append(fixed_font_list, "fixed") ;
      if (getFixedWidthFont(editor->view,
			    fixed_font_list, 10, PANGO_WEIGHT_NORMAL,
			    &font, &font_desc))
	gtk_widget_modify_font(editor->view, font_desc) ;

      g_list_free(fixed_font_list) ;
    }

  gtk_container_add(GTK_CONTAINER(sWindow), editor->view);

  gtk_signal_connect(GTK_OBJECT(editor->window), "delete_event",
		     GTK_SIGNAL_FUNC(editorDeleteEvent),
		     (gpointer)editor);

  gtk_signal_connect(GTK_OBJECT(editor->window), "button_press_event",
		     GTK_SIGNAL_FUNC(ButtonPressCallback), editor);

  gtk_widget_show_all(editor->window);

  return editor ;


#else  /* ie not GTK2 ****************************************/

  GexEditor editor = (GexEditor)handleAlloc(editorFinalise, handle, sizeof(struct GexEditorStruct));
  GtkWidget *table;
  GtkWidget *vscrollbar;
  GtkWidget *bbox;

  editor->data = data;
  editor->okCB = okCB;
  editor->cancelCB = cancelCB;
  
  editor->window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
  gtk_window_set_title(GTK_WINDOW(editor->window), title);
  gtk_window_set_default_size(GTK_WINDOW(editor->window), 
			      editorWidth, editorHeight);
  gtk_widget_set_name(editor->window, "acedb editor window");

  table = gtk_table_new(2, 2, FALSE);
  gtk_table_set_row_spacing(GTK_TABLE(table), 0, 2);
  gtk_table_set_col_spacing(GTK_TABLE(table), 0, 2);

  editor->text = gtk_text_new(NULL, NULL);
  gtk_text_set_editable(GTK_TEXT(editor->text), TRUE);
  gtk_text_set_word_wrap(GTK_TEXT(editor->text), TRUE);
  if (initText)
    gtk_text_insert(GTK_TEXT(editor->text), NULL, NULL, NULL,
		    initText, strlen(initText));
		    
  editor->ok = gtk_button_new_with_label("OK");
  editor->cancel = gtk_button_new_with_label("Cancel");
  bbox = gtk_hbutton_box_new();
  gtk_button_box_set_layout(GTK_BUTTON_BOX(bbox), GTK_BUTTONBOX_END);
  gtk_button_box_set_spacing(GTK_BUTTON_BOX(bbox), 20);
  gtk_box_pack_start(GTK_BOX(bbox), editor->ok, TRUE, TRUE, 0);
  gtk_box_pack_start(GTK_BOX(bbox), editor->cancel, TRUE, TRUE, 0);
  
   
  vscrollbar = gtk_vscrollbar_new(GTK_TEXT(editor->text)->vadj);

  gtk_container_add(GTK_CONTAINER(editor->window), table);
  gtk_table_attach(GTK_TABLE(table), editor->text, 0, 1, 0, 1,
		   GTK_EXPAND | GTK_SHRINK | GTK_FILL,
		   GTK_EXPAND | GTK_SHRINK | GTK_FILL, 0, 0);

  gtk_table_attach(GTK_TABLE(table), vscrollbar, 1, 2, 0, 1,
		   0, GTK_EXPAND | GTK_FILL | GTK_SHRINK, 0, 0);

  gtk_table_attach(GTK_TABLE(table), 
		   bbox, 0, 1, 1, 2, GTK_FILL, GTK_FILL, 5, 4); 
  

  gtk_signal_connect(GTK_OBJECT(editor->cancel), "clicked",
		     GTK_SIGNAL_FUNC(editorCancel),
		     (gpointer)editor);
  gtk_signal_connect(GTK_OBJECT(editor->window), "delete_event",
		     GTK_SIGNAL_FUNC(editorDeleteEvent),
		     (gpointer)editor);
  gtk_signal_connect(GTK_OBJECT(editor->ok), "clicked",
		     GTK_SIGNAL_FUNC(editorOK),
		     (gpointer)editor);

  gtk_widget_grab_focus(editor->text);
  gtk_widget_show_all(editor->window);

  return editor ;
#endif /* ifdef GTK2  */
}


char *gexEditorGetText(GexEditor ep, STORE_HANDLE handle)
{
  GexEditor editor = (GexEditor)ep ;
  char *val ;
  char *ret ;

#ifdef GTK2
  {
    GtkTextIter start, end ;

    gtk_text_buffer_get_bounds(GTK_TEXT_BUFFER(editor->text), &start, &end) ;

    val = gtk_text_buffer_get_text(GTK_TEXT_BUFFER(editor->text), &start, &end, FALSE) ;
  }
#else
  val = gtk_editable_get_chars(GTK_EDITABLE(editor->text),
#endif

  ret = strnew(val, handle);

  g_free(val);

  return ret;
}

/* blocking interface is below here */

static void editorBlockCancel(void *ep, void *arg)
{
  gexLoopReturn(FALSE);
}

static void editorBlockOK(void *ep, void *arg)
{
  gexLoopReturn(TRUE);
}

char *gexTextEditor(char *title, char *initText, STORE_HANDLE handle)
{
  char *ret;
  Graph old_graph = graphActive();
  GexEditor editor = gexTextEditorNew(title, initText, handle, 0,
				      editorBlockOK, 
				      editorBlockCancel,
				      TRUE, TRUE, TRUE) ;   /* editable   */ 
  
  if (gexMainLoop(editor->window))
    ret = gexEditorGetText(editor, handle);
  else
    ret = NULL;

  messfree(editor); /* destroys all via callback */
  graphActivate(old_graph);

  return ret;
}

/*********************************************************************
** signal call back func to call up help pages from buttons/menuitems
**********************************************************************/

void gexHelpOnSig (GtkWidget *widget, gpointer subject)
{
  helpOn((char*)subject);
}


#ifdef GTK2

static GtkWidget *gMenu = NULL ;  /* think this is redundant */


/* ButtonPressCallback ******************************************
** activate popup menu for gexTextEditorNew window 
*****************************************************************/
static gint ButtonPressCallback(GtkWidget *widget, 
				GdkEventButton *event,
				gpointer callback_data)
{
  GexEditor editor = (GexEditor)callback_data ;
  GtkAccelGroup  *accel_group;


  accel_group = gtk_accel_group_new();

  /* attach the accelorator group to the top level text editor window */
  /*  gtk_accel_group_attach(accel_group, GTK_OBJECT(editor->window));*/


  /* now actually pop the menu up */
  if (event->button == 3)                                      /* ie right button only */
    {
      GtkWidget *menu;
      GtkWidget *menu_item;

      menu = gtk_menu_new();

      menu_item = gtk_menu_item_new_with_label("Quit");
      gtk_menu_append(GTK_MENU(menu), menu_item);
      gtk_signal_connect(GTK_OBJECT(menu_item), "activate",
			 GTK_SIGNAL_FUNC(pmQuit), editor->window);
      gtk_widget_show(menu_item);

      if (editor->okCB)
	{
	  menu_item = gtk_menu_item_new_with_label("OK");
	  gtk_menu_append(GTK_MENU(menu), menu_item);
	  gtk_signal_connect(GTK_OBJECT(menu_item), "activate",
			     GTK_SIGNAL_FUNC(editor->okCB), editor);
	  gtk_widget_show(menu_item);
	}

      menu_item = gtk_menu_item_new_with_label("Print");
      gtk_menu_append(GTK_MENU(menu), menu_item);
      gtk_signal_connect(GTK_OBJECT(menu_item), "activate",
			 GTK_SIGNAL_FUNC(gexPrint), editor);
      gtk_widget_show(menu_item);

      menu_item = gtk_menu_item_new_with_label("Help");
      gtk_menu_append(GTK_MENU(menu), menu_item);
      gtk_signal_connect(GTK_OBJECT(menu_item), "activate",
			 GTK_SIGNAL_FUNC(help), editor);
      gtk_widget_show(menu_item);

      gMenu = menu ;
                                                           /* defaults to pointer location */
      gtk_menu_popup(GTK_MENU(menu), NULL, NULL, NULL, NULL, 
		     event->button, event->time);      
    }

  return TRUE;                                  /* tell GTK this event has been handled */
}




/* gexPrint *****************************************************
** 
****************************************************************/
static void gexPrint(GtkWidget *widget, gpointer callback_data)
{
#ifdef __CYGWIN__
  /* This allows the code to compile on Windows and a print dialogue to appear
  ** but when you print you just get the fmap.  Right now we think Windows
  ** users probably won't need this facility, so we're not implementing it.
  */
  graphPrintGDI(gActive);

#else

  GexEditor editor ;
  struct PrintParams *printParams;
  GtkWidget 
    *printWindow, 
    *vbox, *vbox2, *hbox, 
    *toolbar,
    *separator, 
    *label, *printlabel, *addrlabel, *filelabel,
    *docTitle, *addrentry, *fileentry,
    *sWindow, 
    *button, *chooserbutton, 
    *clist; /* *format */
  Array printers = getPrinters();
  int i;

  if (gMenu)
    {
      gtk_menu_popdown(GTK_MENU(gMenu)) ;
      gMenu = NULL ;
    }

  editor = (GexEditor)callback_data;
 
  printParams = (struct PrintParams*)editor->printParams;
  arraySort(printers, arrstrcmp);

  printWindow = gtk_window_new(GTK_WINDOW_TOPLEVEL);
  gtk_window_set_title(GTK_WINDOW(printWindow), "Print/Mail Selection (pfetch)");
  gtk_window_set_default_size(GTK_WINDOW(printWindow), 400, 400);
  gtk_window_set_position(GTK_WINDOW(printWindow), GTK_WIN_POS_NONE);
  gtk_container_border_width(GTK_CONTAINER(printWindow), 5);

  gtk_signal_connect(GTK_OBJECT(printWindow), "delete_event",
		     GTK_SIGNAL_FUNC(editorDeleteEvent),
		     (gpointer)editor);

  /* vbox to hold everything */
  vbox = gtk_vbox_new(FALSE, 1);
  gtk_container_add(GTK_CONTAINER(printWindow), vbox);

  /* toolbar at top of vbox */
  toolbar = gtk_toolbar_new();
  gtk_box_pack_start(GTK_BOX(vbox), toolbar, FALSE, TRUE, 5);
  gtk_toolbar_append_item(GTK_TOOLBAR(toolbar),
			  "OK",                       /* text on button  */
			  NULL,                       /* tooltip         */
			  NULL,                       /* private tooltip */
			  NULL,                       /* icon            */
			  (GtkSignalFunc)(printOK),   /* callback func   */
			  printParams);               /* user data       */
  gtk_toolbar_append_space(GTK_TOOLBAR(toolbar));
  gtk_toolbar_append_item(GTK_TOOLBAR(toolbar),
			  "Cancel", NULL, NULL, NULL,
			  (GtkSignalFunc)(Quit), printWindow);

  separator = gtk_hseparator_new();
  gtk_box_pack_start(GTK_BOX(vbox), separator, FALSE, FALSE, 0);

  /* Document title entry box in its own hbox */
  hbox = gtk_hbox_new(FALSE, 1);
  label = gtk_label_new("Document title:");
  gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);
  docTitle = gtk_entry_new();
  gtk_entry_set_text(GTK_ENTRY(docTitle), printParams->title); 
  gtk_box_pack_start(GTK_BOX(hbox), docTitle, TRUE, TRUE, 20);
  gtk_box_pack_start(GTK_BOX(vbox), hbox,  FALSE, FALSE, 0);

  separator = gtk_hseparator_new();
  gtk_box_pack_start(GTK_BOX(vbox), separator, FALSE, FALSE, 0);

  /* Printer selection stuff in its own hbox */
  hbox = gtk_hbox_new(FALSE, 0);

  /* Toggle button for printing */
  button = gtk_toggle_button_new_with_label("Print");
  vbox2 = gtk_vbox_new(FALSE, 0);
  gtk_box_pack_start(GTK_BOX(vbox2), button, FALSE, FALSE, 0);
  gtk_box_pack_start(GTK_BOX(hbox), vbox2, TRUE, TRUE, 0);
  gtk_signal_connect(GTK_OBJECT(button), "clicked",
		     GTK_SIGNAL_FUNC(PrintSelected),
		     printParams);

  /* "Printers" label needs its own box for alignment */
  vbox2 = gtk_vbox_new(FALSE, 0);
  printlabel = gtk_label_new("Printers:  ");
  gtk_widget_set_sensitive(GTK_WIDGET(printlabel), FALSE);
  gtk_box_pack_start(GTK_BOX(vbox2), printlabel, FALSE, FALSE, 0);
  gtk_box_pack_start(GTK_BOX(hbox), vbox2, TRUE, TRUE, 0);

  /* now sort out the list of printers */
  clist = gtk_clist_new(1);

  for (i = 0; i < arrayMax(printers); i++)
    {
      if (arrayp(printers, i, char*))
	gtk_clist_append(GTK_CLIST(clist), arrayp(printers, i, char*));
    }

  /* scrollable window to hold the list of printers */
  sWindow = gtk_scrolled_window_new(NULL, NULL);
  gtk_widget_set_usize(sWindow, -2, 180);
  gtk_container_add(GTK_CONTAINER(sWindow), clist);
  gtk_widget_set_sensitive(GTK_WIDGET(sWindow), FALSE);
  gtk_box_pack_start(GTK_BOX(hbox), sWindow, TRUE , TRUE , 0);
  gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 10);

  gtk_signal_connect(GTK_OBJECT(sWindow), "button_press_event",
		     GTK_SIGNAL_FUNC(printerSelection),
		     printParams);

  separator = gtk_hseparator_new();
  gtk_box_pack_start(GTK_BOX(vbox), separator, FALSE, FALSE, 0);

  /* hbox to hold Mail toggle button, label and address entry box */
  hbox = gtk_hbox_new(FALSE, 0);
  button = gtk_toggle_button_new_with_label("Mail");
  gtk_box_pack_start(GTK_BOX(hbox), button, TRUE, TRUE, 0);
  gtk_signal_connect(GTK_OBJECT(button), "clicked",
		     GTK_SIGNAL_FUNC(MailSelected),
		     printParams);

  addrlabel = gtk_label_new("Address:");
  gtk_widget_set_sensitive(GTK_WIDGET(addrlabel), FALSE);
  gtk_box_pack_start(GTK_BOX(hbox), addrlabel, TRUE, TRUE, 0);

  addrentry = gtk_entry_new();
  gtk_entry_set_text(GTK_ENTRY(addrentry), getLogin(TRUE));
  gtk_widget_set_sensitive(GTK_WIDGET(addrentry), FALSE);
  gtk_box_pack_start(GTK_BOX(hbox), addrentry, TRUE, TRUE, 0);
  gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 10);

  separator = gtk_hseparator_new();
  gtk_box_pack_start(GTK_BOX(vbox), separator, FALSE, FALSE, 0);

  /* hbox to hold Copy toggle button, label and file chooser button */
  hbox = gtk_hbox_new(FALSE, 0);
  button = gtk_toggle_button_new_with_label("Copy");
  gtk_box_pack_start(GTK_BOX(hbox), button, TRUE, TRUE, 0);
  gtk_signal_connect(GTK_OBJECT(button), "clicked",
		     GTK_SIGNAL_FUNC(CopySelected),
		     printParams);

  filelabel = gtk_label_new("Keep a copy in file:");
  gtk_widget_set_sensitive(GTK_WIDGET(filelabel), FALSE);
  gtk_box_pack_start(GTK_BOX(hbox), filelabel, TRUE, TRUE, 0);

  chooserbutton = gtk_toggle_button_new_with_label("File Chooser");
  gtk_widget_set_sensitive(GTK_WIDGET(chooserbutton), FALSE);
  gtk_box_pack_start(GTK_BOX(hbox), chooserbutton, TRUE, TRUE, 0);
  gtk_signal_connect(GTK_OBJECT(chooserbutton), "clicked",
		     GTK_SIGNAL_FUNC(FileChooser),
		     printParams);
  gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 10);

  /* path of output file */
  fileentry = gtk_entry_new();
  gtk_box_pack_start(GTK_BOX(vbox), fileentry, FALSE, FALSE, 0);
  gtk_widget_set_sensitive(GTK_WIDGET(fileentry), FALSE);

  /* non-plain-text options not implemented yet */

  /* hbox to hold Format label and radio buttons */
  /* all this bit not implemented for now
  ** hbox = gtk_hbox_new(FALSE, 0);
  ** label = gtk_label_new("Format:");
  ** gtk_box_pack_start(GTK_BOX(hbox), label, TRUE, FALSE, 0);

  ** define radio buttons *
  ** format = gtk_radio_button_new_with_label(NULL, "format");

  ** button = gtk_radio_button_new_with_label(gtk_radio_button_group(GTK_RADIO_BUTTON(format)),
  **					   "Postscript");
  ** gtk_box_pack_start(GTK_BOX(hbox), button, FALSE, FALSE, 0);
  ** gtk_signal_connect(GTK_OBJECT(button), "clicked",
  **		     GTK_SIGNAL_FUNC(FormatSelected),
  **	     button);
  ** button = gtk_radio_button_new_with_label(gtk_radio_button_group(GTK_RADIO_BUTTON(format)),
  **					   "Colour Postscript");
  ** gtk_box_pack_start(GTK_BOX(hbox), button, TRUE, FALSE, 0);
  ** gtk_signal_connect(GTK_OBJECT(button), "clicked",
  **		     GTK_SIGNAL_FUNC(FormatSelected),
  **		     button);
  ** button = gtk_radio_button_new_with_label(gtk_radio_button_group(GTK_RADIO_BUTTON(format)),
  **				   "GIF");
  ** gtk_box_pack_start(GTK_BOX(hbox), button, TRUE, TRUE, 0);
  ** gtk_signal_connect(GTK_OBJECT(button), "clicked",
  **		     GTK_SIGNAL_FUNC(FormatSelected),
  **		     button);
  ** button = gtk_radio_button_new_with_label(gtk_radio_button_group(GTK_RADIO_BUTTON(format)),
  **					   "Text Only");
  **  gtk_box_pack_start(GTK_BOX(hbox), button, TRUE, FALSE, 0);
  **  gtk_signal_connect(GTK_OBJECT(button), "clicked",
  **	     GTK_SIGNAL_FUNC(FormatSelected),
  **	     button);
  **  gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 10);

  ** Options also needs its own hbox *
  **  hbox = gtk_hbox_new(FALSE, 0);
  **  label = gtk_label_new("Options:");
  **  gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);
  **  button = gtk_toggle_button_new_with_label("EPSF");
  **  gtk_box_pack_start(GTK_BOX(hbox), button, FALSE, FALSE, 0);
  **  gtk_signal_connect(GTK_OBJECT(button), "clicked",
  **		     GTK_SIGNAL_FUNC(EPSFSelected),
  **		     button);
  **  gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);

  **  separator = gtk_hseparator_new();
  **  gtk_box_pack_start(GTK_BOX(vbox), separator, FALSE, FALSE, 0);
  */
  printParams->docTitle      = docTitle;
  printParams->sWindow       = sWindow;
  printParams->clist         = clist;
  printParams->printers      = printers;
  printParams->printlabel    = printlabel;
  printParams->addrlabel     = addrlabel;
  printParams->addrentry     = addrentry;
  printParams->filelabel     = filelabel;
  printParams->chooserbutton = chooserbutton;
  printParams->fileentry     = fileentry;
  printParams->text          = ((GexEditor)editor)->text;

  gtk_widget_show_all(printWindow);

  return;
#endif
}

/* editSelectAll*************************************************
** Select all the text ready for copying
****************************************************************/
static void editSelectAll(gpointer data, GtkWidget *w)
{
  GexEditor editor;

  editor = (GexEditor)data ;

#ifdef GTK2
  {
    GtkTextIter start, end ;

    gtk_text_buffer_get_bounds(editor->text, &start, &end) ;

    gtk_text_buffer_select_range(editor->text, &end, &start) ;
  }

#else
  gtk_editable_select_region(GTK_EDITABLE(editor->text), 
			     0, gtk_text_buffer_get_char_count((GtkTextBuffer *)editor->text));
#endif



  return;
}



/* printerSelection *********************************************
** record the selected printer - just store the array index.
****************************************************************/
static gint printerSelection(GtkWidget *sWindow, GdkEventButton *event, struct PrintParams *printParams)
{
  gint x, y, row, col;
  x = event->x;
  y = event->y;
  gtk_clist_get_selection_info(GTK_CLIST(printParams->clist), x, y, &row, &col);
  printParams->printer_index = row;

  return TRUE;
}

static gint PrintSelected(GtkWidget *widget, struct PrintParams *printParams)
{

  if (printParams->PRINT_SELECTED)
    {
      printParams->PRINT_SELECTED = FALSE;
      gtk_widget_set_sensitive(GTK_WIDGET(printParams->sWindow), FALSE);
      gtk_widget_set_sensitive(GTK_WIDGET(printParams->printlabel), FALSE);
    }
  else
    {
      printParams->PRINT_SELECTED = TRUE;
      gtk_widget_set_sensitive(GTK_WIDGET(printParams->sWindow), TRUE);
      gtk_widget_set_sensitive(GTK_WIDGET(printParams->printlabel), TRUE);
    }
  return TRUE;
}

static gint MailSelected(GtkWidget *widget, struct PrintParams *printParams)
{
  if (printParams->MAIL_SELECTED)
    {
      printParams->MAIL_SELECTED = FALSE;
      gtk_widget_set_sensitive(GTK_WIDGET(printParams->addrlabel), FALSE);
      gtk_widget_set_sensitive(GTK_WIDGET(printParams->addrentry), FALSE);
    }
  else
    {
      printParams->MAIL_SELECTED = TRUE;
      gtk_widget_set_sensitive(GTK_WIDGET(printParams->addrlabel), TRUE);
      gtk_widget_set_sensitive(GTK_WIDGET(printParams->addrentry), TRUE);
    }
  return TRUE;
}

static gint CopySelected(GtkWidget *widget, struct PrintParams *printParams)
{
  char file_name[20+1];

  if (printParams->file_prefix == NULL)
    printParams->file_prefix = strnew("acedb", 0);

  if (printParams->file_suffix == NULL)
    printParams->file_suffix = strnew("txt", 0);

  if (printParams->file_path == NULL)
    {
      sprintf(file_name, "%s.%d.%s", 
	      printParams->file_prefix,
	      printParams->file_number,
	      printParams->file_suffix);
      gtk_entry_set_text(GTK_ENTRY(printParams->fileentry), file_name);
    }

  if (printParams->FILE_SELECTED)
    {
      printParams->FILE_SELECTED = FALSE;
      gtk_widget_set_sensitive(GTK_WIDGET(printParams->filelabel), FALSE);
      gtk_widget_set_sensitive(GTK_WIDGET(printParams->chooserbutton), FALSE);
      gtk_widget_set_sensitive(GTK_WIDGET(printParams->fileentry), FALSE);
    }
  else
    {
      printParams->FILE_SELECTED = TRUE;
      gtk_widget_set_sensitive(GTK_WIDGET(printParams->filelabel), TRUE);
      gtk_widget_set_sensitive(GTK_WIDGET(printParams->chooserbutton), TRUE);
      gtk_widget_set_sensitive(GTK_WIDGET(printParams->fileentry), TRUE);
    }
  return TRUE;
}


/****************************************************/
/* FileChooser - display a file selection dialogue. */
/****************************************************/
static gint FileChooser(GtkWidget *widget, struct PrintParams *printParams)
{
  GtkWidget *fileSel;
  BOOL active = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(printParams->chooserbutton));

  /* When we return from FileChooser we toggle the chooserbutton */
  /* back up, but that activates this function again, so we have */
  /* to only do this bit when the button is down, ie active.     */
  if (active) 
    {
      fileSel = gtk_file_selection_new("Select File");
      gtk_file_selection_show_fileop_buttons(GTK_FILE_SELECTION(fileSel));
      gtk_signal_connect(GTK_OBJECT(GTK_FILE_SELECTION(fileSel)->ok_button), "clicked",
			 GTK_SIGNAL_FUNC(FileChooserOK),
			 printParams);
      gtk_signal_connect(GTK_OBJECT(GTK_FILE_SELECTION(fileSel)->cancel_button), "clicked",
			 GTK_SIGNAL_FUNC(FileChooserCancel),
			 printParams);
      printParams->fileSelWindow = fileSel;
      gtk_widget_show(fileSel);
    }
  return TRUE;
}

/****************************************************/
/* FileChooserOK - get the file that the user has   */
/* selected and put it into the entry box.          */
/****************************************************/
static gint FileChooserOK(GtkWidget *widget, struct PrintParams *printParams)
{
  GtkFileSelection *fileSel = GTK_FILE_SELECTION(printParams->fileSelWindow);
  gtk_entry_set_text(GTK_ENTRY(printParams->fileentry), 
		     gtk_file_selection_get_filename(fileSel));
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(printParams->chooserbutton), FALSE);
  gtk_widget_hide(GTK_WIDGET(fileSel));
  return TRUE;
}


static gint FileChooserCancel(GtkWidget *widget, struct PrintParams *printParams)
{
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(printParams->chooserbutton), FALSE);
  gtk_widget_hide(printParams->fileSelWindow);
  return TRUE;
}

/****************************************************/
/* printOK - decide which selections have been made */
/* and implement them.                              */
/****************************************************/
static gint printOK(GtkWidget *widget, struct PrintParams *printParams)
{
  const char *subject   = gtk_entry_get_text(GTK_ENTRY(printParams->docTitle));
  const char *mailaddr  = gtk_entry_get_text(GTK_ENTRY(printParams->addrentry));
  const char *target    = gtk_entry_get_text(GTK_ENTRY(printParams->fileentry));
  char *printer   = array(printParams->printers, printParams->printer_index, char *);
  ACETMP tmpFile    = aceTmpCreate("w", 0);
  char *tmpFileName = aceTmpGetFileName(tmpFile);  


  /* write the text to the temporary file */  
  aceOutPrint(aceTmpGetOutput(tmpFile), " pfetch\n%s\n", 
	      gtk_editable_get_chars(GTK_EDITABLE(printParams->text),0,-1));
  aceTmpClose (tmpFile);


  if (printParams->PRINT_SELECTED)
    {
      system(messprintf("lpr -P%s %s", printer, tmpFileName));
    }


  if (printParams->MAIL_SELECTED)
    {
      if (tmpFile)
	{
	  system(messprintf("cat %s | Mail -s \"%s\" %s", 
			    tmpFileName, subject, mailaddr));
	}
      else
	printf("Unable to open temporary file for mailshot.");
    }


  if (printParams->FILE_SELECTED)
    {
      printParams->file_number++;
      system(messprintf("cp %s %s", tmpFileName, target));
    }

  printParams->PRINT_SELECTED = FALSE;
  printParams->MAIL_SELECTED = FALSE;  
  printParams->FILE_SELECTED = FALSE;
  aceTmpDestroy(tmpFile);
  gtk_widget_destroy(gtk_widget_get_toplevel(widget));
  return TRUE;
}

/*
**static gint FormatSelected(GtkWidget *widget, gpointer params)
**{
**
**  return TRUE;
**}
**
**static gint EPSFSelected(GtkWidget *widget, gpointer params)
**{
**
**  return TRUE;
**}
*/

#endif /* ifdef GTK2 */




/* Tries to return a fixed font from the list given in pref_families, returns
 * TRUE if it succeeded in finding a matching font, FALSE otherwise.
 * The list of preferred fonts is treated with most preferred first and least
 * preferred last.  The function will attempt to return the most preferred font
 * it finds.
 *
 * @param widget         Needed to get a context, ideally should be the widget you want to
 *                       either set a font in or find out about a font for.
 * @param pref_families  List of font families (as text strings).
 * @param points         Size of font in points.
 * @param weight         Weight of font (e.g. PANGO_WEIGHT_NORMAL)
 * @param font_out       If non-NULL, the font is returned.
 * @param desc_out       If non-NULL, the font description is returned.
 * @return               TRUE if font found, FALSE otherwise.
 */
static gboolean getFixedWidthFont(GtkWidget *widget, 
				  GList *pref_families, gint points, PangoWeight weight,
				  PangoFont **font_out, PangoFontDescription **desc_out)
{
  gboolean found = FALSE, found_most_preferred = FALSE ;
  PangoContext *context ;
  PangoFontFamily **families;
  gint n_families, i, most_preferred, current ;
  PangoFontFamily *match_family = NULL ;
  PangoFont *font = NULL;

  context = gtk_widget_get_pango_context(widget) ;

  pango_context_list_families(context, &families, &n_families) ;

  most_preferred = g_list_length(pref_families);

  for (i = 0 ; (i < n_families && !found_most_preferred) ; i++)
    {
      const gchar *name ;
      GList *pref ;

      name = pango_font_family_get_name(families[i]) ;
      current = 0;
      
      pref = g_list_first(pref_families) ;
      while(pref && ++current)
	{
	  char *pref_font = (char *)pref->data ;

	  if (g_ascii_strncasecmp(name, pref_font, strlen(pref_font)) == 0
#if GLIB_MAJOR_VERSION == 2 && GLIB_MINOR_VERSION >= 6
	      && pango_font_family_is_monospace(families[i])
#endif
	      )
	    {
	      found = TRUE ;
              if(current <= most_preferred)
                {
                  if((most_preferred = current) == 1)
                    found_most_preferred = TRUE;
                  match_family   = families[i] ;
                }
              match_family = families[i] ;
	      break ;
	    }

	  pref = g_list_next(pref) ;
	}
    }


  if (found)
    {
      PangoFontDescription *desc ;
      const gchar *name ;
      
      name = pango_font_family_get_name (match_family) ;

      desc = pango_font_description_from_string(name) ;
      pango_font_description_set_family(desc, name) ;
      pango_font_description_set_size  (desc, points * PANGO_SCALE) ;
      pango_font_description_set_weight(desc, weight) ;

      font = pango_context_load_font(context, desc) ;
      messAssert(font) ;

      if (font_out)
	*font_out = font ;
      if (desc_out)
	*desc_out = desc ;

      found = TRUE ;
    }
  
  return found ;
}



/************************* eof ******************************/
