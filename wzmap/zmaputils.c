/*  File: zmaputils.c
 *  Author: Gemma Barson <gb10@sanger.ac.uk>
 *  Copyright (C) 2011 Genome Research Ltd
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
 * Description: see zmaputils.h
 *-------------------------------------------------------------------
 */


/************************************************************/
/* Includes and defines */

#include <string.h>
#include <wzmap/zmaputils.h>
#include <wzmap/zmap_.h>
#include <wh/dna.h>
#include <wh/smap.h>
#include <wh/smap.h>
#include <wh/smapconvert.h>
#include <whooks/tags.h>
#include <whooks/sysclass.h>
#include <whooks/classes.h>
#include <wh/pick.h>					    /* For pickWord2Class() which should
							       surely be in somwhere like
							       acedb.h as a fundamental key like
							       func. */

#define MKSTEMP_CONST_CHARS        "XACE"	    /* the prefix to use when creating a temp file name */
#define MKSTEMP_REPLACEMENT_CHARS  "XXXXXX"         /* the required string that will be replaced by unique chars when creating a temp file name */



/* Error codes */
typedef enum
  {
    XACE_ZMAP_ERROR_CREATING_TMP               /* error creating temp file */
  } XaceZmapError;



static KEY _Feature ;
static KEY _Tm, _Temporary ;
static KEY _EMBL_feature ;
       KEY _Arg1_URL_prefix ;		/* really ugly to make global */
       KEY _Arg1_URL_suffix ;
       KEY _Arg2_URL_prefix ;
       KEY _Arg2_URL_suffix ;
static KEY _Spliced_cDNA, _Transcribed_gene ;
static KEY _VTranscribed_gene = 0, _VIST = 0 ;

static BOOL reportErrors;

/* verify a generic pointer to be a FeatureMap */
magic_t ZmapFeatureMap_MAGIC = "ZmapFeatureMap";




/* Callback routine for handling DNA mismatches. */
static consMapDNAErrorReturn dna_mismatch_callback(KEY key, int position)
{
  BOOL OK;

  if (!reportErrors)
    return sMapErrorReturnContinue;

  OK = messQuery("Mismatch error at %d in %s.\n"
		 "Do you wish to see further errors?", 
		 position, name(key));
  
  return OK ? sMapErrorReturnContinue : sMapErrorReturnSilent;
}


static BOOL setZone (FeatureMap look, int newMin, int newMax)
{ 
  BOOL res = FALSE ;

  look->zoneMin = newMin ;
  if (look->zoneMin < 0)
    { look->zoneMin = 0 ; res = TRUE ; }

  look->zoneMax = newMax ;
  if (look->zoneMax > look->length)
    { look->zoneMax = look->length ; res = TRUE ; }

  return res ;
}


int zMapOrder (void *a, void *b)     
{
  SEG *seg1 = (SEG*)a, *seg2 = (SEG*)b ;
  int diff ;

  diff = seg1->type - seg2->type ;
  if (diff > 0)
    return 1 ;
  else if (diff < 0)
    return -1 ;

  if ((seg1->type | 0x1) == SPLICED_cDNA_UP)
    {
      if (!(seg1->source * seg2->source))
 	return seg1->source ? -1 : 1 ;

      diff = seg1->source - seg2->source ;
      if (diff)
	return diff ;

      diff = seg1->parent - seg2->parent ;
      if (diff)
	return diff ;

      {
	int x1, x2, y1, y2 ;
	x1 = seg1->data.i >> 14 & 0x3FFF ;
	x2 = seg1->data.i & 0x3FFF ;
	y1 = seg2->data.i >> 14 & 0x3FFF ;
	y2 = seg2->data.i & 0x3FFF ;
	
	if ((x1 - x2) * (y1 - y2) < 0)
	  return x1 < x2 ? -1 : 1 ;
      }
    }
    
  diff = seg1->x1 - seg2->x1 ;
  if (diff > 0)
    return 1 ;
  else if (diff < 0)
    return -1 ;

  diff = seg1->x2 - seg2->x2 ;
  if (diff > 0)
    return 1 ;
  else if (diff < 0)
    return -1 ;

  return 0 ;
}


