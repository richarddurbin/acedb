/*  File: dump.c
 *  Author: Jean Thierry-Mieg (mieg@mrc-lmb.cam.ac.uk)
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
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 * or see the on-line version at http://www.gnu.org/copyleft/gpl.txt
 * -------------------------------------------------------------------
 * This file is part of the ACEDB genome database package, written by
 * 	Richard Durbin (MRC LMB, UK) rd@mrc-lmb.cam.ac.uk, and
 *	Jean Thierry-Mieg (CRBM du CNRS, France) mieg@kaa.cnrs-mop.fr
 *
 * Description:
 * Exported functions:
 * HISTORY:
 * Last edited: Mar  6 12:47 2009 (edgrif)
 * * Apr 20 16:18 1999 (edgrif): Remove dump window if user aborts.
 * * Aug 18 10:00 1998 (rd):
 *	     - split dumpPossible() into lexIsKeyVisible() and dumpClassPossible
 *	     - remove dumpKey1() as unnecessary
 * * Aug  3 11:29 1998 (rd): 
 *           - user can dump to any directory. Also changed the error format for
 *           - errors form dump  to class \t name \t error mess.
 * * Apr  3 16:18 1997 (rd)
 * * Jun 21 17:05 1992 (mieg): arranged  so the dump file can be read back
 * Created: Sun Feb 16 01:31:41 1992 (mieg)
 * CVS info:   $Id: dump.c,v 1.78 2009-11-16 10:03:14 edgrif Exp $
 *-------------------------------------------------------------------
 */

#include <setjmp.h>
#include <wh/acedb.h>
#include <wh/aceio.h>
#include <wh/dbpath.h>
#include <wh/lex.h>
#include <wh/a.h>
#include <wh/bs.h>
#include <wh/dump.h>
#include <wh/pick.h>
#include <wh/session.h>
#include <wh/query.h>
#include <whooks/sysclass.h>
#include <whooks/classes.h>
#include <wh/java.h>

#ifndef NON_GRAPHIC
#include <wh/graph.h>
#include <wh/display.h>
#endif



/******************************************************************/
/****************** globals used elsewhere ************************/
/******************************************************************/

DumpFuncType dumpFunc[256] ;


static BOOL dumpKey2(ACEOUT dump_out, KEY key, char style, COND cond) ;


/****************** internal globals ******************************/

/* Naming patterns for dump files, used to contruct dump file names, if the  */
/* dump is of a single class then the classname is included in the filename. */
/* The regex's could be made more exact but these are probably good enough.  */
/*                                                                           */
#define DUMPNAME        "%sdump_%s_%c.%d"
#define DUMPNAME_CLASS  "%sdump_%s_%c_%s.%d"
#define DUMPNAME_REGEXP "dump_*_*.1.ace"
#define DUMPALLNAME_REGEXP "dump_*_*.*.ace"


/* Dump size limits in terms of objects or bytes.                            */
/* lower bound. Most systems limit files to 2GB, hence this is 1 GB */
#define MAX_OBJECTS_IN_DUMP 20000
#define MAX_BYTES_IN_DUMP 100000000L



static Array doNotDumpKeySet = NULL ;

static char dumpDir[DIR_BUFFER_SIZE] = "";         /* use this to choose where to dump too */
static BOOL dumpSetDirectory (void);

static Array trapKeys = NULL, trapMessages = NULL ;

#ifndef NON_GRAPHIC

static MENUOPT dumpMenu[]=
{
  { graphDestroy,"Quit"},
  { help,"Help"},
  { 0,0}
};

static Graph dumpGraph = 0;

#endif /* NON_GRAPHIC */



/*********************************************/
/************* public routine ****************/
/*********************************************/



/* Small utility useful for debugging, you just insert dumpObj() calls were you need to see them,
 * obj is dumped to stdout. */
void dumpObj(OBJ obj)
{
  ACEOUT std_out ;

  std_out = aceOutCreateToStdout(0) ;

  dumpKeyBeautifully(std_out, bsKey(obj), 'h', 0) ;
		    
  aceOutDestroy(std_out) ;

  return ;
}



