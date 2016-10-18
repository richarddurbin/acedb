/*  File: block.h
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
 * Description: public functions of blocksubs.c 
 *              handling the cache 
 * Exported functions: 
 * HISTORY:
 * Last edited: Aug 26 16:54 1999 (fw)
 * Created: Thu Aug 26 16:53:35 1999 (fw)
 * CVS info:   $Id: block.h,v 1.4 1999-09-01 11:04:31 fw Exp $
 *-------------------------------------------------------------------
 */

#ifndef ACEDB_BLOCK_H
#define ACEDB_BLOCK_H

#ifndef DEF_BP
#define DEF_BP
typedef void* BP ;
#endif

 void blockInit(void);
                 /*Allocates BLOCKMAX blocks and their control area*/

 void blockwrite(KEY k);     /*force write the relevant block*/
 void blockrewrite(KEY key);
 
 int blockpinn(KEY k, BP *p); /*reads and pinns the relevant block*/
 void blockreallocate(KEY kk,BP *p);
 BOOL blockNext(BP *bpp); /* After blockGet, gives the continuations */
 BP blockGetContinuation(int nc) ; /* gives the nc continuation */
 void  blockSetNext(BP *bpp);
 void blockSetEnd(BP bp) ;
 void blockSetEmpty(BP bp) ;
 void blockunpinn(KEY k);     /*unpinns the relevant block*/
 
 void blockmark(KEY k);      /*marks  the relevant block as modified*/

 void blockSave(BOOL whole);       /*writes everything (half) back to disk*/
 void blockavail(int *used,int *pinned,int *free,int *modif) ;
 void blockStatus(int *nc1rp, int* nc1wp, int *ncc1rp, int* ncc1wp) ;

void blockshow(void);       /*scrmess the cache content*/
KEY blockfriend(KEY key) ;  /* gives a loaded key of same class */ 
 
#endif /* !ACEDB_BLOCK_H */



