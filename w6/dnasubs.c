/*  File: dnasubs.c
 *  Author: Jean Thierry-Mieg (mieg@mrc-lmb.cam.ac.uk)
 *  Copyright (C) J Thierry-Mieg and R Durbin, 1991
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
 * Description:
 **  Packs and upacks dna arrays.                             
 * Exported functions:
 * HISTORY:
 * Last edited: Mar  2 10:38 2009 (edgrif)
 * * Oct 30 13:37 2001 (edgrif): Add new smap calls to get the dna, a
 *              major change.
 * * Apr 23 17:14 1995 (rd): dnaGet() now gets from a Sequence,
 *		recursively finding the DNA - complex code from fmap.
 * * Oct 23 20:16 1991 (mieg): Change + to n in decodeChar
 * Created: Wed Oct 23 18:10:21 1991 (mieg)
 * CVS info:   $Id: dnasubs.c,v 1.90 2009-03-02 13:27:05 edgrif Exp $
 *-------------------------------------------------------------------
 */

#include <wh/acedb.h>
#include <wh/aceio.h>
#include <wh/a.h>
#include <wh/bs.h>
#include <wh/dna.h>
#include <wh/lex.h>
#include <whooks/classes.h>
#include <whooks/systags.h>
#include <whooks/tags.h>
#include <wh/java.h>
#include <wh/bindex.h>
#include <wh/smap.h>
#include <wh/regular.h>
#include <wh/aceiotypes.h>

/*************************************************************************/

#define MAGIC_PACK 7 
#define MAGIC_PACK_ODD 8 
#define MAGIC_DOUBLE_PACK 6
#define MAGIC_NEW_DOUBLE_PACK 4

/*************************************************************************/

typedef struct { int x, y ; } ExonStruct ;


/*************************************************************************/

static Array dnaUnpackArray(Array pack) ;
static Array dnaDoGet(KEY key, STORE_HANDLE handle, DNAActionOnErrorType error_action, BOOL spliced) ;
static BOOL dnaDoDump (ACEOUT dump_out, Array dna, int debut, int fin, int offset, BOOL line_break) ;
static BOOL dnaDoDumpFastAKey(ACEOUT dump_out, KEY key, BOOL allow_dna_errors, BOOL cds_only, BOOL spliced) ;
static int dnaDoDumpFastAKeySet(ACEOUT dump_out, KEYSET kSet, BOOL cds_only) ;
static Array dnaDoCoordGet(KEY key, STORE_HANDLE dna_handle, 
			   int start, int end, BOOL isDnaGetWithErrors, BOOL spliced) ;
/* this should become redundant..... */
static Array dnaExtractCDS(KEY key, Array whole_dna) ;
static BOOL dnaDumpCStyle (ACEOUT dump_out, KEY key, Array dna, int from, int to, char style) ;

/* mieg: i moved the definitions in w6/dnacode.c to be able to
   include them in makeSCF
   */

static consMapDNAErrorReturn continueDNAcb(KEY key, int position) ;
static consMapDNAErrorReturn queryDNAcb(KEY key, int position) ;
static consMapDNAErrorReturn failDNAcb(KEY key, int position) ;


/*************************************************************************/

#ifdef KS_ADDED
  KEYSET ksAdded = 0 ;
#endif 




/*************************************************************************/

/* the idea is to ensure the terminal zero when copying this avoids
 * a frequent subsequenct realloc and doubling of the copied array. */
Array dnaCopy (Array dna)
{
  int n = arrayMax(dna) ;
  Array cc ;

  if (!dna || dna->size != 1)
    messcrash ("bad call to dnaCopy") ;

  array (dna, n, unsigned char) = 0 ;
  cc = arrayCopy(dna) ;
  arrayMax(dna) = arrayMax(cc) = n ;

  return cc ;  
}

/*************************************************************************/

void reverseComplement (Array dna) 
{
  char  c,  *cp = arrp(dna,0,char), *cq = cp + arrayMax(dna) - 1 ;

  while (cp < cq)
    {
      c = complementBase[(int)*cp] ; 
      *cp++ = complementBase[(int)*cq] ; 
      *cq-- = c  ;
    }

  if (cp == cq)
    *cp = complementBase[(int)*cp] ;

  /* add a terminal zero, 
     I do it again here because we often do
     dnaR = arrayCopy(dna) ; (which looses the terminal zero)
     reverseComplement(dnaR) ;
  */
  array(dna, arrayMax(dna), char) = 0 ;
  --arrayMax(dna) ;

  return ;
}

/**************************************************************/

char * dnaDecodeString(char *buf, char *cp, int n)
{
  int i = n > 0 ? n : 0 ;
  char *cq = buf ;

  while(i--) /* fills to zero past first zero */
    *cq++ = *cp ? dnaDecodeChar[((int)*cp++) & 0xff] : 0 ;

  return buf ;
}

/*************************************/

void dnaDecodeArray(Array a)
{ int n = arrayMax(a) ;
  char *cp = arrp(a,0,char) ;

  cp-- ;
  while(++cp, n--)
    *cp = dnaDecodeChar[((int)*cp) & 0xff] ;
}

/*************************************/

void rnaDecodeArray(Array a)
{
  int n = arrayMax(a) ;
  char *cp = arrp(a,0,char) ;

  cp-- ;
  while(++cp, n--)
    *cp = rnaDecodeChar[((int)*cp) & 0xff] ;
}

/*************************************/

void dnaEncodeString(char *cp)
{
  --cp ;
  while(*++cp)
    *cp = dnaEncodeChar[((int)*cp) & 0x7f] ;
}

/*************************************/

void dnaEncodeArray(Array a)
{
  int n = arrayMax(a) ;
  char *cp = arrp(a,0,char) ;

  cp-- ;
  while(++cp, n--)
    *cp = dnaEncodeChar[((int)*cp) & 0x7f] ;
}


/* Dump the dna one as a single line.
 *
 * For style == 'o' line is dumped as  <sequence> <seqlength> <sequence>
 *
 * For stye == 'u' lines is dumped as  <sequence>
 *
 * if formatted is TRUE then DNA is dumped fasta style with 50 bases per line.
 */
