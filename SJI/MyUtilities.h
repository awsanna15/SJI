#pragma once
#include "ippiImage.h"
#include "IppiWrapperAlias.h"

int Round(const double d);
void SmartConversionto8bit(CIppiImage& SrcImage, CIppiImage& ResultImage);
void  FindTemplate(CIppiImage* pImage1, CIppiImage* pImage2, CIppiImage* pResultImage, CPoint& MaxPos, double& MaxVal);
void CreateBumpTemplate(CIppiImage& tmpBumpTemplate, const int BumpSizeInPixels);
void CreateImageMask(CIppiImage& Image8u, int BumpSizeInPixels);
bool dirExists(CStringA strDir);
