/* 
   belvu.c - Pretty colour viewing of multiple alignments

-------------------------------------------------------------
|  File: belvu.c                                            |
|  Author: Erik.Sonnhammer@sbc.su.se                        |
|  Copyright (C) E Sonnhammer                               |
-------------------------------------------------------------

  Date      Modification
--------  ---------------------------------------------------
94-08-06  Created

95-01-23  Removed multiple windows - call xfetch instead.
          Made sure all slx & mul formats are read properly
	  Built-in ruler
95-04-25  Changed internal storage system into an array of ALN structs.
          This allowed direct selex support and any size of alignment.
95-04-28  Added MSF support, highlighting of whole line and new conserved
          residue colouring.
95-05-01  Changed Conservation Colouring to work by colorMap matrix.
95-05-08  Added 'Draw Wrapped Alignment'
95-08-25  [1.4] Added "Make non-redundant" and "Remove highlighted sequence".
95-08-29  [1.4] Added "Remove columns".
95-09-21  [1.4] Added "Remove columns that are all-gaps"
95-10-10  [2.0] Completely rewrote layout and made own scrollbars.
95-10-23  [2.1] Re-implemented "average similarity" coloring.  This has the effect
          over %ID that columns with many different types of similar residues get
	  higher scores, while columns with some predominant residue type will get
	  lower scores if the other residues are very different.  There is a 
	  fudge-factor of 2 for Blosum62, which is totally ad-hoc.  In the old 
	  implementation it was set to 4 (lowest Blosum62 diagonal) but that is too 
	  weak.  With a factor of 2, the highest values becomes 550% (W-W)...
	  Whether this is better than %ID God knows.
95-10-25  [2.2] Added optional column with scores.
          [2.2] Added 'Ask to save' if modified.
95-10-30  [2.2] Added -t Title
          [2.2] Added Centering with middle button.  NOTE: graphBoxShift was too slow.
	  [2.2] Replaced lastboxAbs with highlight() - lights up all boxes of protein.
	  [2.2] Added checkmarks in menu and fixed Toby's colours.
95-11-06  [2.2] Fixed bug that lost Highlight_alnp when re-sorting alignment.
95-11-10  [2.2] Window sized to fit small alignments.
95-12-11  [2.2] Added "Add sequences with matching segments".
96-01-25  [2.2] Fixed -m crashes by actually writing countInserts() (had forgotten).
96-03-08  [2.2] Fixed bug that anticipated '*' to mean 'any insertion'.
96-03-21  [2.2] Enabled efetching when double clicking in alignment.
                Fixed unwanted scrolling when clipping to the left.
		Added 'Remove outliers'.
		Same directory-buffer for all I/O operations.
96-04-01  [2.3] Adapts to any name length up to 50 residues.
                Added "Only colour residues above %id threshold".
		Added "Save current colour scheme to file".
96-05-20  [2.3] "Use gray shades (for printing)" with reverse video.
96-08-23  [2.3] Improved WWW efetch.
96-08-28  [2.4] Made conservation colours and cutoffs changeable.
96-11-19  [2.4] Added "Read labels of highlighted sequence and spread them".
97-03-12  [2.4] Added "Ignore gaps in conservation levels".
97-03-18  [2.4] Removed dirName and fileName from filqueryopen to avoid crashes.
97-06-01  [2.4] Changed setConservColors() for color_by_similarity to not count self-pair residues. 
                This led to ghost conservation for e.g. single C's and H's at small samples (<~3).
97-07-29  [2.5] Added tree-construction, tree-sorting and tree-drawing.
                Changed toolbar to more microsoft'ish look and rephrased various options.
		Fixed bug that would screw up color schemes when changing coloring mode with 
		  color codes window up.  Tried to make it blocking but got strange crashes. 
		  The graph libarary does not like to redraw other windows from a blocking 
		  window - gets confused about which one to block.  Solved by temporarily 
		  changing to harmless colour menu, which prevents changing mode.
		Added 'Plot conservation profile'.
		Added conservation values to '-c Print Conservation table'.
97-10-06  [2.5] Added '-' as legal gap character (in addition to '.').
                Added toggle "Automatically remove all-gap columns after sequence removals"
		Added pickable 'markup' sequences, for consensus or markup lines.
97-12-18  [2.6] Added hiding/unhiding of picked line.
97-12-30  [2.7] Added MSF writing and multiple-choice of output formats on command line.
                Added "Save As..." tool and changed save routines to use the global variables
		saveCoordsOn, saveSeparator, and saveFormat for the format details.
		Added parsing of aligned Fasta.  Discontinued ProDom parsing support 
		in favor of Aligned Fasta format. 
97-12-30  [2.7] Fixed bug so negative scores can be read in.
                Added command-line "make non-redundant".
99-03-02  [2.7] Added "Remove partial sequences".
                Fixed bug in rmOutliers that skipped one seq after a deletion.
99-03-02  [2.7] Added "Remove gappy sequences" and "Remove gappy columns".
99-06-05  [2.8] Added support for Stockholm format GR and GC markup lines.
                (In the belvu code, Stockholm format is still called MUL).
99-06-23  [2.8] Added "Sort by similarity/identity to highlighted sequence".
99-08-05  [2.8] Added "Print tree in New Hampshire format".
99-08-08  [2.8] Code changes in sorting routintes so that #=GR markup sticks 
                to the sequence line it belongs to (markups are removed before
                sorting by separateMarkupLines() and put back in the right place 
                afterwards by reInsertMarkupLines().
                Added commandline option to sort by similarity to 1st sequence.
99-10-26  [2.8] Changed tree calculation and drawing to handle multiple trees 
                simultaneously that can still be printed.  Moved tree line width to 
		tree option (treesettings) control window.
00-01-24  [2.8] Added Collapse functionality to Wrap-around display (residues between [])
                (Should change this to a markup-line, much easier)
                Removed right-hand coordinate in wrap-around display.
00-01-25  [2.8] Added "select gap character".
00-03-03  [2.8] Fixed bug in parseMul so that seqnames can contain '-'.
00-03-06  [2.8] Changed code for foreground coloring to sync normal and wrapped displays.
00-03-21  [2.8] Fixed bug in Tree-mode selection from command line (string didn't change).
00-04-27  [2.9] Center on highlighted after sorting.
00-05-02  [2.9] Show Highlighted in tree.
00-05-15  [2.9] Color tree by organism.
                User-choosable colors (from color setup file).
		Default tree color scheme with decent colors.
		Reroot tree by clicking.
		"Find orthologs option in tree - marks up nodes with 
		different left->org and right->org.
00-05-20  [2.9] Print out list of found orthologs.
00-05-23  [2.9] Added back dirName and fileName to filqueryopen - what was the problem?
                Also added default file name = the original file name.
		Reroot tree on "best balanced" node.
00-08-04  [2.9] Added output tree only in batch mode.
		Fixed BUG: "sort by tree" -> tree highlighting 
		finds wrong sequence in alignment.
00-08-10  [2.9] Added batch-mode generation of bootstrapped trees.
                Added trees without sequence coordinates.
00-08-16  [2.9] Fixed bugs in treeFindBalance and treeSize3way that chose the wrong root.
                     - treeSize3way had bugs in the branchlength selection when going up parents.
                     - treeFindBalance had bugs that ameliorated bugs in treeSize3way.
		     - treeFindBalance now chooses "best subtree balance" tree when more
		       than one rooting give perfect balance.
00-08-16  [2.9] Fixed bug in bootstrapping (the original wasn't kept separately).
00-10-19  [2.9] Added "core name" (without coordinates) parsing of #=GS OS lines.
00-10-19  [2.9] Fixed bug in 'find orthologs': if the node was originally the root, the 
                organism spreading was incorrect.  Fix: spread organisms to higher nodes
		after the tree is calculated.
	        Added average conservation to "Plot conservation" window.
00-10-25  [2.9] Added "Show Organisms" window.
                Changed mknonredundant to remove only inclusive matches.
00-11-13  [2.9] Fixed bug in #=GS OS parsing (messed up the original).
00-12-11  [2.9] Added batch mode "Remove Gappy Columns".
01-09-11 [2.10] Fixed bugs in sequence coordinate handling.
01-11-28 [2.11] Added Sonnhammer-Storm distance correction.
01-12-04 [2.12] Fixed a bug in parsing alignments without coordinates (from 2.10).
                Added treeScale to scale tree according to mode (1.0 normal, 0.3 distcorr).
02-03-22 [2.13] Added reading of user-choosable label for organism info (e.g. OC).
02-04-14 [2.14] Fixed small but segfaulting bug in stripCoordTokens.
02-05-24 [2.15] Added "Show organism" in the tree.
02-07-08 [2.16] Converted to GTK and ACEIN.
02-10-11 [2.17] Added tree-distance from BLOSUM62 (Scoredist)
03-01-29 [2.18] Added "Highlight lowercase characters"
                Added "Print UPGMA-based subfamilies"
03-01-29 [2.19] Fixed bug that prevented bootstrapping from working (stdlib.h needed to be included
                      but there was a clash in mystdlib.h on qsort.  Hand-edited that away!
03-05-07 [2.20] Fixed bug in treeDrawing that scaled the distances when the tree was scaled(!)
                      In particular with STORMSONN correction this gave very bogus distances.
03-11-07 [2.21] Added "Print distance matrix and exit" option
03-12-04 [2.22] Added "Read distance matrix instead of alignment" option
04-01-02 [2.23] Added automatic removal of empty sequences after column removal. (Changed some
                common routines - may have introduced new bugs!).
		- Fixed bug that changed coordinates in wrong direction for reversed coordinates.
		(start > end).
		- Added BELVU_FETCH variable for configurable sequence fetching.
04-01-19 [2.24] Fixed bug in score(), which read the BLOSUM62 table wrong !! (-1 offset in a2b[] missing).
                Added -G which penalizes one-sided gaps in identity() and score(): in identity() they
                are counted as mismatches while in score() they are counted according to the matrix (-4).
		Added Kimura distance correction (adapted from Lasse Arvestad).
		Changed treeScoredist distance to logarithmic function.
		Added treeJukesCantor distance.
04-01-27 [2.25] Fixed serious bug in score().
                Changed gap penalty in score() to -0.6 for Scoredist
		after a suggestion by Timo Lassmann.  (-4 is not a gap penalty!)
04-02-27 [2.26] Fixed some scaling problems with various tree distance methods.
                Added distance method to tree window frame.
                Also put a PAM300 limit to JukesCantor and StormSonnhammer.
                Added the 1.34 magic factor to Blosum tree distances
04-04-05 [2.27] Temporary fix to stop Belvu from segfaulting under linux (BUG20):
                Raised initial size of Align array to 100000.
	(Bug reported by Jerome Gracy, Kimmen, and Nicolas Hulo)
04-11-11 [2.27] Name change BLOSUM to Scoredist, and max value limited to 300 PAM.
05-01-12 [2.28] Fixed bug in "remove outliers" that failed to detect outlier in last row.
	Fixed bug so that "-l" startup shows the colors (like choosing "last palette").
	Added "Ignore gaps in conservation calculation" as startup option.
05-08-30 [2.29] Changed "Print distance matrix" routine to print sequences coordiantes by default.
              Fixed segfaults bug when clicking "colour" in color_by_residue mode.
07-02-06 [2.30] Changes to compile under ACEDB.4.9.32:
                       - gexTextEditorNew parameters
	         - graphButton called with static text strings (crashed otherwise)
	         Fixed memory leak in treeScoredist (noticeable when bootstrapping trees).
07-02-06 [2.31] Fixed segfault problem with Scoredist in case of non-overlapping sequences.
                Changed logic of tree options so that starting with tree window is independent.
07-09-13 [2.33] Added launch Tree options from tree (useful if no alignment).
                Added nodeswapping upon click.
                Added menuitem in tree menu to select swap/reroot when clicking.
09-05-04 [2.35] Fixed bug in main() that closed pipe before treeReadDistances().
                Fixed bug in readColorCodes() that used unallocated alnp to hold data.


    Pending:

        Internal bootstrapping.
	Rationale for basic bootstrapping:
	     0. struct Subtree {nodep, seqstring}.  nodep points to node in orig tree, seqnamestring a sorted concatenation of seqence ordinal numbers
	     1. Traverse original tree from root to 2-seq nodes, insert Subtree structs at each node into a list (sorted on seqstring)
	     2. For each Boostrap tree {
	         Traverse tree, for each internal node except root: if seqstring found in Subtreelist, increment node's boostrap counter
	     }
	        Traverse bootstrap and original tree
	Rationale for group bootstrapping:
		   For each sequence, store array of sequences in merging order in bootstrap tree.
		   For each node in original tree, check if group of sequences 
		   corresponds to the first n sequences in bootstraptree array.
	Rationale for exact bootstrapping:
		   Traverse bootstrap tree from leaf to root:
		      oritree nodes = {same, different, 0}
		      if issequence(parent->other_child) {
		         if (parent->other_child == boottree(parent->other_child))
			    parent = same
			 else
			    parent = different
		      if isseen(parent->other_child) {
		         if (parent->other_child == boottree(parent->other_child))
			    parent = same
			 else
			    parent = diferent
		      if same, continue to parent

	Clean up the parsing code with str2aln calls.

	read in other trees with bootstraps
	use confidence cutoff in find orthologs
	
        make alignment collapsing easier to use, see above.

        Keyboard edit of one sequence.  How to find the right place ? !!

        Undo edits.

	Would like to abort parsing if two sequence lines accidentally have same name -
	How? - this is used in selex format...

	Koonin ideas:
	    Consensus pattern lines (Default 0% and 100% lines, maybe 90% too).
	    Add/remove any nr of lines cutoffs 0 - 100%.
	    Tool to define groups of residues and corresponding symbol.  These
	    are shown on pattern lines (priority to smaller groups).

	Low priority:

	   "Add sequences with matching segments" for more than one sequence.
	    Will probably be very deleterious to the alignment.

	    Ability to choose both foreground and background color - makes it twice
	    as slow - see "Black/white for printing.

 * HISTORY:
 * Last edited: May 28 11:43 2008 (edgrif)
 * Created: Aug 03 1994 (Erik.Sonnhammer@cgb.ki.se)
 * CVS info:   $Id: belvu.c,v 1.15 2009-05-06 14:02:42 esr Exp $
*/


/*  Description of color_by_similarity algorithm in setConservColors():

    ( Corresponds to summing all pairwise scores)

    for each residue i {
        for each residue j {
	    if (i == j) 
	        score(i) += (count(i)-1)*count(j)*matrix(i,j)
	    else
	        score(i) += count(i)*count(j)*matrix(i,j)
	    }
	}

	if (ignore gaps)
	    n = nresidues(pos)
	else 
	    n = nsequences
		    
	if (n == 1)
	    id = 0.0
	else
	    id = score/(n*(n-1))
    }
		
*/

#include <stdarg.h>
/*#include <stdlib.h> / * Needed for RAND_MAX but clashes with other stuff */
#include <sys/types.h>
#include <sys/time.h>
#include <ctype.h>
#include <time.h>

#include <wh/regular.h>
#include <wh/aceio.h>
#include <wh/graph.h>
#include <wh/gex.h>
#include <wh/key.h>
#define BELVU						    /* Fix horrible include mess in dotter_.h */
#include <wh/dotter_.h>
#include <wh/menu.h>


#define MAXLINE   512
#define MAXLENGTH 100000
#define MAXNAMESIZE  256
#define boxColor  WHITE /* LIGHTGRAY */
#define DBL_CLK_DELAY 2 /* seconds */
#define HSCRWID    1.0  /* Width of Horizontal scroll bar */
#define SCRBACKCOLOR PALEGRAY
#define NOCOLOR -1
#define NN NOCOLOR
#define BG WHITE


#define myGraphDestroyDeclare(FUNCNAME)        \
  static void FUNCNAME(void) ;

#define myGraphDestroy(FUNCNAME, FUNCGRAPH)    \
static void FUNCNAME(void)                     \
{                                              \
  messAssert(graphActive() == FUNCGRAPH) ;     \
                                               \
  graphDestroy() ;                             \
                                               \
  FUNCGRAPH = GRAPH_NULL ;                     \
                                               \
  return ;                                     \
}




enum { MUL, RAW };		/* Input formats for IN_FORMAT */
enum { dummy, GF, GC, GS, GR };	/* Markup types in Stockholm format */
enum { UNCORR, KIMURA, STORMSONN, Scoredist, JUKESCANTOR}; /* Distance correction for tree building */


typedef struct alnStruct {
    char name[MAXNAMESIZE+1];
    int  start;
    int  end;
    int  len;			/* Length of this sequence - Do not use!  Use maxLen instead ! */
    char *seq;
    int  nr;			/* Ordinal number in array - must be sorted on this */
    char fetch[MAXNAMESIZE+11];
    float score;
    int  color;			/* Background color of name */
    int  markup;		/* Markup line */
    int  hide;			/* Hide this line */
    int  nocolor;		/* Exclude from coloring */
    char *organism;
} ALN;

typedef struct segStruct {
    int  start;
    int  end;
    int  qstart;
    int  qend;
    struct segStruct *next;
} SEG;

typedef struct treenode {

    /* KEEP IN SYNC WITH TREECPY !!!! */

    float dist;			/* Absolute distance position */
    float branchlen;		/* Length of branch to higher node */
    float boot;	/* Bootstrap value */
    struct treenode *left;
    struct treenode *right;
    struct treenode *parent;
    char *name;
    char *organism;
    ALN *aln;
    int box;
    int color;

    /* KEEP IN SYNC WITH TREECPY !!!! */

} treeNode;


typedef struct treestruct {
   treeNode *head;
   int lastNodeBox;		/* Last box used by a node - name boxes come next */
   int currentPickedBox;
} treeStruct;


typedef struct bootstrapgroup {   /* To store bootstrap group */
    treeNode *node;    /* Points to node in original tree (for incrementing) */
    char *s;       /* Sorted concatenation of all sequences in node, to be inserted in list */
} BOOTSTRAPGROUP;

static char b2a[] = ".ARNDCQEGHILKMFPSTWYV" ;

static char *colorNames[NUM_TRUECOLORS] = {
    "WHITE", 
    "BLACK", 
    "LIGHTGRAY", 
    "DARKGRAY",
    "RED", 
    "GREEN", 
    "BLUE",
    "YELLOW", 
    "CYAN", 
    "MAGENTA",
    "LIGHTRED", 
    "LIGHTGREEN", 
    "LIGHTBLUE",
    "DARKRED", 
    "DARKGREEN", 
    "DARKBLUE",
    "PALERED", 
    "PALEGREEN", 
    "PALEBLUE",
    "PALEYELLOW", 
    "PALECYAN", 
    "PALEMAGENTA",
    "BROWN", 
    "ORANGE", 
    "PALEORANGE",
    "PURPLE", 
    "VIOLET", 
    "PALEVIOLET",
    "GRAY", 
    "PALEGRAY",
    "CERISE", 
    "MIDBLUE"
};

static int treeColors[16] = {
    RED, 
    BLUE,
    DARKGREEN, 
    ORANGE, 
    MAGENTA,
    BROWN, 
    PURPLE, 
    CYAN, 
    VIOLET, 
    MIDBLUE,
    CERISE, 
    LIGHTBLUE,
    DARKRED, 
    GREEN, 
    DARKBLUE,
    GRAY
};

static void     externalCommand();

/* Global variables *******************/

enum { COLORBYRESIDUE, COLORSCHEMESTANDARD, COLORSCHEMEGIBSON, COLORSCHEMECYS, 
COLORSCHEMEEMPTY, COLORSIM, COLORID, COLORIDSIM }; /* Modes - used for checkmarks */

enum { NOLL, SORT_ALPHA, SORT_ORGANISM, SORT_TREE, SORT_SCORE, SORT_SIM, SORT_ID }; /* Initial sorting modes */

enum {UPGMA, NJ};		/* Tree building methods */

enum {NODESWAP, NODEROOT};		/* Tree picking methods */

static int    **conservCount=0,	   /* Matrix of conservation values  - 21 x maxLen */
              **colorMap=0,	   /* Matrix of conservation colours - 21 x maxLen */
               *conservResidues=0, /* Array of number of residues present in each column */

                colorScheme = COLORSIM,  /* Current colour scheme mode (used for checkmarks) */
                color_by_conserv = 1,    /* 1 => by id or similarity; 0 => by residue  */
                id_blosum=1,	         /* Paint similar residues too */
                color_by_similarity = 1, /* 0 => by id */


                maxLen=0,	/* Columns in alignment */
                nseq=0,		/* Rows in alignment */
                verbose=0,
                debug=0,
                xmosaic = 0,
                ip,		/* Int pointer used in array operations */
                init_sort=0, 
                init_rmPartial=0, 
                rmPickLeftOn,
                pickedCol = 0,	/* Last picked column */
                colorRectangles=1,
                maxNameLen,	/* Max string length of any sequence name */
                maxStartLen=0,	/* Max string length of any sequence start */
                maxEndLen=0,	/* Max string length of any sequence end */
                maxScoreLen=0,	/* Max string length of any sequence score */
                AlignXstart=0,	/* x Start position of alignment in alignment window */
                AlignYstart=0,	/* y Start position of alignment in alignment window */
                Alignwid,	/* Witdh of shown alignment */
                Alignheig,	/* Height of shown alignment */
                statsBox,
                HscrollSliderBox,
                VscrollSliderBox,
                scrLeftBox,
                scrRightBox,
                scrUpBox,
                scrDownBox,
                scrLeftBigBox,
                scrRightBigBox,
                scrUpBigBox,
                scrDownBigBox,
                colorButtonBox,
                sortButtonBox,
                editButtonBox,
                maxBox,
                midBox,
                lowBox,
                gridOn = 0,
                IN_FORMAT = MUL,
                saved = 1,
                displayScores = 0,
                Vcrosshair,
                Highlight_matches,
                colorByResIdOn = 0, /* colour by residue type above identity cutoff */
                matchFooter = 0,
                maxfgColor = BLACK,
                midfgColor = BLACK,
                lowfgColor = BLACK,
                maxbgColor = CYAN,
                midbgColor = MIDBLUE,
                lowbgColor = LIGHTGRAY,
                oldmaxfgColor,
                oldmidfgColor,
                oldlowfgColor,
                oldmaxbgColor,
                oldmidbgColor,
                oldlowbgColor,
                printColorsOn = 0,
                wrapLinelen,
                ignoreGapsOn = 0,
                treeOrderNr,	/* Tree node number */
                conservationWindow=1,
                rmEmptyColumnsOn=1,
                saveCoordsOn=1,
                treeMethod,
/*                treeMethod = NJ, my code....should this be initialised ? */
                maxTreeWidth = 0,
                treeColorsOn = 1,
                treeShowBranchlen = 0,
                treeShowOrganism = 1,
                treeCoordsOn = 1,
                treebootstraps = 0, /* Number of bootstrap trees to be made */
	outputBootstrapTrees = 0,  /* Output the individual bootstrap trees */
                treebootstrapsDisplay = 0,  /* Display bootstrap trees on screen */
                stripCoordTokensOn = 1,
                lowercaseOn = 0,
                treePrintDistances = 0,
                treeReadDistancesON = 0,
                penalize_gaps = 0;

static float    Xpos,		/* x screen position of alignment window */
                Aspect,		/* Aspect ratio of coordinate system (width/height) */
                VSCRWID,	/* Width of Vertical scroll bar - depends on Aspect */
                HscrollStart,	/* Start of legal Horizontal scrollbar slider area */
                HscrollEnd,     /* End of -''- */
                VscrollStart,	/* as above */
                VscrollEnd,	/* as above */
                HsliderLen,     /* Length of Horizontal slider */
                VsliderLen,     /* Length of Vertical slider */
                SliderOffset,
                colorByResIdCutoff = 20.0, /* Colour by residue + id cutoff */
                lowIdCutoff = 0.4,	/* %id cutoff for lowest colour */
                midIdCutoff = 0.6,	/* %id cutoff for medium colour */
                maxIdCutoff = 0.8,	/* %id cutoff for maximum colour */
                lowSimCutoff = 0.5,	/* %id cutoff for lowest colour */
                midSimCutoff = 1.5,	/* %id cutoff for medium colour */
                maxSimCutoff = 3.0,	/* %id cutoff for maximum colour */
                oldMax,
                oldMid,
                oldLow,
                screenWidth,
                screenHeight,
                fontwidth, fontheight,
                tree_y, treeLinewidth = 0.3,
                conservationLinewidth = 0.2,
               *conservation,	        /* The max conservation in each column [0..maxLen] */
                conservationRange,
                conservationXScale=1,
                conservationYScale=2,
                oldlinew,
                treeBestBalance,
                treeBestBalance_subtrees,
                treeDistCorr,
                treePickMode,
                treeScale,
                mksubfamilies_cutoff = 0.0;

static char     belvuVersion[] = "2.35",
                line[MAXLENGTH+1],  /* General purpose array */
                Title[256] = "",    /* Window title */
                *ruler=0,	    /* Residue ruler on top */
                stats[256],	    /* Status bar string */
                dirName[DIR_BUFFER_SIZE+1], /* Default directory for file browser */
                fileName[FIL_BUFFER_SIZE+1],
                maxText[11],	/* Cutoff text arrays for boxes */
                midText[11],
                lowText[11],
                saveFormat[50],
                treeMethodString[50],
                treeDistString[50],
                treePickString[50],
                saveSeparator='/',
                gapChar='.',
                *OrganismLabel = "OS";

static Graph    belvuGraph = GRAPH_NULL, showColorGraph = GRAPH_NULL, treeGraph = GRAPH_NULL, 
                conservationGraph = GRAPH_NULL, saveGraph = GRAPH_NULL, annGraph = GRAPH_NULL, 
                treeGUIGraph = GRAPH_NULL, gapCharGraph = GRAPH_NULL, organismsGraph = GRAPH_NULL ;

static Stack    stack ;
static Stack    AnnStack ;
static Array    Align, MarkupAlign, organismArr, bootstrapGroups;

static ALN      aln,	     /* General purpose ALN */
               *alnp,        /* General purpose ALNP */
               *Highlight_alnp=0;

static treeStruct *treestruct;	/* General purpose treeStruct */
static treeNode   *treeHead,	/* Global current treehead */
                  *treeBestBalancedNode;
static FILE    *treeReadDistancesPipe;

ACEIN ace_in = NULL ;

static int stripCoordTokens(char *cp);
static void readColorCodesMenu(void);
static void readColorCodes(FILE *file, int *colorarr);
static void showColorCodesRedraw(void);
static void saveColorCodes(void);
static void belvuRedraw(void);
static void colorSchemeStandard(void);
static void colorByResidue(void);
static void colorSchemeGibson(void);
static void colorSchemeCys(void);
static void colorSchemeEmpty(void);
static void colorId(void);
static void colorIdSim(void);
static void colorSim(void);
static void colorRect(void);
static void xmosaicFetch(void);
static void saveAlignment(void);
static void saveMul(void);
static void saveFasta(void);
static void saveMSF(void);
static void saveRedraw(void);
static void saveMSFSelect(void);
static void saveMulSelect(void);
static void saveFastaAlnSelect(void);
static void saveFastaSelect(void);
static void treeUPGMAselect(void);
static void treeNJselect(void);
static void treeSWAPselect(void);
static void treeROOTselect(void);
static void treeUNCORRselect(void);
static void treeKIMURAselect(void);
static void treeJUKESCANTORselect(void);
static void treeSTORMSONNselect(void);
static void treeScoredistselect(void);
static void treeBootstrap(void);
static void treeBootstrapStats(treeNode *tree);
static void wrapAlignmentWindow(void);
static void mkNonRedundant(float cutoff);
static void mkNonRedundantPrompt(void);
static void rmOutliers(void);
static void rmPartial(void);
static void rmGappySeqs(float cutoff);
static void rmGappySeqsPrompt(void);
static void readLabels(void);
static void selectGaps(void);
static void listIdentity(void);
static float identity(char *s1, char *s2);
static void rmPicked(void);
static void rmPickLeft(void);
static void Help(void);
static void rmColumnPrompt(void);
static void rmColumnCutoff(void);
static void rmColumnLeft(void);
static void rmColumnRight(void);
static void rmEmptyColumns(float cutoff);
static void rmEmptyColumnsInteract(void);
static void rmScore(void);
static void alphaSort(void);
static void simSort(void);
static void idSort(void);
static void organismSort(void);
static void scoreSortBatch(void);
static void scoreSort(void);
static void treeSettings(void);
static void treeDisplay(void);
static void treeSortBatch(void);
static void treeSort(void);
static void printScore(void);
static void belvuMenuDestroy(void);
static void belvuDestroy(void);
static void readMatch(FILE *fil);
int  findCommand (char *command, char **retp);
static void colorByResId(void);
static void printColors (void);
static void ignoreGaps (void);
static void finishShowColorOK(void);
static void finishShowColorCancel(void);
static void initResidueColors(void);
static void initMarkupColors(void);
static int  isAlign(char c);
static int  isGap(char c);
static void treePrintNH_init(void);
static void treeFindOrthologs(void);
static void treeSetLinewidth(char *cp);
static void treeSetScale(char *cp);
static void conservationPlot(void);
static void conservationSetWindow(void);
static void conservationSetLinewidth(void);
static void conservationSetScale(void);
static void rmEmptyColumnsToggle(void);
static void markup(void);
static void hide(void);
static void unhide(void);
static float score(char *s1, char *s2);
static void arrayOrder(void);
static void arrayOrder10(void);
static void highlightScoreSort(char mode);
static int bg2fgColor (int bkcol);
static void alncpy(ALN *dest, ALN *src);
static int alignFind( Array arr, ALN *obj, int *ip);
static void centerHighlighted(void);
static void str2aln(char *src, ALN *alnp);
static treeNode *treeReroot(treeNode *node);
static void treeSwapNode(treeNode *node);
static void treeDraw(treeNode *treeHead);
static treeNode *treecpy(treeNode *node);
static void treeTraverse(treeNode *node, void (*func)());
static void treeBalanceByWeight(treeNode *node1, treeNode *node2, float *llen, float *rlen);
static void treeReadDistances(float **pairmtx);
static float treeSize3way(treeNode *node, treeNode *fromnode);
static void showOrganisms(void);
static void parseMulLine(char *line, ALN *aln);
static void lowercase(void);
static void rmFinaliseGapRemoval(void);
static void graphButtonCheck(char* text, void (*func)(void), float x, float *y, int On);


myGraphDestroyDeclare(belvuDestroy) ;
myGraphDestroyDeclare(treeDestroy) ;

/* Residue colors */
static int color[] = {
        NN,NN,NN,NN,NN,NN,NN,NN,NN,NN,NN,NN,NN,NN,NN,NN,
        NN,NN,NN,NN,NN,NN,NN,NN,NN,NN,NN,NN,NN,NN,NN,NN,
        NN,NN,NN,NN,NN,NN,NN,NN,NN,NN,NN,NN,NN,NN,NN,NN,
        NN,NN,NN,NN,NN,NN,NN,NN,NN,NN,NN,NN,NN,NN,NN,NN,
        NN,NN,NN,NN,NN,NN,NN,NN,NN,NN,NN,NN,NN,NN,NN,NN,
        NN,NN,NN,NN,NN,NN,NN,NN,NN,NN,NN,NN,NN,NN,NN,NN,
        NN,NN,NN,NN,NN,NN,NN,NN,NN,NN,NN,NN,NN,NN,NN,NN,
        NN,NN,NN,NN,NN,NN,NN,NN,NN,NN,NN,NN,NN,NN,NN,NN,

        NN,NN,NN,NN,NN,NN,NN,NN,NN,NN,NN,NN,NN,NN,NN,NN,
        NN,NN,NN,NN,NN,NN,NN,NN,NN,NN,NN,NN,NN,NN,NN,NN,
        NN,NN,NN,NN,NN,NN,NN,NN,NN,NN,NN,NN,NN,NN,NN,NN,
        NN,NN,NN,NN,NN,NN,NN,NN,NN,NN,NN,NN,NN,NN,NN,NN,
        NN,NN,NN,NN,NN,NN,NN,NN,NN,NN,NN,NN,NN,NN,NN,NN,
        NN,NN,NN,NN,NN,NN,NN,NN,NN,NN,NN,NN,NN,NN,NN,NN,
        NN,NN,NN,NN,NN,NN,NN,NN,NN,NN,NN,NN,NN,NN,NN,NN,
        NN,NN,NN,NN,NN,NN,NN,NN,NN,NN,NN,NN,NN,NN,NN,NN
};

/* Residue colors */
static int markupColor[] = {
        BG,BG,BG,BG,BG,BG,BG,BG,BG,BG,BG,BG,BG,BG,BG,BG,
        BG,BG,BG,BG,BG,BG,BG,BG,BG,BG,BG,BG,BG,BG,BG,BG,
        BG,BG,BG,BG,BG,BG,BG,BG,BG,BG,BG,BG,BG,BG,BG,BG,
        BG,BG,BG,BG,BG,BG,BG,BG,BG,BG,BG,BG,BG,BG,BG,BG,
        BG,BG,BG,BG,BG,BG,BG,BG,BG,BG,BG,BG,BG,BG,BG,BG,
        BG,BG,BG,BG,BG,BG,BG,BG,BG,BG,BG,BG,BG,BG,BG,BG,
        BG,BG,BG,BG,BG,BG,BG,BG,BG,BG,BG,BG,BG,BG,BG,BG,
        BG,BG,BG,BG,BG,BG,BG,BG,BG,BG,BG,BG,BG,BG,BG,BG,

        BG,BG,BG,BG,BG,BG,BG,BG,BG,BG,BG,BG,BG,BG,BG,BG,
        BG,BG,BG,BG,BG,BG,BG,BG,BG,BG,BG,BG,BG,BG,BG,BG,
        BG,BG,BG,BG,BG,BG,BG,BG,BG,BG,BG,BG,BG,BG,BG,BG,
        BG,BG,BG,BG,BG,BG,BG,BG,BG,BG,BG,BG,BG,BG,BG,BG,
        BG,BG,BG,BG,BG,BG,BG,BG,BG,BG,BG,BG,BG,BG,BG,BG,
        BG,BG,BG,BG,BG,BG,BG,BG,BG,BG,BG,BG,BG,BG,BG,BG,
        BG,BG,BG,BG,BG,BG,BG,BG,BG,BG,BG,BG,BG,BG,BG,BG,
        BG,BG,BG,BG,BG,BG,BG,BG,BG,BG,BG,BG,BG,BG,BG,BG
};

static int oldColor[256];


#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
/*
#  Matrix made by matblas from blosum45.iij
#  * column uses minimum score
#  BLOSUM Clustered Scoring Matrix in 1/3 Bit Units
#  Blocks Database = /data/blocks_5.0/blocks.dat
#  Cluster Percentage: >= 45
#  Entropy =   0.3795, Expected =  -0.2789
   A  R  N  D  C  Q  E  G  H  I  L  K  M  F  P  S  T  W  Y  V  B  Z  X  * */
