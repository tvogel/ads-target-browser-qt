#pragma once

#include <QByteArray>
#include <QHash>

struct AdsDatatypeEntry;

class AdsDatatypeIndex
{
public: // types
  class Entry;

public: // methods
  AdsDatatypeIndex(const QByteArray & dataTypeUpload)
      : mDataTypeUpload(dataTypeUpload)
  {
    build();
  }
  AdsDatatypeIndex(AdsDatatypeIndex &&) = default;
  Q_DISABLE_COPY(AdsDatatypeIndex)

  ~AdsDatatypeIndex();

  auto lookup(const QString & name) const
  {
    return mNameIndex.value(name, nullptr);
  }

  const auto & entries() const { return mEntries; }

private: // methods
  void build();

private: // attributes
  QByteArray mDataTypeUpload;
  QHash<QString, const AdsDatatypeEntry *> mNameRawIndex;
  QList<const Entry *> mEntries;
  QHash<QString, const Entry *> mNameIndex;
};

class AdsDatatypeIndex::Entry
{
public: // methods
  Entry(const QString & name, uint32_t offset, const AdsDatatypeEntry * adsType, const Entry * parent = nullptr);
  ~Entry();
  Q_DISABLE_COPY(Entry)

  const Entry * parent() const { return mParent; }
  const AdsDatatypeEntry * adsType() const { return mAdsType; }
  int childCount(const AdsDatatypeIndex & index) const;
  QList<const Entry *> children(const AdsDatatypeIndex & index) const;

  QString name() const { return mName; }
  QString fullName() const;
  uint32_t offset() const;

private: // methods
  static int arrayCount(const AdsDatatypeEntry * adsType, const AdsDatatypeIndex & index);

private: // attributes
  const Entry * mParent = nullptr;
  QString mName;
  uint32_t mOffset = 0;
  const AdsDatatypeEntry * mAdsType = nullptr;
  mutable bool mChildrenLoaded = false;
  mutable QList<const Entry *> mChildren;
};
