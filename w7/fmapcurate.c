/*  File: fmapcurate.c
 *  Author: Richard Durbin (rd@sanger.ac.uk)
 *  Copyright (C) Richard Durbin, Genome Research Limited 2001
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
 * This file is part of the ACEDB genome database package
 *
 * Description: Curation tools to accept new CDS
 * HISTORY:
 * Last edited: Jul  4 11:19 2003 (edgrif)
 * Created: Jun 27 14:58 2001 (rd)
 * $Id: fmapcurate.c,v 1.10 2003-07-04 10:41:51 edgrif Exp $
 *-------------------------------------------------------------------
 */

#define ATTRIBUTES

  /* Strategy: make window with following:

if cds is an existing curated cds:
  option [] kill xyz.n unless it forms a bridge
  * NB can just edit the remark in this case.
? *Message: If you want to kill a transcript that forms a bridge,
            replace it with one in one of the final children
	    then kill that.

if cds overlaps nothing:
  [] create new gene xyz.n
  [] if overlaps dead history gene then option to resurrect 
  [] if overlaps live history gene then option to split from this

if cds overlaps one gene:
  [] create new CDS for that gene
  [] replace existing CDS for each existing cds of the gene
  * NB latter can create new genes if it formed a bridge

if cds overlaps multiple genes:
  []  replace xyz.n <for each transcript overlapped>
  < merged gene gets name of this transcript >
  *Message: If you want to create a novel transcript to link multiple
            genes then make it first in one gene and replace.

then two buttons: [CURATE] [CANCEL]

Then attributes.  We could reset the attributes dynamically 
depending on the choice made, i.e. if choose to replace then
show remarks for the CDS to be replaced.

In attributes, always start a new remark starting with:
  [<name> <date>] 

   */

   /* Model assumptions and naming conventions made by this file:

?Gene CDS_info CDS ?CDS XREF Gene
      	       CDS_history ?CDS XREF Gene_history
               CDS_count UNIQUE Int	// for curateNewCDSletter()
      Name Sequence_name UNIQUE ?Gene_name XREF Sequence_name
  // plus more name, history etc. needed by idcurate.c

?CDS Gene_info Gene UNIQUE ?Gene XREF CDS  // when live
               Gene_history UNIQUE ?Gene XREF CDS_history  // when dead
     Method UNIQUE ?Method
     Start_not_found UNIQUE Int
     End_not_found
     CDS
     Species UNIQUE ?Species
     Remark ?Text
     SMap S_Parent Sequence_parent UNIQUE ?Sequence XREF CDS_child
          Source_exons Int Int

?Sequence  Genomic_canonical Gene_count UNIQUE Int
				// for curateNewGeneName()
	   SMap S_Child CDS_child ?CDS XREF Sequence_parent Int Int
	   Species UNIQUE ?Species

   */

#include <wh/acedb.h>
#include <wh/aceio.h>
#include <wh/lex.h>
#include <wh/main.h>		/* for checkWriteAccess() */
#include <wh/query.h>
#include <wh/aql.h>
#include <wh/bs.h>
#include <whooks/systags.h>
#include <wh/bindex.h>
#include <wh/pick.h>		/* for pickWord2Class */
#include <w7/fmap_.h>
#include <wh/dna.h>		/* for codon() */
#include <wh/parse.h>
#include <wh/aceio.h>
#include <wh/dump.h>
#include <wh/peptide.h>
#include <wh/idcurate.h>

static int CURATE_STATE_HOOK ;

typedef struct {
  char *tag ;
  char *value ;
  char *newValue ;
} Attribute ;
				/* all tags you want to get*/
static FREEOPT legalAttributeOpts[] = { 
  { 5, "Legal attribute tags" }, 
  { 1, "Remark" }, 
  { 2, "Confidential_remark" },
  { 3, "DB_remark" },
  { 4, "Brief_identification" },
  { 5, "Pseudogene" }
} ;

typedef struct {
  FeatureMap fmap ;
  Graph graph ;		/* for the curation window */
  STORE_HANDLE handle ;	
  KEY key ;		/* key of CDS object to curate */
  Array cdsOverlaps ;	
  Array geneOverlaps ;	
  char *newGeneName ;	/* next 3 set by curateNewGeneName() */
  KEY newGeneParent ;	/* keep parent, number so can update */
  int newGeneNumber ;	/* Gene_count in parent when made */
  int newCDSletter ;	
  KEY exactMatch ;	/* set by curateExactMatch() */
  int action ;		/* which action to take */
  int option ;
  Array attributes ;
  int newAttributeBox ;
  IdCnxn ix ;
  BOOL isPseudo ;
  KEY  species ;
} CurateState ;

