/*  File: diskPart.h
 *  Author: J Thierry-Mieg
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
 * Description: description  des partitions composants l'espaces des blocs
 * Exported functions: 
 * HISTORY:
 * Last edited: Aug 26 17:14 1999 (fw)
 * Created: Thu Aug 26 17:14:28 1999 (fw)
 * CVS info:   $Id: diskPart.h,v 1.4 1999-09-01 11:07:46 fw Exp $
 *-------------------------------------------------------------------
 */
 
/* partitions description must reside on a unique disk block : */
/*#define MAX_PARTITIONS (BLOC_SIZE - (sizeof(int)*2)) / sizeof(PARTITION)*/
#define MAX_PARTITIONS 20

/* ATTENTION :
 * only a  multiple of INITIALDISKSIZE blocks can be used in each partition
 * See wh/disk.h , wspec/cachesize.wrm (DISK option)
 */
/* default maimum DataBase size in blocks */
#define MAX_DATABASE_SIZE 100 * 1024 

#define UNIX_FILE_SYSTEM 1
#define RAW_FILE_SYSTEM  2
#define LG_NAME          128

typedef struct {
    int type;             /* unix file system, raw, distant, ...            */
    char hostname[LG_NAME]; /* machine on which is the partition            */
    char fileSystem[LG_NAME];/* partition file system or "ACEDN if relative */
    char fileName[LG_NAME];  /* file name or "NONE" if not a file system    */
    int  maxBlocks;       /* max. size in n x  blocks                       */
    int  currentBlocks;   /* current size in n x blocks                     */
    int offset;           /* number of first non reserved block             */
    } PARTITION;

typedef struct {
    int       lastPartition; /* (i.e. number of active partitions)          */
    int       nbPartitions;  /* partition number                            */
    PARTITION partitions[MAX_PARTITIONS];
    } BASE_DEF;  

