/*
 *      Copyright (C) 2012 Team XBMC
 *      http://www.solid-run.com
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
 * Original Dove Overlay Rendere written by Rabeeh Khoury from Solid-Run <support@solid-run.com>
 *
 */

#include "system.h"
#if (defined HAVE_CONFIG_H) && (!defined WIN32)
#include "config.h"
#endif

#if defined(HAVE_LIBCEDAR)

#include "system_gl.h"
#include "OverlayRendererCedar.h"
#include "drv_display_sun4i.h"
#include "utils/log.h"
#include <stdlib.h>
#include <malloc.h>
#include "utils/fastmemcpy.h"
#include "guilib/GraphicContext.h"
#include "utils/GLUtils.h"
#include "threads/Thread.h"

#if defined(CLASSNAME)
#undef CLASSNAME
#endif

#define CLASSNAME "COverlayRendererCedar"

COverlayRendererCedar::YUVBUFFER::YUVBUFFER()
{
  memset(&image , 0, sizeof(image));
  flipindex = 0;
}

COverlayRendererCedar::YUVBUFFER::~YUVBUFFER()
{
}

COverlayRendererCedar::COverlayRendererCedar()
{
  for(int i = 0; i < NUM_BUFFERS; i++)
  {
    YV12Image &img = m_buffers[i].image;
    img.plane[0]  = NULL;
    img.plane[1]  = NULL;
    img.plane[2]  = NULL;
    m_buffers[i].softPicture  = NULL;
  }
  m_cedarDecoder = NULL;

  UnInit();
}

COverlayRendererCedar::~COverlayRendererCedar()
{
  UnInit();
}

void COverlayRendererCedar::WaitFrame(int64_t frameId)
{
  if (!m_bConfigured) return;

  unsigned int start = XbmcThreads::SystemClockMillis();
  unsigned int end = 0;

  while(m_overlayCedar.GetFrameID() != frameId)
  {
    Sleep(10);
    end = XbmcThreads::SystemClockMillis() - start;
    if(end > 30)
      break;
  }
  end = XbmcThreads::SystemClockMillis() - start;

  if(end > 30)
    printf("flip time %3d\n", end);
}

void COverlayRendererCedar::ManageDisplay()
{
  CRect view;

  view.x1 = (float)g_settings.m_ResInfo[m_resolution].Overscan.left;
  view.y1 = (float)g_settings.m_ResInfo[m_resolution].Overscan.top;
  view.x2 = (float)g_settings.m_ResInfo[m_resolution].Overscan.right;
  view.y2 = (float)g_settings.m_ResInfo[m_resolution].Overscan.bottom;

  m_sourceRect.x1 = (float)g_settings.m_currentVideoSettings.m_CropLeft;
  m_sourceRect.y1 = (float)g_settings.m_currentVideoSettings.m_CropTop;
  m_sourceRect.x2 = (float)m_sourceWidth - g_settings.m_currentVideoSettings.m_CropRight;
  m_sourceRect.y2 = (float)m_sourceHeight - g_settings.m_currentVideoSettings.m_CropBottom;

  CalcNormalDisplayRect(view.x1, view.y1, view.Width(), view.Height(), GetAspectRatio() * g_settings.m_fPixelRatio, g_settings.m_fZoomAmount, g_settings.m_fVerticalShift);

  m_glRect.x1 = 0.0f; 
  m_glRect.y1 = 0.0f;
  m_glRect.x2 = (float)g_settings.m_ResInfo[m_resolution].iWidth;
  m_glRect.y2 = (float)g_settings.m_ResInfo[m_resolution].iHeight;

  if(m_destRectOld != m_destRect || m_sourceRectOld != m_sourceRect)
  {
    m_destRectOld = m_destRect;
    m_sourceRectOld = m_sourceRect;
    m_overlayCedar.UpdatePos(m_destRect, m_sourceRect);
  }
}

