/*  File: blxmain.c
 *  Author: Erik Sonnhammer
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
 * Description: BLXMAIN - Standalone calling program for blxview.
 * Exported functions: 
 * HISTORY:
 * Last edited: May 26 17:13 2009 (edgrif)
 * * Aug 26 16:57 1999 (fw): added this header
 * Created: Thu Aug 26 16:56:45 1999 (fw)
 * CVS info:   $Id: blxmain.c,v 1.53 2009-05-29 13:21:02 edgrif Exp $
 *-------------------------------------------------------------------
 */

#include <wh/regular.h>
#include <wh/graph.h>
#include <wh/gex.h>
#include <w9/blixem_.h>


/*                                                                           */
/*            blixem copyright string, use "what" on the executable.         */
/*                                                                           */
static const char *ut_copyright_string = UT_COPYRIGHT_STRING(BLIXEM_TITLE, BLIXEM_VERSION, BLIXEM_RELEASE, BLIXEM_UPDATE, BLIXEM_DESC)


/* scrolled message window max messages. */
enum {BLIXEM_MESG_NUM = 50} ;






static void msgPopupsCB(BOOL msg_list) ;






/* Some globals.... */


/* global debug flag for blixem, set TRUE to get debugging output.           */
BOOL blixem_debug_G = FALSE ;


static char *usageText ="\n\
 Blixem - display Blast matches as a multiple alignment.\n\
\n\
 Reference:  Sonnhammer ELL & Durbin R (1994). A workbench for Large Scale\n\
 Sequence Homology Analysis. Comput. Applic. Biosci. 10:301-307.\n\
\n\
  Usage: blixem [options] <sequencefile> <datafile> [X options] \n\
\n\
 Both <sequencefile> and <datafile> can be substituted by \"-\"\n\
 for reading from stdin (pipe).  If <sequencefile> is piped, the first\n\
 line should contain the sequence name and the second the sequence itself.\n\
\n\
 Options:\n\
 -s <mode>  Sorting mode at startup.\n\
               s = by Score\n\
               i = by Identity\n\
               n = by Name\n\
               p = by Position\n\
 -I         Inverted sorting order\n\
 -a         specify a string giving the names of the alignments, e.g. \"EST_mouse EST_human\" etc.\n\
 -b         Don't start with Big Picture.\n\
 -c <file>  Read configuration options from 'file'.\n\
 -S <#>     Start display at position #.\n\
 -F <file>  Read in query sequence and data from <file> (replaces sequencefile).\n\
 -h         Help and more options.\n\
 -o <optstring>\n\
            Blixem options,  e.g. -o \"MBr\" you'll have to read the source code for details.\n\
 -O <#>     sequence offset.\n\
 -P nodeid<:port>\n\
            Causes Blixem to get sequences from a pfetch server at machine nodeid on the given port (default 22100).\n\
 -r         Remove input files after parsing, used by xace when calling blixem as a\n\
            standalone program.\n\
 -x <file>  Read in extra data (to that in datafile) from from <file>, data may be in a\n\
            different format.\n\
\n\
 Some X options:\n\
 -acefont <font> Main font.\n\
 -font    <font> Menu font.\n\
\n\
 To make the datafile from blast output, run MSPcrunch with option -q.\n\n\
\n\
 by Erik.Sonnhammer@cgr.ki.se\n\
 Version" ;

static char *help_string = "\n\
 o To pipe MSPcrunch output directly to Blixem, use \"-\"\n\
   as the second parameter ([datafile]).  Example:\n\
\n\
   MSPcrunch -q <my.blast_output> | blixem <my.seq> -\n\
\n\
\n\
 o The BLAST program (blastp, blastn, blastx, tblastn, tblastx)\n\
   is automatically detected from the Blast output header by MSPcrunch\n\
   and is passed on to Blixem in the seqbl format (-q).  If\n\
   your output lacks the header, set the program with these options:\n\
\n\
   -p    Use blastp output (blastx is default)\n\
   -n    Use blastn output\n\
   -t    Use tblastn output\n\
   -l    Use tblastx output\n\n" ;




/* Entry point for blixem standalone program, you should be aware when
 * altering this that blxview.c is also compiled into acedb and that
 * blxview() is called directly by acedb code.
 */
