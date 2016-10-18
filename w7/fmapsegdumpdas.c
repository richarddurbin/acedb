/*  File: fmapsegdumpdas.c
 *  Author: Ed Griffiths (edgrif@sanger.ac.uk)
 *  Copyright (c) J Thierry-Mieg and R Durbin, 2003
 *-------------------------------------------------------------------
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
 *-------------------------------------------------------------------
 * This file is part of the ACEDB genome database package, written by
 * 	Richard Durbin (Sanger Centre, UK) rd@sanger.ac.uk, and
 *	Jean Thierry-Mieg (CRBM du CNRS, France) mieg@kaa.crbm.cnrs-mop.fr
 *
 * Description: Functions to output DAS format information, e.g. for
 *              DNA, Features (GFF basically) and other DAS stuff.
 *              
 * Exported functions: See das.h
 * HISTORY:
 * Last edited: Nov 23 12:05 2011 (edgrif)
 * Created: Tue Jan 28 14:01:03 2003 (edgrif)
 * CVS info:   $Id: fmapsegdumpdas.c,v 1.3 2012-10-30 09:58:54 edgrif Exp $
 *-------------------------------------------------------------------
 */

#include <glib.h>
#include <wh/regular.h>
#include <wh/aceio.h>
#include <wh/acedb.h>
#include <wh/lex.h>
#include <whooks/classes.h>
#include <w7/fmapsegdump.h>
#include <wh/dna.h>


/* This is work in progress, I'm just going to lash up some routines to enable command line
 * output of DAS to do testing */


/* Data passed to our callback routine when its called from the seg dumping code to output
 * a feature. */
typedef struct
{
  ACEOUT fo ;
  int *level ;
  STORE_HANDLE handle ;
  GString *attributes ;
  GString *value ;
} DasGFFDataStruct, *DasGFFData ;



static void indent(ACEOUT fo, int level) ;
static void makeTags(STORE_HANDLE handle, char *tag, char *attributes,
		     char **start_tag_out, char **end_tag_out) ;
static void dumpTagStart(ACEOUT fo, int *level, char *tag) ;
static void dumpTagEnd(ACEOUT fo, int *level, char *tag) ;
static char *getDASHeader(void) ;
static BOOL writeDASFeature(void *app_data,
			    char *seqname, char *seqclass,
			    KEY method_obj_key, char *method,
			    BOOL mapped,
			    FMapSegDumpAttr seg_attrs,
			    KEY subpart_tag,
			    KEY parent, SegType parent_segtype, char *parent_type,
			    KEY feature, SegType feature_segType, char *feature_type,
			    KEY source_key,
			    FMapSegDumpBasicType seg_basic_type,
			    char *source,
			    int start, int end,
			    float *score, char strand, char frame,
			    char *attributes, char *comments,
			    FMapSegDumpLine dump_data,
			    char **err_msg_out) ;



/*
 * --------------------  EXTERNAL FUNCTIONS  --------------------
 */


