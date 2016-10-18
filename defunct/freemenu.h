/* $Id: freemenu.h,v 1.1 1996-06-10 11:32:46 rbsk Exp $ */
// freemenu.h : header file
//

class CMenuMapItem : public CObject
{
private:

	UINT m_menuItemID ;
	CString m_menuItemText ;

public:
	CMenuMapItem( UINT menuItemID, CString menuText  )
	{
		m_menuItemID = menuItemID ;
		m_menuItemText = menuText ;
	}
	~CMenuMapItem() { m_menuItemText.Empty() ; }

	UINT GetMenuItemID() const { return m_menuItemID; }
	CString GetMenuItemText() const { return m_menuItemText; }

#ifdef _DEBUG
	virtual void Dump( CDumpContext &dc ) const ;
#endif

} ;

/////////////////////////////////////////////////////////////////////////////
// CFreeMenu dialog

class CFreeMenu :  public CDialog
{
// Construction
public:
	CFreeMenu(FreeMenuFunction proc );   // standard constructor
	~CFreeMenu() ; // Override destructor

// Attributes

private:
	CTypedPtrList<CObList,CMenuMapItem*> m_menuMap ;
	UINT m_selectedMenuItem ;
	FreeMenuFunction m_function ;	// The FreeMenu callback function?

// Operations

public:
	void AddFreeMenuItem( UINT menuItemID, char *menuText ) ;
	UINT GetSelectedMenuItem() const { return m_selectedMenuItem ; }
	FreeMenuFunction CallBack() const { return m_function ; }

#ifdef _DEBUG
	virtual void Dump( CDumpContext &dc ) const ;
#endif

// Dialog Data
	//{{AFX_DATA(CFreeMenu)
	enum { IDD = IDD_FREE_MENU };
	CListBox	m_menuItems;
	CString	m_menuSelection;
	//}}AFX_DATA

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CFreeMenu)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CFreeMenu)
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	afx_msg void OnDblClkMenuItem();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

 
