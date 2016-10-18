// IntroductionPP.cpp : implementation file
//

#include "stdafx.h"
#include "AceServerControlPanel.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CIntroductionPP property page

IMPLEMENT_DYNCREATE(CIntroductionPP, CPropertyPage)

CIntroductionPP::CIntroductionPP() : CPropertyPage(CIntroductionPP::IDD)
{
	//{{AFX_DATA_INIT(CIntroductionPP)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}

CIntroductionPP::~CIntroductionPP()
{
}

void CIntroductionPP::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CIntroductionPP)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CIntroductionPP, CPropertyPage)
	//{{AFX_MSG_MAP(CIntroductionPP)
	ON_WM_PAINT()
	ON_WM_HELPINFO()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CIntroductionPP message handlers

BOOL CIntroductionPP::OnInitDialog() 
{
	CPropertyPage::OnInitDialog();
	
	m_fntTitle.CreateFont(TITLE_FONT_SIZE,0,0,0,FW_BOLD,FALSE,FALSE,
            0,ANSI_CHARSET, OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,
            DEFAULT_QUALITY,DEFAULT_PITCH | FF_DONTCARE,"MS Sans Serif") ;

        m_fntCopyright.CreateFont(MESSAGE_FONT_SIZE,0,0,0,FW_MEDIUM,TRUE,FALSE,
            0,ANSI_CHARSET, OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,
            DEFAULT_QUALITY,DEFAULT_PITCH | FF_DONTCARE,"MS Sans Serif") ;

        return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CIntroductionPP::OnPaint() 
{
	CPaintDC dc(this); // device context for painting

        CConfigPS* lpConfigPS = (CConfigPS*)GetParent() ;

        CStatic* pString = (CStatic*)GetDlgItem(IDC_TITLE) ;
        ASSERT_VALID(pString) ;
        pString->SetFont(&m_fntTitle) ;

	pString = (CStatic*)GetDlgItem(IDC_COPYRIGHT) ;
        ASSERT_VALID(pString) ;
        pString->SetFont(&m_fntCopyright) ;

	pString = (CStatic*)GetDlgItem(IDC_HELP_MESSAGE) ;
        ASSERT_VALID(pString) ;
        pString->SetFont(&lpConfigPS->m_fntMessage) ;

        CString str ;
        str.LoadString(IDS_TITLE) ;
        SetDlgItemText(IDC_TITLE, str) ;
        
        str.LoadString(IDS_COPYRIGHT) ;
        SetDlgItemText(IDC_COPYRIGHT, str) ;

        str.LoadString(IDS_HELP_MESSAGE) ;
        SetDlgItemText(IDC_HELP_MESSAGE, str) ;
}

BOOL CIntroductionPP::OnSetActive() 
{
    // Display the Back & Finish Button
    CPropertySheet* pParent = (CPropertySheet*)GetParent() ;
    pParent->SetWizardButtons(PSWIZB_NEXT) ;

    return CPropertyPage::OnSetActive();
}

BOOL CIntroductionPP::OnHelpInfo(HELPINFO* pHelpInfo) 
{
    return win32FileCall( "Introduction.html" ) ;

}