static BOOL  curateInitialise (void) ; /* FALSE if fails */
static BOOL  curateOverlaps (CurateState *state, KEY method) ;
static BOOL  curateNewGeneName (CurateState *state) ;
static void  setGeneCount (KEY key, int n) ;
static char  curateNewCDSletter (KEY gene) ;
static void  curateSubmit (void) ;

typedef enum { NOTHING = 0, KILL, CREATE, REPLACE, MERGE } CurateAction ;

static KEY   cdsTransfer (KEY source, KEY gene, int letter) ;
static BOOL  convertToHistory (CurateState *state, KEY key) ;

static Array getAttributes (KEY key, STORE_HANDLE handle) ;
static BOOL  storeAttributes (KEY key, Array attributes) ;

static char* seqName (KEY key) ; /* little utilities */
static char* cdsName (KEY key, char c) ;

/******************************************************/

  /* package globals established in curateInitialise() */

static KEY methodCurated ;
static KEY methodHistory ;
static int classSequence ;
static int classGene ;
static int classCDS ;
static char *user = 0 ;

static BOOL curateInitialise (void)
{
  static int done = FALSE ;
  ACEIN in ;			/* for messPrompt() */

  if (done) return TRUE ;

  if (!lexClassKey ("Method:curated", &methodCurated) ||
      !lexClassKey ("Method:history", &methodHistory))
    { messout ("Curation requires existence of Method objects "
	       "\"Curated\" and \"History\"") ;
      return FALSE ;
    }
  if (!(classGene = pickWord2Class ("Gene")) ||
      !(classSequence = pickWord2Class ("Sequence")) ||
      !(classCDS = pickWord2Class ("CDS")))
    { messout ("Curation requires existence of "
	       "Gene, Sequence and CDS classes") ;
      return FALSE ;
    }

  in = messPrompt ("Give user name for remarks (first time only)", 
		   getLogin(TRUE), "wz", 0) ;
  if (in && (user = aceInWord (in)))
    user = strnew (user, 0) ;
  else
    return FALSE ;

  done = TRUE ;
  return TRUE ;
}

/***********************************************************/
/******* PRIMARY FUNCTION TO CREATE CURATION TOOL **********/
/***********************************************************/

static void attributeEditor (KEY key, int x) ; /* callback */

