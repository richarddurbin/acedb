/*  File: mytime.h
 *  Author: Richard Durbin (rd@sanger.ac.uk)
 *  Copyright (C) J Thierry-Mieg and R Durbin, 1996
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
 * 	Richard Durbin (MRC LMB, UK) rd@sanger.ac.uk, and
 *	Jean Thierry-Mieg (CRBM du CNRS, France) mieg@kaa.cnrs-mop.fr
 *
 * $Id: mytime.h,v 1.17 1999-09-06 15:54:49 mieg Exp $
 * Description:
 * Exported functions:
 * HISTORY:
 * Last edited: Dec  4 14:58 1998 (fw)
 * * Jul  9 17:20 1998 (fw): added timeComparison function
 * * Jul  8 15:48 1998 (fw): added timeDiff functions for mins,hours,months,years
 *                           as required by the new AQL date-functions
 * Created: Thu Jan 25 21:30:55 1996 (rd)
 *-------------------------------------------------------------------
 */

#ifndef DEFINE_MYTIME_h
#define DEFINE_MYTIME_h

   /* march 94: these functions can be used in conjunction
      with the _DateType fundamental type which can
      now be used in the same way as _Int _Float in models.wrm
   */

typedef unsigned int mytime_t;	/* for all machines */

/* define some missing prototypes for SunOS */
#ifdef SUN
time_t  time (time_t *timer) ;
mysize_t strftime (char *buf, mysize_t bufsize, const char *fmt,
		   const struct tm *tm) ;
/* double 	difftime (time_t , time_t) ;  seems bsent on Sun */
#define difftime(__t1,__t2) ((__t1) - (__t2))
time_t 	mktime (struct tm *) ;

#else  /* non-SunOS */

#include <time.h>

#endif /* !SUN */

/*****************************************************************/

/* create a dateType from a date string */
mytime_t timeParse (char *cp) ;

/* create a string representation from a dateType */
char*    timeShow2 (char* ace_time, mytime_t t) ; /* ace_time[25], thread safe */
char*    timeShow (mytime_t t) ;

/* the following timeDiff functions will only update tdiff and return TRUE
   if both dates contain the portion of timestamp-detail that is referred to */
BOOL	 timeDiffSecs (mytime_t t1, mytime_t t2, int *tdiff) ; 
BOOL	 timeDiffMins (mytime_t t1, mytime_t t2, int *tdiff) ; 
BOOL	 timeDiffHours (mytime_t t1, mytime_t t2, int *tdiff) ; 
BOOL	 timeDiffDays (mytime_t t1, mytime_t t2, int *tdiff) ; 
BOOL	 timeDiffMonths (mytime_t t1, mytime_t t2, int *tdiff) ; 
BOOL	 timeDiffYears (mytime_t t1, mytime_t t2, int *tdiff) ;  /* always returns TRUE - as we always have years in a date/time */

/* compare two dates, returns boolean result of comparison depending on operator */
BOOL     timeComparison (int op, /* -1 for lessthan, 0 for equal, +1 for greaterthan */
			 mytime_t timeLeft, mytime_t timeRight);
/* see comments in timesubs.c for exact description of behaviour, 
   especially in cases where the level of detail specified in both dates varies */

char *timeDiffShow2 (char *buf, mytime_t t1, mytime_t t2)  ;/* buf[25], thread safe */
char*    timeDiffShow (mytime_t t1, mytime_t t2) ;
char*	 timeShowFormat (mytime_t t, char *format, char *buf, int len) ;
char*    timeShowJava2 (char* ace_time, mytime_t t) ;/* ace_time[25], thread safe */
char*    timeShowJava (mytime_t t) ;
mytime_t timeNow (void) ;
void     timeDiff (mytime_t t1, mytime_t t2, int *ydiff, int *mdiff,
                  int *ddiff, int *hdiff, int *mindiff, int *sdiff, 
                  int *minus) ;
int      monthLength (int year, int month, int minus) ;

#endif

