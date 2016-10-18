/*  File: prefsubs.c
 *  Author: Ian Longden
 *  Copyright (C) J Thierry-Mieg and R Durbin, 1999
 * -------------------------------------------------------------------
 * Acedb is free software; you can redistribute it and/or
 *
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
 * Description: Handles user preferences for acedb which are stored in
 *              the file $HOME/.acedbrc
 *              
 * Exported functions: See wh/pref.h
 * HISTORY:
 * Last edited: Apr  7 12:37 2008 (edgrif)
 * * Jul  5 13:36 2001 (edgrif): Added code to support callback funcs
 *              which means we don't have to hard code them in this
 *              routine as was starting to happen.
 * * May  1 10:57 1999 (fw): moved all graphical code to prefdisp.c
 * Created: Sat May  1 10:56:58 1999 (fw)
 * CVS info:   $Id: prefsubs.c,v 1.46 2008-04-07 12:43:20 edgrif Exp $
 *-------------------------------------------------------------------
 */

#include <wh/acedb.h>
#ifdef __CYGWIN__
#include <windows.h>
#endif
#include <wh/aceio.h>
#include <whooks/systags.h>
#include <whooks/sysclass.h>
#include <wh/lex.h>
#include <wh/pick.h>
#include <wh/session.h>
#include <wh/bs.h>
#include <wh/query.h>
#include <wh/dbpath.h>
#include <wh/pref_.h>

#include <wh/graph.h>
#include <wh/gex.h>


/************************************************************/

static void copySystemPrefs(Array existing_pref_list);
static Array makePrefDefaults(void);
static void prefAdd(Array prefList, PREFLIST *item);


/************************************************************/
/*
il-27/01/97

 Examples of preferences file lines
  
_NAME -T               |set Name to TRUE
_NAME1 -F              |set Name1 to FALSE
_NAME2 -I 2            |set Integer Name2 to 2.
_NAME3 -R 5.5          |set Float Name3 to 5.5.
_NAME4 -C 1            |set colour NAME4 to WHITE (by number)
_NAME4 -C WHITE        |set colour NAME4 to WHITE (by name)
_NAME5 -S string       |set NAME5 to string

where "_" must be the first character on the line followed by the preferences name
then (tab or space delimited) a "-" the type and for int,floats and colours the value.
Any other lines will be ignored.
To use this later on in the program use prefValue(NAME) which will return the BOOL value.
prefInt(NAME) will return the value for NAME etc.
*/


/* We hold a global array of preferences and also record globally whether we managed
 * to read the users .acedbrc file. */
static Array prefList_G = NULL ;
static BOOL user_prefs_read_G = FALSE ;


/************************************************************/
#ifdef __CYGWIN__

/* For cygwin we use the registry, under the key
   HKEY_CURRENT_USER/Software/acedb.org/Acedb/Prefs

   Names of values are [type]name where type is
   b boolean
   C colour
   F float
   i integer
   S string

   Values if keys gives values, with colours as numbers and
   T and F for booleans.
*/

static BOOL prefReadReg(HKEY key, Array prefList)
{
  BOOL result = FALSE ;
  PREFLIST item;
  DWORD buffsiz = 255;
  char buff[256];
  int keys = 0;
  DWORD prefsiz = 255;
  char prefbuff[256];
  
  while (ERROR_SUCCESS == RegEnumValue(key, keys++, buff, &buffsiz,
				       NULL, NULL, prefbuff, &prefsiz))
    { 
      strncpy(item.name, buff+1, 32);
      
      switch (*buff)
	{
	case 'B': case 'b':
	  item.value.bval = (*prefbuff == 'T');
	  item.type = PREF_BOOLEAN;
	  item.deffunc.bfunc = NULL ;
	  item.editfunc.ifunc = NULL ;
	  item.func.bfunc = NULL ;
	  prefAdd(prefList, &item);	
	  break;
	case 'i': case 'I':
	  if(sscanf(prefbuff,"%d",&item.value.ival))
	    {
	      item.type=PREF_INT;
	      item.deffunc.ifunc = NULL ;
	      item.editfunc.ifunc = NULL ;
	      item.func.ifunc = NULL ;
	      prefAdd(prefList, &item);
	    }
	  break;
	case 'C': case 'c':
	  if(sscanf(prefbuff,"%d",&item.value.ival))
	    {
	      item.type=PREF_COLOUR;
	      item.deffunc.ifunc = NULL ;
	      item.editfunc.ifunc = NULL ;
	      item.func.ifunc = NULL ;
	      prefAdd(prefList, &item);
	    }
	  break;
	case 'R': case 'r':
	  if(sscanf(prefbuff,"%f",&item.value.fval))
	    {
	      item.type=PREF_FLOAT;
	      item.deffunc.ffunc = NULL ;
	      item.editfunc.ffunc = NULL ;
	      item.func.ffunc = NULL ;
	      prefAdd(prefList, &item);
	    }
	  break;
	  
	default:
	  break;
	}
      /* reset these before looping - they're input parameters too. */
      buffsiz = 255; 
      prefsiz = 255;
    }

  if (keys > 0)
    result = TRUE ;

  return result ;
}

