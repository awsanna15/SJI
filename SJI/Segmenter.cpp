#include "stdafx.h"
#include "Segmenter.h"
#include "ippiImage.h"
#include "MyUtilities.h"
#include <opencv2/opencv.hpp>
#include "CUDA/ResizeImage.h"

using namespace cv;

#define PI 3.14159265

inline void init_vector_4_connected_pixels(CPoint centre, IppiSize image_size, const std::vector<CPoint> v_4conn_xy_pix_offset, std::vector<CPoint>& v_4conn_search_xy_pix)
{
	v_4conn_search_xy_pix.clear();

	std::vector<CPoint>::const_iterator c_iter_this_point = v_4conn_xy_pix_offset.begin();
	for (; c_iter_this_point != v_4conn_xy_pix_offset.end(); ++c_iter_this_point)
	{
		CPoint neighbor_pix = centre + *c_iter_this_point;
		const BOOL b_is_pix_in_image = (0 <= neighbor_pix.x) && (0 <= neighbor_pix.y) && (image_size.width > neighbor_pix.x) && (image_size.height > neighbor_pix.y);
		if (b_is_pix_in_image)
			v_4conn_search_xy_pix.push_back(neighbor_pix);
	}
}


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

SingleObjectVec CSegmenter::GetBumpLocations()
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
	case BridgeCandidateSelectionMethods::AdaptiveThreshold:
		AdaptiveCandidateSelection();
		break;
	case BridgeCandidateSelectionMethods::LookForNeighbours:
		break;
	case BridgeCandidateSelectionMethods::SuspectsOnly:
		SuspectsOnly();
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
	for (size_t i = 0; i < contours.size(); i++) 
	{
		
		cv::Moments M = cv::moments(contours[i]);  // Compute moments

		if (M.m00 != 0) {  // Prevent division by zero
			int cx = static_cast<int>(M.m10 / M.m00);
			int cy = static_cast<int>(M.m01 / M.m00);

			// Draw the contour
//				cv::drawContours(image_copy, contours, (int)i, cv::Scalar(0, 255, 0), 2);

				// Draw the center of the circle
//				cv::circle(image_copy, cv::Point(cx, cy), 5, cv::Scalar(0, 0, 255), -1);

				// Store center
			ImageSingleObject ob;
			ob.realCentre.x = M.m10 / M.m00;
			ob.realCentre.y = M.m01 / M.m00;
			ob.bridgeID = -1;
			ob.centre.x = cx;
			ob.centre.y = cy;
			ob.floodfillpos = CPoint(cx, cy);
			ob.objID = i;
			Rect bounding_rect= boundingRect(contours[i]);;
	
			ob.height= bounding_rect.width;
			ob.width= bounding_rect.height;
			m_BumpLocations.push_back(ob);


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

				ImageSingleObject ob;
				pair<float,float> cpf=GetRealPosition(CRect(con.rect.x, con.rect.y, con.rect.x+ con.rect.width, con.rect.y + con.rect.height), &Image8u, (int)IDcol);
				ob.realCentre.x = Round(cpf.first);
				ob.realCentre.y = Round(cpf.second);
				ob.bridgeID = -1;
				ob.centre.x = pt.x;
				ob.centre.y = pt.y;
				ob.floodfillpos = CPoint(i, j);
				ob.width = con.rect.width;
				ob.height = con.rect.height;
				ob.objID = m_BumpLocations.size();
				m_BumpLocations.push_back(ob);

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
		ImageSingleObject ob;
		ob.realCentre.x = pt.x;
		ob.realCentre.y = pt.y;
		ob.bridgeID = -1;
		ob.centre.x = pt.x;
		ob.centre.y = pt.y;
		ob.floodfillpos = CPoint(pt.x, pt.y);
		ob.width = keypoints[i].size;
		ob.height = keypoints[i].size;
		ob.objID = m_BumpLocations.size();
		m_BumpLocations.push_back(ob);
	}
	//******************************
}

