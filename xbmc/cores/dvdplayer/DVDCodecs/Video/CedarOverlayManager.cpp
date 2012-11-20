/*
 *      Copyright (C) 2010 Team XBMC
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

#include "CedarOverlayManager.h"

#if defined(HAVE_LIBCEDAR)

#define CLASSNAME "CCedarOverlayManager"

#include "utils/log.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <stropts.h>
#include <unistd.h>
#include <string.h>

#define FBIO_WAITFORVSYNC       _IOW('F', 0x20, __u32)

CCedarOverlayManager::CCedarOverlayManager()
{
  m_DisplayHandle = 0;
  m_open = false;
  m_videoLayer = 0;
  m_videoLayerInit = false;
  m_FrameBufferHandle = 0;
  m_FrameBufferLayerHandle = 0;
  m_screenId = 0;
  memset(&ColorKey, 0x0, sizeof(__disp_colorkey_t));
  m_format = RENDER_FMT_NONE;
}

CCedarOverlayManager::~CCedarOverlayManager()
{
  Close();
}

void CCedarOverlayManager::Close()
{

  if(m_videoLayer)
  {
    /* stop video layer */
    m_cedarArguments[0] = m_screenId;
    m_cedarArguments[1] = m_videoLayer;
    m_cedarArguments[2] = 0;
    m_cedarArguments[3] = 0;
    ioctl(m_DisplayHandle, DISP_CMD_VIDEO_STOP, m_cedarArguments);

    /* relase video layer */
    m_cedarArguments[0] = m_screenId;
    m_cedarArguments[1] = m_videoLayer;
    m_cedarArguments[2] = 0;
    m_cedarArguments[3] = 0;
    ioctl(m_DisplayHandle, DISP_CMD_LAYER_RELEASE, m_cedarArguments);

    m_videoLayer = 0;
  }

  if(m_DisplayHandle)
  {
    /*
    m_cedarArguments[0] = m_screenId;
    m_cedarArguments[1] = 0;
    m_cedarArguments[2] = 0;
    m_cedarArguments[3] = 0;
    ioctl(m_DisplayHandle, DISP_CMD_EXECUTE_CMD_AND_STOP_CACHE, m_cedarArguments);
    */
    close(m_DisplayHandle);
  }

  /* set alpha on */
  if(m_FrameBufferLayerHandle)
  {
    m_cedarArguments[0] = m_screenId;
    m_cedarArguments[1] = m_FrameBufferLayerHandle;
    m_cedarArguments[2] = 0;
    m_cedarArguments[3] = 0;
    ioctl(m_DisplayHandle, DISP_CMD_LAYER_ALPHA_ON, m_cedarArguments);
  }

  if(m_FrameBufferHandle);
    close(m_FrameBufferHandle);

  m_DisplayHandle = 0;
  m_videoLayer = 0;
  m_FrameBufferHandle = 0;
  m_open = false;
  m_videoLayerInit = false;
  m_format = RENDER_FMT_NONE;

  m_dllCedar.Unload();
}

