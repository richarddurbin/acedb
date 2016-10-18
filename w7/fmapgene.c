/*  File: fmapgene.c
 *  Author: Richard Durbin (rd@mrc-lmb.cam.ac.uk)
 *  Copyright (C) J Thierry-Mieg and R Durbin, 1992
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
 * Description: Dynamic programming gene finding
 * HISTORY:
 * Last edited: Mar  9 14:23 2009 (edgrif)
 * * Sep 23 16:47 1999 (fw): ditched the use of method->gaFac
 *              this data is now kept locally in this module
 *              in a local cache gaFacs indexed by KEYKEY(meth->key)
 * * Jul 16 14:27 1998 (edgrif): Introduce private header fmap_.h
 * * Jun 12 14:15 1994 (rd): change to select on BEGIN/END to
 		allow refinement of selections in regions
 * * Jun 12 14:15 1994 (rd): added exonScore, move to pg rules
 * * Jun  7 18:27 1994 (rd): split set Parms function into two
 * Created: Tue Aug  4 18:58:53 1992 (rd)
 * CVS info:   $Id: fmapgene.c,v 1.104 2012-10-30 09:55:41 edgrif Exp $
 *-------------------------------------------------------------------
 */

#include <glib.h>
#include <wh/acedb.h>
#include <wh/aceio.h>
#include <whooks/classes.h>
#include <whooks/sysclass.h>
#include <whooks/tags.h>
#include <whooks/systags.h>
#include <wh/gex.h>
#include <wh/lex.h>
#include <wh/session.h>
#include <wh/dbpath.h>
#include <wh/dna.h>
#include <wh/peptide.h>
#include <wh/method.h>
#include <wh/pick.h>		/* for pickMatch() */
#include <wh/main.h>		/* for checkWriteAccess() */
#include <wh/pref.h>
#include <wh/query.h>
#include <wh/bindex.h>
#include <w7/fmap_.h>


/* Required by sMapGetChildParentTag2(), completely busts BS encapsulation, but what to do ?
 * This is exactly how the treedisp and nicedump code work as well. FAR from ideal but I'm.
 * not going to open that can of worms. */
#include <wh/bs_.h>



static void initialise (FeatureMap look);
static void setTempGene(FeatureMap look,
			int gene_class, Array gene_coords,
			int k, int frame, 
			BOOL startNotFound, BOOL endNotFound,
			BOOL isByHand) ;
static KEY  findPeptideMatch(KEY temp,int newchk,KEY key) ;
static int  calculateAndSaveChecksum(KEY protein);
static void fMapDP (void) ;
static void fMapDP1 (void) ; 
static void setSelectedGene(void) ;
static void fixTemp(MENUITEM menu_item) ;
static void selectedGeneToTemp(MENUITEM menu_item) ;
static void killGeneMenu (void) ;
static void showSelect (void) ;
static void addATG (void) ;
static void addSplice3 (void) ;
static void addSplice5 (void) ;
static void fMapHideGfColumns (void) ;
static void fMapShowGfColumns (void) ;
static void parmsEditor(void) ;
static void parmsLoad(void) ;
static void parmsSave(void) ;

static BOOL killGeneInAllClasses(int temp_gene_index) ;
static BOOL killGene(KEY child_key) ;

static KEYSET findAllSmappedGeneClasses(void) ;
static BOOL isSmappedGeneClass(int class) ;
static BOOL isSmappedParentClass(int class) ;
static KEY getSMapParentTag(KEY child_key) ;
static KEY getSMapParent(KEY child_key) ;

static BOOL getPositionTags(int child_class, int parent_class,
			    KEY *tag_in_parent_out, KEY *tag_in_child_out) ;

/* Should go in smap code I think as it is smap specific. */
static BOOL sMapGetChildParentTag2s(int child_class, int parent_class,
				    KEY *child_tag2_out, KEY *parent_tag2_out) ;



/*************************/

/* Keep ELTtype and eltTypeName in step. */
typedef enum {
  DUMMY=0, BEGIN, END, START, STOP, SPLICE_5, SPLICE_3, FRAME
  } ELTtype ;	/* STOP < SPLICE_5 required for correct sort */

static char *eltTypeName[] = {
  "DUMMY", "BEGIN", "END", "START", "STOP", "SPLICE_5", "SPLICE_3", "FRAME" } ;


typedef struct ELTstruct {
  ELTtype type ;
  int	x ;	/* base before splice5, after splice3 */
		/* first base of ATG, last of stop */
  int	frame ;
  float	localScore ;
  float score ; /* DP partial score */
  struct ELTstruct *ptr ;	/* DP back pointer */
  int	flag ;
} ELT ;

#define FLAG_REQUIRED		0x0001
#define FLAG_ANTI_REQUIRED	0x0002



/*********************************************************/

/* The Genefinder parameter tags in the method objs */
static KEY _Gene_assemble_method ;
static KEY _Intron_min, _Exon_min, _Intron_cost, _Inter_gene_cost ;
static KEY _GF_range, _GF_ATG_cutoff, _GF_5_cutoff, _GF_3_cutoff ;

/* The Genefinder methods */
/* hexExon_span is for old style cumulative scoring */
static KEY M_GF_coding_seg, M_GF_ATG, M_GF_splice ;
static KEY M_hexExon, M_hexIntron, M_hexExon_span ;

/* This is a depressing number of globals. */
static Array codeSegs = 0 ;
static FeatureMap gfLook ;

static int gf_range = 10000 ;
static int intronMin = 40 ;
static int intronRateMin = 55 ;
static float intronRate = -1.5 ; /* per log base bp beyond rate min */
static float intronBase = -3.0 ;
static int exonMin = 33 ;
static float interGeneCost = -5.0 ;
float intron3Cutoff = 0.0;
float intron5Cutoff = -0.5;
float atgCutoff = 0.0;
static Array gaFacs = 0; /* of type float
			  * indexed by KEYKEY(methodKey)
			  * we would need one array per
			  * method context, but we have
			  * only one context here for now */


static Graph parmsGraph = GRAPH_NULL ;
static char parmsName[64] = "" ;
static int parmsNameBox ;

static MENUOPT parmsMenu[] =
{ { graphDestroy, "Quit" },
  { graphPrint, "Print" },
  { parmsLoad, "Load" },
  { parmsSave, "Save" },
  { 0, 0 }
} ;

static MENUOPT selectMenu[] = {
  { graphDestroy, "Quit" },
  { graphPrint, "Print" },
  { showSelect, "Update" },
  { 0, 0 }} ;


FREEOPT fMapChooseMenu[] = {
  { 4, "Splice menu" },
  { 1, "Select" },
  { 2, "Antiselect" },
  { 3, "Unselect" },
  { 4, "Delete" }
} ;


/* Grimness: there is only one "params" window for all fmaps on the screen,  */
/* but in order to run some of the options for the params window we need to  */
/* know which fmap was clicked on. This static _should_ hold the id of the   */
/* fmap from which the params window was invoked. Thus we can look at/setup  */
/* the correct method cache etc. for that specific fmap.                     */
static Graph fmap_graph_G = GRAPH_NULL ;



/******************************************************************
 ************************* public functions ***********************
 ******************************************************************/

void fMapChooseMenuFunc (KEY key, int box)
{ 
  int index ;
  SEG *seg ;
  FeatureMap look = currentFeatureMap("fMapChooseMenuFunc") ;

  if (!(index = arr(look->boxIndex, box, int)))
    return ;
  seg = arrp(look->segs, index, SEG) ;

  switch (key)
    {
    case 1:			/* "Select" */
      assRemove (look->antiChosen, SEG_HASH(seg)) ;
      assInsert (look->chosen, SEG_HASH(seg), assVoid(seg->x1)) ;
      graphBoxDraw (box, -1, GREEN) ;
      break ;

    case 2:			/* "Antiselect" */
      assRemove (look->chosen, SEG_HASH(seg)) ;
      assInsert (look->antiChosen, SEG_HASH(seg), assVoid(seg->x1)) ;
      graphBoxDraw (box, -1, LIGHTGREEN) ;
      break ;

    case 3:			/* "Unselect" */
      assRemove (look->chosen, SEG_HASH(seg)) ;
      assRemove (look->antiChosen, SEG_HASH(seg)) ;
      graphBoxDraw (box, -1, WHITE) ;
      break ;

    case 4:			/* "Delete" */
      assRemove(look->chosen, SEG_HASH(seg));
      assRemove(look->antiChosen, SEG_HASH(seg));
      arrayMax(look->segs) = look->lastTrueSeg;
      *seg = arr(look->segs, arrayMax(look->segs)-1, SEG);
      --arrayMax(look->segs);
      arraySort(look->segs, fMapOrder) ;
      look->lastTrueSeg = arrayMax(look->segs);
      fMapDraw(look, 0);
      break;
    }

  return ;
} /* fMapChooseMenuFunc */

/********************/

/* fmap.h - Used by gfcode.c & hexcode.c */
void fMapAddGfSeg (int type, KEY key, int x1, int x2, float score)
{
  SEG *seg = arrayp (gfLook->segs, arrayMax(gfLook->segs), SEG) ;

  fMapInitialise() ;

  seg->parent = 0 ;
  seg->type = type ;
  seg->key = key ;
  seg->x1 = x1 + gfLook->gf.min ;
  seg->x2 = x2 + gfLook->gf.min ;
  seg->data.f = score ;

  return;
} /* fMapAddGfSeg */

/********************/

/* fmap.h - Used by gfcode.c & hexcode.c */
void fMapAddGfSite (int type, int pos, float score, BOOL comp)
{

  fMapInitialise() ;
  
  switch (type)
    {
    case '3':
      fMapAddGfSeg (comp ? SPLICE3_UP : SPLICE3, M_GF_splice,
		    pos-1, pos, score) ;
      break ;
    case '5':
      fMapAddGfSeg (comp ? SPLICE5_UP : SPLICE5, M_GF_splice,
		    comp ? pos-2 : pos,  comp ? pos-1 : pos+1, score) ;
      break ;
    case 'a':
      fMapAddGfSeg (comp ? ATG_UP : ATG, M_GF_ATG,
		    comp ? pos-3 : pos,  comp ? pos-1 : pos+2, score) ;
      break ;
    }

  return;
} /* fMapAddGfSite */

/********************/

void fMapAddGfCodingSeg (int pos1, int pos2, float score, BOOL comp)
     /* fmap.h - Used by gfcode.c & hexcode.c */
{

  fMapInitialise() ;
  
  fMapAddGfSeg (comp ? FEATURE_UP : FEATURE, M_GF_coding_seg, 
		pos1-1, pos2-1, score) ;

  return;
} /* fMapAddGfCodingSeg */

/********************/

/* fmap_.h - function to start genefinder */


void fMapAddGfSegs(FeatureMap look)
{
  fMapAddGfSegsFull(look, TRUE) ;

  return ;
}

