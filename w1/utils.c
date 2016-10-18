/*  File: utils.c
 *  Author: Ed Griffiths (edgrif@sanger.ac.uk)
 *  Copyright (c) J Thierry-Mieg and R Durbin, 1999
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
 *
 * -------------------------------------------------------------------
 * This file is part of the ACEDB genome database package, written by
 * 	Richard Durbin (Sanger Centre, UK) rd@sanger.ac.uk, and
 *	Jean Thierry-Mieg (CRBM du CNRS, France) mieg@kaa.crbm.cnrs-mop.fr
 *
 * Description: Contains utility functions that are "one-offs" and not
 *              really part of any other package.
 * Exported functions: getLogin
 * HISTORY:
 * Last edited: Mar 27 17:44 2009 (edgrif)
 * * Mar 22 14:42 1999 (edgrif): Added getSystemName().
 * * Jan 27 09:20 1999 (edgrif): Add inokedebugger/regExpMatch from messubs.
 * * Jan 25 16:05 1999 (edgrif): Add getLogin from session.c
 * Created: Thu Jan 21 15:46:49 1999 (edgrif)
 * CVS info:   $Id: utils.c,v 1.47 2009-03-30 09:09:04 edgrif Exp $
 *-------------------------------------------------------------------
 */

#include <wh/regular.h>

#ifdef SGI
#include <math.h>					    /* work around SGI library bug */
#endif

#include <sys/resource.h>
#include <wh/version.h>					    /* For UT_ macros. */
#include <glib.h>                                           /* for g_strup() used in strcasestr() */
#include <ctype.h>                                          /* for toupper() */
#include <utils.h>                                          /* for strcasestr & arrstrcmp */
#include <chrono.h>

#ifdef IBM
/* I can't believe this is still required for IBM machines...sigh..          */

struct passwd {
  char    *pw_name;
  char    *pw_passwd;
  long   pw_uid;
  long   pw_gid;
  char    *pw_gecos;
  char    *pw_dir;
  char    *pw_shell;
} ;
extern struct passwd *getpwuid (long uid) ;
extern struct passwd *getpwnam (const char *name) ;

#else  /* !IBM */

#include <pwd.h>

#endif /* !IBM */


/* This is all pretty unsafe at the moment, it relies on the user having     */
/* called getLogin to initialise the below two globals before using them..   */
/* These globals are also separately set by session.c using the system       */
/* calls getuid() and geteuid(), this is not optimal.                        */
/*                                                                           */
uid_t ruid = -1, euid = -1;

#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
/* Currently unused, there should be a unified interface to uids but this    */
/* requires more understanding of how they are used in the code before       */
/* implementing.                                                             */
uid_t utGetRuid(void)
{
  return ruid ;
}

void utSetRuid(uid_t newRuid)
{
  ruid = newRuid ;
}

uid_t utGetEuid(void)
{
  return euid ;
}

void utSetEuid(uid_t newEuid)
{
  euid = newEuid ;
}
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */



/*************************************************************/
/****************** getLogin() *******************************/

/**** getLogin() for real/effective user names 
***** original by 03.05.1992 kocab DKFZ  
*****/

char *getLogin (BOOL isReal)
     /* general public function */
{
  /* isReal for real or effective can't fail */

#if defined(MACINTOSH)

  return "acedb" ;
  
#elif defined (WIN32)

  char *name = NULL;
  if ((name = getenv("USERNAME")) != NULL )
    return name ;   /* Windows NT has usernames */
  else
    return "acedb" ;

#else  /* all UNIX */
  static char *rname = 0 ;
  static char *ename = 0 ;

/* RD 980417: changed so getlogin() was last resort.  It can return
   "root" inappropriately where getpwuid(ruid)->pw_name returns the
   right answer (ddts problem SANgc02099)
*/

  if (!ename)
    {
      if (ruid == -1)
	{ ruid = getuid() ;
	euid = geteuid () ;
	}
      
      if (!rname)
	if (ruid)
	  if (getpwuid(ruid))
	    rname = strnew(getpwuid(ruid)->pw_name, 0) ;
      if (!rname)
	if (getlogin())
	  rname = strnew(getlogin(), 0) ;
      if (!rname)
	rname = "getLoginFailed" ;
      
      if (!ename)
	if (getpwuid(euid))
	  ename = strnew(getpwuid(euid)->pw_name, 0) ;
      if (!ename)
	ename = rname ;
      /* mieg: jan5 99, new problem
	 on my dec alpha, if i run as a daemon, getLogin fails totally
	 printf ("rname=%s ename =%s",rname?rname:"null",ename ? ename:"null") ; 
	 */
    }

  return isReal ? rname : ename ;
#endif /* UNIX */

} /* getLogin */


