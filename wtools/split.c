/*  File: split.c
 *  Author: mieg
 *  Copyright (c) J Thierry-Mieg and R Durbin, 1999
 *-------------------------------------------------------------------
 * This file is part of the ACEDB genome database package, written by
 * 	Richard Durbin (Sanger Centre, UK) rd@sanger.ac.uk, and
 *	Jean Thierry-Mieg (CRBM du CNRS, France) mieg@kaa.crbm.cnrs-mop.fr
 *
 * Description: stand alone, will split big ace files in a naive way
 * Exported functions: 
 * HISTORY:
 * Last edited: Aug 26 17:40 1999 (fw)
 * Created: Thu Aug 26 17:40:19 1999 (fw)
 * CVS info:   $Id: split.c,v 1.3 1999-08-26 17:00:50 fw Exp $
 *-------------------------------------------------------------------
 */

#include <stdio.h>

void main (int argc, char **argv)
{ int i = 0, n = 0, nn = 0, nf = 0 ;
  char buf[10000], name[120], *cp ;
  FILE *out = 0 ;

  if (argc < 2)
    { fprintf (stderr, "Usage split prefix, splits stdin into n files of max 10^5 lines called prefix.n \n") ;
      exit(1) ;
    }

  strncpy (name, argv[1], 100) ;
  cp = name + strlen(name) ;
  sprintf(cp, ".%d", ++nf) ;
  out = fopen (name, "w") ;
  if (!out)
    { fprintf (stderr, "cannot ope output file\n") ;
      exit(1) ;
    }

  while (fgets(buf, 9999, stdin))
    { n++ ; nn++ ;
      if ((i = strlen(buf)) > 9990)
	{ fprintf(stderr, "Input too long: %d bytes in line %d\n", i, n) ;
	  exit(1) ;
	}
      
      if (nn > 500000 && i == 1)
	{ fclose (out) ;
	  sprintf(cp, ".%d", ++nf) ;
	  out = fopen (name, "w") ;
	  if (!out)
	    { fprintf (stderr, "cannot ope output file %s\n", name) ;
	      exit (1) ;
	    }
	  nn = 0 ;
	}
      else
	fputs(buf, out) ;
    }
  fprintf (stderr, "done, read %d lines, created %d files upto %s\n", n, nf, name) ;
  exit (0) ;
}
