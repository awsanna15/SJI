
// SJIView.cpp : implementation of the CSJIView class
//

#include "stdafx.h"
// SHARED_HANDLERS can be defined in an ATL project implementing preview, thumbnail
// and search filter handlers and allows sharing of document code with that project.
#ifndef SHARED_HANDLERS
#include "SJI.h"
#endif
#include "afx.h"

#include "SJIDoc.h"
#include "SJIView.h"
#include <afx.h>
#include "ippiImage.h"
#include "MainFrm.h"
#include "ImageArithmetics.h"
#include <tchar.h>
#include "stdlib.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

#define PALETTESIZE 256
#define PI 3.14


// CSJIView

IMPLEMENT_DYNCREATE(CSJIView, CScrollView)

BEGIN_MESSAGE_MAP(CSJIView, CScrollView)
	// Standard printing commands
	ON_COMMAND(ID_FILE_PRINT, &CScrollView::OnFilePrint)
	ON_COMMAND(ID_FILE_PRINT_DIRECT, &CScrollView::OnFilePrint)
	ON_COMMAND(ID_FILE_PRINT_PREVIEW, &CSJIView::OnFilePrintPreview)
	ON_WM_MOUSEMOVE()
	ON_WM_LBUTTONDOWN()
	ON_WM_LBUTTONUP()
	ON_WM_RBUTTONDOWN()
	ON_WM_CONTEXTMENU()
	ON_WM_RBUTTONUP()

END_MESSAGE_MAP()

// CSJIView construction/destruction

CSJIView::CSJIView()
	: m_CurrentPoint(0, 0)
	, bi(NULL)
	, m_pImageRaw(NULL)
	, m_pImageBmpRaw(NULL)
{
	// TODO: add construction code here

}

CSJIView::~CSJIView()
{
	if (bi != NULL)delete bi;
	if (m_pImageBmpRaw != NULL)delete m_pImageBmpRaw;
	m_CrosArray.RemoveAll();

	CSJIDoc* pDoc = GetDocument();
	theApp.OpenPages.RemoveView(this);
	theApp.OpenPages.RemoveDoc(pDoc);
}

BOOL CSJIView::PreCreateWindow(CREATESTRUCT& cs)
{
	// TODO: Modify the Window class or styles here by modifying
	//  the CREATESTRUCT cs

	return CScrollView::PreCreateWindow(cs);
}

void CSJIView::SetPalleteForBitmapImageHeader()
{

	RGBQUAD palette[PALETTESIZE];
	SetOutputColouring(palette);


	PALETTEENTRY *pEntry = (PALETTEENTRY *)&bi->bmiColors;

	for (int i = 0; i < PALETTESIZE; i++)
	{
		pEntry[i].peBlue = palette[i].rgbBlue;
		pEntry[i].peGreen = palette[i].rgbGreen;
		pEntry[i].peRed = palette[i].rgbRed;
		pEntry[i].peFlags = 0;
	}
}

void CSJIView::SetOutputColouring(RGBQUAD* pRgb)
{
	SetColourBand(pRgb, 0, 0, 0, 0);
	SetColourBand(pRgb, 255, 255, 255, 255);
}

void CSJIView::SetColourBand(RGBQUAD *pRgb, int topval, int red, int green, int blue)
{
	static int lastval = 0;
	int nDebugLastVal = lastval;
	RGBQUAD    basergb;

	if (topval == 0)
	{
		lastval = 0;

		pRgb->rgbRed = red;
		pRgb->rgbGreen = green;
		pRgb->rgbBlue = blue;
		pRgb->rgbReserved = 0;

		return;
	}

	basergb = pRgb[lastval];

	for (int i = lastval + 1; i < PALETTESIZE && i <= topval; i++)
	{
		pRgb[i].rgbRed = GetNextColourValue(basergb.rgbRed, red, i, lastval, topval);
		pRgb[i].rgbGreen = GetNextColourValue(basergb.rgbGreen, green, i, lastval, topval);
		pRgb[i].rgbBlue = GetNextColourValue(basergb.rgbBlue, blue, i, lastval, topval);
		pRgb[i].rgbReserved = 0;
	}

	lastval = topval;
} // CDageImgPWnd::SetColourBand

BYTE CSJIView::GetNextColourValue(const BYTE base, const BYTE value, const int count, const int lastval, const int topval) const
{
	return (BYTE)(base + (value - base) * (count - lastval) / (topval - lastval));
}

HPALETTE CSJIView::CreateDibPalette()
{
	struct
	{
		WORD Version;
		WORD NumberOfEntries;
		PALETTEENTRY aEntries[256];
	} logicalPalette = { 0x300, 256 };

	for (UINT i = 0; i < 256; i++) {
		logicalPalette.aEntries[i].peRed = bi->bmiColors[i].rgbRed;
		logicalPalette.aEntries[i].peGreen = bi->bmiColors[i].rgbGreen;
		logicalPalette.aEntries[i].peBlue = bi->bmiColors[i].rgbBlue;
		logicalPalette.aEntries[i].peFlags = bi->bmiColors[i].rgbReserved;
	}
	HPALETTE hPalette = CreatePalette((LPLOGPALETTE)&logicalPalette);
	return hPalette;
}

void CSJIView::PresentResult(CIppiImage *pResult)
{
	CSJIDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);
	if (!pDoc)
	{
		return;
	}

	pDoc->m_ImagePtr = pResult;

	SIZE scrollSize;
	scrollSize.cx = pDoc->m_ImagePtr->Width();
	scrollSize.cy = pDoc->m_ImagePtr->Height();
	SetScrollSizes(MM_TEXT, scrollSize);

	UpdateView(pResult, 16);
	//	UpdateRawBmpImage();
	//	OnViewStrechimage();

	if (pDoc->m_CurRect.Width() > 1 && pDoc->m_CurRect.Height() > 1)
	{
		IppiSize RoiSize = { pDoc->m_CurRect.Width() , pDoc->m_CurRect.Height() };
		Ipp64f Mean, stdDev;
		ippiMean_StdDev_16u_C1R((const Ipp16u*)pDoc->m_ImagePtr->Point(pDoc->m_CurRect.left, pDoc->m_CurRect.top), pDoc->m_ImagePtr->Step(), RoiSize, (Ipp64f*)&Mean, (Ipp64f*)&stdDev);
		CString str;
		str.Format(_T("Average %.4f         StdDev %.4f\n"), Mean, stdDev);
		TRACE(str);
		StatusBarMessage(str);
	}


}