BOOL dnaRawDump(char style, KEY key, Array dna, int from, int to, ACEOUT dump_out, BOOL formatted)
{ 
  BOOL result = FALSE ;
  
  if ((style == 'o' || style == 'u') && arrayExists(dna) && dump_out) /* from/to checked in dnaDoDump() */
    {
      if (from < 0)
	from = 0 ;

      ++to ;
      if (to > arrayMax(dna))
	to = arrayMax(dna) ;

      if (style == 'o')
	aceOutPrint(dump_out, "%s\t%d\t", name(key), (to - from)) ;

      result = dnaDoDump(dump_out, dna, from, to, 0, formatted) ;
    }

  return result ;
}



/* called also from dnacptfastaDump */
BOOL dnaDumpFastA (Array dna, int from, int to, char *text, ACEOUT dump_out)
{ 
  if (!arrayExists(dna))
    return FALSE ;
  
  if (from < 0)
    from = 0 ;

  ++to ;
  if (to > arrayMax(dna))
    to = arrayMax(dna) ;

  aceOutPrint(dump_out, ">%s\n", text) ;
  dnaDoDump(dump_out, dna, from, to, 0, TRUE) ;

  return TRUE ;
} /* dnaDumpFastA */


/************************************/

/* Given a sequence name and its dna sequence, will dump the data in FastA   */
/* format.                                                                   */
BOOL dnaRawDumpFastA(char *seqname, char *dna, ACEOUT dump_out)
{
  enum {FASTA_CHARS = 50} ;
  BOOL result = TRUE ;
  char buffer[FASTA_CHARS + 2] ;			    /* FastA chars + \n + \0 */
  int dna_length  = 0 ;
  int lines = 0, chars_left = 0 ;
  char *cp = NULL ;
  int i ;

  /* n.b. we may be called without a sequence name, e.g. by fmapfeatures     */
  /* blixem code, but the other params are obligatory.                       */
  if (!dna || !(*dna) || !dump_out)
    messcrash("dnaRawDumpFastA() received a NULL parameter: %s%s",
	      (!dna || !(*dna) ? "dna " : ""),
	      (!dump_out ? "dump_out" : "")) ;

  if (aceOutPrint(dump_out, ">%s\n", (seqname ? seqname : "")) != 0)
    result = FALSE ;
  else
    {
      dna_length = strlen(dna) ;
      lines = dna_length / FASTA_CHARS ;
      chars_left = dna_length % FASTA_CHARS ;
      cp = dna ;

      /* Do the full length lines.                                           */
      if (lines != 0)
	{
	  buffer[FASTA_CHARS] = '\n' ;
	  buffer[FASTA_CHARS + 1] = '\0' ; 
	  for (i = 0 ; i < lines && result ; i++)
	    {
	      memcpy(&buffer[0], cp, FASTA_CHARS) ;
	      cp += FASTA_CHARS ;
	      if (aceOutPrint(dump_out, "%s", &buffer[0]) != 0)
		result = FALSE ;
	    }
	}

      /* Do the last line.                                                   */
      if (chars_left != 0)
	{
	  memcpy(&buffer[0], cp, chars_left) ;
	  buffer[chars_left] = '\n' ;
	  buffer[chars_left + 1] = '\0' ; 
	  if (aceOutPrint(dump_out, "%s", &buffer[0]) != 0)
	    result = FALSE ;
	}
    }

  return result ;
}


/************************************/

BOOL dnaDump (ACEOUT dump_out, KEY key) 
     /* DumpFuncType for class _VDNA */
{
  Array unpack, pack;

  pack = arrayGet(key, char, "c") ;
  if (!pack)
    return FALSE ;

  unpack = dnaUnpackArray(pack) ; 

  dnaDoDump (dump_out, unpack, 0, arrayMax(unpack), 9, TRUE) ; /* can we remove the 9 spaces? */

  /*  if (level)
      freeOutClose(level) ;*/

  if (pack != unpack)
    arrayDestroy(pack) ;
  arrayDestroy(unpack) ;
  
  return TRUE ;
} /* dnaDump */

/************************************/

BOOL javaDumpDNA (ACEOUT dump_out, KEY key) 
{
  Array dna, pack;

  pack = arrayGet(key, char, "c") ;
  if (!pack)
    return FALSE ;

  aceOutPrint(dump_out, "?DNA?%s?\t?dna?", freejavaprotect(name(key)));

  dna = dnaUnpackArray(pack) ;  /* dna may be packed on disk */
  dnaDecodeArray (dna) ;        /* decode from A_, T_ ... into ascii */
  array(dna,arrayMax(dna),char) = 0 ; /* ensure 0 terminated */
  --arrayMax(dna) ;
  aceOutPrint(dump_out, "%s", arrp(dna,0,char)) ;
  aceOutPrint(dump_out, "?\n\n");

  if (pack != dna) 
    arrayDestroy(pack);
  arrayDestroy(dna) ;

  return TRUE;
} /* javaDumpDNA */


/************************************/

/* ParseFuncType, called from parse.c to read in a DNA sequence into an Array
 * object. */
