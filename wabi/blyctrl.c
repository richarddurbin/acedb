
/*  File: blyctrl.c
 *  Author: mieg
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
 * Description: Graphic interface to acembly
 * HISTORY:
 * Last edited: Apr 22 13:28 1999 (fw)
 * * Nov 19 15:00 1998 (edgrif): Fixed callback func. dec. for
 *              graphTextScrollEditor.
 * Created: April 1997 (mieg)
 *-------------------------------------------------------------------
 */

/* @(#)blyctrl.c	1.2 5/23/97 */

#define ARRAY_CHECK
#define MALLOC_CHECK



#include "acedb.h"
#include "whooks/systags.h"
#include "query.h"
#include "bs.h"
#include "session.h"

#include "display.h"
#include "pick.h"
#include "fmap.h"

/* #define DEBUG */

/**********************************************************************/

enum  MODE { UNDEFINED = 0, SHOTGUN, MUTANT, CDNA, VECTOR} ;

#define NN 30

typedef struct BlyControlLookStruct 
{
  magic_t *magic ;		/* == &BlyControlLook_MAGIC */
  int level ;
  int mode ;
  char *buffer ;
  KEY wildSeq ;
  int wildLength ;
  int state ; 
  char *title ;
  STORE_HANDLE handle ; 
  BOOL isShotgun ;
  mytime_t date ;  
  KEYSET seqVectors ;
  char vname[NN] ;
  char leftSite[NN], rightSite[NN] ; int nn1 ;
} *BlyControlLook ;


static Graph blyControlGraph = 0 ;
static BlyControlLook globalLook = 0 ;

static int state = 0 ;

static magic_t BlyControlLook_MAGIC = "BlyControlLook";


/**********************************************************************/

static void blyctrlControlDraw (BlyControlLook look) ;
static BlyControlLook getBlyControlLook (char *title) ;

/**********************************************************************/

static void blyctrlControlDestroy (void)
{
  BlyControlLook look = getBlyControlLook ("blyctrlControlDestroy") ;
  look->magic = 0 ;
  handleDestroy (look->handle) ;
  if (look->seqVectors)
    keySetDestroy (look->seqVectors) ;
  messfree (look);

  blyControlGraph = 0 ;
  globalLook = 0 ;
}


/***************************************************************************************/

static void blyctrlInit (BlyControlLook look)
{
  KEYSET ks ;
  KEY clone = 0, mode = 0  ;
  OBJ obj = 0 ;
  
  look->state = 0 ;
  look->title = "unc-32" ;
  look->date = timeNow() ;
  look->isShotgun = TRUE ;

  ks = query (0, "FIND Clone Main_clone") ;
  if (!keySetMax(ks) || !(clone = keySet(ks, 0)) || !(obj = bsCreate (clone)))
    goto abort ;
  keySetDestroy (ks) ;
  look->seqVectors = queryKey (clone, "FOLLOW Sequencing_Vector") ;
  
  if (bsFindTag (obj, str2tag("Project_type")))
    bsGetKeyTags (obj, _bsRight, &mode) ;
  if (pickMatch ("*mutation*", name(mode)))
    look->mode = CDNA ;
  else if (pickMatch ("*sequence*", name(mode)))
    look->mode = SHOTGUN ;
  else if (pickMatch ("*cdna*", name(mode)))
    look->mode = CDNA ;
  else
    look->mode = UNDEFINED ;

  bsGetData (obj, str2tag("Level"), _Int, &look->level) ;

abort:
  bsDestroy (obj) ;
  keySetDestroy (ks) ;  
}

/***************************************************************************************/
/***************************************************************************************/
/****************** Bly control graph, actual calls to bly*********************************************/
/***************************************************************************************/

static void popBlyControl(void)
{
  if (graphActivate (blyControlGraph))
    graphPop() ;
}

static BlyControlLook getBlyControlLook (char *title)
{
  BlyControlLook look = globalLook ; 

  
  if (!graphActivate (blyControlGraph))
    { blyControlGraph = 0 ; globalLook = 0 ; return 0 ; }
  
  graphCheckEditors(blyControlGraph, TRUE) ;
  return look ;
}

