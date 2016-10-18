// $Id: mapscrolldoc.cpp,v 1.1 1996-06-10 10:36:02 rbsk Exp $
// MapScrollDoc.cpp : implementation of the CMapScrollGraphDoc class
//

#include "stdafx.h"
#include "win32.h"
#include "WinAce.h"
#include "CGraph.h"
#include "MapScrollDoc.h"

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CMapScrollGraphDoc

IMPLEMENT_DYNCREATE(CMapScrollGraphDoc, CGraph)

BEGIN_MESSAGE_MAP(CMapScrollGraphDoc, CGraph)
	//{{AFX_MSG_MAP(CMapScrollGraphDoc)
		// NOTE - the ClassWizard will add and remove mapping macros here.
		//    DO NOT EDIT what you see in these blocks of generated code!
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CMapScrollGraphDoc construction/destruction

CMapScrollGraphDoc::CMapScrollGraphDoc()
{
	// TODO: add one-time construction code here

}

CMapScrollGraphDoc::~CMapScrollGraphDoc()
{
}

BOOL CMapScrollGraphDoc::OnNewDocument()
{
	if (!CGraph::OnNewDocument())
		return FALSE;

	// TODO: add reinitialization code here
	// (SDI documents will reuse this document)

	return TRUE;
}

/////////////////////////////////////////////////////////////////////////////
// CMapScrollGraphDoc serialization

void CMapScrollGraphDoc::Serialize(CArchive& ar)
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
// CMapScrollGraphDoc diagnostics

#ifdef _DEBUG
void CMapScrollGraphDoc::AssertValid() const
{
	CGraph::AssertValid();
}

void CMapScrollGraphDoc::Dump(CDumpContext& dc) const
{
	CGraph::Dump(dc);
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CMapScrollGraphDoc commands
 