static HKEY prefPrefRegRead(void)    
{
  BOOL success = FALSE;
  HKEY software, acedb_org, acedb, prefs;
  
  if (RegOpenKeyEx(HKEY_CURRENT_USER,
		   "SOFTWARE", 0L, 
		   KEY_EXECUTE,
		   &software) == ERROR_SUCCESS)
    {
      if (RegOpenKeyEx(software,
		       "acedb.org", 0L, 
		       KEY_EXECUTE,
		       &acedb_org) == ERROR_SUCCESS)
	{ 
	  if (RegOpenKeyEx(acedb_org,
			   "Acedb", 0L, 
			   KEY_EXECUTE,
			   &acedb) == ERROR_SUCCESS)
	    { 
	      if (RegOpenKeyEx(acedb,
			       "Prefs", 0L, 
			       KEY_READ,
			       &prefs) == ERROR_SUCCESS)
		{ 
		  success = TRUE;
		}
	      RegCloseKey(acedb);
	    }
	  RegCloseKey(acedb_org);
	}
      RegCloseKey(software);
    }

  if (success)
    return prefs;
  
  return 0;
  
}

void prefInit(void)
{
  HKEY prefKey;

  if (!user_prefs_read_G)
    {
      prefList_G = makePrefDefaults() ;
      
      if ((prefKey = prefPrefRegRead()) && (prefReadReg(prefKey, prefList_G)))
	user_prefs_read_G = TRUE ;
    }
  return ;
}	

static HKEY prefPrefRegWrite(void)    
{
  BOOL success = FALSE;
  HKEY software, acedb_org, acedb, prefs;
  
  if (RegOpenKeyEx(HKEY_CURRENT_USER,
		   "SOFTWARE", 0L, 
		   KEY_EXECUTE,
		   &software) == ERROR_SUCCESS)
    {
      if (RegCreateKeyEx(software,
			 "acedb.org", 0L, NULL, 
			 REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, 
			 &acedb_org, NULL) == ERROR_SUCCESS)
	{ 
	  if (RegCreateKeyEx(acedb_org,
			     "Acedb", 0L, NULL, 
			     REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, 
			     &acedb, NULL) == ERROR_SUCCESS)
	    	    { 
	      if (RegCreateKeyEx(acedb,
				 "Prefs", 0L, NULL, 
				 REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, 
				 NULL, &prefs, NULL) == ERROR_SUCCESS)
		{ 
		  success = TRUE;
		}
	      RegCloseKey(acedb);
	    }
	  RegCloseKey(acedb_org);
	}
      RegCloseKey(software);
    }

  if (success)
    return prefs;
  
  return 0;
  
}
    
void prefSave(void)
{
  HKEY prefKey = prefPrefRegWrite();
  char namebuff[255];
  char valbuff[255];
  PREFLIST *item; 
  int i;
  
  if (prefKey) 
    {
      for(i=0;i<arrayMax(prefList_G);i++)
	{
	   item = arrp(prefList_G,i,PREFLIST);
	   if (item->type == PREF_BOOLEAN)
	     {
	       char *val;
	       *namebuff = 'B';
	       strcpy(namebuff+1, item->name);
	       strcpy(valbuff, item->value.bval ? "T" : "F");
	     }
	   else if (item->type == PREF_INT)
	     {
	       *namebuff = 'I';
	       strcpy(namebuff+1, item->name);
	       sprintf(valbuff, "%d", item->value.ival);
	     }
	   else if (item->type == PREF_COLOUR)
	     {
	       *namebuff = 'C';
	       strcpy(namebuff+1, item->name);
	       sprintf(valbuff, "%d", item->value.ival); 
	     }
	   else if (item->type == PREF_FLOAT)
	     {
	       *namebuff = 'R';
	       strcpy(namebuff+1, item->name);
	       sprintf(valbuff, "%f", item->value.fval); 
	     }
	   
	   RegSetValueEx(prefKey, namebuff, 0L,
			 REG_SZ, valbuff, strlen(valbuff)+1);
	}
      
      RegCloseKey(prefKey);
    
    }
}

#else /* !__CYGWIN__ */

/* This function does the first initialisation of user preferences and tries
 * to read the users .acedbrc file, it _must_ be called before prefInitDB()
 * which calls prefInitDB() */
