#include "AdsDatatypeIndex.h"

#include "AdsCodec.h"
#include "AdsDatatypeEntry.h"

#include <QDebug>
#include <QFile>

AdsDatatypeIndex::~AdsDatatypeIndex()
{
  qDeleteAll(mEntries);
}

void AdsDatatypeIndex::build()
{
  auto end = mDataTypeUpload.constData() + mDataTypeUpload.size();
  auto current =
      reinterpret_cast<const AdsDatatypeEntry *>(mDataTypeUpload.constData());

  while (reinterpret_cast<const char *>(current) < end)
  {
    auto next = reinterpret_cast<const AdsDatatypeEntry *>(
        reinterpret_cast<const char *>(current) + current->entryLength);
    if (reinterpret_cast<const char *>(next) > end)
    {
      qCritical() << "Datatype record extends past end of buffer. Skipped.";
      break;
    }
    auto currentName = Ads::codec()->toUnicode(current->name());
    mNameRawIndex[currentName] = current;
    auto entry = new Entry(currentName, 0, current);
    mEntries << entry;
    mNameIndex[currentName] = entry;
    current = next;
  }
}

AdsDatatypeIndex::Entry::Entry(const QString & name, uint32_t offset, const AdsDatatypeEntry * _adsType, const Entry * _parent)
    : mParent(_parent), mName(name), mOffset(offset), mAdsType(_adsType)
{
}

AdsDatatypeIndex::Entry::~Entry()
{
  qDeleteAll(mChildren);
}

// static
int AdsDatatypeIndex::Entry::arrayCount(const AdsDatatypeEntry * adsType, const AdsDatatypeIndex & index)
{
  if (adsType->arrayDim == 0)
    return 0;
  int count = 1;
  for (int iArrayDim = 0; iArrayDim < adsType->arrayDim; ++iArrayDim)
  {
    auto arrayInfo = adsType->arrayInfo()[iArrayDim];
    count *= arrayInfo.elements;
  }
  auto type = index.mNameRawIndex.value(Ads::codec()->toUnicode(adsType->type()));
  if (Q_UNLIKELY(!type))
  {
    qCritical() << "Unresolved type" << Ads::codec()->toUnicode(adsType->type())
                << "in" << Ads::codec()->toUnicode(adsType->name());
  }
  else if (Q_UNLIKELY(count * type->size != adsType->size))
  {
    return 0;
  }
  return count;
}

static QStringList expandArrayIndices(const AdsDatatypeEntry * adsType)
{
  auto arrayIndices = QList{QString()};
  for (int iArrayDim = adsType->arrayDim - 1; iArrayDim >= 0; --iArrayDim)
  {
    auto arrayInfo = adsType->arrayInfo()[iArrayDim];
    auto newArrayIndices = QList<QString>();
    for (uint i = arrayInfo.lBound; i < arrayInfo.lBound + arrayInfo.elements; ++i)
    {
      for (const auto & name : arrayIndices)
      {
        if (name.isEmpty())
          newArrayIndices << QString::number(i);
        else
          newArrayIndices << QString::number(i) + "," + name;
      }
    }
    arrayIndices = newArrayIndices;
  }
  arrayIndices.removeIf([](const QString & index)
                        { return index.isNull(); });
  return arrayIndices;
}

int AdsDatatypeIndex::Entry::childCount(const AdsDatatypeIndex & index) const
{
  if (mChildrenLoaded)
    return mChildren.count();

  auto codec = Ads::codec();
  auto typeName = codec->toUnicode(mAdsType->type());
  auto declaration = !typeName.isEmpty() ? index.mNameRawIndex.value(typeName) : mAdsType;

  if (Q_UNLIKELY(!declaration))
  {
    qCritical() << "Unresolved type" << typeName << "in" << codec->toUnicode(mAdsType->name());
    return 0;
  }

  return declaration->subItemCount + arrayCount(declaration, index);
}

auto AdsDatatypeIndex::Entry::children(const AdsDatatypeIndex & index) const -> QList<const Entry *>
{
  if (mChildrenLoaded)
    return mChildren;

  mChildrenLoaded = true;

  auto codec = Ads::codec();
  auto typeName = codec->toUnicode(mAdsType->type());
  auto declaration = !typeName.isEmpty() ? index.mNameRawIndex.value(typeName) : mAdsType;

  if (Q_UNLIKELY(!declaration))
  {
    qCritical() << "Unresolved type" << typeName << "in" << codec->toUnicode(mAdsType->name());
    return mChildren;
  }

  auto currentChild = declaration->subItems();
  for (int iChild = 0; iChild < declaration->subItemCount; ++iChild)
  {
    mChildren << new Entry(codec->toUnicode(currentChild->name()), currentChild->offs, currentChild, this);
    currentChild = reinterpret_cast<const AdsDatatypeEntry *>(
        reinterpret_cast<const char *>(currentChild) + currentChild->entryLength);
  }

  auto arrayIndices = expandArrayIndices(declaration);
  if (arrayIndices.isEmpty())
    return mChildren;

  if (declaration->size % arrayIndices.count() != 0)
  {
    qCritical() << "Size of" << codec->toUnicode(declaration->name()) << "is not divisible by the number of array indices:" << arrayIndices.count();
    return mChildren;
  }
  auto itemSize = declaration->size / arrayIndices.count();
  auto offset = declaration->offs;
  if (Q_UNLIKELY(declaration->offs))
  {
    qWarning() << "Offset of" << codec->toUnicode(declaration->name()) << "is not zero, but" << declaration->offs;
    Q_ASSERT(false);
  }
  for (const auto & arrayIndex : expandArrayIndices(declaration))
  {
    mChildren << new Entry("[" + arrayIndex + "]", offset, declaration, this);
    offset += itemSize;
  }

  Q_ASSERT(offset == declaration->size);

  return mChildren;
}

QString AdsDatatypeIndex::Entry::fullName() const
{
  if (!mParent)
    return QString(); // do not want the type name in the full name
  auto result = mParent->fullName();
  if (result.isNull())
    return mName;
  if (mName.startsWith('['))
    return result + name();
  return result + "." + name();
}

uint32_t AdsDatatypeIndex::Entry::offset() const
{
  return (mParent ? mParent->offset() : 0) + mOffset;
}
