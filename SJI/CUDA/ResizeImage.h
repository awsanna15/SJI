


#ifndef __RESIZEIMAGE_H_
#define __RESIZEIMAGE_H_


extern "C" void ResizeImage_Dev(unsigned short *dev_result, unsigned short *dev_src, int iwSrc, int ihSrc, int iwDst, int ihDst);
extern "C" void Zoom_Dev(unsigned short *dev_result, unsigned short *dev_src, int iwDst, int ihDst, float fZoom, float fx=0, float fy=0);
extern "C" void Zoom8u_Dev(unsigned char *dst_dev, unsigned char *src_dev, int DstSrc_dev_step, int DstSrc_dev_w, int DstSrc_dev_h, float fZoom, float fx = 0, float fy = 0);
extern "C" void Zoom8uxy_Dev(unsigned char *dev_result, unsigned char *dev_src, int iwDst, int ihDst, float fZoomx, float fZoomy);
extern "C" void SetC_Dev(unsigned short *dev_SrcDest,int wDst, int hDst, unsigned short ValC);

extern "C" void ResizeImage(unsigned short *dst, unsigned short *src, int iwSrc, int ihSrc, int iwDst, int ihDst);
extern "C" void ResizeImage8u(unsigned char *dst, unsigned char *src, int iwSrc, int ihSrc, int iwDst, int ihDst);
extern "C" void Zoom(unsigned short *dst, unsigned short *src, int iwDst, int ihDst, float fZoom, float fx=0, float fy=0);
extern "C" void Zoom8(unsigned char *dst, unsigned char *src, int step, int iwDst, int ihDst, float fZoom);

extern "C" void Rotate_Dev(unsigned short *dev_result, unsigned short *dev_src, int iwDst, int ihDst, float fAngleDeg);
extern "C" void Rotate8u_Dev(unsigned char *dev_result, unsigned char *dev_src, int iwDst, int ihDst, float fAngleRad);
extern "C" void Rotate(unsigned short *SrcDst, int iwDst, int ihDst, float fAngleDeg);


extern "C" void ShiftImage(unsigned short *SrcDst, int iwDst, int ihDst, const float dx, const float dy);
extern "C" void ShiftImage_dev(unsigned short *dev_src, int iwDst, int ihDst, const float dx, const float dy);
extern "C" void ShiftAddImage_dev(float *devDest, int fWidth, int fHeight, unsigned short *dev_src, int iwDst, int ihDst, const float dx, const float dy);
extern "C" void Div32Image_dev(unsigned short *dev_dst, int iwDst, int ihDst, float *dev_src, int fWidth, int fHeight, float DivC);
extern "C" void MirrorHorizontal_dev(unsigned short *srcDev, int iwSrc, int ihSrc, int step);
extern "C" void MirrorHorizontal(unsigned short *src, int iwSrc, int ihSrc, int step);
extern "C" void ShiftXYImage(unsigned short *SrcDst, int iw, int ih, int dx, int dy);
extern "C" void ShiftXYImage_dev(unsigned short *Dst_dev, unsigned short *Src_dev, int iw, int ih, int dx, int dy, int val=-1);
extern "C" void ShiftXYZoomImage(unsigned short *SrcDst, int iw, int ih, int dx, int dy, float fZoom, int val);
extern "C" void LocalContrast(unsigned short *SrcDst, int iw, int ih, float fVal1, float fVal2);
extern "C" void RealLocalContrast_dev(unsigned short *dev_srcDst, int iw, int ih, float fVal1);
extern "C" void RealLocalContrast(unsigned short *SrcDst, int iw, int ih, float fVal1);
extern "C" void LocalContrast_dev(unsigned short *dev_srcDst, int iw, int ih, float fVal1, float fVal2);
extern "C" void CudaMemCopyRect_dev(unsigned short *dev_result, unsigned short * dev_src, int iwSrc, int ihSrc, int iwDst, int ihDst, int sx, int sy);

extern "C" void ShiftXYMapImage_dev(unsigned short *dev_Dst, unsigned short *Image_dev, unsigned short *ResultImage_dev, int iw, int ih, float sinTheta, float cosTheta, int val = -1);
#endif
