
// SJI.cpp : Defines the class behaviors for the application.
//

#include "stdafx.h"
#include "afxwinappex.h"
#include "afxdialogex.h"
#include "SJI.h"
#include "MainFrm.h"

#include "ChildFrm.h"
#include "SJIDoc.h"
#include "SJIView.h"

#include "ippiImage.h"
#include "IppiWrapperAlias.h"
#include "MyUtilities.h"

#include <opencv2/opencv.hpp>
#include "opencv2/dnn.hpp"
#include <vector>
#include <iostream>
#include <numeric>
#include <cmath>
#include <unordered_map>
#include "math.h"
#include "Segmenter.h"
#include "DageIpp.h"
#include "ippiImageDC.h"
#include "SelectOptions.h"

using namespace cv;
using namespace std;

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// CSJIApp

BEGIN_MESSAGE_MAP(CSJIApp, CWinAppEx)
	ON_COMMAND(ID_APP_ABOUT, &CSJIApp::OnAppAbout)
	// Standard file based document commands
	ON_COMMAND(ID_FILE_NEW, &CWinAppEx::OnFileNew)
	ON_COMMAND(ID_FILE_OPEN, &CWinAppEx::OnFileOpen)
	// Standard print setup command
	ON_COMMAND(ID_FILE_PRINT_SETUP, &CWinAppEx::OnFilePrintSetup)
	ON_COMMAND(ID_SEGMENTATION_EDGEBASEDSEGMENT, &CSJIApp::OnSegmentationEdgebasedsegment)
	ON_COMMAND(ID_SEGMENTATION_IMAGEREGISTRATION, &CSJIApp::OnSegmentationImageregistration)
	ON_COMMAND(ID_SEGMENTATION_SIMPLEBUMPCV, &CSJIApp::OnSegmentationSimplebumpcv)
	ON_COMMAND(ID_TESTFUNCTIONS_ADAPTIVETHREASHOLD, &CSJIApp::OnTestfunctionsAdaptivethreashold)
	ON_COMMAND(ID_INFERENCING_RUNINFERENCE, &CSJIApp::OnInferencingRuninference)
	ON_COMMAND(ID_INFERENCING_TESTSEGMENTS, &CSJIApp::OnInferencingTestsegments)
	ON_COMMAND(ID_INFERENCING_RUNINFERENCEONFOLDER, &CSJIApp::OnInferencingRuninferenceonfolder)
	ON_COMMAND(ID_AUGMENTATION_CREATE1000, &CSJIApp::OnAugmentationCreate1000)
END_MESSAGE_MAP()


// CSJIApp construction

CSJIApp::CSJIApp()
{
	m_bHiColorIcons = TRUE;

	// support Restart Manager
	m_dwRestartManagerSupportFlags = AFX_RESTART_MANAGER_SUPPORT_ALL_ASPECTS;
#ifdef _MANAGED
	// If the application is built using Common Language Runtime support (/clr):
	//     1) This additional setting is needed for Restart Manager support to work properly.
	//     2) In your project, you must add a reference to System.Windows.Forms in order to build.
	System::Windows::Forms::Application::SetUnhandledExceptionMode(System::Windows::Forms::UnhandledExceptionMode::ThrowException);
#endif

	// TODO: replace application ID string below with unique ID string; recommended
	// format for string is CompanyName.ProductName.SubProduct.VersionInformation
	SetAppID(_T("SJI.AppID.NoVersion"));

	// TODO: add construction code here,
	// Place all significant initialization in InitInstance
}

// The one and only CSJIApp object

CSJIApp theApp;


// CSJIApp initialization

