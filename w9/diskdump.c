/*  File: diskdump.c
 *  Author: Jonathan Hodgkin (cgc@mrc-lmb.cam.ac.uk)
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
 * Description: to dump headers of all blocks into ascii file
 *	for debugging purposes
 * Exported functions:
 * HISTORY:
 * Last edited: Oct 16 15:44 1998 (fw)
 * Created: Sat May 22 23:45:07 1993 (cgc)
 *-------------------------------------------------------------------
 */

/* $Id: diskdump.c,v 1.6 1999-09-01 10:58:39 fw Exp $ */

#include "acedb.h"
#include "disk__.h"
#include "regular.h"


/***********************************************/
void chrono (char *cp) { }
void chronoReturn () { }
/***********************************************/

static int readblockfile = -1 ;

int diskBlockRead(BP bp, DISK d)
{
  char *vp=(char *)bp;
  unsigned int size=BLOC_SIZE;
  myoff_t pos,pp ;
  int n, nretry = 0;
  char filename[128];
  int spec = 0 /* O_RDONLY */ | O_BINARY ;
  
  chrono("diskblockread") ;
 
 if (readblockfile<0)
    {
#if defined(THINK_C)
      sprintf(filename,":database:blocks.wrm") ;      
#else
      sprintf(filename,"%s/database/block1.wrm", getenv("ACEDB")) ;
#endif
      readblockfile= open(filename,spec);
      if (readblockfile == -1)
	    messcrash("diskblockread cannot open blocks :%s", filename);
    }
  
  pos=(myoff_t)(d*(long)size) ;
  if((pp = lseek(readblockfile, (myoff_t)pos, SEEK_SET)) != pos)
    return FALSE ;
/*  messcrash(
"Diskblockread with wrong DISK address d=%ld pos=%ld, returned=%d\n",
	      (long)d,(long)pos,(long)pp);
*/
  
  while((n=read(readblockfile,vp,size))!=size)
    if (nretry++ == 5)
      return FALSE ;
/*
      messcrash("Diskblockread  cannot actually read the DISK, n=%d\n",n);

  if(d!=*(DISK*)bp)
    messcrash("ERROR : diskblockread read a block not matching its address");

  if(!isBat(d))
    messerror("I read a block not registered in the BAT") ;
*/
  chronoReturn() ; 
  return TRUE ;
}

/*******************************************/

void main (int argc, char **argv)
{
  BLOCK b ;
  DISK d = 0 ;

  while (diskBlockRead (&b, d))
    printf ("c=%2x %8d %8d %8d %12x %4d\n",
	    class(b.h.key), d++, b.h.disk, b.h.nextdisk, b.h.key, b.h.session) ;

  fprintf (stderr, "%d blocks read from %s/database/blocks.wrm\n",
	   d, getenv ("ACEDB")) ;
}







