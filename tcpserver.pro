#-------------------------------------------------
#
# Project created by QtCreator 2017-05-24T00:50:01
#
#-------------------------------------------------

QT       += core gui network

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = tcpserver
TEMPLATE = app


SOURCES += main.cpp\
        serverwindow.cpp \
    socketclients.cpp \
    socketserver.cpp \
    fileserver.cpp

HEADERS  += serverwindow.h \
    socketclients.h \
    socketserver.h \
    fileserver.h

FORMS    += serverwindow.ui

RESOURCES += \
    qdarkstyle/style.qrc

INCLUDEPATH += $$PWD/qmhd/include
DEPENDPATH += $$PWD/qmhd/include

win32: LIBS +=  -L$$PWD/qmhd/lib/ \
                -lqmhd \
                -lmicrohttpd \
                -lsetupapi \
                -lwsock32 \
                -lws2_32 \
                -lwinmm
