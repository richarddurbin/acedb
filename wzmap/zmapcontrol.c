/*  File: zmapcontrol.c
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
 * 	Richard Durbin (Sanger Institute, UK) rd@sanger.ac.uk, and
 *	Jean Thierry-Mieg (CRBM du CNRS, France) mieg@kaa.crbm.cnrs-mop.fr
 *
 * Description: Functions for starting a ZMap child process
 *-------------------------------------------------------------------
 */

/************************************************************/
/* Includes and defines */

#include <wh/zmap.h>
#include <wh/display.h>
#include <wh/dna.h>
#include <wzmap/zmap_.h>
#include <wzmap/zmaputils.h>
#include <wzmap/zmapsocket.h>
#include <wh/chrono.h>
#include <wh/lex.h>
#include <wh/bindex.h>
#include <wh/method.h>
#include <wh/smap.h>
#include <wh/smapmethods.h>
#include <wh/acedb.h>
#include <wh/aceio.h>
#include <wh/session.h>
#include <wh/command.h>
#include <whooks/classes.h>
#include <whooks/sysclass.h>
#include <whooks/tags.h>
#include <gtk/gtk.h>
#include <math.h>

#define ZMAP_SLEEP_ENV_VAR         "ZMAP_SLEEP"        /* this environment variable forces zmap to sleep for a given number of seconds on startup */
#define ZMAP_STYLES_FILE_ENV_VAR   "ZMAP_STYLES_FILE"  /* if this environment variable is set it tells xace where to look for the zmap styles file */
#define ZMAP_DEFAULT_STYLES_FILE   "/.ZMap/styles.ini" /* default location in the home directory to look for the styles file */
#define USE_METHODS_FLAG           "false"             /* "true" if using Methods; "false" if using new zmap styles */
#define SEPARATOR                  " ; "               /* list separator */
#define ZMAP_PREDEFINED_STYLES     " ; DNA ; 3 Frame ; 3 Frame Translation ; Locus ; GeneFinderFeatures ; Show Translation ; Assembly path" /* these special predefined styles need to be hacked in to the config file in order to be displayed in zmap */
#define GENEFINDER_STYLES          " ; ATG ; hexExon ; hexIntron ; GF_coding_seg ; GF_ATG ; hexExon_span ; RNA ; SPLICED_cDNA ; GF_splice"  /* these genefinder styles need to be hacked in to the config file in order to be displayed in zmap */
#define ZMAP_PREDEFINED_COLUMNS    ZMAP_PREDEFINED_STYLES""GENEFINDER_STYLES
#define ZMAP_CONFIG_FILE_NAME      "ZMap"              /* a config file created by us */
#define ZMAP_LOG_FILE_NAME         "zmap.log"          /* the log file created by zmap */

/* Error domain */
#define XACEZMAP_ERROR g_quark_from_string("Xace ZMap")

/* Error codes */
typedef enum
{
  XACEZMAP_ERROR_CONFIG,       /* Error creating zmap config file */
  XACEZMAP_ERROR_FEATUREMAP,   /* Error creating FeatureMap for zmap */
  XACEZMAP_ERROR_GET_METHODS,  /* Error getting the list of methods */
  XACEZMAP_ERROR_GET_COLUMNS  /* Error constructing the list of columns */
} XaceZmapError;



typedef struct _MethodData
{
  GQuark nameQuark;
  double priority;
} MethodData;


/* template for the zmap config file; has substitution operators for the 
 * following:
 *   %s - reference sequence name
 *   %s - sources (i.e. sequence name)
 *   %s - columns list
 *   %s - predefined columns list
 *   %s - [columns] stanza
 *   %s - blixem featuresets list
 *   %s - source name (i.e. sequence name)
 *   %s - featuresets list
 *   %s - predefined columns list
 *   %s - styles file key-value pair (or empty string if n/a)
 *   %s - host address
 *   %d - port
 *   %s - "true" to use old-style Methods or "false" to use the new zmap styles
 */
#define ZMAP_CONFIG_TEMPLATE "\
\n\
[ZMap]\n\
sequence = %s\n\
#start = \n\
#end = \n\
sources = %s\n\
cookie-jar = ~/.otter/ns_cookie_jar\n\
pfetch = pfetch\n\
pfetch-mode = pipe\n\
legacy-styles=true\n\
#show-mainwindow = false\n\
stylename-from-methodname=true\n\
columns=%s%s\n\
%s\n\
[ZMapWindow]\n\
colour-column-highlight = cornsilk\n\
colour-frame-2 = #e6e6ff\n\
canvas-maxsize = 10000\n\
feature-line-width = 1\n\
feature-spacing = 4.0\n\
colour-frame-1 = #e6ffe6\n\
colour-frame-0 = #ffe6e6\n\
\n\
[blixem]\n\
script = blixemh\n\
#featuresets = curated_features ; Coding Transcripts ; Known CDS Transcripts ; Novel CDS Transcripts ; Putative and NMD\n\
featuresets = %s\n\
dna-featuresets = EST_Human ; EST_Mouse ; EST_Other ; vertebrate_mRNA ; BLAT_EST_BEST ; BLAT_EST_OTHER ; BLAT_NEMBASE ; BLAT_Caen_EST_OTHER ; \n\
protein-featuresets = SwissProt ; TrEMBL ; wublastx_worm ; wublastx_briggsae ; wublastx_remanei ; wublastx_brenneri ; wublastx_japonica ; wublastx_pristionchus ; wublastx_fly ; wublastx_yeast ; wublastx_human ; wublastx_slimSwissProt ; wublastx_slimTrEmbl\n\
homol-max = 0\n\
config-file = ~/.blixemrc\n\
scope = 200000\n\
\n\
[%s]\n\
sequence = true\n\
featuresets=%s%s\n\
%surl = acedb://any:any@%s:%d?use_methods=%s\n\
navigatorsets = scale ; genomic_canonical ; locus\n\
writeback = false\
"

