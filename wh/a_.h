/*  File: a_.h
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
 * Description: definition of the A block private to asubs.c  
 * Exported functions: 
 * HISTORY:
 * Last edited: Aug 26 16:38 1999 (fw)
 * Created: Thu Aug 26 16:37:39 1999 (fw)
 * CVS info:   $Id: a_.h,v 1.8 1999-09-01 11:01:17 fw Exp $
 *-------------------------------------------------------------------
 */

#ifndef ACEDB_A__H
#define ACEDB_A__H
 
#define DEF_BP /* avoid typedefing it as void* in disk.h */
typedef struct ablock *ABLOCK ;
typedef ABLOCK BP ;
 
#include "disk_.h"   /* defines BLOCKHEADER */
#include "regular.h"   /* for Array definition */

typedef struct ahat *AHAT;
struct ahat { BLOCKHEADER h ;
	      int size, max, localmax ;
	      KEY parent ;	/* B object that "owns" me */
            }
              ;  
 /* h.type should be 'A' = permanent Array  */
 /* h.key is the key of the only object in the block */
 /* size, max matches the array */
 /* localmax is the number of elements stored in this block */

#define AMAX      (BLOC_SIZE-sizeof(struct ahat))
struct ablock {
  struct ahat hat;
  char  n[AMAX];
}
                        ;

typedef Array OBJ_HANDLE ;
#define DEF_OBJ_HANDLE

#endif /* !ACEDB_A__H */