void fMapAddGfSegsFull(FeatureMap look, BOOL redraw)
{
  int i, len ;
  SEG *seg ;
  char saved_char ;
  METHOD *meth;

  initialise (look) ;

  /* set the CALCULATED flag - see fmapcontrol.c:addOldSegs() */
  meth = methodCacheGet(look->mcache, M_GF_ATG, "");
  meth->flags |= METHOD_CALCULATED;

  meth = methodCacheGet(look->mcache, M_GF_coding_seg, "");
  meth->flags |= METHOD_CALCULATED;

  meth = methodCacheGet(look->mcache, M_GF_splice, "");
  meth->flags |= METHOD_CALCULATED;

  meth = methodCacheGet(look->mcache, M_hexIntron, "");
  meth->flags |= METHOD_CALCULATED;

  meth = methodCacheGet(look->mcache, M_hexExon, "");
  meth->flags |= METHOD_CALCULATED;

	/* first delete temp segs */
  arrayMax(look->segs) = look->lastTrueSeg ;
	/* then current gf segs */
  for (i = 1 ; i < arrayMax(look->segs) ; ++i)
    {
      seg = arrp(look->segs, i, SEG) ;
      if (seg->key == M_GF_coding_seg ||
	  seg->key == M_GF_ATG ||
	  seg->key == M_GF_splice ||
	  seg->key == M_hexExon ||
	  seg->key == M_hexIntron)
	{
	  *seg = arr(look->segs, arrayMax(look->segs)-1, SEG) ;
	  --arrayMax(look->segs) ; /* RD must do after, for ARR_CHECK */
	  --i ;			/* so test the new item */
	}
    }

  gfLook = look ;

  look->gf.min = look->map->centre - gf_range ;
  if (look->gf.min < 0) 
    look->gf.min = 0 ;
  if (look->gf.min > look->zoneMin)
    look->gf.min = look->zoneMin ;

  look->gf.max = look->map->centre + gf_range ;
  if (look->gf.max > arrayMax(look->dna)) 
    look->gf.max = arrayMax(look->dna) ;
  if (look->gf.max < look->zoneMax)
    look->gf.max = look->zoneMax ;

  len = look->gf.max - look->gf.min ;

  if (look->gf.cum) messfree (look->gf.cum) ;
  look->gf.cum = (float*) halloc (sizeof(float)*(len+1), graphHandle()) ;
  if (look->gf.revCum) messfree (look->gf.revCum) ;
  look->gf.revCum = (float*) halloc (sizeof(float)*(len+1), graphHandle()) ;

  /* ensure 0 termination is safe */
  array(look->dna, arrayMax(look->dna), char) = 0 ;
  --arrayMax(look->dna) ;

  /* geneFinderAce calls an internal routine that assumes that the dna 
   * array is a normal C string, so this horrible code inserts a null to
   * make it so and then restores the char at the end. This code should
   * be encapsulated in geneFinderAce and we should pass in the gf.max to 
   * geneFinderAce as a parameter. Soemthing for another time..... */
  if (look->gf.max < arrayMax(look->dna))
    {
      saved_char = arr(look->dna, look->gf.max, char) ;
      arr(look->dna, look->gf.max, char) = 0 ;
    }

  if (geneFinderAce(arrp(look->dna,look->gf.min,char), &look->gf))
    {

      hexAddSegs ("intron", FEATURE, M_hexIntron, 1, 10, FALSE,
		  arrp(look->dna,look->gf.min,char), len, 0) ;
      hexAddSegs ("intron", FEATURE_UP, M_hexIntron, 1, 10, TRUE,
		  arrp(look->dna,look->gf.min,char), len, 0) ;

      /* exon must come second, to fill cum properly */
      hexAddSegs ("cds", FEATURE, M_hexExon, 3, 10, FALSE,
		  arrp(look->dna,look->gf.min,char), len, look->gf.cum) ;
      hexAddSegs ("cds", FEATURE_UP, M_hexExon, 3, 10, TRUE,
		  arrp(look->dna,look->gf.min,char), len, look->gf.revCum) ;

      arraySort (look->segs, fMapOrder) ;
      look->lastTrueSeg = arrayMax(look->segs) ;

      fMapProcessMethods (look) ;

      if (redraw)
	fMapShowGfColumns() ;				    /* also does fMapDraw() */
      else
	{	
	  int offset, i ;
	  SEG *seg ;

	  /* We did not draw so we need to set correct offset for features.... */

	  offset = look->start ;

	  for (i = 1 ; i < arrayMax(look->segs) ; ++i)
	    {
	      seg = arrp(look->segs, i, SEG) ;
	      switch (seg->type)
		{
		case ATG: case ATG_UP:
		case FEATURE: case FEATURE_UP:
		case SPLICE5: case SPLICE5_UP:
		case SPLICE3: case SPLICE3_UP:
		  {
		    METHOD *meth;

		    if (class(seg->key) != _VMethod)
		      messcrash ("Non-method key in addOldSegs") ;

		    meth = methodCacheGet(look->mcache, seg->key, "");
		    if (meth && meth->flags & METHOD_CALCULATED)
		      { 
			seg->x1 += offset ;  
			seg->x2 += offset ;
		      }

		  break ;
		  }
		default:		/* needed for picky compiler */
		  break ;
		}
	    }
	}
    }

  /* Vital...restore the null'd dna char. */
  if (look->gf.max < arrayMax(look->dna))
    arr(look->dna, look->gf.max, char) = saved_char ;

  return ;
} /* fMapAddGfSegs */


/********************/

/* Construct the Gene Finder tools menu, must be done dynamically now as we need to make
 * submenus of all SMap'able gene classes for creating temp genes.
 * (note I originally added the list to the "fix" menu item as well but have removed this
 * for now)
 * 
 * we dynamically construct the menu, it should be deleted somehow, wonder how this
 * happens with menus.....??????????????? i.e. it should happen when the fmap is destroyed. */
MENU fmapGetGeneOptsMenu(void)
{
  MENU menu ;
  KEYSET gene_classes ;
  MENU gene_classes_menu ;
  int i ;
  char *menu_name = "Gene Finder Tool" ;
  MENUOPT fMapGeneOpts[] = {
    {fMapMenuAddGfSegs, "Genefinder Features"},
    {fMapDP1, "Autofind one gene"},
    {fMapDP, "Autofind genes"},
    {parmsEditor, "Parameters"},
    {setSelectedGene, "Gene -> Selected"},
    {(MenuBaseFunction)selectedGeneToTemp, "Selected -> temp_gene"},
							    /* warning, funcptr used in test below. */
    {(MenuBaseFunction)fixTemp, "Fix temp_gene"},    /* warning, funcptr used in test below. */
    {killGeneMenu, "Remove Gene"},
    {showSelect, "Show Selected"},
    {addATG, "New Gene Start (ATG)"},
    {addSplice3, "New Exon Start"},
    {addSplice5, "New Exon End"},
    {fMapHideGfColumns, "Hide Gf columns"},
    {fMapShowGfColumns, "Show Gf columns"},
    {NULL, NULL}} ;
  MENUOPT *gene_item ;
  MENUITEM menu_item ;

  /* Get a list of all the classes that contain the tags for smap'd gene like objects. */
  messCheck(gene_classes = findAllSmappedGeneClasses(), == NULL,
	    "no SMap'able gene classes in database models") ;

  /* Construct the gene finder menu. */
  menu = menuCreate(menu_name) ;
  gene_item = fMapGeneOpts ;
  while (gene_item->f != NULL)
    {
      /* For the temp gene selection create a submenu giving a choice of classes,
       * for all other options just do the standard menu. */
      if (gene_item->f == (MenuBaseFunction)selectedGeneToTemp)
	{
	  gene_classes_menu = menuCreate(NULL) ;
	  for (i = 0 ; i < keySetMax(gene_classes) ; i++)
	    {
	      int gene_class = keySet(gene_classes, i) ;

	      menu_item = menuCreateItem(pickClass2Word(gene_class), (MENUFUNCTION)gene_item->f) ;
	      menuAddItem(gene_classes_menu, menu_item, NULL) ;
	      menuSetValue(menu_item, gene_class) ;
	    }

	  menu_item = menuCreateItem(gene_item->text, NULL) ;

	  if (!menuSetMenu(menu_item, gene_classes_menu)
	      || !menuAddItem(menu, menu_item, NULL))
	    messcrash("could not create menu item for gene finder menu, "
		      "item was: %s", gene_item->text) ;
	}
      else
	{
	  menu_item = menuCreateItem(gene_item->text, (MENUFUNCTION)gene_item->f) ;
	  menuAddItem(menu, menu_item, NULL);
	}

      gene_item++ ;
    }

  return menu ;
}




/******************************************************************
 ************************ private functions ***********************
 ******************************************************************/



/* There is a problem here....if the ?Method class definition in models.wrm  */
/* does not contain any of the tags hard-coded below then method.c will      */
/* crash. We don't have a mechanism for ensuring that this code and ?Method  */
/* definition are kept in step...uuuggghhh.                                  */
/*                                                                           */
static void initialise(FeatureMap look)
{ 
  static BOOL isGlobalDone = FALSE ;

  if (!look->isGFMethodsInitialised) 
    {
      /* genefinder methods - init once per FMAP-look */
      /* we keep the KEYs of those methods in static vars */
      M_GF_ATG = 
	methodCacheGetByName (look->mcache, "GF_ATG",
			      "Colour ORANGE\n"
			      "Frame_sensitive\n"
			      "Strand_sensitive\n"
			      "Score_by_width\n"
			      "Score_bounds   0  3\n"
			      "Right_priority 4.2\n"
			      ,"")->key;
      
      M_GF_splice =
	methodCacheGetByName (look->mcache, "GF_splice",
			      "Strand_sensitive\n"
			      "Score_bounds   -1  4\n"
			      "Width 10\n"
			      "Right_priority 5.6\n"
			      ,"")->key;
      
      M_GF_coding_seg =
	methodCacheGetByName (look->mcache, "GF_coding_seg",
			      "Colour LIGHTGRAY\n"
			      "Frame_sensitive\n"
			      "Strand_sensitive\n"
			      "Score_by_width\n"
			      "Score_bounds   2  8\n"
			      "Width 2\n"
			      "Right_priority 4.3\n"
			      "Symbol \"C\"\n"
			      ,"")->key;
      
      M_hexExon =
	methodCacheGetByName (look->mcache, "hexExon",
			      "Colour ORANGE\n"
			      "Frame_sensitive\n"
			      "Strand_sensitive\n"
			      "Score_by_width\n"
			      "Score_bounds   10  50\n"
			      "Width 2\n"
			      "Right_priority 4.6\n"
			      "Symbol \"H\"\n"
			      ,"")->key;
      
      M_hexExon_span =
	methodCacheGetByName (look->mcache, "hexExon_span",
			      "" /* no defaults needed - 
				  * empty object is sufficient */
			      ,"")->key;
      
      M_hexIntron =
	methodCacheGetByName (look->mcache, "hexIntron",
			      "Colour BROWN\n"
			      "Strand_sensitive\n"
			      "Score_by_width\n"
			      "Score_bounds   10  40\n"
			      "Width 2\n"
			      "Right_priority 5.1\n"
			      "Symbol \"I\"\n"
			      ,"")->key;
      
      look->isGFMethodsInitialised = TRUE;
    }
  
  if (!isGlobalDone) 
    {
      /* tags - init only once per application process */
      lexaddkey ("Intron_min", &_Intron_min, 0) ;
      lexaddkey ("Exon_min", &_Exon_min, 0) ;
      lexaddkey ("Intron_cost", &_Intron_cost, 0) ;
      lexaddkey ("Inter_gene_cost", &_Inter_gene_cost, 0) ;
      lexaddkey ("GF_range", &_GF_range, 0) ;
      lexaddkey ("GF_ATG_cutoff", &_GF_ATG_cutoff, 0) ;
      lexaddkey ("GF_5_cutoff", &_GF_5_cutoff, 0) ;
      lexaddkey ("GF_3_cutoff", &_GF_3_cutoff, 0) ;
      lexaddkey ("Gene_assemble_method", &_Gene_assemble_method, 0) ;

      /* default methods for gene assembly */
      gaFacs = arrayCreate (lexMax(_VMethod), float);
      array(gaFacs, KEYKEY(M_GF_ATG), float) = 1.0;
      array(gaFacs, KEYKEY(M_GF_splice), float) = 1.0;
      array(gaFacs, KEYKEY(M_hexExon_span), float) = 1.0;

      isGlobalDone = TRUE ;
    }


  return;
} /* initialise */


/********************************************************/
/********** intron and exon scoring routines ************/

static float interGeneScore (ELT *elt1, ELT *elt2)
{
  return interGeneCost ;
}   

static float intronScore (ELT *elt1, ELT *elt2)
{
  int length = elt2->x - elt1->x - 2 ; /* -2 because ->x are exon endpoints */

  if (length < intronMin && 
      elt1->type != BEGIN && elt2->type != END)
    return -1000.0 ;
  else if (length <= intronRateMin)
    return intronBase ;
  else
    return intronBase + intronRate*log10(length - intronRateMin) ;
}

static float exonScore (FeatureMap look, ELT *elt1, ELT *elt2, 
			int frame, GFINFO *gf)
{
  int i, x1 = elt1->x, x2 = elt2->x, s1, s2 ;
  float score = 0, bit ;
  KEY methodKey ;
  SEG *seg ;
  METHOD *meth ;
  HOMOLINFO *hinf ;

  if (elt2->type == STOP) 
    x2 -= 2 ; /* added 2 so sorted correctly with splices */

  if (x2-x1+1 < exonMin &&
      elt1->type != BEGIN && elt1->type != START &&
      elt2->type != END && elt2->type != STOP)
    return -1000.0 ;

  while (x1 % 3 != frame) 
    ++x1 ;			/* so start in frame */
  while (x2 % 3 != frame) 
    --x2 ;			/* so end in frame */

  if (arrayExists(codeSegs))
    for (i = 0 ; i < arrayMax(codeSegs) ; ++i)
      {
	seg = arr(codeSegs,i,SEG*) ;
	if (seg->x1 >= x2 || seg->x2 <= x1)
	  continue ;
	if (seg->type == HOMOL || seg->type == HOMOL_UP)
	  { 
	    hinf = arrp(look->homolInfo, seg->data.i, HOMOLINFO) ;
	    methodKey = hinf->method ;
	    bit = hinf->score ;
	  }
	else if (seg->type == FEATURE || seg->type == FEATURE_UP)
	  { 
	    methodKey = seg->key ;
	    bit = seg->data.f ;
	  }
	meth = methodCacheGet(look->mcache, methodKey, "") ;
	if (!meth)
	  continue;

	if (!array(gaFacs, KEYKEY(meth->key), float))
	  continue ;
	if ((meth->flags & METHOD_FRAME_SENSITIVE)
	    && seg->x1 %3 != frame)
	  continue ;
	s1 = seg->x1 > x1 ? seg->x1 : x1 ;
	s2 = seg->x2 < x2 ? seg->x2 : x2 ;
	score += bit * array(gaFacs, KEYKEY(meth->key), float) *
	  (s2 - s1 + 1) / (float) (seg->x2 - seg->x1 + 1) ;
    }

  /* hack for oldfashioned approach */
  meth = methodCacheGet (look->mcache, M_hexExon_span, "");
  if (meth && array(gaFacs, KEYKEY(meth->key), float)) /* mhmp 08.09.98 */
    {
      x1 += 3 ;
      x2 -= 3 ;
      if (x1 < x2)
	score += array(gaFacs, KEYKEY(meth->key), float) * 
	  (gf->cum[x2 - gf->min] - gf->cum[x1 - gf->min]) ;
    }

#ifdef DEBUG
  printf (" <%.2f>", score) ;
#endif /* DEBUG */

  return score ;
} /* exonScore */


