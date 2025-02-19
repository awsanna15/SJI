
#include "ResizeImage.h"



#include <cuda_runtime.h>

#define PI 3.14159265359




// This will output the proper CUDA error strings in the event that a CUDA host call returns an error
#define checkCudaErrors(err)           __checkCudaErrors (err, __FILE__, __LINE__)

inline void __checkCudaErrors(cudaError err, const char *file, const int line)
{
    if (cudaSuccess != err)
    {
 //       TRACE("line(%i) : CUDA Runtime API error %d: %s.\n", line, (int)err, cudaGetErrorString(err));
		 cudaDeviceReset();
        exit(EXIT_FAILURE);
    }
}


__global__ void
MirrorHorizontal_kernel( unsigned short *SrcDest, int wSrcDst, int hSrcDst, int step)
{
	int x= threadIdx.x + blockIdx.x*blockDim.x;
	int y= threadIdx.y + blockIdx.y*blockDim.y;

	if(x<=int((wSrcDst+1)/2) && y<hSrcDst)
	{
		int offset=x+y*step;
		int offset2=(wSrcDst-1-x)+y*step;
		unsigned short tmp=SrcDest[offset];
		SrcDest[offset]=SrcDest[offset2];
		SrcDest[offset2]=tmp;

	}
}


__global__ void
Rotate8u_kernel(unsigned char *dest, unsigned char *src, int wDst, int hDst, float cos_deg, float sin_deg)
{

	int ix = threadIdx.x + blockIdx.x*blockDim.x;
	int iy = threadIdx.y + blockIdx.y*blockDim.y;

	int resultPix = 0;
	int offset = ix + iy*wDst;

	bool bAvailable = false;

	if (ix<wDst && iy<hDst)
	{
		float xc = (float)wDst / 2;
		float yc = (float)hDst / 2;

		float x = ((float)ix - xc)*cos_deg - ((float)iy - yc)*sin_deg + xc;
		float y = ((float)ix - xc)*sin_deg + ((float)iy - yc)*cos_deg + yc;

		bAvailable = true;

		if (x >= 0.0f && x <= wDst - 1 && y >= 0 && y <= hDst - 1)
		{
			int i = int(x);
			int j = int(y);

			float ki1 = (x - (float)i);
			float ki0 = 1.0f - ki1;

			float kj1 = (y - (float)j);
			float kj0 = 1.0f - kj1;

			float SumPix = 0.0f;
			float SumK = 0.0f;


			SumPix += ki0*kj0*src[i + j*wDst];
			SumK += ki0*kj0;

			if (i + 1 <= wDst - 1)
			{
				SumPix += ki1*kj0*src[(i + 1) + j*wDst];
				SumK += ki1*kj0;
			}
			if (j + 1 <= hDst - 1)
			{
				SumPix += ki0*kj1*src[i + (j + 1)*wDst];
				SumK += ki0*kj1;
			}
			if (i + 1 <= wDst - 1 && j + 1 <= hDst - 1)
			{
				SumPix += ki1*kj1*src[(i + 1) + (j + 1)*wDst];
				SumK += ki1*kj1;
			}
			resultPix = max(0, min(32767, int(SumPix / SumK + 0.5)));

		}
		else
		{
			resultPix = 0;
		}

	}

	__syncthreads();

	if (bAvailable)
	{
		dest[offset] = resultPix;
	}
}

__global__ void
Threshold_kernel(unsigned short *dest, unsigned short *src, int wSrcDst, int hSrcDst, unsigned short threshold)
{
	int x= threadIdx.x + blockIdx.x*blockDim.x;
	int y= threadIdx.y + blockIdx.y*blockDim.y;

	if(x<wSrcDst && y<hSrcDst)
	{
		int offset=x+y*wSrcDst;
		unsigned short tmp=src[offset];
		if(tmp>0)
		{
			dest[offset]=tmp;
		}
		else
		{
			dest[offset]=0;
		}
	}
}

//********************************************************************************
__global__ void
LocalContrast_kernel(unsigned short *dest, unsigned short *src, int w, int h, float fVal1, float fVal2)
{
	int x = threadIdx.x + blockIdx.x*blockDim.x;
	int y = threadIdx.y + blockIdx.y*blockDim.y;

	if (x < w && y < h)
	{
		float sumAverage = (float)0.0f;
		unsigned short currentFrameSample1 = src[x + y*w];
		int cnt = 0;
		for (int i = -3; i <= 3; i++)
		{
			for (int j = -3; j <= 3; j++)
			{
				if ((x + i) >= 0 && (x + i) < w && (y + j) > 0 && (y + j) < h)
				{
					sumAverage += src[(x + i) + (y + j)*w];
					cnt++;
				}
				
			}
		}
		unsigned short currentFrameSample2 = sumAverage / cnt;

		if (currentFrameSample1<currentFrameSample2)
		{
			dest[x + y*w] = currentFrameSample2 + (currentFrameSample1 - currentFrameSample2)*fVal1;
		}
		else
		{
			dest[x + y*w] = currentFrameSample2 + (currentFrameSample1 - currentFrameSample2) / fVal2;
		}

	}
}
//********************************************************************************
__global__ void
RealLocalContrast_kernel(unsigned short *dest, unsigned short *src, int w, int h, float fVal1)
{
	int x = threadIdx.x + blockIdx.x*blockDim.x;
	int y = threadIdx.y + blockIdx.y*blockDim.y;

	if (x < w && y < h)
	{
		float sumAverage = (float)0.0f;
		unsigned short currentFrameSample1 = src[x + y*w];
		int cnt = 0;
		for (int i = -7; i <= 7; i++)
		{
			for (int j = -7; j <= 7; j++)
			{
				if ((x + i) >= 0 && (x + i) < w && (y + j) > 0 && (y + j) < h)
				{
					sumAverage += src[(x + i) + (y + j)*w];
					cnt++;
				}
				
			}
		}
		unsigned short currentFrameSample2 = sumAverage / cnt;
		float tmp=(float)currentFrameSample2 + float(currentFrameSample1 - currentFrameSample2)*fVal1;
		if(tmp<0)
		{
			tmp=0;
		}
		else if(tmp>32767)
		{
			tmp=32767;
		}
		dest[x + y*w] =  unsigned short(tmp);

	}
}

