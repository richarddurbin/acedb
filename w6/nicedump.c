/*  File: nicedump.c
 *  Author: Danielle et Jean Thierry-Mieg (mieg@mrc-lmb.cam.ac.uk)
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
 * 	Richard Durbin (MRC LMB, UK) rd@mrc-lmb.cam.ac.uk, and
 *	Jean Thierry-Mieg (CRBM du CNRS, France) mieg@kaa.cnrs-mop.fr
 *
 * Description:
 * Exported functions:
 * HISTORY:
 * Last edited: Mar  8 14:32 2001 (edgrif)
 * * Lincoln Stein added java dump
 * * Doug Bigwood and John Barnett changes to handle html format
 * * Jul 16 00:11 1998 (rd): lots of cleaning up
 *	new timestamps for Java (never used in human or perl)
 *	down looping not recursion
 *	perl layout simplified (no freeOutxy) and IL \n "fix" undone
 *		(it's bug was because of bad interaction with freeOutxy)
 *	removed dumpQueries static because it was always set TRUE -
 *		Java always dumps ATTACHes to recursion level one,
 *		human and perl never do
 *	isModel set correctly for NEW_MODELS
 * * Dec  8 13:53 1994 (mieg): 
 * Created: Thu Jan 28 13:50:53 1993 (mieg)
 * CVS info:   $Id: nicedump.c,v 1.46 2003-05-15 22:18:34 sienkiew Exp $
 *-------------------------------------------------------------------
 */

#include <wh/acedb.h>
#include <wh/aceio.h>
#include <wh/bs_.h> 
#include <wh/bs.h>
#include <wh/lex.h>
#include <wh/pick.h>
#include <whooks/systags.h>
#include <whooks/sysclass.h>
#include <wh/dump.h>
#include <wh/query.h>
#include <wh/java.h>
#include <wh/model.h>					    /* for isModel() */

/*************/

/* Defines the output style of xml.                                          */
typedef struct _XmlProcessing
{
  BOOL content ;
  BOOL class_att ;
  BOOL value_att ;
  BOOL all_atts ;
} XmlProcessing ;



/*************/

static int niceBShuman (ACEOUT dump_out, BS bs, int x, int y) ;
static char *niceTextUsual(BS bs) ;
static char *niceExpandText(KEY key) ;

static void niceBSPerl (ACEOUT dump_out, BS bs, BS bsm, int position, int level) ;
static char *niceTextPerl (BS bs) ;

static void niceBSXML (ACEOUT dump_out, BS bs, BS bsm,
		       XmlProcessing *style, int position, int level,
		       char *enclosing_class, int enclosing_level) ;
static char* niceTextXML(BS bs, XmlProcessing *style, char **end_tag,
			 char **enclosing_class, int *enclosing_level) ;
static char *niceTextXMLSchema(BS *bs, BS *bsm, XmlProcessing *style, char **end_tag_out) ;

static int niceBSJava (ACEOUT dump_out, BS bs, BS bsm, int position, int level,
		       int style, BOOL isK) ;
static char* niceTextJava (BS bs, int style, BOOL isK) ;


/*************/

/* YUCH, there is really no need for these to be here like this....          */
extern BOOL dumpComments ;
extern BOOL dumpTimeStamps ;

/*************/

#define LINE_LENGTH 120
static BOOL isModele ;
static char *qm = 0 ; /* character or sequence of chars to use as a quote mark */
static BOOL debug = FALSE ;


