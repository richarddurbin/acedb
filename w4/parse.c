/*  File: parse.c
 *  Author: Richard Durbin (rd@mrc-lmb.cam.ac.uk)
 *  Copyright (C) J Thierry-Mieg and R Durbin, 1992
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
 * along with this program; if n t, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 * or see the on-line version at http://www.gnu.org/copyleft/gpl.txt
 * -------------------------------------------------------------------
 * This file is part of the ACEDB genome database package, written by
 * 	Richard Durbin (MRC LMB, UK) rd@mrc-lmb.cam.ac.uk, and
 *	Jean Thierry-Mieg (CRBM du CNRS, France) mieg@kaa.cnrs-mop.fr
 *
 * Description: ace file parser
 * Exported functions: parseControl, parseFile, parseBuffer.
 * HISTORY:
 * Last edited: Mar 19 10:48 2009 (edgrif)
 * * Jul  6 16:16 2000 (edgrif): Fixed a number of bugs, reporting of
 *              all errors for pparse broken, reporting of final stats
 *              for xace "readall" parsing broken etc. etc.
 * * Jan  6 09:28 2000 (edgrif): Fix for SANgc03115.
 * * Oct 28 10:22 1999 (fw): avoid error when parsing class Tag
 *              which may still be present in ACE4 dump files.
 * * Oct 13 16:04 1999 (fw): parsing only possible from ACEIN
 *              this only leaves doParseLevel and gets rid of
 *              parsePipe, parseFile etc.
 * * May 12 15:19 1993 (mieg): added protected mode
 * * Apr 26 15:31 1993 (cgc): removed -T Top option
 * * Apr 25 23:25 1993 (cgc): added -R replace option: relpos, _bsHere
 * * Jun  5 16:18 1992 (mieg): added NON_GRAPHIC to parse in non-graphic mode
 * * Mar  4 02:48 1992 (rd): added -R option for renaming objects
 * Created: Sun Jan 12 17:52:35 1992 (rd)
 * CVS info:   $Id: parse.c,v 1.60 2009-03-19 10:49:08 edgrif Exp $
 *-------------------------------------------------------------------
 */

#include <wh/acedb.h>
#include <wh/aceio.h>
#include <wh/a.h>			/* for arraykill */
#include <wh/bs.h>
#include <wh/lex.h>
#include <whooks/systags.h>
#include <whooks/sysclass.h>
#include <wh/pick.h>
#include <wh/query.h>		/* for queryCheckConstraints() */
#include <wh/session.h>		/* for sessionUserKey() */
#include <wh/dbpath.h>
#include <w4/parse_.h>

/******** extra stuff for graphical parse-window **********/
/*                                                                           */
#ifndef NON_GRAPHIC
#include <wh/display.h>
#include <wh/main.h>
#include <wh/call.h>
#ifdef ACEDB_GTK
#include <wh/gex.h>
#endif /* ACEDB_GTK */

static Graph parseGraph = 0 ;

static void readAll (void);
static void parseDraw(void);
static void parseDispDestroy(void);
static void openFile(void);
static void closeFile(void);
static void ignoreErrors(void) ;
static void readItem(void);
static void readLine(void);
static void skipItem(void);
#ifdef ACEDB_GTK
static void parseDispEnterText(void);
#endif /* ACEDB_GTK */

static MENUOPT parseOptsOpen[] = { 
  { openFile, "Open ace File"},
#if defined (ACEDB_GTK)
  { parseDispEnterText,"Enter text"},
#endif /* UNIX */
  { graphDestroy, "Quit"},
  {0,0}
} ;

#define IGNORE_ERRORS_TXT  "Ignore Errors"
#define STOP_ERRORS_TXT    "Stop on Error"
#define ERROR_INDEX        1
static MENUOPT parseOptsClose[] = { 
  { closeFile, "Close File"},
  { ignoreErrors, IGNORE_ERRORS_TXT},
#if defined (ACEDB_GTK)
  { readAll,  "Read all"},	
  { graphForceInterrupt ,"Stop"},
#elif defined(MACINTOSH)
  { readAll,  "Read all"},			     /* MAC */
#elif defined(WIN32)
  { readAll,  "Read all (Use \"Esc\" to break in)"}, /* WINDOWS */
#else
  { readAll,  "Read all (hit key F4 to break in)"},  /* UNIX-X11 */
#endif /* UNIX */
  { readItem, "Read one Item"},
  { readLine, "Read one Line"},
  { skipItem, "Skip to next Item"},
  {0,0}
} ;
#endif /* !NON_GRAPHIC */



/***********************************************************************/
/*************      globals exported from this module    ***************/

ParseFuncType parseFunc[256] ;  /* == MAXTABLE of lexsubs.c */

/***********************************************************************/
/********** stuff local to the parser routine and initialiser **********/

static magic_t PF_MAGIC = "PF" ;
static char *PARSE_IN_SPECIAL = "\n/\\\t@"; /* \n - end-of-line
					     * // - comments
					     *  \ - escape char
					     * \t - 8 spaces
					     *  @ - commandfile */

typedef enum { DONE, OUTSIDE, INSIDE, REF_BLOCKED } ParseStateType;

#ifndef NON_GRAPHIC
typedef enum { READ_ALL=1, READ_ITEM, READ_LINE } ParseUpdateModeType;
#endif /* !NON_GRAPHIC */

/* Note that the enum and string version arrays must be kept in step.        */
typedef enum {PARSE_NO_UPDATE, PARSE_GENERAL_ERR, PARSE_ARRAY_ERR, PARSE_OBJECT_ERR} ParseErrType ;
char *parse_err_typestrings[4] = {"update", "general", "array", "object"} ;



typedef struct ParseStruct 
{
  magic_t *magic;					    /* == &PF_MAGIC */

  ACEIN parse_in ;
  char *filename;

  ParseStateType state ;				    /* if DONE it's not parsing and 
							       shouldn't have a parse_in */
  BOOL isError ;					    /* parse vs. pparse */

  /* Stats on object parsing:                                                */
  /*                                                                         */
  /*  nob = number of paragraphs (= objects) in the ace data input           */
  /*        (in theory   nob = nok + nerr)                                   */
  /*                                                                         */
  /*  nok = nadded + neditted + ndeleted + naliased + nrenamed               */
  /*        + narray_added + narray_empty + narray_deleted                   */
  /*                                                                         */
  /* nerr = ngen_err + nerror + narray_err                                   */
  /*                                                                         */
  /* I am treating arrays differently because its not really possible to     */
  /* say for all arrays whether they are new, edits of existing arrays or    */
  /* what...its a bit inconsistent at the moment.                            */
  BOOL full_stats ;					    /* TRUE means show everything. */
  /* total parse stats.                                                      */
  int nob ;
  int nok ;
  int nerr ;
  /* General errors                                                          */
  int ngen_err ;
  /* normal object stats                                                     */
  int nadded ;
  int neditted ;
  int ndeleted ;
  int naliased ;
  int nrenamed ;
  int nerror ;
  /* array object stats, note how semantics are different from normal obj.   */
  int narray_added ;
  int narray_empty ;
  int narray_deleted ;
  int narray_err ;


  char *obname  ;
  BOOL new_obj ;					    /* Is the object a new object */
  char *obj_firstline ;					    /* Keep record of first line of
							       ojb. for error messages. */
  int curr_line ;
  BOOL parseBusy ;
  BOOL doParseProtected;				    /* TRUE => doing a kernel parse. */

  BOOL parseKeepGoing ;					    /* Controls parsing behaviour: */
							    /* if FALSE stops parsing on 1st error */
  BOOL parseLineErrors ;				    /* Controls error output: */
							    /* if FALSE then individual line errors
							       are not reported. */
  BOOL parseQuiet ;					    /* TRUE => no message are output. */
  ACEOUT output ;					    /* alternative destination for error
							       messages. */

  /* It's possible for files to get extraneous null chars in them sometimes, */
  /* this will lead to premature end of parsing with no real warning to the  */
  /* user. We trap this by looking at file lengths.                          */
  BOOL has_length ;
  long int file_length ;
  long int file_pos ;

  /* the following are used only in parseLine() */
  int recursion ;
  BOOL isDelete, isRename, isAlias, isBang ;
  OBJ obj ;
  KEY  ref, timeStampKey ;
  BOOL save_error ;					    /* Signals error on trying to save
							       object after rename etc. */


#ifndef NON_GRAPHIC
  char *currentLineText;
  ParseUpdateModeType updateMode ; /* determines which parts of the
				    * display are updated during parse */
#endif /* !NON_GRAPHIC */
} *PF ;


