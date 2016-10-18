// ASInstall.cpp : Defines the class behaviors for the application.
//

#include "stdafx.h"
#include "ASInstall.h"
#include "ASInstallDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CASInstallApp

BEGIN_MESSAGE_MAP(CASInstallApp, CWinApp)
	//{{AFX_MSG_MAP(CASInstallApp)
		// NOTE - the ClassWizard will add and remove mapping macros here.
		//    DO NOT EDIT what you see in these blocks of generated code!
	//}}AFX_MSG
	ON_COMMAND(ID_HELP, CWinApp::OnHelp)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CASInstallApp construction

CASInstallApp::CASInstallApp()
{
	// TODO: add construction code here,
	// Place all significant initialization in InitInstance
}

/////////////////////////////////////////////////////////////////////////////
// The one and only CASInstallApp object

CASInstallApp theApp;

/////////////////////////////////////////////////////////////////////////////
// CASInstallApp initialization
extern "C" {
#include "regular.h"
}

#include "CError.h"
#include <cpl.h>

BOOL CASInstallApp::InitInstance()
{
	// Standard initialization
	// If you are not using these features and wish to reduce the size
	//  of your final executable, you should remove from the following
	//  the specific initialization routines you do not need.


    HINSTANCE hModule ;

    win32Init() ;

    if( (hModule = ::LoadLibrary("ASConfig.cpl")) != NULL )
    {
        NEWCPLINFO info ;
        CPLINFO oldinfo ;
        APPLET_PROC lpfnCPlApplet ;
        UINT param1 = 0 ;
        LONG Result ;

        if( !(lpfnCPlApplet =
            (APPLET_PROC)::GetProcAddress((HMODULE)hModule,"CPlApplet")) )
        {
            CError::ErrorMsg() ;
            ASSERT(FALSE) ;
        }

        Result = (*lpfnCPlApplet)(NULL,CPL_INIT,param1,0) ;
        TRACE("CPL_INIT called, returned %ld\n", Result ) ;

        Result = (*lpfnCPlApplet)(NULL,CPL_GETCOUNT,param1,0) ;
        TRACE("CPL_GETCOUNT called, returned %ld\n", Result ) ;

        Result = (*lpfnCPlApplet)(NULL,CPL_INQUIRE,param1,(LONG)&oldinfo)  ;
        TRACE("CPL_INQUIRE called, returned %ld\n", Result ) ;
        TRACE("Icon:%ld, Name:%ld,\n\tInfo:%ld\n",
               oldinfo.idIcon, oldinfo.idName, oldinfo.idInfo) ;

        Result = (*lpfnCPlApplet)(NULL,CPL_NEWINQUIRE,param1,(LONG)&info) ;
        TRACE("CPL_NEWINQUIRE called, returned %ld\n", Result ) ;
        TRACE("Icon:%p, Name:%s,\n\tInfo:%s\n", info.hIcon, info.szName, info.szInfo) ;

        Result = (*lpfnCPlApplet)(NULL,CPL_SELECT,param1,info.lData ) ;
        TRACE("CPL_SELECT called, returned %ld\n", Result ) ;
            
        Result = (*lpfnCPlApplet)(NULL,CPL_DBLCLK,param1,info.lData) ;
        TRACE("CPL_DBLCLK called, returned %ld\n", Result ) ;

        Result = (*lpfnCPlApplet)(NULL,CPL_STOP,param1,info.lData) ;
        TRACE("CPL_STOP called, returned %ld\n", Result ) ;

        Result = (*lpfnCPlApplet)(NULL,CPL_EXIT,param1,0) ;
        TRACE("CPL_EXIT called, returned %ld\n", Result ) ;

        FreeLibrary(hModule) ;
    }
    else
        AfxMessageBox( "AceServer Configuration DLL could not be loaded?",
                        MB_OK | MB_ICONERROR | MB_SETFOREGROUND );

    // Since the dialog has been closed, return FALSE so that we exit the
	//  application, rather than start the application's message pump.
	return FALSE;
}
