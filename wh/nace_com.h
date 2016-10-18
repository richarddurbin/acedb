/*  File: nace_com.h
 *  Author: Fred Wobus (fw@sanger.ac.uk)
 *  Copyright (c) J Thierry-Mieg and R Durbin, 1999
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
 * 	Richard Durbin (Sanger Centre, UK) rd@sanger.ac.uk, and
 *	Jean Thierry-Mieg (CRBM du CNRS, France) mieg@kaa.crbm.cnrs-mop.fr
 *
 * Description: 
 * Exported functions:
 * HISTORY:
 * Last edited: Aug 26 17:34 1999 (fw)
 * Created: Thu Aug 26 17:34:20 1999 (fw)
 * CVS info:   $Id: nace_com.h,v 1.3 1999-09-01 11:15:20 fw Exp $
 *-------------------------------------------------------------------
 */

#define __malloc_h
#include <errno.h>
#include <rpc/rpc.h>
#include "nace.h"

#ifndef _NACE_COM_
#define _NACE_COM_

#define RPC_DEBUG

CLIENT  *open_ace_server (char *host , u_long prog, u_long vers );
void    close_ace_server (CLIENT *cl );
int     call_ace_server  (CLIENT *cl, net_data *data_for_srv,
			  net_data **data_for_cln, int timeout );
int     wait_for_client  (int(* call_back)(net_data *data_in, net_data *data_out)); 

/*
*******************************************************************
*******************************************************************
**                                                               **
**                            CLIENT SIDE                        **
**                                                               **
**                                                               **
*******************************************************************
*******************************************************************
**
**CLIENT  *open_ace_server (char *host , u_long prog, u_long vers );
**
**
** Client creation routine for a program "prog" and a version "vers".
** "host" identifies the name of the host where the server is.
** If successful it returns a client handle, otherwise it returns NULL.
** NOTES : "prog" and "vers" are defined in "nace.x" and for this 
**         version "prog" can be only "NET_ACE" and "vers" can be only 
**         "NET_ACE_VERS".
**         The transport protocol currently supported is only TCP 
**         ( not UDP).
*******************************************************************
*******************************************************************
**
**void close_ace_server (CLIENT *cl );
**
**
** This routine destroys the client's RPC handle, which was created
** by "open_ace_server"
*******************************************************************
*******************************************************************
**
**int call_ace_server (CLIENT *cl, net_data *data_for_srv,
**		       net_data **data_for_cln, int timeout );
**
**
** Call the remote procedure "ace_server_1" ( name of this procedure 
** is "hard coded" in this version ) associated with the client handle 
** "cl", which is created by "open_ace_server". "data_for_srv" are the
** data, which are to be sent to the server, "data_for_clnt"  is  the result.
** "timeout" is the time allowed for a response from the server.
*******************************************************************
*******************************************************************
*******************************************************************
*******************************************************************
**                                                               **
**                            SERVER SIDE                        **
**                                                               **
**                                                               **
*******************************************************************
*******************************************************************
**
**int wait_for_client ( int(* call_back)(net_data *data_in,
**                                       net_data *data_out)); 
**
**
** This routine waits for RPC requests to arrive, and calls the call back 
** procedure "call_back" with "data_in" and "data_out".
** Normaly, this routine returns only in the case of same errors.
*******************************************************************
*******************************************************************
*/

#endif