/* defines how parseLine should procede next.                                */
typedef enum _ParseNext {PARSE_SKIP, PARSE_NOSKIP, PARSE_ABORT} ParseNext ;

static PF pfCreate (ACEIN parse_in, STORE_HANDLE handle);
static void pfFinalise (void *block);
#define pfDestroy(pf) messfree(pf)
static BOOL pfExists (PF pf);
static BOOL realParseAceIn(ACEIN parse_in, ACEOUT output, KEYSET ks,
			   BOOL keepGoing, BOOL full_stats, BOOL silent, BOOL parse_protected) ;
static BOOL doParseLevel (PF pf, KEYSET ks);
static BOOL parseArray(PF pf, KEY key, int classe, char **err_text) ;
static void parseLine(PF pf, ParseNext parse_type, KEYSET ks);
static BOOL saveObject(PF pf, KEYSET ks, char **errtext) ;


#ifndef NON_GRAPHIC
/*                                                                           */
/* Graphical routines, these should all be done by callback etc. etc. sigh...*/
/*                                                                           */

static magic_t ParseDisp_MAGIC = "ParseDisp" ;

typedef struct ParseDispStruct {
  magic_t *magic;		/* == &ParseDisp_MAGIC */
  PF pf;
  int buttonBox ;
  int  lineBox, itemBox, nparsedBox, nokBox, nerrorBox ;
  char itemText[64], lineText[64], nparsedText[64], nokText[64], nerrorText[64] ;
  char dirSelection[DIR_BUFFER_SIZE];
  char fileSelection[FIL_BUFFER_SIZE];
} *ParseDisp;

static ParseDisp currentParseDisp (char *caller);
static BOOL parseDispExists (ParseDisp pdisp);
static void showState(ParseDisp pdisp);
#endif /* !NON_GRAPHIC */




/******************************************************************
 ************************* public functions ***********************
 ******************************************************************/


/* The new parseAceIn function which allows the caller to specify an         */
/* alternative destination for output messages, this is especially needed    */
/* by the server.                                                            */
/* will destroy parse_in                                                     */
BOOL doParseAceIn(ACEIN parse_in, ACEOUT output, KEYSET ks, BOOL keepGoing, BOOL full_stats) 
{ 
  BOOL ok = FALSE ;
 
  ok = realParseAceIn(parse_in, output, ks, keepGoing, full_stats, FALSE, FALSE) ;
							    /* will destroy parse_in */
  return ok ;
} /* parseAceIn */


BOOL parseBuffer(char *text, KEYSET ks, BOOL keepGoing)
     /* convenience function to parse a text-buffer */
{
  BOOL ok = FALSE ;
  ACEIN parse_in;

  if (!text || !*text)
    messcrash("parseBuffer() - received NULL or empty text-pointer");

  parse_in = aceInCreateFromText (text, "", 0);

  ok = realParseAceIn(parse_in, NULL, ks, keepGoing, FALSE, FALSE, FALSE) ;
							    /* will destroy parse_in */
  return ok ;
} /* parseBuffer */


/* This variation is used by code that silently creates and parses some ace  */
/* data to fulfill some other function and hence does not want to display    */
/* the usual parse message.                                                  */
/* convenience function to parse a text-buffer */
BOOL parseBufferInternal(char *text, KEYSET ks, BOOL keepGoing)
{
  BOOL ok = FALSE ;
  ACEIN parse_in;

  if (!text || !*text)
    messcrash("parseBuffer() - received NULL or empty text-pointer");

  parse_in = aceInCreateFromText(text, "", 0);

  ok = realParseAceIn(parse_in, NULL, ks, keepGoing, FALSE, TRUE, FALSE) ;
							    /* will destroy parse_in */
  return ok ;
} /* parseBuffer */

/* This variation is used by AceC/acinside */
BOOL parseAceCBuffer (char *text, Stack s, ACEOUT given_out, KEYSET ks)
{
  BOOL ok = FALSE ;
  ACEIN parse_in ;
  ACEOUT parse_out ;
  BOOL keepGoing = TRUE ;

  if (!text || !*text)
    messcrash("parseBuffer() - received NULL or empty text-pointer");
  
  parse_in = aceInCreateFromText(text, "", 0);
  if (s)
    parse_out = aceOutCreateToStack (s, 0) ;
  else
    parse_out = given_out ;
  ok = realParseAceIn(parse_in, parse_out, ks, keepGoing, FALSE, TRUE, FALSE) ;
  /* will destroy parse_in */
  if (s)
    aceOutDestroy(parse_out) ;

  return ok ;
} /* parseAceCBuffer */


/* This old function has been recoded to be simply a call to the new         */
/* doParseAceIn function with the "output" parameter set to NULL.            */
/*                                                                           */
/* DON'T USE THIS FUNCTION, USE doParseAceIn INSTEAD.                        */
/*                                                                           */
/* will destroy parse_in */
BOOL parseAceIn(ACEIN fi, KEYSET ks, BOOL keepGoing)
{
  BOOL result ;

  result = realParseAceIn(fi, NULL, ks, keepGoing, FALSE, FALSE, FALSE) ;

  return result ;
}

/* this should be in the private header really....and be called uParse...etc.*/
     /* mustn't be called from user-code, only accessible to 
      * kernel routines */
     /* will destroy parse_in */
BOOL parseAceInProtected (ACEIN parse_in, KEYSET ks)
{
  BOOL ok = FALSE ;

  /* in the future we should check that parse_in is server-side,
   * so no user-data can be read into protected classes, only
   * files likes constraints.wrm which should only be accessible
   * by the server's administrator */

  ok = realParseAceIn(parse_in, NULL, ks, FALSE, FALSE, TRUE, TRUE) ;

  return ok ;
} /* parseAceInProtected */


/************************************************************/


/* This is the function that most of the above calls actually end up going   */
/* through, this stucture enables us to avoid callers having to set parse    */
/* state data.                                                               */
/* Will destroy parse_in                                                     */
/*                                                                           */
static BOOL realParseAceIn(ACEIN parse_in, ACEOUT output, KEYSET ks,
			   BOOL keepGoing, BOOL full_stats, BOOL silent, BOOL parse_protected)
{ 
  PF pf ;
  BOOL ok = FALSE ;
 
  /* Create our global parse control block and initialise it with callers    */
  /* parameters.                                                             */
  pf = pfCreate(parse_in, 0) ;
  pf->output = output ;
  pf->parseKeepGoing = keepGoing ;
  pf->full_stats = full_stats ;
  if (silent)
    {
      pf->parseLineErrors = FALSE ;
      pf->parseQuiet = TRUE ;
    }
  pf->doParseProtected = parse_protected ;

  ok = doParseLevel(pf, ks);

  pfDestroy (pf) ;

  return ok ;
} /* realParseAceIn */



#ifndef NON_GRAPHIC
/* Main function for xace version of file parsing.                           */
/*                                                                           */
void parseControl (void)
{ 
  if (!checkWriteAccess())
    return ;

  /* getMutex */
  if (graphActivate (parseGraph))
    graphPop () ;
  else
    {
      ParseDisp pdisp;
      STORE_HANDLE handle = handleCreate();

      parseGraph = displayCreate ("DtAce_Parser");

      pdisp = messalloc (sizeof(struct ParseDispStruct));
      pdisp->magic = &ParseDisp_MAGIC;
      pdisp->itemBox = 0;
      pdisp->lineBox = 0;
      pdisp->nparsedBox = pdisp->nokBox = pdisp->nerrorBox = 0 ;
      if (getenv ("ACEDB_DATA"))
	strcpy (pdisp->dirSelection, getenv("ACEDB_DATA")) ;
      else if (dbPathStrictFilName("rawdata", "", "", "rd", handle))
	strcpy (pdisp->dirSelection,
		dbPathStrictFilName ("rawdata", "", "", "rd", handle)) ;
      else			/* set to root db-dir */
	strcpy (pdisp->dirSelection, dbPathMakeFilName("", "", "", handle));

      handleDestroy(handle);

      graphAssociate (&ParseDisp_MAGIC, pdisp);
      
      graphTextBounds (60, 20) ; /* needed for text box sizing */
      graphRegister (DESTROY, parseDispDestroy) ;
      
      parseDraw () ;
    }
  /* releaseMutex */
  return ;
} /* parseControl */

