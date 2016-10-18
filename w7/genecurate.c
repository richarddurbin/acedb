/*  File: genecurate.c
 *  Author: Richard Durbin (rd@sanger.ac.uk)
 *  Copyright (C) Richard Durbin, Genome Research Limited, 2002
 *-------------------------------------------------------------------
 * This file is part of the ACEDB genome database package
 *
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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111
 * or see the on-line version at http://www.gnu.org/copyleft/gpl.txt
 * ------------------------------------------------------------------
 * Description: temporary standalone implementation of genecurate.h
 * Exported functions:
 * HISTORY:
 * Last edited: Apr 19 18:43 2002 (edgrif)
 * Created: Tue Jan 22 23:30:14 2002 (rd)
 * CVS info: $Id: genecurate.c,v 1.2 2002-04-19 17:52:37 edgrif Exp $
 *-------------------------------------------------------------------
 */

/* models required by this file 
   NB some changes from RD

?Gene	Version UNIQUE Int
	Name	Public_name UNIQUE ?Gene_name XREF Public_name
			// not directly editable
		CGC_name UNIQUE ?Gene_name XREF CGC_name
		Sequence_name UNIQUE ?Gene_name XREF Sequence_name
		Other_name ?Gene_name XREF Other_name #Evidence
	History	Version_change Int UNIQUE DateType UNIQUE ?Person #Gene_history_action
		Merged_into UNIQUE ?Gene XREF Acquires_merge
		Acquires_merge ?Gene XREF Merged_into
		Split_from UNIQUE ?Gene XREF Split_into
		Split_into ?Gene XREF Split_from_parent
	Live  // remove live to make gene "dead"
	Species ?Species

#Gene_history_action Event Created
                           Killed
	                   Resurrected
	                   Merged_into UNIQUE ?Gene
	                   Acquires_merge UNIQUE ?Gene
	                   Split_from UNIQUE ?Gene
                           Split_into UNIQUE ?Gene
			   Name_change
                     Name CGC_name UNIQUE Text
                          Sequence_name UNIQUE Text
		          Other_name UNIQUE Text

#Evidence Paper ?Paper
	  Person ?Person

// RD 020122 made ?Gene_name tag2 to track how name used

?Gene_name Gene Public_name ?Gene
		CGC_name ?Gene
		Sequence_name ?Gene
		Other_name ?Gene

****************************/

#include "acedb.h"
#include "lex.h"
#include "bs.h"
#include "whooks/systags.h"
#include "genecurate.h"
#include "pick.h"
#include "main.h"		/* for checkWriteAccess() */
#include "bindex.h"
#include "keyset.h"

struct GeneConnectionStruct {
  KEY user ;
  KEY species ;
} ;

static int   classGene = 0 ;
static int   classGeneName = 0 ;
static int   classPerson = 0 ;
static char* idFormat = "WB:gn%06d" ;
static KEY   typeTag[3] ;

static void geneCurateInitialise (void) ;
static KEY  newGene (void) ;
static BOOL setPublicName (KEY gene) ;
static void updateHistory (KEY gene, KEY user,
			   char* event, KEY target, 
			   GeneNameType type, char* name) ;

/*************************************************************/
/****** initialise a connection ******************************/
/*************************************************************/

GeneConnection  geneConnection (char *host, char* user, KEY species,
				STORE_HANDLE handle)
{
  GeneConnection gx ;

  geneCurateInitialise () ;

  if (!user || !*user)
    { messerror ("geneConnection() requires a user string") ;
      return 0 ;
    }

  gx = (GeneConnection) halloc(sizeof(struct GeneConnectionStruct),
			       handle) ;
  lexaddkey (user, &gx->user, classPerson) ;
  gx->species = species ;
  return gx ;
}

/*************************************************************/
/****** primary actions to create, merge, split, kill etc. ***/
/*************************************************************/

KEY geneCreate (GeneConnection gx, GeneNameType type, char* name)
{
  KEY gene, nameKey ;
  OBJ obj ;

  if (!gx || !checkWriteAccess ()) 
    return 0 ;
  if (type > OTHER_NAME) 
    { messerror ("Bad type %d in geneCreate()", type) ; return 0 ; }

  gene = newGene() ;
  obj = bsUpdate (gene) ;
  lexaddkey (name, &nameKey, classGeneName) ;
  bsAddKey (obj, typeTag[type], nameKey) ;
  if (gx->species) bsAddKey (obj, str2tag("Species"), gx->species) ;
  bsAddTag (obj, str2tag("Live")) ;
  bsSave (obj) ;
  
  updateHistory (gene, gx->user, "Created", 0, type, name) ;
  setPublicName (gene) ;

  return gene ;
}

