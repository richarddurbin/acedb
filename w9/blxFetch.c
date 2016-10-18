/*  File: blxFetch.c
 *  Author: Ed Griffiths (edgrif@sanger.ac.uk)
 *  Copyright (c) 2008: Genome Research Ltd.
 *-------------------------------------------------------------------
 * ZMap is free software; you can redistribute it and/or
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
 *-------------------------------------------------------------------
 * This file is part of the acedb genome database package
 * originally written by:
 *
 * 	Ed Griffiths (Sanger Institute, UK) edgrif@sanger.ac.uk,
 *      Roy Storey (Sanger Institute, UK) rds@sanger.ac.uk
 *
 * Description: Blixem functions for control of entry fetching and
 *              display.
 *              
 *              Compiling with -DACEDB includes acedb-specific options.
 *              
 *              Compiling with -DPFETCH_HTML includes code to issue
 *              pfetch requests to a proxy pfetch server using html.
 *              This requires w3rdparty libs libpfetch and libcurlobj
 *              which in turn require libcurl.
 *
 * Exported functions: blxPfetchEntry()
 *              
 * HISTORY:
 * Last edited: Aug 21 17:34 2009 (edgrif)
 * Created: Tue Jun 17 16:20:26 2008 (edgrif)
 * CVS info:   $Id: blxFetch.c,v 1.7 2010-01-19 17:02:53 gb10 Exp $
 *-------------------------------------------------------------------
 */

#include <sys/socket.h> /* for socket(), connect(), send(), and recv() */
#include <netinet/in.h>
#include <arpa/inet.h>  /* for sockaddr_in and inet_addr() */
#include <netdb.h>					    /* for gethostbyname() */
#ifdef PFETCH_HTML 
#include <wh/gex.h>
#include <libpfetch/libpfetch.h>
#endif
#include <w9/blixem_.h>


/* 
 * config file structs/keywords
 */

typedef enum {KEY_TYPE_INVALID, KEY_TYPE_BOOL, KEY_TYPE_INT, KEY_TYPE_DOUBLE, KEY_TYPE_STRING} ConfigKeyType ;

typedef struct
{
  char *name ;
  ConfigKeyType type ;
} ConfigKeyValueStruct, *ConfigKeyValue ;

typedef struct
{
  char *name ;
  ConfigKeyValue key_values ;
} ConfigGroupStruct, *ConfigGroup ;



/* Used to draw/update a progress meter, consider as private. */
typedef struct
{
  GtkWidget *top_level ;
  GtkWidget *progress ;
  GtkWidget *label ;
  GdkColor blue_bar_fg, red_bar_fg ;

  gulong widget_destroy_handler_id;

  gboolean cancelled ;
  int seq_total ;
} ProgressBarStruct, *ProgressBar ;



#ifdef PFETCH_HTML 
#define PFETCH_READ_SIZE 80	/* about a line */
#define PFETCH_FAILED_PREFIX "PFetch failed:"
#define PFETCH_TITLE_FORMAT "blixem - pfetch \"%s\""


typedef struct
{
  GtkWidget *dialog;
  GtkTextBuffer *text_buffer;
  char *title;

  gulong widget_destroy_handler_id;
  PFetchHandle pfetch;
  gboolean got_response;
} PFetchDataStruct, *PFetchData;


typedef struct
{
  gboolean finished ;
  gboolean cancelled ;
  gboolean status ;
  char *err_txt ;					    /* if !status then err message here. */

  gboolean stats ;					    /* TRUE means record and output stats. */
  int min_bytes, max_bytes, total_bytes, total_reads ;	    /* Stats. */

  int seq_total ;
  DICT *dict ;
  BlxSeqType seq_type ;
  Array seqs ;


  ProgressBar bar ;

  PFetchHandle pfetch ;
} PFetchSequenceStruct, *PFetchSequence ;


typedef struct
{
  char    *location;
  char    *cookie_jar;
  char    *mode;
  int      port;
} PFetchUserPrefsStruct ;
#endif



/* Test char to see if it's a iupac dna/peptide code. */
#define ISIUPACDNA(BASE) \
(((BASE) == 'a' || (BASE) == 'c'|| (BASE) == 'g' || (BASE) == 't'       \
  || (BASE) == 'u' || (BASE) == 'r'|| (BASE) == 'y' || (BASE) == 'm'    \
  || (BASE) == 'k' || (BASE) == 'w'|| (BASE) == 's' || (BASE) == 'b'    \
  || (BASE) == 'd' || (BASE) == 'h'|| (BASE) == 'v'                     \
  || (BASE) == 'n' || (BASE) == '.' || (BASE) == '-'))

#define ISIUPACPEPTIDE(PEPTIDE) \
(((PEPTIDE) == 'A' || (PEPTIDE) == 'B'|| (PEPTIDE) == 'C' || (PEPTIDE) == 'D'       \
  || (PEPTIDE) == 'E' || (PEPTIDE) == 'F'|| (PEPTIDE) == 'G' || (PEPTIDE) == 'H'    \
  || (PEPTIDE) == 'I' || (PEPTIDE) == 'K'|| (PEPTIDE) == 'L' || (PEPTIDE) == 'M'    \
  || (PEPTIDE) == 'N' || (PEPTIDE) == 'P'|| (PEPTIDE) == 'Q' || (PEPTIDE) == 'R'    \
  || (PEPTIDE) == 'S' || (PEPTIDE) == 'T'|| (PEPTIDE) == 'U' || (PEPTIDE) == 'V'    \
  || (PEPTIDE) == 'W' || (PEPTIDE) == 'X'|| (PEPTIDE) == 'Y' || (PEPTIDE) == 'Z'    \
  || (PEPTIDE) == '*' || (PEPTIDE) == '.' || (PEPTIDE) == '-'))


static void fetchBypfetch(void) ;
#ifdef PFETCH_HTML 
static void fetchBypfetchhtml(void) ;
#endif
static void fetchByefetch(void) ;
static void fetchByWWWefetch(void) ;
#ifdef ACEDB
static void fetchByacedb(void) ;
static void fetchByacedbtext(void) ;
#endif

#ifdef PFETCH_HTML 
static gboolean getPFetchUserPrefs(PFetchUserPrefsStruct *pfetch) ;
static PFetchStatus pfetch_reader_func(PFetchHandle *handle,
				       char         *text,
				       guint        *actual_read,
				       GError       *error,
				       gpointer      user_data) ;
static void handle_dialog_close(GtkWidget *dialog, gpointer user_data);
static PFetchStatus pfetch_closed_func(gpointer user_data) ;


