#include "RemoteRouteCreation.h"

#include "AdsDef.h"

#include <QByteArray>
#include <QHostAddress>
#include <QHostInfo>
#include <QNetworkDatagram>
#include <QString>
#include <QUdpSocket>
#include <QtEndian>

#include <optional>
#include <stdexcept>

#pragma pack(push, 1)
struct AddRouteReply
{
  uchar unknown[11];
  uchar isReplyCode;
  quint8 amsId[6];     // AMS ID of PLC
  quint16 amsPort;     // System service (10000)
  quint16 commandCode; // LE
  quint16 unknown2[3];
  quint16 errorCode; // LE
  uchar unknown3[2];
};
#pragma pack(pop)

bool adsAddRouteToPLC(
    const QString & sending_net_id,
    const QString & adding_host_name,
    const QString & ip_address,
    const QString & username,
    const QString & password,
    std::optional<QString> route_name,
    std::optional<QString> added_net_id)
{
  QByteArray hostName = adding_host_name.toUtf8();
  QByteArray netIdToAdd = added_net_id.value_or(sending_net_id).toUtf8();
  QByteArray routeName = route_name.value_or(adding_host_name).toUtf8();
  QByteArray user = username.toUtf8();
  QByteArray pass = password.toUtf8();

  QByteArray data_header;
  QDataStream headerStream(&data_header, QIODevice::WriteOnly);
  headerStream.setByteOrder(QDataStream::LittleEndian);
  headerStream.writeRawData("\x03\x66\x14\x71\x00\x00\x00\x00\x06\x00\x00\x00", 12);
  for (const QString & part : sending_net_id.split('.'))
    headerStream << static_cast<quint8>(part.toUInt());
  headerStream << static_cast<quint16>(10000); // PORT_SYSTEMSERVICE
  headerStream.writeRawData("\x05\x00", 2);
  headerStream.writeRawData("\x00\x00\x0c\x00", 4);
  headerStream << static_cast<quint16>(routeName.size() + 1);
  headerStream.writeRawData(routeName.constData(), routeName.size() + 1);
  headerStream.writeRawData("\x07\x00", 2);

  QByteArray actual_data;
  QDataStream dataStream(&actual_data, QIODevice::WriteOnly);
  dataStream.setByteOrder(QDataStream::LittleEndian);
  dataStream << static_cast<quint16>(6); // AMS ID length
  for (const QByteArray & part : netIdToAdd.split('.'))
    dataStream << static_cast<quint8>(part.toUInt());
  dataStream.writeRawData("\x0d\x00", 2);
  dataStream << static_cast<quint16>(user.size() + 1);
  dataStream.writeRawData(user.constData(), user.size() + 1);
  dataStream.writeRawData("\x02\x00", 2);
  dataStream << static_cast<quint16>(pass.size() + 1);
  dataStream.writeRawData(pass.constData(), pass.size() + 1);
  dataStream.writeRawData("\x05\x00", 2);
  dataStream << static_cast<quint16>(hostName.size() + 1);
  dataStream.writeRawData(hostName.constData(), hostName.size() + 1);

  QByteArray packet = data_header + actual_data;

  QUdpSocket socket;
  socket.writeDatagram(packet, QHostAddress(ip_address), 48899);
  if (!socket.waitForReadyRead(1000))
  {
    throw std::runtime_error("No response from PLC");
  }
  QByteArray data;
  QHostAddress sender;
  quint16 senderPort;
  data.resize(32);
  qint64 size = socket.readDatagram(data.data(), data.size(), &sender, &senderPort);
  if (size < 0)
  {
    throw std::runtime_error("Failed to read response from PLC");
  }
  data.resize(size);
  if (data.size() < 32)
  {
    throw std::runtime_error("Received packet is too small");
  }

  auto reply = reinterpret_cast<const AddRouteReply *>(data.constData());

  if (reply->isReplyCode == 0x80)
  {
    auto errorCode = qFromLittleEndian<quint16>(reply->errorCode);
    if (errorCode == ADSERR_NOERR)
    {
      return true;
    }
    if (errorCode == ADSERR_DEVICE_INVALIDACCESS)
    {
      return false;
    }
    throw std::runtime_error(QString("Error adding route: %1").arg(errorCode).toStdString());
  }
  throw std::runtime_error(QString("Unexpected response from PLC: %1").arg(QString(data.toHex(' '))).toStdString());
}
