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

#include "guilib/GraphicContext.h"

#include "utils/log.h"
#include "linux/XMemUtils.h"
#include "DVDClock.h"

#include <sys/time.h>
#include <inttypes.h>

#include "utils/fastmemcpy.h"

#ifdef CLASSNAME
#undef CLASSNAME
#endif
#define CLASSNAME "CDVDVideoCodecCedar"

#define ALIGN(x, n) (((x) + (n) - 1) & (~((n) - 1)))
#define CODEC_4CC(c1,c2,c3,c4) (((u32)(c4)<<24)|((u32)(c3)<<16)|((u32)(c2)<<8)|(u32)(c1))

CDVDVideoCodecCedar::CDVDVideoCodecCedar()
{
  m_is_open           = false;
  m_extradata         = NULL;
  m_extrasize         = 0;
  m_video_codec_name  = "";
  m_Frames            = 0;
  m_cedarDecoder      = NULL;
  m_lastDecodeTime    = 0;
  m_valid_pts         = false;
  m_packet_count      = 0;

  memset(&m_videobuffer, 0, sizeof(DVDVideoPicture));
}

CDVDVideoCodecCedar::~CDVDVideoCodecCedar()
{
  if (m_is_open)
    Dispose();
}

bool CDVDVideoCodecCedar::Open(CDVDStreamInfo &hints, CDVDCodecOptions &options)
{
  if(!m_dllCedar.Load())
    return false;

  m_decoded_width   = hints.width;
  m_decoded_height  = hints.height;
  m_picture_width   = m_decoded_width;
  m_picture_height  = m_decoded_height;

  if(!m_decoded_width || !m_decoded_height)
  {
    CLog::Log(LOGDEBUG, "%s::%s unknown decoded size %dx%d\n", CLASSNAME, __func__, m_decoded_width, m_decoded_height);
    return false;
  }

  vstream_info_t    stream_info;
  memset(&stream_info, 0x0, sizeof(vstream_info_t));

  stream_info.container_format  = CONTAINER_FORMAT_UNKNOW;

  switch(hints.codec)
  {
    case CODEC_ID_H264:
      stream_info.format              = STREAM_FORMAT_H264;
      stream_info.sub_format          = STREAM_SUB_FORMAT_UNKNOW;
      if(hints.codec_tag == 27)       //M2TS and TS
        stream_info.container_format  = CONTAINER_FORMAT_TS;
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

  if(hints.extrasize > 0 && hints.extradata != NULL)
  {
    m_extrasize = hints.extrasize;
    m_extradata = (uint8_t *)malloc(m_extrasize);
    fast_memcpy(m_extradata, hints.extradata, hints.extrasize);
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

  if(!m_cedarDecoder->Open(&stream_info, m_extradata, m_extrasize))
  {
    CLog::Log(LOGERROR, "%s::%s Cedar decoder open\n", CLASSNAME, __func__);
    m_cedarDecoder->Close();
    return false;
  }

  m_Frames        = 0;
  m_is_open       = true;

  CLog::Log(LOGINFO, "%s::%s - Cedar Decoder opened with codec : %s [%dx%d]", CLASSNAME, __func__,
            m_video_codec_name.c_str(), m_decoded_width, m_decoded_height);

  return true;
}

void CDVDVideoCodecCedar::Dispose()
{
  ClearPackages();

  if(m_is_open && m_cedarDecoder)
    m_cedarDecoder->Close();

  if(m_cedarDecoder)
    m_cedarDecoder = m_dllCedar.FreeCedarDecoder(m_cedarDecoder);
  
  m_is_open       = false;

  if(m_extradata)
    free(m_extradata);
  m_extradata = NULL;
  m_extrasize = 0;

  m_video_codec_name  = "";

  m_lastDecodeTime    = 0;

  memset(&m_videobuffer, 0, sizeof(DVDVideoPicture));

  while (!m_dts_queue.empty())
    m_dts_queue.pop();

  m_valid_pts = false;

  m_packet_count = 0;

  m_dllCedar.Unload();
}

void CDVDVideoCodecCedar::SetDropState(bool bDrop)
{
  if(m_drop_state != bDrop)
    m_drop_state = bDrop;

  if(m_drop_state)
  {
    while(m_cedarDecoder->ReadyFrames() > 1)
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

  unsigned int demuxer_bytes = (unsigned int)iSize;
  uint8_t *demuxer_content = pData;

  if(demuxer_bytes && demuxer_content)
  {
    CCedarPackage *cedarPackage = new CCedarPackage(demuxer_content, demuxer_bytes, dts, pts, m_drop_state);
    m_cedarPackages.push_back(cedarPackage);
  }

  if(!m_cedarPackages.empty())
  {
    CCedarPackage *cedarPackage = m_cedarPackages.front();

    if(cedarPackage)
    {
      double pts = (cedarPackage->pts == DVD_NOPTS_VALUE) ? 0 : cedarPackage->pts / 1000;

      m_valid_pts = (cedarPackage->pts != DVD_NOPTS_VALUE);

      vstream_data_t *streamData = m_cedarDecoder->AllocateBuffer(cedarPackage->Size, pts);

      if(streamData)
      {
        m_cedarDecoder->AddBufferData(streamData, cedarPackage->Data, cedarPackage->Size);
        m_cedarDecoder->AddBuffer(streamData);
  
        m_cedarPackages.pop_front();

        m_dts_queue.push(cedarPackage->dts);

        m_packet_count++;

        delete cedarPackage;

      }
    }
  }

  int ret = 0;

  if(m_cedarDecoder->FreeFrames() > 1 && m_cedarDecoder->ReadyBuffers() && m_packet_count > 1)
  {
    unsigned int start = XbmcThreads::SystemClockMillis();
    s32 decoderRet = m_cedarDecoder->Decode(false, pts);
    m_lastDecodeTime = XbmcThreads::SystemClockMillis() - start;
    if(decoderRet == VRESULT_ERR_NO_MEMORY || decoderRet == VRESULT_ERR_UNSUPPORTED)
    {
      CLog::Log(LOGERROR, "%s::%s bitstream unsupported\n", CLASSNAME, __func__);
      return VC_ERROR;
    }
    
    switch(decoderRet)
    {
      case VRESULT_OK:
      case VRESULT_FRAME_DECODED:
      case VRESULT_KEYFRAME_DECODED:
      case VRESULT_NO_BITSTREAM:
        ret |= VC_BUFFER;
        break;
      defaul:
        break;
    }

    if(m_lastDecodeTime > 40)
      printf("decode tooked %3d ms\n", m_lastDecodeTime);
  }

  //printf("Ready %3d Free %3d Buffers %3d\n", m_cedarDecoder->ReadyFrames(), 
  //    m_cedarDecoder->FreeFrames(), m_cedarDecoder->ReadyBuffers());

  if(m_cedarDecoder->ReadyFrames() > 1)
    ret |= VC_PICTURE;

  return ret;
}

bool CDVDVideoCodecCedar::GetPicture(DVDVideoPicture *pDvdVideoPicture)
{
  // clone the video picture buffer settings.
  bool bRet = false;

  CCedarPicture *cedarPicture = NULL;

  //printf("ReadyFrames %2d FreeFrames %2d MaxFrames %2d\n", 
  //    m_cedarDecoder->ReadyFrames(), m_cedarDecoder->FreeFrames(), m_cedarDecoder->MaxFrames());

  vpicture_t *picture = m_cedarDecoder->GetDisplayFrame();
  if(picture)
  {
    cedarPicture = new CCedarPicture(picture, (unsigned int)picture->y,
          (unsigned int)picture->u, (unsigned int)picture->v,
          (unsigned int)picture->size_y, (unsigned int)picture->size_u,
          (unsigned int)picture->size_v);
  }

  if(cedarPicture)
  {
    m_videobuffer.cedarPicture = cedarPicture;
  
    /* set ref on the frame */

    m_videobuffer.format          = RENDER_FMT_BYPASS_CEDAR;
    m_videobuffer.pts             = DVD_NOPTS_VALUE;
    m_videobuffer.dts             = DVD_NOPTS_VALUE;

    if(m_valid_pts)
      m_videobuffer.pts           = cedarPicture->Picture->pts * 1000;

    if (!m_dts_queue.empty())
    {
      m_videobuffer.dts = m_dts_queue.front();
      m_dts_queue.pop();
    }

    //printf("pts %f dts %f\n", m_videobuffer.pts, m_videobuffer.dts);

    m_videobuffer.iDisplayWidth   = m_decoded_width;
    m_videobuffer.iDisplayHeight  = m_decoded_height;
    m_videobuffer.iWidth          = cedarPicture->Picture->width;
    m_videobuffer.iHeight         = cedarPicture->Picture->height;

    m_videobuffer.data[0]         = cedarPicture->Picture->y;
    m_videobuffer.data[1]         = cedarPicture->Picture->u;
    m_videobuffer.data[2]         = cedarPicture->Picture->u;
    m_videobuffer.iLineSize[0]    = cedarPicture->Picture->width;
    m_videobuffer.iLineSize[1]    = cedarPicture->Picture->height / 2;
    m_videobuffer.iLineSize[2]    = cedarPicture->Picture->height / 2;

    m_videobuffer.iFlags          = DVP_FLAG_ALLOCATED;
    m_videobuffer.iFlags          |= m_drop_state ? DVP_FLAG_DROPPED : 0;

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
    delete pDvdVideoPicture->cedarPicture;
  }
  pDvdVideoPicture->cedarPicture = NULL;

  return CDVDVideoCodec::ClearPicture(pDvdVideoPicture);
}

void CDVDVideoCodecCedar::Reset(void)
{
  ClearPackages();
  if(m_cedarDecoder)
    m_cedarDecoder->Flush();
}

void CDVDVideoCodecCedar::ClearPackages()
{
  while(!m_cedarPackages.empty())
  {
    CCedarPackage *cedarPackage = m_cedarPackages.front();
    m_cedarPackages.pop_front();
    delete cedarPackage;
  }
}

#endif
