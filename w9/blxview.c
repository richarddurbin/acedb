/*

   BLIXEM - BLast matches In an X-windows Embedded Multiple alignment

 -------------------------------------------------------------
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
 * -------------------------------------------------------------
   |  File: blxview.c                                          |
   |  Author: Erik.Sonnhammer                                  |
   |  Copyright (C) E Sonnhammer 1993-1997                     |
   -------------------------------------------------------------
 *
 * Exported functions: See wh/blxview.h
 * HISTORY:
 * Last edited: Oct  7 11:39 2010 (edgrif)
 * * Jan 10 10:35 2002 (edgrif): Fix up socket code and add various
 *              options for better sequence display.
 * previous mods:
  Date      Modification
--------  ---------------------------------------------------
93-02-20  Created
93-05-25  Dispstart/dispend fix by Richard for seq's < displen.
93-07-24  All boxes of a protein turn black when one is picked
          Sorting by protein name or score added
93-11-16  Added picking of Big Picture HSP's and Reverse Strand Big Picture
93-11-17  Added blastn support
94-01-29  Added Highlight sequences by names matching regexp
94-03-07  Fixed window limits. Added initial settings (BigPict, gotoNext)
94-03-08  Added 'always-both-strands' in blastn mode.
94-03-27  Added Tblastn support (works in seqbl mode).
94-12-01  [2.0] Added dotter calling.
95-02-02  Rewrote auxseq and padseq allocation to be fully dynamic and unlimited.
95-02-06  Added Tblastx support (works in seqbl mode).
95-06-01  [2.1] Added entropy display
95-06-23  [2.1] Added Settings window
95-07-21  2.1 announced--------------------------------------
95-08-01  [2.2] Initial Sorting mode on command line.
95-09-15  [2.2] Added Settings pull-down menu for faster manipulation.
95-09-29  [2.2] Improved WWW browser finding with findCommand() - doesn't get fooled so easily.
95-10-04  [2.2] Added "Print whole alignment"
95-10-27  [2.2] Added acedb-fetching at double clicking.
          BLIXEM_FETCH_ACEDB makes this default.
          Reorganised Settings window to Toggles and Menus rows.
95-11-01  [2.3] Changed command line syntax to "-" for piping.
          Added X options capability (-acefont, -font).
96-01-05  [2.3] Force all tblastn HSP's to be qframe +1, to harmonize with MSPcrunch 1.4
          which gives sframe for tblastn (otherwise the output would be dead).
96-02-08  [2.3] Added option -S "Start display at position #" to stand-alone command line.
96-02-08  [2.3] Added checkmarks to pull-down settings menu.
96-03-05  2.3 Announced.
96-03-08  [2.4] Only look for WWW browser once.
96-05-09  [2.4] Proper grayscale print colours.
96-05-09  [2.4] Enabled piping of query sequence too, for Pepmap and WWW calls.
96-08-20  [2.4] Fixed minor bug in squashed mode and added restoring of previous sorting after squash.
97-05-28  [2.4] Fixed parsing to handle gapped matches.
                Added "Highlight lower case residues" for gapped alignments and
                "Show sequence descriptions" (for MSPcrunch 2.1).
		Added setting the color of matching residues in the Settings window.
97-06-17  [2.4] Fixed "Highlight differences" for gapped alignments ('.' -> '-').
                Changed "Highlight lower case residues" to "Highlight subject insertions" and
		set this automatically for gapped alignments.  Works for both lower case and number
		insert markers.
                Changed blviewRedraw to use strlen to accommodate reverse gapped alignments.
		Simplified (and thereby debugged) selection of Big Picture MSPs to be drawn.
		Made Big Picture ON/OFF control more logical and consistent.
		Added a calcID() step to fix sortById() at startup.
		Added "Hilight Upper/Lower case" - useful general purpose feature.
97-10-14  [2.4] Added Description box when picking sequences.
97-03-07  [3.0] Added code for FS Feature Segment data. (shared code with Dotter for
                control and parsing; the display code is unique to Blixem).
                Added Inverted sorting order.
		Fixed bug in coordinate display in tblastn mode.
99-07-07  [3.0] Added msp->shape attribute. Added support for XY curve shapes PARTIAL and INTERPOLATE.
                Overhauled selective drawing of BigPicture MSPs, now simple enough to be bugfree (hopefully).
01-10-05	Added getsseqsPfetch to fetch all missing sseqs in one go via socket connection to pfetch [RD]

 * Created: Thu Feb 20 10:27:39 1993 (esr)
 * CVS info:   $Id: blxview.c,v 1.218 2010-10-07 10:47:31 edgrif Exp $
 *-------------------------------------------------------------------
 */


/*
		Pending:

		        Do exons and introns like SFS segments (i.e. eliminate
			magic scores.  Requires changes to fmapfeatures.c).

Known bugs:
-----------

revcomp broken when called from acedb.  Slen/send problem?

MSP score codes:
----------------
-1 exon                  -> Big picture + Alignment
-2 intron                -> Big picture + Alignment
-3 Any coloured segment  -> Big picture
-4 stringentSEGcolor     -> Big picture
-5 mediumSEGcolor        -> Big picture
-6 nonglobularSEGcolor   -> Big picture
-10 hidden by hand
*/

#include <math.h>
#include <ctype.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <stdarg.h>
#include <sys/time.h>
#include <gdk/gdkkeysyms.h>

#include <wh/aceio.h>
#include <wh/graph.h>
#include <wh/gex.h>
#include <wh/key.h>
#include <wh/menu.h>
#include <wh/dotter.h>
#include <wh/dict.h>
#include <w9/blixem_.h>

#ifdef ACEDB
#include <wh/display.h>
#endif


/* This is the only place this is set so that you get the same version/compile whether this is
 * compiled stand alone or as part of xace. */
char *blixemVersion = BLIXEM_VERSION_COMPILE ;



/* get rid of these...ugh.....horrible.....horrific.... */
extern void externalCommand (char *command);
extern int pickMatch (char *cp, char *tp);


typedef struct _BPMSP
{
  char sname[FULLNAMESIZE+1];
  char *desc;
  int box;
  Graph graph;
  struct _BPMSP *next;
} BPMSP;



#define max(a,b) (((a) > (b)) ? (a) : (b))
#define BPoffset 4
#define NAMESPACE 12
#define SEQ2BP(i) (float)plusmin*(i-BigPictStart-qoffset)*BPx/BigPictLen + BPoffset
#define MAXALIGNLEN 10000
#define separatorwidth 0.5

#define autoDotterParamsStr "Automatic Dotter parameters"
#define SortByScoreStr      "Sort by score"
#define SortByIdStr         "Sort by identity"
#define SortByNameStr       "Sort by name"
#define SortByPosStr        "Sort by position"
#define SortInvStr          "Inverted sorting order"
#define BigPictToggleStr    "Big Picture"
#define BigPictToggleRevStr "Big Picture Other Strand"
#define toggleIDdotsStr     "Highlight differences"
#define squashMatchesStr    "Squash matches"
#define squashFSStr         "Squash features"
#define entropytoggleStr    "Complexity curves"
#define printColorsStr      "B/W Print colours"
#define toggleColorsStr     "No colours"
#define toggleVerboseStr    "Verbose mode"
#define toggleHiliteSinsStr  "Highlight subject insertions"
#define toggleHiliteUpperStr "Highlight upper case"
#define toggleHiliteLowerStr "Highlight lower case"
#define toggleDESCStr        "Show sequence descriptions"
#define toggleMatchPasteStr  "Paste Match Set\t\t\t\tm"
#define toggleMatchClearStr  "Clear Match Set\t\t\t\tm"

/* MSP list is sorted by one of these criteria, currently SORTBYID is the default. */
typedef enum {SORTBYUNSORTED, SORTBYSCORE, SORTBYID, SORTBYNAME, SORTBYPOS} SortByType ;

static void initBlxView(void) ;
static void blxDestroy(void) ;
static void blxPrint(void) ;
static void blxHelp(void);
static void wholePrint(void) ;
static void blxShowStats(void) ;

static void blxPaste(BlxPasteType paste_type) ;
static void pasteCB(char *text) ;
BOOL parsePasteText(char *text, BlxPasteData paste_data) ;
void freePasteData(BlxPasteData paste_data) ;
static char *parseMatchLines(char *text) ;
static BOOL parseFeatureLine(char *line,
			     char **feature_name_out,
			     int *feature_start_out, int *feature_end_out, int *feature_length_out) ;

static BOOL setMatchSet(char **matches) ;
static void clearMatchSet(void) ;

static void sortByName(void) ;
static void sortByScore(void) ;
static void sortByScore(void) ;
static void sortByPos(void) ;
static void sortById(void) ;
static void sortToggleInv(void) ;

static void MSPsort(SortByType sort_mode) ;
static void squashMatches(void) ;
static void squashFSdo(void) ;

static void toggleIDdots (void) ;
static void toggleVerbose(void) ;
static void toggleHiliteSins(void) ;
static void toggleHiliteUpper(void) ;
static void toggleHiliteLower(void) ;
static void toggleDESC(void) ;
static void ToggleStrand(void) ;
static void printColors (void) ;

static void scrollRightBig(void);
static void scrollLeftBig(void);
static void scrollRight1(void);
static void scrollLeft1(void);

static void Goto(void) ;
static void gotoMatch(int direc) ;
static BOOL gotoMatchPosition(char *match, int q_start, int q_end) ;
static void prevMatch(void) ;
static void nextMatch(void) ;

static void keyboard(int key, int modifier) ;
static void toggleColors (void);
static void blviewPick (int box, double x_unused, double y_unused, int modifier_unused);
static Graph blviewCreate(char *opts, char *align_types) ;
static void blviewDestroy(GtkWidget *unused) ;
static char *getqseq(int start, int end, char *q);
static char *get3rd_base(int start, int end, char *q);
static void calcID(MSP *msp);
static int gapCoord(MSP *msp, int x, int qfact);
static GtkWidget *makeButtonBar(void);

static BOOL haveAllSequences(MSP *msplist, DICT *dict) ;
static char *fetchSeqRaw(char *seqname) ;
static void getsseq(MSP *msp) ;
static char *getSeq(char *seqname, char *fetch_prog) ;

static BOOL smartDotterRange(char *selected_sequence, MSP *msp_list, int blastn,
			     int strand_sign, int view_start, int view_end,
			     char **dottersseq_out, int *dotter_start_out, int *dotter_end_out) ;

static char *abbrevTxt(char *text, int max_len) ;

static GtkToolItem *addToolbarWidget(GtkToolbar *toolbar, GtkWidget *widget) ;
static GtkToolItem *makeNewToolbarButton(GtkToolbar *toolbar,
					 char *label,
					 char *tooltip,
					 GtkSignalFunc callback_func) ;
static void printMSPs(void) ;

static void toggleMatchSet(void) ;

static void getMspRangeExtents(MSP *msp, int *qMin, int *qMax, int *sMin, int *sMax);
static void getSMapMapRangeExtents(SMapMap *range, int *rMin, int *rMax, int *sMin, int *sMax);


/*
 *                 Local globals....sigh....
 */

static void     BigPictToggle(void),
            entropytoggle(void),
            BigPictToggleRev(void),
            zoomIn(void),
            zoomOut(void),
            zoomWhole(void),
            MiddleDragBP(double x, double y),
            MiddleUpBP(double x, double y),
            MiddleDownBP(double x, double y),
            MiddleDragQ(double x, double y),
            MiddleUpQ(double x, double y),
            MiddleDownQ(double x, double y),
            setHighlight(void),
            clrHighlight(void),
            callDotter(void),
            callDotterHSPs(void),
            callDotterSelf(void),
  /*             dotterPanel(void), */
            setDotterParams(void),
            autoDotterParams(void),
            allocAuxseqs(int len),
            blixemSettings(void),
            settingsRedraw(void),
            menuCheck(MENU menu, int mode, int thismode, char *str),
	    hidePicked(void),
            setMenuCheckmarks(void);


#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
/* currently unused... */
static void pfetchWindow (MSP *msp);
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */



/* GLOBAL VARIABLES...really its hard to beat this for a list of globals,
 * one for the Guinness Book of Records perhaps... */

static BPMSP BPMSPlist, *bpmsp;
static int  fetchCount;
static Graph blixemGraph=0;        /* The blxview window */
GtkWidget *blixemWindow = NULL ;
static GtkWidget *messageWidget;
static Graph frameGraph[3];
static Graph frameGraphQ[3];
static Graph settingsGraph=0;

static int   actstart,        /* Active region coordinates relative to query in BASES */
             actend,
             dispstart,       /* Displayed region relative to query in BASES */
             displen=240;     /* Displayed sequence length in BASES */
static char  actframe[16]="(+1)";    /* Active frame */
static int   plusmin = 1 ;       /* +1 for top strand, -1 */
static float queryy,          /* Screen coords of genome seq */
             separator_y,	/* y coord of previous panel separator */
             lastExonx,
             BPboxheight = 5.7,
             BPboxstart,
             BPboxwidth,
             BigPictZoom = 10,
  oldLinew;


static char *q;    /* The genomic sequence (query=the translated seq) */
static int   qlen;
static int   qoffset;         /* Offset to the 'real' DNA start */
static char *qname_G = NULL ;
       MSP  *MSPlist,          /* List of MSP's */
            *msp;
static MSP  *pickedMSP = NULL ;	      /* Last picked MSP */
static char  message[1024],

             HighlightSeq[LONG_NAMESIZE+4] = "",

             searchSeq[NAMESIZE+4] = "",
            *cp,
            *padseq = 0,
            *auxseq = 0,
            *auxseq2 = 0,
            *dottersseq = NULL,
  dotterqname[LONG_NAMESIZE] = "",
             stringentEntropytx[10],
             mediumEntropytx[10],
             nonglobularEntropytx[10],
  sortModeStr[32] = "Identity" ;


static BlxPasteType paste_type_G = BLXPASTE_INVALID ;

static int   lastbox = 0;
static int   colortoggle = 0;
static int   backgColor = LIGHTGRAY,
             IDcolor = CYAN,
             consColor = MIDBLUE,
             geneColor = BLUE,
             hiColor = YELLOW,
             matchSetColor = CERISE,
             oldcolor,
             BigPictON = 1,
             BigPictRev = 0,	/* Draw other strand in Big picture */
             BigPictStart, BigPictLen, BPbox, BPx,
             gridColor = YELLOW,
             nx, ny,

             blastx = 1, blastp = 0, blastn = 0, tblastn = 0, tblastx = 0,

             symbfact = 3,
             i,
             start_nextMatch = 0,
             dotter_first = 0,
             IDdots = 0,
             squash = 0,
             squashFS = 1,
             verbose = 0,
             HiliteSins = 0,
             HiliteUpperOn = 0,
             HiliteLowerOn = 0,
             DESCon = 0,
             dotterZoom = 0,
             dotterStart = 0,
             dotterEnd = 0,
             dotterHSPs = 0,
             auxseqlen = 0,
             smartDotter = 1,
             entropyOn = 0,
             stringentEntropycolor = LIGHTGREEN,
             stringentEntropybox,
             mediumEntropycolor = GREEN,
             mediumEntropybox,
             nonglobularEntropycolor = DARKGREEN,
             nonglobularEntropybox,
             alphabetsize,
             stringentEntropywin = 12,
             mediumEntropywin = 25,
             nonglobularEntropywin = 45,
             printColorsOn,
             wholePrintOn,
             oneGraph,
             settingsButton,
             sortInvOn = 0,
             HSPgaps = 0;

static SortByType sortMode = SORTBYUNSORTED ;

static int oldWidth = 0, oldHeight = 0;
static double oldx, DNAstep;
static Array stringentEntropyarr, mediumEntropyarr, nonglobularEntropyarr;

static MENU blixemMenu ;	/* Main menu - contains function calls, etc. */
static MENU settingsMenu;	/* Contains toggles and 'states' */

/* The standard context menu */
static MENUOPT mainMenu[] =
{
  {blxDestroy,       "Quit\t\t\t\t\tCtrl-Q"},
  {blxHelp,          "Help\t\t\t\t\tCtrl-H"},
  {blxPrint,         "Print\t\t\t\t\tCtrl-P"},
  {wholePrint,       "Print whole alignment"},
  {blixemSettings,   "Change Settings"},
  {selectFeatures,   selectFeaturesStr},

  {menuSpacer,       ""},
  {toggleMatchSet,   toggleMatchPasteStr},

  {menuSpacer,       ""},
  /*	{dotterPanel,       "Dotter panel"},*/
  {callDotter,       "Dotter"},
  {callDotterHSPs,   "Dotter HSPs only"},
  {callDotterSelf,   "Dotter query vs. itself"},
  {setDotterParams,  "Manual Dotter parameters"},
  {autoDotterParams, autoDotterParamsStr},

  {menuSpacer,       ""},
  {hidePicked,       "Hide picked match"},
  {setHighlight,     "Highlight sequences by name"},
  {clrHighlight,     "Clear highlighted and unhide"},

  {0, 0}
};

/* The standard context menu modified to include additional options for developers */
static MENUOPT developerMainMenu[] =
{
  {blxDestroy,       "Quit\t\t\t\t\tCtrl-Q"},
  {blxHelp,          "Help\t\t\t\t\tCtrl-H"},
  {blxPrint,         "Print\t\t\t\t\tCtrl-P"},
  {wholePrint,       "Print whole alignment"},
  {blixemSettings,   "Change Settings"},
  {selectFeatures,   selectFeaturesStr},
  
  {menuSpacer,       ""},
  {toggleMatchSet,   toggleMatchPasteStr},
  
  {menuSpacer,       ""},
  /*	{dotterPanel,       "Dotter panel"},*/
  {callDotter,       "Dotter"},
  {callDotterHSPs,   "Dotter HSPs only"},
  {callDotterSelf,   "Dotter query vs. itself"},
  {setDotterParams,  "Manual Dotter parameters"},
  {autoDotterParams, autoDotterParamsStr},
  
  {menuSpacer,       ""},
  {hidePicked,       "Hide picked match"},
  {setHighlight,     "Highlight sequences by name"},
  {clrHighlight,     "Clear highlighted and unhide"},
  
  {menuSpacer,       ""},
  {blxShowStats,     "Statistics"},
  {0, 0}
};

static MENUOPT settingsMenuOpt[] =
{
  {sortByScore,      SortByScoreStr},
  {sortById,         SortByIdStr},
  {sortByName,       SortByNameStr},
  {sortByPos,        SortByPosStr},
  {sortToggleInv,    SortInvStr},
  {menuSpacer,       ""},
  {BigPictToggle,    BigPictToggleStr},
  {BigPictToggleRev, BigPictToggleRevStr},
  {entropytoggle,        entropytoggleStr},
  {menuSpacer,       ""},
  {toggleDESC,       toggleDESCStr},
  {toggleHiliteSins, toggleHiliteSinsStr},
  {squashFSdo,       squashFSStr},
  {squashMatches,    squashMatchesStr},
  {toggleIDdots,     toggleIDdotsStr},
  {toggleHiliteUpper,toggleHiliteUpperStr},
  {toggleHiliteLower,toggleHiliteLowerStr},
  {menuSpacer,       ""},
  {printColors,      printColorsStr},
  {toggleColors,     toggleColorsStr},
  {toggleVerbose,    toggleVerboseStr},
  {menuSpacer,       ""},
  {blixemSettings,   "Settings window"},
  {0, 0}
};


/* A stepping stone to having a blixem view context is this global struct. */
static BlixemViewStruct blixem_context_G = {FALSE} ;



/*
 *                External routines
 */










/*
 *                Internal routines
 */

/* Temporary function while introducing a blixem view context. */
static BlixemView getBlxViewContext(void)
{
  return &blixem_context_G ;
}


static void toggleMatchSet(void)
{
  BlixemView blxview = getBlxViewContext() ;
  BOOL result ;

  if (blxview->match_set)
    {
      clearMatchSet() ;
      blxview->match_set = FALSE ;

      result = menuSetLabel(menuItem(blixemMenu, toggleMatchClearStr), toggleMatchPasteStr) ;
    }      
  else
    {
      /* We cannot reset the menu yet because blxPaste() is asynchronous so we don't know if it worked. */
      blxPaste(BLXPASTE_MATCHSET) ;
    }



  return ;
}



static void toggleVerbose(void)
{
  verbose = (verbose ? 0 : 1);

  if (verbose)
    printMSPs() ;

  blviewRedraw();

  return ;  
}

static void toggleHiliteSins(void)
{
    HiliteSins = (HiliteSins ? 0 : 1);
    blviewRedraw();
}

static void toggleHiliteUpper(void)
{
    HiliteUpperOn = (HiliteUpperOn ? 0 : 1);
    blviewRedraw();
}

