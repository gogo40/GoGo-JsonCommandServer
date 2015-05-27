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

#include "base_server.h"

#include <QTime>
#include <QtNetwork>
//#include <QMessageBox>

static const int N_MAX_SERVER_MESSAGES = 50;

JsonCommandServer::BaseServer::BaseServer(QObject *_parent)
    : QObject(_parent),
      BaseController(),
      tcp_server_(0),
      network_session_(0),
      next_key_(0),
      n_messages_(0),
      n_max_clients_(100) {
    connect(this, SIGNAL(dataReceived(QTcpSocket*,QString)), SLOT(processMessage(QTcpSocket*,QString)));
}

JsonCommandServer::BaseServer::~BaseServer() {
}

void JsonCommandServer::BaseServer::initServer() {
    QNetworkConfigurationManager manager;
    if (manager.capabilities() & QNetworkConfigurationManager::NetworkSessionRequired) {
        // Get saved network configuration
        QSettings settings(QSettings::UserScope, QLatin1String("MiningControlServer"));
        settings.beginGroup(QLatin1String("MiningControlNetwork"));
        const QString id = settings.value(QLatin1String("DefaultNetworkConfiguration")).toString();
        settings.endGroup();
        // If the saved network configuration is not currently discovered use the system default
        QNetworkConfiguration config = manager.configurationFromIdentifier(id);
        if ((config.state() & QNetworkConfiguration::Discovered) !=
                QNetworkConfiguration::Discovered) {
            config = manager.defaultConfiguration();
        }
        network_session_ = new QNetworkSession(config, this);
        connect(network_session_, SIGNAL(opened()), this, SLOT(sessionOpened()));
        this->addStatusMessage(tr("Opening network session."));
        network_session_->open();
    } else {
        sessionOpened();
    }
    connect(tcp_server_, SIGNAL(newConnection()), this, SLOT(sendInitialMessage()));
}

QString JsonCommandServer::BaseServer::myIP() {
    return ip_address_;
}

int JsonCommandServer::BaseServer::myPort() {
    return port_server_;
}


void JsonCommandServer::BaseServer::sessionOpened() {
    load();
    // Save the used configuration
    if (network_session_) {
        QNetworkConfiguration config = network_session_->configuration();
        QString id;
        if (config.type() == QNetworkConfiguration::UserChoice)
            id = network_session_->sessionProperty(QLatin1String("UserChoiceConfiguration")).toString();
        else
            id = config.identifier();
        QSettings settings(QSettings::UserScope, QLatin1String("MiningControlServer"));
        settings.beginGroup(QLatin1String("MiningControlNetwork"));
        settings.setValue(QLatin1String("DefaultNetworkConfiguration"), id);
        settings.endGroup();
    }
    tcp_server_ = new QTcpServer(this);
    if (!tcp_server_->listen(QHostAddress::Any, this->port_server_)) {
        this->addErrorMessage(tr("Não foi possível iniciar o servidor: %1.")
                              .arg(tcp_server_->errorString()));
        return;
    }
    ip_address_ = QString();
    QList<QHostAddress> ipAddressesList = QNetworkInterface::allAddresses();
    // use the first non-localhost IPv4 address
    for (int i = 0; i < ipAddressesList.size(); ++i) {
        if (ipAddressesList.at(i) != QHostAddress::LocalHost &&
                ipAddressesList.at(i).toIPv4Address()) {
            ip_address_ = ipAddressesList.at(i).toString();
            break;
        }
    }
    // if we did not find one, use IPv4 localhost
    if (ip_address_.isEmpty())
        ip_address_ = QHostAddress(QHostAddress::LocalHost).toString();
    this->addStatusMessage(tr("O servidor está rodando!\n\nIP: %1\nPorta: %2\n\n"
                              "O sistema já está apto para receber dados dos clientes.")
                           .arg(ip_address_)
                           .arg(QString::number(tcp_server_->serverPort())));
}

