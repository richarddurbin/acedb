/*  File: alignmentdisp.c
 *  Author: Fred Wobus (fw@sanger.ac.uk)
 *  Copyright (C) J Thierry-Mieg and R Durbin, 1998
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
 * Description: graphical alignment display functions
 * Exported functions:
 *              alignDisplay
 * HISTORY:
 * Last edited: Apr 12 14:24 2002 (edgrif)
 * Created: Thu Nov 19 11:41:03 1998 (fw)
 * CVS info:   $Id: alignmentdisp.c,v 1.11 2002-04-16 13:03:54 edgrif Exp $
 *-------------------------------------------------------------------
 */

#include <wh/acedb.h>
#include <wh/display.h>
#include <wh/keysetdisp.h>
#include <wh/alignment_.h>
#include <wh/dbpath.h>


/************************************************************/

#ifdef JUNK

static int ALIGN_MAGIC ;

static ALIGNMENT *alignFromGraph (void)
{ 
  ALIGNMENT *al ;
  if (!(graphAssFind (&ALIGN_MAGIC, &al)))
    messcrash ("Failed to find alignment from graph") ;
  return al ;
}



static void alignTreeShow (void)
{ 
  ALIGNMENT *al = alignFromGraph () ;

  display (al->key, 0, TREE) ;
}

static void alignDisplayDump (void)
{ 
  ALIGNMENT *al = alignFromGraph () ;
  FILE *fil = filqueryopen (0, 0, "aln", "w", "Alignment file name") ;

  if (!fil)
    return ;
  alignDumpKey (al->key, fil) ;
  filclose (fil) ;
}

static void alignDisplayKeySetDump (void)
{ 
  static char directory[DIR_BUFFER_SIZE] = "";
  static char filename[FIL_BUFFER_SIZE] = "";
  KEYSET ks ;
  ACEOUT dump_out;

  if (!keySetActive (&ks, 0))
    { messout ("Can't find active keyset") ;
      return ;
    }

  dump_out = aceOutCreateToChooser ("Alignment file to write to ?",
				    directory, filename, "aln", "w", 0);
  if (dump_out)
    {
      alignDumpKeySet (dump_out, ks) ;
      aceOutDestroy (dump_out);
    }

  return;
} /* alignDisplayKeySetDump */
 


static MENUOPT menu[] = {
  {graphDestroy, "Quit"},
  {graphPrint, "Print"},
  {alignTreeShow, "Show as tree"},
  {alignDisplayDump, "Dump"},
  {alignDisplayKeySetDump, "Dump KeySet"},
   {0, 0}
} ;


BOOL alignDisplay_old (KEY key, KEY from, BOOL isOldGraph)
{ 
  ALIGNMENT *al ;
  ALIGN_COMP *c ;
  int i, xmax = 0, ymax = 0, x, y, maxlen = 0 ;
  int line = 0 ;

  if (!(al = alignGet (key)))
    { display (key, key, TREE) ;
      return FALSE ;
    }

  if (isOldGraph)
    graphClear () ;
  else
    { 
      displayCreate ("DtAlignment") ;
      graphMenu (menu) ;
    }

  graphAssociate (&ALIGN_MAGIC, al) ;

				/* now draw things */

  graphText (name(key), 0, line) ;
  line += 2 ;

  c = arrp(al->comp, 0, ALIGN_COMP) ;
  for (i = arrayMax(al->comp) ; i-- ; ++c)
    { x = strlen (messprintf ("%d - %d", c->start, c->end)) ;
      if (x > xmax)
	xmax = x ;
      y = strlen (name (c->key)) ;
      if (y > ymax)
	ymax = y ;
    }
  ymax += 2 ;
  xmax += ymax + 2 ;

  c = arrp(al->comp, 0, ALIGN_COMP) ;
  for (i = arrayMax(al->comp) ; i-- ; ++c)
    { graphText (name(c->key), 0, line) ;
      graphText (messprintf ("%d - %d", c->start, c->end), ymax, line) ;
      if (c->seq)
	{ graphText (c->seq, xmax, line) ;
	  if (strlen (c->seq) > maxlen)
	    maxlen = strlen (c->seq) ;
	}
      ++line ;
    }
  
  graphTextBounds (xmax+maxlen, line) ;
  graphRedraw () ;
  return TRUE ;
}
#endif /* JUNK */

BOOL alignDisplay (KEY key, KEY from, BOOL isOldGraph, void *unused)
{ 
#if !defined(NO_POPEN)

  ALIGNMENT *al ;
  ALIGN_COMP *c ;
  int i, x, xmax = 0 ;
  FILE *pipe ;
  char *domain, *script ;
  
  extern char *findCommand (char *command, char **retp);
  extern Stack BelvuMatchStack;

  if (!(al = alignGet (key)))
    {
      return FALSE ;
    }

  /* Open pipe to belvu */
  /* Can't call callScriptPipe since we want write access and buildCommand 
     is not an exported function in the graph library */
  script = dbPathFilName(WSCRIPTS, "belvu_script", "", "x", 0) ;
  printf("Calling \"%s %s\"", script, name(key));
  fflush(stdout);
  pipe = (FILE *)popen(messprintf("%s %s", script, name(key)), "w");
  messfree(script);

  /* Find longest name string */
  c = arrp(al->comp, 0, ALIGN_COMP) ;
  for (i = arrayMax(al->comp) ; i-- ; ++c)
    { x = strlen (messprintf ("%s/%d-%d", name(c->key), c->start, c->end)) ;
      if (x > xmax)
	xmax = x ;
    }

  domain = messalloc(xmax+1);

  c = arrp(al->comp, 0, ALIGN_COMP) ;
  for (i = arrayMax(al->comp) ; i-- ; ++c)
    { sprintf (domain, "%s/%d-%d", name(c->key), c->start, c->end) ;
      fprintf (pipe, "%-*s ", xmax, domain) ;
      if (c->seq)
	fprintf (pipe, "%s", c->seq) ;
      fprintf (pipe, "\n") ;
    }
  
  messfree(domain);

  /* MatchFooter */
  if (stackExists(BelvuMatchStack)) {
    char *cp;

    fprintf(pipe, "# matchFooter\n");

    stackCursor(BelvuMatchStack, 0) ;
    cp = stackNextText(BelvuMatchStack);
    printf(" with match %s\n", cp);

    fprintf(pipe, "%s\n", cp);
    fprintf(pipe, "%s\n", stackNextText(BelvuMatchStack));
    while ((cp = stackNextText (BelvuMatchStack)))
      fprintf(pipe, "%s ", cp);
    stackDestroy(BelvuMatchStack);
  }
  else printf("\n");
  fflush(stdout);

  fprintf (pipe, "%c\n", EOF) ; /* To close the pipe, sigh */
  fflush(pipe);
  /* pclose is no good - waits till belvu is finished. */

  return TRUE ;
#else
  return FALSE ;
#endif  /* not NO_POPEN */
} /* alignDisplay */

/*************************** eof ****************************/