static void toggleHiliteLower(void) {
    HiliteLowerOn = (HiliteLowerOn ? 0 : 1);
    blviewRedraw();
}

static void toggleDESC(void) {
    DESCon = (DESCon ? 0 : 1);
    blviewRedraw();
}

static void ToggleStrand(void)
{
  dispstart += plusmin*(displen-1);

  plusmin = -plusmin;

  sprintf(actframe, "(%+d)", plusmin);
  blviewRedraw();

  return ;
}

static void scrollRightBig(void)
{
    dispstart = dispstart + plusmin*displen*.5;
    blviewRedraw();
}
static void scrollLeftBig(void)
{
    dispstart = dispstart - plusmin*displen*.5;
    blviewRedraw();
}
static void scrollRight1(void)
{
    dispstart = dispstart + plusmin*symbfact;
    blviewRedraw();
}
static void scrollLeft1(void)
{
    dispstart = dispstart - plusmin*symbfact;
    blviewRedraw();
}

static void Goto(void)
{
  static char dfault[32] = "";
  int i = 0;
  ACEIN pos_in;

  /*sprintf(posstr, "%d", dispstart + qoffset);*/

  if ((pos_in = messPrompt ("Goto which position: ", dfault, "t", 0)))
    {
      aceInInt(pos_in, &i);
      aceInDestroy (pos_in);

      dispstart = i - qoffset ;
      sprintf(dfault, "%d", i) ;

      blviewRedraw();
    }

  return ;
}


/* AGH...strand probably not good..... */
static BOOL gotoMatchPosition(char *match, int q_start, int q_end)
{
  BOOL result = FALSE ;
  MSP *msp ;

  for (msp = MSPlist; msp ; msp = msp->next)
    {
      if (g_ascii_strcasecmp(msp->sname, match) == 0)
	{
	  if (msp->qstart == (q_start - qoffset) && msp->qend == (q_end - qoffset))
	    {
	      dispstart = q_start - qoffset ;

	      blviewRedraw() ;

	      result = TRUE ;

	      break ;
	    }
	}
    }

  return result ;
}


static void gotoMatch(int direc)
{
  MSP *msp, *closest;
  int offset, closest_offset;
  char strand;

  if (strchr(actframe, '+'))
    strand = '+';
  else
    strand = '-';

  if (direc != -1 && direc != 1)
    {
      messerror( "gotoMatch must have -1 or 1 as argument\n" ) ;

      return ;
    }

  closest_offset = 0;
  closest = NULL;

  for (msp = MSPlist; msp ; msp = msp->next)
    {
      if (strchr(msp->qframe, strand) && (msp->qstart - dispstart)*plusmin*direc - 2 > 0)
	{
	  offset = (msp->qstart - dispstart)*plusmin*direc - 2;

	  if (!closest_offset)
	    closest_offset = offset;
	  else if (offset < closest_offset)
	    closest_offset = offset;
	}
    }

  if (!closest_offset)
    {
      blviewRedraw();
    }
  else
    {
      if (direc < 0)
	dispstart -= 4 * plusmin ;

      dispstart = dispstart + (direc * plusmin * (closest_offset)) ;
      
      blviewRedraw();
    }

  return ;
}

static void prevMatch(void)
{
  gotoMatch(-1);
}

static void nextMatch(void)
{
  gotoMatch(1);
}


static void keyboard(int key, int modifier)
{
  switch (key)
    {
    case '<':
    case ',':
      scrollLeft1();
      break;
    case '>':
    case '.':
      scrollRight1();
      break;

    case UP_KEY:
      blviewPick(lastbox - 1, 0, 0, 0);
      break;
    case DOWN_KEY:
      blviewPick(lastbox + 1, 0, 0, 0);
      break;

    case GDK_G:
    case GDK_g: 
      {
	blxPaste(BLXPASTE_MOVETO) ;
	break ;
      }

    case GDK_M:
    case GDK_m: 
      {
	toggleMatchSet() ;
	break ;
      }

    default:
      break;
    }

  return ;
}


static void toggleColors (void)
{
    static int oldback, oldgrid, oldID, oldcons, oldgene, oldhi;

    graphActivate(settingsGraph);

    if (!colortoggle) {
	oldback = backgColor; backgColor = WHITE;
	oldgrid = gridColor; gridColor = BLACK;
	oldID = IDcolor; IDcolor = WHITE;
	oldcons = consColor; consColor = WHITE;
	oldgene = geneColor; geneColor = BLACK;
	oldhi = hiColor; hiColor = WHITE;
	colortoggle = 1;
    }
    else {
	backgColor = oldback;
	gridColor= oldgrid;
	IDcolor = oldID;
	consColor = oldcons;
	geneColor = oldgene;
	hiColor = oldhi;
	colortoggle = 0;
    }
    blviewRedraw();
}


static void printColors (void)
{
    static int oldback, oldgrid, oldID, oldcons, oldgene, oldhi;

    graphActivate(settingsGraph);

    if (!printColorsOn) {
	oldback = backgColor; backgColor = WHITE;
	oldgrid = gridColor; gridColor = LIGHTGRAY;
	oldID = IDcolor; IDcolor = GRAY;
	oldcons = consColor; consColor = PALEGRAY;
	oldgene = geneColor; geneColor = BLACK;
	oldhi = hiColor; hiColor = LIGHTGRAY;
	printColorsOn = 1;
    }
    else {
	backgColor = oldback;
	gridColor= oldgrid;
	IDcolor = oldID;
	consColor = oldcons;
	geneColor = oldgene;
	hiColor = oldhi;
	printColorsOn = 0;
    }
    blviewRedraw();
}


static void toggleIDdots (void)
{
    IDdots = !IDdots;
    blviewRedraw();
}


static void calcEntropyArray(Array array, int win)
{
    int i, j, *rescount;
    float pi, sum;

    rescount = (int *)messalloc(24*sizeof(int));

    for (i=0; i < qlen; i++) {
	rescount[aa_atob[(unsigned int)q[i]]]++;
	if (i+1 >= win) {
	    for (sum = j = 0; j < 24; j++)
		if (rescount[j]) {
		    pi = (float)rescount[j]/win;
		    sum += pi*log(pi);
		}
	    arr(array, i-win/2, float) = -sum/log(2);
	    rescount[aa_atob[(unsigned int)q[i+1-win]]]--;
	}
    }

    messfree(rescount);

    /* TEST - delete later * /
    for (i=0; i < qlen; i++)
	printf ("%3d  %c  %f\n", i, q[i], arr(stringentEntropyarr, i, float));
	*/
}

static void calcEntropyArrays(BOOL force)
{
    /* force:
       FALSE - only calculate if necessary, i.e. first call.
       TRUE - force (re)calculation.
       */

    if (!stringentEntropyarr) {
	calcEntropyArray(stringentEntropyarr = arrayCreate(qlen, float), stringentEntropywin);
	calcEntropyArray(mediumEntropyarr = arrayCreate(qlen, float), mediumEntropywin);
	calcEntropyArray(nonglobularEntropyarr = arrayCreate(qlen, float), nonglobularEntropywin);
    }
    else if (force) {
	calcEntropyArray(stringentEntropyarr = arrayCreate(qlen, float), stringentEntropywin);
	calcEntropyArray(mediumEntropyarr = arrayCreate(qlen, float), mediumEntropywin);
	calcEntropyArray(nonglobularEntropyarr = arrayCreate(qlen, float), nonglobularEntropywin);
    }
}


static void entropytoggle (void)
{
    entropyOn = !entropyOn;
    if (entropyOn) BigPictON = 1;
    calcEntropyArrays(FALSE);
    blviewRedraw();
}


/* Find the expression 'query' in the string 'text'
 * Return 1 if found, 0 otherwise
 */
int strMatch(char *text, char *query)
{
    /* Non-ANSI bsd way: * /
       if (re_exec(text) == 1) return 1;
       else return 0;
    */

    /* ANSI way: */
    return pickMatch(text, query);
}

void highlightProteinboxes(BOOL warpScroll)
{
  MSP *msp;

  /* Highlight alignment boxes of current search string seq */
  if (*searchSeq)
    for (msp = MSPlist; msp ; msp = msp->next)
      if (msp->box && strMatch(msp->sname, searchSeq))
	{
	  graphActivate(msp->graph);
	  graphBoxDraw(msp->box, BLACK, RED);
	}

  /* Highlight alignment boxes of currently selected seq */
  if (!squash)
    {
      for (msp = MSPlist; msp ; msp = msp->next)
	{
	  if (msp->box && !strcmp(msp->sname, HighlightSeq))
	    {
	      float x1, x2, y1, y2;

	      graphActivate(msp->graph);
	      graphBoxDraw(msp->box, WHITE, BLACK);

	      if (warpScroll)
		{
		  /* Scroll the alignment window so that the currently
		     selected seq is visible. Not that this only happens
		     in response to clicking on the big picture
		     when warpScroll is TRUE. */
		  graphBoxDim(msp->box, &x1, &y1, &x2, &y2);
		  graphGoto(x1, y1);
		}
	    }
	  }
    }

  if (BigPictON)
    {
      /* Highlight Big Picture boxes of current search string seq */
      if (*searchSeq)
	{
	  for (bpmsp = BPMSPlist.next; bpmsp && *bpmsp->sname; bpmsp = bpmsp->next)
	    {
	      if (strMatch(bpmsp->sname, searchSeq))
		{
		  graphActivate(bpmsp->graph);
		  graphBoxDraw(bpmsp->box, RED, BLACK);
		}
	    }
	}

      /* Highlight Big Picture boxes of currently selected seq */
      for (bpmsp = BPMSPlist.next; bpmsp && *bpmsp->sname; bpmsp = bpmsp->next)
	{
	  if (!strcmp(bpmsp->sname, HighlightSeq))
	    {
	      graphActivate(bpmsp->graph);
	      graphBoxDraw(bpmsp->box, CYAN, BLACK);
	    }
	}
    }

  return ;
}

static void hidePicked (void)
{
  MSP *msp;

  for (msp = MSPlist; msp ; msp = msp->next)
    if (!strcmp(msp->sname, HighlightSeq)) {
      msp->id = msp->score;
      msp->score = -999;
    }
  blviewRedraw () ;
}


/* Callback for when user clicks on a sequence to retrieve the EMBL entry
 * for that sequence. The method of retrieving the sequence can be changed
 * via environment variables.
 *                                                                           */
static void blviewPick(int box, double x_unused, double y_unused, int modifier_unused)
{
  MSP *msp;
  Graph origGraph = graphActive();

  if (!box)
    return ;

  /* If there's already an MSP selected, see if the same one has been clicked again */
  gboolean alreadyPicked = FALSE;
  if (pickedMSP)
    {
      for (msp = MSPlist; msp ; msp = msp->next)
	{
	  if (msp == pickedMSP && msp->box == box && msp->graph == graphActive())
	    {
	      alreadyPicked = TRUE;
	      break;
	    }
	}
    }
  
  if (alreadyPicked)
    {
      /* efetch this seq */
      blxDisplayMSP(msp) ;
    }
  else
    {
      /* Reset all highlighted boxes */
      if (!squash)
	{
	  for (msp = MSPlist; msp ; msp = msp->next)
	    {
	      if (msp->box && !strcmp(msp->sname, HighlightSeq))
		{
		  graphActivate(msp->graph);
		  graphBoxDraw(msp->box, BLACK, backgColor);
		}
	    }
	}

      if (BigPictON)
	{
	  for (bpmsp = BPMSPlist.next; bpmsp && *bpmsp->sname; bpmsp = bpmsp->next)
	    {
	      if (!strcmp(bpmsp->sname, HighlightSeq))
		{
		  graphActivate(bpmsp->graph);
		  graphBoxDraw(bpmsp->box, BLACK, backgColor);
		}
	    }
	}

      /* Find clicked protein  ***********************/

      for (msp = MSPlist; msp ; msp = msp->next)
	{
	  if (msp->box == box && msp->graph == origGraph)
	    break ;
	}

      if (msp)
	{
	  /* Picked box in alignment */
	  strcpy(HighlightSeq, msp->sname) ;
	  pickedMSP = msp ;
	}
      else if (BigPictON)
        {
	  /* look for picked box in BigPicture */
	  for (bpmsp = BPMSPlist.next; bpmsp && *bpmsp->sname; bpmsp = bpmsp->next)
	    {
	      if (bpmsp->box == box && bpmsp->graph == origGraph)
		break;
	    }

	  if (bpmsp && *bpmsp->sname)
	    {
	      strcpy(HighlightSeq, bpmsp->sname);
	      pickedMSP = NULL;
	    }
	}

      if (msp || (bpmsp && *bpmsp->sname))
	{
	  char *sname ;
	  char *desc ;
	  BOOL highlight_protein ;
	  

	  /* Put description in message box and in paste buffer. */
	  if (msp)
	    {
	      sname = msp->sname ;
	      desc = msp->desc ;
	      highlight_protein = FALSE ;
	    }
	  else
	    {
	      sname = bpmsp->sname ;
	      desc = bpmsp->desc ;
	      highlight_protein = TRUE ;
	    }

	  strncpy(message, sname, 1023) ;
	  if (desc)
	    {
	      strcat(message, " ") ;
	      strncat(message, desc, 1023-strlen(message)) ;
	    }

	  graphPostBuffer(sname) ;
	  
	  /* Highlight picked box */
	  graphActivate(blixemGraph) ;
	  highlightProteinboxes(highlight_protein) ;

	  gtk_entry_set_text(GTK_ENTRY(messageWidget), message) ;
	  lastbox = box ;
	}
      
      /* gb10: this redraw is a bit of a hack. There was a bug where the text in
       * highlighted rows was being drawn the same colour as the background and was
       * therefore not visible. I fixed this in blviewRedraw but this was not being
       * called after a selection change. I'm not sure where the redrawing for a 
       * selection change should take place, so for a quick fix I've just added a 
       * full redraw in here. It's not great because it causes flicker, but it works,
       * and it actually improves some things (e.g. previously the column separator lines
       * were not being drawn properly after moving off a selection). */
      blviewRedraw() ;
      
    }

  return ;
}


static void mspcpy(MSP *dest, MSP *src)
{
    dest->type   = src->type;
    dest->score  = src->score;
    dest->id     = src->id;

    strcpy(dest->qframe, src->qframe);
    dest->qstart = src->qstart;
    dest->qend   = src->qend;

    dest->sname  = src->sname;
    strcpy(dest->sframe, src->sframe);
    dest->sstart = src->sstart;
    dest->send   = src->send;
    dest->sseq   = src->sseq;

    dest->desc   = src->desc;
    dest->box    = src->box;

    dest->color  = src->color;
    dest->shape  = src->shape;
    dest->fs     = src->fs;

    dest->xy     = src->xy;

    dest->gaps   = src->gaps;

#ifdef ACEDB
    dest->key    = src->key;
#endif
}


static void sortMSPs(int (*func)())
{
  MSP tmpmsp, *msp1, *msp2;

  if (!MSPlist)
    return;

  for (msp1 = MSPlist ; msp1->next ; msp1 = msp1->next )
    {
      for (msp2 = msp1->next ; msp2 ; msp2 = msp2->next )
	{
	  if ( (*func)(msp1, msp2)*(sortInvOn ? -1 : 1) > 0 )
	    {
	      mspcpy(&tmpmsp, msp2);
	      mspcpy(msp2, msp1);
	      mspcpy(msp1, &tmpmsp);
	    }
	}
    }

  if (graphActivate(blixemGraph))
    blviewRedraw() ;

  return ;
}


/* aghhh, this all needs rewriting, how opaque can you get sigh... */

#define FSpriority {if (FS(msp1) && !FS(msp2)) return -1; else if (FS(msp2) && !FS(msp1)) return 1;}

static int possort(MSP *msp1, MSP *msp2) {
    FSpriority
    return ( (msp1->qstart > msp2->qstart) ? 1 : -1 );
}

static int namesort(MSP *msp1, MSP *msp2) {
    FSpriority
    if (strcmp(msp1->sname, msp2->sname))
	return strcmp(msp1->sname, msp2->sname);
    else
	return possort(msp1, msp2);
}
static int scoresort(MSP *msp1, MSP *msp2) {
    FSpriority
    return ( (msp1->score < msp2->score) ? 1 : -1 );
}

static int idsort(MSP *msp1, MSP *msp2)
{
  int result = 0 ;

  FSpriority

  result = ( (msp1->id < msp2->id) ? 1 : -1 ) ;

  return result ;
}



/* Sort the match entries by..... */

static void sortByName(void)
{
  sortMode = SORTBYNAME;
  strcpy(sortModeStr, "Name");
  sortMSPs(namesort);

  return ;
}

static void sortByScore(void)
{
  sortMode = SORTBYSCORE;
  strcpy(sortModeStr, "Score");
  sortMSPs(scoresort);

  return ;
}

static void sortByPos(void)
{
  sortMode = SORTBYPOS;
  strcpy(sortModeStr, "Position");
  sortMSPs(possort);

  return ;
}

static void sortById(void)
{
  sortMode = SORTBYID;
  strcpy(sortModeStr, "Identity");
  sortMSPs(idsort);

  return ;
}


static void MSPsort(SortByType sort_mode)
{
  switch (sort_mode)
    {
    case SORTBYNAME :
      sortByName(); break;
    case SORTBYSCORE :
      sortByScore(); break;
    case SORTBYPOS :
      sortByPos(); break;
    case SORTBYID :
    default:						    /* Make the default sort by Identity */
      {
	for (msp = MSPlist; msp; msp = msp->next)
	  if (!msp->id)
	    calcID(msp);
	sortById();
      }
      break ;
    }

  return ;
}



static void sortToggleInv(void)
{
  sortInvOn = !sortInvOn;
  switch (sortMode)
    {
    case SORTBYNAME : sortByName(); break;
    case SORTBYSCORE : sortByScore(); break;
    case SORTBYPOS : sortByPos(); break;
    case SORTBYID : sortById(); break;
    default: blviewRedraw(); break;
    }

  return ;
}

/*
static void incBack(void) {
    backgColor++;
    if (!(backgColor % BLACK)) backgColor++;
    blviewRedraw();
}
static void decBack(void) {
    backgColor--;
    if (!(backgColor % BLACK)) backgColor--;
    blviewRedraw();
}
static void incGrid(void) {
    gridColor++;
    blviewRedraw();
}
static void decGrid(void) {
    gridColor--;
    blviewRedraw();
}
*/

static void squashMatches(void)
{
    static int oldSortMode;

    if (!squash) {
	oldSortMode = sortMode;
	sortByName();
	squash = 1;
    }
    else {
	switch (oldSortMode) {
	case SORTBYNAME : sortByName(); break;
	case SORTBYSCORE : sortByScore(); break;
	case SORTBYPOS : sortByPos(); break;
	case SORTBYID : sortById(); break;
	}
	squash = 0;
    }

    blviewRedraw();

    return ;
}


static void squashFSdo(void)
{
    squashFS = !squashFS;
    blviewRedraw();
}


static void blxHelp(void)
{
  graphMessage (messprintf("\
\
BLIXEM - BLast matches\n\
         In an\n\
         X-windows\n\
         Embedded\n\
         Multiple alignment\n\
\n\
LEFT MOUSE BUTTON:\n\
  Pick on boxes and sequences.\n\
  Fetch annotation by double clicking on sequence (Requires 'efetch' to be installed.)\n\
\n\
MIDDLE MOUSE BUTTON:\n\
  Scroll horizontally.\n\
\n\
RIGHT MOUSE BUTTON:\n\
  Menu.  Note that the buttons Settings and Goto have their own menus.\n\
\n\
\n\
Keyboard shortcuts:\n\
\n\
Cntl-Q          quit application\n\
Cntl-P          print\n\
Cntl-H          help\n\
\n\
\n\
m/M             for mark/unmark a set of matches from the cut buffer\n\
\n\
g/G             go to the match in the cut buffer\n\
\n\
\n\
\n\
RESIDUE COLOURS:\n\
  Yellow = Query.\n\
  See Settings Panel for matching residues (click on Settings button).\n\
\n\
version %s\n\
(c) Erik Sonnhammer", blixemVersion));

  return ;
}


