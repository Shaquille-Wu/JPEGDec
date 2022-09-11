#include <iostream>
#include <string.h>
#include <math.h>
#include <chrono>
#include <vector>
#include <getopt.h>
#include <sys/types.h>
#include <dirent.h>
#include <sys/stat.h>
#include <tuple>
#include <algorithm>
#include <omp.h>
#include <unistd.h>

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

typedef struct tag_AVCodecObj{
  AVCodec*              pCodecForDecode ;
  AVCodecContext*       pCodecCtxForDecode ;
  AVFrame*              pFrameYUV ;
  AVPacket              avPkt;
  unsigned char*        pEncBuf ;
  int                   encBufSize ;
  KY_VID_DECODE_FRAME*  pDecodeImg ;
}AVCodecObj, *PAVCodecObj;

int main(int argc, char** argv){

  printf("start travel image directory\n");
  std::string               top_dir    = "/localdata/cn-customer-engineering/datasets/imagenet1k/train";
  std::vector<std::string>  sub_dirs = {

    "n01689811",  "n01873310",  "n02088364",  "n02102973",  "n02120505", 
    "n02389026",  "n02666196",  "n02883205",  "n03125729",  "n03445924", 
    "n03697007",  "n03873416",  "n04039381",  "n04265275",  "n04482393", 
    "n07615774",  "n12267677", 
    "n01692333",  "n01877812",  "n02088466",  "n02104029",  "n02123045", 
    "n02391049",  "n02667093",  "n02892201",  "n03126707",  "n03447447", 
    "n03706229",  "n03874293",  "n04040759",  "n04266014",  "n04483307", 
    "n07684084",  "n12620546", 
    "n01693334",  "n01882714",  "n02088632",  "n02104365",  "n02123159", 
    "n02395406",  "n02669723",  "n02892767",  "n03127747",  "n03447721", 
    "n03709823",  "n03874599",  "n04041544",  "n04270147",  "n04485082", 
    "n07693725",  "n12768682", 
    "n01694178",  "n01883070",  "n02089078",  "n02105056",  "n02123394", 
    "n02396427",  "n02672831",  "n02894605",  "n03127925",  "n03450230", 
    "n03710193",  "n03876231",  "n04044716",  "n04273569",  "n04486054", 
    "n07695742",  "n12985857", 
    "n01695060",  "n01910747",  "n02089867",  "n02105162",  "n02123597", 
    "n02397096",  "n02676566",  "n02895154",  "n03131574",  "n03452741", 
    "n03710637",  "n03877472",  "n04049303",  "n04275548",  "n04487081", 
    "n07697313",  "n12998815", 
    "n01697457",  "n01914609",  "n02089973",  "n02105251",  "n02124075", 
    "n02398521",  "n02687172",  "n02906734",  "n03133878",  "n03457902", 
    "n03710721",  "n03877845",  "n04065272",  "n04277352",  "n04487394", 
    "n07697537",  "n13037406", 
    "n01729322",  "n01944390",  "n02091134",  "n02106030",  "n02128925",  
    "n02415577",  "n02704792",  "n02927161",  "n03179701",  "n03476991",  
    "n03729826",  "n03891251",  "n04081281",  "n04311004",  "n04509417",  
    "n07716358",  "n13133613",  "n01729977",  "n01945685",  "n02091244",  
    "n02106166",  "n02129165",  "n02417914",  "n02708093",  "n02930766",  
    "n03180011",  "n03478589",  "n03733131",  "n03891332",  "n04086273",  
    "n04311174",  "n04515003",  "n07716906",  "n15075141",  "n01734418",  
    "n01950731",  "n02091467",  "n02106382",  "n02129604",  "n02422106",  
    "n02727426",  "n02939185",  "n03187595",  "n03481172",  "n03733281",  
    "n03895866",  "n04090263",  "n04317175",  "n04517823",  "n07717410",  
    "n01735189",  "n01955084",  "n02091635",  "n02106550",  "n02130308",  
    "n02422699",  "n02730930",  "n02948072",  "n03188531",  "n03482405",  
    "n03733805",  "n03899768",  "n04099969",  "n04325704",  "n04522168",  
    "n07717556",  "n01737021",  "n01968897",  "n02091831",  "n02106662",  
    "n02132136",  "n02423022",  "n02747177",  "n02950826",  "n03196217",  
    "n03483316",  "n03742115",  "n03902125",  "n04111531",  "n04326547",  
    "n04523525",  "n07718472",
  };
  std::size_t file_cnt = 0;
  std::vector<std::vector<std::string>>  all_files(sub_dirs.size());
  for(std::size_t i = 0 ; i < sub_dirs.size() ; i ++){
    all_files[i] = travel_image_dir(top_dir + std::string("/") + sub_dirs[i],
                                    std::vector<std::string>({std::string(".jpg"), std::string(".jpeg")}), 
                                    std::string(""));
    file_cnt += all_files[i].size();
  }
  std::vector<std::string>  jpeg_files(file_cnt);
  size_t                    ofs = 0;
  for(std::size_t i = 0 ; i < sub_dirs.size() ; i ++){
    for(std::size_t j = 0 ; j < all_files[i].size() ; j ++){
      jpeg_files[ofs + j] = top_dir + std::string("/") + sub_dirs[i] + std::string("/") + all_files[i][j];
    }
    ofs += all_files[i].size();
  }
  printf("total image file: %lu\n", jpeg_files.size());

  const int                 kThdCnt      = 256;
  std::vector<std::vector<std::string>>  jpeg_files_per_thd(kThdCnt);
  std::vector<std::tuple<unsigned char*, std::size_t, std::vector<int> > >  jpeg_buf_per_thd(kThdCnt);
  std::size_t               remain_files = jpeg_files.size();
  ofs = 0;
  while(remain_files > 0){
    std::size_t cur_files = remain_files > kThdCnt ? kThdCnt : remain_files;
    for(std::size_t i = 0 ; i < cur_files ; i ++){
      jpeg_files_per_thd[i].push_back(jpeg_files[ofs + i]);
    }
    remain_files -= cur_files;
    ofs          += cur_files;
  }
  auto   read_file_start       = std::chrono::system_clock::now();
  for(std::size_t t = 0 ; t < kThdCnt ; t ++){
    std::get<0>(jpeg_buf_per_thd[t]) = NULL;
    for(std::size_t i = 0 ; i < jpeg_files_per_thd[t].size() ; i ++){
      std::string cur_file_name = jpeg_files_per_thd[t][i];
      FILE*       jpegFile      = fopen(cur_file_name.c_str(), "rb");
      int         iRes          = 0 ;
      int         objIdx        = t;
      bool        err           = false;
      if(NULL == jpegFile){
        printf("cannot open jpeg file\n");
        err = true;
      }else{
        fseek(jpegFile, 0, SEEK_END) ;
        int iFileSize = ftell(jpegFile) ;
        fseek(jpegFile, 0, SEEK_SET) ;
        fflush(jpegFile);
        fclose(jpegFile);
        std::get<1>(jpeg_buf_per_thd[t]) += iFileSize;
      }
    }
  }

  for(std::size_t t = 0 ; t < kThdCnt ; t ++){
    std::get<0>(jpeg_buf_per_thd[t]) = new unsigned char[std::get<1>(jpeg_buf_per_thd[t])];
    std::get<2>(jpeg_buf_per_thd[t]).resize(jpeg_files_per_thd[t].size());
    unsigned char*   cur_buf = std::get<0>(jpeg_buf_per_thd[t]);
    for(std::size_t i = 0 ; i < jpeg_files_per_thd[t].size() ; i ++){
      std::string cur_file_name = jpeg_files_per_thd[t][i];
      FILE*       jpegFile      = fopen(cur_file_name.c_str(), "rb");
      bool        err           = false;
      if(NULL == jpegFile){
        printf("cannot open jpeg file\n");
        err = true;
      }else{
        fseek(jpegFile, 0, SEEK_END) ;
        int iFileSize = ftell(jpegFile) ;
        fseek(jpegFile, 0, SEEK_SET) ;
        
        fread(cur_buf, iFileSize, 1, jpegFile);
        fflush(jpegFile);
        fclose(jpegFile);
        cur_buf += iFileSize;
        (std::get<2>(jpeg_buf_per_thd[t]))[i] = iFileSize;
      }
    }
  }
  auto read_file_end      = std::chrono::system_clock::now();
  auto read_file_duration = std::chrono::duration_cast<std::chrono::microseconds>(read_file_end - read_file_start).count();
  std::cout << "read file cost(us): " << read_file_duration << std::endl;

  av_log_set_level(AV_LOG_PANIC);
  AVCodecObj*       codecObjs        = new AVCodecObj[kThdCnt];
  memset(codecObjs, 0, kThdCnt * sizeof(AVCodecObj));
  for(int i = 0 ; i < kThdCnt ; i ++){
    codecObjs[i].pCodecForDecode = (AVCodec*)avcodec_find_decoder(AV_CODEC_ID_MJPEG) ;

    if (NULL == codecObjs[i].pCodecForDecode){
      printf("cannot find AV_CODEC_ID_MJPEG\n");
      return 0;
    }

    printf("found AV_CODEC_ID_MJPEG\n");

    codecObjs[i].pCodecCtxForDecode = avcodec_alloc_context3(codecObjs[i].pCodecForDecode) ;

    codecObjs[i].pCodecCtxForDecode->codec_id    = AV_CODEC_ID_MJPEG ;
    codecObjs[i].pCodecCtxForDecode->codec_type  = AVMEDIA_TYPE_VIDEO ;

    if (codecObjs[i].pCodecForDecode->capabilities & AV_CODEC_CAP_TRUNCATED)
      codecObjs[i].pCodecCtxForDecode->flags |= AV_CODEC_FLAG_TRUNCATED;

    if (avcodec_open2(codecObjs[i].pCodecCtxForDecode, codecObjs[i].pCodecForDecode, NULL) < 0)
    {
      av_free(codecObjs[i].pCodecCtxForDecode) ;
      codecObjs[i].pCodecCtxForDecode = NULL ;
      codecObjs[i].pCodecForDecode    = NULL ;
      printf("cannot find open avcodec\n");
      return 0 ;
    }
    printf("open avcodec\n");
    codecObjs[i].pFrameYUV = av_frame_alloc() ;
  }

  int    iDeCodedLen     = 0;
  int    iImgFinished    = 0;
  int    iDepth          = 8;
  int    omp_thd_cnt     = omp_get_num_threads();
  size_t total_file_size = 0;
  printf("there are %d thread in system, and target thread cnt is %d\n", omp_thd_cnt, kThdCnt);
  omp_set_num_threads(kThdCnt);
  auto   dec_start       = std::chrono::system_clock::now();
  #pragma omp parallel for num_threads(kThdCnt)
  for(std::size_t t = 0 ; t < kThdCnt ; t ++){
    unsigned char* cur_buf = std::get<0>(jpeg_buf_per_thd[t]);
    for(std::size_t i = 0 ; i < jpeg_files_per_thd[t].size() ; i ++){
      int   iRes       = 0 ;
      int   objIdx     = t;
      bool  err        = false;
      int   iFileSize  = (std::get<2>(jpeg_buf_per_thd[t]))[i];
      std::string cur_file_name = jpeg_files_per_thd[t][i];
      if(NULL != codecObjs[objIdx].pEncBuf){
        if(codecObjs[objIdx].encBufSize < iFileSize){
          delete[] codecObjs[objIdx].pEncBuf;
          codecObjs[objIdx].pEncBuf     = new unsigned char[iFileSize] ;
          codecObjs[objIdx].encBufSize  = iFileSize ;
        }
      }else{
        codecObjs[objIdx].pEncBuf     = new unsigned char[iFileSize] ;
        codecObjs[objIdx].encBufSize  = iFileSize ;
      }
      memcpy(codecObjs[objIdx].pEncBuf, cur_buf, iFileSize);
      cur_buf += iFileSize;

      av_init_packet(&(codecObjs[objIdx].avPkt));
      codecObjs[objIdx].avPkt.data = codecObjs[objIdx].pEncBuf ;
      codecObjs[objIdx].avPkt.size = iFileSize ;
      iRes = avcodec_send_packet(codecObjs[objIdx].pCodecCtxForDecode, &(codecObjs[objIdx].avPkt)) ;
      if (iRes < 0){
        printf("avcodec_send_packet error: %d\n", iRes);
        err = true;
      }

      if (iRes >= 0)
      {
        iRes = avcodec_receive_frame(codecObjs[objIdx].pCodecCtxForDecode, codecObjs[objIdx].pFrameYUV) ;
        if (iRes == AVERROR(EAGAIN) || iRes == AVERROR_EOF){
          printf("avcodec_receive_frame error: %d\n", iRes);
          err = true;
        }
      }
      av_packet_unref(&(codecObjs[objIdx].avPkt)) ;
      total_file_size += iFileSize;

      if(true == err){
        printf("decode %llu, %s, %d, %d, error!\n", i, cur_file_name.c_str(), iRes, (int)err);
      }
    }
  }
  auto dec_end      = std::chrono::system_clock::now();
  auto dec_duration = std::chrono::duration_cast<std::chrono::microseconds>(dec_end - dec_start).count();
  std::cout << "total_file_size: " << total_file_size << ", run cost(us): " << dec_duration << std::endl;

  for(int i = 0 ; i < kThdCnt ; i ++){
    if (0 != codecObjs[i].pCodecCtxForDecode)
    {
      avcodec_flush_buffers(codecObjs[i].pCodecCtxForDecode) ;
      avcodec_close(codecObjs[i].pCodecCtxForDecode) ;
      av_free(codecObjs[i].pCodecCtxForDecode) ;
      codecObjs[i].pCodecCtxForDecode  = 0 ;
      codecObjs[i].pCodecForDecode     = 0 ;
    }

    // Free the YUV frame
    if (0 != codecObjs[i].pFrameYUV)
    {
      av_frame_free(&(codecObjs[i].pFrameYUV)) ;
      codecObjs[i].pFrameYUV = 0 ;
    }

    if(NULL != codecObjs[i].pDecodeImg){
      if(codecObjs[i].pDecodeImg->pFrameData[0]){
        KyFreeMem(codecObjs[i].pDecodeImg->pFrameData[0]);
      }
      KyFreeMem(codecObjs[i].pDecodeImg);
    }
  }
  delete[] codecObjs;
  return 0 ;
}