/* RD 980817: split dumpPossible() into dumpKeyPossible() and
   dumpClassPossible().  I want dumpKeyPossible to be used for all key
   listing, e.g. in keysetdisplay, list and show commands in tace.
   dumpClassPossible will be used by the complete dump code.

   NB I allow protected classes (including XREF and Model classes) to
   be seen by dumpKeyPossible(), so we can see them in keysets and
   display them, and also list and show them from tace.  But they
   still fail dumpClassPossible() so will not be in full dumps.  This
   is a change.  Is that OK?

   RD 980819: dumpKeyPossible() is the same as lexIsKeyVisible(), not
   surprisingly, so I supressed it.
*/

BOOL dumpClassPossible (int classe, char style)
{
  classe &= 0xff ;

  if (pickList[classe].type == 'A')
    return dumpFunc[classe] ? TRUE : FALSE ;

  if (pickList[classe].private)  /* private to kernel, nobody's business  */
    return FALSE ;

  /* protected classes can not be parsed, so can't be dumped in ace style 
   *  XREF classed are created on the fly by the kernel
   */
  if ((style == 'a' || style == 'A' || style == 0) && 
      ( pickXref(classe) || pickList[classe].protected))
    return FALSE ;

  if (!lexMaxVisible(classe))
    return FALSE ;

  if (classe == _VUserSession && !dumpTimeStamps)
    /* the UserSession objects are meaningless
     * without timestamps */
    return FALSE;

  return TRUE ;
} /* dumpClassPossible */

/*********************************************************/

BOOL dumpKeyBeautifully (ACEOUT dump_out, KEY key, char style, COND cond) 
{
  return dumpKey2 (dump_out, key, style, cond) ;
}

/*********************************************************/

BOOL dumpKey (ACEOUT dump_out, KEY key) 
{
  return dumpKey2 (dump_out, key, 0, 0) ;
}


/*************** Internal routines ***********************/