static void wholePrint(void)
{
    int
	tmp,
	dispstart_save = dispstart,
	BigPictON_save = BigPictON;

    static int
	start=1, end=0;
    ACEIN zone_in;

    if (!end) end = qlen;

    /* Swap coords if necessary */
    if ((plusmin < 0 && start < end) || (plusmin > 0 && start > end )) {
	tmp = start;
	start = end;
	end = tmp;
    }

    /* Apply max limit MAXALIGNLEN */
    if ((abs(start-end)+1) > MAXALIGNLEN*symbfact) {
	start = dispstart - plusmin*MAXALIGNLEN*symbfact;
	if (start < 1) start = 1;
	if (start > qlen) start = qlen;

	end = start + plusmin*MAXALIGNLEN*symbfact;
	if (end > qlen) end = qlen;
	if (end < 1) end = 1;
    }

    if (!(zone_in = messPrompt("Please state the zone you wish to print",
			       messprintf("%d %d", start, end),
			       "iiz", 0)))
      return;

    aceInInt(zone_in, &start);
    aceInInt(zone_in, &end);
    aceInDestroy (zone_in);

    dispstart = start;
    displen = abs(end-start)+1;

    /* Validation */
    if (plusmin > 0 && start > end) {
	messout("Please give a range where from: is less than to:");
	return;
    }
    else if (plusmin < 0 && start < end) {
	messout("Please give a range where from: is more than to:");
	return;
    }
    if (displen/symbfact > MAXALIGNLEN) {
	messout("Sorry, can't print more than %d residues.  Anyway, think of the paper!", MAXALIGNLEN*symbfact);
	return;
    }

    wholePrintOn = 1;
    BigPictON = 0;
    oneGraph = 1;

    blviewRedraw();
    graphPrint();

    /* Restore */
    wholePrintOn = 0;
    dispstart = dispstart_save;
    displen = dispstart_save;
    oneGraph = 0;
    BigPictON = BigPictON_save;

    blviewRedraw();
}

static void blxPrint(void)
{
  oneGraph = 1;
  blviewRedraw();
  graphPrint();
  oneGraph = 0;

  blviewRedraw();
}


static void blxDestroy(void)
{
  gtk_widget_destroy(blixemWindow) ;

  return ;
}



/* 
 *       Set of functions to handle getting data from the clipboard following user actions.
 */

/* Called by menu/keyboard code. */
static void blxPaste(BlxPasteType paste_type)
{
  paste_type_G = paste_type ;				    /* acedb callbacks force us to have a global. */

  graphPasteBuffer(pasteCB) ;				    /* get clipboard data. */

  return ;
}


/* Called by graph asynchronously (when it gets the "select" signal) with data
 * taken from the windowing systems clipboard. */
static void pasteCB(char *text)
{
  BlixemView blxview = getBlxViewContext() ;
  BlxPasteDataStruct paste_data = {BLXPASTE_INVALID} ;

  paste_data.type = paste_type_G ;

  if (!parsePasteText(text, &paste_data))
    {
      messerror("Could not paste from clipboard, unknown format: \"%s\"", (text ? text : "no data on clipboard")) ;
    }
  else
    {
      if (paste_type_G && (paste_type_G != paste_data.type))
	{
	  messerror("Wrong type of clipboard data, expected %d but got %d", paste_type_G, paste_data.type) ;
	}
      else
	{
	  switch (paste_data.type)
	    {
	    case BLXPASTE_MATCHSET:
	      {
		if ((blxview->match_set = setMatchSet(paste_data.data.match_set)))
		  {
		    menuSetLabel(menuItem(blixemMenu, toggleMatchPasteStr), toggleMatchClearStr) ;
		  }

		blviewRedraw() ;
		break ;
	      }
	    case BLXPASTE_MOVETO:
	      {
		if (!gotoMatchPosition(paste_data.data.move_to.match,
				       paste_data.data.move_to.start,
				       paste_data.data.move_to.end))
		  messerror("Failed to find/move to match \"%s\" at %d %d",
			    paste_data.data.move_to.match,
			    paste_data.data.move_to.start, paste_data.data.move_to.end) ;
		break ;
	      }
	    default:
	      {
		printf("debug: data unrecognised !!\n") ;
		break ;
	      }
	    }
	}

      freePasteData(&paste_data) ;
    }

  paste_type_G = BLXPASTE_INVALID ;			    /* Be sure to reset. */

  return ;
}


/* Parses clipboard text looking for one of several formats. */
BOOL parsePasteText(char *text, BlxPasteData paste_data)
{
  BOOL result = FALSE ;

  if (text && *text)
    {
      switch (paste_data->type)
	{
	case BLXPASTE_MATCHSET:
	  {
	    char *match_names ;

	    /* Look for a list of match names. */
	    if ((match_names = parseMatchLines(text))
		&& (match_names = g_strstrip(match_names))
		&& (paste_data->data.match_set = g_strsplit_set(match_names, " ", -1))) /* -1 means do all tokens. */
	      result = TRUE ;

	    break ;
	  }
	case BLXPASTE_MOVETO:
	  {
	    /* Look for a match with a start/end. */
	    char *match_name ;
	    int start = 0, end = 0, length = 0 ;
	  
	    if (parseFeatureLine(text, &match_name, &start, &end, &length))
	      {
		paste_data->data.move_to.match = match_name ;
		paste_data->data.move_to.start = start ;
		paste_data->data.move_to.end = end ;
		
		result = TRUE ;
	      }
	    else
	      result = FALSE ;

	    break ;
	  }
	default:
	  {
	    /* Simply return FALSE. */
	    result = FALSE ;
	    break ;
	  }
	}
    }

  return result ;
}

void freePasteData(BlxPasteData paste_data)
{
  switch (paste_data->type)
    {
    case BLXPASTE_MATCHSET:
      {
	g_strfreev(paste_data->data.match_set) ;

	break ;
      }
    case BLXPASTE_MOVETO:
      {
	g_free(paste_data->data.move_to.match) ;

	break ;
      }
    default:
      break ;
    }

  return ;
}

/* Uses parseLine() to look for match lines in text and then
 * extracts the match names into a space separated list. */
static char *parseMatchLines(char *text)
{
  char *match_names = NULL ;
  char **matches ;

  if ((matches = g_strsplit_set(text, "\n", -1)))		    /* -1 means do all tokens. */
    {
      GString *names ;
      char *match = *matches ;
      gboolean string_free = TRUE ;

      names = g_string_sized_new(100) ;

      while (matches && match)
	{
	  char *match_name ;
	  int start = 0, end = 0, length = 0 ;

	  if (parseFeatureLine(match, &match_name, &start, &end, &length))
	    {
	      g_string_append_printf(names, " %s", match_name) ;

	      g_free(match_name) ;
	    }

	  matches++ ;
	  match = *matches ;
	}

      if (names->len)
	{
	  match_names = names->str ;
	  string_free = FALSE ;
	}

      g_string_free(names, string_free) ;
    }

  return match_names ;
}



/* Parses a line of the form:
 * 
 *                       "\"name\" start end (length)"
 *
 * Returns TRUE if all elements there, FALSE otherwise.
 * 
 * We could verify "name" more but probably not worth it. */
static BOOL parseFeatureLine(char *line,
			     char **feature_name_out,
			     int *feature_start_out, int *feature_end_out, int *feature_length_out)
{
  BOOL result = FALSE ;
  int fields ;
  char sequence_name[1000] = {'\0'} ;
  int start = 0, end = 0, length = 0 ;
  char *format_str = "\"%[^\"]\"%d%d (%d)" ;
	  
  if ((fields = sscanf(line, format_str, &sequence_name[0], &start, &end, &length)) == 4)
    {
      if (*sequence_name && (start > 0 && start < end) && length > 0)
	{
	  *feature_name_out = g_strdup(sequence_name) ;
	  *feature_start_out = start ;
	  *feature_end_out = end ;
	  *feature_length_out = length ;

	  result = TRUE ;
	}
    }

  return result ;
}





/* "Match set" is a subset of matches supplied by the user which will be highlighted
 * in the matchSetColor. Can be used to identify a subset for any number of
 * reasons, e.g. the set of matches used so far as evidence to support a transcript. */


/* Takes a list of match names and looks for those names in the msp list,
 * returns TRUE if any of the matches found. Error message reports matches
 * not found. */
static BOOL setMatchSet(char **matches_in)
{
  BOOL result = FALSE ;
  GString *not_found ;
  char **matches = matches_in ;
  char *match = *matches ;

  not_found = g_string_sized_new(100) ;

  while (matches && match)
    {
      MSP *msp ;
      BOOL found = FALSE ;

      for (msp = MSPlist; msp; msp = msp->next)
	{
	  if (g_ascii_strcasecmp(msp->sname, match) == 0)
	    {
	      result = found = msp->in_match_set = TRUE ;
	    }
	}

      if (!found)
	g_string_append_printf(not_found, " %s ", match) ;

      matches++ ;
      match = *matches ;
    }

  if (not_found->len)
    messerror("Match setting: following matches not found in blixem: %s", not_found->str) ;

  g_string_free(not_found, TRUE) ;

  return result ;
}


/* Reset to no match set. */
static void clearMatchSet(void)
{
  MSP *msp ;

  for (msp = MSPlist; msp; msp = msp->next)
    {
      if (msp->in_match_set)
	msp->in_match_set = FALSE ;
    }

  blviewRedraw() ;

  return ;
}


/* Returns the upper and lower extents of the query and subject sequence ranges in 
 * the given MSP. Any of the return values can be NULL if they are not required. */
static void getMspRangeExtents(MSP *msp, int *qSeqMin, int *qSeqMax, int *sSeqMin, int *sSeqMax)
{
  if (qSeqMin)
    *qSeqMin = min(msp->qstart, msp->qend);
  
  if (qSeqMax)
    *qSeqMax = max(msp->qstart, msp->qend);

  if (sSeqMin)
    *sSeqMin = min(msp->sstart, msp->send);
  
  if (sSeqMax)
    *sSeqMax = max(msp->sstart, msp->send);
}


/* Returns the upper and lower extents of the query and subject sequence ranges in
 * the given gap range. Any of the return values can be passed as NULL if they are not required. */
static void getSMapMapRangeExtents(SMapMap *range, int *qRangeMin, int *qRangeMax, int *sRangeMin, int *sRangeMax)
{
  if (qRangeMin)
    *qRangeMin = min(range->r1, range->r2);
  
  if (qRangeMax)
    *qRangeMax = max(range->r1, range->r2);
  
  if (sRangeMin)
    *sRangeMin = min(range->s1, range->s2);
  
  if (sRangeMax)
    *sRangeMax = max(range->s1, range->s2);
}


/* input is co-ord on query sequence, find corresonding base in subject sequence */
static int gapCoord(MSP *msp, int x, int qfact)
{
  int result = -1 ;

  BOOL qForward = ((strchr(msp->qframe, '+'))) ? TRUE : FALSE ;
  BOOL sForward = ((strchr(msp->sframe, '+'))) ? TRUE : FALSE ;
  BOOL sameDirection = (qForward == sForward);
  
  int qSeqMin, sSeqMin, sSeqMax;
  getMspRangeExtents(msp, &qSeqMin, NULL, &sSeqMin, &sSeqMax);

  BOOL rightToLeft = (qForward && plusmin < 0) || (!qForward && plusmin > 0); 

  Array gaps = msp->gaps ;
  if (!gaps || arrayMax(gaps) < 1)
    {
      /* If strands are in the same direction, find the offset from qSeqMin and add it to sSeqMin.
       * If strands are in opposite directions, find the offset from qSeqMin and subtract it from sSeqMax. */
      int offset = (x - qSeqMin)/qfact ;
      result = (sameDirection) ? sSeqMin + offset : sSeqMax - offset ;
    }
  else
    {
      /* Look to see if x lies inside one of the reference sequence ranges. */
      for (i = 0 ; i < arrayMax(gaps) ; ++i)
        {
          SMapMap *curRange = arrp(gaps, i, SMapMap) ;

	  int qRangeMin, qRangeMax, sRangeMin, sRangeMax;
	  getSMapMapRangeExtents(curRange, &qRangeMin, &qRangeMax, &sRangeMin, &sRangeMax);
          
          /* We've "found" the value if it's in or before this range. Note that the
           * the range values are in decreasing order if the q strand is reversed. */
          BOOL found = qForward ? x <= qRangeMax : x >= qRangeMin;
          
          if (found)
            {
              BOOL inRange = qForward ? x >= qRangeMin : x <= qRangeMax;
            
              if (inRange)
                {
                  /* It's inside this range. Calculate the actual index. */
                  int offset = (x - qRangeMin) / qfact;
                  result = sameDirection ? sRangeMin + offset : sRangeMax - offset;
                }
              else if (i == 0)
                {
                  /* x lies before the first range. Use the start of the first range. */
                  result = curRange->s1;
                
                  if (!rightToLeft)
                    {
                      /* This is a special case where for normal left-to-right display we want to display the
                       * gap before the base rather than after it.  Use 1 beyond the edge of the range to do this. */
                      result = sForward ? result - 1 : result + 1 ;
                    }
                }
              
              break;
            }
          else
            {
              /* Remember the end of the current range (which is the result we want if x lies after the last range). */
              result = curRange->s2;
               
              if (rightToLeft)
                {
                  /* For right-to-left display, we want to display the gap before the base rather 
                   * than after it.  Use 1 index beyond the edge of the range to do this. */
                  result = sForward ? result + 1 : result - 1 ;
                }
            }
        }
    }

  return result ;
}


static void setModeP(void)
{
  blastp = 1;
  blastx = blastn = tblastn = tblastx = 0;
  alphabetsize = 24;
  symbfact = 1;
  BigPictZoom = 10;
}

static void setModeN(void)
{
  blastn = 1;
  blastp = blastx = tblastn = tblastx = 0;
  alphabetsize = 4;
  symbfact = 1;
  BigPictZoom = 30;
}

static void setModeX(void) {
    blastx = 1;
    blastp = blastn = tblastn = tblastx = 0;
    alphabetsize = 4;
    symbfact = 3;
    BigPictZoom = 10;
}
static void setModeT(void) {
    tblastn = 1;
    blastp = blastx = blastn = tblastx = 0;
    alphabetsize = 24;
    symbfact = 1;
}
static void setModeL(void) {
    tblastx = 1;
    blastp = blastx = blastn = tblastn = 0;
    alphabetsize = 24;
    symbfact = 3;
    BigPictZoom = 10;
}



/* blxview() can be called either from other functions in the Blixem
 * program itself or directly by functions in other programs such as
 * xace.
 *
 * Interface
 *      opts:  may contain a number of options that tell blixem to
 *             start up with different defaults
 *    pfetch:  if non-NULL, then we use pfetch instead of efetch for
 *             _all_ sequence fetching (use the node/port info. in the
 *             pfetch struct to locate the pfetch server).
 *
 */
Graph blxview(char *seq, char *seqname, int start, int offset, MSP *msplist,
	      char *opts, PfetchParams *pfetch, char *align_types, BOOL External)
{
  Graph result = GRAPH_NULL ;
  static BOOL init = FALSE ;
  char *opt;
  BOOL status = TRUE ;
  DICT *dict ;


  if (!init)
    {
      initBlxView() ;
    }

  oneGraph = 0 ;
  if (blixemWindow)
    gtk_widget_destroy(blixemWindow) ;

  if (!External)
    {
      char *config_file = NULL ;
      GError *error = NULL ;
      GKeyFile *blxGetConfig(void) ;

      /* Set up program configuration. */
      if (!(blxGetConfig()) && !blxInitConfig(config_file, &error))
	{
	  messcrash("Config File Error: %s", error->message) ;
	}
    }


  q = seq;
  qlen = actend = strlen(q) ;
  qname_G = g_strdup(seqname) ;

  dispstart = start;
  actstart=1;
  qoffset = offset;
  MSPlist = msplist;
  BPMSPlist.next = 0;
  *HighlightSeq = 0;
  blastx = blastp =  blastn = tblastn = tblastx = 0 ;
  sortMode = SORTBYUNSORTED ;

  opt = opts;
  while (*opt)
    {
      /* Used options: 	 BGILMNPRSTXZ-+brsinp      */

    switch (*opt)
      {
      case 'I':
	sortInvOn = 1;                         break;
      case 'G':
	/* Gapped HSPs */
	HiliteSins = HSPgaps = 1;                 break;
      case 'P': setModeP();                       break;
      case 'N': setModeN();                       break;
      case 'X': setModeX();                       break;
      case 'T': setModeT();                       break;
      case 'L': setModeL();                       break;
      case '-':
	strcpy(actframe, "(-1)");
	plusmin = -1;                           break;
      case '+':
	strcpy(actframe, "(+1)");
	plusmin = 1;                            break;
      case 'B':
	BigPictON = 1;                          break;
      case 'b':
	BigPictON = 0;                          break;
      case 'd':
	dotter_first = 1;                       break;
      case 'i':
	sortMode = SORTBYID ;                   break;
      case 'M':
	start_nextMatch = 1;                    break;
      case 'n':
	sortMode = SORTBYNAME ;                 break;
      case 'p':
	sortMode = SORTBYPOS ;                  break;
      case 'R':
	BigPictRev = 1;                         break;
      case 'r':
	BigPictRev = 0;                         break;
      case 's':
	sortMode = SORTBYSCORE ;                break ;
      case 'Z':
	BigPictZoom = strlen(seq);              break;
      }

    opt++;
    }

  if (blastx + blastn + blastp + tblastn + tblastx == 0)
    {
      printf("\nNo blast type specified. Detected ");

      if (Seqtype(q) == 'P')
	{
	  printf("protein sequence. Will try to run Blixem in blastp mode\n");
	  setModeP();
	}
      else
	{
	  printf("nucleotide sequence. Will try to run Blixem in blastn mode\n");
	  setModeN();
	}
    }


  if (pfetch)
    {
      /* If pfetch struct then this sets fetch mode to pfetch. */

      if (blxConfigSetPFetchSocketPrefs(pfetch->net_id, pfetch->port))
	blxSetFetchMode(BLX_FETCH_PFETCH) ;
    }
  else
    {
      char *fetch_mode ;

      fetch_mode = blxFindFetchMode() ;
    }





  /* Find out if we need to fetch any sequences (they may all be contained in the input
   * files), if we do need to, then fetch them by the appropriate method. */
  dict = dictCreate(128) ;
  if (!haveAllSequences(MSPlist, dict))
    {
      if (strcmp(blxGetFetchMode(), BLX_FETCH_PFETCH) == 0)
	{
	  /* Fill msp->sseq fast by pfetch if possible
	   * two ways to use this:
	   *    1) call blixem main with "-P node:port" commandline option
	   *    2) setenv BLIXEM_PFETCH to a dotted quad IP address for the
	   *       pfetch server, e.g. "193.62.206.200" = Plato's IP address
	   *       or to the name of the machine, e.g. "plato"
	   *       and optionally setenv BLIXEM_PORT to the port number for
	   *       the pfetch server. */
	  enum {PFETCH_PORT = 22100} ;			    /* default port to connect on */
	  char *net_id = NULL ;
	  int port = PFETCH_PORT ;

	  if (pfetch)
	    {
	      net_id = pfetch->net_id ;
	      if (!(port = pfetch->port))
		port = PFETCH_PORT ;
	    }
	  else if ((net_id = getenv("BLIXEM_PFETCH")))
	    {
	      char *port_str ;

	      port = 0 ;
	      if ((port_str = getenv("BLIXEM_PORT")))
		port = atoi(port_str) ;

	      if (port <= 0)
		port = PFETCH_PORT ;
	    }
	  else
	    {
	      /* Lastly try for a config file. */

	      blxConfigGetPFetchSocketPrefs(&net_id, &port) ;
	    }

	  if (net_id)
	    status = blxGetSseqsPfetch(MSPlist, dict, net_id, port, External) ;
	}
#ifdef PFETCH_HTML 
      else if (strcmp(blxGetFetchMode(), BLX_FETCH_PFETCH_HTML) == 0)
	{
	  BlxSeqType seqType = blastx || tblastx ? BLXSEQ_PEPTIDE : BLXSEQ_DNA ;
	  status = blxGetSseqsPfetchHtml(MSPlist, dict, seqType) ;
	}
#endif
    }
  messfree(dict) ;

  /* Note that we create a blxview even if MSPlist is empty.
   * But only if it's an internal call.  If external & anything's wrong, we die. */
  if (status || !External)
    result = blviewCreate(opts, align_types) ;

  /* Sort the MSPs according to mode chosen. */
  MSPsort(sortMode) ;


#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
  printMSPs() ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */


  return result ;
}




/* BLVIEWCREATE initializes the display and the buttons
*/
static void addgraph(GtkWidget *vbox, BOOL doFrame, Graph graph)
{
  GtkWidget *widget;

  if (doFrame)
    {
      widget = gtk_frame_new(NULL);
      gtk_container_add(GTK_CONTAINER(widget), gexGraph2Widget(graph));
    }
  else
    widget = gexGraph2Widget(graph);

  gtk_container_border_width (GTK_CONTAINER (widget), 0);

  gtk_box_pack_start(GTK_BOX(vbox),
		     widget,
		     !doFrame, TRUE, 0);

  graphActivate(graph);
  graphRegister (PICK, blviewPick) ;
  graphRegister (MIDDLE_DOWN, MiddleDownQ) ;
  graphRegister (KEYBOARD, keyboard) ;
  graphNewMenu(blixemMenu);

  return ;
}


