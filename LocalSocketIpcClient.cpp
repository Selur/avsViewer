/*
 * LocalSocketIpcClient.cpp
 *
 *  Created on: 19.10.2014
 *      Author: Selur
 */

#include "LocalSocketIpcClient.h"
#include <iostream>
#include <QDataStream>


LocalSocketIpcClient::LocalSocketIpcClient(const QString& remoteServername, QObject *parent)
    : QObject(parent), m_blockSize(0)
{
  m_socket = new QLocalSocket(this);
  m_serverName = remoteServername;

  connect(m_socket, SIGNAL(connected()), this, SLOT(socket_connected()));
  connect(m_socket, SIGNAL(disconnected()), this, SLOT(socket_disconnected()));

  connect(m_socket, SIGNAL(readyRead()), this, SLOT(socket_readReady()));
#if (QT_VERSION < QT_VERSION_CHECK(6, 0, 0))
  connect(m_socket, SIGNAL(error(QLocalSocket::LocalSocketError)), this, SLOT(socket_error(QLocalSocket::LocalSocketError)));
#else
  connect(m_socket, SIGNAL(errorOccurred(QLocalSocket::LocalSocketError)), this, SLOT(socket_error(QLocalSocket::LocalSocketError)));
#endif
}

LocalSocketIpcClient::~LocalSocketIpcClient()
{
  m_socket->abort();
  delete m_socket;
  m_socket = nullptr;
}

void LocalSocketIpcClient::send_MessageToServer(QString message)
{
  m_message = message;
  std::cout << "[avsViewer Client]: setting message " << qPrintable(m_message) << std::endl;
  if (m_socket->state() != QLocalSocket::ConnectedState) {
    m_socket->connectToServer(m_serverName);
    m_socket->waitForConnected();
  }
  this->socket_connected();
}

void LocalSocketIpcClient::socket_connected()
{
  if (m_message.isEmpty()) {
    std::cout << "[avsViewer Client]: connected, nothing to send,.." << std::endl;
    return;
  }
  std::cout << "[avsViewer Client]: sending message " << qPrintable(m_message) << std::endl;
  QByteArray block;
  QDataStream out(&block, QIODevice::WriteOnly);
  out.setVersion(QDataStream::Qt_5_5);
  out << m_message.toUtf8();
  out.device()->seek(0);
  m_message = QString();
  m_socket->write(block);
  m_socket->flush();
}

void LocalSocketIpcClient::socket_disconnected()
{
  std::cout << "[avsViewer Client]: socket_disconnected" << std::endl;
}

void LocalSocketIpcClient::socket_readReady()
{
  std::cout << "[avsViewer Client]: socket_readReady" << std::endl;
}


void LocalSocketIpcClient::socket_error(QLocalSocket::LocalSocketError error)
{
#ifdef QT_DEBUG
  QString message;
  if (error == QLocalSocket::ConnectionRefusedError) {
    message = QString("connection was refused by the peer (or timed out).");
  } else if (error == QLocalSocket::SocketAccessError) {
    message = QString("socket operation failed because the application lacked the required privileges.");
  } else if (error == QLocalSocket::SocketResourceError) {
    message = QString("local system ran out of resources (e.g., too many sockets).");
  } else if (error == QLocalSocket::SocketTimeoutError) {
    message = QString("socket operation timed out.");
  } else if (error == QLocalSocket::DatagramTooLargeError) {
    message = QString("datagram was larger than the operating system's limit (which can be as low as 8192 bytes).");
  } else if (error == QLocalSocket::UnsupportedSocketOperationError) {
    message = QString("requested socket operation is not supported by the local operating system.");
  } else if (error == QLocalSocket::OperationError) {
    message = QString("An operation was attempted while the socket was in a state that did not permit it.");
  }  else if (error == QLocalSocket::UnknownSocketError) {
    message = QString("An unidentified error occurred.");
  } else {
    message = QString("Unknown");
  }
  std::cout << "[avsViewer Client]: socket error: " << qPrintable(message)<< std::endl;
#else
  Q_UNUSED(error)
#endif
}
