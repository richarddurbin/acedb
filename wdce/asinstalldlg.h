// ASInstallDlg.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CASInstallDlg dialog

class CASInstallDlg : public CDialog
{
// Construction
public:
	CASInstallDlg(CWnd* pParent = NULL);	// standard constructor

// Dialog Data
	//{{AFX_DATA(CASInstallDlg)
	enum { IDD = IDD_ASINSTALL_DIALOG };
		// NOTE: the ClassWizard will add data members here
	//}}AFX_DATA

	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CASInstallDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	HICON m_hIcon;

	// Generated message map functions
	//{{AFX_MSG(CASInstallDlg)
	virtual BOOL OnInitDialog();
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};
