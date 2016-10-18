/*  File: gifcommand.c
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
 * Description:
 * Exported functions:
 *              gifControl - function that deals with giface-commands
 *			     gets called from command.c, if the kernel
 *			     command parser can't deal with a command
 *			     because it interact with graphical modules.
 * HISTORY:
 * Last edited: May 28 12:05 2010 (edgrif)
 * * Jan 27 16:05 1999 (fw): created this module to accomodate
 *              the function gifControl() from gifacemain.c
 * Created: Wed Jan 27 16:04:40 1999 (fw)
 * CVS info:   $Id: gifcommand.c,v 1.47 2010-06-04 15:57:24 edgrif Exp $
 *-------------------------------------------------------------------
 */

#include <wh/acedb.h>
#include <wh/aceio.h>
#include <wh/command.h>
#include <wh/session.h>
#include <wh/lex.h>
#include <whooks/sysclass.h>
#include <wh/query.h>
#include <wh/display.h>
#include <wh/update.h>
#include <wh/fmap.h>					    /* for fMapGifXxx. */
#include <wh/pepgifcommand.h>				    /* for pepGifCommandXxxx */

#include <wh/smapconvert.h>

/*  #include <wh/gmap.h>  cannot include because of clashes with MAP2GRAPH macros */
extern void gMapDrawGIF (ACEIN command_in, ACEOUT result_out);
extern void pMapDrawGIF (ACEIN command_in, ACEOUT result_out);


/* Holds state for the current gif command execution. */
typedef struct _GIFCommandStruct
{
  BOOL quiet ;
} GIFCommandStruct, *GIFCommand ;




/*********************************/
/******** giface submenu  ********/

/* General GIF commands. */
enum {GIF_QUIET = 300} ;

enum {GIF_FMAP_RECALC = 400, GIF_FMAP_ACTIVEZONE, GIF_DO_SMAP} ;

enum {GIF_SMAP_SEQGET = 700, GIF_SMAP_SEQFEATURES} ;

enum {GIF_DASDNA = 800, GIF_DASTYPES, GIF_DASGFF} ;


/* NOTE: If you change the functionality of these options
 * you have to update the documentation file whelp/Giface.html
 * to reflect your changes.
 * ALSO NOTE: If you add any new commands you must increment the number in the
 * first member of the options[] array to be the total number of commands.
 */