static PFetchStatus sequence_pfetch_reader(PFetchHandle *handle,
				       char         *text,
				       guint        *actual_read,
				       GError       *error,
				    gpointer      user_data) ;
static PFetchStatus sequence_pfetch_closed(PFetchHandle *handle, gpointer user_data) ;
static void sequence_dialog_closed(GtkWidget *dialog, gpointer user_data) ;
static BOOL parsePfetchBuffer(char *read_text, int length, PFetchSequence fetch_data, int *num_seqs_inout) ;
#endif

static int socketConstruct(char *ipAddress, int port, BOOL External) ;
static BOOL socketSend(int sock, char *text) ;

static ProgressBar makeProgressBar(int seq_total) ;
static void updateProgressBar(ProgressBar bar, char *sequence, int seq_num, BOOL fetch_ok) ;
static gboolean isCancelledProgressBar(ProgressBar bar) ;
static void destroyProgressBar(ProgressBar bar) ;
static void destroyProgressCB(GtkWidget *widget, gpointer cb_data) ; /* internal to progress bar. */
static void cancelCB(GtkWidget *widget, gpointer cb_data) ; /* internal to progress bar. */

static void fillMSPwithSeqs(MSP *msplist, Array seqs, DICT *dict) ;
static void freeSeqs(Array seqs) ;

static BOOL readConfigFile(GKeyFile *key_file, char *config_file, GError **error) ;
ConfigGroup getConfig(char *config_name) ;
static BOOL loadConfig(GKeyFile *key_file, ConfigGroup group, GError **error) ;




/* Some local globals.... */
static char fetchMode[32] = BLX_FETCH_EFETCH ; /* Not done with enum to get menu-strings for free */
static char *URL = NULL ;


/* global configuration object for blixem. */
static GKeyFile *blx_config_G = NULL ;



/* Display the embl entry for a sequence via pfetch, efetch or whatever. */
void blxDisplayMSP(MSP *msp)
{

  if (!strcmp(fetchMode, BLX_FETCH_PFETCH))
    {
      /* --client gives logging information to pfetch server,
       * -F makes sure we get a full sequence entry returned. */
      externalCommand(messprintf("pfetch --client=acedb_%s_%s -F '%s' &",
				 getSystemName(), getLogin(TRUE), msp->sname));

      /* currently unused... pfetchWindow(msp); */
    }
#ifdef PFETCH_HTML 
  else if (!strcmp(fetchMode, BLX_FETCH_PFETCH_HTML))
    {
      blxPfetchEntry(msp->sname) ;
    }
#endif
  else if (!strcmp(fetchMode, BLX_FETCH_EFETCH))
    {
      externalCommand(messprintf("efetch '%s' &", msp->sname));
    }
  else if (!strcmp(fetchMode, BLX_FETCH_WWW_EFETCH))
    {
#ifdef ACEDB
      graphWebBrowser (messprintf ("%s%s", URL, msp->sname));
#else
      {
	char *browser = NULL ;

	if (!browser && !(browser = getenv("BLIXEM_WWW_BROWSER")))
	  {
	    printf("Looking for WWW browsers ...\n");
	    if (!findCommand("netscape", &browser) &&
		!findCommand("Netscape", &browser) &&
		!findCommand("Mosaic", &browser) &&
		!findCommand("mosaic", &browser) &&
		!findCommand("xmosaic", &browser))
	      {
		messout("Couldn't find any WWW browser.  Looked for "
			"netscape, Netscape, Mosaic, xmosaic & mosaic. "
			"System message: \"%s\"", browser);
		return;
	      }
	  }
	printf("Using WWW browser %s\n", browser);
	fflush(stdout);
	system(messprintf("%s %s%s&", browser, URL, msp->sname));
      }
#endif
    }
#ifdef ACEDB
  else if (!strcmp(fetchMode, BLX_FETCH_ACEDB))
    {
      display(msp->key, 0, 0);
    }
  else if (!strcmp(fetchMode, BLX_FETCH_ACEDB_TEXT))
    {
      display(msp->key, 0, "TREE");
    }
#endif
  else
    messout("Unknown fetchMode: %s", fetchMode);

  if (!URL)
    {
      URL = messalloc(256);
      strcpy(URL, "http://www.sanger.ac.uk/cgi-bin/seq-query?");
    }

  return ;
}


/* Needs a better name really, sets the fetch mode by looking at env. vars etc. */
char *blxFindFetchMode(void)
{
  char *tmp_mode ;

  /* Check env. vars to see how to fetch EMBL entries for sequences.         */
  if ((getenv("BLIXEM_FETCH_PFETCH")))
    {
      strcpy(fetchMode, BLX_FETCH_PFETCH);
    }
  else if ((URL = getenv("BLIXEM_FETCH_WWW")))
    {
      strcpy(fetchMode, BLX_FETCH_WWW_EFETCH);
    }
  else if (getenv("BLIXEM_FETCH_EFETCH"))
    {
      strcpy(fetchMode, BLX_FETCH_EFETCH);
    }
  else if ((tmp_mode = blxConfigGetFetchMode()))
    {
      strcpy(fetchMode, tmp_mode) ;
    }
  else
    {
#ifdef ACEDB
      strcpy(fetchMode, BLX_FETCH_ACEDB);
#else
      strcpy(fetchMode, BLX_FETCH_WWW_EFETCH);
#endif
    }

  return fetchMode ;
}


/* Set program to be used for fetching sequences depending on setting
 * of fetchmode: if fetchmode is pfetch we use pfetch, _otherwise_ efetch. */
char *blxGetFetchProg(void)
{
  char *fetch_prog = NULL ;

  if (!strcmp(fetchMode, BLX_FETCH_PFETCH))
    fetch_prog = "pfetch" ;
  else
    fetch_prog = "efetch" ;

  return fetch_prog ;
}


void blxSetFetchMode(char *fetch_mode)
{
  strcpy(fetchMode, fetch_mode) ;

  return ;
}


char *blxGetFetchMode(void)
{
  return fetchMode ;
}




MENUOPT *blxPfetchMenu(void)
{
  static MENUOPT fetchMenu[] =
    {
      {fetchBypfetch,     BLX_FETCH_PFETCH},
#ifdef PFETCH_HTML 
      {fetchBypfetchhtml, BLX_FETCH_PFETCH_HTML},
#endif
      {fetchByefetch,     BLX_FETCH_EFETCH},
      {fetchByWWWefetch,  BLX_FETCH_WWW_EFETCH},
#ifdef ACEDB
      {fetchByacedb,      BLX_FETCH_ACEDB},
      {fetchByacedbtext,  BLX_FETCH_ACEDB_TEXT},
#endif
      {0, 0}
    };

  return fetchMenu ;
}



