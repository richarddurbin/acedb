// CompletionPP.cpp : implementation file
//

#include "stdafx.h"
#include "AceServerControlPanel.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CCompletionPP property page

IMPLEMENT_DYNCREATE(CCompletionPP, CPropertyPage)

CCompletionPP::CCompletionPP() : CPropertyPage(CCompletionPP::IDD)
{
	//{{AFX_DATA_INIT(CCompletionPP)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}

CCompletionPP::~CCompletionPP()
{
}

void CCompletionPP::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CCompletionPP)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CCompletionPP, CPropertyPage)
	//{{AFX_MSG_MAP(CCompletionPP)
	ON_WM_PAINT()
	ON_WM_HELPINFO()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CCompletionPP message handlers

BOOL CCompletionPP::OnSetActive() 
{
    // Display the Back & Finish Button
    CPropertySheet* pParent = (CPropertySheet*)GetParent() ;
    pParent->SetWizardButtons(PSWIZB_BACK | PSWIZB_FINISH) ;

    return CPropertyPage::OnSetActive();
}

BOOL CCompletionPP::OnWizardFinish() 
{
    CConfigPS* lpConfigPS = (CConfigPS*)GetParent() ;

    lpConfigPS->theRegistry.Close() ;

    CPropertySheet* pParent = (CPropertySheet*)GetParent() ;
    // PropSheet_RestartWindows(pParent->m_psh.hwndParent) ;
	
    return CPropertyPage::OnWizardFinish();
}

BOOL CCompletionPP::OnInitDialog() 
{
    CPropertyPage::OnInitDialog();
    
    m_fntText.CreateFont(MESSAGE_FONT_SIZE,0,0,0,FW_BOLD,FALSE,FALSE,
    0,ANSI_CHARSET, OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,
    DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE,"MS Sans Serif") ;

    return TRUE;  // return TRUE unless you set the focus to a control
	          // EXCEPTION: OCX Property Pages should return FALSE
}

void CCompletionPP::OnPaint() 
{
    CPaintDC dc(this); // device context for painting
    
    CStatic* pMsg = (CStatic*)GetDlgItem(IDC_COMPLETION_MESSAGE) ;
    ASSERT_VALID(pMsg) ;
    pMsg->SetFont(&m_fntText) ;

    CString str ;
    str.LoadString(IDS_COMPLETION_MESSAGE) ;
    SetDlgItemText(IDC_COMPLETION_MESSAGE, str) ;

    pMsg = (CStatic*)GetDlgItem(IDC_EXTRA_INSTRUCTIONS) ;
    ASSERT_VALID(pMsg) ;
    CConfigPS* lpConfigPS = (CConfigPS*)GetParent() ;
    pMsg->SetFont(&lpConfigPS->m_fntMessage) ;

    // Do not call CPropertyPage::OnPaint() for painting messages
}

LRESULT CCompletionPP::OnWizardBack() 
{
    return CInstallationHostPP::IDD ;  // Go back to installation host page...
}

BOOL CCompletionPP::OnHelpInfo(HELPINFO* pHelpInfo) 
{
    return win32FileCall( "RunServer.html" ) ;
}
