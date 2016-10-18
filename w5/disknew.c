/*  File: disknew.c
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
 * HISTORY:
 * Last edited: Apr  9 14:38 2003 (edgrif)
 * * Sep 21 12:04 1999 (edgrif): Add sync write mode code for disk writes.
 * * May 10 10:22 1999 (edgrif): Altered DataBaseReadDefinition for Jean.
 * * Apr 16 16:08 1999 (fw): fixed very subtle bug where totalNbpartions
 *              was wrong, because the global variable wasn't reset
 *              probably cause for SANgc02258 and other diskextend bugs
 * * Oct 15 11:33 1998 (fw): included the output from
 *              messSysErrorText in messcrash texts
 *               resulting from disk operations or other OS calls
 * *       *       *        *        *        *
 *	-	extern uid_t euid, ruid moved to session.h
 * * Jun  3 12:56 1996 (rd)
 * * Apr 11 - 94 : multi-partitions version. (pierre Simondon)
 * * Jan 22 10:33 1992 (mieg): Modify bat and session obj en place.
 * Created: Wed Jan 22 10:33:50 1992 (mieg)
 * CVS info:   $Id: disknew.c,v 1.88 2003-04-09 14:06:32 edgrif Exp $
 *-------------------------------------------------------------------
 */

#ifdef ACEDB5
void compilersDoNotLike (void *emptyFiles) {return ; }

#else			/* ACEDB 4 only */

/***************************************************************/
/***************************************************************/
/**  File disksubs.c :                                        **/
/**  Handles the disk of the ACeDB program.                   **/
/***************************************************************/
/***************************************************************/
/*                                                             */
/*  Six routines are public :                                  */
/*     avail,prepare, alloc/free, blockread/blockwrite.        */
/*                                                             */
/*  A single long sequential file large enough to hold         */
/*  DISKSIZE blocks is "prepared" once and for all.            */
/*                                                             */
/*  "diskalloc", using the static BAT table, returns a DISK    */
/* value which is the number of the first free block following */
/*   This number is then stored in the lexique as lxi->diskaddr*/
/* and used, after multiplication by BLOC_SIZE as an offset    */
/* in "diskblockread/write", which use fseek.                  */
/*  "diskfree" returns a bloc to the BAT                       */
/*   diskavail gives disk statistics.                          */
/*                                                             */
/*   The BAT table handling is private.                        */
/*                                                             */
/***************************************************************/

#include <errno.h>
#include <wh/acedb.h>
#include <wh/aceio.h>
#include <wh/disk__.h>
#include <wh/disk.h>
#include <wh/session.h>
#include <wh/chrono.h>
#include <wh/a.h>
#include <whooks/sysclass.h>
#include <whooks/systags.h>
#include <wh/block.h>
#include <wh/lex.h>
#include <wh/lex_bl_.h>
#include <wh/byteswap.h>
#include <wh/bindex.h>
#include <wh/bs.h>			/* for cacheSaveAll() */
#include <wh/dbpath.h>

#if !defined(MACINTOSH)
#include <sys/types.h>
#else
#include "macfilemgrsubs.h"
#endif

extern void swapABlock(BP bp) ;
extern void swapBBlock(BP bp) ;

static void diskBATread(void);
static void diskBATwrite(void);
static void diskBATcompress(Array plusA, Array minusA) ;
static void diskExtend(BOOL zeroing) ;
static BOOL isBat(DISK d) ;

static int PartitionSize(char *partitionName);  /* in blocks */
/* base size in bytes and check accessibility to the data base : */
static int PhysicalBaseSize();
/* relative address of a block in a partition (in bytes) */
static long PartitionAddress(DISK d, int *partition);
/* full pathname of a partition : */
static char *PartitionName(int numPartition);
/* close all open partitions of the base : */
static void CloseDataBase();
/* creation and update database map : */
static BOOL DataBaseReadDefinition(BOOL new, BOOL skip);
static BOOL DataBaseUpdateMap(void);
static void readPartitionMap (void);

static char *diskGetErrorText (char,char);

int INITIALDISKSIZE = 512 ;  /* number of blocks on the disk,
			      * called from blocksubs */

static Array batArray  = 0 ;         /*block allocation table */
static Array plusArray  = 0 ;     /* new blocks in this session */
static Array minusArray  = 0 ;    /* blocked freed by this sesion */

static BOOL batIsModified = FALSE ;
static int blocksRead = 0, blocksWritten = 0;

/* syncWrite - indicates whether acedb should open database files for        */
/*             writing using the O_SYNC flag. This should ensure that acedb  */
/*             will be able to detect timeouts for writes (applies to NFS    */
/*             mounted databases especially).                                */
static BOOL syncWrite = FALSE ;

/* for debug purpose only : */
static int ps_debug  = 0 ;
static int ps_debug1 = 0 ; 

/* Database  splited into partitions */

/* partitions description must reside on a unique disk block : */


/* ATTENTION :
 * only a  multiple of INITIALDISKSIZE blocks can be used in each partition
 * See wh/disk.h , wspec/cachesize.wrm (DISK option)
 */

/* default maximum DataBase size in blocks */
#define MAX_DATABASE_SIZE 100 * 1024 

#define UNIX_FILE_SYSTEM 1
#define RAW_FILE_SYSTEM  2
#define LG_NAME          128


/* Partition: describes a database partition (i.e. one of the blockNN.wrm files) */
typedef struct
{
  int type;						    /* unix file system, raw, distant.. */
  char hostname[LG_NAME];				    /* machine on which is the partition */
  char fileSystem[LG_NAME];				    /* partition file system or "ACEDN if
							       relative */
  char fileName[LG_NAME];				    /* file name or "NONE" if not a file
							       system. */

  int  currentBlocks;					    /* current size in blocks of this partition. */
  int  maxBlocks;					    /* max. size possible in blocks for
							       this partition. */

  int debut ;						    /* total blocks in database up to the
							       end of _previous_ partition.*/
  int fin ;						    /* total blocks in database up to end
							       of this partition. */
} PARTITION;

static int 
  lastPartition_L ,		/* (i.e. number of active partitions) */
  totalNbPartitions_L;		/* number of partitions */

static Array pTable = 0 ;

/*  partition acces handles : (currently only "fd") .  */
/* current read/write cursor position in partitions */
static Array rPos = 0, wPos = 0 , fdRead = 0 , fdWrite = 0 ;


/*******************************************************************/
/* Partitions configuration : must be called on first session  :   */
/*******************************************************************/


/* read database definition and create static description. This description */
/* will be set for futher retrieval in the base itself                      */
BOOL dataBaseCreate()
{ 
  VoidRoutine previousCrashRoutine = messcrashroutine ;

  messcrashroutine = simpleCrashRoutine ;

  if (ps_debug) fprintf(stderr, "DataBaseCreate\n");

  pTable = arrayReCreate (pTable, 2, PARTITION) ;
  rPos = arrayReCreate (rPos, 2, myoff_t) ;
  wPos = arrayReCreate (wPos, 2, myoff_t) ;
  fdRead = arrayReCreate (fdRead, 2, int) ;
  fdWrite = arrayReCreate (fdWrite, 2, int) ;

  if (!DataBaseReadDefinition(TRUE, TRUE))
    messcrash (" DataBase creation : no description file found !!!\n");

  DataBaseUpdateMap() ;

  dataBaseAccessInit();

  messcrashroutine = previousCrashRoutine ;

  return TRUE;
}

/*******************************************************************/
/* must be called at session open.                                 */
/*******************************************************************/