#ifdef OLD_EXON_SCORE
  float min, max = 0, *cum = 0 ;

  cum = &gf->cum[x1 - gf->min] ;
  min = *cum ; cum += 3 ;
  for (i = x1 + 3 ; i <= x2 ; i += 3, cum += 3)
    { if (*cum < min) 
	min = *cum ;
      else if (*cum - min > max)
	max = *cum - min ;
    }

#ifdef COMPENSATE_FOR_LENGTH
  max -= log10(x2-x1+1) ;
#endif /* COMPENSATE_FOR_LENGTH */

  if (max > 0)
    return max * codingCost ;
  else
    return 0 ;
}

static void flattenGfCum (LOOK look)
{
  int i ;
  char *cp = arrp(look->dna, look->gf.min, char) ;
  float max[3] ;
  Array tt ;

if (!(tt = pepGetTranslationTable (look->seqKey, 0)))
     return ;

  for (i = 0 ; i < 3 ; ++i)
    max[i] = -1.0e20 ;

  for (i = look->gf.min ; i < look->gf.max ; ++i, ++cp)
    if (e_codon(cp, tt) == '*')
      max[i%3] = -1.0e20 ;
    else if (look->gf.cum[i] < max[i%3])
      look->gf.cum[i] = max[i%3] ;
    else
      max[i%3] = look->gf.cum[i] ;
}
#endif /* OLD_EXON_SCORE */

/********************** set parameters ***********************/

static void parmsLoadName (char *s, FeatureMap look, BOOL makeParms)
{ 
  KEY key ;
  OBJ obj ;
  BSMARK mark = 0 ;
  float x ;
  int i;
  
  /* no need to initialise - only called from functions
   * where this has already been done */

  if (!lexword2key (s, &key, _VMethod))
    { messout ("Can't find Method object %s", parmsName) ;
      return ;
    }
  strcpy (parmsName, name(key)) ;
  if (!(obj = bsCreate (key)))
    { messout ("Failed to open Method object %s", name(key)) ;
      return ;
    }

  bsGetData (obj, _Intron_min, _Int, &intronMin) ;
  bsGetData (obj, _Exon_min, _Int, &exonMin) ;
  x = bsGetData (obj, _Intron_cost, _Float, &intronBase) &&
    bsGetData (obj, _bsRight, _Float, &intronRate) &&
    bsGetData (obj, _bsRight, _Int, &intronRateMin) ;
  bsGetData (obj, _Inter_gene_cost, _Float, &interGeneCost) ;

  bsGetData (obj, _GF_range, _Int, &gf_range) ;
  bsGetData (obj, _GF_ATG_cutoff, _Float, &atgCutoff) ;
  bsGetData (obj, _GF_5_cutoff, _Float, &intron5Cutoff) ;
  bsGetData (obj, _GF_3_cutoff, _Float, &intron3Cutoff) ;

  {				/* check displayable */
    float rp = 2.0 ;
    if (!bsGetData (obj, str2tag("Right_priority"), _Float, 0))
      bsAddData (obj, str2tag("Right_priority"), _Float, &rp) ;
    if (!bsGetKeyTags (obj, str2tag("Colour"), 0))
      if (bsAddTag (obj, str2tag("Colour")) && bsPushObj (obj))
	{ bsAddTag (obj, _BLUE) ;
	  bsGoto (obj, 0) ;
	}
  }

  /* clear all gene-assembly-factors */
  for (i = 0; i < arrayMax(gaFacs); i++)
    array(gaFacs, i, float) = 0;


  if (bsGetKey (obj, _Gene_assemble_method, &key))
    do
      {
	mark = bsMark (obj, mark) ;
	if (bsGetData (obj, _bsRight, _Float, &x))
	  { 
	    /* make sure the method is in the cache */
	    methodCacheGet (look->mcache, key, "");
	    array(gaFacs, KEYKEY(key), float) = x;
	  }
	bsGoto (obj, mark) ;
      } while (bsGetKey (obj, _bsDown, &key)) ;
  bsMarkFree (mark) ;

  bsDestroy (obj) ;

  if (parmsGraph && makeParms)
    parmsEditor () ;

  return;
} /* parmsLoadName */

static void parmsLoad (void)
{ 
  FeatureMap look = NULL ;
  ACEIN name_in;

  /* We need to operate on the fmap that the user clicked on.                */
  graphActivate(fmap_graph_G) ;
  look = currentFeatureMap("parmsLoad") ;
  initialise (look);

  if ((name_in = messPrompt ("Give name of method object to load from",
			     parmsName, "wz", 0)))
    {
      parmsLoadName (aceInWord(name_in), look, TRUE);
      aceInDestroy (name_in);
    }

  return;
} /* parmsLoad */

static void parmsSave (void)
{
  KEY key ;
  OBJ obj ;
  int i ;
  float f;
  FeatureMap look = NULL ;
  ACEIN name_in;

  /* We need to operate on the fmap that the user clicked on.                */
  graphActivate(fmap_graph_G) ;
  look = currentFeatureMap("parmsSave");
  initialise (look) ;


  if (!checkWriteAccess())
    /* need to establish write access, otherwise the changes stay
     * in the object in secondary chache but will never make their
     * way into the object on disk. */
    return;

  name_in = messPrompt ("Give name of method object to save into:",
			parmsName, "wz", 0);
  if (!name_in)
    return ;
  strncpy (parmsName, aceInWord(name_in), 63); parmsName[63] = '\0';
  aceInDestroy(name_in);

  if (strlen(parmsName) == 0)
    { 
      messout ("You must give a name") ;
      return ;
    }

  if (!graphCheckEditors (parmsGraph, TRUE))
    return ;
  if (!lexaddkey (parmsName, &key, _VMethod) &&
      !messQuery ("Overwrite method %s?", name(key)))
    return ;

  if (!(obj = bsUpdate (key)))
    { messout ("Failed to edit object Method:%s", name(key)) ;
      return ;
    }

  bsAddData (obj, _Intron_min, _Int, &intronMin) ;
  bsAddData (obj, _Exon_min, _Int, &exonMin) ;
  bsAddData (obj, _Intron_cost, _Float, &intronBase) ;
  bsAddData (obj, _bsRight, _Float, &intronRate) ;
  bsAddData (obj, _bsRight, _Int, &intronRateMin) ;
  bsAddData (obj, _Inter_gene_cost, _Float, &interGeneCost) ;

  bsAddData (obj, _GF_range, _Int, &gf_range) ;
  bsAddData (obj, _GF_ATG_cutoff, _Float, &atgCutoff) ;
  bsAddData (obj, _GF_5_cutoff, _Float, &intron5Cutoff) ;
  bsAddData (obj, _GF_3_cutoff, _Float, &intron3Cutoff) ;

  {				/* check displayable */
    float rp = 2.0 ;
    if (!bsGetData (obj, str2tag("Right_priority"), _Float, 0))
      bsAddData (obj, str2tag("Right_priority"), _Float, &rp) ;
    if (!bsGetKeyTags (obj, str2tag("Colour"), 0))
      if (bsAddTag (obj, str2tag("Colour")) && bsPushObj (obj))
	{ bsAddTag (obj, _BLUE) ;
	  bsGoto (obj, 0) ;
	}
  }

  if (bsFindTag (obj, _Gene_assemble_method))
    bsPrune (obj) ;

  /* write out all the gene-assembly-factors
   * for each factor that we have for a method write
   * the key of the corresponding method and the factor */
  for (i = 0 ; i < arrayMax(gaFacs) ; ++i)
    {
      f = arr(gaFacs, i, float);
      
      if (f)
	if (bsAddKey (obj, _Gene_assemble_method, KEYMAKE(_VMethod, i)))
	  bsAddData (obj, _bsRight, _Float, &f) ;
    }

  bsSave (obj) ;

  if (graphActivate (parmsGraph))
    graphBoxDraw (parmsNameBox, -1, -1) ;

  return;
} /* parmsSave */


static void newMethodEntryCallBack (char *new_name)
{ 
  KEY key ;
  float x ;
  FeatureMap look = currentFeatureMap("newMethodEntryCallBack") ;
  ACEIN fac_in;

  if (!(fac_in = messPrompt
	(messprintf ("Give score factor for method %s", new_name), 
	 "0", "fz", 0)))
    return;

  aceInFloat (fac_in, &x) ;
  aceInDestroy (fac_in);

 /* add the method into the cache - the lexaddkey ensires that
  * we have at least an empty object in the database, so soemthing
  * will get added to the cache */
  lexaddkey (new_name, &key, _VMethod) ; 
  methodCacheGet (look->mcache, key, "");

  array(gaFacs, KEYKEY(key), float) = x;
  parmsEditor () ;

  return;
} /* newMethodEntryCallBack */


/* menu/button func */
static void parmsEditor (void)
{ 
  static char newMethodName[64] ;
  int line = 0 ;
  int i ;
  KEY key ;
  float f;
  FeatureMap look = NULL ;
  KEY methodKey;

  /* We need to operate on the fmap that the user clicked on, so need to     */
  /* record which one it was.                                                */
  look = currentFeatureMap("parmsEditor");
  fmap_graph_G = graphActive() ;			    /* MUST be an fmap because of
							       preceding call. */
  initialise (look) ;

  if (graphActivate (parmsGraph))
    { 
      graphPop () ;
      graphClear () ;
    }
  else
    { 
      parmsGraph = graphCreate (TEXT_SCROLL, "Gene assembly parameters", 
				0, 0, 0.4, 0.4) ;
      graphMenu (parmsMenu) ;
    }

  if (!*parmsName)
    if (lexword2key ("assembly-default", &key, _VMethod) && iskey(key) == 2)
      parmsLoadName ("assembly-default", look, FALSE);

  parmsNameBox = graphBoxStart () ;
  graphText ("Name:", 1, line) ;
  graphTextPtr (parmsName, 7, line++, 32) ;
  graphBoxEnd () ;
  ++line ;
  graphText ("Assembly parameters", 1, line++) ;
  graphIntEditor ("Min intron length", &intronMin, 2, line++, 0) ;
  graphIntEditor ("Min exon length", &exonMin, 2, line++, 0) ;
  graphFloatEditor ("Intron base cost", &intronBase, 2, line++, 0) ;
  graphFloatEditor ("Intron rate", &intronRate, 2, line++, 0) ;
  graphIntEditor ("Intron rate min", &intronRateMin, 2, line++, 0) ;
  graphFloatEditor ("Inter-gene cost", &interGeneCost, 2, line++, 0) ;
  ++line ;
  graphText ("Assembly methods", 1, line++) ;

  for (i = 0; i < arrayMax(gaFacs); ++i)
    {
      f = arr(gaFacs, i, float);
      
      if (!f)
	continue;

      methodKey = KEYMAKE(_VMethod, i);

      if ((methodCacheGet(look->mcache, methodKey, "")))
	graphFloatEditor (name(methodKey),
			  (arrp(gaFacs, i, float)),
			  2, line++, 0) ;
    }
  
  *newMethodName = 0 ;
  graphTextScrollEntry (newMethodName, 63, 20, 2, line++, newMethodEntryCallBack) ;
  ++line ;
  graphText ("Genefinder parameters", 1, line++) ;
  graphIntEditor ("Features range (bp)", &gf_range, 2, line++, 0) ;
  graphFloatEditor ("3-splice cutoff", &intron3Cutoff, 2, line++, 0) ;
  graphFloatEditor ("5-splice cutoff", &intron5Cutoff, 2, line++, 0) ;
  graphFloatEditor ("ATG cutoff", &atgCutoff, 2, line++, 0) ;
  ++line ;
  graphText ("Save as \"assembly-default\" to set the default",
	     1, line++) ;

  graphRedraw () ;

  graphActivate(fmap_graph_G) ;				    /* Make sure fmap user clicked on is
							       the active one. */

  return;
} /* parmsEditor */

/********************/

static int eltCompare (void *x, void *y)
{
  int diff = ((ELT*)x)->x - ((ELT*)y)->x ;

  if (diff)
    return diff ;
  else
    return ((ELT*)x)->type - ((ELT*)y)->type ;
}


/********************/

