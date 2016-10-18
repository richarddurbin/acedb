/*  File: libpfetch.c
 *  Author: Roy Storey (rds@sanger.ac.uk)
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
 * This file is part of the ZMap genome database package
 * originally written by:
 *
 * 	Ed Griffiths (Sanger Institute, UK) edgrif@sanger.ac.uk,
 *      Roy Storey (Sanger Institute, UK) rds@sanger.ac.uk
 *
 * Description: 
 *
 * Exported functions: See XXXXXXXXXXXXX.h
 * HISTORY:
 * Last edited: Jun 18 13:16 2009 (edgrif)
 * Created: Fri Apr  4 14:21:42 2008 (rds)
 * CVS info:   $Id: libpfetch.c,v 1.3 2009-06-19 08:06:21 edgrif Exp $
 *-------------------------------------------------------------------
 */

#define LIBPFETCH_READY 1
#ifdef LIBPFETCH_READY


#include <string.h>
#include <signal.h>		/* kill(), SIGKILL */
#include <errno.h>
#include <sys/wait.h>		/* WIFSIGNALED() */
#define __USE_XOPEN_EXTENDED
#include <unistd.h>		/* getpgid(), setpgid()... needs __USE_XOPEN_EXTENDED and after something above... */
#include <libpfetch/libpfetch_P.h>
#include <libpfetch/libpfetch_I.h>
#include <libpfetch/libpfetch-cmarshal.h>

static void pfetch_handle_class_init(PFetchHandleClass pfetch_class);
static void pfetch_handle_init(PFetchHandle pfetch_handle);
static void pfetch_handle_set_property(GObject *gobject, 
				       guint param_id, 
				       const GValue *value, 
				       GParamSpec *pspec);
static void pfetch_handle_get_property(GObject *gobject, 
				       guint param_id, 
				       GValue *value, 
				       GParamSpec *pspec);
static void pfetch_handle_dispose (GObject *object);
static void pfetch_handle_finalize (GObject *object);


static GObjectClass *base_parent_class_G = NULL;

/* Public functions. */

GType PFetchHandleGetType(void)
{
  static GType pfetch_type = 0;

  if(!pfetch_type)
    {
      GTypeInfo pfetch_info = 
	{
	  sizeof(pfetchHandleClass),
	  (GBaseInitFunc)     NULL,
	  (GBaseFinalizeFunc) NULL,
	  (GClassInitFunc) pfetch_handle_class_init,
	  NULL,			/* class_finalize */
	  NULL,			/* class_data */
	  sizeof(pfetchHandle),
	  0,			/* n_preallocs */
	  (GInstanceInitFunc) pfetch_handle_init
	};

      pfetch_type = g_type_register_static(G_TYPE_OBJECT,
					   "PFetchBaseHandle", 
					   &pfetch_info, 0);
    }

  return pfetch_type;
}


PFetchHandle PFetchHandleNew(GType type)
{
  PFetchHandle pfetch = NULL;

  pfetch = g_object_new(type, NULL);

  return pfetch;
}




PFetchStatus PFetchHandleSettings(PFetchHandle pfetch, const gchar *first_arg_name, ...)
{
  PFetchStatus status = PFETCH_STATUS_OK;
  va_list args;
  
  va_start(args, first_arg_name);
  g_object_set_valist(G_OBJECT(pfetch), first_arg_name, args);
  va_end(args);
  
  return status;
}

PFetchStatus PFetchHandleFetch(PFetchHandle pfetch, char *sequence)
{
  PFetchStatus status = PFETCH_STATUS_OK;

  if(pfetch == NULL)
    status   = PFETCH_STATUS_FAILED;

  if(status == PFETCH_STATUS_OK && !PFETCH_IS_HANDLE(pfetch))
    status   = PFETCH_STATUS_FAILED;

  if(status == PFETCH_STATUS_OK && !PFETCH_HANDLE_GET_CLASS(pfetch)->fetch)
    {
      g_warning("Handle with type '%s' does not implement fetch method.\n",
		g_type_name(G_OBJECT_TYPE(pfetch)));

      status = PFETCH_STATUS_FAILED;
    }

  if(status == PFETCH_STATUS_OK)
    status   = (* PFETCH_HANDLE_GET_CLASS(pfetch)->fetch)(pfetch, sequence);

  return status;
}

PFetchStatus PFetchHandleFetchMultiple(PFetchHandle pfetch, char **sequences, int count)
{
  PFetchStatus status = PFETCH_STATUS_OK;
  GString *seq_string = NULL;
  char **seq_ptr = sequences;

  if(seq_ptr && count && *seq_ptr)
    {
      int i;

      seq_string = g_string_sized_new(128);

      for(i = 0; i < count; i++, seq_ptr++)
	{
	  g_string_append_printf(seq_string, "%s ", *seq_ptr);
	}

      status = PFetchHandleFetch(pfetch, seq_string->str);

      g_string_free(seq_string, TRUE);
    }
  

  return status;
}

PFetchHandle PFetchHandleDestroy(PFetchHandle pfetch)
{
  g_object_unref(G_OBJECT(pfetch));

  return NULL;
}

/* INTERNAL Base class functions */