void CSegmenter::AdaptiveCandidateSelection() 
{

}

void CSegmenter::SuspectsOnly()
{
	CIppiImage* pImage1 = m_pInspectionImage;

	CIppiImage Image8u(pImage1->Width(), pImage1->Height(), 1, pp8u);
	SmartConversionto8bit(*pImage1, Image8u);

	CIppiImage Image8ucopy(Image8u);

	CreateImageMask(Image8u, m_BumpSizeInPixels);

	CIppiImage* pFeatureImage = &Image8u;
	CIppiImage FeatureImage32f(pFeatureImage->Width(), pFeatureImage->Height(), 1, pp32f);
	FeatureImage32f.Set(-1);

	int ID = 0;
	Ipp8u IDcol = 0;
	IppiPoint seed = { 0, 0 };
	Ipp8u* pbuf;
	int bufsize, StepBytes;

	ippiFloodFillGetSize(Image8u.Size(), &bufsize);
	pbuf = ippiMalloc_8u_C1(bufsize, 1, &StepBytes);

	IppiConnectedComp	con;

	int x, y;
	int objID;

	int lpi;
	CPoint cpWBA = CPoint(Image8u.Width() / 2, Image8u.Height() / 2);


	for (int pi = 0; pi < m_BumpLocations.size(); pi++)
	{
		CPoint pt = m_BumpLocations[pi].floodfillpos;
		x = pt.x;
		y = pt.y;
		objID = pi;
		if (x >= 0 && x < pFeatureImage->Width() && y >= 0 && y < pFeatureImage->Height() && *(Ipp8u*)pFeatureImage->Point(x, y) > 0)
		{
			lpi = Round(*(Ipp32f*)FeatureImage32f.Point(x, y));
			if (lpi == -1)
			{
				if (*(Ipp8u*)pFeatureImage->Point(x, y) == 255)
				{
					seed.x = x;
					seed.y = y;

					IDcol = 1 + pi % 250;

					ippiFloodFill_8Con_8u_C1IR((Ipp8u*)pFeatureImage->DataPtr(), pFeatureImage->Step(), pFeatureImage->Size(), seed, IDcol, &con, pbuf);

					m_BumpLocations[pi].width = con.rect.width;
					m_BumpLocations[pi].height = con.rect.height;
					m_BumpLocations[pi].bridgeID = -1;
					for (int j = con.rect.y; j < con.rect.y + con.rect.height; j += 1)
					{
						for (int i = con.rect.x; i < con.rect.x + con.rect.width; i += 1)
						{
							if (*(Ipp8u*)pFeatureImage->Point(i, j) == IDcol)
							{
								*(Ipp32f*)FeatureImage32f.Point(i, j) = float(pi);
							}
						}
					}
				}
			}
			else
			{
				ImageSingleObject& ot_first = m_BumpLocations[lpi];
				ImageSingleObject& ot_now = m_BumpLocations[pi];
				if (ot_first.bridgeID >= 0)
				{

					m_BridgeCandidates[ot_first.bridgeID].listBumpIDs.push_back(objID);
					m_BridgeCandidates[ot_first.bridgeID].listBumpPixPos.push_back(ot_first.centre);

				}
				else
				{
					CCandidateBridge cb;
					int objID1 = static_cast<int>(ot_first.objID);
					cb.listBumpIDs.push_back(objID1);
					cb.listBumpPixPos.push_back(ot_first.centre);
					cb.listBumpIDs.push_back(objID);
					cb.listBumpPixPos.push_back(ot_now.centre);

					seed.x = x;
					seed.y = y;
					ippiFloodFill_8Con_8u_C1IR((Ipp8u*)pFeatureImage->DataPtr(), pFeatureImage->Step(), pFeatureImage->Size(), seed, lpi, &con, pbuf);


					cb.boundingRect = CRect(con.rect.x, con.rect.y, con.rect.x+ con.rect.width, con.rect.y+ con.rect.height);
					cb.sizex = cb.boundingRect.Width();
					cb.sizey = cb.boundingRect.Height();
					cb.xreal=
					cb.lp1 = lpi;
					cb.xpix = cb.boundingRect.CenterPoint().x;
					cb.ypix = cb.boundingRect.CenterPoint().y;
					pair<float, float> b = GetRealPosition(cb.boundingRect, pFeatureImage, lpi);
					cb.xreal = b.first;
					cb.yreal = b.second;
					

					//check if has already been included ***************
					BOOL bFound = FALSE;
					for (int i = 0; i < m_BridgeCandidates.size() && !bFound; i++)
					{
						CCandidateBridge candidate = m_BridgeCandidates[i];
						
						if (candidate.lp1 == lpi)
						{
							bFound = TRUE;
							m_BridgeCandidates[i].listBumpIDs.push_back(objID);
							m_BridgeCandidates[i].listBumpPixPos.push_back(ot_now.centre);
						}
					}
					if (!bFound)
					{

						float dx = static_cast<float>(ot_now.floodfillpos.x - ot_first.floodfillpos.x);
						float dy = static_cast<float>(ot_now.floodfillpos.y - ot_first.floodfillpos.y);
						if (_hypot(dx, dy) > m_BumpSizeInPixels)
						{
							cb.BridgeID = m_BridgeCandidates.size() + 1;
							m_BridgeCandidates.push_back(cb);
						}
					}
					//**************************************************
					//******************************************
					
				}
			}
		}
		

	}


	ippiFree(pbuf);

	//********* for all candidates, prepare imagesegment for nn inference *************************
	for (int i = 0; i < m_BridgeCandidates.size(); i++)
	{
		PrepareSegmentImage(m_BridgeCandidates[i], FeatureImage32f, &Image8ucopy, m_BumpSizeInPixels);
	}
	
	//********************************************************

}