/*
* AceC object dumper
*/
static int niceBSC (ACEOUT dump_out, BS bs, BS bsm, int position)
{
  int i, level ;
  char direction ;
  char *cp, buf[8] ;

  level = position + 1;
  while (bs)
    {
      bsModelMatch (bs, &bsm) ;
	
      i = level - position ;
      switch (i)
	{
	case 0: direction = '.' ; break ;
	case 1: direction = '>' ; break ;
	case -1: direction = '<' ; break ;
	default:
	  buf[0] = 'l' ;
	  while (i < -255)
	    { buf[1] = 255 ; aceOutBinary (dump_out, buf,2) ; i += 255 ; }
	  if (i < 0)
	    { buf[1] = -i ; aceOutBinary (dump_out, buf,2) ; } 
	  direction = '.' ;
	  break ;
	}
      buf[0] = direction ; 
      position = level ;

      /* dump cell itself */
      if (bs->key <= _LastC)
	{
	  buf[1] = 't' ; 
	  aceOutBinary ( dump_out, buf,2) ; 
	  cp = bsText(bs) ;
	  aceOutBinary ( dump_out, cp, strlen(cp) + 1) ;
	}
      else if (bs->key == _Int)
	{
	  int ii = bs->n.i ;
	  
	  buf[1] = 'i' ; 
	  aceOutBinary ( dump_out, buf,2) ; 
	  aceOutBinary ( dump_out, (char*)&ii, 4) ;
	}
      else if (bs->key == _Float)
	{ 
	  float f = bs->n.f ;
	  
	  buf[1] = 'f' ; 
	  aceOutBinary ( dump_out, buf,2) ; 
	  aceOutBinary ( dump_out, (char*) &f, 4) ;
	}
      else if (bs->key == _DateType)
	{ 
	  int ii = bs->n.time ;
	  
	  buf[1] = 'd' ; 
	  aceOutBinary ( dump_out, buf,2) ; 
	  aceOutBinary ( dump_out, (char*) &ii, 4) ;
	}
      else if (iskey(bs->key) == 2)
	{
	  char *title = 0 ;

	  if ((title = niceExpandText(bs->key)))
	    {
	      buf[1] = 't' ;
	      aceOutBinary ( dump_out, buf,2) ; 
	      cp = title ;
	      aceOutBinary ( dump_out, cp, strlen(cp) + 1) ;
	    }
	  else 
	    {
	      buf[1] = 'K' ; buf[2] = class(bs->key) ;
	      aceOutBinary ( dump_out, buf,3) ; 
	      cp = name(bs->key) ;
	      aceOutBinary ( dump_out, cp, strlen(cp) + 1) ;
	    }
	}
      else if (class(bs->key)) /* an emtpty object */
	{ 
	  buf[1] = 'k' ; buf[2] = class(bs->key) ;
	  aceOutBinary (dump_out, buf,3) ; 
	  cp = name(bs->key) ;
	  aceOutBinary (dump_out, cp, strlen(cp) + 1) ;
	}
      else if (!class(bs->key)) /* a tag */
	{ 
	  int ii = bs->key ; 
	  
	  buf[1] = 'g' ; 
	  aceOutBinary ( dump_out, buf,2) ; 
	  aceOutBinary ( dump_out, (char*) &ii, 4) ;
	}
      
      /* dump right */
      if (bs->right)
	{
	if (bsIsType(bsm->right))
	  aceOutBinary( dump_out, "!", 1);	/* entering subtype */
	position = niceBSC (dump_out, bs->right, bsModelRight(bsm), position) ;
	if (bsIsType(bsm->right))
	  aceOutBinary( dump_out, "@", 1);	/* leaving subtype */
	}
      /* move down, dump in the cuurent while loop */
      bs = bs->down ;
      if (!bs)
	break ;			/* exit from loop */
      
      aceOutBinary( dump_out, "\n", 1);
  }
  return position ;
}


/*****************************************************************************/
/************************* External routines *********************************/

