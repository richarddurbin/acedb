// $Id: textscrolldoc.cpp,v 1.1 1996-06-10 10:36:30 rbsk Exp $
// TextScrollDoc.cpp : implementation of the CTextScrollGraphDoc class
//

#include "stdafx.h"
#include "win32.h"
#include "WinAce.h"
#include "CGraph.h"
#include "TextScrollDoc.h"

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CTextScrollGraphDoc

IMPLEMENT_DYNCREATE(CTextScrollGraphDoc, CGraph)

BEGIN_MESSAGE_MAP(CTextScrollGraphDoc, CGraph)
	//{{AFX_MSG_MAP(CTextScrollGraphDoc)
		// NOTE - the ClassWizard will add and remove mapping macros here.
		//    DO NOT EDIT what you see in these blocks of generated code!
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CTextScrollGraphDoc construction/destruction

CTextScrollGraphDoc::CTextScrollGraphDoc()
{
	// TODO: add one-time construction code here

}

CTextScrollGraphDoc::~CTextScrollGraphDoc()
{
}

BOOL CTextScrollGraphDoc::OnNewDocument()
{
	if (!CGraph::OnNewDocument())
		return FALSE;

	// TODO: add reinitialization code here
	// (SDI documents will reuse this document)

	return TRUE;
}

/////////////////////////////////////////////////////////////////////////////
// CTextScrollGraphDoc serialization

void CTextScrollGraphDoc::Serialize(CArchive& ar)
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
// CTextScrollGraphDoc diagnostics

#ifdef _DEBUG
void CTextScrollGraphDoc::AssertValid() const
{
	CGraph::AssertValid();
}

void CTextScrollGraphDoc::Dump(CDumpContext& dc) const
{
	CGraph::Dump(dc);
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CTextScrollGraphDoc commands
 
