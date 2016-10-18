/*  File: dceserver.c
 *  Author: Richard Bruskiewich (rbrusk@octogene.medgen.ubc.ca)
 *  Copyright (C) J Thierry-Mieg and R Durbin, 1992
 *-------------------------------------------------------------------
 * This file is part of the ACEDB genome database package, written by
 * 	Richard Durbin (MRC LMB, UK) rd@mrc-lmb.cam.ac.uk, and
 *	Jean Thierry-Mieg (CRBM du CNRS, France) mieg@kaa.cnrs-mop.fr
 *
 * Description:
 * Distributed Computing Environment (DCE) RPC - server
 * Adapted from Microsoft sample code and aceserver.c
 * 
 * HISTORY:
 * Last edited: Jan 10 05:00 1997 (rbrusk)
 * Created: Jan 10 05:00 1997 (rbrusk)
 * * - used Microsoft NT named pipes as stub first draft protocol
 *-------------------------------------------------------------------
 */

/* %W% %G%	 */
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include "rpcace.h"
#include "dceprot.c"
  
void main()
{
    RPC_STATUS status;
    unsigned int    cMinCalls           = 1;
    unsigned int    cMaxCalls           = 20;
    unsigned int    fDontWait           = FALSE;
 
	/*
		psz* parameters set in "dceprot.c";
		host and rpc_port values are currently ignored since
		first draft client/server assumed to reside on single workstation and
		these protocol parameters are hard coded and currently shared with client?
	*/
	status = RpcServerUseProtseqEp(pszProtocolSequence,
                                   cMaxCalls,
                                   pszEndpoint,
                                   pszSecurity); 
 
    if (status) {
        exit(status);
    }
 
    status = RpcServerRegisterIf(RPC_ACE_v1_0_s_ifspec,  
                                 NULL,   
                                 NULL); 
 
    if (status) {
        exit(status);
    }
 
    status = RpcServerListen(cMinCalls,
                             cMaxCalls,
                             fDontWait);
 
    if (status) {
        exit(status);
    }
 
 }  // end main()
 
