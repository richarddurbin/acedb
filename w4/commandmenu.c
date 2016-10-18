/*  File: commandmenu.c
 *  Author: Ed Griffiths (edgrif@sanger.ac.uk)
 *  Copyright (c) J Thierry-Mieg and R Durbin, 2000
 *-------------------------------------------------------------------
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
 *-------------------------------------------------------------------
 * This file is part of the ACEDB genome database package, written by
 * 	Richard Durbin (Sanger Centre, UK) rd@sanger.ac.uk, and
 *	Jean Thierry-Mieg (CRBM du CNRS, France) mieg@kaa.crbm.cnrs-mop.fr
 *
 * Description: Handles the array of commands that can be interpreted
 *              by acedb.
 *              Contains the array describing the command.
 *              Returns an array of allowed commands for a given
 *              command/permissions set.
 *              
 * Exported functions: See command_.h
 * HISTORY:
 * Last edited: Dec 21 10:51 2009 (edgrif)
 * Created: Wed May 17 10:05:43 2000 (edgrif)
 * CVS info:   $Id: commandmenu.c,v 1.30 2009-12-21 10:52:40 edgrif Exp $
 *-------------------------------------------------------------------
 */

#include <wh/regular.h>
#include <wh/aceio.h>
#include <wh/status.h>					    /* For cmd line opts. */
#include <w4/command_.h>

/************************************************************/

static CmdFilespecArg getFlagFile(char **file_name, char *flag, char *curr_word,
				  ACEIN curr_string) ;
static CmdFilespecArg getNoFlagFile(char **file_name, char *curr_word,
				    CmdFilespecType filespec) ;


/************************************************************/


/* Some commands have unix/windows specific instructions to user etc...      */
/*                                                                           */
/* This is for the Parse cmd.                                                */
#if defined(WIN32)
#define UNIX_OR_WINDOWS_INPUTFILES "parses pkzip (.zip) compressed or native ace files or stdin, until EOF"
#else
#define UNIX_OR_WINDOWS_INPUTFILES "parse an ace or ace.Z or ace.gz file or stdin,  until ctrl-D or EOF"
#endif


/* This table defines the commands that can be executed against an acedb
 * database.
 * To add a new command you need to add an entry to this table checking
 * _carefully_ that it does not clash with any entries here. You then need
 * to add a new 'case' statement to handle the command in the big 'switch'
 * statement in the aceCommandExecute routine in command.c, or add your own
 * code to follow a call to aceCommandExecute to deal with the command (see
 * aceserver.c or serverace.c for examples).
 *
 * There are permission levels and command types, each command should have
 * just one of each set, e.g. READ and UNIVERSAL for command "find"
 *
 * All commands should be one of     READ | WRITE | ADMIN
 *
 * All commands should be one of  UNIVERSAL | NONSERVER | SERVER | GIF | RPC
 * (note that RPC will be merged into SERVER once we stop supporting the
 * rpc server)
 *
 * All commands should also specify their in/out file usage via 
 * */
