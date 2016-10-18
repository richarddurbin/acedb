/*  File: pepgifcommand.c
 *  Author: Fred Wobus
 *  Copyright (C) J Thierry-Mieg and R Durbin, 1994
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
 * 	Richard Durbin (MRC LMB, UK) rd@mrc-lmb.cam.ac.uk, and
 *	Jean Thierry-Mieg (CRBM du CNRS, France) mieg@kaa.cnrs-mop.fr
 *
 * $Id: pepdisp.c,v 1.44 1999/10/08 16:05:23 fw Exp $
 * Description: peptide display - using colcontrol.c
 * Exported functions:
 * HISTORY:
 * Last edited: Nov  1 17:45 1999 (fw)
 * Created: Fri Oct 22 14:35:13 1999 (fw)
 *-------------------------------------------------------------------
 */

#include "acedb.h"
#include "aceio.h"
#include "bs.h"
#include "whooks/classes.h"
#include "peptide.h"
#include "lex.h"
#include "pepgifcommand.h"	/* our public header */

/************************************************************/

static magic_t PepGifCommand_MAGIC = "PepGifCommand";

struct PepGifCommandStruct { /* complete opaque struct from pepdisp.h */
  magic_t *magic ;		/* == &PepGifCommand_MAGIC */
  KEY key ;
  int x1, x2 ;
  Array pep ;
} ;

static BOOL pepGifCommandExists (PepGifCommand pepLook);

/******************************************************************
 ************************* public functions ***********************
 ******************************************************************/

PepGifCommand pepGifCommandGet (ACEIN command_in, ACEOUT result_out, PepGifCommand oldPepLook)
{
  KEY key ;
  Array pep = 0 ;
  int x1 = 1, x2 = 0 ;
  char *word ;
  PepGifCommand pepLook;

  if (oldPepLook) 
    pepGifCommandDestroy (oldPepLook);

  if (!(word = aceInWord(command_in)))
    goto usage ;
  if (!lexword2key (word, &key, _VProtein))
    {
      aceOutPrint (result_out, "// gif pepget error : Protein %s not known\n", word) ;
      goto usage ;
    }
  x1 = 1 ; x2 = 0 ;		/* default for whole sequence */
  if (aceInCheck (command_in, "w"))
    { word = aceInWord(command_in) ;
      if (!strcmp (word, "-coords"))
	{ if (!aceInInt (command_in, &x1) || !aceInInt (command_in, &x2) || (x2 <= x1) || x1 < 1)
	    goto usage ;
	}
      else
	goto usage ;
    }

  pepLook = (PepGifCommand)messalloc (sizeof (struct PepGifCommandStruct)) ;
  pepLook->magic = &PepGifCommand_MAGIC ;
  pepLook->key = key ;
  pepLook->x1 = x1 ;
  pepLook->x2 = x2 ;
  pepLook->pep = pep ;
  return pepLook ;

 usage:
  aceOutPrint (result_out, "// gif pepget error : usage: pepget protein [-coords x1 x2]  with 0 < x1 < x2\n") ;
  return (PepGifCommand)NULL ;
} /* pepGifCommandGet */

/**************************************************************/
 
void pepGifCommandSeq (ACEIN command_in, ACEOUT result_out, PepGifCommand pepLook)
{
  int i, x1, x2, nn = 0 ;
  char *cp, *cq, *buf = 0 ;

  if (!pepGifCommandExists (pepLook))
    messcrash("pepGifSeq() - received invalid pepLook");

  if (!pepLook->pep)
    {
      pepLook->pep = peptideGet (pepLook->key);

      if (!pepLook->pep)
	{ 
	  aceOutPrint (result_out, "// gif pepget error : Protein sequence %s not known\n", name(pepLook->key)) ;
	  goto usage ;
	}
    }

  x1 = pepLook->x1 ; x2 = pepLook->x2 ; nn = arrayMax(pepLook->pep) ;
  if (nn && x2 == 0) pepLook->x2 = x2 = nn ;
  if (x1 > nn) x1 = pepLook->x1 = nn ;
  if (x2 > nn) x2 = pepLook->x2 = nn ;
  
  if (x1 >= x2)
    { 
      aceOutPrint (result_out, "// gif pepget error : The protein has length=%d, smaller than your x1=%d\n", nn, pepLook->x1) ;
      return ;
    }
  cp = arrp(pepLook->pep, x1 - 1, char) ; 
  cq = buf = messalloc (x2 - x1 + 1) ;
  for (i = x1 ; i <= x2 ; i++, cp++, cq++)
    *cq = pepDecodeChar[(int)*cp] ;
  aceOutPrint (result_out, ">%s\n%s\n", name(pepLook->key), buf) ;
  messfree (buf) ;
  
  return ;
  
 usage:
  aceOutPrint (result_out, "// gif pepseq error: usage: pepget then pepseq\n") ;
  
  return; 
} /* pepGifCommandSeq */

/**************************************************************/
 
