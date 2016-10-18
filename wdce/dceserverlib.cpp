/*  File: dceserverlib.cpp
 *  Author: Richard Bruskiewich (rbrusk@octogene.medgen.ubc.ca)
 *  Copyright (C) J Thierry-Mieg and R Durbin, 1992
 *-------------------------------------------------------------------
 * This file is part of the ACEDB genome database package, written by
 * 	Richard Durbin (MRC LMB, UK) rd@mrc-lmb.cam.ac.uk, and
 *	Jean Thierry-Mieg (CRBM du CNRS, France) mieg@kaa.cnrs-mop.fr
 *
 * Description:
 * Distributed Computing Environment (DCE) RPC - server side
 * Adapted from Microsoft sample and  rpcace_sp.c code (version 1.13 7/31/96) 
 * 
 * Exported functions:
 *  void wait_for_client (u_long port, BOOL isDaemon )
 *  void stopDCEServer(void)
 *  void ace_server( handle_t IDL_handle, LP_ACE_DATA ace_data,
 *                   error_status_t	*ace_rpc_status ) 
 *
 * HISTORY:
 * Last edited: Apr 14 15:15 1998 (rbrusk): encapsulated into AcedbServerLib.dll
 * * Feb 17 00:10 1998 (rbrusk):
 *  -   converted file from .c to .cpp (w/extern "C"'s added)
 *      to permit integration of MFC code (which is C++)
 *  -   using system registry for program parameters (i.e. connection data)
 * * Feb 18 04:32 1997 (rbrusk): converted to "use all protocols"
 * * Jan 23 14:45 1997 (rbrusk)
 * * Jan 10 05:30 1997 (rbrusk)
 * Created: Jan 10 05:30 1997 (rbrusk)
 ******************************************
 * rpcace_sp.c HISTORY:
 *    from a first version by Peter Kocab
 *    edited by Bigwood & Co
 * Last edited: Jan 10 16:13 1996 (mieg)
 * Created: Fri Nov 18 16:42:20 1994 (mieg)
 *-------------------------------------------------------------------
 */

#include "stdafx.h"
extern "C" {
#include "acedb.h"
#include "graph.h"
}
#include <errno.h>
#include <ctype.h>
#include "rpcace.h"

#include "win32service.h"
#include "ServerRegistry.h"

CServerRegistry theRegistry ;

static RPC_BINDING_VECTOR *lpBindingVector = NULL ;

extern "C" ASVR_FUNC_DEF void OpenServerRegistry (char *szDatabaseName, char*szServiceName)
{
    if(szDatabaseName && *szDatabaseName)
        theRegistry.SetDatabaseName(szDatabaseName) ;

    if(szServiceName && *szServiceName)
        theRegistry.SetServiceName(szServiceName) ;

    theRegistry.SetAdminMode() ;  // Admin mode should be set?

    SetAcedbRegistry((void*)&theRegistry) ;

    if( !theRegistry.OpenDatabaseKeys() )
    {
        CError::ErrorMsg("AceServer::OpenServerRegistry() could not open the registry?", REGISTRY_OPEN_ERROR) ;
        return ; 
    }

    CString dbPath ;
    if( theRegistry.GetDatabaseProperty( "ACEDB", dbPath ))
    {
		dbPath = dbPath.SpanExcluding(";") ;
		dbPath += "\\server.log" ;
		OpenErrorLog(dbPath) ;
    }

    if( !theRegistry.CloseAllKeys() )
        CError::ErrorMsg("AceServer::OpenServerRegistry() could not close the registry?", REGISTRY_CLOSE_ERROR) ;
}