/**************************************************************/

BOOL geneKill (GeneConnection gx, KEY gene)
{
  OBJ obj ;

  if (!gx || !checkWriteAccess () || 
      class(gene) != classGene || !(obj = bsUpdate(gene))) 
    return FALSE ;

  if (bsFindTag (obj, str2tag("Live")))
    { bsRemove (obj) ;
      bsSave (obj) ;
      updateHistory (gene, gx->user, "Killed", 0, 0, 0) ;
      return TRUE ;
    }
  else
    { bsDestroy (obj) ;
      return FALSE ;
    }
}

/**************************************************************/

BOOL geneResurrect (GeneConnection gx, KEY gene)
{
  OBJ obj ;

  if (!gx || !checkWriteAccess () || 
      class(gene) != classGene || !(obj = bsUpdate(gene))) 
    return FALSE ;

  if (bsAddTag (obj, str2tag("Live")))
    { bsSave (obj) ;
      updateHistory (gene, gx->user, "Resurrected", 0, 0, 0) ;
      return TRUE ;
    }
  else
    { bsDestroy (obj) ;
      return FALSE ;
    }
}

/**************************************************************/

BOOL geneMerge (GeneConnection gx, KEY gene, KEY target)
{
  OBJ obj ;

  if (!gx || !checkWriteAccess () || class(target) != classGene 
      || !bIndexTag (target, str2tag("Live")) || 
      class(gene) != classGene || !(obj = bsUpdate(gene)))
    return FALSE ;

  if (bsFindTag (obj, str2tag("Live")))
    { bsRemove (obj) ;
      bsSave (obj) ;
      updateHistory (gene, gx->user, "Merged_into", target, 0, 0) ;
      updateHistory (target, gx->user, "Acquires_merge", gene, 0, 0) ;
      return TRUE ;
    }
  else
    { bsDestroy (obj) ;
      return FALSE ;
    }
}

/**************************************************************/

KEY geneSplit (GeneConnection gx, KEY gene, 
	       GeneNameType type, char *name)
{
  KEY child, nameKey ;
  OBJ obj ;

  if (!gx || !checkWriteAccess () || class(gene) != classGene ||
      !bIndexTag (gene, str2tag("Live")))
    return FALSE ;
  if (type > OTHER_NAME) 
    { messerror ("Bad type %d in geneCreate()", type) ; return 0 ; }

  child = newGene() ;
  obj = bsUpdate (child) ;
  lexaddkey (name, &nameKey, classGeneName) ;
  bsAddKey (obj, typeTag[type], nameKey) ;
  if (gx->species) bsAddKey (obj, str2tag("Species"), gx->species) ;
  bsAddTag (obj, str2tag("Live")) ;
  bsSave (obj) ;
  updateHistory (child, gx->user, "Split_from", gene, type, name) ;
  updateHistory (gene, gx->user, "Split_into", child, 0, 0) ;
  setPublicName (child) ;

  return child ;
}

/*************************************************************/
/****** name functions ***************************************/
/*************************************************************/

static KEYSET geneList = 0 ;
static GeneNameType lastType ;
static char* lastName ;

static void makeGeneList (GeneNameType type, char *name) ;

int geneNameCount (GeneConnection gx,  GeneNameType type, char *name)
{
  if (!gx) return 0 ;

  if (!lastName || lastType != type || strcmp(lastName, name))
    makeGeneList (type, name) ;

  return keySetMax(geneList) ;
}

/****************************************/

KEY geneNameGet (GeneConnection gx, 
		 GeneNameType type, char *name, int n)
{
  if (!gx) return 0 ;

  if (!lastName || lastType != type || strcmp(lastName, name))
    makeGeneList (type, name) ;

  if (n >= 0 && n < keySetMax(geneList))
    return keySet(geneList, n) ;
  else
    return 0 ; 
}

/****************************************/

