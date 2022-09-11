#ifndef __KY_DATA_DEF_H__
#define __KY_DATA_DEF_H__

#define KY_FRAME_BUF_NUM_MAX        (4)
#define KY_CAPSRC_RESERVED_LEN      (8)

typedef struct tag_ky_rtc_info{
  unsigned short int   usYear ;
  unsigned short int   usMonth ;
  unsigned short int   usMonthDay ;
  unsigned short int   usWeekDay ;
  unsigned short int   usHour ;
  unsigned short int   usMinute ;
  unsigned short int   usSecond ;
  unsigned int         uReserved[4] ;
}KY_RTC_INFO, *PKY_RTC_INFO ;

typedef struct tag_ky_buf_info{
  unsigned char         *pBuf ;
  unsigned long long     ullDataLen ;
  unsigned long long     ullBufLen ;
  unsigned int           uReserved[4] ;
}KY_BUF_INFO, *PKY_BUF_INFO ;

typedef enum tag_ky_color_space{
  KY_COLOR_SPACE_GRAY,
  KY_COLOR_SPACE_BGR24,//from low byte to high byte: B G R
  KY_COLOR_SPACE_RGB24, //from low byte to high byte: R G B
  KY_COLOR_SPACE_BGR32, //from low byte to high byte: B G R Alpha
  KY_COLOR_SPACE_RGB32, //from low byte to high byte: R G B Alpha
  KY_COLOR_SPACE_YUYV, //from low byte to high byte: Y U Y V
  KY_COLOR_SPACE_UYVY, //from low byte to high byte: U Y V Y
  KY_COLOR_SPACE_VYUY, //from low byte to high byte: V Y U Y
  KY_COLOR_SPACE_YUV422P,
  KY_COLOR_SPACE_YUV444P,
  KY_COLOR_SPACE_YUV422SP,
  KY_COLOR_SPACE_YUV420P,
  KY_COLOR_SPACE_YUV420SP,
  KY_COLOR_SPACE_BAYER_GBGR,
  KY_COLOR_SPACE_BAYER_GRGB,
  KY_COLOR_SPACE_BAYER_RGBG,
  KY_COLOR_SPACE_BAYER_BGRG,
  KY_COLOR_SPACE_RGB565,
  KY_COLOR_SPACE_UNKNOWN
}KY_COLOR_SPACE;

typedef struct tag_ky_vid_decode_frame{
  unsigned char*       pFrameBuf ;
  unsigned short int   usColorSpace ;
  unsigned short int   usFrameDir ;   //first byte is frame-mirror, second byte is isNeedRotateForPresentation
  unsigned int         uFrameBufLen ;
  unsigned int         uFrameSize ;
  unsigned int         uFrameWidth ;
  unsigned int         uFrameHeight ;
  unsigned char*       pFrameData[KY_FRAME_BUF_NUM_MAX] ;
  unsigned int         uFrameLineSize[KY_FRAME_BUF_NUM_MAX] ;
  KY_RTC_INFO          stFrameTime ;
  unsigned int         uReserved[KY_CAPSRC_RESERVED_LEN] ;
}KY_VID_DECODE_FRAME, *PKY_VID_DECODE_FRAME ;

#ifndef KY_LINESIZE_ALIGNED
#define KY_LINESIZE_ALIGNED(iLineSize, iWidth, iDepth, iAligned)         \
iLineSize = ((iWidth) * ((iDepth) >> 3)) ;                               \
if(0 != (iLineSize & ((iAligned)- 1)))                                   \
	iLineSize = iLineSize - (iLineSize & ((iAligned)- 1)) + (iAligned) ;
#endif

#ifndef KY_LINESIZE_ALIGNED4
#define KY_LINESIZE_ALIGNED4(iLineSize, iWidth, iDepth)                KY_LINESIZE_ALIGNED(iLineSize, iWidth, iDepth, 4)
#endif

#ifndef KY_LINESIZE_ALIGNED8
#define KY_LINESIZE_ALIGNED8(iLineSize, iWidth, iDepth)                KY_LINESIZE_ALIGNED(iLineSize, iWidth, iDepth, 8)
#endif

#ifndef KY_LINESIZE_ALIGNED16
#define KY_LINESIZE_ALIGNED16(iLineSize, iWidth, iDepth)               KY_LINESIZE_ALIGNED(iLineSize, iWidth, iDepth, 16)
#endif

#ifndef KY_LINESIZE_ALIGNED32
#define KY_LINESIZE_ALIGNED32(iLineSize, iWidth, iDepth)               KY_LINESIZE_ALIGNED(iLineSize, iWidth, iDepth, 32)
#endif

#ifndef KY_GET_NUM_ALIGNED4
#define KY_GET_NUM_ALIGNED4(dst, src)                 \
dst = (src) ;                                         \
if(0 != ((src) & 3))                                  \
	dst = (src) - ((src) & 3) + 4 ;
#endif

#ifndef KY_GET_NUM_ALIGNED8
#define KY_GET_NUM_ALIGNED8(dst, src)                \
dst = (src) ;                                        \
if(0 != ((src) & 7))                                 \
	dst = (src) - ((src) & 7) + 8 ;
#endif

#ifndef KY_GET_NUM_ALIGNED16
#define KY_GET_NUM_ALIGNED16(dst, src)               \
dst = (src) ;                                        \
if(0 != ((src) & 15))                                \
	dst = (src) - ((src) & 15) + 16 ;
#endif

#ifndef KY_GET_NUM_ALIGNED32
#define KY_GET_NUM_ALIGNED32(dst, src)              \
dst = (src) ;                                       \
if(0 != ((src) & 31))                               \
	dst = (src) - ((src) & 31) + 32 ;
#endif

#endif