static CHOIX choixMenu [] = 
{ 
  /* General commands.                                                       */
  {CHOIX_UNIVERSAL, PERM_READ, -1,
   "Quit : Exits the program. Any command abbreviates, $starts a subshell, until empty line",
   {FALSE, CMD_FILESPEC_NONE, NULL, CMD_FILESPEC_NONE, NULL}},
  {CHOIX_UNIVERSAL, PERM_READ, 402,
   "system_list : issue the binary AceC system list dump",
   {FALSE, CMD_FILESPEC_NONE, NULL, CMD_FILESPEC_NONE, NULL}},
  {CHOIX_UNIVERSAL, PERM_READ, '?',
   "? : List of commands",
   {FALSE, CMD_FILESPEC_NONE, NULL, CMD_FILESPEC_NONE, NULL}},
  {CHOIX_UNIVERSAL, PERM_READ,
   'h', "Help : for further help",
   {FALSE, CMD_FILESPEC_NONE, NULL, CMD_FILESPEC_NONE, NULL}},
  {CHOIX_UNIVERSAL, PERM_WRITE, 'S',
   "Save [-drop | -regain] :\n"
   "\tSave current state otherwise the system saves every 600 seconds\n"
   "\t-drop   (default) drop write access after saving\n"
   "\t-regain regain write access after saving\n",
   {FALSE, CMD_FILESPEC_NONE, NULL, CMD_FILESPEC_NONE, NULL}},
  {CHOIX_UNIVERSAL, PERM_WRITE, CMD_NOSAVE,
   "NoSave [-stop | -start] :\n"
   "\tStop or start all database saving, the -stop option prevents any saving by any command,\n"
   "\tyou must use the -start option to turn saving on again.\n"
   "\tThis is particularly useful if you want to test scripts that use tace for large\n"
   "\tscale database updates without altering the database.\n"
   "\t-stop   (default) turn saving off\n"
   "\t-start  turn saving on.\n",
   {FALSE, CMD_FILESPEC_NONE, NULL, CMD_FILESPEC_NONE, NULL}},
  {CHOIX_UNIVERSAL, PERM_WRITE, CMD_WRITEACCESS,
   "Writeaccess [-gain | -drop [save | nosave]] :\n"
   "\tSet write access status\n"
   "\t-gain (default) get write access\n"
   "\t-drop lose write access, with or without saving.\n",
   {FALSE, CMD_FILESPEC_NONE, NULL, CMD_FILESPEC_NONE, NULL}},
  {CHOIX_NONSERVER, PERM_WRITE,  '<',
   "Read-only : Definitively drop present and future write access",
   {FALSE, CMD_FILESPEC_NONE, NULL, CMD_FILESPEC_NONE, NULL}},
  {CHOIX_UNIVERSAL, PERM_READ, '/',
   "// : comment, do nothing",
   {FALSE, CMD_FILESPEC_NONE, NULL, CMD_FILESPEC_NONE, NULL}},
  {CHOIX_UNIVERSAL, PERM_READ, CMD_LASTMODIFIED,
   "lastmodified : Returns time database was last modified as a string in acedb time format.",
   {FALSE, CMD_FILESPEC_NONE, NULL, CMD_FILESPEC_NONE, NULL}},
  {CHOIX_UNIVERSAL, PERM_READ, CMD_IDSUBMENU,
   "ID <command> : id interface - type ID ? for commands",
   {FALSE, CMD_FILESPEC_NONE, NULL, CMD_FILESPEC_NONE, NULL}},
  {CHOIX_UNIVERSAL, PERM_READ, CMD_QUIET,
   "quiet -on | -off :\n"
   "\tTurn quiet mode on or off, when \"on\" commands produce only their output without any extra comments.\n"
   "\t(Note the usual \"NNN Active Objects\" is no longer displayed, use the Count command.)\n"
   "\tQuiet mode makes parsing command output easier because either the first line\n"
   "\tis an acedb comment because the command failed or there are no acedb comments in the output.\n"
   "\t-off    (default) verbose output\n"
   "\t-on     no comments unless there is an error.\n",
   {FALSE, CMD_FILESPEC_NONE, NULL, CMD_FILESPEC_NONE, NULL}},
  {CHOIX_UNIVERSAL, PERM_READ, CMD_MSGLOG,
   "msglog -msg <on | off> -log <on | off> :\n"
   "\tTurn display or logging on or off for information and error messages.\n"
   "\tUseful when long operations generate many harmless information or error messages\n"
   "\tand you don't want to see and/or log them.\n"
   "\t-off    display or logging is turned off.\n"
   "\t-on     display or logging is turned on.\n",
   {FALSE, CMD_FILESPEC_NONE, NULL, CMD_FILESPEC_NONE, NULL}},

  {CHOIX_UNIVERSAL, PERM_READ,
   'c', "Classes : Give a list of all the visible class names and how many objects they contain.",
   {FALSE, CMD_FILESPEC_NONE, NULL, CMD_FILESPEC_NONE, NULL}},
  {CHOIX_UNIVERSAL, PERM_READ,
   'z', "Model [-jaq] model : Shows the model of a class or subtype, useful before Follow or Query commands",
   {FALSE, CMD_FILESPEC_NONE, NULL, CMD_FILESPEC_NONE, NULL}},
  {CHOIX_UNIVERSAL, PERM_READ,
   'n', "Find class [name] : Creates a new list with all objects from class, optionally matching name",
   {FALSE, CMD_FILESPEC_NONE, NULL, CMD_FILESPEC_NONE, NULL}},
  {CHOIX_UNIVERSAL, PERM_READ,
   CMD_NEIGHBOURS, "Neighbours : Creates a new list with all objects from current keyset _and_ all objects"
   " referenced by those objects.",
   {FALSE, CMD_FILESPEC_NONE, NULL, CMD_FILESPEC_NONE, NULL}},
  {CHOIX_UNIVERSAL, PERM_READ,
   't', "Follow Tag : i.e. Tag Author gives the authors of the papers of the current list",
    {FALSE, CMD_FILESPEC_NONE, NULL, CMD_FILESPEC_NONE, NULL}},
  {CHOIX_UNIVERSAL, PERM_READ,  'g', "Grep [-active] template : searches for *template* everywhere in the database except LongTexts",
    {FALSE, CMD_FILESPEC_NONE, NULL, CMD_FILESPEC_NONE, NULL}},
  {CHOIX_UNIVERSAL, PERM_READ,  'G', "LongGrep template : searches for *template* also in LongTexts",
    {FALSE, CMD_FILESPEC_NONE, NULL, CMD_FILESPEC_NONE, NULL}},
  {CHOIX_UNIVERSAL, PERM_READ, 'l', 
   "List [-h | -a | -p | -j | -x etc.] [-c max] [-f out_filename] [[-t] template]\n"
   "\tlists names [matching template] of objects in active list in [human | ace | perl | java | xml] style\n"
   "\tother styles: -J -k -K -xclass -xall -xnocontent",
   {TRUE, CMD_FILESPEC_NONE, NULL, CMD_FILESPEC_OPTIONAL | CMD_FILESPEC_FLAG, "-f"}},
  {CHOIX_UNIVERSAL, PERM_READ,  's',
   "Show [-h | -a | -p | -j | -x etc.] [-T] [-b begin] [-c max] [-f out_filename]  [-C] [[-t] tag]\n"
   "\toutputs full object data for active list in [human | ace | perl | java] style\n"
   "\t-T show time stamps, -b begin at object b, -c show max obj, -C ignore comments\n"
   "\tthe last parameter restricts to the [tag]-branch\n"
   "\tother styles: -P -J -k -K -xclass -xall -xnocontent",
   {TRUE, CMD_FILESPEC_NONE, NULL, CMD_FILESPEC_OPTIONAL | CMD_FILESPEC_FLAG, "-f"}},
  {CHOIX_UNIVERSAL, PERM_READ,  'i', "Is template : keeps in list only those objects whose name match the template",
    {FALSE, CMD_FILESPEC_NONE, NULL, CMD_FILESPEC_NONE, NULL}},
  {CHOIX_UNIVERSAL, PERM_READ,  'r', "Remove template : removes from list those objects whose name match the template",
    {FALSE, CMD_FILESPEC_NONE, NULL, CMD_FILESPEC_NONE, NULL}},
  {CHOIX_UNIVERSAL, PERM_READ,  'x', "Query query_string : performs a complex query - 'help query_syntax' for further info",
    {FALSE, CMD_FILESPEC_NONE, NULL, CMD_FILESPEC_NONE, NULL}},
  {CHOIX_UNIVERSAL, PERM_READ,  'x', "Where query_string : same as Query, kept for backwards  compatibility, do NOT use",
    {FALSE, CMD_FILESPEC_NONE, NULL, CMD_FILESPEC_NONE, NULL}},
  {CHOIX_UNIVERSAL, PERM_READ,  'T', "Table-Maker [-active] [ -h | -a | -p | -j | -x etc.] [-b start] [-c max] [-title]\n"
      "\t\t[-o out_filename] [-s obj_name] {command_infile | -n name | =} [params]\n"
      "\tcommands come from file, named Table class object in database (-n option), or command line (= option)\n"
      "\t-active searches active list, -s stores in database in Table class\n"
      "\toutput styles are as in List, Show (see them for additional style options)",
    {TRUE, CMD_FILESPEC_OPTIONAL | CMD_FILESPEC_FLAG, "-f", CMD_FILESPEC_OPTIONAL | CMD_FILESPEC_FLAG, "-o"}},
  {CHOIX_UNIVERSAL, PERM_READ,  'b', "Biblio [-abstracts] : shows the associated bibliography [including the abstracts]",
   {FALSE, CMD_FILESPEC_NONE, NULL, CMD_FILESPEC_NONE, NULL}},
  {CHOIX_UNIVERSAL, PERM_READ,  'd',
   "Dna [-mismatch] [-unspliced | -spliced | -cds] [-C | -p | -U]  [-x1 begin] [-x2 end] [[-f] out_filename] :\n"
   "\tFasta dump of related dna sequences\n"
   "\t-C      dump in \"C\" style struct\n"
   "\t-p      dump in perl style array\n"
   "\t-u      dump unformatted, just the dna as a single line.\n"
   "\t-mismatch          allow mismatches in construction of dna\n"
   "\t-unspliced | -spliced | -cds  for intron/exon objects, dna can be dumped for exons + introns,\n"
   "\t\tthe spliced exons or just the CDS within the spliced exons.\n"
   "\t x1 and x2 may be absent OR \"begin\" OR \"end\" OR a positive number,\n"
   "\t[-f] out_filename  output file for dna.\n",
   {TRUE, CMD_FILESPEC_NONE, NULL, CMD_FILESPEC_OPTIONAL, NULL}},
  {CHOIX_UNIVERSAL, PERM_READ,  'v',
   "Peptide [out_filename] : Fasta dump of related peptide sequences :\n"
   "\t-u      dump unformatted, just the peptide as a single line.\n",
   {TRUE, CMD_FILESPEC_NONE, NULL, CMD_FILESPEC_OPTIONAL, NULL}},
  /* Why are these '0' ?? check back in the old code...                      */
  /* 'a' contains no executed code, I just don't know about 'L'....          */
  { 0, 0,  'L', "Alignment [out_filename] : Fasta dump of related sequences",
    {TRUE, CMD_FILESPEC_NONE, NULL, CMD_FILESPEC_OPTIONAL, NULL}},
  { 0, 0,  'a', "Array class :name format : formatted acedump of A class object",
    {FALSE, CMD_FILESPEC_NONE, NULL, CMD_FILESPEC_NONE, NULL}},

  {CHOIX_UNIVERSAL, PERM_READ,  'K', "KeySet-Read [in_filename | = Locus a;b;Sequence c] : \n\tread keyset from file filename or from stdin or from command line",
    {TRUE, CMD_FILESPEC_OPTIONAL, NULL, CMD_FILESPEC_NONE, NULL}},

  {CHOIX_UNIVERSAL, PERM_READ,  '1', "spush : push active keyset onto stack",
    {FALSE, CMD_FILESPEC_NONE, NULL, CMD_FILESPEC_NONE, NULL}},
  {CHOIX_UNIVERSAL, PERM_READ,  '2', "spop  : remove top keyset from stack and make it the active keyset",
    {FALSE, CMD_FILESPEC_NONE, NULL, CMD_FILESPEC_NONE, NULL}},
  {CHOIX_UNIVERSAL, PERM_READ,  '3', "swap : swap keyset on top of stack and active keyset",
    {FALSE, CMD_FILESPEC_NONE, NULL, CMD_FILESPEC_NONE, NULL}},
  {CHOIX_UNIVERSAL, PERM_READ,  '4', "sand  : replace top of stack by intersection of top of stack and active keyset",
    {FALSE, CMD_FILESPEC_NONE, NULL, CMD_FILESPEC_NONE, NULL}},
  {CHOIX_UNIVERSAL, PERM_READ,  '5', "sor  : replace top of stack by union of top of stack and active keyset",
    {FALSE, CMD_FILESPEC_NONE, NULL, CMD_FILESPEC_NONE, NULL}},
  {CHOIX_UNIVERSAL, PERM_READ,  '6', "sxor  : replace top of stack by exclusive or of top of stack and active keyset",
    {FALSE, CMD_FILESPEC_NONE, NULL, CMD_FILESPEC_NONE, NULL}},
  {CHOIX_UNIVERSAL, PERM_READ,  '7', "sminus  : removes members of the active keyset from the top of the stack ",
    {FALSE, CMD_FILESPEC_NONE, NULL, CMD_FILESPEC_NONE, NULL}},
  {CHOIX_UNIVERSAL, PERM_READ,  '8', "spick nn : pick keyset nn>=0 from stack and make it the active keyset",
    {FALSE, CMD_FILESPEC_NONE, NULL, CMD_FILESPEC_NONE, NULL}},
  {CHOIX_UNIVERSAL, PERM_READ,  '9', "subset  : returns the subset of the alphabetic active keyset from x0>=1 of length nx ",
    {FALSE, CMD_FILESPEC_NONE, NULL, CMD_FILESPEC_NONE, NULL}},

  {CHOIX_UNIVERSAL, PERM_READ,  701, "kstore name  : store active keyset in internal tmp space as name",
    {FALSE, CMD_FILESPEC_NONE, NULL, CMD_FILESPEC_NONE, NULL}},
  {CHOIX_UNIVERSAL, PERM_READ,  702, "kget name  : copy named set as active keyset ",
    {FALSE, CMD_FILESPEC_NONE, NULL, CMD_FILESPEC_NONE, NULL}},
  {CHOIX_UNIVERSAL, PERM_READ,  703, "kclear name  : clear named set ",
    {FALSE, CMD_FILESPEC_NONE, NULL, CMD_FILESPEC_NONE, NULL}},

  
  /* parsing in server/client is done via -ace_in command */
  {CHOIX_NONSERVER, PERM_WRITE,  'p', "Parse [-NOXREF] [-v] [in_filename] : " UNIX_OR_WINDOWS_INPUTFILES "\n"
   "\t[-v] print detailed statistics about parsing",
   {TRUE, CMD_FILESPEC_OPTIONAL, NULL, CMD_FILESPEC_NONE, NULL}},
  {CHOIX_NONSERVER, PERM_WRITE,  'P', "PParse [-NOXREF] [-v] [in_filename] : as above, but don't stop on first error",
   {TRUE, CMD_FILESPEC_OPTIONAL, NULL, CMD_FILESPEC_NONE, NULL}},
  /* EVENTUALLY WHEN CLIENT SIDE FILE READING/WRITING IS IMPLEMENTED, THIS   */
  /* WILL GO AWAY BECAUSE PARSE WILL BE THE SAME FOR SERVER/NONSERVER.       */
  {CHOIX_RPC, PERM_WRITE,  'p', "Parse [-NOXREF] [-v] [in_filename] : parse a file defined on the server, \n\trun aceclient -ace_in to parse a file or stdin defined on the client side",
    {TRUE, CMD_FILESPEC_OPTIONAL, NULL, CMD_FILESPEC_NONE, NULL}},
  {CHOIX_RPC, PERM_WRITE,  'P', "PParse [-NOXREF] [-v] [in_filename] : as above, but don't stop on first error",
    {TRUE, CMD_FILESPEC_OPTIONAL, NULL, CMD_FILESPEC_NONE, NULL}},  
  {CHOIX_SERVER, PERM_WRITE,  '_', "serverparse : parse files in server mode",
    {TRUE, CMD_FILESPEC_NONE, NULL, CMD_FILESPEC_NONE, NULL}},  

  {CHOIX_UNIVERSAL, PERM_READ,  'w', "Write out_filename : acedump current list to file",
    {TRUE, CMD_FILESPEC_NONE, NULL, CMD_FILESPEC_COMPULSORY, NULL}},
  {CHOIX_UNIVERSAL, PERM_WRITE,  'e', "Edit ace_file_command : Apply the command to every member of the activelist",
    {FALSE, CMD_FILESPEC_NONE, NULL, CMD_FILESPEC_NONE, NULL}},
  {CHOIX_UNIVERSAL, PERM_WRITE,  'E', "EEdit ace_file_command : Apply the command to every member of the activelist, "
   "but unlike \"Edit\", conintue processing objects even if there is an error.",
    {FALSE, CMD_FILESPEC_NONE, NULL, CMD_FILESPEC_NONE, NULL}},
  {CHOIX_UNIVERSAL, PERM_WRITE,  'k',
   "Kill : Kill and remove from acedb all objects in the current list",
    {FALSE, CMD_FILESPEC_NONE, NULL, CMD_FILESPEC_NONE, NULL}},

  /* encore stuff...these should not be included in the normal command sets/ */
  /* perms because they are used internally by the code and are not for the  */
  /* user.                                                                   */
  {CHOIX_UNDEFINED, PERM_UNDEFINED, 'm', "More : If there are more objects to list",
    {FALSE, CMD_FILESPEC_NONE, NULL, CMD_FILESPEC_NONE, NULL}}, 
  {CHOIX_UNDEFINED, PERM_UNDEFINED,  'M', "More : If there are more objects to show",
    {FALSE, CMD_FILESPEC_NONE, NULL, CMD_FILESPEC_NONE, NULL}}, 
  {CHOIX_UNDEFINED, PERM_UNDEFINED,  'B', "More : If there are more lines in the table",
    {FALSE, CMD_FILESPEC_NONE, NULL, CMD_FILESPEC_NONE, NULL}}, 
  {CHOIX_UNDEFINED, PERM_UNDEFINED,  'D', "More : If there are more dna or peptide to dump",
    {FALSE, CMD_FILESPEC_NONE, NULL, CMD_FILESPEC_NONE, NULL}},

  {CHOIX_UNIVERSAL, PERM_ADMIN,  'R',
   "Read-models [-index]: reads models, [reindex whole database] you must own the database to do this",
   {FALSE, CMD_FILESPEC_NONE, NULL, CMD_FILESPEC_NONE, NULL}},

  {CHOIX_NONSERVER, PERM_WRITE,  'Y', "tace [-f in_filename] [parameters] : Execute a file of acedb commands",
    {FALSE, CMD_FILESPEC_OPTIONAL | CMD_FILESPEC_FLAG, "-f", CMD_FILESPEC_NONE, NULL}},

#ifdef ACEMBLY
  {CHOIX_UNIVERSAL, PERM_WRITE,  'A', "Acembly [-f in_command_file] [parameters] : Sequence assembly and automatic edition",
    {FALSE, CMD_FILESPEC_NONE, NULL, CMD_FILESPEC_NONE, NULL}},
#endif /* ACEMBLY */

  {CHOIX_UNIVERSAL, PERM_READ,  'Z', "Status {on | off} : Show current stats or toggle statistics display\n"
   "\t{" STATUS_OPTIONS_STRING "} : select stats to print\n",
   {FALSE, CMD_FILESPEC_NONE, NULL, CMD_FILESPEC_NONE, NULL}},
  {CHOIX_UNIVERSAL, PERM_READ, 'J',
   "Date : prints the date and time",
   {FALSE, CMD_FILESPEC_NONE, NULL, CMD_FILESPEC_NONE, NULL}},
  {CHOIX_UNIVERSAL, PERM_READ,  'I', "Time_Stamps {on | off} : Show current setting or toggle time stamps creation",
    {FALSE, CMD_FILESPEC_NONE, NULL, CMD_FILESPEC_NONE, NULL}},

  {CHOIX_UNIVERSAL, PERM_WRITE,  'H', "Chrono {start | stop | show} built in profiler of chrono aware routines",
    {FALSE, CMD_FILESPEC_NONE, NULL, CMD_FILESPEC_NONE, NULL}},

  {CHOIX_UNIVERSAL, PERM_ADMIN,  'V', "Test : test subroutine, may vary",
    {FALSE, CMD_FILESPEC_NONE, NULL, CMD_FILESPEC_NONE, NULL}},

  {CHOIX_UNIVERSAL, PERM_READ,  450,
   "AQL [-h | -a | -p | -j | -x etc.] [-s] [ -f in_query_file] [-o out_results_file] <aql query>\n"
   "\tnew ace query language, -s for silent\n"
   "\toutput styles are as in List, Show (see them for additional style options) plus -A",
   {TRUE, CMD_FILESPEC_OPTIONAL | CMD_FILESPEC_FLAG, "-f", CMD_FILESPEC_OPTIONAL | CMD_FILESPEC_FLAG, "-o"}},
  {CHOIX_UNIVERSAL, PERM_READ,  451, "Select ... <aql query> : new ace query language, use AQL to get options",
    {FALSE, CMD_FILESPEC_NONE, NULL, CMD_FILESPEC_NONE, NULL}},

  {CHOIX_UNIVERSAL, PERM_WRITE,  'F', "New Class Format : i.e New Plate ya\\%dx.y creates ya8x.y if ya7x.y exists",
    {FALSE, CMD_FILESPEC_NONE, NULL, CMD_FILESPEC_NONE, NULL}},
  {CHOIX_UNIVERSAL, PERM_READ,  'N', "Count : number of objects in active keyset",
    {FALSE, CMD_FILESPEC_NONE, NULL, CMD_FILESPEC_NONE, NULL}},
  {CHOIX_UNIVERSAL, PERM_READ,  'C', "Clear : Clear current keyset",
    {FALSE, CMD_FILESPEC_NONE, NULL, CMD_FILESPEC_NONE, NULL}},
  {CHOIX_UNIVERSAL, PERM_READ,  'u', "Undo : returns the current list to its previous state",
    {FALSE, CMD_FILESPEC_NONE, NULL, CMD_FILESPEC_NONE, NULL}},

  /* where do these fit in, looks like more specials...are these for assembly*/
  /* somewhere ?? I don't know if these are used anywhere, can't see how     */
  /* they get called...                                                      */
  { 0,  0, '-', "Depad [seqname] : depads an assembly, or currently padded assembly",
    {FALSE, CMD_FILESPEC_NONE, NULL, CMD_FILESPEC_NONE, NULL}},
  { 0,  0, '*', "Pad seqname : pads an assembly",
    {FALSE, CMD_FILESPEC_NONE, NULL, CMD_FILESPEC_NONE, NULL}},

  {CHOIX_GIF, PERM_READ, '#', "GIF : access to gif drawing submenu",
    {FALSE, CMD_FILESPEC_NONE, NULL, CMD_FILESPEC_NONE, NULL}},
  {CHOIX_UNIVERSAL, PERM_READ, 401, "layout : export the layout file",
    {FALSE, CMD_FILESPEC_NONE, NULL, CMD_FILESPEC_NONE, NULL}},

  {CHOIX_NONSERVER, PERM_ADMIN,  'j', "Dump [-s] [-T] [-C] [directory]: dump database, -s splits by class, -T gives timestamps, -C ignore comments",
    {FALSE, CMD_FILESPEC_NONE, NULL, CMD_FILESPEC_NONE, NULL}},
  {CHOIX_NONSERVER, PERM_ADMIN,  'Q', "dumpread filename : read in a dump, filename must be first dumpfile of a set of dump files.",
    {FALSE, CMD_FILESPEC_NONE, NULL, CMD_FILESPEC_NONE, NULL}},


  /* New, admin commands.                                                    */
  {CHOIX_NONSERVER, PERM_ADMIN,  CMD_NEWLOG, "newlog [-f old_log_name | -reopen] :\n"
   "Starts a new log file renaming the old log as \"log.<ace-time>.wrm\"\n"
   "\t-f allows you to specify a name for the old log file.\n"
   "\t-reopen simply closes the log and then reopens it, this is useful with system rotatelogs script\n"
   "\t        supplied with some unix systems that can be used to control general log files",
    {FALSE, CMD_FILESPEC_NONE, NULL, CMD_FILESPEC_NONE, NULL}},


  /* Smap commands... */
  {CHOIX_UNIVERSAL, PERM_READ,  CMD_SMAP,
   "smap [-coords x1 x2] -from class:object [-to class:object] : "
   "maps cordinates between sequences that have a child -> parent relationship expressed in "
   "SMap tags (or the now deprecated Source/Subsequence tags). Note that objects must be "
   "specified in \"class:object\" as smap can map any class that contains SMap tags.\n"
   "\t-coords x1 x2      coords to be mapped in the -from object, defaults to whole of -from object.\n"
   "\t-from class:object object to be mapped into one of its parent.\n"
   "\t-to class:object   object to be mapped to, must be a parent of the -from object, defaults to "
   "top parent in the smap tree above -from object.",
   {FALSE, CMD_FILESPEC_NONE, NULL, CMD_FILESPEC_NONE, NULL}},
  {CHOIX_UNIVERSAL, PERM_READ,  CMD_SMAPDUMP, "smapdump [-f filename] class:object : "
   "dumps to stdout (or optionally a file) the smap for the object.",
   {FALSE, CMD_FILESPEC_NONE, NULL, CMD_FILESPEC_NONE, NULL}},
  {CHOIX_UNIVERSAL, PERM_READ,  CMD_SMAPLENGTH, "smaplength class:object : "
   "gives the length of the object.",
   {FALSE, CMD_FILESPEC_NONE, NULL, CMD_FILESPEC_NONE, NULL}},


  /* server only commands.                                                   */
  {CHOIX_RPC, PERM_ADMIN,  CMD_SHUTDOWN, "shutdown [now] [norestart] : Closes the server (you need special privilege)",
    {FALSE, CMD_FILESPEC_NONE, NULL, CMD_FILESPEC_NONE, NULL}},
  {CHOIX_RPC, PERM_ADMIN,  CMD_WHO,      "who : who is connected now (you need special privilege)",
    {FALSE, CMD_FILESPEC_NONE, NULL, CMD_FILESPEC_NONE, NULL}},
  {CHOIX_RPC, PERM_ADMIN,  CMD_VERSION,  "data_version : data served now, as stated in wspec/server.wrm",
    {FALSE, CMD_FILESPEC_NONE, NULL, CMD_FILESPEC_NONE, NULL}},
  {CHOIX_RPC, PERM_ADMIN,  CMD_WSPEC,    "wspec : export wspec directory as a tar file",
    {FALSE, CMD_FILESPEC_NONE, NULL, CMD_FILESPEC_NONE, NULL}},

  /* admin only commands (socket server only).                               */
  {CHOIX_SERVER, PERM_ADMIN,  CMD_PASSWD,   "passwd : change your password.",
    {FALSE, CMD_FILESPEC_NONE, NULL, CMD_FILESPEC_NONE, NULL}},  
  {CHOIX_SERVER, PERM_ADMIN,  CMD_USER,     "user passwd|group|new|delete userid [password group] : Create/update/delete userids (you need special privilege).",
    {FALSE, CMD_FILESPEC_NONE, NULL, CMD_FILESPEC_NONE, NULL}},  
  {CHOIX_SERVER, PERM_ADMIN,  CMD_GLOBAL,   "global read|write none|passwd|world : Change global read/write permissions for database (you need special privilege).",
    {FALSE, CMD_FILESPEC_NONE, NULL, CMD_FILESPEC_NONE, NULL}},  
  {CHOIX_SERVER, PERM_ADMIN,  CMD_DOMAIN,   "domain group|new|delete domain-name [group] : Create/update/delete domain names (you need special privilege).",
    {FALSE, CMD_FILESPEC_NONE, NULL, CMD_FILESPEC_NONE, NULL}},  
  {CHOIX_SERVER, PERM_ADMIN,  CMD_REMOTEPARSE, "Parseremote  [-NOXREF] [-v] in_filename : parse a file defined on the servers filesystem.",
    {TRUE, CMD_FILESPEC_OPTIONAL, NULL, CMD_FILESPEC_NONE, NULL}},
  {CHOIX_SERVER, PERM_ADMIN,  CMD_REMOTEPPARSE, "PParseremote [-NOXREF] [-v] in_filename : as above, but don't stop on first error",
    {TRUE, CMD_FILESPEC_OPTIONAL, NULL, CMD_FILESPEC_NONE, NULL}},  
  {CHOIX_SERVER, PERM_ADMIN,  CMD_REMOTEDUMP, "Dumpremote [-s] [-T] [-C] directory : dump database to a file defined on the servers filesystem,\n\t -s splits by class, -T gives timestamps, -C ignore comments",
    {FALSE, CMD_FILESPEC_NONE, NULL, CMD_FILESPEC_NONE, NULL}},
  {CHOIX_SERVER, PERM_ADMIN,  CMD_REMOTEDUMPREAD, "dumpreadremote filename : read in a dump held on the servers filesystem, filename must be first dumpfile of a set of dump files.",
    {FALSE, CMD_FILESPEC_NONE, NULL, CMD_FILESPEC_NONE, NULL}},
  {CHOIX_SERVER, PERM_ADMIN,  CMD_SERVERLOG, "Serverlog [-f old_log_name | -reopen] : "
   "Starts new log and serverlog files renaming the old logs as \"(server)log.<ace-time>.wrm\"\n"
   "\t-f allows you to specify a name for the old log file."
   "\t-reopen simply closes the log and then reopens it, this is useful with system rotatelogs script\n"
   "\t        supplied with some unix systems that can be used to control general log files",
    {FALSE, CMD_FILESPEC_NONE, NULL, CMD_FILESPEC_NONE, NULL}},  

  /* Special end of table entry.                                             */
  {CHOIX_UNDEFINED, PERM_UNDEFINED, 0, NULL, {FALSE, CMD_FILESPEC_NONE, NULL, CMD_FILESPEC_NONE, NULL}}
} ;