/******** giface submenu  ********/

/* General GIF commands. */
enum {GIF_QUIET = 300} ;

enum {GIF_FMAP_RECALC = 400, GIF_FMAP_ACTIVEZONE, GIF_DO_SMAP} ;

enum {GIF_SMAP_SEQGET = 700, GIF_SMAP_SEQFEATURES} ;

enum {GIF_DASDNA = 800, GIF_DASTYPES, GIF_DASGFF} ;


/* NOTE: If you change the functionality of these options
 * you have to update the documentation file whelp/Giface.html
 * to reflect your changes.
 * ALSO NOTE: If you add any new commands you must increment the number in the
 * first member of the options[] array to be the total number of commands.
 */
static FREEOPT options[] =
{
  /* First element _MUST_ be {(array elements - 1), "cmdline prompt"}, we fill in the number
   * automatically. */
  { 0,  "acedb-gif" },

  /* General functions. */
  { -1,  "Quit" },					    /* quit command simulates EOF */
  { '?', "? : this list of commands.  Multiple commands ';' separated accepted" },
  { GIF_QUIET,
   "quiet -on | -off :\n"
   "\tTurn quiet mode on or off, when \"on\" commands produce only their output\n"
   "\twithout any extra comments (e.g. \"NNN Active Objects\" etc.).\n"
   "\tQuiet mode makes parsing command output easier because either the first line\n"
   "\tis an acedb comment because the command failed or there are no acedb comments in the output.\n"
   "\t-off    (default) verbose output\n"
   "\t-on     no comments unless there is an error.\n"},
  { 'd', "dimensions [-D displaytype] x y : image size in pixel for all or the specified displaytype" }, 
  { 'D', "display [-D displaytype] [-view view] [class key] : graphic display [for the current key]" },
  { 'B', "psdump [-e] [-c]  filename : save active graph as postscript" },
  { 'A', "gifdump filename : save active graph as a gif" },
  { 'M', "mouseclick x y: simulate a left button click on the active graph" },

  { 'm', "map map [-view view] [-coords x1 x2] [-hideheader] [-whole]: does not write - use gif after" },

  { 'P', "pmap [-clone <name>] | [-contig <name> <coord>] : physical map" }, 

  /* fmap/sequence operations. */
  { 'S', "seqget [[class:]object [-coords x1 x2]] [-noclip] [-source | -method]: sets current sequence for future ops on same line - if no name then get from active graph\n"
    "\t-coords  specify start/end of section of sequence to get\n"
    "\t-noclip  specify that features should not be clipped to x1, x2 but have their full extent mapped."
    "\t[-source source(s)]  specify a set of sources from which objects should be included/excluded in the virtual sequence."
    "\t[-method methods(s)] specify a set of methods from which objects should be included/excluded in the virtual sequence."
    "\tN.B. source and method must be the last option."
  },
  { 'X', "seqdisplay [-visible_coords v1 v2] [-new] [-title title] : works on current sequence, use gif or ps to actually dump the display\n"
    "\t-new                 make a new sequence, current one is preserved (for use only with xremote currently)\n"
    "\t-title               give your own title to the sequence"
  },
  { 'n', "seqdna [-file fname] <-coords x1 x2> : works on current sequence <coords deprecated - use seqget>" },
  { 'f', "seqfeatures [-quiet] [-file fname] [-version 1|2] [-list [features | keys]] [-source source(s)] [-feature feature(s)] <-coords x1 x2> : works on current sequence, quiet means only GFF in output, source(s)/feature(s) are '|' separated lists, <coords deprecated - use seqget>" },
  { 'g', "seqactions  [-dna] [-gf_features] [-hide_header] [-rev_comp] : works on current sequence" },
  { 'c', "seqcolumns {-on name} {-off name} {-list}: works on active sequence" },
							    /* NB seqcolumns will work BEFORE seqdisplay() now */
  { 'a', "seqalign  [-file fname] [-coords x1 x2] : must follow a seqdisplay <or sequence, which is deprecated>" },
  { 's', "sequence sequence [-coords x1 x2] : <deprecated - use seqget ; seqdisplay>" },
  { GIF_DO_SMAP, "smap -coords x1 x2 -from class:object -to class:object : maps cordinates between related sequences." },
  { GIF_FMAP_RECALC, "seqrecalc causes the sequence to be recalculated for the active graph." },
  { GIF_FMAP_ACTIVEZONE, "seqgetactivezone retrieves the current fmap graphs active zone coords,\n"
    "\tcoords are in the reference frame of the root of the sequence object displayed." },

  
  { 'e', "EMBL filename : embl dump keyset to filename.embl" },

  {500, "pepget <protein | peptide> [-coords x1 x2] : sets current protein for future ops on same line" },  
  {501, "pepseq [-file fname] <-coords x1 x2> : exports current petide sequence" },
  {502, "pepalign [-file fname] <-coords x1 x2> : exports homols of current peptide" },

  { 'K', "makemaps -gmap | -pmap | -cmap | -alpha | -all | -seqmap file | -seqclonemap file : makes cached maps and sorted class keysets" },
  { 'U', "update [-all] : add one official update, or all" },

  /* To support DAS format output. */
  {GIF_DASTYPES, "dastypes : gets types for current sequence.." },
  {GIF_DASDNA, "dasdna : gets dna for current sequence.." },
  {GIF_DASGFF, "dasgff : gets features for current sequence.." }

} ;

