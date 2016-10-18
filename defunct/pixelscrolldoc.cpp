// $Id: pixelscrolldoc.cpp,v 1.1 1996-06-10 10:36:12 rbsk Exp $
// PixelScrollDoc.cpp : implementation of the CPixelScrollGraphDoc class
//

#include "stdafx.h"
#include "win32.h"
#include "WinAce.h"
#include "CGraph.h"
#include "PixelScrollDoc.h"

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CPixelScrollGraphDoc

IMPLEMENT_DYNCREATE(CPixelScrollGraphDoc, CGraph)

BEGIN_MESSAGE_MAP(CPixelScrollGraphDoc, CGraph)
	//{{AFX_MSG_MAP(CPixelScrollGraphDoc)
		// NOTE - the ClassWizard will add and remove mapping macros here.
		//    DO NOT EDIT what you see in these blocks of generated code!
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CPixelScrollGraphDoc construction/destruction

CPixelScrollGraphDoc::CPixelScrollGraphDoc()
{
	// TODO: add one-time construction code here

}

CPixelScrollGraphDoc::~CPixelScrollGraphDoc()
{
}

BOOL CPixelScrollGraphDoc::OnNewDocument()
{
	if (!CGraph::OnNewDocument())
		return FALSE;

	// TODO: add reinitialization code here
	// (SDI documents will reuse this document)

	return TRUE;
}

/////////////////////////////////////////////////////////////////////////////
// CPixelScrollGraphDoc serialization

void CPixelScrollGraphDoc::Serialize(CArchive& ar)
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
// CPixelScrollGraphDoc diagnostics

#ifdef _DEBUG
void CPixelScrollGraphDoc::AssertValid() const
{
	CGraph::AssertValid();
}

void CPixelScrollGraphDoc::Dump(CDumpContext& dc) const
{
	CGraph::Dump(dc);
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CPixelScrollGraphDoc commands
 
