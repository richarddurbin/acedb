/*  File: socketace.c
 *  Author: Ed Griffith (edgrif@sanger.ac.uk) & Jean Thierry-Mieg (mieg@crbm.cnrs-mop.fr)
 *  Copyright (C) J Thierry-Mieg and R Durbin, 1992
 *-------------------------------------------------------------------
 * This file is part of the ACEDB genome database package, written by
 * 	Richard Durbin (MRC LMB, UK) rd@mrc-lmb.cam.ac.uk, and
 *	Jean Thierry-Mieg (CRBM du CNRS, France) mieg@kaa.cnrs-mop.fr
 *
 * Description: Contains code that provides the interface between
 *              acedb and the transport layer. Controls timeouts,
 *              authorises clients, reads server configuration files
 *              and so on.
 *
 * Note that, when saving, you save all clients at once, there
 * is NO WAY to save independantly a single client session.
 *
 * Exported functions:
 * HISTORY:
 * Last edited: Feb 27 13:41 2009 (edgrif)
 * * Dec  2 16:32 1998 (edgrif): Correct decl. of main, add code to
 *              record build time of this module.
 * * Oct 15 10:57 1998 (edgrif): Add new graph/acedb initialisation call.
 * * Feb 25 23:00 1996 (mieg)
 * * Mar 97 (mieg) SIGPIPE handler
 * Created: Wed Nov 25 12:26:06 1992 (mieg)
 * CVS info:   $Id: serverace.c,v 1.62 2009-03-02 13:17:14 edgrif Exp $
 *-------------------------------------------------------------------
 */

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
/* TEST CODE...                                                              */
#include <sys/time.h>
#include <sys/resource.h>
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */


#include <locale.h>
#include <wh/acedb.h>
#include <wh/aceio.h>
#include <wh/acelib.h>
#include <wh/apputils.h>
#include <wh/help.h>
#include <wh/session.h>
#include <wh/dbpath.h>
#include <wh/log.h>
#include <wh/pick.h>
#include <wh/bs.h>
#include <whooks/sysclass.h>
#include <whooks/systags.h>
#include <whooks/tags.h>
#include <wh/call.h>
#include <wh/mytime.h>
#include <wh/parse.h>
#include <wh/freeout.h>
#include <wh/version.h>
#include <wh/command.h>
#include <wh/keyset.h>
#include <wh/query.h>
#include <wh/a.h>
#include <wh/dump.h>
#include <wh/lex.h>
#include <wh/sigsubs.h>
#include <wh/pref.h>
#include <wsocket/serverace_.h>
#include <wsocket/serverclientutils.h>

#ifdef GIFACESERVER
/* See stub routines at end of file for what happens if there is no giface code included. */
#include <wh/acedbgraph.h>
#include <wh/display.h>
#include <wh/graph.h>
#include <w7/fmapsegdump.h>
#endif

/************************************************************/

void initServerState(AceServ server_state) ;

static Stack processRequests(void *server, 
			     ClientConnect connection, ClientConnectionID connection_id,
			     AceSocketRequestTarget *targ_inout,
			     AceSocketRequestType *req_inout,
			     ACE_HEADER hh, char *request) ;

static void checkRequest(AceSocketRequestTarget currTarget, AceSocketRequestType currRequest) ;

static Stack processQueries(AceServ server, 
			    ClientConnect connection, ClientConnectionID connection_id,
			    AceSocketRequestTarget *requestTarget,
			    AceSocketRequestType *requestType,
			    char *request, ACE_HEADER acehead) ;

/* Server pid file functions. */
static void doServerPidFile(AceServ server) ;
static void clearServerPidFile(AceServ server) ;

/* server file functions (logs/lockfiles etc.). */
static char *createLockFile(AceServ server, char *lockfile_name) ;
static void removeUndetectableLockFile(AceServ server_state) ;
static void openServerLog(AceServ server) ;
static void appendServerLog(AceServ server, char *msg) ;
static void closeServerLog(AceServ server, aceLogExitType exit_type) ;
static BOOL newServerLog(AceServ server, char *old_logfile_name) ;
static BOOL reopenServerLog(AceServ server) ;

static void compressMessage(Stack msg) ;

/* Server callback functions, get called from sessionInit and other acedb
 * routines. */
static void sessionInitCB(void *data_ptr) ;
static void sessionCheckFilesCB(void *data_ptr) ;
static void sessionExitCB(void *data_ptr, char *exit_msg);
static void sessionCrashCB(void *data_ptr, char *exit_msg);
static void sessionServerClose(void *data_ptr) ;
static void sessionMessOutCB(char *message, void *data_ptr) ;
static void sessionMessErrCB(char *message, void *data_ptr) ;
static void serverMessages(char *message, AceServ server, FILE *output_dest) ;
static void sessionSigHup(void *data_ptr) ;
static void sessionSigTerm(void *data_ptr) ;

/* Utility functions. */
static char *getWorkingDir(void) ;

/**************************************************************/

/* Signal handler may set this to TRUE to indicate that server has caught    */
/* a SIGHUP and a new log file is required.                                  */
static BOOL logfile_request_new_G = FALSE ;


/**************************************************************/

static void clientDestroy(AceServ server, AceClient aceClient)
{ 
  server->nActiveClients-- ;
  messout("Closing Client %d, now %d active clients", 
	  aceClient->clientId, server->nActiveClients) ;

  assRemove (server->clientAss, assVoid (aceClient->clientId)) ;

  if (aceClient->aceCommand != NULL)
    aceCommandDestroy (aceClient->aceCommand) ;

  messfree (aceClient) ;
}


/************************************************************/

/* Aside from crashes, this is the only routine for exitting the socket
 * server.
 * Note that if the server is sent a SIGTERM we assume that we must close
 * down as quickly as possible, we therefore do not mess about telling
 * clients we are quitting but instead attempt to save the database if
 * possible. This should be "safe" because the signal is only acted on
 * after processing of a current request in the acedb layer is complete.
 * I say "safe" because it may be that someone was sending a very long
 * update and we may have only processed part of it and hence not all
 * their updates may have completed, but in a way that is their fault,
 * they shouldn't be doing long udpates via the network. */
static void closeAcedbServer(AceServ server, AceSocketRequestType request)
{
  char *cp = 0 ;
  AceClient aceClient ;

  /* Destroy all the clients.                                                */
  if (request != ACESOCK_TERM)
    {
      while (assNext(server->clientAss, &cp, &aceClient))
	{
	  clientDestroy(server, aceClient) ;
	  messout("// Destroying client") ;
	  cp = 0 ;    /* restart the search - needed to keep assNext() happy after assRemove */
	}


    }

  /* Shutdown the database.                                                  */
  if (isWriteAccess())
    aceQuit(TRUE);
  else
    aceQuit(FALSE);

  /* Remove the undetectable crash lockfile as we are shutting down in the   */
  /* proper way.                                                             */
  if (!server->readonly_DB)
    removeUndetectableLockFile(server) ;

  server->logmsgs_per_timestamp = 0 ;			    /* Don't timestamp these. */
  if (request == ACESOCK_TERM)
    {
      messout("#### Server finished by SIGTERM exit") ;
      messout("#### A bientot") ;
    }
  else
    {
      messout("#### Server finished by normal exit") ;
      messout("#### A bientot") ;
    }


  /* Close the server log, no more messages now.                             */
  closeServerLog(server, ACELOG_NORMAL_EXIT) ;

  /* Don't messExit here, we have already closed acedb and the log file. */
  exit(0) ;

  /* Never get here...                                                       */
  return ;
}


/**************************************************************/