void CCedarOverlayManager::UpdatePos(CRect &destRect, CRect &sourceRect)
{
  if(m_videoLayer)
  {
    m_destRect = destRect;
    m_sourceRect = sourceRect;

    /*
    memset(&m_videoLayerAttr, 0x0, sizeof(__disp_layer_info_t));
    m_cedarArguments[0] = m_screenId;
    m_cedarArguments[1] = m_videoLayer;
    m_cedarArguments[2] = (unsigned long)(&m_videoLayerAttr);
    m_cedarArguments[3] = 0;
    ioctl(m_DisplayHandle, DISP_CMD_LAYER_GET_PARA, m_cedarArguments);

    m_videoLayerAttr.scn_win.x = m_destRect.x1;
    m_videoLayerAttr.scn_win.y = m_destRect.y1;
    m_videoLayerAttr.scn_win.width  = m_destRect.x2 - m_destRect.x1;
    m_videoLayerAttr.scn_win.height = m_destRect.y2 - m_destRect.y1;
    m_videoLayerAttr.src_win.x = m_sourceRect.x1;
    m_videoLayerAttr.src_win.y = m_sourceRect.y1;
    m_videoLayerAttr.src_win.width = m_sourceRect.x2 - m_sourceRect.x1;
    m_videoLayerAttr.src_win.height = m_sourceRect.y2 - m_sourceRect.y1;

    m_cedarArguments[0] = m_screenId;
    m_cedarArguments[1] = m_videoLayer;
    m_cedarArguments[2] = (unsigned long)(&m_videoLayerAttr);
    m_cedarArguments[3] = 0;
    ioctl(m_DisplayHandle, DISP_CMD_LAYER_SET_PARA, m_cedarArguments);
    */

    /* setup video layer parameters */
    __disp_rect_t scn_win;

    scn_win.x = m_destRect.x1;
    scn_win.y = m_destRect.y1;
    scn_win.width  = m_destRect.x2 - m_destRect.x1;
    scn_win.height = m_destRect.y2 - m_destRect.y1;

    m_cedarArguments[0] = m_screenId;
    m_cedarArguments[1] = m_videoLayer;
    m_cedarArguments[2] = (unsigned long)(&scn_win);
    m_cedarArguments[3] = 0;
    ioctl(m_DisplayHandle, DISP_CMD_LAYER_SET_SCN_WINDOW, m_cedarArguments);

    __disp_rect_t src_win;

    src_win.x = m_sourceRect.x1;
    src_win.y = m_sourceRect.y1;
    src_win.width = m_sourceRect.x2 - m_sourceRect.x1;
    src_win.height = m_sourceRect.y2 - m_sourceRect.y1;

    m_cedarArguments[0] = m_screenId;
    m_cedarArguments[1] = m_videoLayer;
    m_cedarArguments[2] = (unsigned long)(&src_win);
    m_cedarArguments[3] = 0;
    ioctl(m_DisplayHandle, DISP_CMD_LAYER_SET_SRC_WINDOW, m_cedarArguments);
  }
}