#else  /* NON_GRAPHIC */
/* currently the gifaceserver links in the .ng.o version of this file
 * but the rest of the gifaceserver is graphical code, so we have to
 * supply two stubs here to make the link possible. All this is horrible
 * and needs cleaning up */
/*void parseOneFile (FILE *fil, KEYSET ks) {;}*/
void parseControl (void) {;}
#endif  /* NON_GRAPHIC */


/******************************************************************
 ************************ private functions ***********************
 ******************************************************************/

/* constructor */
static PF pfCreate (ACEIN parse_in, STORE_HANDLE handle)
{
  PF pf;

  pf = (PF) halloc (sizeof (struct ParseStruct), handle) ;
  blockSetFinalise (pf, pfFinalise);
  pf->magic = &PF_MAGIC ;
  pf->doParseProtected = FALSE;	/* mustn't be overridden by normal usercode */

  /* All these can be overridden to change error reporting behaviour.        */
  pf->parseKeepGoing = FALSE ;
  pf->output = NULL ;
  pf->parseLineErrors = TRUE ;
  pf->parseQuiet = FALSE ;

  pf->full_stats = FALSE ;
  pf->nob = pf->nok = pf->nerr
    = pf->ngen_err
    = pf->nadded = pf->neditted = pf->ndeleted = pf->naliased = pf->nrenamed = pf->nerror
    = pf->narray_added = pf->narray_empty = pf->narray_err = pf->narray_deleted
    = 0 ;

  pf->state = DONE ;

  pf->save_error = FALSE ;

  pf->parse_in = parse_in;
  aceInSpecial (pf->parse_in, PARSE_IN_SPECIAL);
  pf->filename = strnew (aceInGetURL(parse_in), 0);

  pf->curr_line = 0 ;

  /* Can we take the length of the stream ?                                  */
  if (aceInStreamLength(pf->parse_in, &(pf->file_length)))
    pf->has_length = TRUE ;
  else
    pf->has_length = FALSE ;
  pf->file_pos = 0 ;
  
  return pf;
} /* pfCreate */

static void pfFinalise (void *block)
{
  PF pf = (PF)block;

  if (!pfExists(pf))
    messcrash("pfFinalise() - received invalid block pointer");

  if (pf->state != DONE)
    parseLine(pf, PARSE_ABORT, 0); /* closes open object & ace-file */

  messfree(pf->filename);
  messfree(pf->obname);
  messfree(pf->obj_firstline) ;

#ifndef NON_GRAPHIC
  messfree (pf->currentLineText);
#endif /* !NON_GRAPHIC */

  pf->magic = 0 ;

  return ;
} /* pfFinalise */

static BOOL pfExists (PF pf)
{
  if (pf && pf->magic == &PF_MAGIC)
    return TRUE;

  return FALSE;
} /* pfExists */


/************************************************************/
/* does all parsing of array objects, these are special in that they do not  */
/* appear in the models file but may be attached to bits of models data via  */
/* keys with the same name as the class to which they are attached.          */
/* All array stats incrementing is done here, note that error stats are      */
/* recorded by calling routine.                                              */
/*                                                                           */
static BOOL parseArray(PF pf, KEY key, int classe, char **err_text)
{ 
  BOOL result = FALSE ;

  if (classe <= 0 || classe >= 256)
    messcrash ("bad class %d passed to parseArray", classe) ;

  if (parseFunc[classe])
    {
      /* a ParseFunc is registered */
      ParseFuncReturn parsing ;

      *err_text = NULL ;
      parsing = parseFunc[classe](pf->parse_in, key, err_text) ;
      if (parsing == PARSEFUNC_OK)
	{
	  /* we will need to update the stats here...rather than in the caller */
	  /* unless we return the parse result from here as well...            */
	  /* BUT if we have seperate array stats we could just do them here.   */
	  result = TRUE ;
	  pf->narray_added++ ;
	}
      else if (parsing == PARSEFUNC_EMPTY)
	{
	  result = TRUE ;
	  pf->narray_empty++ ;
	}
      else
        {
	  /* On failure, skip to end of object and report failure.           */
	  while (aceInCard(pf->parse_in) && aceInWord(pf->parse_in)) ;

	  /* cop out, really the functions should always return an error     */
	  /* string if they fail.                                            */
	  if (*err_text == NULL)
	    *err_text = "registered function for parse of array data failed" ;

	  pf->narray_err++ ;
	  result = FALSE ;
	}

      aceInSpecial (pf->parse_in, PARSE_IN_SPECIAL);	    /* parseFunc may change the special */
    }
  else
    { 
      /* no ParseFunc registered, object not empty so skip over it. */
      while (aceInCard(pf->parse_in) && aceInWord(pf->parse_in)) ;

      *err_text = messprintf("No parser available for array class %d", classe) ;
      pf->narray_err++ ;
      result = FALSE ;
    }

  return result ;
} /* parseArray */


/*******************************************/