/* Return true if the current user is in our list of developers. */
static BOOL userIsDeveloper()
{
  gchar* developers[] = {"edgrif", "gb10"};

  BOOL result = FALSE;
  const gchar *user = g_get_user_name();
  int numDevelopers = sizeof(developers) / sizeof(gchar*);
  
  int i = 0;
  for (i = 0; i < numDevelopers; ++i)
    {
      if (strcmp(user, developers[i]) == 0)
        {
          result = TRUE;
          break;
        }
    }

  return result;
}


static Graph blviewCreate(char *opts, char *align_types)
{

  if (!blixemWindow)
    {
      float w, h;
      GtkWidget *vbox;
      GtkWidget *button_bar ;
      int i, frames = blastx ? 3 : 2 ;
      BOOL pep_nuc_align ;

      blixemWindow = gexWindowCreate();
      vbox = gtk_vbox_new(FALSE, 0);
      gtk_container_add(GTK_CONTAINER(blixemWindow), vbox);

      if (!oldWidth)
	gtk_window_set_default_size(GTK_WINDOW(blixemWindow),
				    (int)(((float)gdk_screen_width())*0.9),
				    (int)(((float)gdk_screen_height())*0.6));
      else
	gtk_window_set_default_size(GTK_WINDOW(blixemWindow),
				    oldWidth, oldHeight);


      pep_nuc_align = (*opts == 'X' || *opts == 'N') ;
      gtk_window_set_title(GTK_WINDOW(blixemWindow),
			   messprintf("Blixem %s%s%s:   %s",
				      (pep_nuc_align ? "  (" : ""),
				      (align_types ? align_types : (*opts == 'X' ? "peptide" :
								    (*opts == 'N' ? "nucleotide" : ""))),
				      (pep_nuc_align ? " alignment)" : ""),
				      qname_G));


      w=3.0;
      h=3.0;

      blixemMenu = menuInitialise ("blixem", userIsDeveloper() ? (MENUSPEC*)developerMainMenu : (MENUSPEC*)mainMenu) ;
      menuSetFlags(menuItem(blixemMenu, autoDotterParamsStr),
		   MENUFLAG_DISABLED);

      if (!fsArr)
	menuSetFlags(menuItem(blixemMenu, selectFeaturesStr ),
		     MENUFLAG_DISABLED);

      blixemGraph = graphNakedResizeCreate(TEXT_FIT, NULL, w, h, TRUE);
      addgraph(vbox, TRUE, blixemGraph);
      graphRegister (MIDDLE_DOWN, MiddleDownBP) ;
      graphRegister (RESIZE, blviewRedraw);

      button_bar = makeButtonBar() ;
      gtk_box_pack_start(GTK_BOX(vbox), button_bar, FALSE, TRUE, 0);

      for (i=0; i<frames; i++)
	{
	  frameGraphQ[i] = graphNakedResizeCreate(TEXT_FIT, 0, w, h, FALSE);
	  addgraph(vbox, TRUE, frameGraphQ[i]);

	  frameGraph[i] = graphNakedResizeCreate(TEXT_SCROLL, 0, w, h, FALSE);
	  addgraph(vbox, FALSE, frameGraph[i]);
	}

      gtk_widget_realize(blixemWindow);
      gtk_widget_realize(vbox);
      gtk_widget_show_all(blixemWindow);

      gexDestroyRegister(blixemWindow, blviewDestroy);

      settingsMenu = menuInitialise ("Settings", (MENUSPEC*)settingsMenuOpt) ;
      if (blastp || tblastn)
	menuSetFlags(menuItem(settingsMenu, BigPictToggleRevStr), MENUFLAG_HIDE);
    }


  if ((cp = (char *)strrchr(qname_G, '/')))
    qname_G = cp+1;

  allocAuxseqs(INITDBSEQLEN);

  if (entropyOn)
    calcEntropyArrays(FALSE);

#ifdef ACEDB
  /* Acedb passes revcomp exons incorrectly for blastn * /
     if (blastn) {
     messout ( "\nWarning: exons on the opposite strand may be invisible due "
     "to a strange bug in blastn mode in acedb which inverts them, "

     "although the identical routine works correctly in blastx mode.\n"
     "Also, when Blixem is called from REVCOMP blastn mode, the HSPs "
     "are not revcomped.\n");
     }*/
#endif

  if (dotter_first && MSPlist && MSPlist->sname
      && (MSPlist->type == HSP || MSPlist->type == GSP))
    {
      strcpy(HighlightSeq, MSPlist->sname);
      callDotter();
    }

  if (start_nextMatch)
    nextMatch();
  else
    blviewRedraw();

  return blixemGraph;
}


/* BLVIEWDESTROY frees all the allocated memory
   N.B. This memory was allocated in the calling program (acedb)

   WHAT THIS ROUTINE DOES NOT ADDRESS IS RESETTING THE LARGE NUMBER OF GLOBALS
   IN ANY SENSIBLE WAY......NOT IDEAL TO HAVE GLOBALS, EVEN LESS TO TO NOT RESET
   THEM....GRRRRRRRRRRRRRR............


   Could free auxseq, auxseq2 and padseq too, but they'd have to be remalloc'ed
   next time then.
*/
static void blviewDestroy(GtkWidget *unused)
{
  MSP *msp, *fmsp;
  BPMSP *tmsp;

  g_free(qname_G) ;

  messfree(q);

  /* Free the allocated sequences and names */
  for (msp = MSPlist; msp; msp = msp->next)
    {
      if (msp->sseq && msp->sseq != padseq)
	{
	  for (fmsp = msp->next; fmsp; fmsp = fmsp->next)
	    {
	      if (fmsp->sseq == msp->sseq)
		fmsp->sseq = 0;
	    }

	/* Bug in fmapfeatures.c causes introns to have stale sseq's */
	if (msp->score >= 0)
	  {
	    messfree(msp->sseq);
	    messfree(msp->qname);
	    messfree(msp->sname);
	    messfree(msp->desc);
	    arrayDestroy(msp->gaps);
	    arrayDestroy(msp->xy);
	  }
	}
    }

  for (msp = MSPlist; msp; )
    {
      fmsp = msp;
      msp = msp->next;
      messfree(fmsp);
    }

  for (bpmsp = BPMSPlist.next; bpmsp; )
    {
      tmsp = bpmsp;
      bpmsp = bpmsp->next;
      messfree(tmsp);
    }

  arrayDestroy(stringentEntropyarr);
  arrayDestroy(mediumEntropyarr);
  arrayDestroy(nonglobularEntropyarr);

  blixemWindow = NULL ;
  pickedMSP = NULL ;


  /* Need to start trying to reset some state.... */
  dotterStart = dotterEnd = 0 ;


  return ;
}


void drawBigPictMSP(MSP *msp, int BPx, char strand)
{
  float  msp_y, msp_sx, msp_ex, midx;

  if (FS(msp)) return;

  msp_sx = max((float)plusmin*(msp->qstart-BigPictStart)*BPx/BigPictLen +BPoffset, 4);
  msp_ex = max((float)plusmin*(msp->qend-BigPictStart)*BPx/BigPictLen +BPoffset, 4);

  if (msp->score == -1) /* EXON */
    {
      oldcolor = graphColor(geneColor);
      oldLinew = graphLinewidth(.15);

      msp_y = 7.9 + queryy ;
      if (strand == 'R')
	msp_y += 1.5 ;

      graphRectangle(msp_sx, msp_y, msp_ex, msp_y + 0.7);
      graphColor(oldcolor);
      graphLinewidth(oldLinew);
    }
  else if (msp->score == -2) /* INTRON */
    {
      oldcolor = graphColor(geneColor);
      oldLinew = graphLinewidth(.15);

      msp_y = 7.9 + queryy ;
      if (strand == 'R')
	msp_y += 1.5 ;

      midx = 0.5 * (msp_sx + msp_ex) ;
      graphLine (msp_sx, msp_y+0.4, midx, msp_y) ;
      graphLine (msp_ex, msp_y+0.4, midx, msp_y) ;
      graphColor(oldcolor);
      graphLinewidth(oldLinew);
    }
  else if (msp->score > 0) /* BLAST MATCH */
    {
      int colour ;

      if (!msp->sseq)
	getsseq(msp);
      if (!msp->id)
	calcID(msp);

      msp_y = (float)(140 - msp->id)/20 + queryy ;

      if (strand == 'R')
	msp_y += 9 ;

      if (!bpmsp->next)
	bpmsp->next = (BPMSP *)messalloc(sizeof(BPMSP));

      bpmsp = bpmsp->next;
      strcpy(bpmsp->sname, msp->sname);
      bpmsp->desc = msp->desc;

      oldLinew = graphLinewidth(0.1);
      bpmsp->box = graphBoxStart();
      bpmsp->graph = graphActive();

      graphFillRectangle(msp_sx, msp_y, msp_ex, msp_y+.2);

      /* graphLine doesn't want to be picked */

      graphBoxEnd();

      if (msp->in_match_set)
	colour = matchSetColor ;
      else
	colour = BLACK ;
      
      graphBoxDraw(bpmsp->box, colour, TRANSPARENT);
      graphLinewidth(oldLinew);
    }

  return ;
}


/* Function: return true if pos is a value in between or equal to start and end
 */
static int inRange(pos, start, end)
{
    if (start < end) {
        if (pos >= start && pos <= end) return 1;
	else return 0;
    }
    else {
        if (pos >= end && pos <= start) return 1;
	else return 0;
    }
}


/* Checks if the msp is supposed to be drawn given its frame and position */
static void selectBigPictMSP(MSP *msp, int BPx, int BigPictStart, int BigPictLen)
{
    int BPend = (actframe[1] == '+' ? BigPictStart+BigPictLen : BigPictStart-BigPictLen);

    /* Check if MSP is in range.  There are three cases:
       1. MSP spans beginning of BP range
       2. MSP spans end of BP range
       3. MSP spans entire BP range (i.e. BigPictStart is within the MSP-range)
       (4. MSP is included in BP range, falls into 1. and 2.)
    */
    if (!inRange(msp->qstart, BigPictStart, BPend) &&
	!inRange(msp->qend, BigPictStart, BPend) &&
	!inRange(BigPictStart, msp->qstart, msp->qend))
        return;


    if (actframe[1] == msp->qframe[1])
	drawBigPictMSP(msp, BPx, 'M');

    if (BigPictRev)
	if (actframe[1] != msp->qframe[1])
	    drawBigPictMSP(msp, BPx, 'R');

    return;

#ifdef OLD_CODE_TOO_COMPLICATED_AND_BUGGED__LEFT_FOR_AMUSEMENT
    if (( ((actframe[1] == '+' && msp->frame[1] == '+')) &&
	  (msp->qstart <  BigPictStart && msp->qend >  BigPictStart || /* left || span */
	   msp->qstart >= BigPictStart && msp->qend <= BigPictStart+BigPictLen || /* middle */
	   msp->qstart <  BigPictStart+BigPictLen && msp->qend > BigPictStart+BigPictLen)) /* right || span */
	||
	(actframe[1] == '-' && msp->frame[1] == '-' &&
	 (msp->qend > BigPictStart && msp->qstart < BigPictStart ||
	  msp->qend <= BigPictStart && msp->qstart >= BigPictStart-BigPictLen ||
	  msp->qend > BigPictStart-BigPictLen && msp->qstart < BigPictStart-BigPictLen)))
	drawBigPictMSP(msp, BPx, 'M');
    if (BigPictRev)
	if ( (strchr(actframe, '+') && strchr(msp->frame, '-') &&
	      (msp->qend <  BigPictStart && msp->qstart < BigPictStart+BigPictLen ||
	       msp->qend >= BigPictStart && msp->qstart <= BigPictStart+BigPictLen ||
	       msp->qend <  BigPictStart+BigPictLen && msp->qstart > BigPictStart+BigPictLen))
	     ||
	     (strchr(actframe, '-') && strchr(msp->frame, '+') &&
	      (msp->qend >  BigPictStart && msp->qstart < BigPictStart ||
	       msp->qend <= BigPictStart && msp->qstart >= BigPictStart-BigPictLen ||
	       msp->qend > BigPictStart-BigPictLen && msp->qstart < BigPictStart-BigPictLen)))
	    drawBigPictMSP(msp, BPx, 'R');
#endif
}


/* Function: Put feature segment on the screen

   Note: this routine does not support reversed mode.  Worry about that later.
*/
static void drawSEG(MSP *msp, float offset)
{
    float
	msp_sy,
	msp_ey = queryy + offset-1,
	msp_sx,
	msp_ex;

    if (msp->qstart > BigPictStart+BigPictLen-1 ||
	msp->qend < BigPictStart)
        return;

    msp_sx = max(SEQ2BP(msp->qstart), 4);
    msp_ex = max(SEQ2BP(msp->qend+1), 4);

    msp_sy = msp_ey - (float)msp->score/100;

    oldcolor = graphColor(msp->color); oldLinew = graphLinewidth(.1);

    graphFillRectangle(msp_sx, msp_sy, msp_ex, msp_ey);
    graphColor(BLACK);
    graphRectangle(msp_sx, msp_sy, msp_ex, msp_ey);
    graphText(msp->desc, msp_sx, msp_ey);
    graphColor(oldcolor); graphLinewidth(oldLinew);
}


/* Function: put XY curves on the screen.

   Note: this routine does not support reversed mode.  Worry about that later.
*/
static void drawSEGxy(MSP *msp, float offset)
{
    int i, inNotFilled=0, descShown=0;
    float
	msp_y = queryy + offset-1,
	x, y, xold=0, yold=0;

    oldcolor = graphColor(msp->color); oldLinew = graphLinewidth(.25);

    /* Must go through interpolated data outside the visible area in case the interpolation
       spans the start or the end of the visible area */
    if (msp->shape == XY_INTERPOLATE) {
        for (i = 0; i < BigPictStart; i++) {
	    if (arr(msp->xy, i, int) != XY_NOT_FILLED) {
	      xold = SEQ2BP(i);
	      yold = msp_y - (float)arr(msp->xy, i, int)/100*fsPlotHeight;
	    }
	}
    }

    for (i = BigPictStart; i < BigPictStart+BigPictLen-1; i++) {
	if (arr(msp->xy, i, int) == XY_NOT_FILLED) {
	    inNotFilled = 1;
	}
	else {
	    x = SEQ2BP(i);
	    y = msp_y - (float)arr(msp->xy, i, int)/100*fsPlotHeight;
	    if (xold && (!inNotFilled || msp->shape == XY_INTERPOLATE)) {
	        if (x != xold || y != yold) graphLine(xold, yold, x, y);
		if (!descShown && msp->desc) {
		      int linecolor = graphColor(BLACK);
		      graphText(msp->desc, (xold > BPoffset ? xold : BPoffset), msp_y);
		      graphColor(linecolor);
		      descShown = 1;
		  }
	    }
	    xold = x;
	    yold = y;
	    inNotFilled = 0;
	}
    }

    /* Draw interpolated data if it spans the end of the visible area */
    if (msp->shape == XY_INTERPOLATE && xold) {
        for (; i < qlen; i++) {
	    if (arr(msp->xy, i, int) != XY_NOT_FILLED) {
	        x = SEQ2BP(i);
		y = msp_y - (float)arr(msp->xy, i, int)/100*fsPlotHeight;
		graphLine(xold, yold, x, y);
		break;
	    }
	}
    }

    graphColor(oldcolor); graphLinewidth(oldLinew);
}


static void drawEntropycurve(int start, int end, Array array, int win)
{
    int i;
    float x, y, xold=0, yold=0;

    oldLinew = graphLinewidth(.3);

    for (i = start; i < end; i++) {
	if (i > win/2 && i < qlen - win/2) {
	    x = SEQ2BP(i);
	    y = queryy + 9 - arr(array, i, float)*2;
	    if (xold) graphLine(xold, yold, x, y);
	    xold = x;
	    yold = y;

	    /*if (arr(array, i, float) < min) min = arr(array, i, float);
	    if (arr(array, i, float) > max) max = arr(array, i, float);*/
	}
	/**c = q[i];
	  graphText(c, SEQ2BP(i), queryy+5);*/
    }
    /* printf("min = %f , max = %f\n", min, max); */

    graphLinewidth(oldLinew);
}


/* Draw separator line a la Mosaic <HR> */
static void drawSeparator(void) {
    oldLinew = graphLinewidth(separatorwidth);
    if (squash)
	graphColor(RED);
    else
	graphColor(DARKGRAY);
    graphLine(0, queryy-1.0, nx+1, queryy-1.0);

    graphLinewidth(.2);
    graphColor(WHITE);
    graphLine(0, queryy-1.1+0.5*separatorwidth, nx+1, queryy-1.1+0.5*separatorwidth);
    graphColor(BLACK);
    graphLinewidth(oldLinew);
}


int frame2graphno(int frame)
{
  if (blastn)
    {
      if (frame == 1)
	return 0;
      else
	return 1;
    }
  else
    {
      if (abs(frame) == 1)
	return 0;
      else if (abs(frame) == 2)
	return 1;
      else
	return 2;
    }
}


/* Draw the given sequence name at the given coordinates. The name is formatted so that
 * it includes the direction of the strand and does not exceed the given length. */
void drawStrandName(char *name, char *frame, int max_len, int x, int y)
{
  gchar *displayName = g_strconcat(abbrevTxt(name, max_len - 2), 
                                  (strchr(frame, '+') ? " +" : " -"),
                                   NULL);
  graphText(displayName, x, y);
  g_free(displayName);
}


