/*  File: disksubs.c
 *  Author: Jean Thierry-Mieg (mieg@mrc-lmb.cam.ac.uk)
 *  Copyright (C) J Thierry-Mieg and R Durbin, 1992
 *-------------------------------------------------------------------
 * This file is part of the ACEDB genome database package, written by
 * 	Richard Durbin (MRC LMB, UK) rd@mrc-lmb.cam.ac.uk, and
 *	Jean Thierry-Mieg (CRBM du CNRS, France) mieg@kaa.cnrs-mop.fr
 *
 * Description:
 * Exported functions:
 * HISTORY:
 * Last edited: Nov 15 18:21 1994 (rd)
 * * Jan 22 10:33 1992 (mieg): Modify bat and session obj en place.
 * Created: Wed Jan 22 10:33:50 1992 (mieg)
 *-------------------------------------------------------------------
 */

/* $Id: disksubs.c,v 1.16 1994-11-16 00:03:22 rd Exp $ */


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

extern int errno ;

#if defined(ACEDB1)  /* else disknew.c */

#include "acedb.h"
#include "disk__.h"
#include "disk.h"
#include "session.h"
#include "chrono.h"
#include "a.h"
#include "sysclass.h"
#include "systags.h"
#include "block.h"
#include "lex.h"
#include "lex_bl_.h"
#if !defined(MACINTOSH)
#include <sys/types.h>
#endif
#if defined(metrowerks)
#include "metrowerksglue.h"
#endif

#if defined(MACINTOSH)
#define USEFILEMANAGER
#include "macfilsubs.h"
#endif

extern char* aceFilName() ;
static void diskBATread(void);
static void diskBATwrite(void);
static void diskBATcompress(Array plusA, Array minusA) ;
static void diskExtend(BOOL zeroing) ;
static BOOL isBat(DISK d) ;

int INITIALDISKSIZE = 512 ;  /*number of blocks on the disk, called from blocksubs*/

static Array batArray  = 0 ;         /*block allocation table */
static Array plusArray  = 0 ;     /* new blocks in this session */
static Array minusArray  = 0 ;    /* blocked freed by this sesion */

static BOOL batIsModified = FALSE ;
static int readblockfile = -1 ;
static int writeblockfile = -1 ;

extern void cacheSaveAll(void) ;
#define PRINT FALSE

/**************************************************************/
    /* On first call during a given session, 
       any modified item
       is moved by blocksave to a new disk position, 
       this alters the lex and BAT but the change is not 
       registered on disk, hence the need for a sequence of
       write */

BOOL saveAll(void)
{
  extern void invokeDebugger(void) ;
  int i = 0, j = 3 ;
  BOOL modif = FALSE ;
  extern BOOL blockCheck (void) ;


  if(!isWriteAccess())
    return FALSE ;

  if (!blockCheck())
    messcrash ("Sorry - bad cache structure - in saveAll") ;

  while(j--)
    {
      if(PRINT)
	printf("\n saveAll pass %d",i++);
      if(batIsModified)
	{ j++ ;  /* Three additional loops for security ! */
	  modif = TRUE ;
	}
      diskBATwrite() ;
      
      cacheSaveAll();  /* Secondary Cache */
  
      lexSave () ;                   /* Lexiques go to cache */
      blockSave(TRUE) ;                  /*  flush the cache */
      if(modif)
	sessionRegister() ;
    }
  return modif ;
}

/**************************************************************/
             /* If no explicit call is made,
		when the first cache is full, blockalloc
		flushes a block, calling diskwrite calling
                BATread calling diskalloc with a resulting
		fatal knot in the global used blockend pointer
		*/

