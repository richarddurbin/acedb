/*  File: graphmesglist.c
 *  Author: Ed Griffiths (edgrif@sanger.ac.uk)
 *  Copyright (c) J Thierry-Mieg and R Durbin, 2001
 *-------------------------------------------------------------------
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
 *-------------------------------------------------------------------
 * This file is part of the ACEDB genome database package, written by
 * 	Richard Durbin (Sanger Centre, UK) rd@sanger.ac.uk, and
 *	Jean Thierry-Mieg (CRBM du CNRS, France) mieg@kaa.crbm.cnrs-mop.fr
 *
 * Description: Displays any normal or error messages in a rolling
 *              list rather than in individual popup dialogs. The
 *              glib single linked-list stuff is used to hold the
 *              messages.
 *              
 *              IMPORTANT to note is that although this is currently
 *              done via graphs, it's controlled from gex.c and could
 *              simply be redone in gex/GTK type stuff which would
 *              give cut/paste.
 *              
 *              KNOWN PROBLEMS:
 *                              if the code outputs several error
 *              messages and then a crash message, the error messages
 *              don't get seen because the code kills the message
 *              window before the messages can be seen.
 *              
 * Exported functions: See mesglist.h
 * HISTORY:
 * Last edited: Apr  7 13:21 2008 (edgrif)
 * * Jul  5 13:29 2001 (edgrif): Fix resizing bug, crass stuff, this
 *              now seems to be a pretty solid working window.
 * Created: Mon Jun  4 17:06:40 2001 (edgrif)
 * CVS info:   $Id$
 *-------------------------------------------------------------------
 */

#include <wh/mystdlib.h>
#include <glib.h>
#include <wh/aceio.h>
#include <wh/acedb.h>
#include <w2/graph_.h>

/************************************************************/

/* Holds all the state for a message list, currently the message list is     */
/* actually static because of deficiencies in the graph package but it       */
/* doesn't have to be static.                                                */
typedef struct _MesgListStateStruct
{
  GSList *messages ;
  int max_messages ;
  Graph graph ;
  MENUOPT *menu ;
  STORE_HANDLE handle ;
  GraphFunc app_destroy_cb ;
} MesgListStateStruct, *MesgListState ;


/************************************************************/

static void initMsgList(void) ;
static MesgListState getMsgList(char *calling_func) ;
static void resetMsgList(void) ;
static void freeMsgList(MesgListState msg_list) ;
static void destroyMsgList(void) ;
static void drawWindow(void) ;
static void saveMessages(void) ;
static char *mangleMessage(char *msg) ;
static void mesgDestroy(void) ;


/************************************************************/

static MENUOPT msg_list_menu_G[] =
{
  {drawWindow,          "Refresh"},
  {mesgListReset,       "Reset"},
  {graphDestroy,        "Close" },
  {mesgListDestroy,     "Quit" },
  {menuSpacer,          ""},
  {graphPrint,          "Print"},
  {saveMessages,        "Save"},
  {menuSpacer,          ""},
  {help,                "Help" },
  {NULL, NULL}
} ;

/* By default this is the most messages that can be displayed.               */
enum {MESGLIST_MIN_MESSAGES = 1, MESGLIST_DEFAULT_MESSAGES = 50, MESGLIST_MAX_MESSAGES = 2000} ;

/* Defaults for the message list graph.                                      */
static float MESGLIST_X = 0.0, MESGLIST_Y = 0.0, MESGLIST_WIDTH = 0.6, MESGLIST_HEIGHT = 0.1 ;
static int MESGLIST_MAX_LINE_LENGTH = 1024 ;
enum {MESGLIST_TYPE = TEXT_FULL_SCROLL} ;
static int MESGLIST_BORDER = 1 ;
static char *MESGLIST_TITLE = "ACEDB Normal/Error Messages" ;
static char *MESGLIST_NO_MESSAGES = "  <  No messages currently  >  " ;


/* I hate this, really this should be gex state   somewhere, not all splodged*/
/* in here....maybe we can set something up with user_data etc.              */
/*                                                                           */
/* This should only be looked at by get/resetMsgList().                      */
static MesgListState msg_list_G = NULL ;




/************************************************************/

