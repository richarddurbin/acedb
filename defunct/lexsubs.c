/*  File: lexsubs.c
 *  Author: Jean Thierry-Mieg (mieg@mrc-lmb.cam.ac.uk)
 *  Copyright (C) J Thierry-Mieg and R Durbin, 1991
 *-------------------------------------------------------------------
 * This file is part of the ACEDB genome database package, written by
 * 	Richard Durbin (MRC LMB, UK) rd@mrc-lmb.cam.ac.uk, and
 *	Jean Thierry-Mieg (CRBM du CNRS, France) mieg@kaa.cnrs-mop.fr
 *
 * Description:
 * Exported functions:
 * HISTORY:
 * Last edited: Dec 13 22:00 1995 (mieg)
 * Last edited: Sep  1 20:14 1994 (mieg)
 * * Aug 29 20:07 1994 (mieg): _Tags are now defined as KEY variables
      their value is computed dynamically, ensuring consistency accross databases
 * * Jan 10 00:54 1992 (mieg): added lexAlias, eliminated lastmodif
 * * Dec 11 12:22 1991 (mieg): lexcleanup to remove spaces in addkey
 * * Nov  5 21:28 1991 (mieg): introduced the hash package
 * Created: Tue Nov  5 21:28:40 1991 (mieg)
 *-------------------------------------------------------------------
 */

/* $Id: lexsubs.c,v 1.33 1997-06-10 05:46:23 mieg Exp $ */

/***************************************************************/
/***************************************************************/
/**  File lexsubs.c :                                         **/
/**  Handles as Arrays  the lexique of the ACeDB program.     **/
/***************************************************************/
/***************************************************************/
/*                                                             */
/*  Many    routines are public :                              */
/*     avail,show, make/dump,  2define,                        */
/*     iskey,lock/unlock, iskeylocked,                         */
/*     KEY2LEX,key2word/word2key, addkey, kill,                */
/*     randomkey,lexNext and lexstrcmp.                        */
/*                                                             */
/*  There are 256 vocabularies, the first 128 are generic      */
/*  and common to all implementations of the base, the others  */
/*  can be handled freely by every user.                       */
/*                                                             */
/*  Voc[0] contains the flow control dialog, the others the    */
/*  names of the genes, alleles, bits of the map and other     */
/*  objects, and the keywords describing the phenotypes.       */
/*                                                             */
/*  Each word is coded by a key. A key is 4 bytes long, the    */
/*  first byte is the vocabulary number, thus, the maximum     */
/*  number of entry per voc. is 16M. Each key may then refer   */
/*  to a disk address (or 0 if the word is not the name of an  */
/*  object or a list), and to a cache control area if the      */
/*  object is loaded.                                          */
/*                                                             */
/*  Lexshow is for debugging.                                  */
/*  Lexmake is called on entering the session, it reads in the */
/*  lexiques from disk and return 0 if ok. LexSave writes      */
/*  them back to disk. Note that the lexique is not saved on   */
/*  every modification, therefore, if a crash occurs during    */
/*  a session, the lexiques must be reconstructed from the     */
/*  disk.                                                      */
/*                                                             */
/*  iskey returns 0 on unknown keys, 1 if the key is empty     */
/*  or 2 if an object is associated to this key.               */
/*  lock prevents the simultaneous updating by 2 processes     */
/*  it is invoked by BSlock_make                               */
/*                                                             */
/*  Name make a key into a word, and word2key a word    */
/*  into a key if the word is known.  Lexaddkey(w,k,t) adds    */
/*  a new word w and sets its key k in vocabulary t.           */
/*                                                             */
/*  Lexstrcmp is a case unsensitive strcmp.                    */
/*                                                             */
/***************************************************************/

#include <ctype.h>

#if !defined(ACEDB4)

#include "acedb.h"
#include "lex_bl_.h"
#include "keyset.h"
#include "a.h"
#include "bs.h"   /* recursive for lexalias/subclass checking */
#include "lex.h"
#include "sysclass.h"
#include "systags.h"
#include "key.h"
#include "disk.h"    /*call to diskprepare in lexinit*/

#include "pick.h"

#include "chrono.h"
#include "session.h"

#define MAXLEX   (1<<24 - 1)          /*number of entries per vocabulary <=FFFFFF*/
#define MAXVOCAB (1<<31 - 1)     /*number of char in a vocabulary<max(LEXOFFSET)*/
#define MAXTABLE 256             /*number of vocabulary tables <= 256*/
                                 /* watchout 256 meaning MAXTABLE used in coresubs */
                                 /* and also in parse.c , maybe elsewhere */
                                 /* and corresponds to the first byte inside KEY */
static Array  Lexi [MAXTABLE];
static KEYSET LexHashTable[MAXTABLE] ;
static int    nClass[MAXTABLE] ;  /* number of bits for hasher */
static Stack Voc  [MAXTABLE];
static BOOL  lexIsRead[MAXTABLE] ;
static char* nakedName(KEY key) ;  /* Does not follow aliases */
static int vocmodif[MAXTABLE];
static int lexhmodif[MAXTABLE];
static int leximodif[MAXTABLE];
static int lexSessionStartSize[MAXTABLE] ;

#define si ((U_Int)sizeof(LEXI))

static void lexvocread(int table) ;
static void lexvocwrite(int table);
static void lexiread(int table) ;
static void lexiwrite(int table);

static void lexhread(int table) ;
static void lexhwrite(int table);
static void lexdefine2voc0 (void) ;

extern void lexDefineSystemTags(void) ;   /* in whooks/sysclass.c */
extern void sysClassInit (void) ;
extern lexAlphaMark (KEY key) ;

static void lexHashInsert(int classe, KEY key) ;
 void lexReHashClass(int classe) ;
int lexstrCasecmp(char *a,char *b) ;

#define PRINT FALSE	/* set TRUE to show save progress */

/**************************************************************/

/**************************************************************/

void lexavail(unsigned long *vocnum,
	      unsigned long *lex1num,
	      unsigned long *lex2num,
	      unsigned long *vocspace, 
	      unsigned long *hspace,
	      unsigned long *totalspace
	      )
{
  register  int t = 256;
  unsigned long n = 0, v = 0, k1 = 0, k2 = 0, h = 0,tt = 0;
  while(t--)
    if(Lexi[t])
      { n++;
        v += (unsigned long) stackMark(Voc[t]) ;
        k1 += (unsigned long) arrayMax(Lexi[t]) ;
        h += arrayMax(LexHashTable[t]) ;
	tt += Voc[t]->a->dim +( Lexi[t]->dim)*(Lexi[t]->size)+(LexHashTable[t]->dim)*(LexHashTable[t]->size);
      }
  
  *vocspace=v; *vocnum=n; *lex1num=k1; *lex2num=k2; *hspace = h ; 
  *totalspace = tt/1024;
}

/*************************************************/

