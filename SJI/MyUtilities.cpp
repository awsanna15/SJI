#include "stdafx.h"
#include "MyUtilities.h"

#include <opencv2/opencv.hpp>
#include "opencv2/dnn.hpp"

using namespace cv;
using namespace std;

int Round(const double d)
{
	const double MAX_DOUBLE = INT_MAX - 0.5;
	const double MIN_DOUBLE = INT_MIN + 0.5;

	if (d > 0.0 && d < MAX_DOUBLE)
	{
		return static_cast<int>(d + 0.5);
	}
	else if (d > MIN_DOUBLE && d < 0.0)
	{
		return static_cast<int>(d - 0.5);
	}
	else
	{
		//		ASSERT(0.0 == d);	// This ASSERT detects where d >= MAX_DOUBLE OR d <= MIN_DOUBLE
		return 0;
	}
} // Round

void SmartConversionto8bit(CIppiImage& SrcImage, CIppiImage& ResultImage, CRect roi)
{
	
	double LowerLim = 0.0;
	double UpperLim = 0.0;
	SrcImage.GetMinMaxMean(&LowerLim, &UpperLim, nullptr, 0);

	Ipp32s lowerLevel = 0;
	Ipp32s upperLevel;
	lowerLevel = static_cast<Ipp32s>(LowerLim) - 2;
	upperLevel = static_cast<Ipp32s>(UpperLim) + 1;
	const int nLevels = upperLevel - lowerLevel;
	Ipp32s* pHisto = new Ipp32s[nLevels];
	Ipp32s* pLevels = new Ipp32s[nLevels + 1];
	IppiSize sz = { roi.Width(), roi.Height()};
	ippiHistogramEven_16u_C1R((const Ipp16u*)SrcImage.Point(roi.left, roi.top), SrcImage.Step(), sz,
		(Ipp32s*)pHisto, (Ipp32s*)pLevels, (nLevels + 1), lowerLevel, upperLevel);
	int indlow = 0;
	int indhigh = nLevels + 1;
	int OnePc = Round(double(roi.Width() * roi.Height()) * 0.001);
	int sum = 0;
	for (indlow = 0; indlow < nLevels && sum < OnePc; indlow++)
	{
		sum += pHisto[indlow];
	}
	sum = 0;
	for (indhigh = nLevels - 1; indhigh >= 0 && sum < OnePc; indhigh--)
	{
		sum += pHisto[indhigh];
	}
	int low = pLevels[indlow];
	int hi = pLevels[indhigh + 1];
	low = Round(low - 2 * double(hi - low) / 98);
	hi = Round(hi + 2 * double(hi - low) / 98);

	//int maxval=0;
	//int maxinx=0;
	//for (int i = 0; i < nLevels ; i++)
	//{
	//	if (maxval < pHisto[i])
	//	{
	//		maxval = pHisto[i];
	//		maxinx = pLevels[i];
	//	}
	//}
	//hi = hi - 1.0 * (hi - maxinx);

	CIppiImage tmp32f(SrcImage.Width(), SrcImage.Height(), 1, pp32f);
	ippiConvert_16s32f_C1R((const Ipp16s*)SrcImage.DataPtr(), SrcImage.Step(), (Ipp32f*)tmp32f.DataPtr(), tmp32f.Step(), tmp32f.Size());
	ippiSubC_32f_C1IR((Ipp32f)low, (Ipp32f*)tmp32f.DataPtr(), tmp32f.Step(), tmp32f.Size());
	ippiMulC_32f_C1IR((Ipp32f)(double(256.0) / double(hi - low)), (Ipp32f*)tmp32f.DataPtr(), tmp32f.Step(), tmp32f.Size());
	ippiThreshold_LTValGTVal_32f_C1IR((Ipp32f*)tmp32f.DataPtr(), tmp32f.Step(), tmp32f.Size(), 0, 0, 255, 255);
	if (ResultImage.Type() == pp8u)
	{
		ippiConvert_32f8u_C1R((Ipp32f*)tmp32f.DataPtr(), tmp32f.Step(), (Ipp8u*)ResultImage.DataPtr(), ResultImage.Step(), ResultImage.Size(), ippRndNear);
	}
	else if (ResultImage.Type() == pp16s)
	{
		ippiConvert_32f16s_C1R((Ipp32f*)tmp32f.DataPtr(), tmp32f.Step(), (Ipp16s*)ResultImage.DataPtr(), ResultImage.Step(), ResultImage.Size(), ippRndNear);
	}
	

	delete[]pHisto;
	delete[]pLevels;
}