void diskPrepare(void)
{
  if (sizeof(DISK) != sizeof(void*))
    messcrash("On this machine the actual and hidden definitions"
	      " of DISK, i.e. void* and long, use a different number of"
	      " bytes, this is clearly fatal, see wh/disk.h and disk_.h"
	      ) ;
  if (thisSession.session > 1 )  /* Not during session 0 */
    diskBATread() ;
  else
    { batArray = arrayCreate(100,unsigned char) ;
      plusArray = arrayCreate(100,unsigned char) ;
      minusArray = arrayCreate(100,unsigned char) ;
      diskExtend(TRUE) ;
    }
}

/**************************************************************/
    /* query extends the size of the main database file */
static void diskExtend(BOOL zeroing)
{

#if defined(USEFILEMANAGER)
  int blockfile = 0 ;
#else
  FILE * blockfile;
#endif /* USEFILEMANAGER */
  
  register int i;
  register int  dd ;
  int newdd, dphysical ;
  
  unsigned int un=1, _64_b=64 * BLOC_SIZE;
  void * bp=(void *) messalloc((int) _64_b); /* zeroing implied */



  if(!isWriteAccess())
    messcrash("Disk Extend called without Write Access") ;

  chrono("diskExtend") ;

#if defined(USEFILEMANAGER)
  if(readblockfile>=0)
    FMfilclose(readblockfile) ;
  if(writeblockfile>=0)
    FMfilclose(writeblockfile) ;
#else
  if(readblockfile>=0)
    close(readblockfile) ;
  if(writeblockfile>=0)
    close(writeblockfile) ;
#endif

  readblockfile = writeblockfile = -1 ;

     /* append or create */

#if defined(USEFILEMANAGER)
  blockfile = FMfilopen("database/blocks", "wrm", "ab" ) ;
  if (!blockfile)
    messcrash ("Disk extend cannot open the main database file") ;
#else
  if (!(blockfile= filopen(aceFilName("database/blocks",0,0),"wrm","ab")))
     messcrash ("Disk extend cannot open the main database file") ;
#endif /* USEFILEMANAGER */

  if(!batArray)
    messcrash("NO batArray in diskextend(FALSE)") ;
  dd = 8*arrayMax(batArray) ;

  if (zeroing)
    { dd = 0 ; 
      newdd = INITIALDISKSIZE;
      newdd = newdd - (newdd % 64) ;
    }
  else
    { newdd  = dd + INITIALDISKSIZE;
      newdd = newdd - (newdd % 64) ;
    }

#if defined(USEFILEMANAGER)
  dphysical = FMPBGetEOF ( blockfile ) / BLOC_SIZE ;
  i = (63 + newdd  - dphysical) / 64 ;
  
  if (i>0)
    {
      int writeLength ;

      writeLength = FMPBwrite(blockfile, 0, fsFromLEOF, bp, _64_b * i ) ;

      if ( writeLength != ( _64_b * i ) )
	messcrash("Error during diskExtend(), please help");
    }

  FMfilclose (blockfile) ;
#else
  fseek(blockfile,0L,SEEK_END) ;
  dphysical = ftell(blockfile)/BLOC_SIZE ;
  if (dphysical < dd)
    messcrash("Diskextend found a physical file shorter than expected");

          /* initialize by chunks of 64 blocks */
  i = (63 + newdd  - dphysical)/64 ;
  if (i>0)
    while (i--)
      { if (fwrite(bp,un,_64_b,blockfile)!=_64_b)
	    messcrash("Error during diskExtend(), please help");
      }
  filclose (blockfile) ;
#endif /* USEFILEMANAGER */

  messdump ("The main file now holds %d blocks of data.\n", newdd);

  for(i=dd/8;i<newdd/8;i++)
    { array(batArray,i,unsigned char) = 0;
      array(plusArray,i,unsigned char) = 0;
      array(minusArray,i,unsigned char) = 0;
    }
  array(batArray,0,unsigned char) |= 7 ;
  batIsModified = TRUE ;
  
  chronoReturn() ; 
}

/**************************************************************/
/*************************************************************/
   /* Called from session WriteSuperBlock */  
