// dceServerTimer.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CDCETimer thread

typedef void (*SIGNAL_CALLBACK)(int arg) ;

class CDCETimer : public CWinThread
{
	DECLARE_DYNCREATE(CDCETimer)

protected:
	CDCETimer();           // protected constructor used by dynamic creation

	static CDCETimer *stopWatch ;
	CMutex m_waitFlag ;  // causes a block, for an "alarm" timeOut
	CEvent m_resetFlag ; // releases a block prematurely, if alarm() called
						 // before the previous alarm has timed out...
	SIGNAL_CALLBACK m_callback ;
	unsigned int	m_timeOut ;

// Attributes
public:
// Operations
public:
	static CDCETimer *getStopWatch() ;
	void dceAlarmCBSet(SIGNAL_CALLBACK callback) { m_callback = callback ; } ;
	void dceAlarmStart(unsigned int seconds) ;
}
// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CDCETimer)
	public:
	virtual BOOL InitInstance();
	virtual int ExitInstance();
	//}}AFX_VIRTUAL

// Implementation
protected:
	virtual ~CDCETimer();

	// Generated message map functions
	//{{AFX_MSG(CDCETimer)
        afx_msg void OnAlarm(WPARAM wParam, LPARAM lParam) ;
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()
};
/////////////////////////////////////////////////////////////////////////////