static void makeGeneList (GeneNameType type, char *name)
{
  KEY geneName ;
  OBJ obj ;
  int i ;
  static Array units = 0 ;

  geneList = keySetReCreate (geneList) ;

  if (!lexword2key (name, &geneName, classGeneName) ||
      !(obj = bsCreate (geneName)))
    return ;
  units = arrayReCreate (units, 16, BSunit) ;
  bsGetArray (obj, str2tag("Gene"), units, 2) ;	/* tag 2 */
  for (i = 1 ; i < arrayMax(units) ; i += 2)
    keySet(geneList, keySetMax(geneList)) = arr(units, i, BSunit).k ;
  bsDestroy (obj) ;
  
  lastType = type ;
  if (lastName) messfree (lastName) ;
  lastName = strnew (name, 0) ;	/* local copy */
}

/*************************************************************/
/****** a little utility to follow a chain of merges *********/
/*************************************************************/

BOOL geneDescendant (GeneConnection gx, KEY *gene)
{
  OBJ obj = 0 ;

  while ((obj = bsCreate (*gene)))
    { if (bsFindTag (obj, str2tag ("Live")))
        { bsDestroy (obj) ;
	  return TRUE ;
	}
      if (!bsGetKey (obj, str2tag ("Merged_into"), gene))
	{ bsDestroy (obj) ;
	  return FALSE ;
	}
      bsDestroy (obj) ;
    }

  return FALSE ;
}

/*************************************************************/
/****** local functions **************************************/
/*************************************************************/

static void geneCurateInitialise (void)
{
  int isDone = FALSE ;

  if (isDone) return ;

  if (!(classGene = pickWord2Class ("Gene")))
    messcrash ("No Gene class") ;
  if (!(classGeneName = pickWord2Class ("Gene_name")))
    messcrash ("No Gene_name class") ;
  if (!(classPerson = pickWord2Class ("Person")))
    messcrash ("No Person class") ;
  typeTag[0] = str2tag("CGC_name") ;
  typeTag[1] = str2tag("Sequence_name") ;
  typeTag[2] = str2tag("Other_name") ;
  isDone = TRUE ;
}

/*************************************************************/

static KEY newGene (void)
{
  KEY gene ;
  int n = 0 ;

  while (!lexaddkey (messprintf (idFormat, ++n), &gene, classGene)) ;
  return gene ;
}

/*************************************************************/

static BOOL setPublicName (KEY gene) /* return if name changes */
{
  OBJ obj = bsUpdate (gene) ;
  KEY name ;
  BOOL result = FALSE ;

  if (!obj) 
    return FALSE ;
  if (bsGetKey (obj, str2tag("CGC_name"), &name))
    result = bsAddKey (obj, str2tag("Public_name"), name) ;
  else if (bsGetKey (obj, str2tag("Sequence_name"), &name))
    result = bsAddKey (obj, str2tag("Public_name"), name) ;
  else if (bsGetKey (obj, str2tag("Other_name"), &name))
    result = bsAddKey (obj, str2tag("Public_name"), name) ;
  
  bsSave (obj) ;
  return result ;
}

/*************************************************************/

static void updateHistory (KEY gene, KEY user,
			   char* event, KEY target, 
			   GeneNameType type, char* newName)
{
  OBJ obj = bsUpdate (gene) ;
  int version = 0 ;
  mytime_t time = timeParse ("now") ;

  if (!obj) return ;

  bsGetData (obj, str2tag("Version"), _Int, &version) ;
  ++version ;
  if (!bsAddData (obj, str2tag("Version"), _Int, &version))
    messerror ("updateHistory could not update Version") ;

  if (bsAddData (obj, str2tag("Version_change"), _Int, &version) &&
      bsAddData (obj, _bsRight, _DateType, &time) &&
      bsAddKey (obj, _bsRight, user) &&
      bsPushObj (obj))
    { if (!bsAddTag (obj, str2tag(event)))
        messerror ("Unknown event %s in updateHistory()", event) ;
      if (target && !bsAddKey (obj, _bsRight, target))
        messerror ("Couldn't add target in updateHistory()", event) ;
      if (!bsAddData (obj, typeTag[type], _Text, newName))
        messerror ("Couldn't add nameType %s in updateHistory()", 
		   name(typeTag[type])) ;
    }
  else
    messerror ("updateHistory could not add Version_change") ;

  if (!strcmp (event, "Merged_into"))
    bsAddKey (obj, str2tag(event), target) ;
  else if (!strcmp (event, "Split_from"))
    bsAddKey (obj, str2tag(event), target) ;
		/* don't need reciprocals since done by XREF */

  bsSave (obj) ;
}

/********************* end of file ******************/
