
// SJIDoc.cpp : implementation of the CSJIDoc class
//

#include "stdafx.h"
// SHARED_HANDLERS can be defined in an ATL project implementing preview, thumbnail
// and search filter handlers and allows sharing of document code with that project.
#ifndef SHARED_HANDLERS
#include "SJI.h"
#endif


#include "SJIDoc.h"
#include <memory> // Add this include directive at the top of the file
#include <propkey.h>
#include <cstring>
#include "ippiImage.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#endif

// CSJIDoc

IMPLEMENT_DYNCREATE(CSJIDoc, CDocument)

BEGIN_MESSAGE_MAP(CSJIDoc, CDocument)
END_MESSAGE_MAP()


// CSJIDoc construction/destruction

CSJIDoc::CSJIDoc()
	: m_BitDepth(16)
	, m_ImagePtr(nullptr)
	, m_Measure(RECTANGLE)
	, m_CurRect(0, 0, 0, 0)
	, m_ImagePresentation(STRECHED)
	, m_bAutomaticImageSizeON(TRUE)
{
	// TODO: add one-time construction code here

}

CSJIDoc::~CSJIDoc()
{
	if (m_ImagePtr != NULL)
	{
		delete m_ImagePtr;
		m_ImagePtr = NULL;
	}

	m_RectArray.RemoveAll();
	m_RectCol.RemoveAll();
}

BOOL CSJIDoc::OnNewDocument()
{
	if (!CDocument::OnNewDocument())
		return FALSE;

	// TODO: add reinitialization code here
	// (SDI documents will reuse this document)

	return TRUE;
}




// CSJIDoc serialization