void JsonCommandServer::BaseServer::sendInitialMessage() {
    QTcpSocket *client_connection = tcp_server_->nextPendingConnection();
    this->addStatusMessage("Cliente conectado: " +
                           client_connection->peerName() + "@" +
                           client_connection->peerAddress().toString() +
                           ": " +
                           QString::number(client_connection->peerPort()) +
                           "\n");
    connect(client_connection, SIGNAL(disconnected()),
            client_connection, SLOT(deleteLater()));
    if (numSockets() >= this->n_max_clients_) {
        QString error_message = "Atingindo numero máximo de clientes suportados!";
        bool ok = false;
        this->addErrorMessage(error_message);
        QJsonArray cmd = createError(error_message, ok);
        if (ok) {
            writeMessage(client_connection, cmd);
        }
        client_connection->disconnectFromHost();
        return;
    }
    connect(client_connection, SIGNAL(readyRead()),
            this, SLOT(receiveMessage()));
    connect(client_connection, SIGNAL(error(QAbstractSocket::SocketError)),
            this, SLOT(displayError(QAbstractSocket::SocketError)));
    QString message = "conectado";
    bool ok = false;
    QJsonArray cmd = createStatus(message, ok);
    if (ok) {
        writeMessage(client_connection, cmd);
    }
    addSocket(client_connection);
}

void JsonCommandServer::BaseServer::writeMessage(QTcpSocket *_socket, const QString & _message) {
    if (_socket->state() == QAbstractSocket::ConnectedState) {
        QByteArray data = _message.toLocal8Bit();
        _socket->write(IntToArray(data.size()));
        _socket->write(data);
        //_socket->waitForBytesWritten();
    }
}

void JsonCommandServer::BaseServer::receiveMessage() {
    QTcpSocket* socket = static_cast<QTcpSocket*>(sender());
    QByteArray* buffer = buffers_.value(socket);
    qint32* s = sizes_.value(socket);
    qint32 size = *s;
    while (socket->bytesAvailable() > 0) {
        buffer->append(socket->readAll());
        bool cond1 = (size == 0 && buffer->size() >= 4);
        bool cond2 = (size > 0 && buffer->size() >= size);
        while (cond1 || cond2) {
            if (cond1) {
                size = ArrayToInt(buffer->mid(0, 4));
                *s = size;
                buffer->remove(0, 4);
                //QMessageBox::warning(0, "PACOTE RECEBIDO SERVIDOR -- ", "SIZE = " + QString::number(size));
            } else {
                QByteArray data = buffer->mid(0, size);
                buffer->remove(0, size);
                size = 0;
                *s = size;
                QString message(data);
                //QMessageBox::warning(0, "PACOTE RECEBIDO SERVIDOR", "SIZE = " + QString::number(size) +
                //                   "Messagem recebida: {" + message + "}");
                this->addStatusMessage("Messagem recebida: {" + message + "}");
                emit dataReceived(socket,message);
            }
            cond1 = (size == 0 && buffer->size() >= 4);
            cond2 = (size > 0 && buffer->size() >= size);
        }
    }
}

void JsonCommandServer::BaseServer::updateServer() {
    save();
    closeServer();
    initServer();
}


void JsonCommandServer::BaseServer::closeServer() {
    this->clearMessages();
    clients_test_messages_.clear();
    ips_info_.clear();
    ips_socket_.clear();
    socket_ips_.clear();
    peers_.clear();
    this->updateInfos();
    if (tcp_server_) delete tcp_server_;
    if (network_session_) delete network_session_;
    tcp_server_ = 0;
    network_session_ = 0;
    next_key_ = 0;
}

void JsonCommandServer::BaseServer::sendMessageTo(const QString& from, const QString &to, const QString &message) {
    bool ok;
    if (to == "Todos") {
        broadcastMessage(createMessage(from, message, ok));
    } else {
        QTcpSocket* socket = getPeer(to);
        if (socket) {
            writeMessage(socket, createMessage(from, message, ok));
        }
    }
    addClientMessage(from + " --> " + to + "> " + message);
}

void JsonCommandServer::BaseServer::sendCommandTo(const QString &from, const QString &to, const QJsonArray &cmd) {
    if (to == "Todos") {
        broadcastMessage(cmd);
    } else {
        QTcpSocket* socket = getPeer(to);
        if (socket) {
            writeMessage(socket, cmd);
        }
    }
    //addStatusMessage("cmd "+ from + " --> " + to + " >> " + QJsonDocument(cmd).toJson());
}