__global__ void
ShiftXYMapImage_kernel(unsigned short *dev_Dst, unsigned short *Image_dev, unsigned short *ResultImage_dev, int w, int h, float sinTheta, float cosTheta, int val)
{
	int x = threadIdx.x + blockIdx.x*blockDim.x;
	int y = threadIdx.y + blockIdx.y*blockDim.y;

	if (x < w && y < h && x >= 0 && y >= 0)
	{
		float dx = (float(ResultImage_dev[x + y*w])-128.0f)*cosTheta;
		float dy = (float(ResultImage_dev[x + y*w])-128.0f)*sinTheta;
		int xsrc = x + dx;
		int ysrc = y + dy;

		if (xsrc >= 0 && xsrc < w && ysrc >= 0 && ysrc < h)
		{
			dev_Dst[x + y*w] = Image_dev[xsrc + ysrc*w];
		}
		else
		{
			if (val >= 0)
			{
				dev_Dst[x + y*w] = val;
			}
			else
			{
				dev_Dst[x + y*w] = Image_dev[x + y*w];
			}

		}
	}
	else
	{
		if (val >= 0)
		{
			dev_Dst[x + y*w] = val;
		}
		else
		{
			dev_Dst[x + y*w] = Image_dev[x + y*w];
		}

	}
}

__global__ void
ShiftXYImage_kernel(unsigned short *dest, unsigned short *src, int w, int h, int dx, int dy, int val)
{
	int x = threadIdx.x + blockIdx.x*blockDim.x;
	int y = threadIdx.y + blockIdx.y*blockDim.y;

	if (x < w && y < h && x>=0 && y>=0)
	{
		int xsrc= x + dx;
		int ysrc= y + dy;
		
		if (xsrc >= 0 && xsrc < w && ysrc >= 0 && ysrc < h)
		{
			dest[x + y*w] = src[xsrc + ysrc*w];
		}
		else
		{
			if (val >= 0)
			{
				dest[x + y*w] = val;
			}
			else
			{
				dest[x + y*w] = src[x + y*w];
			}
			
		}
	}
	
}

//************************** ResizeDown *******************************************
__global__ void
ResizeDown_kernel(unsigned short *dest, unsigned short *src, int wSrc, int hSrc, int wDst, int hDst)
{
	int x= threadIdx.x + blockIdx.x*blockDim.x;
	int y= threadIdx.y + blockIdx.y*blockDim.y;

	if(x<wDst && y<hDst)
	{
		int offset=x+y*wDst;

		float aMin=(float)x/(float)wDst;
		float bMin=(float)y/(float)hDst;

		float aMax=(float)(x+1)/(float)wDst;
		float bMax=(float)(y+1)/(float)hDst;

		int a0=int(x*(float)wSrc/(float)wDst);
		int b0=int(y*(float)hSrc/(float)hDst);

		int xtmp=a0;
		int ytmp=b0;

		float SumPix=0.0f;
		float SumK=0.0f;
		float PixP=1.0f/(wSrc*hSrc);

		while((float)xtmp/wSrc < aMax)
		{
			ytmp=b0;
			while((float)ytmp/hSrc < bMax)
			{
				float fx0=max((float)xtmp/wSrc,aMin);
				float fy0=max((float)ytmp/hSrc,bMin);
				float fx1=min((float)(xtmp+1)/wSrc,aMax);
				float fy1=min((float)(ytmp+1)/hSrc,bMax);

				float k=(fx1-fx0)*(fy1-fy0)/PixP;

				if(xtmp<wSrc && ytmp<hSrc)
				{
					SumK+=k;
					SumPix+=k*src[xtmp+ytmp*wSrc]; 
				}
				ytmp++;
			}
			xtmp++;
		}
		if(SumPix>0)
		{
			dest[offset] = int(SumPix/SumK+0.5);
		}
		else
		{
			dest[offset] = 0;
		}

	
	}
   
}
//************************************************************************

//************************* ResizeUp **************************************
__global__ void
ResizeUp_kernel(unsigned short *dest, unsigned short *src, int wSrc, int hSrc, int wDst, int hDst)
{
	int x= threadIdx.x + blockIdx.x*blockDim.x;
	int y= threadIdx.y + blockIdx.y*blockDim.y;

	if(x<wDst && y<hDst)
	{
		int offset=x+y*wDst;

		float a0f=x*(float)wSrc/(float)wDst;
		float b0f=y*(float)hSrc/(float)hDst;
		int a0=int(a0f);
		int b0=int(b0f);

		float ki1=a0f-(float)a0;
		float ki0=1.0f-ki1;

		float kj1=b0f-(float)b0;
		float kj0=1.0f-kj1;

		float SumPix=0.0f;
		float SumK=0.0f;

	
		SumPix+=ki0*kj0*src[a0+b0*wSrc];
		SumK+=ki0*kj0;

		if(a0+1<wSrc)
		{
			SumPix+=ki1*kj0*src[(a0+1)+b0*wSrc];
			SumK+=ki1*kj0;
		}
		if(b0+1<hSrc)
		{
			SumPix+=ki0*kj1*src[a0+(b0+1)*wSrc];
			SumK+=ki0*kj1;
		}
		if(a0+1<wSrc && b0+1<hSrc)
		{
			SumPix+=ki1*kj1*src[(a0+1)+(b0+1)*wSrc];
			SumK+=ki1*kj1;
		}
		dest[offset] = int(SumPix/SumK+0.5);

	}

}



//************************** ResizeDown8u *******************************************
__global__ void
ResizeDown8u_kernel(unsigned char *dest, unsigned char *src, int wSrc, int hSrc, int wDst, int hDst)
{
	int x= threadIdx.x + blockIdx.x*blockDim.x;
	int y= threadIdx.y + blockIdx.y*blockDim.y;

	if(x<wDst && y<hDst)
	{
		int offset=x+y*wDst;

		float aMin=(float)x/(float)wDst;
		float bMin=(float)y/(float)hDst;

		float aMax=(float)(x+1)/(float)wDst;
		float bMax=(float)(y+1)/(float)hDst;

		int a0=int(x*(float)wSrc/(float)wDst);
		int b0=int(y*(float)hSrc/(float)hDst);

		int xtmp=a0;
		int ytmp=b0;

		float SumPix=0.0f;
		float SumK=0.0f;
		float PixP=1.0f/(wSrc*hSrc);

		while((float)xtmp/wSrc < aMax)
		{
			ytmp=b0;
			while((float)ytmp/hSrc < bMax)
			{
				float fx0=max((float)xtmp/wSrc,aMin);
				float fy0=max((float)ytmp/hSrc,bMin);
				float fx1=min((float)(xtmp+1)/wSrc,aMax);
				float fy1=min((float)(ytmp+1)/hSrc,bMax);

				float k=(fx1-fx0)*(fy1-fy0)/PixP;

				if(xtmp<wSrc && ytmp<hSrc)
				{
					SumK+=k;
					SumPix+=k*src[xtmp+ytmp*wSrc]; 
				}
				ytmp++;
			}
			xtmp++;
		}
		if(SumPix>0)
		{
			dest[offset] = char(SumPix/SumK+0.5);
		}
		else
		{
			dest[offset] = 0;
		}

	
	}
   
}
//************************************************************************

