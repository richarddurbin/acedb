/*  File: filsubs.c
 *  Author: Jean Thierry-Mieg (mieg@mrc-lmb.cam.ac.uk)
 *  Copyright (C) J Thierry-Mieg and R Durbin, 1991
 * -------------------------------------------------------------------
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 * -------------------------------------------------------------------
 * This file is part of the ACEDB genome database package, written by
 * 	Richard Durbin (MRC LMB, UK) rd@mrc-lmb.cam.ac.uk, and
 *	Jean Thierry-Mieg (CRBM du CNRS, France) mieg@kaa.cnrs-mop.fr
 *
 * Description:
 *                   cross platform file system routines
 *              
 * Exported functions:
 * HISTORY:
 * Last edited: Dec 12 11:32 2003 (edgrif)
 * * Nov 29 11:53 1999 (edgrif): Added new filCopyFile function.
 * * Apr 13 13:40 1999 (fw): fixed bug in filCheck if spec is "wd" or "ad"
 * * Dec  8 10:20 1998 (fw): new function filAge to determine time since
 *              last modification of file
 * * Oct 22 16:17 1998 (edgrif): Replace unsafe strtok with strstr.
 * * Oct 15 11:47 1998 (fw): include messSysErrorText in some messges
 * * Sep 30 09:37 1998 (edgrif): Replaced my strdup with acedb strnew.
 * * Sep  9 14:07 1998 (edgrif): Add filGetFilename routine that will 
 *               return the filename given a pathname 
 *              (NOT the same as the UNIX basename).
 * * DON'T KNOW WHO DID THE BELOW..assume Richard Bruskiewich (edgrif)
 *	-	fix root path detection for default drives (in WIN32)
 * * Oct  8 23:34 1996 (rd)
 *              filDirectory() returns a sorted Array of character 
 *              strings of the names of files, with specified ending 
 *              and spec's, listed in a given directory "dirName";
 *              If !dirName or directory is inaccessible, 
 *              the function returns 0
 * * Jun  6 17:58 1996 (rd)
 * * Mar 24 02:42 1995 (mieg)
 * * Feb 13 16:11 1993 (rd): allow "" endName, and call getwd if !*dname
 * * Sep 14 15:57 1992 (mieg): sorted alphabetically
 * * Sep  4 13:10 1992 (mieg): fixed NULL used improperly when 0 is meant
 * * Jul 20 09:35 1992 (aochi): Add directory names to query file chooser
 * * Jan 11 01:59 1992 (mieg): If file has no ending i suppress the point
 * * Nov 29 19:15 1991 (mieg): If file had no ending, we were losing the
                               last character in dirDraw()
 * Created: Fri Nov 29 19:15:34 1991 (mieg)
 * CVS info:   $Id: filsubs.c,v 1.80 2003-12-12 14:13:41 edgrif Exp $
 *-------------------------------------------------------------------
 */

#include <wh/regular.h>
#include <wh/aceio.h>
#include <wh/mytime.h>
#include <wh/call.h>					    /* for callScript (to mail stuff) */
#include <wh/mydirent.h>
#include <sys/file.h>

/********************************************************************/




/*****************************************************************************/
/* This function returns the directory part of a given path,                 */
/*                                                                           */
/*   Given   "/some/load/of/directories/filename"  returns                   */
/*   returns "/some/load/of/directories"                                     */
/*                                                                           */
/* The function returns NULL for the following input:                        */
/*                                                                           */
/* 1) supplying a NULL ptr as the path                                       */
/* 2) supplying "" as the path                                               */
/*                                                                           */
/* and returns "." for the following input:                                  */
/*    supplying a path that does not contain a "/"                           */
/*                                                                           */
/* NOTE, this function is _NOT_ the same as the UNIX dirname command or the  */
/* XPG4_UNIX dirname() function which do different things.                   */
/*                                                                           */
/* The function makes a copy of the supplied path on which to work, this     */
/* copy is thrown away each time the function is called.                     */
/*                                                                           */
char *filGetDirname(char *path)
{
  static char *path_copy = NULL ;
  char *result = NULL, *tmp = NULL ;
    
  if (path != NULL)
    {
      if (path_copy != NULL)
	messfree(path_copy) ;
	  
      path_copy = strnew(path, 0) ;

      tmp = strrchr(path_copy, SUBDIR_DELIMITER) ;
      if (tmp)
	{
	  *tmp = '\0' ;
	  result = path_copy ;
	}
      else
	result = "." ;
    }
  
  return(result) ;
} /* filGetDirname */


