/*  File: session.c
 *  Author: Jean Thierry-Mieg (mieg@mrc-lmb.cam.ac.uk)
 *  Copyright (C) J Thierry-Mieg and R Durbin, 1992
 *-------------------------------------------------------------------
 * This file is part of the ACEDB genome database package, written by
 * 	Richard Durbin (MRC LMB, UK) rd@mrc-lmb.cam.ac.uk, and
 *	Jean Thierry-Mieg (CRBM du CNRS, France) mieg@kaa.cnrs-mop.fr
 *
 * Description:
     Session control, passwords, write access, locks etc.
 * Exported functions:
 * HISTORY:
 * Last edited: Aug 31 13:27 1994 (srk)
 * * Feb  2 11:46 1992 (mieg): interactive session control
 * * Jan 24 11:20 1992 (mieg): session fusion.
 * Created: Fri Jan 24 11:19:27 1992 (mieg)
 *-------------------------------------------------------------------
 */

/* # $Id: sessionnew.c,v 1.4 1994-08-31 13:28:04 srk Exp $  */



#include "acedb.h"
#include "disk_.h"   /* defines BLOCKHEADER */
#include "graph.h"  
#include "array.h"
#include "menu.h"
#include "mytime.h"

#if defined(ACEDB3)

/* for debug purpose only : PS_MODIFY */
extern int ps_debug;

extern void lexOverLoad (KEY key,DISK disk) ;
extern DISK lexDisk (KEY key) ;
BOOL sessionCheckUserName (BOOL doTalk);

#ifdef THINK_C
#include "unix.h"
#endif

#define BLKMX (BLOC_SIZE - sizeof(BLOCKHEADER) - sizeof(DISK)\
	       -sizeof(int) -sizeof(int) -sizeof(int))

typedef struct block /* sizeof(block) must be < BLOC_SIZE */
  { BLOCKHEADER  h ;
    DISK gAddress ;
    int mainRelease ,  subDataRelease, subCodeRelease ;
    char c[BLKMX] ;
  } BLOCK, *BLOCKP;   /*the transfer unit between disk and cache*/

#ifdef DEF_BP
  Fatal Error  Double definition of BP
#endif

typedef BLOCKP BP ;
#define DEF_BP

#include "session.h"
#include "a.h"
#include "bs.h"
#include "disk.h"
#include "display.h"
#include "query.h"
#include "lex.h"
#include "systags.h"
#include "sysclass.h"

static  int  writeAccess = 0 ;  /* Sets to gActive->user */
static void sessionFlipMenu(BOOL access) ;
static void sessionReleaseWriteAccess(void) ;
static BLOCK b ;
static BOOL suppressLock = FALSE ;
void sessionUser (void) ;
extern void readModels (void) ;

static Array sessionTree = 0 ;
static Stack stText = 0 ;

static BOOL showDeads =  FALSE ; 
static int  sessionFindSon (KEY father, KEY *sonp) ;
static void sessionTreeConstruct (void) ;

#ifndef NON_GRAPHIC
static Graph sessionSelectionGraph = 0 ;
void sessionControl(void) ;
#endif

SESSION  thisSession ;

typedef struct STstructure *STP ;
typedef struct STstructure 
 { KEY key, father, ancester ;
   STP sta ;
   BOOL isDestroyed, permanent ;
   int user, date, title, color ; 
   int generation , x, y, len , box ;
 } ST ;

/*************************************************************/

static BOOL I_own_the_process(void)
{
#if !defined(THINK_C) 
  return (getuid() == geteuid())  ;
#else
  return TRUE ;
#endif
}

/*******************  add 03.05.1992 kocab DKFZ  ****************/
/****************************************************************/
#ifdef IBM
 
struct passwd {
  char    *pw_name;
  char    *pw_passwd;
  long   pw_uid;
  long   pw_gid;
  char    *pw_gecos;
  char    *pw_dir;
  char    *pw_shell;
} ;
extern struct passwd *getpwuid (long uid) ;
extern struct passwd *getpwnam (const char *name) ;
#else
#include <pwd.h>
#endif /*IBM */

static char *getLogin (void)	/* can't fail */
{
#ifdef THINK_C
  return "acedb" ;
#else
  extern char *getlogin (void) ;
  static char *name = 0 ;
  
  if (!name)
    name = getlogin () ;
  
/* getlogin() == NULL if process not attached to terminal */

  if (!name)
    if (getpwuid(getuid()))
      name = getpwuid(getuid())->pw_name ;
    else
      name = "getLoginFailed" ;

  return name ;
#endif
}    

/*************************************************************/
/************** Disk Lock System *****************************/
/*************************************************************/

static BOOL showWriteLock (void)
{
  static char* logName = "Don't-know-who" ;
  int n = 0, level ;
  FILE* lockFile = 0;
    
  lockFile = filopen ("database/lock","wrm","r");
  if (!lockFile)
    return FALSE ;
    
  level = freesetfile (lockFile,"") ;
  if (freecard(level))
    { freenext() ;
      if (freeint(&n))
	{ freenext() ;
	  logName = freeword() ;
	}
    }
  freeclose (level) ;
  messout ("%s (session %d) already has write access", logName, n) ;
  return TRUE ;
}

/**************************/
 
static int lockFd = -1 ;	/* lock file descriptor */

static void releaseWriteLock (void)
{ if (lockFd == -1)
    messcrash ("Unbalanced call to releaseWriteLock") ;
 
  if (!suppressLock)
    if (lockf (lockFd, F_ULOCK, 0)) /* non-blocking */
      messcrash ("releaseWriteLock cannot unlock the lock file");
  close (lockFd) ;
  lockFd = -1 ;
}

