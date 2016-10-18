/*  File: aql.c
 *  Author: Fred Wobus (fw@sanger.ac.uk)
 *  Copyright (C) J Thierry-Mieg and R Durbin, 1997
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
 * 	Richard Durbin (MRC LMB, UK) rd@sanger.ac.uk, and
 *	Jean Thierry-Mieg (CRBM du CNRS, France) mieg@kaa.cnrs-mop.fr
 *
 * SCCS: $Id: taqlmain.c,v 1.21 2008-09-24 10:28:33 edgrif Exp $
 * Description: top main-function for stand-alone AQL application
 * Exported functions:
 * HISTORY:
 * Last edited: Sep 24 11:14 2008 (edgrif)
 * * Mar 22 14:45 1999 (edgrif): Replaced messErrorInit with messSetMsgInfo
 * * Dec  2 16:27 1998 (edgrif): Added code to record build time of this module.
 * * Aug 17 16:53 1998 (fw): changed tableOut style to 'a' (like tace now)
 * * Aug  3 14:53 1998 (fw): accept an optional debug level parameter for -d option
 * * Jul 30 09:26 1998 (fw): check for env-var $ACEDB
 * * Jul 29 10:09 1998 (fw): reintroduced the prompt for number of line to display
 * * Jul 28 17:42 1998 (fw): use of aqlQueryExecute(), testQuery removed
 * * Jul 23 14:53 1998 (fw): removed the single value return-value thing
                             as aqlEval now always returns a table
 * * Jul 16 16:55 1998 (fw): accounted for scalar return value instead of table
 * * Jul 16 16:35 1998 (fw): slightly changed the interface,
                             global variable isDebug, activated
			     by -d option, removed -u option instead
 * Created: Wed Jul 22 11:16:44 1998 (fw)
 *-------------------------------------------------------------------
 */
/***************************************************************/

#include <locale.h>
#include <wh/acedb.h>
#include <wh/aceio.h>
#include <wh/aql.h>
#include <wh/table.h>
#include <wh/aceio.h>
#include <wh/apputils.h>
#include <wh/session.h>
#include <wh/version.h>
#include <wh/aceversion.h>
#include <wh/sigsubs.h>
#include <wh/pref.h>
#include <readline/readline.h>

/***************************************************************/

/* Defines a routine to return the compile date of this file.                */
UT_MAKE_GETCOMPILEDATEROUTINE()

static void printUsage (void);

/***************************************************************/
/* command-line interface for AQL in a stand-alone application */
/***************************************************************/