BOOL CSJIApp::InitInstance()
{
	// InitCommonControlsEx() is required on Windows XP if an application
	// manifest specifies use of ComCtl32.dll version 6 or later to enable
	// visual styles.  Otherwise, any window creation will fail.
	INITCOMMONCONTROLSEX InitCtrls;
	InitCtrls.dwSize = sizeof(InitCtrls);
	// Set this to include all the common control classes you want to use
	// in your application.
	InitCtrls.dwICC = ICC_WIN95_CLASSES;
	InitCommonControlsEx(&InitCtrls);

	CWinAppEx::InitInstance();


	// Initialize OLE libraries
	if (!AfxOleInit())
	{
		AfxMessageBox(IDP_OLE_INIT_FAILED);
		return FALSE;
	}

	AfxEnableControlContainer();

	EnableTaskbarInteraction();

	// AfxInitRichEdit2() is required to use RichEdit control	
	// AfxInitRichEdit2();

	// Standard initialization
	// If you are not using these features and wish to reduce the size
	// of your final executable, you should remove from the following
	// the specific initialization routines you do not need
	// Change the registry key under which our settings are stored
	// TODO: You should modify this string to be something appropriate
	// such as the name of your company or organization
	SetRegistryKey(_T("Local AppWizard-Generated Applications"));
	LoadStdProfileSettings(4);  // Load standard INI file options (including MRU)


	InitContextMenuManager();

	InitKeyboardManager();

	InitTooltipManager();
	CMFCToolTipInfo ttParams;
	ttParams.m_bVislManagerTheme = TRUE;
	theApp.GetTooltipManager()->SetTooltipParams(AFX_TOOLTIP_TYPE_ALL,
		RUNTIME_CLASS(CMFCToolTipCtrl), &ttParams);

	// Register the application's document templates.  Document templates
	//  serve as the connection between documents, frame windows and views
	CMultiDocTemplate* pDocTemplate;
	pDocTemplate = new CMultiDocTemplate(IDR_SJITYPE,
		RUNTIME_CLASS(CSJIDoc),
		RUNTIME_CLASS(CChildFrame), // custom MDI child frame
		RUNTIME_CLASS(CSJIView));
	if (!pDocTemplate)
		return FALSE;
	AddDocTemplate(pDocTemplate);

	// create main MDI Frame window
	CMainFrame* pMainFrame = new CMainFrame;
	if (!pMainFrame || !pMainFrame->LoadFrame(IDR_MAINFRAME))
	{
		delete pMainFrame;
		return FALSE;
	}
	m_pMainWnd = pMainFrame;


	// Parse command line for standard shell commands, DDE, file open
	CCommandLineInfo cmdInfo;
	ParseCommandLine(cmdInfo);



	// Dispatch commands specified on the command line.  Will return FALSE if
	// app was launched with /RegServer, /Register, /Unregserver or /Unregister.
	//if (!ProcessShellCommand(cmdInfo))
	//	return FALSE;
	// The main window has been initialized, so show and update it
	pMainFrame->ShowWindow(m_nCmdShow);
	pMainFrame->UpdateWindow();

	return TRUE;
}

int CSJIApp::ExitInstance()
{
	//TODO: handle additional resources you may have added
	AfxOleTerm(FALSE);

	return CWinAppEx::ExitInstance();
}

// CSJIApp message handlers


// CAboutDlg dialog used for App About

class CAboutDlg : public CDialogEx
{
public:
	CAboutDlg();

// Dialog Data
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_ABOUTBOX };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

// Implementation
protected:
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialogEx(IDD_ABOUTBOX)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialogEx)
END_MESSAGE_MAP()

// App command to run the dialog
void CSJIApp::OnAppAbout()
{
	CAboutDlg aboutDlg;
	aboutDlg.DoModal();
}

// CSJIApp customization load/save methods

void CSJIApp::PreLoadState()
{
	BOOL bNameValid;
	CString strName;
	bNameValid = strName.LoadString(IDS_EDIT_MENU);
	ASSERT(bNameValid);
	GetContextMenuManager()->AddMenu(strName, IDR_POPUP_EDIT);
	bNameValid = strName.LoadString(IDS_EXPLORER);
	ASSERT(bNameValid);
	GetContextMenuManager()->AddMenu(strName, IDR_POPUP_EXPLORER);
}

void CSJIApp::LoadCustomState()
{
}

void CSJIApp::SaveCustomState()
{
}

// CSJIApp message handlers

void CSJIApp::PresentResult(CIppiImage *pResult)
{
	CSJIView* pView = (CSJIView*)theApp.OpenPages.GetViewAt(0); // get first one
	pView->PresentResult(pResult);
}

