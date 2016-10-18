/*  File: lexsubs4.c
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
 * 	Richard Durbin (Sanger Centre, UK) rd@sanger.ac.uk, and
 *	Jean Thierry-Mieg (CRBM du CNRS, France) mieg@kaa.cnrs-mop.fr
 *
 * Description:
 * Exported functions:
 * HISTORY:
 * Last edited: May 14 17:44 2009 (edgrif)
 * * Mar 11 00:26 2002 (rd): changed sessionUserKey() in readTimeStamps() 
 		to thisSession.userKey because a kernel operation.
 * * Mar 10 23:15 2002 (rd): added #include <ctype.h>.
 * * Sep  2 14:47 1998 (edgrif): Removed faulty declaration of lexAlphaMark,
 *              and lexAlphaSaveAll, which are defined in lex.h anyway.
 * * Aug 29 20:07 1994 (mieg): _Tags are now defined as KEY variables
 *              their value is computed dynamically, ensuring consistency
 *              accross databases
 * * Jan 10 00:54 1992 (mieg): added lexAlias, eliminated lastmodif
 * * Dec 11 12:22 1991 (mieg): lexcleanup to remove spaces in addkey
 * * Nov  5 21:28 1991 (mieg): introduced the hash package
 * Created: Tue Nov  5 21:28:40 1991 (mieg)
 * CVS info:   $Id: lexsubs4.c,v 1.101 2009-05-29 13:16:23 edgrif Exp $
 *-------------------------------------------------------------------
 */

/***************************************************************/
/***************************************************************/
/**  File lexsubs4.c :                                         **/
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
/* NB lextstrcmp & lexstrCasecmp now moved to w1/utils.c       */
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
/*  Name make a key into a word, and word2key a word           */
/*  into a key if the word is known.  Lexaddkey(w,k,t) adds    */
/*  a new word w and sets its key k in vocabulary t.           */
/*                                                             */
/*  Lexstrcmp is a case insensitive strcmp.                    */
/*                                                             */
/***************************************************************/

#if defined(ACEDB4) || defined(ACEDB5)

#include <glib.h>

#include <wh/acedb.h>
#include <wh/aceio.h>
#include <wh/lex_bl_.h>
#include <wh/a.h>
#include <wh/lex.h>
#include <whooks/sysclass.h>
#include <whooks/systags.h>
#include <whooks/tags.h>				    /* for tagInit, classInit */
#include <wh/disk.h>					    /*call to diskprepare in lexinit*/
#include <wh/bs.h>					    /* for aliassubclass */
#include <wh/pick.h>
#include <wh/bindex.h>
#include <wh/chrono.h>
#include <wh/session.h>
#include <wh/client.h>
#include <wh/dbpath.h>
#include <ctype.h>		/* for isspace() and isdigit() */
#include <utils.h>              /* for lexstrcmp */

#define MAXLEX   ((1<<24) - 1)   /*number of entries per vocabulary <=FFFFFF*/

                                 /*number of char in a vocabulary<max(LEXOFFSET)*/
/*#define MAXVOCAB (1<<31 - 1)*/
#define MAXVOCAB (0x7fffffff)	/* compiler probably interpreted the above as 1<< (31-1) --> 1073741824
				   wheras it looks like this number is supposed to be 2147483647,
				   but the alpha-compiler complains about 1<<31 to be too large, 
				   so we make that number directly by its HEX representation */

#define MAXTABLE 256             /*number of vocabulary tables <= 256*/
                                 /* watchout 256 meaning MAXTABLE used in coresubs */
                                 /* and also in parse.c , maybe elsewhere */
                                 /* and corresponds to the first byte inside KEY */

/***********************/

static Array  Lexi1 [MAXTABLE];
static Array  Lexi2 = 0 ;
static int minFreeLexi2 = 0 ;
static KEYSET LexHashTable[MAXTABLE] ;
static int    nClass[MAXTABLE] ;  /* number of bits for hasher */
static Stack Voc  [MAXTABLE];
static BOOL  lexIsRead[MAXTABLE] ;
static int vocmodif[MAXTABLE];
static int lexhmodif[MAXTABLE];
static int leximodif[MAXTABLE];
static int lexSessionStartSize[MAXTABLE] ;
KEYSET touchedByClient = 0 ;


static Array timeStamps[MAXTABLE];
static BOOL timeModif[MAXTABLE];
static void lexKeyUpdated(KEY key);

#define si1 ((U_Int)sizeof(LEXI1))
#define si2 ((U_Int)sizeof(LEXI2))

static void lexvocread(int table) ;
static void lexvocwrite(int table);
static void lexiread(int table) ;
static void lexiwrite(int table);
static void lexhread(int table) ;
static void lexhwrite(int table);
static void lexReadTimeStamps(int t);
static void lexWriteTimeStamps(int t);

extern void lexDefineSystemTags(void) ;   /* in whooks/sysclass.c */
extern void sysClassInit (void) ;

static void lexHashInsert(int classe, KEY key) ;
void lexReHashClass(int classe) ;

static BOOL lexRealAlias(KEY key, char* newName, BOOL doAsk, BOOL keepOldName,
			 char **err_msg_out, STORE_HANDLE handle) ;

static LEXP1 KEY2LEX1(KEY kk) ;
static LEXP KEY2LEX2(KEY kk) ;

static char* nakedName(KEY key) ;  /* Does not follow aliases */
static char *realNameWithClass(KEY k, BOOL decorate) ;



#define PRINT FALSE	/* set TRUE to show save progress */

/**************************************************************/

void lexavail(unsigned long *vocnum,
	      unsigned long *lex1num,
	      unsigned long *lex2num,
	      unsigned long *vocspace, 
	      unsigned long *hspace,
	      unsigned long *totalspace)
{
  register  int t = 256;
  unsigned long n = 0, v = 0, k1 = 0, k2 = 0, h = 0, tt = 0;
  while(t--)
    if(Lexi1[t])
      { n++;
        v += (unsigned long) stackMark(Voc[t]) ;
        k1 += (unsigned long) arrayMax(Lexi1[t]) ;
        h += arrayMax(LexHashTable[t]) ;
	tt += Voc[t]->a->dim +( Lexi1[t]->dim)*(Lexi1[t]->size)+(LexHashTable[t]->dim)*(LexHashTable[t]->size);
      }
  k2 = (unsigned long) arrayMax(Lexi2) ;
  tt += Lexi2->dim * Lexi2->size ;
  *vocspace=v; *vocnum=n; *lex1num=k1; *lex2num=k2; 
  *hspace = h ; *totalspace = tt/1024;
}

/*************************************************/

void lexInit(void)
{ int n, i ;
  LEXP lxp ;
  register int t = MAXTABLE ;
  
  minFreeLexi2 = 1 ;
  n = 2000 ;
  Lexi2 = arrayCreate (n, LEXI2) ;
  array (Lexi2, n - 1, LEXI2).dlx.lx = 0 ; /* make room */
  for (i = 1 , lxp = arrp (Lexi2, i, LEXI2) ; i < n - 1 ; i++, lxp++)
   lxp->dlx.lx = i + 1 ; 

  while(t--)
    { vocmodif[t] = lexhmodif[t] = leximodif[t] = timeModif[t] = FALSE ;
      Voc[t] = 0 ; Lexi1[t] = LexHashTable[t] = timeStamps[t] = 0 ;
      nClass[t] = 0 ;
      lexIsRead[t] = FALSE ;
    }

 /*  diskprepare()  will be called implicitly on first disk access */

    /* Hard define the tags needed for sessionInit */
  lexDefineSystemTags() ;
    /* read the tag files, if they exist, for compatibility in case lex0
       has never been saved
     */
  
  lexIsRead[0] = lexIsRead[1] = TRUE ;
  lexReHashClass(0) ;
  lexReHashClass(1) ;
}

/********************************************/

static void lexi2clear(LEXP lxp)
{ int i ;
  if (lxp)
    { lxp->dlx.dk = 0 ;
      lxp->addr = 0 ;
      lxp->cache  = 0 ;
      i = lxp - arrp(Lexi2, 0, LEXI2) ;
      lxp->key = 0 ;
      lxp->dlx.lx = minFreeLexi2 ; /* overload addr */
      minFreeLexi2 = i ;
    }
}

/************************************************/