static int BLOSUM45[24][24] = {
  {5, -2, -1, -2, -1, -1, -1,  0, -2, -1, -1, -1, -1, -2, -1,  1,  0, -2, -2,  0, -1, -1,  0, -5}, 
  {-2,  7,  0, -1, -3,  1,  0, -2,  0, -3, -2,  3, -1, -2, -2, -1, -1, -2, -1, -2, -1,  0, -1, -5}, 
  {-1,  0,  6,  2, -2,  0,  0,  0,  1, -2, -3,  0, -2, -2, -2,  1,  0, -4, -2, -3,  4,  0, -1, -5}, 
  {-2, -1,  2,  7, -3,  0,  2, -1,  0, -4, -3,  0, -3, -4, -1,  0, -1, -4, -2, -3,  5,  1, -1, -5}, 
  {-1, -3, -2, -3, 12, -3, -3, -3, -3, -3, -2, -3, -2, -2, -4, -1, -1, -5, -3, -1, -2, -3, -2, -5}, 
  {-1,  1,  0,  0, -3,  6,  2, -2,  1, -2, -2,  1,  0, -4, -1,  0, -1, -2, -1, -3,  0,  4, -1, -5}, 
  {-1,  0,  0,  2, -3,  2,  6, -2,  0, -3, -2,  1, -2, -3,  0,  0, -1, -3, -2, -3,  1,  4, -1, -5}, 
  {0, -2,  0, -1, -3, -2, -2,  7, -2, -4, -3, -2, -2, -3, -2,  0, -2, -2, -3, -3, -1, -2, -1, -5}, 
  {-2,  0,  1,  0, -3,  1,  0, -2, 10, -3, -2, -1,  0, -2, -2, -1, -2, -3,  2, -3,  0,  0, -1, -5}, 
  {-1, -3, -2, -4, -3, -2, -3, -4, -3,  5,  2, -3,  2,  0, -2, -2, -1, -2,  0,  3, -3, -3, -1, -5}, 
  {-1, -2, -3, -3, -2, -2, -2, -3, -2,  2,  5, -3,  2,  1, -3, -3, -1, -2,  0,  1, -3, -2, -1, -5}, 
  {-1,  3,  0,  0, -3,  1,  1, -2, -1, -3, -3,  5, -1, -3, -1, -1, -1, -2, -1, -2,  0,  1, -1, -5}, 
  {-1, -1, -2, -3, -2,  0, -2, -2,  0,  2,  2, -1,  6,  0, -2, -2, -1, -2,  0,  1, -2, -1, -1, -5}, 
  {-2, -2, -2, -4, -2, -4, -3, -3, -2,  0,  1, -3,  0,  8, -3, -2, -1,  1,  3,  0, -3, -3, -1, -5}, 
  {-1, -2, -2, -1, -4, -1,  0, -2, -2, -2, -3, -1, -2, -3,  9, -1, -1, -3, -3, -3, -2, -1, -1, -5}, 
  {1, -1,  1,  0, -1,  0,  0,  0, -1, -2, -3, -1, -2, -2, -1,  4,  2, -4, -2, -1,  0,  0,  0, -5}, 
  {0, -1,  0, -1, -1, -1, -1, -2, -2, -1, -1, -1, -1, -1, -1,  2,  5, -3, -1,  0,  0, -1,  0, -5}, 
  {-2, -2, -4, -4, -5, -2, -3, -2, -3, -2, -2, -2, -2,  1, -3, -4, -3, 15,  3, -3, -4, -2, -2, -5}, 
  {-2, -1, -2, -2, -3, -1, -2, -3,  2,  0,  0, -1,  0,  3, -3, -2, -1,  3,  8, -1, -2, -2, -1, -5}, 
  {0, -2, -3, -3, -1, -3, -3, -3, -3,  3,  1, -2,  1,  0, -3, -1,  0, -3, -1,  5, -3, -3, -1, -5}, 
  {-1, -1,  4,  5, -2,  0,  1, -1,  0, -3, -3,  0, -2, -3, -2,  0,  0, -4, -2, -3,  4,  2, -1, -5}, 
  {-1,  0,  0,  1, -3,  4,  4, -2,  0, -3, -2,  1, -1, -3, -1,  0, -1, -2, -2, -3,  2,  4, -1, -5}, 
  {0, -1, -1, -1, -2, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,  0,  0, -2, -1, -1, -1, -1, -1, -5}, 
  {-5, -5, -5, -5, -5, -5, -5, -5, -5, -5, -5, -5, -5, -5, -5, -5, -5, -5, -5, -5, -5, -5, -5,  1}
}; 
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */




/*  BLOSUM62 930809

#  Matrix made by matblas from blosum62.iij
#  * column uses minimum score
#  BLOSUM Clustered Scoring Matrix in 1/2 Bit Units
#  Blocks Database = /data/blocks_5.0/blocks.dat
#  Cluster Percentage: >= 62
#  Entropy =   0.6979, Expected =  -0.5209

  Note: to use with a2b[], always subtract 1 from the values !!!!

  A   R   N   D   C   Q   E   G   H   I   L   K   M   F   P   S   T   W   Y   V   B   Z   X  \* */ 
static int BLOSUM62[24][24] = {
  {4, -1, -2, -2,  0, -1, -1,  0, -2, -1, -1, -1, -1, -2, -1,  1,  0, -3, -2,  0, -2, -1,  0, -4},
  {-1,  5,  0, -2, -3,  1,  0, -2,  0, -3, -2,  2, -1, -3, -2, -1, -1, -3, -2, -3, -1,  0, -1, -4},
  {-2,  0,  6,  1, -3,  0,  0,  0,  1, -3, -3,  0, -2, -3, -2,  1,  0, -4, -2, -3,  3,  0, -1, -4},
  {-2, -2,  1,  6, -3,  0,  2, -1, -1, -3, -4, -1, -3, -3, -1,  0, -1, -4, -3, -3,  4,  1, -1, -4},
  {0, -3, -3, -3,  9, -3, -4, -3, -3, -1, -1, -3, -1, -2, -3, -1, -1, -2, -2, -1, -3, -3, -2, -4},
  {-1,  1,  0,  0, -3,  5,  2, -2,  0, -3, -2,  1,  0, -3, -1,  0, -1, -2, -1, -2,  0,  3, -1, -4},
  {-1,  0,  0,  2, -4,  2,  5, -2,  0, -3, -3,  1, -2, -3, -1,  0, -1, -3, -2, -2,  1,  4, -1, -4},
  { 0, -2,  0, -1, -3, -2, -2,  6, -2, -4, -4, -2, -3, -3, -2,  0, -2, -2, -3, -3, -1, -2, -1, -4},
  {-2,  0,  1, -1, -3,  0,  0, -2,  8, -3, -3, -1, -2, -1, -2, -1, -2, -2,  2, -3,  0,  0, -1, -4},
  {-1, -3, -3, -3, -1, -3, -3, -4, -3,  4,  2, -3,  1,  0, -3, -2, -1, -3, -1,  3, -3, -3, -1, -4},
  {-1, -2, -3, -4, -1, -2, -3, -4, -3,  2,  4, -2,  2,  0, -3, -2, -1, -2, -1,  1, -4, -3, -1, -4},
  {-1,  2,  0, -1, -3,  1,  1, -2, -1, -3, -2,  5, -1, -3, -1,  0, -1, -3, -2, -2,  0,  1, -1, -4},
  {-1, -1, -2, -3, -1,  0, -2, -3, -2,  1,  2, -1,  5,  0, -2, -1, -1, -1, -1,  1, -3, -1, -1, -4},
  {-2, -3, -3, -3, -2, -3, -3, -3, -1,  0,  0, -3,  0,  6, -4, -2, -2,  1,  3, -1, -3, -3, -1, -4},
  {-1, -2, -2, -1, -3, -1, -1, -2, -2, -3, -3, -1, -2, -4,  7, -1, -1, -4, -3, -2, -2, -1, -2, -4},
  { 1, -1,  1,  0, -1,  0,  0,  0, -1, -2, -2,  0, -1, -2, -1,  4,  1, -3, -2, -2,  0,  0,  0, -4},
  { 0, -1,  0, -1, -1, -1, -1, -2, -2, -1, -1, -1, -1, -2, -1,  1,  5, -2, -2,  0, -1, -1,  0, -4},
  {-3, -3, -4, -4, -2, -2, -3, -2, -2, -3, -2, -3, -1,  1, -4, -3, -2, 11,  2, -3, -4, -3, -2, -4},
  {-2, -2, -2, -3, -2, -1, -2, -3,  2, -1, -1, -2, -1,  3, -3, -2, -2,  2,  7, -1, -3, -2, -1, -4},
  { 0, -3, -3, -3, -1, -2, -2, -3, -3,  3,  1, -2,  1, -1, -2, -2,  0, -3, -1,  4, -3, -2, -1, -4},
  {-2, -1,  3,  4, -3,  0,  1, -1,  0, -3, -4,  0, -3, -3, -2,  0, -1, -4, -3, -3,  4,  1, -1, -4},
  {-1,  0,  0,  1, -3,  3,  4, -2,  0, -3, -3,  1, -1, -3, -1,  0, -1, -3, -2, -2,  1,  4, -1, -4},
  { 0, -1, -1, -1, -2, -1, -1, -1, -1, -1, -1, -1, -1, -1, -2,  0,  0, -2, -1, -1, -1, -1, -1, -4},
  {-4, -4, -4, -4, -4, -4, -4, -4, -4, -4, -4, -4, -4, -4, -4, -4, -4, -4, -4, -4, -4, -4, -4, -1}
 };



#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
/*
#  Matrix made by matblas from blosum80.iij
#  * column uses minimum score
#  BLOSUM Clustered Scoring Matrix in 1/2 Bit Units
#  Blocks Database = /data/blocks_5.0/blocks.dat
#  Cluster Percentage: >= 80
#  Entropy =   0.9868, Expected =  -0.7442
   A  R  N  D  C  Q  E  G  H  I  L  K  M  F  P  S  T  W  Y  V  B  Z  X  *  */
static int BLOSUM80[24][24] = {
  {5, -2, -2, -2, -1, -1, -1,  0, -2, -2, -2, -1, -1, -3, -1,  1,  0, -3, -2,  0, -2, -1, -1, -6}, 
  {-2,  6, -1, -2, -4,  1, -1, -3,  0, -3, -3,  2, -2, -4, -2, -1, -1, -4, -3, -3, -2,  0, -1, -6}, 
  {-2, -1,  6,  1, -3,  0, -1, -1,  0, -4, -4,  0, -3, -4, -3,  0,  0, -4, -3, -4,  4,  0, -1, -6}, 
  {-2, -2,  1,  6, -4, -1,  1, -2, -2, -4, -5, -1, -4, -4, -2, -1, -1, -6, -4, -4,  4,  1, -2, -6}, 
  {-1, -4, -3, -4,  9, -4, -5, -4, -4, -2, -2, -4, -2, -3, -4, -2, -1, -3, -3, -1, -4, -4, -3, -6}, 
  {-1,  1,  0, -1, -4,  6,  2, -2,  1, -3, -3,  1,  0, -4, -2,  0, -1, -3, -2, -3,  0,  3, -1, -6}, 
  {-1, -1, -1,  1, -5,  2,  6, -3,  0, -4, -4,  1, -2, -4, -2,  0, -1, -4, -3, -3,  1,  4, -1, -6}, 
  {0, -3, -1, -2, -4, -2, -3,  6, -3, -5, -4, -2, -4, -4, -3, -1, -2, -4, -4, -4, -1, -3, -2, -6}, 
  {-2,  0,  0, -2, -4,  1,  0, -3,  8, -4, -3, -1, -2, -2, -3, -1, -2, -3,  2, -4, -1,  0, -2, -6}, 
  {-2, -3, -4, -4, -2, -3, -4, -5, -4,  5,  1, -3,  1, -1, -4, -3, -1, -3, -2,  3, -4, -4, -2, -6}, 
  {-2, -3, -4, -5, -2, -3, -4, -4, -3,  1,  4, -3,  2,  0, -3, -3, -2, -2, -2,  1, -4, -3, -2, -6}, 
  {-1,  2,  0, -1, -4,  1,  1, -2, -1, -3, -3,  5, -2, -4, -1, -1, -1, -4, -3, -3, -1,  1, -1, -6}, 
  {-1, -2, -3, -4, -2,  0, -2, -4, -2,  1,  2, -2,  6,  0, -3, -2, -1, -2, -2,  1, -3, -2, -1, -6}, 
  {-3, -4, -4, -4, -3, -4, -4, -4, -2, -1,  0, -4,  0,  6, -4, -3, -2,  0,  3, -1, -4, -4, -2, -6}, 
  {-1, -2, -3, -2, -4, -2, -2, -3, -3, -4, -3, -1, -3, -4,  8, -1, -2, -5, -4, -3, -2, -2, -2, -6}, 
  {1, -1,  0, -1, -2,  0,  0, -1, -1, -3, -3, -1, -2, -3, -1,  5,  1, -4, -2, -2,  0,  0, -1, -6}, 
  {0, -1,  0, -1, -1, -1, -1, -2, -2, -1, -2, -1, -1, -2, -2,  1,  5, -4, -2,  0, -1, -1, -1, -6}, 
  {-3, -4, -4, -6, -3, -3, -4, -4, -3, -3, -2, -4, -2,  0, -5, -4, -4, 11,  2, -3, -5, -4, -3, -6}, 
  {-2, -3, -3, -4, -3, -2, -3, -4,  2, -2, -2, -3, -2,  3, -4, -2, -2,  2,  7, -2, -3, -3, -2, -6}, 
  {0, -3, -4, -4, -1, -3, -3, -4, -4,  3,  1, -3,  1, -1, -3, -2,  0, -3, -2,  4, -4, -3, -1, -6}, 
  {-2, -2,  4,  4, -4,  0,  1, -1, -1, -4, -4, -1, -3, -4, -2,  0, -1, -5, -3, -4,  4,  0, -2, -6}, 
  {-1,  0,  0,  1, -4,  3,  4, -3,  0, -4, -3,  1, -2, -4, -2,  0, -1, -4, -3, -3,  0,  4, -1, -6}, 
  {-1, -1, -1, -2, -3, -1, -1, -2, -2, -2, -2, -1, -1, -2, -2, -1, -1, -3, -2, -1, -2, -1, -1, -6}, 
  {-6, -6, -6, -6, -6, -6, -6, -6, -6, -6, -6, -6, -6, -6, -6, -6, -6, -6, -6, -6, -6, -6, -6,  1}
};
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */



/* ASCII-to-binary translation table
  Note: to use with BLOSUM62[], always subtract 1 from the values !!!! */
#undef NA
#define NA 0
static int a2b[] =
  {
    NA,NA,NA,NA,NA,NA,NA,NA,NA,NA,NA,NA,NA,NA,NA,NA,
    NA,NA,NA,NA,NA,NA,NA,NA,NA,NA,NA,NA,NA,NA,NA,NA,
    NA,NA,NA,NA,NA,NA,NA,NA,NA,NA,NA,NA,NA,NA,NA,NA,
    NA,NA,NA,NA,NA,NA,NA,NA,NA,NA,NA,NA,NA,NA,NA,NA,
    NA, 1,NA, 5, 4, 7,14, 8, 9,10,NA,12,11,13, 3,NA,
    15, 6, 2,16,17,NA,20,18,NA,19,NA,NA,NA,NA,NA,NA,
    NA, 1,NA, 5, 4, 7,14, 8, 9,10,NA,12,11,13, 3,NA,
    15, 6, 2,16,17,NA,20,18,NA,19,NA,NA,NA,NA,NA,NA,

    NA,NA,NA,NA,NA,NA,NA,NA,NA,NA,NA,NA,NA,NA,NA,NA,
    NA,NA,NA,NA,NA,NA,NA,NA,NA,NA,NA,NA,NA,NA,NA,NA,
    NA,NA,NA,NA,NA,NA,NA,NA,NA,NA,NA,NA,NA,NA,NA,NA,
    NA,NA,NA,NA,NA,NA,NA,NA,NA,NA,NA,NA,NA,NA,NA,NA,
    NA,NA,NA,NA,NA,NA,NA,NA,NA,NA,NA,NA,NA,NA,NA,NA,
    NA,NA,NA,NA,NA,NA,NA,NA,NA,NA,NA,NA,NA,NA,NA,NA,
    NA,NA,NA,NA,NA,NA,NA,NA,NA,NA,NA,NA,NA,NA,NA,NA,
    NA,NA,NA,NA,NA,NA,NA,NA,NA,NA,NA,NA,NA,NA,NA,NA 
  };

static int a2b_sean[] =
  {
    NA,NA,NA,NA,NA,NA,NA,NA,NA,NA,NA,NA,NA,NA,NA,NA,
    NA,NA,NA,NA,NA,NA,NA,NA,NA,NA,NA,NA,NA,NA,NA,NA,
    NA,NA,NA,NA,NA,NA,NA,NA,NA,NA,NA,NA,NA,NA,NA,NA,
    NA,NA,NA,NA,NA,NA,NA,NA,NA,NA,NA,NA,NA,NA,NA,NA,
    NA, 1,NA, 2, 3, 4, 5, 6, 7, 8,NA, 9,10,11,12,NA,
    13,14,15,16,17,NA,18,19,NA,20,NA,NA,NA,NA,NA,NA,
    NA, 1,NA, 2, 3, 4, 5, 6, 7, 8,NA, 9,10,11,12,NA,
    13,14,15,16,17,NA,18,19,NA,20,NA,NA,NA,NA,NA,NA,

    NA,NA,NA,NA,NA,NA,NA,NA,NA,NA,NA,NA,NA,NA,NA,NA,
    NA,NA,NA,NA,NA,NA,NA,NA,NA,NA,NA,NA,NA,NA,NA,NA,
    NA,NA,NA,NA,NA,NA,NA,NA,NA,NA,NA,NA,NA,NA,NA,NA,
    NA,NA,NA,NA,NA,NA,NA,NA,NA,NA,NA,NA,NA,NA,NA,NA,
    NA,NA,NA,NA,NA,NA,NA,NA,NA,NA,NA,NA,NA,NA,NA,NA,
    NA,NA,NA,NA,NA,NA,NA,NA,NA,NA,NA,NA,NA,NA,NA,NA,
    NA,NA,NA,NA,NA,NA,NA,NA,NA,NA,NA,NA,NA,NA,NA,NA,
    NA,NA,NA,NA,NA,NA,NA,NA,NA,NA,NA,NA,NA,NA,NA,NA
  };


 
#define xmosaicFetchStr        "Use efetch via WWW"

static MENU belvuMenu;

static MENUOPT mainMenu[] = 
{   
  {belvuMenuDestroy,    "Quit"},
  {Help,                "Help"},
  {wrapAlignmentWindow, "Wrap-around alignment window (pretty print)"},
  {graphPrint,          "Print this window"},
  {menuSpacer,          ""},
  {treeDisplay,         "Make tree from current alignment"},
  {treeSettings,        "Tree options"},
  {menuSpacer,          ""},
  {conservationPlot,    "Plot conservation profile"},
  {menuSpacer,          ""},
  {saveAlignment,       "Save alignment"},
  {saveRedraw,          "Save alignment as..."},
  {printScore,          "Output score and coords of line"},
    /* readMatchPrompt,     "Add sequence with matching segments", */   
    /* Too difficult to use interactively - rely solely on matchFooters instead */
  {menuSpacer,          ""},
  {xmosaicFetch,        xmosaicFetchStr},
  {listIdentity,        "Compare all vs. all, list identity"},
  {graphCleanUp,        "Clean up windows"},
  {0, 0}
};

static MENUOPT treeMenu[] = 
{   
  {treeDestroy,         "Quit"},
  {graphPrint,          "Print"},
  {treeSettings,        "Tree options"},
  {treePrintNH_init,    "Save tree in New Hampshire format"},
  {treeFindOrthologs,   "Find putative orthologs"},
  {showOrganisms,       "Show current organisms"},
  {0, 0}
};

static MENUOPT conservationMenu[] = 
{   
  {graphDestroy,        "Quit"},
  {graphPrint,          "Print"},
  {conservationSetWindow, "Set window size"},
  {conservationSetScale,  "Set plot scale factors"},
  {conservationSetLinewidth,    "Set line width of tree"},
  {0, 0}
};

static MENUOPT wrapMenu[] = 
{   
  {graphDestroy,        "Quit"},
  {graphPrint,          "Print"},
  {wrapAlignmentWindow, "New Wrap-around alignment window"},    
  {0, 0}
};



#define colorByResidueStr      "Colour scheme: By residue, Last palette"
#define colorSchemeStandardStr "Colour scheme: By residue, Erik's"
#define colorSchemeGibsonStr   "Colour scheme: By residue, Toby's"
#define colorSchemeCysStr      "Colour scheme: By residue, Cys/Gly/Pro"
#define colorSchemeEmptyStr    "Colour scheme: By residue, Clean slate"
#define colorSimStr            "Colour scheme: Average similarity by Blosum62"
#define colorIdStr             "Colour scheme: Percent identity only"
#define colorIdSimStr          "Colour scheme: Percent identity + Blosum62"
#define colorRectStr           "Display colors (faster without)"
#define printColorsStr         "Use gray shades (for printing)"
#define ignoreGapsStr          "Ignore gaps in conservation calculation"
#define thresholdStr           "Only colour residues above %id threshold"
static MENU colorMenu;
static MENUOPT colorMENU[] = 
{   
  {colorSchemeStandard, colorSchemeStandardStr}, 
  {colorSchemeGibson,   colorSchemeGibsonStr},   
  {colorSchemeCys,      colorSchemeCysStr},      
  {colorSchemeEmpty,    colorSchemeEmptyStr},
  {colorByResidue,      colorByResidueStr}, 
  {colorByResId,        thresholdStr},
/*    setColorCodes,       "Set colours manually",*/
  {saveColorCodes,      "Save current colour scheme"},
  {readColorCodesMenu,  "Read colour scheme from file"},
  {menuSpacer,          ""},
  {colorSim,            colorSimStr},            
  {colorId,             colorIdStr},             
  {colorIdSim,          colorIdSimStr},
  {ignoreGaps,          ignoreGapsStr},
  {menuSpacer,          ""},
  {printColors,         printColorsStr},
  {markup,              "Exclude highlighted from calculations on/off"},
  {colorRect,           colorRectStr},
  {lowercase,           "Highlight lowercase characters"},
  {showColorCodesRedraw, "Open window to edit colour scheme"},
  {0, 0}
};

static MENU colorEditingMenu;
static MENUOPT colorEditingMENU[] = 
{   
  {showColorCodesRedraw, "Close Colour Codes window first"},
  {0, 0}
};

#define rmEmptyColumnsStr "Automatically remove all-gap columns after sequence removals"
static MENU editMenu;
static MENUOPT editMENU[] = 
{   
  {rmPicked,            "Remove highlighted line"},
  {rmPickLeft,          "Remove many sequences"},
  {rmGappySeqsPrompt,   "Remove gappy sequences"},
  {rmPartial,           "Remove partial sequences"},
  {mkNonRedundantPrompt,"Make non-redundant"},
  {rmOutliers,          "Remove outliers"},
  {rmScore,             "Remove sequences below given score"},
  {menuSpacer,          ""},
  {rmColumnPrompt,      "Remove columns"},
  {rmColumnLeft,        "<- Remove columns to the left of cursor (inclusive)"},
  {rmColumnRight,       "Remove columns to the right of cursor (inclusive) -> "},
  {rmColumnCutoff,      "Remove columns according to conservation"},
  {rmEmptyColumnsInteract, "Remove gappy columns"},
  {rmEmptyColumnsToggle, rmEmptyColumnsStr},
  {menuSpacer,          ""},
  {menuSpacer,          ""},
  {readLabels,          "Read labels of highlighted sequence and spread them"},
  {selectGaps,          "Select gap character"},
  {hide,                "Hide highlighted line"},
  {unhide,              "Unhide all hidden lines"},
  {0, 0}
};

static MENU showColorMenu;
static MENUOPT showColorMENU[] = 
{   
  {finishShowColorCancel,   "CANCEL"},
  {finishShowColorOK,       "OK"},
  {0, 0}
};

static MENU sortMenu;
static MENUOPT sortMENU[] = 
{   
  {scoreSort,           "Sort by score"},
  {alphaSort,           "Sort alphabetically"},
  {organismSort,        "Sort by swissprot organism"},
  {treeSort,            "Sort by tree order"},
  {simSort,             "Sort by similarity to highlighted sequence"},
  {idSort,              "Sort by identity to highlighted sequence"},
  {0, 0}
};

#define MSFStr "MSF"
#define MulStr "Stockholm (Pfam/HMMER)"
#define FastaAlnStr "Aligned Fasta"
#define FastaStr "Unaligned Fasta"
static MENU saveMenu;
static MENUOPT saveMENU[] = 
{   
  {saveMSFSelect,           MSFStr},
  {saveMulSelect,           MulStr},
  {saveFastaAlnSelect,      FastaAlnStr},
  {saveFastaSelect,         FastaStr},
  {0, 0}
};

#define UPGMAstr "UPGMA"
#define NJstr "NJ"
static MENU treeGUIMenu;
static MENUOPT treeGUIMENU[] = 
{   
  {treeUPGMAselect,         UPGMAstr},
  {treeNJselect,            NJstr},
  {0, 0}
};


#define SWAPstr "Swap"
#define ROOTstr "Reroot"
static MENU treePickMenu;
static MENUOPT treePickMENU[] = 
{   
  {treeSWAPselect, SWAPstr},
  {treeROOTselect, ROOTstr},
  {0, 0}
};

#define UNCORRstr "Uncorrected differences"
#define KIMURAstr "Kimura correction"
#define JUKESCANTORstr "Jukes-Cantor correction"
#define STORMSONNstr "Storm & Sonnhammer correction"
#define Scorediststr "Scoredist correction"
static MENU treeDistMenu;
static MENUOPT treeDistMENU[] = 
{   
  {treeUNCORRselect,         UNCORRstr},
  {treeJUKESCANTORselect,    JUKESCANTORstr},
  {treeKIMURAselect,         KIMURAstr},
  {treeSTORMSONNselect,      STORMSONNstr},
  {treeScoredistselect,      Scorediststr},
  {0, 0}
};

static void fatal(char *format, ...)
{
    va_list  ap;

    printf("\nFATAL ERROR: "); 

    va_start(ap, format);
    vprintf(format, ap);
    va_end(ap);

    printf("\n"); 
    exit(-1);
}


static void menuCheck(MENU menu, int mode, int thismode, char *str)
{
    if (mode == thismode)
	menuSetFlags(menuItem(menu, str), MENUFLAG_TOGGLE_STATE);
    else
	menuUnsetFlags(menuItem(menu, str), MENUFLAG_TOGGLE_STATE);
}
static void setMenuCheckmarks(void)
{
    menuCheck(colorMenu, colorScheme, COLORBYRESIDUE, colorByResidueStr);
    menuCheck(colorMenu, colorScheme, COLORSCHEMESTANDARD, colorSchemeStandardStr);
    menuCheck(colorMenu, colorScheme, COLORSCHEMEGIBSON, colorSchemeGibsonStr);
    menuCheck(colorMenu, colorScheme, COLORSCHEMECYS, colorSchemeCysStr);
    menuCheck(colorMenu, colorScheme, COLORSCHEMEEMPTY, colorSchemeEmptyStr);
    menuCheck(colorMenu, colorScheme, COLORSIM, colorSimStr);
    menuCheck(colorMenu, colorScheme, COLORID, colorIdStr);
    menuCheck(colorMenu, colorScheme, COLORIDSIM, colorIdSimStr);
    menuCheck(colorMenu, 1, colorRectangles, colorRectStr);
    menuCheck(colorMenu, 1, printColorsOn, printColorsStr);
    menuCheck(colorMenu, 1, ignoreGapsOn, ignoreGapsStr); 
    menuCheck(editMenu, 1, rmEmptyColumnsOn, rmEmptyColumnsStr);

    if (!graphExists(showColorGraph))
	graphNewBoxMenu(colorButtonBox, colorMenu);
    else
	graphNewBoxMenu(colorButtonBox, colorEditingMenu);

    graphNewBoxMenu(sortButtonBox, sortMenu);

    graphNewBoxMenu(editButtonBox, editMenu);

    menuCheck(belvuMenu, 1, xmosaic, xmosaicFetchStr);
    graphNewMenu(belvuMenu);
}


static void Help(void)
{
    graphMessage (messprintf("\
Belvu - a multiple alignment viewer.\n\
\n\
Version %s\n\
Copyright (C) Erik Sonnhammer, 1995\n\
\n\
\n\
ALIGNMENT:\n\
  %d sequences, %d residues wide.\n\
\n\
LEFT MOUSE BUTTON:\n\
  Click on residue to show coordinate.\n\
  Double click on sequence to fetch.\n\
  Scrollbars: drag and page.\n\
  Toolbar buttons:\n\
     File:   Display this text.\n\
     Edit:   Remove many sequences.\n\
     Colour: Edit colour scheme window.\n\
     Sort:   Sort alphabetically.\n\
\n\
MIDDLE MOUSE BUTTON:\n\
  In alignment: centre on crosshair.\n\
  Scrollbars: drag and jump.\n\
\n\
RIGHT MOUSE BUTTON:\n\
  Toolbar menus (file menu elsewhere).\n\
\n\
KEYBOARD:\n\
  Arrow keys: Move screenfulls.\n\
  Home: Go to Top.\n\
  End: Go to Bottom.\n\
  Insert: Go to Start of line.\n\
  Delete: Go to End of line.\n\
For more details, see http://www.sanger.ac.uk/~esr/Belvu.html\n\
", belvuVersion, nseq, maxLen));
}


static void fetchAln(ALN *alnp)
{
  if (xmosaic)
    {
      static char *browser = NULL ;

      if (!browser)
	{
	  printf("Looking for WWW browsers ...\n") ;

	  if (!findCommand("netscape", &browser) &&
	      !findCommand("Netscape", &browser) &&
	      !findCommand("Mosaic", &browser) &&
	      !findCommand("mosaic", &browser) &&
	      !findCommand("xmosaic", &browser))
	    {
	      messout("Couldn't find any WWW browser.  Looked for "
		      "netscape, Netscape, Mosaic, xmosaic & mosaic");
	      return ;
	    }
	}

      printf("Using WWW browser %s\n", browser);
      fflush(stdout);
      system(messprintf("%s http://www.sanger.ac.uk/cgi-bin/seq-query?%s&", 
			browser,
			alnp->fetch));
      /* OLD: system(messprintf("xfetch '%s' &", alnp->fetch));*/
    }
  else
    {
      char  *env, fetchProg[1025]="";

      if ((env = getenv("BELVU_FETCH")) )
	strcpy(fetchProg, env);
      else
	strcpy(fetchProg, "efetch");
	    
      externalCommand(messprintf("%s '%s' &", fetchProg, alnp->fetch));
    }

  return ;
}


static void scrRight() { AlignXstart++; belvuRedraw(); }
static void scrLeft()  { AlignXstart--; belvuRedraw(); }
static void scrUp()    { AlignYstart--; belvuRedraw(); }
static void scrDown()  { AlignYstart++; belvuRedraw(); }
static void scrRightBig() { AlignXstart += Alignwid; belvuRedraw(); }
static void scrLeftBig()  { AlignXstart -= Alignwid; belvuRedraw(); }
static void scrUpBig()    { AlignYstart -= Alignheig; belvuRedraw(); }
static void scrDownBig()  { AlignYstart += Alignheig; belvuRedraw(); }


/* Unset the scroll bar dragging functions. */
static void unregister(void)
{
  graphRegister(LEFT_DRAG, NULL);
  graphRegister(LEFT_UP, NULL);

  graphRegister(MIDDLE_DRAG, NULL);
  graphRegister(MIDDLE_UP, NULL);

  return ;
}


static void HscrollUp(double x, double y) 
{
    float X = x - SliderOffset;

    AlignXstart = (X - HscrollStart)/(HscrollEnd - HscrollStart) * maxLen;
    unregister();
    belvuRedraw();

    return ;
}

static void HscrollDrag(double x, double y) 
{
    float X = x - SliderOffset;

    if (X < HscrollStart) X = HscrollStart;
    if (X + HsliderLen > HscrollEnd) X = HscrollEnd - HsliderLen;

    graphBoxShift(HscrollSliderBox, X, VscrollEnd+HSCRWID);
    graphRegister(LEFT_UP, HscrollUp);

    return ;
}


static void VscrollUp(double x, double y) 
{
    float Y = y - SliderOffset;

    AlignYstart = (Y - VscrollStart)/(VscrollEnd - VscrollStart) * nseq;
    unregister();
    belvuRedraw();

    return ;
}

static void VscrollDrag(double x, double y) 
{
    float Y = y - SliderOffset;

    if (Y < VscrollStart) Y = VscrollStart;
    if (Y + VsliderLen > VscrollEnd) Y = VscrollEnd - VsliderLen;

    graphBoxShift(VscrollSliderBox, HscrollEnd+VSCRWID, Y);
    graphRegister(LEFT_UP, VscrollUp);

    return ;
}

static int x2col(double *x)
{
    /* Round off to half units */
    *x = floor(*x)+0.5;

    if (*x < HscrollStart - VSCRWID+1) *x = HscrollStart - VSCRWID+1;
    else if (*x > HscrollStart - VSCRWID + Alignwid)
	*x = HscrollStart - VSCRWID + Alignwid;

    return (int) (*x - (HscrollStart - VSCRWID + 0.5) + AlignXstart + 1);
}

static void updateStatus(ALN *alnp, int col)
{
    int i, gaps=0, star;

    if (!alnp)
      return;

    strcpy(stats, " ");

    if (col)
      strcat(stats, messprintf("Column %d: ", col));

    strcat(stats, messprintf("%s/%d-%d", alnp->name, alnp->start, alnp->end));
    
    if (col)
      {
	strcat(stats, messprintf("  %c = ", alnp->seq[col-1]));
	
	for (star = i = 0; i < col; i++)
	  {
	    if (isGap(alnp->seq[i])) gaps++;
	    else if (alnp->seq[i] == '*') star = 1;
	  }
	
	if (star)
	  {
	    strcat(stats, "(unknown position due to insertion)");
	  }
	else
	  {
	    strcat(stats, messprintf("%d", col-1 + alnp->start - gaps));
	  }
      }

    strcat(stats, messprintf(" (%d match", Highlight_matches));
    if (Highlight_matches > 1)
      strcat(stats, "es");
    strcat(stats, ")");

    graphBoxDraw(statsBox, BLACK, LIGHTBLUE);

    return ;
}

static void xorVcrosshair(int mode, double x)
{
    static double xold;

    if (mode == 1)
	graphXorLine(xold, VscrollStart-HSCRWID, xold, VscrollEnd+HSCRWID);

    graphXorLine(x, VscrollStart-HSCRWID, x, VscrollEnd+HSCRWID);

    xold = x;
}

static void middleDrag(double x, double y) 
{
    pickedCol = x2col(&x);

    if (pickedCol == Vcrosshair) return;

    updateStatus(Highlight_alnp, pickedCol);

    Vcrosshair = pickedCol;

    xorVcrosshair(1, x);
}
static void middleUp(double x, double y) 
{
    pickedCol = x2col(&x);

    AlignXstart = pickedCol - Alignwid/2;
    unregister();
    belvuRedraw();
}