void JsonCommandServer::BaseServer::displayError(QAbstractSocket::SocketError socketError) {
    QTcpSocket* socket = static_cast<QTcpSocket*>(sender());
    eraseSocket(socket);
    switch (socketError) {
    case QAbstractSocket::RemoteHostClosedError:
        this->addErrorMessage(tr("%1 fechou a conexão.")
                              .arg(socket->peerAddress().toString() + ":" +
                                   QString::number(socket->peerPort()) + "@" +
                                   socket->peerName()));
        break;
    case QAbstractSocket::HostNotFoundError:
        this->addErrorMessage(tr("%1 não encontrado. Por favor, verifique as configurações "
                                 "de host e porta.")
                              .arg(socket->peerAddress().toString() + ":" +
                                   QString::number(socket->peerPort()) + "@" +
                                   socket->peerName()));
        break;
    case QAbstractSocket::ConnectionRefusedError:
        this->addErrorMessage(tr("A conexão foi recusado pelo servidor (%1). "
                                 "Verifique se o servidor está rodando, "
                                 "e confirme as configurações de host e porta. ")
                              .arg(socket->peerAddress().toString() + ":" +
                                   QString::number(socket->peerPort()) + "@" +
                                   socket->peerName()));
        break;
    default:
        this->addErrorMessage(tr("Um erro ocorreu de comunicação com %2: %1.")
                              .arg(socket->errorString())
                              .arg(socket->peerAddress().toString() + ":" +
                                   QString::number(socket->peerPort()) + "@" +
                                   socket->peerName()));
    }
}


void JsonCommandServer::BaseServer::processMessage(QTcpSocket* _socket, const QString &message) {
    bool ok;
    QJsonArray cmds = convertMessage(message, ok);
    if (ok) {
        for (int i  = 0; i < cmds.size(); ++i) {
            QJsonObject cmd = cmds[i].toObject();
            int type = -1;
            if (cmd.contains("type")) {
                type = cmd["type"].toInt();
            } else {
                continue;
            }
            cmd.insert("ip", _socket->peerAddress().toString());
            cmd.insert("port", _socket->peerPort());
            if (type == -1) continue;
            if (type == CLOSE) {
                eraseSocket(_socket);
                _socket->disconnectFromHost();
            } else {
                execute_command(type, this, cmd);
            }
        }
    } else {
        if (message.size() > 0) {
            addErrorMessage("Falha na execução do comando: <" + message + ">");
        }
    }
}

QJsonArray JsonCommandServer::BaseServer::convertMessage(const QString &message, bool &ok) {
    ok = false;
    QJsonArray out;
    std::string str = message.toStdString();
    QByteArray json_str(str.c_str(), str.size());
    QJsonDocument doc = QJsonDocument::fromJson(json_str);
    if (!doc.isNull()) {
        ok = true;
        return doc.array();
    }
    return out;
}

void JsonCommandServer::BaseServer::writeMessage(QTcpSocket *_socket, const QJsonArray &cmd) {
    writeMessage(_socket, QJsonDocument(cmd).toJson());
}

QJsonArray JsonCommandServer::BaseServer::createMessage(const QString &from, const QString &message, bool &ok, int type_message) {
    ok = true;
    QJsonArray out;
    QJsonObject cmd;
    if (message == "close") {
        cmd.insert("type", CLOSE);
    } else {
        cmd.insert("id", newKey());
        cmd.insert("ip", this->myIP());
        cmd.insert("port", this->myPort());
        cmd.insert("type", type_message);
        cmd.insert("time", QTime::currentTime().toString());
        cmd.insert("date", QDate::currentDate().toString());
        cmd.insert("id_client", this->id());
        cmd.insert("group_client", this->group());
        cmd.insert("name_client", from);
        cmd.insert("type_client", this->type());
        cmd.insert("message", message);
    }
    out.append(cmd);
    return out;
}

