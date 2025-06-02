#include "TargetBrowser.h"
#include "ui_TargetBrowser.h"

#include "ConnectDialog.h"
#include "RouteCreationDialog.h"

#include <QAction>
#include <QApplication>
#include <QClipboard>
#include <QDir>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QKeyEvent>
#include <QMessageBox>
#include <QPushButton>
#include <QSettings>
#include <QSortFilterProxyModel>
#include <QStandardPaths>
#include <QVariant>

#include "AdsCodec.h"
#include "AdsDatatypeEntry.h"
#include "AdsDatatypeIndex.h"
#include "AdsDevice.h"
#include "AdsSymbolIndex.h"
#include "AdsSymbolModel.h"
#include "AdsSymbolUploadInfo2.h"
#include "RemoteRouteCreation.h"

struct RecentConnection
{
  QString netId;
  QString ip;
  int port;

  bool operator==(const RecentConnection & other) const
  {
    return netId == other.netId && ip == other.ip && port == other.port;
  }
};

Q_DECLARE_METATYPE(RecentConnection)

TargetBrowser::TargetBrowser(QWidget * parent)
    : QMainWindow(parent), mUi(new Ui::TargetBrowser)
{
  mUi->setupUi(this);

  auto proxyModel = new QSortFilterProxyModel(this);
  proxyModel->setAutoAcceptChildRows(true);
  proxyModel->setRecursiveFilteringEnabled(true);
  proxyModel->setFilterCaseSensitivity(Qt::CaseInsensitive);
  mUi->targetView->setModel(proxyModel);

  mUi->action_Connect_to_recent->setMenu(new QMenu(this));
  connect(mUi->action_Connect, &QAction::triggered, this,
          &TargetBrowser::onConnect);
  connect(mUi->action_Quit, &QAction::triggered, this, &QMainWindow::close);
  connect(mUi->action_Connect_to_recent, &QAction::triggered, this,
          &TargetBrowser::openRecentMenuAndFocusFirstItem);
  connect(mUi->action_Connect_to_recent->menu(), &QMenu::triggered, this,
          &TargetBrowser::connectToRecentTarget);
  connect(mUi->action_Copy_full_name, &QAction::triggered, this,
          &TargetBrowser::copyFullNameToClipboard);
  connect(mUi->action_Read_value, &QAction::triggered, this,
          &TargetBrowser::readSelectedVariableValue);
  connect(mUi->searchInput, &QLineEdit::textChanged, this,
          [this](const QString & text)
          {
            auto proxyModel = qobject_cast<QSortFilterProxyModel *>(mUi->targetView->model());
            Q_ASSERT(proxyModel);
            proxyModel->setFilterFixedString(text);
          });
  connect(mUi->actionCreate_remote_rou_te, &QAction::triggered, this,
          &TargetBrowser::onCreateRemoteRoute);
  connect(mUi->actionExport_Symbols, &QAction::triggered, this, &TargetBrowser::exportSymbols);
  connect(mUi->actionExport_Data_Types, &QAction::triggered, this, &TargetBrowser::exportDataTypes);

  loadRecentConnections();
}

TargetBrowser::~TargetBrowser() { delete mUi; }

void TargetBrowser::onConnect()
{
  ConnectDialog dialog(this);
  if (dialog.exec() == QDialog::Accepted)
  {
    mNetId = dialog.getNetId();
    mIp = dialog.getIp();
    mPort = dialog.getPort();

    connectToTarget();
  }
}