void diskWriteSuperBlock(BP bp)
{
  register int i ;
  register unsigned char * p, *m ;

#if defined(USEFILEMANAGER)
  if(readblockfile>=0)          /* close files to flush them */
    FMfilclose(readblockfile) ;
  if(writeblockfile>=0)
    FMfilclose(writeblockfile) ;
#else
  if(readblockfile>=0)          /* close files to flush them */
    close(readblockfile) ;
  if(writeblockfile>=0)
    close(writeblockfile) ;
#endif

  readblockfile = writeblockfile = -1 ;

  /*  isWriteAccess() is  implied by the call */
  diskblockwrite(bp) ;     /* reopens file */

#if defined(USEFILEMANAGER)
  FMfilclose(writeblockfile) ;
#else
  close(writeblockfile) ;       /* to flush again */
#endif

  writeblockfile = -1 ;
                       /*  batReinitialise */
                       /*  batReinitialise */
  i = arrayMax(plusArray) ;
  p = arrp(plusArray,0,unsigned char) ;
  m = arrp(minusArray,0,unsigned char) ;
  while(i--)
    { *p++ = *m++ = 0 ;
      array(plusArray,i,unsigned char) = 0;
      array(minusArray,i,unsigned char) = 0;
    }
}

/**************************************************************/

static void diskBATread(void)
{ int i = INITIALDISKSIZE/8 ;
  int d1, j,k ;

  if (batArray) 
    return ;

  batArray = arrayGet(__Global_Bat,unsigned char,"c") ;
  if(!batArray || !arrayMax(batArray))
    {
      if(!isWriteAccess())
	messcrash("Cannot read the BAT");

    }

  i = arrayMax(batArray) ;
  plusArray = arrayCreate(i,unsigned char) ;
  minusArray = arrayCreate(i,unsigned char) ;
  arrayMax(minusArray) = arrayMax(plusArray) = i;
  
        /* DISK address 0 1 2 are for the system */
  array(batArray,0,unsigned char) |=7;     
  j = lexDisk(__Global_Bat) ;
  k=1<<(j%8); j =j/8;
  if(array(batArray,j,unsigned char) & k)
   ; 
/*    messdump("\n Correct bat self referenced address %d, session %d",
	   d1, KEY2LEX(__Global_Bat)->addr ?
 KEY2LEX(__Global_Bat)->addr->p->h.session : 0);
*/
  else 
    messcrash("no bat self reference for address %d",d1);

}

/********************************************/

static void diskBATwrite(void)
{ KEY kPlus, kMinus ;
  if (!batArray || ! batIsModified) 
    return ;
  batIsModified = FALSE ; /* must come before actual write */ 
  if(PRINT)
    printf("\nWriting BAT, address %d", 
	   KEY2LEX(__Global_Bat)->diskaddr) ;

  lexaddkey(messprintf("p-%d",thisSession.session),&kPlus,_VBat) ;
  lexaddkey(messprintf("m-%d",thisSession.session),&kMinus,_VBat) ;
  arrayStore(__Global_Bat, batArray,"c") ;
  arrayStore(kPlus, plusArray,"c") ;
  arrayStore(kMinus, minusArray,"c") ;
}

/********************************************/
   /* compress block which have been allocated and freed
      during the same session, i.e. necessary after a batFusion
      */
static void diskBATcompress(Array plusA, Array minusA)
{ register int i;
  register unsigned char *plus, *minus, *bat , c ;

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
}

/********************************************/

static BOOL fuseBats(Array old, Array new)
{ register int i;
  register unsigned char *o , *n ;
  BOOL result = FALSE ;

  if( !arrayExists(old) || !arrayExists(new) ) 
    return FALSE ;
  o = arrp(old,0,unsigned char) ;
  n = arrp(new,0,unsigned char) ;

  if(arrayMax(old) > arrayMax(new))
    messcrash("Inconsistency in fuseBats") ;
    
  i = arrayMax(old) ; o--; n-- ;
  while(o++, n++, i--)
    if (*o)
      { if(*o & *n)
	  messcrash("Duplication in fuseBats") ;
	result = TRUE ;
	*n |= *o ;
      }
    
  return result ;
}

