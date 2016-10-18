// ServiceRegistryPP.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CServiceRegistryPP dialog

class CServiceRegistryPP : public CPropertyPage
{
	DECLARE_DYNCREATE(CServiceRegistryPP)

        // Construction
public:
	CServiceRegistryPP();
	~CServiceRegistryPP();

// Dialog Data
	//{{AFX_DATA(CServiceRegistryPP)
	enum { IDD = IDD_SERVICE_REGISTRY };
	CEdit	m_Service_DisplayName_Ctl;
	CEdit	m_AceServer_Pathname_Ctl;
	int		m_Service_Mode ;
	CString	m_AceServer_Pathname;
	int		m_Installation_Activity;
	CString	m_Service_DisplayName;
	//}}AFX_DATA


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CServiceRegistryPP)
	public:
	virtual BOOL OnSetActive();
	virtual LRESULT OnWizardNext();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CServiceRegistryPP)
	virtual BOOL OnInitDialog();
	afx_msg void OnPaint();
	afx_msg void OnAceserverBrowse();
	afx_msg BOOL OnHelpInfo(HELPINFO* pHelpInfo);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};
