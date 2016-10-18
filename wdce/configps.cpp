// ConfigPS.cpp : implementation file
//

#include "stdafx.h"
#include "AceServerControlPanel.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CConfigPS

IMPLEMENT_DYNAMIC(CConfigPS, CPropertySheet)

CConfigPS::CConfigPS(UINT nIDCaption, CWnd* pParentWnd, UINT iSelectPage)
	:CPropertySheet(nIDCaption, pParentWnd, iSelectPage)
{
    VERIFY( InitSheet() ) ;
}

CConfigPS::CConfigPS(LPCTSTR pszCaption, CWnd* pParentWnd, UINT iSelectPage)
	:CPropertySheet(pszCaption, pParentWnd, iSelectPage)
{
    VERIFY( InitSheet() ) ;
}

CConfigPS::~CConfigPS()
{
    if(m_Introduction_Page != NULL)         delete m_Introduction_Page ;
    if(m_Installation_Host_Page != NULL)    delete m_Installation_Host_Page ;
    if(m_Database_Description_Page != NULL) delete m_Database_Description_Page ;
    if(m_Service_Status_Page != NULL)       delete m_Service_Status_Page ;
    if(m_Service_Registry_Page != NULL)     delete m_Service_Registry_Page ;
    if(m_AceServerConfig_Page != NULL)      delete m_AceServerConfig_Page ;
    if(m_Connections_Page != NULL)          delete m_Connections_Page ;
    if(m_Completion_Page != NULL)           delete m_Completion_Page ;
}


BEGIN_MESSAGE_MAP(CConfigPS, CPropertySheet)
	//{{AFX_MSG_MAP(CConfigPS)
		// NOTE - the ClassWizard will add and remove mapping macros here.
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CConfigPS message handlers

BOOL CConfigPS::InitSheet() 
{
    CError::ConsoleDisplay(TRUE) ;  // Display messages to user!

    if(( m_Introduction_Page =  new CIntroductionPP) == NULL)
        return FALSE ;
    m_Introduction_Page->Construct(IDD_INTRO,0) ;

    if((m_Installation_Host_Page =  new CInstallationHostPP) == NULL)
        return FALSE ;
    m_Installation_Host_Page->Construct(IDD_INSTALLATION_TARGET,0) ;

    if((m_Database_Description_Page =  new CDatabasePP) == NULL)
        return FALSE ;
    m_Database_Description_Page->Construct(IDD_DATABASE_DESCRIPTION,0) ;

    if((m_Service_Status_Page =  new CServiceStatusPP) == NULL)
        return FALSE ;
    m_Service_Status_Page->Construct(IDD_SERVICE_STATUS,0) ;

    if((m_Service_Registry_Page =  new CServiceRegistryPP) == NULL)
        return FALSE ;
    m_Service_Registry_Page->Construct(IDD_SERVICE_REGISTRY,0) ;

    if((m_AceServerConfig_Page =  new CAceServerConfigPP) == NULL)
        return FALSE ;
    m_AceServerConfig_Page->Construct(IDD_ACESERVER_CONFIGURATION,0) ;

    if((m_Connections_Page =  new CConnectionsPP) == NULL)
        return FALSE ;
    m_Connections_Page->Construct(IDD_CONNECTIONS,0) ;

    if((m_Completion_Page =  new CCompletionPP) == NULL)
        return FALSE ;
    m_Completion_Page->Construct(IDD_COMPLETION,0) ;

    AddPage(m_Introduction_Page) ;
    AddPage(m_Installation_Host_Page) ;
    AddPage(m_Database_Description_Page) ;
    AddPage(m_Service_Status_Page) ;
    AddPage(m_Service_Registry_Page) ;
    AddPage(m_AceServerConfig_Page) ;
    AddPage(m_Connections_Page) ;
    AddPage(m_Completion_Page) ;

    SetWizardMode() ;

    return TRUE ;
}


BOOL CConfigPS::OnInitDialog() 
{
    m_fntMessage.CreateFont(MESSAGE_FONT_SIZE,0,0,0,FW_MEDIUM,FALSE,FALSE,
    0,ANSI_CHARSET, OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,
    DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE,"MS Sans Serif") ;

    return CPropertySheet::OnInitDialog();
}