void lexInit(void)
{
  register int t = MAXTABLE ;

  while(t--)
    { vocmodif[t] = lexhmodif[t] = leximodif[t] = FALSE ;
      Voc[t] = 0 ; Lexi[t] = LexHashTable[t] = 0 ;
      nClass[t] = 0 ;
      lexIsRead[t] = FALSE ;
    }

 /*  diskprepare()  will be called implicitly on first disk access */

    /* Hard define the tags needed for sessionInit */
  lexDefineSystemTags() ;
    /* read the tag files, if they exist, for compatibility in case lex0
       has never been saved
     */
  if (filName ("wspec/tags", "wrm", "r"))
    lexdefine2voc0 () ;

  lexIsRead[0] = lexIsRead[1] = TRUE ;
  lexReHashClass(0) ;
  lexReHashClass(1) ;
}

/*************************************************/
  /* To be used only from the session manager */
void lexClear(void)
{
  register int t ;
  
  for(t=3;t< MAXTABLE; t++)
    { vocmodif[t] = lexhmodif[t] = leximodif[t] = FALSE ;
      stackDestroy(Voc[t]) ;
      arrayDestroy(Lexi[t]) ;
      keySetDestroy(LexHashTable[t]) ;
      nClass[t] = 0 ;
      lexIsRead[t] = FALSE ;
    }
}

/*************************************************/

static void lexReadTable(int t)
{
  if (lexIsRead[t])
    return ;
  lexIsRead[t] = TRUE ;

  pickList[3].type = 'A' ;  /* needed during bootstrap */
  if (!t || pickList[t].name)
    { lexvocread(t) ;
      if(Voc[t])  /* could be 0 if class is new */
	{ mainActivity(
	    messprintf("Reading in class %s",
		       pickClass2Word(t))) ;
	  lexiread(t) ;
	  lexhread(t) ;  
	  lexSessionStartSize[t] = arrayMax(Lexi[t]) ;
	  lexClearClassStatus (t, TOUCHSTATUS) ;
	}
      else  /* Reinitialise this lexique */
	{ KEY key ;
	  arrayDestroy(Lexi[t]) ;
	  keySetDestroy(LexHashTable[t]) ;
	  nClass[t] = 0 ;
	  lexSessionStartSize[t] = 0 ;
	  lexaddkey(0,&key,t) ;
	}
    }
}

/*************************************************/

void lexRead (void)
{ KEY key ;
     /* table 0 is the tags, created in lexDefineTags() and dynamically
	table 1 is created by hand
	table 2 is the session chooser
	table 3 is the Voc lexique, it MUST be reinitialised
	table 4 is the BAT chooser
     */

  if (lexword2key ("_voc3", &key, _VGlobal) && iskeyold(key))
    { lexIsRead[_VVoc] = FALSE ; 
      lexReadTable(_VVoc) ;      
    }

  lexReadTable(_VBat) ;

  lexIsRead[_VDisplay] = FALSE ;
  lexIsRead[_VClass] = FALSE ;

/* We now recover the correct values of the tags */
  if (lexword2key ("_voc0", &key, _VVoc) && iskeyold(key))
    { lexIsRead[0] = FALSE ; /* we recover the tags of the previous session */
      lexReadTable(0) ;      
    }
  else if (!filName ("wspec/tags", "wrm", "r")) /* read during lexInit */
    messcrash ("Can't find file wspec/tags.wrm.  It is needed because the "
	       "last session was created by code (%d.%d) prior to 3.4",
	       thisSession.mainCodeRelease, thisSession.subCodeRelease) ;
  lexDefineSystemTags () ; /* Redefine the system tags, which have been overwritten */
  tagInit () ;  /* finally, initialises correctly the _tag variables */
  
/* We now recover the correct values of the classes */
  if (lexword2key (messprintf ("_voc%d", _VMainClasses), &key, _VVoc) &&
      iskeyold(key))
    { lexIsRead[_VMainClasses] = FALSE ; /* we recover the tags of the previous session */
      lexReadTable(_VMainClasses) ;      
    }
  else if (!filName ("wspec/classes", "wrm", "r")) /* read during pickInit */
    messcrash ("Can't find file wspec/classes.wrm. It is needed because the "
	       "last session was created by code (%d.%d) prior to 3.4",
	       thisSession.mainCodeRelease, thisSession.subCodeRelease) ;
/* pickInit n a pas defini les classes hard defined */

  sysClassInit () ; /* We redefine the system classes which may have been lost */
  classInit () ;  /* finally, initialises correctly the _VClass variables */
  if (lexword2key (messprintf ("_voc%d", _VModel), &key, _VVoc) &&
      iskeyold(key))
    { lexIsRead[_VModel] = FALSE ; /* we recover the tags of the previous session */
      lexReadTable (_VModel) ;
    }
}

/*************************************************/

void lexReadGlobalTables(void)
{ lexIsRead[1] = lexIsRead[0] = TRUE ;
  lexiread(1) ;
  lexReHashClass(1) ;

  diskPrepare() ;  /* Reads the Global Bat */
  lexReadTable(2) ;
}

/**************************************************************/
void lexmark(int tt)    /*enables blocksubs to modify leximodif
                        * when changing a disk address
                        * note that the memory address is never
                        * saved to disk, nor the locks
                        * and that only lexaddkey and  lexRename
			* touch lexh and voc
                        */
{
  leximodif[tt] = TRUE ; 
}

/******************************************************************/
      /*writes the modified lexiques to disk*/
      /*public ; called by saveAll() and dosomethingelse()*/

void  lexSave(void) 
{ int t = MAXTABLE ;
  extern lexAlphaSaveAll (void) ;

  lexAlphaSaveAll () ;
  while(t--)  /* DO save lex[0]. tags.wrm only read for compatibility if lex0 is missing */
    {
      if(t==1)
	lexiwrite(t);  /*t = 1 Global voc */
      else 
	if(Lexi[t])
	  {
	    lexvocwrite(t);
	    lexiwrite(t);
	    lexhwrite(t);
	  }
    }
}

/********************************************/

static void lexvocread(int t)
{ KEY key ;
  char buffer[24] ;
  sprintf(buffer, "_voc%d", t) ;
  lexaddkey (buffer,&key, !t || t>_VVoc ? _VVoc : _VGlobal ) ;
  lexaddkey (buffer,&key, !t || t>_VVoc ? _VVoc : _VGlobal ) ;
  stackDestroy(Voc[t]) ;
  Voc[t] = stackGet(key) ;
  vocmodif[t] = FALSE ;
}

/**************************************************************/

static void lexvocwrite(int t)
{
  KEY key ;

  if (!isWriteAccess() || !vocmodif[t])
    return ;

  if(PRINT)
    printf("\n   writingVoc %d",t);
  
  lexaddkey(messprintf("_voc%d",t),&key,!t || t>_VVoc ? _VVoc : _VGlobal ) ;
  lexaddkey(messprintf("_voc%d",t),&key,!t || t>_VVoc ? _VVoc : _VGlobal ) ;
  stackStore(key,Voc[t]) ;
  
  vocmodif [t] = FALSE ;
}

/**************************************************************/