void prefInit(void)    
{
  ACEIN pref_in = NULL;
  char filename[MAXPATHLEN] = "";
   
  /* This can't happen at the moment I think.... */
  if (prefList_G)
    {
      arrayDestroy(prefList_G);
      prefList_G = NULL ;
      user_prefs_read_G = FALSE ;
    }

  prefList_G = makePrefDefaults() ;
   
  if(getenv("HOME"))
    {
      strcat(filename,getenv("HOME"));
      strcat(filename, "/");
    }
  strcat(filename,".acedbrc");
  
  if (filCheckName(filename,"","r")
      && (pref_in = aceInCreateFromFile (filename, "r", "", 0)))
    {
      prefRead(pref_in, prefList_G) ;
      aceInDestroy (pref_in);
      user_prefs_read_G = TRUE ;
    }


  /* We'd probably like to check preferences for validity here but we can't  */
  /* because this is a chicken and egg, this is where we create the struct   */
  /* to hold the checking routine callbacks so they can't have been          */
  /* registered yet....this means its possible for someone to edit their     */
  /* preference file and potentially screw things up. If they do then I      */
  /* guess that's their problem.                                             */


  return ;
}


void prefSave(void)
{
  char filename[FIL_BUFFER_SIZE];
  FILE *prefFil = 0;
  int i=0;
  PREFLIST *item;
  
  if(getenv("HOME"))
    {
      strcpy(filename,getenv("HOME"));
      strcat(filename,"/.acedbrc");
      prefFil = filopen(filename,"","w");
    }


  if (prefFil)
    {
      fprintf(prefFil,"This file contains the preferences for ACEDB.\n");
      fprintf(prefFil,"Do not edit by hand. Use the preference editor from the main menu.\n");
  
      for(i = 0 ; i < arrayMax(prefList_G) ; i++)
	{
	  item = arrp(prefList_G,i,PREFLIST) ;
	  if(item->type == PREF_BOOLEAN)
	    fprintf(prefFil,"_%s -%s\n",item->name,
		    (item->value.bval ? "TRUE":"FALSE")) ;
	  else if(item->type  == PREF_INT)
	    fprintf(prefFil,"_%s -I %d\n",item->name,item->value.ival) ;
	  else if(item->type  == PREF_COLOUR)
	    fprintf(prefFil,"_%s -C %d\n",item->name,item->value.ival) ;
	  else if(item->type  == PREF_FLOAT)
	    fprintf(prefFil,"_%s -R %f\n",item->name,item->value.fval) ;
	  else if(item->type  == PREF_STRING)
	    fprintf(prefFil, "_%s -S %s\n", item->name, item->value.sval) ;
	}
      
      filclose(prefFil) ;
    }
  
  return ;
}

#endif /* !__CYGWIN__ */

/* If there was no user .acedbrc file, there may be an acedbrc file in the
 * wspec directory so we should try to load that. */
void prefReadDBPrefs()
{
  messAssert(prefList_G != NULL) ;

  if (!user_prefs_read_G)
    copySystemPrefs(prefList_G) ;

  return ;
}

Array prefGet(void)
{
  return prefList_G ;
}

/* This creates the new defaults if none exist. 
 * You will have to add here any new preferences that you want to add.
 * Remember to add them to the .acedbrc file,
 * the $ACEDB/wspec/acedbrc or remove the two completely.
 * To have them take effect. */
