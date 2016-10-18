/*  File: acemblyhook.c
 *  Author: Danielle et jean Thierry-Mieg (mieg@mrc-lmba.cam.ac.uk)
 *  Copyright (C) J Thierry-Mieg and R Durbin, 1995
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
 * 	Richard Durbin (MRC LMB, UK) rd@mrc-lmba.cam.ac.uk, and
 *	Jean Thierry-Mieg (CRBM du CNRS, France) mieg@frmop11.bitnet
 *
 * Description:
 * Exported functions:
 * HISTORY:
 * Last edited: Oct  9 11:04 2000 (edgrif)
 * Created: Fri Oct 20 20:21:47 1995 (mieg)
 * CVS info:   $Id: acemblyhook.c,v 1.13 2000-10-09 10:05:42 edgrif Exp $
 *-------------------------------------------------------------------
 */

#include <wh/acedb.h>
#include <wh/aceio.h>
#include <wh/acembly.h>
#include <wh/dump.h>
#include <wh/pick.h>
#include <wh/parse.h>					    /* for ParseFuncType */
#include <wh/lex.h>
#include <wh/bs.h>					    /* for isTimeStamps */
#include <whooks/classes.h>

KEY   
_Clips,
_Later_part_of,
_OriginalBaseCall,
_Align_to_SCF,
_Quality,
_SCF_Position ;

static void parseArrayInit (void)
{
  extern BOOL baseCallDump (ACEOUT dump_out, KEY k) ;
  extern BOOL baseCallParse (ACEIN parse_in, KEY key) ;

  parseFunc[_VBaseCall]  = baseCallParse ;
  dumpFunc[_VBaseCall]   = baseCallDump ;

  pickSetClassOption ("BasePosition",  'A', 'h', FALSE, FALSE) ;
  pickSetClassOption ("BaseQuality",  'A', 'h', FALSE, FALSE) ;
  pickSetClassOption ("BaseCall",  'A', 'h', FALSE, FALSE) ;

  return;
} /* parseArrayInit */

void acemblyInit (void)
{   
  isTimeStamps = FALSE ;
  parseArrayInit () ;
  dnaAlignInit ();

  lexaddkey ("Clips", &_Clips, 0) ; 
  lexaddkey ("Later_part_of", &_Later_part_of, 0) ;
  lexaddkey ("OriginalBaseCall", &_OriginalBaseCall, 0) ;
  lexaddkey ("Align_to_SCF", &_Align_to_SCF, 0) ;
  lexaddkey ("Quality", &_Quality, 0) ;
  lexaddkey ("SCF_Position", &_SCF_Position, 0) ;
}
