#include "AdsDatatypeEntry.h"
#include "AdsCodec.h"
#include "AdsDef.h"
#include "AdsEntryTrailer.h"

#include <QDebug>
#include <QJsonArray>
#include <QJsonObject>
#include <QtEndian>

const char * adsDatatypeIdToString(AdsDatatypeId type)
{
  switch (type)
  {
    case AdsDatatypeId::Void:
      return "Void";
    case AdsDatatypeId::Int8:
      return "Int8";
    case AdsDatatypeId::UInt8:
      return "UInt8";
    case AdsDatatypeId::Int16:
      return "Int16";
    case AdsDatatypeId::UInt16:
      return "UInt16";
    case AdsDatatypeId::Int32:
      return "Int32";
    case AdsDatatypeId::UInt32:
      return "UInt32";
    case AdsDatatypeId::Int64:
      return "Int64";
    case AdsDatatypeId::UInt64:
      return "UInt64";
    case AdsDatatypeId::Real32:
      return "Real32";
    case AdsDatatypeId::Real64:
      return "Real64";
    case AdsDatatypeId::BigType:
      return "BigType";
    case AdsDatatypeId::String:
      return "String";
    case AdsDatatypeId::WString:
      return "WString";
    case AdsDatatypeId::Real80:
      return "Real80";
    case AdsDatatypeId::Bit:
      return "Bit";
    default:
      return "Unknown";
  }
}

QString adsDatatypeFlagsToString(uint32_t flags)
{
  QStringList flagStrings;
  if (flags & ADSDATATYPEFLAG_DATATYPE)
    flagStrings << "DATATYPE";
  if (flags & ADSDATATYPEFLAG_DATAITEM)
    flagStrings << "DATAITEM";
  if (flags & ADSDATATYPEFLAG_REFERENCETO)
    flagStrings << "REFERENCETO";
  if (flags & ADSDATATYPEFLAG_METHODDEREF)
    flagStrings << "METHODDEREF";
  if (flags & ADSDATATYPEFLAG_OVERSAMPLE)
    flagStrings << "OVERSAMPLE";
  if (flags & ADSDATATYPEFLAG_BITVALUES)
    flagStrings << "BITVALUES";
  if (flags & ADSDATATYPEFLAG_PROPITEM)
    flagStrings << "PROPITEM";
  if (flags & ADSDATATYPEFLAG_TYPEGUID)
    flagStrings << "TYPEGUID";
  if (flags & ADSDATATYPEFLAG_PERSISTENT)
    flagStrings << "PERSISTENT";
  if (flags & ADSDATATYPEFLAG_COPYMASK)
    flagStrings << "COPYMASK";
  if (flags & ADSDATATYPEFLAG_TCCOMIFACEPTR)
    flagStrings << "TCCOMIFACEPTR";
  if (flags & ADSDATATYPEFLAG_METHODINFOS)
    flagStrings << "METHODINFOS";
  if (flags & ADSDATATYPEFLAG_ATTRIBUTES)
    flagStrings << "ATTRIBUTES";
  if (flags & ADSDATATYPEFLAG_ENUMINFOS)
    flagStrings << "ENUMINFOS";
  if (flags & ADSDATATYPEFLAG_ALIGNED)
    flagStrings << "ALIGNED";
  if (flags & ADSDATATYPEFLAG_STATIC)
    flagStrings << "STATIC";
  if (flags & ADSDATATYPEFLAG_SPLEVELS)
    flagStrings << "SPLEVELS";
  if (flags & ADSDATATYPEFLAG_IGNOREPERSIST)
    flagStrings << "IGNOREPERSIST";
  if (flags & ADSDATATYPEFLAG_ANYSIZEARRAY)
    flagStrings << "ANYSIZEARRAY";
  if (flags & ADSDATATYPEFLAG_PERSIST_DT)
    flagStrings << "PERSIST_DT";
  if (flags & ADSDATATYPEFLAG_INITONRESET)
    flagStrings << "INITONRESET";

  uint32_t unknownFlags = flags & ~(
                                      ADSDATATYPEFLAG_DATATYPE | ADSDATATYPEFLAG_DATAITEM | ADSDATATYPEFLAG_REFERENCETO |
                                      ADSDATATYPEFLAG_METHODDEREF | ADSDATATYPEFLAG_OVERSAMPLE | ADSDATATYPEFLAG_BITVALUES |
                                      ADSDATATYPEFLAG_PROPITEM | ADSDATATYPEFLAG_TYPEGUID | ADSDATATYPEFLAG_PERSISTENT |
                                      ADSDATATYPEFLAG_COPYMASK | ADSDATATYPEFLAG_TCCOMIFACEPTR | ADSDATATYPEFLAG_METHODINFOS |
                                      ADSDATATYPEFLAG_ATTRIBUTES | ADSDATATYPEFLAG_ENUMINFOS | ADSDATATYPEFLAG_ALIGNED |
                                      ADSDATATYPEFLAG_STATIC | ADSDATATYPEFLAG_SPLEVELS | ADSDATATYPEFLAG_IGNOREPERSIST |
                                      ADSDATATYPEFLAG_ANYSIZEARRAY | ADSDATATYPEFLAG_PERSIST_DT | ADSDATATYPEFLAG_INITONRESET);
  if (unknownFlags)
  {
    flagStrings << QString("UNKNOWN(0x%1)").arg(unknownFlags, 0, 16);
  }

  return flagStrings.join(" | ");
}

