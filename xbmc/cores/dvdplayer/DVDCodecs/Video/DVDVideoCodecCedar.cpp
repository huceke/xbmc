/*
 *      Copyright (C) 2012 Team XBMC
 *      http://www.xbmc.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#if (defined HAVE_CONFIG_H) && (!defined WIN32)
  #include "config.h"
#elif defined(_WIN32)
#include "system.h"
#endif

#if defined(HAVE_LIBCEDAR)

#include "DVDStreamInfo.h"
#include "DVDCodecs/DVDCodecUtils.h"
#include "DVDVideoCodecCedar.h"
#include "DynamicDll.h"
#include "cores/VideoRenderers/RenderFormats.h"
#include "utils/BitstreamConverter.h"

#include "guilib/GraphicContext.h"

#include "utils/log.h"
#include "linux/XMemUtils.h"
#include "DVDClock.h"
#include "threads/Atomics.h"

#include <sys/time.h>
#include <inttypes.h>

#ifdef CLASSNAME
#undef CLASSNAME
#endif
#define CLASSNAME "CDVDVideoCodecCedar"

#define ALIGN(x, n) (((x) + (n) - 1) & (~((n) - 1)))
#define CODEC_4CC(c1,c2,c3,c4) (((u32)(c4)<<24)|((u32)(c3)<<16)|((u32)(c2)<<8)|(u32)(c1))

static long g_cedaropen = 0;

CDVDVideoCodecCedar::CDVDVideoCodecCedar()
{
  m_video_codec_name  = "";
  m_cedarDecoder      = NULL;
  m_valid_pts         = false;
  m_packet_count      = 0;
  m_converter         = NULL;
  m_convert           = false;
  m_is_open           = false;

  memset(&m_videobuffer, 0, sizeof(DVDVideoPicture));
}

CDVDVideoCodecCedar::~CDVDVideoCodecCedar()
{
  Dispose();
}

bool CDVDVideoCodecCedar::NaluFormatStartCodes(enum CodecID codec, uint8_t *in_extradata, int in_extrasize)
{
  switch(codec)
  {
    case CODEC_ID_H264:
      if (in_extrasize < 7 || in_extradata == NULL)
        return true;
      // valid avcC atom data always starts with the value 1 (version), otherwise annexb
      else if ( *in_extradata != 1 )
        return true;
    default: break;
  }
  return false;
}

bool CDVDVideoCodecCedar::Open(CDVDStreamInfo &hints, CDVDCodecOptions &options)
{
  if (cas(&g_cedaropen, 0, 1) != 0)
  {
    CLog::Log(LOGERROR, "%s::%s decoder already in use\n", CLASSNAME, __func__);
    return false;
  }

  if(!m_dllCedar.Load())
    return false;

  Dispose();

  m_decoded_width   = hints.width;
  m_decoded_height  = hints.height;

  if(!m_decoded_width || !m_decoded_height)
  {
    CLog::Log(LOGERROR, "%s::%s unknown decoded size %dx%d\n", CLASSNAME, __func__, m_decoded_width, m_decoded_height);
    return false;
  }

  m_converter = new CBitstreamConverter();
  if(!m_converter)
  {
    CLog::Log(LOGERROR, "%s::%s allocationg bistream converter\n", CLASSNAME, __func__);
    return false;
  }

  int extrasize = hints.extrasize;
  uint8_t *extradata = (uint8_t *)hints.extradata;

  m_convert = m_converter->Open(hints.codec, (uint8_t *)hints.extradata, hints.extrasize, false);
  if(m_convert)
  {
    extrasize = m_converter->GetExtraSize();
    extradata = m_converter->GetExtraData();
  }

  vstream_info_t    stream_info;
  memset(&stream_info, 0x0, sizeof(vstream_info_t));

  stream_info.container_format  = CONTAINER_FORMAT_UNKNOW;

  /*
  printf("extradata : ");
  for(int i = 0; i < extrasize; i++)
  {
    uint8_t *p = (uint8_t *)extradata;
    printf("%02x ", p[i]);
  }
  printf("\n");
  */

  switch(hints.codec)
  {
    case CODEC_ID_H264:
      stream_info.format              = STREAM_FORMAT_H264;
      stream_info.sub_format          = STREAM_SUB_FORMAT_UNKNOW;
      /*
      if(hints.codec_tag == 27 || NaluFormatStartCodes(hints.codec, extradata, extrasize)) //M2TS and TS
      {
        printf("ts\n");
        stream_info.container_format  = CONTAINER_FORMAT_TS;
      }
      */
      m_video_codec_name              = "cedar-h264";
      break;
    case CODEC_ID_MPEG1VIDEO:
      stream_info.format              = STREAM_FORMAT_MPEG2;
      stream_info.sub_format          = MPEG2_SUB_FORMAT_MPEG1;
      m_video_codec_name              = "cedar-mpeg1";
      break;
    case CODEC_ID_MPEG2VIDEO:
      stream_info.format              = STREAM_FORMAT_MPEG2;
      stream_info.sub_format          = MPEG2_SUB_FORMAT_MPEG2;
      extradata = NULL;
      extrasize = 0;
      m_video_codec_name              = "cedar-mpeg2";
      break;
    case CODEC_ID_MSMPEG4V1:
      stream_info.format              = STREAM_FORMAT_MPEG4;
      stream_info.sub_format          = MPEG4_SUB_FORMAT_DIVX1;
      m_video_codec_name              = "cedar-divx1";
      break;
    break;
    case CODEC_ID_MSMPEG4V2:
      stream_info.format              = STREAM_FORMAT_MPEG4;
      stream_info.sub_format          = MPEG4_SUB_FORMAT_DIVX2;
      m_video_codec_name              = "cedar-divx2";
      break;
    case CODEC_ID_MSMPEG4V3:
      stream_info.format              = STREAM_FORMAT_MPEG4;
      stream_info.sub_format          = MPEG4_SUB_FORMAT_DIVX3;
      m_video_codec_name              = "cedar-divx3";
      break;
    case CODEC_ID_MPEG4:
      stream_info.format              = STREAM_FORMAT_MPEG4;
      switch(hints.codec_tag)
      {
        case CODEC_4CC('D','X','5','0'):
        case CODEC_4CC('D','I','V','5'):
          stream_info.sub_format      = MPEG4_SUB_FORMAT_DIVX5;
          m_video_codec_name          = "cedar-divx5";
          break;
        case CODEC_4CC('X','V','I','D'):
        case CODEC_4CC('M','P','4','V'):
        case CODEC_4CC('P','M','P','4'):
        case CODEC_4CC('F','M','P','4'):
          stream_info.sub_format      = MPEG4_SUB_FORMAT_XVID;
          m_video_codec_name          = "cedar-xvid";
          break;
        default:
          return false;
      }
      break;
    case CODEC_ID_VP6F:
      stream_info.format              = STREAM_FORMAT_MPEG4;
      stream_info.sub_format          = MPEG4_SUB_FORMAT_VP6;
      m_video_codec_name              = "cedar-vp6";
      break;
    case CODEC_ID_H263:
      stream_info.format              = STREAM_FORMAT_MPEG4;
      stream_info.sub_format          = MPEG4_SUB_FORMAT_H263;
      m_video_codec_name              = "cedar-h263";
      break;
    case CODEC_ID_VP8:
      stream_info.format              = STREAM_FORMAT_VP8;
      stream_info.sub_format          = STREAM_SUB_FORMAT_UNKNOW;
      m_video_codec_name              = "cedar-vp8";
      break;
    case CODEC_ID_VC1:
      stream_info.format              = STREAM_FORMAT_VC1;
      stream_info.sub_format          = STREAM_SUB_FORMAT_UNKNOW;
      m_video_codec_name              = "cedar-vc1";
      break;
    case CODEC_ID_WMV3:
      stream_info.format              = STREAM_FORMAT_VC1;
      stream_info.sub_format          = STREAM_SUB_FORMAT_UNKNOW;
      m_video_codec_name              = "cedar-wmv3";
      break;
    case CODEC_ID_WMV1:
      stream_info.format              = STREAM_FORMAT_MPEG4;
      stream_info.sub_format          = MPEG4_SUB_FORMAT_WMV1;
      m_video_codec_name              = "cedar-wmv1";
      break;
    case CODEC_ID_WMV2:
      stream_info.format              = STREAM_FORMAT_MPEG4;
      stream_info.sub_format          = MPEG4_SUB_FORMAT_WMV2;
      m_video_codec_name              = "cedar-wmv2";
      break;
    case CODEC_ID_FLV1:
      stream_info.format              = STREAM_FORMAT_MPEG4;
      stream_info.sub_format          = MPEG4_SUB_FORMAT_SORENSSON_H263;
      m_video_codec_name              = "cedar-sorensson";
      break;
    case CODEC_ID_MJPEG:
      stream_info.format              = STREAM_FORMAT_MJPEG;
      stream_info.sub_format          = STREAM_SUB_FORMAT_UNKNOW;
      m_video_codec_name              = "cedar-mjpeg";
      break;
    default:
      CLog::Log(LOGERROR, "%s::%s CodecID 0x%08x not supported by Cedar decoder\n", CLASSNAME, __func__, hints.codec);
      return false;
  }

  stream_info.container_format    = CONTAINER_FORMAT_UNKNOW;
  stream_info.video_width         = m_decoded_width;
  stream_info.video_height        = m_decoded_height;
  if (hints.fpsrate && hints.fpsscale)
    stream_info.frame_rate = DVD_TIME_BASE /
      CDVDCodecUtils::NormalizeFrameduration((double)DVD_TIME_BASE *
      hints.fpsscale / hints.fpsrate) * 1000;
  else
    stream_info.frame_rate = 25.0f * 1000;
  stream_info.frame_duration      = (double)DVD_TIME_BASE / stream_info.frame_rate * 1000.0f;
  stream_info.is_pts_correct      = 1;
  stream_info.aspec_ratio         = m_decoded_width / m_decoded_height * 1000;

  m_cedarDecoder = m_dllCedar.AllocCedarDecoder();

  if(!m_cedarDecoder)
  {
    CLog::Log(LOGERROR, "%s::%s Cedar create\n", CLASSNAME, __func__);
    return false;
  }

  if(!m_cedarDecoder->Open(&stream_info, extradata, extrasize))
  {
    CLog::Log(LOGERROR, "%s::%s Cedar decoder open\n", CLASSNAME, __func__);
    m_cedarDecoder->Close();
    return false;
  }

  m_is_open = true;

  CLog::Log(LOGINFO, "%s::%s - Cedar Decoder opened with codec : %s [%dx%d]", CLASSNAME, __func__,
            m_video_codec_name.c_str(), m_decoded_width, m_decoded_height);

  return true;
}