void niceDump(ACEOUT dump_out, OBJ obj, char style)
{
  static BOOL isFirst = TRUE ;
  BS oldDown = 0 ;


  if (isFirst)
    {
      qm = getenv("ACEQM"); /* quote mark for perl output */
      if (!qm) 
	{
	  qm = messalloc (2);
	  qm[0] = 1; qm[1] = 0; /* default character is ^A */
	}
      isFirst = FALSE ;
    }

  isModele = (class(obj->root->key) == _VModel) || isModel(obj->root->key) ;

  if (obj->curr)
    switch (style)
      {
      case 'h': case 'H':
	aceOutPrint (dump_out, "%s %s", className (obj->key), niceTextUsual (obj->root)) ;

	if (obj->curr != obj->root)
	  {
	    oldDown = obj->curr->down ; obj->curr->down = 0 ;
	    niceBShuman (dump_out, obj->curr,2,1) ;
	    obj->curr->down = oldDown ;
	  }
	else
	  niceBShuman (dump_out, obj->curr->right,2,1) ;

	aceOutxy (dump_out, "",0,1) ; /* space line */
	break ;

      case 'j': case 'J': 
      case 'k' : case 'K':	/* revised 'j' types for AcePerl with # and good times */
	if (obj->curr != obj->root)
	  {
	    aceOutPrint (dump_out, "%s",
			 niceTextJava (obj->root, 
				       (style == 'J' || style == 'K') ? 1 : 0, 
				       (style == 'k' || style == 'K'))) ;
	    oldDown = obj->curr->down ; obj->curr->down = 0 ;
	    niceBSJava (dump_out, obj->curr, obj->modCurr, 0, 1, 
			(style == 'J' || style == 'K') ? 2 : 0, 
			(style == 'k' || style == 'K')) ;
	    obj->curr->down = oldDown ;
	  }
	else
	  {
	    oldDown = obj->curr->down ; obj->curr->down = 0 ;
	    niceBSJava (dump_out, obj->curr, obj->modCurr, 0, 0,
			(style == 'J' || style == 'K') ? 1 : 0, 
			(style == 'k' || style == 'K')) ;
	    obj->curr->down = oldDown ;
	  }
	aceOutPrint (dump_out, "\n") ;
	break;

      case 'p':
	aceOutPrint (dump_out, "{");
	aceOutPrint (dump_out, "%s", niceTextPerl (obj->root)) ;
	if (obj->curr->right)
	  {
	    aceOutPrint (dump_out, ",Pn=>[\n");
	    niceBSPerl (dump_out, obj->curr->right,
			bsModelRight(obj->modCurr), 0, 1) ;
	    aceOutPrint (dump_out, "]");
	  }
	aceOutPrint (dump_out, "}\n");
	break ;

      case 'C':
        {
          char buf[2] ;
          char *cp = name(obj->key) ;

          buf[0] = 'N' ; buf[1] = class(obj->key) ;
          aceOutBinary ( dump_out, buf, 2) ;
          aceOutBinary ( dump_out, cp, strlen (cp) + 1) ;
          if (obj->curr->right)
            niceBSC ( dump_out, obj->curr->right, bsModelRight(obj->modCurr), 0) ;
          buf[0] = '#' ;
          aceOutBinary ( dump_out, buf,1) ;
        }
        break ;


     case 'x': case 'X': case 'y': case 'Y':
       {
	 XmlProcessing xml_style ;
	 char *start_tag ;
	 char *end_tag ;
	 char *enclosing_tag = className(obj->root->key) ;
	 int enclosing_level = 0 ;

	 /* Map the style on to the possible elements we can write out.      */
	 /* With current meanings of flags this if/else is slightly more     */
	 /* compact than a switch, this may change.                          */
	 if (style == 'x')
	   xml_style.class_att = FALSE ;
	 else
	   xml_style.class_att = TRUE ;
	 if (style == 'x' || style == 'X')
	   xml_style.value_att = FALSE ;
	 else
	   xml_style.value_att = TRUE ;
	 if (style == 'y')
	   xml_style.all_atts = TRUE ;
	 else
	   xml_style.all_atts = FALSE ;
	 if (style == 'Y')
	   xml_style.content = FALSE ;
	 else
	   xml_style.content = TRUE ;

	 if (isModele)
	   start_tag = niceTextXMLSchema(&(obj->root), &(obj->modCurr),
					 &xml_style, &end_tag) ;
	 else
	   start_tag = niceTextXML(obj->root, &xml_style, &end_tag,
				   &enclosing_tag, &enclosing_level) ;
	 aceOutPrint(dump_out, "%s", start_tag) ;

	 if (obj->curr->right)
	   {
	     aceOutPrint (dump_out, "\n") ;
	     niceBSXML(dump_out, obj->curr->right, bsModelRight(obj->modCurr),
		       &xml_style, 0, 1, enclosing_tag, enclosing_level) ;
	   }
	 aceOutPrint (dump_out, "%s", end_tag) ;
	 messfree(end_tag) ;
	 aceOutPrint (dump_out, "\n");
	 break ;
       }

#ifdef JUNK   /* not yet done */
>       case 'b': /* Boulder */
>       -name=AC4
>         -class=Sequence
>         FEATURES=....
>         HOMOL=...
> 
> 
>       niceText = niceTextBoulder;
>       aceOutPrint (dump_out, "%s", niceText(obj->root));
>       niceBSBoulder (dump_out, obj->root->key, obj->curr->right, obj->modCurr->right, 0,1);
>       aceOutPrint (dump_out, "\n");
>       break;
> 
#endif

      default:
	messcrash ("Logic error: unknown style %c in niceDump", style) ;
      }

  return;
} /* niceDump */



/*****************************************************************************/
/*********************** Internal routines ***********************************/