/************************************************************/
/* Given the integer representing a command, returns what group that command */
/* belongs to, e.g. CHOIX_SERVER.                                            */
/* If the command is not known, returns  CHOIX_UNDEFINED                     */

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
/* THIS IS NOT NEEDED AT THE MOMENT, I HAVE DONE WHAT I NEED TO BY USING     */
/* THE choix MECHANISM.                                                      */

CmdChoix aceCommandGetChoixGroup(KEY key)
{
  CmdChoix group = CHOIX_UNDEFINED ;
  CHOIX *fc ;

  fc = uAceCommandGetChoixMenu() ;

  while (fc->key && group == CHOIX_UNDEFINED)		    /* watch for the loop termination... */
    {
      if (fc->key == key)
	group = fc->choix ;

      fc++ ;
    }

  return group ;
}
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */


/************************************************************/

/* THIS IS NOT FINISHED...                                                   */
/* N.B. we just return CMD_FILE_ERR whether the command itself was in error, */
/* or whether the way the file was specified was the error.                  */
/*                                                                           */
CmdFilespecArg aceCommandCheckForFiles(char *cmd_string, char **newcmd_string,
				       char **input_file, char **output_file)
{
  CmdFilespecArg result ;
  Array cmd_menu ;
  unsigned int cmd_set, perm_set ;
  BOOL answer ;
  ACEIN command_in ;
  FREEOPT *qmenu ;
  KEY option ;
  char *dummy ;


  /* Default result.                                                         */
  result = CMD_FILE_NONE ;
  *input_file = *output_file = *newcmd_string = NULL ;

  /* Is the command one of the recognised ones ?  Note we have to do some    */
  /* contortions here to enable us to use existing code in acein to check    */
  /* for an option. We choose all possible commands and get returned a menu  */
  /* which we can then feed into aceInKey...phew...                          */
  cmd_set = CHOIX_UNIVERSAL | CHOIX_NONSERVER | CHOIX_SERVER | CHOIX_GIF ;
  perm_set = PERM_READ | PERM_WRITE | PERM_ADMIN ;
  cmd_menu = uAceCommandChoisirMenu(cmd_set, perm_set) ;
  qmenu = arrp (cmd_menu, 0, FREEOPT) ;
  command_in = aceInCreateFromText(cmd_string, NULL, 0) ;
  dummy = aceInCard(command_in) ;
  answer = aceInKey(command_in, &option, qmenu) ;


  /* Ok, now check for an input or output file for the command.              */
  if (!answer)
    result = CMD_FILE_ERR ;				    /* unknown cmd. */
  else
    {
      BOOL found ;
      CHOIX *cmd_description ;

      /* Find the command description in the command.c array of commands,    */
      /* the description tells how to process possible file arguments.       */
      found = FALSE ;
      cmd_description = uAceCommandGetChoixMenu() ;
      while (cmd_description->key && found == FALSE)
	{
	  if (cmd_description->key == option)
	      found = TRUE ;
	  else
	    cmd_description++ ;
	}
      if (found == FALSE)
	messcrash("Could not find \"%s\" in the command.c array of commands.", cmd_string) ;


      /* Does the command use any files ?  If yes, then check them out.      */
      if (!(cmd_description->filespec.any_files))
	result = CMD_FILE_NONE ;			    /* This cmd does not use files. */
      else
	{
	  BOOL more_words ;
	  CmdFilespecArg status ;
	  char *word, prev_word[1000] ;
	  char *infile, *outfile ;
	  Stack cmd_out ;

	  /* aceInKey (above) does an aceInWord, so we need to go back and   */
	  /* get the first word again, clumsy really, could do with an       */
	  /* aceInWordBack.                                                  */
	  aceInCardBack(command_in) ;
	  dummy = aceInCard(command_in) ;
	  word = aceInWord(command_in) ;

	  /* Use a stack to build the command minus the file args.           */
	  cmd_out = stackCreate(1000) ;
	  pushText(cmd_out, word) ;

	  /* Parse out the file args (if any).                               */
	  more_words = TRUE ;
	  word = infile = outfile = NULL ;
	  prev_word[0] = '\0' ; 
	  while (more_words)
	    {
	      status = CMD_FILE_NONE ;
	      word = aceInWord(command_in) ;
	      if (word == NULL)
		more_words = FALSE ;
	      else
		strcpy(&(prev_word[0]), word) ;

	      if (word)
		{
		  /* Still chonking through text, looking for flags...       */
		  if ((cmd_description->filespec.infile & CMD_FILESPEC_FLAG)
		      && infile == NULL
		      && status == CMD_FILE_NONE)
		    status = getFlagFile(&infile, cmd_description->filespec.infile_flag,
					 word, command_in) ;
		  
		  if ((cmd_description->filespec.outfile & CMD_FILESPEC_FLAG)
		      && outfile == NULL
		      && status == CMD_FILE_NONE)
		    status = getFlagFile(&outfile, cmd_description->filespec.outfile_flag,
					 word, command_in) ;

		}
	      else
		{
		  /* Got to the end of text, check for final file name.      */
		  /* N.B. can only be one of an input OR an output file.     */
		  if ((cmd_description->filespec.infile != CMD_FILESPEC_NONE)
		      && !(cmd_description->filespec.infile & CMD_FILESPEC_FLAG)
		      && status == CMD_FILE_NONE)
		    status = getNoFlagFile(&infile, &(prev_word[0]),
					   cmd_description->filespec.infile) ;
		  else if ((cmd_description->filespec.outfile != CMD_FILESPEC_NONE)
		      && !(cmd_description->filespec.outfile & CMD_FILESPEC_FLAG)
		      && status == CMD_FILE_NONE)
		    status = getNoFlagFile(&outfile, &(prev_word[0]),
					   cmd_description->filespec.outfile) ;
		}
	      
	      if (status == CMD_FILE_ERR)
		{
		  result = status ;
		  break ;
		}
	      else if (status == CMD_FILE_FOUND)
		{
		  result = status ;
		  prev_word[0] = '\0' ;
		}

	      if (prev_word[0] != '\0')
		{
		  catText(cmd_out, " ") ;
		  catText(cmd_out, &(prev_word[0])) ;
		}

	    } /* while */


	  /* Set up return values.                                           */
	  if (result == CMD_FILE_FOUND)
	    {
	      if (infile)
		*input_file = infile ;
	      if (outfile)
		*output_file = outfile ;
	      
	      *newcmd_string = strnew(popText(cmd_out), 0) ;
	    }

	  stackDestroy(cmd_out) ;

	} /* cmd uses files */

    } /* command is recognised */

  aceInDestroy(command_in) ;

  return result ;
}


