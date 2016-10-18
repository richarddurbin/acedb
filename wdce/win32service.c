/*-------------------------------------------------------------------
 * File: win32service.c
 *  Adapted by: Richard Bruskiewich (rbrusk@octogene.medgen.ubc.ca)
 *  from code by Craig Link - Microsoft Developer Support
 *  Copyright (C) R. Bruskiewich, J Thierry-Mieg and R Durbin, 1998
 *-------------------------------------------------------------------
//
// Copyright (C) 1993-1995  Microsoft Corporation.  All Rights Reserved.
//
// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE.
//
//
//  MODULE:   service.c
//
//  PURPOSE:  Implements functions required by all services
//            windows.
//
//  FUNCTIONS:
//    win32serviceInit(int argc, char **argv); (actually, main() in disguise...)
//    service_ctrl(DWORD dwCtrlCode);
//    service_main(DWORD dwArgc, LPTSTR *lpszArgv);
//    CmdInstallService(); ************* NOT USED HERE, deleted
//    CmdRemoveService();  ************* NOT USED HERE, deleted
//    CmdDebugService(int argc, char **argv);
//    ControlHandler ( DWORD dwCtrlType );
//    GetLastErrorText( LPTSTR lpszBuf, DWORD dwSize );
//
 *-------------------------------------------------------------------
 * This file is part of the ACEDB genome database package, written by
 * 	Richard Durbin (MRC LMB, UK) rd@mrc-lmb.cam.ac.uk, and
 *	Jean Thierry-Mieg (CRBM du CNRS, France) mieg@kaa.cnrs-mop.fr
 *
 * Description:  WIN32 "Service" startup and management code
 *               Adapted from WIN32 SDK - Windows NT Samples: "SERVICE"
 *               Original code copyright by Craig Link, Microsoft (see above)
 *
 * HISTORY:
 * Last edited: Mar 1 12:45 1998 (rbrusk)
 * Created: Mar 1 12:45 1998 (rbrusk)
 *-------------------------------------------------------------------
 */

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <process.h>
#include <tchar.h>

#include "regular.h"
#include "WinAceError.h"
#include "win32service.h"

// internal variables
SERVICE_STATUS          ssStatus;       // current status of the service
SERVICE_STATUS_HANDLE   sshStatusHandle;

// internal function prototypes
VOID WINAPI service_ctrl(DWORD dwCtrlCode);
VOID WINAPI service_main(DWORD dwArgc, LPTSTR *lpszArgv);
VOID CmdDebugService(int argc, char **argv);
BOOL WINAPI ControlHandler ( DWORD dwCtrlType );

//
//  FUNCTION: win32serviceInit() (actually, main() in disguise...)
//
//  PURPOSE: entrypoint for service
//
//  PARAMETERS:
//    argc - number of command line arguments
//    argv - array of command line arguments
//
//  RETURN VALUE:
//    none
//
//  COMMENTS:
//    main() either performs the command line task, or
//    call StartServiceCtrlDispatcher to register the
//    main service thread.  When the this call returns,
//    the service has stopped, so exit.
//
//  argv[0] should have the name of the service being invoked? 
static LPSERVICESTART _ServiceStart = (void *)0 ; 
static LPSERVICESTOP  _ServiceStop  = (void *)0 ;
 
ASVR_FUNC_DEF void win32serviceInit(int argc, char **argv, 
									LPSERVICESTART lpfnServiceStart, 
									LPSERVICESTOP  lpfnServiceStop) 
{

    SERVICE_TABLE_ENTRY dispatchTable[] =
    {
        { TEXT(argv[0]), (LPSERVICE_MAIN_FUNCTION)service_main },
        { NULL, NULL }
    };

	_ServiceStart = lpfnServiceStart ;
	_ServiceStop  = lpfnServiceStop ;

    if ( (argc > 1) &&
         ((*argv[1] == '-') || (*argv[1] == '/')) )
    {
        if ( _stricmp( "debug", argv[1]+1 ) == 0 )
        {
            // Not running as service
            bServiceDebug = TRUE;

            // pseudo-Service name for commandline debugging
            argv[1] = "AceServer" ; 

            // argv array offset to argv[1] => argv[0] 
            CmdDebugService(argc-1, &argv[1]);
        }
        else  // Running as a service...
        {
		bAceService = TRUE ;
		goto dispatch;
        }
        exit(0);
    }

    // if it doesn't match any of the above parameters
    // the service control manager may be starting the service
    // so we must call StartServiceCtrlDispatcher
    dispatch:
        // this is just to be friendly
        printf( "Type %s -debug [-verbose] <database> to run the AceServer as a console app for debugging\n", argv[0] );
        printf( "(Use the -verbose switch to turn on verbose message reporting)\n" );
        printf( "\nService Mode being run: StartServiceCtrlDispatcher being called.\n" );
        printf( "This may take several seconds.  Please wait.\n" );

        if (!StartServiceCtrlDispatcher(dispatchTable))
            AddToMessageLog(TEXT("StartServiceCtrlDispatcher failed."));
}



