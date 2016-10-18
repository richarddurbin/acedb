/*  File: picksubs.c
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
 *	Jean Thierry-Mieg (CRBM du CNRS, France) mieg@kaa.cnrs-mop.fr
 *
 * Description:
 * Exported functions:
     pickInit
       must be called once at start of program.
     pickWord2Class()
       returns a class number
     pickMatch matches a name to a template 
       accepts the wild char * (anything) and ? (single character).

 * HISTORY:
 * Last edited: Sep 27 10:42 2010 (edgrif)
 * * Sep 28 14:53 1992 (mieg): a fix because one tag has a - rather than _
 * * Apr  2 13:41 1992 (mieg): Simplified customization by defining
   the new wspec files (sys)options and displays.
 * Created: Thu Apr  2 13:41:10 1992 (mieg)
 *-------------------------------------------------------------------
 */

#include <ctype.h>
#include "acedb.h"
#include "aceio.h"
#include "pick.h"
#include "lex.h"
#include "session.h"
#include "bs.h"
#include "query.h"
#include "whooks/systags.h"
#include "whooks/sysclass.h"
#include "whooks/tags.h"
#include "dbpath.h"
#include "utils.h"


/******************************************************************/

extern BOOL READING_MODELS ;


/******************************************************************/

typedef struct { KEY mainClasse ; unsigned char mask ; }  CLASS_MASK ;


/******************************************************************/

FREEOPT pickVocList[256] = { {0,  "Choose from"}} ;
PICKLIST pickList[256] ;


/******************************************************************/

static Stack pickStack = NULL ;
static Array clA = NULL ;
static Array classMasks = NULL ;
static Array classComposite = NULL ;



/********************************************/
/* It has become a common idiom to have a DB name for a feature but also
 * a common or "public" name which annotators recognise. This function
 * returns the public name given the key which precedes the public name
 * object in obj_key.
 * 
 * The string returned is acedb's, do not free it or write into it !! */
char *pickGetAlternativeName(KEY obj_key, char *alternative_tag_name)
{
  char *alternative_name = NULL ;
  KEY alternative_tag ;
  KEY name_obj_key = KEY_NULL ;

  alternative_tag = str2tag(alternative_tag_name) ;

  if (bIndexGetKey(obj_key, alternative_tag, &name_obj_key))
    alternative_name = name(name_obj_key) ;

  return alternative_name ;
}



/********************************************/

char *className(KEY kk)  
{     
  return stackText(pickStack, pickList[class(kk)].name)  ;
}

/********************************************/

char *pickClass2Word (int classe)
{     
  return className(KEYMAKE(classe,0)) ;
}

/********************************************/

int superClass (KEY cl)
{ 
  pickIsA (&cl, 0) ;

  return cl ;
}

/********************************************/

/* Given the key that equates to "?<model>" (e.g. "?Sequence")
 * as found in the model for a class, return the class, returns 0 on failure.
 * Seems surprising this doesn't already exist, perhaps it does and I just
 * couldn't find it. */
int classModel2Class(KEY class_model_key)
{
  int class = 0 ;
  char *class_name ;

  class_name = name(class_model_key) ;
  if (*class_name == '?')
    {
      class_name++ ;
      class = pickWord2Class(class_name) ;
    }

  return class ;
}


/********************************************/
Array pickIsComposite(KEY classe)
{
  int c;

  if (classe > 256)
    {
      if (class(classe) != _VClass)
	messcrash("Non-class passed to pickIsComposite");
      c = KEYKEY(classe);
    }
  else
    c = classe;

  return array(classComposite, c, Array);
}

BOOL pickBelongsToComposite(KEY classe, KEY key)
{
  Array a = pickIsComposite(classe);
  int i;

  if (classe > 256)
    classe = KEYKEY(classe);
  
  if (!a)
    return classe == class(key) ;
  
  for (i = 0 ; i < arrayMax(a) ; i++)
    {
      if (KEYKEY(array(a, i, KEY)) == class(key))
	return TRUE;
    }

  return FALSE;
}
     
     