static BOOL dumpKey2 (ACEOUT dump_out, KEY key, char style, COND cond)
{ 
  OBJ obj ;
  int classe = 0, oldc = 0 ;
  BOOL notEmpty = FALSE ;

  if (doNotDumpKeySet && keySetFind (doNotDumpKeySet, key, 0))
    return FALSE ;

  if (!lexIsKeyVisible (key))
    return FALSE ;

		/* test for tag now, to avoid writing an object name line.  
		   cond is a compiled query built in the calling routine
		   queryFind3 uses bIndexFind() and only makes obj if necessary
		*/
  obj = 0 ;
  if (cond && !queryFind3 (cond, &obj, key))
    { if (obj) bsDestroy (obj) ;
      return FALSE ;
    }
      
  if (pickType(key) == 'A' && !(style == 'j' || style == 'J' || style == 'k' || style == 'K' || style == 'C'))
    style = 0 ;			/* can only dump arrays in .ace and java and AceC */

				/* write object name line for .ace files */
  if (style == 0 || style == 'a' || style == 'A')
    {
      char *tsPref="", *tsVal="", *tsPost="";
      
      if (dumpTimeStamps && lexCreationUserSession(key))
	{
	  tsPref = " -O \"";
	  tsPost = "\"";
	  tsVal = name(lexCreationUserSession(key));
	}
      notEmpty = TRUE ;

      aceOutPrint (dump_out, "%s : %s%s%s%s\n",	   
		   className(key), freeprotect (name(key)),
		   tsPref, tsVal, tsPost) ;
    }

  switch (pickType (key))
    {
    case 'A': 
      switch (style)
	{
	case 'C':
          {
          classe = class(key) ;
          notEmpty = TRUE ;
          if (iskey(key) == 2)
            {
              if (classe == _VDNA)
                {  dnaDumpKeyCstyle (dump_out, key); }
              else if (classe == _VPeptide)
                 { peptideDumpKeyCstyle (dump_out, key); }
            }
          else
            {
              char buf[2] ;
              char *cp = name(key) ;

              buf[0] = 'n' ; buf[0] = class(key) ;      /* bug? */
	      aceOutBinary( dump_out, buf, 2 );
	      aceOutBinary( dump_out, cp, strlen(cp)+1);
              buf[0] = '#' ;
	      aceOutBinary( dump_out, buf, 1 );
            }
          break ;
	  }
	case 'j': case 'J':   /* java dumping */
	case 'k': case 'K':   /* fixed up 'java' dumping for AcePerl */
	  classe = class(key) ;
	  if (iskey(key) == 2)
	    {
	      if (classe == _VDNA)
		{ javaDumpDNA (dump_out, key); }
	      else if (classe == _VPeptide)
		{ javaDumpPeptide (dump_out, key); }
	      else if (classe == _VKeySet)
		{ notEmpty = TRUE ; javaDumpKeySet (dump_out, key); }
	      else if (classe == _VLongText)
		{ notEmpty = TRUE ; javaDumpLongText (dump_out, key); }
	    }
	  else
	    switch (style)
	      { 
	      case 'j': case 'k':
		notEmpty = TRUE ; 
		aceOutPrint (dump_out, "?%s?%s?\n", className(key),
			     freejavaprotect(name(key))) ;
		break ;
	      case 'J': case 'K':
		notEmpty = TRUE ; 
		if (oldc != classe)
		  { 
		    oldc = classe ;
		    aceOutPrint (dump_out, "?%s", className (key)) ;
		  }
		else
		  aceOutPrint (dump_out, "#", className (key)) ;
		if (iskey(key)==2)
		  aceOutPrint (dump_out, "?%s?\n",
			       freejavaprotect(name(key))) ;
		else
		  aceOutPrint (dump_out, "!%s?\n",
			       freejavaprotect(name(key))) ;
	      default:
		break ;
	      }
	  break ;
	    default:
	  if (dumpFunc[class(key)])
	    {
	      if (iskey (key)==2)
		notEmpty = TRUE ;
	      dumpFunc[class(key)] (dump_out, key) ;
	    }
	  break ;
	}
      break ;
    case 'B':
      if (obj || (obj = bsCreate (key)))
	{
	  notEmpty = TRUE ;
	  switch (style)
	    { 
	    case 0: case 'a': case 'A': /* ace format */
	      if (class(key) == _VModel)
		{
		  aceOutPrint (dump_out, "// a model can not be dumped in .ace format\n") ;
		}
	      else
		bsAceDump (obj, dump_out, (cond != 0) ? TRUE : FALSE) ;
	      break ;
	    default:
	      niceDump (dump_out, obj, style) ;
	      break ;
	    }
	}
      else 
	switch (style)
	  { 
	  case 'j': case 'k':
	    notEmpty = TRUE ; 
	    aceOutPrint (dump_out, "?%s?%s?\n",className(key),
			 freejavaprotect(name(key))) ;
	    break ;

	  case 'J': case 'K':
	    notEmpty = TRUE ; 
	    if (oldc != classe)
	      { 
		oldc = classe ;
		aceOutPrint (dump_out, "?%s", className (key)) ;
	      }
	    else
	      aceOutPrint (dump_out, "#", className (key)) ; /* RD 000615 - what is className(key) here for? */
	    if (iskey(key)==2)
	      aceOutPrint (dump_out, "?%s?\n",
			   freejavaprotect(name(key))) ;
	    else
	      aceOutPrint (dump_out, "!%s?\n",
			   freejavaprotect(name(key))) ;
	    break;

	  case 'C':
            notEmpty = TRUE ;
            {
              char buf[2] ;
              char *cp = name(key) ;

              buf[0] = 'n' ; buf[1] = class(key) ;
              aceOutBinary( dump_out, buf, 2) ;
              aceOutBinary( dump_out, cp, strlen(cp) + 1) ;
              buf[0] = '\n' ;
              buf[1] = '#' ;
              aceOutBinary ( dump_out, buf,2) ;
            }
            break ;

	  case 'p':
	    notEmpty = TRUE ; 
	    /* hack to get an OBJ struct from an empty object,
	     * so we can use the routines in nicedump to output
	     * the obj-name in perl-style with correct quote-marks */
	    obj = bsUpdate (key);
	    niceDump (dump_out, obj, style);
	    break;

	  default:
	    break ;
	  }
      break ;
    }
  if (obj) bsDestroy (obj) ;

  /* always leave a blank line after dumped object */
  if (notEmpty)
    {
      aceOutPrint (dump_out, "\n") ;
    }

  return TRUE ;
} /* dumpKey2 */


