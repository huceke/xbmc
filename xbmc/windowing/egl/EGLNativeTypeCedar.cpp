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
#include "EGLNativeTypeCedar.h"
#include <stdlib.h>
#include <linux/fb.h>
#include <sys/ioctl.h>
#include "utils/StringUtils.h"
#include "guilib/gui3d.h"

CEGLNativeTypeCedar::CEGLNativeTypeCedar()
{
}

CEGLNativeTypeCedar::~CEGLNativeTypeCedar()
{
}

bool CEGLNativeTypeCedar::CheckCompatibility()
{
  int tmpfd = open("/dev/cedar_dev", O_RDONLY);

  if(tmpfd >= 0)
  {
    close(tmpfd);
    return true;
  }

  return false;
}

void CEGLNativeTypeCedar::Initialize()
{
  SetCpuMinLimit(true);
  return;
}
void CEGLNativeTypeCedar::Destroy()
{
  SetCpuMinLimit(false);
  return;
}

bool CEGLNativeTypeCedar::CreateNativeDisplay()
{
  m_nativeDisplay = EGL_DEFAULT_DISPLAY;
  return true;
}

bool CEGLNativeTypeCedar::CreateNativeWindow()
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

bool CEGLNativeTypeCedar::GetNativeDisplay(XBNativeDisplayType **nativeDisplay) const
{
  if (!nativeDisplay)
    return false;
  *nativeDisplay = (XBNativeDisplayType*) &m_nativeDisplay;
  return true;
}

bool CEGLNativeTypeCedar::GetNativeWindow(XBNativeWindowType **nativeWindow) const
{
  if (!nativeWindow)
    return false;
  *nativeWindow = (XBNativeWindowType*) &m_nativeWindow;
  return true;
}

bool CEGLNativeTypeCedar::DestroyNativeDisplay()
{
  return true;
}

bool CEGLNativeTypeCedar::DestroyNativeWindow()
{
  free(m_nativeWindow);
  return true;
}

bool CEGLNativeTypeCedar::GetNativeResolution(RESOLUTION_INFO *res) const
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

bool CEGLNativeTypeCedar::SetNativeResolution(const RESOLUTION_INFO &res)
{
  return false;
}

bool CEGLNativeTypeCedar::ProbeResolutions(std::vector<RESOLUTION_INFO> &resolutions)
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

bool CEGLNativeTypeCedar::GetPreferredResolution(RESOLUTION_INFO *res) const
{
  return false;
}

bool CEGLNativeTypeCedar::ShowWindow(bool show)
{
  return false;
}

int CEGLNativeTypeCedar::get_sysfs_str(const char *path, char *valstr, const int size) const
{
  int fd = open(path, O_RDONLY);
  if (fd >= 0)
  {
    int len = read(fd, valstr, size - 1);
    if (len != -1 )
      valstr[len] = '\0';
    close(fd);
  }
  else
  {
    sprintf(valstr, "%s", "fail");
    return -1;
  }
  return 0;
}

int CEGLNativeTypeCedar::set_sysfs_str(const char *path, const char *val) const
{
  int fd = open(path, O_CREAT | O_RDWR | O_TRUNC, 0644);
  if (fd >= 0)
  {
    write(fd, val, strlen(val));
    close(fd);
    return 0;
  }
  return -1;
}

int CEGLNativeTypeCedar::set_sysfs_int(const char *path, const int val) const
{
  char bcmd[16];
  int fd = open(path, O_CREAT | O_RDWR | O_TRUNC, 0644);
  if (fd >= 0)
  {
    sprintf(bcmd, "%d", val);
    write(fd, bcmd, strlen(bcmd));
    close(fd);
    return 0;
  }
  return -1;
}

int CEGLNativeTypeCedar::get_sysfs_int(const char *path) const
{
  int val = 0;
  char bcmd[16];
  int fd = open(path, O_RDONLY);
  if (fd >= 0)
  {
    memset(bcmd, 0, 16);
    read(fd, bcmd, sizeof(bcmd));
    val = strtol(bcmd, NULL, 0);
    close(fd);
  }
  return val;
}

void CEGLNativeTypeCedar::SetCpuMinLimit(bool limit)
{
  // set performance governor, ondemand is optimized for powersave, not our needs
  char scaling_governor[256] = {0};

  if(limit)
  {
    get_sysfs_str("/sys/devices/system/cpu/cpu0/cpufreq/scaling_governor", scaling_governor, 255);
    set_sysfs_str("/sys/devices/system/cpu/cpu0/cpufreq/scaling_governor", "performance");
    m_scaling_governor = scaling_governor;
  }
  else
  {
    set_sysfs_str("/sys/devices/system/cpu/cpu0/cpufreq/scaling_governor", m_scaling_governor.c_str());
  }
}