/*****************************************************************************/
/* This function returns the filename part of a given path,                  */
/*                                                                           */
/*   Given   "/some/load/of/directories/filename"  returns  "filename"       */
/*                                                                           */
/* The function returns NULL for the following errors:                       */
/*                                                                           */
/* 1) supplying a NULL ptr as the path                                       */
/* 2) supplying "" as the path                                               */
/* 3) supplying a path that ends in "/"                                      */
/*                                                                           */
/* NOTE, this function is _NOT_ the same as the UNIX basename command or the */
/* XPG4_UNIX basename() function which do different things.                  */
/*                                                                           */
/* The function makes a copy of the supplied path on which to work, this     */
/* copy is thrown away each time the function is called.                     */
/*                                                                           */
char *filGetFilename(char *path)
{
  static char *path_copy = NULL ;
  const char *path_delim = SUBDIR_DELIMITER_STR ;
  char *result = NULL, *tmp ;
    
  if (path != NULL)
    {
      if (strcmp((path + strlen(path) - 1), path_delim) != 0) /* Last char = "/" ?? */
	{
	  if (path_copy != NULL) messfree(path_copy) ;
	  
	  path_copy = strnew(path, 0) ;
	  
	  tmp = path ;
	  while (tmp != NULL)
	    {
	      result = tmp ;
	      
	      tmp = strstr(tmp, path_delim) ;
	      if (tmp != NULL) tmp++ ;
	    }
	}
    }
  
  return(result) ;
} /* filGetFilename */


/*****************************************************************************/
/* This function returns the base filename part of a given path.             */
/*                                                                           */
/*   Given   "/some/load/of/directories/filename.something.ext"              */
/* returns   "filename.something"                                            */
/*                                                                           */
/* i.e. it lops off the directory bit and the _last_ extension.              */
/*                                                                           */
/* The function returns NULL for the following errors:                       */
/*                                                                           */
/* 1) supplying a NULL ptr as the path                                       */
/* 2) supplying "" as the path                                               */
/* 3) supplying a path that ends in "/"                                      */
/*                                                                           */
/* NOTE, this function is _NOT_ the same as the UNIX basename command or the */
/* XPG4_UNIX basename() function which do different things.                  */
/*                                                                           */
/* The function makes a copy of the supplied path on which to work, this     */
/* copy is thrown away each time the function is called.                     */
/*                                                                           */
char *filGetFilenameBase(char *path)
{
  static char *path_copy = NULL ;
  const char *path_delim = SUBDIR_DELIMITER_STR ;
  char *result = NULL, *tmp ;
    
  if (path != NULL)
    {
      if (strcmp((path + strlen(path) - 1), path_delim) != 0) /* Last char = "/" ?? */
	{
	  if (path_copy != NULL)
	    messfree(path_copy) ;
	  
	  path_copy = strnew(path, 0) ;
	  
	  tmp = path ;
	  while (tmp != NULL)
	    {
	      result = tmp ;
	      
	      tmp = strstr(tmp, path_delim) ;
	      if (tmp != NULL)
		tmp++ ;
	    }

	  tmp = strrchr(result, EXT_DELIMITER) ;
	  if (tmp)
	    *tmp = '\0' ;
	}
    }
  
  return(result) ;
}


