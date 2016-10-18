/* ServiceRegistryPP.cpp : implementation file
 *  Author: Jean Thierry-Mieg (mieg@kaa.cnrs-mop.fr)
 *  Copyright (C) J Thierry-Mieg and R Durbin, 1992
 *-------------------------------------------------------------------
 * This file is part of the ACEDB genome database package, written by
 * 	Richard Durbin (MRC LMB, UK) rd@mrc-lmb.cam.ac.uk, and
 *	Jean Thierry-Mieg (CRBM du CNRS, France) mieg@kaa.cnrs-mop.fr
 *
 * Exported functions:
 * HISTORY:
 * Last edited: 
 * Created: Jan 98
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
// CServiceRegistryPP property page

IMPLEMENT_DYNCREATE(CServiceRegistryPP, CPropertyPage)

CServiceRegistryPP::CServiceRegistryPP() : CPropertyPage(CServiceRegistryPP::IDD)
{
	//{{AFX_DATA_INIT(CServiceRegistryPP)
	m_Service_Mode = 0;
    m_AceServer_Pathname = _T("c:\\acedb\\bin\\aceserver.exe");
	m_Service_DisplayName = _T("ACEDB DCE Server");
	//}}AFX_DATA_INIT
}

CServiceRegistryPP::~CServiceRegistryPP()
{
}

void CServiceRegistryPP::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CServiceRegistryPP)
	DDX_Control(pDX, IDC_SERVICE_DISPLAY_NAME, m_Service_DisplayName_Ctl);
	DDX_Control(pDX, IDC_ACESERVER_PATHNAME, m_AceServer_Pathname_Ctl);
	DDX_Radio(pDX, IDC_DEMAND_SERVICE, m_Service_Mode);
	DDX_Text(pDX, IDC_ACESERVER_PATHNAME, m_AceServer_Pathname);
	DDV_MaxChars(pDX, m_AceServer_Pathname, 256);
	DDX_Text(pDX, IDC_SERVICE_DISPLAY_NAME, m_Service_DisplayName);
	DDV_MaxChars(pDX, m_Service_DisplayName, 256);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CServiceRegistryPP, CPropertyPage)
	//{{AFX_MSG_MAP(CServiceRegistryPP)
	ON_WM_PAINT()
	ON_BN_CLICKED(IDC_ACESERVER_BROWSE, OnAceserverBrowse)
	ON_WM_HELPINFO()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CServiceRegistryPP message handlers

BOOL CServiceRegistryPP::OnInitDialog() 
{
    CPropertyPage::OnInitDialog();
	
    UpdateData(FALSE) ;

    return TRUE;  // return TRUE unless you set the focus to a control
	          // EXCEPTION: OCX Property Pages should return FALSE
}

// Hopefully this is more than adequate buffer space for old service control info...
#define SCBSIZE sizeof(QUERY_SERVICE_CONFIG)+600

BOOL CServiceRegistryPP::OnSetActive() 
{
    CConfigPS* lpConfigPS = (CConfigPS*)GetParent() ;

    if(lpConfigPS->Configuration()->Service() == NULL)
    {
		TRACE("AceServer Service does not exist?\n") ;
        m_Service_Mode = 0;
        m_AceServer_Pathname = "c:\\acedb\\bin\\aceserver.exe" ;
		m_Service_DisplayName = "ACEDB ";
		m_Service_DisplayName += lpConfigPS->Host()->Database() ;
		m_Service_DisplayName += " Server";
    }
    else
    {
        // Retrieve current values of service configuration
	TRACE("AceServer Service exists?\n") ;
        DWORD cbBufSize = SCBSIZE, cbBytesNeeded ; 
        BYTE ServiceConfigBuf[SCBSIZE] ;
        LPQUERY_SERVICE_CONFIG lpServiceConfig = 
            (LPQUERY_SERVICE_CONFIG)ServiceConfigBuf ;

        if( !QueryServiceConfig(lpConfigPS->Configuration()->Service(),	 
                                lpServiceConfig,	 
                                cbBufSize,	
                                &cbBytesNeeded) )  // In principle, I could recover 
        {
            // In principle, I could recover from a buffer size error
            // but I don't expect this to happen very often, so...I die
            CError::ErrorMsg() ;
            return 0 ;
        }
        // What type of service?
        switch(lpServiceConfig->dwStartType)
        {
            case SERVICE_DEMAND_START:
 	        m_Service_Mode = 0;
                break ;
            case SERVICE_AUTO_START:
 	        m_Service_Mode = 1;
                break ;
            case SERVICE_DISABLED:
 	        m_Service_Mode = 2;
                break ;
            default:
                TRACE("CServiceRegistryPP::OnSetActive(): Invalid service start type?") ;
                ASSERT(FALSE) ;
                SetLastError(ERROR_SERVICE_SPECIFIC_ERROR) ;
                CError::ErrorMsg() ;
                return 0 ;
        }
	m_AceServer_Pathname  = lpServiceConfig->lpBinaryPathName ;
	m_Service_DisplayName = lpServiceConfig->lpDisplayName ;
    }

    UpdateData(FALSE) ;

    return CPropertyPage::OnSetActive();
}

LRESULT CServiceRegistryPP::OnWizardNext() 
{
    // Retrieve the data:
    // m_Service_Mode, m_Service_DisplayName and m_AceServer_Pathname
    UpdateData() ;

    //*****************************************************************
    // Register the service, if the user specifies it
    //*****************************************************************
    // What type of service?
    DWORD dwStartType ;
    switch(m_Service_Mode)
    {
        case 1:
            dwStartType = SERVICE_AUTO_START ;
            break ;
        case 2:
            dwStartType = SERVICE_DISABLED ;
            break ;
        case 0 :
        default:
            dwStartType = SERVICE_DEMAND_START ;
            break ;
    }

    // Goto Configuration Property Page for User Account and Password
    CConfigPS* lpConfigPS = (CConfigPS*)GetParent() ;

    if(lpConfigPS->Configuration()->Service() == NULL)  // service does NOT yet exist?
    {
        // ...then create it, or...
        SC_HANDLE hAceService =
            CreateService(  lpConfigPS->Configuration()->Manager(),
                            lpConfigPS->Database()->ServiceName(),
                            m_Service_DisplayName,
                            SERVICE_ALL_ACCESS,	 
                            SERVICE_WIN32_OWN_PROCESS, 
                            dwStartType, 
                            SERVICE_ERROR_NORMAL, 
                            m_AceServer_Pathname,	
                            NULL,NULL,	 
                            // LPCTSTR lpDependencies -- pointer to array of dependency names
                            // i likely need RPC services, etc. listed here, especially for "automatic" mode
                            NULL, 
                            lpConfigPS->Host()->Account(),
                            lpConfigPS->Host()->Password()
                          ) ;

        /* Set Service Security Level here...

            BOOL SetServiceObjectSecurity(
                    hAceService,	// handle of service 
                    SECURITY_INFORMATION dwSecurityInformation,	// type of security information requested  
                    PSECURITY_DESCRIPTOR lpSecurityDescriptor 	// address of security descriptor 
                );
        */
        if(hAceService == NULL)
        {
            CError::ErrorMsg() ;
            return -1 ;
        }
        else 
            lpConfigPS->Configuration()->Service(hAceService) ;
    }
    else
    {
        if( !ChangeServiceConfig(   lpConfigPS->Configuration()->Service(),
                                    SERVICE_WIN32_OWN_PROCESS, 
                                    dwStartType, 
                                    SERVICE_ERROR_NORMAL, 
                                    m_AceServer_Pathname,	
                                    NULL, NULL, NULL,
                                    lpConfigPS->Host()->Account(),
                                    lpConfigPS->Host()->Password(),
                                    m_Service_DisplayName )
                                )
            {
                CError::ErrorMsg() ;
                return -1 ;
            }
     
    }
    return CPropertyPage::OnWizardNext();
}