/* Get the system name using the POSIX uname call.                           */
char *getSystemName(void)
{
  static char *sysname = NULL ;
  
  if (sysname == NULL)
    {
      struct utsname sys_details ;
      
      if (uname(&sys_details) != 0)
	{
	  /* POSIX failed... */
	  sysname = messalloc(101);
	  if (gethostname(sysname, 100) == -1)
	    messcrash("cannot retrieve system name.") ;
      }
      else
	sysname = strnew(sys_details.nodename, 0) ;
    }
  
  return sysname ;
}

char *getUserHomeDir (char *user_name, STORE_HANDLE h)
{
  struct passwd *pwd;
  char *homedir = NULL;

  if ((pwd = getpwnam (user_name)))
    homedir = strnew (pwd->pw_dir, h);
  else
    messerror ("Unknown user: %s", user_name);

  return homedir;
} /* getUserHomeDir */

/*****************************/



#ifdef ED_G_NEVER_INCLUDE_THIS_CODE
/* Some command line stuff.....                                              */
/*                                                                           */
/* I'd like to make use of the gnu stuff for command lines which uses        */
/* getopt.c and getopt1.c and have a package like Xt for handling options    */
/* uniformly.                                                                */
/*                                                                           */

/* Basic idea is not to overdesign this, in theory we could have subsystems  */
/* registering their options and then possibly having them rejected etc. etc.*/
/* but this is overkill. We can just have one place for options, a main      */
/* table looked after by this code. We'll use the table to contruct the args */
/* to the getopt routines and also to construct the "usage" text. This way   */
/* the usage strings will stay up to date and it will be easier to add new   */
/* options.                                                                  */
typedef enum UtOptArgType_ {UT_OPTARG_NONE, UT_OPTARG_OPTIONAL, UT_OPTARG_MANDATORY} UtOptArgType ;

typedef struct UtCmdLineOptions_
{
  char short_form ;
  char *long_form ;
  char *option_title ;
  char *usage_text ;
  UtOptArgType arg_type ;
} UtCmdLineOption ; 

/* Potential problem: Can we make getopts only look for the args that it     */
/* knows about ????  i.e. can we leave gtk args alone ??                     */

void utGetCmdLineOptions(UtCmdLineOption options[], int num_options)
{
  int i ;
  struct option *getopt_options = NULL ;
  char *short_options = NULL ;

  getopt_options = messalloc((num_options + 1), option) ;   /* Need null terminating entry */
  short_options = messalloc(((num_options * 2) + 1), char) ; /* Can't be more than * 2 */

  for (i = 0, j = 0 ; i < num_options ; i++)
    {
      getopt_options[i].name = options[i].long_form ;
      short_options[j++] = options[i].short_form ;
      switch(options[i].arg_type)
	{
	case UT_OPTARG_NONE:
	  getopt_options[i].has_arg = no_argument ;
	  break ;
	case UT_OPTARG_OPTIONAL:
	  getopt_options[i].has_arg = optional_argument ;
	  short_options[j++] = ':' ;
	  break ;
	case UT_OPTARG_MANDATORY:
	  getopt_options[i].has_arg = required_argument ;
	  short_options[j++] = ':' ;
	  break ;
	}
      getopt_options[i].flag = NULL ;
      getopt_options[i].val = options[i].short_form ;
    }
  getopt_options[i].name = NULL, getopt_options[i].has_arg = 0,
    getopt_options[i].flag = NULL, getopt_options[i].val = 0 ;
  short_options[j] = '\0' ;

  return ;
}
#endif /* ED_G_NEVER_INCLUDE_THIS_CODE */




/*****************************/