bool COverlayRendererCedar::Configure(unsigned int width, unsigned int height, unsigned int d_width, unsigned int d_height, float fps, unsigned flags, ERenderFormat format, unsigned extended_format, unsigned int orientation)
{
  m_format = format;
  m_renderOrientation = orientation;

  if(!m_dllCedar.Load())
    return false;

  if (m_format != RENDER_FMT_YUV420P && m_format != RENDER_FMT_BYPASS_CEDAR)
  {
    CLog::Log(LOGERROR, "%s::%s - Bad format\n", CLASSNAME, __func__);
    return false;
  }

  m_sourceWidth   = width;
  m_sourceHeight  = height;
  m_iFlags        = flags;

  // Calculate the input frame aspect ratio.
  CalculateFrameAspectRatio(d_width, d_height);
  ChooseBestResolution(fps);
  SetViewMode(g_settings.m_currentVideoSettings.m_ViewMode);
  ManageDisplay();

  if(!m_overlayCedar.Open(m_destRect, m_sourceRect, m_format))
  {
    CLog::Log(LOGERROR, "%s::%s Cedar overlay open\n", CLASSNAME, __func__);
    return false;
  }

  CLog::Log(LOGDEBUG, "m_sourceRect.x1 %f m_sourceRect.x2 %f m_sourceRect.y1 %f m_sourceRect.y2 %f m_sourceFrameRatio %f\n",
      m_sourceRect.x1, m_sourceRect.x2, m_sourceRect.y1, m_sourceRect.y2, m_sourceFrameRatio);
  CLog::Log(LOGDEBUG, "m_destRect.x1 %f m_destRect.x2 %f m_destRect.y1 %f m_destRect.y2 %f\n",
      m_destRect.x1, m_destRect.x2, m_destRect.y1, m_destRect.y2);

  m_enabled = 0;

  for (unsigned int i = 0; i < NUM_BUFFERS; i++)
  {
    FreeYV12Image(i);
    CreateYV12Image(i, m_sourceWidth, m_sourceHeight);
  }

  m_currentBuffer = 0;
  m_bConfigured   = true;

  CLog::Log(LOGDEBUG, "%s::%s - Proper format, continuing\n", CLASSNAME, __func__);

  return m_bConfigured;
}

unsigned int COverlayRendererCedar::PreInit()
{
  UnInit();

  m_currentBuffer = 0;

  m_resolution = g_guiSettings.m_LookAndFeelResolution;
  if ( m_resolution == RES_WINDOW )
    m_resolution = RES_DESKTOP;

  return true;
}

int COverlayRendererCedar::GetImage(YV12Image *image, int source, bool readonly)
{
  if(!image)
    return -1;

  /* take next available buffer */
  if( source == AUTOSOURCE)
    source = NextYV12Image();

  YV12Image &im = m_buffers[source].image;

  for(int p = 0; p < MAX_PLANES; p++)
  {
    image->plane[p]  = im.plane[p];
    image->stride[p] = im.stride[p];
  }

  image->width    = im.width;
  image->height   = im.height;
  image->flags    = im.flags;
  image->cshift_x = im.cshift_x;
  image->cshift_y = im.cshift_y;

  return source;
}

void COverlayRendererCedar::ReleaseImage(int source, bool preserve)
{
  /*
  CCedarPicture *cedarPicture = m_buffers[source].cedarPicture;
  if(cedarPicture)
  {
    m_buffers[source].cedarPicture = NULL;
  }
  */
}

void COverlayRendererCedar::FlipPage(int source)
{
  if (!m_bConfigured)
    return;

  /*
  CCedarPicture *cedarPicture = NULL;

  cedarPicture = m_buffers[m_currentBuffer].softPicture;
  if(cedarPicture && cedarPicture->Picture)
  {
    if(m_format != RENDER_FMT_YUV420P)
    {
      m_buffers[m_currentBuffer].softPicture = NULL;
      cedarPicture->UnRef();
    }
  }
  */

  /* switch source */
  if( source >= 0 && source < NUM_BUFFERS )
    m_currentBuffer = source;
  else
    m_currentBuffer = NextYV12Image();

  /*
  cedarPicture = m_buffers[m_currentBuffer].softPicture;
  if(cedarPicture && cedarPicture->Picture)
  {
    vpicture_t *picture = cedarPicture->Picture;

    __disp_pixel_fmt_t pixel_format = picture->pixel_format == PIXEL_FORMAT_AW_YUV422 ? DISP_FORMAT_YUV422 : DISP_FORMAT_YUV420;

    g_graphicsContext.BeginPaint();
    m_overlayCedar.RenderFrame(cedarPicture->Y, cedarPicture->U, cedarPicture->V, pixel_format, picture->frame_rate, cedarPicture->DisplayId);
    //m_overlayCedar.WaitVSYNC();
    ReleaseOldest(cedarPicture->DisplayId);
    g_graphicsContext.EndPaint();
  }
  */
}

void COverlayRendererCedar::Reset()
{
}

void COverlayRendererCedar::Update(bool bPauseDrawing)
{
  if (!m_bConfigured) return;
  ManageDisplay();
}

