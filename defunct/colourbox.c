/* NOT used anymore simpleconfColour used in configuration menu */




/*  File: colourbox.c
 *  Author: Clive Brown (cgb@sanger.ac.uk)
 *  Copyright (C) J Thierry-Mieg and R Durbin, 1995
 *-------------------------------------------------------------------
 * This file is part of the ACEDB genome database package, written by
 * 	Richard Durbin (MRC LMB, UK) rd@sanger.ac.uk, and
 *	Jean Thierry-Mieg (CRBM du CNRS, France) mieg@kaa.cnrs-mop.fr
 *
 * SCCS: $Id: colourbox.c,v 1.6 1996-09-24 15:57:53 il Exp $
 * Description:
 * Exported functions:
 * HISTORY:
 * Last edited: Sep 24 11:05 1996 (il)
 *	-	return 0 rather than "FALSE" in BOX *colDraw()
 * * Jun  3 15:45 1996 (rd)
 * Created: Tue Nov  7 14:41:57 1995 (cgb)
 *-------------------------------------------------------------------
 */

#define MAX_COL 32
#include "pepdisp.h" 

typedef struct boxPos{
              int col,boxId;
              BOOL picked ;
	    }BOX;



static void setAaBox(int , int , int );
static void setOK (Array *mapCol );
static void DrawBox(BOX);
static void cleanQuit(void);
static int unDo (Array *oldCols);

static MENUOPT colOpts[] = {
 cleanQuit,"Quit",
 graphPrint,"Print Screen",
 0,0};

static Array colBoxes=0,aaBoxes =0;


/**********************************************************************************************************/
/**********************************************************************************************************/
static void colDraw(double * x, double * y, BOOL yield)
{
  int i,j,k;
  float xstep, ystep, nexty,oldy,nextx,oldx;
  
  if(!arrayExists(colBoxes))
    colBoxes = arrayCreate(MAX_COL , BOX);
  
  if(!yield)
    {
      xstep = (*x - (*x/2))/5 ;
      oldx = (*x/2)+0.2;
      
      ystep = (*y-5) / (MAX_COL/5);
      oldy = 3; 
      graphRectangle(*x-((*x/2)+1), 2, *x-0.5, *y+0.5);
      graphText("Colours :",*x-((*x/2)+1),0.5);
      j =0;
      k=0;
      for(i=0 ;i< MAX_COL ; i++)
	{
	  j = graphBoxStart();
	  nextx = oldx + xstep/2;
	  nexty = oldy + ystep/2;
            
	  graphRectangle (oldx, oldy, nextx,nexty);
        
	  graphBoxEnd();     
	 
	  array(colBoxes,i,BOX).col   = j;
          array(colBoxes,i,BOX).picked = FALSE;
          array(colBoxes,i,BOX).boxId = j;
	  oldx += xstep;
	  k++;
	  if(k > 4){ k=0; oldy += ystep; oldx = (*x/2)+0.2;}
          DrawBox(array(colBoxes, i, BOX));      
	}
    }
}
/******************************************************************************************************/
static void AADraw(Array *mapCol, double * x, double * y, BOOL yield)
{
  int i,k;
  int j;
  float xstep, ystep, nexty,oldy,nextx,oldx;
  char aa[10];

  if(!yield)
    {
      if(!arrayExists(aaBoxes)) aaBoxes = arrayCreate(arrayMax(arr(*mapCol,0,Array)), BOX );
      
      xstep = ((*x/2)-1.5) ;
      oldx = 1.5;
      
      ystep = (*y-2) / arrayMax(arr(*mapCol,0,Array));
      oldy = 3; 
     
      graphRectangle( 1, 2.0, (*x/2)-0.5, *y+0.5);
      graphText(" Residues : ",1,0.5);
      j =0;
      k=0;
      for (i=0 ;i< arrayMax(arr(*mapCol,0,Array)) ; i++)
	{ 
	  sprintf (aa, "   %s : %c = ", pepShortName[arr(arr(*mapCol,0,Array),i,char)],arr(arr(*mapCol,0,Array),i,char) ) ;
	  graphText(aa, oldx, oldy);
          nextx = oldx + xstep-1.5;
	  nexty = oldy + ystep/2;
	 
      	  j = graphBoxStart();
	 
	  graphRectangle (oldx+((nextx-oldx)/2), oldy, nextx,nexty);
	  graphRectangle (oldx+((nextx-oldx)/2)-0.1, oldy-0.1, nextx+0.1,nexty+0.1);
	  graphBoxEnd();
	  array(aaBoxes,i,BOX).boxId = j;     
	  array(aaBoxes,i,BOX).col   = array(arr(*mapCol,1,Array),i,int); /* replaced j with i since i presume j was wrong il */
          array(aaBoxes,i,BOX).picked = FALSE;
	  oldx += xstep;
	  k++;
	  if(k > 0){ k=0; oldy += ystep; oldx = 1.5;} 
          DrawBox(array(aaBoxes,i,BOX));     
	}
    }
}
/******************************************************************************************************/
static void setBox(int box, int x, int y)
{
 int i, index =-1;
   

 for(i=0; i< arrayMax(colBoxes);i++) if(box == arrp(colBoxes,i,BOX)->boxId) index = i;

 setAaBox(box, x, y);

 if((colBoxes) && (index >-1))
   {
     graphColor(BLACK);
     if(arrp(colBoxes,index,BOX)->picked == TRUE)
       {
	 arrp(colBoxes,index,BOX)->picked = FALSE;  
	 DrawBox(arr(colBoxes,index,BOX));
      }else{
	 arrp(colBoxes,index,BOX)->picked = TRUE;
	 DrawBox(arr(colBoxes,index,BOX));
      }
   }
}
/*******************************************************************************************************/
static void setAaBox(int box, int x, int y)
{
 int i, index = -1;
   

 for(i=0; i < arrayMax(aaBoxes); i++) if(arrp(aaBoxes,i,BOX)->boxId == box) index = i;

 if((aaBoxes) && (index != -1))
   {
     graphColor(BLACK);
     if(arrp(aaBoxes,index,BOX)->picked == TRUE)
       {
	 arrp(aaBoxes,index,BOX)->picked = FALSE;  
	 DrawBox(arr(aaBoxes,index,BOX));
     }else{
	 arrp(aaBoxes,index,BOX)->picked = TRUE;  
	 DrawBox(arr(aaBoxes,index,BOX));
      }
   }
 
}
/****************************************************************************************************/
static void cleanQuit(void)
{

 unDo(NULL);  
 if(aaBoxes) arrayDestroy(aaBoxes);
 if(colBoxes) arrayDestroy(colBoxes);
 graphLoopReturn(0);
 graphDestroy();

}
/*****************************************************************************************************/
static int unDo (Array *oldCols)
{
 static Array undo = 0;
 int i;

 if((!undo) && (oldCols))
   {
     undo = arrayReCreate(undo,2,Array);
     array(undo,0,Array) = arrayCopy(arr(*oldCols,0,Array)); /* residue names */
     array(undo,1,Array) = arrayCopy(arr(*oldCols,1,Array));  /* residue colour */
 }  
 else if((oldCols) && (undo))
   for(i=0; i < arrayMax(arr(*oldCols,0,Array)); i++)
     {
       array(arr(*oldCols,0,Array),i,char) =arr(arr(undo,0,Array),i,char); 
       array(arr(*oldCols,1,Array),i,int) =arr(arr(undo,1,Array),i,int); 
       arrp(aaBoxes,i,BOX)->col = arr(arr(undo,1,Array),i,int);
       DrawBox(arr(aaBoxes,i,BOX));
     }                                    /* memcpy is more efficient */
 else if(oldCols == NULL && undo != 0){
   arrayDestroy(arr(undo,0,Array));
   arrayDestroy(arr(undo,1,Array));
   arrayDestroy(undo);
 }
 return(1);
}

