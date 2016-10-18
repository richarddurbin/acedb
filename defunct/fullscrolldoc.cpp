// $Id: fullscrolldoc.cpp,v 1.1 1996-06-10 10:35:40 rbsk Exp $
// FullScrollDoc.cpp : implementation of the CTextFullScrollGraphDoc class
//

#include "stdafx.h"
#include "win32.h"
#include "WinAce.h"
#include "CGraph.h"
#include "FullScrollDoc.h"

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CTextFullScrollGraphDoc

IMPLEMENT_DYNCREATE(CTextFullScrollGraphDoc, CGraph)

BEGIN_MESSAGE_MAP(CTextFullScrollGraphDoc, CGraph)
	//{{AFX_MSG_MAP(CTextFullScrollGraphDoc)
		// NOTE - the ClassWizard will add and remove mapping macros here.
		//    DO NOT EDIT what you see in these blocks of generated code!
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CTextFullScrollGraphDoc construction/destruction

CTextFullScrollGraphDoc::CTextFullScrollGraphDoc()
{
	// TODO: add one-time construction code here

}

CTextFullScrollGraphDoc::~CTextFullScrollGraphDoc()
{
}

BOOL CTextFullScrollGraphDoc::OnNewDocument()
{
	if (!CGraph::OnNewDocument())
		return FALSE;

	// TODO: add reinitialization code here
	// (SDI documents will reuse this document)

	return TRUE;
}

/////////////////////////////////////////////////////////////////////////////
// CTextFullScrollGraphDoc serialization

void CTextFullScrollGraphDoc::Serialize(CArchive& ar)
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
// CTextFullScrollGraphDoc diagnostics

#ifdef _DEBUG
void CTextFullScrollGraphDoc::AssertValid() const
{
	CGraph::AssertValid();
}

void CTextFullScrollGraphDoc::Dump(CDumpContext& dc) const
{
	CGraph::Dump(dc);
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CTextFullScrollGraphDoc commands
 
