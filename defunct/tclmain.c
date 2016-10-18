/* From hcobb@fly2.berkeley.edu Thu Apr 21 19:00:08 1994 */

/*

	In order to run that script, you'd need scriptace, so here 'tis.

	Grab Tcl 7.3, from the usual euro-ftp sites, and then build this
exactly like Aceserver, only with the TCL includes and libs instead of the
Xdr and rpc stuff.

	Note that I've done a little cleanup of memory leaks from
Aceserver, and if server was actually of any further use, I'd suggest
back-propagating them.

*/


/*  File: scriptace.c
 *  Author: Jean Thierry-Mieg (mieg@mrc-lmba.cam.ac.uk)
 *  Copyright (C) J Thierry-Mieg and R Durbin, 1992
 *-------------------------------------------------------------------
 * This file is part of the ACEDB genome database package, written by
 * 	Richard Durbin (MRC LMB, UK) rd@mrc-lmba.cam.ac.uk, and
 *	Jean Thierry-Mieg (CRBM du CNRS, France) mieg@frmop11.bitnet
 *
 * Description:
     main module for Tcl parsing ace variant.
 * Exported functions:
 * HISTORY:
     Mutated from aceserver.c on Tue Nov 30 14:01:35 PST 1993 by HJC.

 * Last edited: Dec 12 18:18 1995 (mieg)
 * Created: Wed Nov 25 12:26:06 1992 (mieg)
 *-------------------------------------------------------------------
 $Revision: 1.11 $
 */


/* $Id: tclmain.c,v 1.11 1995-12-12 17:20:41 mieg Exp $ */


/*******************************************************************/
/*   A C e D B                                                     */
/*   A Caenorhabditis elegans Data Base                            */
/*               by R.Durbin and J.Thierry Mieg                    */
/*******************************************************************/
/*               Network Query Server                              */
/*******************************************************************/

/*  Version number is #defined  in session.h                       */

extern void 
  blockInit(void),
  lexInit(void),
  pickInit(void),
  pickInit(void),
  sessionAutoSelect(void) ;

extern char *stackorigin ;            /* in arraysub */
extern int isInteractive ;           /* in freesubs */

void wormClose(void) ;

#include "acedb.h"
#include "sysclass.h"
#include "session.h"
#include "parse.h"
#include "pick.h"
#include "freeout.h"
#include "bs.h"
#include "spread.h"
#include "systags.h"
#include "classes.h"
#include "tags.wrm"

#include <tcl.h>

extern Stack errorstack;  /* Hidden buffer used by messout -HJC*/

/* #include "nace_com.h" Not needed in scriptace -HJC*/

/* begin suz & SUZ totally perverse patch */
extern ParseFunc parseFunc[] ;
extern BOOL dnaParse (int level, KEY key) ;
extern BOOL longTextParse (int level, KEY) ;
extern BOOL keySetParse (int level, KEY) ;
extern int  dumpFastAFile (FILE *fil, KEYSET ks) ;

static void
parseInit (void)
{
    parseFunc[_VKeySet]  = keySetParse ;
    parseFunc[_VLongText]  = longTextParse ;
    parseFunc[_VDNA]  = dnaParse ;
}

/* end suz & SUZ totally perverse patch */

static Array clients = 0 ;
typedef struct LOOKSTUFF { Array ksNew, ksOld ; } *LOOK ;
/*******************************************************************/
/********** Missing routines ***************************************/

void graphCleanUp(void)
{
  return ;
}

void graphText (char *text, float x, float y)
{ printf(text) ;
}

char *graphHelp(char *x)
{ return NULL;
}
void graphClear(void)
{ printf("\n\n") ;
  return ;
}

BOOL graphExists(Graph g)
{ return 0 ;
}

void graphRedraw(void)
{ printf("\n") ;
  return ;
}

void graphTextBounds(int w, int h)
{ return ;
}

int graphColor(int color)
{ return 0;
}

Graph   graphCreate (int a, char *b, float c, float d, float e, float f)
{ return NULL ;
}

void mainActivity(char *noway)
{
    return;
}

/******************* INITIALISATION ***************************/
/**************************************************************/

