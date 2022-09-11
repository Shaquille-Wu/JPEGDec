#include <iostream>
#include <string.h>
#include <math.h>
#include <chrono>
#include <vector>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
}

#include "KyMEM.h"
#include "KyDataDef.h"

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

KY_VID_DECODE_FRAME* CreateRGBFrameFromAVFrame(AVFrame* pSrcFrame, AVPixelFormat ePixelFmt, int iLineBytesAligned)
{
  KY_VID_DECODE_FRAME*  pDstFrame  = 0 ;
  int                   i          = 0 ;
  int                   iDepth     = 0 ;
  if(AV_PIX_FMT_YUV420P == ePixelFmt || AV_PIX_FMT_YUVJ420P == ePixelFmt ||
     AV_PIX_FMT_YUV422P == ePixelFmt || AV_PIX_FMT_YUVJ422P == ePixelFmt ||
     AV_PIX_FMT_YUV444P == ePixelFmt || AV_PIX_FMT_YUVJ444P == ePixelFmt)
  {
    if(AV_PIX_FMT_YUVJ420P == ePixelFmt){
      ePixelFmt = AV_PIX_FMT_YUV420P;
    }else if(AV_PIX_FMT_YUVJ422P == ePixelFmt){
      ePixelFmt = AV_PIX_FMT_YUV422P;
    }else if(AV_PIX_FMT_YUVJ444P == ePixelFmt){
      ePixelFmt = AV_PIX_FMT_YUV444P;
    }
    pDstFrame = (KY_VID_DECODE_FRAME*)KyAllocMem(sizeof(KY_VID_DECODE_FRAME)) ;
    memset(pDstFrame, 0, sizeof(KY_VID_DECODE_FRAME)) ;

    pDstFrame->usColorSpace    = KY_COLOR_SPACE_BGR24 ;
    iDepth                     = 24 ;

    pDstFrame->uFrameWidth     = pSrcFrame->width ;
    pDstFrame->uFrameHeight    = pSrcFrame->height ;

    if (0 == iLineBytesAligned || 1 == iLineBytesAligned || 4 == iLineBytesAligned)
    {
      pDstFrame->uFrameLineSize[0] = 4 * pDstFrame->uFrameWidth ;
    }
    else if (8 == iLineBytesAligned)
    {
      KY_LINESIZE_ALIGNED8(pDstFrame->uFrameLineSize[0], pDstFrame->uFrameWidth, iDepth);
    }
    else if (16 == iLineBytesAligned)
    {
      KY_LINESIZE_ALIGNED16(pDstFrame->uFrameLineSize[0], pDstFrame->uFrameWidth, iDepth);
    }
    else if (32 == iLineBytesAligned)
    {
      KY_LINESIZE_ALIGNED32(pDstFrame->uFrameLineSize[0], pDstFrame->uFrameWidth, iDepth);
    }
    else
    {
      KY_LINESIZE_ALIGNED4(pDstFrame->uFrameLineSize[0], pDstFrame->uFrameWidth, iDepth);
    }

    pDstFrame->uFrameBufLen  = pDstFrame->uFrameLineSize[0] * pDstFrame->uFrameHeight ;
    pDstFrame->uFrameSize    = pDstFrame->uFrameBufLen ;
    pDstFrame->pFrameBuf     = (unsigned char*)KyAllocMem(pDstFrame->uFrameBufLen) ;
    pDstFrame->pFrameData[0] = pDstFrame->pFrameBuf ;

    if (0 == pDstFrame->pFrameBuf)
      return pDstFrame ;

    SwsContext* pCvtCtxColor2RGB = sws_getContext(pDstFrame->uFrameWidth,
                                                  pDstFrame->uFrameHeight,
                                                  ePixelFmt,
                                                  pSrcFrame->width,
                                                  pSrcFrame->height,
                                                  AV_PIX_FMT_BGR24,
                                                  SWS_BICUBIC,
                                                  NULL,
                                                  NULL,
                                                  NULL) ;
    int iRes = sws_scale(pCvtCtxColor2RGB,
                         pSrcFrame->data,
                         (const int*)(pSrcFrame->linesize),
                         0,
                         pDstFrame->uFrameHeight,
                         pDstFrame->pFrameData,
                         (const int*)(pDstFrame->uFrameLineSize)) ;

    sws_freeContext(pCvtCtxColor2RGB) ;
  }

  return pDstFrame ;
}