/* blViewRedraw Redraws the view in the disp_region */
void blviewRedraw(void)
{
  unsigned char querysym, subjectsym ;
  int     i,
    dbdisp,  /* number of msp's displayed */
    frame=0,
    curframe, DNAline,
    TickStart, TickUnit, gridx,
    pos, exon_end ;
  MSP    *msp;
  float   alignstart;
  int charHeight;
  float fy;
  int py;
  char    *query = NULL, *lastname = NULL ;
  char    text[MAXALIGNLEN+1] ;
  BOOL compN ;


  gdk_window_get_geometry(blixemWindow->window, NULL, NULL,
			  &oldWidth, &oldHeight, NULL);
  settingsRedraw();

  graphActivate(blixemGraph);
  graphClear();
  graphBoxDraw(0, backgColor, backgColor);
  graphTextFormat(FIXED_WIDTH);
  graphScreenSize( NULL, NULL, NULL, &fy, NULL, &py);

  charHeight = (int)((float)py/fy) + 1 ;

  for (msp = MSPlist; msp ; msp = msp->next)
    {
      msp->box = 0 ;
    }

  queryy = separator_y = 0.2;


  /* Calculate window sizes **************************/

  graphFitBounds (&nx, &ny);
  nx -= 2; /* scrollbars */

  graphColor(backgColor);
  graphRectangle(0, 0, nx+100, ny+100);
  graphColor(BLACK);

  if (wholePrintOn)
    {
      nx = displen/symbfact + 43;
    }
  else
    {
      displen = (nx - 43)*symbfact ;        /* Keep spacious for printing */
    }



  /* Window boundary check */
  if (strchr(actframe, '+'))
    {
      if (displen > qlen - (symbfact+1) )                 /* Window too big for sequence */
	{
	  dispstart = 1;
	  displen = qlen - (symbfact+1);
	  if (blastp || blastn) displen = qlen;
	}
      else if (dispstart < 1)                             /* Off the left edge  */
	dispstart = 1 ;
      else if (dispstart + displen > qlen - (blastx||tblastx ? 3 : -1))     /* Off the right edge */
	dispstart = qlen - (blastx||tblastx ? 3 : -1) - displen;
    }
  else if (strchr(actframe, '-'))
    {
      if (displen > qlen - (symbfact+1))                  /* Window too big for sequence */
	{
	  dispstart = qlen;
	  displen = qlen - (symbfact+1);
	  if (blastn) displen = qlen;
	}
      else if (dispstart > qlen)                          /* Off the left edge  */
	dispstart = qlen ;
      else if (dispstart - displen < (blastx||tblastx ? 4 : 0) )   /* Off the right edge */
	dispstart =  (blastx||tblastx ? 4 : 0) + displen ;
    }


  /* Draw Big Picture **************************/

  if (BigPictON)
    {
      BigPictLen = displen * BigPictZoom;

      if (BigPictLen > qlen)
	{
	  BigPictLen = qlen;
	  BigPictZoom = (float)qlen/displen;
	}

      BigPictStart = dispstart + plusmin*displen/2 - plusmin*BigPictLen/2;

      if (plusmin > 0)
	{
	  if (BigPictStart + BigPictLen > qlen)
	    BigPictStart = qlen - BigPictLen;
	  if (BigPictStart < 1)
	    BigPictStart = 1;
	}
      else
	{
	  if (BigPictStart - BigPictLen < 1)
	    BigPictStart = BigPictLen;
	  if (BigPictStart > qlen)
	    BigPictStart = qlen;
	}

      BPx = nx - BPoffset;
      graphButton("Zoom In", zoomIn, .5, queryy +.1);
      graphButton("Zoom Out", zoomOut, 9, queryy +.1);
      graphButton("Whole", zoomWhole, 18.5, queryy +.1);

      /* Draw position lines (ticks) */
      TickUnit = (int)pow(10.0, floor(log10((float)BigPictLen)));
      /* Basic scaleunit */
      while (TickUnit*5 > BigPictLen) TickUnit /= 2;	    /* Final scaleunit */
      TickStart = (int)TickUnit*( floor((float)(BigPictStart+qoffset)/TickUnit + .5));
      /* Round off       */
      for (i=TickStart;
	   (plusmin == 1 ? i < BigPictStart+qoffset + BigPictLen : i > BigPictStart+qoffset - BigPictLen);
	   i = i + plusmin*TickUnit)
	{
	  gridx = (float)plusmin*(i-BigPictStart-qoffset)*BPx/BigPictLen + BPoffset;
	  if (gridx > 24 && gridx < nx-10)
	    {
	      graphText(messprintf("%d", i), gridx, queryy);
	      graphColor(gridColor);
	      graphLine(gridx, queryy+1, gridx, queryy+7 /*+fsTotalHeight(MSPlist)+2*/ );
	      if (BigPictRev)
		graphLine(gridx, queryy+11, gridx, queryy+16);
	      graphColor(BLACK);
	    }
	}

      /* Draw Percentage lines */
      for (i=0; i<=100; i += 20) {
	graphText(messprintf("%3i%%", i), 0, queryy + (float)(130-i)/20);
	if (BigPictRev) graphText(messprintf("%3i%%", i), 0, queryy+9 + (float)(130-i)/20);
	graphColor(gridColor);
	graphLine(4, queryy + (float)(140-i)/20, nx+1, queryy + (float)(140-i)/20);
	if (BigPictRev) graphLine(4, queryy+9 + (float)(140-i)/20, nx+1, queryy+9 + (float)(140-i)/20);
	graphColor(BLACK);
      }


      /* Draw active window frame */
      oldLinew = graphLinewidth(.3);
      BPboxstart = (float)plusmin*(dispstart-BigPictStart)*BPx/BigPictLen + BPoffset;
      BPboxwidth = (float)displen*BPx/BigPictLen;
      BPbox = graphBoxStart();
      graphRectangle(BPboxstart, queryy +1.7, BPboxstart+BPboxwidth, queryy +1.7 + BPboxheight);
      graphBoxEnd(); graphBoxDraw(BPbox, geneColor, TRANSPARENT);
      graphLinewidth(.5);
      graphLinewidth(oldLinew);


      /* Draw Big Picture MSPs */
      lastExonx = 0;
      bpmsp = &BPMSPlist;
      for (msp = MSPlist; msp; msp = msp->next)
        {
	  if (strcmp(msp->sname, HighlightSeq) && !strMatch(msp->sname, searchSeq))
	    selectBigPictMSP(msp, BPx, BigPictStart, BigPictStart+BigPictLen);
	}

      /* Draw the highlighted later, so they're not blocked by black ones */
      for (msp = MSPlist; msp; msp = msp->next)
	{
	  if (!strcmp(msp->sname, HighlightSeq) || strMatch(msp->sname, searchSeq))
	    selectBigPictMSP(msp, BPx, BigPictStart, BigPictStart+BigPictLen);
	}
      if (bpmsp->next) *bpmsp->next->sname = 0;

      queryy += 10;
      if (BigPictRev) queryy += 8;

      /* Draw separator line with perforation * /
	 oldLinew = graphLinewidth(1);
	 if (squash) graphColor(RED);
	 else graphColor(BLACK);
	 graphLine(0, queryy-.5, nx+1, queryy-.5);
	 graphLinewidth(.1);
	 graphColor(WHITE);
	 for (i = 0; i <= nx+1; i++) graphLine((float)i, queryy-.5, (float)i+.8, queryy-.5);
	 graphColor(BLACK);
	 graphLinewidth(oldLinew); */


      /* Draw entropy curves **************************/
      if (entropyOn)
	{
	  int y;

	  drawSeparator();

	  /* Draw scale */
	  for (i = 1; i <= 4; i++)
	    {
	      y = queryy+5 - (i-2)*2;
	      graphText(messprintf("%d", i), 1, y-.5);

	      graphColor(DARKGRAY);
	      graphLine (BPoffset, y, nx, y);
	      graphColor(BLACK);
	    }

	  /* Draw entropy curves */
	  graphColor(stringentEntropycolor);
	  drawEntropycurve(BigPictStart, BigPictStart+BigPictLen,
			   stringentEntropyarr, stringentEntropywin);

	  graphColor(mediumEntropycolor);
	  drawEntropycurve(BigPictStart, BigPictStart+BigPictLen,
			   mediumEntropyarr, mediumEntropywin);

	  graphColor(nonglobularEntropycolor);
	  drawEntropycurve(BigPictStart, BigPictStart+BigPictLen,
			   nonglobularEntropyarr, nonglobularEntropywin);

	  queryy += 9;
	}


      /* Draw colored feature segments ********************************/
      if (fsArr && arrayMax(fsArr))
	{
	  float maxy=0 ;

	  /* drawSeparator();*/

	  if (fsArr)
	    {
	      for (i = 0; i < arrayMax(fsArr); i++)
		arrp(fsArr, i, FEATURESERIES)->y = 0;
	    }


	  for (msp = MSPlist; msp; msp = msp->next)
	    {
	      if (FS(msp) && arrp(fsArr, msp->fs, FEATURESERIES)->on &&
		  (!strcmp(msp->qname, qname_G) || !strcmp(msp->qname, "@1")))
		{
		  if (msp->type == FSSEG)
		    {
		      drawSEG(msp, fs2y(msp, &maxy, 1+1));
		    }
		  else if (msp->type == XY)
		    {
		      drawSEGxy(msp, fs2y(msp, &maxy, fsPlotHeight+1));
		    }
		}
	    }

	  queryy += maxy + 2;
	}

      separator_y = queryy;
    }



  /* Draw buttons & boxes **************************/

#if 0
  if (BigPictON) {
    menuUnsetFlags(menuItem(settingsMenu, BigPictToggleRevStr), MENUFLAG_DISABLED);
  }
  else if (blastn || blastx || tblastx) {
    menuSetFlags(menuItem(settingsMenu, BigPictToggleRevStr), MENUFLAG_DISABLED);
  }
#endif

  if (!oneGraph)
    gtk_widget_set_usize(gexGraph2Widget(blixemGraph), -2, charHeight*queryy) ;

  for (curframe = 1; abs(curframe) <= symbfact; curframe += (blastn ? -2 : 1))
    {
      int frameno;

      frame = plusmin * curframe;
      sprintf(actframe, "(%+d)", frame);

      if (blastn && curframe == -1)
	compN = TRUE ;
      else
	compN = FALSE ;

      alignstart = NAMESIZE + 22 + (blastx || tblastx ? (float)(curframe - 1)/3 : 0);

      frameno = frame2graphno(curframe);

      if (!oneGraph)
	{
	  graphActivate(frameGraphQ[frameno]);
	  queryy = 0; /* at start of new graph */
	  graphClear();
	  graphBoxDraw(0, backgColor, backgColor);
	  graphTextFormat(FIXED_WIDTH);
	  graphColor(backgColor);
	  graphRectangle(0, 0, nx+100, ny+100);
	  graphColor(BLACK);
	}

      if (frameno == 0)
	{
	  if (!blastn)
	    {
	      separator_y = queryy;
	      queryy += 3;
	      DNAline = queryy-2;
	    }
	  else
	    {
	      queryy += 1;
	      DNAline = queryy;
	    }
	  graphText( "Score %ID  Start", 14, queryy);
	  graphText( "End", NAMESIZE + 23 + displen/symbfact, queryy);
	  queryy++;
	  graphLine(NAMESIZE +  1.0, queryy-1, NAMESIZE +  1.0, ny+100);
	  graphLine(NAMESIZE +  7.5, queryy-1, NAMESIZE +  7.5, ny+100);
	  graphLine(NAMESIZE + 11.5, queryy-1, NAMESIZE + 11.5, ny+100);
	  graphLine(NAMESIZE + 21.5, queryy-1, NAMESIZE + 21.5, ny+100);
	  graphLine(NAMESIZE + 22.5 + displen/symbfact, queryy-1,
		    NAMESIZE + 22.5 + displen/symbfact, ny+100);
	  graphLine(NAMESIZE + 22.5 + displen/symbfact/2, DNAline-1,
		    NAMESIZE + 22.5 + displen/symbfact/2, DNAline);

	}


      /* Draw the query (ususally black text on yellow) ********************/

      if (blastx || tblastx)
	{
	  /* Force dispstart to cohere to the frame */
	  if (strchr(actframe, '-'))
	    while ((qlen - dispstart +1 +frame) % symbfact)
	      dispstart--;
	  else
	    while ((dispstart - frame) % symbfact)
	      dispstart++;
	}

      if (!(query = getqseq(dispstart, dispstart + plusmin*(displen-1), q)))
	return;

      if (compN)
	blxComplement(query);

      if (!colortoggle)
	graphColor(hiColor);
      else
	graphColor(WHITE);
      graphFillRectangle(0, queryy, nx, queryy+1);

      graphColor(BLACK);
      graphText(query, alignstart, queryy);

      graphText(messprintf ("%s%s", abbrevTxt(qname_G, NAMESPACE+4), actframe),  1, queryy);

      graphText(messprintf ("%9d", dispstart + qoffset),  NAMESPACE+12, queryy);
      sprintf (text, "%-9d", dispstart+plusmin*displen + qoffset -plusmin);
      graphText (text,  alignstart + 1 + displen/symbfact, queryy);

      if (blastx || tblastx)
	{
	  /* Draw the DNA sequence */
	  if (!oneGraph)
	    graphActivate(frameGraphQ[0]);

	  graphText(get3rd_base(dispstart, dispstart + plusmin*(displen-1), q),
		    alignstart, DNAline++);

	  if (!oneGraph)
	    graphActivate(frameGraphQ[frameno]);
	}

      if (!oneGraph)
	{
	  gtk_widget_set_usize(gexGraph2Widget(frameGraphQ[frameno]),
			       -2, charHeight*(queryy+1));

	  graphRedraw();

	  frameno = frame2graphno(curframe);
	  graphActivate(frameGraph[frameno]);

	  queryy = 0; /* at start of new graph */
	  graphClear();

	  graphBoxDraw(0, backgColor, backgColor);
	  graphTextFormat(FIXED_WIDTH);
	  graphColor(backgColor);
	  graphRectangle(0, 0, nx+100, ny+100);
	  graphColor(BLACK);
	  graphLine(NAMESIZE +  1.0, 0, NAMESIZE +  1.0, ny+100);
	  graphLine(NAMESIZE +  7.5, 0, NAMESIZE +  7.5, ny+100);
	  graphLine(NAMESIZE + 11.5, 0, NAMESIZE + 11.5, ny+100);
	  graphLine(NAMESIZE + 21.5, 0, NAMESIZE + 21.5, ny+100);
	  graphLine(NAMESIZE + 22.5 + displen/symbfact, 0,
		    NAMESIZE + 22.5 + displen/symbfact, ny+100);
	}


      /* Draw the dbhits *********************/

      dbdisp = oneGraph ? 0 : -1;
      fetchCount = 0;
      for (msp = MSPlist ; msp ; msp = msp->next)
        {

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
	  if (g_ascii_strcasecmp(msp->sname, "Em:BF956876.1") == 0)
	    printf("found %s\n", "Em:BF956876.1") ;
	  else if (g_ascii_strcasecmp(msp->sname, "Em:BX479315.1") == 0)
	    printf("found %s\n", "Em:BX479315.1") ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */


	  /* Current frame MSP - display if in window */
	  if (!strcmp(msp->qframe, actframe) || (blastn && msp->qframe[1] == actframe[1]))
	    {
	      if ((!compN && frame>0 && msp->qstart <= dispstart + displen && msp->qend >= dispstart) ||
		  (!compN && frame<0 && msp->qstart >= dispstart - displen && msp->qend <= dispstart) ||
		  ( compN && frame<0 && msp->qend   <= dispstart + displen && msp->qstart >= dispstart) ||
		  ( compN && frame>0 && msp->qend   >= dispstart - displen && msp->qstart <= dispstart))
		{
                  if (msp->score == -1)
		    {
		      /* EXON */
		      if (blastx || tblastx ||  (blastn && msp->qframe[1] == actframe[1]) )
			{
			  dbdisp++;
			  if (!compN)
			    {
			      pos = (msp->qstart - dispstart) / symbfact ;
			      exon_end = (msp->qend - dispstart) / symbfact ;
			    }
			  else
			    {
			      pos = dispstart - msp->qend;
			      exon_end = dispstart - msp->qstart;
			    }
			  if (frame < 0) {
			    pos = -pos;
			    exon_end = -exon_end;
			  }
			  exon_end++;

			  if (pos < 0)
			    pos = 0 ;
			  if (exon_end > displen/symbfact)
			    exon_end = displen/symbfact;

			  graphColor(geneColor);
			  oldLinew = graphLinewidth(0.3) ;
			  graphRectangle(alignstart+pos, queryy, alignstart+exon_end, queryy+1);

			  graphColor(hiColor);
			  graphFillRectangle(alignstart+pos, queryy+dbdisp,
					     alignstart+exon_end, queryy+dbdisp+1);

			  graphColor(BLACK);

                          drawStrandName(msp->sname, msp->sframe, NAMESIZE, 1, queryy+dbdisp);
			  
			  sprintf(text, "%5d  %3d %9d", msp->score, msp->id, (compN? msp->send : msp->sstart));
			  graphText(text,  NAMESIZE+1, queryy+dbdisp);

			  sprintf(text, "%-6d", (compN? msp->sstart : msp->send));
			  graphText(text,  alignstart+1+displen/symbfact, queryy+dbdisp);
			  /* graphText(msp->sseq, alignstart+pos, queryy+dbdisp); */

			  graphLinewidth(oldLinew);
		        }
		    }
		  else if (msp->score >= 0)
		    {
		      /* Alignment. */
		      BOOL squashdo = 0;
		      int seq_len = 0 ;
		      int firstRes ;

		      if (FS(msp))
			{
			  if (squashFS)
			    squashdo = 1;
			}
		      else if (squash)
			squashdo = 1;


		      if (FS(msp) && !arrp(fsArr, msp->fs, FEATURESERIES)->on)
			continue;

		      if (FS(msp))
			msp->sseq = q ;
		      else
			{
			  if (!msp->sseq)
			    getsseq(msp) ;
			}

		      /* Sanity check on match sequence coords to catch      */
		      /* errors in homolgy data, no point in displaying them.*/
		      seq_len = strlen(msp->sseq) ;
		      if (msp->sstart < 1 || msp->send < 1 || msp->sstart > seq_len || msp->send > seq_len)
			{
			  messerror("Match coordinates for homology %s are partly or completely "
				    "outside of its own sequence. "
				    "Sequence for %s is %d long but the start/end "
				    "coordinates are %d to %d.",
				    msp->sname, msp->sname, seq_len, msp->sstart, msp->send) ;

			  continue ;
			}

		      if (!msp->id)
			calcID(msp);

		      if (!squashdo || !lastname || strcmp(msp->sname, lastname))
			dbdisp++;
		      lastname = msp->sname;

		      if (!FS(msp))
			{
			  msp->box = graphBoxStart();
			  msp->graph = graphActive();
			}

		      if (msp->in_match_set)
			graphColor(matchSetColor) ;
		      else if (!strcmp(msp->sname, HighlightSeq))
			graphColor(WHITE) ;
		      else
			graphColor(BLACK) ;

		      if ((cp = (char *)strchr(msp->sname, ':')))
			cp++ ;
		      else
			cp = msp->sname ;
                      
                      drawStrandName(cp, msp->sframe, NAMESIZE, 1, queryy+dbdisp);

		      if (!squashdo)
			{
			  sprintf(text, "%5d  %3d", msp->score, msp->id);
			  strcat(text, messprintf(" %9d", (compN ? msp->send : msp->sstart)));
			  graphText(text,  NAMESIZE+1, queryy+dbdisp);

			  sprintf(text, "%-6d", (compN ? msp->sstart : msp->send));
			  graphText(text,  alignstart+1+displen/symbfact, queryy+dbdisp);
			}

		      firstRes = -1 ;

		      for (i = 0 ; i < displen/symbfact ; i++)
			{
			  text[i] = ' ';
                        
			  if ((!compN && frame>0 && msp->qstart <= dispstart + i*symbfact && msp->qend >= dispstart + i*symbfact) ||
			      (!compN && frame<0 && msp->qstart >= dispstart - i*symbfact && msp->qend <= dispstart - i*symbfact) ||
			      ( compN && frame<0 && msp->qend <= dispstart + i		  && msp->qstart >= dispstart + i	  ) ||
			      ( compN && frame>0 && msp->qend >= dispstart - i		  && msp->qstart <= dispstart - i	  ))
			    {
			      if (firstRes == -1)
				firstRes = i;

                              /* Get the coresponding base in the subject sequence for this base, and the one before it. */ 
			      int msp_offset = gapCoord(msp, (dispstart + plusmin * i * symbfact), symbfact) ;
			      int prev_offset = gapCoord(msp, (dispstart + plusmin * (i - 1) * symbfact), symbfact) ;

                              /* If the indexes are the same, that means we have a gap in the subject sequence, so display
                               * a dot; otherwise display the relevant letter from the subject sequence. */
			      if (prev_offset == msp_offset)
				subjectsym = '.' ;
			      else
				subjectsym = msp->sseq[msp_offset - 1] ;

			      querysym   = query[i];
			      if (FS(msp))
				{
				  if (!colortoggle)
				    graphColor(msp->color);
				  else
				    graphColor(backgColor);
				}
			      else if (freeupper(querysym) == freeupper(subjectsym))
				{
				  if (IDdots)
				    subjectsym = '.';
				  if (!colortoggle && !IDdots)
				    graphColor(IDcolor);
				  else
				    graphColor(backgColor);
				}
			      else if (!blastn && PAM120[aa_atob[(unsigned int)querysym]-1 ][aa_atob[(unsigned int)subjectsym]-1 ] > 0)
				{
				  if (!colortoggle)
				    graphColor(consColor);
				  else
				    graphColor(backgColor);
				}
			      else
				{
				  /* Mismatch */
				  if (IDdots && !colortoggle) {
				    if (subjectsym == '.') subjectsym = '-';
				    graphColor(IDcolor);
				  }
				  else
				    graphColor(backgColor);
				}

			      if (HiliteSins &&
				  ((isdigit(subjectsym) || subjectsym == '(' || subjectsym == ')') ||
				   (isalpha(subjectsym) && subjectsym == freelower(subjectsym))))
				graphColor(RED);

			      if (HiliteUpperOn && isupper(subjectsym))
				graphColor(RED);
			      if (HiliteLowerOn && islower(subjectsym))
				graphColor(RED);

			      text[i] = subjectsym ;

			      graphFillRectangle(i+alignstart, queryy+dbdisp, i+alignstart+1, queryy+dbdisp+1);

                              /* If there is a gap in the reference sequence, draw a yellow bar in the subject sequence
                               * where we have missed letters out. */
			      if (abs(prev_offset - msp_offset) > 1)
				{
                                  int bar_x = alignstart + i ;

                                  BOOL forward = (strchr(msp->qframe, '+') && plusmin > 0) || (strchr(msp->qframe, '-') && plusmin < 0) ;
				  if (forward)
				    {
				      if ((strchr(msp->sframe, '+') && prev_offset > msp_offset) /* Bar at start or end of base ? */
					  || (strchr(msp->sframe, '-') && prev_offset < msp_offset))
					bar_x++ ;
				    }
				  else
				    {
				      if ((strchr(msp->sframe, '+') && prev_offset < msp_offset) /* Bar at start or end of base ? */
					  || (strchr(msp->sframe, '-') && prev_offset > msp_offset))
					bar_x++ ;
				    }
                                     
				  int bar_y = queryy + dbdisp ;

				  graphColor(YELLOW);
				  graphFillRectangle(bar_x - 0.2, bar_y, 
						     bar_x + 0.2, bar_y + 1) ;
				  graphColor(BLACK);
				}
			    }
			}

		      text[i] = '\0';

		      graphColor(BLACK);

		      graphText(text, alignstart, queryy+dbdisp);

		      if (!FS(msp))
			{
			  graphBoxEnd();
			  graphBoxDraw(msp->box, BLACK, TRANSPARENT);
			}

		      if (squashdo)
			{
			  graphColor(RED);
			  graphLine(alignstart+firstRes, queryy+dbdisp,
				    alignstart+firstRes, queryy+dbdisp+1);
			  graphColor(BLACK);
			}
		    }

		  if (!FS(msp) && DESCon && msp->desc)
		    {
		      /* Draw DESC line */
		      int box;

		      dbdisp++;
		      box = graphBoxStart();
		      graphText(messprintf("%s %s", msp->sname, msp->desc), 1, queryy+dbdisp);
		      graphBoxEnd();
		      graphBoxDraw(box, BLACK, WHITE);
		    }
		}

	    } /* else not in active frame */

	} /* End of HSPs */

      queryy += dbdisp + 2;

      if (!wholePrintOn)
	graphRedraw();

      if (!oneGraph)
	{
	  if (queryy == 1)
	    gtk_widget_hide(gexGraph2Widget(frameGraph[frameno]));
	  else
	    {
	      graphTextBounds(nx, queryy-1);
	      gtk_widget_show(gexGraph2Widget(frameGraph[frameno]));
	    }
	}

    } /* End of frames */

  if (oneGraph)
    gtk_widget_set_usize(gexGraph2Widget(blixemGraph), -2, charHeight*queryy);
  else
    graphActivate(blixemGraph);


  if (blastn)
    {
      frame = -frame;
      sprintf(actframe, "(%+d)", frame);
    }

  if (blastx || tblastx)
    dispstart -= plusmin*2;


  /* To ensure that all the blixem graphs get redrawn we invalidate the entire blixem window,
   * this is a hack required by the difficulty of making the graph package coexist with gtk. */
  {
    GdkRectangle widg_rect ;

    widg_rect.x = blixemWindow->allocation.x ;
    widg_rect.y = blixemWindow->allocation.y ;
    widg_rect.width = blixemWindow->allocation.width ;
    widg_rect.height = blixemWindow->allocation.height ;

    gdk_window_invalidate_rect(blixemWindow->window, &widg_rect, TRUE) ;
  }

  if (!wholePrintOn)
    graphRedraw();

  highlightProteinboxes(TRUE);

  messfree(query);

  setMenuCheckmarks();

  return ;
}