//************************* ResizeUp **************************************
__global__ void
ResizeUp8u_kernel(unsigned char *dest, unsigned char *src, int wSrc, int hSrc, int wDst, int hDst)
{
	int x= threadIdx.x + blockIdx.x*blockDim.x;
	int y= threadIdx.y + blockIdx.y*blockDim.y;

	if(x<wDst && y<hDst)
	{
		int offset=x+y*wDst;

		float a0f=x*(float)wSrc/(float)wDst;
		float b0f=y*(float)hSrc/(float)hDst;
		int a0=int(a0f);
		int b0=int(b0f);

		float ki1=a0f-(float)a0;
		float ki0=1.0f-ki1;

		float kj1=b0f-(float)b0;
		float kj0=1.0f-kj1;

		float SumPix=0.0f;
		float SumK=0.0f;

	
		SumPix+=ki0*kj0*src[a0+b0*wSrc];
		SumK+=ki0*kj0;

		if(a0+1<wSrc)
		{
			SumPix+=ki1*kj0*src[(a0+1)+b0*wSrc];
			SumK+=ki1*kj0;
		}
		if(b0+1<hSrc)
		{
			SumPix+=ki0*kj1*src[a0+(b0+1)*wSrc];
			SumK+=ki0*kj1;
		}
		if(a0+1<wSrc && b0+1<hSrc)
		{
			SumPix+=ki1*kj1*src[(a0+1)+(b0+1)*wSrc];
			SumK+=ki1*kj1;
		}
		dest[offset] = char(SumPix/SumK+0.5);

	}

}
__global__ void AddImages_kernel(unsigned short *Dest, unsigned short *SrcImage1, unsigned short *SrcImage2, int wDst, int hDst)
{
	int ix= threadIdx.x + blockIdx.x*blockDim.x;
	int iy= threadIdx.y + blockIdx.y*blockDim.y;
	int offset=ix+iy*wDst;

	if(ix<wDst && iy<hDst)
	{
		Dest[offset]=(SrcImage1[offset]+SrcImage2[offset])/2;
	}

}


__global__ void Set_kernel(unsigned short *SrcDest, int wDst, int hDst, unsigned short ValC)
{
	int x= threadIdx.x + blockIdx.x*blockDim.x;
	int y= threadIdx.y + blockIdx.y*blockDim.y;
	
	int offset=x+y*wDst;
	
	if(x<wDst && y<hDst)
	{
		SrcDest[offset] =ValC;
	}
   
}

__global__ void
CudaMemCopyRect_kernel(unsigned short *dev_result, unsigned short * dev_src, int iwSrc, int ihSrc, int iwDst, int ihDst, int sx, int sy)
{
	int xDst = threadIdx.x + blockIdx.x*blockDim.x;
	int yDst = threadIdx.y + blockIdx.y*blockDim.y;


	if (xDst < iwDst && yDst < ihDst && xDst>=0 && yDst>=0)
	{
		int xSrc = xDst + sx;
		int ySrc = yDst + sy;
		if (xSrc < iwSrc && ySrc < ihSrc && xSrc >= 0 && ySrc >= 0)
		{
			dev_result[xDst + yDst*iwDst] = dev_src[xSrc + ySrc*iwSrc];
		}
		else
		{ 
			dev_result[xDst + yDst*iwDst] = 0;
		}


	}

}

__global__ void 
ShiftImage_kernel(unsigned short *dest, const unsigned short *src, const int width, const int height, const float dx, const float dy)
{
	int xDst= threadIdx.x + blockIdx.x*blockDim.x;
	int yDst= threadIdx.y + blockIdx.y*blockDim.y;

	
	if(xDst<width && yDst<height)
	{
		int xSrc=xDst+int(dx>0 ? dx+0.5f:dx-0.5f);
		int ySrc=yDst+int(dy>0 ? dy+0.5f:dy-0.5f);

		dest[xDst+yDst*width]=src[xSrc+ySrc*width];
	}
}

__global__ void
Div32Image_kernel(unsigned short *dst, const int width, const int height, float *src, const int fwidth, const int fheight, float divc)
{
	int x = threadIdx.x + blockIdx.x*blockDim.x;
	int y = threadIdx.y + blockIdx.y*blockDim.y;


	if (x<width && y<height)
	{
	

		dst[x + y*width] = unsigned short((src[x + y*fwidth]/ divc)+0.5);
	}
}

__global__ void
ShiftAddImage_kernel(float *dest, const int fwidth, const int fheight, const unsigned short *src, const int width, const int height, const float dx, const float dy)
{
	int xDst = threadIdx.x + blockIdx.x*blockDim.x;
	int yDst = threadIdx.y + blockIdx.y*blockDim.y;


	if (xDst<width && yDst<height)
	{
		int xSrc = xDst + int(dx>0 ? dx + 0.5f : dx - 0.5f);
		int ySrc = yDst + int(dy>0 ? dy + 0.5f : dy - 0.5f);

		dest[xDst + yDst*fwidth] += float(src[xSrc + ySrc*width]);
	}
}

