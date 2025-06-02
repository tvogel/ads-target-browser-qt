#include "AdsSymbolIndex.h"

#include "AdsCodec.h"
#include "AdsDatatypeEntry.h"

#include <QDebug>
#include <QJsonObject>

QString AdsSymbolEntryAccess::adsSymbolFlagsToString(uint32_t flags)
{
  QStringList flagStrings;
  if (flags & ADSSYMBOLFLAG_PERSISTENT)
    flagStrings << "PERSISTENT";
  if (flags & ADSSYMBOLFLAG_BITVALUE)
    flagStrings << "BITVALUE";
  if (flags & ADSSYMBOLFLAG_REFERENCETO)
    flagStrings << "REFERENCETO";
  if (flags & ADSSYMBOLFLAG_TYPEGUID)
    flagStrings << "TYPEGUID";
  if (flags & ADSSYMBOLFLAG_TCCOMIFACEPTR)
    flagStrings << "TCCOMIFACEPTR";
  if (flags & ADSSYMBOLFLAG_READONLY)
    flagStrings << "READONLY";
  if (flags & ADSSYMBOLFLAG_CONTEXTMASK)
    flagStrings << "CONTEXTMASK";
  return flagStrings.join(" | ");
}

QJsonObject AdsSymbolEntryAccess::toJson() const
{
  auto codec = Ads::codec();
  QJsonObject json;
  json["name"] = codec->toUnicode(name());
  json["type"] = codec->toUnicode(type());
  json["comment"] = codec->toUnicode(comment());
  json["iGroup"] = int(iGroup);
  json["iOffs"] = int(iOffs);
  json["size"] = int(size);
  json["dataType"] = int(dataType);
  json["dataTypeName"] = adsDatatypeIdToString(AdsDatatypeId(dataType));
  json["flags"] = int(flags);
  json["flagsHex"] = QString::number(flags, 16);
  json["flagsString"] = adsSymbolFlagsToString(flags);
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
      qCritical() << "Datatype record extends past end of buffer. Skipped.";
      break;
    }
    auto currentName = Ads::codec()->toUnicode(current->name());
    mEntries << current;
    mNameIndex[currentName] = current;
    current = maybeNext;
  }
}
