/* $Id: pixelfitdoc.h,v 1.1 1996-06-10 11:33:12 rbsk Exp $ */
// PixelFitDoc.h : interface of the CPixelFitGraphDoc class
//
/////////////////////////////////////////////////////////////////////////////

class CPixelFitGraphDoc : public CGraph
{
protected: // create from serialization only
	CPixelFitGraphDoc();
	DECLARE_DYNCREATE(CPixelFitGraphDoc)

// Attributes
public:

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CPixelFitGraphDoc)
	public:
	virtual BOOL OnNewDocument();
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CPixelFitGraphDoc();
	virtual void Serialize(CArchive& ar);   // overridden for document i/o
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

protected:

// Generated message map functions
protected:
	//{{AFX_MSG(CPixelFitGraphDoc)
		// NOTE - the ClassWizard will add and remove member functions here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////
 
