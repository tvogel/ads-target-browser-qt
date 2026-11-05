#include "AdsSymbolIndex.h"

#include "AdsCodec.h"
#include "AdsDatatypeEntry.h"
#include "AdsEntryTrailer.h"

#include <QDebug>
#include <QJsonArray>
#include <QJsonObject>

// symbol flags not defined in AdsDef.h
#define ADSSYMBOLFLAG_ITFMETHODACCESS ((uint32_t)(1 << 6))
#define ADSSYMBOLFLAG_METHODDEREF ((uint32_t)(1 << 7))
#define ADSSYMBOLFLAG_ATTRIBUTES ((uint32_t)(1 << 12))
#define ADSSYMBOLFLAG_STATIC ((uint32_t)(1 << 13))
#define ADSSYMBOLFLAG_INITONRESET ((uint32_t)(1 << 14))
#define ADSSYMBOLFLAG_EXTENDEDFLAGS ((uint32_t)(1 << 15))

QString AdsSymbolEntryAccess::adsSymbolFlagsToString(uint32_t flags)
{
  static const struct
  {
    uint32_t flag;
    const char * name;
  } knownFlags[] = {
      {ADSSYMBOLFLAG_PERSISTENT, "PERSISTENT"},
      {ADSSYMBOLFLAG_BITVALUE, "BITVALUE"},
      {ADSSYMBOLFLAG_REFERENCETO, "REFERENCETO"},
      {ADSSYMBOLFLAG_TYPEGUID, "TYPEGUID"},
      {ADSSYMBOLFLAG_TCCOMIFACEPTR, "TCCOMIFACEPTR"},
      {ADSSYMBOLFLAG_READONLY, "READONLY"},
      {ADSSYMBOLFLAG_ITFMETHODACCESS, "ITFMETHODACCESS"},
      {ADSSYMBOLFLAG_METHODDEREF, "METHODDEREF"},
      {ADSSYMBOLFLAG_ATTRIBUTES, "ATTRIBUTES"},
      {ADSSYMBOLFLAG_STATIC, "STATIC"},
      {ADSSYMBOLFLAG_INITONRESET, "INITONRESET"},
      {ADSSYMBOLFLAG_EXTENDEDFLAGS, "EXTENDEDFLAGS"},
  };

  // ADSSYMBOLFLAG_CONTEXTMASK is not a flag but a 4 bit field, exported as contextIndex
  flags &= ~ADSSYMBOLFLAG_CONTEXTMASK;

  QStringList flagStrings;
  for (const auto & known : knownFlags)
  {
    if (flags & known.flag)
    {
      flagStrings << known.name;
      flags &= ~known.flag;
    }
  }
  if (flags)
    flagStrings << QString("UNKNOWN(0x%1)").arg(flags, 0, 16);
  return flagStrings.join(" | ");
}

QJsonObject AdsSymbolEntryAccess::toJson() const
{
  auto codec = Ads::codec();
  QJsonObject json;
  json["name"] = codec->toUnicode(name(), nameLength);
  json["type"] = codec->toUnicode(type(), typeLength);
  json["comment"] = codec->toUnicode(comment(), commentLength);
  json["iGroup"] = qint64(iGroup);
  json["iOffs"] = qint64(iOffs);
  json["size"] = qint64(size);
  json["dataType"] = int(dataType);
  json["dataTypeName"] = adsDatatypeIdToString(AdsDatatypeId(dataType));
  json["flags"] = qint64(flags);
  json["flagsHex"] = QString::number(flags, 16);
  json["flagsString"] = adsSymbolFlagsToString(flags);
  json["contextIndex"] = int((flags & ADSSYMBOLFLAG_CONTEXTMASK) >> 8);

  // trailer: GUID, attributes, extended flags; followed by padding up to entryLength
  Ads::TrailerReader reader(comment() + commentLength + 1,
                            reinterpret_cast<const char *>(this) + entryLength);
  if (flags & ADSSYMBOLFLAG_TYPEGUID)
  {
    if (auto p = reader.take(16))
      json["typeGuid"] = Ads::guidToString(p);
  }
  if (flags & ADSSYMBOLFLAG_ATTRIBUTES && reader.ok())
    json["attributes"] = Ads::readAttributes(reader);
  if (flags & ADSSYMBOLFLAG_EXTENDEDFLAGS && reader.ok())
  {
    auto extendedFlags = reader.read<uint32_t>();
    if (reader.ok())
      json["extendedFlags"] = qint64(extendedFlags);
  }
  if (!reader.ok())
    qWarning() << "Truncated symbol information in symbol entry" << json["name"].toString();

  return json;
}

AdsSymbolIndex::~AdsSymbolIndex()
{
}

void AdsSymbolIndex::build()
{
  auto end = mSymbolUpload.constData() + mSymbolUpload.size();
  auto current =
      reinterpret_cast<const AdsSymbolEntryAccess *>(mSymbolUpload.constData());

  while (reinterpret_cast<const char *>(current) < end)
  {
    auto maybeNext = current->maybeNext();
    if (reinterpret_cast<const char *>(maybeNext) > end)
    {
      qCritical() << "Symbol record extends past end of buffer. Skipped.";
      break;
    }
    auto currentName = Ads::codec()->toUnicode(current->name());
    mEntries << current;
    mNameIndex[currentName] = current;
    current = maybeNext;
  }
}
