/*
Json Command Server

BASE SERVER

Copyright (c) 2015, gogo40, Péricles Lopes Machado <pericles.raskolnikoff@gmail.com>
All rights reserved.

Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice, this
list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright notice, this
list of conditions and the following disclaimer in the documentation and/or other
materials provided with the distribution.

3. Neither the name of the copyright holder nor the names of its contributors may
be used to endorse or promote products derived from this software without specific
prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY
EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT
SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/


#ifndef JSONCOMMANDSERVER_BASE_SERVER_H
#define JSONCOMMANDSERVER_BASE_SERVER_H

#include "jsoncommandserver_global.h"

#include <QtNetwork>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

#include <set>
#include <map>
#include <queue>
#include <vector>
#include <QString>

#include "commands_controller.h"

namespace JsonCommandServer {


class JSONCOMMANDSERVERSHARED_EXPORT BaseServer : public QObject, public BaseController {
    Q_OBJECT

public:
    BaseServer(QObject* parent = 0);
    virtual ~BaseServer();

    virtual void initServer();

    virtual QString name() = 0;
    virtual QString type() = 0;
    virtual int id() = 0;
    virtual int group() = 0;
    virtual QString description() = 0;

    virtual QJsonArray serialize() = 0;
    virtual void unserialize(const QJsonArray& _json) = 0;

    virtual void save() = 0;
    virtual void load() = 0;

    QString myIP();
    int myPort();

public slots:
    virtual void sessionOpened();
    void sendInitialMessage();
    void receiveMessage();

    virtual void updateServer();
    void closeServer();

    virtual void addClientMessage(const QString& message) = 0;
    virtual void addStatusMessage(const QString& message) = 0;
    virtual void addErrorMessage(const QString& message) = 0;
    virtual void addIdentify(const QJsonObject& info) = 0;
    virtual void addPeerList(const QList<QString>& peers) = 0;

    virtual void sendMessageTo(const QString& from, const QString& to, const QString& message);
    virtual void sendCommandTo(const QString& from, const QString& to, const QJsonArray& cmd);

    virtual void clearMessages() = 0;

    void displayError(QAbstractSocket::SocketError socketError);

    void processMessage(QTcpSocket* _socket, const QString& message);
    QJsonArray convertMessage(const QString& message, bool& ok);

    void writeMessage(QTcpSocket* _socket, const QJsonArray& cmd);
    void writeMessage(QTcpSocket* _socket, const QString& message);

    /*Commands*/
    QJsonArray createMessage(const QString& from, const QString &message, bool &ok,
                             int type_message = MESSAGE_NORMAL);
    QJsonArray createMessage(const QString &message, bool &ok, int type_message = MESSAGE_NORMAL);
    QJsonArray createStatus(const QString &message, bool &ok);
    QJsonArray createError(const QString &message, bool &ok);
    QJsonArray createPeerList();
    QJsonArray createIdentify();
    QJsonArray createMessageTo(const QString& from, const QString& to, const QString &message);
    QJsonArray createCommandTo(const QString& from, const QString& to, const QJsonArray &cmd);

    virtual void executeCommand(const QJsonArray& cmd);


    void setPortServer(int _port_server);
    void setIPServer(const QString& _IP);

    void addSocketMessage(QTcpSocket* _socket, const QString& _message);
    void addSocket(QTcpSocket* _socket);
    QTcpSocket* getPeer(const QString& _peer);
    QList<QString> getPeers();
    void broadcastMessage(const QJsonArray& cmd);
    void broadcastMessage(const QString& message);
    void eraseSocket(QTcpSocket* _socket);
    int numSockets();

    void setNMaxClients(int _n_max_clients);

    virtual void addNewInfo(const RemoteNodeInfo& new_info);
    virtual void updateInfos() = 0;

    virtual void updatePeers() = 0;

signals:
    void dataReceived(QTcpSocket*, const QString&);

protected:
    int newKey();
    void newMessage();

    QString ip_address_;
    int port_server_;
    QTcpServer* tcp_server_;
    QNetworkSession* network_session_;

    QHash<QTcpSocket*,QByteArray*> buffers_;
    QHash<QTcpSocket*, qint32*> sizes_;

    std::map<QTcpSocket*, QString> clients_test_messages_;
    std::map<QTcpSocket*, QString> socket_ips_;
    std::map<QString, std::map<int, QTcpSocket*> > ips_socket_;
    std::map<QString, std::map<int, RemoteNodeInfo> > ips_info_;
    std::map<QString, QTcpSocket*> peers_;

    int next_key_;
    int n_messages_;
    int n_max_clients_;
};

}  // namespace JsonCommandServer

#endif // JSONCOMMANDSERVER_BASE_SERVER_H