/* menu/button func "Autofind genes" */
static void fMapDP (void)
{
  static Array zombie[3] ;	/* active elts in intron */
  static Array alive[3] ;	/* active elts in coding frame */
  static Array elts ;		/* complete set */
  static Array gene ;
  ELT *elt, *elt2, *dead, end, *best ; /* best needed for one-gene version */
  int i, j, jKeep, frame, zframe ;
  int min, max, endFrame ;
  float score, scoreKeep, fac ;
  SEG *seg ;
  HOMOLINFO *hinf ;
  METHOD *meth ;
  char *cp ;
  Array tt = 0 ;
  FeatureMap look = currentFeatureMap("fMapDP") ;

  initialise (look);

  if (!*parmsName)		/* load default parameters */
    {
      KEY key ;
      if (lexword2key ("assembly-default", &key, _VMethod) && iskey(key) == 2)
	parmsLoadName ("assembly-default", look, TRUE) ;
    }
  else if (parmsGraph)
    graphCheckEditors (parmsGraph, TRUE) ;

  if (!look->gf.cum)
    { 
      fMapAddGfSegs(look) ;
      if (!look->gf.cum)
	messout ("I cannot locate the genefinder tables, sorry.  "
		 "Either put them in $ACEDB/wgf, or set the environment "
		 "variable GF_TABLES.") ;
      return ;
    }
  

  if (!(tt = pepGetTranslationTable (look->seqKey, 0)))
    return ;


  /* find working zone */

  /* simulate return in zone entry box to ensure zone coords up to date */
  fMapSetZoneEntry (look->zoneBuf) ; 
  min = look->zoneMin ;
  max = look->zoneMax + 1 ;
  if (look->gf.min > min)
    min = look->gf.min ;
  if (look->gf.max < max)
    max = look->gf.max ;
  if (max < min)
    {
      messout ("Available range is (%d,%d) is negative", min, max) ;
      return ;
    }


  /* make elts Array of features for dynamic programming */

  elts = arrayReCreate (elts, 1024, ELT) ;
  codeSegs = arrayReCreate (codeSegs, 1024, SEG*) ;
				/* elts from SEGs: SPLICES + STARTS */
  for (i = 1 ; i < arrayMax(look->segs) ; ++i)
    {
      seg = arrp(look->segs, i, SEG) ;

      if (seg->x1 < min ||
	  seg->x2 >= max ||
	  assFind (look->antiChosen, SEG_HASH(seg), 0))
	continue ;

      switch (seg->type)
	{
	case ATG:
	  meth = methodCacheGet(look->mcache, seg->key, "") ;
	  if ((fac = array(gaFacs, KEYKEY(meth->key), float)))
	    {
	      elt = arrayp(elts, arrayMax(elts), ELT) ;
	      elt->type = START ;
	      elt->x = seg->x1 ;
	      elt->frame = seg->x1 % 3 ;
	      elt->localScore = seg->data.f * fac ;
	    }
	  break ;
	case SPLICE5:	/* need 3 - indexed by frame */
	  meth = methodCacheGet(look->mcache, seg->key, "") ;
	  if ((fac = array(gaFacs, KEYKEY(meth->key), float)))
	    for (frame = 0 ; frame < 3 ; ++frame)
	      {
		elt = arrayp(elts, arrayMax(elts), ELT) ;
		elt->type = SPLICE_5 ;
		elt->x = seg->x1 ;
		elt->localScore = seg->data.f * fac ;
		elt->frame = frame ;
	      }
	  break ;
	case SPLICE3:	/* 3 indexed by zframe */
	  meth = methodCacheGet(look->mcache, seg->key, "") ;
	  if ((fac = array(gaFacs, KEYKEY(meth->key), float)))
	    for (zframe = 0 ; zframe < 3 ; ++zframe)
	      {
		elt = arrayp(elts, arrayMax(elts), ELT) ;
		elt->type = SPLICE_3 ;
		elt->x = seg->x2 ;
		elt->localScore = seg->data.f * fac ;
		elt->frame = zframe ;
	      }
	  break ;
	case FEATURE:
	  meth = methodCacheGet(look->mcache, seg->key, "") ;
	  if (array(gaFacs, KEYKEY(meth->key), float) && seg->data.f)
	    array(codeSegs, arrayMax(codeSegs), SEG*) = seg ;
	  break ;
	case HOMOL:
	  hinf = arrp(look->homolInfo, seg->data.i, HOMOLINFO) ;
	  meth = methodCacheGet(look->mcache, hinf->method, "") ;
	  if (array(gaFacs, KEYKEY(meth->key), float) && hinf->score)
	    array(codeSegs, arrayMax(codeSegs), SEG*) = seg ;
	  break ;
	default:
	  break ;
	}
    }

  /* all STOPs */
  cp = arrp (look->dna, min, char) ;
  for (i = min ; i <= max-3 ; ++i)
    if (e_codon(cp++, tt) == '*')
      {
	elt = arrayp(elts, arrayMax(elts), ELT) ;
	elt->type = STOP ;
	elt->x = i+2 ;
	elt->frame = i % 3 ;
      }
  
  arraySort (elts, eltCompare) ;

  /* do the dynamic programming - first initialise to BEGIN */
  { ELT begin ;

    begin.type = BEGIN ; 
    begin.x = min ; 
    begin.ptr = 0 ; 
    begin.score = begin.localScore = 0 ;
    
    for (frame = 0 ; frame < 3 ; ++frame)
      { alive[frame] = arrayReCreate (alive[frame], 32, ELT*) ;
	array (alive[frame], 0, ELT*) = (ELT*) messalloc(sizeof(ELT)) ;
	*arr(alive[frame], 0, ELT*) = begin ;
	arr(alive[frame], 0, ELT*)->frame = frame ;
      }
    for (zframe = 0 ; zframe < 3 ; ++zframe)
      { ELT *myelt ; /* I need to allocate *myelt, otherwise some SGI compilo crash */
        zombie[zframe] = arrayReCreate (zombie[zframe], 32, ELT*) ;
	array (zombie[zframe], 0, ELT*) = myelt = 
	  (ELT*) messalloc(sizeof(ELT)) ;
	*myelt = begin ;  /* here myelt is absolutely needed */
	myelt->frame = zframe ;
      }
    dead = (ELT*) messalloc(sizeof(ELT)) ;
    *dead = begin ;
    dead->score = -interGeneCost ;
    best = 0 ;
  }

  for (i = 0 ; i < arrayMax(elts) ; ++i)
    { elt = arrp(elts, i, ELT) ;
#ifdef DEBUG
      printf ("%d_%d %4.1f %s ", COORD(look,elt->x), elt->frame, 
	      elt->localScore, eltTypeName[elt->type]) ;
#endif /* DEBUG */
      switch (elt->type)
	{
	case START:
	  { ELT *wasDead = dead ;
	    if (dead)
	      { elt->score = dead->score + elt->localScore + 
		  interGeneScore (dead, elt) ;
		if (dead->type != BEGIN)
		  elt->ptr = dead ;
	      }
	    if (assFind (look->chosen, HASH(ATG, elt->x + 2), 0))
	      { for (j = 0 ; j < 3 ; ++j)
		  { arrayMax(alive[j]) = 0 ;
		    arrayMax(zombie[j]) = 0 ;
		  }
		dead = 0 ;
	      }
	    if (wasDead)
	      array (alive[elt->frame], 
		     arrayMax(alive[elt->frame]), ELT*) = elt ;
	  }
	  break ;
	case STOP:
	  if (assFind (look->chosen, HASH(ORF, elt->x - 3), 0))
	    dead = best = 0 ;		/* to force choice of this frame */
	  if (!assFind (look->antiChosen, HASH(ORF, elt->x - 3), 0))
	    for (j = 0 ; j < arrayMax(alive[elt->frame]) ; ++j)
	      { elt2 = arr(alive[elt->frame], j, ELT*) ;
		score = elt2->score + 
		  exonScore (look, elt2, elt, elt->frame, &look->gf) ;
#ifdef DEBUG
	printf (" (%d_%d %4.1f)", COORD(look,elt2->x), elt2->frame, score) ;
#endif /* DEBUG */
		if (!dead || score > dead->score)
		  { elt->score = score ;
		    elt->ptr = elt2 ;
		    dead = elt ;
		  }
		if (!best || score > best->score)
		  { elt->score = score ;
		    elt->ptr = elt2 ;
		    best = elt ;
		  }
	      }
	  if (assFind (look->chosen, HASH(ORF, elt->x - 3), 0))
	    for (j = 0 ; j < 3 ; ++j) /* kill all except dead */
	      { arrayMax(alive[j]) = 0 ;
		arrayMax(zombie[j]) = 0 ;
	      }
	  else
	    arrayMax(alive[elt->frame]) = 0 ;
	  break ;
	case SPLICE_5:
	  zframe = (elt->x + 4 - elt->frame) % 3 ;
	  for (j = 0 ; j < arrayMax(alive[elt->frame]) ; ++j)
	    { elt2 = arr(alive[elt->frame], j, ELT*) ;
	      score = elt2->score + elt->localScore + 
		exonScore (look, elt2, elt, elt->frame, &look->gf) ;
#ifdef DEBUG
	printf (" (%d_%d %4.1f)", COORD(look,elt2->x), elt2->frame, score) ;
#endif /* DEBUG */
	      if (!elt->ptr || score > elt->score)
		{ elt->score = score ;
		  elt->ptr = elt2 ;
		}
	    }
	  if (assFind (look->chosen, HASH(SPLICE5, elt->x+1), 0))
	    { for (j = 0 ; j < 3 ; ++j)
	        { arrayMax(alive[j]) = 0 ;
		  arrayMax(zombie[j]) = 0 ;
		}
	      dead = 0 ;
	    }
	  if (elt->ptr)		/* i.e. found something */
	    array (zombie[zframe], 
		   arrayMax(zombie[zframe]), ELT*) = elt ;
	  break ;
	case SPLICE_3:
	  frame = (3 - elt->frame + elt->x) % 3 ;
	  scoreKeep = -1000000.0 ;
	  jKeep = 0 ;

	  for (j = 0 ; j < arrayMax(zombie[elt->frame]) ; ++j)
	    { elt2 = arr(zombie[elt->frame], j, ELT*) ;
	      score = elt2->score + elt->localScore + 
		intronScore(elt2, elt) ;
#ifdef DEBUG
	printf (" (%d_%d %4.1f)", COORD(look,elt2->x), elt2->frame, score) ;
#endif /* DEBUG */
	      if (!elt->ptr || score > elt->score)
		{ elt->ptr = elt2 ;
		  elt->score = score ;
		}
				/* compress zombie[elt->frame] */
	      if (score > scoreKeep ||
		  elt->x - elt2->x < intronMin) /* keep */
		{ arr(zombie[elt->frame], jKeep++, ELT*) = elt2 ;
		  scoreKeep = score ;
		}
	    }
	  if (assFind (look->chosen, HASH(SPLICE3, elt->x), 0))
	    { for (j = 0 ; j < 3 ; ++j)
	        { arrayMax(alive[j]) = 0 ;
		  arrayMax(zombie[j]) = 0 ;
		}
	      dead = 0 ;
	    }
	  else
	    arrayMax (zombie[elt->frame]) = jKeep ;
	  if (elt->ptr)
	    { array(alive[frame], arrayMax(alive[frame]), ELT*) = elt ;
	      if (elt->ptr->type == BEGIN)
		elt->ptr = 0 ;
	    }
	  break ;
	case FRAME:		/* kill other frames */
	  for (frame = 0 ; frame < 3 ; ++frame)
	    { arrayMax(zombie[frame]) = 0 ;
	      if (frame != elt->frame)
		arrayMax(alive[frame]) = 0 ;
	      dead = 0 ;
	    }
	  break ;
	default:
	  break ;
	}
#ifdef DEBUG
      if (elt->ptr)
	printf (" | %d_%d %4.1f\n", 
		COORD(look,elt->ptr->x), elt->ptr->frame, elt->score) ;
      else
	printf ("\n") ;
#endif /* DEBUG */
    }

			/* find the best end point */
			/* want an end elt for intronScore,exonScore */
  end.type = END ; end.x = max - 1 ; end.ptr = 0 ;
  end.score = -1000000 ; 
  for (frame = 0 ; frame < 3 ; ++frame)
    for (j = 0 ; j < arrayMax(alive[frame]) ; ++j)
      { elt2 = arr(alive[frame], j, ELT*) ;
	score = elt2->score + 
	  exonScore (look, elt2, &end, frame, &look->gf) ;
	if (score > end.score)
	  { end.score = score ;
	    end.ptr = elt2 ;
	    endFrame = frame ;
	  }
      }
  for (zframe = 0 ; zframe < 3 ; ++zframe)
    for (j = 0 ; j < arrayMax(zombie[zframe]) ; ++j)
      { elt2 = arr(zombie[zframe], j, ELT*) ;
	score = elt2->score + intronScore (elt2, &end) ;
	if (score > end.score)
	  { end.score = score ;
	    end.ptr = elt2 ;
	  }
      }
  if (dead && dead->type != BEGIN)
    { if (dead->score > end.score)
	{ end.score = dead->score ;
	  end.ptr = dead ;
	}
    }
  else if (best && best->score > end.score)
    { end.score = best->score ;
      end.ptr = best ;
    }

	/* now backtrack, reversing the order of features using a stack
	*/
  if (end.ptr)
    { int kgene = 0 ;
      BOOL startNotFound ;
      Stack stack = stackCreate (32) ;

      messout ("min, max are %d, %d\n"
	       "total score is %f\n", 
	       COORD(look,min), COORD(look,max), end.score) ;

      for (elt = end.ptr ; elt ; elt = elt->ptr)
	push (stack, elt, ELT*) ;

      gene = arrayReCreate (gene, 16, int) ;
      while (!stackEmpty (stack))
        { elt = pop (stack, ELT*) ;

#ifdef DEBUG
	  printf ("\t%s at %6d score %6.2f localScore %6.2f\n", 
		  eltTypeName[elt->type], COORD(look,elt->x), 
		  elt->score, elt->localScore) ;
#endif /* DEBUG */

	  if (!arrayMax(gene))
	    startNotFound = (elt->type != START) ;
	  if (arrayMax(gene) == 1)			    /* find frame */
	    frame = elt->frame ;
	  array(gene, arrayMax(gene), int) = elt->x ;
	  if (elt->type == STOP)
	    {
	      setTempGene (look, pickWord2Class("Sequence"), gene, ++kgene, frame,
			   startNotFound, FALSE, FALSE) ;
	      gene = arrayReCreate (gene, 16, int) ;
	    }
	}

      if (arrayMax(gene))
	{
	  if (arrayMax(gene) % 2)
	    {
	      if (arrayMax(gene) == 1)
		frame = endFrame ;
	      array(gene, arrayMax(gene), int) = max-1 ;
	    }
	  setTempGene (look, pickWord2Class("Sequence"), gene, ++kgene, frame,
		       startNotFound, TRUE, FALSE) ;
	}

      stackDestroy (stack) ;
    }
  else
    messout ("Couldn't find a gene through the chosen features") ;

  return;
} /* fMapDP */