/**************************/

static BOOL setWriteLock (void)
{    
  unsigned int size ;
  char *cp ;

  if (lockFd != -1)
    messcrash ("Unbalanced call to setWriteLock") ;

  lockFd = open (messprintf ("%s/database/lock.wrm", filsetdir(0)),
			     O_RDWR | O_CREAT | O_SYNC, 0644) ;
  if (lockFd == -1)
    { messerror ("I can't open or create the lock file "
                "%s/database/lock.wrm.  Maybe I do not have the "
                "correct permissions.", filsetdir(0)) ;
      return FALSE ;
    }
#if defined (LINUX)
 suppressLock = TRUE ; /* because F_TLOCK is lacking on linux */
#else
  if (!suppressLock)
    if (lockf (lockFd, F_TLOCK, 0)) /* non-blocking */
      { close (lockFd) ;
	lockFd = -1 ;
	if (!showWriteLock())
	  messout ("Sorry, I am unable to establish a lock on file database/lock.wrm"
		   "Read wspec/passwd.wrm for additional info") ;
	return FALSE ;
      }
#endif
  
  cp = messprintf ("%d %s", thisSession.session, getLogin()) ;
  size = strlen(cp) + 1 ;
  write (lockFd, cp, size) ;
  return TRUE;
}

/*************************************************************/
/****************************************************************/

static KEY sessionChosen = 0 ;

static void sessionStart(void)
{
  OBJ Session ;
  int a, b, c , h ;

  if (ps_debug)
  fprintf(stderr, "sessionStart ...\n");
  a = b = c = h = 0 ;
  Session = bsCreate(sessionChosen) ;
  if(!Session )
    { messout("Sorry, I cannot find %s",name(sessionChosen)) ;
      return ;
    }
  if(!bsGetData(Session,_VocLex,_Int,&a) ||
     !bsGetData(Session,_bsRight,_Int,&b)  ||
     !bsGetData(Session,_bsRight,_Int,&c)  ||
         /* Hasher only introduced in release 1.4 */
     /* I hope that the compiler is not too smart ! */
     ( !bsGetData(Session,_bsRight,_Int,&h) && FALSE)  ||
     !bsGetData(Session,_CodeRelease,
	    _Int,&thisSession.mainCodeRelease) ||
     !bsGetData(Session,_bsRight,
		_Int,&thisSession.subCodeRelease) ||
     !bsGetData(Session,_DataRelease,
	    _Int,&thisSession.mainDataRelease) ||
     !bsGetData(Session,_bsRight,
	    _Int,&thisSession.subDataRelease) ) 
    { messout("No address in this session") ;
      bsDestroy(Session) ;
      return ;
    }
  bsDestroy(Session) ;

  thisSession.from = sessionChosen ;
  thisSession.upLink = sessionChosen ;

  lexOverLoad(__lexi3,(DISK)a) ;
  lexOverLoad(__voc3,(DISK)c) ;

  lexOverLoad(__lexh3,(DISK)h) ;

  lexRead() ; 

  graphCleanUp() ;  /*  all but main  graph */
  sessionChosen = 0 ;
}

/*************************************************************/

void sessionAutoSelect(void)
{ 
  sessionChosen = lexLastKey(_VSession) ;

  if(!sessionChosen)
    messcrash("The session class is empty, Reconstruct, sorry") ;
  sessionStart() ;
}

/*************************************************************/

static BOOL sessionOedipe (void) /* Kill the father of the present session */
{ KEY father = thisSession.upLink, grandFather, fPlus, fMinus ;
  OBJ Father ; 
  Array fPlusArray, fMinusArray ;
  extern void diskFuseOedipeBats(Array fPlusArray, Array fMinusArray)  ;

   if(!father || !(Father = bsUpdate(father)))
    return FALSE ;
  if(!bsGetKey(Father,_BatPlus,&fPlus) ||
     !bsGetKey(Father,_BatMinus,&fMinus) ) 
    { messout("No address in this father") ;
      bsDestroy(Father) ;
      return FALSE ;
    }

  if(!bsFindTag(Father,_Permanent_session))
    { messout("The previous session being delared permanent, I keep it") ;
      bsDestroy(Father) ;
      return FALSE ;
    }

  if (bsGetKey(Father, _Up_linked_to, &grandFather))
    { thisSession.upLink = grandFather ;
      bsAddData(Father, _Destroyed_by_session, _Int, &thisSession.session) ;
      bsSave(Father) ;
    }
  else
   { thisSession.upLink = 0 ;
     bsDestroy(Father) ;
   }  
  
  fPlusArray = arrayGet(fPlus,unsigned char,"c") ;
  fMinusArray = arrayGet(fMinus,unsigned char,"c") ;

  diskFuseOedipeBats(fPlusArray, fMinusArray) ; 

  arrayDestroy(fPlusArray) ;
  arrayDestroy(fMinusArray) ;
              /* Kill the father bats */
  arrayKill(fPlus) ;
  arrayKill(fMinus) ;

  saveAll() ;
  return TRUE ;
}

/******************/

