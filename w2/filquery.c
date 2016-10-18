/*  File: filquery.c
 *  Author: Richard Durbin (rd@sanger.ac.uk)
 *  Copyright (C) J Thierry-Mieg and R Durbin, 1995
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
 *
 * -------------------------------------------------------------------
 * This file is part of the ACEDB genome database package, written by
 * 	Richard Durbin (MRC LMB, UK) rd@mrc-lmb.cam.ac.uk, and
 *	Jean Thierry-Mieg (CRBM du CNRS, France) mieg@kaa.cnrs-mop.fr
 *
 * Description: filqueryopen()
 * Exported functions:
 * HISTORY:
 * Last edited: May  6 10:13 2003 (edgrif)
 * * Apr 20 14:14 1999 (edgrif): Added graphSetBlockMode call so busy 
 *              cursor works correctly.
 * * Jan 28 14:20 1999 (edgrif): Fix dangling else compiler warnings.
 * * Sep 17 13:19 1998 (edgrif): Replace graph.h with graph_.h, replace
 *              ACEDB defs with function calls.
 * Created: Tue Feb  7 22:24:36 1995 (rd)
 *-------------------------------------------------------------------
 */

/* $Id: filquery.c,v 1.46 2003-05-06 13:13:22 edgrif Exp $ */

/****** package to handle file selection a la Mac *******/


#include <w2/graph_.h>

static char  *dirName, *fileName, *endName;
static char file_filter[32], file_spec[3];
static Array boxes = 0 ;
static int   oldPick = 0 ;
static int   dirBox, fileBox ;
static Graph filGraph = 0 ;
static BOOL editableEnd = FALSE ;

static void dirCancel () { graphUnBlock (FALSE) ; }

static void dirSelect (char *name) { graphUnBlock (TRUE) ; }


/*********************************************************/

static BOOL isMail = FALSE ;
static FILE* returnFil = 0 ;
static char address[128] ;

static void dirDraw (void) ;

static void mailSelect (void)
{
  if ((returnFil = filmail (address)))
    graphUnBlock (FALSE) ;
}

static void dirMail (void)
{	/* can't use graphPrompt because graphBlock non-reentrant */
  graphClear () ;
  oldPick = 0 ;

  graphText ("Address:", 1, 1) ; /* 2nd so it is active */
  fileBox = graphTextScrollEntry (address, 127, 35, 
				  10, 1, (GraphEntryFunc)mailSelect) ;

  graphButton (" OK ", mailSelect, 29, 3) ;
  graphButton ("CANCEL", dirCancel, 36, 3) ;
  graphButton ("FILE", dirDraw, 45, 3) ;
  graphRedraw () ;
}

/*********************************************************/

static void setEnd (char *tt)
{ 
  int i = strlen(file_filter) ;
  while (i < 31) file_filter[i++] = 0 ;
  if (!*file_filter) *file_filter = '*' ;
  dirDraw () ;
}
			/* copy from picksubs.c */
 
static BOOL filterMatch (char *cp,char *tp)
{
  char *c=cp, *t=tp;
  char *ts = 0 , *cs = 0, *s = 0 ;
  int star=0;

  while (TRUE)
    switch(*t)
      {
      case '\0':
 /*
        return (!*c ? ( s ? 1 + (s - cp) : 1) : 0) ;
*/
	if(!*c)
	  return  ( s ? 1 + (s - cp) : 1) ;
	if (!star)
	  return 0 ;
        /* else not success yet go back in template */
	t=ts; c=cs+1;
	if(ts == tp) s = 0 ;
	break ;
      case '?' :
	if (!*c)
	  return 0 ;
	if(!s) s = c ;
        t++ ;  c++ ;
        break;
      case '*' :
        ts=t;
        while( *t == '?' || *t == '*')
          t++;
        if (!*t)
          return s ? 1 + (s-cp) : 1 ;
        while (freeupper(*c) != freeupper(*t))
          if(*c)
            c++;
          else
            return 0 ;
        star=1;
        cs=c;
	if(!s) s = c ;
        break;
      default  :
        if (freeupper(*t++) != freeupper(*c++))
          { if(!star)
              return 0 ;
            t=ts; c=cs+1;
	    if(ts == tp) s = 0 ;
          }
	else
	  if(!s) s = c - 1 ;
        break;
      }
}


static void  filterDirectory (Array dir) 
{ 
  int i, j ; char *cp ;
  for (i = 0, j = 0 ; i < arrayMax(dir) ; i++)
    { 
      cp = arr(dir,i, char*) ;
      if (filterMatch (cp, file_filter))
	arr(dir,j++, char*) = arr (dir, i, char*) ;
    }
  arrayMax(dir) = j ;
}