void TargetBrowser::connectToTarget()
{
  qDebug("Connecting to NetId: %s, IP: %s, Port: %d", qPrintable(mNetId),
         qPrintable(mIp), mPort);

  try
  {
    mAdsDevice.reset(
        new AdsDevice(qPrintable(mIp), mNetId.toStdString(), mPort));
  }
  catch (const std::exception & e)
  {
    QMessageBox::critical(this, "Connection Error",
                          strlen(e.what()) > 0 ? e.what() : "Unknown error");
    qCritical("Failed to connect to target: %s", e.what());
    return;
  }

  if (mAdsDevice->GetLocalPort() == 0)
  {
    QMessageBox::critical(this, "Error", "Unable to open ads port");
    return;
  }

  mUi->statusbar->showMessage(
      QString("Connected to NetId: %1, IP: %2, Port: %3")
          .arg(mNetId, mIp)
          .arg(mPort));

  addRecentConnection(mNetId, mIp, mPort);
  saveRecentConnections();

  retrieveSymbolsAndTypes();

  auto typeIndex = AdsDatatypeIndex(mDatatypes);
  auto symbolIndex = AdsSymbolIndex(mSymbols);

  auto proxyModel = qobject_cast<QSortFilterProxyModel *>(mUi->targetView->model());
  Q_ASSERT(proxyModel);
  auto oldModel = proxyModel->sourceModel();
  proxyModel->setSourceModel(new AdsSymbolModel(std::move(typeIndex), std::move(symbolIndex), this));
  proxyModel->setFilterKeyColumn(AdsSymbolModel::FullNameColumn);
  delete oldModel;
  mUi->targetView->hideColumn(AdsSymbolModel::FullNameColumn);
  for (int c = 0; c < mUi->targetView->model()->columnCount(); ++c)
    mUi->targetView->resizeColumnToContents(c);

  connect(mUi->targetView->selectionModel(),
          &QItemSelectionModel::currentChanged, this,
          &TargetBrowser::onCurrentIndexChanged);
}

void TargetBrowser::connectToRecentTarget(QAction * action)
{
  RecentConnection recent = action->data().value<RecentConnection>();
  mNetId = recent.netId;
  mIp = recent.ip;
  mPort = recent.port;
  connectToTarget();
}

void TargetBrowser::addRecentConnection(const QString & netId,
                                        const QString & ip, int port)
{
  RecentConnection recent{netId, ip, port};

  // Check if the connection already exists
  for (QAction * action : mUi->action_Connect_to_recent->menu()->actions())
  {
    RecentConnection existing = action->data().value<RecentConnection>();
    if (existing == recent)
    {
      // Remove the existing action to re-add it at the end
      mUi->action_Connect_to_recent->menu()->removeAction(action);
      delete action; // Clean up the old action
      break;
    }
  }

  auto action = new QAction(
      QString("NetId: %1, IP: %2, Port: %3").arg(netId, ip).arg(port), this);
  action->setData(QVariant::fromValue(recent));
  mUi->action_Connect_to_recent->menu()->addAction(action);
}

void TargetBrowser::saveRecentConnections() const
{
  QSettings settings;
  settings.beginWriteArray("RecentConnections");
  int size = mUi->action_Connect_to_recent->menu()->actions().size();
  for (int i = 0; i < size; ++i)
  {
    QAction * action = mUi->action_Connect_to_recent->menu()->actions().at(i);
    RecentConnection recent = action->data().value<RecentConnection>();
    settings.setArrayIndex(i);
    settings.setValue("netId", recent.netId);
    settings.setValue("ip", recent.ip);
    settings.setValue("port", recent.port);
  }
  settings.endArray();
}

void TargetBrowser::loadRecentConnections()
{
  mUi->action_Connect_to_recent->menu()->clear();

  QSettings settings;
  int size = settings.beginReadArray("RecentConnections");
  for (int i = 0; i < size; ++i)
  {
    settings.setArrayIndex(i);
    QString netId = settings.value("netId").toString();
    QString ip = settings.value("ip").toString();
    int port = settings.value("port").toInt();

    addRecentConnection(netId, ip, port);
  }
  settings.endArray();
}

void TargetBrowser::openRecentMenuAndFocusFirstItem()
{
  mUi->menubar->setActiveAction(mUi->menu_File->menuAction());
  mUi->menu_File->setActiveAction(mUi->action_Connect_to_recent);
  if (auto firstAction =
          mUi->action_Connect_to_recent->menu()->actions().value(0))
    mUi->action_Connect_to_recent->menu()->setActiveAction(firstAction);
  mUi->action_Connect_to_recent->menu()->activateWindow();
}

