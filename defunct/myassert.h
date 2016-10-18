/*  File: myassert.h
 *  Author: Clive Brown (cgb@sanger.ac.uk)
 *  Copyright (C) J Thierry-Mieg and R Durbin, 1995
 *-------------------------------------------------------------------
 * This file is part of the ACEDB genome database package, written by
 * 	Richard Durbin (MRC LMB, UK) rd@sanger.ac.uk, and
 *	Jean Thierry-Mieg (CRBM du CNRS, France) mieg@kaa.cnrs-mop.fr
 *
 * SCCS: $Id: myassert.h,v 1.1 1995-11-07 14:46:06 cgb Exp $
 * Description:
 * Exported functions:
 * HISTORY:
 * Last edited: Nov  7 14:46 1995 (cgb)
 * Created: Tue Nov  7 14:45:54 1995 (cgb)
 *-------------------------------------------------------------------
 */

#ifdef DEBUG

	/* NB use else clause to avoid dangling else lines being misattached */

if (f) {} else messcrash ("Assertion failed: %s, line %u", __FILE__, __LINE__)

#else

#define ASSERT(f)

#endif

	/* Clive's orginal code */

#ifdef CLIVE
static void _Assert (char * strFile, unsigned uLine)
{
 fflush (NULL);
 fprintf (stderr, "\nAssertion failed: %s, line %u\n ",strFile, uLine);
 fflush (stderr);
 abort();
}

#define ASSERT(f)  if (f) {} else _Assert (__FILE__, __LINE__)
#endif


/* assertions */







