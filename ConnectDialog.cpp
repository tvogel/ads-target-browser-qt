#include "ConnectDialog.h"
#include "ui_ConnectDialog.h"

#include <QIntValidator>
#include <QPushButton>
#include <QRegularExpressionValidator>

ConnectDialog::ConnectDialog(QWidget * parent)
    : QDialog(parent), ui(new Ui::ConnectDialog)
{
  ui->setupUi(this);
  ui->netIdLineEdit->setValidator(new QRegularExpressionValidator(
      QRegularExpression(R"(^((25[0-5]|(2[0-4]|1\d|[1-9]|)\d)\.?\b){6}$)"), this));
  ui->portLineEdit->setValidator(new QIntValidator(0, 65535, this));

  connect(ui->netIdLineEdit, &QLineEdit::textChanged, this, &ConnectDialog::checkInputs);
  connect(ui->ipLineEdit, &QLineEdit::textChanged, this, &ConnectDialog::checkInputs);
  connect(ui->portLineEdit, &QLineEdit::textChanged, this, &ConnectDialog::checkInputs);

  // Auto-fill IP address when NetId changes
  connect(ui->netIdLineEdit, &QLineEdit::textChanged, this, &ConnectDialog::onNetIdChanged);

  checkInputs();
}

ConnectDialog::~ConnectDialog() { delete ui; }

QString ConnectDialog::getNetId() const { return ui->netIdLineEdit->text(); }

QString ConnectDialog::getIp() const { return ui->ipLineEdit->text(); }

int ConnectDialog::getPort() const { return ui->portLineEdit->text().toInt(); }

void ConnectDialog::checkInputs()
{
  bool netIdValid = ui->netIdLineEdit->hasAcceptableInput();
  bool ipValid = !ui->ipLineEdit->text().isEmpty();
  bool portValid = ui->portLineEdit->hasAcceptableInput();

  ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(netIdValid && ipValid && portValid);
}

// Auto-fill IP address from NetId
void ConnectDialog::onNetIdChanged(const QString & text)
{
  QStringList parts = text.split('.');
  if (parts.size() >= 4)
  {
    QString ipPart = parts.mid(0, 4).join('.');
    ui->ipLineEdit->setText(ipPart);
  }
}