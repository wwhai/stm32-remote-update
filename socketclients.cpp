#include "socketclients.h"

SocketClients::SocketClients(QObject *parent) : QObject(parent)
{

}

SocketClients::~SocketClients()
{

}

int SocketClients::addNewClient(QString info, QTcpSocket *socket)
{
    mClients[info] = socket;

    return 0;
}

int SocketClients::removeClient(QString info)
{
    return mClients.remove(info);
}

int SocketClients::removeAllClients()
{

    return 0;
}

QTcpSocket *SocketClients::getClientFromInfo(QString info)
{
    return mClients[info];
}