/* Routine to service client specific requests, note that requestTarget will */
/* always be ACESOCK_CLIENT on entry but may change when we return, e.g. to  */
/* ACESOCK_SERVER for a shutdown request.                                    */
/*                                                                           */
static Stack processQueries(AceServ server,
			    ClientConnect connection, ClientConnectionID connection_id,
			    AceSocketRequestTarget *requestTarget,
			    AceSocketRequestType *requestType,
			    char *request, ACE_HEADER acehead)
{
  static Stack s = 0 ;
  static int nClients = 0 , nTransactions = 0 ;
  KEY option ;
  AceClient aceClient ;
  int maxChar = 0 ;
  int num_words ;
  Array words ;
  BOOL status ;
  ClientAccessStatus acc_status ;

  if (server->debug)
    {
      char *msg ;

      /* Interesting, should allow a request to be null ?? probably not...       */
      if (testMsgType(acehead->msgType, ACESERV_MSGDATA) && request)
	msg = "Raw Ace data being passed" ;
      else
	msg = request ;
	  
      messout("ProcessQueries: %s", msg ? msg : "null") ;
    }


  /* Make sure server maximum messages size is not exceeded, n.b. if server max = 0 then
   * there is no maximum and we just set what the client wants. */
  maxChar = acehead->maxBytes ;
  if (maxChar <= 0 || ((server->max_bytes > 0) && (maxChar > server->max_bytes)))
    maxChar = acehead->maxBytes = server->max_bytes ;	    /* n.b. reset maxBytes in header. */


  /* We reuse this stack for every request, this means it must be copied     */
  /* each time....                                                           */
  /* stackTextOnly is vital, otherwise we get funny lengths for messages     */
  /* because of byte padding...                                              */
  s = stackReCreate(s, 20000) ;
  stackTextOnly(s) ;


  nTransactions++ ;

  /* Currently serverVersion is always set to 1, it may be set to other      */
  /* values later on, the client could look at this to decide if it can      */
  /* cope with the server version.                                           */
  acehead->serverVersion = ACESERV_CURRENT_SERVER_VERSION ;



#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
  /* MY LOGIC IS NOT CORRECT HERE, IT NEEDS MORE THOUGHT, WITHOUT THIS       */
  /* TEST THERE IS A POTENTIAL LOOPHOLE OF BAD REQUESTS BUT THE BELOW IS     */
  /* NOT CORRECT AND CAUSES PROBLEMS.                                        */

  /* This is not clean, remember that the transport layer can ask us to kill */
  /* a client, so we can't be sure if the invalid request came from the      */
  /* client or the transport layer....sigh, needs cleaning up.               */
  if (!testMsgType(acehead->msgType, ACESERV_MSGDATA)
      && !testMsgType(acehead->msgType, ACESERV_MSGREQ)
      && !testMsgType(acehead->msgType, ACESERV_MSGENCORE)
      && !testMsgType(acehead->msgType, ACESERV_MSGKILL))
    {
      /* Currently the client is only allowed to send one of these two.      */
      setMsgType(acehead->msgType, ACESERV_MSGKILL) ;
      *requestType = ACESOCK_KILL ;

      pushText (s, "Illegal request, closing your connection.") ;
    }
  else if (!(acehead->clientId))
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

  if (!(acehead->clientId))
    { 
      /* I'm not happy with this clientIdp stuff for identifying the new clients,*/
      /* I would prefer a more explicit way of doing this.                       */

      /* NEW CLIENTS....                                                     */
      AceServPermOptions read_acc, write_acc ;

      *requestType = ACESOCK_KILL ;

      if (!testMsgType(acehead->msgType, ACESERV_MSGREQ))
	{
	  /* Wrong request type...                                           */
	  pushText (s, "Illegal request, closing your connection.") ;
	}
      else if (server->shuttingDown)
	{
	  /* Nothing doing, the server is shutting down.                     */
	  pushText (s, "Sorry, refusing your connection because "
		    "the server is shutting down.") ;
	}
      else if (!checkGlobalDBAccess(&read_acc, &write_acc))
	{
	  /* check serverconfig.wrm to find out what levels of access are allowed. */
	  pushText (s, "Sorry, refusing your connection because the server\n"
		    "cannot access configuration file "
		    "\"wspec/"ACESERV_CONFIG_NAME"."ACESERV_CONFIG_EXT"\".\n"
		    "Please tell the database administrator about this error.") ;
	}
      else if (strcmp(request, ACESERV_CLIENT_HELLO) != 0)
	{
	  pushText(s, "// Sorry, you are an unknown client and your connection "
		   "has been terminated.") ;
	}
      else
	{
	  /* Allocate a new client.                                              */
	  aceClient = (AceClient)messalloc(sizeof(AceClientStruct)) ;

	  /* Set a unique client id for associator for client retrieval.         */
	  acehead->clientId = aceClient->clientId = ++nClients ;
	  assInsert(server->clientAss, assVoid(nClients), aceClient) ;
	  (server->nActiveClients)++ ;

	  /* Force a timestamp for new client records. */
	  {
	    int old_freq = server->logmsgs_per_timestamp ;

	    server->logmsgs_per_timestamp = 1 ;			    /* Will force a timestamp. */
	    messout("New Client %d, %d active clients", acehead->clientId, server->nActiveClients) ;
	    server->logmsgs_per_timestamp = old_freq ;
	  }

	  aceClient->globalReadAcc = read_acc ;
	  aceClient->globalWriteAcc = write_acc ;

	  /* Set up the nonce for use by the client to encrypt the password.     */
	  /* This will be passed to the client to hash the password.             */
	  aceClient->nonce = makeNonce((void *)aceClient) ;
	  pushText(s, aceClient->nonce) ;
	  if (server->debug)
	    messout("Nonce passed: %s", stackText(s,0)) ;
	  
	  /* store time of last access. Used for the killing of dead clients */
	  aceClient->lastAccess = timeNow() ;
	  
	  aceClient->aceCommand = NULL ;

	  /* Store connection data for client (opaque pointer)               */
	  aceClient->connection = connection ;

	  /* Store the connection ID for later security checking.            */
	  aceClient->connection_id = connection_id ;

	  /* OK, we send the nonce back to the client and await the password.    */
	  aceClient->state = ACECLIENT_WAITPASSWD ;
	  setMsgType(acehead->msgType, ACESERV_MSGOK) ;
	  *requestType = ACESOCK_REQ ;

	  aceClient->compress = FALSE ;			    /* no compression by default. */
	}

      if (*requestType == ACESOCK_KILL)
	setMsgType(acehead->msgType, ACESERV_MSGKILL) ;
    }     
  else if (!assFind (server->clientAss, assVoid(acehead->clientId), &aceClient))
    {
      /* CLIENT WITH CONNECTION BUT UNKNOWN ID...                            */
      /* this error could be because the client is being naughty and faking  */
      /* its client id OR it could be an error in our code at the client or  */
      /* socket end or corruption of bytes over the network, here we play    */
      /* safe and chop the connection.                                       */
      char *mesg = NULL ;

      setMsgType(acehead->msgType, ACESERV_MSGKILL) ;
      *requestType = ACESOCK_KILL ;

      mesg = hprintf(0, "Aceserver received an unknown client id = %d, "
		     "this may be an internal error or a naughty client", acehead->clientId) ;
      appendServerLog(server, mesg) ;
      messfree(mesg) ;

      pushText(s, "// Sorry, you are an unknown client and your connection "
	       "has been terminated.") ;
    }
  else if (aceClient->connection_id != connection_id)
    {
      /* Client with known id but the connection id has changed implying     */
      /* that someone is being naughty and trying to fake their client id    */
      /* in the hopes of getting admin. access or something like that.       */
      char *mesg = NULL ;

      setMsgType(acehead->msgType, ACESERV_MSGKILL) ;
      *requestType = ACESOCK_KILL ;

      mesg = hprintf(0, "Aceserver received a known client id (id = %d), "
		     "but the clients \"connection id\" has changed, was %d, is now %d "
		     "this may be an internal error or a naughty client",
		     acehead->clientId, aceClient->connection_id, connection_id) ;
      appendServerLog(server, mesg) ;
      messfree(mesg) ;

      pushText(s, "// Sorry, you are an unknown client and your connection "
	       "has been terminated.") ;
    }
  else if (aceClient->state == ACECLIENT_WAITPASSWD)
    {
      /* CLIENT WHO SHOULD BE SENDING PASSWD...                              */
      AceUserGroup group ;

      *requestType = ACESOCK_KILL ;

      if (!testMsgType(acehead->msgType, ACESERV_MSGREQ))
	{
	  /* Wrong request type...                                           */
	  pushText (s, "Illegal request, closing your connection.") ;
	}
      else
	{
	  aceClient->lastAccess = timeNow (); /* last access, to kill  dead clients */

	  /* need to extract userid/nonce from the clients message here...       */
	  num_words = parseWords(request, &words) ;
	  if (num_words != 2)
	    {
	      char *mesg = NULL ;

	      mesg = hprintf(0, "Aceserver received handshake request from client which should have been "
			     "\"userid password\" but was \"%s\"", request) ;
	      appendServerLog(server, mesg) ;
	      messfree(mesg) ;

	      pushText(s, "// Sorry, illegal request, your connection "
		       "has been terminated.") ;
	      clientDestroy(server, aceClient) ;
	    }
	  else
	    {
	      /* Sort out the read/write/admin access for the client.           */

	      /* OK, here i think I should call  getClientInetAddressInfo() to get     */
	      /* the hostname and hang on to it in the client...and pass it in   */
	      /* to this routine as a param. Would be much cleaner, wouldn't     */
	      /* need to pass in connection below...                             */
	      group = getUserDBAccess(server,
				      array(words, 0, char*), array(words, 1, char*),
				      aceClient->nonce,
				      aceClient->globalReadAcc, aceClient->globalWriteAcc,
				      aceClient->connection,
				      &acc_status) ;
	      if (group == ACEUSER_NONE)
		{
		  char *mesg = NULL ;
		  mesg = hprintf(0, "Aceserver received handshake request from client "
				 "but access was refused,\n"
				 "request was \"%s\" and reason was: %s.",
				 request, getPasswdErrorMessage(acc_status)) ;
		  appendServerLog(server, mesg) ;
		  messfree(mesg) ;

		  if (acc_status == CLIENTACCESS_UNKNOWN || acc_status == CLIENTACCESS_BADHASH)
		    pushText(s, "// Sorry, userid/password incorrect, "
			     "your connection has been terminated.") ;
		  else /* CLIENTACCESS_BLOCKED */
		    pushText(s, "// Sorry, you do not have permission to access the database, "
			     "your connection has been terminated.") ;

		  clientDestroy(server, aceClient) ;
		}
	      else
		{
		  /* Wow, finally we approve the client...now await requests...  */
		  unsigned int cmd_set = CHOIX_UNDEFINED ;
		  unsigned int perm_set = PERM_UNDEFINED ;

		  aceClient->userid = strnew(array(words, 0, char*), 0) ;
		  aceClient->group = group ;
		  pushText(s, ACESERV_SERVER_HELLO) ;
		  aceClient->state = ACECLIENT_ACTIVE ;

		  /* The server has this set of commands, if compiled as the */
		  /* gifaceserver it has some additional ones. Which ones    */
		  /* the users have access to is controlled by the PERMs.    */
		  cmd_set = CHOIX_UNIVERSAL + CHOIX_SERVER + CHOIX_RPC ;
#ifdef GIFACESERVER
		  cmd_set += CHOIX_GIF ;
#endif

		  /* Permissions are supersets from READ -> ADMIN */
		  perm_set = PERM_READ ;
		  if (aceClient->group == ACEUSER_WRITE || aceClient->group == ACEUSER_ADMIN)
		    perm_set += PERM_WRITE ;
		  if (aceClient->group == ACEUSER_ADMIN)
		    perm_set += PERM_ADMIN ;

		  /* Create the command processor instance. */
		  aceClient->aceCommand = aceCommandCreate(cmd_set, perm_set);

		  /* Set command env. to read only if the user only has read access. */
		  if (aceClient->group == ACEUSER_READ)
		    aceCommandNoWrite(aceClient->aceCommand) ;

		  /* Set command env. not to ask for user interaction */
		  aceCommandSetInteraction(aceClient->aceCommand, FALSE) ;


		  setMsgType(acehead->msgType, ACESERV_MSGOK) ;
		  *requestType = ACESOCK_REQ ;
		}
	    }

	  if (num_words != 0)
	    freeWords(words) ;
	}

      if (*requestType == ACESOCK_KILL)
	setMsgType(acehead->msgType, ACESERV_MSGKILL) ;
    }
  else /* aceClient->state == ACECLIENT_ACTIVE */
    {
      /* EXISTING AND KNOWN CLIENTS...                                       */
      if (server->debug)
	{
	  char *msg ;

	  /* Interesting, should allow a request to be null ?? probably not...       */
	  if (testMsgType(acehead->msgType, ACESERV_MSGDATA) && request)
	    msg = "Raw Ace data being passed" ;
	  else
	    msg = request ;
	  
	  messout("ProcessQueries passed auth: %s", msg ? msg : "null") ;
	}

      *requestType = ACESOCK_REQ ;

      aceClient->lastAccess = timeNow (); /* last access, to kill  dead clients */

      if (testMsgType(acehead->msgType, ACESERV_MSGDATA))
	{
	  KEYSET ks ;

	  setMsgType(acehead->msgType, ACESERV_MSGFAIL) ;
	  if (aceClient->group != ACEUSER_WRITE && aceClient->group != ACEUSER_ADMIN)
	      pushText (s,"// Sorry, you do not have write or admin access to the database.\n") ;
	  else
	    { 
	      sessionGainWriteAccess() ; /* try to grab it */ 
	      if (!isWriteAccess())	/* may occur is somebody else grabed it */ 
		  pushText (s,"// Sorry, could not get write access, "
			    "// someone else may have it, please try later.\n") ;
	      else
		{
		  ACEIN fi = aceInCreateFromText(request, "", 0) ;
		  ACEOUT fo = aceOutCreateToStack(s, 0) ;
		  
		  aceInSpecial (fi, "\n/\\\t") ;
		  ks = keySetCreate () ;
		  
		  /* We pass in the request in fi, and get the output back in   */
		  /* fo which we then return to the client. First TRUE means    */
		  /* "keep going" even if there are errors, all errors are      */
		  /* reported back in fo. 2nd TRUE means give full stats.       */
		  doParseAceIn(fi, fo, ks, TRUE, TRUE) ;    /* Will destroy fi */

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
		  /* I don't think we want this message now...the preceding  */
		  /* call returns the stuff we want....                      */
		  catText (s, messprintf ("// Read %d objects \n", keySetMax (ks))) ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

		  keySetDestroy (ks) ;
		  setMsgType(acehead->msgType, ACESERV_MSGOK) ;

		  aceOutDestroy(fo) ;
		}
	    }
	}
      else
	{
	  char *word ;
	  int curr_client_id = acehead->clientId ;	    /* Hold on to this for messages. */


	  /* Start query message */
	  messout("Client %6d. Starting query: %s", curr_client_id, request) ;

	  if (strncmp (request, "wspec",5) != 0)
	    {
	      static ACEIN command_in = NULL ; 
	      static ACEOUT result_out = NULL ;
	      struct messContextStruct newMessContext;
	      struct helpContextStruct newHelpContext;
	      struct messContextStruct oldMessContext;
	      struct helpContextStruct oldHelpContext;

	      if (command_in == NULL)
		command_in = aceInCreateFromText(request, "", 0) ;
	      else
		aceInSetNewText(command_in, request, "") ;
	      aceInSpecial(command_in, "\t\\/") ;	    /* forbid sub shells & cmd files. */

	      if (result_out == NULL)
		result_out = aceOutCreateToStack(s, 0) ;
	      else
		aceOutSetNewStack(result_out, s) ;
	  
	      /* register output context for messout, messerror and helpOn
	       * for the duration of aceCommandExecute */
	      newMessContext.user_func = acedbPrintMessage;
	      newMessContext.user_pointer = (void*)result_out;
	      newHelpContext.user_func = helpPrint;
	      newHelpContext.user_pointer = (void*)result_out;
      
	      oldMessContext = messOutRegister (newMessContext);
	      messErrorRegister (newMessContext);
	      oldHelpContext = helpOnRegister (newHelpContext);

	      /* if 0 (zero) is returned, this implies command not recognised*/
	      option = aceCommandExecute(aceClient->aceCommand,
					 command_in, result_out, 
					 (testMsgType(acehead->msgType, ACESERV_MSGENCORE)
					                       ? aceClient->lastCommand : 0),
					 maxChar) ;

	      /* revert back to original registers */
	      messOutRegister (oldMessContext);
	      messErrorRegister (oldMessContext);
	      helpOnRegister (oldHelpContext);

	      /* This is dodgy, why is it here ? surely the encore logic     */
	      /* should take care of this...???                              */
	      setMsgType(acehead->msgType, ACESERV_MSGOK) ;

	      aceClient->lastAccess = timeNow() ;
	  

	      /* NOW READ THIS CAREFULLY, 'q' and 'm'-'M' are special:       */
	      /* 'q' does not put any text on the stack,                     */
	      /* 'm'-'M' do not require us to put text on the stack.         */
	      /*                                                             */
	      /* FOR ANY OTHER OPTION you MUST NOT use pushText because      */
	      /* there is ALREADY text on the stack, use catText only....    */
	      /* You can imagine the bug here, its taken me a couple of hours*/
	      /* to find the eventual cause of this....                      */
	      switch (option)
		{
		case 'q':
		  clientDestroy(server, aceClient) ;
		  acehead->clientId = 0 ;
		  pushText(s, "// A bientot") ;
		  setMsgType(acehead->msgType, ACESERV_MSGKILL) ;
		  *requestType = ACESOCK_KILL ;
		  break ;
	      
		case 'm':
		case 'D':
		case 'B':
		case 'M':
		  aceClient->lastCommand = option ;
		  setMsgType(acehead->msgType, ACESERV_MSGENCORE) ;
		  break ;
		case 's':
		  setMsgType(acehead->msgType, ACESERV_MSGOK) ;
		  break ;
	      
		case CMD_WHO:				    /* who */
		  if (aceClient->group != ACEUSER_ADMIN)
		    {
		      catText (s,"// Sorry, you must have \"admin\" access "
				"to the database to issue the who command.\n") ;
		      setMsgType(acehead->msgType, ACESERV_MSGFAIL) ;
		    }
		  else
		    {
		      catText(s,messprintf("%d active Clients, %d transactions so far",
					    server->nActiveClients, nTransactions)) ;
		      setMsgType(acehead->msgType, ACESERV_MSGOK) ;
		    }
		  *requestType = ACESOCK_REQ ;
		  break ;

		case CMD_SERVERLOG:			    /* server logging... */
		  if (aceClient->group != ACEUSER_ADMIN)
		    {
		      catText (s,"// Sorry, you must have \"admin\" access "
				"to the database to issue the Serverlog command.\n") ;
		      setMsgType(acehead->msgType, ACESERV_MSGFAIL) ;
		    }
		  else
		    {
		      /* For the server we swap both logs just as we do for  */
		      /* SIGHUP send to server.                              */
		      BOOL log, serverlog ;
		      ACEIN request_in = NULL ;
		      char *cp = NULL ;
		      BOOL reopen = FALSE ;
		      char *logname = NULL ;
		      BOOL status = TRUE ;

		      request_in = aceInCreateFromText(request, NULL, 0) ;
		      cp = aceInWord(command_in);
		      if (cp)
			{
			  if (strncmp(cp, "-f", 2) == 0)		    /* -f log_name */
			    {
			      if ((cp = aceInWord(command_in)))
				logname = cp ;
			      else
				{
				  catText(s, "// Error: option -f requires filename argument\n") ;
				  setMsgType(acehead->msgType, ACESERV_MSGFAIL) ;
				  status = FALSE ;
				}
			    }
			  else if (strcmp(cp, "-reopen") == 0)
			    reopen = TRUE ;
			  else
			    {
			      catText(s, "// Error: unknown option.\n") ;
			      setMsgType(acehead->msgType, ACESERV_MSGFAIL) ;
			      status = FALSE ;
			    }
			}

		      if (status)
			{
			  if (reopen)
			    {
			      log = sessionReopenLogFile() ;

			      serverlog = reopenServerLog(server) ;
			    }
			  else
			    {
			      log = sessionNewLogFile(logname) ;

			      serverlog = newServerLog(server, logname) ;
			    }

			  if (!serverlog || !log)
			    {
			      catText(s,messprintf("// Warning, could not %s logs: %s %s",
						   reopen ? "reopen" : "swop",
						   server->server_log ? "" : "serverlog.wrm now disabled",
						   log ? "" : "log.wrm now disabled")) ;
			      setMsgType(acehead->msgType, ACESERV_MSGFAIL) ;
			    }
			  else
			    {
			      catText(s,messprintf("New logs successfully %s.",
						   reopen ? "reopened" : "started")) ;
			      setMsgType(acehead->msgType, ACESERV_MSGOK) ;
			    }
			}

		      aceInDestroy(request_in) ;
		    }
		  *requestType = ACESOCK_REQ ;
		  break ;
	      
		case CMD_PASSWD:			    /* User wants to change their password */
		  /* We are expecting "passwd <userid> <old_hash> <new_hash>" in client msg.   */
		  *requestType = ACESOCK_REQ ;
		  num_words = parseWords(request, &words) ;
		  if (num_words != 4)
		    {
		      catText(s, "// Sorry, change password request incomplete.\n") ;
		      setMsgType(acehead->msgType, ACESERV_MSGFAIL) ;
		      clientDestroy(server, aceClient) ;
		    }
		  else if (strcmp(aceClient->userid, array(words, 1, char*)) != 0)
		    {
		      /* Bad error, => naughty client or code/network bug. */
		      catText(s, "// Sorry, your userid is invalid.\n") ;
		      setMsgType(acehead->msgType, ACESERV_MSGKILL) ;
		      *requestType = ACESOCK_KILL ;
		      clientDestroy(server, aceClient) ;
		    }
		  else
		    {
		      ClientAccessStatus acc_status ;
		      status = changeUserPasswd(server, aceClient->userid, aceClient->nonce,
						array(words, 2, char*), array(words, 3, char*),
						&acc_status) ;
		      if (!status)
			{
			  catText (s, "// Sorry, could not change password,\n"
				    "// please check that you entered your old/new passwords correctly.\n") ;
			  setMsgType(acehead->msgType, ACESERV_MSGFAIL) ;
			}
		      else
			{
			  catText(s, "// Password changed.\n") ;
			  setMsgType(acehead->msgType, ACESERV_MSGOK) ;
			} 
		    }

		  if (num_words != 0)
		    freeWords(words) ;
		  break ;

		case CMD_USER:  /* Admin user wants to add/update/delete a user */
		  *requestType = ACESOCK_REQ ;
		  setMsgType(acehead->msgType, ACESERV_MSGFAIL) ;
		  if (aceClient->group != ACEUSER_ADMIN)
		    {
		      catText (s,"// Sorry, you must have \"admin\" access "
				"to the database to issue the user command.\n") ;
		    }
		  else
		    {
		      /* We are expecting "user passwd|group|new|delete .."  */
		      /* where ".." is dependent on the 2nd word.            */
		      num_words = parseWords(request, &words) ;
		      if (num_words < 3)
			  catText(s, "// Sorry, change user request incomplete.\n") ;
		      else
			{
			  /* Update a user.                                  */
			  status = updateUser(server, words, &acc_status) ;
			  if (!status)
			      catText(s, "// Sorry, could not update userid,\n"
				       "// please check that you entered the command correctly.\n") ;
			  else
			    {
			      catText(s, "// Userid updated.\n") ;
			      setMsgType(acehead->msgType, ACESERV_MSGOK) ;
			    }
			}

		      if (num_words != 0)
			freeWords(words) ;
		    }
		  break ;

		case CMD_DOMAIN:  /* Admin user wants to add/update/delete a domain name */
		  *requestType = ACESOCK_REQ ;
		  setMsgType(acehead->msgType, ACESERV_MSGFAIL) ;
		  if (aceClient->group != ACEUSER_ADMIN)
		    {
		      catText (s,"// Sorry, you must have \"admin\" access "
				"to the database to issue the domain command.\n") ;
		    }
		  else
		    {
		      /* We are expecting "domain group|new|delete name .."  */
		      /* where ".." is dependent on the 2nd word.            */
		      num_words = parseWords(request, &words) ;
		      if (num_words < 3)
			  catText(s, "// Sorry, change domain request incomplete.\n") ;
		      else
			{
			  /* Update a domain entry.                            */
			  status = updateDomain(server, words, &acc_status) ;
			  if (!status)
			      catText(s, "// Sorry, could not update domain name,\n"
				       "// please check that you entered the command correctly.\n") ;
			  else
			    {
			      catText(s, "// Domain name updated.\n") ;
			      setMsgType(acehead->msgType, ACESERV_MSGOK) ;
			    }
			}

		      if (num_words != 0)
			freeWords(words) ;
		    }
		  break ;

		case CMD_GLOBAL:  /* Admin user wants to update database global read/write perms. */
		  *requestType = ACESOCK_REQ ;
		  setMsgType(acehead->msgType, ACESERV_MSGFAIL) ;
		  if (aceClient->group != ACEUSER_ADMIN)
		    {
		      catText(s,"// Sorry, you must have \"admin\" access "
			       "to the database to issue the global command.\n") ;
		    }
		  else
		    {
		      /* We are expecting "global <read|write> <none|passwd|world>"*/
		      num_words = parseWords(request, &words) ;
		      if (num_words != 3)
		      	{
			  catText(s, "// Sorry, change global database permissions "
				   "request incomplete.\n") ;
			}
		      else
			{
			  /* Update the global entry.                        */
			  status = updateGlobalDBPerms(server, 
						       array(words, 1, char*), array(words, 2, char*)) ;
			  if (!status)
			    catText (s, "// Sorry, could not change global database permissions,\n"
				      "// please check that you entered the command correctly.\n") ;
			  else
			    {
			      catText(s, "// Global database permissions updated.\n") ;
			      setMsgType(acehead->msgType, ACESERV_MSGOK) ;
			    }
			}

		      if (num_words != 0)
			freeWords(words) ;
		    }
		  break ;

		case CMD_SHUTDOWN:  /* shutdown [now] */
		  if (aceClient->group != ACEUSER_ADMIN)
		    {
		      catText (s,"// Sorry, you must have \"admin\" access "
				"to the database to issue the shutdown command.\n") ;
		      *requestType = ACESOCK_REQ ;
		      setMsgType(acehead->msgType, ACESERV_MSGFAIL) ;
		    }
		  else
		    {
		      char *lockfile_path = NULL ;
		      char *mesg = NULL ;

		      server->shuttingDown = TRUE ;
		      *requestTarget = ACESOCK_SERVER ;
		      setMsgType(acehead->msgType, ACESERV_MSGKILL) ;

		      /* Lock database if required.                          */
		      /* HACKY, need to look in command_in for norestart,    */
		      /* check how to do this in a pucker way...             */
		      if (strstr(request, ACESERV_NORESTART_OPT) != NULL
			  || checkForServerConfigValue(server, ACESERV_NORESTART, NULL))
			{
			  if (!server->readonly_DB)
			    {
			      lockfile_path = createLockFile(server, ACESERV_SHUTDOWN_LOCKFILE) ;
			      messfree(lockfile_path) ;
			    }
			}
		      word = aceInWord(command_in) ;
		      if (word && strcasecmp(word, "now") == 0)
			{
			  /* All clients will be killed and sent this termination    */
			  /* message and the database is set to not restart.         */
			  *requestType = ACESOCK_SHUTFORCE ;
			  clientDestroy(server, aceClient) ;
			  acehead->clientId = 0 ;

			  mesg = hprintf(0, "// Sorry, emergency shutdown of server now executing\n"
					 "// A bientot\n") ;
			  catText(s, mesg) ;
			}
		      else
			{
			  /* Signal the server that is should not allow any new clients
			   * and that it should quit when the last client quits. */
			  *requestType = ACESOCK_SHUTQUIESCE ;
			  setMsgType(acehead->msgType, ACESERV_MSGOK) ;

			  mesg = hprintf(0, "// The server will now refuse any new connection "
					 "and shutdown when the last client quits "
					 "(currently %d active client%s)",
					 server->nActiveClients,
					 (server->nActiveClients == 1 ? "" : "s")) ;
			  catText (s, mesg) ;
			}


		      /* Should output same text as has been pushed onto the */
		      /* stack, but I need to get all of it off the stack    */
		      /* nondestructively...                                 */
		      appendServerLog(server, mesg) ;
		      messfree(mesg) ;
		    }
		  break ;


#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
		  /* not implemented at the moment...                        */
		case CMD_COMPRESS:			    /* compress yes|no */
		  if (strstr(request, "yes") != NULL)
		    aceClient->compress = TRUE ;
		  else if (strstr(request, "no") != NULL)
		    aceClient->compress = FALSE ;
		  break ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */


		case 0:
		  /* This means the command was not recognised, so we return */
		  /* failed.                                                 */
		  setMsgType(acehead->msgType, ACESERV_MSGFAIL) ;

		default:
		  /* We don't do anything because anything not above should  */
		  /* have been caught by command.c                           */
		  ;
		}
	  

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
	      aceInDestroy (command_in) ;
	      aceOutDestroy(result_out) ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

	    }
	  else if (!strcmp (request, "wspec1"))
	    {
	      /* export wspec 
		  does not work, i do not know how to untar at the client side
		  catBinary (s, arrp (server->wspecTar, 0, char), arrayMax(server->wspecTar)) ;
	       */
	      catText(s,"// Sorry, the wspec1 command is not supported.\n") ;
	      *requestType = ACESOCK_REQ ;
	      setMsgType(acehead->msgType, ACESERV_MSGFAIL) ;
 	    }
	  else if (!strcmp (request, "wspec"))
	    /* MOVE INTO ABOVE CASE STATEMENT USING  CMD_WSPEC  value              */
	    {
	      /* export wspec */
	      *requestType = ACESOCK_REQ ;
	      setMsgType(acehead->msgType, ACESERV_MSGFAIL) ;

	      if (aceClient->group != ACEUSER_ADMIN)
		{
		  catText(s,"// Sorry, you must have \"admin\" access "
			    "to the database to issue the who command.\n") ;
		}
	      else if (server->wspecTarEncoded == NULL)
		{
		  catText(s,"// Sorry, the wspec command is not supported on this platform.\n") ;
		}
	      else
		{
		  /* I bet this doesn't work....                             */
		  /* I'm not sure it needs to be catBinary given that the    */
		  /* file is uuencoded in fact....                           */
		  catBinary (s, arrp (server->wspecTarEncoded, 0, char),
			     arrayMax(server->wspecTarEncoded)) ;
		  setMsgType(acehead->msgType, ACESERV_MSGOK) ;
		}
	    }
      
	  /*  if (oldAnswer != stackText(s, 0))
	      messout("Relocation: length(answer) = %d",  stackMark(s)) ;
	      oldAnswer = stackText(s, 0) ;
	  
	  */


	  /* End query message, useful when server is run in foreground for seeing how
	   * long commands are taking. */
	  messout("Client %6d.   Ending query: %s", curr_client_id, request) ;
	}
    }


  /* I guess this is sort of a good place to compress at...                  */
  /* It won't be seen by any other code I guess...                           */
  if (aceClient->compress == TRUE)
    {
      compressMessage(s) ;
    }


  return s ;
}

