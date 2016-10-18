/* ConnectionsPP.cpp : implementation file
 *  Author: Richard Bruskiewich (rbrusk@octogene.medgen.ubc.ca)
 *  Copyright (C) R. Bruskiewich, J Thierry-Mieg and R Durbin, 1998
 *-------------------------------------------------------------------
 * This file is part of the ACEDB genome database package, written by
 * 	Richard Durbin (MRC LMB, UK) rd@mrc-lmb.cam.ac.uk, and
 *	Jean Thierry-Mieg (CRBM du CNRS, France) mieg@kaa.cnrs-mop.fr
 *
 * Description:      Microsoft Windows DCE AceServer Configuration Program   
 *                   Connections Property page management
 *
 * HISTORY:
 * Last edited: Jan 16 10:08 1998 (rbrusk)
 * Created: Jan 15 04:02 1998 (rbrusk)
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
// CConnectionsPP dialog

IMPLEMENT_DYNCREATE(CConnectionsPP, CPropertyPage)

CConnectionsPP::CConnectionsPP() : CPropertyPage(CConnectionsPP::IDD)
{
	//{{AFX_DATA_INIT(CConnectionsPP)
	m_Named_Pipe_Endpoint = _T("\\pipe\\aceserver");
	m_Named_Pipe_Enabled = TRUE ;
	m_TCP_IP_Port = MIN_TCPIP_PORT_NO ;
	m_TCP_IP_Enabled = TRUE ;
	m_NetBEUI_Port = 0;
	m_NetBEUI_Enabled = FALSE;
	m_Custom_Protocol = _T("");
	m_Custom_Endpoint = _T("");
	m_Custom_Protocol_Enabled = FALSE;
	//}}AFX_DATA_INIT
}

void CConnectionsPP::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CConnectionsPP)
	DDX_Text(pDX, IDC_NAMED_PIPE_ENDPOINT, m_Named_Pipe_Endpoint);
	DDV_MaxChars(pDX, m_Named_Pipe_Endpoint, 32);
	DDX_Check(pDX, IDC_NAMED_PIPE_ENABLED, m_Named_Pipe_Enabled);
	DDX_Check(pDX, IDC_NETBEUI_ENABLED, m_NetBEUI_Enabled);
	DDX_Text(pDX, IDC_TCP_IP_ENDPOINT, m_TCP_IP_Port);
	DDV_MinMaxUInt(pDX, m_TCP_IP_Port, 11000, 65535);
	DDX_Check(pDX, IDC_TCP_IP_ENABLED, m_TCP_IP_Enabled);
	DDX_CBString(pDX, IDC_CUSTOM_PROTOCOL, m_Custom_Protocol);
	DDV_MaxChars(pDX, m_Custom_Protocol, 16);
	DDX_Text(pDX, IDC_CUSTOM_ENDPOINT, m_Custom_Endpoint);
	DDV_MaxChars(pDX, m_Custom_Endpoint, 32);
	DDX_Check(pDX, IDC_CUSTOM_PROTOCOL_ENABLED, m_Custom_Protocol_Enabled);
	DDX_Text(pDX, IDC_NETBEUI_ENDPOINT, m_NetBEUI_Port);
	DDV_MinMaxInt(pDX, m_NetBEUI_Port, 32, 255);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CConnectionsPP, CPropertyPage)
	//{{AFX_MSG_MAP(CConnectionsPP)
	ON_BN_CLICKED(ID_RESET, OnReset)
	ON_WM_PAINT()
	ON_WM_HELPINFO()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CConnectionsPP message handlers

void CConnectionsPP::ResetDialog() 
{
    // Initialize the dialog member variables with
    // the current AceServer values found in the system registry
    m_Named_Pipe_Endpoint   = m_Default_Named_Pipe.Endpoint();
    m_Named_Pipe_Enabled    = m_Default_Named_Pipe.IsEnabled() ;
    m_TCP_IP_Port           = (UINT)atoi((LPCTSTR)m_Default_TCP_IP.Endpoint()) ;
    m_TCP_IP_Enabled        = m_Default_TCP_IP.IsEnabled() ;
    m_NetBEUI_Port          = (short)atoi((LPCTSTR)m_Default_NetBEUI.Endpoint()) ;
    m_NetBEUI_Enabled       = m_Default_NetBEUI.IsEnabled() ;
    m_Custom_Protocol       = m_Default_Custom.Protocol() ;
    m_Custom_Endpoint       = m_Default_Custom.Endpoint() ;
    m_Custom_Protocol_Enabled = m_Default_Custom.IsEnabled() ;

    // Then update...
    UpdateData(FALSE) ;
}

// Enable named pipe and tcpip by default
BOOL CConnectionsPP::OnSetActive() 
{
    CConfigPS* lpConfigPS = (CConfigPS*)GetParent() ;

    /* Should check the registry for existing CConnection values
       then set each connection type to those values (if present) */
    if( !lpConfigPS->theRegistry.OpenServerKeys() )
    {
        TRACE("CConnectionsPP::OnInitDialog() could not open the registry?") ;
        return FALSE ; // Fatal error: could not open the registry?
    }

    if( !lpConfigPS->theRegistry.GetConnection(CConnection::NAMEDPIPE, m_Default_Named_Pipe) )
    {
		m_Default_Named_Pipe.Protocol("ncacn_np") ;
        CString pipeName("\\pipe\\") ;
		pipeName += lpConfigPS->Database()->ServiceName() ;
        m_Default_Named_Pipe.Endpoint(pipeName) ;
        m_Default_Named_Pipe.Enable(TRUE) ;
        lpConfigPS->theRegistry.SetConnection(  CConnection::NAMEDPIPE, 
                                                m_Default_Named_Pipe) ;
    }

    if( !lpConfigPS->theRegistry.GetConnection(CConnection::TCPIP, m_Default_TCP_IP) )
    {
        m_Default_TCP_IP.Protocol("ncacn_ip_tcp") ;
        m_Default_TCP_IP.Endpoint(MIN_TCPIP_PORT_STR) ;
        m_Default_TCP_IP.Enable(TRUE) ;
        lpConfigPS->theRegistry.SetConnection(  CConnection::TCPIP, 
                                                m_Default_TCP_IP) ;
    }

    if( !lpConfigPS->theRegistry.GetConnection(CConnection::NETBEUI, m_Default_NetBEUI) )
    {
        m_Default_NetBEUI.Protocol("ncacn_nb_nb") ;
        m_Default_NetBEUI.Endpoint(MIN_NETBEUI_PORT_STR) ;
        m_Default_NetBEUI.Enable(FALSE) ;
        lpConfigPS->theRegistry.SetConnection(  CConnection::NETBEUI, 
                                                m_Default_NetBEUI ) ;
    }

    if( !lpConfigPS->theRegistry.GetConnection(CConnection::CUSTOM, m_Default_Custom ) )
        lpConfigPS->theRegistry.SetConnection(  CConnection::CUSTOM,
                                                m_Default_Custom ) ;

    lpConfigPS->theRegistry.CloseAllKeys() ;

    ResetDialog() ;

    return CPropertyPage::OnSetActive();
}