/* Given a list size value, check it and if invalid, return a default value. */
/* This routine is a bit like a C++ static member function, it can be called */
/* before the MesgList is initialised, all functions other cannot.           */
/*                                                                           */
int mesgListGetDefaultListSize(int curr_value)
{
  int new_value = curr_value ;

  if (curr_value < MESGLIST_MIN_MESSAGES)
    new_value = MESGLIST_DEFAULT_MESSAGES ;
  else if (curr_value > MESGLIST_MAX_MESSAGES)
    new_value = MESGLIST_MAX_MESSAGES ;

  return new_value ;
}


/* Like the above but return TRUE if valid, FALSE otherwise. This is to      */
/* support the graphNNNEditor() check callback facility.                     */
/* This routine is a bit like a C++ static member function, it can be called */
/* before the MesgList is initialised, all other functions cannot.           */
/*                                                                           */
BOOL mesgListCheckListSize(int curr_value)
{
  BOOL result = FALSE ;

  if (curr_value >= MESGLIST_MIN_MESSAGES && curr_value <= MESGLIST_MAX_MESSAGES)
    result = TRUE ;

  return result ;
}



/* Should only be called once to initialise, otherwise behaviour is undefined*/
/*                                                                           */
void mesgListCreate(GraphFunc destroy_cb)
{
  MesgListState msg_list = NULL ;
  Graph curr ;

  curr = graphActive() ;

  initMsgList() ;

  msg_list = getMsgList("mesgListCreate") ;

  msg_list->app_destroy_cb = destroy_cb ;

  drawWindow() ;

  graphActivate(curr) ;

  return;
}


/********************************************/

/* Set the list size and adjust the message window accordingly.              */
void mesgListSetSize(int size)
{
  MesgListState msg_list = getMsgList("mesgListSetSize") ;
  Graph curr ;

  curr = graphActive() ;

  if (size < MESGLIST_MIN_MESSAGES)
    size = MESGLIST_MIN_MESSAGES ;
  else if (size > MESGLIST_MAX_MESSAGES)
    size = MESGLIST_MAX_MESSAGES ;

  msg_list->max_messages = size ;

  /* If the list has shrunk then remove some of the list and redisplay.      */
  if (g_slist_length(msg_list->messages) > msg_list->max_messages)
    {
      int i = 0 ;
      char *old_msg = NULL ;

      i = g_slist_length(msg_list->messages) - msg_list->max_messages ;
      while (i > 0)
	{
	  old_msg = msg_list->messages->data ;
	  msg_list->messages = g_slist_remove(msg_list->messages,
					      msg_list->messages->data) ;
	  messfree(old_msg) ;
	  i-- ;
	}

      drawWindow() ;
    }

  graphActivate(curr) ;

  return ;
}


/********************************************/

/* Add a new message.                                                        */
/*                                                                           */
/* All messages are copied as we need to hang on to them for potentially a   */
/* long time.                                                                */
/*                                                                           */
void mesgListAdd(char *msg, void *user_pointer_unused)
{
  MesgListState msg_list = getMsgList("mesgListAdd") ;
  char *msg_copy = NULL ;
  Graph curr ;

  curr = graphActive() ;

  /* Take a copy of the message and prepend a time stamp.                    */
  msg_copy = hprintf(msg_list->handle, "%s - %s", timeShow(timeNow()), msg) ;

  /* Some messages are multiline so hack them into a single line.            */
  msg_copy = mangleMessage(msg_copy) ;

  /* If we've reached the maximum, remove the oldest message.                */
  if (g_slist_length(msg_list->messages) == msg_list->max_messages)
    {
      char *old_msg = NULL ;

      old_msg = msg_list->messages->data ;
      msg_list->messages = g_slist_remove(msg_list->messages,
					  msg_list->messages->data) ;

      messfree(old_msg) ;
    }

  /* Stick new message onto the end of the list.                             */
  msg_list->messages = g_slist_append(msg_list->messages, msg_copy) ;


  /* Refresh the message window.                                             */
  drawWindow() ;

  graphActivate(curr) ;

  return ;
}


/* Free the message list and redraw an empty message window.                 */
void mesgListReset(void)
{
  MesgListState msg_list = getMsgList("mesgListReset") ;
  Graph curr ;

  curr = graphActive() ;

  freeMsgList(msg_list) ;

  drawWindow() ;

  graphActivate(curr) ;

  return ;
}