void CSJIDoc::Serialize(CArchive& ar)
{
	if (ar.IsStoring())
	{
		// TODO: add storing code here
	}
	else
	{
		if (ar.m_strFileName.Right(3).MakeLower().Compare(_T("jpg")) == 0)
		{
			CString FileName = _T("");
			for (int i = 0; i < ar.m_strFileName.GetLength(); i++)
			{
				FileName.Append(ar.m_strFileName.Mid(i, 1));
				if (ar.m_strFileName.Mid(i, 1).Compare(_T("\\")) == 0)
				{
					FileName.Append(_T("\\"));
				}
			}

			

			std::unique_ptr<CIppiImage> pSrcImageEx_tmp = std::make_unique<CIppiImage>();
			std::unique_ptr<CIppiImage> pSrcImageEx_16s = std::make_unique<CIppiImage>();

			pSrcImageEx_tmp.get()->LoadImage(FileName);
			if (pSrcImageEx_tmp.get() != nullptr && pSrcImageEx_tmp.get()->IsValid())
				pSrcImageEx_16s.get()->CreateImage(pSrcImageEx_tmp.get()->Width(), pSrcImageEx_tmp.get()->Height(), 1, pp16s);
			else
				return;

			ippiConvert_8u16s_C1R((const Ipp8u*)pSrcImageEx_tmp.get()->DataPtr(), pSrcImageEx_tmp.get()->Step(),
				(Ipp16s*)pSrcImageEx_16s.get()->DataPtr(), pSrcImageEx_16s.get()->Step(), pSrcImageEx_16s.get()->Size());

			int Width = pSrcImageEx_16s.get()->Width();
			int Height = pSrcImageEx_16s.get()->Height();
			int BitDepth = 16;
			m_BitDepth = BitDepth;
			m_ImagePtr = new CIppiImage(*(pSrcImageEx_16s.get()));

			if (ar.m_strFileName.Right(3).MakeLower().Compare(_T("jpg")) != 0)
			{
				ippiMirror_16u_C1IR((Ipp16u*)m_ImagePtr->DataPtr(), m_ImagePtr->Step(), m_ImagePtr->Size(), ippAxsHorizontal);
			}
			
		}
		else if (ar.m_strFileName.Right(3).MakeLower().Compare(_T("bmp")) == 0)
		{
			CString FileName = _T("");
			for (int i = 0; i < ar.m_strFileName.GetLength(); i++)
			{
				FileName.Append(ar.m_strFileName.Mid(i, 1));
				if (ar.m_strFileName.Mid(i, 1).Compare(_T("\\")) == 0)
				{
					FileName.Append(_T("\\"));
				}
			}

			std::unique_ptr<CIppiImage> pSrcImageEx_tmp = std::make_unique<CIppiImage>();
			std::unique_ptr<CIppiImage> pSrcImageEx_16s = std::make_unique<CIppiImage>();

			CFile* pFile = ar.GetFile();
			int nCount, nSize;
			BITMAPFILEHEADER bmfh;
			LPBITMAPINFOHEADER m_lpBMIH;

			Ipp8u* p8uDataPtr;
			BYTE* pBuffer;
			Ipp16s* p16sDataPtr;
			WORD* pBuffer2;

			nCount = pFile->Read((LPVOID)&bmfh, sizeof(BITMAPFILEHEADER));
			nSize = bmfh.bfOffBits - sizeof(BITMAPFILEHEADER);
			m_lpBMIH = (LPBITMAPINFOHEADER) new char[nSize];
			nCount = pFile->Read(m_lpBMIH, nSize); // info hdr & color table


			pSrcImageEx_16s.get()->CreateImage(m_lpBMIH->biWidth, m_lpBMIH->biHeight, 1, pp16s);
			switch (m_lpBMIH->biBitCount)
			{
			case 8:
				pSrcImageEx_tmp.get()->CreateImage(m_lpBMIH->biWidth, m_lpBMIH->biHeight, 1, pp8u);
				pBuffer = (BYTE*) new BYTE[m_lpBMIH->biSizeImage];
				pFile->Read(pBuffer, m_lpBMIH->biSizeImage);
				p8uDataPtr = (Ipp8u*)pSrcImageEx_tmp.get()->DataPtr();
				memcpy(p8uDataPtr, pBuffer, m_lpBMIH->biSizeImage);
				delete pBuffer;
				ippiConvert_8u16s_C1R((const Ipp8u*)pSrcImageEx_tmp.get()->DataPtr(), pSrcImageEx_tmp.get()->Step(),
					(Ipp16s*)pSrcImageEx_16s.get()->DataPtr(), pSrcImageEx_16s.get()->Step(), pSrcImageEx_16s.get()->Size());
				break;
			case 16:
				pBuffer2 = (WORD*) new WORD[m_lpBMIH->biWidth * m_lpBMIH->biHeight];
				pFile->Read(pBuffer2, m_lpBMIH->biWidth * m_lpBMIH->biHeight);
				p16sDataPtr = (Ipp16s*)pSrcImageEx_16s.get()->DataPtr();
				memcpy(p16sDataPtr, pBuffer2, m_lpBMIH->biSizeImage);
				delete pBuffer2;
				break;
			case 24:
			case 32:
				int ByteDepth = m_lpBMIH->biBitCount / 8;
				pBuffer = (BYTE*) new BYTE[ByteDepth];
				for (int j = 0; j < m_lpBMIH->biHeight; j++)
				{
					for (int i = 0; i < m_lpBMIH->biWidth; i++)
					{
						pFile->Read(pBuffer, ByteDepth);
						p16sDataPtr = (Ipp16s*)pSrcImageEx_16s.get()->Point(i, j);
						*p16sDataPtr = 0;
						for (int k = 0; k < ByteDepth; k++)
						{
							*p16sDataPtr += pBuffer[k];
						}
						*p16sDataPtr /= ByteDepth;
					}
				}
				delete pBuffer;
				break;
			}

			delete m_lpBMIH;



			int Width = pSrcImageEx_16s.get()->Width();
			int Height = pSrcImageEx_16s.get()->Height();
			int BitDepth = 16;
			m_BitDepth = BitDepth;
			m_ImagePtr = new CIppiImage(*(pSrcImageEx_16s.get()));

			if (ar.m_strFileName.Right(3).MakeLower().Compare(_T("jpg")) != 0)
			{
				ippiMirror_16u_C1IR((Ipp16u*)m_ImagePtr->DataPtr(), m_ImagePtr->Step(), m_ImagePtr->Size(), ippAxsHorizontal);
			}



		}
		else if (ar.m_strFileName.Right(3).MakeLower().Compare(_T("raw")) == 0 )
		{
			CString FileName = _T("");
			for (int i = 0; i < ar.m_strFileName.GetLength(); i++)
			{
				FileName.Append(ar.m_strFileName.Mid(i, 1));
				if (ar.m_strFileName.Mid(i, 1).Compare(_T("\\")) == 0)
				{
					FileName.Append(_T("\\"));
				}
			}
			//std::unique_ptr<CIppiImage> pSrcImageEx_tmp = make_unique<CIppiImage>();
			//std::unique_ptr<CIppiImage> pSrcImageEx_16s = make_unique<CIppiImage>();

			CFile *file = ar.GetFile();
			ULONGLONG FileSize = file->SeekToEnd();
			file->SeekToBegin();
			CString KeyName;
			KeyName.Format(_T("FileSize:%d"), FileSize);
			int Width = 0;
			int Height = 0;
			int BitDepth = 0;
			CString result;
			BOOL bRegistriEntryFound = FALSE;

			{

				switch (FileSize)
				{
				case 3888 * 3072 * 2:
					result.Format(_T("16 3072 3888"));
					Width = 3072;
					Height = 3888;
					BitDepth = 16;
					bRegistriEntryFound = TRUE;
					break;
					break;
				case 1944 * 1536 * 2:
					result.Format(_T("16 1536 1944"));
					Width = 1536;
					Height = 1944;
					BitDepth = 16;
					bRegistriEntryFound = TRUE;
					break;
				
		}
	}

			
			/*if ( !bRegistriEntryFound)
			{
				CFileOpenAdditional dlg(ar.m_strFileName, FileSize, BitDepth, Width, Height);
				dlg.DoModal();
				Width = dlg.GetWidth();
				Height = dlg.GetHeight();
				BitDepth = 16;
			}*/

			m_BitDepth = BitDepth;

			if ((UINT)FileSize>(UINT)(Width*Height*(int)((BitDepth + 7) / 8)))
			{
				AfxMessageBox(_T("Specified image is larger then file."));
				AfxGetMainWnd()->PostMessage(WM_COMMAND, ID_FILE_CLOSE);
			}
			else if ((UINT)FileSize<(UINT)(Width*Height*(int)((BitDepth + 7) / 8)))
			{
				AfxMessageBox(_T("Specified image is smaller then file."));
				AfxGetMainWnd()->PostMessage(WM_COMMAND, ID_FILE_CLOSE);
			} //******************************************************************************
			else
			{
				BitDepth = 16;
			
				if (m_ImagePtr != NULL) delete m_ImagePtr;

				if (BitDepth == 8)
				{
					m_ImagePtr = new CIppiImage(Width, Height, 1, pp8u);

				}
				else
				{
					m_ImagePtr = new CIppiImage(Width, Height, 1, pp16s);
				}
				for (int j = 0; j<Height; j++)
				{
					WORD *pData = (WORD *)m_ImagePtr->Point(0, j);
					for (int i = 0; i<Width; i++)
					{
						WORD b;
						ar >> (WORD)b;
						*pData = b;
						pData++;
					}
				}

			}
		}
	}
}