void TargetBrowser::retrieveSymbolsAndTypes()
{
  if (!mAdsDevice)
  {
    return;
  }

  mSymbols.clear();
  mDatatypes.clear();

  QString cacheFilename =
      QStandardPaths::writableLocation(QStandardPaths::CacheLocation) +
      "/symbols/" + mNetId;

  if (loadFromCache())
  {
    return;
  }

  try
  {
    auto symbolUploadInfo = AdsSymbolUploadInfo2::fromDevice(*mAdsDevice);
    mSymbols = symbolUploadInfo.uploadSymbols(*mAdsDevice);
    mDatatypes = symbolUploadInfo.uploadDatatypes(*mAdsDevice);
  }
  catch (const std::exception & e)
  {
    QMessageBox::critical(this, "Error", e.what());
    return;
  }

  saveToCache();
}

void TargetBrowser::saveToCache()
{
  QString cacheFilename =
      QStandardPaths::writableLocation(QStandardPaths::CacheLocation) +
      "/symbols/" + mNetId;

  QDir::root().mkpath(QFileInfo(cacheFilename).absolutePath());
  QFile cacheFile(cacheFilename);
  if (!cacheFile.open(QIODevice::WriteOnly))
  {
    qWarning() << "Failed to open cache file for writing:" << cacheFilename;
    return;
  }

  QDataStream out(&cacheFile);
  out << mSymbols << mDatatypes;
  if (out.status() != QDataStream::Ok)
  {
    qWarning() << "Failed to write symbols and datatypes to cache:"
               << out.status();
  }
  else
  {
    qDebug() << "Symbols and datatypes saved to cache:" << cacheFilename;
  }
}

bool TargetBrowser::loadFromCache()
{
  QString cacheFilename =
      QStandardPaths::writableLocation(QStandardPaths::CacheLocation) +
      "/symbols/" + mNetId;

  if (!QFile::exists(cacheFilename))
    return false;

  QFile cacheFile(cacheFilename);
  if (!cacheFile.open(QIODevice::ReadOnly))
  {
    qWarning() << "Failed to open cache file for reading:" << cacheFilename;
    return false;
  }
  QDataStream in(&cacheFile);
  in >> mSymbols >> mDatatypes;
  if (in.status() != QDataStream::Ok)
  {
    qWarning() << "Failed to read symbols and datatypes from cache:"
               << in.status();
    return false;
  }
  qDebug() << "Loaded symbols and datatypes from cache:" << cacheFilename;
  return true;
}

void TargetBrowser::copyFullNameToClipboard()
{
  auto model = mUi->targetView->model();
  if (!model)
  {
    mUi->statusbar->showMessage("No data loaded.");
    return;
  }

  auto selectedIndex =
      mUi->targetView->selectionModel()->selectedIndexes().value(0);
  auto fullNameIndex =
      model->index(selectedIndex.row(), AdsSymbolModel::FullNameColumn,
                   selectedIndex.parent());
  QString fullName = model->data(fullNameIndex, Qt::DisplayRole).toString();

  if (fullName.isEmpty())
  {
    mUi->statusbar->showMessage("No valid full name to copy.");
    return;
  }

  QClipboard * clipboard = QApplication::clipboard();
  clipboard->setText(fullName);
  mUi->statusbar->showMessage(QString("Full name copied to clipboard: %1").arg(fullName));
}