ParseFuncReturn dnaParse (ACEIN parse_io, KEY key, char **errtext)
{
  char *cp, c = 0, c1, cutter ;
  KEY seq ;
  Array dna = arrayCreate(5000,char) ;
  register int i = 0 ;
  BOOL isFasta = FALSE ;
  ParseFuncReturn result = PARSEFUNC_ERR ;
  
  aceInCardBack (parse_io) ; /* thus I can parse the obj line again */
  aceInCard (parse_io) ;
  cp = aceInWord (parse_io) ;
  if (*cp++ == '>')
    { 
      isFasta = TRUE ;
      if (!*cp)
	{ cp = aceInWord(parse_io) ; } /* > name is allowed */

      if (!cp || !*cp) 
	return PARSEFUNC_EMPTY ;

      cp = aceInWord(parse_io) ;
      if (cp)
	{ cp = aceInPos(parse_io) ; /* additional comments */
	  if (cp) ; /* we could parse them as a comment in the sequence onject */
	}
    }

  while (aceInCard(parse_io))
    if ((cp = aceInWordCut(parse_io, "\n", &cutter)))
      {
	if (isFasta)
	  { 
	    if (*cp == '>')  /* fasta separator */
	      { aceInCardBack (parse_io) ;
		break ;
	      }
	    if (*cp == ';') /* fasta comment */
	      continue ;
	  }
	while ((c = *cp++))
	  { if ((c1 = dnaEncodeChar[((int)c) & 0x7f]))        /* accepted base codes */
	      array(dna,i++,char) = c1 ;
	    else
	      switch(c)
		{			/* accepted tabulations */
		case '-': case 'x':  case 'X': /* x is vector masking */
		  array(dna,i++,char) = 0 ;
		  break ;
		case '*':  /* phil green padding character, ignore */
		case '\n': case '\t': case ' ':
		case '0': case '1': case '2': case '3': case '4': 
		case '5': case '6': case '7': case '8': case '9': 
		  break;
		default:		/* error in the input file */
		  goto abort ;
		}
	  }
      }
    else
      break ;
  
  if (i)
    {
      lexaddkey (name(key), &seq, _VSequence) ;

      dnaStoreDestroy (key, dna) ; /* updates the sequence obj */
      result = PARSEFUNC_OK ;
    }
  else				/* an empty object is OK */
    {
      arrayDestroy (dna) ;
      result = PARSEFUNC_EMPTY ;
    }

  return result ;

  /* AAAAAggggghhhh the logic here is horrible, how can we be going to abort */
  /* if empty DNA is not an error....dreadful...                             */
  /* Sometime I'll rewrite it to behave...                                   */
 abort:
  arrayDestroy(dna) ;

  if (i || c)
    {
      *errtext = messprintf ("  DNAparse error at line %7d in %.25s : bad char %d:%c\n", 
			     aceInStreamLine(parse_io), name(key), (int)c, c) ;
      return PARSEFUNC_ERR ;
    }
  else
    {
      /* an empty DNA is not an error */
      return PARSEFUNC_EMPTY ;
    }

} /* dnaParse */


/*************************************/


static BOOL dnaDoDump (ACEOUT dump_out, Array dna, int debut, int fin,
		       int offset, BOOL line_break)
{ 
  register int i, k, k1 ;
  register char *cp, *cq ;
  char buffer [4010] ;

  if (!dna || debut < 0 || 
      debut >= fin || fin > arrayMax (dna))
    return FALSE ;

  i = fin - debut ;
  cp = arrp(dna,debut,char) ;
  cq = buffer ;

  while(i > 0)
    {
      cq = buffer ;
      for (k=0 ; k < 4000/(50 + offset + 3) ; k++)
        if (i > 0)
          {
	    for (k1 = offset ; k1-- ;)
	      *cq++ = ' ' ;
            k1 = 50 ;
            while (k1--  && i--)
              *cq++ = dnaDecodeChar[*cp++ & 0xff] ;

	    if (line_break)
	      *cq++ = '\n' ;
	    *cq = 0 ;					    /* null terminate string for print. */
          }

      aceOutPrint(dump_out, "%s", buffer) ;
    }

  return TRUE ;
} /* dnaDoDump */



/*************************************/

static int dnaPackingType(Array dna)
{ int n = arrayMax(dna) ;
  char *cp = arrp(dna,0,char) ;

  if(n<4)   /* no packing */
    return 0 ;
  
  while(n--)
    switch(*cp++)
      { 
      case A_: case T_: case G_: case C_:
	break ;
      default:   /* at least one ambiguous base */
	return 1 ;
      }
  return 2 ;  /* double packing, only a t g c present */
}

/********************/

static void modifyLength (KEY key, int n)
{ KEY seq ;
  OBJ obj ;
  int n1 ;

  lexaddkey (name(key), &seq, _VSequence) ;
  /* this addition is written for the sake of the server client
     it amounts to prevent XREFing while parsing from server */
  if (bIndexTag(seq,_DNA) && (obj = bsCreate (seq)))
    {
      if (bsFindKey (obj, _DNA, key) &&
	  bsGetData (obj, _bsRight, _Int, &n1) &&
	  n == n1)
	{
	  bsDestroy (obj) ;
	  return ;
	}
      bsDestroy (obj) ;
    }

  if ((obj = bsUpdate (seq)))
    {
      bsAddKey (obj, _DNA, key) ;
      bsAddData (obj, _bsRight, _Int, &n) ;
      bsSave (obj) ;
    }

  return ;
}

/********************/

