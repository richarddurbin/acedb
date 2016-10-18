// ServiceStatusPP.cpp : implementation file
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
 * Last edited: Mar 15 14:02 1998 (rbrusk): Use LocalSystem account for now?
 * Created: Jan 4 16:02 1998 (rbrusk)
 *-------------------------------------------------------------------
 */
//

#include "stdafx.h"
#include "AceServerControlPanel.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CServiceStatusPP property page

IMPLEMENT_DYNCREATE(CServiceStatusPP, CPropertyPage)

CServiceStatusPP::CServiceStatusPP() : CPropertyPage(CServiceStatusPP::IDD)
{
    m_hSCMgr = NULL ;
    m_hAceService = NULL ;

    //{{AFX_DATA_INIT(CServiceStatusPP)
    m_Existing_Service = 0;
    //}}AFX_DATA_INIT
}

CServiceStatusPP::~CServiceStatusPP()
{
    if(m_hAceService != NULL && !CloseServiceHandle( m_hAceService ) )
        AfxMessageBox(  TEXT("AceService handle could not be closed?"),
                        MB_OK|MB_ICONEXCLAMATION );
    if(m_hSCMgr != NULL && !CloseServiceHandle( m_hSCMgr ) )
        AfxMessageBox(  TEXT("Service Control Manager handle could not be closed?"),
                        MB_OK|MB_ICONEXCLAMATION );
}

BOOL CServiceStatusPP::OnInitDialog() 
{
	CPropertyPage::OnInitDialog();
	
        m_fntServiceName.CreateFont(MESSAGE_FONT_SIZE,0,0,0,FW_MEDIUM,TRUE,FALSE,
                0,ANSI_CHARSET, OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,
                DEFAULT_QUALITY,DEFAULT_PITCH | FF_DONTCARE,"MS Sans Serif") ;

        return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CServiceStatusPP::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CServiceStatusPP)
	DDX_Control(pDX, IDC_EXISTING_SERVICE_GROUP, m_Existing_Service_Group);
	DDX_Control(pDX, IDC_EDIT_SERVICE, m_Existing_Service_Ctl);
	DDX_Control(pDX, IDC_DELETE_SERVICE, m_Delete_Service_Ctl);
	DDX_Radio(pDX, IDC_EDIT_SERVICE, m_Existing_Service);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CServiceStatusPP, CPropertyPage)
	//{{AFX_MSG_MAP(CServiceStatusPP)
	ON_WM_PAINT()
	ON_WM_HELPINFO()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CServiceStatusPP message handlers

BOOL CServiceStatusPP::OnSetActive() 
{
    CConfigPS* lpConfigPS = (CConfigPS*)GetParent() ;

    // Attempt to open an existing 
    // AceServer service record in system registry
    // if a service control manager is open but
    // the specified host is different from previously
    // then close the current service control manager...
    if( m_hSCMgr != NULL  &&
        m_CurrentHostName.CompareNoCase(lpConfigPS->Host()->HostName()) )
    {
        if( !CloseServiceHandle(m_hSCMgr) )
        {
            CError::ErrorMsg() ;
            return 0 ;
        }
        else m_hSCMgr = NULL ;
    }

    // if no service control manager is open, attempt to open one
    if( m_hSCMgr == NULL )
    {
        m_CurrentHostName = lpConfigPS->Host()->HostName() ;
        // Open service control manager (maybe remote...)
        m_hSCMgr = OpenSCManager( m_CurrentHostName,	 
            "ServicesActive", SC_MANAGER_ALL_ACCESS ) ;
        if(m_hSCMgr == NULL)
        {
            CError::ErrorMsg() ;
            return 0 ;
        }
    }

    // if an aceservice handle is open but
    // the specified service is different from previously
    // then close the current service first
    if( m_hAceService != NULL &&
        m_CurrentServiceName.CompareNoCase(lpConfigPS->Database()->ServiceName()) )
    {
        if( !CloseServiceHandle(m_hAceService) )
        {
            CError::ErrorMsg() ;
            return 0 ;
        }
        else m_hAceService = NULL ;
    }

    // if no service is open, attempt to open?...
    if( m_hAceService == NULL )
    {
        m_CurrentServiceName = lpConfigPS->Database()->ServiceName() ;
        m_hAceService =
            OpenService( m_hSCMgr, 
                         m_CurrentServiceName,
                         SERVICE_ALL_ACCESS ) ;

        if( m_hAceService == NULL  && 
            GetLastError() != ERROR_SERVICE_DOES_NOT_EXIST )
        {
            CError::ErrorMsg() ;
            return 0 ;
        }
    }

    if( !CPropertyPage::OnSetActive() ) return FALSE ;

    // Modify Property Page display to reflect status...
    CString str ;
    if( m_hAceService == NULL )
    {
        // New service?
	m_Existing_Service_Group.ShowWindow(SW_HIDE);
	m_Existing_Service_Ctl.ShowWindow(SW_HIDE);
	m_Delete_Service_Ctl.ShowWindow(SW_HIDE);
        str.LoadString(IDS_NEW_SERVICE) ;
        SetDlgItemText(IDC_SERVICE_STATUS_MSG, str) ;
    }
    else
    {
        // Existing service?
	m_Existing_Service_Group.ShowWindow(SW_SHOW);
	m_Existing_Service_Ctl.ShowWindow(SW_SHOW);
	m_Delete_Service_Ctl.ShowWindow(SW_SHOW);
        str.LoadString(IDS_EXISTING_SERVICE) ;
        SetDlgItemText(IDC_SERVICE_STATUS_MSG, str) ;
    }

    return TRUE ;
}