/* Output ace data in human readable form, indentation provides a visual key */
/* to the class/tags etc.                                                    */
/* recursive routine */
static int niceBShuman (ACEOUT dump_out, BS bs, int x, int y)
{
  int length, xPlus, yMax ;
  char *text, *cp ;

  for ( ; bs ; bs = bs->down)
    {
      if (!dumpComments && class(bs->key) == _VComment)
	continue ;

      if (!(text = niceExpandText (bs->key)))
	text = niceTextUsual (bs) ;
   
      if (x < LINE_LENGTH - 18)
	length = LINE_LENGTH - x ;
      else
	length = 18 ;

      xPlus = 0 ;
      yMax = y ;
  
      uLinesText (text, length) ;
      if ((cp = uNextLine(text)))
	{ 
	  if (strlen(cp) > xPlus)
	    xPlus = strlen(cp) ;
	  if ( x + xPlus > LINE_LENGTH )
	    {
	      yMax++ ; x = LINE_LENGTH - length ;
	    }

	  aceOutxy (dump_out, cp,x,yMax++) ;	/* write out this node */
	}
      while ((cp = uNextLine(text))) /* indent following lines */
	{ 
	  aceOutxy (dump_out, cp,x+2,yMax++) ;
	  if (strlen(cp)+2 > xPlus)
	    xPlus = strlen(cp)+2 ;
	}
  
      xPlus += x ;
      xPlus = xPlus + 6 - xPlus%4 ;
  
      if (bs->right)		/* to the right at same y */
	y = niceBShuman (dump_out, bs->right, xPlus, y) ;
  
      if (yMax > y)
	y = yMax ;
    }
  
  return y ;
} /* niceBShuman */

static char* niceTextUsual(BS bs)
{
  if (isModele)
    return name (bs->key) ;

  if (bs->key <= _LastC)
    return bsText(bs) ;
  else if (bs->key == _Int)
    return messprintf ("%d", bs->n.i) ;
  else if (bs->key == _Float)
    return messprintf ("%g", bs->n.f) ;
  else if (bs->key == _DateType)
    return timeShow (bs->n.time) ;
  else
    return name (bs->key) ;
} /* niceTextUsual */

/*************/

static char *niceExpandText (KEY key)
{
  KEY tag = pickExpandTag(key) ;
  OBJ obj ;
  char *text = 0 ;

  if (tag && (obj = bsCreate(key)))
    { if (bsGetKeyTags (obj,tag,0))
	text = niceTextUsual (obj->curr) ;
      bsDestroy (obj) ;
    }

  return text ;
} /* niceExpandText */


/************************************************************/
/*                  ***  Java DUMPING  ***                                   */

/* recursive routine */
static int niceBSJava (ACEOUT dump_out, BS bs, BS bsm, int position, int level, int style, BOOL isK) 
{
  static int attachRecursionLevel = 0 ;
  int i;

  while (bs)
    {
      bsModelMatch (bs, &bsm) ;

      /* tab over to the correct level */
      for (i=position;i<level;i++)
	aceOutPrint (dump_out, "\t");
      aceOutPrint (dump_out, "%s", niceTextJava(bs, style, isK));

      if (bs->right)
	level=niceBSJava (dump_out, bs->right, bsModelRight(bsm), level,level+1, style, isK);

      else if (!attachRecursionLevel &&
	       bsm && bsm->right && class(bsm->right->key) == _VQuery)
	{ 
	  char *cq, *question = name(bsm->right->key) ;
	  OBJ obj2 = 0 ; BS bs1 = bs ;
	  KEYSET ks1 ; KEY key1 ;
	  int ii ;
	  BOOL wholeObj = !strcmp("WHOLE",question) ;
	  
	  cq = question + strlen(question) - 1 ;
	  while (cq > question && *cq != ';')
	    cq-- ;
	  if (*cq == ';')
	    cq++ ;
	  wholeObj = !strcmp("WHOLE",cq) ;
	  
	  /* position k1 upward on first true non-tag key */
	  while (bs1->up && 
		 ( pickType(bs1->key) != 'B' ||
		  !class(bs1->key) || bsIsComment(bs1)))
	    bs1 = bs1->up ;
	  ks1 = queryKey (bs1->key, !wholeObj ? question : "IS *") ;
	  if (wholeObj || queryFind (0, 0, cq)) 
	    for (ii = 0 ; ii < keySetMax (ks1) ; ii++)
	      { key1 = keySet (ks1, ii) ;
		obj2 = bsCreate(key1) ;
		if (!obj2)
		  continue ;
		attachRecursionLevel++ ;
		if (wholeObj || queryFind (obj2, key1, 0)) 
		  level=niceBSJava (dump_out, obj2->curr->right, 
				    bsModelRight(obj2->modCurr), 
				    level, level + 1, style, isK) ;
		bsDestroy (obj2) ;
		attachRecursionLevel-- ;
	      }
	  keySetDestroy (ks1) ;
	}
    
      bs = bs->down ;
      if (!bs)
	break ;			/* exit from loop */

      aceOutPrint (dump_out, "\n");
      position = 0;
  }

  level--;
  return level;
} /* niceBSJava */

/**************/