void CCedarOverlayManager::SetColorKey()
{
  if(m_videoLayer)
  {
    m_cedarArguments[0] = m_screenId;
    m_cedarArguments[1] = m_videoLayer;
    m_cedarArguments[2] = 0;
    m_cedarArguments[3] = 0;
    int ret = ioctl(m_DisplayHandle, DISP_CMD_LAYER_TOP, m_cedarArguments);
    if(ret != 0)
    {
      CLog::Log(LOGERROR, "%s:%s Error iset layer top err(%d)\n", CLASSNAME, __func__, ret);
      return;
    }

    m_cedarArguments[0] = m_screenId;
    m_cedarArguments[1] = m_videoLayer;
    m_cedarArguments[2] = 0;
    m_cedarArguments[3] = 0;
    if(m_screenId == 0)
    {
      ioctl(m_DisplayHandle, DISP_CMD_LAYER_CK_OFF, m_cedarArguments);
    }
    else
    {
      ioctl(m_DisplayHandle, DISP_CMD_LAYER_CK_ON, m_cedarArguments);
    }
  }

  memset(&ColorKey, 0x0, sizeof(__disp_colorkey_t));

  if(m_screenId == 1)
  {
    ColorKey.ck_min.alpha  = 0xFF;
    ColorKey.ck_min.red    = 0x05;
    ColorKey.ck_min.green  = 0x01;
    ColorKey.ck_min.blue   = 0x07;
    ColorKey.ck_max.alpha  = 0xFF;
    ColorKey.ck_max.red    = 0x05;
    ColorKey.ck_max.green  = 0x01;
    ColorKey.ck_max.blue   = 0x07;
  }
  else
  {
    ColorKey.ck_min.alpha  = 0xFF;
    ColorKey.ck_min.red    = 0x00;
    ColorKey.ck_min.green  = 0x00;
    ColorKey.ck_min.blue   = 0x00;
    ColorKey.ck_max.alpha  = 0xFF;
    ColorKey.ck_max.red    = 0x00;
    ColorKey.ck_max.green  = 0x00;
    ColorKey.ck_max.blue   = 0x00;
  }

  ColorKey.red_match_rule   = 2;
  ColorKey.green_match_rule = 2;
  ColorKey.blue_match_rule  = 2;

  m_cedarArguments[0] = 0;
  m_cedarArguments[1] = (unsigned long) (&ColorKey);
  m_cedarArguments[2] = 0;
  m_cedarArguments[3] = 0;
  ioctl(m_DisplayHandle, DISP_CMD_SET_COLORKEY, m_cedarArguments);

  m_cedarArguments[0] = 0;
  m_cedarArguments[1] = (unsigned long) (&ColorKey);
  m_cedarArguments[2] = 0;
  m_cedarArguments[3] = 0;
  ioctl(m_DisplayHandle, DISP_CMD_SET_BKCOLOR, m_cedarArguments);

  m_cedarArguments[0] = m_screenId;
  m_cedarArguments[1] = m_FrameBufferLayerHandle;
  m_cedarArguments[2] = 0;
  m_cedarArguments[3] = 0;
  ioctl(m_DisplayHandle, DISP_CMD_LAYER_SET_PIPE, m_cedarArguments);

  m_cedarArguments[0] = m_screenId;
  m_cedarArguments[1] = m_FrameBufferLayerHandle;
  m_cedarArguments[2] = 0;
  m_cedarArguments[3] = 0;
  ioctl(m_DisplayHandle, DISP_CMD_LAYER_TOP, m_cedarArguments);

  m_cedarArguments[0] = m_screenId;
  m_cedarArguments[1] = m_FrameBufferLayerHandle;
  m_cedarArguments[2] = 0xFF;
  m_cedarArguments[3] = 0;
  ioctl(m_DisplayHandle, DISP_CMD_LAYER_SET_ALPHA_VALUE, m_cedarArguments);

  m_cedarArguments[0] = m_screenId;
  m_cedarArguments[1] = m_FrameBufferLayerHandle;
  m_cedarArguments[2] = 0;
  m_cedarArguments[3] = 0;
  ioctl(m_DisplayHandle, DISP_CMD_LAYER_ALPHA_OFF, m_cedarArguments);

  m_cedarArguments[0] = m_screenId;
  m_cedarArguments[1] = m_FrameBufferLayerHandle;
  m_cedarArguments[2] = 0;
  m_cedarArguments[3] = 0;
  ioctl(m_DisplayHandle, DISP_CMD_LAYER_CK_OFF, m_cedarArguments);
}

bool CCedarOverlayManager::Open(CRect &destRect, CRect &sourceRect, ERenderFormat format)
{
  if(m_open)
    Close();

  if(!m_dllCedar.Load())
    return false;

  m_destRect = destRect;
  m_sourceRect = sourceRect;
  m_format = format;

  m_DisplayHandle = open("/dev/disp", O_RDWR);
  if(m_DisplayHandle < 0)
  {
    CLog::Log(LOGERROR, "%s:%s Error opening display\n", CLASSNAME, __func__);
    return false;
  }

  /* get video layer */
  m_cedarArguments[0] = m_screenId;
  m_cedarArguments[1] = DISP_LAYER_WORK_MODE_NORMAL;
  m_cedarArguments[2] = 0;
  m_cedarArguments[3] = 0;

  m_videoLayer = ioctl(m_DisplayHandle, DISP_CMD_LAYER_REQUEST, m_cedarArguments);
  if(m_videoLayer == 0)
  {
    CLog::Log(LOGERROR, "%s:%s Error getting video layer err(%d)\n", CLASSNAME, __func__, m_videoLayer);
    return false;
  }
 
  /* get display dimensions */
  m_cedarArguments[0] = m_screenId;
  m_cedarArguments[1] = m_videoLayer;
  m_cedarArguments[2] = 0;
  m_cedarArguments[3] = 0;

  m_displayWidth = ioctl(m_DisplayHandle, DISP_CMD_SCN_GET_WIDTH, m_cedarArguments);
  m_displayHeight = ioctl(m_DisplayHandle, DISP_CMD_SCN_GET_HEIGHT, m_cedarArguments);

  m_FrameBufferHandle = open("/dev/fb0", O_RDWR);
  if(m_FrameBufferHandle < 0)
  {
    CLog::Log(LOGERROR, "%s:%s Error opening framebuffer\n", CLASSNAME, __func__);
    return false;
  }
  ioctl(m_FrameBufferHandle, FBIOGET_LAYER_HDL_0, &m_FrameBufferLayerHandle);

  CLog::Log(LOGINFO, "%s:%s Display %dx%d\n", CLASSNAME, __func__, m_displayWidth, m_displayHeight);

  /*
  m_cedarArguments[0] = m_screenId;
  m_cedarArguments[1] = 0;
  m_cedarArguments[2] = 0;
  m_cedarArguments[3] = 0;
  ioctl(m_DisplayHandle, DISP_CMD_START_CMD_CACHE, m_cedarArguments);
  */

  m_open = true;
  return true;
}

