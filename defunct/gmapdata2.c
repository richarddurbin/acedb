 /*  File: gmapdata2.c
 *  Author: Richard Durbin (rd@mrc-lmb.cam.ac.uk)
 *  Copyright (C) J Thierry-Mieg and R Durbin, 1993
 *-------------------------------------------------------------------
 * This file is part of the ACEDB genome database package, written by
 * 	Richard Durbin (MRC LMB, UK) rd@mrc-lmb.cam.ac.uk, and
 *	Jean Thierry-Mieg (CRBM du CNRS, France) mieg@kaa.cnrs-mop.fr
 *
 * Description: basic routines for Muli_pt and 2pt data
 * Exported functions:
 * HISTORY:
 * Last edited: Jan 11 15:45 1995 (srk)
 * Created: Tue Nov 30 18:42:30 1993 (rd)
 *-------------------------------------------------------------------
 */

/* $Id: gmapdata2.c,v 1.11 1995-01-12 15:50:12 srk Exp $ */

#include "gmap.h"

/********************************************************/
/************** Multi_pt_data ***************************/
typedef struct {
  KEY key;
  float x, dx;
  MULTIPTDATA data;
} MULTIPTSEG;

typedef struct MultiPriv {
  Array segs;
} *MULTIPRIV;

BOOL multiPtAddKey(COLINSTANCE instance, KEY key) 
{ MAPCONTROL map = instance->map;
  GMAPLOOK look = gMapLook(map);
  MULTIPRIV private = (MULTIPRIV) instance->private;
  MULTIPTSEG *seg;
  MULTIPTDATA data; 
  int i;

  if (!key) /* clear */
    { arrayMax(private->segs) = 0;
      return TRUE;
    }

  /* If we've already got it, don't do it again */
  for (i=0; i<arrayMax(private->segs); i++)
    { seg = arrp(private->segs, i, MULTIPTSEG);
      if (seg->key == key)
	{
	  if (!instance->displayed)
	    instance->displayed = TRUE;
	  return TRUE;
	}
    }  

  if (!gMapGetMultiPtData(map, key, instance->handle, &data))
    return FALSE;
    
  if (!instance->displayed)
    instance->displayed = TRUE;


  seg = arrayp(private->segs, arrayMax(private->segs), MULTIPTSEG);
  seg->key = key;
  seg->data = data;
  seg->x = data->min; 
  seg->dx = data->max - data->min;
  return TRUE;
}

/*******************/

BOOL logLikeMulti (MAPCONTROL map, MULTIPTDATA data, KEY dataKey, float *ll)
{
	/* assumes single recombinant in locA, locB interval */
  int i = 0, n = arrayMax(data->loci) ;
  float y, z, denom, sum, least, most ;

  if (!getPos (map, arr(data->loci, 0, KEY), &y) || 
      !getPos (map, arr(data->loci, n-1, KEY), &z) ||
      y == z)
    return FALSE ;
  
  *ll = 0 ;
  sum = 0 ;			/* number of recombinants */

  if (z > y)
    { denom = z - y ;
      while (i < n)
	{ least = y ; most = y ;
	  while (++i < n && arr(data->counts,i-1,int) == 0)
	    if (getPos(map, arr(data->loci,i,KEY), &y))
	      if (y < least)
		least = y ;
	      else if (y > most)
		most = y ;
	  --i ;

	  if (sum)
	    if (least <= z)		/* order violates recombinants */
	      { /* keySetInsert (newBadData, dataKey) ; TODO */
		return FALSE ;
	      }
	    else
	      { *ll += sum * log((least - z) / denom) ;
		sum = 0 ;
	      }
	  z = most ;

	  while (++i < n)
	    { sum += arr(data->counts,i-1,int) ;
	      if (getPos(map, arr(data->loci,i,KEY), &y))
		break ;
	    }
	}
    }
  else
    { denom = y - z ;
      while (i < n)
	{ least = y ; most = y ;
	  while (++i < n && arr(data->counts,i-1,int) == 0)
	    if (getPos(map, arr(data->loci,i,KEY), &y))
	      if (y < least)
		least = y ;
	      else if (y > most)
		most = y ;
	  --i ;

	  if (sum)
	    if (most >= z)		/* order violates recombinants */
	      { /* keySetInsert (newBadData, dataKey) ; TDOD */
		return FALSE ;
	      }
	    else
	      { *ll += sum * log((z - most) / denom) ;
		sum = 0 ;
	      }
	  z = least ;

	  while (++i < n)
	    { sum += arr(data->counts,i-1,int) ;
	      if (getPos(map, arr(data->loci,i,KEY), &y))
		break ;
	    }
	}
    }

  return TRUE ;
}

/*******************/