static char* niceTextJava (BS bs, int style, BOOL isK)
{ 
  static KEY old =  0, oldTime = 0 ;
  static Stack s = 0 ;

  if (style < 2) old = oldTime = 0 ;
  if (!bs) return 0 ; /* allow to simply reset old to zero */

  if (!s) s = stackCreate(128) ;
  stackClear (s) ;

  if (style == 0)  /* old style, always start with a ? */
    pushText (s, "?") ;

  if (isModele)
    {
      catText (s, class(bs->key) == _VModel ? "Model" : "tag") ;
      catText (s, messprintf ("?%s?", freejavaprotect(name(bs->key)))) ;
    }
  else if (bs->key <= _LastC)
    {
      if (!old || bs->key != old)
	catText (s, isK ? "#Text" : "txt") ;
      catText (s, messprintf ("?%s?", freejavaprotect(bsText(bs)))) ;
    }
  else if (bs->key == _Int)
    {
      if (!old || bs->key != old)
	catText (s, isK ? "#Int" : "int") ;
      catText (s, messprintf ("?%d?", bs->n.i)) ;
    }
  else if (bs->key == _Float)
    {
      if (!old || bs->key != old)
	catText (s, isK ? "#Float" : "float") ;
      catText (s, messprintf ("?%g?", bs->n.f)) ;
    }
  else if (bs->key == _DateType)
    {
      if (!old || bs->key != old)
	catText (s, isK ? "#Date" : "date") ;
      catText (s, messprintf ("?%s?", isK ? timeShow (bs->n.time) 
			      		  : timeShowJava (bs->n.time))) ;
    }
  else if (!class(bs->key)) /* a tag */
    {
      if (!old || class(old))
	catText (s, "tag") ;
      catText (s, messprintf ("?%s?", freejavaprotect(name(bs->key)))) ;
    }
  else
    {
      if (!old || class(bs->key) != class(old))
	catText (s, messprintf ("%s", freejavaprotect(className(bs->key)))) ;
      catText (s, messprintf ("?%s?", freejavaprotect(name(bs->key)))) ;
    }
  if (dumpTimeStamps && bs->timeStamp &&
      (!oldTime || bs->timeStamp != oldTime ))
    catText (s, name(bs->timeStamp)) ;

  old = bs->key ; oldTime = bs->timeStamp ;

  return stackText (s,0) ;
} /* niceTextJava */


/************************************************************/
/*                  ***  XML DUMPING  ***                                    */

/* recursive routine */
static void niceBSXML(ACEOUT dump_out, BS bs, BS bsm,
		      XmlProcessing *style, int position, int level,
		      char *enclosing_class, int enclosing_level)
{
  static int attachRecursionLevel = 0 ;
  int i ;
  char *start_tag ;
  char *end_tag ;
  char *new_enclosing_class = enclosing_class ;
  int new_enclosing_level = enclosing_level ;
  BS bs_down ;


  bs_down = bs ;
  while (bs)
    {
      BOOL model_query ;

      bsModelMatch (bs, &bsm) ;

      /* Indent                                                              */
      for (i = position; i < level; i++) /* indent */
	aceOutPrint (dump_out, "  ");
#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
	aceOutPrint (dump_out, "\t");
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */



      /* I'm really not sure if this is going to work too well because of    */
      /* all the model stuff...is it really needed to print out a model      */
      /* anyway ???  I just don't know....it seems to do better things but   */
      /* are they correct ?????                                              */

      /* print the current tag                                               */
      end_tag = NULL ;
      if (isModele)
	start_tag = niceTextXMLSchema(&bs, &bsm, style, &end_tag) ;
      else
	start_tag = niceTextXML(bs, style, &end_tag,
				&new_enclosing_class, &new_enclosing_level) ;
      aceOutPrint(dump_out, "%s", start_tag) ;

      /* Check for embedded query in the objects model.                      */
      if (!attachRecursionLevel &&
	  bsm && bsm->right && class(bsm->right->key) == _VQuery)
	model_query = TRUE ;
      else
	model_query = FALSE ;

      /* Anything to right ?  then do it on the next line.                   */
      if (bs->right || model_query)
	{
	  aceOutPrint (dump_out, "\n");
	}

      /* Recurse and get the next item to the right                          */
      if (bs->right)
	{ 
	  niceBSXML(dump_out, bs->right, bsModelRight(bsm), style, 0 /*level */, level+1,
		    new_enclosing_class, new_enclosing_level) ;
	}
      /*
      else if (!attachRecursionLevel &&
	       bsm && bsm->right && class(bsm->right->key) == _VQuery)
      */
      else if (model_query)
	{ 
	  char *cq, *question = name(bsm->right->key) ;
	  OBJ obj2 = 0 ; BS bs1 = bs ;
	  KEYSET ks1 ; KEY key1 ;
	  int ii ;
	  BOOL wholeObj = !strcmp("WHOLE",question) ;
	  
	  cq = question + strlen(question) - 1 ;
	  while (cq > question && *cq != ';')
	    cq-- ;
	  if (*cq == ';')
	    cq++ ;
	  wholeObj = !strcmp("WHOLE",cq) ;
	  
	  /* position k1 upward on first true non-tag key */
	  while (bs1->up && 
		 (pickType(bs1->key) != 'B' || !class(bs1->key) || bsIsComment(bs1)))
	    bs1 = bs1->up ;
	  ks1 = queryKey (bs1->key, !wholeObj ? question : "IS *") ;

	  if (wholeObj || queryFind (0, 0, cq))
	    {
	      for (ii = 0 ; ii < keySetMax (ks1) ; ii++)
		{
		  key1 = keySet (ks1, ii) ;
		  obj2 = bsCreate(key1) ;
		  if (!obj2)
		    continue ;
		  attachRecursionLevel++ ;
		  if (wholeObj || queryFind (obj2, key1, 0)) 
		    {
		      niceBSXML (dump_out, obj2->curr->right, bsModelRight(obj2->modCurr), 
				 style, 0 /*level */, level + 1,
				 new_enclosing_class, new_enclosing_level) ;  
		    }
		  bsDestroy (obj2) ;
		  attachRecursionLevel-- ;
		}
	    }
	  keySetDestroy (ks1) ;
	}

      /* Print the closing tag (indent it if its on different line from      */
      /* opening tag. Always newline after closing tag.                      */
      if (bs->right || model_query)
	{
	  for (i = position; i < level; i++)
	    aceOutPrint (dump_out, "  ");
#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
	    aceOutPrint (dump_out, "\t");
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

	}
      aceOutPrint (dump_out, "%s", end_tag) ;
      aceOutPrint (dump_out, "\n");
      messfree(end_tag) ;

      /* Compare addresses, _not_ contents, free if new address.             */
      if (enclosing_class != new_enclosing_class)
	{
	  messfree(new_enclosing_class) ;
	  new_enclosing_class = enclosing_class ;
	}

      /* reset enclosing level.                                              */
      new_enclosing_level = enclosing_level ;

      /* See if there is another item at same level, otherwise exit from loop*/
      bs = bs->down ;
      if (!bs)
	break ;

    } /* while loop */

  return ;
}