namespace
{
// Decodes an enum value of `size` bytes according to the base data type.
// 64-bit values not representable exactly as double are returned as string.
QJsonValue enumValueToJson(const char * p, uint32_t size, AdsDatatypeId type)
{
  constexpr qint64 maxExactDouble = qint64(1) << 53;
  switch (size)
  {
    case 1:
      return type == AdsDatatypeId::Int8 ? qint64(int8_t(*p)) : qint64(uint8_t(*p));
    case 2:
      return type == AdsDatatypeId::Int16 ? qint64(qFromLittleEndian<int16_t>(p))
                                          : qint64(qFromLittleEndian<uint16_t>(p));
    case 4:
      return type == AdsDatatypeId::Int32 ? qint64(qFromLittleEndian<int32_t>(p))
                                          : qint64(qFromLittleEndian<uint32_t>(p));
    case 8:
      if (type == AdsDatatypeId::Int64)
      {
        auto v = qFromLittleEndian<qint64>(p);
        if (v > maxExactDouble || v < -maxExactDouble)
          return QString::number(v);
        return v;
      }
      else
      {
        auto v = qFromLittleEndian<quint64>(p);
        if (v > quint64(maxExactDouble))
          return QString::number(v);
        return qint64(v);
      }
    default:
      return QString::fromLatin1(QByteArray(p, int(size)).toHex());
  }
}
} // namespace

QJsonObject AdsDatatypeEntry::toJson() const
{
  auto codec = Ads::codec();

  QJsonObject jsonObject{
      {"hashValue", qint64(hashValue)},
      {"typeHashValue", qint64(typeHashValue)},
      {"size", int(size)},
      {"offs", int(offs)},
      {"dataType", int(dataType)},
      {"dataTypeName", adsDatatypeIdToString(AdsDatatypeId(dataType))},
      {"flags", int(flags)},
      {"flagsHex", QString::number(flags, 16)},
      {"flagsString", adsDatatypeFlagsToString(flags)},
      {"name", codec->toUnicode(name(), nameLength)},
      {"type", codec->toUnicode(type(), typeLength)},
      {"comment", codec->toUnicode(comment(), commentLength)},
  };

  // for bit-sized items, offs is given in bits instead of bytes
  if (flags & ADSDATATYPEFLAG_BITVALUES)
    jsonObject["offsInBits"] = true;

  if (arrayDim > 0)
  {
    QJsonArray arrayInfo;
    for (int i = 0; i < arrayDim; ++i)
    {
      const AdsDatatypeArrayInfo * info = this->arrayInfo() + i;
      QJsonObject arrayObj{
          {"lBound", qint64(info->lBound)},
          {"elements", qint64(info->elements)}};
      arrayInfo.append(arrayObj);
    }
    jsonObject["arrayInfo"] = arrayInfo;
  }

  if (subItemCount > 0)
  {
    QJsonArray subItemsArray;
    const AdsDatatypeEntry * entry = subItems();

    for (int i = 0; i < subItemCount; ++i)
    {
      subItemsArray.append(entry->toJson());
      entry = reinterpret_cast<const AdsDatatypeEntry *>(
          reinterpret_cast<const char *>(entry) + entry->entryLength);
    }
    jsonObject["subItems"] = subItemsArray;
  }

  appendTrailerToJson(jsonObject);

  return jsonObject;
}

