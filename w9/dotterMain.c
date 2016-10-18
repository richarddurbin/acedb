/*  File dotterMain.c
 *  Author: esr
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
 * Description: 
 * Exported functions: 
 * HISTORY:
 * Last edited: Aug 26 15:42 2009 (edgrif)
 * Created: Thu Aug 26 17:17:30 1999 (fw)
 * CVS info:   $Id: dotterMain.c,v 1.63 2009-09-02 14:33:54 edgrif Exp $
 *-------------------------------------------------------------------
 */

#include <stdio.h>
#include <ctype.h>
#include <wh/regular.h>
#include <wh/graph.h>
#include <wh/gex.h>
#include <w9/blixem_.h>
#include <wh/dotter_.h>

static void strNamecpy(char *dest, char *src)
{
  char *cp;

  while (*src && *src == ' ')
    src++;

  strcpy(dest, src);

  if ((cp = (char *)strchr(dest, ' ')))
    *cp = 0;
  if ((cp = (char *)strchr(dest, '\n')))
    *cp = 0;

  return ;
}

static char *stringUnprotect(char **textp, char *target)
{
  char *cp, *cpd;
  int count = 0;

 redo:
  cp = *textp;
  cpd = target;
  
  while (*cp)
    {
      if (*cp == '"')
	{
	  cp++;						    /* skip quote */
	  break ;
	}
      else
	cp++ ;
    }

  while (*cp != '"' && *cp)
    {
      if (*cp == '$')
	cp++;

      if (cpd)
	*cpd++ = *cp;
      else
	count++;

      cp++;
    }
  
  if (!target)
    {
      target = messalloc(count+1);
      goto redo;
    }
  
  *cp = '\0' ;
  *textp = cp+1; /* skip quote */

  return target;
}

  
static void addBreakline (MSP **MSPlist, char *name, char *desc, int pos, char seq) {

   MSP   
       *msp;
   char 
       *cp;

   if (!*MSPlist) {
       *MSPlist = (MSP *)messalloc(sizeof(MSP));
       msp = *MSPlist;
   }
   else {
     msp = *MSPlist;
     while(msp->next) msp = msp->next;
     msp->next = (MSP *)messalloc(sizeof(MSP));
     msp = msp->next;
   }

   msp->qname = messalloc(strlen(name)+1);
   strcpy(msp->qname, name);

   msp->desc = messalloc(strlen(desc)+1);
   strcpy(msp->desc, desc);
   if ((cp = (char *)strchr(msp->desc, ' ')))
     *cp = 0;
   if ((cp = (char *)strchr(msp->desc, '\n')))
     *cp = 0;

   msp->qstart = msp->qend = pos;
   *msp->sframe = seq;
   msp->color = DARKGREEN;
   msp->type = FSSEG;
   msp->score = 100;
   insertFS(msp, "chain_separator");
}		      

  
int main(int argc, char **argv)
{
    int     
	l, qoffset=0, 
	soffset=0, 
	selfcall=0, 
	qlen, 
	slen,
	revcompq = 0,
	dotterZoom = 0,
	count,
	install = 1,
	pixelFacset = 0,
	seqInSFS=0;
    char   
	*qseq=0, *sseq=0, line[MAXLINE+1],
	*qname, *sname, *cp, *cc, *cq, type, 
        *firstdesc, *qfilename, *sfilename,
	*savefile = 0,
	*loadfile = 0,
	*FSfilename = 0,
	*mtxfile = 0,
	opts[] = "D    ",	 /* D  display mirror image
			    W  only watson strand
			    C  only crick strand
			    H  only HSPs
			    S  start with swapped greyramptool
			    */
	*winsize = 0,
	text[MAXLINE+1];
    FILE   
	*qfile, *sfile;
    float   
	memoryLimit=0;
    MSP   
	*MSPlist=0, *msp;

    
    int          optc;
    extern int   optind;
    extern char *optarg;
    char        *optstring="b:cDf:F:Hil:M:m:p:q:Rrs:SW:wz:";

    extern char
	*dotterVersion,
	*dotterBinary;

    static char *cc_date = 
#if defined(__DATE__)
    __DATE__
#else
    ""
#endif
    ;

    char *usage;
    static char usageText[] = "\
\n\
 Dotter - Sequence dotplots with image enhancement tools.\n\
\n\
 Reference: Sonnhammer ELL & Durbin R (1995). A dot-matrix program\n\
 with dynamic threshold control suited for genomic DNA and protein\n\
 sequence analysis. Gene 167(2):GC1-10.\n\
\n\
 Usage: dotter [options] <horizontal_sequence> <vertical_sequence>  [X options]\n\
\n\
 Allowed types:                Protein        -      Protein\n\
                               DNA            -      DNA\n\
                               DNA            -      Protein\n\
\n\
 Options:\n\
\n\
 -b <file>      Batch mode, write dotplot to <file>\n\
 -l <file>      Load dotplot from <file>\n\
 -m <float>     Memory usage limit in Mb (default 0.5)\n\
 -z <int>       Set zoom (compression) factor\n\
 -p <int>       Set pixel factor manually (ratio pixelvalue/score)\n\
 -W <int>       Set sliding window size. (K => Karlin/Altschul estimate)\n\
 -M <file>      Read in score matrix from <file> (Blast format; Default: Blosum62).\n\
 -F <file>      Read in sequences and data from <file> (replaces sequencefiles).\n\
 -f <file>      Read feature segments from <file>\n\
 -H             Do not calculate dotplot at startup.\n\
 -R             Reversed Greyramp tool at start.\n\
 -r             Reverse and complement horizontal_sequence (DNA vs Protein)\n\
 -D             Don't display mirror image in self comparisons\n\
 -w             For DNA: horizontal_sequence top strand only (Watson)\n\
 -c             For DNA: horizontal_sequence bottom strand only (Crick)\n\
 -q <int>       Horizontal_sequence offset\n\
 -s <int>       Vertical_sequence offset\n\
\n\
 Some X options:\n\
 -acefont <font> Main font.\n\
 -font    <font> Menu font.\n\
\n\
 See http://www.cgb.ki.se/cgb/groups/sonnhammer/Dotter.html for more info.\n\
\n\
 by Erik.Sonnhammer@cgb.ki.se\n\
 Version ";
    

    usage = messalloc(strlen(usageText) + strlen(dotterVersion) + strlen(cc_date) + 20);
    sprintf(usage, "%s%s, compiled %s\n", usageText, dotterVersion, cc_date);

    
    while ((optc = getopt(argc, argv, optstring)) != EOF)
	switch (optc) 
	{
	case 'b': 
	    savefile = messalloc(strlen(optarg)+1);
	    strcpy(savefile, optarg);         break;
	case 'c': opts[1] = 'C';              break;
	case 'D': opts[0] = ' ';              break;
	case 'f': 
	    FSfilename = messalloc(strlen(optarg)+1);
	    strcpy(FSfilename, optarg);       break;
	case 'F': 
	    seqInSFS = 1;        
	    FSfilename = messalloc(strlen(optarg)+1);
	    strcpy(FSfilename, optarg);       break;
	case 'H': opts[2] = 'H';              break;
	case 'i': install = 0;                break;
	case 'l': 
	    loadfile = messalloc(strlen(optarg)+1);
	    strcpy(loadfile, optarg);         break;
	case 'M': 
	    mtxfile = messalloc(strlen(optarg)+1);
	    strcpy(mtxfile, optarg);          break;
	case 'm': memoryLimit = atof(optarg); break;
	case 'p': pixelFacset = atoi(optarg); break;
	case 'q': qoffset = atoi(optarg);     break;
	case 'R': opts[3] = 'S';              break;
	case 'r': revcompq = 1;               break;
	case 's': soffset = atoi(optarg);     break;
	case 'S': 
	    selfcall = 1;
	    qname = messalloc(strlen(argv[optind])+1); strcpy(qname, argv[optind]);
	    qlen = atoi(argv[optind+1]);
	    sname = messalloc(strlen(argv[optind+2])+1); strcpy(sname, argv[optind+2]);
	    slen = atoi(argv[optind+3]);
	    dotterBinary = messalloc(strlen(argv[optind+4])+1);
	    strcpy(dotterBinary, argv[optind+4]);
	                                      break;
	case 'W': 
	    winsize = messalloc(strlen(optarg)+1);
	    strcpy(winsize, optarg);          break;
	case 'w': opts[1] = 'W';              break;
	case 'z': dotterZoom = atoi(optarg);  break;
	default : fatal("Illegal option");
	}

    if (!savefile)
      {
	graphInit(&argc, argv);
	gexInit(&argc, argv);
      }

    /* Store X options for zooming in */
    {
	int i, len=0;
	extern char *Xoptions;
	
	for (i = optind+2; i < argc; i++)
	    len += strlen(argv[i])+1;
	
	Xoptions = messalloc(len+1);
	
	for (i = optind+2; i < argc; i++) {
	    strcat(Xoptions, argv[i]);
	    strcat(Xoptions, " ");
	}
    }

    if (selfcall) /* Dotter calling dotter */
    {
	qseq = (char *)messalloc(qlen+1);
	sseq = (char *)messalloc(slen+1);

	if ((l=fread(qseq, 1, qlen, stdin)) != qlen) 
	    fatal("Only read %d chars to qseq, expected %d", l, qlen);
	qseq[qlen] = 0;

	if ((l=fread(sseq, 1, slen, stdin)) != slen) 
	    fatal("Only read %d chars to sseq, expected %d", l, slen);
	sseq[slen] = 0;

	/* Read in MSPs */
	while (!feof (stdin))
	{ 
	    char *cp;
	    if (!fgets (text, MAXLINE, stdin) || (unsigned char)*text == (unsigned char)EOF ) break;
	    
	    if (!MSPlist) {
		MSPlist = (MSP *)messalloc(sizeof(MSP));
		msp = MSPlist;
	    }
	    else {
		msp->next = (MSP *)messalloc(sizeof(MSP));
		msp = msp->next;
	    }
	    
	    cp = text;
	    while (*cp == ' ') cp++;
	    msp->type = strtol(cp, &cp, 10);
	    while (*cp == ' ') cp++;
	    msp->score = strtol(cp, &cp, 10);
	    while (*cp == ' ') cp++;
	    msp->color = strtol(cp, &cp, 10);
	    while (*cp == ' ') cp++;
	    msp->qstart = strtol(cp, &cp, 10); 
	    while (*cp == ' ') cp++;
	    msp->qend = strtol(cp, &cp, 10); 
	    while (*cp == ' ') cp++;
	    msp->fs = strtol(cp, &cp, 10);
	    msp->sname = stringUnprotect(&cp, NULL);
	    stringUnprotect(&cp, msp->sframe);
	    msp->qname = stringUnprotect(&cp, NULL);
	    stringUnprotect(&cp, msp->qframe);
	    msp->desc = stringUnprotect(&cp, NULL);


	    /* really horrible hack */
	    if (msp->type == FSSEG)
	      {
		insertFS(msp, "chain_separator");
		opts[4] = 'L';
	      }
	}
	fclose(stdin);
    }
    else if (!seqInSFS)
    {
	if (argc - optind < 2) {
	    fprintf(stderr, "%s\n", usage); 
	    exit(1);
	}
	else if(!(qfile = fopen(argv[optind], "r"))) {
	    fprintf(stderr,"Cannot open %s\n", argv[optind]); exit(1);
	}
	qfilename = argv[optind];
	fseek(qfile, 0, SEEK_END);
	qlen = ftell(qfile);
	qseq = (char *)messalloc(qlen+1);
	fseek(qfile, 0, SEEK_SET);
	if ((cp = (char *)strrchr(argv[optind], '/')))
	  qname = strnew(cp+1, 0);
	else
	  qname = strnew(argv[optind], 0);

	if (!(sfile = fopen(argv[optind+1], "r"))) {
	    fprintf(stderr,"Cannot open %s\n", argv[optind+1]); exit(1);
	}
	sfilename = argv[optind+1];
	fseek(sfile, 0, SEEK_END);
	slen = ftell(sfile);
	sseq = (char *)messalloc(slen+1);
	fseek(sfile, 0, SEEK_SET);
	if ((cp = (char *)strrchr(argv[optind]+1, '/')))
	  sname = strnew(cp+1, 0);
	else
	  sname = strnew(argv[optind+1], 0);

	/* Read in the sequences */
	l = count = 0;
	cc = qseq;
	while (!feof(qfile))
	{
	    if (!fgets(line, MAXLINE, qfile))
	      break;
	    if ((cq = (char *)strchr(line, '\n')))
	      *cq = 0;

	    /* Name headers */
	    if ((cq = (char *)strchr(line, '>'))) {
		cq++;
		if (++l == 1) {
		    qname = messalloc(strlen(cq)+1); strNamecpy(qname, cq);
		    firstdesc = messalloc(strlen(cq)+1);
		    strcpy(firstdesc, cq);
		}
		else {
		  /* Multiple sequences - add break lines */

		    if (l == 2) {
			opts[4] = 'L';

		        /* Second sequence - add break line to mark first sequence */
		        addBreakline (&MSPlist, qfilename, firstdesc, 0, '1');
			
			/* change sequence name to filename */
			qname = messalloc(strlen(qfilename)+1); strcpy(qname, qfilename);
		    }
		    addBreakline (&MSPlist, qfilename, cq, count, '1');
		}
	    }
	    else {
		for (cq = line; *cq; cq++) if (isalpha(*cq)) {
		    *cc++ = *cq;
		    count++;
		}
	    }
	}
	*cc = 0;
	
	l = count = 0;
	cc = sseq;
	while (!feof(sfile))
	{
	    if (!fgets(line, MAXLINE, sfile))
	      break;
	    if ((cq = (char *)strchr(line, '\n')))
	      *cq = 0;

	    /* Name headers */
	    if ((cq = (char *)strchr(line, '>'))) {
		cq++;
		if (++l == 1) {
		    sname = messalloc(strlen(cq)+1); strNamecpy(sname, cq);
		    firstdesc = messalloc(strlen(cq)+1);
		    strcpy(firstdesc, cq);
		}
		else {
		  /* Multiple sequences - add break lines */

		    if (l == 2) {
			opts[4] = 'L';

		        /* Second sequence - add break line to mark first sequence */
		        addBreakline (&MSPlist, sfilename, firstdesc, 0, '2');
			
			/* change sequence name to filename */
			sname = messalloc(strlen(sfilename)+1); strcpy(sname, sfilename);
		    }
		    addBreakline (&MSPlist, sfilename, cq, count, '2');
		}
	    }
	    else {
		for (cq = line; *cq; cq++) if (isalpha(*cq)) {
		    *cc++ = *cq;
		    count++;
		}
	    }
	}
	*cc = 0;
    }

    if (FSfilename) {
	char dummyopts[32];	/* opts have different meaning in blixem */
	FILE *file;
	
	if (!strcmp(FSfilename, "-")) {
	    file = stdin;
	}
	else if(!(file = fopen(FSfilename, "r")))
	    messcrash("Cannot open %s\n", FSfilename);
	
	parseFS(&MSPlist, file, dummyopts, &qseq, qname, &sseq, sname);
    }

    /* Determine sequence types */
    if (Seqtype(qseq) == 'P' && Seqtype(sseq) == 'P') {
	printf("\nDetected sequence types: Protein vs. Protein\n");
	type = 'P';
    }
    else if (Seqtype(qseq) == 'N' && Seqtype(sseq) == 'N') {
	printf("\nDetected sequence types: DNA vs. DNA\n");
	type = 'N';
    }
    else if (Seqtype(qseq) == 'N' && Seqtype(sseq) == 'P') {
	printf("\nDetected sequence types: DNA vs. Protein\n");
	type = 'X';
    }
    else fatal("Illegal sequence types: Protein vs. DNA - turn arguments around!\n\n%s", usage);

    if (revcompq) {
	if (type != 'X')
	    fatal("Revcomp'ing horizontal_sequence only needed in DNA vs. Protein");
	else {
	    cc = messalloc(qlen+1);
	    revComplement(cc, qseq);
	    messfree(qseq);
	    qseq = cc;
	}
    }

    /* Add -install for private colormaps */
    if (install) argvAdd(&argc, &argv, "-install");

    if (!savefile)
      {
#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
	graphLoop(dotter(type, opts, qname, qseq, qoffset, sname, sseq, soffset, 
			 0, 0, savefile, loadfile, mtxfile, memoryLimit, dotterZoom, MSPlist, 0, winsize, pixelFacset));
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

	dotter(type, opts, qname, qseq, qoffset, sname, sseq, soffset, 
	       0, 0, savefile, loadfile, mtxfile, memoryLimit, dotterZoom, MSPlist, 0, winsize, pixelFacset) ;

	graphLoop(0) ;

	graphFinish () ;
      }
    else
      {
	/* stop graphical dialog boxes in batch mode. */
	struct messContextStruct nullContext = { NULL, NULL };
	messOutRegister(nullContext);
	messErrorRegister (nullContext);
	messExitRegister(nullContext);
	messCrashRegister (nullContext);
	dotter(type, opts, qname, qseq, qoffset, sname, sseq, soffset, 
	       0, 0, savefile, loadfile, mtxfile, memoryLimit, dotterZoom, MSPlist, 0, winsize, pixelFacset);
      }

    return (0) ;
}