bool CCedarOverlayManager::InitVideoLayer(__disp_pixel_fmt_t fmt)
{
  int ret = 0;

  if(!m_open)
  {
    CLog::Log(LOGERROR, "%s:%s Error Diplay not opened\n", CLASSNAME, __func__);
    return false;
  }

  memset(&m_videoLayerAttr, 0x0, sizeof(__disp_layer_info_t));

  /* setup video layer parameters */
  m_videoLayerAttr.mode           = DISP_LAYER_WORK_MODE_SCALER;
  m_videoLayerAttr.pipe           = 1;
  m_videoLayerAttr.fb.addr[0]     = 0;
  m_videoLayerAttr.fb.addr[1]     = 0;
  m_videoLayerAttr.fb.size.width  = m_sourceRect.x2 - m_sourceRect.x1; //m_displayWidth;
  m_videoLayerAttr.fb.size.height = m_sourceRect.y2 - m_sourceRect.y1; //m_displayHeight;
  m_videoLayerAttr.alpha_en       = 1;
  m_videoLayerAttr.alpha_val      = 0xFF;
  m_videoLayerAttr.scn_win.x      = m_destRect.x1;
  m_videoLayerAttr.scn_win.y      = m_destRect.y1;
  m_videoLayerAttr.scn_win.width  = m_destRect.x2 - m_destRect.x1;
  m_videoLayerAttr.scn_win.height = m_destRect.y2 - m_destRect.y1;
  m_videoLayerAttr.prio           = 0xff;
  m_videoLayerAttr.src_win.x      = m_sourceRect.x1;
  m_videoLayerAttr.src_win.y      = m_sourceRect.y1;
  m_videoLayerAttr.src_win.width  = m_sourceRect.x2 - m_sourceRect.x1;
  m_videoLayerAttr.src_win.height = m_sourceRect.y2 - m_sourceRect.y1;
  m_videoLayerAttr.fb.b_trd_src   = false;
  m_videoLayerAttr.b_trd_out      = false;
  m_videoLayerAttr.fb.trd_mode    = DISP_3D_SRC_MODE_SSH;
  m_videoLayerAttr.out_trd_mode   = DISP_3D_OUT_MODE_FP;
  m_videoLayerAttr.b_from_screen  = false;

  if((m_destRect.x2 - m_destRect.x1) < 720)
    m_videoLayerAttr.fb.cs_mode   = DISP_BT601;
  else
    m_videoLayerAttr.fb.cs_mode   = DISP_BT709;

  if(fmt == DISP_FORMAT_ARGB8888)
  {
    m_videoLayerAttr.fb.mode      = DISP_MOD_NON_MB_PLANAR;
    m_videoLayerAttr.fb.format    = fmt;
    m_videoLayerAttr.fb.br_swap   = 0;
    m_videoLayerAttr.fb.cs_mode   = DISP_YCC;
    m_videoLayerAttr.fb.seq       = DISP_SEQ_P3210;
  }
  else if(m_format == RENDER_FMT_YUV420P)
  {
    m_videoLayerAttr.fb.mode      = DISP_MOD_NON_MB_PLANAR;
    m_videoLayerAttr.fb.format    = DISP_FORMAT_YUV420;
    m_videoLayerAttr.fb.br_swap   = 0;
    m_videoLayerAttr.fb.seq       = DISP_SEQ_YUYV;
  }
  else if(fmt == DISP_FORMAT_YUV420)
  {
    m_videoLayerAttr.fb.mode      = DISP_MOD_MB_UV_COMBINED;
    m_videoLayerAttr.fb.format    = fmt;
    m_videoLayerAttr.fb.br_swap   = 0;
    m_videoLayerAttr.fb.seq       = DISP_SEQ_UVUV;
  }
  else
  {
    return false;
  }

  m_cedarArguments[0] = m_screenId;
  m_cedarArguments[1] = m_videoLayer;
  m_cedarArguments[2] = (unsigned long) (&m_videoLayerAttr);
  m_cedarArguments[3] = 0;
  ioctl(m_DisplayHandle, DISP_CMD_LAYER_SET_PARA, m_cedarArguments);

  m_cedarArguments[0] = m_screenId;
  m_cedarArguments[1] = m_videoLayer;
  m_cedarArguments[2] = 0;
  m_cedarArguments[3] = 0;

  ret = ioctl(m_DisplayHandle, DISP_CMD_LAYER_BOTTOM, m_cedarArguments);
  if(ret != 0)
  {
    CLog::Log(LOGERROR, "%s:%s Error configure video layer err(%d)\n", CLASSNAME, __func__, ret);
    return false;
  }

  SetColorKey();

  m_cedarArguments[0] = m_screenId;
  m_cedarArguments[1] = m_videoLayer;
  m_cedarArguments[2] = 0;
  m_cedarArguments[3] = 0;

  ret = ioctl(m_DisplayHandle, DISP_CMD_LAYER_OPEN, m_cedarArguments);
  if (ret != 0)
  {
    CLog::Log(LOGERROR, "%s:%s Error open display layer err(%d)\n", CLASSNAME, __func__, ret);
    return false;
  }

  /* start video layer */
  m_cedarArguments[0] = m_screenId;
  m_cedarArguments[1] = m_videoLayer;
  m_cedarArguments[2] = 0;
  m_cedarArguments[3] = 0;

  ret = ioctl(m_DisplayHandle, DISP_CMD_VIDEO_START, m_cedarArguments);
  if(ret != 0) 
  {
    CLog::Log(LOGERROR, "%s:%s Error starting video err(%d)\n", CLASSNAME, __func__, ret);
    return false;
  }

  m_videoLayerInit = true;
  return true;
}

