#include "stdafx.h"
#include "Segmenter.h"
#include "ippiImage.h"
#include "MyUtilities.h"
#include <opencv2/opencv.hpp>

using namespace cv;

CSegmenter::CSegmenter(CIppiImage* pInspectionImage, FindBlobMethods BlobMethod, BridgeCandidateSelectionMethods CandidateSelectMethod, int BumpSizeInPixels)
	: m_BlobMethod(BlobMethod)
	, m_CandidateSelectMethod(CandidateSelectMethod)
	, m_BumpSizeInPixels(BumpSizeInPixels)
{
	m_pInspectionImage = new CIppiImage(*pInspectionImage);
}


CSegmenter::~CSegmenter(void)
{
	ClearAll();
}

void CSegmenter::ClearAll()
{
	if (m_pInspectionImage != NULL)
	{
		delete m_pInspectionImage;
	}
	m_BumpLocations.clear();
	m_BridgeCandidates.clear();
}

std::vector<cv::Point> CSegmenter::GetBumpLocations()
{
	//********* calculate bump locations **************** use FindBlobMethods *************************
	switch (m_BlobMethod)
	{
		case EdgeBased:
		{
			// EdgeBased
			EdgeBasedBumpLocator();
			break;
		}
		case ImageRegistration:
		{
			// ImageRegistration
			ImageRegistrationBumpLocator();
			break;
		}
		case SimpleBumpCV:
		{
			// SimpleBumpCV
			SimpleBumpCVBumpLocator();
			break;
		}
	}
	return m_BumpLocations;
}

std::vector<CCandidateBridge> CSegmenter::GetBridgeCandidates()
{
	m_BumpLocations = GetBumpLocations();

	//TODO:
	//********* calculate bridge candidates **************** use BridgeCandidateSelectionMethods *************************
	// locate, rotate and rescale potential bridge; the image segmetns should be as such will be used to run inverence using nn
	switch (m_CandidateSelectMethod) {
	case AdaptiveThreshold:
		AdaptiveCandidateSelection();
		break;
	case LookForNeighbours:
		break;
	}

	//******************************************************
	return m_BridgeCandidates;
}


void CSegmenter::EdgeBasedBumpLocator()
{
	CIppiImage* pImage1 = m_pInspectionImage;

	CIppiImage Image8u(pImage1->Width(), pImage1->Height(), 1, pp8u);

	SmartConversionto8bit(*pImage1, Image8u);

	cv::Mat gray = cv::Mat(Image8u.Height(), Image8u.Width(), CV_8UC1, Image8u.DataPtr(), Image8u.Step());

	Mat image_copy = gray.clone();
	// Apply Gaussian Blur to reduce noise

	cv::Mat blurred;
	cv::GaussianBlur(gray, blurred, cv::Size(9, 9), 2);
	//cv::imshow("Blurred Image", blurred);
	//cv::waitKey(0);

	// Apply adaptive thresholding to extract dark areas (solder joints)
	cv::Mat thresh;
	cv::adaptiveThreshold(blurred, thresh, 255, cv::ADAPTIVE_THRESH_GAUSSIAN_C, cv::THRESH_BINARY_INV, 11, 2);

	//cv::namedWindow("Thresholded Image", cv::WINDOW_KEEPRATIO);
//cv::imshow("Thresholded Image", thresh);
//cv::resizeWindow("Thresholded Image", 400, 400);
//cv::waitKey(0);

// Apply Morphological Opening (to remove small noise)
	cv::Mat morph;
	cv::morphologyEx(thresh, morph, cv::MORPH_OPEN, cv::Mat::ones(7, 7, CV_8UC1));
	//cv::namedWindow("morph", cv::WINDOW_KEEPRATIO);
//std::string Path1 = CStringA("C:\\Temp\\morph.jpg");
//cv::imwrite(Path1, morph);


		// detect the contours on the binary image using cv2.CHAIN_APPROX_NONE
	vector<vector<Point>> contours;
	vector<Vec4i> hierarchy;
	findContours(morph, contours, hierarchy, RETR_TREE, CHAIN_APPROX_NONE);


	// draw contours on the original image
	for (size_t i = 0; i < contours.size(); i++) {
		cv::Moments M = cv::moments(contours[i]);  // Compute moments

		if (M.m00 != 0) {  // Prevent division by zero
			int cx = static_cast<int>(M.m10 / M.m00);
			int cy = static_cast<int>(M.m01 / M.m00);

			// Draw the contour
//				cv::drawContours(image_copy, contours, (int)i, cv::Scalar(0, 255, 0), 2);

				// Draw the center of the circle
//				cv::circle(image_copy, cv::Point(cx, cy), 5, cv::Scalar(0, 0, 255), -1);

				// Store center
			m_BumpLocations.push_back(cv::Point(cx, cy));

		}
	}
}

