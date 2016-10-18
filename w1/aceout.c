/*  File: aceout.c
 *  Author: Danielle et jean Thierry-Mieg (mieg@mrc-lmba.cam.ac.uk)
 *  Copyright (C) J Thierry-Mieg and R Durbin, 1995
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
 * -------------------------------------------------------------------
 * This file is part of the ACEDB genome database package, written by
 * 	Richard Durbin (MRC LMB, UK) rd@sanger.ac.uk, and
 *	Jean Thierry-Mieg (CRBM du CNRS, France) mieg@crbm.cnrs-mop.fr
 *
 * Description: threadsafe version of freeout
 * Exported functions: see aceio.h
 * HISTORY:
 * Last edited: Nov 22 14:59 2007 (edgrif)
 * * Dec  8 14:15 2000 (edgrif): Replace the static buffer with a
 *              dynamic resizing one, needed because sometimes we
 *              have to output single huge strings (e.g. dna object).
 * Created: Sun Aug 30 1999 (mieg)
 * CVS info:   $Id: aceout.c,v 1.28 2007-11-26 17:07:12 edgrif Exp $
 *-------------------------------------------------------------------
 */

#include <glib.h>
#include <wh/regular.h>
#include <wh/aceio.h>					    /* provide incomplete (public) type */

/************************************************************/

/* We used to have a huge buffer within the aceout struct because we didn't  */
/* want to fail all the time. Now, with dynamic buffers we can afford to be  */
/* more parimonious I think.                                                 */
/*                                                                           */
enum {ACEOUT_INITIAL_BUFSIZE = 16384} ;


static magic_t ACEOUT_MAGIC = "ACEOUT";

struct AceOutStruct		/* complete opaque type */
{
  magic_t *magic ;		/* == &ACEOUT_MAGIC */
  FILE *fil;			/* internal - belongs to this struct */
  char *filename;
  Stack s ;			/* belongs to calling code */
  int buf_len ;
  char *buf ;
  int line ;  /* line number */
  int pos ;   /* char number in line */
  long int byte ;  /* total byte length */
  STORE_HANDLE handle;
} AceOutRec ;


static magic_t ACETMP_MAGIC = "ACETMP";

struct AceTmpStruct
{
  magic_t *magic;					    /* == &ACETMP_MAGIC */
  ACEOUT fo;
  char *filename;					    /* the actual tmp filename. */
  BOOL remove ;						    /* default TRUE => remove tmp files
							       when ACETMP is destroyed. */
};


/***************/

static BOOL aceOutExists (ACEOUT fo);
static ACEOUT aceOutSetFileStack (FILE *fil, Stack s, STORE_HANDLE handle);
static void aceOutCloseFileStack(ACEOUT fo) ;
static void aceOutFinalise (void *block);
static int aceOutBuffer (ACEOUT fo, char *text); /* returns errno */
static void aceTmpFinalise (void *block);
static BOOL aceTmpExists (ACETMP atmp);
static void increaseBuffer(ACEOUT fo, int bytes_needed) ;

/******************************************************************
 ************************* public functions ***********************
 ******************************************************************/

ACEOUT aceOutCreateToFile (char *filename, const char *spec, STORE_HANDLE handle)
     /* spec is one of "w", "wb", "a" or "ab" */
{ 
  FILE *fil;
  ACEOUT fo = NULL;

  if (!filename) messcrash("aceOutCreateToFile() - NULL filename");
  if (!spec) messcrash("aceOutCreateToFile() - NULL spec");
  if (!(spec[0] == 'w' || spec[0] == 'a')) messcrash("aceOutCreateToFile() - non-'w' or 'a' spec");

  fil = filopen(filename, "", (char*)spec); 
  /* NOTE, will messerror on failure */

  if (fil)
    {
      fo = aceOutSetFileStack (fil, 0, handle);
      fo->filename = strnew (filename, fo->handle);
    }

  return fo;
} /* aceOutCreateToFile */