static void leftDown(double x, double y) 
{
    float oldlinew = graphLinewidth(0.05);
    
    graphRectangle(0.5, floor(y), wrapLinelen+0.5, floor(y)+1);
    graphLinewidth(oldlinew);

    graphRedraw();

}


static void middleDown(double x, double y) 
{
    if (x > HscrollStart && x < HscrollEnd &&
	y > VscrollEnd+HSCRWID && y < VscrollEnd+2*HSCRWID)
      {
	graphRegister(MIDDLE_DRAG, HscrollDrag);
	graphRegister(MIDDLE_UP, HscrollUp);
	SliderOffset = HsliderLen/2;
	HscrollDrag(x, y);
      }
    else if (y > VscrollStart && y < VscrollEnd &&
	x > HscrollEnd+VSCRWID && x < HscrollEnd+2*VSCRWID)
      {
	graphRegister(MIDDLE_DRAG, VscrollDrag);
	graphRegister(MIDDLE_UP, VscrollUp);
	SliderOffset = VsliderLen/2;
	VscrollDrag(x, y);
      }
    else if (x > HscrollStart-VSCRWID && x < HscrollEnd+VSCRWID &&
	     y > VscrollStart-HSCRWID && y < VscrollEnd+HSCRWID) 
      {
	/* In alignment */

	graphRegister(MIDDLE_DRAG, middleDrag);
	graphRegister(MIDDLE_UP, middleUp);

	pickedCol = x2col(&x);
	updateStatus(Highlight_alnp, pickedCol);

	Vcrosshair = pickedCol;

	xorVcrosshair(0, x);
    }
}


static int nrorder(ALN *x, ALN *y)
{
    if (x->nr < y->nr)
	return -1;
    else if (x->nr > y->nr)
	return 1;
    else return 0;
}


static int scoreorder(ALN *x, ALN *y)
{
    if (x->score < y->score)
	return -1;
    else if (x->score > y->score)
	return 1;
    else return 0;
}


static int scoreorderRev(ALN *x, ALN *y)
{
    if (x->score > y->score)
	return -1;
    else if (x->score < y->score)
	return 1;
    else return 0;
}


static int organism_order(ALN *x, ALN *y)
{
    return strcmp(x->organism, y->organism);
}



#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
static int alphaorder_alternative(ALN *x, ALN *y)
{
    int retval=0;

    retval = strcmp(x->name, y->name);

    if (!retval) {
	if (x->start < y->start)
	    retval = -1;
	else if (x->start > y->start)
	    retval = 1;
    }

    if (!retval) {
	if (x->end < y->end)
	    retval = -1;
	else if (x->end > y->end)
	    retval = 1;
    }

    if (!retval) {
	if (x->end < y->end)
	    retval = -1;
	else if (x->end > y->end)
	    retval = 1;
    }

    /* printf("Comparing %10s/%4d-%4d with %10s/%4d-%4d = %d\n", 
	   x->name, x->start, x->end, y->name, y->start, y->end, retval); */


    return retval;
}
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */



static int alphaorder(ALN *x, ALN *y)
{
  int retval=0;

  if (!(retval = strcmp(x->name, y->name)))
    {
      if (x->start == y->start)
	{
	  if (x->end < y->end)
	    retval = -1;
	  else if (x->end > y->end)
	    retval = 1;
	}
      else if (x->start < y->start)
	retval = -1;
      else if (x->start > y->start)
	retval = 1;
    }

  /* printf("Comparing %10s %4d %4d with %10s %4d %4d = %d\n", 
     x->name, x->start, x->end, y->name, y->start, y->end, retval); */

  return retval;
}

static int BSorder(BOOTSTRAPGROUP *x, BOOTSTRAPGROUP *y)
{
    return strcmp(x->s, y->s);
}


/* Separate markuplines to another array before resorting
*/
static void separateMarkupLines(void)
{
    int i, count=0;
    MarkupAlign = arrayReCreate(MarkupAlign, 100, ALN);

    if (Highlight_alnp)
	alncpy(&aln, Highlight_alnp);

    arrayOrder();

    for (i = 0; i+count < nseq; ) {
        alnp = arrp(Align, i, ALN);
	if (alnp->markup) {
	    /* printf ("Moving line %d, %s/%d-%d, nseq=%d\n", i, alnp->name, alnp->start, alnp->end, nseq);*/
	    arrayInsert(MarkupAlign, alnp, (void*)alphaorder);
	    arrayRemove(Align, alnp, (void*)nrorder);
	    count++;
	}
	else i++;
    }
    nseq -= count;
    arrayOrder();

    if (Highlight_alnp) {
	if (!alignFind(Align, &aln, &ip))
	    Highlight_alnp = 0;
	else
	    Highlight_alnp = arrp(Align, ip, ALN);
    }
}


/* Reinsert markuplines after mother sequence or if orphan at bottom 
*/
static void reInsertMarkupLines(void)
{
    int i, j;
    char tmpname[MAXNAMESIZE+1], *cp;

    for (i = arrayMax(MarkupAlign)-1; i >=0 ; i--)
      {
        alnp = arrp(MarkupAlign, i, ALN);
	strcpy(tmpname, alnp->name);

	if ((cp = strchr(tmpname, ' ')))
	  *cp = 0;

	arrayOrder10();

	for (j = 0; j < arrayMax(Align); j++)
	  {
	    if (!strcmp(tmpname, arrp(Align, j, ALN)->name))
	      break;
	  }

	alnp->nr = (j+1)*10+5;

	if (0)
	  {
	    printf ("Current alignment:\n");
	    {
	      int k; ALN *alnk;
	      for (k = 0; k < nseq; k++)
		{
		  alnk = arrp(Align, k, ALN);
		  printf("%s %d \n", alnk->name, alnk->nr);
		}
	    }
	    printf ("inserting %s at %d \n\n", alnp->name, alnp->nr);
	  }

	arrayInsert(Align, alnp, (void*)nrorder);
	nseq++;
      }

    arrayOrder();
}


static int organismorder(ALN *x, ALN *y)
{
    int retval=0;
    char *p1 = strchr(x->name, '_'), 
	*p2 = strchr(y->name, '_');

    if (!p1 && !p2) return alphaorder(x, y);
    if (!p1) return 1;
    if (!p2) return -1;

    if (!(retval = strcmp(p1, p2))) 
	return alphaorder(x, y);

    return retval;
}

static void showOrganisms(void)
{
    int i;

    if (!graphActivate(organismsGraph)) {
	organismsGraph = graphCreate (TEXT_FULL_SCROLL, "Organisms", 0, 0, 0.3, 0.5);

	graphTextFormat(FIXED_WIDTH);
	/* graphRegister(PICK, savePick); */
    }
    graphPop();
    graphClear();
    graphTextBounds(50, arrayMax(organismArr)+ 2);

    for (i = 0; i < arrayMax(organismArr); i++) {
	graphColor(arrp(organismArr, i, ALN)->color);
	graphText(arrp(organismArr, i, ALN)->organism, 1, 1+i);
    }
    graphRedraw();
}


/* Highlight the box of Highlight_alnp
 */
static void highlight(int ON)
{
    int i, box;

    if (!Highlight_alnp) return;

    box = Highlight_alnp->nr - AlignYstart;

    if (box > 0 && box <= Alignheig) {

	/* The alignment */
	if (ON) 
	    graphBoxDraw(box, WHITE, BLACK);
	else
	    graphBoxDraw(box, BLACK, WHITE);
    }

    /* All names * /
    for (i = Highlight_matches = 0; i < Alignheig; i++)
	if (!strcmp(arrp(Align, AlignYstart+i, ALN)->name, Highlight_alnp->name)) {
	    if (ON) {
		graphBoxDraw(Alignheig+i+1, WHITE, BLACK);
		Highlight_matches++;
	    }
	    else
		graphBoxDraw(Alignheig+i+1, BLACK, WHITE);
	}
*/

    /* All names - also count all matches */
    for (i = Highlight_matches = 0; i < nseq; i++)
	if (!strcmp(arrp(Align, i, ALN)->name, Highlight_alnp->name)) {
	    Highlight_matches++;

	    if (i >= AlignYstart && i < AlignYstart+Alignheig) {
		if (ON) 
		    graphBoxDraw(i-AlignYstart+1+Alignheig, WHITE, BLACK);
		else
		    graphBoxDraw(i-AlignYstart+1+Alignheig, BLACK, WHITE);
	    }
	}
}

/* General purpose routine to convert a string to ALN struct.
   Note: only fields Name, Start, End are filled!
 */
static void str2aln(char *src, ALN *alnp) {

    char *tmp = messalloc(strlen(src)+1) ;

    strcpy(tmp, src);
    stripCoordTokens(tmp);
    if (sscanf(tmp, "%s%d%d", alnp->name, &alnp->start, &alnp->end) != 3)
      {
	messout("Name to field conversion failed for %s (%s)", src, tmp);
	return;
      }

    if (strlen(alnp->name) > MAXNAMESIZE)
      fatal("buffer overrun in %s !!!!!!!!!\n", "str2aln") ;

    messfree(tmp);
}

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
/* General purpose routine to convert an ALN struct to a string.
 */
static char *aln2str(ALN *alnp) {

    char *cp = messalloc(strlen(alnp->name)+maxStartLen+maxEndLen+10);

    sprintf(cp, "%s/%d-%d\n", alnp->name, alnp->start, alnp->end);

    return cp;
}
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */



/* Action to picking a box in a tree window
*/
static void treeboxPick (int box, double x, double y, int modifier_unused)
{
    treeNode *treenodePicked;

    if (!box)
      return;

    if (!graphAssFind(assVoid(1), &treestruct))
      {
	messout("Can not find treeStruct");
	return;
      };

    if (!graphAssFind(assVoid(100+box), &treenodePicked))
      {
	messout("Picked box %d not associated", box);
	return;
      }

    /* Invert box */
    graphBoxDraw(treestruct->currentPickedBox, BLACK, WHITE);
    graphBoxDraw(box, WHITE, BLACK);
    treestruct->currentPickedBox = box;

    if (box > treestruct->lastNodeBox) { /* sequence box */

	/* Highlight it in the alignment */
	Highlight_alnp = treenodePicked->aln;
	centerHighlighted();
	belvuRedraw();
    }
    else { /* Tree node box picked - swap or reroot */
	if (treePickMode == NODESWAP) {
	    treeSwapNode(treenodePicked);
	    treeDraw(treeHead);
	}
	else {
	    if (treenodePicked->parent->parent) { /* Do not reroot with the same root */
		/* treestruct->head = treecpy(treestruct->head); */
		treeDraw(treeReroot(treenodePicked));
	    }
	}
    }
}

/* Click on name -> fetch 
 * Click on sequence -> update status bar 
 *
 * NOTE: the first Alignheig boxes are the alignment ones, then come Alignheig name ones.
 */
static void boxPick (int box, double x, double y, int modifier_unused)
{
  static int lastbox=0;
  static Graph  ga, lastga ;
  static time_t   lasttime;

    pickedCol = (int)x+1+AlignXstart;

    ga = graphActive();

    /* printf("Picked box %d\n", box); */

    if (box > Alignheig*2)
      {
	/* Click outside alignment - check for scrollbar controls */
	if (box == scrLeftBox) scrLeft();
	else if (box == scrRightBox) scrRight();
	else if (box == scrUpBox) scrUp();
	else if (box == scrDownBox) scrDown();
	else if (box == scrLeftBigBox) scrLeftBig();
	else if (box == scrRightBigBox) scrRightBig();
	else if (box == scrUpBigBox) scrUpBig();
	else if (box == scrDownBigBox) scrDownBig();
	else if (box == HscrollSliderBox)
	  {
	    graphRegister(LEFT_DRAG, HscrollDrag);
	    SliderOffset = x;
	  }
	else if (box == VscrollSliderBox)
	  {
	    graphRegister(LEFT_DRAG, VscrollDrag);
	    SliderOffset = y;
	  }

	return;
      }


    /* Alignment clicked */

    /* Turn last box off */
    if (lastga && lastbox) {
	graphActivate(lastga);
    }
    highlight(0);

    /* Turn all selections off */
    if (!box) {
	Highlight_alnp = 0;
	return;
    }

    aln.nr = (box > Alignheig ? box-Alignheig : box);
    aln.nr += AlignYstart;
    if (!arrayFind(Align, &aln, &ip, (void*)nrorder)) {
	messout("boxPick: Cannot find row %d in alignment array", aln.nr);
	return;
    }
    Highlight_alnp = alnp = arrp(Align, ip, ALN);
    

    /* Double click */
    if (box == lastbox && (time(0) - lasttime < DBL_CLK_DELAY)) 
      {
	lasttime = time(0) - DBL_CLK_DELAY;  /* To avoid triple clicks */

	/* 'Remove many sequences' mode */
	if (rmPickLeftOn)
	  {
	    rmPicked();
	    return;
	  }
	
	/* if (box > Alignheig) */
	fetchAln(alnp);
      }
    else
      lasttime = time(0);

    /* Single click */

    /* Turn this box on */
    graphActivate(ga);
    highlight(1);
    lastbox = box;
    lastga = ga;

    if (box <= Alignheig)
      {
	/* In alignment - update status bar */
	updateStatus(alnp, pickedCol);
      }
    else
      {
	updateStatus(alnp, 0);
	graphPostBuffer(alnp->name) ;
      }

    return ;
}


static void keyboard(int key, int unused)
{
    switch (key)
    {
/*    case UP_KEY: scrUp();             break;
    case DOWN_KEY: scrDown();         break;
    case LEFT_KEY: scrLeft();         break;
    case RIGHT_KEY: scrRight();       break;
*/	
    case PAGE_UP_KEY: scrUpBig();     break; /* Doesn't work */
    case PAGE_DOWN_KEY: scrDownBig(); break; /* Doesn't work */

    case UP_KEY: scrUpBig();             break;
    case DOWN_KEY: scrDownBig();         break;
    case LEFT_KEY: scrLeftBig();         break;
    case RIGHT_KEY: scrRightBig();       break;
	
    case INSERT_KEY: AlignXstart = 0; belvuRedraw();      break;
    case DELETE_KEY: AlignXstart = maxLen; belvuRedraw(); break;
    case HOME_KEY:   AlignYstart = 0; belvuRedraw();      break;
    case END_KEY:    AlignYstart = nseq; belvuRedraw();   break;
    }
}


static void initConservMtx(void)
{
    int i;

    conservCount = (int **)messalloc(21*sizeof(int *));
    colorMap = (int **)messalloc(21*sizeof(int *));
    
    for (i = 0; i < 21; i++) {
	conservCount[i] = (int *)messalloc(maxLen*sizeof(int));
	colorMap[i] = (int *)messalloc(maxLen*sizeof(int));
    }
    conservResidues = (int *)messalloc(maxLen*sizeof(int));

    conservation = (float *)messalloc(maxLen*sizeof(float));
}	


/* Calculate conservation in each column */
static int countResidueFreqs(void)
{
    int   i, j, nseqeff=0;

    if (!conservCount) initConservMtx();

    /* Must reset since this routine may be called many times */
    for (i = 0; i < maxLen; i++)
      {
	for (j = 0; j < 21; j++)
	  {
	    conservCount[j][i] = 0;
	    colorMap[j][i] = 0;
	  }
	conservResidues[i] = 0;
      }
    
    for (j = 0; j < nseq; j++)
      {
	alnp = arrp(Align, j, ALN);
	if (alnp->markup)
	  continue;

	nseqeff++;

	for (i = 0; i < maxLen; i++)
	  {
	    conservCount[a2b[(unsigned char)(alnp->seq[i])]][i]++;

	    if (isalpha(alnp->seq[i]) || alnp->seq[i] == '*')
	      conservResidues[i]++;
	  }
      }

    return nseqeff;
}


/* Return 1 if c1 has priority over c2, 0 otherwise */
static int colorPriority(int c1, int c2) {
    if (c2 == WHITE) return 1;
    if (c2 == maxbgColor) return 0;
    if (c2 == lowbgColor) {
	if (c1 == lowbgColor) return 0;
	else return 1;
    }
    if (c2 == midbgColor) {
	if (c1 == maxbgColor) return 1;
	else return 0;
    }

    fatal("Unknown colour %s", colorNames[c2]);		    /* exits program. */

    return 0 ;
}

static void setConservColors()
{
    int i, j, k, l, colornr, simCount, n, nseqeff;
    float id, maxid;

    if (!conservCount) initConservMtx();

    nseqeff = countResidueFreqs();

    for (i = 0; i < maxLen; i++)
	for (k = 1; k < 21; k++)
	    colorMap[k][i] = WHITE;

    for (i = 0; i < maxLen; i++) {
	maxid = -100.0;
	for (k = 1; k < 21; k++) {

	    if (color_by_similarity) {
		/* Convert counts to similarity counts */
		simCount = 0;
		for (j = 1; j < 21; j++) {
		    if (j == k) {
			if (1)
			    simCount += (conservCount[j][i]-1)*conservCount[k][i]*BLOSUM62[j-1][k-1];
			else
			    /* Alternative, less good way */
			    simCount += 
				(int)floor(conservCount[j][i]/2.0)*
				(int)ceil(conservCount[k][i]/2.0)*
				BLOSUM62[j-1][k-1];
		    }
		    else
			simCount += conservCount[j][i]*conservCount[k][i]*BLOSUM62[j-1][k-1];
		}
		
		if (ignoreGapsOn) 
		    n = conservResidues[i];
		else 
		    n = nseqeff;
		    
		if (n < 2)
		    id = 0.0;
		else {
		    if (1)
			id = (float)simCount/(n*(n-1));
		    else
			/* Alternative, less good way */
			id = (float)simCount/(n/2.0 * n/2.0);
		}
		
		/* printf("%d, %c:  simCount= %d, id= %.2f\n", i, b2a[k], simCount, id); */

		if (id > lowSimCutoff) {
		    if (id > maxSimCutoff) colornr = maxbgColor;
		    else if (id > midSimCutoff) colornr = midbgColor;
		    else colornr = lowbgColor;
		    
		    if (colorPriority(colornr, colorMap[k][i])) colorMap[k][i] = colornr;
		    
		    /* Colour all similar residues too */
		    for (l = 1; l < 21; l++) {
			if (BLOSUM62[k-1][l-1] > 0 && colorPriority(colornr, colorMap[l][i])) {
			    /*printf("%d: %c -> %c\n", i, b2a[k], b2a[l]);*/
			    colorMap[l][i] = colornr;
			}
		    }
		}
	    }
	    else {
		if (ignoreGapsOn && conservResidues[i] != 1)
		    id = (float)conservCount[k][i]/conservResidues[i];
		else
		    id = (float)conservCount[k][i]/nseqeff;
		
		if (colorByResIdOn) {
		    if (id*100.0 > colorByResIdCutoff)
		      colorMap[k][i] = color[(unsigned char)(b2a[k])];
		    else
			colorMap[k][i] = WHITE;
		}
		else if (id > lowIdCutoff) {
		    if (id > maxIdCutoff) colornr = maxbgColor;
		    else if (id > midIdCutoff) colornr = midbgColor;
		    else colornr = lowbgColor;
		    
		    colorMap[k][i] = colornr;
		    
		    if (id_blosum) {
			/* Colour all similar residues too */
			for (l = 1; l < 21; l++) {
			    if (BLOSUM62[k-1][l-1] > 0 && colorPriority(colornr, colorMap[l][i])) {
				/*printf("%d: %c -> %c\n", i, b2a[k], b2a[l]);*/
				colorMap[l][i] = colornr;
			    }
			}
		    }
		}
	    }
	    if (id > maxid) maxid = id;
	}
	conservation[i] = maxid;
    }
}
	    

static int findResidueBGcolor(ALN* alnp, int i) {
    
    if (lowercaseOn && alnp->seq[i] >= 'a' && alnp->seq[i] <= 'z') 
      return(ORANGE);
    else if (alnp->nocolor)
	return boxColor;
    else if (alnp->markup)
      return markupColor[(unsigned char)(alnp->seq[i])];
    else if (color_by_conserv || colorByResIdOn)
      return colorMap[a2b[(unsigned char)(alnp->seq[i])]][i];
    else
      return color[(unsigned char)(alnp->seq[i])];
}


static void colorCons(void)
{
    setConservColors();
    menuSetFlags(menuItem(colorMenu, thresholdStr), MENUFLAG_DISABLED);
    menuUnsetFlags(menuItem(colorMenu, printColorsStr), MENUFLAG_DISABLED);
    menuUnsetFlags(menuItem(colorMenu, ignoreGapsStr), MENUFLAG_DISABLED);
    colorByResIdOn = 0;
    belvuRedraw();
}
static void colorId(void)
{
    colorScheme = COLORID;
    color_by_conserv = 1;
    color_by_similarity = 0;
    id_blosum = 0;
    colorCons();
}
static void colorIdSim(void)
{
    colorScheme = COLORIDSIM;
    color_by_conserv = 1;
    color_by_similarity = 0;
    id_blosum = 1;
    colorCons();
}
static void colorSim(void)
{
    colorScheme = COLORSIM;
    color_by_conserv = 1;
    color_by_similarity = 1;
    colorCons();
}


static void graphScrollBar(float x1, float y1, float x2, float y2)
{
    int
	oldColor = graphColor(LIGHTBLUE);

    graphFillRectangle(x1, y1, x2, y2);
    graphColor(BLACK);
    graphRectangle(x1, y1, x2, y2);

    graphColor(oldColor);
}    


static void graphTriangle(float x1, float y1, float x2, float y2, float x3, float y3)
{
    Array temp = arrayCreate(6, float) ;
    int oldColor = graphColor(LIGHTBLUE);
    
    array(temp, 0, float) = x1;
    array(temp, 1, float) = y1;
    array(temp, 2, float) = x2;
    array(temp, 3, float) = y2;
    array(temp, 4, float) = x3;
    array(temp, 5, float) = y3;
    graphPolygon(temp);
    arrayDestroy(temp);

    graphColor(BLACK);
    graphLine(x1, y1, x2, y2);
    graphLine(x2, y2, x3, y3);
    graphLine(x3, y3, x1, y1);

    graphColor(oldColor);
}


static void belvuMenuDestroy()
{
    if (graphActive() == belvuGraph && 
	!saved && 
	messQuery ("Alignment was modified - save ?")) 
    {
	saveMul();
    }

    belvuDestroy() ;

    return ;
}



static void centerHighlighted(void)
{
    if (Highlight_alnp) 
	AlignYstart = Highlight_alnp->nr - Alignheig/2;
}


static void lowercase(void)
{
    lowercaseOn = !lowercaseOn;
    belvuRedraw();
}
	
static void belvuRedraw(void)
{
  char  seq[MAXLINE+1], ichar[10], widBylen[99], ch[2] = " ";
  int   i, j, box, nx, ny, seqlen, statsStart, bkcol;
  float Ypos=3, Ysave, x1, x2, y1, y2;

  /* Batch usage */
  if (!graphActivate(belvuGraph))
    return ;

  graphClear();
  graphTextFormat(FIXED_WIDTH);
  graphFitBounds(&nx, &ny);


  /* Development grid lines */
  if (gridOn)
    {
      graphColor(ORANGE);

      for (i = 0; i <= ny; i++)
	graphLine(0, i, nx, i);

      for (i=0; i <= nx; i++)
	graphLine(i, 0, i, ny);

      graphColor(BLACK);
    }

  Xpos = maxNameLen+1 + 1;
  if (1 /* coordsOn */ )
    Xpos += maxStartLen+1 + maxEndLen+1;
  if (displayScores)
    Xpos += maxScoreLen+1;


  Alignwid = nx-0.5 - VSCRWID - Xpos;
  if (Alignwid > maxLen)
    {
      Alignwid = maxLen;
      AlignXstart = 0;
    }
  if (AlignXstart < 0)
    AlignXstart = 0;
  if (AlignXstart + Alignwid > maxLen)
    AlignXstart = maxLen - Alignwid;
  seqlen = (Alignwid > MAXLINE ? MAXLINE : Alignwid);


  /* RULER *******/
  {
    if (!ruler)
      {
	ruler = messalloc(maxLen+1);
	memset(ruler, '-', maxLen);
	for (i = 10 ; i <= maxLen; i += 10)
	  {
	    sprintf(ichar, "%d", i);
	    strncpy(ruler + i - strlen(ichar), ichar, strlen(ichar));
	  }
      }

    strncpy(seq, ruler+AlignXstart, seqlen);
    seq[seqlen] = 0;
    graphText(seq, Xpos, Ypos);
    Ypos += 2;
  }
    
  /* Matching sequences ********************/
  {
    /* No, bad idea - have to rewrite all boxPick functions, and
       horizontal scrolling... */
	
    /* graphLine(0.5, Ypos-0.5, nx-0.5, Ypos-0.5);
	
    graphText("Query goes here", 1, Ypos);
    graphTextFormat (PLAIN_FORMAT);
    Ypos += 2;
    */
  }

  Alignheig = ny - 0.5 - HSCRWID - Ypos - 0.5;
  if (Alignheig > nseq)
    {
      Alignheig = nseq;
      AlignYstart = 0;
    }
  if (AlignYstart < 0)
    AlignYstart = 0;
  if (AlignYstart + Alignheig > nseq)
    AlignYstart = nseq - Alignheig;

  /* Alignment ***********/
  Ysave = Ypos;
  for (j = AlignYstart; j < AlignYstart+Alignheig && j < nseq; j++)
    {
      alnp = arrp(Align, j, ALN);
      if (alnp->hide) {
	/* Need dummy box to get box-counting right. Ugly but cheap */
	graphBoxStart();
	graphBoxEnd();
	continue;
      }

      graphBoxStart();

	for (i = AlignXstart; i < AlignXstart+Alignwid && i < maxLen; i++) {

	  if (!isGap(alnp->seq[i]) && alnp->seq[i] != ' ' && colorRectangles) {
	    bkcol = findResidueBGcolor(alnp, i);
	    graphColor(bkcol);
	    graphFillRectangle(Xpos+i-AlignXstart, Ypos, Xpos+i-AlignXstart+1, Ypos+1);
	  }
	  else 
	    bkcol = NOCOLOR;

	  /* Foreground color */
	  if (color_by_conserv) {
	    graphColor(bg2fgColor(bkcol));

	    *ch = alnp->seq[i];
	    graphText(ch, Xpos+i-AlignXstart, Ypos);
	  }
	}

      graphColor(BLACK);
      if (!color_by_conserv) {
	strncpy(seq, alnp->seq+AlignXstart, seqlen);
	seq[seqlen] = 0;
	graphText(seq, Xpos, Ypos);
      }
	
      graphBoxEnd(); 
      Ypos++;
    }
  graphColor(BLACK);

  /* Names, coordinates and scores *******************************/
  Ypos = Ysave;
  for (j = AlignYstart; j < AlignYstart+Alignheig && j < nseq; j++)
    {
      alnp = arrp(Align, j, ALN);
      if (alnp->hide) {
	/* Need dummy box to get box-counting right. Ugly but cheap */
	graphBoxStart();
	graphBoxEnd();
	continue;
      }

      box = graphBoxStart();
      graphText(alnp->name, 1, Ypos);
      graphBoxEnd(); 
      graphBoxDraw(box, BLACK, alnp->color);

      if (alnp->markup != GC /* && coordsOn */ ) {
	graphText(messprintf("%*d", maxStartLen, alnp->start), maxNameLen+2, Ypos);
	graphText(messprintf("%*d", maxEndLen, alnp->end), maxNameLen+maxStartLen+3, Ypos);
      }
      if (displayScores && !alnp->markup) {
	graphText(messprintf("%*.1f", maxScoreLen, alnp->score), maxNameLen+maxStartLen+maxEndLen+4, Ypos);
      }  
    
      /* Fetch command */
      /* Sort out database here since short PIR ones will be seen as
       * Swissprot otherwise - this overrides Efetch' default
       * parsing.  I do this since normally belvu has complete seq
       * names - no need to guess them.
       * /
       if (strchr(alnp->name, ':')) *db = 0;
       else if (strchr(alnp->name, '_')) strcpy(db, "SW:");
       else if (strchr(alnp->name, '.')) strcpy(db, "WP:");
       else strcpy(db, "PIR:");
       sprintf(alnp->fetch, "%s%s", db, alnp->name);
      */
      sprintf(alnp->fetch, "%s", alnp->name);

      Ypos++;
    }


  /* Scrollbars ************/

  HscrollStart = Xpos - 0.5 + VSCRWID;
  HscrollEnd = nx - 0.5 - 2*VSCRWID;
  VscrollStart = Ysave - 0.5 + HSCRWID;
  VscrollEnd = ny - 0.5 - 2*HSCRWID;

  x1 = HscrollStart + ((float)AlignXstart/maxLen)*(HscrollEnd-HscrollStart);
  x2 = HscrollStart + ((float)(AlignXstart+Alignwid)/maxLen)*(HscrollEnd-HscrollStart);
  HsliderLen = x2-x1;

  y1 = VscrollStart + ((float)AlignYstart/nseq)*(VscrollEnd-VscrollStart);
  y2 = VscrollStart + ((float)(AlignYstart+Alignheig)/nseq)*(VscrollEnd-VscrollStart);
  VsliderLen = y2-y1;

  /* Horizontal scrollbar */

  /* arrows */
  scrLeftBox = graphBoxStart();
  graphTriangle(HscrollStart-VSCRWID, ny-0.5-0.5*HSCRWID,
		HscrollStart, ny-0.5-HSCRWID,
		HscrollStart, ny-0.5);
  graphBoxEnd();
  graphBoxDraw(scrLeftBox, BLACK, SCRBACKCOLOR);
  scrRightBox = graphBoxStart();
  graphTriangle(HscrollEnd+VSCRWID, ny-0.5-0.5*HSCRWID,
		HscrollEnd, ny-0.5-HSCRWID,
		HscrollEnd, ny-0.5);
  graphBoxEnd();
  graphBoxDraw(scrRightBox, BLACK, SCRBACKCOLOR);
    
  /* big-scroll boxes */
  scrLeftBigBox = graphBoxStart();
  graphRectangle(HscrollStart, ny-0.5-HSCRWID,
		 x1, ny-0.5);
  graphBoxEnd();
  graphBoxDraw(scrLeftBigBox, BLACK, SCRBACKCOLOR);
  scrRightBigBox = graphBoxStart();
  graphRectangle(x2, ny-0.5-HSCRWID,
		 HscrollEnd, ny-0.5);
  graphBoxEnd();
  graphBoxDraw(scrRightBigBox, BLACK, SCRBACKCOLOR);

  /* slider */
  HscrollSliderBox = graphBoxStart();
  graphScrollBar(x1, ny-0.5-HSCRWID, x2, ny-0.5);
  graphBoxEnd();


  /* Vertical scrollbar */

  /* arrows */
  scrUpBox = graphBoxStart();
  graphTriangle(nx-0.5-0.5*VSCRWID, VscrollStart-HSCRWID,
		nx-0.5-VSCRWID, VscrollStart,
		nx-0.5, VscrollStart);
  graphBoxEnd();
  graphBoxDraw(scrUpBox, BLACK, SCRBACKCOLOR);
  scrDownBox = graphBoxStart();
  graphTriangle(nx-0.5-0.5*VSCRWID, VscrollEnd+HSCRWID,
		nx-0.5-VSCRWID, VscrollEnd,
		nx-0.5, VscrollEnd);
  graphBoxEnd();
  graphBoxDraw(scrDownBox, BLACK, SCRBACKCOLOR);

  /* big-scroll boxes */
  scrUpBigBox = graphBoxStart();
  graphRectangle(nx-0.5-VSCRWID, VscrollStart,
		 nx-0.5, y1);
  graphBoxEnd();
  graphBoxDraw(scrUpBigBox, BLACK, SCRBACKCOLOR);
  scrDownBigBox = graphBoxStart();
  graphRectangle(nx-0.5-VSCRWID, y2,
		 nx-0.5, VscrollEnd);
  graphBoxEnd();
  graphBoxDraw(scrDownBigBox, BLACK, SCRBACKCOLOR);

  /* slider */
  VscrollSliderBox = graphBoxStart();
  graphScrollBar(nx-0.5-VSCRWID, y1, nx-0.5, y2);
  graphBoxEnd();

  /* patch up frame of scroll boxes */
  graphRectangle(HscrollStart-VSCRWID, ny-0.5-HSCRWID,
		 HscrollEnd+VSCRWID, ny-0.5);
  graphRectangle(nx-0.5-VSCRWID, VscrollStart-HSCRWID,
		 nx-0.5, VscrollEnd+HSCRWID);


  /* Buttons *********/
  statsStart = 1;
  graphButton("File", Help, statsStart, 0.9);  /* Uses main menu, haha */
  statsStart += 6;
  editButtonBox = graphButton("Edit", rmPickLeft, statsStart, 0.9);
  statsStart += 6;
  colorButtonBox = graphButton("Colour", showColorCodesRedraw, statsStart, 0.9);
  statsStart += 8;
  sortButtonBox = graphButton("Sort", alphaSort, statsStart, 0.9);
  statsStart += 6;

  /* Status bar ************/
  sprintf(widBylen, "(%dx%d)", nseq, maxLen);
  graphText(widBylen, 1, 3);
  /* statsStart += strlen(widBylen)+1; */
  graphText("Picked:", statsStart, 1);
  statsStart += 8;
  statsBox = graphBoxStart();
  graphTextPtr(stats, statsStart, 1, nx-statsStart-1);
  graphBoxEnd();
  graphBoxDraw(statsBox, BLACK, LIGHTBLUE);

  /* Frame lines *************/
  graphLine(0.5, 0.5, nx-0.5, 0.5);
  graphLine(0.5, 0.5, 0.5, ny-0.5);
  graphLine(nx-0.5, 0.5, nx-0.5, ny-0.5);
  graphLine(0.5, ny-0.5, nx-0.5, ny-0.5);
  graphLine(0.5, 2.5, nx-0.5, 2.5);
  graphLine(0.5, Ysave-0.5, nx-0.5, Ysave-0.5);

  Xpos = 0.5 + maxNameLen + 1; 
  graphLine(Xpos, Ysave-0.5, Xpos, ny-0.5);
  Xpos += maxStartLen + 1;
  graphLine(Xpos, Ysave-0.5, Xpos, ny-0.5);
  Xpos += maxEndLen + 1;
  graphLine(Xpos, Ysave-0.5, Xpos, ny-0.5);
  if (displayScores) {
    Xpos += maxScoreLen + 1;
    graphLine(Xpos, Ysave-0.5, Xpos, ny-0.5);
  }
  

  /* Highlighted sequence **********************/
  highlight(1);

  setMenuCheckmarks();

  graphRedraw() ;

  return ;
}


static int isGap(char c) {
    if (c == '.' || c == '-' ||
	c == '[' || c == ']' /* Collapse-control chars */ ) 
	return 1;
    return 0;
}
  

static int isAlign(char c)
{
    if (isalpha(c) || isGap(c) || c == '*')
	return 1;
    else 
	return 0;
}


