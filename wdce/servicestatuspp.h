// ServiceStatus.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CServiceStatusPP dialog

class CServiceStatusPP : public CPropertyPage
{
	DECLARE_DYNCREATE(CServiceStatusPP)

private:
    // Attributes
    CString     m_CurrentHostName ;
    SC_HANDLE   m_hSCMgr ;
    CString     m_CurrentServiceName ;
    SC_HANDLE   m_hAceService ;

public:
//  Construction
    CServiceStatusPP();
    ~CServiceStatusPP();


// Dialog Data
    //{{AFX_DATA(CServiceStatusPP)
	enum { IDD = IDD_SERVICE_STATUS };
	CButton	m_Existing_Service_Group;
	CButton	m_Existing_Service_Ctl;
	CButton	m_Delete_Service_Ctl;
        int	m_Existing_Service;
	//}}AFX_DATA


// Overrides
    // ClassWizard generate virtual function overrides
    //{{AFX_VIRTUAL(CServiceStatusPP)
	public:
        virtual BOOL OnSetActive();
	virtual LRESULT OnWizardNext();
	protected:
        virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
public:
    SC_HANDLE   Manager()         const { return m_hSCMgr ; }
    SC_HANDLE   Service()    const { return m_hAceService ; }
    void        Service(SC_HANDLE   hAceService) 
        { ASSERT(hAceService != NULL) ; m_hAceService = hAceService ; }

protected:

    CFont m_fntServiceName ;
    // Generated message map functions
    //{{AFX_MSG(CServiceStatusPP)
	afx_msg void OnPaint();
	virtual BOOL OnInitDialog();
	afx_msg BOOL OnHelpInfo(HELPINFO* pHelpInfo);
	//}}AFX_MSG
    DECLARE_MESSAGE_MAP()

    BOOL AceServiceDeleted() ;
};
