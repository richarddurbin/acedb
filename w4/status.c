/*  File: status.c
 *  Author: Jean Thierry-Mieg (mieg@mrc-lmb.cam.ac.uk)
 *  Copyright (C) J Thierry-Mieg and R Durbin, 1991
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
 * Description: get status of program memory and acedb kernel
 * Exported functions:
 *              tStatus - output status info freeOut
 * HISTORY:
 * Last edited: Nov 22 11:58 2007 (edgrif)
 * * May 12 13:27 1999 (fw): moved out all code to statusdisp.c
 *              moved all non-graphical status code here
 * * Dec  3 14:43 1998 (edgrif): Change calls to new interface to aceversion.
 * Created: Fri Nov 29 12:20:30 1991 (mieg)
 * CVS info:   $Id: status.c,v 1.38 2007-11-26 17:12:25 edgrif Exp $
 *-------------------------------------------------------------------
 */
 
#include <wh/acedb.h>
#include <wh/aceio.h>
#include <wh/lex.h>					    /* for lexavail */
#include <wh/disk.h>					    /* for diskavail */
#include <wh/block.h>					    /* for blockavail() */
#include <wh/bs.h>					    /* for BSstatus BTstatus */
#include <wh/session.h>					    /* for isWriteAccess */
#include <wh/dbpath.h>
#include <wh/bindex.h>					    /* for bIndexVersion */
#include <wh/a.h>					    /* for aStatus */
#include <wh/status.h>

/*************************************************************/

typedef enum _STATUS_LINE_TYPE {LINE_SUBHEAD,
				LINE_START, LINE_CONT, LINE_END} STATUS_LINE_TYPE ;


enum {STATUS_TITLE_INDENT = 1, STATUS_MIN_INDENT = 5, STATUS_JUSTIFY = 20} ;

/* For holding mapping of cmdline strings to option flags.                   */
typedef struct _StatusRequest
{
  StatusType type ;
  char *string ;
} StatusRequest ;


/*************************************************************/

static void dumpStatus(ACEOUT fo, StatusType flag, char *user_title) ;
static void doTitle(ACEOUT fo, char *text) ;
static void doSimpleLine(ACEOUT fo, char *text) ;
static void doSingleLine(ACEOUT fo, char *label, char *value) ;
static void doLine(ACEOUT fo, char *label, char *value, STATUS_LINE_TYPE type) ;
static void myText (ACEOUT fo, char *cp, int indent, BOOL startline, BOOL endline) ;




/*************************************************************/

/* Keep in step with StatusType and STATUS_OPTIONS_STRING in status.h        */
/*                                                                           */
static StatusRequest status_types_G[] =
{
  {STATUS_CODE, "-code"},
  {STATUS_DATABASE, "-database"},
  {STATUS_DISK, "-disk"},
  {STATUS_LEXIQUES, "-lexiques"},
  {STATUS_HASHES, "-hash"},
  {STATUS_INDEXING, "-index"},
  {STATUS_CACHE, "-cache"},
  {STATUS_CACHE1, "-cache1"},
  {STATUS_CACHE2, "-cache2"},
  {STATUS_ARRAYS, "-array"},
  {STATUS_MEMORY, "-memory"},
#ifdef ACEMBLY
  {STATUS_DNA, "-dna"},
#endif
  {STATUS_ALL, "-all"}
} ;

static int status_num_G = UtArraySize(status_types_G) ;


/*************************************************************/


/* Print all possible stats.                                                 */
void tStatus(ACEOUT fo)
{
  dumpStatus(fo, STATUS_ALL, NULL) ;

  return ;
}



/* Output stats with title, usefull for debugging, insert at various places in code. */
void tStatusWithTitle(ACEOUT fo, char *title)
{
  dumpStatus(fo, STATUS_MEMORY, title) ;

  return ;
}


/* Print a selected subset of stats as specified by the value of "flag"      */
/*                                                                           */
void tStatusSelect(ACEOUT fo, StatusType flag)
{
  dumpStatus(fo, flag, NULL) ;

  return ;
}