/********************/

static void fMapDP1 (void) 
{
  float temp = interGeneCost ;

  interGeneCost = -10000 ;
  fMapDP () ;
  interGeneCost = temp ;
}


/********************************************/

static int compareChosen (void *x, void *y)
{ 
  return UNHASH_X2(*(char**)x) - UNHASH_X2(*(char**)y) ;
}


/********************************************/

/* "Selected -> temp_gene" menu func */
static void selectedGeneToTemp(MENUITEM menu_item)
{
  int i ;
  int min, max ;
  int kgene = 0 ;
  char* v ;
  static Array chosen, gene ;
  FeatureMap look = currentFeatureMap("selectedGeneToTemp") ;
  int gene_class ;

  /* Retrieve the gene class chosen by the user from the menu. */
  gene_class = menuGetValue(menu_item) ;


  initialise(look) ;					    /* this seems crazy really.... */

  min = look->zoneMin ;
  max = look->zoneMax + 1 ;

  chosen = arrayReCreate(chosen, 8, char*) ;
  v = 0 ;
  while (assNext (look->chosen, &v, 0))
    {
      if (UNHASH_X2(v) >= min && UNHASH_X2(v) < max)
	array(chosen, arrayMax(chosen), char*) = v ;
    }

  if (!arrayMax(chosen))
    {
      messout("No chosen features in the active zone") ;

      return ;
    }

  arraySort (chosen, compareChosen) ;

  gene = arrayReCreate (gene, 8, int) ;

  if (UNHASH_TYPE(arr(chosen,0,char*)) != ATG)
    array(gene, 0, int) = min ;


#define IGNORE  { messout ("Impossible chosen feature %s near %d - ignoring", \
			   fMapSegTypeName[UNHASH_TYPE(v)], \
			   COORD(look, UNHASH_X2(v))) ; break ; }

  for (i = 0 ; i < arrayMax(chosen) ; ++i)
    {
      v = arr(chosen, i, char*) ;
      switch (UNHASH_TYPE(v))
	{
	case SPLICE3:
	  if (arrayMax(gene) % 2) IGNORE ;
	  array (gene, arrayMax(gene), int) = UNHASH_X2(v) ; 
	  break ;
	case SPLICE5:
	  if (!(arrayMax(gene) % 2)) IGNORE ;
	  array (gene, arrayMax(gene), int) = UNHASH_X2(v) - 1  ; 
	  break ;
	case ATG:
	  if (arrayMax(gene)) IGNORE ;
	  array (gene, arrayMax(gene), int) = UNHASH_X2(v) - 2  ; 
	  break ;
	case ORF:
	  if (!(arrayMax(gene) % 2)) IGNORE ;
	  array (gene, arrayMax(gene), int) = UNHASH_X2(v) + 3  ;
	  setTempGene (look, gene_class, gene, ++kgene, 0, FALSE, FALSE, TRUE) ;
	  gene = arrayReCreate (gene, 8, int) ;
	  break ;
	default:
	  /* could alter IGNORE to do this clause as well.... */
	  messout ("Unrecognized chosen feature type %s at %d - ignoring",
		   fMapSegTypeName[UNHASH_TYPE(v)],
		   COORD(look, UNHASH_X2(v))) ; break ;
	}
    }

  if (arrayMax(gene))
    {
      if (arrayMax(gene) % 2)
	array (gene, arrayMax(gene), int) = max ;

      setTempGene(look, gene_class, gene, ++kgene, 0, FALSE, TRUE, TRUE) ;
    }

  return ;
}





static void fMapChooseFromGene(FeatureMap look, KEY gene)
{
  int i ;
  SEG *seg ;

  initialise(look) ;

  /* THERE IS A PROBLEM HERE....IF WE ONLY LOOK AT CDS THEN WE CANNOT REPLICATE
   * TEMP GENE THAT HAS A START OR END SET FOR THE CDS */

  for (i = 1 ; i < arrayMax(look->segs) ; ++i)
    {
      seg = arrp(look->segs, i, SEG) ;
      if (seg->parent == gene)
	{
	  switch (seg->type)
	    {
	    case INTRON:
	      {
		assInsert(look->chosen, HASH (SPLICE5, seg->x1),
			  assVoid(seg->x1 - 1)) ;
		assInsert(look->chosen, HASH (SPLICE3, seg->x2 + 1),
			  assVoid(seg->x2)) ;
		break ;
	      }
	    case CDS:
	      assInsert (look->chosen, HASH (ATG, seg->x1 + 2),
			 assVoid(seg->x1)) ;
	      assInsert (look->chosen, HASH (ORF, seg->x2 - 3),
			 assVoid(1)) ;
	      break ;

	    default:
	      break ;
	    }
	}
    }

  fMapDraw(look, 0) ;

  return ;
}


static void setSelectedGene (void)
     /* menu/button func */
{
  FeatureMap look = currentFeatureMap("setSelectedGene") ;

  if (look->activeBox)
    {
      initialise(look) ;
      fMapChooseFromGene (look, BOXSEG(look->activeBox)->parent) ;
    }

  return ;
}


/********************************/


/* Takes a temp gene number and looks for corresponding gene name "temp_gene_<number>"
 * in all smappable gene classes and tries to 
 * remove its connections to the parent, best we can do I think for clearing up. */
static BOOL killGeneInAllClasses(int temp_gene_index)
{
  BOOL result = FALSE ;
  char *gene_name = g_strdup_printf("temp_gene_%d", temp_gene_index) ;
  KEYSET gene_classes ;

  if ((gene_classes = findAllSmappedGeneClasses()))
    {
      int i ;
      KEY key = KEY_UNDEFINED ;

      for (i = 0, result = TRUE ; i < keySetMax(gene_classes) && result == TRUE ; i++)
	{
	  /* As far as I can tell these calls do _not_ make permanent entries in the lex
	   * of each class which is what we want I think.... */
	  if (lexword2key(gene_name, &key, keySet(gene_classes, i))
	      && lexIsKeyVisible(key))
	    {
	      if (!killGene(key))			    /* can't clear previous entry */
		result = FALSE ;
	    }
	}
    }

  return result ;
}


/* Removes all tags in child that are handled by temp gene code, because of XREF'ing this
 * will result in tag in parent that points to child being removed as well. */
static BOOL killGene(KEY child_key)
{
  BOOL result = FALSE ;
  OBJ obj ;
  KEY parent_tag ;

  if ((parent_tag = getSMapParentTag(child_key)) && (obj = bsUpdate(child_key)))
    {
      if (bsFindTag (obj, parent_tag))
	bsPrune (obj) ;

      if (bsFindTag (obj, _Source_Exons))
	bsPrune (obj) ;

      if (bsFindTag (obj, _Start_not_found))
	bsPrune (obj) ;

      if (bsFindTag (obj, _End_not_found))
	bsPrune (obj) ;

      if (bsFindTag (obj, _CDS))
	bsPrune (obj) ;

      bsSave (obj) ;
      result = TRUE ;
    }
  else
    {
      messout ("Couldn't update %s", nameWithClassDecorate(child_key)) ;
      result = FALSE ;
    }

  return result ;
}



/***********************************/

/* "Remove Gene" menu/button func */
static void killGeneMenu(void)
{
  KEY gene ;
  FeatureMap look = currentFeatureMap("killGeneMenu") ;

  initialise(look) ;

  if (!look->activeBox
      || !(gene = BOXSEG(look->activeBox)->parent)
      || !isSmappedGeneClass(class(gene)))
    {
      messout ("You must select a gene object.") ;
    }
  else
    {
      gchar *gene_name = g_strdup(nameWithClassDecorate(gene)) ;
      gchar *parent_name = g_strdup(nameWithClassDecorate(getSMapParent(gene))) ;

      if (messQuery("Do you want to destroy the tags and location "
		    "information in \"%s\" which attach it to \"%s\" ?",
		    gene_name, parent_name))
	{
	  killGene(gene) ;
	  fMapPleaseRecompute(look) ;
	  fMapDraw(look, 0) ;
	}
     
      g_free(gene_name) ;
      g_free(parent_name) ;
    }

  return ;
}

/**************************/

static void setTempGene(FeatureMap look,
			int gene_class, Array a,
			int k, int frame,
			BOOL startNotFound, BOOL endNotFound, BOOL isByHand)
{
  KEY key, parentKey, speciesKey, fromLabKey ;
  OBJ obj, parentObj = NULL ;
  int i, base ;
  int x, y ;


#ifdef FMAP_DEBUG
  {
    ACEOUT dest = aceOutCreateToStdout(0);
    sMapDump(look->smap, dest);
    aceOutDestroy(dest);
  }
#endif

  if (!arrayMax(a))
    return ;

  if (arrayMax(a) % 2)
    {
      messerror("setTempGene() called with an odd number of defining points") ;
      return ;
    }


  /* -------- Make the temporary gene object. -------- */

  /* If a gene of name "temp_gene_<k>" exists in any of the smappable classes then
   * remove the position information in that gene to remove it from the smap/fmap. */
  if (!killGeneInAllClasses(k))
    return ;

  /* Create an entry in the lex for new temp gene in the class chosen by the user. */
  lexaddkey(messprintf("temp_gene_%d", k), &key, gene_class) ;

  if (!(obj = bsUpdate(key)))
    {
      messerror("Couldn't update temp gene object: \"%s\"", nameWithClassDecorate(key)) ;

      return ;
    }


  /* Find the start. */
  base = arr(a, 0, int) ;


  /* Set the Source_Exons tag. */
  base-- ;						    /* "- 1" because "Source_Exons"
							       must start at "1" not "0" */
  for (i = 0 ; i < arrayMax(a) ; i += 2)
    {
      x = arr(a, i, int) ;
      y = arr(a, i+1, int) ;
      x -= base ;
      y -= base ;
      bsAddData (obj, _Source_Exons, _Int, &x) ;
      bsAddData (obj, _bsRight, _Int, &y) ;
    }


  /* Try to set the CDS tag, note this is all a bit hokey:
   *    - we silently fail if the model for the chosen class does not contain a CDS tag
   *    - we add the tag whether the user wants it or not
   * to do better than this would require offering the user a choice or second guessing
   * the user. */
  bsAddTag (obj, _CDS) ;



  /* Is this CDS frame stuff correct in that base is perhaps off by one now ?? */
  /* Also, the frame stuff is not correct because when setting up the data from
   * which this temp gene is created we only looked at cds segs which are already
   * truncated to the start of the cds so adding a frame is incorrect.... */
  if (startNotFound)
    {
      bsAddTag (obj, _Start_not_found) ;
      frame = (3 + frame - ((base+1)%3)) % 3 + 1 ; /* CDS entry */

      if (frame != 1)
	bsAddData (obj, _Start_not_found, _Int, &frame) ;
    }


  if (endNotFound)
    bsAddTag (obj, _End_not_found) ;


  base++ ;						    /* remember that we subtracted one
							       when doing the source exons. */


  /* Create a default method object and add it to the gene object. */
  {
    KEY mkey ;
    OBJ mobj ;
    float rp = 2.0 ;

    if (isByHand)
      lexaddkey ("hand_built", &mkey, _VMethod) ;
    else if (lexIsGoodName(parmsName))
      lexaddkey (parmsName, &mkey, _VMethod) ;
    else
      lexaddkey ("assembly-default", &mkey, _VMethod) ;

    mobj = bsUpdate (mkey) ;
    if (mobj)
      {
	if (!bsGetData (mobj, str2tag("Right_priority"), _Float, 0))
	  bsAddData (mobj, str2tag("Right_priority"), _Float, &rp) ;
	if (!bsGetKeyTags (mobj, str2tag("Colour"), 0))
	  if (bsAddTag (mobj, str2tag("Colour")) && bsPushObj (mobj))
	    bsAddTag (mobj, isByHand ? _DARKBLUE : _BLUE) ;
	bsSave (mobj) ;
      }

    bsAddKey (obj, str2tag("Method"), mkey) ;
  }

  bsSave (obj) ;



  /* -------- Define the temp genes position in the parent object. -------- */

  x = base ;
  y = base + y - 1 ;
  parentKey = key ;	  /* pass this so don't get it back! */
  
  if (fMapFindSpanSequenceWriteable(look, &parentKey, &x, &y)
      && (parentObj = bsUpdate(parentKey)))
    {
      BOOL result ;
      int child_class, parent_class ;
      KEY tag_in_parent, tag_in_child ;
      
      child_class = gene_class ;
      parent_class = class(parentKey) ;
      

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
      printf("gene class: %s, parent class %s\n",
	     pickClass2Word(child_class), pickClass2Word(parent_class)) ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

      result = getPositionTags(child_class, parent_class,
			       &tag_in_parent, &tag_in_child) ;


      x++, y++ ;					    /* DB coords must be 1-based. */



      /* N.B. relies on xreffing to add reverse tags in child */
      bsAddKey (parentObj, tag_in_parent, key) ;

      bsAddData (parentObj, _bsRight, _Int, &x) ;
      bsAddData (parentObj, _bsRight, _Int, &y) ;
      

      /* copy Species and From_laboratory from parent */
      speciesKey = 0 ; 
      bsGetKey (parentObj, str2tag("Species"), &speciesKey) ;
      fromLabKey = 0 ;
      bsGetKey (parentObj, str2tag("From_laboratory"), &fromLabKey) ;
      bsSave (parentObj) ;

      if ((speciesKey || fromLabKey) && (obj = bsUpdate (key)))
	{
	  if (speciesKey)
	    bsAddKey (obj, str2tag("Species"), speciesKey) ;
	  if (fromLabKey)
	    bsAddKey (obj, str2tag("From_laboratory"), fromLabKey) ;
	  bsSave (obj) ;
	}
      
      fMapPleaseRecompute (look) ;
      
      /* We used to call fMapChooseFromGene() but that's not right, it highlights
       * stuff the user has "unselected", jsut do a draw. */
      fMapDraw(look, 0) ;
    }
  else
    messerror("Failed to update parent sequence object: \"%s\"", name(key)) ;

  return;
}



