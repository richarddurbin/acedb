/*  File: matchtable.c
 *  Author: Richard Durbin (rd@sanger.ac.uk)
 *  Copyright (C) J Thierry-Mieg and R Durbin, 1996
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
 * 	Richard Durbin (MRC LMB, UK) rd@sanger.ac.uk, and
 *	Jean Thierry-Mieg (CRBM du CNRS, France) mieg@kaa.cnrs-mop.fr
 *
 * SCCS: $Id: matchtable.c,v 1.13 2000-08-03 14:02:00 edgrif Exp $
 * Description: code for array class MatchTable
 * Exported functions:
 * HISTORY:
 * Last edited: Jul 28 16:17 2000 (edgrif)
 * Created: Thu Nov 14 20:59:06 1996 (rd)
 *-------------------------------------------------------------------
 */

#include "acedb.h"
#include "aceio.h"
#include "table.h"
#include "lex.h"
#include "bs.h"
#include "pick.h"
#include "whooks/sysclass.h"
#include "matchtable.h"
#include "session.h"		/* for sessionUserKey() */
#include "a.h"

/*
   object name is 
   	className(query) '-' name(query)

   expected .ace file format is rows of:
   
	[-D] TargetClass TargetName Method Score Q1 Q2 T1 T2 [-D] [flags]

   Multiple flags are given separated by '|' without surrounding space,
     e.g.  Bury|EMBL_dump_NO
   -D at the start of a line deletes the whole record
   -D before the flags deletes just those flags

   Internally, a sorted array of MATCH (defined in matchtable.h)
   Note that sorting is important for performance merging in new data.
*/

static int endKey ;
static MATCH endMarker ;
static FLAGSET FLAG_DELETE, FLAG_USED ;
static BOOL isInternalArrayKill = FALSE ;

static int saveMatch (Array ma, KEY tabKey, KEY query) ;

/****************************************************************/

static KEY _Match_table ;
static int _VMatchTable ;
static int _VMethod ;

static FLAGSET saveMask ;
	/* saveMask is to remove flags we do not want to save */

static BOOL  initialise (void)
{ 
  static int done = FALSE ;

  if (!done)
    { KEY key ;

      lexword2key ("MatchTable", &key, _VMainClasses) ;
      _VMatchTable = KEYKEY(key) ;
      if (!key)
	return FALSE ;

      lexword2key ("Method", &key, _VMainClasses) ;
      _VMethod = KEYKEY(key) ;
      if (!key)
	return FALSE ;

      lexaddkey ("Match_table", &_Match_table, 0) ;

      endKey = KEYMAKE(_VMatchTable, 0xffffff) ;
      endMarker.key = endKey ;

      FLAG_DELETE = flag ("Match", "DELETE") ;
      FLAG_USED = flag ("Match", "USED") ;
      saveMask = 0xffff & ~(FLAG_DELETE | FLAG_USED) ;

      done = TRUE ;
    }
  return TRUE ;
}

/************************************************************/

static int rowCompare (void *a, void *b)
{
  MATCH *ma = (MATCH*)a ;
  MATCH *mb = (MATCH*)b ;
  int diff ;
  float fdiff ;

  diff = ma->key - mb->key ;
  if (diff) return diff ;
  diff = ma->q1 - mb->q1 ; 
  if (diff) return diff ;
  diff = ma->q2 - mb->q2 ; 
  if (diff) return diff ;
  diff = ma->t1 - mb->t1 ; 
  if (diff) return diff ;
  diff = ma->t2 - mb->t2 ; 
  if (diff) return diff ;
  diff = ma->meth - mb->meth ;
  if (diff) return diff ;
  fdiff = ma->score - mb->score ; 
  if (fdiff) return fdiff > 0 ? 1 : -1 ;
  return 0 ;
}

/*****************************************************************/

static void rowAdd (Array new, MATCH *m)
{ 
  MATCH *x = arrayp (new, arrayMax(new), MATCH) ;

  *x = *m ;
  x->flags &= saveMask ;
}

