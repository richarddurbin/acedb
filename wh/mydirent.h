/*  File: mydirent.h
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
 * Description: file/directory entity datatypes and symbols 
 *              Filesystem-independent directory information.
 * Exported functions: 
 * HISTORY:
 * Last edited: Apr  3 16:06 2000 (edgrif)
 * Created: Thu Aug 26 17:32:41 1999 (fw)
 * CVS info:   $Id: mydirent.h,v 1.18 2004-08-12 20:21:50 mieg Exp $
 *-------------------------------------------------------------------
 */

#ifndef ACEDB_MYDIRENT_H
#define ACEDB_MYDIRENT_H

#if defined(NEXT)
  #include <sys/dir.h>
  typedef struct direct MYDIRENT ;  /* Crazy next */
#endif

#if defined(ALLIANT)||  defined(IBM)
  #include <sys/dir.h>
  typedef struct dirent MYDIRENT ;
#endif
  
#if defined(CONVEX) 
  #include <sys/stat.h>
  #include <dirent.h>
  #define	S_IFMT		_S_IFMT
  #define	S_IFDIR		_S_IFDIR
  #define	S_IFBLK		_S_IFBLK
  #define	S_IFCHR		_S_IFCHR
  #define	S_IFREG		_S_IFREG
  #define	S_IFLNK		_S_IFLNK
  #define	S_IFSOCK	_S_IFSOCK	
  #define	S_IFIFO		_S_IFIFO
  #define	S_ISVTX		_S_ISVTX
  #define	S_IREAD		_S_IREAD
  #define	S_IWRITE	_S_IWRITE
  #define	S_IEXEC		_S_IEXEC
  typedef struct dirent MYDIRENT ;
#endif

#if !(defined(MACINTOSH) || defined(WIN32))
#include <sys/param.h>
#endif

#if defined (HP) || defined (SOLARIS) || defined (WIN32)
#if !defined (WIN32)
#include <unistd.h>
#endif
#define getwd(buf) getcwd(buf,MAXPATHLEN - 2) 
#else  /* HP || SOLARIS || WIN32 */
extern char *getwd(char *pathname) ;
#endif /* HP || SOLARIS || WIN32 */

#if defined (POSIX) || defined(SUN) || defined(SUNSVR4) \
 || defined(SOLARIS) || defined(DEC) || defined(ALPHA)  \
 || defined(SGI) || defined(LINUX) || defined(OPTERON) || defined(HP)       \
 || defined(FreeBSD) || defined(DARWIN)
#include <dirent.h>
  typedef struct dirent MYDIRENT ;
#endif

#endif /* !ACEDB_MYDIRENT_H */