void fMapCurateCDS (int box)
{
  CurateState *state ;
  FeatureMap look ;
  int i, tag, x ;
  float y ;
  KEY gene, key ;
  ACEIN in ;			/* for messPrompt() */
  STORE_HANDLE h ;
  Array cds ;

  if (!checkWriteAccess ()) 
    return ;
  if (!curateInitialise ())
    return ;
				
  look = currentFeatureMap ("fMapCurateWindow") ;  /* must be before 
						    graphCreate() */
  graphCreate (TEXT_SCROLL, "CDS curation", 0, 0, 0.4, 0.4) ;

  state = (CurateState*) halloc (sizeof(CurateState), graphHandle());
  graphAssociate (&CURATE_STATE_HOOK, state) ;

  state->key = BOXSEG(box)->parent ;
  state->fmap = look ;
  state->graph = graphActive() ;
  state->handle = h = handleHandleCreate (graphHandle()) ;

  graphRetitle (hprintf (h, "Curate %s:%s", 
			 className(state->key), name(state->key))) ;
  y = 1 ;

  {
    OBJ obj = bsCreate (state->fmap->seqKey) ;

    if (obj)
      {
	bsGetKey (obj, str2tag("Species"), &state->species) ;
        bsDestroy (obj) ;
      }
  }


  /* Id connection object to manage Gene class */
  state->ix = idConnect (0, 0, user,  0, 0) ;


  /* make ->cdsOverlaps and ->geneOverlaps */
  if (!curateOverlaps (state, methodCurated))
    {
      messout ("Sorry, I can't find CDS on this display") ;
      graphDestroy () ;		/* destroys state automatically */
      return ;
    }


  /* check for stop codons -> pseudogene */
  if (fMapGetCDS(state->fmap, state->key, &cds, 0))
    {
      Array tt = NULL ;
      
      if (!(tt = pepGetTranslationTable(state->key, 0)))
	{
	  graphDestroy () ;		/* destroys state automatically */
	  return ;
	}

      /* OK to have stop at end (also protects against !(max%3)) */
      arrayMax(cds) -= 3 ;
      for (i = 0 ; i < arrayMax(cds) ; i += 3)
        if (e_codon(arrp(cds, i, char), tt) == '*')
	  state->isPseudo = TRUE ;
      arrayDestroy (cds) ;
    }
  if (state->isPseudo)
    { int warnBox = graphBoxStart () ;
      graphText ("WARNING: this CDS does not translate", 3, y++) ;
      graphText ("Potential Pseudogene", 11, y++) ;
      graphBoxEnd () ;
      graphBoxDraw (warnBox, BLACK, RED) ;
    }

  if (lexAliasOf(arrp(look->seqInfo, 
		      BOXSEG(box)->data.i, SEQINFO)->method) 
      == methodCurated)
    { graphText (hprintf (h, "Curated CDS %s", name (state->key)), 
		 2, y ) ; 
      y += 1.5 ;
      state->action = NOTHING ;	/* do nothing - just add remark */
      graphToggleEditor ("Kill this CDS", (BOOL*) &state->action, 
			 2, y++) ; /* NB action 1 is KILL */
    }

  else if (!keySetMax (state->cdsOverlaps))
    { state->action = CREATE ;
      graphText ("No overlapping curated CDS", 2, y) ;
      y += 1.5 ;
      tag = graphRadioCreate ("Options:", &state->option, 2, y++) ;
      if (!curateNewGeneName (state) &&
	  !((in = messPrompt ("Can't construct new gene name.  "
			      "Please give one", "","wz", h)) &&
	    (state->newGeneName = strnew (aceInWord (in), h))))
	{ graphDestroy () ;
	  return ;
	}
      graphAddRadioEditor (hprintf (h, "Create new gene %s",
				    state->newGeneName),
			   tag, 1, 3, y++) ;

      keySetDestroy (state->cdsOverlaps) ;
      keySetDestroy (state->geneOverlaps) ;
      curateOverlaps (state, methodHistory) ; /* look for old overlap */
      for (i = 0 ; i < keySetMax (state->geneOverlaps) ; ++i)
	{ ID id ;

	  key = keySet(state->geneOverlaps, i) ;
	  id = idUltimate (state->ix, "Gene", name(key)) ;
	  if (idLive (state->ix, "Gene", id))
	    graphAddRadioEditor (hprintf (h, 
					  "Split gene %s from %s",
					  state->newGeneName, id),
				 tag, i+2, 3, y++) ;
	  else
	    graphAddRadioEditor (hprintf (h, "Resurrect gene %s", 
					  id),
				 tag, i+2, 3, y++) ;

	}

      state->option = 1 ;	/* default is to create new gene */
      graphSetRadioEditor (tag, state->option) ;
    }

  else if (keySetMax (state->geneOverlaps) == 1)
    { state->action = REPLACE ;
      gene = keySet (state->geneOverlaps, 0) ;
      graphText (hprintf (h, "Overlap to gene %s", seqName(gene)), 
		 2, y) ; 
      y += 1.5 ;
      tag = graphRadioCreate ("Options:", &state->option, 2, y++) ;

      if (!(state->newCDSletter = curateNewCDSletter (gene)))
	{ messout ("Sorry, can't find a new CDS letter - aborting") ;
	  graphDestroy () ;
	  return ;
	}
      graphAddRadioEditor (hprintf (h, "Create new CDS %s%c",
				    seqName(gene), 
				    state->newCDSletter), 
			   tag, 1, 3, y++) ;

		/* horrible ambiguity about 'a' or no suffix  */
      if (lexword2key (seqName(gene), &key, classCDS))
	graphAddRadioEditor (hprintf (h, "Replace live CDS %s",
				      name(key)),
			     tag, 0, 3, y++) ;
      else if (lexword2key (cdsName(gene, 'a'), &key, classCDS))
	graphAddRadioEditor (hprintf (h, "Replace live CDS %s",
				      name(key)),
			     tag, 'a', 3, y++) ;
      else	/* there must be a live higher CDS so assume 'a' */
	graphAddRadioEditor (hprintf (h, "Replace dead CDS %sa",
				      seqName(gene)), 
			     tag, 'a', 3, y++) ;

      for (i = 'b' ; i < state->newCDSletter ; ++i)
	if (lexword2key (cdsName(gene, i), &key, classCDS))
	  graphAddRadioEditor (hprintf (h, "Replace live CDS %s",
					name(key)),
			       tag, i, 3, y++) ;
	else
	  graphAddRadioEditor (hprintf (h, "Replace dead CDS %s%c",
					seqName(gene), i), 
			       tag, i, 3, y++) ;

      state->option = 1 ;	/* default is to add new CDS */
      graphSetRadioEditor (tag, state->option) ;
    }

  else				/* multiple genes */
    { state->action = MERGE ;
      graphText ("Overlap to multiple genes:", 2, y++) ;
      for (i = 0, x = 2 ; i < keySetMax (state->geneOverlaps) ; )
	{ graphText (seqName(keySet(state->geneOverlaps, i)), x, y) ;
	  x += strlen (seqName(keySet(state->geneOverlaps, i))) + 1 ;
	}
      y += 1.5 ;
      tag = graphRadioCreate ("Merge these by:", &state->option, 2, y++) ;

      for (i = 0 ; i < keySetMax (state->cdsOverlaps) ; ++i)
	graphAddRadioEditor (hprintf (h, "Replacing CDS %s",
				      name (keySet(state->cdsOverlaps, i))),
			     tag, i+2, 3, y++) ;

      state->option = 2 ;	/* default is first in list */
      graphSetRadioEditor (tag, state->option) ;
    }

				/* action buttons */
  y += 0.5 ;
  graphButton ("Submit", curateSubmit, 3, y) ;
  graphButton ("Cancel", graphDestroy, 10, y) ;
  y += 1.5 ;

#ifdef ATTRIBUTES
				/* attributes */
  { Attribute *at ;

    state->attributes = getAttributes (state->key, h) ;
		/* add an empty remark */
    at = arrayp (state->attributes, arrayMax(state->attributes), 
		 Attribute) ;
    at->tag = "Remark" ;
    at->newValue = (char*) halloc (1024, h) ;
    sprintf (at->newValue, "[%s %s] ", 
	     user, timeShow (timeParse ("today"))) ;
    at->value = strnew (at->newValue, h) ;
    /* Effect of the above is that if the new remark is not edited 
       it will not be written back.  If it is edited, the [ ] will
       be removed, which does nothing unless it really was there
       (but that would be rather meaningless), then the new text 
       added.  OK I think.  */

    for (i = 0 ; i < arrayMax(state->attributes) ; ++i)
      { at = arrp(state->attributes, i, Attribute) ;
        graphTextScrollEditor (at->tag, at->newValue, 1024, 60,
			       1, y, 0) ;
	y += 1.5 ;
      }
  }

	/* put new attribute stuff in a box so we can move it */
  state->newAttributeBox = graphBoxStart() ;
  graphText ("New attribute:", 1, y) ;
  graphBoxFreeMenu (graphMenuTriangle (TRUE, 16, y), 
		    attributeEditor, legalAttributeOpts) ;
  graphBoxEnd () ;

#endif /* ATTRIBUTES */

  graphRedraw() ;
  
  return ;
}

