/*  File: quovadis.c
 *  Author: Danielle et Jean Thierry-Mieg (mieg@mrc-lmb.cam.ac.uk)
 *  Copyright (C) J Thierry-Mieg and R Durbin, 1994
 *-------------------------------------------------------------------
 * This file is part of the ACEDB genome database package, written by
 * 	Richard Durbin (MRC LMB, UK) rd@mrc-lmba.cam.ac.uk, and
 *	Jean Thierry-Mieg (CRBM du CNRS, France) mieg@kaa.cnrs-mop.fr
 *
 * Description:
 * Exported functions:
 * HISTORY:
 * Last edited: Oct  4 16:12 2000 (edgrif)
 * * Feb  1 21:41 1996 (rd)
 * Created: Thu Sep  8 15:42:54 1994 (mieg)
 * CVS info:   $Id: quovadis.c,v 1.58 2000-10-09 10:38:07 edgrif Exp $
 *-------------------------------------------------------------------
 */

/* main menu */

#include <wh/acedb.h>
#include <wh/parse.h>
#include <wh/lex.h>
#include <wh/dump.h>
#include <whooks/systags.h>
#include <whooks/sysclass.h>  
#include <whooks/classes.h>  

extern KillFunc killFunc[] ;	/* asubs.c */

/*************** A class parsing functions ****************/


#include <wh/longtext.h>
#include <wh/table.h>
#include <wh/dna.h>
#include <wh/peptide.h>
#include <wh/matchtable.h>

void parseArrayInit (void)
{
  KEY key ;
  int classe ;

  parseFunc[_VKeySet]  = keySetParse ;
  dumpFunc[_VKeySet]   = keySetDumpFunc ;

  parseFunc[_VDNA]  = dnaParse ;
  dumpFunc[_VDNA]   = dnaDump ;

  parseFunc[_VPeptide]  = peptideParse ;
  dumpFunc[_VPeptide]   = peptideDump ;

  parseFunc[_VLongText]  = longTextParse ;
  dumpFunc[_VLongText]   = longTextDump ;

  parseFunc[_VTableResult]  = tableResultParse ;
  dumpFunc[_VTableResult]   = tableResultDump ;

  parseFunc[_VBaseQuality]  = baseQualityParse ;
  dumpFunc[_VBaseQuality]   = baseQualityDump ;

  parseFunc[_VBasePosition]  = basePositionParse ;
  dumpFunc[_VBasePosition]   = basePositionDump ;

  key = 0 ;
  lexword2key ("MatchTable", &key, _VMainClasses) ;
  if (key)
    {
      classe = KEYKEY(key) ;

      parseFunc[classe]  = matchTableParse ;
      dumpFunc[classe]   = matchTableDump ;
      killFunc[classe]   = matchTableKill ;
    }

  return;
} /* parseArrayInit */

/******************** end of file ************************/
