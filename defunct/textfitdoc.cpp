// $Id: textfitdoc.cpp,v 1.1 1996-06-10 10:36:26 rbsk Exp $
// TextFitDoc.cpp : implementation of the CTextFitGraphDoc class
//

#include "stdafx.h"
#include "win32.h"
#include "WinAce.h"
#include "CGraph.h"
#include "TextFitDoc.h"

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CTextFitGraphDoc

IMPLEMENT_DYNCREATE(CTextFitGraphDoc, CGraph)

BEGIN_MESSAGE_MAP(CTextFitGraphDoc, CGraph)
	//{{AFX_MSG_MAP(CTextFitGraphDoc)
		// NOTE - the ClassWizard will add and remove mapping macros here.
		//    DO NOT EDIT what you see in these blocks of generated code!
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CTextFitGraphDoc construction/destruction

CTextFitGraphDoc::CTextFitGraphDoc()
{
	// TODO: add one-time construction code here

}

CTextFitGraphDoc::~CTextFitGraphDoc()
{
}

BOOL CTextFitGraphDoc::OnNewDocument()
{
	if (!CGraph::OnNewDocument())
		return FALSE;

	// TODO: add reinitialization code here
	// (SDI documents will reuse this document)

	return TRUE;
}

/////////////////////////////////////////////////////////////////////////////
// CTextFitGraphDoc serialization

void CTextFitGraphDoc::Serialize(CArchive& ar)
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
// CTextFitGraphDoc diagnostics

#ifdef _DEBUG
void CTextFitGraphDoc::AssertValid() const
{
	CGraph::AssertValid();
}

void CTextFitGraphDoc::Dump(CDumpContext& dc) const
{
	CGraph::Dump(dc);
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CTextFitGraphDoc commands
 
