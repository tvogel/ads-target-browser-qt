#pragma once

#include <QDialog>

namespace Ui
{
class ConnectDialog;
}

class ConnectDialog : public QDialog
{
  Q_OBJECT

public:
  explicit ConnectDialog(QWidget * parent = nullptr);
  ~ConnectDialog();

  QString getNetId() const;
  QString getIp() const;
  int getPort() const;

private slots:
  void checkInputs();
  void onNetIdChanged(const QString & text);

private:
  Ui::ConnectDialog * ui;
};