void  dataBaseAccessInit()
{ 
  int i ;
  PARTITION *pp ;

  fdRead = arrayReCreate (fdRead, 2, int) ;
  fdWrite = arrayReCreate (fdWrite, 2, int) ;
  pTable = arrayReCreate (pTable, 2, PARTITION) ;

  /* get partitions from database map-file */
  readPartitionMap ();

  /* control Database access : */
  PhysicalBaseSize () ;

  for (i = 0; i <= lastPartition_L; i++) 
    {
      char *nam = PartitionName(i);
      pp = arrayp(pTable, i, PARTITION) ;

      pp->currentBlocks = PartitionSize(nam);
      messfree(nam);

      if (ps_debug)
	fprintf(stderr,
		"Partition %d:\n"
		"\ttype %d, f system %s, file %s\n"
		"\tcurrentBlocks: %d, maxBlocks: %d, debut: %d, fin: %d\n",
		i, pp->type, pp->fileSystem, pp->fileName, 
		pp->currentBlocks, pp->maxBlocks, pp->debut, pp->fin);
    }

  /* all partitions closed : */
  fdRead = arrayReCreate (fdRead, 2, int) ;
  fdWrite = arrayReCreate (fdWrite, 2, int) ;
  rPos = arrayReCreate (rPos, 2, myoff_t) ;
  wPos = arrayReCreate (wPos, 2, myoff_t) ;


  return;
} /* dataBaseAccessInit */

/**************************************************************/
    /* On first call during a given session, 
       any modified item
       is moved by blocksave to a new disk position, 
       this alters the lex and BAT but the change is not 
       registered on disk, hence the need for a sequence of
       write */

BOOL saveAll(void)
{
  extern BOOL blockCheck (void) ;
  int i = 0, j = 3 ;
  BOOL modif = FALSE ;
  VoidRoutine previousCrashRoutine = messcrashroutine ;

  /* ici, in regarde si une session est ouverte */
  if(!isWriteAccess())
    return FALSE ;

  messcrashroutine = simpleCrashRoutine;

  if (!blockCheck())
    messcrash ("Sorry - bad cache structure - in saveAll") ;

  messStatus ("Saving...");

  while(j--)
    { int oldBlocks = blocksWritten;
      if(batIsModified)
	{ j++ ;  /* Three additional loops for security ! */
	  modif = TRUE ;
	}
      diskBATwrite() ;
      
      cacheSaveAll();  /* Secondary Cache */
  
      bIndexSave() ;
      lexSave () ;                   /* Lexiques go to cache */
      blockSave(TRUE) ;                  /*  flush the cache */
      if(modif)
	sessionRegister() ;
      if (ps_debug)
	printf("saveAll: pass %d: wrote %d blocks\n", i++, 
	       blocksWritten - oldBlocks);
    }

  messcrashroutine = previousCrashRoutine ;

  return modif ;
} /* saveAll */

/**************************************************************/
             /* If no explicit call is made,
		when the first cache is full, blockalloc
		flushes a block, calling diskwrite calling
                BATread calling diskalloc with a resulting
		fatal knot in the global used blockend pointer
		*/

void diskPrepare(void)
{ if (thisSession.session > 1 )   /* Except during session 0/1 */
    diskBATread() ;               /* la BAT existe deja : */
  else 
    { batArray = arrayCreate(100,unsigned char) ;
      plusArray = arrayCreate(100,unsigned char) ;
      minusArray = arrayCreate(100,unsigned char) ;
      /* select first current partition : */
      lastPartition_L = 0;
      /* reset first partition file : */
      diskExtend(TRUE) ;
    }
}

/**************************************************************/
    /* query extends the size of the main database file */
static void diskExtend(BOOL zeroing)
{
#if defined(MACINTOSH)
  int blockfile;
#else
  FILE * blockfile;
#endif
  int i ;
  long dd, newdd; /*, dphysical ;*/
  int physicalBaseSize;
  int blockExtend;
  unsigned int un  = 1;
  unsigned int _64_b = 64 * BLOC_SIZE;
  void *emptybp = messalloc((int) _64_b); /* zeroing implied */
  BOOL newPartition;
  PARTITION *pp =  arrayp(pTable, lastPartition_L, PARTITION) ;
  VoidRoutine previousCrashRoutine = messcrashroutine ;
  char *attempt_text, *pnam;

  messcrashroutine = simpleCrashRoutine ;

  /* il faut une session ouverte */
  if(!isWriteAccess())
    messcrash("diskExtend() called without Write Access") ;

  chrono("diskExtend") ;

  /* l'extension porte sur la derniere partition : */
  /* on commence par une sequence solide : fermeture systematique de */
  /* toutes les partitions en cas d'extension :                      */
  CloseDataBase();
  
  blockExtend = INITIALDISKSIZE - (INITIALDISKSIZE % 64);
  
  if (zeroing) 
    {
      dd = 0 ; 
      newdd = blockExtend;
      newPartition = TRUE;
    }
  else
    {
      if(!batArray)
	messcrash("No batArray in diskextend(FALSE)") ;

      dd = 8 * arrayMax(batArray) ;
      if (ps_debug)
	fprintf(stderr, "diskExtend : current blocks number in bat = %ld\n",dd);
      
      /* compute new size of the base */
      newdd  = dd + blockExtend;  
      physicalBaseSize = PhysicalBaseSize() ;

      if (physicalBaseSize < dd)
	messcrash("Diskextend found a physical size shorter than expected");

      /* Add a new partition if the new size would overflow the current one. */
      if (newdd > pp->debut + pp->maxBlocks)
	{ 
	  lastPartition_L++ ;
	  if (lastPartition_L >= totalNbPartitions_L)
	    /* we need a partition which isn't yet available
	     * in pTable, so we need to restart and make sure
	     * database.wrm is bigger next time */
	    messExit("Can't extend database. "
		     "You need to add partitions.\n"
		     "Please edit wpsec/database.wrm"); 

	  pp = arrayp(pTable, lastPartition_L, PARTITION) ;
	  pp->debut = dd ;
	  pnam = PartitionName(lastPartition_L);
	  messdump ("Disk extend: Creating partition %d: %s",
		    lastPartition_L, pnam) ;
	  messfree(pnam);
	  /* create new partition : 
	   * go to the end of the partition, 
	   *            extend it with the new extend size 
	   * (initialize by chunks of 64 blocks) 
	   */
	  newPartition = TRUE;
	}
      else
	newPartition = FALSE;
    }
  pnam = PartitionName(lastPartition_L);
  if (!newPartition)
    {
      attempt_text = "open (for appending)";

#if defined(MACINTOSH)
      blockfile = FMfopen( (unsigned char*) pnam,
			   (unsigned char*) "ab");
#else
      seteuid (euid);
      blockfile = fopen(pnam, "ab");
#endif
    }
  else
    {
      attempt_text = "open (for writing)";

#if defined(MACINTOSH)
      blockfile = FMfopen( (unsigned char*) pnam, (unsigned char*) "wb") ;
#else
      seteuid (euid);
      blockfile = fopen(pnam, "wb") ;
#endif
      pp->currentBlocks = 0;
      pp->fin = 0 ;
    }


  if (!blockfile)
    {
      messExit ("Error occured when trying to %s partition file %s (%s)\n"
		"%s"
		"I will have to exit now ...",
		attempt_text, pnam,
		messSysErrorText(), diskGetErrorText('o','w')) ;
    }

  messfree(pnam);
  
  i = (63 + blockExtend)/64 ;
#if defined(MACINTOSH)
  if (i > 0)
    {
      int writeLength ;
      writeLength = FMPBwrite(blockfile, 0, fsFromLEOF, emptybp, _64_b * i ) ;
      if ( writeLength != ( _64_b * i ) )
	messExit ("Error occured when writing to disk (%s)\n"
		  "%s"
		  "I will have to exit now ...",
		  messSysErrorText(), diskGetErrorText('a','w'));
    }
  FMfilclose ( blockfile ) ;
#else  /* UNIX */
  if (i > 0)
    {
      fseek(blockfile, 0L, SEEK_END) ;
      while (i--)
	if (fwrite (emptybp, _64_b, un,  blockfile) != un)
	  {
	    messExit ("Error occured writing to disk (%s)\n"
		      "%s"
		      "I will have to exit now ...",
		      messSysErrorText(), diskGetErrorText('a','w'));
	  }
    }
  fclose (blockfile) ;
  seteuid(ruid);
#endif /* UNIX */
    
  pp->currentBlocks += blockExtend;
  if ( pp->currentBlocks > 50000)
    invokeDebugger() ;
  pp->fin += blockExtend;

  /* write new database map-file, because some partition-records 
   * may have changed if new partitions have been created */
  DataBaseUpdateMap () ;

  messdump ("Now %ld blocks, %d in partition %s",
	    newdd,  pp->currentBlocks , pp->fileName) ;
  
  /* update bats : */
  for(i= dd/8; i<newdd/8 ;i++)
    { array(batArray,i,unsigned char) = 0;
      array(plusArray,i,unsigned char) = 0;
      array(minusArray,i,unsigned char) = 0;
    }
  /* first 3 blocks reserved : */
  array(batArray,0,unsigned char) |= 7 ;
  batIsModified = TRUE ;
  
  messfree(emptybp);
  chronoReturn() ;

  messcrashroutine = previousCrashRoutine ;

  return;
} /* diskExtend */

