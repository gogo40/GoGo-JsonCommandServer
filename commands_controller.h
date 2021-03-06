/*
Json Command Server

COMMANDS CONTROLLER

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

#ifndef JSONCOMMANDSERVER__SERVER_COMMANDS_LIST_H
#define JSONCOMMANDSERVER__SERVER_COMMANDS_LIST_H

#include "jsoncommandserver_global.h"

#include <QObject>
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonDocument>
#include <QThread>
#include <QMutex>

#include <queue>
#include <map>

#include <QTcpSocket>
#include <QTcpServer>
#include <QDataStream>

namespace JsonCommandServer {

inline qint32 ArrayToInt(QByteArray source) {
    qint32 temp;
    QDataStream data(&source, QIODevice::ReadWrite);
    data >> temp;
    return temp;
}


inline QByteArray IntToArray(qint32 source) {
    QByteArray temp;
    QDataStream data(&temp, QIODevice::ReadWrite);
    data << source;
    return temp;
}

////////////////////////////////////////////////////////////////////////////
struct JSONCOMMANDSERVERSHARED_EXPORT RemoteNodeInfo {
    RemoteNodeInfo() {}
    ~RemoteNodeInfo() {}

    int port;
    int id;
    int group;
    QString IP;
    QString name;
    QString type;
    QString description;
    QString time;
    QString date;
};

class JSONCOMMANDSERVERSHARED_EXPORT BaseController {
  public:
    BaseController();
    virtual ~BaseController();

    virtual QJsonArray createMessage(const QString &message, bool &ok, int type_message)
    { return QJsonArray(); }
    virtual QJsonArray createStatus(const QString &message, bool &ok)
    { return QJsonArray(); }
    virtual QJsonArray createError(const QString &message, bool &ok)
    { return QJsonArray(); }
    virtual QJsonArray createIdentify() { return QJsonArray(); }
    virtual QJsonArray createPeerList() { return QJsonArray(); }
    virtual QJsonArray createMessageTo(const QString& from, const QString& to, const QString &message)
    { return QJsonArray(); }
    virtual QJsonArray createCommandTo(const QString& from, const QString& to, const QJsonArray &cmd)
    { return QJsonArray(); }

    virtual void addClientMessage(const QString& message) {}
    virtual void addStatusMessage(const QString& message) {}
    virtual void addErrorMessage(const QString& message) {}
    virtual void addIdentify(const QJsonObject& info) {}
    virtual void executeCommand(const QJsonArray& cmd) {}

    virtual void sendMessageTo(const QString& from, const QString& to, const QString& message) {}
    virtual void sendCommandTo(const QString& from, const QString& to, const QJsonArray& cmd) {}

    virtual void addNewInfo(const RemoteNodeInfo& new_info) {}
    virtual void updateInfos() {}

    virtual void addPeerList(const QList<QString>& peers) {}
    virtual void updatePeers() {}
};


namespace DefaultCommands {
void print_message(BaseController*, const QJsonObject&);
void print_message_status(BaseController*, const QJsonObject&);
void print_message_error(BaseController*, const QJsonObject&);
void process_identify(BaseController*, const QJsonObject&);
void process_peers_list(BaseController*, const QJsonObject&);
void send_message_to(BaseController*, const QJsonObject&);
void send_cmd_to(BaseController*, const QJsonObject&);
}

typedef void (*ProcessCmd)(BaseController*, const QJsonObject&);

void JSONCOMMANDSERVERSHARED_EXPORT execute_command(int cmd_type,
        BaseController* w,
        const QJsonObject& commad);


enum ServerCommands {
    MESSAGE_NORMAL = 0, // SEND NORMAL MESSAGE FROM SERVER TO CLIENT OR FROM CLIENT TO SERVER
    MESSAGE_STATUS = 1, // SEND STATUS MESSAGE FROM SERVER TO CLIENT OR FROM CLIENT TO SERVER
    MESSAGE_ERROR = 2, // SEND ERROR MESSAGE FROM SERVER TO CLIENT
    MESSAGE_IDENTIFY = 3, // SEND INDETIFICATION MESSAGE FROM SERVER TO CLIENT OR FORM CLIENT TO SERVER
    MESSAGE_PEER_LIST = 4, // SEND LIST OF CONNECTED CLIENTS
    MESSAGE_TO = 5, // SEND MESSAGE FROM CLIENT A TO CLIENT B, VIA SERVER
    CMD_TO = 6, // SEND MESSAGE FROM CLIENT A TO CLIENT B, VIA SERVER
    N_CMDS,
    CLOSE = -1, // CLOSE CONNECTION
    NONE = -2
};

enum EquipamentTypes {
    MAIN_SERVER = -2,
    TEST_SERVER = -3
};

enum EquipamentGroups {
    GROUP_SERVER = -2,
    GROUP_CLIENT = -3
};


} // namespace JsonCommandServer

#endif // _SERVER_COMMANDS_LIST_H