/* GET3RD_BASE returns every third base from string q
*/
static char *get3rd_base(int start, int end, char *q)
{
  static int i, len;
  static char *bases = NULL, *aux, *aux2;

  if (start < 1 || end > qlen)
    {
      messerror ( "Genomic sequence coords out of range: %d - %d\n",
		  start, end);
      
      return NULL;
    }

  if (!bases)
    {
      bases = messalloc(qlen+1);
      aux = messalloc(qlen+1);
      aux2 = messalloc(qlen+1);
    }

  len = abs(end-start) + 1;

  if (start < end)
    {
      for (i=start; i < end; i += 3 )
	bases[(i-start)/3] = q[i-1];
    }
  else if (start > end) /* Reverse and complement it first */
    {
      strncpy(aux2, q+end-1, start-end+1);
      aux2[start-end+1] = 0;

      if (!revComplement(aux, aux2))
	  messcrash ("Cannot reverse & complement") ;

      for (i=0; i < len; i += 3)
	bases[i/3] = aux[i];
    }
  else
    {
      return NULL;
    }
      
  bases[len/3] = '\0';

  return bases;
}


/* calcID: caculated percent identity of an MSP
 * 
 * There seems to be a general problem with this routine for protein
 * alignments, the existing code certainly does not do the right thing.
 * I have fixed this routine for gapped sequence alignments but not for
 * protein stuff at all.
 * 
 * To be honest I think this routine is a _waste_ of time, the alignment
 * programs that feed data to blixem produce an identity anyway so why
 * not use that...why reinvent the wheel......
 * 
 * */
static void calcID(MSP *msp)
{
  static int id, i, len ;
  char *qseq ;
  
  BOOL sForward = strchr(msp->sframe, '+') ? TRUE : FALSE;

  int qSeqMin, qSeqMax, sSeqMin, sSeqMax;
  getMspRangeExtents(msp, &qSeqMin, &qSeqMax, &sSeqMin, &sSeqMax);

  if (msp->sseq && msp->sseq != padseq)
    {
      /* Note that getqseq() will reverse complement if qstart > qend, this means that
       * where there is no gaps array the comparison is trivial as coordinates can be
       * ignored and the two sequences just whipped through. */
      if (!(qseq = getqseq(msp->qstart, msp->qend, q)))
	{
	  messout ( "calcID failed: Don't have coords %d - %d on the genomic sequence (required for match sequence %s [match coords = %d - %d])\n",
		    msp->qstart, msp->qend, msp->sname, msp->sstart, msp->send);
	  msp->id = 0;

	  return;
	}


      /* NOTE, tblastn/x are not implemented for gaps yet. */
      if (!(msp->gaps) || arrayMax(msp->gaps) == 0)
	{
	  /* Ungapped alignments. */

	  if (tblastn)
	    {
	      len = msp->qend - msp->qstart + 1;
	  
	      for (i=0, id=0; i < len; i++)
		if (freeupper(msp->sseq[i]) == freeupper(qseq[i]))
		  id++;
	    }
	  else if (tblastx)
	    {
	      len = abs(msp->qend - msp->qstart + 1)/3;
	  
	      for (i=0, id=0; i < len; i++)
		if (freeupper(msp->sseq[i]) == freeupper(qseq[i]))
		  id++;
	    }
	  else						    /* blastn, blastp & blastx */
	    {
	      len = sSeqMax - sSeqMin + 1 ;
	  
	      for (i=0, id=0; i< len; i++)
                {
                  int sIndex = sForward ? sSeqMin + i - 1 : sSeqMax - i - 1;
		  if (freeupper(msp->sseq[sIndex]) == freeupper(qseq[i]))
		    id++;
                }
	    }
	}
      else
          {
	  /* Gapped alignments. */

	  /* To do tblastn and tblastx is not imposssible but would like to work from
	   * examples to get it right.... */
	  if (tblastn)
	    {
	      printf("not implemented yet\n") ;
	    }
	  else if (tblastx)
	    {
	      printf("not implemented yet\n") ;
	    }
	  else
	    {
	      /* blastn and blastp remain simple but blastx is more complex since the query
               * coords are nucleic not protein. */
            
              BOOL qForward = (strchr(msp->qframe, '+')) ? TRUE : FALSE ;
              Array gaps = msp->gaps;
	      int factor = blastx ? 3 : 1 ;

	      len = 0 ;
              int i ;
	      for (i = 0, id = 0 ; i < arrayMax(gaps) ; i++)
		{
		  SMapMap *m = arrp(gaps, i, SMapMap) ;
		  
		  int qRangeMin, qRangeMax, sRangeMin, sRangeMax;
		  getSMapMapRangeExtents(m, &qRangeMin, &qRangeMax, &sRangeMin, &sRangeMax);
		  
                  len += sRangeMax - sRangeMin + 1;
                  
                  /* Note that qseq has been cut down to just the section relating to this msp.
		   * We need to translate the first coord in the range (which is in terms of the full
		   * reference sequence) into coords in the cut-down ref sequence. */
                  int q_start = qForward ? (qRangeMin - qSeqMin) / factor : (qSeqMax - qRangeMax) / factor;
		  
		  /* We can index sseq directly (but we need to adjust by 1 for zero-indexing). We'll loop forwards
		   * through sseq if we have the forward strand or backwards if we have the reverse strand,
		   * so start from the lower or upper end accordingly. */
                  int s_start = sForward ? sRangeMin - 1 : sRangeMax - 1 ;
		  
                  int j = s_start, k = q_start ;
		  while ((sForward && j < sRangeMax) || (!sForward && j >= sRangeMin - 1))
		    {

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
		      char *sub, *query ;
		      char *dummy ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
		      /* for debug..... */
		      sub = msp->sseq + j ;
		      query = qseq + k ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

			
		      if (freeupper(msp->sseq[j]) == freeupper(qseq[k]))
			id++ ;

                      
#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
		      else
			dummy = sub ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */
                      

                      /* Move to the next base. The qseq is always forward, but we might have to
                       * traverse the s sequence in reverse. */
                      ++k ;
                      if (sForward) ++j ;
                      else --j ;
		    }
		}
	    }
	}

      msp->id = (int)((float)100*id/len + .5);

      messfree(qseq);
    }
  else
    msp->id = 0 ;

  return ;
}



static void allocAuxseqs(int len)
{
    if (auxseq) {
	messfree(auxseq);
	messfree(auxseq2);
    }

    auxseq = messalloc(len+1);
    auxseq2 = messalloc(len+1);
    auxseqlen = len;
}




void blxAssignPadseq(MSP *msp)
{
    static int padseqlen=0;
    char *oldpadseq;
    int len = max(msp->sstart, msp->send);
    MSP *hsp;

    if (!padseq) {
	padseq = messalloc(INITDBSEQLEN+1);
	memset(padseq, '-', INITDBSEQLEN);
	padseqlen = INITDBSEQLEN;
    }

    if (len > padseqlen) {
	oldpadseq = padseq;
	messfree(padseq);

	padseq = messalloc(len+1);
	memset(padseq, '-', len);
	padseqlen = len;

	/* Change all old padseqs to new */
	for (hsp = MSPlist; hsp ; hsp = hsp->next)
	    if (hsp->sseq == oldpadseq) hsp->sseq = padseq;
    }

    msp->sseq = padseq;
 }



/* GETQSEQ translates a segment of the query seq (with 'sequence' coords = 1...) */
static char *getqseq(int start, int end, char *q)
{
  char *query = NULL ;

  if (start < 1 || end < 1 || start > qlen || end > qlen)
    {
      messout ( "Requested query sequence %d - %d out of available range: 1 - %d\n",
		start, end, qlen);

      return NULL;
    }

  if (blastp || tblastn)
    {
      query = messalloc(end-start+2);
      strncpy(query, q+start-1, end-start+1);
      query[end-start+1] = 0;

      return query;
    }

  if (abs(end-start)+1 > auxseqlen)
    allocAuxseqs(abs(end-start)+1);

  if (start <= end)
    {
      strncpy(auxseq, q+start-1, end-start+1);
      auxseq[end-start+1] = 0;
    }
  else if (start > end) /* Reverse and complement it */
    {
      strncpy(auxseq2, q+end-1, start-end+1);
      auxseq2[start-end+1] = 0;

      if (!revComplement(auxseq, auxseq2))
	messcrash ("Cannot reverse & complement") ;
    }
  else
    {
      return NULL;
    }

  if (blastn) 
    {
      query =  messalloc(strlen(auxseq)+1);
      strcpy(query, auxseq);

      return query;
    }

  /* Translate the DNA sequence */
  if (!(query = blxTranslate(auxseq, stdcode1)))
    messcrash ("Cannot translate the genomic sequence") ;

  return query;
}


/* GETSSEQ fetches the database sequence from an external database,
 * currently uses either efetch or pfetch. */
static void getsseq(MSP *msp)
{

  MSP  *auxmsp;
  int   len;
  char *fetch_prog ;
  char *seq_buf = NULL ;

  if (!*msp->sname)
    {
      messout ( "Nameless HSP at %d-%d - skipping Efetch\n",
		msp->qstart+qoffset, msp->qend+qoffset);
      blxAssignPadseq(msp);

      return ;
    }

  fetch_prog = blxGetFetchProg() ;

  if (verbose)
    printf("%sing %s\n", fetch_prog, msp->sname);
  else
    {
      if (!fetchCount)
	{
	  fetchCount++;
	  printf("\n%sing external sequences", fetch_prog);
	}
      printf(".");
      fflush(stdout);
    }

  if (msp->score >= 0)
    {
      if ((seq_buf = getSeq(msp->sname, blxGetFetchMode())))
	{
	  msp->sseq = messalloc(strlen(seq_buf)+1);

	  /* Harmonize upper and lower case - (t)blastx always translate
	   * the query to upper case */
	  if (isupper(*q) || tblastx || blastx)
	    {
	      for (i=0; seq_buf[i]; i++)
		msp->sseq[i] = freeupper(seq_buf[i]);
	      msp->sseq[i] = 0;
	    }
	  else
	    {
	      for (i=0; seq_buf[i]; i++)
		msp->sseq[i] = freelower(seq_buf[i]);

	      msp->sseq[i] = 0;
	    }

	  /* Check illegal offsets */
	  len = strlen(msp->sseq);
	  for (auxmsp = MSPlist; auxmsp ; auxmsp = auxmsp->next)
	    if (!strcmp(auxmsp->sname, msp->sname) && auxmsp->send > len )
	      {
		printf("%s HSP with offset beyond sequence (%d > %d) - using pads\n",
		       msp->sname, auxmsp->send, len);
		blxAssignPadseq(msp);

		break;
	      }
	}
      else
	{
#if !defined(ACEDB)
	  messout ( "Unable to %s %s - using pads instead\n", fetch_prog, msp->sname);
#endif
	  /* Sequence not in database - fill up with pads */
	  blxAssignPadseq(msp);
	}

      





      /* Set sseq for all MSPs of this subject, either a padseq or
       * the sequence for that subject, taking into account the _strand_.
       * 
       * THIS IS NOT TOO EFFICIENT...WE COULD BE HASHING HERE....
       *  */
      for (auxmsp = MSPlist ; auxmsp ; auxmsp = auxmsp->next)
	{
	  if (strcmp(auxmsp->sname, msp->sname) == 0)
	    {
	      /* Either assign */
	      if (msp->sseq == padseq)
		blxAssignPadseq(auxmsp);
	      else if (MSPSTRAND(auxmsp->sframe) == MSPSTRAND(msp->sframe))
		auxmsp->sseq = msp->sseq ;

	      calcID(auxmsp);
	    }
	}
    }


  return ;
}


/* Get a sequence entry using either efetch or pfetch. */
static char *fetchSeqRaw(char *seqname)
{
  char *result = NULL ;
  char *fetch_prog = NULL ;
  char *seq_buf = NULL ;

  if (!*seqname)
    {
      messout ( "Nameless sequence - skipping Efetch\n");
      return NULL ;
    }

  fetch_prog = blxGetFetchProg() ;

  if ((seq_buf = getSeq(seqname, fetch_prog)))
    {
      result = messalloc(strlen(seq_buf)+1) ;
      strcpy(result, seq_buf) ;
    }
  else
    {
      messout("Unable to %s %s \n", fetch_prog, seqname);
      result = 0;
    }

  return result ;

}

/* Common routine to call efetch or pfetch to retrieve a sequence entry. */
static char *getSeq(char *seqname, char *fetch_prog)
{
  char *result = NULL ;
  static char *fetchstr = NULL ;
  char *cp ;
  FILE *pipe ;
  int len ;
  FILE *fp ;

  /* I have no idea why blixem trys to open this file, it doesn't work unless you are in
   * a directory that you have write access to anyway....I await an answer from Eric.
   * Meanwhile at least now we record the error rather than crashing later in this
   * routine when we can't open the file. */
  if (!(fp = fopen("myoutput","w")))
    {
      messerror("%s", "Cannot open blixem sequence output file: \"myoutput\"") ;
    }

  if (!strcmp(fetch_prog, "pfetch"))
    {
      /* --client gives logging information to pfetch server,
       * -q  Sequence only output (one line) */
      fetchstr = hprintf(0, "%s --client=acedb_%s_%s -q '%s' &",
			 fetch_prog, getSystemName(), getLogin(TRUE), seqname) ;
    }
  else
    {
      fetchstr = hprintf(0, "%s -q '%s'", fetch_prog, seqname) ;
    }

  printf("%sing %s...\n", fetch_prog, seqname);

  /* Try and get the sequence, if we overrun the buffer then we need to try again. */
  if (!(pipe = (FILE*)popen(fetchstr, "r")))
    messcrash("Failed to open pipe %s\n", fetchstr);
  len = 0;
  cp = auxseq ;
  while (!feof(pipe))
    {
      if (len < auxseqlen)
	*cp++ = fgetc(pipe);
      else
	fgetc(pipe);
      len++;

      if (fp)						    /* n.b. file may not have been opened. */
	fprintf(fp, "%s", cp);
    }
  pclose(pipe);


  /* Check if auxseq was long enough */
  if (len > auxseqlen)
    {
      allocAuxseqs(len);
      cp = auxseq;

      if (!(pipe = (FILE*)popen(fetchstr, "r")))
	messcrash("Failed to open pipe %s\n", fetchstr);

      while (!feof(pipe))
	{
	  *cp++ = fgetc(pipe);
	}
      pclose(pipe);
    }

  *cp = 0;

  if (len > 1) /* Otherwise failed */
    {
      if ((cp = (char *)strchr(auxseq, '\n')))
	*cp = 0;
      result = auxseq ;
    }
  else
    {
      result = NULL ;
    }

  if (fetchstr)
    messfree(fetchstr) ;

  if (fp)
    fclose(fp);

  return result ;
}


/********************************************************************************
**                            BIG PICTURE ROUTINES                            ***
********************************************************************************/
static void GPrefsSettings(GtkButton *button, gpointer args)
{
  blixemSettings();
}

static void GGoto(GtkButton *button, gpointer args)
{
  Goto();
}

static void GprevMatch(GtkButton *button, gpointer args)
{
  prevMatch();
}

static void GnextMatch(GtkButton *button, gpointer args)
{
  nextMatch();
}

static void GscrollLeftBig(GtkButton *button, gpointer args)
{
  scrollLeftBig();
}

static void GscrollRightBig(GtkButton *button, gpointer args)
{
  scrollRightBig();
}

static void GscrollLeft1(GtkButton *button, gpointer args)
{
  scrollLeft1();
}

static void GscrollRight1(GtkButton *button, gpointer args)
{
  scrollRight1();
}

static void GToggleStrand(GtkButton *button, gpointer args)
{
  ToggleStrand();
}

static void GHelp(GtkButton *button, gpointer args)
{
  blxHelp();
}

static void comboChange(GtkEditable *edit, gpointer args)
{

  gchar *val = gtk_editable_get_chars(edit, 0, -1);

  if (GTK_WIDGET_REALIZED(blixemWindow))
   {
     if (strcmp(val, "score") == 0)
       sortByScore();
     else if (strcmp(val, "identity") == 0)
       sortById();
     else if (strcmp(val, "name") == 0)
       sortByName();
     else if (strcmp(val, "position") == 0)
       sortByPos();
   }
  g_free(val);
}

static void buttonAttach(GtkHandleBox *handlebox,
			 GtkWidget *toolbar, gpointer data)
{
  gtk_widget_set_usize(toolbar, 1, -2);
}

static void buttonDetach(GtkHandleBox *handlebox,
			 GtkWidget *toolbar, gpointer data)
{
  gtk_widget_set_usize(toolbar, -1, -2);
}