static Array makePrefDefaults(void)
{
  PREFLIST item;
  Array prefList;

  prefList  = arrayCreate (10, PREFLIST) ; 
  
  strcpy(item.name,"OLD_STYLE_MAIN_WINDOW");
  item.type = PREF_BOOLEAN; 
  item.value.bval = FALSE ;
  item.deffunc.bfunc = NULL ;
  item.editfunc.ifunc = NULL ;				    /* no bool version. */
  item.func.bfunc = NULL ;
  item.display = TRUE;
  array(prefList,arrayMax(prefList),PREFLIST) = item;

  strcpy(item.name,"AUTO_DISPLAY");
  item.type = PREF_BOOLEAN; 
  item.value.bval = TRUE ;
  item.deffunc.bfunc = NULL ;
  item.editfunc.ifunc = NULL ;				    /* no bool version. */
  item.func.bfunc = NULL ;
  item.display = TRUE;
  array(prefList,arrayMax(prefList),PREFLIST) = item;

  /* Should acedb splash screen be displayed at start up. */
  strcpy(item.name, WINDOW_TITLE_PREFIX) ;
  item.type = PREF_BOOLEAN ;
  item.value.bval = FALSE ;
  item.deffunc.bfunc = NULL ;
  item.editfunc.ifunc = NULL ;
  item.func.bfunc = NULL ;
  item.display = TRUE ;
  array(prefList,arrayMax(prefList),PREFLIST) = item ;

  /* Should acedb splash screen be displayed at start up. */
  strcpy(item.name, SPLASH_SCREEN) ;
  item.type = PREF_BOOLEAN ;
  item.value.bval = TRUE ;
  item.deffunc.bfunc = NULL ;
  item.editfunc.ifunc = NULL ;
  item.func.bfunc = NULL ;
  item.display = TRUE ;
  array(prefList,arrayMax(prefList),PREFLIST) = item ;

  strcpy(item.name,"HORIZONTAL_TREE");
  item.type = PREF_BOOLEAN; 
  item.value.bval = TRUE;
  item.deffunc.bfunc = NULL ;
  item.editfunc.ifunc = NULL ;				    /* no bool version. */
  item.func.bfunc = NULL ;
  item.display = TRUE;
  array(prefList,arrayMax(prefList),PREFLIST) = item;

  strcpy(item.name,"ACTION_MENU_IN_TREE_DISPLAY");
  item.type = PREF_BOOLEAN;
  item.value.bval = FALSE;
  item.deffunc.bfunc = NULL ;
  item.editfunc.ifunc = NULL ;				    /* no bool version. */
  item.func.bfunc = NULL ;
  item.display = TRUE;
  array(prefList,arrayMax(prefList),PREFLIST) = item;

  strcpy(item.name,"NO_MESSAGE_WHEN_DISPLAY_BLOCK");
  item.type = PREF_BOOLEAN; 
  item.value.bval = FALSE;
  item.deffunc.bfunc = NULL ;
  item.editfunc.ifunc = NULL ;				    /* no bool version. */
  item.func.bfunc = NULL ;
  item.display = TRUE;
  array(prefList,arrayMax(prefList),PREFLIST) = item;

  /* not used anywhere MAKE SURE YOU ADD ALL THE CALLBACK FUNC POINTER IF
     YOU REINSTATE ANY OF THIS....
  strcpy(item.name,"SHOW_EMPTY_OBJECTS");
  item.type = PREF_BOOLEAN; 
  item.value.bval = FALSE;
  item.display = TRUE;
  array(prefList,arrayMax(prefList),PREFLIST) = item;

  strcpy(item.name,"SHOW_ALIASES");
  item.type = PREF_BOOLEAN; 
  item.value.bval = FALSE;
  item.display = TRUE;
  array(prefList,arrayMax(prefList),PREFLIST) = item;
  */

  strcpy(item.name,"TAG_COLOUR_IN_TREE_DISPLAY");
  item.type = PREF_COLOUR; 
  item.value.ival = BROWN ;
  item.deffunc.ifunc = NULL ;
  item.editfunc.ifunc = NULL ;				    /* no bool version. */
  item.func.ifunc = NULL ;
  item.display = TRUE;
  array(prefList,arrayMax(prefList),PREFLIST) = item;

  strcpy(item.name, BLIXEM_SCOPE);
  item.type = PREF_INT;
  item.value.ival = 40000;
  item.deffunc.ifunc = NULL ;
  item.editfunc.ifunc = NULL ;
  item.func.ifunc = NULL ;
  item.display = TRUE;
  array(prefList,arrayMax(prefList),PREFLIST) = item;

  strcpy(item.name, BLIXEM_HOMOL_MAX);
  item.type = PREF_INT;
  item.value.ival = 0;
  item.deffunc.ifunc = NULL ;
  item.editfunc.ifunc = NULL ;
  item.func.ifunc = NULL ;
  item.display = TRUE;
  array(prefList,arrayMax(prefList),PREFLIST) = item;

  /* Call blixem as a separate process.                                      */
  strcpy(item.name, BLIXEM_EXTERNAL);
  item.type = PREF_BOOLEAN ;
  item.value.bval = FALSE ;
  item.deffunc.bfunc = NULL ;
  item.editfunc.ifunc = NULL ;
  item.func.bfunc = NULL ;
  item.display = TRUE ;
  array(prefList,arrayMax(prefList),PREFLIST) = item ;

  /* If blixem is called as a separate process it is called from a cover script
   * and this is the scripts path. */
  strcpy(item.name, BLIXEM_SCRIPT);
  item.type = PREF_STRING;
  item.value.sval = strnew("", 0) ;
  item.string_length = 300 ;				    /* Should be long enough. */
  item.deffunc.sfunc = NULL ;
  item.editfunc.sfunc = NULL ;
  item.func.sfunc = NULL ;
  item.display = TRUE;
  array(prefList,arrayMax(prefList),PREFLIST) = item;

  /* If blixem is called as a separate process tell it not to erase its temp. input files. */
  strcpy(item.name, BLIXEM_TEMPFILES);
  item.type = PREF_BOOLEAN ;
  item.value.bval = FALSE ;
  item.deffunc.bfunc = NULL ;
  item.editfunc.ifunc = NULL ;
  item.func.bfunc = NULL ;
  item.display = FALSE ;
  array(prefList,arrayMax(prefList),PREFLIST) = item ;

  /* Next three tell blixem to use pfetch instead of efetch, to do this may  */
  /* require setting of machine address and port for pfetch server.          */
  strcpy(item.name, BLIXEM_PFETCH) ;
  item.type = PREF_BOOLEAN ;
  item.value.bval = FALSE ;
  item.deffunc.bfunc = NULL ;
  item.editfunc.ifunc = NULL ;
  item.func.bfunc = NULL ;
  item.display = TRUE ;
  array(prefList, arrayMax(prefList), PREFLIST) = item ;


  strcpy(item.name, BLIXEM_NETID);
  item.type = PREF_STRING;
  item.value.sval = strnew(BLIXEM_DEFAULT_NETID, 0) ;
  item.string_length = 20 ;				    /* long enough for dotted decimal and
							       most hostnames ?? */
  item.deffunc.sfunc = NULL ;
  item.editfunc.sfunc = NULL ;
  item.func.sfunc = NULL ;
  item.display = TRUE;
  array(prefList,arrayMax(prefList),PREFLIST) = item;

  strcpy(item.name, BLIXEM_PORT_NUMBER);
  item.type = PREF_INT;
  item.value.ival = BLIXEM_DEFAULT_PORT_NUMBER ;
  item.deffunc.ifunc = NULL ;
  item.editfunc.ifunc = NULL ;
  item.func.ifunc = NULL ;
  item.display = TRUE;
  array(prefList,arrayMax(prefList),PREFLIST) = item;


  /* User defaults for scrolling message window.                             */
  strcpy(item.name, USE_MSG_LIST);
  item.type = PREF_BOOLEAN ;
  item.value.bval = FALSE ;
  item.deffunc.bfunc = NULL ;
  item.editfunc.ifunc = NULL ;				    /* no bool version. */
  item.func.bfunc = NULL ;
  item.display = TRUE ;
  array(prefList,arrayMax(prefList),PREFLIST) = item ;

  strcpy(item.name, MAX_MSG_LIST_LENGTH) ;
  item.type = PREF_INT ;
  item.value.ival = 0 ;
  item.deffunc.ifunc = NULL ;
  item.editfunc.ifunc = NULL ;
  item.func.ifunc = NULL ;
  item.display = TRUE ;
  array(prefList,arrayMax(prefList),PREFLIST) = item ;

  strcpy(item.name, DNA_HIGHLIGHT_IN_FMAP_DISPLAY);
  item.type = PREF_COLOUR; 
  item.value.ival = PALERED ;
  item.deffunc.ifunc = NULL ;
  item.editfunc.ifunc = NULL ;				    /* no bool version. */
  item.func.ifunc = NULL ;
  item.display = TRUE;
  array(prefList,arrayMax(prefList),PREFLIST) = item;

  return prefList;
} /* makePrefDefaults */


