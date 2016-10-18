/*  File: parse.h
 *  Author: Jonathan Hodgkin (cgc@mrc-lmba.cam.ac.uk)
 *  Copyright (C) J Thierry-Mieg and R Durbin, 1993
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
 * 	Richard Durbin (MRC LMB, UK) rd@mrc-lmba.cam.ac.uk, and
 *	Jean Thierry-Mieg (CRBM du CNRS, France) mieg@crbm1.cnusc.fr
 *
 * Description:
 * Exported functions:
 * HISTORY:
 * Last edited: Jan 30 10:16 2001 (edgrif)
 * Created: Mon Nov 22 14:03:14 1993 (cgc)
 * CVS info:   $Id: parse.h,v 1.20 2004-04-09 22:21:49 mieg Exp $
 *-------------------------------------------------------------------
 */
#ifndef ACEDB_PARSE_H
#define ACEDB_PARSE_H

#include <wh/acedb.h>

/* callback functions can be registered with parse for array type data, e.g. */
/* DNA data. These functions should have the following prototype. If the     */
/* func returns PARSEFUNC_ERR then errtext should be set to show the error.  */
/*                                                                           */
/* parse_io is where to get the input from                                   */
/* key is the key to the current object to be parsed                         */
/* errtext describes the type of error.                                      */
/*                                                                           */
/* Return type:   PARSEFUNC_OK      array object added successfully          */
/*                PARSEFUNC_EMPTY   no data supplied, nothing created        */
/*                PARSEFUNC_ERR     error, nothing created.                  */
/*                                                                           */
typedef enum _ParseFuncReturn {PARSEFUNC_OK, PARSEFUNC_EMPTY, PARSEFUNC_ERR} ParseFuncReturn ;
typedef ParseFuncReturn (*ParseFuncType)(ACEIN parse_io, KEY key, char **errtext) ;


/* this function is not really as it should be, it hides within it the names */
/* of the parse callback functions, but at least its now in a header !       */
void parseArrayInit(void) ;



/* New parseAceIn function with extra "output" parameter, this can slowly    */
/* replace the old parseAceIn calls, it allows the caller to set a different */
/* destination for output (e.g. for the server to output to a client).       */
/* parse ace-data from given stream. 
 * output - alternative destination for error messages (if NULL, errors
 *          are output via messerror).
 * ks - if non-NULL it will add the keys of any objects parsed onto it
 * keepGoing - TRUE it won't stop on errors
 * full_stats - TRUE will display full alias/rename/add/edit/delete stats
 * WILL destroy the given ACEIN */
BOOL doParseAceIn(ACEIN parse_in, ACEOUT output, KEYSET ks, BOOL keepGoing, BOOL full_stats) ;

/*                                                                           */
/* DON'T USE THIS OLD FUNCTION, it is now simply a cover for the above new   */
/* function. When all occurences have been removed from the code it will     */
/* disappear.                                                                */
/*                                                                           */
BOOL parseAceIn(ACEIN fi, KEYSET ks, BOOL keepGoing) ;
/* DON'T USE THIS OLD FUNCTION....                                           */

/* convenience function to parse the content of a text-buffer */
BOOL parseBuffer(char *text, KEYSET ks, BOOL keepGoing);

/* convenience function to parse the content of a text-buffer, but without   */
/* issuing messages, should be used where code silently constructs a snippet */
/* of ace data and parses it in as part of some larger operation on behalf   */
/* of the user.                                                              */
BOOL parseBufferInternal(char *text, KEYSET ks, BOOL keepGoing);
/* convenience call private to AceC */
BOOL parseAceCBuffer (char *text,  Stack s, ACEOUT out, KEYSET ks) ;

#ifndef NON_GRAPHIC
void parseControl(void);	/* menu/button-func to open parse-window */
#endif /* !NON_GRAPHIC */


/* Externally visible array of parse functions for different array classes.  */
extern ParseFuncType parseFunc[] ;


#endif /* !ACEDB_PARSE_H */
/********* end of file ***********/