static void reverseRowAdd (Array new, MATCH *m, KEY k)
{ 
  MATCH *x = arrayp (new, arrayMax(new), MATCH) ;

  *x = *m ;

  x->key = k ;
  x->q1 = m->t1 ;
  x->q2 = m->t2 ;
  x->t1 = m->q1 ;
  x->t2 = m->q2 ;
  x->flags &= saveMask ;
}

/************************************************************/

static BOOL matchKey2Parent (KEY k, KEY *kp)
{ 
  int classe ;
  char *s = name(k) ;
  char *s1 ;
  BOOL result = FALSE ;

  for (s1 = s ; *s1 && *s1 != '-' ; ++s1) ;
  if (!*s1)
    return FALSE ;

  *s1 = 0 ;
  if ((classe = pickWord2Class(s)))
    { lexaddkey (s1+1, kp, classe) ;
      if (KEYKEY(*kp))
	result = TRUE ;
    }

  *s1 = '-' ;
  return result ;
}

/************************************************************/

/*** parse ****
  Strategy is to read into a new table, sort, then merge with
  the existing one (if any).  This avoids linear operations, and
  makes XREF updating more efficient.
****/
ParseFuncReturn matchTableParse (ACEIN parse_io, KEY tableKey, char **errtext)
     /* ParseFuncType */
{
  int classe, nt, ix ;
  float fx ;
  KEY parent, key ;
  char *s ;
  static Array ma, ma2 ;
  static Associator mHash ;
  MATCH *m ;
  KEY sessionKey = sessionUserKey() ;
  ParseFuncReturn result ;

#define ABORT(_s)                                                \
    {                                                            \
      *errtext = messprintf("Match parse error: %s at line %d",  \
		            _s, aceInStreamLine(parse_io)) ;     \
      goto abort ;                                               \
    }

  if (!initialise())
    ABORT("MatchTable initialisation failed") ;

  ma = arrayReCreate (ma, 1024, MATCH) ;
  ma2 = arrayReCreate (ma2, 128, MATCH) ;
  mHash = assReCreate (mHash) ;

  if (!matchKey2Parent (tableKey, &parent))
    ABORT("bad MatchTable object name") ;
  
  nt = 0 ;

  /* now read the entry */
  while (aceInCard (parse_io) && (s = aceInWord(parse_io)))
    {
      m = arrayp (ma, arrayMax(ma), MATCH) ;
      m->flags = 0 ;
      if (strcmp (s, "-D") == 0)       /* possible -D */
	{ m->flags |= FLAG_DELETE ;
	  s = aceInWord(parse_io) ;
	}
						/* target class */
      if (!s || !(classe = pickWord2Class(s)))
	ABORT ("bad class") ;

      if (!(s = aceInWord(parse_io)))          /* target name */
	ABORT("no object name") ;
      lexaddkey (s, &key, classe) ;
      if (!KEYKEY(key))
	ABORT("bad object name") ;
      m->key = key ;

      if (!(s = aceInWord(parse_io)))		/* method */
	ABORT("no method name") ;
      lexaddkey (s, &key, _VMethod) ;
      if (!KEYKEY(key))
	ABORT("bad method name") ;
      m->meth = key ;

      if (!aceInFloat (parse_io, &fx))		/* score */
	ABORT("no score") ;
      m->score = fx ;
      
      if (!aceInInt (parse_io, &ix))		/* q1 */
	ABORT("no q1") ;
      m->q1 = ix ;

      if (!aceInInt (parse_io, &ix))		/* q2 */
	ABORT("no q2") ;
      m->q2 = ix ;

      if (!aceInInt (parse_io, &ix))		/* t1 */
	ABORT("no t1") ;
      m->t1 = ix ;

      if (!aceInInt (parse_io, &ix))		/* t2 */
	ABORT("no t2") ;
      m->t2 = ix ;

				/* must merge flags for same data */
      { int x = (KEYKEY(m->key) << 8) + 163*KEYKEY(m->meth) +
	  17*m->q1 + 31*m->q2 + 19*m->t1 + 37*m->t2 ;
	MATCH *m2 ;
	void *v ;

	if (!assInsert (mHash, assVoid(x), assVoid(arrayMax(ma))))
	  { if (!assFind (mHash, assVoid(x), &v))
	      messcrash ("ass problems in matchTableParse") ;
	    m2 = arrp (ma, assInt(v)-1, MATCH) ;
	    if (m2->key != m->key ||
		m2->meth != m->meth ||
		m2->score != m->score ||
		m2->q1 != m->q1 ||
		m2->q2 != m->q2 ||
		m2->t1 != m->t1 ||
		m2->t2 != m->t2) /* hash duplicate - must search linearly */
	      { int j ;
		m2 = arrp(ma, 0, MATCH) ;
		for (j = 0 ; j < arrayMax(ma) ; ++j, ++m2)
		  if (m2->key == m->key &&
		      m2->meth == m->meth &&
		      m2->score == m->score &&
		      m2->q1 == m->q1 &&
		      m2->q2 == m->q2 &&
		      m2->t1 == m->t1 &&
		      m2->t2 == m->t2)
		    break ;
		if (j >= arrayMax(ma))
		  m2 = 0 ;
	      }
	    if (m2)		/* i.e. if match is found */
	      { if (m->flags & FLAG_DELETE)
		  m2->flags |= FLAG_DELETE ;
		else if (m2->flags & FLAG_DELETE)
		  m2->flags = 0 ; /* clear any flags set */
		m = m2 ;
		--arrayMax(ma) ;
	      }
	  }
      }
				/* now process flags */
      if ((s = aceInWord(parse_io)))
	{ FLAGSET x ;
	  if (strcmp (s, "-D") != 0)
	    { 
	      x = flag ("Match", s) ;
	      m->flags |= x ;
	      m->flags &= ~(x << 16) ;
	    }
	  else if ((s = aceInWord(parse_io)))
	    { 
	      x = flag ("Match", s) ;
	      m->flags &= ~x ;
	      m->flags |= (x << 16) ;
	    }
	}

      m->stamp = sessionKey ;

      ++nt ;
    }

  if (arrayMax (ma))
    {
      arraySort (ma, rowCompare) ;
      array (ma, arrayMax(ma), MATCH) = endMarker ;

				/* first save with parent */
      if (saveMatch (ma, tableKey, parent))
				/* now cross-ref where necessary */
	for (m = arrp(ma, 0, MATCH) ; m->key != endKey ; ++m)
	  if (m->flags & FLAG_USED)
	    { arrayMax(ma2) = 0 ;
	      key = m->key ;
	      do
		{ if (m->flags & FLAG_USED)
		    reverseRowAdd (ma2, m, parent) ;
		  ++m ;
		} while (m->key == key) ;
	      --m ;
	      arraySort (ma2, rowCompare) ;
	      array (ma2, arrayMax(ma2), MATCH) = endMarker ;
	      lexaddkey (messprintf ("%s-%s", className(key), name(key)),
			 &tableKey, _VMatchTable) ;
	      saveMatch (ma2, tableKey, key) ;
	    }
      
      result = PARSEFUNC_OK ;
    }
  else
    result = PARSEFUNC_EMPTY ;

  return result ;

 abort:
  return PARSEFUNC_ERR ;
} /* matchTableParse */

