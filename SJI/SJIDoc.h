
// SJIDoc.h : interface of the CSJIDoc class
//


#pragma once

class CIppiImage;

enum MyMeasureType
{
	NONE,
	RECTANGLE,
	ELIPSE,
	SHIFT
};

enum ImagePresentationType
{
	STRECHED,
	ORIGINAL
};

class CSJIDoc : public CDocument
{
protected: // create from serialization only
	CSJIDoc();
	DECLARE_DYNCREATE(CSJIDoc)

// Attributes
public:
	CIppiImage *m_ImagePtr;
	int m_Measure;
	int m_BitDepth;
	int m_ImagePresentation;
	CPoint m_CurrentPoint;
	CRect m_CurRect;
	CArray<CRect, CRect>m_RectArray;
	CArray<int, int>m_RectCol;
	CString m_FileExtension;
	BOOL m_bAutomaticImageSizeON;
// Operations
public:

// Overrides
public:
	virtual BOOL OnNewDocument();
	virtual void Serialize(CArchive& ar);
#ifdef SHARED_HANDLERS
	virtual void InitializeSearchContent();
	virtual void OnDrawThumbnail(CDC& dc, LPRECT lprcBounds);
#endif // SHARED_HANDLERS

// Implementation
public:
	virtual ~CSJIDoc();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

protected:

// Generated message map functions
protected:
	DECLARE_MESSAGE_MAP()

#ifdef SHARED_HANDLERS
	// Helper function that sets search content for a Search Handler
	void SetSearchContent(const CString& value);
#endif // SHARED_HANDLERS
};