BOOL getCmdLineOption (int *argcp, char **argv,
		       char *arg_name, char **arg_val)
     /* call with (&argc, argv, "-option", &string)
      *   for options with value (e.g. -display <name>)
      * or with   (&argc, argv, "-option", NULL)
      *   for options that are simple switches (e.g. -help)
      * RETURNS:
      *   TRUE if option was found (will set *arg_val if non-NULL)
      * SIDE-EFFECT:
      *   The option (and possibly its value) will be removed from
      *   the aguments (argc,argv). Call only once per option.
      */
{
  int i, num;
  BOOL isFound = FALSE;

  for (i = 1; i < *argcp; i++)
    {
      if (strcmp(argv[i], arg_name) == 0)
	{
	  if (arg_val)
	    {
	      if ((*argcp - i) < 2)
		messExit ("No argument specified for %s option", 
			  arg_name);

	      *arg_val = strnew(argv[i+1], 0);
	      num = 2;
	    }
	  else
	    num = 1;

	  /* clear argument(s) from the list */
	  for (i += num; i < *argcp; i++)
	    argv[i - num] = argv[i];
	  argv[*argcp - num] = 0;
	  (*argcp) -= num;

	  isFound = TRUE;
	  break;
        }
    }
  return isFound;
} /* getCmdLineOption */


/*****************************/

/* Check to see if user supplied the "-sleep secs" option, this is useful    */
/* for debugging when the acedb program is called from within a script. It   */
/* means that you can make the acedb program sleep until you can attach a    */
/* debugger to see what its doing.                                           */
/*                                                                           */
/* This routine is not perfect but its only intended for debugging use.      */
/*                                                                           */
void checkCmdLineForSleep(int *argcp, char **argv)
{
  char *secs_str = NULL ;

  if (getCmdLineOption(argcp, argv, "-sleep", &secs_str))
    {
      int secs = 0 ;

      if (secs_str != NULL)
	secs = atoi(secs_str) ;

      if (secs == 0)					    /* atoi returns 0 on error. */
	secs = 60 ;

      sleep(secs) ;
    }

  return ;
}

/*****************************/

/* This routine tries to set various process resource limits to their max    */
/* values. We especially need this for acedb which certainly requires lots   */
/* of memory and sometimes a large stack as well.                            */
/*                                                                           */
/* The routine reports on any resources that cannot be increased to a        */
/* reasonable level and gives the user the choice of whether to continue.    */
/*                                                                           */
/* The routine will be a noop for machines that don't support the required   */
/* get/setrlimit calls.                                                      */
/*                                                                           */
void utUnlimitResources(BOOL allow_user_abort)
{

#if defined(RLIMIT_DATA) && defined(RLIMIT_STACK)	    /* Usually in sys/resource.h */

  typedef struct ResourceLimit_ 
  {
    int resource ;					    /* resource id from sys/resource.h */
    char *resource_str ;				    /* Stringified resource id. */
    char *resname ;					    /* our string name for resource. */
    int min_limit ;					    /* our required minimum. */
  } ResourceLimit ;

  ResourceLimit our_limits[] = {{RLIMIT_DATA, UT_PUTSTRING(RLIMIT_DATA), "heap size", 52428800},
				{RLIMIT_STACK, UT_PUTSTRING(RLIMIT_STACK), "stack size", 4194304}} ;
  int num_resources = UtArraySize(our_limits) ;
  struct rlimit reslimits ;
  Stack errors = NULL ;
  int i ;
  int result = 0;


  /* Check the hard limits....                                               */
  for (i = 0 ; i < num_resources ; i++)
    {
      if (getrlimit(our_limits[i].resource, &reslimits) != 0)
	messcrash("Unable to retrieve current limits for %s (%s), reason was %s",
		  our_limits[i].resource_str, our_limits[i].resname, messSysErrorText()) ;
      else
	{
	  if (reslimits.rlim_max < our_limits[i].min_limit)
	    {
	      char *resource = NULL ;

	      if (errors == NULL)
		{
		  errors = stackCreate(20) ;
		  pushText(errors,
			   "Your environment has the following user process \"hard\" limits"
			   " which are sufficiently low that they may cause acedb to crash"
			   " (NOTE that you need root permission to raise them): ") ;
		}
	      else
		catText(errors, ", ") ;

	      resource = hprintf(0, "%s (%d bytes)", our_limits[i].resname, reslimits.rlim_max) ;
	      catText(errors, resource) ;
	      messfree(resource) ;
	    }
	  else
	    {
	      if (reslimits.rlim_cur < reslimits.rlim_max)
		{
		  reslimits.rlim_cur = reslimits.rlim_max ;
		  if ((result = setrlimit (our_limits[i].resource, &reslimits)) != 0)
#ifdef __CYGWIN__
		    if (result != 22)   /* on Windows, ignore "can't increase stacksize" msg. */
#endif
		    messcrash("Unable to increase current limit for %s, reason was %s",
			      our_limits[i].resname, messSysErrorText()) ;
		}
	    }
	}
    }

  if (errors != NULL)
    {
      if (allow_user_abort)
	{
	  if (!messQuery("%s. Do you you want to continue ?", popText(errors)))
	    messExit("User initiated exit.") ;
	}
      else
	{
	  /* n.b. this may be a noop if the messdump context has not been    */
	  /* initialised yet.                                                */
	  messdump("%s", popText(errors)) ;
	}
    }

#endif /* defined(RLIMIT_DATA) && defined(RLIMIT_STACK) */

  return ;
}


