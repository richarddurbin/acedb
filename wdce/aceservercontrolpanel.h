// AceServerControlPanel.h : main header file for the ACESERVERCONTROLPANEL DLL
//

#ifndef __AFXWIN_H__
	#error include 'stdafx.h' before including this file for PCH
#endif

#include "..\wdce\resource.h"		// main symbols are AS Control Panel specific

// Configuration dialog property sheet and pages
extern "C" {
#include "regular.h" /* for ErrMsg() */
}
#include "Registry.h"
#include "Connection.h"
#include "ServerRegistry.h"
#include "IntroductionPP.h"
#include "InstallationHostPP.h"
#include "DatabasePP.h"
#include "ServiceStatusPP.h"
#include "ServiceRegistryPP.h"
#include "AceServerConfigPP.h"
#include "ConnectionsPP.h"
#include "CompletionPP.h"      
#include "ConfigPS.h"

/////////////////////////////////////////////////////////////////////////////
// CAceServerControlPanelApp
// See AceServerControlPanel.cpp for the implementation of this class
//

class CAceServerControlPanelApp : public CWinApp
{
private:
    // Attributes
    CConfigPS              *m_ConfigWizard ;

public:
    // Constructor        
    CAceServerControlPanelApp();

    // Operations
    BOOL InitDialog() ;
    BOOL DisplayDialog() ;
    void DestroyDialog() ;

// Overrides
    // ClassWizard generated virtual function overrides
    //{{AFX_VIRTUAL(CAceServerControlPanelApp)
    //}}AFX_VIRTUAL

    //{{AFX_MSG(CAceServerControlPanelApp)
	    // NOTE - the ClassWizard will add and remove member functions here.
	    //    DO NOT EDIT what you see in these blocks of generated code !
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////
