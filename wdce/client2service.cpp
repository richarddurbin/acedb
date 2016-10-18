/*  File: win32serverlib.cpp
 *  Author: Jean Thierry-Mieg (mieg@kaa.cnrs-mop.fr)
 *  Copyright (C) J Thierry-Mieg and R Durbin, 1992
 *-------------------------------------------------------------------
 * This file is part of the ACEDB genome database package, written by
 * 	Richard Durbin (MRC LMB, UK) rd@mrc-lmb.cam.ac.uk, and
 *	Jean Thierry-Mieg (CRBM du CNRS, France) mieg@kaa.cnrs-mop.fr
 *
 * Description: Some WIN32 service related utilities
 * 
 * Exported functions:
 * HISTORY:
 * Last edited: Mar 3 18:25 1998 (rbrusk)
 * Created: Feb 28 19:00 1998 (rbrusk):
 *-------------------------------------------------------------------
 */
#include "stdafx.h"
#include "winsvc.h"
extern "C" {
#include "regular.h"
}
#include "CError.h"

// Limit timeout to something reasonable...
// 15 minutes x 60 seconds == 750 seconds
#define MAXTIMEOUT 750

extern "C" BOOL startServer(char *szHostName, 
                            char *szDatabaseName, 
                            char *szServiceName,
			    DWORD dwTimeOut // in seconds
			    )
{

    TRACE("startServer( szHostName:\"%s\",szDatabaseName:\"%s\",szServiceName:\"%s\")\n",
                        szHostName,szDatabaseName,szServiceName) ;

    // szServiceName and szDatabaseName must be non-NULL
    // but the szHostName CAN be NULL  ==> LocalHost?

    ASSERT(szServiceName && *szServiceName) ; 
    if(!(szServiceName && *szServiceName)) return FALSE ;

    ASSERT(szDatabaseName && *szDatabaseName) ; 
    if(!(szDatabaseName && *szDatabaseName)) return FALSE ;

    SC_HANDLE   hSCMgr = NULL ;
    SC_HANDLE   hAceService = NULL ;

    try 
    {
        TRACE("Opening Handles? ") ;

        if( (hSCMgr = OpenSCManager( szHostName, "ServicesActive", 
                          GENERIC_READ )) == NULL) 
            throw "Could not open Service Control Manager handle";

        // if no service is open, attempt to open?...
        if( (hAceService = OpenService( hSCMgr, szServiceName, 
                               SERVICE_START | SERVICE_QUERY_STATUS )) == NULL )
            throw "Could not open service handle" ;

        // attempt to start the remote server
        TRACE("Starting Service? ") ;
        
	    LPCTSTR args[1] = { szDatabaseName } ;
	    DWORD ecode ;
        if( !StartService(hAceService,1,args) &&
              (ecode = GetLastError()) != ERROR_SERVICE_ALREADY_RUNNING )
            throw "Could not start the service";

	if(ecode == ERROR_SERVICE_ALREADY_RUNNING)
	{
		TRACE("Server already started!\n") ;
		return TRUE ;
	}

	if(dwTimeOut > MAXTIMEOUT) dwTimeOut = MAXTIMEOUT ;

	SERVICE_STATUS ServiceStatus ;
	BOOL FirstTime = TRUE ;
	TRACE("Service start pending\n") ;
        while(	QueryServiceStatus( hAceService, &ServiceStatus ) &&
		ServiceStatus.dwCurrentState == SERVICE_START_PENDING &&
		( FirstTime || 
		  messQuery("ACEDB remote service start still pending! Continue waiting?")
		 )
	      )
	{
		FirstTime = FALSE ; 
		if(dwTimeOut || ServiceStatus.dwWaitHint )
		{
			DWORD timeOut = 
				(dwTimeOut*1000>ServiceStatus.dwWaitHint)?
					dwTimeOut*1000:ServiceStatus.dwWaitHint ;
			Sleep(timeOut) ;
			TRACE("Timeout from sleep for service pending\n") ;
		}
        } 

        if( ServiceStatus.dwCurrentState != SERVICE_RUNNING )
        {
            SetLastError(ERROR_SERVICE_REQUEST_TIMEOUT) ;
            throw "could not get the service running";
        }
        if( hAceService != NULL && !CloseServiceHandle(hAceService) )
            throw "could not close the service handle";
        if( hSCMgr != NULL && !CloseServiceHandle(hSCMgr) )
            throw "could not close the Service Control Manager handle";
    }
    catch(char *msg)
    {
	messout(msg) ;
        TRACE1("*** startService() error: %s\n", msg) ;
        CError::ErrorMsg() ;
        return FALSE ;
    }

    TRACE("Successful Server Startup!\n") ;
    return TRUE ;
}

