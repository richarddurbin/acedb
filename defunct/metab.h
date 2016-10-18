/*  File: metab.h
 *  Author: Stan Letovsky (letovsky@gdb.org)
 *-------------------------------------------------------------------
 * This file is part of the ACEDB genome database package, written by
 * 	Richard Durbin (MRC LMB, UK) rd@mrc-lmb.cam.ac.uk, and
 *	Jean Thierry-Mieg (CRBM du CNRS, France) mieg@kaa.cnrs-mop.fr
 *
 * Description:
 * Exported functions:
 * HISTORY:
 * Last edited: Mar 18 00:27 1995 (mieg)
 * Created: Sun Nov  6 18:55:54 1994 (rd)
 *-------------------------------------------------------------------
 */

/* $Id: metab.h,v 1.5 1995-03-17 22:41:53 mieg Exp $ */

#ifndef metab_h
#define metab_h 1

#include <ctype.h>
#include <assert.h>
#include <math.h>

#include "acedb.h"
#include "graph.h"
#include "array.h"
#include "key.h"
#include "lex.h"
#include "bs.h"
#include "systags.h"
#include "classes.h"
#include "sysclass.h"
#include "tags.h"
#include "disptype.h"
#include "display.h"
#include "query.h"

#define NAME 60
#define ISIZE sizeof(int)

#define NODE_RADIUS 1
#define NODE_LABEL_DX 2.0
#define NODE_LABEL_DY -.5
#define DEBUG 0

/* these dims in character widths */
#define ARROW_HEAD_LENGTH 1.5
#define ARROW_HEAD_HALF_WIDTH 0.5

/* RD commented these out since now all static in one file

extern float X, Y, Pi, DegreesPerRadian,  RadiansPerDegree;
extern struct element *Elements;
extern int Made, Expand;
extern KEY NodeColor, ArcColor, CofColor;

*/

enum element_type { DIAG_TEXT, DIAG_NODE, DIAG_ARC, DIAG_TANGENT };
enum node_type { NORMAL_NODE, POINT_NODE, TANARC_NODE };

struct diagram
  { char *name; 
    KEY key;
    struct element *elements, *selection;
    float xmax, ymax ; /* of selected object within its box */
    Graph graph;
    int modified;
    STORE_HANDLE handle ;
  };

struct element
  { int box, mark, highlight;
    float x, y, x0, y0, width, height; /* redundant representation of boxes
				 used by layout for fast virtual relocation */
    KEY key, color;
    enum element_type type; 
    void *item;
    struct element *next;
  };
  
struct node 
  {
    float x, y;
    struct element *label;
    enum node_type type;
    int id, depth, i, indeg, outdeg, mark, cycle;
  };

struct arc
  {
  float radius, xc, yc, x1, y1, start_angle, arc_angle;
  int id, arrow, /* 0 - no arrowhead; 1 - arrowhead on to-node */
     curvature; /* 0 = straight, 1 = curved, -1 = oppositely curved */
  struct element *label, *to, *from; 
  struct element *tangent, *tangent_to;
  };

struct tangent
{  float radius, xc, yc, xt, yt, start_angle, arc_angle /* angles in degrees */;
   int curvature;
   struct element *tangent_to;
   struct text *from, *to;
 };

struct text
  {
  float x, y;
  int id, outline;
  char *name;
  };

/* function prototypes */
static void diagQuit();
static void diagPick(double x, double y);
static void diagSelect(double x, double y);

static void diagMoveElement(double x, double y);
static void RecomputeArcParams(struct arc *a);
static struct element *ID2Node(int);
static struct element *ID2Arc(int);
static struct element *MakeText(int id, char *name, float x, float y, int outline, KEY color);
static struct element *MakeNode(int id, struct element *label, float x, float y);
static struct element *MakeArc(int id, struct element *from_node, struct element *to_node,
			struct element *label, int curvature);
static struct diagram *MakeDiagram(KEY);
static struct diagram *GetDiagram(KEY);

static void metabPathway();
static void metabRecompute();
static struct element *Pos2Element(struct diagram *d, float x, float y);
static void DrawDiagram(struct diagram *d);
static void metabDrawText( struct element *e );
static void DrawNode( struct element *e);
static void DrawArc( struct element *r);
static void DrawCurvedArc( struct element *r);
static void DrawStraightArc( struct element *r );
static void diagArrowHead( float x, float y, float dir);
static void diagMoveNode( struct element *e, float x, float y);
static void diagMoveText( struct element *e, float x, float y);
static void diagRedrawArc( struct element *e);
static void diagMoveArc( struct element *e, float x, float y);

static void diagRedraw(void);
static void RecomputeCurvedArcParams(struct arc *a);
static float diagTextAspect();
static void diagStraightArc( float xfrom, float yfrom, float xto, float yto );
static void LayoutDiagram(struct diagram *d);
static void SaveDiagram(struct diagram *d);
static void msg(char *msg);

static void *notNULL (void *x);
static BOOL diagBoxOverlap(struct element *a, struct element *b);
static BOOL diagFixOverlap(struct element *a, struct element *b);
static BOOL IntervalsOverlap(float min1, float max1, float min2, float max2);
static BOOL InInterval(float val, float min, float max);
static BOOL LineSegsIntersect(float x11, float y11, float x12, float y12, 
			     float x21, float y21, float x22, float y22, 
			     float *xi, float *yi);

static void metabExpand();
static struct diagram *ActiveDiagram();
static void ExpandDiagram(KEY key);
static void metabArcLabels();

static void metabMakeDiagram0(KEY key, struct diagram *d);
static BOOL Movable(struct element *e);
#endif

