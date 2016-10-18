/* $Id: textfitdoc.h,v 1.1 1996-06-10 11:33:31 rbsk Exp $ */
// TextFitDoc.h : interface of the CTextFitGraphDoc class
//
/////////////////////////////////////////////////////////////////////////////

class CTextFitGraphDoc : public CGraph
{
protected: // create from serialization only
	CTextFitGraphDoc();
	DECLARE_DYNCREATE(CTextFitGraphDoc)

// Attributes
public:

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CTextFitGraphDoc)
	public:
	virtual BOOL OnNewDocument();
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CTextFitGraphDoc();
	virtual void Serialize(CArchive& ar);   // overridden for document i/o
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

protected:

// Generated message map functions
protected:
	//{{AFX_MSG(CTextFitGraphDoc)
		// NOTE - the ClassWizard will add and remove member functions here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////
 
