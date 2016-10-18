/*  File: freesubs.c
 *  Author: Richard Durbin (rd@mrc-lmb.cam.ac.uk)
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
 * -------------------------------------------------------------------
 * This file is part of the ACEDB genome database package, written by
 * 	Richard Durbin (MRC LMB, UK) rd@mrc-lmb.cam.ac.uk, and
 *	Jean Thierry-Mieg (CRBM du CNRS, France) mieg@kaa.cnrs-mop.fr
 *
 * Description: free format input - record based
 * Exported functions: lots - see regular.h
 * HISTORY:
 * Last edited: Jul  2 09:06 2008 (edgrif)
 * * Apr 26 10:06 1999 (edgrif): Add code to trap NULL value for freedouble.
 * * Jan 14 13:31 1999 (fw): increased version umber to 1.1.2
 *              due to changes in the directory handling interface
 *              (Array)filDirectoryCreate -> (FilDir)filDirHandleCreate
 * * Dec  3 14:46 1998 (edgrif): Insert version macros for libfree.
 *    freecard ignores "\r" and "\n" under WIN32
 * * Sep 30 14:19 1998 (edgrif)
 * * Nov 27 12:30 1995 (mieg): freecard no longer stips \, freeword does
 *    also i added freeunprotect
 * Created: Sun Oct 27 18:16:01 1991 (rd)
 * CVS info:   $Id: freesubs.c,v 1.62 2008-07-02 12:54:25 edgrif Exp $
 *-------------------------------------------------------------------
 */
#include <stdlib.h>
#include "regular.h"
#include "version.h"
#include <ctype.h>

/* free package version and copyright string.    */
/*                                               */
#define FREE_TITLE   "Free library"
#define FREE_DESC    "Sanger Centre Informatics utilities library."

#define FREE_VERSION 1
#define FREE_RELEASE 1
#define FREE_UPDATE  2
#define FREE_VERSION_NUMBER  UT_MAKE_VERSION_NUMBER(FREE_VERSION, FREE_RELEASE, FREE_UPDATE)

static const char *ut_copyright_string = UT_COPYRIGHT_STRING(FREE_TITLE, FREE_VERSION, FREE_RELEASE, FREE_UPDATE, FREE_DESC)


/* uuugghhh, this has to go... 
 * have a look at session.s:realIsInteractive about how to determine this properly
 */
BOOL isInteractive = TRUE ;      /* can set FALSE, i.e. in tace */

 
#define MAXSTREAM 80
#define MAXNPAR 80
typedef struct
  { FILE *fil ;
    char *text ;
    char special[24] ;
    int npar ;
    int parMark[MAXNPAR] ;
    int line ;
    BOOL isPipe ;
  } STREAM ;
static STREAM   stream[MAXSTREAM] ;
static int	streamlevel ;
static FILE	*currfil ;	/* either currfil or currtext is 0 */
static char	*currtext ;	/* the other is the current source */
static Stack    parStack = 0 ;
static int	maxcard = 1024 ;
static unsigned char *card = 0, *word = 0, *cardEnd, *pos ;
static Associator filAss = 0 ;
 
#define _losewhite    while (*pos == ' '|| *pos == '\t') ++pos
#define _stepover(x)  (*pos == x && ++pos)
#define _FREECHAR     (currfil ? getc (currfil) : *currtext++)
 
/************************************/
 
void freeinit (void)
{ 
  static BOOL isInitialised = FALSE ;

  if (!isInitialised)
    {
      streamlevel = 0 ;
      currtext = 0 ;
      stream[streamlevel].fil = currfil = stdin ;
      stream[streamlevel].text = 0 ;
      freespecial ("\n\t\\/@%") ;
      card = (unsigned char *) messalloc (maxcard) ;
      cardEnd = &card[maxcard-1] ;
      pos = card ;
      word = (unsigned char *) messalloc (maxcard) ;
      filAss = assCreate () ;
      parStack = stackCreate (128) ;
      isInitialised = TRUE ;
    }

  return ;
}


/************************************/

char *freeVersion(void)
{
  return (char *)ut_copyright_string ;
}



/************************************/
 
void freeshutdown (void)
{
  messfree (card) ;
  messfree (word) ;
  stackDestroy (parStack) ;
  assDestroy (filAss) ;
}

/*******************/

/* Sometimes you may need to know if a function below you succeeded in       */
/* changing a stream level.                                                  */
int freeCurrLevel(void)
  {
  return streamlevel ;
  }

/*******************/

static void freeExtend (unsigned char **pin)
{	/* only happens when getting card */
  unsigned char *oldCard = card ;

  maxcard *= 2 ;
  card = (unsigned char *) messalloc (maxcard) ;
  if (oldCard)     /* jtm june 22, 1992 */
    memcpy (card, oldCard, maxcard/2) ;
  cardEnd = &card[maxcard-1] ;
  *pin += (card - oldCard) ;
  messfree (oldCard) ;
  messfree (word) ;
  word = (unsigned char *) messalloc (maxcard) ;
}
 
/********************/

static char special[256] ;

void freespecial (char* text)
{
  if (!text)
    messcrash ("freespecial received 0 text") ;
  if (strlen(text) > 23)
    messcrash ("freespecial received a string longer than 23") ;
  if (text != stream[streamlevel].special)
    strcpy (stream[streamlevel].special, text) ;
  memset (special, 0, (mysize_t) 256) ;
  while (*text)
    special [((int) *text++) & 0xFF] = TRUE ;
  special[0] = TRUE ;
  special[(unsigned char)EOF] = TRUE ;		/* better have these to ensure streams terminate! */
}
 