/************************************************************/

/* put "break invokeDebugger" in your favourite debugger init file */
void invokeDebugger (void) 
{
  static BOOL reentrant = FALSE ;

  if (!reentrant)
    { reentrant = TRUE ;
      messalloccheck() ;
      reentrant = FALSE ;
    }
}


/*****************************/
/* match to reg expression 

   returns 0 if not found
           1 + pos of first sigificant match (i.e. not a *) if found
*/

int regExpMatch (char *cp,char *tp)
{
  char *c=cp, *t=tp;
  char *ts=0, *cs=0, *s = 0 ;
  int star=0;

  while (TRUE)
    switch(*t)
      {
      case '\0':
 	if(!*c)
	  return  ( s ? 1 + (s - cp) : 1) ;
	if (!star)
	  return 0 ;
        /* else not success yet go back in template */
	t=ts; c=cs+1;
	if(ts == tp) s = 0 ;
	break ;
      case '?' :
	if (!*c)
	  return 0 ;
	if(!s) s = c ;
        t++ ;  c++ ;
        break;
      case '*' :
        ts=t;
        while( *t == '?' || *t == '*')
          t++;
        if (!*t)
          return s ? 1 + (s-cp) : 1 ;
        while (freeupper(*c) != freeupper(*t))
          if(*c)
            c++;
          else
            return 0 ;
        star=1;
        cs=c;
	if(!s) s = c ;
        break;
#ifdef CRAZY_TEST
      case 'A' :
	if (!*c || (*c < 'A' || *c > 'Z'))
	  return 0 ;
	if(!s) s = c ;
        t++ ;  c++ ;
        break;
#endif
      default  :
        if (freeupper(*t++) != freeupper(*c++))
          { if(!star)
              return 0 ;
            t=ts; c=cs+1;
	    if(ts == tp) s = 0 ;
          }
	else
	  if(!s) s = c - 1 ;
        break;
      }
}


/* This SGI workaround make the comiler crash! I've disabled it for now,
   pending determination if it is still needed, or fixes a long-lost library
   bug. - srk */
#if 0

/* work around SGI library bug */
#ifdef  SGI 
double log10 (double x) { return log(x) / 2.3025851 ; }
#endif

#endif


/*************************************************************/

/* calling utilsInit will initialise all the statics makeing
   later calls to getLogin and getSystemName effectively threadsafe
   */

void utilsInit (void)
{
  static int n = 0 ;

  if (n)
    messcrash ("utilsInit is not thread safe, it should be called only once") ;
  n = 1 ;
  getSystemName () ;
  getLogin (TRUE) ;  
}



/****************** little string routines ************************/


/* Translates newlines into escaped newlines, i.e. '\n' becomes '\''n'     */
char *utCleanNewlines(char *s, STORE_HANDLE handle)
{ 
  int n = 0 ;
  char *cp, *cq ;
  char *copy = NULL ;

  for (cp = s ; *cp ; ++cp)
    if (*cp == '\n') ++n ;

  if (n == 0)
    copy = s ;
  else
    {
      copy = halloc (cp-s+n+1, handle) ;
      for (cp = s, cq = copy ; *cp ; ++cp)
	if (*cp == '\n') 
	  { *cq++ = '\\' ; *cq++ = 'n' ; }
	else
	  *cq++ = *cp ;
      *cq = 0 ;
    }

  return copy ;
}


/****************** little arithmetic routines ************************/