static void lexhread(int t)
{
  KEY key ;

  lexaddkey(messprintf("_lexh%d",t),&key,!t || t>_VVoc ? _VVoc : _VGlobal ) ;
  lexaddkey(messprintf("_lexh%d",t),&key,!t || t>_VVoc ? _VVoc : _VGlobal ) ;
  keySetDestroy(LexHashTable[t]) ;
  LexHashTable[t] = arrayGet(key, KEY,"k") ;
  if (!LexHashTable[t])
    lexReHashClass(t) ;
  else
    { int n = arrayMax(LexHashTable[t]) , nBits = 0 ;
      while ((1 << ++nBits) < n) ;
      if(n != (1<<nBits))
	messcrash("Wrong size in lexhread %d", t) ;
      nClass[t] = nBits ;
    }

  lexhmodif [t] = FALSE ;
}

/**************************************************************/

static void lexhwrite(int t)
{
 /*  Commented out since i want to change the hashText 
     routine till it is really good

 KEY key ;
  
 
  if (!isWriteAccess())
    return ;

  if(!lexhmodif[t])
    return ;
  if(PRINT)
    printf("\n   writinglexh %d",t);

  lexaddkey(messprintf("_lexh%d",t),&key,!t || t>_VVoc ? _VVoc : _VGlobal ) ;
  lexaddkey(messprintf("_lexh%d",t),&key,!t || t>_VVoc ? _VVoc : _VGlobal ) ;
  arrayStore(key,LexHashTable[t]) ;
*/  
  lexhmodif[t] = FALSE ;
  return ;
}

/**************************************************************/

static void lexiread(int t)
{
  KEY key ;
  register LEXI* lxi ;
  register int j ;
  Array dummy ;
  
  lexaddkey(messprintf("_lexi%d",t),&key,!t || t>_VVoc ? _VVoc : _VGlobal ) ;
  lexaddkey(messprintf("_lexi%d",t),&key,!t || t>_VVoc ? _VVoc : _VGlobal ) ;
  dummy = arrayGet(key, LEXI,lexiFormat) ;
  arrayDestroy(Lexi[t]) ;
  Lexi[t] = dummy ;
  if (!Lexi[t])
    messcrash("Lexi[%d] not found",t);

  lxi=arrp(Lexi[t], 0, LEXI);
  j = arrayMax(Lexi[t]);
  while(j--)
    { lxi   ->addr=(ALLOCP)NULL;
      lxi   ->cache =(void *)NULL;
      lxi   ->lock &= ~LOCKSTATUS ; /* NOT 3, to zero the last 2 bits */
      lxi++;
    }
  leximodif[t] = FALSE ;
}

/**************************************************************/
static void lexiwrite(int t)
{ 
  KEY key ;

  if (!isWriteAccess() || !leximodif[t])
    return ;

  lexaddkey(messprintf("_lexi%d",t),&key,!t || t>_VVoc ? _VVoc : _VGlobal ) ;
  lexaddkey(messprintf("_lexi%d",t),&key,!t || t>_VVoc ? _VVoc : _VGlobal ) ;

  if(PRINT)
    printf("\n   writinglexi %3d,  %8d  entries.",
	   t, arrayMax(Lexi[t]));  
  leximodif[t] = FALSE ;  /* must come before the write */

  if(PRINT)
    printf("address %d.", KEY2LEX(key)->dlx.dk);  
  arrayStore(key,Lexi[t],lexiFormat) ;
}

/******************************************************************/

void lexOverLoad(KEY key,DISK disk) 
{  KEY2LEX(key)->dlx.dk = disk ;
   KEY2LEX(key)->cache = 0 ;
   KEY2LEX(key)->addr = 0 ;
}

/******************************************************************/

DISK lexDisk(KEY key)
{  return KEY2LEX(key)->dlx.dk ;
}

/******************************************************************/
   /* Remove leadind and trailing spaces and non ascii */
static char *lexcleanup(char *cp)
{  char *cq = cp ;

   while(*cp == ' ')
     cp++ ;
   cq = cp + strlen(cp) ;
   while(cq > cp)
     if (*--cq == ' ')
       *cq = 0 ;
   else
     break ;
 /*  may be we should also have this
#include <ctype.h>
  cq = cp - 1 ;
   while(*++cq)
     if (!isascii(*cq))   or may be isgraph ?
       { *cq = 0 ;
	 break ;
       }
 */  
   return cp ;
}

/******************************************************************/

void lexHardDefineKey (int table, KEY key,  char *cp) 
{ int i ;
  KEY  key2 ;
  char *cq ;

  if(lexword2key(cp,&key2,table))
    if (key == KEYKEY(key2))
      return ;
    else
      messcrash
	("The old %d.%d and new %d values of key %s differ.",
	 table,KEYKEY(key2), key, cp) ;

  i = lexMax(table) ;
  if(!i) i = 1 ; /* Because a zeroth key is created by lexaddkey */

  if (!key) 
    key = i ;
  if(i < key)
    { for(;i < key; i++)
	lexaddkey(messprintf("__sys%d",i),&key2,table) ;
      lexaddkey(cp,&key,table);
    }
  else if(i == key)
    lexaddkey(cp,&key,table);
  else
    { cq = name(KEYMAKE(table,key)) ;
      if(strcmp(cq,messprintf("__sys%d",key)))
	messcrash("Tag %s = %d tries to overwrite tag %s",
		  cp, key, cq) ;
      /* Else I am overwriting a dummy tag */
      array(Lexi[table], (int) key, LEXI).nameoffset 
	= stackMark(Voc[table]) ;
      pushText(Voc[table], lexcleanup(cp) ) ;
      array(Lexi[table], (int) key, LEXI).nameoffset 
	= stackMark(Voc[table]) ;
      pushText(Voc[table], lexcleanup(cp) ) ;
      
      leximodif[table] = lexhmodif[table] = vocmodif[table] = TRUE ;
      lexAlphaMark (table) ;
    }
}

/******************************************************************/

static void lexdefine2voc0 (void)
{
  FILE *myf ;
  KEY  key, key2 ;
  int  table=0, i ;
  int  line = 0 ;
  char *cp, cutter=' ', tag[80] ;
     /* in our new system, this is only useful to convert an old database
	to the new system
     */

  if (!(myf = filopen ("wspec/tags", "wrm" ,"r")))
    return ;
  freeread (myf) ;		/* to skip the file header */

  while(freeread(myf))
    { line++ ;
      cp=freewordcut(" ",&cutter);        /* get #define */
      
      if (!cp || strcmp(cp,"#define"))
	continue;

      cp = freewordcut(" ",&cutter) ;
      if(!cp || *cp != '_')
	messcrash("Tag %s should start by an underscore",
		  cp ? cp : "No tag") ;
      cp++ ;

      if(strlen(cp)>75)
	messcrash("Tag %s is too long",cp) ;
      strcpy (tag, cp) ;

      if (!freeint(&i))
	messcrash ("The value of tag %s is missing in wspec/tags.wrm") ;
      key = i ;

      if(lexword2key(tag,&key2,table) && key != key2)
	messcrash
	  ("The old %d and new %d values of tag %s differ.",
	   key2, key, tag) ;
      i = lexMax(table) ;
      if(!i) i = 1 ; /* Because a zeroth key is created by lexaddkey */
      i = KEYMAKE(table,i) ;
      if(i < key)
	{ for(;i < key; i++)
	    lexaddkey(messprintf("__sys%d",i),&key2,table) ;
	  lexaddkey(tag,&key,table);
	}
      else if(i == key)
	lexaddkey(tag,&key,table);
      else
	{ cp = name(KEYMAKE(key,table)) ;
	  if(strcmp(cp,messprintf("__sys%d",key)))
	    messcrash("Tag %s = %d tries to overwrite tag %s",
		      tag, key, cp) ;
	  /* Else I am overwriting a dummy tag */
	  array(Lexi[0], (int) key, LEXI).nameoffset 
	    = stackMark(Voc[0]) ;
	  pushText(Voc[0], lexcleanup(tag) ) ;

	  leximodif[0] = lexhmodif[0] = vocmodif[0] = TRUE ;
	  lexAlphaMark (0) ;
	}
    }
  filclose(myf);
}