void dnaStoreDestroy(KEY key, Array dna)
{ char *cp , *cq , c1, *base ;
  int m = arrayMax(dna) ;
  char dbp[16] ;
  
  if (dna) modifyLength (key, arrayMax(dna)) ;
  dbp[A_] = 0 ;  dbp[G_] = 1 ;  dbp[C_] = 2 ;  dbp[T_] = 3 ;

  switch(dnaPackingType(dna))
    {
    case 0:   /* no packing , no coding */
      dnaDecodeArray(dna) ;
      break ;

    case 1:  /* 2 bases per byte */
      c1  = (array(dna,0,char)  << 4) | array(dna,1,char) ;
      array(dna,0,char) = m%2 ? MAGIC_PACK_ODD : MAGIC_PACK ;
      array(dna,1,char) = c1 ;  /* first 2 bases */

      /* all the rest but possibly one */
      base =  arrp(dna,0,char) ;
      cp = cq =  arrp(dna,2,char) ;
      m -= 2 ;
      while(m>1)
	{
	  *cp++ = ((*cq) << 4 ) | *(cq + 1) ;
	  cq += 2 ;
	  m -= 2 ;
	}
      if(m)              /* last base in odd case */
	{
	  *cp++ = *cq << 4 ;
	}
      arrayMax(dna) = cp - base ; 
      break ;
      
    case 2:  /* 4 bases per byte */
      cq = arrp(dna,0,char) ;
      c1 =
	(dbp[((int) *cq) & 0xff] << 6 ) |
	  (dbp[((int) *(cq+1)) & 0xff] << 4 ) |
	    (dbp[((int) *(cq+2)) & 0xff] << 2 ) |
	      dbp[(int) *(cq+3)] ;
      array(dna,0,char) =  MAGIC_NEW_DOUBLE_PACK ;
      array(dna,1,char) = m%4 ;
      array(dna,2,char) = c1 ;  /* first 4 bases */

                              /* all the rest but possibly 3 */
      base =  arrp(dna,0,char) ;
      cp =  arrp(dna,3,char) ;
      cq =  arrp(dna,4,char) ;
      m -= 4 ;
      while(m>3)
	{
	  *cp++ =
	    (dbp[((int) *cq) & 0xff] << 6 ) |
	      (dbp[((int) *(cq+1)) & 0xff] << 4 ) |
		(dbp[((int) *(cq+2)) & 0xff] << 2 ) |
		  dbp[((int) *(cq+3)) & 0xff] ;
	  cq += 4 ;
	  m -= 4 ;
	}

      if(m--)              /* last 3 bases */
	{ base-- ; /* to fix arrayMax, without using cp++ */
	  *cp = (dbp[((int) *cq++) & 0xff] << 6 ) ;
	  if(m--)
	    { *cp |= (dbp[((int) *cq++) & 0xff] << 4 ) ;
	      if(m--)
	        *cp |= (dbp[((int) *cq++) & 0xff] << 2) ;
	    }
	}
      arrayMax(dna) = cp - base ; 
      break ;
    }

   arrayStore(key,dna,"c") ;
   arrayDestroy(dna) ;
}

/********************************************************************/

static Array dnaUnpackArray(Array pack)
{ Array unpack ;
  char *cp, *cq ;
  char undoublepack[4] ; /* {A_, T_, G_, C_} ; RD must be static to initialise on SGI */
  char newundoublepack[4] ; /* {A_, G_, C_, T_} ;  RD must be static to initialise on SGI */
  int m, n ;

  undoublepack[0] = A_ ;
  undoublepack[1] = T_ ;
  undoublepack[2] = G_ ;
  undoublepack[3] = C_ ;

  newundoublepack[0] = A_ ;
  newundoublepack[1] = G_ ;
  newundoublepack[2] = C_ ;
  newundoublepack[3] = T_ ;

  if(!pack)
    return 0 ;
  cp = arrp(pack,0,char) ;
  m = 0 ;
  switch(*cp)
    {
    case MAGIC_PACK_ODD: /* 2 bases per byte, odd total */
      m = -1 ; /* fall through */
    case MAGIC_PACK: /* MAGIC packed form */
        n = arrayMax(pack) ;
      if(n<=1)
	  return 0 ;

      m += 2*(n-1) ;  /* skip magic, then every char is 2 base except */
	              /* last char may be a single base in ODD case */
      unpack = arrayCreate(m+1,char) ;  /* ensures zero terminated string */
      array(unpack,m-1,char) = 0 ; /* implies arrayMax = m */
      cp = arrp(pack,0,char) ;  /* so as to start decoding on byte 1 */
      cq = arrp(unpack,0,char) ;
      while(cp++, m--)
	{ *cq++ = (*cp >> 4 ) & (char)(15) ;  /* first half byte */
	       /* &0xf to ensure a left zero after right shift */
	  if(m--)
	    *cq++ = *cp & (char)(15) ;      /* second half byte */
	  else
	    break ;
	}
      return unpack ;

    case MAGIC_DOUBLE_PACK:  /* 4 bases per byte */
        n = arrayMax(pack) ;
      if(n<=2)           /* first byte is MAGIC,  */
	  return 0 ;

      m = array(pack,1,char) ;  /* second byte is max%4 */
      if (!m) m=4 ;  /* favorable case last byte contains 4 bases not zero */
      m = 4*(n-2) - (4-m);
               /* skip magic, residue, then every char is 4 base except */
	              /* last char which holds residue */
      unpack = arrayCreate(m+1,char) ;  /* ensures zero terminated string */
      array(unpack,m-1,char) = 0 ; /* implies arrayMax = m */
      cp = arrp(pack,1,char) ; /* so as to start decoding on byte 2 */
      cq = arrp(unpack,0,char) ;
      while(cp++, m--)
	{ *cq++ = undoublepack[(*cp >> 6 ) & (char)(3)] ;  /* first quarter */
	  if(m--)
	    *cq++ =  undoublepack[(*cp >> 4 ) & (char)(3)] ; 
	  else
	    break ;
	  if(m--)
	    *cq++ =  undoublepack[(*cp >> 2 ) & (char)(3)] ; 
	  else
	    break ;
	  if(m--)
	    *cq++ =  undoublepack[(*cp ) & (char)(3)] ; 
	  else
	    break ;
	}
      return unpack ;

    case MAGIC_NEW_DOUBLE_PACK:  /* 4 bases per byte, new coding, such that ~ is complement */
        n = arrayMax(pack) ;
      if(n<=2)           /* first byte is MAGIC,  */
	  return 0 ;

      m = array(pack,1,char) ;  /* second byte is max%4 */
      if (!m) m=4 ;  /* favorable case last byte contains 4 bases not zero */
      m = 4*(n-2) - (4-m);
               /* skip magic, residue, then every char is 4 base except */
	              /* last char which holds residue */
      unpack = arrayCreate(m+1,char) ;  /* ensures zero terminated string */
      array(unpack,m-1,char) = 0 ; /* implies arrayMax = m */
      cp = arrp(pack,1,char) ; /* so as to start decoding on byte 2 */
      cq = arrp(unpack,0,char) ;
      while(cp++, m--)
	{ *cq++ = newundoublepack[(*cp >> 6 ) & (char)(3)] ;  /* first quarter */
	  if(m--)
	    *cq++ =  newundoublepack[(*cp >> 4 ) & (char)(3)] ; 
	  else
	    break ;
	  if(m--)
	    *cq++ =  newundoublepack[(*cp >> 2 ) & (char)(3)] ; 
	  else
	    break ;
	  if(m--)
	    *cq++ =  newundoublepack[(*cp ) & (char)(3)] ; 
	  else
	    break ;
	}
      return unpack ;

    default:    /* uncoded char form, rare I hope */
      dnaEncodeArray(pack) ;
      return pack ;
    }
}

