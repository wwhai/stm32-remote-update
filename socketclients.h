#ifndef SOCKETCLIENTS_H
#define SOCKETCLIENTS_H

#include <QObject>
#include <QDebug>
#include <QString>
#include <QHash>
#include <QTcpServer>
#include <QTcpSocket>

class SocketClients : public QObject
{
    Q_OBJECT
public:
    explicit SocketClients(QObject *parent = 0);
    ~SocketClients();

    int addNewClient(QString info, QTcpSocket *socket);
    int removeClient(QString info);
    int removeAllClients();

    QTcpSocket* getClientFromInfo(QString info);

private:
    QHash<QString, QTcpSocket*> mClients;

signals:

public slots:
};

#endif // SOCKETCLIENTS_H