// CSJIView drawing

void CSJIView::OnDraw(CDC* pDC)
{
	CSJIDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);
	if (!pDoc)
		return;

	CPoint ScrollPos = GetDeviceScrollPosition();

	if (m_pImageRaw != NULL)
	{
		HPALETTE hPalette = CreateDibPalette();
		// select this palette into the dc
		HPALETTE hOldPalette = SelectPalette(pDC->m_hDC, hPalette, FALSE);
		// map the palette onto the system palette, storing a handle to the old one
		RealizePalette(pDC->m_hDC);
		CRect rect(0, 0, (int)m_pImageRaw->Width(), (int)m_pImageRaw->Height());

		StretchDIBits(pDC->m_hDC,
			0, 0,
			rect.Width(), rect.Height(),
			0, rect.Height() + 1,
			(int)rect.Width(), -(int)rect.Height(),
			m_pImageBmpRaw,
			bi,
			DIB_RGB_COLORS,
			SRCCOPY);



		// select the old palette back into the dc and delete it
		SelectPalette(pDC->m_hDC, hOldPalette, FALSE);
		DeleteObject(hPalette);
	}

	if (pDoc->m_Measure != NONE && m_ButtonPressed == FALSE)
	{
		CPen pen(PS_SOLID, 2, RGB(255, 64, 0));
		CPen* pOldPen;
		pOldPen = pDC->SelectObject(&pen);
		pDC->SelectStockObject(NULL_BRUSH);

		switch (pDoc->m_Measure)
		{
		case SHIFT:
			break;

		case RECTANGLE:
			pDC->Rectangle((pDoc->m_CurRect.left), (pDoc->m_CurRect.top), (pDoc->m_CurRect.right), (pDoc->m_CurRect.bottom));
			if (!pDoc->m_RectArray.IsEmpty())
			{


				for (int i = 0; i < pDoc->m_RectArray.GetCount(); i++)
				{
					COLORREF myCol;
					switch (pDoc->m_RectCol.GetAt(i))
					{
					case 0:
						myCol = RGB(0, 0, 255);
						break;
					case 1:
						myCol = RGB(0, 255, 0);
						break;
					case 2:
						myCol = RGB(255, 64, 0);
						break;
					case 3:
						myCol = RGB(0, 255, 255);
						break;
					case 4:
						myCol = RGB(255, 0, 255);
						break;
					case 5:
						myCol = RGB(255, 255, 0);
						break;
					case 6:
						myCol = RGB(25, 65, 128);;
						break;
					case 7:
						myCol = RGB(0, 125, 125);
						break;
					case 8:
						myCol = RGB(125, 0, 125);
						break;
					case 9:
						myCol = RGB(125, 125, 0);
						break;
					}
					CPen myPen(PS_SOLID, 2, myCol);
					pDC->SelectObject(&myPen);
					pDC->SelectStockObject(NULL_BRUSH);

					CRect tmpRect = pDoc->m_RectArray.GetAt(i);
					pDC->Rectangle((tmpRect.left), (tmpRect.top), (tmpRect.right), (tmpRect.bottom));
					pDC->SetPixel(tmpRect.CenterPoint(), myCol);
				}
			}
			break;
		case ELIPSE:
			pDC->Ellipse((pDoc->m_CurRect.left), (pDoc->m_CurRect.top), (pDoc->m_CurRect.right), (pDoc->m_CurRect.bottom));
			break;
		}

	}

	if (m_CrosArray.GetCount() > 0)
	{
		CPen Bluepen(PS_SOLID, 2, RGB(0, 0, 255));
		CPen* pOldPen;
		pOldPen = pDC->SelectObject(&Bluepen);

		for (int i = 0; i < m_CrosArray.GetCount(); i++)
		{
			CPoint tmp = m_CrosArray.GetAt(i);
			pDC->MoveTo(tmp.x - 4, tmp.y);
			pDC->LineTo(tmp.x + 3, tmp.y);
			pDC->MoveTo(tmp.x, tmp.y - 4);
			pDC->LineTo(tmp.x, tmp.y + 3);
		}
		pDC->SelectObject(pOldPen);
	}
}


// CSJIView printing


void CSJIView::OnFilePrintPreview()
{
#ifndef SHARED_HANDLERS
	AFXPrintPreview(this);
#endif
}

BOOL CSJIView::OnPreparePrinting(CPrintInfo* pInfo)
{
	// default preparation
	return DoPreparePrinting(pInfo);
}

void CSJIView::OnBeginPrinting(CDC* /*pDC*/, CPrintInfo* /*pInfo*/)
{
	// TODO: add extra initialization before printing
}

void CSJIView::OnEndPrinting(CDC* /*pDC*/, CPrintInfo* /*pInfo*/)
{
	// TODO: add cleanup after printing
}

void CSJIView::OnRButtonUp(UINT /* nFlags */, CPoint point)
{
	ClientToScreen(&point);
	OnContextMenu(this, point);
}

void CSJIView::OnContextMenu(CWnd* /* pWnd */, CPoint point)
{
#ifndef SHARED_HANDLERS
	theApp.GetContextMenuManager()->ShowPopupMenu(IDR_POPUP_EDIT, point.x, point.y, this, TRUE);
#endif
}