void multiBoundCheck (MAPCONTROL map, KEY locus, KEY multikey, 
		      float *min, float *max, KEY *minLoc, KEY *maxLoc)
{
	/* assumes single recombinant in locA, locB interval */

  int i, n;
  float x, y ;
  STORE_HANDLE dataHandle = handleCreate();
  MULTIPTDATA multi;

  if (!getPos (map, locus, &x) ||
      !gMapGetMultiPtData(map, multikey, dataHandle, &multi))
    return;
  
  n = arrayMax(multi->loci);

  for (i = 0 ; i < n ; ++i)
    if (arr(multi->loci, i, KEY) == locus)
      break ;
  while (i < n)
    if (arr(multi->counts, i++, int))
      break ;
  while (i < n)
    if (getPos (map, arr(multi->loci, i++, KEY), &y))
      if (y < x && y > *min)
	{ *min = y ; *minLoc = arr(multi->loci, i-1, KEY) ; }
      else if (y > x && y < *max)
	{ *max = y ; *maxLoc = arr(multi->loci, i-1, KEY) ; }

  for (i = n-1 ; i >= 0 ; --i)
    if (arr(multi->loci, i, KEY) == locus)
      break ;
  while (--i >= 0)
    if (arr(multi->counts, i, int))
      break ;
  while (i >= 0)
    if (getPos (map, arr(multi->loci, i--, KEY), &y))
      if (y < x && y > *min)
	{ *min = y ; *minLoc = arr(multi->loci, i+1, KEY) ; }
      else if (y > x && y < *max)
	{ *max = y ; *maxLoc = arr(multi->loci, i+1, KEY) ; }

  handleDestroy(dataHandle);
}

/*******************/



static BOOL multiPtSetSelect (COLINSTANCE instance,
			      int box,
			      double x,
			      double y)
{
  COLCONTROL control = instance->map->control;
  MULTIPTSEG *seg = (MULTIPTSEG *)arr(control->boxIndex2, box, void *);
  GMAPLOOK look = gMapLook(instance->map);

  look->selectKey = seg->key;
  look->multiPtCurrentColumn = instance;  

  /* We can only have neighbours */
  gMapUnselect(instance->map);
  (void)gMapNeighbours(instance->map, &look->neighbours, seg->key);
  look->neighboursInfoValid = TRUE;
  
  return FALSE;

}

static BOOL multiPtUnselect(COLINSTANCE instance, int box)
{
  COLCONTROL control = instance->map->control;
  GMAPLOOK look = gMapLook(instance->map);

  *look->messageText = 0;
  if (look->messageBox)
    graphBoxDraw(look->messageBox, BLACK, CYAN);

  return FALSE;
}

static void multiPtDoColours(COLINSTANCE instance, int box)
{
  MAPCONTROL map = instance->map;
  COLCONTROL control = map->control;
  MULTIPTSEG *seg = (MULTIPTSEG *) arr(control->boxIndex2, box, void *); 
  GMAPLOOK look = gMapLook(instance->map);
  int i;
  float dummy;
  
  if (seg->key == look->selectKey && 
      instance == control->activeInstance) 
    { 
      graphBoxDraw(box, BLACK, CYAN);
      control->activeBox = box;  
      *look->messageText = 0 ;
      strcpy (look->messageText, "Multi_pt ") ;
      strcat (look->messageText, name(seg->key)) ;
      strcat (look->messageText, ": ") ;
      strcat (look->messageText, name(arr(seg->data->loci,0,KEY))) ;
      for (i = 1 ; i < arrayMax(seg->data->loci) ; ++i)
	strcat (look->messageText, 
		messprintf (" %d %s", arr(seg->data->counts,i-1,int), 
			    name(arr(seg->data->loci,i,KEY)))) ;
      if (look->messageBox)
	graphBoxDraw(look->messageBox, BLACK, CYAN);
  
    }
  else
    {
      if (control->activeInstance &&
	   map == control->activeInstance->map &&
	   look->dataInfoValid &&
	   keySetFind(look->multi, seg->key, 0))
	/* instance->proto->specialisation TRUE for RD variant */
	{ if (!instance->proto->specialisation || 
	      logLikeMulti(map, seg->data, seg->key, &dummy))
	    graphBoxDraw(box, BLACK, GREEN);
	  else
	    graphBoxDraw(box, WHITE, DARKGREEN);
	}
      else if (keySetFind(look->highlight, seg->key, 0))
	graphBoxDraw(box, BLACK, MAGENTA);
      else
	graphBoxDraw(box, BLACK, WHITE);
    }
}

static void multiPtRDDraw(COLINSTANCE instance, float *offset)
{
  MAPCONTROL map = instance->map;
  COLCONTROL control = map->control;
  GMAPLOOK look = gMapLook(map);
  MULTIPRIV private = instance->private;
  int   i, j, n;
  float y1, y2, x;
  int ibox;
  MULTIPTSEG *seg ;
  MULTIPTDATA data;


  for (i = 0 ; i < arrayMax(private->segs) ; ++i)
    { seg = arrp(private->segs,i,MULTIPTSEG) ;
      if (!keySetFind(look->hidden, seg->key, 0))
	{ ibox = graphBoxStart();
	  array(control->boxIndex, ibox, COLINSTANCE) = instance;
	  array(control->boxIndex2, ibox, void *) = (void *) seg;
	  data = seg->data;
	  n = arrayMax(data->loci);
	  y1=0, y2=0 ;
	  x = (*offset)++;
 
	    graphPointsize (0.8) ;
	  if (getPos (map, arr(data->loci, 0, KEY), &y1))
	    graphPoint (x, MAP2GRAPH(map, y1)) ;
	  if (getPos (map, arr(data->loci, n-1, KEY), &y2))
	    graphPoint (x, MAP2GRAPH(map, y2)) ;
	  graphLine (x, MAP2GRAPH(map, y1), 
		     x, MAP2GRAPH(map, y2)) ;

	  graphPointsize (0.6) ;
	  for (j = 1 ; j < n ; ++j)	/* redo n-1 to get counts line */
	    if (getPos (map, arr(data->loci,j,KEY), &y2))
	      { graphPoint (x, MAP2GRAPH(map, y2)) ;
		if (arr (data->counts, j-1, int))
		  graphLine (x-0.25, MAP2GRAPH(map, (y1+y2)/2),
			     x+0.25, MAP2GRAPH(map, (y1+y2)/2)) ;
		y1 = y2 ;
	      }
	  graphBoxEnd() ;

	  if (seg->key == control->from)
	    control->fromBox = ibox;
	}
    }
  (*offset)++;
}

