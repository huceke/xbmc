#pragma once

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

#include "EGLNativeType.h"
#include <list>

typedef struct ExynosPreset
{
  int iId;
  char strPreset[32];
} ExynosPreset;

class CEGLNativeTypeExynos : public CEGLNativeType
{
public:
  CEGLNativeTypeExynos();
  virtual ~CEGLNativeTypeExynos();
  virtual std::string GetNativeName() const { return "exynos"; };
  virtual bool  CheckCompatibility();
  virtual void  Initialize();
  virtual void  Destroy();
  virtual int   GetQuirks() { return EGL_QUIRK_NONE; };

  virtual bool  CreateNativeDisplay();
  virtual bool  CreateNativeWindow();
  virtual bool  GetNativeDisplay(XBNativeDisplayType **nativeDisplay) const;
  virtual bool  GetNativeWindow(XBNativeWindowType **nativeWindow) const;

  virtual bool  DestroyNativeWindow();
  virtual bool  DestroyNativeDisplay();

  virtual bool  GetNativeResolution(RESOLUTION_INFO *res) const;
  virtual bool  SetNativeResolution(const RESOLUTION_INFO &res);
  virtual bool  ProbeResolutions(std::vector<RESOLUTION_INFO> &resolutions);
  virtual bool  GetPreferredResolution(RESOLUTION_INFO *res) const;

  virtual bool  ShowWindow(bool show);

protected:
  bool SetDisplayResolution(int deviceHandle, int preset, const RESOLUTION_INFO &res);
  bool PresetToResolution(const char *mode, RESOLUTION_INFO *res) const;
  std::string ResolutionToPreset(const RESOLUTION_INFO &res);

  std::list<ExynosPreset *> m_Presets;

  int m_iDeviceHandle;
  int m_iDeviceHandle1;
  int m_iFBHandle;
};