__global__ void
Rotate_kernel(unsigned short *dest, unsigned short *src, int wDst, int hDst, float cos_deg, float sin_deg)
{

	int ix= threadIdx.x + blockIdx.x*blockDim.x;
	int iy= threadIdx.y + blockIdx.y*blockDim.y;
	
	int resultPix=0;
	int offset=ix+iy*wDst;

	bool bAvailable=false;
	
	if(ix<wDst && iy<hDst && ix>=0 && iy>=0)
	{
		float xc = (float)wDst/2;
		float yc = (float)hDst/2;

		float x = ((float)ix-xc)*cos_deg - ((float)iy-yc)*sin_deg + xc;
		float y = ((float)ix-xc)*sin_deg + ((float)iy-yc)*cos_deg + yc;

		bAvailable=true;

		if(x>=0.0f && x<=wDst-1 && y>=0 && y<=hDst-1)
		{
			int i= int(x);
			int j= int(y);

			float ki1=(x-(float)i);
			float ki0=1.0f-ki1;

			float kj1=(y-(float)j);
			float kj0=1.0f-kj1;

			float SumPix=0.0f;
			float SumK=0.0f;

	
			SumPix+=ki0*kj0*src[i+j*wDst];
			SumK+=ki0*kj0;

			if(i+1<=wDst-1)
			{
				SumPix+=ki1*kj0*src[(i+1)+j*wDst];
				SumK+=ki1*kj0;
			}
			if(j+1<=hDst-1)
			{
				SumPix+=ki0*kj1*src[i+(j+1)*wDst];
				SumK+=ki0*kj1;
			}
			if(i+1<=wDst-1 && j+1<=hDst-1)
			{
				SumPix+=ki1*kj1*src[(i+1)+(j+1)*wDst];
				SumK+=ki1*kj1;
			}
			resultPix = max(0,min(32767,int(SumPix/SumK+0.5)));
			
		}
		else
		{
			resultPix = src[offset];
		}

	}

	__syncthreads();

	if(bAvailable)
	{
		dest[offset] =resultPix;
	}
}


__global__ void
Zoom_kernel2(unsigned short *dest, unsigned short *src, int wDst, int hDst, float fZoom,  int Val)
{
	int ix= threadIdx.x + blockIdx.x*blockDim.x;
	int iy= threadIdx.y + blockIdx.y*blockDim.y;
	
	int resultPix=0;
	int offset=ix+iy*wDst;

	bool bAvailable=false;
	
	if(ix<wDst && iy<hDst)
	{
		float x=((float)ix-(float)wDst/2)*fZoom;
		float y=((float)iy-(float)hDst/2)*fZoom;

		x+=(float)wDst/2;
		y+=(float)hDst/2;

		

		if(x>=0.0f && x<=wDst-1 && y>=0 && y<=hDst-1)
		{
			int i= int(x);
			int j= int(y);

			float ki1=(x-(float)i);
			float ki0=1.0f-ki1;

			float kj1=(y-(float)j);
			float kj0=1.0f-kj1;

			float SumPix=0.0f;
			float SumK=0.0f;

	        if(i>=0 && i<wDst && j>=0 && j<hDst)
            {
			    SumPix+=ki0*kj0*src[i+j*wDst];
			    SumK+=ki0*kj0;
            }

			if((i+1)>=0 && (i+1)<wDst && j>=0 && j<hDst)
			{
				SumPix+=ki1*kj0*src[(i+1)+j*wDst];
				SumK+=ki1*kj0;
			}

			if(i>=0 && i<wDst && (j+1)>=0 && (j+1)<hDst)
			{
				SumPix+=ki0*kj1*src[i+(j+1)*wDst];
				SumK+=ki0*kj1;
			}

			if((i+1)>=0 && (i+1)<wDst && (j+1)>=0 && (j+1)<hDst)
			{
				SumPix+=ki1*kj1*src[(i+1)+(j+1)*wDst];
				SumK+=ki1*kj1;
			}

			resultPix = max(0,min(32767,int(SumPix/SumK+0.5)));
			bAvailable = true;
		}
		else
		{
			bAvailable = true;
			if (Val >= 0)
			{
				resultPix = Val;
			}
			else
			{
				resultPix = src[ix + iy*wDst];
			}
			
		}

	}

	__syncthreads();

	if(bAvailable)
	{
		dest[offset] =resultPix;
	}
}


__global__ void
Zoom_kernel(unsigned short *dest, unsigned short *src, int wDst, int hDst, float fZoom, float dx, float dy)
{
	int ix= threadIdx.x + blockIdx.x*blockDim.x;
	int iy= threadIdx.y + blockIdx.y*blockDim.y;
	
	int resultPix=0;
	int offset=ix+iy*wDst;

	bool bAvailable=false;
	
	if(ix<wDst && iy<hDst)
	{
		float x=((float)ix-(float)wDst/2)*fZoom+dx;
		float y=((float)iy-(float)hDst/2)*fZoom+dy;

		x+=(float)wDst/2;
		y+=(float)hDst/2;

		bAvailable=true;

		if(x>=0.0f && x<=wDst-1 && y>=0 && y<=hDst-1)
		{
			int i= int(x);
			int j= int(y);

			float ki1=(x-(float)i);
			float ki0=1.0f-ki1;

			float kj1=(y-(float)j);
			float kj0=1.0f-kj1;

			float SumPix=0.0f;
			float SumK=0.0f;

	
			SumPix+=ki0*kj0*src[i+j*wDst];
			SumK+=ki0*kj0;

			if(i+1<=wDst-1)
			{
				SumPix+=ki1*kj0*src[(i+1)+j*wDst];
				SumK+=ki1*kj0;
			}
			if(j+1<=hDst-1)
			{
				SumPix+=ki0*kj1*src[i+(j+1)*wDst];
				SumK+=ki0*kj1;
			}
			if(i+1<=wDst-1 && j+1<=hDst-1)
			{
				SumPix+=ki1*kj1*src[(i+1)+(j+1)*wDst];
				SumK+=ki1*kj1;
			}
			resultPix = max(0,min(32767,int(SumPix/SumK+0.5)));
			
		}
		else
		{
			resultPix = 0;
		}

	}

	__syncthreads();

	if(bAvailable)
	{
		dest[offset] =resultPix;
	}
}


//************************************************************************

// Wrapper for the __global__ call that sets up the texture and threads
extern "C" void Zoom_Dev(unsigned short *dev_result, unsigned short *dev_src, int iwDst, int ihDst, float fZoom, float fx, float fy)
{
	dim3 blocks((iwDst+31)/32,(ihDst+15)/16);
	dim3 threads(32,16);

	Zoom_kernel<<<blocks, threads>>>(dev_result, dev_src, iwDst,  ihDst, fZoom, fx, fy);
}