static BOOL sessionFuse(KEY father, KEY son) 
{ KEY fPlus, fMinus, sPlus, sMinus , grandFather ;
  OBJ Father = 0 , Son = 0 ; int n = 0 ;
  Array fPlusArray = 0, fMinusArray = 0, 
        sPlusArray = 0, sMinusArray = 0;
  extern int diskBatSet(Array bat) ;
  extern void diskFuseBats(Array fPlusArray, Array fMinusArray,
			   Array sPlusArray, Array sMinusArray) ;

  Father = bsUpdate(father) ;
  if(!bsGetKey(Father,_BatPlus,&fPlus) ||
     !bsGetKey(Father,_BatMinus,&fMinus) ||
      bsFindTag(Father, _Destroyed_by_session)) 
    { /* messout("Session fuse: No address in the father") ; */
      goto abort ;
    }

  fPlusArray = arrayGet(fPlus,unsigned char,"c") ;
  fMinusArray = arrayGet(fMinus,unsigned char,"c") ;

  if (son)
    { if (!(Son = bsUpdate(son)))
	messcrash("sessionFuse cannot bsupdate son = %s",
		  name(son) ) ;
      
      if( !bsGetKey(Son,_BatPlus,&sPlus) ||
	 !bsGetKey(Son,_BatMinus,&sMinus) )
	{ messout("Session fuse: No address in the son") ;
	  goto abort ;
	}
     
      sPlusArray = arrayGet(sPlus,unsigned char,"c") ;
      sMinusArray = arrayGet(sMinus,unsigned char,"c") ;
      
      if (!sPlusArray || ! sMinusArray)
	goto abort ;
      arrayKill(fPlus) ;  /* Ok iff present session is indeed a descendant */
      arrayKill(fMinus) ;

      diskFuseBats(fPlusArray, fMinusArray, sPlusArray, sMinusArray) ; 
      if (bsGetKey(Father, _Up_linked_to,&grandFather))
	bsAddKey(Son , _Up_linked_to, grandFather) ;
    }
  else
    diskFuseBats(fPlusArray, fMinusArray, 0, 0) ;

 
             /* Kill the father bats */
  bsAddData(Father, _Destroyed_by_session, _Int, &thisSession.session) ;
  bsSave(Father) ;
  

/* In principle one should:
         arrayKill(fPlus) ; 
         arrayKill(fMinus) ;
 * however, their disk block has already been allocated in father
 * and should only be freed by a direct descendant which is not necessarily the case.
 * Since in cache we did not arraySave the cache will emtpy itself automatically.
 * note however that it is crucial to saveAll between each sessionDestroy
 * because if i killed a's father before a then i will have done an arraySave(a's bat)
 */

  arrayDestroy(fPlusArray) ;
  arrayDestroy(fMinusArray) ;

            /* Store the sons bat */
  if (son)
    { bsAddKey(Son, _BatPlus, sPlus) ;   /* to reposition */
      n = diskBatSet(sPlusArray) ;
      bsAddData(Son, _bsRight, _Int, &n) ;
      bsAddKey(Son, _BatMinus, sMinus) ;
      n = diskBatSet(sMinusArray) ;
      bsAddData(Son, _bsRight, _Int, &n) ;
      bsSave(Son) ;
      
      arrayStore(sPlus, sPlusArray,"c") ;
      arrayStore(sMinus, sMinusArray,"c") ;
      arrayDestroy(sPlusArray) ;
      arrayDestroy(sMinusArray) ;
    }
   saveAll() ;
   return TRUE ;

 abort:
  bsSave(Son) ;
  bsSave(Father) ;
  return FALSE ;
}

/****************/

static BOOL sessionKillAncestors (int n) /* Kill my father level n */
{
  KEY father = thisSession.upLink, son ;
  ST *st ;
  int i ;

  for (i = 0, st = arrp(sessionTree,0,ST) ; i < arrayMax(sessionTree) ; st++, i++)
    if (st->key == father)
      break ;

  if (father && st->key != father) 
    { messerror ("sessionkillAncester did not find the father he wanted to murder.") ;
      return FALSE ;
    }
 
  while (st && --n)            /* Move up the ladder */
    st = st->sta ;
  if (!st || n)                /* Not many ancesters alive */
    return FALSE ;
  
  while (st && sessionFindSon(st->key, &son) != 1)
    st = st->sta ;            /* Try higher up */
    
  if (st && st->key && son)
    return sessionFuse(st->key, son) ;

  return FALSE ;
}

/*************************************************************/