/******************************************************************
 ************************ private functions ***********************
 ******************************************************************/


/* Access function to the main command array.                                */
/*                                                                           */
CHOIX *uAceCommandGetChoixMenu(void)
{
  return &(choixMenu[0]) ;
}


/****************************************************************/

/* Return an array of the allowed commands for the given choix and           */
/* permissions sets (e.g. choix might be universal commands and permission   */
/* might be read-only).                                                      */
/*                                                                           */
/* Note we do NO checking of arguments here because this is a private        */
/* function to the command package so the code just should be correct.       */
/*                                                                           */
Array uAceCommandChoisirMenu(unsigned int choix, unsigned int perms)
{
  Array aMenu = arrayCreate (50, FREEOPT) ;
  int i ;
  CHOIX *fc = uAceCommandGetChoixMenu() ;
  FREEOPT *ff ;

  i = 1 ;
  while (fc->key)
    {
      if ((fc->choix & choix) && (fc->perm & perms))
	{
	  ff = arrayp (aMenu, i++, FREEOPT) ;
	  ff->key = fc->key ;
	  ff->text = fc->text ;
	}
      fc++ ;
    }

  ff = arrayp (aMenu, 0, FREEOPT) ;
  ff->key = i - 1 ;
  ff->text = "acedb" ;

  return aMenu ;
} /* choisirMenu */


/****************************************************************/

static CmdFilespecArg getFlagFile(char **file_name, char *flag, char *curr_word, ACEIN curr_string)
{
  CmdFilespecArg file_found = CMD_FILE_NONE ;

  if (strcmp(curr_word, flag) == 0)
    {
      char *infile ;
      
      infile = aceInWord(curr_string) ;

      if (infile && *infile != '-')
	{
	  *file_name = strnew(infile, 0) ;
	  file_found = CMD_FILE_FOUND ;
	}
      else
	file_found = CMD_FILE_ERR ;
    }

  return file_found ;
}

/****************************************************************/

static CmdFilespecArg getNoFlagFile(char **file_name, char *curr_word, CmdFilespecType filespec)
{
  CmdFilespecArg file_found = CMD_FILE_NONE ;

  if (curr_word == NULL)
    {
      if ((filespec & CMD_FILESPEC_COMPULSORY))
	file_found = CMD_FILE_ERR ;
      else
	file_found = CMD_FILE_NONE ;
    }
  else if (*curr_word == '-')
    file_found = CMD_FILE_ERR ;
  else
    {
      *file_name = strnew(curr_word, 0) ;
      file_found = CMD_FILE_FOUND ;
    }

  return  file_found ;
}