enum {OPTIONS_SIZE = (UtArraySize(options) - 1)} ;


/************************************************************/
/* Private functions */



/************************************************/ 

/* Call the given function on all methods in the given FeatureMap */
BOOL zMapConvert(FeatureMap look, SmapCallback callback_func, gpointer callback_data)
{
  BOOL result = FALSE ;
  Array segs = NULL ;
  Array oldSegs = NULL ;
  STORE_HANDLE oldSegsHandle ;
  static Array units = 0 ;
  OBJ	obj ;
  KEY M_Transposon, M_SPLICED_cDNA, M_RNA ;
  KEY M_TRANSCRIPT, M_Pseudogene, M_Coding ;

  chrono("zMapConvert") ;
 
  zMapInitialise () ;

  M_Coding = defaultSubsequenceKey ("Coding", BLUE, 2.0, TRUE) ;
  M_RNA = defaultSubsequenceKey ("RNA", GREEN, 2.0, TRUE) ;
  M_Pseudogene = defaultSubsequenceKey ("Pseudogene", DARKGRAY, 2.0, TRUE) ;
  M_Transposon = defaultSubsequenceKey ("Transposon", DARKGRAY, 2.05, FALSE) ;
  M_TRANSCRIPT = defaultSubsequenceKey ("TRANSCRIPT", DARKGRAY, 2.1, FALSE) ;
  M_SPLICED_cDNA = defaultSubsequenceKey ("SPLICED_cDNA", DARKGRAY, 2.2, FALSE) ;


  if (!look || !look->seqKey)
    messcrash ("No sequence for MapConvert") ;

  if (look->pleaseRecompute == FMAP_RECOMPUTE_YES)
    look->pleaseRecompute = FMAP_RECOMPUTE_NO ;

  if (!(obj = bsCreate (look->seqKey)))
    {
      messout ("No data for sequence %s", name(look->seqKey)) ;
      chronoReturn() ;
      return FALSE ;
    }
  bsDestroy (obj) ;



  /* Initialise handles, arrays and bitsets.                                 */
  oldSegs = look->segs ;
  oldSegsHandle = look->segsHandle ;
  look->segsHandle = handleHandleCreate (look->handle) ;
  segs = look->segs = arrayHandleCreate(oldSegs ? arrayMax(oldSegs) : 128,
					SEG, look->segsHandle) ;
  if (oldSegs)
    arrayMax(oldSegs) = look->lastTrueSeg ;

  units = arrayReCreate (units, 256, BSunit) ;
  int i = 0;
  for (i=0; i<arrayMax(look->homolInfo); i++)
    arrayDestroy(arr(look->homolInfo, i, HOMOLINFO).gaps);

  look->seqInfo = arrayReCreate (look->seqInfo, 32, SEQINFO) ;
  look->homolInfo = arrayReCreate (look->homolInfo, 256, HOMOLINFO) ;
  look->feature_info = arrayReCreate(look->feature_info, 256, FeatureInfo) ;

  look->homolBury = bitSetReCreate (look->homolBury, 256) ;
  look->homolFromTable = bitSetReCreate (look->homolFromTable, 256) ;

  result = sMapFindMethods(look->smap, look->seqKey, look->seqOrig, look->start, 
			    look->stop, look->length,
			    oldSegs, (look->flag & FLAG_COMPLEMENT), look->mcache,
			    look->dna, look->method_set, look->include_methods, look->featDict, 
			    look->sources_dict, look->include_sources,
			    look->segs,
			    look->seqInfo, look->homolInfo, look->feature_info,
			    look->homolBury, look->homolFromTable,
			    look->keep_selfhomols, look->keep_duplicate_introns,
                           look->segsHandle, callback_func, callback_data);
  
  return result ;
}


/* Destroy a FeatureMap */
void featureMapDestroy (FeatureMap look)
{
  if (!look)
    return;
  
  //verifyFeatureMap(look, featureMapDestroy) ;

  handleDestroy (look->handle) ;

  arrayDestroy (look->dna) ;
  arrayDestroy (look->dnaR) ;

  arrayDestroy (look->sites) ;	/* sites come from dnacpt.c */
  stackDestroy(look->siteNames) ;

  arrayDestroy (look->multiplets) ;
  arrayDestroy (look->oligopairs) ;

  mapDestroy (look->map) ;

  /* If this fmap is the "current" (i.e. non-preserved fmap), then remove it from the display
   * systems list of "current" displays. */
  //if (look->graph == displayGetGraph("FMAP"))
  //  displaySetGraph("FMAP", GRAPH_NULL) ;


//  if (graphActivate(look->graph))
//    {
//      graphAssRemove (&GRAPH2FeatureMap_ASSOC) ;
//      graphAssRemove (&MAP2LOOK_ASSOC) ;
//      graphRegister (DESTROY, 0) ; /* deregister */
//    }
//
  
  //  fMapTraceDestroy (look) ;
  //  fMapOspDestroy (look) ;

  /* Sometimes fmap is called to display a temporarily created object, e.g.  */
  /* cDNA display. So now we are finished we need to delete the object.      */
  if (look->destroy_obj)
    {
      OBJ obj ;

      if ((obj = bsUpdate(look->seqKey)) != NULL)
	bsKill(obj) ;
    }


  if (graphActivate (look->selectGraph))   /* from fmapgene.c */
    graphDestroy () ;

  //  if (look == selectedfMap)
  //    selectedfMap = 0 ;
  look->magic = 0 ;

  messfree (look) ;

  return;
} /* featureMapDestroy */