ACEOUT aceOutCreateToURL (char *url, const char *spec, STORE_HANDLE handle)
{
  ACEOUT fo = NULL;

  if (!url) messcrash("aceOutCreateToURL() - NULL url");
  if (!spec) messcrash("aceOutCreateToURL() - NULL spec");
  if (!(spec[0] == 'w' || spec[0] == 'a')) messcrash("aceOutCreateToURL() - non-'w' or 'a' spec");

  if (strlen(url) > 8 
      && strncmp(url, "mailto:", 7) == 0)
    {
      char *address = url + 7;

      fo = aceOutCreateToMail (address, handle);
    }
  else if (strcmp(url, "stdout://") == 0)
    {
      fo = aceOutCreateToStdout (handle);
    }
  else if (strcmp(url, "stderr://") == 0)
    {
      fo = aceOutCreateToStderr (handle);
    }
  else if (strcmp(url, "stack://") == 0)
    {
      /* The word stack may be misleading, and after all it only
       * represents an extensible text-buffer */
      messerror ("Cannot re-open output that was "
		 "printing to a text-buffer");
    }
  else				/* filename */
    {
      fo = aceOutCreateToFile (url, spec, handle);
    }

  return fo;
} /* aceOutCreateToURL */


ACEOUT aceOutCreateToChooser (char *prompt, 
			      char *directory, char *filename,
			      const char *extension,
			      const char *spec, STORE_HANDLE handle)
{
  FILE *fil;
  ACEOUT fo = NULL;

  if (!directory) messcrash("aceOutCreateToChooser() - NULL directory");
  if (!filename) messcrash("aceOutCreateToChooser() - NULL filename");
  if (!spec) messcrash("aceOutCreateToChooser() - NULL spec");
  if (!(spec[0] == 'w' || spec[0] == 'a')) messcrash("aceOutCreateToChooser() - non-'w' or 'a' spec");

  fil = filqueryopen (directory, filename,
		      (char*)extension, (char*)spec, prompt);
  if (!fil)
    return NULL;		/* user clicked "Cancel" etc. */


  fo = aceOutSetFileStack (fil, 0, handle);
  
  /**** now set the URL *****/

  /* until filqueryopen can tell us in a better way whether it
   * wrote to a mailaddress we have to hack here */
  {
    extern Associator mailAddress ;
    char *address;

    if (mailAddress && assFind (mailAddress, fil, &address))
      {
	fo->filename = halloc(strlen("mailto:") +
			      strlen(address) + 1, fo->handle);
	sprintf (fo->filename, "mailto:%s", address);

	return fo;
      }
  }
    
  /* Assemble pathname :
   * Note, the assembled filename will be 
   * <directory>/<filename>.<extension>
   * the filename will NOT incorporate the extension, 
   * because on a Save-FileChooser it isn't editable by the user. */
  fo->filename = halloc (strlen(directory) +
			 strlen(SUBDIR_DELIMITER_STR) +
			 strlen(filename) +
			 strlen(extension) + 2, fo->handle);
  strcpy (fo->filename, directory);
  strcat (fo->filename, SUBDIR_DELIMITER_STR);
  strcat (fo->filename, filename);
  if (extension != NULL && strlen(extension) > 0)
    {
      strcat (fo->filename, ".");
      strcat (fo->filename, extension);
    }

  return fo;
} /* aceOutCreateToChooser */


ACEOUT aceOutCreateToMail (char *address, STORE_HANDLE handle)
{ 
  FILE *fil;
  ACEOUT fo = NULL;

  if (!address) messcrash("aceOutCreateToMail() - NULL address");

  fil = filmail(address);

  if (fil)
    {
      fo = aceOutSetFileStack (fil, 0, handle);
      fo->filename = halloc (strlen("mailto:") +
			     strlen(address) + 1, fo->handle);
      sprintf(fo->filename, "mailto:%s", address);
    }

  return fo;
} /* aceOutCreateToMail */


ACEOUT aceOutCreateToStdout (STORE_HANDLE handle)
{
  ACEOUT fo = NULL;

  fo = aceOutSetFileStack (stdout, 0, handle);
  fo->filename = strnew("stdout://", fo->handle);

  return fo;
} /* aceOutCreateToStdout */