int main(int argc, char **argv)
{
  FILE        *seqfile, *FSfile;
  char seqfilename[1000] = {'\0'}, FSfilename[1000] = {'\0'} ;
  char  *qseq = NULL, 
    *dummyseq = NULL,					    /* Needed for blxparser to handle both */
							    /* dotter and blixem */
    line[MAXLINE+1],
    ch, *cp, *cq, *c;
  MSP         *MSPlist = NULL ;
  static char opts[32]=" MBr Z   ",	/* 0 L|N|P|T|X
					   1 M
					   2 B|b
					   3 R|r (blxparser.c)
					   4    (available)
					   5 Z (blxparser.c)
					   6 I 
					   7 G (blxparser.c)
					   8    sorting mode
					*/
    qname[FULLNAMESIZE+1],
    dummyseqname[FULLNAMESIZE+1];
  int dispstart = 0, offset = 0, install = 1, seqInSFS = 0 ;
  int          optc;
  extern int   optind;
  extern char *optarg;
  char        *optstring="a:bc:F:hIilno:O:pP:rS:s:tx:";
  char *usage;
  BOOL rm_input_files = FALSE ;
  PfetchParams *pfetch = NULL ;
  BOOL xtra_data = FALSE ;
  FILE *xtra_file = NULL ;
  char xtra_filename[1000] = {'\0'} ;
  char *align_types = NULL ;
  char *config_file = NULL ;
  GError *error = NULL ;
 

  /* Set up the message package to report program/user details in messages.  */
  messSetMsgInfo(argv[0], BLIXEM_TITLE_STRING) ;

  /* Stick version info. into usage string. */
  usage = messalloc(strlen(usageText) + strlen(blixemVersion) + 10) ;
  sprintf(usage, "%s %s\n", usageText, blixemVersion) ;


  while ((optc = getopt(argc, argv, optstring)) != EOF )
    {
      switch (optc)
	{
	case 'a':
	  align_types = hprintf(0, "%s", optarg) ;
	  break;
	case 'b':
	  opts[2] = 'b';
	  break;
	case 'c': 
	  config_file = strnew(optarg, 0) ;
	  break;
	case 'F': 
	  seqInSFS = 1;        
	  strcpy(FSfilename, optarg);
	  break;
	case 'h': 
	  fprintf(stderr, "%s\n%s\n", usage, help_string) ;
	  exit(EXIT_FAILURE) ;
	  break;
	case 'I':
	  opts[6] = 'I';
	  break;
	case 'i':
	  install = 0;
	  break;
	case 'l':
	  opts[0] = 'L';
	  break;
	case 'n':
	  opts[0] = 'N';
	  break;
	case 'o':
	  {
	    int i ;

	    memset(opts, ' ', strlen(opts)) ;

	    for (i = 0 ; i < strlen(optarg) ; i++)
	      {
		opts[i] = optarg[i] ;
	      }
	  }
	  break;
	case 'O':
	  /* Note that I don't do any checking of the value, this is in line */
	  /* with the way blxview() accepts any value of offset, I just check*/
	  /* its actually an integer.                                        */
	  if (!(utStr2Int(optarg, &offset)))
	    messcrash("Bad offset argument: \"-%c %s\"", optc, optarg) ;
	  break;
	case 'p':
	  opts[0] = 'P';
	  break;
	case 'P':
	  pfetch = messalloc(sizeof(PfetchParams)) ;
	  pfetch->net_id = strtok(optarg, ":") ;
	  pfetch->port = atoi(strtok(NULL, ":")) ;
	  break;
	case 'r':
	  rm_input_files = TRUE ;
	  break ;
	case 'S': 
	  if (!(dispstart = atoi(optarg)))
	    messcrash("Bad diplay start position: %s", optarg); 
	  opts[1] = ' ';
	  break;
	case 's': 
	  if (*optarg != 's' && *optarg != 'i' && *optarg != 'n' && *optarg != 'p')
	    {
	      fprintf(stderr,"Bad sorting mode: %s\n", optarg); 
	      exit(EXIT_FAILURE) ;
	    }
	  opts[8] = *optarg;
	  break;
	case 't':
	  opts[0] = 'T';
	  break;
	case 'x': 
	  xtra_data = TRUE ;
	  strcpy(xtra_filename, optarg);
	  break;

	default : messcrash("Illegal option");
	}
    }

  if (argc-optind < 2 && !seqInSFS)
    {
      fprintf(stderr, "%s\n", usage); 

      exit(EXIT_FAILURE);
    }


  /* Add -install for private colormaps */
  if (install)
    argvAdd(&argc, &argv, "-install");
  graphInit(&argc, argv) ;
  gexInit(&argc, argv);
  gexSetMessPrefs(FALSE, BLIXEM_MESG_NUM, msgPopupsCB) ;  /* for control of popup or mesglist */


  /* Set up program configuration. */
  if (!blxInitConfig(config_file, &error))
    {
      messcrash("Config File Error: %s", error->message) ;
    }


  if (!seqInSFS)
    {
      strcpy(seqfilename, argv[optind]);
      strcpy(FSfilename, argv[optind+1]);

      if (!strcmp(seqfilename, "-"))
	{
	  seqfile = stdin;
	}
      else if(!(seqfile = fopen(seqfilename, "r")))
	{
	  messcrash("Cannot open %s\n", seqfilename);
	}
	
      /* Read in query sequence */
      if (seqfile == stdin)
	{
	  /* Same file for query and FS data - sequence on first two lines */
	  Array arr = arrayCreate(5000, char);
	  int i=0;
	    
	  if (!fgets(line, MAXLINE, seqfile))
	    messcrash("Error reading seqFile");;

	  sscanf(line, "%s", qname);
	    
	  while ((ch = fgetc(seqfile)) != '\n') {
	    /*if (isAlnRes(ch) || isAlnNonres(ch))*/
	    array(arr, i++, char) = ch;
	  }
	  qseq = messalloc(arrayMax(arr)+1);
	  cq = qseq;
	  for (i = 0; i < arrayMax(arr);) *cq++ = arr(arr, i++, char);
	  arrayDestroy(arr);
	}
      else
	{
	  fseek(seqfile, 0, SEEK_END);
	  qseq = messalloc(ftell(seqfile)+1);
	  cq = qseq;
	  fseek(seqfile, 0, SEEK_SET);
	  while (!feof(seqfile))
	    {
	      if (!fgets(line, MAXLINE, seqfile))
		break;
	      if ((c = (char *)strchr(line, '\n')))
		*c = 0;
		    
	      if (*line == '>')
		{
		  strncpy(qname, line+1, 255);
		  qname[255]=0;
		  if ((c = (char *)strchr(qname, ' ')))
		    *c = 0;
		  continue;
		}
		    
	      for (cp = line; *cp; cp++) 
		/*if (isAlnRes(*cp) || isAlnNonres(*cp))*/ *cq++ = *cp;
	      *cq = 0;
	    }
	  fclose(seqfile);
	}
    }

  /* Parse the data file containing the homol descriptions.                */
  if (!strcmp(FSfilename, "-"))
    {
      FSfile = stdin;
    }
  else if(!(FSfile = fopen(FSfilename, "r")))
    {
      messcrash("Cannot open %s\n", FSfilename);
    }
  parseFS(&MSPlist, FSfile, opts, &qseq, qname, &dummyseq, dummyseqname) ;
  if (FSfile != stdin)
    fclose(FSfile) ;

  /* There may an additional file containing homol data in an alternative format. */
  if (xtra_data)
    {
      if(!(xtra_file = fopen(xtra_filename, "r")))
	{
	  messcrash("Cannot open %s\n", xtra_filename) ;
	}
      parseFS(&MSPlist, xtra_file, opts, &qseq, qname, &dummyseq, dummyseqname) ;
      fclose(xtra_file) ;
    }


  /* Remove the input files if requested.                                  */
  if (rm_input_files)
    {
      if(seqfilename[0] != '\0' && unlink(seqfilename) != 0)
	messerror("Unlink of sequence input file \"%s\" failed: %s\n",
		  seqfilename, messSysErrorText()) ;
      if(FSfilename[0] != '\0' && unlink(FSfilename) != 0)
	messerror("Unlink of MSP input file \"%s\" failed: %s\n",
		  FSfilename, messSysErrorText()) ;
      if (xtra_filename[0] != '\0' && unlink(xtra_filename) != 0)
	messerror("Unlink of extra MSP sequence input file \"%s\" failed: %s\n",
		  xtra_filename, messSysErrorText()) ;
    }


  /* Now display the alignments, this call does not return. (Note that
   * TRUE signals blxview() that it is being called from this standalone
   * blixem program instead of as part of acedb. */
  if (GRAPH_NULL != blxview(qseq, qname, dispstart, offset, MSPlist, opts, pfetch, align_types, TRUE))
    {
      graphLoop(TRUE) ;
    }
    
    
  /* We should not get here.... */
  return (EXIT_FAILURE) ;
}



char *blxVersion(void)
{
  return (char *)ut_copyright_string ;
}





/* 
 *                    Internal functions.
 */




/* Enables user to switch between seeing informational messages in popups
 * or in a scrolled window. */
static void msgPopupsCB(BOOL msg_list)
{
  int list_length ;

  if (msg_list)
    list_length = BLIXEM_MESG_NUM ;
  else
    list_length = 0 ;

  gexSetMessPopUps(msg_list, list_length) ;

  return ;
}