#ifdef PFETCH_HTML 
/* Gets all the sequences needed by blixem but from http proxy server instead of from
 * the pfetch server direct, this enables blixem to be run and get sequences from
 * anywhere that can see the http proxy server. */
BOOL blxGetSseqsPfetchHtml(MSP *msplist, DICT *dict, BlxSeqType seqType)
{
  BOOL status = FALSE ;
  PFetchUserPrefsStruct prefs = {NULL} ;
  gboolean debug_pfetch = FALSE ;
  

  if (getPFetchUserPrefs(&prefs))
    {
      GType pfetch_type = PFETCH_TYPE_HTTP_HANDLE ;
      PFetchSequenceStruct fetch_data = {FALSE} ;
      GString *seq_string = NULL;
      int i ;


      fetch_data.seqs = arrayCreate(128, char*) ;
      fetch_data.dict = dict ;
      fetch_data.status = TRUE ;
      fetch_data.pfetch = PFetchHandleNew(pfetch_type);
      fetch_data.seq_total = dictMax(dict) ;
      fetch_data.bar = makeProgressBar(fetch_data.seq_total) ;
      fetch_data.stats = FALSE ;
      fetch_data.min_bytes = INT_MAX ;
      fetch_data.seq_type = seqType ;

      g_signal_connect(G_OBJECT(fetch_data.bar->top_level), "destroy",
		       G_CALLBACK(sequence_dialog_closed), &fetch_data) ;
      
      PFetchHandleSettings(fetch_data.pfetch, 
			   "port",       prefs.port,
			   "debug",      debug_pfetch,
			   "pfetch",     prefs.location,
			   "cookie-jar", prefs.cookie_jar,
			   "blixem-seqs",  TRUE, /* turns the next two on */
			   "one-per-line", TRUE, /* one sequence per line */
			   "case-by-type", TRUE, /* output case: dna PROTEIN */
			   NULL);

      g_free(prefs.location);
      g_free(prefs.cookie_jar);

      g_signal_connect(G_OBJECT(fetch_data.pfetch), "reader", G_CALLBACK(sequence_pfetch_reader), &fetch_data) ;

      g_signal_connect(G_OBJECT(fetch_data.pfetch), "closed", G_CALLBACK(sequence_pfetch_closed), &fetch_data) ;


      /* Build up a string containing all the sequence names. */
      seq_string = g_string_sized_new(1000) ;
      for (i = 0; i < fetch_data.seq_total; i++)
	{
	  g_string_append_printf(seq_string, "%s ", dictName(dict, i)) ;
	}

      /* Set up pfetch/curl connection routines, this is non-blocking so if connection
       * is successful we block using our own flag. */
      if (PFetchHandleFetch(fetch_data.pfetch, seq_string->str) == PFETCH_STATUS_OK)
	{
	  status = TRUE ;

	  while (!(fetch_data.finished))
	    gtk_main_iteration() ;

	  status = fetch_data.status ;
	  if (!status)
	    {
	      if (!fetch_data.cancelled)
		{
		  messerror("Sequence fetch from http server failed: %s", fetch_data.err_txt) ;
		  g_free(fetch_data.err_txt) ;
		}
	    }
	}
      else
	{
	  status = FALSE ;
	}

      g_string_free(seq_string, TRUE);

      destroyProgressBar(fetch_data.bar) ;

      /* If all good then fill in msp->sseq, otherwise clean up the allocated memory. */
      if (status)
	{
	  fillMSPwithSeqs(msplist, fetch_data.seqs, fetch_data.dict) ;
	}
      else
	{
	  freeSeqs(fetch_data.seqs) ;
	}

      arrayDestroy(fetch_data.seqs) ;
    }
  else
    {
      messerror("%s", "Failed to obtain preferences specifying how to pfetch.") ;
      status = FALSE ;
    }

  return status ;
}


void blxPfetchEntry(char *sequence_name)
{
  PFetchUserPrefsStruct prefs = {NULL} ;
  gboolean debug_pfetch = FALSE ;
  
  if ((getPFetchUserPrefs(&prefs)) && (prefs.location != NULL))
    {
      PFetchData pfetch_data ;
      PFetchHandle pfetch = NULL ;
      GType pfetch_type = PFETCH_TYPE_HTTP_HANDLE ;

      if (prefs.mode && g_ascii_strncasecmp(prefs.mode, "pipe", 4) == 0)
	pfetch_type = PFETCH_TYPE_PIPE_HANDLE ;
	
      pfetch_data = g_new0(PFetchDataStruct, 1);

      pfetch_data->pfetch = pfetch = PFetchHandleNew(pfetch_type);
      
      if ((pfetch_data->title = g_strdup_printf(PFETCH_TITLE_FORMAT, sequence_name)))
	{
	  GexEditor editor ;

	  editor = gexTextEditorNew(pfetch_data->title, "pfetching...\n", 0,
				    NULL, NULL, NULL,
				    FALSE, FALSE, TRUE) ;
	  
	  pfetch_data->dialog = editor->window ;
	  pfetch_data->text_buffer = editor->text ;

	  pfetch_data->widget_destroy_handler_id = 
	    g_signal_connect(G_OBJECT(pfetch_data->dialog), "destroy", 
			     G_CALLBACK(handle_dialog_close), pfetch_data); 
	}
      
      if (PFETCH_IS_HTTP_HANDLE(pfetch))
	PFetchHandleSettings(pfetch, 
			     "full",       TRUE,
			     "port",       prefs.port,
			     "debug",      debug_pfetch,
			     "pfetch",     prefs.location,
			     "cookie-jar", prefs.cookie_jar,
			     NULL);
      else
	PFetchHandleSettings(pfetch, 
			     "full",       TRUE,
			     "pfetch",     prefs.location,
			     NULL);
      
      g_free(prefs.location);
      g_free(prefs.cookie_jar);
      
      g_signal_connect(G_OBJECT(pfetch), "reader", G_CALLBACK(pfetch_reader_func), pfetch_data);
      
      g_signal_connect(G_OBJECT(pfetch), "closed", G_CALLBACK(pfetch_closed_func), pfetch_data);
      
      PFetchHandleFetch(pfetch, sequence_name) ;
    }
  else
    {
      messerror("%s", "Failed to obtain preferences specifying how to pfetch.") ;
    }

  return ;
}


#endif



/*  getsseqsPfetch() adapted from Tony Cox's code pfetch.c
 *
 *  - this version incorporates a progress monitor as a window,
 *    much easier for user to control + has a cancel button.
 */