/*****************************************************************************/
/* This function returns the file-extension part of a given path/filename,   */
/*                                                                           */
/*   Given   "/some/load/of/directories/filename.ext"  returns  "ext"        */
/*                                                                           */
/* The function returns NULL for the following errors:                       */
/*                                                                           */
/* 1) supplying a NULL ptr as the path                                       */
/* 2) supplying a path with no filename                                      */
/*                                                                           */
/* The function returns "" for a filename that has no extension              */
/*                                                                           */
/* The function makes a copy of the supplied path on which to work, this     */
/* copy is thrown away each time the function is called.                     */
/*                                                                           */
char *filGetExtension(char *path)
{
  static char *path_copy = NULL ;
  char *extension = NULL, *cp ;
    
  if (path == NULL)
    return NULL;

  if (strlen(path) == 0)
    return NULL;

  if (path_copy != NULL) messfree(path_copy) ;
  path_copy = messalloc ((strlen(path)+1) * sizeof(char));
  strcpy (path_copy, path);

  cp = path_copy + (strlen(path_copy) - 1);
  while (cp > path_copy &&
	 *cp != SUBDIR_DELIMITER &&
	 *cp != EXT_DELIMITER)
    --cp;

  extension = cp+1;
    
  return(extension) ;
} /* filGetExtension */


/***************************************************************************/

static BOOL filCheck (char *name, char *spec)
	/* allow 'd' as second value of spec for a directory */
{
  char *cp ;
  BOOL result ;
  struct stat status ;

  if (spec[1] == 'd')
    if (stat (name, &status) == 0 && (!(status.st_mode & S_IFDIR)))
      /* the pathname isn't a directory, but we asked for that */
      return FALSE;

  switch (*spec)
    {
    case 'e':						    /* file exists. */
      if (access (name, F_OK) == 0)
	{
	  if (spec[1] != 'd')
	    if (stat (name, &status) == 0 && (status.st_mode & S_IFDIR))
	      /* the pathname exists and is readable, but it is a
		 directory and we didn't ask for that */
	      return FALSE;

	  return TRUE;
	}
      return FALSE;

    case 'r':
      if (access (name, R_OK) == 0)
	{
	  if (spec[1] != 'd')
	    if (stat (name, &status) == 0 && (status.st_mode & S_IFDIR))
	      /* the pathname exists and is readbale, but it is a
		 directory and we didn't ask for that */
	      return FALSE;

	  return TRUE;
	}
      return FALSE;
    case 'w':
    case 'a':
      if (access (name, W_OK) == 0) /* requires file exists */
	{
	  if (spec[1] != 'd')
	    if (stat (name, &status) == 0 && (status.st_mode & S_IFDIR))
	      /* the pathname exists and is writable, but it is a
		 directory and we didn't ask for that */
	      return FALSE;

	  return TRUE ;
	}
      if (spec[1] == 'd')
	return FALSE;

      if (stat (name, &status) == 0)
	/* write access failed, but file exists, so check failed */
	return FALSE;

      /* for file check, we test whether its directory is writable */
      cp = name + strlen (name) ;
      while (cp > name)
	if (*--cp == SUBDIR_DELIMITER) break ;
      if (cp == name)
	return !(access (".", W_OK)) ;
      else
	{ *cp = 0 ;
	  result = !(access (name, W_OK)) ;
	  *cp = SUBDIR_DELIMITER ;
	  return result ;
	}
    case 'x':
      return (access (name, X_OK) == 0) ;
    default:
      messcrash ("Unknown spec %s passed to filName", spec) ;
    }
  return FALSE ;
}

/************************************************/

