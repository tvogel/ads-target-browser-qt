#pragma once

#include "AdsDef.h"

#include <QByteArray>
#include <QHash>

class QJsonObject;

struct AdsSymbolEntryAccess : public AdsSymbolEntry
{
  static QString adsSymbolFlagsToString(uint32_t flags);

public: // methods
  const char * name() const { return reinterpret_cast<const char *>(this + 1); }
  const char * type() const { return name() + nameLength + 1; }
  const char * comment() const { return type() + typeLength + 1; }
  const AdsSymbolEntryAccess * maybeNext() const
  {
    return reinterpret_cast<const AdsSymbolEntryAccess *>(reinterpret_cast<const char *>(this) + entryLength);
  }
  QJsonObject toJson() const;
};

class AdsSymbolIndex
{
public: // methods
  AdsSymbolIndex(const QByteArray & symbolUpload)
      : mSymbolUpload(symbolUpload)
  {
    build();
  }
  AdsSymbolIndex(AdsSymbolIndex &&) = default;
  Q_DISABLE_COPY(AdsSymbolIndex)

  ~AdsSymbolIndex();

  const auto & entries() const { return mEntries; }

private: // methods
  void build();

private: // attributes
  QByteArray mSymbolUpload;
  QList<const AdsSymbolEntryAccess *> mEntries;
  QHash<QString, const AdsSymbolEntryAccess *> mNameIndex;
  // QHash<QString, Entry *> mEntries;
};
