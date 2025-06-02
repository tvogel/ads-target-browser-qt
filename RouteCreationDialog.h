#pragma once

#include <QDialog>
#include <QString>
#include <optional>

namespace Ui
{
class RouteCreationDialog;
}

class RouteCreationDialog : public QDialog
{
  Q_OBJECT

public:
  explicit RouteCreationDialog(QWidget * parent = nullptr);
  ~RouteCreationDialog();

  QString sendingNetId() const;
  QString addingHostName() const;
  QString ipAddress() const;
  QString username() const;
  QString password() const;
  std::optional<QString> routeName() const;
  std::optional<QString> addedNetId() const;

private slots:
  void onSendingNetIdChanged(const QString & text);

private:
  Ui::RouteCreationDialog * ui;
};
