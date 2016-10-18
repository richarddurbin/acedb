/*  File: utils.h
 *  Author: Rob Clack (rnc@sanger.ac.uk)
 *  Copyright (c) J Thierry-Mieg and R Durbin, 2003
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
 * Description: header file for utils.c - general utility functions
 * HISTORY:
 * Last edited: Aug  9 18:25 2005 (edgrif)
 * Created: Fri May 2nd, 2003 (rnc)
 * CVS info:   $Id: utils.h,v 1.11 2005-10-04 13:15:01 edgrif Exp $
 *-------------------------------------------------------------------
 */

#ifndef DARWIN
/* case-insensitive version of strstr */
char *strcasestr      (char *str1, char *str2);
#endif

/* Wrapper to allow lexstrcmp to be used with arraySort.y */
int arrstrcmp(void *s1, void *s2);
/* lextstrcmp does an intuitive sort of strings that include numbers */
int lexstrcmp(char *a,char *b);

/*********************** end of file ********************************/
