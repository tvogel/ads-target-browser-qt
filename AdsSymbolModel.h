#pragma once

#include "AdsDatatypeIndex.h"
#include "AdsSymbolIndex.h"

#include <QAbstractItemModel>
#include <QString>
#include <QVector>

class AdsSymbolModel : public QAbstractItemModel
{
  Q_OBJECT

public: // types
  enum Columns
  {
    NameColumn,
    TypeColumn,
    CommentColumn,
    FullNameColumn,
    ColumnCount
  };
  struct SymbolNode
  {
    SymbolNode * parent = nullptr;
    QList<SymbolNode *> children;
    const AdsSymbolEntryAccess * symbol = nullptr;
    const AdsDatatypeIndex::Entry * type = nullptr;
    uint32_t group() const
    {
      return symbol ? symbol->iGroup : 0;
    }
    uint32_t offset() const
    {
      return (symbol ? symbol->iOffs : 0) + (type ? type->offset() : 0);
    }
  };

public: // methods
  explicit AdsSymbolModel(AdsDatatypeIndex && typeIndex, AdsSymbolIndex && symbolIndex,
                          QObject * parent = nullptr);
  ~AdsSymbolModel();

  // Accessors for exporting
  const AdsDatatypeIndex & typeIndex() const { return mTypeIndex; }
  const AdsSymbolIndex & symbolIndex() const { return mSymbolIndex; }

  QModelIndex index(int row, int column,
                    const QModelIndex & parent = QModelIndex()) const override;
  QModelIndex parent(const QModelIndex & index) const override;
  int rowCount(const QModelIndex & parent = QModelIndex()) const override;
  int columnCount(const QModelIndex & parent = QModelIndex()) const override;
  QVariant data(const QModelIndex & index,
                int role = Qt::DisplayRole) const override;
  QVariant headerData(int section, Qt::Orientation orientation,
                      int role = Qt::DisplayRole) const override;

private: // methods
  void buildModel();
  static void addSymbol(SymbolNode * parentNode, const AdsSymbolEntryAccess * symbol, const AdsDatatypeIndex::Entry * type = nullptr);

private: // attributes
  AdsDatatypeIndex mTypeIndex;
  AdsSymbolIndex mSymbolIndex;
  mutable SymbolNode * mRootNode;
};