BOOL blxGetSseqsPfetch(MSP *msplist, DICT *dict, char *pfetchIP, int port, BOOL External)
{
  BOOL status = TRUE ;
  enum {RCVBUFSIZE = 256} ;				    /* size of receive buffer */
  int sock ;
  STORE_HANDLE handle ;
  Array seq ;		/* of char - the current sequence being parsed */
  Array seqs ;		/* of char* - all sequences indexed by dict index */
  int seq_total ;					    /* Total number of sequences requested. */
  int i, len ;
  static char buffer[RCVBUFSIZE] ;

  handle = handleCreate () ;

  seq = arrayHandleCreate (1024, char, handle) ;
  seqs = arrayHandleCreate (128, char*, handle) ;
  seq_total = dictMax(dict) ;

  /* open socket connection */
  if (status)
    {
      sock = socketConstruct (pfetchIP, port, External) ;
      if (sock < 0)			/* we can't connect to the server */
	{
	  messfree (handle) ;
	  status = FALSE ;
	}
    }


  /* send the command/names to the server */
  if (status)
    {
      status = socketSend (sock, "-q -C") ;		    /* -q to get back one line per
							       sequence, -C for lowercase DNA and
							       uppercase protein. */
    }
  if (status)
    {
      for (i = 0 ; i < seq_total && status ; ++i)
	{
	  status = socketSend(sock, dictName(dict, i)) ;
	}
    }
  if (status)
    {
      /* send a final newline to flush the socket */
      if (send(sock, "\n", 1, 0) != 1)
	{
	  messerror("failed to send final \\n to socket") ;
	  status = FALSE ;
	}
    }


  /* get the sequences back */
  if (status)
    {
      BOOL finished ;
      int n, k, tot ;
      ProgressBar bar ;

      bar = makeProgressBar(seq_total) ;

      finished = FALSE ;
      n = 0 ;						    /* number of sequence */
      k = 0 ;						    /* position in sequence */
      tot = 0 ;						    /* total sequences fetched. */
      while (!finished && status)
	{
	  if (isCancelledProgressBar(bar))
	    {
	      status = FALSE ;
	      goto user_cancelled ;
	    }

	  len = recv(sock, buffer, RCVBUFSIZE, 0) ;
	  if (len == -1)
	    {
	      status = FALSE ;
	      messerror("Could not retrieve sequence data from pfetch server, "
			"error was: %s", messSysErrorText()) ;
	    }
	  else if (len == 0)
	    {
	      finished = TRUE ;
	    }
	  else
	    {
	      BOOL pfetch_ok ;

	      for (i = 0 ; i < len ; ++i)
		{
		  if (isCancelledProgressBar(bar))
		    {
		      status = FALSE ;
		      goto user_cancelled ;
		    }

		  if (buffer[i] == '\n')
		    {

		      array(seq, k, char) = 0 ;
		      if (strcmp(arrp(seq, 0, char), "no match"))
			{
			  /* NB not on handle - these must last after returning */
			  array(seqs, n++, char*) = strnew(arrp(seq, 0, char), 0) ;

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
			  printf("%s\n", arrp(seq, 0, char)) ;
			  printf("\nEND (%d)\n", strlen(arrp(seq, 0, char))) ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

			  tot++ ;
			  pfetch_ok = TRUE ;
			}
		      else
			{
			  array(seqs, n++, char*) = 0 ;

			  pfetch_ok = FALSE ;
			}
		      k = 0 ;

		      if (n > seq_total)
			{
			  status = FALSE ;
			  messerror("Unexpected data from pfetch server, "
				    "received %d lines when only %d sequences requested.",
				    n, dictMax(dict)) ;
			}
		      else
			{
			  updateProgressBar(bar, dictName(dict, (n - 1)), n, pfetch_ok) ;
			}
		    }
		  else
		    array(seq, k++, char) = buffer[i] ;
		}
	    }

	}


      /* User clicked the cancelled button in the dialog... */
    user_cancelled:
      
      shutdown(sock, SHUT_RDWR) ;

      destroyProgressBar(bar) ;
      bar = NULL ;

      if (status && tot != seq_total)
	{
	  double seq_prop ;

	  seq_prop = (float)tot / (float)seq_total ;

	  /* We don't display an error message unless lots of sequences don't get fetched
	   * because users find it annoying as most of the time they don't mind if the
	   * odd sequence isn't fetched successfully. */
	  if (seq_prop < 0.5)
	    messerror("pfetch sent back %d when %d requested", tot, seq_total) ;
	  else
	    messdump("pfetch sent back %d when %d requested", tot, seq_total) ;
	}
    }


  /* If all good then fill in msp->sseq, otherwise clean up the allocated memory. */
  if (status)
    {
      fillMSPwithSeqs(msplist, seqs, dict) ;
    }
  else
    {
      freeSeqs(seqs) ;
    }

  messfree(handle) ;

  return status ;
}



/* Set/Get global config, necessary because we don't have some blixem context pointer.... */
BOOL blxInitConfig(char *config_file, GError **error)
{
  BOOL result = FALSE ;

  messAssert(!blx_config_G) ;

  blx_config_G = g_key_file_new() ;

  if (!config_file)
    {
      result = TRUE ;
    }
  else
    {
      if (!(result = readConfigFile(blx_config_G, config_file, error)))
	{
	  g_key_file_free(blx_config_G) ;
	  blx_config_G = NULL ;
	}
    }

  return result ;
}



GKeyFile *blxGetConfig(void)
{
  return blx_config_G ;
}


BOOL blxConfigSetFetchMode(char *mode)
{
  BOOL result = FALSE ;
  GKeyFile *key_file ;

  key_file = blxGetConfig() ;
  messAssert(key_file) ;

  g_key_file_set_string(key_file, BLIXEM_GROUP, BLIXEM_DEFAULT_FETCH_MODE, mode) ;

  return result ;
}


char *blxConfigGetFetchMode(void)
{
  char *fetch_mode = NULL ;
  GKeyFile *key_file ;
  GError *error = NULL ;

  key_file = blxGetConfig() ;
  messAssert(key_file) ;

  fetch_mode = g_key_file_get_string(key_file, BLIXEM_GROUP, BLIXEM_DEFAULT_FETCH_MODE, &error) ;

  return fetch_mode ;
}