/* Read in the system defaults if they exist. */
static void copySystemPrefs(Array prefsList)
{
  ACEIN pref_in = NULL ;
  char *filename = NULL ;
  
  if ((filename = dbPathFilName("wspec", "acedbrc","","r", 0))
      && (pref_in = aceInCreateFromFile(filename, "r","", 0)))
    {
      prefRead(pref_in, prefsList);
      aceInDestroy (pref_in);
      messfree(filename);
    }

  return ;
} /* copySystemPrefs */


static void prefAdd(Array prefList, PREFLIST *item)
{
  BOOL found = FALSE;
  int i;

  for(i=0;i<arrayMax(prefList);i++)
    {
      if(!strcmp(item->name,arrp(prefList,i,PREFLIST)->name)) /* is it the same name */
	{
	  if(arrp(prefList,i,PREFLIST)->type != item->type)
	    {
	      messout("ERROR: Prefile reading in the preference %s but does not have the same type definition as that in the master copy. This will be ignored.",item->name);
	      return;
	    }

	  if(arrp(prefList,i,PREFLIST)->type == PREF_BOOLEAN)
	    arrp(prefList,i,PREFLIST)->value.bval = item->value.bval;
	  else if(arrp(prefList,i,PREFLIST)->type == PREF_INT ||arrp(prefList,i,PREFLIST)->type == PREF_COLOUR )
	    arrp(prefList,i,PREFLIST)->value.ival = item->value.ival; 
	  else if(arrp(prefList,i,PREFLIST)->type == PREF_FLOAT)
	    arrp(prefList,i,PREFLIST)->value.fval = item->value.fval;
	  else if(arrp(prefList,i,PREFLIST)->type == PREF_STRING)
	    {
	      if (arrp(prefList,i,PREFLIST)->value.sval != NULL)
		messfree(arrp(prefList,i,PREFLIST)->value.sval) ;
	      arrp(prefList,i,PREFLIST)->value.sval = item->value.sval ;
	    }
	  else
	    {
	      messout("ERROR: Prefile reading in the preference %s with invalid type, ignoring.",item->name);
	      return;
	    }

	  arrp(prefList,i,PREFLIST)->display = TRUE;
	  found = TRUE;
	}
    }

  if(!found)
    {
      item->display = FALSE;
      array(prefList,arrayMax(prefList),PREFLIST) = *item;
    }
  
  return;
} /* prefAdd */
  