static int getMatchStates(void)
{
    int i, j, n, retval=0;

    for (j = 0; j < maxLen; j++) {
	n = 0;
	for (i = 0; i < nseq; i++) {
	    alnp = arrp(Align, i, ALN);
	    if (isalpha(alnp->seq[j])) n++;
	}
	if (n > nseq/2) retval++;
    }
    return retval;
}




/* Output a.a. probabilities in HMMER format.

   Disabled counting of match-state residues only (script HMM-random does that already)

   Disabled pseudocounts - not sure what the best way is.
*/
static void outputProbs(FILE *fil)
{
    /* From swissprot 33:

   Ala (A) 7.54   Gln (Q) 4.02   Leu (L) 9.31   Ser (S) 7.19
   Arg (R) 5.15   Glu (E) 6.31   Lys (K) 5.94   Thr (T) 5.76
   Asn (N) 4.54   Gly (G) 6.86   Met (M) 2.36   Trp (W) 1.26
   Asp (D) 5.29   His (H) 2.23   Phe (F) 4.06   Tyr (Y) 3.21
   Cys (C) 1.70   Ile (I) 5.72   Pro (P) 4.91   Val (V) 6.52
   */
    float f_sean[21] = {
	0.0, 
	.08713, .03347, .04687, .04953, .03977, .08861, .03362, .03689, .08048, .08536, 
	.01475, .04043, .05068, .03826, .04090, .06958, .05854, .06472, .01049, .02992
    };

    float p;
    int i, c[21], n=0, nmat;
    char *cp;

    for (i = 1; i <= 20; i++) c[i] = 0;

    for (i = 0; i < nseq; i++)
      {
	alnp = arrp(Align, i, ALN);
	cp = alnp->seq;

	for (; *cp; cp++)
	  {
	    if (a2b_sean[(unsigned char)(*cp)])
	      {
		c[a2b_sean[(unsigned char)(*cp)]]++;
		n++;
	      }
	  }
    }

    if (!n)
      fatal("No residues found");

    if (0) {
	nmat = getMatchStates();	/* Approximation, HMM may differ slightly */
	if (!nmat) fatal("No match state columns found");
	
	printf("Amino\n");
	for (i = 1; i <= 20; i++) {
	    /* One way:  p = ((float)c[i]/nmat + 20*f_sean[i]) / (float)(n/nmat+20);*/
	    /* Other way: */
	    p = ((float)c[i] + 20*f_sean[i]) / (float)(n+20);
	    if (p < 0.000001) p = 0.000001; /* Must not be zero */
	    printf("%f ", p);
	    /* printf("   %d %f %d \n", c[i], f[i], n);*/
	}
    }
    else {
	printf("Amino\n");
	for (i = 1; i <= 20; i++) {
	    p = (float)c[i] / (float)n;
	    if (p < 0.000001) p = 0.000001; /* Must not be zero */
	    printf("%f ", p);
	}
    }
    printf("\n");

    fclose(fil);
    fflush(fil);

    saved = 1;
}


/* 
   Format the name/start-end string 

   For convenience, used by writeMul
*/
char *writeMulName(ALN *aln) {

    static char name[MAXNAMESIZE+50];
    char *cp, *namep, GRname[MAXNAMESIZE*2+2], GRfeat[MAXNAMESIZE+1];

    if (aln->markup == GC) 
	return messprintf("#=GC %s", aln->name);

    /* NOTE: GR lines have the feature concatenated in aln->name - must separate */
    if (aln->markup == GR) {
	namep = GRname;
	strncpy(namep, aln->name, 50);
	cp = strchr(namep, ' ');
	strncpy(GRfeat, cp+1, 50);
	*cp = 0;
    }
    else
	namep = aln->name;

    if (!saveCoordsOn) {
	strcpy(name, messprintf("%s", namep));
    }
    else {
	strcpy(name, messprintf("%s%c%d-%d", namep, saveSeparator, aln->start, aln->end));
    }

    if (aln->markup == GR)
	return messprintf("#=GR %s %s", name, GRfeat);
    else
	return name;
}


static void writeMul(FILE *fil)
{
    static char *cp;
    int i, w, W;

    /* Write Annotation */
    if (!stackEmpty(AnnStack))
      {
	stackCursor(AnnStack, 0);
	while ((cp = stackNextText(AnnStack)))
	  {
	    fprintf(fil, "%s\n", cp);
	  }
      }

    /* Find max width of name column */
    for (i = w = 0; i < nseq; i++)
	if ( (W = strlen(writeMulName(arrp(Align, i, ALN)))) > w) w = W;
    
    /* Write alignment */
    for (i = 0; i < nseq; i++)
	fprintf(fil, "%-*s %s\n", w, writeMulName(arrp(Align, i, ALN)), arrp(Align, i, ALN)->seq);

    fprintf(fil, "//\n");

    fclose(fil);
    fflush(fil);

    saved = 1;
}

static void saveMul(void)
{
    FILE *fil;

    if (!(fil = filqueryopen(dirName, fileName, "","w", "Save as Stockholm file:" ))) return;

    writeMul(fil);
}


static void writeFasta(FILE *pipe)
{
    int i, n;
    char *cp;

    for (i = 0; i < nseq; i++) {
	
	alnp = arrp(Align, i, ALN);
	if (alnp->markup) continue;
	
	if (saveCoordsOn)
	    fprintf(pipe, ">%s%c%d-%d\n", alnp->name, saveSeparator, alnp->start, alnp->end);
	else
	    fprintf(pipe, ">%s\n", alnp->name);
	
	for (n=0, cp = alnp->seq; *cp; cp++) {
	    if (!strcmp(saveFormat, FastaAlnStr)) {
		fputc(*cp, pipe);
		n++;
	    }
	    else {
		if (!isGap(*cp)) {
		    fputc(*cp, pipe);
		    n++;
		}
	    }
	    if (n && !(n % 80) ) {
		fputc('\n', pipe);
		n = 0;
	    }
	}
	if (n) fputc('\n', pipe);
    }

    fclose(pipe);
    fflush(pipe);
}


static void saveFasta(void)
{
    FILE *fil;

    if (!(fil = filqueryopen(dirName, fileName, "","w", "Save as unaligned Fasta file:"))) return;

    writeFasta(fil);

    saved = 1;
}


static void saveAlignment(void)
{
    if (!strcmp(saveFormat, MSFStr))
	saveMSF();
    else if (!strcmp(saveFormat, FastaAlnStr))
	saveFasta();
    else if (!strcmp(saveFormat, FastaStr))
	saveFasta();
    else
	saveMul();
}


static void graphButtonCheck(char* text, void (*func)(void), float x, float *y, int On)
{
    char buttont[1024];

    strcpy(buttont, messprintf("%s %s", On ? "*" : " ", text));

    graphButton(buttont, func, x, *y);
    *y += 2;
}

static void saveMSFSelect(void) {
    strcpy(saveFormat, MSFStr);
    saveRedraw();
}
static void saveMulSelect(void){
    strcpy(saveFormat, MulStr);
    saveRedraw();
}
static void saveFastaAlnSelect(void){
    strcpy(saveFormat, FastaAlnStr);
    saveRedraw();
}
static void saveFastaSelect(void){ /* unaligned */
    strcpy(saveFormat, FastaStr);
    saveRedraw();
}

static void treeUPGMAselect(void){
    strcpy(treeMethodString, UPGMAstr);
    treeMethod = UPGMA;
    treeScale = 1.0;
    treeSettings();
}
static void treeNJselect(void){
    strcpy(treeMethodString, NJstr);
    treeMethod = NJ;
    if (treeDistCorr == UNCORR)
	treeScale = 1.0;
    else 
	treeScale = 0.3;
    treeSettings();
}



static void treeSWAPselect(void) {
    strcpy(treePickString, SWAPstr);
    treePickMode = NODESWAP;
    treeSettings();
}
static void    treeROOTselect(void) {
    strcpy(treePickString, ROOTstr);
    treePickMode = NODEROOT;
    treeSettings();
}

static void setTreeScaleCorr(void) {
    if (treeMethod == UPGMA)
	treeScale = 1.0;
    else if (treeMethod == NJ)
	treeScale = 0.3;
}

static void treeUNCORRselect(void){
    strcpy(treeDistString, UNCORRstr);
    treeDistCorr = UNCORR;
    treeScale = 1.0;
    treeSettings();
}

static void treeJUKESCANTORselect(void){
    strcpy(treeDistString, JUKESCANTORstr);
    treeDistCorr = JUKESCANTOR;
    setTreeScaleCorr();
    treeSettings();
}
static void treeKIMURAselect(void){
    strcpy(treeDistString, KIMURAstr);
    treeDistCorr = KIMURA;
    setTreeScaleCorr();
    treeSettings();
}
static void treeSTORMSONNselect(void){
    strcpy(treeDistString, STORMSONNstr);
    treeDistCorr = STORMSONN;
    setTreeScaleCorr();
    treeSettings();
}
static void treeScoredistselect(void){
    strcpy(treeDistString, Scorediststr);
    treeDistCorr = Scoredist;
    setTreeScaleCorr();
    treeSettings();
}

static void saveCoordsToggle(void) {
    saveCoordsOn = !saveCoordsOn;
    saveRedraw();
}

static void saveSeparatorToggle(void) {
    if (saveSeparator == '/')
	saveSeparator = '=';
    else 
	saveSeparator = '/';
    saveRedraw();
}

static void saveOK(void) {
    
    saveAlignment();

    graphDestroy();
    graphActivate(belvuGraph);
}

static void saveRedraw(void)
{
    float 
	x = 1.0,
	y = 1.0;

    if (!graphActivate(saveGraph)) {
	saveGraph = graphCreate (TEXT_FIT, "Save As", 0, 0, 0.5, 0.25);

	graphTextFormat(FIXED_WIDTH);
	/* graphRegister(PICK, savePick); */
    }
    graphPop();
    graphClear();

    graphText("Format:", x, y);
    graphNewBoxMenu(graphButton(saveFormat, saveRedraw, x+9, y), saveMenu);
    y += 2.0;

    graphButtonCheck("Save with coordinates", saveCoordsToggle, x, &y, saveCoordsOn);

    if (saveCoordsOn) {
	char saveSeparatort[10];

	graphText("Separator char between name and coordinates:", x, y);
	strcpy(saveSeparatort, messprintf(" %c ", saveSeparator));
	graphButton(saveSeparatort, saveSeparatorToggle, x+45, y);
	y += 1.0;
	graphText("(Use = for GCG)", x, y);
	y += 1.0;
    }

    y += 2;
    
    graphButton("OK", saveOK, x, y);
    graphButton("Cancel", graphDestroy, x+4, y);

    graphRedraw();
}


static void treeShowBranchlenToggle(void) {
    treeShowBranchlen = !treeShowBranchlen;
    treeSettings();
}


static void treeShowOrganismToggle(void) {
    treeShowOrganism = !treeShowOrganism;
    treeSettings();
}


static void treeSettings(void)
{
    float 
	x = 1.0,
	y = 1.0;
    static char
	linewidthstr[256],
	treeScalestr[256];

    if (!graphActivate(treeGUIGraph)) {
	treeGUIGraph = graphCreate (TEXT_FIT, "tree GUI", 0, 0, 0.6, 0.35);

	graphTextFormat(FIXED_WIDTH);
	/* graphRegister(PICK, savePick); */
    }
    graphPop();
    graphClear();

    graphText("Select tree building method:", x, y);
    graphNewBoxMenu(graphButton(treeMethodString, treeSettings, x+30, y), treeGUIMenu);
    y += 2.0;

    graphText("Select distance correction method:", x, y);
    graphNewBoxMenu(graphButton(treeDistString, treeSettings, x+35, y), treeDistMenu);
    y += 3.0;

    graphText("Display options:", x, y);
    y += 2.0;

    sprintf(treeScalestr, "%.2f", treeScale);
    graphText("Tree Scale:", x, y);
    graphTextEntry (treeScalestr, 5, x+12, y, (void*)(treeSetScale));
    y += 2.0;

    sprintf(linewidthstr, "%.2f", treeLinewidth);
    graphText("Line width:", x, y);
    graphTextEntry (linewidthstr, 5, x+12, y, (void*)(treeSetLinewidth));
    y += 2.0;

    graphText("Action when picking a node:", x, y);
    graphNewBoxMenu(graphButton(treePickString, treeSettings, x+30, y), treePickMenu);
    y += 2.0;

    graphButtonCheck("Display branch lengths", treeShowBranchlenToggle, x, &y, treeShowBranchlen);

    graphButtonCheck("Display organism", treeShowOrganismToggle, x, &y, treeShowOrganism);

    /*graphButtonCheck("Sample trees in intervals", treeSampleToggle, x, &y, treeBootstrapOn);
    if (treeBootstrapOn) {
              char saveSeparatort[10];

	graphText("Number of bootstrapsSeparator char between name and coordinates:", x, y);
	strcpy(saveSeparatort, messprintf(" %c ", saveSeparator));
	graphButton(saveSeparatort, saveSeparatorToggle, x+45, y);
	y += 1.0;
	graphText("(Use = for GCG)", x, y);
	y += 1.0;
    }*/

    y += 2;
    graphButton("Make tree", treeDisplay, x, y);

    y += 2;
    graphButton("Quit", graphDestroy, x, y);

    graphRedraw();
}


static void alncpy(ALN *dest, ALN *src)
{
    strncpy(dest->name, src->name, MAXNAMESIZE);
    dest->start = src->start;
    dest->end = src->end;
    dest->len = src->len;
    /* dest->seq = messalloc(strlen(src->seq));
       strcpy(dest->seq, src->seq); */
    dest->seq = src->seq;
    dest->nr = src->nr;			
    strncpy(dest->fetch, src->fetch, MAXNAMESIZE+10);
    dest->score = src->score;
    dest->color = src->color;
}


static void resetALN(ALN *alnp)
{
    *alnp->name = *alnp->fetch = 0;
    alnp->start = alnp->end = alnp->len = alnp->nr = 0;
    alnp->seq = 0;
    alnp->score = 0.0;
    alnp->color = WHITE;
    alnp->markup = 0;
    alnp->organism = 0;
}


/* Set the ->nr field in Align array in ascending order
 */
static void arrayOrder(void)
{
    int i;

    for (i = 0; i < nseq; i++) arrp(Align, i, ALN)->nr = i+1;
}


/* Set the ->nr field in Align array in ascending order with increments of 10
   Necessary if one wants to insert items.
 */
static void arrayOrder10(void)
{
    int i;

    for (i = 0; i < nseq; i++) arrp(Align, i, ALN)->nr = (i+1)*10;
}



static void init_sort_do(void)
{
    arraySort(Align, (void*)nrorder);

    if (init_sort) {
	switch(init_sort) 
	{
	case SORT_ALPHA : arraySort(Align, (void*)alphaorder);   break;
	case SORT_ORGANISM : arraySort(Align, (void*)organismorder);   break;
	case SORT_SCORE : scoreSortBatch();   break;
	case SORT_SIM :
	    Highlight_alnp = arrp(Align, 0, ALN);
	    highlightScoreSort('P'); break;
	case SORT_ID : 
	    Highlight_alnp = arrp(Align, 0, ALN);
	    highlightScoreSort('I'); break;
	case SORT_TREE  : treeSortBatch();    break;
	}
    }
}


/* Convert plot coordinates to screen coordinates */
static float xTran(float x)
{
    return(5 + x*conservationXScale);
}
static float yTran(float y)
{
    return(3 + conservationRange - y*conservationYScale);
}


static void conservationSetWindow(void)
{
    ACEIN cons_in;

    if (!(cons_in = messPrompt("Window size:", 
			       messprintf("%d", conservationWindow), 
			       "iz", 0)))
	return;
    
    aceInInt(cons_in, &conservationWindow);
    aceInDestroy(cons_in);
    cons_in = NULL ;

    conservationPlot();
}
static void conservationSetLinewidth(void)
{
    ACEIN cons_in;
    
    if (!(cons_in = messPrompt("Line width:", 
		     messprintf("%.2f", conservationLinewidth), 
			       "fz", 0)))
	return;
    
    aceInFloat(cons_in, &conservationLinewidth);
    aceInDestroy(cons_in);
    cons_in = NULL ;

    conservationPlot();
}
static void conservationSetScale(void)
{
    if (!(ace_in = messPrompt("X and Y scales:", 
		     messprintf("%.1f %.1f", conservationXScale, conservationYScale), 
			      "ffz", 0)))
	return;
    
    aceInFloat(ace_in, &conservationXScale);
    aceInFloat(ace_in, &conservationYScale);
    aceInDestroy(ace_in);
    ace_in = NULL ;

    conservationPlot();
}


static void conservationPlot()
{
    int i;
    float 
	oldlinew, mincons, maxcons, avgcons,
	XscaleBase, YscaleBase,
	*smooth, sum, f;

    /* init window */
    if (!graphActivate(conservationGraph))
      {
	conservationGraph = graphCreate (TEXT_FULL_SCROLL, "Belvu conservation profile", 0, 0, 1, 0.4);
	graphTextFormat(FIXED_WIDTH);
      }

    oldlinew = graphLinewidth(conservationLinewidth);
    graphClear();
    graphMenu(conservationMenu);

    /* smooth  conservation by applying a window */
    smooth = messalloc(maxLen*sizeof(float));
    sum = 0.0;
    for (i = 0; i < conservationWindow; i++) 
	sum += conservation[i];
    smooth[conservationWindow/2] = sum/conservationWindow;
    for (     ; i < maxLen; i++) {
	sum -= conservation[i-conservationWindow];
	sum += conservation[i];
	smooth[i-conservationWindow/2] = sum/conservationWindow;
    }


    /* Find max and min and avg conservation */
    maxcons = -1;
    mincons = 10000;
    avgcons = 0;
    for (i = 0; i < maxLen; i++) {
	if (smooth[i] > maxcons) maxcons = smooth[i];
	if (smooth[i] < mincons) mincons = smooth[i];
	avgcons += conservation[i];
    }
    avgcons /= maxLen*1.0;
    conservationRange = (maxcons - mincons)*conservationYScale;
    graphTextBounds(maxLen*conservationXScale+30, conservationRange*conservationYScale+10);

    graphText(messprintf("Window = %d", conservationWindow), 10, 1);

    /* Draw x scale */
    YscaleBase = -1;
    graphLine(xTran(0), yTran(YscaleBase), xTran(maxLen), yTran(YscaleBase));
    for (i=0; i < maxLen; i += 10) {
	graphLine(xTran(i), yTran(YscaleBase), 
		  xTran(i), yTran(YscaleBase)+0.5);
	if (i+5 < maxLen-1)
	    graphLine(xTran(i+5), yTran(YscaleBase), 
		      xTran(i+5), yTran(YscaleBase)+0.25);
	graphText(messprintf("%d", i), xTran(i)-0.5, yTran(YscaleBase)+1);
    }
    
    /* Draw y scale */
    XscaleBase = -1;
    graphLine(xTran(XscaleBase), yTran(0), xTran(XscaleBase), yTran(maxcons));
    for (f=mincons; f < maxcons; f += 1) {
	graphLine(xTran(XscaleBase), yTran(f), 
		  xTran(XscaleBase-0.5), yTran(f));
	if (f+0.5 < maxcons) graphLine(xTran(XscaleBase), yTran(f+0.5), 
				       xTran(XscaleBase-0.25), yTran(f+0.5));
	graphText(messprintf("%.0f", f), xTran(XscaleBase)-3, yTran(f)-0.5);
    }
    
    /* Draw average line */
    graphColor(RED);
    graphLine(xTran(0), yTran(avgcons), xTran(maxLen), yTran(avgcons));
    graphText("Average conservation", xTran(maxLen), yTran(avgcons));
    graphColor(BLACK);

    /* Plot conservation */
    for (i=1; i <maxLen; i++) {
	graphLine(xTran(i), yTran(smooth[i-1]), 
		  xTran(i+1), yTran(smooth[i]));
    }
    
    graphLinewidth(oldlinew);
    graphRedraw();
    messfree(smooth);

}


/* Linear search for an exact matching sequence name and coordinates,
   typically to find back highlighted row after sorting 
*/
static int alignFind( Array arr, ALN *obj, int *ip)
{
    for (*ip=0; *ip < arrayMax(arr); (*ip)++) {
	if (!alphaorder(arrp(arr, *ip, ALN), obj)) return 1;
    }
    return 0;
}


static void alphaSort(void)
{
    if (Highlight_alnp)
	alncpy(&aln, Highlight_alnp);

    arraySort(Align, (void*)alphaorder);

    if (Highlight_alnp) {
	if (!alignFind(Align, &aln, &ip)) {
	    messout("Cannot find back highlighted seq after sort - probably a bug");
	    Highlight_alnp = 0;
	}
	else
	    Highlight_alnp = arrp(Align, ip, ALN);
    }

    arrayOrder();
    centerHighlighted();
    belvuRedraw();
}


/* Set the default colors of organisms to something somewhat intelligent
 */
static void setOrganismColors(void) {

    int i, color;

    for (i=0; i < arrayMax(organismArr); i++) {
	color = treeColors[i % 16];
	/*if (i > 15) color = treeColors[15];*/

	arrp(organismArr, i, ALN)->color =  color;
    }
}


/* Convert swissprot name suffixes to organisms 
 */
static void suffix2organism(void) {

    int i;
    char *cp;

    for (i=0; i<arrayMax(Align); i++) {

	alnp = arrp(Align, i, ALN);

	if (!alnp->markup && (cp = strchr(alnp->name, '_'))) {
	    char *suffix = messalloc(strlen(cp)+1);
	    strcpy(suffix, cp+1);

	    /* Add organism to table of organisms.  This is necessary to make all 
	       sequences of the same organism point to the same place and to make a 
	       non-redundant list of present organisms */
	    alnp->organism = suffix;
	    arrayInsert(organismArr, alnp, (void*)organism_order);

	    /* Store pointer to unique organism in ALN struct */
	    arrayFind(organismArr, alnp, &ip, (void*)organism_order);
	    alnp->organism = arrp(organismArr, ip, ALN)->organism;
	}
    }
}


static void organismSort(void)
{
    if (Highlight_alnp)
	alncpy(&aln, Highlight_alnp);

    arraySort(Align, (void*)organismorder);

    if (Highlight_alnp) {
	if (!alignFind(Align, &aln, &ip)) {
	    messout("Cannot find back highlighted seq after sort - probably a bug");
	    Highlight_alnp = 0;
	}
	else
	    Highlight_alnp = arrp(Align, ip, ALN);
    }

    arrayOrder();
    centerHighlighted();
    belvuRedraw();
}


static void showAnnotation(void)
{
    int maxwidth=0,
	nlines=0,
	i=0;
    char *cp;

    if (stackEmpty(AnnStack)) return;

    stackCursor(AnnStack, 0);
    while ((cp = stackNextText(AnnStack))) {
	nlines++;
	if (strlen(cp) > maxwidth) maxwidth = strlen(cp);
    }

    if (!graphActivate(annGraph)) {
	annGraph = graphCreate (TEXT_FULL_SCROLL, "Annotations", 0, 0,
				(maxwidth+1)/fontwidth*screenWidth, 
				(nlines+1)/fontheight*screenHeight);
	graphTextFormat(FIXED_WIDTH);
    }
    graphClear();
    graphTextBounds(maxwidth+1, nlines+1);

    stackCursor(AnnStack, 0);
    while ((cp = stackNextText(AnnStack))) {
	graphText(cp, 0, i++);
    }
    
    graphRedraw();
}


/* The actual tree drawing routine.
   Note: must be in sync with treeDrawNodeBox, which draws clickable
   boxes first.
*/
static float treeDrawNode(treeStruct *tree, treeNode *node, float x) {
    float y, yl, yr;
    int leftcolor;

    if (!node) return 0.0;

    yl = treeDrawNode(tree, node->left, x + node->branchlen*treeScale);
    leftcolor = graphColor(BLACK);
    yr = treeDrawNode(tree, node->right, x + node->branchlen*treeScale);

    if (yl) {
	/* internal node */
	y = (yl + yr) / 2.0;

	/* connect children */
	graphLine(x + node->branchlen*treeScale, yr, x + node->branchlen*treeScale, y);
	graphColor(leftcolor);
	graphLine(x + node->branchlen*treeScale, yl, x + node->branchlen*treeScale, y);

	if (node->left->organism != node->right->organism)
	    graphColor(BLACK);
    }
    else {
	/* Sequence name */
	int box ;

	y = tree_y++;

	if (treeColorsOn && node->organism) {
	    aln.organism = node->organism;

	    if (arrayFind(organismArr, &aln, &ip, (void*)organism_order)) {
		graphColor(arrp(organismArr, ip, ALN)->color);
	    }
	}
	else
	    graphColor(BLACK);

	/* Make clickable box for sequence */
	box = graphBoxStart();
	graphText(node->name, x + node->branchlen*treeScale + 1, y-0.5);
	graphBoxEnd();
	graphAssociate(assVoid(100+box), node);

	if (Highlight_alnp && node->aln == Highlight_alnp) {
	    graphBoxDraw(box, WHITE, BLACK);
	    tree->currentPickedBox = box;
	}
	else if (node->aln) {
	    graphBoxDraw(box, BLACK, node->aln->color);
	}	    

	if (treeShowOrganism && node->organism) 
	    graphText(node->organism, x + node->branchlen*treeScale + 2 + strlen(node->name), y-0.5);
	{
	    int pos = x + node->branchlen*treeScale + strlen(node->name);
	    if (pos > maxTreeWidth) 
		maxTreeWidth = pos;
	}
    }

    /* Horizontal branches */
    graphLine(x + node->branchlen*treeScale, y, x, y);

    if (treeShowBranchlen && node->branchlen) {
	float pos = x + node->branchlen*treeScale*0.5 - strlen(messprintf("%.1f", node->branchlen))*0.5;
	graphText(messprintf("%.1f", node->branchlen), pos, y);
    }

    if (treebootstraps && !node->name && node!=treeHead  && !treebootstrapsDisplay) {
	graphColor(BLUE);
	float pos = x + node->branchlen*treeScale - strlen(messprintf("%.0f", node->boot)) - 0.5;
	if (pos < 0.0) pos = 0;
	printf("%f  %f   \n", node->boot, pos);
	graphText(messprintf("%.0f", node->boot), pos, y);
	graphColor(BLACK);
    }

    return y;
}


/* Draws clickable boxes for horizontal lines.  The routine must be in sync with
   treeDrawNode, but can not be integrated since a box may
   accidentally overwrite some text.  Therefore all boxes must be 
   drawn before any text or lines.
*/
static float treeDrawNodeBox(treeStruct *tree, treeNode *node, float x) {
    float y, yl, yr;
    int box;

    if (!node) return 0.0;

    yl = treeDrawNodeBox(tree, node->left, x + node->branchlen*treeScale);
    yr = treeDrawNodeBox(tree, node->right, x + node->branchlen*treeScale);


    if (yl) {
	y = (yl + yr) / 2.0;
    }
    else {
	y = tree_y++;
    }

    /* Make box around horizontal lines */
    box = graphBoxStart();
    graphLine(x + node->branchlen*treeScale, y, x, y);
    oldlinew = graphLinewidth(0.0); graphColor(BG);
    graphRectangle(x + node->branchlen*treeScale, y-0.5, x, y+0.5);
    graphColor(BLACK); graphLinewidth(oldlinew);
    graphBoxEnd();
    
    graphAssociate(assVoid(100+box), node);

    node->box = box;
    tree->lastNodeBox = box;

    return y;
}

static void treeDraw(treeNode *treeHead)
{
    int i;
    float oldlinew;
    treeStruct *treestruct = messalloc(sizeof(treeStruct));

    treeGraph = graphCreate (TEXT_FULL_SCROLL, 
			     messprintf("Belvu %s using %s distances of %s", 
					treeMethod == NJ ? "Neighbor-joining tree" : "UPGMA tree",
					treeDistString,
					Title),
			     0, 0,
			     (treeMethod == UPGMA ? 130 : 110)/fontwidth*screenWidth, 
			     (nseq+7)/fontheight*screenHeight);
    
    graphTextFormat(FIXED_WIDTH);
    graphRegister(PICK, treeboxPick);
    graphRegister(DESTROY, treeDestroy);

    treestruct->head = treeHead;
    graphAssociate(assVoid(1), treestruct);

    oldlinew = graphLinewidth(treeLinewidth);
    graphClear();
    graphMenu(treeMenu);

    /* setTreeScale(treestruct->head); */

    maxTreeWidth = 0;
    tree_y = 1;
    treeDrawNodeBox(treestruct, treestruct->head, 1.0);
    tree_y = 1;
    treeDrawNode(treestruct, treestruct->head, 1.0);
    graphColor(BLACK);
    graphTextBounds(maxTreeWidth+2 + (treeShowOrganism ? 25 : 0), nseq+6);

    /* Draw scale */
    if (treeMethod == UPGMA) {
	tree_y += 2;
	graphLine(1*treeScale, tree_y, 101*treeScale, tree_y);
	for (i=1; i<=101; i += 10) {
	    graphLine(i*treeScale, tree_y, i*treeScale, tree_y+0.5);
	    if (i < 101) graphLine((i+5)*treeScale, tree_y, (i+5)*treeScale, tree_y+0.5);
	    graphText(messprintf("%d", i-1), (i-0.5)*treeScale, tree_y+1);
	}
    }
    else {
	tree_y += 3;
	graphText("0.1", 5*treeScale, tree_y-1);
	graphLine(1*treeScale, tree_y, 11*treeScale, tree_y);
	graphLine(1*treeScale, tree_y-0.5, 1*treeScale, tree_y+0.5);
	graphLine(11*treeScale, tree_y-0.5, 11*treeScale, tree_y+0.5);
    }
    graphLinewidth(oldlinew);

    if (treeMethod == NJ) {	

	float lweight, rweight;
	treeNode *tree = treestruct->head;

	lweight = treeSize3way(tree->left, tree);
	rweight = treeSize3way(tree->right, tree);

	graphText((debug ? messprintf("Tree balance = %.1f (%.1f-%.1f)", 
			     fabsf(lweight - rweight), lweight, rweight) :
		   messprintf("Tree balance = %.1f", fabsf(lweight - rweight))),
		  14, tree_y-0.5);
    }

    graphRedraw();
}


static void treeSetLinewidth(char *cp)
{
    treeLinewidth = atof(cp);
    treeSettings();
}


static void treeSetScale(char *cp)
{
    treeScale = atof(cp);
    treeSettings();
}


/* Note: this treecpy does not reallocate any strings, assuming they will 
   always exist and never change.  Serious problems await if this is
   not true
*/
static treeNode *treecpy(treeNode *node)
{
    treeNode *newnode;

    if (!node) return 0;

    newnode = messalloc(sizeof(treeNode));

    newnode->dist      = node->dist;
    newnode->branchlen = node->branchlen;	
    newnode->boot      = node->boot;
    newnode->name      = node->name;
    newnode->organism  = node->organism;
    newnode->aln       = node->aln;
    newnode->box       = node->box;
    newnode->color     = node->color;

    newnode->left      = treecpy(node->left);
    newnode->right     = treecpy(node->right);

    return newnode;
}


static void fillParents(treeNode *parent, treeNode *node)
{
    if (!node) return;

    node->parent = parent;
    fillParents(node, node->left);
    fillParents(node, node->right);
}


static char *fillOrganism(treeNode *node)
{
    char 
	*leftorganism,
	*rightorganism;

    if (node->name) return node->organism;

    leftorganism = fillOrganism(node->left);
    rightorganism = fillOrganism(node->right);

    /* printf("\nFill: (left=%s, right=%s):  ", leftorganism, rightorganism); */

    node->organism = (leftorganism == rightorganism ? leftorganism : 0);

    return node->organism;
}


static treeNode *treeParent2leaf(treeNode *newparent, treeNode *curr)
{
    if (!curr->parent) { /* i.e. the old root */
	if (curr->left == newparent) {
	    newparent->branchlen += curr->right->branchlen; /* Add second part of this vertex */
	    return curr->right;
	}
	else {
	    newparent->branchlen += curr->left->branchlen;
	    return curr->left;
	}
    } 
    else {
	if (curr->left == newparent) {
	    /* Change the link to the new parent to the old parent */
	    curr->left = treeParent2leaf(curr, curr->parent);
	    curr->left->branchlen = curr->branchlen;
	}
	else {
	    curr->right = treeParent2leaf(curr, curr->parent);
	    curr->right->branchlen = curr->branchlen;
	}
    }
    return curr;
}


/* Rerooting works roughly this way:

   - A new node is created with one child being the node chosen as new root 
     and the other child the previous parent of it.  The sum of left and right
     branch lengths of the new root should equal the branch length of the chosen node.
   - All previous parent nodes are visited and converted so that:
         1. The previous parent node becomes a child.
	 2. The new parent is the node that calls.
	 3. The branchlength of the previous parent node is assigned to its parent.
   - When the previous root is reached, it is deleted and the other child
     becomes the child of the calling node.

     Note that treeReroot destroys the old tree, since it reuses the nodes.  
     Use treecpy if you still need it for something later.
*/
static treeNode *treeReroot(treeNode *node)
{
    treeNode *newroot = messalloc(sizeof(treeNode));

    newroot->left = node;
    newroot->right = treeParent2leaf(node, node->parent);

    fillParents(newroot, newroot->left);
    fillParents(newroot, newroot->right);

    newroot->left->branchlen = newroot->right->branchlen = node->branchlen / 2.0;
    treeBalanceByWeight(newroot->left, newroot->right, 
			&newroot->left->branchlen, &newroot->right->branchlen);

    treeHead = newroot;
    return newroot;
}


static void treeSwapNode(treeNode *node)
{
    void *tmp;

    tmp = node->left;
    node->left = node->right;
    node->right = tmp;
}


static void treeTraverse(treeNode *node, void (*func)()) {
    if (!node) return;

    treeTraverse(node->left, func);
    func(node);
    treeTraverse(node->right, func);
}


static void treeTraverseLRfirst(treeNode *node, void (*func)()) {
    if (!node) return;

    treeTraverseLRfirst(node->left, func);
    treeTraverseLRfirst(node->right, func);
    func(node);
}


/* Sum branchlengths, allow going up parents

   If going up parents, arg1 = parent  ;  arg2 = child.

   If you're not interested in going up parents, simply call me with
   the same node in both arguments.
   
 */
static float treeSize3way(treeNode *node, treeNode *fromnode) {

    int root = 0;
    treeNode *left, *right;
    float len;
    
    if (!node) return 0.0;

    /* Get the correct branch length */
    if (node->left == fromnode || node->right == fromnode)
	/* Going up the tree */
	len = fromnode->branchlen;
    else
	len = node->branchlen;

    if (node->left == fromnode) {
	left = node->parent;
	if (!left) root = 1;
    }
    else {
	left = node->left;
    }

    if (node->right == fromnode) {
	right = node->parent;
	if (!right) root = 1;
    }
    else {
	right = node->right;
    }

    if (root) {
	float retval;

	/* go across tree root */
	if (left) /* Coming from right */
	    retval = treeSize3way(left, left);
	else  /* Coming from left */
	    retval = treeSize3way(right, right);

	if (debug)
	    printf("Returning (%.1f + %.1f) = %.1f\n", 
		   fromnode->branchlen, retval, fromnode->branchlen + retval);

	return fromnode->branchlen + retval;
    }
    else {
	float 
	    l = treeSize3way(left, node),
	    r = treeSize3way(right, node),
	    retval = (l + r)/2.0 + len;

	if (debug)
	    printf("Returning (%.1f + %.1f)/2 + %.1f = %.1f\n", l, r, len, retval);

	return retval;
    }
}