/* Totally assumes that the string coming in is of the form:
 * 
 * "some text|some more text|and|more text"
 * 
 * the text is split _only_ on the "|" symbols so the above would be parsed into:
 * 
 * "some text", "some more text", "and", "more text"
 * 
 * 
 *  */
void zmapAddToSet (ACEIN command_in, DICT *dict)
{ 
  char cutter, *cutset, *word ;

  cutset = "|" ;

  while ((word = aceInWordCut (command_in, cutset, &cutter)))
    { 
      dictAdd (dict, word, 0) ;

      if (cutter != '|')
	break ;
    }

  return ;
}


/* Check the length of the sequence to be viewed, main use of this is to     */
/* give the user a chance to stop if they have clicked on a huge sequence.   */
/*                                                                           */
/* Threshold for asking user if they want to continue is currently set at    */
/* a MBase.                                                                  */
/*                                                                           */
static BOOL checkSequenceLength(KEY seq)
{
  BOOL result = FALSE ;
  KEY parent = KEY_UNDEFINED ;
  int start = 0, stop = 0 ;
  enum {MAX_SEQ_SPAN = 1048576} ;

  if (!(result = sMapTreeRoot(seq, 1, 0, &parent, &start, &stop)))
    messerror("Cannot map this object.") ;
  else
    {
      int len = abs(stop - start) + 1 ;

      if (len > MAX_SEQ_SPAN)
	result = messQuery("You have asked to view a sequence that is %d bases long,"
			   " do you really want to continue ?", len) ;
      else
	result = TRUE ;
    }

  return result ;
}


/* Get all the methods from the given gif command by creating a virtual sequence */
static FeatureMap gifGetMethods(ACEIN command_in, 
                                ACEOUT result_out,
                                KEY key,
                                KEY from,
                                SmapCallback callback_func,
                                gpointer callback_data,
                                GError **error)
{
  BOOL status = FALSE;

  STORE_HANDLE handle = NULL ;
  DICT *methods_dict  = NULL ;
  DICT *sources_dict  = NULL;
  BOOL noclip = FALSE ;
  BOOL include_methods  = FALSE;
  BOOL include_sources  = FALSE;

  messAssert(command_in && result_out) ;

  zMapInitialise() ;

  handle = handleCreate();

  FmapDisplayData *fmap_data = (FmapDisplayData *)halloc(sizeof(FmapDisplayData), handle) ;
  fmap_data->destroy_obj = FALSE ;

  /* now process the command-line options */
  if (sources_dict)
    {
      fmap_data->sources_dict = sources_dict ;
    }
  else
    {
      fmap_data->sources_dict = NULL ;
    }
    
  if (methods_dict)
    {
      fmap_data->methods_dict = methods_dict ;
    }
  else
    {
      fmap_data->methods_dict = NULL ;
    }
  
  fmap_data->include_sources = include_sources ;
  fmap_data->include_methods = include_methods ;

  FeatureMap look = NULL;
  BOOL reverse_strand = FALSE;

  /* recurse up to find topmost link - must do here rather than
   * wait for zMapCreateLook() so we can check if we have it already */
  KEY seq = key;
  OBJ obj;
  gboolean shiftOrigin = FALSE;
  
  if (class(key) == _VSequence || bIndexTag (key, str2tag("SMAP")))
    seq = key ;
  else if (class(key) == _VDNA)
    lexReClass (key, &seq, _VSequence) ;
  else if ((obj = bsCreate (key)))
    { 
      if ((!bsGetKey (obj, _Sequence, &seq))
	  && bsGetKey (obj, str2tag("Genomic_sequence"), &seq))
	shiftOrigin = TRUE ; 
      bsDestroy (obj) ;
    }

  if (key != seq)
    {
      from = key ;
      key = seq ;
    }

  status = (seq && iskey(seq) == 2);	/* iskey tests a full object */
  status &= checkSequenceLength(seq);

  int start = 1, stop = 0;
  sMapTreeRoot (key, 1, 0, &seq, &start, &stop) ;
 
  /* smap keeps coords in the original order but zmap needs them to be in
   * ascending order. */
  if (start > stop)
    {
      int tmp ;
      tmp = start ;
      start = stop ;
      stop = tmp ;
      reverse_strand = TRUE ;
    }

  look = zMapCreateLook(seq, start, stop, FALSE, TRUE, noclip, NULL) ;
  
  if (look)
    {
      /* must create map before convert, because it adds columns */
      look->map = mapCreate (NULL) ; /* map redraw func */
      look->seqKey = seq;
      look->seqOrig = key ;
      look->map->min = 0 ;
      look->map->max = look->length ;

      status = zMapConvert(look, callback_func, callback_data);
    }
  else
    {
      status = FALSE;
    }

  if (status && look)
    {
      zMapCentre (look, key, from) ;
    }

//  if (shiftOrigin)
//    fMapSetOriginFromKey(look, from) ;
//
//  if (!getenv("ACEDB_PROJECT") && (look->zoneMax - look->zoneMin < 1000))
//    {
//      mapColSetByName(look->map, "DNA Sequence", TRUE) ;
//      mapColSetByName(look->map, "Coords", TRUE) ;
//    }
//
//
//  if (reverse_strand && !ZMAP_COMPLEMENTED(look))	    /* revcomp for up strand features. */
//    zMapRevComplement(look) ;

  if (!status)
    g_set_error(error, XACEZMAP_ERROR, XACEZMAP_ERROR_FEATUREMAP, "Error creating FeatureMap.\n");

  if (look && !status)
    {
      featureMapDestroy(look);
      look = NULL;
    }

  return look;
}