void COverlayRendererCedar::AddProcessor(DVDVideoPicture &pic)
{
  if (!m_bConfigured) return;

  //printf("COverlayRendererCedar::AddProcessor %d\n", NextYV12Image());
  YUVBUFFER &buf = m_buffers[NextYV12Image()];

  if(m_format == RENDER_FMT_YUV420P)
  {
    if(!m_cedarDecoder)
      m_cedarDecoder = m_dllCedar.AllocCedarDecoder();

    /* allocate softpicture */
    if(!buf.softPicture)
    {
      vpicture_t *softPicture = (vpicture_t*)malloc(sizeof(vpicture_t));

      memset(softPicture, 0x0, sizeof(vpicture_t));

      /* setup framebuffer picture */
      softPicture->size_y = (pic.iWidth * pic.iHeight);
      softPicture->size_u = pic.iWidth * pic.iHeight / 4;
      softPicture->size_v = pic.iWidth * pic.iHeight / 4;

      softPicture->y = (u8*)m_cedarDecoder->MemPalloc(softPicture->size_y, 1024);
      softPicture->u = (u8*)m_cedarDecoder->MemPalloc(softPicture->size_u, 1024);
      softPicture->v = (u8*)m_cedarDecoder->MemPalloc(softPicture->size_v, 1024);

      softPicture->pixel_format     = PIXEL_FORMAT_AW_YUV420;
      softPicture->store_width      = pic.iWidth;
      softPicture->store_height     = pic.iHeight;
      softPicture->display_width    = pic.iWidth;
      softPicture->display_height   = pic.iHeight;
      softPicture->width            = pic.iWidth;
      softPicture->height           = pic.iHeight;
      softPicture->aspect_ratio     = 1000;

      softPicture->id = m_pictureCount++; 

      buf.softPicture = new CCedarPicture(softPicture, 
          (unsigned int)softPicture->y, (unsigned int)softPicture->u, (unsigned int)softPicture->v,
          (unsigned int)softPicture->size_y, (unsigned int)softPicture->size_u, (unsigned int)softPicture->size_v);
    }

    /* copy to virtual adress */

    int i = 0;
    unsigned char *src = pic.data[0];
    unsigned char *dst = buf.softPicture->Picture->y;
    int w = pic.iWidth;
    int h = pic.iHeight;
    for(i = 0; i < h; i++)
    {
      fast_memcpy(dst, src, w);
      src += pic.iLineSize[0];
      dst += pic.iWidth;
    }

    src = pic.data[1];
    dst = buf.softPicture->Picture->u;

    w >>= 1;
    h >>= 1;

    if(w == pic.iLineSize[1])
    {
        fast_memcpy(dst, src, w*h);
    }
    else
    {
      for(i = 0; i < h; i++)
      {
        fast_memcpy(dst, src, w);
        dst  += w;
        src  += pic.iLineSize[1];
      }
    }

    src = pic.data[2];
    dst = buf.softPicture->Picture->v;

    if(w == pic.iLineSize[1])
    {
        fast_memcpy(dst, src, w*h);
    }
    else
    {
      for(i = 0; i < h; i++)
      {
        fast_memcpy(dst, src, w);
        dst  += w;
        src  += pic.iLineSize[2];
      }
    }
  }
  else
  {
    /* allocate softpicture */
    /*
    if(!buf.softPicture && pic.cedarPicture)
    {
      vpicture_t *softPicture = (vpicture_t*)malloc(sizeof(vpicture_t));
      vpicture_t *cedarPicture = pic.cedarPicture->Picture;

      memcpy(softPicture, cedarPicture, sizeof(vpicture_t));
      if(softPicture->size_y)
        softPicture->y = (u8*)pic.cedarPicture->Decoder->MemPalloc(softPicture->size_y, 1024);
      if(softPicture->size_u)
        softPicture->u = (u8*)pic.cedarPicture->Decoder->MemPalloc(softPicture->size_u, 1024);
      if(softPicture->size_v)
        softPicture->v = (u8*)pic.cedarPicture->Decoder->MemPalloc(softPicture->size_v, 1024);

      softPicture->id = m_pictureCount++; 

      buf.softPicture = new CCedarPicture(softPicture, 
          (unsigned int)softPicture->y, (unsigned int)softPicture->u, (unsigned int)softPicture->v, pic.cedarPicture->Decoder);
    }

    vpicture_t *softPicture = buf.softPicture->Picture;
    if(softPicture && softPicture->y && softPicture->size_u)
    {
      fast_memcpy(softPicture->y, pic.cedarPicture->Picture->y, softPicture->size_y);
      fast_memcpy(softPicture->u, pic.cedarPicture->Picture->u, softPicture->size_u);
    }
    */

    if(pic.cedarPicture)
      buf.softPicture = pic.cedarPicture;
  }

  CCedarPicture *cedarPicture = buf.softPicture;
  if(cedarPicture && cedarPicture->Picture)
  {
    vpicture_t *picture = cedarPicture->Picture;

    __disp_pixel_fmt_t pixel_format = picture->pixel_format == PIXEL_FORMAT_AW_YUV422 ? DISP_FORMAT_YUV422 : DISP_FORMAT_YUV420;

    m_overlayCedar.RenderFrame(cedarPicture->yAddr, cedarPicture->uAddr, cedarPicture->vAddr,
        cedarPicture->ySize, cedarPicture->uSize, cedarPicture->vSize,
        pixel_format, picture->frame_rate, m_pictureCount);
    m_overlayCedar.WaitVSYNC();
    //WaitFrame(m_pictureCount);
    m_pictureCount++;
  }
}