/********************************************/
static LEXP myKey2Lex = 0 ;
                      /* Returns 0 if kk is unknown,
                       * 1 if kk is just a vocabulary entry
                       * 2 if kk corresponds to a full object
		       *
		       * sets myKey2Lex
                       */
int isNakedKey(KEY kk)
{
  KEY k = KEYKEY(kk);
  int t = class(kk);
  if (!kk || (k >= lexMax(t)) )
    return 0 ;

  myKey2Lex = arrp(Lexi[t], k, LEXI);

  return
    ( myKey2Lex->cache || 
     myKey2Lex->addr || 
     myKey2Lex->dlx.dk)  ?  2  :  1  ;
}

/***************************************************/
/* returns true is key has aliasstatus or emptystatus, */
/* cannot use lexGetStatus for this as it follows the alias list */
BOOL lexIsKeyInvisible(KEY key)
{  
  return isNakedKey(key) && (myKey2Lex->lock & (ALIASSTATUS | EMPTYSTATUS));
}

/********************************************/
   /* prevent the formation of alias loops */
BOOL lexAliasLoop(KEY old, KEY new)
{
  KEY kk = new ;
  int result ;
  
  while (TRUE)
    { result = isNakedKey(kk) ;
      if (!result ||
	  ! (myKey2Lex->lock & ALIASSTATUS))
	break ;
      kk = (KEY) myKey2Lex->dlx.dk ;
      if (kk == old)
	 /* to alias old to new would create a loop */
	return TRUE ;
    }
  return FALSE ;
}

/********************************************/
   /* Scans the alias list for occurence of newName */
static BOOL lexAliasExists(KEY key, char* newName)
{
  KEY kk = key ;
  char *name ;
  BOOL isCaseSensitive = pickList[class(key) & 255 ].isCaseSensitive ;
/*   int (*lexstrIsCasecmp)(const char *a,const char *b) = isCaseSensitive ? */
/*    lexstrCasecmp : lexstrcmp ; */
  int (*lexstrIsCasecmp)() = isCaseSensitive ?
    strcmp : strcasecmp ;
 
  while (TRUE)
    { name = nakedName(kk) ;  /* sets myKey2lex */
      if (!lexstrIsCasecmp(name, newName)) /* Equivalence  */
	return TRUE ;
      if (!(myKey2Lex->lock & ALIASSTATUS))
      	return FALSE ;
      kk = (KEY) myKey2Lex->dlx.dk ;
    }
}

/********************************************/
   /* Scans the alias list for valid object, used by bsCreate etc  */
KEY lexAliasOf (KEY key)
{
  while (TRUE)
    {
      if (!isNakedKey(key) ||                     /* sets myKey2lex */
	  ! (myKey2Lex->lock & ALIASSTATUS))
	return key ;
      key = (KEY) myKey2Lex->dlx.dk ;
    }
}

/********************************************/
   /* Iterates through the alias list */
int iskey(KEY kk)
{
  KEY start = kk ;
  int result ;
 
  while (TRUE)
    { result = isNakedKey(kk) ;
      if (!result ||
	  ! (myKey2Lex->lock & ALIASSTATUS))
	break ;
      kk = (KEY) myKey2Lex->dlx.dk ;
      if (kk == start)
	messcrash ("Loop in alias list of Key %d : %s",
		   kk, nakedName(kk) ) ;
    }
  return result ;
}

/********************************************/

                      /* Returns TRUE
                       * if kk has a primary cache or a disk address
                       */
BOOL iskeyold(KEY kk)
{
  return
    iskey(kk) ?
      (  (myKey2Lex->addr || myKey2Lex->dlx.dk) ? TRUE : FALSE ) :
	FALSE ;
}
/********************************************/

LEXP KEY2LEX(KEY kk)
{
 return
   iskey(kk) ?  myKey2Lex : 0 ;
}

/********************************************/

BOOL lexlock(KEY key)
{      /* code partially  duplicated in cacheLock*/
 LEXP q=KEY2LEX(key);
  if(q)
    {
      if(q->lock & LOCKSTATUS)
	messerror("Double locking  of %s",
		  name(key));
      else
	{
	  q->lock |= LOCKSTATUS; return TRUE;
	}
    }
  return FALSE;
}

/********************************************/

void lexunlock(KEY key)
{
 LEXP q=KEY2LEX(key);
  if(   q
     && (q->lock & LOCKSTATUS) )
    q->lock &= ~LOCKSTATUS; 
  else
    messcrash("Unbalanced unlock of %s",
	      name(key));
}

/********************************************/
   /* subClass mechanism */
void lexSetIsA(KEY key, unsigned char c) 
{     
  LEXP q=KEY2LEX(key);
  if(q)
    q->isMask = c ;
}

/********************************************/
   /* subClass mechanism: test appartenance */
BOOL lexIsInClass(KEY key, KEY classe)
{ unsigned char mask ;   
  LEXP q=KEY2LEX(key);

  if (classe < 256)
    return classe == class(key) ;

  if (!q)
    return FALSE ;

  pickIsA(&classe, &mask) ;
  return 
    classe == class(key)  &&
      (mask & q->isMask) == mask ;
}

/********************************************/
  /* alteration of LEXPRIVATESTATUS flags would 
     foul the lex system */
  /* code duplicated in cacheUpdate*/
void lexSetStatus(KEY key, unsigned char c) 
{     
  LEXP q=KEY2LEX(key);
  if(q)
      q->lock |= (c & ~LEXPRIVATESTATUS) ;
   
  else
    messcrash("lexUnsetStatus called with bad key %s",
	      name(key));
}

/********************************************/
  
  /* code duplicated in cacheUpdate*/
void lexUnsetStatus(KEY key, unsigned char c) 
{     
  LEXP q=KEY2LEX(key);
  if(q)
      q->lock &= (~c) | LEXPRIVATESTATUS ;
   
  else
    messcrash("lexUnsetStatus called with bad key %s",
	      name(key));
}

/********************************************/
  
void lexClearClassStatus(KEY classe, unsigned char c) 
{ int i ;        
  LEXP q ;
 
  c = (~c) | LEXPRIVATESTATUS ;
  if (i = lexMax(classe))
    { q = arrp(Lexi[classe], 0, LEXI) - 1 ;
      while (q++, i--)
	q->lock &= c ;
    }
}

/********************************************/