/* Calculate the difference between left and right trees if the tree
   were to be rerooted at this node.

   What is calculated (bal) is the difference between 'left' and
   'right' subtrees minus the length of the branch itself. If this
   difference is negative, a perfectly balanced tree can be made.  For
   imperfectly balanced trees we want to root at the branch that gives
   the best balance.  However, perfectly balanced trees are all
   'perfect' so here we chose the branch with most equal subtrees.

   Actually it is not "left" and "right" but "down" and "up" subtrees.  */
static void treeCalcBalance(treeNode *node) {

    float bal, lweight, rweight;

    if (node == treeBestBalancedNode) return;

    if (debug) printf("Left/Downstream weight\n");
    lweight = treeSize3way(node, node);

    if (debug) printf("Right/Upstream weight\n");
    rweight = treeSize3way(node->parent, node);

    bal = fabsf(lweight - rweight) - node->branchlen;

    if (debug) printf("Node=%s (branchlen = %.1f).  Weights = %.1f  %.1f. Bal = %.1f\n", 
		      node->name, node->branchlen, lweight, rweight, bal);

    if (bal < treeBestBalance) { /* better balance */

	if (treeBestBalance > 0.0 ||
	    /* If previous tree was not perfectly balanced, or
	       If previous tree was perfectly balanced - choose root with best subtree balance */
	    fabsf(lweight - rweight) < treeBestBalance_subtrees)
	{
	    if (debug) printf("            %s has better balance %.1f < %.1f\n", 
			      node->name, bal, treeBestBalance);
	    
	    treeBestBalancedNode = node;
	    treeBestBalance = bal;
	    treeBestBalance_subtrees = fabsf(lweight - rweight);
	}
    }
}


/* Find the node which has most equal balance, return new tree with this as root.
 */
static treeNode *treeFindBalance(treeNode *tree) {

    float lweight, rweight;

    lweight = treeSize3way(tree->left, tree->left);
    rweight = treeSize3way(tree->right, tree->right);

    treeBestBalancedNode = tree;
    treeBestBalance = fabsf(lweight - rweight);
    treeBestBalance_subtrees = fabsf((lweight - tree->left->branchlen) - 
				     (rweight - tree->right->branchlen));

    if (debug) printf("Initial weights = %.1f  %.1f. Bal = %.1f\n", 
		      lweight, rweight, treeBestBalance);

    treeTraverse(tree, treeCalcBalance);

    if (treeBestBalancedNode == tree)
	return tree;
    else
	return treeReroot(treeBestBalancedNode);
}

static int strcmp_(char **x, char **y)
{
    int retval = strcmp(*x, *y);
    return retval;
}

/* Combines left and right sequence groups and insert to bootstrap group list */
static Array fillBootstrapGroups(treeNode *node, treeNode *rootnode, int maintree) {
    int i;
    int ssize=0;
    BOOTSTRAPGROUP *BS = (BOOTSTRAPGROUP *)messalloc(sizeof(BOOTSTRAPGROUP));

    if (!node->name) {
	/* Internal node */
	/* Combine left node sequences into right node array */

	Array left = fillBootstrapGroups(node->left, rootnode, maintree);
	Array right = fillBootstrapGroups(node->right, rootnode, maintree);

	/* No need to do root */
	if (node == rootnode)
	  return NULL ;

	/* Combine left and right groups */
	for (i = 0 ; i < arrayMax(left); ++i) {
	    char *s = arr(left, i, char *);
	    arrayInsert(right, &s, (void *)strcmp_);
	}
	arrayDestroy(left);

	/* Create string with group members */
	for (i = 0 ; i < arrayMax(right) ; ++i) ssize += (strlen(arr(right, i, char *))+1);
	BS->s = messalloc(ssize+1);
	for (i = 0 ; i < arrayMax(right) ; ++i) {
	    strcat(BS->s, arr(right, i, char *));
	    strcat(BS->s, " ");
	}

	/* printf("   New bootstrap group: %s\n", BS->s); */
	
	if (maintree) {
	    /* Associate this node with the group string */
	    BS->node =  node;
	    
	    /* Add group string to array of bootstrap groups */
	    arrayInsert(bootstrapGroups, BS, (void *)BSorder);
	}
	else {
	    /* Find group string and increment counter if exists */
	    BOOTSTRAPGROUP *BS2;
	    
	    if (arrayFind(bootstrapGroups, BS, &ip, (void *)BSorder)) {
		BS2 = arrp(bootstrapGroups, ip, BOOTSTRAPGROUP);
		BS2->node->boot++;
		/* printf("Found bootgroup %s\n", BS->s); */
	    }
	    else
		/* printf("Did not find bootgroup %s\n", BS->s); */
	    
	    messfree(BS->s);
	    messfree(BS);
	}
	return right;
    }
    else {
	/* Leaf node - return as embryonic array with one entry */
	Array leaf = arrayCreate(nseq, char *);
	arrayInsert(leaf, &node->name,  (void *)strcmp_);
	return leaf;
    }
}

static void normaliseBootstraps(treeNode *node) {
    node->boot = 100*node->boot/(float)treebootstraps;
}


/* Balance the two sides of a tree branch by the weights of the two subtrees.
   The branch has two sides: llen and rlen.

   Rationale: The correct center point balances the treesizes on both sides.
   
   Method: Calculate half the unbalance, add it to the appropriate side.

   No real theory for this, but it seems to work in easy cases 
*/
static void treeBalanceByWeight(treeNode *lnode, treeNode *rnode, float *llen, float *rlen)
{
    float adhocRatio = 0.95;

    float 
	halfbal, 
	branchlen = *rlen+*llen;
    float lweight = treeSize3way(lnode, lnode) /*- lnode->branchlen*/;
    float rweight = treeSize3way(rnode, rnode) /*- rnode->branchlen*/;

    halfbal = fabsf(lweight-rweight) / 2.0;

    if (halfbal < *llen && halfbal < *rlen) {

	if (lweight < rweight) {
	    *llen += halfbal;
	    *rlen -= halfbal;
	}
	else {
	    *llen -= halfbal;
	    *rlen += halfbal;
	}
    }
    else {
	/* The difference is larger than the final branch -
	   give nearly all weight to the shorter one (cosmetic hack) */
	if (lweight < rweight) {
	    *llen = branchlen*adhocRatio;
	    *rlen = branchlen*(1.0 - adhocRatio);
	}
	else {
	    *rlen = branchlen*adhocRatio;
	    *llen = branchlen*(1.0 - adhocRatio);
	}
    }
}


/* Print tree in New Hampshire format */
static void treePrintNH(treeStruct *tree, treeNode *node, FILE *file) {
    if (!node) return;
    
    if (node->left && node->right) {
	fprintf(file, "(\n");
	treePrintNH(tree, node->left, file);
	fprintf(file, ",\n");
	treePrintNH(tree, node->right, file);
	fprintf(file, ")\n");
	if (node != tree->head)	/* Not exactly sure why this is necessary, but njplot crashes otherwise */
	    fprintf(file, "%.0f", node->boot+0.5);
    }
    else {
        fprintf(file, "%s", node->name);
    }
    if (node != tree->head)	/* Not exactly sure why this is necessary, but njplot crashes otherwise */
	fprintf(file, ":%.3f", node->branchlen/100.0);
}

/* Save tree in New Hampshire format */
static void treePrintNH_init(void) {
    FILE *file;
    treeStruct *treestruct;
    
    if (!(file = filqueryopen(dirName, fileName, "","w", "Write New Hampshire format tree to file:"))) 
      return;

    if (!graphAssFind(assVoid(1), &treestruct))
      {
	messout("Could not find tree for this graph");
	return;
      }

    treePrintNH(treestruct, treestruct->head, file);
    fprintf(file, ";\n");
    fclose(file);

    return ;
}


static void treePrintNode(treeNode *node) {
    if (node->name) printf("%s ", node->name);
}


static void treeFindOrthologsRecur(treeStruct *treestruct, treeNode *node, char *org1, char *org2) {

    if (!node || !node->left || !node->right) return;

    if (debug) {
	printf("\n 1 (%s, seq=%s):  ", node->left->organism, node->left->name);
	printf("\n 2 (%s, seq=%s)\n: ", node->right->organism, node->right->name);
    }

    /* The open-minded way */
    if (node->left->organism && node->right->organism &&
	node->left->organism != node->right->organism) {
	
	graphBoxDraw(node->box, WHITE, BLACK);
	printf("\nSpecies 1 (%s):  ", node->left->organism);
	treeTraverse(node->left, treePrintNode);
	printf("\nSpecies 2 (%s): ", node->right->organism);
	treeTraverse(node->right, treePrintNode);
	printf("\n");
    }
    
    /* The narrow-minded way * /
    if ((node->left->organism == org1 && node->right->organism == org2) ||
	(node->left->organism == org2 && node->right->organism == org1)) {
	
	printf("Found orthologs");
    }*/

    else {
	treeFindOrthologsRecur(treestruct, node->left, org1, org2);
	treeFindOrthologsRecur(treestruct, node->right, org1, org2);
    }    
}

static void treeFindOrthologs(void) {
    treeStruct *treestruct;
    char *org1 = NULL, *org2 = NULL ;

    
    if (!graphAssFind(assVoid(1), &treestruct))
      {
	messout("Could not find tree for this graph");
	return;
      }

    /* Dialog to define org1 and org2 */

    treeFindOrthologsRecur(treestruct, treestruct->head, org1, org2);
}


static void treeOrder(treeNode *node) {
    if (!node) return;
    
    treeOrder(node->left);
    if (node->aln)
	node->aln->nr = treeOrderNr++;
    treeOrder(node->right);
}


static void printMtx(float **mtx) {
    int i, j;

    printf ("\n");
    for (i = 0; i < nseq; i++) {
	for (j = 0; j < nseq; j++)
	    printf("%6.2f ", mtx[i][j]);
	printf ("\n");
    }
}


/* Correct an observed distance to an estimated 'true' distance.

   Adapted from Lasse Arvestad's lapd program

   od = observed distance in percent 

*/
static float treeJUKESCANTOR(float od)
{
    float cd;

    od /= 100;

    od = 20.0/19.0*od;
    if  (od > 0.95) od = 0.95; /* Limit to 300 PAM */

    cd = -19.0/20.0 * log(1-od);

    return cd*100;
}



/* Correct an observed distance to an estimated 'true' distance.

   Adapted from Lasse Arvestad's lapd program

   od = observed distance in percent 

*/
static float treeKimura(float od)
{
    float adjusted, cd;

    od /= 100;

    adjusted = od + 0.2 * od*od;

    if (adjusted > 0.95) adjusted = 0.95;   /* Limit to 300 PAM */

    cd = -log(1 - adjusted);

    return cd*100;
}



/* Correct an observed distance to an estimated 'true' distance.

   Based on Christian Storm's 5th order polynomial curve fitting of ROSE simulated data

   od = observed distance in percent 

*/
static float treeSTORMSONN(float od)
{
    float cd;			/* Corrected distance */

    if (od > 91.6) return 1000.0;	/* Otherwise log of negative value below */

    od /= 100;

    cd= -log(1 
             -0.95844*od 
             -0.69957*od*od
             +2.4955*od*od*od
             -4.6353*od*od*od*od
             +2.8076*od*od*od*od*od);

    /* printf(" od=%f  cd=%f\n", od, cd); */
   
    if (cd > 3.0) cd=3.0; /* Limit to 300 PAM */
    
    return cd*100;
}

static float treeScoredist(char *seq1, char *seq2)
{
    int len,
	sc = 0, 
	s1sc = 0, 
	s2sc = 0;
    float od, cd, 
	maxsc, 
	expect; 
    char *s1, *s2;
    
#define mtx BLOSUM62
    

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
    if (!penalize_gaps) {

	/* Remove gappy regions from alignment */
	s1 = messalloc(strlen(seq1)+1);
	s2 = messalloc(strlen(seq2)+1);
	
	for (len=0, i=0; i < strlen(seq1); i++) {
	    if (!isGap(seq1[i]) && !isGap(seq2[i])) {
		s1[len] = seq1[i];
		s2[len] = seq2[i];
		len++;
	    }
	}
    }
    else
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */
    {
      s1 = seq1;
      s2 = seq2;
    }

    /* Calc scores */
    for (len=0; *s1; s1++, s2++) {
	if (penalize_gaps) {
	    if (isGap(*s1) || isGap(*s2)) sc -= 0.6;	
	}

	if (!isGap(*s1) && !isGap(*s2)) {
	  sc += mtx[a2b[(unsigned char)(*s1)]-1][a2b[(unsigned char)(*s2)]-1];
	  s1sc += mtx[a2b[(unsigned char)(*s1)]-1][a2b[(unsigned char)(*s1)]-1];
	  s2sc += mtx[a2b[(unsigned char)(*s2)]-1][a2b[(unsigned char)(*s2)]-1];
	    len++;
	}
    }
    
    maxsc = (s1sc + s2sc) / 2.0;
    
    /* Calc expected score */
    expect =  -0.5209 * len;

    od = ((float)sc - expect) / (maxsc - expect);

    if (!len || od < 0.05) od = 0.05;  /* Limit to 300 PAM;  len==0 if no overlap */
    if (od > 1.0) od = 1.0; 
    
    cd = -log(od);

    /* printf ("len=%d  sc=%.2f  maxsc=%.2f  expect=%.2f  maxsc-expect=%.2f  od=%.3f\n", len, sc, maxsc, expect, maxsc-expect, od); */

    cd = cd * 100.0 * 1.337 /* Magic scaling factor optimized for Dayhoff data */ ;

    if (cd > 300) cd = 300;  /* Limit to 300 PAM */
    
/*    if (!penalize_gaps) {
	messfree(s1);
	messfree(s2);
    } */

    return cd;
}



#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
static float tree_Scoredistold(char *s1, char *s2)
{
    float od, cd, maxsc, sc;
    
#define mtx BLOSUM62

    /* Calc score */
    sc = mtxscore(s1, s2, mtx);

    /* Calc max score */
    maxsc = (mtxscore(s1, s1, mtx) + mtxscore(s2, s2, mtx)) / 2.0;
    
    od = 1.0 - sc/maxsc;
    /* od = 19.0/20.0*(1.0 - sc/maxsc); */

    if (od < 0.0) od = 0.0;
    if (od > 0.95) od = 0.95;  /* Limit to 300 PAM */
    
    cd = -log(1 - od);
    /*cd = -log(1 - od) * 20/19; */

    /* printf ("sc=%.2f  maxsc=%.2f  od=%.3f\n", sc, maxsc, od); */

    return cd * 100.0;
}
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */


static treeNode *treeMake(int doBootstrap)
{
  treeNode *newnode = NULL ;
  int i, j, iter, maxi, maxj;
  ALN *alni, *alnj;
  float maxid = 0.0, pmaxid, **pairmtx, **Dmtx, **mtx, *src, *trg, 
    *avgdist,		/* vector r in Durbin et al */
    llen, rlen;
  treeNode **node ;					    /* Array of (primary) nodes.  Value=0 => stale column */
  static STORE_HANDLE  
	Handle = 0;
  STORE_HANDLE treeHandle = NULL ;

  if (doBootstrap)
      treeHandle = Handle;
    
  /* Allocate memory */
  if (treeHandle)
    handleDestroy(treeHandle);

  treeHandle = handleCreate();
  pairmtx = handleAlloc(0, treeHandle, nseq*sizeof(float *));
  Dmtx = handleAlloc(0, treeHandle, nseq*sizeof(float *));
  node = handleAlloc(0, treeHandle, nseq*sizeof(treeNode *));
  avgdist = handleAlloc(0, treeHandle, nseq*sizeof(float));

  for (i = 0; i < nseq; i++)
    {
      pairmtx[i] = handleAlloc(0, treeHandle, nseq*sizeof(float));
      Dmtx[i] = handleAlloc(0, treeHandle, nseq*sizeof(float));
      node[i] = handleAlloc(0, treeHandle, sizeof(treeNode));
      node[i]->name =  handleAlloc(0, treeHandle, strlen(arrp(Align, i, ALN)->name)+50);
      alni = arrp(Align, i, ALN);
      if (!treeCoordsOn) 
	{
	  sprintf(node[i]->name, "%s", alni->name);
	}
      else
	{
	  sprintf(node[i]->name, "%s/%d-%d", 
		  alni->name,
		  alni->start,
		  alni->end);
	}
      node[i]->aln = alni;
      node[i]->organism = alni->organism;
    }
    
	
    if (treeReadDistancesON) {
	treeReadDistances(pairmtx);
    }
    else {
  /* Calculate pairwise distance matrix */
  arrayOrder();
  for (i = 0; i < nseq-1; i++)
    {
      alni = arrp(Align, i, ALN);
      for (j = i+1; j < nseq; j++)
	{
	  alnj = arrp(Align, j, ALN);
	  pairmtx[i][j] = 100.0 - identity(alni->seq, alnj->seq);
	  if (treeDistCorr == KIMURA) pairmtx[i][j] = treeKimura(pairmtx[i][j]);
          else if (treeDistCorr == JUKESCANTOR) pairmtx[i][j] = treeJUKESCANTOR(pairmtx[i][j]);
	  else if (treeDistCorr == STORMSONN) pairmtx[i][j] = treeSTORMSONN(pairmtx[i][j]);
          else if (treeDistCorr == Scoredist) pairmtx[i][j] = treeScoredist(alni->seq, alnj->seq);
	}
      }
    }

    if (treePrintDistances) {
	float dist;

	/* 
	   printMtx(pairmtx);
	   exit(0);
	*/

	for (i = 0; i < nseq; i++) {

	    if (!treeCoordsOn) {
		printf("%s\t", arrp(Align, i, ALN)->name);
	    }
	    else {
		printf("%s/%d-%d\t",
				  arrp(Align, i, ALN)->name,
				  arrp(Align, i, ALN)->start,
				  arrp(Align, i, ALN)->end);
	    }
	}
	printf ("\n");

	for (i = 0; i < nseq; i++) {
	    for (j = 0; j < nseq; j++) {
		if (i == j) 
		    dist = 0.0;
		else if (i < j)
		    dist = pairmtx[i][j];
		else
		    dist = pairmtx[j][i];
				
		printf("%7.3f\t", dist);
	    }
	    printf ("\n");
	}
	exit(0);
    }
    

  /* Construct tree */
  for (iter = 0; iter < nseq-1; iter++)
    {
      if (treeMethod == NJ)
	{	
	  int count;

	  /* Calculate vector r (avgdist) */
	  for (i = 0; i < nseq; i++)
	    {
	      if (!node[i]) continue;
	      avgdist[i] = 0.0;
	      for (count=0, j = 0; j < nseq; j++)
		{
		  if (!node[j]) continue;
		  avgdist[i] += (i < j ? pairmtx[i][j] : pairmtx[j][i]);
		  count++;
		}
	      if (count == 2)	/* Hack, to accommodate last node */
		avgdist[i] = 1;
	      else
		avgdist[i] /= 1.0*(count - 2);
	    }

	  /* Calculate corrected matrix Dij */
	  if (1 /* gjm */)
	    {
	      for (i = 0; i < nseq-1; i++)
		{
		  if (!node[i])
		    continue;

		  for (j = i+1; j < nseq; j++)
		    {
		      if (!node[j]) continue;
		      Dmtx[i][j] = pairmtx[i][j] - (avgdist[i] + avgdist[j]);
		    }
		}
	    }
	  else
	    {		/* Felsenstein */
	      float Q = 0.0;

	      for (i = 0; i < nseq-1; i++)
		{
		  if (!node[i])
		    continue;
		  for (j = i+1; j < nseq; j++)
		    {
		      if (!node[j]) continue;
		      Q += pairmtx[i][j];
		    }
		}
		
	      for (i = 0; i < nseq-1; i++)
		{
		  if (!node[i])
		    continue;
		  for (j = i+1; j < nseq; j++)
		    {
		      if (!node[j])
			continue;
		      Dmtx[i][j] = (pairmtx[i][j] +
				    (2.0*Q)/(count-(count == 2 ? 1 : 2)) - 
				    avgdist[i] - avgdist[j]) / 2.0;
		    }
		}
	    }
	  mtx = Dmtx;
	}
      else 
	mtx = pairmtx;

      if (debug)
	{
	  printf("Node status, Avg distance:\n");
	  for (i = 0; i < nseq; i++)
	    printf("%6d ", (node[i] ? 1 : 0)); 
	  printf("\n");
	  for (i = 0; i < nseq; i++)
	    printf("%6.2f ", avgdist[i]); 
	  printf("\n\nPairdistances, corrected pairdistances:");
	  printMtx(pairmtx);
	  printMtx(Dmtx);
	  printf("\n");
	}

      /* Find smallest distance pair in pairmtx */
      maxi = -1;
      maxj = -1;
      maxid = pmaxid = 1000000;
      for (i = 0; i < nseq-1; i++) {
	if (!node[i]) continue;
	for (j = i+1; j < nseq; j++) {
	  if (!node[j]) continue;
	  /* printf("iter %d, i=%d. j=%d, dist= %f\n", iter, i, j, mtx[i][j]);*/
	  if (mtx[i][j] < maxid) {
	    maxid = mtx[i][j];
	    pmaxid = pairmtx[i][j];
	    maxi = i;
	    maxj = j;
	  }
	  else if (treeMethod == NJ && Dmtx[i][j] == maxid && pairmtx[i][j] < pmaxid) {
	    /* To resolve ties - important for tree look! */
	    maxi = i;
	    maxj = j;
	  }
	}
      }

      maxid = pairmtx[maxi][maxj]; /* Don't want to point to Dmtx in NJ */

      /* Merge rows & columns of maxi and maxj into maxi
	 Recalculate distances to other nodes */
      for (i = 0; i < nseq; i++) {
	if (!node[i]) continue;

	if (i < maxi) 
	  trg = &pairmtx[i][maxi];
	else if (i > maxi) 
	  trg = &pairmtx[maxi][i];
	else continue;

	if (i < maxj) 
	  src = &pairmtx[i][maxj];
	else if (i > maxj) 
	  src = &pairmtx[maxj][i];
	else continue;
	    
	if (treeMethod == UPGMA) 
	  *trg = (*trg + *src) / 2.0;
	else
	  *trg = (*trg + *src - maxid) / 2.0;
      }

      /* Create node for maxi and maxj */
      newnode = messalloc(sizeof(treeNode));

      if (treeMethod == UPGMA)
	{
	  /* subtract lower branch lengths from absolute distance
	     Horribly ugly, only to be able to share code UPGMA and NJ */
	  treeNode *tmpnode = node[maxi]->left;

	  for (llen = maxid; tmpnode; tmpnode = tmpnode->left)
	    llen -= tmpnode->branchlen;

	  tmpnode = node[maxj]->right;
	  for (rlen = maxid; tmpnode; tmpnode = tmpnode->right)
	    rlen -= tmpnode->branchlen;
	}
      else
	{
	  llen = (maxid + avgdist[maxi] - avgdist[maxj]) / 2.0;
	  rlen = maxid - llen;
	    
	  if (iter == nseq-2)
	    { /* Root node */

	      /* Not necessary anymore, the tree is re-balanced at the end which calls this too
		 treeBalanceByWeight(node[maxi], node[maxj], &llen, &rlen);*/
		
	      /* Put entire length of root branch in one leg so the rebalancing 
		 will work properly (otherwise it is hard to take this branch into account */
	      rlen += llen;
	      llen = 0;
	    }
	}
	
      if (debug)
	{
	  printf("Iter %d: Merging %d and %d, dist= %f\n", 
		 iter, maxi+1, maxj+1, mtx[maxi][maxj]);
	  printf("maxid= %f  llen= %f  rlen= %f\n", maxid, llen, rlen);
	  printf("avgdist[left]= %f  avgdist[right]= %f\n\n", 
		 avgdist[maxi], avgdist[maxj]);
	}

      newnode->left = node[maxi];	
      newnode->left->branchlen = llen;

      newnode->right = node[maxj];
      newnode->right->branchlen = rlen;

      newnode->organism = (node[maxi]->organism == node[maxj]->organism ?
			   node[maxi]->organism : 0);
	
      node[maxi] = newnode;
      node[maxj] = 0;
    }

  fillParents(newnode, newnode->left);
  fillParents(newnode, newnode->right);

  if (debug)
    treeDraw(newnode) ;

  if (treeMethod == UPGMA)
    newnode->branchlen = 100 - maxid ;

  if (treeMethod == NJ)
    newnode = treeFindBalance(newnode) ;
    
  fillOrganism(newnode);
  if (doBootstrap && treebootstraps) treeBootstrapStats(newnode);
    
  return newnode ;
}


static void treeDisplay(void)
{
    separateMarkupLines();
    treeHead = treeMake(1);
    treeDraw(treeHead);
    reInsertMarkupLines();
}


static void treeSortBatch(void)
{
    separateMarkupLines();

    treeHead = treeMake(0);
    treeOrderNr = 1;
    treeOrder(treeHead); /* Set nr field according to tree order */
    arraySort(Align, (void*)nrorder);

    reInsertMarkupLines();
}

/* Find back the ALN pointers lost in treeSortBatch
*/
static void treeFindAln(treeNode *node) {

    if (!node->name) return;

    str2aln(node->name, &aln);

    if (!alignFind(Align, &aln, &ip)) {
	messout("Cannot find back seq after sort - probably a bug");
    }
    else
	node->aln = arrp(Align, ip, ALN);
}

static void treeSort(void)
{
    ALN aln;

    if (Highlight_alnp)
	alncpy(&aln, Highlight_alnp);
    
    treeSortBatch();

    /* After treeSortBatch the treeNodes point to the wrong ALN,
       because arraySort only copies contents and not entire
       structs. This can be fixed by either a linear string search or
       by remaking the tree... */
    treeTraverse(treeHead, treeFindAln);

    if (Highlight_alnp) {
	if (!alignFind(Align, &aln, &ip)) {
	    messout("Cannot find back highlighted seq after sort - probably a bug");
	    Highlight_alnp = 0;
	}
	else
	    Highlight_alnp = arrp(Align, ip, ALN);
	centerHighlighted();
    }
    
    treeDraw(treeHead);

    belvuRedraw();
}


static void columnCopy(Array Aligndest, int dest, Array Alignsrc, int src)
{
    int i;

    for (i = 0; i < nseq; i++)
	arrp(Aligndest, i, ALN)->seq[dest] = arrp(Alignsrc, i, ALN)->seq[src];
}


/* Bootstrap the internal nodes in a tree.
   1. Set up a list (array) of all internal nodes with leaf content and pointer to node
   2. In bootstrap tree, for each internal node, check if its contents exists in list. If so, increment node's bootstrap count
   3. Turn increments to percentages
*/
static void treeBootstrapStats(treeNode *tree)
{
    /* Traverse tree, fill array bootstrapGroups */
    bootstrapGroups = arrayCreate(nseq, BOOTSTRAPGROUP);
    fillBootstrapGroups(tree, tree, 1);
  
    treeBootstrap();
    
    /* Turn increments to percentages */
    treeTraverse(tree, normaliseBootstraps);
}

static void treeBootstrap(void)
{
    Array Aligntmp;
    int i, iter, src;
    treeStruct *treestruct = messalloc(sizeof(treeStruct));

    separateMarkupLines();
    Aligntmp = arrayCopy(Align);
    for (i = 0; i < nseq; i++) {
	arrp(Aligntmp, i, ALN)->seq = messalloc(strlen(arrp(Align, i, ALN)->seq)+1);
	strcpy(arrp(Aligntmp, i, ALN)->seq, arrp(Align, i, ALN)->seq);
    }
#if defined(__CYGWIN__) || defined(DARWIN)
    srand(time(0));
#else
    srand48(time(0));
#endif
    for (iter = 0; iter < treebootstraps; iter++) {

	/* Make bootstrapped Alignment */
	for (i = 0; i < maxLen; i++) {
#if defined(__CYGWIN__) || defined(DARWIN)
	    src = (int)((((float)rand())/RAND_MAX)*maxLen);
#else
	    src = (int)(drand48()*maxLen);
#endif

	    /* printf("Bootstrap # %d : %d\n", i, src); */
	    
	    columnCopy(Align, i, Aligntmp, src);
	}
	/* printf("&\n"); */


	treestruct->head = treeMake(0);

	if (outputBootstrapTrees) {
	    if (treebootstrapsDisplay) 
	        treeDraw(treestruct->head);
	    else {
	       printf("\n");
	       treePrintNH(treestruct, treestruct->head, stdout);
	       printf(";\n");
	    }
        }
	else {
	    /* Collect bootstrap statistics */
	    fillBootstrapGroups(treestruct->head, treestruct->head, 0);
	}
    }
 
    /* Restore alignment */
    for (i = 0; i < nseq; i++) {
	strcpy(arrp(Align, i, ALN)->seq, arrp(Aligntmp, i, ALN)->seq);
    }
}


static void subfamilyTrav(treeNode *node) {

    static float dist;
    static int 
	newgroup = 1,
	groupnr = 0;

    if (!node) return;

    if (node->name) {
	dist = node->branchlen;

	if (newgroup) {
	    printf("\nGroup nr %d:\n", ++groupnr);
	    newgroup = 0;
	}
	printf("%s\n", node->name);
    }
    else {
	/* internal node */
	dist += node->branchlen;
    }

    if ( mksubfamilies_cutoff > (100.0-dist) ) { newgroup = 1; }

    /* printf("abs=%.1f  branch=%.1f\n", dist, node->branchlen); */
}


static void mksubfamilies(float cutoff)
{
    treeStruct *treestruct = messalloc(sizeof(treeStruct));

    separateMarkupLines();

    strcpy(treeMethodString, UPGMAstr);
    treeMethod = UPGMA;

    treestruct->head = treeMake(0);

    treeTraverseLRfirst(treestruct->head, subfamilyTrav);
}


static void highlightScoreSort(char mode)
{
    int i, len;

    if (!Highlight_alnp) {
	messout("Please highlight a sequence first");
	return;
    }
    if (Highlight_alnp->markup) {
	messout("Please do not highlight a markup line");
	return;
    }
    alncpy(&aln, Highlight_alnp);
    
    separateMarkupLines();

    /* if (displayScores) {
	if (!(graphQuery("This will erase the current scores, do you want to continue?")))
	    return;
    }*/
    displayScores = 1;

    /* Calculate score relative to highlighted sequence */
    for (i = 0; i < nseq; i++) {
	if (mode == 'P')
	    arrp(Align, i, ALN)->score = score(aln.seq, arrp(Align, i, ALN)->seq);
	else if (mode == 'I')
	    arrp(Align, i, ALN)->score = identity(aln.seq, arrp(Align, i, ALN)->seq);

	if ((len=strlen(messprintf("%.1f", arrp(Align, i, ALN)->score))) > maxScoreLen) maxScoreLen = len;
    }

    arraySort(Align, (void*)scoreorderRev);

    reInsertMarkupLines();

    if (!alignFind(Align, &aln, &i)) {
	messout("Cannot find back highlighted seq after sort - probably a bug");
	Highlight_alnp = 0;
    }
    else
	Highlight_alnp = arrp(Align, i, ALN);
    
    arrayOrder();
    AlignYstart = 0;
    belvuRedraw();
}


static void simSort(void) {
    highlightScoreSort('P');
}

static void idSort(void) {
    highlightScoreSort('I');
}

static void scoreSortBatch(void)
{
    if (!displayScores) messcrash("No scores available");

    arraySort(Align, (void*)scoreorder);
    arrayOrder();
}


static void scoreSort(void)
{
    if (Highlight_alnp)
	alncpy(&aln, Highlight_alnp);

    arraySort(Align, (void*)scoreorder);

    if (Highlight_alnp) {
	if (!alignFind(Align, &aln, &ip)) {
	    messout("Cannot find back highlighted seq after sort - probably a bug");
	    Highlight_alnp = 0;
	}
	else
	    Highlight_alnp = arrp(Align, ip, ALN);
    }

    arrayOrder();
    AlignYstart = 0;
    belvuRedraw();
}


static void printScore(void)
{
    if (!Highlight_alnp) {
	messout("Select a line first");
	return;
    }

    printf("%.1f %s/%d-%d\n", 
	   Highlight_alnp->score,
	   Highlight_alnp->name,
	   Highlight_alnp->start,
	   Highlight_alnp->end);
    fflush(stdout);
}


int GCGchecksum(char *seq)
{
    int  
	check = 0, 
	count = 0, 
	i;

    for (i = 0; i < maxLen; i++) {
	count++;
	check += count * toupper((int) seq[i]);
	if (count == 57) count = 0;
    }
    return (check % 10000);
}


int GCGgrandchecksum(void)
{
    int 
	i,
	grand_checksum=0;

    for(i=0; i < nseq; i++) 
	grand_checksum += GCGchecksum(arrp(Align, i, ALN)->seq);

    return (grand_checksum % 10000);
}


void writeMSF(FILE *pipe) /* c = separator between name and coordinates */
{
    int 
	i, j,
	maxfullnamelen = maxNameLen+1+maxStartLen+1+maxEndLen,
	paragraph=0, alnstart, alnend, alnlen, linelen=50, blocklen=10;

    if (saveCoordsOn)
	maxfullnamelen = maxNameLen+1+maxStartLen+1+maxEndLen;
    else
	maxfullnamelen = maxNameLen;

    /* Title */
    fprintf(pipe, "PileUp\n\n %s  MSF: %d  Type: X  Check: %d  ..\n\n",
	    Title, maxLen, GCGgrandchecksum());
    

    /* Names and checksums */
    for(i=0; i < nseq; i++) {
	alnp = arrp(Align, i, ALN);
	if (alnp->markup) continue;

	fprintf(pipe, "  Name: %-*s  Len:  %5d  Check:  %5d  Weight: %.4f\n",
		maxfullnamelen,
		(saveCoordsOn ? 
		 messprintf("%s%c%d-%d", alnp->name, saveSeparator, alnp->start, alnp->end) :
		 messprintf("%s", alnp->name)),
		maxLen,
		GCGchecksum(alnp->seq),
		1.0);
    }
    fprintf(pipe, "\n//\n\n");


    /* Alignment */
    while (paragraph*linelen < maxLen)
    {
	for (i = 0; i < nseq; i++)
	{
	    alnstart = paragraph*linelen; 
	    alnlen = ( (paragraph+1)*linelen < maxLen ? linelen : maxLen-alnstart );
	    alnend = alnstart + alnlen;

	    alnp = arrp(Align, i, ALN);
	    if (alnp->markup) continue;

	    fprintf(pipe, "%-*s  ", maxfullnamelen, 
		    (saveCoordsOn ? 
		     messprintf("%s%c%d-%d", alnp->name, saveSeparator, alnp->start, alnp->end) :
		     messprintf("%s", alnp->name)));

	    for (j = alnstart; j < alnend; ) {
		fprintf(pipe, "%c", alnp->seq[j]);
		j++;
		if ( !((j-alnstart)%blocklen) ) fprintf(pipe, " ");
	    }
	    fprintf(pipe, "\n");
	}
	fprintf(pipe, "\n");
	paragraph++;
    }

    fclose(pipe);
    fflush(pipe);
}