ACEOUT aceOutCreateToStderr (STORE_HANDLE handle)
{
  ACEOUT fo = NULL;

  fo = aceOutSetFileStack (stderr, 0, handle);
  fo->filename = strnew("stderr://", fo->handle);

  return fo;
} /* aceOutCreateToStderr */


ACEOUT aceOutCreateToStack (Stack s, STORE_HANDLE handle)
{
  ACEOUT fo = NULL;

  if (!stackExists(s)) messcrash("aceOutCreateToStack() - bad stack");

  fo = aceOutSetFileStack (0, s, handle);
  fo->filename = strnew("stack://", fo->handle);

  return fo;
}

/************************************************/

ACEOUT aceOutCopy (ACEOUT source_fo, STORE_HANDLE handle)
{
  ACEOUT new_fo;

  if (source_fo->fil)
    new_fo = aceOutSetFileStack (source_fo->fil, 0, handle);
  else
    new_fo = aceOutSetFileStack (0, source_fo->s, handle);

  new_fo->filename = strnew(source_fo->filename, new_fo->handle);

  return new_fo;
} /* aceOutCopy */


/* Rather than constantly allocate/deallocate an entire ACEOUT because you are
 * perhaps looping and need top reuse one, you can just set a new stack with
 * this function. */
void aceOutSetNewStack(ACEOUT fo, Stack s)
{
  messAssert(aceOutExists(fo) && stackExists(s)) ;

  aceOutCloseFileStack(fo) ;

  fo->s = s ;

  return ;
}


/************************************************/

char *aceOutGetURL (ACEOUT fo)
{
  return fo->filename;
}

/************************************************/

char *aceOutGetFilename(ACEOUT fo)
{
  return fo->filename;
}

/************************************************/

void uAceOutDestroy (ACEOUT fo)	/* only to be called via macro */
{ 
  if (!aceOutExists(fo))
    messcrash("uAceOutDestroy() - received invalid fo pointer");

  messfree (fo) ;		/* trigger aceOutFinalise */

  return ;
} /* uAceOutDestroy */

/************************************************/

BOOL aceOutRewind (ACEOUT fo)
     /* will only work on files */
{
  BOOL result = FALSE;

  if (!aceOutExists(fo))
    messcrash("aceOutRewind() - received invalid fo pointer");

  if (fo->fil && fo->fil != stdout && fo->fil != stderr)
    {
      if (fseek(fo->fil, 0, SEEK_SET) == 0)
	result = TRUE;
      else
	messerror ("Cannot fseek file %s (%s)",
		   fo->filename, messSysErrorText());
    }

  return result;
} /* aceOutRewind */

/************************************************/

BOOL aceOutFlush (ACEOUT fo)
     /* will only work on files */
{
  BOOL result = FALSE;

  if (!aceOutExists(fo))
    messcrash("aceOutRewind() - received invalid fo pointer");

  if (fo->fil)
    {
      if (fflush(fo->fil) == 0)
	result = TRUE;
      else
	messerror ("Cannot fflush file %s (%s)",
		   fo->filename, messSysErrorText());
    }

  return result;
} /* aceOutRewind */

/************************************************/

BOOL aceOutStreamPos (ACEOUT fo, long int *posp)
{
  BOOL result = FALSE;

  if (fo->fil && fo->fil != stdout && fo->fil != stderr)
    {
      long int pos;

      pos = ftell (fo->fil);

      if (pos != -1)
	{
	  *posp = pos;
	  result = TRUE;
	}
      else
	{
	  messerror ("Cannot ftell file %s (%s)",
		     fo->filename, messSysErrorText());
	}
    }
  else if (fo->s)
    {
      *posp = (long int)stackPos (fo->s);
      result = TRUE;
    }

  return result;
} /* aceOutStreamPos */

/*************************************************************/