void prefRead (ACEIN pref_in, Array prefList)
{
  PREFLIST item;
  int i,j;  
  char *junk, *cp;
  KEY colKey = 0 ;

  /* read in the preferences */
  while ((junk = aceInCard (pref_in)))
    {
      if(junk[0] == '_')
	{
	  i =1;
	  j=0;

	  while((isalpha((int)junk[i]) || junk[i] == '_') /*&&(junk[i]!=' ') &&junk[i]!='\t'*/ && j<30)
	    {
	      item.name[j++]= junk[i++];
	    }

	  item.name[j++] = '\0';
	  if(j == 1)
	    messout("Error reading name after _ in %s .acdbrc \n",junk) ;
	  else
	    {
	      while(junk[i] != '-' && junk[i]!='\n')
		{
		  i++;
		}

	      if(junk[i]=='-')
		{
		  if(junk[i+1] == 't' || junk[i+1] == 'T')
		    {
		      item.value.bval = TRUE;
		      item.type = PREF_BOOLEAN;
		      item.deffunc.bfunc = NULL ;
		      item.editfunc.ifunc = NULL ;	    /* no bool version */
		      item.func.bfunc = NULL ;
		      prefAdd(prefList, &item);	      
		    }
		  else if(junk[i+1] == 'f' || junk[i+1] == 'F')
		    {
		      item.value.bval = FALSE;
		      item.type = PREF_BOOLEAN;
		      item.deffunc.bfunc = NULL ;
		      item.editfunc.ifunc = NULL ;	    /* no bool version */
		      item.func.bfunc = NULL ;
		      prefAdd(prefList, &item);
		    }
		  else if(junk[i+1] == 'I' || junk[i+1] == 'i')
		    {
		      cp = &junk[i+2];
		      if(sscanf(cp,"%d",&item.value.ival))
			{
			  item.type=PREF_INT;
			  item.deffunc.ifunc = NULL ;
			  item.editfunc.ifunc = NULL ;
			  item.func.ifunc = NULL ;
			  prefAdd(prefList, &item);
			}
		      else
			messout("ERROR reading integer after %s",item.name);
		    }
		  else if(junk[i+1] == 'C' || junk[i+1] == 'c')
		    {
		      cp = &junk[i+2];
		      if(sscanf(cp,"%d",&item.value.ival))
			{
			  item.type=PREF_COLOUR;
			  item.deffunc.ifunc = NULL ;
			  item.editfunc.ifunc = NULL ;	    /* no bool version */
			  item.func.ifunc = NULL ;
			  prefAdd(prefList, &item);
			}
		      else if (lexword2key(cp,&colKey,0) && 
			   colKey >= _WHITE && 
			   colKey < _WHITE + 32)
			{
			  item.type=PREF_COLOUR;
			  item.value.ival = WHITE + colKey - _WHITE ; /* tags to color enum */
			  item.deffunc.ifunc = NULL ;
			  item.editfunc.ifunc = NULL ;	    /* no bool version */
			  item.func.ifunc = NULL ;
			  prefAdd(prefList, &item);
			}	      
		      else
			messout("ERROR reading colour after %s",item.name);
		    }
		  else if(junk[i+1] == 'R' || junk[i+1] == 'r')
		    {
		      cp = &junk[i+2];
		      if(sscanf(cp,"%f",&item.value.fval))
			{
			  item.type=PREF_FLOAT;
			  item.deffunc.ffunc = NULL ;
			  item.editfunc.ffunc = NULL ;
			  item.func.ffunc = NULL ;
			  prefAdd(prefList, &item);
			}
		      else
			messout("ERROR reading float after %s",item.name);
		    }
		  else if(junk[i+1] == 'S' || junk[i+1] == 's')
		    {
		      cp = &junk[i+2] ;

		      while((*cp == ' ' || *cp == '\t') && *cp != '\n')
			{
			  cp++ ;
			}

		      if (*cp != '\n')
			{
			  item.value.sval = strnew(cp, 0) ;
			  item.type = PREF_STRING ;
			  item.deffunc.sfunc = NULL ;
			  item.editfunc.ifunc = NULL ;
			  item.func.ffunc = NULL ;
			  prefAdd(prefList, &item);
			}
		      else
			messout("ERROR reading string after %s", item.name);
		    }
		  else
		    {
		      messout("Error reading value for %s.",item.name);
		    }
		}
	    }
	}
    }

  return ;
} /* prefRead */