/**************************************************/
/********* ACEDB non graphic Query Server  ********/
/**************************************************/

/* Makes a copy of wspec which can be asked for later by the "wspec" request */
/* to the server.                                                            */
/*                                                                           */
Array copy_wspec(BOOL encode)
{
  Array aa = NULL ;
 
#if !defined(__CYGWIN__)
  /* CYGWIN does not currently do the tar stuff because we don't want users  */
  /* to have install tar on their windows machines. If this becomes an issue */
  /* we will need to provide an alternative way of doing this although I     */
  /* have no idea what that might be.                                        */
  FILE *f = 0 ;
  char c, *cp ;
  int n = 0 ;

  aa = arrayCreate (20000, char) ; 

  cp = dbPathStrictFilName("wspec", "", 0,"rd", 0) ;
  if (!cp) 
    messcrash ("Server cannot open wspec, sorry") ;
  
  freeOut("\n") ;
  if (encode)
    f = popen(messprintf("cd %s ; tar chf - %s %s | uuencode server.wspec.tar",
			 cp, "cachesize.wrm cachesize.wrm database.wrm displays.wrm",
			 "layout.wrm models.wrm options.wrm psfonts.wrm subclasses.wrm xfonts.wrm"),"r") ;
  else
    f = popen(messprintf("cd %s ; tar chf - %s %s",
			 cp, "cachesize.wrm cachesize.wrm database.wrm displays.wrm",
			 "layout.wrm models.wrm options.wrm psfonts.wrm subclasses.wrm xfonts.wrm"),"r") ;

  while (c = getc(f), c  != (char)EOF)
    array (aa, n++, char) = c ;

  pclose (f) ;

  messfree(cp);
#endif /* !defined(__CYGWIN__) */

  return aa ;
}