static char *niceTextXML(BS bs, XmlProcessing *style, char **end_tag_out,
			 char **enclosing_class, int *enclosing_level)
{
  char *start_tag = NULL, *end_tag = NULL ;
  char *element ;
  char *value ;
  char *type ;
  char *num_str ;
  char *level_str ;

  /* These must be NULL because of the messprintf() below.                   */
  element = NULL ;
  value = NULL ;
  type = NULL ;

  /* These must be NULL to ensure freeing of resources correctly.            */
  *end_tag_out = NULL ;
  num_str = NULL ;
  level_str = NULL ;

  if (bs->key <= _LastC)
    {
      element = "Txt" ;
      value = bsText(bs) ;
      type = "txt" ;
      end_tag = element ;
      (*enclosing_level)++ ;
      level_str = hprintf(0, "%s-%d", *enclosing_class, *enclosing_level) ;
    }
  else if (bs->key == _Int)
    {
      element = "Int" ;
      num_str = hprintf(0, "%d", bs->n.i) ;
      value = num_str ;
      type = "int" ;
      end_tag = element ;
      (*enclosing_level)++ ;
      level_str = hprintf(0, "%s-%d", *enclosing_class, *enclosing_level) ;
    }
  else if (bs->key == _Float)
    {
      element = "Float" ;
      num_str = hprintf(0, "%g", bs->n.f) ;
      value = num_str ;
      type = "float" ;
      end_tag = element ;
      (*enclosing_level)++ ;
      level_str = hprintf(0, "%s-%d", *enclosing_class, *enclosing_level) ;
    }
  else if (bs->key == _DateType)
    {
      element = "Date" ;
      value = timeShow(bs->n.time) ;
      type = "date" ;
      end_tag = element ;
      (*enclosing_level)++ ;
      level_str = hprintf(0, "%s-%d", *enclosing_class, *enclosing_level) ;
    }
  else if (class(bs->key))
    {
      /* we've found an object.                                              */
      element = className(bs->key) ;
      value = name(bs->key) ;
      type = "object_ref" ;
      end_tag = element ;
      *enclosing_class = hprintf(0, "%s", element) ;
    }
  else if (!class(bs->key))
    {
      /* we've found a tag.                                                  */
      element = name(bs->key) ;
      type = "tag" ;
      end_tag = element ;
      *enclosing_class = hprintf(0, "%s", element) ;
    }
  else
    {
      messcrash("logic error, unexpected type of tag/class, class was: %s.",
		className(bs->key)) ;
    }


  /* Make:   "<start_tag>content"  and print it.                             */
  start_tag = messprintf ("<%s%s%s%s%s%s%s%s%s%s>%s",	    /* aaaagggghhhhh, the percentage game... */
			  element,
			  style->class_att ? " type=\"" : "",
			  style->class_att ? type : "",
			  style->class_att ? "\"" : "",
			  (style->all_atts && level_str) ? " level=\"" : "",
			  (style->all_atts && level_str) ? level_str : "",
			  (style->all_atts && level_str) ? "\"" : "",
			  (style->value_att && value) ? " value=\"" : "",
			  (style->value_att && value) ? value : "",
			  (style->value_att && value) ? "\"" : "",
			  (style->content && value) ? value : "") ;

  /* Make:   "<end_tag>"  and return it (note, caller must free this).       */
  *end_tag_out = hprintf(0, "</%s>", end_tag) ;

  if (num_str != NULL)
    messfree(num_str) ;

  if (level_str != NULL)
    messfree(level_str) ;

  return start_tag ;
}