/* Set a callback function which will check a given value for a preference   */
/* and if it is invalid will provide a valid "default" value.                */
/* Note that when the caller sets this function, we call it back immediately */
/* to get the default value, this way we don't have to remember later and    */
/* hopefully avoid synchronisation problems with the caller, i.e. they       */
/* wouldn't have called us unless their callback function was ready to be    */
/* called !!  ...would they...???                                            */
/*                                                                           */
BOOL prefSetDefFunc(char *name, void *func_ptr)
{
  BOOL result = FALSE ;
  int i;
  PREFLIST *item;

  if (!name || !*name || !func_ptr || !arrayExists(prefList_G))
    messcrash("Internal Coding Error: %s.",
	      ((!name || !*name)
	       ? "null preference name as arg" 
	       : ((!func_ptr)
		  ? "Callback function pointer is NULL"
		  : "prefInit() has not been called"))) ;

  for(i = 0 ; i < arrayMax(prefList_G) ; i++)
    {
      item = arrp(prefList_G,i,PREFLIST);
      if(strstr(name, item->name))
	{
	  if(item->type == PREF_BOOLEAN)
	    {
	      item->deffunc.bfunc = (PrefDefFuncBOOL)func_ptr ;
	      item->value.bval = item->deffunc.bfunc(item->value.bval) ;
	    }
	  else if(item->type == PREF_INT || item->type == PREF_COLOUR)
	    {
	      item->deffunc.ifunc = (PrefDefFuncInt)func_ptr ;
	      item->value.ival = item->deffunc.ifunc(item->value.ival) ;
	    }
	  else if(item->type == PREF_FLOAT)
	    {
	      item->deffunc.ffunc = (PrefDefFuncFloat)func_ptr ;
	      item->value.fval = item->deffunc.ffunc(item->value.fval) ;
	    }
	  else if(item->type == PREF_STRING)
	    {
	      item->deffunc.sfunc = (PrefDefFuncString)func_ptr ;
	      item->value.sval = item->deffunc.sfunc(item->value.sval) ;
	    }
	  result = TRUE ;
	}
    }

  return result ;
}


/* Set an edit callback (will be called from graphNNNEditor()) to check      */
/* value a user enters into an edit box.                                     */
/* NOTE: this can only be set for int and float prefs.                       */
/*       Really this replicates the above function but I don't want to       */
/*       rewrite the graphNNNEditor() stuff so we have to replicate.         */
/*                                                                           */
BOOL prefSetEditFunc(char *name, void *func_ptr)
{
  BOOL result = FALSE ;
  int i;
  PREFLIST *item;

  if (!name || !*name || !func_ptr || !arrayExists(prefList_G))
    messcrash("Internal Coding Error: %s.",
	      ((!name || !*name)
	       ? "null preference name as arg" 
	       : ((!func_ptr)
		  ? "Callback function pointer is NULL"
		  : "prefInit() has not been called"))) ;

  for(i = 0 ; i < arrayMax(prefList_G) ; i++)
    {
      item = arrp(prefList_G,i,PREFLIST);
      if(strstr(name, item->name))
	{
	  if(item->type == PREF_INT)
	    {
	      item->editfunc.ifunc = (PrefEditFuncInt)func_ptr ;
	    }
	  else if(item->type == PREF_FLOAT)
	    {
	      item->editfunc.ffunc = (PrefEditFuncFloat)func_ptr ;
	    }
	  else if(item->type == PREF_STRING)
	    {
	      item->editfunc.sfunc = (PrefEditFuncString)func_ptr ;
	    }
	  result = TRUE ;
	}
    }

  return result ;
}