gboolean blxConfigGetPFetchSocketPrefs(char **node, int *port)
{
  gboolean result = FALSE ;
  GKeyFile *key_file ;
  GError *error = NULL ;

  if ((key_file = blxGetConfig()) && g_key_file_has_group(key_file, PFETCH_SOCKET_GROUP))
    {
      *node = g_key_file_get_string(key_file, PFETCH_SOCKET_GROUP, PFETCH_SOCKET_NODE, &error) ;
      *port = g_key_file_get_integer(key_file, PFETCH_SOCKET_GROUP, PFETCH_SOCKET_PORT, &error) ;

      /* By the time we get here _all_ the fields should have been filled in the config. */
      messAssert(!error) ;

      result = TRUE ;
    }

  return result ;
}

gboolean blxConfigSetPFetchSocketPrefs(char *node, int port)
{
  gboolean result = TRUE ;				    /* Can't fail. */
  GKeyFile *key_file ;

  key_file = blxGetConfig() ;
  messAssert(key_file) ;

  /* Note that these calls will create the group and key if they don't exist but will
   * overwrite any existing values. */
  g_key_file_set_string(key_file, PFETCH_SOCKET_GROUP, PFETCH_SOCKET_NODE, node) ;
  g_key_file_set_integer(key_file, PFETCH_SOCKET_GROUP, PFETCH_SOCKET_PORT, port) ;

  return result ;
}







/* 
 *                        Internal functions.
 */


static void fillMSPwithSeqs(MSP *msplist, Array seqs, DICT *dict)
{
  int i ;
  MSP *msp ;

  for (msp = msplist ; msp ; msp = msp->next)
    {
      if (!msp->sseq && msp->sname && *msp->sname && dictFind (dict, msp->sname, &i))
	{
	  if (array(seqs, i, char*))
	    {
	      blxSeq2MSP(msp, array(seqs, i, char*)) ;
	    }
	  else
	    {
	      blxAssignPadseq (msp) ;	/* use pads if you can't find it */
	    }
	}
    }

  return ;
}


static void freeSeqs(Array seqs)
{
  int i ;

  for (i = 0 ; i < arrayMax(seqs) ; i++)
    {
      if ((array(seqs, i, char*)))
	messfree(array(seqs, i, char*)) ;
    }

  return ;
}


static int socketConstruct (char *ipAddress, int port, BOOL External)
{
  int sock ;                       /* socket descriptor */
  struct sockaddr_in *servAddr ;   /* echo server address */
  struct hostent *hp ;


  /* Create a reliable, stream socket using TCP */
  if ((sock = socket(PF_INET, SOCK_STREAM, 0)) < 0)
    {
      messerror ("socket() failed") ;
      return -1 ;
    }


  /* Construct the server address structure */
  servAddr = (struct sockaddr_in *) messalloc (sizeof (struct sockaddr_in)) ;
  hp = gethostbyname(ipAddress) ;
  if (!hp)
  {
    if (External)
    {
        messerror("Failed to start external blixem: unknown host \"%s\"", ipAddress);
        return -1;
    }
    else
    {
        messerror("Failed to start internal blixem: unknown host \"%s\"", ipAddress);
        return -1;
    }
  }

  servAddr->sin_family = AF_INET ;			    /* Internet address family */
  bcopy((char*)hp->h_addr, (char*) &(servAddr->sin_addr.s_addr), hp->h_length) ;
							    /* Server IP address */
  servAddr->sin_port = htons ((unsigned short) port) ;	    /* Server port */


  /* Establish the connection to the server */
  if (connect(sock, (struct sockaddr *) servAddr, sizeof(struct sockaddr_in)) < 0)
    {
      messerror ("socket connect() to BLIXEM_PFETCH = %s failed", ipAddress) ;
      sock = -1 ;
    }

  messfree (servAddr) ;

  return sock ;
}

static BOOL socketSend (int sock, char *text)
{
  BOOL status = TRUE ;
  int len, bytes_to_send, bytes_written ;
  char *tmp ;
  struct sigaction oursigpipe, oldsigpipe ;

  /* The adding of 0x20 to the end looks wierd but I think it's because the  */
  /* server may not hold strings in the way C does (i.e. terminating '\0'),  */
  /* so 0x20 is added to mark the end of the string and the C string term-   */
  /* inator is moved up one and is not actually sent.                        */
  len = strlen(text) ;
  tmp = messalloc(len + 2) ;
  strcpy(tmp, text) ;
  tmp[len] = 0x20 ;					    /* Add new string terminator. */
  tmp[len + 1] = 0 ;
  bytes_to_send = len + 1 ;

  /* send() can deliver a SIGPIPE if the socket has been disconnected, by    */
  /* ignoring it we will receive -1 and can look for EPIPE as the errno.     */
  oursigpipe.sa_handler = SIG_IGN ;
  sigemptyset(&oursigpipe.sa_mask) ;
  oursigpipe.sa_flags = 0 ;
  if (sigaction(SIGPIPE, &oursigpipe, &oldsigpipe) < 0)
    messcrash("Cannot set SIG_IGN for SIGPIPE for socket write operations") ;

  bytes_written = send(sock, tmp, bytes_to_send, 0) ;
  if (bytes_written == -1)
    {
      if (errno == EPIPE || errno == ECONNRESET || errno == ENOTCONN)
	{
	  status = FALSE ;
	  messerror("Socket connection to pfetch server has failed, "
		    "error was: %s", messSysErrorText()) ;
	}
      else
	messcrash("Fatal error on socket connection to pfetch server, "
		  "error was: %s", messSysErrorText()) ;
    }
  else if (bytes_written != bytes_to_send)
    messcrash("send() call should have written %s bytes, but actually wrote %s.",
	      bytes_to_send, bytes_written) ;

  /* Reset the old signal handler.                                           */
  if (sigaction(SIGPIPE, &oldsigpipe, NULL) < 0)
    messcrash("Cannot reset previous signal handler for signal SIGPIPE for socket write operations") ;

  messfree(tmp) ;

  return status ;
}



/* Set of callbacks to set up different methods of fetching EMBL entry for a */
/* sequence.                                                                 */
/*                                                                           */
static void fetchBypfetch(void)
{
  strcpy(fetchMode, BLX_FETCH_PFETCH);

  blviewRedraw();

  return ;
}

#ifdef PFETCH_HTML 
static void fetchBypfetchhtml(void)
{
  strcpy(fetchMode, BLX_FETCH_PFETCH_HTML);

  blviewRedraw();

  return ;
}
#endif

static void fetchByefetch(void)
{
  strcpy(fetchMode, BLX_FETCH_EFETCH);

  blviewRedraw();

  return ;
}

static void fetchByWWWefetch(void)
{
  strcpy(fetchMode, BLX_FETCH_WWW_EFETCH);

  blviewRedraw();

  return ;
}