/*****************************************************************/

/* Routine that ties a match table into a BS object and stores it
   If one exists already they are merged (linear time since sorted)
   Rows that are used are flagged with FLAG_USED, for xref
*/

static int saveMatch (Array ma, KEY tabKey, KEY query)
{
  Array maOld ;
  OBJ obj = 0 ;
  int nUsed = 0 ;
  static Array maNew = 0 ;

  maNew = arrayReCreate (maNew, 1024, MATCH) ;

  if ((maOld = arrayGet (tabKey, MATCH, matchFormat))) /* MERGE */
    {
      MATCH *m1, *m2 ;
      int c ;

      array(maOld, arrayMax(maOld), MATCH) = endMarker ; 
      m1 = arrp(maOld, 0, MATCH) ;
      m2 = arrp(ma, 0, MATCH) ;
      while (m1->key != endKey || m2->key != endKey)
	{ c = rowCompare (m1, m2) ;
	  if (c > 0)
	    { if (!(m2->flags & FLAG_DELETE))
		{ rowAdd (maNew, m2) ;
		  m2->flags |= FLAG_USED ;
		  ++nUsed ;
		}
	      ++m2 ;
	    }
	  else if (c < 0)
	    { rowAdd (maNew, m1) ;
	      ++m1 ;
	    }
	  else
	    { if (m2->flags & FLAG_DELETE)
		{ m2->flags |= FLAG_USED ;
		  ++nUsed ;
		}
	      else
		{ if (m2->flags & ~m1->flags) /* ie flags to add */
		    { m1->flags |= (m2->flags & saveMask) ;
		      m2->flags |= FLAG_USED ;
		      ++nUsed ;
		    }
		  if (m1->flags & (m2->flags >> 16)) /* flags to delete */
		    { m1->flags &= ~(m2->flags >> 16) ;
		      m2->flags |= FLAG_USED ;
		      ++nUsed ;
		    }
		  rowAdd (maNew, m1) ;
		}
	      ++m1 ; ++m2 ;
	    }
	}

      if (arrayMax(maNew))
	{ if (nUsed)
	    arrayStore (tabKey, maNew, matchFormat) ;
	}
      else 
	{ isInternalArrayKill = TRUE ;
	  arrayKill (tabKey) ;
	  isInternalArrayKill = FALSE ;
	  if ((obj = bsUpdate (query)))	/* remove reference */
	    { if (bsFindTag (obj, _Match_table))
		bsPrune (obj) ;
	      bsSave (obj) ;
	    }
	}

      arrayDestroy (maOld) ;
    }
  else						/* JUST COPY */
    { MATCH *m ;
      for (m = arrp(ma, 0, MATCH) ; m->key != endKey ; ++m)
	if (!(m->flags & FLAG_DELETE))
	  { rowAdd (maNew, m) ;
	    m->flags |= FLAG_USED ;
	    ++nUsed ;
	  }

      if (arrayMax(maNew))
	{ arrayStore (tabKey, maNew, matchFormat) ;
	  if ((obj = bsUpdate (query)))	/* set up reference */
	    { bsAddKey (obj, _Match_table, tabKey) ;
	      bsSave (obj) ;
	    }
	}
    }

  return nUsed ;
}