/********************/
 
void freeforcecard (char *string)
{ int level = freesettext (string, "") ;
  freespecial ("") ;
  freecard (level) ;
}
 
/********************/

char* freecard (int level)	/* returns 0 when streamlevel drops below level */
{ 
  unsigned char *in,ch,*cp ;
  int kpar ;
  int isecho = FALSE ;		/* could reset sometime? */
  FILE *fil ;
  BOOL acceptShell, acceptCommand ;

restart :
  if (level > streamlevel)
    return 0 ;

  if (isecho)
    printf (!currfil ? "From text >" : "From file >") ;
  in = card - 1 ;

  acceptCommand = special['@'] ;
  acceptShell = special['$'] ;
 
  while (TRUE)
    { if (++in >= cardEnd)
	freeExtend (&in) ;

      *in = _FREECHAR ;
    lao:
      if (special[((int) *in) & 0xFF] && *in != '$' && *in != '@' )
	switch (*in)
	  {
#if defined(WIN32)
	  case '\r':
		  continue ; /* ignore carriage returns */ 
#endif
	  case '\n':		/* == '\x0a' */
	  case ';':		/* card break for multiple commands on one line */
	    goto got_line ;
	  case (unsigned char) EOF:
	  case '\0':
	    freeclose(streamlevel) ;
	    goto got_line;
	  case '\t':     /* tabs should get rounded to 8 spaces */
	    if (isecho)	/* write it out */
	      putchar (*in) ;
            *in++ = ' ' ;
            while ((in - card) % 8)
              { if (in >= cardEnd)
                  freeExtend (&in) ;
                *in++ = ' ' ;
              }
	    --in ;
	    continue ;  
	  case '/':		/* // means start of comment */
	    if ((ch = _FREECHAR) == '/')
	      { 
		while ((ch = _FREECHAR) != '\n' && ch && ch != (unsigned char)EOF) ;
		*in = ch ;
		goto lao ; /* mieg, nov 2001, to treat the EOF, this was breaking the server if sent a comment */
	      }
	    else
	      { if (isecho) putchar (*in) ;
		if (currfil)                     /* push back ch */
		  ungetc (ch, currfil) ;
		else
		  --currtext ;
	      }
	    break ;
	  case '%':		/* possible parameter */
	    --in ; kpar = 0 ;
	    while (isdigit (ch = _FREECHAR))
	      kpar = kpar*10 + (ch - '0') ;
	    if (kpar > 0 && kpar <= stream[streamlevel].npar)
	      for (cp = (unsigned char *) stackText (parStack, 
			     stream[streamlevel].parMark[kpar-1]) ; *cp ; ++cp)
		{ if (++in >= cardEnd)
		    freeExtend (&in) ;
		  *in = *cp ;
		  if (isecho)
		    putchar (*in) ;
		}
	    else
	      messout ("Parameter %%%d can not be substituted", kpar) ;
	    if (++in >= cardEnd)
	      freeExtend (&in) ;
	    *in = ch ; 
	    goto lao ; /* mieg */
	  case '\\':		/* escapes next character - interprets \n */
	    *in = _FREECHAR ;
	    if (*in == '\n')    /* fold continuation lines */
	      { if (isInteractive && !streamlevel)
		  printf ("  Continuation >") ;
		while ((ch = _FREECHAR) == ' ' || ch == '\t') ;
			/* remove whitespace at start of next line */
		if (currfil)                     /* push back ch */
		  ungetc (ch, currfil) ;
		else
		  --currtext ;
		stream[streamlevel].line++ ;
		--in ;
	      }
#if !defined(WIN32)
	    else if (*in == 'n') /* reinterpret \n as a format */
	      { *in = '\n' ; 
	      }
#endif
	    else  /* keep the \ till freeword is called */
	      { *(in+1) = *in ;
		*in = '\\' ;
		if (++in >= cardEnd)
		  freeExtend (&in) ;
	      }
	    break ;
	  default:
	    messerror ("freesubs got unrecognised special character 0x%x = %c\n",
		     *in, *in) ;
	  }
      else
	{ if (!isprint(*in) && *in != '\t' && *in != '\n') /* mieg dec 15 94 */
	    --in ;
	  else if (isecho)	/* write it out */
	    putchar (*in) ;
	}
    }				/* while TRUE loop */
 
got_line:
  stream[streamlevel].line++ ;
  *in = 0 ;
  if (isecho)
    putchar ('\n') ;
  pos = card ;
  _losewhite ;
  if (acceptCommand && _stepover ('@'))        /* command file */
    { char *name ;
      if ((name = freeword ()) && 
	  (fil = filopen (name, 0, "r")))
	freesetfile (fil, (char*) pos) ;
      goto restart ;
    }
  if (acceptShell && _stepover ('$'))        /* shell command */
    {
#if !defined(MACINTOSH)
      system ((char*)pos) ;
#endif
      goto restart ;
    }

  return (char*) card ;
}
 
/************************************************/

void freecardback (void)    /* goes back one card */
{ stream[streamlevel].line-- ;
  freesettext ((char*) card, "") ;
}

/************************************************/

/* reads card from fil, has some embedded logic for dealing with
 * escaped newlines which doesn't use the freespecial() stuff.
 * I don't know the reason for this and probably it could be added
 * but this is now legacy code as the aceIn/Out package should be
 * used instead. */