/* Defines a routine to return the compile date of this file. */
UT_MAKE_GETCOMPILEDATEROUTINE()

/************************************************************/

     /* THIS IS ALSO WRONG, BECAUSE OF WHAT HAPPENS FOR INETD...this is      */
     /* completely wrong we should be messexitting....                       */
static void showUsage (void)
{
  messerror(USAGE_STR) ;
  exit(EXIT_FAILURE) ;
}


/************************************************************/

int main(int argc, char **argv)
{
  BOOL nosigcatch ;
  u_long p ;
  int n ;
  char *cp ;
  char *serv_time, *client_time ;
  BOOL get_write_access = FALSE ;
  static AceServStruct server ;				    /* Main control block for server. */
  ServerSessionCBstruct server_cbs = {{{sessionInitCB}, &server},
				      {{sessionCheckFilesCB}, &server},
				      {sessionMessOutCB, &server},
				      {sessionMessErrCB, &server},
				      {{(ServerCB)sessionExitCB}, &server},
				      {{(ServerCB)sessionCrashCB}, &server},
				      {{sessionSigHup}, &server},
				      {{sessionSigTerm}, &server}} ;
							    /* Our DB session callbacks. */
  char *locale ;


  /* We cannot cope with anything other than the standard locale. */
  locale = setlocale(LC_ALL, "C") ;


#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
  struct rlimit mylimit ;

  /* TEST CODE....                                                           */
  if (chdir("/tmp") != 0)
    messcrash("cannot change directory") ;

  /* Try setting our core limit ourselves...                                 */
  if (getrlimit(RLIMIT_CORE, &mylimit) != 0)
    messcrash("cannot get core limit.") ;
  mylimit.rlim_cur = mylimit.rlim_max ;
  if (setrlimit(RLIMIT_CORE, &mylimit) != 0)
    messcrash("cannot set core limit.") ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */




  /* Does not return if user typed "-version" option.                        */
  appCmdlineVersion(&argc, argv) ;

  /*****************************/

  prefInit() ;						    /* Set up App preferences. */

  /*****************************/


  /* Check for "no signal catching" option.                                  */
  nosigcatch = getCmdLineOption(&argc, argv, NOSIGCATCH_OPT, NULL) ;


  /* Initialise server state.                                                */
  initServerState(&server) ;

  /* Check for "silent" option => no terminal messages.                      */
  server.silent = getCmdLineOption(&argc, argv, "-silent", NULL) ;

  /* Check for "readonly" option, allows running on databases we don't have permissions for. */
  server.readonly_DB = getCmdLineOption(&argc, argv, "-readonly", NULL) ;


  /* This option will force acedb to initialise the database without prompting. */
  if (getCmdLineOption (&argc, argv, "-init", NULL))
    sessionSetforceInit(TRUE);


  /* Start with write access. */
  if (!(server.readonly_DB))
    get_write_access = getCmdLineOption(&argc, argv, "-writeaccess", NULL) ;


#ifdef GIFACESERVER

  /* Crucial, must set up gif sub menu routine, but also must set global     */
  /* UUUGGGHHH to make sure that gif displays get cleared up properly.       */
  gifEntry = gifControl ;
  isGifDisplay = TRUE ;

  freeinit () ;			/* must come before graphInit */


  /* Initialise the graphics package and its interface for acedb.            */
  graphInit (&argc, argv);
  acedbGraphInit() ; 

  /* init message system details.                                            */
  messSetMsgInfo("sgifaceserver", aceGetVersionString()) ;

#else

  messSetMsgInfo("saceserver", aceGetVersionString()) ;

#endif


  /* There is a big problem here...none of these messages will go to the     */
  /* correct place...OK if the server starts as a terminal app., but if inetd*/
  /* started they will all just disappear. This is true until we open the    */
  /* server log file and log.wrm in the database directory...not too good.   */


  if (sizeof(int) != 4)
    messcrash ("On this machine an int is %d bytes long,\n"
	       "sorry the acedb server relies on machines having an int that is 4 bytes",
	       sizeof(int)) ;

  /* uuuggggghhhh convert to proper options....                              */
  n = 2 ;

  /* Get the port for the server.                                            */
  /* If running under inetd we should ignore any port specified from the     */
  /* cmd. line.                                                              */
  if (argc > n && (!strcmp(argv[n],"-port")))
    {
      /* ignore, optional -port */
      n++;
    }
  if (argc > n && (!index(argv[n],':')) && (sscanf(argv[n],"%lu",&p) == 1)
      && !server.inetd_started)
    {
      server.port = p;
      n++;
    }

  /* Get timeouts and other stuff... all as a ":" separated string.          */
  if (argc > n)
    {
      int t ;

      cp = argv[n] ;
      if (sscanf(cp,"%d",&t) == 1)
	{ 
	  server.clientTimeOut = t ;

	  if ((cp = strstr(argv[n],":")) && sscanf(++cp,"%d",&t) == 1)
	    {
	      server.serverTimeOut = t;

	    if ((cp = strstr(cp,":")) && sscanf(++cp,"%d",&t) == 1)
	      {
		if (t < 0)
		  t = 0 ;
		server.max_bytes = t * 1024 ;		    /* user specifies kb. */

		if ((cp = strstr(cp,":")) && sscanf(++cp,"%d",&t) == 1)
		  server.autoSaveInterval = t;
	      }
	    }
	  n++;
	}
      if (server.clientTimeOut < ACESERV_MIN_TIMEOUT
	  || server.serverTimeOut < ACESERV_MIN_TIMEOUT)
	showUsage() ;					    /* exits */
    }

  if (argc < 2 || argc > n)
    showUsage() ;					    /* exits */

  /* Callback registration                                                   */
  /* Add server specific callbacks for initialisation, message output,       */
  /* termination etc. to session control, these will be called during acedb  */
  /* intialisation etc.                                                      */
  /* NOTE, none of the above messages will be seen for an inetd started      */
  /* server...sigh...                                                        */
  sessionRegisterServerCB(&server_cbs) ;


  /* ACEDB initialisation...                                                 */
  /* Initialise acedb passing the database directory if supplied.            */
  aceInit(argc > 1 ? argv[1] : 0) ;


  /* Turn off all saving if server is readonly, note that clients can send commands that alter
     stuff in the im-memory database, its just that it can't be saved back to disk. */
  if (server.readonly_DB)
    sessionSetSaveStatus(FALSE) ;


  if (get_write_access && !sessionGainWriteAccess())
    messcrash ("Cannot get write access.") ;


  /* Set timestamps off for logging by default, the servers log routines will turn this
   * on/off as required. */
  setTimeStampsAceLogFile(FALSE) ;


  /* Set some process resources to unlimited, low resources are logged.      */
  utUnlimitResources(FALSE) ;

  /* once the database is open it is important to deal with signals */
  signalCatchInit(TRUE,					    /* init Ctrl-C as well */
		  nosigcatch) ;


#ifdef GIFACESERVER
  displayInit();	     /* init acedb displays,
				parse wspec/displays.wrm etc,
				comes after aceInit (it uses the db) */
  {
    char *gff_version_str ;
    int gff_version = 2 ;                                    /* version 2 is default for now. */

    if (checkForServerConfigValue(&server, "GFF_VERSION", &gff_version_str))
      {
        char *config_file = "\"wspec/"ACESERV_CONFIG_NAME"."ACESERV_CONFIG_EXT"\" configuration file" ;


        if (!utStr2Int(gff_version_str, &gff_version))
          {
            messerror("Bad %s string \"%s\" in the %s.",
                      "GFF_VERSION", gff_version_str, config_file) ;
          }
        else if (!fMapGifSetGFFVersion(gff_version))
          {
            messerror("Unsupported gff version \"%d\" in the %s.",
                      gff_version, config_file) ;
          }
        else
          {
            /* For now let's also messout() this so it goes to the terminal too. */
            messout("Default gff version is now \"%d\" (set in %s).",
                    gff_version, config_file) ;

            messdump("Default gff version is now \"%d\" (set in %s).",
                     gff_version, config_file) ;
          }
      }
  }
#endif


  /* Set up directory paths, its important if we are inetd started to set    */
  /* the working directory, otherwise we just inherit it....                 */
  server.dbpath = dbPathMakeFilName(NULL, NULL, NULL, 0) ;
  if (!(server.working_dir = getWorkingDir()))
    server.working_dir = server.dbpath ;
  if (chdir(server.working_dir) != 0)
    messcrash("server cannot set working directory during initialisation.") ;
  

  /* Report start of server.                                                 */
  cp = getenv("HOST") ;
  if (!cp)
    cp = "(unknown)" ;

  serv_time = client_time = "" ;
  if (server.clientTimeOut == ACESERV_INFINITE_TIMEOUT)
    client_time = " (= " ACESERV_INFINITE_TIMEOUT_STR ") " ;
  if (server.serverTimeOut == ACESERV_INFINITE_TIMEOUT)
    serv_time = " (= " ACESERV_INFINITE_TIMEOUT_STR ") " ;


  /* start up message, has a timestamp embedded so don't timestamp each one. */
  {
    int old_freq = server.logmsgs_per_timestamp ;

    server.logmsgs_per_timestamp = 0 ;
    messout("#### Server started at %s", timeShow(timeNow())) ;
    messout("#### host=%s  listening port=%d", cp, server.port);
    messout("#### Database dir=%s", server.dbpath) ;
    messout("####  Working dir=%s", server.working_dir) ;
    messout("#### clientTimeout=%d%s serverTimeout=%d%s maxbytes=%d autoSaveInterval=%d",
	    server.clientTimeOut, client_time,
	    server.serverTimeOut, serv_time,
	    server.max_bytes, server.autoSaveInterval); 
    server.logmsgs_per_timestamp = old_freq ;
  }


  server.wspecTarEncoded = copy_wspec(TRUE) ;
  /*
    i do not know how to recosntruct the non encoded file at the other hand, sorry
    copy_wspec (FALSE) ;
    */


  /* Fundamental associator for holding hash of all connected clients.       */
  server.clientAss  = assCreate() ;
  
  /* This function loops listening for clients to connect, it should never   */
  /* return.                                                                 */
  aceSocketListen(server.port, server.serverTimeOut, server.clientTimeOut,
		  processRequests, &server) ;

  messcrash("aceSocketListen() returned indicating internal logic error.") ;

  return EXIT_FAILURE ;
}