static void saveMSF(void)
{
    FILE *fil;

    if (!(fil = filqueryopen(dirName, fileName, "","w", "Save as MSF (/) file:"))) return;

    writeMSF(fil);

    saved = 1;
}

/*
* Expect format:
*
* name1 name2 name3
* 1-1   1-2   1-3
* 2-1   2-2   2-3
* 3-1   3-2   3-3
*/
static void treeReadDistances(float **pairmtx)
{
  char   *p;
  int    i = 0, j = 0 ;
  float  d;

  while (!feof (treeReadDistancesPipe))
    { 
      if (!fgets (line, MAXLENGTH, treeReadDistancesPipe))
	break;

      j = 0;

      while ( (p = strtok(j ? 0 : line, " \t")) && !isspace(*p))
	{
	  d = atof(p);
	  if (debug) printf("%d  %d  %f\n", i, j, d);
	  pairmtx[i][j] = d;
	  j++;
	}

      if (j != nseq)
	fatal ("nseq = %d, but read %d distances in row %d", nseq, j, i);

      i++;
    }

  if (j != nseq)
    fatal ("nseq = %d, but read %d distance rows", nseq, i);

  return ;
}


/* This is to read in sequence names and count sequences */
static void treeReadDistancesNames()
{
    char   *cp;
    ALN     aln;

    if (!fgets (line, MAXLENGTH, treeReadDistancesPipe))
      fatal("Error reading distance matrix");

    if ((cp = strchr(line, '\n')))
      *cp = 0;

    while ((cp = strtok(nseq ? 0 : line, " \t")))
      {
	resetALN(&aln);

	strncpy(aln.name, cp, MAXNAMESIZE);
	aln.name[MAXNAMESIZE] = 0;
	aln.nr = nseq;
	arrayInsert(Align, &aln, (void*)nrorder);

	nseq++;

	/* printf("%d  %s\n", aln.nr, aln.name); */
    }
}



static void readMSF(FILE *pipe)
{
    char seq[1001], *cp, *cq;
    int i;

    fprintf(stderr, "\nDetected MSF format\n");

    /* Read sequence names */
    while (!feof (pipe))
    { 
	if (!fgets (line, MAXLENGTH, pipe)) break;

	if (!strncmp(line, "//", 2)) 
	    break;
	else if (strstr(line, "Name:") && (cp = strstr(line, "Len:")) && strstr(line, "Check:")) 
	{
	    int len;

	    sscanf(cp+4, "%d", &len);

	    if (maxLen < len) maxLen = len;

	    cp = strstr(line, "Name:")+6;
	    while (*cp && *cp == ' ') cp++;

	    parseMulLine(cp, &aln);

	    aln.len = len;
	    aln.seq = messalloc(aln.len+1);
	    aln.nr = ++nseq;
	    if (!arrayInsert(Align, &aln, (void*)alphaorder))
		messout("Failed to arrayInsert %s", aln.name);
	}
    }

    /* Read sequence alignment */
    while (!feof (pipe))
    { 
	if (!fgets (line, MAXLENGTH, pipe)) break;

	cp = line;
	cq = seq;
	while (*cp && *cp == ' ') cp++;
	while (*cp && *cp != ' ') cp++; /* Spin to sequence */
	while (*cp && cq-seq < 1000) {
	    if (isAlign(*cp)) *cq++ = *cp;
	    cp++;
	}
	*cq = 0;

	if (*seq) {
	    cp = line;
	    while (*cp && *cp == ' ') cp++;

	    parseMulLine(cp, &aln);

	    if (arrayFind(Align, &aln, &i, (void*)alphaorder)) {
		alnp = arrp(Align, i, ALN);
		strcat(alnp->seq, seq);
	    }
	    else {
		messout("Cannot find back %s %d %d seq=%s.\n", 
			aln.name, aln.start, aln.end, seq);
	    }
	}
    }
    strcpy(saveFormat, MSFStr);
}


static void readFastaAlnFinalise(char *seq)
{	    
    aln.len = strlen(seq);
    aln.seq = messalloc(aln.len+1);
    strcpy(aln.seq, seq);
    
    if (maxLen) {
	if (aln.len != maxLen) 
	    fatal("Differing sequence lengths: %s %s", maxLen, aln.len);
    }
    else
	maxLen = aln.len;
    
    if (arrayFind(Align, &aln, &ip, (void*)alphaorder))
	fatal ("Sequence name occurs more than once: %s%c%d-%d", 
	       aln.name, saveSeparator, aln.start, aln.end);
    else {
	aln.nr = ++nseq;
	arrayInsert(Align, &aln, (void*)alphaorder);
    }
}


/* readFastaAln()
 *
 * Read aligned fasta format, assumed to be:
 *
 * >5H1A_HUMAN/53-400 more words 
 * GNACVVAAIAL...........ERSLQ.....NVANYLIG..S.LAVTDL
 * MVSVLV..LPMAAL.........YQVL..NKWTL......GQVT.CDL..
 * >5H1A_RAT more words
 * GNACVVAAIAL...........ERSLQ.....NVANYLIG..S.LAVTDL
 * MVSVLV..LPMAAL.........YQVL..NKWTL......GQVT.CDL..
 * ...
 */
static void readFastaAln(FILE *pipe)
{
  char   *cp, *seq, *seqp;
  int    filesize=0, started=0;

  seq = NULL ;
  stack = stackReCreate (stack, 100);

  while (!feof (pipe))
    { 
      if (!fgets (line, MAXLENGTH, pipe))
	break;
      if ((cp = strchr(line, '\n')))
	*cp = 0;
      pushText(stack, line);
      filesize += strlen(line);
    }

  seq = seqp = messalloc(filesize);

  while ((cp = stackNextText(stack)))
    {
      if (*cp == '>')
	{
	  if (started)
	    readFastaAlnFinalise(seq);

	  parseMulLine(cp+1, &aln);
	    
	  seqp = seq;
	  started = 1;
	}
      else
	{
	  strcpy(seqp, cp);
	  seqp += strlen(seqp);
	}
    }

  if (started)
    readFastaAlnFinalise(seq);

  messfree(seq);

  strcpy(saveFormat, FastaAlnStr);

  return ;
}


/* Convenience routine for converting "name/start-end" to "name start end".
   Used by parsing routines.

   Return 1 if both tokens were found, otherwise 0.
*/
static int stripCoordTokens(char *cp)
{
    char *end = cp;

    while (*end && !isspace(*end)) end++;

    if ((cp = strchr(cp, saveSeparator)) && cp < end) {
	*cp = ' ';
	if ((cp = strchr(cp, '-')) && cp < end) {
	    *cp = ' ';
	    IN_FORMAT = MUL;
	    return 1;
	}
    }
    IN_FORMAT = RAW;
    return 0;
}


/*
   Parse name, start and end of a Mul format line

   Convenience routine, part of readMul and other parsers
*/
static void parseMulLine(char *line, ALN *aln) {

    char line2[MAXLENGTH+1], *cp=line2, *cq, GRfeat[MAXNAMESIZE+1];
	
    strncpy(cp, line, MAXLENGTH);

    resetALN(aln);

    if (!strncmp(cp, "#=GC", 4)) {
	aln->markup = GC;
	cp += 5;
    }
    if (!strncmp(cp, "#=RF", 4)) {
	aln->markup = GC;
    } 
    if (!strncmp(cp, "#=GR", 4)) {
	aln->markup = GR;
	cp += 5;
    }

    if (stripCoordTokensOn) stripCoordTokens(cp);

    /* Name */
    strncpy(aln->name, cp, MAXNAMESIZE);
    aln->name[MAXNAMESIZE] = 0;
    if ((cq = strchr(aln->name, ' '))) *cq = 0;
    
    /* Add Start and End coords */
    if (stripCoordTokensOn && (IN_FORMAT != RAW) )
	sscanf(strchr(cp, ' ') + 1, "%d%d", &aln->start, &aln->end);

    if (aln->markup == GR) {
	/* Add GR markup names to name 
	   
	   #=GR O83071 192 246 SA 999887756453524252..55152525....36463774777.....948472782969685958
	   ->name = O83071_SA
	*/
	if (IN_FORMAT == MUL) {
	    sscanf(cp + strlen(aln->name), 
		   "%d%d%s", &aln->start, &aln->end, GRfeat);
	}
	else {
	    sscanf(cp + strlen(aln->name), 
		   "%s", GRfeat);
	}

	if (strlen(aln->name)+strlen(GRfeat)+2 > MAXNAMESIZE)
	    messout("Too long name or/and feature name");
	strcat(aln->name, " ");
	strncat(aln->name, GRfeat, MAXNAMESIZE-strlen(aln->name));

	/* printf("%s, %d chars\n", aln->name, strlen(aln->name)); fflush(stdout); */
    }
}


/* ReadMul 
 * parses an alignment file in mul or selex format and puts it in the Align array
 *
 * Assumes header contains ' ' or '#'
 *
 * Alignment can have one of the following formats:
 *  CSW_DROME  VTHIKIQNNGDFFDLYGGEKFATLP
 *  CSW_DROME  51   75   VTHIKIQNNGDFFDLYGGEKFATLP
 *  CSW_DROME  51   75   VTHIKIQNNGDFFDLYGGEKFATLP P29349
 *  KFES_MOUSE/458-539    .........WYHGAIPW.....AEVAELLT........HTGDFLVRESQG
 *
 */
static void readMul(FILE *pipe)
{
    char   *cp, *cq, ch = '\0';
    int    len, i, alnstart;

    stack = stackReCreate (stack, 100);
    AnnStack = stackReCreate (AnnStack, 100);

    /* Parse header to check for MSF or Fasta format */
    while (!feof (pipe))
    { 
	if (!isspace(ch = fgetc(pipe))) {
	    if (ch == '>') {
		ungetc(ch, pipe);
		readFastaAln(pipe);
		return;
	    }
	    else
		break;
	}
	else
	    if (ch != '\n')
		if (!fgets (line, MAXLENGTH, pipe)) break;

	if (strstr(line, "MSF:") && strstr(line, "Type:") && strstr(line, "Check:")) {
	    readMSF(pipe);
	    return;
	}
	
    }

    if (!feof(pipe))
      ungetc(ch, pipe);

    /* Read raw alignment into stack
     *******************************/

    alnstart = MAXLENGTH;
    while (!feof (pipe))
    { 
	if (!fgets (line, MAXLENGTH, pipe) || (unsigned char)*line == (unsigned char)EOF ) break;
				/* EOF checking to make acedb calling work */

	if (!strncmp(line, "PileUp", 6)) {
	    readMSF(pipe);
	    return;
	}
	    
	if ((cp = strchr(line, '\n'))) *cp = 0;
	len = strlen (line);

	/* Sequences */
	if (len && *line != '#' && strcmp(line, "//"))
	{
	    if (!(cq = strchr(line, ' '))) fatal("No spacer between name and sequence in %s", line);

	    /* Find leftmost start of alignment */
	    for (i = 0; line[i] && line[i] != ' '; i++);
	    for (; line[i] && !isAlign(line[i]); i++);
	    if (i < alnstart) alnstart = i;
	    
	    /* Remove optional accession number at end of alignment */
	    /* FOR PRODOM STYLE ALIGNMENTS W/ ACCESSION TO THE RIGHT - MAYBE MAKE OPTIONAL
	       This way it's incompatible with alignments with ' ' gapcharacters */
	    for (; line[i] && isAlign(line[i]); i++);
	    line[i] = 0;
	    
	    pushText(stack, line);
	}
	/* Markup line */
	else if (!strncmp(line, "#=GF ", 5) || 
		 !strncmp(line, "#=GS ", 5)) {
	    pushText(AnnStack, line);
	}
	else if (!strncmp(line, "#=GC ", 5) || 
		 !strncmp(line, "#=GR ", 5) || 
		 !strncmp(line, "#=RF ", 5)) {
	    pushText(stack, line);
	}
	/* Match Footer  */
	else if (!strncmp(line, "# matchFooter", 13)) {
	    matchFooter = 1;
	    break;
	}
    }

    /* Read alignment from Stack into Align array
     ***************************************/

    /* 
     * First pass - Find number of unique sequence names and the total sequence lengths 
     */
    stackCursor(stack, 0);
    while ((cp = stackNextText(stack)))
      {

	parseMulLine(cp, &aln);

	/* Sequence length */
	len = strlen(cp+alnstart);

	if (arrayFind(Align, &aln, &ip, (void*)alphaorder))
	    arrp(Align, ip, ALN)->len += len;
	else
	  {
	    aln.len = len;
	    aln.nr = ++nseq;
	    arrayInsert(Align, &aln, (void*)alphaorder);
	    
	    /*printf("\nInserted %s %6d %6d %s %d\n", aln.name, aln.start, aln.end, cp+alnstart, len);
	      fflush(stdout);*/
	}
      }

    /* Find maximum length of alignment; allocate it */
    for (i = 0; i < nseq; i++)
      {
	alnp = arrp(Align, i, ALN);
	if (alnp->len > maxLen) maxLen = alnp->len;
      }

    for (i = 0; i < nseq; i++)
      {
	alnp = arrp(Align, i, ALN);
	alnp->seq = messalloc(maxLen+1);
      }

    /* 
     * Second pass - allocate and read in sequence data 
     */
    stackCursor(stack, 0);
    while ((cp = stackNextText(stack)))
      {
	parseMulLine(cp, &aln);

	if (arrayFind(Align, &aln, &i, (void*)alphaorder))
	  {
	    alnp = arrp(Align, i, ALN);
	    strcat(alnp->seq, cp+alnstart);
	  }
	else
	  {
	    messout("Cannot find back %s %d %d seq=%s.\n", 
		    aln.name, aln.start, aln.end, aln.seq);
	  }
    }
    
    /* Parse sequence attributes from #=GS lines */
    stackCursor(AnnStack, 0);
    while ((cp = stackNextText(AnnStack)))
      {
	char
	  *namep,		/* Seqname */
	  name[MAXNAMESIZE+1],/* Seqname */
	  *labelp,		/* Label (OS) */
	  *valuep;		/* Organism (c. elegans) */
	
	if (strncmp(cp, "#=GS", 4))
	  continue;
	
	strcpy(name, cp+4);
	namep = name;
	while (*namep == ' ') namep++;

	labelp = namep;
	while (*labelp != ' ') labelp++;
	*labelp = 0;		/* Terminate namep */
        labelp++;		/* Walk across terminator */
	while (*labelp == ' ') labelp++;

	valuep = labelp;
	while (*valuep != ' ') valuep++;
	while (*valuep == ' ') valuep++;
	
	if (!strncasecmp(labelp, "LO", 2))
	  {
	    int i, colornr;
	    
	    for (colornr = -1, i = 0; i < NUM_TRUECOLORS; i++)
	      if (!strcasecmp(colorNames[i], valuep)) colornr = i;
	    
	    if (colornr == -1)
	      {
		printf("Unrecognized color: %s\n", valuep);
		colornr = 0;
	      }
	    
	    str2aln(namep, &aln);
	    /* Find the corresponding sequence */
	    if (!arrayFind(Align, &aln, &i, (void*)alphaorder))
	      {
		messout("Cannot find %s %d %d in alignment", aln.name, aln.start, aln.end);
		continue;
	      }
	    alnp = arrp(Align, i, ALN);
	    alnp->color = colornr;
	  }
	
	else if (!strncasecmp(labelp, OrganismLabel, 2))
	  {

	    /* Add organism to table of organisms.  This is necessary to make all 
	       sequences of the same organism point to the same place and to make a 
	       non-redundant list of present organisms */

	    /* Find string in permanent stack */
	    if (!(valuep = strstr(cp, valuep)))
	      {
		messout("Failed to parse organism properly");
		continue;
	      }
	    aln.organism = valuep; /* Find string in permanent stack */
	    arrayInsert(organismArr, &aln, (void*)organism_order);
	    
	    if (strchr(cp, '/') && strchr(cp, '-'))
	      {
		
		str2aln(namep, &aln);
		
		/* Find the corresponding sequence */
		if (!arrayFind(Align, &aln, &i, (void*)alphaorder)) {
		    messout("Cannot find %s %d %d in alignment", aln.name, aln.start, aln.end);
		    continue;
		}
		alnp = arrp(Align, i, ALN);

		/* Store pointer to unique organism in ALN struct */
		aln.organism = valuep;
		arrayFind(organismArr, &aln, &ip, (void*)organism_order);
		alnp->organism = arrp(organismArr, ip, ALN)->organism;
	      }
	    else {
		/* Organism specified for entire sequences.
		   Find all ALN instances of this sequences.
		*/
		for (i = 0; i < nseq; i++) {
		    alnp = arrp(Align, i, ALN);
		    if (!strcmp(alnp->name, namep)) {

			/* Store pointer to unique organism in ALN struct */
			aln.organism = valuep;
			arrayFind(organismArr, &aln, &ip, (void*)organism_order);
			alnp->organism = arrp(organismArr, ip, ALN)->organism;
		    }
		}
	    }
	}
    }


    /* For debugging * /
    for (i = 0; i < nseq; i++) {
	alnp = arrp(Align, i, ALN);
	printf("\n%-10s %4d %4d %s %d %s\n", 
	alnp->name, alnp->start, alnp->end, alnp->seq, 
	       alnp->len, alnp->organism);
    }
    for (i=0; i < arrayMax(organismArr); i++) 
	printf("%s\n", arrp(organismArr, i, ALN)->organism);
    */


    if (!nseq || !maxLen) fatal("Unable to read sequence data");

    strcpy(saveFormat, MulStr);
}


static void readScores(char *filename)
{
    char  
	line[MAXLENGTH+1], linecp[MAXLENGTH+1], *cp;
    FILE 
	*file;
    ALN
	aln;
    int
	scoreLen;

    if (!(file = fopen(filename, "r")))
	fatal("Cannot open file %s", filename);

    while (!feof (file))
    { 
	if (!fgets (line, MAXLENGTH, file)) break;
	strcpy(linecp, line);

	resetALN(&aln);

	if (!(cp = strtok(line, " "))) fatal("Error parsing score file %s.\nLine: %s", filename, linecp);
	if (!(sscanf(cp, "%f", &aln.score)))
	    fatal("Error parsing score file %s - bad score.\nLine: %s", filename, linecp);

	if (!(cp = strtok(0, "/"))) fatal("Error parsing score file %s.\nLine: %s", filename, linecp);
	strncpy(aln.name, cp, MAXNAMESIZE);
	aln.name[MAXNAMESIZE] = 0;

	if (!(cp = strtok(0, "-"))) fatal("Error parsing score file %s.\nLine: %s", filename, linecp);
	if (!(aln.start = atoi(cp)))
	    fatal("Error parsing score file %s - no start coordinate.\nLine: %s", filename, linecp);

	if (!(cp = strtok(0, "\n"))) fatal("Error parsing score file %s.\nLine: %s", filename, linecp);
	if (!(aln.end = atoi(cp)))
	    fatal("Error parsing score file %s - no end coordinate.\nLine: %s", filename, linecp);

	if (!alignFind(Align, &aln, &ip)) {
	    /* printf("Warning: %s/%d-%d (score %.1f) not found in alignment\n", 
		   aln.name, aln.start, aln.end, aln.score);*/
	}
	else {
	    arrp(Align, ip, ALN)->score = aln.score;
	    scoreLen = strlen(messprintf("%.1f", aln.score));
	    if (scoreLen > maxScoreLen) maxScoreLen = scoreLen;
	}
    }
    fclose(file);

    displayScores = 1;
}

static void checkAlignment()
{
    int i, g, cres, nres, tmp;

    maxNameLen = maxStartLen = maxEndLen = 0;

    for (i = 0; i < nseq; i++) {

	alnp = arrp(Align, i, ALN);

	if (!alnp->markup) {

	    /* Count residues */
	    for (cres = g = 0; g < maxLen; g++) {
		if (isAlign(alnp->seq[g]) && !isGap(alnp->seq[g])) cres++;
	    }
	    
	    if (!alnp->start) {
		/* No coords provided - reconstruct them */
		alnp->start = 1;
		alnp->end = cres;
	    }
	    else {
		/* Check if provided coordinates are correct */
		nres = abs(alnp->end - alnp->start) + 1;
		
		if ( nres != cres)
		    fprintf(stderr, "Found wrong number of residues in %s/%d-%d: %d instead of %d\n",
			    alnp->name, alnp->start, alnp->end,
			    cres,
			    nres);
	    }
	}

	/* Find max string length of name, start, and end */
	if (maxNameLen  < strlen(alnp->name)) maxNameLen = strlen(alnp->name);
	if (maxStartLen < (tmp = strlen(messprintf("%d", alnp->start)))) maxStartLen = tmp;
	if (maxEndLen   < (tmp = strlen(messprintf("%d", alnp->end)))) maxEndLen = tmp;

    }
}    


void argvAdd(int *argc, char ***argv, char *s)
{
    char **v;
    int i;

    v = (char **)malloc(sizeof(char *)*(*argc+2));

    /* Copy argv */
    for (i = 0; i < (*argc); i++)
	v[i] = (*argv)[i];

    /* Add s */
    v[*argc] = (char *)malloc(strlen(s)+1);
    strcpy(v[*argc], s);

    (*argc)++;

    /* Terminator - ANSI C standard */
    v[*argc] = 0;

    /* free(*argv); */   /* Too risky */
    
    *argv = v;
}


int main(int argc, char **argv)
{
    FILE    
	*file, *pipe;
    char    
	*scoreFile = 0,
	*readMatchFile = 0,
	*colorCodesFile = 0,
	*markupColorCodesFile = 0,
	*output_format = 0,
	*optargc;
    int     
	i,
	pw, ph,	 /* pixel width and height */
	output_probs = 0,
	init_tree = 0,
	only_tree = 0,
        show_ann = 0;
    float   
        makeNRinit = 0.0,
        init_rmEmptyColumns = 0.0,
        init_rmGappySeqs = 0.0,
	cw;      /* character width of initial alignment */

/*    extern int printOnePage;*/

    int          optc;
    extern int   optind;
    extern char *optarg;
    char        *optstring="aBb:CcGgil:L:m:n:O:o:PpQ:q:RrS:s:T:t:uX:z:";

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
 Belvu - View multiple alignments in good-looking colours.\n\
\n\
 Usage: belvu [options] <multiple_alignment>|- [X options]\n\
\n\
 <multiple_alignment>|- = alignment file or pipe.\n\
\n\
 Options:\n\
 -c          Print Conservation table.\n\
 -l <file>   Load residue color code file.\n\
 -L <file>   Load markup and organism color code file.\n\
 -m <file>   Read file with matching sequence segments.\n\
 -r          Read alignment in 'raw' format (Name sequence).\n\
 -R          Do not parse coordinates when reading alignment.\n\
 -o <format> Write alignment or tree to stdout in this format and exit.\n\
                Valid formats: MSF, Mul(Stockholm), Selex, \n\
                               FastaAlign, Fasta, tree.\n\
 -X <#>      Print UPGMA-based subfamilies at cutoff #.\n\
 -n <cutoff> Make non-redundant to <cutoff> %identity at startup.\n\
 -Q <cutoff> Remove columns more gappy than <cutoff>.\n\
 -q <cutoff> Remove sequences more gappy than <cutoff>.\n\
 -G          Penalize gaps in pairwise comparisons.\n\
 -i          Ignore gaps in conservation calculation.\n\
 -P          Remove partial sequences at startup.\n\
 -C          Don't write coordinates to saved file.\n\
 -z <char>   Separator char between name and coordinates in saved file.\n\
 -a          Show alignment annotations on screen (Stockholm format only).\n\
 -p          Output random model probabilites for HMMER.\n\
                (Based on all residues.)\n\
 -S <order>  Sort sequences in this order.\n\
                a -> alphabetically\n\
                o -> by Swissprot organism, alphabetically\n\
                s -> by score\n\
                n -> by Neighbor-joining tree\n\
                u -> by UPGMA tree\n\
                S -> by similarity to first sequence\n\
                i -> by identity to first sequence\n\
 -s <file>   Read in file of scores.\n\
 -T <method> Tree options:\n\
                i -> Start up showing tree\n\
                I -> Start up showing only tree\n\
                d -> Show distances in tree\n\
                n -> Neighbor-joining\n\
                u -> UPGMA\n\
                c -> Don't color tree by organism\n\
                o -> Don't display sequence coordinates in tree\n\
                b -> Use Scoredist distance correction (default)\n\
                j -> Use Jukes-Cantor distance correction\n\
                k -> Use Kimura distance correction\n\
                s -> Use Storm & Sonnhammer distance correction\n\
                r -> Use uncorrected distances\n\
                p -> Print distance matrix and exit\n\
                R -> Read distance matrix instead of alignment\n\
                     (only in combination with Tree routines)\n\
 -b <#>      Apply boostrap analysis with # bootstrap samples\n\
 -B          Print out bootstrap trees and exit\n\
               (Negative value -> display bootstrap trees on screen)\n\
 -O <label>  Read organism info after this label (default OS)\n\
 -t <title>  Set window title.\n\
 -g          Draw grid line (for debugging).\n\
 -u          Start up with uncoloured alignment (faster).\n\
\n\
 Some X options:\n\
 -acefont <font>   Main font.\n\
 -font    <font>   Menu font.\n\
\n\
 Note: X options only work after \"setenv POSIXLY_CORRECT\"\n\
\n\
 setenv BELVU_FETCH to desired sequence fetching program.\n\
\n\
 For documentation, see:\n\
 http://sonnhammer.sbc.su.se/Belvu.html\n\
\n\
 by Erik.Sonnhammer@sbc.su.se\n\
 Version ";


    usage = messalloc(strlen(usageText) + strlen(belvuVersion) + strlen(cc_date) + 20);
    sprintf(usage, "%s%s, compiled %s\n", usageText, belvuVersion, cc_date);

    /* Set up tree defaults */
    strcpy(treeMethodString, NJstr);
    treeMethod = NJ;
    strcpy(treeDistString, Scorediststr);
    treeDistCorr = Scoredist;
    strcpy(treePickString, SWAPstr);
    treePickMode = NODESWAP;
    setTreeScaleCorr();

    while ((optc = getopt(argc, argv, optstring)) != -1)
	switch (optc) 
	{
	case 'a': show_ann = 1;                  break;
	case 'B':  outputBootstrapTrees = 1; break;
	case 'b': treebootstraps = atoi(optarg); break;
	case 'C': saveCoordsOn = 0;              break;
	case 'c': verbose = 1;                   break;
	case 'G': penalize_gaps = 1;             break;
	case 'g': gridOn = 1;                    break;
	case 'l': 
	    colorCodesFile = messalloc(strlen(optarg)+1);
	    strcpy(colorCodesFile, optarg);      break;
	case 'i': 
	    ignoreGapsOn = 1;                           break;
	case 'L': 
	    markupColorCodesFile = messalloc(strlen(optarg)+1);
	    strcpy(markupColorCodesFile, optarg);break;
	case 'm': 
	    readMatchFile = messalloc(strlen(optarg)+1);
	    strcpy(readMatchFile, optarg); 
	                                         break;
	case 'n':
	    makeNRinit = atof(optarg);           break;
	case 'O': 
	    strncpy(OrganismLabel, optarg, 2);    break;
	case 'o': 
	    output_format = messalloc(strlen(optarg)+1);
	    strcpy(output_format, optarg);       break;
	case 'P': init_rmPartial = 1;            break;
	case 'Q': init_rmEmptyColumns = atof(optarg); break;
	case 'q': init_rmGappySeqs = atof(optarg); break;
	case 'p': output_probs = 1;              break;
	case 'R': stripCoordTokensOn = saveCoordsOn = 0; break;
	case 'r': IN_FORMAT = RAW;               break;
	case 'S': 
	    switch (*optarg)
	    {
	    case 'a': init_sort = SORT_ALPHA;    break;
	    case 'o': init_sort = SORT_ORGANISM; break;
	    case 's': init_sort = SORT_SCORE;    break;
	    case 'S': init_sort = SORT_SIM;      break;
	    case 'i': init_sort = SORT_ID;       break;
	    case 'n': 
		treeMethod = NJ;
		init_sort = SORT_TREE;           break;
	    case 'u': 
		treeMethod = UPGMA; 
		init_sort = SORT_TREE;           break;
	    default : fatal("Illegal sorting order: %s", optarg);
	    }                                    break;
	case 's': 
	    scoreFile = messalloc(strlen(optarg)+1);
	    strcpy(scoreFile, optarg);           break;
	case 'T': 
	    for (optargc = optarg; *optargc; optargc++) {
		switch (*optargc)
		{
		case 'n': 
		    strcpy(treeMethodString, NJstr);
		    treeMethod = NJ;          break;
		case 'u': 
		    strcpy(treeMethodString, UPGMAstr);
		    treeMethod = UPGMA;       break;
		case 'c':
		    treeColorsOn = 0;         break;
		case 'd':
		    treeShowBranchlen = 1;    break;
		case 'I':
		    only_tree=1;
		case 'i':
		    init_tree = 1;            break;
		case 'j':
		    strcpy(treeDistString, JUKESCANTORstr);
		    treeDistCorr = JUKESCANTOR;
		    setTreeScaleCorr();         break;
		case 'k':
		    strcpy(treeDistString, KIMURAstr);
		    treeDistCorr = KIMURA;
		    setTreeScaleCorr();         break;
		case 'o':
		    treeCoordsOn = 0;         break;
		case 'p':
		    treePrintDistances = 1;  
		    init_tree = 1;            break;
		case 'R':
		    treeReadDistancesON = 1;  break;
		case 's':
		    strcpy(treeDistString, STORMSONNstr);
		    treeDistCorr = STORMSONN;
		    setTreeScaleCorr();         break;
		case 'b':
		    strcpy(treeDistString, Scorediststr);
		    treeDistCorr = Scoredist;
		    setTreeScaleCorr();         break;
		case 'r':
		    strcpy(treeDistString, UNCORRstr);
		    treeDistCorr = UNCORR;
		    treeScale = 1.0;          break;
		default : fatal("Illegal sorting order: %s", optarg);
		}
	    }                                 break;
	case 't': 
	    strncpy(Title, optarg, 255);         break;
	case 'u': colorRectangles = 0;           break;
	case 'X': mksubfamilies_cutoff = atof(optarg);  break;
	case 'z': saveSeparator = *optarg;       break;
	default : fatal("Illegal option");
	}

    if (argc-optind < 1) {
	fprintf(stderr, "%s\n", usage); 
	exit(1);
    }

    if (!strcmp(argv[optind], "-")) {
	pipe = stdin;
	if (!*Title) strcpy(Title, "stdin");
    }
    else {
	if (!(pipe = fopen(argv[optind], "r")))
	    fatal("Cannot open file %s", argv[optind]);
	if (!*Title) strncpy(Title, argv[optind], 255);
	strncpy(fileName, argv[optind], FIL_BUFFER_SIZE);
    }

/*    printOnePage = TRUE;*/

 #if defined(LINUX) 
    Align = arrayCreate(100000, ALN);
    organismArr = arrayCreate(10000, ALN);
    /* Note: this excessive initialization size is a temporary fix to stop
    Belvu from segfaulting under linux.  The segfaulting happens during 
    arrayInsert expansions but only under linux. As I can't find any bugs
    in Belvu it seems the array package may be bugged under linux. For
    now, avoiding arrayInsert expansions by initializing it to over twice the largest
    family in Pfam 040401 buys me some time to find the problem. */    
#else
    Align = arrayCreate(100, ALN);
    organismArr = arrayCreate(100, ALN);
#endif

    if (treeReadDistancesON) 
    {
        /* Should this really be either or?  Problem: cannot read organism info when reading tree */
	treeReadDistancesPipe = pipe;
	treeReadDistancesNames();
	
	init_tree = 1;
	only_tree = 1;
	treeCoordsOn = 0;
    }
    else
        readMul(pipe);

    if (!arrayMax(organismArr))
      suffix2organism();

    setOrganismColors();

    if (scoreFile) readScores(scoreFile);
    
    init_sort_do();

    if (!matchFooter && readMatchFile) {
	if (!(file = fopen(readMatchFile, "r"))) 
	    fatal("Cannot open file %s", readMatchFile);
	readMatch(file);
	fclose(file);
    }
    else if (matchFooter) 
    {	 
        readMatch(pipe);
	fclose(pipe);
    }

    if (!treeReadDistancesON) {
	checkAlignment();
	setConservColors();
    }

    if (verbose)
      {
	/* Print conservation statistics */
	int i, j, max, consensus = 0 ;
	float totcons = 0.0;

	printf("\nColumn Consensus        Identity       Conservation\n");
	printf  ("------ ---------  -------------------  ------------\n");

	for (i = 0; i < maxLen; i++)
	  {
	    max = 0;

	    for (j = 1; j < 21; j++)
	      {
		if (conservCount[j][i] > max)
		  {
		    max = conservCount[j][i];
		    consensus = j;
		  }
	      }

	    printf("%4d       %c      %4d/%-4d = %5.1f %%  %4.1f\n", 
		   i+1, b2a[consensus], max, nseq, (float)max/nseq*100, conservation[i]);
	    totcons += conservation[i];
	  }

	printf ("\nAverage conservation = %.1f\n", totcons/(maxLen*1.0));

	exit(0);
      }

    initResidueColors();
    if (colorCodesFile) {
	if (!(file = fopen(colorCodesFile, "r"))) 
	    fatal("Cannot open file %s", colorCodesFile);
	readColorCodes(file, color);
 	colorScheme = COLORBYRESIDUE;
	color_by_conserv = colorByResIdOn = 0;
   }
    initMarkupColors();
    if (markupColorCodesFile) {
	if (!(file = fopen(markupColorCodesFile, "r"))) 
	    fatal("Cannot open file %s", markupColorCodesFile);
	readColorCodes(file, markupColor);
    }
    
    if (makeNRinit)
	mkNonRedundant(makeNRinit);
    
    if (init_rmPartial)
	rmPartial();

    if (init_rmEmptyColumns)
	rmEmptyColumns(init_rmEmptyColumns/100.0);

    if (init_rmGappySeqs) {
	rmGappySeqs(init_rmGappySeqs);
	rmFinaliseGapRemoval();
    }

    if (output_format) {
	if (!strcasecmp(output_format, "Stockholm") ||
	    !strcasecmp(output_format, "Mul") ||
	    !strcasecmp(output_format, "Selex"))
	    writeMul(stdout);
	else if (!strcasecmp(output_format, "MSF"))
	    writeMSF(stdout);
	else if (!strcasecmp(output_format, "FastaAlign")) {
	    strcpy(saveFormat, FastaAlnStr);
	    writeFasta(stdout);
	}
	else if (!strcasecmp(output_format, "Fasta")) {
	    strcpy(saveFormat, FastaStr);
	    writeFasta(stdout);
	}
	else if (!strcasecmp(output_format, "tree")) {
	    treeStruct *treestruct = messalloc(sizeof(treeStruct));
	    separateMarkupLines();
	    treestruct->head = treeMake(1);
	    treePrintNH(treestruct, treestruct->head, stdout);
	    printf(";\n");
	}
	else 
	    fatal("Illegal output format: %s", output_format);
	exit(0);
    }
    
    if (outputBootstrapTrees && treebootstraps > 0) {
	treeBootstrap();
	exit(0);
    } 
   
    if (output_probs) {
	outputProbs(stdout);
	exit(0);
    }

    if (mksubfamilies_cutoff) {
	mksubfamilies(mksubfamilies_cutoff);
	exit(0);
    
    }
    fprintf(stderr, "\n%d sequences, max len = %d\n", nseq, maxLen);

    /* Try to get 8x13 font for menus, if not set on command line */
    for ( i=0; i < argc; i++)
	if (!strcmp(argv[i], "-font")) break;
    if (i == argc) {
	argvAdd(&argc, &argv, "-font");
	argvAdd(&argc, &argv, "8x13");
    }

    graphInit(&argc, argv);
    gexInit(&argc, argv);

    graphScreenSize(&screenWidth, &screenHeight, &fontwidth, &fontheight, &pw, &ph);
    Aspect = (pw/fontwidth)/(ph/fontheight);
    VSCRWID = HSCRWID/Aspect;

    if (show_ann) showAnnotation();

    /* Calculate screen width of alignment */
    cw = 1 + maxNameLen+1;
    if (maxStartLen) cw += maxStartLen+1;
    if (maxEndLen)   cw += maxEndLen+1;
    if (maxScoreLen) cw += maxScoreLen+1;
    cw += maxLen + ceil(VSCRWID) + 2;

     colorMenu = menuInitialise ("color", (MENUSPEC*)colorMENU);
    colorEditingMenu = menuInitialise ("color", (MENUSPEC*)colorEditingMENU);
    sortMenu = menuInitialise ("sort", (MENUSPEC*)sortMENU);
    editMenu = menuInitialise ("edit", (MENUSPEC*)editMENU);
    showColorMenu =  menuInitialise ("", (MENUSPEC*)showColorMENU);
    saveMenu =  menuInitialise ("", (MENUSPEC*)saveMENU);
    belvuMenu = menuInitialise ("belvu", (MENUSPEC*)mainMenu);
    treeGUIMenu = menuInitialise ("", (MENUSPEC*)treeGUIMENU);
    treeDistMenu = menuInitialise ("", (MENUSPEC*)treeDistMENU);
    treePickMenu = menuInitialise ("", (MENUSPEC*)treePickMENU);
    if (!displayScores) {
	menuSetFlags(menuItem(sortMenu, "Sort by score"), MENUFLAG_DISABLED);
	menuSetFlags(menuItem(editMenu, "Remove sequences below given score"), MENUFLAG_DISABLED);
	menuSetFlags(menuItem(belvuMenu, "Print score and coords of line"), MENUFLAG_DISABLED);
	menuSetFlags(menuItem(belvuMenu, "Output score and coords of line"), MENUFLAG_DISABLED);
    }
    menuSetFlags(menuItem(colorMenu, thresholdStr), MENUFLAG_DISABLED);

   if (init_tree)
      {
	treeDisplay();

	if (only_tree)
	  {

	    /*ACEOUT out = aceOutCreateToFile("t", "w", 0);
	    graphGIF(treeGraph, out, 0);*/

	    graphLoop(FALSE);
	    graphFinish();
	    
	    exit(0);
	  }
      }

    if (outputBootstrapTrees && treebootstraps < 0)
      {	/* Display [treebootstraps] bootstrap trees */
	treebootstrapsDisplay = 1;
	
	treebootstraps = -treebootstraps;

	treeBootstrap();

	/* graphLoop(FALSE);
	graphFinish();
	exit(0); */
      }



    /* Create the main belvu graph display of aligned sequences. */

    belvuGraph = graphCreate (TEXT_FIT, messprintf("Belvu:  %s", Title), 0, 0, 
			      cw*screenWidth/fontwidth, 0.44*screenHeight);


#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
    /* None of this is in eriks code...is it needed ?? */

    colorMenu = menuInitialise ("color", (MENUSPEC*)colorMENU);
    colorEditingMenu = menuInitialise ("color", (MENUSPEC*)colorEditingMENU);
    sortMenu = menuInitialise ("sort", (MENUSPEC*)sortMENU);
    editMenu = menuInitialise ("edit", (MENUSPEC*)editMENU);
    showColorMenu =  menuInitialise ("", (MENUSPEC*)showColorMENU);
    saveMenu =  menuInitialise ("", (MENUSPEC*)saveMENU);
    belvuMenu = menuInitialise ("belvu", (MENUSPEC*)mainMenu);
    treeGUIMenu = menuInitialise ("", (MENUSPEC*)treeGUIMENU);
    treeDistMenu = menuInitialise ("", (MENUSPEC*)treeDistMENU);
    if (!displayScores)
      {
	menuSetFlags(menuItem(sortMenu, "Sort by score"), MENUFLAG_DISABLED);
	menuSetFlags(menuItem(editMenu, "Remove sequences below given score"), MENUFLAG_DISABLED);
	menuSetFlags(menuItem(belvuMenu, "Print score and coords of line"), MENUFLAG_DISABLED);
	menuSetFlags(menuItem(belvuMenu, "Output score and coords of line"), MENUFLAG_DISABLED);
      }
    menuSetFlags(menuItem(colorMenu, thresholdStr), MENUFLAG_DISABLED);
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

    
    graphRegister(PICK, boxPick);
    graphRegister(MIDDLE_DOWN, middleDown);
    graphRegister(RESIZE, belvuRedraw);
    graphRegister(KEYBOARD, keyboard);
    graphRegister(DESTROY, belvuDestroy) ;

    if (!colorCodesFile) colorSim() ;

    belvuRedraw() ;

    graphLoop(FALSE) ;

    graphFinish() ;

    return(0) ;
}