/* Check a given cmdline flag of form "-XXXX" and if its recognised then set */
/* the supplied flag and return TRUE, else return FALSE.                     */
/*                                                                           */
BOOL tStatusSetOption(char *cmdline_option, StatusType *flag)
{
  BOOL result = FALSE ;
  int i ;

  for (i = 0 ; i < status_num_G && result == FALSE ; i++)
    {
      if (strcmp(cmdline_option, status_types_G[i].string) == 0)
	{
	  result = TRUE ;
	  *flag |= status_types_G[i].type ;
	}
    }

  return result ;
}





/* 
 *                  Internal routines.
 */



/* The function that actually dumps all the stats. */
static void dumpStatus(ACEOUT fo, StatusType flag, char *user_title)
{
  char *db_name ;
  char *main_window ;
  char *last_save_time ;
  extern int blockMax (void) ;
  extern int NII, NIC, NWC, NWF, NWG; /* in lexsubs, hash statistics */
  unsigned long keynum,keynum2, vocnum,vocspace,hspace,tspace ;
  int
    n,
    dused, dfree, plus, minus,
    bsused, bsalloc, btused, btalloc, 
    aUsed, aMade, aAlloc, aReal, 
    nmessalloc, messalloctot,
    bused, bpinned, bfree, bmodif,bsmemory, 
    indexT, indexKb, indexV,
    cacheused, cachel, cachek, cachemod, 
    ndread, ndwrite,nc1r,nc1w,ncc1r, ncc1w, naread,nawrite,nc2read,nc2write,nccr,nccw;
  STORE_HANDLE handle = handleCreate();
#ifdef ACEMBLY
  int dnaMade = 0, dnaTotal = 0 ;
  monDnaReport (&dnaMade, &dnaTotal) ;
#endif
  
  if (fo == NULL)
    messcrash("Internal Logic Error: ACEOUT ptr to tStatus() was NULL") ;


  doSimpleLine(fo, "************************************************") ;


  if (user_title)
    {
      doSimpleLine(fo, "") ;
      doSimpleLine(fo, user_title) ;
      doSimpleLine(fo, "") ;
    }

  if ((db_name = sessionDbName()) == NULL)
    db_name = "<undefined>" ;
  if ((main_window = sessionDBTitle()) == NULL)
    main_window = "<undefined>" ;
  last_save_time = sessionLastSaveTime() ;

  ndwrite = 123 ; diskavail (&dfree,&dused, &plus, &minus,&ndread,&ndwrite) ;
  lexavail (&vocnum,&keynum,&keynum2,&vocspace, &hspace,&tspace);
  BSstatus (&bsused, &bsalloc, &n) ; bsmemory = n ;
  BTstatus (&btused, &btalloc, &n) ; bsmemory += n ;
  arrayStatus (&aMade, &aUsed, &aAlloc, &aReal) ;
  aStatus (&naread, &nawrite) ;
  nmessalloc = messAllocStatus (&messalloctot) ;
  blockavail(&bused,&bpinned,&bfree,&bmodif) ; 
  indexV =  bIndexStatus (&indexT, &indexKb) ;
  blockStatus (&nc1r, &nc1w, &ncc1r, &ncc1w) ;
  cacheStatus(&cacheused,&cachel,&cachek,&cachemod,&nc2read,&nc2write,&nccr,&nccw) ;
  
  /* title and datestamp                                                     */
  doSimpleLine(fo, messprintf("AceDB status at %s", timeShow(timeNow()))) ; 
  doSimpleLine(fo, "") ;

  /* Info. about the acedb program itself.                                   */
  if ((flag & STATUS_ALL) || (flag & STATUS_CODE))
    {
      doTitle(fo, "Code") ;
      doSingleLine(fo, "Program", messGetErrorProgram()) ;
      doSingleLine(fo, "Version", aceGetVersionString()) ;
      doSingleLine(fo, "Build", aceGetLinkDate()) ;
    }

  /* Database name/version info.                                             */
  if ((flag & STATUS_ALL) || (flag & STATUS_DATABASE))
    {
      doTitle(fo, "Database") ;
      doSingleLine(fo, "Title", main_window) ;
      doSingleLine(fo, "Name", db_name) ;
      doSingleLine(fo, "Release", messprintf("%d_%d",
					     thisSession.mainDataRelease, thisSession.subDataRelease)) ;
      doSingleLine(fo, "Directory", dbPathMakeFilName("", "", "", handle)) ;
      doSingleLine(fo, "Session", messprintf("%d", thisSession.session)) ;
      doSingleLine(fo, "User", getLogin(TRUE)) ;
      doSingleLine(fo, "Last Save", last_save_time) ;
      doSingleLine(fo, "Write Access", (isWriteAccess() ? "Yes" : "No" )) ;
      doSingleLine(fo, "Global Address", messprintf("%d", thisSession.gAddress)) ;
    }
  /* Disk blocks.                                                            */
  if ((flag & STATUS_ALL) || (flag & STATUS_DISK))
    {
      doTitle(fo, "Disk blocks") ;
      doLine(fo, "Used", messprintf("%d", dused), LINE_START) ;
      doLine(fo, "Available", messprintf("%d", dfree), LINE_END) ;
      doLine(fo, "This Session", "", LINE_SUBHEAD) ;
      doLine(fo, "allocated",  messprintf("%d", plus), LINE_CONT) ;
      doLine(fo, "freed", messprintf("%d", minus), LINE_END) ;
    }

  if ((flag & STATUS_ALL) || (flag & STATUS_LEXIQUES))
    {
      doTitle(fo, "Lexiques") ;
      doLine(fo, "classes", messprintf("%lu", vocnum), LINE_START) ;
      doLine(fo, "allocated (kb)", messprintf("%lu", tspace), LINE_END) ;
      
      doLine(fo, "keys", messprintf("%lu", keynum), LINE_START) ;
      doLine(fo, "keys in lex2", messprintf("%lu", keynum2), LINE_CONT) ;
      doLine(fo, "names (kb)", messprintf("%lu", vocspace/1024), LINE_CONT) ;
      doLine(fo, "hashing keys", messprintf("%lu", hspace), LINE_END) ;
    }

  if ((flag & STATUS_ALL) || (flag & STATUS_HASHES))
    {
      doTitle(fo, "Hashes") ;
      doLine(fo, "nii", messprintf("%d", NII), LINE_START) ;
      doLine(fo, "nic", messprintf("%d", NIC), LINE_CONT) ;
      doLine(fo, "nwc", messprintf("%d", NWC), LINE_CONT) ;
      doLine(fo, "nwf", messprintf("%d", NWF), LINE_CONT) ;
      doLine(fo, "nwg", messprintf("%d", NWG), LINE_END) ;
    }

  if ((flag & STATUS_ALL) || (flag & STATUS_INDEXING))
    {
      doTitle(fo, "Indexing") ;
      if (!indexV)
	doSingleLine(fo, "Not Available", "Please read models "
		     "and avoid using code prior to 4_7)") ;
      else
	{
	  doLine(fo, "version", messprintf("%d", indexV), LINE_START) ;
	  doLine(fo, "tables", messprintf("%d", indexT), LINE_CONT) ;
	  doLine(fo, "kb", messprintf("%d", indexKb), LINE_END) ;
	}
    }

  if ((flag & STATUS_ALL) || (flag & STATUS_CACHE1))
    {
      doTitle(fo, "Cache 1") ;
      blockavail(&bused, &bpinned, &bfree, &bmodif) ;
      doSingleLine(fo, "Size", messprintf("%d",blockMax())) ;
      doLine(fo, "Current Blocks", "", LINE_SUBHEAD) ;
      doLine(fo, "used", messprintf("%d", bused), LINE_CONT) ;
      doLine(fo, "modified", messprintf("%d", bmodif), LINE_CONT) ;
      doLine(fo, "locked", messprintf("%d", bpinned), LINE_CONT) ;
      doLine(fo, "free", messprintf("%d", bfree), LINE_END) ;
      doLine(fo, "Current Objects", "", LINE_SUBHEAD) ;
      doLine(fo, "served", messprintf("%d", nc1r), LINE_CONT) ;
      doLine(fo, "saved", messprintf("%d", nc1w), LINE_CONT) ;
      doLine(fo, "read from disk", messprintf("%d", ncc1r), LINE_CONT) ;
      doLine(fo, "saved to disk", messprintf("%d", ncc1w), LINE_END) ;
    }


  if ((flag & STATUS_ALL) || (flag & STATUS_CACHE2))
    {
      doTitle(fo, "Cache 2") ;
      doLine(fo, "slots", messprintf("%d", cacheused), LINE_START) ;
      doLine(fo, "allocated (kb)", messprintf("%d", bsmemory/1024), LINE_END) ;
      
      doLine(fo, "Current content (objs)", "", LINE_SUBHEAD) ;
      doLine(fo, "locked", messprintf("%d", cachel), LINE_CONT) ;
      doLine(fo, "known", messprintf("%d", cachek), LINE_CONT) ;
      doLine(fo, "modified", messprintf("%d", cachemod), LINE_END) ;
  
      doLine(fo, "Object Usage", "", LINE_SUBHEAD) ;
      doLine(fo, "served", messprintf("%d", nc2read), LINE_CONT) ;
      doLine(fo, "saved", messprintf("%d", nc2write), LINE_CONT) ;
      doLine(fo, "read from cache1", messprintf("%d", nccr), LINE_CONT) ;
      doLine(fo, "saved to cache1", messprintf("%d", nccw), LINE_END) ;

      doLine(fo, "BS memory cells", "", LINE_SUBHEAD) ;
      doLine(fo, "used", messprintf("%d", bsused), LINE_CONT) ;
      doLine(fo, "allocated", messprintf("%d", bsalloc), LINE_END) ;
      doLine(fo, "BT memory cells", "", LINE_SUBHEAD) ;
      doLine(fo, "used", messprintf("%d", btused), LINE_CONT) ;
      doLine(fo, "allocated", messprintf("%d", btalloc), LINE_END) ;
    }
      
   if ((flag & STATUS_ALL) || (flag & STATUS_ARRAYS))
     {
       doTitle(fo, "Arrays (Including the lexiques)") ;
       doLine(fo, "made", messprintf("%d", aMade), LINE_START) ;
       doLine(fo, "current", messprintf("%d", aUsed), LINE_CONT) ;
       doLine(fo, "allocated (kb)", messprintf("%d", aAlloc/1024), LINE_CONT) ;
       doLine(fo, "used (kb)", messprintf("%d", aReal/1024), LINE_END) ;
     }

#ifdef ACEMBLY
  if ((flag & STATUS_ALL) || (flag & STATUS_DNA))
    {
      doTitle(fo, "Acembly DNA Cache") ;
      doLine(fo, "arrays", messprintf("%d", dnaMade), LINE_START) ;
      doLine(fo, "total (kb)", messprintf("%d", dnaTotal/1024), LINE_END) ;
    }
#endif

  if ((flag & STATUS_ALL) || (flag & STATUS_CACHE))
    {
      doTitle(fo, "Cache statistics: each layer serves the known objects or asks the lower layer") ;
      doLine(fo, "Array", "", LINE_SUBHEAD) ;
      doLine(fo, "read from cache1", messprintf("%d", naread), LINE_CONT) ;
      doLine(fo, "saved to cache1", messprintf("%d", nawrite), LINE_END) ;
  
      doLine(fo, "Cache2", "", LINE_SUBHEAD) ;
      doLine(fo, "served", messprintf("%d", nc2read), LINE_CONT) ;
      doLine(fo, "saved", messprintf("%d", nc2write), LINE_CONT) ;
      doLine(fo, "read from cache1", messprintf("%d", nccr), LINE_CONT) ;
      doLine(fo, "saved to cache1", messprintf("%d", nccw), LINE_END) ;

      doLine(fo, "Cache1", "", LINE_SUBHEAD) ;
      doLine(fo, "served", messprintf("%d", nc1r), LINE_CONT) ;
      doLine(fo, "saved", messprintf("%d", nc1w), LINE_CONT) ;
      doLine(fo, "read from disk", messprintf("%d", ncc1r), LINE_CONT) ;
      doLine(fo, "saved to disk", messprintf("%d", ncc1w), LINE_END) ;

      doLine(fo, "Disk blocks", "", LINE_SUBHEAD) ;
      doLine(fo, "served", messprintf("%d", ndread), LINE_CONT) ;
      doLine(fo, "saved", messprintf("%d", ndwrite), LINE_END) ;
    }

  if ((flag & STATUS_ALL) || (flag & STATUS_MEMORY))
    {
      doTitle(fo, "Overall Memory Usage (caches 1 & 2 + arrays + various)") ;
      if (messalloctot)
	{
	  doLine(fo, "Messalloc", "", LINE_SUBHEAD) ;
	  doLine(fo, "blocks", messprintf("%d", nmessalloc), LINE_CONT) ;
	  doLine(fo, "allocated (kb)", messprintf("%d", messalloctot/1024), LINE_END) ;
	}
      else
	doLine(fo, "Messalloc - blocks", messprintf("%d", nmessalloc), LINE_START) ;
    }
  
  doSimpleLine(fo, "") ;
  doSimpleLine(fo, "************************************************\n") ;

 
  handleDestroy(handle);
  messfree(last_save_time) ;

  return;
} /* tStatus */