/*********************************************/
/*********************************************/


static KEY dumpClass(ACEOUT dump_out, KEY k, int t, int *nk, int *no, int *ne) 
{
  jmp_buf jmpBuf ;
  static jmp_buf *oldJmpBuf ;				    /* static to avoid volatile problems */
  volatile KEY kVolatile = 0 ;				    /* NB following must be volatile to
							       ensure there when jump back */
  long int filepos ;


  *nk = *no = *ne = 0 ;

  while (lexNext(t,&k))
    {
      (*nk)++ ;
  
      kVolatile = k ;
      if (!setjmp(jmpBuf))
	{
	  oldJmpBuf = messCatchCrash (&jmpBuf) ;

	  if (dumpKey2 (dump_out, k, 0, 0))
	    (*no)++ ;

	}
      else
	{
	  /* an error caught in messCrash() */
	  if (!trapKeys)
	    {
	      trapKeys = arrayCreate(32,KEY);
	      trapMessages = arrayCreate(32,char*);
	    }

	  (*ne)++ ;
	  array(trapKeys,arrayMax(trapKeys),KEY) = kVolatile ;
	  array(trapMessages,arrayMax(trapMessages),char*) = strnew (messCaughtMessage(), 0) ;
	}

      messCatchCrash(oldJmpBuf) ; /* restore value for enclosing function */

      if (!aceOutStreamPos (dump_out, &filepos))
	filepos = 0;		/* so second cond. below won't fail */

      if ((*no > MAX_OBJECTS_IN_DUMP))
	return k ;
      else if ((filepos > MAX_BYTES_IN_DUMP))
	return k ;
    }

  return 0;
} /* dumpClass */

/******************************************/
#ifndef NON_GRAPHIC

static void dumpDestroy(void)
{
  dumpGraph = 0 ;
}

#endif /* NON_GRAPHIC */
/******************************************/

static ACEOUT dumpOpenOutput(BOOL newDump, char *classname, int nn)
{ 
  static char letter ;
  static char date[12] ;
  char name[DIR_BUFFER_SIZE+FIL_BUFFER_SIZE] ;
  ACEOUT dump_out;

  if (newDump)
    {
      letter = 'A' ;
      strcpy (date, timeShow (timeParse ("today"))) ;

      if (classname)
	sprintf (name, DUMPNAME_CLASS, dumpDir, date, letter, classname, nn) ;
      else
	sprintf (name, DUMPNAME, dumpDir, date, letter, nn) ;
      while (filCheckName (name, "ace", "r"))
	{
	  ++letter ;
	  if (classname)
	    sprintf (name, DUMPNAME_CLASS, dumpDir, date, letter, classname, nn) ;
	  else
	    sprintf (name, DUMPNAME, dumpDir, date, letter, nn) ;
	}
    }
  else
    if (classname)
      sprintf (name, DUMPNAME_CLASS, dumpDir, date, letter, classname, nn) ;
    else
      sprintf (name, DUMPNAME, dumpDir, date, letter, nn);
	
  strcat (name, ".ace");


#ifdef NON_GRAPHIC

  if (!(dump_out = aceOutCreateToFile(name, "w", 0)))
    messout ("Dump could not open the output file.");

#else /* !NON_GRAPHIC */

  if (!(dump_out = aceOutCreateToFile(name, "w", 0)))
    graphText ("Dump could not open the output file.", 5, 5) ;
  else
    if (newDump && !classname)
      graphText (messprintf ("Ready to dump into : %s", name), 2, 3) ;
  graphRedraw () ;

#endif /* !NON_GRAPHIC */

  return dump_out ;
} /* dumpOpenOutput */