void AdsDatatypeEntry::appendTrailerToJson(QJsonObject & jsonObject) const
{
  // skip sub items to find the start of the trailer
  auto begin = reinterpret_cast<const char *>(subItems());
  for (int i = 0; i < subItemCount; ++i)
    begin += reinterpret_cast<const AdsDatatypeEntry *>(begin)->entryLength;

  auto end = reinterpret_cast<const char *>(this) + entryLength;
  if (begin > end)
  {
    qWarning() << "Sub items of" << jsonObject["name"].toString() << "exceed the entry length.";
    return;
  }
  Ads::TrailerReader reader(begin, end);

  if (flags & ADSDATATYPEFLAG_TYPEGUID)
  {
    if (auto p = reader.take(16))
      jsonObject["typeGuid"] = Ads::guidToString(p);
  }

  if (flags & ADSDATATYPEFLAG_COPYMASK)
  {
    if (auto p = reader.take(size))
      jsonObject["copyMask"] = QString::fromLatin1(QByteArray(p, int(size)).toHex());
  }

  if (flags & ADSDATATYPEFLAG_METHODINFOS)
  {
    // method entries are not decoded; just skip over them
    auto methodCount = reader.read<uint16_t>();
    for (int i = 0; i < methodCount && reader.ok(); ++i)
    {
      auto methodLength = reader.read<uint32_t>();
      if (methodLength < sizeof(uint32_t))
        reader.invalidate();
      else
        reader.take(methodLength - sizeof(uint32_t));
    }
    if (reader.ok())
      jsonObject["methodCount"] = methodCount;
  }

  if (flags & ADSDATATYPEFLAG_ATTRIBUTES && reader.ok())
    jsonObject["attributes"] = Ads::readAttributes(reader);

  if (flags & ADSDATATYPEFLAG_ENUMINFOS && reader.ok())
  {
    QJsonArray enumInfos;
    auto enumInfoCount = reader.read<uint16_t>();
    for (int i = 0; i < enumInfoCount && reader.ok(); ++i)
    {
      auto nameLength = reader.read<uint8_t>();
      auto enumName = reader.readString(nameLength);
      auto valuePtr = reader.take(size);
      if (reader.ok())
        enumInfos.append(QJsonObject{{"name", enumName},
                                     {"value", enumValueToJson(valuePtr, size, AdsDatatypeId(dataType))}});
    }
    jsonObject["enumInfos"] = enumInfos;
  }

  if (!reader.ok())
    qWarning() << "Truncated type information in data type entry" << jsonObject["name"].toString();
}

AdsDatatypeEntry * AdsDatatypeStructItem(AdsDatatypeEntry * p,
                                         unsigned short iItem)
{
  uint16_t i;
  AdsDatatypeEntry * pItem;
  if (iItem >= p->subItemCount)
    return 0;
  pItem = (AdsDatatypeEntry *)(((unsigned char *)(p + 1)) + p->nameLength +
                               p->typeLength + p->commentLength + 3 +
                               p->arrayDim * sizeof(AdsDatatypeArrayInfo));
  for (i = 0; i < iItem; i++)
    pItem = (AdsDatatypeEntry *)(((unsigned char *)pItem) + pItem->entryLength);
  return pItem;
}
