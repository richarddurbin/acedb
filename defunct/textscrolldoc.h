/* $Id: textscrolldoc.h,v 1.1 1996-06-10 11:33:35 rbsk Exp $ */
// TextScrollDoc.h : interface of the CTextScrollGraphDoc class
//
/////////////////////////////////////////////////////////////////////////////

class CTextScrollGraphDoc : public CGraph
{
protected: // create from serialization only
	CTextScrollGraphDoc();
	DECLARE_DYNCREATE(CTextScrollGraphDoc)

// Attributes
public:

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CTextScrollGraphDoc)
	public:
	virtual BOOL OnNewDocument();
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CTextScrollGraphDoc();
	virtual void Serialize(CArchive& ar);   // overridden for document i/o
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

protected:

// Generated message map functions
protected:
	//{{AFX_MSG(CTextScrollGraphDoc)
		// NOTE - the ClassWizard will add and remove member functions here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////
 