/* Close the message window but keep the message stuff around. We need this  */
/* because user may wish to just close the window.                           */
void mesgListClose(void)
{
  MesgListState msg_list = getMsgList("mesgListClose") ;
  Graph curr ;

  curr = graphActive() ;

  /* not much to do here because graph package does the destroy of window.   */
  msg_list->graph = GRAPH_NULL ;

  graphActivate(curr) ;

  return ;
}


/* Really destroy everything, graph, list, the lot. After this a call must   */
/* be made to mesgListCreate() to use the message list again.                */
/*                                                                           */
void mesgListDestroy(void)
{
  MesgListState msg_list = getMsgList("mesgListDestroy") ;
  Graph curr ;
  GraphFunc destroy_cb ;

  curr = graphActive() ;

  /* Its possible for user to "quit" message list window and then use        */
  /* preferences to switch to popups and hence call here to destroi          */
  if (msg_list->graph)
    {
      Graph curr_g, mesg_g ;

      curr_g = graphActive() ;
      mesg_g = msg_list->graph ;

      graphActivate(mesg_g) ;
      graphDestroy() ;
      if (curr_g != mesg_g)
	graphActivate(curr_g) ;
    }

  if (msg_list->app_destroy_cb)
    destroy_cb = msg_list->app_destroy_cb ;

  destroyMsgList() ;

  graphActivate(curr) ;

  if (destroy_cb)
    (*destroy_cb)() ;

  return ;
}



/*                                                                           */
/*                   INTERNAL ROUTINES                                       */
/*                                                                           */

/********************************************/

/* Initialise our control block, sadly static at the moment. Note that this  */
/* is the pathological case where we have to access the our global directly  */
/* so we can set it.                                                         */
/*                                                                           */
static void initMsgList(void)
{
  MesgListState msg_list = NULL ;

  msg_list = (MesgListState)messalloc(sizeof(MesgListStateStruct)) ;

  msg_list->messages = NULL ;
  msg_list->max_messages = MESGLIST_DEFAULT_MESSAGES ;
  msg_list->handle = handleCreate() ;

  msg_list->graph = GRAPH_NULL ;
  msg_list->menu = &msg_list_menu_G[0] ;

  msg_list_G = msg_list ;

  return ;
}


/********************************************/

/* These two routines control access to the only global in this package,     */
/* it would be nice if this wasn't necessary but the graph interface does    */
/* not allow passing around of user_data which condemns us more or less to   */
/* using some sort of global.                                                */
/*                                                                           */
static MesgListState getMsgList(char *calling_func)
{
  if (msg_list_G == NULL)
    messcrash("Internal Error: %s called getMsgList() but"
	      "MesgList has not been initialised.",
	      calling_func) ;

  return msg_list_G ;
}

static void resetMsgList(void)
{
  msg_list_G = NULL ;

  return ;
}

/********************************************/


static void freeMsgList(MesgListState msg_list)
{
  handleDestroy(msg_list->handle) ;			    /* free text of messages. */

  g_slist_free(msg_list->messages) ;			    /* free message list. */
  msg_list->messages = NULL ;

  return ;
}


/********************************************/

/* Frees all the message list memory etc. */
static void destroyMsgList(void)
{
  MesgListState msg_list = getMsgList("destroyMsgList") ;

  freeMsgList(msg_list) ;				    /* Free text/messages. */

  messfree(msg_list) ;					    /* free msg struct. */

  resetMsgList() ;					    /* reset to NULL. */

  return ;
}