static BOOL filCanonPath (Stack s, char *name, char *ending)
{
#ifdef __CYGWIN__
  char posix[PATH_MAX];
  
  /* cygwin_conv doesn't like these (but they're already posix) */
  if ((*name) && (strcmp(name, ".") != 0) && (*name != '~'))
    cygwin_conv_to_full_posix_path(name, posix);
  else
    strcpy(posix, name);
  
  /* win32 doesn't like : in filenames - replace with . */
  for (name = posix; *name; name++)
    if (*name == ':')
      *name = '.';
      
  name = posix;
#endif

  if (*name == '/') /* absolute path */
    catText(s, name);
  else if (*name == '~') /* tilde expansion */
   {
     char *name_copy = strnew(name, 0);
     char *user = name_copy+1;
     char *user_endp, *path;
     char *homedir;
     
     /* get the username */

     user_endp = strstr (name_copy, "/");
     if (user_endp)
       {
	 *user_endp = 0;
	 path = user_endp+1;
       }
     else
       path = 0;
     
      if (strlen(user) == 0)
	user = getLogin(TRUE);
      
      homedir = getUserHomeDir (user, 0);
      messfree(name_copy);
      
      if (homedir)
	{
	  catText(s, homedir);
	  catText(s, "/");
	  messfree(homedir);
	}
      else
	return FALSE;
      
      if (path)
	catText(s, path);
   } 
  else /* Add cwd to make absolute path */
    {
      int wdsiz = MAXPATHLEN;
      char *wd = messalloc(wdsiz);
      
      while (!getcwd(wd, wdsiz))
	{ 
	  messfree(wd);
	  wdsiz += MAXPATHLEN;
	  wd = messalloc(wdsiz);
	}
      catText(s, wd);
      if ((*name) && (strcmp(name, ".") != 0))
	/* dump empty name or "." */
	{
	  catText(s, "/");
	  catText(s, name);
	}
      messfree(wd);
    }
  
  if (ending && *ending)
    { 
      catText(s, ".");
      catText(s, ending);
    }

  return TRUE;
}


char *filGetName(char *name, char *ending, 
		 char *spec, STORE_HANDLE handle)
{
  Stack s = stackCreate(MAXPATHLEN);
  char *ret;
  
  if (!name)
    messcrash ("filGetName received a null name") ;
  
  if (filCanonPath(s, name, ending) && filCheck(stackText(s, 0), spec))
    ret = strnew(stackText(s, 0), handle);
  else
    ret = NULL;
  
  stackDestroy(s);
  
  return ret;
}

BOOL filCheckName(char *name, char *ending, char *spec)
{
  Stack s = stackCreate(MAXPATHLEN);
  BOOL ret;

  if (!name)
    messcrash ("filCheckName received a null name") ;
  
  ret = filCanonPath(s, name, ending) && filCheck(stackText(s, 0), spec);

  stackDestroy(s);
  
  return ret;
}

char *filGetFullPath(char *dir, STORE_HANDLE handle)
{
  Stack s = stackCreate(MAXPATHLEN);
  char *ret;
  
  if (filCanonPath(s, dir, 0))
    ret = strnew(stackText(s, 0), handle);
  else
    ret = NULL;

  stackDestroy(s);

  return ret;
}
  
/* Compatibilty function: Note that:
   THIS IS NOT THREADSAFE.
   Continued use of this function will send you blind. */
char *filName (char *name, char *ending, char *spec)
{ 
  static Stack s = NULL;
  
  if (!s)
    s = stackCreate(MAXPATHLEN);
  else
    stackClear(s);
    
  if (filCanonPath(s, name, ending) && filCheck(stackText(s, 0), spec))
    return stackText(s, 0);
  else
    return NULL;
}

/************************************************************/

BOOL filremove (char *name, char *ending) 
				/* TRUE if file is deleted. -HJC*/
{
  Stack s = stackCreate(MAXPATHLEN);
  BOOL ret;
  
  filCanonPath(s, name, ending) ;
  ret = unlink(stackText(s, 0)) ? FALSE : TRUE ;
  
  stackDestroy(s);
  
  return ret;
} /* filremove */

/**************************************************************/