/* deals with one entry */
/* uummmm, this is not quite right, actually we dive in and out of this      */
/* routine to parse a single entry.                                          */
/*                                                                           */
/* This routine calls certain other routines (lexDoAlias, graph stuff) that  */
/* are not re-entrant and so must not be called again if the initial call to */
/* routine results in recursion. All such calls should be guarded with       */
/* pf->recursion.                                                            */
/*                                                                           */
static void parseLine(PF pf, ParseNext parse_type, KEYSET ks)
{
  int   table, ix ;
  float fx ; 
  mytime_t maDate ;
  char  *word, dummyChar; 
  char *card = 0;
  KEY   key, tag, type, relpos ;
  char *errtext = 0 ;
  char *objname = NULL ;
#ifndef NON_GRAPHIC
  ParseDisp pdisp = currentParseDisp("parseLine");
#endif /* !NON_GRAPHIC */
#ifndef ACEDB4
  extern BOOL READING_MODELS ;
#endif
  ParseErrType err_type = PARSE_OBJECT_ERR ;		    /* commonest err is for non-array
							       objects. */

  pf->isError = FALSE ;


  /* Clear after error or skip.                                              */
  /*                                                                         */
  if (parse_type == PARSE_ABORT)
    {
      /* Current behaviour is to save an empty object if there is some tag   */
      /* error or something minor...don't konw if this is really a good idea.*/
      goto done ;
    }
  else if (parse_type == PARSE_SKIP)
    {
      if (pf->state == DONE)
        goto done ;
      if (pf->state == OUTSIDE)
	return ;
      err_type = PARSE_NO_UPDATE ;
      goto abort ;
    }


  /*---------------------  Process an object  -------------------------------*/
  /*                                                                         */
  switch (pf->state)
    { 
    /* Ace file is done (perhaps because of error)                         */
    case DONE :
      goto done ;


    /* Look for next object                                                */
    case OUTSIDE :

      /* Find the next paragraph, i.e. the next potential object.            */
      pf->isBang = FALSE ;
      messfree(pf->obj_firstline) ;
      while (TRUE)
	{ 
#ifndef NON_GRAPHIC
	  messfree (pf->currentLineText);
#endif /* !NON_GRAPHIC */


	  /* Record where we are in stream, note that by the time we get to  */
	  /* here the file may already have been closed, which case we can't */
	  /* take its length so we cheat and make look like we got to the    */
	  /* end. This is because of the aceIn packages horrible implicit    */
	  /* close for aceInCard() when we reach the end of a file.          */
	  if (pf->has_length)
	    {
	      if (pf->file_pos < pf->file_length)
		{
		  if (!(aceInStreamPos(pf->parse_in, &(pf->file_pos))))
		    pf->file_pos = pf->file_length ;
		}
	    }

	  /* Note, the logic here is a bit tricky, if this fails then we are */
	  /* already at the end of the file and it has already been closed   */
	  /* due to the yucky implicit close that the aceIn package does.    */
	  if (!(card = aceInCard(pf->parse_in)))
	    goto done ;


#ifndef NON_GRAPHIC
	  /* keep the current parse line for showState */
	  if (pf->updateMode == READ_LINE)
	    pf->currentLineText = strnew (card, 0);
#endif /* !NON_GRAPHIC */

	  pf->obj_firstline = strnew(card, 0) ;

	  if (!pf->isBang)
	    {
	      word = aceInWord(pf->parse_in) ;
	      if (word && *word != '!')			    /* Found a paragraph */
		break ;
	      if (word && *word == '!')			    /* jump paragraph */
		pf->isBang = TRUE ;
	    }
	  else
	    {
	      word = aceInWord(pf->parse_in) ;
	      if (!word)
		pf->isBang = FALSE ;  /* end of jumped paragraph */
	    }
	}

      /* If we got here we must have found a potential object.               */
      (pf->nob)++ ;

      /* Special processing for a delete/rename/alias.                       */
      pf->isDelete = pf->isRename = pf->isAlias = FALSE ;

      if (strcmp (word, "-D") == 0)      /* symbol for deletion */
	{
	  pf->isDelete = TRUE ;
	  if (!(word = aceInWord(pf->parse_in)))
	    { 
	      errtext = "No class after -D" ;
	      err_type = PARSE_GENERAL_ERR ;
	      goto abort ;
	    }
	}
      else if (strcmp (word, "-R") == 0) /* symbol for renaming */
	{
	  pf->isRename = TRUE ;
	  if (!(word = aceInWord(pf->parse_in)))
	    { 
	      errtext = "No class after -R" ;
	      err_type = PARSE_GENERAL_ERR ;
	      goto abort ;
	    }
	}
      else if (strcmp (word, "-A") == 0) /* symbol for aliasing */
	{
	  pf->isAlias = pf->isRename = TRUE ;
	  if (!(word = aceInWord(pf->parse_in)))
	    { 
	      errtext = "No class after -A" ;
	      err_type = PARSE_GENERAL_ERR ;
	      goto abort ;
	    }
	}


      /* Get hold of the objects class and name.                             */
      /*                                                                     */
      messfree(pf->obname);
      pf->obname = NULL ;

      if (*word == '>')
	/* recognize > as fasta format for DNA */
	{ 
	  table = pickWord2Class("DNA") ;

	  /* the FASTA-object name can appear directly glued 
	   * to the > character, or in the next word */
	  if (*(word + 1))
	    objname = word + 1 ;
	  else
	    objname = aceInWord (pf->parse_in) ;
	}
      else
	{
	  table = pickWord2Class(word) ; /* word is the class name */
          /* note - Could search classes to parse subclasses */
	  
	  if ((pickList[table].type != 'A') && (pickList[table].type != 'B'))
	    { 
	      errtext = messprintf ("Bad class name : %s", word ? word : "(NULL)") ;
	      err_type = PARSE_GENERAL_ERR ;
	      goto abort ;
	    }
	  
	  if (!table)
	    { 
	      errtext = messprintf("Unrecognised Class %s ", word) ;
	      err_type = PARSE_GENERAL_ERR ;
	      goto abort ;
	    }
	  
	  if (pickList[table].protected && !pf->doParseProtected
#ifdef ACEDB4
	      /* in ACEDB4 only, we avoid complaints about parsing the Tag class */
	      && strcmp(name(pickList[table].classe), "Tag") != 0
#endif
	      )
	    {
	      errtext = messprintf("Trying to read/alter a protected class %s", word) ;
	      err_type = PARSE_GENERAL_ERR ;
	      goto abort ;
	    }
	  
#ifdef ACEDB4
	  /* old models */
	  if (pickList[table].type == 'B') /* a kludge to check existence of the model */
	    {
	      if ((pf->obj = bsCreate (KEYMAKE (table,0))))
		bsDestroy (pf->obj) ;
	      else
		{ 
		  errtext = messprintf ("Missing model of class %s", word) ;
		  goto abort ;
		}
	    }
#else 
	  /* new models */
	  if (!READING_MODELS && pickList[table].type == 'B' && !pickList[table].model)
	    { 
	      errtext = messprintf ("Missing model of class %s", word) ;
	      goto abort ;
	    }
#endif /* ACEDB4 */
	  
	  aceInStep(pf->parse_in, ':') ;               /* optional colon after class */

	  /* get the object name */
	  objname = aceInWord(pf->parse_in) ;
	}

      if (!lexIsGoodName(objname))
	{
	  errtext = messprintf("Object name %s not valid",
			       objname ? objname : "(NULL)") ;
	  err_type = PARSE_GENERAL_ERR ;
	  goto abort ;
	}


      /* we now have the class in 'table' and the object name in objname */

      /* secure object name for showState */
      pf->obname = strnew (objname, 0);


      /* Deal with rename or alias.                                          */
      if (pf->isRename)
	{
	  pf->ref = 0 ;

	  if (!lexword2key (pf->obname, &pf->ref, table))
	    {
	      errtext = messprintf("Object %s not found in database", pf->obname) ;
	      goto abort ;
	    }
	  messfree(pf->obname) ;

	  /* Replace copy of original object name with copy of new name.     */
	  if (!(pf->obname = aceInWord(pf->parse_in)))
	    {
	      errtext = messprintf("No new name for %s",
				   pf->isAlias ? "alias (-A)" : "rename (-R)") ;
	      goto abort ;
	    }
	  else
	    {
	      pf->obname = strnew(pf->obname, 0) ;
	    }


#ifndef NON_GRAPHIC
	  switch (pf->updateMode)
	    {
	    case READ_ALL:
	      if (!((pf->nob)%100))
		showState (pdisp) ;
	      if (!((pf->nob)%5000))
		graphRedraw () ;
	      break ;
	    case READ_ITEM: 
	    case READ_LINE:
	      showState (pdisp);
	      break ;
	    default:
	      break ;
	    }
#endif /* !NON_GRAPHIC */

	  {
	    char *err_msg = NULL ;
	    KEY new_key = KEY_NULL ;
	    BOOL key_is_obj = TRUE ;

	    (pf->recursion)++ ;
	    if (pf->recursion >= 2)
	      messcrash ("Multiple recursive call to lexalias inside parser") ;

	    /* If an alias or an empty object the key is known to the system, record this
	     * so we don't try to add it again after rename. */
	    if (pf->isAlias || (lexword2key(pf->obname, &new_key, table) && iskey(new_key) == LEXKEY_IS_VOC))
	      key_is_obj = FALSE ;

	    /* Do the rename or alias. */
	    if (!lexDoAlias(pf->ref, word, FALSE, pf->isAlias, &err_msg))
	      {
		errtext = messprintf("%s, %s",
				     (pf->isAlias ? "Alias failed" : "Rename failed"),
				     err_msg) ;
		messfree(err_msg) ;
		(pf->recursion)-- ;

		goto abort ;
	      }

	    /* If the new name did not previously exist as an alias or an object
	     * then after the rename we have a new object with a _new_ key and so
	     * need to update pf->ref to reflect this. */
	    if (key_is_obj)
	      {
		pf->ref = KEY_NULL ;

		if (!lexword2key(pf->obname, &pf->ref, table))
		  {
		    messcrash("%s, cannot get a key for new object \"%s\" in database",
			      (pf->isAlias ? "Alias failed" : "Rename failed"),
			      pf->obname) ;
		  }
	      }

	    /* Make sure object is saved and keyset updated.                 */
	    /* I'm really not sure if its OK/necessary to save a             */
	    /* renamed/aliased object, it all seems to come out in the wash..*/
	    if (!(saveObject(pf, ks, &errtext)))
	      goto abort ;

	    if (pf->isAlias)
	      (pf->naliased)++ ;
	    else
	      (pf->nrenamed)++ ;

	    pf->state = OUTSIDE ;
	    (pf->recursion)-- ;
	  }

	  return ;
	}

      /* Deal with a delete.                                                 */
      if (pf->isDelete)
	{
	  if (!lexword2key(pf->obname, &pf->ref, table))
	    {
	      errtext = messprintf("Object %s not found in database", pf->obname) ;
	      goto abort ;
	    }
	  else
	    {
	      if (lexDestroyAlias (pf->ref))		    /* true if this is an alias */
		{
		  /* Don't need to do anything here, just destroy the alias.     */
		}
	      else
		{
		  switch (pickType(pf->ref))
		    {
		    case 'A':
		      arrayKill(pf->ref) ;
		      (pf->narray_deleted)++ ;
		      break ;
		    case 'B':
		      if ((pf->obj = bsUpdate(pf->ref)))
			bsKill(pf->obj) ;
		      (pf->ndeleted)++ ;
		      break ;
		    default:
		      errtext = "Can't handle because unknown class type" ;
		      err_type = PARSE_GENERAL_ERR ;
		      goto abort ;
		    }
		}
	      
	      pf->state = OUTSIDE ;
	      return ;
	    }
	}



      /* OK, must be a 'new' object definition.                              */
      /*                                                                     */

      /* generate a key (pf->ref) for the object (pf->obname), which might   */
      /* be a new object to the database.                                    */
      pf->new_obj = lexaddkey(pf->obname, &(pf->ref), table) ;

#ifndef NON_GRAPHIC
      /* just read another object - redraw screen */
      switch (pf->updateMode)
	{
	case READ_ALL:
	  if (!((pf->nob)%100))
	    showState (pdisp) ;
	  if (!((pf->nob)%5000))
	    graphRedraw () ;
	  break ;
	case READ_ITEM:
	case READ_LINE:
	  showState (pdisp);
	  break ;
	default:
	  break ;
	}
#endif /* !NON_GRAPHIC */

      /* read timeStamp of new object */
      pf->timeStampKey = 0;
      word = aceInWord(pf->parse_in);
      if (word && (strcmp(word, "-O") == 0 ))
	{
	  if (!(word = aceInWord(pf->parse_in)))
	    { 
	      errtext = "No timestamp after -O" ;
	      goto abort ;
	    }
	  lexaddkey(word, &(pf->timeStampKey), _VUserSession);
	}


      switch (pickType (pf->ref))
	{ 
	case 'A':
	  if (ks)
	    keySet (ks, keySetMax (ks)) = pf->ref ;
	  if (parseArray(pf, pf->ref, table, &errtext))
	    return ;
	  else
	    {
	      err_type = PARSE_ARRAY_ERR ;
	      goto abort ;
	    }
	case 'B':
	  break ;		/* handled by REF_BLOCKED */

	default:
	  errtext = "Can't handle because unknown class type" ;
	  goto abort ;
	}
      pf->state = REF_BLOCKED ;
      /* note fall through */

    case REF_BLOCKED :		
      if (!(pf->obj = bsUpdate (pf->ref)))
	{
	  errtext = messprintf ("Object %s is locked - free before continuing",
				pf->obname) ;
	  goto abort ;
	}
      pf->state = INSIDE ;
      return ;


    /* Carry on processing current object.                                 */
    case INSIDE :
#ifndef NON_GRAPHIC
      messfree (pf->currentLineText);
#endif /* !NON_GRAPHIC */

      /* Record where we are in stream, must do this before aceInCard    */
      /* because aceInCard closes file on EOF and we lose the position.  */
      if (pf->has_length)
	{
	  if (pf->file_pos < pf->file_length)
	    aceInStreamPos(pf->parse_in, &(pf->file_pos)) ;
	}

      pf->curr_line = aceInStreamLine(pf->parse_in) - 1 ;


      /* File finished, so save object and clear up.                         */
      if (!(card = aceInCard(pf->parse_in)))
	{

	  if (saveObject(pf, ks, &errtext))
	    {
	      if (pf->new_obj)
		(pf->nadded)++ ;
	      else
		(pf->neditted)++ ;
	      goto done ;
	    }
	  else
	    goto abort ;
	}

#ifndef NON_GRAPHIC
      /* keep the current parse line for showState */
      if (pf->updateMode == READ_LINE)
	pf->currentLineText = strnew (card, 0);
#endif /* !NON_GRAPHIC */

      /* blank line, so save object and go on to next one.                   */
      if (!(word = aceInWord (pf->parse_in)))
	{ 
	  if (!(saveObject(pf, ks, &errtext)))
	    goto abort ;

	  if (pf->new_obj)
	    (pf->nadded)++ ;
	  else
	    (pf->neditted)++ ;

	  pf->state = OUTSIDE ;
	  return ;
	}
      

      /* More of object to process.                                          */
      if (*word == '!')
	return ;

      /* the 'word' is a tag name */

      if (strcmp (word, "-D") == 0)	/* symbol for deletion */
	{ 
	  pf->isDelete = TRUE ;
	  if (!(word = aceInWord(pf->parse_in)))
	    {
	      errtext = "No tag after -D" ;
	      goto abort ;
	    }
	}
      else
	pf->isDelete = FALSE ;

      if (!lexword2key (word,&tag,0)) /* find tag */
	{ 
	  errtext = messprintf ("Unknown tag \"%s\"",word) ;
	  goto abort ;
	}

      aceInNext(pf->parse_in);
      aceInStep (pf->parse_in, ':') ; /* optional colon after tag */

      bsGoto(pf->obj, 0) ;		/* pop out of subobjs
					 * return to obj-root */

      if (bsAddTag (pf->obj,tag))	/* subobjects come on the same line */
	while ((word = aceInWord (pf->parse_in)))
	  { 
	  scanSubObj:
	    relpos = _bsRight ;
	    if (*word == '-' && strlen(word) == 2)
	      switch (word[1])
		{
		case 'C':	/* standard comment */
		  if (!(word = aceInWord(pf->parse_in)))
		    {
		      errtext = "No Comment after -C" ;
		      goto abort ;
		    }
		  bsAddComment (pf->obj, word, 'C') ;
		  if (!(word = aceInWord(pf->parse_in)))
		    continue ;
		  if (strcmp(word,"-O") != 0) /* only a timestamp possible */
		    {
		      errtext = "Extra item on -C line" ;
		      goto abort ;
		    }

		  /* NOTE: fall-through if -O after -C */

		case 'O':	/* timestamp (UserSession class) */
		  if (!(word = aceInWord(pf->parse_in)))
		    { 
		      errtext = "No UserSession key after -O" ;
		      goto abort ;
		    }
		  lexaddkey (word, &key, _VUserSession) ;
		  bsSetTimeStamp (pf->obj, key) ;
		  continue ;

		case 'R':	/* Replace */
		  if (!(word = aceInWord(pf->parse_in)))
		    continue ;
		  relpos = _bsHere ;
		}

	    if (!(type = bsType (pf->obj, relpos)))
	      { 
		errtext = messprintf ("tag \"%s\" : %s - off end of model\n", 
				      name (tag), word) ;
		goto abort ;
	      }

	    if (class(type) == _VQuery)
	      break ;
#ifndef ACEDB4
		/* new models */
	    if (class(type) == _VModel) /* a type */
#else  /* ACEDB4 */
		/* old models */
	    if (class(type) && type == KEYMAKE(class(type), 1)) /* a type */
#endif /* ACEDB4 */
	      {
		if (!bsPushObj(pf->obj))
		  { 
		    errtext = messprintf ("tag \"%s\" : %s - off end of model, "
					  "addObj failed\n", name(tag), word) ;
		    goto abort ;
		  }
		goto scanSubObj ;
              }
	      
#ifndef ACEDB4
	    if (class(type) && (table = type, pickIsA(&table, 0), table))
#else /* ACEDB4 */
	    if ((table = class(type)))
#endif /* ACEDB4 */
	      { 
		if (!lexIsGoodName(word)) /* refuse blank strings: "    " */
		  {
		    errtext = messprintf ("Blank object names names are illicit, sorry") ;
		    goto abort ;
		  }
		if (pickKnown (table) &&
		    !lexword2key (word,&key,table))
		  {
		    errtext = messprintf ("tag \"%s\" : class: \"%s\" %s - unknown. "
					  "You can not create an "
					  "entry in a \"Known\" class in an indirect way (see "
					  "options.wrm)\n",
					  name(tag), pickClass2Word(table), word) ;
		    goto abort ;
		  }
		lexaddkey (word,&key,table) ;
		bsAddKey (pf->obj,relpos,key) ;
	      }
	    else if (type == _Int)
	      {
		if (sscanf (word,"%d%c",&ix,&dummyChar) == 1)
		  bsAddData (pf->obj,relpos,_Int,&ix) ;
		else
		  { 
		    errtext = messprintf ("%s not an integer after %s\n", word, name(tag)) ;
		    goto abort ;
		  }
	      }
	    else if (type == _Float)
	      {
		double bigfloat = 0 ;
		if (sscanf (word,"%lf%c",&bigfloat,&dummyChar) == 1 &&
		    bigfloat < 1e40 && bigfloat > -1e40 )
		  {  
		    char buf[128] ;
		    
		    if  (bigfloat <= ACE_FLT_MIN  && bigfloat >= -ACE_FLT_MIN)
		      bigfloat = 0 ;
		    sprintf (buf, "%1.7g", bigfloat) ; sscanf (buf, "%f", &fx) ;
		    bsAddData (pf->obj,relpos,_Float,&fx) ;
		  }
		else
		  { 
		    errtext = messprintf ("%s not a float after %s\n",
					  word,name(tag)) ;
		    goto abort ;
		  }
	      }
	    else if (type == _DateType)
	      {
		if ((maDate = timeParse (word)))
		  bsAddData (pf->obj,relpos,_DateType,&maDate) ;
		else
		  { 
		    errtext = messprintf ("%s not a DateType after %s\n",
					  word,name(tag)) ;
		    goto abort ;
		  }
	      }
	    else if (type < _LastC)
	      bsAddData (pf->obj,relpos,type,word) ;
	    else          /* a tag */
	      {
		if (!lexword2key (word,&tag,0)) /* find tag */
		  {
		    errtext = messprintf ("Unknown tag \"%s\"",word) ;
		    goto abort ;
		  }
		if (!bsAddTag(pf->obj, tag))
		  { 
		    errtext = messprintf ("Tag \"%s\" not in object",word) ;
		    goto abort ;
		  }
	      }
	  }
      else
	{ 
	  errtext = messprintf ("Tag \"%s\" does not fit model\n", name(tag)) ;
	  goto abort ;
	}
      
      if (pf->isDelete)
	bsPrune (pf->obj) ;
			       
      bsGoto(pf->obj, 0) ;				    /* move obj-cursor back to root */

      return ;

    }
  /* end-switch pf->state */
  /*---------------------  Process an object (End)  -------------------------*/



  /* End processing if we hit an error or file is finished.                  */
  /*                                                                         */
abort:
  /* Report parsing errors:                                                  */
  /* Note that errors are always logged. It's possible to turn off error     */
  /* reporting, don't know if this is ever used.                             */
  if (errtext)
    {
      char *out_msg = NULL ;


      /* Prepare the message, print at most 50 chars of obj_firstline        */
      out_msg = hprintf(0, "%s parse error, near line %d while parsing \"%.50s\", error was: %s", 
			parse_err_typestrings[err_type], aceInStreamLine(pf->parse_in)-1,
			pf->obj_firstline,
			errtext) ;

      /* If we have an output stream then send errors to it, rather than     */
      /* messerror, e.g. server uses this to send messages back to client.   */
      if (pf->output)
	{
	  aceOutPrint(pf->output, "// %s\n",  out_msg) ;    /* N.B. the "// ", message could be
							       going back to the client. */

	  messdump(out_msg) ;
	}
      else
	{
	  if (pf->parseKeepGoing && !pf->parseLineErrors)
	    messdump(out_msg) ;
	  else
	    messerror(out_msg) ;			    /* N.B. messerror DOES a messdump. */
	}

      messfree(out_msg) ;
    }

  /* If save_error = TRUE it means we have just tried to save an object but  */
  /* failed, this means there is no need to skip to the end of the current   */
  /* object. Otherwise we do need to skip to the next object.                */
  if (!pf->save_error)
    {
      while (aceInCard(pf->parse_in))
	{
	  char *first_word ;

	  /* Object definitions/edits must be separated by blank lines,      */
	  /* rename, delete or alias do not have to be.                      */
	  if ((first_word = aceInWord(pf->parse_in)))
	    {
	      if ((strcmp(first_word, "-R") == 0)
		  || (strcmp(first_word, "-D") == 0)
		  || (strcmp(first_word, "-A") == 0))
		{
		  aceInCardBack(pf->parse_in) ;
		  break ;
		}
	    }
	  else
	    break ;
	}
    }

  bsDestroy (pf->obj) ; 
  pf->state = OUTSIDE ;

  if (!pf->parseKeepGoing)
    pf->isError = TRUE ;

  /* parseArray() increments array stats, so only do errors for non-arrays.  */
  if (err_type == PARSE_GENERAL_ERR)
    (pf->ngen_err)++ ;
  else if (err_type == PARSE_OBJECT_ERR)
    (pf->nerror)++ ;

#ifndef NON_GRAPHIC
  showState (pdisp) ;
#endif /* NON_GRAPHIC */

  return ;


 done:
  {
    saveObject(pf, ks, &errtext) ;


    /* finish this stream */
    aceInDestroy (pf->parse_in);			    /*   aceInClose (pf->fi, pf->level) ;*/
    pf->state = DONE ;					    /* must come before showState */

    /* Now that we're done we report the final state to the user */
    /* Note, the recursion is private to the code, not for the end user */
#ifndef NON_GRAPHIC		/* GRAPHIC */
    if (!pf->recursion)
      parseDraw () ;
#else  /* NON_GRAPHIC */

    /* NOTHING TO DO FOR NON-GRAPHICAL CODE, see doParseLevel                */

#endif /* NON_GRAPHIC */

    /* If parsing finished successfully we still need to check if it stopped */
    /* part way through a file, this can happen if a NULL char has crept into*/
    /* a file (e.g. because of an NFS glitch). NOTE if parse_in is pipe/stdin*/
    /* etc. we can't take its length, if file_pos == -1, this says there was */
    /* an error in finding position in file. Fix for SANgc03115              */
    /* Note that this check is impossible to do correctly under cygwin,      */
    /* since the length and pos counts are affected by the translation of    */
    /* line endings from <CR><LF> to <LF>. Since this check is really for    */
    /* NFS problems, which are irrelevant under windows, the check is not    */
    /* done under CYGWIN.                                                    */
#ifndef __CYGWIN__   

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
    /* I have completely disabled this for now because of the following      */
    /* problem:                                                              */
    /* 
       The problem with the check is that it will fail if the last line in the input is
       not terminated with a "\n" because in this case the file/text is closed and the
       stream level decremented without the caller being able to get the final length
       of file/text parsed. In our case we do try to get the length but because the
       streamlevel is decremented to 0 by then, we get length zero returned for the
       file. This then screws up the below test. We could do a new aceInDoCard() that
       takes a BOOL param to specify whether to close the file implicitly. aceInCard()
       then become a macro which expanded to a call to aceInDoCard()

       This is a basic problem with the acein interface really. The only way round it  
       is to provide a variant of aceInCard that allows one to specify that an implicit
       close is not done. This would be fairly easy to do but not a high priority right
       now I think.
    */

    if (parse_type != PARSE_ABORT && pf->has_length)
      {
	if (pf->file_length != pf->file_pos)
	  {
	    messerror("Although parsing was successful, it stopped before the end of the file. "
		      "This may be because NULL characters have accidentally "
		      "been introduced into the file, these will cause the parser to think "
		      "it has reached the end of the file and stop. "
		      "File was %ld bytes long, parsing stopped after %ld bytes.",
		      pf->file_length, pf->file_pos) ;
	  }
      }
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

#endif /* !__CYGWIN__ */

    return ;
  }



  return ;
} /* parseLine */