int utArrondi (float x)
     /*   1.3 ->  1 ;  1.7 ->  2
      *  -1.3 -> -1 ; -1.7 -> -2 */
{
  if (x >= 0)
    return (int) (x + 0.5) ;
  else
    return (int) (x - 0.5) ;
}

/********************************************/

int utMainPart (float p)
 /* returns 1, 2, 5, 10, 20, 50, 100 etc 
  * in absolute value the returned number is smaller than p
  *   19 -> 10  ; -19 -> -10  */
{
  register int i = 1, sign = 1;
  
  if (!p)
    return 0 ;
  if (p < 0) 
    { sign  = -1 ; p = -p ; }
  while(i <= p + .00001) 
    i = 10 * i;
  i /= 10 ;

  if (2 * i > p)
    return i * sign;
  if(5 * i > p)
    return 2 * i * sign;
  return
    5*i*sign;
}

/********************************************/

int utMainRoundPart (float p)
 /* returns 1, 2, 5, 10, 20, 50, 100 etc 
  * but the returned number may be a bit bigger than p
  *   19 -> 20  ; -19 -> -20  */
{
  register int i = 1, sign = 1;

  if (!p)
    return 0 ;
  if (p < 0) 
    {sign = -1 ; p = -p ; } /* RD: ambiguous without spaces on SGI */
  while (i < p + .1) 
    i = 10 * i;
  if (4 * i < 5 * p) 
    return i * sign;
  i /= 2;
  if (4 * i < 5 * p) 
    return i * sign;
  i =  (2 * i) / 5;
  if (4 * i < 5 * p)
    return i * sign;
  i = i/2; 
  return i * sign;
}

/*********************************************************/
double utDoubleMainPart (double p)
     /* returns 1 , 2 , 5 ,10 , 20, 50 ,100 etc */
{
  register double i = 1, sign = 1;

  if (!p)
    return 0.;
  if(p < 0) 
    {sign = -1; p = -p;}
  i = (double) exp(log((double)10.0) * (double)(1 + (int)(log10((double)p)))) ;
  if (4 * i < 5 * p) 
    return i * sign;
  i /= 2;
  if (4 * i < 5 * p)
    return i * sign;
  i = (2 * i) / 5;
  if (4 * i < 5 * p)
    return i*sign;
  i = i/2;
  return i*sign;
}


/*********************************************************/

/* Some of the unix number conversion routines are arcane to put it mildly,  */
/* here are some that (hopefully) work in a more straightforward way....     */

/* N.B. these should be integrated with the code in freesubs.c for freeint() */
/* etc. at some time.                                                        */

/* Given a string will attempt to convert it to an int, the whole string     */
/* must be a valid integer: [+-]?[0-9]*                                      */
/*                                                                           */
/* returns TRUE if the number could be converted and sets num_out to the     */
/* number, FALSE otherwise.                                                  */
/*                                                                           */
BOOL utStr2Int(char *num_str, int *num_out)
{
  BOOL result = FALSE ;
  long int retval = 0 ;
  
  if ((result = utStr2LongInt(num_str, &retval)))
    {
      if (retval >= INT_MIN && retval <= INT_MAX)
	{
	  result = TRUE ;
	  *num_out = (int)retval ;
	}
    }

  return result ;
}


/* Given an int between 0 and 9 returns the corresponding char representation,
 * surely this must exist somewhere....returns '.' if number not in [0-9] */
char utInt2Char(int num)
{
  char result = '.' ;
  char table[] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9'} ;

  if (num >= 0 && num <= 9)
    result = table[num] ;

  return result ;
}






/* Given a string will attempt to convert it to a long int, the whole string */
/* must be a valid integer: [+-]?[0-9]*                                      */
/*                                                                           */
/* returns TRUE if the number could be converted and sets num_out to the     */
/* number, FALSE otherwise.                                                  */
/*                                                                           */
BOOL utStr2LongInt(char *num_str, long int *num_out)
{
  BOOL result = FALSE ;
  long int retval = 0 ;
  char *cp = NULL, *endptr = NULL ;
  int i, num_len, translation ;

  /* It is not possible to detect invalid numbers of the form "1232rubbish"  */
  /* because strtol() will convert as much as it can ("1232" in this case)   */
  /* and return as though everything is OK....sigh...                        */
  /* So here I check that the string is all digits...                        */
  num_len = strlen(num_str) ;
  cp = num_str ;
  if (*cp == '-' || *cp == '+')
    {
      cp++ ;
      num_len-- ;
    }
  for (i = 0, translation = 1 ; i < num_len && translation != 0 ; i++, cp++)
    {
      translation = isdigit((int)*cp) ;
    }

  if (translation != 0)
    {
      errno = 0;
      retval = strtol(num_str, &endptr, 10) ;
      if (retval == 0 && (errno != 0 || num_str == endptr))
	{
	  result = FALSE ;
	}
      else if (errno !=0 && (retval == LONG_MAX || retval == LONG_MIN))
	{
	  result = FALSE ;
	}
      else
	{
	  result = TRUE ;
	  *num_out = retval ;
	}
    }

  return result ;
}