/**************************************************/

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

/**************************************************/

void initServerState(AceServ server)
{
  server->debug = FALSE ;

  if ((server->inetd_started = isInetdDaemon(&(server->port))) == FALSE)
    server->port = ACESERV_PORT ;

  server->working_dir = NULL ;
  server->silent = FALSE ;
  server->readonly_DB = FALSE ;
  server->clientTimeOut = ACESERV_CLIENTTIMEOUT ;
  server->serverTimeOut = ACESERV_SERVERTIMEOUT ;
  server->max_bytes = ACESERV_MAXBYTES ;
  server->autoSaveInterval = ACESERV_AUTOSAVEINTERVAL ;
  server->nActiveClients = 0 ; 
  server->logmsgs_per_timestamp = 0 ;
  server->logmsgs_total = 0 ;
  server->shuttingDown = FALSE ;
  server->clientAss = 0 ;
  server->pidfile = NULL ;
  server->crashLockFile = NULL ;
  server->undetectable_lockfile = NULL ;
  server->crashfile_found_at_startup = FALSE ;
  server->wspecTar = 0 ;
  server->wspecTarEncoded = 0 ;

  return ;
}



/**************************************************/

/* I don't like the assymetry with reads being via buffer/length and writes  */
/* being via stacks, we should the same for both directions, I don't mind    */
/* if its stacks but lets do it both ways.                                   */