/****************************************************************/
/* If an object has been renamed, editted or added, it needs to be saved,    */
/* these are the steps to do it.                                             */
/* Returns TRUE if the save could be done, FALSE if constaint checks failed. */
static BOOL saveObject(PF pf, KEYSET ks, char **errtext)
{
  BOOL result = FALSE ;

  if (pickList[class(pf->ref)].constraints &&
      !queryCheckConstraints (pf->obj, pf->ref, pickList[class(pf->ref)].constraints))
    { 
      *errtext = messprintf ("Object %s does not satisfy constraints", name (pf->ref)) ;
      pf->save_error = TRUE ; /* prevent jumping next object */

      result = FALSE ;
    }
  else
    {
      /* add the object we've just parsed into the user-supplied keyset,     */
      /* this makes the "Active Objects" correct.                            */
      if (ks)
	keySet (ks, keySetMax (ks)) = pf->ref ;

      bsSave (pf->obj) ;
      
      /* Only set the creation timestamp from the acefile if the object was created this session */
      if (pf->timeStampKey && (lexCreationUserSession(pf->ref) == sessionUserKey()))
	lexSetCreationUserSession(pf->ref, pf->timeStampKey);
      
      result = TRUE ;
    }

  return result ;
}


/****************************************************************/

static BOOL doParseLevel(PF pf, KEYSET ks)
{
  BOOL result = FALSE ;
#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
  int active_obj = 0 ;					    /* see comments below. */
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */
  char *parse_msg = NULL ;

  if (!pfExists(pf))
    messcrash("doParseLevel() - received invalid pf pointer");

  pf->nob = pf->nok = pf->nerr
    = pf->ngen_err
    = pf->nadded = pf->neditted = pf->ndeleted = pf->naliased = pf->nrenamed = pf->nerror
    = pf->narray_added = pf->narray_empty = pf->narray_err  = pf->narray_deleted
    = 0 ;

  pf->state = OUTSIDE ;

  do {
    parseLine (pf, PARSE_NOSKIP, ks);
  } while (!pf->isError
	   && pf->state != REF_BLOCKED
	   && pf->state != DONE);

  if (pf->state == DONE)
    {
      result = TRUE ;
    }
  else
    {
      result = FALSE ;

      /* closes open object & ace-file */
      pf->state = DONE ;
      parseLine (pf, PARSE_ABORT, ks) ;
    }

  /* Summary of processing, note that for some processing, active_obj could  */
  /* be zero because ks is NULL, or it could be zero because no objects got  */
  /* put in the active keyset. Currently this all comes out in the wash,     */
  /* because ks is only NULL for non-user parsing (e.g. models file).        */
  if (ks)
    {
      keySetSort(ks) ;
      keySetCompress (ks) ;

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
      /* I'm not sure we want the active object count, its displayed anyway  */
      /* at the end of each command.                                         */
      /* It can always be included later by uncommenting this and including  */
      /* the count in the stats below.                                       */
      active_obj = keySetCountVisible(ks) ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */
    }


  /* some global stats are derived from sub-stats.                           */
  pf->nok = pf->nadded + pf->neditted + pf->ndeleted + pf->nrenamed + pf->naliased
            + pf->narray_added + pf->narray_empty + pf->narray_deleted ;
  pf->nerr = pf->ngen_err + pf->nerror + pf->narray_err ;

  if (pf->full_stats)
    parse_msg
      = hprintf(0,
		"total processed: %d found, %d parsed ok, %d parse failed\n"
		"        general: %d errors\n"
		"        objects: %d added, %d editted, %d deleted, %d renamed, %d aliased, %d errors\n"
		"         arrays: %d added, %d empty, %d deleted, %d errors\n", 
		pf->nob, pf->nok, pf->nerr,
		pf->ngen_err,
		pf->nadded, pf->neditted, pf->ndeleted, pf->nrenamed, pf->naliased, pf->nerror,
		pf->narray_added, pf->narray_empty, pf->narray_deleted, pf->narray_err) ;
  else
    parse_msg = hprintf(0, "objects processed: %d found, %d parsed ok, %d parse failed\n",
			pf->nob, pf->nok,  pf->nerr) ;

  /* Only output the message if its _not_ a kernel parse (for model reading.)*/
  if (!(pf->parseQuiet))
    {
      if (pf->output)
	aceOutPrint(pf->output, "// %s\n", parse_msg) ;
      else
	messout(parse_msg) ;
    }

  messfree(parse_msg) ;
  
  return result ;
} /* doParseLevel */



