#include "StdAfx.h"
#include "OPenPages.h"
#include "SJIDoc.h"
#include "SJIView.h"
//#include "Ippi16ImageEx.h"
//#include "ProtectedTypes.h"
#include "ippiImage.h"
#include "IppiWrapper.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

COPenPages::COPenPages(void)
{
}


COPenPages::~COPenPages(void)
{
}


void COPenPages::AddView(CSJIView *pView)
{


	if(pView!=NULL)
	{
		m_ViewArray.InsertAt(0,pView);
	}

}

void COPenPages::AddDoc(CSJIDoc *pDoc)
{


	if(pDoc!=NULL)
	{
		m_DocArray.InsertAt(0,pDoc);
	}
}

void COPenPages::RemoveView(CSJIView *pView)
{


	if(pView!=NULL)
	{
		
		for(int i=0;i<m_ViewArray.GetCount();i++)
		{
			CSJIView *pViewCurr=m_ViewArray.GetAt(i);
			if(pViewCurr==pView)
			{
				m_ViewArray.RemoveAt(i);
				break;
			}
		}
	}
}

void COPenPages::RemoveDoc(CSJIDoc *pDoc)
{


	if(pDoc!=NULL)
	{
		
		for(int i=0;i<m_DocArray.GetCount();i++)
		{
			CSJIDoc *pDocCurr=m_DocArray.GetAt(i);
			if(pDocCurr==pDoc)
			{
				m_DocArray.RemoveAt(i);
				break;
			}
		}
	}
}

int COPenPages::GetCount()
{
	ASSERT((int)m_DocArray.GetCount()==(int)m_ViewArray.GetCount());


	return (int)m_DocArray.GetCount();
}

CSJIView *COPenPages::GetViewAt(int i)
{


	if(i<m_ViewArray.GetCount())
	{
		CSJIView *pViewCurr=m_ViewArray.GetAt(i);
		return pViewCurr;
	}
	else
	{
		return NULL;
	}
}

CSJIDoc *COPenPages::GetDocAt(int i)
{


	if(i<m_DocArray.GetCount())
	{
		CSJIDoc *pDocCurr=m_DocArray.GetAt(i);
		return pDocCurr;
	}
	else
	{
		return NULL;
	}

	return NULL;
}

CSJIView *COPenPages::GetCurrentView()
{
	return GetViewAt(0);
}

CSJIDoc *COPenPages::GetCurrentDoc()
{
	return GetDocAt(0);
}

void COPenPages::UpdateCurrentOpenPage(CSJIDoc *pDoc)
{
	for(int i=0;i<m_DocArray.GetCount();i++)
	{
		CSJIDoc *pDocCurr=m_DocArray.GetAt(i);
		if(pDoc==pDocCurr)
		{
			m_DocArray.RemoveAt(i);
			m_DocArray.InsertAt(0,pDoc);
			break;
		}
	}
}

void COPenPages::UpdateCurrentOpenPage(CSJIView *pView)
{
	for(int i=0;i<m_ViewArray.GetCount();i++)
	{
		CSJIView *pViewCurr=m_ViewArray.GetAt(i);
		if(pView==pViewCurr)
		{
			m_ViewArray.RemoveAt(i);
			m_ViewArray.InsertAt(0,pView);
			break;
		}
	}
}

void COPenPages::GetImageArray(CArray<CIppiImage *,CIppiImage *>&ImageArray)
{
	ImageArray.RemoveAll();
	for(int i=0;i<m_DocArray.GetCount();i++)
	{
		CSJIDoc *pDocCurr=m_DocArray.GetAt(i);
		if(pDocCurr!=NULL)
		{
			CIppiImage *pImage=pDocCurr->m_ImagePtr;
			if(pImage!=NULL && pImage->DataPtr()!=NULL)
			{
				ImageArray.Add(pImage);
			}
		}
	}

}