extern "C" ASVR_FUNC_DEF BOOL GetServerParameters(int  *clientTimeOut, int  *serverTimeOut, int  *maxKbytes, int  *autoSaveInterval)
{
    UINT value ;
    ASSERT( clientTimeOut && serverTimeOut && maxKbytes && autoSaveInterval) ;

    if( !(clientTimeOut && serverTimeOut && maxKbytes && autoSaveInterval) )
        return FALSE ;
    
    if( !theRegistry.OpenServerKeys() )
    {
        CError::ErrorMsg("AceServer::GetServerParameters() could not open the registry?", REGISTRY_OPEN_ERROR) ;
        return FALSE ; // Fatal error: could not open the registry?
    }

    if( !theRegistry.GetServerProperty( CLIENT_TIMEOUT, value ))
    {
        value = 600 ;  // 10 minutes
        if(  !theRegistry.SetServerProperty( CLIENT_TIMEOUT, value ) )
        {
            CError::ErrorMsg("AceServer::GetServerParameters() could not set clientTimeOut?", GET_PROPERTY_ERROR) ;
            return FALSE ;
        }
    }
    *clientTimeOut = (int)value ;  

    if( !theRegistry.GetServerProperty( SERVER_TIMEOUT, value ))
    {
        value = 600 ;  // 10 minutes
        if(  !theRegistry.SetServerProperty( SERVER_TIMEOUT, value )  )
        {
            CError::ErrorMsg("AceServer::GetServerParameters() could not set serverTimeOut?", GET_PROPERTY_ERROR) ;
            return FALSE ;
        }
    }
    *serverTimeOut = (int)value ;  

    if( !theRegistry.GetServerProperty(MAX_KILOBYTES, value ))
    {
        value = 0 ; // No limit
        if(  !theRegistry.SetServerProperty( MAX_KILOBYTES, value) )
        {
            CError::ErrorMsg("AceServer::GetServerParameters() could not set maxKbytes?", GET_PROPERTY_ERROR) ;
            return FALSE ;
        }
    }
    *maxKbytes = (int)value ;  

    if( !theRegistry.GetServerProperty( AUTOSAVE_INTERVAL, value ))
    {
        value = 600 ; // 10 minutes
        if(  !theRegistry.SetServerProperty( AUTOSAVE_INTERVAL, value) )
        {
            CError::ErrorMsg("AceServer::GetServerParameters() could not set autoSaveInterval?", GET_PROPERTY_ERROR) ;
            return FALSE ;
        }
    }
    *autoSaveInterval = (int)value ;  

    if( !theRegistry.CloseAllKeys() )
    {
        CError::ErrorMsg("AceServer::GetServerParameters() could not close the registry?", REGISTRY_CLOSE_ERROR) ;
        return FALSE ;
    }
    else
        return TRUE ;
}

extern "C" ASVR_FUNC_DEF void stopDCEServer(void)
{
    RPC_STATUS status = RPC_S_OK ;
  
    REPORT_STOP_STATUS(RPC_S_OK)

    status = RpcMgmtStopServerListening(NULL);
 
    if (status != RPC_S_OK) 
    {
        CError::ErrorMsg("AceServer::stopDCEServer() error upon stop server listening?", status) ;
        REPORT_STOP_STATUS(status) // will this work?
    }
 
    REPORT_STOP_STATUS(RPC_S_OK)

    status = RpcServerUnregisterIf( RPC_ACE_v1_2_s_ifspec,
				    NULL, FALSE);
    if (status != RPC_S_OK)
    {
        CError::ErrorMsg("AceServer::stopDCEServer() error upon server interface deregistration?", status) ;
        REPORT_STOP_STATUS(status) // will this work?
    }

REPORT_STOPPED

cleanup:
    exit(status) ;

} //end stopDCEServer()