/**************************************************************/
/*************************************************************/
   /* Called from session WriteSuperBlock */  
void diskWriteSuperBlock(BP bp)
{
  register int i ;
  register unsigned char * p, *m ;
  if (ps_debug) fprintf(stderr, "diskWriteSuperBlock\n");

  CloseDataBase();

  /*  isWriteAccess() is  implied by the call */
  diskblockwrite(bp) ;     /* reopens file */

  CloseDataBase();

  /*  batReinitialise */
  i = arrayMax(plusArray) ;
  p = arrp(plusArray,0,unsigned char) ;
  m = arrp(minusArray,0,unsigned char) ;
  while(i--) {
    /* We can use only one of *p++ = *m++ or array(...) */
      *p++ = *m++ = 0 ;
      array(plusArray,i,unsigned char) = 0;
      array(minusArray,i,unsigned char) = 0;
    }

  return;
} /* diskWriteSuperBlock */

void diskFlushLocalCache(void)
/* call CloseDataBase to flush any client side cached copy of the blocks,
   needed to close a concurrency hole in session.c */
{
  CloseDataBase();
}

/**************************************************************/
/*              lecture de la table des blocs                 */
/**************************************************************/

static void diskBATread(void)
  {
  int i = INITIALDISKSIZE/8 ;
  int j,k ;
  VoidRoutine previousCrashRoutine = messcrashroutine ;

  /* le cas ou on a deja la bat */
  if (batArray) 
    return ;

  messcrashroutine = simpleCrashRoutine ;

  /* acces au bloc contenant la bat */
  batArray = arrayGet(__Global_Bat,unsigned char,"c") ;
  if(!batArray || !arrayMax(batArray))
    {
      messcrash("diskBATread() - Cannot arrayGet the Global_Bat");
    }

  i = arrayMax(batArray) ;
  /* on cree les bat plus et moins , de meme taille */
  plusArray = arrayCreate(i,unsigned char) ;
  minusArray = arrayCreate(i,unsigned char) ;
  arrayMax(minusArray) = arrayMax(plusArray) = i;
  
  /* DISK address 0 1 2 are for the system */
  array(batArray,0,unsigned char) |= 7;     
  j = lexDisk(__Global_Bat) ;
  k = 1 << (j%8); j = j/8;
  if(array(batArray,j,unsigned char) & k)
   ; 
/*    messdump("\n Correct bat self referenced address %d, session %d",
	   j, KEY2LEX(__Global_Bat)->addr ?
 KEY2LEX(__Global_Bat)->addr->p->h.session : 0);
*/
  else 
    messcrash("no bat self reference for address %d",j);

  messcrashroutine = previousCrashRoutine ;

  return;
} /* diskBATread */

/********************************************/

static void diskBATwrite(void)
{ KEY kPlus, kMinus ;
  if (!batArray || ! batIsModified) 
    return ;
  batIsModified = FALSE ; /* must come before actual write */ 
  if(ps_debug)
    printf("\nWriting BAT, address %d", 
	   KEY2LEX(__Global_Bat)->dlx.dk) ;

  lexaddkey(messprintf("p-%d",thisSession.session),&kPlus,_VBat) ;
  lexaddkey(messprintf("m-%d",thisSession.session),&kMinus,_VBat) ;
  arrayStore(__Global_Bat, batArray,"c") ;
  arrayStore(kPlus, plusArray,"c") ;
  arrayStore(kMinus, minusArray,"c") ;

  return;
} /* diskBATwrite */

/********************************************/
   /* compress block which have been allocated and freed
      during the same session, i.e. necessary after a batFusion
      */
static void diskBATcompress(Array plusA, Array minusA)
{
  register int i;
  register unsigned char *plus, *minus, *bat , c ;
  VoidRoutine previousCrashRoutine = messcrashroutine ;

  messcrashroutine = simpleCrashRoutine ;

  bat = arrp(batArray,0,unsigned char) ;
  plus = arrp(plusA,0,unsigned char) ;
  minus = arrp(minusA,0,unsigned char) ;

  if (arrayMax(batArray) < arrayMax(plusA))
    messcrash("bat size inconsistency in diskBATcompress") ;
  i = arrayMax(plusA) ;
  while(i--)
    { c = *minus & *plus ;  /* reset c bits to zero */
      if(c)
	{ 
	  batIsModified = TRUE ;
	  if ( (*bat & c) != c )
	    messcrash("diskBatCompress releasing a non set block %c %c",
		      *bat , c ) ;
	  *bat &= ~c ;  /* tilda NOT c) */
	  *plus &= ~c ;   /* tilda NOT c) */
	  *minus &= ~c ;  /* tilda NOT c) */
	}
      bat++ ; plus++ ; minus++ ;
    }

  messcrashroutine = previousCrashRoutine ;

  return;
} /* diskBATcompress */

/********************************************/

static BOOL fuseBats(Array old, Array new)
{
  register int i;
  register unsigned char *o , *n ;
  BOOL result = FALSE ;
  VoidRoutine previousCrashRoutine = messcrashroutine ;

  if( !arrayExists(old) || !arrayExists(new) ) 
    return FALSE ;

  messcrashroutine = simpleCrashRoutine ;

  o = arrp(old,0,unsigned char) ;
  n = arrp(new,0,unsigned char) ;

  if (arrayMax(old) > arrayMax(new))
    messcrash("Inconsistency in fuseBats") ;

  i = arrayMax(old) ; o--; n-- ;
  while(o++, n++, i--)
    if (*o)
      { if (*o & *n)
	  messcrash("Duplication in fuseBats") ;
	result = TRUE ;
	*n |= *o ;
      }

  messcrashroutine = previousCrashRoutine ;

  return result ;
} /* fuseBats */

/**************************************************************/
static int diskBatSet(Array a) ;

BOOL  diskFuseBats(KEY fPlus, KEY fMinus, KEY sPlus, KEY sMinus,
		  int *nPlusp, int *nMinusp)
{
  BOOL done = FALSE, modif = FALSE ;
  Array fPlusArray = 0, fMinusArray = 0, 
        sPlusArray = 0, sMinusArray = 0;

  fPlusArray = arrayGet(fPlus,unsigned char,"c") ;
  fMinusArray = arrayGet(fMinus,unsigned char,"c") ;
  sPlusArray = arrayGet(sPlus,unsigned char,"c") ;
  sMinusArray = arrayGet(sMinus,unsigned char,"c") ;
  
  if (!sPlusArray || !sMinusArray ||
      !fPlusArray || !fMinusArray)
    goto abort ;
      
  modif |= fuseBats(fPlusArray, sPlusArray) ;
  modif |= fuseBats(fMinusArray, sMinusArray) ;

  if(modif)
    diskBATcompress(sPlusArray, sMinusArray) ;
  *nPlusp = diskBatSet(sPlusArray) ;
  *nMinusp = diskBatSet(sMinusArray) ;

  arrayStore(sPlus, sPlusArray,"c") ;
  arrayStore(sMinus, sMinusArray,"c") ;

  arrayKill(fPlus) ;  /* not valid if there were session branches */
  arrayKill(fMinus) ; 

  done = TRUE ;
abort:
  arrayDestroy(sPlusArray) ;
  arrayDestroy(sMinusArray) ;
  arrayDestroy(fPlusArray) ;
  arrayDestroy(fMinusArray) ;

  return done ;
}