/* Called from gifcommand.c to do a dump of a virtual sequence in DASGFF format. */
void dasGFF(ACEIN command_in, ACEOUT result_out, FeatureMap look)
{
  int x1, x2 ;
  int version = 2 ;
  char *word ;
  STORE_HANDLE handle = handleCreate() ;
  DICT *sourceSet = dictHandleCreate (16, handle) ;
  DICT *featSet = dictHandleCreate (16, handle) ;
  DICT *methodSet = dictHandleCreate (16, handle) ;
  BOOL raw_methods = FALSE ;
  BOOL include_source  = FALSE;
  BOOL include_feature = FALSE;
  BOOL include_method  = FALSE;
  BOOL isList = FALSE ;
  ACEOUT fo = NULL, dump_out = NULL ;
  char *das_start, *das_end,
    *gff_start, *gff_end, *gff_atts,
    *seg_start, *seg_end, *seg_atts ;
  int level ;
  KEY refSeq = KEY_UNDEFINED, seqKey ;
  BOOL reversed ;
  int offset, key_start, key_end, feature_start, feature_end ;
  BOOL result ;
  DasGFFData cb_data ;
  enum {DAS_BUFFER_INITLEN = 256} ;			    /* Vague guess at initial length. */


  /* Bother, we need some common code to analyse all the method include style junk.... */


  messAssert((command_in && result_out && verifyFeatureMap(look, dasFeatures))) ;

  /* by default output goes to same place as normal command results */
  dump_out = aceOutCopy (result_out, 0);


  /* Decide which of these to support....... */
  while (aceInCheck (command_in, "w"))
    { 
      word = aceInWord (command_in) ;
      if (strcmp (word, "-coords") == 0
	  && aceInInt (command_in, &x1)
	  && aceInInt (command_in, &x2)
	  && (x2 != x1))
	{
	  ;
#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
	  setZoneUserCoords (look, x1, x2) ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

	}
      else if (strcmp (word, "-file") == 0
	       && aceInCheck (command_in, "w")
	       && (fo = aceOutCreateToFile (aceInPath (command_in), "w", 0)))
	{
	  /* replace default output with file output */
	  aceOutDestroy (dump_out);
	  dump_out = fo;
	}
      else if (strcmp (word, "-refseq") == 0
	       && aceInCheck (command_in, "w") 
	       && (lexword2key (aceInWord (command_in), &refSeq, _VSequence)))
	{ ; }
      else if (strcmp (word, "-rawmethods") == 0)
	{
	  raw_methods = TRUE ;
	}
      else if (strcmp (word, "-version") == 0
	       && aceInInt (command_in, &version))
	{ ; }
      else if (strcmp (word, "-list") == 0)
	{ 
	  isList = TRUE ;
	}
      else if (strcmp (word, "-source") == 0
	       && aceInCheck (command_in, "w"))
	{

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
	  addToSet (command_in, sourceSet) ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */
	  ;
	}
      else if (strcmp (word, "-feature") == 0
	       && aceInCheck (command_in, "w"))
	{

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
	  addToSet (command_in, featSet) ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */
	  ;
	}
      else
	goto usage ;
    }


  aceOutPrint(dump_out, "%s\n", getDASHeader()) ;

  level = -1 ;

  makeTags(handle, "DASGFF", NULL, &das_start, &das_end) ;
  dumpTagStart(dump_out, &level, das_start) ;

  gff_atts = hprintf(handle, "version=\"%s\" href=\"%s\"", "1.0", "url") ;
  makeTags(handle, "GFF", gff_atts, &gff_start, &gff_end) ;
  dumpTagStart(dump_out, &level, gff_start) ;


  FMapSegDumpRefSeqPos(look, version,
		       &refSeq, &seqKey, &reversed,
		       &offset, &key_start, &key_end,
		       &feature_start, &feature_end) ;

  level-- ;						    /* dump stuff is same level as GFF tag. */
  seg_atts = hprintf(handle, "id=\"%s\" start=\"%d\" stop=\"%d\" version=\"%s\" \" label=\"%s\"",
		     nameWithClass(seqKey), key_start, key_end,
		     timeShow(timeParse("today")), name(seqKey)) ;
  makeTags(handle, "SEGMENT", seg_atts, &seg_start, &seg_end) ;
  dumpTagStart(dump_out, &level, seg_start) ;


  /* Set up data/structs that writeDASFeature() will need, note we use GStrings
   * because they are appendable and reusable which should help with performance. */
  cb_data = messalloc(sizeof(DasGFFDataStruct)) ;
  cb_data->fo = dump_out ;
  cb_data->level = &level ;
  cb_data->handle = handle ;
  cb_data->attributes = g_string_sized_new(DAS_BUFFER_INITLEN) ;
  cb_data->value = g_string_sized_new(DAS_BUFFER_INITLEN) ;

  result = FMapSegDump(writeDASFeature, (void *)cb_data,
		       look, version, 
		       seqKey, offset, feature_start, feature_end,
		       FMAPSEGDUMP_FEATURES, NULL, raw_methods, NULL,
		       sourceSet, featSet, methodSet,
		       include_source, include_feature, include_method,
		       FALSE, FALSE,
		       FALSE, FALSE) ;

  g_string_free(cb_data->attributes, TRUE) ;
  g_string_free(cb_data->value, TRUE) ;
  messfree(cb_data) ;


  dumpTagEnd(dump_out, &level, seg_end) ;
  level++ ;
  dumpTagEnd(dump_out, &level, gff_end) ;
  dumpTagEnd(dump_out, &level, das_end) ;

  aceOutDestroy (dump_out);
  handleDestroy (handle) ;

  if (result)
    aceOutPrint(result_out, "// FMAP_FEATURES %s %d %d\n", freeprotect (name(refSeq)), 
		look->zoneMin+1+offset, look->zoneMax+offset) ;
  return ;

usage:
  aceOutDestroy (dump_out);

  aceOutPrint (result_out, "// gif seqfeatures error: usage: SEQFEATURES [-coords x1 x2] ") ;
  aceOutPrint (result_out, "[-file fname] [-refseq sequence] [-version 1|2] ") ;
  aceOutPrint (result_out, "[-list] [-source source(s)] [-feature feature(s)]\n") ;
  handleDestroy (handle) ;


  return ;
}