/**************************************************************/

void diskFuseBats(Array fPlusArray, Array fMinusArray,
		  Array sPlusArray, Array sMinusArray) 
{
  BOOL modif = FALSE ;

  if(!sPlusArray && !sMinusArray) /* i.e. just kill father allocations */
    {  /* Twice fPlusArray, I mean it, it is NOT a bug */
      diskBATcompress(fPlusArray, fPlusArray) ;
      return ;
    }
      
  modif |= fuseBats(fPlusArray, sPlusArray) ;
  modif |= fuseBats(fMinusArray, sMinusArray) ;

  if(modif)
    diskBATcompress(sPlusArray, sMinusArray) ;
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
int diskBatSet(Array a)
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
void diskavail(int *free,int *used, int *plus, int *minus)
{
  if(!batArray)
    diskBATread();

  *used = diskBatSet(batArray) ;
  *plus = diskBatSet(plusArray) ;
  *minus = diskBatSet(minusArray) ;
  *free = 8*arrayMax(batArray) - *used ;
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

#ifdef DEBUG  
  { static KEY oldkey = 0 ;
    if (bp->h.key == oldkey)
      { putchar ('+') ;
	fflush (stdout) ;
      }
    else
      { printf ("\nDiskAlloc %s:%s %d ", 
		className(bp->h.key), name(bp->h.key), bp->h.session) ;
	oldkey = bp->h.key ;
      }
  }
#endif

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
		messcrash("disk alloc inconsistency") ;
	    }
      cp++;
      if (cp >= end) 
	cp = top;
    }
  
  diskExtend(FALSE);
  goto lao ;
}

/********************************************/
      /* do free only if allocated during this session */
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
  if ((d >= 8*j) || (d<0)) 
    messcrash("diskfree called with a wrong disk address");

  cp = arrp(batArray,(int) (d/8),unsigned char) ;
  minus = arrp(minusArray,(int) (d/8),unsigned char) ;
  plus = arrp(plusArray,(int) (d/8),unsigned char) ;
  
  i = 1 << (d%8);
  if (! (*cp & i))
    messcrash("diskfree called on free block ");

  if(*plus & i)  /* allocated during this session */
    { (*cp) &= ~i;     /* free it */
      (*plus) &= ~i;     /* free it */
    }
  else
    (*minus) |= i ;
  batIsModified = TRUE ;
  chronoReturn() ;
}

/********************************************/

static BOOL isBat(DISK d)
{
  register unsigned char i;  /*unsigned because we use a right shift operator on i*/
  register unsigned char *cp ;
  register int j ;
  
  if(!batArray)
    return TRUE ; /* during init, one does a few direct address reads */
  
  j = arrayMax(batArray) ;   /* must come after eventual BATread */
  if ((d >= 8*j) || (d<0)) 
    messcrash("diskfree called with a wrong disk address");

  cp = arrp(batArray,(int) (d/8),unsigned char) ;
  
  i = 1 << (d%8);
  if (*cp & i)
     return TRUE ;
  return FALSE ;
}

