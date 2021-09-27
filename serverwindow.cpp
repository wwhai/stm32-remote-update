#include "serverwindow.h"
#include "ui_serverwindow.h"
#include "QThread"

ServerWindow::ServerWindow(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::ServerWindow)
{
    ui->setupUi(this);
    this->setWindowTitle(tr("STM32 Remote Update Server"));

    mFile = NULL;
    mFilerServer = NULL;
    mFinished = true;
    mTcpSocket = NULL;
    mFileSize = 0;
    mFileSent = 0;
    mUpdateCmd = tr("select file first");

    mTcpServer = new QTcpServer(this);
    ui->progressBar_tcp->setMaximum(100);
    ui->progressBar_tcp->setMinimum(0);
    ui->progressBar_tcp->setValue(0);

    connect(mTcpServer, SIGNAL(newConnection()), this, SLOT(new_client_request()));
}

ServerWindow::~ServerWindow()
{
    delete ui;
    destroyHttpd();
}

void ServerWindow::mousePressEvent(QMouseEvent *event) {
    m_nMouseClick_X_Coordinate = event->x();
    m_nMouseClick_Y_Coordinate = event->y();
}

void ServerWindow::mouseMoveEvent(QMouseEvent *event) {
    move(event->globalX()- m_nMouseClick_X_Coordinate,event->globalY()-m_nMouseClick_Y_Coordinate);
}

void ServerWindow::generateUpdateCmd(QString cmd, QString filepath)
{
    QJsonObject json;
    json.insert(QString("cmd"), QString(cmd));

    QFile file(filepath);
    if(filepath.isEmpty() || !file.open(QIODevice::ReadOnly)) {
        json.insert("url", "NONE");
        json.insert("md5", "NONE");
    } else {
        QByteArray fileByte = file.readAll();
        QString md5 = QString(QCryptographicHash::hash(fileByte, QCryptographicHash::Md5).toHex());
        file.close();

        json.insert("url", ui->lineEdit_FileUrl->text());
        json.insert("md5", md5);
    }

    QJsonDocument document;
    document.setObject(json);
    QByteArray byte_array = document.toJson(QJsonDocument::Compact);
    mUpdateCmd = QString(byte_array);

    if(0 == QString::compare(QString(CMD_TYPE_UPDATE), cmd)) {
        ui->lineEdit_updatecmd->setText(mUpdateCmd);
        ui->lineEdit_updatecmd->show();
    }
}

int ServerWindow::destroyHttpd()
{
    if(mFilerServer) {
        mFilerServer->stopServer();
        delete mFilerServer;
        mFilerServer = NULL;
        return 0;
    }

    return -1;
}

void ServerWindow::showMessageNoFile()
{
    QMessageBox::warning(NULL, "warning",
                         "Please set update file first!",
                         QMessageBox::Yes, QMessageBox::Yes);
}

void ServerWindow::showMessageNoClient()
{
    QMessageBox::warning(NULL, "warning",
                         "Please select a valid client",
                         QMessageBox::Yes, QMessageBox::Yes);
}

void ServerWindow::disconnectSocket(QTcpSocket* socket)
{
    if(socket == nullptr) {
        return;
    }

    QString peerinfo = tr("%1:%2").arg(socket->peerAddress().toString()).arg(socket->peerPort());
    int tIndex = ui->comboBox->findText(peerinfo);

    ui->comboBox->removeItem(tIndex);
    mSocketClients.removeClient(peerinfo);
    socket->deleteLater();
}

void ServerWindow::on_lost_connection()
{
    QTcpSocket* tSocket = qobject_cast<QTcpSocket*>(sender());
    disconnectSocket(tSocket);
}

void ServerWindow::on_pushButton_browser_clicked()
{
    QFileDialog* tFileDialog = new QFileDialog(this);
    tFileDialog->setWindowTitle(tr("Select update binary file"));
    tFileDialog->setAcceptMode(QFileDialog::AcceptOpen);
    tFileDialog->setFileMode(QFileDialog::AnyFile);
    tFileDialog->setViewMode(QFileDialog::Detail);
    //tFileDialog->setDirectory(QDir::currentPath());

    if(tFileDialog->exec() == QDialog::Accepted) {
        QString path = tFileDialog->selectedFiles()[0];

        mFilerServer->setFile(path);

        generateUpdateCmd(CMD_TYPE_UPDATE, path);
        qDebug() << path;
        ui->lineEdit_file->setText(path);
        ui->lineEdit_file->show();
    }

    delete tFileDialog;
}

void ServerWindow::on_pushButton_close_clicked()
{
    this->close();
}

void ServerWindow::on_pushButton_min_clicked()
{
    this->showMinimized();
}

void ServerWindow::on_pushButton_bind_clicked()
{
    if(mTcpServer->isListening()) {
        mTcpServer->close();
        ui->textBrowser->insertPlainText(tr("Update server closed!\n"));
    } else {
        if(!mTcpServer->listen(QHostAddress::Any, ui->lineEdit_port->text().toInt()))
        {
            mTcpServer->close();
            qDebug() << tr("listen error!");
            return;
        }
        ui->textBrowser->insertPlainText(tr("Update server created!\n"));
    }
}