/******** a little callback to add a new attribute editor ********/

static void attributeEditor (KEY key, int z)
{
  CurateState *state ;
  float x1, y1, x2, y2 ;
  Attribute *at ;

  if (!graphAssFind (&CURATE_STATE_HOOK, &state))
    messcrash ("Can't find state in curateSubmit()") ;

  	/* find where the new attribute stuff is, 
	   then shift down to make space */

  graphBoxDim (state->newAttributeBox, &x1, &y1, &x2, &y2) ;
  graphBoxShift (state->newAttributeBox, x1, y1+1.5) ;
  graphColor (WHITE) ;
  graphFillRectangle (x1, y1, x2, y2) ;
  graphColor (BLACK) ;

  at = arrayp(state->attributes, arrayMax(state->attributes), Attribute) ;
  at->tag = legalAttributeOpts[key].text ;
  at->value = (char*) halloc (1024, state->handle) ;
  at->newValue = (char*) halloc (1024, state->handle) ;
  graphTextScrollEditor (at->tag, at->newValue, 1024, 60, 1, y1, 0) ;

  graphRedraw () ;
}

/****************************************************/

static void curateSubmit (void) 
{
  CurateState *state ;
  KEY gene = 0 ;
  KEY key ;
  ID id ;

  if (!graphAssFind (&CURATE_STATE_HOOK, &state))
    messcrash ("Can't find state in curateSubmit()") ;

	/* make sure all editor values are read back */
  graphCheckEditors (graphActive(), FALSE) ;

  if (!graphActivate (state->fmap->graph))
    { messout ("The FMAP graph is no longer present - aborting") ;
      goto end ;
    }

  switch (state->action)
    { 
    case NOTHING:
      break ;
    case CREATE:
      if (state->option == 1)	/* create new gene */
	{ id = idCreate (state->ix, "Gene") ;
	  idAddName (state->ix, "Gene", 
		     id, "Sequence", state->newGeneName) ;
	  lexword2key (id, &gene, classGene) ;
	  if (state->newGeneNumber)
	    setGeneCount (state->newGeneParent, 
			  state->newGeneNumber) ;
	}
      else			/* recover from historical overlap */
	{ gene = keySet(state->geneOverlaps, state->option) ;
	  id = idUltimate (state->ix, "Gene", name(gene)) ;
	  if (idLive (state->ix, "Gene", id))
	    { id = idSplit (state->ix, "Gene", id) ;
	      idAddName (state->ix, "Gene", 
			 id, "Sequence", state->newGeneName) ;
	      lexword2key (id, &gene, classGene) ;
	      if (state->newGeneNumber)
		setGeneCount (state->newGeneParent, 
			      state->newGeneNumber) ;
	    }
	  else
	    { idResurrect (state->ix, "Gene", id) ;
	      lexword2key (id, &gene, classGene) ;
	    }
	}

      state->newCDSletter = 0 ;
      break ;
    case REPLACE:
      gene = keySet (state->geneOverlaps, 0) ;
      if (state->option == 1)
	{ if (state->newCDSletter == 'b' && 
	      lexword2key (seqName(gene), &key, classCDS))
	    if (lexAlias (key, cdsName(gene, 'a'), FALSE, FALSE))
	      messout ("Note side-effect CDS %s renamed to %s",
		       seqName(gene), name(key)) ;
	}
      else
	state->newCDSletter = state->option ;
      break ;
    case KILL:
      if (!keySetMax (state->cdsOverlaps))
	{ if (!messQuery ("This is the only CDS for this gene.  "
			  "Are you sure you want to kill this? "))
	    goto end ;
	}
      if (!convertToHistory (state, state->key))
	goto end ;
      break ;
#ifdef INCOMPLETE
    case SPLIT:
      key = fixCDS (state->key, state->newCDSname) ;
      add ("key to existing parent gene, with remark") ;
      break ;
    case MERGE:
      key = fixCDS (state->key, state->newGeneName) ;
      create ("new gene and add key to it, with remark") ;
      break ;
#endif /* INCOMPLETE */
    default: ;
    }

  if (gene)
    { key = cdsTransfer (state->key, gene, state->newCDSletter) ;
      if (!key || !storeAttributes (key, state->attributes))
	messout ("Curation failed") ;
    }
  else
    if (!storeAttributes (state->key, state->attributes))
      messout ("Curation failed") ;

  fMapPleaseRecompute (state->fmap) ; /* forces recalculate */
  fMapDraw (state->fmap, state->key) ;

 end:
  graphActivate (state->graph) ;
  graphDestroy () ;		/* frees all memory */
}

