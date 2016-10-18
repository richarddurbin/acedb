/* ConnectionsPP.h : header file
 *  Author: Richard Bruskiewich (rbrusk@octogene.medgen.ubc.ca)
 *  Copyright (C) R. Bruskiewich, J Thierry-Mieg and R Durbin, 1998
 *-------------------------------------------------------------------
 * This file is part of the ACEDB genome database package, written by
 * 	Richard Durbin (MRC LMB, UK) rd@mrc-lmb.cam.ac.uk, and
 *	Jean Thierry-Mieg (CRBM du CNRS, France) mieg@kaa.cnrs-mop.fr
 *
 * Description:      Microsoft Windows DCE AceServer Configuration Program   
 *                   Connections Property page management
 *
 * HISTORY:
 * Last edited: Jan 16 10:08 1998 (rbrusk)
 * Created: Jan 15 04:02 1998 (rbrusk)
 *-------------------------------------------------------------------
 */

class CConnectionsPP : public CPropertyPage
{
	DECLARE_DYNCREATE(CConnectionsPP)

private:
        CConnection m_Default_Named_Pipe, 
                    m_Default_TCP_IP,  
                    m_Default_NetBEUI, 
                    m_Default_Custom ;

public:
// Construction
	CConnectionsPP() ;

// Operations
        void ResetDialog() ;

// Dialog Data
	//{{AFX_DATA(CConnectionsPP)
	enum { IDD = IDD_CONNECTIONS };
	CString	m_Named_Pipe_Endpoint;
	BOOL	m_Named_Pipe_Enabled;
	BOOL	m_NetBEUI_Enabled;
	UINT	m_TCP_IP_Port;
	BOOL	m_TCP_IP_Enabled;
	CString	m_Custom_Protocol;
	CString	m_Custom_Endpoint;
	BOOL	m_Custom_Protocol_Enabled;
	short	m_NetBEUI_Port;
	//}}AFX_DATA

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CConnectionsPP)
	public:
	virtual LRESULT OnWizardNext();
	virtual BOOL OnSetActive();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CConnectionsPP)
	afx_msg void OnReset();
	afx_msg void OnPaint();
	afx_msg BOOL OnHelpInfo(HELPINFO* pHelpInfo);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

// Minimum "safe" default port numbers for TCP/IP and NetBEUI
#define MIN_TCPIP_PORT_NO     11000
#define MIN_TCPIP_PORT_STR   "11000"
#define MIN_NETBEUI_PORT_NO   32
#define MIN_NETBEUI_PORT_STR "32"