/****************************************/


static void recalcChecksums (void)
{
  KEY peptide = 0;

  messout("Recalulating NEW checksums"); 
  while (lexNext (_VProtein,&peptide))
    calculateAndSaveChecksum(peptide);
}      


/****************************************/

/* menu-item "Fix temp_gene"
 * 
 * NOTE, currently the user cannot choose a different class for the final object from the
 * class they chose for the temp gene creation. This can be changed if needed, this function
 * can easily be made to supply, via a submenu, a new user-chosen class.
 *  */
static void fixTemp(MENUITEM menu_item)
{
  char *newName;
  KEY old, source, temp ;
  OBJ oldObj = 0, tempObj = 0, sourceObj = 0,obj = 0 ;
  static KEY lastKey = 0 ;
  int x1, x2, i ;
  BOOL is_CDS, isNoStart, isNoEnd ;
  int cds_min, cds_max, start_offset ;
  static Array a = 0 ;
  BOOL exists = FALSE,thisgo=TRUE;
  Array newpep;
  KEY speciesKey, fromLabKey ;
  int newchk;
  KEY samepep,key,wormkey;
  static BOOL pepManage, first = TRUE;
  static char classtype[40],format[15];
  static int start,finish;
  char *cp = "checksum type 2" ; /* can use 3 4 5 later*/
  KEY dummy= 0;
  FeatureMap look = currentFeatureMap("fixTemp") ;
  ACEIN name_in = NULL;
  int child_class, parent_class ;


#define error(x)	{ messout(x) ; goto abort ;}



  if (!checkWriteAccess())
    return;

  if (!look->activeBox
      || !(temp = BOXSEG(look->activeBox)->parent)
      || !isSmappedGeneClass(class(temp)))
    {
      messout("You must select a gene object fix.") ;

      return ;
    }

  if (!(source = getSMapParent(temp)))
    {
      messerror("%s cannot be fixed as it does not have a parent object", nameWithClassDecorate(temp)) ;
      goto abort ;
    }


  child_class = class(temp) ;
  parent_class = class(source) ;

  /* Make sure dialog is not modal so user can still use fmap ! */
  gexSetModalDialog(FALSE) ;

  if ((name_in = messPrompt(messprintf ("Fix %s to:", nameWithClassDecorate(temp)),
			    lastKey ? name(lastKey) : "","wz", 0)))
    {
      newName = aceInWord(name_in);

      /* If object to fix the temp gene to does not exist then just do a rename of
       * the temp gene to that object, otherwise we have to copy data from
       * selected obj to fix obj. */
      if (!lexword2key(newName, &old, child_class))
	{
	  lexAlias(temp, newName, TRUE, FALSE) ;
	  lastKey = temp ;
	}
      else
	{
	  KEY tag_in_parent, tag_in_child ;
	  KEY method = KEY_NULL ;

	  if (old == temp)
	    error("You are trying to fix a gene to itself, no action taken.") ;

	  /* Examine model, if we fail its serious as it implies a database/model problem. */
	  if (!getPositionTags(child_class, parent_class,
			       &tag_in_parent, &tag_in_child))
	    {
	      messcrash("Can not get SMap tag information from class %s.",
			pickClass2Word(child_class)) ;
	    }


	  /* Destroy existing position information from the existing gene. */
	  if ((oldObj = bsCreate(old)))
	    {
	      if (getSMapParentTag(old)
		  && (!messQuery(messprintf ("Overwrite %s ?\n", nameWithClassDecorate(old)))
		      || !killGene(old)))
		goto abort ;

	      bsDestroy (oldObj) ;
	    }
	  

	  /* Update smap parent of selected gene with name of new gene but maintain
	   * position information of selected gene. */

	  if (!(sourceObj = bsUpdate(source)))
	    error ("Could not update source gene") ;

	  /* Get selected gene position information */
	  if (bsAddKey (sourceObj, tag_in_parent, temp))
	    {
	      bsPrune (sourceObj) ;
	      error ("No cross-reference of temp in parent") ;
	    }
	  if (!bsGetData (sourceObj, _bsRight, _Int, &x1) ||
	      !bsGetData (sourceObj, _bsRight, _Int, &x2))
	    error ("No offsets for temp in its parent") ;


	  /* Get rid of all existing information for selected gene and then add back
	   * new gene name and original position info. */
	  bsAddKey (sourceObj, tag_in_parent, temp) ;
	  bsPrune (sourceObj) ;
	  bsAddKey (sourceObj, tag_in_parent, old) ;
	  bsAddData (sourceObj, _bsRight, _Int, &x1) ;
	  bsAddData (sourceObj, _bsRight, _Int, &x2) ;
	  bsSave (sourceObj) ;
	  

	  /* Make sure we can open the new and old objects. */
	  if (!(oldObj = bsUpdate(old)))
	    error ("Could not update old gene") ;
	  
	  if (!(tempObj = bsUpdate(temp)))
	    error ("Could not update temp gene") ;


	  /* OK, now record all the information we need from the selected gene and then 
	   * erase it from the selected gene. */

	  if (!bsGetKey(tempObj, _Method, &method))
	    error ("No Method in temp gene")
	  else
	    bsGoto(tempObj, 0) ;

	  if (!bsFindTag (tempObj, _Source_Exons))
	    {
	      error ("No source exons in temp gene") ;
	    }
	  else
	    {
	      a = arrayReCreate (a, 32, BSunit) ;
	      bsFlatten (tempObj, 2, a) ;
	    }

	  if (bsFindTag (tempObj, tag_in_child))
	    bsPrune (tempObj) ;

	  if (bsFindTag (tempObj, _Source_Exons))
	    bsPrune (tempObj) ;

	  if ((isNoStart = bsFindTag(tempObj, _Start_not_found)))
	    {
	      start_offset = 0 ;			    /* start value of zero is not legal. */
	      bsGetData(tempObj, _bsRight, _Int, &start_offset) ;

	      bsFindTag(tempObj, _Start_not_found) ;
	      bsPrune (tempObj) ;
	    }

	  if ((isNoEnd = bsFindTag(tempObj, _End_not_found)))
	    bsPrune (tempObj) ;

	  if ((is_CDS = bsFindTag(tempObj, _CDS)))
	    {
	      cds_min = cds_max = 0 ;			    /* cds values of zero are not legal. */
	      if (bsGetData(tempObj, _bsRight, _Int, &cds_min))
		bsGetData(tempObj, _bsRight, _Int, &cds_max) ;

	      bsFindTag(tempObj, _CDS) ;
	      bsPrune (tempObj) ;
	    }

	  speciesKey = 0 ; 
	  if (bsGetKey (tempObj, str2tag("Species"), &speciesKey))
	    bsPrune (tempObj) ;

	  fromLabKey = 0 ;
	  if (bsGetKey (tempObj, str2tag("From_laboratory"), &fromLabKey))
	    bsPrune (tempObj) ;

	  bsSave(tempObj) ;				    /* nulls tempObj. */
	  

	  /* Now add all our recorded data to the existing gene, there's no error checking here !! */


	  if (!bsGetKey(oldObj, _Method, NULL))				    
	    bsAddKey(oldObj, _Method, method) ;		    /* Only add new method if orig obj does not have one. */

	  for (i = 0 ; i < arrayMax(a) ; i += 2)
	    {
	      bsAddData (oldObj, _Source_Exons, _Int, 
			 &(arr(a,i,BSunit).i)) ;
	      bsAddData (oldObj, _bsRight, _Int, 
			 &(arr(a,i+1,BSunit).i)) ;
	    }

	  if (isNoStart)
	    { 
	      bsAddTag (oldObj, _Start_not_found) ;
	      if (start_offset)
		bsAddData(oldObj, _bsRight, _Int, &start_offset) ;
	    }
	  else if (bsFindTag (oldObj, _Start_not_found))
	    bsPrune (oldObj) ;

	  if (isNoEnd) 
	    bsAddTag (oldObj, _End_not_found) ;
	  else if (bsFindTag (oldObj, _End_not_found))
	    bsPrune (oldObj) ;

	  if (is_CDS) 
	    {
	      bsAddTag(oldObj, _CDS) ;
	      if (cds_min)
		bsAddData(oldObj, _bsRight, _Int, &cds_min) ;
	      if (cds_max)
		bsAddData(oldObj, _bsRight, _Int, &cds_max) ;
	    }
	  else if (bsFindTag(oldObj, _CDS))
	    bsPrune (oldObj) ;

	  if (speciesKey) 
	    bsAddKey (oldObj, str2tag("Species"), speciesKey) ;
	  if (fromLabKey) 
	    bsAddKey (oldObj, str2tag("From_laboratory"), fromLabKey) ;

	  bsSave (oldObj) ;
	  lastKey = old ;
	}

      aceInDestroy (name_in);
    }
  gexSetModalDialog(TRUE) ;
  
  if (first)			/* establish whether to pepManage */
    { 
      char *filename;
      ACEIN db_in;
      char *word, *ct, *fmt;

      pepManage = FALSE;
      first = FALSE;
      
      filename = dbPathMakeFilName("wspec", "database","wrm", 0);
      db_in = aceInCreateFromFile (filename, "r", "", 0);
      messfree(filename);
      if (db_in)
	{ 
	  aceInSpecial (db_in, "\n\t\\/@");
	  while (aceInCard (db_in))
	    {
	      if ((word = aceInWord (db_in)))
		{
		  if (strcmp (word, "PEPMANAGE") == 0)
		    {
		      if ((ct = aceInWord(db_in)) 
			  && (fmt = aceInWord(db_in))
			  && (aceInInt(db_in, &start))
			  && (aceInInt(db_in, &finish)))
			{
			  strncpy (classtype, ct, 40);
			  classtype[40-1] = '\0';
			  strncpy (format, ct, 15);
			  format[15-1] = '\0';

			  pepManage = TRUE;
			}
		      else
			messerror
			  ("Invalid PEPMANAGE config in %s\n"
			   "Expecting PEPMANAGE to be followed by 2 "
			   "words (classtype, format) and 2 numbers "
			   "(start,finish)", filename);
		    }
		}
	    }
	  aceInDestroy (db_in);
	}
    }
  
  if (pepManage)
    {
      if ((obj = bsUpdate(source)))
	{
	  if (bsFindTag(obj,str2tag("Wormpep")))
	    {
	      thisgo = TRUE;
	      messout("Using new pepcode routines\n");
	    }
	  else
	    thisgo = FALSE;
	  bsDestroy(obj);
	}
      else
	thisgo = FALSE;

      for (key = 0 ; lexNext(_VClass, &key) ; )
	if (pickMatch (name(key), classtype)) 
	  break ;

      if(!key)
	{ 
	  messout("Could not find the class %s. "
		  "Therefore exiting FixTemp gene\n"
		  "Check that %s is in the subclasses.wrm",
		  classtype, classtype) ;
	  return ;
	}

      if (thisgo)
	{
	  if (!lexword2key (cp, &dummy, 0))
	    {
	      lexaddkey (cp, &dummy, 0) ; /* only once */
	      recalcChecksums();
	    }

	  newpep = peptideGet(temp);
	  newchk = hashArray(newpep);
	  if ((samepep = findPeptideMatch(temp,newchk,key)))
	    exists = TRUE;
	  i=start;
	  while (lexword2key(messprintf(format,i),&key,_VProtein)) i++ ;
	  if (i < finish)
	    lexaddkey(messprintf(format,i),&key,_VProtein);
	  else
	    { messout("ERROR: Limit of numbers for format has been reached see PEPMANAGE in database.wrm\n"
		      "exiting fix temp gene") ; 
	      return ;
	    }
	  if (exists)
	    { messout("existing protein %s matches temp gene %s so setting Corresponding_protein accordingly",
		      name(samepep), name(temp)) ;
	      lastKey = temp ;      
	      obj = bsUpdate(temp);
	      if (!obj || !bsAddKey(obj,str2tag("Corresponding_Protein"), samepep))
		error ("Error adding Corresponding Protein");
	      bsSave (obj);
	    }
	  else
	    { messout("Does NOT match any other protein.  "
		      "Creating new protein object %s", messprintf(format,i)) ;
	      if (!lexword2key (messprintf(format,i), &wormkey, _VProtein))
		{ error("lexwordtokey could not find protein") ;}
	      else if (peptideStore(wormkey,newpep))
		{ if (!(obj = bsUpdate (temp)))
		    error ("Could not update old gene") ;
		  if(!bsAddKey(obj,str2tag("Corresponding_Protein"),wormkey))
		    error ("Error adding Corresponding Protein");
		  bsSave(obj);
		  if ((obj = bsUpdate (wormkey)))
		    { if(!bsAddTag(obj,str2tag(classtype)))
			error("Error adding Wormpep tag");
		      bsSave(obj);
		    }
		}
	      else  
		error ("Unable to add Peptide");
	    }
	}
    }

  fMapPleaseRecompute (look) ;
  fMapDraw (look, lastKey) ;

  return ;
  

 abort:
  bsDestroy (oldObj) ;
  bsDestroy (tempObj) ;
  bsDestroy (sourceObj) ;
  if (name_in)
    aceInDestroy (name_in);

  return ;
} /* fixTemp */

