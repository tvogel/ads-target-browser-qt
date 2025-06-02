#pragma once

#include <cstdint>
#include <memory>

#include <QByteArray>

#pragma pack(push, 1)
struct AdsSymbolUploadInfo2
{
  uint32_t nSymbols = 0;
  uint32_t nSymSize = 0;
  uint32_t nDatatypes = 0;
  uint32_t nDatatypeSize = 0;
  uint32_t nMaxDynSymbols = 0;
  uint32_t nUsedDynSymbols = 0;

  static AdsSymbolUploadInfo2 fromDevice(class AdsDevice & device);

  QByteArray uploadSymbols(class AdsDevice & device) const;
  QByteArray uploadDatatypes(class AdsDevice & device) const;
};
#pragma pack(pop)