/* Commands */
QJsonArray JsonCommandServer::BaseServer::createMessage(const QString &message, bool &ok, int type_message) {
    ok = true;
    QJsonArray out;
    QJsonObject cmd;
    if (message == "close") {
        cmd.insert("type", CLOSE);
    } else {
        cmd.insert("id", newKey());
        cmd.insert("ip", this->myIP());
        cmd.insert("port", this->myPort());
        cmd.insert("type", type_message);
        cmd.insert("time", QTime::currentTime().toString());
        cmd.insert("date", QDate::currentDate().toString());
        cmd.insert("id_client", this->id());
        cmd.insert("group_client", this->group());
        cmd.insert("name_client", this->name());
        cmd.insert("type_client", this->type());
        cmd.insert("message", message);
    }
    out.append(cmd);
    return out;
}

QJsonArray JsonCommandServer::BaseServer::createStatus(const QString &message, bool &ok) {
    return createMessage(message, ok, MESSAGE_STATUS);
}

QJsonArray JsonCommandServer::BaseServer::createError(const QString &message, bool &ok) {
    return createMessage(message, ok, MESSAGE_ERROR);
}

QJsonArray JsonCommandServer::BaseServer::createPeerList() {
    QJsonArray out;
    QJsonObject cmd;
    QList<QString> peers = getPeers();
    cmd.insert("id", newKey());
    cmd.insert("ip", this->myIP());
    cmd.insert("port", this->myPort());
    cmd.insert("type", MESSAGE_PEER_LIST);
    cmd.insert("time", QTime::currentTime().toString());
    cmd.insert("date", QDate::currentDate().toString());
    QJsonArray peers_array;
    for (int i  = 0; i < peers.size(); ++i) {
        peers_array.append(peers[i]);
    }
    cmd.insert("peers", peers_array);
    out.append(cmd);
    return out;
}

QJsonArray JsonCommandServer::BaseServer::createIdentify() {
    QJsonArray out;
    QJsonObject cmd;
    cmd.insert("id", newKey());
    cmd.insert("ip", this->myIP());
    cmd.insert("port", this->myPort());
    cmd.insert("type", MESSAGE_IDENTIFY);
    cmd.insert("time", QTime::currentTime().toString());
    cmd.insert("date", QDate::currentDate().toString());
    cmd.insert("id_client", this->id());
    cmd.insert("group_client", this->group());
    cmd.insert("name_client", this->name());
    cmd.insert("type_client", this->type());
    cmd.insert("description_client", this->description());
    out.append(cmd);
    return out;
}

QJsonArray JsonCommandServer::BaseServer::createMessageTo(const QString &from, const QString &to, const QString &message) {
    QJsonArray out;
    QJsonObject cmd;
    cmd.insert("id", newKey());
    cmd.insert("ip", this->myIP());
    cmd.insert("port", this->myPort());
    cmd.insert("type", MESSAGE_TO);
    cmd.insert("time", QTime::currentTime().toString());
    cmd.insert("date", QDate::currentDate().toString());
    cmd.insert("id_client", this->id());
    cmd.insert("group_client", this->group());
    cmd.insert("name_client", this->name());
    cmd.insert("type_client", this->type());
    cmd.insert("description_client", this->description());
    cmd.insert("from", from);
    cmd.insert("to", to);
    cmd.insert("message", message);
    out.append(cmd);
    return out;
}

QJsonArray JsonCommandServer::BaseServer::createCommandTo(const QString &from, const QString &to, const QJsonArray & _cmd) {
    QJsonArray out;
    QJsonObject cmd;
    cmd.insert("id", newKey());
    cmd.insert("ip", this->myIP());
    cmd.insert("port", this->myPort());
    cmd.insert("type", CMD_TO);
    cmd.insert("time", QTime::currentTime().toString());
    cmd.insert("date", QDate::currentDate().toString());
    cmd.insert("id_client", this->id());
    cmd.insert("group_client", this->group());
    cmd.insert("name_client", this->name());
    cmd.insert("type_client", this->type());
    cmd.insert("description_client", this->description());
    cmd.insert("from", from);
    cmd.insert("to", to);
    cmd.insert("cmd", _cmd);
    out.append(cmd);
    return out;
}

void JsonCommandServer::BaseServer::executeCommand(const QJsonArray &cmd) {
}

void JsonCommandServer::BaseServer::setPortServer(int _port_server) {
    this->port_server_ = _port_server;
}

void JsonCommandServer::BaseServer::setIPServer(const QString &_IP) {
    this->ip_address_= _IP;
}