/************************************************************/
/* case-insensitive version of strstr */

#ifndef DARWIN
char *strcasestr(char *str1, char *str2)
{
  g_strup(str1);
  g_strup(str2);

  return strstr(str1, str2);
}
#endif


/***************************************************************/
/* Wrapper for lexstrcmp to allow it to be used with arraySort */
/* ie for sorting arrays of character pointers.                */
/* lexstrcmp does case-insensitive, intuitive comparison of    */
/* potentially mixed alpha/numeric strings, but requires char* */
/* parameters, while arraySort ultimately utilises qsort which */
/* requires void * parameters.                                 */
/***************************************************************/
int arrstrcmp(void *s1, void *s2)
{
  char *a = *(char **)s1;
  char *b = *(char **)s2;

  return lexstrcmp(a, b);
}

/**************************************************************/
/* Correctly sorts anything containing integers               */
/* lexRename relies on the fact that lexstrcmp must fail      */
/* if the length differ                                       */
/* The chrono functions are for tracking performance and at   */
/* present are null functions.                                */
/**************************************************************/
int lexstrcmp(char *a,char *b)
{ 
  register char c,d,*p,*q ;
  register int  nbza, nbzb ; /* nb de zeros en tete */
  register int  nbzReturn = 0 ;
  chrono("lexstrcmp") ;

  while (*a)
    {                /* Bond007< Bond07 < Bond7 < Bond68 */
      if (isdigit((int)*a) && isdigit((int)*b))
        { 
	  for (nbza = 0 ; *a == '0' ; ++a, nbza++) ;  /* saut des premiers zeros */
          for (nbzb = 0 ; *b == '0' ; ++b, nbzb++) ;
          for (p = a ; isdigit((int)*p) ; ++p) ;
          for (q = b ; isdigit((int)*q) ; ++q) ;
          if (p-a > q-b) { chronoReturn() ; return 1 ; }  /* the longer number is the bigger */
          if (p-a < q-b) { chronoReturn() ; return -1; }
          while (isdigit ((int)*a))
            { 
	      if (*a > *b)     { chronoReturn() ; return 1 ; }
              if (*a++ < *b++) { chronoReturn() ; return -1; }
            }
          if (!nbzReturn)
            { 
	      if (nbza < nbzb) nbzReturn = +1 ;
              if (nbza > nbzb) nbzReturn = -1 ;
            }
        }
      else
        { 
	  if((c=freeupper(*a++))>(d=freeupper(*b++)))
            { chronoReturn() ; return 1 ; }
          if(c<d) { chronoReturn() ; return -1 ; }
        }
    }
 if (!*b)
   { chronoReturn() ; return nbzReturn ; }

 chronoReturn() ; return -1 ;
}

