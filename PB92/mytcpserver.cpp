#include "mytcpserver.h"
#include <QThread>

MyTcpServer::MyTcpServer(QObject *parent): QObject(parent)
{
    tcpServer = new QTcpServer(this);
    connect(tcpServer, &QTcpServer::newConnection, this, &MyTcpServer::onNewConnection);//光电图片数据模拟软件V0.1
    if(!tcpServer->listen(QHostAddress::Any, 16205))
    {
        qDebug()<<"未开始监听";
    }
    else
    {
        qDebug()<<"开始监听";
    }
    sendTimer = new QTimer(this);
    connect(sendTimer, &QTimer::timeout, this, &MyTcpServer::sendDataToClients);

}

MyTcpServer::~MyTcpServer()
{

}

void MyTcpServer::startwrite()
{
    sendTimer->start(500); // 每秒发送一次数据

}

void MyTcpServer::stopwrite()
{
    sendTimer->stop();
}

void MyTcpServer::Disconnect()
{
    tcpSocket->disconnectFromHost();//断开连接);
}

void MyTcpServer::set_data(QByteArray data)
{
    Data = data;
    // qDebug()<<"aaa";
}


void MyTcpServer::onNewConnection()
{
    emit to_str("已监听到客户端",-1);
    emit to_lisent(true);
    tcpSocket = tcpServer->nextPendingConnection();
    tcpSocket->setParent(this);

    connect(tcpSocket, &QTcpSocket::disconnected, this, [=]
    {
        //让该线程中的套接字断开连接
        if(sendTimer->isActive())
        {
            qDebug() << "disconnected!";
            sendTimer->stop();
        }
        tcpSocket->disconnectFromHost();//断开连接
        emit to_lisent(false);
    });
    //    tcpSocket->setSocketOption(QAbstractSocket::SendBufferSizeSocketOption, 60 * 1024 * 1024);
    //    tcpSocket->setSocketOption(QAbstractSocket::LowDelayOption, 1);
    bytesWritten = 0;
    blockSize = 2  * 1024 * 1024;  // 1MB per chunk

}

void MyTcpServer::sendDataToClients()
{
    QString str;
    if (!tcpSocket || tcpSocket->state() != QAbstractSocket::ConnectedState)
    {
        qDebug() << "No client connected or socket is not in connected state!";
        str = "No client connected or socket is not in connected state!";


        emit to_str(str,num);
        emit to_lisent(false);
        sendTimer->stop();

    }
    else {
//        if (bytesWritten >= Data.size()) {
//            // 全部发送完成
//            bytesWritten = 0;
//            num = 0;
//            qDebug() << "File sent successfully!";
//            return;
//        }

//        // 计算剩余数据大小
//        qint64 remaining = Data.size() - bytesWritten;
//        qint64 chunkSize = qMin(remaining, blockSize);

//        // 发送当前块
//        qint64 written = tcpSocket->write(Data.constData() + bytesWritten, chunkSize);
//        if (written == -1) {
//            qDebug() << "Write error:" << tcpSocket->errorString();
//            return;
//        }

//        bytesWritten += written;

        qint64 write = tcpSocket->write(Data);
        tcpSocket->waitForBytesWritten();

        num++;
        qDebug()<<num;
        emit to_str(str,num);
    }



}