/******************************************************************/
/** find overlapping CDS and (from them) Genes with given method **/
/******************************************************************/

static BOOL curateOverlaps (CurateState *state, KEY method)
{
  int min, max ;
  int i, j ;
  BOOL foundSomething = FALSE ;
  SEG *iseg, *jseg ;
  KEYSET cdsSet, geneSet = 0 ;	/* =0 to suppress compiler warning */

  state->cdsOverlaps = cdsSet = keySetHandleCreate (state->handle) ;

  if (!fMapFindSegBounds (state->fmap, EXON, &min, &max)) 
    return FALSE ;

/* loop over EXON segs to find exons for state->key, and then for
   each one find overlapping exons with method "curated" */

  iseg = arrp(state->fmap->segs, min, SEG) ;
  for (i = min ; i < max ; ++i, ++iseg)
    if (iseg->parent == state->key)
      { foundSomething = TRUE ;
        jseg = arrp(state->fmap->segs, min, SEG) ;
        for (j = min ; j < max ; ++j, ++jseg)
	  if (jseg->x1 < iseg->x2 && jseg->x2 > iseg->x1 &&
	      jseg->parent != state->key &&
	      lexAliasOf(arrp(state->fmap->seqInfo, jseg->data.i, SEQINFO)->method) == method) /* overlap */
	    keySet(cdsSet,keySetMax(cdsSet)) = jseg->parent ;
      }

/* now make the list unique, find the genes that these correspond to, and sort alphabetically */

  keySetSort (cdsSet) ;
  keySetCompress (cdsSet) ;
				/* ugly bit coming up.... */
  if (method == methodCurated)
    state->geneOverlaps = geneSet = query (cdsSet, "FOLLOW Gene") ;
  else if (method == methodHistory)
    state->geneOverlaps = geneSet = query (cdsSet, "FOLLOW Gene_history") ;
  else
    messcrash ("code inconsistency: curateOverlaps only expects to "
	       "be called with method Curated or History") ;

  arraySort (cdsSet, keySetAlphaOrder) ;
  arraySort (geneSet, keySetAlphaOrder) ;

  if (keySetMax (cdsSet) && !keySetMax (geneSet))
    { messerror ("found overlapping CDS objects but no genes") ;
      return FALSE ;
    }

  return foundSomething ;
}

