/*  File: command_.h
 *  Author: Ed Griffiths (edgrif@sanger.ac.uk)
 *  Copyright (c) J Thierry-Mieg and R Durbin, 2000
 *-------------------------------------------------------------------
 * This file is part of the ACEDB genome database package, written by
 * 	Richard Durbin (Sanger Centre, UK) rd@sanger.ac.uk, and
 *	Jean Thierry-Mieg (CRBM du CNRS, France) mieg@kaa.crbm.cnrs-mop.fr
 *
 * Description: Internal header for ace commands.
 * HISTORY:
 * Last edited: Mar  2 08:49 2009 (edgrif)
 * Created: Thu May 11 19:43:40 2000 (edgrif)
 * CVS info:   $Id: command_.h,v 1.10 2009-03-02 13:14:43 edgrif Exp $
 *-------------------------------------------------------------------
 */
#ifndef DEF_COMMAND_PRIV_H
#define DEF_COMMAND_PRIV_H

#include <wh/command.h>					    /* command public header. */


/* Callers are returned an opaque reference to this struct, which contains   */
/* all the state associated with a command.                                  */
/*                                                                           */
typedef struct _AceCommandStruct
{ 
  magic_t *magic ;					    /* == &AceCommand_MAGIC */
  ACEOUT dump_out;					    /* used for show -f / aql -o etc */

  SPREAD spread ; 
  KEYSET ksNew, ksOld, kA ;
  Stack ksStack ;
  int nns ;						    /* number of entries on stack */
  unsigned int choix ;
  unsigned int perms;

  Array aMenu ;
  KEY lastCommand ;

  BOOL user_interaction ;				    /* TRUE => cmds may ask user for input. */
  BOOL noWrite ;					    /* TRUE => no write access. */
  BOOL quiet ;						    /* TRUE => only output result of
							       command, no acedb comments. */
  BOOL showStatus , beginTable ;
  BOOL showTableTitle;
  int nextObj, lastSpread ; 
  int cumulatedTableLength ;
  int encore ;
  int minN ;						    /* startup */
  int maxN ;						    /* -1 or, max number of desired output */
  int notListable ;					    /* empty and alias objects */
  BOOL dumpTimeStamps ;
  BOOL dumpComments ;
  BOOL noXref ;

  /* Flags for dumping of dna and much more. */
  BOOL allowMismatches ;
  BOOL spliced ;
  BOOL cds_only ;
  COND showCond ;
  char beauty ;						    /* ace, tace, perl */
        /*
        * For beauty, possible values are:  (see w4/dump.c dumpKey2)
        *  p  ace perl
        *  j  java format
        *  J  another java format
        *  a  ace file format
        *  A  ace file format (same?)
        *  C  acec library binary format
        *  H  ???
        *  h  ???
        *  m  minilib format - much like ace file but shows all tags
        *     and has special markings for type A objects (NCBI only)
	*  k  ??? (Sanger only)
	*  K  ??? (Sanger only)
	*  x  ??? (Sanger only)
	*  X  ??? (Sanger only)
	*  y  ??? (Sanger only)
	*  Y  ??? (Sanger only)
        *
        */
  char fastaCommand ;					    /* for DNA, Peptide more */

  /*
  * these are all related to kget/kstore/kclear commands:
  */
  DICT *kget_dict;
  Array kget_namedSets ;
} AceCommandStruct ;

/*
* bug: there should be a comment here that tells you where AceCommandStruct is
* deallocated.  I added the kget_* fields, but do not know where to free them
* when the AceCommandStruct is no longer used.
*/



/* Note the restricted possibilities here...                                 */
/* CMD_FILESPEC_NONE => cmd does not use files at all                        */
/* CMD_FILESPEC_OPTIONAL                                                     */
/*                   => if file specified, must be last word on line.        */
/* CMD_FILESPEC_OPTIONAL & CMD_FILESPEC_FLAG                                 */
/*                   => will be word following flag on line.                 */
/* CMD_FILESPEC_COMPULSORY                                                   */
/*                   => file must be specified, must be last word on line.   */
typedef enum _CmdFilespecType
{
  CMD_FILESPEC_NONE         = 0x0,			    /* cmd. does not use files. */
  CMD_FILESPEC_FLAG	    = 0x1,			    /* flag used to signal file. */
  CMD_FILESPEC_OPTIONAL     = 0x2,			    /* file is optional. */
  CMD_FILESPEC_COMPULSORY   = 0x4			    /* file is compulsory (must be last
							       word on command line). */
} CmdFilespecType ;


/* Describes how files are specified within acedb command line commands.     */
/* Notes:                                                                    */
/*                                                                           */
typedef struct _CmdFilespecStruct
{
  BOOL any_files ;					    /* Does cmd have any files ? */
  CmdFilespecType infile ;				    /* How is file specified ? */
  char *infile_flag ;					    /* flag, e.g. "-f", */
  CmdFilespecType outfile ;				    /* How is file specified ? */
  char *outfile_flag ;					    /* flag, e.g. "-f", */
} CmdFilespecStruct ;



/* Command struct, defines format and use of an ace command.                 */
/* choix = one of the following:
   CHOIX_UNIVERSAL: universal
   CHOIX_NONSERVER: non-server
      CHOIX_SERVER: server
         CHOIX_GIF: gif
*/
typedef struct
{
  unsigned int choix ;					    /* Which command "sets" this command
							       belongs to. */
  unsigned int perm ;					    /* What level of access (e.g. read) is */
							    /* required to run the command.*/
  KEY key ;						    /* id by which command is known. */
  char *text ;						    /* help text for command. */
  CmdFilespecStruct filespec ;				    /* How in/out files are specified. */
} CHOIX ;




/* Internal to the acecommand package.                                       */
/*                                                                           */
CHOIX *uAceCommandGetChoixMenu(void) ;			    /* return ptr to array of all possible */
							    /* commands. */

Array uAceCommandChoisirMenu(unsigned int choix, unsigned int perms) ;
							    /* Return an array of allowed commands */
							    /* for a given command subset and
							       permission level.*/

#endif /* DEF_COMMAND_PRIV_H */
