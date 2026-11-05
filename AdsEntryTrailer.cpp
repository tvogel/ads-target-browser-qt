#include "AdsEntryTrailer.h"

#include "AdsCodec.h"

#include <QJsonArray>
#include <QJsonObject>
#include <QUuid>

#include <cstring>

namespace Ads
{

QString TrailerReader::readString(size_t length)
{
  auto p = take(length + 1);
  return p ? codec()->toUnicode(p, int(length)) : QString();
}

QString guidToString(const char * p)
{
  uchar b[8];
  std::memcpy(b, p + 8, 8);
  return QUuid(qFromLittleEndian<uint32_t>(p), qFromLittleEndian<uint16_t>(p + 4),
               qFromLittleEndian<uint16_t>(p + 6), b[0], b[1], b[2], b[3], b[4], b[5], b[6], b[7])
      .toString(QUuid::WithBraces);
}

QJsonArray readAttributes(TrailerReader & reader)
{
  QJsonArray attributes;
  auto attributeCount = reader.read<uint16_t>();
  for (int i = 0; i < attributeCount && reader.ok(); ++i)
  {
    auto nameLength = reader.read<uint8_t>();
    auto valueLength = reader.read<uint8_t>();
    auto name = reader.readString(nameLength);
    auto value = reader.readString(valueLength);
    if (reader.ok())
      attributes.append(QJsonObject{{"name", name}, {"value", value}});
  }
  return attributes;
}

} // namespace Ads