pair<float,float> CSegmenter::GetRealPosition(CRect con, CIppiImage* pFeatureImage, int IDcol)
{
	pair<float, float> xy;
	xy.first = 0;
	xy.second = 0;

	int cntxy = 0;

	for (int j = con.top; j < con.bottom; j++)
	{
		for (int i = con.left; i < con.right; i++)
		{
			if (*(Ipp8u*)pFeatureImage->Point(i, j) == IDcol)
			{
				xy.first += i;
				xy.second += j;
				cntxy++;
			}
		}
	}
	xy.first /= cntxy;
	xy.second /= cntxy;

	return xy;
}


void CSegmenter::PrepareSegmentImage(CCandidateBridge &Segment, CIppiImage& FeatureImage32f, CIppiImage* pImage8FOV, int BumpSizePix)
{
	CIppiImage m_tempImage8u(*pImage8FOV);
pImage8FOV->SaveImage(_T("C:\\Temp\\pImage8FOV.bmp"));
	int iconsize = 56;

	if (Segment.pBridgeImg != NULL)
	{
		delete Segment.pBridgeImg;
	}

	Segment.pBridgeImg = new CIppiImage(iconsize, iconsize, 1, pp8u);
	CIppiImage *pImage = Segment.pBridgeImg;
	pImage->Set(255);


	Ipp32f lp1_f = static_cast<Ipp32f>(Segment.lp1);
	float maxRad = 0;
	float maxr = 0;
	float tmpr;
	std::unique_ptr<float[]> maxrarray(new float[72]);
	memset(maxrarray.get(), 0, sizeof(float) * 72);

	float tmpangl;
	int inx;

	std::vector<CPoint> v_4conn_xy_pix_offset;
	//v_4conn_xy_pix_offset.push_back(CPoint(0, 0)); //centre
	v_4conn_xy_pix_offset.push_back(CPoint(1, 0)); //right
	v_4conn_xy_pix_offset.push_back(CPoint(-1, 1)); //left
	v_4conn_xy_pix_offset.push_back(CPoint(0, 1)); //top
	v_4conn_xy_pix_offset.push_back(CPoint(0, -1)); //bottom

	std::vector<CPoint> v_4conn_search_xy_pix;

	CPoint cp = CPoint(Segment.xpix, Segment.ypix);// pBridgeResult->boundingRect.CenterPoint();
	for (int y = Segment.boundingRect.top; y < Segment.boundingRect.bottom; y++)
	{
		for (int x = Segment.boundingRect.left; x < Segment.boundingRect.right; x++)
		{
			CPoint centre_pt = CPoint(x, y );
			BOOL is_centre_pt_valid = FeatureImage32f.IsValid() && FeatureImage32f.PointInImage(centre_pt) && (lp1_f == *(static_cast<Ipp32f*>(FeatureImage32f.Point(centre_pt))));
			BOOL is_centre_pt_good_candidate = FALSE;

			if (is_centre_pt_valid)
			{
				init_vector_4_connected_pixels(centre_pt, FeatureImage32f.Size(), v_4conn_xy_pix_offset, v_4conn_search_xy_pix);

				std::vector<CPoint>::iterator iter_this_point = v_4conn_search_xy_pix.begin();

				for (; iter_this_point != v_4conn_search_xy_pix.end(); ++iter_this_point)
				{
					Ipp32f pixel_value = *(static_cast<Ipp32f*>(FeatureImage32f.Point(*iter_this_point)));
					is_centre_pt_good_candidate = 0.0f > pixel_value;
					if (is_centre_pt_good_candidate)
						break;
				}

				if (is_centre_pt_good_candidate)
				{
					tmpr = static_cast<float>(_hypot(x - cp.x, y - cp.y));
					tmpangl = static_cast<float>(atan2(y - cp.y, x - cp.x) * 180 / PI);
					while (tmpangl < 0) tmpangl += 360;
					while (tmpangl >= 360) tmpangl -= 360;
					inx = Round(tmpangl);
					inx += 2;
					inx %= 360;
					inx = int(inx / 5);
					maxrarray[inx] = max(maxrarray[inx], tmpr);
					if (tmpr > maxr)
					{
						maxr = tmpr;
						maxRad = static_cast<float>(atan2(y - cp.y, x - cp.x));
					}
				}
			}
		}
	}

	for (int p = 0; p < 72; p++)
	{
		TRACE(_T("\t%f\n"), maxrarray[p]);
	}
	int p = Round(maxRad * 180 / PI);
	while (p < 0) p += 360;
	while (p >= 360) p -= 360;
	p += 2;
	p %= 360;
	p = int(p / 5);

	float sumtoatal = 0;
	float suminx = 0;
	int s1, s2;

	for (int q = p - (45 / 5); q < p + (45 / 5); q++)
	{
		s1 = q * 5;
		while (s1 < 0) s1 += 360;
		while (s1 >= 360) s1 -= 360;
		s1 += 2;
		s1 %= 360;
		s1 = int(s1 / 5);
		s2 = q * 5 + 180;
		while (s2 < 0) s2 += 360;
		while (s2 >= 360) s2 -= 360;
		s2 += 2;
		s2 %= 360;
		s2 = int(s2 / 5);

		if (maxrarray[s1] > 0 && maxrarray[s2] > 0 && maxrarray[s1] + maxrarray[s2] > maxr)
		{
			sumtoatal += (maxrarray[s1] + maxrarray[s2]) * q;
			suminx += (maxrarray[s1] + maxrarray[s2]);
		}
	}
	if (suminx == 0)
	{
		inx = p;
	}
	else
	{
		inx = Round(sumtoatal / suminx);
	}

	maxRad = static_cast<float>((inx * 5) * PI / 180);
	if (Segment.listBumpPixPos.size() > 1)
	{
		ImageSingleObject& ot1 = m_BumpLocations[Segment.listBumpIDs[0]];
		ImageSingleObject& ot2 = m_BumpLocations[Segment.listBumpIDs[1]];
		float dx = static_cast<float>(ot2.floodfillpos.x - ot1.floodfillpos.x) / (abs(static_cast<float>(ot2.floodfillpos.x - ot1.floodfillpos.x)) + abs(static_cast<float>(ot2.floodfillpos.y - ot1.floodfillpos.y)));
		float dy = static_cast<float>(ot2.floodfillpos.y - ot1.floodfillpos.y) / (abs(static_cast<float>(ot2.floodfillpos.x - ot1.floodfillpos.x)) + abs(static_cast<float>(ot2.floodfillpos.y - ot1.floodfillpos.y)));
		maxRad = atan2(dy, dx);
		maxr = static_cast<float>(BumpSizePix * 0.75 + _hypot(ot2.floodfillpos.x - ot1.floodfillpos.x, ot2.floodfillpos.y - ot1.floodfillpos.y) / 2);
	}
	//************************************************************************************************

	float m = maxr / static_cast<float>(iconsize / 2);
	float tmprad;

	int xi, yi;
	float xf, yf;
	int x0, y0;
	float kx, ky;
	double sumall = 0;
	double weight_00 = 0.0;
	double weight_10 = 0.0;
	double weight_01 = 0.0;
	double weight_11 = 0.0;
	const int icon_img_half_wh = iconsize >> 1;

	const BOOL b_are_src_imgs_valid = pImage->IsValid() && m_tempImage8u.IsValid();
	IppiSize tmp_img8u_size = { 0,0 };

	if (b_are_src_imgs_valid)
	{
		tmp_img8u_size = m_tempImage8u.Size();
		for (int y = -icon_img_half_wh; y < icon_img_half_wh; ++y)
		{
			for (int x = -icon_img_half_wh; x < icon_img_half_wh; ++x)
			{
				tmpr = static_cast<float>(_hypot(x, y) * m);
				tmprad = static_cast<float>(atan2(y, x));
				tmprad += maxRad;
				xf = tmpr * cos(tmprad);
				yf = tmpr * sin(tmprad);
				x0 = int(xf);
				y0 = int(yf);
				kx = 1.0f - (xf - static_cast<float>(x0));
				ky = 1.0f - (yf - static_cast<float>(y0));

				const auto this_icon_img_pixel = CPoint(x + icon_img_half_wh, y + icon_img_half_wh);
				BOOL b_is_this_icon_img_pixel_valid = pImage->IsValid() && pImage->PointInImage(this_icon_img_pixel);

				xi = Round(xf);
				yi = Round(yf);
				const auto this_feature32_img_pixel = CPoint(xi + cp.x, yi + cp.y);
				BOOL b_is_this_xy_loc_valid = FeatureImage32f.PointInImage(this_feature32_img_pixel) && (lp1_f == *(static_cast<Ipp32f*>(FeatureImage32f.Point(this_feature32_img_pixel))));

				if (b_is_this_icon_img_pixel_valid && b_is_this_xy_loc_valid)
				{
					weight_00 = kx * ky;
					weight_10 = (1.0 - kx) * ky;
					weight_01 = kx * (1.0 - ky);
					weight_11 = (1.0 - kx) * (1.0 - ky);
					sumall = 0.0;
					CPoint p_00 = CPoint((x0 + cp.x), (y0 + cp.y));
					CPoint p_10 = CPoint((x0 + 1 + cp.x), (y0 + cp.y));
					CPoint p_01 = CPoint((x0 + cp.x), (y0 + 1 + cp.y));
					CPoint p_11 = CPoint((x0 + 1 + cp.x), (y0 + 1 + cp.y));
					Ipp8u pixel_value = 0;
					BOOL weighted_sum_valid = FALSE;
					if ((p_00.x >= 0) && (p_00.y >= 0) && (p_00.x < tmp_img8u_size.width) && (p_00.y < tmp_img8u_size.height))
					{
						pixel_value = *(static_cast<Ipp8u*>(m_tempImage8u.Point(p_00)));
						sumall += weight_00 * pixel_value;
						weighted_sum_valid = TRUE;
					}
					if ((p_10.x >= 0) && (p_10.y >= 0) && (p_10.x < tmp_img8u_size.width) && (p_10.y < tmp_img8u_size.height))
					{
						pixel_value = *(static_cast<Ipp8u*>(m_tempImage8u.Point(p_10)));
						sumall += weight_10 * pixel_value;
						weighted_sum_valid = TRUE;
					}
					if ((p_01.x >= 0) && (p_01.y >= 0) && (p_01.x < tmp_img8u_size.width) && (p_01.y < tmp_img8u_size.height))
					{
						pixel_value = *(static_cast<Ipp8u*>(m_tempImage8u.Point(p_01)));
						sumall += weight_01 * pixel_value;
						weighted_sum_valid = TRUE;
					}
					if ((p_11.x >= 0) && (p_11.y >= 0) && (p_11.x < tmp_img8u_size.width) && (p_11.y < tmp_img8u_size.height))
					{
						pixel_value = *(static_cast<Ipp8u*>(m_tempImage8u.Point(p_11)));
						sumall += weight_11 * pixel_value;
						weighted_sum_valid = TRUE;
					}

					if (TRUE == weighted_sum_valid)
					{
						*(static_cast<Ipp8u*>(pImage->Point(this_icon_img_pixel))) = max(0, Round(sumall));
					}
				}
			}
		}
	}

	int w20 = min(max(1, static_cast<int>(0.2 * iconsize)), (iconsize - 2));
	s1 = 0;
	int e1 = iconsize;
	for (int j = 0; j < iconsize && s1 == 0; ++j)
	{
		if (*(Ipp8u*)pImage->Point(w20, j) < 250 && *(Ipp8u*)pImage->Point(w20 + 1, j) < 250 && *(Ipp8u*)pImage->Point(w20 - 1, j) < 250)
		{
			s1 = j;
		}
	}
	for (int j = iconsize - 1; j >= 0 && e1 == iconsize; --j)
	{
		if (*(Ipp8u*)pImage->Point(w20, j) < 250 && *(Ipp8u*)pImage->Point(w20 + 1, j) < 250 && *(Ipp8u*)pImage->Point(w20 - 1, j) < 250)
		{
			e1 = j;
		}
	}
	int w80 = max(1, min(iconsize - 2, iconsize - w20));
	s2 = 0;
	int e2 = iconsize;
	for (int j = 0; j < iconsize && s2 == 0; ++j)
	{
		if (*(Ipp8u*)pImage->Point(w80, j) < 250 && *(Ipp8u*)pImage->Point(w80 - 1, j) < 250 && *(Ipp8u*)pImage->Point(w80 + 1, j) < 250)
		{
			s2 = j;
		}
	}
	for (int j = iconsize - 1; j >= 0 && e2 == iconsize; --j)
	{
		if (*(Ipp8u*)pImage->Point(w80, j) < 250 && *(Ipp8u*)pImage->Point(w80 - 1, j) < 250 && *(Ipp8u*)pImage->Point(w80 + 1, j) < 250)
		{
			e2 = j;
		}
	}
//	int shifty = (s1 + s2 + e1 + e2) / 4 - iconsize / 2;
////	ShiftXYImage8u((unsigned char*)pImage->DataPtr(), pImage->Step(), pImage->Height(), 0, shifty);
//	float dy = static_cast<float>(s1 + e1) / 2 - static_cast<float>(s2 + e2) / 2;
//	float dx = static_cast<float>(0.8 * iconsize - 2);
//	float adeg = atan2(dy, dx) * 180.0f / static_cast<float>(PI);
//	Rotate8u((unsigned char*)pImage->DataPtr(), pImage->Step(), pImage->Height(), -adeg);

	ippiThreshold_LTValGTVal_8u_C1IR((Ipp8u*)pImage->DataPtr(), pImage->Step(), pImage->Size(), 0, 0, 255, 255);


pImage->SaveImage(_T("C:\\Temp\\pImage.bmp"));
}