BOOL freeread (FILE *fil)
{
  unsigned char ch, *in = card ;
  int  *line, chint ;
  
  if (!assFind (filAss, fil, &line))
    {
      line = (int*) messalloc (sizeof (int)) ;
      assInsert (filAss, fil, line) ;
    }
 
  --in ;
  while (TRUE)
    { ++in ;
      if (in >= cardEnd)
	    freeExtend (&in) ;
	  chint = getc(fil) ;
	  if (ferror(fil))
	  	messerror ("chint was bad");
	  *in = chint ;
      switch (*in)
        {
	case '\n' :
	  ++*line ;
	case (unsigned char) EOF :
	  goto got_line ;
	case '/' :		/* // means start of comment */
	  if ((ch = getc (fil)) == '/')
	    { while (getc(fil) != '\n' && !feof(fil)) ;
	      ++*line ;
	      if (in > card)	/* // at start of line ignores line */
		goto got_line ;
	      else
		--in ; /* in = 0   unprintable, so backstepped */
	    }
	  else
	    ungetc (ch,fil) ;
	  break ;

	case '\\' :		/* escape next character, NOTE fall through. */
	  *in = getc(fil) ;
	  if (*in == '\n')	/* continuation */
	    { ++*line ;
	      while (isspace (*in = getc(fil))) ;    /* remove whitespace */
	    }
	  else if (*in == '"' ||
		   *in == '/' || 
		   *in == '\\' || 
		   *in == 'n') /* escape for freeword, newline */
	    {
	      *(in+1) = *in ;
	      *in = '\\' ;
	      ++in ;
	    }
	  /* NB fall through - in case next char is nonprinting */

	default:
	  if (!isprint (*in) && *in != '\t')	/* ignore control chars, e.g. \x0d */
	    --in ;
	}
    }
 
got_line :
  *in = 0 ;
  pos = card ;
  _losewhite ;
  if (feof(fil))
    { assRemove (filAss, fil) ;
      messfree (line) ;
    }
  return *pos || !feof(fil) ;
}

int freeline (FILE *fil)
{ int *line ;

  if (assFind (filAss, fil, &line))
    return *line ;
  else
    return 0 ;
}
 
int freestreamline (int level)
{ 
  return stream[level].line  ;
}
 
/********************************************/


static void freenewstream (char *parms)
{
  int kpar ;

  stream[streamlevel].fil = currfil ;
  stream[streamlevel].text = currtext ;
  if (++streamlevel == MAXSTREAM)
    messcrash ("MAXSTREAM overflow in freenewstream") ;
  strcpy (stream[streamlevel].special, stream[streamlevel-1].special) ;

  stream[streamlevel].npar = 0 ;
  stream[streamlevel].line = 1 ;

 if (!parms || !*parms)
    return ;                           /* can t abuse NULL ! */
  pos = (unsigned char *) parms ;			/* abuse freeword() to get parms */

  for (kpar = 0 ; kpar < MAXNPAR && freeword () ; kpar++) /* read parameters */
    { stream[streamlevel].parMark[kpar] = stackMark (parStack) ;
      pushText (parStack, (char*) word) ;
    }

  stream[streamlevel].npar = kpar ;
  stream[streamlevel].isPipe = FALSE ;
  pos = card ;			/* restore pos to start of blank card */
  *card = 0 ;
}
 
int freesettext (char *string, char *parms)
{
  freenewstream (parms) ;

  currfil = 0 ;
  currtext = string ;

  return streamlevel ;
}

int freesetfile (FILE *fil, char *parms)
{
  freenewstream (parms) ;

  currfil = fil ;
  currtext = 0 ;

  return streamlevel ;
}

int freesetpipe (FILE *fil, char *parms)
{
  freenewstream (parms) ;

  currfil = fil ;
  currtext = 0 ;
  stream[streamlevel].isPipe = TRUE ;
  return streamlevel ;
}

void freeclose(int level)
{ 
  int kpar ;

  while (streamlevel >= level)
    { if (currfil && currfil != stdin && currfil != stdout)
	{
	  if (stream[streamlevel].isPipe)
	    pclose (currfil) ;
	  else
	    filclose (currfil) ;
	}
      for (kpar = stream[streamlevel].npar ; kpar-- ;)
	popText (parStack) ;
      --streamlevel ;
      currfil = stream[streamlevel].fil ;
      currtext = stream[streamlevel].text ;
      freespecial (stream[streamlevel].special) ;
    }
}

/************************************************/
/* freeword(), freewordcut() and freestep() are the only calls that
     directly move pos forward -- all others act via freeword().
   freeback() moves pos back one word.
*/
 
char *freeword (void)
{
  unsigned char *cw ;
 
  _losewhite ;             /* needed in case of intervening freestep() */

  if (_stepover ('"'))
    { for (cw = word ; !_stepover('"') && *pos ; *cw++ = *pos++)
	if (_stepover('\\'))	/* accept next char unless end of line */
	  if (!*pos)
	    break ;
      _losewhite ;
      *cw = 0 ;
      return (char*) word ;	/* always return a word, even if empty */
    }

		/* default: break on space and \t, not on comma */
  for (cw = word ; isgraph (*pos) && *pos != '\t' ; *cw++ = *pos++)
    if (_stepover('\\'))	/* accept next char unless end of line */
      if (!*pos)
	break ;
  _losewhite ;
  *cw = 0 ;
  return *word ? (char*) word : 0 ;
}
 
/************************************************/

#if defined(WIN32)

