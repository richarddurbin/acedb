// AceServerConfigPP.cpp : implementation file
/*  Author: Richard Bruskiewich (rbrusk@octogene.medgen.ubc.ca)
 *  Copyright (C) R. Bruskiewich, J Thierry-Mieg and R Durbin, 1998
 *-------------------------------------------------------------------
 * This file is part of the ACEDB genome database package, written by
 * 	Richard Durbin (MRC LMB, UK) rd@mrc-lmb.cam.ac.uk, and
 *	Jean Thierry-Mieg (CRBM du CNRS, France) mieg@kaa.cnrs-mop.fr
 *
 * Description:      Microsoft Windows DCE AceServer Configuration   
 *                   Control Panel Application program
 *
 * Exported functions: 
 * HISTORY:
 * Last edited: 
 * Created: Jan 4 16:02 1998 (rbrusk)
 *-------------------------------------------------------------------
 */

#include "stdafx.h"
#include "AceServerControlPanel.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CAceServerConfigPP property page

IMPLEMENT_DYNCREATE(CAceServerConfigPP, CPropertyPage)

CAceServerConfigPP::CAceServerConfigPP() : CPropertyPage(CAceServerConfigPP::IDD)
{
	//{{AFX_DATA_INIT(CAceServerConfigPP)
	m_AutoSave_Interval = 600;
	m_Client_Timeout = 600;
	m_Max_Kilobytes = 0;
	m_Server_Timeout = 600;
	//}}AFX_DATA_INIT
}

CAceServerConfigPP::~CAceServerConfigPP()
{
}

void CAceServerConfigPP::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CAceServerConfigPP)
	DDX_Text(pDX, IDC_AUTO_SAVE_INTERVAL, m_AutoSave_Interval);
	DDX_Text(pDX, IDC_CLIENT_TIMEOUT, m_Client_Timeout);
	DDX_Text(pDX, IDC_MAX_KILOBYTES, m_Max_Kilobytes);
	DDX_Text(pDX, IDC_SERVER_TIMEOUT, m_Server_Timeout);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CAceServerConfigPP, CPropertyPage)
	//{{AFX_MSG_MAP(CAceServerConfigPP)
	ON_BN_CLICKED(IDC_RESET, OnReset)
	ON_WM_PAINT()
	ON_WM_HELPINFO()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CAceServerConfigPP message handlers

void CAceServerConfigPP::ResetDialog() 
{
    // Reset the dialog member variables with
    // the values saved from the system registry queries
    m_AutoSave_Interval     = m_Default_AutoSave_Interval;
    m_Client_Timeout        = m_Default_Client_Timeout;
    m_Max_Kilobytes         = m_Default_Max_Kilobytes;
    m_Server_Timeout        = m_Default_Server_Timeout;

    // Then update...
    UpdateData(FALSE) ;
}

void CAceServerConfigPP::OnReset() 
{
    ResetDialog() ;
}

LRESULT CAceServerConfigPP::OnWizardNext() 
{
    TRACE("CAceServerConfigPP::OnWizardNext(): ") ;

    // Retrieve the data
    UpdateData() ;

    CConfigPS* lpConfigPS = (CConfigPS*)GetParent() ;

    // Update the AceServer configuration
    if( !lpConfigPS->theRegistry.OpenServerKeys() )
    {
        TRACE("registry failure?\n") ;
        AfxMessageBox( "Cannot open registry keys? Abandon Ship!", 
                       MB_OK|MB_ICONERROR );

        ASSERT(FALSE) ;
        return -1 ;
    }

    if( !lpConfigPS->theRegistry.SetServerProperty( CLIENT_TIMEOUT,
                                                    m_Client_Timeout ) )
    {
        TRACE("registry failure; cannot update Client Timeout to registry?\n") ;
        AfxMessageBox( "Cannot update Client Timeout to registry?", 
                       MB_OK|MB_ICONERROR );

        ASSERT(FALSE) ;
        return -1 ;
    }

    if( !lpConfigPS->theRegistry.SetServerProperty( SERVER_TIMEOUT, 
                                                    m_Server_Timeout) )
    {
        TRACE("registry failure; cannot update Server Timeout to registry?\n") ;
        AfxMessageBox( "Cannot update Server Timeout to registry?", 
                       MB_OK|MB_ICONERROR );

        ASSERT(FALSE) ;
        return -1 ;
    }

    if( !lpConfigPS->theRegistry.SetServerProperty( MAX_KILOBYTES, 
                                                    m_Max_Kilobytes) )
    {
        TRACE("registry failure; cannot update MaxKbytes to registry?\n") ;
        AfxMessageBox( "Cannot update MaxKbytes to registry?", 
                       MB_OK|MB_ICONERROR );

        ASSERT(FALSE) ;
        return -1 ;
    }

    if( !lpConfigPS->theRegistry.SetServerProperty( AUTOSAVE_INTERVAL, 
                                          m_AutoSave_Interval) )
    {
        TRACE("registry failure; cannot update Auto Save Interval to registry?\n") ;
        AfxMessageBox( "Cannot update Auto Save Interval to registry?", 
                       MB_OK|MB_ICONERROR );

        ASSERT(FALSE) ;
        return -1 ;
    }

    // Then close registry
    lpConfigPS->theRegistry.CloseAllKeys() ;

    TRACE(" successful!\n") ;
    return CPropertyPage::OnWizardNext();
}