static void multiPtJTMDraw(COLINSTANCE instance, float *offset)
{
  MAPCONTROL map = instance->map;
  COLCONTROL control = map->control;
  GMAPLOOK look = gMapLook(map);
  MULTIPRIV private = instance->private;
  int   i, j, n, sens;
  float y1, y2, ymin, ymax, x;
  MULTIPTSEG *seg ;
  MULTIPTDATA data;
  int ibox;

  for (i = 0 ; i < arrayMax(private->segs) ; ++i)
    { seg = arrp(private->segs,i,MULTIPTSEG) ;
      if (!keySetFind(look->hidden, seg->key, 0))
	{ ibox = graphBoxStart();
	  array(control->boxIndex, ibox, COLINSTANCE) = instance;
	  array(control->boxIndex2, ibox, void *) = (void *) seg;
	  data = seg->data;
	  n = arrayMax(data->loci);
	  y1=0, y2=0 ;
	  sens = 0;
	  x = (*offset)++;
	  
	  graphPointsize (0.8) ;
	  if (getPos (map, arr(data->loci, 0, KEY), &y1))
	    graphPoint (x, MAP2GRAPH(map, y1)) ;
	  ymin = ymax = y1 ;
	  if (getPos (map, arr(data->loci, n-1, KEY), &y2))
	    graphPoint (x, MAP2GRAPH(map, y2)) ;
	  
	  graphPointsize (0.6) ;
	  for (j = 1 ; j < n ; ++j)	/* redo n-1 to get counts line */
	    if (getPos (map, arr(data->loci,j,KEY), &y2))
	      { graphPoint (x, MAP2GRAPH(map, y2)) ;
		if (y2 != y1 && arr (data->counts, j-1, int))
		  { 
		    if (sens)
		      { if ((y2-y1) * sens  < 0)
			  { graphColor (RED) ;
			    graphFillRectangle (x - .2, MAP2GRAPH(map, y1), 
						x + .2, MAP2GRAPH(map, y2)) ;
			  }
		      }
		    else
		      sens = y2 - y1  > 0 ? 1 : -1 ;
		    graphLine (x-0.25, MAP2GRAPH(map, (y1+y2)/2),
			       x+0.25, MAP2GRAPH(map, (y1+y2)/2)) ;
		    graphColor (BLACK) ;
		  }
		y1 = y2 ;
		if (y2 < ymin) ymin = y2 ;
		if (y2 > ymax) ymax = y2 ;
	      }
	  graphLine (x, MAP2GRAPH(map, ymin), 
		     x, MAP2GRAPH(map, ymax)) ;
	  
	  graphBoxEnd() ;

	  if (seg->key == control->from)
	    control->fromBox = ibox;
	}
    }
  (*offset)++;
}

static void multiPtFollowBox(COLINSTANCE instance, int box, double x, double y)
{
  MULTIPTSEG *seg = (MULTIPTSEG *) arr(instance->map->control->boxIndex2, 
				       box, void *); 
  
  display(seg->key, instance->map->key, TREE);
}

static BOOL multiPtCreate(COLINSTANCE instance, OBJ init)
{ GMAPLOOK look = gMapLook(instance->map);
  MULTIPRIV private = (MULTIPRIV) handleAlloc(0,
					      instance->handle,
					      sizeof(struct MultiPriv));
  instance->private = private;
  private->segs = arrayHandleCreate(50, MULTIPTSEG, instance->handle);

  if(instance->proto->specialisation)
    instance->draw = multiPtRDDraw;
  else
    instance->draw = multiPtJTMDraw;
  instance->setSelectBox = multiPtSetSelect;
  instance->unSelectBox = multiPtUnselect;
  instance->doColour = multiPtDoColours;
  instance->followBox = multiPtFollowBox;

  look->multiPtCurrentColumn = instance;
  return TRUE;
}

static void multiPtDestroy(char *p)
{ COLINSTANCE instance = (COLINSTANCE)p;
  GMAPLOOK look = gMapLook(instance->map);
  
  if (look->multiPtCurrentColumn == instance)
    look->multiPtCurrentColumn = 0;
}

struct ProtoStruct gMapRDMultiPtColumn = {
  0,
  multiPtCreate,
  multiPtDestroy,
  "Multi_point_RD",
  (void *)TRUE,
  FALSE,
  0,
  0
};

struct ProtoStruct gMapJTMMultiPtColumn = {
  0,
  multiPtCreate,
  multiPtDestroy,
  "Multi_point_JTM",
  (void *)FALSE,
  FALSE,
  0,
  0
};