/*********************************************************/

static void dirDraw (void)
{
  int	ibox = 0, i, line, filBegin, cpLength, cpLengthMax = 20 ;
  FilDir dirList ;
  char *cp, *cq ;

  if ((cp = filGetFullPath (dirName, 0)))
    {
      strcpy (dirName, cp);
      messfree(cp);
    }

  if (dirName[strlen(dirName)-1] == '/')
    dirName[strlen(dirName)-1] = 0 ;

  graphTextScrollEntry (dirName, 0, 0, 0, 0, 0) ;

  if (!filCheckName (dirName, 0, "rd"))
    { messout ("Sorry - %s is not a valid directory", dirName) ;
      return ;
    }

  if (!graphActivate (filGraph))
    return ;

  graphClear () ;
  oldPick = 0 ;

  graphText ("Directory:", 1, 1);
  dirBox = graphTextScrollEntry (dirName, DIR_BUFFER_SIZE-1, 40,
				 11, 1, (GraphEntryFunc)dirDraw) ; 
  	/* note that self is the callback function!
	   OK since graphClear destroys the entry
	*/

  graphButton (" OK ", (GraphFunc)dirSelect, 29, 3) ;
  graphButton ("CANCEL", dirCancel, 36, 3) ;
  if (isMail) 
    graphButton ("MAIL", dirMail, 45, 3) ;

  graphText ("Select from: ", 1, 5) ;

  graphText ("Directories", 2, 6) ;

  if (boxes)
    /* reclaim memory allocated last time */
    {
      for (i = 0 ; i < arrayMax(boxes) ; i++)
	{
	  cp = arr(boxes, i, char*);
	  if (cp)
	    messfree (cp);
	}
      arrayDestroy (boxes);
    }
  boxes = arrayCreate (32, char*) ;

  /* draw directory-entries for selection */
  line = 8 ;

  dirList = filDirCreate(dirName, 0, "rd") ;
  for (i = 0 ; i < filDirMax(dirList) ; i++)
    { 
      cq = filDirEntry(dirList, i) ;

      ibox = graphBoxStart ();

      cp = array(boxes, ibox, char*) = messalloc (strlen(cq)+2) ;
      strcpy (cp, cq) ;
      strcat (cp, "/") ;

      cpLength = strlen(cp) ;
      if (cpLength > cpLengthMax) 
	cpLengthMax = cpLength ;
      graphText (cp, 2, line++) ; 

      graphBoxEnd () ;
    }
  messfree (dirList) ;


  filBegin = cpLengthMax + 5 ;
  line = 6 ;
  graphText ("Filter ", filBegin, 6) ; 
  if (editableEnd)		/* only if select to read */
    {
      graphTextEntry (file_filter, 19,  filBegin + 7, 6, setEnd) ;

      dirList = filDirCreate(dirName, 0, file_spec) ;
      filterDirectory (dirList->entries) ;
    }
  else				/* can't edit the file-extension mask */
    { 
      if (endName && *endName)
	{
	  graphText ("*.", filBegin + 7, 6) ;
	  graphText (endName, filBegin + 9, 6) ;
	}

      dirList = filDirCreate(dirName, endName, "r") ;
    }

   /* mhmp 20.11.98, do not duplicate the endding */
  if (*endName)
    {
    if (editableEnd)
      {
      if (!filterMatch (fileName, file_filter))
	{ 
	  strcat(fileName, ".") ;
	  strcat(fileName, endName) ;
	}
      }
    else if (endName && fileName && filterMatch (fileName, file_filter))
      {
	int endLen = strlen(endName),  nameLen = strlen(fileName) ;
	fileName[nameLen -endLen -1] = '\0' ;
      }
    }

   /*mhmp 20.11.98 tester ici si le  fileName est dans le repertoire*/
  if (editableEnd)
    {
      BOOL isFileInDir = FALSE ;
      
      for (i = 0 ; i < filDirMax(dirList) ; i++)
	{
	  cp = filDirEntry(dirList, i) ;
	  if (!strcmp (cp, fileName))
	    {
	      isFileInDir = TRUE ;
	      break ;
	    }
	}
      if(!isFileInDir)
	{
	if (filDirMax(dirList))
	  strcpy(fileName, filDirEntry(dirList, 0)) ;
	else
	  fileName[0] = '\0' ;
	}
    }

  graphText ("File:",1,3) ; /* after textEntry filter, so it is active */
  fileBox = graphTextScrollEntry (fileName, FIL_BUFFER_SIZE-1, 20, 
				  6, 3,(GraphEntryFunc)dirSelect) ;

  /* draw file-entries for selection */
  line = 8 ;
  /* if we're choosing directories, then ignore the 
     first 2 entries (. and ..), but show all files */
  for (i = (file_spec[1] == 'd' ? 2 : 0) ; i < filDirMax(dirList) ; i++)
    { 
      cq = filDirEntry(dirList, i) ;

      ibox = graphBoxStart ();

      cp = array(boxes, ibox, char*) = messalloc (strlen(cq)+1) ;
      strcpy (cp, cq) ;
      graphText (cp, filBegin, line++) ; 

      graphBoxEnd () ;

      if (strcmp (cp, fileName) == 0)
	{ 
	  oldPick = ibox ;
	  graphBoxDraw (ibox, WHITE, BLACK) ;
	}
    }
  messfree(dirList) ;

  graphTextBounds (60, 8+arrayMax(boxes)) ; 
  graphRedraw () ;
} /* dirDraw */

