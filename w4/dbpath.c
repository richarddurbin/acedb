/*  File: dbpath.c
 *  Author: Jean Thierry-Mieg (mieg@mrc-lmb.cam.ac.uk)
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
 *	Jean Thierry-Mieg (CRBM du CNR, France) mieg@kaa.cnrs-mop.fr
 *
 * Description:
 *  Utility functions for finding files needed by the database system
 * HISTORY:
 * Created: Fri Jan 14 11:19:27 2000 (srk)
 *-------------------------------------------------------------------
 */

#include "acedb.h"
#include "dbpath.h"
#ifdef __CYGWIN__
#include <windows.h>
#endif

static char *dbpath = NULL;
static char *commondir = NULL;

/* dbpath_from_command_line may be zero, in which case we use $ACEDB */
BOOL dbPathInit(char *dbpath_from_command_line)
{
  char *cp;
  if (dbpath)
    messcrash("repeated call to dbPathInit");

  if (!dbpath_from_command_line)
    dbpath_from_command_line = getenv("ACEDB");

  if (!dbpath_from_command_line)
    return FALSE;

  dbpath = filGetName(dbpath_from_command_line, "", "rd", 0);

  if (!dbpath)
    return FALSE;
  

  /* We set $ACEDB to the (absolute, posix) path. 
     It then gets inherited in this for by all processes we spawn.
     If we didn't do this $ACEDB would remain unset when using the command
     line to specfy the database.
     $ACEDB is used eg by  scripts.
  */
  putenv(strnew(messprintf("ACEDB=%s", dbpath), 0));
  /* Note: don't be tempted to use setenv() here: it's not portable
     to Solaris or IRIX. */

  if (getenv("ACEDB_COMMON"))
    commondir = getenv("ACEDB_COMMON");
#ifdef __CYGWIN__
  else
    /* I think I've fundamentally missed the point of this API:
       why can't I do something like
       GetRegEntry("HKEY_LOCAL_MACHINE\SOFTWARE\Acedb.org\Acedb\4.x\InstallPath",
                   STRING, buffer, size);
    */
    { 
      HKEY software, acedb_org, acedb, four_x;
      if (RegOpenKeyEx(HKEY_LOCAL_MACHINE,
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
				   "4.x", 0L, 
				   KEY_EXECUTE,
				   &four_x) == ERROR_SUCCESS)
		    { 
		      DWORD buffsiz = 255;
		      char buff[256];
		      if (RegQueryValueEx(four_x,
					  "InstallPath",
					  NULL,
					  NULL,
					  buff,
					  &buffsiz) == ERROR_SUCCESS)
			commondir = filGetName(buff, "", "rd", 0);
		      RegCloseKey(four_x);
		    }
		  RegCloseKey(acedb);
		}
	      RegCloseKey(acedb_org);
	    }
	  RegCloseKey(software);
	}
    }	    
#elif defined(DARWIN)
  /* Fink put packages under /sw */
  else if (filCheckName("/sw/share/acedb", "", "rd"))
    commondir = filGetName("/sw/share/acedb", "", "rd", 0);
#else
  else if (filCheckName("/usr/share/acedb", "", "rd"))
    commondir = filGetName("/usr/share/acedb", "", "rd", 0);
  else if (filCheckName("/usr/local/acedb", "", "rd"))
    commondir = filGetName("/usr/local/acedb", "", "rd", 0);
#endif

  if (!getenv ("ACEDB_NO_BANNER"))
    {
      printf ("// Database directory: %s\n", dbpath) ;
      if (commondir)
	printf ("// Shared files: %s\n", commondir) ;
    }

  return TRUE;
}
      

/* Look for file in the database and common directories */      
char *dbPathFilName (char *dir, char *name, char *ending, 
		     char *spec, STORE_HANDLE handle)
{
  Stack s = stackCreate(MAXPATHLEN);
  char *ret;

  if (!dbpath)
    messcrash("called dbPathFilname before dbPathInit");
  
  catText(s, dbpath);
  if (dir && *dir)
    {
      catText(s, "/");
      catText(s, dir);
    }
  if (name && *name)
    {
      catText(s, "/");
      catText(s, name);
    }

  if (ret = filGetName(stackText(s, 0), ending, spec, handle))
    {
      stackDestroy(s);
      return ret;
    }
  
  if (commondir)
    {
      stackClear(s);
      catText(s, commondir);
      if (dir && *dir)
	{
	  catText(s, "/");
	  catText(s, dir);
	}
      if (name && *name)
	{
	  catText(s, "/");
	  catText(s, name);
	}
      
      ret = filGetName(stackText(s, 0), ending, spec, handle);
      stackDestroy(s);
      return ret;
    }

  return 0;
}

/* Look for the file only in the database directory */
char *dbPathStrictFilName (char *dir, char *name, char *ending, 
			   char *spec, STORE_HANDLE handle)
{
  Stack s = stackCreate(MAXPATHLEN);
  char *ret;

  if (!dbpath)
    messcrash("called dbPathStrictFilname before dbPathInit");
  
  catText(s, dbpath);
  if (dir && *dir)
    {
      catText(s, "/");
      catText(s, dir);
    }
  if (name && *name)
    {
      catText(s, "/");
      catText(s, name);
    }
  
  ret = filGetName(stackText(s, 0), ending, spec, handle);
  stackDestroy(s);
  return ret;
}

char *dbPathMakeFilName (char *dir, char *name, char *ending, 
			 STORE_HANDLE handle)
{
  Stack s = stackCreate(MAXPATHLEN);
  char *ret;
  
  if (!dbpath)
    messcrash("called dbPathMakeFilname before dbPathInit");
  
  catText(s, dbpath);
  if (dir && *dir)
    {
      catText(s, "/");
      catText(s, dir);
    }
  if (name && *name)
    {
      catText(s, "/");
      catText(s, name);
    }
  if (ending && *ending)
    { 
      catText(s, ".");
      catText(s, ending);
    }
  
  ret = strnew(stackText(s, 0), handle);
  stackDestroy(s);
  return ret;
}  
  
  
  
  
  




