// ASInstall.h : main header file for the ASINSTALL application
//

#ifndef __AFXWIN_H__
	#error include 'stdafx.h' before including this file for PCH
#endif

#include "..\wdce\resource.h"		// main symbols from wdce

/////////////////////////////////////////////////////////////////////////////
// CASInstallApp:
// See ASInstall.cpp for the implementation of this class
//

class CASInstallApp : public CWinApp
{
public:
	CASInstallApp();

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CASInstallApp)
	public:
	virtual BOOL InitInstance();
	//}}AFX_VIRTUAL

// Implementation

	//{{AFX_MSG(CASInstallApp)
		// NOTE - the ClassWizard will add and remove member functions here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};


/////////////////////////////////////////////////////////////////////////////