void CConnectionsPP::OnReset() 
{
    ResetDialog() ;
}

LRESULT CConnectionsPP::OnWizardNext() 
{
    TRACE("CConnectionsPP::OnWizardNext(): ") ;

    // Retrieve the data
    UpdateData() ;

    CConfigPS* lpConfigPS = (CConfigPS*)GetParent() ;

    // Update the AceServer configuration
    if( !lpConfigPS->theRegistry.OpenServerKeys() )
    {
        TRACE("registry failure?\n") ;
        AfxMessageBox( "Cannot update values to registry... Abandon Ship!", 
                       MB_OK|MB_ICONERROR );

        ASSERT(FALSE) ;
        return -1 ;
    }
    
    if(m_Named_Pipe_Enabled)
    {
        TRACE("\tName Pipe Enabled...\n") ;
        CConnection np("ncacn_np",m_Named_Pipe_Endpoint,TRUE) ;
        lpConfigPS->theRegistry.SetConnection(CConnection::NAMEDPIPE, np) ;
    }
    else 
        lpConfigPS->theRegistry.DisableConnection(CConnection::NAMEDPIPE) ;

    if(m_TCP_IP_Enabled)
    {
        TRACE("\tTCP/IP Enabled...\n") ;
        char port[6] ;
        CConnection tcpip("ncacn_ip_tcp",itoa(m_TCP_IP_Port,port,10),TRUE) ;
        lpConfigPS->theRegistry.SetConnection(CConnection::TCPIP, tcpip) ;
    }
    else 
        lpConfigPS->theRegistry.DisableConnection(CConnection::TCPIP) ;

    if(m_NetBEUI_Enabled)
    {
        TRACE("\tNetBEUI Enabled...\n") ;
        char port[4] ;
        CConnection nb("ncacn_nb_nb",itoa(m_NetBEUI_Port,port,10),TRUE) ;
        lpConfigPS->theRegistry.SetConnection(CConnection::NETBEUI, nb) ;
    }
    else 
        lpConfigPS->theRegistry.DisableConnection(CConnection::NETBEUI) ;

    if(m_Custom_Protocol_Enabled)
    {
        TRACE("\tCustom Protocol Enabled...\n") ;
        CConnection cp(m_Custom_Protocol,m_Custom_Endpoint,TRUE) ;
        lpConfigPS->theRegistry.SetConnection(CConnection::CUSTOM, cp) ;
    }
    else 
        lpConfigPS->theRegistry.DisableConnection(CConnection::CUSTOM) ;
    
    // Then close registry
    lpConfigPS->theRegistry.CloseAllKeys() ;
    
    return CPropertyPage::OnWizardNext();
}

void CConnectionsPP::OnPaint() 
{
    CPaintDC dc(this); // device context for painting

    CStatic* pMsg = (CStatic*)GetDlgItem(IDC_CONNECTIONS_MSG) ;
    ASSERT_VALID(pMsg) ;
    CConfigPS* lpConfigPS = (CConfigPS*)GetParent() ;
    pMsg->SetFont(&lpConfigPS->m_fntMessage) ;

    CString str ;
    str.LoadString(IDS_CONNECTIONS_MSG) ;
    SetDlgItemText(IDC_CONNECTIONS_MSG, str) ;
}

BOOL CConnectionsPP::OnHelpInfo(HELPINFO* pHelpInfo) 
{
    return win32FileCall( "Connections.html" ) ;
}
