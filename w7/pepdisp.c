/*  File: pepdisp.c
 *  Author: Richard Durbin (rd@sanger.ac.uk)
 *  Copyright (C) J Thierry-Mieg and R Durbin, 1994
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
 * Description: peptide display - using colcontrol.c
 * Exported functions:
 * HISTORY:
 * Last edited: Dec 21 09:21 2009 (edgrif)
 * Created: Wed Jun  1 15:05:00 1994 (rd)
 * CVS info:   $Id: pepdisp.c,v 1.53 2009-12-21 10:49:37 edgrif Exp $
 *-------------------------------------------------------------------
 */

/*
#define ARRAY_CHECK
#define PEP_DEBUG
*/

#include <wh/acedb.h>
#include <wh/aceio.h>
#include <wh/lex.h>
#include <wh/bs.h>
#include <wh/peptide.h>
#include <wh/a.h>
#include <whooks/classes.h>
#include <whooks/sysclass.h>
#include <whooks/systags.h>
#include <whooks/tags.h>
#include <wh/display.h>
#include <wh/graphAcedbInterface.h> /* for controlMakeColumns() */
#include <wh/pepdisp.h>
#include <wh/graph.h>
#include <wh/gex.h>
#include <wh/dotter.h>

/************************************************************/

static MAPCONTROL pepMapDoMap (COLCONTROL control, KEY key, KEY from, int nmaps, BOOL CDS_only) ;
static void callDotter (void);

static Array allWindows = 0;

/************************************************************/

static void pepMapControlRemove(void *cp)
{ int i;
  COLCONTROL c = *((COLCONTROL *)cp);
  for ( i=0; i<arrayMax(allWindows); i++)
    if (arr(allWindows, i, COLCONTROL) == c)
      arr(allWindows, i, COLCONTROL) = 0;
}

/***************** configuration data ******************/
static void setActiveForKey(KEY key)
{
  MAPCONTROL map = currentMapControl();
  PEPLOOK *look = map->look;  

  look->block = FALSE;
  controlDraw();
}

static void toggleHeader()
{
  MAPCONTROL map = currentMapControl();
  COLCONTROL control = map->control;

  control->hideHeader = !control->hideHeader;
  controlDraw();
}

static void activeZoneGet()
{
  MAPCONTROL map = currentMapControl();
  PEPLOOK *look = map->look;  
  look->block = TRUE;
  displayBlock(setActiveForKey,"Double click key to set active zone");
}

static void none()
{
}


static COLOUROPT analysisButton[] =
{
  { (ColouredButtonFunc)none, 0, BLACK, WHITE, "Analysis...", 0 }
};

static struct ProtoStruct resSetProt ;


static struct ProtoStruct pepSequenceColumn = {
  0, pepSequenceCreate, 0, "pepSequence", 0, FALSE, 0, 0, 0
} ;

static struct ProtoStruct pepFeatureColumn = {
  0, pepFeatureCreate, 0, "pepFeature", 0, FALSE, FeatureConvert, 0, 0
} ;

static struct ProtoStruct hydrophobColumn = {
  0, hydrophobCreate, 0, "Hydrophobicity", 0, FALSE, 0, 0, 0
} ;

static struct ProtoStruct activeZoneColumn = {
  0, activeZoneCreate, 0, "pepActiveZone", 0, FALSE, 0, 0, 0
} ;

static struct ProtoStruct homolColumn = {
  0, homolCreate, 0, "Homol", 0, FALSE, homolConvert, 0, 0
} ;

static struct ProtoStruct homolNameColumn = {
  0, homolNameCreate, 0, "Homol_Name", 0, FALSE, homolConvert, 0, 0
} ;

static COLDEFAULT defaultColSet =
{
  { &mapLocatorColumn, TRUE,  "Locator" },
  { &mapScaleColumn, TRUE,    "Scale" },
  { &activeZoneColumn, TRUE,  "Active Zone" },
  { &resSetProt, FALSE,       "Stupid test" },
  { &pepSequenceColumn, TRUE, "Peptide sequence" },
  { &hydrophobColumn, TRUE,   "Hydrophobicity" },
  { &homolColumn, TRUE,       "Blastp" },
  { &homolColumn, TRUE,       "Blastx" },
  { &homolNameColumn, TRUE,   "Blastx names" },
  { &homolColumn, TRUE,       "Pfam-hmmls+Pfam-hmmfs" }, /* Don't use underscore since it's illegal to use in method names (esr) */
  { &homolColumn, FALSE,      "Pfam-hmmfs" },
  { 0, 0, 0 }
};