#ifdef ACEDB
static void fetchByacedb(void)
{
  strcpy(fetchMode, BLX_FETCH_ACEDB);

  blviewRedraw();

  return ;
}


static void fetchByacedbtext(void)
{
  strcpy(fetchMode, BLX_FETCH_ACEDB_TEXT);

  blviewRedraw();

  return ;
}
#endif



#ifdef PFETCH_HTML 


static gboolean getPFetchUserPrefs(PFetchUserPrefsStruct *pfetch)
{
  gboolean result = FALSE ;
  GKeyFile *key_file ;
  GError *error = NULL ;

  if ((key_file = blxGetConfig()) && g_key_file_has_group(key_file, PFETCH_PROXY_GROUP))
    {
      pfetch->location = g_key_file_get_string(key_file, PFETCH_PROXY_GROUP, PFETCH_PROXY_LOCATION, &error) ;
      pfetch->cookie_jar = g_key_file_get_string(key_file, PFETCH_PROXY_GROUP, PFETCH_PROXY_COOKIE_JAR, &error) ;
      pfetch->mode = g_key_file_get_string(key_file, PFETCH_PROXY_GROUP, PFETCH_PROXY_MODE, &error) ;
      pfetch->port = g_key_file_get_integer(key_file, PFETCH_PROXY_GROUP, PFETCH_PROXY_PORT, &error) ;

      /* By the time we get here _all_ the fields should have been filled in the config. */
      messAssert(!error) ;

      result = TRUE ;
    }

  return result ;
}



static PFetchStatus pfetch_reader_func(PFetchHandle *handle,
				       char         *text,
				       guint        *actual_read,
				       GError       *error,
				       gpointer      user_data)
{
  PFetchData pfetch_data = (PFetchData)user_data;
  PFetchStatus status    = PFETCH_STATUS_OK;

  if (actual_read && *actual_read > 0 && pfetch_data)
    {
      GtkTextBuffer *text_buffer = GTK_TEXT_BUFFER(pfetch_data->text_buffer);

      /* clear the buffer the first time... */
      if(pfetch_data->got_response == FALSE)
	gtk_text_buffer_set_text(text_buffer, "", 0);

      gtk_text_buffer_insert_at_cursor(text_buffer, text, *actual_read);

      pfetch_data->got_response = TRUE;
    }

  return status;
}



static PFetchStatus pfetch_closed_func(gpointer user_data)
{
  PFetchStatus status = PFETCH_STATUS_OK;

#ifdef DEBUG_ONLY
  printf("pfetch closed\n");
#endif	/* DEBUG_ONLY */

  return status;
}


/* GtkObject destroy signal handler */
static void handle_dialog_close(GtkWidget *dialog, gpointer user_data)
{
  PFetchData pfetch_data   = (PFetchData)user_data;
  pfetch_data->text_buffer = NULL;
  pfetch_data->widget_destroy_handler_id = 0; /* can we get this more than once? */

  if(pfetch_data->pfetch)
    pfetch_data->pfetch = PFetchHandleDestroy(pfetch_data->pfetch);

  return ;
}



/* 
 *             Multiple sequence fetch callbacks.
 */

static PFetchStatus sequence_pfetch_reader(PFetchHandle *handle,
					   char         *text,
					   guint        *actual_read,
					   GError       *error,
					   gpointer      user_data)
{
  PFetchStatus status = PFETCH_STATUS_OK ;
  PFetchSequence fetch_data = (PFetchSequence)user_data ;
  ProgressBar bar = fetch_data->bar ;

  if (!(*text) || *actual_read <= 0)
    {
      fetch_data->finished = TRUE ;
      fetch_data->status = FALSE ;
      fetch_data->err_txt = g_strdup("No data returned by pfetch http proxy server.") ;
    }
  else if (*actual_read > 0)
    {
      if (isCancelledProgressBar(bar))
	{
	  fetch_data->finished = TRUE ;
	  fetch_data->status = FALSE ;
	  fetch_data->cancelled = TRUE ;

	  status = PFETCH_STATUS_FAILED ;
	}
      else if (g_ascii_strncasecmp(text, "not authorised", (*actual_read < strlen("not authorised")
							    ? *actual_read : strlen("not authorised"))) == 0
	       || g_ascii_strncasecmp(text, "not authorized", (*actual_read < strlen("not authorised")
							       ? *actual_read : strlen("not authorised"))) == 0)
	{
	  fetch_data->finished = TRUE ;
	  fetch_data->status = FALSE ;
	  fetch_data->err_txt = g_strdup("Not authorised to access pfetch proxy server.") ;
	}
      else
	{
	  static int seq_num = 0 ;

	  if (!parsePfetchBuffer(text, *actual_read, fetch_data, &seq_num))
	    status = PFETCH_STATUS_FAILED ;
	}
    }

  return status ;
}

static PFetchStatus sequence_pfetch_closed(PFetchHandle *handle, gpointer user_data)
{
  PFetchStatus status = PFETCH_STATUS_OK;
  PFetchSequence fetch_data = (PFetchSequence)user_data ;


#ifdef DEBUG_ONLY
  printf("pfetch closed\n");
#endif	/* DEBUG_ONLY */


  fetch_data->finished = TRUE ;

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
  destroyProgressBar(fetch_data->bar) ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

  if (fetch_data->stats)
    {
      printf("Stats for %d reads:\tmin = %d\tmax = %d\tmean = %d\n",
	     fetch_data->total_reads,
	     fetch_data->min_bytes, fetch_data->max_bytes,
	     (fetch_data->total_bytes / fetch_data->total_reads)) ;
    }

  return status ;
}


/* Progress meter has been destroyed so free all pfetch stuff. */
static void sequence_dialog_closed(GtkWidget *dialog, gpointer user_data)
{
  PFetchSequence fetch_data = (PFetchSequence)user_data ;

  fetch_data->pfetch = PFetchHandleDestroy(fetch_data->pfetch) ;

  return ;
}



/* Parse the buffer sent back by proxy server. The pfetch server sends back data separated
 * by newlines. The data is either a valid IUPAC dna sequence or a valid IUPAC peptide
 * sequence or the text "no match". The problem here is that the data is not returned to
 * this function in complete lines so we have to reconstruct the lines as best we can. It's
 * even possible for very long sequences that they may span several buffers. Note also
 * that the buffer is _not_ null terminated, we have to use length to know when to stop reading.
 */