static GtkWidget *makeButtonBar(void)
{
  GtkWidget *handleBox, *toolbar, *entry, *combo ;
  GtkToolItem *item ;
  GList *sortList = NULL;


  handleBox = gtk_handle_box_new() ;

  toolbar = gtk_toolbar_new() ;
  gtk_toolbar_set_tooltips(GTK_TOOLBAR(toolbar), TRUE) ;

  combo = gtk_combo_new() ;

  /* next three lines stop toolbar forcing the size of a blixem window */
  gtk_signal_connect(GTK_OBJECT(handleBox), "child-attached",
		     GTK_SIGNAL_FUNC(buttonAttach), NULL);
  gtk_signal_connect(GTK_OBJECT(handleBox), "child-detached",
		     GTK_SIGNAL_FUNC(buttonDetach), NULL);

  gtk_widget_set_usize(toolbar, 1, -2);

  messageWidget = entry = gtk_entry_new() ;
  gtk_editable_set_editable(GTK_EDITABLE(entry), FALSE);
  gtk_widget_set_size_request(entry, 300, -1) ;

  sortList = g_list_append(sortList, "score");
  sortList = g_list_append(sortList, "identity");
  sortList = g_list_append(sortList, "name");
  sortList = g_list_append(sortList, "position");
  gtk_editable_set_editable(GTK_EDITABLE(GTK_COMBO(combo)->entry), FALSE);

  gtk_widget_set_usize(GTK_COMBO(combo)->entry, 80, -2);
  gtk_signal_connect(GTK_OBJECT(GTK_COMBO(combo)->entry), "changed",
		     (GtkSignalFunc)comboChange, NULL);

  gtk_combo_set_popdown_strings(GTK_COMBO(combo), sortList);
  gtk_entry_set_text(GTK_ENTRY(GTK_COMBO(combo)->entry), "identity");

  gtk_container_add(GTK_CONTAINER(handleBox), toolbar);


  item = makeNewToolbarButton(GTK_TOOLBAR(toolbar),
			      "Help",
			      "Don't Panic",
			      (GtkSignalFunc)GHelp) ;

  item = addToolbarWidget(GTK_TOOLBAR(toolbar), gtk_label_new(" Sort HSPs by: ")) ;
  item = addToolbarWidget(GTK_TOOLBAR(toolbar), combo) ;


  item = makeNewToolbarButton(GTK_TOOLBAR(toolbar),
			      "Settings",
			      "Open the Preferences Window",
			      (GtkSignalFunc)GPrefsSettings) ;

  item = makeNewToolbarButton(GTK_TOOLBAR(toolbar),
			      "Goto", "Go to specified co-ordinates",
			      (GtkSignalFunc)GGoto) ;

  item = makeNewToolbarButton(GTK_TOOLBAR(toolbar),
			      "< match", "Next (leftward) match",
			      (GtkSignalFunc)GprevMatch) ;

  item = makeNewToolbarButton(GTK_TOOLBAR(toolbar),
				 "match >", "Next (rightward) match",
			      (GtkSignalFunc)GnextMatch) ;

  item = makeNewToolbarButton(GTK_TOOLBAR(toolbar),
				 "<<", "Scroll leftward lots",
			      (GtkSignalFunc)GscrollLeftBig) ;

  item = makeNewToolbarButton(GTK_TOOLBAR(toolbar),
			      ">>", "Scroll rightward lots",
			      (GtkSignalFunc)GscrollRightBig) ;

  item = makeNewToolbarButton(GTK_TOOLBAR(toolbar),
				 "<", "Scroll leftward one base",
			      (GtkSignalFunc)GscrollLeft1) ;

  item = makeNewToolbarButton(GTK_TOOLBAR(toolbar),
			      ">", "Scroll rightward one base",
			      (GtkSignalFunc)GscrollRight1) ;


  if (blastx ||tblastx || blastn)
    {
      item = makeNewToolbarButton(GTK_TOOLBAR(toolbar),
				  "Strand^v", "Toggle strand",
				  (GtkSignalFunc)GToggleStrand) ;
    }
  
  item = addToolbarWidget(GTK_TOOLBAR(toolbar), entry) ;


  gtk_widget_show_all(handleBox);

  return handleBox ;
}


static void BigPictToggle (void) {
    BigPictON = !BigPictON;
    blviewRedraw();
}

static void BigPictToggleRev (void) {
    BigPictRev = !BigPictRev;
    blviewRedraw();
}


static void zoomOut(void)
{
  BigPictZoom *= 2;

  blviewRedraw();
}

static void zoomIn(void)
{
  BigPictZoom /= (float)2;
  if (BigPictZoom < 1)
    BigPictZoom = 1;

  blviewRedraw();
}

static void zoomWhole(void)
{
  BigPictZoom = (float)qlen/displen;

  blviewRedraw();
}


/* If crosshair-coordinates are screwed up, change here!
 ********************************************************/
static int x_to_residue(float x)
{
  int retval;

  if (blastx || tblastx)
    retval = dispstart + plusmin*(x - NAMESIZE - 22.3)*3;
  else
    retval = dispstart + plusmin*(x - NAMESIZE - 22);


  if (plusmin > 0) {
    if (retval < dispstart) retval = dispstart;
    if (retval > dispstart+displen-1) retval = dispstart+displen-1;
    return retval;
  }
  else {
    if (retval > dispstart-1) retval = dispstart-1;
    if (retval < dispstart-displen) retval = dispstart-displen;
    return retval +1;
  }
}


static void displayResidue(double x)
{
  int qpos, spos ;
  static char queryname[NAMESIZE+1] ;

  if (!*queryname)
    {
      if (!*qname_G)
	strcpy(queryname, "Query");
      else
	{
	  char *abbrev ;

	  abbrev = abbrevTxt(qname_G, NAMESIZE) ;
	  strncpy(queryname, abbrev, NAMESIZE) ;
	}

      queryname[NAMESIZE] = 0 ;
    }

  /* Get the index of the reference sequence base at this coord */
  qpos = x_to_residue(x);

  if (!pickedMSP)
    {
      sprintf(message, "%d   No subject picked", qpos + qoffset) ;
    }
  else
    {
      /* Find the match sequence base corresponding to this ref seq base */
      int sSeqMin, sSeqMax;
      getMspRangeExtents(pickedMSP, NULL, NULL, &sSeqMin, &sSeqMax);
      
      if (blastx || tblastx)
	{
	  spos = gapCoord(pickedMSP, qpos, 3);
	}
      else if (blastn)
	{
	  spos = gapCoord(pickedMSP, qpos, 1);
	}
      else if (tblastn)
	{
	  if (pickedMSP->sstart < pickedMSP->send)
	    {
	      spos = (qpos - pickedMSP->qstart)*3 + pickedMSP->sstart;
	    }
	  else
	    {
	      spos = pickedMSP->sstart - (qpos - pickedMSP->qstart)*3;
	    }
	}
      else
	{
	  spos = qpos - pickedMSP->qstart + pickedMSP->sstart;
	}
      
      /* Limit it to the bounds of the currently-selected match */
      if (spos < sSeqMin)
	spos = sSeqMin;
      if (spos > sSeqMax)
	spos = sSeqMax;

      /* Create the message text to feed back to the user */
      sprintf(message, "%d   ", qpos + qoffset) ;

      if (HSPgaps)
	strcat(message, "Gapped HSP - no coords");
      else
	strcat(message, messprintf("%s: %d", pickedMSP->sname, spos));
    }

  gtk_entry_set_text(GTK_ENTRY(messageWidget), message);

  return ;
}


static void markDNA(double y)
{
  Graph old = graphActive();

  if (y < 0)
    return;

  graphActivate(frameGraphQ[0]);

  graphXorLine (NAMESIZE+22, 1.5 + y,
		NAMESIZE+22+displen/3, 1.5 + y);

  graphActivate(old);
}



/* Mouse drag on the upper, "big picture", section of the graph. */
static void MiddleDownBP (double x, double y)
{
  graphXorBox (BPbox, x - BPboxwidth/2, 1.85);

  graphRegister (MIDDLE_DRAG, MiddleDragBP) ;	/* must redo */
  graphRegister (MIDDLE_UP, MiddleUpBP) ;

  oldx = x;
}

static void MiddleDragBP (double x, double y)
{
  graphXorBox (BPbox, oldx - BPboxwidth/2, 1.85);
  graphXorBox (BPbox, x - BPboxwidth/2, 1.85);

  oldx = x ;
}

static void MiddleUpBP (double x, double y)
{
  graphFitBounds (&nx, 0);
  nx -= 2;						    /* scrollbars */

  dispstart = BigPictStart + (plusmin * ((x-BPoffset)/(nx-BPoffset)*BigPictLen - displen/2)) ;
  blviewRedraw();

  return ;
}



/* Mouse drag on the lower, "query", section of the graph. */
static void MiddleDragQ (double x, double y)
{
  int i, noframes = blastn ? 2 : 3;

  for(i=0; i<noframes; i++)
    {
      graphActivate(frameGraphQ[i]);
      graphXorLine (oldx, 0, oldx, 1000) ;
      graphXorLine (x, 0, x, 1000);
      graphActivate(frameGraph[i]);
      graphXorLine (oldx, 0, oldx, 1000) ;
      graphXorLine (x, 0, x, 1000);
    }

  if (blastx || tblastx)
    {
      markDNA(DNAstep);
      DNAstep =  abs ( (x_to_residue(x) - dispstart) % 3);
      markDNA(DNAstep);
    }

  graphActivate(blixemGraph);

  displayResidue(x);

  oldx = x ;
}

static void MiddleDownQ (double x, double y)
{
  float nyf;
  int i, noframes = blastn ? 2 : 3;

  graphFitBounds (&nx, 0);
  graphWhere(0,0,0, &nyf);
  ny = nyf - 0.5 ;
  graphRegister(MIDDLE_DRAG, MiddleDragQ) ;	/* must redo */
  graphRegister(MIDDLE_UP, MiddleUpQ) ;

  for (i = 0 ; i < noframes ; i++)
    {
      graphActivate(frameGraphQ[i]);
      graphXorLine (x, 0, x, 1000);
      graphActivate(frameGraph[i]);
      graphXorLine (x, 0, x, 1000);
    }

  /* Cleanse messagebox from pick */
  *message = 0 ;
  gtk_entry_set_text(GTK_ENTRY(messageWidget), message) ;

  graphActivate(frameGraphQ[0]);
  if (blastx || tblastx)
    {
      DNAstep = (x_to_residue(x) - dispstart) % 3;
      markDNA(DNAstep);
    }

  displayResidue(x);
  oldx = x;

  return ;
}

static void MiddleUpQ (double x, double y)
{
  dispstart = x_to_residue(x) - plusmin*displen/2;
  blviewRedraw();
}

	


static void setHighlight(void)
{
    static char dfault[64] = "";
    ACEIN string_in;

    if ((string_in = messPrompt ("String: (wildcards: * ?)",
				 dfault, "t", 0)))
      {
	/* ANSI way */
	strncpy(searchSeq, aceInWord(string_in), NAMESIZE+3);
	searchSeq[NAMESIZE+3] = 0;
	for (cp = searchSeq; *cp ; cp++) *cp = freeupper(*cp);

	/* Non-ANSI bsd way :
	   if (!re_comp(searchSeq)) fprintf(stderr, "%s\n", re_comp(searchSeq));*/

	strncpy(dfault, searchSeq, 63);
	dfault[63] = '\0';

	blviewRedraw();

	aceInDestroy (string_in);
    }
}


static void clrHighlight(void)
{
    MSP *msp;

    /* Clear highlighted */
    *searchSeq = *HighlightSeq = 0;
    pickedMSP = 0;

    /* Unhide hidden matches */
    for (msp = MSPlist; msp ; msp = msp->next)
	if (msp->score == -999) {
	    msp->score = msp->id;
	    calcID(msp);
	}

    blviewRedraw();
}



/* Attempts to set the range of dotter in some sort of sensible way. The problem is that
 * hits can occur over a much wider range than the user is looking at, so the function
 * attempts to find the range of hits that corresponds to what the user can see.
 * Returns TRUE if it managed to find sequences and set a sensible range, FALSE otherwise. */
static BOOL smartDotterRange(char *selected_sequence, MSP *msp_list, int blastn,
			     int strand_sign, int view_start, int view_end,
			     char **dottersseq_out, int *dotter_start_out, int *dotter_end_out)
{
  BOOL result = FALSE ;
  char strand ;
  int qstart, qend, sstart, send, extend, len, mid ;
  MSP *msp ;
  int start, end ;

  if (strand_sign > 0)
    {
      start = view_start ;
      end = view_end ;
    }
  else
    {
      start = view_end ;
      end = view_start ;
    }

  strand = (strand_sign > 0 ? '+' : '-') ;

  /* Estimate wanted query region from extent of HSP's that are completely within view. */
  for (qstart = 0, qend=0, sstart=0, send=0, msp = msp_list ; msp ; msp = msp->next)
    {
      if (strcmp(msp->sname, selected_sequence) == 0 && (msp->qframe[1] == strand || blastn))
	{
	  /* If you alter this code remember that you need to allow for strand. */
	  if (msp->qstart >= start && msp->qend <= end)
	    {
	      if (!qstart)
		{
		  qstart = msp->qstart;
		  qend = msp->qend;
		  sstart = msp->sstart;
		  send = msp->send;
		}
	      else
		{
		  if (msp->qframe[1] == '+')
		    {
		      if (msp->qstart < qstart)
			{
			  qstart = msp->qstart;
			  sstart = msp->sstart;
			}
		      if (msp->qend > qend)
			{
			  qend = msp->qend;
			  send = msp->send;
			}
		    }
		  else
		    {
		      if (msp->qstart > qstart)
			{
			  qstart = msp->qstart;
			  sstart = msp->sstart;
			}
		      if (msp->qend < qend)
			{
			  qend = msp->qend;
			  send = msp->send;
			}
		    }
		}
	    }
	}
    }



  if (!qstart)
    {
      messout("Could not find any matches on the '%c' strand to %s.",
	      strand, selected_sequence) ;
      result = FALSE ;
    }
  else
    {
      /* MSP's hold both query and subject positions as always forward and hold the strand
       * for both separately, this means we can do all these calculations with forward
       * coords and then reverse the start/end when we've finished. */

      /* Extrapolate start to start of vertical sequence */
      extend = sstart;
      if (blastx || tblastx)
	extend *= 3;

      qstart -= extend ;

      /* Extrapolate end to end of vertical sequence */
      if (!tblastn && (*dottersseq_out = fetchSeqRaw(selected_sequence)))
	{
	  extend = strlen(*dottersseq_out) - send;
	}
      else
	{
	  extend = 200;
	}
      if (blastx || tblastx)
	extend *= 3;
      qend += extend;


      /* Due to gaps, we might miss the ends - add some more */
      extend = 0.1 * (qend - qstart) ;
      qstart -= extend ;
      qend += extend ;

      if (blastx || tblastx)
	{
	  /* If sstart and send weren't in the end exons, we'll miss those - add some more */
	  extend = 0.2 * (qend - qstart) ;
	  qstart -= extend ;
	  qend += extend ;
	}

      /* Keep it within bounds */
      if (qstart < 1)
	qstart = 1;
      if (qend > qlen)
	qend = qlen;
      if (qstart > qlen)
	qstart = qlen;
      if (qend < 1)
	qend = 1;

      /* Apply min and max limits:  min 500 residues, max 10 Mb dots */
      len = (qend - qstart) ;
      mid = qstart + len/2 ;

      if (len < 500)
	len = 500 ;
      if (len * (send-sstart) > 1e7)
	len = 1e7/(send - sstart) ;

      qstart = mid - (len / 2) ;
      qend = mid + (len / 2) ;

      /* Keep it within bounds */
      if (qstart < 1)
	qstart = 1;
      if (qend > qlen)
	qend = qlen;
      if (qstart > qlen)
	qstart = qlen;
      if (qend < 1)
	qend = 1;

      /* Return the start/end, now we do need to do something about strand... */
      if (strand_sign > 0)
	{
	  *dotter_start_out = qstart ;
	  *dotter_end_out = qend ;
	}
      else
	{
	  *dotter_start_out = qend ;
	  *dotter_end_out = qstart ;
	}

      result = TRUE ;
    }

  return result ;
}


static void callDotter(void)
{
  static char opts[] = "     ";
  char type, *queryseq, *sname;
  int offset;
  MSP *msp;

  if (!*HighlightSeq)
    {
      messout("Select a sequence first");
      return;
    }

  if (smartDotter)
    {
      int end ;

      if (plusmin == 1)
	end = (BigPictStart + BigPictLen - 1) ;
      else
	end = (BigPictStart - BigPictLen + 1) ;


      if (!smartDotterRange(HighlightSeq, MSPlist, blastn,
			    plusmin, BigPictStart, end,
			    &dottersseq, &dotterStart, &dotterEnd))
	return ;
    }
  else
    dottersseq = fetchSeqRaw(HighlightSeq);

  type = ' ' ;
  if (blastp || tblastn)
    type = 'P';
  else if (blastx)
    type = 'X';
  else if (blastn || tblastx)
    type = 'N';

  /* Try to get the subject sequence, in case we're in seqbl mode (only part of seq in MSP) */
  if (!dottersseq || tblastn)
    {
    /* Check if sequence is passed from acedb */
    if (!tblastx)
      {
	printf("Looking for sequence stored internally ... ");
	for (msp = MSPlist; msp ; msp = msp->next)
	  {
	    if (!strcmp(msp->sname, HighlightSeq) && msp->sseq != padseq)
	      {
		dottersseq = messalloc(strlen(msp->sseq)+1);
		strcpy(dottersseq, msp->sseq);
		break;
	      }
	  }
	if (!dottersseq) printf("not ");
	printf("found\n");
      }

    if (!dottersseq)
      {
	printf("Can't fetch subject sequence for dotter - aborting\n");
	messout("Can't fetch subject sequence for dotter - aborting\n");
	return;
      }
    }

  if (strchr(dottersseq, '-') || tblastn )
    messout("Note: the sequence passed to dotter is incomplete");

  if (!*dotterqname)
    {
      if (!*qname_G)
	strcpy(dotterqname, "Blixem-seq");
      else
	strncpy(dotterqname, qname_G, LONG_NAMESIZE);
      dotterqname[LONG_NAMESIZE] = 0;
    }

  /* Get query sequence */
  /* Avoid translating queryseq by pretending to be blastn - very sneaky and dangerous */
  if (blastx || tblastx)
    blastn = 1;
  if (!(queryseq = getqseq(dotterStart, dotterEnd, q)))
    {
      if (blastx || tblastx)
	blastn = 0;	/* if blastn was set, reset it */
      return;
    }
  if (blastx || tblastx)
    blastn = 0;

  if (plusmin > 0)
    {
      offset = dotterStart-1 + qoffset;
      opts[0] = ' ';
    }
  else
    {
      offset = dotterEnd-1 + qoffset;
      opts[0] = 'R';
    }

  if ((sname = strchr(HighlightSeq, ':')))
    sname++;
  else
    sname = HighlightSeq;

  opts[1] = (dotterHSPs ? 'H' : ' ');

  opts[2] = (HSPgaps ? 'G' : ' ');

  printf("Calling dotter with query sequence region: %d - %d\n", dotterStart, dotterEnd);

  printf("  query sequence: name -  %s, offset - %d\n"
	 "subject sequence: name -  %s, offset - %d\n", dotterqname, offset, sname, 0) ;

  dotter(type, opts, dotterqname, queryseq, offset, sname, dottersseq, 0,
	 0, 0, NULL, NULL, NULL, 0.0, dotterZoom, MSPlist, qoffset, 0, 0);

  return ;
}


static void callDotterHSPs(void)
{
    dotterHSPs = 1;
    callDotter();
    dotterHSPs = 0;
}


static int smartDotterRangeSelf(void)
{
    int len, mid;

    len = 2000;
    mid = dispstart + plusmin*displen/2;

    dotterStart = mid - plusmin*len/2;
    dotterEnd = mid + plusmin*len/2;

    /* Keep it within bounds */
    if (dotterStart < 1) dotterStart = 1;
    if (dotterStart > qlen) dotterStart = qlen;
    if (dotterEnd > qlen) dotterEnd = qlen;
    if (dotterEnd < 1) dotterEnd = 1;

    return 1;
}


static void callDotterSelf(void)
{
    static char opts[] = "     ";
    char type, *queryseq;
    int  offset;

    type = ' ';
    if (blastp || tblastn || tblastx)
      type = 'P';
    else if (blastx || blastn)
      type = 'N';

    if (smartDotter)
	if (!smartDotterRangeSelf())
	  return;

    if (!*dotterqname)
      {
	if (!*qname_G)
	  strcpy(dotterqname, "Blixem-seq");
	else
	  strncpy(dotterqname, qname_G, LONG_NAMESIZE);
	dotterqname[LONG_NAMESIZE] = 0;
      }

    /* Get query sequence */
    /* Can't do reversed strand since Dotter can't reverse vertical scale */
    /* Avoid translating queryseq by pretending to be blastn - very sneaky and dangerous */
    if (blastx || tblastx)
      blastn = 1;

    if (!(queryseq = getqseq(min(dotterStart, dotterEnd), max(dotterStart, dotterEnd), q)))
      return;

    if (blastx || tblastx)
      blastn = 0;

    dottersseq = messalloc(strlen(queryseq)+1);
    strcpy(dottersseq, queryseq);

    offset = min(dotterStart, dotterEnd)-1 + qoffset;

    printf("Calling dotter with query sequence region: %d - %d\n", dotterStart, dotterEnd);

    dotter(type, opts, dotterqname, queryseq, offset, dotterqname, dottersseq, offset,
	   0, 0, NULL, NULL, NULL, 0.0, dotterZoom, MSPlist, qoffset, 0, 0);
}


/* Allows user to set coord range for dotter.
 *
 * NOTE that currently the coord range is stored for the life of the blixem window
 * and must be flipped if the user flips the strand.
 *  */