/**************************************************************/

void diskFuseOedipeBats(Array fPlusArray, Array fMinusArray) 
{ 
  BOOL modif = FALSE ;
    
  modif |= fuseBats(fPlusArray, plusArray) ;
  modif |= fuseBats(fMinusArray, minusArray) ;
  
  if(modif)
     diskBATcompress(plusArray, minusArray) ;
}

/********************************************/

                  /* Returns the number of set bits */
static int diskBatSet(Array a)
{
  register unsigned char *cp ;
  register unsigned int i;
  register int j = arrayExists(a) ? arrayMax(a) : 0 , n = 0 ;

  if(!j)
    return 0 ;
  cp  = arrp(a,0,unsigned char) ;
  while (j--)
    { if(!*cp) n+=8 ;
      else if(*cp != 0xff)
	for (i=1;i<256;i<<=1) if(~(*cp) & i) n++;
      cp++;
    }
  return 8*arrayMax(a) - n ;
}

/********************************************/

                  /*returns the number of used and free blocks*/
void diskavail(int *free,int *used, int *plus, int *minus, int *dr, int* dw)
{
  if(!batArray)
    diskBATread();

  *used = diskBatSet(batArray) ;
  *plus = diskBatSet(plusArray) ;
  *minus = diskBatSet(minusArray) ;
  *free = 8*arrayMax(batArray) - *used ;
  *dr = blocksRead ; *dw = blocksWritten ;
  if (getenv ("ACEDB_DEBUG"))
    messdump ("diskavail: used %d, plus %d, minus %d, free %d", 
	      *used, *plus, *minus, *free) ;
}

/********************************************/
   /* The strategy is always to move blocks on disk when rewriting
      and never reuse a block during a given session.
      In this way, the former session certainly remains consistent
      
      There is however a very important exception to this rule,
      The session objects and the Bat arrays are not moved. The reason 
      is that during session 8 you may be fusing session 3 and 4,
      then you modify obj session4 and its bats. By doing it en place
      you insure that it is written in a block allocated during session 4.
      Otherwise, if you eventually destroy session 8 before session 4, the
      bat of session 8 would be lost.
      */
void diskalloc(BP bp)
{
  register unsigned int i ;
      /*unsigned because we use a right shift operator on i*/
  register int j ;
  register unsigned  char *cp , *top, *end , minus ;
  static DISK d = 0;
  
  if(bp->h.disk && 
     ( bp->h.session == thisSession.session ||
      class(bp->h.key) == _VBat ||
      class(bp->h.key) == _VSession 
      ) )
     return;
  
  if(!batArray)
    diskBATread() ;
                /* We move the block on disk */
  if (class(bp->h.key) != _VGlobal) /* Freeing the global class in unconsistent */
    diskfree(bp) ;
  
 lao:
  j = arrayMax(batArray) ;   /* must come after eventual BATread */
  top = arrp(batArray,0,unsigned char) ;
  end = arrp(batArray,arrayMax(batArray) - 1 ,unsigned char) + 1 ;
  cp = arrp(batArray,(int) (d/8),unsigned char) ;
  if (cp >= end)
    cp = top ;
                
  while (j--) /* explore the whole BAT starting in the middle */
    {
      if(*cp != 0xff)
	for (i=1;i<256;i<<=1)
	  if(~(*cp) & i)  /* free block on the global BAT */
	    { d = (DISK)(8*(cp - top)) ;
              minus = arr(minusArray,(int)(d/8),unsigned char)  ;
	      if(~minus & i)	   /* Not freed in this session */
		{ *cp |= (unsigned char)i ;
		  arr(plusArray,(int)(d/8),unsigned char) |= i ; 
		  while(i>>=1) d++;
		  bp->h.disk=d ;
		  bp->h.session = thisSession.session ;
		  batIsModified = TRUE ;
		  return ;
		}
	      else
		{
		  messcrashroutine = simpleCrashRoutine ;
		  messcrash ("disk alloc inconsistency") ;
		}
	    }
      cp++;
      if (cp >= end) 
	cp = top;
    }
  
  diskExtend(FALSE);
  goto lao ;
} /* diskalloc */

/*************************************************/
/* do free only if allocated during this session */
/*************************************************/

void diskfree(BP bp)
{
  register unsigned char i;  /*unsigned because we use a right shift operator on i*/
  register unsigned char *cp , *minus, *plus ;
  register int j ;
  DISK d = bp->h.disk ;

  bp->h.disk = 0 ; /* prevents recursion */

  if( !d)
    return ;

  chrono("diskfree") ;
  
  if(!batArray)
    diskBATread() ;
  
  j = arrayMax(batArray) ;   /* must come after eventual BATread */
  if ((d >= 8*j) || (d < 0)) 
    { 
      messcrashroutine = simpleCrashRoutine ;
      messcrash("diskfree called with a wrong disk address");
    }

  cp = arrp(batArray,(int) (d/8),unsigned char) ;
  minus = arrp(minusArray,(int) (d/8),unsigned char) ;
  plus = arrp(plusArray,(int) (d/8),unsigned char) ;
  
  i = 1 << (d%8);
  if (! (*cp & i))
    {
      messcrashroutine = simpleCrashRoutine ;
      messcrash("diskfree called on free block ");
    }

  /* on peut oublier les blocs crees pendant la session */
  if(*plus & i)  /* allocated during this session */
    { (*cp) &= ~i;     /* free it */
      (*plus) &= ~i;     /* free it */
    }
  else
    (*minus) |= i ;
  batIsModified = TRUE ;
  chronoReturn() ;

  return;
} /* diskfree */

/********************************************/


static BOOL isBat(DISK d)
{
  register unsigned char i;  /*unsigned because we use a right shift operator on i*/
  register unsigned char *cp ;
  register int j ;
  
  if(!batArray)
    return TRUE ; /* during init, one does a few direct address reads */
  
  j = arrayMax(batArray) ;   /* must come after eventual BATread */
  if ((d >= 8*j) || (d < 0)) 
    /* hors limite de la bat pour ce bloc */
    {
      messcrashroutine = simpleCrashRoutine ;
      messcrash("diskfree called with a wrong disk address");
    }

  cp = arrp(batArray,(int) (d/8),unsigned char) ;
  i = 1 << (d % 8);
  if (*cp & i)
     return TRUE ;

  return FALSE ;
} /* isBat */


/* This could be an option that was turned on/off interactively, but since   */
/* sync mode writing is at least 5x slower, the option is currently set from */
/* session.c via an entry in passwd.wrm. It's the sort of thing a database   */
/* administrator should set when there are NFS problems.                     */
void diskSetSyncWrite(void)
{
  syncWrite = TRUE ;
}

