/*  File: gnbkclient.c
 *  Author: Lincoln Stein (Whitehead) and Jean Thierry-Mieg
 *  Copyright (C) L Stein and J Thierry-Mieg, 1996
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
 * Modified from netclient to handle communication
 * with a gnbk server
 * 
 * Exported functions:
 * HISTORY:
 * Last edited: Feb 25 21:36 1996 (mieg)
 * Created: Wed Nov 25 20:02:45 1992 (mieg)
 *-------------------------------------------------------------------
 */

 /* $Id: gnbkclient.c,v 1.5 2002-05-13 12:59:40 srk Exp $ */

#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <ctype.h>
#include <errno.h>
#if !defined(DARWIN)
#include <malloc.h>
#endif
#include "regular.h"
#include "array.h"

#define OK 200
#define GOODBYE 201
#define DEBUG 301
#define COMMENT 302
#define REDIRECT 303
#define PARAMERROR 401
#define SYNTAXERROR 402
#define UNIMPLEMENTED 403
#define TIMEOUTERROR 501
#define COMMERROR 502
#define MEMERROR 503
#define FILEERROR 504

extern BOOL openServer(char *host, u_long rpc_port) ;
extern int askServer(char *request, char **answerPtr) ;

void writeStatus (int status,char* message) {
  fprintf(stdout,"%d %s\r\n",status,message);
  fflush(stdout);
}

/*************************************************************************/

void doQuery(char* query) 
{
  char* answer = 0 ;
  int retval = askServer(query,&answer) ;

  if (retval > 0) 
    printf ("Server error code %d",retval);
  else
    printf (answer) ;
  messfree(answer);
}

/*************************************************************************/
void printHelp () {
  char* helpText[] = {
    "- Commands:",
    "-    TYpe a genbank accession name",
    "End of HELP info",
    NULL
  };
  char** m = helpText;
  while (*m != NULL)
    writeStatus(COMMENT,*m++);
}

/*************************************************************************/
void main( int argc, char *argv[] )
{ int level = 0 ;
  char *host = "localhost";
  unsigned long port = 0x20000300 ;
  char *command ;

  /* Read command line parameters */
  if (argc < 2)
    {
      fprintf(stderr,"Usage: gnbkclient [-host host] [-port port_num]\n");
      exit (-1);
    }
 
  while (argc > 1) {
    argv++; argc--;
    if ( (argc > 1) && !strcmp("-host",*argv) ) {
      argv++; argc--;
      host = *argv;
    } 
    else if ( (argc > 1) && !strcmp("-port",*argv) ) {
      argv++; argc--;
      port = atoi(*argv);
    }
    else {
      fprintf(stderr,"Usage: gnbkclient [-host host] [-port port_num]\n");
      exit (-1);
    }
  }
  fprintf(stderr,"command line ok\n") ;
  if (!openServer(host, port))
    {
      writeStatus(COMMERROR,"cannot establish connection");
      exit (-1);
    }
  fprintf(stderr, "server ready\n") ;
  level = freesetfile (stdin, "") ;
  freespecial("\n") ;
  while (freecard(level))
    { if ((command = freeword()))
      { printf("Call server with : %s \n", command) ;
	if (!strcasecmp (command,"HELP"))
	  printHelp();
	else if (!strcasecmp (command,"QUIT"))
	  break ;
	else
	  doQuery(command);
	fflush (stdout) ;
      }
    }
  fprintf(stderr,"A bientot");
  fflush (stdout) ;
}

/*************************************************************************/
/*************************************************************************/