/******************************/

static void blyctrlRedraw (void)
{
  BlyControlLook look = getBlyControlLook ("blyctrlRedraw") ;

  popBlyControl() ;
  blyctrlControlDraw (look) ;
}

/******************************/

static void beforeDefCompute (void)
{
  extern void defCompute(void) ;
  graphDestroy() ;
  defCompute () ;
}

/******************************/

static void beforeDefSimple (void)
{
  graphDestroy() ;
  fMapTraceReassembleAll () ;	/* Reassemble from scratch */
}

/*****************************************************************/

static MENUOPT blyControlMenu[] =
{
  /*
 { graphDestroy, "Quit" },
  { help, "Help" }, 
  */
  { beforeDefSimple,       "Assembly: simple interface"},
  { beforeDefCompute,       "Assembly: full toolbox"}, 
  { 0, 0 }
} ;


/*****************************************************************/
static BOOL checkTC (int tc)
{
  if (tc < 20 || tc > 99)
    return FALSE ;
  return TRUE ;
}

static void createVector(void)
{
}

static void selectVector(void)
{
}

static void showVectors(void)
{
  KEYSET ks = query (0,"FIND Vector") ;
  keySetNewDisplay (ks,"Vectors") ;
}


static void vectorSelect(BlyControlLook look)
{
  int line = 8 ; look-> nn1 = 40 ;

  memset (look->vname,0, NN) ;
  memset (look->leftSite,0, NN) ;
  memset (look->rightSite,0, NN) ;
  graphButton ("List Known Vectors", showVectors, 4, line) ; line += 2 ;
  graphButton ("Select a Known Vectors", selectVector, 4, line) ; line += 2 ;
  graphButton ("Create a new vector", createVector, 4, line) ; line += 2 ;
  graphWordEditor ("Vector name",look->vname,18, 4, line++,0) ;
  graphWordEditor ("upStream bases",look->leftSite,28, 4, line++,0) ;
  graphWordEditor ("downStream bases",look->rightSite,28, 4, line++,0) ;
  graphIntEditor ("Primer to insert distance", &look->nn1, 4, line++, 0) ; 
}


/***********************************************************************************/

static void sSh (BlyControlLook look)
{
  look->mode = SHOTGUN ;
  look->level = 0 ;
  blyctrlControlDraw (look) ;
}

static void scDNA (BlyControlLook look)
{  
  look->mode = SHOTGUN ;
  look->level = 0 ;
  blyctrlControlDraw (look) ;
}

static void sMuOk (void)
{
  BlyControlLook look = getBlyControlLook ("ppS") ;
  
  switch (look->level)
    {
    case 0:
      look->level = 1 ;  /* should check */
      break ;
    } 

  blyctrlControlDraw (look) ;
}


static void sMuChange (void)
{
  BlyControlLook look = getBlyControlLook ("ppS") ;
  
  switch (look->level)
    {
    case 1:
      look->level = 0 ; 
      break ;
    } 

  blyctrlControlDraw (look) ;
}

static BOOL dEntry (char *unused1, int unused2)
{
  return TRUE ;
}

static void acefileselect (void)
{
  BlyControlLook look = getBlyControlLook ("acefileselect") ;
  blyctrlControlDraw (look) ;
}


static void dnafileselect (void)
{
  BlyControlLook look = getBlyControlLook ("dnafileselect") ;
  blyctrlControlDraw (look) ;
}


static void dbselect (void)
{
  graphClear () ;
  graphText ("hello world", 3, 3) ;

  graphRedraw () ;
}