unsigned char lexGetStatus(KEY key)
{ 
  LEXP q=KEY2LEX(key);
  if(q)
    return q->lock ;
  else
    messcrash("lexGetStatus called with bad key %s",
	      name(key));
  return 0 ;  /* for compiler happiness */
}

/********************************************/
   /* Avoid the alias system, used by word2key and hasher */
static char * nakedName(KEY kk)  
{     
 return
   isNakedKey(kk)                ?
    stackText(Voc[class(kk)],
	myKey2Lex->nameoffset)
                            :
    "\177\177(NULL KEY)"        ;     
}

/********************************************/

char * name(KEY kk)  
{     
 return
   iskey(kk)                ?
    stackText(Voc[class(kk)],
	myKey2Lex->nameoffset)
                            :
    "\177\177(NULL KEY)"        ;     
}

/********************************************/
  /* Iterates through the alias list */
BOOL nextName(KEY key, char **cpp)  
{ static LEXP lxp = 0 ;
  static KEY lastKey = 0 , kk ;
  
  if ( *cpp && key != lastKey)
    messcrash ("nextName %s called out of context", nakedName(key)) ;
 
  if (!*cpp)    /* initialise */
    kk = lastKey = key ;
  else         /* loop */
    { if (!lxp || !(lxp->lock & ALIASSTATUS)) /* shift once */
	return FALSE ;
      kk = (KEY) lxp->dlx.dk ;
    }

  while (TRUE)
    { if (!isNakedKey(kk))
	return FALSE ;
      if (!(myKey2Lex->lock & EMPTYSTATUS))   /* if empty */
	break ;
      if (!(myKey2Lex->lock & ALIASSTATUS))  /* loop again */
	return FALSE ;
      kk = (KEY) myKey2Lex->dlx.dk ;
    }
  lxp = myKey2Lex ;                         /* used by shift once */
  *cpp = nakedName(kk) ;
  return TRUE ;
}

/********************************************/

BOOL  lexReClass(KEY key,KEY *kp, int t)
{ return 
    lexword2key(name(key),kp,t) ;
}

/**********************************************/

static BOOL colonParse (char* note, char **classe, char **item)
{
  char *cp ;

  for (cp = note ; *cp != ':' ; ++cp) 
    if (!*cp)
      return FALSE ;

  *cp = 0 ;
  *classe = note ;
  *item = ++cp ;
  return TRUE ;
}

BOOL lexClassKey(char *text, KEY *kp)
{ int classe ;
  char *cName, *kName ;
  *kp = 0 ;
  return
    colonParse (text, &cName, &kName) &&
      (classe = pickWord2Class (cName)) &&
	(*(kName - 1) = ':') &&     /* Restore text */
	lexword2key (kName, kp, classe) ;
}

/********************************************/
/********************************************/
/************ Hash Package ******************/

                     /* Hashing system */
#define HASH_RATE 1.6
#define  SIZEOFINT (8 * sizeof (int))

unsigned int hashText(char *cp, int n, BOOL isOdd)
{
  unsigned int 
    i, h = 0 , j = 0,
    rotate = (isOdd ? 21 : 13) ,
    leftover = SIZEOFINT - rotate ;

  chrono("hashText") ;  
  while(*cp)
    { h = freeupper(*cp++) ^  /* XOR*/
	( ( h >> leftover ) | (h << rotate)) ; 
    }
  /* compress down to n bits */
  for(j = h, i = n ; i< SIZEOFINT; i+=n)
    j ^= (h>>i) ;
  chronoReturn() ;

  if (isOdd) /* return odd number */
    j |= 0x1 ;
  return
     j &  ( (1<<n) - 1  )  ;
}

/***************************************************/

 void lexReHashClass(int classe)
{
  KEY key = 0 ; int n ;

  chrono("lexReHashClass") ;

  n = 7 ;  /* chose size */
  while((1 << ++n) <= HASH_RATE * lexMax(classe)) ;
  nClass[classe] = n ;
  
 
  LexHashTable[classe] = arrayReCreate(LexHashTable[classe], 1<<n, KEY) ;
  keySet(LexHashTable[classe], ( 1 << n ) - 1) = 0 ; /* make room */

  key = arrayMax(Lexi[classe]) ;
  while (key--)
    lexHashInsert(classe, key) ;
  chronoReturn() ;
}

/****************************************************/
   /* should only be called if !lexword2key */
   /* key must be just the KEYKEY part */
int NII  , NIC  , NWC  , NWF  , NWG  ;
static void lexHashInsert(int classe, KEY key)
{
  static KEYSET hashTable ;
  unsigned int h, dh = 0 ;
  int max, n , nn ; 
  KEY *kp, *kpMin, *kpMax ;
  char *cp ;

  chrono("lexHashInsert") ;
  hashTable  = LexHashTable[classe] ;
  if(arrayMax(Lexi[classe]) * HASH_RATE > keySetMax(hashTable))
    lexReHashClass(classe) ;

  key = KEYKEY (key) ;
  cp = nakedName(KEYMAKE(classe,key)) ;
  if (!*cp)
    return ;  /* may occur after a rename */
  h = hashText(cp, nClass [classe], FALSE) ;

  nn = nClass[classe] ;
  n = max = 1 << nn ;

  kp = arrp(hashTable, h , KEY) ;
  kpMin = arrp(hashTable, 0 , KEY) ;
  kpMax = arrp(hashTable, n -1, KEY) ;
 
  while ( n--)  /* circulate thru table, starting at h */
    { if(!*kp)      /* found empty slot, do insert */
	{ *kp = key ;
	  NII++ ;
	  chronoReturn() ;
	  return ;
	}
        /* circulate */
      if (!dh)
	dh = hashText(cp, nClass [classe], TRUE) ;

      kp += dh ;
      if (kp > kpMax)
	    kp -= max ;
      NIC++ ;
    }
  messcrash("Hash table full, NIC = %d, lexhashInsert is bugged", NIC) ;
}

/*
  I comment out this function 
  it needs plot so prevents acequery froom linking

  Note that i am not sure that it coincides with the 
  current strategy in hashInsert and word2key

#include "plot.h"
void lexHashTest(int classe)
{
  static KEYSET hashTable ;
  unsigned int h ;
  int n, nn , nn1 ;
  KEY *kp, *kpMin, *kpMax ;
  int empty = 0 , full = 0 , j ;
  Array histo = arrayCreate(200,int) ;
  char *cp ;

  hashTable  = LexHashTable[classe] ;
  if(!hashTable || !arrayMax(hashTable))
    {  messerror("empty table\n") ;
       return ;
     }
  
  kpMin = arrp(hashTable, 0 , KEY) ;
  kpMax = arrp(hashTable, n -1, KEY) ;
  nn = nClass[classe] ;
  nn1 = n = arrayMax(hashTable) ;
  for(h=0; h <nn; h++)
    { 
      kp = arrp(hashTable, h , KEY) ;
      j = 0 ;
      if (!*kp)
	empty++ ;
      else
	{ full++ ;
	  cp = nakedName(KEYMAKE(classe, *kp)) ;
	  n = nn ;
	  while ( n--)  
	    { if(!*kp)      
		{
		 array(histo,j,int)++ ;
		 break ;
		}

	     // circulate 
	      if (!dh)
		dh = hashText(cp, nClass [classe], TRUE) ;
	      kp += dh ;
	      if (kp > kpMax)
		kp -= max ;
	      j++ ;
	    }
	}
    }
  plotHisto
    (messprintf("Hash correl %s", pickClass2Word(classe)), histo) ;
}
*/

