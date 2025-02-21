
// SJI.h : main header file for the SJI application
//
#pragma once

#ifndef __AFXWIN_H__
	#error "include 'stdafx.h' before including this file for PCH"
#endif

#include "resource.h"       // main symbols


// CSJIApp:
// See SJI.cpp for the implementation of this class
//
#include "OPenPages.h"
#include "BridgeResult.h"
#include <vector>

class CSJIApp : public CWinAppEx
{
public:
	CSJIApp();


// Overrides
public:
	virtual BOOL InitInstance();
	virtual int ExitInstance();

// Implementation
	UINT  m_nAppLook;
	BOOL  m_bHiColorIcons;

	virtual void PreLoadState();
	virtual void LoadCustomState();
	virtual void SaveCustomState();
	void PresentResult(CIppiImage *pResult);
	void PresentResult(CSJIDoc *pDoc);


	COPenPages OpenPages;
	afx_msg void OnAppAbout();
	DECLARE_MESSAGE_MAP()
	afx_msg void OnSegmentationEdgebasedsegment();


private:
	void SaveResults(CString strDir, std::vector<CBridgeResult>& bridgeResults, CIppiImage* pInspectionImage);
	CString CreateNewResultsDir(CString SaveResultsDir);
public:
	afx_msg void OnSegmentationImageregistration();
	afx_msg void OnSegmentationSimplebumpcv();
	afx_msg void OnTestfunctionsAdaptivethreashold();

	afx_msg void OnInferencingRuninference();
	afx_msg void OnInferencingTestsegments();
};

extern CSJIApp theApp;