void pepGifCommandAlign (ACEIN command_in, ACEOUT result_out, PepGifCommand pepLook)
{
  Array aa, mypep ; 
  int i, j, ii ;
  char *cp, *cq, *buf = 0, *word ;
  int a1, a2, x1, x2, nn, nn2 ;
  KEY meth, prot, type ;
  float score ;
  OBJ obj = 0 ;
  BSunit *uu ;
  STORE_HANDLE handle = handleCreate();
  ACEOUT dump_out = aceOutCopy (result_out, handle);

  if (!pepGifCommandExists (pepLook))
    messcrash("pepGifAlign() - received invalid pepLook");

  if (!pepLook->pep)
    {
      pepLook->pep = peptideGet (pepLook->key);

      if (!pepLook->pep)
	{ 
	  aceOutPrint (result_out, "// gif pepalign error : Protein sequence %s not known\n", name(pepLook->key)) ;
	  aceOutPrint (result_out, "// gif pepalign error : usage: pepget then pepalign\n") ;
	  handleDestroy (handle);
	  return;
	}
    }
  
  if (aceInCheck (command_in, "w")
      && (word = aceInWord(command_in))
      && strcmp (word, "-file") == 0)
    {
      char *filename;

      if (!aceInCheck (command_in, "w")
	  || !(filename = aceInPath(command_in))
	  || strlen(filename) == 0)
	{
	  aceOutPrint (result_out, "// gif pepalign error : missing filename after -file option\n");
	  handleDestroy (handle);
	  return;
	}

      aceOutDestroy (dump_out);
      dump_out = aceOutCreateToFile (filename, "w", handle);
      if (!dump_out)
	{
	  aceOutPrint (result_out, "// gif pepalign error : Cannot open file %s\n", filename);
	  handleDestroy (handle);
	  return;
	}
    }

  x1 = pepLook->x1 ; x2 = pepLook->x2 ; nn = arrayMax(pepLook->pep) ;
  if (nn && x2 == 0) pepLook->x2 = x2 = nn ;
  if (x1 > nn) x1 = pepLook->x1 = nn ;
  if (x2 > nn) x2 = pepLook->x2 = nn ;
  
  if ((obj = bsCreate (pepLook->key)))
    {
      aa = arrayHandleCreate (240, BSunit, handle);
      bsGetArray (obj, str2tag("Homol"), aa, 8) ;
      bsDestroy (obj) ;
    }

  if (!arrayMax (aa))
    {
      aceOutPrint (result_out, "// gif pepalign error : No homology in %s\n", name(pepLook->key)) ;
      handleDestroy (handle) ;
      return ;
    }

  aceOutPrint (dump_out, "%s/%d-%d ", name(pepLook->key), pepLook->x1, pepLook->x2) ;
  cp = arrp(pepLook->pep, x1 - 1, char) ; 
  cq = buf = messalloc (nn + 1) ;
  memset (buf, (int)'.', nn) ;
  buf[nn] = 0 ;
  for (i = x1 ; i <= x2 ; i++, cp++, cq++)
    *cq = pepDecodeChar[(int)*cp] ;
  aceOutPrint (dump_out, "%s %s\n", buf, name(pepLook->key)) ;
  messfree (buf) ;

  for (ii = 0 ; ii < arrayMax(aa) ; ii+= 8) 
    {
      uu = arrp(aa, ii, BSunit) ;
      type = uu[0].k ;
      prot = uu[1].k ;
      meth = uu[2].k ;
      score = uu[3].f ;
      a1 = uu[4].i ;
      a2 = uu[5].i ;
      x1 = uu[6].i ;
      x2 = uu[7].i ;
      mypep = 0 ;

      if (type == str2tag("Pep_homol") &&
	  (mypep = peptideGet(prot)))
	{
	  aceOutPrint (dump_out, "%s/%d-%d ", name(prot), a1, a2, x1, x2) ;
	  cp = arrp(mypep, x1 - 1, char) ; 
	  cq = buf = messalloc (nn + 1) ;
	  memset (buf, (int)'.', nn) ;
	  buf[nn] = 0 ;
	  nn2 = arrayMax(mypep) ; cq += a1 - 1 ;
	  for (i = x1, j = a1 ; i <= x2 && i > 0 && i < nn2 ; i++, j++ , cp++, cq++)
	    if (j >= 0 && j < nn) 
	      *cq = pepDecodeChar[(int)*cp] ;
	  aceOutPrint (dump_out, "%s %s\n",buf, name(prot)) ;
	  messfree (buf) ;
	  arrayDestroy (mypep) ;
	}
    }

  handleDestroy (handle);
  return ;
} /* pepGifCommandAlign */

/**************************************************************/

void pepGifCommandDestroy (PepGifCommand pepLook)
{
  if (!pepGifCommandExists (pepLook))
    messcrash("pepGifCommandDestroy() - received invalid pepLook");
  
  if (arrayExists (pepLook->pep))
    arrayDestroy (pepLook->pep);

  messfree (pepLook);

  return;
} /* pepGifCommandDestroy */

/******************************************************************
 ************************ private functions ***********************
 ******************************************************************/

static BOOL pepGifCommandExists (PepGifCommand pepLook)
{
  if (pepLook && pepLook->magic == &PepGifCommand_MAGIC)
    return TRUE;

  return FALSE;
} /* pepGifCommandExists */

/*********************** eof *********************************/