/**************************************************************/
/* to write a block to disk   : return 0 if success */
/*         called by blockunload  */
void diskblockwrite(BP bp)
{
  char *vp=(char *)bp;
  long adrPartition;
  int partition;
  unsigned int nb,size=BLOC_SIZE;
  myoff_t pos,pp ;
  long d = bp->h.disk ; 
  long delta;
  int fd, nretry = 0 ;
  VoidRoutine previousCrashRoutine = messcrashroutine ;

  if(!isWriteAccess())
    return ;

  messcrashroutine = simpleCrashRoutine ;

  chrono("diskblockwrite") ;
  blocksWritten++;

  if(!batArray)
    diskBATread() ;
  
  if ((d < 0)|| (d >= 8 * arrayMax(batArray)))
    messcrash("Diskblockwrite called with wrong DISK address d=%ld, max = %d",
	      d, 8 * arrayMax(batArray)); 
  
  /* select partition and block address on partition : */
  adrPartition = PartitionAddress( d, &partition);
  if (adrPartition < 0) 
    messcrash(" Bad Block Address in partition "); 
  /* open partition if not yet */
  fd = array (fdWrite, partition, int) - 1 ;
  if (fd < 0) 
    {
      char* partName = PartitionName(partition) ;
#if defined(MACINTOSH)
      fd = FMPBopen ( (unsigned char *)partName, O_RDWR | O_BINARY ) ;
#else  /* UNIX */
      int mode = O_WRONLY | O_BINARY ;

      seteuid(euid);

      /* NFS can time out without the code being able to detect this, but if */
      /* we open the file for synchronous writing we should be able to detect*/
      /* this because write calls do not return until data is written to     */
      /* disk.     
                                                          */
#if !defined(DARWIN)
      if (syncWrite)
	mode |= O_SYNC ;
#endif

      fd = open(partName, mode, 0);
      seteuid(ruid);
#endif /* UNIX */
      if (fd < 0)
	messExit ("Error occured when trying to open (for read/write) partition file %s (%s)\n"
		  "%s"
		  "I will have to exit now ...",
		  partName, 
		  messSysErrorText(), diskGetErrorText('o','w')); 


      array (fdWrite, partition, int) = fd + 1 ;   /* i shift, so that 0 means not open, and arrayExtends is ok */
      array (wPos, partition, myoff_t) = 0 ;
      messfree(partName);
    }

  if (fd < 0) messcrash("diskblockwrite() - fd < 0");

  pos     = (myoff_t) (adrPartition *(long)size ) ;
  delta = pos -  array (wPos,partition,myoff_t) ;

  if (ps_debug1) 
    fprintf(stderr, "diskblockwrite: d %ld, part %d adr part %d offset %ld\n",
	    (long)d, (int)partition, (int)adrPartition, (long)pos);

#ifdef ACEDB4
  { static BLOCK copyBlock;
    if (swapData)
      { memcpy(&copyBlock, bp, sizeof(BLOCK));
	vp = (char *)&copyBlock;
	
	switch (bp->h.type) /* type must be invariant internal->external */
	  { 
	  case 'S':
	    swapSuperBlock(&copyBlock);
	    break;
	    
	  case 'A': case 'C':
	    swapABlock(&copyBlock);
	    break;
	    
	  case 'B':
	    swapBBlock(&copyBlock);
	    break;
	    
	  default:
	    messcrash("unknown block type trying to swap data"); 
	  }
      }
  }
#endif /* ACEDB4  */

#if defined(MACINTOSH)
  nb = FMPBwrite (fd, (myoff_t)delta, fsFromMark, vp, size ); 
  if ( nb != size )
    messcrash("Diskblockwrite cannot actually write the DISK,n=%d (%s)\n",
	      nb, messSysErrorText()); 
#else
  seteuid(euid);

  if (delta)
    if ( (pp = lseek(fd, delta, SEEK_CUR))!= pos )
      messExit("Error occured when trying to access (for seek) partition file %s at disk address %d (should be pos=%ld, returned=%ld)\n"
	       "(%s)\n"
	       "I will have to exit now ...",
		PartitionName(partition), d, (long) pos, (long)pp, messSysErrorText());

  while ( (nb = write(fd, vp, size)) != size )
    if (nretry++ == 5)
      messExit ("Error occured when trying to access (for writing) partition file %s at disk address %d (%s)\n"
		"%s"
		"I will have to exit now ...",
		PartitionName(partition), d, messSysErrorText(), diskGetErrorText('a','w'));

  seteuid(ruid);
#endif
  /* update current position : */
  array (wPos, partition, myoff_t) = pos + (long)nb;


  chronoReturn() ;

  messcrashroutine = previousCrashRoutine ;

  return;
} /* diskblockwrite */

/**************************************************************/
             /* to read a block from disk   : return 0 if success */
             /*     called by blockload */

void diskblockread (BLOCK *bp, DISK d)
{
  char *vp=(char *)bp;
  long adrPartition;
  long delta;
  int partition;
  unsigned int size=BLOC_SIZE;
  myoff_t pos,pp ;
  int n, nretry = 0, fd ;
  VoidRoutine previousCrashRoutine = messcrashroutine ;

  messcrashroutine = simpleCrashRoutine ;

  chrono("diskblockread") ;
  blocksRead++ ;

  /* select partition and block address on partition : */
  adrPartition = PartitionAddress( d, &partition);
  if (adrPartition < 0) 
    messcrash(" Bad Block Address in partition ");
  /* open partition if not yet */
  fd = array (fdRead, partition, int) - 1 ;
  if (fd < 0) 
    {
      char *partName = PartitionName(partition);
#if defined(MACINTOSH)
      fd = FMPBopen( (unsigned char*) partName, O_RDONLY | O_BINARY );
#else
      fd = open(partName, O_RDONLY | O_BINARY ,0);
#endif
      if (fd < 0)
	messExit ("Error occured when trying to open (for reading) partition file %s (%s)\n"
		  "%s"
		  "I will have to exit now ...",
		  partName, 
		  messSysErrorText(), diskGetErrorText('o','r'));

      array (fdRead, partition, int) = fd + 1 ;
      array (rPos, partition, myoff_t) = 0 ;
      messfree(partName);
    }
  
  if (fd < 0) messcrash("diskblockread() - fd < 0");

  pos = (myoff_t)(adrPartition * (long)size) ; 
  delta = pos - array(rPos, partition, myoff_t) ;
  
  if (ps_debug1)
    fprintf(stderr, "Diskblockread : d %ld, part %d adr part %d offset %ld\n",
	    (long)d, (int)partition, (int)adrPartition, (long)pos);
  
#if defined(MACINTOSH)
  
  n = FMPBread (fd, (myoff_t)delta, fsFromMark, vp, size ); 
  if ( n != size )
    messcrash("Diskblockread : cannot actually read the DISK,n=%d (%s)\n",
	      n, messSysErrorText());

  /* update current position : */
  array(rPos, partition, myoff_t) = pos + size ;

#else

  if (delta)
    if ( (pp = lseek(fd, delta, SEEK_CUR)) != pos )
      messExit("Error occured when trying to access (for seek) partition file %s at disk address %d (should be pos=%ld, returned=%ld)\n"
	       "(%s)\n"
	       "I will have to exit now ...",
		PartitionName(partition), d, (long) pos, (long)pp, messSysErrorText());
  
  while( (n = read(fd, vp, size)) != size )
    if (nretry++ == 5)
      messExit ("Error occured when trying to access (for reading) partition file %s (%s)\n"
		"%s"
		"I will have to exit now...",
		PartitionName(partition),
		messSysErrorText(), diskGetErrorText('a','r'));
  
  /* update current position : */
  array (rPos, partition, myoff_t) = pos + size ;

#endif

#ifdef ACEDB4
  if (swapData)
    switch (bp->h.type) /* type must be invariant internal->external */
      { 
	
      case 'S':
	break;   /* Don't swap superblock, we need to read it before swapData
		    is valid */
      case 'A': case 'C':
	swapABlock(bp);
	break;
	
      case 'B':
	swapBBlock(bp);
	break;
	
      default:
	messcrash("Diskblockread : unknown block type trying to transform data");
      }

#endif /* ACEDB4  */
  if (bp->h.type != 'S' && d != *(DISK*)bp)
    messcrash("Diskblockread : read a block not matching its address");

  if(!isBat(d))
    messerror("Diskblockread : read a block not registered in the BAT") ;

  chronoReturn() ; 

  messcrashroutine = previousCrashRoutine ;

  return;
} /* diskblockread */


