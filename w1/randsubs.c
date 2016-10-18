/*  File: randsubs.c
 *  Author: Jean Thierry-Mieg (mieg@mrc-lmb.cam.ac.uk)
 *  Copyright (C) J Thierry-Mieg and R Durbin, 1992
 * -------------------------------------------------------------------
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 * -------------------------------------------------------------------
 * This file is part of the ACEDB genome database package, written by
 * 	Richard Durbin (MRC LMB, UK) rd@mrc-lmb.cam.ac.uk, and
 *	Jean Thierry-Mieg (CRBM du CNRS, France) mieg@kaa.cnrs-mop.fr
 *
 * Description:
 * Exported functions:
 * HISTORY:
 * Last edited: Dec  4 15:06 1998 (fw)
 * Created: Mon Jun 15 14:44:56 1992 (mieg)
 *-------------------------------------------------------------------
 */

 /* $Id: randsubs.c,v 1.6 2002-02-22 18:00:21 srk Exp $ */

#ifdef ALPHA
int random(void);		/* in libc.a */
#endif /* ALPHA */

/* Despite they are static 
   I consider these values as thread safe
   to complicate the code wiould serve no purpose
   simply successive calls to randInt will be
   randomly spent on all threads
   */

static int xrand = 18721 ;
static int yrand = 37264 ;    /* original value 67571 */
static int zrand = 28737 ;

/*******************************/

double randfloat (void)
{
  double x ;
  
  xrand = 171*xrand % 30269;
  yrand = 172*yrand % 30307;
  zrand = 170*zrand % 30323;
  x = xrand/30269.0 + yrand/30307.0 + zrand/30323.0;
 
  return (x-(int)x);
}

/*******************************/

#ifdef ALPHA
int randint (void)
{   
  return random() ;
}
#else
int randint (void)
{
  xrand = 171*xrand % 30269;
  yrand = 172*yrand % 30307;
  zrand = 170*zrand % 30323;
  
  return (zrand) ;
}
#endif
/*******************************/

double randgauss (void)
{
  double sum ;
  double fac = 3.0/90899.0 ;
  
  xrand = 171*xrand % 30269;
  yrand = 172*yrand % 30307;
  zrand = 170*zrand % 30323;
  sum = xrand + yrand + zrand ;
  xrand = 171*xrand % 30269;
  yrand = 172*yrand % 30307;
  zrand = 170*zrand % 30323;
  sum += xrand + yrand + zrand ;
  xrand = 171*xrand % 30269;
  yrand = 172*yrand % 30307;
  zrand = 170*zrand % 30323;
  sum += xrand + yrand + zrand ;
  xrand = 171*xrand % 30269;
  yrand = 172*yrand % 30307;
  zrand = 170*zrand % 30323;
  sum += xrand + yrand + zrand ;
  
  return (sum*fac - 6.0) ;
}

/*******************************/

void randsave (int *arr)
{
  arr[0] = xrand ;
  arr[1] = yrand ;
  arr[2] = zrand ;
}

/*********************************/

void randrestore (int *arr)
{
  xrand = arr[0] ;
  yrand = arr[1] ;
  zrand = arr[2] ;
}

/*********************************/
/*********************************/