/***********************************************************/
/****** two functions to satisfy worm Gene/CDS naming ******/
/***********************************************************/

static BOOL curateNewGeneName (CurateState *state)

   /* Return new curated gene name for state->key if possible.
      Use worm system of numbering based on Genomic_canonical.
      Client/server version might get this from the server?
   */

{
  KEY parent = 0 ;
  int n = -1 ;
  char *newName ;

  /* First find the stem of the name.
     Find SEG kseg corresponding to state->key and all
     Genomic_canonical segs.  If kseg inside a gencan then
     use that, else use the one containing the start.
     Assume segs array does not move during this routine,
     so can keep pointers into it.
     NB I started off trying to do this by working on the
     SMap hierarchy, but there is no guarantee where the
     Genomic_canonical entries are.
  */
  { SEG *iseg, *kseg = 0 ;
    int i ;
    Array gencan = arrayCreate (32, SEG*) ;

    iseg = arrp (state->fmap->segs, 0, SEG) ;
    for (i = 0 ; i < arrayMax (state->fmap->segs) ; ++i, ++iseg)
      { if (iseg->key == state->key)
          kseg = iseg ;
        if (class(iseg->key) == classSequence &&
	    bIndexTag (iseg->key, str2tag("Genomic_canonical")))
	  array(gencan, arrayMax(gencan), SEG*) = iseg ;
      }
    if (!kseg)
      { arrayDestroy (gencan) ;
        return FALSE ;
      }
    for (i = 0 ; i < arrayMax(gencan) ; ++i)
      { iseg = arr(gencan, i, SEG*) ;
        if (kseg->x1 >= iseg->x1 && kseg->x2 <= iseg->x2)
	  { parent = iseg->key ;
	    break ;
	  }
      }
    if (!parent)
      for (i = 0 ; i < arrayMax(gencan) ; ++i)
	{ iseg = arr(gencan, i, SEG*) ;
          if (kseg->x1 >= iseg->x1 && kseg->x1 <= iseg->x2)
	    { parent = iseg->key ;
	      break ;
	    }
	}
    arrayDestroy (gencan) ;
    if (!parent) return FALSE ;
  }

  /* Now look for next available number using Gene_count */

  { OBJ obj ;
    if ((obj = bsCreate (parent)))
      { bsGetData (obj, str2tag("Gene_count"), _Int, &n) ;
        bsDestroy (obj) ;
      }
  }
  if (n == -1) return FALSE ;

  newName = hprintf (state->handle, "%s.%d", name(parent), n+1) ;
  if (idFind (state->ix, "Gene", "Sequence", newName))
    return FALSE ;	/* check that it doesn't exist already */

  state->newGeneName = newName ;
  state->newGeneParent = parent ;
  state->newGeneNumber = n+1 ;
  return TRUE ;
}

static void setGeneCount (KEY key, int n)
{
  OBJ obj = bsUpdate (key) ;
  if (obj)
    { bsAddData (obj, str2tag ("Gene_count"), _Int, &n) ;
      bsSave (obj) ;
    }
}

/****************************************************/

static char curateNewCDSletter (KEY gene)

     /* Return new CDS letter suffix for gene.
	Limits the maximum letter to 'z', i.e. 26 CDS per gene.
     */

{
  char result ;
  KEY key ;
  OBJ obj ;
  int n = 0 ;

  /*  Find next free letter from CDS_count */

  if ((obj = bsCreate(gene)))
    { bsGetData (obj, str2tag("CDS_count"), _Int, &n) ;
      bsDestroy (obj) ;
    }

  if (!n) return 0 ;

  result = 'a' + n ;

	/* next check not too big and it doesn't exist already */
  if (result > 'z' || 
      lexword2key (cdsName(gene, result), &key, classCDS))
    return 0 ;

  return result ;
}