BOOL CAceServerConfigPP::OnSetActive() 
{
    CConfigPS* lpConfigPS = (CConfigPS*)GetParent() ;

    if( !lpConfigPS->theRegistry.OpenServerKeys() )
    {
        TRACE("CAceServerConfigPP::OnSetActive() could not open the registry?") ;
        return FALSE ; // Fatal error: could not open the registry?
    }

    if( !lpConfigPS->theRegistry.GetServerProperty( CLIENT_TIMEOUT,
                                            m_Default_Client_Timeout
                                         ) )
    {
        m_Default_Client_Timeout = 600 ;  // 10 minutes
        lpConfigPS->theRegistry.SetServerProperty( CLIENT_TIMEOUT,
                                          m_Default_Client_Timeout ) ;
    }

    if( !lpConfigPS->theRegistry.GetServerProperty( SERVER_TIMEOUT,
                                            m_Default_Server_Timeout
                                         ) )
    {
        m_Default_Server_Timeout = 600 ;  // 10 minutes
        lpConfigPS->theRegistry.SetServerProperty( SERVER_TIMEOUT, 
                                          m_Default_Server_Timeout) ;
    }

    if( !lpConfigPS->theRegistry.GetServerProperty(MAX_KILOBYTES,
                                           m_Default_Max_Kilobytes
                                         ) )
    {
        m_Default_Max_Kilobytes = 0 ; // No limit
        lpConfigPS->theRegistry.SetServerProperty( MAX_KILOBYTES, 
                                          m_Default_Max_Kilobytes) ;
    }

    if( !lpConfigPS->theRegistry.GetServerProperty( AUTOSAVE_INTERVAL,
                                            m_Default_AutoSave_Interval
                                         ) )
    {
        m_Default_AutoSave_Interval = 600 ; // 10 minutes
        lpConfigPS->theRegistry.SetServerProperty( AUTOSAVE_INTERVAL, 
                                          m_Default_AutoSave_Interval) ;
    }

    lpConfigPS->theRegistry.CloseAllKeys() ;

    ResetDialog() ;

    return CPropertyPage::OnSetActive();
}

void CAceServerConfigPP::OnPaint() 
{
    CPaintDC dc(this); // device context for painting
	
    CStatic* pMsg = (CStatic*)GetDlgItem(IDC_ACESERVER_CONFIG_MSG) ;
    ASSERT_VALID(pMsg) ;
    CConfigPS* lpConfigPS = (CConfigPS*)GetParent() ;
    pMsg->SetFont(&lpConfigPS->m_fntMessage) ;

    CString str ;
    str.LoadString(IDS_ACESERVER_CONFIG_MSG) ;
    SetDlgItemText(IDC_ACESERVER_CONFIG_MSG, str) ;
}

BOOL CAceServerConfigPP::OnHelpInfo(HELPINFO* pHelpInfo) 
{
    return win32FileCall( "Configuration.html" ) ;
}
