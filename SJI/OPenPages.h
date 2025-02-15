#pragma once
#include "afxmt.h"
#include "afxtempl.h"

class CSJIDoc;
class CSJIView;
class CIppiImage;

class COPenPages
{
public:
	COPenPages(void);
	~COPenPages(void);

public:
	void AddView(CSJIView *pView);
	void AddDoc(CSJIDoc *pDoc);

	void RemoveView(CSJIView *pView);
	void RemoveDoc(CSJIDoc *pDoc);

	int GetCount();

	CSJIView *GetViewAt(int i);
	CSJIDoc *GetDocAt(int i);

	CSJIView *GetCurrentView();
	CSJIDoc *GetCurrentDoc();

	void UpdateCurrentOpenPage(CSJIDoc *pDoc);
	void UpdateCurrentOpenPage(CSJIView *pView);

	void GetImageArray(CArray<CIppiImage *,CIppiImage *>&ImageArray);

private:

	 CCriticalSection m_csViews;
	 CCriticalSection m_csDocs;

	 CArray<CSJIView *, CSJIView *> m_ViewArray;
	 CArray<CSJIDoc *, CSJIDoc *> m_DocArray;

//	 int m_Current;
};