static void finishShowColorOK(void)
{
  graphActivate(showColorGraph);
  graphDestroy();
  showColorGraph = GRAPH_NULL ;
  
  belvuRedraw();

  return ;
}

static void finishShowColorCancel(void)
{
    int i;
    
    if (color_by_conserv) {
	maxfgColor = oldmaxfgColor;
	midfgColor = oldmidfgColor;
	lowfgColor = oldlowfgColor;
	maxbgColor = oldmaxbgColor;
	midbgColor = oldmidbgColor;
	lowbgColor = oldlowbgColor;

	if (color_by_similarity) {
	    maxSimCutoff = oldMax;
	    midSimCutoff = oldMid;
	    lowSimCutoff = oldLow;
	}
	else {
	    maxIdCutoff = oldMax;
	    midIdCutoff = oldMid;
	    lowIdCutoff = oldLow;
	}
    }
    else
	for (i=65; i<96; i++) color[i] = oldColor[i];
    
    setConservColors();

    finishShowColorOK();
}

static void setSim(char *cp) {
    maxSimCutoff = atof(maxText);
    midSimCutoff = atof(midText);
    lowSimCutoff = atof(lowText);
    showColorCodesRedraw();
}
static void setId(char *cp) {
    maxIdCutoff = atof(maxText);
    midIdCutoff = atof(midText);
    lowIdCutoff = atof(lowText);
    showColorCodesRedraw();
}

static void settingsPick(int box, double x_unused, double y_unused, int modifier_unused)
{
    if (box == maxBox) graphTextScrollEntry(maxText,0,0,0,0,0);
    else if (box == midBox) graphTextScrollEntry(midText,0,0,0,0,0);
    else if (box == lowBox) graphTextScrollEntry(lowText,0,0,0,0,0);
}

static void belvuConfColourMenu(KEY key, int box)
{
  int *colour;

  if (graphAssFind(assVoid(box+2000), &colour))
    { 
      *colour = key;
      *(colour+32) = key;
      graphBoxDraw(box, BLACK, *colour);
      showColorCodesRedraw();
    } 
  else
    {
      messout("Cannot find color box %d", box);
    }

  return ;
}

static void belvuConfColour(int *colour, int init, float x, float y, int width, char *text)
{ 
    int box;
    if (text)
	graphText(text, x+width+1, y);

    box = graphBoxStart();
    graphRectangle(x, y, x+width, y+1);
    graphBoxEnd();
    graphBoxFreeMenu(box, belvuConfColourMenu, graphColors);
    *colour = init;
    graphBoxDraw(box, BLACK, init);
    graphAssRemove (assVoid(box+2000)) ;
    graphAssociate(assVoid(box+2000), colour);
}

static void showConservCode(char *text, int *fgcolor, int *bgcolor, float y, char *id, int box, void (*fn)(char*))
{
    graphText(text, 1, y); 
    graphTextScrollEntry (id, 10, 5, 6, y, fn);

    belvuConfColour(fgcolor, *fgcolor, 12, y, 1, 0);
    belvuConfColour(bgcolor, *bgcolor, 13, y, 3, messprintf("%s", colorNames[*bgcolor]));
}


static void paintMessage(int *i) 
{
    /* graphText("All residues with a positive", 1, 1.5*i++);
       graphText("BLOSUM62 score to a coloured", 1, 1.5*i++);
       graphText("residue get the same colour.", 1, 1.5*i++);
       */
    
    graphText("Similar residues according", 1, (*i) * 1.5);
    graphText("to BLOSUM62 are coloured", 1, (*i) * 1.5 + 1);
    graphText("as the most conserved one.", 1, (*i) * 1.5 +2);
    (*i) += 3;
}

	
static void showColorCodesRedraw(void)
{
    int i, j, p;
    char *res = messalloc(2);
    int box;

    *res = ' ' ;

    if (!graphActivate(showColorGraph)) {
	showColorGraph = graphCreate (TEXT_FIT, "Colour Codes", 0, 0, 0.35, 0.5);

	if (color_by_conserv) {
	    oldmaxfgColor = maxfgColor;
	    oldmidfgColor = midfgColor;
	    oldlowfgColor = lowfgColor;
	    oldmaxbgColor = maxbgColor;
	    oldmidbgColor = midbgColor;
	    oldlowbgColor = lowbgColor;
	    
	    if (color_by_similarity) {
		oldMax = maxSimCutoff;
		oldMid = midSimCutoff;
		oldLow = lowSimCutoff;
	    }
	    else {
		oldMax = maxIdCutoff;
		oldMid = midIdCutoff;
		oldLow = lowIdCutoff;
	    }
	}
	else
	    for (i=65; i<96; i++) oldColor[i] = color[i];
    
	graphTextFormat(FIXED_WIDTH);
	graphNewMenu(showColorMenu);
	graphRegister(PICK, settingsPick);
    }
    graphPop();
    graphClear();

    if (color_by_conserv) {
	i = 1; 

	if (color_by_similarity) {

	    graphText("Colouring by average", 1, 1.5*i);
	    graphText("BLOSUM62 score.", 1, 1.5*i+1);
	    i += 2;
	    
	    paintMessage(&i);
	    
	    sprintf(maxText, "%.2f", maxSimCutoff);
	    showConservCode("Max:", &maxfgColor, &maxbgColor, 1.5*i++, maxText, maxBox, setSim);
	    sprintf(midText, "%.2f", midSimCutoff);
	    showConservCode("Mid:", &midfgColor, &midbgColor, 1.5*i++, midText, midBox, setSim);
	    sprintf(lowText, "%.2f", lowSimCutoff);
	    showConservCode("Low:", &lowfgColor, &lowbgColor, 1.5*i++, lowText, lowBox, setSim);
	}
	else {
	    oldMax = maxIdCutoff;
	    oldMid = midIdCutoff;
	    oldLow = lowIdCutoff;

	    graphText("Colouring by %identity.", 1, 1.5*i++);
	    i++;

	    if (id_blosum) paintMessage(&i);

	    sprintf(maxText, "%.1f", maxIdCutoff);
	    showConservCode("Max:", &maxfgColor, &maxbgColor, 1.5*i++, maxText, maxBox, setId);
	    sprintf(midText, "%.1f", midIdCutoff);
	    showConservCode("Mid:", &midfgColor, &midbgColor, 1.5*i++, midText, midBox, setId);
	    sprintf(lowText, "%.1f", lowIdCutoff);
	    showConservCode("Low:", &lowfgColor, &lowbgColor, 1.5*i++, lowText, lowBox, setId);
	}
    
	i += 2;
	graphText("Note: foreground colors are only valid for different background colors.", 1, 1.5*i++);
	graphText("(Hit return after entering new cutoffs.)", 1, 1.5*i++);
	graphButton("OK", finishShowColorOK, 1.5, i*1.5+1);
	graphButton("Cancel", finishShowColorCancel, 5.5, i*1.5+1);

	setConservColors();
    }
    else {
	int used, col=1, row=1;

	for (i=1; i <= 20; i++) {
	    *res = b2a[i];
	    if (i == 11) {
		row = 1;
		col = 20;
	    }
	    graphText(res, col, row*1.5);
	    belvuConfColour(color+*res, color[(unsigned char)(*res)], col+2, row*1.5, 3, 
			    messprintf("%s", colorNames[color[(unsigned char)(*res)]]));
	    row++;
	}
	row++;
	graphText("Groups:", 1, row*1.5);
	for (i=0; i < NUM_TRUECOLORS; i++) {
	    used = 0;
	    for (j=65; j<96; j++) if (color[j] == i) used = 1;
	    if (used) {
		box = graphBoxStart();
		graphText(colorNames[i], 1, ++row*1.5);
		graphBoxEnd();
		graphBoxDraw(box, BLACK, i);
		graphText(":", 11, row*1.5);
		for (p=13, j=65; j<96; j++)
		    if (color[j] == i) {
			graphColor(BLACK);
			*res = j;
			graphText(res, p, row*1.5);
			p++;
		    }
	    }
	}

	graphButton("OK", finishShowColorOK, 1.5, ++row*1.5+1);
	graphButton("Cancel", finishShowColorCancel, 5.5, row*1.5+1);

	if (colorByResIdOn) setConservColors();
    }
    graphColor(BLACK);
    belvuRedraw();

    graphActivate(showColorGraph);
    graphRedraw();
    /* graphLoop(TRUE);*/
}


static void colorByResId(void)
{
  ACEIN ace_in ;

  if ((ace_in = messPrompt ("Only colour residues above %identity: ", 
			     messprintf("%.1f", colorByResIdCutoff), 
			    "fz", 0)))
    {
      aceInFloat(ace_in, &colorByResIdCutoff);
      aceInDestroy(ace_in);
  
      color_by_similarity = 0;
  
      colorByResIdOn = 1;
      setConservColors();

      belvuRedraw();
    }

  return ;
}


static void colorRect(void)
{
    colorRectangles = (!colorRectangles);
    belvuRedraw();
}


static void readColorCodes(FILE *fil, int *colorarr)
{
    char *cp, line[MAXLINE+1], setColor[MAXLINE+1];
    unsigned char c ;
    int i, colornr;

    while (!feof(fil)) {
	if (!fgets (line, MAXLINE, fil)) break;
	
	/* remove newline */
	if ((cp = strchr(line, '\n'))) *cp = 0 ;

	/* Parse color of organism in tree 
	   Format:  #=OS BLUE D. melanogaster*/
	if (!strncmp(line, "#=OS ", 5)) {

	    cp = line+5;
	    sscanf(cp, "%s", setColor);
	    for (colornr = -1, i = 0; i < NUM_TRUECOLORS; i++)
		if (!strcasecmp(colorNames[i], setColor)) colornr = i;
	    if (colornr == -1) {
		printf("Unrecognized color: %s, using black instead.\n", setColor);
		colornr = BLACK;
	    }
	    
	    while(*cp == ' ') cp++;
	    while(*cp != ' ') cp++;
	    while(*cp == ' ') cp++;
	    
	    /* Find organism and set its colour */
	    aln.organism = cp;
	    if (!arrayFind(organismArr, &aln, &ip, (void*)organism_order))
		messout(messprintf("Cannot find organism \"%s\", specified in color code file. Hope that's ok", 
				   aln.organism));
	    else
		arrp(organismArr, ip, ALN)->color = colornr;
	}

	/* Ignore comments */
	if (*line == '#') continue;

	/* Parse character colours */
	if (sscanf(line, "%c%s", &c, setColor) == 2) 
	{
	    c = freeupper(c);
	    for (colornr = -1, i = 0; i < NUM_TRUECOLORS; i++)
		if (!strcasecmp(colorNames[i], setColor)) colornr = i;
	    if (colornr == -1) {
		printf("Unrecognized color: %s\n", setColor);
		colornr = 0;
	    }

	    colorarr[(unsigned char)(c)] = colornr;

	    if (c > 64 && c <= 96)
	      colorarr[(unsigned char)(c+32)] = colorarr[(unsigned char)(c)];
	    else if (c > 96 && c <= 128)
	      colorarr[(unsigned char)(c-32)] = colorarr[(unsigned char)(c)];
	}
    }
    fclose(fil);

    color_by_conserv = 0;
}


static void readColorCodesMenu(void)
{
    FILE *fil;

    if (!(fil = filqueryopen(dirName, fileName, "","r", "Read file:"))) return;

    readColorCodes(fil, color);
    belvuRedraw();
}


static void saveColorCodes(void)
{
    FILE *fil;
    int i;

    if (!(fil = filqueryopen(dirName, fileName, "","w", "Save to file:"))) return;

    for (i = 1; i < 21; i++)
      {
	fprintf(fil, "%c %s\n", b2a[i], colorNames[color[(unsigned char)(b2a[i])]]);
      }

    fclose(fil);
}

static void colorRes(void)
{
    menuUnsetFlags(menuItem(colorMenu, thresholdStr), MENUFLAG_DISABLED);
    menuSetFlags(menuItem(colorMenu, printColorsStr), MENUFLAG_DISABLED);
    menuSetFlags(menuItem(colorMenu, ignoreGapsStr), MENUFLAG_DISABLED);
    color_by_conserv = colorByResIdOn = 0;
    belvuRedraw();
}


static void colorByResidue(void)
{
    colorScheme = COLORBYRESIDUE;
    colorRes();
}

static void initMarkupColors(void)
{
    markupColor['0'] = DARKBLUE;
    markupColor['1'] = BLUE;
    markupColor['2'] = MIDBLUE;
    markupColor['3'] = LIGHTBLUE;
    markupColor['4'] = VIOLET;
    markupColor['5'] = PALEBLUE;
    markupColor['6'] = PALECYAN;
    markupColor['7'] = CYAN;
    markupColor['8'] = CYAN;
    markupColor['9'] = CYAN;
    markupColor['A'] = markupColor['a'] = WHITE;
    markupColor['B'] = markupColor['b'] = RED;
    markupColor['C'] = markupColor['c'] = PALEYELLOW;
    markupColor['D'] = markupColor['d'] = WHITE;
    markupColor['E'] = markupColor['e'] = RED;
    markupColor['F'] = markupColor['f'] = WHITE;
    markupColor['G'] = markupColor['g'] = DARKGREEN;
    markupColor['H'] = markupColor['h'] = DARKGREEN;
    markupColor['I'] = markupColor['i'] = DARKGREEN;
    markupColor['J'] = markupColor['j'] = WHITE;
    markupColor['K'] = markupColor['k'] = WHITE;
    markupColor['L'] = markupColor['l'] = WHITE;
    markupColor['M'] = markupColor['m'] = WHITE;
    markupColor['N'] = markupColor['n'] = WHITE;
    markupColor['O'] = markupColor['o'] = WHITE;
    markupColor['P'] = markupColor['p'] = WHITE;
    markupColor['Q'] = markupColor['q'] = WHITE;
    markupColor['R'] = markupColor['r'] = WHITE;
    markupColor['S'] = markupColor['s'] = YELLOW;
    markupColor['T'] = markupColor['t'] = YELLOW;
    markupColor['V'] = markupColor['v'] = WHITE;
    markupColor['U'] = markupColor['u'] = WHITE;
    markupColor['W'] = markupColor['w'] = WHITE;
    markupColor['X'] = markupColor['x'] = WHITE;
    markupColor['Y'] = markupColor['y'] = WHITE;
    markupColor['Z'] = markupColor['z'] = NOCOLOR;
}


static void initResidueColors(void)
{
    colorScheme = COLORBYRESIDUE;

    color['A'] = color['a'] = WHITE;
    color['B'] = color['b'] = NOCOLOR;
    color['C'] = color['c'] = WHITE;
    color['D'] = color['d'] = WHITE;
    color['E'] = color['e'] = WHITE;
    color['F'] = color['f'] = WHITE;
    color['G'] = color['g'] = WHITE;
    color['H'] = color['h'] = WHITE;
    color['I'] = color['i'] = WHITE;
    color['J'] = color['j'] = WHITE;
    color['K'] = color['k'] = WHITE;
    color['L'] = color['l'] = WHITE;
    color['M'] = color['m'] = WHITE;
    color['N'] = color['n'] = WHITE;
    color['O'] = color['o'] = WHITE;
    color['P'] = color['p'] = WHITE;
    color['Q'] = color['q'] = WHITE;
    color['R'] = color['r'] = WHITE;
    color['S'] = color['s'] = WHITE;
    color['T'] = color['t'] = WHITE;
    color['V'] = color['v'] = WHITE;
    color['U'] = color['u'] = WHITE;
    color['W'] = color['w'] = WHITE;
    color['X'] = color['x'] = WHITE;
    color['Y'] = color['y'] = WHITE;
    color['Z'] = color['z'] = NOCOLOR;
}


static void colorSchemeCys(void)
{
    colorScheme = COLORSCHEMECYS;

    initResidueColors();
    color['C'] = color['c'] = CYAN;
    color['G'] = color['g'] = RED;
    color['P'] = color['p'] = GREEN;

    colorRes();
}


static void colorSchemeEmpty(void)
{
    colorScheme = COLORSCHEMEEMPTY;

    initResidueColors();
    colorRes();
}


static void colorSchemeStandard(void)
{
    colorScheme = COLORSCHEMESTANDARD;

    /* Erik's favorite colours:

       C        - MIDBLUE
       GP       - CYAN
       HKR      - GREEN
       AFILMVWY - YELLOW
       BDENQSTZ - LIGHTRED
    */

    color['A'] = color['a'] = YELLOW;
    color['B'] = color['b'] = NOCOLOR;
    color['C'] = color['c'] = MIDBLUE;
    color['D'] = color['d'] = LIGHTRED;
    color['E'] = color['e'] = LIGHTRED;
    color['F'] = color['f'] = YELLOW;
    color['G'] = color['g'] = CYAN;
    color['H'] = color['h'] = GREEN;
    color['I'] = color['i'] = YELLOW;
    color['K'] = color['k'] = GREEN;
    color['L'] = color['l'] = YELLOW;
    color['M'] = color['m'] = YELLOW;
    color['N'] = color['n'] = LIGHTRED;
    color['P'] = color['p'] = CYAN;
    color['Q'] = color['q'] = LIGHTRED;
    color['R'] = color['r'] = GREEN;
    color['S'] = color['s'] = LIGHTRED;
    color['T'] = color['t'] = LIGHTRED;
    color['V'] = color['v'] = YELLOW;
    color['W'] = color['w'] = YELLOW;
    color['Y'] = color['y'] = YELLOW;
    color['Z'] = color['z'] = NOCOLOR;

    colorRes();
}


static void colorSchemeGibson(void)
{
    colorScheme = COLORSCHEMEGIBSON;

/* Colour scheme by Gibson et. al (1994) TIBS 19:349-353

   Listed in Figure 1:


   Gibson      AA        Here

   orange      G         ORANGE (16-colours: LIGHTRED)
   yellow      P         YELLOW
   blue        ACFILMVW  MIDBLUE
   light blue  Y         LIGHTBLUE (16-colours: CYAN)
   green       NQST      GREEN
   purple      DE        PURPLE (16-colours: MAGENTA)
   red         RK        RED
   pink        H         LIGHTRED (16-colours: DARKRED)
*/

    color['A'] = color['a'] = MIDBLUE;
    color['B'] = color['b'] = NOCOLOR;
    color['C'] = color['c'] = MIDBLUE;
    color['D'] = color['d'] = PURPLE;
    color['E'] = color['e'] = PURPLE;
    color['F'] = color['f'] = MIDBLUE;
    color['G'] = color['g'] = ORANGE;
    color['H'] = color['h'] = LIGHTRED;
    color['I'] = color['i'] = MIDBLUE;
    color['K'] = color['k'] = RED;
    color['L'] = color['l'] = MIDBLUE;
    color['M'] = color['m'] = MIDBLUE;
    color['N'] = color['n'] = GREEN;
    color['P'] = color['p'] = YELLOW;
    color['Q'] = color['q'] = GREEN;
    color['R'] = color['r'] = RED;
    color['S'] = color['s'] = GREEN;
    color['T'] = color['t'] = GREEN;
    color['V'] = color['v'] = MIDBLUE;
    color['W'] = color['w'] = MIDBLUE;
    color['Y'] = color['y'] = LIGHTBLUE;
    color['Z'] = color['z'] = NOCOLOR;

    colorRes();
}



#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
static void setColorCodes(void)
{
    int 
	i, colornr;
    static char 
	aa[64]="", 
	col[64]="", 
	*colors=0, 
	*cp;

    if (!(ace_in = messPrompt ("Give amino acid and colour (e.g. GPC BLUE) ", 
		     messprintf("%s %s", aa, col), 
			       "ww", 0)))
	return;

    colorScheme = COLORBYRESIDUE;

    strncpy(aa, aceInWord(ace_in), 63);
    strncpy(col, aceInWord(ace_in), 63);
    aceInDestroy(ace_in);

    for (colornr = -1, i = 0; i < NUM_TRUECOLORS; i++)
	if (!strcasecmp(colorNames[i], col)) colornr = i;
    if (colornr == -1) {
	if (!colors) {
	    colors = messalloc(12*NUM_TRUECOLORS);
	    for (i = 0; i < NUM_TRUECOLORS; i++) {
		strcat(colors, colorNames[i]);
		strcat(colors, " ");
	    }
	}
	messout(messprintf("Unrecognized color: %s. Choose from: %s\n", col, colors));
	colornr = 0;
    }
    
    for (cp = aa; *cp; cp++) {
      color[(unsigned char)(*cp)] = colornr;
	
	if (*cp < 91) 
	    color[*cp+32] = colornr;
	else
	    color[*cp-32] = colornr;
    }
    
    colorRes();
}
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */



/* call an external shell command and print output in a text_scroll window
 *
 * This is a replacement for the old graph based text window, it has the advantage
 * that it uses gtk directly and provides cut/paste/scrolling but...it has the
 * disadvantage that it will use more memory as it collects all the output into
 * one string and then this is _copied_ into the text widget.
 * 
 * If this proves to be a problem I expect there is a way to feed the text to the
 * text widget a line a time. */
void externalCommand (char *command)
{
#if !defined(MACINTOSH)
  FILE *pipe ;
  char text[MAXLINE+1], *cp ;
  int line=0, len, maxlen=0;
  static Stack stack ;
  Graph old ;
  GString *str_text ;

  old = graphActive() ;

  str_text = g_string_new(NULL) ;

  stack = stackReCreate (stack, 50) ;

  pipe = popen (command, "r") ;
  while (!feof (pipe))
    { 
      if (!fgets (text, MAXLINE, pipe))
	break;

      len = strlen (text) ;
      if (len)
	{ 
	  if (text[len-1] == '\n') 
	    text[len-1] = '\0';
	  pushText (stack, text) ;
	  line++;
	  if (len > maxlen)
	    maxlen = len;
	}
    }
  pclose (pipe);

  stackCursor(stack, 0) ;

  while ((cp = stackNextText (stack)))
    {
      g_string_append_printf(str_text, "%s\n", cp) ;
    }

  gexTextEditorNew(command, str_text->str, 0,
		   NULL, NULL, NULL,
		   FALSE, FALSE, TRUE) ;

  g_string_free(str_text, TRUE) ;

  graphActivate (old) ;

#endif
  return ;
}






static void xmosaicFetch(void)
{
    xmosaic = (xmosaic ? 0 : 1);
    belvuRedraw();
}


static void wrapAlignmentWindow(void)
{
    /*  Column makeup:
	space, maxNameLen, 2xspace, maxEndLen, space, maxLen, maxEndLen
	i.e. 4 spaces
	*/

    int 
	paragraph=0, alnstart, alnend, alnlen, line=1, 
	i, j, oldpos, bkcol, empty, totCollapsed=0,
	collapseRes = 0, collapsePos = 0, collapseOn = 0;
    char 
	collapseStr[10],
	ch[2] = " ";
    static char 
	*seq=0, title[256];
    static int  
	*pos=0,			/* Current residue position of sequence j */
	linelen=80;
    float
	xpos;
    ACEIN wrap_in;

    if (!pos) strcpy(title, Title);
    if (!(wrap_in = messPrompt("Give line width and title (separated by a space):", 
		     messprintf("%d %s", linelen, title), "it", 0))) return;
    aceInInt(wrap_in, &linelen);
    strncpy(title, aceInWord(wrap_in), 255);
    title[255] = 0;
    aceInDestroy(wrap_in);
    wrap_in = NULL ;

    seq = messalloc(linelen+1);
    wrapLinelen = maxNameLen + maxEndLen + maxEndLen + 5 +
		   (linelen > maxLen ? maxLen : linelen);

    if (!pos) pos = (int *)messalloc(nseq*sizeof(int *));
    for (j = 0; j < nseq; j++) pos[j] = arrp(Align, j, ALN)->start;

    graphCreate(TEXT_FULL_SCROLL, Title, 0, 0, screenWidth, screenHeight);
    graphTextFormat(FIXED_WIDTH);
    graphRegister(LEFT_DOWN, leftDown);
    graphMenu(wrapMenu);

    if (*title) {
	graphText(title, 1, 1);
	line += 2;
    }

    while (paragraph*linelen +totCollapsed < maxLen)
    {
	for (j = 0; j < nseq; j++)
	{
	    alnstart = paragraph*linelen +totCollapsed; 
/*printf("alnstart= %d  totCollapsed= %d  collapsePos= %d \n", alnstart, totCollapsed, collapsePos);*/
	    alnlen = ( (paragraph+1)*linelen +totCollapsed < maxLen ? linelen : maxLen-alnstart );
	    alnend = alnstart + alnlen;

	    alnp = arrp(Align, j, ALN);
	    if (alnp->hide) continue;

	    for (empty=1, i = alnstart; i < alnend; i++) {
		if (!isGap(alnp->seq[i]) && alnp->seq[i] != ' ') 
		    empty = 0;
	    }

	    if (!empty) {

		for (collapsePos = 0, oldpos = pos[j], i = alnstart; i < alnend; i++) {	

		    xpos = maxNameLen+maxEndLen+4+i-alnstart -collapsePos;

		    if (alnp->seq[i] == '[') {
			/* Experimental - collapse block:
			   "[" -> Collapse start
			   "]" -> Collapse end

			   e.g.
			   
			   1 S[..........]FKSJFE
			   2 L[RVLKLKYKNS]KDEJHF
			   3 P[RL......DK]FKEJFJ

			          |
				  V

			   1 S[  0]FKSJFE
			   2 L[ 10]KDEJHF
			   3 P[  4]FKEJFJ
			   
			   Minimum collapse = 5 chars.
			   The system depends on absolute coherence in format, 
			   very strange results will be generated otherwise.
			   Edges need to be avoided manually.
			*/
			collapseOn = 1;
			collapsePos += 1;
			collapseRes = 0;
			continue;
		    }
		    if (alnp->seq[i] == ']') {
			collapseOn = 0;
			collapsePos -= 4;
			alnend -= 3;

			sprintf(collapseStr, "[%3d]", collapseRes);

/*printf("\n%s\n", collapseStr);*/

			graphText(collapseStr, xpos, line);
			continue;
		    }
		    if (collapseOn) {
			collapsePos++;
			if (!isGap(alnp->seq[i])) {
			    collapseRes++;
			    pos[j]++;
			}
			alnend++;
			continue;
		    }


		    if (!isGap(alnp->seq[i]) && alnp->seq[i] != ' ') {
			bkcol = findResidueBGcolor(alnp, i);
			graphColor(bkcol);
			graphFillRectangle(xpos, line, xpos+1, line+1);
			pos[j]++;
		    }
		    else
			bkcol = WHITE;


		    /* Foreground color */
		    if (color_by_conserv)
			graphColor(bg2fgColor(bkcol));
		    else
			graphColor(BLACK);
		    
		    *ch = alnp->seq[i];
		    graphText(ch, xpos, line);

		    graphColor(BLACK);
		}
		
		/*if (!(color_by_conserv && printColorsOn)) {
		    strncpy(seq, alnp->seq+alnstart, alnend-alnstart);
		    seq[alnend-alnstart] = 0;
		    graphText(seq, maxNameLen+maxEndLen+4, line);
		}*/

		graphText(alnp->name, 1, line);

		if (!alnp->markup) {
		    graphText(messprintf("%*d", maxEndLen, oldpos), maxNameLen+3, line);
		    if (alnend == maxLen) 
			graphText(messprintf("%-d", pos[j]-1), maxNameLen+maxEndLen+alnlen+5, line);
		}

		line++;
	    }
	}
	paragraph++;
	line++;
	totCollapsed += collapsePos;
    }
    messfree(seq);
    graphTextBounds(wrapLinelen+4, line + 2);
    graphRedraw();
}


/* Calculate overhang between two aligned sequences 
   Return nr of residues at the ends unique to s1 (overhanging s2).
*/
static int alnOverhang(char *s1, char *s2)
{
    int overhang = 0,
	s1started = 0,
	s2started = 0;
    char *s1save = s1, *s2save = s2;

    for (; *s1 && *s2; s1++, s2++) {
	if (!isGap(*s1)) s1started = 1;
	if (!isGap(*s2)) s2started = 1;
	if (s1started && !s2started) overhang++;
    }	
    
    s1--; s2--;
    s1started = s2started = 0;
    for (; s1>=s1save && s2>=s2save; s1--, s2--) {
	if (!isGap(*s1)) s1started = 1;
	if (!isGap(*s2)) s2started = 1;
	if (s1started && !s2started) overhang++;
    }	

    /* printf ("%s\n%s\nOverhang=%d\n", s1save, s2save, overhang);*/

    return overhang;
}


/* Calculate percent identity of two strings */
static float identity(char *s1, char *s2)
{
    int n, id;

    for (n = id = 0; *s1 && *s2; s1++, s2++) {
	if (isGap(*s1) && isGap(*s2)) continue;
	if (isGap(*s1) || isGap(*s2))
	    if (!penalize_gaps) continue;
	n++;
	if (toupper(*s1) == toupper(*s2)) id++;
    }

    if (n)
	return (float)id/n*100;
    else
	return 0.0;
}


/* Calculate score of two sequences */
static float score(char *s1, char *s2)
{
    float sc=0.0;

    for (;*s1 && *s2; s1++, s2++) {

	if (isGap(*s1) && isGap(*s2)) 
	    continue;
	else if (isGap(*s1) || isGap(*s2)) {
	    if (penalize_gaps) sc -= 0.6;
	}
	else
	  sc += (float) BLOSUM62[a2b[(unsigned char)(*s1)]-1][a2b[(unsigned char)(*s2)]-1];
    }
    
    return sc;
}