/****************************************/
/****************************************/

static void fMapHideGfColumns (void)
     /* menu/button func */
{ 
  FeatureMap look = currentFeatureMap("fMapHideGfColumns") ;

  mapColSetByName (look->map, "ATG", FALSE) ;
  mapColSetByName (look->map, "ORF's", FALSE) ; 
  mapColSetByName (look->map, "Coding Frame", FALSE) ; 
  mapColSetByName (look->map, "GF_coding_seg", FALSE) ;
  mapColSetByName (look->map, "GF_splice", FALSE) ;
  mapColSetByName (look->map, "hexExon", FALSE) ;

  look->isGeneFinder = FALSE;
  fMapDraw (look, 0) ;

  return;
} /* fMapHideGfColumns */

static void fMapShowGfColumns (void)
     /* menu/button func */
{ 
  FeatureMap look = currentFeatureMap("fMapShowGfColumns") ;

  mapColSetByName (look->map, "ATG", TRUE) ;
  mapColSetByName (look->map, "ORF's", TRUE) ; 
  mapColSetByName (look->map, "Coding Frame", TRUE) ; 
  mapColSetByName (look->map, "GF_coding_seg", TRUE) ;
  mapColSetByName (look->map, "GF_splice", TRUE) ;
  mapColSetByName (look->map, "hexExon", TRUE) ;

  look->isGeneFinder = TRUE;
  fMapDraw (look, 0) ;

  return;
} /* fMapShowGfColumns */

/**********************************************************/
/************** code to add ATG, splice sites *************/

static void addATG (void)
     /* menu/button func */
{
  SEG *seg;
  int x;
  FeatureMap look = currentFeatureMap("addATG");

  x = look->DNAcoord + 1;

  initialise (look);

  if (!look->activeBox)
    return;
  seg = BOXSEG(look->activeBox);
  if (seg->type != DNA_SEQ)
    return;

  arrayMax(look->segs) = look->lastTrueSeg;
  seg = arrayp(look->segs, arrayMax(look->segs), SEG);
  seg->type = ATG;
  seg->key = M_GF_ATG;
  seg->parent = 0 ;
  seg->x1 = x-1;
  seg->x2 = x+1;
  seg->data.f = 4.0;
  assInsert (look->chosen, SEG_HASH(seg), assVoid(seg->x1)) ;
  arraySort(look->segs, fMapOrder) ;
  look->lastTrueSeg = arrayMax(look->segs);
  fMapProcessMethods(look);
  fMapDraw(look, 0);

  return;
} /* addATG */

static void addSplice3 (void)
     /* menu/button func */
{
  SEG *seg;
  int x;
  FeatureMap look = currentFeatureMap("addSplice3");

  x = look->DNAcoord + 1;

  initialise (look);

  if (!look->activeBox)
    return;
  seg = BOXSEG(look->activeBox);
  if (seg->type != DNA_SEQ)
    return;

  arrayMax(look->segs) = look->lastTrueSeg;
  seg = arrayp(look->segs, arrayMax(look->segs), SEG);
  seg->type = SPLICE3;
  seg->key = M_GF_splice;
  seg->parent = 0 ;
  seg->x1 = x-2;
  seg->x2 = x-1;
  seg->data.f = 4.0;
  assInsert (look->chosen, SEG_HASH(seg), assVoid(seg->x1)) ;
  arraySort(look->segs, fMapOrder) ;
  look->lastTrueSeg = arrayMax(look->segs);
  fMapProcessMethods(look);
  fMapDraw(look, 0);

  return;
} /* addSplice3 */


static void addSplice5 (void)
     /* menu/button func */
{ 
  SEG *seg;
  int x;
  FeatureMap look = currentFeatureMap("addSplice5");

  x = look->DNAcoord +1;

  initialise (look);

  if (!look->activeBox)
    return;
  seg = BOXSEG(look->activeBox);
  if (seg->type != DNA_SEQ)
    return;

  arrayMax(look->segs) = look->lastTrueSeg;
  seg = arrayp(look->segs, arrayMax(look->segs), SEG);
  seg->type = SPLICE5;
  seg->key = M_GF_splice;
  seg->parent = 0 ;
  seg->x1 = x-1;
  seg->x2 = x;
  seg->data.f = 4.0;
  assInsert (look->chosen, SEG_HASH(seg), assVoid(seg->x1)) ;
  arraySort(look->segs, fMapOrder) ;
  look->lastTrueSeg = arrayMax(look->segs);
  fMapProcessMethods(look);
  fMapDraw(look, 0) ;

  return;
} /* addSplice5 */

/****************************************/


/**********************************************************************/
/************** code to show the selected set of features *************/

static void addLine (FeatureMap look, 
		     int *pStart, int *p3, int *p5, int *pStop, 
		     int line)
{
  static int frame ;
  static int oldLine = 10000 ;
  static float score ;
  int i ;
  SEG *seg ;
  static ELT elt1, elt2 ;
  float tscore = 0 ;

  elt1.type = DUMMY ;

  if (*pStart)
    {
      for (i = 0 ; i < arrayMax(look->segs) ; ++i)
	{
	  seg = arrp(look->segs, i, SEG) ;

	  if (seg->type == ATG && seg->x1 == *pStart)
	    {
	      graphText (messprintf ("%5.2f", seg->data.f), 
			 22, line) ;
	      tscore += seg->data.f ;
	      break ;
	    }
	}

      frame = *pStart % 3 ;
      score = 0 ;
      elt2.type = DUMMY ;	/* prevents INTRON score */
      elt1.type = START ;
      elt1.x = *pStart ;
    }

  if (*p3)
    { for (i = 0 ; i < arrayMax(look->segs) ; ++i)
	{ seg = arrp(look->segs, i, SEG) ;
	  if (seg->type == SPLICE3 && seg->x2 == *p3)
	    { graphText (messprintf ("%5.2f", seg->data.f), 
			 22, line) ;
	      tscore += seg->data.f ;
	      break ;
	    }
	}
      elt1.type = SPLICE_3 ;
      elt1.x = *p3 ;
    }

  if (oldLine < line && elt1.type)
    { graphText (messprintf ("%6.2f", intronScore (&elt2, &elt1)),
		 50, line - 0.5) ;
      score += intronScore (&elt2, &elt1) ;
      frame += (elt1.x - elt2.x - 1) ; frame %= 3 ;
    }

  elt2.type = DUMMY ;
  if (*p5)
    { for (i = 0 ; i < arrayMax(look->segs) ; ++i)
	{ seg = arrp(look->segs, i, SEG) ;
	  if (seg->type == SPLICE5 && seg->x1 == *p5)
	    { graphText (messprintf ("%5.2f", seg->data.f), 
			 35, line) ;
	      tscore += seg->data.f ;
	    }
	}
      elt2.type = SPLICE_5 ;
      elt2.x = *p5 ;
    }
  if (*pStop)
    { elt2.type = STOP ;
      elt2.x = *pStop - 3 ;
    }

  if (elt1.type && elt2.type)
    { graphText (messprintf ("%6.2f", 
			     exonScore (look, &elt1, &elt2, 
					frame, &look->gf)),
		 28, line) ;
      tscore += exonScore (look, &elt1, &elt2, frame, &look->gf) ;
      oldLine = line ;
    }

  if (tscore)
    graphText (messprintf ("%6.2f", tscore), 42, line) ;

  score += tscore ;
  graphText (messprintf ("%6.2f", score), 58, line) ;

  *pStart = *p3 = *p5 = *pStop = 0 ;

  return;
} /* addLine */

/******************************/

static void showSelect (void)
{
  Array chosen ;
  char *v, *vx ;
  int i, x, line = 2 ;
  int xStart, x3, x5, xStop ;
  Graph oldGraph ;
  FeatureMap look = currentFeatureMap("showSelect") ;

#define START_OFF 1
#define S3_OFF    3
#define S5_OFF	  11
#define STOP_OFF  13

  if (!(oldGraph = graphActivate (look->selectGraph)))
    {
      look->selectGraph = graphCreate (TEXT_SCROLL, 
				       "Gene structure", 
				       0.0, 0.0, 0.7, 0.2) ;
      graphMenu (selectMenu) ;
    }
  else
    {
      graphPop () ;
      graphClear () ;
    }

  graphText ("ATG/3'   5'/STOP    feat1 coding feat2    "
	     "Exon  Intron   Total",
	     2, 0.4) ;
  graphLine (0, 1.7, 64, 1.7) ;

  chosen = arrayCreate (32, char*) ;
  v = 0 ; 
  while (assNext (look->chosen, &v, &vx))
    { 
      x = assInt (vx) ;
#ifdef GF_SEG_DEFINED
      if (UNHASH_TYPE(v) == GF_SEG)
	v = HASH(GF_SEG, x + 3*((UNHASH_X2(v) - x)/6)) ;
#endif
      array(chosen, arrayMax(chosen), char*) = v ;
    }

  arraySort (chosen, compareChosen) ;

  xStart = x3 = x5 = xStop = 0 ;

  for (i = 0 ; i < arrayMax(chosen) ; ++i)
    {
      v = arr(chosen, i, char*) ;

      switch (UNHASH_TYPE(v))
	{
	case ATG:
	  if (xStart || x3 || x5 || xStop)
	    addLine (look, &xStart, &x3, &x5, &xStop, line++) ;
	  xStart = UNHASH_X2(v) - 2 ;
	  graphText (messprintf ("%7d", COORD(look, xStart)),
		     START_OFF, line) ;
	  break ;
	case SPLICE3:
	  if (xStart || x3 || x5 || xStop)
	    addLine (look, &xStart, &x3, &x5, &xStop, line++) ;
	  x3 = UNHASH_X2(v) ;
	  graphText (messprintf ("%7d", COORD(look, x3)),
		     S3_OFF, line) ;
	  break ;
	case SPLICE5:
	  if (x5 || xStop)
	    addLine (look, &xStart, &x3, &x5, &xStop, line++) ;
	  x5 = UNHASH_X2(v) - 1 ;
	  graphText (messprintf ("%7d", COORD(look, x5)),
		     S5_OFF, line) ;
	  break ;
	case ORF:
	  if (x5 || xStop)
	    addLine (look, &xStart, &x3, &x5, &xStop, line++) ;
	  xStop = UNHASH_X2(v) + 3 ;
	  graphText (messprintf ("%7d", COORD(look, xStop)),
		     STOP_OFF, line) ;
	  break ;
	}
    }

  if (xStart || x3 || x5 || xStop)
    addLine (look, &xStart, &x3, &x5, &xStop, line++) ;

  arrayDestroy (chosen) ;

  graphLine (21, 0, 21, line) ;
  graphLine (49, 0, 49, line) ;
  graphTextBounds (64, line) ;
  graphRedraw () ;

  return;
} /* showSelect */
 
/******************** autocreate peptide code ***********************/

static int calculateAndSaveChecksum(KEY protein)
{
  Array pep;
  int x1,i = 0;
  OBJ obj;
  KEY key;

  if (!(pep = peptideGet(protein)) || arrayMax(pep) < 2)
    {
      return 0;
    }
  else 
    i = hashArray(pep);

  if(pep)
    arrayDestroy(pep);

  if(i)
    {
      if ((obj = bsUpdate(protein)))
	{
	  bsFindTag(obj,_Peptide);
	  bsGetKey(obj,_Peptide,&key); /* peptide */
	  bsGetData(obj,_bsRight, _Int, &x1); /* length */
	  bsAddData(obj,_bsRight, _Int,&i);   /* add the checksum */
	  bsSave(obj);
	}
    }

  return i;
} /* calculateAndSaveChecksum */