void zMapClearDNA (FeatureMap look) /* clear colors array to _WHITE */
{ 
  register int i ;
  char *cp ;
  
  zMapInitialise() ;
  
  if (look->colors)
    { cp = arrp(look->colors, 0, char) ;
      for (i = arrayMax(look->colors) ; i-- ;)
	*cp++ = WHITE ;
    }
}


/* This is the guts of the reverse complement, you should note that the code */
/* only knows about the seg coords and only operates on those. If you have   */
/* other features that have coordinates stored else where you need to add    */
/* code to translate them here (see CDS stuff below).                        */
/*                                                                           */
void zMapRC (FeatureMap look)
{
  int top = look->length - 1 ;
  int tmp ;
  char *ci, *cj, ctmp ;
  float *ftmp ;

  sMapFeaturesComplement(look->length, look->segs, look->seqInfo, look->homolInfo) ;
  
  tmp = arrayMax(look->segs) ;
  arrayMax(look->segs) = look->lastTrueSeg ;
	/* avoid mixing temporary and permanent segs */
  arraySort (look->segs, zMapOrder) ;
  arrayMax(look->segs) = tmp ;

  if (look->dnaR)
    {
      Array tmp = look->dna ;
      look->dna = look->dnaR ;
      look->dnaR = tmp ;
    }
  else
    {
      ci = arrp(look->dna, 0, char) ; 
      cj = arrp(look->dna, top, char) ;
      while (ci <= cj)
	{
	  ctmp = *ci ;
	  *ci++ = complementBase[(int)*cj] ;
	  *cj-- = complementBase[(int)ctmp] ;
	}
    }

  if (look->gf.cum)
    {
      tmp = look->gf.max ;
      look->gf.max = top + 1 - look->gf.min ;
      look->gf.min = top + 1 - tmp ;
      ftmp = look->gf.cum ;
      look->gf.cum = look->gf.revCum ; /* already reversed */
      look->gf.revCum = ftmp ;
    }

  look->map->centre = top - look->map->centre ;
  tmp = look->zoneMin ;
  look->zoneMin = look->length - look->zoneMax ;
  look->zoneMax = look->length - tmp ;
  look->origin = look->length + 1 - look->origin ;
  
  look->flag ^= FLAG_COMPLEMENT ;
  zMapClearDNA (look) ;


  /* The sMap must be the same way round as the displayed sequence,
   * so we re-create it here. It would be much easier to re-calculate from
   * scratch to reverse-complement. */
  sMapDestroy(look->smap);

  /* Note that smap uses 1-based coords so we must increment start/stop by 1. */
  if (look->flag & FLAG_COMPLEMENT)
    look->smap = sMapCreate(look->handle, look->seqKey,
			    look->stop + 1, look->start + 1, NULL);
  else
    look->smap = sMapCreate(look->handle, look->seqKey,
			    look->start + 1, look->stop + 1, NULL);

  return ;
}

static void zMapCompToggleTranslation (FeatureMap look)
{
  if ((mapColSetByName (look->map, "Up Gene Translation", -1)) ||
      (mapColSetByName (look->map, "Down Gene Translation", -1)))
    {
      mapColSetByName (look->map, "Up Gene Translation", 2) ;
      mapColSetByName (look->map, "Down Gene Translation", 2) ; 
    }

  return ;
}


/************************************************************/
/* Public functions */

#ifdef DEBUG
/* Prints whitespace for log level (that is, how indented logging currently is, according to how
 * many DEBUG_ENTER and EXITs we've called), and then increase it by the increase amount (can be negative). */
void debugLogLevel(const int increaseAmt)
{
  static int count = 0;
  
  gboolean done = FALSE;
  
  /* If decreasing the count (i.e. printing an exit statement) decrease the log level
   * first. */
  if (increaseAmt < 0)
    {
      count += increaseAmt;
      if (count < 0) count = 0;
      done = TRUE;
    }
  
  /* Print whitespace */
  int i = 0;
  for ( ; i < count; ++i)
    {
      printf("  "); /* print 2 spaces per log level */
    }
  
  if (!done)
    {
      count += increaseAmt;
      if (count < 0) count = 0;
    }
}
#endif


/* Convert a GSList to an argv array. Takes ownership of the list contents
 * and frees the memory used by the list. The caller should free the resulting 
 * argv array and all of its contents */