int aceOutBinary (ACEOUT fo, char *data, int size)
     /* copy a binary structure onto a text stack
      * RETURNS errno */
{ 
  int errno_result = ESUCCESS;
  int num_bytes_written = 0;

  if (!aceOutExists(fo))
    messcrash("aceOutBinary() - received invalid fo pointer");

  if (fo->fil)
    {
      num_bytes_written = fwrite(data,size,1,fo->fil);
      
      if (ferror (fo->fil) != 0)
	errno_result = errno;	/* error occurred */
    }
  else if (fo->s) 
    {
      catBinary (fo->s,data,size);
      fo->pos = 0;
      fo->line++;
    }

  fo->byte += (long int)num_bytes_written;

  return errno_result;
} /* aceOutBinary */


/*************************************************************/
/* Print output at specified position in a "teletext"-like way.              */
/*                                                                           */
int aceOutxy (ACEOUT fo, char *text, int x, int y)
     /* returns errno */
{ 
  int errno_result = ESUCCESS;
  int i, j, k ;
  int bytes_needed ;

  if (!aceOutExists(fo))
    messcrash("aceOutxy() - received invalid fo pointer");

  /* displacement from where we are now.                                     */
  i = x - fo->pos , j = y - fo->line, k = 0 ;

  /* The most we can write into buf is (i + j), so the buffer had better be  */
  /* at least that big  (- terminating null).                                */
  bytes_needed = i + j ;
  if (bytes_needed >= fo->buf_len)
    increaseBuffer(fo, bytes_needed) ;

  /* In theory we shouldn't need to do this but its very good for catching   */
  /* bugs...                                                                 */
  memset (fo->buf, 0, fo->buf_len) ;

  /* "move" to the right x,y position in the output stream using a combin-   */
  /* ation of blanks and newlines.                                           */
  if (i || j)
    { 
      if (j > 0)
	{
	  while (j--)
	    fo->buf[k++] = '\n' ;
	  i = x ;
	}
      if (i < 0)
	{
	  fo->buf[k++] = '\n' ;
	  i = x ; fo->line-- ; /* kludge, user should ignore this line feed */
	}
      if (i > 0)
	{
	  while (i--)
	    fo->buf[k++] = ' ' ;
	}
      if (k >= fo->buf_len)
	messcrash("Internal coding error, aceOutxy has overwritten its buffer,"
		  "buffer length: %d, num bytes written: %d, positions: x=%d, y=%d",
		  fo->buf_len, k, x, y) ;
      
      if (k)
	errno_result = aceOutBuffer (fo, fo->buf) ;
    } 

  /* Now output the callers text at x,y                                      */
  if (errno_result == ESUCCESS)
    errno_result = aceOutBuffer (fo, text) ;

  return errno_result;
} /* aceOutxy */

/************************************************/

/* Print simple text to stdout/file/whatever.
 * returns errno - ESUCCESS if all is OK
 */
int aceOutPrintStr(ACEOUT fo, char *simple_string)
{
  int errno_result ;

  if (!aceOutExists(fo) || !simple_string)
    messcrash("aceOutPrintStr() - received %s %s",
	      !aceOutExists(fo) ? "invalid fo pointer" : "",
	      !simple_string ? "null string" : "");

  errno_result = aceOutBuffer(fo, simple_string) ;

  return errno_result;
}

/************************************************/