void COverlayRendererCedar::RenderUpdate(bool clear, DWORD flags, DWORD alpha)
{
  if (!m_bConfigured) return;

  glEnable(GL_SCISSOR_TEST);
  glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
  glClear(GL_COLOR_BUFFER_BIT);
}

bool COverlayRendererCedar::RenderCapture(CRenderCapture* capture)
{
  CLog::Log(LOGERROR, "%s::%s - Not implemented\n", CLASSNAME, __func__);
  return true;
}

void COverlayRendererCedar::UnInit()
{
  CLog::Log(LOGDEBUG, "%s::%s\n", CLASSNAME, __func__);

  WaitFrame(m_pictureCount);

  for(int i = 0; i < NUM_BUFFERS; i++)
    FreeYV12Image(i);

  m_overlayCedar.Close();

  m_currentBuffer           = 0;
  m_iFlags                  = 0;
  m_bConfigured             = false;
  m_sourceWidth             = 0;
  m_sourceHeight            = 0;
  m_pictureCount            = 0;
  m_timeOut                 = 0;
  m_format                  = RENDER_FMT_NONE;

  if(m_cedarDecoder)
    m_cedarDecoder = m_dllCedar.FreeCedarDecoder(m_cedarDecoder);

  m_dllCedar.Unload();
}

void COverlayRendererCedar::CreateThumbnail(CBaseTexture* texture, unsigned int width, unsigned int height)
{
  CLog::Log(LOGDEBUG, "%s::%s Was asked to create thumbnail (width = %d, height = %d\n", 
      CLASSNAME, __func__, width, height);
}

bool COverlayRendererCedar::Supports(EDEINTERLACEMODE mode)
{
  return false;
}

bool COverlayRendererCedar::Supports(ERENDERFEATURE feature)
{
  return false;
}

bool COverlayRendererCedar::SupportsMultiPassRendering()
{
  return false;
}

bool COverlayRendererCedar::Supports(EINTERLACEMETHOD method)
{
  return false;
}

bool COverlayRendererCedar::Supports(ESCALINGMETHOD method)
{
  if(method == VS_SCALINGMETHOD_NEAREST || method == VS_SCALINGMETHOD_LINEAR)
    return true;

  return false;
}

EINTERLACEMETHOD COverlayRendererCedar::AutoInterlaceMethod()
{
  return VS_INTERLACEMETHOD_NONE;
}

unsigned int COverlayRendererCedar::NextYV12Image()
{
  return (m_currentBuffer + 1) % NUM_BUFFERS;
}

bool COverlayRendererCedar::CreateYV12Image(unsigned int index, unsigned int width, unsigned int height)
{
  YV12Image &im = m_buffers[index].image;

  im.width  = width;
  im.height = height;
  im.cshift_x = 1;
  im.cshift_y = 1;

  unsigned paddedWidth = (im.width + 15) & ~15;

  im.stride[0] = paddedWidth;
  im.stride[1] = paddedWidth >> im.cshift_x;
  im.stride[2] = paddedWidth >> im.cshift_x;

  im.planesize[0] = im.stride[0] * im.height;
  im.planesize[1] = im.stride[1] * ( im.height >> im.cshift_y );
  im.planesize[2] = im.stride[2] * ( im.height >> im.cshift_y );

  return true;
}

bool COverlayRendererCedar::FreeYV12Image(unsigned int index)
{
  YV12Image &im = m_buffers[index].image;

  for (int i = 0; i < MAX_PLANES; i++)
  {
    im.plane[i] = NULL;
  }

  if(m_buffers[index].softPicture)
  {
    if(m_format == RENDER_FMT_YUV420P)
    {
      CCedarPicture *softPicture = m_buffers[index].softPicture;
      if(softPicture->yAddr)
        m_cedarDecoder->MemPfree(softPicture->Picture->u);
      if(softPicture->uAddr)
        m_cedarDecoder->MemPfree(softPicture->Picture->v);
      if(softPicture->vAddr)
        m_cedarDecoder->MemPfree(softPicture->Picture->v);

      free(softPicture->Picture);

      delete softPicture;
    }
  }
  m_buffers[index].softPicture = NULL;

  memset(&im , 0, sizeof(YV12Image));

  return true;
}

#endif
