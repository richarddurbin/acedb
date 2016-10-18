/*  File: gmap.h
 *  Author: Richard Durbin (rd@mrc-lmb.cam.ac.uk)
 *  Copyright (C) J Thierry-Mieg and R Durbin, 1992
 *-------------------------------------------------------------------
 * This file is part of the ACEDB genome database package, written by
 * 	Richard Durbin (MRC LMB, UK) rd@mrc-lmb.cam.ac.uk, and
 *	Jean Thierry-Mieg (CRBM du CNRS, France) mieg@kaa.cnrs-mop.fr
 *
 * Description: header file for gmap operations
 * Exported functions:
 * HISTORY:
 * Last edited: Aug 30 20:36 1994 (mieg)
 * Created: Thu Nov 26 05:15:10 1992 (rd)
 *-------------------------------------------------------------------
 */

/* $Id: newgmap.h,v 1.3 1994-08-30 19:37:48 mieg Exp $ */

#include "acedb.h"
#include "graph.h"
#include "key.h"
#include "lex.h"
#include "bs.h"
#include "a.h"
#include "systags.h"
#include "classes.h"
#include "sysclass.h"
#include "tags.h"
#include "session.h"
#include "disptype.h"
#include "display.h"
#include "query.h"
#include "bump.h"
#include "colcontrol.h"
#if defined(applec)
#include "Math.h"
#endif

typedef struct dataStruct {
  Associator loc2two ;	/* all loc2 Ass's map key -> KEYSET */
  Associator loc2multi ;
  Associator loc2inside ;
  Associator loc2outside ;
  KEYSET orderedLoci ;
  Associator key2seg ;
} *MapData ;

typedef struct GMapLookStruct
  { int   magic;        /* == GMAPMAGIC */
    MAPCONTROL map;
    KEY   key ;
    int	  messageBox ;
    char  messageText[128] ;
    int   resolution ;
    float errorScale ;  /* to scale the error on gene locations */
    unsigned int flag ;
    Array segs;      	/* array of SEG's from the obj */
    int lastTrueSeg ;	/* arrayMax(look->segs) after Convert */
    MapData data ;      /* private data for gmapdata.c */
  } *GMAPLOOK;

#define gMapLook(map) ((GMAPLOOK)((map)->look))

#define GMAPMAGIC  837648

#define FLAG_ALL_DATA		0x00000001
#define FLAG_HIDE_HEADER	0x00000002


typedef struct
  { KEY key ;
    float x, dx ;
    unsigned int flag ;
  } GMAPSEG ;
#define segFormat "kffi"


  /* seg->flag flags  i use the leftmost octet to store colour*/
#define FLAG_CLONED		0x00000001
#define FLAG_MARKER		0x00000002
#define FLAG_RELATED		0x00000004
#define FLAG_STRESSED		0x00000008
#define FLAG_ANTI_RELATED	0x00000010
#define FLAG_PHYS_GENE		0x00000020
#define FLAG_ANTI_STRESSED      0x00000040
#define FLAG_HAVE_DATA		0x00000080
#define FLAG_WELL_ORDERED	0x00000100
#define FLAG_HIDE		0x00000200
#define FLAG_HIGHLIGHT		0x00000400
#define FLAG_CENTROMERE		0x00000800
#define FLAG_DARK_BAND		0x00001000
#define FLAG_NOR		0x00002000
#define FLAG_MOVED		0x00004000
#define FLAG_DEFICIENCY		0x00008000
#define FLAG_DUPLICATION	0x00010000
#define FLAG_BALANCER   	0x00020000
#define FLAG_CHROM_BAND   	0x00040000
#define FLAG_COLOUR      	0x00080000 /* colour is coded as << 28 */




void pMapToGMap (KEY contig, KEY from, int x) ;

void gjmDrawDbn (GMAPLOOK look, float *offset) ;
void gMapDataDestroy (GMAPLOOK look) ;

BOOL getMulti (KEY) ;
GMAPSEG* insertMulti (GMAPLOOK look, KEY key) ;
BOOL logLikeMulti (float *ll) ;
void multiBoundCheck (KEY locus, float *min, float *max, 
		      KEY *minLoc, KEY *maxLoc) ;
BOOL get2p (KEY key, KEY *loc1, KEY *loc2, KEY *type) ;
GMAPSEG* insert2p (GMAPLOOK look, KEY key) ;
float logLike2pt (float dist, KEY type) ;
void calcAll2pt (void) ;
BOOL getPos (KEY key, float *y) ;
void setPos (GMAPLOOK look) ;
void getAllData (GMAPLOOK look) ;
void gjmMakeDbn (int box) ;
void gMapShowDataFromMenu (int box) ;
void gMapShowData(void);
void overlayColours(int box, GMAPSEG *seg);

extern KEY dataKey ;
extern KEYSET newBadData ;
extern Array loci ;     /* workspace for recombination data */
extern Array counts ;   /* workspace for recombination data */

#define LOG_10 	  2.302585	/* log(10) */

extern MENUOPT gjmMenu[] ;
extern MENUOPT gMapIntervalMenu[] ;

extern struct ProtoStruct gMapOrderedGenesColumn;
extern struct ProtoStruct gMapGenesColumn;
extern struct ProtoStruct gMapRearrColumn;
extern struct ProtoStruct gMapMainMarkersColumn;
extern struct ProtoStruct gMapChromBandsColumn;
extern struct ProtoStruct gMapMiniChromBandsColumn;
extern struct ProtoStruct gMapPhysGenesColumn;
extern struct ProtoStruct gMapContigColumn;
extern struct ProtoStruct gMapRevPhysColumn;
extern struct ProtoStruct gMapLikelihoodColumn;
extern struct ProtoStruct gMapMultiPtColumn;
extern struct ProtoStruct gMapTwoPtColumn;
/********** end of file **********/