static void dirPick (int k, double x_unused, double y_unused, int modifier_unused)
{
  char tempName[DIR_BUFFER_SIZE] ;
  int tempLen ;

  if (!k)
    return ;
 
  if (k == oldPick)
    { *tempName = 0 ;
      strcpy (tempName, arr(boxes, k, char*)) ;
      tempLen = strlen (tempName) ;
    
      if (tempName[tempLen-1] != '/') /* is a file */
	{ strcpy (fileName, tempName) ;
	  graphUnBlock (TRUE) ;
	}
      else			/* is a directory */
	{ tempName[tempLen-1] = 0 ;
	  
	  if (!strcmp(tempName, ".."))  /* parent directory */
	    { int prev = 0, curr = 0 ;

	      while (curr < strlen(dirName))
		{ if (dirName[curr] == '/')
		    prev = curr ;
		  curr++ ;
		}
	      if (prev > 0)
		dirName[prev] = 0 ;
	    }
	  else if (strcmp(tempName, ".")) /* anything not self */
	    { strcat(dirName, "/") ;
	      strcat(dirName, tempName) ;
	    }

	  dirDraw() ;
	}
    }
  else
    { if (oldPick)
	graphBoxDraw (oldPick, BLACK, WHITE) ;
      oldPick = 0 ;
      if (k < arrayMax(boxes) && arr(boxes,k,char*))
	{ graphBoxDraw (k, WHITE, BLACK) ;
	  oldPick = k ;
	  *tempName = 0 ;
	  strcpy (tempName, arr(boxes,k,char*)) ;
	  tempLen = strlen (tempName);
	  if (tempName[tempLen-1] != '/') /* is a file */
	    { strcpy (fileName, tempName);
	      graphTextScrollEntry (fileName, 0, 0, 0, 0, 0) ;
	    }
	}
    }
}

/******************/

/* Remove unpleasant characters from file names */

static void cleanName(char *name)
{ 
  char *cp = name ;

  while (*cp)
    { if ((*cp < 'a' || *cp > 'z') && 
	  (*cp < 'A' || *cp > 'Z') &&
	  (*cp < '0' || *cp > '9') &&
	  *cp != '-' && *cp != '~' &&
	  *cp != '.' && *cp != '_' )
	*cp = '_' ;
      cp++ ;
    }
}

     /* uses the graph package to provide directory prompts */
     /* beware that dname should be DIR_BUFFER_SIZE long and 
	         fname FIL_BUFFER_SIZE long */

static char defaultDir[DIR_BUFFER_SIZE], defaultFile[FIL_BUFFER_SIZE] ;

static MENUOPT helpOnlyMenu [] = { 
  {dirCancel,	"Cancel"},
  {help,	"Help"},
  { 0, 0}
} ;