/* Print text to stout/file/whatever.                                        */
/* returns errno - ESUCCESS if all is OK */
/*                                                                           */
int aceOutPrint(ACEOUT fo, char *format,...)
{
  int errno_result ;
  int bytes_needed, bytes_written ;
  va_list args1, args2 ;

  if (!aceOutExists(fo))
    messcrash("aceOutPrint() - received invalid fo pointer");

  /* We would like to check that if there are no arguments after the format string,
   * then the format string should not contain a single "%" on its own. But this
   * is not possible with the va_args interface. */

  va_start(args1, format) ;
  G_VA_COPY(args2, args1) ;

  /* check size of buffer required, if not big enough then allocate a buffer */
  /* quite a bit bigger. NOTE, this is the absolute maximum...bytes written  */
  /* may be less, this includes terminating null.                            */
  bytes_needed = g_printf_string_upper_bound(format, args1) ;

  if (bytes_needed > fo->buf_len)
    increaseBuffer(fo, bytes_needed) ;

  /* In theory we shouldn't need to do this but its very good for catching   */
  /* bugs...                                                                 */
  memset(fo->buf, 0, fo->buf_len) ;

  /* OK, format the string using this much better call which limits the      */
  /* chars written to our buffer length. Note bytes_written does _not_       */
  /* include terminating null so max bytes_written = fo->buf_len - 1         */
  bytes_written = g_vsnprintf(fo->buf, fo->buf_len, format, args2) ;
  if (bytes_written >= bytes_needed || bytes_written >= fo->buf_len)
    messcrash("A call to g_vsnprintf() failed - "
	      "buffer size: %d,  predicted bytes required: %d,  bytes actually written: %d",
	      fo->buf_len, bytes_needed, bytes_written) ;

  va_end(args1) ;
  va_end(args2) ;

  errno_result = aceOutBuffer(fo, fo->buf) ;

  return errno_result;
} /* aceOutPrint */

/************************************************/

int aceOutLine (ACEOUT fo)
     /* how many lines have been written */
{ 
  if (!aceOutExists(fo))
    messcrash("aceOutLine() - received invalid fo pointer");

  return fo->line ; 
}

long int aceOutByte (ACEOUT fo)
     /* how many bytes have been written */
{
  if (!aceOutExists(fo))
    messcrash("aceOutByte() - received invalid fo pointer");

  return fo->byte ; 
}

int aceOutPos (ACEOUT fo)
{
  if (!aceOutExists(fo))
    messcrash("aceOutPos() - received invalid fo pointer");

  return fo->pos ; 
}

/********* ACETMP - writing to temporary files using ACEOUT ********/

/* Simply a cover func. for aceTmpCreateDir()                                */
/*                                                                           */
ACETMP aceTmpCreate (const char *spec, STORE_HANDLE handle)
{ 
  ACETMP result = NULL ;

  result = aceTmpCreateDir(NULL, spec, handle) ;

  return result ;
} /* aceTmpCreate */


/* Creates a unique tmp file in one of:                                      */
/*     if dir is NULL the file is created in the system tmp dir,             */
/*     if dir is relative, then dir will be created under the system tmp dir */
/*        if it doesn't exist and the file created there,                    */
/*     if dir is absolute, then dir is created if it doesn't exist and the   */
/*        file created there.                                                */
/*                                                                           */
/* spec is one of "w" or "wb" */
ACETMP aceTmpCreateDir(const char *dir, const char *spec, STORE_HANDLE handle)
{
  ACEOUT fo = NULL;
  ACETMP atmp = NULL, result = NULL ;
  char *dirname = NULL ;
  char *nameptr = NULL ;
  BOOL status = TRUE ;
  char *tmpfile = getenv("TEMP");  /* allow user to override location of temp files */

  if (!spec)
    messcrash("aceTmpCreate() - received NULL spec");
  if (spec[0] != 'w')
    messcrash("aceTmpCreate() -  non-'w' spec");

  if (!dir || !(*dir) || *dir != '/')
    {
      char *basename = 
#if defined(SUN) || defined(SOLARIS)
	(tmpfile ? tmpfile : "/var/tmp") ;
#elif defined(__CYGWIN__)
        (tmpfile ? tmpfile : "/cygdrive/c/Temp") ;
#else
        (tmpfile ? tmpfile : "/tmp") ;
#endif

      dirname = hprintf(0, "%s%s%s",
			basename,
			((dir && *dir && *dir == '/') ? "" : "/"),
			((dir && *dir) ? dir : "")) ;
    }
  else
    dirname = strnew((char *)dir, 0) ;


  if (!filCheckName(dirname, NULL, "x"))
    {
      if (mkdir(dirname, S_IRUSR | S_IWUSR | S_IXUSR) == -1)
	{
	  messerror ("Failed to create parent directory \"%s\" for temporary files (%s)",
		     dirname, messSysErrorText()) ;
	  status = FALSE ;
	}
    }

  if (!(nameptr = tempnam(dirname, "ACEDB")))
    {
      messerror ("Failed to create temporary file in directory \"%s\" (%s)",
		 dirname, messSysErrorText()) ;
      status = FALSE ;
    }
  
  /* first check if we can actually create the file */
  if (status)
    {
      fo = aceOutCreateToFile (nameptr, spec, 0);
      if (!fo)
	{
	  messerror ("Failed to create ACEIN for temporary file \"%s\" (%s)",
		     nameptr, messSysErrorText()) ;
	  status = FALSE ;
	}
    }

  if (status)
    {
      atmp = halloc (sizeof(struct AceTmpStruct), handle);
      blockSetFinalise (atmp, aceTmpFinalise);
      atmp->magic = &ACETMP_MAGIC;
      atmp->fo = fo;
      atmp->filename = strnew (nameptr, 0);
      atmp->remove = TRUE ;

      result = atmp ;
    }

  /* tidy up                                                                 */
  if (nameptr)
    free(nameptr);					    /* N.B. was allocated by tempname
							       using malloc */
  if (dirname)
    messfree(dirname) ;

  return result ;
}

