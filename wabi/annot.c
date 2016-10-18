/*  File: annot.c
 *  Author: Danielle et Jean Thierry-Mieg (mieg@mrc-lmba.cam.ac.uk)
 *  Copyright (C) J Thierry-Mieg and R Durbin, 1993
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
 * 	Richard Durbin (MRC LMB, UK) rd@mrc-lmba.cam.ac.uk, and
 *	Jean Thierry-Mieg (CRBM du CNRS, France) mieg@kaa.cnrs-mop.fr
 *
 * Description:
    A general tool to annotate
 * Exported functions:
 * HISTORY:
 * Last edited: Mar 28 18:56 1996 (ulrich)
 * Created: Thu Dec  9 00:01:50 1993 (mieg)
 *-------------------------------------------------------------------
 */

/* %W% %G% */
/*
#define ARRAY_CHECK
#define MALLOC_CHECK
*/

#include "acedb.h"
#include "bs.h"
#include "graph.h"
#include "whooks/systags.h"
#include "session.h"
#include "bindex.h"
#include "annot.h"
#include "mytime.h"


typedef struct annotStruct { KEY key ; void *magic ; Graph graph ; char remark[1001] ; } *ANNOT ;

static ANNOT annot = 0 ;
static BOOL annotSave () ;

static ANNOTATION *notes = 0 ;


static MENUOPT annotMenu[] =
{
  {graphDestroy , "Quit"},
  /*   {graphHelp, "Help"}, */
  {graphPrint , "Print"},
  {0, 0}
} ;

static void annotDestroy (void)
{
  ANNOTATION* note = &notes[0] ;
  if (annot)
    {
      graphCheckEditors (annot->graph, 0) ;
      annotSave () ;
      annot->magic = 0 ;
      annot->graph = 0 ;
      messfree (annot) ;
    
      note-- ; while ((++note)->type)
	switch (note->type)
	  {
	  case _Text:
	    messfree (note->u.s) ;
	    break ;
	  case 0:
	    break ;
	  }
    }
}

static void annotDisplay (void)
{
  annotDestroy () ;
  annot = (ANNOT) messalloc (sizeof (struct annotStruct)) ;
  annot->magic = &annot ;    
  annot->graph = graphCreate (TEXT_FIT, "Annotator",.2, .2, .4, .6) ;

  graphRegister (DESTROY , annotDestroy) ;
  graphMenu (annotMenu) ;
}

static void annotInit (KEY key) 
{
  char *cp ;
  OBJ Obj = bsCreate (key) ;
  ANNOTATION* note = &notes[0] ;
  annot->key = key ;
  
  note = &notes[0] ; note-- ; 
  while ((++note)->type)
    if (note->tagName && strlen(note->tagName))
      note->tag = str2tag (note->tagName) ;
  
  note = &notes[0] ; note-- ; 
  while ((++note)->type)
    if (note->tag) switch (note->type)
      {
	case _Text:
	  if (!note->u.s)
	    note->u.s = messalloc (note->length + 1) ;
	  memset (note->u.s, 0, note->length) ;
	  if (Obj && bsGetData (Obj, note->tag, _Text, &cp))
	    strncpy (note->u.s, cp, note->length) ;
	  break ;
	case _Int:
	  note->u.i = 0 ;
	  if (Obj) bsGetData (Obj, note->tag, _Int, &(note->u.i)) ;
	  break ;
	case _Unsigned:
	  note->u.i = 0 ;
	  if (Obj && bsFindTag (Obj, note->tag))
	    note->u.i = 1 ;
	  break ;
	case _Float:
	  note->u.f = 0 ;
	  if (Obj) bsGetData (Obj, note->tag, _Float, &(note->u.f)) ;
	  break ;
	case _DateType:
	  note->u.f = 0 ;
	  if (Obj) bsGetData (Obj, note->tag, _DateType, &(note->u.time)) ;
	  break ;
      default:
	note->u.i = bsFindTag (Obj, note->tag) ;
	break ;
      }    
  bsDestroy (Obj) ;
}

static BOOL annotSave ()
{
  ANNOTATION* note = &notes[0] ;
  OBJ Obj = annot ? bsUpdate (annot->key) : 0 ;
  
  if (!Obj)
    return FALSE ;
  sessionGainWriteAccess() ;
  graphCheckEditors (annot->graph, TRUE) ; 
  note-- ; while ((++note)->type)
    if (note->tag) switch (note->type)
      {
      case _Text:
	if (note->u.s && *(note->u.s))
	  bsAddData (Obj, note->tag, _Text, note->u.s) ;
	else if (bsFindTag (Obj, note->tag))
	  bsRemove (Obj) ;
	break ;
      case _Int:
	if (note->u.i)
	  bsAddData (Obj, note->tag, _Int, &(note->u.i)) ;
	else if (bsFindTag (Obj, note->tag))
	  bsRemove (Obj) ;
	break ;
      case _Unsigned:
	if (note->u.i)
	  bsAddTag (Obj, note->tag) ;
	else if (bsFindTag (Obj, note->tag))
	  bsRemove (Obj) ;
	break ;
      case _Float:
	if (note->u.f != 0.0)
	  bsAddData (Obj, note->tag, _Float, &(note->u.f)) ;
	else if (bsFindTag (Obj, note->tag))
	  bsRemove (Obj) ;
	break ;
      case _DateType:
	if (note->u.time)
	  bsAddData (Obj, note->tag, _DateType, &(note->u.time)) ;
	else if (bsFindTag (Obj, note->tag))
	  bsRemove (Obj) ;
	break ;
      default:
	if (note->u.i)
	  bsAddTag (Obj, note->tag) ;
	else if (bsFindTag (Obj, note->tag))
	  bsRemove (Obj) ;
	break ;
      }
  bsSave (Obj) ;
  return TRUE ;
}

static void annotDraw ()
{
  int line = 1 ;
  ANNOTATION* note = &notes[0] ;

  graphClear () ;
  
  graphButton ("OK", graphDestroy, 30, line) ;
  graphText ("Gene:", 1, line) ;
  graphText (name(annot->key), 8, line++) ; line++ ;
  note-- ; while ((++note)->type)
    if (note->tag) switch (note->type)
      {
      case _Text:
	graphTextEditor (note->title, note->u.s, note->length, note->x, line++, 0) ;
	break ;
      case _Int:
	graphIntEditor (note->title, &(note->u.i), note->x, line++, 0) ;
	break ;
      case _Unsigned:
	graphIntEditor (note->title, &(note->u.i), note->x, line++, 0) ;
	break ;
      case _Float:
	graphFloatEditor (note->title, &(note->u.f), note->x, line++, 0) ;
	break ;
      case _DateType:
	graphText (note->title, note->x, line) ;
	graphText (timeShow (note->u.time), 5 + strlen(note->title), line++) ;
	break ;
      default:
	graphToggleEditor (note->title,  &(note->u.i), note->x, line++) ;
	break ;
      }
    else
      graphText (note->title, note->x, line++) ;
  graphRedraw () ;
}

void annotate (KEY key, ANNOTATION *an)
{
 
  notes = an ;
  if (!annot || !graphActivate (annot->graph)) 
    annotDisplay () ;
  if (annot->key != key)
    annotInit (key) ;
  graphPop () ;
  annotDraw () ;
}
