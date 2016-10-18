/*  File: prefdisp.c
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
 * Description: graphical editor for acedb preference value.
 * Exported functions:
 * HISTORY:
 * Last edited: Nov  2 09:30 2001 (edgrif)
 * Created: Sat May  1 10:40:59 1999 (fw)
 * CVS info:   $Id: prefdisp.c,v 1.13 2002-03-11 16:15:13 srk Exp $
 *-------------------------------------------------------------------
 */

#include <wh/acedb.h>
#include <wh/aceio.h>
#include <wh/display.h>
#include <wh/main.h>
#include <wh/pref_.h>


/************************************************************/

/* must be kept in step with prefsubs.c number of preferences.                */
enum {PREFDISP_MAX = 20} ;

static union
{
  BOOL bval ;
  int ival ;						    /* used for int and colours */
  float fval ;
  char *sval ;
} prefDispValue[PREFDISP_MAX] ;


/************************************************************/


static void prefDispDestroy(void);
static void prefDispApply(void);
static void prefDispSave(void);

/************************************************************/

static MENUOPT prefButtons[] = {
  {graphDestroy,   "Quit"},
  {help,           "Help"},
  {graphPrint,     "Print"}, 
  {menuSpacer,     ""},
  {prefDispApply,  "Apply"},
  {prefDispSave,   "Save"},
  {0, 0} };

static Graph prefsGraph = GRAPH_NULL ;
static BOOL prefChanges = FALSE ;

/************************************************************/

void editPreferences(void)
{
  /* Enables the user to set preferences values in a graphical display */
  int i, line;
  PREFLIST *item;
  Array prefDispList_P;

  if (graphActivate (prefsGraph))
    { 
      graphPop () ;
      return;
    }
  
  prefsGraph = displayCreate("DtPreferenceEditor");
  if (strcmp(displayGetTitle("DtPreferenceEditor"), "DtPreferenceEditor") == 0)
    graphRetitle("Preferences Editor"); /* otherwise keep setting from displays.wrm */
  if (graphHelp(0) == NULL)
    graphHelp("Preferences");
  
  graphMenu (prefButtons);
  
  line = 1;
  prefDispList_P = prefGet();
  graphButtons(prefButtons, 1, line++, 60);

  line += 1.5;

  for(i = 0 ; i < arrayMax(prefDispList_P) ; i++)
    {
      item = arrp(prefDispList_P, i, PREFLIST) ;
      if(item->display)
	{
	  /* Set up the editor field for the value, for some we may be able  */
	  /* to set a callback routine from the preferences that will check  */
	  /* the users value.                                                */
	  if(item->type == PREF_BOOLEAN)
	    {
	      prefDispValue[i].bval = item->value.bval ;
	      graphToggleEditor(item->name, &(prefDispValue[i].bval), 4.0, line++) ;
	    }
	  else if(item->type == PREF_INT)
	    {
	      prefDispValue[i].ival = item->value.ival;
	      if (item->editfunc.ifunc)
		graphIntEditor(item->name, &(prefDispValue[i].ival), 4.0, line++,
			       item->editfunc.ifunc) ;
	      else
		graphIntEditor(item->name, &(prefDispValue[i].ival), 4.0, line++, NULL) ;

	    }
	  else if(item->type == PREF_COLOUR)
	    {
	      prefDispValue[i].ival = item->value.ival;
	      graphColourEditor(item->name, " ", &(prefDispValue[i].ival), 4.0, line++) ;
	    }
	  else if(item->type == PREF_FLOAT)
	    {
	      prefDispValue[i].fval = item->value.fval;
	      if (item->editfunc.ffunc)
		graphFloatEditor(item->name, &(prefDispValue[i].fval), 4.0, line++,
				 item->editfunc.ffunc) ;
	      else
		graphFloatEditor(item->name, &(prefDispValue[i].fval), 4.0, line++, NULL) ;
	    }
	  else if(item->type == PREF_STRING)
	    {
	      prefDispValue[i].sval = messalloc(item->string_length+1);
	      strcpy(prefDispValue[i].sval, item->value.sval) ;
	      if (item->editfunc.sfunc)
		graphWordEditor(item->name, prefDispValue[i].sval, item->string_length, 4.0, line++,
				item->editfunc.sfunc) ;
	      else
		graphWordEditor(item->name, prefDispValue[i].sval, item->string_length, 4.0, line++,
				NULL) ;
	    }
	  line++;
	}    
    }
  graphRegister(DESTROY, prefDispDestroy);

  graphTextBounds(0, line) ;

  graphRedraw();

  return ;
} /* editPreferences */

/************************************************************/

