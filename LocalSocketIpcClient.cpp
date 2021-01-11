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
  std::cout << "avsViewer setting message " << qPrintable(message) << std::endl;
  m_socket->abort();
  m_message = message;
  m_socket->connectToServer(m_serverName);
}

void LocalSocketIpcClient::socket_connected()
{
  QByteArray block;
  QDataStream out(&block, QIODevice::WriteOnly);
  out.setVersion(QDataStream::Qt_5_5);
  std::cout << "avsViewer sending message " << qPrintable(m_message) << std::endl;
  out << m_message.toUtf8();
  out.device()->seek(0);
  m_socket->write(block);
  m_socket->flush();
}

void LocalSocketIpcClient::socket_disconnected()
{
  std::cout << "avisynth viewer: socket_disconnected" << std::endl;
}

void LocalSocketIpcClient::socket_readReady()
{
  std::cout << "avisynth viewer: socket_readReady" << std::endl;
}

void LocalSocketIpcClient::socket_error(QLocalSocket::LocalSocketError)
{
  std::cout << "avisynth viewer: socket_error" << std::endl;
}
