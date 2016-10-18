/*  File: AceServerControlPanel.cpp
 *  Author: Richard Bruskiewich (rbrusk@octogene.medgen.ubc.ca)
 *  Copyright (C) R. Bruskiewich, J Thierry-Mieg and R Durbin, 1998
 *-------------------------------------------------------------------
 * This file is part of the ACEDB genome database package, written by
 * 	Richard Durbin (MRC LMB, UK) rd@mrc-lmb.cam.ac.uk, and
 *	Jean Thierry-Mieg (CRBM du CNRS, France) mieg@kaa.cnrs-mop.fr
 *
 * Description:      Microsoft Windows DCE AceServer Configuration   
 *                   Control Panel Application program
 *
 * Exported functions: CPlApplet()  ; the standard CPL callback
 * HISTORY:
 * Last edited: Jan 4 16:02 1998 (rbrusk)
 * Created: Jan 4 16:02 1998 (rbrusk)
 *-------------------------------------------------------------------
 */

#include "stdafx.h"
#include <cpl.h>

extern "C" {
#include "regular.h"
}

#include "AceServerControlPanel.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CAceServerControlPanelApp

BEGIN_MESSAGE_MAP(CAceServerControlPanelApp, CWinApp)
	//{{AFX_MSG_MAP(CAceServerControlPanelApp)
		// NOTE - the ClassWizard will add and remove mapping macros here.
		//    DO NOT EDIT what you see in these blocks of generated code!
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CAceServerControlPanelApp construction

CAceServerControlPanelApp::CAceServerControlPanelApp()
{
    m_ConfigWizard = NULL ;
}

/////////////////////////////////////////////////////////////////////////////
// The one and only CAceServerControlPanelApp object

CAceServerControlPanelApp theApp;

/////////////////////////////////////////////////////////////////////////////
// CPlApplet Callback

extern "C" LONG APIENTRY CPlApplet(HWND hwndCPL, UINT uMsg, LPARAM lParam1, LPARAM lParam2) 
{ 
    /* int i = (int) lParam1 ;  -- only needed for multiple applets... */
    HINSTANCE hinst = NULL ;
    LPCPLINFO lpOldCPlInfo ;
    LPNEWCPLINFO lpNewCPlInfo; 
 
    AFX_MANAGE_STATE(AfxGetStaticModuleState())

    switch (uMsg) { 
        case CPL_INIT:      // first message, sent once 
            TRACE("CAceServerControlPanel::CPlApplet(CPL_INIT) called\n") ;
            
            if( strcmp(getenv("OS"),"Windows_NT") )
            {
                AfxMessageBox("Sorry! WinAceServer only runs under Windows NT.") ;
                return 0 ;
            }

            if( !theApp.InitDialog() )
            {
                ASSERT(FALSE) ;
                return 0 ; // Failure!
            }
            return 1 ; // success!
 
        case CPL_GETCOUNT:  // second message, sent once 
            TRACE("CAceServerControlPanel::CPlApplet(CPL_GETCOUNT) called\n") ;
            return 1; // one dialog only...
 
        case CPL_INQUIRE: // third (old) message, sent once per app 
            TRACE("CAceServerControlPanel::CPlApplet(CPL_INQUIRE) called\n") ;
            VERIFY(lpOldCPlInfo = (LPCPLINFO)lParam2) ;
            lpOldCPlInfo->idIcon = IDR_WINACESERVER_CPL ;     
            lpOldCPlInfo->idName = IDS_CONFIG_DIALOG_TITLE ;
            lpOldCPlInfo->idInfo = IDS_CONFIG_DIALOG_DESCRIPTION ;
            return 0 ;

        case CPL_NEWINQUIRE: // third message, sent once per app 
            TRACE("CAceServerControlPanel::CPlApplet(CPL_NEWINQUIRE) called\n") ;

            if( (hinst = (HINSTANCE)
                        GetModuleHandle("ASConfig.cpl") ) == NULL
              )
            {
                ASSERT(FALSE) ;
                return 0; // Failure!?
            }

            TRACE("Using Module Handle == %p\n", hinst) ;

            VERIFY(lpNewCPlInfo = (LPNEWCPLINFO) lParam2); 
            lpNewCPlInfo->dwSize = (DWORD) sizeof(NEWCPLINFO); 
            lpNewCPlInfo->dwFlags = 0; 
            lpNewCPlInfo->dwHelpContext = 0; 
            lpNewCPlInfo->szHelpFile[0] = '\0'; 
            lpNewCPlInfo->lData = 0; 
            
            lpNewCPlInfo->hIcon = LoadIcon(hinst, 
                (LPCTSTR) MAKEINTRESOURCE(IDR_WINACESERVER_CPL));
            VERIFY(lpNewCPlInfo->hIcon != NULL) ;
            if(lpNewCPlInfo->hIcon == NULL) return 1 ; // Failure!
            TRACE("Icon Handle == %p\n", lpNewCPlInfo->hIcon) ;

            if( !LoadString( hinst, IDS_CONFIG_DIALOG_TITLE, 
                             lpNewCPlInfo->szName, 32) )
                return 0 ; // Failure!?
            TRACE("CP Applet Name: %s\n",lpNewCPlInfo->szName) ;
 
            if( !LoadString( hinst, IDS_CONFIG_DIALOG_DESCRIPTION, 
                             lpNewCPlInfo->szInfo, 64) )
                return 0 ; // Failure!?
            TRACE("CP Applet Info: %s\n",lpNewCPlInfo->szInfo) ;
 
            return 1 ; 
 
        case CPL_SELECT:    // applet icon selected 
            TRACE("CAceServerControlPanel::CPlApplet(CPL_SELECT) called\n") ;
            return 0 ; // Success!
 
        case CPL_DBLCLK:    // applet icon double-clicked 

            TRACE("CAceServerControlPanel::CPlApplet(CPL_DBLCLK) called\n") ;
            MessageBeep(MB_ICONEXCLAMATION); 
            if( !theApp.DisplayDialog() ) return 1 ; // Failure!
            return 0 ; // Success!
 
        case CPL_STOP:      // sent once per app. before CPL_EXIT 
            TRACE("CAceServerControlPanel::CPlApplet(CPL_STOP) called\n") ;
            theApp.DestroyDialog() ;
            return 0 ; // Success!
 
        case CPL_EXIT:    // sent once before FreeLibrary is called 
            TRACE("CAceServerControlPanel::CPlApplet(CPL_EXIT) called\n") ;
            return 0 ; // Success!
 
        default: 
            ASSERT(FALSE) ; // shouldn't happen?
    } 
    return 1 ; // Failure here?!
} 
 
//////////////////////////////////////////////////////////////////////////////////
//  AceServer Configuration Functionality

BOOL CAceServerControlPanelApp::InitDialog()
{
    // Prepare the configuration dialog...
    m_ConfigWizard = new CConfigPS(IDS_CONFIG_DIALOG_TITLE) ;
    if( m_ConfigWizard == NULL ) 
        return FALSE ;
    else
        return TRUE ;

}

BOOL CAceServerControlPanelApp::DisplayDialog()
{
    int result = m_ConfigWizard->DoModal() ;
    if( result == -1 || result == IDABORT )
        return FALSE ;
    return TRUE ;
}


void CAceServerControlPanelApp::DestroyDialog()
{
    delete m_ConfigWizard ;
}