void CSegmenter::ImageRegistrationBumpLocator()
{
	CIppiImage* pImage1 = m_pInspectionImage;
	CIppiImage tmpBumpTemplate;
	CreateBumpTemplate(tmpBumpTemplate, m_BumpSizeInPixels);


	CIppiImage* pResult = new CIppiImage(pImage1->Width(), pImage1->Height(), 1, pp16s);


	CPoint MaxPos;
	double MaxVal;
	FindTemplate(pImage1, &tmpBumpTemplate, pResult, MaxPos, MaxVal);


	float thresholdk = 0.85f; // acceptance threshold is critical. too low and we will pick up unwanted features,like tsvs; too high and we migh miss some of the bumps (that could be bridged)
	ippiThreshold_LTVal_16u_C1IR((Ipp16u*)pResult->DataPtr(), pResult->Step(), pResult->Size(), (Ipp16u)(thresholdk * MaxVal), 0);
	ippiThreshold_GTVal_16u_C1IR((Ipp16u*)pResult->DataPtr(), pResult->Step(), pResult->Size(), (Ipp16u)(thresholdk * MaxVal - 1), 255);

	CIppiImage Image8u(pImage1->Width(), pImage1->Height(), 1, pp8u);
	ippiConvert_16s8u_C1R((const Ipp16s*)pResult->DataPtr(), pResult->Step(), (Ipp8u*)Image8u.DataPtr(), Image8u.Step(), Image8u.Size());


	// detect blobs using floodfill **************************************
	Ipp8u IDcol = 128;
	IppiPoint seed = { 0, 0 };
	IppiConnectedComp	con;
	Ipp8u* pbuf;
	int bufsize, StepBytes;

	ippiFloodFillGetSize(Image8u.Size(), &bufsize);
	pbuf = ippiMalloc_8u_C1(bufsize, 1, &StepBytes);

	for (int j = 0; j < Image8u.Height(); j += 2)
	{
		for (int i = 0; i < Image8u.Width(); i += 2)
		{
			if (*(Ipp8u*)Image8u.Point(i, j) == 255)
			{
				seed.x = i;
				seed.y = j;
				ippiFloodFill_8Con_8u_C1IR((Ipp8u*)Image8u.DataPtr(), Image8u.Step(), Image8u.Size(), seed, IDcol, &con, pbuf);
				cv::Point pt = cv::Point(con.rect.x + con.rect.width / 2, con.rect.y + con.rect.height / 2);
				m_BumpLocations.push_back(pt);
			}
		}
	}

	ippiFree(pbuf);
	////***********************************************************************
}

void CSegmenter::SimpleBumpCVBumpLocator()
{

	CIppiImage *pImage1 = m_pInspectionImage;

	CIppiImage Image8u(pImage1->Width(), pImage1->Height(), 1, pp8u);
	SmartConversionto8bit(*pImage1, Image8u);
	cv::Mat gray = cv::Mat(Image8u.Height(), Image8u.Width(), CV_8UC1, Image8u.DataPtr(), Image8u.Step());

	SimpleBlobDetector::Params params;
	// Change thresholds
	params.minThreshold = 1;
	params.maxThreshold = 60;

	//	params.minDistBetweenBlobs = BumpSizeInPixels*1.5;

		// Filter by Area.
	params.filterByArea = false;
	params.filterByCircularity = false;
	params.filterByConvexity = false;
	params.filterByInertia = false;
	// Storage for blobs
	vector<KeyPoint> keypoints;
	Ptr<SimpleBlobDetector> detector = SimpleBlobDetector::create(params);
	detector->detect(gray, keypoints);
	for (int i = 0; i < keypoints.size(); i++)
	{
		cv::Point pt = keypoints[i].pt;
		m_BumpLocations.push_back(pt);
	}
	//******************************
}

void CSegmenter::AdaptiveCandidateSelection() {

}