int main (int argc, char **argv)
{
  char *cp;
  char *dbDir;
  BOOL isQuiet = FALSE ;
  char outputStyle = 'h';
  int parseDebugLevel = 0;		/* default if no -d is given */
  int evalDebugLevel = 0;		/* default if no -d is given */
  int paramValueInt;
  float paramValueFloat;
  mytime_t paramValueDate;
  char *paramValueText = NULL;
  char *paramName = NULL;
  char *paramType = NULL;
  ACEOUT fo;
  BOOL isInteractive;
  char *value;
  Stack queryStack;
  char *outputfilename = NULL;
  char *cl_query = NULL;
  char *locale ;


  /* We cannot cope with anything other than the standard locale. */
  locale = setlocale(LC_ALL, "C") ;

  /* init message system details. */
  messSetMsgInfo(argv[0], aceGetVersionString()) ;

  /**** parse command-line arguments ******/
  /* Does not return if user typed "-version" option.                        */
  appCmdlineVersion(&argc, argv) ;

  /*****************************/

  prefInit() ;						    /* Set up App preferences. */

  /*****************************/
  isInteractive = TRUE ;
  /* do NOT test stdinIsInteractive(), 
   * this would  changes the behaviour of the
   * code in pipe calls
   */







  ++argv ; --argc ;
  if (argc && (!strcmp("-h", *argv) || !strcmp("-help", *argv)))
    {
      printUsage();
      return EXIT_FAILURE;
    }

  /*****************************/

  /* There is a problem with finding this symbol in the readline lib. on darwin.... */
#if !defined(DARWIN)
  rl_extend_line_buffer (2000);
#endif

  while (argc) 
  {
      if (strcmp("-d", *argv) == 0)
      { 
          parseDebugLevel = evalDebugLevel = 1;		/* default if -d is given */
          ++argv ; --argc ;

          if (argc && (*argv[0] >= '0' && *argv[0] <= '4'))
          {
              int l;
              sscanf (*argv, "%d", &l);
              parseDebugLevel = evalDebugLevel = l;
          }
      }
      else if (strcmp("-dp", *argv) == 0)
      { 
          parseDebugLevel = 1;		/* default if -d is given */
          ++argv ; --argc ;

          if (argc && (*argv[0] >= '0' && *argv[0] <= '4'))
          {
              int l;
              sscanf (*argv, "%d", &l);
              parseDebugLevel = l;
          }
      }
      else if (strcmp("-de", *argv) == 0)
      { 
          evalDebugLevel = 1;		/* default if -d is given */
          ++argv ; --argc ;

          if (argc && (*argv[0] >= '0' && *argv[0] <= '4'))
          {
              int l;
              sscanf (*argv, "%d", &l);
              evalDebugLevel = l;
          }
      }
      else if (strcmp("-q", *argv) == 0)
      { 
          isQuiet = TRUE ;
          isInteractive = FALSE;
          putenv ("ACEDB_NO_BANNER="); /* silence the kernel as well */
      }
      else if (strcmp("-a", *argv) == 0)
      {
          outputStyle = 'a';
      }
      else if (strcmp("-A", *argv) == 0)
      {
          outputStyle = 'A';
      }
      else if (strcmp("-j", *argv) == 0)
      {
          outputStyle = 'j';
      }
      else if (strcmp("-J", *argv) == 0)
      {
          outputStyle = 'J';
      }
      else if (strcmp("-param", *argv) == 0 && paramName == NULL)
      {
          ++argv ; --argc ;	  

          if (argc < 3)
          {
              printUsage();
              return EXIT_FAILURE;
          }
	  
          paramName = strnew (*argv, 0);
          ++argv ; --argc ;	  

          paramType = strnew (*argv, 0);
          ++argv ; --argc ;	  

          if (strncasecmp(paramType, "Int", 3) == 0)
              sscanf (*argv, "%d", &paramValueInt);
          else if (strcasecmp(paramType, "Float") == 0)
              sscanf (*argv, "%f", &paramValueFloat);
          else if (strncasecmp(paramType, "Date", 4) == 0)
              paramValueDate = timeParse(*argv);
          else if (strcasecmp(paramType, "Text") == 0 || strcasecmp(paramType, "String") == 0)
              paramValueText = strnew(*argv, 0);
          else
          {
              printUsage();
              return EXIT_FAILURE;
          }

      }
      else if (strcmp("-o", *argv) == 0)
      {
          ++argv ; --argc ;
          if (argc > 0)
          {
              outputfilename = *argv;
          }
      }
      else if (strcmp("-e", *argv) == 0)
      {
          ++argv ; --argc ;
          if (argc > 0)
          {
              cl_query = *argv;
          }
      }
      else
      {

          /* the next command line argument could be a database directory */

          if (argc == 0)
              /* no more command line args, check for $ACEDB */
          {
              if (!(dbDir = getenv("ACEDB")))
                  /* no env-var set, so show usage */
              {
                  printUsage();
                  return EXIT_FAILURE;
              }
          }
          else
              /* we do have another command line argument, 
                 so we take that to be the database directory */
              dbDir = *argv;
      }
      ++argv ; --argc ;	  
  }

  sessionInit (dbDir) ;

  /* Set some process resources to unlimited, as we are running interactive  */
  /* the user is given chance to exit if some resources are too limited.     */
  utUnlimitResources(TRUE) ;

  /* Check "-tsuser" option to allow a different user to be specified as the database 
   * user to be recorded in the timestamps. */
  appTSUser(&argc, argv) ;

  signalCatchInit(TRUE,					    /* init Ctrl-C as well */
		  (getCmdLineOption(&argc, argv, NOSIGCATCH_OPT, NULL) ? TRUE : FALSE)) ;

  if (outputfilename != NULL) {
      fo = aceOutCreateToFile (outputfilename, "w", 0);
  } else {
      fo = aceOutCreateToStdout (0);
  }

  queryStack = stackCreate (1000);

  while (TRUE) { 

      char *line = NULL;
      
      if (cl_query == NULL) {
          char *prompt;

          if (strlen(stackText(queryStack, 0)) == 0)
              prompt = "acedb-aql> ";
          else
              prompt = "> ";		/* continuation line */

          line = readline (prompt);

          if (line == NULL)		/* got EOF */
              break;

      }
      if (cl_query == NULL && (strlen(line) > 0))
      {
          if (strlen(stackText(queryStack, 0)) > 0)
              catText (queryStack, "\n");
          catText (queryStack, line);

          if (line != NULL) free (line);
      }
      else
      { 
          AQL    aql;
          TABLE *result;
          char *query = (cl_query != NULL) ? cl_query : stackText(queryStack, 0);
          int i;

          if (line != NULL) free (line);

          add_history(query);

          for (i = 0; i < strlen(query); ++i)
              if (query[i] == '\n')
                  query[i] = ' ';

          if (paramName != NULL)
          {
              aql = aqlCreate(query, fo, 0, parseDebugLevel, evalDebugLevel, messprintf("$%s", paramName), (char *)NULL) ; 

              if (strncasecmp(paramType, "Int", 3) == 0)
                  result = aqlExecute(aql, 0, 0, 0, messprintf ("%s $%s", paramType, paramName), paramValueInt, (char *)NULL) ;
              else if (strcasecmp(paramType, "Float") == 0)
                  result = aqlExecute(aql, 0, 0, 0, messprintf ("%s $%s", paramType, paramName), paramValueFloat, (char *)NULL) ;
              else if (strncasecmp(paramType, "Date", 4) == 0)
                  result = aqlExecute(aql, 0, 0, 0, messprintf ("%s $%s", paramType, paramName), paramValueDate, (char *)NULL) ;
              else if (strcasecmp(paramType, "Text") == 0 || strcasecmp(paramType, "String") == 0)
                  result = aqlExecute(aql, 0, 0, 0, messprintf ("%s $%s", paramType, paramName), paramValueText, (char *)NULL) ;
          }
          else
          {
              aql = aqlCreate(query,
                              fo,
                              0,                /* beauty */
                              parseDebugLevel,       /* parseDebugLevel*/
                              evalDebugLevel,       /* evalDebugLevel */
                              (char *)NULL) ;                 /* terminator */

              result = aqlExecute(aql, NULL, NULL, NULL, (char *)NULL);
          }


          if (aqlIsError(aql))
          {
              aceOutPrint (fo, "%s", aqlGetErrorReport(aql));
          }
          else
          {
              /* output the results table */

              if (isInteractive)
                  aceOutPrint (fo, "Result table with %d line%s\n",
                               tableMax(result),
                               tableMax(result) == 1 ? "" : "s") ;

              /* output the table with selected style */
              if (isInteractive)
                  aceOutPrint (fo, "------------------------------\n");

              tableOut (fo, result, '\t', outputStyle) ;

              if (isInteractive)
                  aceOutPrint (fo, "------------------------------\n");
          }
          aqlDestroy (aql);

          tableDestroy (result);

          stackClear (queryStack);
      }
      if (cl_query != NULL) {
          break;
      }
  } /* end-while */


  if (isInteractive)
    aceOutPrint (fo, "\n");

  sessionClose (FALSE);		/* clean-up without save */

  aceOutDestroy (fo);

  return (EXIT_SUCCESS);
} /* main */