/********************************************************/
/************************ 2 point ***********************/
typedef struct TwoPriv {
  Array segs; /* private array of things to draw */
} *TWOPRIV;

typedef struct {
  float x, dx;
  KEY key;
  TWOPTDATA data;
} TWOPTSEG;


/***********/

float logLike2pt (TWOPTDATA data, float dist)
{
  /* log probability of observed counts at given distance */

  float p = dist / 100.0 ; /* p(recombination) */
	/* if no interference 0.5 * (1 - exp(-0.02 * dist)) */
  float XY = (1-p) * (1-p) ;
  float W = 2 + XY ;		/* 3 - 2*p + p*p */
  float X = 1 - XY ;
  int n1 = data->n1;
  int n2 = data->n2;
  int n3 = data->n3;
  int n4 = data->n4;
  KEY type = data->type;

  if (p < 0.0001)
    { p = 0.0001 ;
      XY = (1-p) * (1-p) ;
      W = 2 + XY ;
      X = 1 - XY ;
    }

  
  /* segregation of recessives from xy/++ hermaphrodites */
  if (type == _Full)			/* WT X Y XY */
    return n1*log(W) + (n2+n3)*log(X) + n4*log(XY) ;
  else if (type == _One_recombinant)	/* WT X */
    return n1*log(W) + n2*log(X) ;
  else if (type == _Selected)		/* X XY */
    return n1*log(X) + n2*log(XY) ;
  else if (type == _One_all)		/* X ALL */
    return n1*log(X) + (n2-n1)*log(W+X+XY) ;
  else if (type == _Recs_all)		/* X Y ALL */
    return (n1+n2)*log(X) + (n3-n1-n2)*log(W+XY) ;
  /* segregration from xy+/++z  - z is linked recessive lethal */
  else if (type == _One_let)		/* X ALL */
    return n1*log(X) + (n2-n1)*log(2-X) ;
  /* recessive segregation from x+/+y hermaphrodites */
  else if (type == _Tested)		/* H X */
    return n1*log(2*p*(1-p)) + (n2-n1)*2*log(1-p) ;
  else if (type == _Selected_trans)	/* X XY */
    return n1*log(1-p*p) + n2*log(p*p) ;
  else if (type == _Backcross)		/* WT X Y XY */
    return (n1+n4)*log(1-p) + (n2+n3)*log(p) ;
  else if (type == _Back_one)		/* WT X */
    return n1*log(1-p) + n2*log(p) ;
  else if (type == _Sex_full)		/* WT X Y XY */
    return (n1+n4)*log(p) + (n2+n3)*log(1-p) ;
  else if (type == _Sex_one)		/* WT X */
    return n1*log(p) + n2*log(1-p) ;
  else if (type == _Sex_cis)		/* X ALL */
    return n1*log(p) + (n2-n1)*log(2-p) ;
  else if (type == _Dom_one)		/* WT nonWT */
    return n1*log(X) + n2*log(W+X+XY) ;
  else if (type == _Dom_selected)		/* WT X */
    return n1*log(X) + n2*log(XY) ;
  else if (type == _Dom_semi)		/* XD ALL */
    return n1*log(2*p*(1-p)) + (n2-n1)*log(4-2*p*(1-p)) ;
  else if (type == _Dom_let)		/* WT ALL */
    return n1*log(X) + (n2-n1)*log(W) ;
  else if (type == _Direct)		/* R T */
    return n1*log(p) + (n2-n1)*log(1-p) ; 
  else if (type == _Complex_mixed)	/* X ALL */
    return n1*log(X) + (n2-n1)*log(5-X) ;
  else if (type == _Tetrad)		/* PD NPD TT */
    return (n1 + 0.5*n3)*log(1-p) + (n2 + 0.5*n3)*log(p) ;
  else if (type == _Centromere_segregation) /* 1st 2nd */
    return 2*n1*log(1-p) + n2*log(p) ;
  else if (type == 0)		/* Gaussian N1 = mu, N2 = err (2sd) */
      {
	if (data->error)
	  { p = (data->distance - dist) / (0.5 * data->error) ;
	    return -p*p/2 ;
	  }
	else
	  return 0 ;
      }
    else
      { messerror ("Unknown calculation type %s for 2point",
		   name (type)) ;
	return 0 ;
      }
}

/****************/