void CSJIView::StatusBarMessage(CString xPos, CString yPos, CString Intensity)
{
	if (AfxGetApp() != NULL && AfxGetApp()->m_pMainWnd != NULL)
	{

		CMFCStatusBar* pStatus = (CMFCStatusBar*)AfxGetApp()->m_pMainWnd->GetDescendantWindow(AFX_IDW_STATUS_BAR);

		if (pStatus != NULL)
		{
			pStatus->SetPaneText(1, xPos);
			pStatus->SetPaneText(2, yPos);
			pStatus->SetPaneText(4, Intensity);
			pStatus->UpdateWindow();
		}
	}
}

void CSJIView::StatusBarMessage(CString StatucBarMessage)
{
	if (AfxGetApp() != NULL && AfxGetApp()->m_pMainWnd != NULL)
	{

		CMFCStatusBar* pStatus = (CMFCStatusBar*)AfxGetApp()->m_pMainWnd->GetDescendantWindow(AFX_IDW_STATUS_BAR);

		if (pStatus != NULL)
		{
			pStatus->SetPaneText(0, StatucBarMessage);
			pStatus->UpdateWindow();
		}
	}
}


void CSJIView::OnMouseMove(UINT nFlags, CPoint point)
{
	if (m_pImageRaw == NULL)return;

	CPoint ScrollPos = GetDeviceScrollPosition();
	point += ScrollPos;

	if (m_pImageRaw->Width() <= point.x || m_pImageRaw->Height() <= point.y ||
		point.x < 0 || point.y < 0)
	{
		StatusBarMessage(_T(""), _T(""), _T(""));
		StatusBarMessage(_T(""));
		return;
	}

	CSJIDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);
	if (!pDoc)	return;


	if (pDoc->m_Measure != NONE && m_ButtonPressed && m_CurrentPoint != point)
	{
		CClientDC dc(this);
		if (GetCapture() != this) return;

		int oldMode = dc.SetROP2(R2_NOT);
		dc.SelectStockObject(NULL_BRUSH);
		CRect rectTmp;
		switch (pDoc->m_Measure)
		{
		case RECTANGLE:
			rectTmp = CRect((pDoc->m_CurRect.left - ScrollPos.x), (pDoc->m_CurRect.top - ScrollPos.y), (m_CurrentPoint.x - ScrollPos.x), (m_CurrentPoint.y - ScrollPos.y));
			dc.Rectangle(rectTmp);
			if (rectTmp.Width() > 5 && rectTmp.Height() > 5)
			{
				dc.Rectangle(rectTmp.CenterPoint().x - 1, rectTmp.CenterPoint().y - 1, rectTmp.CenterPoint().x + 1, rectTmp.CenterPoint().y + 1);
			}
			m_CurrentPoint = point;
			rectTmp = CRect((pDoc->m_CurRect.left - ScrollPos.x), (pDoc->m_CurRect.top - ScrollPos.y), (m_CurrentPoint.x - ScrollPos.x), (m_CurrentPoint.y - ScrollPos.y));
			dc.Rectangle(rectTmp);
			if (rectTmp.Width() > 5 && rectTmp.Height() > 5)
			{
				dc.Rectangle(rectTmp.CenterPoint().x - 1, rectTmp.CenterPoint().y - 1, rectTmp.CenterPoint().x + 1, rectTmp.CenterPoint().y + 1);
			}
			break;
		case ELIPSE:
			rectTmp = CRect((pDoc->m_CurRect.left - ScrollPos.x), (pDoc->m_CurRect.top - ScrollPos.y), (m_CurrentPoint.x - ScrollPos.x), (m_CurrentPoint.y - ScrollPos.y));
			dc.Ellipse(rectTmp);
			if (rectTmp.Width() > 5 && rectTmp.Height() > 5)
			{
				dc.Rectangle(rectTmp.CenterPoint().x - 1, rectTmp.CenterPoint().y - 1, rectTmp.CenterPoint().x + 1, rectTmp.CenterPoint().y + 1);
			}
			m_CurrentPoint = point;
			rectTmp = CRect((pDoc->m_CurRect.left - ScrollPos.x), (pDoc->m_CurRect.top - ScrollPos.y), (m_CurrentPoint.x - ScrollPos.x), (m_CurrentPoint.y - ScrollPos.y));
			dc.Ellipse(rectTmp);
			if (rectTmp.Width() > 5 && rectTmp.Height() > 5)
			{
				dc.Rectangle(rectTmp.CenterPoint().x - 1, rectTmp.CenterPoint().y - 1, rectTmp.CenterPoint().x + 1, rectTmp.CenterPoint().y + 1);
			}
			break;
		}

		dc.SetROP2(oldMode);
	}


	CString str = _T(""), strX = _T(""), strY = _T(""), strI = _T("");

	strX.Format(_T("%d"), point.x);
	strY.Format(_T("%d"), point.y);
	if (m_pImageRaw->Depth() == 8)
	{
		strI.Format(_T("%d"), *((BYTE*)m_pImageRaw->Point(point.x, point.y)));
	}
	else
	{
		if (m_pImageRaw->Divisor() > 0 && m_pImageRaw->Divisor() < 128)
		{
			WORD tmp = *(WORD*)m_pImageRaw->Point(point.x, point.y);
			int Value;
			if (tmp >= (1 << 15))
			{
				Value = (int)tmp - (int)(1 << 16);
			}
			else
			{
				Value = (int)tmp;
			}
			strI.Format(_T("%d"), Value);
		}
	}

	StatusBarMessage(strX, strY, strI);
}