/****************************************************************/
/* give partition size in #blocks number                        */
/****************************************************************/

static int PartitionSize(char *partitionName)
{ 
  long size ;
  int iSize ;
  int fd;
  VoidRoutine previousCrashRoutine = messcrashroutine ;

  messcrashroutine = simpleCrashRoutine ;

#if defined(MACINTOSH)
  if ( ! ( fd = FMfilopen ( (unsigned char*) partitionName,
                            (unsigned char*) "",
                            (unsigned char*) "r" ) ) )
    messcrash("\nPartitionSize : partition %s unreachable (%s)",
	      partitionName, messSysErrorText());
  size = FMfilgeteof ( fd );
  if (size < 0)
    messcrash ("PartitionSize : negative size %ld found for partition %s (%s)",
	       partitionName, messSysErrorText()) ;
  FMfilclose( fd );
#else  /* UNIX */
  fd = open (partitionName, O_RDONLY);
  if (fd < 0)
    {
      messExit ("Error occurred when trying to open (for reading) partition file %s (%s)\n"
		"%s"
		"I will have to exit now ...",
		partitionName, 
		messSysErrorText(), diskGetErrorText('o','r')) ;
    }
  size = lseek(fd, 0, SEEK_END);
  if (size < 0)
    {
      messExit ("Error occured when trying to access (for seek) partition file %s (%s)\n"
		"I will have to exit now ...",
		partitionName, messSysErrorText()) ;
    }
  close (fd);
#endif /* UNIX */

  iSize = size / BLOC_SIZE ;
  if (ps_debug)
    fprintf(stderr,"PartitionSize : %s = %d\n", partitionName, iSize);

  messcrashroutine = previousCrashRoutine ;

  return iSize ;
} /* PartitionSize */

         /************************************************************/
         /*   give total base size in #blocks number                 */
         /************************************************************/

static int PhysicalBaseSize()
{ int i, totalSize = 0;

  for (i = 0 ; i <= lastPartition_L ; i++) 
       {
	 char *pnam = PartitionName(i);
	 totalSize += PartitionSize(pnam);
	 messfree(pnam);
       }
  if (ps_debug) 
      fprintf(stderr, "total Base size :  %d\n", totalSize);
  return totalSize;
}

         /****************************************************/
         /*   donne l'adresse dans une partition a partir de */
         /*             l'adresse dans la base               */
         /****************************************************/

/* DISK casted to long or char * ?*/
static long PartitionAddress(DISK d, int *partition)
{
  /* premiere version tres bete : on cherche sequentiellement */
  long adresseDisque = (long)d;
  long adrDebutPartition = (long)0;
  PARTITION *pp = arrayp (pTable, 0, PARTITION) ;
  int part;

  for (part = 0; part <= lastPartition_L; part++, pp++)
    {
      long adrFinPartition = adrDebutPartition + pp->currentBlocks ;
      if (d <  adrFinPartition)
	{
	  if(ps_debug1)
	    fprintf(stderr, "pour adr disque %ld adresse part : (%d,%ld)\n",
		    (long)d, part, adresseDisque - adrDebutPartition);
	  *partition = part;
	  return adresseDisque - adrDebutPartition;
	}
      adrDebutPartition += pp->currentBlocks ;
    }

  messcrashroutine = simpleCrashRoutine ;
  messcrash ("impossible disk address %ld", (long) d) ;

  return 0 ; /* for compiler happiness */
} /* PartitionAddress */

         /****************************************************/
         /*  give full partition pathname from #number       */
         /****************************************************/


static char *PartitionName(int num)
{ PARTITION *pp ;

  if (num < 0 || num >= arrayMax(pTable)) 
    { 
      messcrashroutine = simpleCrashRoutine ; 
      messcrash ("PartitionName : invalid number %d\n", num) ;
    }

  pp = arrayp (pTable, num, PARTITION) ;
  if (*pp->fileSystem)
    return strnew(messprintf ("%s/%s", 
			      pp->fileSystem, pp->fileName), 0) ;
  else
#if defined(MACINTOSH)
    return 
      strnew(messprintf ( ":database:%s", pp->fileName ), 0) ;
#else
    return 
      dbPathMakeFilName("database", pp->fileName, "",  0);

#endif
}

         /****************************************************/
         /*  Close all open partitions of the DataBase       */
         /****************************************************/

static void CloseDataBase()
{ 
  int fd, *fdp, part ;
  VoidRoutine previousCrashRoutine = messcrashroutine ;

  messcrashroutine = simpleCrashRoutine ;

  part =  arrayMax(fdRead) ;
  fdp = arrp (fdRead, 0, int) - 1 ;
  while (fdp++, part--)
    { fd = *fdp - 1 ;
      if (fd >= 0) 
	{
#if defined(MACINTOSH)
	  FMfilclose (fd) ;
#else
	  if (close (fd) != 0)
	    messExit ("Error occurred when trying to close a block file (%s)",
		      messSysErrorText());
#endif
	  *fdp = 0 ;
	}
    }

  rPos = arrayReCreate (rPos, 2, myoff_t) ;

  part =  arrayMax(fdWrite) ;
  fdp = arrp (fdWrite, 0, int) - 1 ;
  while (fdp++, part--)
    { fd = *fdp - 1 ;
      if (fd >= 0) 
      { int rc;
#if defined(MACINTOSH)
	FMfilclose (fd) ;
#else
	seteuid(euid);
	rc = close (fd) ;
	seteuid(ruid);
	if (rc != 0)
	  messExit ("Error occurred when trying to close a block file (%s)",
		    messSysErrorText());
#endif
	*fdp = 0 ;
      }
    }
  wPos = arrayReCreate (wPos, 2, myoff_t) ;

  messcrashroutine = previousCrashRoutine ;

  return;
} /* CloseDataBase */

         /****************************************************/
         /*        Database map creation and update          */
         /****************************************************/

  /* Parameters:
   *
   * new - if TRUE, we're initialising a new database.
   *       that means we read the database definition from 
   *       wspec/database.wrm and we can't assume having read a previous
   *       database map from database/database.map
   *       if FALSE it's just a control for extending the database.
   *       In this case, we can just extend the current map with 
   *       new partitions added at the end of the DataBase 
   *       description file.
   *       Others modifications are ignored.
   *
   * skip - if TRUE there is already a database map, 
   *        find new partitions in definition file 
   */