BOOL best2p (TWOPTDATA data, 
	     float *best, 
	     float *lo, 
	     float *hi)
{
  int n ;
  float p = 0, p22 = 0, p12 = 0 ;
  float testScore, x, x1, x2 ;
  int n1 = data->n1;
  int n2 = data->n2;
  int n3 = data->n3;
  int n4 = data->n4;
  KEY type = data->type;

  /* segregation of recessives from xy/++ hermaphrodites */
  if (type == _Full)			/* WT X Y XY */
    p22 = 2.0 * (n2+n3) / (float)(n1+n2+n3+n4) ;	
  else if (type == _One_recombinant)	/* WT X */
    p22 = 3.0 * n2 / (float)(n1+n2) ; 		
  else if (type == _Selected)		/* X XY */
    p22 = n1 / (float)(n1+n2) ; 			
  else if (type == _One_all)		/* X ALL */
    p22 = 4 * n1 / (float)n2 ; 			
  else if (type == _Recs_all)		/* X Y ALL */
    p22 = 2 * (n1+n2) / (float)n3 ; 			
  /* segregration from xy+/++z  - z is linked recessive lethal */
  else if (type == _One_let)		/* X ALL */
    p22 = n1 / (float)n2 ; 				
  /* recessive segregation from x+/+y hermaphrodites */
  else if (type == _Tested)		/* H X */
    p = n1 / (float)(2*n2 - n1) ; 			
  else if (type == _Selected_trans)	/* X XY */
    p = sqrt (n2 / (float)(n1+n2)) ; 			
  else if (type == _Backcross)		/* WT X Y XY */
    p = (n2+n3) / (float)(n1+n2+n3+n4) ; 		
  else if (type == _Back_one)		/* WT X */
    p = n2 / (float)(n1+n2) ; 			
  else if (type == _Sex_full)		/* WT X Y XY */
    p = (n1+n4) / (float)(n1+n2+n3+n4) ; 		
  else if (type == _Sex_one)		/* WT X */
    p = n1 / (float)(n1+n2) ; 			
  else if (type == _Sex_cis)		/* X ALL */
    p = 2 * n1 / (float)n2 ; 				
  else if (type == _Dom_one)		/* WT nonWT */
    p22 = 4 * n1 / (float)(n1 + n2) ;			
  else if (type == _Dom_selected)		/* WT X */
    p22 = n1 / (float)(n1 + n2) ;			
  else if (type == _Dom_semi)		/* XD ALL */
    p12 = 2 * n1 / (float)n2 ;			
  else if (type == _Dom_let)		/* WT ALL */
    p22 = 3 * n1 / (float)n2 ;			 
  else if (type == _Direct)		/* R T */
    p = n1 / (float)n2 ;				
  else if (type == _Complex_mixed)	/* X ALL */
    p22 = 5 * n1 / (float)n2 ;			
  else if (type == _Tetrad)		/* PD NPD TT */
    p = (n2 + 0.5*n3) / (float)(n1 + n2 + n3) ;	
  else if (type == _Centromere_segregation) /* 1st 2nd */
    p = n2 / (float)(2*n1 + n2) ;			
  else if (type == 0)			/* Gaussian N1 = mu, N2 = err */
    { *best = data->distance;
      *hi = (data->distance + data->error);
      *lo = (data->distance - data->error);
      if (*lo < 0) *lo = 0 ;
      return (data->error > 0) ;
    }
  else
    {
      messerror ("Unknown calculation type %s for 2point",
		 name (type)) ;
      return FALSE ;
    }
  
  /* solve quadratic eqn if necessary */
  if (p22)			/* p22 = 2p-p^2 */
    p = 1 - sqrt(1 - p22) ;
  else if (p12)			/* p12 = p-p^2 */
    p = (1 - sqrt(1 - 4*p12)) / 2 ;
  
  *best = 100 * p ;
				/* find *lo, *hi by splitting */
  testScore = logLike2pt (data, *best) - LOG_10 ;

  if (logLike2pt (data, 0) > testScore)
    *lo = 0 ;
  else
    { x1 = 0 ; x2 = *best ;
      for (n = 6 ; n-- ; )
	{ x = (x1 + x2) / 2 ;
	  if (logLike2pt (data, x) < testScore)
	    x1 = x ;
	  else
	    x2 = x ;
	}
      *lo = (x1 + x2) / 2 ;
    }
  
  if (!*best)
    { x1 = 0.01 ;
      if (logLike2pt (data, x1) > testScore)      
	{ *hi = x1 ;
	  return TRUE ;
	}
    }
  else
    x1 = *best ;
  x2 = 2*x1 ;
  while (logLike2pt (data, x2) > testScore) x2 *= 2 ;
  for (n = 6 ; n-- ; )
    { x = (x1 + x2) / 2 ;
      if (logLike2pt (data, x) < testScore)
	x2 = x ;
      else
	x1 = x ;
    }
  *hi = x ;

  return TRUE ;
}


static BOOL twoPtSetSelect (COLINSTANCE instance, int box, double x, double y)
{
  COLCONTROL control = instance->map->control;
  TWOPTSEG *seg = (TWOPTSEG *)arr(control->boxIndex2, box, void *);
  GMAPLOOK look = gMapLook(instance->map);

  look->selectKey = seg->key;
  look->twoPtCurrentColumn = instance;


  gMapUnselect(instance->map);
  /* We can only have neighbours */
  (void)gMapNeighbours(instance->map, &look->neighbours, seg->key);
  look->neighboursInfoValid = TRUE;

  return FALSE;
}

static BOOL twoPtUnselect(COLINSTANCE instance, int box)
{
  COLCONTROL control = instance->map->control;
  GMAPLOOK look = gMapLook(instance->map);

  *look->messageText = 0;
  if (look->messageBox)
    graphBoxDraw(look->messageBox, BLACK, CYAN);

  return FALSE;
}

static void twoPtFollowBox(COLINSTANCE instance, int box, double x, double y)
{
  TWOPTSEG *seg = (TWOPTSEG *) arr(instance->map->control->boxIndex2, 
				 box, void *); 
  
  display(seg->key, instance->map->key, TREE);
}