/********************************************/

BOOL pickIsA (KEY *classp, unsigned char *maskp)
{
  KEY key ;

  if (maskp) *maskp = 0x00 ;  /* Normal situation */
  if (*classp > 256)
    { 
#ifdef OLDJUNK
      if ((obj = bsCreate(*classp)))
	{
	  if (bsGetData (obj,_Belongs_to_class, _Int, &topClass))
	    { n = 0 ;
	      bsGetData (obj,_Mask, _Int, &n) ;
	      *maskp = (unsigned char) n ;
	      *classp = topClass ;
	    }
	  bsDestroy(obj) ;
	}
#endif
      if (class(*classp) == _VClass && classMasks &&
	  (key = KEYKEY (*classp)) && key < arrayMax(classMasks))
	{ 
	  if (maskp) *maskp = arr(classMasks, key, CLASS_MASK).mask ;
	  *classp = arr(classMasks, key, CLASS_MASK).mainClasse ;
	  return TRUE ;
	}
      *classp = 0 ;
      return FALSE ;
    }

  return TRUE ;
}

/******************************************************************/

static void pickGetClassNames(void)
{ 
  int  k , v ;
  KEY key = 0 ;

  pickStack = stackCreate(200) ;
  pushText(pickStack,"System") ; /* prevents name = 0 */


  clA = arrayCreate(20, int) ;
  v = 0 ;

  while(lexNext (_VMainClasses, &key))
    { k = KEYKEY(key) ;
      if (*name(key) != '_')
	pickList[k].name = stackMark(pickStack) ;
      pushText(pickStack, name(key) );
      array(clA, v++, int) = k ;
    }
} 

/******************************************************************/

static KEYSET classAppearance = 0 ;
static FREEOPT classOptions[] =
{
  {13, "class options"},
  {'h', "Hidden"},
  {'v', "Visible"},
  {'a', "Array"},
  {'b', "Btree"},
  {'x', "XREF"},
  {'d', "Display"},
  {'t', "Title"},
  {'s', "Symbol"},
  {'r', "Rename"},
  {'c', "CaseSensitive"},
  {'p', "Protected"},
  {'S', "Sybase"},
  {'k', "Known"}
} ;

int pickMakeClass (char* cp)
{
  KEY key, model, class, classeKey ;
  int k, k1 ;
  

  lexaddkey (cp, &key, _VMainClasses) ;   k = KEYKEY (key) ;
  lexaddkey (cp, &classeKey, _VClass) ;   k1 = KEYKEY (classeKey) ;

  if (!classMasks) classMasks = arrayCreate (128, CLASS_MASK) ;
  array (classMasks, k1, CLASS_MASK).mainClasse = k ;

  if (!classComposite) classComposite = arrayCreate (128, Array) ;
  array (classComposite, k1, Array) = NULL ;

  if (_VModel)
    {
#ifdef ACEDB4
      lexaddkey (messprintf("?%s", name(key)), &model, _VModel) ;
#else
      lexaddkey (messprintf("#%s", name(key)), &model, _VModel) ;
#endif
      pickList[k].model = model ;
    }

  if (!pickList[k].name || !pickList[k].classe ||
      strcmp(stackText(pickStack, pickList[k].name),cp))
    { pickList[k].name = stackMark(pickStack) ;
      pushText(pickStack, cp) ;
      lexaddkey(cp, &class, _VClass) ;
      lexAlias(class, cp, FALSE, FALSE) ; /* rename it to fix case */

      pickList[k].type = 'B' ;	/* default is B */
      pickList[k].visible = 'v' ; /* for  B default is visible */
      /* attention, model.c redeclares them as V */

      pickList[k].classe = classeKey ;
    }

      /* all B classes should be listed in _VModel with the constructed types */

  return k ;
} /* pickMakeClass */
	
/******************************************************************/

