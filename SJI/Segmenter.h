#pragma once
#include <vector>
#include <opencv2/opencv.hpp>
#include "BridgeResult.h"

class CIppiImage;

using namespace std;


enum FindBlobMethods

{
	EdgeBased,
	ImageRegistration,
	SimpleBumpCV
};

enum BridgeCandidateSelectionMethods
{
	AdaptiveThreshold,
	LookForNeighbours
};





class CSegmenter
{
public:
	CSegmenter(CIppiImage* pInspectionImage, FindBlobMethods BlobMethod= SimpleBumpCV, BridgeCandidateSelectionMethods CandidateSelectMethod = AdaptiveThreshold, int BumpSizeInPixels=8);
	~CSegmenter(void);


private:
	int m_BumpSizeInPixels;
	CIppiImage* m_pInspectionImage;

	FindBlobMethods m_BlobMethod;
	BridgeCandidateSelectionMethods m_CandidateSelectMethod;

private:
	std::vector<cv::Point> m_BumpLocations;
	std::vector<CCandidateBridge> m_BridgeCandidates;


private:
	void EdgeBasedBumpLocator();
	void ImageRegistrationBumpLocator();
	void SimpleBumpCVBumpLocator();
	void AdaptiveCandidateSelection();
public:

	std::vector<cv::Point> GetBumpLocations();
	std::vector<CCandidateBridge> GetBridgeCandidates();
	void ClearAll();
};