static void pfetch_handle_class_init(PFetchHandleClass pfetch_class)
{
  GObjectClass      *gobject_class;
  PFetchHandleClass  handle_class;
  guint n_params = 3;

  gobject_class = (GObjectClass *)pfetch_class;
  handle_class  = (PFetchHandleClass)pfetch_class;

  base_parent_class_G = g_type_class_peek_parent(pfetch_class);

  gobject_class->set_property = pfetch_handle_set_property;
  gobject_class->get_property = pfetch_handle_get_property;

  g_object_class_install_property(gobject_class,
				  PFETCH_LOCATION, 
				  g_param_spec_string("pfetch", "pfetch", 
						      "where to find pfetch",
						      "", PFETCH_PARAM_STATIC_RW));
  g_object_class_install_property(gobject_class,
				  PFETCH_FULL, 
				  g_param_spec_boolean("full", "-F option", 
						       "Return full database entry",
						       FALSE, PFETCH_PARAM_STATIC_RW));

  g_object_class_install_property(gobject_class,
				  PFETCH_ARCHIVE, 
				  g_param_spec_boolean("archive", "-A option", 
						       "Search EMBL and UniProtKB archive, including current data",
						       FALSE, PFETCH_PARAM_STATIC_RW));

  g_object_class_install_property(gobject_class,
				  PFETCH_UNIPROT_ISOFORM_SEQ, 
				  g_param_spec_boolean("isoform-seq", "isoform-seq", 
						       "Return the fasta entry for an isofrom as the -F entries don't exist.",
						       FALSE, PFETCH_PARAM_STATIC_RW));

  g_object_class_install_property(gobject_class,
				  PFETCH_BLIXEM_STYLE, 
				  g_param_spec_boolean("blixem-seqs", "fetch like blixem wants", 
						       "Return the sequences one per line with lower case dna and upper case AMINOACIDS",
						       FALSE, PFETCH_PARAM_STATIC_RW));

  g_object_class_install_property(gobject_class,
				  PFETCH_ONE_SEQ_PER_LINE, 
				  g_param_spec_boolean("one-per-line", "-q option", 
						       "Return the sequences one per line.",
						       FALSE, PFETCH_PARAM_STATIC_RW));

  g_object_class_install_property(gobject_class,
				  PFETCH_DNA_LOWER_AA_UPPER, 
				  g_param_spec_boolean("case-by-type", "-C option", 
						       "Return the fasta entry/sequences with lower case dna and upper case AMINOACIDS",
						       FALSE, PFETCH_PARAM_STATIC_RW));

  g_object_class_install_property(gobject_class,
				  PFETCH_DEBUG, 
				  g_param_spec_boolean("debug", "debug", 
						       "turn debugging on/off",
						       FALSE, PFETCH_PARAM_STATIC_RW));

  /* Signals */
  handle_class->handle_signals[HANDLE_READER_SIGNAL] =
    g_signal_new("reader", 	/* signal_name */
		 G_TYPE_FROM_CLASS(pfetch_class), /* signal owner type */
		 G_SIGNAL_RUN_LAST, /* signal flags */
		 G_STRUCT_OFFSET(pfetchHandleClass, reader), /* class closure */
		 NULL, 		/* accumulator */
		 NULL, 		/* accumulator data */
		 libpfetch_cmarshal_ENUM__STRING_POINTER_POINTER, /* marshal */
		 PFETCH_TYPE_HANDLE_STATUS,	/* return type */
		 n_params,	/* number of params */
		 G_TYPE_STRING,
		 G_TYPE_POINTER,
		 G_TYPE_POINTER); /* param types */

  handle_class->handle_signals[HANDLE_ERROR_SIGNAL] =
    g_signal_new("error", 
		 G_TYPE_FROM_CLASS(pfetch_class),
		 G_SIGNAL_RUN_LAST,
		 G_STRUCT_OFFSET(pfetchHandleClass, error), /* class closure */
		 NULL, 		/* accumulator */
		 NULL, 		/* accumulator data */
		 libpfetch_cmarshal_ENUM__STRING_POINTER_POINTER, /* marshal */
		 PFETCH_TYPE_HANDLE_STATUS,	/* return type */
		 n_params,	/* number of params */
		 G_TYPE_STRING,
		 G_TYPE_POINTER,
		 G_TYPE_POINTER); /* param types */


  handle_class->handle_signals[HANDLE_WRITER_SIGNAL] =
    g_signal_new("writer", 
		 G_TYPE_FROM_CLASS(pfetch_class),
		 G_SIGNAL_RUN_LAST,
		 G_STRUCT_OFFSET(pfetchHandleClass, writer), /* class closure */
		 NULL, 		/* accumulator */
		 NULL, 		/* accumulator data */
		 libpfetch_cmarshal_ENUM__STRING_POINTER_POINTER, /* marshal */
		 PFETCH_TYPE_HANDLE_STATUS,	/* return type */
		 n_params,	/* number of params */
		 G_TYPE_STRING,
		 G_TYPE_POINTER,
		 G_TYPE_POINTER); /* param types */
		 

  handle_class->handle_signals[HANDLE_CLOSED_SIGNAL] =
    g_signal_new("closed", 
		 G_TYPE_FROM_CLASS(pfetch_class),
		 G_SIGNAL_RUN_LAST,
		 G_STRUCT_OFFSET(pfetchHandleClass, closed), /* class closure */
		 NULL, 		/* accumulator */
		 NULL, 		/* accumulator data */
		 libpfetch_cmarshal_ENUM__VOID, /* marshal */
		 PFETCH_TYPE_HANDLE_STATUS,	/* return type */
		 0,	/* number of params */
		 G_TYPE_STRING,
		 G_TYPE_POINTER,
		 G_TYPE_POINTER); /* param types */
		 
  gobject_class->dispose  = pfetch_handle_dispose;
  gobject_class->finalize = pfetch_handle_finalize;

  return ;
}

static void pfetch_handle_init(PFetchHandle pfetch_handle)
{
  /* Nothing to do */

  return ;
}

static void pfetch_handle_set_property(GObject *gobject, guint param_id, const GValue *value, GParamSpec *pspec)
{
  PFetchHandle pfetch_handle;

  g_return_if_fail(PFETCH_IS_HANDLE(gobject));

  pfetch_handle = PFETCH_HANDLE(gobject);

  switch(param_id)
    {
    case PFETCH_FULL:
      pfetch_handle->opts.full = g_value_get_boolean(value);
      break;
    case PFETCH_ARCHIVE:
      pfetch_handle->opts.archive = g_value_get_boolean(value);
      break;
    case PFETCH_DEBUG:
      pfetch_handle->opts.debug = g_value_get_boolean(value);
      break;
    case PFETCH_LOCATION:
      if(pfetch_handle->location)
	g_free(pfetch_handle->location);

      pfetch_handle->location = g_value_dup_string(value);

      break;
    case PFETCH_UNIPROT_ISOFORM_SEQ:
      pfetch_handle->opts.isoform_seq = g_value_get_boolean(value);
      break;
    case PFETCH_BLIXEM_STYLE:
      /* turn full off! */
      pfetch_handle->opts.full = !(g_value_get_boolean(value));
      pfetch_handle->opts.one_per_line =
	pfetch_handle->opts.dna_PROTEIN =
	g_value_get_boolean(value);
      break;
    case PFETCH_ONE_SEQ_PER_LINE:
      pfetch_handle->opts.one_per_line = g_value_get_boolean(value);
      if(pfetch_handle->opts.full)
	pfetch_handle->opts.full = FALSE;
      break;
    case PFETCH_DNA_LOWER_AA_UPPER:
      pfetch_handle->opts.dna_PROTEIN = g_value_get_boolean(value);
      if(pfetch_handle->opts.full)
	pfetch_handle->opts.full = FALSE;
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID(gobject, param_id, pspec);
      break;
    }

  return ;
}

