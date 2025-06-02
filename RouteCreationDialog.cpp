#include "RouteCreationDialog.h"
#include "ui_RouteCreationDialog.h"

#include <QRegularExpression>

RouteCreationDialog::RouteCreationDialog(QWidget * parent)
    : QDialog(parent), ui(new Ui::RouteCreationDialog)
{
  ui->setupUi(this);

  // Connect sendingNetId changes to slot
  connect(ui->editSendingNetId, &QLineEdit::textChanged,
          this, &RouteCreationDialog::onSendingNetIdChanged);
}

RouteCreationDialog::~RouteCreationDialog()
{
  delete ui;
}

QString RouteCreationDialog::sendingNetId() const
{
  return ui->editSendingNetId->text();
}

QString RouteCreationDialog::addingHostName() const
{
  return ui->editAddingHostName->text();
}

QString RouteCreationDialog::ipAddress() const
{
  return ui->editIpAddress->text();
}

QString RouteCreationDialog::username() const
{
  return ui->editUsername->text();
}

QString RouteCreationDialog::password() const
{
  return ui->editPassword->text();
}

std::optional<QString> RouteCreationDialog::routeName() const
{
  QString val = ui->editRouteName->text();
  if (val.isEmpty())
    return std::nullopt;
  return val;
}

std::optional<QString> RouteCreationDialog::addedNetId() const
{
  QString val = ui->editAddedNetId->text();
  if (val.isEmpty())
    return std::nullopt;
  return val;
}

void RouteCreationDialog::onSendingNetIdChanged(const QString & text)
{
  // Extract IP part from NetId (first four dot-separated numbers)
  QStringList parts = text.split('.');
  QString ipPart;
  if (parts.size() >= 4)
  {
    ipPart = parts.mid(0, 4).join('.');
    ui->editAddingHostName->setText(ipPart);
  }
}