/********************************************/

static void lexHardRename(KEY key, char *newName)
{
  char 
    *cp, *oldName = nakedName(key) ;  /* sets myKey2Lex */
  int classe = class(key) , n = strlen(oldName) ;

  for (cp = oldName ; *cp ;)
    *cp++ = 0 ;  /* will be left out of hash on next rehashing */
  
  if (strlen(newName) > n )
    { myKey2Lex->nameoffset = stackMark(Voc[classe]) ;
      leximodif[classe] = TRUE ;	/* since nameoffset is changed */
      pushText (Voc[classe], newName) ;
    }
  else
    strcpy (oldName, newName) ;
    
  if (myKey2Lex->lock & EMPTYSTATUS)  /* because the new name should appear in lists */
    { myKey2Lex->lock &= ~EMPTYSTATUS ;
      leximodif[class(key)] = TRUE ;
    }
  lexHashInsert(classe, key) ; /* re-insert in hash table */

  vocmodif[classe] = TRUE ;
  lexAlphaMark (classe) ;
}
  
/********************************************/

static void lexMakeAlias(KEY key, KEY alias, BOOL keepOldName)
{
   isNakedKey(key) ;
  
  myKey2Lex->dlx.alias = alias ; /* overloading diskaddr with alias key */
  myKey2Lex->lock = ALIASSTATUS ;  
  if (!keepOldName)
    myKey2Lex->lock |= EMPTYSTATUS ;  
  myKey2Lex->addr = 0 ;
  myKey2Lex->cache  = 0 ;
  isNakedKey(alias) ;
  myKey2Lex->lock &= ~EMPTYSTATUS ;
  leximodif[class(key)] = TRUE ;
  lexAlphaMark (class(key)) ;
}
 

/********************************************/
  /* needed if filter is name dependant */
static void lexAliasSubClass (KEY key)
{ OBJ obj ;
  Array a ;
  unsigned char mask ;

  if (pickType (key) != 'B') return ;
  obj = bsCreate (key) ;
  if (!obj) return ;
/* set subclass lex bits */
  a = pickList[class(key)].conditions ;
  
  if (arrayExists(a))
    { mask = queryIsA (obj, key, a) ;
      lexSetIsA (key, mask) ;
    }
  bsDestroy (obj) ;
}


/********************************************/
  /* Returns TRUE if newName is accepted */
  /* if keepOldName, the 2 names will be used in the future,
   * so we need 2 keys anyway,
   * If not, we would rather keep a single key except if the old 
   * and new one allready exist because they may be referenced from
   * elsewhere.
   */

BOOL lexAlias(KEY key, char* newName, BOOL doAsk, BOOL keepOldName)
{
  char *oldName = name(key) ;
  int  classe = class(key) ;
  KEY  newKey ;
  extern BOOL bsFuseObjects(KEY key, KEY new) ;
  BOOL isCaseSensitive = pickList[classe & 255 ].isCaseSensitive ;
/*   int (*lexstrIsCasecmp)(const char *a,const char *b) = isCaseSensitive ? */
/*    lexstrCasecmp : lexstrcmp ; */
  int (*lexstrIsCasecmp)() = isCaseSensitive ?
    strcmp : strcasecmp ;


           /********** Protections *************/ 

/*** rd do not protect isWriteAcess() here - like lexaddkey()
     used by internal code e.g. picksubs.c reading options.wrm 
***/

  if (iskey(key))
    myKey2Lex->lock &= ~EMPTYSTATUS ; /* do not discard */

  if (!isNakedKey(key))
    { messerror ("lex Rename called on unknown key %d", key) ;
      return FALSE ;
    }

      /************* Trivial cases *************/

  newName = lexcleanup(newName) ;

  if (!strcmp(oldName, newName)) /* Identity */
    { myKey2Lex->lock &= ~EMPTYSTATUS ; /* do not discard */
      return TRUE ;
    }

  if (!lexstrIsCasecmp(oldName, newName)) /* Equivalence and certainly same length */
    { strcpy (oldName, newName) ;
      vocmodif[classe] = TRUE ;
      goto ok ;
    }

     /************* Hard Rename to unknown name case *************/

  if (!keepOldName
      && !lexword2key (newName, &newKey, classe)) 
    { lexHardRename(key, newName) ;
      lexAliasSubClass (key) ;
      return TRUE ;
    }

     /*** forbid complex alias graphs ******/

  if (name(key) != nakedName(key))
    key = lexAliasOf(key) ;

     /************ Else I want the 2 keys defined  ************/ 

  if (lexiskeylocked(key))
    { messout("Sorry, alias fails because %s is locked elsewhere",
	      name(key)) ;
      return FALSE ;
    }

  lexaddkey(newName, &newKey, classe) ;

  if (lexiskeylocked(newKey))
    { if (doAsk)
	messout("Sorry, alias fails because %s is locked elsewhere",
	      name(newKey)) ;
      return FALSE ;
    }
  
  if(lexAliasLoop(key, newKey))  /* No loops */
    { if (doAsk)
	messout("Sorry, aliassing %s to %s would create a loop.",
		oldName, newName) ;
      return FALSE ;
    }

    if ( isNakedKey(key) == 1 )                /* key is actually empty */
     lexMakeAlias(key, newKey, keepOldName) ;
       
    else     /*  isNakedkey(key) == 2 */
     if (isNakedKey(newKey) == 1 )             /* key is full but newkey is empty */
       {                                  /* Just shift things over */
	 LEXP
	   newLex = KEY2LEX(newKey) ,
	   lex = KEY2LEX(key) ;
	 LEXOFFST offset = lex->nameoffset ;
	 
	 lex->nameoffset = newLex->nameoffset ;
	 newLex->nameoffset = offset ;
	 
	 lexMakeAlias(newKey, key, keepOldName) ;
       }
     else  /* also  isNakedKey(newKey) == 2 */
       {
	 if (pickType(key) != 'B')
	   { if (doAsk)
	       messout (" I don t know how to Fuse non TREE objects, sorry") ;
	     return FALSE ;
	   }
	 if (doAsk)
	   { if(!messQuery
		(messprintf("%s and %s both exist.\n"
			    "In case of conflict i will rather keep the data of %s.\n\n"
			    "Do you want to procees",
			    name(key), name(newKey), name(newKey))))
	       return FALSE ;
	   }
	 if (!bsFuseObjects (key, newKey))
	   return FALSE ;
	 lexMakeAlias(key, newKey, keepOldName) ;
       }
  
  if (!keepOldName)              /* Clean up */
    { char *cp = nakedName(key) ;
      while (*cp) *cp++ = 0 ;
      myKey2Lex->lock |= EMPTYSTATUS ;
    }
  
 ok:
  lexHashInsert(classe, key) ; /* re-insert in hash table */
  				/* set subclass lex bits */
  if (pickType(key) == 'B')
    { Array a = pickList[class(key)].conditions ;
      unsigned char mask ; 
      if (arrayExists(a))
	{ mask = queryIsA (0, key, a) ;
	  lexSetIsA (key, mask) ;
	}
    }

  return TRUE ;
}
  