static COLPROTO pepProts[] = { 
  &mapLocatorColumn,
  &mapScaleColumn,
  &activeZoneColumn,
  &pepFeatureColumn,
  &pepSequenceColumn,
  &homolColumn,
  &homolNameColumn,
  &hydrophobColumn,
  0
} ;

static MENUOPT mainMenu[] =
{
  { graphDestroy,    "Quit" },
  { help,            "Help" },
  { graphPrint,      "Print Screen" },
  { displayPreserve, "Preserve" },
  { toggleHeader,    "Toggle Header" },
  { 0, 0 }
} ;

/************* little callback functions ***************/

static void pepFinalise (void *m)
{
  PEPLOOK *look = (PEPLOOK *)(m) ;
				/* what I do when this dies */
  if (arrayExists(look->pep))
    arrayDestroy (look->pep) ;
}

static void dumpActiveZone (void)
     /* menu/button func */
{
  MAPCONTROL map = currentMapControl();
  PEPLOOK *look = map->look;
  
  if(look->pep)
    { 
      /* fw - really to stdout ?? context-less menu-func can't get
       * at any other output-context. 
       * was there a freeSetFile somewhere else ? XXXXXXXXX */
      ACEOUT dump_out = aceOutCreateToStdout (0);
      pepDumpFastA (look->pep, look->activeStart, look->activeEnd, name(map->key), dump_out, '\0') ;
      aceOutDestroy (dump_out);
    }
} /* dumpActiveZone */

static void setActiveZone(char *cp)
{
  MAPCONTROL map = currentMapControl();
  PEPLOOK *look = map->look;
  int x1,x2;
  BOOL changed = FALSE;

  if(sscanf(cp,"%d %d",&x1,&x2)==2){ /*x1-- ; x2-- ;  No zero mhmp 23.10.97*/
    if(x1 >= 0 && x1 <= arrayMax(look->pep) && x2 >= 0 && x2 <= arrayMax(look->pep)){
      if(x1 < x2){
	look->activeStart = x1;
	look->activeEnd = x2;
      }
      else{
	look->activeStart = x2;
	look->activeEnd = x1;
      }
      controlDraw();
      changed = TRUE;
    }
    else
      graphMessage(messprintf("integers must be between 0 and %d. You entered \"%s\"",arrayMax(look->pep),cp));
  }
  else{
    graphMessage(messprintf("format needs to be int int. Not \"%s\"",cp));
  }
  if(!changed){
    strncpy(look->activeText,messprintf("%d %d",look->activeStart,look->activeEnd),MAXACTIVETEXT);
    graphBoxDraw(look->activeregionBox,-1,-1);   
  }
}

static float header (MAPCONTROL map) 
{ 
  COLCONTROL control = map->control;
  PEPLOOK *look = map->look;
  static MENUOPT activeMenu[]=
    {
      { dumpActiveZone, "Fasta dump of Active zone" },
      { 0,0 }
    };
  float y;
  int x,box;

  look->aboutBox = graphBoxStart();
  graphTextFormat(BOLD);
  if (look->titleKey)
    { x = strlen (name(look->titleKey)) ;
      graphText (name(look->titleKey),1,0.5) ;
    }
  else
    { x = strlen (name(map->key)) ;
      graphText (name(map->key),1,0.5) ;
    }
  graphTextFormat (PLAIN_FORMAT) ;
  graphBoxEnd();
  box = graphButton("Active Zone:",activeZoneGet,x+2,0.5);
  graphBoxMenu(box, activeMenu);

  strncpy(look->activeText,messprintf("%d %d",look->activeStart,look->activeEnd),MAXACTIVETEXT);
  look->activeregionBox = graphBoxStart();
  graphTextEntry(look->activeText,MAXACTIVETEXT-1,x+15.5,0.5,setActiveZone);
  graphBoxEnd();
  graphBoxDraw(look->activeregionBox,BLACK,YELLOW);


  if (map->noButtons) /* may suppress header buttons for WWW */
    y = 1.4;
  else
    y = controlDrawButtons (map, x+28, 0.5, control->graphWidth) ; 
  
  look->messageBox = graphBoxStart();
  graphTextPtr(look->messageText,1,y + 0.25, MAXMESSAGETEXT-1);
  graphBoxEnd();
  graphBoxDraw (look->messageBox, BLACK, PALECYAN);

  return y + 2.0 ;			/* return number of lines used */
}

static void pepMapHeaderPick(MAPCONTROL map, int box,double x,double y)
{
  PEPLOOK *look = map->look;

  if(box == look->aboutBox)
      display(map->key, 0, "TREE");
}

