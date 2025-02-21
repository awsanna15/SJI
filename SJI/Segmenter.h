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
	LookForNeighbours,
	SuspectsOnly,
};


struct ImageSingleObject
{

	CPoint	centre;
	int		width;
	int		height;
	CPoint	realCentre;


	CPoint floodfillpos;

	unsigned int objID;
	int bridgeID;



};

typedef std::vector<ImageSingleObject>  SingleObjectVec;



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
	SingleObjectVec m_BumpLocations;
	std::vector<CCandidateBridge> m_BridgeCandidates;


private:
	void EdgeBasedBumpLocator();
	void ImageRegistrationBumpLocator();
	void SimpleBumpCVBumpLocator();
	void AdaptiveCandidateSelection();
	void SuspectsOnly();
	pair<float, float> GetRealPosition(CRect con, CIppiImage* pFeatureImage, int IDcol);
	void PrepareSegmentImage(CCandidateBridge& Segment, CIppiImage& FeatureImage32f, CIppiImage* pImage, int BumpSizePix);
public:

	SingleObjectVec GetBumpLocations();
	std::vector<CCandidateBridge> GetBridgeCandidates();
	CIppiImage* GetInspectionImage() 
	{ 
		return m_pInspectionImage; 
	}

	void ClearAll();
};

