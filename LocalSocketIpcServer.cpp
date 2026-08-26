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

LocalSocketIpcServer::LocalSocketIpcServer(const QString& servername, QObject *parent)
    : QObject(parent), m_serverName(servername), m_clientConnection(nullptr)
{
  m_server = new QLocalServer(this);
  for (int i = 0; i < 10; ++i) {
    if (m_server->listen(m_serverName)) {
      connect(m_server, SIGNAL(newConnection()), this, SLOT(socket_new_connection()));
      break;
    }
  }
}

LocalSocketIpcServer::~LocalSocketIpcServer()
{
  if (m_server == nullptr) {
    return;
  }
  m_server->close();
  m_server->disconnect();
  m_server->deleteLater();
  m_server = nullptr;
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

  QLocalSocket* incoming = m_server->nextPendingConnection();
  if (incoming == nullptr) {
    return;
  }
#ifdef QT_DEBUG
  currentState(incoming->state());
#endif
  // newest connection wins; the old code tested the incoming socket (always connected), so QT_DEBUG dropped every command
  if (m_clientConnection != nullptr) {
    m_clientConnection->disconnect(this); // don't let close() re-enter socket_disconnected()
    m_clientConnection->close();
    m_clientConnection->deleteLater();
  }
  m_clientConnection = incoming;
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


void LocalSocketIpcServer::socket_readReady()
{
  if (m_clientConnection == nullptr) {
    std::cout << "[avsViewer Server]: no client connection,..." << std::endl;
    return;
  }
#ifdef QT_DEBUG
  std::cout << "[avsViewer Server]:  socket is ready to be read" << std::endl;
  std::cout << "[avsViewer Server]: connection open:"  << (m_clientConnection->isOpen() ? "true" : "false")  << std::endl;
  std::cout << "[avsViewer Server]: connection readable" << (m_clientConnection->isReadable() ? "true" : "false")  << std::endl;
#endif
  QDataStream in(m_clientConnection);
  in.setVersion(QDataStream::Qt_5_5);
  // a transaction rolls the read back until a whole message arrived; a split one used to decode as empty
  forever {
    in.startTransaction();
    QByteArray message;
    in >> message;
    if (!in.commitTransaction()) {
      return; // incomplete - wait for the rest to turn up
    }
#ifdef QT_DEBUG
    std::cout << "[avsViewer Server]: Message received:" << qPrintable(message) << std::endl;
#endif
    emit messageReceived(QString::fromUtf8(message));
    if (m_clientConnection == nullptr) {
      return; // a slot closed the connection underneath us
    }
  }
}