void pickGetClassProperties (BOOL readDisplayTypeOK)
{
  int  k, nc, i ;
  char *cp, *filename ;
  ACEIN options_in;
  KEY tag , option, dummy ;
  static displayOK = FALSE;

  if (readDisplayTypeOK)
    displayOK = TRUE;

  filename = dbPathStrictFilName("wspec", "options","wrm", "r", 0);
  if (!filename)
    messExit("Cannot find Class definition file wspec/options.wrm") ;

  options_in = aceInCreateFromFile (filename, "r", "", 0);
  messfree(filename);

  if (!options_in)
    /* shouldn't really fail after the sessionFilName-check */
    messExit("Cannot open database without Class definition file!") ;


  if (!classAppearance)
    { nc = 0 ; classAppearance = keySetCreate() ; }
  else
    nc = keySetMax(classAppearance) ;

  aceInSpecial (options_in, "\n/\t\\");
  while (aceInCard(options_in))
    if ((cp = aceInWord(options_in)) && *cp++ == '_' && *cp++ == 'V')
      {
	if(!*cp)
	  messExit("Error in wspec/options.wrm at line %d : "
		   "Isolated _V token, please specify a class",
		   aceInStreamLine(options_in)) ;

	if (!strcmp (cp, "System"))
	  k = 0 ;
	else if (!lexword2key(cp, &dummy, _VMainClasses))
	  continue ; /* disregard option of classes without models */
	else
	  k = pickMakeClass (cp) ;

	for (i = 0 ; i < nc ; ++i)
	  if (keySet(classAppearance, i) == k)
	    break ;
	if (i == nc)		/* not found */
	  keySet(classAppearance, nc++) = k ;

	while (TRUE)
	  {
	    aceInNext (options_in) ;
	    if (!aceInStep(options_in, '-')) 
	      {
		if ((cp = aceInWord(options_in)))
		  messExit ("Error in wspec/options.wrm at line %d : "
			    "no hyphen '-' at start of option %s", 
			    aceInStreamLine(options_in),
			    aceInPos(options_in)) ;
		else
		  break ;
	      }

	    if (!aceInKey(options_in, &option, classOptions))
	      messExit("Error in wspec/options.wrm at line %d : "
		       "unknown option %s",
		       aceInStreamLine(options_in),
		       aceInPos(options_in)) ;
	    
	    switch (option) 
	      {
	      case 'h':
		pickList[k].visible = 'H' ;  /* default is visible */
		break ;
	      case 'v':
		pickList[k].visible = 'V' ;  /* default is visible */
		break ;
	      case 'a':
		pickList[k].type = 'A' ;
		break ;
	      case 'b':
		pickList[k].type = 'B' ;
		break ;
	      case 'x':
		pickList[k].Xref = TRUE ;
		pickList[k].visible = 'H' ; 
		pickList[k].type = 'B' ;
		break ;
	      case 'd':		/* Display type */
		aceInNext(options_in) ;
		cp = aceInWord(options_in) ;
		if (displayOK)
		  {
		    if (!lexword2key (cp, &(pickList[k].displayKey), _VDisplay))
		      messExit ("Error in wspec/options.wrm at line %d : "
				"Bad display type %s", 
				aceInStreamLine(options_in), cp) ;
		  }
		break ;
	      case 't':
                               /* Tag for Name expansion  */
		pickList[k].tag = 0 ;
		if ((cp = aceInWord(options_in)))
		  { 
		    lexaddkey(cp, &tag, _VSystem) ; 
		    pickList[k].tag = tag ;
		  }
		break ;
	      case 's':
                               /* Tag for Symbol in maps  */
		pickList[k].symbol = 0 ;
		if ((cp = aceInWord(options_in)))
		  { 
		    lexaddkey(cp, &tag, _VSystem) ; 
		    pickList[k].symbol = tag ;
		  }
		break ;
	      case 'r':   /* other class name */
		if ((cp = aceInWord(options_in)))
		 {
		   if (!pickList[k].alias)  /* Always keep the compilation value */
		     pickList[k].alias = pickList[k].name ;
		   pickList[k].name = stackMark(pickStack) ;
		   pushText(pickStack, cp) ;
		 }
		break ;
	      case 'c':
		pickList[k].isCaseSensitive = TRUE ;
		break ;
	      case 'p':
		pickList[k].protected = TRUE ;
		break ;
	      case 'S':
		pickList[k].sybase = TRUE ;
		break ;
	      case 'k':
		pickList[k].known = TRUE ;
		break ;
	      }
	  }
      }
  aceInDestroy (options_in);

  return;
} /* pickGetClassProperties */

