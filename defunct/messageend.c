/*  File: messageend.c
 *  Author: Jean Thierry-Mieg (mieg@mrc-lmba.cam.ac.uk)
 *  Copyright (C) J Thierry-Mieg and R Durbin, 1991
 *-------------------------------------------------------------------
 * This file is part of the ACEDB genome database package, written by
 *      Richard Durbin (MRC LMB, UK) rd@mrc-lmba.cam.ac.uk, and
 *      Jean Thierry-Mieg (CRBM du CNRS, France) mieg@kaa.cnrs-mop.fr
 *
 * Description:
 * third part of the xxxlink.c file which is created automagically by *
 * The make file, here we  finish up the declaration of funclist
   and provide the call routine to allow calls to functions which
   may not exist. */
/*  Last edited: Sep 22 13:18 1994 (srk) */
/*-------------------------------------------------------------------
 */
/* $Id: messageend.c,v 1.2 1994-09-23 12:38:25 srk Exp $ */ 


0, 0 
};

BOOL call(char *name,...)
{ va_list args;
  int i;

  for (i=0; funclist[i].func; i++)
    { if (!strcmp(funclist[i].name, name))
	{ va_start(args, name);
	  (*funclist[i].func)(args);
	  va_end(args);
	  return TRUE;
	}
    }
  return FALSE;
}

BOOL callExists(char *name)
{ int i;

  for (i=0; funclist[i].func; i++)
    if (!strcmp(funclist[i].name, name)) return TRUE;

  return FALSE;
}

void messageInit(void)
/* Call all message-registered functions whose names start with init */
{
  int i;
  va_list args; /* dummy, don't use */

  for (i=0; funclist[i].func; i++)
    { if (!strncmp(funclist[i].name, "init", 4))
	(*funclist[i].func)(args);
    }
}

/* End of file. */
	