static void showModel(KEY key, Stack s)
{
  FILE *f = filopen("wspec/models", "wrm","r") ;
  char *cp , buf[256] ;

  buf[255] = 0 ;
  if (!f)
    messcrash("showModel can t find the model file") ;
  while (freeread(f))  /* look for ?className */
    if ((cp = freeword()) && *cp == '?' && !strcmp(cp+1, className(key)))
      break ;

  if (strcmp(cp+1, className(key)))
    { catText(s, messprintf ("Model of class %s not found", className(key))) ;
      return ;
    }
  catText(s, cp) ;
  catText(s, " ") ;
  catText(s, freepos()) ;
  catText(s, "\n") ;
  while (cp = fgets(buf, 255, f))
    if (*cp == '?')
	  { fclose(f) ;
	    return ;
	  }
    else  /* printout */
      { catText(s, cp) ; catText(s, "\n") ;}
}

/****************** MAIN LOOP **********************/

#include "keyset.h"
#include "query.h"
#include "a.h"
#include "dump.h"
#include "lex.h"

static FREEOPT qmenu[] = 
{ 
  22,  "acedb",
  'x', "Quit :   Exits the program.       Any command can be abbreviated, $starts a subshell",
  'h', "Help : for further help",
  '?', "? : List of commands",
  'c', "Classes : Give a list of all the visible class names and how many  objects they contain.",
  'n', "Find class [name]: Creates a new current list with all objects from class, optionally matching name",
  'z', "Model class: Shows the model of the given class, useful before Follow or Where commands",
  'g', "Grep template: searches for *template* everywhere in the database",
  'u', "Undo : returns the current list to its previous state",
  'l', "List [template]: lists names of items in current list [matching template]",
  's', "Show [tag] : shows the [branch tag of] objects of current list",
  'i', "Is template : keeps in the active list only those objects whose name match the template",
  'r', "Remove template : removes from the active list only those objects whose name match the template",
  't', "Follow Tag : i.e. Tag Author gives the list of authors of the papers of the current list", 
  'q', "Where query_string : performs a complex query - 'help query_syntax' for further info",
  'w', "Write filename : acedump current list to file",
  'b', "Biblio : shows the associated bibliography",
  'd', "Dna filename : Fasta dump of related sequences", /* suz, added filename */
  'a', "Array class:name format : formatted acedummp of A class object",
  'p', "Parse file : parses an aceFile, or stdio, or Tclvar until \000 or EOF",
  'T', "Table-Maker file outputfile : Executes the file as a table-maker.def command file, outputs ",
  'f', "First : hands off first element of keylist into Tcl variable.",
  'e', "Next : Places next element of keylist into named Tcl variable."
} ;

static FREEOPT helpmenu[] =	/* no \n inside FREEOPT text (else rethink) */
{ 5, "helpmenu",
  1, "Tacedb : copyright etc", 
  2, "Query_syntax for the find command",
  3, "Useful_sequences : SL1 etc",
  4, "Clone_types used in the physical map (vector etc)",
  5, "DNA_and_amino_acids_nomenclature : codes for bases/amino acids and translation table",
} ;

 /* By convention, each reply starts with a message ending on /n
    which will be deleted if aceclient runs silently rather than verbose 
    */

