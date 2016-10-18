/*  File: zmapsocket.c
 *  Author: Gemma Barson <gb10@sanger.ac.uk>
 *  Copyright (C) 2011 Genome Research Ltd
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
 * Description: see zmapsocket.h
 *-------------------------------------------------------------------
 */


/************************************************************/
/* Includes and defines */

#include <wzmap/zmapsocket.h>
#include <wzmap/zmaputils.h>
#include <wh/smap.h>
#include <wh/acedb.h>
#include <wh/aceio.h>
#include <wh/session.h>
#include <wh/command.h>
#include <gtk/gtk.h>

#define MAC_X
#include "wac/acclient_socket_lib.c"

#define ACECONN_SERVER_VERSION     1                /* to do: not sure what this should be. docs here suggest 1: wdoc/Socket_Server/SOCKET_interface.html */
#define ACECONN_MAX_BYTES          1000             /* to do: not sure what this should be either! */
#define DEFAULT_NONCE_STRING       "<to do>"        /* to do: use a random md5 hash instead of this */

/* Error codes */
typedef enum
  {
    XACE_ZMAP_ERROR_SOCKET_ADDRESS             /* error creating socket address */
  } XaceZmapError;


/* This keeps track of the state of the socket, i.e. we read a header first
 * and then expect the body of the message */
typedef enum
  {
    XACE_SOCKET_READY,        /* ready to receive new messages */
    XACE_SOCKET_HEADER_OK,    /* read in header and now waiting to read body */
    XACE_SOCKET_HEADER_ERROR  /* error while reading header */
  } XaceSocketState;


/************************************************************/
/* Structs */

/* Callers are returned an opaque reference to this struct, which contains   */
/* all the state associated with a command.                                  */
/*                                                                           */
typedef struct _AceCommandStruct
{ 
  magic_t *magic ;					    /* == &AceCommand_MAGIC */
  ACEOUT dump_out;					    /* used for show -f / aql -o etc */

  SPREAD spread ; 
  KEYSET ksNew, ksOld, kA ;
  Stack ksStack ;
  int nns ;						    /* number of entries on stack */
  unsigned int choix ;
  unsigned int perms;

  Array aMenu ;
  KEY lastCommand ;

  BOOL user_interaction ;				    /* TRUE => cmds may ask user for input. */
  BOOL noWrite ;					    /* TRUE => no write access. */
  BOOL quiet ;						    /* TRUE => only output result of
							       command, no acedb comments. */
  BOOL showStatus , beginTable ;
  BOOL showTableTitle;
  int nextObj, lastSpread ; 
  int cumulatedTableLength ;
  int encore ;
  int minN ;						    /* startup */
  int maxN ;						    /* -1 or, max number of desired output */
  int notListable ;					    /* empty and alias objects */
  BOOL dumpTimeStamps ;
  BOOL dumpComments ;
  BOOL noXref ;

  /* Flags for dumping of dna and much more. */
  BOOL allowMismatches ;
  BOOL spliced ;
  BOOL cds_only ;
  COND showCond ;
  char beauty ;						    /* ace, tace, perl */
        /*
        * For beauty, possible values are:  (see w4/dump.c dumpKey2)
        *  p  ace perl
        *  j  java format
        *  J  another java format
        *  a  ace file format
        *  A  ace file format (same?)
        *  C  acec library binary format
        *  H  ???
        *  h  ???
        *  m  minilib format - much like ace file but shows all tags
        *     and has special markings for type A objects (NCBI only)
	*  k  ??? (Sanger only)
	*  K  ??? (Sanger only)
	*  x  ??? (Sanger only)
	*  X  ??? (Sanger only)
	*  y  ??? (Sanger only)
	*  Y  ??? (Sanger only)
        *
        */
  char fastaCommand ;					    /* for DNA, Peptide more */

  /*
  * these are all related to kget/kstore/kclear commands:
  */
  DICT *kget_dict;
  Array kget_namedSets ;
} AceCommandStruct ;


/************************************************************/
/* Private functions */