static void pfetch_handle_get_property(GObject *gobject, guint param_id, GValue *value, GParamSpec *pspec)
{
  PFetchHandle pfetch_handle;

  g_return_if_fail(PFETCH_IS_HANDLE(gobject));

  pfetch_handle = PFETCH_HANDLE(gobject);

  switch(param_id)
    {
    case PFETCH_FULL:
      g_value_set_boolean(value, pfetch_handle->opts.full);
      break;
    case PFETCH_ARCHIVE:
      g_value_set_boolean(value, pfetch_handle->opts.archive);
      break;
    case PFETCH_DEBUG:
      g_value_set_boolean(value, pfetch_handle->opts.debug);
      break;
    case PFETCH_LOCATION:
      g_value_set_string(value, pfetch_handle->location);
      break;
    case PFETCH_UNIPROT_ISOFORM_SEQ:
      g_value_set_boolean(value, pfetch_handle->opts.isoform_seq);
      break;
    case PFETCH_BLIXEM_STYLE:
      g_value_set_boolean(value, (pfetch_handle->opts.dna_PROTEIN && pfetch_handle->opts.one_per_line));
      break;
    case PFETCH_ONE_SEQ_PER_LINE:
      g_value_set_boolean(value, pfetch_handle->opts.one_per_line);
      break;
    case PFETCH_DNA_LOWER_AA_UPPER:
      g_value_set_boolean(value, pfetch_handle->opts.dna_PROTEIN);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID(gobject, param_id, pspec);
      break;
    }

  return ;
}

static void pfetch_handle_dispose (GObject *object)
{
  /* no other references exist. */
  if(base_parent_class_G->dispose)
    (*base_parent_class_G->dispose) (object);

  return ;
}

static void pfetch_handle_finalize (GObject *object)
{
  /* just finalize parent */
  if(base_parent_class_G->finalize)
    (*base_parent_class_G->finalize) (object);

  return;
}

static void pfetch_get_argv(PFetchHandle handle, char *seq, char **argv)
{
  int current = 0;
  gboolean add_F = FALSE;
  argv[current++] = handle->location;

  add_F = handle->opts.full;

  if(handle->opts.isoform_seq)
    {
      if(g_strstr_len(seq, -1, "-"))
	add_F = FALSE;
    }

  if(add_F)
    argv[current++] = "-F";

  if(handle->opts.one_per_line)
    argv[current++] = "-q";

  if(handle->opts.dna_PROTEIN)
    argv[current++] = "-C";

  if(handle->opts.archive)
    argv[current++] = "-A";

  if(seq != NULL)
    argv[current++] = seq;

  argv[current] = '\0';
  
  return ;
}

#ifdef DEBUG_DONT_INCLUDE
static PFetchStatus test_reader(PFetchHandle handle, char *output, guint *size, GError *error)
{
  char text[PFETCH_READ_SIZE + 1] = {0} ;
  guint internal = 0;

  if(size)
    {
      if(*size > PFETCH_READ_SIZE)
	internal = PFETCH_READ_SIZE;
      else
	internal = *size;
    }

  snprintf(&text[0], internal, "%s", output);

  printf("%s", &text[0]);

  return PFETCH_STATUS_OK;
}
#endif /* DEBUG_DONT_INCLUDE */


/* ---------------------- *
 * END of Base class code * 
 * ---------------------- */




/* ---------------- *
 * Pipe Handle Code *
 * ---------------- */



static void pfetch_pipe_handle_class_init(PFetchHandlePipeClass pfetch_class);
static void pfetch_pipe_handle_init(PFetchHandlePipe pfetch);
static void pfetch_pipe_handle_dispose(GObject *gobject);
static void pfetch_pipe_handle_finalize(GObject *gobject);

static PFetchStatus pfetch_pipe_fetch(PFetchHandle handle, char *sequence);

static gboolean pipe_stdout_func(GIOChannel *source, GIOCondition condition, gpointer user_data);
static gboolean pipe_stderr_func(GIOChannel *source, GIOCondition condition, gpointer user_data);
static gboolean pipe_stdin_func (GIOChannel *source, GIOCondition condition, gpointer user_data);
static gboolean pfetch_spawn_async_with_pipes(char *argv[], 
					      GIOFunc stdin_writer, 
					      GIOFunc stdout_reader, 
					      GIOFunc stderr_reader, 
					      gpointer pipe_data, 
					      GDestroyNotify pipe_data_destroy, 
					      GError **error,
					      GChildWatchFunc child_func, 
					      ChildWatchData child_func_data,
					      guint *stdin_source_id, 
					      guint *stdout_source_id, 
					      guint *stderr_source_id);
static gboolean fd_to_GIOChannel_with_watch(gint fd, GIOCondition cond, GIOFunc func, 
					    gpointer data, GDestroyNotify destroy,
					    guint *source_id_out);
static void pfetch_child_watch_func(GPid pid, gint status, gpointer user_data);
static gboolean timeout_pfetch_process(gpointer user_data);
static void source_remove_and_zero(guint *source_id);
static void detach_and_kill(PFetchHandlePipe pipe, ChildWatchData child_data);
static void detach_group_for_later_kill(gpointer unused);

/* Public type function */

/*!
 * Get the GType of the object.
 *
 * @param   none
 * @return  GType of the PFetchHandlePipe *object
 */
GType PFetchHandlePipeGetType(void)
{
  static GType pfetch_type = 0;

  if(!pfetch_type)
    {
      GTypeInfo pfetch_info = 
	{
	  sizeof (pfetchHandlePipeClass),
	  (GBaseInitFunc)     NULL,
	  (GBaseFinalizeFunc) NULL,
	  (GClassInitFunc) pfetch_pipe_handle_class_init,
	  NULL,			/* class_finalize */
	  NULL,			/* class_data */
	  sizeof(pfetchHandlePipe),
	  0,			/* n_preallocs */
	  (GInstanceInitFunc) pfetch_pipe_handle_init
	};

      pfetch_type = g_type_register_static(PFETCH_TYPE_HANDLE,
					   "PFetchHandlePipe", 
					   &pfetch_info, 0);
    }

  return pfetch_type;
}

