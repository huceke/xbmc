/*
 *      Copyright (C) 2011-2012 Team XBMC
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
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */
#include <EGL/egl.h>
#include "EGLNativeTypeGeneric.h"
#include <stdlib.h>
#include <linux/fb.h>
#include <sys/ioctl.h>
#include "utils/StringUtils.h"
#include "guilib/gui3d.h"

CEGLNativeTypeGeneric::CEGLNativeTypeGeneric()
{
}

CEGLNativeTypeGeneric::~CEGLNativeTypeGeneric()
{
}

bool CEGLNativeTypeGeneric::CheckCompatibility()
{
  return true;
}

void CEGLNativeTypeGeneric::Initialize()
{
  return;
}
void CEGLNativeTypeGeneric::Destroy()
{
  return;
}

bool CEGLNativeTypeGeneric::CreateNativeDisplay()
{
  m_nativeDisplay = EGL_DEFAULT_DISPLAY;
  return true;
}

bool CEGLNativeTypeGeneric::CreateNativeWindow()
{
#if defined(_FBDEV_WINDOW_H_)
  fbdev_window *nativeWindow = new fbdev_window;
  if (!nativeWindow)
    return false;

  nativeWindow->width = 1280;
  nativeWindow->height = 720;
  m_nativeWindow = nativeWindow;
  return true;
#else
  return false;
#endif
}

bool CEGLNativeTypeGeneric::GetNativeDisplay(XBNativeDisplayType **nativeDisplay) const
{
  if (!nativeDisplay)
    return false;
  *nativeDisplay = (XBNativeDisplayType*) &m_nativeDisplay;
  return true;
}

bool CEGLNativeTypeGeneric::GetNativeWindow(XBNativeWindowType **nativeWindow) const
{
  if (!nativeWindow)
    return false;
  *nativeWindow = (XBNativeWindowType*) &m_nativeWindow;
  return true;
}

bool CEGLNativeTypeGeneric::DestroyNativeDisplay()
{
  return true;
}

bool CEGLNativeTypeGeneric::DestroyNativeWindow()
{
  free(m_nativeWindow);
  return true;
}

bool CEGLNativeTypeGeneric::GetNativeResolution(RESOLUTION_INFO *res) const
{
  res->iWidth  = 1280;
  res->iHeight = 720;
  res->fRefreshRate = 60;
  res->dwFlags= D3DPRESENTFLAG_PROGRESSIVE;
  res->iScreen       = 0;
  res->bFullScreen   = true;
  res->iSubtitles    = (int)(0.965 * res->iHeight);
  res->fPixelRatio   = 1.0f;
  res->iScreenWidth  = res->iWidth;
  res->iScreenHeight = res->iHeight;
  res->strMode.Format("%dx%d @ %.2f%s - Full Screen", res->iScreenWidth, res->iScreenHeight, res->fRefreshRate,
     res->dwFlags & D3DPRESENTFLAG_INTERLACED ? "i" : "");
  return true;
}

bool CEGLNativeTypeGeneric::SetNativeResolution(const RESOLUTION_INFO &res)
{
  return false;
}

bool CEGLNativeTypeGeneric::ProbeResolutions(std::vector<RESOLUTION_INFO> &resolutions)
{
  RESOLUTION_INFO res;

  res.iWidth  = 1280;
  res.iHeight = 720;
  res.fRefreshRate = 60;
  res.dwFlags= D3DPRESENTFLAG_PROGRESSIVE;
  res.iScreen       = 0;
  res.bFullScreen   = true;
  res.iSubtitles    = (int)(0.965 * res.iHeight);
  res.fPixelRatio   = 1.0f;
  res.iScreenWidth  = res.iWidth;
  res.iScreenHeight = res.iHeight;
  res.strMode.Format("%dx%d @ %.2f%s - Full Screen", res.iScreenWidth, res.iScreenHeight, res.fRefreshRate,
     res.dwFlags & D3DPRESENTFLAG_INTERLACED ? "i" : "");

  resolutions.push_back(res);

  return resolutions.size() > 0;
}

bool CEGLNativeTypeGeneric::GetPreferredResolution(RESOLUTION_INFO *res) const
{
  return false;
}

bool CEGLNativeTypeGeneric::ShowWindow(bool show)
{
  return false;
}