/**************************************************************/


/* Write out sequence DNA in DAS format. */
void dasDNA (ACEIN command_in, ACEOUT result_out, FeatureMap look)
{
  KEY key ;
  int x1, x2 ;
  ACEOUT fo = 0, dump_out = 0;
  STORE_HANDLE handle = handleCreate() ;
  char *word ;
  char *dna_start, *dna_end,
    *seq_start, *seq_end, *seq_atts,
    *raw_start, *raw_end, *raw_atts ;
  int level ;

  messAssert((command_in && result_out && verifyFeatureMap(look, dasDNA))) ;

  /* by default output goes to same place as normal command results */
  dump_out = aceOutCopy (result_out, 0);

  while (aceInCheck (command_in, "w"))
    {
      word = aceInWord (command_in) ;
      if (strcmp (word, "-coords") == 0
	  && aceInInt (command_in, &x1)
	  && aceInInt (command_in, &x2)
	  && (x2 != x1))
	{

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
	  setZoneUserCoords (look, x1, x2) ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

	}
      else if (strcmp (word, "-file") == 0
	       && aceInCheck (command_in, "w")
	       && (fo = aceOutCreateToFile (aceInPath (command_in), "w", 0)))
	{
	  /* write to file instead of default output */
	  aceOutDestroy (dump_out);
	  dump_out = fo;
	}
      else
	goto usage ;
    }


  key = look->seqKey ; x1 = look->start ; x2 = look->stop ;

  /* If we extend this to do spliced and dna stuff then
   * WE NEED TO CHECK OF look->seq is same as look->seqorig and if not, we should output
   * look->seqorig for the spliced dna _only_.....*/

  x1++, x2++ ;						    /* fMap keeps 0-based coords and we need to
							       report in 1-based coords. */

  /* Needs to be removed in the end.... */
  aceOutPrint (result_out, "// FMAP_DNA %s %d %d\n", freeprotect (name(key)), x1, x2) ;


  /* Dump the dna in DAS format. */
  aceOutPrint(dump_out, "%s\n", getDASHeader()) ;

  level = -1 ;

  makeTags(handle, "DASDNA", NULL, &dna_start, &dna_end) ;
  dumpTagStart(dump_out, &level, dna_start) ;

  seq_atts = hprintf(handle, "id=\"%s\" start=\"%d\" stop=\"%d\" version=\"%s\"",
		     name(key),
		     x1, x2, "X.XX") ;
  makeTags(handle, "SEQUENCE", seq_atts, &seq_start, &seq_end) ;
  dumpTagStart(dump_out, &level, seq_start) ;

  raw_atts = hprintf(handle, "length=\"%d\"", (x2 - x1 + 1)) ;
  makeTags(handle, "DNA", raw_atts, &raw_start, &raw_end) ;
  dumpTagStart(dump_out, &level, raw_start) ;

  /* Won't be indented at the moment..... */
  dnaRawDump('u', key, look->dna, look->zoneMin, look->zoneMax-1, dump_out, TRUE) ;

  dumpTagEnd(dump_out, &level, raw_end) ;
  dumpTagEnd(dump_out, &level, seq_end) ;
  dumpTagEnd(dump_out, &level, dna_end) ;


  aceOutDestroy (dump_out);
  handleDestroy(handle) ;

  return;

 usage:
  aceOutDestroy (dump_out);
  handleDestroy(handle) ;
  aceOutPrint (result_out, "// dasdna error: usage: dasdna [-coords x1 x2] [-file fname]\n") ;

  return;
}




/*
 * --------------------  INTERNAL FUNCTIONS  --------------------
 */

/* Makes the start and end xml format tags. */
static void makeTags(STORE_HANDLE handle,
		     char *tag, char *attributes, char **start_tag_out, char **end_tag_out)
{
  if (start_tag_out)
    {
      *start_tag_out = hprintf(handle, "<%s%s%s>",
			       tag,
			       attributes ? " " : "",
			       attributes ? attributes: "") ;
    }

  if (end_tag_out)
    {
      *end_tag_out = hprintf(handle, "</%s>", tag) ; 
    }

  return ;
}