static void pfetch_pipe_handle_class_init(PFetchHandlePipeClass pfetch_class)
{
  GObjectClass      *gobject_class;
  PFetchHandleClass handle_class;

  gobject_class = (GObjectClass *)pfetch_class;
  handle_class  = (PFetchHandleClass )pfetch_class;

  gobject_class->set_property = pfetch_handle_set_property;

  handle_class->fetch = pfetch_pipe_fetch;

#ifdef DEBUG_DONT_INCLUDE
  handle_class->reader = test_reader;
#endif /* DEBUG_DONT_INCLUDE */

  /* dispose & finalize? */
  gobject_class->dispose  = pfetch_pipe_handle_dispose;
  gobject_class->finalize = pfetch_pipe_handle_finalize;

  return ;
}

static void pfetch_pipe_handle_init(PFetchHandlePipe pfetch)
{
  /* Nothing to do... */

  return ;
}


static PFetchStatus pfetch_pipe_fetch(PFetchHandle handle, char *sequence)
{
  PFetchHandlePipe pipe = PFETCH_PIPE_HANDLE(handle);
  ChildWatchData child_data = NULL;
  PFetchStatus status = PFETCH_STATUS_OK;
  GError *error = NULL;
  gulong reader_id, writer_id, error_id;
  char *argv[256];
  gboolean result;
  
  child_data = &(pipe->watch_data);

  child_data->watch_data = pipe;

  pfetch_get_argv(handle, sequence, &argv[0]);

  reader_id = g_signal_handler_find(G_OBJECT(handle),
				    G_SIGNAL_MATCH_ID,
				    PFETCH_HANDLE_GET_CLASS(handle)->handle_signals[HANDLE_READER_SIGNAL],
				    0,
				    NULL, NULL, NULL);

  error_id  = g_signal_handler_find(G_OBJECT(handle),
				    G_SIGNAL_MATCH_ID,
				    PFETCH_HANDLE_GET_CLASS(handle)->handle_signals[HANDLE_ERROR_SIGNAL],
				    0,
				    NULL, NULL, NULL);

  writer_id = g_signal_handler_find(G_OBJECT(handle),
				    G_SIGNAL_MATCH_ID,
				    PFETCH_HANDLE_GET_CLASS(handle)->handle_signals[HANDLE_WRITER_SIGNAL],
				    0,
				    NULL, NULL, NULL);


  result = pfetch_spawn_async_with_pipes(argv, 
					 (writer_id != 0 ? pipe_stdin_func  : NULL), 
					 (reader_id != 0 ? pipe_stdout_func : NULL),
					 (error_id  != 0 ? pipe_stderr_func : NULL),
					 (gpointer)pipe,
					 NULL, &error, 
					 pfetch_child_watch_func, child_data,
					 &(pipe->stdin_source_id),
					 &(pipe->stdout_source_id),
					 &(pipe->stderr_source_id));

  if(result)
    pipe->timeout_source_id = g_timeout_add(30 * 1000, timeout_pfetch_process, pipe);
  else
    status = PFETCH_STATUS_FAILED;

  return status;
}

/* GIOFunc - return TRUE to get called again */
static gboolean pipe_stdout_func(GIOChannel *source, GIOCondition condition, gpointer user_data)
{
  PFetchHandleClass handle_class = PFETCH_HANDLE_GET_CLASS(user_data);
  PFetchStatus signal_return;
  GQuark detail = 0;
  gboolean call_again = FALSE;

  if(condition & G_IO_IN)
    {
      GIOStatus io_status;
      gchar output[PFETCH_READ_SIZE] = {0};
      gsize actual_read = 0;
      GError *error = NULL;
      
      if((io_status = g_io_channel_read_chars(source, &output[0], PFETCH_READ_SIZE, 
					      &actual_read, &error)) == G_IO_STATUS_NORMAL)
	{
	  guint actual_read_uint = (guint)actual_read;
	  signal_return = emit_signal(PFETCH_HANDLE(user_data),
				      handle_class->handle_signals[HANDLE_READER_SIGNAL], 
				      detail, &output[0], &actual_read_uint, error);

	  if(signal_return == PFETCH_STATUS_OK)
	    call_again = TRUE;
	}
      else
	{
	  guint actual_read_uint = 0;
	  switch(error->code)
	    {
	      /* Derived from errno */
	    case G_IO_CHANNEL_ERROR_FBIG:
	    case G_IO_CHANNEL_ERROR_INVAL:
	    case G_IO_CHANNEL_ERROR_IO:
	    case G_IO_CHANNEL_ERROR_ISDIR:
	    case G_IO_CHANNEL_ERROR_NOSPC:
	    case G_IO_CHANNEL_ERROR_NXIO:
	    case G_IO_CHANNEL_ERROR_OVERFLOW:
	    case G_IO_CHANNEL_ERROR_PIPE:
	      break;
	      /* Other */
	    case G_IO_CHANNEL_ERROR_FAILED:
	    default:
	      break;
	    }

	  output[0]   = '\0';
	  signal_return = emit_signal(PFETCH_HANDLE(user_data),
				      handle_class->handle_signals[HANDLE_READER_SIGNAL], 
				      detail, &output[0], &actual_read_uint, error);
	  
	  call_again = FALSE;
	}
    }
  else if(condition & G_IO_HUP)
    {
      
    }
  else if(condition & G_IO_ERR)
    {

    }
  else if(condition & G_IO_NVAL)
    {

    }


  return call_again;
}

/* GIOFunc - return TRUE to get called again */
static gboolean pipe_stderr_func(GIOChannel *source, GIOCondition condition, gpointer user_data)
{
  PFetchHandleClass handle_class = PFETCH_HANDLE_GET_CLASS(user_data);
  GQuark detail = 0;
  gboolean call_again = FALSE, signal_return = FALSE;

  if(condition & G_IO_IN)
    {
      GIOStatus status;
      gchar text[PFETCH_READ_SIZE] = {0};
      gsize actual_read = 0;
      GError *error = NULL;

      if((status = g_io_channel_read_chars(source, &text[0], PFETCH_READ_SIZE, 
					   &actual_read, &error)) == G_IO_STATUS_NORMAL)
	{
	  guint actual_read_uint = (guint)actual_read;
	  signal_return = emit_signal(PFETCH_HANDLE(user_data),
				      handle_class->handle_signals[HANDLE_ERROR_SIGNAL], 
				      detail, &text[0], &actual_read_uint, error);
	}
    }

  call_again = signal_return;

  return call_again;
}