void CSJIView::OnLButtonDown(UINT nFlags, CPoint point)
{
	if (m_pImageRaw == NULL)return;

	CPoint ScrollPos = GetDeviceScrollPosition();
	point += ScrollPos;

	if (m_pImageRaw->Width() < point.x || m_pImageRaw->Height() < point.y ||
		point.x < 0 || point.y < 0)
	{
		return;
	}

	CSJIDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);
	if (!pDoc)	return;

	if (pDoc->m_Measure != NONE)
	{
		CRect Region(CPoint(pDoc->m_CurRect.TopLeft() - ScrollPos), CPoint(pDoc->m_CurRect.BottomRight() - ScrollPos));
		Region.NormalizeRect();
		InflateRect(Region, 2, 2);
		InvalidateRect(Region, FALSE);
		SetCapture();
		m_ButtonPressed = TRUE;
		pDoc->m_CurRect.TopLeft() = point;
		m_CurrentPoint = point;
	}

	if (pDoc->m_Measure == SHIFT)
	{
		int Sizecda = pDoc->m_ImagePtr->Step() / 2 * pDoc->m_ImagePtr->Height() * sizeof(unsigned short);
		

	}
}

void CSJIView::OnLButtonUp(UINT nFlags, CPoint point)
{
	if (m_pImageRaw == NULL)return;

	CPoint ScrollPos = GetDeviceScrollPosition();
	point += ScrollPos;

	if (m_pImageRaw->Width()<point.x || m_pImageRaw->Height()<point.y ||
		point.x<0 || point.y<0)
	{
		return;
	}

	CSJIDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);
	if (!pDoc)	return;


	if (pDoc->m_Measure != NONE)
	{
		m_ButtonPressed = FALSE;
		pDoc->m_CurRect.BottomRight() = point;
		if (GetCapture() != this)return;

		ReleaseCapture();

		CClientDC dc(this);
		CPen *pOldPen;
		CPen aPen;
		aPen.CreatePen(PS_SOLID, 2, RGB(255, 64, 0));
		pOldPen = dc.SelectObject(&aPen);
		dc.SelectStockObject(NULL_BRUSH);

		CRect tmpRrect = pDoc->m_CurRect - ScrollPos;
		int col = 2;
		COLORREF myCol = RGB(255, 64, 0);
		switch (pDoc->m_Measure)
		{
		case RECTANGLE:
			if (GetKeyState(VK_NUMPAD0) & 0x8000000 || GetKeyState(VK_NUMPAD1) & 0x8000000 || GetKeyState(VK_NUMPAD2) & 0x8000000
				|| GetKeyState(VK_NUMPAD3) & 0x8000000 || GetKeyState(VK_NUMPAD4) & 0x8000000 || GetKeyState(VK_NUMPAD5) & 0x8000000
				|| GetKeyState(VK_NUMPAD6) & 0x8000000 || GetKeyState(VK_NUMPAD7) & 0x8000000 || GetKeyState(VK_NUMPAD8) & 0x8000000 || GetKeyState(VK_NUMPAD9) & 0x8000000)
			{
				if (!pDoc->m_RectArray.IsEmpty() && pDoc->m_CurRect.Width() == 0 && pDoc->m_CurRect.Height() == 0)
				{
					pDoc->m_RectArray.RemoveAll();
					pDoc->m_RectCol.RemoveAll();
					Invalidate();
				}
				else
				{

					if (GetKeyState(VK_NUMPAD0) & 0x8000000)
					{
						col = 0;
						myCol = RGB(0, 0, 255);
					}
					else if (GetKeyState(VK_NUMPAD1) & 0x8000000)
					{
						col = 1;
						myCol = RGB(0, 255, 0);
					}
					else if (GetKeyState(VK_NUMPAD2) & 0x8000000)
					{
						col = 2;
						myCol = RGB(255, 0, 0);
					}
					else if (GetKeyState(VK_NUMPAD3) & 0x8000000)
					{
						col = 3;
						myCol = RGB(0, 255, 255);
					}
					else if (GetKeyState(VK_NUMPAD4) & 0x8000000)
					{
						myCol = RGB(255, 0, 255);
						col = 4;
					}
					else if (GetKeyState(VK_NUMPAD5) & 0x8000000)
					{
						col = 5;
						myCol = RGB(255, 255, 0);
					}
					else if (GetKeyState(VK_NUMPAD6) & 0x8000000)
					{
						col = 6;
						myCol = RGB(25, 65, 128);
					}
					else if (GetKeyState(VK_NUMPAD7) & 0x8000000)
					{
						col = 7;
						myCol = RGB(0, 125, 125);
					}
					else if (GetKeyState(VK_NUMPAD8) & 0x8000000)
					{
						col = 8;
						myCol = RGB(125, 0, 125);
					}
					else if (GetKeyState(VK_NUMPAD9) & 0x8000000)
					{
						col = 9;
						myCol = RGB(125, 125, 0);
					}
					CPen myPen(PS_SOLID, 2, myCol);
					dc.SelectObject(&myPen);
					dc.SelectStockObject(NULL_BRUSH);
					pDoc->m_RectArray.Add(CRect((pDoc->m_CurRect.left), (pDoc->m_CurRect.top), (pDoc->m_CurRect.right), (pDoc->m_CurRect.bottom)));
					dc.Rectangle((pDoc->m_CurRect.left - ScrollPos.x), (pDoc->m_CurRect.top - ScrollPos.y), (pDoc->m_CurRect.right - ScrollPos.x), (pDoc->m_CurRect.bottom - ScrollPos.y));
					pDoc->m_CurRect = CRect(0, 0, 0, 0);
					pDoc->m_RectCol.Add(col);
				}
			}
			else
			{
				dc.Rectangle((pDoc->m_CurRect.left - ScrollPos.x), (pDoc->m_CurRect.top - ScrollPos.y), (pDoc->m_CurRect.right - ScrollPos.x), (pDoc->m_CurRect.bottom - ScrollPos.y));
			}
			break;
		case ELIPSE:
			dc.Ellipse((pDoc->m_CurRect.left - ScrollPos.x), (pDoc->m_CurRect.top - ScrollPos.y), (pDoc->m_CurRect.right - ScrollPos.x), (pDoc->m_CurRect.bottom - ScrollPos.y));
			break;
		}
		dc.SetPixel(tmpRrect.CenterPoint(), RGB(255, 64, 0));


		if (pDoc->m_Measure != NONE)
		{
			try
			{
				CRect mROI(pDoc->m_CurRect.TopLeft(), pDoc->m_CurRect.BottomRight());
				mROI.NormalizeRect();
				if (mROI.Width() % 2 == 0)
				{
					mROI.InflateRect(0, 0, 1, 0);
				}
				if (mROI.Height() % 2 == 0)
				{
					mROI.InflateRect(0, 0, 0, 1);
				}

				CIppiImage  ImageRegion(mROI.Width() + 1,
					mROI.Height() + 1,
					m_pImageRaw->Channels(), m_pImageRaw->Type());


				if (ImageRegion.Depth() == 8 || ImageRegion.Depth() == 16)
				{
					int depth = ((ImageRegion.Depth() + 7) / 8);
					for (int y = 0; y<ImageRegion.Height(); y++)
					{
						BYTE *pData = (BYTE *)ImageRegion.DataPtr();
						memcpy(
							pData + (y*ImageRegion.Width())*depth,
							(BYTE*)m_pImageRaw->DataPtr() + (y + mROI.top)*m_pImageRaw->Step() + mROI.left*depth,
							abs(ImageRegion.Width())*depth
						);
					}
				}


				dc.SelectObject(pOldPen);


				CString str = _T(""), strX = _T(""), strY = _T(""), strI = _T("");
				if (m_ButtonPressed == FALSE)
				{
					Ipp64f Mean, stdDev;
					if (pDoc->m_CurRect.Width() > 1 && pDoc->m_CurRect.Height() > 1)
					{

						if (ImageRegion.Type() == pp8u)
						{
							ippiMean_StdDev_8u_C1R((const Ipp8u*)ImageRegion.DataPtr(), ImageRegion.Step(),
								ImageRegion.Size(), (Ipp64f*)&Mean, (Ipp64f*)&stdDev);
						}
						else
						{
							ippiMean_StdDev_16u_C1R((const Ipp16u*)ImageRegion.DataPtr(), ImageRegion.Step(),
								ImageRegion.Size(), (Ipp64f*)&Mean, (Ipp64f*)&stdDev);
						}
						CRect mROIrect(pDoc->m_CurRect.TopLeft(), pDoc->m_CurRect.BottomRight());

						mROIrect.NormalizeRect();
						mROIrect.InflateRect(0, 0, 1, 1);
						str.Format(_T("Top-Left Position(%d,%d) Size(%d,%d) Mean=%.4f StdDev=%.4f"), mROIrect.left, mROIrect.top, mROIrect.Width(), mROIrect.Height(), (float)Mean, (float)stdDev);
					}
					else if (pDoc->m_CurRect.top>0 || pDoc->m_CurRect.bottom>0 || pDoc->m_CurRect.left>0 || pDoc->m_CurRect.right>0)
					{
						double x = 0;
						double y = 0;
						int areame = 0;
						CIppiImage *pImage = pDoc->m_ImagePtr;
						CIppiImage tmpImage(*pImage);
						int leftme = pImage->Width();
						int rightme = 0;
						int topme = pImage->Height();
						int bottomme = 0;

						CArray<CPoint, CPoint> FillArray;
						CArray<CPoint, CPoint> tmpFillArray;
						FillArray.Add(point);
						tmpFillArray.Add(point);
						int Val = *(Ipp16s*)tmpImage.Point(point.x, point.y);
						*(Ipp16s*)tmpImage.Point(point.x, point.y) = Val + 1;


						do
						{
							CPoint pt = FillArray.GetAt(0);
							for (int j = max(0, pt.y - 1); j <= min(tmpImage.Height() - 1, pt.y + 1); j++)
							{
								for (int i = max(0, pt.x - 1); i <= min(tmpImage.Width() - 1, pt.x + 1); i++)
								{
									int tmpVal = *(Ipp16s*)tmpImage.Point(i, j);
									if (tmpVal == Val)
									{
										FillArray.Add(CPoint(i, j));
										tmpFillArray.Add(CPoint(i, j));
										*(Ipp16s*)tmpImage.Point(i, j) = Val + 1;
									}
								}
							}

							areame++;
							x += pt.x;
							y += pt.y;
							leftme = min(pt.x, leftme);
							rightme = max(pt.x, rightme);
							topme = min(pt.y, topme);
							bottomme = max(pt.y, bottomme);


							FillArray.RemoveAt(0);
						} while (FillArray.GetCount()>0 && FillArray.GetCount()<1000);

						if (FillArray.IsEmpty())
						{
							x /= areame;
							y /= areame;
							//*************** temp add
							double lengthme; double lmin = 100000; double lmax = 0;
							double widthme; double wmin = 1000000; double wmax = 0;
							double avgdist = 0;
							if (Val > 0 && Val <= 4)
							{
								for (int i = 0; i < tmpFillArray.GetCount(); i++)
								{
									CPoint pt = tmpFillArray.GetAt(i);
									double alpha = atan2(double(pt.y) - y, double(pt.x) - x);
									double r = _hypot(double(pt.y) - y, double(pt.x) - x);
									double dalpha;
									switch (Val)
									{
									case 1:
										dalpha = 0;
										break;
									case 2:
										dalpha = 45.0*PI / 180;
										break;
									case 3:
										dalpha = 90.0*PI / 180;
										break;
									case 4:
										dalpha = 135.0*PI / 180;
										break;
									}
									double xp = x + cos(alpha - dalpha)*r;
									double yp = y + sin(alpha - dalpha)*r;
									lmin = min(xp, lmin);
									lmax = max(xp, lmax);
									wmin = min(yp, wmin);
									wmax = max(yp, wmax);
									avgdist += abs(yp - y);
								}
								lengthme = lmax - lmin;
								widthme = wmax - wmin;
								avgdist /= int(tmpFillArray.GetCount());
								str.Format(_T("Loc( %.2f, %.2f) Area %d TL(%d, %d) Sz(%d, %d) Length %.2f Wdth %.2f avgdist %.2f"), x, y, areame, leftme, topme, (rightme - leftme + 1), (bottomme - topme + 1), lengthme, widthme, avgdist);

							}
							else
							{
								str.Format(_T("Location( %.2f, %.2f) Area %d TopLeft(%d, %d) Size(%d, %d)"), x, y, areame, leftme, topme, (rightme - leftme + 1), (bottomme - topme + 1));
							}
							CString *pStrInfo = NULL;
							CMainFrame *pMainFrmWnd = (CMainFrame*)(AfxGetApp()->m_pMainWnd);
					//		pStrInfo = new CString();	pStrInfo->Format(_T("%s\n"), str); TRACE(*pStrInfo); pMainFrmWnd->PostToDebugWindow((WPARAM)pStrInfo, 0);


						}
						else
						{
							FillArray.RemoveAll();
						}
						tmpFillArray.RemoveAll();
					}


					if (GetKeyState(VK_SHIFT) & 0x8000000)
					{
						CSJIDoc* pDoc1 = GetDocument();
						CDocTemplate *pDocTemp = pDoc1->GetDocTemplate();
						POSITION pos = pDocTemp->GetFirstDocPosition();

						CString ClipboardStringArray = _T("");


						do
						{
							pDoc1 = (CSJIDoc*)pDocTemp->GetNextDoc(pos);

							CIppiImage *pImage = pDoc1->m_ImagePtr;

							CString str;
							IppiSize RoiSize = { mROI.Width(),mROI.Height() };
							for (int i = mROI.left; i < mROI.right; i++)
							{
								double avg = 0;
								for (int j = mROI.top; j < mROI.bottom; j++)
								{
									avg += *(Ipp16u*)pImage->Point(i, j);

								}
								avg /= mROI.Height();

								str.AppendFormat(_T("%d\t"), Round(avg));

							}
							str.Append(_T("\n"));
							ClipboardStringArray.Append(str);
							if (pDoc1 == NULL) break;
						} while (pos != NULL);
						SetClipboardText(ClipboardStringArray);

					}
					else if (GetKeyState(VK_CONTROL) & 0x8000000)
					{
						CSJIDoc* pDoc1 = GetDocument();
						CIppiImage *pImage = pDoc1->m_ImagePtr;
						if (mROI.left >= 0 && mROI.right<pImage->Width() && mROI.top >= 0 && mROI.bottom<pImage->Height())
						{
							CString str;
							IppiSize RoiSize = { mROI.Width(),mROI.Height() };
							for (int j = mROI.top; j<mROI.bottom; j++)
							{
								for (int i = mROI.left; i<mROI.right; i++)
								{
									str.AppendFormat(_T("%d\t"), *(Ipp16u*)pImage->Point(i, j));
								}
								str.Append(_T("\n"));
							}
							SetClipboardText(str);
						}
					}
					else
					{
						IppiSize RoiSize = { mROI.Width(),mROI.Height() };
						if (RoiSize.width>1 && RoiSize.height>1)
						{
							CSJIDoc* pDoc1 = GetDocument();
							CDocTemplate *pDocTemp = pDoc1->GetDocTemplate();
							POSITION pos = pDocTemp->GetFirstDocPosition();
							TRACE(_T("*** Avg and StdDev\n\n"));
							CString ClipboardStringArray = _T("");
							CString str;
							str.Format(_T("Avg\tStdDev\n"));
							ClipboardStringArray.Append(str);
							do
							{
								pDoc1 = (CSJIDoc*)pDocTemp->GetNextDoc(pos);

								CIppiImage *pImage = pDoc1->m_ImagePtr;


								if (mROI.left >= 0 && mROI.right<pImage->Width() && mROI.top >= 0 && mROI.bottom<pImage->Height())
								{
									ippiMean_StdDev_16u_C1R((const Ipp16u*)pImage->Point(mROI.left, mROI.top), pImage->Step(),
										RoiSize, (Ipp64f*)&Mean, (Ipp64f*)&stdDev);

									str.Format(_T("%.4f\t%.4f\n"), Mean, stdDev);
									TRACE(str);
									ClipboardStringArray.Append(str);
								}

								if (pDoc1 == NULL) break;
							} while (pos != NULL);
							SetClipboardText(ClipboardStringArray);
						}
						else
						{
							SetClipboardText(str);
						}
					}
				}


				strX.Format(_T("%d"), point.x);
				strY.Format(_T("%d"), point.y);
				if (m_pImageRaw->Depth() == 8)
				{
					strI.Format(_T("%d"), *((BYTE*)m_pImageRaw->Point(point.x, point.y)));
				}
				else
				{
					strI.Format(_T("%d"), *((WORD*)m_pImageRaw->Point(point.x, point.y)));
				}

				StatusBarMessage(strX, strY, strI);
				StatusBarMessage(str);
			}
			catch (...)
			{

			}
		}


	}


	if (pDoc->m_Measure == SHIFT)
	{

	}
}