static int processTclQueries(ClientData clientData, Tcl_Interp *interp, int argc, char *argv[])
{ 
  static KEYSET ksNew = 0 , ksOld = 0 , kA = 0 , kClass = 0;
  int nn = 0 , i, j, class, successcode;
  KEY option;
  FILE *f ;
  char *cp , c ;
  static Stack grepText = 0 , localStack = 0 ;
  static Stack s = 0 ;   static char *oldAnswer = 0 ;  /* Answer buffer */
  static char *question, *answer ;
  static int nClients = 0 , clientId =0;
  LOOK look ;
  int reply_len;
  static int walking = -1; /* State variable for keylist walks. */
  static char *varname=0; /* Name of Tcl Variable */

  successcode = TCL_OK; /* Assume it works -HJC*/

  if (!clientId)
    { look = (LOOK) messalloc(sizeof(struct LOOKSTUFF)) ;
      ++nClients ;
      clientId = nClients;
      array(clients, nClients, LOOK) = look ;
    }     
  
  if (clientId <= 0  ||
      clientId >= arrayMax(clients))
    {
	char msg[256];
	sprintf(msg,"%d",clientId);
	Tcl_SetErrorCode(interp, "ACEDB",
	    "Aceserver received a bad client id",msg, (char *) NULL) ;
	return TCL_ERROR ;
    }

  look = arr(clients, clientId, LOOK) ;
  ksNew = look->ksNew ;
  ksOld = look->ksOld ;
  
  if(question && strlen(question))
      free(question);

  if(1 < argc)
  {
      /*There are paramters to deal with */
      question = Tcl_Merge(argc-1,argv+1);
          /* Skip over the "acedb" string that Tcl called us with */
  }
  else
  {
      question = "";
  }
  s = stackReCreate(s, 20000) ;
  freeforcecard(question) ;
/*   fprintf(stderr,"aceserver received %s\n", question->cmd) ; */
    { 
      if (freekey (&option, qmenu))
      switch (option)
	{
	case 'x':
	  wormClose() ;
	  return TCL_OK;
	case 'h':
	  if (freekey (&option, helpmenu))
	    { freeforcecard (helpmenu[option].text) ;
	      /* helpAnswer (stdout, freeword()) ; */
	      messout("\nSorry help topic not coded") ;
	    }
	  else
	   {catText(s, "ACEDB Server Usage:\n"
"Commands are not case sensitive and can be abbreviated.\n"
"Lines starting with @ will take the next word as a file name to read\n"
"commands from (include file), passing subsequent words on the line as parameters.\n"
"Lines starting with $ execute the remainder of the line in an interactive subshell.\n"
"Everything following // on a line is ignored (comment).\n"
"To escape any of these special characters use a backslash, eg \\@.\n\n"
"This program maintains an internal list of current objects. \n "
"You can List or Show or Write the content of the list.\n"
"You can change the list with the simple commands: \n"
"       Grep, Find, Is, Follow, Biblio. \n"
"Perform complex queries with: \n"
"       Where search-string', where 'search-string' follows the acedb query syntax.\n"
"Print relational tables with Table table-command-file\n\n"
"To see a list of all possible commands type ?.\n\n"
"For further help on any of the following features type 'help feature'\n"
		     ) ;
	      for (i = 1 ; i <= helpmenu[0].key ; i++)
		catText (s,helpmenu[i].text) ;
	    }
	  break ;
	case '?':
	  catText(s, "ACEDB Server list of commands:\n") ;
	  for (i = 1 ; i <= qmenu[0].key ; i++)
	  {
	      catText(s,qmenu[i].text) ;
	      catText(s,"\n");
	  }
	  break ;   
	case 'l':   /* lists names matching template */
	  if (!ksNew || !keySetExists(ksNew))
	    { messout("\nActive list is empty") ;
	      break ;
	    }
	  if (cp = freeword())
	    { kClass = keySetCreate() ;
	      for (i = 0, j = 0 ; i < keySetMax(ksNew) ; i++)
	      if (pickMatch(name(keySet(ksNew,i)), cp))
		keySet(kClass, j++) = keySet(ksNew, i) ;
	    }
	  else
	    kClass = keySetCopy(ksNew) ;
	  keySetDestroy(kA) ;
	  walking = -1; /* Just destroyed walk state */
	  kA = keySetAlphaHeap(kClass, keySetMax(kClass)) ;
	  keySetDestroy(kClass) ;
	  catText(s, messprintf("KeySet : Answer_%d\n",++nn)) ;
	  keySetDump(0, s, kA) ;
	  break ;

	case 'i':   /* keeps names matching template */
	  if (!ksNew)
	    { catText(s, "Active list is empty") ;
	      break ;
	    }
	  if (cp = freeword())
	    { if (ksNew && ksOld != ksNew)
		{ keySetDestroy(ksOld) ;
		  ksOld =  ksNew ;
		}
	      ksNew = keySetCreate() ;
	      for (i = 0, j = 0 ; i < keySetMax(ksOld) ; i++)
	      if (pickMatch(name(keySet(ksOld,i)), cp))
		keySet(ksNew, j++) = keySet(ksOld, i) ;
	      catText(s, messprintf(" %d objects kept ", keySetMax(ksNew))) ;
	    }
	  else
	    catText(s, "Usage Keep template : keeps names from current list matching template ") ;
	  break ;
	case 'r':   /* remove names matching template */
	  if (!ksNew)
	    { catText(s,"Active list is empty") ;
	      break ;
	    }
	  if (cp = freeword())
	    { if (ksNew && ksOld != ksNew)
		{ keySetDestroy(ksOld) ;
		  ksOld =  ksNew ;
		}
	      ksNew = keySetCreate() ;
	      for (i = 0, j = 0 ; i < keySetMax(ksOld) ; i++)
	      if (!pickMatch(name(keySet(ksOld,i)), cp))
		keySet(ksNew, j++) = keySet(ksOld, i) ;
	      catText(s,
		       messprintf(" %d objects kept ", keySetMax(ksNew))) ;
	    }
	  else
	    catText(s,"Usage Remove template : remove names matching template") ;
	  break ;
	case 't':   /* Tag follow */
	  if (!ksNew || !keySetMax(ksNew))
	    { printf ("Active list is empty") ;
	      break ;
	    }
	  class = 0 ;
	  if (cp = freeword())
	    { /* Autocomplete to a tag */
	       KEY tag = 0 ;
	      
	      while (lexNext(0, &tag))
		if(pickMatch(name(tag), cp))
		  { if (strcmp(cp, name(tag)))
		      catText(s, messprintf ("Autocompleting %s to %s",
			      cp, name(tag))) ;
		    if (ksNew && ksOld != ksNew)
		      { keySetDestroy(ksOld) ;
			ksOld =  ksNew ;
		      }
		    ksNew = query(ksOld, messprintf(">%s", name(tag))) ;
		    goto goodTag ;
		  }
	      catText(s, messprintf ("Sorry, Tag %s unknown ", cp)) ;
	      break ;
	    }
	  else
	    { catText(s,"Usage Follow tag-name, change to list of objetcs pointed by tag, ") ;
	      catText(s, "type Z class to see the model of the class") ;
	      break ;
	    }
	goodTag:
	  catText(s, messprintf ("Found %d objects", keySetMax(ksNew))) ;
	  break ;
	case 's':   /* Show objects */
	  /* look for the class name */
	  if (!ksNew || !keySetMax(ksNew))
	  {
	      catText(s,"Active list is empty") ;
	      break ;
	  }
	  class = 0 ;
	  
	  localStack = stackReCreate(localStack, 80) ;
		/*  pushText(localStack,"") ;
		  while (cp = freeword())
		  {
		  catText(localStack, cp) ;
		  catText(localStack, " ");
		  }
		  */	  
	  kClass = keySetCopy(ksNew) ;
	  if (kClass && keySetMax(kClass))
	  {
	      keySetDestroy(kA) ;
	      walking = -1; /* Just destroyed Walk state */
	      kA = keySetAlphaHeap(kClass, keySetMax(kClass)) ;
	      catText(s,"\n") ;
	      out = freeOutSetStack (s) ;
	      for (i = 0 ; i < keySetMax(kA) ; ++i)
		  dumpKeyBeautifully (keySet(kA,i), 0, s, stackText(localStack,0)) ;
	      freeOutClose (out) ;
	  }
	  else
	      catText(s,"No match ") ;
	  keySetDestroy(kClass) ;
	  break ;
	case 'u':   /* undo */
	  if (ksOld)
	    { keySetDestroy (ksNew) ;
	      ksNew = ksOld ;
	      ksOld = 0 ;
	      catText(s,messprintf("Recovered %d objects", keySetMax(ksNew))) ;
	    }
	  else{
	      messout("\nNo more previous list available") ;
	  }
	  break ;
	case 'c':  /* Class */
	  catText(s,
     "These are the known classes and the number of objects in each class \n");
	  for(i=0; i<256;i++)
	    if(pickVisible(i))
	      catText(s, messprintf("%35s %d \n", pickClass2Word(i),lexMax(i) - 2 )) ;
	  break ;
	case 'n':   /* New class */
	  cp = freeword() ;
	  if (!cp) 
	    { catText(s, messprintf ("Usage Find Class [name]: Construct a new current list"
		      " formed with all objects belonging to class \n")) ;
	      break ;
	    }
	  localStack = stackReCreate(localStack, 80) ;
	  catText(localStack, cp) ;
	  class = 0 ;
	  if (cp = freeword())
	  {
	      keySetDestroy(kA) ;
	      kA = query(0, messprintf("FIND %s IS %s",
				    stackText(localStack, 0),cp)) ;
	  }
	  else
	  {
	      keySetDestroy(kA) ;
	      kA = query(0, messprintf("FIND %s", stackText(localStack, 0))) ;
	  }
	  walking = -1; /* Just destoryed walk state */
	  if (kA && keySetMax(kA))
	    { if (ksNew && ksOld != ksNew)
		{ keySetDestroy(ksOld) ;
		  ksOld =  ksNew ;
		}
	      ksNew = kA ; kA = 0;
	      catText(s, 
		       messprintf ("Found %d objects in this class", keySetMax(ksNew))) ;
	    }
	  break ;

	case 'z':   /* Z class-model */
		   cp = freeword() ;
	  if (!cp) 
	    { messout("\nUsage Model Class : Shows the model of given class,\n"
		       "type C to list the models") ;
	      break ;
	    }
	  class = 0 ;
	  { /* Autocomplete to a Class */
	    KEY key, class = 0;
		
	    if (lexNext(_VClass,&class))
	      while (lexNext(_VClass,&class))
		if (pickMatch
		    (name(class), cp) )
		  { unsigned char mask ;   
  
		    key = class ;
		    pickIsA (&class, &mask) ;
		    if (mask)
		      { catText(s, messprintf("%s is a sub class of %s, here is its definition\n\n",
			       name(key), pickClass2Word(class))) ;
			dumpKey(key, 0, s) ; /* dumpKeyBeautifully (key, 0, s,0) ; */
		      }
		    else
		      {
			key = KEYMAKE(class, 0) ;
			if (pickType(key) == 'B')
			showModel(key,s) ;  /* dumpKey refuses to dump models */
			/* because they could not be parsed */
			else
			  messout(messprintf("Sorry, %c class %s has no model", 
						pickType(key), name)) ;
		      }
		    goto goodClass;
		  }
	  }
	  messout(messprintf ("Sorry, class %s is unknown, type C "
		  "for a list of known classes. ", cp)) ;
	goodClass:
	  break ;
	case 'q':   /* Performs a complex query */
	  if (!(cp = freewordcut("",&c)))
	    break ;
	  localStack = stackReCreate(localStack, 80) ;
	  catText(localStack, cp) ;
	  if (ksNew && ksOld != ksNew)
	    { keySetDestroy(ksOld) ;
	      ksOld =  ksNew ;
	    }
	  ksNew = query(ksOld,stackText(localStack, 0)) ;
	  if (ksNew != ksOld)
	    catText(s, messprintf("Found %d objects", keySetMax(ksNew))) ;
	  break ;
	case 'g':   /* Match == Grep */
		   if (!(cp = freewordcut("",&c)))
	    break ;
	  if (ksNew)
	    { keySetDestroy(ksOld) ;
	      ksOld =  ksNew ;
	    }
	  catText(s,"I search for texts or object names matching *") ;
	  catText(s, cp) ;
	  catText(s,"* ") ;
	  grepText = stackReCreate(grepText,20) ;
	  catText(grepText,messprintf("*%s*",cp)) ;
	  ksNew = queryGrep(stackText(grepText,0)) ;
	  break ;
/*
	case 'a':
	  if (lexClassKey (freeword(), &key)
	      && (fmt = freeword())
	      && (a = uArrayGet(key, sizeOfFormattedArray(fmt), fmt)))
	    { dumpFormattedArray (stdout, a, fmt) ;
	      arrayDestroy (a) ;
	    }
	  break ;
*/
	case 'b':
	    catText(s, "   Associated bibliography ") ;
	  {
	    /* We need to declare all bib... to be able to destroy them */
	    
	    static KEYSET bib1, bib2, bib3, bib4, bib;
	    static KEYSET aut = 0;
	    KEY ref, text, kk; OBJ Ref ;
	    int ix, j ;
	    static Stack buf = 0;

	    aut = arrayReCreate(aut,12, BSunit);
	    buf = stackReCreate(buf,50) ;

	    bib1 = query(ksNew,"?Paper") ; /* Class Paper members of s */
	    bib2 = query(ksNew,">Paper") ; /* Follow tag Paper */
	    bib3 = query(ksNew,">Reference") ; /* Follow tag Reference */
	    
	    bib4 = keySetOR(bib2,bib3) ;
	    bib  = keySetOR(bib1,bib4) ;
	    
	    keySetDestroy(bib1) ;
	    keySetDestroy(bib2) ;
	    keySetDestroy(bib3) ;
	    keySetDestroy(bib4) ;
	    
	    for(i=0 ; i < keySetMax(bib) ; i++)
	      { ref=keySet(bib,i) ;
		catText(s,name(ref)) ;
		
		if (Ref = bsCreate(ref))
		  {
		    if(bsGetKey (Ref,_Title,&text))
		      catText(s,name(text)) ;
		    stackClear(buf) ; catText (buf,"") ;
		    if (bsFindTag (Ref, _Author) &&
			bsFlatten (Ref, 1, aut))
		      {
			for (j=0;j<arrayMax(aut);j++)
			  { if(j)
			      catText(buf,", ") ;
			    catText(buf,name(keySet(aut,j))) ;
			  }
			catText(buf,".");
			catText(s,stackText(buf,0)) ;
		      }
		    stackClear(buf) ; catText(buf,"") ;
		    if (bsGetKey (Ref,_Journal,&kk))
		      catText (buf,name(kk));
		    if (bsGetData (Ref,_Volume,_Int,&ix))
		      { catText (buf, messprintf (" %d",ix)) ;
			if (bsGetData (Ref,_bsRight,_Text,&cp))
			  catText (buf, cp) ;
		      }
		    
		    if (bsGetData (Ref,_Page,_Int,&ix))
		      { catText (buf, messprintf (", %d",ix)) ;
			if (bsGetData (Ref,_bsRight,_Int,&ix))
			  catText (buf, messprintf ("-%d",ix)) ;
		      }
		    
		    if (bsGetData (Ref,_Year,_Int,&ix))
		      catText (buf, messprintf (" (%d)",ix)) ;
		    
		    catText(s, stackText(buf,0)) ;
		    bsDestroy(Ref);
		  }
	      }
	    keySetDestroy(bib) ;
	  }
	  break ;
	case 'w': /* write */
	  if (!ksNew || !keySetMax(ksNew))
	    break ;
	    
	  keySetDestroy(kA) ;
	  walking = -1; /* Just destroyed walk state -HJC */
	  kA = keySetAlphaHeap(ksNew, keySetMax(ksNew)) ;
	  for (j = 0 , i = 0 ; i < keySetMax(kA) ; ++i)
	    if (dumpKey (keySet(kA,i), 0, s))
	      j++ ;
	  	  	  
	  break ;
	
	case 'd': /* by Suzanna Lewis. LBL */
	  /* don't bother if nothing is there */
          if (!ksNew || !keySetMax(ksNew))
	      break ;

	  keySetDestroy(kA) ;
	  walking = -1; /* Just destroyed walk state -HJC */	  
	  kA = keySetAlphaHeap(ksNew, keySetMax(ksNew)) ;

          freenext() ;

	
	  /* closes the file */
	  if (cp = freeword()) {
	      if( f = fopen(cp, "w") ) {
		  i = dumpFastAFile (f, kA) ;
		  catText (s, messprintf("I wrote %d sequences", i)) ;
	      } else{
		  messout(messprintf("Could not open fastA file %s", cp));
	      }
	  } else {
	      /* No filename so stack into output */
	      dumpFastAStack(s, kA);
	  }
	  break ;
	  /* suz, end of dna dump addition */	    
	case 'p':
	  if(varname)
	  {
	      free(varname);
	      varname = 0;
	  }
	  f = 0;
          freenext() ;
	  if((cp = freeword()) && !strncmp("-v", cp, 2)) {
	      if(!(cp = freeword())) {
		  messout("\nusage: Acedb parse -var TclVar.");
		  break;
	      } else {
		  /* Got a Tcl variable name, I hope. */
		  if(!(varname = malloc(strlen(cp)+1)))
		      break;
		  strcpy(varname,cp);
		  if(! (cp = Tcl_GetVar(interp, varname, TCL_LEAVE_ERR_MSG))) {
		      messout("\nusage: Acedb parse -var TclVar.");
		      break;
		  }
	      }
	  } else {
	      f = (cp) ? ( f = fopen(cp, "r") ) : stdin ;
	  }
	  sessionWriteAccess() ; /* try to grab it */
	  if (!isWriteAccess())
	    {
		messout("\nSorry, you do not have write access.") ;
		if (f) fclose(f) ;
		break ;
	    }
	  
	  if (varname) {
	      if (!parseBuffer(cp, 0)) {
		  messout(messprintf("\nError in parse of %s",varname));
		  break;
	      }
	  }else if (f) { /* suz, corrected `if` structure */
	      parseFile(f, 0, 0); /* will close f */
	  }
	  else
	      messout(messprintf("Sorry, I could not open file %s", cp)) ;
	  sessionClose(TRUE) ; /* Does nothing if f==0 */
	  break ;
	  
	case 'T':
	  freenext() ;
	  cp = freeword() ;
	  if (!cp)
	    {
		messout("\nUsage Table command-file [separator (1<sep<256)]") ;
		break ;
	    }

	  f = fopen(cp, "r") ;
	  if (!f)
	    { 
	      messout(messprintf("Sorry, I could not open file %s", cp)) ;
	      break ;
	    }
	  freenext() ;
	  if (freeint(&i) && i > 0 && i < 256) ;
	  else
	    i = '\t' ;	  
	  
	  catText (s,"Table output:\n") ;
	  {
	    void *spread = spreadAlloc() ;
	    spreadDoReadDefinitions (spread, f, 0, "") ; /* will close f */
	    spreadDoRecompute (spread) ;
	    spreadDoDump (spread, i, s, 0) ;
	    spreadDoDestroy(spread) ;
	  }
	  break ;

	case 'f':   /* Start Walk */
	  /* look for the class name */
	  if (!ksNew || !keySetMax(ksNew))
	  {
	      messout("\nActive list is empty") ;
	      break ;
	  }
	  class = 0 ;
	  
	  if (ksNew && keySetMax(ksNew))
	  {
	      keySetDestroy(kA) ;
	      walking = -1; /* Just destroyed Walk state */
	      kA = keySetAlphaHeap(ksNew, keySetMax(ksNew)) ;
	      walking = 0; /* start taking the walk */
	  }
	  /* Note the fall through... -HJC*/

	case 'e': /* Provide next element */
	  if(-1 == walking)
	  {
	      messout("\nNot taking a walk through a keyset");
	      break;
	  }

	  if(!(cp = freewordcut("",&c)))
	  {
	      messout("\nNo variable given to place element in");
	      break;
	  }

	  Tcl_SetVar( interp, cp, "", TCL_LEAVE_ERR_MSG);

	  if(keySetMax(kA) == walking)
	  {
	      walking = -1; /* End of walk */
	      break;
	  }

	  dumpKey(keySet(kA,walking), 0, s) ;
	  
	  Tcl_SetVar( interp, cp, s->pos, TCL_LEAVE_ERR_MSG);
	  s = stackReCreate(s, 20000) ;
	  walking++; /* Next please */
	  break ;
	  
	default:
	  messout("\nOption not yet written, sorry") ;
	  break ;
	}
      else {
	  messout("\nPlease type ? for a list of commands.") ;
      }
    }

  reply_len = stackMark(s) ; 
 
  answer = messalloc(reply_len + 1) ;
  *(answer + reply_len) = 0 ;
  memcpy(answer, stackText(s, 0), reply_len) ;

  Tcl_SetResult(interp, answer, TCL_VOLATILE);
 
  messfree(answer);

  oldAnswer = stackText(s, 0) ;

  look->ksNew = ksNew ;
  look->ksOld = ksOld ;

  reply_len = stackMark(errorstack);
  if(reply_len)
  {
      answer = messalloc(reply_len + 1) ;
      *(answer + reply_len) = 0 ;
      memcpy(answer, stackText(errorstack, 0), reply_len) ;
      
      Tcl_SetErrorCode(interp,"ACEDB",answer, (char*) 0);

      messfree(answer);

      errorstack = stackReCreate(errorstack,128);

      successcode = TCL_ERROR;
  }
  return successcode;       
}

/**************************************************/
/********* ACEDB non graphic Query Server  ********/
/**************************************************/

void wormClose(void)
{
  if (isWriteAccess())
    sessionClose
     (freequery ("You did not save your work, should I ?")) ;
  
  exit(0) ;
}

/**************/

int Tcl_AppInit(Tcl_Interp *interp)
{
    char x ;
    stackorigin = &x ;
    int argc = 0 ;

    if(Tcl_Init(interp) == TCL_ERROR) {
	return TCL_ERROR;
    }
    tcl_RcFileName = "~/.acerc";

    sessionInit (&argc, 0) ;
    printf(" Database directory: %s\n", sessionFilName ("", 0, 0)) ;
    
    clients = arrayCreate(120, LOOK) ;
    
    errorstack = stackCreate(128);
    
    Tcl_CreateCommand (interp, "acedb", processTclQueries, 
		       (ClientData *) 0, (Tcl_CmdDeleteProc *) 0) ;
    return TCL_OK;
}

/**************************************************/
/**************************************************/