static void lexi1Clear (int t)
{ int i ; LEXI1* lx ;

  if (Lexi1[t])
    { i = arrayMax(Lexi1[t]) ;
      lx = arrp(Lexi1[t], 0, LEXI1) - 1 ;
      while (lx++, i--)
	if (lx->is2)
	  lexi2clear (arrp (Lexi2, lx->dlx.lx, LEXI2)) ;
    }
  arrayDestroy(Lexi1[t]) ;
  keySetDestroy(LexHashTable[t]) ;
  arrayDestroy(timeStamps[t]);
}

/*************************************************/
  /* To be used only from the session manager */
void lexClear(void)
{
  register int t ;
  
  for(t=3;t< MAXTABLE; t++)
    { vocmodif[t] = lexhmodif[t] = leximodif[t] = timeModif[t] = FALSE ;
      stackDestroy(Voc[t]) ;
      if (arrayExists(Lexi1[t]))
	lexSessionStartSize[t] = arrayMax(Lexi1[t]) ;
      lexi1Clear (t) ;
      keySetDestroy(LexHashTable[t]) ;
      nClass[t] = 0 ;
      lexIsRead[t] = FALSE ;
      /* lexClearClassStatus (t, TOUCHSTATUS) ; NON: this reads the lex back ! */
    }
}

BOOL lexTableRead(KEY key)
{
  return lexIsRead[class(key)];
}

/*************************************************/

static void lexReadTable(int t)
{ 
  STORE_HANDLE h;

  if (lexIsRead[t])
    return ;
  lexIsRead[t] = TRUE ;
  
  h = handleCreate();
  
  pickList[3].type = 'A' ;  /* needed during bootstrap */
  
  if (!t || pickList[t].name)
    { 
      lexvocread(t) ;
      if(Voc[t])  /* could be 0 if class is new */
	{ 
	  messStatus (hprintf(h, "Reading in class %s",
				pickClass2Word(t))) ;
	  lexiread(t) ;
	  lexhread(t) ;
	  lexSessionStartSize[t] = arrayMax(Lexi1[t]) ;
	  lexClearClassStatus (t, TOUCHSTATUS) ;
	  lexReadTimeStamps(t);
	}
      else  /* Reinitialise this lexique */
	{ 
	  KEY dummy ;
	  lexi1Clear (t) ;
	  nClass[t] = 0 ;
	  lexSessionStartSize[t] = 0 ;
	  lexaddkey(0,&dummy,t) ;
	}
     }

  handleDestroy(h);
}

/*************************************************/

