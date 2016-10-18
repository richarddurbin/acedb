/* $Id: mapscrolldoc.h,v 1.1 1996-06-10 11:33:07 rbsk Exp $ */
// MapScrollDoc.h : interface of the CMapScrollGraphDoc class
//
/////////////////////////////////////////////////////////////////////////////

class CMapScrollGraphDoc : public CGraph
{
protected: // create from serialization only
	CMapScrollGraphDoc();
	DECLARE_DYNCREATE(CMapScrollGraphDoc)

// Attributes
public:

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CMapScrollGraphDoc)
	public:
	virtual BOOL OnNewDocument();
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CMapScrollGraphDoc();
	virtual void Serialize(CArchive& ar);   // overridden for document i/o
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

protected:

// Generated message map functions
protected:
	//{{AFX_MSG(CMapScrollGraphDoc)
		// NOTE - the ClassWizard will add and remove member functions here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////
 