extern "C" void AddImages_Dev(unsigned short *dev_result, unsigned short *dev_src1, unsigned short *dev_src2, int iwDst, int ihDst)
{
	dim3 blocks((iwDst+31)/32,(ihDst+15)/16);
	dim3 threads(32,16);

	AddImages_kernel<<<blocks, threads>>>(dev_result, dev_src1, dev_src2, iwDst, ihDst);
}


extern "C" void Rotate_Dev(unsigned short *dev_result, unsigned short *dev_src, int iwDst, int ihDst, float fAngleDeg)
{
	dim3 blocks((iwDst+31)/32,(ihDst+15)/16);
	dim3 threads(32,16);

	float cos_deg=cos(fAngleDeg*PI/180);
	float sin_deg=sin(fAngleDeg*PI/180);

	Rotate_kernel<<<blocks, threads>>>(dev_result, dev_src, iwDst,  ihDst, cos_deg, sin_deg);
}



extern "C" void ResizeImage_Dev(unsigned short *dev_result, unsigned short *dev_src, int iwSrc, int ihSrc, int iwDst, int ihDst)
{
	dim3 blocks((iwDst+31)/32,(ihDst+15)/16);
	dim3 threads(32,16);

	if(iwDst<iwSrc)
	{
		ResizeDown_kernel<<<blocks, threads>>>(dev_result, dev_src, iwSrc, ihSrc,  iwDst,  ihDst);
	}
	else
	{
		ResizeUp_kernel<<<blocks, threads>>>(dev_result, dev_src, iwSrc, ihSrc,  iwDst,  ihDst);
	}
}

extern "C" void Threshold_Dev(unsigned short *dest, unsigned short *src, int wSrcDst, int hSrcDst, unsigned short threshold)
{
	dim3 blocks((wSrcDst+31)/32,(hSrcDst+15)/16);
	dim3 threads(32,16);

	Threshold_kernel<<<blocks, threads>>>(dest, src, wSrcDst, hSrcDst, threshold);
}


extern "C" void MirrorHorizontal_dev(unsigned short *srcDev, int iwSrc, int ihSrc, int step)
{
	dim3 blocks((iwSrc+31)/32,(ihSrc+15)/16);
	dim3 threads(32,16);

	MirrorHorizontal_kernel<<<blocks, threads>>>( srcDev, iwSrc, ihSrc, step);
}

extern "C" void MirrorHorizontal(unsigned short *src, int iwSrc, int ihSrc, int step)
{
	unsigned short *dev_srcDst=NULL;

	checkCudaErrors(cudaMalloc(&dev_srcDst, sizeof(unsigned short)*iwSrc*ihSrc));
    checkCudaErrors(cudaMemcpy(dev_srcDst, src,sizeof(unsigned short)*iwSrc*ihSrc, cudaMemcpyHostToDevice));

	MirrorHorizontal_dev(dev_srcDst, iwSrc, ihSrc, step);

	checkCudaErrors(cudaMemcpy(src, dev_srcDst, iwSrc*ihSrc*sizeof(unsigned short), cudaMemcpyDeviceToHost));


	checkCudaErrors(cudaFree(dev_srcDst));
}

extern "C" void LocalContrast_dev(unsigned short *dev_srcDst, int iw, int ih, float fVal1, float fVal2)
{
	unsigned short *dev_tmp = NULL;
	(cudaMalloc((void **)&dev_tmp, sizeof(unsigned short)*iw*ih));

	dim3 blocks((iw + 31) / 32, (ih + 15) / 16);
	dim3 threads(32, 16);


	LocalContrast_kernel << <blocks, threads >> >(dev_tmp, dev_srcDst, iw, ih, fVal1, fVal2);
	(cudaMemcpy(dev_srcDst, dev_tmp, iw*ih * sizeof(unsigned short), cudaMemcpyDeviceToDevice));
	(cudaFree(dev_tmp));
}

extern "C" void LocalContrast(unsigned short *SrcDst, int iw, int ih, float fVal1, float fVal2)
{
	
	unsigned short *dev_srcDst = NULL;

	(cudaMalloc(&dev_srcDst, sizeof(unsigned short)*iw*ih));
	(cudaMemcpy(dev_srcDst, SrcDst, sizeof(unsigned short)*iw*ih, cudaMemcpyHostToDevice));

	LocalContrast_dev(dev_srcDst, iw, ih, fVal1, fVal2);

	(cudaMemcpy(SrcDst, dev_srcDst, iw*ih * sizeof(unsigned short), cudaMemcpyDeviceToHost));
	
	(cudaFree(dev_srcDst));
}

extern "C" void RealLocalContrast_dev(unsigned short *dev_srcDst, int iw, int ih, float fVal1)
{
	unsigned short *dev_tmp = NULL;
	(cudaMalloc((void **)&dev_tmp, sizeof(unsigned short)*iw*ih));

	dim3 blocks((iw + 31) / 32, (ih + 15) / 16);
	dim3 threads(32, 16);


	RealLocalContrast_kernel << <blocks, threads >> >(dev_tmp, dev_srcDst, iw, ih, fVal1);
	(cudaMemcpy(dev_srcDst, dev_tmp, iw*ih * sizeof(unsigned short), cudaMemcpyDeviceToDevice));
	(cudaFree(dev_tmp));
}

extern "C" void RealLocalContrast(unsigned short *SrcDst, int iw, int ih, float fVal1)
{
	
	unsigned short *dev_srcDst = NULL;

	(cudaMalloc(&dev_srcDst, sizeof(unsigned short)*iw*ih));
	(cudaMemcpy(dev_srcDst, SrcDst, sizeof(unsigned short)*iw*ih, cudaMemcpyHostToDevice));

	RealLocalContrast_dev(dev_srcDst, iw, ih, fVal1);

	(cudaMemcpy(SrcDst, dev_srcDst, iw*ih * sizeof(unsigned short), cudaMemcpyDeviceToHost));
	
	(cudaFree(dev_srcDst));
}

extern "C" void ShiftXYImage_dev(unsigned short *Dst_dev, unsigned short *Src_dev, int iw, int ih, int dx, int dy, int val)
{


	dim3 blocks((iw + 31) / 32, (ih + 15) / 16);
	dim3 threads(32, 16);


	ShiftXYImage_kernel << <blocks, threads >> >(Dst_dev, Src_dev, iw, ih, dx, dy, val);


}

