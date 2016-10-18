// $Id: pixelfitdoc.cpp,v 1.1 1996-06-10 10:36:08 rbsk Exp $
// PixelFitDoc.cpp : implementation of the CPixelFitGraphDoc class
//

#include "stdafx.h"
#include "win32.h"
#include "WinAce.h"
#include "CGraph.h"
#include "PixelFitDoc.h"

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CPixelFitGraphDoc

IMPLEMENT_DYNCREATE(CPixelFitGraphDoc, CGraph)

BEGIN_MESSAGE_MAP(CPixelFitGraphDoc, CGraph)
	//{{AFX_MSG_MAP(CPixelFitGraphDoc)
		// NOTE - the ClassWizard will add and remove mapping macros here.
		//    DO NOT EDIT what you see in these blocks of generated code!
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CPixelFitGraphDoc construction/destruction

CPixelFitGraphDoc::CPixelFitGraphDoc()
{
	// TODO: add one-time construction code here

}

CPixelFitGraphDoc::~CPixelFitGraphDoc()
{
}

BOOL CPixelFitGraphDoc::OnNewDocument()
{
	if (!CGraph::OnNewDocument())
		return FALSE;

	// TODO: add reinitialization code here
	// (SDI documents will reuse this document)

	return TRUE;
}

/////////////////////////////////////////////////////////////////////////////
// CPixelFitGraphDoc serialization

void CPixelFitGraphDoc::Serialize(CArchive& ar)
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
// CPixelFitGraphDoc diagnostics

#ifdef _DEBUG
void CPixelFitGraphDoc::AssertValid() const
{
	CGraph::AssertValid();
}

void CPixelFitGraphDoc::Dump(CDumpContext& dc) const
{
	CGraph::Dump(dc);
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CPixelFitGraphDoc commands
 