void CSJIView::OnRButtonDown(UINT nFlags, CPoint point)
{
	CSJIDoc* pDoc = GetDocument();
	CPoint ScrollPos = GetDeviceScrollPosition();

	point += ScrollPos;
	ASSERT_VALID(pDoc);
	if (!pDoc)	return;

	if (pDoc->m_Measure == RECTANGLE)
	{
		CRect rect(pDoc->m_CurRect.TopLeft(), pDoc->m_CurRect.BottomRight());
		rect.NormalizeRect();
		BOOL ptInRect = rect.PtInRect(point);
		if (ptInRect)
		{
			CIppiImage *pImage = pDoc->m_ImagePtr;

			double ResizeFactor = max(double(pImage->Width()) / double(rect.Width()), double(pImage->Height()) / double(rect.Height()));
			CRect newRect(int(double(rect.left + rect.right) / 2 - double(pImage->Width()) / (2.0*ResizeFactor)) + 1,
				int(double(rect.top + rect.bottom) / 2 - double(pImage->Height()) / (2.0*ResizeFactor)) + 1,
				int(double(rect.left + rect.right) / 2 + double(pImage->Width()) / (2.0*ResizeFactor)) - 1,
				int(double(rect.top + rect.bottom) / 2 + double(pImage->Height()) / (2.0*ResizeFactor)) - 1);
			CIppiImage ImageSmall(newRect.Width(), newRect.Height(), 1, pp16s);
			ippiCopy_16s_C1R((const Ipp16s*)pImage->Point(newRect.left, newRect.top), pImage->Step(),
				(Ipp16s*)ImageSmall.DataPtr(), ImageSmall.Step(), ImageSmall.Size());


			Resize(pImage, &ImageSmall);

			/*IppiRect srcRect={0,0,ImageSmall.Width(),ImageSmall.Height()};
			IppiRect dstRect={0,0,pImage->Width(),pImage->Height()};

			ippiResizeCenter_16u_C1R((const Ipp16u*) ImageSmall.DataPtr(),ImageSmall.Size(),ImageSmall.Step(), srcRect,
			(Ipp16u*) pImage->DataPtr(),pImage->Step(),pImage->Size(),
			double(pImage->Width())/double(ImageSmall.Width()), double(pImage->Height())/double(ImageSmall.Height()),
			(double)ImageSmall.Width()/2,(double)ImageSmall.Height()/2,IPPI_INTER_CUBIC);*/


			UpdateRawBmpImage();
		//	OnViewStrechimage();
		}
	}
}

