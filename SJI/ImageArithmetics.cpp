// ImageArithmetics.cpp : implementation file
//

#include "stdafx.h"

#include "ImageArithmetics.h"
#include "ippi.h"
#include "ippcv.h"
#include "ippiImage.h"




#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

void Add(CIppiImage *pImage1, CIppiImage *pImage2, CIppiImage *pResultImage)
{
	ippiAdd_16s_C1RSfs((const Ipp16s*) pImage1->DataPtr(), pImage1->Step(),
						(const Ipp16s*) pImage2->DataPtr(), pImage2->Step(), 
						(Ipp16s*) pResultImage->DataPtr(), pResultImage->Step(), pResultImage->Size(),0);
}

void AbsDifference(CIppiImage *pImage1, CIppiImage *pImage2, CIppiImage *pResultImage)
{
	ippiAbsDiff_16u_C1R((const Ipp16u*) pImage1->DataPtr(), pImage1->Step(),
						(const Ipp16u*) pImage2->DataPtr(), pImage2->Step(), 
						(Ipp16u*) pResultImage->DataPtr(), pResultImage->Step(), pResultImage->Size());
}

void MergeMax(CIppiImage *pImage1, CIppiImage *pImage2, CIppiImage *pResultImage)
{
	ippiCopy_16s_C1R((const Ipp16s*) pImage1->DataPtr(), pImage1->Step(),
							(Ipp16s*) pResultImage->DataPtr(), pResultImage->Step(), pResultImage->Size());
	ippiMaxEvery_16s_C1IR((const Ipp16s*)pImage2->DataPtr(), pImage2->Step(),
									(Ipp16s*) pResultImage->DataPtr(), pResultImage->Step(), pResultImage->Size());
}

void MergeMin(CIppiImage *pImage1, CIppiImage *pImage2, CIppiImage *pResultImage)
{
	ippiCopy_16s_C1R((const Ipp16s*) pImage1->DataPtr(), pImage1->Step(),
							(Ipp16s*) pResultImage->DataPtr(), pResultImage->Step(), pResultImage->Size());
	ippiMinEvery_16s_C1IR((const Ipp16s*)pImage2->DataPtr(), pImage2->Step(),
									(Ipp16s*) pResultImage->DataPtr(), pResultImage->Step(), pResultImage->Size());
}


void NegativeImage(CIppiImage *pImage1)
{
	Ipp16s Max, Min;
	ippiMinMax_16s_C1R((const Ipp16s*) pImage1->DataPtr(), pImage1->Step(), pImage1->Size(), &Min, &Max);
	int BitDepth=int(log((double)Max-1)/log(2.0))+1;
	int MaxValue=(int)(pow(2.0,(double)BitDepth))-1;
	MaxValue=max(255,MaxValue);

	CIppiImage tmpImage(*pImage1);

	ippiSet_16s_C1R(Ipp16s((int)MaxValue), (Ipp16s*) pImage1->DataPtr(), pImage1->Step(), pImage1->Size());
	ippiSub_16s_C1IRSfs((const Ipp16s*) tmpImage.DataPtr(), tmpImage.Step(),
								(Ipp16s*) pImage1->DataPtr(), pImage1->Step(), pImage1->Size(), 0);
}

void AddConstant(CIppiImage *pImage1, double Value)
{

		ippiAddC_16s_C1IRSfs((Ipp16s)Value, (Ipp16s*) pImage1->DataPtr(), pImage1->Step(), pImage1->Size(), 0);


}