extern "C" void ShiftXYZoomImage(unsigned short *SrcDst, int iw, int ih, int dx, int dy, float fZoom, int val)
{
	unsigned short *dev_tmp = NULL;
	unsigned short *dev_srcDst = NULL;



	(cudaMalloc(&dev_srcDst, sizeof(unsigned short)*iw*ih));
	(cudaMemcpy(dev_srcDst, SrcDst, sizeof(unsigned short)*iw*ih, cudaMemcpyHostToDevice));

	(cudaMalloc((void **)&dev_tmp, sizeof(unsigned short)*iw*ih));

	dim3 blocks((iw + 31) / 32, (ih + 15) / 16);
	dim3 threads(32, 16);


	ShiftXYImage_kernel << <blocks, threads >> >(dev_tmp, dev_srcDst, iw, ih, dx, dy, val);
	Zoom_kernel2 << <blocks, threads >> >(dev_srcDst, dev_tmp, iw, ih, fZoom, val);

	
	(cudaMemcpy(SrcDst, dev_srcDst, iw*ih * sizeof(unsigned short), cudaMemcpyDeviceToHost));


	(cudaFree(dev_tmp));
	(cudaFree(dev_srcDst));
}


extern "C" void ShiftXYMapImage_dev(unsigned short *dev_Dst, unsigned short *Image_dev, unsigned short *ResultImage_dev, int iw, int ih, float sinTheta, float cosTheta, int val)
{
	dim3 blocks((iw + 31) / 32, (ih + 15) / 16);
	dim3 threads(32, 16);

	ShiftXYMapImage_kernel << <blocks, threads >> > (dev_Dst, Image_dev, ResultImage_dev, iw, ih, sinTheta, cosTheta, val);
}

extern "C" void ShiftXYImage(unsigned short *SrcDst, int iw, int ih, int dx, int dy)
{
	unsigned short *dev_tmp = NULL;
	unsigned short *dev_srcDst = NULL;



	(cudaMalloc(&dev_srcDst, sizeof(unsigned short)*iw*ih));
	(cudaMemcpy(dev_srcDst, SrcDst, sizeof(unsigned short)*iw*ih, cudaMemcpyHostToDevice));

	(cudaMalloc((void **)&dev_tmp, sizeof(unsigned short)*iw*ih));

	dim3 blocks((iw + 31) / 32, (ih + 15) / 16);
	dim3 threads(32, 16);


	ShiftXYImage_kernel << <blocks, threads >> >(dev_tmp, dev_srcDst, iw, ih, dx, dy, -1);
	
	(cudaMemcpy(SrcDst, dev_tmp, iw*ih * sizeof(unsigned short), cudaMemcpyDeviceToHost));


	(cudaFree(dev_tmp));
	(cudaFree(dev_srcDst));
}

extern "C" void ResizeImage(unsigned short *dst, unsigned short *src, int iwSrc, int ihSrc, int iwDst, int ihDst)
{
	unsigned short *dev_result=NULL;
	unsigned short *dev_src=NULL;

	cudaChannelFormatDesc desc = cudaCreateChannelDesc<unsigned short>();
	
	checkCudaErrors(cudaMalloc(&dev_src, sizeof(unsigned short)*iwSrc*ihSrc));
    checkCudaErrors(cudaMemcpy(dev_src, src,sizeof(unsigned short)*iwSrc*ihSrc, cudaMemcpyHostToDevice));

	checkCudaErrors(cudaMalloc( (void **)&dev_result, iwDst*ihDst*sizeof(unsigned short)));

	dim3 blocks((iwDst+31)/32,(ihDst+15)/16);
	dim3 threads(32,16);

	if(iwDst<iwSrc)
	{
		ResizeDown_kernel<<<blocks, threads>>>(dev_result, dev_src, iwSrc, ihSrc,  iwDst,  ihDst);
	}
	else
	{
		ResizeUp_kernel<<<blocks, threads>>>(dev_result, dev_src, iwSrc, ihSrc,  iwDst,  ihDst);
	}
	checkCudaErrors(cudaMemcpy(dst, dev_result, iwDst*ihDst*sizeof(unsigned short), cudaMemcpyDeviceToHost));


	checkCudaErrors(cudaFree(dev_src));
	checkCudaErrors(cudaFree(dev_result));
}

extern "C" void ResizeImage8u(unsigned char *dst, unsigned char *src, int iwSrc, int ihSrc, int iwDst, int ihDst)
{
	unsigned char *dev_result=NULL;
	unsigned char *dev_src=NULL;

	
	checkCudaErrors(cudaMalloc(&dev_src, sizeof(unsigned char)*iwSrc*ihSrc));
    checkCudaErrors(cudaMemcpy(dev_src, src,sizeof(unsigned char)*iwSrc*ihSrc, cudaMemcpyHostToDevice));

	checkCudaErrors(cudaMalloc( &dev_result, iwDst*ihDst*sizeof(unsigned char)));

	dim3 blocks((iwDst+31)/32,(ihDst+15)/16);
	dim3 threads(32,16);

	if(iwDst<iwSrc)
	{
		ResizeDown8u_kernel<<<blocks, threads>>>(dev_result, dev_src, iwSrc, ihSrc,  iwDst,  ihDst);
	}
	else
	{
		ResizeUp8u_kernel<<<blocks, threads>>>(dev_result, dev_src, iwSrc, ihSrc,  iwDst,  ihDst);
	}
	checkCudaErrors(cudaMemcpy(dst, dev_result, iwDst*ihDst*sizeof(unsigned char), cudaMemcpyDeviceToHost));


	checkCudaErrors(cudaFree(dev_src));
	checkCudaErrors(cudaFree(dev_result));
}





extern "C" void Zoom(unsigned short *dst, unsigned short *src, int iwDst, int ihDst, float fZoom, float fx, float fy)
{
	unsigned short *dev_result=NULL;
	unsigned short *dev_src=NULL;

	cudaChannelFormatDesc desc = cudaCreateChannelDesc<unsigned short>();
	
	checkCudaErrors(cudaMalloc(&dev_src, sizeof(unsigned short)*iwDst*ihDst));
    checkCudaErrors(cudaMemcpy(dev_src, src, sizeof(unsigned short)*iwDst*ihDst, cudaMemcpyHostToDevice));

	checkCudaErrors(cudaMalloc( (void **)&dev_result, iwDst*ihDst*sizeof(unsigned short)));


	dim3 blocks((iwDst+31)/32,(ihDst+15)/16);
	dim3 threads(32,16);

	Zoom_kernel<<<blocks, threads>>>(dev_result, dev_src, iwDst,  ihDst, fZoom, fx, fy);

	checkCudaErrors(cudaMemcpy(dst, dev_result, iwDst*ihDst*sizeof(unsigned short), cudaMemcpyDeviceToHost));


	checkCudaErrors(cudaFree(dev_src));
	checkCudaErrors(cudaFree(dev_result));
}