// CSJIView diagnostics

#ifdef _DEBUG
void CSJIView::AssertValid() const
{
	CScrollView::AssertValid();
}

void CSJIView::Dump(CDumpContext& dc) const
{
	CScrollView::Dump(dc);
}

CSJIDoc* CSJIView::GetDocument() const // non-debug version is inline
{
	ASSERT(m_pDocument->IsKindOf(RUNTIME_CLASS(CSJIDoc)));
	return (CSJIDoc*)m_pDocument;
}

void CSJIView::OnInitialUpdate()
{
	CSJIDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);
	if (!pDoc)
	{
		return;
	}

	theApp.OpenPages.AddView(this);
	theApp.OpenPages.AddDoc(pDoc);

	if (pDoc->m_ImagePtr == NULL)
	{
		SIZE scrollSize;
		scrollSize.cx = 1600;
		scrollSize.cy = 1200;
		SetScrollSizes(MM_TEXT, scrollSize);
	}
	else
	{
		SIZE scrollSize;
		scrollSize.cx = pDoc->m_ImagePtr->Width();
		scrollSize.cy = pDoc->m_ImagePtr->Height();
		SetScrollSizes(MM_TEXT, scrollSize);
		m_ImageIndex = -1;
		m_pImageRaw = NULL;
		m_pImageBmpRaw = NULL;
		m_ButtonPressed = FALSE;
		UpdateView(pDoc->m_ImagePtr, pDoc->m_BitDepth);
	}
}