bool CCedarOverlayManager::RenderFrame(unsigned int yAddr, unsigned int uAddr, unsigned int vAddr,
    unsigned int ySize, unsigned int uSize, unsigned int vSize,
    __disp_pixel_fmt_t fmt, int frame_rate, int frame)
{
  __disp_video_fb_t   frameBuffer;
  __disp_layer_info_t layerInfo;
  int ret = 0;

  memset(&frameBuffer, 0x0, sizeof(__disp_video_fb_t));

  //frameBuffer.top_field_first   = display_info->top_field_first;
  frameBuffer.interlace = 0;
  frameBuffer.frame_rate    = 60;

  m_dllCedar.MemFlushCache((u8 *)(yAddr), ySize);
  m_dllCedar.MemFlushCache((u8 *)(uAddr), uSize);

  if(yAddr)
    yAddr = m_dllCedar.MemGetPhyAddr(yAddr);
  if(uAddr)
    uAddr = m_dllCedar.MemGetPhyAddr(uAddr);
  if(vAddr)
    vAddr = m_dllCedar.MemGetPhyAddr(vAddr);

  frameBuffer.addr[0] = yAddr;
  frameBuffer.addr[1] = uAddr;
  frameBuffer.addr[2] = vAddr;

  frameBuffer.addr_right[0] = 0;
  frameBuffer.addr_right[1] = 0;
  frameBuffer.addr_right[2] = 0;

  frameBuffer.id = frame;

  if(!m_videoLayerInit)
  {
    /* setu pvideo layer */
    if(!InitVideoLayer(fmt))
      return false;
        
    memset(&layerInfo, 0x0, sizeof(__disp_layer_info_t));

    /* setup framebuffer address */
    m_cedarArguments[0] = m_screenId;
    m_cedarArguments[1] = m_videoLayer;
    m_cedarArguments[2] = (unsigned long) (&layerInfo);
    m_cedarArguments[3] = 0;
    ioctl(m_DisplayHandle, DISP_CMD_LAYER_GET_PARA, m_cedarArguments);

    layerInfo.fb.addr[0] = frameBuffer.addr[0];
    layerInfo.fb.addr[1] = frameBuffer.addr[1];
    layerInfo.fb.addr[2] = frameBuffer.addr[2];

    layerInfo.fb.trd_right_addr[0] = frameBuffer.addr_right[0];
    layerInfo.fb.trd_right_addr[1] = frameBuffer.addr_right[1];
    layerInfo.fb.trd_right_addr[2] = frameBuffer.addr_right[2];

    m_cedarArguments[0] = m_screenId;
    m_cedarArguments[1] = m_videoLayer;
    m_cedarArguments[2] = (unsigned long) (&layerInfo);
    m_cedarArguments[3] = 0;
    ioctl(m_DisplayHandle, DISP_CMD_LAYER_SET_PARA, m_cedarArguments);

    /* open video layer */
#if 0
    m_cedarArguments[0] = m_screenId;
    m_cedarArguments[1] = m_videoLayer;
    m_cedarArguments[2] = 0;
    m_cedarArguments[3] = 0;

    ret = ioctl(m_DisplayHandle, DISP_CMD_LAYER_OPEN, m_cedarArguments);
    if (ret != 0)
    {
      CLog::Log(LOGERROR, "%s:%s Error open display layer err(%d)\n", CLASSNAME, __func__, ret);
      return false;
    }

    /* start video layer */
    m_cedarArguments[0] = m_screenId;
    m_cedarArguments[1] = m_videoLayer;
    m_cedarArguments[2] = 0;
    m_cedarArguments[3] = 0;

    ret = ioctl(m_DisplayHandle, DISP_CMD_VIDEO_START, m_cedarArguments);
    if(ret != 0) 
    {
      CLog::Log(LOGERROR, "%s:%s Error starting video err(%d)\n", CLASSNAME, __func__, ret);
      return false;
    }
#endif
  }
  else
  {
    m_cedarArguments[0] = m_screenId;
    m_cedarArguments[1] = m_videoLayer;
    m_cedarArguments[2] = (unsigned long) (&frameBuffer);
    m_cedarArguments[3] = 0;
    ioctl(m_DisplayHandle, DISP_CMD_VIDEO_SET_FB, m_cedarArguments);
  }

  return true;
}

int CCedarOverlayManager::GetFrameID()
{
  if(!m_open)
    return -1;

  m_cedarArguments[0] = m_screenId;
  m_cedarArguments[1] = m_videoLayer;
  m_cedarArguments[2] = 0;
  m_cedarArguments[3] = 0;

  return ioctl(m_DisplayHandle, DISP_CMD_VIDEO_GET_FRAME_ID, m_cedarArguments);
}

bool CCedarOverlayManager::WaitVSYNC()
{
  if(m_FrameBufferHandle)
    ioctl(m_FrameBufferHandle, FBIO_WAITFORVSYNC, NULL);

  return true;
}

#endif