static FILE *localQueryOpen (char *dname, char *fname, 
			     char *end, char *spec, 
			     char *title, BOOL editEnd)
{
  Graph old = graphActive () ;
  static BOOL isInside = FALSE ;
  char path[MAXPATHLEN] ;

  if (isInside)
    return 0 ;
  isInside = TRUE ;

  dirName = dname ? dname : defaultDir ; /* set statics */
  endName = end ;
  editableEnd = editEnd ;

  file_spec[0] = spec[0];	/* NOTE: spec must not be 0x0 */
  if (strlen(spec) > 1)
    { file_spec[1] = spec[1]; file_spec[2] = 0; }
  else
    { file_spec[1] = 0; }

  if (!*dirName) getcwd (dirName, MAXPATHLEN) ; /* empty name => cwd */
  fileName = fname ? fname : defaultFile ;

  if (file_spec[1] == 'd')
    strcpy(file_filter, "*") ;	/* selecting directories */
  else
    {
      strcpy(file_filter, "*.*") ; /* selecting files */
      if (end && *end) strncpy(file_filter+2, end, 28) ;
    }

  isMail = !strcmp (spec, "w") ;

  messStatus ("File Chooser");

  if (!filCheckName (dirName, 0, "rd"))
    { if (getenv ("PWD"))
	strcpy (dirName, getenv("PWD")) ;
      else if (getcwd (path, MAXPATHLEN))
	strcpy (dirName, path) ;
      else
	strcpy (dirName, ".") ;
      if (!filCheckName (dirName, 0, "rd"))
	{ strcpy (dirName, "/") ;
	  if (!filCheckName (dirName, 0, "rd"))
	    { messout ("Sorry - Cannot open any directory") ;
	      return 0 ;
	    }
	}
    }
  
  /* ACEDB-GRAPH INTERFACE: If display function registered call it,
     if not we guess about size of window etc. */
  if (getGraphAcedbDisplayCreate() != NULL &&
      (filGraph =
       (getGraphAcedbDisplayCreate())(getGraphAcedbFilqueryName())
       ))
    {
      if (!graphHelp(0))
	graphHelp ("File_Chooser") ; /* default help unless otherwise 
				      * specified in displays.wrm */
    }
  else
    {
      /* create window with default parameters 
       * and default help subject */
      filGraph = graphCreate (TEXT_SCROLL, "", 0.2, 0.1, 0.5, 0.7) ;
      graphHelp ("File_Chooser") ;
    }


  graphSetBlockMode(GRAPH_BLOCKING) ;


  graphMenu (helpOnlyMenu) ;
  graphRegister (PICK, dirPick) ;

  if (title)
    graphRetitle(title) ;
  dirDraw () ;
  
  returnFil = 0 ;
  while (graphBlock())
    { strcpy (path, dirName) ;
      if (!path || path[strlen(path)-1] != '/') /* avoid // */
	strcat (path, "/") ;
      cleanName (fileName) ;

      if (!strlen(fileName)) /*mhmp 30.11.98 */
	{ messout ("No file chosen !!") ;
	  continue ;
	}
      
      strcat (path, fileName) ;
      if (!editableEnd && *endName)
	{ strcat (path, ".") ;
	  strcat (path, endName) ;
	}

      if (spec[0] == 'w' && filCheckName (path, 0, "r"))
	{ if (messQuery (messprintf ("Overwrite %s?", path)))
	    {
	      if ((returnFil = filopen (path, 0, spec)))
		break ;
	    }
	}
      else if (file_spec[1] == 'd')
	{
	  /* It doesn't make sense to return a FILE * for directories
	     as these are not permitted in general (and break under cygwin)
	     We return 0x1 to indicte success, filclose will notice this
	     and ignore it. Note that all of this is a temporary kludge,
	     Fred will upgrade all of this as part of the aceIn/Out effort */
	  returnFil = (FILE *) 0x1;
	  break;
	}
      if ((returnFil = filopen (path, 0, spec)))
	break ;
      else /* mhmp 11.98 */
	messout ("Sorry, can't open file %s", path);
      dirDraw () ;
    }

  graphDestroy () ;

  filGraph = 0 ;

  isInside = FALSE ;
  graphActivate (old) ;
  return returnFil ;
}

/********************* public functions ********************/

FILE *graphQueryOpen (char *dname, char *fname, char *end, 
		      char *spec, char *title)
     /* This function can also be used to enquire the
      * filename, which was selected, that has to be done with care
      * To assemble the filename after selection
      * take note that it depends whether you open with r or w spec
      * spec "r" : path = "dname" + '/' + "fname"
      * spec "w" : path = "dname" + '/' + "fname" + '.' + "end"
      */
{ 
  FILE *result;

  if (spec[0] == 'r')
    result = localQueryOpen (dname, fname, end, spec, title,
			     TRUE) ; /* editEnd = true */
  else
    result = localQueryOpen (dname, fname, end, spec, title, 
			     FALSE) ; /* editEnd = false */

  return result;
} /* graphQueryOpen */

/********************* end of file ********************/
 
 
 
 