/* Function to compare 2 methods by priority. Returns a negative value if
 * a < b, positive if a > b or 0 if values are equal. Given pointers 
 * are pointers to MethodData structs */
static gint compareMethodsByPriority(gconstpointer a, gconstpointer b)
{
  const MethodData *data1 = (const MethodData*)a;
  const MethodData *data2 = (const MethodData*)b;
  
  int result = 0;
  double tolerance = 0.000001;
  
  if (data1->priority - data2->priority > tolerance)
    result = 1;
  else if (data1->priority - data2->priority < -1 * tolerance)
    result = -1;
  else
    result = 0;
  
  return result;
}


/* Function to compare 2 methods by name. Compares for equality only;
 * returns 0 if equal, non-zero otherwise. */
static gint compareMethodsByName(gconstpointer a, gconstpointer b)
{
  const MethodData *data1 = (const MethodData*)a;
  const MethodData *data2 = (const MethodData*)b;
  
  int result = 1;
  
  if (data1->nameQuark == data2->nameQuark)
    result = 0;
  
  return result;
}


/* This callback is called for each METHOD object. It constructs a
 * list of method names. The pointer to the METHOD is added to the GSList
 * unless it already exists in the list, in which case this does nothing.
 * Note that the user data is a pointer-to-a-pointer of the GSList.
 * Note that items are prepended to the list because this is more efficient
 * than appending, so the list order is the reverse order of the order items
 * are added. */
static void constructFeaturesetsList(METHOD *meth, gpointer user_data)
{
  if (!meth)
    return;

  GSList **featuresets = (GSList**)(user_data);

  if (featuresets)
    {
      MethodData *methodData = g_malloc(sizeof *methodData);
      methodData->nameQuark = g_quark_from_string(meth->name);
      methodData->priority = meth->priority;
            
      if (!g_slist_find_custom(*featuresets, methodData, compareMethodsByName))
        *featuresets = g_slist_prepend(*featuresets, methodData);
    }
  else
    {
      g_critical("Internal error: could not construct featuresets list; list is null\n");
    }
}



static void constructColumnsString(GSList *columns,
                                   GHashTable *columnFsLists,
                                   char **columnNames_out,
                                   char **columnStanza_out,
                                   GError **error)

{
  GString *columnNames = g_string_new(NULL);
  GString *columnStanza = g_string_new("");
  GSList *col = columns;
  const int separatorLen = strlen(SEPARATOR);

  for ( ; col; col = col->next)
    {
      const char *colName = g_quark_to_string(GPOINTER_TO_INT(col->data));
      g_string_append_printf(columnNames, "%s%s", colName, SEPARATOR);

      /* If there are multiple featuresets in this column, also add it to the 
       * columns stanza */
      GSList *columnFsList = g_hash_table_lookup(columnFsLists, col->data);

      if (g_slist_length(columnFsList) > 1)
        {
          g_string_append_printf(columnStanza, "%s=", colName);
          GSList *fs = columnFsList;
          
          for ( ; fs; fs = fs->next)
            {
              const char *fsName = g_quark_to_string(GPOINTER_TO_INT(fs->data));
              g_string_append_printf(columnStanza, "%s%s", fsName, SEPARATOR);
            }

          /* Chop off the tailing semi-colon of featuresets list and start new line */
          g_string_truncate(columnStanza, columnStanza->len - separatorLen);
          g_string_append(columnStanza, "\n");
        }
    }

  /* Chop off tailing separator from column names list */
  if (columnNames->len)
    g_string_truncate(columnNames, columnNames->len - separatorLen);

  /* Add [column] stanza heading, if columnStanza has any content */
  if (columnStanza->len)
    g_string_prepend(columnStanza, "\n[columns]\n");

  *columnNames_out = columnNames->str;
  *columnStanza_out = columnStanza->str;
  
  /* Clean up */
  g_string_free(columnNames, FALSE);
  g_string_free(columnStanza, FALSE);
}


/* Group featuresets into columns, where relevant. This results in 
 * a list of column names, which should be included in the [ZMap] stanza,
 * as well as the [columns] stanza, if applicable (empty string if not).
 * Sets the error and does not set the strings if there was any problem.
 * The input list must be sorted by priority. */