char *freepath (void)
{
  unsigned char *cw ;
 
  _losewhite ;             /* needed in case of intervening freestep() */

  if (_stepover ('"'))
    { for (cw = word ; !_stepover('"') && *pos ; *cw++ = *pos++)
	if (_stepover('\\'))	/* accept next char unless end of line */
	  if (!*pos)
	    break ;
      _losewhite ;
      *cw = 0 ;
      return (char*) word ;	/* always return a word, even if empty */
    }

  /* default: break on space, \t or end of line, not on comma
	 also, does not skip over backslashes which are assumed to be
	 MS DOS/Windows path delimiters */
  for (cw = word ; ( *pos == '\\' || isgraph (*pos) ) && *pos != '\t' ; *cw++ = *pos++) ;
  _losewhite ;
  *cw = 0 ;
  return *word ? (char*) word : 0 ;
}

#endif
 
/************************************************/
 
char *freewordcut (char *cutset, char *cutter)
        /* Moves along card, looking for a character from cut, which is a
           0-terminated char list of separators.
           Returns everything up to but not including the first match.
           pos is moved one char beyond the character.
           *cutter contains the char found, or if end of card is reached, 0.
        */
{ unsigned char *cc,*cw ;
 
  for (cw = word ; *pos ; *cw++ = *pos++)
    for (cc = (unsigned char *) cutset ; *cc ; ++cc)
      if (*cc == *pos)
        goto wcut ;
wcut:
  *cutter = *pos ;
  if (*pos)
    ++pos ;
  _losewhite ;
  *cw = 0 ;
  return *word ? (char*) word : 0 ;
}
 
/************************************************/
 
void freeback (void)    /* goes back one word - inefficient but reliable */
 
 {unsigned char *now = pos ;
  unsigned char *old = pos ;
 
  pos = card ; _losewhite ;
  while  (pos < now)
   {old = pos ;
    freeword () ;
   }
  pos = old ;
 }
 
/************************************************/
/* Read a word representing an int from wherever free is pointing to.        */
/*                                                                           */
/* If there is no word OR the word cannot be converted to an int, reset      */
/*   the free pos, don't set the int param. and return FALSE                 */
/* If the word is "NULL" set int param to the POSIX "too small" int value    */
/*   and return TRUE                                                         */
/* Otherwise set the int param to the converted int and return TRUE          */
/*                                                                           */
/* Note that valid range of ints is    INT_MIN < valid < INT_MAX             */
/* otherwise UT_NON_INT doesn't work.                                        */
/*                                                                           */
BOOL freeint (int *p)
 {BOOL result = FALSE ;
  unsigned char *keep ;
  enum {DECIMAL_BASE = 10} ;
  char *endptr ;
  long int bigint ;

  keep = pos ;
  if (freeword ())
    { /*printf ("freeint got '%s'\n", word) ;*/
      if (!strcmp((char *)word, "NULL"))
	{ *p = UT_NON_INT ;
	  result = TRUE ;
	}
      else
	{
	errno = 0 ;
	bigint = strtol((char *)word, &endptr, DECIMAL_BASE) ;

	if ((bigint == 0 && endptr == (char *)word)	    /* first character wrong */
	    || (endptr != (char *)(word + strlen((char *)word))) /* some other character wrong */
	    || (errno == ERANGE)			    /* number too small/big for long int */
            || (bigint <= INT_MIN || bigint >= INT_MAX))    /* number too small/big for int */
	  {
	  pos = keep ;
	  return FALSE ;
	  }
	else
	  {
	  *p = (int)bigint ;
	  result = TRUE ;
	  }
	}
    }
  else
    { pos = keep ;
    result = FALSE ;
    } 
  return result ;
 }
 
/*****************************/
/* Read a word representing a float from wherever free is pointing to.       */
/*                                                                           */
/* If there is no word OR the word cannot be converted to a float, reset     */
/*   the free pos, don't set the float param. and return FALSE               */
/* If the word is "NULL" set float param to the POSIX "too small" float value*/
/*   and return TRUE                                                         */
/* Otherwise set the float param to the converted float and return TRUE      */
/*                                                                           */
/* Note that valid range of floats is:                                       */
/*                             -ve  -FLT_MAX < valid <  -FLT_MIN             */
/*                             +ve   FLT_MIN < valid <   FLT_MAX             */
/* otherwise UT_NON_FLOAT doesn't work as a range check for applications.    */
/*                                                                           */
BOOL freefloat (float *p)
{
  BOOL result = FALSE ;  
  unsigned char *keep ;
  double bigfloat ;
  char *endptr ;

  keep = pos ;
  if (freeword ())
    {
      if (strcmp ((char*)word, "NULL") == 0)
	{
	  *p = UT_NON_FLOAT ;				    /* UT_NON_FLOAT = -FLT_MAX */
	  result = TRUE ;
	}
      else
	{
	  errno = 0 ;
	  bigfloat = strtod((char *)word, &endptr) ;

	  if ((bigfloat == +0.0 && endptr == (char *)word)  /* first character wrong */
	      || (endptr != (char *)(word + strlen((char *)word))) /* some other character wrong */
	      || (errno == ERANGE)			    /* number too small/big for double */
	      || (bigfloat < 0 && (bigfloat >= -FLT_MIN || bigfloat <= -FLT_MAX))
	      || (bigfloat > 0 && (bigfloat <= FLT_MIN || bigfloat >= FLT_MAX)))
	    {						    /* number too small/big for float */
	      pos = keep ;
	      result = FALSE ;
	    }
	  else
	    {
	      *p = (float)bigfloat ;
	      result = TRUE ;
	    }
	}
    }
  else
    {
      pos = keep ;
      result = FALSE ;
    }

  return result ;
}
 