char** convertListToArgv(GSList *list)
{
  char **argv = g_malloc(g_slist_length(list) * sizeof(char*));
  GSList *item = list;
  int i = 0;
  
  for ( ; item; item = item->next, ++i)
    argv[i] = (char*)(item->data);

  g_slist_free(list);

  return argv;
}


/* Create a file in the temp dir with a unique name. Uses a standard prefix and 
 * replacement chars as well as the given description. Sets the error and returns
 * null if there was any problem. The result should be freed by the caller with g_free. */
char* createTempFile(const char *file_desc, GError **error)
{
  /* On some systems the temp dir name seems to have a trailing slash 
   * and on others not, so if it has one then remove it... */
  char *tmpDir = g_strdup(g_get_tmp_dir());
  
  if (tmpDir[strlen(tmpDir) - 1] == '/')
    tmpDir[strlen(tmpDir) - 1] = '\0';
  
  char *filename = g_strdup_printf("%s/%s_%s_%s", tmpDir, MKSTEMP_CONST_CHARS, file_desc, MKSTEMP_REPLACEMENT_CHARS);
  int fileDesc = mkstemp(filename);
  
  if (!filename || fileDesc == -1)
    {
      g_set_error(error, XACE_ZMAP_ERROR, 1, "Error creating temp file '%s'\n", (filename ? filename : ""));
      g_free(filename);
      filename = NULL;
    }
  else
    {
      close(fileDesc);
    }
  
  return filename;
}


/* Create a directory in the temp dir with a unique name. Uses a standard prefix and 
 * replacement chars as well as the given description. Sets the error and returns
 * null if there was any problem. The result should be freed by the caller with g_free. */
char* createTempDir(const char *dir_desc, GError **error)
{
  /* On some systems the temp dir name seems to have a trailing slash
   * and on others not, so if it has one then remove it... */
  char *tmpDir = g_strdup(g_get_tmp_dir());
  
  if (tmpDir[strlen(tmpDir) - 1] == '/')
    tmpDir[strlen(tmpDir) - 1] = '\0';
  
  char *filename = g_strdup_printf("%s/%s_%s_%s", tmpDir, MKSTEMP_CONST_CHARS, dir_desc, MKSTEMP_REPLACEMENT_CHARS);
  mkdtemp(filename);
  
  if (!filename)
    {
      g_set_error(error, XACE_ZMAP_ERROR, 1, "Error creating temp directory '%s'\n", (filename ? filename : ""));
      g_free(filename);
      filename = NULL;
    }
  
  return filename;
}



/* Sets the start and stop coords for the selected sequence and returns the root
 * parent sequence.
 * Can be called in two ways:
 *     if look is NULL, then just works off the start/end coords and the key,
 *        this happens when we first create an fmap of a sequence.
 *     else tries to keep the active zone etc. in scope for recalculations of.
 *        an existing fmap.
 * Note that the start/stop can be  (n,0)  or  (0,n)  meaning "the sequence from
 * "n" to the end.
 * 
 * Sadly this routine seems to get more opaque as we try to show something sensible...
 * 
 *  */