/************************************************************/

static void printUsage (void)
{
  printf ("\n"
	  "  This is a simple command line interface for AQL.  \n");
  printf ("  Type in a query (multiple lines are fine), followed by a blank line.\n"
	  "  Syntax and examples on http://www.sanger.ac.uk/Software/Acedb/AQL/\n"
	  "  Please record crashes and errors for Fred Wobus (details please).\n\n") ;
  
  printf (" Usage:  \n");
  printf ("       taql -h       prints this info\n\n");
  
  printf ("       taql [ options ] [<database>]\n");
  printf ("    Where options may include\n");
  printf ("           -q        quiet run - no messages (prompts etc.) to terminal\n"
	  "                     useful if the program is run as 'taql -q < queryfile.txt'\n"
	  "                     e.g. from within a script.\n\n");
  
  printf ("           -a        output in Acedb format (strings are quoted etc.)\n"
	  "           -A        output in extended Acedb format (results are quoted with their class)\n"
	  "           -j        output in Java parsable format\n"
	  "           -J        output in 'Java Style' format\n"
	  "               the default output style is plain, i.e. just text\n\n");
  
  printf ("           -param <name> <type> <value>\n"
	  "                     pass a scalar context variable\n"
	  "               name  - variable name, the query may then refer to $name\n"
	  "               type  - one of Int, Float, Text, DateType\n"
	  "               value - the scalar value, text values may have to be quoted\n\n");
  
  printf ("           -d        gives all technical debug info\n"
	  "               level 0 - no debug info\n"
	  "                     1 - internal query structure (default if -d is set)\n"
	  "                     2 - all intermediate parsetrees\n"
	  "                     3 - some tracking infor during query exection\n"
	  "                     4 - all debug info during query execution\n\n") ;
  
  printf ("    The <database> parameter is the dircetory the ACeDB database to be accessed.\n"
	  "           if no database is specified on the command line the value\n"
	  "           of the environment variable $ACEDB is enquired.\n\n");

  return;
} /* printUsage */


/**********************************************************************/