void CSJIApp::PresentResult(CSJIDoc *pDoc)
{
	POSITION posView = pDoc->GetFirstViewPosition();
	CSJIView* pView = (CSJIView*)pDoc->GetNextView(posView); // get first one

	pView->PresentResult(pDoc->m_ImagePtr);
}



void CSJIApp::OnSegmentationEdgebasedsegment()
{
	int col = 1; //green
	if (OpenPages.GetCount() >= 1)
	{
		
		for (int i = 0; i < OpenPages.GetCount(); i++)
		{
			CSJIDoc* pDoc1 = OpenPages.GetDocAt(i);
			CIppiImage* pImage1 = pDoc1->m_ImagePtr;

			CSegmenter Segmenter(pImage1,EdgeBased);
			SingleObjectVec circleCenters = Segmenter.GetBumpLocations();
			//draw  markers
			
			pDoc1->m_RectArray.RemoveAll();
			pDoc1->m_RectCol.RemoveAll();
			for (int i = 0; i < circleCenters.size(); i++)
			{
				CPoint cp = circleCenters[i].centre;
				CRect rect = CRect(cp.x - 1, cp.y - 1, cp.x + 2, cp.y + 2);
				pDoc1->m_RectArray.Add(rect);
				pDoc1->m_RectCol.Add(col);
			}
			PresentResult(pDoc1);


		}

		

	}
}

CString CSJIApp::CreateNewResultsDir(CString SaveResultsDir)
{
	CStringA strSaveImageDir = CStringA(SaveResultsDir);
	CString strDir = CString(strSaveImageDir);
	if (strDir.GetAllocLength() == 0)
	{
		CString tmpstr;

		if (tmpstr.GetAllocLength() > 0)
		{
			strDir = tmpstr;
			strSaveImageDir = tmpstr;
		}
	}

	BOOL bDirExist = FALSE;
	if (!dirExists(strSaveImageDir))
	{
		int pos = strDir.ReverseFind('\\');
		CStringA strPartial = strSaveImageDir.Mid(0, pos);
		if (dirExists(strPartial))
		{
			CreateDirectory(strDir, 0);
			strDir.Append(_T("\\001"));
			CreateDirectory(strDir, 0);
			bDirExist = TRUE;
		}

	}
	else
	{
		int cnt = 1;
		CString tmpStr;
		do
		{
			tmpStr.Format(_T("%s\\%03d"), strDir, cnt);
			cnt++;
		} while (dirExists(CStringA(tmpStr)));
		strDir = (tmpStr);
		CreateDirectory(strDir, 0);
		bDirExist = TRUE;
	}

	return strDir;
}