static void twoPtDoColour(COLINSTANCE instance, int box)
{
  MAPCONTROL map = instance->map;
  COLCONTROL control = map->control;
  TWOPTSEG *seg = (TWOPTSEG *)arr(control->boxIndex2, box, void *);
  TWOPTDATA data;
  GMAPLOOK look = gMapLook(map);
  int i, *p;
  float best, lo, hi, y1, y2 ;
  char *buf = look->messageText ;

  if (seg->key == look->selectKey &&
      instance == control->activeInstance)
    { graphBoxDraw(box, BLACK, CYAN);
      control->activeBox = box;
      data = seg->data;
      *buf = 0 ;
      
      strncpy (buf, messprintf ("2point %s: %s %s: %s",
				name(seg->key), 
				name(data->loc1), 
				name(data->loc2), 
				data->type ? name(data->type) : 
				"Simple distance" ), 100) ; /* TODO OK? */

      if (data->type)		     /* 27 chars plenty for counts */
	for (p = &data->n1, i = 0 ; i < data->count ; ++i, p++)
	  strcat (buf, messprintf (" %d", *p)) ;
      else
	strcat (buf, messprintf (" %.2f %.2f",
				 data->distance, 
				 data->error)) ;
      
      if (getPos (map, data->loc1, &y1) && getPos (map, data->loc2, &y2))
	strcat (buf, messprintf (": %.2f", (y1>y2) ? (y1-y2) : (y2-y1))) ;
      
      if (best2p (data, &best, &lo, &hi))
	strncat (buf, messprintf (" [%.2f, %.2f, %.2f]", 
				  lo, best, hi), 127 - strlen(buf)) ;
      
      if (look->messageBox)
 	graphBoxDraw(look->messageBox, BLACK, CYAN);
      
    }
  else
    { 
      if (control->activeInstance &&  
	  map == control->activeInstance->map &&
	  look->dataInfoValid &&
	  keySetFind(look->twoPt, seg->key, 0))
	graphBoxDraw(box, BLACK, GREEN);
      else if (keySetFind(look->highlight, seg->key, 0))
	graphBoxDraw(box, BLACK, MAGENTA);
      else
	graphBoxDraw(box, BLACK, WHITE);
	
    }
}

static void twoPtRDDraw (COLINSTANCE instance, float *offset)
{ 
  MAPCONTROL map = instance->map;
  COLCONTROL control = map->control;
  TWOPRIV private = instance->private;
  GMAPLOOK look = gMapLook(map);
  int   i;
  TWOPTSEG   *seg ;
  int ibox;
  float x;
  float y1, y2, best, lo, hi, cen;


  for (i = 0 ; i < arrayMax(private->segs) ; ++i)
    { seg = arrp(private->segs,i,TWOPTSEG) ;
      if (!keySetFind(look->hidden, seg->key, 0))
	{ ibox = graphBoxStart();
	  array(control->boxIndex, ibox, COLINSTANCE) = instance ;
	  array(control->boxIndex2, ibox, void *) = (void *)seg;
	  x = (*offset)++;

	  if (!getPos (map, seg->data->loc1, &y1) || 
	      !getPos (map, seg->data->loc2, &y2))
	    return ;

	  graphPointsize (0.7) ;
	  graphPoint (x, MAP2GRAPH(map, y1)) ;
	  graphPoint (x, MAP2GRAPH(map, y2)) ;
	  graphLine (x, MAP2GRAPH(map, y1), 
		     x, MAP2GRAPH(map, y2)) ;
	  
	  if (best2p (seg->data, &best, &lo, &hi))
	    { cen = (y1 + y2) / 2 ;
	      graphLine (x-0.3, MAP2GRAPH(map, cen-best/2),
			 x+0.3, MAP2GRAPH(map, cen-best/2)) ;
	      graphLine (x-0.3, MAP2GRAPH(map, cen+best/2),
			 x+0.3, MAP2GRAPH(map, cen+best/2)) ;
	      graphRectangle (x-0.3, MAP2GRAPH(map, cen-hi/2),
			      x+0.3, MAP2GRAPH(map, cen-lo/2)) ;
	      graphRectangle (x-0.3, MAP2GRAPH(map, cen+hi/2),
			      x+0.3, MAP2GRAPH(map, cen+lo/2)) ;
	    }
	  
	  graphBoxEnd() ;
 	  if (seg->key == control->from)
	    control->fromBox = ibox;
	}
    }
  (*offset)++ ;
}

