/*  File: filclean.c
 *  Author: Jean Thierry-Mieg (mieg@mrc-lmb.cam.ac.uk)
 *  Copyright (C) J Thierry-Mieg and R Durbin, 1992
 *-------------------------------------------------------------------
 * This file is part of the ACEDB genome database package, written by
 * 	Richard Durbin (MRC LMB, UK) rd@mrc-lmb.cam.ac.uk, and
 *	Jean Thierry-Mieg (CRBM du CNRS, France) mieg@kaa.cnrs-mop.fr
 *
 * Description:
 **   file opening routines - Unix version
 * Exported functions:
 * HISTORY:
 * Last edited: Oct  3 17:18 1994 (rd)
 * * Oct 24 17:15 1993 (mieg): added filqueryopen
 * Created: Mon Jun 15 14:43:01 1992 (mieg)
 *-------------------------------------------------------------------
 */

/* $Id: filclean.c,v 1.4 1994-10-25 21:07:23 mieg Exp $ */

#include "regular.h"
#include "array.h"

#include <sys/file.h>

#include <sys/types.h>
#include <mydirent.h>	/* drawDir directory operations */
 
static char fullname[DIR_BUFFER_SIZE] = "./" ;
static char *partname = &fullname[2] ;
static char olddir[DIR_BUFFER_SIZE] ;

/******************************/

char *filsetdir (char *s)       /* returns old name - sets if s != 0 */
{
  *partname = 0 ;
  strcpy (olddir,fullname) ;
 
  if (s)
    { strcpy (fullname,s) ;
      strcat (fullname,"/") ;
      partname = &fullname[strlen(fullname)] ;
    }
  olddir[strlen(olddir)-1] = 0 ;
  return olddir ;
}

/*******************************/

static char *filName (char *name, char *ending)
{
  if (!name)
    messcrash ("filexists received a nul parameter") ;
  
  strcpy (partname,name) ;
  if (ending && *ending)
    { strcat (partname,".") ;
      strcat (partname,ending) ;
    }

  if (*name == '.' || *name == '/')
    return partname ;
  else
    return fullname ;
}

BOOL filexists (char *name, char *ending) /* true if file exists. -HJC */
{
  return access (filName (name, ending), F_OK) ? FALSE : TRUE ;
}

BOOL filremove (char *name, char *ending) /* TRUE if file is deleted. -HJC*/
{
  return unlink (filName (name, ending)) ? FALSE : TRUE ;
}

FILE *filopen (char *name, char *ending, char *spec)
{
  FILE *result ;
   
  if (!(result = fopen (filName (name, ending), spec)))
    messerror ("Failed to open %s", filName (name, ending)) ;
  return result ;
}
/********************* temporary file stuff *****************/

static Associator tmpFiles = 0 ;

FILE *filtmpopen (char **nameptr, char *spec)
{
  if (!nameptr)
    messcrash ("filtmpopen requires a non-null nameptr") ;

  if (!strcmp (spec, "r"))
    return filopen (*nameptr, 0, spec) ;

  if (!(*nameptr = tempnam ("/var/tmp", "ACEDB")))
    { messerror ("failed to create temporary file name") ;
      return 0 ;
    }
  if (!tmpFiles)
    tmpFiles = assCreate () ;
  assInsert (tmpFiles, *nameptr, *nameptr) ;

  return filopen (*nameptr, 0, spec) ;
}

BOOL filtmpremove (char *name)	/* delete and free()  */
{ 
  int i ;
  BOOL result = filremove (name, 0) ;
  free (name) ;	/* NB free since allocated by tempnam */
  assRemove (tmpFiles, name) ;
  return result ;
}

void filtmpcleanup (void)
{
  char *name = 0 ;
  if (tmpFiles)
    while (assNext (tmpFiles, &name, 0))
      { filremove (name, 0) ;
	free (name) ;
      }
}

/*******************************/

static Associator mailFile = 0, mailAddress = 0 ;

void filclose (FILE *fil)
{
  char *address ;
  char *filename ;

  if (!fil)
    return ;
  fclose (fil) ;
  if (mailFile && assFind (mailFile, fil, &filename))
    { if (assFind (mailAddress, fil, &address))
	callScript ("mail", messprintf ("%s %s", address, filename)) ;
      else
	messerror ("Have lost the address for mailfile %s",
		   filename) ;
      assRemove (mailFile, fil) ;
      assRemove (mailAddress, fil) ;
      unlink (filename) ;
      free (filename) ;
    }
}

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
}

/*******************************/

FILE *filqueryopen (char *dname, char *fname, char *end, char *spec, char *title)
{ Stack s = stackCreate(50) ;
  FILE*	fil = 0 ;
  int i ;

  if (dname && *dname)
    { pushText(s, dname) ; catText(s,"/") ; }
  if (fname)
    catText(s,fname) ; 
  if (end && *end)
    { catText(s,".") ; catText(s,end) ; }

 lao:
  if (!messPrompt("File name please", stackText(s,0), "w")) 
    { stackDestroy(s) ;
      return 0 ;
    }
  i = stackMark(s) ;
  pushText(s, freeword()) ;
  if (spec[0] == 'w' && 
      (fil = fopen (stackText(s,i), "r")))
    { fclose (fil) ; 
      fil = 0 ;
      if (messQuery (messprintf ("Overwrite %s?",
				 stackText(s,i))))
	if (fil = fopen (stackText(s,i), spec))
	  goto lao ;
	else
	  messout ("Sorry, can't open file %s for writing",
		   stackText (s,i)) ;
    }
  else if (!(fil = fopen (stackText(s,i), spec))) 
    messout ("Sorry, can't open file %s",
	     stackText(s,i)) ;
  stackDestroy(s) ;
  return fil ;
}
/*************** end of file ****************/