void CSJIApp::SaveResults(CString strDir, std::vector<CBridgeResult>& bridgeResults, CIppiImage* pInspectionImage)
{
	

	CString strDirBridge = strDir + _T("\\Bridge");
	CreateDirectory(strDirBridge, 0);
	CString strDirNonBridge = strDir + _T("\\Non-Bridge");
	CreateDirectory(strDirNonBridge, 0);

	CString str;
	for (int i = 0; i < bridgeResults.size(); i++)
	{
		Sleep(2); // to prevent overwrite with the same name
		if (bridgeResults[i].m_bIsTrueBridge)
		{
			str.Format(_T("%s\\cb_%03d.bmp"),strDirBridge, GetTickCount());
            bridgeResults[i].pBridgeImg->SaveImage(str);
		}
		else
		{
			str.Format(_T("%s\\cb_%03d.bmp"), strDirNonBridge, GetTickCount());
			bridgeResults[i].pBridgeImg->SaveImage(str);
		}
	}

	//*********************************** draw overlay on result image *******************************
	CIppiImage Image8u(pInspectionImage->Width(), pInspectionImage->Height(), 1, pp8u);
	SmartConversionto8bit(*pInspectionImage, Image8u, CRect(0,0, pInspectionImage->Width(), pInspectionImage->Height()));
//Image8u.SaveImage(_T("C:\\Temp\\Image8u.bmp"));
	CIppiImage Image8u3R(Image8u.Width(), Image8u.Height(), 3, pp8u);
	DageIPP::ippiConv_8u_C1C3R(&Image8u, &Image8u3R);
//Image8u3R.SaveImage(_T("C:\\Temp\\Image8u3R.bmp"));
	int bridgeThickness = 2;

	CClientDC     dcClient(nullptr);
	CIppiImageDC* pimDC = new CIppiImageDC;
	pimDC->Create(&dcClient, &Image8u3R);
	pimDC->SetData();
	CBrush* oldBrush = (CBrush*)pimDC->SelectStockObject(NULL_BRUSH);
	for (int i = 0; i < bridgeResults.size(); i++)
	{
		if (bridgeResults[i].m_bIsTrueBridge)
		{
			CPoint pt = CPoint(bridgeResults[i].xpix, bridgeResults[i].ypix);
			int ddx = bridgeResults[i].boundingRect.Width() / 4 * 3;
			int ddy = bridgeResults[i].boundingRect.Height() / 4 * 3;
			CPen pen(PS_SOLID, bridgeThickness, RGB(255, 0, 0)); // overlay colour red
			CPen* oldpen = pimDC->SelectObject(&pen);

			pimDC->Ellipse(pt.x - ddx, pt.y - ddy, pt.x + ddx, pt.y + ddy);
			pimDC->SelectObject(oldpen);
		}
	}
	pimDC->SetImage();

	pimDC->SelectObject(oldBrush);
	delete pimDC;
	pimDC = nullptr;

	str.Format(_T("%s\\Result_%03d.bmp"), strDir, GetTickCount());
	Image8u3R.SaveImage(str);
}




void CSJIApp::OnSegmentationImageregistration()
{
	int col = 1; //green
	if (OpenPages.GetCount() >= 1)
	{

		for (int i = 0; i < OpenPages.GetCount(); i++)
		{
			CSJIDoc* pDoc1 = OpenPages.GetDocAt(i);
			CIppiImage* pImage1 = pDoc1->m_ImagePtr;

			CSegmenter Segmenter(pImage1, SimpleBumpCV, SuspectsOnly);
			SingleObjectVec circleCenters = Segmenter.GetBumpLocations();
			//draw  markers

			pDoc1->m_RectArray.RemoveAll();
			pDoc1->m_RectCol.RemoveAll();
			for (int i = 0; i < circleCenters.size(); i++)
			{
				CPoint cp = circleCenters[i].centre;
				CRect rect = CRect(cp.x - 1, cp.y - 1, cp.x + 2, cp.y + 2);
				pDoc1->m_RectArray.Add(rect);
				pDoc1->m_RectCol.Add(col);
			}
			PresentResult(pDoc1);


		}



	}

}


void CSJIApp::OnSegmentationSimplebumpcv()
{
	int col = 1; //green
	if (OpenPages.GetCount() >= 1)
	{

		for (int i = 0; i < OpenPages.GetCount(); i++)
		{
			CSJIDoc* pDoc1 = OpenPages.GetDocAt(i);
			CIppiImage* pImage1 = pDoc1->m_ImagePtr;

			CSegmenter Segmenter(pImage1, SimpleBumpCV);
			SingleObjectVec circleCenters = Segmenter.GetBumpLocations();
			//draw  markers

			pDoc1->m_RectArray.RemoveAll();
			pDoc1->m_RectCol.RemoveAll();
			for (int i = 0; i < circleCenters.size(); i++)
			{
				CPoint cp = circleCenters[i].centre;
				CRect rect = CRect(cp.x - 1, cp.y - 1, cp.x + 2, cp.y + 2);
				pDoc1->m_RectArray.Add(rect);
				pDoc1->m_RectCol.Add(col);
			}
			PresentResult(pDoc1);


		}



	}
}