static void constructColumnsList(GSList *featuresets, 
                                 char **columnNames_out,
                                 char **columnStanza_out,
                                 GError **error)
{
  /* If featuresets have a priority within a certain threshold of another
   * then they are shown in the same column. */
  const double threshold = displayGetOverlapThreshold();

  GSList *fs = featuresets;
  GSList *columns = NULL; /* list of column names (as GQuarks) */
  GHashTable *columnFsLists = g_hash_table_new(g_direct_hash, g_direct_equal); /* hash of column name (as GQuark) to GSList of featureset names (as GQuarks) */
  double prevPriority = -1;
  GQuark currentColumn = 0;
  
  for ( ; fs; fs = fs->next)
    {
      MethodData *methodData = (MethodData*)(fs->data);

      /* Check if this featureset starts a new column */
      if (prevPriority == -1 || fabs(prevPriority - methodData->priority) > threshold)
        {
          columns = g_slist_append(columns, GINT_TO_POINTER(methodData->nameQuark));
          currentColumn = methodData->nameQuark;
        }

      /* Append current featureset name to current column's featureset list */
      GSList *columnFsList = g_hash_table_lookup(columnFsLists, GINT_TO_POINTER(currentColumn));
      columnFsList = g_slist_append(columnFsList, GINT_TO_POINTER(methodData->nameQuark));
      g_hash_table_insert(columnFsLists, GINT_TO_POINTER(currentColumn), columnFsList);

      prevPriority = methodData->priority;
    }

  constructColumnsString(columns, columnFsLists, columnNames_out, columnStanza_out, error);
}


/* Get the list of all featuresets (i.e. methods) for the current database.
 * Returns a list of MethodData pointers. The list and MethodData pointers
 * should be freed by the caller */ 
static GSList* getFeaturesets(KEY key, KEY from, FeatureMap *look_out, GError **error)
{
  char *command = g_strdup_printf("seqget %s", name(from));
  ACEIN command_in = aceInCreateFromText (command, "", 0);

  /* no subshells ($), no commands (@)
   * recognise ';' as card separator - WARNING: this will disable some 
   *   tace-commands where the ; is needed to submit multiple entries
   *   on one command line using the '=' assignment. */
  aceInSpecial (command_in, "\n\t\\/%;");

  Stack resultStack = stackCreate(100);
  ACEOUT command_out = aceOutCreateToStack (resultStack, 0);

  int option = 0;
  options[0].key = OPTIONS_SIZE ;			    /* Simpler to do this every time. */
  aceInOptPrompt (command_in,  &option, options);

  /* This call finds the methods and calls the given callback to populate
   * the featuresets list. */
  GSList *featuresets = NULL;

  FeatureMap look = gifGetMethods(command_in, command_out, key, from, constructFeaturesetsList, &featuresets, error);
  
  if (look_out)
    *look_out = look;
  else
    featureMapDestroy(look);

  aceInDestroy(command_in);
  g_free(command);

  return featuresets;
}


/* Called on each entry in the featuresets list. The value is a pointer to a MethodData
 * struct. The method name is appended to the GString given in the user_data, separated
 * by a semi-colon if there is already content in the GString. */
static void constructFeaturesetsString(gpointer data, gpointer user_data)
{
  GString *resultStr = (GString*)user_data;
  MethodData *methodData = (MethodData*)data;
  
  if (!methodData || !resultStr)
    return;

  DEBUG_OUT("%s, %.2f\n", g_quark_to_string(methodData->nameQuark), methodData->priority);

  if (resultStr->len > 0)
    g_string_append_printf(resultStr, " ; %s", g_quark_to_string(methodData->nameQuark));
  else
    g_string_append(resultStr, g_quark_to_string(methodData->nameQuark));
}


/* Get the list of all columns and featuresets for the current database. 
 * The results are compiled into semi-colon separated strings. The [columns] 
 * stanza is also constructed. The returned strings should be freed by the caller.
 * If there are any problems, error is set and the return values are not set. */
static void getColumns(KEY key,
                       KEY from,
                       char **featuresetNames_out,
                       char **columnNames_out, 
                       char **columnStanza_out,
                       FeatureMap *look_out,
                       GError **error)
{
  /* Get all the featuresets (i.e. methods) */
  GError *tmpError = NULL;
  GSList *featuresets = getFeaturesets(key, from, look_out, &tmpError);
  
  if (!tmpError)
    {
      /* Sort list by priority */
      featuresets = g_slist_sort(featuresets, compareMethodsByPriority);

      /* Construct the featuresets string from the featuresets list */
      GString *resultStr = g_string_new(NULL);
      g_slist_foreach(featuresets, constructFeaturesetsString, resultStr);
      *featuresetNames_out = resultStr->str;
      g_string_free(resultStr, FALSE);

      /* Group the featuresets into columns, as required */
      constructColumnsList(featuresets, columnNames_out, columnStanza_out, &tmpError);
    }

  /* Error reporting */
  propagatePrefixedError(error, tmpError, "Could not construct columns list: ");
  
  /* Clean up */
  GSList *item = featuresets;

  for ( ; item; item = item->next)
    g_free(item->data);
  
  g_slist_free(featuresets);
}


/* Get the config file line to include for the styles file.
 * Returns an empty string if the file is not specified. */