void CServiceRegistryPP::OnPaint() 
{
    CPaintDC dc(this); // device context for painting
	
    CStatic* pMsg = (CStatic*)GetDlgItem(IDC_SERVICE_CONFIG_MSG) ;
    ASSERT_VALID(pMsg) ;
    CConfigPS* lpConfigPS = (CConfigPS*)GetParent() ;
    pMsg->SetFont(&lpConfigPS->m_fntMessage) ;

    CString str ;
    str.LoadString(IDS_SERVICE_CONFIG_MSG) ;
    SetDlgItemText(IDC_SERVICE_CONFIG_MSG, str) ;
}

void CServiceRegistryPP::OnAceserverBrowse() 
{
{
    UpdateData(TRUE) ; // to save already updated values

    static char szFilter[] = "Executable files (*.exe)|*.exe||" ;
    CFileDialog filDlg(TRUE,"exe",NULL,
        OFN_FILEMUSTEXIST|OFN_HIDEREADONLY|OFN_LONGNAMES|
        OFN_NONETWORKBUTTON|OFN_PATHMUSTEXIST, szFilter,(CWnd*)this) ;
    
    char pathnameBuf[256], *ppathnameBuf = pathnameBuf ;

    CConfigPS* lpConfigPS = (CConfigPS*)GetParent() ;

    if(!m_AceServer_Pathname.IsEmpty())
	    strcpy(ppathnameBuf, m_AceServer_Pathname) ;
    else
	    pathnameBuf[0] = '\0' ;
    filDlg.m_ofn.lpstrFile = ppathnameBuf ;
    filDlg.m_ofn.nMaxFile  = 256 ;

    filDlg.m_ofn.lpstrTitle = "AceServer Path Name" ;

    if( filDlg.DoModal() == IDOK )
    {
   	    m_AceServer_Pathname = filDlg.GetPathName() ;
            UpdateData(FALSE) ;
    }
}

}

BOOL CServiceRegistryPP::OnHelpInfo(HELPINFO* pHelpInfo) 
{
    return win32FileCall( "Service.html" ) ;
}