void CSJIApp::OnTestfunctionsAdaptivethreashold()
{
	int BumpSizeInPixels = 8;
	if (OpenPages.GetCount() >= 1)
	{

		CSJIDoc* pDoc1 = OpenPages.GetDocAt(0);
		CIppiImage* pImage1 = pDoc1->m_ImagePtr;

		CIppiImage Image8u(pImage1->Width(), pImage1->Height(), 1, pp8u);

		SmartConversionto8bit(*pImage1, Image8u, CRect(Image8u.Width() / 4, Image8u.Height() / 4, Image8u.Width() * 3 / 4, Image8u.Height() * 3 / 4));


		CreateImageMask(Image8u, BumpSizeInPixels);


		CIppiImage* pResult = new CIppiImage(pImage1->Width(), pImage1->Height(), 1, pp16s);
		ippiConvert_8u16s_C1R((const Ipp8u*)Image8u.DataPtr(), Image8u.Step(), (Ipp16s*)pResult->DataPtr(), pResult->Step(), pResult->Size());
		CWinApp::OnFileNew();
		PresentResult(pResult);

	}
}




void CSJIApp::OnInferencingRuninference()
{

	
	if (OpenPages.GetCount() >= 1)
	{

		CString SaveResultsDir = _T("C:\\Results");

		std::string modelPath = "C:\\Models\\DeepCNNmodel.onnx";  // Change this to your ONNX model path
		int BumpSizeInPixels = 14;
		enum FindBlobMethods SegMethod = SimpleBumpCV;
		enum BridgeCandidateSelectionMethods CandMethod = SuspectsOnly;

		if ((GetKeyState(VK_SHIFT)) & 0x8000)
		{
			CSelectOptions dlg;
			if (dlg.DoModal() == IDOK)
			{
	//			BumpSizeInPixels = dlg.m_BumpSize;
				switch (dlg.m_BumpSeg)
				{
				case 0:
					SegMethod = EdgeBased;
					break;
				case 1: 
					SegMethod = ImageRegistration;
					break;
				case 2: 
					SegMethod = SimpleBumpCV;
					break;
				}

				switch (dlg.m_CandSel)
				{
				case 0:
					CandMethod = SuspectsOnly;
					break;
				case 1:
		//			CandMethod = LookForNeighbours;
					break;
				case 2:
		//			CandMethod = AdaptiveThreshold;
					break;
				}

				switch (dlg.m_NNMod)
				{
				case 0:
					modelPath = "C:\\Models\\DeepCNNmodel.onnx";
					break;
				case 1:
					modelPath = "C:\\Models\\CustomResNet.onnx";
					break;
				case 2:
					modelPath = "C:\\Models\\ResNetReg.onnx";
					break;
				}
			}
		}

		CString strDir=CreateNewResultsDir(SaveResultsDir);

		std::vector<CBridgeResult> bridgeResults;

		for (int i = 0; i < OpenPages.GetCount(); i++)
		{
			CSJIDoc* pDoc1 = OpenPages.GetDocAt(i);
			CIppiImage* pImage1 = pDoc1->m_ImagePtr;

			CSegmenter Segmenter(pImage1, SegMethod, CandMethod, BumpSizeInPixels);
			std::vector<CCandidateBridge> candidateList = Segmenter.GetBridgeCandidates();
			
			
			bool retFlag=RunInference(modelPath, candidateList, bridgeResults);
			if (retFlag)
			{
				SaveResults(strDir, bridgeResults, pImage1);

				// ***** present the results ************************



				pDoc1->m_RectArray.RemoveAll();
				pDoc1->m_RectCol.RemoveAll();
				for (int i = 0; i < bridgeResults.size(); i++)
				{
					if (bridgeResults[i].m_bIsTrueBridge)
					{
						pDoc1->m_RectArray.Add(bridgeResults[i].boundingRect);
						pDoc1->m_RectCol.Add(2); //red
					}
					else
					{
						pDoc1->m_RectArray.Add(bridgeResults[i].boundingRect);
						pDoc1->m_RectCol.Add(1); //green
					}
				}
				PresentResult(pDoc1);
			}

			
			

			bridgeResults.clear();

		}



	}
}


