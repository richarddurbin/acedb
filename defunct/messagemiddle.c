/*  File: messagemiddle.c
 *  Author: Jean Thierry-Mieg (mieg@mrc-lmba.cam.ac.uk)
 *  Copyright (C) J Thierry-Mieg and R Durbin, 1991
 *-------------------------------------------------------------------
 * This file is part of the ACEDB genome database package, written by
 *      Richard Durbin (MRC LMB, UK) rd@mrc-lmba.cam.ac.uk, and
 *      Jean Thierry-Mieg (CRBM du CNRS, France) mieg@kaa.cnrs-mop.fr
 *
 * Description:
 * Second part of the xxxlink.c file which is created automagically by *
 * The make file, here we  define MESSAGE2 to extract initialisers
   from the automatically generated portion, and provide the start of
   the declaration of funclist.   */
/*  Last edited: Sep 21 17:17 1994 (srk) */
/*-------------------------------------------------------------------
 */
/* $Id: messagemiddle.c,v 1.1 1994-09-21 17:26:50 srk Exp $ */ 

#undef MESSAGE1
#define MESSAGE2

static struct { MESSAGERETURN (*func)(va_list);
	 char *name;
       } funclist[] = {
