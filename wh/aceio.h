/*
 *  File aceio.h  : header file for ACEDB input/output module
 *              (formerly known as freesubs/freeout)
 *  Author: Richard Durbin (rd@sanger.ac.uk)
 *  Copyright (C) J Thierry-Mieg and R Durbin, 1994
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
 * HISTORY:
 * Last edited: Dec  2 13:33 2002 (edgrif)
 * * Oct 18 13:51 1999 (fw): aceInKeyMatch now private to acein.c
 * * Oct 18 13:45 1999 (fw): aceInSelect removed
 * * Oct  6 11:38 1999 (fw): changed aceInCreate interface to include handle
 * Created: Sept 1999 (mieg)
 * CVS info:   $Id: aceio.h,v 1.23 2002-12-02 16:54:52 edgrif Exp $
 *-------------------------------------------------------------------
 */


#ifndef ACEDB_ACEIO_DEF
#define ACEDB_ACEIO_DEF

#include <wh/regular.h>

/* constructors */
ACEIN aceInCreateFromFile (char *filename, const char *spec, char *params,
			   STORE_HANDLE handle);
ACEIN aceInCreateFromChooser (char *prompt, 
			      char *directory, char *filename,
			      const char *extension,
			      const char *spec, STORE_HANDLE handle);
ACEIN aceInCreateFromPipe (char *command, const char *spec, char *params,
			   STORE_HANDLE handle);
ACEIN aceInCreateFromScriptPipe (char *script, char *args, char *params,
				 STORE_HANDLE handle);
ACEIN aceInCreateFromStdin (BOOL isInteractive, char *params,
			    STORE_HANDLE handle);
          /* use stdinIsInteractive() to determine if STDIN is a tty */

ACEIN aceInCreateFromText (char *text, char *params,
			   STORE_HANDLE handle);


/* reuse existing acein */
void aceInSetNewText(ACEIN fi, char *text, char *params) ;


/* destructor */
void uAceInDestroy (ACEIN fi);	/* just a stub for messfree */
#define aceInDestroy(fi) (uAceInDestroy(fi),fi=0)


/******/

char *aceInGetURL (ACEIN fi);

BOOL aceInPrompt (ACEIN fi, char *prompt); /* lie aceInCard, but sets
					    * prompt for readline stdin */
BOOL aceInOptPrompt (ACEIN fi, int *keyp, FREEOPT *options);

BOOL aceInIsInteractive (ACEIN fi); /* is the user interactivly in 
				     * control of this stream ? */

/* set which characters get special treatment in aceIn-package
 * possible choices are "\n\t;/%\\@$"
 * e.g. if you leave out @ it won't be able to open command-files 
 * and therefore the @ character will be returned literally */
void aceInSpecial (ACEIN fi, char* text) ;

/* Use these calls to "quote" and unquote text, this allows some control     */
/* over how text will be interpreted when its read in.                       */
char *aceInProtect (ACEIN fi, char* text);    /* aceinword will read result back as text */
char *aceInUnprotect (ACEIN fi, char *text) ;		    /* Remove all quoting:  \"\\  removed */
char *aceInUnprotectQuote(ACEIN fi, char *text) ;	    /* Remove only quotes:  \"  removed  */

/* Read text/data from the acein.                                            */
char *aceInCard (ACEIN fi) ;	/* get a whole line */
void aceInForceCard (ACEIN fi, char *string) ; /* fake a line */
void aceInCardBack (ACEIN fi) ;
char *aceInWord (ACEIN fi) ;	/* get next word as text */
char *aceInPath (ACEIN fi) ;	/* get word as path/filename */
BOOL aceInInt (ACEIN fi, int *p) ; /* get word as int */
BOOL aceInFloat (ACEIN fi, float *p) ; /* get word as float */
BOOL aceInDouble (ACEIN fi, double *p) ; /* get word as double */

char *aceInWordCut (ACEIN fi, char *cutset, char *cutter) ;
void aceInBack (ACEIN fi) ;


BOOL aceInKey (ACEIN fi, KEY *keyp, FREEOPT *options) ;
BOOL aceInQuery (ACEIN fi, ACEOUT fo, char *query) ;
BOOL aceInCheck (ACEIN fi, char *fmt) ;	/* checks if current card 
					 * matches format */
BOOL aceInStep (ACEIN fi, char x) ;
void aceInNext (ACEIN fi) ;
char* aceInPos (ACEIN fi) ;

