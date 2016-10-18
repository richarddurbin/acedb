// DatabasePP.cpp : implementation file
//

#include "stdafx.h"
#include "AceServerControlPanel.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CDatabasePP property page

IMPLEMENT_DYNCREATE(CDatabasePP, CPropertyPage)

CDatabasePP::CDatabasePP() : CPropertyPage(CDatabasePP::IDD)
{
	//{{AFX_DATA_INIT(CDatabasePP)
	m_Service_Name = _T("aceserver");
    m_ACEDB = _T("c:\\acedb");
    m_ACEDB_COMMON = _T("c:\\acedb");
    m_ACEDB_DATA = _T("c:\\acedb\\rawdata");
	m_Database_Value = _T("jpg");
	m_Database_Property = _T("JPEG");
	//}}AFX_DATA_INIT
}

CDatabasePP::~CDatabasePP()
{
}

void CDatabasePP::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CDatabasePP)
	DDX_Text(pDX, IDC_SERVICE_NAME, m_Service_Name);
	DDV_MaxChars(pDX, m_Service_Name, 32);
	DDX_Text(pDX, IDC_ACEDB, m_ACEDB);
	DDX_Text(pDX, IDC_ACEDB_COMMON, m_ACEDB_COMMON);
	DDX_Text(pDX, IDC_ACEDB_DATA, m_ACEDB_DATA);
	DDX_Text(pDX, IDC_VALUE, m_Database_Value);
	DDX_Text(pDX, IDC_PROPERTY, m_Database_Property);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CDatabasePP, CPropertyPage)
	//{{AFX_MSG_MAP(CDatabasePP)
	ON_WM_PAINT()
	ON_BN_CLICKED(IDC_SET, OnPropertySet)
	ON_WM_HELPINFO()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CDatabasePP message handlers

BOOL CDatabasePP::OnSetActive() 
{
    CConfigPS* lpConfigPS = (CConfigPS*)GetParent() ;

    if( !lpConfigPS->theRegistry.OpenDatabaseKeys() )
    {
        TRACE("CDatabasePP::OnSetActive() could not open the OpenDatabaseKeys()?") ;
        return FALSE ; // Fatal error: could not open the registry?
    }

    if( !lpConfigPS->theRegistry.GetDatabaseProperty( "ACEDB", 
                                                      m_ACEDB ) )
    {
        m_ACEDB = _T("c:\\acedb") ;
        lpConfigPS->theRegistry.SetDatabaseProperty(  "ACEDB",
                                                      m_ACEDB) ;
    }

    if( !lpConfigPS->theRegistry.GetDatabaseProperty( "ACEDB_DATA",
                                                      m_ACEDB_DATA ) )
    {
        m_ACEDB_DATA = _T("c:\\acedb\\rawdata") ;
        lpConfigPS->theRegistry.SetDatabaseProperty( "ACEDB_DATA",
                                                     m_ACEDB_DATA ) ;
    }

    if( !lpConfigPS->theRegistry.GetDatabaseProperty( "ACEDB_COMMON",
                                                      m_ACEDB_COMMON ) )
    {
        m_ACEDB_COMMON = _T("c:\\acedb") ;
        lpConfigPS->theRegistry.SetDatabaseProperty( "ACEDB_COMMON",
                                                     m_ACEDB_COMMON ) ;
    }

    UpdateData(FALSE) ;

    lpConfigPS->theRegistry.CloseAllKeys() ;

	
    return CPropertyPage::OnSetActive();
}

