/*  File: command.h
 *  Author: Fred Wobus (fw@sanger.ac.uk)
 *  Copyright (c) J Thierry-Mieg and R Durbin, 1999
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
 *	Jean Thierry-Mieg (CRBM du CNRS, France) mieg@kaa.crbm.cnrs-mop.fr
 *
 * Description: header file for functions in command.c
 * Exported functions:
 * HISTORY:
 * Last edited: Feb 27 17:44 2009 (edgrif)
 * * Nov 23 09:46 1999 (edgrif): Insert new choixmenu values for admin cmds.
 * Created: Thu Aug 26 17:06:53 1999 (fw)
 * CVS info:   $Id: command.h,v 1.35 2009-03-02 13:14:16 edgrif Exp $
 *-------------------------------------------------------------------
 */

#ifndef ACEDB_COMMAND_H
#define ACEDB_COMMAND_H

#include "acedb.h"
#include "query.h"		/* for COND struct */
#include "spread.h"		/* for SPREAD struct */


/* Commands in acedb are grouped, these groups can be summed to produce sets */
/* of commands for particular users/executables (n.b. this is a once only    */
/* operation, we don't turn these on/off like normal flags).                 */
/* (see w3/tacemain.c as an example.)                                        */
/*                                                                           */
/*                                                                           */
/* CHOIX_RPC is a temporary hack while the rpc server is still supported.    */
/* The RPC server only includes CHOIX_RPC, while the socket server includes  */
/* CHOIX_RPC and CHOIX_SERVER. Once the rpc server goes we can adjust the    */
/* code to only use CHOIX_SERVER.                                            */
/*                                                                           */
typedef enum CmdChoix_
{
  CHOIX_UNDEFINED =  0U,
  CHOIX_UNIVERSAL =  1U,
  CHOIX_NONSERVER =  2U,
  CHOIX_SERVER    =  4U,
  CHOIX_GIF       =  8U,
  CHOIX_RPC       = 16U					    /* Temporary hack while rpc server is
							       still used. */
} CmdChoix ;

typedef enum CmdPerm_
{
  PERM_UNDEFINED =  0U,
  PERM_READ      =  1U,
  PERM_WRITE     =  2U,
  PERM_ADMIN     =  4U
} CmdPerm ;


/* One day we will have proper return codes for commands so we know whether they succeeded... */
enum {CMD_NOTVALID = 0, CMD_FAILED = -99, CMD_TERMINATE = -1} ;


/* Each command has a key by which it is known, currently this is defined as
 * a KEY, but this is not correct because a KEY is an unsigned int and the
 * code uses negative values. This will change eventually, for now we just
 * use negative values and it all comes out in the wash. */

/* Set of symbols for new general use command, previous commands used ascii  */
/* symbols which are in the range 0 - 255, so there should be no clashes.    */
enum {CMD_LASTMODIFIED = 300, CMD_IDSUBMENU,
      CMD_WRITEACCESS, CMD_NOSAVE, CMD_INTERACTION, CMD_QUIET, CMD_MSGLOG, CMD_NEIGHBOURS} ;

/* Set of symbols that are the KEYs for general admin commands.              */
/* These commands are in the 400 range, other commands should not use this   */
/* range.                                                                    */
enum {CMD_NEWLOG = 400} ;


/* Set of symbols that are the KEYs for server only commands.                */
/* These commands are in the 500 range, other commands should not use this   */
/* range.                                                                    */
enum {CMD_SHUTDOWN = 500, CMD_WHO, CMD_VERSION, CMD_WSPEC} ;

/* Set of symbols that are the KEYs for server admin commands.               */
/* These commands are in the 600 range, other commands should not use this   */
/* range.                                                                    */
enum {CMD_PASSWD = 600, CMD_USER, CMD_GLOBAL, CMD_DOMAIN,
      CMD_REMOTEPARSE, CMD_REMOTEPPARSE, CMD_REMOTEDUMP, CMD_REMOTEDUMPREAD, CMD_SERVERLOG} ;


/* smap facility commands. */
enum {CMD_SMAP = 800, CMD_SMAPDUMP, CMD_SMAPLENGTH} ;


/* Return result from the file arg finding call.                             */
/* CMD_FILE_NONE =>  cmd does not use any files OR filename not specified.   */
/* CMD_FILE_FOUND => a filename was specified correctly (check returned args */
/*                   for input and/or output file).                          */
/* CMD_FILE_ERR   => filename specified but incorrectly.                     */
/*                                                                           */
typedef enum CmdFilespecArg_ {CMD_FILE_NONE, CMD_FILE_FOUND, CMD_FILE_ERR } CmdFilespecArg ;


/* Opaque reference to an instance of an ace command.                        */
typedef struct _AceCommandStruct *AceCommand ;


/************************************************************/

/* wrapper function */
void commandExecute (ACEIN fi, ACEOUT fo, unsigned int choix, unsigned int perms, KEYSET activeKs) ;

/********************/

AceCommand aceCommandCreate (unsigned int choix, unsigned int perms);

KEY aceCommandExecute (AceCommand look, ACEIN command_in, ACEOUT result_out, int option, int maxChar);
Stack commandStackExecute (AceCommand look, char *command) ; /* no fioritures */


void aceCommandNoWrite (AceCommand look);		    /* drop write access */
void aceCommandSetInteraction(AceCommand look, BOOL user_interaction) ;	/* Set user interaction true or false. */

CmdChoix aceCommandGetChoixGroup(KEY key) ;

CmdFilespecArg aceCommandCheckForFiles(char *cmd_string, char **newcmd_string,
				       char **input_file, char **output_file) ;


void aceCommandSwitchActiveSet (AceCommand look, KEYSET ks, KEYSET ks2) ;
							    /* Ace-C: private use in acinside.c */

void aceCommandDestroy (AceCommand look) ;



/* Only exposed for gif menu, would like that to go.... */
void commandDoSMap(ACEIN command_in, ACEOUT result_out) ;


/******************/

/* Support for extra menu functions for the GIF menu.                        */

/* entry point for gif-commands, is set to gifControl in applications
 * that link with the graph-lib or gif-lib */
typedef void (*GifEntryFunc)(KEYSET activeKS, ACEIN fi, ACEOUT fo, BOOL quiet) ;

extern GifEntryFunc gifEntry ;

/* this is the GifEntryFunc used by command.c to distpatch
 * gif-commands to, if command.c can't deal with it.
 * That mean in graphical/gif apps the global GifEntryFunc
 * can be set to gifControl() */
void gifControl(KEYSET ks, ACEIN fi, ACEOUT fo, BOOL quiet) ;
/* public, called by aceserver */

/******************/


#endif /* !ACEDB_COMMAND_H */