static void sMu (BlyControlLook look)
{
  int line = 3 ;
  
  switch (look->level)
    {
    case 0:
      if (!look->buffer)
	look->buffer = messalloc (32001) ;
      graphText ("Please enter the wild type sequence", 2, line++) ;
      graphText ("You may:", 2, line++) ;
      graphTextScrollEditor ("enter the dna here", look->buffer, 32000, 32, 4, line, dEntry) ;
      line += 1.3 ; graphText ("or select a file containing plain dna", 4, line) ;
      graphButton (" ", dnafileselect, 60, line) ;
      line += 1.3 ; graphText ("or select a fasta file", 4, line) ;
      graphButton (" ", dnafileselect, 60, line) ;
      line += 1.3 ; graphText ("or select a .ace file", 4, line) ;
      graphButton (" ", acefileselect, 60, line) ;
      line += 1.3 ; graphText ("or select among the sequences known it this database", 4, line) ;
      graphButton (" ", dbselect, 60, line) ;
      line += 2 ; 
      graphButton ("OK", sMuOk, 8, line) ;
      break ;
    case 1:
      graphText 
	( messprintf ("The wild type sequence is called %s", name(look->wildSeq)), 
	  2, line++) ;
      graphText 
	( messprintf ("its length is %d base pairs", look->wildLength),
	  2, line++) ;
      graphButton ("Change", sMuChange, 8, line) ;
      
      /*
      
  wild ? ;
    couper coller ou lire un fichier fasta ou un .ace ;

pcr  || cloned fragments

  pcr:
nomenclature ;

    allele+3.date
  
   f122+3.30av
   e2310-7.4aug
   f111.toto.4avr
   f122+3.1.30avr


cloned:
   vector ?
   primer ?

   f122.clone.4avr 
   

SCF_dir ?


==========


  */
    default:
	break ;
    }
}


/***********************************************************************************/
/* Undefined Project */

static void ppM (void)
{
  BlyControlLook look = getBlyControlLook ("ppM") ;
  
  look->mode = MUTANT ;
  look->level = 0 ;
  blyctrlControlDraw (look) ;
}

static void ppS (void)
{
  BlyControlLook look = getBlyControlLook ("ppS") ;
  
  look->mode = SHOTGUN ;
  look->level = 0 ;
  blyctrlControlDraw (look) ;
}

static void ppD (void)
{
  BlyControlLook look = getBlyControlLook ("ppD") ;
  
  look->mode = CDNA ;
  look->level = 0 ;
  blyctrlControlDraw (look) ;
}

static MENUOPT pleaseMenu[]  = 
{
  {ppS, "Sequence a new area"},
  {ppD, "Sequence cDNA and align on genomic"},
  {ppM, "Sequence mutants and compare to wild type"},
  {0, 0}
} ;

/***********************************************************************************/

static void blyctrlControlDraw (BlyControlLook look)
{ 
  int line = 2 ;

  graphClear () ;
  
  switch (look->mode)
    {
    case UNDEFINED:
      graphButtons (blyControlMenu, 2, line, 20) ;
      /*
      graphText ("Please define your project", 2, line++) ;
      graphButtons (pleaseMenu, 2, line, 20) ;
      */
       break ;
    case SHOTGUN:
      sSh (look) ;
      break ;
    case MUTANT:
      sMu (look) ;
      break ;
    case CDNA:
      scDNA(look) ;
      break ;
    }
  graphRedraw () ;
}

void blyControl (void)
{
  Graph old = graphActive () ;
  BlyControlLook look ;

  if (graphActivate (blyControlGraph))
    { graphPop () ;
    return ;
    }
  state = 0 ;
  blyControlGraph = graphCreate (TEXT_SCROLL, "Project parameters", 
				 0, 0, 0.6, 0.5) ;
  graphHelp("Acembly.control") ;
  graphRegister (DESTROY, blyctrlControlDestroy) ;
  graphRegister (RESIZE, blyctrlRedraw) ;
  graphMenu (blyControlMenu) ;
  
  look = (BlyControlLook) messalloc(sizeof(struct BlyControlLookStruct)) ;
  globalLook = look ;
  look->magic = &BlyControlLook_MAGIC ;
  look->handle = handleCreate() ;
  blyctrlInit(look) ;
  blyctrlControlDraw (look) ;
  graphActivate (old) ;
}

/***************************************************************************************/
/***************************************************************************************/