/***************************/

/* Like the above routine but deals with special requirements of a model.    */
static char *niceTextXMLSchema(BS *bs_out, BS *bsm_out,
			       XmlProcessing *style, char **end_tag_out)
{
  char *start_tag = NULL ;
  char *elem_name ;
  char *elem_type ;
  char *elem_occurs ;
  char *elem_basetype ;
  char *elem_xref = NULL ;
  char *debug ;
  BS bs = *bs_out ;
  BS bsm = *bsm_out ;
  BOOL unique = FALSE ;


  /* These must _not_ be NULL because of the messprintf() below.             */
  elem_basetype = "" ;

  /* These must be NULL to ensure freeing of resources correctly.            */
  *end_tag_out = NULL ;


  /* if you do a class(key) you get either model or system, that's it.   */
  debug = nameWithClass(bs->key) ;

  if (bs->key == _UNIQUE)
    {
      unique = TRUE ;

      bs = bs->right ;
      bsModelRight(bsm) ;
      bsModelMatch(bs, &bsm) ;

      *bs_out = bs ;
      *bsm_out = bsm ;
    }


  /* we could just use the keys rather than all this name lark which just    */
  /* wastes time....                                                         */
  elem_name = name(bs->key) ;


  if (strcasecmp(elem_name, "int") == 0)
    {
      elem_type = "INT" ;
      elem_basetype = "<element type=\"acedb:Int\">" ;
    }
  else if (strcasecmp(elem_name, "float") == 0)
    {
      elem_type = "FLOAT" ;
      elem_basetype = "<element type=\"acedb:Float\">" ;
    }
  else if (strcasecmp(elem_name, "datetype") == 0)
    {
      elem_type = "DATE" ;
      elem_basetype = "<element type=\"acedb:DateType\">" ;
    }
  else if (strcasecmp(elem_name, "text") == 0)
    {
      elem_type = "TEXT" ;
      elem_basetype = "<element type=\"acedb:Text\">" ;
    }
  else if (class(bs->key) == _VModel)
    {
      elem_type = "Model" ;
      if (bs->right)
	{
	  if ((bs->right)->key == _XREF)
	    {
	      char *my_name = name((bs->right)->key) ;

	      /* go right to xref...                                         */
	      bs = bs->right ;
	      bsModelRight(bsm) ;
	      bsModelMatch(bs, &bsm) ;

	      /* go right to tag following xref...                           */
	      bs = bs->right ;
	      bsModelRight(bsm) ;
	      bsModelMatch(bs, &bsm) ;

	      elem_xref = name(bs->key) ;

	      *bs_out = bs ;
	      *bsm_out = bsm ;
	    }
	}
      elem_basetype = "<element type=\"acedb:Valid_Name\">" ;
    }
  /* WARNING: NOT SURE IF THIS CATCH ALL IS SAFE...?????                 */
  else
    {
      elem_type = "Tag" ;
    }

  if (unique)
    elem_occurs = " maxOccurs=\"1\"" ;
  else
    elem_occurs = " maxOccurs=\"unbounded\"" ;

  /* Make:   "<start_tag>content"  and return it.                            */
  start_tag = messprintf("<element name=\"%s\" type=\"%s\"%s%s%s%s><complexType>%s",
			 elem_name,
			 elem_type,
			 elem_occurs,
			 elem_xref ? " xref=\"" : "",
			 elem_xref ? elem_xref : "",
			 elem_xref ? "\"" : "",
			 elem_basetype) ;

  /* Make:   "<end_tag>"  and return it (note, caller must free this).       */
  *end_tag_out = hprintf(0, "%s%s",
			 *elem_basetype ? "</element>" : "",
			 "</complexType></element>") ;

  return start_tag ;
}