/********************************************/

static void dump1Check (void)
{
#ifndef NON_GRAPHIC
  static char directory[DIR_BUFFER_SIZE] = "";
  static char filename[FIL_BUFFER_SIZE] = "parse_log";
#endif      
  ACEOUT err_out = NULL;
  int i;

  if (trapKeys)
    {
      messout ("%d errors occurred during dump", arrayMax(trapKeys)) ;
      messdump ("DUMP ERROR: %d errors on dumping\n", arrayMax(trapKeys)) ;
#ifndef NON_GRAPHIC
      strcpy (directory, dumpDir);

      err_out = 
	aceOutCreateToChooser ("Choose file to dump the errors to",
			       directory, filename, "err", "w", 0);
      if (!err_out)
	messout ("Writing errors to logfile (database/log.wrm)") ;
      else
	aceOutPrint (err_out, "%d errors on dumping\n",
		     arrayMax(trapKeys)) ;
#endif      

      for (i = 0 ; i < arrayMax(trapKeys) ; i++)
	{ 
	  if (err_out)
	    aceOutPrint (err_out, "%s\t%s\t%s\n",
			 className(arr(trapKeys,i,KEY)),
			 name(arr(trapKeys,i,KEY)),
			 arr(trapMessages, i, char*)) ;
	  else
	    messdump ("%s\t%s\t%s\n",
		      className(arr(trapKeys,i,KEY)),
		      name(arr(trapKeys,i,KEY)),
		      arr(trapMessages, i, char*)) ;

	  messfree (arr(trapMessages, i, char*)) ;
	}

#ifdef NON_GRAPHIC
      messout ("Errors written to logfile (database/log.wrm)");
#endif

      arrayDestroy (trapKeys) ;
      arrayDestroy (trapMessages) ;
      
      if (err_out)
	aceOutDestroy (err_out);
    }

  return;
} /* dump1Check */


/* Menu function, called from main window menus.                             */
/*                                                                           */
/* Dump the whole database giving the user various options.                  */
/*                                                                           */
void dumpAll (void)
{ 
  dumpTimeStamps = messQuery ("Do you want to dump timestamps?") ;
  dumpComments = messQuery ("Do you want to dump comments?") ;

  if (dumpSetDirectory())
    dumpAllNonInteractive (0, FALSE) ;

  return ;
}


/* Menu function, called from main window menus.                             */
/*                                                                           */
/* Allows user to choose which dump set they want to read in and then calls  */
/* the general routine to do the reading in.                                 */
/*                                                                           */
void dumpReadAll(void)
{
  BOOL status = TRUE ;
  char dirSelection[DIR_BUFFER_SIZE] = "" ;
  char fileSelection[FIL_BUFFER_SIZE] = "" ;
  ACEIN parse_in = NULL ;

  if (!sessionGainWriteAccess())
    { 
      messerror ("Sorry, cannot get write access !");
      status = FALSE ;
    }

  if (status)
    {
      if (!getcwd(dirSelection, DIR_BUFFER_SIZE))
	{
	  char *dirname = NULL ;

	  messerror("Unable to retrieve current working directory, "
		    "using database directory instead.") ;

	  dirname = dbPathMakeFilName("", "", "", 0) ;
	  if (!strcpy(dirSelection, dirname))
	    messcrash("Serious internal error, unable to find database directory.") ;

	  messfree(dirname) ;
	}
    }

  if (status)
    {
      /* Display only ace dump filenames in the file chooser.                */
      strcpy(fileSelection, DUMPALLNAME_REGEXP) ;
      parse_in = aceInCreateFromChooser("Choose file \"" DUMPNAME_REGEXP "\" of the dump",
					dirSelection, fileSelection,
					"ace", "r", 0) ;
      if (parse_in)
	{
	  Stack s = NULL ;
	  ACEOUT result_out = NULL ;

	  s = stackCreate(30) ;
	  stackTextOnly(s) ;
	  result_out = aceOutCreateToStack(s, 0) ;

	  status = dumpDoReadAll(parse_in, result_out, TRUE, dirSelection, fileSelection) ;

	  if (!status)
	    messerror("%s", popText(s)) ;
	  else
	    messout("%s", popText(s)) ;

	  aceOutDestroy(result_out) ;
	  stackDestroy(s) ;
	}
      else
	{
	  /* Currently there's nothing to do here.                           */
	  /* Either user clicked CANCEL, or there was an error which will    */
	  /* already have been reported.                                     */
	}
    }

  return ;
}