void CSJIView::OnActivateView(BOOL bActivate, CView* pActivateView, CView* pDeactiveView)
{
	// TODO: Add your specialized code here and/or call the base class

	CScrollView::OnActivateView(bActivate, pActivateView, pDeactiveView);

	CSJIDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);
	if (!pDoc)
	{
		return;
	}

	if (pDoc != theApp.OpenPages.GetCurrentDoc())
	{
		theApp.OpenPages.UpdateCurrentOpenPage(pDoc);
		theApp.OpenPages.UpdateCurrentOpenPage(this);
	}
	/*	TRACE("*********\n");
	for(int i=0;i<theApp.OpenPages.GetCount();i++)
	{
	CView* pView=(CView*)theApp.OpenPages.GetViewAt(i);
	TRACE(_T("%d.\t%d\n"),i,int(pView));
	}
	TRACE("*********\n");*/
}

void CSJIView::UpdateView(CIppiImage* pImage, int BitDepth, CSJIDoc* pDocNew /*= NULL*/)
{
	if (BitDepth >= 8)
	{
		m_BitDepth = BitDepth;
	}
	if (m_BitDepth == 10)
	{
		pImage->And(0x03FF);
	}
	if (m_BitDepth == 12)
	{
		pImage->And(0x0FFF);
	}

	CreateBmpImageHeader(pImage);

	// if it was a bmp just overwrite the previous stuff with the proper data in the file

	int BitCnt = pImage->Depth();
	m_pImageRaw = pImage;
	if (m_pImageBmpRaw != NULL)
	{
		delete m_pImageBmpRaw;
		m_pImageBmpRaw = NULL;
	}
	m_pImageBmpRaw = (BYTE *)malloc(pImage->Width()*pImage->Height()*(BitCnt / 8));

	UpdateRawBmpImage(pDocNew);

	Invalidate(FALSE);
}

void CSJIView::CreateBmpImageHeader(CIppiImage* pImage)
{
	int nPaletteEntries = 0;

	if (bi != NULL)
	{
		delete bi;
		bi = NULL;
	}
	// set up the BITMAPINFO
	bi = (BITMAPINFO *)malloc(sizeof(BITMAPINFOHEADER) + (256 * sizeof(PALETTEENTRY)));

	BITMAPINFOHEADER header;
	header.biSize = sizeof(header);
	header.biWidth = (LONG)pImage->Width();
	header.biHeight = (LONG)pImage->Height();
	header.biPlanes = 1;
	header.biBitCount = 8;
	header.biCompression = 0;
	header.biSizeImage = 0;
	header.biXPelsPerMeter = 2834L;
	header.biYPelsPerMeter = 2834L;
	header.biClrUsed = 0;
	header.biClrImportant = 0;
	memcpy(bi, &header, sizeof(header));


	SetPalleteForBitmapImageHeader();
}