/**********************************************************************/
/**********************************************************************/

BOOL matchTableDump (ACEOUT dump_out, KEY k)
     /* DumpFuncType for class _VMatchTable */
{ 
  Array ma ;
  int i ;
  MATCH *m ;
  DICT *dict = flagSetDict ("Match") ;

  if (!initialise()) return FALSE ;

  if (!(ma = arrayGet (k, MATCH, matchFormat)))
    return FALSE ;

  for (i = 0 ; i < arrayMax(ma) ; ++i)
    { m = arrp(ma,i,MATCH) ;
      aceOutPrint (dump_out, "%s %s  ", 
		   className(m->key), 
		   freeprotect(name(m->key))) ;
/* must separate to avoid freeprotect() reuse problems */
      aceOutPrint (dump_out, "%s %g  %d %d  %d %d",
		   freeprotect(name(m->meth)), m->score, 
		   m->q1, m->q2, m->t1, m->t2) ;
      aceOutPrint (dump_out, " %s\n",
		    flagNameFromDict (dict, m->flags)) ;
    }

  /*  if (level)
      freeOutClose (level) ;*/
  arrayDestroy (ma) ;

  return TRUE ;
} /* matchTableDump */

/**********************************************************************/
/**********************************************************************/

BOOL matchTableKill (KEY key)
{ 
  MATCH *m ;
  Array ma, ma2 ;
  OBJ obj ;
  KEY parent, tableKey ;

  if (isInternalArrayKill)
    return FALSE ;

  if (!initialise()) return FALSE ;

  if (class(key) != _VMatchTable)
    messcrash ("wrong class %s in matchTableKill", className(key)) ;
  if (!matchKey2Parent (key, &parent))
    { messerror ("Can't kill badly named MatchTable key %s", name(key)) ;
      return FALSE ;
    }

  if (!(ma = arrayGet (key, MATCH, matchFormat)))
    return FALSE ;

				/* kill key and entry in parent */
  isInternalArrayKill = TRUE ;
  arrayKill (key) ;
  isInternalArrayKill = FALSE ;

  if ((obj = bsUpdate (parent)))
    { if (bsFindTag (obj, _Match_table))
	bsPrune (obj) ;
      bsSave (obj) ;
    }
				/* now do XREFs as when parsing */
				/* first flag all ma as DELETE */
  array (ma, arrayMax(ma), MATCH) = endMarker ;
  for (m = arrp(ma, 0, MATCH) ; m->key != endKey ; ++m)
    m->flags |= FLAG_DELETE ;

  ma2 = arrayCreate (64, MATCH) ;

  for (m = arrp(ma, 0, MATCH) ; m->key != endKey ; ++m)
    { arrayMax(ma2) = 0 ;
      key = m->key ;
      do
	{ reverseRowAdd (ma2, m, parent) ;
	  ++m ;
	} while (m->key == key) ;
      --m ;
      arraySort (ma2, rowCompare) ;
      array (ma2, arrayMax(ma2), MATCH) = endMarker ;
      lexaddkey (messprintf ("%s-%s", className(key), name(key)),
		 &tableKey, _VMatchTable) ;
      saveMatch (ma2, tableKey, key) ;
    }

  arrayDestroy (ma) ;
  return TRUE ;
}