void CreateBumpTemplate(CIppiImage& tmpBumpTemplate, const int BumpSizeInPixels)
{

	tmpBumpTemplate.CreateImage(2 * BumpSizeInPixels - 1, 2 * BumpSizeInPixels - 1, 1, pp16s);
	tmpBumpTemplate.Set(255);
	double distance;
	CPoint cp = CPoint(tmpBumpTemplate.Width() / 2, tmpBumpTemplate.Height() / 2);
	for (int j = 0; j < tmpBumpTemplate.Height(); j++)
	{
		for (int i = 0; i < tmpBumpTemplate.Width(); i++)
		{
			distance = _hypot(i - cp.x, j - cp.y);
			if (distance <= BumpSizeInPixels / 2)
			{
				*(Ipp16s*)tmpBumpTemplate.Point(i, j) = 0;
			}
			else
			{
				*(Ipp16s*)tmpBumpTemplate.Point(i, j) = 255;
			}
		}
	}
}


void  FindTemplate(CIppiImage* pImage1, CIppiImage* pImage2, CIppiImage* pResultImage, CPoint& MaxPos, double& MaxVal)
{
	CIppiImage* pImage, * pTemplate;

	CIppiImage tmpImage(*pImage1);
	CIppiImage tmpTemplate(*pImage2);

	Ipp16s Max1, Max2;
	ippiMax_16s_C1R((const Ipp16s*)tmpImage.DataPtr(), tmpImage.Step(), tmpImage.Size(), &Max1);
	ippiMax_16s_C1R((const Ipp16s*)tmpTemplate.DataPtr(), tmpTemplate.Step(), tmpTemplate.Size(), &Max2);

	int DivC = int(int(Max1) / 256) + 1;
	if (DivC > 1)
	{
		ippiDivC_16s_C1IRSfs(DivC, (Ipp16s*)tmpImage.DataPtr(), tmpImage.Step(), tmpImage.Size(), 0);
	}
	DivC = int(int(Max2) / 256) + 1;
	if (DivC > 1)
	{
		ippiDivC_16s_C1IRSfs(DivC, (Ipp16s*)tmpTemplate.DataPtr(), tmpTemplate.Step(), tmpTemplate.Size(), 0);
	}

	pImage = new CIppiImage(tmpImage.Width(), tmpImage.Height(), 1, pp8u);
	ippiConvert_16s8u_C1R((const Ipp16s*)tmpImage.DataPtr(), tmpImage.Step(), (Ipp8u*)pImage->DataPtr(), pImage->Step(), pImage->Size());

	CIppiImage dst(pImage->Width(), pImage->Height(), 1, pp32f);
	dst.Set(0.0);


	pResultImage->Set(0.0);

	//*******************


	pTemplate = new CIppiImage(tmpTemplate.Width(), tmpTemplate.Height(), 1, pp8u);
	ippiConvert_16s8u_C1R((const Ipp16s*)tmpTemplate.DataPtr(), tmpTemplate.Step(), (Ipp8u*)pTemplate->DataPtr(), pTemplate->Step(), pTemplate->Size());
	pImage->SaveImage(_T("C:\\Temp\\pImage.bmp"));
	pTemplate->SaveImage(_T("C:\\Temp\\pTemplate.bmp"));
	IppStatus st;
	st = ippiCrossCorrValid_NormLevel_8u32f_C1R((Ipp8u*)pImage->DataPtr(), pImage->Step(), pImage->Size(),
		(Ipp8u*)pTemplate->DataPtr(), pTemplate->Step(), pTemplate->Size(),
		(Ipp32f*)dst.Point(pTemplate->Size().width / 2, pTemplate->Size().height / 2), dst.Step());

	//pImage->SaveImage(_T("c:\\Temp\\pImage.bmp"));
	//pTemplate->SaveImage(_T("c:\\Temp\\pTemplate.bmp"));
	delete pTemplate;




	//********************


	IppiPoint cMin, cMax;
	Ipp32f	fMax, fMin;
	ippiThreshold_LT_32f_C1IR((Ipp32f*)dst.DataPtr(), dst.Step(), dst.Size(), (Ipp32f)-1.0f);
	ippiMinMaxIndx_32f_C1R((const Ipp32f*)dst.DataPtr(), dst.Step(), dst.Size(),
		&fMin, &fMax, &cMin, &cMax);


	Ipp32f Multiplicator = ((Ipp32f)(1000.0 * fMax) / (fMax - fMin));
	ippiAddC_32f_C1IR((Ipp32f)-fMin, (Ipp32f*)dst.DataPtr(), dst.Step(), dst.Size());
	ippiMulC_32f_C1IR((Ipp32f)Multiplicator, (Ipp32f*)dst.DataPtr(), dst.Step(), dst.Size());

	ippiConvert_32f16s_C1R((const Ipp32f*)dst.DataPtr(), dst.Step(),
		(Ipp16s*)pResultImage->DataPtr(), pResultImage->Step(), dst.Size(), ippRndNear);


	delete pImage;
	double resfVal = (double)(1000.0 * fMax);


	MaxPos.x = (int)cMax.x;
	MaxPos.y = (int)cMax.y;
	MaxVal = resfVal;
}