void lexRead (void)
{ 
  KEY key ;
  /* table 0 is the tags, created in lexDefineTags() and dynamically
     table 1 is created by hand
     table 2 is the session chooser
     table 3 is the Voc lexique, it MUST be reinitialised
     table 4 is the BAT chooser
  */
  
  STORE_HANDLE h = handleCreate();

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
  lexDefineSystemTags () ; /* Redefine the system tags, which have been overwritten */
  tagInit () ;  /* finally, initialises correctly the _tag variables */
  
/* We now recover the correct values of the classes */
  if (lexword2key (hprintf (h, "_voc%d", _VMainClasses), &key, _VVoc) &&
      iskeyold(key))
    { lexIsRead[_VMainClasses] = FALSE ; /* we recover the tags of the previous session */
      lexReadTable(_VMainClasses) ; 
    }

  sysClassInit () ; /* We redefine the system classes which may have been lost */
  classInit () ;  /* finally, initialises correctly the _VClass variables */

  if (lexword2key (hprintf (h, "_voc%d", _VModel), &key, _VVoc) &&
      iskeyold(key))
    { lexIsRead[_VModel] = FALSE ; /* we recover the tags of the previous session */
      lexReadTable (_VModel) ;
    }
  /*  pickGetClassNames() ; */
  sysClassOptions() ;

  handleDestroy(h);
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
  
  lexAlphaSaveAll () ;
  while(t--)  /* DO save lex[0]. tags.wrm only read for compatibility if lex0 is missing */
    {
      if(t==1)
	lexiwrite(t);  /*t = 1 Global voc */
      else 
	if(Lexi1[t])
	  {
	    lexvocwrite(t);
	    lexiwrite(t);
	    lexhwrite(t);
	    lexWriteTimeStamps(t);
	      
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
  STORE_HANDLE h = handleCreate();

  if (!isWriteAccess() || !vocmodif[t])
    return ;

  if(PRINT)
    printf("\n   writingVoc %d",t);
  
  lexaddkey(hprintf(h, "_voc%d",t),&key,!t || t>_VVoc ? _VVoc : _VGlobal ) ;
  lexaddkey(hprintf(h, "_voc%d",t),&key,!t || t>_VVoc ? _VVoc : _VGlobal ) ;
  stackStore(key,Voc[t]) ;
  
  vocmodif [t] = FALSE ;
  handleDestroy(h);
}

/**************************************************************/

static void lexhread(int t)
{
  KEY key ;
  STORE_HANDLE h = handleCreate();
 
  lexaddkey(hprintf(h, "_lexh%d",t),&key,!t || t>_VVoc ? _VVoc : _VGlobal ) ;
  lexaddkey(hprintf(h, "_lexh%d",t),&key,!t || t>_VVoc ? _VVoc : _VGlobal ) ;
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

  handleDestroy(h);
}

/**************************************************************/

static void lexhwrite(int t)
{
 /*  Commented out since i want to change the hashText 
     routine till it is really good

     KEY key ;
  STORE_HANDLE h = handleCreate(); 
 
  if (!isWriteAccess())
    return ;

  if(!lexhmodif[t])
    return ;
  if(PRINT)
    printf("\n   writinglexh %d",t);

  lexaddkey(hprintf(h, "_lexh%d",t),&key,!t || t>_VVoc ? _VVoc : _VGlobal ) ;
  lexaddkey(hprintf(h"_lexh%d",t),&key,!t || t>_VVoc ? _VVoc : _VGlobal ) ;
  arrayStore(key,LexHashTable[t]) ;
  handleDestroy(h);
*/  
  lexhmodif[t] = FALSE ;
  return ;
}

/**************************************************************/

static void lexiread(int t)
{
  KEY key ;
  register LEXI1* lxi1, *lx2 ;
  register int j ;
  Array dummy ;
  STORE_HANDLE h = handleCreate(); 
 
  lexaddkey(hprintf(h, "_lexi%d",t),&key,!t || t>_VVoc ? _VVoc : _VGlobal ) ;
  dummy = arrayGet(key, LEXI1,lexiFormat) ;
  lexi1Clear (t) ;
  Lexi1[t] = dummy ;
  if (!Lexi1[t])
    messcrash("Lexi[%d] not found",t);
  
  lxi1 = arrp(Lexi1[t], 0, LEXI1) - 1;
  j = arrayMax(Lexi1[t]);
  while(lxi1++, j--)
    { lxi1->is2 = 0 ;
      lxi1->lock &= ~LOCKSTATUS ; /* NOT 3, to zero the last 2 bits */
      if (lxi1->lock & ALIASSTATUS)
	{
	  key = lxi1->dlx.alias ;
	  lx2 =  arrp(Lexi1[t], KEYKEY(key), LEXI1) ;
	  lxi1->lock  |= ISALIASSTATUS ;
	}
    }
  leximodif[t] = FALSE ;
  handleDestroy(h);
}


/**************************************************************/
    /* i must restore the disk address in the union */
static void lexiwrite(int t)
{ Array lx ;
  KEY key ;
  DISK dsk ;
  LEXP1 lxp ;
  int i, j ;
  BOOL   needCopy = FALSE ;
  STORE_HANDLE h = handleCreate(); 
 
  if (!isWriteAccess() || !leximodif[t]) 
    return ;

  lexaddkey(hprintf(h, "_lexi%d",t),&key,!t || t>_VVoc ? _VVoc : _VGlobal ) ;

  if(PRINT)
    printf("\n   writinglexi %3d,  %8d  entries.",
	   t, arrayMax(Lexi1[t]));  
  leximodif[t] = FALSE ;  /* must come before the write */

  lx = Lexi1[t] ;
  i = arrayMax(lx) ;
  lxp = arrp(lx, 0, LEXI1) - 1 ;

  while (lxp++, i--)
    if (lxp->is2)
      { needCopy = TRUE ; break ; }

  if (needCopy)  /* restore the disk address in dlx union */
    { lx = arrayCopy (Lexi1[t]) ;
      i = arrayMax(lx) ;
      lxp = arrp(lx, 0, LEXI1) - 1 ;
      while (lxp++, i--)
	if (lxp->is2)
	  { j = lxp->dlx.lx ;
	    if (j > arrayMax(Lexi2) || j <= 0)
	      messcrash("lexi1/2 inconsistency") ;
	    dsk = arrp(Lexi2, j, LEXI2)->dlx.dk ;
	    lxp->is2 = 0 ;
	    lxp->dlx.dk = dsk ;
	  }
    }     

  arrayStore(key,lx,lexiFormat) ;
  if (lx != Lexi1[t]) 
    arrayDestroy(lx) ;

  handleDestroy(h);
}

/*************************************************************************/

static void lexReadTimeStamps(int t)
{ 
  KEY key;
  STORE_HANDLE h = handleCreate();

  lexaddkey(hprintf(h, "_lext%d",t),&key,!t || t>_VVoc ? _VVoc : _VGlobal ) ;
  
  timeStamps[t] = arrayGet(key, TIMESTAMP, timeStampFormat);

  /* If this class has never had timestamps, make some now. */
  if (!timeStamps[t])
    timeStamps[t] = arrayCreate(arrayMax(Lexi1[t]), TIMESTAMP);
  
  /* below we deal with two cases, 
     1) adding timestamps to a class for the first time.
     2) objects having been added by non timestamp aware binaries. 
     In both cases we fill in the missing creation and update 
     timestamps with those of the current kernel session. */
  
  if (arrayMax(Lexi1[t]) > arrayMax(timeStamps[t]))
    { 
      int i;
      if (t>3)
	messdump("Adding missing object-timestamps for class %s",
		 pickClass2Word(t));
      for (i = arrayMax(timeStamps[t]); i<arrayMax(Lexi1[t]); i++)
	{
	  array(timeStamps[t], i, TIMESTAMP).created = thisSession.userKey ;
	  arr(timeStamps[t], i, TIMESTAMP).updated = thisSession.userKey ;
	}
      timeModif[t] = TRUE;
    }
  else 
    timeModif[t] = FALSE;

  handleDestroy(h);
  return ;
}

static void lexKeyUpdated(KEY key)
{
  int t = class(key);
  int i = KEYKEY(key);

  if (t<=1)
    return;

  if (!timeStamps[t])
    lexReadTimeStamps(t);

  if (i>arrayMax(timeStamps[t]))
    return;

  array(timeStamps[t], i, TIMESTAMP).updated = sessionUserKey();
}

static void lexWriteTimeStamps(int t)
{
  KEY key ;
  STORE_HANDLE h = handleCreate();
 
  if (!isWriteAccess())
    return ;

  if(!timeModif[t])
    return ;

  lexaddkey(hprintf(h, "_lext%d",t),&key,!t || t>_VVoc ? _VVoc : _VGlobal ) ;
 
  arrayStore(key, timeStamps[t], timeStampFormat);

  timeModif[t] = FALSE;

  handleDestroy(h);
}

void lexSetCreationUserSession(KEY key, KEY timeStamp)
{
  int t = class(key);
  int i = KEYKEY(key);
 
  if (t<=1)
    return;

  if (!timeStamps[t])
    lexReadTimeStamps(t);

  if (i>arrayMax(timeStamps[t]))
    return;

  arr(timeStamps[t], i, TIMESTAMP).created = timeStamp;
  timeModif[t] = TRUE;

}

KEY lexCreationUserSession(KEY key)
{
  int t = class(key);
  int i = KEYKEY(key);
 
  if (t<=1)
    return 0;

  if (!timeStamps[t])
    lexReadTimeStamps(t);

  if (i>arrayMax(timeStamps[t]))
    return 0;

  return arr(timeStamps[t], i, TIMESTAMP).created;
}

mytime_t lexCreationStamp(KEY key)
{
  return sessionUserSession2Time(lexCreationUserSession(key));
}

KEY lexUpdateUserSession(KEY key)
{
  int t = class(key);
  int i = KEYKEY(key);

  if (t<=1)
    return 0;

  if (!timeStamps[t])
    lexReadTimeStamps(t);

  if (i>arrayMax(timeStamps[t]))
    return 0;

  return arr(timeStamps[t], i, TIMESTAMP).updated;
}

mytime_t lexUpdateStamp(KEY key)        
{
  return sessionUserSession2Time(lexUpdateUserSession(key));
}
/******************************************************************/
  
void lexOverLoad(KEY key,DISK disk) /* used in session.c only, for bootstrap */
{ LEXP lx2 = KEY2LEX2(key) ;
  LEXP1 lx1 = KEY2LEX1(key) ;

   if (lx2)
     { lx2->dlx.dk = disk ;
       lx2->cache = 0 ;
       lx2->addr = 0 ;
       lx2->key = key ;
     }
  else
    lx1->dlx.dk = disk ;
}

/******************************************************************/

DISK lexDisk(KEY key)
{ LEXP lx2 = KEY2LEX2(key) ;
  LEXP1 lx1 = KEY2LEX1(key) ;

   return 
     lx2 ? lx2->dlx.dk : lx1->dlx.dk ;
}

/******************************************************************/
static LEXP1 myKey2Lex1 = 0 ;
static LEXP myKey2Lex2 = 0 ;

void lexSetDisk(KEY key, DISK dk)
{ DISK old = 0 ; 
  iskey(key) ;

  if (myKey2Lex2)  
    { 
      old = myKey2Lex2->dlx.dk ; myKey2Lex2->dlx.dk = dk ;
    }
  else if (myKey2Lex1) 
    { 
      old = myKey2Lex1->dlx.dk  ; myKey2Lex1->dlx.dk = dk ; 
    }
  else messcrash ("bad call to lexSetDisk(%s)", name(key)) ;

  if (dk != old) 
    {
      leximodif[class(key)] = TRUE ;
      if (class(key)>1)
	{ 
	  timeModif[class(key)] = TRUE;
	  array(timeStamps[class(key)], KEYKEY(key), TIMESTAMP).updated = 
	    sessionUserKey();
	}
    }
}

/******************************************************************/
   /* Remove leading and trailing spaces and non ascii */
   /* rd 971009: remove leading/trailing \t, change other \t to ' '
      return result in private buffer (i.e. do not change orginal)
   */

/* check that name has some non-blank chars */
BOOL lexIsGoodName(char *name)
{
  if (!name) 
    return FALSE;

  while (*name) 
    if (!isspace(*name++))
      return TRUE;
  
  return FALSE;
}

char *lexcleanup(char *cp, STORE_HANDLE handle )
{
  char *cq;
  
  while (*cp == ' ' || *cp == '\t')
    cp++ ;
  
  cp = strnew(cp, handle);
  
  for (cq = cp + strlen(cp) ; cq-- > cp ; )
    if (*cq == ' ' || *cq == '\t')
      *cq = 0 ;
    else
      break ;
  
  return cp ;
}

/******************************************************************/

void lexHardDefineKey (int table, KEY key,  char *cp) 
{ int i ;
  KEY  key2 ;
  char *cq ;
  STORE_HANDLE h = handleCreate();

  if(lexword2key(cp,&key2,table))
    {
      if (key == KEYKEY(key2))
	{
	  handleDestroy(h);
	  return ;
	}
      else
	messcrash
	  ("The old %d:%d and new %d:%d values of key %s differ.\n%s\n%s",
	   table,KEYKEY(key2), table, key, cp,
	   "As of ace.3.4 files wspec/sys* and tags can be removed",
	   "before creating the database, but not later on") ;
    }

  i = lexMax(table) ;
  if(!i) i = 1 ; /* Because a zeroth key is created by lexaddkey */

  if (!key) 
    key = i ;
  if(i < key)
    { for(;i < key; i++)
	lexaddkey(hprintf(h, "__sys%d",i),&key2,table) ;
      lexaddkey(cp,&key,table);
    }
  else if(i == key)
    lexaddkey(cp,&key,table);
  else
    { cq = name(KEYMAKE(table,key)) ;
      if(strcmp(cq,hprintf(h, "__sys%d",key)))
	messcrash("Tag %s = %d tries to overwrite tag %s",
		  cp, key, cq) ;
      /* Else I am overwriting a dummy tag */
      array(Lexi1[table], (int) key, LEXI1).nameoffset 
	= stackMark(Voc[table]) ;
      pushText(Voc[table], lexcleanup(cp, h) ) ;
      
      leximodif[table] = lexhmodif[table] = vocmodif[table] = TRUE ;
      lexAlphaMark (table) ;
    }
  handleDestroy(h);
}


/********************************************/
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
   { myKey2Lex1 = 0 ;  myKey2Lex2 = 0 ; return 0 ; }

  myKey2Lex1 = arrp(Lexi1[t], k, LEXI1);
  myKey2Lex2 = myKey2Lex1->is2 ? arrp(Lexi2, myKey2Lex1->dlx.lx, LEXI2) : 0 ;

  if (myKey2Lex2 && myKey2Lex2->key != kk)
    messcrash ("inconsistency in isNakedkey") ;

  return
    myKey2Lex1->dlx.dk || myKey2Lex2  ?  2  :  1  ; /* disk or lex2 both mean full object */
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
	  ! (myKey2Lex1->lock & ALIASSTATUS))
	break ;
      kk = myKey2Lex1->dlx.alias ;
      if (kk == old)
	 /* to alias old to new would create a loop */
	return TRUE ;
    }
  return FALSE ;
}