BOOL aceInEOF (ACEIN fi);	/* TRUE if fi->streamlevel == 0 */

int aceInStreamLine (ACEIN fi) ; /* current line in file/text */
BOOL aceInStreamLength (ACEIN fi, long int *length); /* total file/text-size */
BOOL aceInStreamPos (ACEIN fi, long int *pos); /* current file/text-pos */

extern char ACEIN_UPPER[] ;
extern char ACEIN_LOWER[] ;
#define aceInUpper(_cc) ACEIN_UPPER[(int)_cc]
#define aceInLower(_cc) ACEIN_LOWER[(int)_cc]


/* constructors */
ACEOUT aceOutCreateToFile (char *filename, const char *spec, 
			   STORE_HANDLE handle);
ACEOUT aceOutCreateToChooser (char *prompt, 
			      char *directory, char *filename,
			      const char *extension,
			      const char *spec, STORE_HANDLE handle);
ACEOUT aceOutCreateToMail (char *address, STORE_HANDLE handle);
ACEOUT aceOutCreateToStdout (STORE_HANDLE handle);
ACEOUT aceOutCreateToStderr (STORE_HANDLE handle);
ACEOUT aceOutCreateToStack (Stack s, STORE_HANDLE handle);

ACEOUT aceOutCopy (ACEOUT source_fo, STORE_HANDLE handle); /* only useful for stdout right now. */
void aceOutSetNewStack(ACEOUT fo, Stack s) ;

/* destructor */
void uAceOutDestroy (ACEOUT fo) ;   /* will close file or send mail */
#define aceOutDestroy(fo) (uAceOutDestroy(fo),fo=0)

/******/
char *aceOutGetFilename(ACEOUT fo) ;			    /* Retrieve filename, _if_ there is one. */

char *aceOutGetURL (ACEOUT fo);

BOOL aceOutFlush (ACEOUT fo);/* FALSE if fo isn't file-based or 
				 * the fflush on fo->fil failed */

BOOL aceOutRewind (ACEOUT fo);	/* FALSE if fo isn't file-based or 
				 * the fseek on fo->fil failed */

BOOL aceOutStreamPos (ACEOUT fo, long int *posp);
/* determines the file-position (ftell) or stack-pos if possible */

/* these functions return errno, which is ESUCCESS if all is OK
 * calling code should check for ENOSPC and others */
int aceOutPrintStr(ACEOUT fo, char *simple_string) ;	    /* Use for simple text string. */
int aceOutPrint(ACEOUT fo, char *format,...) ;		    /* Use for formatted printing. */
int aceOutxy(ACEOUT fo,char *text, int x, int y) ; 
int aceOutBinary(ACEOUT fo,char *data, int size) ;


int aceOutLine (ACEOUT fo) ;	/* how many lines written */
long int aceOutByte (ACEOUT fo) ; /* how bytes have been written */
int aceOutPos (ACEOUT fo) ;

/****** ACETMP - writing to temporary files using ACEOUT ************/

/* Create a tmpfile in the system tmp directory.                             */
ACETMP aceTmpCreate (const char *spec, STORE_HANDLE handle) ;

/* Create a tmpfile in the given directory if it is an absolute path, or in  */
/* a subdir of the system tmp directory if its a relative path.              */
/* Currently aceTmpCreateSubdir() will not create any intermediate subdirs   */
/* specified in the subdir parameter, they must already exist. If the actual */
/* subdir does not exist, it is created.                                     */
ACETMP aceTmpCreateDir(const char *dir, const char *spec, STORE_HANDLE handle) ;

void uAceTmpDestroy (ACETMP atmp);
#define aceTmpDestroy(atmp) (uAceTmpDestroy(atmp),atmp=0)

/* By default aceTmp will delete its temporary file when its destroyed, you  */
/* can use this call to turn this on and off. If you turn it off you must    */
/* first retrieve the filename so you can later delete the file.             */
void aceTmpNoRemove(ACETMP atmp, BOOL no_remove) ;

/* The user has access to the internal output stream */
ACEOUT aceTmpGetOutput (ACETMP atmp);

/* close the tmp-file prematurely */
void aceTmpClose (ACETMP atmp);

/* The filename can still be enquired even when the 
 * output stream has already finished */
char *aceTmpGetFileName (ACETMP atmp);


#endif  /* !ACEDB_ACEIO_DEF */