/* Set a callback function for a preference, these get called when someone   */
/* sets a preference value.                                                  */
/*                                                                           */
BOOL prefSetFunc(char *name, void *func_ptr)
{
  BOOL result = FALSE ;
  int i;
  PREFLIST *item;

  if (!name || !*name || !func_ptr || !arrayExists(prefList_G))
    messcrash("Internal Coding Error: %s.",
	      ((!name || !*name)
	       ? "null preference name as arg" 
	       : ((!func_ptr)
		  ? "Callback function pointer is NULL"
		  : "prefInit() has not been called"))) ;

  for(i = 0 ; i < arrayMax(prefList_G) ; i++)
    {
      item = arrp(prefList_G,i,PREFLIST);
      if(strstr(name, item->name))
	{
	  if(item->type == PREF_BOOLEAN)
	    {
	      item->func.bfunc = (PrefFuncBOOL)func_ptr ;
	    }
	  else if(item->type == PREF_INT || item->type == PREF_COLOUR)
	    {
	      item->func.ifunc = (PrefFuncInt)func_ptr ;
	    }
	  else if(item->type == PREF_FLOAT)
	    {
	      item->func.ffunc = (PrefFuncFloat)func_ptr ;
	    }
	  else if(item->type == PREF_STRING)
	    {
	      item->func.sfunc = (PrefFuncString)func_ptr ;
	    }
	  result = TRUE ;
	}
    }

  return result ;
}


/* Returns the value for the char * name if it exists in the preferences. */
/* If it does not exist then returns the default FALSE. */
/*                                                                           */
BOOL prefValue(char * name)
{
  int i;
  PREFLIST *item;

  if (!name || !*name || !arrayExists(prefList_G))
    /* should not need this ??? */
    return FALSE ;

  for(i = 0 ; i < arrayMax(prefList_G) ; i++)
    {
      item = arrp(prefList_G,i,PREFLIST);
      if(strstr(name,item->name))
	{
	  if(item->type == PREF_BOOLEAN)
	    return item->value.bval ;
	  else
	    {
	      messout("%s not of type boolean",item->name);
	      return FALSE;
	    }
	}
    }
  return FALSE;   /* Default is set to false */
}


/* Returns the value for the char * name if it exists in the preferences. */
/* If it does not exist then returns the default 0.0. */
/*                                                                           */
float prefFloat(char * name)
{
  int i;
  PREFLIST *item;

  if (!name || !*name || !arrayExists(prefList_G))
    /* should not need this ??? */
    return 0.0 ;

  for(i=0;i<arrayMax(prefList_G);i++)
    {
      item = arrp(prefList_G,i,PREFLIST);
      if(strstr(name,item->name))
	{
	  if(item->type == PREF_FLOAT)
	    return item->value.fval;
	  else
	    {
	      messout("%s not of type float",item->name);
	      return 0.0;
	    }
	}
    }
  return 0.0;   /* Default is set to 0.0 */
}

/* Returns the value for the char * name if it exists in the preferences. */
/* If it does not exist then returns the default FALSE. */
int prefInt(char * name)
{
  int i;
  PREFLIST *item;

  if (!name || !*name || !arrayExists(prefList_G  ))
    /* should not need this ??? */
    return 0 ;

  for(i=0;i<arrayMax(prefList_G);i++)
    {
      item = arrp(prefList_G,i,PREFLIST);
      if(strstr(name,item->name))
	{
	  if (item->type == PREF_INT ||
	      item->type == PREF_COLOUR) /* should really use prefColour */
	    return item->value.ival;
	  else
	    {
	      messout("%s not of type integer",item->name);
	      return 0;
	    }
	}
    }
  return 0;   /* Default is set to 0 */
}

int prefColour (char * name)
{
/* Returns the value for the char * name if it exists in the preferences. */
/* If it does not exist then returns the default FALSE. */
  int i;
  PREFLIST *item;

  if (!name || !*name || !arrayExists(prefList_G))
    /* should not need this ??? */
    return 0 ;

  for(i=0;i<arrayMax(prefList_G);i++)
    {
      item = arrp(prefList_G,i,PREFLIST);
      if(strstr(name,item->name))
	{
	  if(item->type == PREF_COLOUR)
	    return item->value.ival;
	  else
	    {
	      messout("%s not of type colour",item->name);
	      return 0;
	    }
	}
    }
  return 0;   /* Default is set to 0 */
}


/* Returns the value for the char * name if it exists in the preferences. */
/* If it does not exist then returns the default NULL. */
char *prefString(char * name)
{
  char *result = NULL ;
  int i ;
  PREFLIST *item ;

  if (!name || !*name || !arrayExists(prefList_G))
    result = NULL ;

  for(i = 0 ; i < arrayMax(prefList_G) ; i++)
    {
      item = arrp(prefList_G,i,PREFLIST);
      if(strstr(name,item->name))
	{
	  if (item->type == PREF_STRING)
	    result = item->value.sval ;
	  else
	    messout("%s not of type string", item->name) ;
	}
    }

  return result ;
}


/************************ eof *******************************/
