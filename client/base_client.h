/*
Json Command Server

BASE CLIENT

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

#ifndef JSONCOMMANDSERVER_BASE_CLIENT_H
#define JSONCOMMANDSERVER_BASE_CLIENT_H

#include "jsoncommandserver_global.h"

#include <QtNetwork>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

#include "server/commands_controller.h"


namespace JsonCommandServer {

class JSONCOMMANDSERVERSHARED_EXPORT BaseClient : public QObject, BaseController {
    Q_OBJECT

public:
    BaseClient(QObject* _parent = 0);
    virtual ~BaseClient();

    virtual QString name() = 0;
    virtual QString type() = 0;
    virtual int id() = 0;
    virtual int group() = 0;
    virtual QString description() = 0;

    virtual void setServerHost(const QString& _host);
    virtual void setServerPort(int _port);

    int myServerPort();
    QString myServerIP();

public slots:
    /*Client slots*/

    virtual void clientConnect();
    virtual void clientClose();
    virtual void clientInit();

    void clientRequestNewInitialMessage();
    void clientReadMessage();
    void clientDisplayError(QAbstractSocket::SocketError socketError);
    void clientSessionOpened();
    virtual void clientSendMessage(const QString& _message);
    void clientIdentify();

    QString processMessage(const QString& _message);
    void processMessage(QTcpSocket* _socket, const QString& message);
    QJsonArray convertMessage(const QString& message, bool& ok);

    /*Commands*/
    QJsonArray createMessage(const QString &message, bool &ok, int type_message = MESSAGE_NORMAL);
    QJsonArray createStatus(const QString &message, bool &ok);
    QJsonArray createError(const QString &message, bool &ok);
    QJsonArray createIdentify();
    QJsonArray createPeerList();
    QJsonArray createMessageTo(const QString& from, const QString& to, const QString &message);


    void writeMessage(QTcpSocket* _socket, const QJsonArray& cmd);
    void writeMessage(QTcpSocket* _socket, const QString& message);

    virtual QJsonArray serialize() = 0;
    virtual void unserialize(const QJsonArray& _json) = 0;

    virtual void save() = 0;
    virtual void load() = 0;

    virtual void clearMessages() = 0;
    virtual void enableClient() = 0;
    virtual void disableClient() = 0;

    virtual void addClientMessage(const QString& message) = 0;
    virtual void addStatusMessage(const QString& message) = 0;
    virtual void addErrorMessage(const QString& message) = 0;
    virtual void addIdentify(const QJsonObject& info) = 0;
    virtual void addPeerList(const QList<QString>& peers) = 0;

    virtual void sendMessageTo(const QString& from, const QString& to, const QString& message);

    virtual void addNewInfo(const RemoteNodeInfo& new_info);
    virtual void updateInfos() = 0;

    virtual void updatePeers() = 0;

protected:

    int newKey();
    void newMessage();

    /* Client */
    int client_port_;
    QTcpSocket *client_tcp_socket_;

    QString client_current_message_;
    quint16 client_block_size_;

    QNetworkSession *client_network_session_;

    QString server_ip_;
    int server_port_;

    int next_id_;
    int n_messages_;

    std::map<QString, std::map<int, RemoteNodeInfo> > ips_info_;
};


} // namespace JsonCommandServer

#endif // JSONCOMMANDSERVER_BASE_CLIENT_H