/* Indenting only needed for human output really, should not do this for normal output. */
static void indent(ACEOUT fo, int level)
{
  int i ;

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
  /* THIS BOMBS OUT IN THE GLIB STUFF SOMEWHERE....THIS ROUTINE IS A HACK TO COPE WITH THIS.... */
  aceOutPrint(fo, "%*s", level * 3, " ") ;
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */

  for (i = 0 ; i < (level * 3) ; i++)
    {
      aceOutPrint(fo, "%s", " ") ;
    }

  return ;
}


/* routines to dump start, end or start & end tags. */
static void dumpTagStart(ACEOUT fo, int *level, char *tag)
{
  (*level)++ ;

  indent(fo, *level) ;

  aceOutPrint(fo, "%s\n", tag) ;

  return ;
}

static void dumpTagEnd(ACEOUT fo, int *level, char *tag)
{
  indent(fo, *level) ;

  aceOutPrint(fo, "%s\n", tag) ;

  (*level)-- ;

  return ;
}

static void dumpTagComplete(ACEOUT fo, int *level, 
			    char *tag_name, char *tag_attributes, char *tag_value)
{
  (*level)++ ;

  indent(fo, *level) ;

  aceOutPrint(fo, "<%s%s%s>%s</%s>\n",
	      tag_name,
	      tag_attributes ? " " : "",
	      tag_attributes ? tag_attributes : "",
	      tag_value,
	      tag_name) ;

  (*level)-- ;

  return ;
}


/* Return standard format DAS header. */
static char *getDASHeader(void)
{
  char *header = "<?xml version=\"1.0\" standalone=\"no\"?>\n"
    "<!DOCTYPE DASDNA SYSTEM \"http://www.biodas.org/dtd/dasdna.dtd\">" ;

  return (header) ;
}


/* Callback routine (see wh/fmapsegdump.h) which gets called from FMapSegDump() so we output
 * seg information in the format we like. NB segType added for debugging in
 * WriteGFFLine which is the other callback routine. */
static BOOL writeDASFeature(void *app_data,
			    char *seqname, char *seqclass,
			    KEY method_obj_key, char *method,
			    BOOL mapped,
			    FMapSegDumpAttr seg_attrs,
			    KEY subpart_tag,
			    KEY parent, SegType parent_segtype, char *parent_type,
			    KEY feature, SegType feature_segtype, char *feature_type,
			    KEY source_key,
			    FMapSegDumpBasicType seg_basic_type,
			    char *source,
			    int start, int end,
			    float *score, char strand, char frame,
			    char *attributes, char *comments,
			    FMapSegDumpLine dump_data,
			    char **err_msg_out)
{
  BOOL result = FALSE ;
  DasGFFData cb_data = (DasGFFData)app_data ;
  ACEOUT dump_out = cb_data->fo ;
  STORE_HANDLE handle = cb_data->handle ;
  int *level = cb_data->level ;
  char *feature_start, *feature_end ;
  GString *tag_attributes = cb_data->attributes ;
  GString *tag_value = cb_data->value ;


  g_string_sprintf(tag_attributes, "id=\"%s\" label=\"%s\"",
		   source, name(feature)) ;
  makeTags(handle, "FEATURE", tag_attributes->str, &feature_start, &feature_end) ;
  dumpTagStart(dump_out, level, feature_start) ;


  g_string_sprintf(tag_attributes, "id=\"%s:%s\" category=\"%s:%s\"",
		   method, source,
		   method, source) ;
  dumpTagComplete(dump_out, level, "TYPE", tag_attributes->str, source) ;

  g_string_sprintf(tag_attributes, "id=\"%s\"", method) ;
  dumpTagComplete(dump_out, level, "METHOD",  tag_attributes->str, method) ;

  g_string_sprintf(tag_value, "%d", start) ;
  dumpTagComplete(dump_out, level, "START",  NULL, tag_value->str) ;

  g_string_sprintf(tag_value, "%d", end) ;
  dumpTagComplete(dump_out, level, "END",  NULL, tag_value->str) ;

  g_string_sprintf(tag_value, "%f", *score) ;
  dumpTagComplete(dump_out, level, "SCORE",  NULL, tag_value->str) ;

  g_string_sprintf(tag_value, "%c", strand) ;
  dumpTagComplete(dump_out, level, "ORIENTATION", NULL, tag_value->str) ;

  g_string_sprintf(tag_value, "%c", frame) ;
  dumpTagComplete(dump_out, level, "FRAME", NULL, tag_value->str) ;

  dumpTagComplete(dump_out, level, "NOTE", NULL, attributes) ;

  /* Need code to separate out homol stuff from attributes string..... */

  dumpTagEnd(dump_out, level, feature_end) ;

  return result ;
}