/* General function to read in a set of dumps.                               */
/*                                                                           */
/* Read in a database dump. The dump may be split across a set of files      */
/* which have will have the naming convention:                               */
/*                                                                           */
/*         dump_<date>_<dump-id>(_<classname>).<dump-file-num>.ace           */
/*                                                                           */
/*    e.g. dump_2001-09-05_A.1.ace                                           */
/*      or dump_2001-09-05_A_Sequence.1.ace                                  */
/*                                                                           */
/*    so dump-id is a letter identifying a particular dump and dump-file-num */
/*    is a digit identifying a particular file within that dump and this     */
/*    may be optionally followed by a classname to show that dumps only      */
/*    contain that class.                                                    */
/*                                                                           */
/* Returns TRUE if all files in a database dump were successfully read in,   */
/* FALSE otherwise (e.g. when perhaps only some files could be read in).     */
/*                                                                           */
/* NOTE: the code is of course very sensitive to the above format, if you    */
/* change it you will almost certainly have to change the code (e.g. the bit */
/* that finds the dump prefix).                                              */
/*                                                                           */
/* UUUUGGGGHHHH, I'm not happy about the "interactive" flag, the routine     */
/* could be rewritten to avoid this by providing a separate routine to       */
/* create the dump pattern which would only be called for interactive        */
/* apps...                                                                   */
BOOL dumpDoReadAll(ACEIN parse_in, ACEOUT result_out, BOOL interactive,
		   char *dirSelection, char *fileSelection)
{
  BOOL status = TRUE ;
  char dump_prefix[FIL_BUFFER_SIZE] = "" ;
  char *dump_name = NULL ;
  BOOL dump_stats = FALSE ;
  Stack s = NULL ;
  ACEOUT parse_out = NULL ;
#define DUMP_ERR "Aborting dump reading:"

  if (!parse_in)
    messcrash("acein argument to dumpDoReadAll() is NULL.") ;

  if (!result_out)
    messcrash("aceout argument to dumpDoReadAll() is NULL.") ;

  if (!dirSelection)
    dirSelection = "." ;

  if (!fileSelection)
    messcrash("filename argument to dumpDoReadAll() is NULL.") ;

  if (!isWriteAccess())
    {
      if (!sessionGainWriteAccess())
	{ 
	  aceOutPrint(result_out, "Sorry, cannot get write access !") ;
	  status = FALSE ;
	}
    }

   if (status)
    {
      /* Check the selected name to see if it fits the format for the first  */
      /* of a set of dump files.                                             */
      if (regExpMatch(fileSelection, DUMPNAME_REGEXP) != 1)
	{
	  aceOutPrint(result_out, "Sorry, the name of the file you chose (%s)"
		    " is not the right format for the first file of an ace dump"
		    " i.e. it must be of the pattern \"" DUMPNAME_REGEXP "\".",
		    fileSelection) ;
	  status = FALSE ;
	}
      else
	{
	  /* Get hold of the first part of the dump file name so we can use  */
	  /* it look for further dump files.                                 */
	  int bytes = 0 ;
	  char *dump_num_ptr = NULL ;

	  dump_num_ptr = strrchr(fileSelection, '1') ;
	  bytes = dump_num_ptr - &fileSelection[0] ;
	  memcpy(dump_prefix, fileSelection, bytes) ;
	  dump_prefix[bytes + 1] = '\0' ;		    /* paranoia */
	}
    }


   /* As this is potentially a very time consuming process, give the user a  */
   /* chance to abort, but only if we running interactively.                 */
   if (status && interactive)
    {
      /* should this be for interactive stuff only ????                      */
      if (!messQuery("Dumpset \"%s*.ace\" will be read in,"
		     " do you want to proceed", dump_prefix))
	status = FALSE ;
    }

   /* Loop round reading all the dump files for one database dump. Each time */
   /* we collect up the parsing stats for that dump file.                    */
   if (status)
    {
      BOOL file_found = TRUE, file_parsed = TRUE ;
      int dump_num = 1 ;

      dump_name = hprintf(0, "%s/%s", dirSelection, fileSelection) ;


      s = stackCreate(30) ;
      stackTextOnly(s) ;
      parse_out = aceOutCreateToStack(s, 0) ;

      if (aceOutPrint(parse_out, "Parse Stats for dumpset \"%s*.ace\"\n",
		      dump_prefix) != ESUCCESS)
	messcrash("Unable to write to output stack for dump parse messages") ;

      while (file_found && file_parsed)
	{
	  if (aceOutPrint(parse_out, "\nFile \"%s\":\n",
			  dump_name) != ESUCCESS)
	    messcrash("Unable to write to output stack for dump parse messages") ;


	  file_parsed = doParseAceIn(parse_in, parse_out, 0, FALSE, TRUE) ; /* destroys parse_in */
	  parse_in = NULL ;

	  if (!file_parsed)
	    {
	      aceOutPrint(parse_out, DUMP_ERR " parsing of dump file %s failed.", dump_name);
	      status = FALSE ;
	    }
	  else
	    {
	      dump_stats = TRUE ;			    /* print stats at end. */

	      /* Get next dump file if there is one.                         */
	      dump_num++ ;
	      messfree(dump_name) ;
	      dump_name = hprintf(0, "%s/%s%d.ace", dirSelection, dump_prefix, dump_num) ;
	      if (filCheckName(dump_name, NULL, "e"))
		{
		  if (filCheckName(dump_name, NULL, "r"))
		    {
		      parse_in =  aceInCreateFromFile(dump_name, "r", "", 0) ;
		      if (!parse_in)
			{
			  aceOutPrint(parse_out, DUMP_ERR " could not open dump file %s",
				      dump_name);
			  status = file_found = FALSE ;
			}
		    }
		  else
		    {
		      aceOutPrint(parse_out, DUMP_ERR " could not read dump file %s", dump_name);
		      status = file_found = FALSE ;
		    }
		}
	      else
		file_found = FALSE ;
	    }
	}
    }


   /* Print out summary of parse stats for files read in so far.             */
   if (parse_out)
     aceOutPrint(result_out, "%s", popText(s)) ;

   /* Clean up stuff.                                                       */
   if (parse_in)
     aceInDestroy(parse_in) ;
   if (dump_name)
     messfree(dump_name) ;
   if (parse_out)
     aceOutDestroy(parse_out) ;
   if (s)
     stackDestroy(s) ;

  return status ;
}




