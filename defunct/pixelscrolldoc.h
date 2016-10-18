/* $Id: pixelscrolldoc.h,v 1.1 1996-06-10 11:33:16 rbsk Exp $ */
// PixelScrollDoc.h : interface of the CPixelScrollGraphDoc class
//
/////////////////////////////////////////////////////////////////////////////

class CPixelScrollGraphDoc : public CGraph
{
protected: // create from serialization only
	CPixelScrollGraphDoc();
	DECLARE_DYNCREATE(CPixelScrollGraphDoc)

// Attributes
public:

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CPixelScrollGraphDoc)
	public:
	virtual BOOL OnNewDocument();
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CPixelScrollGraphDoc();
	virtual void Serialize(CArchive& ar);   // overridden for document i/o
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

protected:

// Generated message map functions
protected:
	//{{AFX_MSG(CPixelScrollGraphDoc)
		// NOTE - the ClassWizard will add and remove member functions here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////
 