static QVariant valueToVariant(const QByteArray & value, AdsDatatypeId type)
{
  switch (type)
  {
    case AdsDatatypeId::Void:
      return QVariant();
    case AdsDatatypeId::Bit:
      return value.at(0) != 0;
    case AdsDatatypeId::Int8:
      return static_cast<int8_t>(value.at(0));
    case AdsDatatypeId::UInt8:
      return static_cast<uint8_t>(value.at(0));
    case AdsDatatypeId::Int16:
      return qFromLittleEndian<int16_t>(value.data());
    case AdsDatatypeId::UInt16:
      return qFromLittleEndian<uint16_t>(value.data());
    case AdsDatatypeId::Int32:
      return qFromLittleEndian<int32_t>(value.data());
    case AdsDatatypeId::UInt32:
      return qFromLittleEndian<uint32_t>(value.data());
    case AdsDatatypeId::Int64:
      return qFromLittleEndian<qint64>(value.data());
    case AdsDatatypeId::UInt64:
      return qFromLittleEndian<quint64>(value.data());
    case AdsDatatypeId::Real32:
      return qFromLittleEndian<float>(value.data());
    case AdsDatatypeId::Real64:
      return qFromLittleEndian<double>(value.data());
    case AdsDatatypeId::Real80:
      return QVariant::fromValue(qFromLittleEndian<long double>(value.data()));
    case AdsDatatypeId::String:
      return QString::fromUtf8(value); // or, use Ads::codec()?
    case AdsDatatypeId::WString:
      return QString::fromUtf16(
          reinterpret_cast<const char16_t *>(value.data()),
          value.size() / sizeof(char16_t));
    case AdsDatatypeId::BigType:
      // Handle BigType as a custom type, or return as QByteArray
      return QString("Unresolved struct, hex dump: %1")
          .arg(value.toHex());
    default:
      break;
  }
  return QString("Unknown type %1").arg(int(type)); // Default case
}

void TargetBrowser::onCurrentIndexChanged()
{
  auto model = mUi->targetView->model();
  if (!model)
  {
    mUi->statusbar->showMessage("No data loaded.");
    return;
  }

  auto existingButtons = QList<QPushButton *>();
  for (int i = 0; i < mUi->pathLayout->count(); ++i)
  {
    auto item = mUi->pathLayout->itemAt(i);
    if (auto button = qobject_cast<QPushButton *>(item->widget()))
    {
      existingButtons.append(button);
    }
  }

  auto selectedIndex =
      mUi->targetView->selectionModel()->currentIndex();
  auto symbolNode = model->data(
                             model->index(selectedIndex.row(),
                                          AdsSymbolModel::FullNameColumn,
                                          selectedIndex.parent()),
                             Qt::UserRole)
                        .value<const AdsSymbolModel::SymbolNode *>();
  if (!symbolNode)
    return;

  auto pathParts = QList{Ads::codec()->toUnicode(symbolNode->symbol->name())};
  if (selectedIndex.parent().isValid())
    pathParts << symbolNode->type->fullName().split('.');

  for (int iPathPart = 0; iPathPart < pathParts.size(); ++iPathPart)
  {
    auto part = pathParts.at(iPathPart);
    auto button = existingButtons.value(iPathPart, nullptr);
    if (button)
    {
      button->setText(part);
      button->show();
    }
    else
    {
      button = new QPushButton(part, this);
      connect(button, &QPushButton::clicked, this, [this, iPathPart]()
              { goToLevel(iPathPart); });
      mUi->pathLayout->addWidget(button);
    }
  }
  for (int i = pathParts.size(); i < existingButtons.size(); ++i)
  {
    existingButtons.at(i)->hide();
  }
}

void TargetBrowser::goToLevel(int level)
{
  auto model = mUi->targetView->model();
  if (!model)
  {
    mUi->statusbar->showMessage("No data loaded.");
    return;
  }

  auto selectedIndex =
      mUi->targetView->selectionModel()->currentIndex();
  auto parents = QList{selectedIndex};
  while (parents.first().parent().isValid())
  {
    parents.prepend(parents.first().parent());
  }
  mUi->targetView->setCurrentIndex(parents.value(level));
}