/**********************************************************************/
/**********************************************************************/

TABLE *matchTable (KEY key, STORE_HANDLE h)
{ 
  TABLE *t ;
  Array ma ;
  int i ;
  MATCH *m ;

  if (!initialise()) return FALSE ;

  if (!(ma = arrayGet (key, MATCH, matchFormat)))
    {
      OBJ obj ;
      if (!(obj = bsCreate (key)))
	return 0 ;
      if (!bsGetKey (obj, _Match_table, &key))
	{ bsDestroy (obj) ;
	  return 0 ;
	}
      if (!(ma = arrayGet (key, MATCH, matchFormat)))
	return 0 ;
    }

  t = tableHandleCreate (64, matchFormat, h) ;

  tableSetColumnName (t, 0, "target") ;
  tableSetColumnName (t, 1, "method") ;
  tableSetColumnName (t, 2, "score") ;
  tableSetColumnName (t, 3, "q1") ;
  tableSetColumnName (t, 4, "q2") ;
  tableSetColumnName (t, 5, "t1") ;
  tableSetColumnName (t, 6, "t2") ;
  tableSetColumnName (t, 7, "flagsMatch") ;
  tableSetColumnName (t, 8, "stamp") ;

  m = arrp (ma, 0, MATCH) ;
  for (i = 0 ; i < arrayMax(ma) ; ++i, ++m)
    { tabKey(t,i,0) = m->key ;
      tabKey(t,i,1) = m->meth ;
      tabFloat(t,i,2) = m->score ;
      tabInt(t,i,3) = m->q1 ;
      tabInt(t,i,4) = m->q2 ;
      tabInt(t,i,5) = m->t1 ;
      tabInt(t,i,6) = m->t2 ;
      tabFlag(t,i,7) = m->flags ;
      tabKey(t,i,8) = m->stamp ;
    }

  return t ;
}

/************************ end of file ***********************/
 
 