void sessionInit(void)
{
  FILE * f ;
  extern void lexReadGlobalTables(void) ;
#ifndef NON_GRAPHIC
  BOOL oldGraphics ;
#endif  

  if (filexists ("database/ACEDB","wrm")) 
    {
      /* multi-partitions version : disk access initialisation : PS_MODIFY*/
      dataBaseAccessInit();
      diskblockread (&b,1) ;
      if (b.mainRelease != MAINCODERELEASE)
	if (getenv ("ACEDB_CHANGE_MAIN_RELEASE")) /* override */
	  b.mainRelease = MAINCODERELEASE ;
	else
	  messcrash
	    ("You are running release %d.%d of the ACEDB code.\n"
	     "The database was created under release %d.\n"
	     "Either use the correct binary or upgrade the database.\n",
	     MAINCODERELEASE, SUBCODERELEASE, b.mainRelease) ;
      thisSession.session = ++b.h.session ;
      thisSession.gAddress = b.gAddress ;
      lexOverLoad (__lexi1,b.gAddress) ;
      lexReadGlobalTables () ;
      return ;
    }
  
  /* else reinitialise the system */
  
  /* Only the acedb administrator is entitled to proceed
   * I check this by (a) getuid == geteuid
                     (b) trying to open passwd.wrm for writing
   * In the install documentation, it is stated that
   * passwd.wrm should be set chmod 644
   */
  
  f = 0 ;

  if (!I_own_the_process () || 
      !(f = filopen("wspec/passwd","wrm","a")))
    messcrash
      ("Sorry, the database seems empty.  Only the person"
       "owning the $(ACEDB) directory can reinitialise it.");
  filclose(f) ;
  
  if (!messQuery (
      messprintf ("The file %s/database/ACEDB.wrm is missing, "
		  "indicating that the database is empty.  "
		  "Should I reinitialise the system?", ACEDIR)) )
    messcrash ("Have a look at the documentation. Bye!") ;
  
  if (!sessionCheckUserName (TRUE))  /* Parses the eventual NOLOCK, Wolf May 1994 */
    messcrash ("Not authorized to initialize the database. Check passwd.wrm");

  writeAccess = TRUE  ; /* implied */
  thisSession.session = 1 ;
  strcpy (thisSession.name, "Empty_db") ;
  thisSession.gAddress = 0 ; /* so far */
  thisSession.mainCodeRelease = MAINCODERELEASE ; 
  thisSession.subCodeRelease = SUBCODERELEASE ; 
  thisSession.mainDataRelease = MAINCODERELEASE ; 
  thisSession.subDataRelease = 0 ; 
  thisSession.from = 0 ; 
  thisSession.upLink = 0 ; 

  if (!setWriteLock())
    messcrash ("Cannot set write lock!") ;

  b.h.disk = 1 ;
  b.h.nextdisk = 0 ;
  b.h.type = 'S' ;
  b.h.key  = 0 ;
  
#ifndef NON_GRAPHIC
  oldGraphics = isGraphics ; /* avoid to call twice the user */
  isGraphics = FALSE ;
#endif

  /* DataBase creation : PS_MODIFY*/
#if 0
  *********** old version with a single unix Database file : ********
  if (!(f = filopen ("database/blocks","wrm","r")))
    { /*  mainActivity ("I create the main database file") ; */
      if(!(f = filopen ("database/blocks","wrm","w")))
	messcrash ("Disk extend cannot open the main database file") ;
      fprintf (f, "Hello") ;
    }
  filclose (f) ;
  *******************************************************************
#endif
  if (ps_debug)
  fprintf(stderr, "Initial Session : Database Creation\n");
  if (!dataBaseCreate())
	messcrash ("DataBase Creation : Cannot create blocks partitions") ;

#ifndef NON_GRAPHIC
  isGraphics = oldGraphics ;
#endif

  messdump ("**********************************************************\n") ;
  messdump ("Making a new database file and starting at session 1 again\n") ;
  
  readModels () ;
  /* application specific initialisations, in quovadis.wrm */
  diskPrepare () ;
  sessionClose (TRUE) ;
  
  f = filopen ("database/ACEDB", "wrm", "w") ;
  fprintf (f, "Destroy this file to reinitialize the database\n") ;
  filclose (f) ;
  
  f = filopen ("database/lock", "wrm", "w") ;
  fprintf (f, "The lock file\n") ;
  filclose (f) ;
  
  sessionClose (TRUE) ;
}

/*************************************************************/
static KEY sessionKey = 0 ;
void sessionRegister(void)
{
  OBJ Session , Model = bsCreate(KEYMAKE(_VSession,0)) ;
  KEY key , kPlus, kMinus ;
  DISK d ;
  int free2, plus, minus, used ;
  char *cp ;

  if(!isWriteAccess())
    return ;

  if(!Model || 
     !bsFindTag(Model,_Up_linked_to)
     )  /* Always check on latest tag added to ?Session */
    { messerror("%s\n%s\n%s\n%s\n%s\n",
		"FATAL, an eror occured in sessionRegister",
		"Probably, your model for the session class is not correct",
		"and corresponds to an earlier release.",
		"Look for the latest version of the wspec/models.wrm file,",
		"read the model file and try again.") ;
      exit(1) ;
    }

  bsDestroy(Model) ;				 
				 
  lexaddkey(messprintf("s-%d-%s",
		       thisSession.session,thisSession.name),

	    &key, _VSession) ;
  sessionKey = key ;
  lexaddkey(messprintf("p-%d",thisSession.session),&kPlus,_VBat) ;
  lexaddkey(messprintf("m-%d",thisSession.session),&kMinus,_VBat) ;
  Session = bsUpdate(key) ;
  if(!Session)
    messcrash
      ("Session register fails to create the session object") ;

  if (thisSession.session == 1)
    bsAddTag(Session,_Permanent_session) ;
  cp = thisSession.name ;
  bsAddData(Session,_User,_Text,&cp) ;
  { char *buffer = messalloc(72) ;
    if(0 && timeStamp() && dateStamp() &&
       strlen(dateStamp()) + strlen(timeStamp()) < 70)
      { sprintf(buffer,"%s,  %s", timeStamp(), dateStamp()) ;
	bsAddData(Session,_Date,_Text,&buffer) ;
      }
    messfree(buffer) ;
  }
  if (*thisSession.title)
    bsAddData(Session,_Session_Title,_Text,thisSession.title) ;
  bsAddData(Session,_Session,_Int,&thisSession.session) ;
/*    if(messPrompt("Choose a title for this session","","t")
       && (cp = freeword()))
      bsAddData(Session,_Session_Title,_Text,cp) ;
*/
  if(thisSession.from)
    bsAddKey(Session,_Created_from,thisSession.from) ;
  if(thisSession.upLink)
      bsAddKey(Session,_Up_linked_to,thisSession.upLink) ;


  thisSession.mainCodeRelease = MAINCODERELEASE ; 
  thisSession.subCodeRelease = SUBCODERELEASE ; 
  bsAddData(Session,_CodeRelease,
	    _Int,&thisSession.mainCodeRelease) ;
  bsAddData(Session,_bsRight,
	    _Int,&thisSession.subCodeRelease) ;
  bsAddData(Session,_DataRelease,
	    _Int,&thisSession.mainDataRelease) ;
  bsAddData(Session,_bsRight,
	    _Int,&thisSession.subDataRelease) ;

  diskavail(&free2,&used,&plus, &minus) ;
  d = lexDisk(__Global_Bat) ;
  bsAddData(Session,_GlobalBat,_Int,&d) ; 
  bsAddData(Session,_bsRight,_Int,&used) ;

  d = lexDisk(__lexi1) ;
  bsAddData(Session,_GlobalLex,_Int,&d) ;

  d = lexDisk(__lexi2) ;
  bsAddData(Session,_SessionLex,_Int,&d) ;
  d = lexDisk(__lexa2) ;
  bsAddData(Session,_bsRight,_Int,&d) ;
  d = lexDisk(__voc2) ;
  bsAddData(Session,_bsRight,_Int,&d) ;
  d = lexDisk(__lexh2) ;
  bsAddData(Session,_bsRight,_Int,&d) ;

  d = lexDisk(__lexi3) ;
   bsAddData(Session,_VocLex,_Int,&d) ;
  d = lexDisk(__lexa3) ;
  bsAddData(Session,_bsRight,_Int,&d) ;
  d = lexDisk(__voc3) ;
  bsAddData(Session,_bsRight,_Int,&d) ;
  d = lexDisk(__lexh3) ;
  bsAddData(Session,_bsRight,_Int,&d) ;

  bsAddKey(Session,_BatPlus,kPlus) ;
  bsAddData(Session,_bsRight,_Int,&plus) ;

  bsAddKey(Session,_BatMinus,kMinus) ;
  bsAddData(Session,_bsRight,_Int,&minus) ;
  bsSave(Session) ;
}