/*************** entry point ****************/
/* The app data in our case points to a flag that says whether to translate  */
/* the whole of the DNA or just the CDS part of it.                          */
/*                                                                           */
BOOL pepDisplay(KEY key, KEY from, BOOL isOldGraph, void *app_data)
{
  COLCONTROL control ;
  Array pep ;
  KEYSET maps = keySetCreate();
  KEY _VManyPep, _VView, mm;
  BOOL haveControl = FALSE ;
  int i;
  PepDisplayData *pep_create = (PepDisplayData *)app_data ;
  BOOL CDS_only ;

  CDS_only = pep_create ? pep_create->CDS_only : FALSE ;

#if !defined(PEP_DEBUG)
  return FALSE ;
#endif

  lexaddkey("ManyPep", &mm,_VMainClasses);
  _VManyPep = KEYKEY(mm);
  lexaddkey("View", &mm, _VMainClasses);
  _VView = KEYKEY(mm);

  if (!_VProtein)
    { KEY key ;
      lexaddkey ("Protein", &key, _VMainClasses) ; 
      _VProtein = KEYKEY(key) ;
    }

  if(class(key) != _VManyPep)
    {
      if (class(key) == _VPeptide)
	lexword2key (name(key), &key, _VProtein) ;
    
      {
	OBJ obj ;

	if (class(key) != _VProtein && (obj = bsCreate (key)))
	  {
	    bsGetKey (obj, str2tag ("Corresponding_protein"), &key) ;
	    bsDestroy (obj) ;
	  }
      }
   
      if (!(pep = peptideTranslate(key, CDS_only)) || arrayMax(pep) < 2)
	{
	  if (pep)
	    arrayDestroy (pep) ;
	  return FALSE ;
	}
      keySet(maps,keySetMax(maps))=key;
      from = key;
    }
  else
    {
      KEY k;
      OBJ obj = bsCreate(key);
      if(obj)
	{
	  if(bsGetKey(obj,str2tag("Protein"),&k))
	    do
	      {
		OBJ obj2 ;
		if (class(k) != _VProtein && (obj2 = bsCreate (k)))
		  {
		    bsGetKey (obj2, str2tag ("Corresponding_protein"), &k) ;
		    bsDestroy (obj2) ;
		  }
    
		if (!(pep = peptideTranslate(k, CDS_only)) || arrayMax(pep) < 2)
		  {
		    if (pep) arrayDestroy (pep) ;
		    return FALSE ;
		  }
		else
		  arrayDestroy(pep);

		keySet(maps,keySetMax(maps)) = k;
	      } while(bsGetKey(obj,_bsDown,&k));
	  bsDestroy(obj);
	}
    }

  if (keySetMax(maps) == 0) /* failed to find any maps */
    {
      keySetDestroy(maps);
      return FALSE ;
    }

  /* Remove any non-graphic maps from the list and display them as trees */
  keySetSort(maps); /* for KeySetFind */
  for (i=0; i<arrayMax(maps); i++)
    {
      KEY map = arr(maps, i, KEY);
      OBJ obj = bsCreate(map);
      if (obj && bsFindTag(obj, _Non_graphic))
        {
	  keySetRemove(maps, map);
          display(map, from, "TREE");
        }
      bsDestroy(obj);
    }

  if (keySetMax(maps) == 0)
    {
      keySetDestroy(maps);
      return FALSE;
    }

  if (!allWindows)
    allWindows = arrayCreate(10, COLCONTROL);


  /* Stage two, see if we have any window's open with precisely the right
     maps already displayed, this is inspired by ace3. Note that if we have
     views tied down to any maps, we don't re-use, we re-draw */
  if (isOldGraph)
    { 
      for (i=0 ; i<arrayMax(allWindows); i++)
        { int j;
          COLCONTROL c = arr(allWindows, i, COLCONTROL);
          if (!c)
            continue; /* deleted one */
          if (arrayMax(c->maps) != keySetMax(maps))
            continue; /* different number, failed */
          for (j=0; j<arrayMax(c->maps); j++)
            { MAPCONTROL m = arr(c->maps, j, MAPCONTROL);
              if (!keySetFind(maps, m->key, 0))
                break;

            }
          if (j == arrayMax(c->maps))
            break; /* found the match */
        }
  
      if (i != arrayMax(allWindows)) /* found match */
        { /*float centre;*/
          control = arr(allWindows, i, COLCONTROL);
          graphActivate(control->graph);
	  /*          for (i=0; i<arrayMax(control->maps); i++)
            { MAPCONTROL map = arr(control->maps, i, MAPCONTROL);
              if (getCentre(from, map->key, &centre))
                { map->centre = centre;
                  mapControlCursorSet(map, centre);
                }
            }*/
          haveControl = TRUE ;
        }
    }


  /* Stage three make a new control, in new or existing window, then display
     the maps in it */
  if (!haveControl)
    {
      char buff[1000]; /* for the name */
      COLCONTROL *cp;

      buff[0] = 0;
      for (i=0; i<arrayMax(maps); i++)
        { KEY mk = arr(maps, i, KEY);
          strcat(buff, messprintf("%s ", name(mk)));
        }

      control = colControlCreate(isOldGraph, buff, "PEPMAP");
      array(allWindows, arrayMax(allWindows), COLCONTROL) = control;

      /* The next bit ensures that the pointer to the control in allWindows
         is removed when it goes away */
      cp = (COLCONTROL *) handleAlloc(pepMapControlRemove, 
                                      control->handle,
                                      sizeof(COLCONTROL));
      *cp = control;
      
      for (i=0; i<arrayMax(maps); i++)
        {
	  KEY mk = arr(maps, i, KEY);
	  pepMapDoMap(control, mk, from, arrayMax(maps), CDS_only);
	}
    }
  
  control->from = from;
  controlDraw();
  keySetDestroy(maps);

  return TRUE;
}