/* Set a preference value from the code as if a user had set it. The void *  */
/* will be interpreted according to the value of preftype. Returns TRUE if   */
/* preference could set be set, FALSE otherwise.                             */
/*                                                                           */
BOOL setPreference(char *prefname, PrefFieldType preftype, void *prefvalue)
{
  BOOL result = FALSE ;
  int i;
  PREFLIST *item;
  BOOL changed = FALSE;
  Array prefList = prefGet();
  Graph old_active ;

  old_active = graphActive() ;

  editPreferences();

  /* Find the preference and set it.                                         */
  for(i = 0 ; i < arrayMax(prefList) ; i++)
    {
      item = arrp(prefList, i, PREFLIST) ;

      if (strcmp(item->name, prefname) == 0)
	{
	  if (preftype == PREF_BOOLEAN)
	    {
	      if (*(BOOL *)prefvalue != prefDispValue[i].bval)
		{
		  prefDispValue[i].bval = *(BOOL *)prefvalue ;
		  changed = TRUE ;
		}

	    }
	  else if (preftype == PREF_INT || preftype == PREF_COLOUR)
	    {
	      if (*(int *)prefvalue != prefDispValue[i].ival)
		{
		  changed = TRUE;
		  prefDispValue[i].ival = *(int *)prefvalue ;
		}
	    }
	  else if (preftype == PREF_FLOAT)
	    {
	      if (*(float *)prefvalue != prefDispValue[i].fval)
		{
		  changed = TRUE;
		  prefDispValue[i].fval = *(float *)prefvalue ;
		}
	    }
	  else if (preftype == PREF_STRING)
	    {
	      if (strcmp((char *)prefvalue, prefDispValue[i].sval) != 0)
		{
		  changed = TRUE ;
		  if (prefDispValue[i].sval != NULL)
		    messfree(prefDispValue[i].sval) ;
		  prefDispValue[i].sval = strnew((char *)prefvalue, 0) ;
		}
	    }
	}
    }


  if (changed)
    {
      result = TRUE ; 
      prefChanges = TRUE ;
      prefDispApply() ;					    /* Makes sure above change is applied. */
    }

  return result ;
}



/************************************************************/

/* Private functions.                                                        */

static void prefDispDestroy(void)
{
  
  if (prefChanges && messQuery("Do you want to save the preferences ?"))
    prefDispSave();

  prefsGraph = 0;
  
  return;
} /* prefDispDestroy */


static void prefDispApply(void)
{
  int i;
  PREFLIST *item;
  BOOL changed = FALSE;
  Array prefList = prefGet();
  Graph old_active ;


  old_active = graphActive() ;

  /* Find the type, check its value using the deffunc callback if possible,  */
  /* call the pref action callback if possible.                              */
  /* Note that the edit function callbacks should be supplied for a prefer-  */
  /* ence and should do the checking, but you never know.                    */
  for(i=0;i<arrayMax(prefList);i++)
    {
      item = arrp(prefList,i,PREFLIST);
      if(item->type == PREF_BOOLEAN)
	{
	  if (item->value.bval != prefDispValue[i].bval)
	    {
	      changed = TRUE;
	      item->value.bval = prefDispValue[i].bval;

	      if (item->deffunc.bfunc)
		item->value.bval = item->deffunc.bfunc(item->value.bval) ;

	      if (item->func.bfunc)
		item->func.bfunc(item->value.bval) ;

	      prefDispValue[i].bval = item->value.bval ;
	    }
	}
      else if(item->type == PREF_INT || item->type == PREF_COLOUR)
	{
	  if (item->value.ival != prefDispValue[i].ival)
	    {
	      changed = TRUE;
	      item->value.ival = prefDispValue[i].ival;

	      if (item->deffunc.ifunc)
		item->value.ival = item->deffunc.ifunc(item->value.ival) ;

	      if (item->func.ifunc)
		item->func.ifunc(item->value.ival) ;

	      prefDispValue[i].ival = item->value.ival ;
	    }
	}
      else if(item->type == PREF_FLOAT)
	{
	  if (item->value.fval != prefDispValue[i].fval)
	    {
	      changed = TRUE;
	      item->value.fval = prefDispValue[i].fval;

	      if (item->deffunc.ffunc)
		item->value.fval = item->deffunc.ffunc(item->value.fval) ;

	      if (item->func.ffunc)
		item->func.ffunc(item->value.fval) ;

	      prefDispValue[i].fval = item->value.fval ;
	    }
	}
      else if(item->type == PREF_STRING)
	{
	  if (item->value.fval != prefDispValue[i].fval)
	    {
	      changed = TRUE;
	      item->value.fval = prefDispValue[i].fval;

	      if (item->deffunc.ffunc)
		item->value.fval = item->deffunc.ffunc(item->value.fval) ;

	      if (item->func.ffunc)
		item->func.ffunc(item->value.fval) ;

	      prefDispValue[i].fval = item->value.fval ;
	    }
	}
    }
  if (changed || prefChanges)
    prefChanges = TRUE;

  /* Update the editor window.                                               */
  graphActivate(prefsGraph) ;
  graphUpdateEditors(prefsGraph) ;
  graphActivate(old_active) ;
  
  return;
} /* prefDispApply */


static void prefDispSave (void)
{
  messStatus ("Saving prefs...");

  prefDispApply();
  prefSave();
  prefChanges = FALSE;

  return ;
} /* prefDispSave */


/************************** eof *****************************/