FILE *filopen (char *name, char *ending, char *spec)
{
  Stack s = stackCreate(MAXPATHLEN);
  FILE *result = 0 ;
  
  filCanonPath(s, name, ending);

  if (!filCheck(stackText(s, 0), spec))
    {
      if (spec[0] == 'r')
	messerror ("Failed to open for reading: %s (%s)",
		   stackText(s, 0),
		   messSysErrorText()) ;
      else if (spec[0] == 'w')
	messerror ("Failed to open for writing: %s (%s)",
		   stackText(s ,0),
		   messSysErrorText()) ;
      else if (spec[0] == 'a')
	messerror ("Failed to open for appending: %s (%s)",
		   stackText(s ,0),
		   messSysErrorText()) ;
      else
	messcrash ("filopen() received invalid filespec %s",
		   spec ? spec : "(null)");
    }
  else  if (!(result = fopen (stackText(s, 0), spec)))
    messerror ("Failed to open %s (%s)",
	       stackText(s, 0), messSysErrorText()) ;
     
 
  stackDestroy(s);

  return result ;
} /* filopen */

/********************* temporary file stuff *****************/

static Associator tmpFiles = 0 ;

FILE *filtmpopen (char **nameptr, char *spec)
{
  return filTmpOpenWithSuffix(nameptr, 0, spec);
}


FILE *filTmpOpenWithSuffix (char **nameptr, char *suffix, char *spec)
{
  char *nam, *realname;
  

#ifdef __CYGWIN__
  char *tmpenv = getenv("TEMP");
  char tmppath[PATH_MAX];
  
  if (tmpenv)
    cygwin_conv_to_full_posix_path(tmpenv, tmppath);
  else
    cygwin_conv_to_full_posix_path("c:\\Temp", tmppath);
#endif

  if (!nameptr)
    messcrash ("filtmpopen requires a non-null nameptr") ;

  if (!strcmp (spec, "r"))
    return filopen (*nameptr, 0, spec) ;

#if defined(SUN) || defined(SOLARIS)
  if (!(nam = tempnam ("/var/tmp", "ACEDB")))
#elif defined(__CYGWIN__)
  if (!(nam = tempnam(tmppath, "ACEDB")))
#else
  if (!(nam = tempnam ("/tmp", "ACEDB")))
#endif
    { 
      messerror ("failed to create temporary file (%s)",
		 messSysErrorText()) ;
      return 0 ;
    }
  
  if (suffix == 0)
    {
      realname = strnew(nam, 0);
    }
  else
    {
      realname = messalloc(strlen(nam)+strlen(suffix)+2); 
      /* . and terminator */
      strcpy(realname, nam);
      strcat(realname, ".");
      strcat(realname, suffix);
    }
  
  free(nam);

  if (!tmpFiles)
    tmpFiles = assCreate () ;
  assInsert (tmpFiles, realname, realname) ;

  *nameptr = realname;
  return filopen (*nameptr, 0, spec) ;
} /* filtmpopen */

/************************************************************/

BOOL filtmpremove (char *name)	/* delete and free()  */
{
  BOOL result = filremove (name, 0) ;
  assRemove (tmpFiles, name) ;
  messfree(name);
  return result ;
}

/************************************************************/

void filtmpcleanup (void)
{ char *name = 0 ;
 
  if (tmpFiles)
    while (assNext (tmpFiles, &name, 0))
      { 
	char *name1 = name;
	filremove (name, 0) ;
	messfree (name1) ; /* Since messfree zeros it. */
      }
}

/************* filqueryopen() ****************/

static QueryOpenRoutine queryOpenFunc = 0 ;

QueryOpenRoutine filQueryOpenRegister (QueryOpenRoutine new)
{ QueryOpenRoutine old = queryOpenFunc ; queryOpenFunc = new ; return old ; }