#ifdef NOT_NEED_BACUSE_APPERENTLY_NOT_USED
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
      if (!(myKey2Lex1->lock & ALIASSTATUS))
      	return FALSE ;
      kk = myKey2Lex1->dlx.alias ;
    }
}
#endif /* NOT_NEED_BACUSE_APPERENTLY_NOT_USED */


/********************************************/
/* Called from parse.c, removes an aliased key and relinks the alias list */
BOOL lexDestroyAlias (KEY key)
{ KEY  under = 0 ;
  LEXI1* lx ;
  int t, i ;


      if (!isNakedKey(key) ||                     /* sets myKey2lex */
	  ! (myKey2Lex1->lock & ALIASSTATUS))
	return FALSE ;
      under = myKey2Lex1->dlx.alias ;
      myKey2Lex1->lock &= ~ALIASSTATUS ;  
      myKey2Lex1->lock |= EMPTYSTATUS ;
      myKey2Lex1->dlx.alias = 0 ;
      t = class(key) ;
      leximodif[t] = TRUE ;
      lexAlphaMark (t) ;
      lx = arrp(Lexi1[t], 0, LEXI1) - 1 ;
      i = arrayMax(Lexi1[t]) ;
      while(lx++, i--)
	if ((lx->lock & ALIASSTATUS) && lx->dlx.alias == key)
	  lx->dlx.alias = under ;
      return TRUE ;

}

/********************************************/
/* returns 0:no, 1:may_be, 2:yes, key is alias of another key 
*/

int lexIsAlias (KEY key)
{
  if (!lexIsRead [class(key)])
    return 1 ; /* may be */
  isNakedKey(key) ;
  if (myKey2Lex1->lock & ISALIASSTATUS)
    return 2 ;
  return 0 ;
}

/********************************************/
/* Scans the alias list for valid object, used by bsCreate etc  */
KEY lexAliasOf (KEY key)
{
  while (TRUE)
    {
      if (!isNakedKey(key) ||                     /* sets myKey2lex */
	  ! (myKey2Lex1->lock & ALIASSTATUS))
	return key ;
      key = myKey2Lex1->dlx.alias ;
    }
}

/***************************************************/

/* returns false if key does not exist, has aliasstatus or emptystatus, 
 *  is in class 0 (a tag), a private kernel class  or is a KEYKEY 0 model.
 * Cannot use lexGetStatus for this as it follows the alias list */
BOOL lexIsKeyVisible (KEY key)
{

  if (!isNakedKey(key) || (myKey2Lex1->lock & (ALIASSTATUS | EMPTYSTATUS)))
    return FALSE ;

  if (pickList[class(key)].private)		/* i.e. a tag */
    return FALSE ;

#ifdef ACEDB4
  if (KEYKEY(key) == 0 && pickList[class(key)].type == 'B')	/* i.e. a model */
    return FALSE ;
#endif

  return TRUE ;
}

/********************************************/
   /* Iterates through the alias list */