/******************************************************************/
/************** functions to get DNA from a KEY *******************/

/* Use on an object that is a "dna" object, not a bs object.                 */
/* Called by at least the smap code.                                         */
Array dnaRawGet(KEY key)
{
  Array pack = NULL, unpack = NULL ;
  
  if (class(key) == _VDNA)
    {
      pack = arrayGet(key, char,"c") ;
      if(pack)
	{
	  unpack = dnaUnpackArray(pack) ;
	  if (pack != unpack)
	    arrayDestroy(pack) ;
	}
    }

  return unpack ;
}

/*************************/

static int dnaExonsOrder (void *a, void *b)
{
  ExonStruct *ea = (ExonStruct *)a ;
  ExonStruct *eb = (ExonStruct *)b ;
  return ea->x - eb->x ;
}

void dnaExonsSort (Array a)
{		/* can't just arraySort, because a is in BSunits, not
		   pairs of BSunits */
  int i, n ;
  Array b = 0 ;

  n = arrayMax(a) / 2 ;
  b = arrayCreate (n, ExonStruct) ;
  for (i = 0 ; i < n ; ++i)
    { arrayp(b,i,ExonStruct)->x = arr(a,2*i,BSunit).i ;
      arrp(b,i,ExonStruct)->y = arr(a,2*i+1,BSunit).i ;
    }
  arraySort (b, dnaExonsOrder) ;
  for (i = 0 ; i < n ; ++i)
    { arr(a,2*i,BSunit).i = arrp(b,i,ExonStruct)->x ;
      arr(a,2*i+1,BSunit).i = arrp(b,i,ExonStruct)->y ;
    }
  arrayDestroy (b) ;
}

/*************************/


/* Returns TRUE if the object represented by key contains its own DNA array, FALSE otherwise.
 *
 * We make the terrible assumption that if there is a DNA key it will have dna, its too
 * hard to check for actual DNA without actually doing all the work of loading the array. */
BOOL dnaObjHasOwnDNA(KEY key)
{
  BOOL result = FALSE ;


  if (class(key) == _VDNA)
    {
      result = TRUE ;
    }
  else if (class(key) == _VSequence)
    {
      KEY dummy ;

      result = bIndexGetKey(key, str2tag("DNA"), &dummy) ;
    }

  return result ;
}


/* Get the dna for a given object, this may involve a straight forward
 * reading of the dna from the object or a very much more complicated search
 * of the objects parents and children to get the objects position and then
 * extract the dna for that position.
 * NOTE, default is to get spliced dna, with no mismatch etc. errors. */

Array dnaGet (KEY key)
{
  return dnaDoGet(key, NULL, DNA_QUERY, TRUE) ;
}


Array dnaGetAction(KEY key, DNAActionOnErrorType error_action)
{
  return dnaDoGet(key, NULL, error_action, TRUE) ;
}


/* all the dna calls need rationalising...... */
Array dnaFromObject(KEY key, int dna_start, int dna_end)
{
  Array a ;

  a = dnaDoCoordGet(key, NULL, dna_start, dna_end, FALSE, TRUE) ;

  return a ;
}


Array dnaHandleGet(KEY key, STORE_HANDLE dna_handle)
{
  return dnaDoGet(key, dna_handle, DNA_QUERY, TRUE);
}



/* THIS ROUTINE CAN BE FOLDED IN WITH THE COORDGET ROUTINE..... */

/* Get the dna from the object directly or by smap'ing.
 * 
 * Note that for spliced dna we just smap based on the object, for unspliced dna we map
 * the object into its parent(s) and then get the dna based on that mapping.
 */
static Array dnaDoGet(KEY key, STORE_HANDLE dna_handle, DNAActionOnErrorType error_action, BOOL spliced)
{
  Array ret = NULL ;

  if (class(key) == _VDNA)
    {
      /* Object is a dna array, so just extract the whole thing.             */
      ret = dnaRawGet (key) ;
    }
  else
    {
      /* Object is not DNA so we get the dna from smap of parents etc. 
         Note that an object which contains DNA must still be SMap to
         cover the (slightly odd) case of an object with both DNA tag and 
         Source_exons. */
      SMap *smap ;
      KEY target_key = KEY_UNDEFINED ;
      int start = 0, end = 0 ;
      BOOL got_length = FALSE ;

      if (spliced)
	{
	  /* Prepare to map the object. */
	  target_key = key ;
	  start = 1 ;
	  got_length = (end = sMapLength(key)) ;
	}
      else
	{
	  /* Prepare to map the object relative to its parent(s) */
	  got_length = (sMapTreeRoot(key, 1, 0, &target_key, &start, &end)) ;
	}

      if (got_length)
	{
	  if ((smap = sMapCreate(0, target_key, start, end, NULL)))
	    {
	      sMapDNAErrorCallback func_cb ;

	      switch (error_action)
		{
		case DNA_CONTINUE:
		  func_cb = continueDNAcb ;
		  break ;

		case DNA_FAIL:
		  func_cb = failDNAcb ;
		  break ;

		case DNA_QUERY:
		default:
		  func_cb = queryDNAcb ;
		  break ;
		}

	      ret = sMapDNA(smap, dna_handle, func_cb) ;

	      sMapDestroy(smap);
	    }
	}
    }
  
  /* add a terminal zero, useful for unix calls, but not 
     really correct because there can be internal 0's */
  if (ret)
    {
      array(ret, arrayMax(ret), char) = 0 ;
      --arrayMax(ret) ;
    }
  
  return ret;
}



/* NEW ROUTINE WHICH ALLOWS US TO SPECIFY COORDS FOR THE DNA..... */
/* Get the dna from the object directly or by smap'ing.
 * 
 * Note that for spliced dna we just smap based on the object, for unspliced dna we map
 * the object into its parent(s) and then get the dna based on that mapping.
 * 
 */