static FREEOPT options[] =
{
  /* First element _MUST_ be {(array elements - 1), "cmdline prompt"}, we fill in the number
   * automatically. */
  { 0,  "acedb-gif" },

  /* General functions. */
  { -1,  "Quit" },					    /* quit command simulates EOF */
  { '?', "? : this list of commands.  Multiple commands ';' separated accepted" },
  { GIF_QUIET,
   "quiet -on | -off :\n"
   "\tTurn quiet mode on or off, when \"on\" commands produce only their output\n"
   "\twithout any extra comments (e.g. \"NNN Active Objects\" etc.).\n"
   "\tQuiet mode makes parsing command output easier because either the first line\n"
   "\tis an acedb comment because the command failed or there are no acedb comments in the output.\n"
   "\t-off    (default) verbose output\n"
   "\t-on     no comments unless there is an error.\n"},
  { 'd', "dimensions [-D displaytype] x y : image size in pixel for all or the specified displaytype" }, 
  { 'D', "display [-D displaytype] [-view view] [class key] : graphic display [for the current key]" },
  { 'B', "psdump [-e] [-c]  filename : save active graph as postscript" },
  { 'A', "gifdump filename : save active graph as a gif" },
  { 'M', "mouseclick x y: simulate a left button click on the active graph" },

  { 'm', "map map [-view view] [-coords x1 x2] [-hideheader] [-whole]: does not write - use gif after" },

  { 'P', "pmap [-clone <name>] | [-contig <name> <coord>] : physical map" }, 

  /* fmap/sequence operations. */
  { 'S', "seqget [[class:]object [-coords x1 x2]] [-noclip] [-source | -method]: sets current sequence for future ops on same line - if no name then get from active graph\n"
    "\t-coords  specify start/end of section of sequence to get\n"
    "\t-noclip  specify that features should not be clipped to x1, x2 but have their full extent mapped."
    "\t[-source source(s)]  specify a set of sources from which objects should be included/excluded in the virtual sequence."
    "\t[-method methods(s)] specify a set of methods from which objects should be included/excluded in the virtual sequence."
    "\tN.B. source and method must be the last option."
  },
  { 'X', "seqdisplay [-visible_coords v1 v2] [-new] [-title title] : works on current sequence, use gif or ps to actually dump the display\n"
    "\t-new                 make a new sequence, current one is preserved (for use only with xremote currently)\n"
    "\t-title               give your own title to the sequence"
  },
  { 'n', "seqdna [-file fname] <-coords x1 x2> : works on current sequence <coords deprecated - use seqget>" },
  { 'f', "seqfeatures [-quiet] [-file fname] [-version 1|2] [-list [features | keys]] [-source source(s)] [-feature feature(s)] <-coords x1 x2> : works on current sequence, quiet means only GFF in output, source(s)/feature(s) are '|' separated lists, <coords deprecated - use seqget>" },
  { 'g', "seqactions  [-dna] [-gf_features] [-hide_header] [-rev_comp] : works on current sequence" },
  { 'c', "seqcolumns {-on name} {-off name} {-list}: works on active sequence" },
							    /* NB seqcolumns will work BEFORE seqdisplay() now */
  { 'a', "seqalign  [-file fname] [-coords x1 x2] : must follow a seqdisplay <or sequence, which is deprecated>" },
  { 's', "sequence sequence [-coords x1 x2] : <deprecated - use seqget ; seqdisplay>" },
  { GIF_DO_SMAP, "smap -coords x1 x2 -from class:object -to class:object : maps cordinates between related sequences." },
  { GIF_FMAP_RECALC, "seqrecalc causes the sequence to be recalculated for the active graph." },
  { GIF_FMAP_ACTIVEZONE, "seqgetactivezone retrieves the current fmap graphs active zone coords,\n"
    "\tcoords are in the reference frame of the root of the sequence object displayed." },

  
  { 'e', "EMBL filename : embl dump keyset to filename.embl" },

  {500, "pepget <protein | peptide> [-coords x1 x2] : sets current protein for future ops on same line" },  
  {501, "pepseq [-file fname] <-coords x1 x2> : exports current petide sequence" },
  {502, "pepalign [-file fname] <-coords x1 x2> : exports homols of current peptide" },

  { 'K', "makemaps -gmap | -pmap | -cmap | -alpha | -all | -seqmap file | -seqclonemap file : makes cached maps and sorted class keysets" },
  { 'U', "update [-all] : add one official update, or all" },



#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
  /* Trial code using smap but NO fmap to get a sequence....                 */
  { GIF_SMAP_SEQGET, "smapseqget sequence [-coords x1 x2] : sets current sequence for future." },
  { GIF_SMAP_SEQFEATURES, "smapseqfeatures [-file fname] [-version 1|2] [-list] [-source source(s)] [-feature feature(s)] <-coords x1 x2> : works on current sequence, source(s)/feature(s) are '|' separated lists, <coords deprecated - use seqget>" },
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */


  /* To support DAS format output. */
  {GIF_DASTYPES, "dastypes : gets types for current sequence.." },
  {GIF_DASDNA, "dasdna : gets dna for current sequence.." },
  {GIF_DASGFF, "dasgff : gets features for current sequence.." }

} ;


enum {OPTIONS_SIZE = (UtArraySize(options) - 1)} ;


/*
 * Implements the gif menu/functions, e.g. seqget etc. Is called by a number of
 * components, e.g. tace, xace (fmap), the gif version of the server.
 * 
 */
