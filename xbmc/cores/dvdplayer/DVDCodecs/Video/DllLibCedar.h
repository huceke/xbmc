#pragma once

/*
 *      Copyright (C) 2005-2008 Team XBMC
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
#endif
extern "C" {
#include <libcedar/CedarDecoder.h>
}
#include "DynamicDll.h"
#include "utils/log.h"

#define DLL_PATH_LIBCEDAR "/usr/lib/libcedar.so"

class DllLibCedarInterface
{
public:
  virtual ~DllLibCedarInterface() {}
  virtual CCedarDecoder *AllocCedarDecoder() = 0;
  virtual CCedarDecoder *FreeCedarDecoder(CCedarDecoder *decoder) = 0;
  virtual unsigned int MemGetPhyAddr(unsigned int mem) = 0;
  virtual void *MemPalloc(u32 size, u32 align) = 0;
  virtual void MemPfree(void* p) = 0;
  virtual void MemFlushCache(u8* mem, u32 size) = 0;
};

class DllLibCedar : public DllDynamic, DllLibCedarInterface
{
  DECLARE_DLL_WRAPPER(DllLibCedar, DLL_PATH_LIBCEDAR)
  LOAD_SYMBOLS()
  DEFINE_METHOD0(CCedarDecoder *, AllocCedarDecoder)
  DEFINE_METHOD1(CCedarDecoder *, FreeCedarDecoder, (CCedarDecoder *p1))
  DEFINE_METHOD1(unsigned int, MemGetPhyAddr, (unsigned int p1))
  DEFINE_METHOD2(void *, MemPalloc, (u32 p1, u32 p2))
  DEFINE_METHOD1(void , MemPfree, (void *p1))
  DEFINE_METHOD2(void , MemFlushCache, (u8 *p1, u32 p2))
  BEGIN_METHOD_RESOLVE()
    RESOLVE_METHOD(AllocCedarDecoder)
    RESOLVE_METHOD(FreeCedarDecoder)
    RESOLVE_METHOD(MemGetPhyAddr)
    RESOLVE_METHOD(MemPalloc)
    RESOLVE_METHOD(MemPfree)
    RESOLVE_METHOD(MemFlushCache)
  END_METHOD_RESOLVE()
};