void AverageAll(CArray<CIppiImage *,CIppiImage *>&ImageArray, CIppiImage *pResultImage)
{
	CIppiImage ResultImage32f(pResultImage->Width(),pResultImage->Height(),1,pp32f);
	CIppiImage tmp32f(pResultImage->Width(),pResultImage->Height(),1,pp32f);

	ResultImage32f.Set(0);
	int count=(int)ImageArray.GetCount();
	for(int i=0;i<count;i++)
	{
		CIppiImage *pImage=ImageArray.GetAt(i);
		if(pImage!=NULL && pImage->DataPtr()!=NULL)
		{
			ippiConvert_16s32f_C1R((const Ipp16s*) pImage->DataPtr(), pImage->Step(), 
										   (Ipp32f*) tmp32f.DataPtr(), tmp32f.Step(), tmp32f.Size());
			ippiAdd_32f_C1IR((const Ipp32f*) tmp32f.DataPtr(), tmp32f.Step(),
								(Ipp32f*) ResultImage32f.DataPtr(), ResultImage32f.Step(), ResultImage32f.Size());
		}
	}

	ippiDivC_32f_C1IR((Ipp32f) count, (Ipp32f*) ResultImage32f.DataPtr(), ResultImage32f.Step(), ResultImage32f.Size());
	ippiConvert_32f16s_C1R((const Ipp32f*) ResultImage32f.DataPtr(), ResultImage32f.Step(),
						(Ipp16s*) pResultImage->DataPtr(), pResultImage->Step(), pResultImage->Size(), ippRndNear);

	ImageArray.RemoveAll();
}

void MergeMaxAll(CArray<CIppiImage *,CIppiImage *>&ImageArray, CIppiImage *pResultImage)
{
	if((int)ImageArray.GetCount()>0)
	{
		CIppiImage *pImage=ImageArray.GetAt(0);
		ippiCopy_16s_C1R((const Ipp16s*) pImage->DataPtr(), pImage->Step(),
							(Ipp16s*) pResultImage->DataPtr(), pResultImage->Step(), pResultImage->Size());
		for(int i=1;i<ImageArray.GetCount();i++)
		{
			CIppiImage *pImage=ImageArray.GetAt(i);
			ippiMaxEvery_16s_C1IR((const Ipp16s*)pImage->DataPtr(), pImage->Step(),
									(Ipp16s*) pResultImage->DataPtr(), pResultImage->Step(), pResultImage->Size());
		}
	}

	ImageArray.RemoveAll();
}

void MergeMinAll(CArray<CIppiImage *,CIppiImage *>&ImageArray, CIppiImage *pResultImage)
{
	if((int)ImageArray.GetCount()>0)
	{
		CIppiImage *pImage=ImageArray.GetAt(0);
		ippiCopy_16s_C1R((const Ipp16s*) pImage->DataPtr(), pImage->Step(),
							(Ipp16s*) pResultImage->DataPtr(), pResultImage->Step(), pResultImage->Size());
		for(int i=1;i<ImageArray.GetCount();i++)
		{
			CIppiImage *pImage=ImageArray.GetAt(i);
			ippiMinEvery_16s_C1IR((const Ipp16s*)pImage->DataPtr(), pImage->Step(),
									(Ipp16s*) pResultImage->DataPtr(), pResultImage->Step(), pResultImage->Size());
		}
	}

	ImageArray.RemoveAll();
}