extern "C" void ShiftAddImage_dev(float *devDest, int fWidth, int fHeight, unsigned short *dev_src, int iwDst, int ihDst, const float dx, const float dy)
{

	dim3 blocks((iwDst + 31) / 32, (ihDst + 15) / 16);
	dim3 threads(32, 16);


	ShiftAddImage_kernel << <blocks, threads >> >(devDest, fWidth, fHeight, dev_src, iwDst, ihDst, dx, dy);


}

extern "C" void Div32Image_dev(unsigned short *dev_dst, int iwDst, int ihDst, float *dev_src, int fWidth, int fHeight,  float DivC)
{

	dim3 blocks((iwDst + 31) / 32, (ihDst + 15) / 16);
	dim3 threads(32, 16);


	Div32Image_kernel << <blocks, threads >> >(dev_dst, iwDst, ihDst, dev_src, fWidth, fHeight, DivC);


}

extern "C" void CudaMemCopyRect_dev(unsigned short *dev_result, unsigned short * dev_src, int iwSrc, int ihSrc, int iwDst, int ihDst, int sx, int sy)
{
	dim3 blocks((iwDst + 31) / 32, (ihDst + 15) / 16);
	dim3 threads(32, 16);

	CudaMemCopyRect_kernel << <blocks, threads >> > (dev_result, dev_src, iwSrc, ihSrc, iwDst, ihDst, sx, sy);

}



extern "C" void ShiftImage_dev(unsigned short *dev_src, int iwDst, int ihDst, const float dx, const float dy)
{
	unsigned short *dev_result = NULL;
	checkCudaErrors(cudaMalloc((void **)&dev_result, iwDst*ihDst * sizeof(unsigned short)));

	dim3 blocks((iwDst + 31) / 32, (ihDst + 15) / 16);
	dim3 threads(32, 16);


	ShiftImage_kernel << <blocks, threads >> >(dev_result, dev_src, iwDst, ihDst, dx, dy);

	checkCudaErrors(cudaFree(dev_result));
}

extern "C" void ShiftImage(unsigned short *SrcDst, int iwDst, int ihDst, const float dx, const float dy)
{
	unsigned short *dev_result = NULL;
	unsigned short *dev_src = NULL;


	checkCudaErrors(cudaMalloc(&dev_src, sizeof(unsigned short)*iwDst*ihDst));
	checkCudaErrors(cudaMemcpy(dev_src, SrcDst, sizeof(unsigned short)*iwDst*ihDst, cudaMemcpyHostToDevice));

	checkCudaErrors(cudaMalloc((void **)&dev_result, iwDst*ihDst * sizeof(unsigned short)));

	dim3 blocks((iwDst + 31) / 32, (ihDst + 15) / 16);
	dim3 threads(32, 16);


	ShiftImage_kernel << <blocks, threads >> >(dev_result, dev_src, iwDst, ihDst, dx, dy);


	checkCudaErrors(cudaMemcpy(SrcDst, dev_result, iwDst*ihDst * sizeof(unsigned short), cudaMemcpyDeviceToHost));


	checkCudaErrors(cudaFree(dev_src));
	checkCudaErrors(cudaFree(dev_result));
}


extern "C" void Rotate(unsigned short *SrcDst, int iwDst, int ihDst, float fAngleDeg)
{
	unsigned short *dev_result=NULL;
	unsigned short *dev_src=NULL;

	
	checkCudaErrors(cudaMalloc(&dev_src, sizeof(unsigned short)*iwDst*ihDst));
    checkCudaErrors(cudaMemcpy(dev_src, SrcDst, sizeof(unsigned short)*iwDst*ihDst, cudaMemcpyHostToDevice));

	checkCudaErrors(cudaMalloc( (void **)&dev_result, iwDst*ihDst*sizeof(unsigned short)));

	Rotate_Dev(dev_result, dev_src, iwDst, ihDst, fAngleDeg);


	checkCudaErrors(cudaMemcpy(SrcDst, dev_result, iwDst*ihDst*sizeof(unsigned short), cudaMemcpyDeviceToHost));


	checkCudaErrors(cudaFree(dev_src));
	checkCudaErrors(cudaFree(dev_result));
}


extern "C" void Rotate8u(unsigned char* SrcDst, int iwDst, int ihDst, float fAngleDeg)
{
	unsigned char* dev_result = NULL;
	unsigned char* dev_src = NULL;


	checkCudaErrors(cudaMalloc(&dev_src, sizeof(unsigned char) * iwDst * ihDst));
	checkCudaErrors(cudaMemcpy(dev_src, SrcDst, sizeof(unsigned char) * iwDst * ihDst, cudaMemcpyHostToDevice));

	checkCudaErrors(cudaMalloc((void**)&dev_result, iwDst * ihDst * sizeof(unsigned char)));

	Rotate8u_Dev(dev_result, dev_src, iwDst, ihDst, fAngleDeg);


	checkCudaErrors(cudaMemcpy(SrcDst, dev_result, iwDst * ihDst * sizeof(unsigned char), cudaMemcpyDeviceToHost));


	checkCudaErrors(cudaFree(dev_src));
	checkCudaErrors(cudaFree(dev_result));
}



extern "C" void SetC_Dev(unsigned short *dev_SrcDest,int wDst, int hDst, unsigned short ValC)
{
	dim3 blocks((wDst+31)/32,(hDst+15)/16);
	dim3 threads(32,16);

	Set_kernel<<<blocks, threads>>>(dev_SrcDest, wDst, hDst, ValC);
}


