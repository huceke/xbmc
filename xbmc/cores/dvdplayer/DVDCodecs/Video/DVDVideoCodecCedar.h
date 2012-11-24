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

#pragma once

#if (defined HAVE_CONFIG_H) && (!defined WIN32)
  #include "config.h"
#elif defined(_WIN32)
#include "system.h"
#endif

#if defined(HAVE_LIBCEDAR)

#include "DVDVideoCodec.h"
#include <libcedar/CedarDecoder.h>
#include "CedarOverlayManager.h"
#include "DllLibCedar.h"
#include "threads/Thread.h"
#include "threads/SingleLock.h"

#include <list>
#include <vector>
#include <queue>
#include <deque>

class CDVDVideoCodecCedar;

struct CCedarPackage
{
public:
  CCedarPackage(uint8_t *pData, int iSize, double dts_, double pts_, bool drop)
  {
    Data = NULL;
    Size = 0;
    DropState = drop;
    if(pData && iSize)
    {
      Data = new uint8_t[iSize];
      if(Data)
      {
        memcpy(Data, pData, iSize);
        Size = iSize;
        pts = pts_;
        dts = dts_;
      }
    };
  }
  ~CCedarPackage()
  {
    delete Data;
  };
  bool    DropState;
  uint8_t *Data;
  int     Size;
  double  dts;
  double  pts;
};

struct CCedarPicture
{
public:
  CCedarPicture(vpicture_t *pict, unsigned int yAddr_, unsigned int uAddr_, unsigned int vAddr_,
      unsigned int ySize_, unsigned int uSize_, unsigned int vSize_) 
  { 
    Picture  = pict;
    yAddr    = yAddr_;
    uAddr    = uAddr_;
    vAddr    = vAddr_;
    ySize    = ySize_;
    uSize    = uSize_;
    vSize    = vSize_;
  };
  ~CCedarPicture() {};
  vpicture_t *Picture;
  CCedarDecoder *Decoder;
  unsigned int yAddr;
  unsigned int uAddr;
  unsigned int vAddr;
  unsigned int ySize;
  unsigned int uSize;
  unsigned int vSize;
};

class CDVDVideoCodecCedar : public CDVDVideoCodec
{
public:
  CDVDVideoCodecCedar();
  ~CDVDVideoCodecCedar();

  // Required overrides
  bool Open(CDVDStreamInfo &hints, CDVDCodecOptions &options);
  void Dispose(void);
  int  Decode(uint8_t *pData, int iSize, double dts, double pts);
  void Reset(void);
  void SetDropState(bool bDrop);
  bool GetPicture(DVDVideoPicture *pDvdVideoPicture);
  bool ClearPicture(DVDVideoPicture* pDvdVideoPicture);
  int  GetFrameCount() { return m_Frames; };
  const char* GetName() { return m_video_codec_name.c_str(); };
  void Process();
protected:
  // Video format
  bool                            m_drop_state;
  unsigned int                    m_decoded_width;
  unsigned int                    m_decoded_height;
  unsigned int                    m_picture_width;
  unsigned int                    m_picture_height;
  bool                            m_is_open;
  bool                            m_Pause;
  bool                            m_setStartTime;
  uint8_t                         *m_extradata;
  int                             m_extrasize;
  CStdString                      m_video_codec_name;
  bool                            m_valid_pts;
  uint64_t                        m_packet_count;

  unsigned int m_input_size;

  unsigned int                    m_Frames;

  CCedarDecoder                   *m_cedarDecoder;

  std::queue<double> m_dts_queue;
  typedef std::list<CCedarPackage*>   PackageList;
  PackageList                         m_cedarPackages;

  void ClearPackages();

  DllLibCedar                   m_dllCedar;
  int                           m_lastDecodeTime;

  DVDVideoPicture               m_videobuffer;
};

#endif
