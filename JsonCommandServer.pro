#-------------------------------------------------
#
# Project created by QtCreator 2015-02-12T19:42:40
#
#-------------------------------------------------

QT       += core network

QT       -= gui

TARGET = JsonCommandServer
TEMPLATE = lib

DEFINES += JSONCOMMANDSERVER_LIBRARY

SOURCES += jsoncommandserver.cpp \
    server/base_server.cpp \
    commands_controller.cpp \
    client/base_client.cpp

HEADERS += jsoncommandserver.h\
        jsoncommandserver_global.h \
    commands_controller.h \
    server/base_server.h \
    client/base_client.h

INCLUDEPATH += server \
    client \
    . \
    ..

unix {
    target.path = /usr/lib
    INSTALLS += target
}