static char *getStylesFile()
{
  char *result = NULL;

  const gchar *env = g_getenv(ZMAP_STYLES_FILE_ENV_VAR);
  
  if (env && *env)
    result = g_strdup_printf("stylesfile = %s\n", env);
  //  else
  //    result = g_strdup_printf("%s%s", g_get_home_dir(), ZMAP_DEFAULT_STYLES_FILE);
  else
    result = g_strdup("");
  
  return result;
}


/* Create and populate the zmap config file with the given name */
static void createZMapConfigFile(const char *filename,
                                 const char *seq_name,
                                 const guint16 port, 
                                 const char *featuresets,
                                 const char *columnNames,
                                 const char *columnStanza,
                                 GError **error)
{
  GError *tmpError = NULL;
  char *stylesfile = NULL;

  if (!tmpError)
    stylesfile = getStylesFile();

  if (!tmpError)
    {
      /* write the config contents to the file (currently based on 
       * a hard-coded template) */
      FILE *file = fopen(filename, "w");

      if (file)
        {
          fprintf(file, ZMAP_CONFIG_TEMPLATE, 
                  seq_name,
                  seq_name,
                  columnNames,
                  ZMAP_PREDEFINED_COLUMNS,
                  columnStanza,
                  featuresets,
                  seq_name, 
                  featuresets,
                  ZMAP_PREDEFINED_COLUMNS,
                  stylesfile,
                  HOST_IP_ADDRESS, 
                  port, 
                  USE_METHODS_FLAG);
          
          fclose(file);
        }
      else
        {
          g_set_error(&tmpError, XACEZMAP_ERROR, XACEZMAP_ERROR_CONFIG, "Error opening file '%s'\n", filename); 
        }
    }

  /* Error reporting */
  propagatePrefixedError(error, tmpError, "Error creating config file: ");
  
  /* Clean up */
  g_free(stylesfile);
}


/* Get the arguments that we should pass to zmap on startup. Returns them 
 * as an argv array which must be freed by the caller. */
static char** getZmapArgs(FeatureMap look, 
                          const guint16 port, 
                          const char *featuresets,
                          const char *columnNames,
                          const char *columnStanza,
                          GQuark *configDirQuark_out,
                          GError **error)
{
  char **argv = NULL;

  /* create a temp dir for zmap's config stuff */
  GError *tmpError = NULL;
  char *config_dir = createTempDir("ZMap", &tmpError);
  
  /* create the config file */
  char *config_file = NULL;
  
  if (!tmpError)
    {
      config_file = g_strdup_printf("%s/%s", config_dir, ZMAP_CONFIG_FILE_NAME);
      createZMapConfigFile(config_file, name(look->seqKey), port, featuresets, columnNames, columnStanza, &tmpError);
    }
  
  /* Create the argv array */
  if (!tmpError)
    {
      if (configDirQuark_out)
        *configDirQuark_out = g_quark_from_string(config_dir);

      const int start = look->zoneMin + look->start + 1;
      const int end = look->zoneMax + look->start + 1;

      /* add the args to a list */
      GSList *args_list = NULL;
      args_list = g_slist_append(args_list, g_strdup("zmap"));
      args_list = g_slist_append(args_list, g_strdup_printf("--start=%d", start));
      args_list = g_slist_append(args_list, g_strdup_printf("--end=%d", end));

      if (config_dir)
        args_list = g_slist_append(args_list, g_strdup_printf("--conf_dir=%s", config_dir));

      if (config_file)
        {
          args_list = g_slist_append(args_list, g_strdup_printf("--conf_file=%s", g_path_get_basename(config_file)));
        }
  
      if (g_getenv(ZMAP_SLEEP_ENV_VAR))
        args_list = g_slist_append(args_list, g_strdup_printf("--sleep=%s", g_getenv(ZMAP_SLEEP_ENV_VAR)));

      //args_list = g_slist_append(args_list, g_strdup(seq_name));
      args_list = g_slist_append(args_list, NULL);

      /* convert the list to an array (this destroys the list) */
      argv = convertListToArgv(args_list);
    }

  /* Error reporting */
  propagateError(error, tmpError);

  /* Clean up */
  g_free(config_dir);
  g_free(config_file);

  return argv;
}


/* Destroy a null-terminated array of strings */
static void destroyStrArray(char **arrStr)
{
  if (arrStr)
    {
      /* free argv array */
      int i = 0;
      for ( ; arrStr[i] != NULL; ++i)
        g_free(arrStr[i]);
      
      g_free(arrStr);
    }
}

