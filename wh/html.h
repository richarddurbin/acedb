/*  File: html.h
 *  Authors: Martin Senger (m.senger@dkfz-heidelberg.de)
 *           Petr Kocab    (p.kocab@dkfz-heidelberg.de)
 *
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
 * Description:
 *    Header file for generating HTML documents.
 *
 * HISTORY:
 *    Created: Sun Apr 16 1995 (senger)
 *-------------------------------------------------------------------
 */

/* $Id: html.h,v 1.2 1999-09-01 11:11:42 fw Exp $ */

/******************************************************************************
*
*   --- constants, literals --- 
*
******************************************************************************/
/* --- types of HTML lists --- */
#define HTML_LIST_BULLET       1
#define HTML_LIST_NUMBER       2
#define HTML_LIST_DICT         3

/******************************************************************************
*
*   --- function prototypes --- 
*
******************************************************************************/
FILE* set_html_output (FILE *output);

void html_title (int underline, char* header, char* title);
void html_end (void);
void html (char* format, ...);
void html_button (char* value, char* url);
void html_start_list (int type);
void html_list (char* format, ...);
void html_end_list (void);
void html_line (void);

char* html_bold (char* text);
char* html_link (char* text, char* url);
char* html_name (char* name);
char* html_esc  (char* str);