/**************************************************/
/* Read a word representing a double from wherever free is pointing to.      */
/*                                                                           */
/* If there is no word OR the word cannot be converted to a double, reset    */
/*   the free pos, don't set the double param. and return FALSE              */
/* Otherwise set the double param to the converted double and return TRUE    */
/*                                                                           */
/* Note that valid range of doubles is:                                      */
/*                             -ve  -DBL_MAX < valid <  -DBL_MIN             */
/*                             +ve   DBL_MIN < valid <   DBL_MAX             */
/* otherwise UT_NON_DOUBLE doesn't work as a range check for applications.   */
/*                                                                           */
/* Note that because double is the largest number we can only check to see   */
/* if the converted number is equal to this, not >= as in the case of floats.*/
/*                                                                           */
BOOL freedouble (double *p)
{ 
  BOOL result = FALSE ;  
  unsigned char *keep ;
  double bigfloat ;
  char *endptr ;

  keep = pos ;
  if (freeword ())
    {
      if (strcmp ((char*)word, "NULL") == 0)
	{
	  *p = UT_NON_DOUBLE ;				    /* UT_NON_DOUBLE = -DBL_MAX */
	  result = TRUE ;
	}
      else
	{
	  errno = 0 ;
	  bigfloat = strtod((char *)word, &endptr) ;

	  if ((bigfloat == +0.0 && endptr == (char *)word)  /* first character wrong */
	      || (endptr != (char *)(word + strlen((char *)word))) /* some other character wrong */
	      || (errno == ERANGE)			    /* number too small/big for double */
	      || (bigfloat < 0 && (bigfloat == -DBL_MIN || bigfloat == -DBL_MAX))
	      || (bigfloat > 0 && (bigfloat == DBL_MIN || bigfloat == DBL_MAX)))
	    {						    /* number too small/big. */
	      pos = keep ;
	      result = FALSE ;
	    }
	  else
	    {
	      *p = bigfloat ;
	      result = TRUE ;
	    }
	}
    }
  else
    {
      pos = keep ;
      result = FALSE ;
    }

  return result ;
}
 
/*************************************************/
 
static int ambiguouskey;
 
BOOL freekey (KEY *kpt, FREEOPT *options)
{
  unsigned char  *keep = pos ;

  if (!freeword())
    return FALSE ;

  if (freekeymatch ((char*) word, kpt, options))
    return TRUE;
 
  if (ambiguouskey)
    messout ("Keyword %s is ambiguous",word) ;
  else if (word[0] != '?')
    messout ("Keyword %s does not match",word) ;
 
  pos = keep ;
  return FALSE ;
}
 
/*****************/
 
BOOL freekeymatch (char *cp, KEY *kpt, FREEOPT *options)
{
  char  *io,*iw ;
  int   nopt = (int)options->key ;
  KEY   key ;
 
  ambiguouskey = FALSE;
  if (!nopt || !cp)
    return FALSE ;
 
  while (TRUE)
    { iw = cp ;
      io = (++options)->text ;

      while (freeupper (*iw++) == freeupper(*io++))
	if (!*iw)
	  goto foundit ;
      if (!--nopt)
        return FALSE ;
    }
 
foundit :
  key = options->key ;
 
  if (*io && *io != ' ')		/* not a full word match */
    while (--nopt)	/* check that later options are different */
      { io = (++options)->text ;
	iw = (char*) word ;
	while (freeupper (*iw++) == freeupper (*io++))
	  if (!*iw)
	    { ambiguouskey = TRUE;
	      return FALSE ;
	    }
      }
 
  *kpt = key ;
  return TRUE ;
}
 
/***************************************************/
  /* Return the text corresponding to the key */
char *freekey2text (KEY k, FREEOPT *o)  
{ int i = o->key ; char *title = o->text ;
  if (i<0)
    messcrash("Negative number of options in freekey2text") ;
  while (o++, i--) 
    if (o->key == k)
      return (o->text) ;
  return title ;
}

/***************************************************/
 
BOOL freeselect (KEY *kpt, FREEOPT *options)     /* like the old freemenu */
{
  if (isInteractive)
    printf ("%s > ",options[0].text) ;
  freecard (0) ;                       /* just get a card */
  if (isInteractive)
    while (freestep ('?'))            /* write out options list */
      { int i ;
	for (i = 1 ; i <= options[0].key ; i++)
	  printf ("  %s\n",options[i].text) ;
	printf ("%s > ",options[0].text) ;
	freecard (0) ;
      }
  return freekey (kpt,options) ;
}

/* same but returns TRUE, -1, if stremlevel drops below level */
BOOL freelevelselect (int level, KEY *kpt, FREEOPT *options)     /* like the old freemenu */
{
  if (isInteractive)
    printf ("%s > ",options[0].text) ;
  if (!freecard (level)) /* try to get another card */
    { *kpt = (KEY)(-1) ;  
      return TRUE ;                       
    }

  if (isInteractive)
    while (freestep ('?'))            /* write out options list */
      { int i ;
	for (i = 1 ; i <= options[0].key ; i++)
	  printf ("  %s\n",options[i].text) ;
	printf ("%s > ",options[0].text) ;
	if (!freecard (level)) /* try to get another card */
	 { *kpt = (KEY)(-1) ;  
	   return TRUE ;                       
	 }
       }
  return freekey (kpt,options) ;
} 

/**************************************/
 
BOOL freequery (char *query)
{
  if (isInteractive)
    { int retval, answer = 0 ;
      printf ("%s (y or n) ",query) ;
      answer = getchar () ;
      retval = (answer == 'y' || answer == 'Y') ? TRUE : FALSE ;
      while (answer != (unsigned char) EOF &&
	     answer != -1 && /* mieg: used not to break on EOF in pipes */
	     answer != '\n')
        answer = getchar () ;
      return retval ;
    }
  else
    return TRUE ;
}
 