static void setDotterParams(void)
{
  ACEIN params_in ;

  if (!*dotterqname)
    {
      if (!*qname_G)
	strcpy(dotterqname, "Blixem-seq");
      else
	strncpy(dotterqname, qname_G, LONG_NAMESIZE);

      dotterqname[LONG_NAMESIZE] = 0;
    }

  if (dotterStart == 0)
    dotterStart = dispstart - 100 ;
  if (dotterEnd == 0)
    dotterEnd = dispstart + 100 ;

  /* Flip coords to match strand setting if necessary. */
  if ((plusmin == 1 && dotterEnd < dotterStart)
      || (plusmin == -1 && dotterEnd > dotterStart))
    {
      int tmp ;
      
      tmp = dotterStart ;
      dotterStart = dotterEnd ;
      dotterEnd = tmp ;
    }

  if ((params_in = messPrompt ("Dotter parameters: zoom (compression) factor, "
			       "start, end, Queryname",
			       messprintf("%d %d %d %s", dotterZoom,
					  dotterStart+qoffset,
					  dotterEnd+qoffset,
					  dotterqname),
			       "iiiw", 0)))
    {
      aceInInt(params_in, &dotterZoom);

      aceInInt(params_in, &dotterStart);
      dotterStart -= qoffset;
      aceInInt(params_in, &dotterEnd);
      dotterEnd -= qoffset;

      /* Flip coords to match strand setting if necessary. */
      if ((plusmin == 1 && dotterEnd < dotterStart)
	  || (plusmin == -1 && dotterEnd > dotterStart))
	{
	  int tmp ;

	  tmp = dotterStart ;
	  dotterStart = dotterEnd ;
	  dotterEnd = tmp ;
	}

      strncpy(dotterqname, aceInWord(params_in), LONG_NAMESIZE);
      dotterqname[LONG_NAMESIZE] = '\0';

      aceInDestroy(params_in);

      smartDotter = 0;

      menuUnsetFlags(menuItem(blixemMenu, autoDotterParamsStr), MENUFLAG_DISABLED);
      graphNewMenu(blixemMenu);

      blviewRedraw();
    }

  return ;
}


static void autoDotterParams(void)
{
    smartDotter = 1;
    menuSetFlags(menuItem(blixemMenu, autoDotterParamsStr), MENUFLAG_DISABLED);
    graphNewMenu(blixemMenu);
}



/************************  BLIXEM SETTINGS  WINDOW  **************************/

static void blixemConfColourMenu(KEY key, int box)
{
  /* Taken from ? */
  int *colour;

  if (graphAssFind(assVoid(box+2000), &colour))
    {
      *colour = key;
      graphBoxDraw(box, BLACK, *colour);
      graphRedraw();
      blviewRedraw();
    }

  return ;
}


static void blixemConfColour(int *colour, int init, float x, float *y, int len, char *text)
{
    int box;
    if (text)
	graphText(text, x+len+1, *y);

    box = graphBoxStart();
    graphRectangle(x, *y, x+len, *y+1);
    graphBoxEnd();
    graphBoxFreeMenu(box, blixemConfColourMenu, graphColors);
    *colour = init;
    graphBoxDraw(box, BLACK, init);
    graphAssociate(assVoid(box+2000), colour);

    *y += 1.5;
}


static void buttonCheck(char* text, void (*func)(void), float x, float *y, int On)
{
  char *but_text ;

  but_text = hprintf(0, "%s %s", On ? "*" : " ", text) ;
  graphButton(but_text, func, x, *y);
  messfree(but_text) ;

  /* Could be more fancy like this
     if (On) graphFillRectangle(x-.5, *y, x-2, *y+1.2);
     else graphRectangle(x-.5, *y, x-2, *y+1.2);*/

  *y += 1.5;

  return ;
}


static void graphButtonDisable(char* text, float x, float *y, int On)
{
    int box = graphBoxStart();
    graphText(messprintf("%s %s", On ? "*" : " ", text), x, *y);
    graphBoxEnd();
    graphBoxDraw (box, DARKGRAY, WHITE);

    *y += 1.5;
}


static void setStringentEntropywin(char *cp)
{
    stringentEntropywin = atoi(cp);
    calcEntropyArrays(TRUE);
    blviewRedraw();
}
static void setMediumEntropywin(char *cp)
{
    mediumEntropywin = atoi(cp);
    calcEntropyArrays(TRUE);
    blviewRedraw();
}
static void setNonglobularEntropywin(char *cp)
{
    nonglobularEntropywin = atoi(cp);
    calcEntropyArrays(TRUE);
    blviewRedraw();
}



static void settingsPick(int box, double x_unused, double y_unused, int modifier_unused)
{
    if (box == stringentEntropybox) graphTextScrollEntry(stringentEntropytx,0,0,0,0,0);
    else if (box == mediumEntropybox) graphTextScrollEntry(mediumEntropytx,0,0,0,0,0);
    else if (box == nonglobularEntropybox) graphTextScrollEntry(nonglobularEntropytx,0,0,0,0,0);
}

static void settingsRedraw(void)
{
  float x1=1, x2=35, y;

  static MENUOPT sortMenu[] =
    {
      {sortByScore,      "Score"},
      {sortById,         "Identity"},
      {sortByName,       "Name"},
      {sortByPos,        "Position"},
      {0, 0}
    };


  if (!graphActivate(settingsGraph))
    return;

  graphClear();

    /* Background */
    graphBoxDraw(0, backgColor, backgColor);
    graphFitBounds (&nx, &ny);
    graphColor(backgColor); graphRectangle(0, 0, nx+100, ny+100);graphColor(BLACK);

    y  = 1;
    graphText("Toggles:", x1, y);
    y += 1.5;

    buttonCheck(BigPictToggleStr, BigPictToggle, x1, &y, BigPictON);
    if (blastn || blastx || tblastx) {
	if (BigPictON) {
	    buttonCheck(BigPictToggleRevStr, BigPictToggleRev, x1, &y, BigPictRev);
	    menuUnsetFlags(menuItem(settingsMenu, BigPictToggleRevStr), MENUFLAG_DISABLED);
	}
	else {
	    graphButtonDisable(BigPictToggleRevStr, x1, &y, BigPictRev);
	    menuSetFlags(menuItem(settingsMenu, BigPictToggleRevStr), MENUFLAG_DISABLED);
	}
    }
    buttonCheck(entropytoggleStr, entropytoggle, x1, &y, entropyOn);

    buttonCheck(toggleDESCStr, toggleDESC, x1, &y, DESCon);
    buttonCheck(squashFSStr, squashFSdo, x1, &y, squashFS);
    buttonCheck(squashMatchesStr, squashMatches, x1, &y, squash);
    buttonCheck(toggleIDdotsStr, toggleIDdots, x1, &y, IDdots);
    buttonCheck(printColorsStr, printColors, x1, &y, printColorsOn);
    buttonCheck(SortInvStr, sortToggleInv, x1, &y, sortInvOn);

/* Old disused toggles ...? */
/*    buttonCheck("Display colours", toggleColors, x1, &y, );*/
/*    buttonCheck("Printerphilic colours", printColors, x1, &y, printColorsOn);*/


    y = 1;
    graphText("Menus:", x2, y);
    y += 1.5;

    blixemConfColour(&backgColor, backgColor, x2, &y, 3, "Background colour");
    blixemConfColour(&gridColor, gridColor, x2, &y, 3, "Grid colour");
    blixemConfColour(&IDcolor, IDcolor, x2, &y, 3, "Identical residues");
    blixemConfColour(&consColor, consColor, x2, &y, 3, "Conserved residues");


    graphText("Sort HSPs by ", x2, y);
    graphBoxMenu(graphButton(messprintf("%s", sortModeStr), settingsRedraw, x2+13, y), sortMenu);
    y += 1.5;

    graphText("Fetch by ", x2, y);
    graphBoxMenu(graphButton(messprintf("%s", blxGetFetchMode()), settingsRedraw, x2+9, y), blxPfetchMenu()) ;
    y += 1.5;

    if (entropyOn) {
	graphText("Complexity curves:", x2, y);
	y += 1.5;

	sprintf(stringentEntropytx, "%d", stringentEntropywin);
	stringentEntropybox = graphTextScrollEntry (stringentEntropytx, 6, 3, x2+19, y, setStringentEntropywin);
	blixemConfColour(&stringentEntropycolor, stringentEntropycolor, x2, &y, 3, "window length:");

	sprintf(mediumEntropytx, "%d", mediumEntropywin);
	mediumEntropybox = graphTextScrollEntry (mediumEntropytx, 6, 3, x2+19, y, setMediumEntropywin);
	blixemConfColour(&mediumEntropycolor, mediumEntropycolor, x2, &y, 3, "window length:");

	sprintf(nonglobularEntropytx, "%d", nonglobularEntropywin);
	nonglobularEntropybox = graphTextScrollEntry (nonglobularEntropytx, 6, 3, x2+19, y, setNonglobularEntropywin);
	blixemConfColour(&nonglobularEntropycolor, nonglobularEntropycolor, x2, &y, 3, "window length:");
    }

    graphRedraw();


    return ;
}


static void blixemSettings(void)
{
    if (!graphActivate(settingsGraph)) {
	settingsGraph = graphCreate(TEXT_FIT, "Blixem Settings", 0, 0, .6, .3);
	graphRegister(PICK, settingsPick);
	settingsRedraw();
    }
    else graphPop();
}

/*
static void dotterPanel(void)
{
    graphCreate(TEXT_FIT, "Dotter Panel", 0, 0, .3, .1);
    graphButton("Dotter", callDotter, 1, 1);
    graphButton("Dotter HSPs only",callDotterHSPs, 1, 2.5);
    graphButton("Dotter query vs. itself",callDotterSelf, 1, 4);
    graphRedraw();
}
*/

/* Menu checkmarks */

static void menuCheck(MENU menu, int mode, int thismode, char *str)
{
  if (mode == thismode)
    menuSetFlags(menuItem(menu, str), MENUFLAG_TOGGLE_STATE);
  else
    menuUnsetFlags(menuItem(menu, str), MENUFLAG_TOGGLE_STATE);

  return ;
}


static void setMenuCheckmarks(void)
{
  menuCheck(settingsMenu, sortMode, SORTBYSCORE, SortByScoreStr);
  menuCheck(settingsMenu, sortMode, SORTBYID, SortByIdStr);
  menuCheck(settingsMenu, sortMode, SORTBYNAME, SortByNameStr);
  menuCheck(settingsMenu, sortMode, SORTBYPOS, SortByPosStr);
  menuCheck(settingsMenu, 1, sortInvOn, SortInvStr);
  
  menuCheck(settingsMenu, 1, BigPictON, BigPictToggleStr);
  menuCheck(settingsMenu, 1, BigPictRev, BigPictToggleRevStr);
  menuCheck(settingsMenu, 1, IDdots, toggleIDdotsStr);
  menuCheck(settingsMenu, 1, squash, squashMatchesStr);
  menuCheck(settingsMenu, 1, squashFS, squashFSStr);
  menuCheck(settingsMenu, 1, entropyOn, entropytoggleStr);
  menuCheck(settingsMenu, 1, printColorsOn, printColorsStr);
  menuCheck(settingsMenu, 1, colortoggle, toggleColorsStr);
  menuCheck(settingsMenu, 1, verbose, toggleVerboseStr);
  menuCheck(settingsMenu, 1, HiliteSins, toggleHiliteSinsStr);
  menuCheck(settingsMenu, 1, HiliteUpperOn, toggleHiliteUpperStr);
  menuCheck(settingsMenu, 1, HiliteLowerOn, toggleHiliteLowerStr);
  menuCheck(settingsMenu, 1, DESCon, toggleDESCStr);
  graphNewBoxMenu(settingsButton, settingsMenu);
  graphNewMenu(blixemMenu);

  return ;
}


/* Checks the MSP list of sequences for blixem to display to see if it contains sequence
 * data for each sequence (this may happen if acedb, for instance, starts blixem up and
 * acedb already contained all the sequences).
 * Returns TRUE if all the sequences are already there, FALSE otherwise.
 * Optionally returns the names of all the sequences that need to be fetched in dict if
 * one is supplied by caller. */
static BOOL haveAllSequences(MSP *msplist, DICT *dict)
{
  BOOL result = TRUE ;

  for (msp = msplist ; msp ; msp = msp->next)
    {
      if (!msp->sseq && msp->sname && *msp->sname && msp->score >= 0)
	{
	  result = FALSE ;
	  if (dict)
	    dictAdd (dict, msp->sname, 0) ;
	  else
	    break ;					    /* No dict so no need to carry on. */
	}
    }
  /* RD note: I would expect to have looked at msp->type above */

  return result ;
}



#ifdef ED_G_NEVER_INCLUDE_THIS_CODE

/* currently unused.... */
static void pfetchWindow (MSP *msp)
{
  ACEIN pipe;
  char  cmd[50+1] = "pfetch";
  char  title[50+1];
  char *lineBuf, *textBuf;
  STORE_HANDLE handle = handleCreate();

  sprintf(cmd, "pfetch --client=acedb_%s_%s -F '%s' &",
	  getSystemName(), getLogin(TRUE), msp->sname);
  sprintf(title, "pfetch: %s", msp->sname);

  pipe = aceInCreateFromPipe(cmd, "r", NULL, handle);
  if (pipe)
    {
      textBuf = strnew("", handle);
      aceInSpecial(pipe, "\n\t");

      while ((lineBuf = aceInCard(pipe)))
	  textBuf = g_strconcat(textBuf,"\n", lineBuf, NULL);

      /* call gexTextEditor with the pfetch output & no buttons */
      gexTextEditorNew(title,
		       textBuf,
		       0,
		       NULL,
		       NULL,          /* editorOK */
		       NULL,          /* editorCancel */
		       FALSE          /* set to readonly */
		       );
    }
  handleDestroy(handle);
}
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

/* Given a string, returns the string if is less than a fudge factor or returns
 * the string abbreviated in the form "xxxxx...yyyyy". The string is a pointer
 * to a static buffer, caller should make a copy of it if they wish to retain
 * it or alter it. */
static char *abbrevTxt(char *text, int max_len)
{
  char *result ;
  char *abbrev = "<>" ;
  static char *abbrev_buf = NULL ;
  static int buf_len, text_len, head_bytes, tail_bytes ;

  /* First time through allocate the reusable buffer, allocate larger one if required. */
  if (abbrev_buf == NULL)
    {
      buf_len = 50 ;					    /* not many sequence names this long. */
      abbrev_buf = (char *)messalloc(buf_len) ;
    }
  else if (max_len > (buf_len - 1))
    {
      messfree(abbrev_buf) ;
      buf_len = (max_len * 1.5) ;
      abbrev_buf = (char *)messalloc(buf_len) ;
    }


  result = abbrev_buf ;

  text_len = strlen(text) ;
  if (text_len <= max_len)
    {
      result = strcpy(result, text) ;
    }
  else
    {
      char *tail_ptr ;

      /* don't really need to calculate these each time... */
      head_bytes = (max_len / 2) - 1 ;
      tail_bytes = max_len - ((max_len / 2) + 1) ;

      tail_ptr = text + text_len - tail_bytes ;		    /* trailing null fudged in here. */

      result = strncpy(result, text, head_bytes) ;

      strcpy((result + head_bytes), "") ;

      result = strcat(result, abbrev) ;

      result = strcat(result, tail_ptr) ;
    }

  return (result) ;
}



static GtkToolItem *addToolbarWidget(GtkToolbar *toolbar, GtkWidget *widget)
{
  GtkToolItem *tool_item = NULL ;

  tool_item = gtk_tool_item_new() ;

  gtk_container_add(GTK_CONTAINER(tool_item), widget) ;

  gtk_toolbar_insert(toolbar, tool_item, -1) ;	    /* -1 means "append" to the toolbar. */

  return tool_item ;
}


static GtkToolItem *makeNewToolbarButton(GtkToolbar *toolbar,
					 char *label,
					 char *tooltip,
					 GtkSignalFunc callback_func)
{
  GtkToolItem *tool_button = NULL ;

  tool_button = gtk_tool_button_new(NULL, label) ;

  gtk_tool_item_set_homogeneous(tool_button, FALSE) ;

  gtk_tool_item_set_tooltip(tool_button, toolbar->tooltips,
			    tooltip, NULL) ;

  gtk_signal_connect(GTK_OBJECT(tool_button), "clicked",
		     GTK_SIGNAL_FUNC(callback_func), NULL);


  gtk_toolbar_insert(toolbar, tool_button, -1) ;	    /* -1 means "append" to the toolbar. */

  return tool_button ;
}


/* Print out MSP's, probably for debugging.... */
static void printMSPs(void)
{
  MSP *msp;

  for (msp = MSPlist; msp; msp = msp->next)
    {
#ifdef ACEDB
      if (msp->key)
	printf("%d %s ", msp->key, name(msp->key)) ;
      else
	printf("0 NULL_KEY") ;
#endif
      printf("%s %d %d %d %d %d %d :%s:\n",
	     msp->sname, msp->score, msp->id,
	     msp->qstart+qoffset, msp->qend+qoffset,
	     msp->sstart, msp->send, (msp->sseq ? msp->sseq : ""));
    }

  return ;
}


static void initBlxView(void)
{
  GraphGlobalCallbacksStruct graph_callbacks = {{NULL, NULL}} ;

  /* Init our general graph callbacks (Cntl-Q etc.). */
  graph_callbacks.help.func = (GraphCallBack)blxHelp ;
  graph_callbacks.print.func = (GraphCallBack)blxPrint ;
  graph_callbacks.quit.func = (GraphCallBack)blxDestroy ;

  graphRegisterGlobalCallbacks(&graph_callbacks) ;

  return ;
}


static void getStats(GString *result, MSP *MSPlist)
{
  int numMSPs = 0;          //count of MSPs
  int numSeqs = 0;          //count of sequences (excluding duplicates)
  gint32 totalSeqSize = 0;  //total memory used by the sequences
  GHashTable *sequences = g_hash_table_new(NULL, NULL); //remembers which sequences we've already seen
  
  for (msp = MSPlist; msp ; msp = msp->next)
    {
      ++numMSPs;
    
      if (!g_hash_table_lookup(sequences, msp->sseq))
        {
          ++numSeqs;
        if (msp->sseq)
          {
            totalSeqSize += strlen(msp->sseq) * sizeof(char);
          }
          
          g_hash_table_insert(sequences, msp->sseq, &msp->slength);
        }
    }
  
  g_hash_table_destroy(sequences);

  /* Create the text based on the results */
  g_string_printf(result, "%s%d%s%s%d%s%s%d%s%s%d%s%s%d%s%s%d%s",
                  "Length of reference sequence\t\t= ", qlen, " characters\n\n",
                  "Number of match sequences\t\t= ", numSeqs, "\n",
                  "Total memory used by sequences\t= ", totalSeqSize, " bytes\n\n",
		  "Number of MSPs\t\t\t\t\t= ", numMSPs, "\n",
                  "Size of each MSP\t\t\t\t\t= ", (int)sizeof(MSP), " bytes\n",
		  "Total memory used by MSPs\t\t= ", (int)sizeof(MSP) * numMSPs, " bytes");
}


static void showStatsDialog(MSP *MSPlist)
{
  /* Create a modal dialog widget with an OK button */
  GtkWidget *dialog = gtk_dialog_new_with_buttons("Statistics", 
                                                  GTK_WINDOW(blixemWindow),
						  GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
						  GTK_STOCK_OK,
						  GTK_RESPONSE_ACCEPT,
						  NULL);

  /* Ensure that the dialog box (along with any children) is destroyed when the user responds. */
  g_signal_connect_swapped (dialog, "response", G_CALLBACK(gtk_widget_destroy), dialog);

  /* Create a text buffer containing the required text*/
  GString *displayText = g_string_sized_new(200); //will be extended if we need more space
  getStats(displayText, MSPlist);
  GtkTextBuffer *textBuffer = gtk_text_buffer_new(gtk_text_tag_table_new());
  gtk_text_buffer_set_text(GTK_TEXT_BUFFER(textBuffer), displayText->str, -1);
  g_string_free(displayText, TRUE);

  /* Create a text view widget and put it in the vbox area of the dialog */
  GtkWidget *textView = gtk_text_view_new_with_buffer(GTK_TEXT_BUFFER(textBuffer));
  gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->vbox), textView, TRUE, TRUE, 0);
  
  /* Show the dialog */
  gtk_widget_show(textView);
  gtk_widget_show(dialog);
}


/* Pop up a dialog reporting on various statistics about the process */
static void blxShowStats(void)
{
  showStatsDialog(MSPlist);
}


/***************** end of file ***********************/