void uAceTmpDestroy (ACETMP atmp)
{
  if (!aceTmpExists(atmp))
    messcrash("aceTmpDestroy() - called with invalid ACETMP pointer");

  messfree (atmp);		/* trigger aceTmpFinalise */

  return;
} /* uAceTmpDestroy */

void aceTmpClose (ACETMP atmp)
{
  /* NOTE: this zeroes the fo field */
  aceOutDestroy (atmp->fo);

  return;
} /* aceTmpClose */

ACEOUT aceTmpGetOutput (ACETMP atmp)
     /* The returned ACEOUT stream may be destroyed by the
      * calling code before this ACETMP is finalised.
      * This enables the user to close the tmp-file
      * before removing it. */
{
  if (!aceTmpExists (atmp))
    messcrash("aceOutGetOutput() - received invalid atmp pointer");

  return atmp->fo;
} /* aceTmpGetOutput */

char *aceTmpGetFileName (ACETMP atmp)
{
  if (!aceTmpExists (atmp))
    messcrash("aceOutGetFileName() - received invalid atmp pointer");

  return atmp->filename;
} /* aceTmpGetFileName */


/* Set state so that tmp files will/will not be removed when the ACETMP is   */
/* destroyed (contorted logic is because default state is to _remove_.       */
void aceTmpNoRemove(ACETMP atmp, BOOL no_remove)
{
  atmp->remove = !no_remove ;

  return ;
}

/******************************************************************
 ************************ private functions ***********************
 ******************************************************************/

/* Increase size of our output buffer.                                       */
/* Policy here is that if we need to make the buffer bigger, we make it 1.5x */
/* what is needed to try and avoid constant reallocation just because the    */
/* next string is a bit bigger than the last. I don't think we need the      */
/* usual malloc policy of doubling because much of what we use is strings    */
/* which tend to be of a certain size depending on the objects/dna being     */
/* looked at.                                                                */
static void increaseBuffer(ACEOUT fo, int bytes_needed)
{
  fo->buf_len = bytes_needed + (bytes_needed / 2) ;
  messfree(fo->buf) ;
  fo->buf = (char *)halloc(fo->buf_len, fo->handle) ;

  return ;
}



static ACEOUT aceOutSetFileStack(FILE *fil, Stack s, STORE_HANDLE handle)
{ 
  ACEOUT fo;

  fo = (ACEOUT)halloc(sizeof(AceOutRec), handle);
  blockSetFinalise (fo, aceOutFinalise);
  fo->magic = &ACEOUT_MAGIC ;
  fo->handle = handleCreate();

  if (fil) 
    fo->fil = fil ;
  else if (s) 
    fo->s = s ;
  else
    messcrash("aceOutSetFileStack() - fil and s is NULL");

  fo->buf_len = ACEOUT_INITIAL_BUFSIZE ;
  fo->buf = (char *)halloc(fo->buf_len, fo->handle) ;

  fo->line = fo->pos = fo->byte = 0 ;    
  fo->filename = NULL ;

  return fo ;
} /* aceOutSetFileStack */