/**********/
 
BOOL freeprompt (char *prompt, char *dfault, char *fmt)
{ 
  if (isInteractive)
    printf("%s ? > ",prompt);
  freecard (0) ;                       /* just get a card */
  if (freecheck (fmt))
    return TRUE ;
  else
    { messout ("input mismatch : format '%s' expected, card was\n%s",
	       fmt, card) ;
      return FALSE ;
   }
}
 
/*************************************/
 
int freefmtlength (char *fmt)
 
 {char *cp ;
  int length = 0 ;
 
  if (isdigit((int)*fmt))
   {
     sscanf (fmt,"%d",&length) ;
     return length ;
   }
 
  for (cp = fmt ; *cp ; ++cp)
    switch (*cp)
     {
     case 'i' : case 'f' : case 'd' : length += 8 ; break ;
     case 'w' : length += 32 ; break ;
     case 't' : length += 80 ; break ;
     case 'o' :
       if (*++cp)
	 messcrash ("'o' can not end free format %s",fmt) ;
       length += 2 ; break ;
     }
 
  if (!length)
    length = 40 ;
  
  return length ;
 }

/****************/
 
BOOL freecheck (char *fmt)
        /* checks that whatever is in card fits specified format
           note that 't' format option changes card by inserting a '"' */
{
  unsigned char *keep = pos ;
  union {int i ; float r ; double d ;}
  target ;
  char *fp ;
  unsigned char *start ;
  int nquote = 1 ;
  
  for (fp = fmt ; *fp ; ++fp)
    switch (*fp)
      {
      case 'w' : if (freeword ()) break ; else goto retFALSE ;
      case 'i' : if (freeint (&target.i)) break ; else goto retFALSE ;
      case 'f' : if (freefloat (&target.r)) break ; else goto retFALSE ;
      case 'd' : if (freedouble (&target.d)) break ; else goto retFALSE ;
      case 't' :      /* must insert '"' and escape any remaining '"'s or '\'s */
	for (start = pos ; *pos ; ++pos)
	  if (*pos == '"' || *pos == '\\')
	    ++nquote ;
	*(pos+nquote+1) = '"' ;		/* end of line */
	for ( ; pos >= start ; --pos)
	  {
	    *(pos + nquote) = *pos ;
	    if (*pos == '"' || *pos == '\\')
	      *(pos + --nquote) = '\\' ;
	  }
	*start = '"' ;
	goto retTRUE ;
      case 'z' : if (freeword ()) goto retFALSE ; else goto retTRUE ;
      case 'o' :
	if (!*++fp) messcrash ("'o' can not end free format %s",fmt) ;
	freestep (*fp) ; break ;
      case 'b' : break; /* special for graphToggleEditor no check needed  il */
      default :
	if (!isdigit((int)*fp) && !isspace((int)*fp))
	  messerror ("unrecognised char %d = %c in free format %s",
		     *fp, *fp, fmt) ;
      }
 
  retTRUE :
    pos = keep ; return TRUE ;
  retFALSE :
    pos = keep ; return FALSE ;
}
 
/************************ little routines ************************/
 
BOOL freestep (char x)
 {
   return (*pos && freeupper (*pos) == x && pos++) ;
 }
 
void freenext (void)
 {
   _losewhite ;
 }
 
char FREE_UPPER[] =
{
  0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,
  16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,
  32,33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,
  48,49,50,51,52,53,54,55,56,57,58,59,60,61,62,63,
  64,65,66,67,68,69,70,71,72,73,74,75,76,77,78,79,
  80,81,82,83,84,85,86,87,88,89,90,91,92,93,94,95,
  96,65,66,67,68,69,70,71,72,73,74,75,76,77,78,79,
  80,81,82,83,84,85,86,87,88,89,90,123,124,125,126,127
} ;

char FREE_LOWER[] =
{ 
  0,   1,   2,   3,   4,   5,   6,   7,   8,   9,  10,  11,  12,  13,  14,  15, 
  16,  17,  18,  19,  20,  21,  22,  23,  24,  25,  26,  27,  28,  29,  30,  31,  
  32,  33,  34,  35,  36,  37,  38,  39,  40,  41,  42,  43,  44,  45,  46,  47,  
  48,  49,  50,  51,  52,  53,  54,  55,  56,  57,  58,  59,  60,  61,  62,  63,  
  64,  97,  98,  99, 100, 101, 102, 103, 104, 105, 106, 107, 108, 109, 110, 111,
  112, 113, 114, 115, 116, 117, 118, 119, 120, 121, 122,  91,  92,  93,  94,  95,
  96,  97,  98,  99, 100, 101, 102, 103, 104, 105, 106, 107, 108, 109, 110, 111,
  112, 113, 114, 115, 116, 117, 118, 119, 120, 121, 122, 123, 124, 125, 126, 127
} ;

char* freepos (void)		/* cheat to give pos onwards */
{
  return (char*) pos ;
}

/****************** little arithmetic routines ************************/
/*   1.3 ->  1 ;  1.7 ->  2
 *  -1.3 -> -1 ; -1.7 -> -2
 */
int freeArrondi (float x)
{
  if (x >= 0)
    return (int) (x + 0.5) ;
  else
    return (int) (x - 0.5) ;
}

/********************************************/
 /* returns 1, 2, 5, 10, 20, 50, 100 etc 
  * in absolute value the returned number is smaller than p
  *   19 -> 10  ; -19 -> -10
  */