static BOOL parsePfetchBuffer(char *read_text, int length, PFetchSequence fetch_data, int *num_seqs_inout)
{
  BOOL status = TRUE ;
  static int seq_num = 0 ;
  static char *unfinished = NULL ;
  char *seq_start, *cp ;
  Array seqs = fetch_data->seqs ;
  int seqlen = 0 ;
  gboolean new_sequence ;

  if (fetch_data->stats)
    {
      if (length < fetch_data->min_bytes)
	fetch_data->min_bytes = length ;

      if (length > fetch_data->max_bytes)
	fetch_data->max_bytes = length ;

      fetch_data->total_bytes += length ;

      fetch_data->total_reads++ ;
  
      if (length < 100)
	{
	  char *str ;

	  str = g_strndup(read_text, length) ;
	  printf("Read %d: \"%s\"\n", fetch_data->total_reads, str) ;
	  g_free(str) ;
	}
    }


  /* Loop through each base in the sequence */
  new_sequence = TRUE ;
  seq_start = cp = read_text ;
  while (cp - read_text < length)
    {
      gboolean fetch_ok = TRUE ;

      /* Each time we start a new sequence it's either "no match\n" or IUPAC chars. */
      if (new_sequence && g_ascii_strncasecmp(cp, "no match", (length < strlen("not match")
							       ? length : strlen("no match"))) == 0)
	{
	  fetch_ok = FALSE ;

	  seq_num++ ;
	  cp += strlen("no match") + 1 ;

	  updateProgressBar(fetch_data->bar, dictName(fetch_data->dict, (seq_num - 1)),
			    seq_num, fetch_ok) ;
	}
      else
	{
	  /* Small optimisation: sequences will be all dna or all peptide so cut 
	   * down testing of char by checking the sequence type. */

	  gboolean isValidChar = FALSE;
	  if (fetch_data->seq_type == BLXSEQ_DNA)
	    {
	      /* Convert to correct case, then see if it's one of the valid letters for a base */
	      *cp = tolower(*cp);
	      isValidChar = ISIUPACDNA(*cp);
	    }
	  else if (fetch_data->seq_type == BLXSEQ_PEPTIDE)
	    {
	      /* Convert to correct case, then see if it's one of the valid letters for a peptide */
	      *cp = toupper(*cp);
	      isValidChar = ISIUPACPEPTIDE(*cp);
	    }
	   
	  if (isValidChar)
	    {
#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
	      printf("%c", *cp) ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

	      if (!seq_start)
		seq_start = cp ;

	      cp++ ;
	      seqlen++ ;
	    }
	  else if (*cp == '\n')
	    {
	      /* We've reached the end of a sequence so save it. */
	      int length ;
	      char *sequence = NULL ;


#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
	      printf("\nEND (%d)\n", seqlen) ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

	      length = cp - seq_start ;

	      if (!unfinished)
		{
		  /* No sequence left over from previous buffer so just copy what we have. */

		  sequence = g_strndup(seq_start, length) ;
		}
	      else
		{
		  /* There is unfinished sequence so concatenate. */
		  char *end ;

		  end = g_strndup(seq_start, length) ;

		  sequence = g_strconcat(unfinished, end, NULL) ;

		  g_free(end) ;
		  g_free(unfinished) ;
		  unfinished = NULL ;
		}

	      /* ghastly waste of time to recopy string but it must be inserted into
	       * acedb's alloc/free system or there will be trouble elsewhere.... */
	      array(seqs, seq_num++, char*) = strnew(sequence, 0) ;
	      g_free(sequence) ;

	      fetch_ok = TRUE ;

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
	      printf("seqlen = %d\n", strlen(array(seqs, *num_seqs_inout, char*))) ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

	      seq_start = NULL ;
	      seqlen = 0 ;

	      updateProgressBar(fetch_data->bar, dictName(fetch_data->dict, (seq_num - 1)),
				seq_num, fetch_ok) ;

	      cp++ ;
	    }
	  else
	    {
	      GString *err_msg ;

	      err_msg = g_string_sized_new(length * 2) ;
	      g_string_append_printf(err_msg, "Bad char in input stream: '%c', stream was: \"", *cp) ;
	      err_msg = g_string_append_len(err_msg, read_text, length) ;
	      g_string_append(err_msg, "\"") ;

	      fetch_data->finished = TRUE ;
	      fetch_data->status = FALSE ;
	      fetch_data->err_txt = g_string_free(err_msg, FALSE) ;

	      status = FALSE ;
	      goto abort ;
	    }

	}

    }


  /* OK, here we have got to the end of the data but it's not the end of a sequence so
   * we need to save the dangling sequence to be completed on the next read(s). */
  if (*(cp - 1) != '\n')
    {

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
      printf("\nNO END (%d), char = '%c'\n", seqlen, *(cp - 1)) ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

      /* If there's no unfinished sequence then just copy, otherwise concatnate with existing
       * unfinished sequence. */
      if (!unfinished)
	{
	  unfinished = g_strndup(seq_start, (cp - seq_start)) ;
	}
      else
	{
	  char *end ;
	  char *sequence ;

	  end = g_strndup(seq_start, (cp - seq_start)) ;

	  sequence = g_strconcat(unfinished, end, NULL) ;

	  g_free(end) ;
	  g_free(unfinished) ;

	  unfinished = sequence ;
	}
    }


  /* We jump here if we come across unexpected chars in the input buffer. */
 abort:

  return status ;
}



#endif /* PFETCH_HTML */



/* Functions to display, update, cancel and remove a progress meter. */

static ProgressBar makeProgressBar(int seq_total)
{
  ProgressBar bar = NULL ;
  GtkWidget *toplevel, *frame, *vbox, *progress, *label, *hbox, *cancel_button ;
  char *title ;

  bar = g_new0(ProgressBarStruct, 1) ;

  bar->seq_total = seq_total ;

  gdk_color_parse("blue", &(bar->blue_bar_fg)) ;
  gdk_color_parse("red", &(bar->red_bar_fg)) ;

  bar->top_level = toplevel = gtk_window_new(GTK_WINDOW_TOPLEVEL) ;
  title = g_strdup_printf("Blixem - pfetching %d sequences...", seq_total) ;
  gtk_window_set_title(GTK_WINDOW(toplevel), title) ;
  g_free(title) ;
  g_signal_connect(G_OBJECT(bar->top_level), "destroy", G_CALLBACK(destroyProgressCB), bar) ;


  gtk_window_set_default_size(GTK_WINDOW(toplevel), 350, -1) ;

  frame = gtk_frame_new(NULL) ;
  gtk_container_add(GTK_CONTAINER(toplevel), frame) ;

  vbox = gtk_vbox_new(FALSE, 5) ;
  gtk_container_add(GTK_CONTAINER(frame), vbox) ;

  bar->progress = progress = gtk_progress_bar_new() ; 

  gtk_widget_modify_fg(progress, GTK_STATE_NORMAL, &(bar->blue_bar_fg)) ;

  gtk_box_pack_start(GTK_BOX(vbox), progress, TRUE, TRUE, 0);

  bar->label = label = gtk_label_new("") ;
  gtk_box_pack_start(GTK_BOX(vbox), label, TRUE, TRUE, 0);

  hbox = gtk_hbox_new(FALSE, 0) ;
  gtk_box_pack_start(GTK_BOX(vbox), hbox, TRUE, TRUE, 0);
  gtk_container_border_width(GTK_CONTAINER(hbox), 5);

  cancel_button = gtk_button_new_with_label("Cancel") ;
  gtk_signal_connect(GTK_OBJECT(cancel_button), "clicked",
		     GTK_SIGNAL_FUNC(cancelCB), (gpointer)bar) ;
  gtk_box_pack_start(GTK_BOX(hbox), cancel_button, TRUE, TRUE, 0) ;

  gtk_widget_show_all(toplevel) ;
  while (gtk_events_pending())
    gtk_main_iteration() ;


  return bar ;
}