/*************** graphical functions ***********************/

#ifndef NON_GRAPHIC
static ParseDisp currentParseDisp (char *caller)
{
  ParseDisp pdisp = 0;

  if (!graphActivate (parseGraph))
    return NULL;

  graphAssFind (&ParseDisp_MAGIC, &pdisp) ;

  if (!pdisp)
    return NULL;

  if (!parseDispExists(pdisp))
    messcrash("%s() - found non-magic ParseDisp on graph", caller);

  return pdisp;
} /* currentParseDisp */

static BOOL parseDispExists (ParseDisp pdisp)
{
  if (pdisp && pdisp->magic == &ParseDisp_MAGIC)
    return TRUE;

  return FALSE;
} /* parseDispExists */

static void readAll (void)
     /* parse-window function */
{ 
  ParseDisp pdisp = currentParseDisp("readAll");
  BOOL orig_parseLineErrors ;

  if (!pdisp || !pdisp->pf || pdisp->pf->parseBusy)
    return;
  
  pdisp->pf->parseBusy = TRUE;
  parseDraw();						    /* redraw screen in its busy state */

  pdisp->pf->updateMode = READ_ALL ;

#ifdef ACEDB_GTK
  graphBusyCursorAll();
#else  /* !ACEDB_GTK */
  graphBusyCursorAll(TRUE);
#endif /* !ACEDB_GTK */

  graphSetBlockMode(GRAPH_BLOCKING);

  /* Don't report multiple errors for graphical parsing (user can instead    */
  /* step through from error to error).                                      */
  orig_parseLineErrors = pdisp->pf->parseLineErrors ;
  pdisp->pf->parseLineErrors = FALSE ;

  pdisp->pf->isError = pdisp->pf->parseKeepGoing ;

  do
    {
      parseLine (pdisp->pf, PARSE_NOSKIP, 0);
    }
  while (!messIsInterruptCalled() 
	 && !pdisp->pf->isError
	 && pdisp->pf->state != REF_BLOCKED
	 && pdisp->pf->state != DONE) ;
  
  graphSetBlockMode(GRAPH_BLOCKABLE);

  pdisp->pf->parseLineErrors = orig_parseLineErrors ;
  pdisp->pf->parseBusy = FALSE;

  parseDraw ();						    /* redraw screen in its idle state */

  /* are we finished totally ?? */
  if (pdisp->pf->state == DONE)
    pfDestroy (pdisp->pf);


  return;
} /* readAll */

