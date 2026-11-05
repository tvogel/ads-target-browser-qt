#pragma once

/**
 * @file AdsEntryTrailer.h
 *
 * @brief Helpers for decoding the optional, flag-controlled data that follows the fixed
 * part and the strings of AdsSymbolEntry and AdsDatatypeEntry records.
 */

#include <QString>
#include <QtEndian>

#include <cstddef>

class QJsonArray;

namespace Ads
{

// Bounds-checked reader. Once a read would exceed the end, the reader becomes invalid
// and all further reads return default values.
class TrailerReader
{
public:
  TrailerReader(const char * begin, const char * end)
      : mPos(begin), mEnd(end) {}

  bool ok() const { return mOk; }
  void invalidate() { mOk = false; }
  bool has(size_t n) const { return mOk && size_t(mEnd - mPos) >= n; }

  const char * take(size_t n)
  {
    if (!has(n))
    {
      mOk = false;
      return nullptr;
    }
    auto p = mPos;
    mPos += n;
    return p;
  }

  template <typename T>
  T read()
  {
    auto p = take(sizeof(T));
    return p ? qFromLittleEndian<T>(p) : T{};
  }

  // reads a string of given length followed by a terminating \0
  QString readString(size_t length);

private:
  const char * mPos;
  const char * mEnd;
  bool mOk = true;
};

// formats a 16 byte Windows GUID (little endian Data1..Data3) as {xxxxxxxx-xxxx-...}
QString guidToString(const char * p);

// reads "uint16 count; { uint8 nameLength, valueLength; name\0; value\0 }[count]"
// into an array of {name, value} objects
QJsonArray readAttributes(TrailerReader & reader);

} // namespace Ads
