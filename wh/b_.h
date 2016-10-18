/*  File: b_.h
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
 * Description: definition of the B block  private to bsubs and b2bs.c
 * Exported functions: 
 * HISTORY:
 * Last edited: Aug 26 16:53 1999 (fw)
 * Created: Thu Aug 26 16:52:43 1999 (fw)
 * CVS info:   $Id: b_.h,v 1.5 1999-09-14 16:15:26 srk Exp $
 *-------------------------------------------------------------------
 */
#ifndef ACEDB_B__H
#define ACEDB_B__H
 
#define DEF_BP /* avoid typedefing it as void* in disk.h */
typedef struct bblock *BBLOCK ;
typedef BBLOCK BP ;

#include "disk_.h"        /* defines BLOCKHEADER */

typedef short NODE;
 
typedef struct bhat *BHAT;
struct bhat { BLOCKHEADER h ;
              NODE top, free;
            }
              ;  
 /* h.type should be 'B' = Bblock  */
 /* h.key is the key of the first object in the block */

#include "bs_.h"    /* defines the BSdata Union */ 

#define NODEX sizeof(BSdata)  /* >= sizeof(KEY) I hope */
union char_key {
                 KEY key ;   
                 BSdata x ;
                 char c[NODEX] ;
               }
                 ;
typedef struct bnode {
               NODE d1, d2;
               union char_key ck;
             } 
                *NODEP ;

#define BNODEMAX ((BLOC_SIZE-sizeof(struct bhat))/sizeof(struct bnode))

struct bblock {
  struct bhat hat;
  struct bnode n[BNODEMAX];
}
                        ;
 


 
/***************************************************************************/
 
 void   Binit            (BP p);
 void   Bget             (BP *bp,NODE *nn,KEY key);
 void   Bdump            (KEY key);
 void   Bnode2str        (char *cq, BP bp, NODEP *r);
 void   Bstr2node        (BS bs, BP bp, NODEP *r);
 void   Bprune           (BP bp, NODE nn,BOOL bothsides);
 int    Bbranchlen       (BP bp, NODE nn);
 int    Bfreelen         (BP bp);
 NODEP  Bnewnode         (BP bp);
 BOOL   Bnextkey         (void **it,BP p,KEY *key);
 void   Balias           (BP p,KEY key, KEY newKey);

#endif /* !ACEDB_B__H */
 