/******************************************************************/
   /* called from whooks/sysclass.c */ 
void pickSetClassOption (char *nom, char type,  char v, 
			 BOOL protected, BOOL private)
{
  int i, k, nc ;

  if (!classAppearance)
    { nc = 0 ; classAppearance = keySetCreate() ; }
  else
    nc = keySetMax(classAppearance) ;

  if (!strcmp (nom, "System"))
    k = 0 ;
  else
    k = pickMakeClass (nom) ;

  for (i = 0 ; i < nc ; ++i)
    if (keySet(classAppearance, i) == k)
      break ;
  if (i == nc)		/* not found */
    keySet(classAppearance, nc++) = k ;
  
  if (!pickList[k].visible || pickList[k].visible == freelower(pickList[k].visible))
    pickList[k].visible = v ;
  pickList[k].type = type ;
  if (type == 'X')
    { pickList[k].Xref = TRUE ;
      pickList[k].type = 'B' ;
      pickList[k].isCaseSensitive = TRUE ;
    }

  pickList[k].protected = protected ;  /* do not parse */
  pickList[k].private = private ;      /* do not dump */
#ifdef ACEDB4
  lexaddkey (messprintf("?%s", nom), &pickList[k].model, _VModel) ;
#else
  if (pickList[k].type == 'B') 
    lexaddkey (messprintf("#%s", nom), &pickList[k].model, _VModel) ;
#endif
  lexaddkey (nom, &pickList[k].classe, _VClass) ;
  
  return;
} /* pickSetClassOption */

void pickSetClassDisplayType (char *nom, char *displayName)
{
  KEY key;
  int k;

  if (!strcmp (nom, "System"))
    k = 0 ;
  else
    {
      if (!lexword2key (nom, &key, _VMainClasses))
	messcrash("pickSetClassDispType() - bad class name %s", nom);
      k = KEYKEY (key) ;
    }
  
  /* this function needs to be called after the main display types
   * have been initialised (only displays with a DisplayFunc should be
   * present in the class 'Display' at this point - see displayInit) */
  /* If it's called before the display types are initialised 
     (and it is, from getNewModels on startup with no database,
     it now fails silently - srk */
  lexword2key(displayName, &(pickList[k].displayKey), _VDisplay);
  
  return;
} /* pickSetClassDisplayType */

/******************************************************************/

  /* Try name, then alias = compilation name */
int  pickWord2Class(char *word)     /* returns class of word */
{
  register int i = 256 ;
#ifdef ACEDB4
  while(--i && lexstrcmp(word,stackText(pickStack, pickList[i].name))) ;
  if (!i)
    { i = 256 ;
      while(--i && 
	    lexstrcmp(word, stackText(pickStack, pickList[i].alias)));
    }
#else
  if (!lexstrcmp(word,"System"))
    i = 0 ;
  else
    while (--i && lexstrcmp(word,className(i <<24))) ;
#endif
  return i ;
} /* pickWord2Class */

/*******************************************************************/
/*******************************************************************/

/* match to template with wildcards.   Authorized wildchars are * ? #
     ? represents any single char
     * represents any set of chars
   case sensitive.   Example: *Nc*DE# fits abcaNchjDE23

   returns 0 if not found
           1 + pos of first sigificant match (i.e. not a *) if found
*/

