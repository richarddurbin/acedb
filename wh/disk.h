/*  File: disk.h
 *  Author: Fred Wobus (fw@sanger.ac.uk)
 *  Copyright (c) J Thierry-Mieg and R Durbin, 1999
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
 *	Jean Thierry-Mieg (CRBM du CNRS, France) mieg@kaa.crbm.cnrs-mop.fr
 *
 * Description: public functions of disksubs.c 
 *              handling the disk. 
 * Exported functions:
 * HISTORY:
 * Last edited: Sep 21 11:55 1999 (edgrif)
 * * Sep 21 11:55 1999 (edgrif): added prototype for diskSetSyncWrite
 * Created: Thu Aug 26 17:12:07 1999 (fw)
 * CVS info:   $Id: disk.h,v 1.12 2000-03-02 15:32:00 srk Exp $
 *-------------------------------------------------------------------
 */

#ifndef ACEDB_DISK_H
#define ACEDB_DISK_H

#ifndef DEF_BP
#define DEF_BP
typedef void* BP ;
#endif

#ifndef DEF_DISK
#define DEF_DISK
typedef KEY  DISK;             /*holds disk addresses as 4 bytes unsined int */
#define NULLDISK 0
#endif /* DEF_DISK */
 
void diskPrepare    (void) ;
                      /*To be called once in the disk lifetime*/
                      /*prepares DISKSIZE blocks of storage and the*/
                      /*corresponding BAT file */
void diskSave       (void) ; 
                      /* Rewrites the Block allocation table BAT*/
                      /* of the main database file */
void diskWriteSuperBlock (BP bp) ;
void  aDiskReadSuperBlock(BP bp);                      /* General database management */
void diskavail      (int *free,int *used, int *plus, int *minus, int *dread, int *dwrite) ;
                      /*gives disk statistics*/
void diskalloc      (BP bp);
                      /*modifies the BAT and gives a free block address*/
                      /* in sequence */
void diskfree       (BP bp) ;
                      /* returns d to the BAT */
void diskSetSyncWrite(void) ;				    /* Sets synchronous disk writing on. */
void diskblockread  (BP p,DISK d) ; /* reads a block from disk */
void diskblockwrite (BP p) ; /* writes a block to disk */
BOOL dataBaseCreate (void) ;
void dataBaseAccessInit (void) ;
void diskFlushLocalCache(void);
BOOL saveAll(void) ; 

void diskFuseOedipeBats(Array fPlusArray, Array fMinusArray)  ;
BOOL diskFuseBats(KEY fPlus, KEY fMinus, 
		  KEY sPlus, KEY sMinus,
		  int *nPlusp, int *nMinusp) ;

#endif /* !ACEDB_DISK_H */