/*************************************************************/
#define NFATHERS 2 /* Number of sessions kept alive, must be >= 1 */
void sessionClose (BOOL doSave)
{
  KEY son ;

  if (!isWriteAccess())
    return ;
  if (!doSave)
    { sessionReleaseWriteAccess() ;
      return ;
    }

  showDeads = FALSE ;  /* Locate father  in the session tree */
  if (thisSession.session != 1)
    sessionTreeConstruct() ;
   
  if (saveAll()) 
    {     /* I keep only NFATHERS session alive */
       if (thisSession.session != 1)
	 {
	   if (NFATHERS == 1)
	     { if (sessionFindSon(thisSession.upLink, &son) == 1)
		 sessionOedipe () ;
	     }
	   else
	     while (sessionKillAncestors(NFATHERS)) ;
	 }

                     /* increment session number */
      b.gAddress = thisSession.gAddress = lexDisk(__lexi1) ;
      b.mainRelease = MAINCODERELEASE ;
      b.subCodeRelease = SUBCODERELEASE ;
      thisSession.from =  sessionKey ;
      thisSession.upLink =  sessionKey ;
      b.h.session =  thisSession.session++ ;
      diskblockwrite(&b) ;
      diskWriteSuperBlock(&b) ;  /* writes and zeros the BATS */

      messdump ("SAVE SESSION %d: %s %s\n", 
		thisSession.session, dateStamp(), timeStamp()) ;
    }

  sessionReleaseWriteAccess()  ;
}

/*************************************************************/

BOOL isWriteAccess(void)
{
  return writeAccess != 0 ;
}

/*************************************************************/

static void sessionReleaseWriteAccess(void)
{
  if (!isWriteAccess())
    messcrash ("Unbalanced Get/Release Write Access") ;
  releaseWriteLock () ;
  writeAccess = 0 ;
  sessionFlipMenu (FALSE) ;
}

/*************************************************************/

BOOL sessionCheckUserName (BOOL doTalk)
{
  int level ;
  FILE *fil ; 
  register char *cp, *name ;

  Stack uStack = stackCreate(32) ;

/* Only users listed in wspec/passwd.wrm can get write access */

/* I cannot use cuserid, because I want the real userid and, 
   depending on systems, cuserid may return either the real or 
   the effective userid which is probably reset by setuid().
   
   On the other hand, Unix getlogin() is replaced by local 
   getLogin() because getlogin() returns NULL if the process 
   is not attached to a terminal */

  name = getLogin() ;
  strncpy (thisSession.name, getLogin(), 78) ;
  fil = filopen ("wspec/passwd","wrm","r") ;
  if (!fil)
    { messout ("Sorry, sessionWriteAccess cannot find wspec/passwd.wrm");
      return FALSE ;
    }
  level = freesetfile (fil, "") ;
  while (freecard (level))
    { freenext () ;
      if (!freestep ('#'))	/* skip # lines */
	if (cp = freeword())	/* read user name */
	  pushText(uStack,cp) ;
    }

  stackCursor(uStack, 0) ;
  while (!stackAtEnd (uStack))
    if (!strcmp ("NOLOCK", stackNextText(uStack)))
      suppressLock = TRUE ;

  stackCursor(uStack, 0) ;
  while (!stackAtEnd (uStack))
    if (!strcmp (name, stackNextText(uStack)))
      { stackDestroy (uStack) ;
	return TRUE ;
      }
  stackDestroy(uStack) ;

  /* failed to find the name */
  if (doTalk)
    if (!strcmp (name, "getLoginFailed"))
      messout ("On your particular machine, the Unix call \"getlogin\" failed.  "
	       "To circumvent this problem, you may uncomment the user name "
	       "\"getLoginFailed\" in the file wspec/passwd.wrm.  "
	       "But from then on, everybody will have write access unless you "
	       "set up other security measures using write permissions.") ;
    else
      messout ("%s is not a registered user.  To register a user for write",
	       "access add their login name to wspec/passwd.wrm", name) ;

  return FALSE ;
}

