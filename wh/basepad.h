/*  File: basepad.h
 *  Author: Ed Griffiths (edgrif@sanger.ac.uk)
 *  Copyright (c) J Thierry-Mieg and R Durbin, 2000
 *-------------------------------------------------------------------
 * This file is part of the ACEDB genome database package, written by
 * 	Richard Durbin (Sanger Centre, UK) rd@sanger.ac.uk, and
 *	Jean Thierry-Mieg (CRBM du CNRS, France) mieg@kaa.crbm.cnrs-mop.fr
 *
 * Description: Header for functions to manage a padded assembly.
 *              
 * HISTORY:
 * Last edited: Feb 17 14:20 2000 (edgrif)
 * Created: Thu Feb 17 14:17:59 2000 (edgrif)
 * CVS info:   $Id: basepad.h,v 1.1 2000-02-17 14:21:13 edgrif Exp $
 *-------------------------------------------------------------------
 */
#ifndef BASEPAD_H
#define BASEPAD_H

/* There is as much documentation in this header as in w6/basepad.c  sigh... */

BOOL depad (char *name) ;

BOOL pad (char *name) ;


#endif	/* BASEPAD_H */