static BOOL DataBaseReadDefinition(BOOL new, BOOL skip)
{
  char *defName ;
  ACEIN dbdef_in;
  char *cp ;
  PARTITION *pp = 0 ;
  int nbPartitions = 0;
  VoidRoutine previousCrashRoutine = messcrashroutine ;

  /* DataBase description file name : */
  defName = dbPathStrictFilName("wspec", "database", "wrm", "r", 0);
  if (ps_debug) fprintf(stderr, "opening def file %s\n", defName);

  /* open DataBase description file. If no file and DataBase creation,
   * default to the standard single unix file DataBase */
  if (defName)
    { 
      dbdef_in = aceInCreateFromFile (defName, "r", "", 0);
      messfree(defName);
    }
  else
    dbdef_in = NULL;
  
  if (!dbdef_in)
    {
      /* can't open the database description file,
       * unless we're initialising a new database, 
       * this is unacceptable */
      if (!new) return FALSE ;

      messdump("No DataBase Definition, default to standard unix file");
      pp =  arrayp(pTable, 0, PARTITION) ;
      pp->type     = UNIX_FILE_SYSTEM;
      strcpy(pp->hostname, "local");
      strcpy(pp->fileSystem, "ACEDB");
      strcpy(pp->fileName, "blocks.wrm");
      /* can't exceed 1 Giga Bytes : */
      pp->maxBlocks = MAX_DATABASE_SIZE;
      pp->currentBlocks = 0;
      /* complete memory map initialization : */
      lastPartition_L = -1;
      totalNbPartitions_L  = 1;
      return TRUE;
   }

  if (!new && skip) 
    {
      int n = 0;
      /* there is already a DataBase and a map, we may have to 
       * extend the map, if the user has added new partitions to 
       * the definition-file */
      
      /* skip existing partitions,
       * after this loop the file-pointer is positioned
       * at the end of the last partition-definition that we knew about
       * when we started it up last time. */
      while (n < totalNbPartitions_L && aceInCard (dbdef_in))
	{
	  if (!(cp = aceInWord (dbdef_in)))
	    continue ; /* skip empty lines and comments */
          if (strcmp(cp, "PART") == 0)
	    n++;
	}

      /* set nbPartitions to total calculated from database map,
       * so if the definition file has grown since the last map was made
       * (i.e. the user had added new partitions), we'll add new
       * the new definitions to the partition table, and therefore
       * increase the total, which will cause the map to be extended. */
      nbPartitions = totalNbPartitions_L;
    }
  else
    {
      /* we're not skipping existing partitions, so we'are reading
       * the definition file from the start */
      nbPartitions = 0;
    }


  /* create or extend partitions map : */
  messcrashroutine = simpleCrashRoutine ;

  while (aceInCard (dbdef_in))
    {
      if (!(cp = aceInWord(dbdef_in)))
	continue ; /* skip empty lines and comments */
      if (strcmp(cp, "PART") != 0)	/* only recognise PART keyword */
	continue ;
      pp =  arrayp(pTable, nbPartitions++, PARTITION) ;
      aceInStep (dbdef_in, ':') ;
      if (! aceInInt (dbdef_in, &pp->type) ||
	  ! (cp = aceInWord (dbdef_in)) ||
	  ! (strcpy (pp->hostname, cp), aceInWord (dbdef_in)) ||
	  ! (strcpy (pp->fileSystem, cp), aceInWord (dbdef_in)) ||
	  ! (strcpy (pp->fileName, cp)) ||
	  ! aceInInt (dbdef_in, &pp->maxBlocks) )
	messExit("Parse error in database definition file %s at line %d",
		 dbPathMakeFilName("wspec", "database", "wrm", 0),
		 aceInStreamLine(dbdef_in)-1);

      /* current, maximum partition size in blocks and offset : */
      
      if (strcmp("ACEDB", pp->fileSystem) == 0)
	*pp->fileSystem = 0 ;

      if (strcmp("NONE", pp->fileName) == 0)
	*pp->fileName = 0 ;
      else
	{ /* this partition name cannot match an existing one !!!! */
	  int i;
	  PARTITION *pp1 ;
	  for (i = 0; i < nbPartitions-1; i++)
	    {
	      pp1 = arrp(pTable, i, PARTITION);
	      if (strcmp(pp1->fileName, pp->fileName) == 0)
		messExit("Error in database definition file %s at line %d.\n"
			 "Partition name %s already exists!",
			 dbPathMakeFilName("wspec", "database", "wrm", 0),
			 aceInStreamLine (dbdef_in)-1, pp->fileName);
	    }

	  /* delete partition if exists , but not old style blocks.wrm !! */
	  if (strcmp("blocks.wrm", pp->fileName) != 0)
	    {
	      char *fn = dbPathStrictFilName("database", pp->fileName, 0, "r", 0);
	      if (fn)
		{
		  unlink (fn) ;
		  messfree(fn);
		}
	    }
	}
      pp->currentBlocks = 0;
    }

  aceInDestroy (dbdef_in);

  /* complete memory map initialization : */
  if (new)
    lastPartition_L = -1;

  /* set new total, as it may have changed if the def-file had grown */
  totalNbPartitions_L  = nbPartitions;

  /* in case of "old DataBase style" map and in case we have 
   * more than this "blocks.wrm" partition :
   * we adjust max blocks.wrm partition to its current size */
  pp =  arrayp(pTable, 0, PARTITION) ;
  if (strcmp(pp->fileName, "blocks.wrm") == 0 && nbPartitions > 1)
    pp->maxBlocks = pp->currentBlocks ;

  messcrashroutine = previousCrashRoutine ;

  return TRUE;
} /* DataBaseReadDefinition */



  /* assume that DataBase memory map exists */

static BOOL DataBaseUpdateMap(void)
{
#if defined(MACINTOSH)

  BOOL wasUpdated = FALSE ;
  char *mapName ;
  int fpMap ;
  PARTITION *pp =  arrayp(pTable, 0, PARTITION) ;
  char fileSystem[256], fileName[256], mapString[256] ;
  int nbPartitions = totalNbPartitions_L;

  mapName = dbPathStrictFilName("database", "database", "map", "w", 0) ;
  if (ps_debug1) fprintf(stderr, "opening map file %s\n", mapName);
  if ( mapName && ( fpMap = FMfilopen (
  		(unsigned char*) mapName,
  		(unsigned char*) "",
  		(unsigned char*) "w" )))
  {
    sprintf( mapString, "%d %d\n", totalNbPartitions_L, lastPartition_L) ;
    FMfilwrite ( fpMap, (unsigned char*) mapString, strlen ( mapString ) ) ;

    while (nbPartitions--) {
      /* system and file names : */
      if (!pp->type)
	continue ;
      strcpy(fileSystem, pp->fileSystem) ;
      strcpy(fileName, pp->fileName);
      if (!strlen(pp->fileSystem)) strcpy(fileSystem, "ACEDB");
      if (!strlen(pp->fileName))   strcpy(fileName, "NONE");
      sprintf( mapString, "%d %s %s %s %d %d %d\n",
              pp->type, pp->hostname, fileSystem, fileName,
              pp->maxBlocks, pp->currentBlocks, 0) ;  
      /* last 0, for backward compatibility, was the unused offset variable */

      FMfilwrite (fpMap, (unsigned char*) mapString, strlen (mapString) ) ;
      pp++;
    }
    FMfilclose (fpMap);
    wasUpdated = TRUE ;
  }
  if (mapName)
    messfree(mapName);

  return wasUpdated;

#else  /* !MACINTOSH */

	/* DataBaseUpdateMap */

  char *mapName;
  FILE *fil = 0 ;
  PARTITION *pp ;
  char fileSystem[256], fileName[256];
  int i;

  seteuid(euid); 
  mapName = dbPathMakeFilName("database","database", "map", 0);
  /* dbPathStrictFilname fails on file which need write access via euid */

  fil = fopen(mapName, "w");
  messfree(mapName);

  if (!fil)
    {
      messcrash("Unable to update database/database.map file (%s)",
		messSysErrorText());
      return FALSE ;
    }

  /* write first line */
  fprintf(fil, "%d %d\n", totalNbPartitions_L, lastPartition_L) ;

  /* write all partitions */
  for (i = 0; i < totalNbPartitions_L; i++) 
    {
      pp = arrayp (pTable, i, PARTITION);

      if (!pp->type) continue ;

      /* system and file names : */
      strcpy(fileSystem, pp->fileSystem);
      strcpy(fileName, pp->fileName);

      if (strlen(pp->fileSystem) == 0)
	strcpy(fileSystem, "ACEDB");

      if (strlen(pp->fileName) == 0)
	strcpy(fileName, "NONE");

      if (fprintf(fil, "%d %s %s %s %d %d %d\n",
		  pp->type, pp->hostname, fileSystem, fileName,
		  pp->maxBlocks, pp->currentBlocks, 0) == EOF)
	{
	  messdump("Unable to update database/database.map file (%s)",
		   messSysErrorText());
	  return FALSE;
	}
    }
  fclose (fil);

  seteuid(ruid);

  return TRUE;
#endif  /* !MACINTOSH */
} /* DataBaseUpdateMap */

              /*************************************************/
              /* download in memory structure the database map */
              /*************************************************/

              /****************************************************/
              /* try to build a DataBase map with first partition */
              /* as old style dataBase file database/blocks.wrm   */
              /****************************************************/