/******************************************************************************************************/
static void DrawBox (BOX  box)
{
  int fcol = WHITE;
  
  if(box.picked == TRUE) fcol = BLACK;
  graphBoxDraw(box.boxId , fcol, box.col);
}
/******************************************************************************************************/
static void unSelect (void)
{
  int i;
  
  for(i=0; i < arrayMax(colBoxes); i++){ arrp(colBoxes,i,BOX)->picked = FALSE; DrawBox(arr(colBoxes,i,BOX));}
  for(i=0; i < arrayMax(aaBoxes); i++){ arrp(colBoxes,i,BOX)->picked = FALSE; DrawBox(arr(colBoxes,i,BOX));}
}
/*******************************************************************************************************/
static void setOK (Array *colMap)
{
  int i, color ;
  int x,y;
 
  graphFitBounds(&x,&y);

  for(i=1; i < arrayMax(colBoxes); i++)
    if (arrp(colBoxes,i,BOX)->picked)
      color = arrp(colBoxes,i,BOX)->col;
     
  for(i=0; i < arrayMax(aaBoxes); i++)
    if(arrp(aaBoxes,i,BOX)->picked) 
      {
	arrp(aaBoxes,i,BOX)->col  = color;
	array(arr(*colMap,1,Array),i,int) = color;
	DrawBox(arr(aaBoxes,i,BOX));
      }

  unSelect();
}
/*******************************************************************************************************/
static void colSave (Array *mapCol)
{
}
/*******************************************************************************************************/
static void colLoad (Array *mapCol)
{
}
/*******************************************************************************************************/
extern void colBox (Array *mapCol)
{
 Graph colBox ;
 int nx, ny ;
 double x, y ;
   
 colBox = graphCreate(TEXT_FIT, "Colour Box ",0,0,0.6, 0.7);
 graphMenu (colOpts); 
 graphRegister(PICK ,setBox);
 graphRegister(RESIZE, graphRedraw);
 
 graphBoxStart();
 
 graphFitBounds(&nx,&ny);
 x = nx ;
 y = ny-6 ;
 colDraw(&x, &y,FALSE);
 AADraw(mapCol, &x, &y, FALSE);
 graphColouredButton(" <<- CHANGE  ", (ColouredButtonFunc)setOK, mapCol, BLACK, WHITE, nx/3, ny-5);
 if (unDo(mapCol)) 
   graphColouredButton(" Undo Last  ->>", (ColouredButtonFunc)unDo, mapCol, BLACK, WHITE, nx/3, ny-3);
 graphColouredButton("    DONE    ", (ColouredButtonFunc)cleanQuit, NULL, BLACK, WHITE,(nx/3)*2, ny-1);
 graphColouredButton(" LOAD ", (ColouredButtonFunc)colLoad, mapCol, BLACK, WHITE, 2, ny-5);
 graphColouredButton(" SAVE ", (ColouredButtonFunc)colSave, mapCol, BLACK, WHITE, 2, ny-3);
 graphRedraw();        
 
 graphBoxEnd();
 
 graphLoop(TRUE);
}



 
