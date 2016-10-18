// DatabasePP.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CDatabasePP dialog

class CDatabasePP : public CPropertyPage
{
	DECLARE_DYNCREATE(CDatabasePP)

// Construction
public:
	CDatabasePP();
	~CDatabasePP();

// Dialog Data
	//{{AFX_DATA(CDatabasePP)
	enum { IDD = IDD_DATABASE_DESCRIPTION };
	CString	m_Service_Name;
	CString	m_ACEDB;
	CString	m_ACEDB_COMMON;
	CString	m_ACEDB_DATA;
	CString	m_Database_Value;
	CString	m_Database_Property;
	//}}AFX_DATA


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CDatabasePP)
	public:
	virtual LRESULT OnWizardNext();
	virtual BOOL OnSetActive();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

public:
// Implementation
        CString     ServiceName() { return m_Service_Name ; }

protected:
	// Generated message map functions
	//{{AFX_MSG(CDatabasePP)
	afx_msg void OnPaint();
	afx_msg void OnPropertySet();
	afx_msg BOOL OnHelpInfo(HELPINFO* pHelpInfo);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};