static void CreateOldStyleDataBaseMap(void)
{
  PARTITION *pp = arrayp (pTable, 0, PARTITION) ;
  char *oldBaseFileName = 
    dbPathStrictFilName("database", "blocks","wrm","r", 0) ;
  if (!oldBaseFileName)
    messExit("Consistency error - "
	     "No DataBase map and no old style DataBase "
	     "(i.e. database/blocks.wrm)\n"
	     "You may have to re-initialise the database") ;
  if (!(pp->currentBlocks = PartitionSize (oldBaseFileName)))
    messcrash("\n  old DataBase (database/blocks.wrm) is empty !!\n");
  messfree(oldBaseFileName);
  pp->type     = UNIX_FILE_SYSTEM;
  strcpy(pp->hostname, "local");
  strcpy(pp->fileSystem, "");
  strcpy(pp->fileName, "blocks.wrm");
  /* maximum size is current size (we only have one block) */
  pp->maxBlocks = pp->currentBlocks;
  lastPartition_L = 0;
  totalNbPartitions_L  = 1;

  DataBaseUpdateMap () ;   /* add others partitions, */

  DataBaseReadDefinition(FALSE, FALSE);	/* new=FALSE,skip=FALSE */

  if(totalNbPartitions_L == 1) 
    {
      /* single file partition : extend max size to 1 Giga : */
      pp->maxBlocks = MAX_DATABASE_SIZE;
      DataBaseUpdateMap ();
    }

  return;
} /* CreateOldStyleDataBaseMap */


static void readPartitionMap (void)
{
  char *mapName, *cp ;
  ACEIN dbmap_in = 0;
  PARTITION *pp ;
  int i, mapPartitionNum, nn ;
  VoidRoutine previousCrashRoutine = messcrashroutine ;
  STORE_HANDLE handle = handleCreate();

  messcrashroutine = simpleCrashRoutine ;

  mapName = dbPathStrictFilName("database", "database", "map", "r", handle);

  if (mapName)
    /* map-file exists and is readable */
    dbmap_in = aceInCreateFromFile (mapName, "r", "", 0);
      
  if (!dbmap_in)
    {
      /* if map-file not readable, attempt to create one
       * with old DataBase file blocks.wrm */

      CreateOldStyleDataBaseMap () ;

      /* now, again to open the file */
      dbmap_in = aceInCreateFromFile (mapName, "r", "", 0);
    }

  if (!dbmap_in)
    {
      totalNbPartitions_L = 1 ; /* default */
    }
  else
    /* read the database.map file */
    {
      /* skip first line */
      if (!aceInCard (dbmap_in))
	messExit ("Error in database map file %s : \n"
		  "Unable to read first line. The file mustn't be empty !",
		  mapName);
      
      nn = 0 ; i = 0 ; 
      lastPartition_L = -1 ;
      totalNbPartitions_L = 0;
      
      while (aceInCard (dbmap_in))
	{ 
	  /* read map file line by line */
	  cp = aceInPos (dbmap_in) ;
	  if (!*cp)
	    continue ;
	  
	  pp = arrayp(pTable, i, PARTITION)  ;
	  totalNbPartitions_L++ ;
	  
	  if (!aceInInt (dbmap_in, &pp->type) ||
	      !(cp = aceInWord(dbmap_in)) ||
	      (strcpy(pp->hostname, cp), !(cp = aceInWord(dbmap_in))) ||
	      (strcpy(pp->fileSystem, cp), !(cp = aceInWord(dbmap_in))) ||
	      (strcpy(pp->fileName, cp), !aceInInt (dbmap_in, &pp->maxBlocks)) ||
	      !aceInInt (dbmap_in, &pp->currentBlocks))
	    messExit ("Parse error in database map file %s at line %d",
		      mapName, aceInStreamLine(dbmap_in)-1);
	  
	  /* system and file names : */
	  if (strcmp(pp->fileSystem, "ACEDB") == 0)
	    strcpy(pp->fileSystem, "");
	  if (strcmp(pp->fileName,    "NONE") == 0)
	    strcpy(pp->fileName, "");
	  
	  pp->debut = nn ;
	  nn += pp->currentBlocks ;
	  pp->fin = nn ;
	  
	  if (pp->currentBlocks > 0)
	    lastPartition_L = i ;
	  i++ ;
	}

      aceInDestroy (dbmap_in);
    }


  /* now, control if new partitions are defined in the  */
  /* DataBase Description skip current definitions :    */
  mapPartitionNum = totalNbPartitions_L;

  if (DataBaseReadDefinition(FALSE, TRUE)) /* new=FALSE, skip=TRUE */
    {
      if (totalNbPartitions_L > mapPartitionNum)
	/* the database definition file has grown, since the
	 * map was built (having mapPartitionNum partitions),
	 * so write a new map */
	DataBaseUpdateMap();
    }

  messcrashroutine = previousCrashRoutine ;

  handleDestroy(handle);

  return;
} /* readPartitionMap */

/************************************************************/

static char *diskGetErrorText (char action, /* o = open
					     * a = access */
			       char spec) /* r = read
					   * w = write
					   * a = append */
{
  static char error_text[5000] = "";

  switch (errno)
    {
    case EACCES:		/* Permission denied */
      {
	char *purpose = (spec == 'r') ? "readable" : "writable";
	char *what = (action == 'o') ? "directory" : "partition file";
	sprintf(error_text, "Check that the database %s is %s by %s\n", 
		what, purpose, getLogin (FALSE));
      }
      break;
      
    case EFBIG:		/* File too large */
      sprintf(error_text, "The maximum file size has been exceeded. "
	      "Please check the 'max_size' number in wspec/database.wrm.\n");
      break;
		
    case EDQUOT:
      sprintf(error_text, "The quota of disk-blocks or i-nodes defined for the user "
	      "on the filsystem containing the database has been exhausted.\n");
      break;
	  
    case EINTR:
      {
	char *attempt = (action == 'o') ? "open a new" : "access a";
	sprintf(error_text, "The attempt to %s partition file was "
		"stopped by an interrupt signal\n", attempt);
      }
      break;
      
    case EMFILE:
      sprintf(error_text, "You have reached the soft limit for open files for this process. "
	      "Type 'limit descriptors' in your shell to check the maximum allowed.\n");
      break;
      
    case ENFILE:
      sprintf(error_text, "The system file table is full\n");
      break;
      
    case ENOSPC:
      {
	char *what = (action == 'o') ? "directory" : "partition file";
	sprintf(error_text, "The database disk is full "
				 "and the %s could not be extended.\n",what);
      }
      break;
      
    case ERANGE:		/* Result too large */
      sprintf(error_text, "The disk access request size was "
	      "outside the range supported by the file system\n");
      break;
		
    case EROFS:
      {
	char *hint = (spec == 'r') ? "" : "It needs to be re-configured to be able to extend the database.";
	sprintf(error_text, "The database files are located on a read-only file-system.%s\n", hint);
      }
      break;
      
    case EREMOTE:
      sprintf(error_text, "It seems that the NFS server mounting the database disk has "
	      "attempted to handle the request to extend the database by generating "
	      "a request to another NFS server, which is not allowed.\n");
      break;
      
    case ESTALE:
      sprintf(error_text, "The NFS server of the database "
	      "disk has unmounted the database directory.\n");
      break;
      
    case ETIMEDOUT:
      sprintf(error_text, "The network connection to the NFS "
	      "server mounting the database directory "
	      "has timed out. The server may be down or "
	      "there is a network problem.\n");
      break;
      
    default:
      strcpy(error_text, "");
    }

  return &error_text[0];
} /* diskGetErrorText */

#endif /* ACEDB 4 only */

/**************************************************************/
/**************************************************************/
 
