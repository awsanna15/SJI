#pragma once

class CIppiImage;

void Add(CIppiImage *pImage1, CIppiImage *pImage2, CIppiImage *pResultImage);
void AbsDifference(CIppiImage *pImage1, CIppiImage *pImage2, CIppiImage *pResultImage);
void MergeMax(CIppiImage *pImage1, CIppiImage *pImage2, CIppiImage *pResultImage);
void MergeMin(CIppiImage *pImage1, CIppiImage *pImage2, CIppiImage *pResultImage);


void NegativeImage(CIppiImage *pImage1);

void AddConstant(CIppiImage *pImage1,double Value);



void AverageAll(CArray<CIppiImage *,CIppiImage *>&ImageArray, CIppiImage *pResultImage);
void MergeMaxAll(CArray<CIppiImage *,CIppiImage *>&ImageArray, CIppiImage *pResultImage);
void MergeMinAll(CArray<CIppiImage *,CIppiImage *>&ImageArray, CIppiImage *pResultImage);


void Resize(CIppiImage *pImage1, CIppiImage *pResultImage);

void Mirror(BOOL bHorizontal, CIppiImage *pImage1);