BOOL setStartStop(KEY seq, int *start_inout, int *stop_inout, KEY *key_out,
                  BOOL extend, FeatureMap look)
{
  BOOL result = FALSE ;
  int start, stop, length ;
  BOOL swop ;
  KEY *key_ptr ;

  start = *start_inout ;
  stop = *stop_inout ;
  if (key_out != NULL)
    key_ptr = key_out ;
  else
    key_ptr = NULL ;

  /* Only check coords if they are _not_ in form (n,0), in this form they can be passed
   * straight through to sMapTreeRoot(). */
  if (!(start == 0) && !(stop == 0))
    {
      swop = (start > stop) ;				    /* ascending order for easier arithmetic. */
      if (swop)
	{
	  int tmp ;
	  tmp = start, start = stop, stop = tmp ;
	}

      if (!look)
	{
	  if (extend)
	    {
	      int start_out = 0, stop_out = 0 ;
	      KEY root_key = KEY_UNDEFINED ;
	      int max_length ;

	      /* If we are constructing a new look, make it a decent length otherwise user may be
	       * given a ridiculously tiny map. We do this by extending the length in the root
	       * object coordinates to make sure that we don't go off the end, note that this
	       * means we do the final sMapTreeRoot() using the root key only. */
	      if (sMapTreeRoot(seq, start, stop, &root_key, &start_out, &stop_out)
		  && (max_length = sMapLength(root_key)))
		{
		  length = stop - start + 1 ;

		  if (length < 20000)
		    length = 20000 ;

		  start = start_out - length ;
		  stop = stop_out + length ;

		  if (start < 1)
		    start = 1 ;
		  if (stop > max_length)
		    stop = max_length ;

		  seq = root_key ;			    /* all coords in root key. */
		}
	    }
	}
      else
	{
	  /* Logic is: keep length the same or bigger, if start/stop are within an
	   * already calculated bit of sequence then just keep all coords the same,
	   * otherwise slide coords up to be centred around new start/stop. */

	  length = stop - start + 1 ;
	  if (length < look->length)
	    length = look->length ;
      
	  if ((start >= (look->start + 1) && stop <= (look->stop + 1)))
	    {
	      start = look->start + 1 ;
	      stop = look->stop + 1 ;
	    }
	  else
	    {
	      /* This doesn't look correct to me.......this seems to extend the coords,
	       * not slide them all up as the comment implies.... */

	      int extend_len = (length - (stop - start + 1)) / 2 ;
	  
	      start -= extend_len * 2 ;
	      stop += extend_len * 2 ;
	    }
	}


      /* smap coords start at 1 so make sure they do, no need to clamp the stop value, this is
       * done by sMapTreeRoot() */
      if (start < 1)
	start = 1 ;
      
      if (swop)
	{
	  int tmp ;
	  tmp = start, start = stop, stop = tmp ;
	}
    }

  /* Ok now find the actual start/stop for the sequence. */
  if (sMapTreeRoot(seq, start, stop, key_ptr, &start, &stop))
    {
      result = TRUE ;

      *start_inout = start ;
      *stop_inout = stop ;
    }

  return result ;
}


/* Create a zmap virtual sequence and call the given callback function on it. 
 *  Returns true if successful, false if there were problems */