void JsonCommandServer::BaseServer::addSocketMessage(QTcpSocket *_socket, const QString &_message) {
    this->clients_test_messages_[_socket] = _message;
}

void JsonCommandServer::BaseServer::addSocket(QTcpSocket *_socket) {
    QString IP = _socket->peerAddress().toString();
    int port = _socket->peerPort();
    this->ips_socket_[IP][port] = _socket;
    this->socket_ips_[_socket] = IP;
    this->peers_['@' + IP + ":" + QString::number(port)] = _socket;
    QByteArray* buffer = new QByteArray;
    qint32* s = new qint32(0);
    buffers_.insert(_socket, buffer);
    sizes_.insert(_socket, s);
    updateInfos();
    broadcastMessage(createPeerList());
}

QTcpSocket* JsonCommandServer::BaseServer::getPeer(const QString &_peer) {
    if (this->peers_.find(_peer) != this->peers_.end()) {
        return this->peers_[_peer];
    }
    return 0;
}

QList<QString> JsonCommandServer::BaseServer::getPeers() {
    QList<QString> list;
    for (std::map<QString, QTcpSocket*>::iterator it = peers_.begin(); it != peers_.end(); ++it) {
        list.push_back(it->first);
    }
    return list;
}

void JsonCommandServer::BaseServer::broadcastMessage(const QJsonArray &cmd) {
    for (std::map<QTcpSocket*, QString>::iterator it = socket_ips_.begin(); it != socket_ips_.end(); ++it) {
        QTcpSocket* socket = it->first;
        writeMessage(socket, cmd);
    }
}

void JsonCommandServer::BaseServer::broadcastMessage(const QString &message) {
    for (std::map<QTcpSocket*, QString>::iterator it = socket_ips_.begin(); it != socket_ips_.end(); ++it) {
        QTcpSocket* socket = it->first;
        writeMessage(socket, message);
    }
}

void JsonCommandServer::BaseServer::eraseSocket(QTcpSocket *_socket) {
    QString IP = _socket->peerAddress().toString();
    QString name = "";
    int port = _socket->peerPort();
    this->clients_test_messages_.erase(_socket);
    this->socket_ips_.erase(_socket);
    this->ips_socket_[IP].erase(port);
    if (this->ips_socket_[IP].size() == 0) {
        this->ips_socket_.erase(IP);
    }
    name = ips_info_[IP][port].name;
    this->ips_info_[IP].erase(port);
    if (this->ips_info_[IP].size() == 0) {
        this->ips_info_.erase(IP);
    }
    this->peers_.erase("@" + IP + ":" + QString::number(port));
    this->peers_.erase(name + "@" + IP + ":" + QString::number(port));
    qint32* s = sizes_.value(_socket);
    QByteArray* buffer = buffers_.value(_socket);
    delete s;
    delete buffer;
    buffers_.remove(_socket);
    sizes_.remove(_socket);
    this->updateInfos();
    broadcastMessage(createPeerList());
}

int JsonCommandServer::BaseServer::numSockets() {
    return this->clients_test_messages_.size();
}

void JsonCommandServer::BaseServer::setNMaxClients(int _n_max_clients) {
    this->n_max_clients_ = _n_max_clients;
}

void JsonCommandServer::BaseServer::addNewInfo(const RemoteNodeInfo &new_info) {
    RemoteNodeInfo& info = ips_info_[new_info.IP][new_info.port];
    info.date = new_info.date;
    info.description = new_info.description;
    info.group = new_info.group;
    info.id = new_info.id;
    info.IP = new_info.IP;
    info.name = new_info.name;
    info.port = new_info.port;
    info.time = new_info.time;
    info.type = new_info.type;
    this->peers_.erase("@" + info.IP + ":" + QString::number(info.port));
    this->peers_[info.name + "@" + info.IP + ":" + QString::number(info.port)] =
        ips_socket_[info.IP][info.port];
    this->updateInfos();
    broadcastMessage(createPeerList());
}


int JsonCommandServer::BaseServer::newKey() {
    ++next_key_;
    return next_key_;
}

void JsonCommandServer::BaseServer::newMessage() {
    ++n_messages_;
    if (n_messages_ > N_MAX_SERVER_MESSAGES) {
        n_messages_ = 0;
        this->clearMessages();
    }
}

