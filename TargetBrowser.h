#pragma once

#include <QMainWindow>

#include "AdsSymbolUploadInfo2.h"

class AdsSymbolModel;

namespace Ui
{
class TargetBrowser;
}

class TargetBrowser : public QMainWindow
{
  Q_OBJECT
public:
  explicit TargetBrowser(QWidget * parent = nullptr);
  ~TargetBrowser() override;

  void onConnect();

private slots:
  void exportSymbols();
  void exportDataTypes();

private: // methods
  void connectToTarget();
  void connectToRecentTarget(QAction * action);

  void addRecentConnection(const QString & netId, const QString & ip, int port);
  void saveRecentConnections() const;
  void loadRecentConnections();
  void openRecentMenuAndFocusFirstItem();

  void retrieveSymbolsAndTypes();
  void saveToCache();
  bool loadFromCache();

  void onCurrentIndexChanged();
  void goToLevel(int level);

  void readSelectedVariableValue();
  void copyFullNameToClipboard();

  void onCreateRemoteRoute();

  AdsSymbolModel * symbolModel() const;

private: // attributes
  Ui::TargetBrowser * mUi = nullptr;
  QString mNetId;
  QString mIp;
  int mPort = 581;

  std::unique_ptr<class AdsDevice> mAdsDevice;
  QByteArray mSymbols;
  QByteArray mDatatypes;
};
