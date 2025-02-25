#pragma once
#include "ippiImage.h"
#include "IppiWrapperAlias.h"
#include "BridgeResult.h"
#include <string>

int Round(const double d);
void SmartConversionto8bit(CIppiImage& SrcImage, CIppiImage& ResultImage, CRect roi);
void  FindTemplate(CIppiImage* pImage1, CIppiImage* pImage2, CIppiImage* pResultImage, CPoint& MaxPos, double& MaxVal);
void CreateBumpTemplate(CIppiImage& tmpBumpTemplate, const int BumpSizeInPixels);
void CreateImageMask(CIppiImage& Image8u, int BumpSizeInPixels);
bool dirExists(CStringA strDir);
void GetSubdirs(std::vector<CString>& output, const CString& path);
bool RunInference(std::string& modelPath, std::vector<CCandidateBridge>& candidateList, std::vector<CBridgeResult>& bridgeResults);