void CDVDVideoCodecCedar::Dispose()
{
  if(m_is_open)
    m_cedarDecoder->Close();

  if(m_cedarDecoder)
  {
    cas(&g_cedaropen, 1, 0);
    m_dllCedar.FreeCedarDecoder(m_cedarDecoder);
    m_cedarDecoder = NULL;
  }
  
  m_video_codec_name  = "";

  memset(&m_videobuffer, 0, sizeof(DVDVideoPicture));

  while (!m_dts_queue.empty())
    m_dts_queue.pop();

  m_valid_pts = false;

  m_packet_count = 0;

  delete m_converter;
  m_converter = NULL;
  m_convert = false;

  m_dllCedar.Unload();

  m_is_open = false;
}

void CDVDVideoCodecCedar::SetDropState(bool bDrop)
{
  m_drop_state = bDrop;

  /*
  if(m_drop_state)
  {
    while(m_cedarDecoder->ReadyFrames())
    {
      vpicture_t *picture = m_cedarDecoder->GetDisplayFrame();
      if(picture)
      {
        m_cedarDecoder->ReturnDisplayFrame(picture);
        if(!m_dts_queue.empty())
          m_dts_queue.pop();
      }
      else
        break;
    }
  }
  */
}

int CDVDVideoCodecCedar::Decode(uint8_t *pData, int iSize, double dts, double pts)
{
  if (!pData)
  {
    int ret = 0;
    if(m_cedarDecoder->ReadyFrames() == 1)
      ret = VC_PICTURE | VC_BUFFER;
    else if(m_cedarDecoder->ReadyFrames() > 1)
      ret = VC_PICTURE;
    else
      ret = VC_BUFFER;
    return ret;
  }

  int demuxer_bytes = 0;
  uint8_t *demuxer_content = NULL;

  if(m_convert)
  {
    m_converter->Convert(pData, iSize);
    demuxer_bytes = m_converter->GetConvertSize();
    demuxer_content = m_converter->GetConvertBuffer();
  }
  else
  {
    demuxer_bytes = iSize;
    demuxer_content = pData;
  }

  if(demuxer_bytes && demuxer_content)
  {
    double pts_cedar = (pts == DVD_NOPTS_VALUE) ? 0 : pts / 1000;

    m_valid_pts = (pts != DVD_NOPTS_VALUE);

    vstream_data_t *streamData = m_cedarDecoder->AllocateBuffer(demuxer_bytes, pts_cedar);

    if(streamData)
    {
      m_cedarDecoder->AddBufferData(streamData, demuxer_content, demuxer_bytes);
      m_cedarDecoder->AddBuffer(streamData);
  
      m_dts_queue.push(dts);

      m_packet_count++;
    }
    else
    {
      CLog::Log(LOGERROR, "%s::%s decoder buffer full. frame dropped\n" , CLASSNAME, __func__);
    }
  }

  int ret = 0;

  if(m_cedarDecoder->FreeFrames() > 1 && m_cedarDecoder->ReadyBuffers() && m_packet_count > 1)
  {
    //unsigned int start = XbmcThreads::SystemClockMillis();
    s32 decoderRet = m_cedarDecoder->Decode(false, pts);
    //int lastDecodeTime = XbmcThreads::SystemClockMillis() - start;
    if(decoderRet == VRESULT_ERR_NO_MEMORY || decoderRet == VRESULT_ERR_UNSUPPORTED)
    {
      CLog::Log(LOGERROR, "%s::%s bitstream unsupported\n", CLASSNAME, __func__);
      return VC_ERROR;
    }
    //if(lastDecodeTime > 40)
    //  printf("decode tooked %3d ms\n", lastDecodeTime);
    
    switch(decoderRet)
    {
      case VRESULT_OK:
      case VRESULT_FRAME_DECODED:
      case VRESULT_KEYFRAME_DECODED:
      case VRESULT_NO_BITSTREAM:
        ret |= VC_BUFFER;
        break;
      default:
        break;
    }
  }

  if(m_cedarDecoder->ReadyFrames())
    ret |= VC_PICTURE;

  return ret;
}