static void readItem (void)
     /* parse-window function */
{ 
  ParseDisp pdisp = currentParseDisp("readItem");

  if (!pdisp || !pdisp->pf 
      || pdisp->pf->parseBusy)
    return;

  pdisp->pf->parseBusy = TRUE;

  pdisp->pf->updateMode = READ_ITEM ;
  do {
    parseLine (pdisp->pf, PARSE_NOSKIP, 0);
  } while (pdisp->pf->state == INSIDE) ;

  /* start parsing next item, so we have already the name to report
   * in the display */
  parseLine (pdisp->pf, PARSE_NOSKIP, 0);

  pdisp->pf->parseBusy = FALSE;

  /* are we finished totally ?? */
  if (pdisp->pf->state == DONE)
    pfDestroy (pdisp->pf);

  parseDraw ();

  return;
} /* readItem */

static void readLine (void)
{
  ParseDisp pdisp = currentParseDisp("readLine");

  if (!pdisp || !pdisp->pf 
      || pdisp->pf->parseBusy)
    return;

  pdisp->pf->parseBusy = TRUE;

  pdisp->pf->updateMode = READ_LINE ;
  parseLine (pdisp->pf, PARSE_NOSKIP, 0);

  pdisp->pf->parseBusy = FALSE;

  /* are we finished totally ?? */
  if (pdisp->pf->state == DONE)
    pfDestroy (pdisp->pf);

  /*  parseDraw ();*/

  return;
} /* readLine */

static void skipItem (void)
     /* parse-window function */
{
  ParseDisp pdisp = currentParseDisp("skipItem");

  if (!pdisp || !pdisp->pf 
      || pdisp->pf->parseBusy)
    return;

  pdisp->pf->parseBusy = TRUE;

  pdisp->pf->updateMode = READ_LINE ;
  parseLine (pdisp->pf, PARSE_SKIP, 0); /* skip one item */
  parseLine (pdisp->pf, PARSE_NOSKIP, 0);

  pdisp->pf->parseBusy = FALSE;

  return;
} /* skipItem */

#ifdef ACEDB_GTK
static void parseDispEnterText(void)
     /* parse-window function */
{
  ParseDisp pdisp = currentParseDisp("parseDispEnterText");
  char *buff;

  if (!pdisp ||
      /* only refuse to parseDispEnterText,
       * if an existing parse-action is busy */
      (pdisp->pf && pdisp->pf->parseBusy))
    return;

  buff = gexTextEditor("Type text to be read", NULL, 0);

  if (buff)
    {
      parseBuffer(buff, 0, FALSE);
      messfree (buff);
    }

  return;
} /* parseDispEnterText */
#endif /* ACEDB_GTK */