FILE *filqueryopen (char *dname, char *fname, char *end, char *spec, char *title)
{
  Stack s ;
  FILE*	fil = 0 ;
  int i ;
  ACEIN answer_in;
				/* use registered routine if available */
  if (queryOpenFunc)
    return (*queryOpenFunc)(dname, fname, end, spec, title) ;

  /* otherwise do here and use messPrompt() */
  s = stackCreate(50);

  if (dname && *dname)
    { pushText(s, dname) ; catText(s,"/") ; }
  if (fname)
    catText(s,fname) ; 
  if (end && *end)
    { catText(s,".") ; catText(s,end) ; }

 lao:
  answer_in = messPrompt("File name please", stackText(s,0), "w", 0);

  if (!answer_in)
    { 
      stackDestroy(s) ;
      return 0 ;
    }

  i = stackMark(s) ;
  pushText(s, aceInPath(answer_in));
  aceInDestroy (answer_in);

  if (spec[0] == 'w' && filCheckName (stackText(s,i), "", "r"))
    { 
      if (messQuery (messprintf ("Overwrite %s?",
				 stackText(s,i))))
	{ 
	  if ((fil = filopen (stackText(s,i), "", spec)))
	    goto bravo ;
	  else
	    messout ("Sorry, can't open file %s for writing",
		     stackText (s,i)) ;
	}
      goto lao ;
    }
  else if (!(fil = filopen (stackText(s,i), "", spec))) 
    messout ("Sorry, can't open file %s",
	     stackText(s,i));

bravo:
  stackDestroy(s) ;

  return fil ;
} /* filqueryopen */

/*********************************************/

Associator mailFile = 0, mailAddress = 0 ;

void filclose (FILE *fil)
{
  char *address ;
  char *filename ;

  if (!fil || fil == stdin || fil == stdout || 
      fil == stderr || fil == (FILE *)0x1) 
    /* see filquery.c - 0x1 == dummy directory */
    return ;
  fclose (fil) ;
  if (mailFile && assFind (mailFile, fil, &filename))
    { if (assFind (mailAddress, fil, &address))
	callScript ("mail", messprintf ("%s < %s", address, filename)) ;
      else
	messerror ("Have lost the address for mailfile %s", filename) ;
      assRemove (mailFile, fil) ;
      assRemove (mailAddress, fil) ;
      filtmpremove (filename) ;	/* came from filmail() */
    }
} /* filclose */

/***********************************/

FILE *filmail (char *address)	/* requires filclose() */
{
  char *filename ;
  FILE *fil ;

  if (!mailFile)
    { mailFile = assCreate () ;
      mailAddress = assCreate () ;
    }
  if (!(fil = filtmpopen (&filename, "w")))
    { messout ("failed to open temporary mail file %s", filename) ;
      return 0 ;
    }
  assInsert (mailFile, fil, filename) ;
  assInsert (mailAddress, fil, address) ;
  return fil ;
} /* filmail */

/******************* directory stuff *************************/

static int dirOrder(void *a, void *b)
{
  char *cp1 = *(char **)a, *cp2 = *(char**)b;
  return strcmp(cp1, cp2) ;
} /* dirOrder */

static void filDirFinalise (void *vp)
{
  FilDir fdir = (FilDir)vp;

  handleDestroy (fdir->handle);

  return;
} /* filDirFinalise */


FilDir filDirHandleCopy (FilDir fd, STORE_HANDLE handle)
{
  int i;
  FilDir fdir;

  fdir = (FilDir)halloc(sizeof(struct FilDirStruct), handle);
  blockSetFinalise (fdir, filDirFinalise);

  fdir->handle = handleCreate();
  fdir->entries = arrayHandleCreate (arrayMax(fd->entries), char*, fdir->handle) ;

  for (i = 0; i < filDirMax(fd); ++i)
    {
      array (fdir->entries, i, char*) = strnew(filDirEntry(fd, i), fdir->handle);
    }

  return fdir;
} /* filDirCopy */


