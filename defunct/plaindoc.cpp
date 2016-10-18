// $Id: plaindoc.cpp,v 1.1 1996-06-10 10:36:16 rbsk Exp $
// PlainDoc.cpp : implementation of the CPlainGraphDoc class
//

#include "stdafx.h"
#include "win32.h"
#include "WinAce.h"
#include "CGraph.h"
#include "PlainDoc.h"

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CPlainGraphDoc

IMPLEMENT_DYNCREATE(CPlainGraphDoc, CGraph)

BEGIN_MESSAGE_MAP(CPlainGraphDoc, CGraph)
	//{{AFX_MSG_MAP(CPlainGraphDoc)
		// NOTE - the ClassWizard will add and remove mapping macros here.
		//    DO NOT EDIT what you see in these blocks of generated code!
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CPlainGraphDoc construction/destruction

CPlainGraphDoc::CPlainGraphDoc()
{
	// TODO: add one-time construction code here

}

CPlainGraphDoc::~CPlainGraphDoc()
{
}

BOOL CPlainGraphDoc::OnNewDocument()
{
	if (!CGraph::OnNewDocument())
		return FALSE;

	// TODO: add reinitialization code here
	// (SDI documents will reuse this document)

	return TRUE;
}

/////////////////////////////////////////////////////////////////////////////
// CPlainGraphDoc serialization

void CPlainGraphDoc::Serialize(CArchive& ar)
{
	if (ar.IsStoring())
	{
		// TODO: add storing code here
	}
	else
	{
		// TODO: add loading code here
	}
}

/////////////////////////////////////////////////////////////////////////////
// CPlainGraphDoc diagnostics

#ifdef _DEBUG
void CPlainGraphDoc::AssertValid() const
{
	CGraph::AssertValid();
}

void CPlainGraphDoc::Dump(CDumpContext& dc) const
{
	CGraph::Dump(dc);
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CPlainGraphDoc commands
 