int freeMainPart (float p)
{
  register int i = 1, sign = 1;
  
  if (!p)
    return 0 ;
  if (p < 0) 
    { sign  = -1 ; p = -p ; }
  while(i <= p + .00001) 
    i = 10 * i;
  i /= 10 ;

  if (2 * i > p)
    return i * sign;
  if(5 * i > p)
    return 2 * i * sign;
  return
    5*i*sign;
}

/********************************************/
 /* returns 1, 2, 5, 10, 20, 50, 100 etc 
  * but the returned number may be a bit bigger than p
  *   19 -> 20  ; -19 -> -20
  */
int freeMainRoundPart (float p)
{
  register int i = 1, sign = 1;

  if (!p)
    return 0 ;
  if (p < 0) 
    {sign = -1 ; p = -p ; }	/* RD: ambiguous without spaces on SGI */
  while (i < p + .1) 
    i = 10 * i;
  if (4 * i < 5 * p) 
    return i * sign;
  i /= 2;
  if (4 * i < 5 * p) 
    return i * sign;
  i =  (2 * i) / 5;
  if (4 * i < 5 * p)
    return i * sign;
  i = i/2; 
  return i * sign;
}

/*************************************************************************/
/* returns 1 , 2 , 5 ,10 , 20, 50 ,100 etc */
double freeDoubleMainPart (double p)
{
  register double i = 1, sign = 1;

  if (!p)
    return 0.;
  if(p < 0) 
    {sign = -1; p = -p;}
  i = (double) exp(log((double)10.0) * (double)(1 + (int)(log10((double)p)))) ;
  if (4 * i < 5 * p) 
    return i * sign;
  i /= 2;
  if (4 * i < 5 * p)
    return i * sign;
  i = (2 * i) / 5;
  if (4 * i < 5 * p)
    return i*sign;
  i = i/2;
  return i*sign;
}

/*************************************************************************/
/*************************************************************************/

char* freeunprotect (char *text)
{
  static char *buf = 0 ;
  char *cp, *cp0, *cq ;
  messfree (buf) ;
  buf = strnew(text ? text : "", 0) ;

  /* remove external space and tabs and first quotes */
  cp = buf ;
  while (*cp == ' ' || *cp == '\t') cp++ ;
  if (*cp == '"') cp++ ;
  while (*cp == ' ' || *cp == '\t') cp++ ;

  cq = cp + strlen(cp) - 1 ;

  while (cq > cp && (*cq == ' ' || *cq == '\t')) *cq-- = 0 ;

  if (*cq == '"') /* remove one unprotected quote */
    {
      int i = 0 ; char *cr = cq - 1 ;
      while (cr > cp && *cr == '\\')
	{ i++ ; cr-- ; }
      if ( i%2 == 0)
	*cq-- = 0 ;  /* discard */
    }
  while (cq > cp && (*cq == ' ' || *cq == '\t')) *cq-- = 0 ;

  /* gobble the \ */
  cp0 = cq = cp-- ;
  while (*++cp)
    switch (*cp)
      {
      case '\\': 
	if (*(cp+1) == '\\') { cp++ ; *cq++ = '\\' ; break ;}
	if (*(cp+1) == '\n') { cp ++ ; break ; } /* skip backslash-newline */
	if (*(cp+1) == 'n') { cp ++ ; *cq++ = '\n' ; break ; }
	break ;
      default: *cq++ = *cp ;
      }
  *cq = 0 ;   /* terminate the string */
  return cp0 ;
}

char* freeprotect (char* text)	/* freeword will read result back as text */
{
  static Array a = 0 ;
  char *cp, *cq ;
  int base ;

		/* code to make this efficiently reentrant */

  if (a && text >= arrp(a,0,char) && text < arrp(a,arrayMax(a),char))
    { base = text - arrp(a,0,char) ;
      array (a, base+3*(1+strlen(text)), char) = 0 ; /* ensure long enough */
      text = arrp(a,0,char) + base ;            /* may have relocated */
      base += 1 + strlen(text) ;
    }
  else
    { a = arrayReCreate (a, 128, char) ;
      base = 0 ;
      array (a, 2*(1+strlen(text)), char) = 0 ; /* ensure long enough */
    }

  cq = arrp (a, base, char) ;
  *cq++ = '"' ;
  for (cp = text ; *cp ; *cq++ = *cp++)
    {
      if (*cp == '\\' || *cp == '"' ||			    /* protect these */
	  *cp == '/' || *cp == '%' || *cp == ';' ||
	  *cp == '\t' || *cp == '\n')
	*cq++ = '\\' ;
      if (*cp == '\n')					    /* -> /n/n (text then real) */
	{
	  *cq++ = 'n' ;
	  *cq++ = '\\' ;
	}
    }
  *cq++ = '"' ;
  *cq = 0 ;
  return arrp (a, base, char) ;
}

char* freejavaprotect (char* text)	/* freeword will read result back as text */
{
  static Array a = 0 ;
  char *cp, *cq ;
  int base ;

		/* code to make this efficiently reentrant */

  if (a && text >= arrp(a,0,char) && text < arrp(a,arrayMax(a),char))
    { base = text - arrp(a,0,char) ;
      array (a, base+3*(1+strlen(text)), char) = 0 ; /* ensure long enough */
      text = arrp(a,0,char) + base ;            /* may have relocated */
      base += 1 + strlen(text) ;
    }
  else
    { a = arrayReCreate (a, 128, char) ;
      base = 0 ;
      array (a, 2*(1+strlen(text)), char) = 0 ; /* ensure long enough */
    }

  cq = arrp (a, base, char) ;
  cp = text;
  while (*cp) 
    switch (*cp)
      {
      case '\n':
	*cq++ = '\\';
	*cq++ = 'n';
	cp++;
	break;
      case '\t':
	*cq++ = '\\';
	*cq++ = 't';
	cp++;
	break;
      case '\\': case '?':
	*cq++ = '\\' ;
	/* fall thru */
      default:
	*cq++ = *cp++;
      }
  *cq = 0 ;
  return arrp (a, base, char) ;
}