/* Show the message window, if there is no message window then make it and   */
/* then show it.                                                             */
/* Note that current behaviour is to make sure window is shown to user,      */
/* either because its newly created or because we pop it to the top.         */
/*                                                                           */
static void drawWindow(void)
{
  MesgListState msg_list = getMsgList("drawWindow") ;
  int start, lines, line_length ;

  if(graphActivate(msg_list->graph))
    {
      graphPop() ;
      graphClear();
    }
  else
    { 
      msg_list->graph = graphCreate(MESGLIST_TYPE, MESGLIST_TITLE,
				    MESGLIST_X, MESGLIST_Y, MESGLIST_WIDTH, MESGLIST_HEIGHT) ;
      graphHelp("MesgList") ;

      graphMenu(msg_list->menu) ;

      graphRegister(DESTROY, mesgListClose) ;
      graphRegister(RESIZE, drawWindow) ;
    }

  /* Display all current messages (in black), with most recent one in red.   */
  /* Various fiddlings are done with MESGLIST_BORDER to give a decent width  */
  /* around the text, this hopefully gives the user a warm feeling about     */
  /* being able to see all of a message.                                     */
  start = MESGLIST_BORDER ;				    /* start a bit over and a bit down. */
  lines = MESGLIST_BORDER ;

  line_length = 0 ;
  graphColor(BLACK) ;
  if (msg_list->messages == NULL)
    {
      /* Display default message if there are no messages yet.               */
      graphText(MESGLIST_NO_MESSAGES, start, lines) ;
      line_length = strlen(MESGLIST_NO_MESSAGES) ;
    }
  else
    {
      GSList* next ;
      int i, list_length, line_len = 0 ;

      list_length = g_slist_length(msg_list->messages) ;
      next = msg_list->messages ;

      for (i = 0 ; i < list_length ; i++)
	{
	  line_len = strlen((char *)next->data) ;
	  if (line_len > line_length)
	    line_length = line_len ;

	  if (i == (list_length - 1))			    /* Highlight last item in red. */
	    graphColor(RED) ;

	  graphText((char *)next->data, start, lines) ;
	  lines++ ;
	  next = g_slist_next(next) ;
	}
    }

  /* Set size of underlying message window up to a maximum (number of lines  */
  /* is constrained by max. no. of messages which is set to be showable.     */
  /* Again fiddle around to get a nice border.                               */
  if (line_length > MESGLIST_MAX_LINE_LENGTH)
    line_length = MESGLIST_MAX_LINE_LENGTH ;
  graphTextBounds((MESGLIST_BORDER + line_length + MESGLIST_BORDER),
		  lines + MESGLIST_BORDER) ;

  /* Scroll to bottom of window so latest message is visible.                */
  graphGoto(0, lines + MESGLIST_BORDER);

  graphRedraw() ;

  return ;
}

/* Save the messages to a file chosen by user.                               */
/*                                                                           */
static void saveMessages(void)
{
  MesgListState msg_list = getMsgList("saveMessages") ;
  static char fname[FIL_BUFFER_SIZE]="", dname[DIR_BUFFER_SIZE]="" ;
  ACEOUT save_file ;
  GSList* next ;

  save_file = aceOutCreateToChooser ("File to save messages in",
				     dname, fname, "msg", "w", 0) ;
  if (save_file)
    {
      next = msg_list->messages ;
      do
	{
	  aceOutPrint(save_file, "%s\n", (char *)next->data);
	}
      while ((next = g_slist_next(next))) ;

      aceOutDestroy(save_file) ;
    }

  return ;
}


/* Some messages will have tabs/newlines in them, I'm going to change them   */
/* to a set number of blanks so there is feedback about where they are.      */
/* The function gets handed a pointer the message, if the message needs no   */
/* changes then the pointer is simply returned, otherwise the original       */
/* message is freed and a new one returned.                                  */
/*                                                                           */
static char *mangleMessage(char *msg)
{
  char *result ;
  enum {BLANKS = 3} ;
  int msg_len = strlen(msg) ;
  int i, count ;
  char *new_msg = NULL ;
  char *tmp ;

  for (i = 0, count = 0, tmp = msg ; i < msg_len ; i++, tmp++)
    {
      if (*tmp == '\n' || *tmp == '\t')
	count++ ;
    }

  if (count == 0)
    result = msg ;
  else
    {
      result = new_msg = messalloc(msg_len + (count * BLANKS) + 1) ;
      tmp = msg ;

      while (*tmp)
	{
	  if (*tmp == '\n' || *tmp == '\t')
	    {
	      memset(new_msg, (int)' ', BLANKS) ;
	      new_msg+=3 ;
	    }
	  else
	    {
	      *new_msg = *tmp ;
	      new_msg++ ;
	    }
	  tmp++ ;
	}
      *new_msg = '\0' ;
      
      messfree(msg) ;			  
    }

  return result ;
}



/********************** eof *********************************/