LexKeyType iskey(KEY kk)
{
  KEY start = kk ;
  int result ;
 
  while (TRUE)
    { result = isNakedKey(kk) ;
      if (!result ||
	  ! (myKey2Lex1->lock & ALIASSTATUS))
	break ;
      kk = myKey2Lex1->dlx.alias ;
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
  if (!iskey(kk))
    return FALSE ;
  if (!myKey2Lex2)
    return myKey2Lex1->dlx.dk ? TRUE : FALSE ;
  return
    myKey2Lex2->addr || myKey2Lex2->dlx.dk ? TRUE : FALSE ;
}

/********************************************/

static LEXP1 KEY2LEX1(KEY kk)
{
  return
    iskey(kk) ?  myKey2Lex1 : 0 ;
}

/********************************************/

static LEXP KEY2LEX2(KEY kk)
{
  return
    iskey(kk) ?  myKey2Lex2 : 0 ;
}

/********************************************/
/* for external code ; creates the lex2 entry */

static int newLexi2(void)
{ LEXP lx2 ;
  int i , j ;
  
  i = minFreeLexi2 ;
  if (!i)
    { i = arrayMax (Lexi2) ;
      array (Lexi2, i + 1000, LEXI2).dlx.lx = 0 ;
      for (j = i , lx2 = arrp (Lexi2, j, LEXI2) ; j < i + 1000 ; j++, lx2++)
	lx2->dlx.lx = j + 1 ;
    }
  lx2 = arrayp(Lexi2, i, LEXI2) ;
  minFreeLexi2 = lx2->dlx.lx ;
  lx2->dlx.dk = 0 ; /* un-overload dlx */
  return i ;  
}

LEXP KEY2LEX(KEY kk)
{ DISK disk ;
  int j2 ;
  int result;

  result = iskey(kk);		/* sets myKey2Lex1,2 */
  if (!result)
    messcrash("KEY2LEX called with invalid KEY %s", name(kk));
    
  if (myKey2Lex2)
    return myKey2Lex2 ;
  if (!myKey2Lex1)
    return 0 ;
  
  disk = myKey2Lex1->dlx.dk ;
  myKey2Lex1->dlx.lx = j2 = newLexi2() ;
  myKey2Lex1->is2 = 1 ;
  myKey2Lex2 = arrayp(Lexi2, j2, LEXI2) ;
  myKey2Lex2->dlx.dk = disk ;
  myKey2Lex2->cache = 0 ;
  myKey2Lex2->addr = 0 ;
  {		/* RD 980807 : important bug fix
		   must follow aliases!!
		   NB can't use lexAliasOf() because crash during bootstrap
		   but LEXI2->key only needed for checking
		   so could remove it completely - it was bugged!
		*/
    while (arrp(Lexi1[class(kk)], KEYKEY(kk), LEXI1)->lock & ALIASSTATUS)
      kk = arrp(Lexi1[class(kk)], KEYKEY(kk), LEXI1)->dlx.alias ;
  }
  myKey2Lex2->key = kk ;
  return
    myKey2Lex2 ;
}

/********************************************/

BOOL lexlock(KEY key)
{      /* code partially  duplicated in cacheLock*/
 LEXP1 q=KEY2LEX1(key);
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
 LEXP1 q=KEY2LEX1(key);
  if(   q
     && (q->lock & LOCKSTATUS) )
    q->lock &= ~LOCKSTATUS; 
  else
    messcrash("Unbalanced unlock of %s",
	      name(key));
}

/********************************************/
   /* subClass mechanism */
void lexSetIsA (KEY key, unsigned char c) 
{     
  LEXP1 q=KEY2LEX1(key);
  if(q)
    q->isMask = c ;
}

/********************************************/
   /* subClass mechanism: test appartenance */
BOOL lexIsInClass(KEY key, KEY classe)
{ unsigned char mask ;   
  LEXP1 q=KEY2LEX1(key);

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
  /* code duplicated in cacheUpdate
     jan.97: i think this is no longer true
  */

/* do not mark as touched an object obtained from the server */
BOOL X_CLIENT_PARSING = FALSE ;

void lexSetStatus(KEY key, unsigned char c) 
{     
  LEXP1 q=KEY2LEX1(key);

  if (c == TOUCHSTATUS)
    { if (X_CLIENT_PARSING)
	return ;
      if (touchedByClient) 
	keySet(touchedByClient, keySetMax(touchedByClient)) = key;
    }

  if (q)
    q->lock |= (c & ~LEXPRIVATESTATUS) ;
  else
    messcrash("lexSetStatus called with bad key %s", name(key));
}

/********************************************/
  
  /* code duplicated in cacheUpdate
      jan.97: i think this is no longer true
  */
void lexUnsetStatus(KEY key, unsigned char c) 
{     
  LEXP1 q=KEY2LEX1(key);

  if (q)
    q->lock &= (~c) | LEXPRIVATESTATUS ;
  else
    messcrash("lexUnsetStatus called with bad key %s",
	      name(key));
}

/********************************************/
  
void lexClearClassStatus(KEY classe, unsigned char c) 
{ int i ;        
  LEXP1 q ;
 
  c = (~c) | LEXPRIVATESTATUS ;
  if ((i = lexMax(classe)))
    { q = arrp(Lexi1[classe], 0, LEXI1) - 1 ;
      while (q++, i--)
	q->lock &= c ;
    }
}

/********************************************/

unsigned char lexGetStatus(KEY key)
{ 
  LEXP1 q=KEY2LEX1(key);
  if(q)
    return q->lock ;
  else
    messcrash("lexGetStatus called with bad key %s",
	      name(key));
  return 0 ;  /* for compiler happiness */
}

/********************************************/


/* A set of functions to go to/from    "string" <--> tag/class
 * 
 * NOTE that all these functions return pointers to strings that are internal to acedb,
 * they should NOT be free'd or altered in any way. If you need to hold on to them then
 * you chould free them, you should note in particular that doing things like the
 * following will _not_ produce the result you want:
 * 
 * printf("obj1: %s, obj2 %s, nameWithClass(obj1_key), nameWithClass(obj2_key)) ;
 * 
 * You need to take copies of the strings returned by nameWithClass().
 * 
 * Also note that if you pass a crud key in, you will get output of the form:
 * 
 * (NULL KEY)........
 * 
 *  */


/* Return the text associated with a key. */
char *name(KEY kk)  
{     
  return (iskey(kk)
	  ? stackText(Voc[class(kk)], myKey2Lex1->nameoffset)
	  : "\177\177(NULL KEY)") ;     
}


/* Return the object class and name for a key in the simple form  'class:name',
 * good for producing output that will be parsed by a program. */
char *nameWithClass(KEY k)
{
  char *buf ;

  buf = realNameWithClass(k, FALSE) ;

  return buf ;
}


/* Return the object class and name for a key in the form  class:"name"  this makes
 * it easier to read when users have used ":" as part of the name of their objects. */
char *nameWithClassDecorate(KEY k)
{
  char *buf ;

  buf = realNameWithClass(k, TRUE) ;

  return buf ;
}


/* Case insensitive compare. */
BOOL keyIsStr(KEY key, char *str)
{
  BOOL result = FALSE ;

  if (g_ascii_strcasecmp(name(key), str) == 0)
    result = TRUE ;

  return result ;
}

/* Case sensitive compare. */
BOOL keyIsExactStr(KEY key, char *str)
{
  BOOL result = FALSE ;

  if (strcmp(name(key), str) == 0)
    result = TRUE ;

  return result ;
}


/* Is str in key. */
BOOL keyHasStr(KEY key, char *sub_str)
{
  BOOL result = FALSE ;

  if ((strstr(name(key), sub_str)))
    result = TRUE ;

  return result ;
}



/* Convert string to tag key. */
KEY str2tag(char *tagname)
{
  KEY k = KEY_NULL ; 
 
  lexaddkey(tagname, &k, 0);

  return k ;
}





/* Iterates through the alias list */
BOOL nextName(KEY key, char **cpp)  
{
  static LEXP1 lxp = 0 ;
  static KEY lastKey = 0 , kk ;
  
  if ( *cpp && key != lastKey)
    messcrash ("nextName %s called out of context", nakedName(key)) ;
 
  if (!*cpp)    /* initialise */
    kk = lastKey = key ;
  else         /* loop */
    { if (!lxp || !(lxp->lock & ALIASSTATUS)) /* shift once */
	return FALSE ;
      kk = lxp->dlx.alias ;
    }

  while (TRUE)
    { if (!isNakedKey(kk))
	return FALSE ;
      if (!(myKey2Lex1->lock & EMPTYSTATUS))   /* if empty */
	break ;
      if (!(myKey2Lex1->lock & ALIASSTATUS))  /* loop again */
	return FALSE ;
      kk = myKey2Lex1->dlx.alias ;
    }
  lxp = myKey2Lex1 ;                         /* used by shift once */
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

  key = arrayMax(Lexi1[classe]) ;
  while (key--)
    lexHashInsert(classe, key) ;
  chronoReturn() ;
}

/****************************************************/
   /* should only be called if !lexword2key */
   /* key must be just the KEYKEY part */
int NII  , NIC  , NWC  , NWF  , NWG  ;
/* Simon Kelley sent this fix. so i have put it in (il) */
static void lexHashInsert(int classe, KEY key)
{
  static KEYSET hashTable ;
  unsigned int h, dh = 0 ;
  int max, n , nn ; 
  KEY *kp, *kpMax ;
  char *cp ;
  BOOL failedFirstPass = FALSE;

  chrono("lexHashInsert") ;
  key = KEYKEY (key) ;
  hashTable  = LexHashTable[classe] ;
retry:
  if(failedFirstPass || 
     (arrayMax(Lexi1[classe]) * HASH_RATE > keySetMax(hashTable)))
    lexReHashClass(classe) ;

  cp = nakedName(KEYMAKE(classe,key)) ;
  if (!*cp)
    return ;  /* may occur after a rename */
  h = hashText(cp, nClass [classe], FALSE) ;

  nn = nClass[classe] ;
  n = max = 1 << nn ;

  kp = arrp(hashTable, h , KEY) ;
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
  if (failedFirstPass) 
    messcrash("Hash table full, NIC = %d, lexhashInsert is bugged", NIC)
;
  else
    { /* we may fail to find a slot because of the presence of stale
keys */
      /* after lexHardRename has run, try one more time, having purged
*/
      /* the stale keys by re-hashing - srk */
      failedFirstPass = TRUE;
      goto retry;
    }
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
    { myKey2Lex1->nameoffset = stackMark(Voc[classe]) ;
      leximodif[classe] = TRUE ;	/* since nameoffset is changed */
      pushText (Voc[classe], newName) ;
    }
  else
    strcpy (oldName, newName) ;
    
  if (myKey2Lex1->lock & EMPTYSTATUS)  /* because the new name should appear in lists */
    { myKey2Lex1->lock &= ~EMPTYSTATUS ;
      leximodif[class(key)] = TRUE ;
    }
  lexHashInsert(classe, key) ; /* re-insert in hash table */
  
  vocmodif[classe] = TRUE ;

}
  
/********************************************/

void lex2Check (void)
{return ;
/*
  LEXP lx = arrp(Lexi2, 0, LEXI2) - 1 ;
  int i = arrayMax(Lexi2) ;

  while (lx++, i--)
  if (lx->cache && (*(KEY*)lx->cache) != lx->key)
  messcrash("lex2Check error") ;
  */
}

void lex2clear (KEY key)
{ iskey (key) ;
  if (!myKey2Lex2 ||
      myKey2Lex2->cache ||
      myKey2Lex2->addr)
    return ;
/*   lex2Check() ; */
  myKey2Lex1->dlx.dk = myKey2Lex2->dlx.dk ;
  myKey2Lex1->is2 = 0 ;
  lexi2clear(myKey2Lex2) ;
}

/********************************************/

static void lexMakeAlias(KEY key, KEY alias, BOOL keepOldName)
{ isNakedKey(key) ;
  
  myKey2Lex1->dlx.alias = alias ; /* overloading dlx union with alias key */
  myKey2Lex1->lock = ALIASSTATUS ;  
  if (!keepOldName)
    myKey2Lex1->lock |= EMPTYSTATUS ;  

      /* destroy the lex2 entry */
  myKey2Lex1->is2 = 0 ;
  if (myKey2Lex2)
    lexi2clear(myKey2Lex2) ;
      
  if (!keepOldName)              /* Clean up */
    { char *cp = nakedName(key) ;
      while (*cp) *cp++ = 0 ;
      myKey2Lex1->nameoffset = 0 ;
    }
  
  isNakedKey(alias) ;
  myKey2Lex1->lock &= ~EMPTYSTATUS ;
  myKey2Lex1->lock |= ISALIASSTATUS ;
  leximodif[class(key)] = TRUE ;
}
 

/********************************************/
/* Returns TRUE if newName is accepted */
/* if keepOldName, the 2 names will be used in the future,
 * so we need 2 keys anyway,
 * If not, we would rather keep a single key except if the old 
 * and new one allready exist because they may be referenced from
 * elsewhere.
 */
static BOOL lexRealAlias(KEY key, char* newName, 
			 BOOL doAsk, BOOL keepOldName,
			 char **err_msg_out,
			 STORE_HANDLE handle)
{
  char *oldName, *oldnamepreserve = 0 ;
  int  classe = class(key) ;
  KEY  newKey ;
  BOOL isCaseSensitive = pickList[classe & 255 ].isCaseSensitive ;
  int (*lexstrIsCasecmp)() = isCaseSensitive ?
    strcmp : strcasecmp ;

     /********** Protections *************/ 

  if (!isNakedKey(key) ||  /* key is not a vocabulary entry */
      (myKey2Lex1->lock & (ALIASSTATUS | EMPTYSTATUS)))
	/* already aliased or EMPTY, to forbid complex alias graphs */
    return TRUE ; /* no error when parsing same ace file twice */

  if (lexiskeylocked(key))
    {
      if (doAsk) 
	messout("Sorry, alias fails because %s is locked elsewhere", name(key)) ;
      else if (err_msg_out)
	*err_msg_out = hprintf(0, "Sorry, alias fails because %s is locked elsewhere",
			       name(key)) ;

      return FALSE ;
    }

  /************* Trivial cases *************/

  newName = lexcleanup(newName, handle) ;
  oldName = name(key) ;

  if (!strcmp(oldName, newName)) /* Identity */
    return TRUE ;

  if (externalAlias) 
    oldnamepreserve = strnew(oldName, handle) ;
  if (!lexstrIsCasecmp(oldName, newName)) /* Equivalence and certainly same length */
    { lexHardRename(key, newName) ;         /* but not exact (dealt with earlier */
    lexKeyUpdated(key);
      goto ok ;
    }

     /************* Hard Rename to unknown name case *************/

  if (!keepOldName
      && !lexword2key (newName, &newKey, classe)) 
    { lexHardRename(key, newName) ;
      lexKeyUpdated(key);
      goto ok ;
    }

     /************ Else I want the 2 keys defined ************/

  lexaddkey(newName, &newKey, classe) ;  /* attention, this may invalidate oldName pointer */

  if (lexiskeylocked(newKey))
    {
      if (doAsk)
	messout("Sorry, alias fails because %s is locked elsewhere", name(newKey)) ;
      else if (err_msg_out)
	*err_msg_out = hprintf(0, "Sorry, alias fails because %s is locked elsewhere",
			       name(newKey)) ;
      return FALSE ;
    }
  
  if(lexAliasLoop(key, newKey))  /* No loops */
    {
      if (doAsk)
	messout("Sorry, aliasing %s to %s would create a loop.",
		oldName, newName) ;
      else if (err_msg_out)
	*err_msg_out = hprintf(0, "Sorry, aliasing %s to %s would create a loop.",
			       oldName, newName) ;
      return FALSE ;
    }

  /* should I fuse ? */
  if (isNakedKey(key) == 1 )                /* key is actually empty */
    lexMakeAlias(key, newKey, keepOldName) ;
  else     /*  isNakedkey(key) == 2 */
    {
      if (isNakedKey(newKey) == 1 )             /* key is full but newkey is empty */
	{                                  /* Just shift things over */
	  LEXP1
	    newLex1 = KEY2LEX1(newKey) ,
	    lex1 = KEY2LEX1(key) ;
	  LEXOFFST offset = lex1->nameoffset ;
	  KEY kk ;

	  lex1->nameoffset = newLex1->nameoffset ;
	  newLex1->nameoffset = offset ;
	  
	  kk = key ; key = newKey ; newKey = kk ; /* swap before clean up */
	  lexMakeAlias(key, newKey, keepOldName) ;
	  lexKeyUpdated(newKey);
	}
      else  /* also  isNakedKey(newKey) == 2 */
	{
	  if (pickType(key) != 'B')
	    {
	      char *msg = "merging of non-Tree objects cannot be done." ;
	      if (doAsk)
		messout(msg) ;
	      else if (err_msg_out)
		*err_msg_out = hprintf(0, msg) ;
	      return FALSE ;
	    }

	  /* NOTE WELL, if interactive we ask the user if they really want   */
	  /* to merge one object with another. But even if err_msg_out is    */
	  /* non-NULL we don't return anything here, the fusing is just done.*/
	  if (doAsk &&
	      !messQuery
	      (hprintf(handle, "%s and %s both exist.\n"
		       "In case of conflict i will rather keep the data of %s.\n\n"
		       "Do you want to proceed",
		       name(key), name(newKey), name(newKey))))
	    return FALSE ;

	  bsFuseObjects (key, newKey) ;

	  /* to late to return FALSE, old is already destroyed */
	  lexMakeAlias(key, newKey, keepOldName) ;
	  lexKeyUpdated(newKey);
	  lexKeyUpdated(key);
	}
    }
ok:
  lexHashInsert(classe, key) ; /* re-insert in hash table */

  if (pickType (key) == 'B')  /* set subclass lex bits */
    {
      OBJ obj = bsCreate (key) ;
      bIndexObject(key, obj) ;   /* needed always */
      bsDestroy (obj) ;
    }
  lexAlphaMark (class(key)) ;
 
  if (externalAlias) 
    externalAlias (key, oldnamepreserve, keepOldName) ;

  return TRUE ;
}
  
int lexNakedAlphaOrder(void *a, void *b) /* called in lexalpha.c */
{ 
  KEY k1 = *(KEY*)a, k2 = *(KEY*)b ;
  int i = class(k1) - class(k2) ;

  if (i)   
    return i ;
  return lexstrcmp (nakedName(k1), nakedName(k2)) ;
}

/********************************************/
/* Alias functions, these will set up an alias for an object, or if the new  */
/* name is an existing object, the two objects will be merged.               */
/*                                                                           */
/* I would like to see lexAlias gradually replaced by lexDoAlias.            */
/*                                                                           */
/* The original lexAlias() function either does not output error messaages   */
/* or outputs them to the screen. For the parse code to work with the        */
/* server, I need to returned a pointer to the message. This is done via     */
/* err_msg_out, if its NULL no error message is returned (as in lexAlias),   */
/* otherwise an error message is returned. N.B. caller is responsible for    */
/* messfree() of err_msg_out.                                                */
BOOL lexAlias(KEY key, char* newName, BOOL doAsk, BOOL keepOldName)
{
  STORE_HANDLE h ;
  BOOL ret ;

  h = handleCreate();
  ret = lexRealAlias(key, newName, doAsk, keepOldName, NULL, h);
  handleDestroy(h);

  return ret;
}

BOOL lexDoAlias(KEY key, char* newName, BOOL doAsk, BOOL keepOldName, char **err_msg_out)
{
  STORE_HANDLE h ;
  BOOL ret ;

  h = handleCreate();
  ret = lexRealAlias(key, newName, doAsk, keepOldName, err_msg_out, h);
  handleDestroy(h);

  return ret;
}



/********************************************/
/********************************************/

/* following are private communication lexword2key -> lexaddkey */
static KEY *KP ;
static BOOL isSubclassFailInWord2key ;

BOOL lexword2key(char *cp, KEY *key, KEY classe)
                        /* given a word *cp, sets its key in *key */
                        /* returns TRUE if found, FALSE if not */
                        /* No completion performed */
			/* classe can be a table or key in class Class */
{
  unsigned int h, dh = 0 ;
  int max, n , nn ;
  KEY *kp, *kpMax ;
  KEYSET hashTable ;
  int t ; unsigned char mask ;
  LEXI1 *lexi ;
  char *voc ;
  BOOL isCaseSensitive ;
  int (*lexstrIsCasecmp)() ; 

  pickIsA(&classe, &mask) ; t = classe ;
  isCaseSensitive = pickList[t & 255 ].isCaseSensitive ;
  lexstrIsCasecmp = isCaseSensitive ? strcmp : strcasecmp ;

  if (t < 0 || t >= MAXTABLE)
    messcrash("lexword2key called on impossible class %d", classe) ;

  if(!lexIsRead[classe])
    lexReadTable(classe) ;

  hashTable = LexHashTable [classe] ;
  if ( !cp || !*cp || !nClass[classe] || !arrayMax(Lexi1[classe]) )
    { *key = 0;    
      chronoReturn() ;
      return FALSE ;
    }

  chrono("lexword2key") ;
  cp = lexcleanup(cp, 0) ;  /* remove spaces etc */

  voc = stackText(Voc[classe],0) ;
  lexi = arrp(Lexi1[classe], 0 ,LEXI1) ;  
  h = hashText(cp, nn = nClass [classe], 0) ;

  n = max = arrayMax(hashTable) ;
  kp = arrp(hashTable, h , KEY) ;
  kpMax = arrp(hashTable, n -1, KEY) ;
  while ( n--)  /* circulate thru table, starting at h */
    { if(!*kp)      /* found empty slot, cp is unknown */
	{ *key = 0 ;
	  KP = kp ;  /* private to be used by lexaddkey */
	  chronoReturn() ;
	  NWG++ ;
	  messfree(cp);
	  return FALSE ;
	}
      if (!lexstrIsCasecmp(cp, voc +  (lexi + *kp)->nameoffset)) 
	{ *key = KEYMAKE(classe,*kp) ;
	  chronoReturn() ;
	  if (mask != (mask & lexi[*kp].isMask)) /* subset criteria fail */
	    { isSubclassFailInWord2key = TRUE ;
	      NWG++ ;
	      messfree(cp);
	      return FALSE ;
	    }
	  NWF++ ;
	  messfree(cp);
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
  messfree(cp);
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
  KEY key = 0, k = 0 ;
  
  while(lexNext(classe,&k))
    key = k ;

  return key ;
}
 
/********/
		/* RD 980713: removed static i to check sequential use on same
		   class.  Necessary for AQL, but discussion with jean concluded
		   this is also correct, not changing the semantics described above!
		*/

BOOL lexNext (KEY classe, KEY *kp)
{
  LEXP1 lxp1 ;
  int i ;
  unsigned char mask ;
  
  if (!pickIsA(&classe, &mask))
    { *kp = 0 ; return FALSE ; }

  if (!lexIsRead[classe])
    lexReadTable(classe) ;
  
  if (!*kp)       /* Find beginning of lexique */
    {
      if (!classe)
	{ *kp = 1 ; return TRUE ; }    /* prevents looping on class 0 */
      else
	{
#ifdef ACEDB4
	  i = pickList[classe].type == 'B' ? 0 : -1 ;
#else
	  i = -1 ;
#endif
	}
    }
  else				/* *kp is already in the classe (we hope!) */
    { if (class(*kp) != classe)
	{ messerror ("class mixup in lexNext: class(%s) is %s not %s",
		     name(*kp), className(*kp), className(KEYMAKE(classe,0))) ;
	  return FALSE ;
	}
      i = KEYKEY(*kp) ;
    }
  
  if (Lexi1[classe]                              /* lexique non-empty */
      && i+1 < arrayMax(Lexi1[classe]))
    { lxp1 = arrp(Lexi1[classe], i, LEXI1) ;
      while (++lxp1, ++i < arrayMax(Lexi1[classe]))    /* not at end of lexique */
	  if (!(lxp1-> lock & EMPTYSTATUS))
	    {
	      if (!(lxp1->lock & ALIASSTATUS))
		{
		  
		  if ( mask == (mask & lxp1-> isMask))
		    { *kp = KEYMAKE(classe, i) ;
		    return TRUE;
		    } 
		}
	      else
		{ /* it an alias so test the mask on the canonical key */
		  lexAliasOf(KEYMAKE(classe, i));
		  if ( mask == (mask & myKey2Lex1-> isMask))
		    { 
		      *kp = KEYMAKE(classe, i) ;
		      return TRUE;
		    } 
		}

	    }	    
    }
  *kp = 0 ;
  return FALSE ;
}

/********************************************/
void lexkill(KEY kk)      /*deallocates a key*/
{
  iskey(kk) ;
  
  if (!myKey2Lex1)
    return ;
  chrono("lexkill") ;
  
  /*  saveAll() ;  must be done before  to empty the cache 
      but not here because lexkill is used by sessionControl 
      */ 

  if (lexDestroyAlias(kk))
    return ;

  if(myKey2Lex1->dlx.dk)
   { myKey2Lex1->dlx.dk = 0 ;
     leximodif[class(kk)] = TRUE ;
     lexAlphaMark (class(kk)) ;
   }
  
  if ( ! (myKey2Lex1->lock & EMPTYSTATUS ))
    { myKey2Lex1->lock |= EMPTYSTATUS;
      leximodif[class(kk)] = TRUE ;
      lexAlphaMark (class(kk)) ;
    }
  myKey2Lex1->is2 = 0 ;
  if (myKey2Lex2)
    lexi2clear(myKey2Lex2) ;
  
  chronoReturn() ;
  return;
}

/**************************************************************/

BOOL  lexaddkey(char *cp,KEY *kptr,KEY t)
                   /*add to the t lexique the word *cp */
                   /*returns TRUE if added, FALSE if known */
{
  KEY k;
  LEXI1 ai;
  unsigned char mask ;
#ifndef ACEDB4
  extern BOOL READING_MODELS ;
#endif
  STORE_HANDLE h = handleCreate();

  chrono("lexaddkey") ;
  
  pickIsA(&t, &mask) ;
  
#ifndef ACEDB4
  if (t == _VMainClasses  && !READING_MODELS  )
    {
      handleDestroy(h);
      return lexword2key (cp, kptr, t) ;
    }
#endif
  
  KP = 0 ;
  isSubclassFailInWord2key = FALSE ;
  if (cp && (lexword2key (cp,kptr,t) || isSubclassFailInWord2key))
    {
      chronoReturn() ;
      handleDestroy(h);
      return FALSE ;
    }
				/* word already known */
  if (!lexIsRead[t])
    { 
      lexReadTable(t) ;
      KP = 0 ;
    }
                                /* initialise the lexique */
  if (!Lexi1[t])
    {
      int nBits = 8 ;
      Voc[t] = stackCreate(1 << (nBits + 2)) ;
      stackTextOnly(Voc[t]);
      Lexi1[t] = arrayCreate(1 << (nBits - 1), LEXI1);
      
      timeStamps[t] = arrayCreate(1 << (nBits - 1), TIMESTAMP);
      timeModif[t] = TRUE;
	
      LexHashTable[t] = arrayCreate(1<<nBits, KEY);
      keySet(LexHashTable[t], (1<<nBits) - 1) = 0 ; /* make room */
      nClass[t] = nBits ;
      
#ifdef ACEDB4
      if (t && (pickType(KEYMAKE(t,0)) == 'B'))
	lexaddkey (hprintf (h, "?%s", pickClass2Word(t)), &k, t) ;
#endif
      if (lexaddkey("\177\176(NULL KEY)",&k,t) &&
	  KEY2LEX1(k) )
	KEY2LEX1(k)-> lock = EMPTYSTATUS ;  /* i.e. never display it */
      KP = 0 ;
    }
  
  k = (KEY) arrayMax(Lexi1[t]) ;
  if (!cp && k <= 2) /* call with cp=0 used for initialisation */
    {
      *kptr = KEYMAKE(t,0);
      chronoReturn() ;
      handleDestroy(h) ;
      return TRUE ;
    }
  
  cp = lexcleanup(cp, h) ;
  if (!cp || !*cp)
    messcrash("Attempt to create a key with emtpy name in class%s",
	      className (KEYMAKE(t,0))) ;
  
  if ( (k>=MAXLEX) ||
       ( (stackMark(Voc[t]) + strlen(cp)) > MAXVOCAB ))
    messcrash("Lexique %d full: now %d keys, %d size stack",
	      t, k, stackMark(Voc[t])) ;
  
  /*create a lex record*/
  ai.nameoffset = stackMark(Voc[t]) ;
  ai.dlx.dk = 0 ;
  ai.lock = 0;
  ai.isMask = 0;
  ai.is2 = 0 ;
  
  /* add the word to the end of vocabulary */
  pushText(Voc[t], cp) ;
  /*add an entry to the end of lex */
  array(Lexi1[t], (int) k, LEXI1) = ai ; 
  /* note that Voc and Lexi must be updated first 
     for lexHashInsert to work */
  
  if (KP)
    { chrono("Found KP") ; chronoReturn() ;
    *KP = k ;
    /* direct insertion , be careful if you touch the code */
    /* KP is inherited from the last call to lexword2key */
    
    /* SRK - restored this to original in lexsubs.c, we were
       re-hashing on every call to lexaddkey 
       if(arrayMax(Lexi1[t]) > .34 * keySetMax(LexHashTable[t]))
    */
    if(arrayMax(Lexi1[t]) * HASH_RATE > keySetMax(LexHashTable[t]))
      lexReHashClass(t) ;
    }
  else
    lexHashInsert(t, k) ;
  
  /* NOTE: sessionUserKey() can call lexaddkey which corrupts KP, so
     the following code MUST follow the block above, and not vice-versa, 
     alternatively, rewrite this stuff to eliminate the static variables
     and do us all a favour. - srk */

  if ((t>1) && timeStamps[t])
    {
      array(timeStamps[t], (int) k, TIMESTAMP).created =
	array(timeStamps[t], (int) k, TIMESTAMP).updated = 
	sessionUserKey();  
      timeModif[t] = TRUE;
    }
  
  leximodif[t] = lexhmodif[t] = vocmodif[t] = TRUE ;
  lexAlphaMark (t) ;
  *kptr = KEYMAKE(t,k) ;
  chronoReturn() ;
  handleDestroy(h);
  return TRUE;
}

/*************************************************/

int lexMax(int t)
{
  int nn = 0 ;
  if (t<0 || t>MAXTABLE)
    return 0 ;
  if (!lexIsRead[t])
    lexReadTable(t) ;
  if (Lexi1[t])
    nn = arrayMax(Lexi1[t]) ; /* nn introduced to please solaris */
  return nn ;
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
    if (lexIsRead[t] && Lexi1[t])
      { lexSessionStartSize[t] = arrayMax(Lexi1[t]) ;
	lexClearClassStatus (t, TOUCHSTATUS) ;
      }
    else
      lexSessionStartSize[t] = 0 ; 
  keySetDestroy (touchedByClient) ;
  if (externalSaver) touchedByClient = keySetCreate () ;

}

void lexSessionEnd (void)
{
  static int lastSession = 0 ;
  KEYSET newKeys = keySetCreate () ;
  KEYSET touchedKeys = keySetCreate () ;
  int	 t, i ;
  LEXI1* q ;
  char *cp = 0 ;
  STORE_HANDLE h = handleCreate();
  ACEOUT new_out, touched_out;
#ifndef ACEDB4
#define KEYKEY0 1
#else
#define KEYKEY0 2
#endif

  if (lastSession == thisSession.session)
    {
      handleDestroy(h);
      return ;			/* prevents loops */
    }

  for (t = 0 ; t < MAXTABLE ; t++)
    if (!pickList[t].protected && lexIsRead[t] && Lexi1[t])
      { 
	q = 0 ;
	i = lexSessionStartSize[t]>KEYKEY0 ? lexSessionStartSize[t] : KEYKEY0 ;
	if (i < arrayMax(Lexi1[t]))
	  {  
	    q = arrp(Lexi1[t], i, LEXI1) ;
	    for(; i < arrayMax(Lexi1[t]) ; ++i, ++q)
	      if (!(q->lock & ALIASSTATUS))
		keySet(newKeys, arrayMax(newKeys)) = KEYMAKE(t,i) ;
	  }
	if (KEYKEY0 < arrayMax(Lexi1[t]))
	  for (i = KEYKEY0, q = arrp(Lexi1[t], i, LEXI1) ; 
	       i < arrayMax(Lexi1[t]) ; ++i, ++q)
	    if ((q->lock & TOUCHSTATUS) && !(q->lock & ALIASSTATUS))
	      keySet(touchedKeys, arrayMax(touchedKeys)) = KEYMAKE(t,i) ;
      }

  if (keySetMax (newKeys))
    { keySetSort (newKeys) ;
#ifdef NEW_INSIDE
      lexaddkey (hprintf (h, "new-%s", name(thisSession.userKey)), 
		 &key, _VKeySet) ;
      arrayStore (key, newKeys, "k") ;
#else  /* !NEW_INSIDE */

#if !( defined(MACINTOSH) || defined(WIN32) )
      seteuid(euid);
#endif /* !(MAC || WIN) */

      if (!dbPathStrictFilName("database", "new", "", "wd", h))
	{
	  char *path = dbPathMakeFilName("database", "new", "", h);
	  int result = mkdir (path, 0755);
	  if (result == -1)
	    messerror ("Cannot create directory %s (%s)",
		       path, messSysErrorText());
	}

      cp = dbPathMakeFilName("database/new",
			     hprintf (h, "new-%s", name(thisSession.userKey)),
			     0, h) ;

      new_out = aceOutCreateToFile (cp, "w", h);
      if (!new_out) 
	{ 
	  if (errno == EACCES)	/* permission denied */
	    messout ("Please make sure the directory 'database/new' is writable !") ;
	  else
	    messout ("Please create a writable directory called 'database/new' !") ;
	  cp = dbPathMakeFilName("database",
				 hprintf (h, "new-%s", 
					  name(thisSession.userKey)),
				 0, h) ;
	  new_out = aceOutCreateToFile (cp, "w", h);
	}
      if (new_out)
	keySetDump(new_out, newKeys) ;

#if !( defined(MACINTOSH) || defined(WIN32) )
      seteuid(ruid);
#endif /* !(MAC || WIN) */

#endif /* !NEW_INSIDE */
    }
  keySetDestroy (newKeys) ;

  if (keySetMax (touchedKeys))
    {
      keySetSort (touchedKeys) ;
#ifdef NEW_INSIDE
      lexaddkey (hprintf (h, "touched-%s", name(thisSession.userKey)), 
		 &key, _VKeySet) ;
      arrayStore (key, touchedKeys, "k") ;
#else  /* !NEW_INSIDE */
#if !( defined(MACINTOSH) || defined(WIN32) )
      seteuid(euid);
#endif /* !(MAC || WIN) */

      if (!dbPathStrictFilName("database", "touched", "", "wd", h))
	{
	  char *path = dbPathMakeFilName("database", "touched", "", h);
	  int result = mkdir (path, 0755);
	  if (result == -1)
	    messerror ("Cannot create directory %s (%s)",
		       path, messSysErrorText());
	}

      cp = dbPathMakeFilName("database/touched",
			     hprintf (h, "touched-%s", 
				      name(thisSession.userKey)), 
			     0, h) ;
      touched_out = aceOutCreateToFile (cp, "w", h);

      if (!touched_out) 
	{ 
	  if (errno == EACCES)	/* permission denied */
	    messout ("Please make sure the directory 'database/touched' is writable !") ;
	  else
	    messout ("Please create a writable directory called 'database/touched' !") ;
	  cp = dbPathMakeFilName("database",
				 hprintf (h, "touched-%s", 
					  name(thisSession.userKey)),
				 0, h) ;
	  touched_out = aceOutCreateToFile (cp, "w", h);
	}
      if (touched_out)
	keySetDump(touched_out, touchedKeys);

#if !( defined(MACINTOSH) || defined(WIN32) )
      seteuid(ruid);
#endif /* !(MAC || WIN) */

#endif /* !NEW_INSIDE */
    }
  keySetDestroy (touchedKeys) ; 
  keySetDestroy (touchedByClient) ;
  handleDestroy(h);

  lastSession = thisSession.session ;

  return;
} /* lexSessionEnd */






/* 
 *             Internal functions.
 */

/* Avoid the alias system, used by word2key and hasher */
static char *nakedName(KEY kk)  
{     
  return (isNakedKey(kk)
	  ? stackText(Voc[class(kk)], myKey2Lex1->nameoffset)
	  : "\177\177(NULL KEY)") ;
}


static char *realNameWithClass(KEY k, BOOL decorate)
{
  static char *buf = NULL ;
  static int buflen = 0 ;
  char *nam, *classNam, *format ;
  enum {EXTRA_CHARS = 4} ;				    /* room for : + " + "\0" */

  /* God where is the error handling in className().... */
  nam = name(k) ;
  classNam = className(k) ;

  if ((strlen(nam) + strlen(classNam) + EXTRA_CHARS) > buflen)
    {
      if (buf)
	messfree(buf) ;
      buflen = strlen(nam) + strlen(classNam) + EXTRA_CHARS ;
      buf = (char*)messalloc(buflen) ;
    }

  if (decorate)
    format = "%s:\"%s\"" ;
  else
    format = "%s:%s" ;

  sprintf(buf, format, classNam, nam) ;

  return buf ;
}



/************** end of file **********************/

#endif /* ACEDB4 */



