/*
 *      Copyright (C) 2012 Team XBMC
 *      http://xbmc.org
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

#pragma once

#if (defined HAVE_CONFIG_H) && (!defined WIN32)
  #include "config.h"
#elif defined(_WIN32)
#include "system.h"
#endif

#if defined(HAVE_LIBCEDAR)

#undef __u8
#undef byte

#include "../../settings/VideoSettings.h"
#include "../dvdplayer/DVDCodecs/Video/DVDVideoCodec.h"
#include "RenderFlags.h"
#include "BaseRenderer.h"
#include "settings/GUISettings.h"
#include "settings/Settings.h"

#include "cores/dvdplayer/DVDCodecs/Video/CedarOverlayManager.h"
#include "cores/dvdplayer/DVDCodecs/Video/DVDVideoCodecCedar.h"

class CRenderCapture;
class CBaseTexture;

extern "C"
{
#include <sys/ioctl.h>
#include <linux/fb.h>
#include "drv_display_sun4i.h"
}

struct DRAWRECT
{
  float left;
  float top;
  float right;
  float bottom;
};

struct YUVRANGE
{
  int y_min, y_max;
  int u_min, u_max;
  int v_min, v_max;
};

struct YUVCOEF
{
  float r_up, r_vp;
  float g_up, g_vp;
  float b_up, b_vp;
};

#define AUTOSOURCE -1

#define PLANE_Y 0
#define PLANE_U 1
#define PLANE_V 2

#define FIELD_FULL 0
#define FIELD_ODD 1
#define FIELD_EVEN 2

#define NUM_BUFFERS 2

extern YUVRANGE yuv_range_lim;
extern YUVRANGE yuv_range_full;
extern YUVCOEF yuv_coef_bt601;
extern YUVCOEF yuv_coef_bt709;
extern YUVCOEF yuv_coef_ebu;
extern YUVCOEF yuv_coef_smtp240m;

class COverlayRendererCedar : public CBaseRenderer
{
  public:
    COverlayRendererCedar();
    virtual ~COverlayRendererCedar();

    virtual void Update(bool bPauseDrawing);
    virtual void SetupScreenshot() {};

    bool RenderCapture(CRenderCapture* capture); 

    void CreateThumbnail(CBaseTexture *texture, unsigned int width, unsigned int height);

    // Player functions
    virtual void ManageDisplay();
    bool Configure(unsigned int width, unsigned int height, unsigned int d_width, unsigned int d_height, 
        float fps, unsigned flags, ERenderFormat format, unsigned extended_format, unsigned int orientation);
    virtual bool IsConfigured() { return m_bConfigured; }
    virtual int          GetImage(YV12Image *image, int source = AUTOSOURCE, bool readonly = false);
    virtual void         ReleaseImage(int source, bool preserve = false);
    virtual void         FlipPage(int source);
    virtual unsigned int PreInit();
    virtual void         UnInit();
    virtual void         Reset(); /* resets renderer after seek for example */

    virtual void         AddProcessor(DVDVideoPicture &pic);

    virtual void RenderUpdate(bool clear, DWORD flags = 0, DWORD alpha = 255);

    // Feature support
    virtual bool SupportsMultiPassRendering();
    virtual bool Supports(ERENDERFEATURE feature);
    virtual bool Supports(EDEINTERLACEMODE mode);
    virtual bool Supports(EINTERLACEMETHOD method);
    virtual bool Supports(ESCALINGMETHOD method);

    virtual EINTERLACEMETHOD AutoInterlaceMethod();

  private:
    void WaitFrame(int64_t frameId);

    unsigned int NextYV12Image();
    bool CreateYV12Image(unsigned int index, unsigned int width, unsigned int height);
    bool FreeYV12Image(unsigned int index);

    bool          m_bConfigured;
    unsigned int  m_iFlags;

    struct YUVBUFFER
    {
      YUVBUFFER();
     ~YUVBUFFER();

      YV12Image image;
      unsigned  flipindex;

      CedarPicture *cedarPicture;
    };

    YUVBUFFER     m_buffers[NUM_BUFFERS];
    unsigned int  m_currentBuffer;

    int                       m_enabled;

    CCedarOverlayManager      m_overlayCedar;
    CRect                     m_glRect;
    CRect                     m_destRectOld;
    CRect                     m_sourceRectOld;
    int64_t                   m_pictureCount;
    int                       m_timeOut;
    ERenderFormat             m_format;
    DllLibCedar               m_dllCedar;
    CCedarDecoder             *m_cedarDecoder;
};

inline int NP2( unsigned x )
{
  --x;
  x |= x >> 1;
  x |= x >> 2;
  x |= x >> 4;
  x |= x >> 8;
  x |= x >> 16;
  return ++x;
}
#endif