/****************************************************/
/***** Attribute stuff - uses .ace dump/parse *******/
/****************************************************/

int wordMatch (char *word, FREEOPT *opts) { /* little utility */
  int n = opts->key ;
  while (n--)
    if (!strcmp (word, (++opts)->text))
      return opts->key ;
  return 0 ;
}

static Array getAttributes (KEY key, STORE_HANDLE handle)
{
  Stack s = stackCreate (512) ;	/* to hold .ace dump */
  Array atts = arrayHandleCreate (16, Attribute, handle) ;

  { ACEOUT out = aceOutCreateToStack (s, 0) ;
    dumpKey (out, key) ;
    aceOutDestroy (out) ;
  }

  { ACEIN in = aceInCreateFromText (stackText (s, 0), 0, 0) ;
    int key ;
    Attribute *att ;
    char *word ;

    aceInCard(in) ;	/* skip initial class : name line */
    while (aceInCard(in))
      if ((word = aceInWord (in)) && 
	  (key = wordMatch (word, legalAttributeOpts)))
	{ att = arrayp (atts, arrayMax(atts), Attribute) ;
	  att->tag = legalAttributeOpts[key].text ;
	  att->value = strnew (aceInPos (in), handle) ;
	  att->newValue = strnew (att->value, handle) ;
	}
    aceInDestroy (in) ;
  }

  stackDestroy (s) ;
  return atts ;
}

/****************************************************/


static BOOL storeAttributes (KEY key, Array atts)
{
  Stack s = stackCreate (512) ;	/* to hold .ace dump */
  BOOL result = TRUE ;

#ifdef ATTRIBUTES

  { ACEOUT out = aceOutCreateToStack (s, 0) ;
    int i ;
    Attribute *att ;

    aceOutPrint (out, "CDS %s\n", name(key)) ;
    for (i = 0 ; i < arrayMax(atts) ; ++i)
      { att = arrp(atts, i, Attribute) ;
        if (strcmp (att->value, att->newValue))
	  { if (*att->value)
	      aceOutPrint (out, "-D %s \"%s\"\n", att->tag, att->value) ;
	    if (*att->newValue)
	      aceOutPrint (out, "%s \"%s\"\n", att->tag, att->newValue) ;
	    else
	      aceOutPrint (out, "%s\n", att->tag) ;
	  }
      }
    aceOutDestroy (out) ;
  }
  
  result = parseBuffer (stackText (s, 0), 0, TRUE) ;

#endif /* ATTRIBUTES */

  stackDestroy (s) ;
  return result ;
}

/****************************************************/
/***** basic CDS curation activities ****************/
/****************************************************/

static BOOL coordsTransfer (KEY source, KEY target, KEY method)
{
  OBJ obj ;
  KEY parent = 0, species = 0, lab = 0 ;
  BOOL startNotFound = FALSE, endNotFound = FALSE ;
  int startOffset = 0 ;
  static Array units = 0 ;

  units = arrayReCreate (units, 32, BSunit) ;

  if (!(obj = bsUpdate (source))) 
    return FALSE ;
  bsGetKeyTags (obj, str2tag("S_Parent"), 0) ;
  bsGetKey (obj, _bsRight, &parent) ;
  if (bsGetArray (obj, str2tag("Source_exons"), units, 2))
    bsPrune (obj) ;
  if ((startNotFound = bsFindTag (obj, str2tag("Start_not_found"))))
    { bsGetData (obj, _bsRight, _Int, &startOffset) ;
      bsPrune (obj) ;
    }
  if ((endNotFound = bsFindTag (obj, str2tag("End_not_found"))))
    bsPrune (obj) ;
  bsSave (obj) ;

  if (!(obj = bsUpdate (parent))) /* includes check parent != 0 */
    return FALSE ;
  if (!bsFindKey2  (obj, str2tag("S_Child"), source, 0))
    { bsDestroy (obj) ;
      return FALSE ;
    }
  bsAddKey (obj, _bsHere, target) ; 
    /* replaces in situ without removing to the right - essential to 
       maintain coordinate information
    */
  bsGetKey (obj, str2tag("Species"), &species) ;
  bsGetKey (obj, str2tag("From_laboratory"), &lab) ;
  bsSave (obj) ;

  if ((obj = bsUpdate (target)))
    { if (arrayMax(units))
        bsAddArray (obj, str2tag("Source_exons"), units, 2) ;
      else if (bsFindTag (obj, str2tag("Source_exons")))
	bsPrune (obj) ;
      bsAddKey (obj, str2tag("Method"), method) ;
      bsAddTag (obj, str2tag("CDS")) ;
      if (species) bsAddKey (obj, str2tag("Species"), species) ;
      if (lab) bsAddKey (obj, str2tag("From_laboratory"), lab) ;
      if (startNotFound)
	{ bsAddTag (obj, str2tag("Start_not_found")) ;
	  if (startOffset) 
	    bsAddData (obj, _bsRight, _Int, &startOffset) ;
	} 
      else if (bsFindTag (obj, str2tag("Start_not_found")))
	bsPrune (obj) ;
      if (endNotFound) 
	bsAddTag (obj, str2tag("End_not_found")) ;
      else if (bsFindTag (obj, str2tag("End_not_found")))
	bsPrune (obj) ;
	
      bsSave (obj) ;
      return TRUE ;
    }
  else
    return FALSE ;
}