/********************************************/
/********************************************/
static KEY *KP ;
BOOL lexword2key(char *cp,KEY *key,KEY classe)
                                    /* Case unsensitive  */
                            /*given a word *cp, sets its key in *ip */
                            /*search done only in the t lexique */
                            /*returns TRUE if found, FALSE if not */
                            /* No completion performed */
{
  unsigned int h, dh = 0 ;
  int max, n , nn ;
  KEY *kp, *kpMin, *kpMax ;
  KEYSET hashTable ;
  int t ; unsigned char mask ;
  LEXI *lexi ;
  char *voc ;
  BOOL isCaseSensitive = pickList[classe & 255 ].isCaseSensitive ;
/*  int (*lexstrIsCasecmp)(const char *a,const char *b) = isCaseSensitive ? */
/*    lexstrCasecmp : lexstrcmp ; */
  int (*lexstrIsCasecmp)() = isCaseSensitive ?
    strcmp : strcasecmp ;

  pickIsA(&classe, &mask) ; t = classe ;

  if ( t<0 || t>=MAXTABLE )
    messcrash("lexword2key called on impossible class %d", classe) ;

  if(!lexIsRead[classe])
    lexReadTable(classe) ;

  hashTable = LexHashTable [classe] ;
  if ( !cp || !*cp || !nClass[classe] || !arrayMax(Lexi[classe]) )
    { *key = 0;    
      chronoReturn() ;
      return FALSE ;
    }

  chrono("lexword2key") ;
  cp = lexcleanup(cp) ;  /* remove spaces etc */

  voc = stackText(Voc[classe],0) ;
  lexi = arrp(Lexi[classe], 0 ,LEXI) ;  
  h = hashText(cp, nn = nClass [classe], 0) ;

  n = max = arrayMax(hashTable) ;
  kp = arrp(hashTable, h , KEY) ;
  kpMin = arrp(hashTable, 0 , KEY) ;
  kpMax = arrp(hashTable, n -1, KEY) ;
  while ( n--)  /* circulate thru table, starting at h */
    { if(!*kp)      /* found empty slot, cp is unknown */
	{ *key = 0 ;
	  KP = kp ;  /* private to be used by lexaddkey */
	  chronoReturn() ;
	  NWG++ ;
	  return FALSE ;
	}
      if( !lexstrIsCasecmp(cp, voc +  (lexi + *kp)->nameoffset)) 
	{ *key = KEYMAKE(classe,*kp) ;
	  chronoReturn() ;
	  NWF++ ;
	  return TRUE ;   /* found */
	}
        /* circulate */
      if (!dh)
	dh = hashText(cp, nn, TRUE) ;

      kp += dh ;
      if (kp > kpMax)
	    kp -= max ;

      NWC++ ;
    }
  chronoReturn() ;
  KP = 0 ;
  return FALSE ;   /* Whole table scanned without success */
}

/**************************************************************************/
                 /* To obtain the next key in KEY  order        */
                 /* or the first entry if *key=0.               */
                 /* Updtates the value of *key,                 */
                 /* Returns 0 if the vocabulary is empty        */
                 /* or if *key is its last entry, 1 otherwise.  */
 
KEY lexLastKey(int classe)
{
  KEY key, k = 0 ;
  
  while(lexNext(classe,&k))
    key = k ;

  return key ;
}
 
/********/

BOOL lexNext(KEY classe, KEY *k)
{
 static int i = -1 ; unsigned char mask ;

 if (!pickIsA(&classe, &mask))
   { *k = 0 ; i = 0 ; return FALSE ;}
 if(!lexIsRead[classe])
   lexReadTable(classe) ;

 if(!*k)       /* Find beginning of lexique */
    { i = pickList[classe].type == 'B' ? 0 : -1; /* jump models */
      if(!classe)
	{ i = 1 ; *k = i ; return TRUE ; }    /* prevents looping on class 0 */
    }

  if (Lexi[classe]                              /* lexique non-empty */
     && (!*k || KEYKEY(*k) == i))  /* in order */
    while (i+1 < arrayMax(Lexi[classe]))    /* not at end of lexique */
      {
	*k = KEYMAKE(classe, ++i) ;
	if (!(arrp(Lexi[classe], i, LEXI)-> lock & EMPTYSTATUS) &&
	    ( mask == (mask & (arrp(Lexi[classe], i, LEXI)-> isMask)))   )
	  return TRUE; 
      }

  *k = 0 ; i = 0  ;
  return FALSE ;
}

/********************************************/
void lex2clear (KEY key) 
{ return ;    /* more complex call in ace 4 */
} 

void lexkill(KEY kk)      /*deallocates a key*/
{
 iskey(kk) ;

 chrono("lexkill") ;

 /*  saveAll() ;  must be done before  to empty the cache 
     but not here because lexkill is used by sessionControl 
  */
 if(myKey2Lex->dlx.dk)
   { myKey2Lex->dlx.dk = 0 ;
     leximodif[class(kk)] = TRUE ;
     lexAlphaMark (class(kk)) ;
   }
 myKey2Lex->addr = 0 ;
 if ( ! (myKey2Lex->lock & EMPTYSTATUS ))
   { myKey2Lex->lock |= EMPTYSTATUS;
     leximodif[class(kk)] = TRUE ;
     lexAlphaMark (class(kk)) ;
   }
 myKey2Lex->cache = 0 ;
 chronoReturn() ;
 return;
}
/***********************************/

/* Correctly sorts anything containing integers */
/* lexReanme relies on the fact that lexstrcmp must fail
 *  if the length differ
 */
int lexstrcmp(char *a,char *b)
{ register char c,d,*p,*q;

  chrono("lexstrcmp") ;
  while (*a)
    {                /* Bond7 should come before Bond68 */
      if (isdigit(*a) && isdigit(*b))
	{
	  for (p = a ; isdigit(*p) ; ++p) ;
	  for (q = b ; isdigit(*q) ; ++q) ;
	  if (p-a > q-b) { chronoReturn() ; return 1 ; }  /* the longer number is the bigger */
	  if (p-a < q-b) { chronoReturn() ;return -1 ; }
	  while (isdigit (*a))
	    { if (*a > *b) { chronoReturn() ; return 1 ; }
	      if (*a++ < *b++) { chronoReturn() ; return -1 ; }
	    }
	}
      else
        { if((c=freeupper(*a++))>(d=freeupper(*b++))) { chronoReturn() ; return 1 ; }
	  if(c<d) { chronoReturn() ; return -1 ; }
	}
    }
 if (!*b) { chronoReturn() ; return 0 ;}
 { chronoReturn() ;  return -1 ; }
 }

/***********************************/
/* Same as above but CASE SENSITIVE */
/* Correctly sorts anything containing integers */
/* lexReanme relies on the fact that lexstrcmp must fail
 *  if the length differ
 */