static void openFile (void)
     /* parse-window function */
{
  ACEIN parse_in;
  ParseDisp pdisp = currentParseDisp("openFile");

  if (!sessionGainWriteAccess())
    { 
      /* may occur if somebody else grabbed it */
      messerror ("You no longer have write access!");
      graphDestroy () ;
      return ;
    }

  if (!pdisp ||
      /* only refuse to openFile,
       * if an existing parse-action is busy */
      (pdisp->pf && pdisp->pf->parseBusy))
    return;

  /* destroy the old parse-actions, because we'll make a new one */
  if (pdisp->pf)
    pfDestroy (pdisp->pf);

  /* write access may be lost doing a save */

  parse_in =
    aceInCreateFromChooser ("Which ace file do you wish to parse ?",
			    pdisp->dirSelection,
			    pdisp->fileSelection, 
			    "ace", "r", 0);
  if (!parse_in)
    return ;

  pdisp->pf = pfCreate (parse_in, 0);
  aceInSpecial (pdisp->pf->parse_in, PARSE_IN_SPECIAL) ;

  messdump ("Parsing file %s", pdisp->pf->filename) ;

  pdisp->pf->state = OUTSIDE ;

  pdisp->pf->nob = pdisp->pf->nok = pdisp->pf->nerror
    = pdisp->pf->ngen_err
    = pdisp->pf->nadded = pdisp->pf->neditted = pdisp->pf->ndeleted = pdisp->pf->naliased = pdisp->pf->nrenamed
    = pdisp->pf->narray_added = pdisp->pf->narray_empty = pdisp->pf->narray_err = pdisp->pf->narray_deleted
    = 0 ;

  parseDraw () ;

  return;
} /* openFile */

static void closeFile (void)
     /* parse-window function */
{
  ParseDisp pdisp = currentParseDisp("closeFile");

  if (!pdisp || (pdisp->pf && pdisp->pf->parseBusy))
    return;

  if (pfExists(pdisp->pf))
    pfDestroy (pdisp->pf);	/* kill current parse-action */

  parseDraw () ;

  return;
} /* closeFile */


/* Toggle error state.                                                       */
static void ignoreErrors(void)
{
  ParseDisp pdisp = currentParseDisp("parseDispEnterText");

  if (pdisp && pdisp->pf && !(pdisp->pf->parseBusy))
    {
      pdisp->pf->parseKeepGoing = !(pdisp->pf->parseKeepGoing) ;

      if (pdisp->pf->parseKeepGoing)
	parseOptsClose[ERROR_INDEX].text = STOP_ERRORS_TXT ;
      else
	parseOptsClose[ERROR_INDEX].text = IGNORE_ERRORS_TXT ;
      
      pdisp->buttonBox = graphButtons (parseOptsClose, 1, 3, 55) ;
      if (!pdisp->pf->parseBusy)
	graphBoxClear (pdisp->buttonBox + 3);		    /* remove Stop button */
      graphRedraw() ;
    }

  return ;
}


/********************************************/

static void parseDraw (void)
     /* parse-window function */
{ 
  int line = 9 ;
  ParseDisp pdisp = currentParseDisp("parseDraw");

  if (!pdisp)
    return;

  graphPop () ;
  graphClear () ;

  if (pfExists(pdisp->pf))
    /* we're parsing at the moment */
    {
      if (pdisp->pf->filename && strlen(pdisp->pf->filename) > 0)
	/* show name of file being parsed */
	{ 
	  graphText ("Filename:", 1, 1);
	  graphText (pdisp->pf->filename, 11,1);
	}

      pdisp->buttonBox = graphButtons (parseOptsClose, 1, 3, 55) ;
      if (!pdisp->pf->parseBusy)
	graphBoxClear (pdisp->buttonBox + 3);		    /* remove Stop button */
    }
  else
    /* we're not parsing at the moment */
    pdisp->buttonBox = graphButtons (parseOptsOpen, 1, 3, 55) ;

  graphText ("  Item:",1,line) ;	
  pdisp->itemBox = graphBoxStart () ;
  graphTextPtr (pdisp->itemText, 9, line, 64) ;
  graphBoxEnd () ;

  line += 1.5 ;
  graphText ("  Line:",1,line) ;
  pdisp->lineBox = graphBoxStart () ;
  graphTextPtr (pdisp->lineText, 9, line, 64) ;
  graphBoxEnd () ;

  line += 1.5 ;
  graphText ("Parsed:",1,line) ;
  pdisp->nparsedBox = graphBoxStart () ;
  graphTextPtr (pdisp->nparsedText, 9, line, 64) ;
  graphBoxEnd () ;

  line += 1.5 ;
  graphText ("    OK:",1,line) ;
  pdisp->nokBox = graphBoxStart () ;
  graphTextPtr (pdisp->nokText, 9, line, 64) ;
  graphBoxEnd () ;

  line += 1.5 ;
  graphText ("Errors:",1,line) ;
  pdisp->nerrorBox = graphBoxStart () ;
  graphTextPtr (pdisp->nerrorText, 9, line, 64) ;
  graphBoxEnd () ;

  showState (pdisp) ;

  graphRedraw () ;

  return;
} /* parseDraw */

/***************************************/

static void parseDispDestroy (void)	/* called when parseGraph dies */
     /* parse-window function */
{
  ParseDisp pdisp = currentParseDisp("parseDispDestroy");
  VoidRoutine old ;
  parseGraph = 0 ;

  if (pfExists(pdisp->pf))
    pfDestroy (pdisp->pf);	/* also closes file etc. */

  pdisp->magic = 0;
  messfree (pdisp);

  /* The new data may have influenced the way the main-window
   * is drawn, so we simulate a change in writeAccess
   * to redraw - if the update has added longtext objects
   * (where there were no before) this is useful, to make
   * the [even in longtexts] button appear */
  if ((old = writeAccessChangeRegister(0)))
    (old)() ;

  return;
} /* parseDispDestroy */

/********************************************/

static void showState (ParseDisp pdisp)
     /* parse-window function */
{
  PF pf ;

  if (!parseDispExists(pdisp))
    return;

  pf = pdisp->pf ;					    /* shortcut to pf */

  /* N.B. we always show the state even when we've finished parsing because  */
  /* otherwise the window goes blank at the end of parsing, infuriating for  */
  /* the user.                                                               */

  if (!pfExists(pf))
    strcpy (pdisp->itemText, "");
  else
    strcpy (pdisp->itemText,
	    messprintf ("%d  %.40s",
			pf->nob,
			pf->obname ? pf->obname : "")) ;
  graphBoxDraw (pdisp->itemBox, BLACK, WHITE) ;

  if (!pfExists(pf))
    strcpy (pdisp->lineText, "");
  else
    { 
      long int pos;
      Stack s = stackCreate (64);

      if (pf->state != DONE)
	{
	  pushText (s, messprintf ("%d", aceInStreamLine (pf->parse_in)-1));

	  if (pf->file_length && aceInStreamPos (pf->parse_in, &pos))
	    {
	      float percentage = (100.0 * pos) / pf->file_length;
	      catText (s, messprintf (" (%2.0f%%)", percentage));
	    }
	}
      else
	{
	  pushText (s, messprintf ("%d", pf->curr_line)) ;

	  if (pf->has_length)
	    {
	      float percentage ;

	      percentage = (100.0 * pf->file_pos) / pf->file_length;
	      catText (s, messprintf (" (%2.0f%%)", percentage));
	    }
	}


      if (pf->currentLineText)
	{
	  catText (s, " ");
	  catText (s, pf->currentLineText);
	}

      strncpy (pdisp->lineText, stackText(s, 0), 64);
      pdisp->lineText[64-1] = '\0';

      stackDestroy (s);
    }
  graphBoxDraw (pdisp->lineBox, BLACK, WHITE) ;


  /* Show some stats on parsing, currently we only show the totals, we could */
  /* show everything if required...                                          */
  /*                                                                         */
  if (!pfExists(pf))
    {
      strcpy(pdisp->nparsedText, "") ;
      strcpy(pdisp->nokText, "") ;
      strcpy(pdisp->nerrorText, "") ;
    }
  else
    {
      pf->nok = pf->nadded + pf->neditted + pf->ndeleted + pf->nrenamed + pf->naliased
	+ pf->narray_added + pf->narray_empty + pf->narray_deleted ;
      pf->nerr = pf->ngen_err + pf->nerror + pf->narray_err ;

      strcpy(pdisp->nparsedText, messprintf("%d", pf->nob)) ;
      strcpy(pdisp->nokText, messprintf("%d", pf->nok)) ;
      strcpy(pdisp->nerrorText, messprintf("%d", pf->nerr)) ;
    }
  graphBoxDraw(pdisp->nparsedBox, BLACK, WHITE) ;
  graphBoxDraw(pdisp->nokBox, BLACK, WHITE) ;
  graphBoxDraw(pdisp->nerrorBox, BLACK, WHITE) ;


  graphProcessEvents ();

  return;
} /* showState */
#endif  /* !NON_GRAPHIC */


/************************************************************/
/************************** end of file *********************/
/************************************************************/
