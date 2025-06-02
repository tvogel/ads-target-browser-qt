#include "AdsSymbolModel.h"

#include "AdsCodec.h"
#include "AdsDatatypeEntry.h"

AdsSymbolModel::AdsSymbolModel(AdsDatatypeIndex && typeIndex, AdsSymbolIndex && symbolIndex, QObject * parent)
    : QAbstractItemModel(parent), mTypeIndex(std::move(typeIndex)), mSymbolIndex(std::move(symbolIndex)),
      mRootNode(new SymbolNode)
{
  buildModel();
}

AdsSymbolModel::~AdsSymbolModel()
{
  // Recursively delete all nodes
  QVector<SymbolNode *> stack = {mRootNode};
  while (!stack.isEmpty())
  {
    SymbolNode * node = stack.takeLast();
    stack += node->children;
    delete node;
  }
}

QModelIndex AdsSymbolModel::index(int row, int column,
                                  const QModelIndex & parent) const
{

  SymbolNode * parentNode =
      parent.isValid() ? static_cast<SymbolNode *>(parent.internalPointer())
                       : mRootNode;
  if (parentNode->children.isEmpty() && parentNode != mRootNode)
  {
    for (auto child : parentNode->type->children(mTypeIndex))
    {
      addSymbol(parentNode, parentNode->symbol, child);
    }
  }

  if (row < 0 || row >= parentNode->children.size() || column < 0 ||
      column >= ColumnCount)
    return QModelIndex();

  return createIndex(row, column, parentNode->children[row]);
}

QModelIndex AdsSymbolModel::parent(const QModelIndex & index) const
{
  if (!index.isValid())
    return QModelIndex();

  SymbolNode * childNode = static_cast<SymbolNode *>(index.internalPointer());
  SymbolNode * parentNode = childNode->parent;

  if (parentNode == mRootNode)
    return QModelIndex();

  SymbolNode * grandParentNode = parentNode->parent;
  int row = grandParentNode ? grandParentNode->children.indexOf(parentNode) : 0;
  return createIndex(row, 0, parentNode);
}

int AdsSymbolModel::rowCount(const QModelIndex & parent) const
{
  SymbolNode * parentNode =
      parent.isValid() ? static_cast<SymbolNode *>(parent.internalPointer())
                       : mRootNode;

  if (parent.isValid() && parent.column() != 0)
    return 0;

  if (parentNode == mRootNode)
    return mRootNode->children.size();
  return parentNode->type->childCount(mTypeIndex);
}

int AdsSymbolModel::columnCount(const QModelIndex & parent) const
{
  Q_UNUSED(parent);
  return ColumnCount;
}

QVariant AdsSymbolModel::data(const QModelIndex & index, int role) const
{
  if (!index.isValid())
    return QVariant();

  SymbolNode * node = static_cast<SymbolNode *>(index.internalPointer());
  auto codec = Ads::codec();
  if (role == Qt::DisplayRole)
  {
    switch (index.column())
    {
      case NameColumn:
        return node->parent == mRootNode
                   ? codec->toUnicode(node->symbol->name())
                   : node->type->name();
      case TypeColumn:
        return codec->toUnicode(node->type->adsType()->type());
      case CommentColumn:
        return codec->toUnicode(node->type->adsType()->comment());
      case FullNameColumn:
        return node->parent == mRootNode ? codec->toUnicode(node->symbol->name())
                                         : codec->toUnicode(node->symbol->name()) + "." + node->type->fullName();
      default:
        return QVariant();
    }
  }
  if (role == Qt::UserRole)
  {
    switch (index.column())
    {
      case FullNameColumn:
        return QVariant::fromValue<const SymbolNode *>(node);
    }
  }
  return QVariant();
}

QVariant AdsSymbolModel::headerData(int section, Qt::Orientation orientation,
                                    int role) const
{
  if (orientation != Qt::Horizontal || role != Qt::DisplayRole)
    return QVariant();

  switch (section)
  {
    case NameColumn:
      return "Name";
    case TypeColumn:
      return "Type";
    case CommentColumn:
      return "Comment";
    case FullNameColumn:
      return "Full Name";
    default:
      return QVariant();
  }
}

void AdsSymbolModel::buildModel()
{
  auto codec = Ads::codec();
  for (const AdsSymbolEntryAccess * symbol : mSymbolIndex.entries())
  {
    auto type = mTypeIndex.lookup(codec->toUnicode(symbol->type()));
    if (!type)
    {
      qCritical() << "Symbol type not found for symbol:" << codec->toUnicode(symbol->name()) << "Skipping.";
      continue;
    }
    addSymbol(mRootNode, symbol, type);
  }
}

void AdsSymbolModel::addSymbol(SymbolNode * parentNode, const AdsSymbolEntryAccess * symbol, const AdsDatatypeIndex::Entry * type)
{
  SymbolNode * newNode = new SymbolNode{
      parentNode,
      QList<SymbolNode *>(),
      symbol,
      type};
  parentNode->children.append(newNode);
}