/* This routine is the acedb interface to the socket layer, it accepts       */
/* requests from the socket layer and interprets them.                       */
/* Note the intent is to divide requests into two types: those generated by  */
/* the socket layer which are interpreted here and those generated by a      */
/* client which are interpreted in the processQueries routine. It just helps */
/* a bit to seperate out the logic of these two.                             */
/*                                                                           */
static Stack processRequests(void *server_in,
			     ClientConnect connection, ClientConnectionID connection_id,
			     AceSocketRequestTarget *requestTarget,
			     AceSocketRequestType *requestType,
			     ACE_HEADER hh, char *request)
{
  Stack s = 0, t = 0 ;
  AceServ server = (AceServ)server_in ;

  /* Check request from socket layer to make sure it is OK, exit if not.     */
  checkRequest(*requestTarget, *requestType) ;

  if (*requestTarget == ACESOCK_SERVER)
    {
      if (*requestType == ACESOCK_EXIT || *requestType == ACESOCK_TERM)
	{
	  closeAcedbServer(server, *requestType) ;	    /* EXIT THE PROGRAM. */
	}
      else if (*requestType == ACESOCK_TIMEDOUT)
	{
	  t = stackCreate(500) ;
	  pushText(t, "// Server has timed out because there have been\n"
		   "// no requests recently and will now close down.\n// A bientot\n") ;

	}
    }
  else if (*requestTarget == ACESOCK_CLIENT)
    {
      if (*requestType == ACESOCK_REQ)
	{
	  s = processQueries(server, connection, connection_id,
			     requestTarget, requestType, request, hh) ;

	  /* This is very inefficient, needs altering so this routine hands  */
	  /* a new stack to processQueries each time...                      */
	  /* I tried just handing the stack in each time but the code        */
	  /* bombed on a big encore request, not sure why. I'll do this if   */
	  /* I have some spare time.                                         */
	  t = stackCopy (s, 0) ;
	}
      else if (*requestType == ACESOCK_KILL)
	{
	  /* This means the socket layer is signalling a problem with the    */
	  /* client socket, so don't try to send back a message to client.   */
	  s = processQueries(server, connection, connection_id,
			     requestTarget, requestType, "quit", hh) ;
	}
      else if (*requestType == ACESOCK_TIMEDOUT)
	{
	  /* signal a client timeout as detected by the server.              */
	  s = processQueries(server, connection, connection_id,
			     requestTarget, requestType, "quit", hh) ;
	  t = stackCreate(500) ;
	  pushText(s, "// Sorry, you have been inactive for too long\n"
		   "// and have been disconnected.\n// A bientot\n") ;
	}
    }

  return t ;
}   


/**************************************************/

/* Check validity of request from socket code. Add any new checks to         */
/* here rather then cluttering up main socket code.                          */
static void checkRequest(AceSocketRequestTarget target, AceSocketRequestType request)
{

  if (target != ACESOCK_SERVER && target != ACESOCK_CLIENT)
    messcrash("Internal logic error, acedb server code received bad target from socket code:\n"
	      "currTarget currTarget has invalid value: %d \n"
	      "(should be %d or %d).",
	      target, ACESOCK_SERVER, ACESOCK_CLIENT) ; 

  if (target == ACESOCK_SERVER
      && (request != ACESOCK_EXIT && request != ACESOCK_TIMEDOUT && request != ACESOCK_TERM))
    messcrash("Internal logic error, acedb server code received bad request from socket code:\n"
	      "currTarget == ACESOCK_SERVER, but currRequest has invalid value: %d \n"
	      "(should be %d or %d).",
	      request, ACESOCK_EXIT, ACESOCK_TIMEDOUT) ; 

  if (target == ACESOCK_CLIENT
      && (request != ACESOCK_REQ && request != ACESOCK_KILL && request != ACESOCK_TIMEDOUT))
    messcrash("Internal logic error, acedb server code received bad request from socket code:\n"
	      "currTarget == ACESOCK_CLIENT, but currRequest has invalid value: %d \n"
	      "(should be %d or %d or %d).",
	      request, ACESOCK_REQ, ACESOCK_KILL, ACESOCK_TIMEDOUT) ; 


  return ;
}


/**************************************************/

/* Server log is where we write messages that result from server execution,
 * or perhaps debugging messages. It's really for non-fatal errors, fatal
 * ones are recorded in the database log file. It's a place where all non-
 * fatal output can be held without cluttering up the database log.
 *
 * These routines are responsible for handling opening/appending/closing of the file.
 * Note that open/close is always timestamped, append is done according to
 * frequency in serverconfig file. */

static void openServerLog(AceServ server)
{
  BOOL log_ok = FALSE ;
  ACEOUT logfile = NULL ;

  if (!server->readonly_DB)
    {
      log_ok = openAceLogFile(SERVER_LOGNAME, &logfile, TRUE) ;

      /* Note that this message will not reach the user if we are inetd started. */
      if (!log_ok)
	messcrash("Could not create server log file, check that database directory is writeable "
		  "by server and that the disk is not full, or that there is some other problem "
		  "with the directory.") ;
      
      server->server_log = logfile ;
    }

  return ;
}

static void appendServerLog(AceServ server, char *mesg)
{
  if (!server->readonly_DB && server->server_log)
    {
      /* If user has requested new log file then swop to new one.            */
      if (logfile_request_new_G)
	{
	  logfile_request_new_G = FALSE ;

	  newServerLog(server, NULL) ;
	}

      if (server->server_log && mesg)
	{
	  /* Add timestamp to log message according to specified frequency. */
	  server->logmsgs_total++ ;
	  if (server->logmsgs_per_timestamp)
	    {
	      if (server->logmsgs_per_timestamp == 1
		  || server->logmsgs_total % server->logmsgs_per_timestamp == 0)
		setTimeStampsAceLogFile(TRUE) ;
	    }

	  appendAceLogFile(server->server_log, mesg) ;

	  setTimeStampsAceLogFile(FALSE) ;
	}
    }

  return ;
}

static void closeServerLog(AceServ server, aceLogExitType exit_type)
{
  if (!server->readonly_DB && server->server_log)
    closeAceLogFile(server->server_log, exit_type) ;

  return ;
}

static BOOL newServerLog(AceServ server, char *old_logfile_name)
{
  BOOL result = FALSE ;
  ACEOUT new_logfile = NULL ;

  if (!server->readonly_DB)
    {
      if (server->server_log)
	{
	  char *old_log = NULL ;

	  if (old_logfile_name && *old_logfile_name)
	    {
	      old_log = hprintf(0, "%s-%s", old_logfile_name, SERVER_LOGNAME) ;
	    }

	  new_logfile = startNewAceLogFile(server->server_log,
					   (old_log ? old_log : SERVER_LOGNAME),
					   TRUE, NULL, FALSE) ;
	  if (old_log)
	    messfree(old_log) ;
	}
      else
	{
	  openAceLogFile(SERVER_LOGNAME, &new_logfile, FALSE) ;
	}

      if (new_logfile)
	{
	  server->server_log = new_logfile ;
	  result = TRUE ;
	}
      else
	{
	  server->server_log = NULL ;
	  result = FALSE ;
	}
    }

  return result ;
}

static BOOL reopenServerLog(AceServ server)
{
  BOOL result = FALSE ;
  ACEOUT new_logfile = NULL ;

  if (!server->readonly_DB)
    {
      if (server->server_log)
	{
	  new_logfile = server->server_log ;
	  result = reopenAceLogFile(&new_logfile) ;

	  if (result)
	    {
	      server->server_log = new_logfile ;
	      result = TRUE ;
	    }
	  else
	    {
	      server->server_log = NULL ;
	      result = FALSE ;
	    }
	}
    }

  return result ;
}


/************************************************************/

/* All in one routine to find a particular file, the routine logs any errors */
/* in finding a opening the file. In most cases failure will mean that the   */
/* calling code decides to messcrash.                                        */
ACEIN findAndOpenFile(AceServ server, char *filename, char **filepath_out)
{
  char *filepath = NULL ;
  ACEIN file = NULL ;
  char *mesg = NULL ;

  /* Can we find the file ?                                                  */
  filepath = dbPathFilName("wspec", filename, "wrm", "r", 0) ;
  if (filepath == NULL)
    {
      mesg = hprintf(0, "Server cannot find the \"%s.wrm\" file, "
		     "the server may not run on this database until it can find this file.",
		     filename) ;
    }

  /* Can we open the file ?                                                  */
  if (filepath != NULL)
    {
      file = aceInCreateFromFile(filepath, "r", "", 0) ;
      if (file == NULL)
	mesg = hprintf(0, "Server cannot open the \"%s.wrm\" file, "
		       "the server may not run on this database until it can open this file.",
		       filename) ;
    }

  if (mesg != NULL)
    {
      appendServerLog(server, mesg) ;
      messfree(mesg) ;
    }


  /* Return the path for the file if requested.                              */
  if (file != NULL && filepath_out != NULL)
    *filepath_out = filepath ;

  return file ;
}


/************************************************************/

/* Split out words from the supplied string and return an array of pointers  */
/* to copies of the words. The routine returns 0 if no words were found,     */
/* otherwise the number of words pointed to by the array is returned.        */
/* Currently it is the responsibility of the caller to free the word strings */
/* and the array.                                                            */
/*                                                                           */
/* Use freeWords to get rid of the array and associated strings.             */
/*                                                                           */
int parseWords(char *request, Array *word_array_out)
{
  int num_words = 0 ;
  ACEIN user_info = NULL ;
  Array words = NULL ;
  char *next_word ;

  user_info = aceInCreateFromText(request, "", 0) ;
  if (user_info == NULL)
    messcrash("Could not create aceIn from text for user arguements retrieval") ;

  if (aceInCard(user_info))
    {
      words = arrayCreate(10, char*) ;
  
      while ((next_word = aceInWord(user_info)))
	{
	  array(words, arrayMax(words), char*) = strnew(next_word, 0) ;
	  num_words++ ;
	}
  
      if (num_words > 0)
	{
	*word_array_out = words ;
	}
      else
	{
	  arrayDestroy(words) ;
	}
    }

  aceInDestroy(user_info) ;

  return num_words ;
}

int stringInWords(Array words, char *string)
{
  enum {INVALID = -1} ;
  int index = INVALID ;
  int i ;
  
  for (i = 0 ; i < arrayMax(words) && index == INVALID ; i++)
    {
      if (strcmp(array(words, i, char*), string) == 0)
	index = i ;
    }

  return index ;
}


void freeWords(Array words)
{
  int i ;

  for (i = 0 ; i < arrayMax(words) ; i++)
    {
      messfree(array(words, i, char*)) ;
    }
  arrayDestroy(words) ;

  return ;
}


/************************************************************/

