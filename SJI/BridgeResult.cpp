#include "stdafx.h"
#include "BridgeResult.h"



#include "ippiImage.h"

#define ACCEPTANCETHRESHOLD 0.5f

CBridgeResult::CBridgeResult()
	: m_bIsTrueBridge(false)
	, m_fnotBridge(0)
	, m_fbridge(0)

{
}

CBridgeResult::CBridgeResult(CCandidateBridge candidate, float fbridge, float fnotBridge)
{
	xpix= candidate.xpix;
	ypix= candidate.ypix;
	sizex= candidate.sizex;
	sizey= candidate.sizey;
	xreal= candidate.xreal;
	yreal= candidate.yreal;
	boundingRect= candidate.boundingRect;
	std::vector<int> listBumpIDs= candidate.listBumpIDs;
	std::vector<CPoint>listBumpPixPos= candidate.listBumpPixPos;
	pBridgeImg=new CIppiImage(*candidate.pBridgeImg);



	if(fbridge> ACCEPTANCETHRESHOLD)
	{
		m_bIsTrueBridge = true;
	}
	else
	{
		m_bIsTrueBridge = false;
	}

	m_fbridge = fbridge;
	m_fnotBridge = fnotBridge;
}

CBridgeResult::~CBridgeResult()
{

}

CCandidateBridge::CCandidateBridge()
	: xpix(-1)
	, ypix(-1)
	, sizex(-1)
	, sizey(-1)
	, xreal(-1)
	, yreal(-1)
	, boundingRect(0, 0, 0, 0)
	, pBridgeImg(NULL)
	, lp1(-1)
	, BridgeID(-1)
{
}

CCandidateBridge::~CCandidateBridge()
{
	listBumpIDs.clear();
	listBumpPixPos.clear();

}