/*************************************************************/

static BOOL sessionDatabaseWritable (void)
{
  FILE *f1, *f2, *f3 ;
  
  if (!(f1 = filopen ("database/ACEDB","wrm","a")) ||
      !(f2 = filopen ("database/blocks","wrm","a")) ||
      !(f3 = filopen ("database/log","wrm","a")))
    { 
#ifdef THINK_C 
      messout ("Please create a folder called 'database' and start over");
#else   
      messout ("Sorry, the program does not have permission to rewrite "
	       "files in $(ACEDB)/database.  Does it exist?") ;
#endif
      return FALSE ;
    }
  filclose (f1) ; filclose (f2) ; filclose (f3) ;
  
  return TRUE ;
} 

/*************************************************************/

  /* If another process has written since I started
     either he has grabed the lock, and I can't write,
     or he may have released it but then he may have updated
     the disk.
     In this case, the session number on block 1 will be
     modified and I will know that my BAT is out of date
     */

static BOOL checkSessionNumber(void)
{
  BLOCK bb ;

  diskblockread (&bb,1) ;
  if (thisSession.gAddress != bb.gAddress)
    { messout ("Sorry, while you where reading the database, another process "
	       "updated it.  You can go on reading, but to write, you must "
	       "quit and rerun.") ;
      releaseWriteLock() ;
      return FALSE ;
    }
  return TRUE ;
}

/*************************************************************/

void sessionWriteAccess (void)	/* menu function */
{
  if (isWriteAccess() ||	/* security checks */
      !sessionDatabaseWritable() || 
      !sessionCheckUserName (TRUE) || 
      !setWriteLock() ||
      !checkSessionNumber ())	/* NB after setWriteLock() */
    return ;
  
  /* Success */
  strncpy (thisSession.name, getLogin(), 78) ;
  messdump ("Write access : User %s\n", getLogin()) ;
  sessionFlipMenu (TRUE) ;
  writeAccess = TRUE ;
}

/**************************************************************/

void sessionSaveAccess (void)	/* menu function */
{
  if (isWriteAccess())
    sessionClose (TRUE) ;
  else
    sessionWriteAccess () ; /* try to grab it */
}

/*************************************************************/

static void sessionFlipMenu(BOOL access)
{
#ifndef NON_GRAPHIC
  extern MENUOPT* mainMenu ;
  if (access)
    { menuSuppress ("Write Access", &mainMenu) ;
      menuRestore ("Save", &mainMenu) ;
    }
  else 
    { menuSuppress ("Save", &mainMenu) ;
      menuRestore ("Write Access", &mainMenu) ;
    }
#endif
}

/*************************************************************/
/****************************************************************/

static void sessionTreeConstruct(void)
{ KEY key = 0 , kk ; int n = 0 ; char *cp ;
  OBJ obj ;
  ST * st , *st1 ;
  int gen, i, j , max ;
  BOOL found ;

  sessionTree = arrayReCreate(sessionTree, 50,ST) ;
  stText = stackReCreate(stText, 50) ;
  pushText(stText,"dummy") ;

  
    while (lexNext(_VSession, &key))
      if (obj = bsCreate(key))
	{ st = arrayp(sessionTree, n++, ST) ;
	  st->key = key ;
	  if (bsGetKey(obj,_Created_from, &kk))
	    st->father = st->ancester = kk ;
	  if (bsGetKey(obj,_Up_linked_to, &kk))
	    st->ancester = kk ;
	  else if (st->key != KEYMAKE(_VSession, 1))
	    st->isDestroyed = TRUE ; /* i.e. to old a release */
	  if (bsFindTag(obj,_Destroyed_by_session))
	    st->isDestroyed = TRUE ;
	  if (bsFindTag(obj,_Permanent_session))
	    st->permanent = TRUE ;
	  if (bsGetData(obj,_User, _Text, &cp))
	      { st->user = stackMark(stText) ;
		pushText(stText,cp) ;
	      }
	  if (bsGetData(obj,_Date, _Text, &cp))
	      { st->date = stackMark(stText) ;
		pushText(stText,cp) ;
	      }
	  if (bsGetData(obj,_Session_Title, _Text, &cp))
	      { st->title = stackMark(stText) ;
		pushText(stText,cp) ;
	      }

	  bsDestroy(obj) ;
	}

  /* Add thisSession */
  st = arrayp(sessionTree, n++, ST) ;
  st->key = _This_session ;
  st->father = st->ancester = thisSession.from ;

  /* links the st */
  for (i = 0, st = arrp(sessionTree,0,ST) ; i < arrayMax(sessionTree) ; st++, i++)
    for (j = 0, st1 = arrp(sessionTree,0,ST) ; j < arrayMax(sessionTree) ; st1++, j++)
      if(st->key == st1->ancester)
	st1->sta = st ;

  /* Count generations */
  if (showDeads)
    { 
      for (i = 0, st = arrp(sessionTree,0,ST) ; i < arrayMax(sessionTree) ; st++, i++)
	if (!st->ancester)
	  st->generation = 1 ;
    }
  else
    {
      for (i = 0, st = arrp(sessionTree,0,ST) ; i < arrayMax(sessionTree) ; st++, i++)
	if ((!st->isDestroyed) && ( !st->ancester || (st->sta && st->sta->isDestroyed)) )
	  st->generation = 1 ;
    }
  gen = 1 ;
  max = arrayMax(sessionTree) ;
  found = TRUE ;
  while (found)
    { found = FALSE ;
      if (showDeads)
	{
	  for (i = 0, st = arrp(sessionTree,0,ST) ; i < arrayMax(sessionTree) ; st++, i++)
	    if (st->sta && st->sta->generation == gen)
	      { st->generation = gen + 1 ;
		found = TRUE ;
	      }
	}
      else
	{
	  for (i = 0, st = arrp(sessionTree,0,ST) ; i < arrayMax(sessionTree) ; st++, i++)
	    if (!st->isDestroyed && st->sta && st->sta->generation == gen)
	      { st->generation = gen + 1 ;
		found = TRUE ;
	      }
	}
      gen++ ;
    }
}
    