BOOL CServiceStatusPP::AceServiceDeleted()
{
    CConfigPS* lpConfigPS = (CConfigPS*)GetParent() ;

    CString msg = "Delete the current AceServer service from the system?" ;
    if( AfxMessageBox( msg, MB_OKCANCEL|MB_ICONQUESTION|MB_DEFBUTTON2 ) == IDOK)
    {
        // Proceed with deletion...
        // *** NOTE: If the service is currently running (unlikely in this case)
        // then the service will only be fully deleted once it stops running...
        if( DeleteService(m_hAceService) &&
              CloseServiceHandle(m_hAceService) )
        {    
            // Deletes current ServiceKey data?
            lpConfigPS->theRegistry.DeleteServerData() ; 

            TRACE("Successfully deleted current AceServer service!") ;
            AfxMessageBox( 
                TEXT("Specified AceServer service successfully deleted from the system."),
                MB_OK|MB_ICONINFORMATION );

            m_hAceService = NULL ;
            return TRUE ;
        }
        else
        {
            CError::ErrorMsg() ;
            return FALSE ;
        }
    }
    else
        return FALSE ;
}

LRESULT CServiceStatusPP::OnWizardNext() 
{
    TRACE("CServiceStatusPP::OnWizardNext()\n") ;

    // Retrieve the data?
    UpdateData() ;

    if( m_hAceService == NULL )
    {
        // Register new service?
        return CServiceRegistryPP::IDD ;
    }
    else
    {
        // Existing service?
        // Do I edit this service or delete it?
        switch(m_Existing_Service)
        {
            case 0 :  // Edit?
                return CServiceRegistryPP::IDD ;
            case 1:   // Delete!
                if( AceServiceDeleted() )
                    return CCompletionPP::IDD ;
                else
                    return -1L ;
            default:
                ASSERT(FALSE) ; // program error?
        }
    }
    return CPropertyPage::OnWizardNext();
}

void CServiceStatusPP::OnPaint() 
{
	CPaintDC dc(this); // device context for painting

        CConfigPS* lpConfigPS = (CConfigPS*)GetParent() ;

	CStatic *pMsg = (CStatic*)GetDlgItem(IDC_SERVICE_NAME) ;
        ASSERT_VALID(pMsg) ;
        pMsg->SetFont(&m_fntServiceName) ;

        CString str = lpConfigPS->Database()->ServiceName() ;
        str += "@" ;
        str += !*(lpConfigPS->Host()->HostName())?
                        "LocalHost" :
                        lpConfigPS->Host()->HostName() ;
        SetDlgItemText( IDC_SERVICE_NAME, 
                        str) ;

        pMsg = (CStatic*)GetDlgItem(IDC_SERVICE_STATUS_MSG) ;
        ASSERT_VALID(pMsg) ;
        pMsg->SetFont(&lpConfigPS->m_fntMessage) ;
}

BOOL CServiceStatusPP::OnHelpInfo(HELPINFO* pHelpInfo) 
{
    return win32FileCall( "Status.html" ) ;
}
