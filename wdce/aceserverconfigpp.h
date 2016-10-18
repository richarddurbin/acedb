#ifndef ACESERVERCONFIGPP_H
#define ACESERVERCONFIGPP_H


// AceServerConfigPP.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CAceServerConfigPP dialog

class CAceServerConfigPP : public CPropertyPage
{
	DECLARE_DYNCREATE(CAceServerConfigPP)

// Construction
public:
	CAceServerConfigPP();
	~CAceServerConfigPP();

// Dialog Data
	//{{AFX_DATA(CAceServerConfigPP)
	enum { IDD = IDD_ACESERVER_CONFIGURATION };
	UINT	m_AutoSave_Interval;
	UINT	m_Client_Timeout;
	UINT	m_Max_Kilobytes;
	UINT	m_Server_Timeout;
	//}}AFX_DATA

// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CAceServerConfigPP)
	public:
	virtual LRESULT OnWizardNext();
	virtual BOOL OnSetActive();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
        void ResetDialog() ;
	UINT	m_Default_AutoSave_Interval;
	UINT	m_Default_Client_Timeout;
	UINT	m_Default_Max_Kilobytes;
	UINT	m_Default_Server_Timeout;

	// Generated message map functions
	//{{AFX_MSG(CAceServerConfigPP)
	afx_msg void OnReset();
	afx_msg void OnPaint();
	afx_msg BOOL OnHelpInfo(HELPINFO* pHelpInfo);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

#endif /* #define ACESERVERCONFIGPP_H */
