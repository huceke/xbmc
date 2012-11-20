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

#ifndef _OVERLAYRENDERCEDAR_H_
#define _OVERLAYRENDERCEDAR_H_

#include "system.h"

#if defined(HAVE_LIBCEDAR)

#include <asm/types.h>
#include <drv_display_sun4i.h>
#include "guilib/Geometry.h"
#include "cores/VideoRenderers/RenderFlags.h"
#include "cores/VideoRenderers/RenderFormats.h"
#include "DllLibCedar.h"

class CCedarOverlayManager
{
public:
  CCedarOverlayManager();
  ~CCedarOverlayManager();
  void Close();
  void UpdatePos(CRect &destRect, CRect &sourceRect);
  void SetColorKey();
  bool Open(CRect &destRect, CRect &sourceRect, ERenderFormat format);
  bool InitVideoLayer(__disp_pixel_fmt_t fmt);
  bool RenderFrame(unsigned int yAddr, unsigned int uAddr, unsigned int vAddr,
      unsigned int ySize, unsigned int uSize, unsigned int vSize,
      __disp_pixel_fmt_t fmt, int frame_rate, int frame);
  int  GetFrameID();
  bool WaitVSYNC();
  __disp_colorkey_t ColorKey;
private:
protected:
  unsigned long m_cedarArguments[4];
  unsigned int  m_width;
  unsigned int  m_height;
  int           m_screenId;
  int           m_DisplayHandle;
  int           m_videoLayer;
  int           m_FrameBufferHandle;
  int           m_FrameBufferLayerHandle;
  int           m_displayLayer;
  bool          m_open;
  bool          m_videoLayerInit;
  unsigned int  m_displayWidth;
  unsigned int  m_displayHeight;
  CRect         m_destRect;
  CRect         m_sourceRect;
  __disp_layer_info_t m_videoLayerAttr;
  ERenderFormat m_format;
  DllLibCedar   m_dllCedar;
};

#endif

#endif
