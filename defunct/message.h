/*  File: message.h
 *  Author: Jean Thierry-Mieg (mieg@mrc-lmba.cam.ac.uk)
 *  Copyright (C) J Thierry-Mieg and R Durbin, 1991
 *-------------------------------------------------------------------
 * This file is part of the ACEDB genome database package, written by
 *      Richard Durbin (MRC LMB, UK) rd@mrc-lmba.cam.ac.uk, and
 *      Jean Thierry-Mieg (CRBM du CNRS, France) mieg@kaa.cnrs-mop.fr
 *
 * Description:
 * Header file for message system to allow calls to non-existant functions.. */
/*  Last edited: Sep 22 13:18 1994 (srk) */
/*-------------------------------------------------------------------
 */
/* $Id: message.h,v 1.2 1994-10-03 13:56:36 srk Exp $ */ 

#include <stdarg.h>


typedef int MESSAGERETURN ;

BOOL call(char *name, ...);
BOOL callExists(char *name);
void messageInit(void);


