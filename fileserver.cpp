#include <QCoreApplication>
#include <QThread>
#include <QTimer>
#include <QFile>
#include <QByteArray>
#include <QDebug>
#include <QCryptographicHash>
#include "fileserver.h"

QString FileServer::mFirmware("");

FileServer::FileServer(QObject* parent)
    : QMHDController(parent)
{
    mServer = NULL;
    mRouter = NULL;
}

FileServer::~FileServer()
{
    stopServer();
}

int FileServer::startServer(int port)
{
    mServer = new QMHDServer(qApp);
    mRouter = new QMHDRouter(qApp);

    mRouter->addRoute("GET", "/firmware.bin", this, SLOT(sendFile()));
    mRouter->connect(mServer, &QMHDServer::newRequest,
                    mRouter, &QMHDRouter::processRequest,
                    Qt::DirectConnection);

    if (!mServer->listen(port)) {
        qDebug()<<tr("Failed to create server");
    }

    return 0;
}

int FileServer::stopServer()
{
    if(mServer) {
        mServer->close();
        mServer->deleteLater();
    }

    if(mRouter) {
        mRouter->deleteLater();
    }

    return 0;
}

int FileServer::setFile(QString filepath)
{
    mMutex.lock();

    mFirmware = filepath;

    mMutex.unlock();

    return 0;
}

void FileServer::sendFile()
{
    mMutex.lock();

    if(mFirmware.isEmpty()) {
        qDebug()<<tr("Please set file path first!");
        response()->setStatus(QMHD::HttpStatus::NotFound);
        response()->send();
    } else {
        QFile file(mFirmware);
        if(!file.open(QIODevice::ReadOnly)) {
            qDebug()<<tr("Open file:%1 failed!").arg(mFirmware);
            response()->setStatus(QMHD::HttpStatus::NotFound);
            response()->send();
        } else {
            QByteArray fileByte = file.readAll();
            file.close();
            response()->setHeader("Content-Type", "application/octet-stream");
            response()->setHeader("X-Thread-Id", QString::number((quint64) QThread::currentThreadId()));
            response()->send(fileByte);
        }
    }

    mMutex.unlock();
}