/* This routine would be better written as a general routine that takes an   */
/* array of chars and their replacement strings and then just does it, this  */
/* way it doesn't need to know anything about xml or java or anything....    */
/*                                                                           */
char* freeXMLprotect (char* text)	/* freeword will read result back as text */
{
  static Array a = 0 ;
  char *cp, *cq ;
  int base ;

  /* ACTUALLY I THINK THIS IS ALL RATHER HORRIBLE, IT JUST MAKES FOR COMPLEX */
  /* CODE THAT ISN'T GREAT....WHY NOT JUST RETURN A POINTER TO A STATIC      */
  /* AND HAVE PEOPLE COPY IT IF NECESSARY...OR ALWAYS RETURN A COPY AND      */
  /* PEOPLE HAVE TO THROW IT AWAY....                                        */
  /* code to make this efficiently reentrant */
  if (a && text >= arrp(a,0,char) && text < arrp(a,arrayMax(a),char))
    { base = text - arrp(a,0,char) ;
      array (a, base+7*(1+strlen(text)), char) = 0 ; /* ensure long enough */
      text = arrp(a,0,char) + base ;            /* may have relocated */
      base += 1 + strlen(text) ;
    }
  else
    { a = arrayReCreate (a, 128, char) ;
      base = 0 ;
      array (a, 6*(1+strlen(text)), char) = 0 ; /* ensure long enough */
    }

  cq = arrp (a, base, char) ;
  cp = text ;
  while (*cp) 
    switch (*cp)
      {
      case '<':
	strcat (cq, "&lt;") ; cq += 4 ;
	cp++;
	break;
      case '>':
	strcat (cq, "&gt;") ; cq += 4 ;
	cp++;
	break;
      case '"':
	strcat (cq, "&quot;") ; cq += 6 ;
	cp++;
	break;
      case '\'':
	strcat (cq, "&apos;") ; cq += 6 ;
	cp++;
	break;
      case '&':
	strcat (cq, "&amp;") ; cq += 5 ;
	cp++;
	break;
      case '\n':
	strcat (cq, "&amp;") ; cq += 5 ;
	cp++;
	break;

      default:
	*cq++ = *cp++;
      }
  *cq = 0 ;
  return arrp (a, base, char) ;
}




#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
/* A generalised "freeXXXprotect()" routine which doesn't need to know about */
/* java, xml etc. etc.                                                       */
/*                                                                           */
/*                                                                           */
/* Note that freeword() when used on the result will read it as straight text*/
/*                                                                           */
/* Efficiency will be a problem here, several things could be done:          */
/* - copy the translations into a new array which had the lengths of the     */
/*   protect strings already worked out, this would be faster, we could      */
/*   do memcpy's then.                                                       */
/* - forget this routine and just do the xml stuff with a switch.            */
/*                                                                           */
/*                                                                           */
char *freeCharProtect(char* text, Array translations)
{
  static Array a = 0 ;
  char *cp, *cq ;
  int base ;

  /* code to make this efficiently reentrant */
  /* DOES THIS ACTUALLY WORK FOR ALL CASES ?? I'm not so sure....            */
  /* OK, THIS WOULD HAVE TO BE REWRITTEN, THE CALCULATIONS DEPEND ON THE     */
  /* LENGTHS OF STUFF SUBSTITUED IN....                                      */

  if (a && text >= arrp(a,0,char) && text < arrp(a,arrayMax(a),char))
    {
      base = text - arrp(a,0,char) ;
      array (a, base+7*(1+strlen(text)), char) = 0 ; /* ensure long enough */
      text = arrp(a,0,char) + base ;            /* may have relocated */
      base += 1 + strlen(text) ;
    }
  else
    {
      a = arrayReCreate (a, 128, char) ;
      base = 0 ;
      array (a, 6*(1+strlen(text)), char) = 0 ; /* ensure long enough */
    }

  cq = arrp (a, base, char) ;
  cp = text ;
  while (*cp)
    {
      int i ;
      BOOL found ;

      for (i = 0, found = FALSE ; i < arrayMax(translations) ; i++)
	{
	  if (*cp == array(translations, i, ARRAYTYPE).char)
	    {
	      strcat (cq, array(translations, i, ARRAYTYPE).protect) ;
	      cq += strlen(array(translations, i, ARRAYTYPE).protect) ;
	      cp++;
	      found = TRUE ;
	    }
	}
      if (!found)
	*cq++ = *cp++;
    }

  *cq = '\0' ;						    /* string terminator. */

  return arrp (a, base, char) ;
}
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */



/*******************************************************/
/* mieg: i am putting here the non thread safe timesubs functions */

#include "mytime.h"

char *timeShowJava (mytime_t t) 
{
  static char ace_time[25] ;
  return timeShowJava2 (ace_time, t) ;
}

/*******/

char *timeShow (mytime_t t) 
{
  static char ace_time[25] ;
  
  return timeShow2 (ace_time, t) ;
}

/*******/

char *timeDiffShow (mytime_t t1, mytime_t t2) 
{
  static char buf[25] ;

  return timeDiffShow2 (buf, t1, t2) ;
}

/*******************************************************/
/*********** end of file *****************/
 
 