/************************************************************/  

static int sessionFindSon(KEY father, KEY *sonp)
{ int i , n = 0 ; ST *st, *sta ;
  if (!sessionTree)
    sessionTreeConstruct() ;

  *sonp = 0 ;             /* Suicide */
  if (father == _This_session)
    return -2 ;
  if (!father)           /* Trivial */
    return -3 ;
  if (father == KEYMAKE(_VSession, 1))
    return -6 ;

  for (i = 0, sta = arrp(sessionTree,0,ST) ; i < arrayMax(sessionTree) ; sta++, i++)
    if (sta->key == father)
      break ;

  if (sta->key != father)       /* not found error */
    return -5 ;
  
  if (sta->isDestroyed)          /* dead */
      return -3 ;
 
  for (i = 0, st = arrp(sessionTree,0,ST) ; i < arrayMax(sessionTree) ; st++, i++)
    if(!st->isDestroyed && st->sta == sta)
      { *sonp = st->key ;
	n++ ;
      }
  
  if (n == 1)            /* is st1 in my direct ancestors */
    { for (i = 0, st = arrp(sessionTree,0,ST) ; i < arrayMax(sessionTree) ; st++, i++)
	if (st->key == thisSession.from)
	  {
	    while (st)
	      { if (st == sta)
		  return 1 ;
		st = st->sta ;  
	      }
	    return -4 ;
	  }
      return -4 ;        /* not ancestor */
    }

  return n ;             /* number of sons */
}

/*************************************************************/
/**************** Interactive session control ****************/
/*************************************************************/

#ifndef NON_GRAPHIC

static void localDestroy(void)
{ 
  sessionSelectionGraph =  0 ;
  arrayDestroy(sessionTree) ;
  stackDestroy(stText) ;
}

/*************************************************************/

static void otherSessionStart(void)
{ extern void lexClear(void) ;

  if(!sessionChosen)
   { messout("First pick a session in the Session List") ;
     return ;
   }

  if (isWriteAccess())
    { messout("First save your previous work") ;
      return ;
    }
  
  sessionWriteAccess() ;
  if (!isWriteAccess())
    { messout("Sorrry, you do not have write access") ;
      return ;
    }

  graphCleanUp () ;  /* all but main graph */
  sessionClose (TRUE) ;   /* To empty all caches */
  
  arrayDestroy(sessionTree) ;
  lexClear() ;

  sessionStart() ;
}

/************************************************************/  

static void sessionPick(KEY box)
{ static int previousBox = 0 ;
  int i ;
  ST *st ;

  for (i = 0, st = arrp(sessionTree,0,ST) ; i < arrayMax(sessionTree) ; st++, i++)
    if (st->box == previousBox)
      graphBoxDraw(st->box, BLACK, st->color) ;
      	
  for (i = 0, st = arrp(sessionTree,0,ST) ; i < arrayMax(sessionTree) ; st++, i++)
    if (st->box == box)
      { if (!st->isDestroyed && st->key != _This_session)
	  sessionChosen = st->key ;
	else
	  sessionChosen = 0 ;
	graphBoxDraw(st->box, BLACK, RED) ;
	if (st->key != 1)
	  display(st->key, 0, 0) ;
      }
  previousBox = box ;
}

/************************************************************/  

static void sessionDestroy(void)
{ KEY son ;
  
  if (!isWriteAccess())
    { messout("Sorrry, you do not have write access") ;
      return ;
    }

  if (!sessionChosen)
    { messout ("First select a living session.") ;
      return ;
    }

  if (!iskeyold(sessionChosen))
    { messout ("This session no longer exists, sorry") ;
      return ;
    }

  /* I save once here to let the system a chance to crash if need be */
  saveAll() ; 
 
  if (messQuery("Destroy this session"))
    {  if (sessionChosen == thisSession.from)
	 { messout("Thou shall not kill thy own father, Oedipus") ;
	   return ;
	 }
      switch(sessionFindSon(sessionChosen, &son))
	{
        case -2:
	  messout("Thou shall not commit suicide") ;
	  return ;
	case -3:
	  messout ("This session no longer exists, sorry") ;
	  return ;
	case -4:
	  messout ("You can only kill your direct ancestors or an extremity") ;
	  return ;
	case -5:
	  messerror ("Sorry, sessionDestroy cannot locate this session") ;
	  return ;
	case -6:
	  messout ("You cannot kill the empty database") ;
	  return ;
	case 0: case 1:
	  sessionFuse(sessionChosen, son) ;
  /* This save will rewrite Session to disk without altering the BAT */
	  saveAll() ;
	  arrayDestroy(sessionTree) ;
	  sessionControl() ;
	  return ;
	  break ;
	default:
	  messout("This session has more than 1 son, i cannot kill it.") ;
	  return ;
	}
    }
}