void ServerWindow::on_pushButton_clear_clicked()
{
    ui->textBrowser->clear();
}

void ServerWindow::new_client_request()
{
    QTcpSocket* newSocket = mTcpServer->nextPendingConnection();

    QString peerinfo = tr("%1:%2").arg(newSocket->peerAddress().toString()).arg(newSocket->peerPort());

    qDebug() << peerinfo;
    mSocketClients.addNewClient(peerinfo, newSocket);
    ui->comboBox->addItem(peerinfo);

    ui->textBrowser->insertPlainText(tr("New client connected!\n"));

    connect(newSocket, SIGNAL(connected()), this, SLOT(new_client_connected()));
    connect(newSocket, SIGNAL(readyRead()), this, SLOT(got_new_data()));
    connect(newSocket, SIGNAL(disconnected()), this, SLOT(on_lost_connection()));
    connect(newSocket, SIGNAL(bytesWritten(qint64)), this, SLOT(on_socket_wirten(qint64)));
}

void ServerWindow::new_client_connected()
{
    ui->textBrowser->insertPlainText(tr("New client connected!\n"));
}

void ServerWindow::got_new_data()
{
    QTcpSocket* tSocket = qobject_cast<QTcpSocket*>(sender());
    QString res;

    res += tSocket->readAll();
    ui->textBrowser->moveCursor(QTextCursor::End);
    ui->textBrowser->textCursor().insertText(res);
}

void ServerWindow::on_pushButton_clear_in_clicked()
{
    ui->lineEdit_out->clear();
}

void ServerWindow::on_pushButton_send_clicked()
{
    if(!mTcpSocket) {
        showMessageNoClient();
    } else {
        QTextStream tOutStream(mTcpSocket);
        tOutStream << ui->lineEdit_out->text();
    }
}

void ServerWindow::on_socket_wirten(qint64)
{
    if(mFinished) {
        return;
    }

    if(!mTcpSocket) {
        showMessageNoClient();
        return;
    }

    if(!mFile) {
        showMessageNoFile();
        return;
    }

    char tFileBuf[FILE_BLOCK_SIZE] = {0};
    int readLen =  mFile->read(tFileBuf, FILE_BLOCK_SIZE);
    if(readLen != FILE_BLOCK_SIZE) {
        mFinished = true;
        ui->progressBar_tcp->setValue(100);
    }

    mTcpSocket->write(tFileBuf, readLen);

    mFileSent += readLen;
    ui->progressBar_tcp->setValue(100 * (mFileSent/mFileSize));
}

void ServerWindow::on_pushButton_send_file_clicked()
{
    mFile = new QFile(ui->lineEdit_file->text());
    if(!(mFile->open(QFile::ReadOnly))){
        ui->textBrowser->insertPlainText(tr("open input file error!\n"));
    } else {
        ui->progressBar_tcp->setValue(0);
        ui->textBrowser->insertPlainText(tr("Successed to open update file!\n"));
        mFinished = false;
        mFileSize = mFile->size();
        mFileSent = 0;
        on_socket_wirten(0);
    }
}

void ServerWindow::on_pushButton_update_clicked()
{
    if(!mTcpSocket) {
        showMessageNoClient();
    } else {
        QTextStream tOutStream(mTcpSocket);
        tOutStream << ui->lineEdit_updatecmd->text();
    }
}

void ServerWindow::on_comboBox_currentIndexChanged(const QString &arg1)
{
    qDebug() << arg1;
    mTcpSocket = mSocketClients.getClientFromInfo(arg1);
    if(mTcpSocket != nullptr) {
        qDebug() << mTcpSocket;
    } else {
        qDebug() << tr("No a valid client");
    }
}

void ServerWindow::on_pushButton_loader_clicked()
{
    generateUpdateCmd(CMD_TYPE_LOADER, FILE_NAME_NONE);

    if(!mTcpSocket) {
        showMessageNoClient();
    } else {
        QTextStream tOutStream(mTcpSocket);
        tOutStream << mUpdateCmd;
    }
}

void ServerWindow::on_pushButton_app_clicked()
{
    generateUpdateCmd(CMD_TYPE_RUNAPP, FILE_NAME_NONE);

    if(!mTcpSocket) {
        showMessageNoClient();
    } else {
        QTextStream tOutStream(mTcpSocket);
        tOutStream << mUpdateCmd;
    }
}

void ServerWindow::on_pushButton_disconnect_clicked()
{
    disconnectSocket(mTcpSocket);
}

void ServerWindow::on_pushButton_httpd_clicked()
{
    if(0 != destroyHttpd()) {
        mFilerServer = new FileServer();
        mFilerServer->startServer(ui->lineEdit_HttpdPort->text().toInt());
    }
}