/* returns a directory listing of strings representing the filename in the
   given directory according to the spec. "r" will list all files,
   and "rd" will list all directories.
   The behavior of the "w" spec is undefined.

   The directory entries are accessed using 
      char *filDirEntry (FilDir dir, int i);
      int filDirMax (FilDir dir);

   The returned FilDir object has to be destroyed using messfree,
   which reclaims all memory. */

FilDir filDirHandleCreate (char *dirName,
			   char *ending, 
			   char *spec,
			   STORE_HANDLE handle)
{
  FilDir fdir ;
  DIR	*dirp ;
  char	*dName, entryPathName[MAXPATHLEN], *leaf ;
  int	dLen, endLen ;
  MYDIRENT *dent ;

  if (!dirName || !(dirp = opendir (dirName)))
    return 0 ;

  fdir = (FilDir)halloc(sizeof(struct FilDirStruct), handle);
  blockSetFinalise (fdir, filDirFinalise);

  fdir->handle = handleCreate();
  fdir->entries = arrayHandleCreate (16, char*, fdir->handle) ;


  if (ending)
    endLen = strlen (ending) ;
  else
    endLen = 0 ;

  strcpy (entryPathName, dirName) ;
  strcat (entryPathName, "/") ;
  leaf = entryPathName + strlen(dirName) + 1 ;

  while ((dent = readdir (dirp)))           
    { dName = dent->d_name ;
      dLen = strlen (dName) ;
      if (endLen && (dLen <= endLen ||
		     dName[dLen-endLen-1] != '.'  ||
		     strcmp (&dName[dLen-endLen],ending)))
	continue ;

      strcpy (leaf, dName) ;
      if (!filCheck (entryPathName, spec))
	continue ;

      if (ending && dName[dLen - endLen - 1] == '.') /* remove ending */
	dName[dLen - endLen - 1] = 0 ;

      /* the memory of these strings is reclaimed
	 by filDirFinalise, if the returned FilDir object 
	 is destroyed using messfree() */
      array(fdir->entries,  arrayMax(fdir->entries), char*) = 
	strnew (dName, fdir->handle);
    }
  
  closedir (dirp) ;
  
  /************* reorder ********************/
    
  arraySort(fdir->entries, dirOrder) ;
  return fdir ;
} /* filDirCreate */

/*************************************************************/