static void compressMessage(Stack msg)
{

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
  int result ;
  char *new_msg ;
  /* not sure about efficiency here...too much copying ??? probably.         */
  new_msg = strnew((StackMark(msg) * 1.2) + 12) ;
  
etc.etc.

  result = compress(Bytef *dest, uLongf *destLen, const Bytef *source, uLong sourceLen); 
       Compresses the source buffer into the destination buffer. sourceLen is the byte length of the source buffer. Upon entry, destLen is the total size of the destination buffer,
       which must be at least 0.1% larger than sourceLen plus 12 bytes. Upon exit, destLen is the actual size of the compressed buffer.



#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

  return ;
}


/************************************************************/

/* Look for the specified keyword in the serverconfig.wrm file, return TRUE
 * if found, FALSE otherwise.
 * If keyword_value is non-NULL the word following the keyword is returned,
 * if there is no word then keyword_value is not changed. The caller is
 * responsible for messfree'ing the returned string.
 * It is a fatal error if serverconfig.wrm cannot be opened. */
BOOL checkForServerConfigValue(AceServ server, char *keyword, char **keyword_value)
{
  BOOL keyword_found = FALSE ;
  ACEIN server_file = NULL ;
  char *filename = ACESERV_CONFIG_NAME ;
  char *word ;

  if (!(server_file = findAndOpenFile(server, filename, NULL)))
    {
      char *mesg = NULL ;

      mesg = hprintf(0, "Cannot continue without being able to find/open %s.wrm", filename) ;

      appendServerLog(server, mesg) ;

      messcrash("%s", mesg) ;

      messfree(mesg) ;					    /* belt and braces, messcrash() exits. */
    }

  aceInSpecial(server_file, "\n") ;

  while (aceInCard(server_file) && keyword_found == FALSE)
    {
      word = aceInWord(server_file) ;
      if (word && (strcmp(word, keyword) == 0))
	{
	  keyword_found = TRUE ;

	  if (keyword_value && (word = aceInWord(server_file)))
	    *keyword_value = strnew(word, 0) ;
	}
    }

  aceInDestroy(server_file) ;

  return keyword_found ;
}


/************************************************************/

/* Create a lockfile which will help to trap various server close down types,*/
/* e.g. shutdown, detected crash, undetected crash.                          */
/*                                                                           */
/* Returns the lockfile path if lockfile could be created, NULL otherwise.   */
/* N.B. caller must messfree path when finished with.                        */
/*                                                                           */
static char *createLockFile(AceServ server, char *lockfile_name)
{
  char *path ;
  FILE *lockfile ;

  path = dbPathMakeFilName(ACESERV_LOCKFILE_DIR, lockfile_name,
			   ACESERV_LOCKFILE_EXT, 0) ;
 
   /* Create a lock file to show that the server crashed/exitted.             */
  lockfile = filopen(path, NULL, "a") ;
  if (!lockfile)
    {
      char *mesg = NULL ;

      mesg = hprintf(0, "Warning: could not create the %s lockfile in "
		     "the database directory, this may lead to database integrity being compromised "
		     "because there will nothing to stop the server being restarted on this database.",
		     lockfile_name) ;
      appendServerLog(server, mesg) ;
      messfree(mesg) ;

      messfree(path) ;
      path = NULL ;
    }
  else
    filclose(lockfile) ;
  
  return(path) ;
}


/* Remove the undetectable crash lock file, unlike the other lock files      */
/* (which are always manually removed), this lock file must be removed       */
/* automatically for any shutdown that the code has been able to detect.     */
/* N.B. the lockfile will only have been created if the NORESTART option     */
/* was specified in the serverconfig file.                                   */
/*                                                                           */
static void removeUndetectableLockFile(AceServ server_state)
{
  if (server_state->undetectable_lockfile != NULL)
    {
      if (!filremove(server_state->undetectable_lockfile, NULL))
	{
	  char *mesg = NULL ;
      
	  mesg = hprintf(0, "Warning, server could not remove \"undetectable crash\" lockfile "
		   "lockfile name was: %s", server_state->undetectable_lockfile) ;
	  appendServerLog(server_state, mesg) ;
	  messfree(mesg) ;
	}
      else
	{
	  messfree(server_state->undetectable_lockfile) ;
	  server_state->undetectable_lockfile = NULL ;
	}
    }

  return ;
}



/************************************************************/


/* The below routines are callback routines that get called from session.c   */
/* code. We need these routines to enable us to check for lock files at      */
/* start-up and to write lock files if a messcrash occurs.                   */
/*                                                                           */

/* Must be called after first call to sessionSetPath() to set up             */
/* the database path.                                                        */
static void sessionInitCB(void *data_ptr)
{
  AceServ server = (AceServ)data_ptr ;

  /* Turn on debugging if requested.                                         */
  if (checkForServerConfigValue(server, ACESERV_DEBUG, NULL))
    server->debug = TRUE ;

  /* Is the database to be completely read only....??                        */
  if (checkForServerConfigValue(server, ACESERV_READONLY_DATABASE, NULL))
    server->readonly_DB = TRUE ;

  /* Set the log messages per time stamp frequency, for debug every message is
   * time stamped, otherwise depends on user because timestamps are expensive. */
  if (server->debug)
    server->logmsgs_per_timestamp = 1 ;
  else
    {
      char *frequency_str = NULL ;

      if (checkForServerConfigValue(server, ACESERV_LOG_TIMESTAMP_FREQUENCY, &frequency_str))
	{
	  if ((server->logmsgs_per_timestamp = atoi(frequency_str)) < 0)
	    server->logmsgs_per_timestamp = 0 ;

	  messfree(frequency_str) ;
	}
    }


  /* open the server log file, can't be done until acedb is initialised, any 
   * error messages output before this call will not go to the log and may
   * just be lost altogether. */
  openServerLog(server) ;


  /* Construct the path of the crash lock file and keep it around for use    */
  /* by serverDie() when memory may be scarce/corrupted...DON'T free this.   */
  server->crashLockFile = dbPathMakeFilName(ACESERV_LOCKFILE_DIR, ACESERV_CRASH_LOCKFILE,
					    ACESERV_LOCKFILE_EXT, 0) ;

  return ;
}


/* This routine is called from sessionInit() to allow us to check for config */
/* files and lock files in the database directory. It is only called once    */
/* when the server starts up and acedb is initialised.                       */
/*                                                                           */
/* Config files:                                                             */
/*   Check that the password file exists, we don't check its contents.       */
/*                                                                           */
/* Shutdown and Crash Lock files:                                            */
/*   These are only written to the /database directory if the NORESTART      */
/*   keyword is specified in serverconfig.wrm.                               */
/*                                                                           */
/*   If the shutdown lock file is there is means that an admin user has      */
/*   shutdown the database and does not want it restarted.                   */
/*                                                                           */
/*   If the crash lockfile is there it means that the server crashed and     */
/*   the admin user does not want it restarted.                              */
/*                                                                           */
/* note how we do NOT automatically remove the lock files, the whole point   */
/* is that this is drastic action and locks should have to be manually       */
/* removed by an administrator.                                              */
/*                                                                           */
/* Undetectable Crash lockfile:                                              */
/*   This is only written to the /database directory if the NORESTART        */
/*   keyword is specified in serverconfig.wrm.                               */
/*                                                                           */
/*   We write this file when the server starts up and remove it when we      */
/*   shut down. Thus if we find it when we start up it means there has       */
/*   been an undetectable crash.                                             */
/*                                                                           */
/* inetd and the server:                                                     */
/*                       If we crash in the server leaving a message pending */
/* on the listening socket, inetd will detect that message when it resumes   */
/* control of the listening socket. It will then try to start the server     */
/* again. This can cause an ugly looping where the server repeatedly crashes */
/* and inetd repeatedly tries to restart the server. To solve this I have    */
/* added code that attempts to make sure that when a messcrash occurs, the   */
/* listening socket gets cleared. This seems to work.                        */
/*                                                                           */
static void sessionCheckFilesCB(void *data_ptr)
{
  AceServ server = (AceServ)data_ptr ;
  ACEIN passwd_file = NULL ;
  char *passwd_path = NULL ;
  char *lockfile ;
  char *exit_message = NULL ;

  /* Check for server config files.                                          */
  passwd_file = findAndOpenPasswdFile(server, &passwd_path) ;
  if (passwd_file == NULL)
    messExit("Server cannot find the password file: \"%s.%s\",\n"
	     "// the server cannot be run on this database until there is a password file.",
	     ACESERV_PASSWD_NAME, ACESERV_PASSWD_EXT) ;
  else
    {
      aceInDestroy(passwd_file) ;
      messfree(passwd_path) ;
    }


  /* Check for server pid file and write out one if required by server config. */
  doServerPidFile(server) ;


  /* THESE COMMENT BLOCKS NEED ALMALGAMATING...                              */

  /* Check for the undetectable crash lockfile, if it exists this means we did not */
  /* shut down cleanly when we exitted last time. Print a warning to the log */
  /* and if "no restart" has been specified then don't restart.              */
  /* If the file doesn't exist, then all is well so write it to this         */
  /* directory.                                                              */

  /* Check for lock files (name of file shows reason for locking), if one    */
  /* exists then do not restart, log this as an error and then exit.         */
  if ((lockfile = dbPathStrictFilName(ACESERV_LOCKFILE_DIR, 
				      ACESERV_SHUTDOWN_LOCKFILE,
				      ACESERV_LOCKFILE_EXT, "r", 0)) != NULL)
    exit_message = ACESERV_SHUTDOWN_LOCKFILE_MSG ;
  else if ((lockfile = dbPathStrictFilName(ACESERV_LOCKFILE_DIR, 
					   ACESERV_CRASH_LOCKFILE,
					   ACESERV_LOCKFILE_EXT, "r", 0)) != NULL)
    exit_message = ACESERV_CRASH_LOCKFILE_MSG ;
  else if ((lockfile = dbPathStrictFilName(ACESERV_LOCKFILE_DIR, 
					   ACESERV_UNDETECTABLECRASH_LOCKFILE,
					   ACESERV_LOCKFILE_EXT, "r", 0)) != NULL)
    exit_message = ACESERV_UNDETECTABLECRASH_LOCKFILE_MSG ;
  else
    {
      lockfile = NULL ;

      /* If NORESTART is specified then write the server undetectable crash  */
      /* lock file...                                                        */
      if (!server->readonly_DB)
	{
	  if (checkForServerConfigValue(server, ACESERV_NORESTART, NULL))
	    server->undetectable_lockfile = createLockFile(server, ACESERV_UNDETECTABLECRASH_LOCKFILE) ;
	}
    }

  if (lockfile)
    {
      server->crashfile_found_at_startup = TRUE ;

      messExit(exit_message, lockfile) ;
    }

  return ;
}