static int doPickMatch (char *cp, char *tp, BOOL caseSensitive)
{
  char *c=cp, *t=tp;
  char *ts = 0, *cs = 0, *s = 0 ;
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
      case '?':
	if (!*c)
	  return 0 ;
	if(!s) s = c ;
        t++ ;  c++ ;
        break;
      case '*':
        ts=t;
        while( *t == '?' || *t == '*')
          t++;
        if (!*t)
          return s ? 1 + (s-cp) : 1 ;
	if (!caseSensitive)
	  {
	    while (freeupper(*c) != freeupper(*t))
	      if(*c)
		c++;
	      else
		return 0 ;
	  }
	else
	  {
	    while (*c != *t)
	      if(*c)
		c++;
	      else
		return 0 ;
	  }
        star=1;
        cs=c;
	if(!s) s = c ;
        break;
      default:
	if (!caseSensitive)
	  {      
	    if (freeupper(*t++) != freeupper(*c++))
	      { if(!star)
		return 0 ;
	      t=ts; c=cs+1;
	      if(ts == tp) s = 0 ;
	      }
	    else
	      if(!s) s = c - 1 ;
	  }
	else
	  {
	    if (*t++ != *c++)
	      { if(!star)
		return 0 ;
	      t=ts; c=cs+1;
	      if(ts == tp) s = 0 ;
	      }
	    else
	      if(!s) s = c - 1 ;
	  }
        break;
      }
} /* pickMatch */

int pickMatch (char *cp,char *tp)
{ return doPickMatch (cp,tp,FALSE) ; }
int pickMatchCaseSensitive (char *cp,char *tp, BOOL caseSensitive)
{ return doPickMatch (cp,tp,caseSensitive) ; }

/**************************************************************/

static void pickDefaultProperties(void)
{
  int j, k ;  

      /* clA is used to have the Visible Class in main menu in
       * same order as they appear in classes.wrm.
       */
  for (j = 0 ; j < arrayMax(clA) ; j++)
    { k = array(clA, j, int) ;
      if (pickList[k].name)
	{
	  if (!pickList[k].type)            /* default is B */
	    pickList[k].type = 'B' ;
#ifdef JUNK
	  /* too early, .model is not yet established */
	  if (!pickList[k].visible)
	    {
	      if (pickList[k].type == 'B' &&  /* for  B default is visible */
		  iskey(pickList[k].model)==2) /* iff the model exists */
		pickList[k].visible = 'v' ;   
	      else
		pickList[k].visible = 'h' ;   /* for  A default is hidden */
	    }
#endif
	}
    }

  arrayDestroy(clA);

  return;
} /* pickDefaultProperties */

/******************************************************************/

void pickRegisterConstraints (void)
{ OBJ obj ;
  Stack s = 0 ;
  char *cp ;
  KEY key = 0 ;
  KEY mainClasse = 0 ;
  
  while (lexNext (_VClass, &key))
    { 
      mainClasse = 0 ;
      if ((obj = bsCreate (key)))
	{ if (bsGetData (obj, _Constraints, _Text, &cp))
	    { if (!s)
		s = stackCreate (50) ;
	      pushText (s, messprintf ("(%s)", cp)) ;
	      while (bsGetData (obj, _bsDown, _Text, &cp))
		catText (s, messprintf ("AND ( %s )", cp)) ;
	      mainClasse = key ;
	      pickIsA (&mainClasse, 0) ;
	    }
	  bsDestroy (obj) ;
	}
      if (s && stackMark (s) && mainClasse &&
	  !queryConstraintsInit (stackText(s,0), mainClasse))
	messExit ("Syntax error in wspec/constraints.wrm, class %s\n: %s\n", 
		   pickClass2Word (mainClasse), stackText(s,0)) ;
      if (s) stackClear (s) ;
    }
  stackDestroy (s) ;
} /* pickRegisterConstraints */

/********************************************************/
/******************* Public Routine *********************/
/********************************************************/

/* pickPreInit is a duplicate of pickInit
it could certainly be simplified but this works like that
so i don t care (mieg, oct 18,94)
*/