void CSJIApp::OnInferencingTestsegments()
{
	
	if (OpenPages.GetCount() >= 1)
	{

		std::string modelPath = "C:\\Models\\DeepCNNmodel.onnx";  // Change this to your ONNX model path
		cv::dnn::Net net = cv::dnn::readNetFromONNX(modelPath);

		for (int i = 0; i < OpenPages.GetCount(); i++)
		{
			CSJIDoc* pDoc1 = OpenPages.GetDocAt(i);
			pDoc1->m_RectArray.RemoveAll();
			pDoc1->m_RectCol.RemoveAll();
			
			CIppiImage* pSegment = pDoc1->m_ImagePtr;
			CIppiImage Image8u(pSegment->Width(), pSegment->Height(),1,pp8u);
			ippiConvert_16s8u_C1R((const Ipp16s*)pSegment->DataPtr(), pSegment->Step(), (Ipp8u*)Image8u.DataPtr(), Image8u.Step(), Image8u.Size());

			cv::Mat finalCropped = cv::Mat(pSegment->Height(), pSegment->Width(), CV_8UC1, Image8u.DataPtr(), Image8u.Step());
			finalCropped.convertTo(finalCropped, CV_8U);

			int width = finalCropped.cols;
			int height = finalCropped.rows;

			int segmentsize = 56;

			int new_width, new_height;
			if (width > height) {
				new_width = segmentsize;
				new_height = segmentsize;
			}
			else {
				new_height = segmentsize;
				new_width = segmentsize;
			}

			// Resize the image while keeping the aspect ratio
			cv::Mat resizedImg;
			cv::resize(finalCropped, resizedImg, cv::Size(new_width, new_height), 0, 0, cv::INTER_LINEAR);


			cv::Mat finalImg(segmentsize, segmentsize, resizedImg.type(), cv::Scalar(255, 255, 255));

			// Compute the position to center the resized image
			int x_offset = (segmentsize - new_width) / 2;
			int y_offset = (segmentsize - new_height) / 2;

			// Place resized image in the center of the white background
			resizedImg.copyTo(finalImg(cv::Rect(x_offset, y_offset, new_width, new_height)));

			cv::Mat blob;
			blob=cv::dnn::blobFromImage(finalImg, 1.0 / 255.0, cv::Size(segmentsize, segmentsize), cv::Scalar(0), false, false);

			// Set the blob as the input to the network
			net.setInput(blob);


			// Run forward pass to classify the image
			cv::Mat output = net.forward();

			// Interpret output: Find the index of the highest confidence score
			cv::Point classIdPoint;
			double confidence;
			cv::minMaxLoc(output, nullptr, &confidence, nullptr, &classIdPoint);
			int classId = classIdPoint.x;

			pDoc1->m_RectArray.Add(CRect(5, 5, 20, 20));
			pDoc1->m_RectCol.Add(classId+1);
			PresentResult(pDoc1);
		}
	}
}


void CSJIApp::OnInferencingRuninferenceonfolder()
{
	if (OpenPages.GetCount() >= 1)
	{
		CArray<CString, CString>m_AllProjNames;

		CSJIDoc* pDoc1 = OpenPages.GetDocAt(0);
		CString PathName = (CString)pDoc1->GetPathName();
		FindProjectionsInDir(PathName, m_AllProjNames);



			//********************************************************************************************************************
			CString SaveResultsDir = _T("C:\\Results");
			CString strDir = CreateNewResultsDir(SaveResultsDir);
			std::string modelPath = "C:\\Models\\DeepCNNmodel.onnx";  // Change this to your ONNX model path
			int BumpSizeInPixels = 14;
			enum FindBlobMethods SegMethod = SimpleBumpCV;
			enum BridgeCandidateSelectionMethods CandMethod = SuspectsOnly;

			if ((GetKeyState(VK_SHIFT)) & 0x8000)
			{
				CSelectOptions dlg;
				if (dlg.DoModal() == IDOK)
				{
					//			BumpSizeInPixels = dlg.m_BumpSize;
					switch (dlg.m_BumpSeg)
					{
					case 0:
						SegMethod = EdgeBased;
						break;
					case 1:
						SegMethod = ImageRegistration;
						break;
					case 2:
						SegMethod = SimpleBumpCV;
						break;
					}

					switch (dlg.m_CandSel)
					{
					case 0:
						CandMethod = SuspectsOnly;
						break;
					case 1:
						//			CandMethod = LookForNeighbours;
						break;
					case 2:
						//			CandMethod = AdaptiveThreshold;
						break;
					}

					switch (dlg.m_NNMod)
					{
					case 0:
						modelPath = "C:\\Models\\DeepCNNmodel.onnx";
						break;
					case 1:
						modelPath = "C:\\Models\\CustomResNet.onnx";
						break;
					case 2:
						modelPath = "C:\\Models\\ResNetReg.onnx";
						break;
					}
				}
			}


			std::vector<CBridgeResult> bridgeResults;

			pDoc1 = OpenPages.GetDocAt(0);
			CIppiImage* pImage0 = pDoc1->m_ImagePtr;
			for (int i = 0; i < m_AllProjNames.GetCount(); i++)
			{
				
				CString PathName = m_AllProjNames.GetAt(i);
				CIppiImage Image16s(pImage0->Width(), pImage0->Height(),1,pp16s);
				CIppiImage* pImage1 = &Image16s;
				pImage1->LoadImage(PathName);
				

				CSegmenter Segmenter(pImage1, SegMethod, CandMethod, BumpSizeInPixels);
				std::vector<CCandidateBridge> candidateList = Segmenter.GetBridgeCandidates();


				bool retFlag = RunInference(modelPath, candidateList, bridgeResults);
				if (retFlag)
				{
					// store image segments to the dedicated result folders (bridge, notbridge) ****************************************
					SaveResults(strDir, bridgeResults, pImage1);
				}


				bridgeResults.clear();

			}

			//********************************************************************************************************************
		


	}
}


