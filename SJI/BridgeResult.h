#pragma once

#include "stdafx.h"

#include <vector>	
class CIppiImage;

class CCandidateBridge
{
public:
	CCandidateBridge();
	~CCandidateBridge();
	int xpix;
	int ypix;
	int sizex;
	int sizey;
	float xreal;
	float yreal;
	CRect boundingRect;
	std::vector<int> listBumpIDs;
	std::vector<POSITION>listBumpPixPos;
	CIppiImage* pBridgeImg;

};

class CBridgeResult : public CCandidateBridge
{
public:
	CBridgeResult();
	CBridgeResult(CCandidateBridge candidate, float fbridge, float fnotBridge);
	~CBridgeResult();

	bool m_bIsTrueBridge;
	float m_fnotBridge;
	float m_fbridge;



};

