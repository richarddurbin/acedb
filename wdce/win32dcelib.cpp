
/*  File: win32dcelib.cpp
 *  Author: Richard Bruskiewich (rbrusk@octogene.medgen.ubc.ca)
 *  Copyright (C) J Thierry-Mieg and R Durbin, 1992
 *-------------------------------------------------------------------
 * This file is part of the ACEDB genome database package, written by
 * 	Richard Durbin (MRC LMB, UK) rd@mrc-lmb.cam.ac.uk, and
 *	Jean Thierry-Mieg (CRBM du CNRS, France) mieg@kaa.cnrs-mop.fr
 *
 * Description:
 * Distributed Computing Environment (DCE) RPC 
 *	WIN32 port specific function definitions
 * 
 * Exported functions:
 *	 void my_Signal( void (*callback)(int arg) )
 *	 void my_Alarm(unsigned int seconds)
 *
 * HISTORY:
 * Last edited: August 1 21:50 1997 (rbrusk): fully implemented
 * * Feb 12 20:10 1997 (rbrusk)
 * Created: Feb 12 20:10 1997 (rbrusk): with stubs
 ***************************************************************/

/* %W% %G%	 */

#include "stdafx.h"
#include "dceServerTimer.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/* These routines simulate the UNIX
 * SIGALRM functionality for the aceserver, using threads...
 */

/* my_Signal just sets the callback */

extern "C" void my_Signal( SIGNAL_CALLBACK callback )
{
	CTimer *stopWatch = CTimer::getStopWatch() ;
	if( stopWatch != NULL )
		stopWatch->dceAlarmCBSet(callback) ;
}

/* my_Alarm() sets a SIGALRM; calls the signal(callback)
   when it times out, unless otherwise reset again */
extern "C" void my_Alarm(unsigned int seconds)
{
	CTimer *stopWatch = CTimer::getStopWatch() ;
	if( stopWatch != NULL )
		stopWatch->dceAlarmStart(seconds) ;
}

/////////////////////////////////////////////////////////////////////////////
// CDCETimer

IMPLEMENT_DYNCREATE(CDCETimer, CWinThread)

CDCETimer *CDCETimer::stopWatch = NULL ;

// the CDCETimer creating processes grabs the m_waitFlag CMutex
// so the CDCETimer can time out by waiting for it, upon request
// for time == m_timeOut seconds
CDCETimer::CDCETimer() : m_waitFlag(TRUE) 
{
	m_callback = NULL ;
	m_timeOut = 0 ;
}

CDCETimer::~CDCETimer()
{
}

BOOL CDCETimer::InitInstance()
{
	// TODO:  perform and per-thread initialization here
	return TRUE;
}

int CDCETimer::ExitInstance()
{
	// TODO:  perform any per-thread cleanup here
	return CWinThread::ExitInstance();
}

BEGIN_MESSAGE_MAP(CDCETimer, CWinThread)
	//{{AFX_MSG_MAP(CDCETimer)
        ON_THREAD_MESSAGE( WM_CREATE, OnAlarm )
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CDCETimer message handlers

CDCETimer *CDCETimer::getStopWatch()
{
    TRACE("Entering getStopWatch()\n") ;
    if( stopWatch == NULL)
    {
        /* CREATE_SUSPENDED in order to provide for parameter passing */
        stopWatch = 
			(CDCETimer *)AfxBeginThread( RUNTIME_CLASS(CDCETimer),
										THREAD_PRIORITY_NORMAL,
										256 /* 256 bytes should be adequate? */,
										CREATE_SUSPENDED ) ;
        if( stopWatch == NULL)
		{
			TRACE("CDCETimer could not be created!?\nExiting getStopWatch()\n") ;
			ASSERT(FALSE) ;
			return NULL ;
		}
    }
	else
	{
		TRACE("Exiting getStopWatch()\n") ;
		return stopWatch ;
    }
}

void CDCETimer::dceAlarmStart(unsigned int seconds)
{
	if( seconds ) // set the alarm time duration in millisecs
		m_timeOut = seconds*1000 ;

	if( !(m_callback && m_timeOut) )  // both have to be non-NULL
	{
		errno = -1 ;
		messerror("CDCETimer thread has no callback or a zero time out?!?\n"
			  "dceAlarmStart() failure!\n") ;
		return ;
	}

	DWORD result ;
	while( result = ResumeThread() ) // until zero == not suspended?
	{
		if( result == 0xFFFFFFFF )
		{
			errno = -1 ;
			messerror("CDCETimer thread could not be resumed!?\n"
				  "dceAlarmStart() failure!\n") ;
			return ;
		}
	}
	m_ResetFlag.PulseEvent() ;
	PostThreadMessage(m_hThread,WM_CREATE,0,0) ;
}

// WM_CREATE mesage calls the OnAlarm() handler... see message map above
int CDCETimer::OnAlarm(WPARAM wParam, LPARAM lParam)
{
	CSingleLock waitFlagLock(m_waitFlag) ;
	CSyncObjects *flagArray[2] = { &m_waitFlag, &m_ResetFlag } ;
	CMultiLock sleep(flagArray,2) ;

	sleep.Lock((DWORD)m_timeOut*1000) ;

	// waitLock.IsLocked() is true if m_ResetFlag was signalled?
	if(!waitFlagLock.IsLocked() && m_callback)
	{
		((SIGNAL_CALLBACK)m_callback)(0) ;
		return 0 ;
	}
	else
		return -1 ;
}