void pickPreInit(void)
{ 
  BOOL old = READING_MODELS ;
  register int k ;
  
  READING_MODELS = TRUE ;

  k = 256 ;
  while(k--)
    pickList[k].type = 0;
  pickList[_VGlobal].type = 'A' ;
  pickList[_VVoc].type = 'A' ;

  sysClassInit () ;
  pickList[_VClass].type = 'B' ; /* needed for pickMakeClass() to work */
  pickList[_VDisplay].type = 'B' ;
  classInit () ;

  pickGetClassNames() ;

  pickMakeClass ("Session") ;

  pickGetClassProperties(FALSE) ;	/* options.wrm */
  sysClassOptions() ; 

  pickDefaultProperties() ;

  READING_MODELS = old ;

  return;
} /* pickPreInit */



void pickInit(void)
{
  BOOL old = READING_MODELS ;
  register int k ;
  
  READING_MODELS = TRUE ;

  if (classAppearance)
    keySetReCreate(classAppearance);

  k = 256 ;
  while(k--)
    { pickList[k].type = 0;
      pickList[k].name = 0;
      pickList[k].alias = 0;
      pickList[k].visible = 0;
      pickList[k].mask = 0;
      pickList[k].model = 0;
      pickList[k].displayKey = 0;
      pickList[k].protected = 0;
      pickList[k].Xref = FALSE;
    }
  pickList[_VGlobal].type = 'A' ;
  pickList[_VVoc].type = 'A' ;

  sysClassInit () ;
  pickList[_VClass].type = 'B' ; /* needed for pickMakeClass() to work */
  pickList[_VDisplay].type = 'B' ;
  classInit () ;

  pickGetClassNames() ;

  pickGetClassProperties(FALSE) ;
  sysClassOptions() ;

  pickDefaultProperties() ;
  pickRegisterConstraints () ;

  READING_MODELS = old ;

  return;
} /* pickInit */

/******************************************/


/********************************************************/

static void pickCheckClassDefinitions(void)
{ 
  KEY metaClass = 0, _Buried ;
  int n = 256 ; 
  OBJ obj; 

#ifdef ACEDB4
  if (iskey(KEYMAKE(_VClass,0)) != 2)
    return ;   /* i.e. the Class model does not yet exists   */
#else
  if (!lexword2key("#Class", &metaClass, _VModel))
    return ;   /* i.e. the Class model does not yet exists   */
#endif
  lexaddkey ("Buried", &_Buried, 0) ;
  while (n--)
    { if (!pickList[n].name)
	continue ;

      lexaddkey(pickClass2Word(n), &metaClass, _VClass) ;
      obj = bsUpdate(metaClass) ;

	  /* too early, .model is not yet esatablished */
	  if (!pickList[n].visible)
	    {
	      if (pickList[n].type == 'B' &&  /* for  B default is visible */
		  iskey(pickList[n].model)) /* iff the model exists */
		pickList[n].visible = 'v' ;   
	      else
		pickList[n].visible = 'h' ;   /* for  A default is hidden */
	    }

      if (freelower(pickList[n].visible) == 'v')
	bsAddTag(obj, _Visible) ;   
      else if (pickList[n].visible == 'H')
	bsAddTag(obj, _Hidden) ; /* show in the triangle */
      else
	bsAddTag(obj, _Buried) ;    /* show nowhere */

#ifdef ACEDB4
      /* KLUDGE, mieg sept 96
	 there is clearly a bug here, 
	 consisder a subclass sub of class c
	 sub should not have an entry in pickList and mainclasses
         furthermore, without the kludge, Belongs to class 
	 is reset in each newe session to sub then to c
	 implying always some locked obj, which imply
	 taht cache 1 has to get write access silently
         to prevent complete locking

	 this kludge avoids the problem, but the code
	 is not clean as we know in this area
	 */
      if (!bsFindTag(obj, _Is_a_subclass_of)) /* KLUDGE */
	/* compiled_as is used in bindex.c before sep 1998
	 * Belongs_to_class is used in pickIsA before sep 1998
         * it is therefore essential to position them correctly
	 * untill ACEDB5
	 */
	{ 
	  bsAddData(obj, _Compiled_as,  _Text, 
		  stackText(pickStack, 
			    pickList[n].alias ? pickList[n].alias : pickList[n].name
			    )) ;
          bsAddData(obj, _bsRight, _Int, &n) ;
	  bsAddData(obj, _Belongs_to_class, _Int, &n) ;
	}
#endif
      bsSave(obj) ;
    }
} /* pickCheckClassDefinitions */

