#include <iostream>
#include <string.h>
#include <math.h>
#include <chrono>
#include <vector>
#include <getopt.h>
#include <sys/types.h>
#include <dirent.h>
#include <sys/stat.h>
#include <algorithm>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <libavutil/pixdesc.h>
#include <libavutil/log.h>
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

static std::string               get_file_ext(const std::string& src)
{
    int    len        = src.length();
    char*  ext        = new char[len + 1];
    int    start_pos  = 0;
    int    i          = 0;

    for(i = len - 1 ; i >= 0 ; i --)
    {
        if('.' == src.c_str()[i])
        {
            start_pos = i;
            break;
        }
    }

    memset(ext, 0, len + 1);
    for(i = start_pos ; i < len ; i ++)
    {
        ext[i - start_pos] = src.c_str()[i];
    }

    std::string   result = ext;

    delete[] ext;
    return result;
}

#define MAX_PATH_LEN 1024
static std::vector<std::string>  travel_image_dir(const std::string&                src_image_dir, 
                                                  const std::vector<std::string>&   ext_name_list,
                                                  const std::string&                sub_dir)
{
  DIR*                         d                  = NULL;
  struct dirent*               dp                 = NULL;
  struct stat                  st;    
  char                         p[MAX_PATH_LEN]    = {0};
  std::vector<std::string>     image_file_list;
  int                          i                  = 0;
  int                          ext_name_list_cnt  = (int)(ext_name_list.size());
  std::string                  cur_sub_dir        = "";    

  if(stat(src_image_dir.c_str(), &st) < 0 || !S_ISDIR(st.st_mode)) {
      printf("invalid path: %s\n", src_image_dir.c_str());
      return image_file_list;
  }

  if(!(d = opendir(src_image_dir.c_str()))) {
      printf("opendir[%s] error: %m\n", src_image_dir.c_str());
      return image_file_list;
  }

  while((dp = readdir(d)) != NULL) 
  {
      if((!strncmp(dp->d_name, ".", 1)) || (!strncmp(dp->d_name, "..", 2)))
          continue;

      snprintf(p, sizeof(p) - 1, "%s/%s", src_image_dir.c_str(), dp->d_name);
      stat(p, &st);
      if(!S_ISDIR(st.st_mode)) 
      {
          std::string    cur_file      = dp->d_name;
          int            str_len       = cur_file.length();
          bool           found         = false;
          std::string    cur_file_ext  = get_file_ext(cur_file);

          for(i = 0 ; i < ext_name_list_cnt ; i ++)
          {
              const std::string&  cur_ext_name = ext_name_list.at(i);
              if(0 == strcasecmp(cur_ext_name.c_str(), cur_file_ext.c_str()))
              {
                  found = true;
                  break;
              }
          }
          if(true == found)
          {
              if("" != sub_dir)
                  cur_file = sub_dir + std::string("/") + cur_file;
              image_file_list.push_back(cur_file);
          }
      } 
      else
      {
          if("" == sub_dir)
              cur_sub_dir = dp->d_name;
          else
              cur_sub_dir = sub_dir + "/" + dp->d_name;
          std::vector<std::string> sub_image_file_list = travel_image_dir(std::string(p), ext_name_list, cur_sub_dir);
          image_file_list.insert(image_file_list.end(),sub_image_file_list.begin(),sub_image_file_list.end());
      }
  }
  closedir(d);

  std::sort(image_file_list.begin(), image_file_list.end());

  return image_file_list;
}

int main(int argc, char** argv){
  av_log_set_level(AV_LOG_PANIC);
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

  int                   iDepth       = 8;
  KY_VID_DECODE_FRAME*  pDecodeImg   = NULL;
  AVPacket              avPkt        = { 0 };

  printf("start travel image directory\n");
  std::string               top_dir    = "/localdata/cn-customer-engineering/datasets/imagenet1k/train/n07718472";
  std::vector<std::string>  jpeg_files = travel_image_dir(top_dir,
                                                          std::vector<std::string>({std::string(".jpg"), std::string(".jpeg")}), 
                                                          std::string(""));
  printf("total image file: %llu\n", jpeg_files.size());
  bool err      = false;
  auto start    = std::chrono::system_clock::now();
  for(std::size_t i = 0 ; i < jpeg_files.size() ; i ++){
    std::string cur_file_name = top_dir + std::string("/") + jpeg_files[i];
    FILE*       jpegFile      = fopen(cur_file_name.c_str(), "rb");
    int         iRes          = 0 ;
    if(NULL == jpegFile){
      printf("cannot open jpeg file\n");
      err = true;
    }else{
      fseek(jpegFile, 0, SEEK_END) ;
      int iFileSize = ftell(jpegFile) ;
      fseek(jpegFile, 0, SEEK_SET) ;
      unsigned char*      pEncBuf        = new unsigned char[iFileSize] ;
      int                 iEncSize       = iFileSize ;
      fread(pEncBuf, iEncSize, 1, jpegFile);
      fflush(jpegFile);
      fclose(jpegFile);

      av_init_packet(&avPkt);
      avPkt.data = pEncBuf ;
      avPkt.size = iEncSize ;
      iRes = avcodec_send_packet(pCodecCtxForDecode, &avPkt) ;
      if (iRes < 0){
        printf("avcodec_send_packet error: %d\n", iRes);
        err = true;
      }

      if (iRes >= 0)
      {
        iRes = avcodec_receive_frame(pCodecCtxForDecode, pFrameYUV) ;
        if (iRes == AVERROR(EAGAIN) || iRes == AVERROR_EOF){
          printf("avcodec_receive_frame error: %d\n", iRes);
          err = true;
        }
      }
      av_packet_unref(&avPkt) ;
      delete[] pEncBuf;
    }

    printf("decode %llu, %s, %d, %d, ", i, cur_file_name.c_str(), iRes, (int)err);
    if(false == err){
      printf("(%d, %d, %s)\n", pFrameYUV->width, pFrameYUV->height, av_get_pix_fmt_name(pCodecCtxForDecode->pix_fmt));
    }else{
      printf("\n");
    }
  }
  auto end      = std::chrono::system_clock::now();
  auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
  std::cout << "run cost(us): " << duration << std::endl;

  if(true == err)
    goto DECODE_IMAGE_LEAVE ;

  pDecodeImg = CreateRGBFrameFromAVFrame(pFrameYUV, pCodecCtxForDecode->pix_fmt, 4) ;
  if(KY_COLOR_SPACE_BGR24 == pDecodeImg->usColorSpace ||
     KY_COLOR_SPACE_RGB24 == pDecodeImg->usColorSpace){
    iDepth = 24;
  }else if(KY_COLOR_SPACE_BGR32 == pDecodeImg->usColorSpace ||
           KY_COLOR_SPACE_RGB32 == pDecodeImg->usColorSpace){
    iDepth = 32;
  }

  SaveBMP("last_image.bmp", 
          pDecodeImg->pFrameData[0], 
          pDecodeImg->uFrameWidth,
          pDecodeImg->uFrameHeight,
          pDecodeImg->uFrameLineSize[0],
          iDepth, 
          0);

DECODE_IMAGE_LEAVE:
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