#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
/* Calculate score of two sequences */
static float mtxscore(char *s1, char *s2, int mtx[24][24])
{
    float sc=0.0;

    for ( ;*s1; s1++, s2++) {

	if (penalize_gaps) {
	    if (isGap(*s1) || isGap(*s2)) sc -= 0.6;	
	}

	if (!isGap(*s1) && !isGap(*s2))
	  sc += (float) mtx[a2b[(unsigned char)(*s1)]-1][a2b[(unsigned char)(*s2)]-1];
    }
    
    return sc;
}
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */




static void rmFinalise(void) 
{
  /*    ruler[maxLen] = 0;*/
    checkAlignment();
    setConservColors();
    belvuRedraw();
}
static void rmFinaliseGapRemoval(void)
{
    if (rmEmptyColumnsOn) rmEmptyColumns(1.0);
    rmFinalise();
}
static void rmFinaliseColumnRemoval(void)
{
    rmGappySeqs(100.0);
    rmFinalise();
}



/* Get rid of seqs that are less than x% identical with any of the others. 
 */
static void rmOutliers(void)
{
    int i,j, n=0;
    ALN *alni, *alnj;
    static float cutoff = 20.0, id, maxid;

    if (!(ace_in = messPrompt ("Remove sequences less identical to others than (%) :", 
		      messprintf("%.0f", cutoff), 
			       "fz", 0)))
	return;

    aceInFloat(ace_in, &cutoff);
    aceInDestroy(ace_in);
    ace_in = NULL ;

    for (i = 0; i < nseq-1; ) {
	alni = arrp(Align, i, ALN);

	for (maxid=0, j = 0; j < nseq; j++) {
	    if (i == j) continue;
	    alnj = arrp(Align, j, ALN);
	    id = identity(alni->seq, alnj->seq);
	    if (id > maxid) maxid = id;
	}

	if (maxid < cutoff) {

	    printf("%s/%d-%d was max %.1f%% identical to any other sequence and was removed.\n",
		   alni->name, alni->start, alni->end, maxid);
	    
	    /* Remove entry */
	    n++;
	    nseq--;
	    if (Highlight_alnp == alni) Highlight_alnp = 0;
	    arrayRemove(Align, alni, (void*)nrorder);
	    saved = 0;
	}
	else i++;
    }

    printf("%d sequences removed at the %.0f%% level.  %d seqs left\n\n", n, cutoff, nseq);

    arrayOrder();
    rmFinaliseGapRemoval();
}


/* Get rid of seqs that start or end with a gap.
 */
static void rmPartial(void)
{
    int i, n=0;
    ALN *alni;

    for (i = 0; i < nseq; ) {
	alni = arrp(Align, i, ALN);

	if (isGap(alni->seq[0]) || isGap(alni->seq[maxLen-1])) {

	    /* Remove entry */
	    n++;
	    nseq--;
	    if (Highlight_alnp == alni) Highlight_alnp = 0;
	    arrayRemove(Align, alni, (void*)nrorder);
	    saved = 0;
	}
	else i++;
    }

    fprintf(stderr, "%d partial sequences removed.  %d seqs left\n\n", n, nseq);

    arrayOrder();
    rmFinaliseGapRemoval();
}


/* Get rid of seqs that are too gappy.
 */
static void rmGappySeqs(float cutoff)
{
    int i, j, n=0, gaps;
    ALN *alni;

    for (i = 0; i < nseq; ) {
	alni = arrp(Align, i, ALN);

	for (gaps = j = 0; j < maxLen; j++)
	    if (isGap(alni->seq[j])) gaps++;

	if ((float)gaps/maxLen >= cutoff/100.0) {
	    /* Remove entry */
	    n++;
	    nseq--;
	    if (Highlight_alnp == alni) Highlight_alnp = 0;
	    arrayRemove(Align, alni, (void*)nrorder);
	    saved = 0;
	}
	else i++;
    }

    fprintf(stderr, "%d gappy sequences removed.  %d seqs left\n\n", n, nseq);

    arrayOrder();
}


static void rmGappySeqsPrompt()
{
    static float cutoff = 50.0 ;

    if (!(ace_in = messPrompt ("Remove sequences with more (or equal) gaps than (%) :", 
			       messprintf("%.0f", cutoff), 
			       "fz", 0)))
	return;

    aceInFloat(ace_in, &cutoff);
    aceInDestroy(ace_in);
    ace_in = NULL ;

    rmGappySeqs(cutoff);
    rmFinaliseGapRemoval();
}

static void readLabels(void)
{
    int row, col, seqpos, seqlen;
    ALN *alnrow;
    char 
      *labelseq,		/* The raw sequence of labels */
      *label,			/* The mapped sequence of labels, 1-maxlen */
      *cp, *cq;
    FILE *fil;

    if (!Highlight_alnp) {
        messout("pick a sequence first");
	return;
    }

    labelseq = messalloc(maxLen+1);
    label = messalloc(maxLen+1);
    if (!(fil = filqueryopen(dirName, fileName, "","r", 
			     messprintf("Read labels of %s from file:", Highlight_alnp->name))))
      return;

    /* read file */
    cq = labelseq;
    while (!feof (fil)) { 
	if (!fgets (line, MAXLENGTH, fil)) break;
	cp = line;
	while (*cp) {
	    if (isalpha(*cp)) *cq++ = *cp;
	    cp++;
	}
    }
    fclose(fil);
    
    /* Warn if seq too long, return if too short */
    seqlen = Highlight_alnp->end - Highlight_alnp->start +1;
    if (strlen(labelseq) > seqlen)
        messout(messprintf("The sequence of labels is longer (%d) than the sequence (%d).  "
			   "Hope that's ok", 
			   strlen(labelseq), seqlen));
    if (strlen(labelseq) < seqlen) {
        messout(messprintf("The sequence of labels is shorter (%d) than the sequence (%d).  "
			   "Aborting", 
			   strlen(labelseq), seqlen));
	return;
    }

    /* map labels to alignment */
    for (col = 0, seqpos = 0; col < maxLen; col++) {
        label[col] = labelseq[seqpos];
	if (isalpha(Highlight_alnp->seq[col]) && labelseq[seqpos+1]) seqpos++;
    }

    for (row = 0; row < nseq; row++) {
	alnrow = arrp(Align, row, ALN);
	for (col = 0; col < maxLen; col++)
  	    if (isalpha(alnrow->seq[col]))
	        alnrow->seq[col] = label[col];
    }
      
    messfree(labelseq);
    belvuRedraw();
}


static void gapCharOK (void) {

    int i,j;

    for (i = 0; i < nseq; i++) {
	alnp = arrp(Align, i, ALN);
	for (j = 0; j < maxLen; j++) {
	    if (isGap(alnp->seq[j])) alnp->seq[j] = gapChar;
	}
    }
    graphDestroy();
    belvuRedraw();
}


static void gapCharToggle(void) {
    if (gapChar == '.')
	gapChar = '-';
    else 
	gapChar = '.';
    selectGaps();
}


static void selectGaps(void) {

    char text[10];

    if (!graphActivate(gapCharGraph)) {
	gapCharGraph = graphCreate (TEXT_FIT, "Select gap char", 0, 0, 0.5, 0.25);

	graphTextFormat(FIXED_WIDTH);
    }
    graphPop();
    graphClear();

    graphText("Gap character:", 1, 2);
    strcpy(text, messprintf(" %c ", gapChar));
    graphButton(text, gapCharToggle, 16, 2);

    graphButton("OK", gapCharOK, 1, 4);
    graphButton("Cancel", graphDestroy, 1, 5.5);

    graphRedraw();
}


/* Get rid of seqs that are more than x% identical with others. 
 * Keep the  first one.
 */
static void mkNonRedundant(float cutoff)
{
    int i,j, n=0, overhang;
    ALN *alni, *alnj;
    float id;

    for (i = 0; i < nseq; i++) {
	alni = arrp(Align, i, ALN);
	for (j = 0; j < nseq; j++) {

	    if (i == j) continue;

	    alnj = arrp(Align, j, ALN);
	    
	    overhang = alnOverhang(alnj->seq, alni->seq);
	    id = identity(alni->seq, alnj->seq);

	    if (!overhang && id > cutoff)
	    {

		fprintf(stderr, "%s/%d-%d and %s/%d-%d are %.1f%% identical. "
		       "The first includes the latter which was removed.\n",
		       alni->name, alni->start, alni->end,
		       alnj->name, alnj->start, alnj->end,
		       id);
		
		/* Remove entry j */
		n++;
		nseq--;
		if (Highlight_alnp == alnj) Highlight_alnp = 0;
		arrayRemove(Align, alnj, (void*)nrorder);
		saved = 0;
		if (j < i) {
		    i--;
		    alni = arrp(Align, i, ALN);
		}
		j--;
	    }
	}
    }

    fprintf(stderr, "%d sequences removed at the %.0f%% level.  %d seqs left\n\n", n, cutoff, nseq);

    arrayOrder();
    rmFinaliseGapRemoval();
}


static void mkNonRedundantPrompt(void)
{
    static float cutoff = 80.0;

    if (!(ace_in = messPrompt ("Remove sequences more identical than (%):", 
		      messprintf("%.0f", cutoff), 
			       "fz", 0)))
	return;
    
    aceInFloat(ace_in, &cutoff);
    aceInDestroy(ace_in);
    ace_in = NULL ;

    mkNonRedundant(cutoff);
}


static void rmScore(void)
{
    int 
	i;
    static float 
	cutoff = 20.0;

    if (!(ace_in = messPrompt ("Remove sequences scoring less than: ", 
		      messprintf("%.0f", cutoff), 
			       "fz", 0)))
	return;

    aceInFloat(ace_in, &cutoff);
    aceInDestroy(ace_in);
    ace_in = NULL ;

    scoreSort();

    /* Save Highlight_alnp */
    if (Highlight_alnp)
	alncpy(&aln, Highlight_alnp);

    for (i = 0; i < nseq; ) {
	alnp = arrp(Align, i, ALN);
	if (alnp->score < cutoff) {
	    fprintf(stderr, "Removing %s/%d-%d (score %.1f\n",
		   alnp->name, alnp->start, alnp->end, alnp->score);
		
	    nseq--;
	    if (Highlight_alnp == alnp) Highlight_alnp = 0;
	    arrayRemove(Align, alnp, (void*)nrorder);
	    saved = 0;
	}
	else
	    i++;
    }
    
    arrayOrder();

    /* Find Highlight_alnp in new array */
    if (Highlight_alnp) {
	if (!arrayFind(Align, &aln, &ip, (void*)scoreorder)) {
	    Highlight_alnp = 0;
	}
	else
	    Highlight_alnp = arrp(Align, ip, ALN);
	centerHighlighted();
    }

    AlignYstart = 0;

    rmFinaliseGapRemoval();
}


static void listIdentity(void)
{
    int 
      i,j,n ;
    ALN 
	*alni, *alnj;
    float 
	sc, totsc=0, maxsc=0, minsc=1000000,
	id, totid=0.0, maxid=0.0, minid=100.0;

    for (i = n = 0; i < nseq-1; i++) {
	alni = arrp(Align, i, ALN);
	for (j = i+1; j < nseq; j++, n++) {
	    alnj = arrp(Align, j, ALN);
	    
	    id = identity(alni->seq, alnj->seq);
	    totid += id;
	    if (id > maxid) maxid = id;
	    if (id < minid) minid = id;
	    
	    sc = score(alni->seq, alnj->seq);
	    totsc += sc;
	    if (sc > maxsc) maxsc = sc;
	    if (sc < minsc) minsc = sc;
	    
	    printf("%s/%d-%d and %s/%d-%d are %.1f%% identical, score=%f\n",
		   alni->name, alni->start, alni->end,
		   alnj->name, alnj->start, alnj->end,
		   id, sc);
		
	}
	printf("\n");
    }
    printf("Maximum %%id was: %.1f\n", maxid);
    printf("Minimum %%id was: %.1f\n", minid);
    printf("Mean    %%id was: %.1f\n", totid/n);
    printf("Maximum score was: %.1f\n", maxsc);
    printf("Minimum score was: %.1f\n", minsc);
    printf("Mean    score was: %.1f\n", (float)totsc/n);
}


static void markup (void)
{
    if (!Highlight_alnp) {
	messout ("Pick a sequence first!");
	return;
    }

    /* Store orignal state in the nocolor field:
       1 = normal sequence
       2 = markup line

       This is needed to restore markup lines (nocolor lines are always markups)
    */

    if (!Highlight_alnp->nocolor) {
	if (Highlight_alnp->markup)
	    Highlight_alnp->nocolor = 2;
	else
	    Highlight_alnp->nocolor = 1;
	Highlight_alnp->markup = 1;
    }
    else {
	if (Highlight_alnp->nocolor == 1) 
	    Highlight_alnp->markup = 0;
	Highlight_alnp->nocolor = 0;
    }
    
    belvuRedraw();
}


static void hide (void)
{
    if (!Highlight_alnp) {
	messout ("Pick a sequence first!");
	return;
    }

    Highlight_alnp->hide = 1;
    belvuRedraw();
}


static void unhide (void)
{
    int i;

    for (i = 0; i < nseq; i++)
	arrp(Align, i, ALN)->hide = 0;
    
    belvuRedraw();
}


static void rmPicked(void)
{
    if (!Highlight_alnp) {
	messout ("Pick a sequence first!");
	return;
    }

    printf("Removed %s/%d-%d.  ", Highlight_alnp->name, Highlight_alnp->start, Highlight_alnp->end);

    nseq--;
    arrayRemove(Align, Highlight_alnp, (void*)nrorder);
    saved = 0;
    arrayOrder();
    Highlight_alnp = 0;

    printf("%d seqs left\n\n", nseq);

    rmFinaliseGapRemoval();
}


static void rmPickLeftOff(void)
{
    rmPickLeftOn = 0;
}

static void rmPickLeft(void)
{
    rmPickLeftOn = 1;
    graphRegister(MESSAGE_DESTROY, rmPickLeftOff);
    graphMessage("Remove sequences by double clicking.  Remove this window when finished.");

}


/* Removes all columns between from and to, inclusively.
 * Note that from and to are sequence coordinates, 1...maxLen !!!!!  */
static void rmColumn(int from, int to)
{
    int
	i, j, len = to - from + 1;
    ALN 
	*alni;

    fprintf(stderr, "Removing Columns %d-%d.\n", from, to);

    for (i = 0; i < nseq; i++) {
	alni = arrp(Align, i, ALN);

	/* If N or C terminal trim, change the coordinates */
	if (from == 1)
	    for (j = from; j <= to;  j++) {
		/* Only count real residues */
		if (!isGap(alni->seq[j-1]))
		    (alni->start < alni->end ? alni->start++ : alni->start--); 
	    }
	if (to == maxLen)
	    for (j = from; j <= to;  j++) {
		/* Only count real residues */
		if (!isGap(alni->seq[j-1]))
		    (alni->start < alni->end ? alni->end-- : alni->end++); 
	    }
	
	/* Remove the columns */
	for (j = 0; j < maxLen-to+1 /* Including terminator 0 */;  j++) {
	    alni->seq[from+j-1] = alni->seq[to+j];
	}
	j--;
	if (alni->seq[from+j-1] || alni->seq[to+j])
	    printf("Still a bug in rmColumn(): End=%c, Oldend=%c\n", 
		   alni->seq[from+j-1],
		   alni->seq[to+j]);
    }

    maxLen -= len;

    saved = 0;
}


/* Inserts n gap columns after position p, in sequence coordinate, 1...maxLen 
 */
static void insertColumns(int p, int n)
{
    int
	i, j;

    ALN 
	*alni;
    char
	*dest, *src, *seq;

    printf("Inserting %d columns after column %d\n", n, p);

    maxLen += n;

    for (i = 0; i < nseq; i++) {
	alni = arrp(Align, i, ALN);
	
	seq = messalloc(maxLen + 1);

	dest = seq;
	src = alni->seq;

	for (j = 0; j < p;  j++) {
	    *dest++ = *src++;
	}
	for (; j < p+n;  j++) {
	    *dest++ = '.';
	}
	for (; j < maxLen;  j++) {
	    *dest++ = *src++;
	}

	messfree(alni->seq);
	alni->seq = seq;
    }

    saved = 0;
}


static void rmEmptyColumnsToggle(void)
{
    rmEmptyColumnsOn = !rmEmptyColumnsOn;
    belvuRedraw();
}


static void rmEmptyColumns(float cutoff)
{
    int
	i, j, gaps, totseq, removed=0, oldmaxLen=maxLen;
    ALN 
	*alni;
    char
	c;

    for (totseq = i = 0; i < nseq; i++) {
	alni = arrp(Align, i, ALN);
	if (!alni->markup) totseq++;
    }

    for (j = maxLen-1; j >= 0; j--) {
	for (gaps = i = 0; i < nseq; i++) {
	    alni = arrp(Align, i, ALN);
	    if (!alni->markup) {
		c = alni->seq[j];
		if (isGap(c) || c == ' ') gaps++;
	    }
	}

	if ((float)gaps/totseq >= cutoff) {
	    rmColumn(j+1, j+1);
	    if (++removed == oldmaxLen)
		messExit("You have removed all columns.  Prepare to exit Belvu");
	}
    }
}


static void rmEmptyColumnsInteract(void)
{
    static float 
	cutoff = 50.0;

    if (!(ace_in = messPrompt ("Remove columns with higher fraction of gaps than (%): ", 
		      messprintf("%.0f", cutoff), 
			       "fz", 0)))
	return;

    aceInFloat(ace_in, &cutoff);
    aceInDestroy(ace_in);
    ace_in = NULL ;

    rmEmptyColumns(cutoff/100.0);
    rmFinaliseColumnRemoval();
}


static void rmColumnCutoff(void)
{
    int 
	i, j, max, removed=0, oldmaxLen=maxLen;
    static float 
	from = -1,
	to = 0.9,
      cons ;

    if (!color_by_conserv) {
	messout("You must use a conservation coloring scheme");
	return;
    }

    if (!(ace_in = messPrompt ("Remove columns with a (maximum) conservation between x1 and x2 ( i.e. if (c > x1 && c <= x2) ).  Provide x1 and x2:", 
		      messprintf("%.2f %.2f", from, to), 
			       "ffz", 0)))
	return;

    aceInFloat(ace_in, &from);
    aceInFloat(ace_in, &to);
    aceInDestroy(ace_in);
    ace_in = NULL ;


    for (i = maxLen-1; i >= 0; i--) {

	if (color_by_similarity) {
	    cons = conservation[i];
	}
	else {
	    max = 0;
	    for (j = 1; j < 21; j++) {
		if (conservCount[j][i] > max) {
		    max = conservCount[j][i];
		}
	    }	
	    cons = (float)max/nseq;
	}


	if (cons > from && cons <= to) {
	    printf("removing %d, cons= %.2f\n", i+1, cons);
	    rmColumn(i+1, i+1);
	    if (++removed == oldmaxLen) {
		messExit("You have removed all columns.  Prepare to exit Belvu");
	    }
	}
    }

    rmFinaliseColumnRemoval();
}


static void rmColumnPrompt(void)
{
    int 
	from=1, to = maxLen;
    
    if (!(ace_in = messPrompt ("Remove columns From: and To: (inclusive)", 
		      messprintf("%d %d", from, to), 
			       "iiz", 0)))
	return;

    aceInInt(ace_in, &from);
    aceInInt(ace_in, &to);
    aceInDestroy(ace_in);
    ace_in = NULL ;

    if (from < 1 || to > maxLen || to < from) {
	messout("Bad coordinates: %d-%d.  The range is: 1-%d", from, to, maxLen);
	return;
    }

    rmColumn(from, to);
    rmFinaliseColumnRemoval();
}


static void rmColumnLeft(void)
{
    int 
	from=1, to=pickedCol;
    
    if (!pickedCol) {
	messout("Pick a column first");
	return;
    }

    rmColumn(from, to);

    AlignXstart=0;

    rmFinaliseColumnRemoval();
}

static void rmColumnRight(void)
{
    int 
	from=pickedCol, to=maxLen;
    
    if (!pickedCol) {
	messout("Pick a column first");
	return;
    }

    rmColumn(from, to);

    rmFinaliseColumnRemoval();
}


static int countInserts(SEG *seg)
{
    int   
	Align_gap, Query_gap, gap, insert_counter=0;

    if (!seg)
      return insert_counter ;

    while (seg->next) {

	Align_gap = seg->next->start - seg->end - 1;
	if (Align_gap < 0) 
	    fatal("Negative Align_gap: %d (%d-%d)", Align_gap, seg->start, seg->next->end );
		
	Query_gap = seg->next->qstart - seg->qend - 1;
	if (Query_gap < 0) 
	    fatal("Negative Query_gap: %d (%d-%d)", Query_gap, seg->qstart, seg->next->qend );

	gap = Query_gap - Align_gap;
	if (gap > 0) {
	    insert_counter += gap;
	}

	seg = seg->next;
    }

    return insert_counter;
}

static void makeSegList(SEG **SegList, char *line)
{
    int n, i;
    char *p, *linecopy;
    SEG *seg, *prevseg=0;

    /* Count coordinates - has to be multiple of 4 */
    linecopy = messalloc(strlen(line)+1);
    strcpy(linecopy, line);
    n = 0;
    if (atoi(strtok(linecopy, " "))) n++;;
    while ( (p = strtok(0, " ")) && atoi(p) ) n++;
    messfree(linecopy);
    
    if (!n || n % 4) fatal("Segments not multiple of 4 ints (%s)", line);

    for (i = 0; i < n/4; i++) {
	seg = (SEG *)messalloc(sizeof(SEG));
	if (prevseg) 
	    prevseg->next = seg;
	else
	    *SegList = seg;
	prevseg = seg;

	seg->qstart = atoi(i ? strtok(0, " ") : strtok(line, " "));
	seg->qend = atoi(strtok(0, " "));
	seg->start = atoi(strtok(0, " "));
	seg->end = atoi(strtok(0, " "));

    }

    for (seg = *SegList; seg; seg = seg->next) {
	if (debug) printf("%d %d %d %d\n", seg->qstart, seg->qend, seg->start, seg->end);

	if (seg == *SegList && seg->qstart != 1)
	    fatal("Bad qstart: Must start on 1");

	if (seg->qstart < 1 || seg->qstart > maxLen)
	    fatal("Bad qstart: %d.  Range: 1-%d", seg->qstart, maxLen);
	if (seg->qend < 1 || seg->qend > maxLen)
	    fatal("Bad qend: %d.  Range: 1-%d", seg->qend, maxLen);
	if (seg->start < 1 || seg->start > maxLen)
	    fatal("Bad start: %d.  Range: 1-%d", seg->start, maxLen);
	if (seg->end < 1 || seg->end > maxLen)
	    fatal("Bad end: %d.  Range: 1-%d", seg->end, maxLen);

	if (seg->qstart > seg->qend)
	    fatal("qstart > qend  (%d > %d)", seg->qstart, seg->qend);
	if (seg->start > seg->end)
	    fatal("start > end  (%d > %d)", seg->start, seg->end);
    }
}


/* readMatch displays a matching sequences to the alignment so that
   the match can be displayed in relation to the whole family.

   Considerations:

   - Must work both from ACEDB and command line (and from Web server).

   Display:

   - The score should be visible (turn on score column.)

   - Should matches be displayed on separate lines or in the middle of the alignment?
     On top would be nice, but would require a lot of extra programming (status bar etc).
     Instead add to alignment (SEED usually fits on one screen) 
     Draw names with red background.  
     Add on top.


   Steps needed on command line:

   hmmb -W tmp HMM align
   selex2alignmap tmp > alignmap

   hmm?s -F HMM query | ?smapback map=alignmap > query.mapmatch  ( -> acedb)
   belvu -m query.mapmatch align

   The two last lines should be scripted in hmm?sBelvu


*/
static void readMatch(FILE *fil)
{
  /* Format:
       
  Using segments is more difficult than residues, but is
  necessary to display in ACEDB Pepmap.

  seg_match/3-30                   (matching segment name)
  WLPLHTLinsertAACGEFYLVDSLLKH     (only matching part, no pads!)
  1 7 3 9  14 28 10 24

  seq_match2...
  */
  int	orig_maxLen, tmp,
    Align_gap, Query_gap, gap, inserts, seglen,
    done_one=0, len,  was_saved=saved, insert_counter;
  char *cp, *seq, *rawseq = NULL, *seqp;
  SEG	*SegList, *seg;

  while (!done_one)
    {
      if (!fgets(line, MAXLENGTH, fil))
	break;

      /*printf("%s\n", line); continue;*/

      if (*line != '\n' && *line != '#')
	{

	  if (done_one)
	    {
	      messout("Sorry, can only do one match");
	      continue;
	    }

	  /* Name */
	  resetALN(&aln);
	  if (!strtok(line, "/"))
	    fatal("Bad format: %s", line);
	  strncpy(aln.name, line, MAXNAMESIZE);
	  aln.name[MAXNAMESIZE] = 0;
	  if ((cp = strchr(aln.name, ' ')))
	    *cp = 0;
	  if (maxNameLen  < strlen(aln.name))
	    maxNameLen = strlen(aln.name);

	  /* Start & End */
	  if (!(cp = strtok(0, "-"))) fatal("Bad start: %s", cp);
	  aln.start = atoi(cp);
	  if ((len=strlen(cp)) > maxStartLen)
	    maxStartLen = len;
	  if (maxStartLen < (tmp = strlen(messprintf("%d", aln.start))))
	    maxStartLen = tmp;
	
	  if (!(cp = strtok(0, " ")))
	    fatal("Bad end: %s", cp);
	  aln.end = atoi(cp);
	  if ((len=strlen(cp)) > maxEndLen)
	    maxEndLen = len;
	  if (maxEndLen   < (tmp = strlen(messprintf("%d", aln.end))))
	    maxEndLen = tmp;
    
	  if (alignFind(Align, &aln, &ip))
	    {
	      messout("Sorry, already have sequence %s/%d-%d\n", 
		      aln.name, aln.start, aln.end);
	      return;
	    }

	  /* Score */
	  if (!(cp = strtok(0, "\n")))
	    fatal("Bad score: %s", cp);

	  aln.score = atof(cp);
	  if ((len=strlen(messprintf("%.1f", aln.score))) > maxScoreLen)
	    maxScoreLen = len;
	  displayScores = 1;

	  /* Sequence */
	  if (!fgets(line, MAXLENGTH, fil))
	    break;

	  if ((cp = strchr(line, '\n')))
	    *cp = 0 ;
	  rawseq = messalloc(strlen(line)+1);
	  strcpy(rawseq, line);

	  /* Matching segments */
	  if (!fgets(line, MAXLENGTH, fil))
	    break;
	  if ((cp = strchr(line, '\n'))) *cp = 0;

	  makeSegList(&SegList, line);

	  inserts = countInserts(SegList);
	  seq = messalloc(maxLen + inserts + 1);
	  memset(seq, '.', maxLen + inserts);
	    
	  orig_maxLen = maxLen;

	  /* Add first segment */
	  seg = SegList;

	  seqp = seq + seg->start-1;
	  seglen = seg->end - seg->start+1;
	  strncpy(seqp, rawseq, seglen);
	  seqp += seglen; 

	  /* Add subsequent segments */
	  insert_counter = 0;
	  while (seg->next)
	    {
	      Align_gap = seg->next->start - seg->end - 1;
	      Query_gap = seg->next->qstart - seg->qend - 1;


#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
	      if (Align_gap > 0)
		{ 
		  /* Gap in query - Add (gap) pads to query */
		  memset(seqp, '.', Query_gap);
		  seqp += Query_gap;
		}
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

	      if (Query_gap > 0)
		{ 
		  strncpy(seqp, rawseq + seg->qend, Query_gap);
		  seqp += Query_gap; 
		}

	      gap = Query_gap - Align_gap;
	      if (gap > 0)
		{
		  insertColumns(seg->end+insert_counter, gap);
		  insert_counter += gap;
		}
	      else if (gap < 0)
		{
		  seqp += -gap;
		  if (debug)
		    printf("inserting %d gaps at %d\n", -gap, (int)(seqp-seq));
		}

	      seg = seg->next;

	      /* Add sequence segment */
	      seglen = seg->end - seg->start+1;
	      strncpy(seqp, rawseq + seg->qstart-1, seglen);
	      seqp += seglen;
	    }

	  /* Add final pads */
	  memset(seqp, '.', orig_maxLen - seg->end);

	  aln.len = strlen(seq);
	  aln.seq = seq;
	  aln.color = RED;
	  aln.nr = 0;

	  if (!arrayInsert(Align, &aln, (void*)nrorder))
	    fatal("Failed to insert %s into alignment", aln.name);
	  nseq++;

	  done_one = 1;

	} /* End of this matchSeq */
    }

  messfree(rawseq);

  arrayOrder();
  saved = was_saved;

  return ;
}



#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
static void readMatchPrompt(void)
{
    FILE *file;
    static char fileName[FIL_BUFFER_SIZE] ;
    
    if (!(file = filqueryopen(dirName, fileName, "","r", "Read file:"))) return;
   
    readMatch(file);
    belvuRedraw();
}
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */


/* Copied from master copy in dotter.c */
/* Bourne shells (popen) sometimes behave erratically - use csh instead
 */
int findCommand (char *command, char **retp)
{
#if !defined(NO_POPEN)
    FILE *pipe;
    static char retval[1025], *cp, csh[]="/bin/csh";

    /* Check if csh exists */
    if (access(csh, X_OK)) {
	messout("Could not find %s", csh);
	return 0;
    }

    if (!(pipe = (FILE *)popen(messprintf("%s -cf \"which %s\"", csh, command), "r"))) {
	return 0;
    }

    while (!feof(pipe))
	fgets(retval, 1024, pipe);
    retval[1024] = 0;
    pclose(pipe);

    if ((cp = strchr(retval, '\n')))
      *cp = 0;
    if (retp) *retp = retval;
    
    /* Check if whatever "which" returned is an existing and executable file */
    if (!access(retval, F_OK) && !access(retval, X_OK))
        return 1;
    else
	return 0;
#endif
}


/* Return the foreground color selected for a given
   background color in color_by_conserv mode 
*/
static int bg2fgColor (int bkcol) {

    if (bkcol == maxbgColor)
	return maxfgColor;
    else if (bkcol == midbgColor)
	return midfgColor;
    else if (bkcol == lowbgColor)
	return lowfgColor;

    /* Anything else is either uncolored or markup - make them black */
    return BLACK;
}

static void printColors (void)
{
    static int 
	oldmaxbg, oldmidbg, oldlowbg,
	oldmaxfg, oldmidfg, oldlowfg;

    if (!printColorsOn) {
	oldmaxbg = maxbgColor; maxbgColor = BLACK; 
	oldmaxfg = maxfgColor; maxfgColor = WHITE;

	oldmidbg = midbgColor; midbgColor = GRAY;
	oldmidfg = midfgColor; midfgColor = BLACK;

	oldlowbg = lowbgColor; lowbgColor = LIGHTGRAY;
	oldlowfg = lowfgColor; lowfgColor = BLACK;
	printColorsOn = 1;
    }
    else {
	maxbgColor = oldmaxbg;
	maxfgColor = oldmaxfg;
	midbgColor = oldmidbg;
	midfgColor = oldmidfg;
	lowbgColor = oldlowbg;
	lowfgColor = oldlowfg;
	printColorsOn = 0;
    }
    setConservColors();
    belvuRedraw();
}


static void ignoreGaps (void)
{
    ignoreGapsOn = (ignoreGapsOn ? 0 : 1);
    setConservColors();
    belvuRedraw();
}


/* Not used - the idea was to find balancing point of a tree by walking to
   neighbors and checking if they are better.  However, the best balanced
   node may be beyond less balanced nodes.  (This is because the subtree weights
   are averaged branchlengths and not the sum of the total branchlengths).
*/
static void treeBalanceTreeRecurse(treeNode *node, float *bestbal, treeNode **bestNode)
{
    float newbal, lweight, rweight;

    /* Probe left subtree */

    if (node->left) {
	lweight = treeSize3way(node, node->left) - node->branchlen;
	rweight = treeSize3way(node->left, node) - node->left->branchlen;
	newbal = fabsf(lweight - rweight);
	
	/*
	printf("Subtree weights = %.1f  %.1f\n", lweight, rweight);
	printf("oldbal = %.1f   newbal = %.1f\n", *bestbal, newbal);
	*/

	if (newbal < *bestbal) { /* better balance */

	    printf("Better balance: %.1f (oldbest = %.1f)\n", newbal, *bestbal);
	    
	    *bestbal = newbal;
	    *bestNode = node;
	    
	    treeBalanceTreeRecurse(node->right, bestbal, bestNode);
	}
    }

    if (node->right) {

	lweight = treeSize3way(node, node->right) - node->branchlen;
	rweight = treeSize3way(node->right, node) - node->right->branchlen;
	newbal = fabsf(lweight - rweight);
	
	/*
	printf("Subtree weights = %.1f  %.1f\n", lweight, rweight);
	printf("oldbal = %.1f   newbal = %.1f\n", *bestbal, newbal);
	*/
	
	if (newbal < *bestbal) { /* better balance */
	    
	    printf("Better bal: %f\n", newbal);
	    
	    *bestbal = newbal;
	    *bestNode = node;
	    
	    treeBalanceTreeRecurse(node->right, bestbal, bestNode);
	}
    }
}



#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
/* Calculate the difference between left and right trees if the tree
   were to be rerooted at this node.
*/
static void treeCalcBalanceOld(treeNode *node) {

    float bal, lweight, rweight;

    if (node == treeBestBalancedNode) return;

    if (node->left) {
	lweight = treeSize3way(node, node) /*- node->branchlen*/;
	rweight = treeSize3way(node->left, node) /*- node->left->branchlen*/;
	bal = fabsf(lweight - rweight);

	printf("Left weights = %.1f  %.1f. Bal = %.1f\n", lweight, rweight, bal);

	if (bal < treeBestBalance) { /* better balance */

	    if (debug) printf("%s Found better balance %.1f < %.1f\n", node->left->name, bal, treeBestBalance);
	    treeBestBalance	= bal;
	    treeBestBalancedNode = node;
	}
    }

    if (node->right) {
	lweight = treeSize3way(node, node->right) /*- node->branchlen*/;
	rweight = treeSize3way(node->right, node) /*- node->right->branchlen*/;
	bal = fabsf(lweight - rweight);
	
	printf("Right weights = %.1f  %.1f. Bal = %.1f\n", lweight, rweight, bal);

	if (bal < treeBestBalance) { /* better balance */

	    if (debug) printf("%s Found better balance %.1f < %.1f\n", node->right->name, bal, treeBestBalance);
	    treeBestBalance	= bal;
	    treeBestBalancedNode = node;
	}
    }
}
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */


/* Declare common destroy funcs.... */
myGraphDestroy(belvuDestroy, belvuGraph)

myGraphDestroy(treeDestroy, treeGraph)