extern "C" ASVR_FUNC_DEF void wait_for_client ()
{
    int nServers = 0 ;
    RPC_STATUS status = RPC_S_OK ; 
    unsigned int    cMinCalls           = 1;
    unsigned int    cMaxCalls           = 20;
    unsigned int    fDontWait           = FALSE ; // implies a Wait Server Listen
    int i ;
    CConnection connection ;

    CError::ErrorMsg("Entering AceServer::wait_for_client():") ;

    REPORT_START_STATUS(RPC_S_OK)

    if( !theRegistry.Open() ) // Local host
    {
        CError::ErrorMsg("AceServer::wait_for_client() could not open system registry to retrieve server information", 
						  REGISTRY_OPEN_ERROR ) ;
        return ;
    }

    REPORT_START_STATUS(RPC_S_OK)

    if( !theRegistry.OpenServerKeys() )
    {
        CError::ErrorMsg("AceServer::wait_for_client() could not open system registry key for server information", 
						  KEY_OPEN_ERROR) ;
        theRegistry.Close() ;
        return ;
    }

    REPORT_START_STATUS(RPC_S_OK)

    if( theRegistry.GetConnection(CConnection::NAMEDPIPE, connection) &&
        connection.IsEnabled())
    {
        // Named pipe protocol
        status = RpcServerUseProtseqEp((unsigned char*)(LPCTSTR)connection.Protocol(),
                                       cMaxCalls,
                                       (unsigned char*)(LPCTSTR)connection.Endpoint(),
                                       NULL); 
        if (status != RPC_S_OK) 
        {
            TRACE1("Could not register Named Pipe connection:\"%s\"\n",connection.Endpoint() ) ;
            CError::ErrorMsg("AceServer::wait_for_client() error while registering Named Pipe connection\n", status ) ;
        }
        else
        {
            CError::ErrorMsg("Named pipe connection registered!") ;
            ++nServers ;
        }
    }

    REPORT_START_STATUS(RPC_S_OK)

    if( theRegistry.GetConnection(CConnection::TCPIP, connection) &&
        connection.IsEnabled())
    {
        // TCP/IP protocol
        status = RpcServerUseProtseqEp((unsigned char*)(LPCTSTR)connection.Protocol(),
                                       cMaxCalls,
                                       (unsigned char*)(LPCTSTR)connection.Endpoint(),
                                       NULL); 
        if (status != RPC_S_OK) 
        {
                TRACE1("Could not register TCP/IP port:\"%s\"\n", connection.Endpoint() ) ;
                CError::ErrorMsg("AceServer::wait_for_client() error while registering TCP/IP connection\n", status ) ;
        }
        else
        {
            CError::ErrorMsg("TCP/IP connection registered!") ;
            ++nServers ;
        }
    }

    REPORT_START_STATUS(RPC_S_OK)

    if( theRegistry.GetConnection(CConnection::NETBEUI, connection) &&
        connection.IsEnabled() )
    {
        // NetBEUI protocol
        status = RpcServerUseProtseqEp((unsigned char*)(LPCTSTR)connection.Protocol(),
                                       cMaxCalls,
                                       (unsigned char*)(LPCTSTR)connection.Endpoint(),
                                       NULL); 
        if (status != RPC_S_OK) 
        {
                TRACE1("Could not register NetBEUI port:\"%s\"\n", connection.Endpoint()) ;
                CError::ErrorMsg("AceServer::wait_for_client() error while registering NetBEUI connection\n", status ) ;
        }
        else
        {
            CError::ErrorMsg("NetBEUI connection registered!") ;
            ++nServers ;
        }
    }

    REPORT_START_STATUS(RPC_S_OK)

    if( theRegistry.GetConnection(CConnection::CUSTOM, connection) &&
        connection.IsEnabled() )
    {
        // Custom protocol
        status = RpcServerUseProtseqEp((unsigned char*)(LPCTSTR)connection.Protocol(),
                                       cMaxCalls,
                                       (unsigned char*)(LPCTSTR)connection.Endpoint(),
                                       NULL); 
 
        if (status != RPC_S_OK) 
        {
            TRACE1("Could not register Custom connection endpoint:\"%s\"\n", connection.Endpoint() ) ;
            CError::ErrorMsg("AceServer::wait_for_client() error while registering custom connection\n", status ) ;
        }
        else
        {
            CError::ErrorMsg("Custom connection registered!") ;
            ++nServers ;
        }
    }

    if( !theRegistry.CloseAllKeys() )
    {
        CError::ErrorMsg("AceServer::wait_for_client() could not close the registry?", REGISTRY_CLOSE_ERROR) ;
        AceASSERT(FALSE) ;
        exit(REGISTRY_CLOSE_ERROR);
    }

    if(!nServers)
    {
        CError::ErrorMsg("AceServer::wait_for_client() could not register any protocol connections?\n" ) ;
        AceASSERT(FALSE) ;
        REPORT_START_STATUS(ERROR_PROTOCOL_UNREACHABLE)
    }

    REPORT_START_STATUS(RPC_S_OK)

    status = RpcServerRegisterIf( RPC_ACE_v1_2_s_ifspec, NULL, NULL ) ;
	
    if (status != RPC_S_OK) 
    {
        CError::ErrorMsg("AceServer::wait_for_client() could not register server interface?", status) ;
        REPORT_START_STATUS(status) // will this work?
        exit(status);
    }

    CError::ErrorMsg("AceServer Initialization Complete; Server is now listening...") ;

    REPORT_RUNNING

    try
    {
        status = RpcServerListen(cMinCalls,
                                 cMaxCalls,
                                 fDontWait);
    }
    catch(CException *e)
    {
        e->Delete() ;
        stopDCEServer() ;
    }

    if (status != RPC_S_OK) 
    {
        CError::ErrorMsg("AceServer::wait_for_client() error upon RpcServerListen() return", status) ;
        REPORT_STOP_STATUS(status) // will this work?
    }

cleanup:
    exit(status) ;
}
	