/* wrapper function for a command main-loop */
static void xaceCommandExecute (ACEIN fi, ACEOUT fo, AceCommand look)
{ 
  /* This is essential for at least Aquila, which needs the sub shells to    */
  /* $echo its logging info. to the screen/files, otherwise this logging info */
  /* is passed through to acedb !!!!!!!!!!                                   */
  if (!getenv("ACEDB_SUBSHELLS"))
    aceInSpecial (fi,"\n\t\\/@%") ;  /* forbid sub shells */
  else  
    aceInSpecial (fi,"\n\t\\/@%$") ; /* allow sub shells */


  if (aceInIsInteractive(fi) && !getenv("ACEDB_NO_BANNER"))
    aceOutPrint (fo, "\n\n// Type ? for a list of options\n\n") ;

  int option = 0 ;
  int maxChar = 0 ;

 if (!look->ksNew)
   look->ksNew = keySetCreate() ;

 gboolean finished = FALSE;
 
  while (!finished)
    {
      option = aceCommandExecute (look, fi, fo, option, maxChar) ;
    
      switch (option)
	{
	case 'q':
	  finished = TRUE;
          break;

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

  return;
} /* xaceCommandExecute */


/* returns the text output produced by the command */
static char *xaceRemoteHandler (char *command, AceCommand look)
{
  Stack resultStack;
  char *resultText;
  ACEIN fi;
  ACEOUT fo;
  struct messContextStruct oldOutContext;
  struct messContextStruct newOutContext;


  gifEntry = gifControl;	/* extend command language to deal
				   with giface commands */


  fi = aceInCreateFromText (command, "", 0);
  /* no subshells ($), no commands (@)
   * recognise ';' as card separator - WARNING: this will disable some 
   *   tace-commands where the ; is needed to submit multiple entries
   *   on one command line using the '=' assignment. */
  aceInSpecial (fi, "\n\t\\/%;");

  resultStack = stackCreate(100);
  fo = aceOutCreateToStack (resultStack, 0);

  /* make sure we also redirect messOut/Error to the same output stack
   * while the remote-command is being executed */
  newOutContext.user_func = acedbPrintMessage;
  newOutContext.user_pointer = fo;

  /* redirect messout/messerror to the same resultStack
   * as the output for the command-execution */
  oldOutContext = messOutRegister (newOutContext);

  xaceCommandExecute(fi, fo, look) ;


  gifEntry = 0;

  messOutRegister (oldOutContext);

  /* this buffer will be free'd after data has been sent to the
   * XA_XREMOTE_RESPONSE atom */
  resultText = strnew(popText(resultStack), 0);

  aceInDestroy (fi);
  aceOutDestroy (fo);
  stackDestroy (resultStack);

  messStatus (0);

  return resultText;		/* NOTE: must never be NULL */
} /* xaceRemoteHandler */



static void sendCharsToChannel(GIOChannel *channel, const gchar *buffer, const gssize bytes_requested, GError **error)
{
  gsize total_written = 0;

  while (*error == NULL && total_written < bytes_requested)
    {
      gsize bytes_written = 0;
      g_io_channel_write_chars(channel, buffer, bytes_requested - total_written, &bytes_written, error);
      total_written += bytes_written;
    }
}


/* Send the given message to the client on the given channel. Takes care of adding 
 * the requried header required by the acedb protocol. */
static void sendToClient(GIOChannel *channel, 
                         const gchar *message, 
                         char *msg_type, 
                         GError **error)
{
  /* Construct the header */
  AceConnHeaderRec hdr;
  hdr.byte_swap = 0;
  hdr.server_version = ACECONN_SERVER_VERSION;
  //  hdr.client_id = /* to do - should be unique id */
  hdr.max_bytes = ACECONN_MAX_BYTES;
  setMsgType(hdr.msg_type, msg_type);
  hdr.length = strlen(message) + 1;

  /* Convert the header struct to a buffer */
  gchar *header = g_malloc0(ACECONN_HEADER_BYTES);
  hdr2buf(&hdr, header);

  /* Write the header */
  sendCharsToChannel(channel, header, ACECONN_HEADER_BYTES, error);
  
  /* Now write the actual message */
  if (*error == NULL)
    {
      DEBUG_OUT("xace: SENDING :\n%s\n", message);
      sendCharsToChannel(channel, message, hdr.length, error);
    }
  
  if (*error == NULL)
    {
      g_io_channel_flush(channel, error);
    }
}


/* Process a message from the client. Sends a reply if necessary. */
static GIOStatus processClientMessage(GIOChannel *channel, char *message, AceCommand look)
{
  DEBUG_OUT("xace: RECEIVED:\n%s\n", message);

  static char *nonce = NULL; /* gets set during handshaking */
  
  GIOStatus status = G_IO_STATUS_NORMAL;
  GError *error = NULL;

  if (nonce)
    {
      /* we are in the middle of a handshake communication. The client
       * should now have sent us a userid and a password hashed together with
       * our nonce and we should check it is ok before returning our standard
       * reply (to do) */
      sendToClient(channel, ACECONN_SERVER_HELLO, ACECONN_MSGOK, &error);
      nonce = NULL; /* indicates we've finished the handshake */
    }
  else if (strcmp(message, ACECONN_CLIENT_HELLO) == 0)
    {
      /* the client is starting a handshake. to return the handshake, we should
       * send the client a hashed random number that it will then return
       * to us, hashed together with the password (to do) */
      nonce = DEFAULT_NONCE_STRING;
      sendToClient(channel, nonce, ACECONN_MSGOK, &error);
    }
  else if (strcmp(message, ACECONN_CLIENT_GOODBYE) == 0)
    {
      sendToClient(channel, "<to do>", ACECONN_MSGKILL, &error);
      status = G_IO_STATUS_EOF; /* indicates to calling function that we should shut down channel */
    }
  else
    {
      char *reply = xaceRemoteHandler(message, look);
      sendToClient(channel, reply, ACECONN_MSGOK, &error);
    }

  if (error)
    g_critical("xace: Error sending reply: %s\n", error->message);

  return status;
}


/* Read an acedb-format message from the given channel (with a known-size 
 * header followed by the message body) */
static GIOStatus readClientMessage(GIOChannel *channel, AceCommand look, GError **error)
{
  static XaceSocketState state = XACE_SOCKET_READY;
  static int message_len = 0;
  GIOStatus status = G_IO_STATUS_NORMAL;

  if (state == XACE_SOCKET_READY)
    {
      /* Read the header (which is a fixed number of bytes) */
      gsize bytes_read = 0;
      gchar *header = g_malloc0(ACECONN_HEADER_BYTES);
      status = g_io_channel_read_chars(channel, header, ACECONN_HEADER_BYTES, &bytes_read, error);

      if (bytes_read > 0 && status == G_IO_STATUS_NORMAL && *error == NULL)
        {
          state = XACE_SOCKET_HEADER_OK;
          
          /* Extract the header info from the buffer, and get the size of the message body */
          AceConnHeaderRec hdr;
          buf2hdr(header, &hdr);
          message_len = hdr.length;
        }
      else
        {
          state = XACE_SOCKET_HEADER_ERROR;
        }
      
      g_free(header);
    }
  else 
    {
      if (state == XACE_SOCKET_HEADER_OK)
        {
          /* Read the message body */
          gsize bytes_read = 0;
          gchar *message = g_malloc0(message_len);
          status = g_io_channel_read_chars(channel, message, message_len, &bytes_read, error);
          
          if (bytes_read > 0 && status == G_IO_STATUS_NORMAL && *error == NULL)
            status = processClientMessage(channel, message, look);
          
          g_free(message);
        }

      state = XACE_SOCKET_READY; /* ready for next message */
    }
  
  return status;
}


/* Read an incoming message from the client. Takes care of reading the message 
 * header required by the acedb protocol. */
static gboolean onMessageFromClient(GIOChannel *channel, GIOCondition condition, gpointer user_data)
{
  GIOStatus status = G_IO_STATUS_NORMAL;
  AceCommand look = (AceCommand)user_data;

  if (condition == G_IO_IN)
    {
      GError *error = NULL;
      status = readClientMessage(channel, look, &error);
 
      if (error)
        g_critical("xace: Error reading client message: %s\n", error->message);
    }
  
  if (condition == G_IO_HUP || status == G_IO_STATUS_EOF)
    {
      /* The client hung up shutdown the channel and stop watching it. */
      DEBUG_OUT("xace: CLIENT DISCONNECTED\n");

      GError *error = NULL;
      g_io_channel_shutdown(channel, TRUE, &error);

      if (error)
        printf("xace: error shutting down channel\n");
      
      /* destroy the command-state struct that was created when this client connected */
      aceCommandDestroy(look);

      return FALSE; /* return false to stop listening on this channel */
    }

  return TRUE;
}


/* Accept an incoming new client connection */
static gboolean onNewClientConnection(GIOChannel *source, GIOCondition condition, gpointer user_data)
{
  /* Accept the connection on the listening socket (given in the user data). This
   * returns a new socket, which will be used to send/receive messages for this client. */
  GError *error = NULL;
  GSocket *sockIn = G_SOCKET(user_data);
  GSocket *sock = g_socket_accept(sockIn, NULL, &error);

  if (!error)
    {
      /* Set up the channel for sending/receiving messages. We need to set the 
       * encoding to NULL because we are going to send non-UTF8 text in the headers */
      int fd = g_socket_get_fd(sock);
      GIOChannel *channel = g_io_channel_unix_new(fd);
      g_io_channel_set_encoding(channel, NULL, &error);

      /* Specify the callback for when the client sends us a message. We need a 
       * persistent AceCommand object that we will pass to the callback function
       * (this is used to store the "active list" so that the client can perform
       * actions on the results from a previous command) */

      /* If a user has write access they get to see more commands than if they   */
      /* can only read.                                                          */
      unsigned int command_set = (CHOIX_UNIVERSAL | CHOIX_NONSERVER | CHOIX_GIF) ;
      unsigned int perm_set = PERM_UNDEFINED ;		    /* we do logic on these variable. */
      
      if (writeAccessPossible())
        perm_set = (PERM_READ | PERM_WRITE | PERM_ADMIN) ;
      else
        perm_set = PERM_READ ;

      AceCommand look = aceCommandCreate(command_set, perm_set);

      g_io_add_watch(channel, G_IO_IN, (GIOFunc)onMessageFromClient, look);
      g_io_channel_unref(channel);

      DEBUG_OUT("xace: CLIENT CONNECTED\n");
    }

  if (error)
    {
      printf("xace: error accepting client connection: %s\n", error->message);
    }
  
  return TRUE; /* return true to continue listening for new connections */
}


/* Listen for incomming connections on the given socket. Adds it to the idle
 * loop with the onNewClientConnection callback */
static void watchSocket(GSocket *sock, GError **error)
{
  GIOChannel *channel = NULL;

  /* set up the socket to listen */
  g_socket_listen(sock, error);

  if (*error == NULL)
    {
      /* create an IO channel for the socket */
      const int fd = g_socket_get_fd(sock);
      channel = g_io_channel_unix_new(fd);
      g_io_channel_set_encoding(channel, NULL, error);
    }
  
  if (*error == NULL)
    {
      /* add a watch to the idle loop, and set the callback that will be
       * called when new clients connect */
      g_io_add_watch(channel, G_IO_IN, (GIOFunc)onNewClientConnection, sock);
      g_io_channel_unref(channel);
    }
}


/* create an INET type socket and bind to it */
static GSocket* createInetSocket(guint16 *port, GError **error)
{
  GSocket *sock = g_socket_new(G_SOCKET_FAMILY_IPV4, 
                               G_SOCKET_TYPE_STREAM, 
                               G_SOCKET_PROTOCOL_TCP,
                               error);

  GInetAddress *addr = g_inet_address_new_from_string(HOST_IP_ADDRESS);

  if (!addr)
    g_set_error(error, XACE_ZMAP_ERROR, XACE_ZMAP_ERROR_SOCKET_ADDRESS, "xace: error creating inet address\n");

  if (*error == NULL)
    {
      GSocketAddress *sock_addr = g_inet_socket_address_new(addr, *port);
      g_socket_bind(sock, G_SOCKET_ADDRESS(sock_addr), TRUE, error);

      /* If the bind failed because the port is already in use, try
       * again with a different port number. (To do: is there a better
       * way of finding an unused port number than this? We just increment
       * until we find one where bind works...) */
      const int maxTries = 100;
      int count = 0;

      while (*error != NULL && count < maxTries)
        {
          ++count;
          *port += 1;

          g_error_free(*error);
          *error = NULL;

          g_object_unref(sock_addr);
          sock_addr = g_inet_socket_address_new(addr, *port);
          g_socket_bind(sock, G_SOCKET_ADDRESS(sock_addr), TRUE, error);
        }
    }

  return sock;
}


/************************************************************/
/* Public functions */

/* Set up a socket for listening for incoming client connections */
void listenForClientConnections(guint16 *port, GError **error)
{
  /* we only set up the listening socket once */
  static gboolean done = FALSE;
  
  if (!done)
    {
      GSocket *sock = createInetSocket(port, error);

      if (*error == NULL)
        watchSocket(sock, error);

      done = TRUE;
      DEBUG_OUT("xace: LISTENING FOR CLIENT CONNECTIONS\n");
    }
}