#ifdef SHARED_HANDLERS

// Support for thumbnails
void CSJIDoc::OnDrawThumbnail(CDC& dc, LPRECT lprcBounds)
{
	// Modify this code to draw the document's data
	dc.FillSolidRect(lprcBounds, RGB(255, 255, 255));

	CString strText = _T("TODO: implement thumbnail drawing here");
	LOGFONT lf;

	CFont* pDefaultGUIFont = CFont::FromHandle((HFONT) GetStockObject(DEFAULT_GUI_FONT));
	pDefaultGUIFont->GetLogFont(&lf);
	lf.lfHeight = 36;

	CFont fontDraw;
	fontDraw.CreateFontIndirect(&lf);

	CFont* pOldFont = dc.SelectObject(&fontDraw);
	dc.DrawText(strText, lprcBounds, DT_CENTER | DT_WORDBREAK);
	dc.SelectObject(pOldFont);
}

// Support for Search Handlers
void CSJIDoc::InitializeSearchContent()
{
	CString strSearchContent;
	// Set search contents from document's data. 
	// The content parts should be separated by ";"

	// For example:  strSearchContent = _T("point;rectangle;circle;ole object;");
	SetSearchContent(strSearchContent);
}

void CSJIDoc::SetSearchContent(const CString& value)
{
	if (value.IsEmpty())
	{
		RemoveChunk(PKEY_Search_Contents.fmtid, PKEY_Search_Contents.pid);
	}
	else
	{
		CMFCFilterChunkValueImpl *pChunk = NULL;
		ATLTRY(pChunk = new CMFCFilterChunkValueImpl);
		if (pChunk != NULL)
		{
			pChunk->SetTextValue(PKEY_Search_Contents, value, CHUNK_TEXT);
			SetChunkValue(pChunk);
		}
	}
}

#endif // SHARED_HANDLERS

// CSJIDoc diagnostics

#ifdef _DEBUG
void CSJIDoc::AssertValid() const
{
	CDocument::AssertValid();
}

void CSJIDoc::Dump(CDumpContext& dc) const
{
	CDocument::Dump(dc);
}
#endif //_DEBUG


// CSJIDoc commands