static void twoPtJTMDraw (COLINSTANCE instance, float *offset)
{ 
  MAPCONTROL map = instance->map;
  COLCONTROL control = map->control;
  TWOPRIV private = instance->private;
  GMAPLOOK look = gMapLook(map);
  int   i;
  TWOPTSEG   *seg ;
  int ibox;
  float x;
  float y1, y2, best, lo, hi, c;


  for (i = 0 ; i < arrayMax(private->segs) ; ++i)
    { seg = arrp(private->segs,i,TWOPTSEG) ;
      if (!keySetFind(look->hidden, seg->key, 0))
	{ ibox = graphBoxStart();
	  array(control->boxIndex, ibox, COLINSTANCE) = instance ;
	  array(control->boxIndex2, ibox, void *) = (void *)seg;
	  x = (*offset)++;
	  
	  if (!getPos (map, seg->data->loc1, &y1) || 
	      !getPos (map, seg->data->loc2, &y2))
	    return ;
	  
	  if (y1 > y2)
	    { float tmp = y1 ; 
	      y1 = y2 ;
	      y2 = tmp ;
	    }
	  
	  /* join the 2 genes */
	  graphLine (x, MAP2GRAPH(look->map, y1), 
		     x, MAP2GRAPH(look->map, y2)) ;
	  
	  
	  if (best2p (seg->data, &best, &lo, &hi))
	    { c = (y1 + y2) / 2 ;
	      
	      /* a red rectangle from true to best positions */
	      graphColor(RED) ;
	      
	      graphFillRectangle(x-.2,MAP2GRAPH(look->map,y1),
				 x+.2,MAP2GRAPH(look->map,c - best/2)) ;
	      graphFillRectangle(x-.2,MAP2GRAPH(look->map,y2),
				 x+.2,MAP2GRAPH(look->map,c + best/2)) ;
	      
	      graphColor(BLACK) ;
	      graphRectangle(x-.2,MAP2GRAPH(look->map,y1),
			     x+.2,MAP2GRAPH(look->map,c - best/2)) ;
	      graphRectangle(x-.2,MAP2GRAPH(look->map,y2),
			     x+.2,MAP2GRAPH(look->map,c + best/2)) ;


	      /* a green error box around best position, hence hiding the red box */
	      graphColor(GREEN) ;
	      graphFillRectangle(x-.25,MAP2GRAPH(look->map,c - lo/2),
				 x+.25,MAP2GRAPH(look->map,c - hi/2)) ;
	      graphFillRectangle(x-.25,MAP2GRAPH(look->map,c + lo/2),
				 x+.25,MAP2GRAPH(look->map,c + hi/2)) ;
	      
	      graphColor(BLACK) ;
	      graphRectangle(x-.25,MAP2GRAPH(look->map,c - lo/2),
			     x+.25,MAP2GRAPH(look->map,c - hi/2)) ;
	      graphRectangle(x-.25,MAP2GRAPH(look->map,c + lo/2),
			     x+.25,MAP2GRAPH(look->map,c + hi/2)) ;
	    }
	  
	  /* point the genes, must come last */
	  graphPointsize(.7) ;
	  graphPoint(x,MAP2GRAPH(look->map,y1)) ;
	  graphPoint(x,MAP2GRAPH(look->map,y2)) ;

	  
	  graphBoxEnd() ;
 	  if (seg->key == control->from)
	    control->fromBox = ibox;
	}
    }

  (*offset)++ ;
}

BOOL twoPtAddKey(COLINSTANCE instance, KEY key)
/* Display the the piece of twoPt data whose key is key in this
   instance of the twopoint column. KEY zero is special, and means clear. */
{ TWOPRIV private = instance->private;
  TWOPTSEG *seg;
  int i;
  TWOPTDATA data;

  if (!key) /* clear */
    { arrayMax(private->segs) = 0;
      return TRUE;
    }
  if (!instance->displayed)
    instance->displayed = TRUE; /* pop it */

/* If we've already got it, don't do it again */
  for (i=0; i<arrayMax(private->segs); i++)
    { seg = arrp(private->segs, i, TWOPTSEG);
      if (seg->key == key)
	return TRUE;
    }
  /* now get a new one */
  
  if (!gMapGet2PtData(instance->map, key, instance->handle, &data))
    return FALSE;
 
  seg = arrayp(private->segs, arrayMax(private->segs), TWOPTSEG);
  seg->key = key;
  seg->data = data;
  if (data->y1 > data->y2)
    { seg->x = data->y2; seg->dx = data->y1 - data->y2; }
  else
    { seg->x = data->y1; seg->dx = data->y2 - data->y1; }
   
  return TRUE;

}  
  


static BOOL twoPtCreate(COLINSTANCE instance, OBJ init)
{
  GMAPLOOK look = gMapLook(instance->map);
  TWOPRIV private = (TWOPRIV) handleAlloc(0, 
					  instance->handle, 
					  sizeof(struct TwoPriv));

  instance->private = private;
  private->segs = arrayHandleCreate(50, TWOPTSEG, instance->handle);
  
  if (instance->proto->specialisation)
    instance->draw = twoPtRDDraw;
  else
    instance->draw = twoPtJTMDraw;
  instance->setSelectBox = twoPtSetSelect;
  instance->unSelectBox = twoPtUnselect; 
  instance->doColour = twoPtDoColour;
  instance->followBox = twoPtFollowBox;

  look->twoPtCurrentColumn = instance; 
  return TRUE;
}

static void twoPtDestroy(char *p)
{ COLINSTANCE instance = (COLINSTANCE)p;
  GMAPLOOK look = gMapLook(instance->map);
  
  if (look->twoPtCurrentColumn == instance)
    look->twoPtCurrentColumn = 0;
}

struct ProtoStruct gMapRDTwoPtColumn = {
  0,
  twoPtCreate,
  twoPtDestroy,
  "Two_point_RD",
  (void *) TRUE,
  FALSE,
  0,
  0
};

struct ProtoStruct gMapJTMTwoPtColumn = {
  0,
  twoPtCreate,
  twoPtDestroy,
  "Two_point_JTM",
  (void *) FALSE,
  FALSE,
  0,
  0
};