void CSJIApp::OnAugmentationCreate1000()
{

	if (OpenPages.GetCount() == 2)
	{
		CSJIDoc* pDoc1 = OpenPages.GetDocAt(0);
		CString PathName = (CString)pDoc1->GetPathName();
		int width = pDoc1->m_ImagePtr->Width();
		int height = pDoc1->m_ImagePtr->Height();

		CSJIDoc* pDoc2 = OpenPages.GetDocAt(1);
		CString PathName2 = (CString)pDoc2->GetPathName();

		CString PathNameBridge;
		CString  PathNameNonBridge;
		int tmpPos = PathName.Find(_T("Non-Bridge"), 0);
		if (tmpPos != -1)
		{
			PathNameNonBridge = PathName;
			PathNameBridge = PathName2;
		}
		else
		{
			PathNameNonBridge = PathName2;
			PathNameBridge = PathName;

		}

		CArray<CString, CString> bridgeArray;
		CArray<CString, CString> nonBridgeArray;

		FindProjectionsInDir(PathNameBridge, bridgeArray);
		FindProjectionsInDir(PathNameNonBridge, nonBridgeArray);

		CString DestinationPath = _T("C:\\synthBridge");
		CreateDirectory(DestinationPath,0);
		CString strDir = CreateNewResultsDir(DestinationPath);

		CIppiImage segment01(width,height,1,pp8u);
		CIppiImage segment02(width, height, 1, pp8u);
		CIppiImage segment03(width, height, 1, pp8u);
		CIppiImage nbsegment(width, height, 1, pp8u);

		CIppiImage resultImage(width, height, 1, pp8u);

		int index;
		for (int i = 0;i < 1000;i++) 
		{
			index=int(((double)rand()/RAND_MAX)* bridgeArray.GetCount());
			segment01.LoadImage(bridgeArray.GetAt(index));
			//ApplyOperator(&segment01);

			index = int(((double)rand() / RAND_MAX) * bridgeArray.GetCount());
			segment02.LoadImage(bridgeArray.GetAt(index));
			//ApplyOperator(&segment02);

			index = int(((double)rand() / RAND_MAX) * bridgeArray.GetCount());
			segment03.LoadImage(bridgeArray.GetAt(index));
			//ApplyOperator(&segment03);

			index = int(((double)rand() / RAND_MAX) * nonBridgeArray.GetCount());
			nbsegment.LoadImage(nonBridgeArray.GetAt(index));

			SynthBridge(segment01, segment02, segment03, nbsegment,resultImage);

			CString str;
			str.Format(_T("%s\\sb_%04d.bmp"), strDir, i + 1);
			resultImage.SaveImage(str);
		}
	}
}