void TargetBrowser::readSelectedVariableValue()
{
  if (!mAdsDevice)
  {
    mUi->statusbar->showMessage("Not connected to any target.");
    return;
  }

  auto model = mUi->targetView->model();
  if (!model)
  {
    mUi->statusbar->showMessage("No data loaded.");
    return;
  }

  auto selectedIndex =
      mUi->targetView->selectionModel()->selectedIndexes().value(0);
  auto fullNameIndex =
      model->index(selectedIndex.row(), AdsSymbolModel::FullNameColumn,
                   selectedIndex.parent());
  auto symbolNode = model->data(fullNameIndex, Qt::UserRole)
                        .value<const AdsSymbolModel::SymbolNode *>();
  if (!(symbolNode))
  {
    mUi->statusbar->showMessage("Invalid variable.");
    return;
  }

  try
  {
    QByteArray value(symbolNode->type->adsType()->size, Qt::Uninitialized);
    auto result =
        mAdsDevice->ReadReqEx2(
            symbolNode->group(),
            symbolNode->offset(),
            value.size(),
            value.data(), nullptr);
    if (result != ADSERR_NOERR)
      throw AdsException(result);

    auto interpretedValue = valueToVariant(
        value,
        AdsDatatypeId(symbolNode->type->adsType()->dataType));

    mUi->statusbar->showMessage(QString("Value read: %1").arg(interpretedValue.toString()));
  }
  catch (const std::exception & e)
  {
    mUi->statusbar->showMessage(
        QString("Failed to read variable: %1").arg(e.what()));
  }
}

void TargetBrowser::onCreateRemoteRoute()
{
  RouteCreationDialog dialog(this);
  if (dialog.exec() != QDialog::Accepted)
    return;

  try
  {
    bool ok = adsAddRouteToPLC(
        dialog.sendingNetId(),
        dialog.addingHostName(),
        dialog.ipAddress(),
        dialog.username(),
        dialog.password(),
        dialog.routeName(),
        dialog.addedNetId());
    if (ok)
      QMessageBox::information(this, "Route Creation", "Route created successfully.");
    else
      QMessageBox::warning(this, "Route Creation", "Failed to create route: Incorrect password or access denied.");
  }
  catch (const std::exception & e)
  {
    QMessageBox::critical(this, "Route Creation Error", e.what());
  }
}

AdsSymbolModel * TargetBrowser::symbolModel() const
{
  return static_cast<AdsSymbolModel *>(
      static_cast<QSortFilterProxyModel *>(
          mUi->targetView->model())
          ->sourceModel());
}

void TargetBrowser::exportSymbols()
{
  auto model = symbolModel();
  if (!model)
    return;

  QString fileName = QFileDialog::getSaveFileName(this, tr("Export Symbols"), QString(), tr("JSON Files (*.json)"));
  if (fileName.isEmpty())
    return;

  QJsonArray arr;
  const auto & symbols = model->symbolIndex().entries();
  for (const auto & entry : symbols)
  {
    arr.append(entry->toJson());
  }
  QJsonDocument doc(arr);
  QFile file(fileName);
  if (file.open(QIODevice::WriteOnly))
    file.write(doc.toJson());
}

void TargetBrowser::exportDataTypes()
{
  auto model = symbolModel();
  if (!model)
    return;

  QString fileName = QFileDialog::getSaveFileName(this, tr("Export Data Types"), QString(), tr("JSON Files (*.json)"));
  if (fileName.isEmpty())
    return;

  QJsonArray arr;
  const auto & types = model->typeIndex().entries();
  for (const auto & entry : types)
  {
    arr.append(entry->adsType()->toJson());
  }
  QJsonDocument doc(arr);
  QFile file(fileName);
  if (file.open(QIODevice::WriteOnly))
    file.write(doc.toJson());
}

int main(int argc, char * argv[])
{
  QApplication app(argc, argv);

  app.setApplicationName("ADS Target Browser");
  app.setApplicationVersion("1.0.0");
  app.setOrganizationName("Tilman Vogel Excellent Code Solutions");
  app.setOrganizationDomain("excellent-co.de");

  TargetBrowser browser;
  browser.show();
  return app.exec();
}