void CSJIView::UpdateRawBmpImage(CSJIDoc* pDocNew /*= NULL*/)
{
	CSJIDoc* pDoc;
	if (pDocNew == NULL)
	{
		pDoc = GetDocument();
	}
	else
	{
		pDoc = pDocNew;
	}
	//	ASSERT_VALID(pDoc);
	if (!pDoc)	return;


	CIppiImage tmpImage8u(m_pImageRaw->Width(), m_pImageRaw->Height(), 1, pp8u);
	CIppiImage tmpImage16s(*m_pImageRaw);
	CIppiImage tmpImage32f(m_pImageRaw->Width(), m_pImageRaw->Height(), 1, pp32f);

	if (m_pImageRaw->Type() == pp8u)
	{
		Ipp8u Max;
		ippiMax_8u_C1R((const Ipp8u*)m_pImageRaw->DataPtr(), m_pImageRaw->Step(), m_pImageRaw->Size(), &Max);
		Ipp8u Min;
		ippiMin_8u_C1R((const Ipp8u*)m_pImageRaw->DataPtr(), m_pImageRaw->Step(), m_pImageRaw->Size(), &Min);
	}
	else
	{
		if (pDoc->m_ImagePresentation == STRECHED)
		{
			Ipp16s Max;
			ippiMax_16s_C1R((const Ipp16s*)m_pImageRaw->DataPtr(), m_pImageRaw->Step(), m_pImageRaw->Size(), &Max);
			Ipp16s Min;
			ippiMin_16s_C1R((const Ipp16s*)m_pImageRaw->DataPtr(), m_pImageRaw->Step(), m_pImageRaw->Size(), &Min);
			ippiSubC_16s_C1RSfs((const Ipp16s*)m_pImageRaw->DataPtr(), m_pImageRaw->Step(), Min,
				(Ipp16s*)tmpImage16s.DataPtr(), tmpImage16s.Step(), tmpImage16s.Size(), 0);
			ippiConvert_16s32f_C1R((Ipp16s*)tmpImage16s.DataPtr(), tmpImage16s.Step(),
				(Ipp32f*)tmpImage32f.DataPtr(), tmpImage32f.Step(), tmpImage32f.Size());
			Ipp32f Multiplicator = ((Ipp32f)255 / (Max - Min));
			ippiMulC_32f_C1IR(Multiplicator, (Ipp32f*)tmpImage32f.DataPtr(), tmpImage32f.Step(), tmpImage32f.Size());
			ippiConvert_32f8u_C1R((const Ipp32f*)tmpImage32f.DataPtr(), tmpImage32f.Step(),
				(Ipp8u*)tmpImage8u.DataPtr(), tmpImage8u.Step(), tmpImage8u.Size(), ippRndZero);
			memcpy(m_pImageBmpRaw, (BYTE*)tmpImage8u.DataPtr(), m_pImageRaw->Height()*m_pImageRaw->Width());
		}
		else
		{
			ippiConvert_16s32f_C1R((Ipp16s*)m_pImageRaw->DataPtr(), m_pImageRaw->Step(),
				(Ipp32f*)tmpImage32f.DataPtr(), tmpImage32f.Step(), tmpImage32f.Size());
			Ipp32f MaxValue = (Ipp32f)(1 << m_BitDepth);
			Ipp32f Multiplicator = ((Ipp32f)255 / MaxValue);
			ippiMulC_32f_C1IR(Multiplicator, (Ipp32f*)tmpImage32f.DataPtr(), tmpImage32f.Step(), tmpImage32f.Size());
			ippiConvert_32f8u_C1R((const Ipp32f*)tmpImage32f.DataPtr(), tmpImage32f.Step(),
				(Ipp8u*)tmpImage8u.DataPtr(), tmpImage8u.Step(), tmpImage8u.Size(), ippRndZero);
			memcpy(m_pImageBmpRaw, (BYTE*)tmpImage8u.DataPtr(), m_pImageRaw->Height()*m_pImageRaw->Width());
		}

	}
}


int CSJIView::CopyTextToClipboard(CWnd* pWnd, LPCTSTR str)
{
	HGLOBAL hText = ::GlobalAlloc(GMEM_SHARE, (_tcslen(str) + 1) * sizeof(TCHAR));
	LPTSTR  pText = (LPTSTR)::GlobalLock(hText);

	if (pText == NULL)
	{
		// failed
		return -1;
	}

	_tcscpy(pText, str);


	::GlobalUnlock(hText);

	if (pWnd->OpenClipboard())
	{
		EmptyClipboard();
#if defined(UNICODE)
		SetClipboardData(CF_UNICODETEXT, hText);
#else
		SetClipboardData(CF_TEXT, hText);
#endif
		CloseClipboard();

		return 0;
	}
	else
	{
		// failed
		return -1;
	}
}

BOOL CSJIView::SetClipboardText(CString strText)
{
	int ok = CopyTextToClipboard(this, strText.GetBuffer());
	strText.ReleaseBuffer();
	return ok == 0;
}
int CSJIView::Round(const double d)
{
	const double MAX_DOUBLE = INT_MAX - 0.5;
	const double MIN_DOUBLE = INT_MIN + 0.5;

	if (d > 0.0 && d < MAX_DOUBLE)
	{
		return static_cast<int>(d + 0.5);
	}
	else if (d > MIN_DOUBLE && d < 0.0)
	{
		return static_cast<int>(d - 0.5);
	}
	else
	{
		//		ASSERT(0.0 == d);	// This ASSERT detects where d >= MAX_DOUBLE OR d <= MIN_DOUBLE
		return 0;
	}
} // Round

#endif //_DEBUG


// CSJIView message handlers