/****************************************************************/

static void sessionRename(void)
{ OBJ Session ;
  char *cp ;

  if (!isWriteAccess())
    { messout("Sorrry, you do not have write access") ;
      return ;
    }
    
  if (!sessionChosen)
    { messout ("First select a session.") ;
      return ;
    }

  if (!iskeyold(sessionChosen))
    { messout ("This session no longer exists, sorry") ;
      return ;
    }

  /* I save once here to let the system a chance to crash if need be */
  saveAll() ; 
  Session = bsUpdate(sessionChosen) ;
  if (!bsFindTag(Session, _Up_linked_to))
    { messout("I cannot rename a session created before code 1-6") ;
      bsDestroy(Session) ;
      return ;
    }
  bsGetData(Session, _Session_Title,_Text,&cp) ;
  if (messPrompt("Rename this session and make it permanent",cp,"t") &&
      ( cp = freeword()) )
    { if(strlen(cp) > 80)
	*(cp + 80) = 0 ;
      bsAddTag(Session, _Permanent_session) ;
      bsAddData(Session, _Session_Title,_Text,cp) ;
    }
  bsSave(Session) ;
  /* This save will rewrite Session to disk without altering the BAT */
  saveAll() ;
}

/****************************************************************/
static void sessionDraw()  ;
static void sessionShowDeads (void)
{
  showDeads = showDeads ? FALSE : TRUE ;
  sessionTreeConstruct() ;
  sessionDraw() ;
}

/****************************************************************/

static MENUOPT
   sessionMenu[] = {
     graphDestroy,"Quit",
     help,"Help",
     otherSessionStart,"Start Session",
     sessionRename,"Rename Session",
     sessionDestroy,"Destroy Session",
     sessionShowDeads,"Invoke/Repulse the spirits",
     0,0
     } ;

/****************************************************************/

static void sessionDrawBranch(ST* st, Array x)
{ int i, g = st->generation , n = arrayMax(sessionTree) ;
  ST *st1 ;
  
  if(!array(x, g, int))
    array(x, g, int ) = 4 ;

         /* Draw self */
  st->box = graphBoxStart() ;
  st->x = array(x, g, int) ;
  st->y = 7 + 3*g ;
  graphText(name(st->key), st->x, st->y ) ;
  st->len =strlen(name(st->key)) ;
  array(x, g, int) += st->len + 5 ;

  graphBoxEnd() ;
  if(st->isDestroyed)  /* cannot pick ! */
    st->color = WHITE ;
  else
    st->color = YELLOW ;
  if (st->key == _This_session || st->key == KEYMAKE(_VSession, 1))
    st->color = LIGHTBLUE ;

  graphBoxDraw(st->box, BLACK, st->color) ;
  for (i = 0, st1 = arrp(sessionTree,0,ST) ; i < n ;
       st1++, i++)
    if (st->ancester == st1->key)
      graphLine(st1->x + st1->len/2, st1->y + 1.0, st->x + st->len/2 , st->y - .0) ;
  
               /* Recursion */
  for (i = 0, st1 = arrp(sessionTree,n - 1,ST) ; i < n ;
       st1--, i++)
     if (st1->sta == st)
       sessionDrawBranch(st1, x) ;
}

/****************************************************************/

static void sessionDraw() 
{ int i, max = 0 , maxGen = 0 , n ;
  ST *st ;
  Array maxX = arrayCreate(12, int) ;
  
  if(!sessionTree)
    sessionTreeConstruct() ;
  
  n = arrayMax(sessionTree) ;
  graphClear() ;
  graphPop() ;
  for (i = 0, st = arrp(sessionTree,n - 1,ST) ; i < n ; st--, i++)
    { if (st->generation == 1)
	sessionDrawBranch(st, maxX) ;
      if (st->generation > maxGen)
	maxGen = st->generation ;
    }

  for (i=0 ; i< arrayMax(maxX); i++)
    if (max < arr(maxX,i, int))
      max = arr(maxX,i,int) ;
  if (!max)
    max = 40 ;
 
  graphTextBounds (max + 3 , 7 + 3 * maxGen) ;
  graphButtons(sessionMenu, 2., 2., 40) ;
  graphRedraw() ;
  arrayDestroy(maxX) ;
}

/*************************************************************/

void sessionControl(void)
{ 
  sessionChosen = 0 ;

  if ( graphActivate(sessionSelectionGraph))
    graphPop() ;
  else
    sessionSelectionGraph =  displayCreate(DtSession) ;
  graphMenu(sessionMenu) ;

  graphRegister(DESTROY,localDestroy) ;
  graphRegister(RESIZE, sessionDraw) ;
  graphRegister(PICK,  sessionPick) ;
  sessionDraw() ;
}

#endif    /* NON_GRAPHIC */

/*************************************************************/
/*************************************************************/

#endif   /* ACEDB3 */