/* GIOFunc - return TRUE to get called again */
static gboolean pipe_stdin_func(GIOChannel *source, GIOCondition condition, gpointer user_data)
{
  PFetchHandlePipe pipe = PFETCH_PIPE_HANDLE(user_data);
#ifdef PFETCH_SUPPORTS_INTERACTIVE
  PFetchHandleClass handle_class = PFETCH_HANDLE_GET_CLASS(pipe);
  GQuark detail = 0;
#endif /* PFETCH_SUPPORTS_INTERACTIVE */
  gboolean call_again = FALSE, signal_return = FALSE;

  if(pipe->watch_data.watch_id && condition & G_IO_OUT)
    {
#ifdef PFETCH_SUPPORTS_INTERACTIVE
      char text[PFETCH_READ_SIZE] = {0};
      guint actual = 0;
      GError *error = NULL;
      signal_return = emit_signal(PFETCH_HANDLE(user_data),
				  handle_class->handle_signals[HANDLE_WRITER_SIGNAL],
				  detail, &text[0], &actual, error);
      if(signal_return == PFETCH_STATUS_OK)
	{
	  GError *write_error = NULL;
	  gssize actual_size  = strlen(&text[0]);
	  gsize bytes_written = 0;

	  g_io_channel_write_chars(source, &text[0], actual_size, 
				   &bytes_written, &write_error);
	  g_io_channel_flush(source, &error);
	}
#endif /* PFETCH_SUPPORTS_INTERACTIVE */

      g_warning("Sorry, the interactive version of pfetch is no longer supported. %s", 
		"Your connected signal will not be called.");
    }

  call_again = signal_return;

  return call_again;
}

/*!
 * Wrapper around g_spawn_async_with_pipes() to aid with the logic
 * contained in making that call. In the process of making the call
 * the function converts requested File descriptors to non-blocking
 * GIOChannels adding the corresponding GIOFunc to the channel with
 * g_io_add_watch.
 *
 * Passing NULL for any of the GIOFunc parameters means the rules
 * g_spawn_async_with_pipes() uses apply as NULL will be passed in
 * place there.
 *
 * N.B. passing a GChildWatchFunc is untested... I waited for 
 * G_IO_HUP instead.
 *
 * @param **argv        Argument vector
 * @param stdin_writer  GIOFunc to handle writing to the pipe.
 * @param stdout_reader GIOFunc to handle reading from the pipe.
 * @param stderr_reader GIOFunc to handle reading from the pipe.
 * @param pipe_data     user data to pass to the GIOFunc
 * @param pipe_data_destroy GDestroyNotify for pipe_data
 * @param error         GError location to return into 
 *                       (from g_spawn_async_with_pipes)
 * @param child_func    A GChildWatchFunc for the spawned PID 
 * @param child_func_data data for the child_func
 * @return              gboolean from g_spawn_async_with_pipes()
 *
 *  */

static gboolean pfetch_spawn_async_with_pipes(char *argv[], GIOFunc stdin_writer, GIOFunc stdout_reader, GIOFunc stderr_reader, 
					      gpointer pipe_data, GDestroyNotify pipe_data_destroy, GError **error,
					      GChildWatchFunc child_func, ChildWatchData child_func_data,
					      guint *stdin_source_id, guint *stdout_source_id, guint *stderr_source_id)
{
  char *cwd = NULL, **envp = NULL;
  GSpawnChildSetupFunc child_setup = detach_group_for_later_kill;
  gpointer child_data = child_func_data;
  GPid child_pid;
  gint *stdin_ptr = NULL,
    *stdout_ptr = NULL,
    *stderr_ptr = NULL,
    stdin_fd,
    stdout_fd, 
    stderr_fd;
  GSpawnFlags flags = G_SPAWN_SEARCH_PATH;
  gboolean result = FALSE;
  GError *our_error = NULL;	/* In case someone decides to live on the edge! */

  if(stdin_writer)
    stdin_ptr = &stdin_fd;
  if(stdout_reader)
    stdout_ptr = &stdout_fd;
  if(stderr_reader)
    stderr_ptr = &stderr_fd;

  if(child_func)
    flags |= G_SPAWN_DO_NOT_REAP_CHILD;
  else
    child_setup = NULL;

  if((result = g_spawn_async_with_pipes(cwd, argv, envp,
					flags, child_setup, child_data, 
					&child_pid, stdin_ptr,
					stdout_ptr, stderr_ptr, 
					&our_error)))
    {
      GIOCondition read_cond  = G_IO_IN  | G_IO_HUP | G_IO_ERR | G_IO_NVAL;
      GIOCondition write_cond = G_IO_OUT | G_IO_HUP | G_IO_ERR | G_IO_NVAL;

      if(child_func && child_func_data && (child_func_data->watch_pid = child_pid))
	child_func_data->watch_id = g_child_watch_add(child_pid, child_func, child_func_data);

      /* looks good so far... Set up the readers and writer... */
      if(stderr_ptr && !fd_to_GIOChannel_with_watch(*stderr_ptr, read_cond, stderr_reader, pipe_data, pipe_data_destroy, stderr_source_id))
	g_warning("%s", "Failed to add watch to stderr");
      if(stdout_ptr && !fd_to_GIOChannel_with_watch(*stdout_ptr, read_cond, stdout_reader, pipe_data, pipe_data_destroy, stdout_source_id))
	g_warning("%s", "Failed to add watch to stdout");
      if(stdin_ptr && !fd_to_GIOChannel_with_watch(*stdin_ptr, write_cond, stdin_writer, pipe_data, pipe_data_destroy, stdin_source_id))
	g_warning("%s", "Failed to add watch to stdin");
    }
  else if(!error)      /* We'll Log the error */
    g_warning("%s", our_error->message);

  if(error)
    *error = our_error;

  return result;
}