void CreateImageMask(CIppiImage& Image8u, int BumpSizeInPixels)
{
	cv::Mat gray = cv::Mat(Image8u.Height(), Image8u.Width(), CV_8UC1, Image8u.DataPtr(), Image8u.Step());

	Mat image_copy = gray.clone();
	// Apply Gaussian Blur to reduce noise

	cv::Mat blurred;
	cv::medianBlur(gray, blurred, (BumpSizeInPixels / 2) * 2 - 1);
	//cv::imshow("Blurred Image", blurred);
	//cv::waitKey(0);

	// Apply adaptive thresholding to extract dark areas (solder joints)
	cv::Mat thresh;
	cv::adaptiveThreshold(blurred, thresh, 255, cv::ADAPTIVE_THRESH_GAUSSIAN_C, cv::THRESH_BINARY_INV, (BumpSizeInPixels) * 2 + 1, 2);

	cv::Mat morph;
	cv::morphologyEx(thresh, morph, cv::MORPH_DILATE, cv::Mat::ones((BumpSizeInPixels/3)+1, (BumpSizeInPixels/3)+1, CV_8UC1));


	ippiCopy_8u_C1R((const Ipp8u*)morph.data, (int)morph.step, (Ipp8u*)Image8u.DataPtr(), Image8u.Step(), Image8u.Size());

//	Image8u.MedianFilter(5,5);
}

bool dirExists(CStringA strDir)
{
	DWORD ftyp = GetFileAttributesA(strDir);
	if (ftyp == INVALID_FILE_ATTRIBUTES)
		return false;  //something is wrong with your path!

	if (ftyp & FILE_ATTRIBUTE_DIRECTORY)
		return true;   // this is a directory!

	return false;    // this is not a directory!
}


void GetSubdirs(std::vector<CString>& output, const CString& path)
{
	WIN32_FIND_DATA FindFileData;
	HANDLE hFind = INVALID_HANDLE_VALUE;

	CString Path = path + _T("\\*");

	const char* pBuffer = (const char*)Path.GetBuffer();

	hFind = FindFirstFile((LPCWSTR)pBuffer, &FindFileData);
	Path.ReleaseBuffer();


	if (hFind != INVALID_HANDLE_VALUE)
	{
		do
		{
			if ((FindFileData.dwFileAttributes | FILE_ATTRIBUTE_DIRECTORY) == FILE_ATTRIBUTE_DIRECTORY
				&& (FindFileData.cFileName[0] != '.'))
			{
				CString FullPath = path + _T("\\") + FindFileData.cFileName;
				GetSubdirs(output, FullPath);
				output.push_back(FullPath);
			}
		} while (FindNextFile(hFind, &FindFileData) != 0);
		FindClose(hFind);
	}


}


bool RunInference(std::string& modelPath, std::vector<CCandidateBridge>& candidateList, std::vector<CBridgeResult>& bridgeResults)
{
	bool retFlag = true;
	// do inference on each end every candidate ************************************************************
	//Use onnx model to classify images
	// Load the ONNX model

	cv::dnn::Net net = cv::dnn::readNetFromONNX(modelPath);
	if (net.empty())
	{
		std::cerr << "Error: Could not load the ONNX model!" << std::endl;
		retFlag = false;

	}
	else
	{
		for (int c = 0; c < candidateList.size(); c++)
		{
			CIppiImage* pSegment = candidateList[c].pBridgeImg;   // should be 8bit 224x224 image

			cv::Mat finalCropped = cv::Mat(pSegment->Height(), pSegment->Width(), CV_8UC1, pSegment->DataPtr(), pSegment->Step());
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
			blob = cv::dnn::blobFromImage(finalImg, 1.0 / 255.0, cv::Size(segmentsize, segmentsize), cv::Scalar(0), false, false);

			// Set the blob as the input to the network
			net.setInput(blob);


			// Run forward pass to classify the image
			cv::Mat output = net.forward();

			// Interpret output: Find the index of the highest confidence score
			cv::Point classIdPoint;
			double confidence;
			cv::minMaxLoc(output, nullptr, &confidence, nullptr, &classIdPoint);
			int classId = classIdPoint.x;

			//store inference results in result array
			float fbridge = classId == 1 ? (float)confidence : 1.0f - (float)confidence;
			float fnotbridge = 1.0f - fbridge;
			bridgeResults.push_back(CBridgeResult(candidateList[c], fbridge, fnotbridge));
		}
	}

	return retFlag;
}

