/*  File: disk_.h
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
 * Description: disk block universal structure 
 *              replicated in all classes  
 * Exported functions:
 * HISTORY:
 * Last edited: Aug 26 17:15 1999 (fw)
 * Created: Thu Aug 26 17:15:10 1999 (fw)
 * CVS info:   $Id: disk_.h,v 1.6 1999-09-01 11:07:50 fw Exp $
 *-------------------------------------------------------------------
 */

#ifndef ACEDB_DISK__H 
#define ACEDB_DISK__H

#ifndef DEF_DISK
#include "disk.h"
#endif

#define BLOC_SIZE 1024      /*number of char in a block*/
typedef struct  blockheader
  { DISK disk,                 /* where */
         nextdisk ;
    int session ;              /* when */
    KEY key ;                  /* who */
    char type ;                /* how */
  } BLOCKHEADER ;


#endif /* !ACEDB_DISK__H */