__global__ void
Zoom8uxy_kernel(unsigned char *dest, unsigned char *src, int wDst, int hDst, float fZoomx, float fZoomxy)
{
	int ix = threadIdx.x + blockIdx.x*blockDim.x;
	int iy = threadIdx.y + blockIdx.y*blockDim.y;

	int resultPix = 0;
	int offset = ix + iy*wDst;

	bool bAvailable = false;

	if (ix<wDst && iy<hDst)
	{
		float x = ((float)ix - (float)wDst / 2)*fZoomx;
		float y = ((float)iy - (float)hDst / 2)*fZoomxy;

		x += (float)wDst / 2;
		y += (float)hDst / 2;

		bAvailable = true;

		if (x >= 0.0f && x <= wDst - 1 && y >= 0 && y <= hDst - 1)
		{
			int i = int(x);
			int j = int(y);

			float ki1 = (x - (float)i);
			float ki0 = 1.0f - ki1;

			float kj1 = (y - (float)j);
			float kj0 = 1.0f - kj1;

			float SumPix = 0.0f;
			float SumK = 0.0f;


			SumPix += ki0*kj0*src[i + j*wDst];
			SumK += ki0*kj0;

			if (i + 1 <= wDst - 1)
			{
				SumPix += ki1*kj0*src[(i + 1) + j*wDst];
				SumK += ki1*kj0;
			}
			if (j + 1 <= hDst - 1)
			{
				SumPix += ki0*kj1*src[i + (j + 1)*wDst];
				SumK += ki0*kj1;
			}
			if (i + 1 <= wDst - 1 && j + 1 <= hDst - 1)
			{
				SumPix += ki1*kj1*src[(i + 1) + (j + 1)*wDst];
				SumK += ki1*kj1;
			}
			resultPix = max(0, min(255, int(SumPix / SumK + 0.5)));

		}
		else
		{
			resultPix = 0;
		}

	}

	__syncthreads();

	if (bAvailable)
	{
		dest[offset] = resultPix;
	}
}

extern "C" void Zoom8uxy_Dev(unsigned char *dev_result, unsigned char *dev_src, int iwDst, int ihDst, float fZoomx, float fZoomy)
{
	dim3 blocks((iwDst + 31) / 32, (ihDst + 15) / 16);
	dim3 threads(32, 16);

	Zoom8uxy_kernel << <blocks, threads >> >(dev_result, dev_src, iwDst, ihDst, fZoomx, fZoomy);
}

extern "C" void Rotate8u_Dev(unsigned char *dev_result, unsigned char *dev_src, int iwDst, int ihDst, float fAngleRad)
{
	dim3 blocks((iwDst + 31) / 32, (ihDst + 15) / 16);
	dim3 threads(32, 16);

	float cos_deg = cos(fAngleRad);
	float sin_deg = sin(fAngleRad);

	Rotate8u_kernel << <blocks, threads >> >(dev_result, dev_src, iwDst, ihDst, cos_deg, sin_deg);
}




__global__ void
Zoom8u_kernel(unsigned char *dest, unsigned char *src, int DstSrc8u_step, int DstSrc8u_w, int DstSrc8u_h, float fZoom, float dx, float dy)
{
	int ix = threadIdx.x + blockIdx.x*blockDim.x;
	int iy = threadIdx.y + blockIdx.y*blockDim.y;

	int resultPix = 0;
	int dst_offset = ix + iy*DstSrc8u_step;

	bool bAvailable = false;

	if (ix<DstSrc8u_w && iy<DstSrc8u_h)
	{
		float x = ((float)ix - (float)DstSrc8u_w / 2)*fZoom + dx;
		float y = ((float)iy - (float)DstSrc8u_h / 2)*fZoom + dy;

		x += (float)DstSrc8u_w / 2;
		y += (float)DstSrc8u_h / 2;

		bAvailable = true;

		int i = int(x);
		int j = int(y);

		if (i >= 0 && i < DstSrc8u_w && y >= 0 && y < DstSrc8u_h)
		{
			float ki1 = (x - (float)i);
			float ki0 = 1.0f - ki1;

			float kj1 = (y - (float)j);
			float kj0 = 1.0f - kj1;

			float SumPix = 0.0f;
			float SumK = 0.0f;


			SumPix += ki0*kj0*src[i + j*DstSrc8u_step];
			SumK += ki0*kj0;

			if (i + 1 <= DstSrc8u_w - 1)
			{
				SumPix += ki1*kj0*src[(i + 1) + j*DstSrc8u_step];
				SumK += ki1*kj0;
			}
			if (j + 1 <= DstSrc8u_h - 1)
			{
				SumPix += ki0*kj1*src[i + (j + 1)*DstSrc8u_step];
				SumK += ki0*kj1;
			}
			if (i + 1 <= DstSrc8u_w - 1 && j + 1 <= DstSrc8u_h - 1)
			{
				SumPix += ki1*kj1*src[(i + 1) + (j + 1)*DstSrc8u_step];
				SumK += ki1*kj1;
			}
			int result = SumK == 0.0f ? 0 : int(SumPix / SumK + 0.5);
			resultPix = max(0, min(255, result));
		}
		else
		{
			resultPix = src[ix + iy*DstSrc8u_step];
		}

	}

	__syncthreads();

	if (bAvailable)
	{
		dest[dst_offset] = resultPix;
	}
}

extern "C" void Zoom8u_Dev(unsigned char *dev_result, unsigned char *dev_src, int dev_DstSrc_step, int dev_DstSrc_w, int dev_DstSrc_h, float fZoom, float fx, float fy)
{
	dim3 blocks((dev_DstSrc_w + 31) / 32, (dev_DstSrc_h + 15) / 16);
	dim3 threads(32, 16);

	Zoom8u_kernel << <blocks, threads >> >(dev_result, dev_src, dev_DstSrc_step, dev_DstSrc_w, dev_DstSrc_h, fZoom, fx, fy);
}

extern "C" void Zoom8(unsigned char *dst, unsigned char *src, int step, int iwDst, int ihDst, float fZoom)
{
	unsigned char* dev_result = NULL;
	unsigned char* dev_src = NULL;

	cudaMalloc(&dev_src, sizeof(unsigned char)*step*ihDst);
	cudaMemcpy(dev_src, src, sizeof(unsigned char)*step*ihDst, cudaMemcpyHostToDevice);

	cudaMalloc(&dev_result, sizeof(unsigned char)*step*ihDst);

	Zoom8u_Dev(dev_result, dev_src, step, iwDst, ihDst, fZoom);

	cudaMemcpy(dst, dev_result, sizeof(unsigned char)*step*ihDst, cudaMemcpyDeviceToHost);

	cudaFree(dev_src);
	cudaFree(dev_result);
}