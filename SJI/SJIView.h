
// SJIView.h : interface of the CSJIView class
//

#pragma once


class CSJIView : public CScrollView
{
protected: // create from serialization only
	CSJIView();
	DECLARE_DYNCREATE(CSJIView)

	// Attributes
public:
	CSJIDoc* GetDocument() const;

// Operations
public:
	int m_BitDepth;
	BITMAPINFO* bi;

private:
	CIppiImage *m_pImageRaw;
	BYTE *m_pImageBmpRaw;

	int m_ImageIndex;

	CPoint m_CurrentPoint;
	BOOL m_ButtonPressed;
	CArray<CPoint, CPoint> m_CrosArray;

// Overrides
public:
	void OnInitialUpdate();
	virtual void OnActivateView(BOOL bActivate, CView* pActivateView, CView* pDeactiveView);
	void UpdateView(CIppiImage* pImage, int BitDepth, CSJIDoc* pDocNew = NULL);
	void CreateBmpImageHeader(CIppiImage* pImage);
	void UpdateRawBmpImage(CSJIDoc* pDocNew = NULL);
	int CopyTextToClipboard(CWnd* pWnd, LPCTSTR str);
	BOOL SetClipboardText(CString strText);
	int Round(const double d);
	virtual void OnDraw(CDC* pDC);  // overridden to draw this view
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	void SetPalleteForBitmapImageHeader();
	void SetOutputColouring(RGBQUAD* pRgb);
	void SetColourBand(RGBQUAD *pRgb, int topval, int red, int green, int blue);
	BYTE GetNextColourValue(const BYTE base, const BYTE value,
		const int count, const int lastval,
		const int topval) const;
	HPALETTE CreateDibPalette();

	void PresentResult(CIppiImage *pResult);


protected:
	virtual BOOL OnPreparePrinting(CPrintInfo* pInfo);
	virtual void OnBeginPrinting(CDC* pDC, CPrintInfo* pInfo);
	virtual void OnEndPrinting(CDC* pDC, CPrintInfo* pInfo);

// Implementation
public:
	virtual ~CSJIView();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

protected:
	void StatusBarMessage(CString xPos, CString yPos, CString Intensity);
	void StatusBarMessage(CString StatucBarMessage);

// Generated message map functions
protected:
	afx_msg void OnFilePrintPreview();
	afx_msg void OnRButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnContextMenu(CWnd* pWnd, CPoint point);
	
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnRButtonDown(UINT nFlags, CPoint point);
	DECLARE_MESSAGE_MAP()
};

#ifndef _DEBUG  // debug version in SJIView.cpp
inline CSJIDoc* CSJIView::GetDocument() const
   { return reinterpret_cast<CSJIDoc*>(m_pDocument); }
#endif