void Resize(CIppiImage *pImage1, CIppiImage *pResultImage)
{
	double ResizeX=(double)pResultImage->Width()/(double)pImage1->Width();
	double ResizeY=(double)pResultImage->Height()/(double)pImage1->Height();

	IppiRect srcRect={0,0,pImage1->Width(),pImage1->Height()};
	IppiRect dstRect={0,0,pResultImage->Width(),pResultImage->Height()};
	

	if(ResizeX>=1 && ResizeY>=1)
	{
		IppiResizeSpec_32f* pSpec = 0;
		int specSize = 0, initSize = 0, bufSize = 0;
		Ipp8u* pBuffer = 0;
		Ipp8u* pInitBuf = 0;
		IppiPoint dstOffset = { 0, 0 };

		/* cubic interpolation coefficients */
		Ipp32f valueB = 0.f, valueC = 0.5f; /* Catmull-Rom spline */

		/* Spec and init buffer sizes */
		IppStatus status = ippiResizeGetSize_16s(pImage1->Size(), pResultImage->Size(), ippLanczos, 0, &specSize, &initSize);

		pInitBuf = ippsMalloc_8u(initSize);
		pSpec = (IppiResizeSpec_32f*)ippsMalloc_8u(specSize);
		if (pInitBuf == NULL || pSpec == NULL)
		{
			ippsFree(pInitBuf);
			ippsFree(pSpec);
			return; /* ippStsNoMemErr; */
		}


		/* Filter initialization */
		status = ippiResizeCubicInit_8u(pImage1->Size(), pResultImage->Size(), valueB, valueC, pSpec, pInitBuf);
		ippsFree(pInitBuf);

		if (status != ippStsNoErr)
		{
			ippsFree(pSpec);
			return; /* status; */
		}

		/* work buffer size */
		status = ippiResizeGetBufferSize_8u(pSpec, pResultImage->Size(), 1, &bufSize);
		if (status != ippStsNoErr)
		{
			ippsFree(pSpec);
			return; /* status; */
		}

		pBuffer = ippsMalloc_8u(bufSize);
		if (pBuffer == NULL)
		{
			ippsFree(pSpec);
			return; /* ippStsNoMemErr;*/
		}

		/* Resize processing */
		status = ippiResizeCubic_16s_C1R((const Ipp16s*)pImage1->DataPtr(), pImage1->Step(), (Ipp16s*)pResultImage->DataPtr(), pResultImage->Step(), dstOffset, pResultImage->Size(), ippBorderRepl, 0, pSpec, pBuffer);

		ippsFree(pSpec);
		ippsFree(pBuffer);

	}
	else
	{
		IppiResizeSpec_32f* pSpec = 0;
		int specSize = 0, initSize = 0, bufSize = 0;
		Ipp8u* pBuffer = 0;
		IppiPoint dstOffset = { 0, 0 };

		///* cubic interpolation coefficients */
		//Ipp32f valueB = 0.f, valueC = 0.5f; /* Catmull-Rom spline */

		/* Spec and init buffer sizes */
		IppStatus status = ippiResizeGetSize_16s(pImage1->Size(), pResultImage->Size(), ippSuper, 0, &specSize, &initSize);

		pSpec = (IppiResizeSpec_32f*)ippsMalloc_8u(specSize);
		if ( pSpec == NULL)
		{
			ippsFree(pSpec);
			return; /* ippStsNoMemErr; */
		}


		/* Filter initialization */
		status = ippiResizeSuperInit_8u(pImage1->Size(), pResultImage->Size(), pSpec);

		if (status != ippStsNoErr)
		{
			ippsFree(pSpec);
			return; /* status; */
		}

		/* work buffer size */
		status = ippiResizeGetBufferSize_8u(pSpec, pResultImage->Size(), 1, &bufSize);
		if (status != ippStsNoErr)
		{
			ippsFree(pSpec);
			return; /* status; */
		}

		pBuffer = ippsMalloc_8u(bufSize);
		if (pBuffer == NULL)
		{
			ippsFree(pSpec);
			return; /* ippStsNoMemErr;*/
		}

		/* Resize processing */
		status = ippiResizeSuper_16s_C1R((const Ipp16s*)pImage1->DataPtr(), pImage1->Step(), (Ipp16s*)pResultImage->DataPtr(), pResultImage->Step(), dstOffset, pResultImage->Size(), pSpec, pBuffer);

		ippsFree(pSpec);
		ippsFree(pBuffer);

	}
	
}



void Mirror(BOOL bHorizontal, CIppiImage *pImage1)
{
	if(bHorizontal)
	{
		ippiMirror_16u_C1IR((Ipp16u*)pImage1->DataPtr(), pImage1->Step(), pImage1->Size(), ippAxsHorizontal);
	}
	else
	{
		ippiMirror_16u_C1IR((Ipp16u*)pImage1->DataPtr(), pImage1->Step(), pImage1->Size(), ippAxsVertical);
	}
}