static MAPCONTROL pepMapDoMap (COLCONTROL control, KEY key, KEY from, int nmaps, BOOL CDS_only)
{
  PEPLOOK *look ;
  MAPCONTROL map = mapControlCreate(control,0) ;
  static MENUOPT Analysis[]=
    {
      { callDotter, "Self-dotplot in Dotter " },
      { dumpActiveZone, "Fasta dump of Active zone" },
      { 0,0 }
    };

  look = (PEPLOOK*) halloc (sizeof(PEPLOOK), map->handle) ;
  blockSetFinalise ((char*)look, pepFinalise) ;
  /* must give map handle, since per map */
  map->look = (void*) look ;
  look->map = map ;
  

  if(from)
    look->titleKey = from;
  
  if(class(key) == str2tag("Peptide"))
    lexword2key(name(key), &key, _VProtein);

  if (!(look->pep = peptideTranslate(key, CDS_only)) || arrayMax(look->pep) < 2)
    {
      if (look->pep)
	arrayDestroy (look->pep) ;
      display (key, from, "TREE") ;
      return map ;
    }

  *look->activeText = 0;

  map->displayName = "PEPMAP" ;		/* unused, but perhaps useful? */
  map->name = name(key) ;
  map->key = key ;
  map->menu = mainMenu ;
  map->headerPick = pepMapHeaderPick;
  map->drawHeader = header ;	/* to draw header */
  map->convertTrans = 0 ;	/* called each time you Draw */
  map->keyboard = 0 ;		/* called on keystrokes in your map */
  map->topMargin = 2.5 ;		/* size of header */
  map->prototypes = pepProts ;
  map->hasCursor = TRUE ;
  map->hasProjectionLines = TRUE ;
  map->viewTag = str2tag("Pepmap");
  map->min = 1;
  map->max = arrayMax (look->pep) ;
  map->centre = map->max/2 ;
  look->activeStart = 0;
  look->activeEnd = map->max;

  controlAddMap (control, map);
  controlAddButton(map,analysisButton ,Analysis);
  controlConvertPerm(map);
  /* mhmp 24.10.97 */
  map->mag = (control->graphHeight-4)/ (map->max - 1) ;	/* approx Whole */
  controlMakeColumns (map, defaultColSet, 0, FALSE) ;

  control->from = from ;
  /*  map->mag = (control->graphHeight-4)/ (map->max - 1) ;	approx Whole */
  controlDraw () ;

  return map ;
}

/*********************************************************************/
/************************ homol columns ******************************/


#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
/* This seems not to be used, no idea why its here....                       */

typedef struct { 
  KEY seq ;		/* subject, i.e. target sequence for match */
  KEY meth ;		/* type of match, e.g. BLASTP - controls display etc. */
  float score ;
  int qstart ;		/* query start and end, i.e. in self */
  int qend ;
  int sstart ;		/* subject start and end */
  int send ;
} HOMOL ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */




static void callDotter(void)
{
  MAPCONTROL currentMap = currentMapControl();
  PEPLOOK *look = (PEPLOOK*)currentMap->look;
  char *seq, *cp;
  int i;
  
  if (!look->pep)
    return;
  
  seq = messalloc(currentMap->max+1);
  cp = seq;
  for (i = 0; i < currentMap->max; i++)
    *cp++ = pepDecodeChar[(int)(arr(look->pep, i, char))];
  dotter ('P',
          0,
          currentMap->name,
          seq,
          0,
          currentMap->name,
          seq,
          0,
          0,
          0,
          0,
          0,
          0,
          0,
          0,
          0,
          0,
          0,
	  0);

  return ;
}