///* Display a dialog showing the default zmap range, and give the user
// * the opportunity to edit it */
//static gint showZmapRangeDialog(FeatureMap look)
//{
//  GtkWidget *dialog = gtk_dialog_new_with_buttons("ZMap range", 
//                                                  NULL, 
//                                                  GTK_DIALOG_DESTROY_WITH_PARENT,
//                                                  GTK_STOCK_CANCEL,
//                                                  GTK_RESPONSE_REJECT,
//                                                  GTK_STOCK_OK,
//                                                  GTK_RESPONSE_ACCEPT,
//                                                  NULL);
//
//  gtk_dialog_set_default_response(GTK_DIALOG(dialog), GTK_RESPONSE_ACCEPT);
//  //  g_signal_connect(dialog, "response", G_CALLBACK(onResponseExtentDialog), GINT_TO_POINTER(TRUE));
//  
//  GtkBox *vbox = GTK_BOX(GTK_DIALOG(dialog)->vbox);
//
//  GtkBox *hbox = GTK_BOX(gtk_hbox_new(FALSE, 0));
//  gtk_box_pack_start(vbox, GTK_WIDGET(hbox), FALSE, FALSE, 0);
//
//  GtkWidget *labelStart = gtk_label_new("Start");
//  gtk_box_pack_start(hbox, labelStart, FALSE, FALSE, 0);
//
//  GtkWidget *entryStart = gtk_entry_new();
//  char *strStart = g_strdup_printf("%d", look->zoneMin + 1);
//  gtk_entry_set_text(GTK_ENTRY(entryStart), strStart);
//  gtk_entry_set_activates_default(GTK_ENTRY(entryStart), TRUE);
//  g_free(strStart);
//  gtk_box_pack_start(hbox, entryStart, FALSE, FALSE, 0);
//
//  GtkWidget *labelEnd = gtk_label_new("End");
//  gtk_box_pack_start(hbox, labelEnd, FALSE, FALSE, 0);
//
//  GtkWidget *entryEnd = gtk_entry_new();
//  char *strEnd = g_strdup_printf("%d", look->zoneMax + 1);
//  gtk_entry_set_text(GTK_ENTRY(entryEnd), strEnd);
//  gtk_entry_set_activates_default(GTK_ENTRY(entryEnd), TRUE);
//  g_free(strEnd);
//  gtk_box_pack_start(hbox, entryEnd, FALSE, FALSE, 0);
//
//  /* Show dialog and wait for a response */
//  gtk_widget_show_all(dialog);
//  gint response = gtk_dialog_run(GTK_DIALOG(dialog));
//
//  if (response == GTK_RESPONSE_ACCEPT)
//    {
//      look->zoneMin = atoi(gtk_entry_get_text(GTK_ENTRY(entryStart))) - 1;
//      look->zoneMax = atoi(gtk_entry_get_text(GTK_ENTRY(entryEnd))) - 1;
//    }
//
//  gtk_widget_destroy(dialog);
//
//  return response;
//}


/* Custom callback to be called when a zmap process is closed.
 * It takes the config file name (as a GQuark) as the user data,
 * and it deletes this file. */
static void onZmapChildProcessClosed(GPid child_pid, gint status, gpointer data)
{
  GQuark configDirQuark = GPOINTER_TO_INT(data);
  const gchar *config_dir = g_quark_to_string(configDirQuark);
  
  if (config_dir)
    {
      DEBUG_OUT("xace: Deleting config directory for process %d: %s \n", child_pid, config_dir);

      char *config_file = g_strdup_printf("%s/%s", config_dir, ZMAP_CONFIG_FILE_NAME);
      char *log_file = g_strdup_printf("%s/%s", config_dir, ZMAP_LOG_FILE_NAME);
      
      if (unlink(config_file))
        printf("xace: Error cleaning up ZMap config file: %s\n", config_file);
      
      if (unlink(log_file))
        printf("xace: Error cleaning up ZMap log file: %s\n", log_file);
      
      if (rmdir(config_dir))
        printf("xace: Error cleaning up ZMap config directory: %s\n", config_dir);
      
      g_free(config_file);
      g_free(log_file);
    }
  
  /* We must always call the standard callback when a child process
   * is closed. */
  onChildProcessClosed(child_pid, status, NULL);
}


/* Start up zmap as a child process */
static BOOL startZmap(KEY key, KEY from, const guint16 port, GError **error)
{
  BOOL status = FALSE;

  GError *tmpError = NULL;

  /* Get the column and featureset names. Also returns the coords in the 'look' */
  char *featuresets = NULL;
  char *columnNames = NULL;
  char *columnStanza = NULL;
  FeatureMap look  = NULL;
  
  getColumns(key, from, &featuresets, &columnNames, &columnStanza, &look, &tmpError);
  
  if (look && !tmpError)
    {
      //      if (showZmapRangeDialog(look) == GTK_RESPONSE_ACCEPT)
        {
          GQuark configDirQuark = 0;
          char **argv = getZmapArgs(look, port, featuresets, columnNames, columnStanza, &configDirQuark, &tmpError);

          if (!tmpError)
            status = spawnChildProcess(argv, onZmapChildProcessClosed, GINT_TO_POINTER(configDirQuark), &tmpError);

          destroyStrArray(argv);
        }
//      else
//        {
//          status = TRUE; /* user cancelled so return status is ok */
//        }
    }

  featureMapDestroy(look);
  g_free(columnStanza);
  g_free(columnNames);
  g_free(featuresets);
  
  propagateError(error, tmpError);
  
  return status;
}


/************************************************************/
/* Public functions */

/* Main entry routine to start up a zmap child process. The first time
 * this is called, it sets up a socket to listen for incoming client
 * connections before it spawns the first zmap. */
BOOL zMapDisplay(KEY key, KEY from, BOOL isOldGraph, void *app_data)
{
  BOOL status = FALSE;
  GError *error = NULL;
  guint16 port = HOST_PORT; /* gets updated if original port number is already in use */

  /* First, set up sockets to listen for zmap clients connecting */
  listenForClientConnections(&port, &error);

  if (!error)
    status = startZmap(key, from, port, &error);

  if (error)
    printf("xace: error starting ZMap: %s\n", error->message);

  return status;
}