/************************************************/

/* Just close everything, leave the ACEOUT otherwise intact. Note that if there
 * is a stack, it belongs to the caller so we don't free it. */
static void aceOutCloseFileStack(ACEOUT fo)
{
  messAssert(aceOutExists(fo)) ;

  /* it'll check for stdout/stderr needed here to make sure the eventual filMail
   * is actually being sent */
  if (fo->fil)
    filclose (fo->fil);
  fo->fil = NULL ;

  if (fo->filename)
    messfree(fo->filename) ;
  fo->filename = NULL ;

  fo->line = fo->pos = fo->byte = 0 ;

  return ;
}


/************************************************/

static void aceOutFinalise (void *block)
{
  ACEOUT fo = (ACEOUT)block;

  if (!aceOutExists(fo))
    messcrash("aceOutFinalise() - received invalid block pointer");

  if (fo->fil)
    filclose (fo->fil);		/* it'll check for stdout/stderr
				 * needed here to make sure the eventual
				 * filMail is actually being sent
				 */
  handleDestroy(fo->handle);

  fo->magic = 0;		/* taint this memory segment as free'd
				 * this will catch assertions if we
				 * try to access this free'd bit 
				 * of memory */

  return;
} /* aceOutFinalise */

/************************************************/

static BOOL aceOutExists (ACEOUT fo)
{
  if (fo && fo->magic == &ACEOUT_MAGIC)
    return TRUE;

  return FALSE;
} /* aceOutExists */

/************************************************/

static int aceOutBuffer (ACEOUT fo, char *text)
     /* return errno, which is ESUCCESS, if all is OK */
{ 
  int errno_result = ESUCCESS;
  char *cp ;
  int pos = 0, line = 0, ln  ;
  
  if (!aceOutExists(fo))
    messcrash("aceOut() - received invalid fo pointer");

  cp = text ;
  ln = strlen(text) ;
  while (*cp) 
    if (*cp++ == '\n') 
      { pos = 0 ; line++ ;}
    else
      pos++ ;
  
  if (fo->fil)
    {
      int num_bytes_written;

      /* Output the text, n.b. text may contain "%" which we don't want to   */
      /* be interpreted, hence our "%s" format string.                       */
      num_bytes_written = fprintf(fo->fil, "%s", text) ;
      if (num_bytes_written < 0)
	errno_result = errno ;
      else
	ln = num_bytes_written ;
    }
  else if (fo->s)
    {
      catText(fo->s, text) ;
    }
  else 
    messcrash("aceOutBuffer() - fo struct has neither ->fil nor ->s");

  fo->byte += (long int)ln ;
  if (line)
    { fo->line += line ; fo->pos = pos ; }
  else
    fo->pos += pos ;

  return errno_result;
} /* aceOutBuffer */

/************** ACETMP **********************/

static BOOL aceTmpExists (ACETMP atmp)
{
  if (atmp && atmp->magic == &ACETMP_MAGIC)
    return TRUE;

  return FALSE;
} /* aceTmpExists */

static void aceTmpFinalise (void *block)
     /* Note that the ACETMP structure has a longer life-time
      * than its output stream (atmp->fo).
      * The life-time of the ACETMP struct is the lifetime of the 
      * tmp-file itself. The file _may_ be deleted upon destruction.
      * depending on the setting of remove */
{
  ACETMP atmp = (ACETMP)block;

  if (!aceTmpExists (atmp))
    messcrash("aceTmpFinalise() - received invalid block pointer");

  /* finish output to close the file, if not done already */
  if (atmp->fo)
    aceOutDestroy (atmp->fo);

  /* remove the file */
  if (atmp->remove)
    {
      if (unlink (atmp->filename) == -1)
	messerror ("Failed to remove tmp-file %s (%s)",
		   atmp->filename, messSysErrorText());
    }

  /* free the filename buffer */
  messfree (atmp->filename);

  atmp->magic = 0;		/* taint memory block as free'd */

  return;
} /* aceTmpFinalise */

/*********************** eof ********************************/