void dumpAllNonInteractive (char *dir, BOOL split)
{
  int nk, ne, no , t , total = 0, nfile = 0 ;
  KEY cKey, key=0 ;
  ACEOUT dump_out;
  long int filepos;
#ifndef NON_GRAPHIC
  int box = 0, line = 0 ;

  if(!graphActivate(dumpGraph))
    { 
      dumpGraph =  displayCreate("DtDump");
      graphMenu (dumpMenu);
      graphTextBounds (80,100) ;
      graphRegister(DESTROY,dumpDestroy) ;
      graphColor (BLACK) ;
    }
  else
    { 
      graphPop() ;
      graphClear();
    }
#endif /* NON_GRAPHIC */

  if (dir)
    strcpy (dumpDir, dir) ;

  if (!split)
    { 
      dump_out = dumpOpenOutput(TRUE, 0, ++nfile);
      if (!dump_out) 
	goto end;
    }
  else 
    dump_out = 0;

#ifndef NON_GRAPHIC
  
  graphText(messprintf("Session %d", thisSession.session),
	    4, 2) ;
  graphText ("Any object in the keyset called DoNotDump will be jumped", 
	     4, 6) ;
  graphRedraw () ;

  /* If user does not want to proceed get rid of the dump window.            */
  if (!messQuery("Should I proceed ?"))
    {
      graphDestroy() ;
      return ;
    }

  line = 8 ;
  
  graphText ("Dumping classes ", 4, line++)  ;
  graphRedraw () ;
#endif /* NON_GRAPHIC */ 
  
  if (lexword2key ("DoNotDump",&cKey,_VKeySet))
    doNotDumpKeySet  = arrayGet(cKey, KEY, "k") ;
  for (t = 5; t<256 ; t++)
    if (lexMax(t))
      if (dumpClassPossible(t, 'a'))
	{ 
	  if (split)
	    { 
	      BOOL first;
	      if (dump_out) 
		{ 
		  aceOutPrint (dump_out,
			       "\n\n // End of this dump file \n\n") ;
		  aceOutDestroy (dump_out);
		  total = 0;
		  first = FALSE;
		}
	      else
		first = TRUE;
	      dump_out = dumpOpenOutput (first, pickClass2Word(t),
					 ++nfile);
	      if (!dump_out) 
		break;
	    }
	  aceOutPrint (dump_out,
		       "\n\n // Class %s \n\n", pickClass2Word(t))  ;
	  if ((key = dumpClass (dump_out, key, t, &nk, &no, &ne)))
	    {
	      total = 0;
	      t--; /* class not complete, continue; */
	    }
	  else
	    { 
	      if (split)
		nfile = 0;
#ifndef NON_GRAPHIC
	      box = graphBoxStart() ;
	      graphText (messprintf ("Class %s : %d keys, %d obj, %d errors",
				     pickClass2Word(t),
				     nk,no, ne) , 4, line++)  ;
	      graphBoxEnd () ;
	      graphBoxDraw(box,BLACK,WHITE) ;
#endif /* NON_GRAPHIC */
	    }
	  if (!aceOutStreamPos (dump_out, &filepos))
	    filepos = 0;

	  if (!split && (((total += no) > MAX_OBJECTS_IN_DUMP) ||
			 (filepos > MAX_BYTES_IN_DUMP)))
	    { 
	      aceOutPrint (dump_out,
			   "\n\n // End of this dump file \n\n") ;
	      aceOutDestroy (dump_out);
	      total = 0 ;
#ifndef NON_GRAPHIC
	      graphText("Starting a new dump file", 8, line += 3) ;
#endif
	      dump_out = dumpOpenOutput(FALSE, split ? pickClass2Word(t) : 0, ++nfile) ;
	      if (!dump_out)
		break;
	      
	    }
	}
 end:
  if (dump_out)
    { 
      aceOutPrint (dump_out, "\n\n // End of this dump file \n\n") ;
      aceOutDestroy (dump_out);
    }
  else
#ifdef NON_GRAPHIC
    messout ("Failed to open dump file, aborting dump.\n");
#else  /* !NON_GRAPHIC */
    graphText("Failed to open dump file, cannot complete dump.",8,line);
#endif /* !NON_GRAPHIC */

  keySetDestroy (doNotDumpKeySet) ;
#ifndef NON_GRAPHIC
  graphRedraw() ;
#endif /* !NON_GRAPHIC */

  dump1Check();
  dumpTimeStamps = FALSE ;
  dumpDir[0] = '\0';

  return;
} /* dumpAllNonInteractive */

/************************************************************/

static BOOL dumpSetDirectory (void)
{
  static char directory[DIR_BUFFER_SIZE] = "";
  static char filename[FIL_BUFFER_SIZE] = "";
  FILE *f = 0;

  f = filqueryopen (directory, filename, "", "rd", 
		    "Choose directory for dump files");
  if (!f)
    return FALSE;
  filclose (f);

  sprintf (dumpDir,"%s/", directory);

  return TRUE;
} /* dumpSetDirectory */

/********************* eof **********************************/