/**********************************************************************/

static void pickRegisterClassMasks(void)
{
  KEY key = 0 ;
  int n, k ;
  OBJ obj = 0 ;

  while (lexNext(_VClass, &key))
    {
      obj = bsCreate (key) ;
      k = KEYKEY (key) ;

      if (obj)
	{
	  if (bsGetData(obj, _Belongs_to_class, _Int, &n))
	    array(classMasks, k, CLASS_MASK).mainClasse  = n ;

	  if (bsGetData (obj, _Mask, _Int, &n))
	    array(classMasks, k, CLASS_MASK).mask  = n ;

	  /* The "Composite_of" tag is bound to be followed by several class keys,
	   * otherwise what's the point in using it ! So copy them across into
	   * an array of keys, _note_ that the bsFlatten array will not do as it is
	   * a union of among other things a pointer which may be 8 bytes. */
	  if (bsFindTag(obj, str2tag("Composite_of")))
	    {
	      Array units = arrayCreate(10, BSunit) ;

	      if (bsFlatten(obj, 1, units))
		{
		  int i ;
		  Array keys ;

		  keys = arrayCreate(10, KEY) ;

		  for (i = 0 ; i < arrayMax(units) ; i++)
		    {
		      BSunit *u ;

		      u = arrayp(units, i, BSunit) ;

		      array(keys, i, KEY) = u[0].k  ;
		    }

		  array(classComposite, k, Array) = keys ;

		  /* Do not make any real objects in this class */
		  pickList[k].protected = TRUE ;
		}

	      arrayDestroy(units) ;
	    }

	  bsDestroy (obj) ;
	}
    }      
} /* pickRegisterClassMasks */

/**********************************************************************/
void pickVisibility(void)
{ KEY  *classp, metaClass ;
  OBJ obj ;
  KEYSET subclasses = query(0, ">?Class Is_a_subclass_of") ;
  int v = 0, i ;

  i= keySetMax(classAppearance) ;
  if(!i)
    return ;


  classp = arrp(classAppearance,0,KEY) - 1 ;
  while(classp++, i--)
    if (lexword2key(pickClass2Word(*classp), &metaClass, _VClass) &&
	( obj = bsCreate(metaClass)) )
      { if (bsFindTag(obj, _Visible))
	  {
	    v++ ;
	    pickVocList[v].text = name(metaClass) ;
	    pickVocList[v].key = metaClass ;	  
	  }
	
	bsDestroy(obj) ;
	if (v >= 254)
	  break ;
      }

  i= keySetMax(subclasses) ;
  
  classp = arrp(subclasses,0,KEY) - 1 ;
  while(classp++, i--)
    if ((obj = bsCreate(*classp)))
	{ if (bsFindTag(obj, _Visible))
	    {
	      v++ ;
	      pickVocList[v].text = name(*classp) ;
	      pickVocList[v].key = *classp ;	  
	    }

	  bsDestroy(obj) ;
	  if (v >= 254)
	    break ;
	}

  keySetDestroy(subclasses) ;
  pickVocList[0].key = v ;
  pickVocList[0].text =  "Choose a class" ;
  if (v < 1)
    messout("This database has no visible class.\n"
            "Most probably, you should study and edit\n"
	    "the files wspec/options.wrm and wspec/models.wrm,\n",
	    "then use the option Read-Models from main menu.") ;
}

/********************************************************/

void pickCheckConsistency(void)
{
  extern BOOL classSortAll(void) ;

  pickCheckClassDefinitions() ;
  pickRegisterClassMasks () ;
  pickVisibility() ;
  classSortAll() ;
} /* pickCheckConsistency */








/********************* eof ***********************************/