static gboolean fd_to_GIOChannel_with_watch(gint fd, GIOCondition cond, GIOFunc func, 
					    gpointer data, GDestroyNotify destroy,
					    guint *source_id_out)
{
  gboolean success = FALSE;
  GIOChannel *io_channel;

  if((io_channel = g_io_channel_unix_new(fd)))
    {
      GError *flags_error = NULL;
      GIOStatus status;

      if((status = g_io_channel_set_flags(io_channel, 
					  (G_IO_FLAG_NONBLOCK | g_io_channel_get_flags(io_channel)), 
					  &flags_error)) == G_IO_STATUS_NORMAL)
	{
	  guint source_id;

	  //g_io_channel_set_encoding(io_channel, "ISO8859-1", NULL);

	  source_id = g_io_add_watch_full(io_channel, G_PRIORITY_DEFAULT, cond, func, data, destroy);

	  if(source_id_out)
	    *source_id_out = source_id;

	  success = TRUE;
	}
      else
	g_warning("%s", flags_error->message);

      g_io_channel_unref(io_channel); /* We don't need this anymore */
    }

  return success;
}

static void pfetch_child_watch_func(GPid pid, gint status, gpointer user_data)
{
  ChildWatchData watch_data = (ChildWatchData)user_data;

  if(!WIFSIGNALED(status))
    {
      if(watch_data->watch_pid == 0)
	{
	  g_warning("%s", "pfetch pid == 0");
	}
      else if(watch_data->watch_pid != pid)
	g_warning("pfetch process pid '%d' does not match '%d'", pid, watch_data->watch_pid);
      else if(watch_data->watch_data)
	{
	  PFetchHandleClass handle_class = PFETCH_HANDLE_GET_CLASS(watch_data->watch_data);
	  PFetchStatus signal_return = PFETCH_STATUS_OK;
	  GQuark detail = 0;

	  watch_data->watch_pid = 0;

	  signal_return = emit_signal(PFETCH_HANDLE(watch_data->watch_data),
				      handle_class->handle_signals[HANDLE_CLOSED_SIGNAL], 
				      detail, NULL, NULL, NULL);
	}
      else
	{
	  /* This is actually the default... */
	  source_remove_and_zero(&(watch_data->watch_id));

	  watch_data->watch_pid = 0;
	}
    }
  else
    g_warning("pfetch process [pid=%d] terminated (kill -%d).", pid, WTERMSIG(status));

  /* always do this. */
  g_spawn_close_pid(pid);

  return ;
}

static void source_remove_and_zero(guint *source_id)
{
  if(source_id && *source_id)
    {
      gboolean source_removed;

      source_removed = g_source_remove(*source_id);
      
      *source_id = 0;
    }

  return ;
}

static void detach_and_kill(PFetchHandlePipe pipe, ChildWatchData child_data)
{
  GPid pid;

  source_remove_and_zero(&(pipe->stdin_source_id));

  source_remove_and_zero(&(pipe->stdout_source_id));

  source_remove_and_zero(&(pipe->stderr_source_id));

  source_remove_and_zero(&(pipe->timeout_source_id));

  if((pid = child_data->watch_pid))
    {
      pid_t gid;
      child_data->watch_pid = 0;

      errno = 0;
      gid   = getpgid(pid);

      if(errno == ESRCH)
	{
	  g_warning("pfetch process [pid=%d] not found!", pid);
	  errno = 0;
	}
      else if(gid > 1 && pid == gid)
	{
	  g_warning("pfetch process [pid=%d, gid=%d] being killed", pid, gid);
	  kill(-gid, SIGKILL);
	}
      else
	{
	  g_warning("pfetch process [pid=%d, gid=%d] not killed as pid != gid", pid, gid);
	}
    }

  return ;
}

static gboolean timeout_pfetch_process(gpointer user_data)
{
  if(PFETCH_IS_PIPE_HANDLE(user_data))
    {
      PFetchHandlePipe pipe = NULL;
      PFetchHandleClass handle_class = NULL;
      ChildWatchData child_data = NULL;
      GQuark detail = 0;
      gchar *text = NULL;
      guint actual_read;
      GError *error = NULL;

      pipe = PFETCH_PIPE_HANDLE(user_data);

      child_data   = &(pipe->watch_data);

      if(child_data->watch_pid)
	{
	  handle_class = PFETCH_HANDLE_GET_CLASS(pipe);

	  if((text = g_strdup_printf("Process '%s' [pid=%d] timed out. Will be killed.",
				     PFETCH_HANDLE(pipe)->location, child_data->watch_pid)))
	    {
	      emit_signal(PFETCH_HANDLE(user_data),
			  handle_class->handle_signals[HANDLE_ERROR_SIGNAL], 
			  detail, text, &actual_read, error);
	      
	      g_free(text);
	    }

	  /* we ned to do this before detach and kill I think... */
	  /* returning FALSE ensures it's removed. having a
	   * g_source_remove() go on, might be bad. */
	  pipe->timeout_source_id = 0;
	  
	  detach_and_kill(pipe, child_data);
	}
    }

  /* Always remove it. */
  return FALSE;
}

static void pfetch_pipe_handle_dispose(GObject *gobject)
{
  PFetchHandlePipe pipe = PFETCH_PIPE_HANDLE(gobject);
  ChildWatchData child_data = NULL;

  child_data = &(pipe->watch_data);

  detach_and_kill(pipe, child_data);

  return ;
}



static void pfetch_pipe_handle_finalize(GObject *gobject)
{
  /* nothing to do here. */
  return ;
}

/* WARNING: This is _only_ executed in the child process! In order to
 * debug this with totalview you _must_ use the following in LDFLAGS
 * -L/usr/totalview/linux-x86/lib -ldbfork */
