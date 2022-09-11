#include <iostream>
#include <string.h>
#include <math.h>
#include <chrono>
#include <vector>

#include <jpeglib.h>
#include <turbojpeg.h>

#ifdef _WINDOWS
#include <Windows.h>
#else
#ifndef WIN32
#pragma pack(push, 2)
typedef struct tagBITMAPFILEHEADER {
    unsigned short int    bfType ;
    unsigned int          bfSize ;
    unsigned short int    bfReserved1 ;
    unsigned short int    bfReserved2 ;
    unsigned int          bfOffBits ;
}BITMAPFILEHEADER, *PBITMAPFILEHEADER ;
#pragma pack(pop)

typedef struct tagBITMAPINFOHEADER{
    unsigned int          biSize ;
    int                   biWidth ;
    int                   biHeight;
    unsigned short int    biPlanes;
    unsigned short int    biBitCount;
    unsigned int      	  biCompression;
    unsigned int      	  biSizeImage;
    int                   biXPelsPerMeter;
    int                   biYPelsPerMeter;
    unsigned int      	  biClrUsed;
    unsigned int      	  biClrImportant;
} BITMAPINFOHEADER, *PBITMAPINFOHEADER;

typedef struct tagRGBQUAD {
    unsigned char         rgbBlue;
    unsigned char         rgbGreen;
    unsigned char         rgbRed;
    unsigned char         rgbReserved;
}RGBQUAD, *PRGBQUAD;

#define BI_RGB        0L
#define BI_RLE8       1L
#define BI_RLE4       2L
#define BI_BITFIELDS  3L
#define BI_JPEG       4L
#define BI_PNG        5L

#endif

#endif


static void SaveBMP(char const*           szFileName,
                    unsigned char const*  pSrcImg, 
                    int                   iWidth, 
                    int                   iHeight, 
                    int                   iLineSize,
                    int                   iDepth,
                    int                   iQuality)
{
  int              i              = 0 ;
  int              iQuaLen        = 0 ;
  int              iBMPLineSize   = 0 ;
  unsigned char*   pBMPImg        = (unsigned char*)pSrcImg ;

  BITMAPFILEHEADER        stBmpFileHeader ;
  BITMAPINFOHEADER        stBmpInfoHeader ;
  RGBQUAD                *pQuad          = new RGBQUAD[256] ;
  memset(&stBmpFileHeader, 0, sizeof(BITMAPFILEHEADER)) ;
  memset(&stBmpInfoHeader, 0, sizeof(BITMAPINFOHEADER)) ;
  memset(pQuad, 0, 256 * sizeof(RGBQUAD)) ;

  iBMPLineSize = (((iWidth * (iDepth >> 3)) + 3) >> 2) << 2;

  if(8 == iDepth)
  {
        for (i = 0 ; i < 256 ; i++)
        {
            pQuad[i].rgbRed = pQuad[i].rgbGreen = pQuad[i].rgbBlue = i ;
            pQuad[i].rgbReserved = 0 ;
        }
        iQuaLen = 256 * sizeof(RGBQUAD) ;
  }

  stBmpFileHeader.bfType                  = 0x4D42 ; //BM
  stBmpFileHeader.bfOffBits               = sizeof(BITMAPFILEHEADER) +  sizeof(BITMAPINFOHEADER) + iQuaLen ;
  stBmpFileHeader.bfSize                  = stBmpFileHeader.bfOffBits + iBMPLineSize * iHeight ;
  stBmpInfoHeader.biSize                  = sizeof(BITMAPINFOHEADER) ;
  stBmpInfoHeader.biWidth                 = iWidth ;
  stBmpInfoHeader.biHeight                = iHeight ;
  stBmpInfoHeader.biPlanes                = 1 ;
  stBmpInfoHeader.biBitCount              = iDepth ;
  stBmpInfoHeader.biCompression           = BI_RGB ;
  stBmpInfoHeader.biSizeImage             = iBMPLineSize * iHeight ;
  stBmpInfoHeader.biXPelsPerMeter         = 7872 ;
  stBmpInfoHeader.biYPelsPerMeter         = 7872 ;
  stBmpInfoHeader.biClrUsed               = 0 ;
  stBmpInfoHeader.biClrImportant          = 0 ;

  if(iBMPLineSize != iLineSize)
  {
    pBMPImg = new unsigned char[stBmpInfoHeader.biSizeImage] ;
    for(i = 0 ; i < iHeight ; i ++)
      memcpy(pBMPImg + i * iBMPLineSize, pSrcImg + i * iLineSize, (iDepth >> 3) * iWidth) ;
  }

  FILE *fp = NULL ;
#ifdef _WINDOWS
  fopen_s(&fp, szFileName, "wb") ;
#else
  fp = fopen(szFileName, "wb") ;
#endif
  if(NULL != fp)
  {
    fwrite(&stBmpFileHeader, sizeof(BITMAPFILEHEADER), 1, fp) ;
    fwrite(&stBmpInfoHeader, sizeof(BITMAPINFOHEADER), 1, fp) ;
    if(8 == stBmpInfoHeader.biBitCount)
      fwrite(pQuad, sizeof(RGBQUAD), 256, fp) ;

    for(i = 0 ; i < iHeight ; i ++)
      fwrite(pBMPImg + (iHeight - 1 - i) * iBMPLineSize, 1, iBMPLineSize, fp) ;

    fflush(fp) ;
    fclose(fp) ;
  }

  if(NULL != pBMPImg && pBMPImg != pSrcImg)
    delete pBMPImg ;
  pBMPImg = NULL ;

  delete[] pQuad ;
}

int main(int argc, char** argv){
  std::string  jpeg_file_name = std::string("../WechatIMG4149.jpeg");

  FILE* infile = fopen(jpeg_file_name.c_str(), "rb");
	if (infile == nullptr) {
		fprintf(stderr, "can't open %s\n", jpeg_file_name.c_str());
		return 0;
	}
 
	fseek(infile, 0, SEEK_END);
	unsigned long  srcSize = ftell(infile);
	unsigned char* srcBuf  = (unsigned char *)tjAlloc(srcSize);
	fseek(infile, 0, SEEK_SET);
	fread(srcBuf, srcSize, 1, infile);
	fclose(infile);
 
	tjhandle handle = tjInitDecompress();
  int            width    = 0;
  int            height   = 0;
  int            channels = 1;
  unsigned char* dstBuf   = NULL;
  auto start    = std::chrono::system_clock::now();
  for(size_t i = 0 ; i < 1000 ; i ++){
    int subsamp, cs;
    int ret = tjDecompressHeader3(handle, srcBuf, srcSize, &width, &height, &subsamp, &cs);
    if (cs == TJCS_GRAY) channels = 1;
    else channels = 3;
  
    int            ps     = tjPixelSize[cs];
    if(NULL == dstBuf)
      dstBuf = (unsigned char *)tjAlloc(width * height * ps);
    ret = tjDecompress2(handle, srcBuf, srcSize, dstBuf, width, 0, height, cs, TJFLAG_FASTDCT);
    if(0 != ret){
      printf("decode error: %d\n", ret);
    }
  }
  auto end      = std::chrono::system_clock::now();
  auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
  std::cout << "run cost(us): " << duration << std::endl;

  SaveBMP("last_image.bmp", 
          dstBuf, 
          width,
          height,
          width * channels,
          channels << 3, 
          0);

  tjDestroy(handle);
  tjFree(srcBuf);
  tjFree(dstBuf);

  return 0 ;
}