/* This function copies a file (surprisingly there is no standard Posix      */
/* function to do this). The file is created with read/write permissions     */
/* for the user. Note that it is permissible for the new file to exist, in   */
/* which case its contents will be OVERWRITTEN. This is to allow the caller  */
/* to create a file with a unique name, close it and then supply it as an    */
/* argument to this call (e.g. caller may use aceTmpCreate() to create the   */
/* file).                                                                    */
/*                                                                           */
/* The function returns FALSE for the following errors:                      */
/*                                                                           */
/* 1) supplying a NULL ptr or empty string for either file name.             */
/* 2) if the file to be copied does not exist/is not readable.               */
/* 3) if the file to be created is not writeable.                            */
/* 4) if the copy fails for some other reason (e.g. read/write failed)       */
/*                                                                           */
/* This code is adapted from ffcopy.c from "POSIX Programmers Guide"         */
/* by Donald Levine, publ. by O'Reilly.                                      */
/*                                                                           */
/* It would be handy to use access() to check whether we can read a file     */
/* but this only uses the real UID, not the effective UID. stat() returns    */
/* info. about the file mode but we would need to get hold of all sorts of   */
/* other information (effective UID, group membership) to use it.            */
/*                                                                           */
BOOL filCopyFile(char *curr_name, char *new_name)
{
  BOOL status = TRUE ;
  struct stat file_stat ;
  ssize_t buf_size ;
  size_t bytes_left ;
  enum {BUF_MAX_BYTES = 4194304} ;			    /* Buffer can be up to this size. */
  char *buffer = NULL ;
  int curr_file = -1, new_file = -1 ;

  /* File names supplied ?                                                   */
  if (!curr_name || !(*curr_name) || !new_name || !(*new_name))
    status = FALSE ;

  /* Make sure file to be copied exists and can be opened for reading.       */
  if (status)
    {
      if (stat(curr_name, &file_stat) != 0)
       status = FALSE ;
      else
	{
	  bytes_left = buf_size = file_stat.st_size ;
	  if (buf_size > BUF_MAX_BYTES)
	    buf_size = BUF_MAX_BYTES ;
	  
	  if ((curr_file = open(curr_name, O_RDONLY, 0)) == -1)
	    status = FALSE ;
	}
    }

  /* Make sure file to be written can be opened for writing (O_TRUNC means   */
  /* existing contents of the file will be overwritten).                     */
  if (status)
    {
      if ((new_file = open(new_name, O_WRONLY|O_CREAT|O_TRUNC, S_IRUSR|S_IWUSR)) == -1)
	status = FALSE ;
    }

  
  /* allocate a buffer for the whole file, or the maximum chunk if the file  */
  /* is bigger.                                                              */
  if (status)
    {
      if((buffer = (char *)messalloc(buf_size)) == NULL)
	status = FALSE ;
    }
  

  /* Copy the file. */
  if (status)
    {
      while (bytes_left > 0 && status)
	{
	  if (read(curr_file, buffer, buf_size) != buf_size)
	    status = FALSE ;

	  if (status)
	    {
	      if (write(new_file, buffer, buf_size) != buf_size)
		status = FALSE ;
	    }

	  if (status)
	    {
	      bytes_left -= buf_size;
	      if (bytes_left < buf_size)
		buf_size = bytes_left ;
	    }
	}

    }

  /* Clear up buffer and files.                                              */
  if (buffer != NULL)
    messfree(buffer) ;

  if (curr_file > -1)
    {
      if (close(curr_file) != 0)
	messcrash("close() of file being copied failed.") ;
    }
  if (new_file > -1)
    {
      if (close(new_file) != 0)
	messcrash("close() of file being copied failed.") ;
    }

  return status ;
}



/************************************************************/
/* determines the age of a file, according to its last modification date.

   returns TRUE if the age could determined and the int-pointers
   (if non-NULL will be filled with the numbers).

   returns FALSE if the file doesn't exist, is not readable,
   or the age could not be determined. */
/************************************************************/
BOOL filAge (char *name, char *end,
	     int *diffYears, int *diffMonths, int *diffDays,
	     int *diffHours, int *diffMins, int *diffSecs)
{
  struct stat status;
  mytime_t time_now, time_modified;
  char time_modified_str[25];
  char *filName;
  time_t t;
  struct tm *ts;

  /* get the last-modification time of the file,
     we parse the time into two acedb-style time structs
     in order to compare them using the timediff functions */
  
  if (!(filName = filGetName(name, end, "r", 0)))
    return FALSE;

  if (stat (filName, &status) == -1)
    {
      messfree(filName);
      return FALSE;
    }
  
  messfree(filName);
  
  t = status.st_mtime;
  
  /* convert the time_t time into a tm-struct time */
  ts = localtime(&t);		
  
  /* get a string with that time in it */
  strftime (time_modified_str, 25, "%Y-%m-%d_%H:%M:%S", ts) ;
    
  time_now =      timeNow();
  time_modified = timeParse(time_modified_str);
  
  if (diffYears)
    timeDiffYears (time_modified, time_now, diffYears);
  
    if (diffMonths)
      timeDiffMonths (time_modified, time_now, diffMonths);
    
    if (diffDays)
      timeDiffDays (time_modified, time_now, diffDays);
    
    if (diffHours)
      timeDiffHours (time_modified, time_now, diffHours);
    
    if (diffMins)
      timeDiffMins (time_modified, time_now, diffMins);
    
    if (diffSecs)
      timeDiffSecs (time_modified, time_now, diffSecs);

    
    return TRUE;
} /* filAge */


/*************** end of file ****************/