FeatureMap zMapCreateLook(KEY seq, int start, int stop, 
                          BOOL reuse, BOOL extend, BOOL noclip,
                          FmapDisplayData *display_data)
{
  FeatureMap look ;
  KEY seqOrig = seq ;

  /* do again - no problem if done already in fMapDisplay */
  if (!setStartStop(seq, &start, &stop, &seq, extend, NULL))
    {
      messerror("zMapCreateLook() - Could not find start/stop of key: \"%s\", start: %d  stop: %d",
		name(seq), start, stop) ;

      return FALSE;
    }


  look = (FeatureMap) messalloc (sizeof(struct FeatureMapStruct)) ;
  look->magic = &ZmapFeatureMap_MAGIC ;
  look->handle = handleCreate () ;
  look->segsHandle = handleHandleCreate (look->handle) ;
  look->seqKey = seq ;
  look->seqOrig = seqOrig ;

  /* Can either get mappings with features clipped to start, stop or get _all_ features that
   * overlap start/stop and hence extend beyond start/stop */
  if (noclip)
    {
      int begin, end ;
      begin = 1, end = 0 ;
      look->smap = sMapCreateEx(look->handle, look->seqKey, begin, end, start, stop, NULL) ;
    }
  else
    look->smap = sMapCreate(look->handle, look->seqKey, start, stop, NULL);


  if (look->smap)
    {
      look->noclip = noclip ;
    }
  else
    {
      messerror("Could not construct smap, key: %s, start: %d  stop: %d", look->seqKey, start, stop) ;
      return FALSE;
    }


  /* This can go once all old dna stuff is removed. */
  look->reportDNAMismatches = FALSE ;


  if (display_data)
    {
      look->keep_selfhomols = display_data->keep_selfhomols ;
      look->keep_duplicate_introns = display_data->keep_duplicate_introns ;
    }


  /* test code....                                                           */
  if (display_data && display_data->methods_dict)
    {
      look->method_set = sMapFeaturesSet(look->handle,
					  display_data->methods_dict,
					  display_data->include_methods); 
      if (!look->method_set)
	return FALSE;

      look->include_methods = display_data->include_methods;
    }
  else
    {
      look->include_methods = FALSE; 
    }

  if (display_data && display_data->sources_dict)
    {
      look->sources_dict = display_data->sources_dict;
      look->include_sources = display_data->include_sources;
    }


  reportErrors = look->reportDNAMismatches;
  if (!(look->dna = sMapDNA(look->smap, 0, dna_mismatch_callback)))
    {
      messfree (look->handle) ;
      messfree (look) ;
      return FALSE;
    }



  /* start/stop can be reversed at this stage for reverse sequence genes... */
  look->zoneMin = 0 ;
  if (stop > start)
    look->zoneMax = stop - start + 1 ;
  else
    look->zoneMax = start - stop + 1 ;
  look->origin = 0 ;

  /* OK, now set start/stop to represent the length of sequence asked for originally. */
  stop = start + arrayMax(look->dna) - 1; 


  
  look->colors = arrayHandleCreate (arrayMax(look->dna), char, look->handle) ;
  arrayMax(look->colors) = arrayMax(look->dna) ;
  //  fmapSetDNAHightlight(prefColour(DNA_HIGHLIGHT_IN_FMAP_DISPLAY)) ;
  //  fMapClearDNA (look) ;		/* clear colors array to _WHITE */
  
  look->chosen = assHandleCreate (look->handle) ;
  look->antiChosen = assHandleCreate (look->handle) ;

  look->length = arrayMax(look->dna) ;
  look->fullLength = sMapLength (seq) ;

  look->start = start-1;
  look->stop = stop-1;


  look->featDict = dictHandleCreate (1000, look->handle) ;
  dictAdd (look->featDict, "null", 0) ; /* so real entries are non-zero */

  look->seqInfo = arrayHandleCreate (32, SEQINFO, look->handle) ;
  look->homolInfo = arrayHandleCreate (256, HOMOLINFO, look->handle) ;
  look->feature_info = arrayHandleCreate(256, FeatureInfo, look->handle) ;

  look->homolBury = bitSetCreate (256, look->handle) ;
  look->homolFromTable = bitSetCreate (256, look->handle) ;

  look->visibleIndices = arrayHandleCreate (64, int, look->handle) ;
  look->preserveColumns = TRUE ;
  look->redrawColumns = TRUE ;
  if (display_data)
    look->destroy_obj = display_data->destroy_obj ;
  else
    look->destroy_obj = FALSE ;
  
  look->mcache = methodCacheCreate (look->handle);
  
  /* must create map before convert, because it adds columns */
  //look->map = mapCreate (fMapRefresh) ; /* map redraw func */
  //look->map->drag = fMapDrag;
    
    
  /* no graph perhaps, so do not attach to graph! */
  //  fMapDefaultColumns (look->map) ;
  
//  look->boxIndex = arrayHandleCreate (64, int, look->handle) ;
//  look->activeBox = 0 ;
//  look->active_seg_index = SEG_INVALID_INDEX ;
//  look->flag = 0 ;
//  look->flag |= FLAG_AUTO_WIDTH ;  /* I honestly cannot understand why you would take that away */
//
  

  return look;
} /* zMapCreateLook */


/* Centre the FeatureMap on "key", perhaps using "from" to decide zoom.
 * 
 * This is an old comment:
 *       "need key and from for Calcul positioning" */
