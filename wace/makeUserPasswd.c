/*  File: makeUserPasswd.c
 *  Author: Ed Griffiths (edgrif@sanger.ac.uk)
 *  Copyright (c) J Thierry-Mieg and R Durbin, 1999
 *-------------------------------------------------------------------
 * This file is part of the ACEDB genome database package, written by
 * 	Richard Durbin (Sanger Centre, UK) rd@sanger.ac.uk, and
 *	Jean Thierry-Mieg (CRBM du CNRS, France) mieg@kaa.crbm.cnrs-mop.fr
 *
 * Description: Program to make an entry as it should appear in the
 *              serverpasswd.wrm file.
 *              Note that the output can be parsed to get the valid
 *              entry, it will be the only non-blank line not beginning
 *              with "//"
 *              
 * Exported functions: none
 * HISTORY:
 * Last edited: Feb  3 10:09 2000 (edgrif)
 * Created: Wed Nov 17 14:07:06 1999 (edgrif)
 * CVS info:   $Id: makeUserPasswd.c,v 1.2 2000-02-03 10:14:37 edgrif Exp $
 *-------------------------------------------------------------------
 */
#include <wh/mystdlib.h>
#include <termios.h>					    /* for non-echoing terminal code. */
#include <readline/readline.h>				    /* for getting user input. */

#include <wh/regular.h>
#include <wmd5/global.h>
#include <wmd5/md5.h>

enum {MD5_HASHLEN = 16, MD5_HEX_HASHLEN = ((MD5_HASHLEN * 2) + 1)} ;

void usage(char *progname) ;
char *getPasswd(void) ;
char *convertMD5toHexStr(unsigned char digest[]) ;


/* This program takes as input the userid, the program then prompts for the  */
/* password, does some simple password checking and outputs to stdout a      */
/* string in the form:                                                       */
/*                       userid: hash_string                                 */
/*                                                                           */
/* where hash_string is the MD5 hash of the userid and the pasword.          */
/*                                                                           */
int main(int argc, char **argv)
{
  MD5_CTX Md5Ctx ;
  unsigned char digest[MD5_HASHLEN] ;
  char *hash ;

  if (argc != 2)
    usage(argv[0]) ;
  else
    {
      char *passwd ;

      /* Get passwd from user, call either succeeds or exits.                */
      passwd = getPasswd() ;

      /* Do the hash                                                         */
      MD5Init(&Md5Ctx) ;
      MD5Update(&Md5Ctx, (unsigned char *)argv[1], strlen(argv[1])) ;
      MD5Update(&Md5Ctx, (unsigned char *)passwd, strlen(passwd)) ;
      MD5Final(digest, &Md5Ctx) ;

      /* Change the hash number into a hex string version.                   */
      hash = convertMD5toHexStr(&digest[0]) ;

      messout("The following line is a valid entry for wspec/serverpasswd.wrm\n") ;

      printf("%s %s\n\n", argv[1], hash) ;
    }

  return 0 ;
}



void usage(char *progname)
{
  printf("Run the program like this:\n"
	 "\n"
	 "       %s userid\n"
	 "\n"
	 "the program then produces on stdout\n"
	 "\n"
	 "       userid <MD5 hash of userid/password>\n"
	 "\n",
	 progname) ;

  return ;
}


/* Both this and the following routine should really be common to this       */
/* program and the server password stuff....they need to be incorporated...  */
char *getPasswd(void)
{
  struct termios term ;
  char *passwd = NULL ;
  char *passwd1, *passwd2 ;
  
  /* Get the terminal attributes to manipulate echo'ing.                     */
  if (tcgetattr(fileno(stdin), &term) < 0)
    messcrash("Unable to get terminal attributes for input of password.");
  
  /* echo off...                                                             */
  term.c_lflag &= ~(ECHO) ;
  if (tcsetattr(fileno(stdin), TCSADRAIN, &term) < 0)
    messcrash("Unable to set terminal attributes for input of password.");


  /* Get the passwd (twice). Note that newline after password is not echoed  */
  /* to terminal so we have to send one.                                     */
  passwd1 = readline("// Please enter passwd: ") ;
  printf("\n") ;

  passwd2 = readline("// Please re-enter passwd: ") ;
  printf("\n") ;


  /* echo on...                                                              */
  term.c_lflag |= ECHO ;
  if (tcsetattr(fileno(stdin), TCSADRAIN, &term) < 0)
    messcrash("Unable to reset terminal attributes after input of password.");


  /* do some basic checking on the password.                                 */
  if (passwd1 == NULL					    /* can readline return NULL ? */
      || (strcmp(passwd1, "") == 0)			    /* can't be empty string */
      || (strlen(passwd1) < 6)				    /* must be at least 6 chars */
      || (strstr(passwd1, " ") != NULL)			    /* no blanks */
      || (strtok(passwd1, "abcdefghijklmnopqrstuvwxyz" "ABCDEFGHIJKLMNOPQRSTUVWXYZ" "0123456789")
	  != NULL))					    /* alphanumerics only. */
    messExit("Password invalid, must be at least six chars with no blanks,\n"
	     "only alphanumerics are allowed [a-z, A-Z, 0-9].") ;

  if (strcmp(passwd1, passwd2) != 0)			    /* must be reentered correctly. */
     messExit("Password incorrectly re-entered.") ;

  passwd = passwd1 ;

  return passwd ;
}



/* This should be in a separate library really as it also exists in          */
/* socketace.c                                                               */
char *convertMD5toHexStr(unsigned char digest[])
{
  char *digest_str ;
  int i ;
  char *hex_ptr ;

  digest_str = (char *)malloc(MD5_HEX_HASHLEN) ;
  for (i = 0, hex_ptr = digest_str ; i < MD5_HASHLEN ; i++, hex_ptr+=2)
    {
      sprintf(hex_ptr, "%02x", digest[i]) ;
    }

  return digest_str ;
}
