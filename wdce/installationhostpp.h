// InstallationHostPP.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CInstallationHostPP dialog

class CInstallationHostPP : public CPropertyPage
{
	DECLARE_DYNCREATE(CInstallationHostPP)

private:
        CString m_Account ; 

// Construction
public:
	CInstallationHostPP();
	~CInstallationHostPP();

// Dialog Data
	//{{AFX_DATA(CInstallationHostPP)
	enum { IDD = IDD_INSTALLATION_TARGET };
	CEdit	m_Domain_Name_Edit_Ctl;
	CButton	m_Local_Host_Ctl;
	CEdit	m_User_Name_Edit_Ctl;
	CEdit	m_Remote_Host_Name_Edit_Ctl;
	CEdit	m_Password_Edit_Ctl;
	CString	m_Remote_Host_Name;
	CString	m_Password;
	CString	m_User_Name;
	int		m_Selected_Host;
	CString	m_Domain_Name;
	CString	m_Database_Name;
	//}}AFX_DATA


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CInstallationHostPP)
	public:
	virtual LRESULT OnWizardNext();
	virtual BOOL OnSetActive();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
public:
        LPCTSTR     Database() { return m_Database_Name ; }
        LPCTSTR     HostName() { return m_Remote_Host_Name ; }
        LPCTSTR     Account() ;
        LPCTSTR     Password() ;

protected:
	// Generated message map functions
	//{{AFX_MSG(CInstallationHostPP)
	virtual BOOL OnInitDialog();
	afx_msg void OnLocalHost();
	afx_msg void OnRemoteHost();
	afx_msg void OnPaint();
	afx_msg BOOL OnHelpInfo(HELPINFO* pHelpInfo);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};