/*
** Remote version for DCE "ace_server"
*/
// This typedef should be in win32server.h but was generating a compiler error there, so...
typedef Stack (*LPPROCQUERYFN)(int *, int *, char *, int , int *) ;
static LPPROCQUERYFN _ProcessQueries ;

extern "C" ASVR_FUNC_DEF void dceServerInit( void *lpfnProcessQueries )
{
	_ProcessQueries = (LPPROCQUERYFN)lpfnProcessQueries ;
}

extern "C" void ace_server( handle_t IDL_handle, LP_ACE_DATA ace_data,
                 error_status_t	*ace_rpc_status ) 
{
	int nn ;
	Stack s ;
	int clientId = ace_data->clientId ;
	int magic = ace_data->magic ;
	char *cp ;
	int maxBytes = (int)(ace_data->kBytes * 1024);
	int encore = 0 ;
	RPC_STATUS status ;

	*ace_rpc_status = 0 /* error_status_ok/ESUCCESS (?) */ ;

	/* No previous result to free in DCE/RPC (unlike rpcgen xdr_* data) */
        /* Assume success at start */

        encore = - ace_data->encore ;  /* sign is inverted to allow overloading */

	s = (*_ProcessQueries)(&clientId, &magic, (char *)ace_data->question, maxBytes, &encore) ; 

	nn = s && stackMark(s) ? stackMark(s) : 100 ;
	/* stackMrk == 0 happens if we write to a file and are in the encore case */

	cp = (char *)RpcSmAllocate( sizeof(unsigned char)*(nn + 1), &status ) ;	
	if (status == RPC_S_OK && cp)
	{
		memset(cp, 0, nn+1) ;
		if (s &&  stackMark(s))
			memcpy (cp, stackText(s,0), nn) ;
		else 
		{ 
                    if (!s)
                        strcpy(cp,"// Sorry, broken connection, possibly due to client time out") ;
                    nn = strlen (cp) ;
		}
	}
	else
	{
		cp = (char *)RpcSmAllocate( sizeof(unsigned char)*300, &status ) ;	
		if (status == RPC_S_OK)
		{
                    sprintf(cp, "%s%d%s\n%s\n",
                                "//! Sorry, the answer is too long (",stackMark (s)," kilobytes),",
                                "//! I can\'t mallocate a sufficient buffer." ) ;
		}
		else
		{
			cp = NULL ; /* a real memory allocation problem!!! */
		}
                *ace_rpc_status = ENOMEM ; /* signal an "out of memory" error? */ ;
                nn = strlen (cp) ;
	}

	ace_data->reponse = (unsigned char *)cp ;
	ace_data->clientId = clientId ;
	ace_data->magic = magic ;
	ace_data->encore = encore; /* let client know if you have more */
}

/********* end of file ********/