static Array dnaDoCoordGet(KEY key, STORE_HANDLE dna_handle, 
			   int start, int end, BOOL isDnaGetWithErrors, BOOL spliced)
{
  Array ret = NULL;

  if (class(key) == _VDNA)
    {
      /* Object is a dna array, so just extract the whole thing. */

      /* UNSURE IF WE NEED TO DO THE COORDS THING HERE OR NOT.....THINK ABOUT IT,
       * MAY MAKE SMAPDNA CODE SIMPLER..... */

      ret = dnaRawGet(key) ;
    }
  else
    {
      /* Object is not DNA so we get the dna from smap of parents etc. 
       * Note that an object which contains DNA must still be SMap to
       * cover the (slightly odd) case of an object with both DNA tag and 
       * Source_exons. */
      SMap *smap ;
      KEY target_key = KEY_UNDEFINED ;
      int dna_start = 0, dna_end = 0 ;
      BOOL got_length = FALSE ;

      /* Get start/end of objects dna. For something that has exons and we want the
       * spliced length then just take its own length and map the object itself before
       * getting the dna. If we want the unspliced dna then we must map relative to the
       * objects parent. */
      if (spliced)
	{
	  target_key = key ;
	  dna_start = 1 ;
	  got_length = (dna_end = sMapLength(key)) ;
	}
      else
	{
	  got_length = (sMapTreeRoot(key, 1, 0, &target_key, &dna_start, &dna_end)) ;
	}

      if (got_length)
	{
	  int dna_length = (dna_end - dna_start + 1) ;

	  if (start <= dna_length && end <= dna_length)
	    {
	      start = dna_start + (start - 1) ;
	      if (end == 0)
		end = dna_length ;
	      end = dna_end - (dna_length - end) ;
	      
	      if ((smap = sMapCreate(0, target_key, start, end, NULL)))
		{
		  sMapDNAErrorCallback func_cb ;

		  if (isDnaGetWithErrors)
		    func_cb = continueDNAcb ;
		  else
		    func_cb = queryDNAcb ;

		  ret = sMapDNA(smap, dna_handle, func_cb) ;
		  
		  sMapDestroy(smap) ;
		}
	    }
	}
    }
  
  /* add a terminal zero, useful for unix calls, but not 
     really correct because there can be internal 0's */
  if (ret)
    {
      array(ret, arrayMax(ret), char) = 0 ;
      --arrayMax(ret) ;
    }
  
  return ret;
}


/**********************************************************/
/**********************************************************/

ACEOUT dnaFileOpen (STORE_HANDLE handle)
{
  static char directory[DIR_BUFFER_SIZE]= "";
  static char filename[FIL_BUFFER_SIZE] = "";
  ACEOUT dna_out = NULL;

  dna_out = aceOutCreateToChooser ("Choose a file for export in fasta format",
				   directory, filename, "dna", "w", handle);

  return dna_out;
} /* dnaFileOpen */


/**********************************************************/

/* DNA FastA format dumping code.
 * There are a couple of "cover" functions that just set the right flags and
 * call the main dnaDoDumpFastAKey() function.
 * The functions to dump DNA for whole keysets also in the end call the
 * dnaDoDumpFastAKey() function. */

BOOL dnaDumpFastAKey (ACEOUT dump_out, KEY key)
{ 
  BOOL result;

  result = dnaDoDumpFastAKey (dump_out, key, FALSE, FALSE, TRUE) ;
  
  return result;
} /* dnaDumpFastAKey */

BOOL dnaDumpFastAKeyWithErrors (ACEOUT dump_out, KEY key)
{
  BOOL result ;

  result = dnaDoDumpFastAKey(dump_out, key, TRUE, FALSE, TRUE) ;

  return result ;
} /* dnaDumpFastAKeyWithErrors */


BOOL dnaZoneDumpFastAKey(ACEOUT dump_out, KEY key,
			 DNAActionOnErrorType error_action, BOOL cds_only, BOOL spliced, int x1, int x2, char style)
{ 
  BOOL result = FALSE ;
  Array a = NULL ;

  a = dnaDoGet(key, NULL, error_action, spliced) ;
   
  /* if cds then truncate.....
   * I assume that if there is no CDS then its an error and we shouldn't return anything. */
  if (a && cds_only)
    {
      Array cds_dna = NULL ;

      cds_dna = dnaExtractCDS(key, a) ;

      arrayDestroy(a) ;
      a = NULL ;

      /* Try messerror if no cds in object but may get too annoying for user.*/
      if (!cds_dna)
	messerror("Object %s has no CDS tag so CDS DNA cannot be dumped", name(key)) ;
      else
	a = cds_dna ;
    }

  if (a)
    {
      if (x1 > arrayMax(a)) 
	x1 = arrayMax (a) ;
      if (x2 > arrayMax(a)) 
	x2 = arrayMax (a) ;
      if (x1 == 0)
	x1 = 1 ;
      if (x2 == 0)
	x2 = arrayMax(a) ;
      if (x1 == -1)
	x1 = arrayMax(a) ;
      if (x2 == -1 || x2 == 0)
	x2 = arrayMax(a) ;
      if (x1 > x2)
	{
	  reverseComplement (a) ;
	  x1 = arrayMax(a) - x1 + 1 ;
	  x2 = arrayMax(a) - x2 + 1 ;
	}

      if (x1 != x2)  /* in this case i do not know the orientation, or both were outside 1/max */
	{
	  x1-- ; x2-- ;
	  if (style == 'C' || style == 'B')
	    {
	      result = dnaDumpCStyle (dump_out, key, a, x1, x2, style) ;
	    }
	  else if (style == 'u' || style == 'o')
	    {
	      result = dnaRawDump(style, key, a, x1, x2, dump_out, FALSE) ;
	    }
	  else
	    {
	      result = dnaDumpFastA(a, x1, x2, name(key), dump_out) ;
	    }
	}

      arrayDestroy (a) ;
    }

  return result ;
} /* dnaZoneDumpFastAKey */