/******************/
/*     dbn column */
/******************/
#define NBIN 50
#define DBN_X(seg, z) ((((z)+1)*(seg)->dbnMax + \
			(NBIN-(z))*(seg)->dbnMin)/(NBIN+1))

typedef struct DbnSeg {
  KEY key;
  float dbn[NBIN], dbnMin, dbnMax;
  float x;
  COLINSTANCE instance;
} *DBNSEG;

typedef struct DbnPriv {
  Array segs;
} *DBNPRIV;

BOOL dbnAddKey (COLINSTANCE instance, KEY locus)
{ DBNPRIV private = instance->private;
  MAPCONTROL map = instance->map;
  DBNSEG seg;
  int i ;
  float x, dMax;
  KEY minLoc, maxLoc ;
  
  if (!locus) /* clear */
    { arrayMax(private->segs) = 0;
      return TRUE;
    }

  for(i=0; i<arrayMax(private->segs); i++)
    if (arr(private->segs, i, DBNSEG) && 
	arr(private->segs, i, DBNSEG)->key == locus)
      { if (!instance->displayed)
	  instance->displayed = TRUE;
	return TRUE;
      }

  getBadData() ;

  if (!getPos (map, locus, &x))
    return FALSE ;
  
  if (!instance->displayed)
    instance->displayed = TRUE;

  seg = (DBNSEG)handleAlloc(0, instance->handle, sizeof(struct DbnSeg));
  array(private->segs, arrayMax(private->segs), DBNSEG) = seg;
  seg->x = x;
  seg->instance = instance;
  seg->key = locus;

  if (!boundFind (map, locus, 
		  &seg->dbnMin, &seg->dbnMax, &minLoc, &maxLoc))
    { if (seg->dbnMin < -999999)
	seg->dbnMin = x - 10 ;
      if (seg->dbnMax > 999999)
	seg->dbnMax = x + 10 ;
    }

  dMax = -1E20 ; ;
  for (i = 0 ; i < NBIN ; ++i)
    { setTestPos (map, locus, DBN_X(seg, i)) ;
      seg->dbn[i] = logLikeLocus (map, locus) ;
      if (seg->dbn[i] > dMax)
	dMax = seg->dbn[i] ;
    }
  for (i = 0 ; i < NBIN ; ++i)
    seg->dbn[i] = exp(seg->dbn[i] - dMax) ;

  setTestPos (map, locus, x) ;
  return TRUE ;
}

static void dbnErase(void *p)
{ DBNSEG seg = (DBNSEG)p;
  DBNPRIV private = seg->instance->private;
  Array segs  = private->segs;
  int i;

  for (i=0; i<arrayMax(segs); i++)
    { if (arr(segs, i, DBNSEG) == seg)
	arr(segs, i, DBNSEG) = 0;
    }

  controlDraw();
}
  

static void dbnDraw (COLINSTANCE instance, float *offset) 
{
  int i, j, ibox;
  DBNPRIV private = instance->private;
  DBNSEG seg;
  MAPCONTROL map = instance->map;
  COLCONTROL control = map->control;
  float top, bottom, x;

  for (i = 0; i<arrayMax(private->segs); i++)
    { seg = arr(private->segs, i, DBNSEG);
      if (!seg) 
	continue; /* deleted one */
      bottom = MAP2GRAPH(map, seg->dbnMax);
      top = MAP2GRAPH(map, seg->dbnMin);
      if ((bottom < control->topMargin+2) ||
	  (top> control->graphHeight))
	continue;
	  
      graphColouredButton("X", dbnErase, seg, BLACK, WHITE, 
			  *offset+2, control->topMargin+0.5); 

      graphColor (RED) ;
        if (top > control->topMargin+2)
	  graphLine (*offset, top, *offset+6, top) ;
      if (bottom < control->graphHeight)
	graphLine (*offset, bottom, *offset+6, bottom) ;

      graphColor (BLUE) ;
      for (j = 0 ; j < NBIN ; ++j)
	{ x = MAP2GRAPH(map, DBN_X(seg, j)) ;
	  if (x > control->topMargin+2 && x < control->graphHeight)
	    graphLine (*offset, x, *offset + 6*seg->dbn[j], x) ;
	}
      graphColor (BLACK) ;
      *offset += 6 ;
    }
}


static BOOL dbnCreate(COLINSTANCE instance, OBJ init)
{ GMAPLOOK look = gMapLook(instance->map);
  DBNPRIV private = (DBNPRIV) handleAlloc(0,
					  instance->handle,
					  sizeof(struct DbnPriv));
  instance->private = private;
  private->segs = arrayHandleCreate(50, DBNSEG, instance->handle);
  instance->draw =  dbnDraw;


  look->dbnCurrentColumn = instance;
  
  return TRUE;
}

static void dbnDestroy(char *p)
{ COLINSTANCE instance = (COLINSTANCE)p;
  GMAPLOOK look = gMapLook(instance->map);
  
  if (look->dbnCurrentColumn == instance)
    look->dbnCurrentColumn = 0;
}


struct ProtoStruct gMapLikelihoodColumn = {
  0,
  dbnCreate,
  dbnDestroy,
  "Likelihood",
  0,
  FALSE,
  0,
  0
};