static void detach_group_for_later_kill(gpointer unused)
{
#ifdef WHY_IS_THIS_HERE
  ChildWatchData child_data = (ChildWatchData)unused;
  PFetchHandlePipe handle;
#endif /* WHY_IS_THIS_HERE */
  int setgrp_rv = 0;

  errno = 0;
  if((setgrp_rv = setpgid(0,0)) != 0)
    {
      /* You'll almost certainly _not_ see the result of these prints */
      switch(errno)
	{
	case EACCES:
	  printf("errno = EACCES\n");
	  break;
	case EINVAL:
	  printf("errno = EINVAL\n");
	  break;
	case EPERM:
	  printf("errno = EPERM\n");
	  break;
	case ESRCH:
	  printf("errno = ESRCH\n");
	  break;
	case 0:
	default:
	  /* should not be reached according to man setpgrp() */
	  break;
	}
      errno = 0;
    }
#ifdef WHY_IS_THIS_HERE
  else
    {
      PFetchHandleClass handle_class;
      GQuark detail = 0;
      gchar output[PFETCH_READ_SIZE] = {"libpfetch.c code failed"};
      guint actual_read_uint = PFETCH_READ_SIZE;
      GError *error = NULL;

      handle = PFETCH_PIPE_HANDLE(child_data->watch_data);

      handle_class = PFETCH_HANDLE_GET_CLASS(handle);

      emit_signal(PFETCH_HANDLE(handle),
		  handle_class->handle_signals[HANDLE_READER_SIGNAL], 
		  detail, &output[0], &actual_read_uint, error);

    }
#endif /* WHY_IS_THIS_HERE */

  return ;
}



/* ---------------- *
 * HTTP Handle Code *
 * ---------------- */

static void pfetch_http_handle_class_init(PFetchHandleHttpClass pfetch_class);
static void pfetch_http_handle_init(PFetchHandleHttp pfetch);
static void pfetch_http_handle_dispose (GObject *gobject);
static void pfetch_http_handle_finalize(GObject *gobject);
static void pfetch_http_handle_set_property(GObject      *gobject, 
					    guint         param_id, 
					    const GValue *value, 
					    GParamSpec   *pspec);
static void pfetch_http_handle_get_property(GObject    *gobject, 
					    guint       param_id, 
					    GValue     *value, 
					    GParamSpec *pspec);

static PFetchStatus pfetch_http_fetch(PFetchHandle handle, char *sequence);

static size_t http_curl_write_func (void *ptr, size_t size, size_t nmemb, void *stream);
static size_t http_curl_header_func(void *ptr, size_t size, size_t nmemb, void *stream);

static char *build_post_data(PFetchHandleHttp pfetch, char *sequence);
static void conn_close_handler(CURLObject curl_object, PFetchHandleHttp pfetch);

/* Public type function */

/*!
 * Get the GType of the object.
 *
 * @param   none
 * @return  GType of the PFetchHandleHttp *object
 */

GType PFetchHandleHttpGetType(void)
{
  static GType pfetch_type = 0;

  if(!pfetch_type)
    {
      GTypeInfo pfetch_info = 
	{
	  sizeof(pfetchHandleHttpClass),
	  (GBaseInitFunc)     NULL,
	  (GBaseFinalizeFunc) NULL,
	  (GClassInitFunc) pfetch_http_handle_class_init,
	  NULL,			/* class_finalize */
	  NULL,			/* class_data */
	  sizeof(pfetchHandleHttp),
	  0,			/* n_preallocs */
	  (GInstanceInitFunc) pfetch_http_handle_init
	};

      pfetch_type = g_type_register_static(PFETCH_TYPE_HANDLE,
					   "PFetchHttpHandle", 
					   &pfetch_info, 0);
    }

  return pfetch_type;
}

#ifdef DEBUG_DONT_INCLUDE
#ifdef NO_POINT_WITHOUT_HTTP1_1
static PFetchStatus test_writer(PFetchHandle handle, 
				char *output, 
				guint *size, 
				GError *error)
{
  char *sequence = "BF591426.1";
  guint internal = 0;
  static gboolean done = FALSE;

  if(size && !done)
    {
      internal = strlen(sequence) + 2;
      done = TRUE;
      if(*size > internal)
	*size = internal;
      snprintf(&output[0], *size, "%s ", sequence);
    }
  else
    *size = 0;

  return PFETCH_STATUS_OK;
}
#endif /* NO_POINT_WITHOUT_HTTP1_1 */
#endif /* DEBUG_DONT_INCLUDE */

static void pfetch_http_handle_class_init(PFetchHandleHttpClass pfetch_class)
{
  GObjectClass      *gobject_class;
  PFetchHandleClass  handle_class;

  gobject_class = (GObjectClass *)pfetch_class;
  handle_class  = (PFetchHandleClass )pfetch_class;

  handle_class->fetch = pfetch_http_fetch;
  
  gobject_class->set_property = pfetch_http_handle_set_property;
  gobject_class->get_property = pfetch_http_handle_get_property;
#ifdef WRONG
  g_object_class_install_property(gobject_class,
				  PFETCH_LOCATION,
				  g_param_spec_string("url", "url",
						      "pfetch url",
						      "", PFETCH_PARAM_STATIC_RW));
#endif	/* WRONG */
  g_object_class_install_property(gobject_class,
				  PFETCH_PORT,
				  g_param_spec_uint("port", "port",
						    "pfetch port", 
						    80, 65535,
						    80, PFETCH_PARAM_STATIC_RW));

  g_object_class_install_property(gobject_class,
				  PFETCH_COOKIE_JAR,
				  g_param_spec_string("cookie-jar", "cookie-jar",
						      "netscape cookie jar",
						      "", PFETCH_PARAM_STATIC_RW));


#ifdef DEBUG_DONT_INCLUDE
  handle_class->reader = test_reader;
  handle_class->writer = test_writer;
#endif

  gobject_class->dispose  = pfetch_http_handle_dispose;
  gobject_class->finalize = pfetch_http_handle_finalize;


  return ;
}

static void pfetch_http_handle_init(PFetchHandleHttp pfetch)
{
  pfetch->curl_object = CURLObjectNew();

  g_signal_connect(G_OBJECT(pfetch->curl_object), "connection_closed",
		   (GCallback)conn_close_handler, pfetch);

  return ;
}

static void pfetch_http_handle_dispose(GObject *gobject)
{
  PFetchHandleHttp pfetch = PFETCH_HTTP_HANDLE(gobject);

  if(pfetch->curl_object)
    pfetch->curl_object = CURLObjectDestroy(pfetch->curl_object);

  return ;
}

static void pfetch_http_handle_finalize(GObject *gobject)
{
  PFetchHandleHttp pfetch = PFETCH_HTTP_HANDLE(gobject);

  if(pfetch->cookie_jar_location)
    g_free(pfetch->cookie_jar_location);

  pfetch->cookie_jar_location = NULL;

  return ;
}