//
//  FUNCTION: service_main
//
//  PURPOSE: To perform actual initialization of the service
//
//  PARAMETERS:
//    dwArgc   - number of command line arguments
//    lpszArgv - array of command line arguments
//
//  RETURN VALUE:
//    none
//
//  COMMENTS:
//    This routine performs the service initialization and then calls
//    the user defined ServiceStart() routine to perform majority
//    of the work.
//
void WINAPI service_main(DWORD dwArgc, LPTSTR *lpszArgv)
{

    // register our service control handler:
    //
    sshStatusHandle = RegisterServiceCtrlHandler( TEXT(lpszArgv[0]), service_ctrl);

    if (!sshStatusHandle)
        goto cleanup;

    ssStatus.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
    ssStatus.dwServiceSpecificExitCode = 0;

    REPORT_START_STATUS(NO_ERROR)

    (*_ServiceStart)( (int)dwArgc, (char **)lpszArgv ) ;

cleanup:

    // try to report the stopped status to the service control manager.
    //
    if (sshStatusHandle)
        (VOID)ReportStatusToSCMgr(
                            SERVICE_STOPPED,
                            WinAce_ExitCode,
                            0);
    return;
}



//
//  FUNCTION: service_ctrl
//
//  PURPOSE: This function is called by the SCM whenever
//           ControlService() is called on this service.
//
//  PARAMETERS:
//    dwCtrlCode - type of control requested
//
//  RETURN VALUE:
//    none
//
//  COMMENTS:
//
VOID WINAPI service_ctrl(DWORD dwCtrlCode)
{
    // Handle the requested control code.
    //
    switch(dwCtrlCode)
    {
        // Stop the service.
        //
        case SERVICE_CONTROL_STOP:
            ssStatus.dwCurrentState = SERVICE_STOP_PENDING;
            (*_ServiceStop)();
            break;

        // Update the service status.
        //
        case SERVICE_CONTROL_INTERROGATE:
            break;

        // invalid control code
        //
        default:
            break;

    }
    ReportStatusToSCMgr(ssStatus.dwCurrentState, NO_ERROR, 0);
}

//
//  FUNCTION: ReportStatusToSCMgr()
//
//  PURPOSE: Sets the current status of the service and
//           reports it to the Service Control Manager
//
//  PARAMETERS:
//    dwCurrentState - the state of the service
//    dwWin32ExitCode - error code to report
//    dwWaitHint - worst case estimate to next checkpoint
//
//  RETURN VALUE:
//    TRUE  - success
//    FALSE - failure
//
//  COMMENTS:
//
ASVR_FUNC_DEF BOOL ReportStatusToSCMgr(DWORD dwCurrentState,
                         DWORD dwWin32ExitCode,
                         DWORD dwWaitHint)
{
    static DWORD dwCheckPoint = 1;
    BOOL fResult = TRUE;

    if ( !bServiceDebug ) // when debugging we don't report to the SCM
    {
        if (dwCurrentState == SERVICE_START_PENDING)
            ssStatus.dwControlsAccepted = 0;
        else
            ssStatus.dwControlsAccepted = SERVICE_ACCEPT_STOP;

        ssStatus.dwCurrentState = dwCurrentState;
        ssStatus.dwWin32ExitCode = dwWin32ExitCode;
        ssStatus.dwWaitHint = dwWaitHint;

        if ( ( dwCurrentState == SERVICE_RUNNING ) ||
             ( dwCurrentState == SERVICE_STOPPED ) )
            ssStatus.dwCheckPoint = 0;
        else
            ssStatus.dwCheckPoint = dwCheckPoint++;


        // Report the status of the service to the service control manager.
        //
        if (!(fResult = SetServiceStatus( sshStatusHandle, &ssStatus))) {
            AddToMessageLog(TEXT("SetServiceStatus"));
        }
    }
    return fResult;
}

///////////////////////////////////////////////////////////////////
//
//  The following code is for running the service as a console app
//

//
//  FUNCTION: CmdDebugService(int argc, char ** argv)
//
//  PURPOSE: Runs the service as a console application
//
//  PARAMETERS:
//    argc - number of command line arguments
//    argv - array of command line arguments
//
//  RETURN VALUE:
//    none
//
//  COMMENTS:
//
void CmdDebugService(int argc, char ** argv)
{
    DWORD dwArgc;
    LPTSTR *lpszArgv;

#ifdef UNICODE
    lpszArgv = CommandLineToArgvW(GetCommandLineW(), &(dwArgc) );
#else
    dwArgc   = (DWORD) argc;
    lpszArgv = argv;
#endif

    _tprintf(TEXT("Debugging %s.\n"), TEXT(argv[0]));

    SetConsoleCtrlHandler( ControlHandler, TRUE );

    (*_ServiceStart)( dwArgc, lpszArgv );
}


//
//  FUNCTION: ControlHandler ( DWORD dwCtrlType )
//
//  PURPOSE: Handled console control events
//
//  PARAMETERS:
//    dwCtrlType - type of control event
//
//  RETURN VALUE:
//    True - handled
//    False - unhandled
//
//  COMMENTS:
//
BOOL WINAPI ControlHandler ( DWORD dwCtrlType )
{
    switch( dwCtrlType )
    {
        case CTRL_BREAK_EVENT:  // use Ctrl+C or Ctrl+Break to simulate
        case CTRL_C_EVENT:      // SERVICE_CONTROL_STOP in debug mode
            _tprintf(TEXT("Stopping AceServer.\n"));
            (*_ServiceStop)();
            return TRUE;
            break;

    }
    return FALSE;
}