void gifControl(KEYSET ks, ACEIN fi, ACEOUT fo, BOOL quiet)
{ 
  GIFCommandStruct gif_cmd ;
  int option ;
  char *word = NULL ;
  FeatureMap seqLook = NULL ;
  PepGifCommand pepLook = NULL ;
  BOOL oldisGifDisplay = isGifDisplay ;
#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
  GTimer* timer ;

  timer = g_timer_new() ;
  g_timer_start(timer) ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

  options[0].key = OPTIONS_SIZE ;			    /* Simpler to do this every time. */


  gif_cmd.quiet = quiet ;

  while (TRUE)
    {
      if (aceInOptPrompt (fi,  &option, options))
	switch (option)
	  {
	    /**** end of input / quit command ****/
	  case -1:
	    goto finish ;	/* only way to break out of while-loop */

	  case '?':		/* help */
	    { 
	      int i ;
	      for (i = 1 ; i <= options[0].key ; i++)
		aceOutPrint (fo, "// %s\n", options[i].text) ;
	    }
	    break ;

	  case GIF_QUIET:				    /* Set quiet mode for commands a la Unix. */
	    {
	      char *cp ;
	      while ((cp = aceInWord(fi)))
		{
		  if (strcmp (cp, "-on") == 0)
		    gif_cmd.quiet = TRUE ;
		  else if (strcmp(cp, "-off") == 0)
		    gif_cmd.quiet = FALSE ;
		}
	      break ;
	    }
	  case 'd':		/* dimensions in pixels */
	    {
	      KEY displayKey = 0;
	      char *cp;
	      int xpixels, ypixels;
	      
	      if ((cp = aceInWord(fi)))
		{
		  if (strcmp(cp, "-D") == 0)
		    {
		      if ((cp = aceInWord (fi)))
			{
			  if (displayIsObjectGeneric(cp, &displayKey))
			    {
			      if (aceInInt (fi,&xpixels) && 
				  aceInInt (fi,&ypixels) &&
				  xpixels > 0 && ypixels > 0)
				{
				  displaySetSize(name(displayKey),
						 xpixels, ypixels);
				  break;
				}
			    }
			  else
			    {
			      aceOutPrint (fo, "// Error : unsuitable display type %s\n", cp);
			      break;
			    }
			}
		      else
			{
			  aceOutPrint (fo, "// Error: -D must be followed by a display type\n") ;
			  break ;
			}
		    }
		  else
		    {
		      aceInBack(fi);
		      
		      if (aceInInt (fi,&xpixels) &&
			  aceInInt (fi,&ypixels) &&
			  xpixels > 0 && ypixels > 0)
			{
			  displaySetSizes(xpixels, ypixels);
			  break;
			}
		    }
		}
	      
	      aceOutPrint (fo, "// Usage: dimensions [-D displaytype] x y : "
			   "image size in pixel for all or the specified displaytype\n") ;
	    }
	    break ;

	  case 'D':   /* display an object */
	    /* only displays single objects, multiple objects is an error. */
	    { KEYSET kA ;
	      KEY classKey = 0, displayKey = 0, view = 0 ;
	      char *cp ;
	      KEY _VView, mm;

	      lexaddkey("View", &mm, _VMainClasses);
	      _VView = KEYKEY(mm);
	      
	      while ((cp = aceInWord(fi)))
		{
		  if (*cp != '-')
		    { aceInBack(fi) ; break ; }
		  if (!strcmp(cp, "-D")) /* -D displaytype */
		    {
		      if (!(cp = aceInWord(fi)))
			{ aceOutPrint (fo, "// Error: -D must be followed by a display type\n") ;
			  break ;
			}
		    
		      if (!displayIsObjectGeneric(cp, &displayKey))
			{ aceOutPrint (fo, "// Error: unsuitable display type %s\n", cp) ;
			  break ;
			}
		    }
                  else if (!strcmp(cp, "-view")) /* -view view */
                    { if (!(cp = aceInWord(fi)))
                        { aceOutPrint (fo, "// Error: -view must be followed by a view name\n") ;
                          break ;
                        }
                      if (!lexword2key (cp, &view, _VView))
                        { aceOutPrint (fo, "// Error: unknown view %s\n", cp) ;
                          break ;
                        }
                    }
		}
	      
	      if (aceInCheck (fi,"ww")) /* [class name] */
		{ cp = aceInWord(fi) ;
		  if (!lexword2key (cp, &classKey, _VClass))
		    { aceOutPrint (fo, "// Error: bad class name %s\n", cp) ;
		      break ;
		    }
		  kA = query(0, messprintf("Find %s \"%s\"", 
					   name(classKey), aceInWord(fi))) ;
		}
	      else if (!ks)
		{ aceOutPrint (fo, "// Error: no active list\n") ;
		  break ;
		}
	      else
		kA = ks ;
		  
	      if (!keySetMax (kA))
		{ aceOutPrint (fo, "// Error: active list is empty\n") ;
		  break ;
		}
	      if (keySetMax(kA) == 1) 
		display (keySet(kA,0), view, displayKey ? name(displayKey) : 0) ;
	      else
		aceOutPrint (fo, "// Error: can not display multiple objects\n") ;

	      /* Only destroy the keySet if we created a new one, DON'T destroy */
	      /* the keyset we were passed in !!                             */
	      if (kA != ks) keySetDestroy (kA) ;
	    }
	    break;

	  case 'B':   /* dump the active graph as PostScript */
	    {
	      BOOL colour = FALSE, EPSF = FALSE;
	      word = aceInWord(fi) ;
	      
	      while ((strcmp(word, "-e") == 0) || (strcmp(word, "-c") == 0))
		{ 
		  if (strcmp(word, "-e") == 0)
		    EPSF = TRUE;
		  if (strcmp(word, "-c") == 0)
		    colour = TRUE;
		  word = aceInWord(fi);
		}
	      
	      if (!graphActive())
		aceOutPrint (fo, "// psdump error : no active graph\n") ;
	      if (!word || !*word)
		aceOutPrint (fo, "// Usage: psdump -e -c filename: dump the active graph in file as PostScript\n") ;
	      else
		{
		  char fileName[MAXPATHLEN] ;
		  ACEOUT ps_out;
		  BOOL rotation ;
		  float scale ;
		  int pages ;
		  int err;
		  char *fp = filGetFullPath(word, 0);
		  
		  strcpy (fileName, fp);
		  messfree(fp);
		  
		  if (strcmp(filGetExtension(word), "ps") != 0)
		    strcat (fileName, ".ps");
		  
		  ps_out = aceOutCreateToFile(fileName, "w", 0);
		  if (ps_out)
		    {
		      graphPSdefaults(graphActive(), PAPERTYPE_A4,
				      &rotation, &scale, &pages) ;
		  
		      err = graphPS(graphActive(), ps_out,
				    NULL, 
				    colour, EPSF, rotation, 
				    PAPERTYPE_A4, scale, pages) ;
		      if (err == ESUCCESS)
			aceOutPrint (fo, 
				     "// I wrote the active graph "
				     "to file %s\n", fileName) ;
		      else
			aceOutPrint (fo, 
				     "// ERROR: Failed to write to "
				     "file %s (%s)\n",
				     fileName, messErrnoText(err)) ;
		      aceOutDestroy(ps_out);
		    }
		}
	    }
	    break;
	    
	  case 'A':   /* dump a GIF of the active graph */
	    {
	      word = aceInWord(fi) ;
	      
	      if (!graphActive())
		aceOutPrint (fo, "// gifdump error : no active graph\n") ;
	      else if (!word || !*word)
		aceOutPrint (fo, "// Usage gifdump filename: dump the active graph in file\n") ;
	      else if (strcmp(word,"-") == 0)
		{ /* LS 2 Mar 98 dump GIF to standard output */
		  
		  if (graphGIF(graphActive(), fo, TRUE) == ESUCCESS)	
		    graphBoxInfoFile (fo);
		  else
		    aceOutPrint (fo, "// ERROR: Failed to write GIF data\n");
		}
	      else
		{
		  char fileName[MAXPATHLEN];
		  ACEOUT gif_out, boxes_out;
		  char *fp = filGetFullPath(word, 0);

		  strcpy (fileName, fp);
		  messfree(fp);
		  
		  if (strcmp(filGetExtension(fileName), "gif") == 0)
		    fileName[strlen(fileName)-4] = '\0';
		  
		  gif_out = aceOutCreateToFile(messprintf("%s.gif", fileName),
					       "wb", 0);
		  
		  if (gif_out)
		    {
		      int err = graphGIF(graphActive(), gif_out, FALSE);
		      if (err == ESUCCESS)
			aceOutPrint (fo,
				     "// I wrote the active graph to file %s\n",
				     fileName) ;
		      else
			aceOutPrint (fo, 
				     "// ERROR: Failed to write to "
				     "file %s (%s)\n",
				     fileName, messErrnoText(err)) ;
		 
		      aceOutDestroy(gif_out);
		    }
		  boxes_out = aceOutCreateToFile(messprintf("%s.boxes", 
							    fileName),
						 "w", 0);
		  if (boxes_out)
		    { 
		      graphBoxInfoFile (boxes_out) ;
		      aceOutDestroy (boxes_out);
		    }
		}
	    }
	  break;
	  
	  case 'M' :  /* mouseclick x y */
	    {
	      int xClick, yClick;

	      if (!graphActive())
		aceOutPrint (fo, "// mouseclick error : no active graph\n");
	      else if (!aceInInt (fi,&xClick) || !aceInInt (fi,&yClick))
		aceOutPrint (fo, "// Usage:  mouseclick x y - simulates a left button click in the active graph\n") ;
	      else
		graphGIFLeftDown (xClick, yClick) ;
	    }
	    break;

	  case 'm':		/* map/view draw */
	    { 
	      gMapDrawGIF (fi, fo);
	    }
	    break ;

	  case 'P':		/* pmap draw */
	    {
	      pMapDrawGIF (fi, fo);
	    }
	    break ;


	    /* Sequence/fmap based operations. */

	  case 'S':		/* get sequence, init fMap package */
	    seqLook = fMapGifGet (fi, fo, seqLook) ; /* destroys existing seqLook */
	    break ;

	  case 'X':		/* sequence display */
	    if (seqLook) fMapGifDisplay (fi, fo, seqLook) ;
	    else aceOutPrint (fo, "// gif seqdisplay without active sequence\n") ;
	    break ;

	  case 'n':		/* dna dump */
	    if (seqLook)
	      fMapGifDNA (fi, fo, seqLook, gif_cmd.quiet) ;
	    else
	      aceOutPrint (fo, "// gif seqdna without active sequence\n") ;
	    break ;

	  case 'f':		/* feature dump */
	    if (seqLook)
	      fMapGifFeatures(fi, fo, seqLook, gif_cmd.quiet) ;
	    else
	      aceOutPrint (fo, "// gif seqfeatures without active sequence\n") ;
	    break ;

	  case 'g':		/* actions */
	    if (seqLook) fMapGifActions (fi, fo, seqLook) ;
	    else aceOutPrint (fo, "// gif seqactions without active sequence\n") ;
	    break ;

	  case 'c':		/* column control */
	    if (seqLook) fMapGifColumns (fi, fo, seqLook) ;
	    else aceOutPrint (fo, "// gif seqcolumns without active sequence\n") ;
	    break ;

	  case 'a':		/* alignment */
	    if (seqLook) fMapGifAlign (fi, fo, seqLook) ;
	    else aceOutPrint (fo, "// gif seqalign without displayed active sequence\n") ;
	    break ;

	  case 's':		/* sequence draw - deprecated */
	    seqLook = fMapGifGet (fi, fo, seqLook) ;
	    if (seqLook) fMapGifDisplay (fi, fo, seqLook) ;
	    else aceOutPrint (fo, "// gif seqdisplay without active sequence\n") ;
	    break ;

	  case GIF_FMAP_RECALC:				    /* do recalculate of sequence. */
	    fMapGifRecalculate(fi, fo, seqLook) ;
	    break ;

	  case GIF_FMAP_ACTIVEZONE:			    /* Get the active zone. */
	    fMapGifActiveZone(fi, fo, seqLook) ;
	    break ;

	  case GIF_DO_SMAP:
	    commandDoSMap(fi, fo) ;
	    break;

	  case 500:		/* get protein */
	    pepLook = pepGifCommandGet (fi, fo, pepLook) ;
	    break ;

	  case 501:		/* dna dump */
	    if (pepLook) pepGifCommandSeq (fi, fo, pepLook) ;
	    else aceOutPrint (fo, "// gif pepseq without active protein\n") ;
	    break ;

	  case 502:		/* feature dump */
	    if (pepLook) pepGifCommandAlign (fi, fo, pepLook) ;
	    else aceOutPrint (fo, "// gif pepalign without active protein\n") ;
	    break ;

/*------------------------------------------------------------*/

	  case 'e':		/* EMBL dump */
	    { extern void emblDumpKeySetFile (KEYSET kset, char *fname) ;
	      if ((word = aceInWord(fi)))
		emblDumpKeySetFile (ks, word) ;
	      else
		aceOutPrint (fo, "// Usage: EMBL filename\n") ;
	    }
	    break ;

/*------------------------------------------------------------*/

	  case 'K':		/* make cached maps */
	    { extern void gMapMakeAll(void) ;
	      extern void pMapMakeAll(void) ;
	      extern void cMapMakeAll(void) ;
	      extern void sMapMake (FILE *fil, BOOL isSeq) ;
	      char *filname ;
	      FILE *fil ;

	/* must save rest of free card, else sessionWriteAccess() destroys */
	      /*  useless with aceIn   cardKeep = strnew (freepos(), 0) ; */
	      if (!sessionGainWriteAccess())
		{ aceOutPrint (fo, "// Error: can't get write access\n") ;
		  break ;
		}
	      /* freeforcecard (cardKeep) ; messfree (cardKeep) ; */

	      while (aceInStep (fi, '-') && (word = aceInWord(fi)))
		{ 
		  /* cardKeep = strnew (freepos(), 0)  again useless now  save because freesubs used in some *MakeAll */

		  if (!strcmp (word, "gmap"))
		    gMapMakeAll () ;
		  else if (!strcmp (word, "pmap"))
		    pMapMakeAll () ;
		  else if (!strcmp (word, "cmap"))
		    cMapMakeAll () ;
		  else if (!strcmp (word, "alpha"))
		    lexAlphaMakeAll () ;
		  else if (!strcmp (word, "all"))
		    { pMapMakeAll () ;
		      gMapMakeAll () ;
		      cMapMakeAll () ;
		      lexAlphaMakeAll () ;
		    }
		  else if (!strcmp (word, "seqmap"))
		    {
		      if (!(word = aceInWord(fi)))
			aceOutPrint (fo, "// Error: makemaps -seqmap "
				 "requires a file name\n") ;
		      else if (!(fil = fopen (word, "w")))
			aceOutPrint (fo, "// Error: makemaps -seqmap "
				  "failed to open file %s\n", word) ;
		      else
			{ 
			  filname = strnew (word, 0) ;
			  sMapMake (fil, TRUE) ;
			  aceOutPrint (fo, "// Sequence-* map file written to %s\n", 
				    filname) ;
			  messfree (filname) ;
			  fclose (fil) ;
			}
		    }
		  else if (!strcmp (word, "seqclonemap"))
		    {
		      if (!(word = aceInWord(fi)))
			aceOutPrint (fo, "// Error: makemaps -seqclonemap "
				 "requires a file name\n") ;
		      else if (!(fil = fopen (word, "w")))
			aceOutPrint (fo, "// Error: makemaps -seqclonemap "
				  "failed to open file %s\n", word) ;
		      else
			{ 
			  filname = strnew (word, 0) ;
			  sMapMake (fil, FALSE) ;
			  aceOutPrint (fo, "// Sequence-* Clone map file written to %s\n", 
				    filname) ;
			  messfree (filname) ;
			  fclose (fil) ;
			}
		    }
		  else
		    aceOutPrint (fo, "// Usage: makemaps -gmap | -pmap | -cmap | -alpha | -all | -seqmap file | -seqclonemap file\n") ;

		  /* freeforcecard (cardKeep) ; messfree (cardKeep) ; useless with aceIn */
		}
	      
	      if (aceInWord(fi))	/* i.e. something not starting with '-' */
		{ 
		  aceOutPrint (fo, "// unrecognized option %s\n", word) ;
		  aceOutPrint (fo, "// Usage: makemaps -gmap | -pmap | -cmap | -alpha | -all | -seqmap file | -seqclonemap file\n") ;
		}
	    }
	    break ;

	  case 'U':		/* add updates */
	    if ((word = aceInWord(fi)))
	      {
		if (!strcmp (word, "-all"))
		  updateDoAction (TRUE) ;
		else
		  aceOutPrint (fo, "// Usage: update [-all]\n") ;
	      }
	    else
	      updateDoAction (FALSE) ;
	    break ;

	    /* DAS functions. */
	  case GIF_DASTYPES:
	    aceOutPrint (fo, "// DAS types command.\n") ;
	    break ;

	  case GIF_DASDNA:
	    if (seqLook)
	      dasDNA (fi, fo, seqLook) ;
	    else
	      aceOutPrint (fo, "// gif dasdna without active sequence\n") ;
	    break ;

	  case GIF_DASGFF:
	    if (seqLook)
	      dasGFF(fi, fo, seqLook) ;
	    else
	      aceOutPrint (fo, "// gif dasgff without active sequence\n") ;
	    break ;

	  } /* end-switch (key) */

    } /* end-while(TRUE) */



#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
  printf("Elapsed time: %g\n", g_timer_elapsed(timer, NULL)) ;
  g_timer_destroy(timer) ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */



  /* This function can be called in one of two ways: as a result of using    */
  /* giface/sgifaceserver, or using the xremote facility of xace. For the    */
  /* former it is crucial that the below stuff is freed, for the latter it   */
  /* should not be freed as this will leave xace with nothing to display.    */
 finish:

  if (isGifDisplay)					    /* yuch global. */
    {
      /* must kill seqLook before graphCleanup, else will not
	 know if seqLook had a graph and is already destroyed */
      if (seqLook)
	fMapDestroy (seqLook) ;

      if (pepLook)
	pepGifCommandDestroy (pepLook) ;
      
      graphCleanUp () ;		/* kill all graphs except active graph */
      graphDestroy () ;		/* kill active graph */
    }

  isGifDisplay = oldisGifDisplay ;

  return;
} /* gifControl */

/**************************************************/
/**************************************************/
 
 