static BOOL dnaDoDumpFastAKey(ACEOUT dump_out, KEY key, BOOL allow_dna_errors, BOOL cds_only, BOOL spliced)
{
  return dnaZoneDumpFastAKey(dump_out, key, allow_dna_errors, cds_only, spliced, 0, 0, 0) ;
} /* dnaDoDumpFastAKey */

int dnaDumpFastAKeySet(ACEOUT dump_out, KEYSET kSet)
{
  int result = 0 ;

  result = dnaDoDumpFastAKeySet(dump_out, kSet, FALSE) ;

  return result ;
}


int dnaDumpCDSFastAKeySet(ACEOUT dump_out, KEYSET kSet)
{
  int result = 0 ;

  result = dnaDoDumpFastAKeySet(dump_out, kSet, TRUE) ;

  return result ;
}

static int dnaDoDumpFastAKeySet(ACEOUT dump_out, KEYSET kSet, BOOL cds_only)
{
  KEYSET alpha ;
  int i, n = 0 ;

  if (!keySetExists(kSet) || keySetMax(kSet) == 0)
    return 0 ;

  alpha = keySetAlphaHeap (kSet, keySetMax(kSet)) ;
  for (i = 0 ; i < keySetMax(alpha) ; ++i)
    {
      if (dnaDoDumpFastAKey(dump_out, keySet(alpha, i), FALSE, cds_only, TRUE))
	++n ;
    }

  keySetDestroy (alpha) ;

  messout ("I wrote %d sequences", n) ;

  return n ;
} /* dnaDumpFastAKeySet */


/**********************************************************/



/* If object referred to by key contains cds tags then the cds portion of
 * whole_dna will be extracted into a new array and returned, otherwise NULL
 * is returned. */
static Array dnaExtractCDS(KEY key, Array whole_dna)
{
  Array result = NULL ;
  OBJ obj ;

  /* Does object have a CDS tag ? */
  if (bIndexTag(key, _CDS)
      && (obj = bsCreate(key))
      && bsFindTag(obj, _CDS))
    {
      int dna_start, dna_end, cds_start, cds_end ;
      int i, j, cds_length ;

      dna_start = cds_start = 1 ;
      dna_end = cds_end = arrayMax(whole_dna) ;

      if (bsGetData(obj, _bsRight, _Int, &cds_start))
	{
	  bsGetData(obj, _bsRight, _Int, &cds_end) ;
	}
      bsDestroy(obj) ;
	      
      if (cds_start < dna_start || cds_end > dna_end)
	{
	  messerror("The start/end of the CDS in object %s"
		    " is outside of the spliced DNA coordinates for that object"
		    " (spliced DNA start = %d & end = %d,"
		    " CDS start = %d & end = %d)."
		    " Make sure the CDS positions have been specified"
		    " in spliced DNA, not Source_exon, coordinates."
		    " CDS start/end positions have been reset to start/end of"
		    " spliced DNA.",
		    name(key), dna_start, dna_end, cds_start, cds_end) ;

	  cds_start = dna_start ;
	  cds_end = dna_end ;
	}

      cds_length = cds_end - cds_start + 1 ;

      result = arrayCreate(cds_length, char) ;

      for (i = (cds_start - 1), j = 0 ; i < cds_end ; i++, j++)
	{
	  array(result, j, char) = array(whole_dna, i, char) ; 
	}

      /* The below snippet of code is copied from other existing routines in */
      /* this file. To be honest I hate this....its yuck....                 */

      /* add a terminal zero, useful for unix calls, but not 
	 really correct because there can be internal 0's */
      array(result, arrayMax(result), char) = 0 ;
      --arrayMax(result) ;
    }

  return result ;
}


/************************************************************/
/************************************************************/

/*
   save BaseQuality and BasePosition information for assemblies in two new
   classes, each an array class of unsigned char (for position this is
   a delta-packing).
	BaseQuality is a measure of base quality: unsigned char
   	BasePosition is the change in position in the trace
   Both autogenerate an entry in the Sequence object of the same name, 
   with the array length following the key like DNA. 
*/

BOOL baseQualityDump (ACEOUT dump_out, KEY k)
     /* DumpFuncType for class _VBaseQuality */
{ 
  Array a ;
  int n, x, i ; 
  unsigned char *bc ;

  a = arrayGet (k, unsigned char, "c") ;
  if (!a || !arrayMax(a))
    { arrayDestroy (a) ;
      return FALSE ;
    }

  n = arrayMax (a) ;
  bc = arrp (a, 0, unsigned char) ;
  while (n)
    { for (i = 0 ; i < 50 && n ; i++, --n)
	{ x = *bc++ ;
	  aceOutPrint (dump_out, "%d ", x) ;
	}
      aceOutPrint (dump_out, "\n") ;
    }
  aceOutPrint (dump_out, "\n") ;

  arrayDestroy (a) ;
 
  /* if (level)
     freeOutClose (level) ;*/
 
  return TRUE ;
} /* baseQualityDump */


BOOL basePositionDump (ACEOUT dump_out, KEY k)
     /* DumpFuncType for class _VBasePosition */
{
  Array a ;
  int n, x, i ; 
  signed char *bc ;

  a = arrayGet (k, signed char, "c") ;
  if (!a || !arrayMax(a))
    { arrayDestroy (a) ;
      return FALSE ;
    }

  n = arrayMax (a) ;
  bc = arrp (a, 0, signed char) ;
  x = 0 ;
  while (n)
    { for (i = 0 ; i < 50 && n ; i++, --n)
	{ x += *bc++ ;
	  aceOutPrint (dump_out, "%d ", x) ;
	}
      aceOutPrint (dump_out, "\n") ;
    }
  aceOutPrint (dump_out, "\n") ;

  arrayDestroy (a) ;

  /* if (level)
     freeOutClose (level) ;*/

  return TRUE ;
} /* basePositionDump */

