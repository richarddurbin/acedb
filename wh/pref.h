/*  File: pref.h
 *  Author: Fred Wobus (fw@sanger.ac.uk)
 *  Copyright (C) J Thierry-Mieg and R Durbin, 1999
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
 * 	Richard Durbin (Sanger Centre, UK) rd@sanger.ac.uk, and
 *	Jean Thierry-Mieg (CRBM du CNRS, France) mieg@crbm.cnrs-mop.fr
 *
 * Description: public header for the acedb preference module
 *              
 *              preInit should be called once at startup
 *              
 *              prefValue/Float/Int/Colour may be used anywhere
 *              in the code to configure the behaviour.
 *              
 *              Look at the function prefsubs.c:makePrefDefaults() for
 *              names of currently used prefence values.
 * HISTORY:
 * Last edited: Apr  7 12:38 2008 (edgrif)
 * Created: Sat May  1 11:01:58 1999 (fw)
 * CVS info:   $Id: pref.h,v 1.15 2008-04-07 12:43:20 edgrif Exp $
 *-------------------------------------------------------------------
 */
#ifndef ACEDB_PREF_H
#define ACEDB_PREF_H

/* Preferences include these basic datatypes at the moment.                  */
typedef enum _PrefFieldType {PREF_BOOLEAN = 1, PREF_FLOAT, PREF_INT, PREF_COLOUR,
			     PREF_STRING} PrefFieldType ;


void prefInit() ;					    /* Initialise preference system. */

void prefReadDBPrefs() ;				    /* Read prefs from wspec/acedbrc if
							       there was no $HOME/.acedbrc file. */

/* Get value for a given preference. */
BOOL prefValue(char * name) ;
float prefFloat(char * name) ;
int prefInt(char * name) ;
int prefColour (char * name) ;
char *prefString(char *name) ;


/* Callback system, callers of pref. system may set callback functions that  */
/* will be called to get the default value of a preference, or that may be   */
/* called when a particular preference is changed/set (in this case the      */
/* callback function will be called with the value of the preference).       */
/*                                                                           */

/* Callbacks to return a default value for a preference, given the current   */
/* value the function should this if its OK, or a sensible alternative if    */
/* its not. These functions _MUST_ be callable before                        */
typedef BOOL  (*PrefDefFuncBOOL)(BOOL curr_value) ;
typedef float (*PrefDefFuncFloat)(float curr_value) ;
typedef int   (*PrefDefFuncInt)(int curr_value) ;	    /* Used for int & Colours */
typedef char *(*PrefDefFuncString)(char *curr_value) ;


/* Oh dear, this set of functions does essentially the same as the above     */
/* but tailored for the graphNNEditor int/float editors (you can't have      */
/* these for BOOL or colour by the way, graph doesn't support this). This    */
/* is repetition really but I'm not going to rewrite graph....               */
typedef BOOL (*PrefEditFuncFloat)(float curr_value) ;
typedef BOOL (*PrefEditFuncInt)(int curr_value) ;
typedef BOOL (*PrefEditFuncString)(char *curr_value) ;

/* Callbacks for when a preference is changed/set.                           */
typedef void (*PrefFuncBOOL)(BOOL) ;
typedef void (*PrefFuncFloat)(float) ;
typedef void (*PrefFuncInt)(int) ;			    /* Used for int & Colours */
typedef void (*PrefFuncString)(char *) ;


/* Functions to set callbacks, N.B. need to cast your callback to (void *)   */
/* Note that when you call prefSetDefFunc(), your callback function will be  */
/* called from this routine immediately to get the default value.            */
BOOL prefSetDefFunc(char * name, void *func_ptr) ;	
BOOL prefSetEditFunc(char * name, void *func_ptr) ;	
BOOL prefSetFunc(char * name, void *func_ptr) ;



/* We have held back from going the whole hog and having these set as well,  */
/* really it would be better to have it all set in one go more dynamically.  */
/* But this would require lots of recoding, is it worth it ??                */

/* I think the actual preference strings should be declared here rather than
 * scattered through the code...note that preference names should be just letters
 * and underscores, i.e. no spaces. */

#define SPLASH_SCREEN "SPLASH_SCREEN"
#define WINDOW_TITLE_PREFIX "WINDOW_TITLE_PREFIX"

#define USE_MSG_LIST        "USE_MSG_LIST"
#define MAX_MSG_LIST_LENGTH "MAX_MSG_LIST_LENGTH"

#define BLIXEM_SCOPE       "BLIXEM_SCOPE"
#define BLIXEM_HOMOL_MAX   "BLIXEM_HOMOL_MAX"
#define BLIXEM_EXTERNAL    "BLIXEM_EXTERNAL"
#define BLIXEM_SCRIPT      "BLIXEM_SCRIPT"
#define BLIXEM_TEMPFILES   "BLIXEM_TEMPFILES"
#define BLIXEM_PFETCH      "BLIXEM_PFETCH"
#define BLIXEM_NETID       "BLIXEM_NETID"
#define BLIXEM_DEFAULT_NETID       "pubseq"
#define BLIXEM_PORT_NUMBER "BLIXEM_PORT_NUMBER"
#define BLIXEM_DEFAULT_PORT_NUMBER 22100
#define DNA_HIGHLIGHT_IN_FMAP_DISPLAY "DNA_HIGHLIGHT_IN_FMAP_DISPLAY"


/* These functions are part of the graphical interface to preferences, you   */
/* must call editPreferences() before you use any of the others.             */

void editPreferences(void) ;	/* top-level menu-func to start 
				 * graphical editor */

/* Set a preference value from the code as if a user had set it. The void *  */
/* will be interpreted according to the value of preftype. Returns TRUE if   */
/* preference could set be set, FALSE otherwise.                             */
BOOL setPreference(char *prefname, PrefFieldType preftype, void *prefvalue) ;


#endif /* !ACEDB_PREF_H */
 
 