/**************************************************************/
/* to write a block to disk   : return 0 if success */
/*         called by blockunload  */
void diskblockwrite(BP bp)
{
  char *vp=(char *)bp;
  unsigned int nb,size=BLOC_SIZE;
  myoff_t pos,pp ;
  long d = bp->h.disk ;
  char *filename ;
  int spec=O_WRONLY | O_BINARY;
  int nretry = 0 ;
  
  if(!isWriteAccess())
    return ;

  chrono("diskblockwrite") ;
  
  if(!batArray)
    diskBATread() ;
  
  if ((d<0)||(d>=8*arrayMax(batArray)))
    messcrash("Diskblockwrite called with wrong DISK address d=%ld, max = %d",
	      d,8*arrayMax(batArray));
  
  if (writeblockfile<0)
    {
/* NB use "r" here to make sure it is the same file that we read from! */

#if defined(MACINTOSH)
#if defined(USEFILEMANAGER)
      writeblockfile= FMfilopen("database/blocks", "wrm", "w" );
#else
      writeblockfile = open ( aceFilName ("database/blocks", 0,0), "wrm", "r", spec );
#endif /* USEFILEMANAGER */
      if (!writeblockfile)
	messcrash ("Failed to find database/blocks.wrm") ;
#else
      if (!(filename = aceFilName ("database/blocks", "wrm", "r")))
	messcrash ("Failed to find database/blocks.wrm") ;
      writeblockfile = open ( filename, spec, 0 );
#endif /* MACINTOSH */

      if (writeblockfile == -1)
	    messcrash ("diskblockwrite cannot open blocks :%s", filename);
    }
  pos=(myoff_t) (d*(long)size ) ;

#if defined(USEFILEMANAGER)

  nb = FMPBwrite ( writeblockfile, (myoff_t)pos, fsFromStart, vp, size ); 
  if ( nb != size )
      messcrash("Diskblockwrite cannot actually write the DISK,n=%d\n", nb);

#else

  if((pp = lseek(writeblockfile, (myoff_t)pos, SEEK_SET)) != pos)
    messcrash(
 "Diskblockwrite with wrong DISK address d=%ld pos=%ld, returned=%ld\n",
	            (long)d,(long) pos,(long)pp);
  
  while( (nb=write(writeblockfile,vp,size))!=size)
    if (nretry++ == 5)
      messcrash("Diskblockwrite  cannot actually write the DISK,n=%d\n",
		nb);

#endif
  
  chronoReturn() ;
}

/**************************************************************/
             /* to read a block from disk   : return 0 if success */
             /*     called by blockload */
void diskblockread(BLOCK *bp, DISK d)
{
  char *vp=(char *)bp;
  unsigned int size=BLOC_SIZE;
  myoff_t pos,pp ;
  int n, nretry = 0;
  char *filename ;
  int spec=O_RDONLY | O_BINARY;
  
  chrono("diskblockread") ;
 
 if (readblockfile < 0)
    {
#if defined(MACINTOSH)
#if defined(USEFILEMANAGER)
      readblockfile = FMfilopen("database/blocks", "wrm", "r" );
#else
      readblockfile= open(filename,spec);
#endif /* USEFILEMANAGER */
      if (!readblockfile)
	messcrash ("Can't find blocks.wrm") ;
#else
      if (!(filename = aceFilName ("database/blocks", "wrm", "r")))
	messcrash ("Can't find blocks.wrm") ;
      readblockfile= open(filename,spec,0);
#endif /* MACINTOSH */

      if (readblockfile == -1)
	messcrash("diskblockread cannot open blocks :%s", filename);
    }
  
  pos=(myoff_t)(d*(long)size) ;

#if defined(USEFILEMANAGER)

  n = FMPBread ( readblockfile, (myoff_t)pos, fsFromStart, vp, size ); 
  if ( n != size )
      messcrash("Diskblockread cannot actually read the DISK,n=%d\n", n);

#else

  if((pp = lseek(readblockfile, (myoff_t)pos, SEEK_SET)) != pos)
    messcrash(
	      "Diskblockread  with wrong DISK address d=%ld pos=%ld, returned=%d\n",
	            (long)d,(long)pos,(long)pp);
  
  while((n=read(readblockfile,vp,size))!=size)
    if (nretry++ == 5)
      messcrash(
		"Diskblockread  cannot actually read the DISK, n=%d\n",n);

#endif

  
  if(d!=*(DISK*)bp)
    messcrash("ERROR : diskblockread read a block not matching its address");

  if(!isBat(d))
    messerror("I read a block not registered in the BAT") ;

  chronoReturn() ; 
}

/**************************************************************/
/**************************************************************/

#endif   /* ACEDB3 */
