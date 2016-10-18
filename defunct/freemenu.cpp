// $Id: freemenu.cpp,v 1.1 1996-06-10 10:35:34 rbsk Exp $
// freemenu.cpp : implementation file
//

#include "stdafx.h"
#include "win32.h"
#include "WinAce.h"

extern "C"
{
#include "regular.h"
}

#include "CGraph.h"

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

#define new DEBUG_NEW

/////////////////////////////////////////////////////////////////////////////
// CFreeMenu dialog

CFreeMenu::CFreeMenu( FreeMenuFunction proc )
	: CDialog(CFreeMenu::IDD, /* pParent == */ NULL)
{
	//{{AFX_DATA_INIT(CFreeMenu)
	m_menuSelection = _T("");
	//}}AFX_DATA_INIT

	m_selectedMenuItem = 0 ;
	m_function = proc ;
}

CFreeMenu::~CFreeMenu()
{
	for(POSITION pos = m_menuMap.GetHeadPosition() ; pos != NULL; )
	{
		CMenuMapItem *mmi = m_menuMap.GetNext( pos ) ;
		if( mmi != NULL ) delete mmi ;
	}
}


void CFreeMenu::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CFreeMenu)
	DDX_Control(pDX, IDC_FREEOPTS, m_menuItems);
	DDX_LBString(pDX, IDC_FREEOPTS, m_menuSelection);
	//}}AFX_DATA_MAP
}

void CFreeMenu::AddFreeMenuItem( UINT menuItemID, char *menuText )
{
	TRY
	{
		m_menuMap.AddTail( new CMenuMapItem( menuItemID, menuText ) ) ;		
	}
	CATCH( CMemoryException, e )
	{
		messcrash("CFreeMenu: out of memory exception?") ;
	}
	END_CATCH
}

BEGIN_MESSAGE_MAP(CFreeMenu, CDialog)
	//{{AFX_MSG_MAP(CFreeMenu)
	ON_LBN_DBLCLK(IDC_FREEOPTS, OnDblClkMenuItem)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

#ifdef _DEBUG

void CMenuMapItem::Dump( CDumpContext &dc ) const
{
	dc	<< "\n*** Start of CMenuMapItem Dump *** \n" ; 
	CObject::Dump( dc ) ;

	dc	<< "\tMenuItemID: " <<  m_menuItemID << "\n"
		<< "\tMenuItemText:	" <<  m_menuItemText << "\n"
		<< "\n*** End of CMenuMapItem Dump *** \n" ;
}

void CFreeMenu::Dump( CDumpContext &dc ) const
{
	CDialog::Dump( dc ) ;

	dc	<< "\n*** Start of CFreeMenu Dump *** \n" 
		<< "\n\tMenu Map: \n" ;
	
	m_menuMap.Dump( dc ) ;

	dc 	<< "\n\tCurrent Selected Menu Item: " << m_selectedMenuItem 
		<< "\n\tFreeMenuFunction Pointer: " << m_function
		<< "\n*** End of CFreeMenu Dump *** \n" ;
}

#endif


/////////////////////////////////////////////////////////////////////////////
// CFreeMenu message handlers

BOOL CFreeMenu::OnInitDialog() 
{
	CDialog::OnInitDialog();

	for(POSITION pos = m_menuMap.GetHeadPosition() ; pos != NULL; )
	{
		CMenuMapItem *mmi = m_menuMap.GetNext( pos ) ;
		int menuItemIdx = m_menuItems.AddString( (LPCSTR) mmi->GetMenuItemText() ) ;
		if( menuItemIdx == LB_ERR )
			messcrash("Freemenu creation error?") ;
		else if( menuItemIdx == LB_ERRSPACE ) 
			messcrash("Freemenu creation error: Out of String Space?") ;
		else m_menuItems.SetItemData( menuItemIdx, (DWORD) mmi->GetMenuItemID() ) ;
	}

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CFreeMenu::OnOK() 
{
	int menuItemIdx = m_menuItems.GetCurSel() ;
	if( menuItemIdx == LB_ERR )
		m_selectedMenuItem = 0 ;
	else
	{
		DWORD menuItemID = m_menuItems.GetItemData( menuItemIdx ) ;
		if( menuItemID == LB_ERR )
			m_selectedMenuItem = 0 ;
		else 
			m_selectedMenuItem = (UINT) menuItemID ;
	}

	CDialog::OnOK();
}


void CFreeMenu::OnDblClkMenuItem() 
{
	OnOK() ;	
}
 