int lexstrCasecmp(char *a,char *b)
{ register char c,d,*p,*q;

  chrono("lexstrCasecmp") ;
  while (*a)
    {                /* Bond7 should come before Bond68 */
      if (isdigit(*a) && isdigit(*b))
	{
	  for (p = a ; isdigit(*p) ; ++p) ;
	  for (q = b ; isdigit(*q) ; ++q) ;
	  if (p-a > q-b) { chronoReturn() ; return 1 ; }  /* the longer number is the bigger */
	  if (p-a < q-b) { chronoReturn() ;return -1 ; }
	  while (isdigit (*a))
	    { if (*a > *b) { chronoReturn() ; return 1 ; }
	      if (*a++ < *b++) { chronoReturn() ; return -1 ; }
	    }
	}
      else
        { if((c = *a++) > (d = *b++)) { chronoReturn() ; return 1 ; }
	  if(c<d) { chronoReturn() ; return -1 ; }
	}
    }
 if (!*b) { chronoReturn() ; return 0 ;}
 { chronoReturn() ;  return -1 ; }
 }

/**************************************************************/

BOOL  lexaddkey(char *cp,KEY *kptr,KEY t)
                   /*add to the t lexique the word *cp */
                   /*returns TRUE if added, FALSE if known */
{
 KEY k;
 LEXI ai;
 unsigned char mask ;

 chrono("lexaddkey") ;

 pickIsA(&t, &mask) ;

 KP = 0 ;
  if(cp && lexword2key (cp,kptr,t))
    { chronoReturn() ;
      return FALSE ;
    }
				/*word allready known*/
 if(!lexIsRead[t])
   { lexReadTable(t) ;
     KP = 0 ;
   }

                                /* initialise the lexique */
 if (!Lexi[t])
   { int nBits = 8 ;
     Voc[t] = stackCreate(1 << (nBits + 2)) ;
     Lexi[t] = arrayCreate(1 << (nBits - 1), LEXI);
     LexHashTable[t] = arrayCreate(1<<nBits, KEY);
     keySet(LexHashTable[t], (1<<nBits) - 1) = 0 ; /* make room */
     nClass[t] = nBits ;
  
     if (t && (pickType(KEYMAKE(t,0)) == 'B'))
       lexaddkey (messprintf ("?%s", pickClass2Word(t)), &k, t) ;
     if (lexaddkey("\177\176(NULL KEY)",&k,t) &&
	 KEY2LEX(k) )
       KEY2LEX(k)-> lock = EMPTYSTATUS ;  /* i.e. never display it */
     KP = 0 ;
   }
 
 if (!cp || !*cp) /* call with cp=0 used for initialisation */
   {
     *kptr = KEYMAKE(t,0);
     chronoReturn() ;
     return TRUE ;
   }

 k = (KEY) arrayMax(Lexi[t]) ;
 if ((k>=MAXLEX) || ((stackMark(Voc[t]) + strlen(cp)) > MAXVOCAB))
   messcrash("Lexique %d = %s full: now %d keys, %d size stack",
	     t, pickClass2Word(t), k, stackMark(Voc[t])) ;

      /*create a lex record*/
 ai.nameoffset = stackMark(Voc[t]) ;
 ai.lock=0;
 ai.dlx.dk=NULLDISK;
 ai.addr=NULL;
 ai.cache = NULL;

      /* add the word to the end of vocabulary */
 pushText(Voc[t], lexcleanup(cp)) ;
         /*add an entry to the end of lex */
  array(Lexi[t], (int) k, LEXI) = ai ; 
/* note that Voc and Lexi must be updated first 
   for lexHashInsert to work */

 if(KP)
   { chrono("Found KP") ; chronoReturn() ;
     *KP = k ;
     /* direct insertion , be carefull if you touch the code */
     /* KP is inherited from the last call too lexword2key */
     if(arrayMax(Lexi[t]) * HASH_RATE > keySetMax(LexHashTable[t]))
       lexReHashClass(t) ;
   }
 else
   lexHashInsert(t, k) ;
 
 leximodif[t] = lexhmodif[t] = vocmodif[t] = TRUE ;
  lexAlphaMark (t) ;
  *kptr = KEYMAKE(t,k) ;
  chronoReturn() ;
  return TRUE;
}

/*************************************************/

int lexMax(int t)
{
  if (t<0 || t>MAXTABLE)
    return 0 ;
  if (!lexIsRead[t])
    lexReadTable(t) ;
  return
    Lexi[t] ? arrayMax(Lexi[t]) : 0 ;
}

/*************************************************/

int lexHashMax(int t)
{
  return LexHashTable[t] ? arrayMax (LexHashTable[t]) : 0 ;
}

/*************************************************/

int vocMax(int t)
{
  return Voc[t] ? stackMark(Voc[t]) : 0 ;
}

/*************************************************/
/*************************************************/

void lexSessionStart (void)
{
  int t ;

  for (t = 0 ; t < MAXTABLE ; ++t)
    if (lexIsRead[t] && Lexi[t])
      { lexSessionStartSize[t] = arrayMax(Lexi[t]) ;
	lexClearClassStatus (t, TOUCHSTATUS) ;
      }
    else
      lexSessionStartSize[t] = 0 ;
}

void lexSessionEnd (void)
{
  KEYSET newKeys = keySetCreate () ;
  KEYSET touchedKeys = keySetCreate () ;
  int	 t, i ;
  LEXP	 q ;
  KEY	 key ;
  static int lastSession = 0 ;

  if (lastSession == thisSession.session)
    return ;			/* prevents loops */

  /* grep NEW_MODELS should find this, 
    may want to include objects 0 and 1 then */
  for (t = 0 ; t < MAXTABLE ; t++)
    if (!pickList[t].protected && lexIsRead[t] && Lexi[t])
      { for (i = lexSessionStartSize[t]>2 ? lexSessionStartSize[t] : 2,
	     q = arrp(Lexi[t], i, LEXI) ; 
	     i < arrayMax(Lexi[t]) ; ++i, ++q)
	  if (!(q->lock & ALIASSTATUS))
	    keySet(newKeys, arrayMax(newKeys)) = KEYMAKE(t,i) ;
	for (i = 2, q = arrp(Lexi[t], 0, LEXI) ; 
	     i < arrayMax(Lexi[t]) ; ++i, ++q)
	  if ((q->lock & TOUCHSTATUS) && !(q->lock & ALIASSTATUS))
	    keySet(touchedKeys, arrayMax(touchedKeys)) = KEYMAKE(t,i) ;
      }

  if (keySetMax (newKeys))
    { keySetSort (newKeys) ;
      lexaddkey (messprintf ("new-%s", name(thisSession.userKey)), 
		 &key, _VKeySet) ;
      arrayStore (key, newKeys, "k") ;
    }
  keySetDestroy (newKeys) ;

  if (keySetMax (touchedKeys))
    { keySetSort (touchedKeys) ;
      lexaddkey (messprintf ("touched-%s", name(thisSession.userKey)), 
		 &key, _VKeySet) ;
      arrayStore (key, touchedKeys, "k") ;
    }
  keySetDestroy (touchedKeys) ;

  lastSession = thisSession.session ;
}

/************** end of file **********************/

#endif /* ACEDB4 */