void zMapCentre(FeatureMap look, KEY key, KEY from)
{ 
  int i ;
  float nx, ny ;
  SEG *seg ;
  int top = INT_MAX, bottom = 0 ;

  graphFitBounds (&mapGraphWidth,&mapGraphHeight) ;
  halfGraphHeight = 0.5 * (mapGraphHeight - topMargin) ;
  nx = mapGraphWidth ;
  ny = mapGraphHeight - topMargin - 5 ;			    /* WHY IS IT  "- 5" ???????? */


  /* find top/bottom of feature to be mapped, default to master seg. */
  seg = NULL ;
  if (key)
    {
      SEG *seg1 ;

      for (i = 1 ; i < arrayMax (look->segs) ; ++i)
	{
	  seg1 = arrp (look->segs, i, SEG) ;

	  if (seg1->key == key && seg1->type <= ALLELE_UP)  /* Only use uppable tags to get extent. */
	    {
	      if (top > seg1->x1)
		top = seg1->x1 ;
	      if (bottom < seg1->x2)
		bottom = seg1->x2 ;
	      seg = seg1 ;
	    }
	}
    }

  if (seg == NULL)
    {
      seg = arrp(look->segs, 0 , SEG) ;
      top = seg->x1 ;
      bottom = seg->x2 ;
    }


  /* Centre on object. */
  i = KEYKEY(from)*10 ;
  if (class(from) == _VCalcul  &&  i < seg->x2 - seg->x1 + 1)
    {
      if (seg->type == SEQUENCE_UP)
	look->map->centre = seg->x2 - i ;
      else
	look->map->centre = seg->x1 + i ;
   /* Go to rather high magnification 
      look->map->mag = (ny-5)/ 5000.0 ; */
    }
  else
    look->map->centre = 0.5 * (top + bottom) ;

  /* Set zoom for object. */
  look->map->mag = ny/(bottom - top + 1.0) ;

  /* This seems hokey...if the segs are correct then this shouldn't happen.... */
  if (look->map->mag <= 0)  /* safeguard */
    look->map->mag = 0.05 ;

  /* If "from" probably contains "key" then zoom out slightly for better view of
   * "key" in context. */
  if (from)
    {
      if (class(from) == _VSequence
	  || (_VTranscribed_gene && class(from) == _VTranscribed_gene)
	  || (_VIST && class(from) == _VIST)
	  || (_VProtein && class(from) == _VProtein) /* so that protein homol get centered */
	  || (class(from) == pickWord2Class("Homol_data")))
	look->map->mag /= 1.4 ;
    }

  look->origin = top ;

  /* Bizarrely the zone is held as going from 0 -> length as opposed to 0 -> (length - 1)
   * or 1 -> length, as segs are 0-based we must increment "bottom" */
  setZone(look, top, (bottom + 1)) ;

  return ;
}


void zMapRevComplement(FeatureMap look)
{
  zMapCompToggleTranslation (look) ;
  zMapRC (look) ;

  return ;
}



/* This routine is called whenever an fmap interface routine is called       */
/* to make sure that the fmap package is initialised, initialisation is      */
/* only done on the first call.                                              */
void zMapInitialise (void)
{
  static int isDone = FALSE ;
  KEY key ;

  if (!isDone)
    {
      isDone = TRUE ;

      _Feature = str2tag("Feature") ;
      _EMBL_feature = str2tag("EMBL_feature") ;
      _Arg1_URL_prefix = str2tag("Arg1_URL_prefix");
      _Arg1_URL_suffix = str2tag("Arg1_URL_suffix") ;
      _Arg2_URL_prefix = str2tag("Arg2_URL_prefix") ;
      _Arg2_URL_suffix = str2tag("Arg2_URL_suffix") ;
      _Spliced_cDNA  = str2tag ("Spliced_cDNA") ;
      _Transcribed_gene = str2tag ("Transcribed_gene") ;
      _Tm = str2tag ("Tm") ;
      _Temporary = str2tag ("Temporary") ;
      if (lexword2key ("Transcribed_gene", &key, _VMainClasses))
	_VTranscribed_gene = KEYKEY(key) ;
      if (lexword2key ("IST", &key, _VMainClasses))
	_VIST = KEYKEY(key) ;
    }

  return ;
}


/* If both errors are non-null, prepend the given prefix string onto the source
 * error and propagate it to the dest error. Free src. If either error is null, do nothing. */
void propagatePrefixedError(GError **dest, GError *src, char *formatStr, ...)
{
//  g_propagate_prefixed_error(dest, src, formatStr, ...);  /* Only from GLib v2.16 */

  if (src && dest)
    {
      va_list argp;
      va_start(argp, formatStr);
      
      /* Print the prefix text and args into a new string. We're not sure how much space we need
       * for the args, so give a generous buffer but use vsnprintf to make sure we don't overrun.
       * (g_string_vprintf would avoid this problem but is only available from GLib version 2.14). */
      const int len = strlen(formatStr) + 200;
      char tmpStr[len];
      vsnprintf(tmpStr, len, formatStr, argp);

      va_end(argp);

      /* Append the error message */
      char *resultStr = g_malloc(len + strlen(src->message));
      snprintf(resultStr, len, "%s%s", tmpStr, src->message);
      
      /* Replace the error message text with the new string. */
      g_free(src->message);
      src->message = resultStr;

      g_propagate_error(dest, src);
    }
}


/* Like g_propagate_error but allows src to be null, in which case does nothing */
void propagateError(GError **dest, GError *src)
{
  if (src)
    g_propagate_error(dest, src);
}
