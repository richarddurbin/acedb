/* $Id: fullscrolldoc.h,v 1.1 1996-06-10 11:32:51 rbsk Exp $ */
// FullScrollDoc.h : interface of the CTextFullScrollGraphDoc class
//
/////////////////////////////////////////////////////////////////////////////

class CTextFullScrollGraphDoc : public CGraph
{
protected: // create from serialization only
	CTextFullScrollGraphDoc();
	DECLARE_DYNCREATE(CTextFullScrollGraphDoc)

// Attributes
public:

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CTextFullScrollGraphDoc)
	public:
	virtual BOOL OnNewDocument();
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CTextFullScrollGraphDoc();
	virtual void Serialize(CArchive& ar);   // overridden for document i/o
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

protected:

// Generated message map functions
protected:
	//{{AFX_MSG(CTextFullScrollGraphDoc)
		// NOTE - the ClassWizard will add and remove member functions here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////
 
