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
#include "DllLibCedar.h"
#include "threads/Thread.h"
#include "threads/SingleLock.h"
#include "DllAvFormat.h"

#include <list>
#include <vector>
#include <queue>
#include <deque>

class CDVDVideoCodecCedar;
class CBitstreamConverter;

typedef struct CedarPicture
{
  vpicture_t *Picture;
} CedarPicture;

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
  const char* GetName() { return m_video_codec_name.c_str(); };
  void Process();
protected:
  // Video format
  bool                            m_drop_state;
  unsigned int                    m_decoded_width;
  unsigned int                    m_decoded_height;
  CStdString                      m_video_codec_name;
  bool                            m_valid_pts;
  uint64_t                        m_packet_count;
  bool                            m_is_open;

  unsigned int m_input_size;

  std::queue<double> m_dts_queue;

  CCedarDecoder                   *m_cedarDecoder;
  CBitstreamConverter             *m_converter;
  bool                            m_convert;
  DllLibCedar                     m_dllCedar;
  DVDVideoPicture                 m_videobuffer;
  CedarPicture                    m_cedarPicture;

  bool NaluFormatStartCodes(enum CodecID codec, uint8_t *in_extradata, int in_extrasize);
};

#endif