static void doTitle(ACEOUT fo, char *text)
{
  char *msg_text = NULL ;

  msg_text = hprintf(0, "- %s", text) ;
  myText(fo, msg_text, STATUS_TITLE_INDENT, TRUE, TRUE) ;
  messfree(msg_text) ;

  return ;
}

static void doSimpleLine(ACEOUT fo, char *text)
{
  char *msg_text = NULL ;
  
  msg_text = hprintf(0, "%s", text) ;
  myText(fo, msg_text, STATUS_TITLE_INDENT, TRUE, TRUE) ;
  messfree(msg_text) ;

  return ;
}


static void doSingleLine(ACEOUT fo, char *label, char *value)
{
  int indent = 0 ;
  char *msg_text = NULL ;

  if ((indent = STATUS_JUSTIFY - strlen(label)) < STATUS_MIN_INDENT)
    indent = STATUS_MIN_INDENT ;

  msg_text = hprintf(0, "%s: %s", label, value) ;
  myText(fo, msg_text, indent, TRUE, TRUE) ;
  messfree(msg_text) ;

  return ;
}


static void doLine(ACEOUT fo, char *label, char *value, STATUS_LINE_TYPE type)
{
  int indent = 0 ;
  BOOL start, end ;
  char *separator = "" ;
  char *msg_text = NULL ;

  if (type == LINE_SUBHEAD || type == LINE_START)
    {
      start = TRUE ;
      if ((indent = STATUS_JUSTIFY - strlen(label)) < STATUS_MIN_INDENT)
	indent = STATUS_MIN_INDENT ;
    }
  else
    {
      start = FALSE ;
      indent = 2 ;
    }

  if (type == LINE_END)
    {
      end = TRUE ;
      separator = "" ;
    }
  else
    {
      end = FALSE ;
      separator = "," ;
    }
  
  if (type == LINE_SUBHEAD)
    {
      msg_text = hprintf(0, "%s -", label) ;
      myText(fo, msg_text, indent - 1, start, end) ;
    }
  else
    {
      msg_text = hprintf(0, "%s: %s%s", label, value, separator) ;
      myText(fo, msg_text, indent, start, end) ;
    }
  messfree(msg_text) ;

  return ;
}


static void myText(ACEOUT fo, char *cp, int indent, BOOL startline, BOOL endline)
{ 
  char ww[1000] ;
  int i = 990 ;
  char *cq = &ww[0] ;

  if (indent < 0)
    indent = 0 ;

  if (startline)
    {
      *cq++ = ' ' ; *cq++ = '/' ;   *cq++ = '/' ; 
    }

  while (indent--) *cq++ = ' ' ;
  while (i-- && *cp) *cq++ = *cp++ ;

  if (endline)
    *cq++ = '\n' ;

  *cq++ = 0 ;

  aceOutPrint(fo, "%s", ww) ;

  return;
}



/*************************** eof ****************************/