static void updateProgressBar(ProgressBar bar, char *sequence, int seq_num, BOOL fetch_ok)
{
  char *label_text ;

  if (fetch_ok)
    gtk_widget_modify_fg(bar->progress, GTK_STATE_NORMAL, &(bar->blue_bar_fg)) ;
  else
    gtk_widget_modify_fg(bar->progress, GTK_STATE_NORMAL, &(bar->red_bar_fg)) ;

  label_text = g_strdup_printf("%s - %s", sequence, fetch_ok ? "Fetched" : "Not found") ;
  gtk_label_set_text(GTK_LABEL(bar->label), label_text) ;
  g_free(label_text) ;
  
  gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(bar->progress),
				(double)((double)seq_num / (double)(bar->seq_total))) ;

  while (gtk_events_pending())
    gtk_main_iteration();

  return ;
}


static gboolean isCancelledProgressBar(ProgressBar bar)
{
  return bar->cancelled ;
}


static void destroyProgressBar(ProgressBar bar)
{
  gtk_widget_destroy(bar->top_level) ;

  return ;
}


/* called from progress meter when being destroyed, do not call directly. */
static void destroyProgressCB(GtkWidget *widget, gpointer cb_data)
{
  ProgressBar bar = (ProgressBar)cb_data ;

  g_free(bar) ;

  return ;
}


/* Called from progress meter. */
static void cancelCB(GtkWidget *widget, gpointer cb_data)
{
  ProgressBar bar = (ProgressBar)cb_data ;

  bar->cancelled = TRUE ;

  return ;
}





/* 
 *            Configuration file reading.
 */

BOOL readConfigFile(GKeyFile *key_file, char *config_file, GError **error)
{
  BOOL result = FALSE ;
  GKeyFileFlags flags = G_KEY_FILE_NONE ;
  
  if ((result = g_key_file_load_from_file(key_file, config_file, flags, error)))
    {
      ConfigGroup config ;
      char **groups, **group ;
      gsize num_groups ;
      int i ;
      gboolean config_loaded = FALSE ;

      groups = g_key_file_get_groups(key_file, &num_groups) ;

      for (i = 0, group = groups ; result && i < num_groups ; i++, group++)
	{
	  if ((config = getConfig(*group)))
	    {
	      config_loaded = TRUE ;
	      result = loadConfig(key_file, config, error) ;
	    }
	}

      if (!config_loaded)
	{
	  *error = g_error_new_literal(g_quark_from_string("BLIXEM_CONFIG"), 1,
				       "No groups found in config file.") ;
	  result = FALSE ;
	}
    }

  
  return result ;
}


/* Returns the config keyvalues pairs of the group config_name or NULL if that group cannot be found. */
ConfigGroup getConfig(char *config_name)
{
  ConfigGroup config = NULL ;
  static ConfigKeyValueStruct pfetch_http[] = {{PFETCH_PROXY_LOCATION, KEY_TYPE_STRING},
					       {PFETCH_PROXY_COOKIE_JAR, KEY_TYPE_STRING},
					       {PFETCH_PROXY_MODE, KEY_TYPE_STRING},
					       {PFETCH_PROXY_PORT, KEY_TYPE_INT},
					       {NULL, KEY_TYPE_INVALID}} ;
  static ConfigKeyValueStruct pfetch_socket[] = {{PFETCH_SOCKET_NODE, KEY_TYPE_STRING},
						 {PFETCH_SOCKET_PORT, KEY_TYPE_INT},
						 {NULL, KEY_TYPE_INVALID}} ;
  static ConfigGroupStruct groups[] = {{PFETCH_PROXY_GROUP, pfetch_http},
				       {PFETCH_SOCKET_GROUP, pfetch_socket},
				       {NULL, NULL}} ;
  ConfigGroup tmp ;

  tmp = &(groups[0]) ;
  while (tmp->name)
    {
      if (g_ascii_strcasecmp(config_name, tmp->name) == 0)
	{
	  config = tmp ;
	  break ;
	}

      tmp++ ;
    }

  return config ;
}


/* Tries to load values for all keyvalues given in ConfigGroup fails if any are missing. */
static BOOL loadConfig(GKeyFile *key_file, ConfigGroup group, GError **error)
{
  BOOL result = TRUE ;
  ConfigKeyValue key_value ;

  key_value = group->key_values ;
  while (key_value->name && result)
    {
      switch(key_value->type)
	{
	case KEY_TYPE_BOOL:
	  {
	    gboolean tmp ;

	    if (!(tmp = g_key_file_get_boolean(key_file, group->name, key_value->name, error)))
	      result = FALSE ;

	    break ;
	  }
	case KEY_TYPE_INT:
	  {
	    int tmp ;

	    if (!(tmp = g_key_file_get_integer(key_file, group->name, key_value->name, error)))
	      result = FALSE ;

	    break ;
	  }
	case KEY_TYPE_DOUBLE:
	  {
#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
	    /* We can do this by steam using our own funcs in fact.... */

	    double tmp ;

	    /* Needs 2.12.... */

	    if (!(tmp = g_key_file_get_double(key_file, group->name, key_value->name, &error)))
	      result = FALSE ;

#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

	    break ;
	  }
	case KEY_TYPE_STRING:
	  {
	    char *tmp ;

	    if (!(tmp = g_key_file_get_string(key_file, group->name, key_value->name, error)))
	      result = FALSE ;

	    break ;
	  }
	default:
	  messAssertNotReached("Bad Copnfig key type") ;
	  break ;
	}

      key_value++ ;
    }


  return result ;
}