static KEY cdsTransfer (KEY source, KEY gene, int letter)
{
  KEY target ;
  OBJ obj ;

  if (!lexaddkey (cdsName (gene, letter), &target, classCDS))
    {	/* make history and copy coords from target */
      char *historyName = hprintf (0, "%s:%s", name(target), 
				   timeShow (timeParse ("now"))) ;
      KEY history ;

      lexaddkey (historyName, &history, classCDS) ;
      if (coordsTransfer (target, history, methodHistory))
	{ if ((obj = bsUpdate (history)))
	    { bsAddKey (obj, str2tag("Gene_history"), gene) ;
	      bsSave (obj) ;
	    }
	}
      else if ((obj = bsUpdate (history)))
	bsKill (obj) ;
    }

/* now copy coords from source to target - fail if this fails */
  if (!coordsTransfer (source, target, methodCurated))
    return 0 ;

/* add connect gene to target, and up cdsCount in gene if novel */
  if ((obj = bsUpdate (gene)))
    { int n = letter ? letter - 'a' + 1 : 1 ;
      int m = 0 ;

      bsAddKey (obj, str2tag("CDS"), target) ;
      bsGetData (obj, str2tag("CDS_count"), _Int, &m) ;
      if (n > m)
	bsAddData (obj, str2tag("CDS_count"), _Int, &n) ;
      bsSave (obj) ;
    }

  return target ;
}

static BOOL convertToHistory (CurateState *state, KEY key)
 /* rename object, move Gene link and reset method */
{
  char *historyName ;
  char *nam ;
  KEY gene ;
  OBJ obj ; 
  
  if (!(obj = bsUpdate (key)))
    return FALSE ;

		/* switch Gene link to Gene_history */

  if (bsGetKey (obj, str2tag("Gene"), &gene))
    { bsPrune (obj) ;
      bsAddKey (obj, str2tag("Gene_history"), gene) ;
    }

		/* switch method to "history" */

  bsAddKey (obj, str2tag("Method"), methodHistory) ;
  bsSave (obj) ;

		/* rename object */

  historyName = hprintf (0, "%s:%s", name(key), 
			 timeShow (timeParse ("now"))) ;
  lexAlias (key, historyName, FALSE, FALSE) ;

		/* if this was the last CDS, remove Sequence name
		   from gene and mark dead if this was last name */

  if (!(obj = bsCreate (gene)))
    return FALSE ;
  if (!bsFindTag (obj, str2tag("CDS")) &&
      (nam = idTypedName (state->ix, "Gene", name(gene),"Sequence")))
    { idRemoveName (state->ix, "Gene", name(gene), "Sequence", nam) ;
      if ((nam = idPublicName (state->ix, "Gene", name(gene))) &&
	  !strcmp (nam, name(gene))) /* i.e. no explicit name left */
	idKill (state->ix, "Gene", name(gene)) ;
    }
  bsDestroy (obj) ;

  return TRUE ;
}

/****************************************************/
/***** A couple of utilities ************************/
/****************************************************/

static char* seqName (KEY key)
{
  char *result = 0 ;
  OBJ obj = bsCreate(key) ;
  if (obj)
    { if (bsGetKey (obj, str2tag("Sequence_name"), &key))
        result = name(key) ;
      bsDestroy (obj) ;
    }
  return result ;
}

static char* cdsName (KEY key, char c)
{
  static char *buf = 0 ;
  messfree (buf) ;		/* OK to messfree(0) */
  if (c)
    buf = hprintf (0, "%s%c", seqName(key), c) ;
  else
    buf = strnew (seqName(key), 0) ;
  return buf ;
}

/********* end of file ***********/
