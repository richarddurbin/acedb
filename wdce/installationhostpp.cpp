// InstallationHostPP.cpp : implementation file
//

#include "stdafx.h"
#include "AceServerControlPanel.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CInstallationHostPP property page

IMPLEMENT_DYNCREATE(CInstallationHostPP, CPropertyPage)

CInstallationHostPP::CInstallationHostPP() : CPropertyPage(CInstallationHostPP::IDD)
{

        //{{AFX_DATA_INIT(CInstallationHostPP)
	m_Database_Name = _T("ACEDB");
	m_Selected_Host = 0;
        m_Remote_Host_Name = _T("");
        m_User_Name = _T("LocalSystem");
        m_Password = _T("");
	m_Domain_Name = _T("");
	//}}AFX_DATA_INIT
}

CInstallationHostPP::~CInstallationHostPP()
{
}

void CInstallationHostPP::DoDataExchange(CDataExchange* pDX)
{
        CPropertyPage::DoDataExchange(pDX);
        //{{AFX_DATA_MAP(CInstallationHostPP)
	DDX_Control(pDX, IDC_DOMAIN, m_Domain_Name_Edit_Ctl);
        DDX_Control(pDX, IDC_LOCAL_HOST, m_Local_Host_Ctl);
        DDX_Control(pDX, IDC_USER_NAME, m_User_Name_Edit_Ctl);
        DDX_Control(pDX, IDC_REMOTE_HOST_NAME, m_Remote_Host_Name_Edit_Ctl);
        DDX_Control(pDX, IDC_PASSWORD, m_Password_Edit_Ctl);
        DDX_Text(pDX, IDC_REMOTE_HOST_NAME, m_Remote_Host_Name);
        DDV_MaxChars(pDX, m_Remote_Host_Name, 256);
        DDX_Text(pDX, IDC_PASSWORD, m_Password);
        DDV_MaxChars(pDX, m_Password, 14);
        DDX_Text(pDX, IDC_USER_NAME, m_User_Name);
        DDV_MaxChars(pDX, m_User_Name, 32);
	DDX_Radio(pDX, IDC_LOCAL_HOST, m_Selected_Host);
	DDX_Text(pDX, IDC_DOMAIN, m_Domain_Name);
        DDV_MaxChars(pDX, m_Domain_Name, 32);
	DDX_Text(pDX, IDC_DATABASE_NAME, m_Database_Name);
	DDV_MaxChars(pDX, m_Database_Name, 32);
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CInstallationHostPP, CPropertyPage)
    //{{AFX_MSG_MAP(CInstallationHostPP)
    ON_BN_CLICKED(IDC_LOCAL_HOST, OnLocalHost)
    ON_BN_CLICKED(IDC_REMOTE_HOST, OnRemoteHost)
	ON_WM_PAINT()
	ON_WM_HELPINFO()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CInstallationHostPP message handlers

BOOL CInstallationHostPP::OnInitDialog() 
{
    CPropertyPage::OnInitDialog();
    
    return TRUE;  // return TRUE unless you set the focus to a control
	          // EXCEPTION: OCX Property Pages should return FALSE
}

void CInstallationHostPP::OnLocalHost() 
{
    m_Remote_Host_Name_Edit_Ctl.EnableWindow(FALSE) ;
}

void CInstallationHostPP::OnRemoteHost() 
{
    m_Remote_Host_Name_Edit_Ctl.EnableWindow(TRUE) ;
}


BOOL CInstallationHostPP::OnSetActive() 
{
    // Display the Back & Next Button
    CPropertySheet* pParent = (CPropertySheet*)GetParent() ;
    pParent->SetWizardButtons(PSWIZB_BACK | PSWIZB_NEXT) ;

    return CPropertyPage::OnSetActive();
}

LRESULT CInstallationHostPP::OnWizardNext() 
{
    UpdateData() ;

    CConfigPS* lpConfigPS = (CConfigPS*)GetParent() ;

    if( m_Database_Name.IsEmpty() )
    {
        TRACE("empty service name?\n") ;
        AfxMessageBox( "Database name must be provided!", 
                    MB_OK|MB_ICONERROR );
        m_Database_Name = _T("Acedb");
        return -1 ;
    }
    else
        lpConfigPS->theRegistry.SetDatabaseName(m_Database_Name) ;

     // Remote Host is selected?
    if( !m_Local_Host_Ctl.GetCheck() ) // Remote Host is selected?
    {
        if( m_Remote_Host_Name.IsEmpty() )
        {
            TRACE("empty host name?\n") ;
            AfxMessageBox( "Host name must be provided if Remote Host button selected!", 
                        MB_OK|MB_ICONERROR );
            return -1 ;
        }
        if( !lpConfigPS->theRegistry.Open(m_Remote_Host_Name) )
        {
            CString msg ;
            AfxFormatString1( msg, IDS_REMOTE_HOST_CONNECT_FAILURE,
                              m_Remote_Host_Name ); 
            AfxMessageBox( msg, MB_OK|MB_ICONERROR );
            return -1 ;
        }
    }
    else // Local Host selected
    {
        lpConfigPS->theRegistry.Open() ;
    }

    return CPropertyPage::OnWizardNext();
}

LPCTSTR CInstallationHostPP::Account() 
{ 
    static CString account ;
    if ( m_User_Name.IsEmpty() )
        return NULL ;
    else
    {
        if ( !m_Domain_Name.IsEmpty() )
            account = m_Domain_Name ;
        else
            account = "." ;
        account += "\\" ;
        account += m_User_Name ;
        return (LPCTSTR) account ;
    }
}

LPCTSTR CInstallationHostPP::Password() 
{ 
    if ( m_User_Name.IsEmpty() )
        return NULL ;
    else
        return (LPCTSTR) m_Password ;
}


void CInstallationHostPP::OnPaint() 
{
    CPaintDC dc(this); // device context for painting
    
    CStatic* pMsg = (CStatic*)GetDlgItem(IDC_INSTALLATION_MSG) ;
    ASSERT_VALID(pMsg) ;
    CConfigPS* lpConfigPS = (CConfigPS*)GetParent() ;
    pMsg->SetFont(&lpConfigPS->m_fntMessage) ;

    CString str ;
    str.LoadString(IDS_INSTALLATION_MSG) ;
    SetDlgItemText(IDC_INSTALLATION_MSG, str) ;
}

BOOL CInstallationHostPP::OnHelpInfo(HELPINFO* pHelpInfo) 
{
    return win32FileCall( "Target.html" ) ;

}
