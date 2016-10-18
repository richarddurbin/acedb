// IntroductionPP.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CIntroductionPP dialog

class CIntroductionPP : public CPropertyPage
{
	DECLARE_DYNCREATE(CIntroductionPP)

// Construction
public:
	CIntroductionPP();
	~CIntroductionPP();

// Dialog Data
	//{{AFX_DATA(CIntroductionPP)
	enum { IDD = IDD_INTRO };
		// NOTE - ClassWizard will add data members here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_DATA


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CIntroductionPP)
	public:
	virtual BOOL OnSetActive();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
        CFont m_fntTitle, m_fntCopyright ;

	// Generated message map functions
	//{{AFX_MSG(CIntroductionPP)
	virtual BOOL OnInitDialog();
	afx_msg void OnPaint();
	afx_msg BOOL OnHelpInfo(HELPINFO* pHelpInfo);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};

#define TITLE_FONT_SIZE 24