/************************************************************/
/* Same as above but CASE SENSITIVE                         */
/* Correctly sorts anything containing integers             */
/* lexRename relies on the fact that lexstrcmp must fail    */
/*  if the length differ                                    */
/* The chrono functions are for tracking performance and at */
/* present are null functions.                              */
/************************************************************/
int lexstrCasecmp(char *a,char *b)
{ 
  register char c,d,*p,*q;
  register int  nbza = 0, nbzb = 0 ; /* nb de zeros en tete */
  chrono("lexstrCasecmp") ;
  while (*a)
    {                /* Bond007< Bond07 < Bond7 < Bond68 */
      if (isdigit((int)*a) && isdigit((int)*b))
	{ 
	  nbza = nbza * 10 ; /* pour plusieurs series de 0 */
	  nbzb = nbzb * 10 ;
	  for (p = a ; *p == '0' ; ++p, nbza++) ;  /* saut des premiers zeros*/
	  a = p ;
	  for (q = b ; *q == '0' ; ++q, nbzb++) ;
	  b = q ;
	  for (p = a ; isdigit((int)*p) ; ++p) ;
	  for (q = b ; isdigit((int)*q) ; ++q) ;
	  if (p-a > q-b) { chronoReturn() ; return 1 ; }  /* the longer number is the bigger */
	  if (p-a < q-b) { chronoReturn() ; return -1; }
	  while (isdigit ((int)*a))
	    { 
	      if (*a > *b)     { chronoReturn() ; return 1 ; }
	      if (*a++ < *b++) { chronoReturn() ; return -1; }
	    }
	}
      else
        { 
	  if((c=(*a++))>(d=(*b++))) { chronoReturn() ; return 1 ; }
	  if(c<d) { chronoReturn() ; return -1 ; }
	}
    }
 if (!*b) 
   { 
     if (nbza < nbzb) { chronoReturn() ; return +1 ; } 
     if (nbza > nbzb) { chronoReturn() ; return -1 ; }
     chronoReturn() ; return 0 ;
   }
 chronoReturn() ; return -1 ;
}

/************************************************************/
/* mieg, 2003, measure the number of bytes for a vararg list */
int utPrintfSizeOfArgList (char * formatDescription, va_list args)
{
  int len = 0, lenAdd, width, stopCountingWidth, isLong;
  char *fmt, *typeOf;
  void *pVal; int iVal; double fVal;
  /* we do not care if we slightly overestimate */
#define	textSIZEOFSLASH		1	 /* should be 1 logically */
#define	textSIZEOFINT   	16	 /* should be width or strlen(sprintf(intnumber)) */
#define	textSIZEOFREAL		32	 /* see above ... */
#define	textSIZEOFADDRESS	16 /* should be (2 + sizeof(void*)*2) each byte is two hexadecimal characters */
#define textSIZEOFCHAR		1	/* 1 */
#define textSIZEOFDEFAULT	1	/* 1 */
  
  
  if (!formatDescription)
    return 0 ;
  
  for (fmt = formatDescription;*fmt;fmt++)
    {
      if(*fmt == '%')
	{ /* scan for format specification */
	  
	  /* see the width of the variable, example: %10.3lf */
	  width = 0; stopCountingWidth = 0;
	  for (typeOf = fmt + 1; *typeOf && *typeOf != '%' && !isalpha((int)(*typeOf)); typeOf++ )
	    {
	      
				/* stop counting if dot is enountered after some numbers */
	      if(*typeOf == '.')
		stopCountingWidth = 1; 
				/* accumulate the width */
	      if(isdigit((int)(*typeOf)) && (!stopCountingWidth))
		width = width*10+(*typeOf)-'0';
	    }
	  
	  if (*typeOf  ==  'l')
	    { isLong = 1; typeOf++;} /* long format */
	  else
	    isLong = 0 ;
	  switch (tolower((int)*typeOf))
	    {
	    case 'i': case 'd': case 'x':
	      if( isLong)iVal = va_arg(args, int);
	      else iVal = va_arg(args, long int );
	      lenAdd = textSIZEOFINT;
	      break;
	    case 'f': case 'g': case 'e':
	      if (isLong)fVal = va_arg(args, double );
	      else fVal = va_arg(args, double );
	      lenAdd = textSIZEOFREAL;
	      break;	
	    case 's':
	      pVal = va_arg(args, char * );
	      lenAdd = strlen(pVal);
	      break;
	    case 'p': 
	      pVal = va_arg(args, void * );
	      lenAdd = textSIZEOFADDRESS;
	      break;
	    case 'c': 
	      iVal = va_arg(args, int );
	      lenAdd = textSIZEOFCHAR;
	      break;
	    default: /* %% or alike */
	      lenAdd = textSIZEOFDEFAULT;
	      break;
	    }
	  if (width)
	    lenAdd = width;
	  fmt = typeOf;
	  len += lenAdd; 
	}
      else 
	len++;
    }
  
  return len;
} /* utPrintfSizeOfArgList */

int utPrintfSizeOf(char * formatDescription , ...)
{
  int len = 0 ;
  va_list args ;
  
  va_start (args, formatDescription) ;
  len =  utPrintfSizeOfArgList (formatDescription, args) ;
  va_end(args);
  
  return len;
}  /* utPrintfSizeOf */


/********************* eof ***********************************/
