/*  File: dceprot.c
 *  Author: Richard Bruskiewich (rbrusk@octogene.medgen.ubc.ca)
 *  Copyright (C) R. Bruskiewich, J Thierry-Mieg and R Durbin, 1997
 *-------------------------------------------------------------------
 * This file is part of the ACEDB genome database package, written by
 * 	Richard Durbin (MRC LMB, UK) rd@mrc-lmb.cam.ac.uk, and
 *	Jean Thierry-Mieg (CRBM du CNRS, France) mieg@kaa.cnrs-mop.fr
 *
 * Description:
 * WIN32 Distributed Computing Environment (DCE) RPC - client/server shared file
 * This module declares static values for DCE communication protocol parameters
 * which will likely become dynamically specified once the system is debugged.
 * 
 * HISTORY:
 * Last edited: Feb 18 05:24 1997 (rbrusk): AceServerEntryName; generalizing interface
 * * Jan 10 05:15 1997 (rbrusk)
 * Created: Jan 10 05:15 1997 (rbrusk): version 1 is a named pipe?
 *-------------------------------------------------------------------
 */

/* %W% %G%	 */

static unsigned char * pszUuid			= NULL;
static unsigned char * pszProtocolSequence	= "ncacn_np";
static unsigned char * pszSecurity		= NULL; /*Security not implemented */
static unsigned char * pszNetworkAddress	= NULL;
static unsigned char * pszEndpoint		= "\\pipe\\AceServer";
static unsigned char * pszOptions		= NULL;

static unsigned char * pszDefaultEntryName = "/.:/AceServer" ;

 
