/*  File: dotterng.c
 *  Author: Ed Griffiths (edgrif@sanger.ac.uk)
 *  Copyright (c) 2009: Genome Research Ltd.
 *-------------------------------------------------------------------
 * ZMap is free software; you can redistribute it and/or
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
 *-------------------------------------------------------------------
 * This file is part of the ZMap genome database package
 * originally written by:
 *
 * 	Ed Griffiths (Sanger Institute, UK) edgrif@sanger.ac.uk
 *
 * Description: Provides stub functions for linking a non-graphic
 *              version of dotter.
 *
 * Exported functions: All of the below.
 *              
 * HISTORY:
 * Last edited: Sep  2 17:58 2009 (edgrif)
 * Created: Wed Sep  2 17:35:46 2009 (edgrif)
 * CVS info:   $Id: dotterng.c,v 1.1 2009-11-24 09:39:11 edgrif Exp $
 *-------------------------------------------------------------------
 */



/* NOTE THAT THESE FUNCTIONS/VARIABLES DO NOT EVEN HAVE THE RIGHT SIGNATURE
 * AS THEY ARE __NEVER__ INTENDED TO BE CALLED, ALL THEY DO IS ACT AS
 * PLACE HOLDERS TO ALLOW A BATCH VERSION OF DOTTER TO BE COMPILED. */


void *blixemWindow = 0 ;

void blviewRedraw(void)
{
  return ;
}

void gexInit(void)
{ 
  return ;
}

void gexTextEditorNew(void)
{
  return ;
}

void gexRampSet(void)
{
  return ;
}

void gexRampTool(void)
{
  return ;
}

void gexRampPop(void)
{
  return ;
}