/************************************************************/
/*                  ***  Perl DUMPING  ***                                   */

static char* niceTextPerl(BS bs)
{
  char *title;

  if (isModele)
    return name (bs->key);

  if (bs->key <= _LastC)
    return messprintf ("ty=>tx,va=>%s%s%s", qm, bsText(bs), qm);
  else if (bs->key == _Int)
    return messprintf ("ty=>in,va=>%d", bs->n.i);
  else if (bs->key == _Float)
    return messprintf ("ty=>fl,va=>%g", bs->n.f);
  else if (bs->key == _DateType)
    return messprintf ("ty=>dt,va=>%s%s%s", qm, timeShow (bs->n.time), qm);
  else if (iskey(bs->key) == 2)
    {
      if ((title = niceExpandText(bs->key)))
	return 
	  messprintf ("ty=>ob,cl=>%s%s%s,va=>%s%s%s,ti=>%s%s%s", 
		      qm, className(bs->key), qm, qm, name(bs->key), qm, qm, title, qm);
      else 
	return 
	  messprintf ("ty=>ob,cl=>%s%s%s,va=>%s%s%s", 
		      qm, className(bs->key), qm, qm, name(bs->key), qm);
    }
  else if (!class(bs->key)) /* a tag */
    return messprintf ("ty=>tg,va=>%s%s%s", qm, name(bs->key), qm);
  else
    return messprintf ("ty=>ob,cl=>%s%s%s,va=>%s%s%s,mt=>1", 
		       qm, className(bs->key), qm, qm, name(bs->key), qm);
} /* niceTextPerl */

/************************************************************/

/* recursive routine */
/* RD 980716 simpler layout */
static void niceBSPerl (ACEOUT dump_out, BS bs, BS bsm, int position, int level)
{
  static int attachRecursionLevel = 0 ;
  int i ;

  while (bs)
    {
      bsModelMatch (bs, &bsm) ;
      for (i = position; i < level; i++) /* indent */
	aceOutPrint (dump_out, "\t");

      aceOutPrint (dump_out, "{") ;
      aceOutPrint (dump_out, "%s", niceTextPerl(bs)) ;

      if (bs->right) 
	{ 
	  aceOutPrint (dump_out, ",Pn=>[\n") ;
	  niceBSPerl (dump_out, bs->right, bsModelRight(bsm), 0 /*level */ , level+1) ;
	  aceOutPrint (dump_out, "]") ;
	}


      else if (!attachRecursionLevel &&
	       bsm && bsm->right && class(bsm->right->key) == _VQuery)
	{ 
	  char *cq, *question = name(bsm->right->key) ;
	  OBJ obj2 = 0 ; BS bs1 = bs ;
	  KEYSET ks1 ; KEY key1 ;
	  int ii ;
	  BOOL wholeObj = !strcmp("WHOLE",question) ;
	  
	  cq = question + strlen(question) - 1 ;
	  while (cq > question && *cq != ';')
	    cq-- ;
	  if (*cq == ';')
	    cq++ ;
	  wholeObj = !strcmp("WHOLE",cq) ;
	  
	  /* position k1 upward on first true non-tag key */
	  while (bs1->up && 
		 ( pickType(bs1->key) != 'B' ||
		  !class(bs1->key) || bsIsComment(bs1)))
	    bs1 = bs1->up ;
	  ks1 = queryKey (bs1->key, !wholeObj ? question : "IS *") ;
	  if (wholeObj || queryFind (0, 0, cq)) 
	    for (ii = 0 ; ii < keySetMax (ks1) ; ii++)
	      { key1 = keySet (ks1, ii) ;
		obj2 = bsCreate(key1) ;
		if (!obj2)
		  continue ;
		attachRecursionLevel++ ;
		if (wholeObj || queryFind (obj2, key1, 0)) 
		  {
		    aceOutPrint (dump_out, ",Pn=>[\n") ;
		    niceBSPerl (dump_out, obj2->curr->right, 
				bsModelRight(obj2->modCurr), 
				0 /*level */, level + 1) ;  
		    aceOutPrint (dump_out, "]") ;
		  }
		bsDestroy (obj2) ;
		attachRecursionLevel-- ;
	      }
	  keySetDestroy (ks1) ;
	}
      aceOutPrint (dump_out, "}") ;

      bs = bs->down ;
      if (!bs)
	break ;			/* exit from loop */

      aceOutPrint (dump_out, ",\n") ;
    }
}

/**************/


/************************* eof ******************************/