/* server pid file:
 *     check to see if one already exists and log a warning if it does.
 *     if required, create a new pid file. */
static void doServerPidFile(AceServ server)
{
  char *pid_path = NULL ;
  ACEOUT pid_file = NULL ;

  /* Always check for pid file, nags database admin. person to tidy up. */
  if ((pid_path = dbPathFilName("database", ACESERV_PIDFILE_NAME, "wrm", "r", 0)))
    {
      messerror("Server found pid file \"%s.%s\" during initialisation which may mean "
		"the server crashed badly last time it was run.",
		ACESERV_PIDFILE_NAME, ACESERV_PIDFILE_EXT) ;
    }

  /* write a new pid file if required (provided database is not readonly). */
  if (!server->readonly_DB)
    {
      if (checkForServerConfigValue(server, ACESERV_PIDFILE, NULL))
	{
	  pid_t mypid = 0 ;

	  mypid = getpid();

	  if (pid_path)					    /* Delete old pid file. */
	    unlink(pid_path) ;
	  else
	    pid_path = dbPathMakeFilName("database", ACESERV_PIDFILE_NAME, "wrm", 0) ;

	  pid_file = aceOutCreateToFile(pid_path, "w", 0) ;
	  aceOutPrint(pid_file, "%d", (int)mypid) ;
	  aceOutDestroy(pid_file) ;

	  server->pidfile = pid_path ;
	}
    }

  return ;
}

static void clearServerPidFile(AceServ server)
{
  if (server->pidfile)
    {
      unlink(server->pidfile) ;
      messfree(server->pidfile) ;
      server->pidfile = NULL ;
    }

  return ;
}


/************************************************************/

/* The following two message routines will be called by messout/messerror    */
/* to output messages instead of executing their default code (output to     */
/* stdout/stderr). This allows us to always log messages to server.log,      */
/* but only output messages to stdout/stderr if we are running in the        */
/* foreground (i.e. not inetd started).                                      */
/*                                                                           */

static void sessionMessOutCB(char *message, void *data_ptr)
{
  /* messouts go to stdout.                                                  */
  serverMessages(message, (AceServ)data_ptr, stdout) ;

  return ;
}

static void sessionMessErrCB(char *message, void *data_ptr)
{
  /* messouts go to stderr.                                                  */
  serverMessages(message, (AceServ)data_ptr, stderr) ;

  return ;
}

static void serverMessages(char *message, AceServ server, FILE *output_dest)
{
  /* Always write to server log file in database directory.                  */
  appendServerLog(server, message) ;

  /* Only write to stdout/stderr if we are running in foreground and user    */
  /* has not requested silent operation.                                     */
  if (server->inetd_started == FALSE && !(server->silent))
    {
      /* This is just what messerror currently does in fact except we flush  */
      /* so that its possible to see server messages as they occur, helps    */
      /* with debugging.                                                     */
      fprintf(output_dest, "// %s\n", message) ;
      fflush(output_dest) ;
    }

  return ;
}


/************************************************************/

/* This routine is called from the internal session.c routines that get      */
/* called if there is a messExit or messcrash. We need to be able to write   */
/* a lock file when this happens so that the checkServerFiles() can detect   */
/* if the database is restarting after a crash.                              */
/*                                                                           */
/* Shouldn't call messExit or messcrash from here because they will trap the */
/* recursion and will not have the desired effect.                           */
/*                                                                           */

static void sessionExitCB(void *data_ptr, char *exit_msg)
{
  AceServ server = (AceServ)data_ptr ;

  sessionServerClose(data_ptr) ;

  /* Write exit message and then close the log.                              */
  if (exit_msg)
    {
      server->logmsgs_per_timestamp = 1 ;		    /* Will force a timestamp. */
      appendServerLog(server, exit_msg) ;
    }

  closeServerLog(server, ACELOG_ABNORMAL_EXIT) ;

  return ;
}

static void sessionCrashCB(void *data_ptr, char *exit_msg)
{
  AceServ server = (AceServ)data_ptr ;

  sessionServerClose(data_ptr) ;

  /* Write exit message and then close the log.                              */
  server->logmsgs_per_timestamp = 1 ;			    /* Will force a timestamp. */
  appendServerLog(server, exit_msg) ;
  closeServerLog(server, ACELOG_CRASH_EXIT) ;

  return ;
}

static void sessionServerClose(void *data_ptr)
{
  AceServ server = (AceServ)data_ptr ;

  /* What we would like to happen here is that any existing clients will     */
  /* receive a termination message and their sockets be closed, the listeing */
  /* socket would also need to be cleared of any outstanding requests in the */
  /* same way as in serverCheckFiles...but in order to do this we need some  */
  /* data from the server...i.e. the list of clients, the listening socket   */
  /* and so on....we need context in other words...                          */
  /* this would ensure that for any messcrash/exit, all clients were         */
  /* cleared up properly...much better mechanism...                          */


  /* We should always clear the listening socket when we stop.               */
  if (server->inetd_started)
    {
      /* Clear any pending request on the listening socket.              */
      aceSocketInetdCleanup() ;
    }

  /* pidfile stuff. */
  clearServerPidFile(server) ;

  /* We only do any of this if there was no lock file found at start up,     */
  /* otherwise we are shutting down because of the lock file.                */
  if (server->crashfile_found_at_startup == FALSE
      && !server->readonly_DB)
    {

      /* Remove the undetectable crash lock file, the fact that we have arrived  */
      /* here means that we are shutting down in a detectable way so this lock   */
      /* file should be removed. Otherwise we will detect when we start up again */
      /* and this will prevent us from restarting.                               */
      removeUndetectableLockFile(server) ;


      /* Check for "no restart" option in serverconfig.wrm. If TRUE for database, then */
      /* write a lockfile for the crash, this will prevent acedb from restarting.*/
      /* NOTE, it may be dodgy to dynamically check for no restart because we    */
      /* may short on memory, if this proves a problem we could check for no     */
      /* restart at database startup and keep a static boolean which we could    */
      /* test here. This could also be set by a transaction by the admin user.   */
      if (checkForServerConfigValue(server, ACESERV_NORESTART, NULL))
	{
	  FILE *lockfile ;

	  /* Create a lock file to show that the server crashed/exitted.             */
	  lockfile = filopen(server->crashLockFile, NULL, "a") ;
	  if (!lockfile)
	    {
	      /* Bad news, try to log, may fail as acedb is already crashing.        */
	      char *inetd_msg = NULL, *mesg = NULL ; 
	  
	      if (server->inetd_started)
		inetd_msg = " " ACESERV_CRASH_INETD_MSG ;
	      else
		inetd_msg = "" ;

	      mesg = hprintf(0, "Warning: the server has crashed and it could not create a crash lock file in "
			     "the database directory, this may lead to database integrity being compromised "
			     "because there will nothing to stop the server being restarted on this database."
			     "%s",
			     inetd_msg) ;

	      appendServerLog(server, mesg) ;
	      
	      messfree(mesg) ;
	    }
	  else
	    {
	      /* Close just to make sure the file gets created.                     */
	      filclose(lockfile) ;
	    }
	}

    }


  return ;
}


/************************************************************/

/* Special server processing when receiving a SIGHUP, it starts a new log    */
/* file.                                                                     */
/*                                                                           */
static void sessionSigHup(void *data_ptr)
{
#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
  AceServ server = (AceServ)data_ptr ;			    /* currently unused. */
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

  logfile_request_new_G = TRUE ;

  /* Make sure that the very next logging request will go to a new log file. */
  sessionRequestNewLogFile() ;

  return ;
}


/* Special server processing when receiving a SIGTERM, we call a transport
 * layer routine to tell the transport layer to call us back next time round
 * its processing loop. Avoids getting blocked in the transport layer
 * waiting for a request but also avoids us calling signal unsafe routines. */
static void sessionSigTerm(void *data_ptr)
{
#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
  AceServ server = (AceServ)data_ptr ;			    /* currently unused. */
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

  aceTransportRequestShutdown() ;

  return ;
}


/************************************************************/
/* Database administrator can specify a working directory for the server in  */
/* the database.wrm file using the WORKING_DIRECTORY keyword, this routine   */
/* retrieves the directory name.                                             */
/*                                                                           */
static char *getWorkingDir(void)
{
  BOOL status = TRUE ;
  char *working_dir = NULL ;
  char *filename  = NULL ;
  ACEIN acefi = NULL ;

  /* Can we find the file ?                                                  */
  if (status == TRUE)
    {
      filename = dbPathStrictFilName("wspec", ACESERV_CONFIG_NAME, ACESERV_CONFIG_EXT, "r", 0) ;
      if (filename == NULL)
	{
	  messdump("Server cannot find the "
		   "\"wspec/"ACESERV_CONFIG_NAME"."ACESERV_CONFIG_EXT"\" configuration file, "
		   "the server will not run on this database until it can find the file.") ;
	  status = FALSE ;
	}
    }

  /* Can we open the file ?                                                  */
  if (status == TRUE)
    {
      acefi = aceInCreateFromFile(filename, "r", "", 0) ;
      if (acefi == NULL)
	{
	  messdump("Server cannot open the "
		   "\"wspec/"ACESERV_CONFIG_NAME"."ACESERV_CONFIG_EXT"\" configuration file, "
		   "the server will not run on this database until it can open this file.") ;
	  status = FALSE ;
	}
    }

  /* Look for the working directory.                                         */
  if (status == TRUE)
    {
      working_dir = NULL ;

      while (aceInCard(acefi) && working_dir == NULL)
	{
	  char *word ;
	  
	  if ((word = aceInWord(acefi)))
	    {
	      if (strcmp(word, ACESERV_WORKING_DIRECTORY) == 0)
		{
		  if ((word = aceInWord(acefi)))
		    working_dir = strnew(word, 0) ;
		}
	    }
	}
    }

  if (filename)
    messfree(filename) ;

  if (acefi)
    aceInDestroy(acefi) ;

  return working_dir ;
}


/***************************************************************************/
/* Stubs to make giface link. Once the display code has been enhanced to
   allow provide interactive and display-only sections of data displays
   this should go. */


void gexSetModalDialog(BOOL modal)
{
  return ;
}

int gexGetDefListSize(int curr_value)
{
  return 0 ;
}

BOOL gexCheckListSize(int curr_value)
{
  return FALSE ;
}

void gexSetMessListSize(int list_length)
{
  return ;
}

void gexSetMessPrefs(BOOL use_msg_list, int msg_list_length)
{
  return ;
}

void gexSetMessPopUps(BOOL msg_list, int list_length)
{
  return ;
}





/************************************************************/
/************************************************************/


