// ConfigPS.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CConfigPS

class CConfigPS : public CPropertySheet
{
    DECLARE_DYNAMIC(CConfigPS)

// Construction
public:
    CConfigPS(UINT nIDCaption, CWnd* pParentWnd = NULL, UINT iSelectPage = 0);
    CConfigPS(LPCTSTR pszCaption, CWnd* pParentWnd = NULL, UINT iSelectPage = 0);

// Attributes
private:
    CIntroductionPP         *m_Introduction_Page ;
    CInstallationHostPP     *m_Installation_Host_Page ;
    CDatabasePP             *m_Database_Description_Page ;
    CServiceStatusPP        *m_Service_Status_Page ;
    CServiceRegistryPP      *m_Service_Registry_Page ;
    CAceServerConfigPP      *m_AceServerConfig_Page ;
    CConnectionsPP          *m_Connections_Page ;
    CCompletionPP           *m_Completion_Page ;

public:
// Operations
    CServerRegistry         theRegistry ;
    CInstallationHostPP     *Host() { return m_Installation_Host_Page ; }
    CDatabasePP             *Database() { return m_Database_Description_Page ; }
    CServiceStatusPP        *Configuration() { return m_Service_Status_Page ; }

private:
    BOOL InitSheet() ; 

public:
        CFont m_fntMessage ;

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CConfigPS)
	public:
	virtual BOOL OnInitDialog();
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CConfigPS();

	// Generated message map functions
protected:
	//{{AFX_MSG(CConfigPS)
		// NOTE - the ClassWizard will add and remove member functions here.
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

#define MESSAGE_FONT_SIZE 20