LRESULT CDatabasePP::OnWizardNext() 
{
    UpdateData() ;

    CConfigPS* lpConfigPS = (CConfigPS*)GetParent() ;

    if( m_Service_Name.IsEmpty() )
    {
        TRACE("empty service name?\n") ;
        AfxMessageBox( "Service name must be provided!", 
                    MB_OK|MB_ICONERROR );
        m_Service_Name = _T("AceServer");
        return -1 ;
    }
    else
        lpConfigPS->theRegistry.SetServiceName(m_Service_Name) ;

    if( !lpConfigPS->theRegistry.OpenDatabaseKeys() )
    {
        TRACE("CDatabasePP::OnWizardNext() could not open the OpenDatabaseKeys()?") ;
        return FALSE ; // Fatal error: could not open the registry?
    }

    if( !lpConfigPS->theRegistry.SetDatabaseProperty( "ACEDB",
                                                      m_ACEDB ) )
    {
        TRACE("registry failure; cannot update ACEDB parameter to database registry?\n") ;
        AfxMessageBox( "Cannot update ACEDB parameter to registry?", 
                       MB_OK|MB_ICONERROR );

        ASSERT(FALSE) ;
        return -1 ;
    }

    if( !lpConfigPS->theRegistry.SetDatabaseProperty( "ACEDB_DATA", 
                                                      m_ACEDB_DATA) )
    {
        TRACE("registry failure; cannot update ACEDB_DATA parameter to database registry?\n") ;
        AfxMessageBox( "Cannot update ACEDB_DATA parameter to registry?", 
                       MB_OK|MB_ICONERROR );

        ASSERT(FALSE) ;
        return -1 ;
    }

    if( !lpConfigPS->theRegistry.SetDatabaseProperty( "ACEDB_COMMON", 
                                                      m_ACEDB_COMMON) )
    {
        TRACE("registry failure; cannot update ACEDB_COMMON parameter to database registry?\n") ;
        AfxMessageBox( "Cannot update ACEDB_COMMON parameter to registry?", 
                       MB_OK|MB_ICONERROR );

        ASSERT(FALSE) ;
        return -1 ;
    }

    // For redundancy, save any "other" property values exhibited
    if( !m_Database_Property.IsEmpty() &&
        !lpConfigPS->theRegistry.SetDatabaseProperty( m_Database_Property, 
                                                      m_Database_Value) )
    {
        TRACE2("registry failure; cannot update Property \"%s\" value == \"%s\" to database registry?\n",
               m_Database_Property, m_Database_Value ) ;
        AfxMessageBox( "Cannot update ACEDB_COMMON parameter to registry?", 
                       MB_OK|MB_ICONERROR );

        ASSERT(FALSE) ;
        return -1 ;
    }

    lpConfigPS->theRegistry.CloseAllKeys() ;

    return CPropertyPage::OnWizardNext();
}

void CDatabasePP::OnPaint() 
{
    CPaintDC dc(this); // device context for painting
	
    CStatic* pMsg = (CStatic*)GetDlgItem(IDC_DATABASE_MSG) ;
    ASSERT_VALID(pMsg) ;
    CConfigPS* lpConfigPS = (CConfigPS*)GetParent() ;
    pMsg->SetFont(&lpConfigPS->m_fntMessage) ;

    CString str ;
    str.LoadString(IDS_DATABASE_MSG) ;
    SetDlgItemText(IDC_DATABASE_MSG, str) ;
}

/**** Removed for now... Keep code here for reference ***

void CDatabasePP::OnDatabaseBrowse() 
{
    UpdateData(TRUE) ; // to save already updated values

    static char szFilter[] = "ACEDB wspec files (*.wrm)|*.wrm||" ;
    CFileDialog filDlg(TRUE,"wrm",NULL,
        OFN_FILEMUSTEXIST|OFN_HIDEREADONLY|OFN_LONGNAMES|
        OFN_NONETWORKBUTTON|OFN_PATHMUSTEXIST, szFilter,(CWnd*)this) ;
    
    char pathnameBuf[256], *ppathnameBuf = pathnameBuf ;

    CConfigPS* lpConfigPS = (CConfigPS*)GetParent() ;

    if(!m_Database_Location.IsEmpty())
	    strcpy(ppathnameBuf, m_Database_Location) ;
    else
	    pathnameBuf[0] = '\0' ;
    filDlg.m_ofn.lpstrFile = ppathnameBuf ;
    filDlg.m_ofn.nMaxFile  = 256 ;

    filDlg.m_ofn.lpstrTitle = "Acedb Database Directory" ;

    if( filDlg.DoModal() == IDOK )
    {
   	    m_Database_Location = filDlg.GetPathName() ;
            UpdateData(FALSE) ;
    }
}
*/
void CDatabasePP::OnPropertySet() 
{
    UpdateData() ;

    CConfigPS* lpConfigPS = (CConfigPS*)GetParent() ;

    if( !lpConfigPS->theRegistry.OpenDatabaseKeys() )
    {
        TRACE("CDatabasePP::OnPropertySet() could not open the OpenDatabaseKeys()?") ;
	messout("The database registry key could not be opened?") ;
        return ;
    }
    if( !lpConfigPS->theRegistry.SetDatabaseProperty( m_Database_Property, 
                                                      m_Database_Value) )
    {
        TRACE2("CDatabasePP::OnPropertySet() could not set property:\"%s\" to value:\"%s\"\n",
               m_Database_Property, m_Database_Value) ;
	messout("Database property value could not be set?") ;
        return ;
    }
    if( !lpConfigPS->theRegistry.CloseAllKeys() )
    {
        TRACE("CDatabasePP::OnPropertySet() could not close the CloseAllKeys()?") ;
	messout("Database registry could not be closed?") ;
        return ;
    }
    TRACE2("CDatabasePP::OnPropertySet(): database property:\"%s\" successfully set to value:\"%s\"\n",
           m_Database_Property, m_Database_Value) ;
    messout(messprintf("Database property \"%s\" successfully set to value \"%s\"",
                        m_Database_Property, m_Database_Value )) ;
}

BOOL CDatabasePP::OnHelpInfo(HELPINFO* pHelpInfo) 
{
    return win32FileCall( "Database.html" ) ;
}