ParseFuncReturn baseQualityParse (ACEIN parse_io, KEY key, char **errtext)
     /* ParseFuncType for class _VBaseQuality */
{ 
  Array a = arrayCreate (1000, unsigned char) ;
  int x, n = 0 ;
  ParseFuncReturn result ;

  if (class(key) != _VBaseQuality)
    messcrash ("baseQualityParse called on a non-BaseQuality key") ;

  while (aceInCard (parse_io) && aceInInt (parse_io, &x))
    { 
      do 
	{ if (x < 0 || x > 255)
	    goto abort ;
	  array(a, n++, unsigned char) = x ; 
	} while (aceInInt (parse_io, &x)) ;

      if (aceInWord(parse_io))
	goto abort ;
    }

  if (aceInWord(parse_io))
    goto abort ;

  /* If we have some data in the input string then save it.                  */
  if (arrayMax(a))
    {
      KEY seq ;
      OBJ obj ;

      /* Try to save a reference to the quality data in the sequence object. */
      lexaddkey (name(key), &seq, _VSequence) ;
      if ((obj = bsUpdate (seq)))
	{
	  /* got object for updating....                                     */
	  KEY _Quality ;

	  lexaddkey ("Quality", &_Quality, 0) ;
	  bsAddKey (obj, _Quality, key) ;
	  bsAddData (obj, _bsRight, _Int, &n) ;
	  bsSave (obj) ;
	}

      arrayStore (key, a, "c") ;
      result = PARSEFUNC_OK ;
    }
  else
    result = PARSEFUNC_EMPTY ;
  
  arrayDestroy (a) ;
  return result ;

 abort:
  *errtext = messprintf("Error parsing BaseQuality %s at line %d (not an int 0-255)", 
			name(key), aceInStreamLine(parse_io)) ;
  arrayDestroy (a);

  return PARSEFUNC_ERR ;
} /* baseQualityParse */

/************************************************************/

ParseFuncReturn basePositionParse (ACEIN parse_io, KEY key, char **errtext)
     /* ParseFuncType for class _VBasePosition */
{ 
  Array a = arrayCreate (1000, signed char) ;
  int dx, x, old = 0, n = 0 ;
  ParseFuncReturn result ;

  if (class(key) != _VBasePosition)
    messcrash ("basePositionParse called on a non-BasePosition key") ;

  while (aceInCard (parse_io) && aceInInt (parse_io, &x)) 
    { 
      do 
	{ dx = x - old ;
	  if (dx > 127) dx = 127 ;
	  if (dx < -128) dx = -128 ;
	  array (a, n++, signed char) = dx ;
	  old += dx ;
	} while (aceInInt (parse_io, &x)) ;

      if (aceInWord (parse_io))
	goto abort ;
    }
  if (aceInWord (parse_io))
    goto abort ;
  
  if (arrayMax(a))
    { KEY seq ;
      OBJ obj ;

      lexaddkey (name(key), &seq, _VSequence) ;
      if ((obj = bsUpdate (seq)))
	{ KEY _SCF_Position ;

	  lexaddkey ("SCF_Position", &_SCF_Position, 0) ;
	  bsAddKey (obj, _SCF_Position, key) ;
	  bsAddData (obj, _bsRight, _Int, &n) ;
	  bsSave (obj) ;
	}

      arrayStore (key, a, "c") ;
      result = PARSEFUNC_OK ;
    }
  else
    result = PARSEFUNC_EMPTY ;

  arrayDestroy (a) ;
  return result ;

 abort:
  *errtext = messprintf("Non-integer parsing BasePosition %s at line %d", 
			name(key), aceInStreamLine(parse_io)) ;
  arrayDestroy (a);

  return PARSEFUNC_ERR ;
} /* basePositionParse */


/**********************************************************/

static BOOL dnaDumpCStyle (ACEOUT dump_out, KEY key, Array dna, int from, int to, char style)
{
  int ii = to - from + 1 ;
  char buf[2] ;
  char *cp = name(key) ;

  if (!dna || from < 0 || from >= to || to >= arrayMax(dna))
    return FALSE ;
  if (style == 'C')
    dnaDecodeArray (dna) ;

  buf[0] = 'N' ; buf[1] = class(key) ;
  aceOutBinary ( dump_out, buf, 2) ;
  aceOutBinary ( dump_out, cp, strlen(cp) + 1) ;

  buf[0] = '\n' ;
  buf[1] = 'c' ;
  aceOutBinary ( dump_out, buf,2) ;

  aceOutBinary ( dump_out, (char*) &ii, 4) ;

  buf[0] = '\n' ;
  buf[1] = 'D' ;
  aceOutBinary ( dump_out, buf,2) ;

  aceOutBinary ( dump_out, arrp (dna, from, char), ii) ;

  buf[0] = '\n' ;
  buf[1] = '#' ;
  aceOutBinary ( dump_out, buf,2) ;

  return TRUE ;
}


BOOL dnaDumpKeyCstyle (ACEOUT dump_out, KEY key)
{
  Array dna, pack = arrayGet(key, char, "c") ;
  int ii ;

  if (!pack)
    return FALSE ;

  dna = dnaUnpackArray(pack) ;  /* dna may be packed on disk */
  array(dna,arrayMax(dna),char) = 0 ; /* ensure 0 terminated */
  --arrayMax(dna) ;

  ii = arrayMax(dna) ;
  dnaDumpCStyle (dump_out, key, dna, 0, ii - 1, 'C') ;

  if (pack !=dna)
    arrayDestroy(pack);
  arrayDestroy(dna) ;

  return TRUE;
}




/* 
 *                 Internal functions
 */



static consMapDNAErrorReturn queryDNAcb(KEY key, int position)
{
  BOOL OK ;
  
  OK = messQuery("Mismatch error at %d in %s.\n"
		 "Do you wish to see further errors?", 
		 position, name(key));
  
  return OK ? sMapErrorReturnContinueFail : sMapErrorReturnFail;
}

static consMapDNAErrorReturn continueDNAcb(KEY key, int position)
{
  return sMapErrorReturnSilent ;
}

static consMapDNAErrorReturn failDNAcb(KEY key, int position)
{
  return sMapErrorReturnFail ;
}





/*********************** eof ******************************/