bool CDVDVideoCodecCedar::GetPicture(DVDVideoPicture *pDvdVideoPicture)
{
  // clone the video picture buffer settings.
  bool bRet = false;

  vpicture_t *picture = m_cedarDecoder->GetDisplayFrame();
  if(picture)
  {
    m_cedarPicture.Picture  = picture;
    m_videobuffer.cedarPicture = &m_cedarPicture;
  
    /* set ref on the frame */

    m_videobuffer.format          = RENDER_FMT_BYPASS_CEDAR;
    m_videobuffer.pts             = DVD_NOPTS_VALUE;
    m_videobuffer.dts             = DVD_NOPTS_VALUE;

    if(m_valid_pts)
      m_videobuffer.pts           = m_cedarPicture.Picture->pts * 1000;

    if (!m_dts_queue.empty())
    {
      m_videobuffer.dts = m_dts_queue.front();
      m_dts_queue.pop();
    }

    //printf("pts %f dts %f\n", m_videobuffer.pts, m_videobuffer.dts);

    m_videobuffer.iDisplayWidth   = m_decoded_width;
    m_videobuffer.iDisplayHeight  = m_decoded_height;
    m_videobuffer.iWidth          = m_cedarPicture.Picture->width;
    m_videobuffer.iHeight         = m_cedarPicture.Picture->height;

    m_videobuffer.data[0]         = m_cedarPicture.Picture->y;
    m_videobuffer.data[1]         = m_cedarPicture.Picture->u;
    m_videobuffer.data[2]         = m_cedarPicture.Picture->u;
    m_videobuffer.iLineSize[0]    = m_cedarPicture.Picture->width;
    m_videobuffer.iLineSize[1]    = m_cedarPicture.Picture->height / 2;
    m_videobuffer.iLineSize[2]    = m_cedarPicture.Picture->height / 2;

    m_videobuffer.iFlags          = DVP_FLAG_ALLOCATED;
    /*
    m_videobuffer.iFlags          |= m_drop_state ? DVP_FLAG_DROPPED : 0;
    */

    bRet = true;
  }
  else
  {
    m_videobuffer.cedarPicture    = NULL;
    m_videobuffer.iFlags          = DVP_FLAG_DROPPED;
  }

  *pDvdVideoPicture = m_videobuffer;

  return bRet;
}

bool CDVDVideoCodecCedar::ClearPicture(DVDVideoPicture* pDvdVideoPicture)
{
  if(pDvdVideoPicture->cedarPicture != NULL)
  {
    m_cedarDecoder->ReturnDisplayFrame(pDvdVideoPicture->cedarPicture->Picture);
  }
  pDvdVideoPicture->cedarPicture = NULL;

  return CDVDVideoCodec::ClearPicture(pDvdVideoPicture);
}

void CDVDVideoCodecCedar::Reset(void)
{
  if(m_cedarDecoder)
    m_cedarDecoder->Flush();
}

#endif