static KEY findPeptideMatch(KEY temp,int newchk,KEY key2) 
{
  KEY peptide = 0,key;
  int i,x1;
  Array pep1,pep2;
  BOOL found=TRUE;
  OBJ obj;
  char *p1,*p2;

/*  printf("Searching for subclass %s\n",name(key2));*/
  while (lexNext (_VProtein,&peptide)){
    if(lexIsInClass(peptide,key2)){
      if ((obj = bsCreate(peptide))){
	if(bsFindTag(obj,str2tag("Peptide"))){
	  if(bsGetKey(obj,_Peptide,&key)){              /* peptide */
	    if(!bsGetData(obj,_bsRight, _Int, &x1))     /* length */
	      continue;
	    x1 = 0;
	    bsGetData(obj,_bsRight, _Int, &x1);         /* checksum */
	    bsDestroy(obj);
	    if(x1==0)
	      x1 = calculateAndSaveChecksum(peptide);
	    if(x1==newchk && peptide != temp){ /* if same checksum and not the one we are testing against */
	      printf ("Same check sum found\n");
	      pep1 = peptideGet(temp);
	      pep2 = peptideGet(peptide);
	      found = TRUE;
	      if(arrayMax(pep1) == arrayMax(pep2)){ /* If they are the same length */
		printf ("They have the same length\n");
		p1 = arrp(pep1, 0, char) ;
		p2 = arrp(pep2, 0, char) ;
		for(i=0;i<arrayMax(pep1);i++){
		  if(pepDecodeChar[(int)*p1++] != pepDecodeChar[(int)*p2++]){
		    found = FALSE;
		    printf ("Not the same sequence though\n");
		    break;
		  }
		}
	      }
	      arrayDestroy(pep1);
	      arrayDestroy(pep2);
	      if(found)
		printf ("Identical peptides\n");
	      if(found)
		return peptide;
	    }
	  }
	}
	else
	  bsDestroy(obj);
      }
    }
  }

  return 0;
} /* findPeptideMatch */



/* 
 *            Utility routines for creating/fixing temporary objects.
 * 
 */


/* These two functions could become part of the smap package....(not the isSmappedGeneClass()
 * bit obviously. */

/* Returns the tag in the child that specifies the parent object. */
static KEY getSMapParentTag(KEY child_key)
{
  KEY parent_map_tag = KEY_UNDEFINED ;

  if (isSmappedGeneClass(class(child_key)))
    {
      if (class(child_key) == pickWord2Class("Sequence"))
	parent_map_tag = str2tag("Source") ;
      else
	parent_map_tag = str2tag("S_Parent") ;
    }

  return parent_map_tag ;
}

/* Returns the parent object of the child. */
static KEY getSMapParent(KEY child_key)
{
  KEY parent_key = KEY_UNDEFINED ;

  if (isSmappedGeneClass(class(child_key)))
    {
      /* Ignore result of bIndex calls, we will return KEY_UNDEFINED if either fails. */
      if (class(child_key) == pickWord2Class("Sequence"))
	bIndexGetKey(child_key, str2tag("Source"), &parent_key) ;
      else
	bIndexGetTag2Key(child_key, str2tag("S_Parent"), &parent_key) ;
    }

  return parent_key ;
}


/* Currently the Sequence class can always be an Smappable gene, other classes must
 * have at least the S_Parent and Source_Exons tags. */
static BOOL isSmappedGeneClass(int class)
{
  BOOL result = FALSE ;
  KEY smap_tag, exons_tag ;

  smap_tag = str2tag("S_Parent") ;
  exons_tag = str2tag("Source_Exons") ;

  if (class == pickWord2Class("Sequence")
      || (bsIsTagInClass(class, smap_tag) && bsIsTagInClass(class, exons_tag)))
    result = TRUE ;

  return result ;
}

/* Currently the Sequence class can always be a parent of a gene, other classes must
 * have at the S_Child tag. */
static BOOL isSmappedParentClass(int class)
{
  BOOL result = FALSE ;
  KEY schild_tag ;

  schild_tag = str2tag("S_Child") ;

  if (class == pickWord2Class("Sequence")
      || (bsIsTagInClass(class, schild_tag)))
    result = TRUE ;

  return result ;
}


/* Finds all the classes that are defined to be SMap'd gene classes and returns
 * them in a keyset, NOTE WELL that each item in the keyset is a _class_ and
 * _not_ a key,
 * 
 *              i.e. you can do  pickClass2Word(keySet(gene_keyset, n))
 *                      but not  className(keySet(gene_keyset, n))
 * 
 * because the latter expects to be given a valid KEY.
 *
 *
 * What's interesting here is that it seems to be impossible to search an "object" in the
 * Model class using query language or any of the tag searching code, it just doesn't
 * work. Here its all a bit steam-like, I convert the "object" name which will be of the
 * form "?class_name" to "class_name" and then find out the class id (an int) for that
 * name and then use the id to look for a tag in the class...ouch.
 * 
 * Ideally we need a couple of levels of functions added to the bs package to do this:
 *             BOOL bsAreTagsInClass(int class, keyset tags)
 * which would be called by something like:
 *             KeySet bsAreTagsInModel(keyset tags)
 *  */
static KEYSET findAllSmappedGeneClasses(void)
{
  KEYSET model_classes, gene_classes ;
  char *all_models_query = "FIND Model" ;
  int i, j, tot_classes ;
  KEY smap_tag, exons_tag ;

  smap_tag = str2tag("S_Parent") ;
  exons_tag = str2tag("Source_Exons") ;

  model_classes = query(0, all_models_query) ;
  tot_classes = keySetMax(model_classes) ;

  gene_classes = keySetCreate() ;

  for (i = 0, j = 0 ; i < tot_classes ; i++)
    {
      KEY model_key ;
      int class ;

      model_key = keySet(model_classes, i) ;

      class = classModel2Class(model_key) ;
      
      if (isSmappedGeneClass(class))
	{
	  keySet(gene_classes, j) = class ;

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
	  printf("found tags in obj class: %s, name: %s, giving class: %s\n",
		 className(model_key), name(model_key), pickClass2Word(class)) ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

	  j++ ;
	}
    }

  if (keySetMax(gene_classes) == 0)
    {
      keySetDestroy(gene_classes) ;
      gene_classes = NULL ;
    }

  return gene_classes ;
}



/* Get the position tags for the child and parent classes so we can link the temp
 * gene child correctly to the parent. */
static BOOL getPositionTags(int child_class, int parent_class,
			    KEY *tag_in_parent_out, KEY *tag_in_child_out)
{
  BOOL result = FALSE ;

  /* We can't do anything unless the classes are defined properly. */
  if (isSmappedGeneClass(child_class) && isSmappedParentClass(parent_class))
    {
      KEY tag_in_child, tag_in_parent ;
      char *child_str, *parent_str ;

      child_str = pickClass2Word(child_class) ;
      parent_str = pickClass2Word(parent_class) ;

      /* Deal with Sequence class a bit differently as currently is basically
       * "hard-coded", other classes all use SMap tags. */
      if ((strcasecmp(child_str, "Sequence")) == 0 && (strcasecmp(parent_str, "Sequence") == 0))
	{
	  tag_in_parent = str2tag("Subsequence") ;
	  tag_in_child = str2tag("Source") ;
	  result = TRUE ;
	}
      else
	{
	  if (sMapGetChildParentTag2s(child_class, parent_class, &tag_in_child, &tag_in_parent))
	    result = TRUE ;
	}

      if (result)
	{
	  if (tag_in_parent_out)
	    *tag_in_parent_out = tag_in_parent ;
	  if (tag_in_child_out)
	    *tag_in_child_out = tag_in_child ;
	}

    }

  return result ;
}




/* Should be part of SMap package.....??????? But would need to deal with repeated
 * parent class problem first (see below). */

/* Returns the tag2's used to make the link between a child and its parent in an
 * smap. These can be used to insert SMap tags into objects to link them into
 * an SMap. Returns TRUE if the tag2's could be found correctly, the tags are
 * optionally returned as well if non-NULL pointers are supplied for child/parent
 * tags.
 * 
 * Makes use of this layout in the child class:
 *
 * SMap S_Parent UNIQUE child_tag2_A UNIQUE parent_class_A XREF parent_tag2
 *                      child_tag2_B UNIQUE parent_class_B XREF parent_tag2
 *                      etc.
 * 
 * Note that currently this code will not work for situations where the same
 * parent class appears more than once in the list of possible parents, the
 * function will simply return the _first_ matching parent_class it finds.
 * 
 * 
 *  */
static BOOL sMapGetChildParentTag2s(int child_class, int parent_class,
				    KEY *child_tag2_out, KEY *parent_tag2_out)
{
  BOOL result = FALSE ;
  KEY sparent_tag = str2tag("S_Parent") ;
  KEY unique_tag = str2tag("UNIQUE"), xref_tag = str2tag("XREF") ;
  KEYSET model_path ;
  KEY path_key ;
  KEYSET model_classes ;
  KEY model_key ;
  OBJ model_obj ;
  BS bs ;
  int i, j ;

  /* Can't do anything unless we can find "S_Parent" in class and we can retrieve
   * the path to it.
   * NOTE: comments embedded in bsGetPath() say that you should not use keySetMax(),
   * the key you want will be before the end of the array, you should follow path until
   * you get a NULL entry. Seems really hokey to me....bsGetPath() is probably back-tracking
   * and not resetting arrayMax. */
  if (!bsIsTagInClass(child_class, sparent_tag)
      || !(model_path = bsGetPath(child_class, sparent_tag)))
    {
      result = FALSE ;
      return result ;
    }


  /* Find the model "object" for the class. */
  {
    gchar *model_query ;

    model_query = g_strdup_printf("FIND Model ?%s", pickClass2Word(child_class)) ;
    model_classes = query(0, model_query) ;
    g_free(model_query) ;
  }


  /* Open the model "object" and search down it until we find S_Parent tag,
   * (Note that you _cannot_ use the normal bFindNNNN() functions on a model object.) */

  model_key = keySet(model_classes, 0) ;
  model_obj = bsCreate(model_key) ;
  bs = model_obj->curr ;
  i = keySetMax(model_path) ;
  j = 0 ;
  path_key = keySet(model_path, j) ;

  /* We need to stop as soon as there is a null entry in the path, not when we get to
   * keySetMax(). */
  while (path_key && i--)
    {
      bs = bs->right ;
      while (bs->key != path_key)
	{
	  bs = bs->down ;
	}

      j++ ;
      path_key = keySet(model_path, j) ; 
    }


  /* ok, should be located at S_Parent tag now, crash here if not
   * equal because it means that the database is inconsistent. */
  if (bs->key != sparent_tag)
    {
      messcrash("Database inconsistent: path exists to S_Parent tag in class \"%s\""
		"but could not find S_Parent tag in model for class",
		pickClass2Word(child_class)) ;
    }


  {
    /* Move from S_Parent to tag2's, skipping over intervening UNIQUEs and
     * checking for XREF:
     *
     *    S_Parent UNIQUE child_tag2 UNIQUE parent_class XREF parent_tag2
     */
    KEY tag2_class_tag ;
    int tag2_class ;
    BOOL found ;
    KEY  child_tag2, parent_tag2 ;

	
    bs = bs->right ;
    if (bs->key != unique_tag)
      messcrash("SMap tag set incorrect, S_Parent must be followed by UNIQUE") ;
    else
      bs = bs->right ;
    
    /* Search down the tag2's until we find one that is followed by parent_class, when
     * we find it, record the tag2 and also the tag following the XREF. */
    found = FALSE ;
    while (bs)
      {
	KEY tag2 ;

	tag2 = bs->key ;

	if ((bs->right)->key != unique_tag)
	  messcrash("SMap tag set incorrect, S_Parent tag2 must be followed by UNIQUE") ;
	else
	  tag2_class_tag = (bs->right)->right->key ;

	tag2_class = classModel2Class(tag2_class_tag) ;

	/* Found the right class so record child and XREF'd parent tag2's and exit loop. */
	if (tag2_class == parent_class)
	  {
	    BS bs_tmp ;

	    bs_tmp = (bs->right)->right->right ;
	    if (bs_tmp->key != xref_tag)
	      messcrash("SMap tag set incorrect, S_Parent parent class "
			"must be followed by XREF") ;

	    found = TRUE ;
	    child_tag2 = bs->key ;
	    parent_tag2 = bs_tmp->right->key ;

	    break ;					    /* exit loop. */
	  }

	/* Otherwise down to next entry. */
	bs = bs->down ;
      }


    /* If we didn't find what we wanted is this a fatal error ? Think about it.... */
    if (found)
      {
	if (child_tag2_out)
	  *child_tag2_out = child_tag2 ;
	if (parent_tag2_out)
	  *parent_tag2_out = parent_tag2 ;

	result = TRUE ;
      }
  }
      

  return result ;
}


/*********************** end of file **********************/