int main(int argc, char** argv){
  AVCodec const*    pCodecForDecode  = avcodec_find_decoder(AV_CODEC_ID_MJPEG) ;

  if (NULL == pCodecForDecode){
    printf("cannot find AV_CODEC_ID_MJPEG\n");
    return 0;
  }

  printf("found AV_CODEC_ID_MJPEG\n");

  AVCodecContext* pCodecCtxForDecode = avcodec_alloc_context3(pCodecForDecode) ;

  pCodecCtxForDecode->codec_id    = AV_CODEC_ID_MJPEG ;
  pCodecCtxForDecode->codec_type  = AVMEDIA_TYPE_VIDEO ;

  if (pCodecForDecode->capabilities & AV_CODEC_CAP_TRUNCATED)
    pCodecCtxForDecode->flags |= AV_CODEC_FLAG_TRUNCATED;

  if (avcodec_open2(pCodecCtxForDecode, pCodecForDecode, NULL) < 0)
  {
    av_free(pCodecCtxForDecode) ;
    pCodecCtxForDecode = NULL ;
    pCodecForDecode    = NULL ;
    printf("cannot find open avcodec\n");
    return 0 ;
  }
  printf("open avcodec\n");

  AVFrame*            pFrameYUV      = av_frame_alloc() ;
  int                 iDeCodedLen    = 0 ;
  int                 iImgFinished   = 0 ;

  FILE*               jpegFile       = fopen("../WechatIMG1.jpeg", "rb");
  if(NULL == jpegFile){
    printf("cannot open jpeg file\n");
    return 0 ;
  }
  fseek(jpegFile, 0, SEEK_END) ;
  int iFileSize = ftell(jpegFile) ;
  fseek(jpegFile, 0, SEEK_SET) ;
  unsigned char*      pEncBuf        = new unsigned char[iFileSize] ;
  int                 iEncSize       = iFileSize ;
  int                 iRes           = 0 ;
  fread(pEncBuf, iEncSize, 1, jpegFile);
  fflush(jpegFile);
  fclose(jpegFile);

  int                   iDepth     = 8;
  KY_VID_DECODE_FRAME*  pDecodeImg = NULL;
  AVPacket              avPkt      = { 0 };
  av_init_packet(&avPkt);

  avPkt.data = pEncBuf ;
  avPkt.size = iEncSize ;

  iRes = avcodec_send_packet(pCodecCtxForDecode, &avPkt) ;
  if (iRes < 0)
    goto DECODE_IMAGE_LEAVE ;
  if (iRes >= 0)
  {
    iRes = avcodec_receive_frame(pCodecCtxForDecode, pFrameYUV) ;
    if (iRes == AVERROR(EAGAIN) || iRes == AVERROR_EOF)
      goto DECODE_IMAGE_LEAVE ;
    else if (iRes < 0)
      goto DECODE_IMAGE_LEAVE ;

    pDecodeImg = CreateRGBFrameFromAVFrame(pFrameYUV, pCodecCtxForDecode->pix_fmt, 4) ;
  }
  printf("decode ok\n");

  if(KY_COLOR_SPACE_BGR24 == pDecodeImg->usColorSpace ||
     KY_COLOR_SPACE_RGB24 == pDecodeImg->usColorSpace){
    iDepth = 24;
  }else if(KY_COLOR_SPACE_BGR32 == pDecodeImg->usColorSpace ||
           KY_COLOR_SPACE_RGB32 == pDecodeImg->usColorSpace){
    iDepth = 32;
  }

  SaveBMP("WechatIMG1.bmp", 
          pDecodeImg->pFrameData[0], 
          pDecodeImg->uFrameWidth,
          pDecodeImg->uFrameHeight,
          pDecodeImg->uFrameLineSize[0],
          iDepth, 
          0);

DECODE_IMAGE_LEAVE:
  av_packet_unref(&avPkt) ;

  if (0 != pCodecCtxForDecode)
  {
    avcodec_flush_buffers(pCodecCtxForDecode) ;
    avcodec_close(pCodecCtxForDecode) ;
    av_free(pCodecCtxForDecode) ;
    pCodecCtxForDecode  = 0 ;
    pCodecForDecode     = 0 ;
  }

  // Free the YUV frame
  if (0 != pFrameYUV)
  {
    av_frame_free(&pFrameYUV) ;
    pFrameYUV = 0 ;
  }

  if(NULL != pDecodeImg){
    if(pDecodeImg->pFrameData[0]){
      KyFreeMem(pDecodeImg->pFrameData[0]);
    }
    KyFreeMem(pDecodeImg);
  }

  return 0 ;
}