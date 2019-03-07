/*
 * LocalSocketIpcServer.cpp
 *
 *  Created on: 19.10.2014
 *      Author: Selur
 */

#include "LocalSocketIpcServer.h"
#include <QLocalSocket>
#include <iostream>
#include <QDataStream>

LocalSocketIpcServer::LocalSocketIpcServer(QString servername, QObject *parent)
    : QObject(parent), m_serverName(servername), m_clientConnection(nullptr)
{
  m_server = new QLocalServer(this);
  bool started = false;
  for (int i = 0; i < 10; ++i) {
    started = m_server->listen(m_serverName);
    if (started) {
      connect(m_server, SIGNAL(newConnection()), this, SLOT(socket_new_connection()));
      break;
    }
  }
}

LocalSocketIpcServer::~LocalSocketIpcServer()
{
  m_server->close();
  m_server->deleteLater();
}

void currentState(QLocalSocket::LocalSocketState state)
{
  if (state == QLocalSocket::UnconnectedState) {
    std::cout << "[avsViewer Server]: UnconnectedState" << std::endl;
  } else if (state == QLocalSocket::ConnectingState) {
    std::cout << "[avsViewer Server]: ConnectingState" << std::endl;
  } else if (state == QLocalSocket::ConnectedState) {
    std::cout << "[avsViewer Server]: ConnectedState" << std::endl;
  } else if (state == QLocalSocket::ClosingState) {
    std::cout << "[avsViewer Server]: ClosingState" << std::endl;
  }
}

void LocalSocketIpcServer::socket_new_connection()
{
#ifdef QT_DEBUG
  std::cout << "[avsViewer Server]:: incoming connection" << std::endl;
#endif

  m_clientConnection = m_server->nextPendingConnection();
#ifdef QT_DEBUG
  currentState(m_clientConnection->state());
#endif
  connect(m_clientConnection, SIGNAL(disconnected()), this, SLOT(socket_disconnected()));
  connect(m_clientConnection, SIGNAL(readyRead()), this, SLOT(socket_readReady()));
}

void LocalSocketIpcServer::socket_disconnected()
{
  if (m_clientConnection == nullptr) {
    return;
  }
  m_clientConnection->close();
  m_clientConnection->deleteLater();
  m_clientConnection = nullptr;
}
#include <iostream>
void LocalSocketIpcServer::socket_readReady()
{
  if (m_clientConnection == nullptr) {
    return;
  }
#ifdef QT_DEBUG
  std::cout << "[avsViewer Server]:  socket is ready to be read" << std::endl;
  std::cout << "[avsViewer Server]: connection open:"  << (m_clientConnection->isOpen() ? "true" : "false"))  << std::endl;
  std::cout << "[avsViewer Server]: connection readable", (m_clientConnection->isReadable() ? "true" : "false"))  << std::endl;
#endif
  QDataStream in(m_clientConnection);
  in.setVersion(QDataStream::Qt_5_5);
  try {
    while (m_clientConnection != nullptr && m_clientConnection->bytesAvailable() >= int(sizeof(quint16))) {
      QByteArray message;
      in >> message;
  #ifdef QT_DEBUG
      std::cout << "[avsViewer Server]: Message received:" << qPrintable(message) << std::endl;
  #endif
      emit messageReceived(QString::fromUtf8(message));
    }
  } catch (...) {
#ifdef QT_DEBUG
    std::cout << "[avsViewer Server]: Problem with client connection,..." << std::endl;
    m_clientConnection = nullptr;
#endif
  }
}