static void pfetch_http_handle_set_property(GObject *gobject, guint param_id, 
					    const GValue *value, GParamSpec *pspec)
{
  PFetchHandleHttp pfetch = PFETCH_HTTP_HANDLE(gobject);

  switch(param_id)
    {
    case PFETCH_PORT:
      pfetch->http_port = g_value_get_uint(value);
      break;
    case PFETCH_COOKIE_JAR:
      if(pfetch->cookie_jar_location)
	g_free(pfetch->cookie_jar_location);
      
      pfetch->cookie_jar_location = g_value_dup_string(value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID(gobject, param_id, pspec);
      break;
    }

  return ;
}

static void pfetch_http_handle_get_property(GObject *gobject, guint param_id, 
					    GValue *value, GParamSpec *pspec)
{
  PFetchHandleHttp pfetch = PFETCH_HTTP_HANDLE(gobject);

  switch(param_id)
    {
    case PFETCH_PORT:
      g_value_set_uint(value, pfetch->http_port);
      break;
    case PFETCH_COOKIE_JAR:
      g_value_set_string(value, pfetch->cookie_jar_location);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID(gobject, param_id, pspec);
      break;
    }
  
  return ;
}

static PFetchStatus pfetch_http_fetch(PFetchHandle handle, char *sequence)
{
  PFetchHandleHttp pfetch = PFETCH_HTTP_HANDLE(handle);
  PFetchStatus     status = PFETCH_STATUS_OK;

  if((pfetch->post_data = build_post_data(pfetch, sequence)))
    {
      CURLObjectSet(pfetch->curl_object,
		    /* general settings */
		    "debug", PFETCH_HANDLE(pfetch)->opts.debug,
		    "post",  TRUE,
		    "url",   PFETCH_HANDLE(pfetch)->location,
		    "port",  pfetch->http_port,
		    /* request */
		    "postfields",  pfetch->post_data,   
		    "cookiefile",  pfetch->cookie_jar_location,
		    /* functions */
		    "writefunction",  http_curl_write_func,
		    "writedata",      pfetch,
		    "headerfunction", http_curl_header_func,
		    "headerdata",     pfetch,
		    /* end of options */
		    NULL);
      
      pfetch->request_counter++;
      if(CURLObjectPerform(pfetch->curl_object, TRUE) == CURL_STATUS_FAILED)
	{
	  PFetchHandleClass handle_class = PFETCH_HANDLE_GET_CLASS(pfetch);
	  GError *error = NULL;
	  char *curl_object_error = NULL;
	  unsigned int error_size = 0;

	  /* first get message */
	  CURLObjectErrorMessage(pfetch->curl_object, &curl_object_error);
	  error_size = strlen(curl_object_error);

	  /* signal error handler... */
	  emit_signal(PFETCH_HANDLE(pfetch), handle_class->handle_signals[HANDLE_ERROR_SIGNAL],
		      0, curl_object_error, &error_size, error);
	  
	  if(curl_object_error)	/* needs freeing */
	    g_free(curl_object_error);

	  /* set our return status */
	  status = PFETCH_STATUS_FAILED;
	}

      /* #define TESTING_MULTIPLE_FETCHING */
#ifdef TESTING_MULTIPLE_FETCHING
      CURLObjectSet(pfetch->curl_object,
		    /* general settings */
		    "debug", PFETCH_HANDLE(pfetch)->opts.debug,
		    "post",  TRUE,
		    "url",   PFETCH_HANDLE(pfetch)->location,
		    "port",  pfetch->http_port,
		    /* request */
		    "postfields",  pfetch->post_data,   
		    "cookiefile",  pfetch->cookie_jar_location,
		    /* functions */
		    "writefunction",  http_curl_write_func,
		    "writedata",      pfetch,
		    "headerfunction", http_curl_header_func,
		    "headerdata",     pfetch,
		    /* end of options */
		    NULL);
      
      pfetch->request_counter++;
      CURLObjectPerform(pfetch->curl_object, TRUE);
#endif

    }

  return status;
}

/* curl functions */
static size_t http_curl_write_func( void *ptr, size_t size, size_t nmemb, void *stream)
{
  PFetchHandleClass handle_class = PFETCH_HANDLE_GET_CLASS(stream);
  size_t size_handled = 0;
  GError *error     = NULL;
  guint actual_read = 0;
  guint saved_size  = 0;
  GQuark detail     = 0;
  PFetchStatus signal_return;

  saved_size = actual_read = size * nmemb;

  signal_return = emit_signal(PFETCH_HANDLE(stream),
			      handle_class->handle_signals[HANDLE_READER_SIGNAL],
			      detail, ptr, &actual_read, error);
  
  if(signal_return == PFETCH_STATUS_OK)
    size_handled = actual_read;
  
  if(saved_size != actual_read)
    g_warning("'%d' != '%d'. Are you sure?", saved_size, actual_read);


  return size_handled;
}

static size_t http_curl_header_func( void *ptr, size_t size, size_t nmemb, void *stream)
{
  size_t size_handled = 0;

  size_handled = size * nmemb;

  return size_handled;
}
/* end of curl functions */


static char *build_post_data(PFetchHandleHttp pfetch, char *sequence)
{
  GString *post_string = NULL;
  char *post = NULL;

  if(sequence)
    {
      char *argv[256] = { '\0' };
      char **argv_ptr = &argv[1];

      post_string = g_string_sized_new(128);
      
      g_string_append_printf(post_string, "request=");
      
      pfetch_get_argv(PFETCH_HANDLE(pfetch), sequence, argv);

      if(argv_ptr && *argv_ptr)
	{
	  while(argv_ptr && *argv_ptr)
	    {
	      g_string_append_printf(post_string, "%s ", *argv_ptr);
	      argv_ptr++;
	    }
	}

      post = g_string_free(post_string, FALSE);
    }

  return post;
}


static void conn_close_handler(CURLObject curl_object, PFetchHandleHttp pfetch)
{
  pfetch->request_counter--;

  if(pfetch->request_counter == 0)
    {
      PFetchHandleClass handle_class = PFETCH_HANDLE_GET_CLASS(pfetch);
      PFetchStatus signal_return = PFETCH_STATUS_OK;
      GQuark detail = 0;

      signal_return = emit_signal(PFETCH_HANDLE(pfetch),
				  handle_class->handle_signals[HANDLE_CLOSED_SIGNAL], 
				  detail, NULL, NULL, NULL);
    }

  return ;
}


#endif /* LIBPFETCH_READY */
